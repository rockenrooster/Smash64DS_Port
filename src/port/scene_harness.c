#include <ft/fighter.h>
#include <nds/nds_scene_harness.h>
#include "nds_scene_harness_config.h"

#ifndef NDS_DEV_SCENE_HARNESS
#define NDS_DEV_SCENE_HARNESS NDS_DEV_SCENE_HARNESS_NORMAL
#endif

volatile u32 gNdsSceneHarnessMode;
volatile u32 gNdsSceneHarnessResult;
volatile u32 gNdsSceneHarnessSceneCurr;
volatile u32 gNdsSceneHarnessScenePrev;
volatile u32 gNdsSceneHarnessReservedMask;

static void ndsSceneHarnessSetDefaultScene(SCKind scene_curr, SCKind scene_prev)
{
    dSCManagerDefaultSceneData.scene_curr = (u8)scene_curr;
    dSCManagerDefaultSceneData.scene_prev = (u8)scene_prev;

    gNdsSceneHarnessSceneCurr = (u32)scene_curr;
    gNdsSceneHarnessScenePrev = (u32)scene_prev;
}

static void ndsSceneHarnessSeedVSDefaults(void)
{
    s32 i;

    dSCManagerDefaultSceneData.maps_vsmode_gkind = nGRKindPupupu;
    dSCManagerDefaultSceneData.maps_training_gkind = nGRKindPupupu;
    gSCManagerSceneData.maps_vsmode_gkind = nGRKindPupupu;
    gSCManagerSceneData.maps_training_gkind = nGRKindPupupu;

    gSCManagerTransferBattleState.game_rules = SCBATTLE_GAMERULE_TIME;
    gSCManagerTransferBattleState.time_limit = 3;
    gSCManagerTransferBattleState.stocks = 2;
    gSCManagerTransferBattleState.handicap = 0;
    gSCManagerTransferBattleState.is_team_battle = FALSE;
    gSCManagerTransferBattleState.is_team_attack = FALSE;
    gSCManagerTransferBattleState.is_stage_select = TRUE;
    gSCManagerTransferBattleState.is_reset_players = FALSE;
    gSCManagerTransferBattleState.pl_count = 0;
    gSCManagerTransferBattleState.cp_count = 0;

    gSCManagerBackupData.fighter_mask = LBBACKUP_CHARACTER_MASK_ALL;
    gSCManagerBackupData.unlock_mask = 0;

    for (i = 0; i < GMCOMMON_PLAYERS_MAX; i++)
    {
        gSCManagerTransferBattleState.players[i].player = (u8)i;
        gSCManagerTransferBattleState.players[i].pkind = nFTPlayerKindNot;
        gSCManagerTransferBattleState.players[i].fkind = nFTKindNull;
        gSCManagerTransferBattleState.players[i].level = 1;
        gSCManagerTransferBattleState.players[i].handicap = 0;
        gSCManagerTransferBattleState.players[i].team = 0;
        gSCManagerTransferBattleState.players[i].costume = 0;
        gSCManagerTransferBattleState.players[i].shade = 0;
        gSCManagerTransferBattleState.players[i].color = 0;
    }
}

static void ndsSceneHarnessSeedBattleFDDefaults(void)
{
    s32 i;

    dSCManagerDefaultSceneData.gkind = nGRKindLast;
    gSCManagerSceneData.gkind = nGRKindLast;

    gSCManagerTransferBattleState = dSCManagerDefaultBattleState;
    gSCManagerTransferBattleState.game_rules = SCBATTLE_GAMERULE_STOCK;
    gSCManagerTransferBattleState.time_limit = 3;
    gSCManagerTransferBattleState.stocks = 3;
    gSCManagerTransferBattleState.handicap = 0;
    gSCManagerTransferBattleState.is_team_battle = FALSE;
    gSCManagerTransferBattleState.is_team_attack = FALSE;
    gSCManagerTransferBattleState.is_stage_select = FALSE;
    gSCManagerTransferBattleState.is_reset_players = FALSE;
    gSCManagerTransferBattleState.pl_count = 1;
    gSCManagerTransferBattleState.cp_count = 0;

    for (i = 0; i < GMCOMMON_PLAYERS_MAX; i++)
    {
        gSCManagerTransferBattleState.players[i].player = (u8)i;
        gSCManagerTransferBattleState.players[i].pkind = nFTPlayerKindNot;
        gSCManagerTransferBattleState.players[i].fkind = nFTKindNull;
        gSCManagerTransferBattleState.players[i].level = 1;
        gSCManagerTransferBattleState.players[i].handicap = 0;
        gSCManagerTransferBattleState.players[i].team = 0;
        gSCManagerTransferBattleState.players[i].costume = 0;
        gSCManagerTransferBattleState.players[i].shade = 0;
        gSCManagerTransferBattleState.players[i].color = 0;
        gSCManagerTransferBattleState.players[i].stock_count = 0;
    }

    gSCManagerTransferBattleState.players[0].pkind = nFTPlayerKindMan;
    gSCManagerTransferBattleState.players[0].fkind = nFTKindMario;
    gSCManagerTransferBattleState.players[0].handicap = 9;
    gSCManagerTransferBattleState.players[0].stock_count = 3;

    gSCManagerBackupData.error_flags = 0;
    gSCManagerBackupData.boot = 0;
    gSCManagerBackupData.fighter_mask = LBBACKUP_CHARACTER_MASK_ALL;
    gSCManagerBackupData.ground_mask = 0xFFFFu;

    dSCManagerDefaultBattleState = gSCManagerTransferBattleState;
}

static void ndsSceneHarnessSeedBattlePupupuStageDefaults(void)
{
    s32 i;

    dSCManagerDefaultSceneData.gkind = nGRKindPupupu;
    gSCManagerSceneData.gkind = nGRKindPupupu;

    gSCManagerTransferBattleState = dSCManagerDefaultBattleState;
    gSCManagerTransferBattleState.game_rules = SCBATTLE_GAMERULE_TIME;
    gSCManagerTransferBattleState.time_limit = 3;
    gSCManagerTransferBattleState.stocks = 2;
    gSCManagerTransferBattleState.handicap = 0;
    gSCManagerTransferBattleState.is_team_battle = FALSE;
    gSCManagerTransferBattleState.is_team_attack = FALSE;
    gSCManagerTransferBattleState.is_stage_select = TRUE;
    gSCManagerTransferBattleState.is_reset_players = FALSE;
    gSCManagerTransferBattleState.pl_count = 2;
    gSCManagerTransferBattleState.cp_count = 0;

    for (i = 0; i < GMCOMMON_PLAYERS_MAX; i++)
    {
        gSCManagerTransferBattleState.players[i].player = (u8)i;
        gSCManagerTransferBattleState.players[i].pkind = nFTPlayerKindNot;
        gSCManagerTransferBattleState.players[i].fkind = nFTKindNull;
        gSCManagerTransferBattleState.players[i].level = 1;
        gSCManagerTransferBattleState.players[i].handicap = 0;
        gSCManagerTransferBattleState.players[i].team = 0;
        gSCManagerTransferBattleState.players[i].costume = 0;
        gSCManagerTransferBattleState.players[i].shade = 0;
        gSCManagerTransferBattleState.players[i].color = 0;
        gSCManagerTransferBattleState.players[i].stock_count = 0;
    }

    gSCManagerTransferBattleState.players[0].pkind = nFTPlayerKindMan;
    gSCManagerTransferBattleState.players[0].fkind = nFTKindMario;
    gSCManagerTransferBattleState.players[0].handicap = 9;
    gSCManagerTransferBattleState.players[0].team = 0;
    gSCManagerTransferBattleState.players[0].stock_count = 2;

    gSCManagerTransferBattleState.players[1].pkind = nFTPlayerKindMan;
    gSCManagerTransferBattleState.players[1].fkind = nFTKindFox;
    gSCManagerTransferBattleState.players[1].handicap = 9;
    gSCManagerTransferBattleState.players[1].team = 1;
    gSCManagerTransferBattleState.players[1].stock_count = 2;

    gSCManagerBackupData.error_flags = 0;
    gSCManagerBackupData.boot = 0;
    gSCManagerBackupData.fighter_mask = LBBACKUP_CHARACTER_MASK_ALL;
    gSCManagerBackupData.ground_mask = 0xFFFFu;

    dSCManagerDefaultBattleState = gSCManagerTransferBattleState;
}

static void ndsSceneHarnessSeedBattlePlayableDefaults(void)
{
    ndsSceneHarnessSeedBattlePupupuStageDefaults();

    gSCManagerTransferBattleState.game_rules = SCBATTLE_GAMERULE_STOCK;
    gSCManagerTransferBattleState.time_limit = SCBATTLE_TIMELIMIT_INFINITE;
    gSCManagerTransferBattleState.stocks = 2;
    gSCManagerTransferBattleState.players[0].stock_count = 2;
    gSCManagerTransferBattleState.players[1].stock_count = 2;

    dSCManagerDefaultBattleState = gSCManagerTransferBattleState;
}

void ndsDevSceneHarnessApply(void)
{
    gNdsSceneHarnessMode = (u32)NDS_DEV_SCENE_HARNESS;
    gNdsSceneHarnessResult = NDS_SCENE_HARNESS_NONE;
    gNdsSceneHarnessReservedMask = 0;
    gNdsSceneHarnessSceneCurr = dSCManagerDefaultSceneData.scene_curr;
    gNdsSceneHarnessScenePrev = dSCManagerDefaultSceneData.scene_prev;

#if defined(SSB64_TARGET_NDS)
    switch (NDS_DEV_SCENE_HARNESS)
    {
    case NDS_DEV_SCENE_HARNESS_NORMAL:
        return;

    case NDS_DEV_SCENE_HARNESS_TITLE:
        ndsSceneHarnessSetDefaultScene(nSCKindTitle, nSCKindOpeningNewcomers);
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_VS_SETUP:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_VS_START_TRANSITION:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_PLAYERS_SETUP:
        ndsSceneHarnessSetDefaultScene(nSCKindPlayersVS, nSCKindVSMode);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_MAPS_SETUP:
        ndsSceneHarnessSetDefaultScene(nSCKindMaps, nSCKindPlayersVS);
        ndsSceneHarnessSeedVSDefaults();
        gSCManagerTransferBattleState.is_stage_select = TRUE;
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_VSBATTLE:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_PUPUPU_UPDATE:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_MODEL:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STRUCT:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_INIT:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_TICK:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_GROUND:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DISPLAY_PROBE:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_SCAN:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_EXECUTE:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_MULTI:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_ALL:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_INPUT:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_JUMP_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LANDING_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PROCESS_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_SCHEDULER_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_CONTROLLER_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PREVIEW_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCDRAWALL_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_DAMAGE_LOOP:
    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        ndsSceneHarnessSeedVSDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_FD:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattleFDDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_PUPUPU_STAGE:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_PUPUPU_UPDATE:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_MODEL:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STRUCT:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_INIT:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WAIT:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WAIT_TICK:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WAIT_GROUND:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DISPLAY_PROBE:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_SCAN:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_EXECUTE:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_DRAW:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_DRAW_MULTI:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_DRAW_ALL:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WALK_INPUT:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WALK_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DASH_RUN:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_JUMP_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_LANDING_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_PROCESS_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_SCHEDULER_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_CONTROLLER_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_PREVIEW_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_GCRUNALL_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_GCDRAWALL_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_GCDRAWALL_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_COLLISION_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_EDGE_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNWAIT_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_TURN_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_INPUT_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_INISHIE_SCALE_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_DAMAGE_LOOP:
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePlayableDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_LIVE_PREVIEW:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    default:
        gNdsSceneHarnessReservedMask = 0x80000000u;
        return;
    }
#endif
}
