"""Slice 2a deliverable 2, with details: attribute every enumerated key to
HIGH / LOW through the fighter's own part tables (FTCommonPartContainer ->
DObjDesc display lists + MObjSub lists; FTModelPartDesc -> modelparts[mp][d]),
following G_DL calls; keys reached from neither are 'other' (weapons, items,
effects, skeleton, shield pose in the Special/other files)."""
import collections, itertools, json, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import reach  # noqa: E402
import reach2  # noqa: E402

ART = reach.ART
DOBJDESC = 44
FTMODELPART = 20
MAIN_FILE = {'Mario': 0xcb, 'Fox': 0xd1, 'Donkey': 0xd5, 'Samus': 0xd9, 'Luigi': 0xdd, 'Link': 0xe1,
             'Yoshi': 0xf7, 'Captain': 0xec, 'Kirby': 0xe5, 'Pikachu': 0xf3, 'Purin': 0xe9, 'Ness': 0xef}


def objs_of_type(kind, fid, tname):
    return [o for o in kind.objects.get(fid, ()) if o.type_name == tname]


def pointer_list(kind, ref, max_n=64):
    """Entries of a pointer array object starting at ref."""
    o = kind.obj_at(*ref)
    n = (o.size - (ref[1] - o.offset)) // 4 if o else 1
    out = []
    for i in range(min(n, max_n)):
        p = kind.ptr(ref[0], ref[1] + 4 * i)
        out.append(p)
    return out


def dl_roots_of(kind, ref):
    """A DObjDesc dl field may point at a Gfx or at a Gfx* list (pairs)."""
    o = kind.obj_at(*ref)
    if o is None:
        return set()
    if o.type_name == 'Gfx':
        return {tuple(ref)}
    roots = set()
    for p in pointer_list(kind, ref, 16):
        if p is None:
            continue
        q = kind.obj_at(*p)
        if q is not None and q.type_name == 'Gfx':
            roots.add(tuple(p))
    return roots


def mobjsubs_of(kind, ref):
    """MObjSub** list -> MObjSub offsets."""
    out = set()
    for p in pointer_list(kind, ref, 32):
        if p is None:
            continue
        q = kind.obj_at(*p)
        if q is not None and q.type_name == 'MObjSub':
            out.add((p[0], q.offset))
    return out


def detail_roots(kind, name):
    main = MAIN_FILE[name]
    roots = {0: set(), 1: set()}
    mobjs = {0: set(), 1: set()}
    # ftparam.c: HIGH draws commonparts[0]; LOW draws commonparts[1] for a
    # joint only when its LOW dl is non-NULL, else that joint falls back to
    # commonparts[0] (dl AND its MObjSubs).
    for c in objs_of_type(kind, main, 'FTCommonPartContainer'):
        dls = {}
        pms = {}
        for d in (0, 1):
            base = c.offset + 16 * d
            dd = kind.ptr(main, base)
            pm = kind.ptr(main, base + 4)
            dls[d] = []
            if dd:
                o = kind.obj_at(*dd)
                n = (o.size // DOBJDESC) if o else 0
                dls[d] = [kind.ptr(dd[0], dd[1] + DOBJDESC * i + 4) for i in range(n)]
            pms[d] = pointer_list(kind, pm, 64) if pm else []
        n = max(len(dls[0]), len(dls[1]))
        for j in range(n):
            dl0 = dls[0][j] if j < len(dls[0]) else None
            dl1 = dls[1][j] if j < len(dls[1]) else None
            ms0 = pms[0][j] if j < len(pms[0]) else None
            ms1 = pms[1][j] if j < len(pms[1]) else None
            if dl0:
                roots[0] |= dl_roots_of(kind, dl0)
            if ms0:
                mobjs[0] |= mobjsubs_of(kind, ms0)
            low_dl, low_ms = (dl1, ms1) if dl1 else (dl0, ms0)
            if low_dl:
                roots[1] |= dl_roots_of(kind, low_dl)
            if low_ms:
                mobjs[1] |= mobjsubs_of(kind, low_ms)
    for c in objs_of_type(kind, main, 'FTModelPartDesc'):
        for desc in pointer_list(kind, (main, c.offset), 64):
            if desc is None:
                continue
            o = kind.obj_at(*desc)
            if o is None:
                continue
            n = o.size // FTMODELPART
            for i in range(n):
                d = i % 2
                slot = desc[1] + FTMODELPART * i
                dl = kind.ptr(desc[0], slot)
                ms = kind.ptr(desc[0], slot + 4)
                if dl:
                    roots[d] |= dl_roots_of(kind, dl)
                if ms:
                    mobjs[d] |= mobjsubs_of(kind, ms)
    return roots, mobjs


def gfx_closure(kind, roots):
    seen = set()
    stack = list(roots)
    while stack:
        fid, off = stack.pop()
        o = kind.obj_at(fid, off)
        if o is None or o.type_name != 'Gfx':
            continue
        key = (fid, o.offset)
        if key in seen:
            continue
        seen.add(key)
        payload = kind.files[fid].source['payload']
        for pos in range(o.offset, o.offset + o.size, 8):
            if payload[pos] == 0xDE:
                t = kind.ptr(fid, pos + 4)
                if t:
                    stack.append(tuple(t))
    return seen


def main():
    arm = sys.argv[1] if len(sys.argv) > 1 else 'census-b4'
    runtime = reach2.load_runtime(arm)
    types = reach.est.TypeTable()
    types.load_dirs(reach.est.HEADER_DIRS)
    table = {}
    kirby = {}
    for name in reach2.KINDS:
        idx, _ = reach.est.index_closure(name, types)
        kind = reach.Kind(name, idx)
        observed = {r['img']: (r['w'], r['h']) for r in runtime.get(name, ()) if r['img']}
        keys, templates = reach2.enumerate_kind(name, kind, observed)
        roots, mobjs = detail_roots(kind, name)
        reach_dl = {d: gfx_closure(kind, roots[d]) for d in (0, 1)}
        rows = []
        for (img, pal, w, h), rec in keys.items():
            dets = set()
            if rec['src'] == 'DL':
                for d in (0, 1):
                    if any(tuple(x) in reach_dl[d] for x in rec['dls']):
                        dets.add(d)
            else:
                for d in (0, 1):
                    if tuple(rec['mobj']) in mobjs[d]:
                        dets.add(d)
            rows.append(dict(img=img, pal=pal, w=w, h=h, fmt=rec['fmt'], src=rec['src'],
                             details=sorted(dets), bytes=reach2.tex_bytes(w, h, rec['fmt']),
                             palbytes=32 if rec['fmt'] == 'Pal16' else 0))
        # Kirby hats: hat-specific = reached from a hat's roots only
        hat_only = {}
        if name == 'Kirby':
            hroots = reach2.kirby_hat_roots(kind)
            per_hat = {c: reach2.hat_keys(kind, rs) for c, rs in hroots.items()}
            common = set.intersection(*per_hat.values()) if per_hat else set()
            for c, ks in per_hat.items():
                for img in ks - common:
                    hat_only.setdefault(img, set()).add(c)
        for r in rows:
            r['hats'] = sorted(hat_only.get(r['img'], ()))
        # cross-check against runtime
        miss = []
        for r in runtime.get(name, ()):
            hit = [x for x in rows if x['img'] == r['img'] and (x['w'], x['h']) == (r['w'], r['h'])]
            det = sorted({d for x in hit for d in x['details']})
            if not hit:
                miss.append((r['img'], r['w'], r['h']))
            r['enum_details'] = det
        table[name] = rows
        low = [r for r in rows if 1 in r['details'] and not r['hats']]
        high = [r for r in rows if 0 in r['details'] and not r['hats']]
        other = [r for r in rows if not r['details'] and not r['hats']]
        hats = [r for r in rows if r['hats']]
        rt = runtime.get(name, [])
        rt_low = sum(1 for r in rt if 1 in r.get('enum_details', []))
        kirby[name] = dict(
            low_keys=len(low), low_bytes=sum(r['bytes'] for r in low), low_pal=sum(r['palbytes'] for r in low),
            high_keys=len(high), high_bytes=sum(r['bytes'] for r in high),
            other_keys=len(other), other_bytes=sum(r['bytes'] for r in other),
            hat_bytes={c: sum(r['bytes'] for r in hats if c in r['hats'])
                       for c in reach2.HAT_MP} if name == 'Kirby' else {},
            runtime_keys=len(rt), runtime_missing=miss, runtime_in_low=rt_low,
            observed_bytes=sum(r['bytes'] for r in rt),
            roots=(len(roots[0]), len(roots[1])), mobjs=(len(mobjs[0]), len(mobjs[1])))
    print('%-8s | %5s %9s | %5s %9s | %5s %9s | %s' % ('kind', 'LOWk', 'LOW B', 'HIGHk', 'HIGH B',
                                                     'othk', 'other B', 'runtime keys / missing / in LOW / observed B'))
    for name in reach2.KINDS:
        s = kirby[name]
        print('%-8s | %5d %9s | %5d %9s | %5d %9s | %s' % (
            name, s['low_keys'], '{:,}'.format(s['low_bytes']), s['high_keys'], '{:,}'.format(s['high_bytes']),
            s['other_keys'], '{:,}'.format(s['other_bytes']),
            ('%d / %d / %d / %s' % (s['runtime_keys'], len(s['runtime_missing']), s['runtime_in_low'],
                                   '{:,}'.format(s['observed_bytes']))) if s['runtime_keys'] else '-'))
        if s['runtime_missing']:
            print('     missing:', s['runtime_missing'])
    print('Kirby hat-only bytes:', kirby['Kirby']['hat_bytes'])
    print('roots/mobjs per detail:', {k: (v['roots'], v['mobjs']) for k, v in kirby.items()})
    json.dump(dict(summary=kirby, rows={k: [dict(r, img=list(r['img']) if r['img'] else None,
                                                  pal=list(r['pal']) if r['pal'] else None) for r in v]
                                         for k, v in table.items()}),
              open(os.path.join(ART, 'reachable-set.json'), 'w'), indent=1)


if __name__ == '__main__':
    main()
