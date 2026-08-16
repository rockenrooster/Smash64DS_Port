#!/usr/bin/env python3
"""Rank stall owners on the EXPENSIVE FRAMES of a v3 profile, not on the match.

A whole-match census answers "what does a frame cost on average". The gate is a
percentile, and `artifacts/performance/.../RESIDUE.md` §1 records that a lane
which is bimodal at the percentile returns less than its mean. So this masks the
profile to its own most expensive frames first and ranks inside that mask.

WHY THE MASK IS BUILT FROM `total_cycles - halt_wait` AND NOT FROM
`total_cycles`. A profile region is a PRESENTED FRAME, so `total_cycles` is the
wall-clock interval between presents and is VBlank-quantized exactly like the
tick HUD's `ALL` bucket: on the 2026-08-14 v3 baseline 1,537 of 1,601 regions
sit within a few hundred cycles of 2,240,760 = two VBlanks. Sorting on that
sorts frames by rounding noise. `total_cycles - halt_wait` is the work the frame
actually did and is the profile-side analogue of `WORK-H`.

UNITS: 2 profile cycles = 1 project tick, and `ticks/frame = cycles /
(2 x regions)` -- the same basis `analyze-symbol-line-profile.py` prints. Every
table below states the basis it used. A census row read as cycles==ticks
overstates by exactly 2x and has done so at least once in this campaign.

TWO PHASES, because the input is 3.7 GB and re-ranking must be free:

  --reduce   one streaming pass over arm9-profile.csv; writes a per-PC CSV
             carrying both the whole-match and the marginal-frame totals.
  --report   reads that small CSV, attributes PCs to functions against the
             LINKED ELF, and prints the rankings.

ATTRIBUTION IS DONE TWICE ON PURPOSE. `nm` gives the symbol census (which
function owns the address range). `addr2line -f` gives the inline attribution
(which function the compiler says that instruction came from, following
inlining). They disagree wherever a helper is inlined, and the campaign has
twice ranked a lever off whichever one it happened to run: a symbol census
cannot see an inlined helper at all, and addr2line names deleted and inlined
functions, so two inlined copies of one loop land on two different names. Read
both columns before believing either.

Usage:
  python scripts/census-marginal-frame-owners.py --reduce \
      --profile artifacts/performance/<run>/v3-baseline \
      --out artifacts/performance/<out>/marginal-pc.csv [--marginal 160]
  python scripts/census-marginal-frame-owners.py --report \
      --pc-csv artifacts/performance/<out>/marginal-pc.csv \
      --build builds/<dir> [--top 30]
  python scripts/census-marginal-frame-owners.py --diff \
      --pc-csv <candidate>.csv --pc-csv-base <control>.csv \
      --build builds/<dir> [--build-base builds/<other>] [--top 30]

`--diff` exists because a stall PARTITION is only readable as a difference. A
single capture says a function costs N cycles of `icache_fill`; it cannot say
whether flipping one switch MOVED work from `issue` into `icache_fill`, which is
the question a candidate/control pair is run to answer. It requires the two arms
to have IDENTICAL CODE LAYOUT -- one runtime-gated `.data` word apart, the
`NDS_R2_*_DISPATCH` shape -- because it joins the two captures ON THE PROGRAM
COUNTER. It verifies that itself from the two symbol tables and refuses
otherwise; a relinked pair would silently attribute one arm's PCs to the other
arm's functions.
"""

from __future__ import annotations

import argparse
import collections
import csv
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

NM = os.environ.get("NM", r"C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe")
ADDR2LINE = os.environ.get(
    "ADDR2LINE", r"C:/devkitPro/devkitARM/bin/arm-none-eabi-addr2line.exe")
CYCLES_PER_TICK = 2

# Column order carried in the reduced CSV, and the source column index in
# arm9-profile.csv for each.  region=0 pc=1 mode=2 opcode=3.
CLASSES = (
    ("instructions", 4),
    ("total_cycles", 5),
    ("issue", 9),
    ("icache_fill", 10),
    ("dcache_fill", 11),
    ("write_buffer", 12),
    ("bus_contention", 13),
    ("dma_hold", 14),
    ("interlock", 16),
    ("halt_wait", 17),
)
# The six classes plan.md ranks on: everything the CPU pays for that is not
# idle and not DMA hold.  dma_hold is excluded because it is 8,701 tk/frame of
# a transfer the CPU asked for, not an instruction-stream cost.
RANK_CLASSES = ("issue", "icache_fill", "dcache_fill",
                "write_buffer", "interlock", "bus_contention")
# The three no lane has ever inspected.
POOL_CLASSES = ("write_buffer", "interlock", "bus_contention")


def profile_regions(profile_dir: Path) -> int:
    meta = next(profile_dir.glob("*profile.meta.txt"), None)
    if meta is None:
        return 0
    for line in meta.read_text(errors="ignore").splitlines():
        key, _, value = line.partition("=")
        if key.strip() == "regions":
            try:
                return int(value.strip())
            except ValueError:
                return 0
    return 0


def marginal_regions(profile_dir: Path, count: int,
                     band_min: int = 0, band_max: int = 0,
                     region_list: str = ""):
    """-> (set of region ids as strings, diagnostic dict).

    Ranked on non-idle cycles, for the reason in the module docstring.

    `band_min`/`band_max` are TICKS and select a closed band instead of the top
    N. The band exists because "top N by cost" and "the N frames the gate needs
    to convert" are not the same set: the gate's marginal frames are the
    CHEAPEST dropped ones, so a plain top-N mask pulls in load-frame outliers
    that no cadence conversion has to fix. Run both and compare the rankings --
    if they disagree, the top-N answer was an outlier artefact.

    `region_list` masks by an EXPLICIT set of region ids instead of ranking
    anything. It exists so that a population defined by the OTHER instrument --
    the tick-HUD gate run's rank-80 frames -- can be carried onto a profile
    without the caller re-deriving it, and so that the difference between the
    two masks is measurable rather than assumed.

    THE FRAME -> REGION MAPPING IS `region = frame - 439`, NOT `- 438`.
    run-task37-profile-census.ps1's banner prints "region r = presented frame
    438 + r", which is the ROM's own accounting; measured against the tick-HUD
    sampler's frame numbering on 2026-08-15 it is off by one. It is not a
    judgement call -- at `-438` the c185 rank-80 frames land at median profile
    rank 454 of 1600 with 13 of 79 inside the profile's own top-80, and the
    resulting attribution reads EVERY SITR row BELOW its whole-match rate; at
    `-439` they land at median rank 43 with 63 of 79 inside it. Lags -1 and +2
    are as bad as 0, so the alignment is unique. Re-measure it (rank the mapped
    regions) before trusting either number on a new capture.

    Even aligned, the two instruments do not bracket the same interval -- the
    tick-HUD sums brackets inside one logic+draw iteration, a region is the span
    between two CP15 markers -- so their rank-80 memberships differ. Two
    independent PROFILE arms correlate r=0.98 with 71/80 overlap; a profile
    against a tick-HUD arm peaks at r~0.67. Use this to cross-check a ranking,
    not to relabel one mask as the other.
    """
    path = profile_dir / "arm9-profile.regions.csv"
    work = []
    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        # A v2 capture has no stall columns at all, so the mask axis this module
        # exists to use does not exist either. Before 2026-08-15 that surfaced as
        # `KeyError: 'halt_wait'` after the caller had already spent an emulator
        # run, and the obvious reading of that traceback -- "the script is broken"
        # -- is wrong. Name the real fault: the wrong emulator produced the
        # profile. run-task37-profile-census.ps1 now refuses the invocation that
        # causes it (-MelonDS plus -RunnerSlot), but a v2 capture on disk from
        # before that guard still reaches here.
        if reader.fieldnames is not None and "halt_wait" not in reader.fieldnames:
            meta = profile_dir / "arm9-profile.meta.txt"
            fmt = ""
            if meta.exists():
                for line in meta.read_text().splitlines():
                    if line.startswith("format="):
                        fmt = line.split("=", 1)[1]
            raise SystemExit(
                f"{path} carries no `halt_wait` column, so this is a v2 profile"
                f"{' (' + fmt + ')' if fmt else ''}, not a v3 stall capture."
                " Re-run the census with emulators/melonds-attributor/melonDS.exe"
                " and WITHOUT -RunnerSlot; a runner slot always resolves to the"
                " v2 build and silently drops every stall class.")
        for row in reader:
            non_idle = int(row["total_cycles"]) - int(row["halt_wait"])
            work.append((non_idle, row["region"]))
    work.sort(reverse=True)
    if region_list:
        text = (Path(region_list[1:]).read_text() if region_list.startswith("@")
                else region_list)
        wanted = {t.strip() for t in text.replace("\n", ",").split(",")
                  if t.strip()}
        chosen = [entry for entry in work if entry[1] in wanted]
        missing = wanted - {region for _, region in chosen}
        if missing:
            raise SystemExit(
                f"--region-list names {len(missing)} regions this profile does "
                f"not have: {sorted(missing)[:8]}. A silently short mask "
                "changes every per-frame divisor below it.")
        axis = f"explicit region list, {len(chosen)} regions"
    elif band_max:
        lo = band_min * CYCLES_PER_TICK
        hi = band_max * CYCLES_PER_TICK
        chosen = [entry for entry in work if lo <= entry[0] <= hi]
        axis = f"total_cycles-halt_wait in [{band_min},{band_max}] ticks"
    else:
        chosen = work[:count]
        axis = "total_cycles-halt_wait, top N"
    return ({region for _, region in chosen}, {
        "regions_total": len(work),
        "marginal_count": len(chosen),
        "axis": axis,
        "threshold_ticks": chosen[-1][0] // CYCLES_PER_TICK if chosen else 0,
        "max_ticks": work[0][0] // CYCLES_PER_TICK if work else 0,
        "median_ticks": work[len(work) // 2][0] // CYCLES_PER_TICK if work else 0,
    })


def reduce_profile(profile_dir: Path, out_path: Path, count: int,
                   band_min: int = 0, band_max: int = 0,
                   region_list: str = "") -> int:
    mask, info = marginal_regions(profile_dir, count, band_min, band_max,
                                  region_list)
    print(f"profile        {profile_dir}")
    print(f"regions        {info['regions_total']}")
    print(f"marginal       {info['marginal_count']} frames, non-idle "
          f">= {info['threshold_ticks']:,} ticks "
          f"(median frame {info['median_ticks']:,}, max {info['max_ticks']:,})")
    csv_path = profile_dir / "arm9-profile.csv"
    size = csv_path.stat().st_size
    print(f"streaming      {csv_path} ({size / (1 << 30):.2f} GiB), one pass")

    width = len(CLASSES)
    indices = [index for _, index in CLASSES]
    every: dict[str, list[int]] = {}
    marginal: dict[str, list[int]] = {}
    every_get = every.get
    marginal_get = marginal.get
    rows = 0
    with csv_path.open("r", buffering=1 << 22) as handle:
        handle.readline()
        for line in handle:
            fields = line.split(",")
            pc = fields[1]
            values = [int(fields[i]) for i in indices]
            slot = every_get(pc)
            if slot is None:
                slot = [0] * width
                every[pc] = slot
            for i in range(width):
                slot[i] += values[i]
            if fields[0] in mask:
                slot = marginal_get(pc)
                if slot is None:
                    slot = [0] * width
                    marginal[pc] = slot
                for i in range(width):
                    slot[i] += values[i]
            rows += 1
            if (rows & 0x3FFFFFF) == 0:
                print(f"  {rows:,} rows", flush=True)
    print(f"rows           {rows:,} over {len(every):,} distinct PCs")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    names = [name for name, _ in CLASSES]
    header = (["pc"] + [f"all_{n}" for n in names]
              + [f"marg_{n}" for n in names])
    zeros = [0] * width
    with out_path.open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(header)
        for pc, slot in every.items():
            writer.writerow([pc] + slot + marginal.get(pc, zeros))
    meta_path = out_path.with_suffix(".meta.txt")
    meta_path.write_text(
        f"source={csv_path.as_posix()}\n"
        f"regions={info['regions_total']}\n"
        f"marginal_frames={info['marginal_count']}\n"
        f"marginal_threshold_ticks={info['threshold_ticks']}\n"
        f"marginal_axis=total_cycles-halt_wait\n"
        f"rows={rows}\n"
        f"distinct_pcs={len(every)}\n"
        f"cycles_per_tick={CYCLES_PER_TICK}\n",
        encoding="utf-8")
    print(f"wrote          {out_path} and {meta_path}")
    return 0


def elf_symbols(elf: Path):
    listing = subprocess.run([NM, "-C", "-S", "--defined-only", str(elf)],
                             capture_output=True, text=True, check=True).stdout
    entries = []
    for line in listing.splitlines():
        parts = line.split()
        if len(parts) < 4:
            continue
        try:
            address = int(parts[0], 16)
            size = int(parts[1], 16)
        except ValueError:
            continue
        if parts[2].lower() not in ("t", "w"):
            continue
        entries.append((address, size, parts[3]))
    entries.sort()
    return entries


def symbol_for(entries, starts, address: str) -> str:
    import bisect
    value = int(address, 16) & ~1
    index = bisect.bisect_right(starts, value) - 1
    if index < 0:
        return "?"
    start, size, name = entries[index]
    if size and value >= start + size:
        return "?"
    return name


def addr2line_names(elf: Path, addresses: list[str]) -> list[str]:
    out: list[str] = []
    for begin in range(0, len(addresses), 1500):
        chunk = addresses[begin:begin + 1500]
        with tempfile.NamedTemporaryFile("w", suffix=".txt",
                                         delete=False) as handle:
            handle.write("\n".join(chunk))
            path = handle.name
        result = subprocess.run([ADDR2LINE, "-f", "-e", str(elf), f"@{path}"],
                                capture_output=True, text=True)
        os.unlink(path)
        lines = result.stdout.strip().split("\n")
        out.extend(lines[0::2])
    return out


OBJDUMP = os.environ.get(
    "OBJDUMP", r"C:/devkitPro/devkitARM/bin/arm-none-eabi-objdump.exe")

# The six per-fighter procs gcRunAll dispatches, named by the tick-HUD bracket
# each one is wrapped in (src/port/reloc_backend_diagnostic_recorders.c:5632-
# 5780). The wrappers forward to the `battleship_` symbol, so THAT is the root
# of the static call closure -- rooting at the wrapper would stop at the call.
DEFAULT_OWNER_ROOTS = {
    "SINT": ("battleship_ftMainProcUpdateInterrupt",),
    "SHDT": ("battleship_ftMainProcSearchHitAll",),
    "SPHD": ("battleship_ftMainProcPhysicsMapDefault",),
    "SPRM": ("battleship_ftMainProcParams",),
    "SCAT": ("battleship_ftMainProcSearchCatch",),
    "SPHC": ("battleship_ftMainProcPhysicsMapCapture",),
}
CALL_RE = re.compile(r"\sbl[x]?\s+[0-9a-f]+ <([^>+]+)")
FUNC_RE = re.compile(r"^[0-9a-f]+ <([^>]+)>:$")


def call_graph(elf: Path) -> dict[str, set[str]]:
    """Direct-call graph from the LINKED image.

    Only `bl`/`blx <literal>` edges exist here: an indirect call through a
    register or a GObj proc pointer is invisible, so every reachable set below
    is a LOWER BOUND. That is stated wherever the result is printed rather than
    left for a reader to assume -- `linked-elf-is-the-reader-oracle`.
    """
    listing = subprocess.run([OBJDUMP, "-d", str(elf)],
                             capture_output=True, text=True, check=True).stdout
    graph: dict[str, set[str]] = {}
    current = None
    for line in listing.splitlines():
        header = FUNC_RE.match(line)
        if header:
            current = header.group(1)
            graph.setdefault(current, set())
            continue
        if current is None:
            continue
        call = CALL_RE.search(line)
        if call:
            graph[current].add(call.group(1))
    return graph


def reachable(graph: dict[str, set[str]], roots) -> set[str]:
    seen: set[str] = set()
    stack = [r for r in roots]
    while stack:
        name = stack.pop()
        if name in seen:
            continue
        seen.add(name)
        stack.extend(graph.get(name, ()))
    return seen


def write_marginal_census(rows, entries, starts, elf: Path, out: Path,
                          pc_csv: Path, marginal_frames: int) -> None:
    """Emit a census.json-shaped file whose cycles are the MARGINAL mask only.

    `analyze-subtree-attribution.py` and `analyze-leaf-helper-attribution.py`
    both take a `census.json` and both were written against a WHOLE-MATCH one,
    which is the wrong population for a percentile question -- `RESIDUE.md` §1's
    rule that a lane bimodal at the percentile returns less than its mean is
    exactly why this reducer exists. Writing the same schema off the marginal
    columns makes "which caller drives this helper ON THE FRAMES THAT SET P95"
    expressible with the tools that already exist, at zero extra cost: the
    reduced CSV is already per-PC and already carries `marg_*`.

    `total_cycles` here is the marginal-frame total, so any percentage a
    downstream tool prints is a share of the marginal frames, not of the match.
    """
    per_symbol_cycles: dict[str, int] = collections.defaultdict(int)
    per_symbol_insns: dict[str, int] = collections.defaultdict(int)
    total = 0
    for row in rows:
        name = symbol_for(entries, starts, row["pc"])
        cycles = int(row["marg_total_cycles"])
        per_symbol_cycles[name] += cycles
        per_symbol_insns[name] += int(row["marg_instructions"])
        total += cycles
    # ONE ROW PER ADDRESS RANGE, aliases listed rather than duplicated. `nm`
    # reports `__aeabi_fmul` and `__mulsf3` at the same address; `symbol_for`
    # can only return one of them, so emitting a row per nm entry gives the
    # other 0 cycles -- and the first smoke test of this export read the whole
    # soft-float multiply lane as zero for exactly that reason. Summing over a
    # reachable set (analyze-subtree-attribution) would also double-count if the
    # alias carried the same cycles.
    grouped: dict[int, list[str]] = collections.defaultdict(list)
    sizes: dict[int, int] = {}
    for address, size, name in entries:
        grouped[address].append(name)
        sizes[address] = max(sizes.get(address, 0), size)
    symbols = []
    for address, names in sorted(grouped.items()):
        owner = symbol_for(entries, starts, hex(address))
        if owner not in names:
            owner = names[0]
        symbols.append({
            "name": owner,
            "aliases": [n for n in names if n != owner],
            "address": address,
            "size": sizes[address],
            "section": "",
            "tier": "",
            "cycles": per_symbol_cycles.get(owner, 0),
            "instructions": per_symbol_insns.get(owner, 0),
            "cycles_per_byte": 0.0,
        })
    out.write_text(json.dumps({
        "profile": str(pc_csv),
        "elf": str(elf),
        "mask": "marginal",
        "marginal_frames": marginal_frames,
        "total_cycles": total,
        "unattributed_cycles": per_symbol_cycles.get("?", 0),
        "section_sizes": {},
        "symbols": symbols,
    }, indent=1))
    print(f"\nwrote {out}  (marginal-mask census.json, total_cycles "
          f"{total:,} over {marginal_frames} frames)")


def report(pc_csv: Path, build: Path, top: int, attribute_top: int,
           owner_roots: bool = False, census_out: Path | None = None) -> int:
    meta = {}
    meta_path = pc_csv.with_suffix(".meta.txt")
    if meta_path.exists():
        for line in meta_path.read_text().splitlines():
            key, _, value = line.partition("=")
            meta[key.strip()] = value.strip()
    regions = int(meta.get("regions", "0") or 0)
    marginal_frames = int(meta.get("marginal_frames", "0") or 0)
    elf = next(build.glob("*.elf"), None)
    if elf is None:
        raise SystemExit(f"no .elf in {build}")
    print(f"pc-csv    {pc_csv}")
    print(f"elf       {elf}")
    print(f"basis     whole-match ticks/frame = cycles / "
          f"({CYCLES_PER_TICK} x {regions} regions) = cycles / "
          f"{CYCLES_PER_TICK * regions:,}")
    print(f"basis     marginal ticks/frame = cycles / "
          f"({CYCLES_PER_TICK} x {marginal_frames} frames) = cycles / "
          f"{CYCLES_PER_TICK * marginal_frames:,}")
    print(f"mask      {meta.get('marginal_axis')} "
          f">= {int(meta.get('marginal_threshold_ticks', 0)):,} ticks")

    rows = list(csv.DictReader(pc_csv.open(newline="")))
    names = [name for name, _ in CLASSES]

    def rank_value(row, prefix, classes):
        return sum(int(row[f"{prefix}_{c}"]) for c in classes)

    entries = elf_symbols(elf)
    starts = [entry[0] for entry in entries]

    ordered = sorted(rows, key=lambda r: rank_value(r, "marg", RANK_CLASSES),
                     reverse=True)
    hot = [r["pc"] for r in ordered[:attribute_top]]
    inline = dict(zip(hot, addr2line_names(elf, hot)))

    def table(prefix, classes, divisor, title, group_inline):
        buckets = collections.defaultdict(lambda: [0] * (len(classes) + 1))
        for row in rows:
            value = rank_value(row, prefix, classes)
            if value == 0:
                continue
            if group_inline:
                name = inline.get(row["pc"])
                if name is None:
                    name = "(not attributed: below the addr2line cut)"
            else:
                name = symbol_for(entries, starts, row["pc"])
            slot = buckets[name]
            slot[0] += value
            for i, cls in enumerate(classes):
                slot[i + 1] += int(row[f"{prefix}_{cls}"])
        total = sum(slot[0] for slot in buckets.values())
        print(f"\n{title}")
        print(f"  total {total / divisor:,.0f} ticks/frame over "
              f"{len(buckets)} owners")
        head = f"  {'ticks/fr':>9s} {'share':>6s}  "
        head += " ".join(f"{c[:9]:>9s}" for c in classes)
        print(head + "  owner")
        for name, slot in sorted(buckets.items(), key=lambda kv: -kv[1][0])[:top]:
            line = f"  {slot[0] / divisor:9,.0f} {100.0 * slot[0] / total:5.1f}%  "
            line += " ".join(f"{slot[i + 1] / divisor:9,.0f}"
                             for i in range(len(classes)))
            print(line + f"  {name}")

    marg_div = float(CYCLES_PER_TICK * marginal_frames)
    all_div = float(CYCLES_PER_TICK * regions)
    table("marg", RANK_CLASSES, marg_div,
          f"MARGINAL {marginal_frames} FRAMES -- symbol census (nm ranges)",
          False)
    table("marg", RANK_CLASSES, marg_div,
          f"MARGINAL {marginal_frames} FRAMES -- inline attribution (addr2line)",
          True)
    table("marg", POOL_CLASSES, marg_div,
          "THE UNEXAMINED POOL on the marginal frames -- "
          "write_buffer + interlock + bus_contention (symbol census)",
          False)
    table("all", POOL_CLASSES, all_div,
          "THE UNEXAMINED POOL whole match -- "
          "write_buffer + interlock + bus_contention (symbol census)",
          False)

    if owner_roots:
        owner_table(rows, entries, starts, inline, elf, marg_div)
    if census_out is not None:
        write_marginal_census(rows, entries, starts, elf, census_out, pc_csv,
                              marginal_frames)
    return 0


def owner_table(rows, entries, starts, inline, elf: Path, divisor: float):
    """Split the marginal ranking by which fighter proc statically reaches it.

    `MARGINAL_OWNERS.md` §7 named `SITR`/`SHDT`/`SPHD`/`SPRM` as the owners of
    the P95 excess at BRACKET granularity. A per-PC profile has no call stack,
    so the only way to ask "which functions inside `gcRunAll` hold it" is to
    intersect the ranking with each proc's static call closure.

    THREE THINGS THIS IS NOT. It is not proof a function ran under that proc --
    reachability is not execution. A function reachable from two procs is
    reported as SHARED, never split between them, because there is nothing in
    the data to split it with. And it is not a bound in either direction: it
    misses every indirect call (so a proc's true closure is larger) while
    charging a shared leaf such as `__aeabi_fadd` to the procs that can reach it
    even on the frames where only the renderer called it.
    """
    graph = call_graph(elf)
    sets = {name: reachable(graph, roots)
            for name, roots in DEFAULT_OWNER_ROOTS.items()}
    per_symbol = collections.defaultdict(int)
    per_inline = collections.defaultdict(int)
    for row in rows:
        value = sum(int(row[f"marg_{c}"]) for c in RANK_CLASSES)
        if value == 0:
            continue
        per_symbol[symbol_for(entries, starts, row["pc"])] += value
        name = inline.get(row["pc"])
        if name is not None:
            per_inline[name] += value

    def classify(name: str) -> str:
        owners = sorted(k for k, members in sets.items() if name in members)
        if not owners:
            return "(not reached by any fighter proc)"
        if len(owners) == 1:
            return owners[0]
        return "SHARED:" + "+".join(owners)

    for label, counts in (("symbol census", per_symbol),
                          ("inline attribution", per_inline)):
        roll = collections.defaultdict(int)
        for name, value in counts.items():
            roll[classify(name)] += value
        total = sum(roll.values())
        print(f"\nFIGHTER-PROC REACHABILITY -- {label} "
              "(static bl/blx closure. It is NOT a bound in either direction: "
              "indirect calls are not followed, so a proc's real closure is "
              "larger; and a shared leaf reachable from a proc is charged here "
              "even when the draw side called it. Only the EXCLUSIVE rows are "
              "a clean attribution.)")
        print(f"  total {total / divisor:,.0f} ticks/frame")
        for name, value in sorted(roll.items(), key=lambda kv: -kv[1]):
            print(f"  {value / divisor:9,.0f} {100.0 * value / total:5.1f}%  "
                  f"{name}")

    print("\n  PER-OWNER TOP FUNCTIONS (symbol census, exclusive owners only)")
    for owner in sorted(sets):
        members = [(v, n) for n, v in per_symbol.items()
                   if classify(n) == owner]
        if not members:
            continue
        members.sort(reverse=True)
        subtotal = sum(v for v, _ in members)
        print(f"  {owner}: {subtotal / divisor:,.0f} ticks/frame exclusive, "
              f"{len(members)} functions")
        for value, name in members[:8]:
            print(f"      {value / divisor:9,.0f}  {name}")


def read_pc_rows(pc_csv: Path):
    """-> (rows keyed by pc, meta dict)."""
    meta = {}
    meta_path = pc_csv.with_suffix(".meta.txt")
    if meta_path.exists():
        for line in meta_path.read_text().splitlines():
            key, _, value = line.partition("=")
            meta[key.strip()] = value.strip()
    rows = {}
    for row in csv.DictReader(pc_csv.open(newline="")):
        rows[row["pc"]] = row
    return rows, meta


def assert_same_layout(elf_a: Path, elf_b: Path) -> None:
    """Refuse a diff across a relink.

    The join key is the program counter, so if the two arms placed their code
    differently the same PC names two different functions and every row of the
    output is fiction. `nm` addresses are the cheapest complete statement of the
    layout: identical address->name maps means identical placement.
    """
    if elf_a.resolve() == elf_b.resolve():
        return
    def table(elf):
        return {(addr, name) for addr, _size, name in elf_symbols(elf)}
    only_a = table(elf_a) - table(elf_b)
    only_b = table(elf_b) - table(elf_a)
    if only_a or only_b:
        sample = sorted(only_a | only_b)[:6]
        raise SystemExit(
            f"{elf_a.name} and {elf_b.name} do not share a code layout: "
            f"{len(only_a)} symbols only in the first, {len(only_b)} only in "
            f"the second, e.g. {sample}. A per-PC diff joins the two captures "
            "on the program counter, so this comparison would attribute one "
            "arm's addresses to the other arm's functions. Rebuild the pair so "
            "they differ only in a runtime-gated .data word.")


def diff(pc_csv: Path, pc_csv_base: Path, build: Path, build_base: Path,
         top: int) -> int:
    """Per-symbol candidate-minus-control delta, by stall class.

    Two populations are printed and they answer different questions. The
    WHOLE-MATCH delta covers the same 1,600 presented frames on both arms and is
    the clean total. The MARGINAL delta is each arm masked to ITS OWN most
    expensive frames, which is the right population for a percentile -- the same
    convention rank-80 uses -- but the two masks are not the same frame set, so
    read it as "where the cost sits at the percentile", never as a total.
    """
    elf = next(build.glob("*.elf"), None)
    elf_base = next(build_base.glob("*.elf"), None)
    if elf is None or elf_base is None:
        raise SystemExit(f"no .elf in {build} or {build_base}")
    assert_same_layout(elf, elf_base)

    rows, meta = read_pc_rows(pc_csv)
    rows_base, meta_base = read_pc_rows(pc_csv_base)
    regions = int(meta.get("regions", "0") or 0)
    regions_base = int(meta_base.get("regions", "0") or 0)
    marg = int(meta.get("marginal_frames", "0") or 0)
    marg_base = int(meta_base.get("marginal_frames", "0") or 0)
    if regions != regions_base or marg != marg_base:
        raise SystemExit(
            f"window mismatch: {regions} regions / {marg} marginal against "
            f"{regions_base} / {marg_base}. Both arms must run the same window.")

    entries = elf_symbols(elf)
    starts = [entry[0] for entry in entries]
    names = [name for name, _ in CLASSES]

    print(f"candidate {pc_csv}")
    print(f"control   {pc_csv_base}")
    print(f"elf       {elf}")
    print(f"basis     2 cycles = 1 tick; whole-match ticks/frame = cycles / "
          f"{CYCLES_PER_TICK * regions:,}")
    print(f"mask      candidate >= "
          f"{int(meta.get('marginal_threshold_ticks', 0)):,} ticks, control >= "
          f"{int(meta_base.get('marginal_threshold_ticks', 0)):,} ticks "
          f"({marg} frames each, EACH ARM ITS OWN)")

    for prefix, frames, label in (("all", regions, "WHOLE MATCH"),
                                  ("marg", marg, "MARGINAL")):
        divisor = float(CYCLES_PER_TICK * frames)
        buckets = collections.defaultdict(lambda: collections.defaultdict(int))
        zero = {f"{prefix}_{n}": "0" for n in names}
        for pc in set(rows) | set(rows_base):
            row = rows.get(pc, zero)
            base = rows_base.get(pc, zero)
            slot = buckets[symbol_for(entries, starts, pc)]
            for cls in names:
                slot[cls] += (int(row[f"{prefix}_{cls}"])
                              - int(base[f"{prefix}_{cls}"]))
        shown = ("total_cycles", "issue", "icache_fill", "dcache_fill",
                 "write_buffer", "interlock", "bus_contention", "instructions")

        # `instructions` is a COUNT, not a cycle total, so it does NOT take the
        # 2-cycles-per-tick divisor. Dividing it by 2 x frames like the stall
        # classes halves it, and that is exactly what happened: SPLIT.md's "the
        # ring executes 204 FEWER instructions per frame" is the true -407 read
        # through a cycle divisor. Corrected 2026-08-15; `plan.md` section 2's
        # units audit exists for this class and this is its third instance.
        def scale(cls: str) -> float:
            return float(frames) if cls == "instructions" else divisor

        grand = {cls: sum(s[cls] for s in buckets.values()) for cls in shown}
        print(f"\n{label} -- candidate minus control, ticks/frame over "
              f"{frames} frames (instructions column is a COUNT per frame)")
        print("  " + " ".join(f"{c[:11]:>11s}" for c in shown) + "  owner")
        print("  " + " ".join(f"{grand[c] / scale(c):11,.0f}" for c in shown)
              + "  == ALL SYMBOLS ==")
        ordered = sorted(buckets.items(),
                         key=lambda kv: -abs(kv[1]["total_cycles"]))
        for name, slot in ordered[:top]:
            print("  " + " ".join(f"{slot[c] / scale(c):11,.0f}"
                                  for c in shown) + f"  {name}")
    return 0


CONC_CLASSES = ("total_cycles", "issue", "icache_fill", "dcache_fill")


def concentration(pc_csv: Path, build: Path, symbols: list[str]) -> int:
    """Per-symbol COST concentration and CALL concentration, side by side.

    The quantity that converts a whole-match saving into a rank-80 saving is a
    concentration factor, and this campaign has repeatedly guessed one. It is
    already in the reduced CSV: every row carries `all_*` and `marg_*`, so
    `marg/marginal_frames` over `all/regions` IS the factor, measured.

    BOTH columns are printed because they are different numbers and the
    campaign has substituted one for the other (memory `call-share-is-not-cost-
    share`). Cost concentration is what a saving scales by. CALL concentration
    is what a per-entry cost -- instruction fetch of a body entered once per
    call -- scales by. On the 2026-08-15 ring pair they read 4.15x and 4.16x
    for the fixed kernels but 7.03x and 8.66x for the float bodies they
    replace, and a sizing that used one factor for both terms was wrong by the
    ratio.

    Call counts come from the ENTRY PC (`entry-pc-gives-exact-call-counts`):
    the first instruction of a function executes exactly once per call, so
    `all_instructions` at the symbol's address is the exact call count over the
    window. It is 0 for a symbol whose entry was never executed, which is
    itself a result -- do not read it as "the tool failed".
    """
    meta: dict[str, str] = {}
    meta_path = pc_csv.with_suffix(".meta.txt")
    if meta_path.exists():
        for line in meta_path.read_text().splitlines():
            key, _, value = line.partition("=")
            meta[key.strip()] = value.strip()
    regions = int(meta.get("regions", "0") or 0)
    marginal_frames = int(meta.get("marginal_frames", "0") or 0)
    if not regions or not marginal_frames:
        raise SystemExit(f"{meta_path} carries no regions/marginal_frames")
    elf = next(build.glob("*.elf"), None)
    if elf is None:
        raise SystemExit(f"no .elf in {build}")

    entries = elf_symbols(elf)
    starts = [entry[0] for entry in entries]
    sizes: dict[str, int] = {}
    address: dict[str, int] = {}
    for start, size, name in entries:
        if size > sizes.get(name, 0):
            sizes[name] = size
            address[name] = start
        address.setdefault(name, start)

    wanted = set(symbols)
    missing = wanted - set(sizes)
    agg: dict[str, list[int]] = collections.defaultdict(
        lambda: [0] * (2 * len(CONC_CLASSES)))
    calls: dict[int, tuple[int, int]] = {}
    for row in csv.DictReader(pc_csv.open(newline="")):
        pc = int(row["pc"], 16) & ~1
        calls[pc] = (int(row["all_instructions"]),
                     int(row["marg_instructions"]))
        name = symbol_for(entries, starts, row["pc"])
        if name not in wanted:
            continue
        slot = agg[name]
        for i, cls in enumerate(CONC_CLASSES):
            slot[i] += int(row[f"all_{cls}"])
            slot[len(CONC_CLASSES) + i] += int(row[f"marg_{cls}"])

    all_div = float(CYCLES_PER_TICK * regions)
    marg_div = float(CYCLES_PER_TICK * marginal_frames)
    print(f"pc-csv    {pc_csv}")
    print(f"elf       {elf}")
    print(f"basis     whole-match ticks/frame = cycles / {all_div:,.0f}   "
          f"marginal = cycles / {marg_div:,.0f}")
    print(f"mask      {meta.get('marginal_axis')} >= "
          f"{int(meta.get('marginal_threshold_ticks', 0)):,} ticks, "
          f"{marginal_frames} of {regions} frames")
    if missing:
        print(f"NOT DEFINED IN THIS ELF: {', '.join(sorted(missing))}")
    header = (f"{'symbol':<46}{'bytes':>7}{'all tk/fr':>10}{'marg tk/fr':>11}"
              f"{'COST x':>8}{'all ic':>8}{'marg ic':>9}{'ic x':>7}"
              f"{'calls/fr':>9}{'m calls/fr':>11}{'CALL x':>8}")
    print("\n" + header)
    totals = [0] * (2 * len(CONC_CLASSES))
    call_totals = [0, 0]
    for name in symbols:
        if name not in sizes:
            continue
        slot = agg.get(name, [0] * (2 * len(CONC_CLASSES)))
        for i in range(2 * len(CONC_CLASSES)):
            totals[i] += slot[i]
        entry = calls.get(address[name], (0, 0))
        call_totals[0] += entry[0]
        call_totals[1] += entry[1]
        print(_conc_row(name, sizes[name], slot, entry, all_div, marg_div,
                        regions, marginal_frames))
    print(_conc_row("TOTAL", 0, totals, tuple(call_totals), all_div, marg_div,
                    regions, marginal_frames))
    return 0


def _conc_row(name, size, slot, entry, all_div, marg_div, regions,
              marginal_frames) -> str:
    n = len(CONC_CLASSES)
    all_tk = slot[0] / all_div
    marg_tk = slot[n] / marg_div
    all_ic = slot[2] / all_div
    marg_ic = slot[n + 2] / marg_div
    all_calls = entry[0] / regions
    marg_calls = entry[1] / marginal_frames

    def ratio(top, bottom):
        return f"{top / bottom:.2f}" if bottom else "-"

    return (f"{name:<46}{size or '':>7}{all_tk:>10,.0f}{marg_tk:>11,.0f}"
            f"{ratio(marg_tk, all_tk):>8}{all_ic:>8,.0f}{marg_ic:>9,.0f}"
            f"{ratio(marg_ic, all_ic):>7}{all_calls:>9.2f}{marg_calls:>11.2f}"
            f"{ratio(marg_calls, all_calls):>8}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reduce", action="store_true")
    parser.add_argument("--report", action="store_true")
    parser.add_argument("--diff", action="store_true")
    parser.add_argument("--concentration", action="store_true",
                        help="per-symbol COST and CALL concentration on the "
                             "marginal mask -- the factor that converts a "
                             "whole-match saving into a rank-80 one, measured "
                             "rather than assumed")
    parser.add_argument("--symbols", default="",
                        help="comma-separated symbol names for "
                             "--concentration, or @path to a file with one "
                             "name per line")
    parser.add_argument("--pc-csv-base", type=Path)
    parser.add_argument("--build-base", type=Path)
    parser.add_argument("--profile", type=Path)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--pc-csv", type=Path)
    parser.add_argument("--build", type=Path)
    parser.add_argument("--marginal", type=int, default=160)
    parser.add_argument("--band-min", type=int, default=0)
    parser.add_argument("--band-max", type=int, default=0)
    parser.add_argument("--region-list", default="",
                        help="mask by these profile REGION ids (comma list or "
                             "@file) instead of ranking. For carrying the tick-"
                             "HUD gate arm's frame population onto a profile; "
                             "the caller owns the frame->region mapping, "
                             "because the two instruments do not bracket the "
                             "same interval.")
    parser.add_argument("--top", type=int, default=25)
    parser.add_argument("--attribute-top", type=int, default=30000)
    parser.add_argument("--owner-roots", action="store_true",
                        help="also split the marginal ranking by which of the "
                             "six fighter procs statically reaches each owner")
    parser.add_argument("--census-out", type=Path,
                        help="write a census.json-shaped file whose cycles are "
                             "the MARGINAL mask, so analyze-subtree-attribution "
                             "and analyze-leaf-helper-attribution rank the "
                             "frames that set P95 instead of the whole match")
    args = parser.parse_args()
    if args.reduce:
        if not args.profile or not args.out:
            raise SystemExit("--reduce needs --profile and --out")
        return reduce_profile(args.profile, args.out, args.marginal,
                              args.band_min, args.band_max, args.region_list)
    if args.report:
        if not args.pc_csv or not args.build:
            raise SystemExit("--report needs --pc-csv and --build")
        return report(args.pc_csv, args.build, args.top, args.attribute_top,
                      args.owner_roots, args.census_out)
    if args.diff:
        if not args.pc_csv or not args.pc_csv_base or not args.build:
            raise SystemExit(
                "--diff needs --pc-csv (candidate), --pc-csv-base (control) "
                "and --build")
        return diff(args.pc_csv, args.pc_csv_base, args.build,
                    args.build_base or args.build, args.top)
    if args.concentration:
        if not args.pc_csv or not args.build or not args.symbols:
            raise SystemExit(
                "--concentration needs --pc-csv, --build and --symbols")
        if args.symbols.startswith("@"):
            names = [line.strip() for line
                     in Path(args.symbols[1:]).read_text().splitlines()
                     if line.strip() and not line.startswith("#")]
        else:
            names = [s for s in args.symbols.split(",") if s]
        return concentration(args.pc_csv, args.build, names)
    parser.print_help()
    return 2


if __name__ == "__main__":
    sys.exit(main())
