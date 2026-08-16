"""FALSIFIER: does the SHDT excursion scale with the number of pair tests?

`[[a-residual-divided-by-a-count-is-not-a-price]]` -- six retractions in this
campaign came from dividing a residual by a count and calling the quotient a
price. The audit's own force-load section is the most recent: "~228,600 per cache
hit" fell apart the moment the frames were split by load COUNT and the implied
per-load figure dropped 2.1x from one load to two.

So before any per-pair figure is quoted for SHDT, split the profile's regions by
their exact pair-test count and check whether the premium is linear in it.

The count is exact, not sampled: a function's ENTRY pc executes exactly once per
call, so the profiler's instruction count at that pc IS that region's call count
(`[[entry-pc-gives-exact-call-counts]]`).

Basis: build-c191-sitr-profile-c185 (COLLISION_FIXED 0, GX_COMPOSE 0,
BOTH_CPU 1, BATTLEPACK 1, KEEP_CACHE 1, TICK_HUD_DRAW 0), regions=1601,
ticks = cycles / (2 x 1600).
"""
import sys
import numpy as np

CACHE = sys.argv[1]
z = np.load(CACHE, allow_pickle=True)
cyc, calls, names = z['cyc'], z['calls'], list(z['names'])
# region 0 is outside the census window, exactly as the split tool drops it
cyc, calls = cyc[1:1601], calls[1:1601]
REG = cyc.shape[0]
TK = 2 * REG                      # ticks = cycles / (2 * regions)
idx = {n: i for i, n in enumerate(names)}

PAIR = 'gmCollisionCheckFighterAttackDamageCollide'
IDLE = 'armWaitForIrq'
npair = calls[:, idx[PAIR]]
work = cyc.sum(1) - cyc[:, idx[IDLE]]          # non-idle cycles per region

print(f'regions {REG}   total pair tests {npair.sum():,}   '
      f'frames with >=1 pair test {int((npair > 0).sum())}')
print(f'whole-match rate {npair.sum()/REG:.2f} calls/frame  '
      f'(EXCHANGE.md section 3.5 counter gNdsCfxFighterDamagePhaseCalls = 1,938/match)')

base = np.median(work[npair == 0])
print(f'\nbaseline non-idle (median of the {int((npair==0).sum())} zero-pair '
      f'frames) = {base/2:,.0f} tk/fr\n')

print(f'{"pairs on the frame":>20s} {"n":>5s} {"non-idle tk/fr":>15s} '
      f'{"lift":>10s} {"IMPLIED per pair":>17s}')
buckets = [(1, 4), (5, 9), (10, 19), (20, 29), (30, 49), (50, 999)]
for lo, hi in buckets:
    m = (npair >= lo) & (npair <= hi)
    if m.sum() == 0:
        continue
    mean_pairs = npair[m].mean()
    lift = work[m].mean() - base
    print(f'{f"{lo}-{hi}":>20s} {int(m.sum()):>5d} {work[m].mean()/2:>15,.0f} '
          f'{lift/2:>10,.0f} {lift/2/mean_pairs:>17,.0f}')

# the same, resolved per exact count where n is large enough to mean anything
print(f'\nresolved per exact pair count (n >= 8):')
print(f'{"pairs":>6s} {"n":>5s} {"non-idle tk/fr":>15s} {"lift":>10s} '
      f'{"IMPLIED per pair":>17s}')
for k in sorted(set(npair.tolist())):
    if k == 0:
        continue
    m = npair == k
    if m.sum() < 8:
        continue
    lift = work[m].mean() - base
    print(f'{k:>6d} {int(m.sum()):>5d} {work[m].mean()/2:>15,.0f} '
          f'{lift/2:>10,.0f} {lift/2/k:>17,.0f}')

# regression through the engaged population -- slope IS the marginal price if
# the relationship is linear; r2 says whether "a price" is even the right word.
m = npair > 0
A = np.vstack([npair[m], np.ones(m.sum())]).T
slope, intercept = np.linalg.lstsq(A, work[m] / 2, rcond=None)[0]
pred = A @ [slope, intercept]
ss = 1 - ((work[m] / 2 - pred) ** 2).sum() / ((work[m] / 2 - (work[m] / 2).mean()) ** 2).sum()
print(f'\nOLS over the {int(m.sum())} engaged frames: '
      f'non-idle tk/fr = {slope:,.0f} x pairs + {intercept:,.0f}   R2 = {ss:.3f}')
print(f'  (baseline from the zero-pair frames is {base/2:,.0f}; the intercept is '
      f'{intercept - base/2:+,.0f} above it)')

# what does a pair test actually execute? exact call ratios per pair.
print(f'\nEXACT per-pair-test call counts, engaged frames, chain symbols:')
tot_pairs = npair[m].sum()
for s in ['gmCollisionTestRectangle', 'gmCollisionGetWorldPosition',
          'gmCollisionSetInvertMatrix', 'func_ovl2_800ED490', 'func_ovl2_800EDE5C',
          'func_ovl2_800EDBA4', 'gmCollisionTransformMatrixAll',
          'gmCollisionCheckFighterInFighterRange', '__aeabi_fadd', '__aeabi_fmul',
          '__aeabi_fdiv', 'sqrtf']:
    if s not in idx:
        continue
    on = calls[m, idx[s]].sum()
    off = calls[~m, idx[s]].sum()
    rate_off = off / max((~m).sum(), 1)
    marginal = on - rate_off * m.sum()      # calls above the non-engaged rate
    print(f'  {s:44s} {on/tot_pairs:>8.2f} raw   '
          f'{marginal/tot_pairs:>8.2f} above baseline rate')
