#!/usr/bin/env python3
"""Paired / level analysis for tick-HUD rows CSVs.

  analyze.py <a-rows.csv> <b-rows.csv> [series]

Reports the level statistics for each arm (rank-80 raw and net, the 41-120
band, P50) and, when the two runs cover the same frame ids, the PAIRED
per-frame delta -- the only estimator that is immune to the tail permuting
between arms.

APPARATUS 24,947.  GATE 1,120,380.  Series defaults to WORK-H.

Rows CSVs written before 2026-08-16 have no WRAPFIX column and are NOT
wrap-corrected; --fixwrap applies the sampler's own correction offline so an
old basis can be compared with a new run.  Correcting only one side moves
rank-80 by up to 6,592.
"""
import csv
import statistics
import sys

APPARATUS = 24947
GATE = 1120380
WRAP = 4194304


def load(path, fixwrap):
    out = {}
    with open(path, newline='') as fh:
        rd = csv.DictReader(fh)
        cols = [c for c in rd.fieldnames if c not in ('frame', 'WRAPFIX')]
        for row in rd:
            vals = {c: int(row[c]) for c in cols}
            if fixwrap and vals['ALL'] >= WRAP:
                for c in cols:
                    if vals[c] >= WRAP:
                        vals[c] -= WRAP
            out[int(row['frame'])] = vals
    return out


def rank(vals, n):
    return sorted(vals, reverse=True)[n - 1]


def band(vals, lo, hi):
    s = sorted(vals, reverse=True)
    return sum(s[lo - 1:hi]) / float(hi - lo + 1)


def level(tag, vals):
    r80 = rank(vals, 80)
    print('%-26s rank-80 raw %9d  net %9d  LEVEL %+8d  band41-120 %9.0f  P50 %9d'
          % (tag, r80, r80 - APPARATUS, (r80 - APPARATUS) - GATE,
             band(vals, 41, 120), statistics.median(vals)))
    return r80


def main():
    ap, bp = sys.argv[1], sys.argv[2]
    series = sys.argv[3] if len(sys.argv) > 3 and not sys.argv[3].startswith('-') else 'WORK-H'
    fa = '--fixwrap-a' in sys.argv
    fb = '--fixwrap-b' in sys.argv
    a, b = load(ap, fa), load(bp, fb)
    av = [r[series] for r in a.values()]
    bv = [r[series] for r in b.values()]
    print('series %s   A=%s (%d rows, fixwrap=%s)   B=%s (%d rows, fixwrap=%s)'
          % (series, ap.split('\\')[-1].split('/')[-1], len(a), fa,
             bp.split('\\')[-1].split('/')[-1], len(b), fb))
    level('A', av)
    level('B', bv)

    common = sorted(set(a) & set(b))
    if len(common) < 100:
        print('no paired population (%d common frames)' % len(common))
        return 0
    d = [b[f][series] - a[f][series] for f in common]
    print('\nPAIRED over %d common frames' % len(common))
    print('  median %+8d   mean %+9.0f   improved %d/%d (%.1f%%)'
          % (statistics.median(d), statistics.fmean(d),
             sum(1 for x in d if x < 0), len(d),
             100.0 * sum(1 for x in d if x < 0) / len(d)))
    ds = sorted(d)
    tr = ds[8:-8]
    print('  trimmed(8/8) mean %+9.0f   min %+d   max %+d'
          % (statistics.fmean(tr), ds[0], ds[-1]))

    # marginal-80: A's own top-80 frames, paired there
    top = sorted(common, key=lambda f: -a[f][series])[:80]
    dm = [b[f][series] - a[f][series] for f in top]
    print('  marginal-80 paired median %+8d  (%d/80 improve)'
          % (statistics.median(dm), sum(1 for x in dm if x < 0)))

    print('\n  rank curve (A rank -> delta at that rank position)')
    asrt, bsrt = sorted(av, reverse=True), sorted(bv, reverse=True)
    for r in (1, 5, 10, 20, 40, 80, 120, 160, 320, 640, 800, 1200, 1600):
        if r <= len(asrt):
            print('    rank %5d  A %9d  B %9d  %+8d'
                  % (r, asrt[r - 1], bsrt[r - 1], bsrt[r - 1] - asrt[r - 1]))
    return 0


if __name__ == '__main__':
    sys.exit(main())
