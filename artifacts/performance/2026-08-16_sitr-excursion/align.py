"""Measure the profile-region <-> tick-HUD-frame offset, using the card reads as anchor.

The harness banner prints 'region r = presented frame 438 + r'.
SITR_DIRECT_CHILDREN.md section 7.1 measured that banner to be off by one on its
capture.  Rather than inherit either claim, this scores every offset in -4..+4
against two independent anchors of the c220 basis run:
  (a) the 7 card-read frames (pf220 counter deltas), which must be extreme; and
  (b) the 80 largest WORK-H frames, whose profile ranks must be small.
"""
import csv, statistics as st
D='artifacts/performance/2026-08-16_sitr-excursion/'
reg={}
for r in csv.DictReader(open(D+'v3-c221/arm9-profile.regions.csv')):
    reg[int(r['region'])]=int(r['total_cycles'])-int(r['halt_wait'])
order=sorted(reg,key=lambda k:-reg[k])
rank={k:i+1 for i,k in enumerate(order)}
print('profile regions=%d  non-idle median %d  max %d  rank-80 %d'
      % (len(reg),st.median(list(reg.values())),max(reg.values()),reg[order[79]]))
card=[int(x) for x in open(D+'frames-card.txt')]
ring={int(r['frame']):int(r['WORK-H']) for r in csv.DictReader(
    open('artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'))}
top80=set(sorted(ring,key=lambda f:-ring[f])[:80])
print('\n%6s %10s %10s %12s %10s' % ('offset','card med','card mean','top80 medrk','top80 in profile top-80'))
best=None
for off in range(434,445):
    cr=[rank.get(f-off) for f in card if (f-off) in rank]
    tr=[rank.get(f-off) for f in top80 if (f-off) in rank]
    if len(cr)!=len(card) or len(tr)!=len(top80): continue
    row=(st.median(cr),st.mean(cr),st.median(tr),sum(1 for r in tr if r<=80))
    print('%6d %10.1f %10.1f %12.1f %10d' % (off,row[0],row[1],row[2],row[3]))
    if best is None or row[0]<best[1][0]: best=(off,row)
print('\nbest offset = %+d  (region = frame - %d)' % (best[0],best[0]))
OFF=best[0]
print('\ncard-read frames at that offset:')
for f in card:
    print('   frame %4d -> region %4d   non-idle %9d   profile rank %4d   ring WORK-H %9d'
          % (f,f-OFF,reg[f-OFF],rank[f-OFF],ring[f]))
# write region lists at the measured offset
for name in ('sitr25','event288','card','quiet'):
    fr=[int(x) for x in open(D+'frames-%s.txt'%name)]
    open(D+'regions-%s.txt'%name,'w').write('\n'.join(str(f-OFF) for f in fr if (f-OFF) in reg)+'\n')
    print('regions-%s.txt : %d of %d frames map into the profile' % (name,sum(1 for f in fr if (f-OFF) in reg),len(fr)))
