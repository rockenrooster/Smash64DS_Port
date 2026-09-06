"""Fighting Polygon Team asset staging: plans, ids, and --check coverage.

The twelve polygon kinds (nFTKindNStart..NEnd, kinds 14-25) own 23 reloc
files: twelve Mains and eleven Models. NLuigi reuses NMarioModel
(BattleShip ftdata.c:3156; no llNLuigiModel exists), so there is no 24th
file. Every N Main zeroes specials/catch/voice and reuses its base
MainMotion and ShieldPose, so only the 23 own files stage here, into
NDS_1P_RELOC_FILES beside the boss (the polygon team fights only on the
Zako 1P stage). All 23 are symbol-less (FileIDs only, defined once in
reloc_backend_ftdata_symbols.c), so the staging blocks are extern-only.

These tests pin that contract without launching a build.
"""
import importlib.util
import re
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
if str(Path(__file__).resolve().parent) not in sys.path:
    sys.path.insert(0, str(Path(__file__).resolve().parent))


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


admit = load_module("admit_fighter_polygon_test",
                    REPO / "scripts/fighters/admit_fighter.py")
stage = load_module("stage_reloc_file_polygon_test",
                    REPO / "scripts/menus/stage_reloc_file.py")

# Twelve Mains in kind order (14-25) + eleven Models (NLuigi reuses NMario's).
POLYGON_IDS = {
    "NMarioMain": 0xCF, "NMarioModel": 0x12D,
    "NFoxMain": 0xD3, "NFoxModel": 0x12F,
    "NDonkeyMain": 0xD6, "NDonkeyModel": 0x134,
    "NSamusMain": 0xDB, "NSamusModel": 0x135,
    "NLuigiMain": 0xDF,
    "NLinkMain": 0xE3, "NLinkModel": 0x136,
    "NYoshiMain": 0xF8, "NYoshiModel": 0x130,
    "NCaptainMain": 0xED, "NCaptainModel": 0x137,
    "NKirbyMain": 0xE7, "NKirbyModel": 0x131,
    "NPikachuMain": 0xF5, "NPikachuModel": 0x133,
    "NPurinMain": 0xEA, "NPurinModel": 0x132,
    "NNessMain": 0xF1, "NNessModel": 0x138,
}


class PolygonListTests(unittest.TestCase):
    def test_twenty_three_files_no_nluigi_model(self):
        self.assertEqual(tuple(admit.POLYGON_FILES), tuple(POLYGON_IDS))
        self.assertEqual(len(admit.POLYGON_FILES), 23)
        self.assertNotIn("NLuigiModel", admit.POLYGON_FILES)
        self.assertEqual(admit.POLYGON_LIST, "NDS_1P_RELOC_FILES")

    def test_nluigi_reuses_nmario_model(self):
        nluigi = admit.SPECS["nluigi"]
        self.assertTrue(nluigi["reuse_owner"])
        self.assertEqual((nluigi["model"], nluigi["model_file_id"]),
                         ("NMarioModel", 0x12D))


class PolygonIdTests(unittest.TestCase):
    def test_ids_match_tools_linker_table(self):
        tools_ids = admit.polygon_tools_ids(REPO)
        for name, file_id in POLYGON_IDS.items():
            with self.subTest(file=name):
                self.assertEqual(tools_ids[name], file_id)

    def test_plan_ids_match_containers(self):
        for name, file_id in POLYGON_IDS.items():
            with self.subTest(file=name):
                plan = admit.PolygonPlan(REPO, stage, name)
                self.assertEqual(plan.file_id, file_id)
                self.assertEqual(plan.dir, "reloc_fighters_main")
                self.assertEqual(plan.symbols, {})
                self.assertEqual(plan.rows, [])


class PolygonStagedTreeTests(unittest.TestCase):
    def test_staged_tree_passes_stage_check(self):
        for name in POLYGON_IDS:
            with self.subTest(file=name):
                plan = admit.PolygonPlan(REPO, stage, name)
                self.assertEqual(stage.check(REPO, plan, admit.POLYGON_LIST), [])

    def test_header_carries_fileid_extern_only(self):
        header = (REPO / "include/reloc_data.h").read_text(encoding="utf-8")
        for name in POLYGON_IDS:
            with self.subTest(file=name):
                self.assertTrue(
                    re.search(rf"(?m)^extern uintptr_t ll{re.escape(name)}FileID;", header),
                    f"missing FileID declaration for {name}")

    def test_makefile_lists_all_twenty_three(self):
        make = (REPO / "Makefile").read_text(encoding="utf-8").replace("\r\n", "\n")
        for name in POLYGON_IDS:
            with self.subTest(file=name):
                # Line continuations vary (only the list tail lacks ` \`).
                self.assertTrue(
                    re.search(rf"(?m)^\treloc_fighters_main/{re.escape(name)}(?:[ \t]+\\)?[ \t]*$", make),
                    f"missing Makefile staging row for {name}")

    def test_no_undefined_known_symbol_macros(self):
        header = (REPO / "include/reloc_data.h").read_text(encoding="utf-8")
        backend = (REPO / "src/port/reloc_backend_assets.c").read_text(encoding="utf-8")
        defined = set(re.findall(r"#define\s+(NDS_\w+_RELOC_SYMBOLS)\(", header))
        used = set(re.findall(r"\b(NDS_\w+_RELOC_SYMBOLS)\(", backend))
        self.assertEqual(used - defined, set())


if __name__ == "__main__":
    unittest.main()
