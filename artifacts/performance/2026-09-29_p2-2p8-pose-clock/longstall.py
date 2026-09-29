"""Instructions whose average cost exceeds a threshold (bus/DMA/FIFO waits),
summed per PC across the whole profile, with over-gate split.

usage: python longstall.py <nm.txt> <dis> <profile-dir> <gate_cycles> [min_avg]
"""
import bisect
import collections
import re
import sys

nm, dis, pdir, gate = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
min_avg = float(sys.argv[5]) if len(sys.argv) > 5 else 100.0
fn = []
for line in open(nm):
    p = line.split()
    if len(p) == 4 and p[2] in 'tTwW':
        fn.append((int(p[0], 16), int(p[1], 16), p[3]))
fn.sort()
starts = [f[0] for f in fn]


def sym(a):
    i = bisect.bisect_right(starts, a) - 1
    return fn[i][2] if (i >= 0 and fn[i][0] <= a < fn[i][0] + fn[i][1]) else '?'


text = {}
for line in open(dis, encoding='utf-8', errors='replace'):
    m = re.match(r'^\s*([0-9a-f]+):\s+(.*)$', line)
    if m:
        text[int(m.group(1), 16)] = m.group(2).strip()
over = set()
n = 0
with open(pdir + '/arm9-profile.regions.csv') as f:
    next(f)
    for line in f:
        p = line.split(',')
        n += 1
        if int(p[2]) > gate:
            over.add(p[0])
tot = collections.Counter()
tot_o = collections.Counter()
ins = collections.Counter()
with open(pdir + '/arm9-profile.csv') as f:
    next(f)
    for line in f:
        p = line.split(',', 6)
        c = int(p[5])
        k = int(p[4])
        if k == 0 or c / k < min_avg:
            continue
        pc = int(p[1], 16)
        tot[pc] += c
        ins[pc] += k
        if p[0] in over:
            tot_o[pc] += c
grand = sum(tot.values())
print(f'frames {n} over {len(over)}; instructions averaging >= {min_avg} cycles: '
      f'{grand/n/2:,.0f} ticks/frame, {sum(tot_o.values())/max(len(over),1)/2:,.0f} ticks per over-gate frame')
print('  ticks/fr  over/fr   avg   pc        symbol / instruction')
for pc, c in tot.most_common(40):
    if pc == 0:
        continue
    print(f'{c/n/2:9,.0f} {tot_o[pc]/max(len(over),1)/2:8,.0f} {c/ins[pc]:6.0f}  {pc:08x}  '
          f'{sym(pc & ~1)} | {text.get(pc & ~1, "?")}')
