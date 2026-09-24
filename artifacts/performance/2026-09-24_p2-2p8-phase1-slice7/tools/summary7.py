"""Slice 7 per-roster summary: the ROM as it ships (no pokes) against the old
path on the same ROM (route 0 + admission 0 poked at boot), and the route 2
oracle arm (route 2 poked at boot, admission at its default 2).

Usage: summary7.py PREFIX [PREFIX...]
Arms: PREFIX-default, PREFIX-old, PREFIX-oracle (any may be missing).
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


def run_tool(tool, a, b, *args):
    pa = os.path.join(HERE, a + '-rows.csv')
    pb = os.path.join(HERE, b + '-rows.csv')
    if not (os.path.exists(pa) and os.path.exists(pb)):
        return 'n/a'
    out = subprocess.run([sys.executable, tool, pa, pb, *args], capture_output=True, text=True)
    return (out.stdout.strip().splitlines() or [''])[-1]


def main():
    for pre in sys.argv[1:]:
        arms = {k: '%s-%s' % (pre, k) for k in ('default', 'old', 'oracle')}
        dd, exd = extras(arms['default'])
        do, exo = extras(arms['old'])
        dr, exr = extras(arms['oracle'])
        print('=' * 100)
        print('ROSTER %s   ROM %s' % (pre, ((dd or do or dr or {}).get('romSha256') or '')[:16]))
        for label, d, ex in (('default', dd, exd), ('old', do, exo), ('oracle', dr, exr)):
            if d is None:
                continue
            print('  %-7s boot pokes %s; words at end route=%s admit=%s; admission runs=%s word=%s frame=%s '
                  'entries=%s bytes=%s fails=%s outside=%s locked=%s regions=%s bg3_not_empty=%s' % (
                      label, [(p['name'], p['readback']) for p in (d.get('bootSetGlobals') or [])],
                      ex.get('gNdsFtrLeanRoute'), ex.get('gNdsFtrLeanAdmit'),
                      ex.get('gNdsFtrAdmitLab.runs'), ex.get('gNdsFtrAdmitLab.word'),
                      ex.get('gNdsFtrAdmitLab.frame'), ex.get('gNdsFtrAdmitLab.entries_admitted'),
                      ex.get('gNdsFtrAdmitLab.bytes_admitted'), ex.get('gNdsFtrAdmitLab.fails'),
                      ex.get('gNdsFtrAdmitLab.outside_count'), ex.get('gNdsFtrAdmitLab.locked'),
                      ex.get('gNdsFtrAdmitLab.regions'), ex.get('gNdsFtrAdmitLab.bg3_not_empty')))
        print('  digest default vs old:', run_tool(DIGEST, arms['old'], arms['default']))
        print('  digest oracle  vs old:', run_tool(DIGEST, arms['old'], arms['oracle']))
        print('  digest by sequence default / oracle vs old: %s / %s' % (
            run_tool(DIGEST, arms['old'], arms['default'], '--sequence'),
            run_tool(DIGEST, arms['old'], arms['oracle'], '--sequence')))
        print('  native failures old / default / oracle: %s / %s / %s' % tuple(
            (e or {}).get('gNdsRendererNativeFailure.count') for e in (exo, exd, exr)))
        print('  native direct rejects old / default / oracle: %s / %s / %s' % tuple(
            (e or {}).get('gNdsRendererNativeDirectReject.count') for e in (exo, exd, exr)))
        print('  fighter uploads after GO old / default / oracle: %s / %s / %s' % tuple(
            g(e, 'fighter_uploads_after_go') for e in (exo, exd, exr)))
        print('  heap general low-water old / default / oracle: %s / %s / %s; libc top min %s / %s / %s' % (
            tuple((e or {}).get('gNdsTaskmanGeneralHeapFreeMin') for e in (exo, exd, exr)) +
            tuple((e or {}).get('gNdsTaskmanLibcTopChunkMin') for e in (exo, exd, exr))))
        print('  packets old / default: records %s / %s hits %s / %s faults %s / %s declines %s / %s' % (
            (exo or {}).get('gNdsFighterPacketRecords'), (exd or {}).get('gNdsFighterPacketRecords'),
            (exo or {}).get('gNdsFighterPacketHits'), (exd or {}).get('gNdsFighterPacketHits'),
            (exo or {}).get('gNdsFighterPacketFaults'), (exd or {}).get('gNdsFighterPacketFaults'),
            (exo or {}).get('gNdsFighterPacketDeclines'), (exd or {}).get('gNdsFighterPacketDeclines')))
        print('  Fox blaster draws old / default: %s / %s' % tuple(
            (e or {}).get('gNdsRendererFoxGunDrawCount') for e in (exo, exd)))
        r0, r1 = rows(arms['old']), rows(arms['default'])
        if r0 and r1:
            print('  %-8s %38s | %38s' % ('', 'old: P50 / P95 / P99 (mean)', 'default: P50 / P95 / P99 (mean)'))
            for c in ('FTR', 'WORK-H', 'STG'):
                cells = []
                for rr in (r0, r1):
                    v = [int(x[c]) for x in rr]
                    cells.append('%9s %9s %9s (%s)' % ('{:,.0f}'.format(pct(v, .5)), '{:,.0f}'.format(pct(v, .95)),
                                                       '{:,.0f}'.format(pct(v, .99)), '{:,.0f}'.format(statistics.mean(v))))
                print('  %-8s %s | %s' % (c, cells[0], cells[1]))
            print('  VBlanks old 2/3/4/5+: %s   default: %s' % tuple(
                ' '.join(str((d or {}).get(k)) for k in ('vbi2', 'vbi3', 'vbi4', 'vbi5plus')) for d in (do, dd)))
        print('  default: lean share per row (kind): draws / attempts, declines, programs, high draws, words max')
        for k in range(4):
            own = g(exd, 'k_owner[%d]' % k)
            if not own:
                continue
            name = OWNERS[own - 1] if 1 <= own <= 12 else '?'
            att = g(exd, 'k_attempts[%d]' % k)
            drw = g(exd, 'k_draws[%d]' % k)
            dec = {DECLINE[i]: g(exd, 'k_decline[%d][%d]' % (k, i)) for i in range(20)
                   if g(exd, 'k_decline[%d][%d]' % (k, i))}
            prog = {i: g(exd, 'k_program_draws[%d][%d]' % (k, i)) for i in range(16)
                    if g(exd, 'k_program_draws[%d][%d]' % (k, i))}
            print('    %-8s %5d / %5d = %6.2f%%  declines %s  programs %s  high %d  words max %d  mats %d' % (
                name, drw, att, 100.0 * drw / att if att else 0, dec or '{}', prog,
                g(exd, 'k_high_draws[%d]' % k), g(exd, 'k_words_max[%d]' % k), g(exd, 'k_materializations[%d]' % k)))
        if exr:
            print('  oracle (route 2): runs %d words %d mismatch %s clip max %s donor_memo %d rec_under_hit %s' % (
                g(exr, 'oracle_runs'), g(exr, 'oracle_words'),
                [g(exr, 'oracle_mismatch[%d]' % i) for i in range(8)],
                [g(exr, 'oracle_clip_max[%d]' % i) for i in range(2)], g(exr, 'oracle_donor_memo'),
                [g(exr, 'oracle_record_under_hit[%d]' % i) for i in range(2)]))
            for k in range(4):
                own = g(exr, 'k_owner[%d]' % k)
                if not own:
                    continue
                name = OWNERS[own - 1] if 1 <= own <= 12 else '?'
                mm = [g(exr, 'k_oracle_mismatch[%d][%d]' % (k, i)) for i in range(8)]
                prog = {i: g(exr, 'k_program_draws[%d][%d]' % (k, i)) for i in range(16)
                        if g(exr, 'k_program_draws[%d][%d]' % (k, i))}
                print('    %-8s runs %5d mismatch %s' % (name, g(exr, 'k_oracle_runs[%d]' % k), mm))


if __name__ == '__main__':
    main()
