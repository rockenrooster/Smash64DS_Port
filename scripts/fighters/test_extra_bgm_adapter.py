#!/usr/bin/env python3
"""Independent score/bank/control rejection fixtures for EXTRA victory music."""
from pathlib import Path
import struct
import unittest

import extra_bgm_adapter as bgm


class DecoderFixture:
    SIZE_OF = {"ALSound": (16, 16), "ALEnvelope": (14, 16),
               "ALKeyMap": (6, 8), "ALWaveTable": (20, 24)}

    @staticmethod
    def book_alignment(size):
        return (8 + size + 15) // 16 * 16


class SourceBankFixture:
    """One complete authored graph behind an original music-bank index."""
    def __init__(self):
        self.ctl = bytearray(320)
        self.tbl = bytes.fromhex("001234567012345670")
        self.bank = {"flags": 0, "pad": 0, "sampleRate": 32000,
                     "percussion_off": 0, "instCount": 1, "instArray_offs": [64]}
        struct.pack_into(">hBBiII", self.ctl, 32, 1, 0, 0, 32000, 0, 64)
        struct.pack_into(">BBBB8BhhII", self.ctl, 64,
                         126, 63, 5, 0, *([0] * 8), 200, 1, 96, 0)
        struct.pack_into(">III4B", self.ctl, 96, 112, 128, 144, 64, 127, 0, 0)
        struct.pack_into(">iii4B", self.ctl, 112, 0, 0, 30000, 127, 127, 0, 0)
        struct.pack_into(">8B", self.ctl, 128, 0, 127, 0, 127, 60, 0, 0, 0)
        struct.pack_into(">IIBBHIII", self.ctl, 144, 0, 9, 0, 0, 0, 0, 176, 0)
        struct.pack_into(">ii16h", self.ctl, 176, 2, 1, *([0] * 16))
        self.by_offset = {
            64: {"offset": 64, "kind": "ALInstrument", "soundCount": 1, "soundArray_offs": [96]},
            96: {"offset": 96, "kind": "ALSound", "env_off": 112, "keyMap_off": 128, "wavetable_off": 144},
            112: {"offset": 112, "kind": "ALEnvelope"},
            128: {"offset": 128, "kind": "ALKeyMap"},
            144: {"offset": 144, "kind": "ALWaveTable", "base": 0, "length": 9,
                  "type": 0, "book_off": 176, "loop_off": 0},
            176: {"offset": 176, "kind": "ALADPCMBook", "entries": [0] * 16},
        }
        self.rom = bytearray(0x3D770)
        struct.pack_into(">I", self.rom, 0x3D75C, 0x1000)
        struct.pack_into(">I", self.rom, 0x3D760, 0x2000)
        self.rom[0x1000:0x1000 + len(self.ctl)] = self.ctl
        struct.pack_into(">I", self.rom, 0x1004, 32)
        self.rom[0x2000:0x2000 + len(self.tbl)] = self.tbl

    def qualify(self):
        return bgm.qualify_bank(bytes(self.rom), "origin 0x2c00000\nbase 0x80400000\n",
                                bytes(self.ctl), self.tbl, self.bank, self.by_offset, {0}, DecoderFixture)


class SourceBankTests(unittest.TestCase):
    def test_complete_graph_and_sample_identity(self):
        source = SourceBankFixture()
        result = source.qualify()
        self.assertEqual(result["reachable_struct_count"], 6)
        self.assertEqual(result["reachable_kind_counts"]["ALEnvelope"], 1)
        self.assertTrue(result["original_instrument_graph_and_samples_exact"])
        self.assertEqual(result["waves"][0]["sha256"], bgm.audio.sha256(source.tbl))

    def test_source_envelope_or_wave_mutation_cannot_use_original_bank(self):
        for offset in (0x1000 + 112, 0x2000 + 1):
            source = SourceBankFixture()
            source.rom[offset] ^= 1
            with self.assertRaisesRegex(bgm.BgmAdmissionError, "changed"):
                source.qualify()

    def test_changed_program_mapping_and_missing_reached_object_fail(self):
        source = SourceBankFixture()
        struct.pack_into(">I", source.rom, 0x1000 + 32 + 12, 68)
        with self.assertRaisesRegex(bgm.BgmAdmissionError, "mapping changed"):
            source.qualify()
        source = SourceBankFixture()
        del source.by_offset[112]
        with self.assertRaisesRegex(bgm.BgmAdmissionError, "missing reachable"):
            source.qualify()


class ScoreIdentityTests(unittest.TestCase):
    def test_table_resolves_exact_score_without_assuming_midi_order(self):
        rom = bytearray(0x3D770)
        struct.pack_into(">I", rom, 0x3D768, 0x100)
        struct.pack_into(">HH4I", rom, 0x100, 0x5331, 2, 0x100, 4, 0x110, 4)
        rom[0x200:0x204] = b"base"
        rom[0x210:0x214] = b"meta"
        self.assertEqual(bgm.resolve_score(bytes(rom), b"meta"), (1, 0x210))
        rom[0x200:0x204] = b"meta"
        with self.assertRaisesRegex(bgm.BgmAdmissionError, "unique"):
            bgm.resolve_score(bytes(rom), b"meta")

    def test_finite_native_header_uses_derived_identity(self):
        header = bgm.runtime_header({"music_id": 375, "source_pcm_bytes": 260096,
                                     "bytes": 65160, "packet_count": 8})
        self.assertIn("NDS_P4_METAKNIGHT_VICTORY_BGM 375u", header)
        self.assertIn("NDS_P4_METAKNIGHT_VICTORY_PACKET_COUNT 8u", header)


class ControlAdmissionTests(unittest.TestCase):
    @staticmethod
    def fixture():
        notes = [{"program": 5}]
        bank = {"instArray_offs": [0] * 5 + [64]}
        by_offset = {64: {"tremType": 0, "vibType": 128}}
        return notes, bank, by_offset

    def test_sine_oscillator_is_neutral_with_source_zero_pressure(self):
        notes, bank, by_offset = self.fixture()
        events = [(0, 0, 0, ("midi", 0xB0, 22, 24)),
                  (0, 0, 0, ("midi", 0xB0, 23, 127)),
                  (0, 0, 0, ("midi", 0xB0, 6, 12))]
        result = bgm.qualify_controls(events, notes, by_offset, bank)
        self.assertEqual(result["effective_vibrato_modulation"], 0)
        self.assertEqual(len(result["source_fx_controls"]), 2)

    def test_live_gain_and_active_oscillator_require_conversion(self):
        notes, bank, by_offset = self.fixture()
        for event in ((1, 0, 0, ("midi", 0xB0, 7, 90)),
                      (0, 0, 0, ("midi", 0xD0, 64, None)),
                      (0, 0, 0, ("midi", 0xB0, 64, 127))):
            with self.assertRaises(bgm.BgmAdmissionError):
                bgm.qualify_controls([event], notes, by_offset, bank)


if __name__ == "__main__":
    unittest.main()
