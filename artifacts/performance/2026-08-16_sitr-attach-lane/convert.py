"""Conversion curve: what a per-frame saving on the 288 event frames is WORTH
at rank-80.

The SITR diagnosis (../2026-08-16_sitr-excursion/SITR_EXCURSION.md) sized every
candidate as a mean tk/fr delta over the 288 attach/force-load frames.  The gate
is a LEVEL at rank-80 of 1,600 frames, so a mean over a sub-population does not
convert 1:1 unless that population sits entirely above the rank.  This script
re-ranks the basis's own rows with a FIXED subtraction D applied to the 288, for
the D of every candidate, and reads the 80th value.

Basis build-c220-camship, apparatus 24,947, gate 1,120,380.  No emulator needed.
"""
import csv
import statistics as st

RING = 'artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'
PF = 'artifacts/performance/2026-08-16_sitr-excursion/pf220-rows.csv'
PFB = 'artifacts/performance/2026-08-16_sitr-excursion/pf220b-rows.csv'
APPARATUS = 24947
GATE = 1120380

ring = {int(r['frame']): {k: int(v) for k, v in r.items()}
        for r in csv.DictReader(open(RING))}


def deltas(path, cols):
    rows = sorted(({k: int(v) for k, v in r.items()}
                   for r in csv.DictReader(open(path))),
                  key=lambda r: r['frame'])
    return {rows[i]['frame']: {c: rows[i][c] - rows[i - 1][c] for c in cols}
            for i in range(1, len(rows))}


d1 = deltas(PF, ['gNdsR204AnimForceLoadTotal'])
d2 = deltas(PFB, ['gNdsFighterNaturalMotionFigatreeAttachCount'])
OFF = 1


def c(f, k):
    return d1.get(f - OFF, {}).get(k, d2.get(f - OFF, {}).get(k, 0))


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

base = sorted((r['WORK-H'] for r in ring.values()), reverse=True)[79]
print('CONTROL  rank-80 %d  net %d  level %+d   (expect 1,210,624 / 1,185,677 / +65,297)'
      % (base, base - APPARATUS, base - APPARATUS - GATE))
print('event frames %d of %d' % (len(evt), len(allf)))

# How many of the 288 are above rank-80 at all?  A saving on a frame below the
# rank moves nothing.
above = sum(1 for f in evt if ring[f]['WORK-H'] >= base)
print('of those, %d sit at or above rank-80 -- a saving on the other %d cannot '
      'move the level until the ones above it come down' % (above, len(evt) - above))


def rerank_fixed(frames, d):
    ser = []
    for f in allf:
        w = ring[f]['WORK-H']
        if f in frames:
            w -= min(d, max(0, L[f]['SITR'] - med['SITR']))
        ser.append(w)
    ser.sort(reverse=True)
    return ser[79]


print('\nA FIXED saving of D tk/fr on all 288 event frames (capped at each '
      "frame's own SITR excess):")
print('%-46s %10s %8s %11s %8s' % ('candidate', 'D', 'rank-80', 'moved',
                                   'level'))
CAND = [
    ('ndsRelocAssetIDForToken (resolver)', 4118),
    ('resolver + AObjToQConvert None store', 4118 + 2631),
    ('the whole ATTACH chain group', 23801),
    ('ANIM evaluate group', 26813),
    ('ANIM parse group', 28094),
    ('parse + evaluate (the two "engineering" groups)', 54907),
    ('parse + evaluate + attach', 78708),
]
for name, d in CAND:
    r = rerank_fixed(evt, d)
    print('%-46s %10d %8d %8d %+11d'
          % (name, d, r, base - r, r - APPARATUS - GATE))

print('\nfor reference, the diagnosis\'s own ceiling (remove EVERY frame\'s '
      'full SITR excess):')
r = rerank_fixed(evt, 10 ** 9)
print('%-46s %10s %8d %8d %+11d'
      % ('all SITR excess on the 288', 'inf', r, base - r, r - APPARATUS - GATE))

print('\nmarginal conversion rate (how many ticks of rank-80 per tick of D):')
for d in (1000, 2000, 4118, 6749, 10000, 20000, 30000, 54907, 78708):
    r = rerank_fixed(evt, d)
    print('  D=%-7d rank-80 %d  moved %6d  ratio %.3f'
          % (d, r, base - r, (base - r) / d))


def rerank_uncapped(frames, d):
    ser = [ring[f]['WORK-H'] - (d if f in frames else 0) for f in allf]
    ser.sort(reverse=True)
    return ser[79]


print('\nSAME CURVE UNCAPPED (allow the saving to cut below the SITR median -- '
      'strictly more optimistic than any real fix):')
for d in (4118, 6749, 23801, 26813, 28094, 54907, 78708):
    r = rerank_uncapped(evt, d)
    print('  D=%-7d rank-80 %d  moved %6d  ratio %.3f  level %+d'
          % (d, r, base - r, (base - r) / d, r - APPARATUS - GATE))

# THE FLOOR OF ANY EVENT-FRAME-ONLY LEVER: make all 288 cost nothing.
rest = sorted((ring[f]['WORK-H'] for f in allf if f not in evt), reverse=True)
print('\nFLOOR of ANY lever that only touches the 288 event frames:')
print('  make all 288 free              rank-80 %d  moved %d  level %+d'
      % (rest[79], base - rest[79], rest[79] - APPARATUS - GATE))
print('  (that is the 80th of the %d frames the lever cannot reach)' % len(rest))
over = [f for f in allf if ring[f]['WORK-H'] > GATE + APPARATUS]
overrest = [f for f in over if f not in evt]
print('  over-gate frames %d, of which %d carry no attach and no force-load'
      % (len(over), len(overrest)))
