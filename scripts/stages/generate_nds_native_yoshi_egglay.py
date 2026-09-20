#!/usr/bin/env python3
"""Generate/check Yoshi's Egg Lay victim-egg native effect."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))
import generate_nds_native_stage as sm  # noqa: E402

OUT = REPO / "src/nds/generated/nds_native_yoshi_egglay.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_yoshi_egglay.generated.h"

SPECIAL3 = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/YoshiSpecial3",
    "ff6aabae33167ca660e4942dbdd4051cf057bff48cb820c24fbed1d7a3328ced",
    0x153, 12, 0,
    "078e9e1c63ddb5683f65102c3fec3315989b0ae954e92515f31d76ca9b5df7fa")

ASSET = 339
ROOT = 0x0870
DL_WORDS = 30
PALETTE = 0x0008
IMAGE = 0x0030
VERTEX = 0x0830
PALETTE_WORD = 11
IMAGE_WORD = 17
VERTEX_WORD = 21
TRI_WORD = 22
EXPECTED_OPS = (
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5, 0xF5,
    0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2, 0xFD, 0xE6, 0xF3,
    0xE7, 0x01, 0x06, 0xE7, 0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)
EXPECTED_VERTS = (
    (120, 120, 0, 2047, 2048, 0xFFFFFFFF),
    (120, -120, 0, 2048, 0, 0xFFFFFFFF),
    (-120, -120, 0, 0, 0, 0xFFFFFFFF),
    (-120, 120, 0, 0, 2047, 0xFFFFFFFF),
)
EXPECTED_TRIS = ((3, 2, 1), (1, 0, 3))
EXPECTED_FIXUPS = (
    (ROOT + PALETTE_WORD * 8 + 4, ASSET, PALETTE),
    (ROOT + IMAGE_WORD * 8 + 4, ASSET, IMAGE),
    (ROOT + VERTEX_WORD * 8 + 4, ASSET, VERTEX),
)


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def decode():
    f = sm.load_o2r(REPO, SPECIAL3)
    raw = words_at(f.payload, ROOT, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != EXPECTED_OPS:
        raise RuntimeError(f"Yoshi EggLay opcode census changed: {ops!r}")
    fixups = tuple(
        (slot, ref.asset_id, ref.offset)
        for slot, ref in sorted(f.internal.items())
        if ROOT <= slot < ROOT + DL_WORDS * 8)
    if fixups != EXPECTED_FIXUPS:
        raise RuntimeError(f"Yoshi EggLay root fixups changed: {fixups!r}")
    verts = tuple(sm.decode_vertex(f, VERTEX + i * 16) for i in range(4))
    if verts != EXPECTED_VERTS:
        raise RuntimeError(f"Yoshi EggLay vertices changed: {verts!r}")
    tris = tuple(sm.decode_triangles(raw[TRI_WORD][0] >> 24, *raw[TRI_WORD]))
    if tris != EXPECTED_TRIS:
        raise RuntimeError(f"Yoshi EggLay triangles changed: {tris!r}")
    return raw, verts, tris


def render_header(raw, verts, tris):
    return f"""/* Yoshi Egg Lay native effect constants (generated).
 * Do not hand-edit; regenerate with generate_nds_native_yoshi_egglay.py. */
#ifndef NDS_NATIVE_YOSHI_EGGLAY_GENERATED_H
#define NDS_NATIVE_YOSHI_EGGLAY_GENERATED_H
#define NDS_NATIVE_YOSHI_EGGLAY_ASSET {ASSET}u
#define NDS_NATIVE_YOSHI_EGGLAY_ROOT 0x{ROOT:04x}u
#define NDS_NATIVE_YOSHI_EGGLAY_PALETTE_OFFSET 0x{PALETTE:04x}u
#define NDS_NATIVE_YOSHI_EGGLAY_IMAGE_OFFSET 0x{IMAGE:04x}u
#define NDS_NATIVE_YOSHI_EGGLAY_VERTEX_OFFSET 0x{VERTEX:04x}u
#define NDS_NATIVE_YOSHI_EGGLAY_PALETTE_W0 0x{raw[PALETTE_WORD][0]:08x}u
#define NDS_NATIVE_YOSHI_EGGLAY_IMAGE_W0 0x{raw[IMAGE_WORD][0]:08x}u
#define NDS_NATIVE_YOSHI_EGGLAY_VERTEX_W0 0x{raw[VERTEX_WORD][0]:08x}u
#define NDS_NATIVE_YOSHI_EGGLAY_VERTEX_COUNT {len(verts)}u
#define NDS_NATIVE_YOSHI_EGGLAY_TRIANGLE_COUNT {len(tris)}u
#define NDS_NATIVE_YOSHI_EGGLAY_CORNER_COUNT {len(tris) * 3}u
#endif
"""


def render(raw, verts, tris):
    lines = [
        "/* Yoshi Egg Lay victim-egg native packet (generated).",
        " * Source: SHA-pinned YoshiSpecial3 asset 339 root 0x0870.",
        " * One fixed CI4 64x64 textured quad; DObj animation/transform stays live.",
        " * Do not hand-edit; regenerate with generate_nds_native_yoshi_egglay.py. */",
        "#include <nds/generated/nds_native_yoshi_egglay.generated.h>", "",
        f"static const u16 sNdsNativeYoshiEggLayTriIndices[{len(tris)*3}] =", "{",
    ]
    for tri in tris:
        lines.append("    " + " ".join(f"{v}u," for v in tri))
    lines += ["};", "", f"static const s16 sNdsNativeYoshiEggLayVerts[{len(verts)*5}] =", "{"]
    for v in verts:
        lines.append(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    lines += ["};", "", f"static const u32 sNdsNativeYoshiEggLayVertColors[{len(verts)}] =", "{"]
    for v in verts:
        lines.append(f"    0x{v[5]:08x}u,")
    lines += ["};", "", "static void ndsNativeYoshiEggLaySetup(NDSRendererStats *stats, const void *palette, const void *image)", "{"]
    lines.append(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) | 0x{raw[1][1]:08x}u;")
    for i in (2,3,4):
        lines.append(f"    ndsRendererRecordOtherMode(stats, 0x{raw[i][0]>>24:02x}u, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    lines.append(f"    ndsRendererRecordSetCombine(stats, 0x{raw[5][0]:08x}u, 0x{raw[5][1]:08x}u);")
    lines.append(f"    stats->blend_color = 0x{raw[6][1]:08x}u;")
    for i in (8,9,10):
        lines.append(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    lines.append(f"    ndsRendererRecordSetImage(stats, 0x{raw[11][0]:08x}u, (u32)(uintptr_t)palette);")
    lines.append(f"    ndsRendererRecordLoadTlut(stats, 0x{raw[13][1]:08x}u);")
    lines.append(f"    ndsRendererRecordTextureState(stats, 0x{raw[15][0]:08x}u, 0x{raw[15][1]:08x}u);")
    lines.append(f"    ndsRendererRecordSetTileSize(stats, 0x{raw[16][0]:08x}u, 0x{raw[16][1]:08x}u);")
    lines.append(f"    ndsRendererRecordSetImage(stats, 0x{raw[17][0]:08x}u, (u32)(uintptr_t)image);")
    lines.append(f"    ndsRendererRecordLoadBlock(stats, 0x{raw[19][0]:08x}u, 0x{raw[19][1]:08x}u);")
    lines += ["}", "", "static void ndsNativeYoshiEggLayFinish(NDSRendererStats *stats)", "{"]
    lines.append(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) | 0x{raw[25][1]:08x}u;")
    for i in (26,27,28):
        lines.append(f"    ndsRendererRecordOtherMode(stats, 0x{raw[i][0]>>24:02x}u, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    lines += ["}", "", "/* census: asset=339 root=0x0870 verts=4 tris=2 material=none */", ""]
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser(); ap.add_argument("--emit", action="store_true"); ap.add_argument("--check", action="store_true"); args = ap.parse_args()
    raw, verts, tris = decode(); packet = render(raw, verts, tris); header = render_header(raw, verts, tris)
    if args.emit:
        for p, text in ((OUT, packet), (OUT_HEADER, header)):
            p.parent.mkdir(parents=True, exist_ok=True)
            if not p.exists() or p.read_text() != text: p.write_text(text)
        print(f"emitted {OUT.relative_to(REPO)} and {OUT_HEADER.relative_to(REPO)}")
    if args.check or not args.emit:
        for p, text in ((OUT, packet), (OUT_HEADER, header)):
            if not p.exists() or p.read_text() != text: raise RuntimeError(f"generated artefact stale: {p.relative_to(REPO)}")
        print("YOSHI_EGGLAY_NATIVE_OK asset=339 root=0x0870 verts=4 tris=2 material=none")
    return 0

if __name__ == "__main__": raise SystemExit(main())
