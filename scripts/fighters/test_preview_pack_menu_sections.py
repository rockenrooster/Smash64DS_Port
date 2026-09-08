#!/usr/bin/env python3
"""Tests for the FPC1 section-2 menu anims (generate_preview_core_packs).

Covers the GENERATOR side only: with_menu_sections=False output keeps
sections 0/1 byte-identical (same section records, fixups, span prefix);
with_menu_sections=True appends exactly one section carrying the idle
(submotion row 0) + Selected Win-clip bytes for every kind in KIND_ORDER
inside NDS_PREVIEW_PACK_MAX_SECTIONS. Section 3 (owner preview geometry)
is intentionally absent: no file in the tree provides per-kind owner
image bytes offline.

Fixtures are generated in temporary directories from repo reference source
via preview_source_metadata plus the encoder. No ignored builds inputs.

Run:
    python -m pytest scripts/fighters/test_preview_pack_menu_sections.py -q
"""

from __future__ import annotations

import io
import json
import os
import sys
import tempfile
import unittest
from contextlib import redirect_stdout

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import generate_preview_core_packs as gen
import preview_source_metadata as srcgen


class MenuSectionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.work = tempfile.mkdtemp(prefix="fpc1-menu-")
        cls.src_dir = os.path.join(cls.work, "source")
        os.makedirs(cls.src_dir, exist_ok=True)
        result = srcgen.generate(cls.src_dir, list(gen.KIND_ORDER))
        assert result["ok"], "source metadata must generate"
        cls.maps = {}
        cls.raws = {}
        cls.plain = {}
        cls.menu = {}
        for kind in gen.KIND_ORDER:
            fkind = gen.KIND_ORDER.index(kind)
            with open(os.path.join(cls.src_dir, kind + "_compact_map.json"),
                      encoding="utf-8") as f:
                cls.maps[kind] = json.load(f)
            with open(os.path.join(cls.src_dir, kind + "_compact.bin"),
                      "rb") as f:
                raw = f.read()
            assert raw[:4] == b"MCM2"
            raw = raw[8:]
            cls.raws[kind] = raw
            blob, _ = gen.build_pack(kind, fkind, cls.maps[kind], raw)
            cls.plain[kind] = gen.decode_pack(blob)
            blob, meta = gen.build_pack(kind, fkind, cls.maps[kind], raw,
                                        with_menu_sections=True)
            cls.menu[kind] = (gen.decode_pack(blob), meta)

    def test_default_mode_section_counts(self):
        for kind, d in self.plain.items():
            want = 3 if kind == "donkey" else 2
            self.assertEqual(len(d["sections"]), want, kind)

    def test_sections_zero_one_byte_identical(self):
        for kind in gen.KIND_ORDER:
            plain = self.plain[kind]
            flagged, _ = self.menu[kind]
            self.assertEqual(flagged["sections"][:2], plain["sections"][:2],
                             kind)
            self.assertEqual(flagged["fixups"], plain["fixups"], kind)
            self.assertEqual(flagged["spans"][:len(plain["spans"])],
                             plain["spans"], kind)

    def test_menu_section_for_every_kind(self):
        for kind in gen.KIND_ORDER:
            m = self.maps[kind]
            raw = self.raws[kind]
            flagged, meta = self.menu[kind]
            self.assertLessEqual(len(flagged["sections"]), gen.MAX_SECTIONS,
                                 kind)
            self.assertEqual(len(flagged["sections"]),
                             len(self.plain[kind]["sections"]) + 1, kind)
            sec = flagged["sections"][-1]
            self.assertEqual(sec[0], m["idle"]["file"], kind)
            self.assertEqual(sec[6], 0, kind)
            self.assertEqual(sec[7], 0, kind)
            idle_sec = next(s for s in m["sections"]
                            if s["name"] == "anim-idle")
            sel_sec = next(s for s in m["sections"]
                           if s["name"] == "anim-selected")
            idle_len = idle_sec["len"]
            sel_len = sel_sec["len"]
            self.assertEqual(idle_len, m["idle"]["bytes"], kind)
            self.assertEqual(sel_len, m["selected_proof"]["total_bytes"],
                             kind)
            want = (raw[idle_sec["new"]:idle_sec["new"] + idle_len]
                    + raw[sel_sec["new"]:sel_sec["new"] + sel_len])
            got = bytes(flagged["data"][sec[1]:sec[1] + sec[2]])
            self.assertIn(want[:idle_len], got, (kind, "idle"))
            self.assertIn(want[idle_len:], got, (kind, "selected"))
            self.assertEqual(sec[3], idle_len + sel_len, kind)
            self.assertEqual(sec[5], 2, kind)
            self.assertEqual(meta["menu_selected_file"],
                             m["selected"]["file"], kind)

    def test_menu_asset_id_unique(self):
        for kind in gen.KIND_ORDER:
            flagged, _ = self.menu[kind]
            ids = [s[0] for s in flagged["sections"]]
            self.assertEqual(len(set(ids)), len(ids), kind)

    def test_size_report_printed(self):
        out = os.path.join(self.work, "report")
        buf = io.StringIO()
        with redirect_stdout(buf):
            rc = gen.main(["--input-dir", self.src_dir, "--output-dir", out,
                           "--kinds", "mario,donkey",
                           "--with-menu-sections"])
        self.assertEqual(rc, 0)
        text = buf.getvalue()
        self.assertIn("size report (bytes per section + total):", text)
        self.assertIn("00.fpc mario", text)
        self.assertIn("02.fpc donkey", text)
        self.assertIn("total=", text)


if __name__ == "__main__":
    unittest.main()
