#!/usr/bin/env python3
"""P7 U3: the unlock registry's C table from scripts/p7/unlocks.json
(docs/P7/P7_Implementation_Plan.md Board 2 and U3).

Writes src/nds/nds_p7_unlocks.generated.inc: one NDSP7UnlockRow a row, the
fighters first in the JSON's order (the order unlocks are delivered in, as
the source orders its own), then the stages. Names resolve against the
source's kinds (fighters by nFTKind, 1P stages by nSC1PGameStage, stages by
nGRKind), scripts/p4/contents.json (a content's selection id is 12 + its id)
and scripts/p5/stages.json (a P5 stage's unlock id and gkind).

    python scripts/p7/generate_p7_unlocks.py [--check]
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
OUT = REPO / "src" / "nds" / "nds_p7_unlocks.generated.inc"

ORIGINALS = ("Mario", "Fox", "Donkey", "Samus", "Luigi", "Link", "Yoshi", "Captain", "Kirby",
             "Pikachu", "Purin", "Ness")
SEL_METAL = 12
STAGES_1P = ("Link", "Yoshi", "Fox", "Bonus1", "Mario", "Pikachu", "Donkey", "Bonus2", "Kirby",
             "Samus", "MMario", "Bonus3", "Zako", "Boss")
GKINDS = ("Castle", "Sector", "Jungle", "Zebes", "Hyrule", "Yoster", "Pupupu", "Yamabuki", "Inishie")
SITES = {"run_end": 0, "stage_clear": 1, "bonus1_end": 2, "bonus2_end": 3, "unlock": 4, "bonus_end": 5}
DELIVERY = {"direct": 0, "challenger": 1, "companion": 2}
ANY = 0xFF


class UnlockError(SystemExit):
    pass


# The table's names for contents whose contents.json title is shorter.
ALIASES = {"mad piano": "piano", "lanky kong": "lanky", "king dedede": "dedede",
           "meta knight": "metaknight", "mr. game & watch": "mrgaw"}


def _contents() -> dict[str, int]:
    rows = json.loads((REPO / "scripts" / "p4" / "contents.json").read_text(encoding="utf-8"))
    rows = rows if isinstance(rows, list) else rows.get("contents", [])
    return {str(r["name"]).lower(): int(r["id"]) for r in rows if isinstance(r, dict) and "name" in r}


def _p5() -> dict[str, dict]:
    rows = json.loads((REPO / "scripts" / "p5" / "stages.json").read_text(encoding="utf-8"))
    rows = rows if isinstance(rows, list) else rows.get("stages", [])
    return {r["name"]: r for r in rows if isinstance(r, dict) and "unlock_id" in r}


def _sel(name: str, contents: dict) -> int:
    if name in ORIGINALS:
        return ORIGINALS.index(name)
    if name.lower() == "metal mario":
        return SEL_METAL
    key = ALIASES.get(name.lower(), name.lower())
    if key in contents:
        return 12 + contents[key]
    raise UnlockError(f"unlocks.json: fighter {name!r} is neither an original nor a content")


def _mask(sels) -> int:
    m = 0
    for s in sels:
        m |= 1 << int(s)
    return m


def _gkind(name: str, p5: dict) -> int:
    if name in GKINDS:
        return GKINDS.index(name)
    if name in p5:
        return int(p5[name]["gkind"])
    raise UnlockError(f"unlocks.json: stage {name!r} is neither a source stage nor a P5 stage")


def rows(spec: dict) -> list[dict]:
    contents, p5 = _contents(), _p5()
    out = []
    for r in spec["fighters"]:
        sel = int(r["sel"])
        if sel != SEL_METAL and sel - 12 not in contents.values():
            raise UnlockError(f"unlocks.json: {r['name']}: selection {sel} has no content (12 + id)")
        if _sel(r["name"], contents) != sel:
            raise UnlockError(f"unlocks.json: {r['name']}: selection {sel}, its name says {_sel(r['name'], contents)}")
        out.append(_row(r, 0, sel, 0 if sel == SEL_METAL else sel - 12, 0, contents, p5))
    for r in spec["stages"]:
        st = p5.get(r["stage"])
        if st is None:
            raise UnlockError(f"unlocks.json: stage {r['stage']!r} is not a P5 stage")
        out.append(_row(r, 1, int(st["unlock_id"]), 0, int(st["gkind"]), contents, p5))
    return out


def _row(r: dict, kind: int, ident: int, content: int, stage_gkind: int, contents, p5) -> dict:
    site = r["site"]
    if site not in SITES:
        raise UnlockError(f"unlocks.json: site {site!r}")
    delivery = r["delivery"]
    if delivery not in DELIVERY:
        raise UnlockError(f"unlocks.json: delivery {delivery!r}")
    stage_1p = r.get("stage_cleared", r.get("stage") if site == "stage_clear" else None)
    if site == "stage_clear" and stage_1p not in STAGES_1P:
        raise UnlockError(f"unlocks.json: 1P stage {stage_1p!r}")
    challenger = r.get("stage") if delivery == "challenger" else None
    bonus = r.get("bonus_records") or {}
    return {
        "kind": kind, "id": ident, "site": SITES[site],
        "stage": STAGES_1P.index(stage_1p) if site == "stage_clear" else ANY,
        "fighters": _mask(_sel(n, contents) for n in r.get("fighters", ())),
        "min_difficulty": int(r.get("min_difficulty", 0)),
        "max_continues": int(r.get("max_continues", ANY)),
        "max_falls": int(r.get("max_falls", ANY)),
        "max_stage_falls": int(r.get("max_stage_falls", ANY)),
        "stock_setting": int(r.get("stock_setting", ANY)),
        "bonus_targets": int(bonus.get("targets", 0)),
        "bonus_platforms": int(bonus.get("platforms", 0)),
        "all_cleared": 1 if r.get("all_cleared") else 0,
        "all_targets": 1 if r.get("all_targets") else 0,
        "delivery": DELIVERY[delivery],
        "challenger_gkind": _gkind(challenger, p5) if challenger else ANY,
        "content": content, "stage_gkind": stage_gkind,
        "max_time_s": int(r.get("max_time_s", 0xFFFF)),
        "min_race_left_s": int(r.get("min_race_left_s", 0)),
        "requires": _mask(r.get("requires", ())),
        "companions": [c for c in r.get("companions", ())],
        "name": r.get("name", r.get("stage")),
    }


def render(table: list[dict]) -> str:
    lines = ["/* GENERATED by scripts/p7/generate_p7_unlocks.py from scripts/p7/unlocks.json.",
             " * Do not edit; regenerate. (P7 U3: the unlock registry.) */",
             f"#define NDS_P7_UNLOCK_ROWS {len(table)}u",
             "static const NDSP7UnlockRow sNdsP7UnlockRows[NDS_P7_UNLOCK_ROWS] = {"]
    for r in table:
        lines.append(
            "    {{ {kind}u, {id}u, {site}u, {stage}u, {min_difficulty}u, {max_continues}u, {max_falls}u, "
            "{max_stage_falls}u, {stock_setting}u, {bonus_targets}u, {bonus_platforms}u, {all_cleared}u, "
            "{all_targets}u, {delivery}u, {challenger_gkind}u, {content}u, {stage_gkind}u, {max_time_s}u, "
            "{min_race_left_s}u, 0x{fighters:016X}ull, 0x{requires:016X}ull }}, /* {name} */".format(**r))
    lines.append("};")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--check", action="store_true", help="fail if the generated file is stale")
    args = ap.parse_args()
    spec = json.loads((HERE / "unlocks.json").read_text(encoding="utf-8"))
    text = render(rows(spec))
    if args.check:
        if not OUT.exists() or OUT.read_text(encoding="utf-8") != text:
            print(f"{OUT} is stale: run scripts/p7/generate_p7_unlocks.py", file=sys.stderr)
            return 1
        return 0
    OUT.write_text(text, encoding="utf-8", newline="\n")
    print(f"generate_p7_unlocks: {OUT.relative_to(REPO)} ({len(spec['fighters'])} fighters, "
          f"{len(spec['stages'])} stages)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
