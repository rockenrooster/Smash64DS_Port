#!/usr/bin/env python3
"""Split an arm9 profile by an ARBITRARY per-frame mask and rank every symbol.

Why this exists
---------------
`task37_census.py` can already split the census frames three ways -- by a symbol
that ran (`--split-by-symbol`), by the gate threshold (`--split-over-gate`), and
by the tail itself (`--split-top-frames`). All three derive the mask from the
profile's own cost. That is exactly the wrong key when the question is "which
symbols own THIS lane's excursion", because ranking by frame cost re-derives the
costliest frames, which are a different population (2026-08-13: the `SHDT` spike
frames overlap the profile's own top-88-by-cost frames at 21 of 88).

The profiler emits one row per (region, pc), so the region axis is a per-frame
series for every symbol in the ROM. This tool carries a mask IN -- typically the
`SHDT`/`SPRM`/... lane column of a tick-HUD gate run -- and asks what the
profile was doing on those frames. That closed the `SHDT` band in one pass over
an artifact already on disk, with no build and no emulator run.

It is also ~20x faster than the `csv.DictReader` path (26 s vs ~10 min on a
2.6 GB profile) because it decodes the pc column vectorised, so iterating on
several masks is cheap.

Two exact quantities come out, and they answer different questions:

* **self cycles** per (region, symbol) -- where the PC was.
* **call counts** per (region, symbol) -- a function's ENTRY pc executes exactly
  once per call, so the profiler's own instruction count at that pc IS that
  frame's call count. No sampling, no breakpoints. A call-count RATIO is what
  names a mechanism; a cycle premium only sizes it.

`ticks/frame = cycles / (2 * regions)` -- the ARM9 runs at 2x the tick timer
(`artifacts/performance/2026-08-13_c-residue/RESIDUE.md` section 0). Every
figure printed here is ticks unless the header says cycles.

Usage
-----
    python scripts/analyze-profile-region-split.py \
        artifacts/performance/<run>/profile/arm9-profile.csv \
        --census artifacts/performance/<run>/profile/census.json \
        --gate-csv artifacts/performance/<gate>/gate-rows.csv \
        --gate-lane SHDT --gate-min 30000 \
        --control-lane SHDT --control-max 10000

    # or an explicit region list, or the profile's own tail:
        --regions-file band.txt
        --top-by-symbols gmCollisionTestRectangle,func_ovl2_800ED490 --top 88

Always read `--check-controls`: a mask that reproduces the same ranking as a
random mask is measuring frame cost, not the mechanism.
"""
from __future__ import annotations

import argparse
import csv
import json
import re
import sys
from pathlib import Path

import numpy as np
import pandas as pd

IDLE = "armWaitForIrq"
_HEX = np.zeros(256, dtype=np.int64)
for _c in range(10):
    _HEX[ord("0") + _c] = _c
for _c in range(6):
    _HEX[ord("a") + _c] = 10 + _c
    _HEX[ord("A") + _c] = 10 + _c


def load_symbols(census_path: Path):
    census = json.loads(census_path.read_text(encoding="utf-8"))
    syms = census["symbols"]
    start = np.array([s["address"] for s in syms], dtype=np.int64)
    order = np.argsort(start, kind="stable")
    start = start[order]
    size = np.array([max(syms[i]["size"], 4) for i in order], dtype=np.int64)
    names = [syms[i]["name"] for i in order]
    return start, start + size, names, census


def scan(profile: Path, start, end, n_regions: int, chunk: int, site_pc=None):
    """One pass -> (self cycles, call counts, per-bl-site calls)."""
    ns = len(start)
    cyc = np.zeros((n_regions, ns), dtype=np.int64)
    calls = np.zeros((n_regions, ns), dtype=np.int64)
    nsite = 0 if site_pc is None else len(site_pc)
    sites = np.zeros((n_regions, nsite), dtype=np.int64)
    unmapped = 0
    for part in pd.read_csv(
        profile,
        usecols=["region", "pc", "total_cycles", "instructions"],
        dtype={"region": np.int64, "pc": str,
               "total_cycles": np.int64, "instructions": np.int64},
        chunksize=chunk, engine="c",
    ):
        text = part["pc"].to_numpy().astype("S10")
        if text.dtype.itemsize != 10:
            raise SystemExit("pc column is not fixed-width 0xXXXXXXXX")
        raw = np.frombuffer(text.tobytes(), dtype=np.uint8).reshape(-1, 10)
        pc = np.zeros(raw.shape[0], dtype=np.int64)
        for col in range(2, 10):
            pc = (pc << 4) | _HEX[raw[:, col]]
        region = part["region"].to_numpy()
        if region.max() >= n_regions:
            raise SystemExit(
                f"region {region.max()} exceeds regions={n_regions - 1} from "
                "arm9-profile.meta.txt -- wrong census/profile pairing")
        cycles = part["total_cycles"].to_numpy()
        insns = part["instructions"].to_numpy()

        idx = np.searchsorted(start, pc, side="right") - 1
        inside = (idx >= 0) & (pc < end[np.clip(idx, 0, ns - 1)])
        unmapped += int(cycles[~inside].sum())
        np.add.at(cyc.reshape(-1), region[inside] * ns + idx[inside],
                  cycles[inside])

        entry = np.searchsorted(start, pc)
        is_entry = (entry < ns) & (start[np.clip(entry, 0, ns - 1)] == pc)
        np.add.at(calls.reshape(-1), region[is_entry] * ns + entry[is_entry],
                  insns[is_entry])

        if nsite:
            s = np.searchsorted(site_pc, pc)
            on = (s < nsite) & (site_pc[np.clip(s, 0, nsite - 1)] == pc)
            np.add.at(sites.reshape(-1), region[on] * nsite + s[on], insns[on])
    return cyc, calls, sites, unmapped


BL = re.compile(
    r"^\s*([0-9a-f]+):\s+[0-9a-f ]+\s+bl(?:x)?(?:\.w)?\s+[0-9a-f]+ <([^>+]+)>\s*$")
HDR = re.compile(r"^([0-9a-f]{8}) <(.+)>:")


def leaf_sites(dis: Path, leaves: set[str]):
    """Every `bl <leaf>` site in the linked ELF, with its containing function.

    A per-PC profiler charges a leaf helper to itself and never to whoever ran
    it, which is why the soft-float class resists ranking. The profiler's own
    instruction count at the `bl` IS the number of calls that site made on that
    frame, so this needs no extra run and no sampling assumption. Sites with a
    `+0x..` suffix are branches INTO a symbol, not calls to it, and are dropped.
    """
    caller, out = None, []
    with dis.open(encoding="utf-8", errors="replace") as stream:
        for line in stream:
            head = HDR.match(line)
            if head:
                caller = head.group(2)
                continue
            hit = BL.match(line)
            if hit and caller and hit.group(2) in leaves:
                out.append((int(hit.group(1), 16), caller, hit.group(2)))
    return out


def gate_series(path: Path, lane: str):
    rows = list(csv.DictReader(path.open(newline="", encoding="utf-8")))
    if lane not in rows[0]:
        raise SystemExit(f"{path} has no column {lane!r}; columns: "
                         + ", ".join(k for k in rows[0]))
    return np.array([int(r[lane]) for r in rows], dtype=np.int64)


def runs_of(mask: np.ndarray) -> int:
    return int((np.diff(np.concatenate(([0], mask.astype(np.int8), [0])))
                == 1).sum())


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("profile", type=Path)
    ap.add_argument("--census", type=Path, required=True)
    ap.add_argument("--gate-csv", type=Path,
                    help="tick-HUD gate rows CSV whose lane column is the mask")
    ap.add_argument("--gate-lane", default="SHDT")
    ap.add_argument("--gate-min", type=int, default=30_000)
    ap.add_argument("--control-lane")
    ap.add_argument("--control-max", type=int)
    ap.add_argument("--gate-offset", type=int, default=1,
                    help="profile region for gate row 0 (default 1; region 0 "
                    "is outside the census window). Verify with --check-align.")
    ap.add_argument("--regions-file", type=Path,
                    help="explicit region ids, comma or newline separated")
    ap.add_argument("--top-by-symbols",
                    help="rank regions by these symbols' cycles instead")
    ap.add_argument("--top", type=int, default=88)
    ap.add_argument("--symbols", default="",
                    help="also print this comma-separated set as a named chain")
    ap.add_argument("--check-align", action="store_true",
                    help="correlate profile non-idle against every gate lane "
                    "at a range of offsets before trusting the mask")
    ap.add_argument("--check-controls", action="store_true",
                    help="re-run the ranking under a random mask and under the "
                    "profile's own top-N-by-cost mask. A finding that survives "
                    "only on its own mask is a finding; one that reproduces "
                    "under both is frame cost.")
    ap.add_argument("--attribute-leaves", metavar="SYM[,SYM...]",
                    help="charge these leaf helpers (soft float, memcpy, ...) "
                    "back to their CALLERS on this mask. Needs --dis. Without "
                    "it the collision chain reads as its self cycles only, "
                    "which on the 2026-08-13 SHDT band was 23,329 tk/frame of "
                    "a real 67,230 -- the other 65%% is soft float the per-PC "
                    "profiler charges to __aeabi_fmul.")
    ap.add_argument("--dis", type=Path,
                    help="arm-none-eabi-objdump -d output for the profiled ELF")
    ap.add_argument("--head", type=int, default=30)
    ap.add_argument("--chunk", type=int, default=4_000_000)
    ap.add_argument("--cache", type=Path,
                    help="npz to write/reuse so several masks cost one pass")
    args = ap.parse_args()

    # Validate the mask BEFORE the pass -- the scan is ~26 s on a 2.6 GB
    # profile and there is no reason to spend it to print a usage error.
    if not (args.gate_csv or args.regions_file or args.top_by_symbols):
        raise SystemExit("pass --gate-csv, --regions-file or --top-by-symbols")

    meta = (args.profile.parent / "arm9-profile.meta.txt")
    n_regions = 1602
    if meta.exists():
        for line in meta.read_text(encoding="utf-8").splitlines():
            if line.startswith("regions="):
                n_regions = int(line.split("=", 1)[1]) + 1
    start, end, names, census = load_symbols(args.census)

    leaves = {s.strip() for s in (args.attribute_leaves or "").split(",")
              if s.strip()}
    site_pc, site_caller, site_leaf = np.zeros(0, dtype=np.int64), [], []
    if leaves:
        if not args.dis:
            raise SystemExit("--attribute-leaves needs --dis")
        found = leaf_sites(args.dis, leaves)
        if not found:
            raise SystemExit(f"no bl sites to {sorted(leaves)} in {args.dis} -- "
                             "the names were inlined, renamed or dropped")
        found.sort()
        site_pc = np.array([f[0] for f in found], dtype=np.int64)
        site_caller = [f[1] for f in found]
        site_leaf = [f[2] for f in found]
        print(f"bl sites     {len(found):,} into "
              f"{len(set(site_leaf))} of {len(leaves)} named leaves")

    key = f"{len(site_pc)}"
    if args.cache and args.cache.exists():
        blob = np.load(args.cache, allow_pickle=True)
        if list(blob["names"]) != names or str(blob["sitekey"]) != key:
            raise SystemExit(f"{args.cache} was built from a different census "
                             "or leaf set -- delete it or pass a new --cache")
        cyc, calls = blob["cyc"], blob["calls"]
        sitecalls, unmapped = blob["sites"], int(blob["unmapped"])
        # Basename only. This output is committed as evidence and a cache under
        # a scratch directory carries the build machine's user directory into a
        # tracked file -- which the owner-name scan then fails on.
        print(f"cache        {args.cache.name} (reused)")
    else:
        cyc, calls, sitecalls, unmapped = scan(
            args.profile, start, end, n_regions, args.chunk,
            site_pc if len(site_pc) else None)
        if args.cache:
            np.savez_compressed(args.cache, cyc=cyc, calls=calls,
                                sites=sitecalls, unmapped=unmapped,
                                sitekey=key, names=np.array(names))
    ns = len(names)
    idx = {n: i for i, n in enumerate(names)}

    live = np.arange(1, n_regions - 1)
    per = cyc[live]
    kper = calls[live]
    nonidle = per.sum(axis=1) - (per[:, idx[IDLE]] if IDLE in idx else 0)
    frames = len(live)
    print(f"profile      {args.profile}")
    print(f"regions      {frames} (region 0 dropped)")
    print(f"basis        ticks = cycles / (2 x {frames}) -- ARM9 is 2x the tick timer")
    print(f"attributed   {cyc.sum():,} of {census['total_cycles']:,} cycles "
          f"({unmapped:,} unmapped)")
    print(f"non-idle     {nonidle.sum() / (2 * frames):,.0f} tk/frame mean")

    gate = None
    if args.gate_csv:
        gate = gate_series(args.gate_csv, args.gate_lane)
        if args.check_align:
            print("\nalignment: profile non-idle vs the gate lane, by offset")
            for off in range(args.gate_offset - 4, args.gate_offset + 5):
                ia = np.arange(len(gate)) + off
                ok = (ia >= 1) & (ia <= frames)
                r = float(np.corrcoef(nonidle[ia[ok] - 1].astype(float),
                                      gate[ok].astype(float))[0, 1])
                print(f"   offset {off:+3d}   r = {r:+.3f}"
                      + ("   <- chosen" if off == args.gate_offset else ""))

    def mask_from_gate(series, lo=None, hi=None):
        m = np.zeros(frames, bool)
        ia = np.arange(len(series)) + args.gate_offset
        ok = (ia >= 1) & (ia <= frames)
        sel = np.ones(len(series), bool)
        if lo is not None:
            sel &= series >= lo
        if hi is not None:
            sel &= series <= hi
        m[ia[ok & sel] - 1] = True
        return m

    if args.regions_file:
        want = {int(t) for t in args.regions_file.read_text().replace(",", " ").split()}
        band = np.array([r in want for r in live])
        control = ~band
        label = f"regions-file {args.regions_file}"
    elif args.top_by_symbols:
        keys = [idx[n] for n in args.top_by_symbols.split(",") if n in idx]
        if not keys:
            raise SystemExit("--top-by-symbols named nothing in this ELF")
        rank = np.argsort(per[:, keys].sum(axis=1))[::-1]
        band = np.zeros(frames, bool)
        band[rank[:args.top]] = True
        control = ~band
        label = f"top {args.top} by {args.top_by_symbols}"
    elif gate is not None:
        band = mask_from_gate(gate, lo=args.gate_min)
        if args.control_lane or args.control_max is not None:
            cs = (gate_series(args.gate_csv, args.control_lane)
                  if args.control_lane else gate)
            control = mask_from_gate(cs, hi=args.control_max)
        else:
            control = ~band
        label = f"gate {args.gate_lane} >= {args.gate_min:,}"
    else:
        raise SystemExit("pass --gate-csv, --regions-file or --top-by-symbols")

    if not band.any() or not control.any():
        raise SystemExit("mask or control is empty -- nothing to compare")

    def report(band, control, label, head):
        prem = per[band].mean(axis=0) - per[control].mean(axis=0)
        order = np.argsort(prem)[::-1]
        dtot = (nonidle[band].mean() - nonidle[control].mean()) / 2
        print(f"\n=== {label} ===")
        print(f"    {int(band.sum())} frames in {runs_of(band)} runs | "
              f"control {int(control.sum())} frames")
        print(f"    non-idle premium {dtot:+,.0f} tk/frame")
        print(f"    {'+tk/fr':>9} {'bandtk':>9} {'ctltk':>8} {'on':>5} "
              f"{'callsB':>9} {'callsC':>8} {'ratio':>6}  symbol")
        for j in order[:head]:
            kb = kper[band, j].mean()
            kc = kper[control, j].mean()
            print(f"    {prem[j]/2:9,.0f} {per[band, j].mean()/2:9,.0f} "
                  f"{per[control, j].mean()/2:8,.0f} "
                  f"{int((per[band, j] > 0).sum()):5} {kb:9.2f} {kc:8.2f} "
                  f"{(kb/kc if kc else float('inf')):6.1f}  {names[j]}")
        return prem, dtot

    prem, dtot = report(band, control, label, args.head)

    leafprem = {}
    if leaves:
        # cyc/call for each leaf, from its own whole-match total over its own
        # entry-pc count -- both exact, both already in this profile.
        percall = {}
        for name, j in idx.items():
            if name in leaves:
                k = calls[live, j].sum()
                percall[name] = (cyc[live, j].sum() / k) if k else 0.0
        cost = np.array([percall.get(lf, 0.0) for lf in site_leaf])
        work = sitecalls[live] * cost
        by_caller = {}
        for i, who in enumerate(site_caller):
            by_caller.setdefault(who, []).append(i)
        table = []
        for who, cols in by_caller.items():
            w = work[:, cols].sum(axis=1)
            k = sitecalls[live][:, cols].sum(axis=1)
            table.append((w[band].mean() - w[control].mean(),
                          k[band].mean(), k[control].mean(), who))
        table.sort(reverse=True)
        leafprem = {t[3]: t[0] for t in table}
        total = sum(t[0] for t in table) / 2
        print(f"\n=== leaf work charged to its CALLER, same mask "
              f"({total:+,.0f} tk/frame over all callers) ===")
        print(f"    {'+tk/fr':>9} {'leafB':>9} {'leafC':>8}  caller")
        for d, kb, kc, who in table[:args.head]:
            print(f"    {d/2:9,.0f} {kb:9.1f} {kc:8.1f}  {who}")

    if args.symbols:
        chain = [n for n in args.symbols.split(",") if n in idx]
        missing = [n for n in args.symbols.split(",") if n not in idx]
        cj = [idx[n] for n in chain]
        self_share = prem[cj].sum() / 2
        leaf_share = sum(leafprem.get(n, 0.0) for n in chain) / 2
        share = self_share + leaf_share
        print(f"\nnamed chain: {len(chain)} symbols, premium {share:+,.0f} tk/frame "
              f"(self {self_share:+,.0f} + leaf {leaf_share:+,.0f}) "
              f"= {share/dtot*100 if dtot else 0:.1f}% of the non-idle premium")
        if missing:
            print(f"  NOT IN THIS ELF (inlined, renamed or dropped): "
                  + ", ".join(missing))

    if args.check_controls:
        rng = np.random.default_rng(0)
        rand = np.zeros(frames, bool)
        rand[rng.choice(frames, int(band.sum()), replace=False)] = True
        cost = np.zeros(frames, bool)
        cost[np.argsort(nonidle)[::-1][:int(band.sum())]] = True
        print(f"\n--- controls: does the ranking survive only on ITS OWN mask? ---")
        print(f"    overlap band vs profile-top-cost: "
              f"{int((band & cost).sum())} of {int(band.sum())} "
              f"(chance {band.sum() * band.sum() / frames:.1f})")
        for nm, m in (("random mask (negative control)", rand),
                      ("profile's own top-N by cost", cost)):
            report(m, ~m, nm, min(args.head, 8))
    return 0


if __name__ == "__main__":
    sys.exit(main())
