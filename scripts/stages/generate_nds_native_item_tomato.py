#!/usr/bin/env python3
"""Generate/check the Maxim Tomato's native item program.

WHAT THIS OBJECT IS.  `llITCommonDataTomatoItemAttributes` is 0xB8
(reloc_data.us.h) and `dITTomatoItemDesc` names it beside `nITKindTomato`
(ittomato.c:10-15), so the ITAttributes at ITCommonData (file 251) + 0xB8 IS
the Maxim Tomato (item kind 4, `nITKindTomato = nITKindUtilityStart`).  Its
`data` field is file 251's external fixup at slot 0x00B8, which resolves to
file 86 (MiscData086) 0x0AB0 -- a DObjDesc[2] whose entry 1 draws root 0x09C0
and whose entry 2 is the id-18 terminator.

ONE LIST PER DRAWN FRAME.  `is_item_dobjs` is clear, so `itManagerMakeItem`
builds the tree with the common DObj setup (itmanager.c) and then
`lbCommonEjectTreeDObj` DELETES the DL-less root, promoting the single
DL-carrying child to be the GObj's DObj.  `is_display_xlu` and
`is_display_colanim` are both clear, so the display proc is
`itDisplayOPAProcDisplay` (itdisplay.c), which calls `gcDrawDObjTreeForGObj`
once.  One item, one DObj, one display list, DL head 0.  (The flag word also
carries hitlag/weight bits, 0x18000000; those do not affect the draw and only
the top three display bits are pinned.)

MATERIAL 0 IS `p_mobjsubs == NULL` (ITCommonData at 0x00BC is NULL), NOT
"untextured": root 0x09C0 carries its own complete binding out of file 86's
own internal fixups -- 16-entry RGBA16 TLUT at 0x0758, CI4 32x32 image at
0x0780, four vertices at 0x0980.  No word in the program is a segment-E hook
(word-by-word 0xDE scan below), and the combiner is (TEXEL0 - 0) * SHADE + 0
with alpha TEXEL0_A in both cycles, so no PRIM and no ENV are read: this is
the BAKE-EVERYTHING shape, and everything bakes into the emitted setup.

ONE REFERRER, GAME-WIDE.  The `--census` sweep below walks every O2R file in
the image; the default check pins what that sweep reduces to: file 86 holds
exactly ONE internal pointer to root 0x09C0 (slot 0x0AE0, which is DObjDesc
0x0AB0 entry 1's `dl` field), file 251's external table holds 68 pointers
into file 86, of which the only one naming 0x0AB0 is 0x00B8 -- the
Tomato ITAttributes.data -- and exactly one foreign file (YoshiMain, 247)
points at file 86 (slot 0x0040 -> 0x5458).  No second item kind, weapon or effect can reach
the root, so unlike the Peach's Castle bumper this owner needs no live-kind
discriminator; the adapter still reads ITStruct.kind so a future second
referrer records NO_PROGRAM loudly instead of being drawn by a program baked
for the Tomato.

THE SIBLING ROOTS ARE NOT THIS OWNER.  File 86 holds many other same-length
30-word roots (Box, Taru, Egg, Sword parts, Pokemon, ...); they are separate
roots with separate referrers and get their own owners.  The census assertion
below pins every external pointer into file 86, so a sibling that silently
started pointing here would fail the build.

TEMPLATE NOTE (item-specific vs general).  This is the first of the
bake-everything item owners and the template the rest copy: everything in
`decode`/`render` is GENERAL except the constants block (asset ids, offsets,
VERTS, OPS, TEXT_PINS, EXPECTED_* tables) and the anim-joints NULL assertion
(some siblings carry an AnimJoint; those owners pin it instead of asserting
NULL).  The executor gate `NDS_P2_ITEM_CORE` is item-specific; the sibling
owners gate on their own TU flag.
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

OUT = REPO / "src/nds/generated/nds_native_item_tomato.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_item_tomato.generated.h"

MODEL_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscData086",
    "96e987d81b24497ad0c314372edb79f123016b59187b5f94b57b73e4bb122c04",
    86, 402, 0,
    "1c642e60401ca5e6df2fe1b0d6ddeb6e322b0288f4c13a1e608875322fbf9438")
ATTR_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_items/ITCommonData",
    "8ad38613162d33e712f79f9d5584540dadd3fbddad90287dedf8cfc59cc76f32",
    251, 0, 68,
    "264b5ef35dfe41bd22cc94a82a80034e6913b9d1d827839392a6328ac9a60145")

TEXT_PINS = (
    ("decomp/BattleShip-main/decomp/src/it/itcommon/ittomato.c",
     "26023d15ab34d183da3a7a770d691de20d79f280b46e6fc8fd1116610474ab8f",
     ("nITKindTomato,                          // Item Kind",
      "&llITCommonDataTomatoItemAttributes,    // Offset of item attributes in file?")),
    ("decomp/BattleShip-main/include/reloc_data.us.h",
     "8c2d5938590e9a38ca2dad6ac0fa45b4742d125ed5d89f305c38774e40551385",
     ("#define llITCommonDataTomatoItemAttributes ((intptr_t)0xB8)",
      "#define ll_86_FileID ((intptr_t)0x56)")),
    ("decomp/BattleShip-main/decomp/src/it/itmanager.c",
     "47654e634120ff4878136c64d230b054ee307ba137affb2f2a184f0dd812a1c3",
     ("else proc_display = (attr->is_display_xlu) ? itDisplayXLUProcDisplay : itDisplayOPAProcDisplay;",
      "lbCommonEjectTreeDObj(DObjGetStruct(item_gobj));")),
    ("decomp/BattleShip-main/decomp/src/it/itdisplay.c",
     "7e7d038211d13d252cae62b159420fd90a2cb70e8c7a64f4e2eec22990ce8c62",
     ("void itDisplayOPAProcDisplay(GObj *item_gobj)",
      "gcDrawDObjTreeForGObj(item_gobj);")),
    ("decomp/BattleShip-main/decomp/src/lb/lbcommon.c",
     "7c9cfa9f257abb6e9504ecbca0716974ce997a33b0f1a62068c45e033895a01e",
     ("void lbCommonEjectTreeDObj(DObj *dobj)",
      "child_dobj->parent_gobj->obj = child_dobj;")),
)

ASSET = 86
ATTR_ASSET = 251
ATTR_OFFSET = 0x00B8          # llITCommonDataTomatoItemAttributes
DOBJDESC = 0x0AB0             # ITAttributes.data

ROOT = 0x09C0
DL_WORDS = 30
VTX = 0x0980
TLUT_OFFSET = 0x0758
TLUT_ENTRIES = 16
IMAGE_OFFSET = 0x0780         # CI4 32x32
IMAGE_BYTES = 512
# The end of the highest span the executor dereferences.  The vertex pool sits
# at exactly this offset and is baked, so it is never read at runtime.
FILE_END = IMAGE_OFFSET + IMAGE_BYTES

TLUT_WORD = 11
IMAGE_WORD = 17
VTX_WORD = 21
TRI_WORD = 22

OPS = (
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8,
    0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
    0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)
# Item-specific geometry, baked verbatim: a 360x360 quad.  The t range
# (-83..1107) is the measured asset, not a typo; the DS repack keeps it exact.
VERTS = (
    (180, -180, 0, 1024, -83, 0xFFFFFFFF),
    (-180, -180, 0, 0, -83, 0xFFFFFFFF),
    (-180, 180, 0, 0, 1107, 0xFFFFFFFF),
    (180, 180, 0, 1024, 1107, 0xFFFFFFFF),
)
TRIS = ((3, 2, 1), (0, 3, 1))

# The whole-image referrer census, reduced to the tables that can hold a
# pointer into this model.  --census re-derives it over all 2,132 O2R files.
# Item-specific: file 251's external table holds all 68 pointers into file 86
# (data + anim-joint slots of every ITCommonData kind), and exactly one other
# file in the image points at file 86: YoshiMain (247) slot 0x0040 names
# 0x5458, the same offset two file-251 slots name.
EXPECTED_ROOT_REFS = (0x0AE0,)
EXPECTED_FOREIGN_REFS = ((247, 0x0040, 0x5458),)
EXPECTED_ATTR_REFS = (
    (0x0050, ASSET, 0x0670), (0x00B8, ASSET, DOBJDESC),
    (0x0100, ASSET, 0x1158), (0x0148, ASSET, 0x1560),
    (0x014C, ASSET, 0x12B8), (0x0154, ASSET, 0x15F0),
    (0x0190, ASSET, 0x1918), (0x01D8, ASSET, 0x1E00),
    (0x0220, ASSET, 0x2198), (0x0268, ASSET, 0x3F50),
    (0x02B0, ASSET, 0x40A8), (0x02E4, ASSET, 0x46B0),
    (0x02E8, ASSET, 0x4388), (0x02F0, ASSET, 0x4760),
    (0x0374, ASSET, 0x2750), (0x03BC, ASSET, 0x39A0),
    (0x0424, ASSET, 0x33F8), (0x0428, ASSET, 0x3230),
    (0x048C, ASSET, 0x4B60), (0x04D4, ASSET, 0x5458),
    (0x0508, ASSET, 0x5458), (0x053C, ASSET, 0x5F88),
    (0x0540, ASSET, 0x5DE0), (0x0584, ASSET, 0x5F88),
    (0x0588, ASSET, 0x5DE0), (0x05CC, ASSET, 0x6778),
    (0x0634, ASSET, 0x71A8), (0x069C, ASSET, 0x7648),
    (0x06A0, ASSET, 0x7488), (0x06E4, ASSET, 0x9430),
    (0x06E8, ASSET, 0x9120), (0x072C, ASSET, 0xA140),
    (0x0774, ASSET, 0xAB98), (0x0778, ASSET, 0xA9D0),
    (0x07A8, ASSET, 0xB158), (0x07F0, ASSET, 0xB708),
    (0x07F4, ASSET, 0xB540), (0x0838, ASSET, 0xBCC0),
    (0x0880, ASSET, 0xC130), (0x08C8, ASSET, 0xC520),
    (0x08FC, ASSET, 0xD5C0), (0x0900, ASSET, 0xD410),
    (0x098C, ASSET, 0xDF38), (0x0990, ASSET, 0xDD70),
    (0x09D4, ASSET, 0xE4A8), (0x09D8, ASSET, 0xE2E0),
    (0x09E0, ASSET, 0xE560), (0x0A08, ASSET, 0xEA60),
    (0x0A50, ASSET, 0xF9D8), (0x0A54, ASSET, 0xF6C0),
    (0x0A58, ASSET, 0xFA90), (0x0A5C, ASSET, 0xFB70),
    (0x0A84, ASSET, 0x10000), (0x0ACC, ASSET, 0x104A0),
    (0x0AD4, ASSET, 0x10550), (0x0B34, ASSET, 0x112A0),
    (0x0B38, ASSET, 0x110E0), (0x0B7C, ASSET, 0x119D8),
    (0x0BB0, ASSET, 0x11F40), (0x0BF8, ASSET, 0x12820),
    (0x0C40, ASSET, 0x13100), (0x0C44, ASSET, 0x12F78),
    (0x0C48, ASSET, 0x13190), (0x0C4C, ASSET, 0x131E0),
    (0x0C74, ASSET, 0x13598), (0x0CBC, ASSET, 0x13598),
    (0x0CF0, ASSET, 0x7648), (0x0CF4, ASSET, 0x7488),
)


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def check_text_pins() -> None:
    for path, sha, tokens in TEXT_PINS:
        blob = (REPO / path).read_bytes()
        actual = hashlib.sha256(blob).hexdigest()
        if actual != sha:
            raise RuntimeError(f"{path}: SHA256 {actual} != pinned {sha}")
        text = blob.decode("utf-8", "replace")
        for token in tokens:
            if token not in text:
                raise RuntimeError(f"{path} pin missing {token!r}")


def census() -> tuple:
    """Every O2R file in the image, for the root and the DObjDesc."""
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
        raise RuntimeError("file 86 no longer contains the Tomato list")
    raw = words_at(model.payload, ROOT, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != OPS:
        raise RuntimeError(f"Tomato 0x09c0 opcode census changed: {ops!r}")
    if (raw[DL_WORDS - 1][0] >> 24) != 0xDF:
        raise RuntimeError("Tomato program no longer ends in G_ENDDL")
    # The shape discriminator, read off the words rather than trusted: a
    # segment-0xE call (opcode 0xDE targeting 0x0E000000) would make the image
    # live and this bake-everything owner must refuse it loudly.
    for i, (w0, w1) in enumerate(raw):
        if (w0 >> 24) == 0xDE:
            raise RuntimeError(
                f"Tomato word {i} is a display-list call "
                f"(0x{w0:08x} 0x{w1:08x}); owner shape changed")

    # ITAttributes at ITCommonData 0xB8: data, p_mobjsubs, anim_joints,
    # p_matanim_joints.  All three pointer slots are NULL here -- that is why
    # this DObj has mobj == NULL, no AnimJoint and no material animation.
    # (Item-specific: siblings WITH an anim joint pin it instead of this.)
    data_ref = attr.pointer_at(ATTR_OFFSET)
    if data_ref is None or (data_ref.asset_id, data_ref.offset) != (ASSET, DOBJDESC):
        raise RuntimeError(f"Tomato ITAttributes.data changed: {data_ref!r}")
    for name, off in (("p_mobjsubs", 4), ("anim_joints", 8),
                      ("p_matanim_joints", 12)):
        if attr.pointer_at(ATTR_OFFSET + off) is not None:
            raise RuntimeError(f"Tomato ITAttributes.{name} is no longer NULL")
        if struct.unpack_from(">I", attr.payload, ATTR_OFFSET + off)[0] != 0:
            raise RuntimeError(f"Tomato ITAttributes.{name} is no longer NULL")
    # is_display_xlu | is_item_dobjs | is_display_colanim occupy the top three
    # bits of the flag word; all three must stay clear or the display proc, the
    # DObj tree shape and the DL head all change under this owner.
    flags = struct.unpack_from(">I", attr.payload, ATTR_OFFSET + 0x10)[0]
    if (flags & 0xE0000000) != 0:
        raise RuntimeError(f"Tomato display flags changed: {flags:#010x}")

    # The DObjDesc chain: a DL-less root, ONE DL-carrying child, terminator.
    if struct.unpack_from(">i", model.payload, DOBJDESC)[0] != 0:
        raise RuntimeError("Tomato DObjDesc entry 0 id changed")
    if model.pointer_at(DOBJDESC + 4) is not None:
        raise RuntimeError("Tomato DObjDesc entry 0 gained a display list")
    if struct.unpack_from(">i", model.payload, DOBJDESC + 0x2C)[0] != 1:
        raise RuntimeError("Tomato DObjDesc entry 1 id changed")
    ref = model.pointer_at(DOBJDESC + 0x2C + 4)
    if ref is None or (ref.asset_id, ref.offset) != (ASSET, ROOT):
        raise RuntimeError(f"Tomato DObjDesc entry 1 dl changed: {ref!r}")
    if struct.unpack_from(">i", model.payload, DOBJDESC + 2 * 0x2C)[0] != 18:
        raise RuntimeError("Tomato DObjDesc terminator changed")

    # Referrer census: ONE internal pointer to the root; file 251's external
    # table must equal the 68 pinned pointers into file 86, and the
    # whole-image sweep must add only the one pinned foreign referrer.
    refs = tuple(sorted(
        slot for slot, ref in model.internal.items() if ref.offset == ROOT))
    if refs != EXPECTED_ROOT_REFS:
        raise RuntimeError(f"root {ROOT:#06x} referrer census changed: {refs!r}")
    if refs[0] != DOBJDESC + 0x30:
        raise RuntimeError("the only pointer to the root is not the DObjDesc slot")
    attr_refs = tuple(sorted(
        (slot, ref.asset_id, ref.offset)
        for slot, ref in attr.external.items() if ref.asset_id == ASSET))
    if attr_refs != EXPECTED_ATTR_REFS:
        raise RuntimeError(f"file 251 -> 86 census changed: {attr_refs!r}")
    if any(off == ROOT for _, _, off in attr_refs):
        raise RuntimeError("file 251 gained a direct pointer to the root")
    if run_census:
        scanned, hits = census()
        want = tuple(sorted(
            EXPECTED_FOREIGN_REFS +
            tuple((ATTR_ASSET, slot, off) for slot, _, off in EXPECTED_ATTR_REFS)))
        if hits != want:
            raise RuntimeError(
                f"whole-image census into asset {ASSET} changed "
                f"({scanned} files): {hits!r}")

    # Bindings.  Both SETTIMG words are file 86's OWN internal fixups; compare
    # them, never assume the loader's fixup pass ran.
    tlut_ref = model.pointer_at(ROOT + TLUT_WORD * 8 + 4)
    image_ref = model.pointer_at(ROOT + IMAGE_WORD * 8 + 4)
    vtx_ref = model.pointer_at(ROOT + VTX_WORD * 8 + 4)
    for name, ref, want in (("TLUT", tlut_ref, TLUT_OFFSET),
                            ("image", image_ref, IMAGE_OFFSET),
                            ("vertices", vtx_ref, VTX)):
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, want):
            raise RuntimeError(f"Tomato {name} ref changed: {ref!r}")
    if model.external:
        raise RuntimeError("file 86 gained an external fixup")
    if len(model.payload) < FILE_END:
        raise RuntimeError("file 86 no longer contains the Tomato image")

    # The TLUT: index 0 transparent, the other fifteen opaque, sixteen distinct
    # colours, and the image spans every 4-bit index -- so the DS PAL16 repack
    # is exact and GL_TEXTURE_COLOR0_TRANSPARENT keeps applying.
    entries = struct.unpack_from(f">{TLUT_ENTRIES}H", model.payload, TLUT_OFFSET)
    if (entries[0] & 1) != 0:
        raise RuntimeError("Tomato TLUT entry 0 is no longer transparent")
    if any((entry & 1) == 0 for entry in entries[1:]):
        raise RuntimeError("a Tomato TLUT entry other than 0 lost its alpha bit")
    if len(set(entries)) != TLUT_ENTRIES:
        raise RuntimeError("Tomato TLUT entries are no longer distinct")
    nibbles = set()
    for byte in model.payload[IMAGE_OFFSET:IMAGE_OFFSET + IMAGE_BYTES]:
        nibbles.add(byte >> 4)
        nibbles.add(byte & 0x0F)
    if nibbles != set(range(16)):
        raise RuntimeError(
            f"Tomato CI4 image no longer spans all 16 indices: {sorted(nibbles)!r}")

    # The three loads must still describe the spans this owner bounds.
    if (((raw[13][1] >> 14) & 0x3FF) + 1) != TLUT_ENTRIES:
        raise RuntimeError(f"Tomato LOADTLUT count changed: {raw[13][1]:#010x}")
    if ((raw[19][1] >> 12) & 0xFFF) + 1 != IMAGE_BYTES * 2 // 4:
        raise RuntimeError(f"Tomato LOADBLOCK changed: {raw[19][1]:#010x}")
    if raw[16] != (0xF2000000, 0x0007C07C):
        raise RuntimeError(f"Tomato SETTILESIZE changed: {raw[16]!r}")
    # G_VTX: four vertices ending at cache slot four, so v0 is zero.
    vtx_w0 = raw[VTX_WORD][0]
    if ((vtx_w0 >> 12) & 0xFF) != 4 or ((vtx_w0 >> 1) & 0x7F) != 4:
        raise RuntimeError(f"Tomato G_VTX changed: {vtx_w0:#010x}")
    if raw[TRI_WORD] != (0x06060402, 0x00000602):
        raise RuntimeError(f"Tomato triangle word changed: {raw[TRI_WORD]!r}")
    # The combiner reads TEXEL0 and SHADE only: no PRIM, no ENV, so nothing the
    # item layer seeds into stats can change this draw and this owner must not
    # invent a colour of its own.
    if raw[5] != (0xFC127E24, 0xFFFFF3F9):
        raise RuntimeError(f"Tomato SETCOMBINE changed: {raw[5]!r}")

    verts = tuple(sm.decode_vertex(model, VTX + i * 16) for i in range(4))
    if verts != VERTS:
        raise RuntimeError(f"Tomato vertices changed: {verts!r}")
    tris = tuple(sm.decode_triangles(0x06, *raw[TRI_WORD]))
    if tris != TRIS:
        raise RuntimeError(f"Tomato triangles changed: {tris!r}")
    return raw, verts


def render_header(raw) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Maxim Tomato native item constants (generated).")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_item_tomato.py. */")
    a("#ifndef NDS_NATIVE_ITEM_TOMATO_GENERATED_H")
    a("#define NDS_NATIVE_ITEM_TOMATO_GENERATED_H")
    a("")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_ASSET {ASSET}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_ROOT 0x{ROOT:04x}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_DL_BYTES {DL_WORDS * 8}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_TLUT_OFFSET 0x{TLUT_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_IMAGE_OFFSET 0x{IMAGE_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_FILE_END 0x{FILE_END:04x}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_TLUT_W0 0x{raw[TLUT_WORD][0]:08x}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_IMAGE_W0 0x{raw[IMAGE_WORD][0]:08x}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_TLUT_SLOT {TLUT_WORD * 8 + 4}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_IMAGE_SLOT {IMAGE_WORD * 8 + 4}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_VERTEX_OFFSET 0x{VTX:04x}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_VERTEX_COUNT {len(VERTS)}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_TRIANGLE_COUNT {len(TRIS)}u")
    a(f"#define NDS_NATIVE_ITEM_TOMATO_CORNER_COUNT {len(TRIS) * 3}u")
    a("")
    a("#endif")
    return "\n".join(lines) + "\n"


def _othermode(a, raw, index, indent="    "):
    a(f"{indent}ndsRendererRecordOtherMode(stats, 0x{raw[index][0] >> 24:02x}u,"
      f" 0x{raw[index][0]:08x}u, 0x{raw[index][1]:08x}u);")


def render(raw, verts) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Maxim Tomato native item packet (generated).")
    a(" * Source: SHA-pinned file 86 root 0x09c0 (Gfx[30]), vertex pool 0x0980,")
    a(" * 16-entry RGBA16 TLUT 0x0758 and CI4 32x32 image 0x0780 -- every one of")
    a(" * them internal to file 86.  The item has NO MObj (ITCommonData")
    a(" * ITAttributes.p_mobjsubs at 0x00bc is NULL), so the list owns its whole")
    a(" * material and nothing here is a segment-E hook.  The combiner is")
    a(" * (TEXEL0 - 0) * SHADE + 0 in both cycles: no PRIM and no ENV are read.")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_item_tomato.py. */")
    a("#include <nds/generated/nds_native_item_tomato.generated.h>")
    a("")
    a(f"static const u16 sNdsNativeItemTomatoTriIndices[{len(TRIS) * 3}] =")
    a("{")
    for tri in TRIS:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    a(f"static const s16 sNdsNativeItemTomatoVerts[{len(verts) * 5}] =")
    a("{")
    for v in verts:
        a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeItemTomatoVertColors[{len(verts)}] =")
    a("{")
    for v in verts:
        a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a("/* Words 0/7/12/14/18/20/23/24 are pipe/tile/load syncs and word 29 is")
    a(" * ENDDL: they carry no state.  Every state word is emitted in source")
    a(" * order, so the SETTIMG that names the palette still precedes the")
    a(" * LOADTLUT that latches texture_tlut_image and the image SETTIMG still")
    a(" * follows it. */")
    a("static void ndsNativeItemTomatoSetup(")
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
    a("static void ndsNativeItemTomatoFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) |"
      f" 0x{raw[25][1]:08x}u;")
    for i in (26, 27, 28):
        _othermode(a, raw, i)
    a("}")
    a("")
    a(f"/* census: dl_words={DL_WORDS} verts={len(verts)} tris={len(TRIS)}"
      f" material=none tlut={ASSET}:0x{TLUT_OFFSET:04x}"
      f" image={ASSET}:0x{IMAGE_OFFSET:04x} referrers=Tomato */")
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
                raise RuntimeError(
                    f"generated artefact stale: {path.relative_to(REPO)}")
        print("ITEM_TOMATO_NATIVE_OK root=0x09c0 verts=4 tris=2 "
              "material=none tlut=86:0x0758 image=86:0x0780 referrers=Tomato")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
