"""Slice 6: replay-digest comparison by SEQUENCE, for runs whose frame keys slip.

compare-replay-digest.py pairs rows by the sampler's presented-frame key. When a
run's ring drain loses or repeats a row (heavy frames), every later key is off by
one and the key compare reports a divergence the game never had. This compares
the two runs' (DGSA, DGSB) sequences in frame order instead: it walks both,
pairing equal digest pairs, and reports every row one side has that the other
lacks (a sampling gap), and every place where both sides have a row but the
digests differ (a real divergence). Identical gameplay = zero real divergences.

Usage: digestseq6.py CONTROL-rows.csv CANDIDATE-rows.csv
"""
import csv
import sys


def seq(path):
    rows = sorted(csv.DictReader(open(path)), key=lambda r: int(r['frame']))
    return [(int(r['frame']), r['DGSA'], r['DGSB']) for r in rows]


def main():
    a, b = seq(sys.argv[1]), seq(sys.argv[2])
    i = j = 0
    gaps_a, gaps_b, real = [], [], []
    while i < len(a) and j < len(b):
        if a[i][1:] == b[j][1:]:
            i += 1
            j += 1
            continue
        # one side has a row the other lacks: look a few rows ahead on each side
        ahead_b = next((k for k in range(j + 1, min(j + 4, len(b))) if b[k][1:] == a[i][1:]), None)
        ahead_a = next((k for k in range(i + 1, min(i + 4, len(a))) if a[k][1:] == b[j][1:]), None)
        if ahead_b is not None and (ahead_a is None or ahead_b - j <= ahead_a - i):
            gaps_a.extend(b[j:ahead_b])
            j = ahead_b
        elif ahead_a is not None:
            gaps_b.extend(a[i:ahead_a])
            i = ahead_a
        else:
            real.append((a[i], b[j]))
            i += 1
            j += 1
    print('rows: control %d, candidate %d, paired %d' % (len(a), len(b), len(a) - len(gaps_b) - len(real)))
    print('rows only the candidate sampled (control ring gaps): %d %s' % (len(gaps_a), [x[0] for x in gaps_a][:10]))
    print('rows only the control sampled (candidate ring gaps): %d %s' % (len(gaps_b), [x[0] for x in gaps_b][:10]))
    print('REAL divergences (both sampled, digests differ): %d %s' % (len(real), real[:3]))
    print('SEQUENCE IDENTICAL' if not real else 'SEQUENCE DIVERGED')
    return 1 if real else 0


if __name__ == '__main__':
    sys.exit(main())
