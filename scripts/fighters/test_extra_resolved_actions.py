#!/usr/bin/env python3
"""Linked-table admission fixtures; no donor assembly or DS build is executed."""

from __future__ import annotations

import hashlib
from pathlib import Path
import struct
import tempfile
import unittest

import extra_resolved_actions as export


# Independent wire fixture: Character.asm uses 0x14-byte actions, 0xC-byte
# parameters, 0xDC shared actions, and fifteen menu actions. The fixture keeps
# all these tables separate so a wrong origin or table stride changes evidence.
HOOK_SPECS = (
    ("ground_nsp", 4), ("air_nsp", 4),
    ("ground_usp", 4), ("air_usp", 4),
    ("ground_dsp", 4), ("air_dsp", 4),
    ("ai_behaviour", 4), ("on_action_changed", 4),
    ("initial_script", 4), ("grounded_script", 4),
    ("action_replace_map", 4), ("custom_capture_dk_interrupt", 4),
    ("entry_script", 4), ("static_part", 8),
    ("crowd_chant_fgm", 2), ("kirby_inhale_struct", 12),
    ("default_costume", 8), ("costume_shield_color", 4),
)


class LinkedImage:
    CHAR = 0x100
    PARAMS = 0x200
    SPECIALS = 0x240
    SHARED = 0x400
    MENU = 0x1600
    HOOKS = 0x1800
    EXPORT = 0x2000
    DIRECTORY = EXPORT + 60
    KNOWN_CALLBACK = 0x800D94C4
    CUSTOM_CALLBACK = 0x80402000
    UNKNOWN_CALLBACK = 0x800DDE00

    def __init__(self) -> None:
        self.rom = bytearray(self.DIRECTORY + 18 * 12)
        self.rom[:4] = bytes.fromhex("80371240")
        self.rom[self.EXPORT:self.EXPORT + 8] = b"S64P4EX1"
        struct.pack_into(
            ">13I", self.rom, self.EXPORT + 8,
            1, self.EXPORT, self.CHAR, self.PARAMS, 4,
            self.SPECIALS, 3, self.SHARED, 0xDC,
            self.MENU, 15, 0x62, 18,
        )
        struct.pack_into(
            ">9I", self.rom, self.CHAR,
            0x900, 0x901, 0x902, 0x903, 0x904, 0, 0, 0, 0,
        )
        struct.pack_into(
            ">4I", self.rom, self.CHAR + 0x60,
            0x3FFFC, 0x80400200, 0x80401600, 4,
        )
        for index, row in enumerate((
            (0xA00, 0x80410000, 0x10000000),
            (0xA01, 0x80410040, 0x50000000),
            (0xA02, 0x80000000, 0x1F000000),
            (0xA03, 0x80410080, 0x5F000000),
        )):
            struct.pack_into(">III", self.rom, self.PARAMS + index * 12, *row)
        for index in range(15):
            struct.pack_into(
                ">III", self.rom, self.MENU + index * 12,
                0xB00 + index, 0x80420000 + index * 4, 0x10800000 + index,
            )

        self.action(self.SHARED, 0, 2, 0x3F, 0xA5A5,
                    self.KNOWN_CALLBACK, 0, self.UNKNOWN_CALLBACK, 0)
        self.action(self.SHARED, 27, 1, 5, 0x0102,
                    0, self.UNKNOWN_CALLBACK, 0, self.KNOWN_CALLBACK)
        self.action(self.SHARED, 0xDB, 0x3FF, 0x0B, 0x8000, 0, 0, 0, 0)
        self.action(self.SPECIALS, 0, 3, 0x1D, 0xBEEF,
                    self.CUSTOM_CALLBACK, self.KNOWN_CALLBACK,
                    self.UNKNOWN_CALLBACK, 0)
        self.action(self.SPECIALS, 1, 1, 0, 0, 0, 0, 0, 0)
        self.action(self.SPECIALS, 2, 0, 1, 0x1234, 0, 0, 0, 0)

        for index, (_, size) in enumerate(HOOK_SPECS):
            offset = self.HOOKS + index * 16
            struct.pack_into(">III", self.rom, self.DIRECTORY + index * 12,
                             index, offset, size)
            self.rom[offset:offset + size] = bytes(
                (0x80 + index + byte) & 0xFF for byte in range(size))

    def header_word(self, index: int, value: int) -> None:
        struct.pack_into(">I", self.rom, self.EXPORT + 8 + index * 4, value)

    def action(self, table: int, index: int, parameter: int, stale: int,
               flags: int, *callbacks: int) -> None:
        struct.pack_into(">HHIIII", self.rom, table + index * 20,
                         (parameter << 6) | stale, flags, *callbacks)

    def decode(self, symbols: dict[int, list[str]] | None = None) -> dict:
        return export.decode_export(bytes(self.rom), symbols or {})


class DecodeTests(unittest.TestCase):
    def test_shared_and_character_tables_preserve_linked_inheritance(self) -> None:
        image = LinkedImage()
        result = image.decode()
        actions = result["actions"]
        self.assertEqual(len(actions), 0xDC + 3)
        self.assertEqual([row["status_id"] for row in actions],
                         list(range(0xDC + 3)))
        self.assertEqual(actions[27]["parameter_index"], 1)
        self.assertEqual(actions[27]["callbacks_donor"], {
            "update": 0, "interrupt": image.UNKNOWN_CALLBACK,
            "physics": 0, "map": image.KNOWN_CALLBACK,
        })
        self.assertEqual(actions[0xDC]["parameter_index"], 3)
        self.assertEqual(actions[0xDC]["callbacks_donor"], {
            "update": image.CUSTOM_CALLBACK, "interrupt": image.KNOWN_CALLBACK,
            "physics": image.UNKNOWN_CALLBACK, "map": 0,
        })
        self.assertEqual(actions[-1]["parameter_index"], 0)
        self.assertEqual(result["core_file_ids"],
                         [0x900, 0x901, 0x902, 0x903, 0x904, 0, 0, 0, 0])
        self.assertEqual(result["attribute_offset"], 0x3FFFC)
        self.assertEqual(result["donor_character_id"], 0x62)
        self.assertEqual(result["rom_sha256"],
                         hashlib.sha256(image.rom).hexdigest())
        self.assertEqual(result["admission"], "UNRESOLVED_NATIVE_CONVERSION")

    def test_packed_stale_id_does_not_bleed_into_parameter_or_flags(self) -> None:
        actions = LinkedImage().decode()["actions"]
        self.assertEqual((actions[0]["parameter_index"], actions[0]["stale_id"],
                          actions[0]["unknown_flags"]), (2, 63, 0xA5A5))
        self.assertEqual((actions[0xDC]["parameter_index"],
                          actions[0xDC]["stale_id"],
                          actions[0xDC]["unknown_flags"]), (3, 29, 0xBEEF))

    def test_both_no_parameter_sentinels_preserve_reserved_value(self) -> None:
        # The pinned Character.asm explicitly reserves both 0x3FE and 0x3FF.
        # Exercise each sentinel in each table, including all six staling bits.
        for table in (LinkedImage.SHARED, LinkedImage.SPECIALS):
            for sentinel in (0x3FE, 0x3FF):
                with self.subTest(table=table, sentinel=sentinel):
                    image = LinkedImage()
                    image.action(table, 0, sentinel, 63, 0xABCD, 0, 0, 0, 0)
                    status = 0 if table == image.SHARED else 0xDC
                    row = image.decode()["actions"][status]
                    self.assertIsNone(row["parameter_index"])
                    self.assertEqual(row["parameter_sentinel"], sentinel)
                    self.assertEqual(row["stale_id"], 63)
                    self.assertEqual(row["unknown_flags"], 0xABCD)

    def test_no_script_sentinel_and_raw_parameter_values(self) -> None:
        parameters = LinkedImage().decode()["parameters"]
        self.assertEqual(parameters[2], {
            "index": 2, "animation_file_id": 0xA02,
            "script_donor_value": 0x80000000, "flags": 0x1F000000,
            "script_state": "no_script",
        })
        self.assertEqual(parameters[3]["script_donor_value"], 0x80410080)
        self.assertEqual(parameters[3]["script_state"], "unconverted")

    def test_callback_aliases_are_deduplicated_and_unknowns_stay_unresolved(self) -> None:
        image = LinkedImage()
        symbols = export.parse_symbols(
            "800D94C4 ftCommonUpdate\n"
            "800d94c4 Alias.common_update\n"
            "800D94C4 ftCommonUpdate\n"
            "80402000 MetaKnightSpecial.USP.main\n"
        )
        callbacks = image.decode(symbols)["callbacks"]
        self.assertEqual([row["donor_address"] for row in callbacks],
                         sorted((image.KNOWN_CALLBACK, image.CUSTOM_CALLBACK,
                                 image.UNKNOWN_CALLBACK)))
        by_address = {row["donor_address"]: row for row in callbacks}
        self.assertEqual(by_address[image.KNOWN_CALLBACK]["symbols"],
                         ["Alias.common_update", "ftCommonUpdate"])
        self.assertEqual(by_address[image.UNKNOWN_CALLBACK]["symbols"], [])
        self.assertTrue(all(row["native_conversion"] == "unresolved"
                            for row in callbacks))
        self.assertNotIn(0, by_address)

    def test_menu_and_mixed_width_hooks_remain_big_endian_donor_data(self) -> None:
        image = LinkedImage()
        result = image.decode()
        self.assertEqual(result["menu_parameters"][14], {
            "animation_file_id": 0xB0E, "script_donor_value": 0x80420038,
            "flags": 0x1080000E,
        })
        for index, (name, size) in enumerate(HOOK_SPECS):
            with self.subTest(hook=name):
                row = result["character_hooks"][index]
                offset = image.HOOKS + index * 16
                self.assertEqual(row["name"], name)
                self.assertEqual(row["rom_offset"], offset)
                self.assertEqual(row["bytes_big_endian"],
                                 bytes(image.rom[offset:offset + size]).hex())
                self.assertEqual(row["native_conversion"], "unresolved")

    def test_wrong_export_version_and_location_are_rejected(self) -> None:
        for field, value in ((0, 2), (1, LinkedImage.EXPORT + 4)):
            with self.subTest(field=field):
                image = LinkedImage()
                image.header_word(field, value)
                with self.assertRaisesRegex(ValueError, "version/location mismatch"):
                    image.decode()

    def test_missing_duplicate_and_truncated_metadata_are_rejected(self) -> None:
        image = LinkedImage()
        cases = (
            (bytes(image.rom[:image.EXPORT]), "expected one review export, found 0"),
            (bytes(image.rom) + b"S64P4EX1", "expected one review export, found 2"),
            (bytes(image.rom[:image.EXPORT + 59]), "review header span outside"),
            (bytes(image.rom[:-1]), "hook directory span outside"),
        )
        for rom, error in cases:
            with self.subTest(error=error), self.assertRaisesRegex(ValueError, error):
                export.decode_export(rom, {})

    def test_incompatible_pinned_table_shapes_and_counts_are_rejected(self) -> None:
        for field, value in ((8, 0xDB), (10, 14), (12, 17),
                             (4, 0), (4, 0x3FF), (6, 0), (6, 0x3FF)):
            with self.subTest(field=field, value=value):
                image = LinkedImage()
                image.header_word(field, value)
                with self.assertRaises(ValueError):
                    image.decode()

    def test_truncated_or_unaligned_referenced_table_spans_are_rejected(self) -> None:
        for field, label in ((2, "character descriptor"), (3, "parameters"),
                             (5, "actions"), (7, "actions"),
                             (9, "menu parameters")):
            for offset in (0xFFFFFFFC, 1):
                with self.subTest(field=field, offset=offset):
                    image = LinkedImage()
                    image.header_word(field, offset)
                    with self.assertRaisesRegex(ValueError, label):
                        image.decode()

    def test_descriptor_count_and_out_of_range_parameter_ids_are_rejected(self) -> None:
        image = LinkedImage()
        struct.pack_into(">I", image.rom, image.CHAR + 0x6C, 5)
        with self.assertRaisesRegex(ValueError, "parameter count disagrees"):
            image.decode()
        for table, status in ((LinkedImage.SHARED, 0),
                              (LinkedImage.SPECIALS, 0xDC)):
            with self.subTest(table=table):
                image = LinkedImage()
                image.action(table, 0, 4, 0, 0, 0, 0, 0, 0)
                with self.assertRaisesRegex(
                        ValueError, f"action {status}: invalid parameter index 4"):
                    image.decode()

    def test_bad_hook_identity_width_and_span_are_rejected(self) -> None:
        for identity, offset, size, error in (
            (18, LinkedImage.HOOKS, 4, "unexpected hook identity/width"),
            (0, LinkedImage.HOOKS, 8, "unexpected hook identity/width"),
            (0, 0xFFFFFFFC, 4, "ground_nsp span outside"),
            (0, LinkedImage.HOOKS + 1, 4, "invalid ground_nsp span"),
        ):
            with self.subTest(identity=identity, offset=offset, size=size):
                image = LinkedImage()
                struct.pack_into(">III", image.rom, image.DIRECTORY,
                                 identity, offset, size)
                with self.assertRaisesRegex(ValueError, error):
                    image.decode()


class SymbolTests(unittest.TestCase):
    def test_malformed_symbol_lines_are_rejected(self) -> None:
        for line in ("800D94C ftCommon", "0x800D94C4 ftCommon",
                     "800D94C4", "800D94C4 ftCommon extra", "XYZD94C4 ftCommon"):
            with self.subTest(line=line), self.assertRaisesRegex(
                    ValueError, "unsupported Bass symbol line"):
                export.parse_symbols(line)

    def test_conflicting_names_are_rejected_and_blank_lines_are_allowed(self) -> None:
        with self.assertRaisesRegex(ValueError, "conflicting Bass symbol"):
            export.parse_symbols("800D94C4 ftCommon\n80402000 ftCommon\n")
        self.assertEqual(export.parse_symbols("\n \t\n800D94C4 ftCommon \n"),
                         {0x800D94C4: ["ftCommon"]})


class InstallationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(prefix="extra-review-export-")
        self.root = Path(self.temp.name)

    def tearDown(self) -> None:
        self.temp.cleanup()

    def test_outside_builds_destinations_are_rejected_before_any_write(self) -> None:
        for stage in (self.root / "decomp/stage", self.root / "builds-old/stage",
                      self.root / "builds/../escape"):
            with self.subTest(stage=stage):
                stage.mkdir(parents=True, exist_ok=True)
                main = stage / "main.asm"
                main.write_text("owner source\n", encoding="utf-8")
                with self.assertRaisesRegex(ValueError, "inside this checkout's builds"):
                    export.install_review_export(stage, self.root)
                self.assertEqual(main.read_text(encoding="utf-8"), "owner source\n")
                self.assertFalse((stage / export.REVIEW_INCLUDE).exists())

    def test_unstaged_and_ambiguous_includes_are_rejected(self) -> None:
        stage = self.root / "builds/review"
        stage.mkdir(parents=True)
        with self.assertRaisesRegex(ValueError, "pinned EXTRA appender"):
            export.install_review_export(stage, self.root)
        main = stage / "main.asm"
        for content in (
            'include "p4_native_review_export.asm"\n' * 2,
            'include "different/p4_native_review_export.asm"\n',
        ):
            with self.subTest(content=content):
                main.write_text(content, encoding="utf-8")
                with self.assertRaisesRegex(ValueError, "ambiguous existing"):
                    export.install_review_export(stage, self.root)
                self.assertEqual(main.read_text(encoding="utf-8"), content)
                self.assertFalse((stage / export.REVIEW_INCLUDE).exists())

    def test_symlinked_files_cannot_escape_builds_or_mutate_reference(self) -> None:
        for name in ("main.asm", export.REVIEW_INCLUDE):
            with self.subTest(name=name):
                stage = self.root / "builds" / name.replace(".", "-")
                stage.mkdir(parents=True)
                target = self.root / ("owner-" + name)
                target.write_text("owner bytes\n", encoding="utf-8")
                link = stage / name
                try:
                    link.symlink_to(target)
                except OSError as error:
                    self.skipTest(f"file symlinks unavailable: {error}")
                main = stage / "main.asm"
                if name != "main.asm":
                    main.write_text("donor padding\n", encoding="utf-8")
                original_main = main.read_bytes()
                with self.assertRaisesRegex(ValueError, "symlink"):
                    export.install_review_export(stage, self.root)
                self.assertEqual(target.read_text(encoding="utf-8"), "owner bytes\n")
                self.assertEqual(main.read_bytes(), original_main)

    def test_installation_appends_after_padding_and_is_idempotent(self) -> None:
        stage = self.root / "builds/review"
        stage.mkdir(parents=True)
        main = stage / "main.asm"
        original = "endian msb\nfill 0x4000000 - origin()\n"
        main.write_text(original, encoding="utf-8")
        output = export.install_review_export(stage, self.root)
        installed = main.read_bytes()
        self.assertTrue(installed.decode("utf-8").startswith(original))
        self.assertTrue(installed.decode("utf-8").endswith(
            '\ninclude "p4_native_review_export.asm"\n'))
        self.assertEqual(output, stage.resolve() / export.REVIEW_INCLUDE)
        self.assertTrue(output.is_file())
        export.install_review_export(stage, self.root)
        self.assertEqual(main.read_bytes(), installed)


if __name__ == "__main__":
    unittest.main()
