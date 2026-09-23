import bisect, csv, collections, json, re, sys, statistics
sys.path.insert(0,'.')
import importlib.util
spec=importlib.util.spec_from_file_location('a','attrib.py')
NM, CSV = sys.argv[1], sys.argv[2]
# reuse symbol loading from attrib.py by exec of its top part
src=open('attrib.py').read().split('sym_cyc = collections.Counter()')[0]
sys.argv=[None,NM,CSV,'nul']; g={'__name__':'x'}
exec(src,g)
lookup=g['lookup']; classify=g['classify']
rs=collections.defaultdict(collections.Counter)
cache={}
with open(CSV,newline='') as fh:
    r=csv.reader(fh); next(r)
    for row in r:
        rg=int(row[0]); pc=int(row[1],16)&~1; cyc=int(row[5])
        k=cache.get(pc)
        if k is None:
            res=lookup(pc); k=res[0] if res else '?'; cache[pc]=k
        rs[rg][k]+=cyc
regions=sorted(rs)
R=len(regions)
work={rg:(sum(rs[rg].values())-rs[rg]['armWaitForIrq'])/2 for rg in regions}
ws=sorted(regions,key=lambda x:work[x])
lo=ws[int(R*0.4):int(R*0.6)]; hi=ws[int(R*0.9):]
allsyms=set()
for rg in regions: allsyms|=set(rs[rg])
rows=[]
for s in allsyms:
    m=sum(rs[rg][s] for rg in lo)/len(lo)/2
    t=sum(rs[rg][s] for rg in hi)/len(hi)/2
    rows.append((t-m,s,m,t))
rows.sort(reverse=True)
print('tail regions',len(hi),'work mean tail %.0f med %.0f'%(statistics.mean(work[x] for x in hi),statistics.mean(work[x] for x in lo)))
for d,s,m,t in rows[:45]:
    print('%9.0f  med %8.0f tail %8.0f  %s'%(d,m,t,s))
json.dump({'work':work},open('work_by_region.json','w'))
