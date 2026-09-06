"""Campaign entry regression: real wrapper and BattleShip stage setup on the host.

Use --patch PATH to apply a candidate to a temporary source copy. The live TU
is never changed. Hardware/task execution is mocked; the original stage tables,
setup, costume selection and shuffle helpers are compiled verbatim. This checks
state ownership and setup, not fighter assets, rendering or a playable campaign.
"""
import argparse
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from source_test_helpers import braced, clean, function, original_enum

ROOT = Path(__file__).resolve().parents[2]
TU = Path("src/import/battleship_sc1pgame_runtime.c")
DECOMP = ROOT / "decomp/BattleShip-main/decomp/src"
PATCH = None


def declaration(source, name):
    return braced(source, rf"^\w+\s+{name}\s*\[[^\]]*\]\s*=\s*\{{", True)


def host_source(candidate):
    source = (DECOMP / "sc/sc1pmode/sc1pgame.c").read_text()
    helpers = "\n".join(function(source, name) for name in (
        "sc1PGameGetNextFreePlayerPort", "sc1PGameGetNextFreeCostume",
        "sc1PGameGetFighterKindsNum", "sc1PGameSetupEnemyPlayer",
        "sc1PGameSetupStageAll"))
    enums = "\n".join(original_enum(path, name) for path, name in (
        ("ft/ftdef.h", "FTKind"), ("ft/ftdef.h", "FTPlayerKind"),
        ("ft/ftdef.h", "FTComputerTraitKind"), ("gr/grdef.h", "GRKind"),
        ("mp/mpdef.h", "MPMapObjKind"), ("if/ifdef.h", "IFPlayerTagKind"),
        ("sc/scdef.h", "SCKind"), ("sc/scdef.h", "SCBattlePlayerColor"),
        ("sc/scdef.h", "SCBattleTeamID"), ("sc/scdef.h", "SCBattleGameType"),
        ("sc/scdef.h", "SCBattleItemSwitch"), ("sc/scdef.h", "SC1PGameDifficulty"),
        ("sc/scdef.h", "SC1PGameStageKind")))
    types = (DECOMP / "sc/sctypes.h").read_text()
    types = "\n".join("typedef " + braced(types, rf"struct {name}\s*\{{") +
                      f" {name};" for name in
                      ("SC1PGameComputer", "SC1PGameStage", "SC1PGameFighter"))
    constants = "\n".join(re.findall(r"^#define SC1PGAME_STAGE_\w+[^\n]*",
        (DECOMP / "sc/sc1pmode/sc1pgame.h").read_text(), re.M))
    constants += "\n" + re.search(r"^#define SCBATTLE_TIMELIMIT_INFINITE[^\n]*",
        (DECOMP / "sc/scdef.h").read_text(), re.M)[0]
    tables = "\n".join(declaration(source, name) for name in
                      ("dSC1PGameComputerDesc", "dSC1PGameStageDesc",
                       "dSC1PGameKirbyTeamCopyKinds"))
    # Unrelated scoring counters use host fixtures; their reset bodies still run.
    scalars = sorted(set(re.findall(r"\b[gs]SC1P\w+", helpers)) - {
        "gSC1PManagerLevelDrop", "sSC1PGamePlayerSetups",
        "sSC1PGameEnemyVariations"})
    globals_c = "\n".join("static int " + name +
        ("[64];" if re.search(rf"\b{name}\s*\[", helpers) else ";")
        for name in scalars)
    return r'''
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define TRUE 1
#define FALSE 0
#define GMCOMMON_PLAYERS_MAX 4
#define ARRAY_COUNT(x) ((int)(sizeof(x) / sizeof((x)[0])))
typedef int32_t s32;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int sb32;
typedef uint8_t ub8;
typedef float f32;
''' + enums + "\n" + types + "\n" + constants + r'''
typedef struct {
    int level, handicap, fkind, team, costume, shade, color, tag;
    int is_single_stockicon, stock_count, is_spgame_enemy, pkind;
} Player;
typedef struct {
    int game_type, is_team_battle, game_rules, damage_ratio;
    int is_show_score, is_not_teamshadows, gkind, is_team_attack, time_limit;
    u32 item_toggles;
    int item_appearance_rate, pl_count, cp_count;
    Player players[4];
} Battle;
static Battle gSCManager1PGameBattleState, other_state;
static Battle *gSCManagerBattleState;
static struct {
    u8 spgame_stage, player, fkind, costume, ally_players[2];
    int is_reset, challenger_level_drop, spgame_time_limit, scene_prev, scene_curr;
} gSCManagerSceneData;
static struct { int spgame_difficulty, spgame_stock_count; } gSCManagerBackupData;
static int gSC1PManagerLevelDrop;
static SC1PGameFighter sSC1PGamePlayerSetups[4];
static u8 sSC1PGameEnemyVariations[30];
static int rng_calls;
static int syUtilsRandIntRange(int range) { CHECK(range > 0); rng_calls++; return 0; }
static int ftParamGetCostumeCommonID(int kind, int costume) { (void)kind; return costume; }
''' + globals_c + "\n" + tables + "\n" + helpers + r'''
typedef struct { struct { void *arena_start; size_t arena_size; } scene_setup;
                 void (*func_start)(void); } SYTaskmanSetup;
static SYTaskmanSetup dSC1PGameTaskmanSetup;
static struct { void *zbuffer; } dSC1PGameVideoSetup;
#define SYVIDEO_ZBUFFER_START(...) ((void *)1)
static char arena[32];
static void *ndsTaskmanArenaStart(void) { return arena; }
static size_t ndsTaskmanArenaSize(void) { return sizeof(arena); }
static int starts, phase, expected_stock, expected_allies;
static Player expected_ally[2];
static void sc1PGameFuncStart(void)
{
    CHECK(phase == 2); phase = 3; starts++;
    CHECK(gSCManagerBattleState == &gSCManager1PGameBattleState);
    CHECK(gSCManagerBattleState->game_type == nSCBattleGameType1PGame);
    CHECK(gSCManagerBattleState->players[gSCManagerSceneData.player].stock_count == expected_stock);
    CHECK(gSCManagerBattleState->is_show_score == FALSE);
    CHECK(gSCManagerBattleState->is_not_teamshadows == TRUE);
    CHECK(gSCManagerBattleState->is_team_battle == TRUE);
    for (int i = 0; i < expected_allies; i++)
        CHECK(memcmp(&gSCManagerBattleState->players[gSCManagerSceneData.ally_players[i]],
                     &expected_ally[i], sizeof(Player)) == 0);
    sc1PGameSetupStageAll();
}
static void ndsPlatformSet3DLayerEnabled(int enabled) { CHECK(enabled); }
static void syVideoInit(void *setup) { CHECK(setup && phase == 0); phase = 1; }
static void ndsBattleRebudgetSceneSetup(SYTaskmanSetup *setup)
{ CHECK(setup && phase == 1); phase = 2; }
static void scManagerFuncUpdate(SYTaskmanSetup *setup)
{
    CHECK(setup->scene_setup.arena_start == arena);
    CHECK(setup->scene_setup.arena_size == sizeof(arena));
    CHECK(gSCManagerSceneData.scene_curr == nSCKind1PGame);
    CHECK(setup->func_start == sc1PGameFuncStart);
    setup->func_start();
}
static void sc1PGameInitBonusStats(void) { CHECK(phase == 3); phase = 4; }
static void syAudioStopBGMAll(void) { CHECK(phase == 4); phase = 5; }
static int syAudioCheckBGMPlaying(int player) { CHECK(player == 0 && phase == 5); return FALSE; }
static void syAudioSetBGMVolume(int player, int volume)
{ CHECK(player == 0 && volume == 0x7800 && phase == 5); phase = 6; }
static void func_800266A0_272A0(void) { CHECK(phase == 6); phase = 7; }
static void gmRumbleInitPlayers(void) { CHECK(phase == 7); phase = 8; }
static u32 gNdsSC1PGameBridgeStageRequested, gNdsSC1PGameBridgeAppliedCount;
static int refused;
static void ndsSC1PGameBridgeRefuse(u8 stage) { (void)stage; refused++; }
''' + function(candidate, "sc1PGameStartScene") + r'''
static void enter(int stage, int stock, int allies)
{
    gSCManagerSceneData.spgame_stage = stage;
    gSCManagerSceneData.scene_curr = nSCKind1PIntro;
    gSCManagerSceneData.spgame_time_limit = 5;
    gSCManagerBattleState = &other_state; /* The intro owned another scene state. */
    expected_stock = stock;
    expected_allies = allies;
    for (int i = 0; i < allies; i++)
        expected_ally[i] = gSCManager1PGameBattleState.players[gSCManagerSceneData.ally_players[i]];
    starts = phase = rng_calls = refused = 0;
    sc1PGameStartScene();
    CHECK(refused == 0 && starts == 1 && phase == 8);
    CHECK(gSCManager1PGameBattleState.players[gSCManagerSceneData.player].stock_count == stock);
    for (int i = 0; i < allies; i++) {
        Player *ally = &gSCManager1PGameBattleState.players[gSCManagerSceneData.ally_players[i]];
        CHECK(ally->fkind == expected_ally[i].fkind);
        CHECK(ally->costume == expected_ally[i].costume && ally->shade == expected_ally[i].shade);
        CHECK(ally->tag == nIFPlayerTagKindHeart && ally->team == 0);
    }
}
int main(void)
{
    for (int port = 0; port < 4; port++) {
        memset(&gSCManager1PGameBattleState, 0, sizeof(gSCManager1PGameBattleState));
        gSCManagerSceneData.player = port;
        gSCManagerSceneData.fkind = nFTKindMario;
        gSCManagerSceneData.ally_players[0] = (port + 1) % 4;
        gSCManagerSceneData.ally_players[1] = (port + 2) % 4;
        gSCManagerBackupData.spgame_stock_count = 4;
        gSCManagerBackupData.spgame_difficulty = nSC1PGameDifficultyNormal;
        gSC1PManagerLevelDrop = 0;
        Battle *state = &gSCManager1PGameBattleState;
        state->is_show_score = FALSE;
        state->is_not_teamshadows = state->is_team_battle = TRUE;
        state->players[port].fkind = nFTKindMario;
        state->players[port].pkind = nFTPlayerKindMan;
        state->players[port].stock_count = 4;
        enter(nSC1PGameStageLink, 4, 0);
        state->players[port].stock_count = 2; /* Lost two lives, then won. */
        enter(nSC1PGameStageYoshi, 2, 0);
        CHECK(rng_calls == SC1PGAME_STAGE_YOSHI_VARIATIONS_COUNT);
        CHECK(sSC1PGameTeamPlayersRemaining == 15 && state->cp_count == 3);
        enter(nSC1PGameStageFox, 2, 0);
        state->players[port].stock_count = gSCManagerBackupData.spgame_stock_count;
        gSC1PManagerLevelDrop = 1; /* Manager's continue branch refills stocks. */
        enter(nSC1PGameStageFox, 4, 0);
        CHECK(state->players[(port + 1) % 4].level ==
              dSC1PGameComputerDesc[nSC1PGameStageFox].enemy_level[nSC1PGameDifficultyNormal] - 1);
        state->players[(port + 1) % 4].fkind = nFTKindLuigi;
        state->players[(port + 1) % 4].costume = 1;
        state->players[(port + 1) % 4].shade = 0;
        enter(nSC1PGameStageMario, 4, 1);
        state->players[(port + 1) % 4].fkind = nFTKindSamus;
        state->players[(port + 2) % 4].fkind = nFTKindFox;
        enter(nSC1PGameStageDonkey, 4, 2);
        enter(nSC1PGameStageKirby, 4, 0);
        CHECK(rng_calls == 0 && state->cp_count == SC1PGAME_STAGE_KIRBY_SIM_COUNT);
        CHECK(sSC1PGamePlayerSetups[(port + 1) % 4].copy_kind == dSC1PGameKirbyTeamCopyKinds[0]);
        enter(nSC1PGameStageZako, 4, 0);
        CHECK(rng_calls == SC1PGAME_STAGE_MAX_VARIATIONS_COUNT);
        CHECK(sSC1PGameTeamPlayersRemaining == 27 && state->cp_count == 3);
        enter(nSC1PGameStageBonus3, 4, 0);
        CHECK(rng_calls == SC1PGAME_STAGE_MAX_OPPONENT_COUNT && state->time_limit == 1);
        for (int i = 1; i < 4; i++) {
            Player *enemy = &state->players[(port + i) % 4];
            CHECK(enemy->fkind >= nFTKindNStart && enemy->fkind <= nFTKindNEnd);
            CHECK(sSC1PGamePlayerSetups[(port + i) % 4].is_skip_entry);
        }
        state->players[port].stock_count = 0; /* Challenger's manager-owned stock. */
        enter(nSC1PGameStageLuigi, 0, 0);
        CHECK(state->time_limit == SCBATTLE_TIMELIMIT_INFINITE);
    }
    return 0;
}
'''


class CampaignBattleStateTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workspace = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.workspace.cleanup)
        cls.directory = Path(cls.workspace.name)
        copy = cls.directory / TU
        copy.parent.mkdir(parents=True)
        copy.write_text((ROOT / TU).read_text(), newline="\n")
        if PATCH:
            subprocess.run(["git", "apply", "--no-index", str(PATCH)],
                           cwd=cls.directory, check=True)
        cls.candidate = copy.read_text()

    def test_source_ownership_and_cleanup(self):
        wrapper = clean(function(self.candidate, "sc1PGameStartScene"))
        for forbidden in ("ndsMatchConfigApply", "gSCManagerTransferBattleState",
                          "gSCManagerBackupData", "syUtilsRandIntRange", "NdsMatchConfig",
                          "sc1PGameSetupStageAll"):
            self.assertNotIn(forbidden, wrapper)
        source = (DECOMP / "sc/sc1pmode/sc1pgame.c").read_text()
        start = clean(function(source, "sc1PGameFuncStart"))
        self.assertEqual(start.count("sc1PGameSetupStageAll();"), 1)
        self.assertLess(start.index("sc1PGameSetupStageAll();"), start.index("ftManagerMakeFighter("))
        self.assertIn("#include <battleship_overlay/src/sc/sc1pmode/sc1pgame.c>", self.candidate)
        original = clean(function(source, "sc1PGameStartScene"))
        cleanup = "sc1PGameInitBonusStats();"
        self.assertEqual(re.sub(r"\s+", "", wrapper[wrapper.index(cleanup):]),
                         re.sub(r"\s+", "", original[original.index(cleanup):]))

    def run_host(self, candidate):
        compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc")
                         if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required (clang/gcc/cc)")
        source = self.directory / "campaign.c"
        program = self.directory / "campaign.exe"
        source.write_text(host_source(candidate))
        subprocess.run([compiler, "-std=c11", "-DREGION_US=1", "-Wall", "-Wextra",
                        "-Wno-unused-variable", str(source), "-o", str(program)], check=True)
        return subprocess.run([str(program)], capture_output=True, text=True)

    def test_manager_state_and_original_stage_setup(self):
        result = self.run_host(self.candidate)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_negative_controls(self):
        anchor = "    gSCManagerBattleState->game_type = nSCBattleGameType1PGame;"
        self.assertEqual(self.candidate.count(anchor), 1)
        for regression in (
            "gSCManagerBattleState->players[gSCManagerSceneData.player].stock_count = "
            "gSCManagerBackupData.spgame_stock_count;",
            "sc1PGameSetupStageAll();",
        ):
            with self.subTest(regression=regression):
                candidate = self.candidate.replace(anchor, anchor + "\n    " + regression)
                self.assertNotEqual(self.run_host(candidate).returncode, 0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--patch", type=Path)
    options, unittest_args = parser.parse_known_args()
    PATCH = options.patch.resolve() if options.patch else None
    unittest.main(argv=[__file__, *unittest_args])
