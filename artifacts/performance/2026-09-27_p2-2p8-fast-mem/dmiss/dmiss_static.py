import csv,bisect,subprocess,collections,sys
elf=sys.argv[1]; dm=sys.argv[2]; frames=int(sys.argv[3])
objs=[]
for l in subprocess.run(['C:/devkitPro/devkitARM/bin/arm-none-eabi-nm','-S','-n',elf],capture_output=True,text=True).stdout.split('\n'):
    p=l.split()
    if len(p)==4 and p[2] in 'bBdDrRvV' and int(p[1],16)>0:
        objs.append((int(p[0],16),int(p[1],16),p[3],p[2]))
objs.sort(); oa=[o[0] for o in objs]
c=collections.Counter(); heap=0; tot=0
for r in csv.DictReader(open(dm)):
    line=int(r['line'],16); n=int(r['misses']); tot+=n
    # objects overlapping [line, line+32): start < line+32 and end > line
    i=bisect.bisect_left(oa,line+32)-1
    best=None
    while i>=0 and objs[i][0] + 0x40000 > line:
        o=objs[i]
        if o[0]+o[1] > line and o[0] < line+32:
            ov=min(o[0]+o[1],line+32)-max(o[0],line)
            if best is None or ov>best[0]: best=(ov,i)
        if o[0] < line - 0x4000: break
        i-=1
    if best: c[best[1]]+=n
    else: heap+=n
print('total %.0f/fr, static %.0f/fr, other %.0f/fr' % (tot/frames, sum(c.values())/frames, heap/frames))
rows=[(v/frames, objs[k][1], objs[k][2], objs[k][3], hex(objs[k][0])) for k,v in c.items()]
mode=sys.argv[4] if len(sys.argv)>4 else 'perkb'
rows.sort(key=(lambda r:-r[0]/max(r[1],32)) if mode=='perkb' else (lambda r:-r[0]))
cum=0
for f,s,n,t,a in rows[:60]:
    cum+=f
    print(f'{f:8.1f} {s:6d} {f/max(s,32)*1024:9.1f} cum{cum:8.0f}  {t} {n} {a}')
