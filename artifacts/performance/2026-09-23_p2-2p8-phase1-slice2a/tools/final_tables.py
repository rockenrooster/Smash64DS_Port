import itertools, json, os
ART = r'D:\Stuff\DevFolder\Smash64DS_Port\artifacts\performance\2026-09-23_p2-2p8-phase1-slice2a'
d = json.load(open(os.path.join(ART, 'reachable-final.json')))
S = d['summary']
KINDS = ['Mario', 'Fox', 'Donkey', 'Samus', 'Luigi', 'Link', 'Yoshi', 'Captain', 'Kirby',
         'Pikachu', 'Purin', 'Ness']
# Kirby's body material cycles palettes within one match (runtime saw 5 of them):
KIRBY_PAL_VARIANTS = 4 * (512 + 32)
HATS = S['Kirby']['hats']


def inst(k):
    s = S[k]
    return s['low_instance'][0] + s['low_instance'][1] + (KIRBY_PAL_VARIANTS if k == 'Kirby' else 0)


print('| kind | LOW per instance (texel B / palette B / textures) | LOW all palettes | HIGH per instance | other-file textures (B / n) | runtime keys (B) | runtime keys outside LOW enumeration |')
print('|---|---:|---:|---:|---:|---:|---:|')
for k in KINDS:
    s = S[k]
    f = lambda t: '{:,} / {:,} / {}'.format(*t)
    rt = ('{} ({:,})'.format(s['runtime_keys'], s['runtime_bytes'])) if s['runtime_keys'] else '-'
    print('| %s | %s | %s | %s | %s | %s | %s |' % (
        k, f(s['low_instance']), f(s['low_all_palettes']), f(s['high_instance']),
        '{:,} / {}'.format(*s['other']), rt, len(s['runtime_not_in_low']) if s['runtime_keys'] else '-'))
print()
print('hats', HATS)
combos = []
for c in itertools.combinations(KINDS, 4):
    b = sum(inst(k) for k in c)
    if 'Kirby' in c:
        b += sum(HATS[o][0] + HATS[o][1] for o in c if o != 'Kirby')
    o = sum(S[k]['other'][0] for k in c)
    combos.append((b, o, c))
print('heaviest by body (LOW per instance + palettes):')
for b, o, c in sorted(combos, reverse=True)[:5]:
    print('  {:,}  (+other files {:,})  {}'.format(b, o, ' + '.join(c)))
print('heaviest by body + other-file textures:')
for b, o, c in sorted(combos, key=lambda x: -(x[0] + x[1]))[:5]:
    print('  {:,}  (body {:,} + other {:,})  {}'.format(b + o, b, o, ' + '.join(c)))
ex = [x for x in combos if set(x[2]) == {'Donkey', 'Samus', 'Link', 'Kirby'}][0]
print('stress roster: body {:,} other {:,}'.format(ex[0], ex[1]))
