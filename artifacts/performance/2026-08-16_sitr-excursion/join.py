"""Join the per-frame counter run to the basis ring buckets and profile the SITR cluster.

RING buckets: ../2026-08-16_camera-ship/ship220-rows.csv  (build-c220-camship, -RingDump,
              wrap-corrected live, closure exact -- see overgate.py).
COUNTERS:     pf220-rows.csv, same ROM, -PerFrameGlobals.  Its BUCKETS are torn
              (IO_AUDIT.md section 4: ALL from iteration f, the rest from f+1) and are NOT
              used here; only its counter columns are.  The frame offset between the two
              instruments is MEASURED below, not assumed.
"""
import csv, statistics as st
from collections import defaultdict

RING='artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'
PF='artifacts/performance/2026-08-16_sitr-excursion/pf220-rows.csv'
CTR=['gNdsR204AnimForceLoadTotal','gNdsRelocAssetPayloadReadCount','gNdsR2FtAnimParseCalls',
     'gNdsR2FtAnimNullSkips','gNdsR2AnimCacheHits','gNdsBattlePackHits','gNdsR2CubicEvals',
     'gNdsRelocFindMemoScans']

ring={int(r['frame']):{k:int(v) for k,v in r.items()} for r in csv.DictReader(open(RING))}
pf=[{k:int(v) for k,v in r.items()} for r in csv.DictReader(open(PF))]
pf.sort(key=lambda r:r['frame'])
# per-frame DELTA of each cumulative counter (events that happened in that frame)
delta={}
for i in range(1,len(pf)):
    delta[pf[i]['frame']]={c: pf[i][c]-pf[i-1][c] for c in CTR}
print('pf frames %d..%d   ring frames %d..%d' % (pf[0]['frame'],pf[-1]['frame'],
      min(ring),max(ring)))
print('counter totals over the pf window:',
      {c: pf[-1][c]-pf[0][c] for c in CTR})

LEAF=['FTR','STG','BG','AUD','MISC','OTHRW','SRCRES','SWRM','GCRARES',
      'SITR','SCPU','SPHD','SPHC','SCAT','SHDT','SPRM']
def leaves(r):
    six=r['SINT']+r['SPHD']+r['SPHC']+r['SCAT']+r['SHDT']+r['SPRM']
    return dict(zip(LEAF,[r['FTR'],r['STG'],r['BG'],r['AUD'],r['MISC'],r['OTHR']-r['WAIT'],
        r['SRC']-r['GCRA']-r['SWRM'],r['SWRM'],r['GCRA']-six,r['SINT']-r['SCPU'],r['SCPU'],
        r['SPHD'],r['SPHC'],r['SCAT'],r['SHDT'],r['SPRM']]))
L={f:leaves(r) for f,r in ring.items()}
med={n: st.median([L[f][n] for f in L]) for n in LEAF}

# --- measure the offset: which shift maximises the SITR association of a force-load?
print('\noffset scan: mean SITR on frames whose counter-delta shows a force-load')
base=st.mean([L[f]['SITR'] for f in L])
for off in (-1,0,1,2):
    fl=[f+off for f in delta if delta[f]['gNdsR204AnimForceLoadTotal']>0 and (f+off) in L]
    if not fl: continue
    print('  offset %+d : n=%3d  mean SITR %9.0f   lift %+9.0f' %
          (off,len(fl),st.mean([L[f]['SITR'] for f in fl]),
           st.mean([L[f]['SITR'] for f in fl])-base))
print('  (all frames mean SITR %.0f)'%base)

OFF=1   # measured above; counters for ring frame f come from pf row f-1
def ctr(f,c): return delta.get(f-OFF,{}).get(c,0)

# ---------- the SITR cluster on the current basis ----------
order=sorted(ring.values(), key=lambda r:-r['WORK-H'])
top=[r['frame'] for r in order[:80]]
def owner(f):
    return max(((L[f][n]-med[n],n) for n in LEAF))[1]
sitr=[f for f in top if owner(f)=='SITR']
shdt=[f for f in top if owner(f)=='SHDT']
print('\n=== SITR cluster: %d frames ===' % len(sitr))
print('%6s %10s %10s %9s %6s %6s %8s %8s %7s %7s %7s' %
      ('frame','WORK-H','SITR','exc','FL','card','parse','nullskip','cubic','packHit','cacheHit'))
for f in sorted(sitr):
    print('%6d %10d %10d %9d %6d %6d %8d %8d %7d %7d %7d' %
          (f,ring[f]['WORK-H'],L[f]['SITR'],L[f]['SITR']-med['SITR'],
           ctr(f,'gNdsR204AnimForceLoadTotal'),ctr(f,'gNdsRelocAssetPayloadReadCount'),
           ctr(f,'gNdsR2FtAnimParseCalls'),ctr(f,'gNdsR2FtAnimNullSkips'),
           ctr(f,'gNdsR2CubicEvals'),ctr(f,'gNdsBattlePackHits'),ctr(f,'gNdsR2AnimCacheHits')))

print('\n--- run medians of the same columns over all 1,600 ring frames ---')
allf=sorted(L)
for c in CTR:
    v=[ctr(f,c) for f in allf]
    print('  %-34s median %8.1f   mean %9.2f   max %8d' % (c,st.median(v),st.mean(v),max(v)))

# ---------- split the cluster by force-load ----------
withfl=[f for f in sitr if ctr(f,'gNdsR204AnimForceLoadTotal')>0]
nofl=[f for f in sitr if ctr(f,'gNdsR204AnimForceLoadTotal')==0]
print('\nSITR cluster split: %d carry a force-load, %d do not' % (len(withfl),len(nofl)))
for name,g in (('force-load',withfl),('no force-load',nofl)):
    if not g: continue
    print('  %-14s n=%2d  median SITR %9.0f  median exc %9.0f  median parse %6.0f  median cubic %7.0f'
          % (name,len(g),st.median([L[f]['SITR'] for f in g]),
             st.median([L[f]['SITR']-med['SITR'] for f in g]),
             st.median([ctr(f,'gNdsR2FtAnimParseCalls') for f in g]),
             st.median([ctr(f,'gNdsR2CubicEvals') for f in g])))
rest=[f for f in allf if f not in set(sitr)]
print('  %-14s n=%2d  median SITR %9.0f  median exc %9.0f  median parse %6.0f  median cubic %7.0f'
      % ('all others',len(rest),st.median([L[f]['SITR'] for f in rest]),
         st.median([L[f]['SITR']-med['SITR'] for f in rest]),
         st.median([ctr(f,'gNdsR2FtAnimParseCalls') for f in rest]),
         st.median([ctr(f,'gNdsR2CubicEvals') for f in rest])))

# ---------- population table over ALL frames by force-load count ----------
print('\n=== all 1,600 ring frames, grouped by force-loads on the frame ===')
print('%3s %5s %10s %10s %9s %8s %8s %8s %8s' %
      ('n','cnt','WORK-H mean','SITR mean','SITR med','parse','nullskip','cubic','SPHD mean'))
byn=defaultdict(list)
for f in allf: byn[min(ctr(f,'gNdsR204AnimForceLoadTotal'),3)].append(f)
for n in sorted(byn):
    g=byn[n]
    print('%3d %5d %10.0f %10.0f %9.0f %8.1f %8.1f %8.1f %8.0f' %
          (n,len(g),st.mean([ring[f]['WORK-H'] for f in g]),st.mean([L[f]['SITR'] for f in g]),
           st.median([L[f]['SITR'] for f in g]),
           st.mean([ctr(f,'gNdsR2FtAnimParseCalls') for f in g]),
           st.mean([ctr(f,'gNdsR2FtAnimNullSkips') for f in g]),
           st.mean([ctr(f,'gNdsR2CubicEvals') for f in g]),
           st.mean([L[f]['SPHD'] for f in g])))

# ---------- does SITR track parse calls at all? ----------
import math
def pearson(a,b):
    ma,mb=st.mean(a),st.mean(b)
    num=sum((x-ma)*(y-mb) for x,y in zip(a,b))
    den=math.sqrt(sum((x-ma)**2 for x in a)*sum((y-mb)**2 for y in b))
    return num/den if den else 0.0
S=[L[f]['SITR'] for f in allf]
print('\nPearson r against per-frame SITR, all 1,600 frames:')
for c in CTR:
    print('   %-34s r=%+.4f' % (c,pearson(S,[ctr(f,c) for f in allf])))
