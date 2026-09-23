"""Slice 2a deliverable 2: per-kind reachable fighter DS texture set.

Walks what creation loads (relocData closure via estimate_fighter_pack):
every display list (reach.run_dl) and every MObjSub sprite/palette table.
DS bytes follow the port's conversion rules (reach.ds_dims / ds_format).
Kirby's copy hats (joint 6 model parts 3..13, 228_KirbyMainMotion.c copy
table) are attributed per copied kind. Cross-checked against the runtime
keys of a census-b run. Writes reachable-set.json and prints the tables."""
import collections, itertools, json, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import reach  # noqa: E402

ART = reach.ART
KINDS = ['Mario', 'Fox', 'Donkey', 'Samus', 'Luigi', 'Link', 'Yoshi', 'Captain', 'Kirby',
         'Pikachu', 'Purin', 'Ness']
PACK_NUM = {k: '%02d' % i for i, k in enumerate(KINDS)}
MODEL_FILE = {'Mario': 0x128, 'Fox': 0x139, 'Donkey': 0x13d, 'Samus': 0x140, 'Luigi': 0x143,
              'Link': 0x144, 'Yoshi': 0x152, 'Captain': 0x14c, 'Kirby': 0x148, 'Pikachu': 0x155,
              'Purin': 0x14a, 'Ness': 0x14f}
HAT_MP = {'Mario': 12, 'Fox': 7, 'Donkey': 4, 'Samus': 8, 'Luigi': 11, 'Link': 10, 'Yoshi': 5,
          'Captain': 9, 'Pikachu': 6, 'Purin': 3, 'Ness': 13}
KIRBY_JOINT6_DESC = 0x0CC   # 229_KirbyMain.c dKirbyMain_modelparts_desc_0x0CC (joint 6)
KIRBY_MAIN_FILE = 0xe5
FTMODELPART = 20


def tex_bytes(w, h, fmt):
    return w * h // 2 if fmt == 'Pal16' else w * h * 2


def enumerate_kind(name, kind, observed_dims):
    """Returns {texture key: record}; key = (image, palette-or-None, w, h)."""
    keys, templates, mobj = reach.enumerate_kind(kind)
    out = {}
    for k, rec in keys.items():
        fmt = reach.ds_format(kind, rec['img'], rec['fmt'], rec['siz'], rec['src_w'], rec['src_h'])
        key = (rec['img'], None, rec['ds_w'], rec['ds_h'])
        if key in out:
            out[key]['dls'] |= rec['dls']
            continue
        out[key] = dict(src='DL', fmt=fmt, w=rec['ds_w'], h=rec['ds_h'], dl=rec['dl'],
                        dls=set(rec['dls']))
    # MObj sprite x palette tables
    by_mobj = collections.defaultdict(list)
    for (img, base, fid), rec in mobj.items():
        by_mobj[rec['mobj']].append(rec)
    for mobj_id, recs in by_mobj.items():
        sprites = [r['img'] for r in recs]
        palettes = recs[0]['palettes'] or [None]
        # dims: an observed runtime key on any sprite or palette of this table,
        # else a DL key on the sprite, else the MObjSub tile size.
        dims = None
        for s in sprites + [p for p in palettes if p]:
            if s in observed_dims:
                dims = observed_dims[s]
                break
        if dims is None:
            for s in sprites:
                dl = [(k[2], k[3]) for k in out if k[0] == s]
                if dl:
                    dims = dl[0]
                    break
        if dims is None:
            o = kind.obj_at(*sprites[0])
            obj_bytes = o.size if o else 0
            cand = [t for t in templates if t['load16'] * 2 == obj_bytes]
            if cand:
                dims = (cand[0]['ds_w'], cand[0]['ds_h'])
        if dims is None and 1 <= recs[0]['tw'] <= 128 and 1 <= recs[0]['th'] <= 128:
            dims = (reach.next_pow2(recs[0]['tw']), reach.next_pow2(recs[0]['th']))
        if dims is None:
            o = kind.obj_at(*sprites[0])
            texels = (o.size * 2) if o else 64
            side = 8
            while side * side < texels and side < 128:
                side <<= 1
            dims = (side, max(8, reach.next_pow2(texels // side)))
            dims = (min(dims[0], 128), min(dims[1], 128))
        fmt = 'Pal16' if (recs[0]['fmt'] == 2 and recs[0]['siz'] == 0) else \
            reach.ds_format(kind, sprites[0], recs[0]['fmt'], recs[0]['siz'], dims[0], dims[1])
        for s in sprites:
            for p in palettes:
                key = (s, p, dims[0], dims[1])
                if (s, None, dims[0], dims[1]) in out and p is None:
                    continue
                out[key] = dict(src='MObj', fmt=fmt, w=dims[0], h=dims[1], mobj=mobj_id)
            # a palette-as-image entry (Kirby body: the recorded image is the
            # costume palette itself) -- same dims, one per palette
        if recs[0]['fmt'] == 2 and len(palettes) > 1:
            for p in palettes:
                if p is not None and p in observed_dims:
                    out[(p, p, dims[0], dims[1])] = dict(src='MObj-palette', fmt=fmt, w=dims[0],
                                                         h=dims[1], mobj=mobj_id)
    return out, templates


def kirby_hat_roots(kind):
    """{copied kind: set of (fid, off) DL roots and MObjSub tables of that hat}."""
    fid = KIRBY_MAIN_FILE
    roots = {}
    for copied, mp in HAT_MP.items():
        rs = set()
        for detail in (0, 1):
            slot = KIRBY_JOINT6_DESC + (mp * 2 + detail) * FTMODELPART
            dl = kind.ptr(fid, slot)
            ms = kind.ptr(fid, slot + 4)
            if dl:
                rs.add(('dl', dl))
            if ms:
                rs.add(('mobj', ms))
        roots[copied] = rs
    return roots


def hat_keys(kind, roots):
    """Texture keys reachable from one hat's DL roots (fresh state)."""
    found = set()

    def emit(state, dl):
        tile = state['tiles'].get(state['tex_tile'], {})
        load = state['tmem'].get(tile.get('tmem', 0))
        if 'fmt' not in tile or load is None or load[0] is None:
            return
        found.add(tuple(load[0][0]) if load[0][0] else None)
    for kindname, ref in roots:
        if kindname == 'dl':
            st = {'timg': None, 'tiles': {}, 'tmem': {}, 'tlut': None, 'tex_on': True, 'tex_tile': 0}
            reach.run_dl(kind, ref[0], ref[1], st, emit, 0, set())
        else:
            # MObjSub** list: every MObjSub's sprites
            t = kind.obj_at(*ref)
            n = (t.size // 4) if t else 1
            for i in range(n):
                m = kind.ptr(ref[0], ref[1] + 4 * i)
                if not m:
                    continue
                spr = kind.ptr(m[0], m[1] + 4)
                pal = kind.ptr(m[0], m[1] + 0x2C)
                for tbl in (spr, pal):
                    if not tbl:
                        continue
                    tt = kind.obj_at(*tbl)
                    nn = (tt.size // 4) if tt else 1
                    for j in range(nn):
                        p = kind.ptr(tbl[0], tbl[1] + 4 * j)
                        if p:
                            found.add(tuple(p))
    found.discard(None)
    return found


def load_runtime(arm):
    runtime = json.load(open(os.path.join(ART, arm + '-keys.json')))
    spans_by_asset = {}
    for name, num in (('Donkey', '02'), ('Samus', '03'), ('Link', '05'), ('Kirby', '08')):
        p = reach.fpc.decode_pack(open(os.path.join(reach.PACKS, num + '.fpc'), 'rb').read())
        for (aid, doff, dbytes, src, first, count, roff, rcnt) in p['sections']:
            spans_by_asset[aid] = p['spans'][first:first + count]

    def to_source(ref):
        if not isinstance(ref, list):
            return None
        aid, off = ref
        for old, new, size in spans_by_asset.get(aid, ()):
            if new <= off < new + size:
                return (aid, old + (off - new))
        return None
    per_kind = {}
    for slot, name in enumerate(('Donkey', 'Samus', 'Link', 'Kirby')):
        rows = []
        for rk in runtime['slots'][slot]['keys']:
            rows.append(dict(img=to_source(rk[0]), tlut=to_source(rk[1]), w=rk[4], h=rk[5],
                             bytes=rk[6], pal=rk[7], uploads=rk[8], dsfmt=rk[3]))
        per_kind[name] = rows
    return per_kind


def main():
    arm = sys.argv[1] if len(sys.argv) > 1 else 'census-b4'
    runtime = load_runtime(arm)
    types = reach.est.TypeTable()
    types.load_dirs(reach.est.HEADER_DIRS)
    result = {}
    kirby_hats = {}
    for name in KINDS:
        idx, _ = reach.est.index_closure(name, types)
        kind = reach.Kind(name, idx)
        observed = {}
        for r in runtime.get(name, ()):
            if r['img']:
                observed[r['img']] = (r['w'], r['h'])
        keys, templates = enumerate_kind(name, kind, observed)
        model = MODEL_FILE[name]
        hat_of = {}
        if name == 'Kirby':
            roots = kirby_hat_roots(kind)
            for copied, rs in roots.items():
                for img in hat_keys(kind, rs):
                    hat_of.setdefault(img, set()).add(copied)
        rows = []
        for (img, pal, w, h), rec in keys.items():
            cls = 'model' if (img and img[0] == model) else 'other-file'
            hats = sorted(hat_of.get(img, ()))
            if hats:
                cls = 'hat'
            rows.append(dict(img=img, pal=pal, w=w, h=h, fmt=rec['fmt'], src=rec['src'], cls=cls,
                             hats=hats, bytes=tex_bytes(w, h, rec['fmt']),
                             palbytes=32 if rec['fmt'] == 'Pal16' else 0))
        # cross-check
        missing = []
        for r in runtime.get(name, ()):
            hit = [x for x in rows if x['img'] == r['img'] and (x['w'], x['h']) == (r['w'], r['h'])]
            if not hit:
                hit = [x for x in rows if x['img'] == r['img']]
                missing.append((r, 'image enumerated, dims differ %s' % [(x['w'], x['h']) for x in hit])
                               if hit else (r, 'NOT ENUMERATED'))
        result[name] = dict(rows=rows, missing=missing, templates=len(templates))
    # report
    print('%-8s %6s %9s %9s %9s %9s | %7s %8s | %s' % ('kind', 'keys', 'model B', 'other B', 'hats B',
                                                       'pal B', 'rt keys', 'rt miss', 'observed B'))
    summary = {}
    for name in KINDS:
        rows = result[name]['rows']
        mb = sum(r['bytes'] for r in rows if r['cls'] == 'model')
        ob = sum(r['bytes'] for r in rows if r['cls'] == 'other-file')
        hb = sum(r['bytes'] for r in rows if r['cls'] == 'hat')
        pb = sum(r['palbytes'] for r in rows)
        rt = runtime.get(name, [])
        obs = sum(r['bytes'] for r in rt)
        summary[name] = dict(keys=len(rows), model=mb, other=ob, hats=hb, pal=pb,
                             runtime_keys=len(rt), runtime_missing=len(result[name]['missing']),
                             observed=obs)
        print('%-8s %6d %9s %9s %9s %9s | %7s %8s | %s' % (
            name, len(rows), '{:,}'.format(mb), '{:,}'.format(ob), '{:,}'.format(hb), '{:,}'.format(pb),
            len(rt) if rt else '-', result[name]['missing'] and len(result[name]['missing']) or (0 if rt else '-'),
            '{:,}'.format(obs) if rt else '-'))
    for name in KINDS:
        for r, why in result[name]['missing']:
            print('  MISS', name, r['img'], r['tlut'], r['w'], r['h'], '->', why)
    # Kirby hats per copied kind
    hats = collections.defaultdict(int)
    for r in result['Kirby']['rows']:
        for h in r['hats']:
            hats[h] += r['bytes']
    print('Kirby hat DS bytes per copied kind:', dict(hats))
    json.dump(dict(summary=summary, kirby_hats=dict(hats),
                   rows={k: [dict(r, img=list(r['img']) if r['img'] else None,
                                  pal=list(r['pal']) if r['pal'] else None) for r in v['rows']]
                         for k, v in result.items()}),
              open(os.path.join(ART, 'reachable-set.json'), 'w'), indent=1)
    return summary, dict(hats)


if __name__ == '__main__':
    main()
