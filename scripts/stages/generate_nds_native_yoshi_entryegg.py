#!/usr/bin/env python3
"""Generate/check Yoshi's animated match-entry egg native owner."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))
import generate_nds_native_stage as sm  # noqa: E402

OUT = REPO / "src/nds/generated/nds_native_yoshi_entryegg.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_yoshi_entryegg.generated.h"
SPEC = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/YoshiSpecial2",
    "1a155408894ef7010e1ec8d9fe04b92cb1fc6560bb194b2c4afc4345e71086b3",
    0x162, 10, 0,
    "e9b179dacb35328e727478040cbad19512b12ac271829e30ca918957b32cc0f2")

ASSET = 354
ROOT = 0x0530
WORDS = 25
PALETTE = 0x0030
VERTEX = 0x04F0
MATERIAL_EFFECTS = 0x1600  # CURRENT_IMAGE | RENDER_TILE_SIZE | TEXTURE
PALETTE_WORD = 9
MATERIAL_WORD = 13
LOADBLOCK_WORD = 15
VERTEX_WORD = 17
TRI_WORD = 18
EXPECTED_OPS = (
    0xE7,0xD9,0xE3,0xE2,0xFC,0xE8,0xF5,0xF5,0xF5,0xFD,
    0xE6,0xF0,0xE7,0xDE,0xE6,0xF3,0xE7,0x01,0x06,0xE7,
    0xE7,0xD9,0xE3,0xE2,0xDF)
EXPECTED_VERTS = (
    (0,-16,90,0,0,0xffffffff),
    (0,-16,-90,1024,0,0xffffffff),
    (0,164,-90,1023,1024,0xffffffff),
    (0,164,90,0,1023,0xffffffff),
)
EXPECTED_TRIS = ((3,2,1),(1,0,3))
EXPECTED_ROOT_FIXUPS = (
    (ROOT + PALETTE_WORD*8 + 4, ASSET, PALETTE),
    (ROOT + VERTEX_WORD*8 + 4, ASSET, VERTEX),
)

def decode():
    f = sm.load_o2r(REPO, SPEC)
    raw = [struct.unpack_from(">II", f.payload, ROOT+i*8) for i in range(WORDS)]
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != EXPECTED_OPS:
        raise RuntimeError(f"Yoshi entry egg opcode census changed: {ops!r}")
    fixes = tuple((slot, ref.asset_id, ref.offset)
                  for slot, ref in sorted(f.internal.items())
                  if ROOT <= slot < ROOT + WORDS*8)
    if fixes != EXPECTED_ROOT_FIXUPS:
        raise RuntimeError(f"Yoshi entry egg root fixups changed: {fixes!r}")
    if raw[MATERIAL_WORD] != (0xDE000000, 0x0E000000):
        raise RuntimeError("Yoshi entry egg segment-E material hook changed")
    verts = tuple(sm.decode_vertex(f, VERTEX+i*16) for i in range(4))
    if verts != EXPECTED_VERTS:
        raise RuntimeError(f"Yoshi entry egg vertices changed: {verts!r}")
    tris = tuple(sm.decode_triangles(raw[TRI_WORD][0] >> 24, *raw[TRI_WORD]))
    if tris != EXPECTED_TRIS:
        raise RuntimeError(f"Yoshi entry egg triangles changed: {tris!r}")
    return raw, verts, tris

def header(raw, verts, tris):
    return f"""/* Yoshi entry egg native constants (generated). */
#ifndef NDS_NATIVE_YOSHI_ENTRYEGG_GENERATED_H
#define NDS_NATIVE_YOSHI_ENTRYEGG_GENERATED_H
#define NDS_NATIVE_YOSHI_ENTRYEGG_ASSET {ASSET}u
#define NDS_NATIVE_YOSHI_ENTRYEGG_ROOT 0x{ROOT:04x}u
#define NDS_NATIVE_YOSHI_ENTRYEGG_PALETTE_OFFSET 0x{PALETTE:04x}u
#define NDS_NATIVE_YOSHI_ENTRYEGG_VERTEX_OFFSET 0x{VERTEX:04x}u
#define NDS_NATIVE_YOSHI_ENTRYEGG_PALETTE_W0 0x{raw[PALETTE_WORD][0]:08x}u
#define NDS_NATIVE_YOSHI_ENTRYEGG_VERTEX_W0 0x{raw[VERTEX_WORD][0]:08x}u
#define NDS_NATIVE_YOSHI_ENTRYEGG_MATERIAL_EFFECTS 0x{MATERIAL_EFFECTS:04x}u
#define NDS_NATIVE_YOSHI_ENTRYEGG_VERTEX_COUNT {len(verts)}u
#define NDS_NATIVE_YOSHI_ENTRYEGG_TRIANGLE_COUNT {len(tris)}u
#define NDS_NATIVE_YOSHI_ENTRYEGG_CORNER_COUNT {len(tris)*3}u
#endif
"""

def packet(raw, verts, tris):
    lines = [
        "/* Yoshi Special2 match-entry egg native packet (generated).",
        " * Fixed root 0x0530; segment-E keeps the live two-frame MObj animation. */",
        "#include <nds/generated/nds_native_yoshi_entryegg.generated.h>", "",
        f"static const u16 sNdsNativeYoshiEntryEggTriIndices[{len(tris)*3}] =", "{"
    ]
    for tri in tris:
        lines.append("    " + " ".join(f"{v}u," for v in tri))
    lines += ["};", "", f"static const s16 sNdsNativeYoshiEntryEggVerts[{len(verts)*5}] =", "{"]
    for v in verts:
        lines.append(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    lines += ["};", "", f"static const u32 sNdsNativeYoshiEntryEggVertColors[{len(verts)}] =", "{"]
    for v in verts:
        lines.append(f"    0x{v[5]:08x}u,")
    lines += ["};", "",
        "static void ndsNativeYoshiEntryEggSetup(NDSRendererStats *stats, const void *palette)",
        "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) | 0x{raw[1][1]:08x}u;",
        f"    ndsRendererRecordOtherMode(stats, 0x{raw[2][0]>>24:02x}u, 0x{raw[2][0]:08x}u, 0x{raw[2][1]:08x}u);",
        f"    ndsRendererRecordOtherMode(stats, 0x{raw[3][0]>>24:02x}u, 0x{raw[3][0]:08x}u, 0x{raw[3][1]:08x}u);",
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[4][0]:08x}u, 0x{raw[4][1]:08x}u);",
    ]
    for i in (6,7,8):
        lines.append(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    lines += [
        f"    ndsRendererRecordSetImage(stats, 0x{raw[9][0]:08x}u, (u32)(uintptr_t)palette);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[11][1]:08x}u);",
        "}", "",
        "static void ndsNativeYoshiEntryEggAfterMaterial(NDSRendererStats *stats)",
        "{",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[LOADBLOCK_WORD][0]:08x}u, 0x{raw[LOADBLOCK_WORD][1]:08x}u);",
        "}", "",
        "static void ndsNativeYoshiEntryEggFinish(NDSRendererStats *stats)",
        "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[21][0]:08x}u) | 0x{raw[21][1]:08x}u;",
        f"    ndsRendererRecordOtherMode(stats, 0x{raw[22][0]>>24:02x}u, 0x{raw[22][0]:08x}u, 0x{raw[22][1]:08x}u);",
        f"    ndsRendererRecordOtherMode(stats, 0x{raw[23][0]>>24:02x}u, 0x{raw[23][0]:08x}u, 0x{raw[23][1]:08x}u);",
        "}", "",
        "/* census: asset=354 root=0x0530 tris=2 material=0x1600 */", ""
    ]
    return "\n".join(lines)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    raw, verts, tris = decode()
    bodies = ((OUT, packet(raw, verts, tris)), (OUT_HEADER, header(raw, verts, tris)))
    if args.emit:
        for p, body in bodies:
            p.parent.mkdir(parents=True, exist_ok=True)
            if not p.exists() or p.read_text() != body:
                p.write_text(body)
    if args.check or not args.emit:
        for p, body in bodies:
            if not p.exists() or p.read_text() != body:
                raise RuntimeError(f"generated artefact stale: {p.relative_to(REPO)}")
        print("YOSHI_ENTRYEGG_NATIVE_OK asset=354 root=0x0530 tris=2 material=0x1600")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
