/* P2-6 step 1 (+ full-ladder bridge). 1P Game runtime half: setup, spawn,
 * team-next + full-ladder boot.
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c, following
 * battleship_sc1pbonusstage.c. sc1PGameSetupEnemyPlayer (decomp :948),
 * sc1PGameSetupStageAll (:977), sc1PGameSpawnEnemyTeamNext (:1314) and
 * sc1PGameSetPlayerDefeatStats (:1780) run whole from the include. The two
 * team hooks replace the NDS_WEAK stubs in
 * src/port/battle_playable_compat_stubs.c:98-107 by normal strong-over-weak
 * link override; that file is untouched.
 *
 * Everything sc1pgame.c defines -- the ladder tables, the bonus counters,
 * setup/spawn/team-next/defeat-stats -- is defined here ONCE under its
 * source name. The port's flag-off owners of the six bonus counters
 * (ftcommon_get.c, item_map_core.c, compat_shims.c) are gated out by
 * NDS_P2_1P_GAME, and shieldbreakfly.c's weak owner is ub8 like the
 * source so the strong definition here has the same width. Only
 * sc1PGameStartScene is excluded from the include: the DS bridge below
 * OWNS that name (ladder-to-descriptor through ndsMatchConfigApply, NOT the
 * N64 taskman/video boot at sc1pgame.c:2898-2920). dSC1PGameTaskmanSetup /
 * dSC1PGameVideoSetup / sc1PGameFuncStart / sc1PGameFuncLights stay as
 * ordinary included definitions: nothing here references them, so the
 * linker drops them (with the missing ovl65_BSS_END reference inside the
 * taskman setup); a kept reference would fail the link, which is the
 * honest signal that the full N64 boot path is not imported.
 *
 * Difficulty/stock wiring: mnPlayers1PGameSetSceneData is the source's own,
 * from the mnplayers1pgame.c include (step 8). The sc1PGameStartScene bridge
 * builds the port match descriptor for any ladder spgame_stage from the real
 * tables (dSC1PGameStageDesc / dSC1PGameComputerDesc), the backup difficulty
 * (mnplayers1pgame.c:3266), the continue level drop (sc1pgame.c:950-957) and
 * the backup stock count, and applies it through ndsMatchConfigApply, which
 * carries the 1P-only fields (per-slot stock_count / is_spgame_enemy, team
 * Com, game_type 1PGame, spgame_stage) onto gSCManagerTransferBattleState
 * and the per-fighter slots beside their VS siblings. The bridge also seeds
 * the included TU's wave globals (enemy counts, variation shuffle, per-port
 * setups) exactly as sc1PGameSetupStageAll does, because the runtime team
 * hooks read them. Spawn positions (mapobj_kind) are NOT seeded: the DS
 * fighter creation path reads the transfer block, never the N64 map objects.
 *
 * NOT this path (recorded and left unapplied, never faked):
 * - Bonus 1/2: the manager's bonus branch (sc1pmanager.c:370-378) routes them
 *   to sc1PBonusStageStartScene on the 1PBonusStageOverlay, owned by
 *   battleship_sc1pbonusstage.c.
 * - Bonus 3 / Boss: they reach sc1PGameStartScene in source (sc1pmanager.c:
 *   381-385 default arm), but Bonus 3 needs the scrolling Race venue
 *   (grBonus3MakeGround is a stub, battleship_grpupupu_ground.c:709;
 *   grbonus3.c) and Boss needs the Last venue plus sc1pgameboss.c
 *   (sc1PGameBossSetBossPlayer / wallpaper / camera), so the bridge leaves
 *   both to those owners.
 * - Venues with no wired native packet (YosterSmall, Metal, Zako, Last): no
 *   arm in grMainSetupMakeGround (grmainsetup.c:11-22 lists the nine VS kinds
 *   only), no grCommonSetupInitAll admission (battleship_grpupupu_ground.c
 *   admits Pupupu plus the NDS_P2_STAGE_* VS kinds, compatibility stub
 *   otherwise); native descriptors scripts/stages/native_stage_descriptors/
 *   {yostersmall,metal,zako,last}.py are unwired. They wait on the P2-4 1P
 *   venue path.
 * - Variant kinds with no admitted behaviour/stats (GDonkey, MMario): Jungle
 *   is wired but Giant DK waits on the variant-stats step (ftmanager.c:
 *   587-588 resist 48.0 plus scale, P2-6 step 4); Metal is unwired AND Metal
 *   Mario waits on ftmanager.c:577-578 resist 30.0. Polygon variants
 *   (nFTKindNStart..NEnd, sc1pgame.c:1160-1201 traits) wait on the same step;
 *   Boss waits on the ftboss statuses + sc1pgameboss.c step.
 * Base-roster kinds ride their own NDS_P2_* admission flags at build time.
 *
 * DS deltas on the verified path (Link/Hyrule, Mario, step 1), kept for all
 * stages: game_rules carry the 1PGAME bit (sc1pmanager.c:268) so the derived
 * stock icon matches enemies/allies; the player icon override lives in
 * ndsMatchConfigApply. Kirby initial copy abilities (dSC1PGameKirbyTeamCopyKinds
 * :1219, final copy gSC1PManagerKirbyTeamFinalCopy from sc1pmanager.c:352-358)
 * are seeded into the per-port setups for the runtime waves, but the DS
 * fighter creation path has no copy_kind input, so the opening two Kirbys
 * fight without a copied power until that plumbing lands. Ally kind picks
 * stay the manager's (sc1pmanager.c:326-350); the bridge seats their
 * levels/teams/stocks from ally_players[] and reads kinds/costumes from the
 * live 1PGameBattleState the manager seeded.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <ssb_types.h>
#include <ft/fighter.h>
#include <ft/ftcomputer.h>
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

/* dSC1PGameTaskmanSetup (included, unreferenced, linker-dropped) takes the
 * address of ovl65_BSS_END, which the port does not declare (DECLARE_OVL
 * stops at 64). Declared here so the definition compiles; nothing in step 1
 * keeps it, so nothing links against it. */
extern uintptr_t ovl65_BSS_END;

/* Owned by the step-1 pair battleship_sc1pmanager.c (originals there). */
extern s32 gSC1PManagerLevelDrop;
extern u8 gSC1PManagerKirbyTeamFinalCopy;

/* The N64 boot path is not imported in step 1; the DS bridge below owns the
 * name. Everything else in sc1pgame.c is defined here once (file doc). */
#define sc1PGameStartScene ndsExcludedSC1PGameStartScene

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c"

#undef sc1PGameStartScene

/* Bridge diagnostics: what was asked, what booted, what was refused and why.
 * RefusedStage is indexed by spgame_stage (CommonStart..ChallengerEnd). */
volatile u32 gNdsSC1PGameBridgeStageRequested;
volatile u32 gNdsSC1PGameBridgeAppliedCount;
volatile u32 gNdsSC1PGameBridgeRefusedCount;
volatile u32 gNdsSC1PGameBridgeRefusedStage[nSC1PGameStageChallengerEnd + 1];

static void ndsSC1PGameBridgeRefuse(u8 stage)
{
    gNdsSC1PGameBridgeRefusedCount++;
    if (stage <= (u8)nSC1PGameStageChallengerEnd)
    {
        gNdsSC1PGameBridgeRefusedStage[stage]++;
    }
}

/* decomp sc1pgame.c:839-877 verbatim over the descriptor: first common
 * costume id no occupied slot already uses for this kind. Occupancy here is
 * the descriptor (empty init, then player, then allies in seating order),
 * which mirrors the live BattleState the source scans: SetupStageAll clears
 * non-player slots (:1027-1030) and seats allies before enemies (:1069-1090). */
static s32 ndsSC1PGameBridgeFreeCostume(NdsMatchConfig *cfg, s32 com, s32 fkind)
{
    s32 used_costume = 0;
    s32 player = 0;
    s32 cp_costume;

get_costume:
    cp_costume = ftParamGetCostumeCommonID(fkind, used_costume);
    for (player = 0; player < NDS_MATCH_FIGHTERS_MAX; player++)
    {
        if (player == com)
        {
            continue;
        }
        if (cfg->fighters[player].pkind == (u8)nFTPlayerKindNot)
        {
            continue;
        }
        if (((s32)cfg->fighters[player].fkind == fkind) &&
            ((s32)cfg->fighters[player].costume == cp_costume))
        {
            used_costume++;
            goto get_costume;
        }
    }
    return cp_costume;
}

/* DS full-ladder boot: one descriptor per spgame_stage from the two ladder
 * tables, the backup difficulty, the continue level drop and the backup
 * stock count, applied through ndsMatchConfigApply. Field-by-field the
 * source is sc1PGameSetupEnemyPlayer (:948-974) per enemy,
 * sc1PGameSetupStageAll (:977-1311) for the stage frame, and the manager
 * pre-seed (sc1pmanager.c:267-287) for the player slot. Bonus stages and the
 * boss stage refuse (file doc); unwired venues and unadmitted variants
 * refuse loudly rather than booting a wrong match. */
void sc1PGameStartScene(void)
{
    u8 stage = gSCManagerSceneData.spgame_stage;
    SC1PGameStage *stagesetup;
    SC1PGameComputer *comsetup;
    NdsMatchConfig cfg;
    s32 i;
    s32 difficulty;
    s32 level;
    s32 cursor;
    s32 initial;
    s32 is_challenger;
    s32 is_yoshi;
    s32 is_zako;
    s32 is_kirby;
    s32 variations;
    u16 flags;
    s32 cycle;

    gNdsSC1PGameBridgeStageRequested = stage;

    if (stage > (u8)nSC1PGameStageChallengerEnd)
    {
        ndsSC1PGameBridgeRefuse(stage);
        return;
    }
    if ((stage == (u8)nSC1PGameStageBonus1) ||
        (stage == (u8)nSC1PGameStageBonus2) ||
        (stage == (u8)nSC1PGameStageBonus3) ||
        (stage == (u8)nSC1PGameStageBoss))
    {
        ndsSC1PGameBridgeRefuse(stage);
        return;
    }
    stagesetup = &dSC1PGameStageDesc[stage];
    comsetup = &dSC1PGameComputerDesc[stage];

    /* Venue admission (file doc): YosterSmall / Metal / Zako / Last have no
     * wired native packet. Bonus3's gkind never reaches here (refused above). */
    if ((stagesetup->gkind == (u8)nGRKindYosterSmall) ||
        (stagesetup->gkind == (u8)nGRKindMetal) ||
        (stagesetup->gkind == (u8)nGRKindZako) ||
        (stagesetup->gkind == (u8)nGRKindLast))
    {
        ndsSC1PGameBridgeRefuse(stage);
        return;
    }
    /* Variant admission (file doc): Giant DK, Metal Mario, polygon kinds and
     * Boss have no admitted behaviour/stats yet. Base kinds ride their
     * NDS_P2_* build flags. */
    for (i = 0; i < 2; i++)
    {
        if ((stagesetup->fkind[i] == (u8)nFTKindGDonkey) ||
            (stagesetup->fkind[i] == (u8)nFTKindMMario) ||
            (stagesetup->fkind[i] == (u8)nFTKindBoss) ||
            ((stagesetup->fkind[i] >= (u8)nFTKindNStart) &&
             (stagesetup->fkind[i] <= (u8)nFTKindNEnd)))
        {
            ndsSC1PGameBridgeRefuse(stage);
            return;
        }
    }

    is_challenger = (stage >= (u8)nSC1PGameStageChallengerStart) ? TRUE : FALSE;
    is_yoshi = (stage == (u8)nSC1PGameStageYoshi) ? TRUE : FALSE;
    is_zako = (stage == (u8)nSC1PGameStageZako) ? TRUE : FALSE;
    is_kirby = (stage == (u8)nSC1PGameStageKirby) ? TRUE : FALSE;

    /* Backup difficulty (mnplayers1pgame.c:3266), clamped to the table. */
    difficulty = (s32)gSCManagerBackupData.spgame_difficulty;
    if (difficulty < (s32)nSC1PGameDifficultyVeryEasy)
    {
        difficulty = (s32)nSC1PGameDifficultyVeryEasy;
    }
    if (difficulty > (s32)nSC1PGameDifficultyVeryHard)
    {
        difficulty = (s32)nSC1PGameDifficultyVeryHard;
    }
    /* Enemy level minus the continue drop, floored at 1 (:950-957). */
    level = (s32)comsetup->enemy_level[difficulty] - gSC1PManagerLevelDrop;
    if (level <= 0)
    {
        level = 1;
    }
    if (is_challenger != FALSE)
    {
        /* Challenger rematch softening (:1102-1104). */
        level -= (s32)gSCManagerSceneData.challenger_level_drop;
        if (level <= 0)
        {
            level = 1;
        }
    }
    if (gSCManagerSceneData.player >= (u8)GMCOMMON_PLAYERS_MAX)
    {
        gSCManagerSceneData.player = 0;
    }

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
        cfg.fighters[i].stock_count = 0;
        cfg.fighters[i].is_spgame_enemy = FALSE;
        /* Per-port setup reset (:1017-1031, minus mapobj_kind: the DS fighter
         * creation path never reads N64 map objects). */
        sSC1PGamePlayerSetups[i].figatree = NULL;
        sSC1PGamePlayerSetups[i].copy_kind = nFTKindKirby;
        sSC1PGamePlayerSetups[i].team_order = 0;
        sSC1PGamePlayerSetups[i].is_skip_entry = FALSE;
        sSC1PGamePlayerSetups[i].is_magnify_ignore = FALSE;
        sSC1PGamePlayerSetups[i].cp_trait = nFTComputerTraitDefault;
        sSC1PGamePlayerSetups[i].camera_frame_mul = 1.0F;
    }

    /* Stage frame (:990-1011 manager:267-269). */
    cfg.gkind = stagesetup->gkind;
    cfg.game_rules = (u8)(SCBATTLE_GAMERULE_1PGAME | SCBATTLE_GAMERULE_TIME);
    cfg.game_type = (u8)nSCBattleGameType1PGame;
    cfg.spgame_stage = stage;
    if (is_challenger != FALSE)
    {
        cfg.time_limit = SCBATTLE_TIMELIMIT_INFINITE;
    }
    else
    {
        cfg.time_limit = gSCManagerSceneData.spgame_time_limit;
    }
    cfg.stocks = gSCManagerBackupData.spgame_stock_count;
    cfg.handicap_mode = (u8)nSCBattleHandicapOff;
    cfg.item_appearance_rate = comsetup->item_appearance_rate;
    cfg.damage_ratio = 100;
    cfg.is_team_battle = TRUE;
    cfg.is_team_attack = comsetup->is_team_attack;
    cfg.is_stage_select = FALSE;
    cfg.is_reset_players = FALSE;
    cfg.item_toggles = stagesetup->item_toggles;

    /* Wave counts (:1013-1015); TeamPlayersRemaining nets the :973 decrements
     * for the initial spawns seated below. */
    if ((is_yoshi != FALSE) || (is_zako != FALSE))
    {
        initial = SC1PGAME_STAGE_MAX_OPPONENT_COUNT;
    }
    else if (is_kirby != FALSE)
    {
        initial = SC1PGAME_STAGE_KIRBY_SIM_COUNT;
    }
    else
    {
        initial = (s32)stagesetup->opponent_count;
    }
    sSC1PGameEnemyPlayerCount = sSC1PGameEnemyStocksRemaining = stagesetup->opponent_count;
    sSC1PGameEnemyStockSpriteFlags = 0;
    sSC1PGameTeamPlayersRemaining = (u8)((s32)stagesetup->opponent_count - initial);

    /* Player slot (manager:278-287; challenger single-stock at :518). */
    cfg.fighters[gSCManagerSceneData.player].fkind = gSCManagerSceneData.fkind;
    cfg.fighters[gSCManagerSceneData.player].pkind = (u8)nFTPlayerKindMan;
    cfg.fighters[gSCManagerSceneData.player].handicap = (u8)FTCOMMON_HANDICAP_DEFAULT;
    cfg.fighters[gSCManagerSceneData.player].team = 0;
    cfg.fighters[gSCManagerSceneData.player].costume = gSCManagerSceneData.costume;
    cfg.fighters[gSCManagerSceneData.player].shade = 0;
    if (is_challenger != FALSE)
    {
        cfg.fighters[gSCManagerSceneData.player].stock_count = 0;
    }
    else
    {
        cfg.fighters[gSCManagerSceneData.player].stock_count =
            (s8)gSCManagerBackupData.spgame_stock_count;
    }
    cfg.fighters[gSCManagerSceneData.player].is_spgame_enemy = FALSE;

    /* Ally rotation (manager:302-311): deterministic and idempotent, so the
     * bridge stays correct whether or not the manager loop ran. Kinds and
     * costumes stay the manager's pick (:326-350), read live. */
    cursor = (s32)gSCManagerSceneData.player;
    for (i = 0; i < 2; i++)
    {
        cursor = (cursor == (GMCOMMON_PLAYERS_MAX - 1)) ? 0 : cursor + 1;
        gSCManagerSceneData.ally_players[i] = (u8)cursor;
    }
    /* Allies (:1069-1085): raw ally tables, no level drop; team 0. */
    for (i = 0; i < (s32)stagesetup->ally_count; i++)
    {
        s32 ally = (s32)gSCManagerSceneData.ally_players[i];

        cfg.fighters[ally].fkind =
            gSCManager1PGameBattleState.players[ally].fkind;
        cfg.fighters[ally].pkind = (u8)nFTPlayerKindCom;
        cfg.fighters[ally].level = comsetup->ally_level[difficulty];
        cfg.fighters[ally].handicap = comsetup->ally_handicap[difficulty];
        cfg.fighters[ally].team = 0;
        cfg.fighters[ally].costume =
            gSCManager1PGameBattleState.players[ally].costume;
        cfg.fighters[ally].shade =
            gSCManager1PGameBattleState.players[ally].shade;
        cfg.fighters[ally].stock_count = 0;
        cfg.fighters[ally].is_spgame_enemy = FALSE;
        sSC1PGamePlayerSetups[ally].cp_trait = stagesetup->ally_behavior;
    }

    /* Team-wave variation shuffle (:1113-1134 Yoshi, :1160-1183 Zako). */
    if (is_yoshi != FALSE)
    {
        variations = SC1PGAME_STAGE_YOSHI_VARIATIONS_COUNT;
        flags = 0;
        for (i = 0; i < SC1PGAME_STAGE_YOSHI_VARIATIONS_COUNT; i++)
        {
            sSC1PGameEnemyVariations[i] =
                (u8)sc1PGameGetFighterKindsNum(flags, syUtilsRandIntRange(variations));
            flags |= (u16)(1 << sSC1PGameEnemyVariations[i]);
            variations--;
        }
        for (cycle = 0, i = SC1PGAME_STAGE_YOSHI_VARIATIONS_COUNT;
             i < (s32)stagesetup->opponent_count; i++)
        {
            sSC1PGameEnemyVariations[i] = sSC1PGameEnemyVariations[cycle];
            cycle = (cycle == (SC1PGAME_STAGE_YOSHI_VARIATIONS_COUNT - 1)) ?
                0 : cycle + 1;
        }
        sSC1PGameCurrentEnemyVariation = 0;
    }
    if (is_zako != FALSE)
    {
        variations = SC1PGAME_STAGE_MAX_VARIATIONS_COUNT;
        flags = 0;
        for (i = 0; i < SC1PGAME_STAGE_MAX_VARIATIONS_COUNT; i++)
        {
            sSC1PGameEnemyVariations[i] =
                (u8)sc1PGameGetFighterKindsNum(flags, syUtilsRandIntRange(variations));
            flags |= (u16)(1 << sSC1PGameEnemyVariations[i]);
            sSC1PGameEnemyVariations[i] += (u8)nFTKindNStart;
            variations--;
        }
        for (cycle = 0, i = SC1PGAME_STAGE_MAX_VARIATIONS_COUNT;
             i < (s32)stagesetup->opponent_count; i++)
        {
            sSC1PGameEnemyVariations[i] = sSC1PGameEnemyVariations[cycle];
            cycle = (cycle == (SC1PGAME_STAGE_MAX_VARIATIONS_COUNT - 1)) ?
                0 : cycle + 1;
        }
        sSC1PGameCurrentEnemyVariation = 0;
    }
    if (is_kirby != FALSE)
    {
        sSC1PGameCurrentEnemyVariation = 0;
    }

    /* Initial enemies. Singles seat opponent_count from the table kinds
     * (:1086-1110); Yoshi seats 3 (:1137-1157), Zako 3 (:1186-1200), Kirby 2
     * (:1206-1226). Ports rotate from the last ally (:1088) or the player. */
    cursor = ((s32)stagesetup->ally_count > 0) ?
        (s32)gSCManagerSceneData.ally_players[stagesetup->ally_count - 1] :
        (s32)gSCManagerSceneData.player;
    for (i = 0; i < initial; i++)
    {
        s32 slot;
        s32 fkind;

        cursor = sc1PGameGetNextFreePlayerPort(cursor);
        slot = cursor;

        /* :948-969 per-enemy fields; costume/shade/fkind specialize below. */
        cfg.fighters[slot].pkind = (u8)nFTPlayerKindCom;
        cfg.fighters[slot].level = (u8)level;
        cfg.fighters[slot].handicap = comsetup->enemy_handicap[difficulty];
        cfg.fighters[slot].team = (u8)nSCBattleTeamIDCom;
        cfg.fighters[slot].shade = 0;
        cfg.fighters[slot].stock_count = 0;
        cfg.fighters[slot].is_spgame_enemy = TRUE;
        sSC1PGamePlayerSetups[slot].cp_trait = stagesetup->opponent_behavior;

        if (is_yoshi != FALSE)
        {
            /* :1141-1156. */
            fkind = (s32)stagesetup->fkind[0];
            cfg.fighters[slot].fkind = (u8)fkind;
            cfg.fighters[slot].costume =
                sSC1PGameEnemyVariations[sSC1PGameCurrentEnemyVariation];
            if ((gSCManagerSceneData.fkind == (u8)nFTKindYoshi) &&
                (gSCManagerSceneData.costume == cfg.fighters[slot].costume))
            {
                cfg.fighters[slot].shade = 1;
            }
            sSC1PGamePlayerSetups[slot].team_order = sSC1PGameCurrentEnemyVariation;
            sSC1PGamePlayerSetups[slot].camera_frame_mul = 0.3F;
            sSC1PGamePlayerSetups[slot].is_magnify_ignore = TRUE;
            sSC1PGameCurrentEnemyVariation++;
        }
        else if (is_zako != FALSE)
        {
            /* :1190-1199. */
            cfg.fighters[slot].fkind =
                sSC1PGameEnemyVariations[sSC1PGameCurrentEnemyVariation];
            cfg.fighters[slot].costume = 0;
            sSC1PGamePlayerSetups[slot].team_order = sSC1PGameCurrentEnemyVariation;
            sSC1PGamePlayerSetups[slot].camera_frame_mul = 0.3F;
            sSC1PGamePlayerSetups[slot].is_magnify_ignore = TRUE;
            sSC1PGameCurrentEnemyVariation++;
        }
        else if (is_kirby != FALSE)
        {
            /* :1210-1225. Costume reads the live slot (:1212) the way the
             * source reads the live BattleState. */
            fkind = (s32)stagesetup->fkind[0];
            cfg.fighters[slot].fkind = (u8)fkind;
            if ((gSCManagerSceneData.fkind == (u8)nFTKindKirby) &&
                (gSCManagerSceneData.costume ==
                 gSCManager1PGameBattleState.players[slot].costume))
            {
                cfg.fighters[slot].costume =
                    (u8)ftParamGetCostumeCommonID(nFTKindKirby, 1);
            }
            else
            {
                cfg.fighters[slot].costume = 0;
            }
            sSC1PGameEnemyKirbyCostume = (s32)cfg.fighters[slot].costume;
            sSC1PGamePlayerSetups[slot].team_order = sSC1PGameCurrentEnemyVariation;
            sSC1PGamePlayerSetups[slot].copy_kind =
                dSC1PGameKirbyTeamCopyKinds[sSC1PGameCurrentEnemyVariation];
            sSC1PGamePlayerSetups[slot].camera_frame_mul = 0.3F;
            sSC1PGamePlayerSetups[slot].is_magnify_ignore = TRUE;
            sSC1PGameCurrentEnemyVariation++;
        }
        else
        {
            /* :1090-1092 with the free-costume scan (:839-877). */
            fkind = (s32)stagesetup->fkind[i];
            cfg.fighters[slot].fkind = (u8)fkind;
            cfg.fighters[slot].costume =
                (u8)ndsSC1PGameBridgeFreeCostume(&cfg, slot, fkind);
        }
    }

    ndsMatchConfigApply(&cfg);

    gSCManager1PGameBattleState = gSCManagerTransferBattleState;
    gSCManagerBattleState = &gSCManager1PGameBattleState;

    gNdsSC1PGameBridgeAppliedCount++;
}

#endif /* NDS_P2_1P_GAME */
