#!/usr/bin/env python3
"""Generate/check Pikachu's air Thunder Jolt: its exact native weapon program.

WHAT THIS OBJECT IS.  `llPikachuSpecial1ThunderJoltAirWeaponAttributes` is 0x0
(reloc_data.us.h:3725), so the WPAttributes at PikachuSpecial1 (file 244) + 0
IS the air Thunder Jolt.  Its `data` field is file 244's external fixup at slot
0x0000, and that fixup resolves straight to file 342 (PikachuSpecial3) 0x0270 --
the display list itself, not a DObjDesc.  That is the same shape Samus's Charge
Shot has: a WPDesc whose flags are 0x00 and whose attributes point directly at
one Gfx list.

Note the two files.  The attributes live in PikachuSpecial1 and the geometry in
PikachuSpecial3, which is why a search of Special3's own relocations finds NO
pointer to the root: the only referrer in the whole game image is external, and
it comes from Special1.

ONE LIST, NO MATERIAL.  `p_mobjsubs` at 244+0x04 is NULL and so is
`p_matanim_joints` at +0x0C, so no DObj here carries an MObj -- "material 0" in
the failure record is that NULL and NOT "untextured".  The list carries its own
complete binding out of file 342's own internal fixups: a 16-entry RGBA5551
TLUT at 0x0008 and a CI4 image at 0x0030 whose 512 loaded bytes end exactly
where the vertex pool at 0x0230 begins.  File 342 has zero external fixups.

ONE REFERRER, GAME-WIDE.  The `--census` sweep walks every O2R file; the
default check pins what it reduces to.  Exactly one pointer in the image
reaches 342:0x0270 -- file 244 slot 0x0000 -- and file 244's six externals into
file 342 are the complete set.  No `ll` constant names 0x0270 either: the
`llPikachuSpecial3*` block (reloc_data.us.h:4178-4183) is all Thunder Jolt B
and its MObjSub/AnimJoint tables, none of them this root.  So asset and root
discriminate completely and no live-kind test is required.

TWENTY-NINE WORDS, NOT THIRTY.  This is the Link Bomb body template with one
fewer restore: words 0..27 match it opcode for opcode and then it ends, where
the bomb writes a fourth othermode word first.  Do not copy the bomb's word
indices; they are pinned here from this file's own payload.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402

OUT = REPO / "src/nds/generated/nds_native_pikachu_thunderjolt.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_pikachu_thunderjolt.generated.h"

MODEL_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/PikachuSpecial3",
    "b8a9eb805db18a30c2366fcc97ba031e0bfc668bc5250cf9ea5326e354e51a4b",
    342, 104, 0,
    "e09ecb102e8e963c375d32584e461868db536371be7a85ed1e14ab0ee8b99faa")
ATTR_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/PikachuSpecial1",
    "8ebd04da94a0e76960f856921f9202a0c74a1ca266a9e8748cd155263ebf1c41",
    244, 0, 6,
    "698a215ec264eb4585272ca1c876fc786ce5f709d9d25e5a5fa3b6aac0f0ad25")

TEXT_PINS = (
    ("decomp/BattleShip-main/include/reloc_data.us.h",
     ("#define llPikachuSpecial1ThunderJoltAirWeaponAttributes ((intptr_t)0x0)",
      "#define llPikachuSpecial3FileID ((intptr_t)0x156)")),
)

ASSET = 342
ATTR_ASSET = 244
ATTR_OFFSET = 0x0000          # llPikachuSpecial1ThunderJoltAirWeaponAttributes
ANIMJOINT = 0x0360            # WPAttributes.anim_joints

ROOT = 0x0270
DL_WORDS = 29
VTX = 0x0230
TLUT_OFFSET = 0x0008
TLUT_ENTRIES = 16
IMAGE_OFFSET = 0x0030
IMAGE_BYTES = 512
# The image's 512 loaded bytes end exactly where the vertex pool begins, so the
# executor never dereferences past here; the vertices are baked.
FILE_END = IMAGE_OFFSET + IMAGE_BYTES

TLUT_WORD = 11
IMAGE_WORD = 17
VTX_WORD = 21
TRI_WORD = 22

OPS = (
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8,
    0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
    0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xE2, 0xDF,
)
VERTS = (
    (72, 71, 0, 2048, 2048, 0xFFFFFFFF),
    (72, -71, 0, 2048, 0, 0xFFFFFFFF),
    (-70, -71, 0, 0, 0, 0xFFFFFFFF),
    (-70, 71, 0, 0, 2048, 0xFFFFFFFF),
)
TRIS = ((3, 2, 1), (0, 3, 1))
# Pinned exactly rather than by a rule: entry 0 AND entry 15 both have the
# RGBA5551 alpha bit clear here, unlike the Link Bomb where only entry 0 does,
# so "every entry but the first is opaque" would be a false assertion.
TLUT = (0x003E, 0x003F, 0x18FD, 0x31BD, 0x423D, 0x4A7D, 0x633D, 0x7BFD,
        0x8C7D, 0x9CFD, 0xAD7D, 0xC63D, 0xD6BD, 0xF7BD, 0xFFFF, 0xEF7E)

EXPECTED_ATTR_REFS = (
    (0x0000, ASSET, ROOT), (0x0008, ASSET, ANIMJOINT),
    (0x0034, ASSET, 0x1888), (0x0038, ASSET, 0x1018),
    (0x003C, ASSET, 0x1A20), (0x0040, ASSET, 0x1AE0),
)


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def check_text_pins() -> None:
    for path, tokens in TEXT_PINS:
        text = (REPO / path).read_text(encoding="utf-8", errors="replace")
        for token in tokens:
            if token not in text:
                raise RuntimeError(f"{path} pin missing {token!r}")


def census() -> tuple:
    """Every O2R file in the image, for pointers into asset 342."""
    root = REPO / "decomp/BattleShip-main/BattleShip_o2r"
    hits = []
    scanned = 0
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        blob = path.read_bytes()
        if len(blob) < 0x50 or blob[4:8] != b"OLER":
            continue
        scanned += 1
        rel = str(path.relative_to(REPO)).replace("\\", "/")
        res = sm.load_o2r(REPO, sm.InputSpec(rel, hashlib.sha256(blob).hexdigest()))
        for slot, ref in res.external.items():
            if ref.asset_id == ASSET:
                hits.append((res.file_id, slot, ref.offset))
    return scanned, tuple(sorted(hits))


def decode(run_census: bool = True):
    model = sm.load_o2r(REPO, MODEL_FILE)
    attr = sm.load_o2r(REPO, ATTR_FILE)
    check_text_pins()

    if len(model.payload) < ROOT + DL_WORDS * 8:
        raise RuntimeError("file 342 no longer contains the Thunder Jolt list")
    raw = words_at(model.payload, ROOT, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != OPS:
        raise RuntimeError(f"Thunder Jolt 0x0270 opcode census changed: {ops!r}")

    # WPAttributes at PikachuSpecial1 + 0: data IS the display list, and both
    # material pointers are NULL, which is why the DObj has mobj == NULL.
    data_ref = attr.pointer_at(ATTR_OFFSET)
    if data_ref is None or (data_ref.asset_id, data_ref.offset) != (ASSET, ROOT):
        raise RuntimeError(f"ThunderJolt WPAttributes.data changed: {data_ref!r}")
    anim_ref = attr.pointer_at(ATTR_OFFSET + 8)
    if anim_ref is None or (anim_ref.asset_id, anim_ref.offset) != (ASSET, ANIMJOINT):
        raise RuntimeError(f"ThunderJolt anim_joints changed: {anim_ref!r}")
    for name, off in (("p_mobjsubs", 4), ("p_matanim_joints", 12)):
        if attr.pointer_at(ATTR_OFFSET + off) is not None:
            raise RuntimeError(f"ThunderJolt WPAttributes.{name} is no longer NULL")
        if struct.unpack_from(">I", attr.payload, ATTR_OFFSET + off)[0] != 0:
            raise RuntimeError(f"ThunderJolt WPAttributes.{name} is no longer NULL")
    if struct.unpack_from(">I", attr.payload, ATTR_OFFSET + 0x10)[0] != 0:
        raise RuntimeError("ThunderJolt WPDesc flag word is no longer 0")

    # Referrer census.  File 342 holds NO pointer to its own root; the single
    # referrer in the whole image is file 244's external at slot 0.
    internal_roots = tuple(sorted(
        slot for slot, ref in model.internal.items() if ref.offset == ROOT))
    if internal_roots != ():
        raise RuntimeError(f"file 342 gained an internal pointer to the root: {internal_roots!r}")
    attr_refs = tuple(sorted(
        (slot, ref.asset_id, ref.offset)
        for slot, ref in attr.external.items() if ref.asset_id == ASSET))
    if attr_refs != EXPECTED_ATTR_REFS:
        raise RuntimeError(f"file 244 -> 342 census changed: {attr_refs!r}")
    if run_census:
        scanned, hits = census()
        want = tuple((ATTR_ASSET, slot, off) for slot, _, off in EXPECTED_ATTR_REFS)
        if hits != want:
            raise RuntimeError(
                f"whole-image census into asset {ASSET} changed ({scanned} files): {hits!r}")

    # Bindings.  All three are file 342's OWN internal fixups; compare them,
    # never assume the loader's fixup pass ran.
    tlut_ref = model.pointer_at(ROOT + TLUT_WORD * 8 + 4)
    image_ref = model.pointer_at(ROOT + IMAGE_WORD * 8 + 4)
    vtx_ref = model.pointer_at(ROOT + VTX_WORD * 8 + 4)
    for name, ref, want in (("TLUT", tlut_ref, TLUT_OFFSET),
                            ("image", image_ref, IMAGE_OFFSET),
                            ("vertices", vtx_ref, VTX)):
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, want):
            raise RuntimeError(f"ThunderJolt {name} ref changed: {ref!r}")
    if model.external:
        raise RuntimeError("file 342 gained an external fixup")
    if len(model.payload) < FILE_END:
        raise RuntimeError("file 342 no longer contains the Thunder Jolt image")
    if IMAGE_OFFSET + IMAGE_BYTES != VTX:
        raise RuntimeError("the image no longer ends where the vertex pool begins")

    entries = struct.unpack_from(f">{TLUT_ENTRIES}H", model.payload, TLUT_OFFSET)
    if entries != TLUT:
        raise RuntimeError(f"ThunderJolt TLUT changed: {tuple(hex(e) for e in entries)!r}")
    if ((raw[19][1] >> 12) & 0xFFF) + 1 != IMAGE_BYTES * 2 // 4:
        raise RuntimeError(f"ThunderJolt LOADBLOCK changed: {raw[19][1]:#010x}")
    if (((raw[13][1] >> 14) & 0x3FF) + 1) != TLUT_ENTRIES:
        raise RuntimeError(f"ThunderJolt LOADTLUT count changed: {raw[13][1]:#010x}")

    verts = tuple(sm.decode_vertex(model, VTX + i * 16) for i in range(len(VERTS)))
    if verts != VERTS:
        raise RuntimeError(f"ThunderJolt vertices changed: {verts!r}")
    tris = tuple(sm.decode_triangles(0x06, *raw[TRI_WORD]))
    if tris != TRIS:
        raise RuntimeError(f"ThunderJolt triangles changed: {tris!r}")
    return raw, verts


def render_header(raw) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Pikachu air Thunder Jolt native weapon constants (generated).")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_pikachu_thunderjolt.py. */")
    a("#ifndef NDS_NATIVE_PIKACHU_THUNDERJOLT_GENERATED_H")
    a("#define NDS_NATIVE_PIKACHU_THUNDERJOLT_GENERATED_H")
    a("")
    a(f"#define NDS_NATIVE_THUNDERJOLT_ASSET {ASSET}u")
    a(f"#define NDS_NATIVE_THUNDERJOLT_ROOT 0x{ROOT:04x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLT_DL_BYTES {DL_WORDS * 8}u")
    a(f"#define NDS_NATIVE_THUNDERJOLT_TLUT_OFFSET 0x{TLUT_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLT_IMAGE_OFFSET 0x{IMAGE_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLT_FILE_END 0x{FILE_END:04x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLT_TLUT_W0 0x{raw[TLUT_WORD][0]:08x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLT_IMAGE_W0 0x{raw[IMAGE_WORD][0]:08x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLT_VERTEX_OFFSET 0x{VTX:04x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLT_VERTEX_COUNT {len(VERTS)}u")
    a(f"#define NDS_NATIVE_THUNDERJOLT_TRIANGLE_COUNT {len(TRIS)}u")
    a(f"#define NDS_NATIVE_THUNDERJOLT_CORNER_COUNT {len(TRIS) * 3}u")
    a("")
    a("#endif")
    return "\n".join(lines) + "\n"


def _othermode(a, raw, index):
    a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[index][0] >> 24:02x}u,"
      f" 0x{raw[index][0]:08x}u, 0x{raw[index][1]:08x}u);")


def render(raw, verts) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Pikachu air Thunder Jolt native weapon packet (generated).")
    a(" * Source: SHA-pinned file 342 root 0x0270 (Gfx[29]), vertex pool 0x0230,")
    a(" * 16-entry RGBA5551 TLUT 0x0008 and CI4 image 0x0030 -- all three internal")
    a(" * to file 342, which has zero external fixups.  The weapon has NO MObj")
    a(" * (PikachuSpecial1 WPAttributes.p_mobjsubs is NULL), so the list owns its")
    a(" * whole material and nothing here is a segment-E hook.")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_pikachu_thunderjolt.py. */")
    a("#include <nds/generated/nds_native_pikachu_thunderjolt.generated.h>")
    a("")
    a(f"static const u16 sNdsNativeThunderJoltTriIndices[{len(TRIS) * 3}] =")
    a("{")
    for tri in TRIS:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    a(f"static const s16 sNdsNativeThunderJoltVerts[{len(verts) * 5}] =")
    a("{")
    for v in verts:
        a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeThunderJoltVertColors[{len(verts)}] =")
    a("{")
    for v in verts:
        a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a("/* Words 0/7/12/14/18/20/23/24 are pipe/tile/load syncs and word 28 is")
    a(" * ENDDL: they carry no state.  Every state word is emitted in source")
    a(" * order, so the SETTIMG naming the palette still precedes the LOADTLUT")
    a(" * that latches texture_tlut_image, and the image SETTIMG still follows. */")
    a("static void ndsNativeThunderJoltSetup(")
    a("    NDSRendererStats *stats, const void *tlut, const void *image)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) |"
      f" 0x{raw[1][1]:08x}u;")
    for i in (2, 3, 4):
        _othermode(a, raw, i)
    a(f"    ndsRendererRecordSetCombine(stats, 0x{raw[5][0]:08x}u,"
      f" 0x{raw[5][1]:08x}u);")
    a(f"    stats->blend_color = 0x{raw[6][1]:08x}u;")
    for i in (8, 9, 10):
        a(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u,"
          f" 0x{raw[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[11][0]:08x}u,"
      "\n        (u32)(uintptr_t)tlut);")
    a(f"    ndsRendererRecordLoadTlut(stats, 0x{raw[13][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{raw[15][0]:08x}u,"
      f" 0x{raw[15][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{raw[16][0]:08x}u,"
      f" 0x{raw[16][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[17][0]:08x}u,"
      "\n        (u32)(uintptr_t)image);")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{raw[19][0]:08x}u,"
      f" 0x{raw[19][1]:08x}u);")
    a("}")
    a("")
    a("/* Only THREE restore words, not the Link Bomb body's four: this list ends")
    a(" * at word 28 without a second SETOTHERMODE_L. */")
    a("static void ndsNativeThunderJoltFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) |"
      f" 0x{raw[25][1]:08x}u;")
    for i in (26, 27):
        _othermode(a, raw, i)
    a("}")
    a("")
    a(f"/* census: dl_words={DL_WORDS} verts={len(verts)} tris={len(TRIS)}"
      f" material=none tlut={ASSET}:0x{TLUT_OFFSET:04x}"
      f" image={ASSET}:0x{IMAGE_OFFSET:04x} referrer=244:0x0000 */")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--census", action="store_true",
                    help="print the whole-image referrer sweep and exit")
    args = ap.parse_args()
    if args.census:
        scanned, hits = census()
        print(f"scanned {scanned} O2R files")
        for file_id, slot, off in hits:
            print(f"  file {file_id} slot 0x{slot:04x} -> {ASSET}:0x{off:04x}")
        return 0
    raw, verts = decode()
    text = render(raw, verts)
    header = render_header(raw)
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
        print("PIKACHU_THUNDERJOLT_NATIVE_OK root=0x0270 verts=4 tris=2 "
              "material=none tlut=342:0x0008 image=342:0x0030 referrer=244:0x0000")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
