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

Usage:
  python scripts/analyze-subtree-attribution.py \
      --dis <objdump -d of the matching ELF> \
      --census artifacts/performance/<run>/census.json \
      --root battleship_ftMainProcUpdateInterrupt \
      [--extra-edge caller=callee] [--top 20]
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


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--dis", required=True)
    ap.add_argument("--census", required=True)
    ap.add_argument("--root", required=True)
    ap.add_argument("--extra-edge", action="append", default=[],
                    help="caller=callee, for a dynamic call the graph cannot see")
    ap.add_argument("--top", type=int, default=25)
    args = ap.parse_args(argv)

    graph = load_graph(args.dis)
    for spec in args.extra_edge:
        caller, _, callee = spec.partition("=")
        graph[caller].add(callee)

    if args.root not in graph:
        sys.exit("root {!r} not in disassembly".format(args.root))

    cen = json.load(open(args.census))
    cycles = collections.Counter()
    for s in cen["symbols"]:
        cycles[s["name"]] += s["cycles"]
        for a in s.get("aliases", ()):
            cycles.setdefault(a, s["cycles"])
    delta = {r["name"]: r["delta_cycles_per_frame"]
             for r in cen.get("split", {}).get("rows", ())}
    idle = cycles.get("armWaitForIrq", 0)
    non_idle = cen["total_cycles"] - idle

    children = sorted(graph[args.root])
    sets = {c: reachable(graph, c) for c in children}

    owner = collections.Counter()
    for c in children:
        for sym in sets[c]:
            owner[sym] += 1

    whole = reachable(graph, args.root)
    print("root {} -- subtree reaches {:,} symbols, {} direct children\n".format(
        args.root, len(whole), len(children)))

    rows = []
    for c in children:
        excl = [s for s in sets[c] if owner[s] == 1]
        rows.append((
            sum(cycles.get(s, 0) for s in excl),
            sum(delta.get(s, 0.0) for s in excl),
            len(excl), c))
    rows.sort(reverse=True)

    shared = [s for s in whole if owner.get(s, 0) > 1]
    sh_cyc = sum(cycles.get(s, 0) for s in shared)
    sh_del = sum(delta.get(s, 0.0) for s in shared)

    print("{:44s} {:>13s} {:>7s} {:>12s} {:>6s}".format(
        "direct child (EXCLUSIVE subtree only)", "cycles", "%nonidle",
        "over-gate", "syms"))
    for cyc, dlt, n, name in rows[:args.top]:
        print("{:44s} {:>13,} {:>6.2f}% {:>12,.0f} {:>6,}".format(
            name[:44], cyc, 100.0 * cyc / non_idle, dlt, n))
    print("{:44s} {:>13,} {:>6.2f}% {:>12,.0f} {:>6,}".format(
        "SHARED among children (not attributable)", sh_cyc,
        100.0 * sh_cyc / non_idle, sh_del, len(shared)))

    tot_c = sum(r[0] for r in rows) + sh_cyc
    tot_d = sum(r[1] for r in rows) + sh_del
    print("\n{:44s} {:>13,} {:>6.2f}% {:>12,.0f}".format(
        "SUBTREE TOTAL", tot_c, 100.0 * tot_c / non_idle, tot_d))
    print("\nthe SHARED row is the accuracy limit: a symbol reachable from two "
          "children\nis charged to neither. Rank children by their exclusive "
          "rows, then read SHARED\nbefore believing any child is cheap.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
