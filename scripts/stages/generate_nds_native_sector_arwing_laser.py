#!/usr/bin/env python3
"""Generate/check Sector Z's exact native Arwing-laser weapon program.

File 153 root 0x1C50 is a Gfx[27] with no MObj: it carries its own immutable
texture binding, whose two SETTIMG pointers are file 153's ONLY external
fixups (file 161 FoxSpecial3 0x10C8 TLUT, 0x10F0 CI4 image).  The list draws
one closed 6-vertex / 8-triangle spindle with lighting and smooth shading
cleared, then restores both.  Nothing else in the game data points at root
0x1C50: the only two pointers to it are GRSectorMap external fixups 0x00BC
and 0x00F0, the ArwingLaser2D/3D WPAttributes.data fields, so one owner
serves both weapon kinds and no other object can reach this admission.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402

OUT = REPO / "src/nds/generated/nds_native_sector_arwing_laser.generated.inc"
TYPED153 = REPO / "decomp/BattleShip-main/decomp/src/relocData/153_StageSectorFile3.c"
TYPED262 = REPO / "decomp/BattleShip-main/decomp/src/relocData/262_GRSectorMap.c"
GROUND = REPO / "decomp/BattleShip-main/decomp/src/gr/grcommon/grsector.c"
WPMANAGER = REPO / "decomp/BattleShip-main/decomp/src/wp/wpmanager.c"

LASER_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank153",
    "23425ab123f59c992ee8ae6e35020539212b4608701b34eadea2ad9e43ff3899",
    153, 94, 2,
    "a5ee542bc72f6c601630dad6abd003d374fef1237bd7abaddc31112f6251999f")
TEXTURE_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/FoxSpecial3",
    "b928c17ea613e047bfa2e9a554456322153e410a80ecb7ec811b5ff96fb651e4",
    161, 85, 3,
    "10cbc64c2969c22d31d0164178e5af473facbd7cbaf6d864d23af168c62d5c98")
MAP_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRSectorMap",
    "63c75e3f482a5f614820e1d9790f00a7bb4563237cadb5add2ba724c6919bad3",
    262, 1, 9,
    "30e70b204089cbaf8f9135ebb6629a4892ce60e196b79db824d380d4a24ab07b")

TEXT_PINS = (
    (TYPED153, "04b53340925cd03c85f5b8aadee3ee5fb0ad02fad9353d3e1166a126361a9f17",
     ("Gfx dStageSectorFile3_AnimJoint_0x1C50[27]",
      "Vtx dStageSectorFile3_Sub_0x1BF0[6]")),
    (TYPED262, "d8ff322feb391d1912cecc5231a3819cf02e2427d46c6434c6b82343cf6d61e7",
     ("WPAttributes dGRSectorMap_ArwingLaser2D_WeaponAttributes",
      "WPAttributes dGRSectorMap_ArwingLaser3D_WeaponAttributes")),
    (GROUND, "4f5e8b827408fd3c0e45dbf7b0e7e3e51f969d7373cfb9fa3b7bd383a66290a3",
     ("nWPKindArwingLaser2D,                       // Weapon Kind",
      "nWPKindArwingLaser3D,                       // Weapon Kind",
      "gGRCommonStruct.sector.arwing_appear_timer = 600;")),
    (WPMANAGER, "e3f026c7c056155a4db75013b1fcb8ebf52194a4a8cd9fa8ffc46aa536faeb99",
     ("lbCommonInitDObj3Transforms(gcAddDObjForGObj(weapon_gobj, attr->data)",
      "proc_display = (wp_desc->flags & WEAPON_FLAG_DOBJLINKS) ? "
      "wpDisplayDObjDLLinks : wpDisplayDLHead1;")),
)

ROOT = 0x1C50
VTX = 0x1BF0
DL_WORDS = 27
TEX_ASSET = 161
TLUT_OFFSET = 0x10C8
TLUT_BYTES = 32
IMAGE_OFFSET = 0x10F0
IMAGE_BYTES = 128

EXPECTED_OPS = (
    0xE7, 0xD9, 0xE3, 0xFC, 0xE8, 0xF5, 0xF5, 0xF5,
    0xFD, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2, 0xFD, 0xE6,
    0xF3, 0xE7, 0x01, 0x06, 0x06, 0x06, 0x06, 0xE7,
    0xD9, 0xE3, 0xDF,
)
EXPECTED_VERTS = (
    (0, 0, 569, 509, 510, 0xFFFFFFFF),
    (56, 0, 511, 509, 0, 0xFFFFFFFF),
    (0, 35, 383, 480, 476, 0xFFFFFFFF),
    (0, -36, 383, 480, 476, 0xFFFFFFFF),
    (0, 0, -564, 0, 0, 0xFFFFFFFF),
    (-56, 0, 511, 0, 511, 0xFFFFFFFF),
)
EXPECTED_TRIS = (
    (5, 4, 3), (4, 5, 2), (4, 1, 3), (0, 5, 3),
    (5, 0, 2), (1, 0, 3), (0, 1, 2), (1, 4, 2),
)
# The whole-game census of pointers to file 153 root 0x1C50.
EXPECTED_MAP_REFS = ((0x00BC, 153, 0x1C50), (0x00F0, 153, 0x1C50))


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def decode():
    laser = sm.load_o2r(REPO, LASER_FILE)
    texture = sm.load_o2r(REPO, TEXTURE_FILE)
    stage_map = sm.load_o2r(REPO, MAP_FILE)

    for path, sha, tokens in TEXT_PINS:
        data = sm.checked_bytes(REPO, sm.InputSpec(
            str(path.relative_to(REPO)).replace("\\", "/"), sha))
        text = data.decode("utf-8", "replace")
        for token in tokens:
            if token not in text:
                raise RuntimeError(f"{path.name} pin missing {token!r}")

    if len(laser.payload) < ROOT + DL_WORDS * 8:
        raise RuntimeError("file 153 no longer contains the laser list")
    raw = words_at(laser.payload, ROOT, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != EXPECTED_OPS:
        raise RuntimeError(f"laser 0x1C50 opcode census changed: {ops!r}")

    # Root identity: only the two ArwingLaser WPAttributes.data fields.
    refs = tuple(sorted(
        (slot, ref.asset_id, ref.offset)
        for slot, ref in stage_map.external.items() if ref.offset == ROOT))
    if refs != EXPECTED_MAP_REFS:
        raise RuntimeError(f"root 0x1C50 reference census changed: {refs!r}")
    if any(ref.offset == ROOT for ref in laser.internal.values()):
        raise RuntimeError("file 153 gained an internal pointer to root 0x1C50")

    vtx_ref = laser.pointer_at(ROOT + 18 * 8 + 4)
    if vtx_ref is None or (vtx_ref.asset_id, vtx_ref.offset) != (153, VTX):
        raise RuntimeError(f"laser vertex ref changed: {vtx_ref!r}")
    tlut_ref = laser.pointer_at(ROOT + 8 * 8 + 4)
    image_ref = laser.pointer_at(ROOT + 14 * 8 + 4)
    if tlut_ref is None or (tlut_ref.asset_id, tlut_ref.offset) != (
            TEX_ASSET, TLUT_OFFSET):
        raise RuntimeError(f"laser TLUT ref changed: {tlut_ref!r}")
    if image_ref is None or (image_ref.asset_id, image_ref.offset) != (
            TEX_ASSET, IMAGE_OFFSET):
        raise RuntimeError(f"laser image ref changed: {image_ref!r}")
    if len(laser.external) != 2:
        raise RuntimeError("file 153 external fixups are no longer laser-only")
    if len(texture.payload) < IMAGE_OFFSET + IMAGE_BYTES:
        raise RuntimeError("file 161 no longer contains the laser texture")
    for i in range(16):
        entry = struct.unpack_from(">H", texture.payload, TLUT_OFFSET + i * 2)[0]
        if (entry & 1) == 0:
            raise RuntimeError(f"laser TLUT entry {i} is no longer opaque")

    verts = tuple(sm.decode_vertex(laser, VTX + i * 16) for i in range(6))
    if verts != EXPECTED_VERTS:
        raise RuntimeError(f"laser vertices changed: {verts!r}")
    tris: list[tuple[int, int, int]] = []
    for i in (19, 20, 21, 22):
        tris.extend(sm.decode_triangles(0x06, *raw[i]))
    if tuple(tris) != EXPECTED_TRIS:
        raise RuntimeError(f"laser triangles changed: {tris!r}")
    return raw, verts, tuple(tris)


def render(raw, verts, tris) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Sector Z Arwing laser native weapon packet (generated).")
    a(" * Source: SHA-pinned file 153 root 0x1c50 (Gfx[27]) + vertex pool 0x1bf0.")
    a(" * The list owns its whole material: no MObj, and its two SETTIMG words are")
    a(" * file 153's only external fixups, bound by the reloc loader to file 161")
    a(" * (FoxSpecial3) 0x10c8 TLUT / 0x10f0 CI4 image.  Both ArwingLaser weapon")
    a(" * kinds share this one list and differ in no drawn respect.")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_sector_arwing_laser.py. */")
    a("#define NDS_NATIVE_SECTOR_LASER_ASSET 153u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_ROOT 0x{ROOT:04x}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_DL_BYTES {DL_WORDS * 8}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_TEX_ASSET {TEX_ASSET}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_TLUT_OFFSET 0x{TLUT_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_IMAGE_OFFSET 0x{IMAGE_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_TEX_END 0x{IMAGE_OFFSET + IMAGE_BYTES:04x}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_TLUT_W0 0x{raw[8][0]:08x}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_IMAGE_W0 0x{raw[14][0]:08x}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_TLUT_SLOT {8 * 8 + 4}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_IMAGE_SLOT {14 * 8 + 4}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_VERTEX_COUNT {len(verts)}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_TRIANGLE_COUNT {len(tris)}u")
    a(f"#define NDS_NATIVE_SECTOR_LASER_CORNER_COUNT {len(tris) * 3}u")
    a("")
    a(f"static const u16 sNdsNativeSectorLaserTriIndices[{len(tris) * 3}] =")
    a("{")
    for tri in tris:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    a(f"static const s16 sNdsNativeSectorLaserVerts[{len(verts) * 5}] =")
    a("{")
    for v in verts:
        a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeSectorLaserVertColors[{len(verts)}] =")
    a("{")
    for v in verts:
        a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a("/* Source words 0/4/9/11/15/17/23 are pipe/tile/load syncs and word 26 is")
    a(" * ENDDL: they carry no state.  Every state word is emitted in source")
    a(" * order, so the recorded stats are the source's own. */")
    a("static void ndsNativeSectorLaserSetup(")
    a("    NDSRendererStats *stats, const void *tlut, const void *image)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) |"
      f" 0x{raw[1][1]:08x}u;")
    a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[2][0] >> 24:02x}u,"
      f" 0x{raw[2][0]:08x}u, 0x{raw[2][1]:08x}u);")
    a(f"    ndsRendererRecordSetCombine(stats, 0x{raw[3][0]:08x}u,"
      f" 0x{raw[3][1]:08x}u);")
    for i in (5, 6, 7):
        a(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u,"
          f" 0x{raw[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[8][0]:08x}u,"
      "\n        (u32)(uintptr_t)tlut);")
    a(f"    ndsRendererRecordLoadTlut(stats, 0x{raw[10][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{raw[12][0]:08x}u,"
      f" 0x{raw[12][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{raw[13][0]:08x}u,"
      f" 0x{raw[13][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[14][0]:08x}u,"
      "\n        (u32)(uintptr_t)image);")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{raw[16][0]:08x}u,"
      f" 0x{raw[16][1]:08x}u);")
    a("}")
    a("")
    a("static void ndsNativeSectorLaserFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[24][0]:08x}u) |"
      f" 0x{raw[24][1]:08x}u;")
    a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[25][0] >> 24:02x}u,"
      f" 0x{raw[25][0]:08x}u, 0x{raw[25][1]:08x}u);")
    a("}")
    a("")
    a(f"/* census: dl_words={DL_WORDS} verts={len(verts)} tris={len(tris)}"
      " material=none tex=161:0x10f0 tlut=161:0x10c8 */")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    raw, verts, tris = decode()
    text = render(raw, verts, tris)
    if args.emit:
        OUT.parent.mkdir(parents=True, exist_ok=True)
        if (not OUT.exists()) or OUT.read_text() != text:
            OUT.write_text(text)
        print(f"emitted {OUT.relative_to(REPO)}")
    if args.check or not args.emit:
        if not OUT.exists() or OUT.read_text() != text:
            raise RuntimeError(f"generated packet stale: {OUT.relative_to(REPO)}")
        print("SECTOR_ARWING_LASER_NATIVE_OK root=0x1c50 verts=6 tris=8 "
              "material=none tex=161")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
