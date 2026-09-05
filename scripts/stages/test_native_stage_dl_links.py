"""Source fixtures for multi-head stage packets; no ROM or compiler needed."""
import dataclasses
from pathlib import Path
import unittest

import check_nds_native_stage as check
import generate_nds_native_stage as generator
from native_stage_descriptors import get_descriptor


class StageDLLinkTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[2]
        cls.packets = {name: generator.generate(cls.root, name)
                       for name in ("sector", "hyrule", "zebes")}

    def test_source_mapping_and_full_packet_contracts(self):
        for name, packet in self.packets.items():
            with self.subTest(stage=name):
                check.verify_packet(packet, get_descriptor(name))
                check.verify_dl_link_bindings(self.root, packet, get_descriptor(name))

    def test_hyrule_shared_dobj_keeps_both_display_heads(self):
        packet = self.packets["hyrule"]
        bindings = [i for i, dobj in enumerate(packet.binding_dobjs) if dobj == 3]
        self.assertEqual(len(bindings), 2)
        self.assertEqual([packet.binding_heads[i] for i in bindings], [0, 1])
        self.assertEqual([packet.bindings[i].root_offset for i in bindings],
                         [0x4180, 0x44C8])
        self.assertEqual(packet.baked_world_matrices[bindings[0]],
                         packet.baked_world_matrices[bindings[1]])

    def test_missing_second_link_does_not_pass(self):
        packet = self.packets["hyrule"]
        changed = dataclasses.replace(packet, binding_dobjs=packet.binding_dobjs[:-1])
        with self.assertRaises(generator.Falsifier):
            generator.validate_packet(changed, "hyrule")

    def test_wrong_head_is_rejected_against_original(self):
        packet = self.packets["hyrule"]
        heads = list(packet.binding_heads)
        heads[heads.index(1)] = 0
        changed = dataclasses.replace(packet, binding_heads=tuple(heads))
        with self.assertRaises(generator.Falsifier):
            check.verify_dl_link_bindings(self.root, changed, get_descriptor("hyrule"))

    def test_specialized_program_is_optional(self):
        packet = self.packets["sector"]
        self.assertIsNone(generator.build_generated_segment0_program(packet, "sector"))
        output = generator.render_include(packet, "sector").decode("ascii")
        self.assertIn("sNdsNativeStageSectorBindingHeads", output)
        self.assertNotIn("sNdsNativeStageSectorSegment0ColdCertificate", output)

    def test_zebes_layer1_binds_one_material_per_segment_slot(self):
        """18 MObjs across 8 DLLink bindings; branch slot 8*i selects MObj i.

        Source: dStageZebesFile2_Layer1MObj_MObjSub@0x2B48 is an MObjSub**
        per DObj (gcAddMObjAll), and gcDrawMObjForDObj emits one 8-byte
        branch slot per MObj, so a DObj's display list selects material i
        with a segment-0xE branch at offset 8*i (objdisplay.c:1204/1259).
        """
        packet = self.packets["zebes"]
        self.assertEqual(len(packet.materials), 18)
        by_binding = {}
        for event in packet.materials:
            by_binding.setdefault(event.binding_index, []).append(event)
        self.assertEqual(sorted(by_binding), [0, 1, 3, 4, 8, 16, 21, 24])
        for binding_index, events in by_binding.items():
            self.assertEqual(
                [event.segment_index for event in events],
                [8 * index for index in range(len(events))],
                f"binding {binding_index} lost gcDrawMObjForDObj's slot order",
            )
        # The palette-only MObjSubs decode to the same three-command program.
        for event in packet.materials:
            self.assertEqual(event.flags, 0x0004)
            self.assertEqual(event.source_command_count, 3)

    def test_zebes_wrong_material_slot_does_not_pass(self):
        desc = get_descriptor("zebes")
        rows = list(desc.material_sources)
        # Point binding 0's first MObj at branch slot 0x18, which its
        # display list never selects.
        rows[0] = (rows[0][0], rows[0][1], rows[0][2], 0x18)
        mutated = dataclasses.replace(desc, material_sources=tuple(rows))
        with self.assertRaises(generator.Falsifier):
            generator.generate(self.root, mutated)

    def test_existing_packet_bytes_remain_frozen(self):
        for name in ("dreamland", "yoster", "castle", "jungle",
                     "sector", "hyrule", "zebes"):
            with self.subTest(stage=name):
                desc = get_descriptor(name)
                packet = generator.generate(self.root, desc)
                self.assertEqual(generator.sha256(generator.render_include(packet, desc)),
                                 desc.include_sha)


if __name__ == "__main__":
    unittest.main()
