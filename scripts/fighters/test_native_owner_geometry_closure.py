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


if __name__ == "__main__":
    unittest.main()
