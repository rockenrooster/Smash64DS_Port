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
                       for name in ("sector", "hyrule")}

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

    def test_existing_packet_bytes_remain_frozen(self):
        for name in ("dreamland", "yoster", "castle", "jungle"):
            with self.subTest(stage=name):
                desc = get_descriptor(name)
                packet = generator.generate(self.root, desc)
                self.assertEqual(generator.sha256(generator.render_include(packet, desc)),
                                 desc.include_sha)


if __name__ == "__main__":
    unittest.main()
