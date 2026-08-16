#!/usr/bin/env python3
"""Split a function's per-PC profile cycles into named address blocks."""
import csv, sys, collections

PC_CSV = sys.argv[1]

# (name, lo, hi_inclusive)
BLOCKS = [
    ("CLOCK      (prologue+anim_wait/frame+cmp, EVERY call)", 0x208ca18, 0x208ca5e),
    ("EPILOGUE   (shared by every return)",                   0x208cabc, 0x208cac8),
    ("EARLYOUT   (counter++ then branch to epilogue)",        0x208cff6, 0x208cffe),
    ("CHANGED    (anim_wait = -anim_frame arm)",              0x208caca, 0x208cad0),
]
FUNC = (0x208ca18, 0x208d3e7)
OTHERS = {
    "ndsR2AnimBuildTrackTable": (0x208c8e8, 0x208c92f),
    "ndsR2AnimTargetValue":     (0x208c930, 0x208ca17),
    "ndsR2AnimAObjToQConvert":  (0x208c594, 0x208c8e7),
    "ndsR2AnimAdvanceTail":     (0x208c4e4, 0x208c593),
}

acc = collections.defaultdict(lambda: [0, 0, 0, 0])   # marg_tk, whole_tk, marg_insn, dfill
func_tot = [0, 0, 0, 0]
blk = {b[0]: [0, 0, 0, 0] for b in BLOCKS}

with open(PC_CSV, newline="") as fh:
    for r in csv.DictReader(fh):
        pc = int(r["pc"], 16)
        mt = int(r["marg_total_cycles"]) - int(r["marg_halt_wait"])
        wt = int(r["all_total_cycles"]) - int(r["all_halt_wait"])
        mi = int(r["marg_instructions"])
        df = int(r["marg_dcache_fill"])
        if FUNC[0] <= pc <= FUNC[1]:
            func_tot[0] += mt; func_tot[1] += wt; func_tot[2] += mi; func_tot[3] += df
            for name, lo, hi in BLOCKS:
                if lo <= pc <= hi:
                    v = blk[name]
                    v[0] += mt; v[1] += wt; v[2] += mi; v[3] += df
        for name, (lo, hi) in OTHERS.items():
            if lo <= pc <= hi:
                v = acc[name]
                v[0] += mt; v[1] += wt; v[2] += mi; v[3] += df

MARG = 160.0     # cycles / (2 * 80 frames) -> ticks per marginal frame
WHOLE = 3202.0   # cycles / (2 * 1601)

def row(name, v):
    print("%-56s %9.0f %9.0f %9.1f %9.0f" % (
        name, v[0]/MARG, v[1]/WHOLE, v[2]/80.0, v[3]/80.0))

print("%-56s %9s %9s %9s %9s" % ("block", "marg tk/fr", "whole", "insn/fr", "dfill/fr"))
row("ndsR2FtAnimParseDObjFigatree TOTAL", func_tot)
for name, lo, hi in BLOCKS:
    row("  " + name, blk[name])
covered = [sum(blk[b[0]][i] for b in BLOCKS) for i in range(4)]
rest = [func_tot[i] - covered[i] for i in range(4)]
row("  STEPPED-EXCLUSIVE (everything else in the body)", rest)
print()
for name in OTHERS:
    row(name, acc[name])
