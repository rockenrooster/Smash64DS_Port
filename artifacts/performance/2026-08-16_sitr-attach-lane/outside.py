"""What the attach lane cannot reach: the top-80 frames that carry NO attach and
NO force-load, and which leaf owns them.

Same basis and same leaf closure as convert.py.  No emulator needed.
"""
import csv
import statistics as st
from collections import Counter

RING = 'artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'
PF = 'artifacts/performance/2026-08-16_sitr-excursion/pf220-rows.csv'
PFB = 'artifacts/performance/2026-08-16_sitr-excursion/pf220b-rows.csv'
APPARATUS = 24947
GATE = 1120380

ring = {int(r['frame']): {k: int(v) for k, v in r.items()}
        for r in csv.DictReader(open(RING))}


def deltas(path, cols):
    rows = sorted(({k: int(v) for k, v in r.items()}
                   for r in csv.DictReader(open(path))), key=lambda r: r['frame'])
    return {rows[i]['frame']: {c: rows[i][c] - rows[i - 1][c] for c in cols}
            for i in range(1, len(rows))}


d1 = deltas(PF, ['gNdsR204AnimForceLoadTotal'])
d2 = deltas(PFB, ['gNdsFighterNaturalMotionFigatreeAttachCount'])


def c(f, k):
    return d1.get(f - 1, {}).get(k, d2.get(f - 1, {}).get(k, 0))


LEAF = ['FTR', 'STG', 'BG', 'AUD', 'MISC', 'OTHRW', 'SRCRES', 'SWRM',
        'GCRARES', 'SITR', 'SCPU', 'SPHD', 'SPHC', 'SCAT', 'SHDT', 'SPRM']


def leaves(r):
    six = r['SINT'] + r['SPHD'] + r['SPHC'] + r['SCAT'] + r['SHDT'] + r['SPRM']
    return dict(zip(LEAF, [
        r['FTR'], r['STG'], r['BG'], r['AUD'], r['MISC'], r['OTHR'] - r['WAIT'],
        r['SRC'] - r['GCRA'] - r['SWRM'], r['SWRM'], r['GCRA'] - six,
        r['SINT'] - r['SCPU'], r['SCPU'], r['SPHD'], r['SPHC'], r['SCAT'],
        r['SHDT'], r['SPRM']]))


L = {f: leaves(r) for f, r in ring.items()}
med = {n: st.median([L[f][n] for f in L]) for n in LEAF}
allf = sorted(L)
evt = set(f for f in allf
          if c(f, 'gNdsFighterNaturalMotionFigatreeAttachCount') > 0
          or c(f, 'gNdsR204AnimForceLoadTotal') > 0)
top = sorted(allf, key=lambda f: -ring[f]['WORK-H'])[:80]
base = ring[top[79]]['WORK-H']


def owner(f):
    return max(((L[f][n] - med[n], n) for n in LEAF))[1]


inn = [f for f in top if f in evt]
out = [f for f in top if f not in evt]
print('basis rank-80 %d  level %+d' % (base, base - APPARATUS - GATE))
print('top-80: %d carry an attach or a force-load, %d carry neither'
      % (len(inn), len(out)))

for label, grp in (('IN the lane (attach/force-load)', inn),
                   ('OUTSIDE the lane', out)):
    print('\n%s -- %d frames, WORK-H median %s' % (
        label, len(grp), format(int(st.median([ring[f]['WORK-H'] for f in grp])), ',')))
    print('  dominant leaf: %s' % dict(Counter(owner(f) for f in grp).most_common()))
    print('  %-9s %11s %11s %8s' % ('leaf', 'median', 'run median', 'ratio'))
    for n in LEAF:
        m = st.median([L[f][n] for f in grp])
        if med[n] and (m / med[n] > 1.15 or m - med[n] > 8000):
            print('  %-9s %11s %11s %7.2fx'
                  % (n, format(int(m), ','), format(int(med[n]), ','), m / med[n]))

# What is the level if the lane is fully cleared -- i.e. every attach/force-load
# frame is capped at the highest OUTSIDE-lane frame?
cap = max(ring[f]['WORK-H'] for f in out)
ser = sorted((min(ring[f]['WORK-H'], cap) if f in evt else ring[f]['WORK-H']
              for f in allf), reverse=True)
print('\nIf every event frame were capped at the worst non-event frame (%s):'
      % format(cap, ','))
print('  rank-80 %s  level %+d' % (format(ser[79], ','), ser[79] - APPARATUS - GATE))
print('  -> the non-event population alone still holds the level %d over gate'
      % (ser[79] - APPARATUS - GATE))
