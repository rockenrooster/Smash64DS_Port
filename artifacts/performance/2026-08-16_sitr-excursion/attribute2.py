"""The 288-frame attach/force-load population, per symbol, against a matched control.

That population is what rerank.py prices at 72,768 ticks at rank-80 on the
build-c220-camship basis.  The control is DERIVED rather than sampled: `rest` is
every frame that is not in event288, computed as
(whole*1601 - event288*288)/1313, so no second mask and no second scan is needed
and the two populations are exhaustive and disjoint by construction.

UNITS: 2 profile cycles = 1 project tick.
"""
import csv
import bisect

D = 'artifacts/performance/2026-08-16_sitr-excursion/'
N = {'whole': 1601, 'sitr25': 25, 'event288': 288, 'quiet': 773, 'card': 7}
COLS = ['instructions', 'total_cycles', 'issue', 'icache_fill', 'dcache_fill',
        'interlock']
MASKS = ['whole', 'sitr25', 'event288', 'quiet', 'card']

syms = []
for line in open(D + 'c221-nm.txt'):
    p = line.split()
    if len(p) == 4 and p[2] in 'tTwW':
        a = int(p[0], 16)
        s = int(p[1], 16)
        if s:
            syms.append((a, a + s, p[3]))
syms.sort()
starts = [s[0] for s in syms]


def owner(pc):
    i = bisect.bisect_right(starts, pc) - 1
    if i >= 0 and pc < syms[i][1]:
        return syms[i][2]
    return '<unattributed>'


agg = {}
pcrow = {}
for r in csv.DictReader(open(D + 'c221-multimask.csv')):
    pc = int(r['pc'], 16)
    pcrow[pc] = r
    o = owner(pc)
    a = agg.get(o)
    if a is None:
        a = agg[o] = {m: {c: 0 for c in COLS} for m in MASKS}
    for m in MASKS:
        for c in COLS:
            a[m][c] += int(r[m + '_' + c])


def tk(o, m, c='total_cycles'):
    return agg[o][m][c] / (2.0 * N[m])


def rest(o, c='total_cycles'):
    return (agg[o]['whole'][c] - agg[o]['event288'][c]) / (2.0 * (1601 - 288))


print('SITR EXCURSION, PER SYMBOL -- profile build-c221-sitrprof')
print('(c220 config + the profiler; config-diff.txt is 4 lines and all 4 are the')
print('profiler or the git string, so this is the first per-PC census taken in the')
print('SHIPPING configuration: GX_COMPOSE 0, FTANIM_TRACK 0, CAMERA_FIXED 1)')
print('window 1,600 presented frames 439-2038, regions=1601')
print('alignment region = frame - 439, MEASURED (align.py): the 7 card-read frames land')
print('at median profile rank 12 of 1601 there and 315-699 at every other offset in')
print('434..444; r(profile non-idle, tick-HUD WORK-H) = +0.694 at 439, +0.342 at 438,')
print('+0.338 at 440.  2 profile cycles = 1 project tick.')
print('')
print('MASKS  event288 = every frame carrying a figatree attach or an animation')
print('                  force-load (per-frame counters, offset measured at +1)')
print('       rest     = the other 1,313 frames, derived (whole*1601-event*288)/1313')
print('       quiet    = 773 below-median-WORK-H frames carrying neither event')
print('       sitr25   = the 25 over-gate frames whose dominant excess leaf is SITR')
print('')

rows = sorted(((tk(o, 'event288') - rest(o), o) for o in agg), reverse=True)
H = '%-52s %9s %9s %9s %9s %9s %8s'
print(H % ('symbol', 'event288', 'rest', 'DELTA', 'quiet', 'sitr25', 'icache d'))
for d, o in rows[:40]:
    tag = o + ('  (IDLE)' if o == 'armWaitForIrq' else '')
    print(H % (tag[:52], '%.0f' % tk(o, 'event288'), '%.0f' % rest(o), '%.0f' % d,
               '%.0f' % tk(o, 'quiet'), '%.0f' % tk(o, 'sitr25'),
               '%.0f' % (tk(o, 'event288', 'icache_fill') - rest(o, 'icache_fill'))))
print('')
print('NEGATIVE DELTAS (cheaper on an event frame)')
for d, o in rows[-6:]:
    print(H % (o[:52], '%.0f' % tk(o, 'event288'), '%.0f' % rest(o), '%.0f' % d,
               '%.0f' % tk(o, 'quiet'), '%.0f' % tk(o, 'sitr25'), ''))

GROUPS = [
    ('ATTACH chain (status transition -> figatree attach)',
     ['battleship_ftMainSetStatus', 'lbCommonAddFighterPartsFigatree',
      'gcAddDObjAnimJoint', 'ndsRelocAssetIDForToken',
      'ndsRelocNormalizeFighterAObj16File', 'ndsRelocApplyWordByteSwap',
      'ndsRelocRemoveFighterAObj16StatusAliases', 'ndsRelocSetStatusBufferFile',
      'ndsBattlePackContains', 'ndsRelocFindLoadedFileContaining',
      'ndsRelocGetFileData']),
    ('ANIM parse (re-derives AObj fields from static FIGATREE bytes)',
     ['ndsR2FtAnimParseDObjFigatree',
      'ndsR2AnimBuildTrackTable.constprop.0.isra.0',
      'ndsR2AnimTargetValue.constprop.0', 'ndsR2AnimAObjToQConvert']),
    ('ANIM evaluate',
     ['gcPlayDObjAnimJoint', 'ndsR2AnimValueQ', 'ndsR2CubicValueFixed',
      'ftParamUpdateAnimKeys', 'ndsFTParamsInvalidateSubtree',
      'ndsBaseGcPlayMObjMatAnim', 'gcParseMObjMatAnimJoint']),
    ('CARD I/O stack (FAT + NitroFS + newlib + calico locks)',
     ['get_fat.isra.0', 'f_read', 'f_lseek', '_FAT_read_r', '_read_r', 'read',
      'validate', 'move_window', 'mutexLock', 'mutexUnlock', 'threadRemoveWaiter',
      '_dvmDiscCacheReadWrite', '_nitroromFdRead', 'nitroromReadIter',
      '__libc_lock_acquire', '__libc_lock_release', '__syscall_getreent',
      '__getreent', 'strncasecmp', 'disk_read', 'check_fs', 'find_volume']),
    ('SOFT FLOAT leaves',
     ['__aeabi_fadd', '__mulsf3', '__divsf3', '__aeabi_f2iz', '__aeabi_fcmple',
      '__aeabi_fcmpge', '__aeabi_fcmplt', '__aeabi_fcmpgt', '__aeabi_fcmpeq',
      '__aeabi_ui2f', '__floatsisf', '__aeabi_l2f']),
    ('MEMORY movers', ['memcpy', 'memset', 'armCopyMem32', 'memmove']),
]
print('')
print('GROUPED, tk/fr  (each symbol counted once)')
G = '%-58s %10s %10s %10s %10s'
print(G % ('group', 'event288', 'rest', 'DELTA', 'sitr25-rest'))
seen = set()
for name, members in GROUPS:
    e = q = s = 0.0
    for n in members:
        if n in agg and n not in seen:
            seen.add(n)
            e += tk(n, 'event288')
            q += rest(n)
            s += tk(n, 'sitr25')
    print(G % (name, '%.0f' % e, '%.0f' % q, '%.0f' % (e - q), '%.0f' % (s - q)))
oe = sum(tk(o, 'event288') for o in agg if o not in seen and o != 'armWaitForIrq')
oq = sum(rest(o) for o in agg if o not in seen and o != 'armWaitForIrq')
os_ = sum(tk(o, 'sitr25') for o in agg if o not in seen and o != 'armWaitForIrq')
print(G % ('everything else (non-idle)', '%.0f' % oe, '%.0f' % oq,
           '%.0f' % (oe - oq), '%.0f' % (os_ - oq)))
ne = sum(tk(o, 'event288') for o in agg if o != 'armWaitForIrq')
nq = sum(rest(o) for o in agg if o != 'armWaitForIrq')
ns = sum(tk(o, 'sitr25') for o in agg if o != 'armWaitForIrq')
print(G % ('TOTAL non-idle', '%.0f' % ne, '%.0f' % nq, '%.0f' % (ne - nq),
           '%.0f' % (ns - nq)))
print(G % ('armWaitForIrq (idle)', '%.0f' % tk('armWaitForIrq', 'event288'),
           '%.0f' % rest('armWaitForIrq'),
           '%.0f' % (tk('armWaitForIrq', 'event288') - rest('armWaitForIrq')),
           '%.0f' % (tk('armWaitForIrq', 'sitr25') - rest('armWaitForIrq'))))

print('')
print('ENTRY-PC CALL RATES, calls per frame.  The profiler is instruction-accurate, so')
print('the instruction count at a function entry PC is exactly its call count.')
C = '%-52s %10s %10s %10s %10s %8s'
print(C % ('symbol', 'event288', 'rest', 'sitr25', 'whole', 'ratio'))
WANT = ['battleship_ftMainSetStatus', 'lbCommonAddFighterPartsFigatree',
        'gcAddDObjAnimJoint', 'ndsRelocAssetIDForToken',
        'ndsRelocNormalizeFighterAObj16File', 'ndsRelocSetStatusBufferFile',
        'ndsR2FtAnimParseDObjFigatree',
        'ndsR2AnimBuildTrackTable.constprop.0.isra.0',
        'ndsR2AnimTargetValue.constprop.0', 'ndsR2AnimAObjToQConvert',
        'gcPlayDObjAnimJoint', 'ndsR2AnimValueQ', 'ftParamUpdateAnimKeys',
        'get_fat.isra.0', 'ftMainProcUpdateInterrupt',
        'battleship_ftMainProcUpdateInterrupt', 'ftComputerProcessAll',
        'ftMainPlayAnim']
for name in WANT:
    for a, _b, n in syms:
        if n != name:
            continue
        r = pcrow.get(a)
        if r:
            e = int(r['event288_instructions']) / 288.0
            rr = (int(r['whole_instructions']) -
                  int(r['event288_instructions'])) / 1313.0
            ratio = ('%.2fx' % (e / rr)) if rr else 'inf'
            print(C % (name[:52], '%.2f' % e, '%.2f' % rr,
                       '%.2f' % (int(r['sitr25_instructions']) / 25.0),
                       '%.2f' % (int(r['whole_instructions']) / 1601.0), ratio))
        break
