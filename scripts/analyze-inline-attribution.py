#!/usr/bin/env python3
"""GLOBAL cost attribution by innermost INLINED function, across every caller.

WHY THIS EXISTS. `census.txt` and `analyze-symbol-line-profile.py` both rank
**symbols**. A `static inline` helper has no symbol, so its cost is scattered
across every function it was inlined into and it can be the sixth most expensive
thing in the ROM while appearing in no census row at all. That is not
hypothetical: on 2026-08-13 this script found `ndsRendererTask29GXRecord` at
**14,577 ticks/frame** and `ndsRendererNativeStagePreparedTextureValid` at
**9,369**, neither of which appears anywhere in
`artifacts/performance/2026-08-13_c-residue/RESIDUE.md` §3's 25-row consumer
table, because that table is a symbol ranking.

**Rank the inline attribution, not only the symbol census.**

No build and no emulator run: it reads a profile that already exists.

    # one 79-second pass, writes the cache next to the analysis
    python scripts/analyze-inline-attribution.py \
        --build builds/build-c123-profile \
        --profile artifacts/performance/2026-08-12_c123-rebank/profile \
        --cache artifacts/performance/2026-08-13_c-flagsweep/c123-pc-cycles.csv

    # every later question is a 98k-row read, ~20 seconds
    python scripts/analyze-inline-attribution.py --build ... --profile ... \
        --cache <same path> --function ndsRendererTask29GXRecord

The cache is `pc,cycles,instructions,symbol` for every PC in the profile. It is
reproducible from the profile in one pass, so it is a convenience, not evidence:
regenerate it rather than trusting a stale copy against a different ELF.

UNITS. A PROFILE CYCLE IS HALF A TICK — the ARM9 runs at twice the tick timer
the HUD and the gate are expressed in, so `ticks/frame = cycles / (2 x regions)`
where `regions` comes out of `arm9-profile.meta.txt` (RESIDUE.md §0 carries the
proof). Every figure printed here is ticks/frame on that basis and the basis
line says so.

CAUTIONS, all of which have cost this project a cycle at least once:

  - `addr2line -f` names the INNERMOST inlined function, which is the point,
    but its line number can land on a `}`, a `#endif` or a blank line when the
    compiler attributes a block's setup to its closing brace. A row on a blank
    line means "somewhere in this region", not "this statement".
  - The line numbers describe the source **as the profiled ELF was built**, not
    HEAD. `--quote` resolves them against that build's own
    `NDS_TASK10_GIT_SHORT` commit.
  - A high `tk/fr` on a low instruction count is a STALL, not arithmetic. Check
    cycles-per-instruction before designing: 7-11 cyc/insn means the lever is
    "stop touching the objects", not "do fewer compares" (slice 44, -35,904).
  - An inlined helper's cost is not automatically deletable. Attribute it back
    to its OWNERS (`--function`) and ask, per owner, whether the work is dead
    on that path.
  - **`addr2line` IS NOT BIT-DETERMINISTIC HERE, AND THE REST OF THIS PROJECT'S
    INSTRUMENTS ARE.** Measured 2026-08-13 on the c123 profile: two identical
    resolutions of the same 98,346 PCs against the same ELF disagreed on **56 of
    196,692 entries (0.028%)** — addresses where a nested inline shares its
    parent's `low_pc` and the DWARF inline-tree walk picks either frame, e.g.
    `ndsRendererTask29GlMatrixMode` vs `ndsRendererHardwareSetMatrixMode` at
    `0x02013404`. The practical effect is a wobble of order **0.002%** on a large
    row (900 cycles of 46,676,977). **So a difference of a few hundred cycles
    between two runs of this script is noise, not a finding** — unlike the tick
    sampler, where any repeat difference is fatal. `--verify-resolution` measures
    it for you. The per-chunk "exactly two lines per address" invariant is
    asserted, because a genuine misalignment would scramble every later address
    and must never pass silently.
"""

from __future__ import annotations

import argparse
import bisect
import collections
import csv
import os
import subprocess
import sys
import tempfile
from pathlib import Path

NM = os.environ.get("NM", r"C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe")
ADDR2LINE = os.environ.get(
    "ADDR2LINE", r"C:/devkitPro/devkitARM/bin/arm-none-eabi-addr2line.exe")
IDLE_SYMBOL = "armWaitForIrq"
CYCLES_PER_TICK = 2


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


def build_commit(build: Path) -> str | None:
    # Revision-first with config fallback for older build directories; see
    # analyze-symbol-line-profile.py.
    for name in ("nds_build_revision.h", "nds_build_config.h"):
        header = build / name
        if not header.exists():
            continue
        for line in header.read_text(errors="ignore").splitlines():
            if "NDS_TASK10_GIT_SHORT" in line and '"' in line:
                return line.split('"')[1]
    return None


def symbol_table(elf: Path):
    listing = subprocess.run([NM, "-S", "--size-sort", str(elf)],
                             capture_output=True, text=True).stdout
    syms = []
    for line in listing.splitlines():
        parts = line.split()
        if len(parts) == 4:
            syms.append((int(parts[0], 16), int(parts[1], 16), parts[3]))
    syms.sort()
    return syms


def build_cache(csv_path: Path, cache: Path, syms) -> None:
    starts = [s[0] for s in syms]

    def owner(pc: int) -> str:
        i = bisect.bisect_right(starts, pc) - 1
        if i >= 0 and syms[i][0] <= pc < syms[i][0] + syms[i][1]:
            return syms[i][2]
        return "?"

    cyc = collections.Counter()
    ins = collections.Counter()
    total = 0
    with csv_path.open(newline="") as handle:
        reader = csv.reader(handle)
        next(reader)
        for row in reader:
            c = int(row[5])
            total += c
            pc = int(row[1], 16)
            cyc[pc] += c
            ins[pc] += int(row[4])
    cache.parent.mkdir(parents=True, exist_ok=True)
    with cache.open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["pc", "cycles", "instructions", "symbol"])
        for pc in sorted(cyc):
            writer.writerow([f"0x{pc:08x}", cyc[pc], ins[pc], owner(pc)])
    print(f"cache   {cache}  ({len(cyc):,} PCs, {total:,} cycles)")


def read_cache(cache: Path):
    rows = []
    with cache.open(newline="") as handle:
        for d in csv.DictReader(handle):
            rows.append((int(d["pc"], 16), int(d["cycles"]),
                         int(d["instructions"]), d["symbol"]))
    return rows


def addr2line(elf: Path, addresses: list[int]) -> list[str]:
    """-> [func, file:line] x len(addresses). Raises if that shape ever breaks:
    a short chunk would shift every later address into the wrong function."""
    out: list[str] = []
    for start in range(0, len(addresses), 1500):
        chunk = addresses[start:start + 1500]
        with tempfile.NamedTemporaryFile("w", suffix=".txt",
                                         delete=False) as handle:
            handle.write("\n".join(f"0x{a:x}" for a in chunk))
            path = handle.name
        result = subprocess.run([ADDR2LINE, "-f", "-e", str(elf), f"@{path}"],
                                capture_output=True, text=True)
        os.unlink(path)
        lines = result.stdout.strip().split("\n")
        if len(lines) != 2 * len(chunk):
            raise SystemExit(
                f"addr2line returned {len(lines)} lines for {len(chunk)} "
                f"addresses at offset {start} (rc={result.returncode}). Every "
                "later address would be attributed to the wrong function; "
                "refusing to print a scrambled table.")
        out.extend(lines)
    return out


def source_at(commit: str, path: str) -> list[str] | None:
    try:
        blob = subprocess.run(["git", "show", f"{commit}:{path}"],
                              capture_output=True, text=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None
    return blob.stdout.split("\n")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="attribute a profile by innermost inlined function")
    parser.add_argument("--build", type=Path, required=True,
                        help="build dir holding the profiled ELF")
    parser.add_argument("--profile", type=Path, required=True,
                        help="profile run dir (arm9-profile.csv + .meta.txt)")
    parser.add_argument("--cache", type=Path, required=True,
                        help="pc,cycles,instructions,symbol cache; built if absent")
    parser.add_argument("--rebuild-cache", action="store_true")
    parser.add_argument("--function",
                        help="drill into ONE inlined function: per (owner, line)")
    parser.add_argument("--top", type=int, default=60)
    parser.add_argument("--quote", action="store_true",
                        help="quote each source line from the build's own commit")
    parser.add_argument("--verify-resolution", action="store_true",
                        help="resolve twice and report how much cost sits on "
                             "addresses addr2line names inconsistently")
    args = parser.parse_args()

    elf = next(args.build.glob("*.elf"), None)
    if elf is None:
        raise SystemExit(f"no ELF in {args.build}")
    csv_path = next(args.profile.glob("*profile.csv"), None)
    if csv_path is None:
        raise SystemExit(f"no profile csv in {args.profile}")
    regions = profile_regions(args.profile)
    if regions <= 0:
        raise SystemExit(f"no `regions` in {args.profile}/*profile.meta.txt -- "
                         "without it there is no tick basis and every number "
                         "printed would be in an undeclared unit")
    divisor = float(CYCLES_PER_TICK * regions)
    commit = build_commit(args.build)

    print(f"elf     {elf}")
    print(f"profile {csv_path}")
    print(f"commit  {commit or 'UNKNOWN -- line quotes suppressed'}"
          f"   <-- line numbers describe THIS revision, not HEAD")
    print(f"basis   ticks/frame = cycles / ({CYCLES_PER_TICK} x {regions} "
          f"regions) = cycles / {CYCLES_PER_TICK * regions:,}")

    syms = symbol_table(elf)
    if args.rebuild_cache or not args.cache.exists():
        build_cache(csv_path, args.cache, syms)
    rows = read_cache(args.cache)

    idle = {s[2] for s in syms if s[2] == IDLE_SYMBOL}
    pcs = [r[0] for r in rows]
    resolved = addr2line(elf, pcs)

    if args.verify_resolution:
        second = addr2line(elf, pcs)
        unstable = {i // 2 for i, (a, b) in enumerate(zip(resolved, second))
                    if a != b}
        mass = sum(rows[i][1] for i in unstable)
        print(f"resolution: {len(unstable):,} of {len(pcs):,} addresses "
              f"({100.0*len(unstable)/max(len(pcs),1):.3f}%) named "
              f"inconsistently by addr2line, carrying {mass:,} cycles "
              f"= {mass/divisor:,.0f} tk/frame. A delta of that order between "
              f"two runs is NOISE, not a finding.")

    by_fn = collections.Counter()
    by_fn_insns = collections.Counter()
    fn_owner = collections.defaultdict(collections.Counter)
    fn_owner_line = collections.Counter()
    fn_owner_line_insns = collections.Counter()
    fn_line_file = {}
    non_idle = 0

    for i, (pc, cycles, insns, symbol) in enumerate(rows):
        if symbol in idle:
            continue
        non_idle += cycles
        fn = resolved[2 * i].strip()
        loc = resolved[2 * i + 1].strip().split(" (")[0]
        path, _, line = loc.rpartition(":")
        by_fn[fn] += cycles
        by_fn_insns[fn] += insns
        fn_owner[fn][symbol] += cycles
        fn_owner_line[(fn, symbol, line)] += cycles
        fn_owner_line_insns[(fn, symbol, line)] += insns
        fn_line_file.setdefault((fn, line), path)

    print(f"non-idle {non_idle:,} cycles = {non_idle/divisor:,.0f} tk/frame\n")

    if args.function:
        want = args.function
        total = by_fn.get(want, 0)
        if total == 0:
            raise SystemExit(f"{want} resolved at no PC in this profile")
        print(f"{want}: {total:,} cyc = {total/divisor:,.0f} tk/fr, "
              f"{by_fn_insns[want]:,} insns, "
              f"{total/max(by_fn_insns[want],1):.2f} cyc/insn")
        by_line = collections.Counter()
        for (fn, _sym, line), c in fn_owner_line.items():
            if fn == want:
                by_line[line] += c
        print("\n-- by source line (a high tk/fr on few insns is a STALL) --")
        for line, c in by_line.most_common():
            path = fn_line_file.get((want, line), "?")
            print(f"  :{line:<6} {c/divisor:>8,.0f} tk/fr   {path}")
        print("\n-- by (owning symbol, line): ask per owner whether it is dead --")
        for (fn, sym, line), c in fn_owner_line.most_common():
            if fn != want:
                continue
            print(f"  {c/divisor:>8,.0f} tk/fr {fn_owner_line_insns[(fn,sym,line)]:>12,} insn"
                  f"  {sym} :{line}")
        return 0

    print(f"== top {args.top} innermost inlined/leaf functions, GLOBAL ==")
    print(f"{'cycles':>14} {'tk/fr':>8} {'cyc/insn':>9}  function   [top owner]")
    sources: dict[str, list[str] | None] = {}
    for fn, c in by_fn.most_common(args.top):
        owner_name, owner_c = fn_owner[fn].most_common(1)[0]
        cpi = c / max(by_fn_insns[fn], 1)
        print(f"{c:>14,} {c/divisor:>8,.0f} {cpi:>9.2f}  {fn}"
              f"   [{owner_name} {owner_c/divisor:,.0f}]")
        if args.quote and commit:
            best = max(((l, v) for (f, s, l), v in fn_owner_line.items()
                        if f == fn), key=lambda kv: kv[1], default=None)
            if best is None:
                continue
            line, _ = best
            path = fn_line_file.get((fn, line))
            if not path:
                continue
            rel = path.replace("\\", "/")
            marker = "/Smash64DS_Port/"
            if marker in rel:
                rel = rel.split(marker, 1)[1]
            elif not rel.startswith(("src/", "include/", "scripts/")):
                continue
            if rel not in sources:
                sources[rel] = source_at(commit, rel)
            text = sources[rel]
            try:
                idx = int(line) - 1
            except ValueError:
                continue
            if text and 0 <= idx < len(text):
                print(f"{'':>34}| {text[idx].strip()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
