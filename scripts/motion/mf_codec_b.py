#!/usr/bin/env python3
"""MF experiment, candidate B: structural re-encoding + static Huffman.

The AObj16 stream is re-expressed as the symbols the pose parser consumes,
each class coded with its own static canonical Huffman table (resident once
per kind, or once for all kinds), and decoded back to the exact BPS1 image.

Symbol order per clip (the decoder's order; one MSB-first bitstream):
  NSLOT                         slot count
  SLOT x nslot                  0 NULL | 1 next new run | 1+d back d | -g skip g
  per run, in offset order:
    CMD (ctx: previous word)    -> u16 command word. rich/lean: a Huffman
                                   table per frequent previous word, ESC ->
                                   CMD0 order-0 word index. fast: CRANK = rank
                                   in the previous word's successor list (one
                                   shared table), -1 -> CRANK0 order-0 index.
    Loop/TraI: JUMP             raw 16 bits
    toggle: PAY (ctx: opcode)   u16 payload (frame count)
    per selected track t:
      ops 2,3,7,8,9,10,4,5: VD  value - last value of t in this run (s16 wrap)
      ops 4,5 also:        RT   rate - pred (pred: 0, or slope from VD/PAY)
      op 6:                RT6  rate - last rate of t
    after End/Loop: TAIL        0 none | m: 2m zero bytes | -m-1: m raw u16
Values use hybrid alphabets: literals for frequent values (per context) plus
17 JPEG-style magnitude categories as escapes (category, then `cat` raw bits).

Every table's serialised size (Table.cost_bits, the word list, the successor
lists) is charged so ratios include the model, the way candidate A includes
its dictionary. The round trip compares the decoder's bytes against the BPS1
image of every clip; any mismatch aborts the run.

Usage:
  python scripts/motion/mf_codec_b.py [--kinds ...] [--scope kind|global]
       [--cfg rich|lean|fast] [--rate zero|slope] [--json OUT]
       [--clips-json OUT]
"""

from __future__ import annotations

import argparse
import collections
import heapq
import json
import math
import pathlib
import statistics
import struct
import sys
import time

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import mf_aobj16 as ma  # noqa: E402
import mf_corpus as mc  # noqa: E402

MAXLEN = 12           # code length limit (decoder: 8-bit lookup + slow path)
NCAT = 17             # categories 0..16
PER = ma.PER_TRACK


# ----------------------------------------------------------------- bits
class BitWriter:
    def __init__(self):
        self.out = bytearray()
        self.acc = 0
        self.n = 0
        self.bits = 0

    def put(self, code, length):
        if length == 0:
            return
        self.acc = (self.acc << length) | code
        self.n += length
        self.bits += length
        while self.n >= 8:
            self.n -= 8
            self.out.append((self.acc >> self.n) & 0xFF)
        self.acc &= (1 << self.n) - 1

    def done(self):
        if self.n:
            self.out.append((self.acc << (8 - self.n)) & 0xFF)
            self.n = 0
            self.acc = 0
        return bytes(self.out)


class BitReader:
    def __init__(self, data):
        self.d = data
        self.pos = 0     # bit position

    def get(self, length):
        v = 0
        for _ in range(length):
            byte = self.d[self.pos >> 3]
            v = (v << 1) | ((byte >> (7 - (self.pos & 7))) & 1)
            self.pos += 1
        return v


# -------------------------------------------------------------- huffman
def huffman_lengths(counts, maxlen=MAXLEN):
    """Length-limited Huffman code lengths (package-merge)."""
    syms = [s for s, c in counts.items() if c > 0]
    if not syms:
        return {}
    if len(syms) == 1:
        return {syms[0]: 1}
    if len(syms) > (1 << maxlen):
        raise ValueError("alphabet too large for length limit")
    items = sorted((counts[s], i) for i, s in enumerate(syms))
    leaves = [(w, (i,)) for w, i in items]
    packages = list(leaves)
    for _ in range(maxlen - 1):
        merged = []
        for k in range(0, len(packages) - 1, 2):
            a, b = packages[k], packages[k + 1]
            merged.append((a[0] + b[0], a[1] + b[1]))
        packages = sorted(leaves + merged, key=lambda x: x[0])
    take = packages[:2 * len(syms) - 2]
    lens = collections.Counter()
    for _w, members in take:
        for i in members:
            lens[i] += 1
    return {syms[i]: lens[i] for i in range(len(syms))}


def canonical(lengths):
    """-> {sym: (code, len)}, symbols ordered by (len, sort key)."""
    order = sorted(lengths, key=lambda s: (lengths[s], _skey(s)))
    codes, code, prev = {}, 0, 0
    for s in order:
        L = lengths[s]
        code <<= (L - prev)
        codes[s] = (code, L)
        code += 1
        prev = L
    return codes


def _skey(s):
    return (0, s) if isinstance(s, int) else (1, str(s))


# ------------------------------------------------------ value categories
def s16(v):
    return ((v + 0x8000) & 0xFFFF) - 0x8000


def cat_of(v):
    return 0 if v == 0 else abs(v).bit_length()


def cat_bits(v, k):
    return v if v > 0 else v + (1 << k) - 1


def cat_value(bits, k):
    if k == 0:
        return 0
    return bits if (bits >> (k - 1)) else bits - (1 << k) + 1


# --------------------------------------------------------------- tables
class Table:
    """One context: literal values + category escapes (+ optional CMD0 esc).

    Serialised cost (bits): 12 (alphabet sizes) + per literal
    (lit_bits + 4 length bits) + 4 bits x NCAT category lengths."""

    def __init__(self, lit_counts, esc_counts, lit_bits, esc_kind):
        self.lit_bits = lit_bits
        self.esc_kind = esc_kind            # "cat" or "cmd0"
        counts = {("L", v): c for v, c in lit_counts.items()}
        for k, c in esc_counts.items():
            counts[("E", k)] = c
        self.lengths = huffman_lengths(counts)
        self.codes = canonical(self.lengths)
        self.decode_map = {(L, code): s for s, (code, L) in self.codes.items()}
        self.lits = set(lit_counts)
        self.cost_bits = 12 + len(lit_counts) * (lit_bits + 4) + \
            (4 * NCAT if esc_kind == "cat" and esc_counts else
             (4 if esc_counts else 0))

    def data_bits(self, lit_counts, esc_counts, extra_bits):
        b = extra_bits
        for v, c in lit_counts.items():
            b += c * self.lengths[("L", v)]
        for k, c in esc_counts.items():
            b += c * self.lengths[("E", k)]
        return b


def build_table(value_counts, lit_bits, esc_kind="cat", candidates=None):
    """Choose the literal set (count >= m) minimising data + table bits.
    A CMD0 escape is charged ~0.75 x lit_bits for the order-0 index that
    follows it (the selection only; the reported sizes are real bits)."""
    best = None
    ms = candidates or (1, 2, 3, 4, 6, 8, 12, 16, 32, 64, 1 << 30)
    for m in ms:
        lits = {v: c for v, c in value_counts.items() if c >= m}
        esc = collections.Counter()
        extra = 0
        for v, c in value_counts.items():
            if v in lits:
                continue
            if esc_kind == "cat":
                k = cat_of(v)
                esc[k] += c
                extra += c * k
            else:
                esc["cmd0"] += c
                extra += c * 0.75 * lit_bits
        if not lits and not esc:
            continue
        if len(lits) + len(esc) > (1 << MAXLEN):
            continue
        t = Table(lits, esc, lit_bits, esc_kind)
        total = t.cost_bits + t.data_bits(lits, esc, extra)
        if best is None or total < best[0]:
            best = (total, t, m)
    return best[1]


# ---------------------------------------------------------------- model
TG = [0, 0, 0, 1, 2, 2, 2, 3, 3, 3]     # rot, TraI, tra, scale


BLOCK_OPS = (1, 2, 4, 7, 9)


class Config:
    """rich: per-previous-word CMD tables, VD|(track group, op, first),
             RT|(track group, op), PAY|op.
       lean: as rich for CMD; VD|tg, RT|tg, PAY single.
       fast: the decoder-friendly set -- CMD as a RANK into the previous
             word's successor list (one shared rank table + one order-0
             table), VD|(tg, first) = 8, RT|tg = 4, RT6 single, PAY|block?
             = 2; about 20 tables in all."""

    def __init__(self, cfg="rich", rate="slope", cmd_ctx_min=48,
                 first_ctx=True):
        self.cfg = cfg
        self.rate = rate
        self.cmd_ctx_min = cmd_ctx_min
        self.first_ctx = first_ctx
        self.cmd_rank = (cfg == "fast")

    def vd_ctx(self, t, op, first):
        if self.cfg == "lean":
            return (TG[t],)
        if self.cfg == "fast":
            return (TG[t], bool(first))
        return (TG[t], op, bool(first) if self.first_ctx else False)

    def rt_ctx(self, t, op):
        return (TG[t],) if self.cfg in ("lean", "fast") else (TG[t], op)

    def rt6_ctx(self, t):
        return () if self.cfg == "fast" else (TG[t],)

    def pay_ctx(self, op):
        if self.cfg == "lean":
            return ()
        if self.cfg == "fast":
            return (op in BLOCK_OPS,)
        return (op,)


RECIP15 = [0] + [int(round(32768 * 0.625 / p)) for p in range(1, 256)]


def rate_pred(cfg, d, p):
    if cfg.rate != "slope" or not p or p > 255:
        return 0
    return (d * RECIP15[p]) >> 15


def events(parsed, cfg, words_index=None):
    """The symbol stream of one clip as (class, ctx, value) events.

    CMD events carry the u16 word; the coder maps words to indices. The
    decoder below consumes exactly this order."""
    ev = []
    slots = parsed["slots"]
    offs = [r["off"] for r in parsed["runs"]]
    ev.append(("NSLOT", (), len(slots)))
    nxt = 0
    for w in slots:
        if w == 0:
            ev.append(("SLOT", (), 0))
            continue
        j = offs.index(w)
        if j == nxt:
            ev.append(("SLOT", (), 1))
            nxt += 1
        elif j < nxt:
            ev.append(("SLOT", (), 1 + (nxt - j)))
        else:
            ev.append(("SLOT", (), -(j - nxt)))
            nxt = j + 1
    for r in parsed["runs"]:
        prevw = None
        last_v = [0] * 10
        last_r = [0] * 10
        seen = [False] * 10
        for cm in r["cmds"]:
            ev.append(("CMD", (prevw,), cm["word"]))
            prevw = cm["word"]
            op = cm["op"]
            if op in (ma.OP_LOOP, ma.OP_INTERP):
                ev.append(("JUMP", (), cm["jump"] & 0xFFFF))
                continue
            if op == ma.OP_END:
                continue
            p = cm["payload"]
            if cm["toggle"]:
                ev.append(("PAY", cfg.pay_ctx(op), s16(p)))
            per = PER.get(op, 0)
            if not per:
                continue
            tracks = [b for b in range(10) if (cm["flags"] >> b) & 1]
            vals = cm["vals"]
            for k, t in enumerate(tracks):
                tv = vals[k * per:(k + 1) * per]
                if op == 6:
                    ev.append(("RT6", cfg.rt6_ctx(t), s16(tv[0] - last_r[t])))
                    last_r[t] = tv[0]
                    continue
                d = s16(tv[0] - last_v[t])
                ev.append(("VD", cfg.vd_ctx(t, op, not seen[t]), d))
                last_v[t] = tv[0]
                seen[t] = True
                if per == 2:
                    pred = rate_pred(cfg, d, p if cm["toggle"] else 0)
                    ev.append(("RT", cfg.rt_ctx(t, op), s16(tv[1] - pred)))
                    last_r[t] = tv[1]
                else:
                    last_r[t] = 0
        tail = r["tail"]
        if not tail:
            ev.append(("TAIL", (), 0))
        elif tail == bytes(len(tail)):
            ev.append(("TAIL", (), len(tail) // 2))
        else:
            ev.append(("TAIL", (), -(len(tail) // 2) - 1))
            for i in range(0, len(tail), 2):
                ev.append(("RAW16", (), struct.unpack_from("<H", tail, i)[0]))
    return ev


class Model:
    """Tables for one scope (a kind, or all kinds)."""

    def __init__(self, parsed_clips, cfg):
        self.cfg = cfg
        stats = collections.defaultdict(collections.Counter)
        word_freq = collections.Counter()
        for p in parsed_clips:
            for cls, ctx, v in events(p, cfg):
                stats[(cls, ctx)][v] += 1
                if cls == "CMD":
                    word_freq[v] += 1
        self.words = [w for w, _ in word_freq.most_common()]
        self.windex = {w: i for i, w in enumerate(self.words)}
        wbits = max(1, math.ceil(math.log2(max(2, len(self.words)))))
        self.tables = {}
        # CMD: dedicated context tables for frequent previous words.
        cmd_ctx_counts = collections.Counter()
        for (cls, ctx), cnt in stats.items():
            if cls == "CMD":
                cmd_ctx_counts[ctx] = sum(cnt.values())
        self.cmd_dedicated = {ctx for ctx, n in cmd_ctx_counts.items()
                              if n >= cfg.cmd_ctx_min}
        cmd0 = collections.Counter()
        other = collections.Counter()
        self.succ = {}
        self.succ_bits = 0
        if cfg.cmd_rank:
            # a context is "dedicated" only if it has a non-empty successor
            # list (the ARM9 decoder tests succ_len[prev] != 0)
            for (cls, ctx), cnt in stats.items():
                if cls == "CMD" and ctx in self.cmd_dedicated and \
                        not any(c >= 2 for c in cnt.values()):
                    self.cmd_dedicated.discard(ctx)
            crank = collections.Counter()
            crank0 = collections.Counter()
            for (cls, ctx), cnt in stats.items():
                if cls != "CMD":
                    continue
                if ctx in self.cmd_dedicated:
                    lst = [w for w, c in cnt.most_common() if c >= 2]
                    self.succ[ctx] = {w: r for r, w in enumerate(lst)}
                    self.succ_bits += 8 + wbits * len(lst)
                    for w, c in cnt.items():
                        if w in self.succ[ctx]:
                            crank[self.succ[ctx][w]] += c
                        else:
                            crank[-1] += c
                            crank0[self.windex[w]] += c
                else:
                    for w, c in cnt.items():
                        crank0[self.windex[w]] += c
            self.succ_list = {ctx: sorted(d, key=d.get) for ctx, d in self.succ.items()}
            self.tables[("CRANK", ())] = build_table(crank, 8)
            self.tables[("CRANK0", ())] = build_table(crank0, wbits)
        for (cls, ctx), cnt in stats.items():
            if cls != "CMD" or cfg.cmd_rank:
                continue
            if ctx in self.cmd_dedicated:
                idx = collections.Counter({self.windex[w]: c
                                           for w, c in cnt.items()})
                t = build_table(idx, wbits, esc_kind="cmd0",
                                candidates=(2, 3, 4, 6, 8, 16, 1 << 30))
                self.tables[("CMD", ctx)] = t
                for w, c in cnt.items():
                    if self.windex[w] not in t.lits:
                        cmd0[self.windex[w]] += c
            else:
                for w, c in cnt.items():
                    other[self.windex[w]] += c
        for i, c in other.items():
            cmd0[i] += c
        if not cfg.cmd_rank:
            self.tables[("CMD0", ())] = build_table(cmd0, wbits, esc_kind="cat",
                                                    candidates=(1,))
        for (cls, ctx), cnt in stats.items():
            if cls in ("CMD", "JUMP", "RAW16"):
                continue
            self.tables[(cls, ctx)] = build_table(cnt, 16)
        self.word_list_bits = 16 * len(self.words) + 12
        self.table_bits = self.word_list_bits + self.succ_bits + sum(
            t.cost_bits for t in self.tables.values())
        self.literals = sum(len(t.lits) for t in self.tables.values())

    # ------------------------------------------------------------ encode
    def put_value(self, bw, cls, ctx, v):
        """Code one value; records (code length, extra bits) in self.lenlog
        for the ARM9 cost model (mf_cost_model.py)."""
        t = self.tables[(cls, ctx)]
        if self.access is not None:
            self.access.append(("T", (cls, ctx), bw.bits))
        if v in t.lits:
            code, L = t.codes[("L", v)]
            bw.put(code, L)
            self.lenlog[(L, 0)] += 1
            return 1
        k = cat_of(v)
        code, L = t.codes[("E", k)]
        bw.put(code, L)
        bw.put(cat_bits(v, k), k)
        self.lenlog[(L, k if k else -1)] += 1
        return 1

    access = None       # set to [] to log table accesses (cost model)

    def encode(self, parsed):
        bw = BitWriter()
        nsym = collections.Counter()
        self.lenlog = collections.Counter()
        for cls, ctx, v in events(parsed, self.cfg):
            nsym[cls] += 1
            if cls == "CMD" and self.cfg.cmd_rank:
                i = self.windex[v]
                if ctx in self.cmd_dedicated:
                    r = self.succ[ctx].get(v)
                    if r is not None:
                        self.put_value(bw, "CRANK", (), r)
                        if self.access is not None:
                            self.access.append(("S", ctx, r))
                        continue
                    self.put_value(bw, "CRANK", (), -1)
                    nsym["CMD0"] += 1
                self.put_value(bw, "CRANK0", (), i)
                if self.access is not None:
                    self.access.append(("W", i, 0))
            elif cls == "CMD":
                i = self.windex[v]
                if ctx in self.cmd_dedicated:
                    t = self.tables[("CMD", ctx)]
                    if i in t.lits:
                        code, L = t.codes[("L", i)]
                        bw.put(code, L)
                        continue
                    code, L = t.codes[("E", "cmd0")]
                    bw.put(code, L)
                    nsym["CMD0"] += 1
                self.put_value(bw, "CMD0", (), i)
            elif cls in ("JUMP", "RAW16"):
                bw.put(v, 16)
            else:
                self.put_value(bw, cls, ctx, v)
        nsym["_lenlog"] = {"%d/%d" % k: n for k, n in self.lenlog.items()}
        return bw.done(), bw.bits, nsym

    # ------------------------------------------------------------ decode
    def get_sym(self, br, t):
        code, L = 0, 0
        while True:
            code = (code << 1) | br.get(1)
            L += 1
            s = t.decode_map.get((L, code))
            if s is not None:
                return s
            if L > MAXLEN:
                raise ValueError("bad code")

    def get_value(self, br, cls, ctx):
        t = self.tables[(cls, ctx)]
        kind, v = self.get_sym(br, t)
        if kind == "L":
            return v
        return cat_value(br.get(v), v)

    def get_word(self, br, ctx):
        if self.cfg.cmd_rank:
            if ctx in self.cmd_dedicated:
                r = self.get_value(br, "CRANK", ())
                if r >= 0:
                    return self.succ_list[ctx][r]
            return self.words[self.get_value(br, "CRANK0", ())]
        if ctx in self.cmd_dedicated:
            kind, v = self.get_sym(br, self.tables[("CMD", ctx)])
            if kind == "L":
                return self.words[v]
        return self.words[self.get_value(br, "CMD0", ())]

    def decode(self, data, size):
        """Bitstream -> exact BPS1 clip bytes (independent of the parser:
        it writes bytes as the ARM9 decoder would, then the caller compares
        against the original clip)."""
        cfg = self.cfg
        br = BitReader(data)
        nslot = self.get_value(br, "NSLOT", ())
        slot_sym = [self.get_value(br, "SLOT", ()) for _ in range(nslot)]
        run_of_slot, nxt = [], 0
        for s in slot_sym:
            if s == 0:
                run_of_slot.append(None)
            elif s == 1:
                run_of_slot.append(nxt)
                nxt += 1
            elif s > 1:
                run_of_slot.append(nxt - (s - 1))
            else:
                j = nxt + (-s)
                run_of_slot.append(j)
                nxt = j + 1
        nruns = max([j for j in run_of_slot if j is not None], default=-1) + 1
        out = bytearray(4 * nslot)
        run_off = []
        for _ in range(nruns):
            run_off.append(len(out))
            prevw = None
            last_v = [0] * 10
            last_r = [0] * 10
            seen = [False] * 10
            while True:
                w = self.get_word(br, (prevw,))
                prevw = w
                out += struct.pack("<H", w)
                op, flags, tog = w & 0x1F, (w >> 5) & 0x3FF, w >> 15
                if op in (ma.OP_LOOP, ma.OP_INTERP):
                    out += struct.pack("<H", br.get(16))
                    if op == ma.OP_LOOP:
                        break
                    continue
                if op == ma.OP_END:
                    break
                p = 0
                if tog:
                    p = self.get_value(br, "PAY", cfg.pay_ctx(op)) & 0xFFFF
                    out += struct.pack("<H", p)
                per = PER.get(op, 0)
                if not per:
                    continue
                for t in range(10):
                    if not (flags >> t) & 1:
                        continue
                    if op == 6:
                        r = s16(last_r[t] + self.get_value(br, "RT6", cfg.rt6_ctx(t)))
                        out += struct.pack("<h", r)
                        last_r[t] = r
                        continue
                    d = self.get_value(br, "VD", cfg.vd_ctx(t, op, not seen[t]))
                    v = s16(last_v[t] + d)
                    out += struct.pack("<h", v)
                    last_v[t] = v
                    seen[t] = True
                    if per == 2:
                        pred = rate_pred(cfg, s16(d), p if tog else 0)
                        r = s16(pred + self.get_value(br, "RT", cfg.rt_ctx(t, op)))
                        out += struct.pack("<h", r)
                        last_r[t] = r
                    else:
                        last_r[t] = 0
            tl = self.get_value(br, "TAIL", ())
            if tl >= 0:
                out += bytes(2 * tl)
            else:
                for _ in range(-tl - 1):
                    out += struct.pack("<H", br.get(16))
        for i, j in enumerate(run_of_slot):
            struct.pack_into("<I", out, 4 * i, 0 if j is None else run_off[j])
        if len(out) != size:
            raise ValueError("decoded %d bytes, directory says %d" % (len(out), size))
        return bytes(out)


# ------------------------------------------------------------------ run
def run(kinds, scope, cfg, log=print, clips_out=None):
    c = mc.load_corpus(log=log)
    clips = c["clips"]
    banks = collections.defaultdict(list)
    for aid in sorted(clips):
        banks[clips[aid]["bank"]].append(aid)
    parsed = {a: ma.parse_clip(clips[a]["bytes"]) for a in clips}
    models = {}
    if scope == "global":
        g = Model([parsed[a] for a in sorted(clips)], cfg)
        models = {k: g for k in mc.KINDS}
    else:
        for k in kinds:
            models[k] = Model([parsed[a] for a in banks[k]], cfg)
    res = {"kinds": {}, "scope": scope, "cfg": vars(cfg)}
    per_clip = {}
    tot_raw = tot_comp = 0
    for k in kinds:
        m = models[k]
        t0 = time.time()
        sizes, raw = [], 0
        symtot = collections.Counter()
        for a in banks[k]:
            data, bits, nsym = m.encode(parsed[a])
            back = m.decode(data, len(clips[a]["bytes"]))
            if back != clips[a]["bytes"]:
                raise SystemExit("ROUND TRIP FAILED %s 0x%x" % (k, a))
            # clip payload is padded to 4 B so a resident clip starts aligned
            sizes.append((len(data) + 3) & ~3)
            raw += len(clips[a]["bytes"])
            symtot.update({k: v for k, v in nsym.items() if k != "_lenlog"})
            per_clip[a] = {"kind": k, "raw": len(clips[a]["bytes"]),
                           "comp": (len(data) + 3) & ~3, "bits": bits,
                           "syms": dict(nsym)}
        table_bytes = (m.table_bits + 7) // 8
        comp = sum(sizes)
        q = sorted(sizes)
        r = {"raw": raw, "clips": len(sizes), "comp": comp,
             "tables_bytes": table_bytes,
             "tables_charged": table_bytes if scope == "kind" else 0,
             "ratio_ex_tables": comp / raw,
             "ratio_incl_tables": (comp + (table_bytes if scope == "kind" else 0)) / raw,
             "clip_median": statistics.median(q), "clip_p95": q[int(0.95 * (len(q) - 1))],
             "clip_max": q[-1], "roundtrip": "%d/%d" % (len(sizes), len(sizes)),
             "tables": len(m.tables), "words": len(m.words), "literals": m.literals,
             "symbols": dict(symtot), "seconds": round(time.time() - t0, 1)}
        res["kinds"][k] = r
        tot_raw += raw
        tot_comp += comp + r["tables_charged"]
        log("%-8s raw %7d -> %7d + tables %6d = ratio %.3f (ex tables %.3f) clip med/p95/max "
            "%d/%d/%d rt %s tables=%d lits=%d words=%d %.0fs" % (
                k, raw, comp, table_bytes, r["ratio_incl_tables"], r["ratio_ex_tables"],
                r["clip_median"], r["clip_p95"], r["clip_max"], r["roundtrip"],
                len(m.tables), m.literals, len(m.words), r["seconds"]))
    if scope == "global":
        gt = (models[kinds[0]].table_bits + 7) // 8
        res["global_tables_bytes"] = gt
        tot_comp += gt
        log("global tables %d B (charged once)" % gt)
    res["total_raw"] = tot_raw
    res["total_comp"] = tot_comp
    res["total_ratio"] = tot_comp / tot_raw
    log("TOTAL %d -> %d ratio %.3f" % (tot_raw, tot_comp, tot_comp / tot_raw))
    if clips_out is not None:
        clips_out.update(per_clip)
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--kinds", default=",".join(mc.KINDS))
    ap.add_argument("--scope", default="kind", choices=("kind", "global"))
    ap.add_argument("--cfg", default="rich", choices=("rich", "lean", "fast"))
    ap.add_argument("--rate", default="slope", choices=("zero", "slope"))
    ap.add_argument("--cmd-ctx-min", type=int, default=48)
    ap.add_argument("--json", type=pathlib.Path)
    ap.add_argument("--clips-json", type=pathlib.Path)
    a = ap.parse_args()
    cfg = Config(a.cfg, a.rate, a.cmd_ctx_min)
    per_clip = {}
    res = run(a.kinds.split(","), a.scope, cfg, clips_out=per_clip)
    if a.json:
        a.json.write_text(json.dumps(res, indent=1, sort_keys=True, default=str))
    if a.clips_json:
        a.clips_json.write_text(json.dumps({"0x%x" % k: v for k, v in sorted(per_clip.items())},
                                           separators=(",", ":")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
