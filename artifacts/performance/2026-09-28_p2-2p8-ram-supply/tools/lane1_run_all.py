import sys, re, json
import lane1_o2r as o
import lane1_classify as c
import lane1_tiling as tl

STAGES = [('Peach\'s Castle','Castle'),('Sector Z','Sector'),('Kongo Jungle','Jungle'),('Planet Zebes','Zebes'),
          ('Hyrule Castle','Hyrule'),("Yoshi's Island",'Yoster'),('Dream Land','Pupupu'),('Saffron City','Yamabuki'),
          ('Mushroom Kingdom','Inishie')]
txt=open(o.REPO/'decomp/BattleShip-main/include/reloc_data.us.h').read()
def map_id(n):
    m=re.search(r'#define llGR%sMapFileID \(\(intptr_t\)(0x[0-9a-f]+)\)'%n,txt)
    return int(m.group(1),16)

def role_of(f, mapid):
    if f.file_id==mapid: return 'map'
    if f.rel.startswith('reloc_stages/Stage') or 'Wallpaper' in f.rel: return 'wallpaper'
    return 'typed'

def walk(stage_key):
    mid=map_id(stage_key)
    tree=o.tree(mid)
    w=c.Walker(tree)
    w.ground_header(mid,0x14)
    w.finish()
    return mid,tree,w

def compare(tree,w,mid):
    """walker vs tiling: for each byte the walker reached, does the tiling class agree?"""
    out={}
    for f in tree:
        role=role_of(f,mid)
        blocks,_=tl.tile(f.file_id,role)
        tcls=[None]*f.data_size
        for b in blocks:
            for i in range(b.off,b.end): tcls[i]=b.cls
        lay=w.lay[f.file_id]
        agree=0; dis={}
        for i in range(f.data_size):
            wc=c.CN[lay.cls[i]]
            if wc in ('unk','unk0'): continue
            tc=tcls[i]
            ok = (wc==tc) or (wc=='ptrtab' and tc in ('anim','mobj')) or (wc=='hdr' and tc in ('hdr',)) \
                 or (wc=='sprite' and tc=='sprite') or (wc=='pixels' and tc=='pixels') \
                 or (wc=='lmv' and tc in ('anim','gfx')) or (wc=='mobj' and tc in ('mobj',)) \
                 or (wc=='anim' and tc in ('anim','pad','other16')) 
            if ok: agree+=1
            else: dis[(wc,tc)]=dis.get((wc,tc),0)+1
        out[f.file_id]=(agree,dis)
    return out

if __name__=='__main__':
    for label,key in STAGES:
        mid,tree,w=walk(key)
        cmp=compare(tree,w,mid)
        print('==',label,mid)
        for f in tree:
            ag,dis=cmp[f.file_id]
            print('  ',f.file_id,f.rel.split('/')[-1],f.data_size,'walker-reached',ag+sum(dis.values()),'agree',ag,'disagree',dis)
