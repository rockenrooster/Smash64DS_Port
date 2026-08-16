"""What a saving inside the FTR capture pass is worth at rank-80.

Same instrument as ../2026-08-16_ftr-attribution/convert-ftr.py: an exact
re-sort of the c220 basis's own 1,600 rows with a UNIFORM D subtracted from
every frame, capped at that frame's own FTR, reading the 80th value. A capture
saving touches every frame, so a uniform D is the right shape and a
clip-to-median excess is not (that form overstated a lane 5x once).

The candidate rows below are `hit_rate x ceiling - key_cost`, with the hit rate
MEASURED by this cycle's census rather than assumed.

Basis build-c220-camship, ../2026-08-16_camera-ship/ship220-rows.csv,
apparatus 24,947, gate 1,120,380, rank-80 1,210,624 raw, REQUIREMENT +65,297.
No emulator. 2 profile cycles = 1 project tick.
"""
import csv
import statistics as st

RING = 'artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'
APP, GATE, TOPN = 24947, 1120380, 80

rows = [{k: int(v) for k, v in r.items()} for r in csv.DictReader(open(RING))]
W = [r['WORK-H'] for r in rows]
F = [r['FTR'] for r in rows]
order = sorted(range(len(W)), key=lambda i: -W[i])
base = sorted(W, reverse=True)[TOPN - 1]
band = sum(sorted(W, reverse=True)[40:120]) / 80.0
print('CONTROL  rank-80 %d raw / %d net   level %+d   band 41-120 %.0f'
      % (base, base - APP, base - APP - GATE, band))
print('         (expect 1,210,624 / 1,185,677 / +65,297 / 1,218,356)')
ftr_band = sum(F[i] for i in order[40:120]) / 80.0
print('FTR P50 %d, band 41-120 %.0f, ratio %.3f'
      % (st.median(F), ftr_band, ftr_band / st.median(F)))


def rerank(d):
    return sorted((W[i] - min(d, F[i]) for i in range(len(W))),
                  reverse=True)[TOPN - 1]


# Measured by this cycle (c222-run.log). 4,076 comparisons, 51 changes.
HIT = 4025.0 / 4076.0
CEIL_FULL = 34307   # ndsBaseFTDisplayMainProcDisplay 30,190 + CountFlags 4,117
CEIL_WALK = 19300   # ftDisplayMainDrawAll -> ftDisplayMainDrawDefault only
CEIL_FLAGS = 4117   # ndsFighterDisplayContractCountFlags, diagnostic-only
# The sound key is the head's OWN output -- the contract scalars it has already
# written when the walk starts -- so it costs a compare, not a walk. The tree
# key this cycle measured is both unsound (49 of 51 changes invisible to it)
# and expensive: FTR P50 rose 290,432 -> 301,120 with the census in, and most
# of that +10,688 is the tree walk's extra cold pointer loads.
KEY = 0


def curve(cands):
    print('')
    print('%-58s %8s %9s %8s %7s %11s'
          % ('candidate', 'D', 'rank-80', 'moved', 'ratio', 'level'))
    for name, d in cands:
        d = int(max(d, 0))
        r = rerank(d)
        print('%-58s %8d %9d %8d %7.3f %+11d'
              % (name[:58], d, r, base - r,
                 (base - r) / d if d else 0.0, r - APP - GATE))


if __name__ == '__main__':
    print('measured hit rate %.4f (4,025 unchanged of 4,076 comparisons)' % HIT)
    curve([
        ('the >=14,080 cross-build placement floor, for reference', 14080),
        ('CountFlags deleted -- diagnostic-only tree walk, NO memo needed',
         CEIL_FLAGS),
        ('WALK memo at the measured 0.9875 (SOUND, recommended)',
         HIT * CEIL_WALK - KEY),
        ('WALK memo + CountFlags deleted', HIT * CEIL_WALK + CEIL_FLAGS - KEY),
        ('WALK memo, ceiling if the contract never changed', CEIL_WALK),
        ('WHOLE capture memo at 0.9875 -- needs the head side effects proven',
         HIT * CEIL_FULL - KEY),
        ('WHOLE capture memo, ceiling (FTR_LANE section 5s figure)', CEIL_FULL),
        ('the requirement itself', 65297),
    ])
