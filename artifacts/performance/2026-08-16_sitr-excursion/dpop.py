"""The 'D' sub-population: SITR excursion frames with every animation counter at
the run median.  Question: is the excursion SITR-specific work, or a whole-frame
slowdown that SITR merely reports?"""
import csv, statistics as st
RING='artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'
ring={int(r['frame']):{k:int(v) for k,v in r.items()} for r in csv.DictReader(open(RING))}
LEAF=['FTR','STG','BG','AUD','MISC','OTHRW','SRCRES','SWRM','GCRARES',
      'SITR','SCPU','SPHD','SPHC','SCAT','SHDT','SPRM']
def leaves(r):
    six=r['SINT']+r['SPHD']+r['SPHC']+r['SCAT']+r['SHDT']+r['SPRM']
    return dict(zip(LEAF,[r['FTR'],r['STG'],r['BG'],r['AUD'],r['MISC'],r['OTHR']-r['WAIT'],
        r['SRC']-r['GCRA']-r['SWRM'],r['SWRM'],r['GCRA']-six,r['SINT']-r['SCPU'],r['SCPU'],
        r['SPHD'],r['SPHC'],r['SCAT'],r['SHDT'],r['SPRM']]))
L={f:leaves(r) for f,r in ring.items()}
med={n:st.median([L[f][n] for f in L]) for n in LEAF}
hudmed=st.median([ring[f]['HUD'] for f in ring])
print('run median HUD = %d ; frames with HUD > 100,000 = %d'
      % (hudmed,sum(1 for f in ring if ring[f]['HUD']>100000)))
D=[530,989,991,1302]
print('\n%6s %9s %9s %9s %9s %9s %9s %9s %9s' %
      ('frame','ALL','HUD','AUD','SITR','SCPU','SPHD','SHDT','FTR'))
for f in D+[f for f in (447,521,830,1900)]:
    r=ring[f]
    print('%6d %9d %9d %9d %9d %9d %9d %9d %9d' %
          (f,r['ALL'],r['HUD'],r['AUD'],L[f]['SITR'],L[f]['SCPU'],L[f]['SPHD'],L[f]['SHDT'],L[f]['FTR']))
print('%6s %9d %9d %9d %9d %9d %9d %9d %9d' %
      ('median',st.median([ring[f]['ALL'] for f in ring]),hudmed,med['AUD'],
       med['SITR'],med['SCPU'],med['SPHD'],med['SHDT'],med['FTR']))

# ratio profile: on the D frames, how much does EVERY leaf rise?
print('\nD population: median leaf / run median')
for n in LEAF:
    v=st.median([L[f][n] for f in D])
    print('   %-8s %10.0f / %8.0f = %6.2fx' % (n,v,med[n],v/max(med[n],1)))

# is HUD the driver?  correlate HUD with SITR
import math
allf=sorted(L)
def pearson(a,b):
    ma,mb=st.mean(a),st.mean(b)
    num=sum((x-ma)*(y-mb) for x,y in zip(a,b))
    den=math.sqrt(sum((x-ma)**2 for x in a)*sum((y-mb)**2 for y in b))
    return num/den if den else 0.0
S=[L[f]['SITR'] for f in allf]
print('\nr(SITR, HUD) = %+.4f    r(SITR, AUD) = %+.4f    r(SITR, FTR) = %+.4f'
      % (pearson(S,[ring[f]['HUD'] for f in allf]),
         pearson(S,[L[f]['AUD'] for f in allf]),
         pearson(S,[L[f]['FTR'] for f in allf])))
hi=[f for f in allf if ring[f]['HUD']>100000]
print('HUD-refresh frames: n=%d  SITR median %d (run %d)  WORK-H median %d'
      % (len(hi),st.median([L[f]['SITR'] for f in hi]),med['SITR'],
         st.median([ring[f]['WORK-H'] for f in hi])))
print('HUD-refresh frames that are in the SITR cluster: %s'
      % sorted(set(hi)&set([530,989,991,1302,447,521,553,702,830,877,954,1013,1015,1032,
                            1186,1229,1302,1323,1372,1447,1471,1491,1625,1655,1886,1900])))
