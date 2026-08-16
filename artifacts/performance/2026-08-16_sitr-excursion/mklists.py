"""Emit the frame lists this cycle's reduces need, from the basis rows + counters."""
import csv, statistics as st, sys
from collections import defaultdict
D='artifacts/performance/2026-08-16_sitr-excursion/'
ring={int(r['frame']):{k:int(v) for k,v in r.items()} for r in csv.DictReader(
    open('artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'))}
def deltas(path,cols):
    rows=sorted(({k:int(v) for k,v in r.items()} for r in csv.DictReader(open(path))),key=lambda r:r['frame'])
    return {rows[i]['frame']:{c:rows[i][c]-rows[i-1][c] for c in cols} for i in range(1,len(rows))}
d1=deltas(D+'pf220-rows.csv',['gNdsR204AnimForceLoadTotal','gNdsRelocAssetPayloadReadCount'])
d2=deltas(D+'pf220b-rows.csv',['gNdsFighterNaturalMotionFigatreeAttachCount'])
LEAF=['FTR','STG','BG','AUD','MISC','OTHRW','SRCRES','SWRM','GCRARES',
      'SITR','SCPU','SPHD','SPHC','SCAT','SHDT','SPRM']
def leaves(r):
    six=r['SINT']+r['SPHD']+r['SPHC']+r['SCAT']+r['SHDT']+r['SPRM']
    return dict(zip(LEAF,[r['FTR'],r['STG'],r['BG'],r['AUD'],r['MISC'],r['OTHR']-r['WAIT'],
        r['SRC']-r['GCRA']-r['SWRM'],r['SWRM'],r['GCRA']-six,r['SINT']-r['SCPU'],r['SCPU'],
        r['SPHD'],r['SPHC'],r['SCAT'],r['SHDT'],r['SPRM']]))
L={f:leaves(r) for f,r in ring.items()}
med={n:st.median([L[f][n] for f in L]) for n in LEAF}
order=sorted(ring.values(),key=lambda r:-r['WORK-H'])
top=[r['frame'] for r in order[:80]]
sitr=[f for f in top if max(((L[f][n]-med[n],n) for n in LEAF))[1]=='SITR']
OFF=1
card=[f for f in sorted(L) if d1.get(f-OFF,{}).get('gNdsRelocAssetPayloadReadCount',0)>0]
evt=[f for f in sorted(L)
     if d1.get(f-OFF,{}).get('gNdsR204AnimForceLoadTotal',0)>0
     or d2.get(f-OFF,{}).get('gNdsFighterNaturalMotionFigatreeAttachCount',0)>0]
quiet=[f for f in sorted(L)
       if f not in set(evt) and ring[f]['WORK-H']<st.median([ring[g]['WORK-H'] for g in L])]
OFFSET=int(sys.argv[1]) if len(sys.argv)>1 else 439
for name,fr in (('sitr25',sitr),('card',card),('event288',evt),('quiet',quiet)):
    open(D+'frames-%s.txt'%name,'w').write('\n'.join(str(f) for f in sorted(fr))+'\n')
    open(D+'regions-%s.txt'%name,'w').write('\n'.join(str(f-OFFSET) for f in sorted(fr))+'\n')
    print('%-10s n=%4d  frames %s%s' % (name,len(fr),sorted(fr)[:8],' ...' if len(fr)>8 else ''))
print('region files written with region = frame - %d (to be VERIFIED against the card anchor)'%OFFSET)
