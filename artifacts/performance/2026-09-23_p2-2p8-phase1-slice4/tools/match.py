"""Match every runtime-recorded packet dump against the host emitter.

For each dump the root vector is recovered block by block: the dump is cut
at every projection load (MTX_MODE(0)), and for each block every context
root is tried from the running host state; a candidate matches when every
command and every unmasked word agrees.  Material-dependent structure (tint
route, palette word) is read from the dump (GuidedDecider); texture, shade,
matrix, light and texgen words are masked.
"""
import collections
import copy
import sys

import hostlist as hl
import pkt

KIND_BY_SLOT = {0: 'donkey', 1: 'samus', 2: 'link', 3: 'kirby'}
SKIP = ('proj', 'mv', 'light', 'texgen', 'teximage', 'pltt', 'shade')
DEFAULT_PRE = dict(geometry_mode=0x1 | 0x4 | 0x400 | 0x20000 | 0x200000,
                   cycle_type=1 << 20, render_mode=0xC4112078,
                   prim=0xffffffff, env=0xffffffff, light_valid=1,
                   light_dir=(0, 0, 0))


def dump_cmds(words):
    return [(c, prm, pi) for (_ci, _b, pi, c, prm) in pkt.decode(words)]


def blocks(cmds):
    starts = [k for k, (op, prm, _pi) in enumerate(cmds)
              if op == hl.C_MTX_MODE and prm[0] == 0]
    return starts


def cmd_equal(host, dump_cmd):
    hop, hparams = host
    dop, dparams = dump_cmd[0], dump_cmd[1]
    if hop != dop:
        return False
    for hv, dv in zip(hparams, dparams):
        if isinstance(hv, hl.Tag):
            if hv.kind in SKIP:
                continue
            return False
        if hv != dv:
            return False
    return True


def host_cmd_list(em, first_cmd):
    out = []
    for (first, op, n) in em.pk.cmds[first_cmd:]:
        out.append((op, [em.tags.get(first + k, em.pk.words[first + k]) for k in range(n)]))
    return out


def candidates(ctx):
    """Every executable root the kind can draw: its own rows, then every
    program root with the table set that owns it (mixed-file donors)."""
    out = []
    seen = set()
    for r in range(len(ctx['roots'])):
        key = (id(ctx), tuple(ctx['roots'][r]))
        if key not in seen:
            seen.add(key)
            out.append((ctx, r, None, None, ctx['root_bindings'][r]))
    for prog in ctx.get('root_programs', []):
        vcs = prog.get('verification_contexts') or [ctx] * len(prog['roots'])
        for k, row in enumerate(prog['roots']):
            rctx = vcs[k]
            key = (id(rctx), tuple(row))
            if key in seen:
                continue
            seen.add(key)
            out.append((rctx, None, tuple(row), prog['light_indices'][k], -1))
    return out


def match_dump(p, ctx, verbose=False):
    cmds = dump_cmds(p['words'])
    starts = blocks(cmds)
    ends = starts[1:] + [len(cmds)]
    em = hl.Emitter(ctx, decide=hl.GuidedDecider([(c[0], c[1]) for c in cmds]))
    chosen = []
    cands = candidates(ctx)
    for ri, (a, b) in enumerate(zip(starts, ends)):
        block = cmds[a:b]
        slot = 31
        for k in range(min(5, len(block))):
            if block[k][0] == hl.C_MTX_STORE:
                slot = block[k][1][0]
                break
        found = None
        best = (-1, None)
        order = sorted(range(len(cands)), key=lambda c: (cands[c][4] != ri, c))
        for ci in order:
            rctx, rindex, row, light, _b = cands[ci]
            trial = copy.deepcopy(em)
            n0 = len(trial.pk.cmds)
            try:
                trial.emit_root(ri, rindex, DEFAULT_PRE, None, slot, ctx=rctx,
                                row=row, light_index=light)
            except Exception:
                continue
            hc = host_cmd_list(trial, n0)
            k = 0
            while k < len(hc) and k < len(block) and cmd_equal(hc[k], block[k]):
                k += 1
            if k == len(hc) == len(block):
                found = (ci, trial)
                break
            if k > best[0]:
                best = (k, ci, len(hc), len(block))
        if found is None:
            return dict(ok=False, roots=chosen, fail_root=ri, best=best)
        chosen.append(found[0])
        em = found[1]
    return dict(ok=True, roots=chosen, emitter=em)


def main(labels=('r0',)):
    ctxs = {}
    stats = collections.Counter()
    vectors = collections.defaultdict(collections.Counter)
    for label in labels:
        for p in pkt.dumps(label):
            kind = KIND_BY_SLOT[p['slot']]
            if kind not in ctxs:
                ctxs[kind] = hl.context(kind, 'low')
            res = match_dump(p, ctxs[kind])
            stats[(kind, res['ok'])] += 1
            if res['ok']:
                vectors[kind][tuple(res['roots'])] += 1
            else:
                print('FAIL', label, 'n=%d' % p['n'], kind, 'wc', p['wc'], 'root', res['fail_root'],
                      'best', res['best'], 'so far', res['roots'])
    print(dict(stats))
    for kind, vs in vectors.items():
        print(kind, 'distinct root vectors', len(vs))
        for v, n in vs.most_common(6):
            print('   x%d' % n, v)


if __name__ == '__main__':
    main(sys.argv[1:] or ('r0',))
