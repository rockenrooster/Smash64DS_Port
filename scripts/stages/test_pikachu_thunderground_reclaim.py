#!/usr/bin/env python3
"""The ground Thunder Jolt coverage owner must stay RECLAIMABLE.

P01 gave the ground jolt a soft A5I3 edge by moving its three images out of the
generic texture cache and onto three dedicated GL names.  That was right for the
ground jolt and wrong for its siblings: `ndsRendererHardwareEvictTexture` walks
`sNdsRendererHardwareTextureCache[]` only, so the moment those images stopped
being cache entries they stopped being reclaimable, and
`ndsRendererHardwarePrepareIFCommonAtlas` has no other way to make room when
`glTexImage2D` refuses.  The byte total went down; the RECLAIMABLE total went to
zero, and that is the number a starved sibling spends.  The air Thunder Jolt --
same asset 342, a 64x64 A3I5 upload through the shared CI4 quad helper, the
largest single request in the family -- regressed in r24 for exactly that.

These tests are structural, not numeric: they pin the shape that keeps the
accepted ground repair AND gives the bytes back on demand.  Nothing here builds,
runs or writes.
"""

import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
COVERAGE_INC = REPO / "src/nds/nds_native_pikachu_thunderground_coverage.inc"
COVERAGE_H = REPO / "include/nds/nds_native_pikachu_thunderground_coverage.h"
EXEC_INC = REPO / "src/nds/nds_native_pikachu_thunderground.exec.inc"
AIR_EXEC_INC = REPO / "src/nds/nds_native_pikachu_thunderjolt.exec.inc"
FX_EXEC_INC = REPO / "src/nds/nds_native_pikachu_thunderjolt_effect.exec.inc"
QUAD_INC = REPO / "src/nds/nds_native_textured_quad.exec.inc"

RECLAIM = "ndsNativeThunderGroundReleaseOneCoverageTexture"


def read(path):
    return path.read_text(encoding="utf-8")


def function_body(text, signature):
    """The one function, not the rest of the file: a slice that ran to EOF
    would let any later line satisfy or break an assertion about this one."""
    start = text.index(signature)
    end = text.index("\n}\n", start)
    return text[start:end]


class ReclaimContract(unittest.TestCase):

    def test_the_owner_can_hand_one_name_back(self):
        body = read(COVERAGE_INC)
        self.assertIn(f"s32 {RECLAIM}(void)", body,
                      "the on-demand reclaim entry point is the whole repair")
        self.assertIn(f"s32 {RECLAIM}(void);", read(COVERAGE_H),
                      "a shared-file caller needs the prototype")

    def test_reclaim_refuses_a_name_bound_this_frame(self):
        """Same rule ndsRendererHardwareEvictTexture applies to its own
        entries: the segment that bound it has not drawn yet."""
        fn = function_body(read(COVERAGE_INC), f"s32 {RECLAIM}(void)")
        self.assertIn("u32 now = sNdsRendererHardwareFrameSerial + 1u;", fn,
                      "'now' must be the same stamp the bind paths write")
        self.assertIn("slot->frame == now", fn,
                      "the current-frame guard IS the repair's safety rule")
        self.assertIn("slot->name == 0u", fn,
                      "an empty slot -- including the one a re-entrant prepare "
                      "is currently filling -- must be skipped")

    def test_reclaim_keeps_the_acceptance_signal(self):
        """gNdsThunderGroundCoverageImageMask records which images were ever
        REACHED this scene.  Clearing it on a reclaim would destroy the only
        evidence that all three got a soft edge."""
        fn = function_body(read(COVERAGE_INC), f"s32 {RECLAIM}(void)")
        self.assertNotIn("gNdsThunderGroundCoverageImageMask", fn)
        # ...and the bind path must still SET it, or the signal never exists.
        self.assertIn("gNdsThunderGroundCoverageImageMask |= 1u << index;",
                      read(COVERAGE_INC))

    def test_residency_is_reported_beside_the_bytes(self):
        body = read(COVERAGE_INC)
        self.assertIn("gNdsThunderGroundCoverageResidentSlots", body)
        self.assertIn("gNdsThunderGroundCoverageReclaimCount", body)
        for name in ("gNdsThunderGroundCoverageResidentSlots",
                     "gNdsThunderGroundCoverageReclaimCount"):
            self.assertIn(f"extern volatile u32 {name};", read(COVERAGE_H))

    def test_one_writer_for_the_vram_figure(self):
        """Every add and every drop must republish the live sum through the
        same helper; a second hand-rolled loop is how the two numbers drift."""
        body = read(COVERAGE_INC)
        writes = [line for line in body.splitlines()
                  if "gNdsThunderGroundCoverageVramBytes" in line
                  and "=" in line and "extern" not in line]
        self.assertEqual(
            len(writes), 1,
            "gNdsThunderGroundCoverageVramBytes must be assigned only inside "
            f"ndsNativeThunderGroundCoverageAccountVram; found: {writes}")
        self.assertIn("ndsNativeThunderGroundCoverageAccountVram", body)

    def test_every_slot_teardown_clears_the_frame_stamp(self):
        """A stale stamp on an empty slot would make the wrap-safe 'older
        than' compare pick the wrong victim once the serial laps."""
        body = read(COVERAGE_INC)
        self.assertEqual(body.count(".key = 0u;") + body.count("->key = 0u;"),
                         body.count(".frame = 0u;") + body.count("->frame = 0u;"),
                         "key and frame must be cleared together")

    def test_the_accepted_ground_repair_is_untouched(self):
        body = read(COVERAGE_INC)
        self.assertIn("ndsRendererHardwarePrepareIFCommonCloudAtlas", body)
        self.assertIn("sNdsNativeThunderGroundCoverageAlpha5", body)
        self.assertIn("sNdsNativeThunderGroundCoverageIndex3", body)
        self.assertIn("sNdsNativeThunderGroundCoveragePalette", body)
        self.assertIn("ndsRendererReadTextureByte", body)
        self.assertIn("material->current_image", body)
        exec_body = read(EXEC_INC)
        soft = exec_body.index("ndsNativeThunderGroundBindCoverageTexture")
        generic = exec_body.index("ndsRendererHardwareBindTexture(stats, config")
        self.assertLess(soft, generic, "soft coverage must still be tried first")

    def test_the_air_and_effect_owners_were_not_edited(self):
        """This row owns the GROUND path.  The regression it repairs is a
        shared-resource one, so the fix must not reach into the siblings."""
        for path in (AIR_EXEC_INC, FX_EXEC_INC, QUAD_INC):
            text = read(path)
            self.assertNotIn(RECLAIM, text)
            self.assertNotIn("ThunderGroundCoverage", text)


if __name__ == "__main__":
    unittest.main()
