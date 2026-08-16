import csv, statistics, os, collections

D = r'D:\Stuff\DevFolder\Smash64DS_Port\artifacts\performance\2026-08-16_hwmath-route'
GATE = 1120380
APPARATUS = 24947
BUCK = ['WORK-H', 'ALL', 'FTR', 'STG', 'SRC', 'MISC', 'OTHR', 'GCRA', 'SINT', 'WAIT']

raw = {}
for a in range(4):
    raw[a] = list(csv.DictReader(open(os.path.join(D, f'r{a}-rows.csv'), newline='')))

# Pair by FRAME LABEL, not row index: the ring-stop seam shifts the label by +-1
# between runs, so index pairing compares different simulation frames across a
# whole 96-frame block. The match is bit-identical on every arm (same binary,
# same script, bit-identical arithmetic), so frame N is the same frame on all
# four. Labels that repeat inside one arm are dropped from the paired set.
def bymap(a, b):
    c = collections.Counter(r['frame'] for r in raw[a])
    return {int(r['frame']): int(r[b]) for r in raw[a] if c[r['frame']] == 1}

def rank(vals, n=80):
    return sorted(vals, reverse=True)[n - 1]

wh_all = {a: [int(r['WORK-H']) for r in raw[a]] for a in range(4)}
wh = {a: bymap(a, 'WORK-H') for a in range(4)}
common = sorted(set(wh[0]) & set(wh[1]) & set(wh[2]) & set(wh[3]))
print(f'rows per arm = {len(raw[0])}; paired frames common to all four = {len(common)}')

print()
print(f"{'arm':>4} {'rank80 raw':>12} {'net':>12} {'gap':>10} {'P50':>11} {'mean':>11}")
for a in range(4):
    r80 = rank(wh_all[a])
    print(f'{a:>4} {r80:>12,} {r80-APPARATUS:>12,} {r80-APPARATUS-GATE:>+10,} '
          f'{int(statistics.median(wh_all[a])):>11,} {int(statistics.mean(wh_all[a])):>11,}')

# marginal-80 population, defined on the CONTROL arm's own paired frames
ctrl = [wh[0][f] for f in common]
thr = rank(ctrl)
marg = [f for f in common if wh[0][f] >= thr]
print(f'\nmarginal-80 population (arm 0, paired set): {len(marg)} frames, threshold {thr:,}')

print()
hdr = (f"{'pair':>8} {'paired median':>15} {'mean':>11} {'win share':>10} "
       f"{'marg80 median':>15} {'marg80 mean':>12} {'dRank80':>10}")
print(hdr)
res = {}
for a in (1, 2, 3):
    d = [wh[a][f] - wh[0][f] for f in common]
    dm = [wh[a][f] - wh[0][f] for f in marg]
    win = sum(1 for x in d if x < 0) / len(d)
    res[a] = statistics.median(d)
    print(f'{"0->"+str(a):>8} {int(statistics.median(d)):>+15,} {int(statistics.mean(d)):>+11,} '
          f'{win:>9.1%} {int(statistics.median(dm)):>+15,} {int(statistics.mean(dm)):>+12,} '
          f'{rank(wh_all[a])-rank(wh_all[0]):>+10,}')

print(f'\nadditivity: med(0->1) + med(0->2) = {int(res[1]+res[2]):+,}   vs med(0->3) = {int(res[3]):+,}')
for lo, hi, name in ((1, 3, 'B on top of A'), (2, 3, 'A on top of B')):
    d = [wh[hi][f] - wh[lo][f] for f in common]
    dm = [wh[hi][f] - wh[lo][f] for f in marg]
    print(f'{name:>16}: arm{lo}->arm{hi} paired median {int(statistics.median(d)):+,}  '
          f'marg80 median {int(statistics.median(dm)):+,}')

print('\nper-bucket paired medians vs arm 0 (whole run / marginal-80):')
print(f"{'bucket':8} {'arm1':>19} {'arm2':>19} {'arm3':>19}")
for b in BUCK:
    m = {a: bymap(a, b) for a in range(4)}
    line = f'  {b:8s}'
    for a in (1, 2, 3):
        d = [m[a][f] - m[0][f] for f in common]
        dm = [m[a][f] - m[0][f] for f in marg]
        line += f' {int(statistics.median(d)):+8,}/{int(statistics.median(dm)):+8,}'
    print(line)

# re-rank estimate: subtract the marginal-80 paired median from every control row
for a in (1, 2, 3):
    dm = statistics.median([wh[a][f] - wh[0][f] for f in marg])
    print(f'\narm{a}: flat-cut re-rank of arm 0 by {int(dm):+,} -> rank80 '
          f'{rank([v + dm for v in wh_all[0]]):,.0f} (measured arm{a} rank80 {rank(wh_all[a]):,})')
