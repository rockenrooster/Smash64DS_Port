#!/usr/bin/env python3
"""Read-root isolation fixtures; no generators, builds, or reference writes."""
from __future__ import annotations

import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import _paths
import generate_battle_hud as hud
import generate_mn_ui_kit as kit
import generate_nds_banner_icon as banner
import generate_native_wallpapers as wallpaper
import generate_nds_native_stage as stage
import generate_nds_particle_banks as particles


O2R_PREFIX = Path("decomp/BattleShip-main/BattleShip_o2r")
RELOC_PREFIX = Path("decomp/BattleShip-main/decomp/assets/us/relocData")


def reloc_container(file_id: int, payload: bytes = b"\0" * 4) -> bytes:
    header = bytearray(0x50)
    struct.pack_into("<I", header, 4, 0x52454C4F)
    struct.pack_into("<IHHII", header, 0x40, file_id, 0xFFFF, 0xFFFF, 0,
                     len(payload))
    return bytes(header) + payload


class ReferenceRootTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(prefix="reference-read-roots-")
        self.base = Path(self.temp.name)
        self.repo = self.base / "isolated checkout"
        self.assets = self.base / "qualified O2R input"
        self.reloc = self.base / "qualified decompressed input"
        self.repo.mkdir()
        self.assets.mkdir()
        self.reloc.mkdir()
        self.env = patch.dict(os.environ, {}, clear=True)
        self.env.start()

    def tearDown(self) -> None:
        self.env.stop()
        self.temp.cleanup()

    def write(self, path: Path, payload: bytes) -> Path:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)
        return path

    def select_external(self) -> None:
        os.environ["BATTLESHIP_O2R"] = str(self.assets)
        os.environ["BATTLESHIP_RELOCDATA"] = str(self.reloc)

    def test_default_roots_preserve_the_original_local_input_layout(self) -> None:
        self.assertEqual(_paths.battleship_o2r_root(self.repo),
                         (self.repo / O2R_PREFIX).resolve())
        self.assertEqual(_paths.battleship_relocdata_root(self.repo),
                         (self.repo / RELOC_PREFIX).resolve())
        self.assertEqual(_paths.battleship_input_path(self.repo, O2R_PREFIX / "bank"),
                         self.repo / O2R_PREFIX / "bank")

    def test_explicit_roots_route_assets_without_redirecting_source_or_outputs(self) -> None:
        self.select_external()
        self.assertEqual(_paths.battleship_input_path(
            self.repo, O2R_PREFIX / "reloc_menus/MNTitle"),
            self.assets / "reloc_menus/MNTitle")
        self.assertEqual(_paths.battleship_input_path(
            self.repo, RELOC_PREFIX / "346.vpk0.bin"), self.reloc / "346.vpk0.bin")
        for relative in (
            "src/nds/nds_renderer.c", "include/reloc_data.h",
            "decomp/BattleShip-main/decomp/src/relocData/35_FTEmblemModels.c",
            "decomp/BattleShip-main/include/reloc_data.us.h",
            "assets/menus/mn_surfaces.bin", "src/nds/generated/mn_ui_kit.flags.stamp",
            "decomp/BattleShip-main/BattleShip_o2r-old/asset",
        ):
            with self.subTest(relative=relative):
                self.assertEqual(_paths.battleship_input_path(self.repo, relative),
                                 self.repo / relative)

    @unittest.skipUnless(os.name == "nt", "MSYS drive spelling is Windows-specific")
    def test_native_windows_accepts_make_msys_absolute_drive_paths(self) -> None:
        native = self.assets.resolve().as_posix()
        os.environ["BATTLESHIP_O2R"] = "/" + native[0].lower() + native[2:]
        self.assertEqual(_paths.battleship_o2r_root(self.repo), self.assets.resolve())
        native = self.reloc.resolve().as_posix()
        os.environ["BATTLESHIP_RELOCDATA"] = "/" + native[0].lower() + native[2:]
        self.assertEqual(_paths.battleship_relocdata_root(self.repo), self.reloc.resolve())

    def test_explicit_missing_corpus_does_not_fall_back_to_local_assets(self) -> None:
        self.write(self.repo / O2R_PREFIX / "reloc_menus/MNTitle", reloc_container(18))
        os.environ["BATTLESHIP_O2R"] = str(self.base / "missing corpus")
        with self.assertRaisesRegex(kit.ConvertError, "missing corpus"):
            kit.o2r_path(self.repo, "MNTitle")
        spec = stage.InputSpec(str(O2R_PREFIX / "reloc_menus/MNTitle"), "0" * 64)
        with self.assertRaisesRegex(stage.Falsifier, "required input is absent"):
            stage.checked_bytes(self.repo, spec)

    def test_stage_asset_pins_and_local_source_pins_are_still_enforced(self) -> None:
        self.select_external()
        asset = b"qualified stage bytes"
        path = self.write(self.assets / "reloc_stages/StageDreamLand", asset)
        spec = stage.InputSpec(str(O2R_PREFIX / "reloc_stages/StageDreamLand"),
                               hashlib.sha256(asset).hexdigest())
        log = io.StringIO()
        with contextlib.redirect_stderr(log):
            self.assertEqual(stage.checked_bytes(self.repo, spec), asset)
        identity = json.loads(log.getvalue().strip().removeprefix("REFERENCE_INPUT "))
        self.assertEqual(identity, {"path": str(path.resolve()), "bytes": len(asset),
                                    "sha256": hashlib.sha256(asset).hexdigest()})
        path.write_bytes(b"wrong stage bytes")
        with self.assertRaisesRegex(stage.Falsifier, "SHA256.*pinned"):
            stage.checked_bytes(self.repo, spec)
        relative = "src/nds/nds_renderer.c"
        local = b"own checkout source"
        self.write(self.repo / relative, local)
        source_spec = stage.InputSpec(relative, hashlib.sha256(local).hexdigest())
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(stage.checked_bytes(self.repo, source_spec), local)
        (self.repo / relative).write_bytes(b"owner source edit")
        with self.assertRaisesRegex(stage.Falsifier, "SHA256.*pinned"):
            stage.checked_bytes(self.repo, source_spec)

    def test_menu_reloc_reader_and_font_use_the_selected_corpus(self) -> None:
        self.select_external()
        path = self.write(self.assets / "reloc_menus/MNCommonFonts", reloc_container(33))
        self.write(self.repo / O2R_PREFIX / "reloc_menus/MNCommonFonts", reloc_container(99))
        with contextlib.redirect_stderr(io.StringIO()):
            file = kit.RelocFile(kit.o2r_path(self.repo, "MNCommonFonts"))
            self.assertEqual(file.path, path)
            self.assertEqual(file.file_id, 33)
            with self.assertRaisesRegex(kit.ConvertError, "file id 0x21.*0xff"):
                kit.convert_font(self.repo, {"llMNCommonFontsFileID": 255})

    def test_menu_offsets_continue_to_validate_own_checkout_header(self) -> None:
        self.select_external()
        self.write(self.repo / "include/reloc_data.h", b"X(llOwnSprite, 0x1234)\n")
        self.assertEqual(kit.load_reloc_offsets(self.repo)["llOwnSprite"], 0x1234)
        self.write(self.repo / "decomp/BattleShip-main/include/reloc_data.us.h",
                   b"#define llOwnSprite ((intptr_t)0x5678)\n")
        with self.assertRaisesRegex(kit.ConvertError, "says 0x1234.*says 0x5678"):
            kit.load_reloc_offsets(self.repo)

    def test_hud_stock_reader_uses_external_bytes_and_keeps_sprite_validation(self) -> None:
        self.select_external()
        payload = bytearray(0x144)
        payload[8:88] = b"\x11" * 80
        struct.pack_into(">16H", payload, 0x58, 0, *([0xFFFF] * 15))
        struct.pack_into(">hh", payload, 0x104, 8, 10)
        struct.pack_into(">H", payload, 0x114, kit.SP_TEXSHUF)
        payload[0x130] = kit.G_IM_FMT_CI
        payload[0x131] = kit.G_IM_SIZ_4b
        path = self.write(self.assets / "reloc_fighters_main/FixtureIcon",
                          reloc_container(5456, payload))
        spec = {"file": "FixtureIcon", "sprite": 0x100, "texture": 8,
                "palettes": [0x58]}
        with contextlib.redirect_stderr(io.StringIO()):
            graphics, palettes = hud.stock_asset(kit, self.repo, spec)
        self.assertEqual(len(graphics), 32)
        self.assertEqual(len(palettes), 1)
        self.assertEqual(len(palettes[0]), 16)
        payload[0x130] = kit.G_IM_FMT_RGBA
        path.write_bytes(reloc_container(5456, payload))
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaisesRegex(
                hud.BakeError, "stock Sprite drifted"):
            hud.stock_asset(kit, self.repo, spec)

    def test_particle_blob_root_override_keeps_hash_magic_and_extent_gates(self) -> None:
        self.select_external()
        blob = bytearray(0x44)
        blob[4:8] = b"BLBO"
        struct.pack_into("<I", blob, 0x40, 4)
        blob += b"bank"
        path = self.write(self.assets / "particles/fixture_bank", blob)
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(particles.load_o2r_blob(
                self.repo, "fixture_bank", hashlib.sha256(blob).hexdigest()), b"bank")
        with self.assertRaisesRegex(SystemExit, "SHA-256"):
            particles.load_o2r_blob(self.repo, "fixture_bank", "0" * 64)
        for offset, value, error in ((4, ord("!"), "not an O2R blob"),
                                     (0x40, 5, "declared 5 bytes")):
            corrupt = bytearray(blob)
            corrupt[offset] = value
            path.write_bytes(corrupt)
            with contextlib.redirect_stderr(io.StringIO()), self.assertRaisesRegex(
                    SystemExit, error):
                particles.load_o2r_blob(self.repo, "fixture_bank",
                                        hashlib.sha256(corrupt).hexdigest())

    def test_particle_reloc_reader_keeps_palette_transparency_validation(self) -> None:
        self.select_external()
        raw = struct.pack(">16H", 0, *([0xFFFF] * 15))
        path = self.write(self.reloc / "fixture_palette.bin", raw)
        self.write(self.repo / RELOC_PREFIX / "fixture_palette.bin",
                   struct.pack(">16H", *([0xFFFF] * 16)))
        spec = {"file": "fixture_palette.bin", "palette_offsets": [0]}
        with patch.object(particles, "FIREBALL_ASSET", spec):
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(particles.build_fireball_palettes(self.repo),
                                 [[0] + [0x7FFF] * 15])
            path.write_bytes(struct.pack(">16H", *([0xFFFF] * 16)))
            with contextlib.redirect_stderr(io.StringIO()), self.assertRaisesRegex(
                    SystemExit, "entry 0 OPAQUE"):
                particles.build_fireball_palettes(self.repo)

    def test_wallpaper_and_banner_read_external_containers(self) -> None:
        self.select_external()
        self.write(self.assets / "reloc_stages/StageDreamLand", reloc_container(88))
        source = wallpaper.WallpaperSource("fixture", "reloc_stages/StageDreamLand",
                                           99, 0, True)
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaisesRegex(
                kit.ConvertError, "file id 0x58.*expected 0x63"):
            wallpaper.convert_source(self.repo, source)
        self.write(self.repo / "include/reloc_data.h",
                   f"X({banner.SOURCE_SYMBOL}, 0)\n".encode())
        self.write(self.assets / "reloc_menus/MNTitle", reloc_container(18))
        with contextlib.redirect_stderr(io.StringIO()), patch.object(
                banner, "decode_sprite_raster", side_effect=RuntimeError("pixel probe")):
            with self.assertRaisesRegex(RuntimeError, "pixel probe"):
                banner.build_icon(self.repo)


if __name__ == "__main__":
    unittest.main()
