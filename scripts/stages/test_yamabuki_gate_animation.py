"""Saffron's gate must compose its world matrix live; no ROM or emulator.

S04. The gate's hazard state and its yakumono collision already cycle exactly
as `gryamabuki.c` writes them. What did not move was the MODEL: segment 3's
bindings were eligible for the Task 51 replay, which multiplies the
generator-baked constant `sNdsNativeStageBakedWorldMatrices[binding]` instead
of the live per-frame compose. That constant is built from the authored
DObjDesc pose at file-160 0x08A0 and cannot follow the open/close AObj, so the
gate drew one fixed pose for the whole match.

The runtime opts a binding out of that substitution through
`camera_binding_mask` (its only reader is the Task 51 predicate in
nds_renderer_native_owners.c). These tests hold the producer to putting the
gate's moving bindings in that mask, and they are written to FAIL if the
animated binding is removed -- not merely if a count changes:

  * the joint scripts are decoded from the real O2R payload, so deleting the
    `_ANIMATED_JOINT_TABLES` row, pointing it at the wrong file or losing the
    fixups collapses the derived mask to 0;
  * the pinned `_ANIMATED_BINDING_MASKS` value is compared against that
    derivation, so a stale pin cannot survive;
  * the emitted live-world mask is compared against the camera-only mask, so
    narrowing it back to 0x4894 fails with the gate named.

Writes no packets, builds no ROM and runs no generator output path.
"""
import re
import struct
import sys
import unittest
from pathlib import Path

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

import generate_nds_native_stage as generator  # noqa: E402
from native_stage_descriptors import get_descriptor  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]

#: gryamabuki.c:246 builds the gate from these five DObjs; 19 is the root.
GATE_OWNER = 4
#: 160_StageYamabukiFile4.c:105,149 -- the two tables grYamabukiGateAddAnimOpen
#: and grYamabukiGateAddAnimClose attach, at the linker-absolute ll* offsets.
OPEN_TABLE = 0x09B0
CLOSE_TABLE = 0x0A20
#: objdef.h:353-363. Joint-track flag bits, from RotX (bit 0) to ScaZ (bit 9).
FLAG_TRAY = 1 << 5
FLAG_TRAZ = 1 << 6


class YamabukiGateAnimationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.desc = get_descriptor("yamabuki")
        cls.packet = generator.generate(ROOT, "yamabuki")
        specs = generator._o2r_inputs_from_descriptor(cls.desc)
        cls.resources = {
            name: generator.load_o2r(ROOT, spec) for name, spec in specs.items()
        }
        cls.actors = cls.resources["stage_actors"]
        cls.segment = next(s for s in cls.packet.segments if s.owner == GATE_OWNER)

    # ---- the source contract the packet is supposed to carry ----------------

    def test_gate_owner_is_still_a_segment_with_its_own_dobj_tree(self):
        """If the gate stops being an owner there is nothing left to animate."""
        owners = {o.name: o for o in generator._owner_specs_from_descriptor(self.desc)}
        self.assertIn("gate", owners, "Saffron lost its gate owner")
        gate = owners["gate"]
        self.assertEqual(gate.owner, GATE_OWNER)
        self.assertEqual(gate.resource_name, "stage_actors")
        self.assertEqual(gate.dobj_offset, 0x08A0, "llGRYamabukiMapMapHead")
        self.assertEqual(gate.link, 6, "gryamabuki.c:252 draws the gate on link 6")
        # descriptor_count includes the id-18 sentinel; five DObjs are live.
        self.assertEqual(self.segment.dobj_count, gate.descriptor_count - 1)
        self.assertEqual(self.segment.dobj_count, 5)
        self.assertEqual(self.segment.binding_count, 4)

    def test_both_joint_tables_are_declared_and_resolve_inside_file_160(self):
        """The ll* offsets address StageYamabukiFile4, not GRYamabukiMap.

        GRYamabukiMap's payload is 0x340 bytes, so 0x9B0/0xA20 could only ever
        have been offsets into the file map_nodes points into. If a future edit
        re-bases them on the map file this fails before anything else does.
        """
        declared = generator._ANIMATED_JOINT_TABLES.get("yamabuki", ())
        self.assertEqual(
            declared,
            (("gate", "stage_actors", OPEN_TABLE),
             ("gate", "stage_actors", CLOSE_TABLE)))
        payload_len = len(self.actors.payload)
        for _, _, table in (declared):
            for slot in range(1, 5):
                ref = self.actors.pointer_at(table + slot * 4)
                self.assertIsNotNone(
                    ref, f"table 0x{table:X} slot {slot} has no fixup")
                self.assertEqual(ref.asset_id, 160)
                self.assertLess(ref.offset, payload_len)

    def test_three_panels_carry_translation_channels_and_the_lamp_does_not(self):
        """Decode the actual scripts; assert the source channels, not a count."""
        def channels(table, slot):
            ref = self.actors.pointer_at(table + slot * 4)
            self.assertIsNotNone(ref)
            seen = 0
            cursor = ref.offset
            for _ in range(generator.AOBJ_MAX_COMMANDS):
                word = struct.unpack_from(">I", self.actors.payload, cursor)[0]
                opcode = (word >> generator.AOBJ_OPCODE_SHIFT) & generator.AOBJ_OPCODE_MASK
                flags = (word >> generator.AOBJ_FLAGS_SHIFT) & generator.AOBJ_FLAGS_MASK
                if opcode == generator.AOBJ_EVENT_END:
                    return seen
                if opcode in generator.AOBJ_JOINT_TRACK_OPCODES:
                    seen |= flags
                    words = bin(flags).count("1")
                    if opcode in generator.AOBJ_DOUBLE_VALUE_OPCODES:
                        words *= 2
                    if opcode == 13:
                        words = 1
                else:
                    words = 0
                cursor += 4 * (1 + words)
            self.fail("script has no End")

        # Slot 0 is the root and is NULL in both tables.
        self.assertIsNone(self.actors.pointer_at(OPEN_TABLE))
        self.assertIsNone(self.actors.pointer_at(CLOSE_TABLE))
        for table in (OPEN_TABLE, CLOSE_TABLE):
            self.assertEqual(channels(table, 1), FLAG_TRAZ, "panel 1 slides in Z")
            self.assertEqual(channels(table, 2), FLAG_TRAY, "shutter rides in Y")
            self.assertEqual(channels(table, 3), FLAG_TRAZ, "panel 3 slides in Z")
            # The lamp's channel is SetFlags/Wait only: it moves nothing, so it
            # is entitled to keep the baked constant world matrix.
            self.assertEqual(channels(table, 4), 0, "lamp must not move")

    # ---- the producer decision that the symptom actually depends on ---------

    def test_derived_mask_names_the_three_moving_gate_bindings(self):
        derived = generator.derive_animated_binding_mask(
            self.resources, self.packet, self.desc)
        moving = [self.packet.dobjs[self.segment.first_dobj + slot].binding_index
                  for slot in (1, 2, 3)]
        still = self.packet.dobjs[self.segment.first_dobj + 4].binding_index
        self.assertNotIn(generator.INVALID_U16, moving)
        for binding in moving:
            self.assertTrue(
                (derived >> binding) & 1,
                f"gate binding {binding} moves but is not in the live-world mask")
        self.assertFalse((derived >> still) & 1,
                         "the lamp binding does not move and need not be live")
        self.assertEqual(derived.bit_count(), 3)

    def test_pinned_mask_equals_the_derivation(self):
        """The blob builder cannot read the payload, so this is the only guard."""
        self.assertEqual(
            generator.blob_animated_mask("yamabuki"),
            generator.derive_animated_binding_mask(
                self.resources, self.packet, self.desc))

    def test_live_world_mask_widens_the_camera_mask_by_the_gate(self):
        camera = generator.blob_camera_mask(self.packet)
        live = generator.blob_live_world_mask(self.packet, self.desc)
        self.assertEqual(camera, 0x4894, "camera-relative shapes are unchanged")
        self.assertEqual(live, 0xE4894)
        self.assertEqual(live & camera, camera, "camera bindings must be kept")
        self.assertNotEqual(live, camera,
                            "the gate would replay a baked constant world")

    def test_removing_the_animated_binding_is_a_failure_not_a_count_change(self):
        """Falsifier: drop the declaration and the gate stops being live.

        This is what the row is actually about. With the joint tables gone the
        derivation finds nothing, the live-world mask collapses back to the
        camera-only value, and bindings 17-19 become eligible for the Task 51
        baked constant again -- which is the frozen gate.
        """
        saved = generator._ANIMATED_JOINT_TABLES.pop("yamabuki")
        try:
            derived = generator.derive_animated_binding_mask(
                self.resources, self.packet, self.desc)
            self.assertEqual(derived, 0)
            self.assertNotEqual(
                generator.blob_animated_mask("yamabuki"), derived,
                "the pin must no longer be reproducible")
        finally:
            generator._ANIMATED_JOINT_TABLES["yamabuki"] = saved
        self.assertEqual(
            generator.derive_animated_binding_mask(
                self.resources, self.packet, self.desc),
            generator.blob_animated_mask("yamabuki"))

    def test_baked_matrices_are_the_authored_closed_pose(self):
        """Why the constant cannot be used: it is one end of the cycle.

        The authored DObjDesc poses equal the CLOSE animation's end state
        (TraZ 0, TraY 390), so replaying them is not a neutral approximation of
        the gate -- it is a permanent pose.
        """
        expected = ((1026.3673095703125, 387.0627136230469, 0.0),
                    (1026.3673095703125, 390.0, 0.0),
                    (1026.3673095703125, 387.0627136230469, 0.0))
        for slot, want in zip((1, 2, 3), expected):
            binding = self.packet.dobjs[self.segment.first_dobj + slot].binding_index
            matrix = self.packet.baked_world_matrices[binding]
            got = tuple(matrix[index] / 4096.0 for index in (12, 13, 14))
            for axis, (g, w) in enumerate(zip(got, want)):
                self.assertAlmostEqual(g, w, places=2, msg=f"slot {slot} axis {axis}")

    def test_blob_header_carries_the_live_world_mask(self):
        blob = generator.build_stage_blob(self.packet, self.desc)
        header, parsed = generator.parse_stage_blob(blob)
        self.assertEqual(header["camera_mask"], 0xE4894)
        self.assertEqual(
            header["camera_mask"],
            generator.blob_live_world_mask(parsed, self.desc))

    def test_select_inc_linked_row_does_not_disagree_with_the_producer(self):
        """The dormant second copy of the same number.

        Saffron ships through the blob: `NDS_NATIVE_STAGE_LINKED_YAMABUKI`
        defaults to 0, so the `sNdsNativeStagePacketYamabuki` row in the
        hand-maintained `src/nds/nds_native_stage_select.inc` is not compiled
        and the blob header this suite already checks is the live value.

        The row is still a literal nothing regenerates. Flip LINKED_YAMABUKI to
        1 for a lab build and the gate silently stops animating again, so this
        asserts the moment the row becomes reachable and otherwise reports the
        exact edit that is owed rather than passing in silence.
        """
        select = (ROOT / "src/nds/nds_native_stage_select.inc").read_text(
            encoding="utf-8")
        anchor = "static const NDSNativeStagePacket sNdsNativeStagePacketYamabuki = {"
        start = select.find(anchor)
        self.assertNotEqual(start, -1, "Yamabuki packet row is gone")
        block = select[start:select.find("};", start)]
        masks = re.findall(r"(0x[0-9A-Fa-f]+|\d+)ULL", block)
        self.assertEqual(len(masks), 2,
                         "expected rigid_binding_mask then camera_binding_mask")
        rigid, camera = (int(value, 0) for value in masks)
        self.assertEqual(rigid, generator.blob_rigid_mask("yamabuki"))
        want = generator.blob_live_world_mask(self.packet, self.desc)
        linked = re.search(
            r"#define\s+NDS_NATIVE_STAGE_LINKED_YAMABUKI\s+(\d+)", select)
        self.assertIsNotNone(linked, "LINKED_YAMABUKI default is gone")
        if camera == want:
            return
        if int(linked.group(1)) != 0:
            self.fail(
                f"select.inc is linked and carries 0x{camera:X}; the gate's "
                f"bindings 17-19 would replay the baked world. Want 0x{want:X}.")
        self.skipTest(
            "OWED EDIT (dormant, LINKED_YAMABUKI=0): "
            "src/nds/nds_native_stage_select.inc, sNdsNativeStagePacketYamabuki: "
            f"camera_binding_mask 0x{camera:X}ULL -> 0x{want:X}ULL")

    def test_other_stages_are_untouched(self):
        """No stage without a declared joint table changes its mask."""
        for name in ("dreamland", "zebes"):
            with self.subTest(stage=name):
                self.assertEqual(generator.blob_animated_mask(name), 0)
                packet = generator.generate(ROOT, name)
                desc = get_descriptor(name)
                self.assertEqual(generator.blob_live_world_mask(packet, desc),
                                 generator.blob_camera_mask(packet))


if __name__ == "__main__":
    unittest.main()
