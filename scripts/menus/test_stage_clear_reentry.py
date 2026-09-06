"""Source tally timing across repeated entries without N64 overlay BSS clearing."""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from source_test_helpers import braced, function, original_enum

ROOT = Path(__file__).resolve().parents[2]
DECOMP = ROOT / "decomp/BattleShip-main/decomp/src"


def host_source(wrapper):
    source = (DECOMP / "sc/sc1pmode/sc1pstageclear.c").read_text()
    functions = "\n".join(function(source, name) for name in (
        "sc1PStageClearSetupBonusStats", "sc1PStageClearCheckHaveBonusStats",
        "sc1PStageClearCheckHaveBonusStatID", "sc1PStageClearGetUpdateBonusStatPointsAll",
        "sc1PStageClearCheckNoTimer", "sc1PStageClearInitVars",
        "sc1PStageUpdateBonusStatAll", "sc1PStageClearUpdateStageClearScore",
        "sc1PStageClearUpdateTotal1PGameScore"))
    enums = "\n".join(original_enum("sc/scdef.h", name) for name in
                      ("SC1PGameStageKind", "SC1PStageClearKind"))
    header = (ROOT / "include/sc/scene.h").read_text()
    stats = braced(header, r"typedef struct SC1PStageClearStats\s*\{", True)
    names = sorted(set(re.findall(r"\bsSC1PStageClear\w+", functions)))
    globals_c = []
    for name in names:
        if name.endswith("GObjs"):
            globals_c.append(f"static GObj *{name}[10];")
        elif name.endswith("GObj"):
            globals_c.append(f"static GObj *{name};")
        elif name.endswith("BonusFlags"):
            globals_c.append(f"static u32 {name}[3];")
        else:
            globals_c.append(f"static int {name};")
    return r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define FALSE 0
#define TRUE 1
#define ARRAY_COUNT(x) ((int)(sizeof(x) / sizeof((x)[0])))
#define NBITS(x) ((int)(sizeof(x) * 8))
#define SCBATTLE_TIMELIMIT_INFINITE 100
typedef int32_t s32;
typedef uint32_t u32;
typedef float f32;
typedef int sb32;
typedef void GObj;
''' + enums + "\n" + stats + "\n" + "\n".join(globals_c) + r'''
static struct {
    int player, spgame_stage, spgame_time_limit, bonus_tasks_complete, spgame_time_remain, spgame_score;
    u32 bonus_get_mask[3];
} gSCManagerSceneData;
static struct { int spgame_difficulty; } gSCManagerBackupData;
static struct { struct { int total_damage_given; } players[4]; } gSCManager1PGameBattleState;
static int points_tick, points_calls, time_calls, damage_calls;
static void gcEjectGObj(GObj *object) { (void)object; }
static void sc1PStageClearMakeBonusPageArrow(void) {}
static int sc1PStageClearGetAppendBonusStatPoints(int id, int slot, float x, float y)
{
    (void)slot; (void)x; (void)y; CHECK(id == 0);
    points_tick = sSC1PStageClearTotalTimeTics; points_calls++; return 30;
}
static int sc1PStageClearGetAppendTotalTimeScore(float y) { (void)y; time_calls++; return 10; }
static int sc1PStageClearGetAppendTotalDamageScore(float y) { (void)y; damage_calls++; return 20; }
static void sc1PStageClearMakeTimerTextSObjs(float y) { (void)y; sSC1PStageClearTimerTextGObj = (GObj *)1; }
static void sc1PStageClearMakeTimerDigits(float y) { (void)y; sSC1PStageClearTimerMultiplierGObj = (GObj *)1; }
static void sc1PStageClearMakeDamageTextSObjs(float y) { (void)y; sSC1PStageClearDamageTextGObj = (GObj *)1; }
static void sc1PStageClearMakeDamageDigits(float y) { (void)y; sSC1PStageClearDamageMultiplierGObj = (GObj *)1; }
static void sc1PStageClearUpdateBonusScore(void) { sSC1PStageClearScoreTextGObj = (GObj *)1; }
static void sc1PStageClearMakeBonusTable(void) {}
''' + functions + r'''
static void ndsBaseSC1PStageClearStartScene(void)
{
    sc1PStageClearInitVars();
    sSC1PStageClearBonusTextGObj = (GObj *)1;
    sSC1PStageClearScoreTextGObj = (GObj *)1;
    points_tick = points_calls = time_calls = damage_calls = 0;
    while (!sSC1PStageClearIsAllowProceedNext && sSC1PStageClearTotalTimeTics < 300) {
        sSC1PStageClearTotalTimeTics++;
        if (sSC1PStageClearTotalTimeTics >= 10) sc1PStageClearUpdateStageClearScore();
    }
    CHECK(sSC1PStageClearIsAllowProceedNext);
    sc1PStageClearUpdateTotal1PGameScore();
}
''' + wrapper + r'''
int main(void)
{
    gSCManagerSceneData.spgame_stage = nSC1PGameStageLink;
    gSCManagerSceneData.bonus_get_mask[0] = 1;
    gSCManagerSceneData.spgame_score = 1000;
    /* A prior campaign used no timer; the next uses the five-minute setting.
     * Both entries retain this TU's globals on DS, unlike the N64 overlay. */
    gSCManagerSceneData.spgame_time_limit = SCBATTLE_TIMELIMIT_INFINITE;
    sc1PStageClearStartScene();
    CHECK(points_tick == 70 && points_calls == 1 && time_calls == 0 && damage_calls == 1);
    CHECK(sSC1PStageClearTotalTimeTics == 120 && gSCManagerSceneData.spgame_score == 1050);
    gSCManagerSceneData.spgame_time_limit = 5;
    sc1PStageClearStartScene();
    CHECK(points_tick == 130 && points_calls == 1 && time_calls == 1 && damage_calls == 1);
    CHECK(sSC1PStageClearTotalTimeTics == 180 && gSCManagerSceneData.spgame_score == 1110);
    sc1PStageClearStartScene();
    CHECK(points_tick == 130 && sSC1PStageClearTotalTimeTics == 180);
    CHECK(gSCManagerSceneData.spgame_score == 1170);
    return 0;
}
'''


class StageClearReentryTest(unittest.TestCase):
    def test_source_tally_deadlines_and_reentry(self):
        text = (ROOT / "src/import/battleship_sc1pstageclear.c").read_text()
        wrapper = function(text, "sc1PStageClearStartScene")
        compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc") if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required")
        with tempfile.TemporaryDirectory() as directory:
            source, program = Path(directory) / "tally.c", Path(directory) / "tally.exe"
            for broken in (False, True):
                candidate = re.sub(r"    sSC1PStageClear(?:CommonAdvance|BonusShowNext|BonusAdvance)Tic = 0;\n", "", wrapper) if broken else wrapper
                self.assertEqual(candidate != wrapper, broken)
                source.write_text(host_source(candidate))
                subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Wno-unused-variable",
                                str(source), "-o", str(program)], check=True)
                result = subprocess.run([str(program)], capture_output=True, text=True)
                self.assertEqual(result.returncode != 0, broken, result.stderr)


if __name__ == "__main__":
    unittest.main()
