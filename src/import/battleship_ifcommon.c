#if NDS_IMPORT_BATTLESHIP_IFCOMMON
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <nds/nds_scene_harness.h>
#include <nds/nds_startup.h>
#include <sys/objhelper.h>
#include <sys/objman.h>

#ifndef U16_MAX
#define U16_MAX 0xffffu
#endif

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifndef SObjGetStruct
#define SObjGetStruct(gobj) ((SObj *)((gobj)->obj))
#endif

#ifndef CObjGetStruct
#define CObjGetStruct(gobj) ((CObj *)((gobj)->obj))
#endif

extern Mtx44f gGMCameraMatrix;
extern f32 gGMCameraPauseCameraEyeX;
extern f32 gGMCameraPauseCameraEyeY;
extern void gmCameraRunFuncCamera(GObj *camera_gobj);
extern void gmCameraSetStatusPrev(void);
extern void gmCameraSetStatusDefault(void);
extern void gmCameraSetStatusPlayerZoom(GObj *fighter_gobj, f32 eye_x,
                                        f32 eye_y, f32 dist, f32 pan_scale,
                                        f32 fov);
extern void gmCameraSetStatusMapZoom(Vec3f *origin, Vec3f *target);
extern sb32 gmCameraCheckTargetInBounds(f32 pos_x, f32 pos_y);
extern sb32 gmCameraCheckPausePlayerOutBounds(Vec3f *pos);
extern void func_ovl2_800EB924(CObj *cobj, Mtx44f matrix, Vec3f *pos,
                               f32 *dist_x, f32 *dist_y);
extern void gcDrawDObjDLHead0(GObj *gobj);
extern void gcDrawDObjTreeForGObj(GObj *gobj);
extern void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints,
                              f32 anim_frame);
extern void gcPlayAnimAll(GObj *gobj);
extern void gcSetupCustomDObjs(GObj *gobj, DObjDesc *dobjdesc, DObj **dobjs,
                               u8 matrix_kind, u8 mobj_kind, u8 aobj_kind);
extern void grWallpaperResumePerspUpdate(void);
extern void grWallpaperPausePerspUpdate(void);
extern void grWallpaperRunProcessAll(void);
extern void grWallpaperResumeProcessAll(void);
extern void sc1PGameSetCameraZoom(void);
extern s32 func_800264A4_270A4(void);
extern s32 func_80026594_27194(void);
extern void func_800266A0_272A0(void);
extern u32 sySchedulerGetTicCount(void);
extern void sySchedulerSetTicCount(u32 tics);
extern void lbCommonDrawSObjAttr(GObj *gobj);
extern void lbCommonEjectGObjLinkedList(GObj *gobj);
extern void lbCommonPrepSObjAttr(Gfx **dls, SObj *sobj);
extern void lbCommonPrepSObjDraw(Gfx **dls, SObj *sobj);
extern void lbCommonSetExternSpriteParams(Sprite *sprite);
extern void lbCommonClearExternSpriteParams(void);
extern SObj *lbCommonMakeSObjForGObj(GObj *gobj, Sprite *sprite);
extern void efManagerStockSnapMakeEffect(f32 pos_x, f32 pos_y);
extern void efManagerStockStealStartMakeEffect(f32 pos_x, f32 pos_y);
extern void efManagerStockStealEndMakeEffect(f32 pos_x, f32 pos_y);
extern LBParticle *efManagerBattleScoreMakeEffect(Vec3f *pos, s32 score);
extern GObj *gEFParticleStructsGObj;
extern GObj *gEFParticleGeneratorsGObj;
extern void efParticleGObjSetSkipAll(void);
extern void efParticleGObjClearSkipID(u32 id);
extern void gmRumbleInitPlayers(void);
extern void gmRumbleResumeProcessAll(void);
extern void ftParamUnlockPlayerControl(GObj *fighter_gobj);
extern void ftCommonAppearSetStatus(GObj *fighter_gobj);
extern sb32 ftCommonSleepCheckIgnorePauseMenu(GObj *fighter_gobj);
extern void ftPublicDefeatedAddID(u16 sfx_id);
extern void func_ovl65_8018F6DC(void);

static sb32 ndsIFCommonFastIterationIsEnabled(void);
static u32 ndsIFCommonGetTicCount(void);
static void ndsIFCommonSetTicCount(u32 tics);

#define ifCommonEntryAllMakeInterface ndsIFCommonEntryAllMakeInterfaceOriginal
#define ifCommonBattleUpdateInterfaceAll ndsIFCommonBattleUpdateInterfaceAllOriginal
#define sySchedulerGetTicCount ndsIFCommonGetTicCount
#define sySchedulerSetTicCount ndsIFCommonSetTicCount
#include "../../decomp/BattleShip-main/decomp/src/if/ifcommon.c"
#undef sySchedulerSetTicCount
#undef sySchedulerGetTicCount
#undef ifCommonBattleUpdateInterfaceAll
#undef ifCommonEntryAllMakeInterface

static sb32 ndsIFCommonFastIterationIsEnabled(void)
{
    return (gNdsSceneHarnessMode ==
            NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE_REALTIME) &&
           (gNdsBattlePlayableFoxCpuEnabled == 0u);
}

static u32 ndsIFCommonGetTicCount(void)
{
    return (ndsIFCommonFastIterationIsEnabled() != FALSE) ?
           sIFCommonTimerStamp : sySchedulerGetTicCount();
}

static void ndsIFCommonSetTicCount(u32 tics)
{
    if (ndsIFCommonFastIterationIsEnabled() == FALSE)
    {
        sySchedulerSetTicCount(tics);
    }
}

void ifCommonEntryAllMakeInterface(void)
{
    if (ndsIFCommonFastIterationIsEnabled() != FALSE)
    {
        ifCommonAnnounceGoSetStatus();
        return;
    }
    ndsIFCommonEntryAllMakeInterfaceOriginal();
}

void ifCommonBattleUpdateInterfaceAll(void)
{
    ndsIFCommonBattleUpdateInterfaceAllOriginal();

    if (ndsIFCommonFastIterationIsEnabled() != FALSE)
    {
        sIFCommonTimerIsStarted = FALSE;
    }
}

static u32 ndsIFCommonPackDamageDigits(u32 player)
{
    return ((u32)sIFCommonPlayerDamageInterface[player].chars[0].image_id) |
           ((u32)sIFCommonPlayerDamageInterface[player].chars[1].image_id << 8) |
           ((u32)sIFCommonPlayerDamageInterface[player].chars[2].image_id << 16) |
           ((u32)sIFCommonPlayerDamageInterface[player].chars[3].image_id << 24);
}

static void ndsIFCommonRecordDamageState(u32 player)
{
    u32 damage;
    u32 digits;

    if (sIFCommonPlayerDamageInterface[player].interface_gobj != NULL)
    {
        gNdsIFCommonHUDObjectMask |= 1u << player;
    }

    damage = (u32)sIFCommonPlayerDamageInterface[player].damage;
    digits = ndsIFCommonPackDamageDigits(player);

    if (player == 0u)
    {
        gNdsIFCommonHUDP0DamageCurrent = damage;
        if (damage > gNdsIFCommonHUDP0DamageMax)
        {
            gNdsIFCommonHUDP0DamageMax = damage;
            gNdsIFCommonHUDP0DigitCount =
                (u32)sIFCommonPlayerDamageInterface[player].char_display_count;
            gNdsIFCommonHUDP0Digits = digits;
        }
    }
    else
    {
        gNdsIFCommonHUDP1DamageCurrent = damage;
        if (damage > gNdsIFCommonHUDP1DamageMax)
        {
            gNdsIFCommonHUDP1DamageMax = damage;
            gNdsIFCommonHUDP1DigitCount =
                (u32)sIFCommonPlayerDamageInterface[player].char_display_count;
            gNdsIFCommonHUDP1Digits = digits;
        }
    }
}

static void ndsIFCommonRecordStockState(u32 player)
{
    u32 stock_display = (u32)sIFCommonPlayerStocksNum[player];
    volatile u32 *stock_current;
    volatile u32 *stock_min;
    volatile u32 *stock_max;

    if (player == 0u)
    {
        stock_current = &gNdsIFCommonHUDP0StockCurrent;
        stock_min = &gNdsIFCommonHUDP0StockMin;
        stock_max = &gNdsIFCommonHUDP0StockMax;
    }
    else
    {
        stock_current = &gNdsIFCommonHUDP1StockCurrent;
        stock_min = &gNdsIFCommonHUDP1StockMin;
        stock_max = &gNdsIFCommonHUDP1StockMax;
    }

    *stock_current = stock_display;
    if ((stock_display != S8_MAX) && (stock_display > 0u))
    {
        gNdsIFCommonHUDObjectMask |= 1u << (player + 4u);
        if ((*stock_min == 0u) || (stock_display < *stock_min))
        {
            *stock_min = stock_display;
        }
        if (stock_display > *stock_max)
        {
            *stock_max = stock_display;
        }
    }
}

void ndsIFCommonRecordHUDState(void)
{
    u32 active_mask = 0u;
    u32 show_damage_mask = 0u;
    u32 single_stock_mask = 0u;
    u32 cpu_player_mask = 0u;
    u32 player;

    if ((gSCManagerSceneData.scene_curr != nSCKindVSBattle) ||
        (gSCManagerBattleState == NULL))
    {
        return;
    }

    gNdsIFCommonHUDRecordCount++;

    ndsIFCommonRecordDamageState(0u);
    ndsIFCommonRecordDamageState(1u);
    ndsIFCommonRecordStockState(0u);
    ndsIFCommonRecordStockState(1u);

    for (player = 0u; player < 2u; player++)
    {
        if (gSCManagerBattleState->players[player].pkind !=
            nFTPlayerKindNot)
        {
            active_mask |= 1u << player;
        }
        if (sIFCommonPlayerDamageInterface[player].is_show_interface !=
            FALSE)
        {
            show_damage_mask |= 1u << player;
        }
        if (gSCManagerBattleState->players[player].is_single_stockicon !=
            FALSE)
        {
            single_stock_mask |= 1u << player;
        }
        if (gSCManagerBattleState->players[player].pkind ==
            nFTPlayerKindCom)
        {
            cpu_player_mask |= 1u << player;
        }
    }
    gNdsIFCommonHUDActivePlayerMask = active_mask;
    gNdsIFCommonHUDShowDamageMask = show_damage_mask;
    gNdsIFCommonHUDSingleStockMask = single_stock_mask;
    gNdsIFCommonHUDCPUPlayerMask = cpu_player_mask;
    gNdsIFCommonHUDP0FighterKind =
        (u32)gSCManagerBattleState->players[0].fkind;
    gNdsIFCommonHUDP1FighterKind =
        (u32)gSCManagerBattleState->players[1].fkind;
    gNdsIFCommonHUDP0Level =
        (u32)gSCManagerBattleState->players[0].level;
    gNdsIFCommonHUDP1Level =
        (u32)gSCManagerBattleState->players[1].level;
    gNdsIFCommonHUDP0LowerStock =
        (gSCManagerBattleState->players[0].stock_count < 0) ? S8_MAX :
        ((gSCManagerBattleState->players[0].is_single_stockicon != FALSE) ?
         1u : (u32)gSCManagerBattleState->players[0].stock_count + 1u);
    gNdsIFCommonHUDP1LowerStock =
        (gSCManagerBattleState->players[1].stock_count < 0) ? S8_MAX :
        ((gSCManagerBattleState->players[1].is_single_stockicon != FALSE) ?
         1u : (u32)gSCManagerBattleState->players[1].stock_count + 1u);
    gNdsIFCommonHUDTimeRemain = gSCManagerBattleState->time_remain;
    gNdsIFCommonHUDTimerLimit = sIFCommonTimerLimit;
    gNdsIFCommonHUDTimerStarted =
        (sIFCommonTimerIsStarted != FALSE) ? 1u : 0u;
    gNdsIFCommonHUDGameStatus = gSCManagerBattleState->game_status;
}

u32 ndsIFCommonRouteGObjToLowerTextHUD(GObj *gobj)
{
    u32 route = 0u;

    if (gobj == NULL)
    {
        return FALSE;
    }
    if (gNdsIFCommonHUDLowerTextMode == 0u)
    {
        return FALSE;
    }
    if (gobj->proc_display == ifCommonTimerProcDisplay)
    {
        route = 1u;
        gNdsIFCommonHUDLowerTimerRouteCount++;
    }
    else if ((gobj->proc_display == ifCommonPlayerStockMultiProcDisplay) ||
             (gobj->proc_display == ifCommonPlayerStockSingleProcDisplay))
    {
        route = 2u;
        gNdsIFCommonHUDLowerStockRouteCount++;
    }
    if (route != 0u)
    {
        gNdsIFCommonHUDLowerRouteMask |= route;
        gNdsIFCommonHUDLowerRouteCount++;
        return TRUE;
    }
    return FALSE;
}
#endif
