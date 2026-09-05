/* P2-6 step 1. 1P Game runtime half: setup, spawn, team-next + first-stage boot.
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c, following
 * battleship_sc1pbonusstage.c (import as ndsBase* and re-export under the
 * source name). Only the step-1 slice is re-exported: sc1PGameSetupEnemyPlayer
 * (decomp :948), sc1PGameSetupStageAll (:977), sc1PGameSpawnEnemyTeamNext
 * (:1314) and sc1PGameSetPlayerDefeatStats (:1780). The two team hooks replace
 * the NDS_WEAK stubs in src/port/battle_playable_compat_stubs.c:98-107 by
 * normal strong-over-weak link override; that file is untouched.
 *
 * Excluded rather than duplicated (each renamed symbol has exactly one
 * definition elsewhere):
 * - dSC1PGameKirbyTeamCopyKinds, dSC1PGameComputerDesc, dSC1PGameStageDesc:
 *   already defined in battleship_sc1pgame_tables.c. The included bodies
 *   below read PRIVATE identical-value copies (ndsExcluded*); the DS bridge
 *   further below reads the REAL tables. Values are identical by source, but
 *   the addresses differ: a compile-time `&` comparison would be the first
 *   to show it. Step 1 only boots stage 0 (Link), whose row the bridge reads
 *   from the real table, so no behaviour diverges on the verified path.
 * - gSC1PGameBonusTomatoCount/HeartCount/StarCount/ShieldBreaker/
 *   GiantImpact/MewCatcher: already defined by the port (ftcommon_get.c:40-41,
 *   compat_shims.c:394-395, shieldbreakfly.c:26 weak, item_map_core.c:167).
 *   Same private-copy rule; the public SetupStageAll wrapper below zeroes the
 *   REAL six after calling the base, so the stage-start reset the source does
 *   at sc1pgame.c:1278-1285 still holds where the tally (step 2) will read it.
 *   gSC1PGameBonusBrosCalamity and the Attack/Defend arrays have no port
 *   definition and are defined here once, normally.
 * - sc1PGameStartScene: excluded from the include; the DS bridge below OWNS
 *   that name (first-stage boot through ndsMatchConfigApply, NOT the N64
 *   taskman/video boot at sc1pgame.c:2898-2920). dSC1PGameTaskmanSetup /
 *   dSC1PGameVideoSetup / sc1PGameFuncStart / sc1PGameFuncLights stay as
 *   ordinary included definitions: nothing in step 1 references them, so the
 *   linker drops them (with the missing ovl65_BSS_END reference inside the
 *   taskman setup); a kept reference would fail the link, which is the
 *   honest signal that the full N64 boot path is not imported.
 *
 * Difficulty/stock wiring (decomp mn/mnplayers/mnplayers1pgame.c:3262-3279):
 * mnPlayers1PGameSetSceneData below mirrors SetSceneData minus the menu-owned
 * sMNPlayers1PGame* statics, which land with the menus in step 8: stage 0,
 * port 0, Mario in the player slot, difficulty/stock/time preserved from the
 * current backup/scene, then lbBackupWrite. The sc1PGameStartScene bridge
 * carries difficulty (enemy/ally level+handicap via dSC1PGameComputerDesc[0]),
 * stock (match stocks from backup) and stage (Hyrule from dSC1PGameStageDesc[0])
 * onto the port match config through ndsMatchConfigApply. Non-Link stages are
 * NOT implemented here: the bridge records the requested stage and returns
 * without applying, rather than inventing gameplay.
 *
 * DS deltas on the verified path (Link/Hyrule, Mario, step 1):
 * - Enemy single-stock (source sets BattleState stock_count 0 at :967) cannot
 *   be expressed in one match-wide NdsMatchConfig.stocks alongside the
 *   player's spgame_stock_count; the bridge sets match stocks from the backup
 *   and leaves per-slot 1P stock seeding to step 3 (team hooks own it).
 * - game_type 1PGame is set explicitly (NdsMatchConfig does not model it).
 * - Intro/continue/clear/challenger/ending/credits scenes stay stubs, so the
 *   full manager loop is linkable but not yet runnable; the bridge alone boots
 *   the fight.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <ssb_types.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <mn/menu.h>
#include <nds/nds_match_config.h>
#include <sc/scene.h>
#include <sys/dma.h>
#include <sys/video.h>
#include <reloc_data.h>

/* Same local extern as every other import TU; no port header publishes it. */
s32 syUtilsRandIntRange(s32 range);

/* decomp ft/ftdef.h:5 verbatim (port include/ft/fighter.h lacks it; the
 * included TU uses it at sc1pgame.c:1385 setup path). */
#ifndef FTCOMMON_HANDICAP_DEFAULT
#define FTCOMMON_HANDICAP_DEFAULT 9
#endif

/* decomp sc/scdef.h:220 verbatim: nSCBattleTeamIDCom = nSCBattleTeamIDBattleEnd
 * + 1 = 3. Port include/sc/scene.h:375-381 stops at BattleEnd; the bridge
 * and the included SetupEnemyPlayer (sc1pgame.c:961) both need Com. Macro,
 * not enum, so a later header promotion collides loudly instead of silently. */
#ifndef nSCBattleTeamIDCom
#define nSCBattleTeamIDCom 3
#endif

/* dSC1PGameTaskmanSetup (included, unreferenced, linker-dropped) takes the
 * address of ovl65_BSS_END, which the port does not declare (DECLARE_OVL
 * stops at 64). Declared here so the definition compiles; nothing in step 1
 * keeps it, so nothing links against it. */
extern uintptr_t ovl65_BSS_END;

/* Owned by the step-1 pair battleship_sc1pmanager.c (originals there). */
extern s32 gSC1PManagerLevelDrop;
extern u8 gSC1PManagerKirbyTeamFinalCopy;

/* Step-1 slice: imported as ndsBase*, re-exported under the source name. */
#define sc1PGameSetupEnemyPlayer ndsBaseSC1PGameSetupEnemyPlayer
#define sc1PGameSetupStageAll ndsBaseSC1PGameSetupStageAll
#define sc1PGameSpawnEnemyTeamNext ndsBaseSC1PGameSpawnEnemyTeamNext
#define sc1PGameSetPlayerDefeatStats ndsBaseSC1PGameSetPlayerDefeatStats
void ndsBaseSC1PGameSetupEnemyPlayer(SC1PGameStage *stagesetup, SC1PGameComputer *comsetup, s32 player, s32 enemy_player_num);
void ndsBaseSC1PGameSetupStageAll(void);
void ndsBaseSC1PGameSpawnEnemyTeamNext(GObj *player_gobj);
void ndsBaseSC1PGameSetPlayerDefeatStats(s32 player, s32 team_order);

/* Already defined in battleship_sc1pgame_tables.c: private copies here. */
#define dSC1PGameKirbyTeamCopyKinds ndsExcludedSC1PGameKirbyTeamCopyKinds
#define dSC1PGameComputerDesc ndsExcludedSC1PGameComputerDesc
#define dSC1PGameStageDesc ndsExcludedSC1PGameStageDesc

/* Already defined by the port (see file doc): private copies here. */
#define gSC1PGameBonusTomatoCount ndsExcludedSC1PGameBonusTomatoCount
#define gSC1PGameBonusHeartCount ndsExcludedSC1PGameBonusHeartCount
#define gSC1PGameBonusStarCount ndsExcludedSC1PGameBonusStarCount
#define gSC1PGameBonusShieldBreaker ndsExcludedSC1PGameBonusShieldBreaker
#define gSC1PGameBonusGiantImpact ndsExcludedSC1PGameBonusGiantImpact
#define gSC1PGameBonusMewCatcher ndsExcludedSC1PGameBonusMewCatcher

/* N64 boot path is not imported in step 1; the DS bridge below owns the name. */
#define sc1PGameStartScene ndsExcludedSC1PGameStartScene

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c"

#undef sc1PGameStartScene
#undef gSC1PGameBonusMewCatcher
#undef gSC1PGameBonusGiantImpact
#undef gSC1PGameBonusShieldBreaker
#undef gSC1PGameBonusStarCount
#undef gSC1PGameBonusHeartCount
#undef gSC1PGameBonusTomatoCount
#undef dSC1PGameStageDesc
#undef dSC1PGameComputerDesc
#undef dSC1PGameKirbyTeamCopyKinds
#undef sc1PGameSetPlayerDefeatStats
#undef sc1PGameSpawnEnemyTeamNext
#undef sc1PGameSetupStageAll
#undef sc1PGameSetupEnemyPlayer

/* Real tables (battleship_sc1pgame_tables.c) for the DS bridge. The port
 * header does not declare them; neither does any other TU need to. */
extern SC1PGameStage dSC1PGameStageDesc[];
extern SC1PGameComputer dSC1PGameComputerDesc[];

/* Real bonus counters (port owners) for the stage-start reset. Types match
 * the port definitions: ShieldBreaker is sb32-weak in shieldbreakfly.c:26,
 * GiantImpact is u8 in compat_shims.c:395, MewCatcher is ub8 in
 * item_map_core.c:167, Tomato/Heart/Star are u8 in ftcommon_get.c:40-41 and
 * compat_shims.c:394. Decomp types differ for ShieldBreaker (ub8); writing 0
 * is representation-safe on either width and the tally (step 2) will unify. */
extern u8 gSC1PGameBonusTomatoCount;
extern u8 gSC1PGameBonusHeartCount;
extern u8 gSC1PGameBonusStarCount;
extern sb32 gSC1PGameBonusShieldBreaker;
extern u8 gSC1PGameBonusGiantImpact;
extern ub8 gSC1PGameBonusMewCatcher;

/* Step-1 link diagnostics: the bridge is Link-only. */
volatile u32 gNdsSC1PGameBridgeStageRequested;
volatile u32 gNdsSC1PGameBridgeAppliedCount;

void sc1PGameSetupEnemyPlayer(SC1PGameStage *stagesetup, SC1PGameComputer *comsetup, s32 player, s32 enemy_player_num)
{
    ndsBaseSC1PGameSetupEnemyPlayer(stagesetup, comsetup, player, enemy_player_num);
}

void sc1PGameSetupStageAll(void)
{
    ndsBaseSC1PGameSetupStageAll();

    /* The base above zeroed the private copies; zero the real counters the
     * tally will read (source sc1pgame.c:1278-1285). */
    gSC1PGameBonusTomatoCount = 0;
    gSC1PGameBonusHeartCount = 0;
    gSC1PGameBonusStarCount = 0;
    gSC1PGameBonusShieldBreaker = FALSE;
    gSC1PGameBonusGiantImpact = FALSE;
    gSC1PGameBonusMewCatcher = FALSE;
}

void sc1PGameSpawnEnemyTeamNext(GObj *player_gobj)
{
    ndsBaseSC1PGameSpawnEnemyTeamNext(player_gobj);
}

void sc1PGameSetPlayerDefeatStats(s32 player, s32 team_order)
{
    ndsBaseSC1PGameSetPlayerDefeatStats(player, team_order);
}

/* decomp mn/mnplayers/mnplayers1pgame.c:3262-3279, minus the menu-owned
 * sMNPlayers1PGame* statics (step 8). Stage 0, port 0, Mario; difficulty,
 * stock and time ride through from the current backup/scene. */
void mnPlayers1PGameSetSceneData(void)
{
    gSCManagerSceneData.player = 0;
    gSCManagerSceneData.spgame_stage = (u8)nSC1PGameStageLink;
    gSCManagerSceneData.fkind = (u8)nFTKindMario;
    gSCManagerSceneData.costume = 0;

    lbBackupWrite();
}

/* DS first-stage boot. Stage 0 (Link on Hyrule) only: builds the port match
 * descriptor from the real ladder tables plus backup difficulty/stock and
 * applies it. Any other stage is recorded and left unapplied. */
void sc1PGameStartScene(void)
{
    SC1PGameStage *stagesetup;
    SC1PGameComputer *comsetup;
    NdsMatchConfig cfg;
    s32 i;
    s32 difficulty;
    s32 level;
    s32 enemy;

    gNdsSC1PGameBridgeStageRequested = gSCManagerSceneData.spgame_stage;

    if (gSCManagerSceneData.spgame_stage != (u8)nSC1PGameStageLink)
    {
        return;
    }
    stagesetup = &dSC1PGameStageDesc[nSC1PGameStageLink];
    comsetup = &dSC1PGameComputerDesc[nSC1PGameStageLink];

    difficulty = (s32)gSCManagerBackupData.spgame_difficulty;
    if (difficulty < (s32)nSC1PGameDifficultyVeryEasy)
    {
        difficulty = (s32)nSC1PGameDifficultyVeryEasy;
    }
    if (difficulty > (s32)nSC1PGameDifficultyVeryHard)
    {
        difficulty = (s32)nSC1PGameDifficultyVeryHard;
    }
    level = (s32)comsetup->enemy_level[difficulty] - gSC1PManagerLevelDrop;
    if (level <= 0)
    {
        level = 1;
    }
    if (gSCManagerSceneData.player >= (u8)GMCOMMON_PLAYERS_MAX)
    {
        gSCManagerSceneData.player = 0;
    }
    enemy = (gSCManagerSceneData.player == (u8)(GMCOMMON_PLAYERS_MAX - 1)) ? 0 :
        (s32)gSCManagerSceneData.player + 1;

    for (i = 0; i < NDS_MATCH_FIGHTERS_MAX; i++)
    {
        cfg.fighters[i].fkind = (u8)nFTKindNull;
        cfg.fighters[i].pkind = (u8)nFTPlayerKindNot;
        cfg.fighters[i].level = 1;
        cfg.fighters[i].handicap = 0;
        cfg.fighters[i].team = 0;
        cfg.fighters[i].costume = 0;
        cfg.fighters[i].shade = 0;
        cfg.fighters[i].color = 0;
    }
    cfg.gkind = stagesetup->gkind;
    cfg.game_rules = (u8)SCBATTLE_GAMERULE_TIME;
    cfg.time_limit = gSCManagerSceneData.spgame_time_limit;
    cfg.stocks = gSCManagerBackupData.spgame_stock_count;
    cfg.handicap_mode = (u8)nSCBattleHandicapOff;
    cfg.item_appearance_rate = comsetup->item_appearance_rate;
    cfg.damage_ratio = 100;
    cfg.is_team_battle = TRUE;
    cfg.is_team_attack = comsetup->is_team_attack;
    cfg.is_stage_select = FALSE;
    cfg.is_reset_players = FALSE;
    cfg.item_toggles = stagesetup->item_toggles;

    cfg.fighters[gSCManagerSceneData.player].fkind = gSCManagerSceneData.fkind;
    cfg.fighters[gSCManagerSceneData.player].pkind = (u8)nFTPlayerKindMan;
    cfg.fighters[gSCManagerSceneData.player].handicap = (u8)FTCOMMON_HANDICAP_DEFAULT;
    cfg.fighters[gSCManagerSceneData.player].team = 0;
    cfg.fighters[gSCManagerSceneData.player].costume = gSCManagerSceneData.costume;

    cfg.fighters[enemy].fkind = stagesetup->fkind[0];
    cfg.fighters[enemy].pkind = (u8)nFTPlayerKindCom;
    cfg.fighters[enemy].level = (u8)level;
    cfg.fighters[enemy].handicap = comsetup->enemy_handicap[difficulty];
    cfg.fighters[enemy].team = (u8)nSCBattleTeamIDCom;
    cfg.fighters[enemy].costume = 0;

    ndsMatchConfigApply(&cfg);

    gSCManager1PGameBattleState = gSCManagerTransferBattleState;
    gSCManager1PGameBattleState.game_type = (u8)nSCBattleGameType1PGame;
    gSCManagerBattleState = &gSCManager1PGameBattleState;

    gNdsSC1PGameBridgeAppliedCount++;
}

#endif /* NDS_P2_1P_GAME */
