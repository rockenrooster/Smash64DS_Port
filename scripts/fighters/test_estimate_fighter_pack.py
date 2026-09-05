#!/usr/bin/env python3
"""Unit tests for the stage-2 disposition ledger of estimate_fighter_pack.

Specification: ``docs/p2/P2-2-pack-estimator.md`` (disposition table plus the
two audited corrections) and the stage-2 task brief.  These tests pin

  * Kirby's ledger totals -- if an intentional source or rule change moves a
    number, update the pin in the same change;
  * the STOP rule: an object with no disposition is a STOP, never a guess;
  * the reconciliation law (retained + removable + motion + stop == indexed);
  * the costume axis (4 costumes; stock palettes resolved via the stock LUT);
  * the set-enumeration count formula and the conservative atom union.

The full-closure tests need the read-only decomp tree and the generated
manifest; they are skipped when either is absent so the file stays importable
on a bare checkout.

Run:
    python -m unittest scripts.fighters.test_estimate_fighter_pack -v
    python scripts/fighters/test_estimate_fighter_pack.py
"""

from __future__ import annotations

import os
import sys
import unittest

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import estimate_fighter_pack as e  # noqa: E402

HAVE_CORPUS = (os.path.isdir(e.RELOCDATA_DIR)
               and os.path.isfile(e.MANIFEST_PATH)
               and os.path.isfile(e.NATIVE_IMAGE_PATH))


def make_row(**kw):
    """A synthetic ObjectRow with stage-2 defaults for the test's overrides."""
    base = dict(
        symbol=kw.pop("symbol", "dSynthetic_row"),
        type_name=kw.pop("type_name", "u8"),
        pointer_depth=kw.pop("pointer_depth", 0),
        count=kw.pop("count", 1),
        elem_size=kw.pop("elem_size", 1),
        size=kw.pop("size", 1),
        offset=kw.pop("offset", None),
        offset_source=kw.pop("offset_source", None),
        line=kw.pop("line", 1),
        file_id=kw.pop("file_id", 999),
        file_name=kw.pop("file_name", "999_Synthetic.c"),
        is_pad=kw.pop("is_pad", False),
        anchor_offset=kw.pop("anchor_offset", None),
        anchor_size=kw.pop("anchor_size", None),
        name_offset=kw.pop("name_offset", None),
        walk_offset=kw.pop("walk_offset", None),
        size_source=kw.pop("size_source", "test"),
        notes=kw.pop("notes", []),
        init_text=kw.pop("init_text", None),
        head_comment=kw.pop("head_comment", None),
    )
    assert not kw, "unknown overrides: %r" % list(kw)
    return e.ObjectRow(**base)


# ---------------------------------------------------------------------------
# Pure rule tests -- no corpus needed
# ---------------------------------------------------------------------------


class TestStopRule(unittest.TestCase):
    """An object with no disposition is a STOP, never a guess."""

    def test_unknown_type_with_pointer_initializer_stops(self):
        row = make_row(symbol="dX_mystery", type_name="OpaqueBlob",
                       init_text="= { (void*)&dX_other }")
        a = e.classify_object(row, "other", "Kirby", {})
        self.assertEqual(a.disposition, "STOP")

    def test_unknown_byte_payload_stops(self):
        row = make_row(symbol="dX_payload", type_name="u8", size=64,
                       init_text="= { some_nonnumeric_junk }")
        a = e.classify_object(row, "other", "Kirby", {})
        self.assertEqual(a.disposition, "STOP")

    def test_stop_invalidates_the_verdict(self):
        v, _reason = e.verdict_for(1000, 900, stop_count=1)
        self.assertEqual(v, "STOP")

    def test_known_types_never_stop(self):
        for tname, init in (
            ("Vtx", "= { #include <x.vtx.inc.c> }"),
            ("Gfx", "= { #include <x.dl.inc.c> }"),
            ("MObjSub", "= { { 0x0000, G_IM_FMT_CI, G_IM_SIZ_16b, NULL } }"),
            ("DObjDesc", "= { { 0, NULL, { 0.0f, 0.0f, 0.0f } } }"),
            ("FTAttributes", "= { 0.91f, 90.0f }"),
            ("FTModelPart", "= { { NULL, NULL } }"),
            ("Sprite", "= { 0, 0, 8, 10, 1.0f }"),
            ("DObjDLLink", "= { { 1, NULL } }"),
            ("ftMotionCommand", "= { ftMotionCommandWait(3) }"),
            ("WPAttributes", "= { NULL, NULL, 10 }"),
            ("u32", "= { 0xEF7CFFC0, 0x00000000 }"),
            ("u16", "= { #include <x.palette.inc.c> }"),
        ):
            row = make_row(type_name=tname, init_text=init)
            a = e.classify_object(row, "other", "Kirby", {})
            self.assertNotEqual(a.disposition, "STOP",
                                "%s must never STOP" % tname)


class TestCorrections(unittest.TestCase):
    """The two audited corrections, applied verbatim."""

    def test_hurtbox_owner_retained_whole(self):
        row = make_row(type_name="FTAttributes", size=840,
                       init_text="= { 0.91f, 90.0f }")
        a = e.classify_object(row, "other", "Kirby", {})
        self.assertEqual(a.disposition, "RETAINED_SEMANTIC")
        self.assertIn("ftParamResetFighterDamageCollsAll", a.reason)

    def test_no_rule_filters_by_detail_label(self):
        # Correction 2: the effective low-detail selection includes the
        # null-entry fallback, so retained-whole classes keep both details.
        # Structurally: joint trees are retained whole, and no classification
        # branch consults a high/low label.
        row = make_row(type_name="DObjDesc", init_text="= { { 0, NULL } }")
        a = e.classify_object(row, "body_model", "Kirby", {})
        self.assertEqual(a.disposition, "RETAINED_JOINT_TREE")
        self.assertIn("BOTH details", a.reason)


class TestEvidenceAndEstimates(unittest.TestCase):
    def test_initializer_evidence_kinds(self):
        self.assertEqual(e.initializer_evidence("= { 1, 2, 0x3 }").kind,
                         "numeric")
        self.assertEqual(
            e.initializer_evidence("= { ftMotionCommandWait(3) }").kind,
            "macro")
        self.assertEqual(
            e.initializer_evidence("= { #include <a.tex.inc.c> }").kind,
            "symbol")
        self.assertEqual(e.initializer_evidence(None).kind, "none")

    def test_pointer_table_replacement_is_two_bytes_per_pointer(self):
        row = make_row(type_name="MObjSub", pointer_depth=1, count=5,
                       elem_size=4, size=20,
                       init_text="= { NULL, NULL, NULL, NULL, NULL }")
        a = e.classify_object(row, "other", "Kirby", {})
        self.assertEqual(a.disposition, "PTR_TABLE")
        self.assertEqual(e.REPL_PTR_REF * 5, 10)

    def test_verdict_bands(self):
        self.assertEqual(e.verdict_for(100_000, 90_000, 0)[0], "GREEN")
        self.assertEqual(e.verdict_for(160_000, 100_000, 0)[0], "YELLOW")
        self.assertEqual(e.verdict_for(200_000, 180_000, 0)[0], "RED")
        self.assertEqual(e.verdict_for(200_000, 150_000, 0)[0], "UNKNOWN")

    def test_constants_match_the_review(self):
        self.assertEqual(e.F_NEW_BASE, 208372)
        self.assertEqual(e.FLOOR_BYTES, 32768)
        self.assertEqual(e.W_CEILING, 175604)
        self.assertEqual(e.COSTUME_COUNT, 4)

    def test_native_image_guards(self):
        self.assertTrue(e._eval_image_guard(
            "!NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER",
            dict(e.NATIVE_IMAGE_FLAGS_BASE)))
        self.assertFalse(e._eval_image_guard(
            "!NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER",
            dict(e.NATIVE_IMAGE_FLAGS_HWTRI)))
        with self.assertRaises(e.Refusal):
            e._eval_image_guard("NDS_SOME_UNKNOWN_FLAG",
                                dict(e.NATIVE_IMAGE_FLAGS_HWTRI))


# ---------------------------------------------------------------------------
# Full-corpus tests
# ---------------------------------------------------------------------------


@unittest.skipUnless(HAVE_CORPUS, "decomp corpus / manifest / census absent")
class TestKirbyLedgerPins(unittest.TestCase):
    """Kirby's full ledger, pinned.  Update pins only with the change that
    moves them, and say why in that change."""

    @classmethod
    def setUpClass(cls):
        cls.types = e.TypeTable()
        cls.types.load_dirs(e.HEADER_DIRS)
        cls.census = e.parse_native_image_census()
        idx, entry = e.index_closure("Kirby", cls.types)
        cls.ledger = e.FighterLedger("Kirby", idx, entry, cls.census, "hwtri")

    def test_totals_pinned(self):
        self.assertEqual(self.ledger.totals(), {
            "indexed_bytes": 204183,
            "retained": 54567,
            "removable": 138692,
            "replacement": 48100,
            "unresolved_membership": 20896,
            "unresolved_weapon_native": 28520,
            "bank_count": 144,
            "native_census_both": 10380,
            "native_census_low": 4416,
            "native_owner_static": False,
            "w_profile_a_worst": 102667,
            "w_profile_a_vram": 81771,
            "w_profile_b_worst": 113591,
            "w_profile_b_vram": 92695,
            "motion_bytes": 10924,
            "stop_count": 0,
            "stop_bytes": 0,
            "reconciles": True,
        })

    def test_reconciliation_law(self):
        t = self.ledger.totals()
        self.assertTrue(t["reconciles"])
        self.assertEqual(t["retained"] + t["removable"] + t["motion_bytes"]
                         + t["stop_bytes"], t["indexed_bytes"])

    def test_disposition_class_counts_pinned(self):
        counts = {r["disposition"]: r["objects"]
                  for r in self.ledger.class_rows()}
        self.assertEqual(counts, {
            "CONSERVATIVE_RETAIN": 277,
            "RETAINED_SEMANTIC": 19,
            "PTR_TABLE": 446,
            "MOTION_STREAM": 229,
            "PADDING_DROP": 118,
            "TEXEL_BANK": 57,
            "NATIVE_REPLACE_WEAPON": 176,
            "SETUP_TRANSIENT": 7,
            "RETAINED_JOINT_TREE": 17,
            "PALETTE_BANK": 87,
            "EVENT_STREAM_RETAIN": 8,
            "MATERIAL_RECORD": 131,
            "NATIVE_REPLACE_BODY": 224,
            "SCENE_SPLIT": 10,
        })
        self.assertNotIn("STOP", counts)

    def test_copy_hat_rows_retained(self):
        # FTModelPart rows -- including the copy-hat rows that point at the
        # Link boomerang and Fox donor models -- are retained whole.
        rows = [a for a in self.ledger.assignments
                if a.row.type_name == "FTModelPart"]
        self.assertTrue(rows)
        for a in rows:
            self.assertEqual(a.disposition, "RETAINED_SEMANTIC")
            self.assertIn("copy-hat", a.reason)

    def test_costume_rows(self):
        rows = self.ledger.costume_rows()
        self.assertEqual(len(rows), e.COSTUME_COUNT)
        for c, row in enumerate(rows):
            self.assertEqual(row["costume"], c)
            self.assertEqual(row["w_profile_a_worst"], 102667)
            self.assertEqual(row["resolved_banks_vram_bytes"], 208)

    def test_stock_lut_resolves_four_distinct_costume_palettes(self):
        indexes = sorted(i for i in self.ledger.stock_lut.values())
        self.assertEqual(indexes, [0, 1, 2, 3])

    def test_native_census_split(self):
        self.assertEqual(self.census["Kirby"], {"High": 5964, "Low": 4416})
        self.assertNotIn("Mario", self.census)
        self.assertNotIn("Fox", self.census)

    def test_single_kind_set_matches_totals(self):
        # The atom-union path and the per-fighter totals path must agree for
        # a single-kind set (the census + header fixed term included).
        _target, sets, _stop = e.enumerate_sets({"Kirby": self.ledger})
        self.assertEqual(len(sets), 1)
        _kinds, w = sets[0]
        self.assertEqual(w, self.ledger.totals()["w_profile_a_worst"])

    def test_unknowns_never_become_zero(self):
        t = self.ledger.totals()
        self.assertGreater(t["unresolved_membership"], 0)
        self.assertGreater(t["unresolved_weapon_native"], 0)


@unittest.skipUnless(HAVE_CORPUS, "decomp corpus / manifest / census absent")
class TestSetEnumeration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.types = e.TypeTable()
        cls.types.load_dirs(e.HEADER_DIRS)
        census = e.parse_native_image_census()
        cls.ledgers = e.build_fighter_ledgers(
            ["Mario", "Fox", "Kirby"], cls.types, "hwtri", census)

    def test_one_through_four_kind_count(self):
        target, sets, _stop = e.enumerate_sets(self.ledgers)
        # 3 closable fighters: C(3,1)+C(3,2)+C(3,3) = 3+3+1 = 7; the 12-kind
        # roster target the spec names is 793.
        self.assertEqual(len(sets), 7)
        self.assertEqual(target, 7)
        self.assertEqual(sum(e._nCr(12, r) for r in range(1, 5)), 793)

    def test_worst_set_and_union_conservatism(self):
        _t, sets, _s = e.enumerate_sets(self.ledgers)
        worst_kinds, worst_w = sets[0]
        self.assertEqual(set(worst_kinds), {"Mario", "Fox", "Kirby"})
        singles = {kinds[0]: w for kinds, w in sets if len(kinds) == 1}
        for name, w in singles.items():
            self.assertGreaterEqual(worst_w, w)
        # 338_YoshiModel is a Kirby donor; the union must charge the shared
        # atom conservatively (>= either single-kind view of it), so the set
        # figure can only grow over any single member's view.
        kirby_atoms = self.ledgers["Kirby"].atom_keep_bytes()
        self.assertGreaterEqual(worst_w, sum(v[0] for v in kirby_atoms.values()))

    def test_no_stops_in_closable_ledgers(self):
        for name, led in self.ledgers.items():
            self.assertEqual(led.stops, [], "%s has STOPs" % name)


if __name__ == "__main__":
    unittest.main(verbosity=2)
