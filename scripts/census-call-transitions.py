#!/usr/bin/env python3
"""Phase 3: the weighted caller->callee transition graph, and the clusters in it.

The 2026-08-14 placement report claimed a hot callee's lines are "evicted between
calls regardless" of how close the caller is linked. That claim is exactly what
this graph is for, and the report asserted it without building the graph. It is
withdrawn until this says otherwise.

Edges are exact, not estimated: every `bl <callee>` is an instruction with its
own address, and the profile carries an execution count per address, so summing
counts per call site gives the true dynamic edge weight. Static fan-out is never
consulted.

What the graph is used for here:

  * CLUSTERS -- connected components over edges above a weight floor, which is
    the closest static proxy for "executes near in time".
  * SPAN -- the address range a cluster currently occupies, and the fraction of
    that range that is NOT cluster members (i.e. cold text interleaved between
    hot members, Phase 4 pattern 3).
  * SET PRESSURE -- distinct 32-byte lines per I-cache set contributed by the
    cluster alone, which is the quantity a reordering could actually change.

A cluster whose OWN footprint already exceeds the 8 KB cache cannot be helped by
reordering its members; one that fits, but is currently spread across a span far
larger than itself, is the case where placement pays.

Geometry is the reference emulator's, verified: 8192 B, 32 B lines, 4-way,
64 sets, set = (addr >> 5) & 63, round-robin replacement
(melonDS-Accurate src/CP15_Constants.h, src/CP15.cpp).

UNITS: profile cycles. Two profile cycles = one project tick.
"""

from __future__ import annotations

import argparse
import collections
import csv
import re

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2,8} )+)\s*(\S+)\s*(.*)$")
CALL = re.compile(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2,8} )+\s*bl(?:x)?(?:\.\w)?\s+"
                  r"[0-9a-f]+\s+<([^>+]+)(?:\+[^>]*)?>")

ICACHE_BYTES, LINE, WAYS = 8192, 32, 4
SETS = ICACHE_BYTES // LINE // WAYS          # 64
ITCM_LO, ITCM_HI = 0x01FF8000, 0x02000000


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--regions", type=int, required=True)
    ap.add_argument("--min-edge", type=float, default=5.0,
                    help="edge floor in calls/frame for cluster membership")
    ap.add_argument("--top-edges", type=int, default=25)
    args = ap.parse_args()

    owner: dict[int, str] = {}
    start: dict[str, int] = {}
    end: dict[str, int] = {}
    sites: dict[int, tuple[str, str]] = {}
    cur = None
    with open(args.dis, errors="ignore") as handle:
        for raw in handle:
            m = FUNC.match(raw.rstrip("\n"))
            if m:
                cur = m.group(2)
                start.setdefault(cur, int(m.group(1), 16))
                continue
            if cur is None:
                continue
            mi = INSN.match(raw)
            if not mi:
                continue
            pc = int(mi.group(1), 16)
            owner[pc] = cur
            end[cur] = max(end.get(cur, 0), pc + len(mi.group(2).replace(" ", "")) // 2)
            mc = CALL.match(raw)
            if mc:
                sites[pc] = (cur, mc.group(2))

    edge: collections.Counter = collections.Counter()
    cycles: collections.Counter = collections.Counter()
    lines_seen: dict[str, set] = collections.defaultdict(set)
    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        next(rows)
        for row in rows:
            try:
                pc = int(row[1], 16)
            except ValueError:
                continue
            n = int(row[4])
            s = sites.get(pc)
            if s is not None:
                edge[s] += n
            fn = owner.get(pc)
            if fn is not None and not (ITCM_LO <= pc < ITCM_HI):
                cycles[fn] += int(row[5])
                lines_seen[fn].add(pc & ~(LINE - 1))

    R = args.regions
    print(f"I-cache 8192 B / 32 B line / 4-way / {SETS} sets, "
          f"set=(addr>>5)&{SETS-1}, round-robin  [verified: melonDS-Accurate]")
    print(f"regions {R:,}   edges {len(edge):,}   "
          f"non-ITCM executed functions {len(cycles):,}\n")

    print(f"== hottest caller->callee edges ==")
    print(f"{'calls/fr':>9}{'gap B':>12}{'sets':>6}  edge")
    for (caller, callee), n in edge.most_common(args.top_edges):
        if caller not in start or callee not in start:
            continue
        gap = abs(start[callee] - start[caller])
        # do the two overlap in the cache?
        cs = {(a >> 5) & (SETS - 1) for a in lines_seen.get(caller, ())}
        ds = {(a >> 5) & (SETS - 1) for a in lines_seen.get(callee, ())}
        print(f"{n / R:>9,.1f}{gap:>12,}{len(cs & ds):>6}  "
              f"{caller[:30]} -> {callee[:30]}")

    # Clusters: union-find over edges at or above the floor.
    parent: dict[str, str] = {}

    def find(x):
        parent.setdefault(x, x)
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def union(a, b):
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[rb] = ra

    for (caller, callee), n in edge.items():
        if n / R < args.min_edge:
            continue
        if caller in cycles and callee in cycles:
            union(caller, callee)

    groups: dict[str, list] = collections.defaultdict(list)
    for fn in list(parent):
        groups[find(fn)].append(fn)

    ranked = sorted(groups.values(),
                    key=lambda g: -sum(cycles.get(f, 0) for f in g))
    print(f"\n== clusters (edges >= {args.min_edge}/frame) ==")
    hdr = (f"{'cyc/fr':>9}{'members':>8}{'own B':>9}{'x8K':>6}"
           f"{'span B':>11}{'in-span%':>9}{'maxset':>7}  head")
    print(hdr)
    print("-" * len(hdr))
    for g in ranked[:10]:
        members = [f for f in g if f in start and f in end]
        if not members:
            continue
        cyc = sum(cycles.get(f, 0) for f in g)
        own = sum(end[f] - start[f] for f in members)
        lo = min(start[f] for f in members)
        hi = max(end[f] for f in members)
        span = hi - lo
        per_set: collections.Counter = collections.Counter()
        for f in members:
            for a in lines_seen.get(f, ()):
                per_set[(a >> 5) & (SETS - 1)] += 1
        maxset = max(per_set.values()) if per_set else 0
        head = max(members, key=lambda f: cycles.get(f, 0))
        print(f"{cyc / R:>9,.0f}{len(members):>8}{own:>9,}"
              f"{own / ICACHE_BYTES:>6.1f}{span:>11,}"
              f"{100 * own / span if span else 0:>8.1f}%{maxset:>7}  {head[:34]}")

    print("\nin-span% = cluster's own bytes as a share of the address range it "
          "spans;\n  a low value means cold text is interleaved between hot "
          "members (Phase 4 pattern 3).")
    print("x8K = the cluster's OWN footprint in units of the whole I-cache; "
          ">1.0 means\n  reordering its members cannot make it resident.")


if __name__ == "__main__":
    main()
