#!/usr/bin/env python3
"""Audio admission wire/negative fixtures; no donor or DS build is executed."""
from __future__ import annotations

from pathlib import Path
import struct
import unittest

import extra_audio_adapter as audio


def chunk(tag: bytes, payload: bytes) -> bytes:
    return tag + struct.pack(">I", len(payload)) + payload + bytes(len(payload) & 1)


def aifc_fixture(comm_samples=16, predictor=0) -> bytes:
    # Independent AIFC fixture: mono s16, one VADPCM frame, order 2/predictor 1.
    # 80-bit extended 16000 Hz is authored directly, not through the parser.
    comm = struct.pack(">HIH", 1, comm_samples, 16)
    comm += bytes.fromhex("400cfa00000000000000") + b"VAPC\x00"
    book = b"stoc\x0bVADPCMCODES" + struct.pack(">HHH16h", 1, 2, 1, *([0] * 16))
    sound = struct.pack(">II", 0, 0) + bytes([predictor]) + bytes.fromhex("1234567012345670")
    body = b"AIFC" + chunk(b"COMM", comm) + chunk(b"APPL", book) + chunk(b"SSND", sound)
    return b"FORM" + struct.pack(">I", len(body)) + body


class AifcAdmissionTests(unittest.TestCase):
    def test_wire_codebook_and_extended_rate(self):
        parsed = audio.parse_aifc(aifc_fixture())
        self.assertEqual((parsed.sample_rate, parsed.sample_count, parsed.order, parsed.predictors),
                         (16000, 16, 2, 1))
        self.assertEqual(parsed.book, (0,) * 16)
        self.assertEqual(parsed.vadpcm.hex(), "001234567012345670")

    def test_source_form_length_has_distinct_semantics(self):
        data = bytearray(aifc_fixture())
        struct.pack_into(">I", data, 4, 177 * 123)
        parsed = audio.parse_aifc(bytes(data))
        self.assertEqual(parsed.form_size, 177 * 123)
        row = {"type": "VOICE", "length": -1, "sample_rate": 16000, "reverb": 40}
        ucd, art = audio.microcode(row, parsed)
        self.assertEqual(ucd.hex(), "de00d18000d2ffd3d4d224d5ff6f807bd0")
        self.assertEqual(art.hex(), "60800020fb503028089f407f70")

    def test_one_comm_sample_mismatch_is_recorded_not_invented(self):
        parsed = audio.parse_aifc(aifc_fixture(comm_samples=17))
        self.assertEqual(parsed.sample_count, 17)
        self.assertEqual(len(parsed.vadpcm) // 9 * 16, 16)
        with self.assertRaisesRegex(audio.AudioAdmissionError, "exceeds"):
            audio.parse_aifc(aifc_fixture(comm_samples=18))

    def test_truncated_chunk_and_bad_predictor_fail(self):
        with self.assertRaisesRegex(audio.AudioAdmissionError, "truncated"):
            audio.parse_aifc(aifc_fixture()[:-2])
        with self.assertRaisesRegex(audio.AudioAdmissionError, "predictor"):
            audio.parse_aifc(aifc_fixture(predictor=1))

    def test_chant_and_sleep_are_distinct_source_programs(self):
        parsed = audio.parse_aifc(aifc_fixture())
        chant, art = audio.microcode({"type": "CHANT", "length": 260,
                                     "sample_rate": 16000, "reverb": 0}, parsed)
        self.assertEqual(chant.hex(), "de04d18000d2ffd37fd224dc26d5ff6f8104d0")
        self.assertEqual(art.hex(), "60800020306420fb5a0b747f70")
        sleep, art = audio.microcode({"type": "SLEEP", "length": 0x120,
                                     "sample_rate": 16000, "reverb": 0}, parsed)
        self.assertEqual(sleep.hex(), "de00d18000d2ffdc0ad3c8d2a4d5ff778120d5ff778120d5ff778120d0")
        self.assertEqual(art.hex(), "60800020fb500a2c6470")


class SoundIdentityTests(unittest.TestCase):
    def test_incomplete_or_changed_donor_cannot_admit_audio(self):
        manifest = {"phase": "resolved", "character": "MetaKnight",
                    "outputs": [{"path": "extra/src/FGM.asm", "size": 4,
                                 "sha256": audio.sha256(b"code")}]}
        audio.verify_resolved_inputs(manifest, "MetaKnight", {"extra/src/FGM.asm": b"code"})
        with self.assertRaisesRegex(audio.AudioAdmissionError, "identity changed"):
            audio.verify_resolved_inputs(manifest, "MetaKnight", {"extra/src/FGM.asm": b"evil"})
        manifest["phase"] = "resolving"
        with self.assertRaisesRegex(audio.AudioAdmissionError, "resolved"):
            audio.verify_resolved_inputs(manifest, "MetaKnight", {"extra/src/FGM.asm": b"code"})

    def test_generated_ordinal_identity_is_required(self):
        log = "SOUND_ADD_LIST(character): {'AnnouncerMeta': '0609', 'chant': '060D'}\n"
        self.assertEqual(audio.resolved_sound_ids(log, {"AnnouncerMeta", "chant"}),
                         {"AnnouncerMeta": 0x609, "chant": 0x60D})
        with self.assertRaisesRegex(audio.AudioAdmissionError, "complete"):
            audio.resolved_sound_ids(log, {"AnnouncerMeta", "chant", "ko"})
        with self.assertRaisesRegex(audio.AudioAdmissionError, "duplicated"):
            audio.resolved_sound_ids(log.replace("060D", "0609"), {"AnnouncerMeta", "chant"})

    def test_generated_macro_settings_cannot_be_silently_ignored(self):
        valid = "add_sound(../build/extra_characters/MetaKnight/sounds/chant, SAMPLE_RATE_16000, FGM_TYPE_CHANT, 0, 260)"
        row = audio.generated_sound_rows(valid, "MetaKnight")[0]
        self.assertEqual((row["name"], row["type"], row["length"]), ("chant", "CHANT", 260))
        with self.assertRaises(audio.AudioAdmissionError):
            audio.generated_sound_rows(valid.replace("CHANT", "UNKNOWN"), "MetaKnight")


class PackExtensionTests(unittest.TestCase):
    @staticmethod
    def record(cue=4, ima=b"\x00\x00\x00\x00", envelope=None):
        return {"id": cue, "flags": 0, "ima": ima, "sample_count": 2,
                "frequency": 16000, "duration_ticks": 20,
                "volume": 70, "pan": 64, "sound": 12,
                "envelope": envelope or [], "loop_point_words": 0}

    def test_base_samples_envelopes_and_duplicate_bodies_are_preserved(self):
        base = [self.record(envelope=[{"tick": 7, "ds_volume": 30, "event_flags": 2}]),
                self.record(cue=8)]
        original = audio.serialize_records(base, 0x12345678)
        addition = self.record(cue=0x609, ima=b"\x22\x11\x00\x00")
        combined, metadata = audio.append_extension(original, [addition], {"entries": [{"id": 0x609}]})
        self.assertEqual(audio.unpack_records(combined), base + [addition])
        self.assertEqual(metadata["entry_count"], 3)
        self.assertTrue(metadata["original_records_preserved"])
        self.assertEqual(audio.HEADER.unpack_from(combined)[3], len(combined))
        first = audio.ENTRY.unpack_from(combined, 16)
        second = audio.ENTRY.unpack_from(combined, 48)
        self.assertEqual(first[2], second[2])
        self.assertEqual(combined[first[10]:first[10] + 4], b"\x07\x00\x1e\x02")

    def test_collision_and_unowned_tail_are_rejected(self):
        original = audio.serialize_records([self.record()], 1)
        with self.assertRaisesRegex(audio.AudioAdmissionError, "collides"):
            audio.append_extension(original, [self.record()], {})
        invalid = bytearray(original + b"\x00\x00\x00\x00")
        struct.pack_into("<I", invalid, 8, len(invalid))
        with self.assertRaisesRegex(audio.AudioAdmissionError, "unowned"):
            audio.unpack_records(bytes(invalid))


class LinkedProgramTests(unittest.TestCase):
    def test_actual_donor_map_bytes_are_checked(self):
        root = Path(__file__).resolve().parents[2]
        decoder = audio.load_module(root / "decomp/BattleShip-main/decomp/tools/extract_fgm.py", "audio_fixture_decoder")
        rom = bytearray(0x188400)
        rom[:4] = bytes.fromhex("80371240")
        struct.pack_into(">H", rom, 0x1883BA, 47)
        struct.pack_into(">I", rom, 0x3D798, 0x100)
        struct.pack_into(">I", rom, 0x3D790, 0x200)
        struct.pack_into(">2I", rom, 0x100, 1, 0x38C9F0)
        struct.pack_into(">2I", rom, 0x200, 1, 0x38F8C0)
        expected_ucd, expected_art = audio.microcode(
            {"type": "VOICE", "length": 20, "sample_rate": 16000, "reverb": 40},
            audio.parse_aifc(aifc_fixture()))
        rom[0x300:0x300 + len(expected_ucd)] = expected_ucd
        rom[0x400:0x400 + len(expected_art)] = expected_art
        main = "origin 0x0\nbase 0x80400000\n"
        ucd, art = audio.linked_microcode(bytes(rom), main, 0, expected_ucd, expected_art, decoder)
        self.assertEqual((ucd, art), (expected_ucd, expected_art))
        rom[0x300 + 12] ^= 1
        with self.assertRaisesRegex(audio.AudioAdmissionError, "macro differs"):
            audio.linked_microcode(bytes(rom), main, 0, expected_ucd, expected_art, decoder)


if __name__ == "__main__":
    unittest.main()
