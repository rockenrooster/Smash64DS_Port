"""Saffron haze omission as generated-program/geometry proof; no ROM or emulator.

Owner authorization: docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md permits
removal of only Saffron's identified white haze panel (file 112, layer-3 DObj
slot 2 / link 1 / head 1, target DL 0x1664). This test generates the Yamabuki
packet in memory and proves the omission removes only that drawable while
every binding/DObj identity the runtime topology and admission validate is
retained. It writes no shared packets and builds no ROM; Main owns the final
shared regeneration and include_sha re-pin.
"""
import sys
import unittest
from dataclasses import replace
from pathlib import Path

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

import check_nds_native_stage as check
import generate_nds_native_stage as generator
from native_stage_descriptors import get_descriptor

ROOT = Path(__file__).resolve().parents[2]
SOURCE_112 = (
    ROOT / "decomp/BattleShip-main/decomp/src/relocData/112_StageYamabukiFile2.c"
)
BUG_NOTES = ROOT / "docs/p2/BUG_NOTES.md"

DRAW_FIELDS = ("triangle_count", "run_count", "source_vertex_count",
               "vertex_command_count", "source_command_count",
               "texture_epoch_count")


class YamabukiHazeOmissionTests(unittest.TestCase):
    # Pre-omission pins for the baseline packet: the current descriptor
    # carries the post-omission re-pin, so the unomitted control restores
    # the values the host census measured before the omission.
    BASELINE_COUNTS = {
        "commands": 963, "vertex_commands": 78, "source_vertices": 429,
        "triangle_commands": 119, "triangles": 232, "runs": 81,
        "texture_epochs": 67, "submit_classes": (58, 138, 36),
        "state_events": 447, "sync_events": 294, "alpha_clone_vertices": 12,
    }
    BASELINE_SEGMENTS = ((0, 4, 0, 6, 0, 33), (1, 6, 6, 9, 33, 26),
                         (3, 17, 15, 2, 59, 22))

    @classmethod
    def setUpClass(cls):
        cls.desc = get_descriptor("yamabuki")
        cls.packet = generator.generate(ROOT, "yamabuki")
        baseline_counts = dict(cls.desc.expected_counts)
        baseline_counts.update(cls.BASELINE_COUNTS)
        cls.baseline = generator.generate(
            ROOT, replace(cls.desc, omitted_draw_roots=(),
                          expected_counts=baseline_counts,
                          segment_partition=cls.BASELINE_SEGMENTS))
        layer3 = next(s for s in cls.packet.segments if s.owner == 3)
        cls.haze = next(
            i for i in range(layer3.first_binding,
                             layer3.first_binding + layer3.binding_count)
            if cls.packet.binding_heads[i] == 1)
        cls.sibling = next(
            i for i in range(layer3.first_binding,
                             layer3.first_binding + layer3.binding_count)
            if cls.packet.binding_heads[i] == 0)

    def test_descriptor_declares_single_stable_root(self):
        self.assertEqual(self.desc.omitted_draw_roots, ((112, "layer3", 2, 1, 0x8688),))

    def test_source_identity_present_verbatim(self):
        text = SOURCE_112.read_text(encoding="utf-8")
        self.assertIn(
            "DObjDLLink "
            "dStageYamabukiFile2_Layer1MatAnim_MatAnimJoint_data_0x1664_link1[2]",
            text)
        self.assertIn("{ 1, dStageYamabukiFile2_Layer1MatAnim_MatAnimJoint_data_0x1664 },", text)
        self.assertIn("DObjDesc dStageYamabukiFile2_Layer3DObj[] = {", text)
        self.assertIn(
            "{ 2, (void*)"
            "dStageYamabukiFile2_Layer1MatAnim_MatAnimJoint_data_0x1664_link1,",
            text)
        notes = BUG_NOTES.read_text(encoding="utf-8")
        self.assertIn("Saffron white band = the source's own haze panel", notes)

    def test_haze_drawable_removed_identities_retained(self):
        packet, baseline = self.packet, self.baseline
        self.assertEqual(len(packet.bindings), 17)
        self.assertEqual(len(packet.dobjs), 19)
        self.assertEqual(list(packet.binding_dobjs), list(baseline.binding_dobjs))
        self.assertEqual(list(packet.binding_heads), list(baseline.binding_heads))
        self.assertEqual([b.root_offset for b in packet.bindings],
                         [b.root_offset for b in baseline.bindings])
        haze, base_haze = packet.bindings[self.haze], baseline.bindings[self.haze]
        self.assertNotEqual(base_haze.triangle_count, 0)
        for field in DRAW_FIELDS:
            self.assertEqual(getattr(haze, field), 0, field)
        self.assertNotEqual(haze.root_offset, 0)
        check.verify_dl_link_bindings(ROOT, packet, self.desc)

    def test_all_sibling_drawables_bit_identical(self):
        # The haze is the final binding. Compare actual geometry/material words,
        # not only counts: moved vertices or changed colors must fail too.
        for field in ("runs", "vertices", "corners", "epochs", "policies",
                      "materials", "baked_world_matrices", "dobjs"):
            kept = getattr(self.packet, field)
            original = getattr(self.baseline, field)
            self.assertEqual(kept, original[:len(kept)], field)
        for index, (kept, base) in enumerate(
                zip(self.packet.bindings, self.baseline.bindings)):
            if index == self.haze:
                continue
            with self.subTest(binding=index):
                for field in DRAW_FIELDS + ("root_offset",):
                    self.assertEqual(getattr(kept, field), getattr(base, field),
                                     field)
        self.assertEqual(len(self.packet.corners) // 3,
                         len(self.baseline.corners) // 3
                         - self.baseline.bindings[self.haze].triangle_count)
        self.assertEqual(len(self.packet.runs),
                         len(self.baseline.runs)
                         - self.baseline.bindings[self.haze].run_count)

    def test_layer3_sibling_and_layer0_floor_preserved(self):
        sibling, base_sibling = (self.packet.bindings[self.sibling],
                                 self.baseline.bindings[self.sibling])
        self.assertNotEqual(sibling.triangle_count, 0)
        self.assertEqual(sibling.triangle_count, base_sibling.triangle_count)
        layer0 = next(s for s in self.packet.segments if s.owner == 0)
        layer0_tris = sum(r.triangle_count for r in self.packet.runs[
            layer0.first_run:layer0.first_run + layer0.run_count])
        base_layer0 = next(s for s in self.baseline.segments if s.owner == 0)
        base_layer0_tris = sum(r.triangle_count for r in self.baseline.runs[
            base_layer0.first_run:base_layer0.first_run + base_layer0.run_count])
        self.assertNotEqual(layer0_tris, 0)
        self.assertEqual(layer0_tris, base_layer0_tris)

    def test_stale_root_omission_is_rejected(self):
        with self.assertRaisesRegex(generator.Falsifier, "unmatched drawable omissions"):
            generator.generate(ROOT, replace(self.desc,
                omitted_draw_roots=((112, "layer3", 2, 1, 0x1664),)))


if __name__ == "__main__":
    unittest.main()
