#!/usr/bin/env python3
"""MF experiment: ARM9 bind-cost model for candidate B ("fast", global tables).

Two parts, labelled separately in the README:

1. Instruction cycles (ESTIMATE). Per-symbol and per-output costs come from
   the ARM listing of the decoder inner loop (README §5): ARM946E-S,
   single issue, code in ITCM, 1 cycle per data op, +1 cycle for a
   register-specified shift, LDR result +1 interlock, LDRH/LDRSH +2,
   taken branch 3 cycles. The counts of each symbol class, code length and
   escape come from actually encoding every clip (MEASURED inputs).

2. Memory stalls (ESTIMATE from a MEASURED access trace). The encoder logs
   every table lookup the decoder will make (table, bit position) and every
   successor-list / word-list read; the bit window at that position gives the
   exact LUT entry the ARM9 would load. Distinct 32 B lines touched per bind
   x 46 cycles (23 bus cycles per main-RAM line refill) prices a bind with
   the tables cold in main RAM; the stream itself is read sequentially
   (ceil(bytes/32) + 1 lines). "DTCM tables" = the tables cost no misses.

1 timer tick = 2 ARM9 cycles (67 MHz / 33.5 MHz).

Usage:
  python scripts/motion/mf_cost_model.py [--lut-bits 6,8] [--json OUT]
"""

from __future__ import annotations

import argparse
import collections
import json
import math
import pathlib
import statistics
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import mf_aobj16 as ma  # noqa: E402
import mf_codec_b as mb  # noqa: E402
import mf_corpus as mc  # noqa: E402

LINE = 32
MISS = 46          # ARM9 cycles per main-RAM line refill (23 bus cycles)

# ---- instruction model (cycles), see README §5 listing -------------------
C_SYM = 12         # LUT hit: peek, LDR entry (+1), len, shift (2), sub, esc test, value
C_REFILL = 10      # taken branch + LDRH + shift/or + add, once per 16 bits consumed
C_LONG_BASE = 9    # LUT miss: canonical compare loop setup + LDR sym entry
C_LONG_PER = 3     # per code length beyond the LUT width (CMP + branch)
C_ESC = 16         # category escape: k raw bits + sign fix
GLUE = {           # per decoded symbol of each class, beyond the symbol decode
    "CMD": 25,     # succ/word LDRH (+2 each), STRH, opcode dispatch, flags, loop
    "PAY": 5,      # AND, STRH, reciprocal LDR
    "VD": 15,      # context select, LDRSH last (+2), ADD, STRH x2, seen, track loop
    "RT": 9,       # MUL + ASR predictor, ADD, STRH x2
    "RT6": 8,
    "SLOT": 11,    # run-index bookkeeping + final slot-table STR
    "NSLOT": 20,
    "TAIL": 30,    # tail + per-run state clear (STM) + run offset
    "JUMP": 10,    # raw 16 bits + STRH
    "RAW16": 10,
    "CMD0": 0,     # counted inside CMD (the escape's second lookup is a symbol)
}
C_BIND = 250       # entry, directory row, reader init, registration


def window(data, bitpos, n):
    v = 0
    for i in range(n):
        p = bitpos + i
        byte = data[p >> 3] if (p >> 3) < len(data) else 0
        v = (v << 1) | ((byte >> (7 - (p & 7))) & 1)
    return v


def table_layout(model, lut_bits):
    """Addresses (region, offset) for LUTs, canonical arrays, symbol arrays,
    successor lists and the word list; returns lookup helpers + RAM bytes."""
    keys = sorted(model.tables, key=lambda k: (k[0], str(k[1])))
    tid = {k: i for i, k in enumerate(keys)}
    canon_index, sym_base, off = {}, {}, 0
    for k in keys:
        t = model.tables[k]
        order = sorted(t.lengths, key=lambda s: (t.lengths[s], mb._skey(s)))
        canon_index[k] = {s: i for i, s in enumerate(order)}
        sym_base[k] = off
        off += 4 * len(order)
    sym_bytes = off
    succ_off, off = {}, 0
    for ctx in sorted(model.succ_list, key=lambda c: (c[0] is None, c[0] or 0)):
        succ_off[ctx] = off
        off += 2 * len(model.succ_list[ctx])
    succ_bytes = off + 2 * len(model.words)     # + per-word list offsets
    ram = {"lut": len(keys) * (4 << lut_bits), "canon": len(keys) * (13 * 4 + 13 * 2),
           "symbols": sym_bytes, "succ": succ_bytes, "words": 2 * len(model.words)}
    return tid, canon_index, sym_base, succ_off, ram


def bind_cost(model, parsed, raw_bytes, lut_bits, layout):
    tid, canon_index, sym_base, succ_off, _ram = layout
    model.access = []
    data, bits, nsym = model.encode(parsed)
    acc = model.access
    model.access = None
    lenlog = nsym.pop("_lenlog")
    cyc = C_BIND
    nsyms = 0
    for key, n in lenlog.items():
        L, k = (int(x) for x in key.split("/"))
        nsyms += n
        c = C_SYM
        if L > lut_bits:
            c += C_LONG_BASE + C_LONG_PER * (L - lut_bits)
        if k > 0:
            c += C_ESC
        cyc += c * n
    cyc += C_REFILL * math.ceil(bits / 16)
    for cls, n in nsym.items():
        cyc += GLUE.get(cls, 0) * n
    lines = set()
    for a in acc:
        if a[0] == "T":
            key, bitpos = a[1], a[2]
            t = model.tables[key]
            w = window(data, bitpos, mb.MAXLEN)
            # the symbol at this position (canonical decode on the MAXLEN window)
            sym, L = None, None
            for l in range(1, mb.MAXLEN + 1):
                s = t.decode_map.get((l, w >> (mb.MAXLEN - l)))
                if s is not None:
                    sym, L = s, l
                    break
            lut_idx = w >> (mb.MAXLEN - lut_bits)
            lines.add(("lut", (tid[key] * (1 << lut_bits) + lut_idx) * 4 // LINE))
            if L > lut_bits:
                lines.add(("canon", tid[key] * 78 // LINE))
                lines.add(("sym", (sym_base[key] + 4 * canon_index[key][sym]) // LINE))
        elif a[0] == "S":
            ctx, r = a[1], a[2]
            lines.add(("succ", (succ_off[ctx] + 2 * r) // LINE))
            prev = ctx[0]
            pidx = model.windex[prev] if prev is not None else len(model.words)
            lines.add(("succptr", 2 * pidx // LINE))
            wi = model.windex[model.succ_list[ctx][r]]
            lines.add(("words", 2 * wi // LINE))
        elif a[0] == "W":
            lines.add(("words", 2 * a[1] // LINE))
    src_lines = math.ceil(len(data) / LINE) + 1
    return {"raw": raw_bytes, "comp": len(data), "bits": bits, "symbols": nsyms,
            "instr_cycles": cyc, "src_miss_cycles": src_lines * MISS,
            "table_lines": len(lines), "table_miss_cycles": len(lines) * MISS}


def summarize(rows, key):
    v = sorted(r[key] for r in rows)
    return {"mean": statistics.mean(v), "median": statistics.median(v),
            "p95": v[int(0.95 * (len(v) - 1))], "max": v[-1]}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lut-bits", default="6,8")
    ap.add_argument("--json", type=pathlib.Path)
    a = ap.parse_args()
    c = mc.load_corpus()
    clips = c["clips"]
    parsed = {i: ma.parse_clip(clips[i]["bytes"]) for i in clips}
    cfg = mb.Config("fast", "slope")
    model = mb.Model([parsed[i] for i in sorted(clips)], cfg)
    pack_ids = {i for i, x in clips.items() if x["source"] == "pack"}
    out = {}
    for P in [int(x) for x in a.lut_bits.split(",")]:
        layout = table_layout(model, P)
        rows = {}
        for i in sorted(clips):
            rows[i] = bind_cost(model, parsed[i], len(clips[i]["bytes"]), P, layout)
        allr = list(rows.values())
        packr = [rows[i] for i in pack_ids]
        big = max(allr, key=lambda r: r["raw"])
        tot_raw = sum(r["raw"] for r in allr)
        res = {"lut_bits": P, "table_ram_bytes": layout[4],
               "table_ram_total": sum(layout[4].values())}
        for name, rs in (("all_1570", allr), ("pack_1139", packr)):
            s = {}
            for k in ("raw", "comp", "symbols", "instr_cycles", "src_miss_cycles",
                      "table_lines", "table_miss_cycles"):
                s[k] = summarize(rs, k)
            res[name] = s
        instr_per_byte = sum(r["instr_cycles"] for r in allr) / tot_raw
        res["instr_cycles_per_output_byte"] = instr_per_byte
        res["largest_clip"] = big
        out["P%d" % P] = res

        def ticks(r, dtcm):
            cyc = r["instr_cycles"] + r["src_miss_cycles"] + \
                (0 if dtcm else r["table_miss_cycles"])
            return cyc / 2

        for dtcm in (True, False):
            tag = "dtcm" if dtcm else "cold"
            tk_all = [ticks(r, dtcm) for r in allr]
            tk_pack = [ticks(r, dtcm) for r in packr]
            res["ticks_%s" % tag] = {
                "mean_all": statistics.mean(tk_all),
                "mean_pack": statistics.mean(tk_pack),
                "p95_all": sorted(tk_all)[int(0.95 * (len(tk_all) - 1))],
                "largest_clip": ticks(big, dtcm),
                "per_2200B_raw": statistics.mean(
                    t * 2200 / r["raw"] for t, r in zip(tk_all, allr))}
        print("LUT %d bits: table RAM %d B %s" % (P, res["table_ram_total"],
                                                 layout[4]))
        print("  instr cycles/output byte %.2f; mean clip raw %.0f B comp %.0f B, %.0f symbols"
              % (instr_per_byte, res["all_1570"]["raw"]["mean"],
                 res["all_1570"]["comp"]["mean"], res["all_1570"]["symbols"]["mean"]))
        print("  table lines touched per bind: mean %.0f p95 %d max %d" % (
            res["all_1570"]["table_lines"]["mean"], res["all_1570"]["table_lines"]["p95"],
            res["all_1570"]["table_lines"]["max"]))
        for tag in ("dtcm", "cold"):
            t = res["ticks_%s" % tag]
            print("  ticks/bind (%s tables): mean all %.0f, mean pack %.0f, p95 %.0f, "
                  "largest clip (%d B raw) %.0f, per 2.2 KB %.0f" % (
                      tag, t["mean_all"], t["mean_pack"], t["p95_all"], big["raw"],
                      t["largest_clip"], t["per_2200B_raw"]))
    if a.json:
        a.json.write_text(json.dumps(out, indent=1, default=str))
    return 0


if __name__ == "__main__":
    sys.exit(main())
