#!/usr/bin/env python3
"""Focused tests for Ness preview source metadata closure.

The pinned 335_NessModel.reloc family is stale in two generic ways, both
fixed in preview_source_metadata without Ness-specific constants:
- PKThunderWaveMObjSub reloc edges use the old combined-block base 0x9870
  (6-entry sprites table @ 0x9870 per 335_NessModel.c; the re-typed MObjSub
  sits at 0x9888). One self-referential edge carries a stale target value
  occurring nowhere in the O2R payload, so it abstains from the sibling
  vote instead of poisoning it; the remaining three vote 0x9870.
- PKThunderWaveMatAnimJoint..._data names an opaque u32 block the source
  decomposes; the unique same-stem _0xH definition (..._0x9BBC[40] @ 0x9BBC)
  anchors the family origin, and every edge still needs an exact O2R word
  match. Chain-mismatched lines are retried in the round-2 fixpoint, which
  already consults implied/chained targets.

All 20 edges land in the pruned geometry gap (slots outside kept spans),
so they are recorded as pruned-slot identities, never silently nulled
retained dependencies. Motion behavior is untouched (DemoNull idle plus
Selected Win2 for Ness).

Run:
    python -m pytest scripts/fighters/test_ness_preview_metadata.py -q
"""

from __future__ import annotations

import hashlib
import json
import os
import struct
import sys
import unittest

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import preview_source_metadata as srcgen

REPO = srcgen.REPO
BASELINE_SHA256 = {
    'captain': '588732dc9f579c176e19560ae9aa672edfac6f27be1fbf0751dd72563187f506',
    'donkey': '9e5bf61fd1b34e6988b3709bb2b1c91cb561bd8e93eb06ddbe9e9fdb94f754b4',
    'fox': 'cc856574f512a99e53318e3b4c4c32bdb3232af4f7075404f783b4ad6e651058',
    'kirby': 'ea731d504328580c922a3d8a2f698679f38ddb57bcc49e7f47c0df4e384224cc',
    'link': '05ab208afeb6fdaf10e32d24e9b273c27b65b2cf969f759553cd9dc2e35ac34f',
    'luigi': '740a9070e40ebbfe9a0bbf86dddda71e79c0cbc8ae2f0d5ce256c8dc8332faf3',
    'mario': 'eb110d020e2f6640468f2650471d7370c81b5401570f073de75d122a381430ee',
    'pikachu': '660bfd3c179c661cd350af3106182124ab9c410e3d85d9c95dd76f0acedadc6b',
    'purin': 'e10880cad27d07ad0510fcb59944c738f431b0eff5bcccd17ea610f941497f5f',
    'samus': '8ccec4dd5767727c262be0f267d2ca34690047cfa3c6b9e722d615620b90f248',
    'yoshi': 'e47fcd4d4d55704f2371c357794631f5aae20528e308531fa55494f16106d9d1',
}

# Pinned O2R-chain resolutions (measured from the Ness Model O2R payload
# plus 335_NessModel.c @ offsets; asserted with payload agreement below).
MOBJ_BASE = 0x9870
MOBJ_EDGES = [  # (slot_old, target_old)
    (0x9878, 0x9900),  # table[2] -> MObj* array (gap_0x98E8_sub_0x18)
    (0x987C, 0x9470),  # table[3] -> Tex_0x9470 (gap_0x9050_sub_0x420)
    (0x9880, 0x9068),  # table[4] -> Tex_0x9068 (gap_0x9050_sub_0x18)
    (0x988C, 0x987C),  # self edge -> table[3] slot
    (0x9900, 0x9888),  # MObj* array -> MObjSub (MObjSub+0x18 @ 0x9870 base)
]
MDATA_BASE = 0x9BBC
MDATA_EDGES = [  # (slot_add, target_add) relative to MDATA_BASE
    (0x9C, 0x0), (0xA0, 0x0), (0x148, 0xA4), (0x288, 0x194),
    (0x5F8, 0x2D4), (0x648, 0x384), (0x6F0, 0x504), (0x7E8, 0x724),
    (0x8F8, 0x834), (0x9D8, 0x944), (0xAB8, 0xA24), (0xBC8, 0xB04),
    (0xCD8, 0xC14), (0xDB8, 0xD24), (0xE98, 0xE04),
]
MDATA_BASE_SYMBOL = ("dNessModel_PKThunderWaveMatAnimJoint_MatAnimJoint_data")
MDATA_ANCHOR = ("dNessModel_PKThunderWaveMatAnimJoint_MatAnimJoint_0x9BBC")
MOBJ_BASE_SYMBOL = "dNessModel_PKThunderWaveMObjSub_MObjSub"


def _u32be(b, off):
    return struct.unpack_from(">I", b, off)[0]


def _decode_ptr(w):
    return (w & 0xFFFF) * 4


class NessPreviewMetadataTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import tempfile
        cls._tmp = tempfile.TemporaryDirectory(prefix="ness-preview-test-")
        cls.out = srcgen.generate(cls._tmp.name, ["ness"])
        assert cls.out["ok"], cls.out["failures"]
        cls.kind = cls.out["kinds"][0]
        cls.mi = cls.kind["model_intern"]
        import generate_nds_native_owners as gen
        cls.payload = gen.load_o2r_payload(REPO, "ness")

    @classmethod
    def tearDownClass(cls):
        cls._tmp.cleanup()

    def test_no_gaps(self):
        self.assertEqual(self.mi["unresolved"], [])
        self.assertEqual(self.mi["chain_mismatches"], [])
        self.assertEqual(self.kind["failures"], [])
        self.assertEqual(self.out["failures"], [])

    def test_opaque_alias_provenance(self):
        aliases = self.mi.get("opaque_aliases", {})
        self.assertIn(MDATA_BASE_SYMBOL, aliases)
        info = aliases[MDATA_BASE_SYMBOL]
        self.assertEqual(info["base_old"], "0x%x" % MDATA_BASE)
        self.assertEqual(info["anchor"], MDATA_ANCHOR)
        self.assertEqual(info["anchor_off"], "0x%x" % MDATA_BASE)
        implied = self.mi["implied_bases"].get(MDATA_BASE_SYMBOL)
        self.assertIsNotNone(implied)
        self.assertEqual(implied[0], MDATA_BASE)

    def test_combined_block_base_provenance(self):
        implied = self.mi["implied_bases"].get(MOBJ_BASE_SYMBOL)
        self.assertIsNotNone(implied)
        self.assertEqual(implied[0], MOBJ_BASE)

    def _pruned_by_line(self, prefix):
        found = {}
        for r in self.mi["pruned_slots"]:
            line = r.get("line", "")
            if prefix in line:
                found[line] = r
        return found

    def test_pkthunder_mobj_edges_pruned_with_chain_agreement(self):
        by_line = self._pruned_by_line("PKThunderWaveMObjSub_MObjSub")
        by_line.update(self._pruned_by_line("gap_0x98E8_sub_0x18"))
        self.assertEqual(len(by_line), 5)
        for slot, target in MOBJ_EDGES:
            self.assertEqual(_decode_ptr(_u32be(self.payload, slot)), target,
                             "O2R chain disagreement at 0x%x" % slot)
        got = sorted((r["slot_old"], r["target_old"]) for r in by_line.values())
        self.assertEqual(got, sorted(MOBJ_EDGES))

    def test_matanim_edges_pruned_with_chain_agreement(self):
        by_line = self._pruned_by_line(MDATA_BASE_SYMBOL)
        # 15 _data edges plus the MatAnimJoint+0x8 anchor edge.
        self.assertEqual(len(by_line), 16)
        for madd, tadd in MDATA_EDGES:
            slot, target = MDATA_BASE + madd, MDATA_BASE + tadd
            self.assertEqual(_decode_ptr(_u32be(self.payload, slot)), target,
                             "O2R chain disagreement at 0x%x" % slot)
        got = sorted((r["slot_old"], r["target_old"])
                     for r in by_line.values()
                     if "MatAnimJoint_MatAnimJoint+0x8" not in r["line"])
        want = sorted((MDATA_BASE + m, MDATA_BASE + t)
                      for m, t in MDATA_EDGES)
        self.assertEqual(got, want)

    def test_motion_behavior_intact(self):
        # Ness previews DemoNull idle and Selected Win2; shieldpose pruned.
        self.assertEqual(self.kind["selected_index"], 2)
        self.assertIn("Selected", self.kind["selected_anim_symbol"])
        self.assertEqual(int(self.kind["idle_flags"], 16) & 0x2, 0)
        self.assertEqual(int(self.kind["selected_flags"], 16) & 0x2, 0)
        sel = self.kind["selected_parse"]
        self.assertGreater(sel["joints"], 0)
        self.assertEqual(sel["table_entries"], sel["joints"])

    def test_emitted_bytes_pinned(self):
        self.assertEqual(self.kind["emitted_bytes"], 21216)

    def test_deterministic_rerun(self):
        import tempfile
        with tempfile.TemporaryDirectory(prefix="ness-preview-rerun-") as d:
            out2 = srcgen.generate(d, ["ness"])
            self.assertTrue(out2["ok"])
            b1 = (os.path.join(self._tmp.name, "ness_compact.bin"))
            b2 = (os.path.join(d, "ness_compact.bin"))
            with open(b1, "rb") as f:
                h1 = hashlib.sha256(f.read()).hexdigest()
            with open(b2, "rb") as f:
                h2 = hashlib.sha256(f.read()).hexdigest()
            self.assertEqual(h1, h2)
            with open(os.path.join(self._tmp.name,
                                   "ness_compact_map.json"), "rb") as f:
                m1 = hashlib.sha256(f.read()).hexdigest()
            with open(os.path.join(d, "ness_compact_map.json"), "rb") as f:
                m2 = hashlib.sha256(f.read()).hexdigest()
            self.assertEqual(m1, m2)

    def test_other_kinds_byte_identical(self):
        import tempfile
        kinds = [k for _, k, _ in srcgen.KINDS if k != "ness"]
        with tempfile.TemporaryDirectory(prefix="preview-11-check-") as d:
            out = srcgen.generate(d, kinds)
            self.assertTrue(out["ok"], out["failures"])
            for _, key, _ in srcgen.KINDS:
                if key == "ness":
                    continue
                name = key + "_compact.bin"
                with open(os.path.join(d, name), "rb") as f:
                    h_new = hashlib.sha256(f.read()).hexdigest()
                self.assertEqual(h_new, BASELINE_SHA256[key], name)


if __name__ == "__main__":
    unittest.main()
