#!/usr/bin/env python3
"""Host fixtures for census and generated-reference read roots.

Only temporary fixture trees are written. No producer, build, or reference
checkout is run or modified.
"""
from __future__ import annotations

import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import _paths
import generate_nds_native_ef_lakitu_bronto as lakitu
import generate_nds_native_link_bomb as bomb
import generate_nds_native_owners as owners
import generate_nds_native_pikachu_thunderground as ground
import generate_nds_native_pikachu_thunderjolt as air
import generate_nds_native_pikachu_thunderjolt_effect as effect
import generate_nds_native_stage as stage
import generate_nds_native_yamabuki_live_item as yamabuki
import native_asset_census


O2R_PREFIX = Path("decomp/BattleShip-main/BattleShip_o2r")
BUILD_PREFIX = Path("decomp/BattleShip-main/decomp/build")
DL_DIRECTORY = BUILD_PREFIX / "us/src/relocData/StageCastleFile2"
CENSUS_MODULES = (bomb, air, ground, effect, yamabuki)


def reloc_container(file_id: int, references=(), payload: bytes | None = None) -> bytes:
    """Encode a real OLER extern chain with payload-relative byte slots."""
    if payload is None:
        size = max([16] + [slot + 4 for slot, _, _ in references])
        data = bytearray(size)
    else:
        data = bytearray(payload)
    for index, (slot, _target, offset) in enumerate(references):
        next_word = (references[index + 1][0] // 4
                     if index + 1 < len(references) else 0xFFFF)
        struct.pack_into(">I", data, slot, (next_word << 16) | (offset // 4))
    header = bytearray(0x4C)
    header[4:8] = b"OLER"
    struct.pack_into("<IHHI", header, 0x40, file_id, 0xFFFF,
                     references[0][0] // 4 if references else 0xFFFF,
                     len(references))
    extern_ids = b"".join(struct.pack("<H", target)
                          for _, target, _ in references)
    return bytes(header) + extern_ids + struct.pack("<I", len(data)) + bytes(data)


class ReferenceCensusPathTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(prefix="reference-census-paths-")
        self.base = Path(self.temp.name)
        self.repo = self.base / "isolated checkout"
        self.assets = self.base / "qualified O2R corpus"
        self.reference = self.base / "qualified reference checkout"
        self.repo.mkdir()
        self.assets.mkdir()
        self.reference.mkdir()
        self.env = patch.dict(os.environ, {}, clear=True)
        self.env.start()

    def tearDown(self) -> None:
        self.env.stop()
        self.temp.cleanup()

    def write(self, path: Path, payload: bytes) -> Path:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)
        return path

    def populate_corpus(self, root: Path) -> dict[Path, bytes]:
        corpus = {
            Path("reloc_items/nested/bank21"): reloc_container(
                21, ((12, bomb.ASSET, 0x40), (0, air.ASSET, 0x20),
                     (8, 999, 0x10), (28, yamabuki.ASSET, 0x80))),
            Path("reloc_fighters_main/bank7"): reloc_container(
                7, ((4, bomb.ASSET, 0x88), (16, ground.ASSET, 0x100),
                    (24, effect.ASSET, effect.ROOT))),
            Path("reloc_stages/unreferenced"): reloc_container(6),
            Path("particles/not_OLER"): b"\0" * 4 + b"BLBO" + b"\0" * 96,
            Path("short_file"): b"OLER",
        }
        for relative, data in corpus.items():
            self.write(root / relative, data)
        (root / "ignored directory").mkdir()
        return corpus

    def census_calls(self):
        yield "shared", lambda: native_asset_census.census(
            self.repo, stage, bomb.ASSET), bomb.ASSET
        for module in CENSUS_MODULES:
            call = module._census if module is yamabuki else module.census
            yield module.__name__, call, module.ASSET

    def test_relocated_corpus_preserves_scan_count_slots_offsets_and_pins(self) -> None:
        corpus = self.populate_corpus(self.repo / O2R_PREFIX)
        self.populate_corpus(self.assets)
        expected = {}
        with contextlib.ExitStack() as stack:
            for module in CENSUS_MODULES:
                stack.enter_context(patch.object(module, "REPO", self.repo))
            for name, call, _ in self.census_calls():
                expected[name] = call()
            self.assertEqual(expected["shared"],
                             (3, ((7, 4, 0x88), (21, 12, 0x40))))
            self.assertEqual(expected[effect.__name__], (3, ((7, 24, effect.ROOT),)))
            self.assertEqual(expected[yamabuki.__name__], (3, ((21, 28, 0x80),)))
            os.environ["BATTLESHIP_O2R"] = str(self.assets)
            original_load = stage.load_o2r
            for name, call, _ in self.census_calls():
                with self.subTest(census=name):
                    specs = []

                    def audited_load(repo, spec):
                        specs.append(spec)
                        return original_load(repo, spec)

                    log = io.StringIO()
                    with patch.object(stage, "load_o2r", side_effect=audited_load), \
                            contextlib.redirect_stderr(log):
                        self.assertEqual(call(), expected[name])
                    self.assertEqual(len(specs), 3)
                    for spec in specs:
                        logical = Path(spec.path)
                        self.assertTrue(logical.is_relative_to(O2R_PREFIX))
                        relative = logical.relative_to(O2R_PREFIX)
                        self.assertEqual(spec.sha256,
                                         hashlib.sha256(corpus[relative]).hexdigest())
                        self.assertRegex(spec.sha256, r"^[0-9a-f]{64}$")
                    receipts = [json.loads(line.removeprefix("REFERENCE_INPUT "))
                                for line in log.getvalue().splitlines()
                                if line.startswith("REFERENCE_INPUT ")]
                    self.assertTrue(receipts)
                    for receipt in receipts:
                        path = Path(receipt["path"])
                        self.assertTrue(path.is_relative_to(self.assets.resolve()))
                        self.assertEqual(receipt["sha256"],
                                         hashlib.sha256(path.read_bytes()).hexdigest())
                        self.assertEqual(receipt["bytes"], path.stat().st_size)

    def test_missing_selected_corpus_fails_instead_of_returning_zero_or_local_hits(self) -> None:
        self.populate_corpus(self.repo / O2R_PREFIX)
        os.environ["BATTLESHIP_O2R"] = str(self.base / "missing corpus")
        with contextlib.ExitStack() as stack:
            for module in CENSUS_MODULES:
                stack.enter_context(patch.object(module, "REPO", self.repo))
            for name, call, _ in self.census_calls():
                with self.subTest(census=name), self.assertRaises(
                        (stage.Falsifier, FileNotFoundError, RuntimeError)):
                    call()

    def test_relocated_oler_corpus_keeps_relocation_validation(self) -> None:
        os.environ["BATTLESHIP_O2R"] = str(self.assets)
        broken = bytearray(reloc_container(17, ((0, bomb.ASSET, 0x20),)))
        # One extern ID, but the payload's relocation chain ends before visiting it.
        struct.pack_into("<H", broken, 0x46, 0xFFFF)
        self.write(self.assets / "reloc_items/broken", broken)
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaisesRegex(
                stage.Falsifier, "extern chain/file table mismatch"):
            native_asset_census.census(self.repo, stage, bomb.ASSET)

    def owner_fixture(self, data: bytes, file_id: int = 0x21):
        relative = O2R_PREFIX / "reloc_fighters_main/FixtureOwner"
        self.write(self.assets / relative.relative_to(O2R_PREFIX), data)
        self.write(self.repo / relative, b"local corpus must not be read")
        return patch.dict(owners.P2_O2R_ASSETS, {
            "fixture": (str(relative), file_id, hashlib.sha256(data).hexdigest())})

    def test_native_owner_payload_uses_selected_corpus_and_keeps_hash_pin(self) -> None:
        os.environ["BATTLESHIP_O2R"] = str(self.assets)
        payload = b"owner payload fixture"
        source = reloc_container(0x21, payload=payload)
        with self.owner_fixture(source), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(owners.load_o2r_payload(self.repo, "fixture"), payload)
            (self.assets / "reloc_fighters_main/FixtureOwner").write_bytes(source + b"!")
            with self.assertRaisesRegex(ValueError, "SHA256.*!="):
                owners.load_o2r_payload(self.repo, "fixture")

    def test_native_owner_selected_bytes_still_validate_header_id_and_extent(self) -> None:
        os.environ["BATTLESHIP_O2R"] = str(self.assets)
        valid = reloc_container(0x21, payload=b"owner payload fixture")
        wrong_magic = bytearray(valid)
        wrong_magic[4:8] = b"NOPE"
        wrong_id = bytearray(valid)
        struct.pack_into("<I", wrong_id, 0x40, 0x22)
        wrong_extent = bytearray(valid)
        struct.pack_into("<I", wrong_extent, 0x4C, 999)
        for data, message in ((bytes(wrong_magic), "invalid resource magic"),
                              (bytes(wrong_id), "file ID"),
                              (bytes(wrong_extent), "data ends"),
                              (b"short", "truncated resource header")):
            with self.subTest(gate=message), self.owner_fixture(data), \
                    contextlib.redirect_stderr(io.StringIO()), \
                    self.assertRaisesRegex(ValueError, message):
                owners.load_o2r_payload(self.repo, "fixture")

    def test_native_owner_missing_selected_input_has_no_local_fallback(self) -> None:
        source = reloc_container(0x21)
        with self.owner_fixture(source):
            os.environ["BATTLESHIP_O2R"] = str(self.base / "missing corpus")
            with self.assertRaises(FileNotFoundError):
                owners.load_o2r_payload(self.repo, "fixture")

    def test_generated_build_route_keeps_source_outputs_and_prefix_escapes_local(self) -> None:
        os.environ["NDS_REFERENCE_ROOT"] = str(self.reference)
        relative = BUILD_PREFIX / "us/src/relocData/StageCastleFile2/DL_0x3F20.dl.inc.c"
        self.assertEqual(_paths.battleship_input_path(self.repo, relative),
                         self.reference / relative)
        for relative in (
            "decomp/BattleShip-main/decomp/src/relocData/106_StageCastleFile2.c",
            "decomp/BattleShip-main/decomp/build-old/us/generated.c",
            "decomp/BattleShip-main/decomp/building/generated.c",
            "decomp/BattleShip-main/BattleShip_o2r-old/bank",
            "decomp/BattleShip-main/decomp/assets/us/relocData-old/bank",
            "decomp/BattleShip-main/decomp/build/../src/relocData/106_StageCastleFile2.c",
            "decomp/BattleShip-main/BattleShip_o2r/../decomp/src/gr/grcommon/gryoster.c",
            "decomp/BattleShip-main/decomp/assets/us/relocData/../../../src/local.c",
            "src/nds/generated/nds_native_actor_ef_lakitu.generated.inc",
            "include/nds/nds_native_actor_ef_lakitu.h",
            "assets/nds/fighters/meta_owner.bin",
        ):
            with self.subTest(relative=relative):
                self.assertEqual(_paths.battleship_input_path(self.repo, relative),
                                 self.repo / relative)

    def test_reference_root_supplies_assets_and_explicit_corpus_roots_take_precedence(self) -> None:
        os.environ["NDS_REFERENCE_ROOT"] = str(self.reference)
        self.assertEqual(_paths.battleship_o2r_root(self.repo),
                         self.reference / O2R_PREFIX)
        relative = Path("decomp/BattleShip-main/decomp/assets/us/relocData/346.bin")
        self.assertEqual(_paths.battleship_input_path(self.repo, relative),
                         self.reference / relative)
        os.environ["BATTLESHIP_O2R"] = str(self.assets)
        os.environ["BATTLESHIP_RELOCDATA"] = str(self.base / "explicit decompressed corpus")
        self.assertEqual(_paths.battleship_o2r_root(self.repo), self.assets)
        self.assertEqual(_paths.battleship_input_path(self.repo, relative),
                         self.base / "explicit decompressed corpus/346.bin")

    def test_generated_build_input_keeps_pin_and_exact_read_receipt(self) -> None:
        os.environ["NDS_REFERENCE_ROOT"] = str(self.reference)
        relative = DL_DIRECTORY / "DL_0x3F20.dl.inc.c"
        data = b"qualified generated display-list source"
        path = self.write(self.reference / relative, data)
        self.write(self.repo / relative, b"unqualified local build input")
        spec = stage.InputSpec(str(relative), hashlib.sha256(data).hexdigest())
        log = io.StringIO()
        with contextlib.redirect_stderr(log):
            self.assertEqual(stage.checked_bytes(self.repo, spec), data)
        receipt = json.loads(log.getvalue().strip().removeprefix("REFERENCE_INPUT "))
        self.assertEqual(receipt, {"path": str(path.resolve()), "bytes": len(data),
                                   "sha256": hashlib.sha256(data).hexdigest()})
        path.write_bytes(b"changed reference build input")
        with self.assertRaisesRegex(stage.Falsifier, "SHA256.*pinned"):
            stage.checked_bytes(self.repo, spec)

    def test_missing_selected_build_input_has_no_local_fallback(self) -> None:
        os.environ["NDS_REFERENCE_ROOT"] = str(self.reference)
        relative = DL_DIRECTORY / "DL_0x3F20.dl.inc.c"
        data = b"local generated source"
        self.write(self.repo / relative, data)
        spec = stage.InputSpec(str(relative), hashlib.sha256(data).hexdigest())
        with self.assertRaisesRegex(stage.Falsifier, "required input is absent"):
            stage.checked_bytes(self.repo, spec)

    @unittest.skipUnless(os.name == "nt", "MSYS drive spelling is Windows-specific")
    def test_native_windows_reference_root_accepts_make_msys_drive_path(self) -> None:
        native = self.reference.resolve().as_posix()
        os.environ["NDS_REFERENCE_ROOT"] = "/" + native[0].lower() + native[2:]
        relative = DL_DIRECTORY / "DL_0x3F20.dl.inc.c"
        self.assertEqual(_paths.battleship_input_path(self.repo, relative),
                         self.reference.resolve() / relative)

    def test_lakitu_generated_dl_reads_use_reference_and_typed_source_stays_local(self) -> None:
        os.environ["NDS_REFERENCE_ROOT"] = str(self.reference)
        names = ("DL_0x3F20.dl.inc.c", "DL_0x3F80.dl.inc.c", "DL_0x3FF8.dl.inc.c",
                 "gap_0x3684_sub_0x9EC.dl.inc.c")
        tokens = (
            "gsSPDisplayList((Gfx *)dStageCastleFile2_DL_0x3F80)",
            "gsSPVertex((Vtx *)dStageCastleFile2_gap_0x3684_sub_0x7DC, 4, 0)",
            "gsSPVertex((Vtx *)dStageCastleFile2_gap_0x3684_sub_0x81C, 4, 0)",
            "gsSPVertex((Vtx *)dStageCastleFile2_gap_0x3684_sub_0x85C, 4, 0)",
            "gsSP2Triangles(3, 2, 1, 0, 0, 3, 1, 0)",
            "gsSP2Triangles(3, 2, 1, 0, 2, 0, 1, 0)",
        )
        for name in names:
            self.write(self.reference / DL_DIRECTORY / name, "\n".join(tokens).encode())
        payload = bytearray(0x4200)
        pointers = {}
        for name, (offset, _count) in lakitu.LAK_DLS.items():
            for index, opcode in enumerate(lakitu.LAK_EXPECTED_OPS[name]):
                struct.pack_into(">II", payload, offset + index * 8, opcode << 24, 0)
        for run in lakitu.LAK_RUNS:
            offset, _ = lakitu.LAK_DLS[run["dl"]]
            for index, target in ((12, run["vtx"]), (8, run["img"][0]),
                                  (3, run["pal"][0])):
                pointers[offset + index * 8 + 4] = stage.PointerRef(106, target)
        resource = SimpleNamespace(payload=bytes(payload), pointer_at=pointers.get)

        def geometry(_res, _payload, _off, _cnt, tag, loads, _want_tris):
            triangles = [(3, 2, 1), (2, 0, 1) if tag.endswith("quad2") else (0, 3, 1)]
            return [], [()] * 4, triangles, loads

        local_typed = self.repo / (
            "decomp/BattleShip-main/decomp/src/relocData/106_StageCastleFile2.c")
        reads = []
        original_read = Path.read_text
        original_bytes = Path.read_bytes
        failures = []

        class ReachedLocalSource(Exception):
            pass

        def read_text(path, *args, **kwargs):
            reads.append(path)
            if path == local_typed:
                raise ReachedLocalSource
            return original_read(path, *args, **kwargs)

        def read_bytes(path):
            reads.append(path)
            return original_bytes(path)

        with patch.object(lakitu, "REPO", self.repo), \
                patch.object(lakitu, "CASTLE_TYPED", local_typed), \
                patch.object(lakitu, "load_bank", return_value=resource), \
                patch.object(lakitu, "walk_geometry", side_effect=geometry), \
                patch.object(lakitu, "FAILURES", failures), \
                patch.object(Path, "read_text", read_text), \
                patch.object(Path, "read_bytes", read_bytes), \
                contextlib.redirect_stderr(io.StringIO()), \
                self.assertRaises(ReachedLocalSource):
            lakitu.decode_lakitu()
        self.assertEqual(reads, [self.reference / DL_DIRECTORY / name for name in names]
                         + [local_typed])
        self.assertEqual(failures, [])


if __name__ == "__main__":
    unittest.main()
