#!/usr/bin/env python3
"""Prove an MFP2 pack with the runtime decoder and roster metadata.

The decoder under test is src/nds/nds_motion_mf.c itself, compiled for the
host (scripts/motion/host/mf_host.c includes it verbatim) and driven exactly as
the ROM will drive it: pack header -> expanded tables -> directory row ->
stream -> output buffer. The oracle is the BPS1 clip image, produced without
MF: the pack producer's bytes (assets/animation/ftanim_stream_pack.bin) and,
for the kinds the BPS1 pack does not carry, the producer's own normaliser over
the O2R files (mf_corpus.build_corpus, which proves that re-emitter against
every packed clip first). An oracle sharing the decoder would prove nothing.

Per clip: decoded bytes == oracle bytes; bytes written == directory size;
bits consumed == directory stream bits (no over-read); CRC32 == directory.
Per table: Kraft sum == 1 (a one-symbol table: 1/2), code lengths <= 12, every
LUT entry agrees with the canonical decode.

Usage:
  python scripts/motion/check_mf_pack.py PACK [--json OUT] [--cc gcc]
Exit 0 only when everything passes.
"""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import pathlib
import struct
import subprocess
import sys
import tempfile
import zlib
from fractions import Fraction

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
import mf_corpus as mc  # noqa: E402
import mf_residency as mr  # noqa: E402

HEADER = struct.Struct("<IHHIIIIIIIIII")
KIND_ROW = struct.Struct("<8sHHI")
ENTRY = struct.Struct("<IIIIIBBHHH")
RAW_ENTRY = struct.Struct("<IIIIHBB")

PACK_MAGIC = 0x3250464D
PACK_VERSION = 2
KIND_MASK = (1 << len(mc.KINDS)) - 1


def validator_fingerprint():
    digest = hashlib.sha256()
    for relative in (
        "scripts/motion/check_mf_pack.py", "scripts/motion/mf_residency.py",
        "scripts/motion/mf_corpus.py", "scripts/motion/host/mf_host.c",
        "src/nds/nds_motion_mf.c", "include/nds/nds_motion_mf.h",
    ):
        digest.update(relative.encode() + b"\0")
        digest.update((ROOT / relative).read_bytes())
    return digest.hexdigest()


def align4(value):
    return (value + 3) & ~3


def span_valid(offset, size, total):
    return 0 <= offset <= total and 0 <= size <= total - offset


def build_host_library(cc, out_dir):
    ext = ".dll" if sys.platform == "win32" else ".so"
    lib = out_dir / ("mf_host" + ext)
    cmd = [cc, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", "-shared",
           "-DNDS_MF_HOST", "-I", str(ROOT / "include"),
           str(HERE / "host" / "mf_host.c"), "-o", str(lib)]
    if sys.platform != "win32":
        cmd.insert(1, "-fPIC")
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise SystemExit("host build failed:\n%s%s" % (r.stdout, r.stderr))
    return lib


def check_tables(blob):
    """Independent parse of the MFT1 blob: Kraft and length limits."""
    magic, nwords, nsucc, ntables, _z = struct.unpack_from("<IHHHH", blob, 0)
    if magic != 0x3154464D:
        return ["tables magic 0x%08x" % magic], 0
    pos = 12 + 512 + 2 * nwords
    for _ in range(nsucc):
        _ctx, ln = struct.unpack_from("<HH", blob, pos)
        pos += 4 + 2 * ln
    errors = []
    for _ in range(ntables):
        cls, ctx, nsym, nesc = struct.unpack_from("<BBHH", blob, pos)
        counts = struct.unpack_from("<12H", blob, pos + 6)
        pos += 30 + 2 * nsym + 2 * nesc
        if sum(counts) != nsym:
            errors.append("table %d/%d: counts %d != nsym %d" % (cls, ctx, sum(counts), nsym))
        kraft = sum(Fraction(n, 1 << (L + 1)) for L, n in enumerate(counts))
        want = Fraction(1, 2) if nsym == 1 else Fraction(1)
        if kraft != want:
            errors.append("table %d/%d: Kraft sum %s != %s" % (cls, ctx, kraft, want))
    if pos != len(blob):
        errors.append("tables blob: parsed %d of %d bytes" % (pos, len(blob)))
    return errors, ntables


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("pack", type=pathlib.Path)
    ap.add_argument("--json", type=pathlib.Path)
    ap.add_argument("--cc", default="gcc")
    a = ap.parse_args()

    pack = a.pack.read_bytes()
    identity = {"pack_sha256": hashlib.sha256(pack).hexdigest(),
                "validator_sha256": validator_fingerprint()}
    errors = []
    if len(pack) < HEADER.size:
        raise SystemExit("truncated MFP2 header")
    (magic, version, nkinds, tables_off, tables_bytes, dir_off, dir_count,
     kind_off, by_id_off, data_off, data_bytes, raw_off, raw_count) = HEADER.unpack_from(pack, 0)
    if magic != PACK_MAGIC or version != PACK_VERSION:
        raise SystemExit("not an MFP2 pack")

    # Validate the serialized section graph before unpacking attacker-controlled
    # counts or passing any pointer to the host C decoder.
    expected_kind_off = HEADER.size
    expected_tables_off = align4(expected_kind_off + KIND_ROW.size * nkinds)
    expected_dir_off = align4(expected_tables_off + tables_bytes)
    expected_by_id_off = expected_dir_off + ENTRY.size * dir_count
    expected_raw_off = align4(expected_by_id_off + 2 * dir_count)
    expected_data_off = align4(expected_raw_off + RAW_ENTRY.size * raw_count)
    for label, got, want in (
        ("kind_off", kind_off, expected_kind_off),
        ("tables_off", tables_off, expected_tables_off),
        ("dir_off", dir_off, expected_dir_off),
        ("by_id_off", by_id_off, expected_by_id_off),
        ("raw_off", raw_off, expected_raw_off),
        ("data_off", data_off, expected_data_off),
    ):
        if got != want:
            errors.append("%s %d != expected %d" % (label, got, want))
    if data_off + data_bytes != len(pack):
        errors.append("data span %d+%d != pack %d" % (data_off, data_bytes, len(pack)))
    if nkinds != len(mc.KINDS):
        errors.append("kind count %d != corpus %d" % (nkinds, len(mc.KINDS)))

    kind_span = span_valid(kind_off, KIND_ROW.size * nkinds, len(pack))
    table_span = span_valid(tables_off, tables_bytes, len(pack))
    dir_span = span_valid(dir_off, ENTRY.size * dir_count, len(pack))
    by_id_span = span_valid(by_id_off, 2 * dir_count, len(pack))
    raw_span = span_valid(raw_off, RAW_ENTRY.size * raw_count, len(pack))
    data_span = span_valid(data_off, data_bytes, len(pack))
    if not all((kind_span, table_span, dir_span, by_id_span, raw_span, data_span)):
        errors.append("one or more MFP2 sections lie outside the pack")
        result = {**identity, "pack": str(a.pack), "pack_bytes": len(pack),
                  "passed": 0, "clips": dir_count, "errors": errors,
                  "error_count": len(errors)}
        if a.json:
            a.json.write_text(json.dumps(result, indent=1))
        print("MFP2_CHECK=FAIL: " + "; ".join(errors[:8]))
        return 1

    try:
        t_err, ntables = check_tables(pack[tables_off:tables_off + tables_bytes])
        errors += t_err
    except (struct.error, ValueError, OverflowError) as exc:
        ntables = 0
        errors.append("malformed MFT1 tables: %s" % exc)

    corpus = mc.load_corpus(log=lambda *x: None)
    identity["corpus_inputs_sha256"] = corpus["inputs_sha256"]
    oracle = {aid: c["bytes"] for aid, c in corpus["clips"].items()}
    try:
        raw_oracle = mr.raw_exception_inventory(corpus)
    except (OSError, ValueError, SystemExit) as exc:
        raw_oracle = []
        errors.append("raw exception inventory failed: %s" % exc)
    main_users = mr.main_user_masks(corpus)

    entries = [ENTRY.unpack_from(pack, dir_off + ENTRY.size * i) for i in range(dir_count)]
    kinds = [KIND_ROW.unpack_from(pack, kind_off + KIND_ROW.size * i) for i in range(nkinds)]
    raw_entries = [RAW_ENTRY.unpack_from(pack, raw_off + RAW_ENTRY.size * i)
                   for i in range(raw_count)]
    by_id = struct.unpack_from("<%dH" % dir_count, pack, by_id_off)
    if sorted(by_id) != list(range(dir_count)):
        errors.append("by-id index is not a permutation of directory rows")
    elif [entries[i][0] for i in by_id] != sorted(e[0] for e in entries):
        errors.append("by-id index is not sorted by asset id")

    covered = 0
    for ki, (name, first, count, _r) in enumerate(kinds):
        if ki >= len(mc.KINDS):
            errors.append("unexpected extra kind row %d" % ki)
            continue
        want_name = mc.KINDS[ki].encode()[:8]
        if name.rstrip(b"\0") != want_name:
            errors.append("kind %d name %r != %r" % (ki, name, want_name))
        if _r != 0:
            errors.append("kind %d reserved field is nonzero" % ki)
        if first != covered:
            errors.append("kind %d starts at %d after coverage %d" % (ki, first, covered))
        if first + count > dir_count:
            errors.append("kind %d span exceeds directory" % ki)
            continue
        for i in range(first, first + count):
            if entries[i][5] != ki:
                errors.append("entry %d kind %d outside kind %d's span" % (i, entries[i][5], ki))
        covered += count
    if covered != dir_count:
        errors.append("kind spans cover %d of %d entries" % (covered, dir_count))

    expected_order = sorted(
        range(dir_count),
        key=lambda i: (entries[i][5], entries[i][6], entries[i][7], entries[i][0]),
    )
    if expected_order != list(range(dir_count)):
        errors.append("directory is not sorted by kind/class/need-mask/asset-id")
    entry_ids = [e[0] for e in entries]
    if len(set(entry_ids)) != len(entry_ids):
        errors.append("duplicate compressed asset ids in directory")
    invalid_stream_ids = set()
    for i, (aid, rel, bits, size, crc, ki, cls, need_mask, user_mask, reserved) in enumerate(entries):
        if ki >= nkinds or ki >= len(mc.KINDS):
            errors.append("entry %d has invalid kind %d" % (i, ki))
            continue
        if cls not in (mr.CLS_ALWAYS, mr.CLS_VICTIM, mr.CLS_COPY):
            errors.append("entry 0x%x has invalid class %d" % (aid, cls))
        if need_mask & ~KIND_MASK or user_mask & ~KIND_MASK:
            errors.append("entry 0x%x has out-of-range kind mask" % aid)
        if cls == mr.CLS_ALWAYS and need_mask != 0:
            errors.append("always entry 0x%x has a nonzero need mask" % aid)
        if cls in (mr.CLS_VICTIM, mr.CLS_COPY) and need_mask == 0:
            errors.append("conditional entry 0x%x has an empty need mask" % aid)
        if reserved != 0:
            errors.append("entry 0x%x reserved field is nonzero" % aid)
        if aid in oracle:
            expected_kind = mc.KINDS.index(corpus["clips"][aid]["bank"])
            if ki != expected_kind:
                errors.append("entry 0x%x owner kind %d != source %d" %
                              (aid, ki, expected_kind))
            expected_users = main_users.get(aid, 0)
            if user_mask != expected_users:
                errors.append("entry 0x%x main-user mask 0x%x != 0x%x" %
                              (aid, user_mask, expected_users))
            want_cls, want_need = mr.classify_clip(corpus, mc.KINDS[ki], aid)
            if (cls, need_mask) != (want_cls, want_need):
                errors.append("entry 0x%x class/mask %d/0x%x != %d/0x%x" %
                              (aid, cls, need_mask, want_cls, want_need))
        if size <= 0 or bits <= 0:
            errors.append("entry 0x%x has empty stream or decoded clip" % aid)
            invalid_stream_ids.add(aid)
            continue
        stream_bytes = (bits + 7) // 8
        padded_bytes = align4(stream_bytes)
        if (rel & 3) != 0 or bits > padded_bytes * 8 or rel + padded_bytes > data_bytes:
            errors.append("entry 0x%x stream span is invalid" % aid)
            invalid_stream_ids.add(aid)
        elif pack[data_off + rel + stream_bytes:data_off + rel + padded_bytes] != bytes(padded_bytes - stream_bytes):
            errors.append("entry 0x%x alignment padding is nonzero" % aid)
        if i == 0 and rel != 0:
            errors.append("first stream does not start at data offset zero")
        elif i > 0:
            prev = entries[i - 1]
            prev_end = prev[1] + align4((prev[2] + 7) // 8)
            if rel != prev_end:
                errors.append("entry 0x%x stream is not contiguous after prior row" % aid)

    stream_end = (entries[-1][1] + align4((entries[-1][2] + 7) // 8)
                  if entries else 0)
    if stream_end != data_bytes:
        errors.append("stream rows cover %d of %d data bytes" % (stream_end, data_bytes))

    missing = sorted(set(oracle) - {e[0] for e in entries})
    extra = sorted({e[0] for e in entries} - set(oracle))
    if missing:
        errors.append("%d oracle clips absent from the pack (first 0x%x)" % (len(missing), missing[0]))
    if extra:
        errors.append("%d pack clips without an oracle (first 0x%x)" % (len(extra), extra[0]))

    raw_by_id = {r[0]: r for r in raw_entries}
    if len(raw_by_id) != len(raw_entries):
        errors.append("duplicate raw exception ids")
    if set(raw_by_id) & set(entry_ids):
        errors.append("asset id appears in both MF and raw exception tables")
    expected_raw = {r["asset_id"]: r for r in raw_oracle}
    if set(raw_by_id) != set(expected_raw):
        errors.append("raw exception ids disagree with the explicit O2R inventory")
    if [r[0] for r in raw_entries] != sorted(
        raw_by_id, key=lambda aid: (raw_by_id[aid][5], aid)
    ):
        errors.append("raw exception rows are not sorted by kind/asset id")
    for aid, payload, source, source_crc, user_mask, ki, reason in raw_entries:
        want = expected_raw.get(aid)
        if want is None:
            continue
        if ki >= nkinds or ki >= len(mc.KINDS) or mc.KINDS[ki] != want["kind"]:
            errors.append("raw exception 0x%x owner kind changed" % aid)
        if (payload, source, source_crc, user_mask, ki, reason) != (
            want["payload_bytes"], want["source_bytes"], want["source_crc32"],
            want["main_user_mask"], want["kind_index"], want["reason"],
        ):
            errors.append("raw exception 0x%x metadata differs from O2R source" % aid)

    # A malformed directory is already a failed pack. Do not hand it to the C
    # bridge, which exists to compare valid byte streams rather than parse an
    # unchecked file format. Source/ownership errors cannot become a decode PASS.
    if errors:
        result = {**identity, "pack": str(a.pack), "pack_bytes": len(pack),
                  "passed": 0, "clips": dir_count, "phase": "metadata",
                  "errors": errors[:50], "error_count": len(errors)}
        if a.json:
            a.json.write_text(json.dumps(result, indent=1))
        print("MFP2_CHECK=FAIL metadata: " + "; ".join(errors[:8]))
        return 1

    # the loaded DLL stays locked on Windows until exit, so cleanup may fail
    with tempfile.TemporaryDirectory(ignore_cleanup_errors=True) as td:
        lib = ctypes.CDLL(str(build_host_library(a.cc, pathlib.Path(td))))
        lib.mfh_init.argtypes = [ctypes.c_char_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
        lib.mfh_decode.argtypes = [ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32,
                                   ctypes.POINTER(ctypes.c_uint32)]
        storage = ctypes.c_uint32(0)
        buf = ctypes.create_string_buffer(pack, len(pack))
        rc = lib.mfh_init(buf, len(pack), ctypes.byref(storage))
        decoder_ready = rc == 0
        if not decoder_ready:
            errors.append("mfh_init failed: %d" % rc)
        else:
            lut_bad = lib.mfh_lut_selfcheck()
            if lut_bad:
                errors.append("%d LUT entries disagree with the canonical decode" % lut_bad)
        passed = 0
        per_kind = {}
        for i, row in enumerate(entries):
            aid, rel, bits, size, crc, ki, cls, mask = row[:8]
            want = oracle.get(aid)
            if want is None:
                continue
            if aid in invalid_stream_ids:
                continue
            if size != len(want):
                errors.append("clip 0x%x directory size %d != oracle %d" %
                              (aid, size, len(want)))
                continue
            if not decoder_ready:
                continue
            out = ctypes.create_string_buffer(size + 64)
            used = ctypes.c_uint32(0)
            n = lib.mfh_decode(i, out, size, ctypes.byref(used))
            got = out.raw[:max(n, 0)]
            problems = []
            if n < 0:
                problems.append("decoder error %d" % n)
            elif n != size:
                problems.append("wrote %d, directory says %d" % (n, size))
            if used.value != bits:
                problems.append("consumed %d bits, stream has %d" % (used.value, bits))
            if want is None or got != want:
                problems.append("bytes differ from the BPS1 oracle")
            if (zlib.crc32(got) & 0xFFFFFFFF) != crc:
                problems.append("CRC32 mismatch")
            if out.raw[size:size + 64] != bytes(64):
                problems.append("wrote past the directory size")
            if ki >= len(kinds):
                continue
            k = kinds[ki][0].rstrip(b"\0").decode()
            pk = per_kind.setdefault(k, {"clips": 0, "bps1": 0, "mf": 0, "max_decoded": 0})
            pk["clips"] += 1
            pk["bps1"] += size
            pk["mf"] += ((bits + 7) // 8 + 3) & ~3
            pk["max_decoded"] = max(pk["max_decoded"], size)
            if problems:
                errors.append("clip 0x%x (%s): %s" % (aid, k, "; ".join(problems)))
            else:
                passed += 1

    try:
        arm_tables_bytes = mr.mft1_arm_storage_bytes(pack[tables_off:tables_off + tables_bytes])
    except ValueError as exc:
        arm_tables_bytes = 0
        errors.append("cannot size ARM MFT1 storage: %s" % exc)
    raw_payload_bytes = sum(r[1] for r in raw_entries)
    raw_source_bytes = sum(r[2] for r in raw_entries)
    result = {**identity, "pack": str(a.pack), "pack_bytes": len(pack),
              "metadata_bytes": data_off, "tables_bytes": tables_bytes,
              "tables": ntables, "expanded_table_bytes_host": storage.value,
              "expanded_table_bytes_arm": arm_tables_bytes,
              "nds_mf_tables_object_bytes_arm": mr.ARM_NDS_MF_TABLES_BYTES,
              "clips": dir_count, "passed": passed,
              "raw_exceptions": raw_count, "raw_payload_bytes": raw_payload_bytes,
              "raw_source_bytes": raw_source_bytes, "errors": errors[:50],
              "error_count": len(errors), "per_kind": per_kind}
    if a.json:
        a.json.write_text(json.dumps(result, indent=1))
    status = "PASS" if not errors else "FAIL"
    print("MF_PACK_CHECK=%s clips %d/%d byte-exact with the C decoder, "
          "raw exceptions %d, tables %d (blob %d B, host expanded %d B, "
          "ARM expanded %d B), metadata %d B, pack %d B" % (
              status, passed, dir_count, raw_count, ntables, tables_bytes,
              storage.value, arm_tables_bytes, data_off, len(pack)))
    for e in errors[:20]:
        print("  " + e)
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main())
