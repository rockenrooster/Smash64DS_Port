#!/usr/bin/env python3
"""Admission boundaries; no reference writes, downloads, or donor builds."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import prepare_extra_donor as donor


class AdmissionTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(prefix="extra-admission-")
        self.root = Path(self.temp.name)

    def tearDown(self) -> None:
        self.temp.cleanup()

    def repository(self) -> tuple[Path, str]:
        root = self.root / "reference"
        root.mkdir()
        (root / "input.bin").write_bytes(b"source bytes")
        donor.run(["git", "init", str(root)])
        donor.run(["git", "-C", str(root), "add", "input.bin"])
        donor.run(["git", "-C", str(root), "-c", "user.name=Admission Fixture",
                   "-c", "user.email=fixture@example.invalid", "commit", "-m", "fixture"])
        pin = donor.run(["git", "-C", str(root), "rev-parse", "HEAD"])
        return root, pin

    def test_wrong_pin_is_rejected_before_copy(self) -> None:
        root, pin = self.repository()
        donor.verify_repository(root, pin)
        with self.assertRaisesRegex(donor.AdmissionError, "wrong source pin"):
            donor.verify_repository(root, "0" * 40)

    def test_empty_nested_submodule_cannot_inherit_parent_identity(self) -> None:
        root, pin = self.repository()
        nested = root / "nested"
        nested.mkdir()
        with self.assertRaisesRegex(donor.AdmissionError, "not an initialized repository root"):
            donor.verify_repository(nested, pin)

    def test_dirty_reference_is_rejected(self) -> None:
        root, pin = self.repository()
        (root / "input.bin").write_bytes(b"owner edit")
        with self.assertRaisesRegex(donor.AdmissionError, "not clean"):
            donor.verify_repository(root, pin)

    def test_escaping_destination_and_builds_root_are_rejected(self) -> None:
        for path in (self.root / "decomp/stage", self.root / "builds",
                     self.root / "builds/../../elsewhere"):
            with self.subTest(path=path), self.assertRaises(donor.AdmissionError):
                donor.destination(path, self.root)
        allowed = self.root / "builds/meta-knight"
        self.assertEqual(donor.destination(allowed, self.root), allowed.resolve())

    def test_wrong_rom_bytes_and_byte_order_are_rejected(self) -> None:
        path = self.root / "owner.z64"
        payload = donor.ROM_MAGIC + b"fixture"
        path.write_bytes(payload)
        with patch.object(donor, "ROM_SIZE", len(payload)), patch.object(
                donor, "ROM_SHA1", hashlib.sha1(payload).hexdigest()):
            donor.verify_rom(path)
            path.write_bytes(payload[:-1] + b"!")
            with self.assertRaisesRegex(donor.AdmissionError, "wrong source ROM SHA-1"):
                donor.verify_rom(path)
            path.write_bytes(bytes.fromhex("37804012") + payload[4:])
            with self.assertRaisesRegex(donor.AdmissionError, "big-endian"):
                donor.verify_rom(path)
            path.write_bytes(payload[:-1])
            with self.assertRaisesRegex(donor.AdmissionError, "wrong source ROM size"):
                donor.verify_rom(path)

    def character(self) -> Path:
        folder = self.root / "extra_characters/MetaKnight"
        folder.mkdir(parents=True)
        for name in ("config.yaml", "main.asm", "MetaKnightSpecial.asm", "CPU.asm",
                     "main.bin", "character.bin", "main_reqlist.txt", "portrait.png",
                     "portrait_flash.png", "nameplate.png", "nameplate_singleplayer.png",
                     "victory_theme.bin", "transparency.txt"):
            (folder / name).write_bytes(b"fixture")
        for subdir, filename in (("animations", "idle.bin"), ("moveset", "IDLE.bin"),
                                 ("sounds", "taunt.aifc")):
            (folder / subdir).mkdir()
            (folder / subdir / filename).write_bytes(b"fixture")
        (folder / "main.asm").write_text(
            'File.METAKNIGHT_ANIM_IDLE\ninsert IDLE_, "moveset/IDLE.bin"\n'
            '// File.METAKNIGHT_ANIM_COMMENTED_OUT\n', encoding="utf-8")
        return folder

    def test_missing_reached_motion_and_missing_model_are_rejected(self) -> None:
        folder = self.character()
        donor.verify_character(self.root)
        (folder / "animations/idle.bin").unlink()
        with self.assertRaisesRegex(donor.AdmissionError, "missing Meta Knight animation: IDLE"):
            donor.verify_character(self.root)
        (folder / "animations/idle.bin").write_bytes(b"fixture")
        (folder / "character.bin").unlink()
        with self.assertRaisesRegex(donor.AdmissionError, "missing or symlinked input file"):
            donor.verify_character(self.root)

    def test_source_mutation_between_preflight_and_copy_is_rejected(self) -> None:
        root, pin = self.repository()
        snapshot = donor.verify_repository(root, pin)
        (root / "input.bin").write_bytes(b"changed later")
        with self.assertRaisesRegex(donor.AdmissionError, "source changed after preflight"):
            donor.copy_source(snapshot, self.root / "staged")
        self.assertFalse((self.root / "staged").exists())

    def test_unpinned_dependency_is_rejected(self) -> None:
        lock = self.root / "Pipfile.lock"
        lock.write_text(json.dumps({"default": {package: {
            "version": "==1.0", "hashes": ["sha256:" + "a" * 64]}
            for package in donor.APPENDER_PACKAGES}}), encoding="utf-8")
        self.assertIn("--hash=sha256:", donor.locked_requirements(lock))
        data = json.loads(lock.read_text())
        data["default"]["lineinfile"]["version"] = "*"
        lock.write_text(json.dumps(data), encoding="utf-8")
        with self.assertRaisesRegex(donor.AdmissionError, "missing exact package pin"):
            donor.locked_requirements(lock)

    def test_review_export_can_change_crc_but_never_gameplay_bytes(self) -> None:
        reference = bytes(range(64))
        review = bytearray(reference + b"appended typed export")
        review[0x10:0x18] = b"CRCWORDS"
        self.assertTrue(donor.compare_review_payload(reference, bytes(review))[
            "existing_payload_identical"])
        review[32] ^= 1
        with self.assertRaisesRegex(donor.AdmissionError, "existing donor payload"):
            donor.compare_review_payload(reference, bytes(review))
        with self.assertRaisesRegex(donor.AdmissionError, "existing donor payload"):
            donor.compare_review_payload(reference, reference)


if __name__ == "__main__":
    unittest.main()
