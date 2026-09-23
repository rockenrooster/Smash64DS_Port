import csv,collections,sys,bisect,re
NM,CSVF=sys.argv[1],sys.argv[2]
src=open('attrib.py').read().split('sym_cyc = collections.Counter()')[0]
sys.argv=[None,NM,CSVF,'nul']; g={'__name__':'x'}; exec(src,g)
syms=g['syms']; addrs=g['addrs']; classify=g['classify']
size={s[2]:s[1] for s in syms}
fileof={s[2]:s[3] for s in syms}
reg_pcs=collections.defaultdict(set)
reg_syms=collections.defaultdict(set)
cls_cyc=collections.Counter(); cls_ins=collections.Counter()
with open(CSVF,newline='') as fh:
    r=csv.reader(fh); next(r)
    for row in r:
        rg=int(row[0]); pc=int(row[1],16)&~1
        i=bisect.bisect_right(addrs,pc)-1
        n=syms[i][2] if i>=0 else '?'
        reg_syms[rg].add(n)
        reg_pcs[rg].add(pc)
        c=classify(n,fileof.get(n,''))
        cls_cyc[c]+=int(row[5]); cls_ins[c]+=int(row[4])
import statistics
nsym=[len(v) for v in reg_syms.values()]
bytes_=[sum(size.get(n,0) for n in v) for v in reg_syms.values()]
pcs=[len(v) for v in reg_pcs.values()]
print('distinct functions per frame: median',statistics.median(nsym),'max',max(nsym))
print('code bytes of functions touched per frame: median',statistics.median(bytes_),'max',max(bytes_))
print('distinct PCs (instructions) per frame: median',statistics.median(pcs),' => ~bytes (thumb 2B avg ~2.4)', statistics.median(pcs)*2.4)
print()
for c,v in sorted(cls_cyc.items(), key=lambda x:-x[1])[:22]:
    print('%-16s CPI %.2f'%(c, v/cls_ins[c] if cls_ins[c] else 0))
