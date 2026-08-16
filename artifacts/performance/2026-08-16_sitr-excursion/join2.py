"""Second counter pass: the status-transition / figatree-attach chain.

Same instrument, same ROM, same offset (+1) as join.py, which measured the
offset rather than assuming it.
"""
import csv, statistics as st, math
from collections import defaultdict

RING='artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'
PF='artifacts/performance/2026-08-16_sitr-excursion/pf220b-rows.csv'
PF1='artifacts/performance/2026-08-16_sitr-excursion/pf220-rows.csv'
CTR=['gNdsFighterNaturalMotionFigatreeAttachCount','gNdsShieldAnimJointInstallCalls',
     'gNdsR2FtAnimParseStepped','gNdsR2FtAnimParseEarlyOut','gNdsR2FtAnimRecipMisses',
     'gNdsR2AnimCacheBytes','gNdsR2RelocAliasVisits','gNdsFTComputerStatusChangeCount']
CTR1=['gNdsR204AnimForceLoadTotal','gNdsR2FtAnimParseCalls','gNdsR2CubicEvals']

ring={int(r['frame']):{k:int(v) for k,v in r.items()} for r in csv.DictReader(open(RING))}
def deltas(path,cols):
    rows=sorted(({k:int(v) for k,v in r.items()} for r in csv.DictReader(open(path))),
                key=lambda r:r['frame'])
    d={}
    for i in range(1,len(rows)):
        d[rows[i]['frame']]={c:rows[i][c]-rows[i-1][c] for c in cols}
    return rows,d
rows2,d2=deltas(PF,CTR)
rows1,d1=deltas(PF1,CTR1)
print('pass-2 counter totals:',{c:rows2[-1][c]-rows2[0][c] for c in CTR})

LEAF=['FTR','STG','BG','AUD','MISC','OTHRW','SRCRES','SWRM','GCRARES',
      'SITR','SCPU','SPHD','SPHC','SCAT','SHDT','SPRM']
def leaves(r):
    six=r['SINT']+r['SPHD']+r['SPHC']+r['SCAT']+r['SHDT']+r['SPRM']
    return dict(zip(LEAF,[r['FTR'],r['STG'],r['BG'],r['AUD'],r['MISC'],r['OTHR']-r['WAIT'],
        r['SRC']-r['GCRA']-r['SWRM'],r['SWRM'],r['GCRA']-six,r['SINT']-r['SCPU'],r['SCPU'],
        r['SPHD'],r['SPHC'],r['SCAT'],r['SHDT'],r['SPRM']]))
L={f:leaves(r) for f,r in ring.items()}
med={n:st.median([L[f][n] for f in L]) for n in LEAF}
allf=sorted(L)
OFF=1
def c2(f,c): return d2.get(f-OFF,{}).get(c,0)
def c1(f,c): return d1.get(f-OFF,{}).get(c,0)

# determinism control: the two runs are the same ROM, so pass-1 and pass-2 must
# agree wherever they overlap. They share no counter, so compare the ring-frame
# WORK-H series each run wrote instead.
r1w={r['frame']:r['WORK-H'] for r in rows1}
r2w={r['frame']:r['WORK-H'] for r in rows2}
common=set(r1w)&set(r2w)
print('pass1 vs pass2 WORK-H identical on %d of %d common frames'
      % (sum(1 for f in common if r1w[f]==r2w[f]),len(common)))

order=sorted(ring.values(),key=lambda r:-r['WORK-H'])
top=[r['frame'] for r in order[:80]]
def owner(f): return max(((L[f][n]-med[n],n) for n in LEAF))[1]
sitr=[f for f in top if owner(f)=='SITR']

def pearson(a,b):
    ma,mb=st.mean(a),st.mean(b)
    num=sum((x-ma)*(y-mb) for x,y in zip(a,b))
    den=math.sqrt(sum((x-ma)**2 for x in a)*sum((y-mb)**2 for y in b))
    return num/den if den else 0.0
S=[L[f]['SITR'] for f in allf]
print('\nPearson r against per-frame SITR (all 1,600 ring frames):')
for c in CTR:
    print('   %-42s r=%+.4f   median %6.0f  mean %9.2f  max %8d'
          % (c,pearson(S,[c2(f,c) for f in allf]),
             st.median([c2(f,c) for f in allf]),st.mean([c2(f,c) for f in allf]),
             max(c2(f,c) for f in allf)))

print('\n=== SITR cluster (%d frames) vs the rest, medians ===' % len(sitr))
rest=[f for f in allf if f not in set(sitr)]
hdr='%-42s %12s %12s %10s'
print(hdr % ('column','SITR cluster','all others','ratio'))
def show(name,fn):
    a=st.median([fn(f) for f in sitr]); b=st.median([fn(f) for f in rest])
    print(hdr % (name,'%.1f'%a,'%.1f'%b,('%.2fx'%(a/b)) if b else 'inf'))
show('SITR (ticks)',lambda f:L[f]['SITR'])
for c in CTR1: show(c,lambda f,c=c:c1(f,c))
for c in CTR:  show(c,lambda f,c=c:c2(f,c))
show('SITR ticks per parse call',lambda f:L[f]['SITR']/max(c1(f,'gNdsR2FtAnimParseCalls'),1))

print('\n=== attach-count populations over all 1,600 frames ===')
byn=defaultdict(list)
for f in allf: byn[min(c2(f,'gNdsFighterNaturalMotionFigatreeAttachCount'),4)].append(f)
print('%6s %6s %11s %11s %10s %9s %9s' % ('attach','cnt','SITR mean','SITR med','WORK-H mean','parse','forceLoad'))
for n in sorted(byn):
    g=byn[n]
    print('%6d %6d %11.0f %11.0f %10.0f %9.1f %9.2f'
          % (n,len(g),st.mean([L[f]['SITR'] for f in g]),st.median([L[f]['SITR'] for f in g]),
             st.mean([ring[f]['WORK-H'] for f in g]),
             st.mean([c1(f,'gNdsR2FtAnimParseCalls') for f in g]),
             st.mean([c1(f,'gNdsR204AnimForceLoadTotal') for f in g])))

print('\n=== SITR cluster rows, attach chain ===')
print('%6s %10s %7s %7s %7s %8s %8s %9s %7s' %
      ('frame','SITR','attach','install','parseC','stepped','earlyOut','cacheB','aliasV'))
for f in sorted(sitr):
    print('%6d %10d %7d %7d %7d %8d %8d %9d %7d' %
          (f,L[f]['SITR'],c2(f,'gNdsFighterNaturalMotionFigatreeAttachCount'),
           c2(f,'gNdsShieldAnimJointInstallCalls'),c1(f,'gNdsR2FtAnimParseCalls'),
           c2(f,'gNdsR2FtAnimParseStepped'),c2(f,'gNdsR2FtAnimParseEarlyOut'),
           c2(f,'gNdsR2AnimCacheBytes'),c2(f,'gNdsR2RelocAliasVisits')))

# ---------------- sub-populations inside the SITR cluster ----------------
print('\n=== SITR cluster sub-populations ===')
def cls(f):
    if c1(f,'gNdsR204AnimForceLoadTotal')>0 and c2(f,'gNdsR2AnimCacheBytes')>0: return 'A card read'
    if c1(f,'gNdsR204AnimForceLoadTotal')>0: return 'B force-load only'
    if c2(f,'gNdsFighterNaturalMotionFigatreeAttachCount')>0: return 'C attach only'
    return 'D no counter moves'
sub=defaultdict(list)
for f in sitr: sub[cls(f)].append(f)
COLS=['SITR','SCPU','SPHD','SHDT','SPRM','GCRARES','MISC','FTR']
print('%-20s %3s %10s %10s %8s %8s %8s' % ('population','n','SITR med','exc med','parse','stepped','attach'))
for k in sorted(sub):
    g=sub[k]
    print('%-20s %3d %10.0f %10.0f %8.0f %8.0f %8.0f'
          % (k,len(g),st.median([L[f]['SITR'] for f in g]),
             st.median([L[f]['SITR']-med['SITR'] for f in g]),
             st.median([c1(f,'gNdsR2FtAnimParseCalls') for f in g]),
             st.median([c2(f,'gNdsR2FtAnimParseStepped') for f in g]),
             st.median([c2(f,'gNdsFighterNaturalMotionFigatreeAttachCount') for f in g])))
print('\nother leaves on each sub-population (median), run median in the last row:')
print('%-20s %3s ' % ('population','n') + ' '.join('%9s'%c for c in COLS))
for k in sorted(sub):
    g=sub[k]
    print('%-20s %3d ' % (k,len(g)) + ' '.join('%9.0f'%st.median([L[f][c] for f in g]) for c in COLS))
print('%-20s %3d ' % ('run median',1600) + ' '.join('%9.0f'%med[c] for c in COLS))
print('\nD population detail (frame, +-3 neighbours WORK-H):')
for f in sub.get('D no counter moves',[]):
    print('  frame %d  SITR %d  ALL %d  WORK-H %d   neighbours WORK-H %s'
          % (f,L[f]['SITR'],ring[f]['ALL'],ring[f]['WORK-H'],
             [ring[g]['WORK-H'] for g in range(f-3,f+4) if g in ring]))
    print('        SITR of neighbours %s' % [L[g]['SITR'] for g in range(f-3,f+4) if g in ring])

# ---------------- how much of the cluster excess is attach-driven ----------------
print('\n=== per-event regression over ALL 1,600 frames ===')
att=[c2(f,'gNdsFighterNaturalMotionFigatreeAttachCount') for f in allf]
stp=[c2(f,'gNdsR2FtAnimParseStepped') for f in allf]
fl =[c1(f,'gNdsR204AnimForceLoadTotal') for f in allf]
for name,x in (('attach',att),('stepped-parse',stp),('force-load',fl)):
    zero=[L[f]['SITR'] for f,v in zip(allf,x) if v==0]
    print('  %-14s frames with 0: n=%4d  SITR median %8.0f   with >0: n=%4d  SITR median %8.0f'
          % (name,len(zero),st.median(zero),
             len(allf)-len(zero),
             st.median([L[f]['SITR'] for f,v in zip(allf,x) if v>0])))
