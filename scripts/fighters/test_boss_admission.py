"""Master Hand pipeline admission: spec, manifest Boss entry, staging plans.

The 1P-only boss (nFTKindBoss, kind 12) is neither selectable nor variant:
no native owner packet, no CSS/stock/selected/audio surfaces. Admission is
the three reloc roots staged by admit_fighter.py --fighter boss plus the
manifest Boss entry that resolves his ftdata-reachable file graph to pinned
US O2Rs. These tests pin that contract without launching a build.
"""
import importlib.util
import json
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


admit = load_module("admit_fighter_boss_test",
                    REPO / "scripts/fighters/admit_fighter.py")
stage = load_module("stage_reloc_file_boss_test",
                    REPO / "scripts/menus/stage_reloc_file.py")
MANIFEST = json.loads(
    (REPO / "scripts/fighters/fighter_production_manifest.json")
    .read_text(encoding="utf-8"))

BOSS_IDS = {"BossMain": 0xFA, "BossMainMotion": 0xF9, "BossModel": 0x158}


class BossSpecTests(unittest.TestCase):
    def test_spec_is_boss_only(self):
        spec = admit.SPECS["boss"]
        self.assertEqual((spec["Name"], spec["kind"], spec["token"]),
                         ("Boss", 12, "BOSS"))
        self.assertTrue(spec["boss"])
        self.assertEqual(tuple(spec["files"]), tuple(BOSS_IDS))
        self.assertEqual((spec["main_file_id"], spec["mainmotion_file_id"],
                          spec["model_file_id"]), (0xFA, 0xF9, 0x158))

    def test_spec_has_no_selectable_surfaces(self):
        spec = admit.SPECS["boss"]
        for key in ("css_portrait", "css_emblem", "css_name", "owner_slot",
                    "image_slot", "selected_file"):
            self.assertNotIn(key, spec)
        self.assertIsNone(spec["stock"])
        self.assertIsNone(spec["entry_statuses"])


class BossManifestTests(unittest.TestCase):
    def test_core_slots_resolve_to_pinned_o2r(self):
        boss = MANIFEST["boss"]
        self.assertEqual((boss["fighter"], boss["kind"],
                          boss["ftdata_symbol"]),
                         ("Boss", 12, "dFTBossData"))
        core = {row["slot"]: row for row in boss["core"]}
        self.assertEqual(
            {slot: (core[slot]["symbol"], core[slot]["asset"]["id"])
             for slot in ("main", "mainmotion", "model")},
            {"main": ("llBossMainFileID", 0xFA),
             "mainmotion": ("llBossMainMotionFileID", 0xF9),
             "model": ("llBossModelFileID", 0x158)})
        for slot in ("main", "mainmotion", "model"):
            asset = core[slot]["asset"]
            self.assertTrue(asset["path"].startswith("reloc_fighters_main/"))
            self.assertEqual(len(asset["sha256"]), 64)
            self.assertGreater(asset["bytes"], 0)

    def test_motion_table_resolves_to_real_files(self):
        boss = MANIFEST["boss"]
        self.assertEqual(len(boss["motion_files"]), 30)
        for row in boss["motion_files"]:
            self.assertEqual(len(row["asset"]["sha256"]), 64)
            self.assertIsNotNone(row["relocdata_resource"])
        pose = [row for row in boss["submotion_files"]
                if row.get("symbol") == "llFTBossAnimPose1PFileID"]
        self.assertEqual(len(pose), 1)
        self.assertEqual(pose[0]["asset"]["id"], 0x1CD)

    def test_status_contract_present(self):
        contract = MANIFEST["boss"]["special_status_contract"]
        self.assertGreater(contract["descriptor_count"], 0)
        self.assertEqual(len(contract["descriptors"]),
                         contract["descriptor_count"])


class BossStagePlanTests(unittest.TestCase):
    def test_core_ids_and_offsets(self):
        for name, file_id in BOSS_IDS.items():
            with self.subTest(file=name):
                plan = admit.BossPlan(REPO, stage, name)
                self.assertEqual(plan.file_id, file_id)
                self.assertEqual(plan.dir, "reloc_fighters_main")
        motion = admit.BossPlan(REPO, stage, "BossMainMotion")
        self.assertEqual(
            motion.symbols,
            {"llBossMainMotionBulletNormalWeaponAttributes": 0x774,
             "llBossMainMotionBulletHardWeaponAttributes": 0x7A8})

    def test_staged_tree_passes_stage_check(self):
        for name in BOSS_IDS:
            with self.subTest(file=name):
                plan = admit.BossPlan(REPO, stage, name)
                self.assertEqual(stage.check(REPO, plan, admit.BOSS_LIST), [])


if __name__ == "__main__":
    unittest.main()
