"""Slice 6: the README's per-roster gate tables, as markdown, from the run files.

Usage: tables6.py PREFIX [PREFIX...]   (e.g. c6 c6-mfly c6-cppn c6-hi-dklk)
Reads PREFIX-route{0,1,2}.json / -rows.csv (run-s6.ps1 output) and prints one
gate table per roster, then one per-row coverage table (route 1 lean share,
named declines, programs, high draws; route 2 oracle runs and mismatches).
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
SEQ = os.path.join(HERE, 'tools', 'digestseq6.py')
OWNERS = ['Mario', 'Fox', 'Luigi', 'Donkey', 'Captain', 'Samus', 'Link',
          'Pikachu', 'Yoshi', 'Ness', 'Purin', 'Kirby']
DECLINE = ['kind', 'skeleton', 'camera', 'plan', 'validate', 'kirby_head',
           'roots', 'material', 'tables', 'policy', 'texture', 'capacity',
           'topology', 'kernel', 'texgen', 'tint', 'stale', 'inputs',
           'alpha_test', '19']


def load(arm):
    pj = os.path.join(HERE, arm + '.json')
    pr = os.path.join(HERE, arm + '-rows.csv')
    if not (os.path.exists(pj) and os.path.exists(pr)):
        return None
    d = json.load(open(pj))
    ex = {e['name']: e['value'] for e in d['extras']}
    rows = list(csv.DictReader(open(pr)))
    return d, ex, rows


def pct(v, q):
    s = sorted(v)
    k = (len(s) - 1) * q
    f = int(k)
    c = min(f + 1, len(s) - 1)
    return s[f] + (s[c] - s[f]) * (k - f)


def g(ex, n):
    return (ex or {}).get('gNdsFtrLean.' + n, 0) or 0


def last_line(cmd):
    out = subprocess.run(cmd, capture_output=True, text=True).stdout.strip().splitlines()
    return out[-1] if out else 'n/a'


def f(x):
    return '{:,.0f}'.format(x)


def main():
    for pre in sys.argv[1:]:
        arms = {r: '%s-route%d' % (pre, r) for r in (0, 1, 2)}
        runs = {r: load(a) for r, a in arms.items()}
        if runs[0] is None or runs[1] is None:
            print('### %s: route 0/1 runs missing\n' % pre)
            continue
        (d0, ex0, r0), (d1, ex1, r1) = runs[0], runs[1]
        ex2 = runs[2][1] if runs[2] else None
        p = {r: os.path.join(HERE, arms[r] + '-rows.csv') for r in (0, 1, 2)}
        dk1 = last_line([sys.executable, DIGEST, p[0], p[1]])
        ds1 = last_line([sys.executable, SEQ, p[0], p[1]])
        dk2 = last_line([sys.executable, DIGEST, p[0], p[2]]) if runs[2] else 'n/a'
        ds2 = last_line([sys.executable, SEQ, p[0], p[2]]) if runs[2] else 'n/a'
        print('### %s (ROM sha256 %s)\n' % (pre, (d1.get('romSha256') or '')[:16]))
        print('| gate | result |')
        print('|---|---|')
        print('| digest route 1 vs 0 | %s; by sequence: %s |' % (dk1, ds1))
        print('| digest route 2 vs 0 | %s; by sequence: %s |' % (dk2, ds2))
        print('| native failures r0 / r1 / r2 | %s / %s / %s |' % (
            ex0.get('gNdsRendererNativeFailure.count'), ex1.get('gNdsRendererNativeFailure.count'),
            (ex2 or {}).get('gNdsRendererNativeFailure.count')))
        mm = [g(ex2, 'oracle_mismatch[%d]' % i) for i in range(8)] if ex2 else None
        print('| route 2 oracle | %s runs, %s words, mismatches %s |' % (
            f(g(ex2, 'oracle_runs')), f(g(ex2, 'oracle_words')), mm))
        print('| fighter uploads after GO r0 / r1 | %s / %s |' % (
            g(ex0, 'fighter_uploads_after_go'), g(ex1, 'fighter_uploads_after_go')))
        print('| heap low-water r0 / r1 (general, B) | %s / %s |' % (
            f(ex0.get('gNdsTaskmanGeneralHeapFreeMin') or 0), f(ex1.get('gNdsTaskmanGeneralHeapFreeMin') or 0)))
        print('| libc top chunk min r0 / r1 (B) | %s / %s |' % (
            f(ex0.get('gNdsTaskmanLibcTopChunkMin') or 0), f(ex1.get('gNdsTaskmanLibcTopChunkMin') or 0)))
        print('| Fox blaster draws r0 / r1 | %s / %s |' % (
            ex0.get('gNdsRendererFoxGunDrawCount'), ex1.get('gNdsRendererFoxGunDrawCount')))
        print('| VBlanks 2/3/4/5+ r0 | %s |' % ' / '.join(str(d0.get(k)) for k in ('vbi2', 'vbi3', 'vbi4', 'vbi5plus')))
        print('| VBlanks 2/3/4/5+ r1 | %s |' % ' / '.join(str(d1.get(k)) for k in ('vbi2', 'vbi3', 'vbi4', 'vbi5plus')))
        print()
        print('| counter | r0 P50 | r0 P95 | r0 P99 | r1 P50 | r1 P95 | r1 P99 | mean r1-r0 |')
        print('|---|---:|---:|---:|---:|---:|---:|---:|')
        for c in ('FTR', 'WORK-H', 'STG'):
            v0 = [int(x[c]) for x in r0]
            v1 = [int(x[c]) for x in r1]
            print('| %s | %s | %s | %s | %s | %s | %s | %+.0f |' % (
                c, f(pct(v0, .5)), f(pct(v0, .95)), f(pct(v0, .99)),
                f(pct(v1, .5)), f(pct(v1, .95)), f(pct(v1, .99)),
                statistics.mean(v1) - statistics.mean(v0)))
        print()
        print('| row | kind | route 1 lean draws / attempts | share | declines | programs | high draws | words max | r2 oracle runs | r2 mismatches |')
        print('|---|---|---:|---:|---|---|---:|---:|---:|---|')
        for k in range(4):
            own = g(ex1, 'k_owner[%d]' % k)
            if not own:
                continue
            name = OWNERS[own - 1] if 1 <= own <= 12 else '?'
            att = g(ex1, 'k_attempts[%d]' % k)
            dr = g(ex1, 'k_draws[%d]' % k)
            dec = {DECLINE[i]: g(ex1, 'k_decline[%d][%d]' % (k, i)) for i in range(20)
                   if g(ex1, 'k_decline[%d][%d]' % (k, i))}
            prog = {i: g(ex1, 'k_program_draws[%d][%d]' % (k, i)) for i in range(16)
                    if g(ex1, 'k_program_draws[%d][%d]' % (k, i))}
            orr = g(ex2, 'k_oracle_runs[%d]' % k) if ex2 else 'n/a'
            omm = sum(g(ex2, 'k_oracle_mismatch[%d][%d]' % (k, i)) for i in range(8)) if ex2 else 'n/a'
            print('| %d | %s | %s / %s | %.2f%% | %s | %s | %s | %s | %s | %s |' % (
                k, name, f(dr), f(att), 100.0 * dr / att if att else 0.0,
                ', '.join('%s %s' % kv for kv in dec.items()) or 'none',
                ', '.join('%s: %s' % kv for kv in prog.items()) or '-',
                f(g(ex1, 'k_high_draws[%d]' % k)), f(g(ex1, 'k_words_max[%d]' % k)), orr, omm))
        print()


if __name__ == '__main__':
    main()
