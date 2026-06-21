#include <ft/fighter.h>
#include <nds/nds_scene_harness.h>

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

    case NDS_DEV_SCENE_HARNESS_BATTLE_FD:
        gNdsSceneHarnessReservedMask = 1u;
        ndsSceneHarnessSetDefaultScene(nSCKindTitle, nSCKindOpeningNewcomers);
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_RESERVED_PASS;
        return;

    default:
        gNdsSceneHarnessReservedMask = 0x80000000u;
        return;
    }
#endif
}
