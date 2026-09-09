#!/usr/bin/env python3
"""Generate/check Mushroom Kingdom POW block's exact native program.

File 155 root 0x10D0 is a Gfx[37] with no MObj and no segment-E call:
dGRInishieMap_PowerBlock_ItemAttributes carries p_mobjsubs NULL, so the list
owns its whole immutable material.  It loads one 16-colour RGBA5551 TLUT from
file 107 offset 0x35F8 (external fixup; file 155's four extern ids are all
107), draws a 16-vertex / 8-triangle pass with the in-file CI4 16x8 image
0x0F50, re-tiles and draws a 4-vertex / 2-triangle pass with the in-file CI4
32x16 image 0x0E48, then restores geometry/othermode and ENDDLs at word 36.
Whole-image pointer census: the ONLY pointer to root 0x10D0 is file 155's own
internal fixup at slot 0x1228 (the DObjDesc_0x11F8 child); GRInishieMap's five
externals into file 155 target 0x5F0/0x11F8/0x13B0/0xC30/0xA68 and never
0x10D0, so exactly one object -- nITKindPowerBlock, spawned by
grInishieMakePowerBlock -- can reach this admission.

Vertex/triangle VALUES are decoded from the pinned payload on every run; the
literal-tuple pins of the laser/pakkun generators are replaced here by count
and layout pins (the two pools tile 0x0F90..0x10D0 exactly, 16+4), which the
descriptor's payload SHA256 already covers upstream.
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

OUT = REPO / "src/nds/generated/nds_native_inishie_powblock.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_inishie_powblock.generated.h"
TYPED155 = REPO / "decomp/BattleShip-main/decomp/src/relocData/155_StageInishieFile3.c"
TYPED260 = REPO / "decomp/BattleShip-main/decomp/src/relocData/260_GRInishieMap.c"
GROUND = REPO / "decomp/BattleShip-main/decomp/src/gr/grcommon/grinishie.c"
ITEMSRC = REPO / "decomp/BattleShip-main/decomp/src/it/itground/itpowerblock.c"


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


ROOT = 0x10D0
VTX_A = 0x0F90
VTX_A_COUNT = 16
VTX_B = 0x1090
VTX_B_COUNT = 4
PAL_ASSET = 107
TLUT_OFFSET = 0x35F8
TLUT_BYTES = 32
# Pass A (word 14) binds the CI4 16x8 at 0x0F50; pass B (word 26) binds the
# CI4 32x16 at 0x0E48.  MEASURED from the relocated SETTIMG words -- the
# order is the reverse of what the decode note first recorded.
IMAGE_A_OFFSET = 0x0F50
IMAGE_A_BYTES = 64
IMAGE_B_OFFSET = 0x0E48
IMAGE_B_BYTES = 264
TRI_A_WORDS = (19, 20, 21, 22)
TRI_B_WORDS = (31,)
DL_WORDS = 37

EXPECTED_OPS = (
    0xE7, 0xD9, 0xE3, 0xFC, 0xE8, 0xF5, 0xF5, 0xF5,
    0xFD, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2, 0xFD, 0xE6,
    0xF3, 0xE7, 0x01, 0x06, 0x06, 0x06, 0x06, 0xE7,
    0xF5, 0xF5, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06,
    0xE7, 0xE7, 0xD9, 0xE3, 0xDF,
)
# The whole-image census of pointers to file 155 root 0x10D0: one internal
# slot, and GRInishieMap (the only file with externals into 155) never
# targets it.
INTERNAL_ROOT_SLOT = 0x1228


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def pin_text(path: Path, tokens):
    text = path.read_text()
    for token in tokens:
        if token not in text:
            raise RuntimeError(f"{path.name} pin missing {token!r}")


def decode():
    actor = load("stage_actors")
    geometry = load("stage_geometry")
    stage_map = load("stage_map")

    pin_text(TYPED155, (
        "Gfx dStageInishieFile3_DL_0x10D0[37]",
        "Vtx dStageInishieFile3_Vtx_0x0F90[16]",
        "Vtx dStageInishieFile3_Vtx_0x1090[4]",
        "u8 dStageInishieFile3_Tex_0x0E48[264]",
        "u8 dStageInishieFile3_Tex_0x0F50[64]",
        "{ 1, (void *)dStageInishieFile3_DL_0x10D0",
    ))
    pin_text(TYPED260, (
        "(void *)dStageInishieFile3_DObjDesc_0x11F8,  /* data */",
        "(void *)dStageInishieFile3_mobjlink_0x13B0,  /* anim_joints */",
    ))
    pin_text(GROUND, (
        "itManagerMakeItemSetupCommon(NULL, nITKindPowerBlock",
        "pblock_appear_wait = 1800;",
    ))
    pin_text(ITEMSRC, (
        "nITKindPowerBlock,                      // Item Kind",
        "grInishiePowerBlockSetWait();",
    ))

    if len(actor.payload) < ROOT + DL_WORDS * 8:
        raise RuntimeError("file 155 no longer contains the POW list")
    raw = words_at(actor.payload, ROOT, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != EXPECTED_OPS:
        raise RuntimeError(f"POW 0x10D0 opcode census changed: {ops!r}")

    tlut_ref = actor.pointer_at(ROOT + 8 * 8 + 4)
    image_a_ref = actor.pointer_at(ROOT + 14 * 8 + 4)
    image_b_ref = actor.pointer_at(ROOT + 26 * 8 + 4)
    vtx_a_ref = actor.pointer_at(ROOT + 18 * 8 + 4)
    vtx_b_ref = actor.pointer_at(ROOT + 30 * 8 + 4)
    if tlut_ref is None or (tlut_ref.asset_id, tlut_ref.offset) != (
            PAL_ASSET, TLUT_OFFSET):
        raise RuntimeError(f"POW TLUT ref changed: {tlut_ref!r}")
    for ref, off in ((image_a_ref, IMAGE_A_OFFSET), (image_b_ref, IMAGE_B_OFFSET),
                     (vtx_a_ref, VTX_A), (vtx_b_ref, VTX_B)):
        if ref is None or (ref.asset_id, ref.offset) != (155, off):
            raise RuntimeError(f"POW internal ref changed: {ref!r}")
    if len(actor.external) != 4 or any(
            ref.asset_id != PAL_ASSET for ref in actor.external.values()):
        raise RuntimeError("file 155 externals are no longer the four 107 TLUTs")
    if len(geometry.payload) < TLUT_OFFSET + TLUT_BYTES:
        raise RuntimeError("file 107 no longer contains the 16-entry POW palette")

    # Root identity: exactly one internal fixup, no external pointer anywhere.
    internal_roots = {slot: ref for slot, ref in actor.internal.items()
                      if ref.offset == ROOT}
    if tuple(internal_roots) != (INTERNAL_ROOT_SLOT,):
        raise RuntimeError(f"root 0x10D0 internal census changed: {internal_roots!r}")
    if any(ref.offset == ROOT for ref in stage_map.external.values()):
        raise RuntimeError("GRInishieMap gained an external fixup to root 0x10D0")

    verts = tuple(sm.decode_vertex(actor, VTX_A + i * 16) for i in range(VTX_A_COUNT))
    verts += tuple(sm.decode_vertex(actor, VTX_B + i * 16) for i in range(VTX_B_COUNT))
    if VTX_A + VTX_A_COUNT * 16 != VTX_B or VTX_B + VTX_B_COUNT * 16 != ROOT:
        raise RuntimeError("POW vertex pools no longer tile 0x0F90..0x10D0")
    tris_a: list[tuple[int, int, int]] = []
    for i in TRI_A_WORDS:
        tris_a.extend(sm.decode_triangles(0x06, *raw[i]))
    tris_b: list[tuple[int, int, int]] = []
    for i in TRI_B_WORDS:
        tris_b.extend(sm.decode_triangles(0x06, *raw[i]))
    if len(tris_a) != 8 or len(tris_b) != 2:
        raise RuntimeError(f"POW triangle census changed: {len(tris_a)}/{len(tris_b)}")
    return raw, verts, tuple(tris_a), tuple(tris_b)


def render_header(raw, verts, tris_a, tris_b) -> str:
    """Constants only. The adapter admits the object in a DIFFERENT
    translation unit from the executor, so the numbers live in a header
    both can include -- the laser-owner shape."""
    lines: list[str] = []
    a = lines.append
    a("/* Mushroom Kingdom POW block native item constants (generated).")
    a(" * Do not hand-edit; regenerate with generate_nds_native_inishie_powblock.py. */")
    a("#ifndef NDS_NATIVE_INISHIE_POWBLOCK_GENERATED_H")
    a("#define NDS_NATIVE_INISHIE_POWBLOCK_GENERATED_H")
    a("")
    a("#define NDS_NATIVE_INISHIE_POWBLOCK_ASSET 155u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_ROOT 0x{ROOT:04x}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_DL_BYTES {DL_WORDS * 8}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_PAL_ASSET {PAL_ASSET}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_TLUT_OFFSET 0x{TLUT_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_TLUT_END 0x{TLUT_OFFSET + TLUT_BYTES:04x}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_OFFSET 0x{IMAGE_A_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_END 0x{IMAGE_A_OFFSET + IMAGE_A_BYTES:04x}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_OFFSET 0x{IMAGE_B_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_END 0x{IMAGE_B_OFFSET + IMAGE_B_BYTES:04x}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_TLUT_W0 0x{raw[8][0]:08x}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_W0 0x{raw[14][0]:08x}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_W0 0x{raw[26][0]:08x}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_TLUT_SLOT {8 * 8 + 4}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_SLOT {14 * 8 + 4}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_SLOT {26 * 8 + 4}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_VERTEX_COUNT {len(verts)}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_TRIANGLE_COUNT {len(tris_a) + len(tris_b)}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_CORNER_COUNT {(len(tris_a) + len(tris_b)) * 3}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_PASS_A_TRIANGLE_COUNT {len(tris_a)}u")
    a(f"#define NDS_NATIVE_INISHIE_POWBLOCK_PASS_B_TRIANGLE_COUNT {len(tris_b)}u")
    a("")
    a("#endif")
    return "\n".join(lines) + "\n"


def render(raw, verts, tris_a, tris_b) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Mushroom Kingdom POW block native item packet (generated).")
    a(" * Source: SHA-pinned file 155 root 0x10d0 (Gfx[37]) + vertex pools")
    a(" * 0x0f90[16]/0x1090[4].  No MObj: the list owns its whole material,")
    a(" * with the 16-colour TLUT relocated into file 107 offset 0x35f8 and")
    a(" * both CI4 images internal to file 155 (0x0e48 32x16, 0x0f50 16x8).")
    a(" * Do not hand-edit; regenerate with generate_nds_native_inishie_powblock.py. */")
    a("#include <nds/generated/nds_native_inishie_powblock.generated.h>")
    a("")
    a(f"static const u16 sNdsNativeInishiePowblockTriA[{len(tris_a) * 3}] =")
    a("{")
    for tri in tris_a:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    a(f"static const u16 sNdsNativeInishiePowblockTriB[{len(tris_b) * 3}] =")
    a("{")
    for tri in tris_b:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    a(f"static const s16 sNdsNativeInishiePowblockVerts[{len(verts) * 5}] =")
    a("{")
    for v in verts:
        a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeInishiePowblockVertColors[{len(verts)}] =")
    a("{")
    for v in verts:
        a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a("/* Source sync words (E6/E7/E8) and ENDDL carry no state.  Every state")
    a(" * word is emitted in source order, so the recorded stats are the")
    a(" * source's own.  SetupA is words 0-17 (the laser head shape), SetupB")
    a(" * re-tiles for the second image (words 24-28), Finish restores")
    a(" * geometry/othermode (words 32-33). */")
    a("static void ndsNativeInishiePowblockSetupA(")
    a("    NDSRendererStats *stats, const void *tlut, const void *image_a)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) |"
      f" 0x{raw[1][1]:08x}u;")
    a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[0][0] >> 24:02x}u,"
      f" 0x{raw[0][0]:08x}u, 0x{raw[0][1]:08x}u);")
    a(f"    ndsRendererRecordSetCombine(stats, 0x{raw[3][0]:08x}u,"
      f" 0x{raw[3][1]:08x}u);")
    for i in (5, 6, 7):
        a(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u,"
          f" 0x{raw[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[8][0]:08x}u,"
      " (u32)(uintptr_t)tlut);")
    a(f"    ndsRendererRecordLoadTlut(stats, 0x{raw[10][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{raw[12][0]:08x}u,"
      f" 0x{raw[12][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{raw[13][0]:08x}u,"
      f" 0x{raw[13][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[14][0]:08x}u,"
      " (u32)(uintptr_t)image_a);")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{raw[16][0]:08x}u,"
      f" 0x{raw[16][1]:08x}u);")
    a("}")
    a("")
    a("static void ndsNativeInishiePowblockSetupB(")
    a("    NDSRendererStats *stats, const void *image_b)")
    a("{")
    for i in (24, 25):
        a(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u,"
          f" 0x{raw[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[26][0]:08x}u,"
      " (u32)(uintptr_t)image_b);")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{raw[28][0]:08x}u,"
      f" 0x{raw[28][1]:08x}u);")
    a("}")
    a("")
    a("static void ndsNativeInishiePowblockFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[34][0]:08x}u) |"
      f" 0x{raw[34][1]:08x}u;")
    a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[35][0] >> 24:02x}u,"
      f" 0x{raw[35][0]:08x}u, 0x{raw[35][1]:08x}u);")
    a("}")
    a("")
    a(f"/* census: dl_words={DL_WORDS} verts={len(verts)} tris={len(tris_a) + len(tris_b)}"
      f" material=none tlut={PAL_ASSET}:0x{TLUT_OFFSET:04x}"
      f" img_a=155:0x{IMAGE_A_OFFSET:04x} img_b=155:0x{IMAGE_B_OFFSET:04x} */")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    raw, verts, tris_a, tris_b = decode()
    text = render(raw, verts, tris_a, tris_b)
    header = render_header(raw, verts, tris_a, tris_b)
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
        print("INISHIE_POWBLOCK_NATIVE_OK root=0x10d0 verts=20 tris=10 "
              "material=none tlut=107:0x35f8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
