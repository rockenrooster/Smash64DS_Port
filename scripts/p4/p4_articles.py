#!/usr/bin/env python3
"""P4 S6: the native draw roots of each content's own articles.

A content with its own special files (generate_p4_fighter.py
OWN_SPECIAL_FILES) draws its effects and weapons from them. Each article
below names the source structure Remix's description points at: an
EFDesc's DObjDesc tree or display list, or a weapon's attributes. This
module resolves it, through the content's O2R files, to the display-list
roots the entry-effect generator compiles
(scripts/3d_vfx/generate_nds_entry_effects.py --p4), each with the special
file it lives in (the runtime admits a list by `*storage + offset`) and,
for a material whose MatAnim cycles TEXID, the source images it cycles.

    p4_articles.py --o2r <gen>/<name>/o2r --export <export>/<name> \
        --content <name> --out <gen>/<name>/articles.json

The roots are offsets into the user's files: build output only.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

import generate_battle_playable_texture_census as census  # noqa: E402

# Per content, in the order the runtime admits them. "special" is the slot
# (1-4) of the file the structure is read from; its roots may live in
# another of the content's files (a weapon's attributes name its graphic).
ARTICLES = {
    "wolf": (
        # Wolf.asm wolf_reflect_graphic_struct: DObjDesc 0x288.
        {"name": "reflector", "special": 2, "dobjdesc": 0x288},
        # Wolf.asm entry_anim_struct_WOLF: DObjDesc 0x2610, drawn through
        # DObjDLLinks (gcDrawDObjTreeDLLinksForGObj); entry lifetime.
        {"name": "wolfen", "special": 3, "dobjdesc": 0x2610, "dllinks": True, "entry": True},
        # captainshared.asm slash_anim_struct_WOLF: one display list with
        # the MObjSub table 0xA90 (Falcon Punch's shape).
        {"name": "slash", "special": 4, "dl": 0x8F0, "mobjsubs": 0xA90},
        # WolfSpecial.asm _blaster_projectile_struct: WPAttributes at 0.
        {"name": "blaster", "special": 1, "wpattributes": 0x0},
        # His gun: joint 17's model part 0, a list in WOLF_WEAPON (0xB47),
        # outside his model file, so it is drawn beside the body as Fox's
        # pistol is (renderer_adapter_fighter.c's gun sidecar).
        {"name": "gun", "modelpart_joint": 17, "modelpart": 0},
    ),
    "bowser": (
        # captainshared.asm entry_anim_struct_BOWSER: the Falcon Flyer's
        # description on his Clown Copter file, DObjDesc 0x1E80, drawn
        # through DObjDLLinks; entry lifetime.
        {"name": "clown_copter", "special": 2, "dobjdesc": 0x1E80, "dllinks": True,
         "entry": True},
    ),
}

MOBJ_FLAG_PALETTE = 0x0004  # segment E loads the TLUT from palettes[palette_id]
FTATTRIBUTES_MODELPARTS = 808  # FTAttributes.modelparts_container
FTPARTS_JOINT_COMMON_START = 4
FTMODELPART_SIZE = 20
DOBJ_DESC_SIZE = 0x2C
DOBJ_DESC_END = 18          # DObjDesc id terminator
DLLINK_END = 4              # DObjDLLink list_id terminator
FILE_SLOTS = {1: 5, 2: 6, 3: 7, 4: 8}  # special n -> descriptor file_ids index


class ArticleError(RuntimeError):
    pass


def load(path: Path, file_id: int) -> census.O2RResource:
    path = path.resolve()
    spec = census.InputSpec(str(path), hashlib.sha256(path.read_bytes()).hexdigest(), file_id)
    # census.load_o2r joins the path to a root; an absolute path stands.
    return census.load_o2r(ROOT, spec)


def u32(res: census.O2RResource, offset: int) -> int:
    return census.checked_u32(res.payload, offset, f"asset {res.file_id:#x}")


def dobjdesc_lists(res: census.O2RResource, start: int) -> list[int]:
    """The display-list fields of a DObjDesc array, in tree order."""
    found = []
    for i in range(256):
        at = start + i * DOBJ_DESC_SIZE
        if (u32(res, at) & 0xFFFF) == DOBJ_DESC_END:
            return found
        ref = res.pointer_at(at + 4)
        if ref is None:
            if u32(res, at + 4) != 0:
                raise ArticleError(f"{res.file_id:#x}+{at + 4:#x}: unresolved DObjDesc pointer")
            continue
        if ref.asset_id != res.file_id:
            raise ArticleError(f"{res.file_id:#x}+{at + 4:#x}: DObjDesc list in another file")
        found.append(ref.offset)
    raise ArticleError(f"{res.file_id:#x}+{start:#x}: DObjDesc array has no terminator")


def dllink_lists(res: census.O2RResource, start: int) -> list[tuple[int, int]]:
    """A DObjDLLink array's (list_id, display list) pairs up to id 4. The id
    is the display-list head the list joins (0 opaque, 1 translucent)."""
    found = []
    for i in range(16):
        at = start + i * 8
        list_id = u32(res, at)
        if list_id == DLLINK_END:
            return found
        ref = res.pointer_at(at + 4)
        if ref is None or ref.asset_id != res.file_id:
            raise ArticleError(f"{res.file_id:#x}+{at + 4:#x}: DObjDLLink list not in its file")
        found.append((list_id, ref.offset))
    raise ArticleError(f"{res.file_id:#x}+{start:#x}: DObjDLLink array has no terminator")


def modelpart_list(main: census.O2RResource, o_attributes: int, joint: int,
                   part: int) -> census.PointerRef:
    """A fighter model part's display list: the attributes' model-part
    container (FTAttributes + 808), the joint's FTModelPartDesc, and its
    part's high-detail FTModelPart (20 bytes each, [part][detail])."""
    container = main.pointer_at(o_attributes + FTATTRIBUTES_MODELPARTS)
    if container is None or container.asset_id != main.file_id:
        raise ArticleError(f"{main.file_id:#x}: no model-part container")
    desc = main.pointer_at(container.offset + (joint - FTPARTS_JOINT_COMMON_START) * 4)
    if desc is None:
        raise ArticleError(f"{main.file_id:#x}: joint {joint} has no model parts")
    owner = main if desc.asset_id == main.file_id else None
    if owner is None:
        raise ArticleError(f"{main.file_id:#x}: joint {joint}'s parts are in another file")
    dl = main.pointer_at(desc.offset + part * 2 * FTMODELPART_SIZE)
    if dl is None:
        raise ArticleError(f"{main.file_id:#x}: joint {joint} part {part} has no display list")
    return dl


def texid_images(res: census.O2RResource, table: int) -> dict:
    """A one-DObj MObjSub*** table whose one MObjSub's sprites MatAnim cycles:
    its images (the consecutive non-NULL sprites) and their format."""
    # Only the first DObj's list is read: these articles are one DObj, and
    # the word after the table is often other data (the blaster's palette
    # array).
    lst = res.pointer_at(table)
    if lst is None or lst.asset_id != res.file_id:
        raise ArticleError(f"{res.file_id:#x}+{table:#x}: MObjSub table did not resolve")
    sub = res.pointer_at(lst.offset)
    if sub is None or sub.asset_id != res.file_id or res.pointer_at(lst.offset + 4) is not None:
        raise ArticleError(f"{res.file_id:#x}+{lst.offset:#x}: not a single-MObjSub list")
    # MObjSub (sys/objtypes.h): the segment-E image's fmt/siz at +2/+3, the
    # render tile's size at +0x0C/+0x0E (flag 0x20), palettes at +0x2C,
    # flags at +0x30, the texel block's format and size at +0x32/+0x33.
    fmt, siz = res.payload[sub.offset + 2], res.payload[sub.offset + 3]
    width, height = struct.unpack_from(">HH", res.payload, sub.offset + 0x0C)
    flags = struct.unpack_from(">H", res.payload, sub.offset + 0x30)[0]
    block_fmt, block_siz = res.payload[sub.offset + 0x32], res.payload[sub.offset + 0x33]
    palette = None
    if flags & MOBJ_FLAG_PALETTE:
        palettes = res.pointer_at(sub.offset + 0x2C)
        first = res.pointer_at(palettes.offset) if palettes is not None else None
        if first is None or first.asset_id != res.file_id:
            raise ArticleError(f"{res.file_id:#x}+{sub.offset:#x}: palette 0 did not resolve")
        palette = first.offset
    sprites = res.pointer_at(sub.offset + 4)
    if sprites is None or sprites.asset_id != res.file_id:
        raise ArticleError(f"{res.file_id:#x}+{sub.offset:#x}: MObjSub sprites did not resolve")
    images = []
    for i in range(8):
        image = res.pointer_at(sprites.offset + i * 4)
        if image is None:
            break
        if image.asset_id != res.file_id:
            raise ArticleError(f"{res.file_id:#x}: TEXID {i} image in another file")
        images.append(image.offset)
    if not images:
        raise ArticleError(f"{res.file_id:#x}+{sub.offset:#x}: MObjSub has no sprites")
    return {"mobjsub_table": table, "mobjsub": sub.offset, "flags": flags,
            "images": images, "fmt": fmt, "siz": siz, "width": width, "height": height,
            "block_fmt": block_fmt, "block_siz": block_siz, "palette": palette}


def resolve(content: str, title: str, o2r: Path, descriptor: dict) -> list[dict]:
    file_ids = list(descriptor["file_ids"])
    resources: dict[int, census.O2RResource] = {}

    def res_of(fid: int) -> census.O2RResource:
        if fid not in resources:
            resources[fid] = load(o2r / f"{fid:04x}", fid)
        return resources[fid]

    storage = {file_ids[FILE_SLOTS[n]]: f"gNdsP4{title}Special{n}" for n in FILE_SLOTS}
    roots = []

    def add(article: dict, res: census.O2RResource, offset: int, texid: dict | None,
            head: int = 0) -> None:
        sidecar = article.get("modelpart_joint")
        if sidecar is None and res.file_id not in storage:
            raise ArticleError(f"{content} {article['name']}: root in {res.file_id:#x}, "
                               "not one of its special files")
        roots.append({"article": article["name"], "file_id": res.file_id,
                      "storage": None if sidecar is not None else storage[res.file_id],
                      "offset": offset, "entry": bool(article.get("entry")),
                      "texid": texid, "head": head, "sidecar_joint": sidecar})

    for article in ARTICLES.get(content, ()):
        if "modelpart_joint" in article:
            main = res_of(file_ids[0])
            dl = modelpart_list(main, descriptor["o_attributes"],
                                article["modelpart_joint"], article["modelpart"])
            add(article, res_of(dl.asset_id), dl.offset, None)
            continue
        res = res_of(file_ids[FILE_SLOTS[article["special"]]])
        if "dobjdesc" in article:
            for dl in dobjdesc_lists(res, article["dobjdesc"]):
                lists = dllink_lists(res, dl) if article.get("dllinks") else [(0, dl)]
                for head, root in lists:
                    add(article, res, root, None, head)
        elif "dl" in article:
            texid = texid_images(res, article["mobjsubs"]) if "mobjsubs" in article else None
            add(article, res, article["dl"], texid)
        elif "wpattributes" in article:
            at = article["wpattributes"]
            data = res.pointer_at(at)
            if data is None:
                raise ArticleError(f"{content} {article['name']}: attributes name no display list")
            target = res_of(data.asset_id)
            texid = None
            mobjsubs = res.pointer_at(at + 4)
            if mobjsubs is not None:
                if mobjsubs.asset_id != data.asset_id:
                    raise ArticleError(f"{content} {article['name']}: materials in another file")
                texid = texid_images(target, mobjsubs.offset)
            add(article, target, data.offset, texid)
        else:
            raise ArticleError(f"{content} {article['name']}: no source structure")
    return roots


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--o2r", type=Path, required=True)
    ap.add_argument("--export", type=Path, required=True)
    ap.add_argument("--content", required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    resolved = json.loads((args.export / "resolved.json").read_text(encoding="utf-8"))
    titles = {row["name"]: row["title"] for row in json.loads(
        Path(__file__).with_name("contents.json").read_text(encoding="utf-8"))["contents"]}
    roots = resolve(args.content, titles[args.content], args.o2r.resolve(),
                    resolved["descriptor"])
    args.out.write_text(json.dumps({"content": args.content, "roots": roots}, indent=1),
                        encoding="utf-8", newline="\n")
    print(f"{args.content}: {len(roots)} article roots")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
