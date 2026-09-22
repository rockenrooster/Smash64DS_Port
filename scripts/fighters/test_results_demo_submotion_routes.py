#!/usr/bin/env python3
"""The VS Results demo poses must be staged, routed and gated as ONE thing.

WHY THIS EXISTS.  Making the Results demo animations loadable (BUGS.md R02/R03
/K06) took four wiring sites: a table in the generator, a NitroFS file list in
`fighter_production_files.mk`, a token-row macro in
`nds_fighter_production.generated.h`, and a consumer table plus a roster gate in
`reloc_backend_assets.c` and the `Makefile`.  Three of those are generated from
one table, but the two gates are hand-written, and the original bug was itself a
wiring gap that nothing detected: `ftMainSetStatus` assigns
`fp->figatree = fp->figatree_heap` unconditionally and discards
`lbRelocGetForceExternHeapFile`'s return value, so an unroutable token produced a
STALE POSE rather than a decline.  A half-wired row would fail exactly the same
silent way -- the fighter would stand in his last battle motion and no counter
would move.

`check_results_demo_motion_closure.py` proves the ROUTE exists.  This proves the
route is backed by a staged file of the right identity and reachable under the
build's own gates:

  1. every id is read from the O2R file's own header at 0x40, never trusted from
     a name -- decomp and O2R label the same file differently (id 416 is
     `FTKirbyAnimWin1` in decomp and `FTKirbySubMotionAppearR` in O2R), so a
     transcribed table with a systematic off-by-N would put the wrong animation
     on every fighter and still look plausible;
  2. staged paths and routed paths are a bijection -- a route with no file
     cannot load, and a packed file with no route is dead NitroFS bytes;
  3. every generated `*_DEMO_ANIM_ASSET_ROWS` macro is expanded by
     `sNdsRelocDemoAnimTokens`, under the same `NDS_P2_<KIND>` gate;
  4. every generated `*_DEMO_RELOC_FILES` list is fanned into
     `NDS_VS_RESULTS_RELOC_FILES`, under that same gate, so the ROM the routed
     id names is actually in the ROM;
  5. a fighter is never gated out of a row he needs: the fighter -> symbol map
     is re-derived from the decomp `dFT<Kind>SubMotionDescs` tables, and each
     symbol's gate must be the always-compiled base pair or that fighter
     himself (the corpus borrows freely -- Purin's Win1 names a Luigi-shaped
     symbol whose file is Purin's, and Luigi's DemoLose is Mario's Claps).

Usage:
    python scripts/fighters/test_results_demo_submotion_routes.py
    python scripts/fighters/test_results_demo_submotion_routes.py --self-test

Exit 0 when all five hold; 1 otherwise, naming the row and the site.
"""
from __future__ import annotations

import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import _paths  # noqa: E402
import check_results_demo_motion_closure as closure  # noqa: E402
import generate_fighter_production_manifest as gen  # noqa: E402

REPO = _paths.REPO_ROOT
O2R = REPO / "decomp/BattleShip-main/BattleShip_o2r"
MANIFEST = REPO / "scripts/fighters/fighter_production_manifest.json"
MAKE_FRAGMENT = REPO / "scripts/fighters/fighter_production_files.mk"
MAKEFILE = REPO / "Makefile"
PRODUCTION_HEADER = (
    REPO / "include/nds/generated/nds_fighter_production.generated.h")
RELOC_BACKEND = REPO / "src/port/reloc_backend_assets.c"

BASE_GATES = set(gen.RESULTS_DEMO_BASE_GATES)
C_TABLE = "sNdsRelocDemoAnimTokens"


def demo_rows() -> list[dict]:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    rows = manifest.get("results_demo_submotions")
    if not rows:
        raise SystemExit(
            "fighter_production_manifest.json carries no "
            "results_demo_submotions; the Results demo table was removed from "
            "the generator and this test must follow it")
    return list(rows)


def o2r_file_id(path: str) -> int | None:
    """The id from the file's own header, which is the authority."""
    blob = O2R / path
    if not blob.is_file():
        return None
    raw = blob.read_bytes()
    if len(raw) < 0x44 or raw[4:8] != b"OLER":
        return None
    return int(struct.unpack_from("<I", raw, 0x40)[0])


def make_variable(text: str, name: str) -> list[str]:
    """The paths a `NAME := \\`-continued Make assignment lists."""
    # `\Z` as well as the next unindented line: the last generated list sits at
    # end of file, and a lookahead that only accepts a following statement
    # reads it as empty -- which shows up as a routed-but-unstaged false alarm.
    match = re.search(
        r"^%s\s*:=(.*?)(?=\n[^\s\\]|\Z)" % re.escape(name), text,
        re.S | re.M)
    if match is None:
        return []
    body = match.group(1).replace("\\\n", " ")
    return [token for token in body.split() if token]


def gate_for(name: str) -> str:
    """The Make/C flag a generated demo variable rides."""
    kind = re.match(r"NDS_(?:P2_)?(\w+)_DEMO_(?:ANIM_ASSET_ROWS|RELOC_FILES)$",
                    name)
    if kind is None:
        return ""
    return kind.group(1)


def required_symbols() -> dict[str, set[str]]:
    """{animation symbol: fighters whose Results rows name it}."""
    wanted: dict[str, set[str]] = {}
    for owner in closure.admitted_fighters():
        _tag, rows = closure.submotion_rows(owner)
        win = closure.KIRBY_WIN_ROWS if owner == "kirby" else closure.WIN_ROWS
        for row in tuple(win) + (closure.LOSE_ROW,):
            if row not in rows:
                continue
            anim = rows[row][0]
            if anim.startswith("ll"):
                wanted.setdefault(anim, set()).add(owner)
    return wanted


def check(rows: list[dict] | None = None,
          header: str | None = None,
          backend: str | None = None,
          makefile: str | None = None) -> list[str]:
    """Arguments exist for --self-test."""
    rows = demo_rows() if rows is None else rows
    header = (PRODUCTION_HEADER.read_text(encoding="utf-8", errors="replace")
              if header is None else header)
    backend = (RELOC_BACKEND.read_text(encoding="utf-8", errors="replace")
               if backend is None else backend)
    makefile = (MAKEFILE.read_text(encoding="utf-8", errors="replace")
                if makefile is None else makefile)
    fragment = MAKE_FRAGMENT.read_text(encoding="utf-8", errors="replace")
    failures: list[str] = []

    # 1 -- the id is the file's own, not the table's.
    for row in rows:
        path = str(row["asset"]["path"])
        claimed = int(row["asset"]["id"])
        actual = o2r_file_id(path)
        if actual is None:
            failures.append(
                "%s: %s is not a readable O2R file, so the route names nothing"
                % (row["symbol"], path))
            continue
        if actual != claimed:
            failures.append(
                "%s: routed id 0x%x but %s carries 0x%x in its own header -- "
                "this would play a DIFFERENT fighter's animation"
                % (row["symbol"], claimed, path, actual))

    # 2 -- staged paths and routed paths are a bijection.
    routed_paths = {str(row["asset"]["path"]) for row in rows}
    staged_paths: set[str] = set()
    for match in re.finditer(r"^(NDS_\w*_DEMO_RELOC_FILES)\s*:=", fragment,
                             re.M):
        staged_paths.update(make_variable(fragment, match.group(1)))
    for path in sorted(routed_paths - staged_paths):
        failures.append(
            "%s is routed but never staged into NitroFS; the token would "
            "resolve to an asset the ROM does not carry" % path)
    for path in sorted(staged_paths - routed_paths):
        failures.append(
            "%s is staged into NitroFS but no token routes to it; dead bytes"
            % path)

    # 3 -- every generated macro is consumed by the C table, under its gate.
    table_start = backend.find("%s[] =" % C_TABLE)
    if table_start < 0:
        failures.append(
            "reloc_backend_assets.c no longer defines %s; nothing expands the "
            "generated demo rows into a runtime route" % C_TABLE)
        table_body = ""
    else:
        table_body = backend[table_start:backend.find("\n};", table_start)]
    macros = sorted(set(re.findall(r"^#define (NDS_\w*_DEMO_ANIM_ASSET_ROWS)\(X\)",
                                   header, re.M)))
    if not macros:
        failures.append(
            "the generated header emits no *_DEMO_ANIM_ASSET_ROWS macro; the "
            "Results demo rows were dropped from the generator")
    for macro in macros:
        if macro not in table_body:
            failures.append(
                "%s is generated but %s never expands it -- the rows exist in "
                "the header and resolve to nothing at runtime" % (macro, C_TABLE))
            continue
        gate = gate_for(macro)
        if gate in {name.upper() for name in BASE_GATES} or gate == "MARIOFOX":
            continue
        guard = "#if NDS_P2_%s\n    %s(" % (gate, macro)
        if guard not in table_body:
            failures.append(
                "%s is expanded in %s without its `#if NDS_P2_%s` gate, so a "
                "build without that fighter would route poses it does not pack"
                % (macro, C_TABLE, gate))

    # 4 -- every generated file list is fanned into the VS Results set.
    lists = sorted(set(re.findall(r"^(NDS_\w*_DEMO_RELOC_FILES)\s*:=", fragment,
                                  re.M)))
    for name in lists:
        if not make_variable(fragment, name):
            continue  # an empty list needs no fan-in
        fan = "NDS_VS_RESULTS_RELOC_FILES += $(%s)" % name
        if fan not in makefile:
            failures.append(
                "%s is generated but the Makefile never adds it to "
                "NDS_VS_RESULTS_RELOC_FILES; those poses are routed but never "
                "packed" % name)
            continue
        gate = gate_for(name)
        if gate == "MARIOFOX":
            continue
        guard = "ifeq ($(NDS_P2_%s),1)\n%s\nendif" % (gate, fan)
        if guard not in makefile:
            failures.append(
                "%s is fanned in without its `ifeq ($$(NDS_P2_%s),1)` gate, so "
                "a build without that fighter would pack his poses" % (name, gate))

    # 5 -- no fighter is gated out of a row he needs.
    gate_of = {str(row["symbol"]): str(row["gate"]) for row in rows}
    for symbol, owners in sorted(required_symbols().items()):
        gate = gate_of.get(symbol)
        if gate is None:
            continue  # routed by a compiled-in clip or a base-pair bank
        if gate in BASE_GATES:
            continue
        for owner in sorted(owners):
            if owner.lower() != gate.lower():
                failures.append(
                    "%s is gated on %s but %s's Results rows name it; a build "
                    "with %s and without %s would replay his stale battle pose"
                    % (symbol, gate, owner, owner, gate))
    return failures


def self_test() -> int:
    """Prove each arm goes red when its input is taken away."""
    rows = demo_rows()
    header = PRODUCTION_HEADER.read_text(encoding="utf-8", errors="replace")
    backend = RELOC_BACKEND.read_text(encoding="utf-8", errors="replace")
    makefile = MAKEFILE.read_text(encoding="utf-8", errors="replace")
    problems: list[str] = []

    # PROVE THE CONTROL DIFFERS first: an already-red checker would "pass"
    # every arm below without the removal doing anything.
    if check(rows, header, backend, makefile):
        return _report(["control arm: the check is already red, so no removal "
                        "below proves anything"])

    # ARM 1 -- a corrupted id must not pass as a name match.
    victim = [dict(row) for row in rows]
    victim[0] = json.loads(json.dumps(victim[0]))
    victim[0]["asset"]["id"] = int(victim[0]["asset"]["id"]) + 1
    if not any("its own header" in line for line in
               check(victim, header, backend, makefile)):
        problems.append("an off-by-one routed id did not fail the identity arm")

    # ARM 2 -- the C table stops expanding a generated macro.
    stripped = backend.replace(
        "    NDS_P2_KIRBY_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)\n",
        "")
    if stripped == backend:
        problems.append(
            "control arm: NDS_P2_KIRBY_DEMO_ANIM_ASSET_ROWS is not expanded "
            "the way this arm expects, so removing it proves nothing")
    elif not any("never expands it" in line for line in
                 check(rows, header, stripped, makefile)):
        problems.append(
            "dropping a generated macro from the C table did not fail the "
            "consumer arm")

    # ARM 3 -- the Makefile stops packing a routed fighter's poses.
    unpacked = makefile.replace(
        "NDS_VS_RESULTS_RELOC_FILES += $(NDS_P2_KIRBY_DEMO_RELOC_FILES)", "")
    if unpacked == makefile:
        problems.append(
            "control arm: the Kirby demo fan-in is not in the Makefile the way "
            "this arm expects, so removing it proves nothing")
    elif not any("never adds it to" in line for line in
                 check(rows, header, backend, unpacked)):
        problems.append(
            "dropping a demo file list from NDS_VS_RESULTS_RELOC_FILES did not "
            "fail the staging arm")

    # ARM 4 -- a row gated away from the fighter who needs it.
    misgated = [json.loads(json.dumps(row)) for row in rows]
    for row in misgated:
        if row["symbol"] == "llFTKirbyAnimWin1FileID":
            row["gate"] = "Yoshi"
    if not any("gated on Yoshi" in line for line in
               check(misgated, header, backend, makefile)):
        problems.append(
            "moving Kirby's Win1 row onto another fighter's gate did not fail "
            "the gate-coverage arm")

    return _report(problems)


def _report(problems: list[str]) -> int:
    if problems:
        print("RESULTS_DEMO_SUBMOTION_ROUTES_SELFTEST_FAIL")
        for problem in problems:
            print("  %s" % problem)
        return 1
    print("RESULTS_DEMO_SUBMOTION_ROUTES_SELFTEST_OK every removal arm goes red")
    return 0


def main(argv: list[str]) -> int:
    if "--self-test" in argv:
        return self_test()
    rows = demo_rows()
    failures = check(rows)
    print("  Results demo submotions: %d routed rows, ids read from the O2R "
          "headers" % len(rows))
    if failures:
        print("RESULTS_DEMO_SUBMOTION_ROUTES_FAIL")
        for failure in failures:
            print("  %s" % failure)
        return 1
    print("RESULTS_DEMO_SUBMOTION_ROUTES_OK staged, routed and gated agree")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
