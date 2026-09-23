#!/usr/bin/env python3
"""MF experiment, candidate A: per-kind shared dictionary + LZ4-class tokens.

Format (one clip, decoded against the kind's resident dictionary D):
    repeat:
      token u8        hi nibble = literal count, lo nibble = match length - MIN
                      (15 in either nibble = extended by 255-run bytes, LZ4 style)
      [ext bytes]
      literals        literal count units, copied verbatim
      offset u16 LE   distance back from the current output position, in
                      units; a distance beyond the output start continues into
                      D's tail (the dictionary is a virtual prefix)
      [ext bytes]     match-length extension
    the last sequence carries literals only (its token's lo nibble is 0 and no
    offset follows); the decoder stops at the clip's decoded size, which the
    per-kind directory stores.
`unit` = 1 (byte LZ4) or 2 (halfword LZ: every BPS1 field is 2 B aligned, so
lengths and offsets count u16s and MIN = 2 units = 4 bytes).

Dictionary: zstd-COVER-style greedy segment selection over the kind's clips
(k-byte segments scored by the sample frequency of the distinct d-mers they
add), most valuable segment last (nearest the data).

Parsing is lazy-greedy over hash chains (depth 64): an optimal parse would
gain a few percent, which the verdict does not depend on (see README §3).

Usage:
  python scripts/motion/mf_candidate_a.py [--kinds k1,k2] [--dicts 8,16,32]
         [--unit 1|2|both] [--json OUT] [--clips-json OUT]
"""

from __future__ import annotations

import argparse
import collections
import json
import pathlib
import statistics
import sys
import time

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import mf_corpus as mc  # noqa: E402

MIN_BYTES = 4


# ------------------------------------------------------------- dictionary
def cover_dict(samples, size, k=256, d=8, step=None, unit=2):
    """Greedy COVER (Liu & Cheung / zstd): pick the k-byte segment whose
    not-yet-covered d-mers have the highest total sample frequency."""
    step = step or k // 4
    freq = collections.Counter()
    for s in samples:
        freq.update({s[i:i + d] for i in range(0, len(s) - d + 1, unit)})
    cands = []
    for si, s in enumerate(samples):
        for st in range(0, max(1, len(s) - k + 1), step):
            seg = s[st:st + k]
            dm = {seg[i:i + d] for i in range(0, len(seg) - d + 1, unit)}
            cands.append([sum(freq[x] for x in dm), si, st, dm])
    chosen, total, used = [], 0, set()
    while total < size and cands:
        # lazy re-scoring: a stale score is an upper bound
        cands.sort(key=lambda c: -c[0])
        while True:
            top = cands[0]
            fresh = sum(freq[x] for x in top[3] - used)
            if fresh >= (cands[1][0] if len(cands) > 1 else -1):
                break
            top[0] = fresh
            cands.sort(key=lambda c: -c[0])
        top = cands.pop(0)
        seg = samples[top[1]][top[2]:top[2] + k]
        used |= top[3]
        chosen.append(seg)
        total += len(seg)
    blob = b"".join(reversed(chosen))
    if len(blob) > size:
        blob = blob[len(blob) - size:]
    if unit == 2 and (len(blob) & 1):
        blob = blob[1:]
    return blob


# ------------------------------------------------------------------ codec
def _match_len(buf, i, j, limit):
    n = 0
    while n + 32 <= limit and buf[i + n:i + n + 32] == buf[j + n:j + n + 32]:
        n += 32
    while n < limit and buf[i + n] == buf[j + n]:
        n += 1
    return n


class DictIndex:
    """Hash chains over the dictionary, built once and cloned per clip."""

    def __init__(self, dict_bytes, unit):
        self.d = dict_bytes
        self.unit = unit
        self.head = {}
        self.prev = {}
        for i in range(0, len(dict_bytes) - 3, unit):
            h = dict_bytes[i:i + 4]
            if h in self.head:
                self.prev[i] = self.head[h]
            self.head[h] = i


def _ext(n, out):
    while n >= 255:
        out.append(255)
        n -= 255
    out.append(n)


def compress(index: DictIndex, data: bytes, depth=64):
    unit = index.unit
    D = index.d
    base = len(D)
    buf = D + data
    head = dict(index.head)
    prev = dict(index.prev)
    minu = MIN_BYTES // unit
    n = len(buf)
    out = bytearray()
    lit_start = base
    i = base

    def best_at(pos):
        if pos + MIN_BYTES > n:
            return 0, 0
        h = buf[pos:pos + 4]
        j = head.get(h)
        best_len, best_off = 0, 0
        chain = 0
        limit = n - pos
        while j is not None and chain < depth:
            dist = pos - j
            if dist // unit > 0xFFFF:
                break
            L = _match_len(buf, pos, j, limit)
            if unit == 2:
                L &= ~1
            if L > best_len:
                best_len, best_off = L, dist
                if L == limit:
                    break
            j = prev.get(j)
            chain += 1
        return best_len, best_off

    inserted = [base]       # every position below this is in the chains

    def insert(pos):
        if pos < inserted[0] or pos + 4 > n:
            return
        h = buf[pos:pos + 4]
        if h in head:
            prev[pos] = head[h]
        head[h] = pos
        inserted[0] = pos + unit

    def emit(lit_bytes, mlen_bytes, off_bytes):
        ll = len(lit_bytes) // unit
        ml = (mlen_bytes // unit - minu) if mlen_bytes else 0
        tok = (min(ll, 15) << 4) | (min(ml, 15) if mlen_bytes else 0)
        out.append(tok)
        if ll >= 15:
            _ext(ll - 15, out)
        out.extend(lit_bytes)
        if mlen_bytes:
            o = off_bytes // unit
            out.append(o & 0xFF)
            out.append(o >> 8)
            if ml >= 15:
                _ext(ml - 15, out)

    while i < n:
        L, off = best_at(i)
        if L >= MIN_BYTES:
            # lazy step: a longer match one unit later wins
            if i + unit < n:
                insert(i)
                L2, off2 = best_at(i + unit)
                if L2 > L + unit:
                    i += unit
                    L, off = L2, off2
            emit(buf[lit_start:i], L, off)
            end = i + L
            while i < end:
                insert(i)
                i += unit
            lit_start = i
        else:
            insert(i)
            i += unit
    emit(buf[lit_start:n], 0, 0)
    return bytes(out)


def decompress(dict_bytes, comp, size, unit):
    """Reference decoder: exactly what the ARM9 loop does (README §5)."""
    D = dict_bytes
    out = bytearray()
    p = 0
    minu = MIN_BYTES // unit
    while True:
        tok = comp[p]
        p += 1
        ll = tok >> 4
        if ll == 15:
            while True:
                b = comp[p]
                p += 1
                ll += b
                if b != 255:
                    break
        out += comp[p:p + ll * unit]
        p += ll * unit
        if len(out) >= size:
            break
        off = (comp[p] | (comp[p + 1] << 8)) * unit
        p += 2
        ml = tok & 15
        if ml == 15:
            while True:
                b = comp[p]
                p += 1
                ml += b
                if b != 255:
                    break
        ml = (ml + minu) * unit
        src = len(out) - off
        for _ in range(ml):
            out.append(D[len(D) + src] if src < 0 else out[src])
            src += 1
    if p != len(comp):
        raise ValueError("trailing bytes")
    return bytes(out)


def run(kinds, dict_kb, units, log=print, clips_out=None):
    c = mc.load_corpus(log=log)
    clips = c["clips"]
    banks = collections.defaultdict(list)
    for aid in sorted(clips):
        banks[clips[aid]["bank"]].append(aid)
    results = {}
    for kind in kinds:
        ids = banks[kind]
        samples = [clips[a]["bytes"] for a in ids]
        raw = sum(map(len, samples))
        for unit in units:
            for kb in [0] + list(dict_kb):
                t0 = time.time()
                D = cover_dict(samples, kb * 1024, unit=unit) if kb else b""
                idx = DictIndex(D, unit)
                sizes = []
                for a, s in zip(ids, samples):
                    comp = compress(idx, s)
                    back = decompress(D, comp, len(s), unit)
                    if back != s:
                        raise SystemExit("ROUND TRIP FAILED %s 0x%x unit %d dict %d"
                                         % (kind, a, unit, kb))
                    sizes.append(len(comp))
                    if clips_out is not None:
                        clips_out.setdefault("u%d/d%d" % (unit, kb), {})[
                            "0x%x" % a] = {"kind": kind, "raw": len(s),
                                           "comp": len(comp)}
                tot = sum(sizes)
                key = "%s/u%d/d%d" % (kind, unit, kb)
                q = sorted(sizes)
                results[key] = {
                    "raw": raw, "clips": len(ids), "dict": len(D),
                    "comp": tot, "ratio_ex_dict": tot / raw,
                    "ratio_incl_dict": (tot + len(D)) / raw,
                    "clip_min": q[0], "clip_median": statistics.median(q),
                    "clip_p95": q[int(0.95 * (len(q) - 1))], "clip_max": q[-1],
                    "per_clip_ratio_median": statistics.median(
                        cs / len(s) for cs, s in zip(sizes, samples)),
                    "roundtrip": "%d/%d" % (len(ids), len(ids)),
                    "seconds": round(time.time() - t0, 1)}
                r = results[key]
                log("%-8s unit %d dict %2d KB: %7d -> %7d + %5d  ratio %.3f (ex dict %.3f)"
                    "  clip med/p95/max %d/%d/%d  rt %s  %.0fs" % (
                        kind, unit, kb, raw, tot, len(D), r["ratio_incl_dict"],
                        r["ratio_ex_dict"], r["clip_median"], r["clip_p95"],
                        r["clip_max"], r["roundtrip"], r["seconds"]))
    return results


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--kinds", default=",".join(mc.KINDS))
    ap.add_argument("--dicts", default="8,16,32")
    ap.add_argument("--unit", default="both")
    ap.add_argument("--json", type=pathlib.Path)
    ap.add_argument("--clips-json", type=pathlib.Path)
    a = ap.parse_args()
    units = (1, 2) if a.unit == "both" else (int(a.unit),)
    per_clip = {}
    res = run(a.kinds.split(","), [int(x) for x in a.dicts.split(",") if x],
              units, clips_out=per_clip)
    if a.json:
        a.json.write_text(json.dumps(res, indent=1, sort_keys=True))
    if a.clips_json:
        a.clips_json.write_text(json.dumps(per_clip, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
