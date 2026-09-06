#!/usr/bin/env python3
"""Tests for the FPC1 core-pack encoder (generate_preview_core_packs).

Decodes produced packs, verifies pointer coverage, root identities, span
mapping, address bounds and hashes against generated source metadata,
rejects corrupted metadata/bytes, and covers all twelve fighters.

Fixtures are generated in temporary directories from repo reference source
via preview_source_metadata plus the encoder. No ignored builds inputs.

Run:
    python -m pytest scripts/fighters/test_preview_core_packs.py -q
"""

from __future__ import annotations

import json
import os
import struct
import sys
import tempfile
import unittest

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import generate_preview_core_packs as gen
import preview_source_metadata as srcgen

CLEAN = list(gen.KIND_ORDER)


def expected_slots(kind, m, raw_main_len, model_spans, sec_bases, cells):
    """Data-relative (slot, target-or-NULL) pairs the maps require."""
    sec0, sec1 = sec_bases[0], sec_bases[1]
    rel = {}
    base = 0
    key = []
    for s in model_spans:
        key.append((s["new"], s["new"] + s["len"], base))
        base += s["len"]

    def remap(off):
        for lo, hi, b in key:
            if lo <= off < hi:
                return b + (off - lo)
        raise AssertionError("%s model offset %d uncovered" % (kind, off))

    want = {}
    model_fid = int(m["model_intern"]["reloc_file"].split("_")[0])
    main_fid = next(iter({r["target_file"]
                          for r in m["main_intern"]["retained"]}))
    tail_new = {}
    for s in m["sections"]:
        if s.get("src", "").startswith("file-"):
            tail_new[int(s["src"].split("-")[1])] = s["new"]
    for r in m["main_intern"]["retained"]:
        want[sec0 + r["slot_new"]] = sec0 + r["target_new"]
    for r in m["main_extern"]["kept"]:
        slot = sec0 + r["slot_new"]
        fid = r["target_file"]
        if fid == model_fid:
            want[slot] = sec1 + remap(r["target_new"])
        elif fid == main_fid:
            want[slot] = sec0 + r["target_new"]
        else:
            want[slot] = sec_bases[2 + [t[0] for t in
                                        [(t["fid"], t["bytes"])
                                         for t in m["tail_files"]]].index(fid)] \
                + (r["target_new"] - tail_new[fid])
    for r in m["main_extern"]["pruned"]:
        slot = sec0 + r["slot_new"]
        if r.get("reason") == "pruned-dl":
            want[slot] = sec1 + rel_model_roots(m, model_spans) \
                + cells[r["target_old"]] * 8
        else:
            want[slot] = gen.NULL
    for r in m["model_intern"]["retained"]:
        slot = sec1 + remap(r["slot_new"])
        if r.get("target_new") == "sentinel":
            if r.get("reason") == "pruned-dl":
                want.setdefault(slot, sec1 + rel_model_roots(m, model_spans)
                                + cells[r["target_old"]] * 8)
            else:
                want.setdefault(slot, gen.NULL)
        else:
            want.setdefault(slot, sec1 + remap(r["target_new"]))
    return want


def rel_model_roots(m, model_spans):
    return sum(s["len"] for s in model_spans)


class CorePackTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Measured result: source generation for all 12 kinds takes about
        # 2.5 seconds on host. One generation serves every test in this
        # class, including Ness's recovered relocation symbols.
        cls.work = tempfile.mkdtemp(prefix="fpc1-test-")
        cls.src_dir = os.path.join(cls.work, "source")
        cls.tmp = os.path.join(cls.work, "packs")
        os.makedirs(cls.src_dir, exist_ok=True)
        os.makedirs(cls.tmp, exist_ok=True)
        src_result = srcgen.generate(cls.src_dir, list(gen.KIND_ORDER))
        assert any(k["fighter"] == "Ness" for k in src_result["kinds"]), \
            "source generation must include Ness metadata"
        rc = gen.main(["--input-dir", cls.src_dir, "--output-dir", cls.tmp,
                       "--kinds", ",".join(CLEAN)])
        assert rc == 0, "clean subset must encode"
        cls.packs = {}
        cls.maps = {}
        for kind in CLEAN:
            with open(os.path.join(cls.tmp, "%02d.fpc" % gen.KIND_ORDER.index(kind)),
                      "rb") as f:
                cls.packs[kind] = f.read()
            with open(os.path.join(cls.src_dir, kind + "_compact_map.json"),
                      encoding="utf-8") as f:
                cls.maps[kind] = json.load(f)

    def test_header_abi(self):
        for kind, blob in self.packs.items():
            d = gen.decode_pack(blob)
            (magic, version, fbytes, fkind, nsec, nfix, nspan, _, dbytes,
             *_rest) = d["header"]
            self.assertEqual(magic, gen.MAGIC, kind)
            self.assertEqual(version, 1, kind)
            self.assertEqual(fbytes, len(blob), kind)
            self.assertEqual(fkind, gen.KIND_ORDER.index(kind), kind)
            self.assertEqual(dbytes, len(d["data"]), kind)
            self.assertEqual(nfix, len(d["fixups"]), kind)
            self.assertEqual(nspan, len(d["spans"]), kind)
            self.assertEqual(len(blob),
                             64 + 32 * nsec + dbytes + 8 * nfix + 12 * nspan,
                             kind)

    def test_sections_match_source_identity(self):
        for kind, blob in self.packs.items():
            m = self.maps[kind]
            d = gen.decode_pack(blob)
            secs = d["sections"]
            model_fid = int(m["model_intern"]["reloc_file"].split("_")[0])
            main_fid = next(iter({r["target_file"]
                                  for r in m["main_intern"]["retained"]}))
            self.assertEqual(secs[0][0], main_fid, kind)
            self.assertEqual(secs[1][0], model_fid, kind)
            main_len = m["section_boundaries"]["main"][1]
            self.assertEqual(secs[0][3], main_len, kind)  # source extent
            self.assertEqual(secs[1][3], m["checks"]["model_payload_bytes"], kind)
            self.assertEqual(secs[0][2], main_len, kind)  # compact identity
            span_total = sum(s["len"] for s in m["pointer_map"])
            self.assertEqual(secs[1][2], span_total + 8 * secs[1][7], kind)
            self.assertEqual(secs[1][6], span_total, kind)  # roots origin
            for i, s in enumerate(secs):
                self.assertEqual(s[1] % 16, 0, (kind, i))
            tails = m.get("tail_files", [])
            self.assertEqual(len(secs), 2 + len(tails), kind)
            for s, t in zip(secs[2:], tails):
                self.assertEqual(s[0], t["fid"], kind)
                self.assertEqual(s[2], t["bytes"], kind)
                self.assertEqual(s[3], t["bytes"], kind)

    def test_main_fields_have_no_container_header_shift(self):
        # BattleShip 203_MarioMain.c: dead/dead-slam voices are 439/292.
        # FTAttributes.dead_fgm_ids is at 0xb4 (the live port ABI assertion).
        # This oracle is independent of load_kind's slicing: its old code
        # copied MCM2 into Main and put two floats at this exact field.
        data = gen.decode_pack(self.packs["mario"])["data"]
        attr = self.maps["mario"]["main_intern"]["attr_base"]
        self.assertEqual(struct.unpack_from(">HH", data, attr + 0xb4), (439, 292))
        self.assertNotEqual(data[:4], b"MCM2")

    def test_pointer_coverage_and_targets(self):
        for kind, blob in self.packs.items():
            m = self.maps[kind]
            d = gen.decode_pack(blob)
            secs = d["sections"]
            bases = [s[1] for s in secs]
            data_len = len(d["data"])
            # Expected root cell order: sorted unique pruned-dl targets.
            need = set()
            for r in m["model_intern"]["retained"]:
                if r.get("target_new") == "sentinel" \
                        and r.get("reason") == "pruned-dl":
                    need.add(r["target_old"])
            for r in m["main_extern"]["pruned"]:
                if r.get("reason") == "pruned-dl":
                    need.add(r["target_old"])
            cells = {t: i for i, t in enumerate(sorted(need))}
            self.assertEqual(len(d["roots"]), len(cells), kind)
            self.assertEqual(d["roots"], set(cells), kind)
            want = expected_slots(kind, m, secs[0][2], m["pointer_map"],
                                  bases, cells)
            got = dict(d["fixups"])
            self.assertEqual(sorted(got), sorted(want), kind)
            for slot, target in want.items():
                self.assertEqual(got[slot], target, (kind, slot))
            # Every non-null target lands in a section body, 4-aligned.
            bodies = [(s[1], s[1] + s[2]) for s in secs]
            for slot, target in d["fixups"]:
                self.assertTrue(any(lo <= slot < hi for lo, hi in bodies),
                                (kind, slot))
                if target != gen.NULL:
                    self.assertEqual(target % 4, 0, (kind, target))
                    self.assertTrue(any(lo <= target < hi for lo, hi in bodies),
                                    (kind, target))

    def test_root_identities_preserve_original_offsets(self):
        for kind, blob in self.packs.items():
            d = gen.decode_pack(blob)
            _, mdoff, _mdbytes, _, _, _, mroff, mrcnt = d["sections"][1]
            for i in range(mrcnt):
                cell = d["data"][mdoff + mroff + 8 * i:
                                 mdoff + mroff + 8 * (i + 1)]
                tag, root = struct.unpack(">2I", cell)
                self.assertEqual(tag, 0xDF000000, (kind, i))
                self.assertIn(root, d["roots"], (kind, i))
            # No shared sentinel: every cell carries its own original offset.
            self.assertEqual(mrcnt, len(d["roots"]), kind)

    def test_span_table_maps_sources(self):
        for kind, blob in self.packs.items():
            m = self.maps[kind]
            d = gen.decode_pack(blob)
            secs = d["sections"]
            spans = d["spans"]
            main_len = m["section_boundaries"]["main"][1]
            s0 = spans[secs[0][4]:secs[0][4] + secs[0][5]]
            self.assertEqual(s0[0][0], 0, kind)
            pos = 0
            for src, rel, ln in s0:  # Main/tails identity
                self.assertEqual(src, pos, kind)
                self.assertEqual(rel, pos, kind)
                pos += ln
            self.assertEqual(pos, main_len, kind)
            s1 = spans[secs[1][4]:secs[1][4] + secs[1][5]]
            self.assertEqual(len(s1), len(m["pointer_map"]), kind)
            rel = 0
            for (src, r, ln), pm in zip(s1, m["pointer_map"]):
                self.assertEqual((src, r, ln),
                                 (pm["old"], rel, pm["len"]), kind)
                rel += ln
            # Every retained model slot/target is span-covered.
            covered = [(s["old"], s["old"] + s["len"]) for s in m["pointer_map"]]
            for r in m["model_intern"]["retained"]:
                for off in (r["slot_old"], r["target_old"]):
                    if r.get("target_new") == "sentinel" and off == r["target_old"]:
                        continue
                    self.assertTrue(any(lo <= off < hi for lo, hi in covered),
                                    (kind, off))

    def test_source_body_ranges_match_compact_inputs(self):
        for kind, blob in self.packs.items():
            with open(os.path.join(self.src_dir, kind + "_compact.bin"), "rb") as f:
                raw = f.read()
            self.assertEqual(raw[:4], b"MCM2")
            self.assertEqual(struct.unpack_from("<I", raw, 4)[0], len(raw) - 8)
            raw = raw[8:]  # Section offsets exclude the container header.
            m = self.maps[kind]
            d = gen.decode_pack(blob)
            secs = d["sections"]
            main_len = m["section_boundaries"]["main"][1]
            self.assertEqual(d["data"][secs[0][1]:secs[0][1] + main_len],
                             raw[0:main_len], kind)
            rel = 0
            for s in m["pointer_map"]:
                want = raw[s["new"]:s["new"] + s["len"]]
                got = d["data"][secs[1][1] + rel:secs[1][1] + rel + s["len"]]
                self.assertEqual(got, want, (kind, s["old"]))
                rel += s["len"]
            for sec, t in zip(secs[2:], m.get("tail_files", [])):
                entry = next(s for s in m["sections"]
                             if s.get("src") == "file-%d" % t["fid"])
                self.assertEqual(
                    d["data"][sec[1]:sec[1] + sec[2]],
                    raw[entry["new"]:entry["new"] + entry["len"]], kind)

    def test_hashes_recomputed(self):
        import generate_preview_core_packs as g
        for kind, blob in self.packs.items():
            h = struct.unpack(g.HEADER_FMT, blob[:64])
            nsec, nfix, nspan = h[4], h[5], h[6]
            base = 64 + 32 * nsec
            data = blob[base:base + h[8]]
            fix = blob[base + h[8]:base + h[8] + 8 * nfix]
            span = blob[base + h[8] + 8 * nfix:]
            self.assertEqual(g.fnv1a32(data), h[9], kind)
            self.assertEqual(g.fnv1a32(fix), h[10], kind)
            self.assertEqual(g.fnv1a32(span), h[11], kind)

    def test_corruption_rejected(self):
        blob = self.packs["mario"]
        h = struct.unpack(gen.HEADER_FMT, blob[:64])
        nsec, data_len = h[4], h[8]
        data_base = 64 + 32 * nsec
        bad = bytearray(blob)
        bad[data_base + 4] ^= 0xFF
        with self.assertRaises(gen.PackError):
            gen.decode_pack(bytes(bad))
        bad = bytearray(blob)
        bad[data_base + data_len + 1] ^= 0xFF
        with self.assertRaises(gen.PackError):
            gen.decode_pack(bytes(bad))
        bad = bytearray(blob)
        bad[len(bad) - 1] ^= 0xFF
        with self.assertRaises(gen.PackError):
            gen.decode_pack(bytes(bad))
        bad = bytearray(blob)
        bad[0:4] = struct.pack("<I", 0xDEADBEEF)
        with self.assertRaises(gen.PackError):
            gen.decode_pack(bytes(bad))
        with self.assertRaises(gen.PackError):
            gen.decode_pack(blob[:len(blob) - 8])
        # Fixup retargeted into inter-section padding must fail.
        d = gen.decode_pack(blob)
        secs = sorted(d["sections"], key=lambda s: s[1])
        gap = None
        for (_, a0, n0, *_), (_, b0, *_x) in zip(secs, secs[1:]):
            if b0 > a0 + n0:
                gap = a0 + n0
                break
        if gap is not None:
            fix = bytearray()
            for s, t in d["fixups"]:
                fix += struct.pack("<2I", s, gap if t != gen.NULL else t)
                break
            bad = (blob[:data_base + data_len] + bytes(fix)
                   + blob[data_base + data_len + 8:])
            with self.assertRaises(gen.PackError):
                gen.decode_pack(bytes(bad))

    def test_no_absolute_paths_in_output(self):
        for kind, blob in self.packs.items():
            self.assertNotIn(b".py", blob, kind)

    def test_unresolved_metadata_still_rejected(self):
        m, raw = gen.load_kind(self.src_dir, "ness")
        m["model_intern"]["unresolved"] = ["unresolved retained pointer"]
        with self.assertRaises(gen.PackError):
            gen.build_pack("ness", 11, m, raw)

    def test_default_all_complete(self):
        out = os.path.join(self.tmp, "all12")
        rc = gen.main(["--input-dir", self.src_dir, "--output-dir", out])
        self.assertEqual(rc, 0)
        for fkind in range(12):
            self.assertTrue(os.path.exists(os.path.join(out, "%02d.fpc" % fkind)))

    def test_default_input_dir_generates_source(self):
        # Proposal lock: omitting --input-dir must auto-generate source
        # metadata into <output-dir>/source-metadata for the requested
        # kinds. Single-kind run keeps this check cheap.
        out = os.path.join(self.work, "default-gen")
        rc = gen.main(["--output-dir", out, "--kinds", "mario"])
        self.assertEqual(rc, 0)
        self.assertTrue(os.path.exists(os.path.join(out, "00.fpc")))
        self.assertTrue(os.path.exists(
            os.path.join(out, "source-metadata", "mario_compact_map.json")))
        self.assertTrue(os.path.exists(
            os.path.join(out, "source-metadata", "mario_compact.bin")))


if __name__ == "__main__":
    unittest.main()
