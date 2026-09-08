"""Host tests for the DS-native wallpaper conversion.

Runs the REAL all-nine-plus-Results conversion into a temp dir (no ROM, no
emulator) and checks: every payload present with exact byte size, the C
header schema against the files on disk, native dimensions at source*4/5,
pixel-center sample correspondence (including strip boundaries and corners),
battle opacity (source alpha rejected, bit 15 set on every halfword), and
Results intensity identity (output byte == source nibble * 17, black kept 0).

Run:  python -m pytest scripts/stages/test_native_wallpapers.py -q
  or: python scripts/stages/test_native_wallpapers.py
"""

from __future__ import annotations

import re
import struct
import sys
import tempfile
import unittest
from pathlib import Path

_p = Path(__file__).resolve().parent
while _p.name != "scripts":
    _p = _p.parent
sys.path.insert(0, str(_p))
import _paths  # noqa: F401,E402

import generate_native_wallpapers as gen  # noqa: E402
from generate_mn_ui_kit import RelocFile, decode_sprite_raster  # noqa: E402

BATTLE_BYTES = 240 * 176 * 2
RESULTS_BYTES = 240 * 176


class NativeWallpaperTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.repo = Path(__file__).resolve().parents[2]
        cls.tmp = tempfile.TemporaryDirectory(prefix="smash64ds-wallpapers-")
        cls.out = Path(cls.tmp.name)
        cls.header = cls.out / "native_wallpapers.generated.inc"
        cls.assets = gen.generate(cls.repo, cls.out, cls.header)
        cls.by_key = {asset.source.key: asset for asset in cls.assets}
        cls.rasters: dict[str, list] = {}
        for asset in cls.assets:
            path = (cls.repo / "decomp" / "BattleShip-main" /
                    "BattleShip_o2r" / asset.source.o2r)
            _sprite, raster = decode_sprite_raster(
                RelocFile(path), asset.source.key, asset.source.sprite_offset)
            cls.rasters[asset.source.key] = raster

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_all_nine_battle_plus_results_present_and_sized(self):
        keys = [source.key for source in gen.SOURCES]
        self.assertEqual(len(keys), 10, "nine VS stages plus Results")
        self.assertEqual(len([s for s in gen.SOURCES if s.battle]), 9,
                         "1P stays paused: exactly nine battle wallpapers")
        for key in keys:
            with self.subTest(stage=key):
                asset = self.by_key[key]
                data = (self.out / asset.filename).read_bytes()
                want = (BATTLE_BYTES if asset.source.battle
                        else RESULTS_BYTES)
                self.assertEqual(len(data), want,
                                 f"{key}: payload size")

    def test_header_schema_matches_files_on_disk(self):
        text = self.header.read_text(encoding="utf-8")
        rows = re.findall(
            r"\{\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+),\s*"
            r"(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*\"([^\"]+)\"\s*\}",
            text)
        self.assertEqual(len(rows), 10, "header holds one row per asset")
        for file_id, src_off, src_w, src_h, nat_w, nat_h, fmt, name in rows:
            with self.subTest(row=name):
                match = [a for a in self.assets if a.filename == name]
                self.assertEqual(len(match), 1, f"{name}: one asset")
                asset = match[0]
                self.assertEqual(int(file_id, 0), asset.source.file_id,
                                 f"{name}: file_id")
                container = RelocFile(self.repo / "decomp" / "BattleShip-main" /
                                      "BattleShip_o2r" / asset.source.o2r)
                bitmap_offset = container.sprite(asset.source.sprite_offset).bitmap
                self.assertNotEqual(bitmap_offset, asset.source.sprite_offset)
                self.assertEqual(int(src_off, 0), bitmap_offset,
                                 f"{name}: source bitmap payload offset")
                self.assertEqual((int(src_w), int(src_h)),
                                 (asset.src_w, asset.src_h),
                                 f"{name}: source dimensions")
                self.assertEqual((int(nat_w), int(nat_h)),
                                 (asset.native_w, asset.native_h),
                                 f"{name}: native dimensions")
                self.assertEqual(int(fmt), asset.format, f"{name}: format")
                payload = (self.out / name).read_bytes()
                expect = (int(nat_w) * int(nat_h) * 2 if int(fmt) == 0
                          else int(nat_w) * int(nat_h))
                self.assertEqual(len(payload), expect,
                                 f"{name}: payload matches header geometry")

    def test_native_dimensions_are_source_times_4_over_5(self):
        for asset in self.assets:
            with self.subTest(stage=asset.source.key):
                self.assertEqual((asset.src_w, asset.src_h), (300, 220))
                self.assertEqual(asset.native_w, asset.src_w * 4 // 5)
                self.assertEqual(asset.native_h, asset.src_h * 4 // 5)
                self.assertEqual((asset.native_w, asset.native_h), (240, 176))

    def test_sample_correspondence_corners_edges_and_interior(self):
        probes = [(0, 0), (239, 0), (0, 175), (239, 175),
                  (120, 88), (37, 151), (200, 40)]
        for asset in self.assets:
            raster = self.rasters[asset.source.key]
            data = (self.out / asset.filename).read_bytes()
            with self.subTest(stage=asset.source.key):
                for dx, dy in probes:
                    sx = gen.sample_index(dx, 300, 240)
                    sy = gen.sample_index(dy, 220, 176)
                    if asset.source.battle:
                        red, green, blue, _a = raster[sy][sx]
                        want = ((1 << 15) | ((blue >> 3) << 10) |
                                ((green >> 3) << 5) | (red >> 3))
                        (got,) = struct.unpack_from(
                            "<H", data, (dy * 240 + dx) * 2)
                        self.assertEqual(got, want, f"dest ({dx},{dy})")
                    else:
                        red, _g, _b, _a = raster[sy][sx]
                        self.assertEqual(data[dy * 240 + dx], red,
                                         f"dest ({dx},{dy})")
        # Strip boundaries: the dest edges sample the source edges exactly.
        self.assertEqual(gen.sample_index(0, 300, 240), 0)
        self.assertEqual(gen.sample_index(239, 300, 240), 299)
        self.assertEqual(gen.sample_index(0, 220, 176), 0)
        self.assertEqual(gen.sample_index(175, 220, 176), 219)

    def test_battle_alpha_rejected_and_every_halfword_opaque(self):
        for asset in self.assets:
            if not asset.source.battle:
                continue
            with self.subTest(stage=asset.source.key):
                raster = self.rasters[asset.source.key]
                for row in raster:
                    for _r, _g, _b, alpha in row:
                        self.assertEqual(alpha, 255, "source fully opaque")
                data = (self.out / asset.filename).read_bytes()
                for off in range(0, len(data), 2):
                    (half,) = struct.unpack_from("<H", data, off)
                    self.assertTrue(half & 0x8000, f"bit15 at {off:#x}")
        with self.assertRaises(gen.ConvertError):
            gen.rgba8_to_ds_opaque(255, 255, 255, 0, "probe")

    def test_results_intensity_identity_and_black_kept(self):
        asset = self.by_key["results"]
        raster = self.rasters["results"]
        data = (self.out / asset.filename).read_bytes()
        self.assertEqual(asset.format, gen.FORMAT_RESULTS_I8)
        seen_zero = seen_full = False
        for sy in range(220):
            for sx in range(300):
                red, green, blue, _a = raster[sy][sx]
                self.assertEqual(red, green, "intensity channels agree")
                self.assertEqual(green, blue, "intensity channels agree")
                self.assertEqual(red % 17, 0, "nibble * 17 intensity")
        for dy in range(176):
            for dx in range(240):
                sx = gen.sample_index(dx, 300, 240)
                sy = gen.sample_index(dy, 220, 176)
                self.assertEqual(data[dy * 240 + dx], raster[sy][sx][0])
        seen_zero = any(v == 0 for v in data)
        seen_full = any(v == 255 for v in data)
        self.assertTrue(seen_zero, "black intensity 0 survives")
        self.assertTrue(seen_full, "full intensity 255 survives")


if __name__ == "__main__":
    unittest.main()
