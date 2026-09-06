#!/usr/bin/env python3
"""Unit tests for the disposition ledger and recovery levers of
estimate_fighter_pack.

Specification: ``docs/p2/P2-2-pack-estimator.md`` (disposition table plus the
two audited corrections) and review section 7's three recovery levers.  These
tests pin

  * Kirby's ledger totals -- if an intentional source or rule change moves a
    number, update the pin in the same change;
  * the STOP rule: an object with no disposition is a STOP, never a guess;
  * the reconciliation law (retained + removable + stop == indexed);
  * legal VS costume IDs from the source royal/team selectors, including
    nonsequential CSS IDs and extra team palettes;
  * resident core commands and the full manifest motion-file union;
  * the set-enumeration count formula and the conservative atom union;
  * one synthetic-object test per recovery lever (7.1 bank membership,
    7.2 weapon/donor natives, 7.3 u32 conservative retains by readers).

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
import hashlib
import io
import struct
import tempfile
import unittest
from unittest.mock import patch

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import estimate_fighter_pack as e  # noqa: E402

HAVE_CORPUS = (os.path.isdir(e.RELOCDATA_DIR)
               and os.path.isfile(e.MANIFEST_PATH)
               and os.path.isfile(e.NATIVE_IMAGE_PATH)
               and os.path.isdir(os.path.join(e.REPO_ROOT, "decomp", "BattleShip-main",
                                               "BattleShip_o2r")))


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


class TestSourceCostumeDomain(unittest.TestCase):
    def test_source_selectors_not_css_choice_count_define_domain(self):
        types = e.TypeTable()
        types.constants["nFTKindSynthetic"] = 0
        source = "FTCostume dFTParamCostumeIDs[] = { {{0,4,1,3}, {1,5,2}, 9} };"
        self.assertEqual(e.source_costume_ids("Synthetic", types, source),
                         (0, 1, 2, 3, 4, 5))

    def test_unknown_selector_fails_closed(self):
        types = e.TypeTable()
        types.constants["nFTKindSynthetic"] = 0
        with self.assertRaises(e.Refusal):
            e.source_costume_ids("Synthetic", types,
                "FTCostume dFTParamCostumeIDs[] = { {{0,1,2,3}, {0,1,UNKNOWN}, 0} };")

    @unittest.skipUnless(HAVE_CORPUS, "decomp corpus absent")
    def test_source_playable_domains(self):
        expected = {
            "Mario": (0, 1, 2, 3, 4), "Fox": (0, 1, 2, 3),
            "Donkey": (0, 1, 2, 3, 4), "Samus": (0, 1, 2, 3, 4),
            "Luigi": (0, 1, 2, 3), "Link": (0, 1, 2, 3),
            "Yoshi": (0, 1, 2, 3), "Captain": (0, 1, 2, 3, 4, 5),
            "Kirby": (0, 1, 2, 3, 4), "Pikachu": (0, 1, 2, 3),
            "Purin": (0, 1, 2, 3), "Ness": (0, 1, 2, 3),
        }
        for fighter, ids in expected.items():
            self.assertEqual(e.source_costume_ids(fighter), ids)


class TestResidentMotionMembers(unittest.TestCase):
    @staticmethod
    def member(file_id, size=32, **kw):
        return dict(symbol="llTest%d" % file_id,
                    asset=dict(id=file_id, data_bytes=size - 3,
                               alloc_bytes=size, sha256="test%d" % file_id), **kw)

    def test_aliases_event32_items_and_core_overlap(self):
        entry = dict(core_extern_closure=[dict(id=1)],
                     motion_files=[self.member(1), self.member(2),
                                   self.member(2), self.member(3, item_related=True)],
                     event32_motion_files=[self.member(2), self.member(4)])
        members = e.resident_motion_files(entry)
        self.assertEqual(set(members), {2, 3, 4})
        self.assertEqual(sum(a["alloc_bytes"] for a in members.values()), 96)

    def test_conflicting_file_identity_fails_closed(self):
        entry = dict(core_extern_closure=[], motion_files=[self.member(2)],
                     event32_motion_files=[self.member(2, 64)])
        with self.assertRaises(e.Refusal):
            e.resident_motion_files(entry)

    def test_missing_member_size_fails_closed(self):
        with self.assertRaises(e.Refusal):
            e.resident_motion_files(dict(core_extern_closure=[],
                                         motion_files=[dict(asset=dict(id=2))]))

    def test_shared_file_is_charged_once_across_fighters(self):
        entry = dict(core_extern_closure=[], motion_files=[self.member(2)])
        ledgers = {name: e.FighterLedger(name, _synthetic_closure([]), entry,
                                         {}, "hwtri", costume_ids=(0,))
                   for name in ("Mario", "Fox")}
        _target, sets, _stop = e.enumerate_sets(ledgers, "b_worst")
        self.assertEqual(sets[0][1], 2 * e.REPL_PACK_HEADER + 32)


class TestSourceLayoutReconciliation(unittest.TestCase):
    def setUp(self):
        self.types = e.TypeTable()
        self.types.typedefs.update(u8="unsigned char", u16="unsigned short",
                                   u32="unsigned int")

    @staticmethod
    def source(payload, referenced=(), pointers=None):
        return dict(payload=payload, sha256=hashlib.sha256(payload).hexdigest(),
                    referenced=set(referenced), pointers=pointers or {})

    def parse(self, code, source):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "999_Test.c")
            with open(path, "w", encoding="utf-8") as fh:
                fh.write(code)
            pf = e.parse_reloc_c(path, 999, self.types, source=source)
        e.check_file_tail(pf, len(source["payload"]))
        e.reconcile_source_locations(pf)
        return pf

    CODE = ("/* @ 0x0, 4 bytes */ u8 dTest_bytes[3] = {1,2,3};\n"
            "/* @ 0x4, 4 bytes */ u32 dTest_word = 7;")
    PAYLOAD = b"\x01\x02\x03\0\0\0\0\x07" + b"\0" * 8

    def test_alignment_and_tail_are_indexed_with_raw_evidence(self):
        pf = self.parse(self.CODE, self.source(self.PAYLOAD))
        self.assertEqual(pf.disagreements, [])
        self.assertEqual(pf.accounted_bytes, len(self.PAYLOAD))
        self.assertEqual([(o.offset, o.size) for o in pf.objects if o.is_pad],
                         [(3, 1), (8, 8)])

    def test_nonzero_gap_is_not_silently_dropped(self):
        payload = self.PAYLOAD[:3] + b"\x09" + self.PAYLOAD[4:]
        pf = self.parse(self.CODE, self.source(payload))
        self.assertTrue(pf.disagreements)
        self.assertNotIn(3, [o.offset for o in pf.objects if o.is_pad])

    def test_zero_gap_with_incoming_pointer_is_not_padding(self):
        pf = self.parse(self.CODE, self.source(self.PAYLOAD, referenced=(3,)))
        self.assertTrue(pf.disagreements)
        self.assertNotIn(3, [o.offset for o in pf.objects if o.is_pad])

    def test_tail_with_data_or_pointer_stays_unresolved(self):
        for payload, refs in ((self.PAYLOAD[:-1] + b"\x09", ()),
                              (self.PAYLOAD, (15,))):
            pf = self.parse(self.CODE, self.source(payload, refs))
            self.assertIn("tail", [d.kind for d in pf.disagreements])

    def test_explicit_alignment_comment_for_halfword_array(self):
        code = ("u16 dTest_first[1] = {1};\n"
                "/* 2 bytes pad to align next decl */\n"
                "/* @ 0x4, 4 bytes */ u16 dTest_second[2] = {2,3};")
        payload = struct.pack(">4H", 1, 0, 2, 3) + b"\0" * 8
        pf = self.parse(code, self.source(payload))
        self.assertEqual(pf.disagreements, [])
        self.assertIn((2, 2), [(o.offset, o.size) for o in pf.objects if o.is_pad])

    def test_relative_comment_does_not_rewind_file_cursor(self):
        code = ("u32 dTest_prefix[16] = {0};\n"
                "/* @ 0x40 */ u32 dTest_LayerAnim_AnimJoint[4] = {0};\n"
                "/* @ 0x10 within LayerAnim_AnimJoint */\n"
                "u16 dTest_LayerAnim_palette_0x10[16] = {0};")
        pf = self.parse(code, self.source(b"\0" * 0x70))
        self.assertEqual(pf.disagreements, [])
        self.assertEqual(pf.objects[-1].offset, 0x50)

    def test_nested_data_uses_immediate_parent_not_animation_root(self):
        rows = [make_row(symbol="dTest_MatAnimJoint", offset=0x40),
                make_row(symbol="dTest_MatAnimJoint_data", offset=0x48)]
        offset, _rule = e.contextual_name_offset("dTest_MatAnimJoint_data_at_0x10", rows)
        self.assertEqual(offset, 0x58)

    def test_unresolved_absolute_metadata_stays_a_disagreement(self):
        pf = self.parse("u32 dTest_0x8 = 0;",
                        self.source(b"\0" * 16))
        self.assertTrue(e.source_extraction_layout_matches(pf))
        self.assertTrue(pf.disagreements)

    def test_complete_pointer_array_rejects_shifted_candidate_and_nonzero_null(self):
        pf = e.ParsedFile(999, "999_Test.c", "<memory>")
        target = make_row(symbol="dTest_target", offset=0x20)
        row = make_row(symbol="dTest_ptrs", type_name="u8", pointer_depth=1,
                       offset=0, elem_size=4, size=12, count=3,
                       init_text="= { NULL, dTest_target, NULL }")
        pf.objects = [row, target]
        payload = struct.pack(">4I", 0, 0xFFFF0008, 0, 0) + b"\0" * 32
        pf.source = self.source(payload, pointers={4: (999, 0x20)})
        self.assertTrue(e.pointer_array_source_matches(pf, row, 0))
        self.assertFalse(e.pointer_array_source_matches(pf, row, 4))
        pf.source["payload"] = struct.pack(">I", 1) + payload[4:]
        self.assertFalse(e.pointer_array_source_matches(pf, row, 0))

    def make_disputed_include(self):
        pf = e.ParsedFile(999, "999_Test.c", "<memory>")
        row = make_row(symbol="dTest_tex_0x4", type_name="u8", offset=0,
                       name_offset=4, count=16, size=16, elem_size=1,
                       init_text="= { #include <Test/tex.tex.inc.c> }")
        reader = make_row(symbol="dTest_reader", type_name="u8", pointer_depth=1,
                          count=1, size=4, elem_size=4, offset=16,
                          init_text="= { dTest_tex_0x4 }")
        pf.objects = [row, reader]
        pf.source = self.source(b"\x01" * 16 + struct.pack(">I", 0xFFFF0000) + b"\0" * 12,
                                referenced=(0, 16), pointers={16: (999, 0)})
        pf.disagreements = [e.Disagreement(pf.file_name, row.symbol, 1, "offset:name",
                                            "name=0x4; walk=0x0")]
        return pf, row

    def test_full_generated_include_is_checked_including_its_last_byte(self):
        with tempfile.TemporaryDirectory() as root, patch.object(e, "DECOMP_ROOT", root):
            directory = os.path.join(root, "build", "us", "src", "relocData", "Test")
            os.makedirs(directory)
            include = os.path.join(directory, "tex.tex.inc.c")
            for final, reconciled in (("0x02", False), ("0x01", True)):
                with open(include, "w", encoding="utf-8") as fh:
                    fh.write("0x01," * 15 + final + ",")
                pf, _row = self.make_disputed_include()
                e.reconcile_source_locations(pf)
                self.assertEqual(not pf.disagreements, reconciled)

    def test_missing_include_uses_complete_recipe_and_requires_actual_target(self):
        with tempfile.TemporaryDirectory() as root, patch.object(e, "DECOMP_ROOT", root):
            pf, row = self.make_disputed_include()
            self.assertEqual(e.included_array_source_bytes(pf, row), b"\x01" * 16)
            e.reconcile_source_locations(pf)
            self.assertFalse(pf.disagreements)
            pf, _row = self.make_disputed_include()
            pf.source["pointers"][16] = (999, 4)
            e.reconcile_source_locations(pf)
            self.assertTrue(pf.disagreements)

    def test_disagreement_is_not_waived_when_extractor_layout_does_not_match(self):
        pf, row = self.make_disputed_include()
        row.offset = 1
        e.reconcile_source_locations(pf)
        self.assertTrue(pf.disagreements)

    def test_actual_target_does_not_override_a_changed_source_reader(self):
        pf, _row = self.make_disputed_include()
        pf.objects[1].init_text = "= { NULL }"
        e.reconcile_source_locations(pf)
        self.assertTrue(pf.disagreements)

    def test_dllink_reconciliation_requires_complete_scalar_and_pointer_match(self):
        pf = e.ParsedFile(999, "999_Test.c", "<memory>")
        pf.objects = [make_row(symbol="dTest_dl", offset=0x20)]
        payload = b"\0" * 16 + struct.pack(">4I", 1, 0xFFFF0008, 4, 0) + b"\0" * 16
        pf.source = self.source(payload, pointers={20: (999, 0x20)})
        init = "= {{1, dTest_dl}, {4, NULL}}"
        self.assertTrue(e.dllink_source_matches(pf, init, 16))
        self.assertFalse(e.dllink_source_matches(pf, init, 0))
        pf.source["pointers"][20] = (999, 0x24)
        self.assertFalse(e.dllink_source_matches(pf, init, 16))

    @staticmethod
    def o2r(payload, intern=0xFFFF):
        raw = bytearray(0x50)
        raw[4:8] = b"OLER"
        struct.pack_into("<IHHII", raw, 0x40, 999, intern, 0xFFFF, 0, len(payload))
        return bytes(raw) + payload

    def test_pinned_o2r_rejects_changed_payload_and_malformed_chain(self):
        for raw, sha in ((self.o2r(b"\0" * 16), "wrong"),
                         (self.o2r(b"\0" * 16, intern=0), None)):
            member = dict(id=999, path="test", data_bytes=16,
                          sha256=sha or hashlib.sha256(raw).hexdigest())
            with patch("builtins.open", return_value=io.BytesIO(raw)):
                with self.assertRaises(e.Refusal):
                    e.load_pinned_sources([member])

    def test_actual_chain_protects_pointer_slot_and_target(self):
        raw = self.o2r(struct.pack(">I", 0xFFFF0003) + b"\0" * 12, intern=0)
        member = dict(id=999, path="test", data_bytes=16,
                      sha256=hashlib.sha256(raw).hexdigest())
        with patch("builtins.open", return_value=io.BytesIO(raw)):
            source = e.load_pinned_sources([member])[999]
        self.assertEqual(source["pointers"], {0: (999, 12)})
        self.assertEqual(source["referenced"], {0, 1, 2, 3, 12})


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
            "indexed_bytes": 204208,
            "retained": 50395,
            "removable": 153813,
            "replacement": 46164,
            # lever 7.1: costume membership resolved from the costume
            # material bindings (MObjSub tables paired with their
            # AObjEvent32 programs) plus DL-immediate banks
            "unresolved_membership": 2560,
            "anim_retained_banks": 3240,
            "costume_resolved_banks": 15400,
            # lever 7.2: YoshiModel is owned by Yoshi's native image;
            # Special2/FoxUnknown/LinkBoomerang keep their unresolved line
            "unresolved_weapon_native": 5200,
            "donor_census_bytes": 20928,
            "bank_count": 144,
            "native_census_both": 10380,
            "native_census_low": 4416,
            "native_owner_static": False,
            "w_profile_a_worst": 96559,
            "w_profile_a_vram": 93999,
            "w_profile_b_worst": 495823,
            "w_profile_b_vram": 493263,
            "motion_bytes": 399264,
            "motion_file_count": 188,
            "core_motion_bytes": 10924,
            "stop_count": 0,
            "stop_bytes": 0,
            "reconciles": True,
        })

    def test_reconciliation_law(self):
        t = self.ledger.totals()
        self.assertTrue(t["reconciles"])
        self.assertEqual(t["retained"] + t["removable"]
                         + t["stop_bytes"], t["indexed_bytes"])

    def test_disposition_class_counts_pinned(self):
        counts = {r["disposition"]: r["objects"]
                  for r in self.ledger.class_rows()}
        self.assertEqual(counts, {
            "CONSERVATIVE_RETAIN": 152,
            "RETAINED_SEMANTIC": 22,
            "PTR_TABLE": 446,
            "MOTION_STREAM": 229,
            "PADDING_DROP": 123,
            "TEXEL_BANK": 57,
            "NATIVE_REPLACE_WEAPON": 31,
            "SETUP_TRANSIENT": 7,
            "RETAINED_JOINT_TREE": 17,
            "PALETTE_BANK": 87,
            # lever 7.3 moved the structural AObjEvent32 programs here
            "EVENT_STREAM_RETAIN": 130,
            "MATERIAL_RECORD": 131,
            # lever 7.2 moved YoshiModel's geometry here
            "NATIVE_REPLACE_BODY": 369,
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
        self.assertEqual(len(rows), 5)
        for c, row in enumerate(rows):
            self.assertEqual(row["costume"], c)
            self.assertEqual(row["w_profile_a_worst"], 96559)
            self.assertEqual(row["resolved_banks_vram_bytes"], 15016)

    def test_kirby_body_costume_ladder(self):
        # The corpus's own worked case: the body material's PALETTEID ladder
        # selects palettes[0..4] for legal costumes 0..4, the body texel is
        # costume-common, and the eye texels (frames 5..10 of the same
        # program) are animation-reachable, never dropped.
        by_symbol = {a.row.symbol: a for a in self.ledger.assignments}
        ladder = ("dKirbyModel_gap_0x1A2FC_sub_0x2C3C",
                  "dKirbyModel_gap_0x1A2FC_sub_0x1DDC",
                  "dKirbyModel_gap_0x1A2FC_sub_0x1E04",
                  "dKirbyModel_gap_0x1A2FC_sub_0x1E2C",
                  "dKirbyModel_gap_0x1A2FC_sub_0x1E54")
        for c, sym in enumerate(ladder):
            a = by_symbol[sym]
            self.assertEqual(a.costume_index, frozenset({c}),
                             "costume %d palette" % c)
        self.assertEqual(
            by_symbol["dKirbyModel_Tex_0x1CF60"].costume_index,
            frozenset({0, 1, 2, 3, 4}))

    def test_stock_lut_includes_team_only_palette(self):
        indexes = sorted(i for i in self.ledger.stock_lut.values())
        self.assertEqual(indexes, [0, 1, 2, 3, 4])

    def test_entire_carried_motion_set_includes_items_and_event32_once(self):
        entry = self.ledger.entry
        expected = {row["asset"]["id"] for row in entry["motion_files"]}
        self.assertEqual(set(self.ledger.motion_files), expected)
        self.assertEqual(sum(row["item_related"] for row in entry["motion_files"]), 19)
        self.assertTrue({row["asset"]["id"] for row in entry["event32_motion_files"]}
                        <= expected)
        _target, sets, _stop = e.enumerate_sets({"Kirby": self.ledger}, "b_worst")
        self.assertEqual(sets[0][1], self.ledger.totals()["w_profile_b_worst"])

    def test_unresolved_reader_counts_do_not_call_live_shield_scripts_orphans(self):
        counts = self.ledger.lever7_3
        self.assertEqual(counts["unresolved_without_readers_bytes"], 60)
        self.assertEqual(counts["unresolved_with_readers_bytes"], 10564)
        self.assertEqual(counts["unresolved_without_readers_bytes"]
                         + counts["unresolved_with_readers_bytes"],
                         counts["CONSERVATIVE_RETAIN_bytes"])

    def test_index_disagreements_keep_verdict_provisional(self):
        first = self.ledger.idx.files[0]
        issue = e.Disagreement(first.file_name, "unresolved", 1, "offset:name", "test conflict")
        with patch.object(first, "disagreements", [issue]):
            report = e.build_ledger_json({"Kirby": self.ledger}, self.census, "hwtri")
        self.assertEqual(report["verdict"]["verdict"], "STOP")
        self.assertGreater(report["fighters"][0]["source_validation"]["disagreements"], 0)

    def test_identical_split_palettes_keep_distinct_atoms_and_complete_bytes(self):
        pf = next(p for p in self.ledger.idx.files if p.file_id == 328)
        symbols = ("dKirbyModel_palette_0x1E7C", "dKirbyModel_palette_0x237C")
        rows = [next(o for o in pf.objects if o.symbol == name) for name in symbols]
        self.assertEqual([r.offset for r in rows], [0x1C178, 0x1C678])
        self.assertEqual([r.size for r in rows], [32, 32])
        self.assertEqual(e.included_array_source_bytes(pf, rows[0]),
                         e.included_array_source_bytes(pf, rows[1]))
        atoms = self.ledger.atom_keep_bytes()
        for row in rows:
            self.assertTrue(e.split_palette_provenance(pf, row))
            self.assertTrue(any("location reconciled" in note for note in row.notes))
            self.assertGreaterEqual(atoms[(328, row.symbol)][0], row.size)
        self.assertFalse(self.ledger.idx.disagreements)

    def test_palette_split_requires_the_next_anchor_and_unreferenced_padding(self):
        pf = next(p for p in self.ledger.idx.files if p.file_id == 328)
        row = next(o for o in pf.objects if o.symbol == "dKirbyModel_palette_0x1E7C")
        following = pf.objects[pf.objects.index(row) + 2]
        previous = pf.objects[pf.objects.index(row) - 2]
        with patch.object(previous, "symbol", "dDifferentParent"):
            self.assertFalse(e.split_palette_provenance(pf, row))
        with patch.object(following, "anchor_offset", following.anchor_offset + 4):
            self.assertFalse(e.split_palette_provenance(pf, row))
        with patch.dict(pf.source, referenced=pf.source["referenced"] | {row.offset - 1}):
            self.assertFalse(e.split_palette_provenance(pf, row))

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


# ---------------------------------------------------------------------------
# Recovery levers -- synthetic objects, no corpus needed
# ---------------------------------------------------------------------------


def _synthetic_closure(rows):
    """A one-file ClosureIndex carrying exactly the given ObjectRows."""
    idx = e.ClosureIndex("Synthetic")
    pf = e.ParsedFile(999, "999_Synthetic.c", "<memory>")
    pf.objects.extend(rows)
    idx.files.append(pf)
    return idx


def _reader(type_name, disposition=None):
    view = e._ReaderView.__new__(e._ReaderView)
    view.type_name = type_name
    view.disposition = disposition
    return view


class TestLever71BankMembership(unittest.TestCase):
    """Lever 7.1 on a synthetic costume material binding."""

    def setUp(self):
        rows = [
            make_row(symbol="dSynth_modelparts", type_name="FTModelPart",
                     init_text="= { { (Gfx*)dSynth_dl, (MObjSub**)&dSynth_mchain, "
                               "(AObjEvent32**)&dSynth_cchain, NULL, 0x00 } }"),
            make_row(symbol="dSynth_mchain", type_name="MObjSub",
                     pointer_depth=1,
                     init_text="= { (MObjSub *)dSynth_material, NULL }"),
            make_row(symbol="dSynth_cchain", type_name="AObjEvent32",
                     pointer_depth=1,
                     init_text="= { (AObjEvent32 *)dSynth_program }"),
            make_row(symbol="dSynth_material", type_name="MObjSub",
                     init_text="= { { 0x0, 0, 0, (void**)dSynth_sprites, "
                               "0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "
                               "(void**)dSynth_palettes, 0x0 } }"),
            make_row(symbol="dSynth_sprites", type_name="u8",
                     pointer_depth=1, count=2, elem_size=4, size=8,
                     init_text="= { dSynth_texA, dSynth_texB }"),
            make_row(symbol="dSynth_palettes", type_name="u16",
                     pointer_depth=1, count=5, elem_size=4, size=20,
                     init_text="= { dSynth_pal0, dSynth_pal1, dSynth_pal2, "
                               "dSynth_pal3, dSynth_pal4 }"),
            make_row(symbol="dSynth_program", type_name="u32", count=10,
                     elem_size=4, size=40,
                     # the corpus's own ladder shape: PALETTEID steps with
                     # value == effective frame; TEXID is never set, so the
                     # sprite index stays at the runtime default 0
                     init_text="= { aobjEvent32SetValAfterBlock("
                               "AOBJ_MATFLAG_PALETTEID, 1), 0x3F800000, "
                               "aobjEvent32SetValAfterBlock("
                               "AOBJ_MATFLAG_PALETTEID, 1), 0x40000000, "
                               "aobjEvent32SetValAfterBlock("
                               "AOBJ_MATFLAG_PALETTEID, 1), 0x40400000, "
                               "aobjEvent32SetValAfterBlock("
                               "AOBJ_MATFLAG_PALETTEID, 1), 0x40800000, "
                               "aobjEvent32End() }"),
        ]
        self.idx = _synthetic_closure(rows)
        self.graph = e.SymbolGraph(self.idx)
        self.cm = e.resolve_costume_membership(self.idx, self.graph, (0, 1, 2, 3))

    def test_palette_ladder_resolves_per_costume(self):
        # objanim.c: gcAddMObjMatAnimJoint sets anim_wait = -anim_frame
        # (:844), gcParseMObjMatAnimJoint consumes commands while
        # anim_wait <= 0 (:1240) and each SetValAfterBlock step fires at
        # cursor+payload (:1049-1083, :1296) -- so costume c evaluates the
        # ladder to value c: pal0 is the frame-0/default entry, selected
        # by costume 0 alone.  (Mirrors the Kirby body-ladder pin:
        # frozenset({c}) per entry.)
        got = {sym: self.cm.selected.get((999, sym), frozenset())
               for sym in ("dSynth_pal0", "dSynth_pal1", "dSynth_pal2",
                           "dSynth_pal3", "dSynth_pal4")}
        self.assertEqual(got["dSynth_pal0"], frozenset({0}))
        self.assertEqual(got["dSynth_pal1"], frozenset({1}))
        self.assertEqual(got["dSynth_pal2"], frozenset({2}))
        self.assertEqual(got["dSynth_pal3"], frozenset({3}))

    def test_unselected_bank_is_animation_reachable_not_dropped(self):
        # pal4 sits in a resolved table but no costume selects it: it must
        # keep an EMPTY membership (charged, retained), never be dropped
        self.assertIn((999, "dSynth_pal4"),
                      self.cm.banks_in_resolved_tables)
        self.assertEqual(self.cm.selected.get((999, "dSynth_pal4")),
                         frozenset())

    def test_default_texture_index_is_common(self):
        # TEXID never set -> runtime default 0 -> sprites[0] for every
        # costume; sprites[1] is animation-reachable
        self.assertEqual(self.cm.selected.get((999, "dSynth_texA")),
                         frozenset({0, 1, 2, 3}))
        self.assertEqual(self.cm.selected.get((999, "dSynth_texB")),
                         frozenset())

    def test_material_counted_resolved(self):
        self.assertEqual(self.cm.materials_resolved, 1)
        self.assertEqual(self.cm.materials_unresolved, 0)

    def test_team_costume_beyond_four_selects_its_own_bank(self):
        cm = e.resolve_costume_membership(self.idx, self.graph, (0, 1, 2, 3, 4))
        self.assertEqual(cm.selected[(999, "dSynth_pal4")], frozenset({4}))

    def test_control_flow_at_team_costume_frame_is_not_ignored(self):
        row = make_row(type_name="u32", init_text=(
            "= { aobjEvent32Wait(4), aobjEvent32Jump(), 0x00000000 }"))
        self.assertIsNotNone(e.parse_matanim_program(row, frame_limit=3))
        self.assertIsNone(e.parse_matanim_program(row, frame_limit=4))
        self.assertIsNone(e.parse_matanim_program(row))


class TestLever72WeaponNatives(unittest.TestCase):
    """Lever 7.2: donor geometry priced at its owner's native image."""

    def setUp(self):
        self.census = {"Yoshi": {"High": 12088, "Low": 8840}}

    def test_owned_donor_is_priced_like_body_geometry(self):
        row = make_row(symbol="dYoshiModel_Vtx_1", type_name="Vtx", size=64,
                       init_text="= { #include <YoshiModel/v.inc.c> }")
        a = e.classify_object(row, "donor_model", "Kirby", {},
                              donor_owner="Yoshi")
        self.assertEqual(a.disposition, "NATIVE_REPLACE_BODY")
        self.assertIn("Yoshi", a.reason)

    def test_unowned_weapon_keeps_its_own_unresolved_line(self):
        row = make_row(symbol="dFoxUnknown_Vtx", type_name="Vtx", size=64,
                       init_text="= { #include <x.vtx.inc.c> }")
        a = e.classify_object(row, "donor_model", "Kirby", {},
                              donor_owner=None)
        self.assertEqual(a.disposition, "NATIVE_REPLACE_WEAPON")

    def test_owner_detection_and_census_charge(self):
        self.assertEqual(e.donor_native_owner("338_YoshiModel.c",
                                              self.census), "Yoshi")
        self.assertEqual(e.donor_native_owner("348_KirbySpecial2.c",
                                              self.census), None)
        # a static owner (Mario/Fox) owns its model but charges 0 extra
        # bytes: the translated form is already in the ARM9 baseline
        self.assertEqual(e.donor_native_owner("296_MarioModel.c",
                                              self.census), "Mario")
        self.assertEqual(
            e.donor_native_census_bytes("Mario", self.census), 0)
        self.assertEqual(
            e.donor_native_census_bytes("Yoshi", self.census), 20928)


class TestLever73U32ByReaders(unittest.TestCase):
    """Lever 7.3: u32 conservative retains classified by their readers."""

    PROGRAM = ("= { aobjEvent32SetValAfterBlock(AOBJ_MATFLAG_PALETTEID, 1), "
               "0x3F800000, aobjEvent32End() }")

    def test_structural_program_with_animation_reader_translates(self):
        row = make_row(symbol="dSynth_prog", type_name="u32",
                       init_text=self.PROGRAM)
        parse = e.parse_matanim_program(row)
        self.assertIsNotNone(parse)
        d, _reason = e.classify_u32_by_readers(
            row, {_reader("AObjEvent32", "PTR_TABLE")}, parse)
        self.assertEqual(d, "EVENT_STREAM_RETAIN")

    def test_semantic_reader_retains_whole(self):
        row = make_row(symbol="dSynth_table", type_name="u32",
                       init_text="= { (u32)&dSynth_other, 0 }")
        d, _reason = e.classify_u32_by_readers(
            row, {_reader("FTAttributes", "RETAINED_SEMANTIC")}, None)
        self.assertEqual(d, "RETAINED_SEMANTIC")

    def test_transient_only_reader_drops(self):
        row = make_row(symbol="dSynth_scaffold", type_name="u32",
                       init_text="= { 0 }")
        d, _reason = e.classify_u32_by_readers(
            row, {_reader("DObjDLLink", "SETUP_TRANSIENT")}, None)
        self.assertEqual(d, "SETUP_TRANSIENT")

    def test_no_reader_evidence_stays_unresolved(self):
        row = make_row(symbol="dSynth_orphan", type_name="u32",
                       init_text=self.PROGRAM)
        parse = e.parse_matanim_program(row)
        d, reason = e.classify_u32_by_readers(row, set(), parse)
        self.assertEqual(d, "CONSERVATIVE_RETAIN")
        self.assertIn("unresolved", reason)

    def test_unparseable_program_with_readers_stays_unresolved(self):
        row = make_row(symbol="dSynth_jumpy", type_name="u32",
                       init_text="= { aobjEvent32JumpCmd(0x1, 0x2), 0 }")
        self.assertIsNone(e.parse_matanim_program(row))
        d, reason = e.classify_u32_by_readers(
            row, {_reader("AObjEvent32", "PTR_TABLE")}, None)
        self.assertEqual(d, "CONSERVATIVE_RETAIN")
        self.assertIn("reader(s) exist", reason)


if __name__ == "__main__":
    unittest.main(verbosity=2)
