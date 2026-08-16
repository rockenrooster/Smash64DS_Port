"""Attribute the c200 per-PC census to symbols, for the C2 byte ledger.

Source: artifacts/performance/2026-08-15_ftanim-dispatch-attribution/c200-off-pc.csv
        builds/build-c200-trackprof-off/.../.elf  (nm -S, the same binary)
Window: marginal-80 frames.  tk/fr = marg_<column> / 80 / 2   (2 cycles = 1 tick)
"""
import csv, subprocess, os, sys, bisect

ROOT = r'D:\Stuff\DevFolder\Smash64DS_Port'
ELF = os.path.join(ROOT, 'builds', 'build-c200-trackprof-off',
                   'smash64ds-battle-playable-tickhud-hwtri.elf')
CSV = os.path.join(ROOT, 'artifacts', 'performance',
                   '2026-08-15_ftanim-dispatch-attribution', 'c200-off-pc.csv')
NM = os.path.join(os.environ.get('DEVKITARM', r'C:\devkitPro\devkitARM'),
                  'bin', 'arm-none-eabi-nm.exe')
FRAMES = 80.0
CYC_PER_TK = 2.0

syms = []
out = subprocess.run([NM, '-S', ELF], capture_output=True, text=True).stdout
for line in out.splitlines():
    p = line.split()
    if len(p) == 4 and p[2].lower() in 'tw':
        try:
            syms.append((int(p[0], 16), int(p[1], 16), p[3]))
        except ValueError:
            pass
syms.sort()
starts = [s[0] for s in syms]


def owner(pc):
    i = bisect.bisect_right(starts, pc) - 1
    if i < 0:
        return None
    a, n, name = syms[i]
    return name if pc < a + n else None


COLS = ['instructions', 'total_cycles', 'issue', 'icache_fill', 'dcache_fill',
        'write_buffer', 'interlock']
agg = {}
entry = {}
for r in csv.DictReader(open(CSV, newline='')):
    pc = int(r['pc'], 16)
    nm_ = owner(pc)
    if nm_ is None:
        continue
    a = agg.setdefault(nm_, dict.fromkeys(COLS, 0))
    for c in COLS:
        a[c] += int(r['marg_' + c])
    i = bisect.bisect_right(starts, pc) - 1
    if pc == syms[i][0]:
        entry[nm_] = int(r['marg_instructions'])

size = {n: s for _, s, n in syms}

WANT = sys.argv[1:] or [
    'ndsR2FtAnimParseDObjFigatree', 'ndsBaseGcPlayDObjAnimJoint',
    'ndsBaseGcPlayMObjMatAnim', 'gmCollisionTransformMatrixAll',
    'ndsR2CubicValueFixed', 'ndsR2AnimSegmentStart', 'ndsR2AnimAdvanceTail',
    'lbCommonSin', 'lbCommonCos', 'ndsRendererSyncTextureTile',
    'ndsRendererHardwareBindTextureName', 'glBindTexture',
]

hdr = (f"{'symbol':44}{'bytes':>7}{'ent/fr':>9}{'issue':>9}{'icache':>9}"
       f"{'dcache':>9}{'wbuf':>8}{'ilock':>8}{'TOTAL':>9}{'tk/B':>7}")
print(hdr)
print('-' * len(hdr))
tot = dict.fromkeys(COLS, 0)
tb = 0
for n in WANT:
    a = agg.get(n)
    if a is None:
        cands = [k for k in agg if n in k]
        if not cands:
            print(f'{n:44}{"NOT IN CENSUS":>60}')
            continue
        n2 = cands[0]
        a = agg[n2]
    else:
        n2 = n
    b = size.get(n2, 0)
    f = lambda c: a[c] / FRAMES / CYC_PER_TK
    t = f('total_cycles')
    print(f'{n2:44}{b:>7,}{entry.get(n2,0)/FRAMES:>9.1f}{f("issue"):>9,.0f}'
          f'{f("icache_fill"):>9,.0f}{f("dcache_fill"):>9,.0f}'
          f'{f("write_buffer"):>8,.0f}{f("interlock"):>8,.0f}{t:>9,.0f}'
          f'{(f("icache_fill")/b if b else 0):>7.2f}')
