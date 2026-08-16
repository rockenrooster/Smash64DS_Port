"""One pass over the v3 capture that reduces to FOUR masks at once.

Masks are region id sets carried from the tick-HUD basis run at the measured
alignment `region = frame - 439` (align.py scores every offset; the card-read
anchor lands at median profile rank 12 there and 315-699 everywhere else).

  whole      all 1,601 regions
  sitr25     the 25 over-gate frames the SITR leaf owns  (overgate.py)
  event288   every frame carrying a figatree attach or a force-load
  quiet      below-median-WORK-H frames carrying neither

Emits one row per PC with <mask>_<column> for each, so a single scan answers
"what does the SITR excursion execute that a quiet frame does not".
"""
import csv, sys, os
D='artifacts/performance/2026-08-16_sitr-excursion/'
PROF=D+'v3-c221/arm9-profile.csv'
COLS=['instructions','total_cycles','issue','icache_fill','dcache_fill',
      'write_buffer','bus_contention','dma_hold','interlock','halt_wait']
MASKS=['whole','sitr25','event288','quiet','card']
sets={}
for m in MASKS[1:]:
    sets[m]=set(int(x) for x in open(D+'regions-%s.txt'%m) if x.strip())
with open(PROF,newline='') as fh:
    head=fh.readline().rstrip('\n').split(',')
idx={c:head.index(c) for c in COLS}
ri=head.index('region'); pi=head.index('pc')
acc={}
n=0
with open(PROF,'rb') as fh:
    fh.readline()
    for line in fh:
        f=line.split(b',')
        n+=1
        reg=int(f[ri]); pc=f[pi]
        rec=acc.get(pc)
        if rec is None: rec=acc[pc]=[0]*(len(MASKS)*len(COLS))
        vals=[int(f[idx[c]]) for c in COLS]
        for j,v in enumerate(vals): rec[j]+=v          # whole
        for k,m in enumerate(MASKS[1:],start=1):
            if reg in sets[m]:
                base=k*len(COLS)
                for j,v in enumerate(vals): rec[base+j]+=v
print('rows=%d  distinct_pcs=%d' % (n,len(acc)), file=sys.stderr)
out=open(D+'c221-multimask.csv','w',newline='')
w=csv.writer(out)
w.writerow(['pc']+['%s_%s'%(m,c) for m in MASKS for c in COLS])
for pc,rec in acc.items():
    w.writerow([pc.decode()]+rec)
out.close()
open(D+'c221-multimask.meta.txt','w').write(
 'source=%s\nrows=%d\ndistinct_pcs=%d\nregions=1601\n'
 'alignment=region = frame - 439 (align.py: card anchor median profile rank 12)\n'
 'mask sizes: whole=1601 sitr25=%d event288=%d quiet=%d card=%d\n'
 'cycles_per_tick=2\n'
 % (PROF,n,len(acc),len(sets['sitr25']),len(sets['event288']),len(sets['quiet']),len(sets['card'])))
