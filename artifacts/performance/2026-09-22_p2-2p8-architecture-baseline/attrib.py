#!/usr/bin/env python3
"""Module/subsystem self-time attribution of a melonDS per-PC ARM9 profile.

Outputs:
  - per-symbol totals (cycles, insns) with file
  - per-subsystem avg ticks/frame (ticks = cycles/2)
  - per-region subsystem matrix -> median vs tail composition
"""
import bisect, csv, collections, json, re, sys, statistics

NM = sys.argv[1]
CSV = sys.argv[2]
OUT = sys.argv[3]

syms = []  # (addr, size, name, file)
for line in open(NM, encoding='utf-8', errors='replace'):
    line = line.rstrip('\n')
    parts = line.split('\t')
    head = parts[0].split()
    f = parts[1] if len(parts) > 1 else ''
    if len(head) == 4:
        addr, size, typ, name = head
        size = int(size, 16)
    elif len(head) == 3:
        addr, typ, name = head
        size = 0
    else:
        continue
    if typ not in 'tTwW':
        continue
    a = int(addr, 16) & ~1
    f = re.sub(r':\d+$', '', f).replace('\\', '/')
    syms.append((a, size, name, f))
syms.sort()
# dedupe by address: prefer sized, non-dot names
byaddr = {}
for a, s, n, f in syms:
    cur = byaddr.get(a)
    if cur is None or (cur[1] == 0 and s) or (cur[2].startswith('.') and not n.startswith('.')) or (cur[2].startswith('$')):
        byaddr[a] = (a, s, n, f)
syms = sorted(byaddr.values())
addrs = [s[0] for s in syms]

def lookup(pc):
    i = bisect.bisect_right(addrs, pc) - 1
    if i < 0:
        return None
    a, s, n, f = syms[i]
    if s and pc >= a + s:
        # maybe inside next-lower sized symbol? treat as the nearest anyway
        return (n + '?', f)
    return (n, f)

def classify(name, f):
    n = name.rstrip('?')
    fl = f.lower()
    if n == 'armWaitForIrq':
        return 'IDLE'
    if n in ('tickGetCount',) or 'DebugHud' in n or 'diagnostics_' in fl or 'census' in fl:
        return 'INSTRUMENT'
    if re.match(r'^(__aeabi_f|__aeabi_d|__addsf3|__subsf3|__mulsf3|__divsf3|__fixsfsi|__fixunssfsi|__floatsisf|__floatunsisf|__floatdisf|__floatundisf|__extendsfdf2|__truncdfsf2|__eqsf2|__nesf2|__ltsf2|__lesf2|__gtsf2|__gesf2|__unordsf2|__cmpsf2|__aeabi_cf|__ieee754_sqrtf|sqrtf|__fixsfdi|__fixunssfdi|__adddf3|__muldf3|__divdf3|__subdf3|__floatsidf|__fixdfsi|ndsF32|__aeabi_l2f|__aeabi_ul2f|__aeabi_i2f|__aeabi_ui2f|__aeabi_f2)', n) or n.startswith('ndsF32') or n.startswith('____ieee'):
        return 'SOFTFLOAT'
    if re.match(r'^(__udivsi3|__divsi3|__aeabi_idiv|__aeabi_uidiv|__aeabi_ldiv|__aeabi_uldiv|__udivmoddi4|__divdi3|__udivdi3|__moddi3|__umoddi3|__aeabi_lmul|__muldi3|\.udivsi3|\.divsi3|__aeabi_llsl|__aeabi_llsr|__aeabi_lasr|__ashldi3|__lshrdi3|__ashrdi3)', n) or 'Div64' in n:
        return 'INTHELPER'
    if n in ('memset', 'memcpy', 'memmove', 'armCopyMem32', 'armFillMem32', '__aeabi_memcpy', '__aeabi_memset', '__aeabi_memclr', 'bzero', 'dmaCopyWords', 'dmaFillWords') or 'DynamicArray' in n or n.startswith('__aeabi_mem'):
        return 'MEM'
    if 'lbcommon' in fl and ('Sin' in n or 'Cos' in n or 'Atan' in n or 'Sqrt' in n):
        return 'MATHLIB'
    if re.search(r'(sinf|cosf|atan2f|__kernel_|__ieee754|lbCommonSin|lbCommonCos|syUtils|syMath)', n):
        return 'MATHLIB'
    if 'audio' in fl or 'n_al' in fl or '/al/' in fl or 'nds_fgm' in fl or 'syaudio' in fl or 'sound' in fl or re.match(r'^(n_al|al[A-Z]|func_ovl0_|syAudio|nds(Audio|Bgm|Fgm|Sound))', n):
        return 'AUDIO'
    # renderer / presentation
    if 'nds_renderer' in fl or 'renderer_adapter' in fl or 'nds_native' in fl or 'nds_fighter' in fl or 'nds_ft_pose' in fl or 'nds_stage' in fl or 'nds_entry_effects' in fl or 'nds_ifcommon' in fl or 'nds_gx' in fl or 'nds_hud' in fl:
        if 'nds_ft_pose' in fl or n.startswith('ndsFtPose'):
            return 'POSE'
        if 'Stage' in n or 'stage' in fl:
            return 'RENDER_STAGE'
        if 'Fighter' in n or 'fighter' in fl or n.startswith('ndsFt') or 'Packet' in n:
            return 'RENDER_FIGHTER'
        if 'Particle' in n or 'Effect' in n or 'effect' in fl:
            return 'RENDER_EFFECT'
        if 'IF' in n or 'Hud' in n or 'HUD' in n or 'ifcommon' in fl:
            return 'RENDER_HUD'
        return 'RENDER_COMMON'
    if 'ftdisplay' in fl or n.startswith('ftDisplay'):
        return 'RENDER_FIGHTER'
    if 'lbparticle' in fl or n.startswith('lbParticle'):
        return 'PARTICLE'
    if '/ef/' in fl or 'efmanager' in fl or n.startswith('ef'):
        return 'EFFECT_LOGIC'
    if 'ftcomputer' in fl or n.startswith('ftComputer') or 'FTComputer' in n:
        return 'FT_AI'
    if 'objanim' in fl or re.match(r'^(gcPlay|gcParse|gcAddAnim|gcAdd.*Anim|ftParamUpdateAnimKeys|ftAnim|gcPlayAnimAll|gcPlayDObj|gcPlayMObj|ndsBaseGcPlay)', n) or 'AnimKeys' in n or 'MatAnim' in n:
        return 'ANIM'
    if ('Draw' in n or 'Display' in n) and ('movement' in fl or 'Stage' in n):
        return 'RENDER_STAGE'
    if 'mp_collision' in fl or '/mp/' in fl or n.startswith('mp') or 'StageMP' in n or 'StageCollision' in n or 'movement' in fl:
        return 'MAP_COLLISION'
    if 'gmcollision' in fl or n.startswith('gmCollision') or 'ftCollision' in n:
        return 'HIT_COLLISION'
    if 'gmcamera' in fl or n.startswith('gmCamera') or 'Camera' in n:
        return 'CAMERA'
    if '/ft/' in fl or 'battleship_ft' in fl or n.startswith('ft') or n.startswith('battleship_ft') or 'ftparam' in fl or 'FTParams' in n or 'reloc_backend_ft' in fl:
        return 'FT_LOGIC'
    if '/it/' in fl or 'battleship_item' in fl or n.startswith('it') and n[2:3].isupper():
        return 'ITEM_LOGIC'
    if '/wp/' in fl or 'weapon' in fl or n.startswith('wp') and n[2:3].isupper():
        return 'WEAPON_LOGIC'
    if '/gr/' in fl or 'ground' in fl or n.startswith('gr') and n[2:3].isupper():
        return 'STAGE_LOGIC'
    if '/if/' in fl or 'ifcommon' in fl or n.startswith('if') and n[2:3].isupper():
        return 'HUD_LOGIC'
    if 'objman' in fl or 'taskman' in fl or 'objdisplay' in fl or n.startswith('gc') or 'Gc' in n or 'GObj' in n or 'DObj' in n or 'objhelper' in fl:
        return 'OBJMAN'
    if 'reloc' in fl:
        return 'RELOC'
    if 'libnds' in fl or 'calico' in fl or 'libdvm' in fl or 'dldi' in fl or 'fat' in fl:
        return 'SYSLIB'
    if '/sc/' in fl or 'scvsbattle' in fl or '/gm/' in fl:
        return 'SCENE'
    if 'lbcommon' in fl or n.startswith('lb'):
        return 'LBCOMMON'
    if 'nds_platform' in fl or 'nds_input' in fl or 'controller' in fl or 'nds_irq' in fl or 'main.c' in fl:
        return 'PLATFORM'
    return 'OTHER'

sym_cyc = collections.Counter()
sym_ins = collections.Counter()
sym_file = {}
reg_cls = collections.defaultdict(collections.Counter)
reg_tot = collections.Counter()
cache = {}
with open(CSV, newline='') as fh:
    r = csv.reader(fh)
    next(r)
    for row in r:
        region = int(row[0])
        pc = int(row[1], 16) & ~1
        ins = int(row[4]); cyc = int(row[5])
        key = cache.get(pc)
        if key is None:
            res = lookup(pc)
            if res is None:
                key = ('?', '', 'UNATTR')
            else:
                key = (res[0], res[1], classify(res[0], res[1]))
            cache[pc] = key
        n, f, c = key
        sym_cyc[n] += cyc
        sym_ins[n] += ins
        sym_file[n] = (f, c)
        reg_cls[region][c] += cyc
        reg_tot[region] += cyc

regions = sorted(reg_tot)
R = len(regions)
cls_tot = collections.Counter()
for rg in regions:
    cls_tot.update(reg_cls[rg])

out = {
    'regions': R,
    'class_avg_ticks': {c: v / 2 / R for c, v in cls_tot.most_common()},
    'symbols': [
        {'sym': n, 'file': sym_file[n][0], 'class': sym_file[n][1], 'ticks_per_frame': sym_cyc[n] / 2 / R, 'ins_per_frame': sym_ins[n] / R, 'cpi': (sym_cyc[n] / sym_ins[n]) if sym_ins[n] else 0}
        for n, _ in sym_cyc.most_common(400)
    ],
    'region_class_ticks': {rg: {c: v / 2 for c, v in reg_cls[rg].items()} for rg in regions},
}
json.dump(out, open(OUT, 'w'), indent=1)

# print summary
work = {rg: (reg_tot[rg] - reg_cls[rg]['IDLE']) / 2 for rg in regions}
ws = sorted(regions, key=lambda x: work[x])
print('regions', R)
print('work ticks P50 %.0f  P90 %.0f  P95 %.0f  max %.0f' % (
    work[ws[R // 2]], work[ws[int(R * 0.9)]], work[ws[int(R * 0.95)]], work[ws[-1]]))
print()
print('%-16s %10s %6s   %10s %10s %10s' % ('class', 'avg tk/fr', '%work', 'med-band', 'tail-band', 'tail-med'))
lo = ws[int(R * 0.4):int(R * 0.6)]
hi = ws[int(R * 0.9):]
tot_work = sum(v for c, v in cls_tot.items() if c != 'IDLE') / 2 / R
for c, v in cls_tot.most_common():
    avg = v / 2 / R
    m = statistics.mean(reg_cls[rg][c] / 2 for rg in lo)
    t = statistics.mean(reg_cls[rg][c] / 2 for rg in hi)
    pct = 100 * avg / tot_work if c != 'IDLE' else 0
    print('%-16s %10.0f %6.1f   %10.0f %10.0f %10.0f' % (c, avg, pct, m, t, t - m))
print('total work avg %.0f' % tot_work)
