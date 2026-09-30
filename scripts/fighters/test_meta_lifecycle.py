"""Source-bound lifecycle transformations and the actual C save-tail codec."""
from __future__ import annotations

import ctypes
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

import generate_meta_lifecycle_source as lifecycle


class LifecycleSourceTests(unittest.TestCase):
    def test_owned_source_transformations_close_the_named_indexed_reads(self):
        root = lifecycle.ROOT / "decomp/BattleShip-main/decomp/src"
        rows = {
            "ft/ftchar/ftkirby/ftkirbyspecialn.c": "copy[victim_fp->fkind].copy_id",
            "ft/ftcommon/ftcommoncapturekirby.c": "copy[fp->fkind].star_damage",
            "ft/ftpublic.c": "dFTCommonDataPublicFighterCallFGMs[ftGetStruct(fighter_gobj)->fkind]",
            "mn/mnvsmode/mnvsresults.c": "gSCManagerBackupData.vs_records[this_fkind]",
            "ef/efmanager.c": "dFTCommonYoshiEggDamageCollDescs[fp->fkind].effect_size",
            "ft/ftcommon/ftcommoncapturecaptain.c": "offset_add[capture_fp->fkind]",
            "ft/ftcommon/ftcommoncaptureyoshi.c": "dFTCommonYoshiEggDamageCollDescs[fp->fkind]",
            "ft/ftcommon/ftcommondownwaitbounce.c": "dFTCommonDataDownBounceSFX[fp->fkind]",
            "ft/ftcommon/ftcommonthrow.c": "this_fp->attr->thrown_status[catch_fp->fkind]"}
        for relative, unsafe in rows.items():
            with self.subTest(relative=relative):
                original = (root / relative).read_bytes()
                text = lifecycle.transform(relative, original.decode())
                self.assertNotIn(unsafe, text)
                self.assertEqual(hashlib.sha256((root / relative).read_bytes()).digest(),
                                 hashlib.sha256(original).digest())

    def test_changed_source_or_unowned_file_fails_closed(self):
        with self.assertRaises(lifecycle.LifecycleError):
            lifecycle.transform("ft/ftchar/ftkirby/ftkirbyspecialn.c", "changed body")
        with self.assertRaises(lifecycle.LifecycleError):
            lifecycle.transform("ft/ftmain.c", "unowned body")

    def test_crlf_and_lf_sources_generate_identical_semantics(self):
        source = "else x = copy[victim_fp->fkind].copy_id;\n"
        self.assertEqual(lifecycle.transform("ftkirbyspecialn.c", source),
                         lifecycle.transform("ftkirbyspecialn.c", source.replace("\n", "\r\n")))

    def test_float_macros_are_valid_c_literals_and_policy_has_own_identity(self):
        metadata = {"extra_pin": "a" * 40, "rom_sha256": "b" * 64,
                    "model_scale_bits": 0x3FA147AE, "copy_id": 8, "hat_id": 0,
                    "electric_family": 24,
                    "star_scale_bits": 0x3FCCCCCD, "star_damage": 17,
                    "results": {"name": "META KNIGHT", "name_x": 20.0,
                                "name_scale": .55, "wins_x": 180.0},
                    "costumes": [0, 1, 4, 5, 2, 0, 3]}
        metadata["victim_lookup"] = {"lookup_kind": 10, "down_bounce": {"fgm": 306}}
        header = lifecycle.header(metadata)
        self.assertIn("20.00000000F", header)
        self.assertIn('"META KNIGHT"', header)
        self.assertIn("NDS_META_KIRBY_COPY_ID 8u", header)
        self.assertIn("NDS_META_KIRBY_HAT_ID 0u", header)
        self.assertIn("NDS_META_INHERITED_VICTIM_LOOKUP_KIND 10u", header)
        self.assertIn("NDS_META_DOWN_BOUNCE_FGM 306u", header)


class InheritedVictimContractTests(unittest.TestCase):
    def setUp(self):
        self.donor_id = 3
        self.rom = bytearray(0x02C00000 + 0x1000)
        self.labels = {"Character." + name + ".table": 0x80400000 + offset
                       for name, offset in (("f_thrown_action", 0x100), ("b_thrown_action", 0x200),
                                            ("falcon_dive_id", 0x300), ("yoshi_egg", 0x400),
                                            ("down_bound_fgm", 0x800))}
        self.source = "\n".join((
            "add_to_id_table(f_thrown_action, id.{name}, id.{parent})",
            "add_to_id_table(b_thrown_action, id.{name}, id.{parent})",
            "add_to_id_table(falcon_dive_id, id.{name}, id.{parent})",
            "add_to_table(yoshi_egg, id.{name}, id.{parent}, 0x1C)",
            "add_to_table(down_bound_fgm, id.{name}, id.{parent}, 0x2)"))
        for name in ("f_thrown_action", "b_thrown_action", "falcon_dive_id"):
            self.write(name, self.donor_id, 4, (10).to_bytes(4, "big"))
        for kind in (10, self.donor_id):
            self.write("yoshi_egg", kind, 28, bytes(range(28)))
            self.write("down_bound_fgm", kind, 2, (306).to_bytes(2, "big"))

    def write(self, name, kind, width, value):
        at = self.labels["Character." + name + ".table"] - 0x80400000 + 0x02C00000 + kind * width
        self.rom[at:at + width] = value

    def test_explicit_aliases_and_equal_egg_rows_qualify_only_column10(self):
        data = lifecycle.inherited_victim_metadata(self.rom, self.labels, self.donor_id, self.source)
        self.assertEqual(data["lookup_kind"], 10)
        self.assertEqual([row["value"] for row in data["aliases"]], [10, 10, 10])
        self.assertEqual(data["down_bounce"]["fgm"], 306)

    def test_missing_source_contract_and_wrong_alias_reject(self):
        with self.assertRaises(lifecycle.LifecycleError):
            lifecycle.inherited_victim_metadata(self.rom, self.labels, self.donor_id,
                                                 self.source.replace("add_to_id_table(falcon_dive_id", "// disabled(falcon_dive_id"))
        self.write("f_thrown_action", self.donor_id, 4, (9).to_bytes(4, "big"))
        with self.assertRaises(lifecycle.LifecycleError):
            lifecycle.inherited_victim_metadata(self.rom, self.labels, self.donor_id, self.source)

    def test_overridden_egg_and_changed_bounce_cue_reject(self):
        self.write("yoshi_egg", self.donor_id, 28, bytes(28))
        with self.assertRaises(lifecycle.LifecycleError):
            lifecycle.inherited_victim_metadata(self.rom, self.labels, self.donor_id, self.source)
        self.write("yoshi_egg", self.donor_id, 28, bytes(range(28)))
        self.write("down_bound_fgm", self.donor_id, 2, (307).to_bytes(2, "big"))
        with self.assertRaises(lifecycle.LifecycleError):
            lifecycle.inherited_victim_metadata(self.rom, self.labels, self.donor_id, self.source)

    def test_unclassified_or_truncated_linked_table_rejects(self):
        labels = dict(self.labels, **{"Character.falcon_dive_id.table": 0x80300000})
        with self.assertRaises(lifecycle.LifecycleError):
            lifecycle.inherited_victim_metadata(self.rom, labels, self.donor_id, self.source)
        with self.assertRaises(lifecycle.LifecycleError):
            lifecycle.inherited_victim_metadata(self.rom[:0x02C00100], self.labels, self.donor_id, self.source)


class ActualRecordCodecTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path("C:/msys64/ucrt64/bin/gcc.exe")
        if os.name != "nt" or not compiler.is_file():
            raise unittest.SkipTest("Windows host compiler unavailable for actual codec")
        cls.temp = tempfile.TemporaryDirectory(prefix="meta-codec-host-")
        folder = Path(cls.temp.name)
        source = folder / "codec_probe.c"
        source.write_text('#include <nds/nds_meta_record_codec.h>\n'
                          '__declspec(dllexport) void encode(u8 *r,const u8 *p) { ndsMetaRecordEncode(r,p); }\n'
                          '__declspec(dllexport) int decode(const u8 *r,u8 *p) { return ndsMetaRecordDecode(r,p); }\n',
                          encoding="utf-8")
        library = folder / "codec_probe.dll"
        environment = os.environ.copy()
        environment["PATH"] = str(compiler.parent) + os.pathsep + environment.get("PATH", "")
        built = subprocess.run([str(compiler), "-shared", "-O2", "-I" + str(lifecycle.ROOT / "include"),
                                str(source), "-o", str(library)], text=True, capture_output=True,
                               env=environment)
        if built.returncode:
            raise RuntimeError("host codec compile failed:\n" + built.stdout + built.stderr)
        cls.lib = ctypes.CDLL(str(library))
        pointer = ctypes.POINTER(ctypes.c_uint8)
        cls.lib.encode.argtypes = [pointer, pointer]
        cls.lib.decode.argtypes = [pointer, pointer]
        cls.lib.decode.restype = ctypes.c_int
        cls.Record = ctypes.c_uint8 * 188
        cls.Payload = ctypes.c_uint8 * 170

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, "lib"):
            import _ctypes
            _ctypes.FreeLibrary(cls.lib._handle)
            del cls.lib
            cls.temp.cleanup()

    def encoded(self):
        payload = self.Payload(*[(index * 37) & 255 for index in range(170)])
        record = self.Record()
        self.lib.encode(record, payload)
        return record, bytes(payload)

    def test_cold_roundtrip_retains_the_own_record_bytes(self):
        record, expected = self.encoded()
        payload = self.Payload()
        self.assertEqual(self.lib.decode(record, payload), 1)
        self.assertEqual(bytes(payload), expected)

    def test_old_save_has_no_meta_record_and_keeps_all_legacy_bytes(self):
        # Actual ARM32 oracle: copy0=1516; copy1 begins1520 and ends3036.
        # Own source-derived tail begins3040, with copies3040 and3232.
        image = bytearray((index * 11) & 255 for index in range(4096))
        legacy_before = bytes(image[:3036])
        image[3040:3420] = bytes(380)
        payload = self.Payload(*([0xA5] * 170))
        self.assertEqual(self.lib.decode(self.Record.from_buffer_copy(image[3040:3228]), payload), 0)
        self.assertEqual(bytes(payload), bytes([0xA5] * 170))
        self.assertEqual(bytes(image[:3036]), legacy_before)
        new, _ = self.encoded()
        image[3040:3228] = bytes(new)
        image[3232:3420] = bytes(new)
        self.assertEqual(bytes(image[:3036]), legacy_before)

    def test_each_single_bit_corruption_is_rejected_without_publishing_payload(self):
        record, _ = self.encoded()
        original = bytes(record)
        for byte in range(188):
            for bit in range(8):
                changed = bytearray(original)
                changed[byte] ^= 1 << bit
                payload = self.Payload(*([0xA5] * 170))
                self.assertEqual(self.lib.decode(self.Record.from_buffer_copy(changed), payload), 0)
                self.assertEqual(bytes(payload), bytes([0xA5] * 170))

    def test_copy1_recovers_when_copy0_is_corrupt(self):
        record, expected = self.encoded()
        corrupted = self.Record.from_buffer_copy(bytes(record))
        corrupted[19] ^= 1
        payload = self.Payload()
        valid = self.lib.decode(corrupted, payload)
        if not valid:
            valid = self.lib.decode(record, payload)
        self.assertEqual(valid, 1)
        self.assertEqual(bytes(payload), expected)


if __name__ == "__main__":
    unittest.main()
