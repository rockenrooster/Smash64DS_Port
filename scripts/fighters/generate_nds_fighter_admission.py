#!/usr/bin/env python3
"""Generate the fighter texture admission table (P2-2p8 Phase 1 slice 2b).

Every fighter texture a kind can show, enumerated WITHOUT drawing, as the RDP
texture state the draw would present at the textured triangle -- so the
runtime can replay it through the renderer's own state recorders and the
texture cache's own resolve/convert/upload path, and pin the result before GO.

Enumeration (validated in slice 2a against the runtime key recorder:
artifacts/performance/2026-09-23_p2-2p8-phase1-slice2a/README.md section 2):

  input    the relocData closure fighter creation loads
           (estimate_fighter_pack.index_closure: payload, pointer map, types)
  parts    per detail, the fighter's own part tables exactly as ftparam.c
           selects them: FTCommonPartContainer.commonparts[d].dobjdesc[j].dl
           with its p_mobjsubs[j] (LOW falls back to HIGH per joint when the
           LOW dl is NULL, ftparam.c:788), and every FTModelPartDesc
           modelparts[mp][d] (model-part states, Entry/Appear, Kirby hats)
  DLs      SETTIMG / SETTILE / LOADBLOCK / LOADTILE / LOADTLUT / SETTILESIZE /
           TEXTURE / SETCOMBINE / SETOTHERMODE_H/L / RDPSETOTHERMODE /
           SETPRIMCOLOR / SETENVCOLOR / DL / ENDDL; a material segment call
           (G_DL 0x0E000000 + 8*i) applies MObj i's branch the way
           objdisplay.c builds it (PALETTE, FRAC|SPLIT, FRAC|ALPHA, prim/env)
           for every sprite (texture animation) and every palette
  record   at each textured triangle: the image/TLUT (asset id + SOURCE
           offset -- the runtime maps it with ndsRelocNativeAssetAddress), the
           exact command words that loaded and described them, and the
           combine / othermode / prim / env / texture state
  costume  each record carries the costumes that select its image and TLUT
           banks (estimate_fighter_pack.resolve_costume_membership); banks
           outside a resolved costume table, and animation-reachable banks,
           are admitted for every costume
  hats     Kirby's joint-6 model parts 3..13 (228_KirbyMainMotion.c
           FTKirbyCopy) are tagged with the copied kind; the runtime admits a
           hat only when that kind is in the match

Outputs:
  --out PATH   the NitroFS payload (fighters/admission.bin)
  the tracked header include/nds/generated/nds_fighter_admission.generated.h
               (format, per kind x detail record counts, payload size + hash)
  --check      regenerate in memory; exit 1 when the tracked header differs
  --report     per kind x detail texture counts and DS bytes
"""
import argparse
import collections
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, HERE)
import estimate_fighter_pack as est  # noqa: E402

HEADER_PATH = os.path.join(ROOT, 'include', 'nds', 'generated',
                           'nds_fighter_admission.generated.h')
MAGIC = 0x314D4441  # 'ADM1'
VERSION = 1
RECORD_WORDS = 28

# BattleShip fttypes.h ordinals -> (closure name, Main file id)
KINDS = [
    ('Mario', 0xcb), ('Fox', 0xd1), ('Donkey', 0xd5), ('Samus', 0xd9),
    ('Luigi', 0xdd), ('Link', 0xe1), ('Yoshi', 0xf7), ('Captain', 0xec),
    ('Kirby', 0xe5), ('Pikachu', 0xf3), ('Purin', 0xe9), ('Ness', 0xef),
]
KIND_INDEX = {name: i for i, (name, _) in enumerate(KINDS)}
# 228_KirbyMainMotion.c FTKirbyCopy: copied kind -> joint-6 model part
KIRBY_HAT_MP = {'Mario': 12, 'Fox': 7, 'Donkey': 4, 'Samus': 8, 'Luigi': 11,
                'Link': 10, 'Yoshi': 5, 'Captain': 9, 'Pikachu': 6,
                'Purin': 3, 'Ness': 13}
KIRBY_JOINT6_DESC = 0x0CC   # 229_KirbyMain.c dKirbyMain_modelparts_desc_0x0CC

DOBJDESC = 44
FTMODELPART = 20
MOBJ_ALPHA, MOBJ_SPLIT, MOBJ_PALETTE, MOBJ_FRAC = 1, 2, 4, 16
MOBJ_TEXTURE, MOBJ_PRIMCOLOR, MOBJ_ENVCOLOR = 1 << 7, 1 << 9, 1 << 10
MAXD = 128
CLAMP = 2

F_HAS_TLUT = 1 << 0
F_LOAD_TILE = 1 << 1
F_HAS_PRIM = 1 << 2
F_HAS_ENV = 1 << 3
F_HAT = 1 << 4
F_MOBJ = 1 << 5


# -- DS size (diagnostic only; the runtime converts with its own code) -------

def _next_pow2(v):
    p = 8
    while p < v:
        p <<= 1
    return p


def _source_bytes(fmt, siz, texels):
    if fmt == 2:
        return (texels + 1) // 2 if siz == 0 else (texels if siz == 1 else 0)
    if fmt == 0:
        return texels * 2 if siz == 2 else (texels * 4 if siz == 3 else 0)
    return (texels + 1) // 2 if siz == 0 else (texels if siz == 1 else texels * 2)


def ds_dims(fmt, siz, line, tile_w, tile_h, load16, cms, cmt, masks, maskt):
    """nds_renderer_textures_effects.c: extent, materialise, windows, pow2."""
    loaded = load16 * (4 if siz == 3 else 2)
    w, h = tile_w, tile_h
    if (not w or not h or w > MAXD or h > MAXD or
            _source_bytes(fmt, siz, w * h) > loaded):
        w = line * (16, 8, 4, 2)[siz]
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
        return tile > me and me <= src <= tile
    ms, mt = mat(cms, masks, sw, tile_w), mat(cmt, maskt, sh, tile_h)
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
    return _next_pow2(w), _next_pow2(h)


# -- closure access ------------------------------------------------------------

class Closure:
    def __init__(self, name, idx):
        self.name = name
        self.files = {f.file_id: f for f in idx.files}
        self.objects = collections.defaultdict(list)
        for f in idx.files:
            for o in f.objects:
                self.objects[f.file_id].append(o)
        for v in self.objects.values():
            v.sort(key=lambda o: o.offset)

    def obj_at(self, fid, off):
        for o in self.objects.get(fid, ()):
            if o.offset <= off < o.offset + max(o.size, 1):
                return o
        return None

    def payload(self, fid):
        return self.files[fid].source['payload']

    def ptr(self, fid, slot):
        f = self.files.get(fid)
        return None if f is None else f.source['pointers'].get(slot)

    def u16(self, fid, off):
        return struct.unpack_from('>H', self.payload(fid), off)[0]

    def u32(self, fid, off):
        return struct.unpack_from('>I', self.payload(fid), off)[0]

    def pointer_list(self, ref, max_n=64):
        o = self.obj_at(*ref)
        n = (o.size - (ref[1] - o.offset)) // 4 if o else 1
        return [self.ptr(ref[0], ref[1] + 4 * i) for i in range(min(n, max_n))]

    def dl_roots(self, ref):
        """A DObjDesc dl field names a Gfx or a Gfx* list (pairs)."""
        o = self.obj_at(*ref)
        if o is None:
            return []
        if o.type_name == 'Gfx':
            return [tuple(ref)]
        out = []
        for p in self.pointer_list(ref, 16):
            if p is not None:
                q = self.obj_at(*p)
                if q is not None and q.type_name == 'Gfx':
                    out.append(tuple(p))
        return out

    def mobjsubs(self, ref):
        out = []
        for p in self.pointer_list(ref, 32):
            if p is None:
                continue
            q = self.obj_at(*p)
            if q is not None and q.type_name == 'MObjSub':
                out.append((p[0], q.offset))
        return out


def mobj_info(c, ref):
    fid, off = ref
    data = c.payload(fid)
    spr = c.ptr(fid, off + 4)
    pal = c.ptr(fid, off + 0x2C)
    flags = struct.unpack_from('>H', data, off + 0x30)[0]
    sprites = [tuple(p) for p in (c.pointer_list(spr, 64) if spr else []) if p]
    palettes = [tuple(p) for p in (c.pointer_list(pal, 64) if pal else []) if p]
    return dict(
        fmt=data[off + 2], siz=data[off + 3], flags=flags,
        bfmt=data[off + 0x32], bsiz=data[off + 0x33],
        prim=struct.unpack_from('>I', data, off + 0x50)[0],
        prim_m=data[off + 0x55],
        env=struct.unpack_from('>I', data, off + 0x58)[0],
        sprites=sprites, palettes=palettes)


def part_pairs(c, main, d):
    """[(dl root, [MObjSub refs], where)] for one detail, ftparam.c rules."""
    out = []
    for cont in [o for o in c.objects.get(main, ()) if o.type_name == 'FTCommonPartContainer']:
        dls, pms = {}, {}
        for dd in (0, 1):
            base = cont.offset + 16 * dd
            desc = c.ptr(main, base)
            pm = c.ptr(main, base + 4)
            dls[dd] = []
            if desc:
                o = c.obj_at(*desc)
                n = (o.size // DOBJDESC) if o else 0
                dls[dd] = [c.ptr(desc[0], desc[1] + DOBJDESC * i + 4) for i in range(n)]
            pms[dd] = c.pointer_list(pm, 64) if pm else []
        for j in range(max(len(dls[0]), len(dls[1]))):
            dl0 = dls[0][j] if j < len(dls[0]) else None
            dl1 = dls[1][j] if j < len(dls[1]) else None
            ms0 = pms[0][j] if j < len(pms[0]) else None
            ms1 = pms[1][j] if j < len(pms[1]) else None
            dl, ms = (dl0, ms0) if (d == 0 or not dl1) else (dl1, ms1)
            if dl:
                mobjs = c.mobjsubs(ms) if ms else []
                for r in c.dl_roots(dl):
                    out.append((r, mobjs, ('joint', j)))
    for cont in [o for o in c.objects.get(main, ()) if o.type_name == 'FTModelPartDesc']:
        for ji, desc in enumerate(c.pointer_list((main, cont.offset), 64)):
            if desc is None:
                continue
            o = c.obj_at(*desc)
            if o is None:
                continue
            for i in range(o.size // FTMODELPART):
                if i % 2 != d:
                    continue
                slot = desc[1] + FTMODELPART * i
                dl = c.ptr(desc[0], slot)
                ms = c.ptr(desc[0], slot + 4)
                if dl:
                    mobjs = c.mobjsubs(ms) if ms else []
                    for r in c.dl_roots(dl):
                        out.append((r, mobjs, ('mp', ji, i // 2, tuple(desc))))
    return out


def kirby_hat_of(where):
    """Copied kind for a Kirby joint-6 hat model part, else None."""
    if where[0] != 'mp' or where[3] != (0xe5, KIRBY_JOINT6_DESC):
        return None
    for kind, mp in KIRBY_HAT_MP.items():
        if where[2] == mp:
            return kind
    return None


# -- the RDP texture state machine ---------------------------------------------

BE2 = struct.Struct('>II')


# The fighter display's own preamble, which every part DL inherits
# (ftdisplaymain.c:1176-1178): gDPSetCycleType(G_CYC_2CYCLE) and
# gDPSetRenderMode(G_RM_FOG_PRIM_A, G_RM_AA_ZB_OPA_SURF2). The cycle type
# decides which combiner cycle's alpha the texture key's ALPHA_IGNORES_TEXELS
# bit reads, so a walk that started from 1-cycle built a different key than
# the draw for every fighter texture (slice 2b, measured).
PREAMBLE_OTHERMODE_H = 1 << 20          # G_CYC_2CYCLE
PREAMBLE_OTHERMODE_L = 0xC4112078       # G_RM_FOG_PRIM_A | G_RM_AA_ZB_OPA_SURF2


def fresh_state():
    return {
        'timg': None, 'tiles': {}, 'tmem': {}, 'tlut': None,
        'tex': (0xD7000002, 0xFFFFFFFF), 'combine': (0, 0),
        'omh': PREAMBLE_OTHERMODE_H, 'oml': PREAMBLE_OTHERMODE_L,
        'prim': None, 'env': None,
    }


def run(c, fid, off, st, emit, depth, mobj_hook):
    f = c.files.get(fid)
    o = c.obj_at(fid, off)
    if f is None or o is None or o.type_name != 'Gfx':
        return
    payload = f.source['payload']
    pos, end = off, o.offset + o.size
    while pos + 8 <= end:
        w0, w1 = BE2.unpack_from(payload, pos)
        op = w0 >> 24
        if op == 0xFD:
            st['timg'] = (w0, c.ptr(fid, pos + 4))
        elif op == 0xF5:
            st['tiles'].setdefault((w1 >> 24) & 7, {})['set'] = (w0, w1)
        elif op in (0xF3, 0xF4):
            t = (w1 >> 24) & 7
            tile = st['tiles'].setdefault(t, {})
            sw = tile.get('set', (0, t << 24))
            tmem = sw[0] & 0x1ff
            if op == 0xF3:
                texels = ((w1 >> 12) & 0xfff) + 1
            else:
                uls, ult, lrs, lrt = (w0 >> 12) & 0xfff, w0 & 0xfff, (w1 >> 12) & 0xfff, w1 & 0xfff
                texels = (((lrs - uls) >> 2) + 1) * (((lrt - ult) >> 2) + 1)
            st['tmem'][tmem] = dict(timg=st['timg'], settile=sw, op=op, w0=w0, w1=w1,
                                    texels=texels)
        elif op == 0xF0:
            t = (w1 >> 24) & 7
            tile = st['tiles'].setdefault(t, {})
            st['tlut'] = dict(timg=st['timg'], settile=tile.get('set', (0, t << 24)), w1=w1)
        elif op == 0xF2:
            st['tiles'].setdefault((w1 >> 24) & 7, {})['size'] = (w0, w1)
        elif op == 0xD7:
            st['tex'] = (w0, w1)
        elif op == 0xFC:
            st['combine'] = (w0, w1)
        elif op in (0xE3, 0xE2):
            shift = 32 - ((w0 >> 8) & 0xff) - ((w0 & 0xff) + 1)
            length = (w0 & 0xff) + 1
            mask = ((1 << length) - 1) << shift if 0 <= shift < 32 else 0
            key = 'omh' if op == 0xE3 else 'oml'
            st[key] = (st[key] & ~mask & 0xffffffff) | (w1 & mask)
        elif op == 0xEF:
            st['omh'] = (st['omh'] & 0xff000000) | (w0 & 0x00ffffff)
            st['oml'] = w1
        elif op == 0xFA:
            st['prim'] = (w0, w1)
        elif op == 0xFB:
            st['env'] = (w0, w1)
        elif op in (0x05, 0x06, 0x07):
            if st['tex'][0] & 0xff:
                emit(st)
        elif op == 0xDE:
            tgt = c.ptr(fid, pos + 4)
            branch = ((w0 >> 16) & 0xff) != 0
            if tgt is None and (w1 >> 24) == 0x0E and mobj_hook is not None:
                mobj_hook(st, (w1 & 0xffffff) // 8)
            elif tgt is not None and depth < 24:
                run(c, tgt[0], tgt[1], st, emit, depth + 1, mobj_hook)
            if branch:
                return
        elif op == 0xDF:
            return
        pos += 8


def settimg_w0(fmt, siz, width):
    return (0xFD << 24) | ((fmt & 7) << 21) | ((siz & 3) << 19) | ((width - 1) & 0xfff)


def mobj_branch(info, si, pi, st):
    """objdisplay.c's branch DL for one MObj, sprite si / palette pi."""
    fl = info['flags']
    spr, pal = info['sprites'], info['palettes']
    if fl & MOBJ_PALETTE and pal:
        p = pal[min(pi, len(pal) - 1)]
        st['timg'] = (settimg_w0(0, 2, 1), p)
        if fl & (MOBJ_SPLIT | MOBJ_ALPHA):
            # SETTILE(RGBA, 4b, 0, 0x100, tile 5) ; LOADTLUT(5, 15|255)
            tile5 = ((0xF5 << 24) | (0 << 21) | (0 << 19) | 0x100, 5 << 24)
            st['tiles'].setdefault(5, {})['set'] = tile5
            count = 0xFF if info['siz'] == 1 else 0xF
            st['tlut'] = dict(timg=st['timg'], settile=tile5,
                              w1=(5 << 24) | ((count & 0x3ff) << 14))
    if fl & (MOBJ_PRIMCOLOR | MOBJ_FRAC | 0x8):
        st['prim'] = ((0xFA << 24) | ((info['prim_m'] & 0xff) << 8), info['prim'])
    if fl & MOBJ_ENVCOLOR:
        st['env'] = (0xFB << 24, info['env'])
    if fl & (MOBJ_FRAC | MOBJ_SPLIT) and spr:
        bsiz = 3 if info['bsiz'] == 3 else 2
        st['timg'] = (settimg_w0(info['bfmt'], bsiz, 1), spr[min(si + 1, len(spr) - 1)])
    if fl & (MOBJ_FRAC | MOBJ_ALPHA) and spr:
        st['timg'] = (settimg_w0(info['fmt'], info['siz'], 1), spr[min(si, len(spr) - 1)])
    if fl & MOBJ_TEXTURE:
        st['tex'] = (0xD7000002, st['tex'][1])


# -- enumeration ---------------------------------------------------------------

def bank_mask(membership, c, ref):
    """Costumes that select the bank holding ref (0xff = every costume)."""
    if ref is None:
        return 0xff
    o = c.obj_at(*ref)
    if o is None:
        return 0xff
    key = (o.file_id, o.symbol)
    if key not in membership.banks_in_resolved_tables:
        return 0xff
    sel = membership.selected.get(key, frozenset())
    if not sel:
        return 0xff   # animation-reachable: selected by no costume
    m = 0
    for i in sel:
        if 0 <= i < 8:
            m |= 1 << i
    return m or 0xff


def enumerate_kind(name, main, c, membership):
    """{detail: {record key: record dict}}"""
    out = {}
    for d in (0, 1):
        recs = collections.OrderedDict()
        for root, mobjs, where in part_pairs(c, main, d):
            infos = [mobj_info(c, m) for m in mobjs]
            n_spr = max([len(i['sprites']) for i in infos] + [1])
            n_pal = max([len(i['palettes']) for i in infos] + [1])
            hat = kirby_hat_of(where) if name == 'Kirby' else None
            for si in range(n_spr):
                for pi in range(n_pal):
                    def hook(st, idx, si=si, pi=pi):
                        if idx < len(infos):
                            mobj_branch(infos[idx], si, pi, st)

                    def emit(st, hat=hat, via_mobj=bool(infos)):
                        tile_i = (st['tex'][0] >> 8) & 7
                        tile = st['tiles'].get(tile_i, {})
                        if 'set' not in tile:
                            return
                        rw0, rw1 = tile['set']
                        load = st['tmem'].get(rw0 & 0x1ff)
                        if load is None or load['timg'] is None or load['timg'][1] is None:
                            return
                        img = tuple(load['timg'][1])
                        fmt, siz = (rw0 >> 21) & 7, (rw0 >> 19) & 3
                        tlut = st['tlut'] if (fmt == 2 and st['tlut'] and
                                              st['tlut']['timg'] and st['tlut']['timg'][1]) else None
                        ts = tile.get('size', ((0xF2 << 24), (tile_i << 24)))
                        tw = (((ts[1] >> 12) & 0xfff) - ((ts[0] >> 12) & 0xfff) >> 2) + 1
                        th = (((ts[1]) & 0xfff) - ((ts[0]) & 0xfff) >> 2) + 1
                        dims = ds_dims(fmt, siz, (rw0 >> 9) & 0x1ff, tw, th, load['texels'],
                                       (rw1 >> 8) & 3, (rw1 >> 18) & 3, (rw1 >> 4) & 15,
                                       (rw1 >> 14) & 15)
                        if dims is None:
                            return
                        tl_ref = tuple(tlut['timg'][1]) if tlut else None
                        rec = dict(
                            img=img, tlut=tl_ref, settimg=load['timg'][0],
                            load_set=load['settile'], load_op=load['op'],
                            load_w=(load['w0'], load['w1']),
                            tlut_settimg=(tlut['timg'][0] if tlut else 0),
                            tlut_set=(tlut['settile'] if tlut else (0, 0)),
                            loadtlut_w1=(tlut['w1'] if tlut else 0),
                            render_set=(rw0, rw1), tilesize=ts, tex=st['tex'],
                            combine=st['combine'], omh=st['omh'], oml=st['oml'],
                            prim=st['prim'], env=st['env'], hat=hat, mobj=via_mobj,
                            dims=dims,
                            costume=bank_mask(membership, c, img) & bank_mask(membership, c, tl_ref))
                        key = (img, tl_ref, rec['load_set'], rec['load_w'], rec['render_set'],
                               rec['tilesize'], rec['combine'], rec['omh'], rec['oml'],
                               rec['prim'], rec['env'], rec['tex'][0], hat)
                        if key in recs:
                            recs[key]['costume'] |= rec['costume']
                        else:
                            recs[key] = rec
                    st = fresh_state()
                    run(c, root[0], root[1], st, emit, 0, hook)
        out[d] = recs
    return out


def joint_root_max(c, main):
    """The most joints one detail of this kind can carry a display list on:
    common-part joints with a DL (after the LOW fallback) plus every joint a
    model part can put a DL on. The lean path's per-root arrays are sized by
    the maximum over all kinds (NDS_FIGHTER_ADMISSION_ROOT_MAX)."""
    best = 0
    for d in (0, 1):
        joints = set()
        for _root, _mobjs, where in part_pairs(c, main, d):
            joints.add(where[1])
        best = max(best, len(joints))
    return best


def pack_record(r, fkind):
    flags = 0
    if r['tlut'] is not None:
        flags |= F_HAS_TLUT
    if r['load_op'] == 0xF4:
        flags |= F_LOAD_TILE
    if r['prim'] is not None:
        flags |= F_HAS_PRIM
    if r['env'] is not None:
        flags |= F_HAS_ENV
    if r['mobj']:
        flags |= F_MOBJ
    hat_kind = 0xff
    if r['hat'] is not None:
        flags |= F_HAT
        hat_kind = KIND_INDEX[r['hat']]
    w = [0] * RECORD_WORDS
    w[0] = flags | (hat_kind << 8)
    w[1] = r['costume'] & 0xff
    w[2] = (r['img'][0] & 0xffff) | (((r['tlut'][0] if r['tlut'] else 0) & 0xffff) << 16)
    w[3] = r['img'][1]
    w[4] = r['tlut'][1] if r['tlut'] else 0
    w[5] = r['settimg']
    w[6], w[7] = r['load_set']
    w[8], w[9] = r['load_w']
    w[10] = r['tlut_settimg']
    w[11], w[12] = r['tlut_set']
    w[13] = r['loadtlut_w1']
    w[14], w[15] = r['render_set']
    w[16], w[17] = r['tilesize']
    w[18], w[19] = r['tex']
    w[20], w[21] = r['combine']
    w[22] = r['omh']
    w[23] = r['oml']
    if r['prim'] is not None:
        w[24], w[25] = r['prim']
    if r['env'] is not None:
        w[26] = r['env'][1]
    w[27] = (r['dims'][0] & 0xffff) | ((r['dims'][1] & 0xffff) << 16)
    return struct.pack('<%dI' % RECORD_WORDS, *[x & 0xffffffff for x in w])


def fnv1a32(data):
    h = 2166136261
    for b in data:
        h ^= b
        h = (h * 16777619) & 0xffffffff
    return h


def build(kinds_only=None):
    types = est.TypeTable()
    types.load_dirs(est.HEADER_DIRS)
    census = est.parse_native_image_census(est.NATIVE_IMAGE_FLAGS_BY_NAME['hwtri'])
    tables = {}
    names = [n for n, _ in KINDS if (kinds_only is None or n in kinds_only)]
    ledgers = est.build_fighter_ledgers(names, types, 'hwtri', census)
    root_max = 0
    for name, main in KINDS:
        if name not in ledgers:
            tables[name] = {0: {}, 1: {}}
            continue
        L = ledgers[name]
        c = Closure(name, L.idx)
        tables[name] = enumerate_kind(name, main, c, L.membership)
        root_max = max(root_max, joint_root_max(c, main))
    build.root_max = root_max
    # payload: header (magic, version, kinds, 12 x 2 x (first, count)) + records
    index = []
    records = b''
    count = 0
    for fkind, (name, _) in enumerate(KINDS):
        for d in (0, 1):
            recs = list(tables[name][d].values())
            index.append((count, len(recs)))
            for r in recs:
                records += pack_record(r, fkind)
            count += len(recs)
    head = struct.pack('<4I', MAGIC, VERSION, len(KINDS), RECORD_WORDS)
    head += b''.join(struct.pack('<2I', a, b) for a, b in index)
    payload = head + records
    return tables, index, payload


def render_header(index, payload):
    lines = [
        '/* GENERATED by scripts/fighters/generate_nds_fighter_admission.py -- do not edit.',
        ' * P2-2p8 Phase 1 slice 2b: the fighter texture admission table. The',
        ' * records ship as NitroFS fighters/admission.bin; this header pins the',
        ' * format and the payload (size + FNV-1a) so a stale payload is refused',
        ' * and `--check` catches drift. */',
        '#pragma once',
        '',
        '#define NDS_FIGHTER_ADMISSION_MAGIC 0x%08Xu' % MAGIC,
        '#define NDS_FIGHTER_ADMISSION_VERSION %du' % VERSION,
        '#define NDS_FIGHTER_ADMISSION_KINDS %du' % len(KINDS),
        '#define NDS_FIGHTER_ADMISSION_RECORD_WORDS %du' % RECORD_WORDS,
        '#define NDS_FIGHTER_ADMISSION_HEADER_BYTES %du' % (16 + 8 * len(index)),
        '#define NDS_FIGHTER_ADMISSION_PAYLOAD_BYTES %du' % len(payload),
        '#define NDS_FIGHTER_ADMISSION_PAYLOAD_FNV 0x%08Xu' % fnv1a32(payload),
        '#define NDS_FIGHTER_ADMISSION_F_HAS_TLUT 0x%02Xu' % F_HAS_TLUT,
        '#define NDS_FIGHTER_ADMISSION_F_LOAD_TILE 0x%02Xu' % F_LOAD_TILE,
        '#define NDS_FIGHTER_ADMISSION_F_HAS_PRIM 0x%02Xu' % F_HAS_PRIM,
        '#define NDS_FIGHTER_ADMISSION_F_HAS_ENV 0x%02Xu' % F_HAS_ENV,
        '#define NDS_FIGHTER_ADMISSION_F_HAT 0x%02Xu' % F_HAT,
        '#define NDS_FIGHTER_ADMISSION_F_MOBJ 0x%02Xu' % F_MOBJ,
        '/* most DL-bearing joints of any kind x detail (common parts + model',
        ' * parts): the bound for per-root arrays (src/port/renderer_fighter_lean.c) */',
        '#define NDS_FIGHTER_ADMISSION_ROOT_MAX %du' % getattr(build, 'root_max', 0),
        '',
        '/* records per kind (BattleShip ordinal) x detail (0 HIGH, 1 LOW) */',
    ]
    for fkind, (name, _) in enumerate(KINDS):
        for d in (0, 1):
            first, n = index[fkind * 2 + d]
            lines.append('/* %-8s %s: first %4d count %3d */' % (name, 'LOW ' if d else 'HIGH', first, n))
    return '\n'.join(lines) + '\n'


def report(tables):
    bpp = {'Pal16': 4}
    print('%-8s %-4s %6s %6s %10s %8s' % ('kind', 'det', 'recs', 'hats', 'DS B (all)', 'costume0'))
    for name, _ in KINDS:
        for d in (1, 0):
            recs = list(tables[name][d].values())
            hats = sum(1 for r in recs if r['hat'])
            allb = sum(r['dims'][0] * r['dims'][1] // 2 for r in recs)
            c0 = sum(r['dims'][0] * r['dims'][1] // 2 for r in recs
                     if (r['costume'] & 1) and not r['hat'])
            print('%-8s %-4s %6d %6d %10s %8s' % (name, 'LOW' if d else 'HIGH', len(recs), hats,
                                                  '{:,}'.format(allb), '{:,}'.format(c0)))


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--out', default=None, help='write the NitroFS payload here')
    ap.add_argument('--check', action='store_true',
                    help='exit 1 when the tracked header differs from a regeneration')
    ap.add_argument('--report', action='store_true')
    ap.add_argument('--no-header', action='store_true',
                    help='do not rewrite the tracked header (payload only)')
    args = ap.parse_args(argv)
    tables, index, payload = build()
    header = render_header(index, payload)
    if args.check:
        try:
            current = open(HEADER_PATH, encoding='utf-8').read()
        except OSError:
            current = ''
        if current != header:
            print('generate_nds_fighter_admission: %s is stale -- rerun the generator'
                  % os.path.relpath(HEADER_PATH, ROOT), file=sys.stderr)
            return 1
        print('generate_nds_fighter_admission: OK (%d records, %d bytes, fnv 0x%08x)'
              % ((len(payload) - 16 - 8 * len(index)) // (4 * RECORD_WORDS), len(payload),
                 fnv1a32(payload)))
        return 0
    if not args.no_header:
        os.makedirs(os.path.dirname(HEADER_PATH), exist_ok=True)
        with open(HEADER_PATH, 'w', encoding='utf-8', newline='\n') as fh:
            fh.write(header)
    if args.out:
        os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
        with open(args.out, 'wb') as fh:
            fh.write(payload)
    if args.report:
        report(tables)
    return 0


if __name__ == '__main__':
    sys.exit(main())
