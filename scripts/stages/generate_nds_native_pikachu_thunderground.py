#!/usr/bin/env python3
"""Generate/check Pikachu's GROUND Thunder Jolt: six one-triangle segments.

WHAT THIS OBJECT IS.  `llPikachuSpecial1ThunderJoltGroundWeaponAttributes` is
0x34 (reloc_data.us.h:3726), and that slot's `data` field resolves to file 342
(PikachuSpecial3) DObjDesc 0x1888.  So this is the GROUND Thunder Jolt, sibling
of the AIR variant at root 0x0270 whose attributes sit at 0x0 in the same file;
`nWPKindThunderJoltAir` and `nWPKindThunderJoltGround` are adjacent at
`wp/wpdef.h:52-53`.

SIX LISTS, ONE WEAPON.  DObjDesc 0x1888 holds two non-drawable entries and then
six drawable children, all id 2, whose DObjDLLinks at 0x1828, 0x1838, 0x1848,
0x1858, 0x1868 and 0x1878 resolve to the six roots below.  Each root draws
exactly ONE triangle and carries exactly ONE segment-0xE call, and
`p_mobjsubs` at 342:0x1018 is NULL in heads 0 and 1 -- matching the two
non-drawable DObjs -- with six MObjSub lists for the six children.  It is the
segmented ground spark.

ALL SIX ARE NEEDED, MEASURED.  A reconnaissance witness read a root mask of
0x3f on a Pikachu mirror at 1,200 presents, so every one of the six is walked.
The failure record latched the FOURTH child rather than the first only because
`ndsRendererAdapterSubmitStageDObjTreeDepth` skips a `DOBJ_FLAG_HIDDEN` child
AND its subtree silently while still walking siblings, so the visible subset
varies frame to frame -- which is also why the reject count is not a multiple
of six.

THE LIVE HALF IS THE CURRENT IMAGE, MEASURED.  The same witness read a material
effects word of 0x200 across all six roots, which is
`NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE` alone.  `wppikachuthunder.c:172`
and `:196` write `mobj->texture_id_curr`, once to a fixed 3 and once to
`syUtilsRandIntRange(WPPIKACHUTHUNDER_TEXTURES_NUM - 1)`.  So this is exactly
the Mushroom Kingdom Pakkun contract: the image is chosen live through segment
0xE and everything else -- combiner, prim, env, both tiles, texture state, tile
size, geometry mode -- is immutable and bakes here.

ROOT 0x15C0 IS THE ODD ONE and the generator must not paper over it: it is 20
words, not 19, because it issues TWO G_VTX commands (one vertex into cache slot
0, then two ending at slot 3) before its single triangle.  Its corners are
therefore drawn from two separate source pools.  The emitter flattens each
root's three corners in draw order, so the cache-slot arithmetic is resolved
here once rather than reproduced at runtime.
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

OUT = REPO / "src/nds/generated/nds_native_pikachu_thunderground.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_pikachu_thunderground.generated.h"

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
     ("#define llPikachuSpecial1ThunderJoltGroundWeaponAttributes ((intptr_t)0x34)",)),
    ("decomp/BattleShip-main/decomp/src/wp/wppikachu/wppikachuthunder.c",
     ("DObjGetStruct(weapon_gobj)->mobj->texture_id_curr = 3;",)),
)

ASSET = 342
ATTR_ASSET = 244
ATTR_OFFSET = 0x0034          # llPikachuSpecial1ThunderJoltGroundWeaponAttributes
DOBJDESC = 0x1888
MOBJSUBS = 0x1018
ANIMJOINT = 0x1A20

ROOTS = (0x1490, 0x1528, 0x15C0, 0x1660, 0x16F8, 0x1790)
DLLINKS = (0x1828, 0x1838, 0x1848, 0x1858, 0x1868, 0x1878)
# Root 0x15C0 issues two G_VTX commands, so it is one word longer.
ROOT_WORDS = (19, 19, 20, 19, 19, 19)

# Shared by every root: the state words sit at the same indices in all six, and
# only the two-VTX root shifts its geometry tail by one.
COMBINE_WORD = 1
PRIM_WORD = 2
ENV_WORD = 3
TILE_WORDS = (4, 5)
TEXTURE_WORD = 6
TILESIZE_WORD = 7
SEGMENT_WORD = 8
LOADBLOCK_WORD = 10
GEOM_WORD = 12
GEOM_RESTORE_WORD_OFFSET = -2   # from ENDDL

EXPECTED_ATTR_REFS = (
    (0x0000, ASSET, 0x0270), (0x0008, ASSET, 0x0360),
    (0x0034, ASSET, DOBJDESC), (0x0038, ASSET, MOBJSUBS),
    (0x003C, ASSET, ANIMJOINT), (0x0040, ASSET, 0x1AE0),
)


def check_text_pins() -> None:
    for path, tokens in TEXT_PINS:
        text = (REPO / path).read_text(encoding="utf-8", errors="replace")
        for token in tokens:
            if token not in text:
                raise RuntimeError(f"{path} pin missing {token!r}")


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def census() -> tuple:
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


def decode_root(model, root: int, expect_words: int):
    """Return (raw words, three corner vertices in DRAW order).

    The corners are flattened out of the source's vertex cache here: a root may
    load its three corners with one G_VTX or with two, and the triangle indexes
    cache SLOTS, not pool positions.
    """
    raw = words_at(model.payload, root, expect_words)
    if (raw[-1][0] >> 24) != 0xDF:
        raise RuntimeError(f"root {root:#06x} does not end in G_ENDDL")
    if (raw[SEGMENT_WORD][0] >> 24) != 0xDE:
        raise RuntimeError(f"root {root:#06x} lost its segment-E material call")
    if raw[SEGMENT_WORD][1] != 0x0E000000:
        raise RuntimeError(
            f"root {root:#06x} segment-E target changed: {raw[SEGMENT_WORD][1]:#010x}")

    slots: dict[int, tuple] = {}
    tri = None
    for index, (w0, w1) in enumerate(raw):
        op = w0 >> 24
        if op == 0x01:
            count = (w0 >> 12) & 0xFF
            end = (w0 >> 1) & 0x7F
            ref = model.pointer_at(root + index * 8 + 4)
            if ref is None or ref.asset_id != ASSET:
                raise RuntimeError(f"root {root:#06x} vertex ref changed: {ref!r}")
            first = end - count
            for k in range(count):
                slots[first + k] = sm.decode_vertex(model, ref.offset + k * 16)
        elif op == 0x05:
            got = tuple(sm.decode_triangles(0x05, w0, w1))
            if len(got) != 1:
                raise RuntimeError(f"root {root:#06x} is no longer one triangle")
            tri = got[0]
        elif op == 0x06:
            raise RuntimeError(f"root {root:#06x} gained a G_TRI2")
    if tri is None:
        raise RuntimeError(f"root {root:#06x} has no triangle")
    corners = []
    for slot in tri:
        if slot not in slots:
            raise RuntimeError(
                f"root {root:#06x} triangle uses unloaded cache slot {slot}")
        corners.append(slots[slot])
    return raw, tuple(corners)


def decode(run_census: bool = True):
    model = sm.load_o2r(REPO, MODEL_FILE)
    attr = sm.load_o2r(REPO, ATTR_FILE)
    check_text_pins()

    # WPAttributes at PikachuSpecial1 + 0x34.  p_mobjsubs is NOT null here --
    # unlike the air jolt -- which is exactly why this owner takes a live
    # material and the air one does not.
    data_ref = attr.pointer_at(ATTR_OFFSET)
    if data_ref is None or (data_ref.asset_id, data_ref.offset) != (ASSET, DOBJDESC):
        raise RuntimeError(f"ground jolt WPAttributes.data changed: {data_ref!r}")
    mobj_ref = attr.pointer_at(ATTR_OFFSET + 4)
    if mobj_ref is None or (mobj_ref.asset_id, mobj_ref.offset) != (ASSET, MOBJSUBS):
        raise RuntimeError(f"ground jolt p_mobjsubs changed: {mobj_ref!r}")
    attr_refs = tuple(sorted(
        (slot, ref.asset_id, ref.offset)
        for slot, ref in attr.external.items() if ref.asset_id == ASSET))
    if attr_refs != EXPECTED_ATTR_REFS:
        raise RuntimeError(f"file 244 -> 342 census changed: {attr_refs!r}")

    # The DObjDesc: two non-drawable entries, six drawable children all id 2,
    # then the id-18 terminator.
    for index in (0, 1):
        entry = DOBJDESC + index * 0x2C
        if struct.unpack_from(">i", model.payload, entry)[0] != index:
            raise RuntimeError(f"ground jolt DObjDesc entry {index} id changed")
        if model.pointer_at(entry + 4) is not None:
            raise RuntimeError(f"ground jolt DObjDesc entry {index} gained a list")
    for child, (link, root) in enumerate(zip(DLLINKS, ROOTS)):
        entry = DOBJDESC + (child + 2) * 0x2C
        if struct.unpack_from(">i", model.payload, entry)[0] != 2:
            raise RuntimeError(f"ground jolt child {child} is no longer id 2")
        ref = model.pointer_at(entry + 4)
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, link):
            raise RuntimeError(f"ground jolt child {child} dl link changed: {ref!r}")
        ref = model.pointer_at(link + 4)
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, root):
            raise RuntimeError(f"ground jolt child {child} root changed: {ref!r}")
    if struct.unpack_from(">i", model.payload, DOBJDESC + 8 * 0x2C)[0] != 18:
        raise RuntimeError("ground jolt DObjDesc terminator changed")

    # Six MObjSub heads for six drawable children; heads 0 and 1 are NULL
    # because the first two DObjs draw nothing.
    for head in (0, 1):
        if model.pointer_at(MOBJSUBS + head * 4) is not None:
            raise RuntimeError(f"ground jolt MObjSub head {head} is no longer NULL")
    for head in range(2, 8):
        if model.pointer_at(MOBJSUBS + head * 4) is None:
            raise RuntimeError(f"ground jolt MObjSub head {head} went NULL")

    if run_census:
        scanned, hits = census()
        want = tuple((ATTR_ASSET, slot, off) for slot, _, off in EXPECTED_ATTR_REFS)
        if hits != want:
            raise RuntimeError(
                f"whole-image census into asset {ASSET} changed ({scanned} files): {hits!r}")

    decoded = []
    for root, expect in zip(ROOTS, ROOT_WORDS):
        raw, corners = decode_root(model, root, expect)
        decoded.append((root, raw, corners))

    # Every root shares one immutable material, so a divergence would silently
    # give one segment a different look; pin it rather than emit six copies of
    # a value nobody compared.
    first = decoded[0][1]
    for root, raw, _ in decoded[1:]:
        for name, index in (("combine", COMBINE_WORD), ("prim", PRIM_WORD),
                            ("env", ENV_WORD), ("texture", TEXTURE_WORD),
                            ("tilesize", TILESIZE_WORD),
                            ("tile0", TILE_WORDS[0]), ("tile1", TILE_WORDS[1]),
                            ("loadblock", LOADBLOCK_WORD), ("geom", GEOM_WORD)):
            if raw[index] != first[index]:
                raise RuntimeError(
                    f"root {root:#06x} {name} word diverges from 0x1490: "
                    f"{raw[index]!r} != {first[index]!r}")
    return decoded


def render_header(decoded) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Pikachu ground Thunder Jolt native weapon constants (generated).")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_pikachu_thunderground.py. */")
    a("#ifndef NDS_NATIVE_PIKACHU_THUNDERGROUND_GENERATED_H")
    a("#define NDS_NATIVE_PIKACHU_THUNDERGROUND_GENERATED_H")
    a("")
    a(f"#define NDS_NATIVE_THUNDERGROUND_ASSET {ASSET}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_ROOT_COUNT {len(ROOTS)}u")
    a("#define NDS_NATIVE_THUNDERGROUND_CORNER_COUNT 3u")
    for index, (root, raw, _) in enumerate(decoded):
        a(f"#define NDS_NATIVE_THUNDERGROUND_ROOT{index} 0x{root:04x}u")
        a(f"#define NDS_NATIVE_THUNDERGROUND_ROOT{index}_BYTES {len(raw) * 8}u")
    a("")
    a("#endif")
    return "\n".join(lines) + "\n"


def render(decoded) -> str:
    first = decoded[0][1]
    lines: list[str] = []
    a = lines.append
    a("/* Pikachu ground Thunder Jolt native weapon packet (generated).")
    a(" * Six one-triangle segments from SHA-pinned file 342, reached from")
    a(" * PikachuSpecial1's WPAttributes at 0x34.  Each root carries one")
    a(" * segment-0xE call and the measured live material is CURRENT_IMAGE only")
    a(" * (mobj->texture_id_curr), so the image is chosen at runtime and every")
    a(" * other word here is immutable.  Corners are flattened out of the")
    a(" * source vertex cache in DRAW order, which is why root 0x15c0's two")
    a(" * G_VTX loads need no special case at runtime.")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_pikachu_thunderground.py. */")
    a("#include <nds/generated/nds_native_pikachu_thunderground.generated.h>")
    a("")
    a(f"static const s16 sNdsNativeThunderGroundVerts[{len(decoded) * 3 * 5}] =")
    a("{")
    for root, _, corners in decoded:
        a(f"    /* 0x{root:04x} */")
        for v in corners:
            a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeThunderGroundVertColors[{len(decoded) * 3}] =")
    a("{")
    for _, _, corners in decoded:
        for v in corners:
            a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a("/* One immutable material for all six segments; the generator refuses to")
    a(" * emit if any root's combine, prim, env, tile, texture, tile-size,")
    a(" * load-block or geometry word diverges from the first. */")
    a("static void ndsNativeThunderGroundSetup(NDSRendererStats *stats)")
    a("{")
    a(f"    ndsRendererRecordSetCombine(stats, 0x{first[COMBINE_WORD][0]:08x}u,"
      f" 0x{first[COMBINE_WORD][1]:08x}u);")
    a(f"    stats->prim_color = 0x{first[PRIM_WORD][1]:08x}u;")
    a(f"    stats->env_color = 0x{first[ENV_WORD][1]:08x}u;")
    for index in TILE_WORDS:
        a(f"    ndsRendererRecordSetTile(stats, 0x{first[index][0]:08x}u,"
          f" 0x{first[index][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{first[TEXTURE_WORD][0]:08x}u,"
      f" 0x{first[TEXTURE_WORD][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{first[TILESIZE_WORD][0]:08x}u,"
      f" 0x{first[TILESIZE_WORD][1]:08x}u);")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{first[LOADBLOCK_WORD][0]:08x}u,"
      f" 0x{first[LOADBLOCK_WORD][1]:08x}u);")
    a(f"    stats->geometry_mode = (stats->geometry_mode &"
      f" 0x{first[GEOM_WORD][0]:08x}u) | 0x{first[GEOM_WORD][1]:08x}u;")
    a("}")
    a("")
    a("static void ndsNativeThunderGroundFinish(NDSRendererStats *stats)")
    a("{")
    restore = first[len(first) + GEOM_RESTORE_WORD_OFFSET]
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{restore[0]:08x}u) |"
      f" 0x{restore[1]:08x}u;")
    a("}")
    a("")
    a(f"/* census: roots={len(ROOTS)} tris={len(ROOTS)} corners_each=3"
      " material=CURRENT_IMAGE referrer=244:0x0034 */")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--census", action="store_true")
    args = ap.parse_args()
    if args.census:
        scanned, hits = census()
        print(f"scanned {scanned} O2R files")
        for file_id, slot, off in hits:
            print(f"  file {file_id} slot 0x{slot:04x} -> {ASSET}:0x{off:04x}")
        return 0
    decoded = decode()
    text = render(decoded)
    header = render_header(decoded)
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
        print("PIKACHU_THUNDERGROUND_NATIVE_OK roots=6 tris=6 "
              "material=CURRENT_IMAGE referrer=244:0x0034")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
