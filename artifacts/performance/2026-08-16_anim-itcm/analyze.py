import csv, statistics, os, collections, sys

D = os.path.dirname(os.path.abspath(__file__))
GATE = 1120380
APPARATUS = 24947
BUCK = ['WORK-H', 'ALL', 'FTR', 'STG', 'SRC', 'MISC', 'OTHR', 'GCRA', 'SINT', 'WAIT']

ARMS = sys.argv[1:] or ['t0', 't1']

raw = {}
for a in ARMS:
    raw[a] = list(csv.DictReader(open(os.path.join(D, f'{a}-rows.csv'), newline='')))


# Pair by FRAME LABEL, not row index (the ring-stop seam shifts the label by
# +-1 between runs). Labels that repeat inside one arm are dropped.
def bymap(a, b):
    c = collections.Counter(r['frame'] for r in raw[a])
    return {int(r['frame']): int(r[b]) for r in raw[a] if c[r['frame']] == 1}


def rank(vals, n=80):
    return sorted(vals, reverse=True)[n - 1]


def band(vals, lo=41, hi=120):
    s = sorted(vals, reverse=True)
    return statistics.mean(s[lo - 1:hi])


wh_all = {a: [int(r['WORK-H']) for r in raw[a]] for a in ARMS}
wh = {a: bymap(a, 'WORK-H') for a in ARMS}
common = sorted(set.intersection(*[set(wh[a]) for a in ARMS]))
print(f'rows per arm = {len(raw[ARMS[0]])}; paired frames common to all = {len(common)}')

print()
print(f"{'arm':>6} {'rank80 raw':>12} {'net':>12} {'gap':>10} {'ranks41-120':>13} {'P50':>11} {'mean':>11}")
for a in ARMS:
    r80 = rank(wh_all[a])
    print(f'{a:>6} {r80:>12,} {r80-APPARATUS:>12,} {r80-APPARATUS-GATE:>+10,} '
          f'{int(band(wh_all[a])):>13,} {int(statistics.median(wh_all[a])):>11,} '
          f'{int(statistics.mean(wh_all[a])):>11,}')

ctrl_arm = ARMS[0]
ctrl = [wh[ctrl_arm][f] for f in common]
thr = rank(ctrl)
marg = [f for f in common if wh[ctrl_arm][f] >= thr]
print(f'\nmarginal-80 population (arm {ctrl_arm}, paired set): {len(marg)} frames, threshold {thr:,}')

print()
print(f"{'pair':>10} {'paired median':>15} {'mean':>11} {'win share':>10} "
      f"{'marg80 median':>15} {'marg80 mean':>12} {'dRank80':>10} {'dBand':>10}")
for a in ARMS[1:]:
    d = [wh[a][f] - wh[ctrl_arm][f] for f in common]
    dm = [wh[a][f] - wh[ctrl_arm][f] for f in marg]
    win = sum(1 for x in d if x < 0) / len(d)
    print(f'{ctrl_arm+"->"+a:>10} {int(statistics.median(d)):>+15,} {int(statistics.mean(d)):>+11,} '
          f'{win:>9.1%} {int(statistics.median(dm)):>+15,} {int(statistics.mean(dm)):>+12,} '
          f'{rank(wh_all[a])-rank(wh_all[ctrl_arm]):>+10,} '
          f'{int(band(wh_all[a])-band(wh_all[ctrl_arm])):>+10,}')

print('\nper-bucket paired medians vs the control arm (whole run / marginal-80):')
print(f"{'bucket':10}" + ''.join(f'{a:>19}' for a in ARMS[1:]))
for b in BUCK:
    m = {a: bymap(a, b) for a in ARMS}
    line = f'  {b:8s}'
    for a in ARMS[1:]:
        d = [m[a][f] - m[ctrl_arm][f] for f in common]
        dm = [m[a][f] - m[ctrl_arm][f] for f in marg]
        line += f' {int(statistics.median(d)):+8,}/{int(statistics.median(dm)):+8,}'
    print(line)

for a in ARMS[1:]:
    dm = statistics.median([wh[a][f] - wh[ctrl_arm][f] for f in marg])
    print(f'\n{a}: flat-cut re-rank of the control by {int(dm):+,} -> rank80 '
          f'{rank([v + dm for v in wh_all[ctrl_arm]]):,.0f} '
          f'(measured {a} rank80 {rank(wh_all[a]):,})')
