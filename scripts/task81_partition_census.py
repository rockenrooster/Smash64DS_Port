#!/usr/bin/env python3
"""Task 81: re-derive the frame as a PARTITION, with stall separated from
instruction cost.

`COMPILER_FIRST_ARCHITECTURE.md` opens with a table of "approximate per-frame
work" whose own caption warns that the categories overlap -- soft-float, `mem*`
and matrix work all occur inside renderer work, so the rows cannot be summed and
a task may not claim two of them as independent budgets. That warning made the
table unusable for scheduling: every candidate lever looked bigger than it was,
because its cost was also counted inside another row.

This script produces the replacement the plan asks for. Every symbol lands in
exactly one class, the classes sum to the frame, and each carries its stall and
instruction cost separately -- because a class that is mostly stall is fixed by
placement or data layout, and a class that is mostly instructions is fixed only
by executing fewer of them, and the two are different tasks.

Reads census.json emitted by task65_subsystem_census.py, which already resolves
every PC to a symbol through DWARF. Units are tick-HUD ticks per presented
frame, matching PROJECT_GOAL.md's 1,120,000 gate without conversion.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

CYCLES_PER_TICK = 2

SOFTFLOAT_RE = re.compile(
    r"^(__aeabi_(f|d|i2f|ui2f|f2|d2)|__(add|sub|mul|div|neg|cmp|fix|float)[sd]f|"
    r"__ieee754_|__eqsf|__ltsf|__gtsf|__lesf|__gesf|__nesf|__unordsf|"
    r"__kernel_|__udivmoddi4|__udivsi3|__divsi3|sqrtf$|cosf$|sinf$)"
)
MEM_SYMBOLS = {
    "memset", "memcpy", "memmove", "memcmp", "strlen", "strcmp", "strcpy",
    "bzero", "__aeabi_memcpy", "__aeabi_memset", "__aeabi_memclr",
}

# First match wins. The order is the partition: a symbol is charged to the most
# specific class that claims it, and never to two.
CLASSES = (
    ("idle (VBlank wait)",
     lambda n, s: n == "armWaitForIrq"),

    # Leaf kernels first -- they are called from everywhere, so charging them to
    # a caller-shaped class is exactly the overlap this table exists to remove.
    ("soft-float + libgcc",
     lambda n, s: bool(SOFTFLOAT_RE.match(n))),
    ("mem* (libc)",
     lambda n, s: n in MEM_SYMBOLS),

    # Fighter native production: the generated path, the one Tasks 77-79 found
    # already built.
    ("fighter: native production",
     lambda n, s: n.startswith("ndsRendererNative") or
                  "NativeFighterOwner" in n or
                  n.startswith("ndsRendererExecuteNative")),
    # Fighter generic scaffolding: what COMPILER_FIRST Task 79 proposed to
    # delete. Task 91 E1 measured the walk+validate part of this at 13,888.
    ("fighter: generic scaffolding",
     lambda n, s: n.startswith("ndsFighter") or
                  n.startswith("ndsRendererAdapter") or
                  n.startswith("ftDisplayMain") or
                  n.startswith("ndsBaseFTDisplay")),

    ("stage: native replay",
     lambda n, s: "Task36Replay" in n or "NativeStage" in n or
                  "CommitNativeStage" in n),
    ("stage: geometry + collision",
     lambda n, s: n.startswith("ndsStage") or n.startswith("ndsMP") or
                  n.startswith("mpCollision")),

    ("renderer: texture + material",
     lambda n, s: "Texture" in n or "TexName" in n or "Texel" in n or
                  "Material" in n or "Palette" in n or "Tile" in n or
                  "Combine" in n),
    ("renderer: matrix",
     lambda n, s: "Mtx" in n or "Matrix" in n),
    ("renderer: GX submit + vertex",
     lambda n, s: "SubmitVertex" in n or "SubmitHardwareTriangle" in n or
                  "SubmitTriangle" in n or "ScanList" in n or
                  "TransformVertex" in n or "Batch" in n or
                  n.startswith("gl") or "Vertex" in n),
    ("renderer: other",
     lambda n, s: n.startswith("ndsRenderer")),

    ("animation (gc* + figatree)",
     lambda n, s: n.startswith("gc") or "Anim" in n or "Figatree" in n or
                  "figatree" in n),
    ("fighter simulation (ft*)",
     lambda n, s: n.startswith("ft") or n.startswith("battleship_ft") or
                  n.startswith("ndsFT")),
    ("game/scene/system (gm, sc, sy)",
     lambda n, s: n.startswith("gm") or n.startswith("sc") or
                  n.startswith("sy") or n.startswith("ndsScene")),
    ("resource relocation + cart",
     lambda n, s: n.startswith("ndsReloc") or "ntrcard" in n or
                  "RomRead" in n or n.startswith("ndsAsset")),
    ("audio",
     lambda n, s: n.startswith("ndsAudio") or n.startswith("ndsFGM") or
                  n.startswith("ndsBGM")),
    ("platform + HUD",
     lambda n, s: n.startswith("ndsPlatform") or n.startswith("ndsIFCommon") or
                  "Hud" in n or "HUD" in n),
)


def classify(name: str, section: str) -> str:
    for label, rule in CLASSES:
        if rule(name, section):
            return label
    return "unclassified"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("census", type=Path)
    ap.add_argument("--frames", type=int, default=128)
    ap.add_argument("--compare", type=Path, default=None)
    ap.add_argument("--json", type=Path, default=None)
    args = ap.parse_args()

    def load(path: Path):
        data = json.loads(path.read_text())
        buckets: dict[str, dict[str, float]] = {}
        for sym in data["symbols"]:
            label = classify(sym["name"], sym.get("section", ""))
            b = buckets.setdefault(
                label, {"cycles": 0, "instructions": 0, "syms": 0})
            b["cycles"] += sym["cycles"]
            b["instructions"] += sym["instructions"]
            b["syms"] += 1
        return data, buckets

    data, buckets = load(args.census)
    prior = load(args.compare)[1] if args.compare else None

    div = CYCLES_PER_TICK * args.frames
    total = data["total_cycles"] / div
    idle = buckets.get("idle (VBlank wait)", {"cycles": 0})["cycles"] / div
    work = total - idle

    print()
    print(f"Task 81 -- frame as a partition   ({args.census})")
    print(f"total {total:,.0f} ticks/frame   idle {idle:,.0f}   "
          f"work {work:,.0f}   gate 1,120,000")
    print()
    header = ("class                             ticks/f   %work     stall  "
              "stall%   instr")
    if prior:
        header += "     vs prior"
    print(header)
    print("-" * (len(header)))

    rows = sorted(
        ((lbl, b) for lbl, b in buckets.items() if lbl != "idle (VBlank wait)"),
        key=lambda kv: -kv[1]["cycles"])
    checksum = 0.0
    for label, b in rows:
        ticks = b["cycles"] / div
        instr = b["instructions"] / div
        stall = max(0.0, ticks - instr)
        checksum += ticks
        line = (f"{label:<32} {ticks:9,.0f} {100*ticks/work:6.1f}% "
                f"{stall:9,.0f} {100*stall/ticks if ticks else 0:6.1f}% "
                f"{instr:7,.0f}")
        if prior:
            p = prior.get(label)
            delta = ticks - (p["cycles"] / div) if p else ticks
            line += f"  {delta:+9,.0f}"
        print(line)

    print("-" * len(header))
    print(f"{'sum of classes':<32} {checksum:9,.0f} "
          f"{100*checksum/work:6.1f}%   <- partition check (must equal work)")
    print()
    print(f"gap to gate: {work - 1_120_000:,.0f} ticks/frame")
    print()

    if args.json:
        args.json.write_text(json.dumps({
            "census": str(args.census),
            "frames": args.frames,
            "totalTicksPerFrame": total,
            "idleTicksPerFrame": idle,
            "workTicksPerFrame": work,
            "gate": 1_120_000,
            "classes": {
                lbl: {
                    "ticksPerFrame": b["cycles"] / div,
                    "instructionsPerFrame": b["instructions"] / div,
                    "stallTicksPerFrame": max(
                        0.0, (b["cycles"] - b["instructions"]) / div),
                    "symbols": b["syms"],
                } for lbl, b in buckets.items()
            },
        }, indent=2))
        print(f"wrote {args.json}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
