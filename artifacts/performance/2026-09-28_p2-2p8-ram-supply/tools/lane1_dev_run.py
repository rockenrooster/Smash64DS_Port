import sys, re
import lane1_o2r as o
import lane1_classify as c

idx=o.index()
txt=open(o.REPO/'decomp/BattleShip-main/include/reloc_data.us.h').read()
def map_id(n):
    m=re.search(r'#define llGR%sMapFileID \(\(intptr_t\)(0x[0-9a-f]+)\)'%n,txt)
    return int(m.group(1),16)

def run(stage):
    fid=map_id(stage)
    tree=o.tree(fid)
    w=c.Walker(tree)
    nodes=w.ground_header(fid,0x14)
    return w,tree,nodes

if __name__=='__main__':
    stage=sys.argv[1]
    w,tree,nodes=run(stage)
    print('map_nodes', nodes)
    w.finish()
    for f in tree:
        lay=w.lay[f.file_id]
        cnt=lay.counts()
        print(f.file_id,f.rel,f.data_size, {k:v for k,v in sorted(cnt.items())}, 'conflicts',lay.conflicts, dict(lay.conflict_detail))
    print(w.notes[:20])
