#!/usr/bin/env python3
"""Generate/check Samus Charge Shot's exact native weapon program.

The root looked for two rounds like a mis-resolved relocation and it is not.
`218_SamusSpecial1.c:22` sets `dSamusSpecial1_ChargeShot_WeaponAttributes.data`
to `&dSamusSpecial3_JointVerts_Vtx[4]`, three vertex records short of
`BombDL` at 0x2A0, and `218_SamusSpecial1.reloc:4` carries the matching extern
fixup.  Charge Shot's WPDesc flags are 0x00, so `WEAPON_FLAG_DOBJDESC` is clear
and `wptypes.h:38` says `data` IS the display list; `wpmanager.c:268` attaches
it and `:270` selects `wpDisplayDLHead1`.

DECODED, and the surprise resolves: the forty-eight bytes at 0x270 are not
vertex data being executed as garbage.  They are six perfectly valid GBI words
-- pipesync, two other-mode writes, a set-combine and a blend colour -- that the
source packed into the space ahead of `BombDL` on purpose.  The whole program is
28 words, 0x270 through the ENDDL at 0x348, and it draws ONE textured quad:
four vertices at 0x230 and one G_TRI2.

Everything it binds is internal to file 321 -- palette 0x0008, image 0x0030,
vertex pool 0x0230 -- so unlike the Sector Z laser there is no cross-file fixup
to verify, and unlike the Peach's Castle bumper there is no MObj and therefore
no live palette: `218_SamusSpecial1.c:23` has `p_mobjsubs` NULL, which is why
the failure record carries material 0.  The whole program is fixed; only the
DObj transform is live.

Kirby's copy reaches the same root: `230_KirbySpecial1.c` references
`dSamusSpecial1_ChargeShot_WeaponAttributes`, so an admission keyed on asset and
root serves both without knowing about Kirby at all.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402

OUT = REPO / "src/nds/generated/nds_native_samus_chargeshot.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_samus_chargeshot.generated.h"

SPECIAL3 = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/SamusSpecial3",
    "f040ee234f476c6b7cd335c7f7e00c7007f80dfa53246a454611b40abafab542")

ROOT = 0x270
DL_WORDS = 28
VTX = 0x230
PALETTE = 0x0008
IMAGE = 0x0030

VTX_WORD = 21              # G_VTX, four vertices
TRI_WORD = 22              # one G_TRI2
PALETTE_WORD = 8           # SETTIMG naming the TLUT source
IMAGE_WORD = 14            # SETTIMG naming the CI image

EXPECTED_OPS = (
    0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5,
    0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2,
    0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x06, 0xE7,
    0xD9, 0xE3, 0xE2, 0xDF,
)
EXPECTED_VERTS = (
    (15, 15, 0, 0, 2048, 0xFFFEFFFF),
    (15, -15, 0, 0, 0, 0xFFFEFFFF),
    (-15, -15, 0, 2048, 0, 0xFFFEFFFF),
    (-15, 15, 0, 2048, 2048, 0xFFFEFFFF),
)
EXPECTED_TRIS = ((3, 2, 1), (0, 3, 1))
# Every pointer the program carries, and they are all internal to file 321.
EXPECTED_FIXUPS = (
    (0x02C4, 321, PALETTE),
    (0x02F4, 321, IMAGE),
    (0x031C, 321, VTX),
)


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def decode():
    f = sm.load_o2r(REPO, SPECIAL3)
    if len(f.payload) < ROOT + DL_WORDS * 8:
        raise RuntimeError("file 321 no longer contains the Charge Shot program")
    raw = words_at(f.payload, ROOT, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != EXPECTED_OPS:
        raise RuntimeError(f"Charge Shot opcode census changed: {ops!r}")
    if (raw[DL_WORDS - 1][0] >> 24) != 0xDF:
        raise RuntimeError("Charge Shot program no longer ends in G_ENDDL")

    fixups = tuple(sorted(
        (slot, ref.asset_id, ref.offset) for slot, ref in f.internal.items()))
    if fixups != EXPECTED_FIXUPS:
        raise RuntimeError(f"Charge Shot fixup census changed: {fixups!r}")
    if f.external:
        raise RuntimeError("file 321 gained an external fixup")

    # G_VTX: four vertices ending at cache slot four, so v0 is zero.
    vtx_w0 = raw[VTX_WORD][0]
    count = (vtx_w0 >> 12) & 0xFF
    end = (vtx_w0 >> 1) & 0x7F
    if count != 4 or end != 4:
        raise RuntimeError(f"Charge Shot G_VTX changed: {vtx_w0:#010x}")
    if raw[TRI_WORD] != (0x06060402, 0x00000602):
        raise RuntimeError(f"Charge Shot triangle word changed: {raw[TRI_WORD]!r}")

    verts = tuple(sm.decode_vertex(f, VTX + i * 16) for i in range(4))
    if verts != EXPECTED_VERTS:
        raise RuntimeError(f"Charge Shot vertices changed: {verts!r}")
    tris = tuple(sm.decode_triangles(0x06, *raw[TRI_WORD]))
    if tris != EXPECTED_TRIS:
        raise RuntimeError(f"Charge Shot triangles changed: {tris!r}")
    return raw, verts, tris


def render_header(raw, verts, tris) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Samus Charge Shot native weapon constants (generated).")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_samus_chargeshot.py. */")
    a("#ifndef NDS_NATIVE_SAMUS_CHARGESHOT_GENERATED_H")
    a("#define NDS_NATIVE_SAMUS_CHARGESHOT_GENERATED_H")
    a("")
    a("#define NDS_NATIVE_CHARGESHOT_ASSET 321u")
    a(f"#define NDS_NATIVE_CHARGESHOT_ROOT 0x{ROOT:04x}u")
    a(f"#define NDS_NATIVE_CHARGESHOT_DL_BYTES {DL_WORDS * 8}u")
    a(f"#define NDS_NATIVE_CHARGESHOT_PALETTE_OFFSET 0x{PALETTE:04x}u")
    a(f"#define NDS_NATIVE_CHARGESHOT_IMAGE_OFFSET 0x{IMAGE:04x}u")
    a(f"#define NDS_NATIVE_CHARGESHOT_VERTEX_COUNT {len(verts)}u")
    a(f"#define NDS_NATIVE_CHARGESHOT_TRIANGLE_COUNT {len(tris)}u")
    a(f"#define NDS_NATIVE_CHARGESHOT_CORNER_COUNT {len(tris) * 3}u")
    a("")
    a("#endif")
    return "\n".join(lines) + "\n"


def render(raw, verts, tris) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Samus Charge Shot native weapon packet (generated).")
    a(" * Source: SHA-pinned file 321 root 0x270, 28 Gfx words through the ENDDL")
    a(" * at 0x348, drawing one textured quad from the vertex pool at 0x230.")
    a(" * Palette, image and vertices are all internal to file 321 and there is")
    a(" * no MObj, so the whole program is fixed and only the DObj transform is")
    a(" * live.  Kirby's copy reaches the same root and needs no special case.")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_samus_chargeshot.py. */")
    a("#include <nds/generated/nds_native_samus_chargeshot.generated.h>")
    a("")
    a(f"static const u16 sNdsNativeChargeShotTriIndices[{len(tris) * 3}] =")
    a("{")
    for tri in tris:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    a(f"static const s16 sNdsNativeChargeShotVerts[{len(verts) * 5}] =")
    a("{")
    for v in verts:
        a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeChargeShotVertColors[{len(verts)}] =")
    a("{")
    for v in verts:
        a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a("/* Words 0/6/11/13/17/19/23 are pipe/tile/load syncs and word 27 is ENDDL:")
    a(" * they carry no state.  Every state word is emitted in source order, so")
    a(" * the recorded stats are the source's own. */")
    a("static void ndsNativeChargeShotSetup(")
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
    a("static void ndsNativeChargeShotFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[24][0]:08x}u) |"
      f" 0x{raw[24][1]:08x}u;")
    for i in (25, 26):
        a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[i][0] >> 24:02x}u,"
          f" 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    a("}")
    a("")
    a(f"/* census: dl_words={DL_WORDS} verts={len(verts)} tris={len(tris)}"
      f" material=none palette=321:0x{PALETTE:04x} image=321:0x{IMAGE:04x} */")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    raw, verts, tris = decode()
    text = render(raw, verts, tris)
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
                raise RuntimeError(
                    f"generated artefact stale: {path.relative_to(REPO)}")
        print("SAMUS_CHARGESHOT_NATIVE_OK root=0x270 verts=4 tris=2 "
              "material=none palette=321:0x0008 image=321:0x0030")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
