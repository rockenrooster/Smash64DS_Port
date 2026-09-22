"""Host tests for the ground Thunder Jolt's soft-coverage conversion.

The owner's row is "neutral b attack VFX that walks terrain still has hard
edges".  The source edge is NOT a palette and NOT a filter: the six ground
roots carry no G_LOADTLUT at all, their render tile is IA8 32x32 clamped, and
the baked combiner is
`rgb = (PRIMITIVE - ENVIRONMENT) * TEXEL0 + ENVIRONMENT, alpha = TEXEL0_A`.
So the low nibble of every texel IS the coverage, and the shipping upload --
RGB5A1 packed into GL_RGB16 -- keeps one bit of it.

These tests decode the real images out of the SHA-pinned asset, run the exact
byte the runtime writes, and assert the produced DS texel alphas are a
sixteen-level monotone ramp.  The guard that matters is
`test_one_bit_alpha_fails_this_test`: it feeds the SAME assertions the
shipping 1-bit rule and requires them to fail, so a conversion that quietly
regressed to hard alpha turns this file RED rather than green.

Run:  python -m pytest scripts/stages/test_pikachu_thunderground_coverage.py -q
  or: python scripts/stages/test_pikachu_thunderground_coverage.py
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

_p = Path(__file__).resolve().parent
while _p.name != "scripts":
    _p = _p.parent
sys.path.insert(0, str(_p))
import _paths  # noqa: F401,E402

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_nds_native_stage as sm  # noqa: E402
import generate_nds_native_pikachu_thunderground as gg  # noqa: E402

REPO = Path(__file__).resolve().parents[2]
EXEC_INC = REPO / "src/nds/nds_native_pikachu_thunderground.exec.inc"
COVERAGE_INC = REPO / "src/nds/nds_native_pikachu_thunderground_coverage.inc"
COVERAGE_H = REPO / "include/nds/nds_native_pikachu_thunderground_coverage.h"


def ds_texel(value: int, alpha5, index3) -> int:
    """The byte ndsNativeThunderGroundCoverageFill writes, in Python."""
    return ((alpha5[value & 0x0F] << 3) | index3[(value >> 4) & 0x0F]) & 0xFF


class GroundJoltCoverageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        model = sm.load_o2r(REPO, gg.MODEL_FILE)
        cls.coverage = gg.decode_coverage(model)
        cls.palette, cls.alpha5, cls.index3 = gg.coverage_tables()
        cls.images = {}
        for offset in cls.coverage["images"]:
            cls.images[offset] = model.payload[offset:offset + gg.IMAGE_BYTES]

    # -- the source contract ------------------------------------------------

    def test_source_is_ia8_with_a_real_alpha_ramp(self):
        self.assertEqual(len(self.coverage["images"]), 3)
        for offset, data in self.images.items():
            self.assertEqual(len(data), gg.IMAGE_BYTES, hex(offset))
        # Every one of the sixteen alpha levels is populated; the edge is a
        # ramp, not a mask.
        self.assertEqual(len([n for n in self.coverage["alpha"] if n > 0]), 16)
        self.assertEqual(len([n for n in self.coverage["intensity"] if n > 0]), 16)
        # ...and most texels sit strictly between transparent and opaque, which
        # is exactly the population a 1-bit upload destroys.
        self.assertEqual(self.coverage["total"], 3 * gg.IMAGE_BYTES)
        self.assertGreater(self.coverage["partial"], self.coverage["total"] // 2)

    def test_intensity_and_alpha_are_independent_nibbles(self):
        """An I texture's alpha IS its intensity; this one's is not.

        This is why the shared converter's graded-coverage arm, which is gated
        on NDS_RENDERER_HW_TEXTURE_FMT_I16, cannot serve this owner.
        """
        differing = 0
        for data in self.images.values():
            differing += sum(1 for b in data if (b >> 4) != (b & 0x0F))
        self.assertGreater(differing, self.coverage["total"] // 2)

    # -- the conversion -----------------------------------------------------

    def test_alpha5_lut_is_a_monotone_sixteen_level_ramp(self):
        self.assertEqual(len(self.alpha5), 16)
        self.assertEqual(self.alpha5[0], 0)
        self.assertEqual(self.alpha5[15], 31)
        self.assertEqual(len(set(self.alpha5)), 16, "injective into 0..31")
        for n in range(15):
            self.assertLess(self.alpha5[n], self.alpha5[n + 1])
        # (n * 0x11) >> 3 is round(n * 31 / 15) at all sixteen inputs.
        for n in range(16):
            self.assertEqual(self.alpha5[n], round(n * 31 / 15))

    def test_produced_texel_alphas_form_the_expected_ramp(self):
        seen = {}
        for data in self.images.values():
            for value in data:
                produced = ds_texel(value, self.alpha5, self.index3)
                seen.setdefault(value & 0x0F, set()).add(produced >> 3)
        self.assertEqual(sorted(seen), list(range(16)),
                         "every source coverage level reached the upload")
        for nibble, produced in seen.items():
            self.assertEqual(produced, {self.alpha5[nibble]},
                             f"coverage {nibble} did not map to one alpha")
        levels = sorted(next(iter(v)) for v in seen.values())
        self.assertEqual(levels, sorted(self.alpha5))
        self.assertGreater(len(set(levels)), 2,
                           "a two-level result is 1-bit alpha, not a ramp")

    def test_one_bit_alpha_fails_this_test(self):
        """The RED guard: the shipping rule must NOT satisfy the assertions.

        ndsRendererHardwareConvertIA sets the single RGB5A1 alpha bit for any
        non-zero nibble, so re-deriving the ramp from that result yields two
        levels.  If a future change routes this owner back through it, the
        assertion above stops holding -- and this test proves the assertion is
        strong enough to notice.
        """
        hard = sorted({31 if (n != 0) else 0 for n in range(16)})
        self.assertEqual(hard, [0, 31])
        with self.assertRaises(AssertionError):
            self.assertGreater(len(hard), 2)
        # ...and the partial-coverage texels it would destroy are the majority.
        forced = sum(self.coverage["alpha"][1:-1])
        self.assertEqual(forced, self.coverage["partial"])
        self.assertGreater(forced, 2000)

    def test_palette_is_the_shared_blendpe_lerp_and_is_monotone(self):
        self.assertEqual(len(self.palette), gg.COVERAGE_PALETTE_ENTRIES)
        reds = [entry[0] for entry in self.palette]
        greens = [entry[1] for entry in self.palette]
        for n in range(len(self.palette) - 1):
            self.assertLess(reds[n], reds[n + 1])
            self.assertLess(greens[n], greens[n + 1])
        # ENV is (0, 89, 255) and PRIM is white, so blue is pinned at the top
        # of the RGB5 range across the whole ramp.
        for entry in self.palette:
            self.assertEqual(entry[2], 31)
        # Endpoints agree with the shared bake at the extreme weights.
        self.assertEqual(self.palette[0],
                         gg.blend_prim_env_texel0((1 * 0x11) >> 4))
        self.assertEqual(self.palette[-1],
                         gg.blend_prim_env_texel0((29 * 0x11) >> 4))

    def test_index3_covers_the_palette_monotonically(self):
        self.assertEqual(len(self.index3), 16)
        self.assertEqual(self.index3[0], 0)
        self.assertEqual(self.index3[15], gg.COVERAGE_PALETTE_ENTRIES - 1)
        for n in range(15):
            self.assertLessEqual(self.index3[n], self.index3[n + 1])
        self.assertEqual(len(set(self.index3)), gg.COVERAGE_PALETTE_ENTRIES)

    def test_every_produced_byte_fits_the_a5i3_layout(self):
        for data in self.images.values():
            for value in data:
                produced = ds_texel(value, self.alpha5, self.index3)
                self.assertLessEqual(produced, 0xFF)
                self.assertEqual(produced >> 3, self.alpha5[value & 0x0F])
                self.assertEqual(produced & 0x07,
                                 self.index3[(value >> 4) & 0x0F])

    # -- the wiring ---------------------------------------------------------

    def test_generated_tables_and_runtime_agree(self):
        decoded, coverage = gg.decode(run_census=False)
        packet = gg.render(decoded, coverage)
        header = gg.render_header(decoded, coverage)
        self.assertIn("sNdsNativeThunderGroundCoveragePalette[8]", packet)
        self.assertIn("sNdsNativeThunderGroundCoverageAlpha5[16]", packet)
        self.assertIn("sNdsNativeThunderGroundCoverageIndex3[16]", packet)
        self.assertIn("NDS_NATIVE_THUNDERGROUND_IMAGE_COUNT 3u", header)
        self.assertIn("NDS_NATIVE_THUNDERGROUND_COVERAGE_PALETTE_ENTRIES 8u",
                      header)
        self.assertIn(
            f"NDS_NATIVE_THUNDERGROUND_COVERAGE_PARTIAL_TEXELS "
            f"{coverage['partial']}u", header)
        for name in ("sNdsNativeThunderGroundCoverageAlpha5",
                     "sNdsNativeThunderGroundCoverageIndex3",
                     "sNdsNativeThunderGroundCoveragePalette"):
            self.assertIn(name, COVERAGE_INC.read_text(encoding="utf-8"))

    def test_runtime_uploads_five_alpha_bits_not_one(self):
        body = COVERAGE_INC.read_text(encoding="utf-8")
        # A5I3 through the existing helper, never the packed-nibble PAL16 one.
        self.assertIn("ndsRendererHardwarePrepareIFCommonCloudAtlas", body)
        self.assertNotIn("PrepareIFCommonPal16Atlas", body)
        # alpha5 in bits 3-7, palette index in bits 0-2.
        self.assertIn("<< 3", body)
        # The word-swapped O2R payload must go through the shared lane reader.
        self.assertIn("ndsRendererReadTextureByte", body)
        # The live selector is the material's own image, not a stats snapshot.
        self.assertIn("material->current_image", body)
        # The cache key carries asset generation, coverage class and extent.
        self.assertIn("gNdsTaskmanHeapGeneration", body)
        self.assertIn("NDS_NATIVE_THUNDERGROUND_COVERAGE_CLASS", body)
        self.assertIn("gNdsRendererSceneTextureVramResetCount", body)

    def test_exec_tries_coverage_first_and_keeps_the_zero_alpha_skip(self):
        body = EXEC_INC.read_text(encoding="utf-8")
        self.assertIn("nds_native_pikachu_thunderground_coverage.inc", body)
        soft = body.index("ndsNativeThunderGroundBindCoverageTexture")
        generic = body.index("ndsRendererHardwareBindTexture(stats, config")
        self.assertLess(soft, generic, "soft coverage must be tried first")
        self.assertIn("gNdsThunderGroundCoverageFallbackCount", body)
        # POLY_ALPHA 0 is wireframe on DS, so the skip has to stay.
        self.assertIn("if (poly_alpha != 0u)", body)
        # Six one-triangle segments, no quad substitution.
        self.assertIn("NDS_NATIVE_THUNDERGROUND_CORNER_COUNT", body)
        self.assertNotIn("Quad", body)

    def test_coverage_header_documents_the_measured_choice(self):
        body = COVERAGE_H.read_text(encoding="utf-8")
        self.assertIn("GL_RGB8_A5", body)
        self.assertIn("NDS_NATIVE_THUNDERGROUND_COVERAGE_SLOTS", body)
        self.assertIn("gNdsThunderGroundCoverageFallbackCount", body)

    def test_air_and_effect_siblings_are_untouched(self):
        """This row owns the GROUND path only; the other two are regression
        coverage and must keep their own CI4 quad helper."""
        for name in ("generate_nds_native_pikachu_thunderjolt.py",
                     "generate_nds_native_pikachu_thunderjolt_effect.py"):
            text = (REPO / "scripts/stages" / name).read_text(encoding="utf-8")
            self.assertNotIn("CoveragePalette", text)
            self.assertNotIn("GL_RGB8_A5", text)


if __name__ == "__main__":
    unittest.main()
