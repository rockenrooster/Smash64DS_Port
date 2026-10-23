/* The fighter manager seam: the live registry from a player number to its
 * source fighter GObj (every GMCOMMON_PLAYERS_MAX slot), which the native
 * renderer and collision helpers read; the display fighters the 1P and
 * bonus menus register (ndsFighterManagerRegisterDisplayFighter); the
 * physics helpers the shims share; and names for the imported manager's
 * FTStruct and parts pools (no second pool: in the ship layout
 * sNdsFighterPartsPool is not even linked). A slice of reloc_backend.c. */

#include "nds_scene_harness_config.h"

static void *sNdsFighterFTManagerCommonFile;
static void *sNdsFighterMarioMainMotionFile;
static void *sNdsFighterMarioMainFile;
static void *sNdsFighterMarioSpecial1File;
static void *sNdsFighterMarioModelFile;
static void *sNdsFighterMarioSpecial3File;
static void *sNdsFighterMarioShieldPoseFile;
static void *sNdsFighterMarioSpecial2File;
static void *sNdsFighterFoxSpecial3File;
static void *sNdsFighterFoxMainMotionFile;
static void *sNdsFighterFoxMainFile;
static void *sNdsFighterFoxSpecial1File;
static void *sNdsFighterFoxModelFile;
static void *sNdsFighterFoxShieldPoseFile;
static void *sNdsFighterFoxSpecial4File;
static void *sNdsFighterFoxSpecial2File;
/* The imported manager owns these pools (ftmanager.c:25/:31, allocated at
 * :144/:154).  Keep the historical DS proof names as source-storage aliases so
 * the old bounded probes still compile without reserving a second FTStruct or
 * FTParts image.  The parts allocation is flat in BattleShip but is laid out as
 * player-major [player][joint], exactly what the old 2-D declaration encoded. */
extern FTStruct *sFTManagerStructsAllocBuf;
extern FTParts *sFTManagerPartsAllocBuf;
#define sNdsFighterStructPool sFTManagerStructsAllocBuf
#define sNdsFighterPartsPool \
    ((FTParts (*)[nFTPartsJointNumMax])sFTManagerPartsAllocBuf)
/* BattleShip's VS battle allocates/iterates all GMCOMMON_PLAYERS_MAX slots.
 * Keep the DS live registry at the same capacity: this registry is the bridge
 * used by DS-native render/collision helpers to recover the source fighter
 * GObj for a player number.  A two-entry registry made source-created players
 * 2/3 disappear at that bridge even though scVSBattleStartBattle had created
 * them correctly. */
static GObj *sNdsFighterManagerLiveGObjs[GMCOMMON_PLAYERS_MAX];
extern u16 gMPCollisionUpdateTic;

static void ndsFighterManagerClearLiveGObjs(void)
{
    bzero(sNdsFighterManagerLiveGObjs, sizeof(sNdsFighterManagerLiveGObjs));
}

static void ndsFighterMarioFoxResetFileSlots(void)
{
    ndsFighterManagerClearLiveGObjs();
    sNdsFighterFTManagerCommonFile = NULL;
    sNdsFighterMarioMainMotionFile = NULL;
    sNdsFighterMarioMainFile = NULL;
    sNdsFighterMarioSpecial1File = NULL;
    sNdsFighterMarioModelFile = NULL;
    sNdsFighterMarioSpecial3File = NULL;
    sNdsFighterMarioShieldPoseFile = NULL;
    sNdsFighterMarioSpecial2File = NULL;
    sNdsFighterFoxSpecial3File = NULL;
    sNdsFighterFoxMainMotionFile = NULL;
    sNdsFighterFoxMainFile = NULL;
    sNdsFighterFoxSpecial1File = NULL;
    sNdsFighterFoxModelFile = NULL;
    sNdsFighterFoxShieldPoseFile = NULL;
    sNdsFighterFoxSpecial4File = NULL;
    sNdsFighterFoxSpecial2File = NULL;

    gNdsFighterMarioMainMotionPtrReady = 0u;
    gNdsFighterMarioMainPtrReady = 0u;
    gNdsFighterMarioModelPtrReady = 0u;
    gNdsFighterMarioAttrPtrReady = 0u;
    gNdsFighterMarioCommonPartsReady = 0u;
    gNdsFighterFoxMainMotionPtrReady = 0u;
    gNdsFighterFoxMainPtrReady = 0u;
    gNdsFighterFoxModelPtrReady = 0u;
    gNdsFighterFoxAttrPtrReady = 0u;
    gNdsFighterFoxCommonPartsReady = 0u;

}
extern void gcSetupCustomDObjs(GObj *gobj, DObjDesc *dobjdesc, DObj **dobjs,
                               u8 tk1, u8 tk2, u8 tk3);
extern void func_80017EC0(GObj *gobj);





#define NDS_MARIOFOX_LANDING_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_LANDING_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LANDING_LOOP) || \
     NDS_MARIOFOX_PROCESS_LOOP_HARNESS)

#define NDS_MARIOFOX_PROCESS_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_PROCESS_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PROCESS_LOOP) || \
     NDS_MARIOFOX_SCHEDULER_LOOP_HARNESS)

#define NDS_MARIOFOX_SCHEDULER_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_SCHEDULER_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_SCHEDULER_LOOP) || \
     NDS_MARIOFOX_CONTROLLER_LOOP_HARNESS)

#define NDS_MARIOFOX_CONTROLLER_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_CONTROLLER_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_CONTROLLER_LOOP) || \
     NDS_MARIOFOX_PREVIEW_LOOP_HARNESS)

#define NDS_MARIOFOX_PREVIEW_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_PREVIEW_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PREVIEW_LOOP) || \
     NDS_MARIOFOX_GCRUNALL_LOOP_HARNESS)

#define NDS_MARIOFOX_GCRUNALL_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_GCRUNALL_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP) || \
     NDS_MARIOFOX_GCDRAWALL_LOOP_HARNESS)

#define NDS_MARIOFOX_GCDRAWALL_LOOP_HARNESS \
    (NDS_MARIOFOX_STAGE_GCDRAWALL_LOOP_HARNESS || \
     NDS_MARIOFOX_LIVE_PREVIEW_HARNESS)

#define NDS_MARIOFOX_STAGE_GCDRAWALL_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
     NDS_MARIOFOX_STAGE_COLLISION_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_COLLISION_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_COLLISION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
     NDS_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
     NDS_MARIOFOX_STAGE_FLOOR_EDGE_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_FLOOR_EDGE_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPASS_INPUT_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPPASS_INPUT_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_INISHIE_SCALE_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_INISHIE_SCALE_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
     NDS_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_TURN_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_TURN_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) || \
     NDS_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPDOWNWAIT_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_TURN_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
     NDS_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
     NDS_MARIOFOX_STAGE_MPLIVEHIT_DAMAGE_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP))

#define NDS_MARIOFOX_STAGE_MPLIVEHIT_DAMAGE_LOOP_HARNESS \
    NDS_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP_HARNESS

#define NDS_MARIOFOX_STAGE_MPPASSIVE_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPDOWNWAIT_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP_HARNESS || \
     NDS_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
     NDS_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP_HARNESS)

#define NDS_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
     NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS)

#define NDS_MARIOFOX_LIVE_PREVIEW_HARNESS \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_LIVE_PREVIEW) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW))



static sb32 ndsFighterMarioFoxDashRunProofEnabled(void)
{
    return FALSE;
}

static sb32 ndsFighterMarioFoxJumpAttackAirProofEnabled(void)
{
    return FALSE;
}

static sb32 ndsFighterMarioFoxStageGCDrawAllLoopProofEnabled(void)
{
#if NDS_MARIOFOX_STAGE_GCDRAWALL_LOOP_HARNESS || (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE)
    return TRUE;
#else
    return FALSE;
#endif
}

static sb32 ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled(void)
{
    return FALSE;
}

static sb32 ndsFighterMarioFoxStageMPPassiveLoopProofEnabled(void)
{
    return FALSE;
}

/* DO NOT ADD AN ACCESSOR FOR sNdsFighterPartsPool. R2-07 L7's first draft did,
 * to give its oracle a flat array to walk, and the linker map answered a
 * question the board had got wrong: in the shipping-shaped configuration this
 * pool IS NOT LINKED. `.bss.sNdsFighterPartsPool` contributes 0 bytes to
 * build-tickhud and 33,152 the moment anything outside references it; the
 * imported-manager path has no runtime reference to this compatibility pool.
 *
 * Two consequences, both load-bearing. The FTParts collision reads through
 * ftGetParts do NOT come from here in that build, so "the port already owns the
 * structure collision reads" is false for the ROM that ships -- anything hooking
 * L7 on this pool would hook an array nothing populates. And a reference to it
 * costs 33,152 bytes of image, which is eight taskman arena steps: enough on its
 * own to drop the battle under ifCommonSetMaxNumGObj's 25 KiB latch. Walk the
 * live GObj/DObj tree instead; battleship_gmcollision.c does. */


static sb32 ndsFighterStructIsPoolPointer(const void *ptr)
{
    (void)ptr;
    return FALSE;
}

static GObj *ndsFighterManagerLiveGObj(u32 slot)
{
    if (slot >= ARRAY_COUNT(sNdsFighterManagerLiveGObjs))
    {
        return NULL;
    }
    return sNdsFighterManagerLiveGObjs[slot];
}

static FTStruct *ndsFighterManagerLiveStruct(u32 slot)
{
    GObj *fighter_gobj = ndsFighterManagerLiveGObj(slot);

    return (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
}

/* renderer_adapter_fighter.c, included after this file. */
void ndsFighterRendererInvalidateSlotDrawCaches(u32 slot);

void ndsFighterManagerRegisterDisplayFighter(GObj *fighter_gobj, u32 slot)
{
    FTStruct *fp;

    if (slot >= ARRAY_COUNT(sNdsFighterManagerLiveGObjs))
    {
        return;
    }
    /* Registered or released: either way the slot's drawn DObjs change. */
    ndsFighterRendererInvalidateSlotDrawCaches(slot);
    /* Scene-local fighters live in taskman arenas. A menu/results owner must
     * clear its registration before that arena is rewound or the next scene
     * can mistake a stale FTStruct address for a live fighter. */
    if (fighter_gobj == NULL)
    {
        sNdsFighterManagerLiveGObjs[slot] = NULL;
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if (fp == NULL)
    {
        return;
    }
    sNdsFighterManagerLiveGObjs[slot] = fighter_gobj;
    fp->nds_magic = NDS_FTSTRUCT_MAGIC;
    fp->nds_slot = slot;
}

static u32 ndsFighterManagerLiveMask(void)
{
    u32 mask = 0u;
    u32 i;

    for (i = 0u; i < ARRAY_COUNT(sNdsFighterManagerLiveGObjs); i++)
    {
        if (ndsFighterManagerLiveStruct(i) != NULL)
        {
            mask |= 1u << i;
        }
    }
    return mask;
}


/* ITCM (2026-09-27 census: ~1.1K ticks/frame non-memory stall at 88 B). */
static sb32 __attribute__((section(".itcm"))) ndsFighterStructIsTrackedPointer(const void *ptr)
{
    const FTStruct *fp = ptr;
    u32 i;

    if (ndsFighterStructIsPoolPointer(ptr) != FALSE)
    {
        return TRUE;
    }
    for (i = 0u; i < ARRAY_COUNT(sNdsFighterManagerLiveGObjs); i++)
    {
        if ((fp == ndsFighterManagerLiveStruct(i)) &&
            (fp != NULL) &&
            (fp->nds_magic == NDS_FTSTRUCT_MAGIC))
        {
            return TRUE;
        }
    }
    return FALSE;
}


static GObj *ndsFighterGetPlayerNumGObj(s32 player_num)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

    /* BattleShip ftparam.c:186-200. `player_num` is NOT the battle player
     * index: ftManagerGetNextStructAlloc seeds gFTManagerPlayersNum at 1 and
     * assigns a monotonically increasing instance number to every created
     * fighter.  The old DS helper accidentally treated it as slot 0..3, which
     * resolves the wrong attacker as soon as creation order differs from battle
     * slot order (and can never resolve player_num 4 in a four-fighter match).
     *
     * Keep the source's linked-list lookup exactly.  Damage/public-call paths
     * pass attacker_player_num here, and that field is the fighter instance
     * number recorded by the source hitlog rather than FTStruct::player. */
    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) && (fp->player_num == player_num))
        {
            return fighter_gobj;
        }
        fighter_gobj = fighter_gobj->link_next;
    }
    return NULL;
}





static void ndsFighterManagerRecordCreatedFighter(GObj *fighter_gobj,
                                                   s32 player)
{
    FTStruct *fp;
    u32 bit;
    u32 live_mask;
    u32 live_count = 0u;

    if ((fighter_gobj == NULL) || (player < 0) ||
        (player >= GMCOMMON_PLAYERS_MAX))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if (fp == NULL)
    {
        return;
    }
    bit = 1u << player;
    sNdsFighterManagerLiveGObjs[player] = fighter_gobj;
    fp->nds_magic = NDS_FTSTRUCT_MAGIC;
    fp->nds_slot = (u32)player;
    ndsFighterRendererInvalidateSlotDrawCaches((u32)player);
    /* The imported BattleShip FTStruct is authoritative.  Earlier port proof
     * code copied all 3,012 bytes into a second static pool here; no gameplay
     * reader consumed that copy, while it permanently reduced the DS heap that
     * can back taskman.  Keep only the live GObj registry and DS identity tags. */

    if (fp->joints[nFTPartsJointTopN] != NULL)
    {
        DObj *root_dobj = fp->joints[nFTPartsJointTopN];

        if (player == 0)
        {
            gNdsFighterInitP0GA = (u32)fp->ga;
            gNdsFighterInitP0FloorLineID = (u32)fp->coll_data.floor_line_id;
            gNdsFighterInitP0RootTranslateXBits =
                ndsFloatBits(root_dobj->translate.vec.f.x);
            gNdsFighterInitP0RootTranslateYBits =
                ndsFloatBits(root_dobj->translate.vec.f.y);
        }
        else if (player == 1)
        {
            gNdsFighterInitP1GA = (u32)fp->ga;
            gNdsFighterInitP1FloorLineID = (u32)fp->coll_data.floor_line_id;
            gNdsFighterInitP1RootTranslateXBits =
                ndsFloatBits(root_dobj->translate.vec.f.x);
            gNdsFighterInitP1RootTranslateYBits =
                ndsFloatBits(root_dobj->translate.vec.f.y);
        }
    }

    /* P2-2 uses Mario/Fox mirrors in any of the four source player slots.
     * Do not let the old P0=Mario/P1=Fox proof convention define runtime
     * capacity.  The existing P0/P1 diagnostic fields below remain intentionally
     * slot-specific, but these masks/counts describe every supported live
     * battle fighter.
     *
     * P2-3r11: the fkind test that used to guard this was that intent written
     * as the then-complete list of playable kinds, and it went stale the moment
     * Luigi and Donkey Kong landed.  A Mario/Fox/Luigi/Donkey battle recorded
     * `gNdsSCVSBattleOriginalFighterGObjCount = 2` and active mask 3 with all
     * four fighters alive, which read as "two fighters were never created" and
     * cost a cycle of chasing a creation failure that had not happened.  The
     * only correct predicate is the one the comment above always described: a
     * live fighter GObj in a real player slot.  `fp` is non-NULL and `player` is
     * range-checked above, and the sole caller is
     * `ftParamInitPlayerBattleStats` out of `scVSBattleStartBattle`'s per-player
     * loop -- character-select previews never reach here -- so there is nothing
     * left for a kind list to exclude. */
    gNdsFighterManagerFighterMask |= bit;
    gNdsSCVSBattleOriginalActivePlayerMask |= bit;
    /* P2-3r15: one byte per player slot, holding `fkind + 1` so an empty
     * slot (0) stays distinct from Mario (nFTKindMario == 0). Written as a
     * clear-then-set field rather than an OR, so two creation paths writing
     * the same slot cannot fuse two kinds into one byte. */
    gNdsSCVSBattleOriginalFighterKinds =
        (gNdsSCVSBattleOriginalFighterKinds &
         ~(0xffu << ((u32)player * 8u))) |
        ((((u32)fp->fkind + 1u) & 0xffu) << ((u32)player * 8u));
    if ((fp->fkind >= 0) && (fp->fkind < nFTKindEnumCount) &&
        (fp->data == dFTManagerDataFiles[fp->fkind]) &&
        (fp->attr != NULL) && (fp->figatree_heap != NULL))
    {
        gNdsFighterManagerDataMask |= bit;
    }
    if (fp->status_id == nFTCommonStatusWait)
    {
        gNdsFighterManagerWaitMask |= bit;
    }
    if (fp->status_id == nFTCommonStatusEntry)
    {
        gNdsFighterManagerEntryMask |= bit;
    }

    live_mask = gNdsFighterManagerFighterMask;
    while (live_mask != 0u)
    {
        live_count += live_mask & 1u;
        live_mask >>= 1;
    }
    gNdsFighterManagerFighterCount = live_count;
    gNdsSCVSBattleOriginalFighterGObjCount =
        gNdsFighterManagerFighterCount;
    gNdsSCVSBattleOriginalFighterCreateCount =
        gNdsFighterManagerFighterCount;
    gNdsSCVSBattleOriginalActivePlayerCount =
        gNdsFighterManagerFighterCount;
    if (player == 0)
    {
        gNdsSCVSBattleOriginalP0FKind = (u32)fp->fkind;
        gNdsSCVSBattleOriginalP0LR = (u32)fp->lr;
    }
    else if (player == 1)
    {
        gNdsSCVSBattleOriginalP1FKind = (u32)fp->fkind;
        gNdsSCVSBattleOriginalP1LR = (u32)fp->lr;
    }
    gNdsSCVSBattleCompatManagerMask |= 1u << 3;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
    ndsFighterManagerRefreshProof();
}
















static s32 ndsFloatToMilliSigned(f32 value)
{
    if (value >= 0.0F)
    {
        return (s32)((value * 1000.0F) + 0.5F);
    }
    return (s32)((value * 1000.0F) - 0.5F);
}

static void ndsFighterSyncPhysicsToLegacyVel(FTStruct *fp)
{
    if (fp == NULL)
    {
        return;
    }
    fp->vel_air = fp->physics.vel_air;
    fp->vel_ground = fp->physics.vel_ground;
}

static void ndsFTPhysicsSetGroundVelTransferAirOriginal(GObj *fighter_gobj,
                                                        FTStruct *fp)
{
    DObj *root = DObjGetStruct(fighter_gobj);

    if ((fp == NULL) || (root == NULL))
    {
        return;
    }

    fp->physics.vel_air.x =
        ((fp->lr * fp->coll_data.floor_angle.y *
          fp->physics.vel_ground.x) + fp->physics.vel_jostle_x);
    fp->physics.vel_air.y =
        (fp->lr * -fp->coll_data.floor_angle.x *
         fp->physics.vel_ground.x);
    fp->physics.vel_air.z = fp->physics.vel_jostle_z;

    if (((fp->physics.vel_jostle_z > 0.0F) &&
         (root->translate.vec.f.z < 0.0F) &&
         ((root->translate.vec.f.z + fp->physics.vel_air.z) > 0.0F)) ||
        ((fp->physics.vel_air.z < 0.0F) &&
         (root->translate.vec.f.z > 0.0F) &&
         ((root->translate.vec.f.z + fp->physics.vel_air.z) < 0.0F)))
    {
        fp->physics.vel_air.z = -root->translate.vec.f.z;
    }
    if ((fp->physics.vel_air.z > 0.0F) &&
        ((root->translate.vec.f.z + fp->physics.vel_air.z) > 60.0F))
    {
        fp->physics.vel_air.z = 60.0F - root->translate.vec.f.z;
    }
    else if ((fp->physics.vel_air.z < 0.0F) &&
             ((root->translate.vec.f.z + fp->physics.vel_air.z) < -60.0F))
    {
        fp->physics.vel_air.z = -60.0F - root->translate.vec.f.z;
    }
    fp->physics.vel_air.z += fp->physics.vel_ground.z;
}

static sb32 ndsMPCommonCheckFighterOnCliffEdgeBounded(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        return FALSE;
    }

    NDS_DIAG(gNdsFighterWaitGroundMapCheckCount++);
    if (ndsStageFloorFollowLoopUpdateFighter(fighter_gobj) != FALSE)
    {
        NDS_DIAG(gNdsFighterWaitGroundMapSafeFloorCount++);
        return TRUE;
    }
    if ((fp->ga == nMPKineticsGround) &&
        (fp->coll_data.floor_line_id == 0) &&
        ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u))
    {
        NDS_DIAG(gNdsFighterWaitGroundMapSafeFloorCount++);
        fp->coll_data.is_coll_end = FALSE;
        return TRUE;
    }

    fp->coll_data.is_coll_end = TRUE;
    return FALSE;
}



