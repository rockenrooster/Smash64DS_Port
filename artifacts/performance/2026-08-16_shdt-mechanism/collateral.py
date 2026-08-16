"""How much of an engaged frame's premium is EXTRA WORK, and how much is the rest
of the frame getting slower at unchanged work?

engaged_split.py showed several of the largest premium rows have a FLAT call
count -- memcpy 123.75 vs 118.73, lbParticleDrawTextures 4.00 vs 4.00,
ndsRendererAdapterBuildDObjXObjMatrix 57.39 vs 55.09 -- while their cycles rise.
Same work, more cycles, is not arithmetic; it is the frame's working set being
displaced. That distinction decides where a lever can aim, so it is measured
rather than argued.

Every symbol on the engaged set is classified by its own exact call ratio:

  MARGINAL    calls/frame rose >= 10%   -- work that genuinely happened more
  COLLATERAL  calls/frame within +-10%  -- the same calls, costing more cycles

The classification uses call counts only; the cycles are then summed inside each
class. A symbol cannot be assigned to the class that flatters it.

Basis: build-c191-sitr-profile-c185 (COLLISION_FIXED 0, GX_COMPOSE 0, BOTH_CPU 1,
BATTLEPACK 1, KEEP_CACHE 1, TICK_HUD_DRAW 0), regions=1601, engaged = the 57
regions that execute gmCollisionCheckFighterAttackDamageCollide at least once,
ticks = cycles / (2 x 1600).
"""
import sys
import numpy as np

z = np.load(sys.argv[1], allow_pickle=True)
cyc, calls, names = z['cyc'][1:1601], z['calls'][1:1601], list(z['names'])
idx = {n: i for i, n in enumerate(names)}
IDLE = 'armWaitForIrq'
eng = calls[:, idx['gmCollisionCheckFighterAttackDamageCollide']] > 0

cE, cC = cyc[eng].mean(0), cyc[~eng].mean(0)
kE, kC = calls[eng].mean(0), calls[~eng].mean(0)
prem = (cE - cC) / 2
work_prem = prem.sum() - prem[idx[IDLE]]
print(f'engaged {int(eng.sum())} regions vs {int((~eng).sum())} not')
print(f'non-idle work premium {work_prem:,.0f} tk/fr '
      f'(idle {prem[idx[IDLE]]:,.0f} excluded throughout)\n')

MARG, COLL, NEW = [], [], []
for i, n in enumerate(names):
    if n == IDLE or abs(prem[i]) < 1:
        continue
    if kC[i] < 0.005 and kE[i] >= 0.005:
        NEW.append(i)
    elif kC[i] >= 0.005 and kE[i] / kC[i] >= 1.10:
        MARG.append(i)
    elif kC[i] >= 0.005:
        COLL.append(i)
    else:
        COLL.append(i)

for label, group in (('NEW      (absent off the engaged set)', NEW),
                     ('MARGINAL (calls/frame up >= 10%)', MARG),
                     ('COLLATERAL (calls/frame flat within +-10%)', COLL)):
    tk = sum(prem[i] for i in group)
    print(f'{label:44s} {len(group):>4d} symbols  {tk:>10,.0f} tk/fr  '
          f'{tk/work_prem*100:>5.1f}%')

print('\n--- the COLLATERAL rows, largest first: same calls, more cycles ---')
print(f'{"+tk/fr":>9s} {"callsE":>9s} {"callsC":>9s} {"ratio":>6s} '
      f'{"tk/call E":>10s} {"tk/call C":>10s}  symbol')
for i in sorted(COLL, key=lambda j: -prem[j])[:18]:
    if prem[i] < 300:
        break
    pe = cE[i] / 2 / kE[i] if kE[i] > 0.005 else 0
    pc = cC[i] / 2 / kC[i] if kC[i] > 0.005 else 0
    print(f'{prem[i]:>9,.0f} {kE[i]:>9.2f} {kC[i]:>9.2f} '
          f'{kE[i]/kC[i] if kC[i] > 0.005 else 0:>6.2f} {pe:>10,.0f} {pc:>10,.0f}  {names[i]}')

print('\n--- the MARGINAL rows, largest first: work that genuinely happened ---')
print(f'{"+tk/fr":>9s} {"callsE":>9s} {"callsC":>9s} {"ratio":>6s}  symbol')
for i in sorted(MARG + NEW, key=lambda j: -prem[j])[:18]:
    if prem[i] < 300:
        break
    r = kE[i] / kC[i] if kC[i] > 0.005 else float('inf')
    print(f'{prem[i]:>9,.0f} {kE[i]:>9.2f} {kC[i]:>9.2f} {r:>6.1f}  {names[i]}')

# ---- named families inside the marginal half -------------------------------
FAM = {
    'fighter collision chain (self cycles only)': [
        'gmCollisionCheckFighterAttackDamageCollide', 'gmCollisionTestRectangle',
        'gmCollisionSetInvertMatrix', 'func_ovl2_800ED490', 'func_ovl2_800EDE5C',
        'func_ovl2_800EDBA4', 'gmCollisionGetWorldPosition',
        'gmCollisionTransformMatrixAll', 'gmCollisionCheckFighterInFighterRange',
        'gmCollisionGetFighterPartsWorldPosition'],
    'soft float (all helpers)': [n for n in names if n.startswith('__aeabi_f')
                                 or n in ('__mulsf3', '__divsf3', '__addsf3',
                                          '__subsf3', 'sqrtf')],
    'asset I/O (FAT / DLDI / card)': ['armCopyMem32', 'get_fat.isra.0', 'f_lseek',
                                      'f_read', '_dvmDiscCacheReadWrite', '_read_r',
                                      'disk_read', 'f_open'],
    'animation rebuild (status change)': [
        'ndsR2FtAnimParseDObjFigatree', 'ndsR2AnimValueQ', 'gcPlayDObjAnimJoint',
        'ndsBaseGcPlayDObjAnimJoint', 'ndsR2AnimBuildTrackTable.constprop.0.isra.0',
        'ftParamUpdateAnimKeys', 'ftMainParseMotionEvent'],
    'hit sound effect': ['ndsAudioFgmPlayAtPan'],
    'trig table (lbCommonSin/Cos)': ['lbCommonSin', 'lbCommonCos'],
}
print('\n--- named families, engaged premium (self cycles, disjoint sets) ---')
seen = set()
for lab, syms in FAM.items():
    ii = [idx[s] for s in syms if s in idx and idx[s] not in seen]
    seen.update(ii)
    tk = sum(prem[i] for i in ii)
    print(f'  {lab:44s} {tk:>10,.0f} tk/fr  {tk/work_prem*100:>5.1f}%')
rest = work_prem - sum(prem[i] for i in seen)
print(f'  {"everything else":44s} {rest:>10,.0f} tk/fr  {rest/work_prem*100:>5.1f}%')
