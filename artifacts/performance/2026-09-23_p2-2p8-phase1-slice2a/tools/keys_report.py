"""Decode the per-slot fighter texture keys of a slice 2a census-b run."""
import json, os, sys, collections

ART = r'D:\Stuff\DevFolder\Smash64DS_Port\artifacts\performance\2026-09-23_p2-2p8-phase1-slice2a'
arm = sys.argv[1] if len(sys.argv) > 1 else 'census-b2'
d = json.load(open(os.path.join(ART, arm + '.json')))
ex = {e['name']: e.get('value') for e in d['extras']}
BPP = [0, 8, 2, 4, 8, 2, 8, 16]
DSFMT = ['none', 'A3I5', 'Pal4', 'Pal16', 'Pal256', '4x4', 'A5I3', 'Direct']
N64FMT = ['RGBA', 'YUV', 'CI', 'IA', 'I', '?5', '?6', '?7']
N64SIZ = ['4b', '8b', '16b', '32b']
PALB = [0, 8, 32, 512]
KIND = ['DK (slot 0)', 'Samus (slot 1)', 'Link (slot 2)', 'Kirby (slot 3)']
out = []
allkeys = []
for slot in range(4):
    g = lambda f: ex.get('gNdsVramCensus.%s[%d]' % (f, slot))
    n = g('key_count') or 0
    tex_total = pal_total = 0
    rows = []
    for k in range(min(n, 48)):
        w = [ex.get('gNdsVramCensus.key[%d][%d][%d]' % (slot, k, i)) for i in range(3)]
        w2 = w[2]
        fmt, siz, dsf = w2 & 7, (w2 >> 3) & 3, (w2 >> 5) & 7
        s, t, pc, cnt = (w2 >> 8) & 7, (w2 >> 11) & 7, (w2 >> 14) & 3, w2 >> 20
        rf = (w2 >> 16) & 0xf
        width, height = 8 << s, 8 << t
        tb = width * height * BPP[dsf] // 8
        pb = PALB[pc]
        tex_total += tb
        pal_total += pb
        img = (w[0] >> 20, w[0] & 0xfffff) if w[0] != 0xffffffff else None
        tl = None if w[1] == 0xffffffff else ('unresolved' if w[1] == 0xfffffffe else (w[1] >> 20, w[1] & 0xfffff))
        rows.append((img, tl, N64FMT[fmt] + N64SIZ[siz], DSFMT[dsf], width, height, tb, pb, cnt))
        allkeys.append((slot, img, tl, fmt, siz, dsf, width, height, tb, pb, cnt))
    out.append((slot, n, g('key_overflow'), g('key_uploads'), g('key_uploads_after_go'), g('key_prov_fail'), tex_total, pal_total, rows))

print('run', arm, 'rom', d.get('romSha256', '')[:16])
print('%-16s %6s %6s %8s %10s %8s %10s %9s' % ('slot', 'keys', 'ovfl', 'uploads', 'after GO', 'provfail', 'texel B', 'pal B'))
for slot, n, ov, up, upgo, pf, tb, pb, rows in out:
    print('%-16s %6d %6s %8s %10s %8s %10s %9s' % (KIND[slot], n, ov, up, upgo, pf, '{:,}'.format(tb), '{:,}'.format(pb)))
print()
for slot, n, ov, up, upgo, pf, tb, pb, rows in out:
    print('==', KIND[slot])
    files = collections.Counter(r[0][0] if r[0] else None for r in rows)
    print('   image files (asset id: keys):', {('0x%x' % k if k is not None else None): v for k, v in files.items()})
    for r in sorted(rows, key=lambda r: (r[0] or (0, 0))):
        img = '0x%03x+0x%05x' % r[0] if r[0] else 'unresolved'
        tl = ('0x%03x+0x%05x' % r[1]) if isinstance(r[1], tuple) else str(r[1])
        print('   %-16s tlut %-16s %-7s -> %-6s %3dx%-3d %6d B pal %3d  uploads %d' % (img, tl, r[2], r[3], r[4], r[5], r[6], r[7], r[8]))
json.dump({'slots': [{'slot': s, 'keys': [list(map(lambda x: list(x) if isinstance(x, tuple) else x, r)) for r in rows]} for s, n, ov, up, upgo, pf, tb, pb, rows in out]},
          open(os.path.join(ART, arm + '-keys.json'), 'w'), indent=1)
