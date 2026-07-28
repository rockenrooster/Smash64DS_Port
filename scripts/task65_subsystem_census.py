#!/usr/bin/env python3
"""Task 65: split the frame into idle wait, stall, and work, then attribute the
work to subsystems and to cross-cutting kernels.

Task 64 established that the tick-HUD `ALL` bucket is VBlank-quantized wall
time, so it cannot be used to search for savings -- only to accept them. This
script produces the search-side instrument instead: per-frame ARM9 ticks, with
the idle VBlank wait separated out, attributed two ways.

  by subsystem   -- where in the codebase the ticks are spent, resolved from
                    DWARF line info rather than guessed from symbol names, so a
                    renderer helper called from the fighter path is charged to
                    the file it lives in and nothing is double counted.

  by kernel      -- soft float, mem*, texture resolution, matrix arithmetic and
                    GX submission cut ACROSS every subsystem. A plan organised
                    only by bucket rediscovers the same kernels from each end
                    and pays for them twice, which is exactly what a bucket-only
                    view hides.

Units. The melonDS profiler counts real ARM9 core cycles at 67.028 MHz. The
ROM's tick HUD counts the 33.514 MHz system timer through cpuGetTiming(). The
factor is therefore exactly 2, and every number this script prints is in tick
HUD units so it can be compared to the P95 <= 1.12M budget in PROJECT_GOAL.md
without further conversion.
"""

from __future__ import annotations

import argparse
import csv
import re
import subprocess
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path

# Profiler cycles (67.028 MHz ARM9 core) per cpuGetTiming tick (33.514 MHz).
CYCLES_PER_TICK = 2

IDLE_SYMBOLS = {"armWaitForIrq"}

# Kernel classification, first match wins. These are the costs that appear in
# every bucket, so they are tested before any subsystem rule.
KERNEL_RULES = (
    ("idle-vblank-wait", lambda fn, src: fn in IDLE_SYMBOLS),
    ("soft-float", lambda fn, src: bool(SOFTFLOAT_RE.match(fn))),
    ("mem*", lambda fn, src: fn in MEM_SYMBOLS),
    ("texture-resolve", lambda fn, src: "Texture" in fn or "TexName" in fn),
    ("matrix", lambda fn, src: "Mtx" in fn or "Matrix" in fn),
    ("gx-submit", lambda fn, src: bool(GX_SUBMIT_RE.search(fn))),
    ("rom-read", lambda fn, src: "ntrcard" in fn or "RomRead" in fn),
)

SOFTFLOAT_RE = re.compile(
    r"^(__aeabi_(f|d|i2f|ui2f|f2|d2)|__(add|sub|mul|div|neg|cmp|fix|float)[sd]f|"
    r"__ieee754_|__eqsf|__ltsf|__gtsf|__lesf|__gesf|__nesf|__unordsf)"
)
MEM_SYMBOLS = {"memset", "memcpy", "memmove", "memcmp", "strlen", "strcmp", "strcpy"}
GX_SUBMIT_RE = re.compile(
    r"(SubmitVertex|SubmitHardwareTriangle|EmitProduction|EmitNoZ|ScanList|"
    r"ReplayRun|CommitNative|SubmitTriangle)"
)

# Subsystem rules run against the DWARF source path, normalised to forward
# slashes and made relative to the repo root where possible. First match wins.
SUBSYSTEM_RULES = (
    ("SIM/fighter (decomp ft)", "decomp/battleship-main/decomp/src/ft"),
    ("SIM/game (decomp gm)", "decomp/battleship-main/decomp/src/gm"),
    ("SIM/effects (decomp ef)", "decomp/battleship-main/decomp/src/ef"),
    ("SIM/items (decomp it)", "decomp/battleship-main/decomp/src/it"),
    ("SIM/scene (decomp sc)", "decomp/battleship-main/decomp/src/sc"),
    ("SIM/system (decomp sy)", "decomp/battleship-main/decomp/src/sy"),
    ("SIM/other (decomp)", "decomp/battleship-main/decomp"),
    ("SIM/imported (src/import)", "src/import"),
    ("REND/renderer", "src/nds/nds_renderer"),
    ("REND/fighter draw", "src/nds/nds_fighter"),
    ("REND/stage draw", "src/nds/nds_stage"),
    # reloc_backend_renderer_*.c is renderer adapter code that happens to live
    # under a reloc_ filename. It must be matched before the generic
    # "src/port/reloc" rule below, which otherwise charges the draw path to
    # relocation -- in the R2-00b baseline that mis-filed 147,777 ticks/frame,
    # 10.2% of the frame's work, into a bucket named after loading.
    ("REND/adapter", "src/port/reloc_backend_renderer"),
    ("PORT/taskman seam", "src/port/taskman_seam"),
    ("PORT/reloc", "src/port/reloc"),
    ("PORT/other", "src/port"),
    ("NDS/platform", "src/nds/nds_platform"),
    ("NDS/other", "src/nds"),
    ("LIB/devkitpro", "devkitpro"),
    ("LIB/libnds", "libnds"),
)


@dataclass
class Bucket:
    cycles: int = 0
    instructions: int = 0
    mem_cycles: int = 0
    mem_instructions: int = 0
    pcs: int = 0

    @property
    def stall(self) -> int:
        return max(0, self.cycles - self.instructions)

    @property
    def mem_stall(self) -> int:
        return max(0, self.mem_cycles - self.mem_instructions)

    @property
    def nonmem_stall(self) -> int:
        return max(0, self.stall - self.mem_stall)


@dataclass
class Function:
    name: str
    source: str
    bucket: Bucket = field(default_factory=Bucket)


def is_memory_op(opcode: int, thumb: bool) -> bool:
    """Same predicate as task37_census.py: does this instruction touch data?

    Stall on a memory instruction is the bus or the cache waiting on data.
    Stall on anything else is fetch, interlock or pipeline refill. Only the
    first kind can be GX FIFO backpressure, because backpressure is a write to
    the command port that cannot retire.
    """
    if thumb:
        opcode &= 0xFFFF
        if 0x4800 <= opcode <= 0x4FFF:
            return True
        if 0x5000 <= opcode <= 0x9FFF:
            return True
        if 0xB400 <= opcode <= 0xB5FF or 0xBC00 <= opcode <= 0xBDFF:
            return True
        if 0xC000 <= opcode <= 0xCFFF:
            return True
        return False

    kind = (opcode >> 26) & 0x3
    if kind == 0x1:
        return True
    if ((opcode >> 25) & 0x7) == 0x4:
        return True
    if ((opcode >> 25) & 0x7) == 0x0 and (opcode & 0x90) == 0x90:
        return ((opcode >> 4) & 0xF) != 0x9
    return False


def resolve_pcs(pcs: list[int], elf: Path, addr2line: str) -> dict[int, tuple[str, str]]:
    """pc -> (function, source path). One batched addr2line call."""
    stdin = "\n".join(f"0x{pc:08x}" for pc in pcs)
    try:
        result = subprocess.run(
            [addr2line, "-f", "-e", str(elf)],
            input=stdin,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
        )
    except FileNotFoundError as exc:
        raise RuntimeError(f"could not run {addr2line}") from exc
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(exc.stderr.strip() or "addr2line failed") from exc

    lines = result.stdout.splitlines()
    if len(lines) != 2 * len(pcs):
        raise RuntimeError(
            f"addr2line returned {len(lines)} lines for {len(pcs)} addresses"
        )
    out: dict[int, tuple[str, str]] = {}
    for index, pc in enumerate(pcs):
        name = lines[2 * index].strip()
        location = lines[2 * index + 1].strip()
        source = location.rsplit(":", 1)[0] if location else "??"
        out[pc] = (name if name != "??" else f"pc_{pc:08x}", source)
    return out


def classify_kernel(function: str, source: str) -> str:
    for label, rule in KERNEL_RULES:
        if rule(function, source):
            return label
    return "other"


def normalise(source: str, root: Path) -> str:
    text = source.replace("\\", "/")
    lowered = text.lower()
    marker = str(root).replace("\\", "/").lower() + "/"
    if lowered.startswith(marker):
        return text[len(marker):]
    return text


def classify_subsystem(source: str, function: str, root: Path) -> str:
    if source in ("??", ""):
        # No line info: libc and libgcc ship without it. The kernel classifier
        # already names these precisely, so defer to it rather than guessing.
        kernel = classify_kernel(function, source)
        return f"LIB/{kernel}" if kernel != "other" else "LIB/unattributed"
    relative = normalise(source, root).lower()
    for label, prefix in SUBSYSTEM_RULES:
        if prefix in relative:
            return label
    return "other"


def table(rows: list[list[str]], headers: list[str], aligns: str) -> str:
    widths = [len(head) for head in headers]
    for row in rows:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(cell))

    def render(cells: list[str]) -> str:
        out = [
            cell.rjust(widths[i]) if aligns[i] == "r" else cell.ljust(widths[i])
            for i, cell in enumerate(cells)
        ]
        return "  ".join(out).rstrip()

    lines = [render(headers), render(["-" * w for w in widths])]
    lines.extend(render(row) for row in rows)
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("profile", type=Path)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--addr2line", default="arm-none-eabi-addr2line")
    parser.add_argument("--frames", type=int, required=True,
                        help="presented frames in the census window")
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument("--top", type=int, default=40)
    parser.add_argument("--budget", type=int, default=1_120_000,
                        help="PROJECT_GOAL.md per-frame tick budget for 30 FPS")
    args = parser.parse_args()

    try:
        rows = list(csv.DictReader(args.profile.open(newline="", encoding="utf-8")))
        if not rows:
            raise RuntimeError(f"{args.profile} contained no rows")

        pcs = sorted({int(row["pc"], 16) for row in rows})
        resolved = resolve_pcs(pcs, args.elf, args.addr2line)

        functions: dict[str, Function] = {}
        by_kernel: dict[str, Bucket] = defaultdict(Bucket)
        by_subsystem: dict[str, Bucket] = defaultdict(Bucket)
        total = Bucket()

        for row in rows:
            pc = int(row["pc"], 16)
            name, source = resolved[pc]
            cycles = int(row["total_cycles"])
            instructions = int(row["instructions"])
            memory = is_memory_op(int(row["opcode"], 16), row["mode"] == "thumb")

            kernel = classify_kernel(name, source)
            subsystem = (
                "IDLE" if kernel == "idle-vblank-wait"
                else classify_subsystem(source, name, args.root)
            )

            function = functions.get(name)
            if function is None:
                function = functions[name] = Function(name, normalise(source, args.root))
            for bucket in (function.bucket, by_kernel[kernel],
                           by_subsystem[subsystem], total):
                bucket.cycles += cycles
                bucket.instructions += instructions
                bucket.pcs += 1
                if memory:
                    bucket.mem_cycles += cycles
                    bucket.mem_instructions += instructions

        def ticks(cycles: int) -> int:
            return round(cycles / CYCLES_PER_TICK / args.frames)

        idle = by_kernel["idle-vblank-wait"].cycles
        wall_ticks = ticks(total.cycles)
        idle_ticks = ticks(idle)
        work_ticks = wall_ticks - idle_ticks

        print(f"profile        {args.profile}")
        print(f"elf            {args.elf}")
        print(f"presented      {args.frames} frames")
        print(f"pc rows        {len(rows):,}  ({len(pcs):,} distinct)")
        print(f"cycles         {total.cycles:,} @ 67.028 MHz")
        print()
        print("== 1. the frame, in tick-HUD units (cpuGetTiming, 33.514 MHz) ==")
        print(f"  wall per frame        {wall_ticks:>10,}")
        print(f"  idle VBlank wait      {idle_ticks:>10,}   "
              f"{100.0 * idle / total.cycles:.2f}% of wall")
        print(f"  REAL WORK             {work_ticks:>10,}")
        print(f"  30 FPS budget         {args.budget:>10,}   (PROJECT_GOAL.md)")
        gap = work_ticks - args.budget
        verdict = "over" if gap > 0 else "under"
        print(f"  GAP                   {gap:>10,}   work is {verdict} budget")
        print()
        stall_total = total.stall - by_kernel["idle-vblank-wait"].stall
        busy = total.cycles - idle
        print("  of the real work:")
        print(f"    retired instructions {ticks(total.instructions - by_kernel['idle-vblank-wait'].instructions):>9,}"
              f"   {100.0 * (total.instructions - by_kernel['idle-vblank-wait'].instructions) / busy:.1f}%")
        print(f"    memory stall         {ticks(total.mem_stall - by_kernel['idle-vblank-wait'].mem_stall):>9,}"
              f"   {100.0 * (total.mem_stall - by_kernel['idle-vblank-wait'].mem_stall) / busy:.1f}%"
              "   bus/cache/GX backpressure")
        print(f"    non-memory stall     {ticks(stall_total - (total.mem_stall - by_kernel['idle-vblank-wait'].mem_stall)):>9,}"
              f"   {100.0 * (stall_total - (total.mem_stall - by_kernel['idle-vblank-wait'].mem_stall)) / busy:.1f}%"
              "   fetch/interlock/refill")

        for title, groups in (
            ("2. by cross-cutting kernel", by_kernel),
            ("3. by subsystem (DWARF source path)", by_subsystem),
        ):
            print()
            print(f"== {title} ==")
            body = []
            for label, bucket in sorted(groups.items(), key=lambda kv: -kv[1].cycles):
                body.append([
                    label,
                    f"{ticks(bucket.cycles):,}",
                    f"{100.0 * bucket.cycles / total.cycles:.2f}",
                    f"{100.0 * bucket.cycles / busy:.2f}" if label not in ("IDLE", "idle-vblank-wait") else "-",
                    f"{bucket.cycles / bucket.instructions:.2f}" if bucket.instructions else "-",
                    f"{ticks(bucket.mem_stall):,}",
                ])
            print(table(
                body,
                ["group", "ticks/frame", "%wall", "%work", "cyc/insn", "mem stall"],
                "lrrrrr",
            ))

        print()
        print(f"== 4. top {args.top} functions by ticks per frame ==")
        ranked = sorted(functions.values(), key=lambda f: -f.bucket.cycles)
        body = []
        for function in ranked[: args.top]:
            bucket = function.bucket
            body.append([
                f"{ticks(bucket.cycles):,}",
                f"{100.0 * bucket.cycles / busy:.2f}" if function.name not in IDLE_SYMBOLS else "-",
                f"{bucket.cycles / bucket.instructions:.2f}" if bucket.instructions else "-",
                classify_kernel(function.name, function.source),
                function.name,
                function.source,
            ])
        print(table(
            body,
            ["ticks/frame", "%work", "cyc/insn", "kernel", "function", "source"],
            "rrrlll",
        ))
        return 0
    except (OSError, RuntimeError, ValueError, KeyError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
