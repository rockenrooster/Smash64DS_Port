"""Bit-exact host-side replica of the port's runtime stage-DObj matrix pipeline.

Task 51 bakes the 42 constant Dream Land stage binding world matrices
host-side so the runtime can emit ``MTX_MULT4x3`` of a baked matrix under a
once-loaded view, instead of CPU-composing ``projection x view x model`` per
binding per frame.  The bake is only correct if the host result is bit-identical
to the runtime ``NDSRendererMatrix20p12`` that
``ndsRendererAdapterBuildDObjWorldMatrixUncached`` produces from the same DObj
descriptor bytes.

This module reproduces, bit-for-bit, the fixed-point pipeline the port runs:

  gSYSinTable            (decomp/.../sys/sintable.c, u16[2048])
  syGetSinCosUShort      (decomp/.../lb/lbdef.h macro)
  FTOFIX32 / scale*256   (decomp/.../include/PR/gu.h, sys/matrix.c)
  syMatrixTraRotRpyRSca  (decomp/.../sys/matrix.c, N64 Mtx packed s15.16)
  MtxCellS16p16          (src/nds/nds_renderer.c, splits integral/fractional)
  MtxLoadN64ToDS20p12    (src/nds/nds_renderer.c, s16.16 -> s20.12, round shift)
  MtxMulAffine20p12      (src/nds/nds_renderer.c, parent-chain compose)
  AdapterMtxIdentity20p12(src/port/reloc_backend_renderer_dl.c)

Every step mirrors the C integer arithmetic in the same width and order; the
differ (Task 49) is the final correctness gate that proves the host bake
matches the runtime for the bindings this task converts.
"""

from __future__ import annotations

import struct
from pathlib import Path

# Fractional-bit counts, copied verbatim from the port sources.
N64_MTX_FRAC_BITS = 16
DS_MTX_FRAC_BITS = 12
N64_TO_DS_SHIFT = N64_MTX_FRAC_BITS - DS_MTX_FRAC_BITS  # 4

# 32-bit signed range (s32 clamp used by MtxMulAffine20p12).
S32_MIN = -(1 << 31)
S32_MAX = (1 << 31) - 1

# The N64 PI constant the decomp uses for SINTABLE_RAD_TO_ID.
PI32 = 3.14159265358979323846


def _load_sint_table(repo_root: Path) -> tuple[int, ...]:
    """Parse ``decomp/.../sys/sintable.c`` into the 2048-entry u16 table.

    The table ships as a verbatim C initializer in the read-only decomp tree;
    parsing it (rather than embedding a copy) keeps a single source of truth.
    """
    source = (repo_root / "decomp" / "BattleShip-main" / "decomp" / "src" /
              "sys" / "sintable.c").read_text(encoding="utf-8")
    values: list[int] = []
    for token in source.replace(",", " ").split():
        token = token.strip()
        if token.startswith("0x") or token.startswith("0X"):
            try:
                values.append(int(token, 16) & 0xFFFF)
            except ValueError:
                continue
    if len(values) != 2048:
        raise ValueError(
            f"gSYSinTable parse yielded {len(values)} entries, expected 2048")
    return tuple(values)


def round_shift_s32(value: int, shift: int) -> int:
    """Replica of ndsRendererRoundShiftS32 (src/nds/nds_renderer.c)."""
    if shift == 0:
        return value
    wide = value
    bias = 1 << (shift - 1)
    if wide < 0:
        return -(((-wide) + bias) >> shift)
    return (wide + bias) >> shift


def round_shift_s64(value: int, shift: int) -> int:
    """Replica of ndsRendererRoundShiftS64 (src/nds/nds_renderer.c)."""
    if shift == 0:
        return value
    bias = 1 << (shift - 1)
    if value < 0:
        return -(((-value) + bias) >> shift)
    return (value + bias) >> shift


def clamp_s64_to_s32(value: int) -> int:
    """Replica of ndsRendererClampS64ToS32 (src/nds/nds_renderer.c)."""
    if value > S32_MAX:
        return S32_MAX
    if value < S32_MIN:
        return S32_MIN
    return value


def _to_u32(value: int) -> int:
    """Mask a Python int to a 32-bit unsigned word."""
    return value & 0xFFFFFFFF


def _combine_integral(a: int, b: int) -> int:
    """COMBINE_INTEGRAL (decomp/.../include/macros.h), as a u32."""
    return _to_u32((_to_u32(a) & 0xFFFF0000) | (_to_u32(b) >> 16))


def _combine_fractional(a: int, b: int) -> int:
    """COMBINE_FRACTIONAL (decomp/.../include/macros.h), as a u32."""
    return _to_u32((_to_u32(a) << 16) | (_to_u32(b) & 0xFFFF))


def _ftofix32(value: float) -> int:
    """FTOFIX32 (decomp/.../include/PR/gu.h): float -> s32 s15.16."""
    bits = struct.pack("<f", float(value))
    f_bits = struct.unpack("<I", bits)[0]
    exponent = (f_bits >> 23) & 0xFF
    if exponent == 0xFF:
        raise ValueError("FTOFIX32 of a non-finite float")
    if exponent == 0:
        return 0
    magnitude = (f_bits & 0x7FFFFF) | 0x800000
    sign = -1 if (f_bits >> 31) else 1
    shift = exponent - 127 - 23 + 16  # N64_MTX_FRAC_BITS
    if shift < 0:
        mag = magnitude >> (-shift)
    else:
        mag = magnitude << shift
    return sign * mag


def _sincos_ushort(angle: float, table: tuple[int, ...]) -> tuple[int, int]:
    """syGetSinCosUShort (decomp/.../lb/lbdef.h), returning signed s32 sin/cos.

    The macro yields u16 table values; the runtime widens them to s32 for the
    fixed-point matrix arithmetic.  Negative angles route through the 0x800
    sign bit the same way the C integer cast does.
    """
    index = int(angle * (len(table) / PI32)) & 0xFFF
    sin_val = table[index & 0x7FF]
    if index & 0x800:
        sin_val = -sin_val
    cos_index = (index + 0x400) & 0xFFF
    cos_val = table[cos_index & 0x7FF]
    if cos_index & 0x800:
        cos_val = -cos_val
    return sin_val, cos_val


class NdsMatrix20p12:
    """NDSRendererMatrix20p12: 4x4 s20.12, m[row][col] as Python ints."""

    __slots__ = ("m",)

    def __init__(self, values: list[list[int]] | None = None):
        if values is None:
            self.m = [[0] * 4 for _ in range(4)]
        else:
            self.m = [list(row) for row in values]

    @classmethod
    def identity(cls) -> "NdsMatrix20p12":
        out = cls()
        for i in range(4):
            out.m[i][i] = 1 << DS_MTX_FRAC_BITS
        return out

    def cell(self, row: int, col: int) -> int:
        return self.m[row][col]

    def is_affine(self) -> bool:
        """The affine precondition MtxMulAffine20p12 demands of both inputs."""
        return (self.m[0][3] == 0 and self.m[1][3] == 0 and
                self.m[2][3] == 0 and
                self.m[3][3] == (1 << DS_MTX_FRAC_BITS))


def build_local_from_descriptor(
    translate: tuple[float, float, float],
    rotate: tuple[float, float, float],
    scale: tuple[float, float, float],
    table: tuple[int, ...],
) -> NdsMatrix20p12:
    """Build a 20.12 local matrix from DObj translate/rotate/scale.

    Replicates syMatrixTraRotRpyRSca (N64 Mtx) then MtxLoadN64ToDS20p12.
    The N64 Mtx packs two s15.16 cells per u32; integral halves live in
    m[0..1], fractional halves in m[2..3] (see MtxCellS16p16).
    """
    tx, ty, tz = translate
    r, p, y = rotate
    sx, sy, sz = scale

    sinr, cosr = _sincos_ushort(r, table)
    sinp, cosp = _sincos_ushort(p, table)
    siny, cosy = _sincos_ushort(y, table)

    scalex = int(sx * 256)
    scaley = int(sy * 256)
    scalez = int(sz * 256)

    # N64 Mtx packed s15.16: ai = m[0..1], af = m[2..3].  pair = row*2 + col/2.
    # syMatrixTraRotRpyRSca writes pairs e1/e2 for adjacent columns.
    ai = [[0] * 4 for _ in range(2)]  # integral halves (rows 0-1)
    af = [[0] * 4 for _ in range(2)]  # fractional halves (rows 2-3)

    # Row 0: [0,0]&[0,1] pair, [0,2]&[0,3] pair.
    e1 = (((cosp * cosy) >> 14) * scalex) >> 8
    e2 = (((cosp * siny) >> 14) * scalex) >> 8
    ai[0][0] = _combine_integral(e1, e2)
    af[0][0] = _combine_fractional(e1, e2)

    e1 = (-sinp * scalex) >> 7
    ai[0][1] = _combine_integral(e1, _ftofix32(0.0))
    af[0][1] = _combine_fractional(e1, _ftofix32(0.0))

    # Row 1.
    e1 = ((((((sinr * sinp) >> 15) * cosy) >> 14) - ((cosr * siny) >> 14))
          * scaley) >> 8
    e2 = ((((((sinr * sinp) >> 15) * siny) >> 14) + ((cosr * cosy) >> 14))
          * scaley) >> 8
    ai[0][2] = _combine_integral(e1, e2)
    af[0][2] = _combine_fractional(e1, e2)

    e1 = (((sinr * cosp) >> 14) * scaley) >> 8
    ai[0][3] = _combine_integral(e1, _ftofix32(0.0))
    af[0][3] = _combine_fractional(e1, _ftofix32(0.0))

    # Row 2.
    e1 = ((((((cosr * sinp) >> 15) * cosy) >> 14) + ((sinr * siny) >> 14))
          * scalez) >> 8
    e2 = ((((((cosr * sinp) >> 15) * siny) >> 14) - ((sinr * cosy) >> 14))
          * scalez) >> 8
    ai[1][0] = _combine_integral(e1, e2)
    af[1][0] = _combine_fractional(e1, e2)

    e1 = (((cosr * cosp) >> 14) * scalez) >> 8
    ai[1][1] = _combine_integral(e1, _ftofix32(0.0))
    af[1][1] = _combine_fractional(e1, _ftofix32(0.0))

    # Row 3 (translation).
    e1 = _ftofix32(tx)
    e2 = _ftofix32(ty)
    ai[1][2] = _combine_integral(e1, e2)
    af[1][2] = _combine_fractional(e1, e2)

    e1 = _ftofix32(tz)
    ai[1][3] = _combine_integral(e1, _ftofix32(1.0))
    af[1][3] = _combine_fractional(e1, _ftofix32(1.0))

    # Now MtxLoadN64ToDS20p12: extract each cell via MtxCellS16p16 then
    # round-shift s16.16 -> s20.12.  MtxCellS16p16 reads
    #   ai_flat[pair], af_flat[pair]  with pair = row*2 + col//2
    # and combines hi16(ai) : hi16(af) (even col) or lo16(ai) : lo16(af).
    out = NdsMatrix20p12()
    for row in range(4):
        for col in range(4):
            pair = row * 2 + col // 2
            ai_row, ai_col = divmod(pair, 4)
            af_row = ai_row  # af mirrors ai's layout
            hi = ai[ai_row][ai_col]
            lo = af[af_row][ai_col]
            if (col & 1) == 0:
                cell_s16p16 = _to_u32((hi & 0xFFFF0000) |
                                      ((lo >> 16) & 0xFFFF))
            else:
                cell_s16p16 = _to_u32(((hi << 16) & 0xFFFF0000) |
                                      (lo & 0xFFFF))
            # Interpret the u32 as s32 (the C function returns s32).
            cell_s32 = cell_s16p16 - (1 << 32) if cell_s16p16 >> 31 else cell_s16p16
            out.m[row][col] = round_shift_s32(cell_s32, N64_TO_DS_SHIFT)
    return out


def mul_affine_20p12(
    lhs: NdsMatrix20p12, rhs: NdsMatrix20p12
) -> NdsMatrix20p12:
    """Replica of ndsRendererMtxMulAffine20p12 for affine inputs.

    Falls back to the full 4x4 path when either input is not affine, matching
    the runtime guard.  The full path is implemented inline for fidelity.
    """
    out = NdsMatrix20p12()
    if lhs.is_affine() and rhs.is_affine():
        for row in range(3):
            for col in range(3):
                total = (lhs.m[row][0] * rhs.m[0][col] +
                         lhs.m[row][1] * rhs.m[1][col] +
                         lhs.m[row][2] * rhs.m[2][col])
                out.m[row][col] = clamp_s64_to_s32(
                    round_shift_s64(total, DS_MTX_FRAC_BITS))
            out.m[row][3] = 0
    # Translation row (verbatim from MtxMulAffine20p12 lines 4840-4850):
    # sum = lhs.m[3][0..2] * rhs.m[0..2][col] + lhs.m[3][3] * rhs.m[3][col].
    for col in range(3):
        total = (lhs.m[3][0] * rhs.m[0][col] +
                 lhs.m[3][1] * rhs.m[1][col] +
                 lhs.m[3][2] * rhs.m[2][col] +
                 lhs.m[3][3] * rhs.m[3][col])
        out.m[3][col] = clamp_s64_to_s32(
            round_shift_s64(total, DS_MTX_FRAC_BITS))
    out.m[3][3] = 1 << DS_MTX_FRAC_BITS
    return out
    # Full 4x4 path (ndsRendererMtxMul20p12).
    for row in range(4):
        for col in range(4):
            total = sum(lhs.m[row][k] * rhs.m[k][col] for k in range(4))
            out.m[row][col] = clamp_s64_to_s32(
                round_shift_s64(total, DS_MTX_FRAC_BITS))
    return out


def build_world_from_chain(
    chain_transforms: list[NdsMatrix20p12],
) -> NdsMatrix20p12:
    """Replica of ndsRendererAdapterBuildDObjWorldMatrixUncached's compose.

    The runtime walks parent -> child (chain[0] = root) and does
    ``MtxMulAffine20p12(&local, out, out)`` starting from identity, i.e.
    out = local[0] * local[1] * ... * local[depth-1] (root-to-self).
    """
    result = NdsMatrix20p12.identity()
    for local in chain_transforms:
        result = mul_affine_20p12(local, result)
    return result
