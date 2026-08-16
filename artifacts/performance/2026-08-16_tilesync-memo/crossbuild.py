import csv, statistics, os, collections

D = os.path.dirname(os.path.abspath(__file__))
BASIS = os.path.join(D, '..', '2026-08-16_hwmath-route', 'c215-ship-rows.csv')
SHIP = os.path.join(D, 'ship-rows.csv')
GATE, APP = 1120380, 24947


def load(p):
    return list(csv.DictReader(open(p, newline='')))


def rank(v, n=80):
    return sorted(v, reverse=True)[n - 1]


def band(v, lo=41, hi=120):
    return statistics.mean(sorted(v, reverse=True)[lo - 1:hi])


def bymap(rows, b):
    c = collections.Counter(r['frame'] for r in rows)
    return {int(r['frame']): int(r[b]) for r in rows if c[r['frame']] == 1}


arms = {'c215-hwmath-ship (basis)': load(BASIS), 'c217-tilesync-ship': load(SHIP)}
print(f"{'build':30}{'rank80 raw':>12}{'net':>12}{'gap':>10}{'r41-120':>12}{'P50':>11}")
w = {}
for k, r in arms.items():
    v = [int(x['WORK-H']) for x in r]
    w[k] = v
    print(f'{k:30}{rank(v):>12,}{rank(v)-APP:>12,}{rank(v)-APP-GATE:>+10,}'
          f'{int(band(v)):>12,}{int(statistics.median(v)):>11,}')

a, b = list(arms)
print(f'\ncross-build {a} -> {b}')
print(f"  rank-80      {rank(w[b])-rank(w[a]):+,}")
print(f"  ranks 41-120 {int(band(w[b])-band(w[a])):+,}")
print(f"  P50          {int(statistics.median(w[b])-statistics.median(w[a])):+,}")
for n in (20, 40, 60, 80, 100, 120, 160, 240, 400, 800, 1200):
    print(f'  rank{n:>5}: {sorted(w[a],reverse=True)[n-1]:>11,} -> '
          f'{sorted(w[b],reverse=True)[n-1]:>11,}   '
          f'{sorted(w[b],reverse=True)[n-1]-sorted(w[a],reverse=True)[n-1]:>+9,}')

ma, mb = bymap(arms[a], 'WORK-H'), bymap(arms[b], 'WORK-H')
common = sorted(set(ma) & set(mb))
d = [mb[f] - ma[f] for f in common]
print(f'\npaired by frame label ({len(common)} frames): median {int(statistics.median(d)):+,}  '
      f'mean {int(statistics.mean(d)):+,}  win {sum(1 for x in d if x<0)/len(d):.1%}')
thr = rank([ma[f] for f in common])
marg = [f for f in common if ma[f] >= thr]
dm = [mb[f] - ma[f] for f in marg]
print(f'marginal-80 (basis-defined, {len(marg)} frames): median {int(statistics.median(dm)):+,}  '
      f'mean {int(statistics.mean(dm)):+,}')
print('\nper-bucket paired medians (whole run / marginal-80):')
for bu in ('FTR', 'STG', 'SRC', 'MISC', 'OTHR', 'WAIT', 'ALL'):
    x, y = bymap(arms[a], bu), bymap(arms[b], bu)
    dd = [y[f] - x[f] for f in common]
    dmm = [y[f] - x[f] for f in marg]
    print(f'  {bu:6s} {int(statistics.median(dd)):+9,} / {int(statistics.median(dmm)):+9,}')
