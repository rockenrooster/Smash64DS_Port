"""Zebes acid actor descriptor proof; no ROM or emulator.

The native Zebes packet rejected the acid pool at domain2 scene22
identity0x03f2009d status3 root0x9d8: grZebesMakeAcid
(grzebes.c:71-115) composes its GObj from llGRZebesMapAcidDObjDesc
(file 157 @ 0xB08) and draws it through gcDrawDObjTreeDLLinksForGObj
at link 12, but the descriptor owned only the layer1 map owner, so the
acid DL had no native program. This test generates the Zebes packet in
memory and proves the new acid owner covers exactly the source DL root
0x09D8 with the live four-frame sprite material contract intact, while
every layer1 binding and the ground hazard/source-update path are
bit-identical. It writes no shared packets and builds no ROM; Main owns
the final shared regeneration (the on-disk .inc), the C packet
registration, and runtime admission after all inputs are stable.

Scope: only root 0x09D8 completeness and the live sprite material
frame contract. The 0x0BD8 material script's ROTX/TRAY/SCAY entries are
material-script channels; this test pins their presence and asserts no
spatial-rotation claim (the packet snapshots the immutable stream plus
live material state, never a rotated geometry pose). No
visibility/performance claims.
"""
import sys
import unittest
from dataclasses import replace
from pathlib import Path

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

import generate_nds_native_stage as generator
from native_stage_descriptors import get_descriptor

ROOT = Path(__file__).resolve().parents[2]
TYPED_157 = (
    ROOT / "decomp/BattleShip-main/decomp/src/relocData/157_StageZebesFile3.c"
)
GRZEBES = ROOT / "decomp/BattleShip-main/decomp/src/gr/grcommon/grzebes.c"

# Pre-acid pins: the descriptor before the acid owner landed (layer1 only).
BASELINE_COUNTS = {
    "callbacks": 1, "dobjs": 28, "bindings": 25, "commands": 586,
    "vertex_commands": 45, "source_vertices": 309,
    "modify_vertex_commands": 0, "triangle_commands": 78, "triangles": 144,
    "runs": 55, "texture_epochs": 41, "material_events": 18,
    "submit_classes": (92, 0, 52), "state_events": 251, "state_deltas": 123,
    "sync_events": 165, "cross_runs": 0, "cross_tris": 0,
    "cross_corners": 0, "alpha_clone_vertices": 21,
}


class ZebesAcidDescriptorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.desc = get_descriptor("zebes")
        cls.packet = generator.generate(ROOT, "zebes")
        layer1 = next(
            row for row in cls.desc.owner_specs if row[1] == "layer1")
        baseline_counts = dict(cls.desc.expected_counts)
        baseline_counts.update(BASELINE_COUNTS)
        cls.baseline = generator.generate(
            ROOT, replace(
                cls.desc,
                expected_counts=baseline_counts,
                asset_order=(("stage_geometry", 1), ("stage_map", 4)),
                owner_specs=(layer1,),
                material_sources=tuple(cls.desc.material_sources[:18]),
                material_command_partition=(3,) * 18,
                segment_partition=((1, 6, 0, 25, 0, 55),),
                callback_partition=(
                    ("layer1", "grDisplayLayer1SecProcDisplay", 6),),
                adapter_segment_count=1, adapter_dobj_count=28,
                adapter_binding_count=25, adapter_asset_count=2,
                adapter_material_count=18,
                adapter_asset_ids=(0x69, 0x101),
                adapter_asset_sizes=(0xDF60, 0x00E0)))
        cls.acid = next(
            row for row in cls.desc.owner_specs if row[1] == "acid")
        cls.typed = TYPED_157.read_text(encoding="utf-8")
        cls.ground = GRZEBES.read_text(encoding="utf-8")

    def test_acid_owner_row_matches_source_actor(self):
        self.assertEqual(
            self.acid,
            (2, "acid", "stage_actors", 0xB08, 3, 12,
             "gcDrawDObjTreeDLLinksForGObj", True))
        self.assertIn("grZebesMakeAcid", self.ground)
        self.assertIn(
            "gcAddGObjDisplay(map_gobj, gcDrawDObjTreeDLLinksForGObj, 12",
            self.ground)

    def test_acid_inputs_pinned_to_actual_banks(self):
        actors = self.desc.o2r_inputs["stage_actors"]
        self.assertEqual(actors["file_id"], 157)
        self.assertEqual(
            (actors["internal_fixups"], actors["external_fixups"]), (16, 0))
        self.assertEqual(
            actors["sha256"],
            "7e4a6f970a333635b4fe2591db45d6a28942c294085dc773e9bb58165c62dc2a")
        self.assertEqual(
            actors["payload_sha256"],
            "2ba9176aef611ce0043225097b1305c8fad4ce8addfacfd3b42a2cbc755d670f")
        typed = self.desc.text_inputs["actors_typed"]
        self.assertEqual(
            typed["sha256"],
            "7a6daf9c1939b9b35bbb07586bfdc12c3961c2816c4431a1d524e662ec58f0d3")

    def test_acid_root_set_complete_at_0x09d8(self):
        # File 157 holds exactly one display list symbol; the DLLink table
        # targets only it, so the actor's complete root set is { 0x09D8 }.
        acid_bindings = [
            index for index, binding in enumerate(self.packet.bindings)
            if self.packet.segments[1].first_binding
            <= index < self.packet.segments[1].first_binding
            + self.packet.segments[1].binding_count]
        self.assertEqual(len(acid_bindings), 1)
        binding = self.packet.bindings[acid_bindings[0]]
        self.assertEqual(binding.root_offset, 0x9D8)
        self.assertEqual(binding.asset_index, 1)
        self.assertEqual(
            (self.packet.binding_dobjs[acid_bindings[0]],
             self.packet.binding_heads[acid_bindings[0]]), (29, 1))
        self.assertIn("Gfx dStageZebesFile3_DL_0x09D8[36]", self.typed)
        self.assertIn(
            "{ 1, dStageZebesFile3_DL_0x09D8 },", self.typed)
        self.assertIn(
            "DObjDesc dStageZebesFile3_DObjDesc_0x0B08[3]", self.typed)

    def test_live_sprite_material_frame_contract(self):
        event = self.packet.materials[18]
        self.assertEqual(
            (event.binding_index, event.mobj_offset, event.segment_index,
             event.source_command_count, event.flags),
            (25, 0x8D8, 0, 10, 0x6B))
        # Four-frame CI4/TLUT contract, file-grounded in the typed source.
        for offset in ("0x00A8", "0x02B0", "0x04B8", "0x06C0"):
            self.assertIn(
                "/* @ %s — sprite-frame texture */" % offset, self.typed)
            self.assertIn("/* @tex fmt=CI4 dim=32x32 */", self.typed)
        for offset in ("0x0008", "0x0030", "0x0058", "0x0080"):
            self.assertIn("— 16-colour TLUT */", self.typed)
            self.assertIn("dStageZebesFile3_LUT_%s[16]" % offset, self.typed)
        self.assertIn(
            "MObjSub dStageZebesFile3_MObjSub_0x08D8[1]", self.typed)
        self.assertIn("dStageZebesFile3_sprites_0x08C8", self.typed)
        # Source wrapper 0x8C0 feeds the MObjSub list live via gcAddMObjAll.
        self.assertIn(
            "lbRelocGetFileData(MObjSub***, map_head, "
            "&llGRZebesMapAcidMObjSub)", self.ground)
        # TRAY joint animation stays source-side; presence pinned only.
        self.assertIn(
            "dStageZebesFile3_AnimJoint_0x0B98[12]", self.typed)

    def test_material_script_makes_no_spatial_claim(self):
        # The 0x0BD8 script exists and the packet snapshots material state;
        # nothing in the packet derives a rotated geometry pose from its
        # ROTX/TRAY/SCAY channels.
        self.assertIn(
            "dStageZebesFile3_AnimJoint_0x0BD8[122]", self.typed)
        self.assertIn("AOBJ_FLAG_ROTX", self.typed)
        acid_runs = self.packet.runs[
            self.packet.segments[1].first_run:
            self.packet.segments[1].first_run
            + self.packet.segments[1].run_count]
        self.assertTrue(acid_runs)
        for run in acid_runs:
            self.assertEqual(run.binding_index, 25)

    def test_layer1_drawables_bit_identical(self):
        packet, baseline = self.packet, self.baseline
        self.assertEqual(
            list(packet.bindings[:25]), list(baseline.bindings))
        self.assertEqual(list(packet.runs[:55]), list(baseline.runs))
        self.assertEqual(list(packet.dobjs[:28]), list(baseline.dobjs))
        self.assertEqual(
            list(packet.materials[:18]), list(baseline.materials))
        self.assertEqual(list(packet.epochs[:41]), list(baseline.epochs))
        self.assertEqual(packet.segments[0], baseline.segments[0])
        self.assertEqual(
            tuple(event.source_command_count
                  for event in packet.materials[:18]), (3,) * 18)

    def test_ground_hazard_and_source_updates_preserved(self):
        self.assertEqual(self.desc.map_constructor_text_key, "ground")
        self.assertEqual(
            self.desc.map_constructor_token, "grZebesMakeGround(")
        for token in (
                "grZebesMakeGround(", "grZebesMakeAcid(",
                "ftMainCheckAddGroundHazard(acid_gobj, "
                "grZebesAcidCheckGetDamageKind)",
                "DObjGetStruct(gGRCommonStruct.zebes.map_gobj)"
                "->translate.vec.f.y",
                "gcAddAnimAll"):
            self.assertIn(token, self.ground)

    def test_rendered_include_hash_pinned_in_memory(self):
        rendered = generator.render_include(self.packet, self.desc)
        self.assertEqual(generator.sha256(rendered), self.desc.include_sha)


if __name__ == "__main__":
    unittest.main()
