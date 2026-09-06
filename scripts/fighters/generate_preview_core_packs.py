#!/usr/bin/env python3
"""Production FPC1 core-pack encoder (offline, no ROM/build/emulator).

Generates checked source metadata from the decomp/O2R inputs, then
emits FPC1 packs per include/nds/nds_preview_pack.h: u32 LE header64,
section records32, raw BE section data, LE fixup pairs, LE span triples.
Hashes are FNV1a32 over the emitted data/fixup/span byte arrays.

Layout per kind:
- section 0: Main identity bytes (compact bin [0:main_len]).
- section 1: Model kept spans concatenated (remapped) + one unique 8-byte
  root identity cell per pruned DL target referenced by a retained slot
  (BE ENDDL 0xDF000000 + BE original root byte offset).
- tail sections: separately identified tail files (Donkey DkIcon), identity.
- No idle/Selected animation bytes are shipped; source animation loading
  stays Main's decision.

Fixups cover every retained Main/Model slot: kept targets point at their
section (Main identity, Model remapped, tail identity); retained slots
whose target is a pruned DL point at that DL's unique root cell; retained
slots with any other pruned target write UINT32_MAX null.

Failures (no pack emitted): model/main unresolved, chain mismatches,
closure holes, unclassified or otherwise unplaceable kept extern edges.

Run:
    python scripts/fighters/generate_preview_core_packs.py --output-dir builds/preview-core --kinds mario,fox
    # Omitting --kinds requests all 12.
"""

from __future__ import annotations

import argparse
import json
import os
import struct
import sys

MAGIC = 0x31435046  # FPC1
VERSION = 1
NULL = 0xFFFFFFFF
MAX_SECTIONS = 4
ENDDL = 0xDF000000
ALIGN = 16

HEADER_FMT = "<16I"
SECTION_FMT = "<8I"
FIXUP_FMT = "<2I"
SPAN_FMT = "<3I"

# Actual FTKind enum source order (include/ft/fighter.h), not report order.
KIND_ORDER = [
    "mario", "fox", "donkey", "samus", "luigi", "link",
    "yoshi", "captain", "kirby", "pikachu", "purin", "ness",
]

class PackError(Exception):
    pass


def fnv1a32(data: bytes) -> int:
    h = 0x811C9DC5
    for b in data:
        h ^= b
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h


def align_up(n: int, a: int) -> int:
    return (n + a - 1) // a * a


def load_kind(input_dir: str, kind: str):
    with open(os.path.join(input_dir, kind + "_compact_map.json"),
              "r", encoding="utf-8") as f:
        m = json.load(f)
    with open(os.path.join(input_dir, kind + "_compact.bin"), "rb") as f:
        b = f.read()
    # Map offsets describe the payload, not its eight-byte MCM2 container.
    # Copying that header into Main shifts every source field and relocation.
    if len(b) < 8 or b[:4] != b"MCM2":
        raise PackError(kind + ": invalid MCM2 container")
    size = struct.unpack_from("<I", b, 4)[0]
    if size != m["emitted_bytes"] or len(b) != size + 8:
        raise PackError(kind + ": MCM2 payload length mismatch")
    return m, b[8:]


def gate_metadata(kind: str, m: dict) -> None:
    """Fail on anything the loader could not resolve."""
    problems = []
    for key in ("main_intern",):
        blk = m.get(key, {})
        for sub in ("unresolved", "chain_mismatches", "closure_holes"):
            v = blk.get(sub, 0)
            n = len(v) if isinstance(v, list) else v
            if n:
                problems.append("%s.%s=%d" % (key, sub, n))
    mi = m.get("model_intern", {})
    for sub in ("unresolved", "chain_mismatches", "closure_holes"):
        v = mi.get(sub, [])
        if len(v):
            sample = v[0] if isinstance(v[0], str) else v[0].get("line", v[0])
            problems.append("model_intern.%s=%d (e.g. %s)"
                            % (sub, len(v), sample))
    me = m.get("model_extern", {})
    for sub in ("unclassified", "kept"):
        v = me.get(sub, [])
        if len(v):
            problems.append("model_extern.%s=%d" % (sub, len(v)))
    if problems:
        raise PackError(kind + " red: " + "; ".join(problems))


def build_pack(kind: str, fkind: int, m: dict, raw: bytes):
    gate_metadata(kind, m)
    checks = m.get("checks", {})

    # Asset ids: Main from intern edges, Model from reloc filename, tails explicit.
    main_fids = {r["target_file"] for r in m["main_intern"]["retained"]}
    if len(main_fids) != 1:
        raise PackError(kind + ": main asset id ambiguous: %r" % (main_fids,))
    main_fid = main_fids.pop()
    reloc_file = m["model_intern"].get("reloc_file", "")
    model_fid = int(reloc_file.split("_")[0])
    tails = [(t["fid"], t["bytes"]) for t in m.get("tail_files", [])]
    if len(tails) + 2 > MAX_SECTIONS:
        raise PackError(kind + ": too many sections")

    # Section 0 extent: Main-image identity tiling, must cover [0, main_len).
    main_secs = sorted(
        (s for s in m["sections"] if s.get("src") == "Main-image"),
        key=lambda s: s["old"])
    if not main_secs:
        raise PackError(kind + ": no Main-image sections")
    main_len = m["section_boundaries"]["main"][1]
    if main_secs[0]["old"] != 0:
        raise PackError(kind + ": main tiling starts at %d"
                        % main_secs[0]["old"])
    cursor = 0
    for s in main_secs:
        if s["old"] != cursor or s["new"] != s["old"] or s["mode"] != "identity":
            raise PackError(kind + ": main tiling hole at %d: %r"
                            % (cursor, s))
        cursor += s["len"]
    if cursor != main_len:
        raise PackError(kind + ": main tiling end %d != %d" % (cursor, main_len))
    if checks.get("main_sections_sum") not in (None, main_len):
        raise PackError(kind + ": main_sections_sum changed meaning")
    main_source_bytes = main_len
    main_body = raw[0:main_len]
    if len(main_body) != main_len:
        raise PackError(kind + ": compact bin short for main")

    # Section 1 spans: pointer_map order, verified inside the compact bin.
    model_spans = m["pointer_map"]  # [{old, len, new}] compact-relative
    model_source_bytes = checks.get("model_payload_bytes")
    if not model_source_bytes:
        raise PackError(kind + ": missing model_payload_bytes")
    span_total = sum(s["len"] for s in model_spans)
    if max(s["old"] + s["len"] for s in model_spans) > model_source_bytes:
        raise PackError(kind + ": model span exceeds source extent")

    def model_remap(off: int) -> int:
        """Compact-bin offset -> section-1-relative offset."""
        rel = 0
        for s in model_spans:
            if s["new"] <= off < s["new"] + s["len"]:
                return rel + (off - s["new"])
            rel += s["len"]
        raise PackError(kind + ": model offset %d outside kept spans" % off)

    # Root cells: unique pruned-DL targets referenced by retained slots.
    proots = {r["target_old"] for r in m.get("pruned_dl_roots", [])}
    need_cells = set()
    for r in m["model_intern"]["retained"]:
        if r.get("target_new") == "sentinel":
            if r.get("reason") == "pruned-dl":
                need_cells.add(r["target_old"])
            elif "target_old" not in r:
                raise PackError(kind + ": model sentinel without target: %r" % (r,))
    for r in m["main_extern"]["pruned"]:
        if r.get("reason") == "pruned-dl":
            need_cells.add(r["target_old"])
    missing = need_cells - proots
    if missing:
        raise PackError(kind + ": pruned-dl target outside measured roots: %r"
                        % (sorted(missing)[:5],))
    if any(t >= model_source_bytes for t in need_cells):
        raise PackError(kind + ": root cell beyond model source extent")
    cells = sorted(need_cells)
    cell_index = {t: i for i, t in enumerate(cells)}

    # Section bodies.
    model_body = b"".join(
        raw[s["new"]:s["new"] + s["len"]] for s in model_spans)
    if len(model_body) != span_total:
        raise PackError(kind + ": compact bin short for model spans")
    roots_body = b"".join(struct.pack(">2I", ENDDL, t) for t in cells)
    tail_bodies = []
    for fid, nbytes in tails:
        entry = next(s for s in m["sections"]
                     if s.get("src") == "file-%d" % fid)
        chunk = raw[entry["new"]:entry["new"] + entry["len"]]
        if len(chunk) != nbytes or entry["len"] != nbytes:
            raise PackError(kind + ": tail %d size drift" % fid)
        tail_bodies.append(chunk)

    bodies = [main_body, model_body + roots_body] + tail_bodies
    asset_ids = [main_fid, model_fid] + [fid for fid, _ in tails]
    source_bytes = [main_source_bytes, model_source_bytes] + [n for _, n in tails]

    # Section data offsets, 16-aligned; padding is zero and hashed.
    data_offsets = []
    off = 0
    for body in bodies:
        off = align_up(off, ALIGN)
        data_offsets.append(off)
        off += len(body)
    data_len = off
    data = bytearray(data_len)
    for base, body in zip(data_offsets, bodies):
        data[base:base + len(body)] = body

    sec1_base = data_offsets[1]
    roots_rel = span_total  # section-1-relative cell origin

    def cell_target(target_old: int) -> int:
        return sec1_base + roots_rel + cell_index[target_old] * 8

    # Fixups for every retained Main/Model slot.
    fixups = []  # (slot_data_rel, target_data_rel_or_NULL)
    seen_slots: dict = {}

    def emit(slot: int, target: int) -> None:
        # Scratch numeric metadata can list one edge twice (symbol + hex
        # alias lines); identical pairs dedupe, divergent targets fail.
        if slot in seen_slots:
            if seen_slots[slot] != target:
                raise PackError(kind + ": conflicting fixup slot %d" % slot)
            return
        seen_slots[slot] = target
        fixups.append((slot, target))

    sec0_base = data_offsets[0]
    for r in m["main_intern"]["retained"]:
        slot, target = r["slot_new"], r["target_new"]
        if not (0 <= slot < main_len and 0 <= target < main_len):
            raise PackError(kind + ": main intern out of bounds: %r" % (r,))
        emit(sec0_base + slot, sec0_base + target)
    tail_base = {}
    for i, (fid, nbytes) in enumerate(tails):
        tail_base[fid] = data_offsets[2 + i]
    for r in m["main_extern"]["kept"]:
        slot = sec0_base + r["slot_new"]
        if not (0 <= r["slot_new"] < main_len):
            raise PackError(kind + ": main kept slot out of bounds: %r" % (r,))
        fid = r["target_file"]
        if fid == model_fid:
            target = sec1_base + model_remap(r["target_new"])
        elif fid == main_fid:
            if not (0 <= r["target_new"] < main_len):
                raise PackError(kind + ": main->main target out of bounds")
            target = sec0_base + r["target_new"]
        elif fid in tail_base:
            entry = next(s for s in m["sections"]
                         if s.get("src") == "file-%d" % fid)
            rel = r["target_new"] - entry["new"]
            if not (0 <= rel < entry["len"]) or rel != r["target_old"]:
                raise PackError(kind + ": tail target drift: %r" % (r,))
            target = tail_base[fid] + rel
        else:
            raise PackError(kind + ": main kept target without section: %r" % (r,))
        emit(slot, target)
    for r in m["main_extern"]["pruned"]:
        slot = sec0_base + r["slot_new"]
        if not (0 <= r["slot_new"] < main_len):
            raise PackError(kind + ": main pruned slot out of bounds: %r" % (r,))
        if r.get("reason") == "pruned-dl":
            emit(slot, cell_target(r["target_old"]))
        else:
            emit(slot, NULL)
    for r in m["model_intern"]["retained"]:
        slot = sec1_base + model_remap(r["slot_new"])
        if not (0 <= r["slot_old"] < model_source_bytes):
            raise PackError(kind + ": model slot beyond source extent: %r" % (r,))
        if r.get("target_new") == "sentinel":
            if r.get("reason") == "pruned-dl":
                emit(slot, cell_target(r["target_old"]))
            else:
                emit(slot, NULL)
        else:
            if not (0 <= r["target_old"] < model_source_bytes):
                raise PackError(kind + ": model target beyond extent: %r" % (r,))
            emit(slot, sec1_base + model_remap(r["target_new"]))

    # Span table: original offset -> section-relative compact offset.
    spans = []  # (source_offset, data_offset, data_bytes)
    first_count = []
    for s in main_secs:
        first_count.append((len(spans), None))
        spans.append((s["old"], s["old"], s["len"]))
    first0, _ = first_count[0]
    n_main = len(main_secs)
    rel = 0
    model_first = len(spans)
    for s in model_spans:
        spans.append((s["old"], rel, s["len"]))
        rel += s["len"]
    n_model = len(model_spans)
    tail_first_counts = []
    for (fid, nbytes), entry in zip(
            tails,
            [next(s for s in m["sections"] if s.get("src") == "file-%d" % fid)
             for fid, _ in tails]):
        tail_first_counts.append((len(spans), 1))
        spans.append((0, 0, nbytes))

    sections = [
        (asset_ids[0], data_offsets[0], len(bodies[0]), source_bytes[0],
         first0, n_main, 0, 0),
        (asset_ids[1], data_offsets[1], len(bodies[1]), source_bytes[1],
         model_first, n_model, roots_rel, len(cells)),
    ]
    for (fid, _), (first, count), base, body, src in zip(
            tails, tail_first_counts, data_offsets[2:], bodies[2:],
            source_bytes[2:]):
        sections.append((fid, base, len(body), src, first, count, 0, 0))

    fixup_bytes = b"".join(struct.pack(FIXUP_FMT, s, t) for s, t in fixups)
    span_bytes = b"".join(struct.pack(SPAN_FMT, *s) for s in spans)
    header = struct.pack(
        HEADER_FMT, MAGIC, VERSION,
        64 + 32 * len(sections) + data_len + len(fixup_bytes) + len(span_bytes),
        fkind, len(sections), len(fixups), len(spans), 0,
        data_len, fnv1a32(bytes(data)), fnv1a32(fixup_bytes),
        fnv1a32(span_bytes), main_fid, model_fid, 0, 0)
    blob = (header
            + b"".join(struct.pack(SECTION_FMT, *s) for s in sections)
            + bytes(data) + fixup_bytes + span_bytes)
    meta = {
        "kind": kind,
        "fkind": fkind,
        "sections": len(sections),
        "fixups": len(fixups),
        "spans": len(spans),
        "root_cells": len(cells),
        "file_bytes": len(blob),
    }
    return blob, meta


def decode_pack(blob: bytes) -> dict:
    """Decode + fully validate an FPC1 blob. Raises PackError on any defect."""
    if len(blob) < 64:
        raise PackError("short header")
    h = struct.unpack(HEADER_FMT, blob[:64])
    (magic, version, file_bytes, fkind, nsec, nfix, nspan, _,
     data_bytes, data_hash, fixup_hash, span_hash,
     main_id, model_id, _, _) = h
    if magic != MAGIC:
        raise PackError("bad magic %08x" % magic)
    if version != VERSION:
        raise PackError("bad version %d" % version)
    if file_bytes != len(blob):
        raise PackError("file_bytes %d != actual %d" % (file_bytes, len(blob)))
    if nsec > MAX_SECTIONS or nsec < 2:
        raise PackError("bad section count %d" % nsec)
    secs = []
    for i in range(nsec):
        rec = struct.unpack(SECTION_FMT, blob[64 + 32 * i:64 + 32 * (i + 1)])
        secs.append(rec)
    data_base = 64 + 32 * nsec
    if data_base + data_bytes + 8 * nfix + 12 * nspan != len(blob):
        raise PackError("body size mismatch")
    data = blob[data_base:data_base + data_bytes]
    fixraw = blob[data_base + data_bytes:
                  data_base + data_bytes + 8 * nfix]
    spanraw = blob[data_base + data_bytes + 8 * nfix:]
    if fnv1a32(data) != data_hash:
        raise PackError("data hash mismatch")
    if fnv1a32(fixraw) != fixup_hash:
        raise PackError("fixup hash mismatch")
    if fnv1a32(spanraw) != span_hash:
        raise PackError("span hash mismatch")
    fixups = [struct.unpack(FIXUP_FMT, fixraw[8 * i:8 * (i + 1)])
              for i in range(nfix)]
    spans = [struct.unpack(SPAN_FMT, spanraw[12 * i:12 * (i + 1)])
             for i in range(nspan)]
    for i, (aid, doff, dbytes, src, first, count, roff, rcnt) in enumerate(secs):
        if doff % ALIGN:
            raise PackError("section %d unaligned offset" % i)
        if doff + dbytes > data_bytes:
            raise PackError("section %d overruns data" % i)
        if first + count > nspan:
            raise PackError("section %d span range overruns" % i)
        if i == 1:
            if roff + rcnt * 8 > dbytes:
                raise PackError("model roots overrun section")
        elif rcnt or roff:
            raise PackError("roots on non-model section %d" % i)
    # Section bodies must not overlap.
    ranges = sorted((doff, doff + dbytes) for _, doff, dbytes, *_ in secs)
    for (a0, a1), (b0, b1) in zip(ranges, ranges[1:]):
        if b0 < a1:
            raise PackError("section overlap")
    for slot, target in fixups:
        if not (0 <= slot < data_bytes and slot % 4 == 0):
            raise PackError("fixup slot %d out of bounds/unalinged" % slot)
        if target != NULL and not (0 <= target < data_bytes and target % 4 == 0):
            raise PackError("fixup target %d out of bounds" % target)
        if target != NULL and not any(
                doff <= target < doff + dbytes for _, doff, dbytes, *_ in secs):
            raise PackError("fixup target %d inside padding" % target)
    # Root cells: BE ENDDL + BE original offset, unique.
    _, mdoff, mdbytes, _, _, _, mroff, mrcnt = secs[1]
    seen_roots = set()
    for i in range(mrcnt):
        cell = data[mdoff + mroff + 8 * i:mdoff + mroff + 8 * (i + 1)]
        tag, root = struct.unpack(">2I", cell)
        if tag != ENDDL:
            raise PackError("root cell %d tag %08x" % (i, tag))
        if root in seen_roots:
            raise PackError("duplicate root cell %d" % root)
        seen_roots.add(root)
    return {"header": h, "sections": secs, "fixups": fixups, "spans": spans,
            "data": data, "roots": seen_roots}


def parse_kinds(arg: str | None) -> list:
    if not arg:
        return list(KIND_ORDER)
    out = []
    for tok in arg.split(","):
        tok = tok.strip().lower()
        if not tok:
            continue
        if tok.isdigit():
            idx = int(tok)
            if not (0 <= idx < len(KIND_ORDER)):
                raise PackError("bad fkind %s" % tok)
            out.append(KIND_ORDER[idx])
        elif tok in KIND_ORDER:
            out.append(tok)
        else:
            raise PackError("unknown kind %s" % tok)
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="Encode FPC1 preview core packs")
    ap.add_argument("--input-dir", default=None,
                    help="reuse explicitly supplied MCM2 metadata instead of regenerating source")
    ap.add_argument("--output-dir", required=True)
    ap.add_argument("--kinds", default=None,
                    help="comma list of kind names or fkind numbers")
    args = ap.parse_args(argv)
    try:
        kinds = parse_kinds(args.kinds)
    except PackError as e:
        print("error: %s" % e)
        return 2
    os.makedirs(args.output_dir, exist_ok=True)
    if args.input_dir is None:
        import preview_source_metadata
        args.input_dir = os.path.join(args.output_dir, "source-metadata")
        result = preview_source_metadata.generate(args.input_dir, kinds)
        if not result["ok"]:
            print("source metadata contains unresolved entries; affected packs will be refused")
    failed = False
    for kind in kinds:
        fkind = KIND_ORDER.index(kind)
        try:
            m, raw = load_kind(args.input_dir, kind)
            blob, meta = build_pack(kind, fkind, m, raw)
            decode_pack(blob)  # self-check before writing
            with open(os.path.join(args.output_dir, "%02d.fpc" % fkind),
                      "wb") as f:
                f.write(blob)
            print("%02d.fpc %-8s bytes=%d sections=%d fixups=%d spans=%d "
                  "roots=%d" % (fkind, kind, meta["file_bytes"],
                                 meta["sections"], meta["fixups"],
                                 meta["spans"], meta["root_cells"]))
        except PackError as e:
            print("%02d.fpc %-8s FAILED: %s" % (fkind, kind, e))
            failed = True
        except (OSError, KeyError, ValueError, struct.error) as e:
            print("%02d.fpc %-8s FAILED: %s: %s"
                  % (fkind, kind, type(e).__name__, e))
            failed = True
    if failed:
        print("result: INCOMPLETE, no all-complete claim")
        return 1
    print("result: requested set complete")
    return 0


if __name__ == "__main__":
    sys.exit(main())
