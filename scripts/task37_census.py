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


def attribute(symbols: list[Symbol], profile: Path) -> tuple[int, int, int]:
    """Fold the per-PC CSV into the symbol list. Returns (cycles, unmapped, rows)."""
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
            symbol.regions.add(int(row["region"]))
            if is_memory_op(int(row["opcode"], 16), row["mode"] == "thumb"):
                symbol.mem_cycles += cycles
                symbol.mem_instructions += instructions

    return total_cycles, unmapped_cycles, rows


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
    args = parser.parse_args()

    try:
        sections = parse_sections(run_tool(args.readelf, "-SW", str(args.elf)))
        symbols = parse_symbols(run_tool(args.readelf, "-sW", str(args.elf)), sections)
        if not symbols:
            raise RuntimeError("no FUNC symbols found in the ELF")

        total_cycles, unmapped_cycles, rows = attribute(symbols, args.profile)
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
            args.json.write_text(json.dumps(payload, indent=1), encoding="utf-8")
            print(f"wrote {args.json}")
        return 0
    except (OSError, RuntimeError, ValueError, KeyError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
