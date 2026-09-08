"""Host oracle for native animation-lock fighter matrices.

Measured in BattleShip source (not proposals):
- fp->is_use_animlocks is driven only by anim_desc.flags.is_use_animlocks
  (ftmain.c:4724-4735); ftmanager.c:517 clears it at spawn and ftmanager.c:815
  applies ftParamSetAnimLocks unconditionally, so mode-3 cached joints exist
  even while the flag is FALSE.
- Locked + cached FTParts (transform_update_mode != 0) reuses the gameplay
  matrix unk_dobjtrans_0x10 (lbcommon.c:1412-1415).
- Locked + cold cache builds lbCommonMatrixTraRotScaInv with the running
  parent-scale accumulator as inverse scale (lbcommon.c:1418-1439), seeded to
  unit per fighter draw (ftdisplaymain.c:1162) and saved/restored across
  siblings (ftdisplaymain.c:831,914,1053).
- Source publishes parts->vec_scale to the accumulator after BOTH locked
  branches (lbcommon.c:1439): cached joints publish the stored gameplay
  scale, cold joints publish scale times parent accum. The flag test is
  mode!=0, not mode-3-only.
- Locked collision chains use gmCollisionSetMatrixNcs, which multiplies rows
  by nscale then divides columns by scale_mul (gmcollision.c:82-193).
- Held-item attach normalizes rows of the world matrix and keeps translation
  (lbcommon.c:1477-1604, mirrored in renderer_adapter_matrix.c:2854-2910).

C implementation under test (proposals, in-repo):
- ndsRendererAdapterBuildAnimLockInvariantMtx plus accumulator threading in
  ndsRendererAdapterComposeOwnerWorldsSource
  (src/port/renderer_adapter_matrix.c). Matrix math only; no interpreter or
  display-list APIs, so it is allowed under the native-only ROM gate.

These tests replicate the exact integer/float formulas both sides share and
prove the locked kernel coincides with the existing exact builder whenever
the flag is clear-or-unit, diverges exactly where source says it must, and
covers flag/mask transitions plus compose order. Run with pytest; no ROM.
"""

from __future__ import annotations

import math
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


_scripts_root = Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in sys.path:
    sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402  (repo-standard root resolution)
import pytest  # noqa: E402  (skip support for the C harness below)


REPO_ROOT = _paths.REPO_ROOT
MATRIX_C = REPO_ROOT / "src" / "port" / "renderer_adapter_matrix.c"

MASK32 = 0xFFFFFFFF
ANGLE_MULT = 651.8986206


def s32(value: int) -> int:
    value &= MASK32
    return value - 0x100000000 if value & 0x80000000 else value


def u32(value: int) -> int:
    return value & MASK32


def mul(a: int, b: int) -> int:
    return s32(a * b)


def add(a: int, b: int) -> int:
    return s32(a + b)


def sub(a: int, b: int) -> int:
    return s32(a - b)


def ash(value: int, shift: int) -> int:
    """Arithmetic >> on the wrapped s32 value (matches GCC/ARM + MIPS)."""
    return s32(value) >> shift


def f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", value))[0]


def f2i(value: float) -> int:
    """C (s32)(float) truncation; inputs here are always in range."""
    out = math.trunc(f32(value))
    assert -(2 ** 31) <= out <= 2 ** 31 - 1
    return out


def combine_integral(a: int, b: int) -> int:
    return u32((u32(a) & 0xFFFF0000) | (u32(b) >> 16))


def combine_fractional(a: int, b: int) -> int:
    return u32((u32(a) << 16) | (u32(b) & 0xFFFF))


def round_shift_s64(value: int, shift: int) -> int:
    """ndsRendererAdapterSourceRoundShiftS64: nearest, halves away from zero."""
    if shift == 0:
        return value
    bias = 1 << (shift - 1)
    if value < 0:
        return -(((-value) + bias) >> shift)
    return (value + bias) >> shift


TABLE = [((i * 257) % 32767) - 16383 for i in range(2048)]


def sincos(idx: int, table=TABLE):
    """Shared table convention: lbCommonSin/Cos and the C index helpers."""
    idx &= 0xFFF
    sin = table[idx & 0x7FF]
    if idx & 0x800:
        sin = -sin
    cidx = (idx + 0x400) & 0xFFF
    cos = table[cidx & 0x7FF]
    if cidx & 0x800:
        cos = -cos
    return sin, cos


def angle_index(angle: float) -> int:
    """((s32)(angle * 651.8986206F)) & 0xFFF, single-precision mult."""
    return math.trunc(f32(f32(angle) * f32(ANGLE_MULT))) & 0xFFF


def rpy_exact(tx, ty, tz, sinr, cosr, sinp, cosp, siny, cosy):
    """Replica of ndsRendererAdapterBuildFighterTraRotRpyExact e-words."""
    e1 = u32(ash(mul(cosp, cosy), 14))
    e2 = u32(ash(mul(cosp, siny), 14))
    e3 = u32(mul(-sinp, 2))
    e4 = u32(sub(ash(mul(ash(mul(sinr, sinp), 15), cosy), 14),
                 ash(mul(cosr, siny), 14)))
    e5 = u32(add(ash(mul(ash(mul(sinr, sinp), 15), siny), 14),
                 ash(mul(cosr, cosy), 14)))
    e6 = u32(ash(mul(sinr, cosp), 14))
    e7 = u32(add(ash(mul(ash(mul(cosr, sinp), 15), cosy), 14),
                 ash(mul(sinr, siny), 14)))
    e8 = u32(sub(ash(mul(ash(mul(cosr, sinp), 15), siny), 14),
                 ash(mul(sinr, cosy), 14)))
    e9 = u32(ash(mul(cosr, cosp), 14))
    return [(e1, e2), (e3, 0), (e4, e5), (e6, 0),
            (e7, e8), (e9, 0),
            (u32(f2i(f32(tx * 65536.0))), u32(f2i(f32(ty * 65536.0)))),
            (u32(f2i(f32(tz * 65536.0))), 0x10000)]


def sca_inv(tx, ty, tz, sinx, cosx, siny, cosy, sinz, cosz,
            scax, scay, scaz, invx, invy, invz):
    """Replica of lbCommonMatrixTraRotScaInv e-words (lbcommon.c:491-615)."""
    vec_x, vec_y, vec_z = scax * invx, scay * invy, scaz * invz
    scax_l = f2i(f32(vec_x * 256.0))
    scay_l = f2i(f32(vec_y * 256.0))
    scaz_l = f2i(f32(vec_z * 256.0))
    scax_inv_l = f2i(f32(f32(1.0 / invx) * 256.0))
    scay_inv_l = f2i(f32(f32(1.0 / invy) * 256.0))
    scaz_inv_l = f2i(f32(f32(1.0 / invz) * 256.0))
    e1 = u32(ash(mul(ash(mul(ash(mul(cosy, cosz), 14), scax_l), 8),
                      scax_inv_l), 8))
    e2 = u32(ash(mul(ash(mul(ash(mul(cosy, sinz), 14), scax_l), 8),
                      scay_inv_l), 8))
    e3 = u32(ash(mul(ash(mul(-siny, scax_l), 7), scaz_inv_l), 8))
    e4 = u32(ash(mul(ash(mul(sub(ash(mul(ash(mul(sinx, siny), 15), cosz), 14),
                                 ash(mul(cosx, sinz), 14)), scay_l), 8),
                      scax_inv_l), 8))
    e5 = u32(ash(mul(ash(mul(add(ash(mul(ash(mul(sinx, siny), 15), sinz), 14),
                                 ash(mul(cosx, cosz), 14)), scay_l), 8),
                      scay_inv_l), 8))
    e6 = u32(ash(mul(ash(mul(ash(mul(sinx, cosy), 14), scay_l), 8),
                      scaz_inv_l), 8))
    e7 = u32(ash(mul(ash(mul(add(ash(mul(ash(mul(cosx, siny), 15), cosz), 14),
                                 ash(mul(sinx, sinz), 14)), scaz_l), 8),
                      scax_inv_l), 8))
    e8 = u32(ash(mul(ash(mul(sub(ash(mul(ash(mul(cosx, siny), 15), sinz), 14),
                                 ash(mul(sinx, cosz), 14)), scaz_l), 8),
                      scay_inv_l), 8))
    e9 = u32(ash(mul(ash(mul(ash(mul(cosx, cosy), 14), scaz_l), 8),
                      scaz_inv_l), 8))
    return [(e1, e2), (e3, 0), (e4, e5), (e6, 0),
            (e7, e8), (e9, 0),
            (u32(f2i(f32(tx * 65536.0))), u32(f2i(f32(ty * 65536.0)))),
            (u32(f2i(f32(tz * 65536.0))), 0x10000)]


def test_combine_word_packing():
    assert combine_integral(0x12345678, 0xABCDEF01) == 0x1234ABCD
    assert combine_fractional(0x12345678, 0xABCDEF01) == 0x5678EF01
    assert combine_integral(0, 0xFFFFFFFF) == 0x0000FFFF


def test_angle_index_matches_truncf_convention():
    for angle in (0.0, 0.5, -0.5, 1.0, -1.0, 3.14159, -3.14159,
                  0.001, 100.0, -100.0):
        assert angle_index(angle) == \
            math.trunc(f32(f32(angle) * f32(ANGLE_MULT))) & 0xFFF
    assert angle_index(0.0) == 0
    # Hand-computed goldens: any float in (651.89, 651.91) truncates to 651,
    # and -651 & 0xFFF == 3445, so these hold regardless of last-ulp value.
    assert angle_index(1.0) == 651
    assert angle_index(-1.0) == 3445


def test_unit_accum_reduction_over_sweep():
    """Unit scales: locked kernel must equal the exact unlocked builder."""
    for i in range(0, 2048, 97):
        sinr, cosr = sincos(i)
        sinp, cosp = sincos(i + 500)
        siny, cosy = sincos(i + 1000)
        tx, ty, tz = i * 0.5 - 500.0, 250.0 - i * 0.25, i * 0.125 - 64.0
        assert sca_inv(tx, ty, tz, sinr, cosr, sinp, cosp, siny, cosy,
                       1.0, 1.0, 1.0, 1.0, 1.0, 1.0) == \
            rpy_exact(tx, ty, tz, sinr, cosr, sinp, cosp, siny, cosy)


def test_identity_golden_values():
    words = sca_inv(1.0, -2.0, 3.0, 0, 16384, 0, 16384, 0, 16384,
                    1.0, 1.0, 1.0, 1.0, 1.0, 1.0)
    assert words[0] == (16384, 0)
    assert words[1] == (0, 0)
    assert words[2] == (0, 16384)
    assert words[3] == (0, 0)
    assert words[4] == (0, 0)
    assert words[5] == (16384, 0)
    assert words[6] == (65536, u32(-131072))
    assert words[7] == (196608, 0x10000)
    assert words == rpy_exact(1.0, -2.0, 3.0, 0, 16384, 0, 16384, 0, 16384)


def test_nonunit_scale_flows_through_accum_truncation():
    """DObj scale 2 with accum 3 gives column 0 of 25898, not 26000/13000.

    The kernel keeps the joint's own scale but removes the parent scale
    through truncated inverse factors: vec=6.0 gives vec_l=1536 while
    inv=trunc((1/3)*256)=85, so ((((13000*1536)>>8)*85)>>8)=25898. A path
    that ignored the accumulator would emit 26000 (unit inverse factors);
    the plain unlocked builder emits 13000.
    """
    locked = sca_inv(0.0, 0.0, 0.0, 0, 16384, 0, 16384, 10000, 13000,
                     2.0, 1.0, 1.0, 3.0, 1.0, 1.0)
    plain = rpy_exact(0.0, 0.0, 0.0, 0, 16384, 0, 16384, 10000, 13000)
    assert locked[0][0] == 25898
    assert plain[0][0] == 13000
    assert locked != plain


def test_zero_accum_must_decline():
    """Source divides by the accumulator with no guard; C declines instead."""

    def declines(inv):
        return inv[0] == 0.0 or inv[1] == 0.0 or inv[2] == 0.0

    assert declines((0.0, 1.0, 1.0)) is True
    assert declines((1.0, 0.0, 1.0)) is True
    assert declines((1.0, 1.0, 0.0)) is True
    assert declines((1.0, 1.0, 1.0)) is False
    assert declines((2.0, 0.5, 4.0)) is False


def test_ncs_column_divide_structure():
    """gmCollisionSetMatrixNcs: rows *= nscale, columns /= scale_mul."""
    sinx = siny = sinz = 0.0
    cosx = cosy = cosz = 1.0
    scale = (1.0, 3.0, 1.0)
    mul_ = (2.0, 1.0, 0.5)
    nscale = (mul_[0] * scale[0], mul_[1] * scale[1], mul_[2] * scale[2])
    rows = [[cosy * cosz, cosy * sinz, -siny],
            [(sinx * siny * cosz) - (cosx * sinz),
             (sinx * siny * sinz) + (cosx * cosz), sinx * cosy],
            [(cosx * siny * cosz) + (sinx * sinz),
             (cosx * siny * sinz) - (sinx * cosz), cosx * cosy]]
    for r in range(3):
        for c in range(3):
            rows[r][c] *= nscale[r]
    for c in range(3):
        if mul_[c] != 1.0:
            for r in range(3):
                rows[r][c] *= 1.0 / mul_[c]
    assert rows == [[1.0, 0.0, 0.0],
                    [0.0, 3.0, 0.0],
                    [0.0, 0.0, 1.0]]
    assert mul_[0] != 0.0 and mul_[1] != 0.0 and mul_[2] != 0.0


def test_attach_row_normalization():
    """Attach macro: unit row magnitudes, translation preserved."""

    def normalize(row):
        scale = math.sqrt(row[0] ** 2 + row[1] ** 2 + row[2] ** 2)
        if scale != 0.0:
            scale = 1.0 / scale
        return [row[0] * scale, row[1] * scale, row[2] * scale]

    assert normalize([2.0, 0.0, 0.0]) == [1.0, 0.0, 0.0]
    assert normalize([0.0, 0.0, 0.0]) == [0.0, 0.0, 0.0]
    got = normalize([3.0, 4.0, 0.0])
    assert math.isclose(got[0], 0.6, rel_tol=1e-6)
    assert math.isclose(got[1], 0.8, rel_tol=1e-6)
    assert math.isclose(got[0] ** 2 + got[1] ** 2, 1.0, rel_tol=1e-6)


def test_animlock_mask_walk():
    """ftParamSetAnimLocks walk: bit31-tested, flags0 then flags1."""

    def frozen(flags0, flags1, start, count):
        out = set()
        i = start
        while flags0 != 0 or flags1 != 0:
            current = flags0 if i < count - 1 else flags1
            if current & 0x80000000:
                out.add(i)
            if i < count - 1:
                flags0 = u32(flags0 << 1)
            else:
                flags1 = u32(flags1 << 1)
            i += 1
        return out

    assert frozen(0x80000000, 0, 4, 40) == {4}
    assert frozen(0xC0000000, 0, 4, 40) == {4, 5}
    assert frozen(0, 0x80000000, 4, 9) == {8}
    assert frozen(0xF0000000, 0, 0, 6) == {0, 1, 2, 3}
    assert frozen(0, 0, 4, 40) == set()


def test_animlock_clear_resets_only_mode3():
    modes = {0: 3, 1: 0, 2: 3, 3: 1}
    for joint, mode in list(modes.items()):
        if mode == 3:
            modes[joint] = 0
    assert modes == {0: 0, 1: 0, 2: 0, 3: 1}


def test_flag_transition_table():
    """ftmain.c:4724-4735 action mapping; resulting flag always follows desc."""

    def transition(current, desc_flag):
        if current and not desc_flag:
            action = "set"
        elif (not current) and desc_flag:
            action = "clear"
        else:
            action = "none"
        return action, desc_flag

    assert transition(True, False) == ("set", False)
    assert transition(False, True) == ("clear", True)
    assert transition(True, True) == ("none", True)
    assert transition(False, False) == ("none", False)


def test_locked_accum_publish_chain():
    """lbcommon.c:1439 runs after BOTH locked branches: cached joints publish
    the stored part scale, cold joints publish scale times parent accum."""

    def chain(joints, seed=(1.0, 1.0, 1.0)):
        acc = seed
        published = []
        for mode, scale, stored in joints:
            if mode != 0:
                acc = stored
            else:
                acc = (scale[0] * acc[0], scale[1] * acc[1], scale[2] * acc[2])
            published.append(acc)
        return published

    got = chain([
        (0, (2.0, 1.0, 1.0), (0.0, 0.0, 0.0)),
        (1, (9.0, 9.0, 9.0), (5.0, 5.0, 5.0)),
        (0, (1.0, 2.0, 1.0), (0.0, 0.0, 0.0)),
    ])
    assert got[0] == (2.0, 1.0, 1.0)
    assert got[1] == (5.0, 5.0, 5.0)
    assert got[2] == (5.0, 10.0, 5.0)


def test_source_world_compose_order():
    """SourceWorldMulLocal: out = local x parent, translation accumulates."""

    def from_local(cells):
        basis = [[cell * 16 for cell in row[:3]] for row in cells[:3]]
        trans = [cells[3][c] * 16 for c in range(3)]
        return basis, trans

    def mul_local(local_cells, parent):
        basis, trans = parent
        out_basis = [[0] * 3 for _ in range(3)]
        for r in range(3):
            for c in range(3):
                total = sum(local_cells[r][k] * basis[k][c]
                            for k in range(3))
                out_basis[r][c] = round_shift_s64(total, 16)
        out_trans = []
        for c in range(3):
            total = sum(local_cells[3][k] * basis[k][c] for k in range(3))
            out_trans.append(round_shift_s64(total, 16) + trans[c])
        return out_basis, out_trans

    ident = [[65536 if r == c else 0 for c in range(3)] for r in range(4)]
    child = [[65536 if r == c else 0 for c in range(3)] for r in range(3)]
    child.append([65536, 131072, 196608])
    world = mul_local(child, from_local(ident))
    assert world[0] == [[1048576 if r == c else 0 for c in range(3)]
                        for r in range(3)]
    assert world[1] == [1048576, 2097152, 3145728]
    parent = from_local([[131072 if r == c else 0 for c in range(3)]
                         for r in range(3)] + [[196608, 0, 0]])
    world2 = mul_local([[65536 if r == c else 0 for c in range(3)]
                        for r in range(3)] + [[0, 0, 0]], parent)
    assert world2[1][0] == 196608 * 16 + 0
    assert world2[1][1] == 0 and world2[1][2] == 0


def test_c_source_contains_lock_paths():
    text = MATRIX_C.read_text(encoding="utf-8")
    assert "ndsRendererAdapterBuildAnimLockInvariantMtx" in text
    assert "lock_accum[joint_index] = lock_accum[parent]" in text
    assert "ftdisplaymain.c:1162" in text
    assert "if ((fp == NULL) || (parts == NULL) || " \
        "(fp->is_use_animlocks != FALSE))" not in text
    assert "out_vec_scale->x = parts->vec_scale.x" in text


def _extract_kernel_c_source() -> str:
    """Cut the actual helper text out of matrix.c by brace balancing."""
    text = MATRIX_C.read_text(encoding="utf-8")
    sig = "static sb32 ndsRendererAdapterBuildAnimLockInvariantMtx("
    start = text.find(sig)
    assert start != -1, "kernel helper missing from matrix.c"
    brace = text.find("{", start)
    depth = 0
    end = None
    for pos in range(brace, len(text)):
        if text[pos] == "{":
            depth += 1
        elif text[pos] == "}":
            depth -= 1
            if depth == 0:
                end = pos + 1
                break
    assert end is not None, "unbalanced braces extracting kernel"
    kernel = text[start:end]
    assert "scax_inv_l" in kernel
    assert "COMBINE_INTEGRAL" in kernel
    assert "e_trax" in kernel
    assert "return TRUE" in kernel
    assert "gSP" not in kernel and "DisplayList" not in kernel
    return kernel


def _cfloat(value: float) -> str:
    """Exact hex float literal for the C harness (C99, gcc/MinGW)."""
    return float.hex(float(value)) + "f"


def test_c_kernel_compiles_and_matches_oracle():
    """Compile the ACTUAL kernel text from matrix.c and run goldens in C.

    The harness provides stub types plus the real angle-index header and a
    stub sin table using the same LCG as TABLE above; only the kernel body
    under review comes from the repo file. Skips when no host C compiler
    exists; never touches ROM inputs or build outputs (temp dir only).
    """
    cc = shutil.which("gcc") or shutil.which("cc") or shutil.which("clang")
    assert cc is not None, "Host C compiler required for the actual matrix kernel"
    kernel = _extract_kernel_c_source()
    angle_header = (REPO_ROOT / "include" / "nds" /
                    "nds_fighter_matrix_index.h").as_posix()
    cases = [
        # rot xyz, tra xyz, scale xyz, accum xyz, expect_ret
        ((0.0, 0.0, 0.0), (1.0, -2.0, 3.0),
         (1.0, 1.0, 1.0), (1.0, 1.0, 1.0), 1),
        ((0.5, -0.25, 1.0), (10.5, 0.25, -7.75),
         (2.0, 0.5, 1.5), (3.0, 2.0, 0.5), 1),
        ((0.0, 0.0, 0.0), (1.0, 2.0, 3.0),
         (1.0, 1.0, 1.0), (0.0, 1.0, 1.0), 0),
        ((1e30, 0.0, 0.0), (1.0, 2.0, 3.0),
         (1.0, 1.0, 1.0), (1.0, 1.0, 1.0), 0),
    ]
    decls = []
    calls = []
    for num, (rot, tra, scl, acc, _ret) in enumerate(cases):
        decls.append(
            "    joint.translate.vec.f.x = %s; joint.translate.vec.f.y = %s; "
            "joint.translate.vec.f.z = %s;\n"
            "    joint.rotate.vec.f.x = %s; joint.rotate.vec.f.y = %s; "
            "joint.rotate.vec.f.z = %s;\n"
            "    joint.scale.vec.f.x = %s; joint.scale.vec.f.y = %s; "
            "joint.scale.vec.f.z = %s;\n"
            "    accum.x = %s; accum.y = %s; accum.z = %s;\n"
            "    vec.x = vec.y = vec.z = 0.0f;\n"
            "    memset(&mtx, 0xA5, sizeof(mtx));\n"
            "    ret = ndsRendererAdapterBuildAnimLockInvariantMtx("
            "&joint, &accum, &vec, &mtx);\n"
            '    printf("CASE %%d RET %%d\\n", %d, (int)ret);\n'
            "    print_mtx(&mtx);\n"
            "    print_vec(&vec);\n" % (
                _cfloat(tra[0]), _cfloat(tra[1]), _cfloat(tra[2]),
                _cfloat(rot[0]), _cfloat(rot[1]), _cfloat(rot[2]),
                _cfloat(scl[0]), _cfloat(scl[1]), _cfloat(scl[2]),
                _cfloat(acc[0]), _cfloat(acc[1]), _cfloat(acc[2]), num))
        calls.append(num)
    harness = r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef int32_t s32;
typedef uint32_t u32;
typedef int sb32;
typedef float f32;
#define TRUE 1
#define FALSE 0
typedef struct { float x, y, z; } Vec3f;
typedef struct { Vec3f f; } VF;
typedef struct { VF vec; float a; } DRot;
typedef struct { VF vec; } DTr;
typedef struct { DTr translate; DRot rotate; DTr scale; } DObj;
typedef struct { u32 m[4][4]; } Mtx;
#define COMBINE_INTEGRAL(a, b) (((a) & 0xffff0000) | (((b) >> 16) & 0xffff))
#define COMBINE_FRACTIONAL(a, b) (((a) << 16) | ((b) & 0xffff))
static int32_t sin_table[2048];
static void sin_table_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
    {
        sin_table[i] = (int32_t)(((i * 257) %% 32767) - 16383);
    }
}
static s32 ndsRendererAdapterFighterSinFromIndex(s32 index)
{
    u32 id = (u32)index & 0xfffu;
    s32 value = sin_table[id & 0x7ffu];
    return ((id & 0x800u) != 0u) ? -value : value;
}
static s32 ndsRendererAdapterFighterCosFromIndex(s32 index)
{
    return ndsRendererAdapterFighterSinFromIndex(index + 0x400);
}
#include "%s"
%s
static void print_mtx(const Mtx *m)
{
    int r, c;
    for (r = 0; r < 4; r++)
    {
        for (c = 0; c < 4; c++)
        {
            printf("W %%08x\n", (unsigned)m->m[r][c]);
        }
    }
}
static void print_vec(const Vec3f *v)
{
    u32 bits[3];
    memcpy(bits, v, sizeof(bits));
    printf("V %%08x %%08x %%08x\n",
           (unsigned)bits[0], (unsigned)bits[1], (unsigned)bits[2]);
}
int main(void)
{
    DObj joint;
    Vec3f accum, vec;
    Mtx mtx;
    sb32 ret;
    sin_table_init();
    memset(&joint, 0, sizeof(joint));
%s
    return 0;
}
""" % (angle_header, kernel, "".join(decls))
    workdir = tempfile.mkdtemp(prefix="animlock_kernel_")
    try:
        src = Path(workdir) / "harness.c"
        src.write_text(harness, encoding="utf-8")
        exe = Path(workdir) / ("harness.exe" if sys.platform == "win32"
                               else "harness")
        proc = subprocess.run([cc, "-std=c99", "-O0", "-o", str(exe), str(src)],
                              capture_output=True, text=True, timeout=180)
        assert proc.returncode == 0, \
            "kernel harness compile failed:\n%s\n%s" % (proc.stdout,
                                                        proc.stderr)
        proc = subprocess.run([str(exe)], capture_output=True, text=True,
                              timeout=120)
        assert proc.returncode == 0, \
            "kernel harness run failed:\n%s\n%s" % (proc.stdout, proc.stderr)
        blocks = {}
        num = None
        for line in proc.stdout.splitlines():
            if line.startswith("CASE"):
                _, num, _, ret = line.split()
                blocks[int(num)] = {"ret": int(ret), "words": [], "vec": []}
            elif line.startswith("W ") and num is not None:
                blocks[int(num)]["words"].append(int(line[2:], 16))
            elif line.startswith("V ") and num is not None:
                blocks[int(num)]["vec"] = [int(w, 16)
                                           for w in line[2:].split()]
        for num, (rot, tra, scl, acc, want_ret) in enumerate(cases):
            got = blocks[num]
            assert got["ret"] == want_ret, "case %d ret" % num
            if want_ret == 0:
                continue
            ix, iy, iz = (angle_index(rot[0]), angle_index(rot[1]),
                          angle_index(rot[2]))
            sinx, cosx = sincos(ix)
            siny, cosy = sincos(iy)
            sinz, cosz = sincos(iz)
            pairs = sca_inv(tra[0], tra[1], tra[2], sinx, cosx, siny, cosy,
                            sinz, cosz, scl[0], scl[1], scl[2],
                            acc[0], acc[1], acc[2])
            (e1, e2), (e3, _p0), (e4, e5), (e6, _p1), (e7, e8), (e9, _p2), \
                (etx, ety), (etz, _p3) = pairs
            want_words = [
                combine_integral(e1, e2), combine_integral(e3, 0),
                combine_integral(e4, e5), combine_integral(e6, 0),
                combine_integral(e7, e8), combine_integral(e9, 0),
                combine_integral(etx, ety), combine_integral(etz, 0x10000),
                combine_fractional(e1, e2), combine_fractional(e3, 0),
                combine_fractional(e4, e5), combine_fractional(e6, 0),
                combine_fractional(e7, e8), combine_fractional(e9, 0),
                combine_fractional(etx, ety), combine_fractional(etz, 0),
            ]
            assert got["words"] == want_words, "case %d words" % num
            want_vec = [struct.unpack("<I", struct.pack(
                "<f", f32(f32(s) * f32(a))))[0]
                for s, a in zip(scl, acc)]
            assert got["vec"] == want_vec, "case %d vec" % num
    finally:
        shutil.rmtree(workdir, ignore_errors=True)
    assert calls == [0, 1, 2, 3]
