import json, os, re
D = r'D:\Stuff\DevFolder\Smash64DS_Port\artifacts\performance\2026-09-26_p2-2p8-ftr-item-tail'
CLIP = {'Mario': 378, 'Fox': 318, 'DK': 268, 'Samus': 508, 'Luigi': 378, 'Link': 557,
        'Yoshi': 728, 'Captain': 599, 'Kirby': 214, 'Pikachu': 758, 'Purin': 498, 'Ness': 373}
ROSTER = {'l': ['Ness', 'Yoshi', 'Pikachu', 'Purin'], 'b': ['Pikachu', 'Yoshi', 'Captain', 'Link'],
          's': ['DK', 'Samus', 'Link', 'Kirby'], 'm': ['Mario', 'Fox', 'Samus', 'Captain']}
BOUND = [1536, 2560, 2560, 4608, 1536, 3328, 3072, 2560, 3072]
NAME = ['Castle', 'Sector', 'Jungle', 'Zebes', 'Hyrule', 'Yoshi', 'DreamLand', 'Saffron', 'Mushroom']
MARGIN = 384


def hw(arm):
    j = os.path.join(D, arm + '.json')
    if os.path.exists(j):
        d = {x['name']: x['value'] for x in json.load(open(j))['extras']}
        return d.get('gNdsAObjEvent32NormalizedHighWater'), d.get('gNdsTaskmanGeneralHeapFreeMin'), 'end'
    lg = os.path.join(D, arm + '-run.log')
    if os.path.exists(lg):
        m = re.findall(r'TICKEXTRA=([0-9,]*)', open(lg, errors='replace').read())
        if m:
            v = m[-1].split(',')
            return int(v[-6]), int(v[-2]), 'fault'
    return None, None, 'missing'


arms = {'l': 'ev_l_g%d', 'b': 'ev_b_g%d', 's': 'ev_s_g%d', 'm': 'ev_m_g%d'}
for g in range(9):
    parts = []
    s_max = 0
    for r, pat in arms.items():
        arm = pat % g
        if r == 'b' and g == 3:
            arm = 'ev_bz_g3'
        if r == 's' and g == 3:
            arm = 'ev_sz_g3'
        h, heap, how = hw(arm)
        if h is None:
            continue
        sp = h - sum(CLIP[k] for k in ROSTER[r])
        s_max = max(s_max, sp)
        parts.append(f'{r}:{h}({how[0]},heap{heap})')
    worst = max(sum(CLIP[k] for k in v) for v in ROSTER.values())
    print(f'g{g} {NAME[g]:10} bound {BOUND[g]} S {s_max:5} | ' + ' '.join(parts))
