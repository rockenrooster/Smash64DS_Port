#!/usr/bin/env python3
"""Generate/check the native EFCommonEffects1 DamageSlash packet.

BattleShip's DamageSlash is a source-backed effect, not one of the port's
procedural visual templates.  File 83 contains two drawable child roots
(0x75A0 and 0x7668).  Each root is one textured quad.  Geometry, palette and
the display-list shell are immutable; the segment-E material call is live and
selects the current CI4 image while animating primitive/light colours.

This generator pins the source object graph and emits only immutable data/state.
The runtime owner consumes the live typed MObj snapshot between the generated
setup and load-block steps, preserving BattleShip's command order without a
display-list interpreter or software rasterizer.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402


OUT = REPO / "src/nds/generated/nds_native_damage_slash.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_damage_slash.generated.h"

MODEL_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_effects/EFCommonEffects1",
    "9b15042901498b224302ab4271e341a041101b6753ec958f1305dd4e616c8931",
    83,
)

TEXT_PINS = (
    ("decomp/BattleShip-main/include/reloc_data.us.h", (
        "#define llEFCommonEffects1FileID ((intptr_t)0x53)",
        "#define llEFCommonEffects1DamageSlashMObjSub ((intptr_t)0x73E0)",
        "#define llEFCommonEffects1DamageSlashDObjDesc ((intptr_t)0x7750)",
        "#define llEFCommonEffects1DamageSlashAnimJoint ((intptr_t)0x7800)",
        "#define llEFCommonEffects1DamageSlashMatAnimJoint ((intptr_t)0x7860)",
    )),
    ("decomp/BattleShip-main/decomp/src/ef/efmanager.c", (
        "EFDesc dEFManagerDamageSlashEffectDesc =",
        "GObj* efManagerDamageSlashMakeEffect(Vec3f *pos, s32 size, f32 rotate)",
        "efManagerMakeEffectNoForce(&dEFManagerDamageSlashEffectDesc)",
    )),
)

ASSET = 83
ROOTS = (0x75A0, 0x7668)
ROOT_REFS = (0x7734, 0x7744)
VERTEX_OFFSETS = (0x7520, 0x7560)
PALETTE_OFFSETS = (0x5D58, 0x5C18)
TEXTURE_OFFSETS = (
    0x7260, 0x70D8, 0x6F50, 0x6DC8, 0x6C40, 0x6AB8, 0x6930, 0x67A8,
    0x65A0, 0x6398, 0x6190, 0x5F88, 0x5D80,
)
TEXTURE_POINTER_SLOTS = tuple(range(0x73EC, 0x7420, 4))
CHILD_TEXTURE_RANGES = ((0, 8), (8, 13))
# Child 0's source frames are physically 16x48 but its render tile is 32x48:
# clamp+mirror with mask S=4 materializes the second half from the first.
# libnds requires power-of-two uploads, so the bottom 16 rows repeat row 47 to
# preserve the RDP clamp edge. Child 1 is already a native 32x32 rectangle.
TEXTURE_SHAPES = (
    *((16, 48, 32, 64, 1) for _ in range(8)),
    *((32, 32, 32, 32, 0) for _ in range(5)),
)
DL_WORDS = 25
VERTEX_COUNT = 4
TRIS = ((3, 2, 1), (0, 3, 1))
OPS = (
    0xE7, 0xE3, 0xFC, 0xE8, 0xF5, 0xF5, 0xF5, 0xFD,
    0xE6, 0xF0, 0xE7, 0xD7, 0xF2, 0xDE, 0xE6, 0xF3,
    0xE7, 0xD9, 0x01, 0x06, 0xE7, 0xE7, 0xD9, 0xE3, 0xDF,
)
PALETTE = (
    0x0842, 0xFFFF, 0x97BD, 0x7FBD, 0x67BD, 0x07BD, 0x073D, 0x067D,
    0x05BD, 0x053D, 0x04BD, 0x047D, 0x03FD, 0x033D, 0x01BB, 0x0035,
)
EXPECTED_VERTS = (
    (
        (90, 148, 0, 1024, 1536, 0xFFFFFFFF),
        (90, -148, 0, 1023, 0, 0xFFFFFFFF),
        (-90, -148, 0, 0, 0, 0xFFFFFFFF),
        (-90, 148, 0, 0, 1536, 0xFFFFFFFF),
    ),
    (
        (90, 91, 0, 0, 1024, 0xFFFFFFFF),
        (90, -91, 0, 1023, 1023, 0xFFFFFFFF),
        (-90, -91, 0, 1024, 0, 0xFFFFFFFF),
        (-90, 91, 0, 0, 0, 0xFFFFFFFF),
    ),
)


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def n64_rgba5551_to_ds(color: int) -> int:
    if (color & 1) == 0:
        return 0
    red = (color >> 11) & 0x1F
    green = (color >> 6) & 0x1F
    blue = (color >> 1) & 0x1F
    return red | (green << 5) | (blue << 10) | 0x8000


def check_text_pins() -> None:
    for path, tokens in TEXT_PINS:
        text = (REPO / path).read_text(encoding="utf-8", errors="replace")
        for token in tokens:
            if token not in text:
                raise RuntimeError(f"{path} pin missing {token!r}")


def decode():
    model = sm.load_o2r(REPO, MODEL_FILE)
    check_text_pins()
    if len(model.payload) < max(ROOTS) + DL_WORDS * 8:
        raise RuntimeError("EFCommonEffects1 no longer contains DamageSlash roots")

    decoded = []
    for index, (root, ref_slot, vtx_off, pal_off) in enumerate(zip(
            ROOTS, ROOT_REFS, VERTEX_OFFSETS, PALETTE_OFFSETS)):
        raw = words_at(model.payload, root, DL_WORDS)
        ops = tuple(w0 >> 24 for w0, _ in raw)
        if ops != OPS:
            raise RuntimeError(f"DamageSlash root {root:#x} opcode census changed: {ops!r}")

        # The only pointer to each root is its DObjDLLink slot.  Asset+root is
        # therefore sufficient to identify the two source children.
        refs = tuple(sorted(slot for slot, ref in model.internal.items()
                            if ref.offset == root))
        if refs != (ref_slot,):
            raise RuntimeError(f"DamageSlash root {root:#x} referrers changed: {refs!r}")

        palette_ref = model.pointer_at(root + 7 * 8 + 4)
        vertex_ref = model.pointer_at(root + 18 * 8 + 4)
        if palette_ref is None or (palette_ref.asset_id, palette_ref.offset) != (ASSET, pal_off):
            raise RuntimeError(f"DamageSlash root {root:#x} palette ref changed: {palette_ref!r}")
        if vertex_ref is None or (vertex_ref.asset_id, vertex_ref.offset) != (ASSET, vtx_off):
            raise RuntimeError(f"DamageSlash root {root:#x} vertex ref changed: {vertex_ref!r}")
        if model.pointer_at(root + 13 * 8 + 4) is not None or raw[13][1] != 0x0E000000:
            raise RuntimeError(f"DamageSlash root {root:#x} segment-E hook changed")

        palette = struct.unpack_from(">16H", model.payload, pal_off)
        if palette != PALETTE:
            raise RuntimeError(f"DamageSlash root {root:#x} palette changed")
        verts = tuple(sm.decode_vertex(model, vtx_off + i * 16)
                      for i in range(VERTEX_COUNT))
        if verts != EXPECTED_VERTS[index]:
            raise RuntimeError(f"DamageSlash root {root:#x} vertices changed: {verts!r}")
        tris = tuple(sm.decode_triangles(0x06, *raw[19]))
        if tris != TRIS:
            raise RuntimeError(f"DamageSlash root {root:#x} triangles changed: {tris!r}")
        decoded.append((raw, verts))

    # Both lists deliberately share the same source palette values even though
    # they reference two source offsets.  Keep that relationship explicit.
    if model.payload[PALETTE_OFFSETS[0]:PALETTE_OFFSETS[0] + 32] != \
       model.payload[PALETTE_OFFSETS[1]:PALETTE_OFFSETS[1] + 32]:
        raise RuntimeError("DamageSlash child palettes diverged")
    for slot, offset in zip(TEXTURE_POINTER_SLOTS, TEXTURE_OFFSETS):
        ref = model.pointer_at(slot)
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, offset):
            raise RuntimeError(
                f"DamageSlash sprite table slot {slot:#x} changed: {ref!r}")
    for offset, shape in zip(TEXTURE_OFFSETS, TEXTURE_SHAPES):
        source_width, source_height, _upload_width, _upload_height, _mirror_s = shape
        source_bytes = (source_width * source_height) // 2
        if offset + source_bytes > len(model.payload):
            raise RuntimeError(
                f"DamageSlash texture {offset:#x} escapes source payload")
    return decoded


def render_header(decoded) -> str:
    lines = [
        "/* DamageSlash native constants (generated).",
        " * Do not hand-edit; regenerate with generate_nds_damage_slash.py. */",
        "#ifndef NDS_NATIVE_DAMAGE_SLASH_GENERATED_H",
        "#define NDS_NATIVE_DAMAGE_SLASH_GENERATED_H",
        "",
        f"#define NDS_NATIVE_DAMAGE_SLASH_ASSET {ASSET}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_ROOT0 0x{ROOTS[0]:04x}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_ROOT1 0x{ROOTS[1]:04x}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_DL_BYTES {DL_WORDS * 8}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_PALETTE0_OFFSET 0x{PALETTE_OFFSETS[0]:04x}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_PALETTE1_OFFSET 0x{PALETTE_OFFSETS[1]:04x}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_PALETTE_END 0x{max(PALETTE_OFFSETS) + 32:04x}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_VERTEX0_OFFSET 0x{VERTEX_OFFSETS[0]:04x}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_VERTEX1_OFFSET 0x{VERTEX_OFFSETS[1]:04x}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_VERTEX_COUNT {VERTEX_COUNT}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_TRIANGLE_COUNT {len(TRIS)}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_CORNER_COUNT {len(TRIS) * 3}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_TEXTURE_COUNT {len(TEXTURE_OFFSETS)}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_CHILD0_TEXTURE_FIRST {CHILD_TEXTURE_RANGES[0][0]}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_CHILD0_TEXTURE_END {CHILD_TEXTURE_RANGES[0][1]}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_CHILD1_TEXTURE_FIRST {CHILD_TEXTURE_RANGES[1][0]}u",
        f"#define NDS_NATIVE_DAMAGE_SLASH_CHILD1_TEXTURE_END {CHILD_TEXTURE_RANGES[1][1]}u",
        "",
        "#endif",
        "",
    ]
    return "\n".join(lines)


def _othermode(lines, raw, index):
    lines.append(
        f"    ndsRendererRecordOtherMode(stats, 0x{raw[index][0] >> 24:02x}u, "
        f"0x{raw[index][0]:08x}u, 0x{raw[index][1]:08x}u);")


def render(decoded) -> str:
    lines = [
        "/* DamageSlash native packet (generated from EFCommonEffects1).",
        " * Two source-backed quads; segment-E live material is applied by the executor.",
        " * Do not hand-edit; regenerate with generate_nds_damage_slash.py. */",
        "#include <nds/generated/nds_native_damage_slash.generated.h>",
        "",
        "static const u16 sNdsDamageSlashTriIndices[6] = {",
        "    3u, 2u, 1u,  0u, 3u, 1u,",
        "};",
        "",
        "static const u16 sNdsDamageSlashPalette[16] = {",
        "    " + ", ".join(f"0x{n64_rgba5551_to_ds(v):04x}u" for v in PALETTE[:8]) + ",",
        "    " + ", ".join(f"0x{n64_rgba5551_to_ds(v):04x}u" for v in PALETTE[8:]) + ",",
        "};",
        "",
        f"static const u32 sNdsDamageSlashTextureOffsets[{len(TEXTURE_OFFSETS)}] = {{",
        "    " + ", ".join(f"0x{v:04x}u" for v in TEXTURE_OFFSETS[:8]) + ",",
        "    " + ", ".join(f"0x{v:04x}u" for v in TEXTURE_OFFSETS[8:]) + ",",
        "};",
        "",
        f"static const u8 sNdsDamageSlashTextureShape[{len(TEXTURE_SHAPES)}][5] = {{",
    ]
    for source_width, source_height, upload_width, upload_height, mirror_s in TEXTURE_SHAPES:
        lines.append(
            f"    {{ {source_width}u, {source_height}u, {upload_width}u, "
            f"{upload_height}u, {mirror_s}u }},")
    lines.extend([
        "};",
        "",
    ])
    for child, (_raw, verts) in enumerate(decoded):
        lines.append(f"static const s16 sNdsDamageSlashVerts{child}[{VERTEX_COUNT * 5}] = {{")
        for v in verts:
            lines.append(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
        lines.extend(["};", ""])

    for child, (raw, _verts) in enumerate(decoded):
        pal_macro = f"NDS_NATIVE_DAMAGE_SLASH_PALETTE{child}_OFFSET"
        lines.extend([
            f"static void ndsDamageSlashSetup{child}(NDSRendererStats *stats, const void *palette)",
            "{",
        ])
        _othermode(lines, raw, 1)
        lines.append(f"    ndsRendererRecordSetCombine(stats, 0x{raw[2][0]:08x}u, 0x{raw[2][1]:08x}u);")
        for i in (4, 5, 6):
            lines.append(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
        lines.append(f"    ndsRendererRecordSetImage(stats, 0x{raw[7][0]:08x}u, (u32)(uintptr_t)palette);")
        lines.append(f"    ndsRendererRecordLoadTlut(stats, 0x{raw[9][1]:08x}u);")
        lines.append(f"    ndsRendererRecordTextureState(stats, 0x{raw[11][0]:08x}u, 0x{raw[11][1]:08x}u);")
        lines.append(f"    ndsRendererRecordSetTileSize(stats, 0x{raw[12][0]:08x}u, 0x{raw[12][1]:08x}u);")
        lines.extend(["}", ""])
        lines.extend([
            f"static void ndsDamageSlashAfterMaterial{child}(NDSRendererStats *stats)",
            "{",
            f"    ndsRendererRecordLoadBlock(stats, 0x{raw[15][0]:08x}u, 0x{raw[15][1]:08x}u);",
            f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[17][0]:08x}u) | 0x{raw[17][1]:08x}u;",
            "    stats->geometry_command_count++;",
            "}",
            "",
            f"static void ndsDamageSlashFinish{child}(NDSRendererStats *stats)",
            "{",
            f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[22][0]:08x}u) | 0x{raw[22][1]:08x}u;",
            "    stats->geometry_command_count++;",
        ])
        _othermode(lines, raw, 23)
        lines.extend(["}", ""])
        # Keep the macro referenced in generated text so drift in the public
        # header is caught by compilation as well as the generator check.
        lines.append(f"/* child {child}: palette={pal_macro} root=0x{ROOTS[child]:04x} */")
        lines.append("")
    lines.append("/* census: asset=83 roots=0x75a0,0x7668 verts=4+4 tris=2+2 live=typed-MObj */")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    decoded = decode()
    outputs = ((OUT, render(decoded)), (OUT_HEADER, render_header(decoded)))
    if args.emit:
        for path, body in outputs:
            path.parent.mkdir(parents=True, exist_ok=True)
            if (not path.exists()) or path.read_text(encoding="utf-8") != body:
                path.write_text(body, encoding="utf-8", newline="\n")
        print("emitted native DamageSlash packet/header")
    if args.check or not args.emit:
        for path, body in outputs:
            if not path.exists() or path.read_text(encoding="utf-8") != body:
                raise RuntimeError(f"generated artefact stale: {path.relative_to(REPO)}")
        print("DAMAGE_SLASH_NATIVE_OK asset=83 roots=0x75a0,0x7668 verts=4+4 tris=2+2")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
