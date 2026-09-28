#!/usr/bin/env python3
"""lane3_lean_coverage.py -- lean-path coverage over recorded four-CPU runs.

Usage: lane3_lean_coverage.py <artifacts/performance> <since-date>
Per run: rows 0..3 = battle slots; k_owner[row] = owner slot + 1 (owner slot = fighter kind
0..11 for VS), k_attempts[row], k_draws[row], k_decline[row][0..19].
Prints per owner kind: #runs, attempts, draws, declines (by reason), and per gkind (stage) the
number of runs, so a reader sees which kind x stage cells the recorded evidence covers.
"""
import collections
import json
import os
import re
import sys

KIND_NAMES = ['Mario', 'Fox', 'Luigi', 'Donkey', 'Captain', 'Samus', 'Link', 'Pikachu', 'Yoshi', 'Ness', 'Purin', 'Kirby']
STAGE_NAMES = {0: 'Castle', 1: 'Sector', 2: 'Jungle', 3: 'Zebes', 4: 'Hyrule', 5: 'Yoster', 6: 'Pupupu', 7: 'Yamabuki', 8: 'Inishie'}
DECL = ['Kind', 'Skeleton', 'Camera', 'Plan', 'Validate', 'KirbyHead', 'Roots', 'Material', 'Tables', 'Policy', 'Texture',
        'Capacity', 'Topology', 'Kernel', 'Texgen', 'Tint', 'Stale', 'Inputs', 'AlphaTest', 'R19']


def main():
    root, since = sys.argv[1], sys.argv[2]
    kind_runs = collections.Counter()
    kind_attempts = collections.Counter()
    kind_draws = collections.Counter()
    kind_decl = collections.defaultdict(collections.Counter)
    cell = collections.Counter()
    stage_runs = collections.Counter()
    nruns = 0
    for top in sorted(os.listdir(root)):
        if top[:10] < since or not os.path.isdir(os.path.join(root, top)):
            continue
        for dp, dn, fn in os.walk(os.path.join(root, top)):
            for f in fn:
                if not f.endswith('.json'):
                    continue
                p = os.path.join(dp, f)
                try:
                    d = json.load(open(p, encoding='utf-8', errors='replace'))
                except Exception:
                    continue
                if not isinstance(d, dict) or not isinstance(d.get('extras'), list):
                    continue
                ev = {e['name']: e['value'] for e in d['extras'] if isinstance(e, dict) and 'name' in e}
                if 'gNdsFtrLean.attempts' not in ev:
                    continue
                if ev.get('gNdsFtrLean.attempts', 0) < 500:
                    continue           # a stub / early-stop run
                bg = {g.get('name'): g.get('readback') for g in (d.get('bootSetGlobals') or []) if isinstance(g, dict)}
                gk = bg.get('gNdsLabFourCpuGkind')
                nruns += 1
                stage_runs[gk] += 1
                for row in range(4):
                    own = ev.get('gNdsFtrLean.k_owner[%d]' % row)
                    if not own:
                        continue
                    kind = own - 1
                    att = ev.get('gNdsFtrLean.k_attempts[%d]' % row, 0)
                    drw = ev.get('gNdsFtrLean.k_draws[%d]' % row, 0)
                    kind_runs[kind] += 1
                    kind_attempts[kind] += att
                    kind_draws[kind] += drw
                    cell[(kind, gk)] += 1
                    for r in range(20):
                        v = ev.get('gNdsFtrLean.k_decline[%d][%d]' % (row, r), 0)
                        if v:
                            kind_decl[kind][DECL[r]] += v
    print('runs with >=500 lean attempts:', nruns)
    print('runs per stage (gkind; None = default stage of the ROM):', {(STAGE_NAMES.get(k, k)): v for k, v in sorted(stage_runs.items(), key=lambda kv: str(kv[0]))})
    print('%-9s %6s %10s %10s  declines' % ('kind', 'runs', 'attempts', 'draws'))
    for k in sorted(kind_runs):
        nm = KIND_NAMES[k] if k < len(KIND_NAMES) else str(k)
        print('%-9s %6d %10d %10d  %s' % (nm, kind_runs[k], kind_attempts[k], kind_draws[k], dict(kind_decl[k])))
    missing = [KIND_NAMES[k] for k in range(12) if k not in kind_runs]
    print('VS kinds never recorded:', missing)
    print('kind x stage cells (runs):')
    stages = sorted({c[1] for c in cell}, key=lambda x: str(x))
    print('%-9s' % '', ' '.join('%8s' % (STAGE_NAMES.get(s, str(s))[:8]) for s in stages))
    for k in sorted(kind_runs):
        print('%-9s' % KIND_NAMES[k], ' '.join('%8d' % cell.get((k, s), 0) for s in stages))


if __name__ == '__main__':
    main()
