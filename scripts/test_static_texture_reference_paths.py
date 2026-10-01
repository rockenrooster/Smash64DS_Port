"""Pinned static/water inputs may be external; sources and output roots stay local."""
import contextlib
import hashlib
import io
import os
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import _paths
import generate_battle_playable_texture_census as census
import generate_pupupu_water_aot as water
import preview_source_metadata as preview
import estimate_fighter_pack as estimator


class StaticTextureReferencePaths(unittest.TestCase):
    def test_preview_lookup_cache_tracks_the_actual_asset_root(self):
        with tempfile.TemporaryDirectory(prefix="preview-reference-") as directory:
            roots = [Path(directory) / "a", Path(directory) / "b"]
            legacy_cache = preview.O2R_ID_CACHE
            for index, root in enumerate(roots):
                root.mkdir()
                raw = bytearray(0x50)
                raw[4:8] = b"OLER"
                struct.pack_into("<I", raw, 0x40, 12345)
                path = root / f"asset{index}"
                path.write_bytes(raw)
                with patch.dict(os.environ, {"BATTLESHIP_O2R": str(root)}, clear=True):
                    self.assertEqual(preview.o2r_path_by_id(12345), path)
                    self.assertEqual(preview.o2r_path_by_id(12345, repo_root=Path(directory)), path)
            with patch.dict(os.environ, {"BATTLESHIP_O2R": str(roots[0])}, clear=True):
                self.assertEqual(preview.o2r_path_by_id(12345), roots[0] / "asset0")
                self.assertEqual(preview.o2r_path_by_id(12345, o2r_root=roots[1]), roots[1] / "asset1")
            self.assertIs(preview.O2R_ID_CACHE, legacy_cache)

    def test_shield_pack_sources_honor_the_qualified_corpus_and_keep_pin_checks(self):
        with tempfile.TemporaryDirectory(prefix="shield-reference-") as directory:
            root = Path(directory)
            payload = bytes(range(16))
            raw = bytearray(0x50)
            raw[4:8] = b"OLER"
            struct.pack_into("<IHHII", raw, 0x40, 999, 0xFFFF, 0xFFFF, 0, len(payload))
            data = bytes(raw) + payload
            (root / "fixture").write_bytes(data)
            member = {"path": "fixture", "id": 999, "data_bytes": 16,
                      "sha256": hashlib.sha256(data).hexdigest()}
            with patch.dict(os.environ, {"BATTLESHIP_O2R": str(root)}, clear=True):
                self.assertEqual(estimator.load_pinned_sources([member])[999]["payload"], payload)
                with self.assertRaises(estimator.Refusal):
                    estimator.load_pinned_sources([dict(member, sha256="0" * 64)])

    def test_corpus_override_keeps_pin_and_logical_identity(self):
        with tempfile.TemporaryDirectory(prefix="static-reference-") as directory:
            base = Path(directory)
            local = base / "isolated"
            external = base / "readonly"
            local.mkdir(); external.mkdir()
            relative = Path("decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/Fixture")
            payload = bytes(range(16))
            raw = bytearray(0x50)
            raw[4:8] = b"OLER"
            struct.pack_into("<IHHII", raw, 0x40, 103, 0xFFFF, 0xFFFF, 0, len(payload))
            data = bytes(raw) + payload
            path = external / "reloc_extern_data/Fixture"
            path.parent.mkdir(); path.write_bytes(data)
            digest = hashlib.sha256(data).hexdigest()
            spec = census.InputSpec(str(relative), digest, 103, 0, 0)
            with patch.dict(os.environ, {"BATTLESHIP_O2R": str(external)}, clear=True):
                receipt = io.StringIO()
                with contextlib.redirect_stderr(receipt):
                    self.assertEqual(census.checked_bytes(local, spec), data)
                    result = water.load_o2r_resource(local, relative, digest,
                                                     hashlib.sha256(payload).hexdigest(), 103, 0)
                self.assertEqual(result.relative_path, relative)
                self.assertEqual(result.payload, payload)
                self.assertIn(str(path).replace("\\", "\\\\"), receipt.getvalue())
                self.assertFalse((local / relative).exists())
                with self.assertRaises(census.Falsifier):
                    census.checked_bytes(local, census.InputSpec(str(relative), "0" * 64))
            with patch.dict(os.environ, {}, clear=True):
                with self.assertRaises(census.Falsifier):
                    census.checked_bytes(local, spec)

    def test_external_assets_do_not_redirect_tracked_source(self):
        with tempfile.TemporaryDirectory(prefix="static-source-") as directory:
            base = Path(directory)
            local, external = base / "local", base / "external"
            relative = Path("src/nds/nds_renderer.c")
            for root, body in ((local, b"local source"), (external, b"external source")):
                path = root / relative
                path.parent.mkdir(parents=True)
                path.write_bytes(body)
            with patch.dict(os.environ, {"NDS_REFERENCE_ROOT": str(external)}, clear=True):
                self.assertEqual(census.checked_bytes(local, census.InputSpec(str(relative),
                    hashlib.sha256(b"local source").hexdigest())), b"local source")


if __name__ == "__main__":
    unittest.main()
