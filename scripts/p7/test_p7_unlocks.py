"""P7 U3 host test: the unlock registry's evaluator (src/nds/nds_p7_unlocks.c)
over its generated table (scripts/p7/unlocks.json), every row just below, at
and above its thresholds, skipped when its content (or its challenger's
stage) is not compiled, when a prerequisite is missing and when it is
already unlocked, and the companions a fighter's unlock opens.

    python -m pytest -q scripts/p7/test_p7_unlocks.py
"""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))

import generate_p7_unlocks as G  # noqa: E402

ANY = G.ANY
SITE = G.SITES


def _compiler() -> str | None:
    for name in ("clang", "gcc", "cc"):
        path = shutil.which(name)
        if path:
            return path
    return None


def host_source() -> str:
    header = re.sub(r"^#include[^\n]*", "", (ROOT / "include/nds/nds_p7.h").read_text(), flags=re.M)
    table = (ROOT / "src/nds/nds_p7_unlocks.generated.inc").read_text()
    body = (ROOT / "src/nds/nds_p7_unlocks.c").read_text()
    body = re.sub(r"^#include[^\n]*", "", body, flags=re.M)
    return r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
typedef unsigned long long u64; typedef int s32; typedef unsigned char ub8;
#define TRUE 1
#define FALSE 0
#define GMCOMMON_FIGHTERS_PLAYABLE_NUM 12
#define NDS_P7_UNLOCK_HOOKS 0
typedef struct { u32 spgame_hiscore; u8 bonus1_task_count; u8 bonus2_task_count;
                 ub8 is_spgame_complete; } LBBackup1PRecord;
static struct { LBBackup1PRecord spgame_records[GMCOMMON_FIGHTERS_PLAYABLE_NUM]; } gSCManagerBackupData;
static u64 sFighters, sStages, sContents, sGkinds;
''' + header + r'''
u32 ndsP7FighterUnlocked(u32 sel) { return (u32)((sFighters >> sel) & 1u); }
u32 ndsP7StageUnlocked(u32 stage) { return (u32)((sStages >> stage) & 1u); }
void ndsP7SetFighterUnlocked(u32 sel) { sFighters |= 1ull << sel; }
void ndsP7SetStageUnlocked(u32 stage) { sStages |= 1ull << stage; }
u32 ndsP7UnlockContentCompiled(u32 content) { return (u32)((sContents >> content) & 1u); }
u32 ndsP7UnlockStageCompiled(u32 gkind) { return (u32)((sGkinds >> gkind) & 1u); }
''' + table + "\n" + body.replace('"nds_p7_unlocks.generated.inc"', '') + r'''
int main(int argc, char **argv)
{
    NDSP7Run run;
    u8 out[64];
    u32 n, i, k;
    u32 complete, targets, platforms;

    if (argc != 18) return 2;
    sFighters = strtoull(argv[1], 0, 16); sStages = strtoull(argv[2], 0, 16);
    sContents = strtoull(argv[3], 0, 16); sGkinds = strtoull(argv[4], 0, 16);
    complete = (u32)strtoul(argv[5], 0, 16); targets = (u32)strtoul(argv[6], 0, 16);
    platforms = (u32)strtoul(argv[7], 0, 16);
    for (k = 0; k < 12; k++) {
        gSCManagerBackupData.spgame_records[k].is_spgame_complete = (complete >> k) & 1u;
        gSCManagerBackupData.spgame_records[k].bonus1_task_count = ((targets >> k) & 1u) ? 10 : 9;
        gSCManagerBackupData.spgame_records[k].bonus2_task_count = ((platforms >> k) & 1u) ? 10 : 9;
    }
    run.site = (u8)atoi(argv[8]); run.sel = (u8)atoi(argv[9]); run.difficulty = (u8)atoi(argv[10]);
    run.stock_setting = (u8)atoi(argv[11]); run.continues = (u8)atoi(argv[12]);
    run.falls = (u8)atoi(argv[13]); run.stage = (u8)atoi(argv[14]); run.stage_falls = (u8)atoi(argv[15]);
    run.time_s = (u16)atoi(argv[16]); run.race_left_s = (u16)atoi(argv[17]);
    n = ndsP7UnlockMatch(&run, out, 64);
    for (i = 0; i < n; i++) printf("%u ", out[i]);
    printf("\n");
    return 0;
}
'''


class UnlockRegistryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc = _compiler()
        if cls.cc is None:
            raise unittest.SkipTest("no host C compiler")
        cls.tmp = tempfile.mkdtemp()
        src = Path(cls.tmp) / "unlocks.c"
        src.write_text(host_source())
        cls.exe = str(Path(cls.tmp) / ("unlocks.exe" if os.name == "nt" else "unlocks"))
        subprocess.run([cls.cc, "-std=c11", "-Wall", "-Wextra", "-Werror", str(src), "-o", cls.exe],
                       check=True)
        import json
        cls.spec = json.loads((HERE / "unlocks.json").read_text(encoding="utf-8"))
        cls.rows = G.rows(cls.spec)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(getattr(cls, "tmp", ""), ignore_errors=True)

    def test_generated_table_is_current(self):
        self.assertEqual((ROOT / "src/nds/nds_p7_unlocks.generated.inc").read_text(encoding="utf-8"),
                         G.render(self.rows))

    # Everything compiled, nothing unlocked, no records.
    def match(self, run: dict, fighters=0, stages=0, contents=(1 << 64) - 1, gkinds=(1 << 64) - 1,
              complete=0, targets=0, platforms=0) -> list[int]:
        r = dict(site=0, sel=0, difficulty=0, stocks=3, continues=0, falls=0, stage=ANY,
                 stage_falls=0, time=600, race=0xFFFF)
        r.update(run)
        args = [self.exe] + [f"{v:x}" for v in (fighters, stages, contents, gkinds, complete, targets,
                                                 platforms)]
        args += [str(r[k]) for k in ("site", "sel", "difficulty", "stocks", "continues", "falls", "stage",
                                     "stage_falls", "time", "race")]
        out = subprocess.run(args, check=True, capture_output=True, text=True).stdout.split()
        return [int(x) for x in out]

    def base_run(self, row: dict) -> tuple[dict, dict]:
        """A run (and save state) that just meets the row."""
        run = {"site": row["site"] if row["site"] != SITE["bonus_end"] else SITE["bonus1_end"]}
        sels = [s for s in range(64) if (row["fighters"] >> s) & 1]
        run["sel"] = sels[0] if sels else 0
        run["difficulty"] = row["min_difficulty"]
        if row["max_continues"] != ANY:
            run["continues"] = row["max_continues"]
        if row["max_falls"] != ANY:
            run["falls"] = row["max_falls"]
        if row["stock_setting"] != ANY:
            run["stocks"] = row["stock_setting"]
        if row["stage"] != ANY:
            run["stage"] = row["stage"]
        if row["max_stage_falls"] != ANY:
            run["stage_falls"] = row["max_stage_falls"]
        if row["max_time_s"] != 0xFFFF:
            run["time"] = row["max_time_s"]
        if row["min_race_left_s"]:
            run["race"] = row["min_race_left_s"]
        state = {"fighters": row["requires"]}
        all12 = (1 << 12) - 1
        if row["all_cleared"]:
            state["complete"] = all12
        if row["all_targets"]:
            state["targets"] = all12
        if row["bonus_targets"]:
            state["targets"] = state.get("targets", 0) | (row["fighters"] & all12)
        if row["bonus_platforms"]:
            state["platforms"] = row["fighters"] & all12
        return run, state

    def test_every_row_at_below_and_above(self):
        for i, row in enumerate(self.rows):
            if row["site"] == SITE["unlock"]:
                continue
            with self.subTest(row=row["name"]):
                run, state = self.base_run(row)
                self.assertIn(i, self.match(run, **state), "at the threshold")
                # Already unlocked: skipped.
                key = "fighters" if row["kind"] == 0 else "stages"
                done = dict(state)
                done[key] = done.get(key, 0) | (1 << row["id"])
                self.assertNotIn(i, self.match(run, **done), "already unlocked")
                # Below each threshold the row names.
                if row["min_difficulty"] > 0:
                    self.assertNotIn(i, self.match(dict(run, difficulty=row["min_difficulty"] - 1), **state))
                if row["min_difficulty"] < 4:
                    self.assertIn(i, self.match(dict(run, difficulty=row["min_difficulty"] + 1), **state))
                if row["max_continues"] != ANY:
                    self.assertNotIn(i, self.match(dict(run, continues=row["max_continues"] + 1), **state))
                if row["max_falls"] != ANY:
                    self.assertNotIn(i, self.match(dict(run, falls=row["max_falls"] + 1), **state))
                if row["stock_setting"] != ANY:
                    self.assertNotIn(i, self.match(dict(run, stocks=row["stock_setting"] + 1), **state))
                if row["max_time_s"] != 0xFFFF:
                    self.assertNotIn(i, self.match(dict(run, time=row["max_time_s"] + 1), **state))
                    self.assertIn(i, self.match(dict(run, time=row["max_time_s"] - 1), **state))
                if row["min_race_left_s"]:
                    self.assertNotIn(i, self.match(dict(run, race=row["min_race_left_s"] - 1), **state))
                    self.assertNotIn(i, self.match(dict(run, race=0xFFFF), **state))
                    self.assertIn(i, self.match(dict(run, race=row["min_race_left_s"] + 1), **state))
                if row["max_stage_falls"] != ANY:
                    self.assertNotIn(i, self.match(dict(run, stage_falls=row["max_stage_falls"] + 1), **state))
                if row["stage"] != ANY:
                    self.assertNotIn(i, self.match(dict(run, stage=(row["stage"] + 1) % 14), **state))
                if row["fighters"]:
                    other = next(s for s in range(40) if not (row["fighters"] >> s) & 1)
                    self.assertNotIn(i, self.match(dict(run, sel=other), **state), "another fighter")
                # The wrong site.
                wrong = SITE["run_end"] if run["site"] != SITE["run_end"] else SITE["stage_clear"]
                self.assertNotIn(i, self.match(dict(run, site=wrong), **state))
                # Records short of the row's.
                if row["all_cleared"]:
                    self.assertNotIn(i, self.match(run, **dict(state, complete=(1 << 12) - 2)))
                if row["all_targets"] or row["bonus_targets"]:
                    self.assertNotIn(i, self.match(run, **dict(state, targets=0)))
                if row["bonus_platforms"]:
                    self.assertNotIn(i, self.match(run, **dict(state, platforms=0)))
                # A missing prerequisite (its content off, so this run cannot
                # unlock it on the way: test_order_and_chains covers that).
                for sel in range(64):
                    if (row["requires"] >> sel) & 1:
                        off = ((1 << 64) - 1) & ~(1 << (sel - 12)) if sel > 12 else (1 << 64) - 1
                        self.assertNotIn(i, self.match(run, contents=off,
                                                       **dict(state, fighters=row["requires"] & ~(1 << sel))))
                # Not compiled: its content, its stage, its challenger's stage.
                if row["kind"] == 0 and row["content"]:
                    self.assertNotIn(i, self.match(run, contents=((1 << 64) - 1) & ~(1 << row["content"]), **state))
                if row["kind"] == 1:
                    self.assertNotIn(i, self.match(run, gkinds=((1 << 64) - 1) & ~(1 << row["stage_gkind"]), **state))
                if row["delivery"] == G.DELIVERY["challenger"]:
                    self.assertNotIn(i, self.match(run, gkinds=((1 << 64) - 1) & ~(1 << row["challenger_gkind"]),
                                                   **state))

    def test_companions(self):
        names = [r["name"] for r in self.rows]
        metal = names.index("Metal Mario")
        crystal = names.index("meta_crystal")
        run, state = self.base_run(self.rows[metal])
        got = self.match(run, **state)
        self.assertEqual(got, [metal, crystal], "Meta Crystal comes with Metal Mario, after him")
        # Hyrule Temple: with the second of Ganondorf and Sheik only.
        temple = names.index("hyrule_temple")
        ganon = names.index("Ganondorf")
        sheik = self.rows[names.index("Sheik")]["id"]
        run, state = self.base_run(self.rows[ganon])
        self.assertNotIn(temple, self.match(run, **state))
        self.assertIn(temple, self.match(run, **dict(state, fighters=1 << sheik)))

    def test_order_and_chains_in_one_run(self):
        # A Very Hard, no-continue clear unlocks Marth, then Roy (Marth now
        # counts), then Ike (both) -- in table order, one run.
        names = [r["name"] for r in self.rows]
        got = self.match({"site": SITE["run_end"], "sel": 0, "difficulty": 4, "continues": 0, "falls": 1})
        want = [names.index(n) for n in ("Marth", "Roy", "Ike")]
        for w in want:
            self.assertIn(w, got)
        self.assertEqual([g for g in got if g in want], sorted(want))


if __name__ == "__main__":
    unittest.main()
