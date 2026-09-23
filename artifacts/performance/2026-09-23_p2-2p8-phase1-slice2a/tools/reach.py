"""P2-2p8 Phase 1 slice 2a: per-kind reachable fighter DS texture set, enumerated
WITHOUT drawing, from what fighter creation loads (the relocData closure the
pack estimator indexes: display lists, MObjSub texture tables, palettes).

Two sources of keys:
  DL   every Gfx array in the closure is interpreted (SETTIMG / SETTILE /
       LOADBLOCK / LOADTILE / LOADTLUT / SETTILESIZE / TEXTURE / DL / ENDDL);
       each textured triangle emits (image, tlut, render tile) and the port's
       DS dimension rules (nds_renderer_textures_effects.c: source extent,
       masked-clamp materialisation, clamped window period, wrap period,
       next pow2, 128 max) give the DS texture.
  MObj every MObjSub's sprite x palette table (texture animation, costume
       palettes) -- the image comes from the MObj's branch DL, the tile from
       the DObj's DL; dims from the MObjSub tile size (unk0C x unk0E) unless
       a DL template with the same load size exists.
Validated against the runtime keys of a census-b run (image, tlut, DS dims).
"""
import collections, json, os, struct, sys

ROOT = r'D:\Stuff\DevFolder\Smash64DS_Port'
ART = os.path.join(ROOT, r'artifacts\performance\2026-09-23_p2-2p8-phase1-slice2a')
PACKS = os.path.join(ROOT, r'builds\build-p2-fourcpu-tickhud\battle-core')
sys.path.insert(0, os.path.join(ROOT, 'scripts', 'fighters'))
import estimate_fighter_pack as est  # noqa: E402
import generate_preview_core_packs as fpc  # noqa: E402

BE2 = struct.Struct('>II')
MAXD = 128
CLAMP = 2
FMT = ['RGBA', 'YUV', 'CI', 'IA', 'I']


def next_pow2(v):
    p = 8
    while p < v:
        p <<= 1
    return p


def source_bytes(fmt, siz, texels):
    if fmt == 2:
        return (texels + 1) // 2 if siz == 0 else (texels if siz == 1 else 0)
    if fmt == 0:
        return texels * 2 if siz == 2 else (texels * 4 if siz == 3 else 0)
    # IA / I
    return (texels + 1) // 2 if siz == 0 else (texels if siz == 1 else texels * 2)


def line_pixels(siz, line):
    return line * (16, 8, 4, 2)[siz]


def ds_dims(fmt, siz, line, tile_w, tile_h, load16, cms, cmt, masks, maskt):
    loaded = load16 * (4 if siz == 3 else 2)
    w, h = tile_w, tile_h
    if (not w or not h or w > MAXD or h > MAXD or source_bytes(fmt, siz, w * h) > loaded):
        w = line_pixels(siz, line)
        tex = load16 * 2
        if siz == 0:
            tex *= 2
        elif siz in (2, 3):
            tex //= 2
        h = tex // w if w else 0
    if not w or not h or w > MAXD or h > MAXD:
        return None
    sw, sh = w, h

    def mat(mode, mask, src, tile):
        if not (mode & CLAMP) or mask == 0 or mask >= 31 or src == 0 or tile > MAXD:
            return False
        me = 1 << mask
        return tile > me and src >= me and src <= tile
    ms = mat(cms, masks, sw, tile_w)
    mt = mat(cmt, maskt, sh, tile_h)
    if ms:
        w = tile_w
    if mt:
        h = tile_h
    if not ms and (cms & CLAMP) and 3 <= masks < 31 and tile_w > MAXD and (1 << masks) <= sw:
        w = 1 << masks
    if not mt and (cmt & CLAMP) and 3 <= maskt < 31 and tile_h > MAXD and (1 << maskt) <= sh:
        h = 1 << maskt
    if not (cms & CLAMP) and 3 <= masks < 31 and (1 << masks) < w:
        w = 1 << masks
    if not (cmt & CLAMP) and 3 <= maskt < 31 and (1 << maskt) < h:
        h = 1 << maskt
    return next_pow2(w), next_pow2(h), sw, sh


class Kind:
    def __init__(self, name, idx):
        self.name = name
        self.idx = idx
        self.files = {f.file_id: f for f in idx.files}
        self.objects = {}
        for f in idx.files:
            for o in f.objects:
                self.objects.setdefault(f.file_id, []).append(o)
        for v in self.objects.values():
            v.sort(key=lambda o: o.offset)

    def obj_at(self, fid, off):
        for o in self.objects.get(fid, ()):
            if o.offset <= off < o.offset + max(o.size, 1):
                return o
        return None

    def u32(self, fid, off):
        return struct.unpack_from('>I', self.files[fid].source['payload'], off)[0]

    def u16(self, fid, off):
        return struct.unpack_from('>H', self.files[fid].source['payload'], off)[0]

    def ptr(self, fid, slot):
        return self.files[fid].source['pointers'].get(slot)


def unique_values(kind, img, fmt, siz, w, h):
    """Distinct source texel values over w x h (upper bound on DS colours)."""
    if img is None:
        return 999
    fid, off = img
    data = kind.files[fid].source['payload']
    n = w * h
    o = kind.obj_at(fid, off)
    if o is not None:
        avail = o.offset + o.size - off
        per = {0: 0.5, 1: 1, 2: 2, 3: 4}[siz]
        n = min(n, int(avail / per))
    vals = set()
    if siz == 0:
        for i in range((n + 1) // 2):
            if off + i >= len(data):
                break
            b = data[off + i]
            vals.add(b >> 4)
            vals.add(b & 15)
    elif siz == 1:
        vals = set(data[off:off + n])
    elif siz == 2:
        vals = set(struct.unpack_from('>%dH' % min(n, (len(data) - off) // 2), data, off))
    else:
        vals = set(struct.unpack_from('>%dI' % min(n, (len(data) - off) // 4), data, off))
    return len(vals)


def ds_format(kind, img, fmt, siz, sw, sh):
    """Pal16 when the converted texture has <= 16 colours, else direct."""
    if fmt == 2 and siz == 0:
        return 'Pal16'
    return 'Pal16' if unique_values(kind, img, fmt, siz, sw, sh) <= 16 else 'Direct'


def run_dl(kind, fid, off, state, emit, depth, seen_calls):
    f = kind.files.get(fid)
    o = kind.obj_at(fid, off)
    if f is None or o is None or o.type_name != 'Gfx':
        return
    payload = f.source['payload']
    end = o.offset + o.size
    pos = off
    while pos + 8 <= end:
        w0, w1 = BE2.unpack_from(payload, pos)
        op = w0 >> 24
        if op == 0xFD:
            state['timg'] = (kind.ptr(fid, pos + 4), (w0 >> 21) & 7, (w0 >> 19) & 3)
        elif op == 0xF5:
            t = (w1 >> 24) & 7
            tile = state['tiles'].setdefault(t, {})
            tile.update(fmt=(w0 >> 21) & 7, siz=(w0 >> 19) & 3, line=(w0 >> 9) & 0x1ff,
                        tmem=w0 & 0x1ff, pal=(w1 >> 20) & 15, cmt=(w1 >> 18) & 3,
                        maskt=(w1 >> 14) & 15, cms=(w1 >> 8) & 3, masks=(w1 >> 4) & 15)
        elif op == 0xF3:
            t = (w1 >> 24) & 7
            tile = state['tiles'].setdefault(t, {})
            state['tmem'][tile.get('tmem', 0)] = (state['timg'], ((w1 >> 12) & 0xfff) + 1)
        elif op == 0xF4:
            t = (w1 >> 24) & 7
            tile = state['tiles'].setdefault(t, {})
            uls, ult = (w0 >> 12) & 0xfff, w0 & 0xfff
            lrs, lrt = (w1 >> 12) & 0xfff, w1 & 0xfff
            texels = (((lrs - uls) >> 2) + 1) * (((lrt - ult) >> 2) + 1)
            state['tmem'][tile.get('tmem', 0)] = (state['timg'], texels)
        elif op == 0xF0:
            state['tlut'] = state['timg'][0] if state['timg'] else None
        elif op == 0xF2:
            t = (w1 >> 24) & 7
            tile = state['tiles'].setdefault(t, {})
            uls, ult = (w0 >> 12) & 0xfff, w0 & 0xfff
            lrs, lrt = (w1 >> 12) & 0xfff, w1 & 0xfff
            tile['w'] = ((lrs - uls) >> 2) + 1
            tile['h'] = ((lrt - ult) >> 2) + 1
        elif op == 0xD7:
            state['tex_on'] = (w0 & 0xff) != 0
            state['tex_tile'] = (w0 >> 8) & 7
        elif op in (0x05, 0x06, 0x07):
            if state['tex_on']:
                emit(state, (fid, o.offset))
        elif op == 0xDE:
            tgt = kind.ptr(fid, pos + 4)
            branch = ((w0 >> 16) & 0xff) != 0
            if tgt is not None and depth < 24 and (tgt, depth) not in seen_calls:
                seen_calls.add((tgt, depth))
                run_dl(kind, tgt[0], tgt[1], state, emit, depth + 1, seen_calls)
            if branch:
                return
        elif op == 0xDF:
            return
        pos += 8


def enumerate_kind(kind):
    keys = {}
    templates = []   # DL tile setups whose image comes from an MObj

    def emit(state, dl):
        tile = state['tiles'].get(state['tex_tile'], {})
        if 'fmt' not in tile:
            return
        load = state['tmem'].get(tile.get('tmem', 0))
        if load is None:
            return
        (timg, load16) = load
        fmt, siz = tile['fmt'], tile['siz']
        dims = ds_dims(fmt, siz, tile.get('line', 0), tile.get('w', 0), tile.get('h', 0), load16,
                       tile.get('cms', 0), tile.get('cmt', 0), tile.get('masks', 0), tile.get('maskt', 0))
        if dims is None:
            return
        img = timg[0] if timg else None
        tlut = state['tlut'] if fmt == 2 else None
        rec = dict(fmt=fmt, siz=siz, line=tile.get('line', 0), tile_w=tile.get('w', 0),
                   tile_h=tile.get('h', 0), load16=load16, cms=tile.get('cms', 0),
                   cmt=tile.get('cmt', 0), masks=tile.get('masks', 0), maskt=tile.get('maskt', 0),
                   ds_w=dims[0], ds_h=dims[1], src_w=dims[2], src_h=dims[3], dl=dl)
        if img is None:
            templates.append(rec)
            return
        k = (tuple(img), tuple(tlut) if tlut else None, dims[0], dims[1])
        if k not in keys:
            rec['img'] = tuple(img)
            rec['tlut'] = tuple(tlut) if tlut else None
            rec['source'] = 'DL'
            rec['dls'] = set()
            keys[k] = rec
        keys[k]['dls'].add(dl)

    for fid, objs in kind.objects.items():
        for o in objs:
            if o.type_name != 'Gfx':
                continue
            state = {'timg': None, 'tiles': {}, 'tmem': {}, 'tlut': None, 'tex_on': True, 'tex_tile': 0}
            run_dl(kind, fid, o.offset, state, emit, 0, set())

    # MObjSub tables: every sprite x palette the material can show.
    mobj_keys = {}
    for fid, objs in kind.objects.items():
        for o in objs:
            if o.type_name != 'MObjSub' or o.size < 0x30:
                continue
            base = o.offset
            fmt = kind.files[fid].source['payload'][base + 2]
            siz = kind.files[fid].source['payload'][base + 3]
            spr_tbl = kind.ptr(fid, base + 4)
            pal_tbl = kind.ptr(fid, base + 0x2C)
            tw, th = kind.u16(fid, base + 0x0C), kind.u16(fid, base + 0x0E)
            sprites = []
            if spr_tbl is not None:
                t = kind.obj_at(*spr_tbl)
                n = (t.size // 4) if t else 1
                for i in range(n):
                    p = kind.ptr(spr_tbl[0], spr_tbl[1] + 4 * i)
                    if p is not None:
                        sprites.append(p)
            palettes = []
            if pal_tbl is not None:
                t = kind.obj_at(*pal_tbl)
                n = (t.size // 4) if t else 1
                for i in range(n):
                    p = kind.ptr(pal_tbl[0], pal_tbl[1] + 4 * i)
                    if p is not None:
                        palettes.append(p)
            if not sprites:
                continue
            for s in sprites:
                mobj_keys[(tuple(s), base, fid)] = dict(img=tuple(s), fmt=fmt, siz=siz, tw=tw, th=th,
                                                        palettes=[tuple(p) for p in palettes],
                                                        mobj=(fid, base))
    return keys, templates, mobj_keys


def main(kinds_arg=None):
    kinds = (kinds_arg or (sys.argv[1] if len(sys.argv) > 1 else 'Donkey,Samus,Link,Kirby')).split(',')
    types = est.TypeTable()
    types.load_dirs(est.HEADER_DIRS)
    result = {}
    for name in kinds:
        idx, _ = est.index_closure(name, types)
        kind = Kind(name, idx)
        keys, templates, mobj = enumerate_kind(kind)
        result[name] = (kind, keys, templates, mobj)
        print('%-8s DL keys %3d  MObj-image templates %3d  MObj sprite entries %3d' % (
            name, len(keys), len(templates), len(mobj)))
    return result


if __name__ == '__main__':
    main()
