"""Synthetic semantic tests; not visual/GPU or full N64 material validation."""
from dataclasses import replace
from pathlib import Path
import random
import struct
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from texture_alpha import decode_n64, encode_ds, decode_ds, NativeTexture, validate_texture_alpha_draw


class TextureAlphaTests(unittest.TestCase):
    def test_exhaustive_rgba5551_bit_repacking(self):
        source = b"".join(struct.pack(">H", n) for n in range(65536))
        decoded = decode_n64(source, "RGBA16", 65536)
        native = encode_ds(decoded, "GL_RGBA")
        actual = struct.unpack("<65536H", native.texels)
        expected = tuple(((n >> 11) & 31) | (((n >> 6) & 31) << 5) |
                         (((n >> 1) & 31) << 10) | ((n & 1) << 15) for n in range(65536))
        self.assertEqual(actual, expected)
        self.assertEqual(decode_ds(native), decoded)

    def test_ci4_nonzero_transparency_fixture_60_68(self):
        # Original synthetic 16x8 stream, not a game texture: 60 opaque black,
        # 68 transparent red, transparency in TLUT entry 15 rather than zero.
        indices = [0] * 60 + [15] * 68
        raw = bytes((indices[i] << 4) | indices[i+1] for i in range(0, 128, 2))
        tlut = struct.pack(">16H", *([1] + [0xffff]*14 + [0xf800]))
        rgba = decode_n64(raw, "CI4", 128, tlut=tlut, tlut_format="RGBA16")
        for fmt in ("GL_RGB4", "GL_RGB16", "GL_RGB256", "GL_RGBA"):
            with self.subTest(fmt=fmt):
                native = encode_ds(rgba, fmt)
                output = decode_ds(native)
                self.assertEqual(sum(p[3] == 0 for p in output), 68)
                self.assertEqual(sum(p[3] == 255 for p in output), 60)
                self.assertEqual(output[0], (0, 0, 0, 255))
                self.assertEqual([p[3] for p in output], [p[3] for p in rgba])

    def test_multiple_transparent_entries_and_equal_rgb(self):
        palette = [1] * 16
        palette[3] = 0
        palette[15] = 0xf800
        rgba = decode_n64(bytes([0x03, 0xf0]), "CI4", 4,
                          tlut=struct.pack(">16H", *palette), tlut_format="RGBA16")
        native = encode_ds(rgba, "GL_RGB16")
        self.assertEqual(native.texels, b"\x01\x10")
        self.assertTrue(native.color0_transparent)
        self.assertEqual([p[3] for p in decode_ds(native)], [255, 0, 0, 255])

    def test_ci8_ia16_tlut(self):
        palette = bytearray(512)
        palette[0:2] = bytes([255, 255])
        palette[510:512] = bytes([128, 17])
        rgba = decode_n64(bytes([0, 255]), "CI8", 2, tlut=bytes(palette), tlut_format="IA16")
        self.assertEqual(rgba, ((255, 255, 255, 255), (128, 128, 128, 17)))

    def test_ia_and_intensity_channels(self):
        cases = [("IA4", b"\xe1", ((255,255,255,0), (0,0,0,255))),
                 ("IA8", b"\xf0\x1f", ((255,255,255,0), (17,17,17,255))),
                 ("IA16", b"\xff\x00\x20\x80", ((255,255,255,0), (32,32,32,128))),
                 ("I4", b"\x0f", ((0,0,0,0), (255,255,255,255))),
                 ("I8", b"\x00\x80", ((0,0,0,0), (128,128,128,128)))]
        for fmt, data, wanted in cases:
            with self.subTest(fmt=fmt): self.assertEqual(decode_n64(data, fmt, 2), wanted)

    def test_rgba32_keeps_explicit_alpha(self):
        self.assertEqual(decode_n64(bytes([5,6,7,128]), "RGBA32", 1), ((5,6,7,128),))

    def test_native_nibble_order_and_opaque_index_zero(self):
        native = encode_ds([(0,0,0,255), (255,0,0,255)], "GL_RGB16")
        self.assertEqual(native.texels, b"\x10")
        self.assertFalse(native.color0_transparent)
        self.assertEqual(decode_ds(native)[0][3], 255)

    def test_palette_capacity_accounts_for_reserved_zero(self):
        colors = [(i*8,0,0,255) for i in range(16)]
        hole = [(0,0,0,0)]
        self.assertEqual(len(encode_ds(colors[:15]+hole, "GL_RGB16").palette), 16)
        with self.assertRaises(ValueError): encode_ds(colors+hole, "GL_RGB16")
        self.assertEqual(len(encode_ds(colors, "GL_RGB16").palette), 16)
        with self.assertRaises(ValueError): encode_ds(colors[:4]+hole, "GL_RGB4")

    def test_graded_is_not_silently_thresholded(self):
        for fmt in ("GL_RGB4", "GL_RGB16", "GL_RGB256", "GL_RGBA", "GL_RGB"):
            with self.subTest(fmt=fmt), self.assertRaises(ValueError):
                encode_ds([(0,0,0,128)], fmt, allow_alpha_quantization=True)

    def test_rgb_upload_cannot_preserve_cutout(self):
        with self.assertRaises(ValueError): encode_ds([(255,0,0,0)], "GL_RGB")
        forced = NativeTexture("GL_RGB", 1, b"\x1f\x00", (), False)
        self.assertEqual(decode_ds(forced)[0], (255,0,0,255))

    def test_alpha_formats_can_use_opaque_color_zero(self):
        for fmt in ("GL_RGB32_A3", "GL_RGB8_A5"):
            native = encode_ds([(0,0,0,255), (0,0,0,0)], fmt)
            self.assertFalse(native.color0_transparent)
            self.assertEqual(decode_ds(native), ((0,0,0,255), (0,0,0,0)))
            self.assertEqual(native.texels[0] & (31 if fmt == "GL_RGB32_A3" else 7), 0)

    def test_graded_quantization_requires_permission_and_bounds(self):
        for fmt, limit in (("GL_RGB32_A3", 20), ("GL_RGB8_A5", 4)):
            with self.subTest(fmt=fmt):
                with self.assertRaises(ValueError): encode_ds([(255,255,255,128)], fmt)
                rgba = [(255,255,255,a) for a in range(256)]
                out = decode_ds(encode_ds(rgba, fmt, allow_alpha_quantization=True))
                self.assertLessEqual(max(abs(a-p[3]) for a,p in enumerate(out)), limit)
                self.assertEqual(out[0][3], 0)
                self.assertEqual(out[-1][3], 255)
                self.assertEqual([p[3] for p in out], sorted(p[3] for p in out))

    def test_a3_hardware_alpha_expansion(self):
        native = NativeTexture("GL_RGB32_A3", 8, bytes(i << 5 for i in range(8)), (0,)*32, False)
        expected5 = [0,4,9,13,18,22,27,31]
        self.assertEqual([p[3] for p in decode_ds(native)], [(x << 3) | (x >> 2) for x in expected5])

    def test_odd_count_and_padding(self):
        native = encode_ds([(0,0,0,255)]*3, "GL_RGB16")
        self.assertEqual(len(native.texels), 2)
        self.assertEqual(len(decode_ds(native)), 3)
        self.assertEqual(len(decode_n64(b"\xff\xf0", "I4", 3)), 3)

    def test_random_binary_masks(self):
        rng = random.Random(20260911)
        colors = [(0,0,0), (255,0,0), (0,255,0)]
        for size in (1,3,8,63,128,1024):
            rgba = [(*rng.choice(colors), rng.choice((0,255))) for _ in range(size)]
            for fmt in ("GL_RGB4", "GL_RGB16", "GL_RGB256", "GL_RGBA", "GL_RGB32_A3", "GL_RGB8_A5"):
                with self.subTest(size=size, fmt=fmt):
                    output = decode_ds(encode_ds(rgba, fmt))
                    self.assertEqual([p[3] for p in output], [p[3] for p in rgba])
                    for src,dst in zip(rgba,output):
                        if src[3]: self.assertEqual(src,dst)

    def test_palette_alpha_animation_requires_texel_rebuild(self):
        a = encode_ds([(255,0,0,255), (0,0,0,0)], "GL_RGB16")
        b = encode_ds([(255,0,0,0), (0,0,0,255)], "GL_RGB16")
        self.assertNotEqual(a.texels,b.texels)
        self.assertEqual([p[3] for p in decode_ds(b)], [0,255])

    def test_reject_incomplete_ambiguous_and_invalid_inputs(self):
        invalid = [(b"", "I8", 1), (b"\0", "I8", 0), (b"\0", "I8", True),
                   (b"\0", "RGBA16", 1), (b"\0", "unknown", 1)]
        for args in invalid:
            with self.subTest(args=args), self.assertRaises(ValueError): decode_n64(*args)
        with self.assertRaises(ValueError): decode_n64(b"\0", "CI4", 2, tlut=b"\0"*32)
        with self.assertRaises(ValueError): decode_n64(b"\0", "I8", 1, tlut=b"\0"*32)
        with self.assertRaises(ValueError): encode_ds([], "GL_RGBA")
        with self.assertRaises(ValueError): encode_ds([(0,0,0,True)], "GL_RGBA")
        with self.assertRaises(ValueError): NativeTexture("GL_RGB16",1,b"\0",(0x8000,)*16,True)
        with self.assertRaises(ValueError): NativeTexture("GL_RGBA",1,b"\0",(),False)
        with self.assertRaises(ValueError): NativeTexture("GL_RGB32_A3",1,b"\0",(0,)*32,True)


class DrawContractTests(unittest.TestCase):
    def setUp(self):
        self.state = dict(color0_transparent=True, polygon_mode="modulation", polygon_alpha=31,
                          blend_enabled=False, texture_enabled=True, alpha_test_threshold=0)

    def test_valid_binary_recipe(self):
        validate_texture_alpha_draw("GL_RGB16", "cutout", **self.state)
        self.state["color0_transparent"] = False
        validate_texture_alpha_draw("GL_RGBA", "cutout", **self.state)

    def test_reject_lost_mask_decal_wireframe_and_global_conflicts(self):
        changes = [{"color0_transparent": False}, {"polygon_mode":"decal"}, {"polygon_alpha":0},
                   {"texture_enabled":False}, {"alpha_test_threshold":31}, {"alpha_test_threshold":1},
                   {"polygon_alpha":15}]
        for change in changes:
            with self.subTest(change=change), self.assertRaises(ValueError):
                validate_texture_alpha_draw("GL_RGB16", "cutout", **(self.state | change))

    def test_graded_requires_suitable_format_and_blend(self):
        self.state.update(color0_transparent=False, blend_enabled=True)
        validate_texture_alpha_draw("GL_RGB8_A5", "graded", **self.state)
        for fmt in ("GL_RGBA", "GL_RGB16", "GL_RGB"):
            with self.subTest(fmt=fmt), self.assertRaises(ValueError):
                validate_texture_alpha_draw(fmt, "graded", **self.state)
        self.state["blend_enabled"] = False
        with self.assertRaises(ValueError): validate_texture_alpha_draw("GL_RGB32_A3", "graded", **self.state)

    def test_upload_type_and_state_flags(self):
        self.state["color0_transparent"] = False
        with self.assertRaises(ValueError): validate_texture_alpha_draw("GL_RGB", "cutout", **self.state)
        self.state["blend_enabled"] = 1
        with self.assertRaises(ValueError): validate_texture_alpha_draw("GL_RGBA", "cutout", **self.state)

if __name__ == "__main__": unittest.main()
