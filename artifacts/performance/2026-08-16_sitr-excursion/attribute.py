"""Per-symbol attribution of the SITR excursion, from the multi-mask reduce.

For every symbol: cost per frame on the SITR-25 mask, on the 288 attach/force-load
frames, on the 773 quiet frames, and whole match; plus the SITR-25 - quiet delta,
which is what an excursion frame executes that an ordinary frame does not.

UNITS: 2 profile cycles = 1 project tick.  Masks and their sizes come from
c221-multimask.meta.txt; the alignment is align.py's.
"""
import csv, sys
D='artifacts/performance/2026-08-16_sitr-excursion/'
N={'whole':1601,'sitr25':25,'event288':288,'quiet':773,'card':7}
syms=[]
for line in open(D+'c221-nm.txt'):
    p=line.split()
    if len(p)==4 and p[2] in 'tTwW':
        a=int(p[0],16); s=int(p[1],16)
        if s: syms.append((a,a+s,p[3]))
syms.sort()
starts=[s[0] for s in syms]
import bisect
def owner(pc):
    i=bisect.bisect_right(starts,pc)-1
    if i>=0 and pc<syms[i][1]: return syms[i][2]
    return '<unattributed 0x%08x>'%(pc & ~0xfff)
COLS=['instructions','total_cycles','issue','icache_fill','dcache_fill',
      'write_buffer','bus_contention','dma_hold','interlock','halt_wait']
MASKS=['whole','sitr25','event288','quiet','card']
agg={}
rd=csv.DictReader(open(D+'c221-multimask.csv'))
for r in rd:
    pc=int(r['pc'],16)
    o=owner(pc)
    a=agg.get(o)
    if a is None: a=agg[o]={m:{c:0 for c in COLS} for m in MASKS}
    for m in MASKS:
        for c in COLS: a[m][c]+=int(r['%s_%s'%(m,c)])
def tk(o,m,c='total_cycles'): return agg[o][m][c]/(2.0*N[m])
rows=[(tk(o,'sitr25')-tk(o,'quiet'),o) for o in agg]
rows.sort(reverse=True)
print('SYMBOLS RANKED BY (SITR-25 tk/fr) - (QUIET tk/fr)')
print('masks: sitr25=25 frames, quiet=773 frames, event288=288, whole=1601; 2 cyc = 1 tick')
print()
hdr='%-56s %10s %10s %10s %10s %9s %9s'
print(hdr%('symbol','sitr25','quiet','DELTA','event288','whole','icache d'))
tot=0
for d,o in rows[:45]:
    print(hdr%(o[:56],'%.0f'%tk(o,'sitr25'),'%.0f'%tk(o,'quiet'),'%.0f'%d,
               '%.0f'%tk(o,'event288'),'%.0f'%tk(o,'whole'),
               '%.0f'%(tk(o,'sitr25','icache_fill')-tk(o,'quiet','icache_fill'))))
print()
print('SUM over all symbols  sitr25 %.0f  quiet %.0f  delta %.0f (non-idle: minus armWaitForIrq)'
      % (sum(tk(o,'sitr25') for o in agg),sum(tk(o,'quiet') for o in agg),
         sum(tk(o,'sitr25')-tk(o,'quiet') for o in agg)))
idle='armWaitForIrq'
if idle in agg:
    print('   armWaitForIrq  sitr25 %.0f  quiet %.0f' % (tk(idle,'sitr25'),tk(idle,'quiet')))
print()
print('NEGATIVE DELTAS (cheaper on an excursion frame):')
for d,o in rows[-8:]:
    print(hdr%(o[:56],'%.0f'%tk(o,'sitr25'),'%.0f'%tk(o,'quiet'),'%.0f'%d,
               '%.0f'%tk(o,'event288'),'%.0f'%tk(o,'whole'),''))
print()
print('STALL SHAPE of the delta, top 12 symbols:')
print('%-52s %9s %9s %9s %9s %9s'%('symbol','issue','icache','dcache','bus+dma','interlock'))
for d,o in rows[:12]:
    f=lambda c: (agg[o]['sitr25'][c]/(2.0*25))-(agg[o]['quiet'][c]/(2.0*773))
    print('%-52s %9.0f %9.0f %9.0f %9.0f %9.0f'%(o[:52],f('issue'),f('icache_fill'),
          f('dcache_fill'),f('bus_contention')+f('dma_hold'),f('interlock')))
