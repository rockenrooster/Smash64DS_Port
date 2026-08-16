"""If the SHDT excursion is not per pair, what is it per?

pair_scaling.py refuted the per-pair model: OLS slope -440 tk/pair at R2 = 0.003
over the 57 engaged frames, and the implied per-pair figure collapses 66x across
the count buckets. This script asks the follow-up two ways:

  1. The mask used by c191-mask-damagecollide.txt took the top 88 regions by
     gmCollisionCheckFighterAttackDamageCollide cycles, but only 57 regions
     execute it at all -- so 31 of those 88 carried ZERO engagement and diluted
     every premium in that table. Re-rank on the exact engaged set.
  2. Pair counts are quantised (11 hurtboxes x 2 sim ticks per live attack
     collision). Compare the 22-pair and 44-pair populations directly: if the
     per-JOINT work is latched once per frame as the source claims, doubling the
     attack collisions must leave the joint work flat.

Basis: build-c191-sitr-profile-c185, regions=1601, ticks = cycles / (2 x 1600).
Call counts are exact (entry pc executes once per call).
"""
import sys
import numpy as np

z = np.load(sys.argv[1], allow_pickle=True)
cyc, calls, names = z['cyc'][1:1601], z['calls'][1:1601], list(z['names'])
REG = cyc.shape[0]
idx = {n: i for i, n in enumerate(names)}
PAIR, IDLE = 'gmCollisionCheckFighterAttackDamageCollide', 'armWaitForIrq'
npair = calls[:, idx[PAIR]]
work = cyc.sum(1) - cyc[:, idx[IDLE]]

eng = npair > 0
print(f'engaged regions {int(eng.sum())} of {REG}   pair-count histogram:')
u, c = np.unique(npair[eng], return_counts=True)
print('   ' + '  '.join(f'{int(a)}x{int(b)}' for a, b in zip(u, c)))

# ---------- 1. the ranking on the EXACT engaged set --------------------------
base = cyc[~eng].mean(0)
prem = cyc[eng].mean(0) - base
callb, callc = calls[eng].mean(0), calls[~eng].mean(0)
work_prem = (work[eng].mean() - work[~eng].mean()) / 2
print(f'\n=== engaged ({int(eng.sum())}) vs not ({int((~eng).sum())}) ===')
print(f'non-idle work premium {work_prem:,.0f} tk/fr   '
      f'(idle armWaitForIrq premium {prem[idx[IDLE]]/2:,.0f})')
ordr = np.argsort(-prem)
print(f'{"+tk/fr":>9s} {"callsE":>9s} {"callsC":>9s} {"ratio":>7s}  symbol')
shown = 0
for i in ordr:
    if names[i] == IDLE or prem[i] / 2 < 400:
        continue
    r = callb[i] / callc[i] if callc[i] > 0.005 else float('inf')
    print(f'{prem[i]/2:>9,.0f} {callb[i]:>9.2f} {callc[i]:>9.2f} {r:>7.1f}  {names[i]}')
    shown += 1
    if shown >= 24:
        break

# ---------- 2. 22 pairs vs 44 pairs: is the joint work latched? --------------
a, b = npair == 22, npair == 44
print(f'\n=== 22-pair frames (n={int(a.sum())}) vs 44-pair frames (n={int(b.sum())}) ===')
print('  a live attack collision contributes 11 hurtboxes x 2 sim ticks = 22 pairs,')
print('  so the 44-pair frames run EXACTLY TWICE the pair tests of the 22-pair ones.')
print(f'\n{"symbol":44s} {"22p":>9s} {"44p":>9s} {"x":>6s}   {"22p tk":>9s} {"44p tk":>9s}')
for s in ['gmCollisionCheckFighterAttackDamageCollide', 'gmCollisionTestRectangle',
          'gmCollisionGetWorldPosition', 'func_ovl2_800EDE5C',
          'gmCollisionSetInvertMatrix', 'func_ovl2_800EDBA4', 'func_ovl2_800ED490',
          'gmCollisionTransformMatrixAll', 'gmCollisionCheckFighterInFighterRange',
          '__aeabi_fadd', '__aeabi_fmul', '__aeabi_fdiv', 'sqrtf',
          'lbCommonSin', 'lbCommonCos']:
    if s not in idx:
        continue
    i = idx[s]
    ca, cb = calls[a, i].mean(), calls[b, i].mean()
    print(f'  {s:42s} {ca:>9.2f} {cb:>9.2f} {cb/ca if ca else 0:>6.2f}   '
          f'{cyc[a, i].mean()/2:>9,.0f} {cyc[b, i].mean()/2:>9,.0f}')
print(f'  {"NON-IDLE TOTAL":42s} {"":>9s} {"":>9s} {"":>6s}   '
      f'{work[a].mean()/2:>9,.0f} {work[b].mean()/2:>9,.0f}')
print(f'  {"non-idle lift over the zero-pair median":42s} '
      f'{(work[a].mean()-np.median(work[~eng]))/2:>32,.0f} '
      f'{(work[b].mean()-np.median(work[~eng]))/2:>9,.0f}')

# ---------- 3. what the per-frame event costs, sized per JOINT ---------------
inv = calls[eng, idx['gmCollisionSetInvertMatrix']].sum()
print(f'\n=== the per-frame event ===')
print(f'  gmCollisionSetInvertMatrix over the match {int(inv):,} calls on '
      f'{int(eng.sum())} engaged frames = {inv/eng.sum():.2f} joints/engaged frame')
print(f'  (FTDAMAGECOLL_NUM_MAX = 11, include/ft/fighter.h:322)')
sf = (calls[eng, idx['__aeabi_fadd']].mean() + calls[eng, idx['__aeabi_fmul']].mean()
      - calls[~eng, idx['__aeabi_fadd']].mean() - calls[~eng, idx['__aeabi_fmul']].mean())
sfc = (cyc[eng, idx['__aeabi_fadd']].mean() + cyc[eng, idx['__aeabi_fmul']].mean()
       - cyc[~eng, idx['__aeabi_fadd']].mean() - cyc[~eng, idx['__aeabi_fmul']].mean())
print(f'  extra __aeabi_fadd+fmul calls per engaged frame {sf:,.0f}  '
      f'costing {sfc/2:,.0f} tk/fr  = {sfc/sf:.1f} cycles/call')
print(f'  that is {sfc/2/work_prem*100:.1f}% of the {work_prem:,.0f} tk/fr work premium')
