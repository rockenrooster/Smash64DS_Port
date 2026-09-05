#!/usr/bin/env python3
"""Census of the scene kinds the linked sources can enter versus the port's
scene registry (``src/port/nds_scene_manager.c``).

The registry fails closed on a *request* (``ndsSceneManagerRequest`` refuses
an unlisted kind and counts it) and only counts a *direct* entry (the source
writes ``gSCManagerSceneData.scene_curr`` itself and unwinds; Enter bumps
``gNdsSceneManagerUnregisteredEnterCount``).  Either way an imported scene
without a row is a hole the code-first mode cannot see: on 2026-09-05 the
Option screen's SCREEN ADJUST row dispatched to a stub that parked forever
because nothing had rowed ``nSCKindScreenAdjust`` or imported its scene.

This lists every ``nSCKind*`` written to ``scene_curr`` (or passed to
``ndsMenuShellGoto`` / ``ndsSceneManagerRequest``) by a source the port
compiles -- the decomp files each ``src/import/battleship_*.c`` includes,
after the ``REGION_JP`` / ``!SSB64_TARGET_NDS`` arms are dropped, plus
``src/nds`` and ``src/port`` -- that has no ``{ (u8)nSCKind... }`` row.

Usage:
    python scripts/menus/check_scene_registry_census.py            # report
    python scripts/menus/check_scene_registry_census.py --strict   # exit 1 if any

Kinds in ALLOW are known and accepted: Startup is deliberately outside the
registry (its arena is the N64 overlay span, see ndsSceneManagerEnter),
NoController cannot happen on a DS, and the opening movie chain is
owner-deferred (it enters unregistered today; rows are hygiene only).
"""
from __future__ import annotations

import argparse
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_reloc_symbol_census import INC_RE, as_built, patched_source, read  # noqa: E402

WRITE_RES = (
    re.compile(r"scene_curr\s*=\s*(nSCKind\w+)"),
    re.compile(r"ndsMenuShellGoto\(\s*\(u32\)\s*(nSCKind\w+)"),
    re.compile(r"ndsSceneManagerRequest\(\s*\(u32\)\s*(nSCKind\w+)"),
)
ROW_RE = re.compile(r"\{\s*\(u8\)(nSCKind\w+)")
ALLOW = {"nSCKindStartup", "nSCKindNoController"} | {
    "nSCKindOpening" + n for n in (
        "Room", "Portraits", "Mario", "Donkey", "Samus", "Fox", "Link", "Yoshi",
        "Pikachu", "Kirby", "Run", "Yoster", "Cliff", "Standoff", "Yamabuki",
        "Clash", "Sector", "Jungle", "Newcomers")
}


def scan(text: str, tag: str, out: dict[str, set[str]]) -> None:
    for pat in WRITE_RES:
        for kind in pat.findall(text):
            out.setdefault(kind, set()).add(tag)


def census(repo: str) -> tuple[dict[str, list[str]], int]:
    decomp_src = os.path.join(repo, "decomp", "BattleShip-main", "decomp", "src")
    requested: dict[str, set[str]] = {}
    for tu in sorted(glob.glob(os.path.join(repo, "src", "import", "battleship_*.c"))):
        name = os.path.basename(tu)
        text = read(tu)
        scan(as_built(text), name, requested)
        for m in INC_RE.finditer(text):
            rel = m.group(1)
            src = os.path.join(decomp_src, rel)
            if os.path.exists(src):
                scan(as_built(patched_source(repo, rel, src)), name, requested)
    for tu in glob.glob(os.path.join(repo, "src", "nds", "*.c")) + glob.glob(os.path.join(repo, "src", "port", "*.c")):
        scan(read(tu), os.path.basename(tu), requested)
    rows = set(ROW_RE.findall(read(os.path.join(repo, "src", "port", "nds_scene_manager.c"))))
    missing = {k: sorted(v) for k, v in requested.items() if k not in rows and k not in ALLOW}
    return missing, len(requested)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--repo-root", default=os.path.join(os.path.dirname(__file__), "..", ".."))
    ap.add_argument("--strict", action="store_true", help="exit 1 when any kind is unrowed")
    args = ap.parse_args()
    repo = os.path.abspath(args.repo_root)
    missing, total = census(repo)
    for kind, tus in sorted(missing.items()):
        print(f"{kind:<32} unrowed, written by: {', '.join(tus[:5])}")
    print(f"SCENE_REGISTRY_CENSUS requested={total} unrowed={len(missing)}")
    return 1 if (args.strict and missing) else 0


if __name__ == "__main__":
    sys.exit(main())
