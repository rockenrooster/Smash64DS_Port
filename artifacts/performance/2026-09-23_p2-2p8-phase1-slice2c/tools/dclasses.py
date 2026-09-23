"""VRAM by owner class and the part of it in bank D (census snapshots)."""
import bisect, json, os, subprocess, sys
ART = r'D:\Stuff\DevFolder\Smash64DS_Port\artifacts\performance\2026-09-23_p2-2p8-phase1-slice2c'
ELF = r'D:\Stuff\DevFolder\Smash64DS_Port\builds\build-p2-fourcpu-tickhud\smash64ds-p2-fourcpu-tickhud-hwtri.elf'
NM = r'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
FIXED = ['cache:stage', 'cache:static', 'cache:stage_warm', 'cache:slot0 DK', 'cache:slot1 Samus', 'cache:slot2 Link',
         'cache:slot3 Kirby', 'cache:other', 'entry:startup', 'entry:kept', 'tint', 'halo+shield']
arm = sys.argv[1]
d = json.load(open(os.path.join(ART, arm + '.json')))
ex = {e['name']: e.get('value') for e in d['extras']}
syms = []
for line in subprocess.run([NM, '-S', '--defined-only', ELF], capture_output=True, text=True).stdout.splitlines():
    p = line.split()
    if len(p) == 4 and p[2] in 'tTwW':
        syms.append((int(p[0], 16), int(p[1], 16), p[3]))
syms.sort(); starts = [s[0] for s in syms]
def func_of(pc):
    i = bisect.bisect_right(starts, pc) - 1
    return syms[i][2] if i >= 0 and syms[i][0] <= pc < syms[i][0] + syms[i][1] else '?%08x' % pc
n = ex.get('gNdsVramCensusSiteCount') or 0
names = FIXED + ['site:' + func_of(ex.get('gNdsVramCensusSitePc[%d]' % i) or 0) for i in range(30)] + ['untagged']
print('%-58s %10s %10s %10s %10s' % ('class', 'GO', 'GO in D', 'after', 'after in D'))
for i, nm in enumerate(names):
    v = [ex.get('gNdsVramCensus.%s.%s[%d]' % (s, a, i)) or 0 for s, a in (('go', 'bytes'), ('go', 'bytes_d'), ('after', 'bytes'), ('after', 'bytes_d'))]
    if any(v):
        print('%-58s %10s %10s %10s %10s' % ((nm[:58],) + tuple('{:,}'.format(x) for x in v)))
for s in ('go', 'after'):
    print(s, 'frame', ex.get('gNdsVramCensus.%s.frame' % s), 'tex_bytes', ex.get('gNdsVramCensus.%s.tex_bytes' % s),
          'in D', ex.get('gNdsVramCensus.%s.tex_bytes_d' % s), 'usable', ex.get('gNdsVramCensus.%s.tex_usable' % s),
          'free', ex.get('gNdsVramCensus.%s.tex_free_usable' % s), 'largest', ex.get('gNdsVramCensus.%s.tex_largest_usable' % s),
          'runs', ex.get('gNdsVramCensus.%s.tex_free_runs_usable' % s), 'vramcnt %08x' % (ex.get('gNdsVramCensus.%s.vramcnt_abcd' % s) or 0))
