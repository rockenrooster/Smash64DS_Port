#!/usr/bin/env python3
"""P2-2p8 Phase 3: emit the MF1 compact motion pack (ftanim_mf_pack.bin).

Candidate B of the Phase 0 experiment (mf_codec_b.py, cfg "fast", one global
table set), productionised: every AObj16 clip of every kind is encoded against
one set of static canonical Huffman tables and written as

    header (NdsMfPackHeader, 48 B)
    kind table   (NdsMfPackKind x kinds, 16 B each)
    tables blob  ("MFT1", expanded once at boot by ndsMfExpandTables)
    directory    (NdsMfPackEntry x clips, 24 B each), per kind contiguous,
                 ordered [always resident][victim clips by opponent]
                 [Kirby copies by opponent]
    by-id index  (u16 x clips: entry indices sorted by asset id)
    data         (one bitstream per clip, 4-aligned, in directory order)

Layouts are the C structs in include/nds/nds_motion_mf.h; the decoder is
src/nds/nds_motion_mf.c; scripts/motion/check_mf_pack.py proves the pack with
that decoder compiled for the host.

Tables blob ("MFT1"), little-endian:
    u32 magic, u16 nwords, u16 nsucc, u16 ntables, u16 0
    u16 recip15[256]                 rate predictor reciprocals
    u16 words[nwords]                command words by order-0 index
    nsucc x (u16 ctx, u16 len, u16 word_index[len])
                                     successor lists in rank order; ctx 0 =
                                     run start, i + 1 = after word i
    ntables x (u8 cls, u8 ctx, u16 nsym, u16 nesc, u16 count[12],
               s16 sym[nsym], u16 esc_pos[nesc])
                                     symbols in canonical order; an escape
                                     position holds its category (0-16)

Usage:
  python scripts/motion/mf_emit.py --out DIR [--json MANIFEST]
"""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import pathlib
import struct
import sys
import zlib

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import mf_aobj16 as ma  # noqa: E402
import mf_codec_b as mb  # noqa: E402
import mf_corpus as mc  # noqa: E402

TABLES_MAGIC = 0x3154464D  # "MFT1"
PACK_MAGIC = 0x3150464D    # "MFP1"
PACK_VERSION = 1

CLASS_IDS = {"NSLOT": 0, "SLOT": 1, "CRANK": 2, "CRANK0": 3, "PAY": 4,
             "VD": 5, "RT": 6, "RT6": 7, "TAIL": 8}
CLS_ALWAYS, CLS_VICTIM, CLS_COPY = 0, 1, 2
HEADER = struct.Struct("<IHHIIIIIIII8x")
KIND_ROW = struct.Struct("<8sHHI")
ENTRY = struct.Struct("<IIIIIBBH")
assert HEADER.size == 48 and KIND_ROW.size == 16 and ENTRY.size == 24


def ctx_id(cls, ctx):
    if cls == "PAY":
        return int(bool(ctx[0]))
    if cls == "VD":
        return 2 * ctx[0] + int(bool(ctx[1]))
    if cls == "RT":
        return ctx[0]
    if ctx != ():
        raise ValueError("unexpected context %r for %s" % (ctx, cls))
    return 0


def s16_ok(v):
    return -0x8000 <= v <= 0x7FFF


def serialise_tables(model):
    """-> bytes of the MFT1 blob for one mf_codec_b.Model (cfg fast)."""
    words = model.words
    windex = model.windex
    out = bytearray()
    succ_rows = []
    for ctx, lst in model.succ_list.items():
        prevw = ctx[0]
        cidx = 0 if prevw is None else windex[prevw] + 1
        succ_rows.append((cidx, [windex[w] for w in lst]))
    succ_rows.sort()
    tables = []
    for (cls, ctx), t in model.tables.items():
        tables.append((CLASS_IDS[cls], ctx_id(cls, ctx), cls, ctx, t))
    tables.sort(key=lambda r: (r[0], r[1]))
    out += struct.pack("<IHHHH", TABLES_MAGIC, len(words), len(succ_rows), len(tables), 0)
    out += struct.pack("<256H", *[min(r, 0xFFFF) for r in mb.RECIP15])
    out += struct.pack("<%dH" % len(words), *words)
    for cidx, lst in succ_rows:
        out += struct.pack("<HH", cidx, len(lst))
        out += struct.pack("<%dH" % len(lst), *lst)
    for cid, xid, cls, ctx, t in tables:
        order = sorted(t.codes, key=lambda s: (t.codes[s][1], t.codes[s][0]))
        counts = [0] * 12
        syms, esc = [], []
        for i, s in enumerate(order):
            L = t.codes[s][1]
            if not 1 <= L <= 12:
                raise ValueError("code length %d out of range" % L)
            counts[L - 1] += 1
            kind, v = s
            if kind == "E":
                if not 0 <= v <= 16:
                    raise ValueError("escape category %r" % (v,))
                esc.append(i)
                syms.append(v)
            else:
                if not s16_ok(v):
                    raise ValueError("literal %d outside s16 (%s %r)" % (v, cls, ctx))
                syms.append(v)
        out += struct.pack("<BBHH", cid, xid, len(syms), len(esc))
        out += struct.pack("<12H", *counts)
        out += struct.pack("<%dh" % len(syms), *syms)
        if esc:
            out += struct.pack("<%dH" % len(esc), *esc)
    return bytes(out)


def classify_clip(corpus, kind, aid):
    cls, who = mc.classify(kind, corpus["names"].get(aid))
    if cls not in ("victim", "copy"):
        return CLS_ALWAYS, 0
    kinds = (who,) if isinstance(who, str) else tuple(who)
    mask = 0
    for k in kinds:
        mask |= 1 << mc.KINDS.index(k)
    return (CLS_VICTIM if cls == "victim" else CLS_COPY), mask


def build(log=print):
    corpus = mc.load_corpus(log=log)
    clips = corpus["clips"]
    parsed = {a: ma.parse_clip(clips[a]["bytes"]) for a in clips}
    for a, p in parsed.items():
        if len(p["slots"]) > 64 or len(p["runs"]) > 64:
            raise SystemExit("clip 0x%x: %d slots / %d runs exceed the decoder's 64"
                             % (a, len(p["slots"]), len(p["runs"])))
    cfg = mb.Config("fast", "slope")
    model = mb.Model([parsed[a] for a in sorted(clips)], cfg)
    blob = serialise_tables(model)

    rows = []  # (kind index, cls, mask, aid, stream, bits, bps1)
    for a in sorted(clips):
        kind = clips[a]["bank"]
        data, bits, _n = model.encode(parsed[a])
        # the experiment's own decoder must agree before anything is written;
        # the proof that matters is check_mf_pack.py with the C decoder
        if model.decode(data, len(clips[a]["bytes"])) != clips[a]["bytes"]:
            raise SystemExit("python round trip failed for 0x%x" % a)
        cls, mask = classify_clip(corpus, kind, a)
        rows.append((mc.KINDS.index(kind), cls, mask, a, data, bits, clips[a]["bytes"]))
    rows.sort(key=lambda r: (r[0], r[1], r[2], r[3]))

    kinds_present = sorted({r[0] for r in rows})
    kind_rows = []
    entries = []
    data = bytearray()
    for ki, k in enumerate(mc.KINDS):
        first = len(entries)
        for r in rows:
            if r[0] != ki:
                continue
            _k, cls, mask, aid, stream, bits, bps1 = r
            rel = len(data)
            data += stream
            data += bytes((-len(data)) & 3)
            entries.append((aid, rel, bits, len(bps1), zlib.crc32(bps1) & 0xFFFFFFFF,
                            ki, cls, mask, bps1, stream))
        kind_rows.append((k.encode()[:8], first, len(entries) - first))
    by_id = sorted(range(len(entries)), key=lambda i: entries[i][0])

    kind_off = HEADER.size
    tables_off = kind_off + KIND_ROW.size * len(kind_rows)
    dir_off = (tables_off + len(blob) + 3) & ~3
    by_id_off = dir_off + ENTRY.size * len(entries)
    data_off = (by_id_off + 2 * len(entries) + 3) & ~3
    out = bytearray(data_off)
    HEADER.pack_into(out, 0, PACK_MAGIC, PACK_VERSION, len(kind_rows), tables_off,
                     len(blob), dir_off, len(entries), kind_off, by_id_off, data_off,
                     len(data))
    for i, (name, first, count) in enumerate(kind_rows):
        KIND_ROW.pack_into(out, kind_off + KIND_ROW.size * i, name, first, count, 0)
    out[tables_off:tables_off + len(blob)] = blob
    for i, e in enumerate(entries):
        ENTRY.pack_into(out, dir_off + ENTRY.size * i, *e[:8])
    struct.pack_into("<%dH" % len(by_id), out, by_id_off, *by_id)
    out += data

    manifest = {
        "pack_bytes": len(out), "tables_bytes": len(blob), "data_bytes": len(data),
        "clips": len(entries), "kinds_present": [mc.KINDS[k] for k in kinds_present],
        "bps1_bytes": sum(len(e[8]) for e in entries),
        "tables": len(model.tables), "words": len(model.words),
        "succ_contexts": len(model.succ_list),
        "entries": [{"id": "0x%x" % e[0], "kind": mc.KINDS[e[5]], "cls": e[6],
                     "need_mask": e[7], "bps1_bytes": e[3],
                     "bps1_sha256": hashlib.sha256(e[8]).hexdigest(),
                     "mf_bytes": (len(e[9]) + 3) & ~3, "stream_bits": e[2]}
                    for e in entries],
    }
    manifest["ratio"] = (len(data) + len(blob)) / manifest["bps1_bytes"]
    return bytes(out), manifest


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=pathlib.Path, required=True, help="output directory")
    ap.add_argument("--json", type=pathlib.Path, help="manifest path (default OUT/ftanim_mf_pack.json)")
    a = ap.parse_args()
    pack, manifest = build()
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / "ftanim_mf_pack.bin").write_bytes(pack)
    (a.json or (a.out / "ftanim_mf_pack.json")).write_text(json.dumps(manifest, indent=1))
    print("MF1 pack: %d clips, %d B (tables %d, data %d) from %d B of BPS1: ratio %.3f" % (
        manifest["clips"], manifest["pack_bytes"], manifest["tables_bytes"],
        manifest["data_bytes"], manifest["bps1_bytes"], manifest["ratio"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
