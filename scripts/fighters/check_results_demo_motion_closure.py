#!/usr/bin/env python3
"""Every VS Results demo pose must resolve to a loadable figatree and a bake.

THE FAILURE THIS CATCHES, and it is the whole of the owner's three Results
animation rows ("All Fighters are not doing correct poses/animations on results
screen", "No contest results screen, all fighters should be doing the clapping
animations", "Kirby has a wierd pose on results screen").

`mnVSResultsSetFighterStatus` (decomp mn/mnvsmode/mnvsresults.c:892) hands every
present fighter a DEMO status: `mnVSResultsGetStatusWin` picks Win1/Win2/Win3
(Win1/Win2 only for Kirby, :878-882) for the winner and `mnVSResultsGetStatusLose`
returns DemoLose for everyone else -- and for EVERY present fighter when the kind
is No Contest (:894-897). `ftMainSetStatus` resolves a demo status through the
identity table `D_ovl1_80390BE8`, so `motion_id = status - 0x10000` indexes the
fighter's `dFT<Kind>SubMotionDescs` row directly (decomp ft/ftmain.c:4559-4562,
4608-4610).

That row's first field is the animation the pose needs.  `ftMainSetStatus` then
does this, and the second line is the trap:

    else if (motion_desc->anim_file_id != 0)
    {
        lbRelocGetForceExternHeapFile(motion_desc->anim_file_id, fp->figatree_heap);
        fp->figatree = fp->figatree_heap;          <-- unconditional
    }

`anim_file_id` is the ADDRESS of an `ll...FileID` symbol, so it is never zero and
the guard never fires.  The return value is discarded.  When the port cannot map
that token to an asset, `lbRelocGetExternHeapFile` returns the heap untouched
(src/port/reloc_backend_assets.c:12424-12427) and the fighter is bound to
WHATEVER FIGATREE THE HEAP STILL HELD -- its last battle motion.  The demo event
script still runs, so the model parts still swap; only the animation is stale.
There is no decline, no reject and no halt: the fighter simply holds a battle
pose with clapping hands attached.

So a Results demo row is closed only when BOTH hold:

  * the animation symbol resolves to something the port can actually load --
    a compiled-in clip in `battleship_scsubsysdata_ft.c`, one of the Mario/Fox
    bridge arrays in `reloc_backend_assets.c`, or a generated
    `NDS_P2_<KIND>_ANIM_ASSET_ROWS` row with its NitroFS path; and
  * every model part the row's demo script installs has a native bake, which is
    `check_model_part_mutation_coverage.py`'s job -- this checker re-runs that
    resolution for the Results rows alone so a removed demo root fails HERE too,
    which is the fixture the consolidated brief asks for (an inventory count is
    not a test).

Usage:
    python scripts/fighters/check_results_demo_motion_closure.py
    python scripts/fighters/check_results_demo_motion_closure.py --self-test

Exit 0 when every Results-reachable demo pose of every admitted fighter is
closed; 1 otherwise, naming the fighter, the outcome, the submotion row and the
symbol or root that is missing.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import _paths  # noqa: E402
import check_model_part_mutation_coverage as census  # noqa: E402

REPO = _paths.REPO_ROOT
SCSUBSYS = REPO / "decomp/BattleShip-main/decomp/src/sc/scsubsys"
MANIFEST = REPO / "scripts/fighters/fighter_production_manifest.json"
SCSUBSYS_IMPORT = REPO / "src/import/battleship_scsubsysdata_ft.c"
RELOC_BACKEND = REPO / "src/port/reloc_backend_assets.c"
RELOC_ASSET_PATHS = REPO / "src/nds/nds_reloc_assets.c"
PRODUCTION_HEADER = (
    REPO / "include/nds/generated/nds_fighter_production.generated.h")

# `mnVSResultsGetStatusWin`/`GetStatusLose` in source order.  The demo status
# value IS the submotion row (see the module docstring), so these are rows.
WIN_ROWS = (1, 2, 3)
KIRBY_WIN_ROWS = (1, 2)
LOSE_ROW = 5
ROW_LABEL = {1: "Win1", 2: "Win2", 3: "Win3", 5: "Lose/NoContest claps"}

SUBMOTION_TABLE = re.compile(
    r"FTMotionDesc dFT(\w+?)SubMotionDescs\[\]\s*=\s*\{(.*?)\n\};", re.S)
ANIM_SYMBOL = re.compile(r"ll[A-Za-z0-9_]*FileID")


def admitted_fighters() -> list[str]:
    """The roster the production manifest admits, lower-cased."""
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    return [str(row["fighter"]).lower() for row in manifest["fighters"]]


def submotion_rows(owner: str) -> tuple[str, dict[int, tuple[str, str]]]:
    """(source table tag, {row: (anim symbol, event script field)})."""
    path = SCSUBSYS / ("scsubsysdata%s.c" % owner)
    if not path.is_file():
        return "", {}
    text = path.read_text(encoding="utf-8", errors="replace")
    table = SUBMOTION_TABLE.search(text)
    if table is None:
        return "", {}
    fields = [field.strip() for field in table.group(2).split(",")]
    rows: dict[int, tuple[str, str]] = {}
    for row in range(0, len(fields) // census.SUBMOTION_FIELDS_PER_ROW):
        base = row * census.SUBMOTION_FIELDS_PER_ROW
        anim = fields[base].lstrip("&")
        script = fields[base + 1].lstrip("&")
        rows[row] = (anim, script)
    return table.group(1), rows


def loadable_anim_symbols() -> dict[str, str]:
    """Animation symbols the port can actually resolve, and by which route.

    Read from the three places that answer `ndsRelocAssetIDForToken`, not from a
    hand-kept list: a route that is deleted or renamed must make this checker go
    red rather than silently shrink the covered set.
    """
    routes: dict[str, str] = {}

    text = SCSUBSYS_IMPORT.read_text(encoding="utf-8", errors="replace")
    loader = text.find("void *ndsBattleShipLoadCSSSelectedFigatree")
    if loader < 0:
        raise SystemExit(
            "battleship_scsubsysdata_ft.c no longer defines "
            "ndsBattleShipLoadCSSSelectedFigatree; the compiled-in demo clip "
            "route moved and this checker must follow it")
    for symbol in ANIM_SYMBOL.findall(text[loader:]):
        routes.setdefault(symbol, "compiled-in clip (battleship_scsubsysdata_ft.c)")

    backend = RELOC_BACKEND.read_text(encoding="utf-8", errors="replace")
    # `sNdsRelocDemoAnimTokens` is the VS Results demo table. It contributes no
    # symbol text of its own -- it expands the generated `*_DEMO_ANIM_ASSET_ROWS`
    # macros read below -- but it is the only thing that turns those rows into a
    # runtime route, so requiring it here is what stops a build that still emits
    # the macros while nothing consumes them from reading as closed.
    for array in ("sNdsRelocMarioBattleAnimFileIDs", "sNdsRelocFoxAnimFileIDs",
                  "sNdsRelocDemoAnimTokens"):
        start = backend.find("%s[] =" % array)
        if start < 0:
            raise SystemExit(
                "reloc_backend_assets.c no longer defines %s; the base-pair "
                "animation bridge moved and this checker must follow it" % array)
        end = backend.find("\n};", start)
        for symbol in ANIM_SYMBOL.findall(backend[start:end]):
            routes.setdefault(symbol, "base-pair bridge (%s)" % array)

    if PRODUCTION_HEADER.is_file():
        header = PRODUCTION_HEADER.read_text(encoding="utf-8", errors="replace")
        # `NDS_\w+` rather than `NDS_P2_\w+`: Mario and Fox are the
        # always-compiled base pair and carry no P2 flag, so their Results demo
        # rows are emitted as NDS_MARIOFOX_DEMO_ANIM_ASSET_ROWS. Narrowing this
        # to the P2 prefix would hide three real routes (and Mario's Claps is
        # Luigi's DemoLose besides). This only widens which producer text is
        # read; every assertion below is unchanged.
        for match in re.finditer(
                r"#define (NDS_\w+_ANIM_ASSET_ROWS)\(X\)(.*?)\n\n",
                header, re.S):
            for symbol in ANIM_SYMBOL.findall(match.group(2)):
                routes.setdefault(symbol, "generated %s" % match.group(1))
    return routes


def demo_path_route_failures() -> list[str]:
    """Every demo-anim row the token table claims must also have a NitroFS path.

    THE FAILURE THIS CATCHES, and it cost the Results rows an entire cycle.
    `NDS_*_DEMO_ANIM_ASSET_ROWS` carries three fields -- symbol, asset id, path.
    `sNdsRelocDemoAnimTokens` in reloc_backend_assets.c expands the first two, so
    the token resolved, `ndsRelocIsFighterAnimID` answered TRUE and the loader ran.
    Nothing expanded the third. `ndsRelocForceLoadFighterAObj16File` then hit its
    `ndsRelocAssetGetPath(asset_id) == NULL` guard, returned NULL, and
    `ftMainSetStatus` bound the stale figatree heap -- no decline, no reject, and
    every fighter holding its last battle pose on the Results screen.

    Static routing is not a runtime load. A row is only routed when BOTH tables
    expand it, so require the two arm-for-arm.
    """
    macro = re.compile(r"(NDS_\w+_DEMO_ANIM_ASSET_ROWS)\s*\(")
    backend = RELOC_BACKEND.read_text(encoding="utf-8", errors="replace")
    start = backend.find("sNdsRelocDemoAnimTokens[] =")
    if start < 0:
        raise SystemExit(
            "reloc_backend_assets.c no longer defines sNdsRelocDemoAnimTokens; "
            "the demo token route moved and this checker must follow it")
    token_arms = set(macro.findall(backend[start:backend.find("\n};", start)]))

    if not RELOC_ASSET_PATHS.is_file():
        return ["%s is missing; the demo animation path table cannot be read"
                % RELOC_ASSET_PATHS]
    paths_text = RELOC_ASSET_PATHS.read_text(encoding="utf-8", errors="replace")
    start = paths_text.find("sNdsRelocAssets[] = {")
    if start < 0:
        raise SystemExit(
            "nds_reloc_assets.c no longer defines sNdsRelocAssets; the asset "
            "path table moved and this checker must follow it")
    path_arms = set(macro.findall(paths_text[start:paths_text.find("\n};", start)]))

    return [
        "%s is expanded into sNdsRelocDemoAnimTokens but not into "
        "sNdsRelocAssets: its demo poses resolve an asset id and then fail "
        "ndsRelocAssetGetPath, so ftMainSetStatus silently binds the stale "
        "figatree heap" % arm
        for arm in sorted(token_arms - path_arms)
    ]


def demo_root_failures(owners: list[str]) -> list[str]:
    """Model-part roots the Results demo scripts install with no native bake.

    Reuses the census's own resolution chain so the two checkers cannot drift.
    """
    baked = census.baked_offsets()
    failures: list[str] = []
    for owner in owners:
        cap = owner[:1].upper() + owner[1:]
        demo_path = SCSUBSYS / ("scsubsysdata%s.c" % owner)
        if not demo_path.is_file():
            continue
        demo_text = demo_path.read_text(encoding="utf-8", errors="replace")
        _tag, rows = submotion_rows(owner)
        results_rows = set(WIN_ROWS) | {LOSE_ROW}
        wanted = {
            script: sorted(
                row for row, (_anim, other) in rows.items()
                if other == script and row in results_rows)
            for _row, (_anim, script) in rows.items()
            for script in (rows[_row][1],)
        }
        wanted = {script: hits for script, hits in wanted.items() if hits}
        mutations: list[tuple[str, int, int]] = []
        current = "?"
        for line in demo_text.splitlines():
            match = census.CSS_ARR.match(line)
            if match:
                current = match.group(1)
                continue
            if current not in wanted:
                continue
            for word in census.RAW_HEX.findall(line):
                decoded = census.decode_raw_modelpart(int(word, 16))
                if decoded is None:
                    continue
                joint, part = decoded
                if 0 < part < 64:
                    label = "%s (%s)" % (
                        current,
                        "/".join(ROW_LABEL[row] for row in wanted[current]))
                    mutations.append((label, joint, part))
        if not mutations:
            continue

        main_path = next(census.RELOC.glob("[0-9]*_%sMain.c" % cap), None)
        if main_path is None:
            failures.append("%s: no Main.c to resolve its demo mutations" % owner)
            continue
        main_text = main_path.read_text(encoding="utf-8", errors="replace")
        container = census.CONTAINER.search(main_text)
        if container is None:
            failures.append("%s: no modelparts_container in Main.c" % owner)
            continue
        entries = [entry.strip()
                   for entry in container.group(1).replace("\n", " ").split(",")
                   if entry.strip()]
        descriptors = {
            "desc_%s" % tag.lower(): census.GFXROW.findall(body)
            for tag, body in census.DESC.findall(main_text)
        }
        offsets = census.model_offsets(cap)
        for label, joint, part in sorted(set(mutations)):
            if (owner, joint, part) in census.FOREIGN_PROGRAM_ROOTS:
                continue
            index = joint - 4
            entry = entries[index] if 0 <= index < len(entries) else ""
            tag_match = re.search(r"modelparts_desc_(0x[0-9A-Fa-f]+)", entry)
            symbols = descriptors.get(
                "desc_%s" % tag_match.group(1).lower() if tag_match else "", [])
            for detail_index, detail in enumerate(("high", "low")):
                row = part * 2 + detail_index
                symbol = symbols[row] if row < len(symbols) else None
                offset = offsets.get(symbol) if symbol else None
                if offset is None:
                    failures.append(
                        "%s demo %s: joint %d part %d %s does not resolve to a "
                        "display list" % (owner, label, joint, part, detail))
                    continue
                if offset not in baked.get(owner, {}).get(detail, set()):
                    failures.append(
                        "%s demo %s: joint %d part %d %s is 0x%x, which has NO "
                        "native bake -- this Results pose stops the fighter "
                        "drawing" % (owner, label, joint, part, detail, offset))
    return failures


def check(owners: list[str] | None = None,
          routes: dict[str, str] | None = None) -> tuple[list[str], int]:
    """(failures, closed row count).  Arguments exist for --self-test."""
    owners = admitted_fighters() if owners is None else owners
    routes = loadable_anim_symbols() if routes is None else routes
    failures: list[str] = []
    closed = 0
    for owner in owners:
        tag, rows = submotion_rows(owner)
        if not rows:
            failures.append(
                "%s: no dFT*SubMotionDescs table -- the Results screen has no "
                "demo row to play for this fighter" % owner)
            continue
        win_rows = KIRBY_WIN_ROWS if owner == "kirby" else WIN_ROWS
        for row in tuple(win_rows) + (LOSE_ROW,):
            if row not in rows:
                failures.append(
                    "%s dFT%sSubMotionDescs: row %d (%s) is missing from the "
                    "source table" % (owner, tag, row, ROW_LABEL[row]))
                continue
            anim, _script = rows[row]
            if not anim.startswith("ll"):
                failures.append(
                    "%s %s: submotion row %d names no animation symbol (%s)"
                    % (owner, ROW_LABEL[row], row, anim))
                continue
            if anim not in routes:
                failures.append(
                    "%s %s: submotion row %d wants %s, which no port route can "
                    "resolve -- ftMainSetStatus will bind the stale figatree "
                    "heap and replay the last battle motion"
                    % (owner, ROW_LABEL[row], row, anim))
                continue
            closed += 1
    failures.extend(demo_path_route_failures())
    failures.extend(demo_root_failures(owners))
    return failures, closed


def self_test() -> int:
    """Prove the checker fails when a required Results input is removed.

    An inventory count is not a test: both arms below take a currently closed
    input away and require the red.
    """
    routes = loadable_anim_symbols()
    problems: list[str] = []

    # ARM 1 -- a removed animation route, with its own control. Link's Results
    # Win1 row plays his compiled-in Selected clip, which resolves today, so
    # deleting that one route must move this owner from green to red. PROVE THE
    # CONTROL DIFFERS: a checker that is already red for another reason would
    # otherwise "pass" this arm without the removal doing anything.
    victim = "llFTLinkAnimSelectedFileID"
    before, _closed = check(["link"], routes)
    if any(victim in line for line in before):
        problems.append(
            "control arm: %s is already unresolvable, so its removal proves "
            "nothing" % victim)
    reduced = {k: v for k, v in routes.items() if k != victim}
    after, _closed = check(["link"], reduced)
    if not any(victim in line for line in after):
        problems.append(
            "removing %s from the resolvable set did not fail the animation "
            "arm" % victim)

    # ARM 2 -- a removed model-part bake, with the same control. Mario's Claps
    # row installs his two alternate hands; they are baked only in
    # BASE_MODEL_PART_ROOT_VARIANTS, and Mario is green there today.
    if demo_root_failures(["mario"]):
        problems.append(
            "control arm: mario's demo model-part roots are already red, so "
            "removing his bake proves nothing")
    saved = census.native.BASE_MODEL_PART_ROOT_VARIANTS.pop("mario", None)
    try:
        failures = demo_root_failures(["mario"])
    finally:
        if saved is not None:
            census.native.BASE_MODEL_PART_ROOT_VARIANTS["mario"] = saved
    if not any("NO native bake" in line for line in failures):
        problems.append(
            "removing mario's BASE_MODEL_PART_ROOT_VARIANTS rows did not fail "
            "the demo model-part arm")

    if problems:
        print("RESULTS_DEMO_MOTION_SELFTEST_FAIL")
        for problem in problems:
            print("  %s" % problem)
        return 1
    print("RESULTS_DEMO_MOTION_SELFTEST_OK both removal arms go red")
    return 0


def main(argv: list[str]) -> int:
    if "--self-test" in argv:
        return self_test()
    failures, closed = check()
    print("  Results demo closure: %d reachable (fighter, outcome) rows have a "
          "resolvable figatree" % closed)
    if failures:
        print("RESULTS_DEMO_MOTION_CLOSURE_FAIL")
        for failure in failures:
            print("  %s" % failure)
        return 1
    print("RESULTS_DEMO_MOTION_CLOSURE_OK every reachable Results pose has a "
          "figatree route and a native bake")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
