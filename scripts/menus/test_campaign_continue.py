"""Host-execute source Continue decisions and manager score/record updates."""
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
    source = (DECOMP / "mn/mn1pmode/mn1pcontinue.c").read_text()
    manager = (DECOMP / "sc/sc1pmode/sc1pmanager.c").read_text()
    update = function(source, "mnPlayers1PGameContinueFuncRun")
    initialize = function(source, "mnPlayers1PGameContinueInitVars")
    base = function(source, "mnPlayers1PGameContinueStartScene").replace(
        "void mnPlayers1PGameContinueStartScene", "void ndsBaseMNPlayers1PGameContinueStartScene")
    record = function(manager, "sc1PManagerTrySaveBackup")
    manager_branch = braced(manager, r"if \(is_player_lose != FALSE\)\s*\{")
    manager_branch = manager_branch[manager_branch.index("if (gSCManagerSceneData.is_continue"):
                                    manager_branch.rfind("}")]
    score = function((DECOMP / "sc/sc1pmode/sc1pstageclear.c").read_text(),
                     "sc1PStageClearUpdateTotal1PGameScore")
    enums = "\n".join(original_enum(path, name) for path, name in (
        ("sc/scdef.h", "SCKind"), ("sc/scdef.h", "MN1PContinueOption"),
        ("sc/scdef.h", "SC1PGameDifficulty")))
    header = (ROOT / "include/sc/scene.h").read_text()
    record_type = braced(header, r"typedef struct LBBackup1PRecord\s*\{", True)
    scalar_names = set(re.findall(r"\bsMN1PContinue\w+", update + initialize))
    scalar_names.discard("sMN1PContinueFighterDemoDesc")
    globals_c = "\n".join("static " + ("GObj *" if name.endswith("GObj") else "int ") + name + ";"
                          for name in sorted(scalar_names))
    return r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define TRUE 1
#define FALSE 0
#define I_SEC_TO_TICS(x) ((int)((x) * 60))
#define A_BUTTON 0x8000
#define START_BUTTON 0x1000
#define L_TRIG 0x20
#define L_JPAD 0x200
#define L_CBUTTONS 2
#define R_TRIG 0x10
#define R_JPAD 0x100
#define R_CBUTTONS 1
typedef uint8_t u8;
typedef uint8_t ub8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef int sb32;
typedef void GObj;
''' + enums + "\n" + record_type + r'''
enum { nFTDemoStatusFigureStand, nSYAudioBGM1PGameOver, nSYAudioVoiceAnnounceGameOver,
       nSYAudioFGM1PGameContinue, nSYAudioFGMMenuScroll1, nSYAudioVoiceAnnounceContinue };
static struct {
    int scene_curr, scene_prev, player, fkind, is_continue, continues_used, bonus_count, spgame_stage;
    u32 spgame_score;
} gSCManagerSceneData;
static struct { struct { int fkind, costume, shade, stock_count; } players[4]; } gSCManager1PGameBattleState;
static struct { LBBackup1PRecord spgame_records[12]; int spgame_difficulty, spgame_stock_count; } gSCManagerBackupData;
static struct { int fkind, costume, shade; } sMN1PContinueFighterDemoDesc;
static int gSC1PManagerLevelDrop, sSC1PManagerLevelGuard = 2;
static u32 sSC1PStageClearScoreTotal;
static int loaded, choice_tick, choose_no, time_out, writes;
''' + globals_c + r'''
static int scSubsysControllerGetPlayerStickInRangeLR(int low, int high) { (void)low; (void)high; return TRUE; }
static int scSubsysControllerGetPlayerStickInRangeUD(int low, int high) { (void)low; (void)high; return TRUE; }
static int scSubsysControllerGetPlayerTapButtons(int mask)
{
    return !time_out && (mask & A_BUTTON) &&
        (sMN1PContinueTotalTimeTics == choice_tick ||
         (choose_no && sMN1PContinueTotalTimeTics == choice_tick + 91));
}
#define mnPlayers1PGameContinueCheckGetOptionStickInputLR(stick, min, direction) ((stick) = 0, FALSE)
#define mnPlayers1PGameContinueSetOptionChangeWaitN(stick) (sMN1PContinueOptionChangeWait = ((stick) + 160) / 5)
#define mnPlayers1PGameContinueSetOptionChangeWaitP(stick) (sMN1PContinueOptionChangeWait = (160 - (stick)) / 5)
static void syTaskmanSetLoadScene(void) { loaded++; }
static void gcEjectGObj(GObj *object) { (void)object; }
static void scSubsysFighterSetStatus(GObj *object, int status) { (void)object; (void)status; }
static void syAudioPlayBGM(int player, int music) { (void)player; (void)music; }
static void func_800269C0_275C0(int sound) { (void)sound; }
static void mnPlayers1PGameContinueMakeScoreDisplay(u32 value) { CHECK(value == gSCManagerSceneData.spgame_score); }
#define NOOP(name) static void name(void) {}
NOOP(mnPlayers1PGameContinueMakeRoomFadeOut)
NOOP(mnPlayers1PGameContinueMakeGameOverText)
NOOP(mnPlayers1PGameContinueMakeGameOver)
NOOP(mnPlayers1PGameContinueUnused0x80133990)
NOOP(mnPlayers1PGameContinueMakeSpotlight)
NOOP(mnPlayers1PGameContinueMakeSpotlightFade)
NOOP(mnPlayers1PGameContinueMakeRoomFadeIn)
NOOP(mnPlayers1PGameContinueMakeRoom)
NOOP(mnPlayers1PGameContinueMakeContinue)
NOOP(mnPlayers1PGameContinueMakeOptions)
NOOP(mnPlayers1PGameContinueMakeCursor)
static void lbBackupWrite(void) { writes++; }
''' + initialize + "\n" + update + "\n" + record + "\n" + score + r'''
static struct { void *zbuffer; } dMN1PContinueVideoSetup;
typedef struct { struct { size_t arena_size; } scene_setup; } SYTaskmanSetup;
static SYTaskmanSetup dMN1PContinueTaskmanSetup;
static uintptr_t ovl1_VRAM, ovl55_BSS_END;
#define SYVIDEO_ZBUFFER_START(...) ((void *)1)
static void syVideoInit(void *setup) { CHECK(setup != NULL); }
static void scManagerFuncUpdate(SYTaskmanSetup *setup)
{
    CHECK(setup == &dMN1PContinueTaskmanSetup);
    CHECK(gSCManagerSceneData.scene_curr == nSCKind1PContinue);
    CHECK(sMN1PContinueOptionYesRetryTic == 0);
    CHECK(sMN1PContinueOptionChangeWait == 0 && sMN1PContinueIsSelectContinue == FALSE);
    mnPlayers1PGameContinueInitVars();
    sMN1PContinueOptionSelect = choose_no ? nMN1PContinueOptionNo : nMN1PContinueOptionYes;
    loaded = 0;
    while (!loaded && sMN1PContinueTotalTimeTics < 5000)
        mnPlayers1PGameContinueFuncRun(NULL);
    CHECK(loaded == 1);
}
''' + base + "\n" + wrapper + r'''
static void manager_after_continue(void)
{
''' + manager_branch + r'''
    gSCManagerSceneData.spgame_stage++;
    /* The next iteration's source intro routing supersedes Continue's Title. */
    gSCManagerSceneData.scene_prev = nSCKind1PGame;
    gSCManagerSceneData.scene_curr = nSCKind1PIntro;
}
static void enter(int no, int timeout, int tick)
{
    gSCManagerSceneData.scene_curr = nSCKind1PGame;
    gSCManagerSceneData.scene_prev = nSCKind1PIntro;
    choose_no = no; time_out = timeout; choice_tick = tick;
    mnPlayers1PGameContinueStartScene();
    CHECK(gSCManagerSceneData.scene_curr == nSCKindTitle);
    CHECK(gSCManagerSceneData.scene_prev == nSCKind1PContinue);
}
int main(void)
{
    for (int port = 0; port < 4; port++) {
        gSCManagerSceneData.player = port;
        gSCManagerSceneData.fkind = 0;
        gSCManagerSceneData.spgame_stage = 2;
        gSCManagerSceneData.spgame_score = 10001;
        gSCManagerSceneData.continues_used = 0;
        gSCManagerSceneData.bonus_count = 9;
        gSCManagerBackupData.spgame_difficulty = nSC1PGameDifficultyNormal;
        gSCManagerBackupData.spgame_stock_count = 2; /* three displayed stocks */
        gSCManager1PGameBattleState.players[port].stock_count = -1;
        sSC1PManagerLevelGuard = 2; gSC1PManagerLevelDrop = 0;
        enter(FALSE, FALSE, 151);
        CHECK(gSCManagerSceneData.is_continue && gSCManagerSceneData.spgame_score == 5000);
        manager_after_continue();
        CHECK(gSCManagerSceneData.continues_used == 1 && gSCManagerSceneData.spgame_stage == 2);
        CHECK(gSCManager1PGameBattleState.players[port].stock_count == 2);
        CHECK(gSCManagerSceneData.scene_curr == nSCKind1PIntro && gSC1PManagerLevelDrop == 0);
        /* Previous Yes's retry tick is 391; no input arrives before tick 600. */
        enter(TRUE, FALSE, 600);
        CHECK(!gSCManagerSceneData.is_continue && gSCManagerSceneData.spgame_score == 5000);
        memset(&gSCManagerBackupData.spgame_records[0], 0, sizeof(LBBackup1PRecord));
        writes = 0; manager_after_continue();
        CHECK(gSCManagerSceneData.scene_curr == nSCKindStartup && writes == 1);
        CHECK(gSCManagerBackupData.spgame_records[0].spgame_hiscore == 5000);
        CHECK(gSCManagerBackupData.spgame_records[0].spgame_continues == 1);
        CHECK(!gSCManagerBackupData.spgame_records[0].is_spgame_complete);
        enter(FALSE, TRUE, 0);
        CHECK(!gSCManagerSceneData.is_continue && gSCManagerSceneData.spgame_score == 5000);
        CHECK(sMN1PContinueTotalTimeTics == I_SEC_TO_TICS(70));
        /* Stage tally publishes score; source completion and record rules stay intact. */
        sSC1PStageClearScoreTotal = 7000;
        sc1PStageClearUpdateTotal1PGameScore();
        writes = 0; sc1PManagerTrySaveBackup(TRUE);
        CHECK(writes == 1 && gSCManagerBackupData.spgame_records[0].spgame_hiscore == 7000);
        CHECK(gSCManagerBackupData.spgame_records[0].spgame_best_difficulty == nSC1PGameDifficultyNormal + 1);
        CHECK(gSCManagerBackupData.spgame_records[0].is_spgame_complete);
        writes = 0; sc1PManagerTrySaveBackup(FALSE); CHECK(writes == 0);
    }
    return 0;
}
'''


class CampaignContinueTest(unittest.TestCase):
    def test_continue_routes_and_source_state(self):
        text = (ROOT / "src/import/battleship_mn1pcontinue.c").read_text()
        wrapper = function(text, "mnPlayers1PGameContinueStartScene")
        registry = (ROOT / "src/port/nds_scene_manager.c").read_text()
        self.assertRegex(registry, r"nSCKind1PContinue,\s*NDS_SCENE_FLAG_ARENA_RESET \| NDS_SCENE_FLAG_MENU")
        pump = (ROOT / "src/port/taskman_seam_harness.c").read_text()
        self.assertIn("case nSCKind1PContinue:", pump)
        compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc") if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required")
        with tempfile.TemporaryDirectory() as directory:
            source, program = Path(directory) / "continue.c", Path(directory) / "continue.exe"
            for removed in (None, "gSCManagerSceneData.scene_curr = nSCKind1PContinue;",
                            "sMN1PContinueOptionYesRetryTic = 0;",
                            "sMN1PContinueIsSelectContinue = FALSE;"):
                with self.subTest(negative_control=removed):
                    candidate = wrapper.replace(removed, "") if removed else wrapper
                    self.assertEqual(candidate != wrapper, removed is not None)
                    source.write_text(host_source(candidate))
                    # Only A input is exercised; the source's uninitialized
                    # stick-range warning is on an unexecuted left/right branch.
                    subprocess.run([compiler, "-std=c11", "-DREGION_US=1", "-Wall", "-Wextra",
                                    "-Wno-unused-variable", "-Wno-unused-parameter", "-Wno-uninitialized",
                                    str(source), "-o", str(program)], check=True)
                    result = subprocess.run([str(program)], capture_output=True, text=True)
                    self.assertEqual(result.returncode != 0, removed is not None, result.stderr)


if __name__ == "__main__":
    unittest.main()
