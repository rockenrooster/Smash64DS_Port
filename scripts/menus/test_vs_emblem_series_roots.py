#!/usr/bin/env python3
"""R01-B: every winner-emblem series root must survive, from source to header.

`mnVSResultsMakeEmblem` and `mnCharactersMakeEmblem` index three parallel
twelve-entry tables over the SAME ten `FTEmblemModels` roots -- Mario/Luigi
share one entry and Pikachu/Purin share another. A dropped or mis-shared root
does not fail loudly: it silently gives some fighter the wrong series emblem,
or none.

This test re-derives the table from the source and fails if the generated
header disagrees, if a root disappears, if the sharing changes, or if a
fighter kind stops mapping.

    python scripts/menus/test_vs_emblem_series_roots.py
"""

from __future__ import annotations

import importlib.util
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GENERATOR = ROOT / "scripts" / "menus" / "generate_nds_native_vs_emblem.py"
HEADER = (ROOT / "include" / "nds" / "generated" /
          "nds_native_vs_emblem.generated.h")
RESULTS_SRC = (ROOT / "decomp" / "BattleShip-main" / "decomp" / "src" / "mn" /
               "mnvsmode" / "mnvsresults.c")
CHARACTERS_SRC = (ROOT / "decomp" / "BattleShip-main" / "decomp" / "src" /
                  "mn" / "mndata" / "mncharacters.c")

# The source's own sharing, stated once here so a change to either consumer
# has to be argued rather than absorbed.
EXPECTED_SERIES = [
    "Mario", "Fox", "Donkey", "Metroid", "Mario", "Zelda",
    "Yoshi", "FZero", "Kirby", "PMonsters", "PMonsters", "Mother",
]
EXPECTED_DISTINCT = 10

failures: list[str] = []


def check(condition: bool, message: str) -> None:
    if not condition:
        failures.append(message)


def load_generator():
    spec = importlib.util.spec_from_file_location("gen_vs_emblem", GENERATOR)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def source_selection(path: Path, func: str) -> list[str]:
    """Read a consumer's own dobjdescs[] table out of the decomp."""
    text = path.read_text(encoding="utf-8")
    start = text.index(func)
    table = text.index("dobjdescs[", start)
    end = text.index("};", table)
    return re.findall(r"llFTEmblemModels(\w+?)DObjDesc", text[table:end])


def main() -> int:
    for path in (GENERATOR, RESULTS_SRC, CHARACTERS_SRC):
        if not path.is_file():
            print(f"FAIL: missing {path}", file=sys.stderr)
            return 1

    gen = load_generator()

    # 1. The generator's hardcoded fkind map must equal the source's own.
    check(gen.FKIND_SERIES == EXPECTED_SERIES,
          f"generator FKIND_SERIES drifted: {gen.FKIND_SERIES}")

    for path, func in ((RESULTS_SRC, "mnVSResultsMakeEmblem"),
                       (CHARACTERS_SRC, "mnCharactersMakeEmblem")):
        selection = source_selection(path, func)
        check(selection == EXPECTED_SERIES,
              f"{func} ({path.name}) selects {selection}, expected "
              f"{EXPECTED_SERIES}. Both consumers share these roots; they "
              f"must not diverge silently.")

    # 2. Ten distinct roots, and the two shared pairs really are shared.
    distinct = []
    for name in EXPECTED_SERIES:
        if name not in distinct:
            distinct.append(name)
    check(len(distinct) == EXPECTED_DISTINCT,
          f"expected {EXPECTED_DISTINCT} distinct series roots, "
          f"derived {len(distinct)}: {distinct}")
    check(EXPECTED_SERIES[0] == EXPECTED_SERIES[4],
          "Mario (0) and Luigi (4) no longer share a series root")
    check(EXPECTED_SERIES[9] == EXPECTED_SERIES[10],
          "Pikachu (9) and Jigglypuff (10) no longer share a series root")

    # 3. The generator must still resolve every root out of the source tables
    #    and agree with the port's registered reloc offsets.
    try:
        rows = gen.collect()
    except gen.SourceError as exc:
        print(f"FAIL: generator refused the source tables: {exc}",
              file=sys.stderr)
        return 1

    check(len(rows) == EXPECTED_DISTINCT,
          f"generator produced {len(rows)} roots, expected "
          f"{EXPECTED_DISTINCT}")
    got_names = [row["name"] for row in rows]
    check(got_names == distinct,
          f"generator root order {got_names} != selection order {distinct}")

    offsets = [row["dl_offset"] for row in rows]
    check(len(set(offsets)) == len(offsets),
          f"two series resolved to the same display list: {offsets}")
    for row in rows:
        check(row["vertex_count"] > 0,
              f"{row['name']}: zero vertices")
        check(row["dl_bytes"] > 0,
              f"{row['name']}: empty display list")
        check(row["dobj_count"] == 2,
              f"{row['name']}: tree is {row['dobj_count']} DObjs, not the "
              "source's root+child pair the owner walks")

    # 4. The committed header must be what the generator emits now, and must
    #    carry every root.
    if not HEADER.is_file():
        failures.append(f"{HEADER} does not exist; run the generator")
    else:
        current = HEADER.read_text(encoding="utf-8")
        check(current == gen.render(rows),
              f"{HEADER.relative_to(ROOT)} is stale; rerun "
              "scripts/menus/generate_nds_native_vs_emblem.py")
        for row in rows:
            token = f"NDS_NATIVE_VS_EMBLEM_{row['name'].upper()}_ROOT"
            check(token in current, f"{token} missing from the header")
        check(f"NDS_NATIVE_VS_EMBLEM_SERIES_COUNT {EXPECTED_DISTINCT}u"
              in current,
              "header SERIES_COUNT does not state ten roots")

    if failures:
        for message in failures:
            print(f"FAIL: {message}", file=sys.stderr)
        return 1
    print(f"PASS: winner-emblem series roots "
          f"({EXPECTED_DISTINCT} roots, 12 fighter kinds, 2 consumers)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
