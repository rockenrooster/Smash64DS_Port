#!/usr/bin/env python3
"""Attribute a dispatcher's cost to its children, inclusively, off an existing profile.

The tick-HUD buckets are *inclusive* timers wrapped around a handful of gateway
functions, but a census reports *self* time -- and a gateway's self time is
nothing. `ftMainProcUpdateInterrupt` is 0.04% of non-idle by itself while its
`SINT` bucket is the single largest over-gate discriminator at +88,082. Asking
"which child causes that" is the question `docs/optimization/SRC_CPI_OPTIMIZATION.md`
calls "the big one", and nothing in the toolchain answered it.

This builds the static call graph out of a disassembly, walks the subtree under a
named root, and sums each direct child's reachable set -- both total cycles and
the over-gate discrimination from `census.json`'s `--split-over-gate` rows.

The honest part is the sharing. `memcpy`, the soft-float helpers and most small
utilities are reachable from nearly every child, so charging them to one is
fiction. A symbol reachable from more than one direct child is reported in its
own SHARED row and never folded into a child's total, which means the per-child
numbers are *lower bounds on exclusive cost* rather than a partition that
happens to add up. Read the shared pool before concluding a child is cheap.

Two further limits, stated rather than papered over:

  - The graph is STATIC. An indirect call through a function pointer -- which is
    how BattleShip dispatches fighter status procs -- is invisible here, so a
    child whose real work hangs off a table will look small. `--extra-edge`
    adds a known dynamic edge by hand.
  - Reachability is not execution. A symbol reachable from a child may not have
    run on the frames in question; the cycles are real, the attribution to that
    child is an upper bound on how much of it that child could explain.

`--child-group` exists because a DISPATCH SLOT is one child, not N. Adding the
60 enumerated `proc_update`/`proc_interrupt`/`proc_passive` targets as 60
`--extra-edge`s makes every symbol two of them share look SHARED and reports
sixty ~zero rows instead of one callback row; the 2026-08-15 callback census
had to compute that union by hand for exactly this reason. A group is also how
a source-level child the linker INLINED gets its name back:
`ftMainPlayAnimEventsAll` is inlined into the interrupt proc, so the graph shows
its callees `ftMainPlayAnim` and `ftMainUpdateMotionEventsAll` as direct
children of the root and the name the design speaks in has no row.

`--census-whole` prints a second cycles column from another census of the same
ELF. The intended pair is a marginal-mask census (`census-marginal-frame-owners
--census-out`) as `--census` and the whole-match one as `--census-whole`, which
makes each child's CONCENTRATION visible in the same table instead of assumed --
`a-flat-lane-is-the-best-converting-lane` and its opposite error both came from
reading one of those two numbers as the other.

Usage:
  python scripts/analyze-subtree-attribution.py \
      --dis <objdump -d of the matching ELF> \
      --census artifacts/performance/<run>/census.json \
      --root battleship_ftMainProcUpdateInterrupt \
      [--extra-edge caller=callee] [--child-group NAME=sym,sym] \
      [--census-whole <whole-match census.json>] [--frames N] [--top 20]
"""

from __future__ import annotations

import argparse
import collections
import json
import re
import sys

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
CALL = re.compile(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2,8} )+\s*"
                  r"(bl|blx|b|bx)(?:\.\w)?\s+[0-9a-f]+ <([^>+]+)(\+0x[0-9a-f]+)?>")


def load_graph(path):
    """-> {caller: set(callee)}. Tail branches out of the function count."""
    graph = collections.defaultdict(set)
    cur = None
    for line in open(path, errors="ignore"):
        m = FUNC.match(line.rstrip("\n"))
        if m:
            cur = m.group(2)
            graph.setdefault(cur, set())
            continue
        if cur is None:
            continue
        c = CALL.match(line)
        if not c:
            continue
        target = c.group(2)
        # An internal branch (b .L1 inside the same function) is not an edge.
        if target != cur:
            graph[cur].add(target)
    return graph


def reachable(graph, start, stop_at=None):
    seen, stack = set(), [start]
    while stack:
        n = stack.pop()
        if n in seen or (stop_at and n in stop_at and n != start):
            continue
        seen.add(n)
        stack.extend(graph.get(n, ()))
    return seen


FUNC_ADDR = re.compile(r"^([0-9a-f]+) <(.+?)>:$")


def load_addresses(path):
    """-> {name: entry address}. objdump prints the even address for Thumb."""
    out = {}
    for line in open(path, errors="ignore"):
        m = FUNC_ADDR.match(line.rstrip("\n"))
        if m:
            out.setdefault(m.group(2), int(m.group(1), 16) & ~1)
    return out


def load_calls(pc_csv):
    """-> {address: (whole calls, marginal calls)} from ENTRY PCs.

    A function's first instruction executes exactly once per call, so the
    reduced CSV's instruction count at the entry address IS the call count
    (`entry-pc-gives-exact-call-counts`). This is the only measured quantity
    that can split a shared descendant between two possible parents, which is
    precisely what the static SHARED row cannot do.
    """
    import csv
    out = {}
    with open(pc_csv, newline="") as handle:
        for row in csv.DictReader(handle):
            out[int(row["pc"], 16) & ~1] = (int(row["all_instructions"]),
                                            int(row["marg_instructions"]))
    return out


def load_cycles(path):
    """-> (per-symbol cycles counter, non-idle total). Aliases share one row."""
    cen = json.load(open(path))
    cycles = collections.Counter()
    for s in cen["symbols"]:
        cycles[s["name"]] += s["cycles"]
        for a in s.get("aliases", ()):
            cycles.setdefault(a, s["cycles"])
    idle = cycles.get("armWaitForIrq", 0)
    return cen, cycles, cen["total_cycles"] - idle


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--dis", required=True)
    ap.add_argument("--census", required=True)
    ap.add_argument("--census-whole",
                    help="second census.json of the same ELF, printed as an "
                         "extra column so concentration is visible")
    ap.add_argument("--root", required=True)
    ap.add_argument("--extra-edge", action="append", default=[],
                    help="caller=callee, for a dynamic call the graph cannot see")
    ap.add_argument("--child-group", action="append", default=[],
                    help="NAME=sym[,sym...] -- one logical child. Members that "
                         "are direct children are merged; members that are not "
                         "are dynamic dispatch targets the static graph cannot "
                         "see. A dispatch slot is ONE child, not one per target.")
    ap.add_argument("--frames", type=int, default=0,
                    help="frames in --census; cycles/(2*frames) = ticks/frame")
    ap.add_argument("--whole-frames", type=int, default=0,
                    help="frames in --census-whole. It is a DIFFERENT divisor "
                         "(80 marginal against 1601 match) and dividing both "
                         "columns by one of them is how a concentration column "
                         "silently becomes a ratio of populations.")
    ap.add_argument("--callers", type=int, default=0,
                    help="also list every subtree symbol at or above this many "
                         "MARGINAL ticks/frame with its program-wide callers "
                         "and its exact call rate. Needs --pc-csv.")
    ap.add_argument("--pc-csv",
                    help="reduced per-PC CSV from census-marginal-frame-owners "
                         "--reduce; supplies exact call counts from entry PCs")
    ap.add_argument("--top", type=int, default=25)
    args = ap.parse_args(argv)

    graph = load_graph(args.dis)
    for spec in args.extra_edge:
        caller, _, callee = spec.partition("=")
        graph[caller].add(callee)

    if args.root not in graph:
        sys.exit("root {!r} not in disassembly".format(args.root))

    cen, cycles, non_idle = load_cycles(args.census)
    whole_cycles = None
    if args.census_whole:
        _, whole_cycles, _ = load_cycles(args.census_whole)
    delta = {r["name"]: r["delta_cycles_per_frame"]
             for r in cen.get("split", {}).get("rows", ())}

    # A group's members leave the plain child list; a member the graph never saw
    # as a direct child is a dynamic target and joins anyway.
    grouped = {}
    claimed = set()
    for spec in args.child_group:
        label, _, members = spec.partition("=")
        names = [m for m in members.split(",") if m]
        unknown = [m for m in names if m not in graph]
        if unknown:
            sys.exit("--child-group {}: not in disassembly: {}".format(
                label, ", ".join(unknown)))
        grouped[label] = names
        claimed.update(names)

    children = [c for c in sorted(graph[args.root]) if c not in claimed]
    sets = {c: reachable(graph, c) for c in children}
    for label, names in grouped.items():
        sets[label] = set().union(*(reachable(graph, n) for n in names))
    labels = children + sorted(grouped)

    owner = collections.Counter()
    for c in labels:
        for sym in sets[c]:
            owner[sym] += 1

    whole = reachable(graph, args.root) | set().union(
        *(sets[c] for c in labels)) if labels else reachable(graph, args.root)
    # The root's own body belongs to no child and must not vanish from the
    # reconciliation; it is the one row a subtree census always leaves implicit.
    self_cyc = cycles.get(args.root, 0)
    print("root {} -- subtree reaches {:,} symbols, {} children "
          "({} direct + {} grouped)\n".format(
              args.root, len(whole), len(labels), len(children), len(grouped)))

    div = 2.0 * args.frames if args.frames else 0.0
    wdiv = 2.0 * (args.whole_frames or args.frames) if args.frames else 0.0

    def tk(value):
        return "{:>11,.0f}".format(value / div) if div else "{:>11s}".format("-")

    rows = []
    for c in labels:
        excl = [s for s in sets[c] if owner[s] == 1]
        cyc = sum(cycles.get(s, 0) for s in excl)
        rows.append((
            cyc,
            sum(delta.get(s, 0.0) for s in excl),
            len(excl), c,
            sum(whole_cycles.get(s, 0) for s in excl) if whole_cycles else 0))
    rows.sort(reverse=True)

    shared = [s for s in whole if owner.get(s, 0) > 1]
    sh_cyc = sum(cycles.get(s, 0) for s in shared)
    sh_del = sum(delta.get(s, 0.0) for s in shared)
    sh_whole = sum(whole_cycles.get(s, 0) for s in shared) if whole_cycles else 0

    head = "{:40s} {:>13s} {:>7s} {:>11s} {:>6s}".format(
        "child (EXCLUSIVE subtree only)", "cycles", "%nonidle", "tk/fr", "syms")
    if whole_cycles:
        head += " {:>11s} {:>6s}".format("whole tk/fr", "conc")
    print(head)

    def line(name, cyc, dlt, n, whole_cyc):
        out = "{:40s} {:>13,} {:>6.2f}% {} {:>6,}".format(
            name[:40], cyc, 100.0 * cyc / non_idle, tk(cyc), n)
        if whole_cycles:
            wtk = "{:>11,.0f}".format(whole_cyc / wdiv) if wdiv else "{:>11s}".format("-")
            conc = ("{:>6.2f}".format((cyc / div) / (whole_cyc / wdiv))
                    if (wdiv and whole_cyc) else "{:>6s}".format("-"))
            out += " {} {}".format(wtk, conc)
        return out

    print(line("(root body, no child)", self_cyc, delta.get(args.root, 0.0), 1,
               whole_cycles.get(args.root, 0) if whole_cycles else 0))
    for cyc, dlt, n, name, wcyc in rows[:args.top]:
        print(line(name, cyc, dlt, n, wcyc))
    print(line("SHARED among children (not attributable)", sh_cyc, sh_del,
               len(shared), sh_whole))

    tot_c = sum(r[0] for r in rows) + sh_cyc + self_cyc
    tot_w = (sum(r[4] for r in rows) + sh_whole
             + (whole_cycles.get(args.root, 0) if whole_cycles else 0))
    print()
    print(line("SUBTREE TOTAL", tot_c, 0.0, 0, tot_w))
    print("\nthe SHARED row is the accuracy limit: a symbol reachable from two "
          "children\nis charged to neither. Rank children by their exclusive "
          "rows, then read SHARED\nbefore believing any child is cheap. The "
          "totals are a CENSUS of self time over a\nreachable set, not the "
          "root's inclusive cost: a symbol also called from outside\nthe root "
          "contributes all of its cycles here, so SUBTREE TOTAL is an UPPER "
          "bound\non the root and must be reconciled against a bracket, never "
          "quoted as one.")

    if args.callers:
        if not args.pc_csv:
            sys.exit("--callers needs --pc-csv for the entry-PC call counts")
        addr = load_addresses(args.dis)
        calls = load_calls(args.pc_csv)
        callers = collections.defaultdict(set)
        for caller, callees in graph.items():
            for callee in callees:
                callers[callee].add(caller)
        print("\n\nSUBTREE SYMBOLS >= {:,} marginal tk/fr -- with the two "
              "things that\nresolve a SHARED row: the exact call rate, and who "
              "can call it PROGRAM-WIDE.\n'ext' marks a caller outside this "
              "root's subtree, i.e. cost this census\ncharges to the root that "
              "the root did not necessarily cause.".format(args.callers))
        print("\n{:38s}{:>11s}{:>11s}{:>6s}{:>10s}{:>10s}{:>7s}  {}".format(
            "symbol", "marg tk/fr", "whole", "conc", "m calls", "calls",
            "CALL x", "children reaching | callers (ext marked *)"))
        listing = sorted(((cycles.get(s, 0), s) for s in whole), reverse=True)
        for cyc, sym in listing:
            marg_tk = cyc / div if div else 0.0
            if marg_tk < args.callers:
                break
            wcyc = whole_cycles.get(sym, 0) if whole_cycles else 0
            wtk = wcyc / wdiv if wdiv else 0.0
            entry = calls.get(addr.get(sym, -1), (0, 0))
            mc = entry[1] / args.frames if args.frames else 0.0
            ac = entry[0] / args.whole_frames if args.whole_frames else 0.0
            reach = [c for c in labels if sym in sets[c]]
            outside = sorted(c for c in callers.get(sym, ())
                             if c not in whole and c != args.root)
            inside = sorted(c for c in callers.get(sym, ()) if c in whole)
            who = ",".join(inside[:4]) + ("+{}".format(len(inside) - 4)
                                          if len(inside) > 4 else "")
            if outside:
                who += " *" + ",".join(outside[:3]) + (
                    "+{}".format(len(outside) - 3) if len(outside) > 3 else "")
            print("{:38s}{:>11,.0f}{:>11,.0f}{:>6}{:>10.2f}{:>10.2f}{:>7}  "
                  "{} | {}".format(
                      sym[:38], marg_tk, wtk,
                      "{:.2f}".format(marg_tk / wtk) if wtk else "-",
                      mc, ac,
                      "{:.2f}".format(mc / ac) if ac else "-",
                      "+".join(reach) if reach else "(root only)", who))
    return 0


if __name__ == "__main__":
    sys.exit(main())
