"""Slice 6 per-roster gate summary (route 0 / 1 / 2 runs of one ROM each).

Usage: summary6.py PREFIX [PREFIX...]
PREFIX is the arm stem, e.g. c5 (default roster: c5-route0/1/2), c5-mfly,
c5-hi-dklk. Prints, per roster: ROM, digest verdicts (route 1 and 2 against
route 0), native failures per route, FTR / WORK-H / STG P50 P95 P99 and mean
for routes 0 and 1, the per-row lean share and named declines, the route 2
oracle per row, heap figures, and the per-kind counts the slice cares about
(program draws incl. skeleton, high draws, Fox blaster draws, uploads after GO).
"""
import csv
import json
import os
import statistics
import subprocess
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
DIGEST = os.path.join(ROOT, 'scripts', 'compare-replay-digest.py')
OWNERS = ['Mario', 'Fox', 'Luigi', 'Donkey', 'Captain', 'Samus', 'Link',
          'Pikachu', 'Yoshi', 'Ness', 'Purin', 'Kirby']
DECLINE = ['kind', 'skeleton', 'camera', 'plan', 'validate', 'kirby_head',
           'roots', 'material', 'tables', 'policy', 'texture', 'capacity',
           'topology', 'kernel', 'texgen', 'tint', 'stale', 'inputs', 'alpha_test', '19']
ORACLE = ['structure', 'projection', 'modelview', 'shade', 'light', 'other', 'tint', 'texgen']


def rows(arm):
    p = os.path.join(HERE, arm + '-rows.csv')
    return list(csv.DictReader(open(p))) if os.path.exists(p) else None


def extras(arm):
    p = os.path.join(HERE, arm + '.json')
    if not os.path.exists(p):
        return None, None
    d = json.load(open(p))
    return d, {e['name']: e['value'] for e in d['extras']}


def pct(v, q):
    s = sorted(v)
    k = (len(s) - 1) * q
    f = int(k)
    c = min(f + 1, len(s) - 1)
    return s[f] + (s[c] - s[f]) * (k - f)


def g(ex, n):
    return (ex or {}).get('gNdsFtrLean.' + n, 0) or 0


def digest(a, b):
    pa = os.path.join(HERE, a + '-rows.csv')
    pb = os.path.join(HERE, b + '-rows.csv')
    if not (os.path.exists(pa) and os.path.exists(pb)):
        return 'n/a'
    out = subprocess.run([sys.executable, DIGEST, pa, pb], capture_output=True, text=True)
    return (out.stdout.strip().splitlines() or [''])[-1]


def seqdigest(a, b):
    pa = os.path.join(HERE, a + '-rows.csv')
    pb = os.path.join(HERE, b + '-rows.csv')
    if not (os.path.exists(pa) and os.path.exists(pb)):
        return 'n/a'
    out = subprocess.run([sys.executable, os.path.join(HERE, 'tools', 'digestseq6.py'), pa, pb],
                         capture_output=True, text=True)
    lines = out.stdout.strip().splitlines()
    return lines[-1] if lines else 'n/a'


def main():
    for pre in sys.argv[1:]:
        arms = {r: '%s-route%d' % (pre, r) for r in (0, 1, 2)}
        d1, ex1 = extras(arms[1])
        d0, ex0 = extras(arms[0])
        d2, ex2 = extras(arms[2])
        print('=' * 100)
        print('ROSTER %s   ROM %s' % (pre, ((d1 or d0 or {}).get('romSha256') or '')[:16]))
        print('  digest route1 vs route0:', digest(arms[0], arms[1]))
        print('  digest route2 vs route0:', digest(arms[0], arms[2]))
        print('  digest by sequence r1 / r2 vs r0: %s / %s' % (seqdigest(arms[0], arms[1]), seqdigest(arms[0], arms[2])))
        print('  native failures r0 / r1 / r2: %s / %s / %s' % tuple(
            (e or {}).get('gNdsRendererNativeFailure.count') for e in (ex0, ex1, ex2)))
        print('  fighter uploads after GO r0 / r1 / r2: %s / %s / %s' % tuple(
            g(e, 'fighter_uploads_after_go') for e in (ex0, ex1, ex2)))
        print('  Fox blaster draws r0 / r1: %s / %s' % tuple(
            (e or {}).get('gNdsRendererFoxGunDrawCount') for e in (ex0, ex1)))
        r0, r1 = rows(arms[0]), rows(arms[1])
        if r0 and r1:
            print('  %-8s %36s | %36s' % ('', 'route 0: P50 / P95 / P99 (mean)', 'route 1: P50 / P95 / P99 (mean)'))
            for c in ('FTR', 'WORK-H', 'STG'):
                cells = []
                for rr in (r0, r1):
                    v = [int(x[c]) for x in rr]
                    cells.append('%9s %9s %9s (%s)' % ('{:,.0f}'.format(pct(v, .5)), '{:,.0f}'.format(pct(v, .95)),
                                                       '{:,.0f}'.format(pct(v, .99)), '{:,.0f}'.format(statistics.mean(v))))
                print('  %-8s %s | %s' % (c, cells[0], cells[1]))
            f0 = statistics.mean(int(x['FTR']) for x in r0)
            f1 = statistics.mean(int(x['FTR']) for x in r1)
            w0 = statistics.mean(int(x['WORK-H']) - int(x['STG']) for x in r0)
            w1 = statistics.mean(int(x['WORK-H']) - int(x['STG']) for x in r1)
            wh0 = statistics.mean(int(x['WORK-H']) for x in r0)
            wh1 = statistics.mean(int(x['WORK-H']) for x in r1)
            print('  stop rule: FTR mean %+.0f, WORK-H mean %+.0f, WORK-H less STG mean %+.0f' % (
                f1 - f0, wh1 - wh0, w1 - w0))
            for k in ('vbi2', 'vbi3', 'vbi4', 'vbi5plus'):
                pass
            print('  VBlanks r0 2/3/4/5+: %s   r1: %s' % tuple(
                ' '.join(str((d or {}).get(k)) for k in ('vbi2', 'vbi3', 'vbi4', 'vbi5plus')) for d in (d0, d1)))
        print('  heap (r1): general low-water %s, libc top min %s, arena %s' % (
            (ex1 or {}).get('gNdsTaskmanGeneralHeapFreeMin'), (ex1 or {}).get('gNdsTaskmanLibcTopChunkMin'),
            (ex1 or {}).get('gNdsTaskmanArenaChosenSize')))
        print('  route 1 lean share per row (kind): draws / attempts, declines, programs, high draws, words max')
        for k in range(4):
            own = g(ex1, 'k_owner[%d]' % k)
            if not own:
                continue
            name = OWNERS[own - 1] if 1 <= own <= 12 else '?'
            att = g(ex1, 'k_attempts[%d]' % k)
            dr = g(ex1, 'k_draws[%d]' % k)
            dec = {DECLINE[i]: g(ex1, 'k_decline[%d][%d]' % (k, i)) for i in range(20) if g(ex1, 'k_decline[%d][%d]' % (k, i))}
            prog = {i: g(ex1, 'k_program_draws[%d][%d]' % (k, i)) for i in range(16) if g(ex1, 'k_program_draws[%d][%d]' % (k, i))}
            print('    %-8s %5d / %5d = %6.2f%%  declines %s  programs %s  high %d  words max %d  mats %d' % (
                name, dr, att, 100.0 * dr / att if att else 0, dec or '{}', prog, g(ex1, 'k_high_draws[%d]' % k),
                g(ex1, 'k_words_max[%d]' % k), g(ex1, 'k_materializations[%d]' % k)))
        if ex2:
            print('  route 2 oracle: runs %d words %d mismatch %s clip max %s donor_memo %d rec_under_hit %s record_diff %s' % (
                g(ex2, 'oracle_runs'), g(ex2, 'oracle_words'),
                [g(ex2, 'oracle_mismatch[%d]' % i) for i in range(8)],
                [g(ex2, 'oracle_clip_max[%d]' % i) for i in range(2)], g(ex2, 'oracle_donor_memo'),
                [g(ex2, 'oracle_record_under_hit[%d]' % i) for i in range(2)],
                [g(ex2, 'oracle_record_diff[%d]' % i) for i in range(8)]))
            for k in range(4):
                own = g(ex2, 'k_owner[%d]' % k)
                if not own:
                    continue
                name = OWNERS[own - 1] if 1 <= own <= 12 else '?'
                mm = [g(ex2, 'k_oracle_mismatch[%d][%d]' % (k, i)) for i in range(8)]
                print('    %-8s runs %5d mismatch %s' % (name, g(ex2, 'k_oracle_runs[%d]' % k), mm))
            print('  mirrors (route 2): fence %d tint %d; old-path tinted fail-closed per slot %s' % (
                g(ex2, 'fence_rerecords_mirrored'), g(ex2, 'tint_rerecords_mirrored'),
                [g(ex2, 'tint_rerecords[%d]' % i) for i in range(4)]))


if __name__ == '__main__':
    main()
