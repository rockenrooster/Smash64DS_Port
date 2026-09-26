"""Focused tests for the final-ELF frontend overlay relocation gate."""

from __future__ import annotations

import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import check_frontend_overlay as checker  # noqa: E402


def _string_table(names):
    data = bytearray(b"\0")
    offsets = {"": 0}
    for name in names:
        if name in offsets:
            continue
        offsets[name] = len(data)
        data.extend(name.encode("utf-8") + b"\0")
    return bytes(data), offsets


def _symbol(name_offset, value, size, binding, kind, section_index):
    return struct.pack("<IIIBBH", name_offset, value, size,
                       (binding << 4) | kind, 0, section_index)


def make_elf(*, include_overlay=True, overlay_address=0x2000,
             overlay_size=32, rel_text=None, rel_rodata=None,
             rel_overlay=None, rel_debug=None, rela_text=None,
             resident_function_value=0x1000, text_size=4,
             frontend_entry_section=3, text_data=None):
    """Create a tiny linked ELF32 ARM image with real SHT_REL/SHT_RELA tables."""
    rel_text = [(0x1000, (3 << 8) | 2)] if rel_text is None else rel_text
    rel_rodata = [(0x1800, (4 << 8) | 2)] if rel_rodata is None else rel_rodata
    rel_overlay = [(0x2000, (3 << 8) | 2)] if rel_overlay is None else rel_overlay
    rel_debug = [(0x3000, (3 << 8) | 2)] if rel_debug is None else rel_debug

    names = [
        "resident_func", "resident_table", "frontend_entry", "frontend_table",
        "", "resident-to-frontend", "frontend-self", "debug-to-frontend",
    ]
    strings, str_offsets = _string_table(names)
    symbol_data = b"".join((
        _symbol(0, 0, 0, 0, 0, 0),
        _symbol(str_offsets["resident_func"], resident_function_value, 4, 1, 2, 1),
        _symbol(str_offsets["resident_table"], 0x1800, 4, 1, 1, 2),
        _symbol(str_offsets["frontend_entry"], overlay_address, 4, 1, 2,
                frontend_entry_section),
        _symbol(str_offsets["frontend_table"], overlay_address + 4, 4, 1, 1, 3),
        _symbol(0, overlay_address, 0, 0, 3, 3),  # section symbol (not allowlistable)
    ))

    def rel_bytes(records):
        return b"".join(struct.pack("<II", offset, info)
                        for offset, info in records)

    def rela_bytes(records):
        return b"".join(struct.pack("<IIi", offset, info, addend)
                        for offset, info, addend in records)

    section_defs = [
        {"name": "", "type": 0, "flags": 0, "addr": 0, "data": b"",
         "size": 0, "link": 0, "info": 0, "align": 0, "entsize": 0},
        {"name": ".text", "type": 1, "flags": 0x6, "addr": 0x1000,
         "data": b"\0" * text_size if text_data is None else text_data,
         "link": 0, "info": 0, "align": 4, "entsize": 0},
        {"name": ".rodata", "type": 1, "flags": 0x2, "addr": 0x1800,
         "data": b"\0\0\0\0", "link": 0, "info": 0, "align": 4, "entsize": 0},
        {"name": ".ovl.frontend" if include_overlay else ".other.frontend",
         "type": 1, "flags": 0x6, "addr": overlay_address,
         "data": b"\0" * overlay_size, "size": overlay_size,
         "link": 0, "info": 0, "align": 4, "entsize": 0},
        {"name": ".rel.text", "type": 9, "flags": 0, "addr": 0,
         "data": rel_bytes(rel_text), "link": 9, "info": 1, "align": 4, "entsize": 8},
        {"name": ".rel.rodata", "type": 9, "flags": 0, "addr": 0,
         "data": rel_bytes(rel_rodata), "link": 9, "info": 2, "align": 4, "entsize": 8},
        {"name": ".rel.ovl.frontend", "type": 9, "flags": 0, "addr": 0,
         "data": rel_bytes(rel_overlay), "link": 9, "info": 3, "align": 4, "entsize": 8},
        {"name": ".debug_info", "type": 1, "flags": 0, "addr": 0,
         "data": b"\0\0\0\0", "link": 0, "info": 0, "align": 1, "entsize": 0},
        {"name": ".rel.debug_info", "type": 9, "flags": 0, "addr": 0,
         "data": rel_bytes(rel_debug), "link": 9, "info": 7, "align": 4, "entsize": 8},
        {"name": ".symtab", "type": 2, "flags": 0, "addr": 0,
         "data": symbol_data, "link": 10, "info": 1, "align": 4, "entsize": 16},
        {"name": ".strtab", "type": 3, "flags": 0, "addr": 0,
         "data": strings, "link": 0, "info": 0, "align": 1, "entsize": 0},
    ]
    if rela_text is not None:
        section_defs[4]["type"] = 4
        section_defs[4]["data"] = rela_bytes(rela_text)
        section_defs[4]["entsize"] = 12

    shstr, shname_offsets = _string_table([item["name"] for item in section_defs] +
                                          [".shstrtab"])
    section_defs.append({"name": ".shstrtab", "type": 3, "flags": 0,
                         "addr": 0, "data": shstr, "link": 0, "info": 0,
                         "align": 1, "entsize": 0})
    shstr, shname_offsets = _string_table([item["name"] for item in section_defs])
    section_defs[-1]["data"] = shstr

    image = bytearray(b"\0" * 52)
    section_offsets = []
    for item in section_defs:
        while len(image) % max(1, item["align"]):
            image.append(0)
        offset = len(image)
        payload = item["data"]
        image.extend(payload)
        section_offsets.append(offset)

    while len(image) % 4:
        image.append(0)
    shoff = len(image)
    for index, item in enumerate(section_defs):
        size = item.get("size", len(item["data"]))
        header = (
            shname_offsets[item["name"]], item["type"], item["flags"],
            item["addr"], section_offsets[index], size, item["link"],
            item["info"], item["align"], item["entsize"],
        )
        image.extend(struct.pack("<IIIIIIIIII", *header))

    ident = bytearray(16)
    ident[:4] = checker.ELF_MAGIC
    ident[4] = checker.ELFCLASS32
    ident[5] = checker.ELFDATA2LSB
    ident[6] = 1
    elf_header = struct.pack(
        "<HHIIIIIHHHHHH", checker.ET_EXEC, checker.EM_ARM, 1,
        0x1000, 0, shoff, 0, 52, 0, 0, 40, len(section_defs),
        len(section_defs) - 1,
    )
    image[:16] = ident
    image[16:52] = elf_header
    return bytes(image)


class FrontendOverlayTests(unittest.TestCase):
    def inspect(self, **kwargs):
        return checker.inspect_elf(make_elf(**kwargs), elf_name="fixture.elf",
                                   required=True)

    def test_thumb_function_entry_uses_code_address(self):
        report = self.inspect(resident_function_value=0x1001, rel_rodata=[])
        self.assertEqual(report["unknown_crossing_pairs"][0]["source_symbol"],
                         "resident_func")

    def test_thumb_end_does_not_authorize_adjacent_data(self):
        report = checker.inspect_elf(
            make_elf(resident_function_value=0x1001, text_size=8,
                     rel_text=[(0x1004, (3 << 8) | 2)], rel_rodata=[]),
            elf_name="fixture.elf", required=True,
            allowed_pairs={("resident_func", "frontend_entry")},
        )
        self.assertFalse(report["passed"])
        self.assertEqual(report["allowed_crossing_pairs"], [])
        self.assertNotEqual(report["unknown_crossing_pairs"][0]["source_symbol"],
                            "resident_func")

    def test_absolute_alias_into_overlay_cannot_hide_crossing(self):
        report = self.inspect(frontend_entry_section=0xfff1, rel_rodata=[])
        self.assertFalse(report["passed"])
        self.assertEqual(report["unknown_crossing_pairs"][0]["target_symbol"],
                         "frontend_entry")

    def test_resolved_abs32_addend_into_overlay_cannot_hide_crossing(self):
        report = self.inspect(
            rel_text=[(0x1000, (2 << 8) | 2)], rel_rodata=[],
            text_data=struct.pack("<I", 0x2004),
        )
        self.assertFalse(report["passed"])
        self.assertEqual(len(report["unknown_crossing_pairs"]), 1)

    def test_exact_relocations_are_gated_and_reported_completely(self):
        report = self.inspect()
        self.assertFalse(report["passed"])
        self.assertEqual(report["overlay_bytes"], 32)
        self.assertEqual(report["resident_alloc_relocations"], 2)
        self.assertEqual(
            [(pair["source_symbol"], pair["target_symbol"], pair["count"])
             for pair in report["unknown_crossing_pairs"]],
            [("resident_func", "frontend_entry", 1),
             ("resident_table", "frontend_table", 1)],
        )
        # Overlay-origin and debug-origin relocations are intentionally ignored.
        self.assertEqual(report["allowed_crossing_pairs"], [])
        for pair in report["unknown_crossing_pairs"]:
            self.assertEqual(len(pair["relocations"]), pair["count"])
            self.assertEqual(pair["relocations"][0]["relocation_type"], 2)

    def test_only_exact_allowlisted_symbol_pair_passes(self):
        allowed = {("resident_func", "frontend_entry")}
        report = checker.inspect_elf(
            make_elf(), elf_name="fixture.elf", allowed_pairs=allowed,
            allowlist_reasons={("resident_func", "frontend_entry"): "reviewed"},
            required=True,
        )
        self.assertFalse(report["passed"])
        self.assertEqual(len(report["allowed_crossing_pairs"]), 1)
        self.assertEqual(report["allowed_crossing_pairs"][0]["reason"], "reviewed")
        self.assertEqual(
            [(pair["source_symbol"], pair["target_symbol"])
             for pair in report["unknown_crossing_pairs"]],
            [("resident_table", "frontend_table")],
        )

    def test_section_symbol_target_is_not_allowlistable(self):
        report = self.inspect(rel_text=[(0x1000, (5 << 8) | 2)],
                              rel_rodata=[])
        self.assertFalse(report["passed"])
        self.assertEqual(report["unknown_crossing_pairs"][0]["target_symbol"],
                         "<section:.ovl.frontend>")
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "allow.json"
            path.write_text(json.dumps({"schema_version": 1, "pairs": [{
                "source_symbol": "resident_func",
                "target_symbol": "<section:.ovl.frontend>",
            }]}), encoding="utf-8")
            with self.assertRaises(checker.CheckerError):
                checker.load_allowlist(path)

    def test_required_overlay_checks_presence_size_and_alignment(self):
        missing = checker.inspect_elf(make_elf(include_overlay=False),
                                      elf_name="missing.elf", required=True)
        self.assertFalse(missing["passed"])
        self.assertFalse(missing["overlay_present"])
        self.assertEqual(missing["overlay_bytes"], 0)
        self.assertTrue(any("is missing" in error for error in missing["errors"]))

        empty = self.inspect(overlay_size=0)
        self.assertTrue(any("is empty" in error for error in empty["errors"]))

        misaligned = self.inspect(overlay_address=0x2002)
        self.assertTrue(any("not aligned" in error for error in misaligned["errors"]))

        bad_end = self.inspect(overlay_size=8)
        self.assertTrue(any("end address" in error for error in bad_end["errors"]))

    def test_required_fails_when_linked_relocations_are_absent(self):
        report = self.inspect(rel_text=[], rel_rodata=[], rel_overlay=[], rel_debug=[])
        self.assertFalse(report["passed"])
        self.assertTrue(any("--emit-relocs" in error for error in report["errors"]))

    def test_rela_addends_and_exact_allowlist_json(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "allow.json"
            path.write_text(json.dumps({
                "schema_version": 1,
                "pairs": [{
                    "source_symbol": "resident_func",
                    "target_symbol": "frontend_entry",
                    "reason": "lifecycle-reviewed fixture edge",
                }],
            }), encoding="utf-8")
            allowed, reasons = checker.load_allowlist(path)
        report = checker.inspect_elf(
            make_elf(rel_text=[], rel_rodata=[],
                     rela_text=[(0x1000, (3 << 8) | 2, -12)]),
            elf_name="rela.elf", allowed_pairs=allowed,
            allowlist_reasons=reasons, required=True,
        )
        self.assertTrue(report["passed"])
        reloc = report["allowed_crossing_pairs"][0]["relocations"][0]
        self.assertEqual(reloc["addend"], -12)

    def test_allowlist_rejects_wildcard_pairs(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "allow.json"
            path.write_text(json.dumps({"schema_version": 1, "pairs": [{
                "source_symbol": "resident_*", "target_symbol": "frontend_entry",
            }]}), encoding="utf-8")
            with self.assertRaisesRegex(checker.CheckerError, "wildcards"):
                checker.load_allowlist(path)

    def test_cli_emits_json_with_bytes_and_unknown_pairs(self):
        script = ROOT / "scripts" / "check_frontend_overlay.py"
        with tempfile.TemporaryDirectory() as temp:
            elf = Path(temp) / "fixture.elf"
            elf.write_bytes(make_elf())
            result = subprocess.run(
                [sys.executable, str(script), str(elf), "--required"],
                capture_output=True, text=True, check=False,
            )
        self.assertEqual(result.returncode, 1, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report["overlay_bytes"], 32)
        self.assertEqual(len(report["unknown_crossing_pairs"]), 2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
