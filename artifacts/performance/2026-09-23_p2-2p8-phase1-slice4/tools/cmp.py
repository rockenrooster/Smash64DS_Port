"""Compare a host-emitted packet (hostlist.emit) with a runtime dump."""
import sys

import hostlist as hl
import pkt

SKIP_TAGS = ('proj', 'mv', 'light', 'texgen', 'teximage', 'pltt', 'shade')


def host_cmds(pk, tags):
    out = []
    for first, op, n in pk.cmds:
        params = []
        for k in range(n):
            w = first + k
            params.append(tags.get(w, pk.words[w]))
        out.append((op, params, first))
    return out


def dump_cmds(words):
    return [(c, prm, pi) for (_ci, _b, pi, c, prm) in pkt.decode(words)]


def compare(pk, tags, words, verbose=True, limit=12):
    h = host_cmds(pk, tags)
    d = dump_cmds(words)
    i = j = 0
    mism = []
    uv_bad = 0
    while i < len(h) and j < len(d):
        hop, hp, hpi = h[i]
        dop, dp, dpi = d[j]
        if hop == hl.C_PLTT and dop != hl.C_PLTT and isinstance(hp[0], hl.Tag) and hp[0].info.get('optional'):
            i += 1
            continue
        if hop != dop:
            mism.append(('op', i, j, hex(hop), hex(dop), hpi, dpi))
            break
        for k, (hv, dv) in enumerate(zip(hp, dp)):
            if isinstance(hv, hl.Tag):
                if hv.kind in SKIP_TAGS:
                    continue
                mism.append(('tag', i, j, hv, hex(dv), hpi + k, dpi + k))
                continue
            if hv != dv:
                if hop == hl.C_TEXCOORD:
                    uv_bad += 1
                mism.append(('word', i, j, pkt.NAMES.get(hop, hex(hop)), hex(hv), hex(dv), hpi + k, dpi + k))
        i += 1
        j += 1
    tail = (len(h) - i, len(d) - j)
    if verbose:
        print('host cmds %d dump cmds %d; mismatches %d (uv %d); unconsumed %s' % (
            len(h), len(d), len(mism), uv_bad, tail))
        for m in mism[:limit]:
            print('   ', m)
    return mism, tail
