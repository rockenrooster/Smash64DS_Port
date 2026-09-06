"""Regression controls for Yoshi's source pre/post display-list pairs."""
import contextlib
import io
import struct
import unittest
from unittest.mock import patch

import check_native_owner_geometry_closure as closure


class YoshiSourceClosureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.programs = {
            detail: closure.owner_program("yoshi", detail)
            for detail in closure.DETAILS
        }
        cls.payload = closure.native.load_o2r_payload(closure.REPO, "yoshi")

    def failures(self, detail, program):
        with contextlib.redirect_stdout(io.StringIO()):
            return closure.source_closure("yoshi", detail, program)

    def test_original_pairs_pass_both_details(self):
        for detail, program in self.programs.items():
            with self.subTest(detail=detail):
                self.assertEqual(self.failures(detail, program), [])

    def test_wrong_canonical_stream_is_rejected(self):
        for detail, program in self.programs.items():
            with self.subTest(detail=detail):
                changed = dict(program, roots=list(program["roots"]))
                # Joint 2 uses a pre/post pair in both source JointTrees.
                # Substituting joint 1's valid DL must still fail identity.
                root = changed["roots"][1]
                changed["roots"][1] = (changed["roots"][0][0], *root[1:])
                self.assertTrue(any("canonical root 1" in failure
                                    for failure in self.failures(detail, changed)))

    def test_missing_parent_load_is_rejected_even_when_triangles_match(self):
        for detail, program in self.programs.items():
            with self.subTest(detail=detail):
                offset = program["roots"][1][0]
                commands = closure.root_commands(self.payload, offset)
                first_load = next(i for i, (w0, _) in enumerate(commands)
                                  if w0 >> 24 == closure.G_VTX)
                changed = bytearray(self.payload)
                # Replace the parent-bound load with a harmless pipe sync.
                struct.pack_into(">II", changed, offset + first_load * 8,
                                 0xE7000000, 0)
                self.assertEqual(
                    closure.walk_root_triangles(self.payload, offset),
                    closure.walk_root_triangles(changed, offset))
                with patch.object(closure.native, "load_o2r_payload",
                                  return_value=bytes(changed)):
                    self.assertTrue(any("source pre/post" in failure
                                        for failure in self.failures(detail, program)))


class BossSourceClosureTests(unittest.TestCase):
    def test_new_pre_only_state_cannot_be_discarded(self):
        native = closure.native
        payload = native.load_o2r_payload(closure.REPO, "boss")
        original = native._source_commands

        def changed(data, owner, offset):
            commands = original(data, owner, offset)
            if offset == 0x20f8:
                commands = [(0xe3, 0xe3001001, 0x8000), *commands[1:]]
            return commands

        with patch.object(native, "_source_commands", side_effect=changed):
            with self.assertRaisesRegex(ValueError, "lacks consumer replay"):
                native._pair_welded_commands(payload, "boss", 0x1568, 0x20f8)

    def test_original_empty_and_pre_only_pairs_pass(self):
        for detail in closure.DETAILS:
            with self.subTest(detail=detail), contextlib.redirect_stdout(io.StringIO()):
                program = closure.owner_program("boss", detail)
                self.assertEqual(closure.source_closure("boss", detail, program), [])

    def test_pre_only_parent_load_cannot_disappear_from_child(self):
        program = closure.owner_program("boss", "high")
        payload = closure.native.load_o2r_payload(closure.REPO, "boss")
        # Original joint8 has only pre0x20f8; joint9's post0x1568 consumes it.
        offset = program["roots"][5][0]
        commands = closure.root_commands(payload, offset)
        first_load = next(i for i, (w0, _) in enumerate(commands)
                          if w0 >> 24 == closure.G_VTX)
        changed = bytearray(payload)
        struct.pack_into(">II", changed, offset + first_load * 8, 0xE7000000, 0)
        self.assertEqual(closure.walk_root_triangles(payload, offset),
                         closure.walk_root_triangles(changed, offset))
        with patch.object(closure.native, "load_o2r_payload", return_value=bytes(changed)), \
                contextlib.redirect_stdout(io.StringIO()):
            failures = closure.source_closure("boss", "high", program)
        self.assertTrue(any("canonical root 5" in failure for failure in failures))

    def test_pre_only_loads_cannot_escape_their_subtree(self):
        with self.assertRaises(ValueError):
            closure.selected_source_roots([(2, (0x20, None)), (2, 0x40)], [0, 1])

    def test_unlit_colors_are_not_treated_as_normals(self):
        program = closure.owner_program("boss", "high")
        payload = closure.native.load_o2r_payload(closure.REPO, "boss")
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(closure.facing_closure("boss", "high", program), [])
        offset = program["roots"][0][0]
        changed = bytearray(payload)
        found = False
        for i, (w0, w1) in enumerate(closure.root_commands(payload, offset)):
            if w0 >> 24 == 0xD9 and not (w0 | w1) & 0x20000:
                struct.pack_into(">II", changed, offset + i * 8, w0 | 0x20000, w1)
                found = True
        self.assertTrue(found)
        with patch.object(closure.native, "load_o2r_payload", return_value=bytes(changed)), \
                contextlib.redirect_stdout(io.StringIO()):
            self.assertTrue(closure.facing_closure("boss", "high", program))


if __name__ == "__main__":
    unittest.main()
