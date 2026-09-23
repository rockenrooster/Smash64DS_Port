"""Structure decisions the host cannot make alone, as the runtime recorded
them: per (kind, root row, epoch) the tint route, per (kind, root row, run)
the palette word -- and whether any of them varies across the dumps."""
import collections
import sys

import hostlist as hl
import match
import pkt


class Logger(hl.GuidedDecider):
    def __init__(self, ref, log, key_fn):
        hl.GuidedDecider.__init__(self, ref)
        self.log = log
        self.key_fn = key_fn

    def tint(self, pk, ri, e, default):
        v = hl.GuidedDecider.tint(self, pk, ri, e, default)
        self.log.append(('tint', self.key_fn(), e, v, default))
        return v

    def pltt(self, pk, ri, r):
        v = hl.GuidedDecider.pltt(self, pk, ri, r)
        self.log.append(('pltt', self.key_fn(), r, v, None))
        return v


def main(labels=('r0',)):
    seen = collections.defaultdict(collections.Counter)
    ctxs = {}
    for label in labels:
        for p in pkt.dumps(label):
            kind = match.KIND_BY_SLOT[p['slot']]
            if kind not in ctxs:
                ctxs[kind] = hl.context(kind, 'low')
            res = match.match_dump(p, ctxs[kind])
            if not res['ok']:
                continue
            # replay the matched vector with a logging decider
            cands = match.candidates(ctxs[kind])
            cmds = match.dump_cmds(p['words'])
            log = []
            cur = {'key': None}
            em = hl.Emitter(ctxs[kind], decide=Logger([(c[0], c[1]) for c in cmds], log,
                                                      lambda: cur['key']))
            starts = match.blocks(cmds)
            for ri, ci in enumerate(res['roots']):
                rctx, rindex, row, light, _b = cands[ci]
                block = cmds[starts[ri]:]
                slot = 31
                for k in range(min(5, len(block))):
                    if block[k][0] == hl.C_MTX_STORE:
                        slot = block[k][1][0]
                        break
                rowkey = tuple(rctx['roots'][rindex]) if row is None else row
                cur['key'] = (id(rctx), rowkey)
                em.emit_root(ri, rindex, match.DEFAULT_PRE, None, slot, ctx=rctx,
                             row=row, light_index=light)
            for (what, key, idx, v, default) in log:
                seen[(kind, what, key, idx)][(v, default)] += 1
    varying = collections.Counter()
    totals = collections.Counter()
    nondefault = collections.Counter()
    for (kind, what, key, idx), c in sorted(seen.items(), key=lambda kv: str(kv[0])):
        totals[(kind, what)] += 1
        vals = set(v for (v, d) in c)
        if len(vals) > 1:
            varying[(kind, what)] += 1
            print('VARIES', kind, what, 'root', key[1][:3], 'idx', idx, dict(c))
        if what == 'tint' and any(v != d for (v, d) in c):
            nondefault[(kind, what)] += 1
    for k in sorted(totals):
        print(k, 'sites', totals[k], 'varying', varying[k],
              'tint!=default' if k[1] == 'tint' else '', nondefault.get(k, ''))
    # palette presence per run
    for kind in ('donkey', 'samus', 'link', 'kirby'):
        t = collections.Counter()
        for (k2, what, key, idx), c in seen.items():
            if k2 == kind and what == 'pltt':
                t[tuple(sorted(set(v for (v, d) in c)))] += 1
        print(kind, 'pltt value sets', dict(t))


if __name__ == '__main__':
    main(sys.argv[1:] or ('r0',))
