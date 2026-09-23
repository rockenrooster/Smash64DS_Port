import json, sys, os
ART = r'D:\Stuff\DevFolder\Smash64DS_Port\artifacts\performance\2026-09-23_p2-2p8-phase1-slice1'
arms = sys.argv[1:]
data = []
for a in arms:
    p = a if a.endswith('.json') else os.path.join(ART, a + '.json')
    d = json.load(open(p))
    ex = {e['name']: e.get('value') for e in d['extras']}
    data.append((a, d, ex))
names = []
for _, _, ex in data:
    for n in ex:
        if n not in names:
            names.append(n)
print('arm'.ljust(48) + ''.join(a[:16].rjust(17) for a, _, _ in data))
for a, d, ex in data:
    pass
for n in names:
    vals = [ex.get(n) for _, _, ex in data]
    if all(v in (0, None) for v in vals):
        continue
    short = n.replace('gNdsFtrLean.', 'L.')
    print(short.ljust(48) + ''.join((str(v) if v is not None else '-').rjust(17) for v in vals))
for a, d, ex in data:
    print(a, 'setGlobals', d.get('setGlobals'), 'rows', len(d['rows']), 'cadenceViolations', d.get('cadenceViolations'),
          'vbi2..5+', d.get('vbi2'), d.get('vbi3'), d.get('vbi4'), d.get('vbi5plus'), 'gitDirty', d.get('gitDirtyPaths'))
