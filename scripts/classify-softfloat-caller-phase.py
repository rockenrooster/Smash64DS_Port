#!/usr/bin/env python3
"""Classify each soft-float caller as draw-only, simulation-only, shared, or unresolved.

`analyze-leaf-helper-attribution.py` says WHICH functions drive the soft-float
library and what they cost. It cannot say whether converting one is allowed:
`PROJECT_GOAL.md` permits rendering-side approximation and forbids touching
simulation state, and a source-name guess ("it starts with `ndsRenderer`, so it
is draw") has already mis-sorted symbols here -- `syMatrixLookAtF` is camera,
`gmCollisionGetWorldPosition` is called from both phases.

The answer comes from the LINKED ELF, not from names. Every `bl`/`blx`/`b`
into a symbol is an edge; walking the reverse graph up from a soft-float caller
gives the set of ROOTS (functions with no static caller) that can reach it. A
caller whose roots are all draw roots is draw-only; all simulation roots,
simulation-only; both, shared; none, unresolved (reached only through a function
pointer, which a static graph cannot follow -- `gcRunAll` dispatches every GObj
`func_run`/`func_display` that way, so "unresolved" is a large and honest class
here, not a failure).

Roots are labelled from a small table of *entry points*, not from the caller's
own name -- that is the whole point. Anything a root table does not name is
reported as UNKNOWN-ROOT so the residue is visible rather than silently folded
into `shared`.

Usage:
  python scripts/classify-softfloat-caller-phase.py \
      --dis <objdump -d of the profiled ELF> \
      --callers <softfloat-callers json from analyze-leaf-helper-attribution> \
      [--frames 80] [--min-tk 50] [--json out.json]
"""

from __future__ import annotations

import argparse
import collections
import json
import re
import sys

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+[0-9a-f ]+\s+(\S+)\s*(.*)$")
TARGET = re.compile(r"\b(?:bl|blx|b|bx)(?:\.\w+)?\s+[0-9a-f]+ <([^>+]+)")

# Entry points of the DS frame, named once. These are the only names this tool
# trusts; every other classification is derived by reachability from them.
#
# DRAW: the per-frame present/draw pipeline. `ndsRendererDraw*` /
# `ndsRendererPresent*` / the native stage and fighter emitters, plus decomp's
# own display half (`gcDrawAll` and the `*ProcDisplay` / `*Draw*` callbacks the
# GObj display list dispatches).
#
# SIM: `gcRunAll` -- proven in `src/import/battleship_sys_objman.c` to be the
# sole gateway to the whole simulation inside the SRC bracket -- plus the fighter
# `Proc*` callbacks and the map/collision entry points it dispatches.
# Ordered, and the order is the whole design. A GObj callback's ROLE is in its
# suffix, not its module prefix: `efManagerShieldProcDisplay` is a DISPLAY
# callback even though everything else called `efManager*` is simulation, and
# `grWallpaperCommonProcUpdate` is a state UPDATE even though "Wallpaper" reads
# like presentation. Sorting by prefix first put the entire renderer/matrix
# cluster into `shared` on the strength of two mislabelled roots, so the suffix
# rules are evaluated before any prefix rule.
SIM_CALLBACK_MARK = ("ProcUpdate", "ProcMap", "ProcPhysics", "ProcInterrupt",
                     "ProcCollision", "ProcParams", "ProcSearchHit",
                     "SetStatus", "CheckInterrupt", "FuncRun", "FuncCamera",
                     "ProcDefault", "RunAll")
DRAW_CALLBACK_MARK = ("ProcDisplay", "FuncDisplay", "FuncLights", "Draw",
                      "Render", "Present", "DLHead", "DObjTree")

DRAW_ROOT_PREFIX = (
    "ndsRenderer", "ndsFighterMarioFox", "ndsStageGCDraw", "ndsDrawSObj",
    "ndsDrawLayeredSObj", "gcDraw", "ftDisplay", "ndsBaseFTDisplay",
    "efDisplay", "ndsParticleDraw", "lbParticleDraw",
)
SIM_ROOT_PREFIX = (
    "gcRunAll", "ndsBaseGcRunAll", "gcRunGObj", "ftMain", "ftPublic",
    "ftComputer", "mpCommon", "mpCollision", "mpProcess", "wpProcess",
    "efManager", "ifCommon", "gmCamera", "gmCollision", "grPupupu",
    "ftManager", "ndsBaseFTCommon", "ftCommon", "ndsMPCommon", "wpManager",
    "wpMain", "wpMario", "wpFox", "ftPhysics", "ftParam", "ndsR2Battle",
    "battleship_ft", "grWallpaper", "ndsStageMP", "ndsMP",
)

# Roots that dispatch BOTH halves through a function pointer and therefore say
# nothing about phase. Named rather than folded into UNKNOWN so the report can
# separate "reached only through the task dispatcher" from "no root at all".
DISPATCH_ROOT = ("syTaskmanRunTask", "syMainThread", "gcRunGObjProcess",
                 ".vectors", "syScheduler")


def label_root(name: str) -> str:
    """DRAW / SIM / DISPATCH / UNKNOWN for a graph root, by role then module."""
    if name.startswith(DISPATCH_ROOT):
        return "DISPATCH"
    for mark in SIM_CALLBACK_MARK:
        if mark in name:
            return "SIM"
    for mark in DRAW_CALLBACK_MARK:
        if mark in name:
            return "DRAW"
    if name.startswith(DRAW_ROOT_PREFIX):
        return "DRAW"
    if name.startswith(SIM_ROOT_PREFIX):
        return "SIM"
    return "UNKNOWN"


def read_graph(dis_path: str):
    """(callers[callee] -> {caller}, functions) from an objdump -d listing."""
    callers = collections.defaultdict(set)
    functions = set()
    cur = None
    with open(dis_path, errors="ignore") as handle:
        for line in handle:
            m = FUNC.match(line.rstrip("\n"))
            if m:
                cur = m.group(2)
                functions.add(cur)
                continue
            if cur is None:
                continue
            mi = INSN.match(line)
            if not mi:
                continue
            mt = TARGET.search(line)
            if not mt:
                continue
            callee = mt.group(1)
            if callee != cur:
                callers[callee].add(cur)
    return callers, functions


def live_symbols(pc_csv: str, nm_path: str, column: str) -> set:
    """Names that executed at least one instruction in the profile.

    A static reverse walk otherwise drags in whole scenes that never run in this
    match -- `mnTitleLogoFireProcDisplay` and `mvOpeningDonkeyFuncLights` are
    static ancestors of the particle and light code, and both execute ZERO times
    in a battle profile. Leaving them in turns "draw-only" into "shared" for
    reasons that have nothing to do with the battle. Pruning to executed code is
    the same discipline `analyze-leaf-helper-attribution.py` documents for its
    own ranking.
    """
    import subprocess
    starts = []
    text = subprocess.run([nm_path, "-nS", "--defined-only", pc_csv[:0] or
                           NM_ELF[0]], capture_output=True, text=True,
                          check=True).stdout
    for line in text.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[2] in ("t", "T"):
            starts.append((int(parts[0], 16), int(parts[1], 16), parts[3]))
    starts.sort()
    import bisect
    keys = [s[0] for s in starts]
    live = set()
    with open(pc_csv, newline="") as handle:
        import csv as _csv
        for row in _csv.DictReader(handle):
            if int(row[column]) <= 0:
                continue
            pc = int(row["pc"], 16)
            i = bisect.bisect_right(keys, pc) - 1
            if i >= 0:
                addr, size, name = starts[i]
                if size == 0 or pc < addr + size:
                    live.add(name)
    return live


NM_ELF = [""]


def roots_of(name, callers, cache, stack=()):
    """Every static ancestor of `name` that has no caller of its own."""
    if name in cache:
        return cache[name]
    if name in stack:            # recursion: contributes nothing new
        return frozenset()
    up = callers.get(name)
    if not up:
        out = frozenset({name})
    else:
        acc = set()
        for parent in up:
            acc |= roots_of(parent, callers, cache, stack + (name,))
        out = frozenset(acc) if acc else frozenset({name})
    cache[name] = out
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dis", required=True)
    ap.add_argument("--callers", required=True)
    ap.add_argument("--frames", type=int, default=80)
    ap.add_argument("--min-tk", type=float, default=50.0)
    ap.add_argument("--json", default="")
    ap.add_argument("--pc-csv", default="",
                    help="reduced per-PC CSV; prunes the graph to code that "
                         "actually executed in this match")
    ap.add_argument("--elf", default="", help="ELF matching --pc-csv")
    ap.add_argument("--nm", default="arm-none-eabi-nm")
    args = ap.parse_args()

    callers, functions = read_graph(args.dis)
    if len(functions) < 1000:
        sys.exit("FAIL: only %d function labels parsed -- objdump -d must keep "
                 "the raw-byte column (do NOT pass --no-show-raw-insn)"
                 % len(functions))

    if bool(args.pc_csv) != bool(args.elf):
        ap.error("--pc-csv and --elf go together")
    if args.pc_csv:
        NM_ELF[0] = args.elf
        live = live_symbols(args.pc_csv, args.nm, "all_instructions")
        print("live symbols in this match: %d of %d linked\n"
              % (len(live), len(functions)))
        callers = {k: {c for c in v if c in live}
                   for k, v in callers.items()}
        callers = {k: v for k, v in callers.items() if v}

    data = json.load(open(args.callers))
    rows = data["callers"]
    div = float(2 * args.frames)

    cache: dict = {}
    out = []
    for name, row in rows.items():
        if row["cycles"] <= 0:
            continue
        rts = roots_of(name, callers, cache)
        labels = {label_root(r) for r in rts}
        hard = labels - {"DISPATCH", "UNKNOWN"}
        if hard == {"DRAW"}:
            phase = "draw-only" if labels == {"DRAW"} else "draw+dispatch"
        elif hard == {"SIM"}:
            phase = "sim-only" if labels == {"SIM"} else "sim+dispatch"
        elif hard == {"DRAW", "SIM"}:
            phase = "shared"
        else:
            phase = "unresolved"
        out.append({
            "caller": name,
            "phase": phase,
            "calls": row["calls"],
            "cycles": row["cycles"],
            "tk_per_frame": row["cycles"] / div,
            "calls_per_frame": row["calls"] / float(args.frames),
            "roots": sorted(rts)[:8],
            "root_count": len(rts),
        })
    out.sort(key=lambda r: -r["cycles"])

    print("basis  ticks/frame = cycles / (2 x %d frames)" % args.frames)
    print("%-52s %-13s %10s %10s %9s" %
          ("caller", "phase", "tk/fr", "calls/fr", "roots"))
    for r in out:
        if r["tk_per_frame"] < args.min_tk:
            continue
        print("%-52s %-13s %10.0f %10.1f %9d" %
              (r["caller"][:52], r["phase"], r["tk_per_frame"],
               r["calls_per_frame"], r["root_count"]))

    tot = collections.Counter()
    cnt = collections.Counter()
    for r in out:
        tot[r["phase"]] += r["cycles"]
        cnt[r["phase"]] += 1
    grand = sum(tot.values())
    print("\n%-14s %12s %8s %6s" % ("phase", "tk/fr", "share", "fns"))
    for phase, cyc in tot.most_common():
        print("%-14s %12.0f %7.1f%% %6d" %
              (phase, cyc / div, 100.0 * cyc / grand, cnt[phase]))
    print("%-14s %12.0f %7.1f%% %6d" % ("TOTAL", grand / div, 100.0, len(out)))

    if args.json:
        json.dump({"frames": args.frames, "rows": out,
                   "phase_cycles": dict(tot)}, open(args.json, "w"), indent=1)
        print("\nwrote %s" % args.json)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
