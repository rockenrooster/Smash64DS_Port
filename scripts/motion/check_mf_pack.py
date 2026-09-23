#!/usr/bin/env python3
"""Prove an MF1 pack with the runtime decoder: every clip of every kind.

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

HEADER = struct.Struct("<IHHIIIIIIII8x")
KIND_ROW = struct.Struct("<8sHHI")
ENTRY = struct.Struct("<IIIIIBBH")


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
    (magic, version, nkinds, tables_off, tables_bytes, dir_off, dir_count,
     kind_off, by_id_off, data_off, data_bytes) = HEADER.unpack_from(pack, 0)
    errors = []
    if magic != 0x3150464D or version != 1:
        raise SystemExit("not an MF1 pack")
    if data_off + data_bytes != len(pack):
        errors.append("data span %d+%d != pack %d" % (data_off, data_bytes, len(pack)))
    t_err, ntables = check_tables(pack[tables_off:tables_off + tables_bytes])
    errors += t_err

    corpus = mc.load_corpus(log=lambda *x: None)
    oracle = {aid: c["bytes"] for aid, c in corpus["clips"].items()}

    entries = [ENTRY.unpack_from(pack, dir_off + ENTRY.size * i) for i in range(dir_count)]
    kinds = [KIND_ROW.unpack_from(pack, kind_off + KIND_ROW.size * i) for i in range(nkinds)]
    by_id = struct.unpack_from("<%dH" % dir_count, pack, by_id_off)
    if [entries[i][0] for i in by_id] != sorted(e[0] for e in entries):
        errors.append("by-id index is not sorted by asset id")
    covered = 0
    for ki, (name, first, count, _r) in enumerate(kinds):
        for i in range(first, first + count):
            if entries[i][5] != ki:
                errors.append("entry %d kind %d outside kind %d's span" % (i, entries[i][5], ki))
        covered += count
    if covered != dir_count:
        errors.append("kind spans cover %d of %d entries" % (covered, dir_count))
    missing = sorted(set(oracle) - {e[0] for e in entries})
    extra = sorted({e[0] for e in entries} - set(oracle))
    if missing:
        errors.append("%d oracle clips absent from the pack (first 0x%x)" % (len(missing), missing[0]))
    if extra:
        errors.append("%d pack clips without an oracle (first 0x%x)" % (len(extra), extra[0]))

    # the loaded DLL stays locked on Windows until exit, so cleanup may fail
    with tempfile.TemporaryDirectory(ignore_cleanup_errors=True) as td:
        lib = ctypes.CDLL(str(build_host_library(a.cc, pathlib.Path(td))))
        lib.mfh_init.argtypes = [ctypes.c_char_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
        lib.mfh_decode.argtypes = [ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32,
                                   ctypes.POINTER(ctypes.c_uint32)]
        storage = ctypes.c_uint32(0)
        buf = ctypes.create_string_buffer(pack, len(pack))
        rc = lib.mfh_init(buf, len(pack), ctypes.byref(storage))
        if rc != 0:
            raise SystemExit("mfh_init failed: %d" % rc)
        lut_bad = lib.mfh_lut_selfcheck()
        if lut_bad:
            errors.append("%d LUT entries disagree with the canonical decode" % lut_bad)
        passed = 0
        per_kind = {}
        for i, (aid, rel, bits, size, crc, ki, cls, mask) in enumerate(entries):
            want = oracle.get(aid)
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

    result = {"pack": str(a.pack), "pack_bytes": len(pack), "tables_bytes": tables_bytes,
              "tables": ntables, "expanded_table_bytes": storage.value,
              "clips": dir_count, "passed": passed, "errors": errors[:50],
              "error_count": len(errors), "per_kind": per_kind}
    if a.json:
        a.json.write_text(json.dumps(result, indent=1))
    status = "PASS" if not errors else "FAIL"
    print("MF_PACK_CHECK=%s clips %d/%d byte-exact with the C decoder, tables %d "
          "(blob %d B, expanded %d B), pack %d B" % (
              status, passed, dir_count, ntables, tables_bytes, storage.value, len(pack)))
    for e in errors[:20]:
        print("  " + e)
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main())
