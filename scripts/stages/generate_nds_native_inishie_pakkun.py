#!/usr/bin/env python3
"""Generate/check Mushroom Kingdom Pakkun's exact native quad program.

The live item owns file 155 root 0x0B40.  Its immutable list loads the
file-107 palette, calls one segment-E material program, then draws one quad.
The MObj flags are ALPHA only, so the material hook contributes exactly the
current file-155 sprite image; texture_id_curr remains source-owned/live.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402
from native_stage_descriptors.inishie import DESCRIPTOR  # noqa: E402

OUT = REPO / "src/nds/nds_native_inishie_pakkun.generated.inc"
TYPED155 = REPO / "decomp/BattleShip-main/decomp/src/relocData/155_StageInishieFile3.c"
OBJDRAW = REPO / "decomp/BattleShip-main/decomp/src/sys/objdisplay.c"

ROOT = 0x0B40
VTX = 0x0B00
PALETTE = 0x3620
FRAME_OFFSETS = (0x0968, 0x0860, 0x0758)
FRAME_BYTES = (256, 264, 264)
EXPECTED_OPS = (
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8,
    0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
    0xF2, 0xDE, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)
EXPECTED_VERTS = (
    (180, 180, -24, 1056, 1056, 0xCCCCCCFF),
    (180, -240, -24, 1056, 32, 0xCCCCCCFF),
    (-180, -240, -24, 32, 32, 0xCCCCCCFF),
    (-180, 180, -24, 32, 1056, 0xCCCCCCFF),
)
EXPECTED_TRIS = ((3, 2, 1), (0, 3, 1))


def load(key: str):
    e = DESCRIPTOR.o2r_inputs[key]
    return sm.load_o2r(
        REPO,
        sm.InputSpec(
            e["path"], e["sha256"], e.get("file_id"),
            e.get("internal_fixups"), e.get("external_fixups"),
            e.get("payload_sha256"),
        ),
    )


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def decode():
    actor = load("stage_actors")
    geometry = load("stage_geometry")
    raw = words_at(actor.payload, ROOT, len(EXPECTED_OPS))
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != EXPECTED_OPS:
        raise RuntimeError(f"Pakkun 0x0B40 opcode census changed: {ops!r}")

    palette_ref = actor.pointer_at(ROOT + 11 * 8 + 4)
    vtx_ref = actor.pointer_at(ROOT + 21 * 8 + 4)
    if palette_ref is None or (palette_ref.asset_id, palette_ref.offset) != (107, PALETTE):
        raise RuntimeError(f"Pakkun palette ref changed: {palette_ref!r}")
    if vtx_ref is None or (vtx_ref.asset_id, vtx_ref.offset) != (155, VTX):
        raise RuntimeError(f"Pakkun vertex ref changed: {vtx_ref!r}")
    if raw[17] != (0xDE000000, 0x0E000000):
        raise RuntimeError(f"Pakkun material hook changed: {raw[17]!r}")
    if raw[22] != (0x06060402, 0x00000602):
        raise RuntimeError(f"Pakkun triangle word changed: {raw[22]!r}")

    verts = tuple(sm.decode_vertex(actor, VTX + i * 16) for i in range(4))
    if verts != EXPECTED_VERTS:
        raise RuntimeError(f"Pakkun vertices changed: {verts!r}")
    tris = tuple(sm.decode_triangles(0x06, *raw[22]))
    if tris != EXPECTED_TRIS:
        raise RuntimeError(f"Pakkun triangles changed: {tris!r}")
    if len(geometry.payload) < PALETTE + 32:
        raise RuntimeError("file 107 no longer contains the 16-entry Pakkun palette")

    text = TYPED155.read_text()
    for token in (
        "u8 dStageInishieFile3_Tex_0x0758[264]",
        "u8 dStageInishieFile3_Tex_0x0860[264]",
        "u8 dStageInishieFile3_Tex_0x0968[256]",
        "0x0000, 2, 2,",
        "0x0001, 2, 0, 0x0010, 0x0020, 0x0020, 0x0020,",
        "Gfx dStageInishieFile3_DL_0x0B40[30]",
        "{ 1, (void *)dStageInishieFile3_DL_0x0B40",
    ):
        if token not in text:
            raise RuntimeError(f"file155 typed pin missing {token!r}")
    obj = OBJDRAW.read_text()
    for token in (
        "if (flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA))",
        "mobj->sub.sprites[mobj->texture_id_curr]",
    ):
        if token not in obj:
            raise RuntimeError(f"objdisplay material pin missing {token!r}")
    return raw, verts


def render(raw, verts) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Mushroom Kingdom Pakkun native item packet (generated).")
    a(" * Source: SHA-pinned file 155 root 0x0B40 + file 107 palette 0x3620.")
    a(" * Material hook is live MOBJ_FLAG_ALPHA current-image selection only.")
    a(" * Do not hand-edit; regenerate with generate_nds_native_inishie_pakkun.py. */")
    a("#define NDS_NATIVE_INISHIE_PAKKUN_ROOT 0x0b40u")
    a("#define NDS_NATIVE_INISHIE_PAKKUN_PALETTE_OFFSET 0x3620u")
    a("#define NDS_NATIVE_INISHIE_PAKKUN_MOBJ_FLAGS 0x0001u")
    a("#define NDS_NATIVE_INISHIE_PAKKUN_VERTEX_COUNT 4u")
    a("#define NDS_NATIVE_INISHIE_PAKKUN_TRIANGLE_COUNT 2u")
    a("#define NDS_NATIVE_INISHIE_PAKKUN_CORNER_COUNT 6u")
    a("")
    a("static const u16 sNdsNativeInishiePakkunTriIndices[6] =")
    a("{")
    for tri in EXPECTED_TRIS:
        for v in tri:
            a(f"    {v}u,")
    a("};")
    a("")
    a("static const s16 sNdsNativeInishiePakkunVerts[20] =")
    a("{")
    for v in verts:
        a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a("static const u32 sNdsNativeInishiePakkunVertColors[4] =")
    a("{")
    for v in verts:
        a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a("static const u16 sNdsNativeInishiePakkunFrameOffsets[3] =")
    a("{")
    for off in FRAME_OFFSETS:
        a(f"    0x{off:04x}u,")
    a("};")
    a("static const u16 sNdsNativeInishiePakkunFrameBytes[3] =")
    a("{")
    for size in FRAME_BYTES:
        a(f"    {size}u,")
    a("};")
    a("")
    a("static void ndsNativeInishiePakkunSetup(")
    a("    NDSRendererStats *stats, const void *palette)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) | 0x{raw[1][1]:08x}u;")
    for i in (2, 3, 4):
        a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[i][0] >> 24:02x}u, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetCombine(stats, 0x{raw[5][0]:08x}u, 0x{raw[5][1]:08x}u);")
    a(f"    stats->blend_color = 0x{raw[6][1]:08x}u;")
    for i in (8, 9, 10):
        a(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[11][0]:08x}u, (u32)(uintptr_t)palette);")
    a(f"    ndsRendererRecordLoadTlut(stats, 0x{raw[13][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{raw[15][0]:08x}u, 0x{raw[15][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{raw[16][0]:08x}u, 0x{raw[16][1]:08x}u);")
    a("}")
    a("")
    a("static void ndsNativeInishiePakkunSetupTail(NDSRendererStats *stats)")
    a("{")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{raw[19][0]:08x}u, 0x{raw[19][1]:08x}u);")
    a("}")
    a("")
    a("static void ndsNativeInishiePakkunFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) | 0x{raw[25][1]:08x}u;")
    for i in (26, 27, 28):
        a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[i][0] >> 24:02x}u, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    a("}")
    a("")
    a("/* census: dl_words=30 verts=4 tris=2 frames=3 material_flags=0x0001 */")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    raw, verts = decode()
    text = render(raw, verts)
    if args.emit:
        OUT.write_text(text)
        print(f"emitted {OUT.relative_to(REPO)}")
    if args.check or not args.emit:
        if not OUT.exists() or OUT.read_text() != text:
            raise RuntimeError(f"generated packet stale: {OUT.relative_to(REPO)}")
        print("INISHIE_PAKKUN_NATIVE_OK root=0x0b40 verts=4 tris=2 frames=3 flags=0x0001")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
