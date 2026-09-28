#!/usr/bin/env python3
"""lane3_native_failures.py -- summarise recorded runs' native-failure / decline counters.

Usage: lane3_native_failures.py <artifacts/performance> <since-date> [counter-name ...]
Default counters: gNdsRendererNativeFailure.count, gNdsFighterPacketRecords,
gNdsRendererNativeDirectReject.count, gNdsFtrLean.decline[14].
For each counter prints the count of runs, nonzero runs, the nonzero folders, and the
top 8 nonzero runs (value, file, git, gkind, kinds).
"""
import collections
import json
import os
import sys


def main():
    root, since = sys.argv[1], sys.argv[2]
    names = sys.argv[3:] or ['gNdsRendererNativeFailure.count', 'gNdsFighterPacketRecords',
                             'gNdsRendererNativeDirectReject.count', 'gNdsFtrLean.decline[14]']
    runs = []
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
                bg = {g.get('name'): g.get('readback') for g in (d.get('bootSetGlobals') or []) if isinstance(g, dict)}
                runs.append((os.path.relpath(p, root), ev, bg, d.get('gitShort')))
    print('runs', len(runs))
    for n in names:
        nz = [(r[1].get(n, 0), r[0], r[3], r[2].get('gNdsLabFourCpuGkind'), r[2].get('gNdsLabFourCpuKinds')) for r in runs if r[1].get(n, 0)]
        nz.sort(reverse=True)
        print('== %s : runs with counter %d, nonzero %d' % (n, sum(1 for r in runs if n in r[1]), len(nz)))
        by = collections.Counter(x[1].split(os.sep)[0] for x in nz)
        print('   folders:', dict(by))
        for x in nz[:8]:
            print('   ', x)


if __name__ == '__main__':
    main()
