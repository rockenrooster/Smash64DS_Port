#!/usr/bin/env python3
"""Task 37 census: join the melonDS per-PC cycle profile to ELF placement.

The repo-owned melonDS build writes a CSV attributing emulated ARM9 cycles to
every program counter it executed, including the cost of cache fills, cache
streaming, write-buffer drains, bus waits, interlocks, and pipeline refills.
That is the first time code placement has been observable off-device.

This script answers the three questions Task 37 needs:

  A. Which symbols cost the most, and what placement tier are they in today?
  B. What rent does each current .itcm resident pay for its slot? A resident is
     ranked by cycles per byte, and residents that never executed are listed
     too -- absence from the profile is the strongest eviction signal there is,
     and it is exactly what a cycles-only view hides.
  C. Which unplaced symbols would fit the free space in .text.hot?

Placement comes from the ELF, cost comes from the CSV, and every symbol in the
placed sections appears in the output whether or not it executed.
"""

from __future__ import annotations

import argparse
import bisect
import csv
import json
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

# Output sections that hold deliberately placed code, best tier first. Anything
# else a FUNC symbol lands in is reported as its own section name.
PLACED_SECTIONS = (".itcm", ".text.hot", ".text.hot.draw")


@dataclass
class Symbol:
    address: int
    size: int
    name: str
    section: str
    aliases: list = field(default_factory=list)
    cycles: int = 0
    instructions: int = 0
    counted_pcs: int = 0
    mem_cycles: int = 0
    mem_instructions: int = 0
    regions: set = field(default_factory=set)

    @property
    def cycles_per_byte(self) -> float:
        return self.cycles / self.size if self.size else 0.0

    @property
    def cycles_per_insn(self) -> float:
        return self.cycles / self.instructions if self.instructions else 0.0

    @property
    def stall_cycles(self) -> int:
        """Cycles beyond one per instruction: the part placement could address."""
        return max(0, self.cycles - self.instructions)

    @property
    def nonmem_stall(self) -> int:
        """Stall on instructions that touch no data. Fetch, interlock, refill."""
        return max(0, (self.cycles - self.mem_cycles) - (self.instructions - self.mem_instructions))

    @property
    def label(self) -> str:
        if not self.aliases:
            return self.name
        return f"{self.name} (+{len(self.aliases)} alias)"


def run_tool(tool: str, *args: str) -> str:
    try:
        result = subprocess.run(
            [tool, *args],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
        )
    except FileNotFoundError as exc:
        raise RuntimeError(f"could not run {tool}") from exc
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(exc.stderr.strip() or f"{tool} failed") from exc
    return result.stdout


SECTION_RE = re.compile(
    r"^\s*\[\s*(\d+)\]\s+(\S+)\s+\S+\s+([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)"
)
SYMBOL_RE = re.compile(
    r"^\s*\d+:\s+([0-9a-fA-F]+)\s+(\d+)\s+(\S+)\s+(\S+)\s+\S+\s+(\S+)\s+(\S+)"
)


def parse_sections(text: str) -> dict[int, tuple[str, int, int]]:
    """index -> (name, address, size)"""
    sections: dict[int, tuple[str, int, int]] = {}
    for line in text.splitlines():
        match = SECTION_RE.match(line)
        if match:
            index, name, address, size = match.groups()
            sections[int(index)] = (name, int(address, 16), int(size, 16))
    return sections


def parse_symbols(text: str, sections: dict[int, tuple[str, int, int]]) -> list[Symbol]:
    """One Symbol per distinct address.

    The libgcc float helpers in .itcm are heavily aliased: __addsf3,
    __aeabi_fadd and __aeabi_frsub can all name overlapping code. Left as
    separate rows they would triple-count the section's bytes and hand the
    cycles to whichever name happened to sort last, which would make the rent
    table meaningless exactly where it matters most.
    """
    by_address: dict[int, list[tuple[int, str, str]]] = {}
    for line in text.splitlines():
        match = SYMBOL_RE.match(line)
        if not match:
            continue
        value, size, kind, _bind, ndx, name = match.groups()
        if kind != "FUNC" or not ndx.isdigit():
            continue
        section = sections.get(int(ndx))
        if section is None:
            continue
        # ARM ELF sets bit 0 of st_value on Thumb function symbols; execution
        # addresses in the profile never have it.
        address = int(value, 16) & ~1
        entry = (int(size), name, section[0])
        bucket = by_address.setdefault(address, [])
        if entry not in bucket:
            bucket.append(entry)

    symbols: list[Symbol] = []
    for address, entries in by_address.items():
        # The widest declared size is the real extent of the code; the rest are
        # names for a prefix of it.
        entries.sort(key=lambda e: (-e[0], e[1]))
        size, name, section = entries[0]
        symbols.append(
            Symbol(address, size, name, section, aliases=[e[1] for e in entries[1:]])
        )
    return sorted(symbols, key=lambda s: (s.address, s.name))


def occupied_bytes(symbols: list[Symbol]) -> int:
    """Union of the symbols' address ranges, so overlaps are counted once."""
    spans = sorted((s.address, s.address + s.size) for s in symbols if s.size)
    total = 0
    current_start = current_end = None
    for start, end in spans:
        if current_end is None or start > current_end:
            if current_end is not None:
                total += current_end - current_start
            current_start, current_end = start, end
        else:
            current_end = max(current_end, end)
    if current_end is not None:
        total += current_end - current_start
    return total


def attribute(
    symbols: list[Symbol], profile: Path, detail: dict | None = None
) -> tuple[int, int, int]:
    """Fold the per-PC CSV into the symbol list. Returns (cycles, unmapped, rows).

    With NDS_TASK37_PROFILE_PER_FRAME_REGION=1 the emulator emits one row per
    (region, pc) pair, so passing `detail` also builds
    region -> symbol index -> [cycles, instructions, mem_cycles]. Window totals
    can hide a cost that only appears on a handful of frames, and those frames
    are where PROJECT_GOAL.md's P95 gate is decided.
    """
    addresses = [symbol.address for symbol in symbols]
    total_cycles = 0
    unmapped_cycles = 0
    rows = 0

    with profile.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            pc = int(row["pc"], 16)
            cycles = int(row["total_cycles"])
            instructions = int(row["instructions"])
            total_cycles += cycles
            rows += 1

            index = bisect.bisect_right(addresses, pc) - 1
            symbol = symbols[index] if index >= 0 else None
            if symbol is None or (symbol.size and pc >= symbol.address + symbol.size):
                unmapped_cycles += cycles
                continue
            symbol.cycles += cycles
            symbol.instructions += instructions
            symbol.counted_pcs += 1
            region = int(row["region"])
            symbol.regions.add(region)
            is_mem = is_memory_op(int(row["opcode"], 16), row["mode"] == "thumb")
            if is_mem:
                symbol.mem_cycles += cycles
                symbol.mem_instructions += instructions
            if detail is not None:
                slot = detail.setdefault(region, {}).setdefault(index, [0, 0, 0])
                slot[0] += cycles
                slot[1] += instructions
                if is_mem:
                    slot[2] += cycles

    return total_cycles, unmapped_cycles, rows


def disassemble_range(
    objdump: str, elf: Path, start: int, end: int
) -> dict[int, tuple[str, str]]:
    """pc -> (source line, mnemonic) over [start, end), from `objdump -d -l`.

    Source attribution here is safe in a way plain addr2line is not: the range is
    bounded by one symbol taken from the ELF symbol table, so an inlined callee's
    line number is a fact about that address rather than a claim about which
    function owns the cycles. addr2line's habit of naming deleted and inlined
    functions is what made three earlier attributions wrong.
    """
    text = run_tool(
        objdump, "-d", "-l", f"--start-address=0x{start:x}",
        f"--stop-address=0x{end:x}", str(elf)
    )
    mapping: dict[int, tuple[str, str]] = {}
    source = "?"
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.endswith(":") and (":" in stripped) and not stripped[:1].isdigit():
            # "path/file.c:1234" or "func():" -- keep the file:line form only.
            head = stripped[:-1]
            if head.rsplit(":", 1)[-1].split(" ")[0].isdigit():
                source = head.rsplit("/", 1)[-1].rsplit("\\", 1)[-1]
            continue
        match = re.match(r"^\s*([0-9a-f]{6,8}):\s+\S+\s+(.*)$", line)
        if match:
            mnemonic = match.group(2).split("@")[0].strip()
            mapping[int(match.group(1), 16)] = (source, mnemonic)
    return mapping


# PROJECT_GOAL.md budgets 1.12M ARM9 *ticks* per presented frame. The profiler
# counts ARM9 *cycles*, which run at twice the tick clock, so two VBlanks is
# 2,240,760 cycles and three is 3,361,140. Presentation is VBlank-quantized, so
# frames cluster on those values and nothing lands between them.
#
# THE THRESHOLD MUST NOT BE THE QUANTUM ITSELF. It was 2_240_760 until
# 2026-08-11, on the reasoning that an exact quantum "is a definition rather
# than a tuned knob". The quantum is not exact: on the c118 profile the
# 2-VBlank cluster spans 2,239,036 .. 2,242,486, a 3,450-cycle spread straddling
# 2,240,760, so **616 of its 1262 frames were classified over-gate on +-1,700
# cycles of jitter** -- 0.154% of the threshold. That made `--split-over-gate`
# mark 954 of 1600 frames (60%) when only 338 (21%) genuinely miss the cadence,
# and every ranking it produced was computed on a population that was 65% noise.
# Sit the threshold BETWEEN the clusters instead, where the jitter cannot reach
# it. Anything from ~2.3M to ~3.3M gives the same answer.
GATE_CYCLES = 2_800_950  # 2.5 VBlanks: above every 2-VBlank frame, below every 3


def split_regions_over_gate(detail: dict) -> tuple[list[int], list[int]]:
    """Partition census regions into (over the gate, inside it) by their own cost.

    Both populations come from one run, on one ROM, with one cache history, so
    the comparison carries none of the cross-build floor that makes two separate
    censuses incomparable. Region 0 is "outside the census window" and is dropped.
    """
    over, clean = [], []
    for region, per_symbol in detail.items():
        if region == 0:
            continue
        total = sum(v[0] for v in per_symbol.values())
        (over if total > GATE_CYCLES else clean).append(region)
    return sorted(over), sorted(clean)


def split_regions_top(
    detail: dict, symbols: list[Symbol], idle: set[str], count: int
) -> tuple[list[int], list[int]]:
    """Partition regions into (the `count` most expensive, everything else).

    This is the partition the gate is actually defined on and the one this tool
    was missing. `--split-over-gate` thresholds on 2 VBlanks, which on the c118
    profile marks 895 of 1600 frames -- 56%, not a tail -- and `ALL` is
    VBlank-quantized besides, so a frame that finishes early simply waits longer
    and lands in the same bucket. `P95 <= 1.12M` is a statement about the 80th
    most expensive frame of 1600, so ranking is the only partition that reaches
    it.

    Frames rank on NON-IDLE cycles. Ranking on the total would rank them by how
    long they waited, which is backwards: the busiest frame spins in
    `armWaitForIrq` the least.
    """
    drop = {
        index for index, symbol in enumerate(symbols)
        if symbol.name in idle or any(a in idle for a in symbol.aliases)
    }
    busy = []
    for region, per_symbol in detail.items():
        if region == 0:
            continue
        busy.append((
            sum(v[0] for i, v in per_symbol.items() if i not in drop), region))
    if count >= len(busy):
        raise RuntimeError(
            f"--split-top-frames {count} covers every one of {len(busy)} census "
            "regions, leaving no control population")
    busy.sort(reverse=True)
    return (sorted(r for _, r in busy[:count]),
            sorted(r for _, r in busy[count:]))


def split_regions(
    detail: dict, symbols: list[Symbol], marker: str
) -> tuple[list[int], list[int]]:
    """Partition regions into (marker ran, marker did not).

    The partition is defined by a symbol the profile itself observed, so the
    populations and their control come from one run. Naming the frames from a
    previous run's ring would reintroduce the cross-run assumption this split
    exists to remove. Region 0 is "outside the census window" and is dropped.
    """
    wanted = {
        index
        for index, symbol in enumerate(symbols)
        if symbol.name == marker or marker in symbol.aliases
    }
    if not wanted:
        raise RuntimeError(
            f"marker symbol {marker!r} is not a FUNC symbol in this ELF -- it was "
            "inlined, renamed, or dropped, and a partition keyed on an absent "
            "name silently classifies every frame as a control frame"
        )
    hot, control = [], []
    for region, per_symbol in detail.items():
        if region == 0:
            continue
        cost = sum(per_symbol.get(index, (0,))[0] for index in wanted)
        (hot if cost else control).append(region)
    return sorted(hot), sorted(control)


def tier_of(symbol: Symbol) -> str:
    return symbol.section if symbol.section in PLACED_SECTIONS else ".main"


def is_memory_op(opcode: int, thumb: bool) -> bool:
    """True when the instruction moves data between registers and memory.

    Placement changes what an instruction fetch costs. It does nothing for what
    a load costs. Splitting stall cycles by this predicate is what separates a
    symbol that is waiting on its own code from one that is waiting on the data
    it walks -- and only the first kind is a placement candidate.
    """
    if thumb:
        opcode &= 0xFFFF
        if 0x4800 <= opcode <= 0x4FFF:  # LDR literal
            return True
        if 0x5000 <= opcode <= 0x9FFF:  # reg/imm/halfword/SP-relative transfers
            return True
        if 0xB400 <= opcode <= 0xB5FF or 0xBC00 <= opcode <= 0xBDFF:  # PUSH/POP
            return True
        if 0xC000 <= opcode <= 0xCFFF:  # LDMIA/STMIA
            return True
        return False

    kind = (opcode >> 26) & 0x3
    if kind == 0x1:  # single data transfer
        return True
    if ((opcode >> 25) & 0x7) == 0x4:  # block data transfer
        return True
    # ARMv5 halfword / signed / doubleword transfers sit in the data-processing
    # space and are identified by bit 7 and bit 4 both set.
    if ((opcode >> 25) & 0x7) == 0x0 and (opcode & 0x90) == 0x90:
        return ((opcode >> 4) & 0xF) != 0x9  # 0b1001 is MUL/SWP, not a transfer
    return False


def format_table(rows: list[list[str]], headers: list[str], aligns: str) -> str:
    widths = [len(head) for head in headers]
    for row in rows:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(cell))

    def render(cells: list[str]) -> str:
        out = []
        for i, cell in enumerate(cells):
            out.append(cell.rjust(widths[i]) if aligns[i] == "r" else cell.ljust(widths[i]))
        return "  ".join(out).rstrip()

    lines = [render(headers), render(["-" * width for width in widths])]
    lines.extend(render(row) for row in rows)
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("profile", type=Path, help="arm9-profile.csv from melonDS")
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--readelf", default="arm-none-eabi-readelf")
    parser.add_argument("--top", type=int, default=40)
    parser.add_argument(
        "--hot-free",
        type=int,
        default=0,
        help="free bytes in .text.hot; 0 derives it from the ELF and the linker cap",
    )
    parser.add_argument(
        "--hot-cap", type=int, default=8192, help="linker ASSERT cap on .text.hot"
    )
    parser.add_argument(
        "--exclude",
        action="append",
        default=["armWaitForIrq"],
        help="symbols that burn cycles by design and must not skew tier stats",
    )
    parser.add_argument(
        "--itcm-free",
        type=int,
        default=0,
        help="free bytes in .itcm; 0 derives it from the ELF and --itcm-cap",
    )
    parser.add_argument("--itcm-cap", type=int, default=32736)
    parser.add_argument("--json", type=Path)
    parser.add_argument(
        "--pc-detail",
        metavar="SYMBOL",
        help="rank the individual program counters INSIDE one function by cycles, "
        "joined to the source line and mnemonic from objdump. Answers 'which "
        "instruction in this loop is expensive', which the per-symbol tables "
        "cannot: R2-07 R0h needed it after objdump had already settled the "
        "instruction COUNT (9 per pixel) and measurement still said 70 cycles.",
    )
    parser.add_argument(
        "--objdump",
        default="arm-none-eabi-objdump",
        help="only used by --pc-detail",
    )
    parser.add_argument(
        "--split-by-symbol",
        metavar="SYMBOL",
        help="needs NDS_TASK37_PROFILE_PER_FRAME_REGION=1. Split the census "
        "frames into the ones that executed SYMBOL and the ones that did not, "
        "and rank every symbol by the per-frame cycle difference between them. "
        "Answers 'where does an expensive class of frame actually spend the "
        "premium', which window totals cannot.",
    )
    parser.add_argument(
        "--split-over-gate",
        action="store_true",
        help="needs NDS_TASK37_PROFILE_PER_FRAME_REGION=1. Same table as "
        "--split-by-symbol, but the partition is the gate itself: frames costing "
        "more than 2 VBlanks against frames costing 2. Use it when no symbol is "
        "known to name the expensive class -- which is the usual case, because "
        "naming one is what the table is for.",
    )
    parser.add_argument(
        "--split-top-frames",
        type=int,
        metavar="N",
        help="needs NDS_TASK37_PROFILE_PER_FRAME_REGION=1. Same table again, but "
        "the partition is the tail itself: the N most expensive frames by "
        "NON-IDLE cycles against all the rest. This is the one that matches the "
        "gate -- P95 of 1600 frames is --split-top-frames 80. Prefer it over "
        "--split-over-gate, whose 2-VBlank threshold is both quantized and far "
        "too wide to be a tail.",
    )
    args = parser.parse_args()
    chosen = [n for n, v in (("--split-over-gate", args.split_over_gate),
                             ("--split-by-symbol", args.split_by_symbol),
                             ("--split-top-frames", args.split_top_frames)) if v]
    if len(chosen) > 1:
        parser.error(f"{' and '.join(chosen)} are partitions of the same frames; "
                     "pass one")

    try:
        sections = parse_sections(run_tool(args.readelf, "-SW", str(args.elf)))
        symbols = parse_symbols(run_tool(args.readelf, "-sW", str(args.elf)), sections)
        if not symbols:
            raise RuntimeError("no FUNC symbols found in the ELF")

        detail: dict | None = (
            {} if (args.split_by_symbol or args.split_over_gate
                   or args.split_top_frames) else None)
        total_cycles, unmapped_cycles, rows = attribute(symbols, args.profile, detail)
        if not rows:
            raise RuntimeError(f"{args.profile} contained no rows")

        by_name = {(name, address, size): None for name, address, size in sections.values()}
        del by_name
        section_sizes = {name: size for name, _addr, size in sections.values()}

        hot_used = section_sizes.get(".text.hot", 0)
        hot_free = args.hot_free or max(0, args.hot_cap - hot_used)

        executed = [s for s in symbols if s.cycles]
        executed.sort(key=lambda s: -s.cycles)

        print(f"profile      {args.profile}")
        print(f"elf          {args.elf}")
        print(f"pc rows      {rows:,}")
        print(f"cycles       {total_cycles:,}")
        share = 100.0 * unmapped_cycles / total_cycles if total_cycles else 0.0
        print(f"unattributed {unmapped_cycles:,} ({share:.2f}% -- BIOS, ITCM stubs, data)")
        print(f"symbols hit  {len(executed):,} of {len(symbols):,} FUNC symbols")
        print()
        print("section sizes")
        for name in PLACED_SECTIONS:
            size = section_sizes.get(name, 0)
            note = ""
            if name in (".text.hot", ".text.hot.draw"):
                note = f"  free {args.hot_cap - size:,} of cap {args.hot_cap:,}"
            print(f"  {name:<16} {size:>9,}{note}")
        for name, size in sorted(section_sizes.items(), key=lambda kv: -kv[1])[:4]:
            if name not in PLACED_SECTIONS and name.startswith((".text", ".main", ".itcm")):
                print(f"  {name:<16} {size:>9,}")

        # ---- Optional: per-PC detail inside one function ----
        if args.pc_detail:
            # Comma-separated, because the scan below is the expensive part and
            # it is the SAME scan for every symbol. Asking about a second
            # function used to mean a second ten-minute pass over 54.7M rows,
            # which is how cycle 117 ended up designing from one symbol's detail
            # when it wanted three.
            ranges = []
            for name in [n.strip() for n in args.pc_detail.split(",") if n.strip()]:
                target = next(
                    (s for s in symbols
                     if s.name == name or name in s.aliases),
                    None,
                )
                if target is None:
                    raise RuntimeError(
                        f"--pc-detail {name!r} is not a FUNC symbol in "
                        f"{args.elf}. The census only sees symbols the ELF defines."
                    )
                ranges.append((target.address, target.address + (target.size or 4),
                               target))
            lowest = min(r[0] for r in ranges)
            highest = max(r[1] for r in ranges)
            folded: dict[int, list] = {}
            with args.profile.open(newline="", encoding="utf-8") as stream:
                for row in csv.DictReader(stream):
                    pc = int(row["pc"], 16)
                    if not (lowest <= pc < highest):
                        continue
                    if not any(lo <= pc < hi for lo, hi, _t in ranges):
                        continue
                    # Fold the per-frame regions back together: this table is
                    # about WHERE in the function, not which frame.
                    slot = folded.setdefault(pc, [0, 0, row["mode"]])
                    slot[0] += int(row["total_cycles"])
                    slot[1] += int(row["instructions"])

            for lo, hi, target in ranges:
                listing = disassemble_range(args.objdump, args.elf, lo, hi)
                pcs = [(pc, v[0], v[1], v[2])
                       for pc, v in folded.items() if lo <= pc < hi]
                pcs.sort(key=lambda r: -r[1])
                counted = sum(r[1] for r in pcs)
                print()
                print(f"per-PC detail: {target.name} "
                      f"[{lo:#010x}, {hi:#010x}) {target.size:,} bytes")
                print(f"  {counted:,} cycles over {len(pcs):,} distinct PCs, "
                      f"{100.0 * counted / total_cycles if total_cycles else 0:.2f}% "
                      "of the window")
                table = []
                for pc, cycles, insns, mode in pcs[: args.top]:
                    source, mnemonic = listing.get(pc, ("?", "?"))
                    table.append([
                        f"{pc:#010x}",
                        f"{cycles:,}",
                        f"{100.0 * cycles / counted:.1f}" if counted else "-",
                        f"{insns:,}",
                        f"{cycles / insns:.2f}" if insns else "-",
                        mode,
                        source,
                        mnemonic,
                    ])
                print()
                print(format_table(
                    table,
                    ["pc", "cycles", "%fn", "insns", "cyc/insn", "mode", "source",
                     "instruction"],
                    "rrrrrlll",
                ))
            print()
            print("cyc/insn is the whole point: a row near 1.0 is doing work, a "
                  "row far above it is waiting. Rank by cycles, then read the "
                  "column that says which.")

        # ---- Table 0: is there a placement problem at all? ----
        # Placement can only ever recover non-memory stall cycles. If the tiers
        # already show the same non-memory stall rate, the icache is doing its
        # job and there is nothing here to move.
        # armWaitForIrq is the VBlank spin. It is 8 bytes of deliberate idling
        # in .itcm and it dominates that tier's totals, so leaving it in would
        # make the zero-waitstate tier look like the worst-stalling one.
        idle_spin = set(args.exclude)
        print()
        print(
            "== 0. stall accounting by tier (placement can only touch non-mem stall) ==\n"
            f"   excluding idle spin: {', '.join(sorted(idle_spin))}"
        )
        table = []
        for tier in (*PLACED_SECTIONS, ".main"):
            group = [
                s for s in executed if tier_of(s) == tier and s.name not in idle_spin
            ]
            cycles = sum(s.cycles for s in group)
            insns = sum(s.instructions for s in group)
            nonmem = sum(s.nonmem_stall for s in group)
            memstall = sum(
                max(0, s.mem_cycles - s.mem_instructions) for s in group
            )
            if not insns:
                continue
            table.append(
                [
                    tier,
                    f"{cycles:,}",
                    f"{insns:,}",
                    f"{cycles / insns:.2f}",
                    f"{nonmem:,}",
                    f"{100.0 * nonmem / cycles:.1f}",
                    f"{memstall:,}",
                    f"{100.0 * memstall / cycles:.1f}",
                ]
            )
        print(
            format_table(
                table,
                ["tier", "cycles", "insns", "cyc/insn", "nonmem stall", "%", "mem stall", "%"],
                "lrrrrrrr",
            )
        )

        # ---- Table A: cost toppers ----
        print()
        print(f"== A. top {args.top} symbols by measured cycles ==")
        table = []
        for symbol in executed[: args.top]:
            table.append(
                [
                    f"{symbol.cycles:,}",
                    f"{100.0 * symbol.cycles / total_cycles:.2f}",
                    f"{symbol.size:,}",
                    f"{symbol.cycles_per_byte:,.1f}",
                    tier_of(symbol),
                    symbol.label,
                ]
            )
        print(
            format_table(
                table,
                ["cycles", "%tot", "bytes", "cyc/byte", "tier", "symbol"],
                "rrrrll",
            )
        )

        # ---- Table E: per-frame split, when a marker symbol names the class ----
        split_summary = None
        if detail is not None:
            if args.split_over_gate:
                label = f"over the gate ({GATE_CYCLES:,} cycles)"
                hot, control = split_regions_over_gate(detail)
            elif args.split_top_frames:
                label = f"the {args.split_top_frames} costliest frames (non-idle)"
                hot, control = split_regions_top(
                    detail, symbols, idle_spin, args.split_top_frames)
            else:
                label = args.split_by_symbol
                hot, control = split_regions(detail, symbols, args.split_by_symbol)
            if not hot or not control:
                raise RuntimeError(
                    f"{label} selected {len(hot)} of "
                    f"{len(hot) + len(control)} census regions -- a split needs "
                    "both populations. If every region is on one side the build "
                    "is missing NDS_TASK37_PROFILE_PER_FRAME_REGION=1, so the "
                    "whole window collapsed into one region; if the regions are "
                    "there, this window simply holds one kind of frame -- widen "
                    "it or move it until it straddles the class you want."
                )

            def totals(regions: list[int]) -> dict[int, list[int]]:
                out: dict[int, list[int]] = {}
                for region in regions:
                    for index, (cyc, insn, mem) in detail[region].items():
                        slot = out.setdefault(index, [0, 0, 0])
                        slot[0] += cyc
                        slot[1] += insn
                        slot[2] += mem
                return out

            hot_totals, control_totals = totals(hot), totals(control)
            hot_sum = sum(v[0] for v in hot_totals.values())
            control_sum = sum(v[0] for v in control_totals.values())
            hot_mean = hot_sum / len(hot)
            control_mean = control_sum / len(control)
            print()
            print(
                f"== E. per-frame split on {label}: "
                f"{len(hot)} marked frames vs {len(control)} control =="
            )
            print(
                f"   marked   {hot_sum:,} cycles, {hot_mean:,.0f}/frame\n"
                f"   control  {control_sum:,} cycles, {control_mean:,.0f}/frame\n"
                f"   premium  {hot_mean - control_mean:,.0f}/frame"
            )
            # The ROM sets region r at the end of iteration START+r-1, so region r
            # accumulates iteration START+r. Print the ids raw: only the caller
            # knows its own START, and inventing one here is how an off-by-one
            # gets published as a frame number.
            print(f"   marked frames (region ids, frame = START + id): "
                  f"{', '.join(map(str, hot))}")
            premium = hot_mean - control_mean
            deltas = []
            for index in set(hot_totals) | set(control_totals):
                a = hot_totals.get(index, [0, 0, 0])
                b = control_totals.get(index, [0, 0, 0])
                delta = a[0] / len(hot) - b[0] / len(control)
                mem_delta = a[2] / len(hot) - b[2] / len(control)
                insn_delta = a[1] / len(hot) - b[1] / len(control)
                deltas.append((delta, mem_delta, insn_delta, symbols[index]))
            deltas.sort(key=lambda d: -d[0])
            table = []
            for delta, mem_delta, insn_delta, symbol in deltas[: args.top]:
                if abs(delta) < 1.0:
                    continue
                table.append(
                    [
                        f"{delta:,.0f}",
                        f"{100.0 * delta / premium:.1f}" if premium else "-",
                        f"{mem_delta:,.0f}",
                        f"{insn_delta:,.0f}",
                        f"{delta / insn_delta:,.1f}" if insn_delta else "-",
                        tier_of(symbol),
                        symbol.label,
                    ]
                )
            print()
            print(
                format_table(
                    table,
                    ["+cyc/frame", "%prem", "of it mem", "+insn", "cyc/insn",
                     "tier", "symbol"],
                    "rrrrrll",
                )
            )
            named = sum(d[0] for d in deltas if d[0] > 0)
            print()
            print(
                f"positive rows total {named:,.0f}/frame against a "
                f"{premium:,.0f} premium; the same instruction stream costing "
                "more is a cache effect, more instructions is real work"
            )
            split_summary = {
                "marker": label,
                "marked_regions": hot,
                "marked_frames": len(hot),
                "control_frames": len(control),
                "marked_mean": hot_mean,
                "control_mean": control_mean,
                "premium_per_frame": premium,
                "rows": [
                    {
                        "name": s.name,
                        "tier": tier_of(s),
                        "delta_cycles_per_frame": d,
                        "delta_mem_cycles_per_frame": m,
                        "delta_instructions_per_frame": i,
                    }
                    for d, m, i, s in deltas
                    if abs(d) >= 1.0
                ],
            }

        # ---- Table B: ITCM rent ----
        residents = [s for s in symbols if s.section == ".itcm"]
        residents.sort(key=lambda s: (-s.cycles_per_byte, -s.cycles, s.name))
        resident_bytes = occupied_bytes(residents)
        itcm_size = section_sizes.get(".itcm", 0)
        idle = [s for s in residents if not s.cycles]
        print()
        print(
            f"== B. .itcm rent: {len(residents)} residents covering "
            f"{resident_bytes:,} of {itcm_size:,} section bytes "
            f"({itcm_size - resident_bytes:,} unnamed); "
            f"{len(idle)} never executed ({occupied_bytes(idle):,} B idle) =="
        )
        table = []
        for symbol in residents:
            table.append(
                [
                    f"{symbol.cycles_per_byte:,.1f}",
                    f"{symbol.cycles:,}",
                    f"{symbol.size:,}",
                    f"{symbol.instructions:,}",
                    symbol.label,
                ]
            )
        print(format_table(table, ["cyc/byte", "cycles", "bytes", "insns", "symbol"], "rrrrl"))

        # ---- Table C: .main candidates that fit .text.hot ----
        print()
        print(
            f"== C. unplaced candidates for {hot_free:,} free .text.hot bytes, "
            "ranked by recoverable (non-mem) stall =="
        )
        candidates = [s for s in executed if tier_of(s) == ".main" and s.size]
        candidates.sort(key=lambda s: -s.nonmem_stall)
        table = []
        budget = hot_free
        taken = 0
        for symbol in candidates[: args.top]:
            fits = symbol.size <= budget
            if fits:
                budget -= symbol.size
                taken += symbol.nonmem_stall
            table.append(
                [
                    "fit" if fits else "-",
                    f"{symbol.nonmem_stall:,}",
                    f"{symbol.cycles:,}",
                    f"{symbol.size:,}",
                    f"{symbol.cycles_per_insn:.2f}",
                    f"{symbol.nonmem_stall / symbol.size:,.1f}",
                    symbol.label,
                ]
            )
        print(
            format_table(
                table,
                ["pack", "nonmem stall", "cycles", "bytes", "cyc/insn", "stall/byte", "symbol"],
                "lrrrrrl",
            )
        )
        print()
        print(
            f"greedy pack claims {hot_free - budget:,} of {hot_free:,} free bytes "
            f"and reaches {taken:,} non-mem stall cycles "
            f"({100.0 * taken / total_cycles:.2f}% of measured)"
        )

        # ---- Table D: ITCM admissions, ranked by stall recovered per byte ----
        itcm_free = args.itcm_free or max(0, args.itcm_cap - itcm_size)
        idle_bytes = occupied_bytes(idle)
        print()
        print(
            f"== D. ITCM admissions by non-mem stall per byte "
            f"({itcm_free:,} B free now, +{idle_bytes:,} B recoverable by eviction) =="
        )
        admissions = [
            s for s in executed if tier_of(s) != ".itcm" and s.size and s.nonmem_stall
        ]
        admissions.sort(key=lambda s: -s.nonmem_stall / s.size)
        table = []
        free_budget = itcm_free
        evict_budget = itcm_free + idle_bytes
        free_gain = 0
        evict_gain = 0
        for symbol in admissions[: args.top]:
            slot = "-"
            if symbol.size <= free_budget:
                slot = "free"
                free_budget -= symbol.size
                free_gain += symbol.nonmem_stall
            if symbol.size <= evict_budget:
                slot = slot if slot == "free" else "evict"
                evict_budget -= symbol.size
                evict_gain += symbol.nonmem_stall
            table.append(
                [
                    slot,
                    f"{symbol.nonmem_stall / symbol.size:,.0f}",
                    f"{symbol.nonmem_stall:,}",
                    f"{symbol.size:,}",
                    f"{symbol.cycles_per_insn:.2f}",
                    tier_of(symbol),
                    symbol.label,
                ]
            )
        print(
            format_table(
                table,
                ["slot", "stall/byte", "nonmem stall", "bytes", "cyc/insn", "tier", "symbol"],
                "lrrrrll",
            )
        )
        print()
        print(
            f"zero-eviction pack: {itcm_free - free_budget:,} B, "
            f"{free_gain:,} non-mem stall cycles in reach"
        )
        print(
            f"with eviction:      {itcm_free + idle_bytes - evict_budget:,} B, "
            f"{evict_gain:,} non-mem stall cycles in reach"
        )

        if args.json:
            payload = {
                "profile": str(args.profile),
                "elf": str(args.elf),
                "total_cycles": total_cycles,
                "unattributed_cycles": unmapped_cycles,
                "section_sizes": section_sizes,
                "symbols": [
                    {
                        "name": s.name,
                        "aliases": s.aliases,
                        "address": s.address,
                        "size": s.size,
                        "section": s.section,
                        "tier": tier_of(s),
                        "cycles": s.cycles,
                        "instructions": s.instructions,
                        "cycles_per_byte": s.cycles_per_byte,
                    }
                    for s in symbols
                    if s.cycles or s.section in PLACED_SECTIONS
                ],
            }
            if split_summary is not None:
                payload["split"] = split_summary
            args.json.write_text(json.dumps(payload, indent=1), encoding="utf-8")
            print(f"wrote {args.json}")
        return 0
    except (OSError, RuntimeError, ValueError, KeyError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
