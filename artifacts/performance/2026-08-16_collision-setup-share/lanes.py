"""What every leaf lane is worth at rank-80, and what the fighter collision
per-joint SETUP is worth at its own ceiling.

Basis build-c220-camship, ../2026-08-16_camera-ship/ship220-rows.csv, whole
match, gate arm NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 KEEP_CACHE=1, mode 163
one-minute match, 1,600 samples, frames 439-2038, DLDI ON, slips=0.
Apparatus 24,947, gate 1,120,380.  No emulator, no build: every row is an exact
re-sort of the basis's own 1,600 rows.

Three interventions per lane, all exact:
  FRACTION  remove f x lane[frame] from every frame -- what a cheaper
            implementation of that lane actually produces
  EXCESS    remove everything the lane spends above its own run P50 -- the
            ceiling of removing all of its VARIANCE
  WHOLE     remove the lane entirely -- the absolute ceiling

The EXCESS column is a per-frame VARIABLE and is a ceiling, not a lever
(../2026-08-16_sitr-attach-lane/ATTACH_LANE.md section 1 corrected exactly this
mistake on SITR).  Quote FRACTION when sizing an engineering change.
"""
import csv
import numpy as np

PATH = 'artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'
TOPN, GATE, APP = 80, 1120380, 24947
rows = [{k: int(v) for k, v in x.items()} for x in csv.DictReader(open(PATH))]

LEAF = ['FTR', 'STG', 'BG', 'AUD', 'MISC', 'OTHRW', 'SRCRES', 'SWRM', 'GCRARES',
        'SITR', 'SCPU', 'SPHD', 'SPHC', 'SCAT', 'SHDT', 'SPRM']


def leaves(r):
    six = r['SINT'] + r['SPHD'] + r['SPHC'] + r['SCAT'] + r['SHDT'] + r['SPRM']
    return [r['FTR'], r['STG'], r['BG'], r['AUD'], r['MISC'], r['OTHR'] - r['WAIT'],
            r['SRC'] - r['GCRA'] - r['SWRM'], r['SWRM'], r['GCRA'] - six,
            r['SINT'] - r['SCPU'], r['SCPU'], r['SPHD'], r['SPHC'], r['SCAT'],
            r['SHDT'], r['SPRM']]


L = np.array([leaves(r) for r in rows], dtype=np.int64)
W = np.array([r['WORK-H'] for r in rows], dtype=np.int64)
assert (L.sum(1) - W == 0).all(), 'leaf closure failed'


def rank80(w):
    return np.sort(np.asarray(w, np.int64))[::-1][TOPN - 1]


def band(w):
    return np.sort(np.asarray(w, np.int64))[::-1][40:120].mean()


base = rank80(W)
REQ = base - APP - GATE
print('CONTROL  rank-80 %d raw / %d net   level %+d   band 41-120 %.0f'
      % (base, base - APP, REQ, band(W)))
print('         (expect 1,210,624 / 1,185,677 / +65,297 / 1,218,356)')
p50sum = sum(int(np.median(L[:, i])) for i in range(len(LEAF)))
print('\nSum of the sixteen lane medians = %d.  The raw gate is %d, so a frame '
      'that\nspends every lane at its own median passes with %d to spare: '
      'the whole\nremaining requirement is EXCURSION above that baseline, '
      'not baseline.' % (p50sum, GATE + APP, GATE + APP - p50sum))

print('\n=== every lane, three interventions, exact re-rank ===')
print('%-8s | %9s %9s %9s | %8s | %8s %9s | %8s %9s'
      % ('lane', 'P50', 'band41-120', 'x P50', 'f to close', 'EXCmoved',
         'level', 'ALLmoved', 'level'))
out = []
for i, n in enumerate(LEAF):
    p50 = int(np.median(L[:, i]))
    lane = L[:, i]
    exc = np.maximum(lane - p50, 0)
    we, wa = W - exc, W - lane
    # smallest proportional cut of this lane that closes the gate, if any
    fclose = None
    for f in np.arange(0.01, 1.001, 0.01):
        if rank80(W - (lane * f).astype(np.int64)) - APP - GATE <= 0:
            fclose = f
            break
    out.append((base - rank80(we), n, p50, L[np.argsort(-W)[40:120], i].mean(),
                fclose, base - rank80(we), rank80(we), base - rank80(wa),
                rank80(wa)))
for _, n, p50, bn, fc, mv, re_, mva, ra in sorted(out, reverse=True):
    print('%-8s | %9d %9.0f %9.2f | %8s | %8d %+9d | %8d %+9d'
          % (n, p50, bn, (bn / p50) if p50 else 0,
             ('%.0f%%' % (fc * 100)) if fc else 'never',
             mv, re_ - APP - GATE, mva, ra - APP - GATE))

print('\n=== the fighter collision per-joint SETUP, at its own ceiling ===')
print("""SHDT_MECHANISM.md section 2 proved the setup is per ENGAGED frame, so it lives
in the SHDT EXCURSION (bucket minus its own run P50), not in the flat baseline.
Its size, from that document's section 3.1 engaged-57 leaf attribution (self+leaf
tk/fr on the 57 engaged frames of build-c191-sitr-profile-c185):
    func_ovl2_800ED490      world matrix compose   7,006 + 20,696 = 27,702
    gmCollisionSetInvertMatrix  3x3 cofactor       5,301 +  9,329 = 14,630
    func_ovl2_800EDE5C      axis scales, 3 sqrtf   1,509 +  9,686 = 11,195
    gmCollisionTransformMatrixAll  local matrix    5,060 +  5,791 = 10,851
    lbCommonSin + lbCommonCos   its trig           3,805 +  2,477 =  6,282
                                                  SETUP TOTAL      70,660
against the whole chain's 102,988 (68.6% of it).  NOT setup, and excluded: the
CONSUMERS -- gmCollisionGetWorldPosition 14,737 and gmCollisionTestRectangle
11,880, which section 2's natural experiment already showed are the flat half.""")
sh = L[:, LEAF.index('SHDT')]
exc = np.maximum(sh - int(np.median(sh)), 0)
print('\n  SHDT excursion, whole match: %d tk' % exc.sum())
for label, tkfr in (('setup incl. trig', 70660), ('setup excl. trig', 64378),
                    ('whole chain', 102988)):
    f = tkfr * 57 / exc.sum()
    r = rank80(W - (exc * f).astype(np.int64))
    print('  %-18s %6d tk/fr x 57 frames = %8d tk = f %.3f of the excursion'
          % (label, tkfr, tkfr * 57, f))
    print('  %-18s rank-80 %d  moved %6d  level %+d  band %.0f'
          % ('', r, base - r, r - APP - GATE, band(W - (exc * f).astype(np.int64))))
print('\n  sensitivity -- the answer is flat across the whole plausible range:')
for f in (0.15, 0.20, 0.224, 0.245, 0.30, 0.35, 0.50):
    r = rank80(W - (exc * f).astype(np.int64))
    print('    f=%.3f  rank-80 %d  moved %6d  level %+d' % (f, r, base - r,
                                                            r - APP - GATE))

print('\n=== reference: what a UNIFORM saving on EVERY frame is worth ===')
for d in (3542, 10112, 14080, 21267, REQ):
    r = rank80(W - d)
    print('  D=%-6d moved %6d  ratio %.3f  level %+d'
          % (d, base - r, (base - r) / d, r - APP - GATE))
