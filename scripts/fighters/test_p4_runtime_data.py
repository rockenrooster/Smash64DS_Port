#!/usr/bin/env python3
"""Admission faults and consuming-seam invariants for the native P4 adapter.

Host only; tests read frozen source/export and never generate production output.
"""
from __future__ import annotations

import copy
import json
from pathlib import Path
import unittest

import generate_p4_runtime_data as producer

ROOT = Path(__file__).resolve().parents[2]
DONOR = ROOT / "builds/p4/meta-knight-donor/extra"
EXPORT = ROOT / "builds/p4/meta-knight-donor/resolved-actions.json"


class RuntimeAdmission(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if not EXPORT.is_file():
            raise unittest.SkipTest("linked donor review export is not present")
        cls.source = json.loads(EXPORT.read_text(encoding="utf-8"))

    def test_every_linked_callback_resolves_to_a_typed_symbol(self) -> None:
        mapped = producer.callback_bindings(ROOT, self.source)
        self.assertEqual(len(mapped), 205)
        self.assertEqual(mapped[0x8013E700], "ftCommonTurnProcInterrupt")
        self.assertEqual(mapped[0x80596BE4], "ndsMetaKnightUSPUpdate")
        self.assertEqual(mapped[0x8051E2C4], "ndsMetaKnightAerialAttackPhysics")
        self.assertTrue(all(name.isidentifier() for name in mapped.values()))

    def test_unknown_callback_refuses_the_import(self) -> None:
        broken = copy.deepcopy(self.source)
        broken["callbacks"][0] = {
            "donor_address": 0x8FFFFFFC, "symbols": ["Unsupported.callback"]}
        with self.assertRaisesRegex(ValueError, "unsupported native callbacks"):
            producer.callback_bindings(ROOT, broken)

    def test_changed_character_hook_refuses_the_import(self) -> None:
        producer.validate_hook_inventory(self.source)
        broken = copy.deepcopy(self.source)
        broken["character_hooks"][0]["bytes_big_endian"] = "8FFFFFFC"
        with self.assertRaisesRegex(ValueError, "unqualified character hook semantics"):
            producer.validate_hook_inventory(broken)

    def test_adapters_keep_source_definition_tables_and_remove_new_kind_indexes(self) -> None:
        adapted = producer.transformed_imports(ROOT)
        manager = adapted["battleship_ftmanager.generated.inc"]
        main = adapted["battleship_ftmain.generated.inc"]
        computer = adapted["battleship_ftcomputer.generated.inc"]
        self.assertNotIn("dFTManagerDataFiles[", manager)
        self.assertIn("for (i = 0; i < nFTKindEnumCount; i++)", manager)
        self.assertIn("FTStatusDesc *dFTMainSpecialStatusDescs[", main)
        self.assertIn("ndsP4GetStatusDesc(fp->fkind, status_id)", main)
        self.assertIn("ndsP4GetThrownScriptColumn(fp->throw_fkind)", main)
        self.assertIn("FTComputerAttack *dFTComputerAttackList[", computer)
        self.assertNotIn("dFTComputerAttackList[this_fp->fkind]", computer)
        self.assertNotIn("dFTComputerPlayerInputScripts[index]", computer)

    def test_cpu_uses_own_source_ranges_and_facing_scripts(self) -> None:
        table = producer.cpu_attacks(DONOR)
        self.assertIn("{ 49, 9, 0, 213.0F, 561.0F, -60.0F, 288.0F }", table)
        self.assertIn("{ 50, 20, 0, -474.0F, -90.0F, 72.0F, 292.0F }", table)
        self.assertIn("{ 51, 7, 0, 373.0F, 959.0F, 195.0F, 323.0F }", table)
        self.assertEqual(table.count("{ -1,"), 2)
        scripts = producer.cpu_scripts()
        self.assertIn("FTCOMPUTER_EVENT_STICK_X(0x83, 0)", scripts)
        self.assertIn("FTCOMPUTER_EVENT_STICK_X(0x84, 0)", scripts)

    def test_source_drift_fails_before_transformation(self) -> None:
        with self.assertRaisesRegex(ValueError, "source drift"):
            producer.replace_exact("changed source", "expected seam", "replacement", 1)

    def test_cpu_constant_parser_cannot_execute_source(self) -> None:
        self.assertEqual(producer.integer_expression("28+6"), 34)
        self.assertEqual(producer.integer_expression("469-200"), 269)
        for text in ("open('secret')", "10*3", "True", "__import__('os')"):
            with self.assertRaises(ValueError):
                producer.integer_expression(text)


if __name__ == "__main__":
    unittest.main()
