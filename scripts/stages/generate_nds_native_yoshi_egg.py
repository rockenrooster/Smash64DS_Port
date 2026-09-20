#!/usr/bin/env python3
"""Generate/check YoshiModel's shared native egg presentation root.

Yoshi's Up-B EggThrow WPAttributes (YoshiMain 0x0c) point across to YoshiModel
root 0xA860.  The same root is used by dEFManagerYoshiShieldEffectDesc and
dEFManagerYoshiEggEscapeEffectDesc, so one exact native owner covers the thrown
egg, Yoshi's source shield egg, and the intro/escape egg that replaces the
hidden fighter body.

The root is a self-contained CI4 64x64 textured quad: palette 0x9ff8, image
0xa020, vertices 0xa820.  There is no MObj, so immutable source geometry/state
is fully baked and only the live DObj transform remains runtime-owned.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402

OUT = REPO / "src/nds/generated/nds_native_yoshi_egg.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_yoshi_egg.generated.h"

MODEL = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/YoshiModel",
    "e2654cbdc969a473de1e78fa392211a4f465657a7c16ffc93e6bb2b073d4b04c",
    0x152, 562, 10,
    "e5694728565f731e677e1012504de38bb9e2140ccf5ffe5f19ae2cbc04de4030")

ASSET = 338
ROOT = 0xA860
DL_WORDS = 29
PALETTE = 0x9FF8
IMAGE = 0xA020
VERTEX = 0xA820
PALETTE_BYTES = 16 * 2
IMAGE_BYTES = (64 * 64) // 2
PALETTE_WORD = 10
IMAGE_WORD = 16
VERTEX_WORD = 21
TRI_WORD = 22

EXPECTED_OPS = (
    0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5,
    0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2,
    0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xE2, 0xDF,
)
EXPECTED_VERTS = (
    (134, 133, 0, 2048, 2048, 0xFFFFFFFF),
    (134, -136, 0, 2048, 0, 0xFFFFFFFF),
    (-134, -136, 0, 0, 0, 0xFFFFFFFF),
    (-134, 133, 0, 0, 2048, 0xFFFFFFFF),
)
EXPECTED_TRIS = ((3, 2, 1), (0, 3, 1))
EXPECTED_ROOT_FIXUPS = (
    (ROOT + PALETTE_WORD * 8 + 4, ASSET, PALETTE),
    (ROOT + IMAGE_WORD * 8 + 4, ASSET, IMAGE),
    (ROOT + VERTEX_WORD * 8 + 4, ASSET, VERTEX),
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


def decode():
    model = sm.load_o2r(REPO, MODEL)
    raw = words_at(model.payload, ROOT, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != EXPECTED_OPS:
        raise RuntimeError(f"Yoshi egg opcode census changed: {ops!r}")
    if (raw[-1][0] >> 24) != 0xDF:
        raise RuntimeError("Yoshi egg root no longer ends in G_ENDDL")

    root_fixups = tuple(
        (slot, ref.asset_id, ref.offset)
        for slot, ref in sorted(model.internal.items())
        if ROOT <= slot < ROOT + DL_WORDS * 8
    )
    if root_fixups != EXPECTED_ROOT_FIXUPS:
        raise RuntimeError(f"Yoshi egg root fixups changed: {root_fixups!r}")

    vtx_w0 = raw[VERTEX_WORD][0]
    count = (vtx_w0 >> 12) & 0xFF
    end = (vtx_w0 >> 1) & 0x7F
    if (count, end) != (4, 4):
        raise RuntimeError(f"Yoshi egg G_VTX changed: {vtx_w0:#010x}")
    verts = tuple(sm.decode_vertex(model, VERTEX + i * 16) for i in range(4))
    if verts != EXPECTED_VERTS:
        raise RuntimeError(f"Yoshi egg vertices changed: {verts!r}")
    tris = tuple(sm.decode_triangles(raw[TRI_WORD][0] >> 24, *raw[TRI_WORD]))
    if tris != EXPECTED_TRIS:
        raise RuntimeError(f"Yoshi egg triangles changed: {tris!r}")
    if (PALETTE + PALETTE_BYTES > len(model.payload) or
            IMAGE + IMAGE_BYTES > len(model.payload)):
        raise RuntimeError("Yoshi egg palette/image payload moved out of bounds")
    palette_source = tuple(
        struct.unpack_from(">H", model.payload, PALETTE + i * 2)[0]
        for i in range(16))
    if ((palette_source[0] & 1) != 0 or
            any((entry & 1) == 0 for entry in palette_source[1:])):
        raise RuntimeError(
            "Yoshi egg PAL16 transparency contract changed: "
            f"{palette_source!r}")
    palette_ds = tuple(n64_rgba5551_to_ds(entry) for entry in palette_source)
    image_source = model.payload[IMAGE:IMAGE + IMAGE_BYTES]
    # N64 CI4 stores the left texel in the high nibble. Nintendo DS PAL16
    # stores the left texel in the low nibble. This byte-local swap is the
    # complete texel conversion; indices and therefore source art stay exact.
    image_ds = bytes(
        ((byte & 0x0F) << 4) | (byte >> 4) for byte in image_source)
    return raw, verts, tris, palette_ds, image_ds


def render_header(raw, verts, tris) -> str:
    return f"""/* Yoshi shared egg native constants (generated).
 * Do not hand-edit; regenerate with generate_nds_native_yoshi_egg.py. */
#ifndef NDS_NATIVE_YOSHI_EGG_GENERATED_H
#define NDS_NATIVE_YOSHI_EGG_GENERATED_H

#define NDS_NATIVE_YOSHI_EGG_ASSET {ASSET}u
#define NDS_NATIVE_YOSHI_EGG_ROOT 0x{ROOT:04x}u
#define NDS_NATIVE_YOSHI_EGG_DL_BYTES {DL_WORDS * 8}u
#define NDS_NATIVE_YOSHI_EGG_PALETTE_OFFSET 0x{PALETTE:04x}u
#define NDS_NATIVE_YOSHI_EGG_IMAGE_OFFSET 0x{IMAGE:04x}u
#define NDS_NATIVE_YOSHI_EGG_VERTEX_OFFSET 0x{VERTEX:04x}u
#define NDS_NATIVE_YOSHI_EGG_PALETTE_BYTES {PALETTE_BYTES}u
#define NDS_NATIVE_YOSHI_EGG_IMAGE_BYTES {IMAGE_BYTES}u
#define NDS_NATIVE_YOSHI_EGG_PALETTE_W0 0x{raw[PALETTE_WORD][0]:08x}u
#define NDS_NATIVE_YOSHI_EGG_IMAGE_W0 0x{raw[IMAGE_WORD][0]:08x}u
#define NDS_NATIVE_YOSHI_EGG_VERTEX_W0 0x{raw[VERTEX_WORD][0]:08x}u
#define NDS_NATIVE_YOSHI_EGG_VERTEX_COUNT {len(verts)}u
#define NDS_NATIVE_YOSHI_EGG_TRIANGLE_COUNT {len(tris)}u
#define NDS_NATIVE_YOSHI_EGG_CORNER_COUNT {len(tris) * 3}u

#endif
"""


def render(raw, verts, tris, palette_ds, image_ds) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Yoshi shared egg native packet (generated).")
    a(" * Source: SHA-pinned YoshiModel asset 338 root 0xA860.")
    a(" * Used by EggThrow and the Yoshi shield/intro egg effects.")
    a(" * The root is one fixed CI4 64x64 textured quad; only DObj transform is live.")
    a(" * The compact battle pack keeps only the root identity cell, so palette and")
    a(" * texels are baked here too. RGBA5551 is converted exactly to DS BGR555+A,")
    a(" * and CI4 nibble order is swapped from N64 high-first to DS low-first.")
    a(" * Do not hand-edit; regenerate with generate_nds_native_yoshi_egg.py. */")
    a("#include <nds/generated/nds_native_yoshi_egg.generated.h>")
    a("")
    a(f"static const u16 sNdsNativeYoshiEggPalette[{len(palette_ds)}] =")
    a("{")
    for start in range(0, len(palette_ds), 8):
        a("    " + ", ".join(
            f"0x{entry:04x}u" for entry in palette_ds[start:start + 8]) + ",")
    a("};")
    a("")
    a(f"static const u8 sNdsNativeYoshiEggTexels[{len(image_ds)}] =")
    a("{")
    for start in range(0, len(image_ds), 16):
        a("    " + ", ".join(
            f"0x{byte:02x}u" for byte in image_ds[start:start + 16]) + ",")
    a("};")
    a("")
    a(f"static const u16 sNdsNativeYoshiEggTriIndices[{len(tris) * 3}] =")
    a("{")
    for tri in tris:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    a(f"static const s16 sNdsNativeYoshiEggVerts[{len(verts) * 5}] =")
    a("{")
    for v in verts:
        a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeYoshiEggVertColors[{len(verts)}] =")
    a("{")
    for v in verts:
        a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a("static void ndsNativeYoshiEggSetup(")
    a("    NDSRendererStats *stats, const void *palette, const void *image)")
    a("{")
    for i in (1, 2, 3):
        a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[i][0] >> 24:02x}u,"
          f" 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetCombine(stats, 0x{raw[4][0]:08x}u,"
      f" 0x{raw[4][1]:08x}u);")
    a(f"    stats->blend_color = 0x{raw[5][1]:08x}u;")
    for i in (7, 8, 9):
        a(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u,"
          f" 0x{raw[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[10][0]:08x}u,"
      "\n        (u32)(uintptr_t)palette);")
    a(f"    ndsRendererRecordLoadTlut(stats, 0x{raw[12][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{raw[14][0]:08x}u,"
      f" 0x{raw[14][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{raw[15][0]:08x}u,"
      f" 0x{raw[15][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[16][0]:08x}u,"
      "\n        (u32)(uintptr_t)image);")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{raw[18][0]:08x}u,"
      f" 0x{raw[18][1]:08x}u);")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[20][0]:08x}u) |"
      f" 0x{raw[20][1]:08x}u;")
    a("}")
    a("")
    a("static void ndsNativeYoshiEggFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) |"
      f" 0x{raw[25][1]:08x}u;")
    for i in (26, 27):
        a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[i][0] >> 24:02x}u,"
          f" 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    a("}")
    a("")
    a(f"/* census: root=0x{ROOT:04x} dl_words={DL_WORDS} verts={len(verts)} "
      f"tris={len(tris)} palette=338:0x{PALETTE:04x} image=338:0x{IMAGE:04x} */")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    raw, verts, tris, palette_ds, image_ds = decode()
    text = render(raw, verts, tris, palette_ds, image_ds)
    header = render_header(raw, verts, tris)
    if args.emit:
        for path, body in ((OUT, text), (OUT_HEADER, header)):
            path.parent.mkdir(parents=True, exist_ok=True)
            if (not path.exists()) or path.read_text() != body:
                path.write_text(body)
        print(f"emitted {OUT.relative_to(REPO)} and {OUT_HEADER.relative_to(REPO)}")
    if args.check or not args.emit:
        for path, body in ((OUT, text), (OUT_HEADER, header)):
            if not path.exists() or path.read_text() != body:
                raise RuntimeError(f"generated artefact stale: {path.relative_to(REPO)}")
        print("YOSHI_EGG_NATIVE_OK asset=338 root=0xa860 verts=4 tris=2 "
              "palette=32 image=2048 material=none")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
