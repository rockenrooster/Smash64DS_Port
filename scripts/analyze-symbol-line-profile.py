#!/usr/bin/env python3
"""Break one symbol's measured cycles down by inlined function and by source line.

A census ranks whole symbols. That is the wrong grain for a 9,844-byte function
whose cost is spread over 1,208 PCs with no hot spot -- the question there is
"which inlined helper, and which line inside it", and this answers both off a
profile that already exists. No build, no emulator run.

READ THIS BEFORE TRUSTING A LINE NUMBER. `addr2line` resolves against the DWARF
in the ELF, which describes the source **as it was when that ELF was built** --
not HEAD. This script therefore reads `NDS_TASK10_GIT_SHORT` out of the build's
own `nds_build_config.h`, prints it, and quotes each line from THAT commit. It
exists because a cycle was spent sizing a slice off line numbers read from HEAD
when the profile came from a commit ~85 lines adrift: the largest single row,
2,966 ticks/frame, landed on a blank line, and the fields that looked expensive
were the ones that were actually free. The measured result was 1,181 against a
6,500 prediction, and the prediction was wrong for this reason alone.

Two further cautions the same cycle re-proved:

  - `addr2line -f` names the INNERMOST inlined function at an address, which is
    the useful part, but the line number inside it can still land on a `}`, a
    `#endif`, or a blank line when the compiler attributes a block's setup to
    its closing brace. A row on a blank line means "somewhere in this region",
    not "this statement".
  - A cold region in a PROFILED build is not necessarily cold, or even present,
    in the shipped one. Check the shipped ELF before costing placement work:
    `ndsFighterDrawPlanVerify` is 1,848 bytes of the profiled function and does
    not exist in `smash64ds-battle-playable-hwtri.elf` at all.

Usage:
  python scripts/analyze-symbol-line-profile.py <symbol> \
      [--build builds/<dir>] [--profile <run dir>] [--top 20] [--by-function]
"""

from __future__ import annotations

import argparse
import bisect
import collections
import csv
import os
import re
import subprocess
import tempfile
from pathlib import Path

NM = os.environ.get("NM", r"C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe")
ADDR2LINE = os.environ.get(
    "ADDR2LINE", r"C:/devkitPro/devkitARM/bin/arm-none-eabi-addr2line.exe")
IDLE_SYMBOL = "armWaitForIrq"


def build_commit(build_dir: Path) -> str | None:
    config = build_dir / "nds_build_config.h"
    if not config.exists():
        return None
    text = config.read_text(encoding="utf-8", errors="replace")
    match = re.search(r'#define\s+NDS_TASK10_GIT_SHORT\s+"([0-9a-f]+)"', text)
    return match.group(1) if match else None


def source_at(commit: str, path: str) -> list[str] | None:
    try:
        blob = subprocess.run(["git", "show", f"{commit}:{path}"],
                              capture_output=True, text=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None
    return blob.stdout.split("\n")


def addr2line(elf: Path, addresses: list[int]) -> list[str]:
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
        out.extend(result.stdout.strip().split("\n"))
    return out


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("symbol")
    parser.add_argument("--build", type=Path,
                        default=Path("builds/build-c106-profile"))
    parser.add_argument("--profile", type=Path,
                        default=Path("artifacts/performance/"
                                     "2026-08-09_c106-profile"))
    parser.add_argument("--top", type=int, default=20)
    parser.add_argument("--by-function", action="store_true")
    parser.add_argument("--frame-budget", type=int, default=1128000)
    parser.add_argument("--non-idle", type=int, default=0)
    args = parser.parse_args()

    elf = next(args.build.glob("*.elf"), None)
    if elf is None:
        raise SystemExit(f"no ELF in {args.build}")
    csv_path = next(args.profile.glob("*profile.csv"), None)
    if csv_path is None:
        raise SystemExit(f"no profile csv in {args.profile}")

    commit = build_commit(args.build)
    print(f"elf     {elf}")
    print(f"profile {csv_path}")
    print(f"commit  {commit or 'UNKNOWN -- line quotes suppressed'}"
          f"   <-- line numbers describe THIS revision, not HEAD")

    listing = subprocess.run([NM, "-S", "--size-sort", str(elf)],
                             capture_output=True, text=True).stdout
    symbols = []
    for line in listing.splitlines():
        parts = line.split()
        if len(parts) == 4:
            symbols.append((int(parts[0], 16), int(parts[1], 16), parts[3]))
    symbols.sort()
    table = {name: (addr, size) for addr, size, name in symbols}
    if args.symbol not in table:
        raise SystemExit(f"{args.symbol} not in {elf}")
    base, size = table[args.symbol]

    counts: dict[int, list[int]] = collections.defaultdict(lambda: [0, 0])
    idle = 0
    with csv_path.open(newline="") as handle:
        reader = csv.reader(handle)
        next(reader)
        idle_range = table.get(IDLE_SYMBOL)
        for row in reader:
            pc = int(row[1], 16)
            if base <= pc < base + size:
                counts[pc][0] += int(row[4])
                counts[pc][1] += int(row[5])
            if idle_range and idle_range[0] <= pc < idle_range[0] + idle_range[1]:
                idle += int(row[5])

    non_idle = args.non_idle
    if non_idle <= 0:
        total_all = 0
        with csv_path.open(newline="") as handle:
            reader = csv.reader(handle)
            next(reader)
            for row in reader:
                total_all += int(row[5])
        non_idle = total_all - idle

    pcs = sorted(counts)
    resolved = addr2line(elf, pcs)
    sources: dict[str, list[str] | None] = {}
    rows: dict[tuple, list[int]] = collections.defaultdict(lambda: [0, 0])
    for index, pc in enumerate(pcs):
        func = resolved[2 * index] if 2 * index < len(resolved) else "?"
        loc = resolved[2 * index + 1] if 2 * index + 1 < len(resolved) else "?"
        key = (func,) if args.by_function else (func, loc)
        rows[key][0] += counts[pc][0]
        rows[key][1] += counts[pc][1]

    total = sum(v[1] for v in rows.values())
    print(f"\n{args.symbol}: {total:,} cycles = "
          f"{total / non_idle * args.frame_budget:,.0f} ticks/frame, "
          f"{len(pcs)} PCs\n")
    for key, value in sorted(rows.items(), key=lambda kv: -kv[1][1])[:args.top]:
        ticks = value[1] / non_idle * args.frame_budget
        head = f"  {value[1]:>10,} cyc {ticks:>7,.0f} tk/fr ex {value[0]:>9,}"
        if args.by_function:
            print(f"{head}  {key[0]}")
            continue
        func, loc = key
        print(f"{head}  {func} @ {loc.split('/')[-1]}")
        if not commit or ":" not in loc:
            continue
        raw, _, number = loc.rpartition(":")
        rel = raw.replace("\\", "/")
        marker = "/Smash64DS_Port/"
        if marker in rel:
            rel = rel.split(marker, 1)[1]
        if rel not in sources:
            sources[rel] = source_at(commit, rel)
        lines = sources[rel]
        if lines and number.isdigit() and 0 < int(number) <= len(lines):
            text = lines[int(number) - 1].strip()
            if not text:
                text = ("(blank line -- the compiler charged a region here, "
                        "not a statement)")
            print(f"{'':>34}| {text}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
