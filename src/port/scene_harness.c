#include <ft/fighter.h>
#include <nds/nds_match_config.h>
#include <nds/nds_scene_harness.h>
#include "nds_build_config.h"
#include "nds_scene_harness_config.h"

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

static void ndsSceneHarnessSyncSingleStockIconFlags(void)
{
    s32 i;
    ub8 is_single_stockicon =
        (gSCManagerTransferBattleState.game_rules & SCBATTLE_GAMERULE_TIME) ? TRUE : FALSE;

    for (i = 0; i < GMCOMMON_PLAYERS_MAX; i++)
    {
        /* BattleShip mnplayersvs.c:4417 derives this from the time-rule bit. */
        gSCManagerTransferBattleState.players[i].is_single_stockicon = is_single_stockicon;
    }
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

/* Dev-open cartridge, harness builds only. The Boundary lap (p2_shell_loop)
 * plays fighters the source locks and the scripted stage sweep visits Inishie,
 * so those verifiers need every fighter and stage available. Published
 * smash64ds.nds builds NDS_DEV_SCENE_HARNESS=normal (ID 0 -- Makefile:119,
 * :3635-3636, :5717; published list :65-66), so this gate is false there and
 * the boot save stands as lbBackupIsSramValid left it (lbbackup.c:44-63 via
 * scmanager.c:852-853). Every harness target overrides the harness to
 * battle_playable_realtime (Makefile:2893, :3095), so the override holds
 * there. Unlocks are then earned only through mnMessageApplyUnlock
 * (mnmessage.c:284-301). */
#if (NDS_DEV_SCENE_HARNESS != NDS_DEV_SCENE_HARNESS_NORMAL)
    gSCManagerBackupData.fighter_mask = LBBACKUP_CHARACTER_MASK_ALL;
    gSCManagerBackupData.unlock_mask = 0;
#endif

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

    ndsSceneHarnessSyncSingleStockIconFlags();
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

    ndsSceneHarnessSyncSingleStockIconFlags();

    gSCManagerTransferBattleState.players[0].pkind = nFTPlayerKindMan;
    gSCManagerTransferBattleState.players[0].fkind = nFTKindMario;
    gSCManagerTransferBattleState.players[0].handicap = 9;
    gSCManagerTransferBattleState.players[0].stock_count = 3;

    gSCManagerBackupData.error_flags = 0;
    gSCManagerBackupData.boot = 0;
    /* Same dev-open gate as ndsSceneHarnessSeedVSDefaults: direct-battle
     * harnesses create fighters without passing the CSS, so they keep the
     * open cart; published keeps the save. */
#if (NDS_DEV_SCENE_HARNESS != NDS_DEV_SCENE_HARNESS_NORMAL)
    gSCManagerBackupData.fighter_mask = LBBACKUP_CHARACTER_MASK_ALL;
    gSCManagerBackupData.ground_mask = 0xFFFFu;
#endif

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

    ndsSceneHarnessSyncSingleStockIconFlags();

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
    /* Same dev-open gate as ndsSceneHarnessSeedVSDefaults. */
#if (NDS_DEV_SCENE_HARNESS != NDS_DEV_SCENE_HARNESS_NORMAL)
    gSCManagerBackupData.fighter_mask = LBBACKUP_CHARACTER_MASK_ALL;
    gSCManagerBackupData.ground_mask = 0xFFFFu;
#endif

    dSCManagerDefaultBattleState = gSCManagerTransferBattleState;
}

/* Mode 163's configuration is now a PRESET plus an APPLY (P2-1a). The whole
 * match -- who fights, at what CPU level, on which stage, under which rules,
 * with which items -- lives in `gNdsMatchConfig`, and every field that used to
 * be written here inline now travels through it. `nds_match_config.c` carries
 * the reasoning that used to sit in this function, including the both-CPU
 * stress arm and the soak's separate match-length flag.
 *
 * What stays here is not match configuration: the error/boot flags are save
 * data the harness declares so a booted match sees a clean boot. The unlock
 * masks below are dev-open only (same gate as ndsSceneHarnessSeedVSDefaults):
 * mode 163 reached through the shell is the Boundary p2_battle_realtime arm,
 * and it needs the open cart there; published keeps the save. */
static void ndsSceneHarnessSeedBattlePlayableDefaults(void)
{
    ndsMatchConfigLoadMarioFoxDreamLand(&gNdsMatchConfig);
    ndsMatchConfigApply(&gNdsMatchConfig);

    gSCManagerBackupData.error_flags = 0;
    gSCManagerBackupData.boot = 0;
#if (NDS_DEV_SCENE_HARNESS != NDS_DEV_SCENE_HARNESS_NORMAL)
    gSCManagerBackupData.fighter_mask = LBBACKUP_CHARACTER_MASK_ALL;
    gSCManagerBackupData.ground_mask = 0xFFFFu;
#endif
}

/* Seed the transfer state as if the canonical one-minute Time match had just
 * ended with Mario ahead, so VS Results can be entered directly from boot.
 *
 * Results is a pure consumer of this struct: `mnvsresults.c` reads pkind/fkind
 * for presence and models, costume/shade/team for colour, score and falls for
 * the KO and fall rows, the damage and KO totals for the backup records, and
 * `time_passed`/`gkind` for the record update -- and nothing else. There is no
 * live battle GObj to fake, which is what makes booting the scene legitimate
 * instead of a mock.
 *
 * The numbers are one consistent match, not placeholders, because the source
 * derives visible content from them: Mario takes 3 KOs to Fox's 1 and falls
 * once to Fox's three, so `mnVSResultsGetBestMan` ranks Mario first and the
 * winner text reads MARIO WINS. `total_kos_players` has to agree with `score`
 * and `falls` or the rankings and the per-fighter records disagree with the
 * rows drawn beside them. `time_passed` is the full 3,600-tick minute: Results
 * divides it by UPDATE_INTERVAL into `time_used`.
 *
 * A Fox-win arm is deliberately not a second harness mode -- swap the two
 * players' figures here when the fidelity matrix needs it. The research doc
 * requires both arms proven before acceptance, and one mode with edited
 * constants beats two modes that can drift apart. */
static void ndsSceneHarnessSeedResultsPlayableDefaults(void)
{
    struct SCPlayerData *mario;
    struct SCPlayerData *fox;

    ndsSceneHarnessSeedBattlePlayableDefaults();

    gSCManagerTransferBattleState.time_passed = 3600;

    mario = &gSCManagerTransferBattleState.players[0];
    fox = &gSCManagerTransferBattleState.players[1];

    mario->place = 0;
    mario->score = 3;
    mario->falls = 1;
    mario->total_kos_players[1] = 3;
    mario->total_damage_given = 412;
    mario->total_damage_all = 168;
    mario->total_damage_players[1] = 168;
    mario->total_selfdestructs = 0;

    fox->place = 1;
    fox->score = 1;
    fox->falls = 3;
    fox->total_kos_players[0] = 1;
    fox->total_damage_given = 168;
    fox->total_damage_all = 412;
    fox->total_damage_players[0] = 412;
    fox->total_selfdestructs = 0;

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
    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE:
#if NDS_P2_MENU_SHELL
        /* P2-1d. The VS shell boots the GAME, not the match: the flow starts
         * at the splash and the player reaches the battle through the menus.
         *
         * The seeding below is unchanged and deliberately still runs -- the
         * mode-163 preset is what fills the match descriptor, and the VS rules
         * screen edits that descriptor rather than replacing it, so the match
         * the menus enter is the canonical one. ONLY the boot scene moves, and
         * only under this flag: at NDS_P2_MENU_SHELL == 0, which is every
         * published and Boundary configuration, mode 163 still boots straight
         * into nSCKindVSBattle with nSCKindMaps behind it. */
        ndsSceneHarnessSetDefaultScene(nSCKindStartup, nSCKindStartup);
#else
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
#endif
        ndsSceneHarnessSeedBattlePlayableDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_LIVE_PREVIEW:
        ndsSceneHarnessSetDefaultScene(nSCKindVSBattle, nSCKindMaps);
        ndsSceneHarnessSeedBattlePupupuStageDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    /* `scene_prev` is VSBattle, not Maps: Results is reached from a match, and
     * the source branches on where it came from. Boot lands in the scene the
     * player would have reached, on the same source timeline from tic 0. */
    case NDS_DEV_SCENE_HARNESS_RESULTS_PLAYABLE:
        ndsSceneHarnessSetDefaultScene(nSCKindVSResults, nSCKindVSBattle);
        ndsSceneHarnessSeedResultsPlayableDefaults();
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    default:
        gNdsSceneHarnessReservedMask = 0x80000000u;
        return;
    }
#endif
}
