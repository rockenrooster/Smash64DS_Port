#!/usr/bin/env python3
"""Generate/check Saffron City's GLucky (Chansey) native item program.

WHAT THIS OBJECT IS.  `llGRYamabukiMapGLuckyItemAttributes` is 0xBC
(reloc_data.us.h:4000) and `dITGLuckyItemDesc` names it beside
`nITKindGLucky` (itglucky.c:12-16), so the ITAttributes at GRYamabukiMap
(file 264) + 0xBC IS Chansey.  It is a STAGE hazard, not a Poke Ball
Pokemon: `nITKindGLucky` sits in the ground-monster range (port
include/it/item.h:896-903) and `grYamabukiGateMakeMonster`
(gryamabuki.c) is its spawner.  Its `data` field is file 264's external
fixup at slot 0x00BC, which resolves to file 159 (MiscDataBank159 /
StageYamabukiFile3) 0x0360 -- a DObjDesc[3] whose entry 1 draws root
0x0270 and whose entry 2 is the id-18 terminator.

ONE LIST PER DRAWN FRAME.  `is_item_dobjs` is SET, so `itManagerMakeItem`
builds the tree with `itManagerSetupItemDObjs` (itmanager.c:385) and then
`lbCommonEjectTreeDObj` (itmanager.c:397, lbcommon.c) DELETES the DL-less
root, promoting the single DL-carrying child to be the GObj's DObj.
`is_display_colanim` and `is_display_xlu` are both clear, so the display
proc is `itDisplayOPAProcDisplay` (itmanager.c:255), which calls
`gcDrawDObjTreeForGObj` once.  One item, one DObj, one display list, DL
head 0.

MATERIAL 0 IS `p_mobjsubs == NULL` (GRYamabukiMap ITAttributes at 0x00C0
is NULL), NOT "untextured": root 0x0270 carries its own complete binding
out of file 159's own internal fixups -- 16-entry RGBA16 TLUT at 0x0008,
CI4 32x32 image at 0x0030, four vertices at 0x0230.  Nothing in the
program is a segment-E hook (word-by-word 0xDE scan below), and the
combiner is (TEXEL0 - 0) * SHADE + 0 with alpha TEXEL0_A in both cycles,
so no PRIM and no ENV are read: this is the BAKE-EVERYTHING shape, and
everything bakes into the emitted setup.

ONE REFERRER, GAME-WIDE.  The `--census` sweep below walks every O2R file
in the image; the default check pins what that sweep reduces to: file 159
holds exactly ONE internal pointer to root 0x0270 (slot 0x0390, which is
DObjDesc 0x0360 entry 1's `dl` field), and file 264 holds exactly thirteen
external pointers into file 159, of which the only one naming 0x0360 is
0x00BC -- the GLucky ITAttributes.data.  No second item kind, weapon or
effect can reach the root, so unlike the Peach's Castle bumper this owner
needs no live-kind discriminator; the adapter still reads ITStruct.kind so
a future second referrer records NO_PROGRAM loudly instead of being drawn
by a program baked for Chansey.

THE FOUR SIBLINGS ARE NOT THIS OWNER.  Marumine 0x06A0, Porygon 0x0DB0,
Hitokage 0x18A0 and Fushigibana 0x2250 are the same 30-word template in the
same file, reached from 264:0x0104/0x016C/0x01FC/0x0278.  They are separate
roots with separate referrers and get their own owners; the census
assertion below pins every one of those thirteen pointers, so a sibling
that silently started pointing here would fail the build.

TEMPLATE NOTE (item-specific vs general).  This copies the tomato /
Marumine bake-everything owners: everything in `decode`/`render` is
GENERAL except the constants block (asset ids, offsets, VERTS, OPS,
TEXT_PINS, EXPECTED_* tables, TLUT/image span pins), the anim-joint pin
(GLucky carries an AnimJoint like Marumine; joint-less siblings assert NULL
instead), the `(flags & 0xE0000000)` gate (GLucky has is_item_dobjs SET, so
the gate pins 0x40000000 rather than 0) and the `NDS_P2_ITEM_CORE`
executor gate.
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

OUT = REPO / "src/nds/generated/nds_native_item_glucky.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_item_glucky.generated.h"

MODEL_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank159",
    "7b8c444662623b4a1948c82371401cdae5bdf3108223b798b696542adba0d1b2",
    159, 43, 0,
    "977292623f0a4a3e5175fc77e4e39105d23e76fca4bf0ebc76613413f84e0ab6")
ATTR_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRYamabukiMap",
    "8e17aa95be010e865711353a8ea965f9dc4f8ec9c17f615e453e0a8b3fb4505d",
    264, 1, 23,
    "4236015e06f24d4f8b701053b89df003bae5eac210cf7d3b7fbcddaf5fc53b85")

TEXT_PINS = (
    ("decomp/BattleShip-main/decomp/src/it/itground/itglucky.c",
     "990c9babd95ef1601d9a6794303408dc823b3a36a3a33193968e4fa6ccd8c50e",
     ("nITKindGLucky,                          // Item Kind",
      "&llGRYamabukiMapGLuckyItemAttributes,   // Offset of item attributes in file?")),
    ("decomp/BattleShip-main/include/reloc_data.us.h",
     "8c2d5938590e9a38ca2dad6ac0fa45b4742d125ed5d89f305c38774e40551385",
     ("#define llGRYamabukiMapGLuckyItemAttributes ((intptr_t)0xBC)",
      "#define ll_159_FileID ((intptr_t)0x9f)")),
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
    ("decomp/BattleShip-main/decomp/src/gr/grcommon/gryamabuki.c",
     "fbeccef139f60567aec93c5c31f1e328787856b31a033cb7edaecfd6ba9dc06b",
     ("gGRCommonStruct.yamabuki.monster_gobj = itManagerMakeItemSetupCommon(",)),
)

ASSET = 159
ATTR_ASSET = 264
ATTR_OFFSET = 0x00BC          # llGRYamabukiMapGLuckyItemAttributes
DOBJDESC = 0x0360             # ITAttributes.data
ANIMJOINT = 0x03F0            # ITAttributes.anim_joints

ROOT = 0x0270
DL_WORDS = 30
VTX = 0x0230
TLUT_OFFSET = 0x0008
TLUT_ENTRIES = 16
IMAGE_OFFSET = 0x0030         # CI4 32x32
IMAGE_BYTES = 512
# The end of the highest span the executor dereferences.  The vertex pool
# sits at exactly this offset and is baked, so it is never read at runtime.
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
# Item-specific geometry, baked verbatim.  The x range (-315..270) is the
# measured asset, not a typo; the DS repack keeps it exact.
VERTS = (
    (270, 270, 0, 1024, 1024, 0xFFFFFFFF),
    (270, -270, 0, 1024, 0, 0xFFFFFFFF),
    (-315, -270, 0, 0, 0, 0xFFFFFFFF),
    (-315, 270, 0, 0, 1024, 0xFFFFFFFF),
)
TRIS = ((3, 2, 1), (0, 3, 1))

# The whole-image referrer census, reduced to the two tables that can hold a
# pointer into this model.  --census re-derives it over all 2,132 O2R files.
# Item-specific only in EXPECTED_ROOT_REFS; the attr table is file 264's
# thirteen pointers into file 159, shared with the Marumine owner.
EXPECTED_ROOT_REFS = (0x0390,)
EXPECTED_ATTR_REFS = (
    (0x00BC, ASSET, 0x0360), (0x00C4, ASSET, 0x03F0),
    (0x0104, ASSET, 0x0790), (0x010C, ASSET, 0x0820),
    (0x016C, ASSET, 0x0EA0), (0x0174, ASSET, 0x0F30),
    (0x01FC, ASSET, 0x1990), (0x0200, ASSET, 0x17D0),
    (0x0204, ASSET, 0x1A20), (0x0278, ASSET, 0x2340),
    (0x027C, ASSET, 0x2180), (0x0280, ASSET, 0x23D0),
    (0x0308, ASSET, 0x2A50),
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
        raise RuntimeError("file 159 no longer contains the GLucky list")
    raw = words_at(model.payload, ROOT, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != OPS:
        raise RuntimeError(f"GLucky 0x0270 opcode census changed: {ops!r}")
    if (raw[DL_WORDS - 1][0] >> 24) != 0xDF:
        raise RuntimeError("GLucky program no longer ends in G_ENDDL")
    # The shape discriminator, read off the words rather than trusted: a
    # segment-0xE call (opcode 0xDE) would make the image live and this
    # bake-everything owner must refuse it loudly.
    for i, (w0, w1) in enumerate(raw):
        if (w0 >> 24) == 0xDE:
            raise RuntimeError(
                f"GLucky word {i} is a display-list call "
                f"(0x{w0:08x} 0x{w1:08x}); owner shape changed")

    # ITAttributes at GRYamabukiMap 0xBC: data, p_mobjsubs, anim_joints,
    # p_matanim_joints.  The two NULLs are why this DObj has mobj == NULL and
    # why no material animation can touch the list.
    # (Item-specific: this sibling carries an AnimJoint like Marumine, so the
    # joint is pinned instead of asserted NULL.)
    data_ref = attr.pointer_at(ATTR_OFFSET)
    anim_ref = attr.pointer_at(ATTR_OFFSET + 8)
    if data_ref is None or (data_ref.asset_id, data_ref.offset) != (ASSET, DOBJDESC):
        raise RuntimeError(f"GLucky ITAttributes.data changed: {data_ref!r}")
    if anim_ref is None or (anim_ref.asset_id, anim_ref.offset) != (ASSET, ANIMJOINT):
        raise RuntimeError(f"GLucky ITAttributes.anim_joints changed: {anim_ref!r}")
    for name, off in (("p_mobjsubs", 4), ("p_matanim_joints", 12)):
        if attr.pointer_at(ATTR_OFFSET + off) is not None:
            raise RuntimeError(f"GLucky ITAttributes.{name} is no longer NULL")
        if struct.unpack_from(">I", attr.payload, ATTR_OFFSET + off)[0] != 0:
            raise RuntimeError(f"GLucky ITAttributes.{name} is no longer NULL")
    # is_display_xlu | is_item_dobjs | is_display_colanim occupy the top three
    # bits of the flag word (ittypes.h:149-151).  GLucky has is_item_dobjs SET
    # (item-DObj setup at itmanager.c:385, still ejected at :397), while xlu
    # and colanim stay clear so the display proc is still
    # itDisplayOPAProcDisplay (itmanager.c:251-255).  The flag word reads
    # 0x58000000; the 0x18000000 above the display bits is hitlag/weight and
    # does not reach the draw.
    flags = struct.unpack_from(">I", attr.payload, ATTR_OFFSET + 0x10)[0]
    if (flags & 0xE0000000) != 0x40000000:
        raise RuntimeError(f"GLucky display flags changed: {flags:#010x}")

    # The DObjDesc chain: a DL-less root, ONE DL-carrying child, terminator.
    if struct.unpack_from(">i", model.payload, DOBJDESC)[0] != 0:
        raise RuntimeError("GLucky DObjDesc entry 0 id changed")
    if model.pointer_at(DOBJDESC + 4) is not None:
        raise RuntimeError("GLucky DObjDesc entry 0 gained a display list")
    if struct.unpack_from(">i", model.payload, DOBJDESC + 0x2C)[0] != 1:
        raise RuntimeError("GLucky DObjDesc entry 1 id changed")
    ref = model.pointer_at(DOBJDESC + 0x2C + 4)
    if ref is None or (ref.asset_id, ref.offset) != (ASSET, ROOT):
        raise RuntimeError(f"GLucky DObjDesc entry 1 dl changed: {ref!r}")
    if struct.unpack_from(">i", model.payload, DOBJDESC + 2 * 0x2C)[0] != 18:
        raise RuntimeError("GLucky DObjDesc terminator changed")
    # anim_joints[0] is NULL and anim_joints[1] is the roll-in AnimJoint: the
    # animation lives on the SAME DObj that carries the list.
    if model.pointer_at(ANIMJOINT) is not None:
        raise RuntimeError("GLucky anim_joints[0] is no longer NULL")
    ref = model.pointer_at(ANIMJOINT + 4)
    if ref is None or (ref.asset_id, ref.offset) != (ASSET, 0x03F8):
        raise RuntimeError(f"GLucky anim_joints[1] changed: {ref!r}")

    # Referrer census: ONE internal pointer to the root, and file 264 holds
    # every external pointer into file 159.
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
        raise RuntimeError(f"file 264 -> 159 census changed: {attr_refs!r}")
    if any(off == ROOT for _, _, off in attr_refs):
        raise RuntimeError("file 264 gained a direct pointer to the root")
    if run_census:
        scanned, hits = census()
        want = tuple((ATTR_ASSET, slot, off) for slot, _, off in EXPECTED_ATTR_REFS)
        if hits != want:
            raise RuntimeError(
                f"whole-image census into asset {ASSET} changed "
                f"({scanned} files): {hits!r}")

    # Bindings.  Both SETTIMG words are file 159's OWN internal fixups; compare
    # them, never assume the loader's fixup pass ran.
    tlut_ref = model.pointer_at(ROOT + TLUT_WORD * 8 + 4)
    image_ref = model.pointer_at(ROOT + IMAGE_WORD * 8 + 4)
    vtx_ref = model.pointer_at(ROOT + VTX_WORD * 8 + 4)
    for name, ref, want in (("TLUT", tlut_ref, TLUT_OFFSET),
                            ("image", image_ref, IMAGE_OFFSET),
                            ("vertices", vtx_ref, VTX)):
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, want):
            raise RuntimeError(f"GLucky {name} ref changed: {ref!r}")
    if model.external:
        raise RuntimeError("file 159 gained an external fixup")
    if len(model.payload) < FILE_END:
        raise RuntimeError("file 159 no longer contains the GLucky image")

    # The TLUT: index 0 transparent, the other fifteen opaque, and the image
    # spans every 4-bit index -- so the DS PAL16 repack is exact and
    # GL_TEXTURE_COLOR0_TRANSPARENT keeps applying.  (No distinctness claim:
    # 0xECE5 sits at both index 2 and index 14 in the measured asset; the
    # repack copies the palette verbatim so the duplicate is exact too.)
    entries = struct.unpack_from(f">{TLUT_ENTRIES}H", model.payload, TLUT_OFFSET)
    if (entries[0] & 1) != 0:
        raise RuntimeError("GLucky TLUT entry 0 is no longer transparent")
    if any((entry & 1) == 0 for entry in entries[1:]):
        raise RuntimeError("a GLucky TLUT entry other than 0 lost its alpha bit")
    nibbles = set()
    for byte in model.payload[IMAGE_OFFSET:IMAGE_OFFSET + IMAGE_BYTES]:
        nibbles.add(byte >> 4)
        nibbles.add(byte & 0x0F)
    if nibbles != set(range(16)):
        raise RuntimeError(
            f"GLucky CI4 image no longer spans all 16 indices: {sorted(nibbles)!r}")

    # The three loads must still describe the spans this owner bounds.
    if (((raw[13][1] >> 14) & 0x3FF) + 1) != TLUT_ENTRIES:
        raise RuntimeError(f"GLucky LOADTLUT count changed: {raw[13][1]:#010x}")
    if ((raw[19][1] >> 12) & 0xFFF) + 1 != IMAGE_BYTES * 2 // 4:
        raise RuntimeError(f"GLucky LOADBLOCK changed: {raw[19][1]:#010x}")
    if raw[16] != (0xF2000000, 0x0007C07C):
        raise RuntimeError(f"GLucky SETTILESIZE changed: {raw[16]!r}")
    # G_VTX: four vertices ending at cache slot four, so v0 is zero.
    vtx_w0 = raw[VTX_WORD][0]
    if ((vtx_w0 >> 12) & 0xFF) != 4 or ((vtx_w0 >> 1) & 0x7F) != 4:
        raise RuntimeError(f"GLucky G_VTX changed: {vtx_w0:#010x}")
    if raw[TRI_WORD] != (0x06060402, 0x00000602):
        raise RuntimeError(f"GLucky triangle word changed: {raw[TRI_WORD]!r}")
    # The combiner reads TEXEL0 and SHADE only: no PRIM, no ENV, so nothing the
    # item layer seeds into stats can change this draw and this owner must not
    # invent a colour of its own.
    if raw[5] != (0xFC127E24, 0xFFFFF3F9):
        raise RuntimeError(f"GLucky SETCOMBINE changed: {raw[5]!r}")

    verts = tuple(sm.decode_vertex(model, VTX + i * 16) for i in range(4))
    if verts != VERTS:
        raise RuntimeError(f"GLucky vertices changed: {verts!r}")
    tris = tuple(sm.decode_triangles(0x06, *raw[TRI_WORD]))
    if tris != TRIS:
        raise RuntimeError(f"GLucky triangles changed: {tris!r}")
    return raw, verts


def render_header(raw) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Saffron City GLucky native item constants (generated).")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_item_glucky.py. */")
    a("#ifndef NDS_NATIVE_ITEM_GLUCKY_GENERATED_H")
    a("#define NDS_NATIVE_ITEM_GLUCKY_GENERATED_H")
    a("")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_ASSET {ASSET}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_ROOT 0x{ROOT:04x}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_DL_BYTES {DL_WORDS * 8}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_TLUT_OFFSET 0x{TLUT_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_IMAGE_OFFSET 0x{IMAGE_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_FILE_END 0x{FILE_END:04x}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_TLUT_W0 0x{raw[TLUT_WORD][0]:08x}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_IMAGE_W0 0x{raw[IMAGE_WORD][0]:08x}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_TLUT_SLOT {TLUT_WORD * 8 + 4}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_IMAGE_SLOT {IMAGE_WORD * 8 + 4}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_VERTEX_OFFSET 0x{VTX:04x}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_VERTEX_COUNT {len(VERTS)}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_TRIANGLE_COUNT {len(TRIS)}u")
    a(f"#define NDS_NATIVE_ITEM_GLUCKY_CORNER_COUNT {len(TRIS) * 3}u")
    a("")
    a("#endif")
    return "\n".join(lines) + "\n"


def _othermode(a, raw, index, indent="    "):
    a(f"{indent}ndsRendererRecordOtherMode(stats, 0x{raw[index][0] >> 24:02x}u,"
      f" 0x{raw[index][0]:08x}u, 0x{raw[index][1]:08x}u);")


def render(raw, verts) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Saffron City GLucky native item packet (generated).")
    a(" * Source: SHA-pinned file 159 root 0x0270 (Gfx[30]), vertex pool 0x0230,")
    a(" * 16-entry RGBA16 TLUT 0x0008 and CI4 32x32 image 0x0030 -- every one of")
    a(" * them internal to file 159.  The item has NO MObj (GRYamabukiMap")
    a(" * ITAttributes.p_mobjsubs at 0x00c0 is NULL), so the list owns its whole")
    a(" * material and nothing here is a segment-E hook.  The combiner is")
    a(" * (TEXEL0 - 0) * SHADE + 0 in both cycles: no PRIM and no ENV are read.")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_item_glucky.py. */")
    a("#include <nds/generated/nds_native_item_glucky.generated.h>")
    a("")
    a(f"static const u16 sNdsNativeItemGLuckyTriIndices[{len(TRIS) * 3}] =")
    a("{")
    for tri in TRIS:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    a(f"static const s16 sNdsNativeItemGLuckyVerts[{len(verts) * 5}] =")
    a("{")
    for v in verts:
        a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeItemGLuckyVertColors[{len(verts)}] =")
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
    a("static void ndsNativeItemGLuckySetup(")
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
    a("static void ndsNativeItemGLuckyFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) |"
      f" 0x{raw[25][1]:08x}u;")
    for i in (26, 27, 28):
        _othermode(a, raw, i)
    a("}")
    a("")
    a(f"/* census: dl_words={DL_WORDS} verts={len(verts)} tris={len(TRIS)}"
      f" material=none tlut={ASSET}:0x{TLUT_OFFSET:04x}"
      f" image={ASSET}:0x{IMAGE_OFFSET:04x} referrers=GLucky */")
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
        print("ITEM_GLUCKY_NATIVE_OK root=0x0270 verts=4 tris=2 "
              "material=none tlut=159:0x0008 image=159:0x0030 referrers=GLucky")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
