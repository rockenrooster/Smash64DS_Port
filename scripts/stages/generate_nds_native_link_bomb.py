#!/usr/bin/env python3
"""Generate/check Link's Bomb: the exact native item program for BOTH of its
display lists.

WHAT THIS OBJECT IS.  `llLinkMainBombItemAttributes` is 0x40
(reloc_data.us.h:3709) and `dItLinkBombItemDesc` names it beside
`nITKindLinkBomb` (itlinkbomb.c:20-25), so the ITAttributes at LinkMain (file
225) + 0x40 IS Link's Bomb.  Its `data` field is file 225's external fixup at
slot 0x0040, which resolves to file 353 (LinkSpecial2) 0x18D8 -- a DObjDesc[4]
whose entry 1 draws root 0x16F8 and whose entry 2 draws root 0x17E8.  The
decomp names that whole blob `dLinkSpecial2_SpinAttackMatAnimJoint_*` because
the extractor carved one raw range starting at
`llLinkSpecial2SpinAttackMatAnimJoint` (0x12F0); only the first 0x58 bytes of
that range are the Spin Attack material animation.  Everything from 0x1348 on
is the bomb, and the name is a misnomer.

TWO LISTS, ONE ITEM, ONE TREE WALK.  `p_mobjsubs` at LinkMain 0x44 is NULL, so
neither DObj carries an MObj -- "material 0" in the failure record is that NULL
and NOT "untextured".  Both lists carry their own complete binding out of file
353's own internal fixups: 0x16F8 loads TLUT 0x1348 + CI4 32x32 image 0x1478,
0x17E8 loads IA8 16x16 image 0x1370.  `is_display_colanim` and
`is_display_xlu` are both set in the attribute word at 0x50 (0xB8000000), so
`itDisplayColAnimXLU` (itdisplay.c:283) walks the tree ONCE through
`gcDrawDObjTreeDLLinksForGObj` and emits 0x16F8 into DL head 0 and 0x17E8 into
DL head 1.  One bomb therefore offers TWO lists per drawn frame, in that order,
which is why the first-failure record names 0x16F8 and not 0x17E8.

ONE REFERRER EACH, GAME-WIDE.  The `--census` sweep below walks every O2R file
in the image; the default check pins the two censuses that sweep reduces to:
file 353 holds exactly one internal pointer to each root (0x18BC -> 0x16F8,
0x18CC -> 0x17E8), and file 225 holds the only three external pointers into
file 353 (0x0004 -> 0x03F8 entry wave, 0x0040 -> 0x18D8, 0x0048 -> 0x1990).
No second item kind, weapon or effect can reach either root, so unlike the
Peach's Castle bumper this owner needs no live-kind discriminator; the adapter
still reads ITStruct.kind so that a future second referrer records NO_PROGRAM
loudly instead of being drawn by a program baked for the bomb.
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

OUT = REPO / "src/nds/generated/nds_native_link_bomb.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_link_bomb.generated.h"

BOMB_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/LinkSpecial2",
    "3decd2670e012cffb135b47b4caabf66db1b90fd637f45408a3e8641f1ea31f1",
    353, 44, 0,
    "a89ebaf67c241c07d911b1d0cf74229111ebe55ce8c3b3f5494c357d036e093c")
ATTR_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/LinkMain",
    "8a771a6e2b9e9d8e4b1b103d4c2be332e7290c36adf47c324822c2957797a005",
    225, 21, 93,
    "cb98c41dd1e42d259c21ac3a5da1617740d896ab42e0165e358c170252b542c2")

TEXT_PINS = (
    ("decomp/BattleShip-main/decomp/src/it/itfighter/itlinkbomb.c",
     "3398aeb24f15192153249cdc6525031550ffd70c9f80ee5d76e561ced33c2f1e",
     ("nITKindLinkBomb, \t\t\t\t\t\t// Item Kind",
      "&llLinkMainBombItemAttributes,\t\t\t// Offset of item attributes in file?")),
    ("decomp/BattleShip-main/include/reloc_data.us.h",
     "8c2d5938590e9a38ca2dad6ac0fa45b4742d125ed5d89f305c38774e40551385",
     ("#define llLinkMainBombItemAttributes ((intptr_t)0x40)",
      "#define llLinkSpecial2FileID ((intptr_t)0x161)")),
    ("decomp/BattleShip-main/decomp/src/it/itmanager.c",
     "47654e634120ff4878136c64d230b054ee307ba137affb2f2a184f0dd812a1c3",
     ("proc_display = (attr->is_display_xlu) ? itDisplayColAnimXLUProcDisplay : itDisplayColAnimOPAProcDisplay;",
      "if (!(attr->is_item_dobjs))")),
    ("decomp/BattleShip-main/decomp/src/it/itdisplay.c",
     "7e7d038211d13d252cae62b159420fd90a2cb70e8c7a64f4e2eec22990ce8c62",
     ("void itDisplayColAnimXLU(GObj *item_gobj)",
      "gcDrawDObjTreeDLLinksForGObj(item_gobj);")),
    ("decomp/BattleShip-main/decomp/src/lb/lbcommon.c",
     "7c9cfa9f257abb6e9504ecbca0716974ce997a33b0f1a62068c45e033895a01e",
     ("current_dobj = array_dobjs[id] = gcAddChildForDObj(array_dobjs[id - 1], dobjdesc->dl);",)),
)

ASSET = 353
ATTR_OFFSET = 0x0040          # llLinkMainBombItemAttributes
DOBJDESC = 0x18D8             # ITAttributes.data
ANIMJOINT = 0x1990            # ITAttributes.anim_joints
BODY_DLLINK = 0x18B8
FUSE_DLLINK = 0x18C8

BODY_ROOT = 0x16F8
BODY_WORDS = 30
BODY_VTX = 0x1678
FUSE_ROOT = 0x17E8
FUSE_WORDS = 26
FUSE_VTX = 0x16B8

TLUT_OFFSET = 0x1348
TLUT_ENTRIES = 16
BODY_IMAGE_OFFSET = 0x1478    # CI4 32x32
BODY_IMAGE_BYTES = 512
FUSE_IMAGE_OFFSET = 0x1370    # IA8 16x16
FUSE_IMAGE_BYTES = 256
# The end of the highest span the executor actually dereferences.  The vertex
# pools sit above it and are baked, so they are never read at runtime.
FILE_END = BODY_IMAGE_OFFSET + BODY_IMAGE_BYTES

BODY_TLUT_WORD = 11
BODY_IMAGE_WORD = 17
FUSE_IMAGE_WORD = 13

BODY_OPS = (
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8,
    0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
    0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)
FUSE_OPS = (
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xFC, 0xFA, 0xFB,
    0xF9, 0xF5, 0xF5, 0xD7, 0xF2, 0xFD, 0xE6, 0xF3,
    0xE7, 0x01, 0x06, 0xE7, 0xE7, 0xD9, 0xE3, 0xE2,
    0xE2, 0xDF,
)
BODY_VERTS = (
    (113, -113, 0, 0, 0, 0xFFFFFFFF),
    (-113, -113, 0, 1023, 0, 0xFFFFFFFF),
    (-113, 113, 0, 1024, 1024, 0xFFFFFFFF),
    (113, 113, 0, 0, 1024, 0xFFFFFFFF),
)
FUSE_VERTS = (
    (63, -63, 0, 0, 0, 0x00007FFF),
    (-63, -63, 0, 1024, 0, 0x00007FFF),
    (-63, 63, 0, 1024, 1024, 0x00007FFF),
    (63, 63, 0, 0, 1024, 0x00007FFF),
)
TRIS = ((3, 2, 1), (0, 3, 1))

# The whole-image referrer census, reduced to the two tables that can hold a
# pointer into this model.  --census re-derives it over all 2,132 O2R files.
EXPECTED_BODY_REFS = (0x18BC,)
EXPECTED_FUSE_REFS = (0x18CC,)
EXPECTED_ATTR_REFS = ((0x0004, 353, 0x03F8), (0x0040, 353, DOBJDESC),
                      (0x0048, 353, ANIMJOINT))


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def check_text_pins() -> None:
    for path, sha, tokens in TEXT_PINS:
        data = sm.checked_bytes(REPO, sm.InputSpec(path, sha))
        text = data.decode("utf-8", "replace")
        for token in tokens:
            if token not in text:
                raise RuntimeError(f"{path} pin missing {token!r}")


def census() -> tuple:
    """Every O2R file in the image, for the two roots and the DObjDesc."""
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
    bomb = sm.load_o2r(REPO, BOMB_FILE)
    attr = sm.load_o2r(REPO, ATTR_FILE)
    check_text_pins()

    if len(bomb.payload) < FUSE_ROOT + FUSE_WORDS * 8:
        raise RuntimeError("file 353 no longer contains the bomb lists")
    body = words_at(bomb.payload, BODY_ROOT, BODY_WORDS)
    fuse = words_at(bomb.payload, FUSE_ROOT, FUSE_WORDS)
    if tuple(w0 >> 24 for w0, _ in body) != BODY_OPS:
        raise RuntimeError("bomb body 0x16f8 opcode census changed")
    if tuple(w0 >> 24 for w0, _ in fuse) != FUSE_OPS:
        raise RuntimeError("bomb fuse 0x17e8 opcode census changed")

    # ITAttributes at LinkMain 0x40: data, p_mobjsubs, anim_joints,
    # p_matanim_joints.  The two NULLs are why every DObj here has mobj == NULL
    # and why no material animation can touch either list.
    data_ref = attr.pointer_at(ATTR_OFFSET)
    anim_ref = attr.pointer_at(ATTR_OFFSET + 8)
    if data_ref is None or (data_ref.asset_id, data_ref.offset) != (ASSET, DOBJDESC):
        raise RuntimeError(f"LinkBomb ITAttributes.data changed: {data_ref!r}")
    if anim_ref is None or (anim_ref.asset_id, anim_ref.offset) != (ASSET, ANIMJOINT):
        raise RuntimeError(f"LinkBomb ITAttributes.anim_joints changed: {anim_ref!r}")
    for name, off in (("p_mobjsubs", 4), ("p_matanim_joints", 12)):
        if attr.pointer_at(ATTR_OFFSET + off) is not None:
            raise RuntimeError(f"LinkBomb ITAttributes.{name} is no longer NULL")
        if struct.unpack_from(">I", attr.payload, ATTR_OFFSET + off)[0] != 0:
            raise RuntimeError(f"LinkBomb ITAttributes.{name} is no longer NULL")
    flags = struct.unpack_from(">I", attr.payload, ATTR_OFFSET + 0x10)[0]
    if (flags & 0xB8000000) != 0xB8000000:
        raise RuntimeError(f"LinkBomb display flags changed: {flags:#010x}")

    # The DObjDesc chain, in the order lbCommonSetupTreeDObjs builds it.
    for index, (dl_link, root) in enumerate(
            ((BODY_DLLINK, BODY_ROOT), (FUSE_DLLINK, FUSE_ROOT)), start=1):
        entry = DOBJDESC + index * 0x2C
        if struct.unpack_from(">i", bomb.payload, entry)[0] != index:
            raise RuntimeError(f"bomb DObjDesc entry {index} id changed")
        ref = bomb.pointer_at(entry + 4)
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, dl_link):
            raise RuntimeError(f"bomb DObjDesc entry {index} dl changed: {ref!r}")
        if struct.unpack_from(">i", bomb.payload, dl_link)[0] != index - 1:
            raise RuntimeError(f"bomb DObjDLLink {dl_link:#06x} head changed")
        ref = bomb.pointer_at(dl_link + 4)
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, root):
            raise RuntimeError(f"bomb DObjDLLink {dl_link:#06x} list changed: {ref!r}")
        if struct.unpack_from(">i", bomb.payload, dl_link + 8)[0] != 4:
            raise RuntimeError(f"bomb DObjDLLink {dl_link:#06x} lost its terminator")
    if struct.unpack_from(">i", bomb.payload, DOBJDESC + 3 * 0x2C)[0] != 18:
        raise RuntimeError("bomb DObjDesc terminator changed")

    # Referrer census: one internal pointer to each root, and LinkMain holds
    # every external pointer into file 353.
    for root, expected in ((BODY_ROOT, EXPECTED_BODY_REFS),
                           (FUSE_ROOT, EXPECTED_FUSE_REFS)):
        refs = tuple(sorted(
            slot for slot, ref in bomb.internal.items() if ref.offset == root))
        if refs != expected:
            raise RuntimeError(f"root {root:#06x} referrer census changed: {refs!r}")
    attr_refs = tuple(sorted(
        (slot, ref.asset_id, ref.offset)
        for slot, ref in attr.external.items() if ref.asset_id == ASSET))
    if attr_refs != EXPECTED_ATTR_REFS:
        raise RuntimeError(f"file 225 -> 353 census changed: {attr_refs!r}")
    if run_census:
        scanned, hits = census()
        if hits != tuple((225, slot, off) for slot, _, off in EXPECTED_ATTR_REFS):
            raise RuntimeError(
                f"whole-image census into asset 353 changed ({scanned} files): {hits!r}")

    # Bindings.  Both SETTIMG words are file 353's OWN internal fixups; compare
    # them, never assume the loader's fixup pass ran.
    tlut_ref = bomb.pointer_at(BODY_ROOT + BODY_TLUT_WORD * 8 + 4)
    body_image_ref = bomb.pointer_at(BODY_ROOT + BODY_IMAGE_WORD * 8 + 4)
    fuse_image_ref = bomb.pointer_at(FUSE_ROOT + FUSE_IMAGE_WORD * 8 + 4)
    body_vtx_ref = bomb.pointer_at(BODY_ROOT + 21 * 8 + 4)
    fuse_vtx_ref = bomb.pointer_at(FUSE_ROOT + 17 * 8 + 4)
    for name, ref, want in (("body TLUT", tlut_ref, TLUT_OFFSET),
                            ("body image", body_image_ref, BODY_IMAGE_OFFSET),
                            ("fuse image", fuse_image_ref, FUSE_IMAGE_OFFSET),
                            ("body vertices", body_vtx_ref, BODY_VTX),
                            ("fuse vertices", fuse_vtx_ref, FUSE_VTX)):
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, want):
            raise RuntimeError(f"bomb {name} ref changed: {ref!r}")
    if bomb.external:
        raise RuntimeError("file 353 gained an external fixup")
    if len(bomb.payload) < BODY_IMAGE_OFFSET + BODY_IMAGE_BYTES:
        raise RuntimeError("file 353 no longer contains the bomb image")
    entries = struct.unpack_from(f">{TLUT_ENTRIES}H", bomb.payload, TLUT_OFFSET)
    if (entries[0] & 1) != 0:
        raise RuntimeError("bomb TLUT entry 0 is no longer transparent")
    if any((entry & 1) == 0 for entry in entries[1:]):
        raise RuntimeError("a bomb TLUT entry other than 0 lost its alpha bit")

    # The two LOADBLOCKs must still describe the two spans this owner bounds.
    if ((body[19][1] >> 12) & 0xFFF) + 1 != BODY_IMAGE_BYTES * 2 // 4:
        raise RuntimeError(f"bomb body LOADBLOCK changed: {body[19][1]:#010x}")
    if ((fuse[15][1] >> 12) & 0xFFF) + 1 != FUSE_IMAGE_BYTES // 2:
        raise RuntimeError(f"bomb fuse LOADBLOCK changed: {fuse[15][1]:#010x}")

    body_verts = tuple(sm.decode_vertex(bomb, BODY_VTX + i * 16) for i in range(4))
    fuse_verts = tuple(sm.decode_vertex(bomb, FUSE_VTX + i * 16) for i in range(4))
    if body_verts != BODY_VERTS:
        raise RuntimeError(f"bomb body vertices changed: {body_verts!r}")
    if fuse_verts != FUSE_VERTS:
        raise RuntimeError(f"bomb fuse vertices changed: {fuse_verts!r}")
    for label, raw, index in (("body", body, 22), ("fuse", fuse, 18)):
        tris = tuple(sm.decode_triangles(0x06, *raw[index]))
        if tris != TRIS:
            raise RuntimeError(f"bomb {label} triangles changed: {tris!r}")
    return body, fuse, body_verts, fuse_verts


def render_header(body, fuse) -> str:
    """The constants only.  The adapter admits this item in a DIFFERENT
    translation unit from the executor, so the numbers have to be in a header
    both can include -- the Sector laser / Castle bumper shape."""
    lines: list[str] = []
    a = lines.append
    a("/* Link's Bomb native item constants (generated).")
    a(" * Do not hand-edit; regenerate with generate_nds_native_link_bomb.py. */")
    a("#ifndef NDS_NATIVE_LINK_BOMB_GENERATED_H")
    a("#define NDS_NATIVE_LINK_BOMB_GENERATED_H")
    a("")
    a(f"#define NDS_NATIVE_LINK_BOMB_ASSET {ASSET}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_BODY_ROOT 0x{BODY_ROOT:04x}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_BODY_DL_BYTES {BODY_WORDS * 8}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_FUSE_ROOT 0x{FUSE_ROOT:04x}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_FUSE_DL_BYTES {FUSE_WORDS * 8}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_TLUT_OFFSET 0x{TLUT_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_BODY_IMAGE_OFFSET 0x{BODY_IMAGE_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_FUSE_IMAGE_OFFSET 0x{FUSE_IMAGE_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_FILE_END 0x{FILE_END:04x}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_BODY_TLUT_W0 0x{body[BODY_TLUT_WORD][0]:08x}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_BODY_IMAGE_W0 0x{body[BODY_IMAGE_WORD][0]:08x}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_FUSE_IMAGE_W0 0x{fuse[FUSE_IMAGE_WORD][0]:08x}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_BODY_TLUT_SLOT {BODY_TLUT_WORD * 8 + 4}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_BODY_IMAGE_SLOT {BODY_IMAGE_WORD * 8 + 4}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_FUSE_IMAGE_SLOT {FUSE_IMAGE_WORD * 8 + 4}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_VERTEX_COUNT {len(BODY_VERTS)}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_TRIANGLE_COUNT {len(TRIS)}u")
    a(f"#define NDS_NATIVE_LINK_BOMB_CORNER_COUNT {len(TRIS) * 3}u")
    a("")
    a("#endif")
    return "\n".join(lines) + "\n"


def _othermode(a, raw, index, indent="    "):
    a(f"{indent}ndsRendererRecordOtherMode(stats, 0x{raw[index][0] >> 24:02x}u,"
      f" 0x{raw[index][0]:08x}u, 0x{raw[index][1]:08x}u);")


def render(body, fuse, body_verts, fuse_verts) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Link's Bomb native item packet (generated).")
    a(" * Source: SHA-pinned file 353 roots 0x16f8 (Gfx[30]) and 0x17e8 (Gfx[26]),")
    a(" * vertex pools 0x1678 / 0x16b8, TLUT 0x1348, CI4 32x32 image 0x1478 and")
    a(" * IA8 16x16 image 0x1370 -- every one of them internal to file 353.  The")
    a(" * item has NO MObj (LinkMain ITAttributes.p_mobjsubs is NULL), so the two")
    a(" * lists own their whole material and nothing here is a segment-E hook.")
    a(" * Do not hand-edit; regenerate with generate_nds_native_link_bomb.py. */")
    a("#include <nds/generated/nds_native_link_bomb.generated.h>")
    a("")
    a(f"static const u16 sNdsNativeLinkBombTriIndices[{len(TRIS) * 3}] =")
    a("{")
    for tri in TRIS:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    for name, verts in (("Body", body_verts), ("Fuse", fuse_verts)):
        a(f"static const s16 sNdsNativeLinkBomb{name}Verts[{len(verts) * 5}] =")
        a("{")
        for v in verts:
            a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
        a("};")
        a("")
        a(f"static const u32 sNdsNativeLinkBomb{name}VertColors[{len(verts)}] =")
        a("{")
        for v in verts:
            a(f"    0x{v[5]:08x}u,")
        a("};")
        a("")
    a("/* Body words 0/7/12/14/18/20/23/24 are pipe/tile/load syncs and word 29 is")
    a(" * ENDDL: they carry no state.  Every state word is emitted in source order,")
    a(" * so the SETTIMG that names the palette still precedes the LOADTLUT that")
    a(" * latches texture_tlut_image and the image SETTIMG still follows it. */")
    a("static void ndsNativeLinkBombBodySetup(")
    a("    NDSRendererStats *stats, const void *tlut, const void *image)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{body[1][0]:08x}u) |"
      f" 0x{body[1][1]:08x}u;")
    for i in (2, 3, 4):
        _othermode(a, body, i)
    a(f"    ndsRendererRecordSetCombine(stats, 0x{body[5][0]:08x}u,"
      f" 0x{body[5][1]:08x}u);")
    a(f"    stats->blend_color = 0x{body[6][1]:08x}u;")
    for i in (8, 9, 10):
        a(f"    ndsRendererRecordSetTile(stats, 0x{body[i][0]:08x}u,"
          f" 0x{body[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{body[11][0]:08x}u,"
      "\n        (u32)(uintptr_t)tlut);")
    a(f"    ndsRendererRecordLoadTlut(stats, 0x{body[13][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{body[15][0]:08x}u,"
      f" 0x{body[15][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{body[16][0]:08x}u,"
      f" 0x{body[16][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{body[17][0]:08x}u,"
      "\n        (u32)(uintptr_t)image);")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{body[19][0]:08x}u,"
      f" 0x{body[19][1]:08x}u);")
    a("}")
    a("")
    a("static void ndsNativeLinkBombBodyFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{body[25][0]:08x}u) |"
      f" 0x{body[25][1]:08x}u;")
    for i in (26, 27, 28):
        _othermode(a, body, i)
    a("}")
    a("")
    a("/* Fuse words 0/14/16/19/20 are syncs and word 25 is ENDDL.  This list DOES")
    a(" * carry its own prim and env colours (words 6/7), so unlike the body it")
    a(" * overrides the item layer's ColAnim env rather than reading it. */")
    a("static void ndsNativeLinkBombFuseSetup(")
    a("    NDSRendererStats *stats, const void *image)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{fuse[1][0]:08x}u) |"
      f" 0x{fuse[1][1]:08x}u;")
    for i in (2, 3, 4):
        _othermode(a, fuse, i)
    a(f"    ndsRendererRecordSetCombine(stats, 0x{fuse[5][0]:08x}u,"
      f" 0x{fuse[5][1]:08x}u);")
    a(f"    stats->prim_color = 0x{fuse[6][1]:08x}u;")
    a(f"    stats->env_color = 0x{fuse[7][1]:08x}u;")
    a(f"    stats->blend_color = 0x{fuse[8][1]:08x}u;")
    for i in (9, 10):
        a(f"    ndsRendererRecordSetTile(stats, 0x{fuse[i][0]:08x}u,"
          f" 0x{fuse[i][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{fuse[11][0]:08x}u,"
      f" 0x{fuse[11][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{fuse[12][0]:08x}u,"
      f" 0x{fuse[12][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{fuse[13][0]:08x}u,"
      "\n        (u32)(uintptr_t)image);")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{fuse[15][0]:08x}u,"
      f" 0x{fuse[15][1]:08x}u);")
    a("}")
    a("")
    a("static void ndsNativeLinkBombFuseFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{fuse[21][0]:08x}u) |"
      f" 0x{fuse[21][1]:08x}u;")
    for i in (22, 23, 24):
        _othermode(a, fuse, i)
    a("}")
    a("")
    a(f"/* census: body_words={BODY_WORDS} fuse_words={FUSE_WORDS}"
      f" verts={len(BODY_VERTS)}+{len(FUSE_VERTS)} tris={len(TRIS)}+{len(TRIS)}"
      " material=none tlut=353:0x1348 body_image=353:0x1478"
      " fuse_image=353:0x1370 referrer=nITKindLinkBomb */")
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
        print(f"scanned {scanned} O2R files; pointers into asset {ASSET}:")
        for file_id, slot, offset in hits:
            print(f"  file {file_id} slot 0x{slot:04x} -> {ASSET}:0x{offset:04x}")
        return 0
    body, fuse, body_verts, fuse_verts = decode()
    text = render(body, fuse, body_verts, fuse_verts)
    header = render_header(body, fuse)
    if args.emit:
        for path, blob in ((OUT, text), (OUT_HEADER, header)):
            path.parent.mkdir(parents=True, exist_ok=True)
            if (not path.exists()) or path.read_text() != blob:
                path.write_text(blob)
        print(f"emitted {OUT.relative_to(REPO)} and {OUT_HEADER.relative_to(REPO)}")
    if args.check or not args.emit:
        for path, blob in ((OUT, text), (OUT_HEADER, header)):
            if not path.exists() or path.read_text() != blob:
                raise RuntimeError(f"generated artefact stale: {path.relative_to(REPO)}")
        print("LINK_BOMB_NATIVE_OK roots=0x16f8,0x17e8 verts=4+4 tris=2+2 "
              "material=none referrer=nITKindLinkBomb")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
