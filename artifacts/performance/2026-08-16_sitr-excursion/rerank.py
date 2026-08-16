"""Exact re-rank of the basis's own 1,600 rows, by SITR sub-population.

Ceilings, not implementations: each row sets the named frames' SITR to the run
median and re-sorts WORK-H.  Basis build-c220-camship, apparatus 24,947,
gate 1,120,380.
"""
import csv, statistics as st
from collections import defaultdict
RING='artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'
PF='artifacts/performance/2026-08-16_sitr-excursion/pf220-rows.csv'
PFB='artifacts/performance/2026-08-16_sitr-excursion/pf220b-rows.csv'
ring={int(r['frame']):{k:int(v) for k,v in r.items()} for r in csv.DictReader(open(RING))}
def deltas(path,cols):
    rows=sorted(({k:int(v) for k,v in r.items()} for r in csv.DictReader(open(path))),key=lambda r:r['frame'])
    return {rows[i]['frame']:{c:rows[i][c]-rows[i-1][c] for c in cols} for i in range(1,len(rows))}
d1=deltas(PF,['gNdsR204AnimForceLoadTotal','gNdsR2FtAnimParseCalls'])
d2=deltas(PFB,['gNdsFighterNaturalMotionFigatreeAttachCount','gNdsR2AnimCacheBytes'])
LEAF=['FTR','STG','BG','AUD','MISC','OTHRW','SRCRES','SWRM','GCRARES',
      'SITR','SCPU','SPHD','SPHC','SCAT','SHDT','SPRM']
def leaves(r):
    six=r['SINT']+r['SPHD']+r['SPHC']+r['SCAT']+r['SHDT']+r['SPRM']
    return dict(zip(LEAF,[r['FTR'],r['STG'],r['BG'],r['AUD'],r['MISC'],r['OTHR']-r['WAIT'],
        r['SRC']-r['GCRA']-r['SWRM'],r['SWRM'],r['GCRA']-six,r['SINT']-r['SCPU'],r['SCPU'],
        r['SPHD'],r['SPHC'],r['SCAT'],r['SHDT'],r['SPRM']]))
L={f:leaves(r) for f,r in ring.items()}
med={n:st.median([L[f][n] for f in L]) for n in LEAF}
allf=sorted(L); OFF=1
def c(f,k):
    return d1.get(f-OFF,{}).get(k, d2.get(f-OFF,{}).get(k,0))
order=sorted(ring.values(),key=lambda r:-r['WORK-H'])
top=[r['frame'] for r in order[:80]]
def owner(f): return max(((L[f][n]-med[n],n) for n in LEAF))[1]
sitr=[f for f in top if owner(f)=='SITR']
def cls(f):
    if c(f,'gNdsR204AnimForceLoadTotal')>0 and c(f,'gNdsR2AnimCacheBytes')>0: return 'A card read'
    if c(f,'gNdsR204AnimForceLoadTotal')>0: return 'B force-load only'
    if c(f,'gNdsFighterNaturalMotionFigatreeAttachCount')>0: return 'C attach only'
    return 'D no counter moves'
sub=defaultdict(list)
for f in sitr: sub[cls(f)].append(f)
base=sorted((r['WORK-H'] for r in ring.values()),reverse=True)[79]
def rerank(frames):
    ser=[]
    for f in allf:
        w=ring[f]['WORK-H']
        if f in frames: w-=max(0,L[f]['SITR']-med['SITR'])
        ser.append(w)
    ser.sort(reverse=True); return ser[79]
print('basis rank-80 %d  net %d  level %+d' % (base,base-24947,base-24947-1120380))
print('\n%-24s %4s %12s %9s %10s' % ('SITR sub-population','n','rank-80','moved','new level'))
for k in sorted(sub):
    r=rerank(set(sub[k]))
    print('%-24s %4d %12d %9d %+10d' % (k,len(sub[k]),r,base-r,r-24947-1120380))
r=rerank(set(sitr))
print('%-24s %4d %12d %9d %+10d' % ('ALL 25',len(sitr),r,base-r,r-24947-1120380))
# and: every frame in the run that carries an attach or a force-load, not just the top-80
evt=set(f for f in allf if c(f,'gNdsFighterNaturalMotionFigatreeAttachCount')>0
        or c(f,'gNdsR204AnimForceLoadTotal')>0)
r=rerank(evt)
print('%-24s %4d %12d %9d %+10d' % ('every attach/FL frame',len(evt),r,base-r,r-24947-1120380))
allfr=set(allf); r=rerank(allfr)
print('%-24s %4d %12d %9d %+10d' % ('SITR->median everywhere',len(allfr),r,base-r,r-24947-1120380))
