#!/usr/bin/env python3
"""Attribute a leaf helper's cycles to the functions that CALL it.

The Task 37 census ranks symbols by self time, and for a leaf helper that is
exactly the wrong view: `__aeabi_fadd` is the largest non-idle symbol in the
whole profile, and "optimize __aeabi_fadd" is not a task anyone can do. What is
actionable is which callers drive it, and self time cannot say. Task 78 killed
the animation lever at 1.64x its target for precisely this reason -- see the
board's `self-time-is-not-a-subsystem-budget` rule.

Static call sites cannot answer it either. `ndsOpeningRoomRenderDLPreview` has
277 static calls to the soft-float helpers and *zero* cycles in a battle
profile, so a static ranking puts a function that never runs at the top.

The profile is per-PC and reports an instruction count for every PC, so the
count at a `bl <helper>` instruction IS that call site's exact dynamic call
count. Summing per caller gives exact call counts, and multiplying by the
helper's measured cycles-per-call attributes its cost. This costs no build and
no emulator run -- it reads a profile that already exists, the same way
`task37_census.py --split-by-symbol` does.

Two traps this handles, both of which produced wrong numbers by hand first:

  - `__aeabi_fsub` is a two-instruction thunk that falls through into
    `__aeabi_fadd`. Its self time is therefore ~1 cycle per call and all of its
    real cost is charged to fadd. `--thunk fsub=fadd` folds its calls into
    fadd's divisor, which moves fadd from a nonsensical 60.6 cycles per call to
    36.4.
  - A helper reached by a tail `b` rather than `bl` is still a call site, so
    both mnemonics are matched.

Usage:
  python scripts/analyze-leaf-helper-attribution.py \
      artifacts/performance/<run>/arm9-profile.csv \
      --census artifacts/performance/<run>/census.json \
      --dis <objdump -d output of the matching ELF> \
      --helpers softfloat --json artifacts/performance/<run>/softfloat.json

`--helpers softfloat` is the built-in set; pass a comma-separated list for
anything else (memset,memcpy,... works and is how the mem-op ranking was
produced).
"""

import argparse
import collections
import csv
import json
import re
import sys

SOFTFLOAT = [
    "__aeabi_fadd", "__aeabi_fsub", "__aeabi_fmul", "__aeabi_fdiv",
    "__aeabi_fcmpeq", "__aeabi_fcmplt", "__aeabi_fcmpgt", "__aeabi_fcmple",
    "__aeabi_fcmpge", "__aeabi_i2f", "__aeabi_ui2f", "__aeabi_f2iz",
    "__aeabi_l2f", "__floatsisf",
]

# Callers are grouped for the subsystem view. Order matters: first match wins.
GROUPS = [
    ("collision / stage MP", ("ndsMPFC", "ndsStageMP", "ndsMPSweep", "ndsMPCollision",
                              "mpCollision", "mpProcess", "gmCollision")),
    ("matrices / transform", ("syMatrix", "guMtx", "ndsRendererMtx",
                              "ndsRendererAdapterBuild", "ndsRendererLoadHardware")),
    ("CPU player AI", ("ftComputer",)),
    ("particles", ("lbParticle",)),
    ("renderer / other", ("ndsRenderer", "ndsSObj", "ndsPreview", "ndsOpening")),
    ("gameplay (other decomp)", ("ft", "gm", "lb", "sy", "wp", "it", "ef")),
]
ANIM_MARKS = ("AnimJoint", "Figatree", "MatAnim")
ANIM_PREFIX = ("gcPlay", "gcParse", "gcAdd", "ndsBaseGc", "ndsR2Cubic", "ftAnim")


def group_of(name):
    if any(m in name for m in ANIM_MARKS) or name.startswith(ANIM_PREFIX):
        return "animation evaluation"
    for label, prefixes in GROUPS:
        if name.startswith(prefixes):
            return label
    return "other"


def call_sites(dis_path, helpers):
    """Map every PC holding a call to one of `helpers` -> (caller, helper)."""
    func = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
    insn = re.compile(r"^\s*([0-9a-f]+):\s+[0-9a-f ]+\s+\S+")
    # `b` as well as `bl`/`blx`: a tail call is still a call site.
    call = re.compile(r"\b(?:bl|blx|b)\s+[0-9a-f]+ <([^>+]+)")
    sites = {}
    cur = None
    with open(dis_path, errors="ignore") as handle:
        for line in handle:
            m = func.match(line.rstrip("\n"))
            if m:
                cur = m.group(2)
                continue
            if cur is None:
                continue
            mi = insn.match(line)
            if not mi:
                continue
            mc = call.search(line)
            if mc and mc.group(1) in helpers:
                sites[int(mi.group(1), 16)] = (cur, mc.group(1))
    return sites


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("profile", help="arm9-profile.csv from melonDS")
    ap.add_argument("--census", required=True, help="census.json for the same run")
    ap.add_argument("--dis", required=True, help="objdump -d of the matching ELF")
    ap.add_argument("--helpers", default="softfloat",
                    help="'softfloat' or a comma-separated symbol list")
    ap.add_argument("--thunk", action="append", default=[],
                    help="a=b: fold a's calls into b's divisor and charge them "
                         "at b's rate (e.g. __aeabi_fsub=__aeabi_fadd)")
    ap.add_argument("--top", type=int, default=22)
    ap.add_argument("--json", default="")
    args = ap.parse_args(argv)

    helpers = (SOFTFLOAT if args.helpers == "softfloat"
               else [s.strip() for s in args.helpers.split(",") if s.strip()])
    thunks = {}
    for pair in args.thunk:
        if "=" not in pair:
            ap.error("--thunk expects a=b, got %r" % pair)
        src, dst = pair.split("=", 1)
        thunks[src.strip()] = dst.strip()
    if args.helpers == "softfloat" and not thunks:
        thunks["__aeabi_fsub"] = "__aeabi_fadd"

    sites = call_sites(args.dis, set(helpers))
    if not sites:
        sys.exit("no call sites found -- is --dis the objdump of the profiled ELF?")

    calls = collections.Counter()
    per_helper = collections.Counter()
    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        next(rows)
        for row in rows:
            site = sites.get(int(row[1], 16))
            if site is None:
                continue
            n = int(row[4])
            calls[site] += n
            per_helper[site[1]] += n

    census = json.load(open(args.census))
    cycles = {s["name"]: s["cycles"] for s in census["symbols"]}
    work = census["total_cycles"] - cycles.get("armWaitForIrq", 0)

    # A thunk's callers are charged at its target's rate, and its calls count
    # toward the target's divisor -- otherwise the target's per-call cost is
    # inflated by every call that reached it through the thunk.
    divisor = collections.Counter()
    for helper, n in per_helper.items():
        divisor[thunks.get(helper, helper)] += n
    rate = {h: (cycles.get(h, 0) / divisor[h] if divisor[h] else 0.0)
            for h in divisor}
    for src, dst in thunks.items():
        rate[src] = rate.get(dst, 0.0)

    attributed = collections.Counter()
    ncalls = collections.Counter()
    for (caller, helper), n in calls.items():
        attributed[caller] += n * rate.get(helper, 0.0)
        ncalls[caller] += n
    total = sum(attributed.values())

    print("non-idle work {:,} cycles (armWaitForIrq {:,} excluded)".format(
        work, cycles.get("armWaitForIrq", 0)))
    print("attributed    {:,} cycles = {:.2f}% of non-idle\n".format(
        int(total), 100.0 * total / work))

    print("{:24s} {:>12s} {:>12s} {:>9s}".format(
        "helper", "calls", "cycles", "cyc/call"))
    for helper, n in per_helper.most_common():
        tag = " (thunk -> %s)" % thunks[helper] if helper in thunks else ""
        print("{:24s} {:>12,} {:>12,} {:>9.1f}{}".format(
            helper, n, cycles.get(helper, 0), rate.get(helper, 0.0), tag))

    print("\n{:50s} {:>11s} {:>12s} {:>7s} {:>12s}".format(
        "caller", "calls", "attributed", "%work", "self"))
    for name, cyc in attributed.most_common(args.top):
        print("{:50s} {:>11,} {:>12,} {:>6.2f}% {:>12,}".format(
            name[:50], ncalls[name], int(cyc), 100.0 * cyc / work,
            cycles.get(name, 0)))

    groups = collections.Counter()
    gcount = collections.Counter()
    for name, cyc in attributed.items():
        groups[group_of(name)] += cyc
        gcount[group_of(name)] += 1
    print("\n{:26s} {:>13s} {:>7s} {:>6s}".format(
        "subsystem", "cycles", "%work", "fns"))
    for label, cyc in groups.most_common():
        print("{:26s} {:>13,} {:>6.2f}% {:>6d}".format(
            label, int(cyc), 100.0 * cyc / work, gcount[label]))

    if args.json:
        json.dump({
            "profile": args.profile,
            "non_idle_cycles": work,
            "attributed_cycles": int(total),
            "helpers": {h: {"calls": n, "cycles": cycles.get(h, 0),
                            "cycles_per_call": rate.get(h, 0.0)}
                        for h, n in per_helper.items()},
            "callers": {n: {"calls": ncalls[n], "cycles": int(c)}
                        for n, c in attributed.items()},
            "subsystems": {k: int(v) for k, v in groups.items()},
        }, open(args.json, "w"), indent=1)
        print("\nwrote " + args.json)
    return 0


if __name__ == "__main__":
    sys.exit(main())
