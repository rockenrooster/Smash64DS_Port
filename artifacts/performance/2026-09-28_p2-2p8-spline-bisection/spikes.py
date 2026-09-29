"""Per-(symbol, frame) cycles from a per-frame-region profile: for each symbol,
how many frames it costs more than a threshold in, and its excess there.

usage: python spikes.py <nm.txt> <profile-dir> <gate_cycles> [thresh_ticks]
"""
import bisect
import collections
import sys

nm, pdir, gate = sys.argv[1], sys.argv[2], int(sys.argv[3])
thresh = int(sys.argv[4]) if len(sys.argv) > 4 else 10000
fn = []
for line in open(nm):
    p = line.split()
    if len(p) == 4 and p[2] in 'tTwW':
        fn.append((int(p[0], 16), int(p[1], 16), p[3]))
fn.sort()
starts = [f[0] for f in fn]
cache = {}


def sym(pc):
    s = cache.get(pc)
    if s is None:
        a = int(pc, 16) & ~1
        i = bisect.bisect_right(starts, a) - 1
        s = fn[i][2] if (i >= 0 and fn[i][0] <= a < fn[i][0] + fn[i][1]) else '?'
        cache[pc] = s
    return s


over = set()
nreg = 0
with open(pdir + '/arm9-profile.regions.csv') as f:
    next(f)
    for line in f:
        p = line.split(',')
        nreg += 1
        if int(p[2]) > gate:
            over.add(p[0])
per = collections.defaultdict(lambda: collections.Counter())
with open(pdir + '/arm9-profile.csv') as f:
    next(f)
    for line in f:
        p = line.split(',', 6)
        per[sym(p[1])][p[0]] += int(p[5])
rows = []
for s, regs in per.items():
    if s == 'armWaitForIrq':
        continue
    vals = sorted(regs.values())
    med = vals[len(vals) // 2] if len(regs) > nreg // 2 else 0
    big = [(r, c) for r, c in regs.items() if (c - med) / 2 > thresh]
    if not big:
        continue
    ex = sum(c - med for r, c in big) / 2
    exo = sum(c - med for r, c in big if r in over) / 2
    nbo = sum(1 for r, c in big if r in over)
    rows.append((exo, ex, len(big), nbo, med / 2, s))
rows.sort(reverse=True)
print(f'regions {nreg} over {len(over)}; spike threshold {thresh} ticks over the symbol median')
print('  excess-in-over  excess-all  spike-frames  of-them-over  median/fr  symbol')
for exo, ex, nb, nbo, med, s in rows[:50]:
    print(f'{exo/1000:12.0f}K {ex/1000:10.0f}K {nb:8d} {nbo:8d} {med/1000:9.1f}K  {s}')
