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
    ap.add_argument("profile", nargs="?", default="",
                    help="arm9-profile.csv from melonDS (whole match)")
    # THE PERCENTILE FORM. The positional profile is the whole match, and a
    # whole-match caller ranking is the wrong population for a gate that is a
    # percentile -- 2026-08-14 read the soft-float trio at 74,283 tk/fr whole
    # match and 99,762 on the 80 frames that SET P95, and closed the lane on the
    # first number. `census-marginal-frame-owners.py --reduce` already writes a
    # per-PC CSV carrying `marg_instructions`, and the instruction count at a
    # `bl <helper>` PC IS that site's exact dynamic call count on those frames.
    # Pair it with `--census` from the same reducer's `--census-out` so the rate
    # and the counts come from the SAME mask; mixing a marginal count with a
    # whole-match rate silently rescales every row.
    ap.add_argument("--pc-csv", default="",
                    help="reduced per-PC CSV from census-marginal-frame-owners"
                         " --reduce; call counts come from its mask instead of"
                         " a full profile pass")
    ap.add_argument("--mask", choices=("marginal", "all"), default="marginal",
                    help="which column set of --pc-csv to read (default "
                         "marginal = the frames that set P95)")
    ap.add_argument("--census", required=True, help="census.json for the same run")
    ap.add_argument("--dis", required=True, help="objdump -d of the matching ELF")
    ap.add_argument("--helpers", default="softfloat",
                    help="'softfloat' or a comma-separated symbol list")
    ap.add_argument("--thunk", action="append", default=[],
                    help="a=b: fold a's calls into b's divisor and charge them "
                         "at b's rate (e.g. __aeabi_fsub=__aeabi_fadd)")
    ap.add_argument("--top", type=int, default=22)
    ap.add_argument("--json", default="")
    ap.add_argument("--matrix-json", default="",
                    help="per-(caller, helper) call counts and attributed "
                         "cycles. --json collapses the helper axis, which is "
                         "the axis that decides whether a lane converts: a "
                         "MAC-heavy caller and a divide-heavy caller of equal "
                         "cost convert at rates that differ by more than 3x "
                         "(1.70 measured on the camera chain against a 5.14 "
                         "MAC prior). Rebuilding the matrix by hand needed the "
                         "whole tool a second time.")
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

    if bool(args.profile) == bool(args.pc_csv):
        ap.error("give exactly one of the positional profile or --pc-csv")

    calls = collections.Counter()
    per_helper = collections.Counter()
    if args.pc_csv:
        column = "marg_instructions" if args.mask == "marginal" \
            else "all_instructions"
        with open(args.pc_csv, newline="") as handle:
            for row in csv.DictReader(handle):
                site = sites.get(int(row["pc"], 16))
                if site is None:
                    continue
                n = int(row[column])
                calls[site] += n
                per_helper[site[1]] += n
    else:
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
    # Resolve through `aliases`: `__aeabi_fmul` and `__mulsf3` are one range with
    # one row, and which of the two the census names depends on the reducer.
    cycles = {}
    for s in census["symbols"]:
        cycles[s["name"]] = s["cycles"]
        for alias in s.get("aliases", ()):
            cycles.setdefault(alias, s["cycles"])
    work = census["total_cycles"] - cycles.get("armWaitForIrq", 0)
    frames = int(census.get("marginal_frames") or 0)
    if args.pc_csv and census.get("mask") != args.mask and args.mask == "marginal":
        print("WARNING: --mask marginal but --census is not a marginal census; "
              "the counts and the rate are on different populations.\n")
    if frames:
        print("basis     ticks/frame = cycles / (2 x {} frames) = cycles / {:,}"
              .format(frames, 2 * frames))

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

    tick_div = float(2 * frames) if frames else 0.0
    print("\n{:50s} {:>11s} {:>12s} {:>7s} {:>10s} {:>10s}".format(
        "caller", "calls", "attributed", "%work", "tk/fr", "calls/fr"))
    for name, cyc in attributed.most_common(args.top):
        tk = f"{cyc / tick_div:,.0f}" if tick_div else "-"
        cpf = f"{ncalls[name] / frames:,.1f}" if frames else "-"
        print("{:50s} {:>11,} {:>12,} {:>6.2f}% {:>10s} {:>10s}".format(
            name[:50], ncalls[name], int(cyc), 100.0 * cyc / work, tk, cpf))

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
    if args.matrix_json:
        matrix = collections.defaultdict(dict)
        for (caller, helper), n in calls.items():
            matrix[caller][helper] = {
                "calls": n,
                "cycles": int(n * rate.get(helper, 0.0)),
            }
        json.dump({
            "pc_csv": args.pc_csv,
            "profile": args.profile,
            "mask": args.mask,
            "marginal_frames": frames,
            "tick_divisor": 2 * frames,
            "rates": {h: rate.get(h, 0.0) for h in sorted(rate)},
            "callers": matrix,
        }, open(args.matrix_json, "w"), indent=1)
        print("wrote " + args.matrix_json)
    return 0


if __name__ == "__main__":
    sys.exit(main())
