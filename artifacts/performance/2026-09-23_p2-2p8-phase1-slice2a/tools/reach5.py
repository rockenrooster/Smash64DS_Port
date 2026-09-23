"""Slice 2a deliverable 2 (final): reachable fighter DS textures per kind and
detail, simulating what the draw would do WITHOUT drawing:

  for every joint of the detail (FTCommonPartContainer, LOW falling back to
  HIGH per joint exactly as ftparam.c does) and every model part
  (FTModelPartDesc modelparts[mp][detail]): run the joint's display list;
  where it calls a material segment (G_DL 0x0E000000 + 8*i) apply MObj i's
  branch (objdisplay.c: PALETTE -> SETTIMG(palette) [+LOADTLUT], FRAC|SPLIT
  -> SETTIMG(next sprite), FRAC|ALPHA -> SETTIMG(current sprite)) for every
  sprite (texture animation) and every palette (costume / palette animation).
  Each textured triangle yields (image, tlut, DS dims) by the port's rules.

Every Gfx array outside the part tables (weapons, items, effects in the
kind's other files) is scanned on its own and reported as 'other'.
Per-instance bytes count one palette per material (the instance's costume);
'all palettes' is the upper bound. Cross-checked against census-b4."""
import collections, itertools, json, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import reach  # noqa: E402
import reach2  # noqa: E402
import reach3  # noqa: E402

ART = reach.ART
BE2 = reach.BE2
MOBJ_ALPHA, MOBJ_SPLIT, MOBJ_PALETTE, MOBJ_FRAC = 1, 2, 4, 16


def mobj_info(kind, ref):
    fid, off = ref
    data = kind.files[fid].source['payload']
    fmt, siz = data[off + 2], data[off + 3]
    flags = struct.unpack_from('>H', data, off + 0x30)[0]
    bfmt, bsiz = data[off + 0x32], data[off + 0x33]
    spr = kind.ptr(fid, off + 4)
    pal = kind.ptr(fid, off + 0x2C)
    sprites = [p for p in reach3.pointer_list(kind, spr, 64) if p] if spr else []
    palettes = [p for p in reach3.pointer_list(kind, pal, 64) if p] if pal else []
    return dict(fmt=fmt, siz=siz, flags=flags, bfmt=bfmt, bsiz=bsiz,
                sprites=[tuple(p) for p in sprites], palettes=[tuple(p) for p in palettes])


def run(kind, fid, off, state, emit, depth, mobj_hook):
    f = kind.files.get(fid)
    o = kind.obj_at(fid, off)
    if f is None or o is None or o.type_name != 'Gfx':
        return
    payload = f.source['payload']
    pos = off
    end = o.offset + o.size
    while pos + 8 <= end:
        w0, w1 = BE2.unpack_from(payload, pos)
        op = w0 >> 24
        if op == 0xFD:
            state['timg'] = (kind.ptr(fid, pos + 4), (w0 >> 21) & 7, (w0 >> 19) & 3)
        elif op == 0xF5:
            t = (w1 >> 24) & 7
            tile = state['tiles'].setdefault(t, {})
            tile.update(fmt=(w0 >> 21) & 7, siz=(w0 >> 19) & 3, line=(w0 >> 9) & 0x1ff,
                        tmem=w0 & 0x1ff, cmt=(w1 >> 18) & 3, maskt=(w1 >> 14) & 15,
                        cms=(w1 >> 8) & 3, masks=(w1 >> 4) & 15)
        elif op == 0xF3:
            tile = state['tiles'].setdefault((w1 >> 24) & 7, {})
            state['tmem'][tile.get('tmem', 0)] = (state['timg'], ((w1 >> 12) & 0xfff) + 1)
        elif op == 0xF4:
            tile = state['tiles'].setdefault((w1 >> 24) & 7, {})
            uls, ult, lrs, lrt = (w0 >> 12) & 0xfff, w0 & 0xfff, (w1 >> 12) & 0xfff, w1 & 0xfff
            state['tmem'][tile.get('tmem', 0)] = (state['timg'],
                                                  (((lrs - uls) >> 2) + 1) * (((lrt - ult) >> 2) + 1))
        elif op == 0xF0:
            state['tlut'] = state['timg'][0] if state['timg'] else None
        elif op == 0xF2:
            tile = state['tiles'].setdefault((w1 >> 24) & 7, {})
            uls, ult, lrs, lrt = (w0 >> 12) & 0xfff, w0 & 0xfff, (w1 >> 12) & 0xfff, w1 & 0xfff
            tile['w'] = ((lrs - uls) >> 2) + 1
            tile['h'] = ((lrt - ult) >> 2) + 1
        elif op == 0xD7:
            state['tex_on'] = (w0 & 0xff) != 0
            state['tex_tile'] = (w0 >> 8) & 7
        elif op in (0x05, 0x06, 0x07):
            if state['tex_on']:
                emit(state)
        elif op == 0xDE:
            tgt = kind.ptr(fid, pos + 4)
            branch = ((w0 >> 16) & 0xff) != 0
            if tgt is None and (w1 >> 24) == 0x0E and mobj_hook is not None:
                mobj_hook(state, (w1 & 0xffffff) // 8)
            elif tgt is not None and depth < 24:
                run(kind, tgt[0], tgt[1], state, emit, depth + 1, mobj_hook)
            if branch:
                return
        elif op == 0xDF:
            return
        pos += 8


def key_of(kind, state):
    tile = state['tiles'].get(state['tex_tile'], {})
    if 'fmt' not in tile:
        return None
    load = state['tmem'].get(tile.get('tmem', 0))
    if load is None or load[0] is None or load[0][0] is None:
        return None
    timg, load16 = load
    fmt, siz = tile['fmt'], tile['siz']
    dims = reach.ds_dims(fmt, siz, tile.get('line', 0), tile.get('w', 0), tile.get('h', 0), load16,
                         tile.get('cms', 0), tile.get('cmt', 0), tile.get('masks', 0), tile.get('maskt', 0))
    if dims is None:
        return None
    img = tuple(timg[0])
    tlut = tuple(state['tlut']) if (fmt == 2 and state['tlut']) else None
    dsfmt = reach.ds_format(kind, img, fmt, siz, dims[2], dims[3])
    return (img, tlut, dims[0], dims[1], dsfmt)


def pairs_for_detail(kind, name, d):
    """[(dl root, [mobj refs])] for one detail, ftparam.c selection rules."""
    main = reach3.MAIN_FILE[name]
    out = []
    for c in reach3.objs_of_type(kind, main, 'FTCommonPartContainer'):
        dls, pms = {}, {}
        for dd in (0, 1):
            base = c.offset + 16 * dd
            desc = kind.ptr(main, base)
            pm = kind.ptr(main, base + 4)
            dls[dd] = []
            if desc:
                o = kind.obj_at(*desc)
                n = (o.size // reach3.DOBJDESC) if o else 0
                dls[dd] = [kind.ptr(desc[0], desc[1] + reach3.DOBJDESC * i + 4) for i in range(n)]
            pms[dd] = reach3.pointer_list(kind, pm, 64) if pm else []
        for j in range(max(len(dls[0]), len(dls[1]))):
            dl0 = dls[0][j] if j < len(dls[0]) else None
            dl1 = dls[1][j] if j < len(dls[1]) else None
            ms0 = pms[0][j] if j < len(pms[0]) else None
            ms1 = pms[1][j] if j < len(pms[1]) else None
            dl, ms = (dl0, ms0) if (d == 0 or not dl1) else (dl1, ms1)
            if dl:
                mobjs = sorted(reach3.mobjsubs_of(kind, ms)) if ms else []
                for r in reach3.dl_roots_of(kind, dl):
                    out.append((r, mobjs, 'joint%d' % j))
    for c in reach3.objs_of_type(kind, main, 'FTModelPartDesc'):
        for ji, desc in enumerate(reach3.pointer_list(kind, (main, c.offset), 64)):
            if desc is None:
                continue
            o = kind.obj_at(*desc)
            if o is None:
                continue
            for i in range(o.size // reach3.FTMODELPART):
                if i % 2 != d:
                    continue
                slot = desc[1] + reach3.FTMODELPART * i
                dl = kind.ptr(desc[0], slot)
                ms = kind.ptr(desc[0], slot + 4)
                if dl:
                    mobjs = sorted(reach3.mobjsubs_of(kind, ms)) if ms else []
                    for r in reach3.dl_roots_of(kind, dl):
                        out.append((r, mobjs, 'mp%d.%d' % (ji, i // 2)))
    return out


def enumerate_detail(kind, name, d):
    keys = {}   # (img, tlut, w, h) -> dict(fmt, where, per_instance)
    for root, mobjs, where in pairs_for_detail(kind, name, d):
        infos = [mobj_info(kind, m) for m in mobjs]
        # choices: every sprite index and every palette index of each material
        n_spr = max([len(i['sprites']) for i in infos] + [1])
        n_pal = max([len(i['palettes']) for i in infos] + [1])
        for si in range(n_spr):
            for pi in range(n_pal):

                def hook(state, idx):
                    if idx >= len(infos):
                        return
                    info = infos[idx]
                    fl = info['flags']
                    spr = info['sprites']
                    pal = info['palettes']
                    if fl & MOBJ_PALETTE and pal:
                        p = pal[min(pi, len(pal) - 1)]
                        state['timg'] = (p, 0, 2)
                        if fl & (MOBJ_SPLIT | MOBJ_ALPHA):
                            state['tlut'] = p
                    if fl & (MOBJ_FRAC | MOBJ_SPLIT) and spr:
                        state['timg'] = (spr[min(si + 1, len(spr) - 1)], info['bfmt'], info['bsiz'])
                    if fl & (MOBJ_FRAC | MOBJ_ALPHA) and spr:
                        state['timg'] = (spr[min(si, len(spr) - 1)], info['fmt'], info['siz'])

                def emit(state):
                    k = key_of(kind, state)
                    if k is None:
                        return
                    rec = keys.setdefault(k[:4], dict(fmt=k[4], where=set(), pal_index=set()))
                    rec['where'].add(where)
                    rec['pal_index'].add(pi)
                st = {'timg': None, 'tiles': {}, 'tmem': {}, 'tlut': None, 'tex_on': True, 'tex_tile': 0}
                run(kind, root[0], root[1], st, emit, 0, hook)
    return keys


def main():
    arm = sys.argv[1] if len(sys.argv) > 1 else 'census-b4'
    runtime = reach2.load_runtime(arm)
    types = reach.est.TypeTable()
    types.load_dirs(reach.est.HEADER_DIRS)
    summary = {}
    allrows = {}
    for name in reach2.KINDS:
        idx, _ = reach.est.index_closure(name, types)
        kind = reach.Kind(name, idx)
        per_d = {d: enumerate_detail(kind, name, d) for d in (0, 1)}
        # other: every Gfx outside the part-table closure (weapons, items, effects)
        part_roots = set()
        for d in (0, 1):
            for r, _, _ in pairs_for_detail(kind, name, d):
                part_roots.add(r)
        part_closure = reach3.gfx_closure(kind, part_roots)
        other = {}
        dlkeys, _, _ = reach.enumerate_kind(kind)
        for k, rec in dlkeys.items():
            if not any(tuple(x) in part_closure for x in rec['dls']):
                fmt = reach.ds_format(kind, rec['img'], rec['fmt'], rec['siz'], rec['src_w'], rec['src_h'])
                other[(rec['img'], rec['ds_w'], rec['ds_h'])] = fmt

        def budget(keys, one_palette):
            texels = pal = n = 0
            seen = set()
            for (img, tlut, w, h), rec in keys.items():
                if one_palette and rec['pal_index'] and 0 not in rec['pal_index']:
                    continue
                k = (img, tlut, w, h)
                if k in seen:
                    continue
                seen.add(k)
                texels += reach2.tex_bytes(w, h, rec['fmt'])
                pal += 32 if rec['fmt'] == 'Pal16' else 0
                n += 1
            return texels, pal, n
        # Kirby hats: joint-6 model parts 3..13 (mp index 3..13 of the joint-6 desc)
        hat = {}
        if name == 'Kirby':
            for copied, mp in reach2.HAT_MP.items():
                ks = {k: v for k, v in per_d[1].items() if any(w.endswith('.%d' % mp) and w.startswith('mp') for w in v['where'])}
                base = {k for k, v in per_d[1].items() if any(not (w.startswith('mp') and w.split('.')[1] in [str(x) for x in reach2.HAT_MP.values()]) for w in v['where'])}
                only = {k: v for k, v in ks.items() if k not in base}
                hat[copied] = budget(only, True)
        low_all = budget(per_d[1], False)
        low_inst = budget(per_d[1], True)
        high_inst = budget(per_d[0], True)
        if name == 'Kirby':
            # the instance draws its own head; hats are added per present opponent
            hat_keys = set()
            for copied, mp in reach2.HAT_MP.items():
                hat_keys |= {k for k, v in per_d[1].items() if all(w.startswith('mp') and w.split('.')[1] in [str(x) for x in reach2.HAT_MP.values()] for w in v['where'])}
            low_inst = budget({k: v for k, v in per_d[1].items() if k not in hat_keys}, True)
        ot = (sum(reach2.tex_bytes(w, h, f) for (i, w, h), f in other.items()), len(other))
        # cross-check
        rt = runtime.get(name, [])
        miss_low = []
        for r in rt:
            hit = [k for k in per_d[1] if k[0] == r['img'] and (k[2], k[3]) == (r['w'], r['h'])]
            if not hit:
                miss_low.append((r['img'], r['tlut'], r['w'], r['h']))
        summary[name] = dict(low_instance=low_inst, low_all_palettes=low_all, high_instance=high_inst,
                             other=ot, hats=hat, runtime_keys=len(rt),
                             runtime_bytes=sum(r['bytes'] for r in rt), runtime_not_in_low=miss_low)
        allrows[name] = {'low': [[list(k[0]), list(k[1]) if k[1] else None, k[2], k[3], v['fmt'],
                                  sorted(v['where']), sorted(v['pal_index'])] for k, v in per_d[1].items()],
                         'high': [[list(k[0]), list(k[1]) if k[1] else None, k[2], k[3], v['fmt'],
                                   sorted(v['where'])] for k, v in per_d[0].items()]}
    print('%-8s %22s %22s %22s %16s | %s' % ('kind', 'LOW/instance B,pal,n', 'LOW/all-pal B,pal,n',
                                            'HIGH/instance B,pal,n', 'other B,n', 'runtime keys, B, not-in-LOW'))
    for name in reach2.KINDS:
        s = summary[name]
        f = lambda t: '{:,}/{:,}/{}'.format(*t)
        print('%-8s %22s %22s %22s %16s | %s' % (
            name, f(s['low_instance']), f(s['low_all_palettes']), f(s['high_instance']),
            '{:,}/{}'.format(*s['other']),
            ('%d, %s, %d' % (s['runtime_keys'], '{:,}'.format(s['runtime_bytes']), len(s['runtime_not_in_low'])))
            if s['runtime_keys'] else '-'))
        for m in s['runtime_not_in_low']:
            print('        runtime key not in LOW enumeration:', m)
    print('Kirby hats (LOW, hat-only, per copied kind):', {k: v for k, v in summary['Kirby']['hats'].items()})
    json.dump(dict(summary=summary, rows=allrows), open(os.path.join(ART, 'reachable-final.json'), 'w'),
              indent=1, default=list)
    return summary


if __name__ == '__main__':
    main()
