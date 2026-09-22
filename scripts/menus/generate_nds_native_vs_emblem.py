#!/usr/bin/env python3
"""Emit the VS Results / Characters winner-emblem series-root descriptor table.

R01-B.  `mnVSResultsMakeEmblem` (decomp mn/mnvsmode/mnvsresults.c:615) and
`mnCharactersMakeEmblem` (decomp mn/mndata/mncharacters.c:1496) both build a
winner-selected DObj tree out of the SAME `FTEmblemModels` reloc file.  Twelve
fighter kinds index those tables, but two pairs share a series entry --
Mario/Luigi both take the Mario row and Pikachu/Purin both take the PMonsters
row -- so the file holds exactly TEN distinct roots, not twelve and not eleven.

This generator reads those ten roots out of the source tables rather than
letting anyone hand-author them:

  * `decomp/.../relocData/35_FTEmblemModels.c` carries, in the block comments
    the decomp's own exporter wrote, each series' Vtx offset + vertex count,
    DisplayList offset + byte count and DObjDesc offset + entry count.
  * `include/reloc_data.h` carries the port's registered reloc symbol offsets
    for the same objects.  The two are cross-checked: a DObjDesc offset that
    disagrees between them is a hard error, because the runtime recognises a
    root by the offset the loader produced, not by the one the decomp printed.

The emitted header is recognition data only -- asset id, per-series root
offsets and counts, and the twelve-entry fighter-kind -> series map.  It bakes
no geometry: the native path reads the source Vtx/Gfx out of the loaded reloc
file, exactly as the stage and item owners do, so the emblem's geometry,
ordered bindings and materials stay the source's own bytes.

Usage:
    python scripts/menus/generate_nds_native_vs_emblem.py
    python scripts/menus/generate_nds_native_vs_emblem.py --check
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
RELOC_DATA_C = (REPO / "decomp" / "BattleShip-main" / "decomp" / "src" /
                "relocData" / "35_FTEmblemModels.c")
RELOC_DATA_H = REPO / "include" / "reloc_data.h"
OUT_H = (REPO / "include" / "nds" / "generated" /
         "nds_native_vs_emblem.generated.h")

# NDS_RELOC_ASSET_FT_EMBLEM_MODELS, src/port/reloc_backend_assets.c:158.
# Staged into the ROM by the Makefile's NDS_VS_RESULTS_RELOC_FILES block
# (reloc_fighters_common/FTEmblemModels), so the file is resident whenever the
# Results scene runs.
ASSET_ID = 0x23

# THE SOURCE SELECTION, copied structurally -- not the geometry, the INDEXING.
# mnvsresults.c:622-648 and mncharacters.c:1500-1526 hold three parallel
# twelve-entry tables (DObjDesc / MObjSub / MatAnimJoint) and both spell the
# same series in the same order.  Index is FTKind.
FKIND_SERIES = [
    "Mario",      # 0  Mario
    "Fox",        # 1  Fox
    "Donkey",     # 2  Donkey Kong
    "Metroid",    # 3  Samus
    "Mario",      # 4  Luigi      -- SHARED with Mario
    "Zelda",      # 5  Link
    "Yoshi",      # 6  Yoshi
    "FZero",      # 7  Captain Falcon
    "Kirby",      # 8  Kirby
    "PMonsters",  # 9  Pikachu
    "PMonsters",  # 10 Jigglypuff -- SHARED with Pikachu
    "Mother",     # 11 Ness
]

# mnvsresults.c:649 `s32 colors[] = { 0, 1, 3 };` -- the team -> colour-index
# map handed to gcAddMatAnimJointAll.  mnVSResultsMakeWallpaper repeats the
# same three values at :716 for its own winner tint.
TEAM_COLOR_INDEX = [0, 1, 3]

VTX_RE = re.compile(
    r"/\*\s*Vtx:\s*(?P<name>\w+)\s*@\s*0x(?P<off>[0-9A-Fa-f]+)\s*"
    r"\((?P<count>\d+)\s+vertices\)\s*\*/")
DL_RE = re.compile(
    r"/\*\s*DisplayList:\s*(?P<name>\w+)\s*@\s*0x(?P<off>[0-9A-Fa-f]+)\s*"
    r"\((?P<bytes>\d+)\s+bytes\)\s*\*/")
DESC_RE = re.compile(
    r"/\*\s*DObjDesc:\s*(?P<name>\w+)\s*@\s*0x(?P<off>[0-9A-Fa-f]+)\s*"
    r"\((?P<entries>\d+)\s+entries\)\s*\*/")
DESC_ARRAY_RE = re.compile(
    r"^DObjDesc\s+dFTEmblemModels_(?P<name>\w+)\[\]\s*=\s*\{(?P<body>.*?)^\};",
    re.S | re.M)
DESC_ENTRY_RE = re.compile(r"\{\s*(?P<id>\d+)\s*,\s*(?P<dl>[^,]+),")
RELOC_H_RE = re.compile(
    r"X\(NDS_RELOC_ASSET_FT_EMBLEM_MODELS,\s*"
    r"llFTEmblemModels(?P<name>\w+?)(?P<kind>MObjSub|DObjDesc|MatAnimJoint),\s*"
    r"0x(?P<off>[0-9A-Fa-f]+)u\)")

# gcSetupCommonDObjs (decomp sys/objanim.c:2153) terminates on
# `dobjdesc->id == DOBJ_ARRAY_MAX`, and DOBJ_ARRAY_MAX is 18
# (decomp sys/objtypes.h:38).
DOBJ_ARRAY_MAX = 18


class SourceError(RuntimeError):
    pass


def parse_reloc_data_c(text: str) -> dict[str, dict[str, int]]:
    """Pull each series' geometry offsets out of the exporter's own comments."""
    series: dict[str, dict[str, int]] = {}

    def slot(name: str) -> dict[str, int]:
        return series.setdefault(name, {})

    for m in VTX_RE.finditer(text):
        s = slot(m.group("name"))
        s["vtx_offset"] = int(m.group("off"), 16)
        s["vertex_count"] = int(m.group("count"))
    for m in DL_RE.finditer(text):
        s = slot(m.group("name"))
        s["dl_offset"] = int(m.group("off"), 16)
        s["dl_bytes"] = int(m.group("bytes"))
    for m in DESC_RE.finditer(text):
        s = slot(m.group("name"))
        s["desc_offset"] = int(m.group("off"), 16)
        s["desc_entries"] = int(m.group("entries"))

    # The tree shape is a contract the runtime owner depends on: the recogniser
    # walks root -> child and submits the child's display list.  Read it rather
    # than assume it.
    for m in DESC_ARRAY_RE.finditer(text):
        name = m.group("name")
        entries = DESC_ENTRY_RE.findall(m.group("body"))
        if not entries:
            raise SourceError(f"{name}: DObjDesc array parsed with no entries")
        ids = [int(e[0]) for e in entries]
        dls = [e[1].strip() for e in entries]
        if ids[-1] != DOBJ_ARRAY_MAX:
            raise SourceError(
                f"{name}: DObjDesc does not terminate on id {DOBJ_ARRAY_MAX} "
                f"(got {ids[-1]}); gcSetupCommonDObjs would run off the array")
        body_ids = ids[:-1]
        if body_ids != [0, 1]:
            raise SourceError(
                f"{name}: expected the source's two-DObj root/child tree "
                f"(ids [0, 1]), got {body_ids}.  The runtime owner submits the "
                f"child's display list; a deeper tree needs the owner updated "
                f"in the same change.")
        if dls[0] != "(void*)0x00000000":
            raise SourceError(
                f"{name}: root DObjDesc carries a display list ({dls[0]}); "
                f"the source root is geometry-free and only positions the tree")
        expect_child_dl = f"(void*)dFTEmblemModels_{name}_DisplayList"
        if dls[1] != expect_child_dl:
            raise SourceError(
                f"{name}: child DObjDesc display list is {dls[1]}, "
                f"expected {expect_child_dl}")
        slot(name)["dobj_count"] = len(body_ids)

    return series


def parse_reloc_data_h(text: str) -> dict[str, dict[str, int]]:
    """Pull the port's registered symbol offsets for the same objects."""
    registered: dict[str, dict[str, int]] = {}
    for m in RELOC_H_RE.finditer(text):
        registered.setdefault(m.group("name"), {})[m.group("kind")] = int(
            m.group("off"), 16)
    return registered


def collect() -> list[dict[str, object]]:
    if not RELOC_DATA_C.is_file():
        raise SourceError(
            f"missing source table {RELOC_DATA_C}.  decomp/ is fetched by "
            f"scripts/fetch-battleship-reference.ps1 and is read-only "
            f"reference data; this generator only reads it.")
    series = parse_reloc_data_c(RELOC_DATA_C.read_text(encoding="utf-8"))
    registered = parse_reloc_data_h(RELOC_DATA_H.read_text(encoding="utf-8"))

    wanted = []
    for name in FKIND_SERIES:
        if name not in wanted:
            wanted.append(name)

    missing = [n for n in wanted if n not in series]
    if missing:
        raise SourceError(
            f"series named by the source selection tables but absent from "
            f"{RELOC_DATA_C.name}: {', '.join(missing)}")

    rows: list[dict[str, object]] = []
    for name in wanted:
        s = series[name]
        for field in ("vtx_offset", "vertex_count", "dl_offset", "dl_bytes",
                      "desc_offset", "desc_entries", "dobj_count"):
            if field not in s:
                raise SourceError(f"{name}: source table has no {field}")
        reg = registered.get(name)
        if reg is None:
            raise SourceError(
                f"{name}: no NDS_RELOC_ASSET_FT_EMBLEM_MODELS symbols "
                f"registered in {RELOC_DATA_H.name}; the loader cannot "
                f"resolve this root at runtime")
        for kind in ("MObjSub", "DObjDesc", "MatAnimJoint"):
            if kind not in reg:
                raise SourceError(
                    f"{name}: {kind} is not registered in "
                    f"{RELOC_DATA_H.name}; mnVSResultsMakeEmblem dereferences "
                    f"all three")
        if reg["DObjDesc"] != s["desc_offset"]:
            raise SourceError(
                f"{name}: DObjDesc offset disagrees -- source table says "
                f"0x{s['desc_offset']:04x}, {RELOC_DATA_H.name} registers "
                f"0x{reg['DObjDesc']:04x}.  The runtime recognises a root by "
                f"the registered offset, so these must not drift.")
        rows.append({
            "name": name,
            "vtx_offset": s["vtx_offset"],
            "vertex_count": s["vertex_count"],
            "dl_offset": s["dl_offset"],
            "dl_bytes": s["dl_bytes"],
            "desc_offset": s["desc_offset"],
            "dobj_count": s["dobj_count"],
            "mobjsub_offset": reg["MObjSub"],
            "matanim_offset": reg["MatAnimJoint"],
        })
    return rows


def render(rows: list[dict[str, object]]) -> str:
    n = len(rows)
    index_of = {row["name"]: i for i, row in enumerate(rows)}
    out: list[str] = []
    a = out.append
    a("/* VS Results / Characters winner-emblem series roots (generated).")
    a(" * Do not hand-edit; regenerate with")
    a(" * scripts/menus/generate_nds_native_vs_emblem.py.")
    a(" *")
    a(" * Source tables: mnvsresults.c:622-648 (mnVSResultsMakeEmblem) and")
    a(" * mncharacters.c:1500-1526 (mnCharactersMakeEmblem) select one of these")
    a(" * roots per fighter kind; relocData/35_FTEmblemModels.c holds them.")
    a(" * Twelve kinds, TEN distinct roots: Mario/Luigi share the Mario entry")
    a(" * and Pikachu/Purin share the PMonsters entry.")
    a(" *")
    a(" * Recognition data only -- no geometry is baked here.  The native owner")
    a(" * reads the source Vtx/Gfx out of the loaded reloc file. */")
    a("#ifndef NDS_NATIVE_VS_EMBLEM_GENERATED_H")
    a("#define NDS_NATIVE_VS_EMBLEM_GENERATED_H")
    a("")
    a(f"#define NDS_NATIVE_VS_EMBLEM_ASSET 0x{ASSET_ID:02x}u")
    a(f"#define NDS_NATIVE_VS_EMBLEM_SERIES_COUNT {n}u")
    a(f"#define NDS_NATIVE_VS_EMBLEM_FKIND_COUNT {len(FKIND_SERIES)}u")
    a("/* gcSetupCommonDObjs builds this many DObjs per emblem tree: an")
    a(" * origin-free root that positions the series and one child that owns")
    a(" * the display list. */")
    a(f"#define NDS_NATIVE_VS_EMBLEM_DOBJ_COUNT {rows[0]['dobj_count']}u")
    a("/* mnVSResultsMakeEmblem:662,665 -- GObj id and Results DL link. */")
    a("#define NDS_NATIVE_VS_EMBLEM_RESULTS_GOBJ_ID 23u")
    a("#define NDS_NATIVE_VS_EMBLEM_RESULTS_DL_LINK 33u")
    a("/* mnCharactersMakeEmblem:1529,1531 -- the second consumer of these")
    a(" * same ten roots.  Different scene, different GObj id and link; the")
    a(" * root table below is shared and must stay scene-independent. */")
    a("#define NDS_NATIVE_VS_EMBLEM_CHARACTERS_GOBJ_ID 19u")
    a("#define NDS_NATIVE_VS_EMBLEM_CHARACTERS_DL_LINK 28u")
    a("")
    a("/* mnvsresults.c:649 `s32 colors[] = { 0, 1, 3 };` -- winning team to")
    a(" * material-animation colour index. */")
    a("#define NDS_NATIVE_VS_EMBLEM_TEAM_COLOR_COUNT "
      f"{len(TEAM_COLOR_INDEX)}u")
    a("#define NDS_NATIVE_VS_EMBLEM_TEAM_COLORS { " +
      ", ".join(f"{c}u" for c in TEAM_COLOR_INDEX) + " }")
    a("")
    for i, row in enumerate(rows):
        u = row["name"].upper()
        a(f"/* [{i}] {row['name']}: {row['vertex_count']} vertices, "
          f"{row['dl_bytes']} display-list bytes. */")
        a(f"#define NDS_NATIVE_VS_EMBLEM_{u}_INDEX {i}u")
        a(f"#define NDS_NATIVE_VS_EMBLEM_{u}_ROOT 0x{row['dl_offset']:04x}u")
        a(f"#define NDS_NATIVE_VS_EMBLEM_{u}_DL_BYTES {row['dl_bytes']}u")
        a(f"#define NDS_NATIVE_VS_EMBLEM_{u}_VTX_OFFSET "
          f"0x{row['vtx_offset']:04x}u")
        a(f"#define NDS_NATIVE_VS_EMBLEM_{u}_VERTEX_COUNT "
          f"{row['vertex_count']}u")
        a(f"#define NDS_NATIVE_VS_EMBLEM_{u}_DESC_OFFSET "
          f"0x{row['desc_offset']:04x}u")
        a(f"#define NDS_NATIVE_VS_EMBLEM_{u}_MOBJSUB_OFFSET "
          f"0x{row['mobjsub_offset']:04x}u")
        a(f"#define NDS_NATIVE_VS_EMBLEM_{u}_MATANIM_OFFSET "
          f"0x{row['matanim_offset']:04x}u")
        a("")
    a("/* Every distinct root, in selection-table order.  The owner accepts a")
    a(" * tree only when its child display list sits at one of these offsets")
    a(" * inside the loaded FTEmblemModels file. */")
    a("#define NDS_NATIVE_VS_EMBLEM_ROOTS { \\")
    for i, row in enumerate(rows):
        tail = " \\" if i + 1 < n else " }"
        a(f"    0x{row['dl_offset']:04x}u,{tail}")
    a("")
    a("#define NDS_NATIVE_VS_EMBLEM_ROOT_DL_BYTES { \\")
    for i, row in enumerate(rows):
        tail = " \\" if i + 1 < n else " }"
        a(f"    {row['dl_bytes']}u,{tail}")
    a("")
    a("/* FTKind -> index into the root table above.  Mario/Luigi and")
    a(" * Pikachu/Purin repeat, which is the source's own sharing. */")
    a("#define NDS_NATIVE_VS_EMBLEM_FKIND_SERIES { \\")
    for i, name in enumerate(FKIND_SERIES):
        tail = " \\" if i + 1 < len(FKIND_SERIES) else " }"
        a(f"    {index_of[name]}u, /* {i} {name} */{tail}")
    a("")
    a("#endif /* NDS_NATIVE_VS_EMBLEM_GENERATED_H */")
    return "\n".join(out) + "\n"


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--check", action="store_true",
                    help="fail if the generated header is out of date")
    args = ap.parse_args(argv)

    try:
        rows = collect()
    except SourceError as exc:
        print(f"generate_nds_native_vs_emblem: {exc}", file=sys.stderr)
        return 2

    text = render(rows)
    if args.check:
        if not OUT_H.is_file():
            print(f"generate_nds_native_vs_emblem: {OUT_H} does not exist",
                  file=sys.stderr)
            return 1
        current = OUT_H.read_text(encoding="utf-8")
        if current != text:
            print("generate_nds_native_vs_emblem: "
                  f"{OUT_H.relative_to(REPO)} is out of date; rerun without "
                  "--check", file=sys.stderr)
            return 1
        print(f"generate_nds_native_vs_emblem: OK ({len(rows)} series roots)")
        return 0

    OUT_H.parent.mkdir(parents=True, exist_ok=True)
    OUT_H.write_text(text, encoding="utf-8")
    print(f"generate_nds_native_vs_emblem: wrote "
          f"{OUT_H.relative_to(REPO)} ({len(rows)} series roots)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
