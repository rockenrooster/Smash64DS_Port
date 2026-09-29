"""Counterfactual P95 per symbol from a per-frame-region task37 profile.

Pass 1 (slow, cached): per-(symbol, region) cycles -> <profile-dir>/symreg.pkl
Pass 2: work(frame) = cycles - idle; for each symbol, P95 of work without it.

usage: python cf95.py <nm.txt> <profile-dir> [top]
"""
import bisect
import collections
import os
import pickle
import sys

nm, pdir = sys.argv[1], sys.argv[2]
top = int(sys.argv[3]) if len(sys.argv) > 3 else 60
cache_path = os.path.join(pdir, 'symreg.pkl')
if os.path.exists(cache_path):
    per = pickle.load(open(cache_path, 'rb'))
else:
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

    per = collections.defaultdict(collections.Counter)
    with open(os.path.join(pdir, 'arm9-profile.csv')) as f:
        next(f)
        for line in f:
            p = line.split(',', 6)
            per[sym(p[1])][int(p[0])] += int(p[5])
    per = {k: dict(v) for k, v in per.items()}
    pickle.dump(per, open(cache_path, 'wb'))

regions = sorted({r for v in per.values() for r in v})
idle = per.get('armWaitForIrq', {})
work = {r: 0 for r in regions}
for s, v in per.items():
    if s == 'armWaitForIrq':
        continue
    for r, c in v.items():
        work[r] += c
n = len(regions)


def p95(values):
    v = sorted(values)
    return v[int((n - 1) * 0.95)]


base = p95(work.values())
base50 = sorted(work.values())[n // 2]
print(f'frames {n}; work P50 {base50/2:,.0f} P95 {base/2:,.0f} ticks (cycles/2)')
rows = []
for s, v in per.items():
    if s == 'armWaitForIrq':
        continue
    tot = sum(v.values())
    if tot < n * 2000:
        continue
    cf = p95(work[r] - v.get(r, 0) for r in regions)
    rows.append(((base - cf) / 2, tot / n / 2, s))
rows.sort(reverse=True)
print('  P95 drop if removed   mean/frame   symbol')
for d, m, s in rows[:top]:
    print(f'{d:12,.0f} {m:12,.0f}   {s}')
