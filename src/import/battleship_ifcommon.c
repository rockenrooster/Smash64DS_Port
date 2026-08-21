#if NDS_IMPORT_BATTLESHIP_IFCOMMON
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <nds/nds_battle_hud.h>
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

/* NOT INSTRUMENTED HERE, and the reason is worth keeping: the announcement
 * question ("does the source announce GAME SET / TIME UP and the port fail to
 * draw, or does the source never announce?") CANNOT be answered by wrapping
 * these three functions at this seam. A `#define` rename renames the decomp's
 * DEFINITION and its internal call sites together, and every call that matters
 * here is internal to `ifcommon.c` (`sIFCommonBattlePlace` reaching 0 calls
 * `ifCommonAnnounceEndMessage`, which calls the GameSet constructor). So the
 * wrappers end up with no callers, `--gc-sections` drops them, and their
 * counters vanish from the ELF -- measured 2026-07-31: `nm` had no
 * `gNdsIFCommonAnnounceGameSetCount` at all while the EndMessage counter
 * survived only because `ftcommondead.c` and `sc1pgame.c` call that name from
 * OTHER translation units. This is the same seam limitation the L7/L9 rows
 * record; it is a property of the technique, not a mistake to retry.
 *
 * Read the source's own state instead -- `sIFCommonBattlePlace` and
 * `gSCManagerBattleState->game_status` -- which is what the harnesses now do. */

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

/* P2-2: the source interface arrays are GMCOMMON_PLAYERS_MAX wide. Keep the
 * long-standing P0/P1 probe symbols for compatibility, but select the matching
 * per-player publication slot instead of collapsing every player > 0 into P1. */
static volatile u32 *ndsIFCommonDamageCurrentPtr(u32 player)
{
    switch (player)
    {
    case 0u: return &gNdsIFCommonHUDP0DamageCurrent;
    case 1u: return &gNdsIFCommonHUDP1DamageCurrent;
    case 2u: return &gNdsIFCommonHUDP2DamageCurrent;
    default: return &gNdsIFCommonHUDP3DamageCurrent;
    }
}

static volatile u32 *ndsIFCommonDamageMaxPtr(u32 player)
{
    switch (player)
    {
    case 0u: return &gNdsIFCommonHUDP0DamageMax;
    case 1u: return &gNdsIFCommonHUDP1DamageMax;
    case 2u: return &gNdsIFCommonHUDP2DamageMax;
    default: return &gNdsIFCommonHUDP3DamageMax;
    }
}

static volatile u32 *ndsIFCommonDigitCountPtr(u32 player)
{
    switch (player)
    {
    case 0u: return &gNdsIFCommonHUDP0DigitCount;
    case 1u: return &gNdsIFCommonHUDP1DigitCount;
    case 2u: return &gNdsIFCommonHUDP2DigitCount;
    default: return &gNdsIFCommonHUDP3DigitCount;
    }
}

static volatile u32 *ndsIFCommonDigitsPtr(u32 player)
{
    switch (player)
    {
    case 0u: return &gNdsIFCommonHUDP0Digits;
    case 1u: return &gNdsIFCommonHUDP1Digits;
    case 2u: return &gNdsIFCommonHUDP2Digits;
    default: return &gNdsIFCommonHUDP3Digits;
    }
}

static volatile u32 *ndsIFCommonStockCurrentPtr(u32 player)
{
    switch (player)
    {
    case 0u: return &gNdsIFCommonHUDP0StockCurrent;
    case 1u: return &gNdsIFCommonHUDP1StockCurrent;
    case 2u: return &gNdsIFCommonHUDP2StockCurrent;
    default: return &gNdsIFCommonHUDP3StockCurrent;
    }
}

static volatile u32 *ndsIFCommonStockMinPtr(u32 player)
{
    switch (player)
    {
    case 0u: return &gNdsIFCommonHUDP0StockMin;
    case 1u: return &gNdsIFCommonHUDP1StockMin;
    case 2u: return &gNdsIFCommonHUDP2StockMin;
    default: return &gNdsIFCommonHUDP3StockMin;
    }
}

static volatile u32 *ndsIFCommonStockMaxPtr(u32 player)
{
    switch (player)
    {
    case 0u: return &gNdsIFCommonHUDP0StockMax;
    case 1u: return &gNdsIFCommonHUDP1StockMax;
    case 2u: return &gNdsIFCommonHUDP2StockMax;
    default: return &gNdsIFCommonHUDP3StockMax;
    }
}

static void ndsIFCommonRecordDamageState(u32 player)
{
    u32 damage;
    u32 digits;
    volatile u32 *damage_current;
    volatile u32 *damage_max;
    volatile u32 *digit_count;
    volatile u32 *digit_bits;

    if (sIFCommonPlayerDamageInterface[player].interface_gobj != NULL)
    {
        gNdsIFCommonHUDObjectMask |= 1u << player;
    }

    damage = (u32)sIFCommonPlayerDamageInterface[player].damage;
    digits = ndsIFCommonPackDamageDigits(player);
    damage_current = ndsIFCommonDamageCurrentPtr(player);
    damage_max = ndsIFCommonDamageMaxPtr(player);
    digit_count = ndsIFCommonDigitCountPtr(player);
    digit_bits = ndsIFCommonDigitsPtr(player);

    *damage_current = damage;
    if (damage > *damage_max)
    {
        *damage_max = damage;
        *digit_count =
            (u32)sIFCommonPlayerDamageInterface[player].char_display_count;
        *digit_bits = digits;
    }
}

static void ndsIFCommonRecordStockState(u32 player)
{
    u32 stock_display = (u32)sIFCommonPlayerStocksNum[player];
    volatile u32 *stock_current;
    volatile u32 *stock_min;
    volatile u32 *stock_max;

    stock_current = ndsIFCommonStockCurrentPtr(player);
    stock_min = ndsIFCommonStockMinPtr(player);
    stock_max = ndsIFCommonStockMaxPtr(player);

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

u32 ndsIFCommonGetBattleHudDamageState(u32 player,
                                       NDSBattleHudDamageState *out)
{
    IFPlayerDamage *source;
    f32 damage_scale;
    u32 color_id;
    u32 char_count;
    u32 visible;
    u32 i;

    if ((out == NULL) || (player >= (u32)GMCOMMON_PLAYERS_MAX))
    {
        return FALSE;
    }

    source = &sIFCommonPlayerDamageInterface[player];
    char_count = source->char_display_count;
    if (char_count > NDS_BATTLE_HUD_DAMAGE_CHARS)
    {
        char_count = NDS_BATTLE_HUD_DAMAGE_CHARS;
    }

    /* ifCommonPlayerDamageProcDisplay:795-802.  This is deliberately the
     * source's DISPLAY predicate rather than the broader active-player mask:
     * after a stock reaches -1 the digits linger for dead_stopupdate_wait, and
     * only then disappear. */
    visible = ((source->is_show_interface != FALSE) &&
               ((gSCManagerBattleState->players[player].stock_count >= 0) ||
                (source->dead_stopupdate_wait != 0))) ? TRUE : FALSE;

    out->scale = source->scale;
    out->damage = source->damage;
    color_id = source->color_id;
    if (color_id > (u32)GMCOMMON_PLAYERS_MAX)
    {
        color_id = player;
    }
    out->color_id = (u8)color_id;
    if (color_id == (u32)GMCOMMON_PLAYERS_MAX)
    {
        out->color_r = dIFCommonPlayerDamageDigitColorsR[color_id];
        out->color_g = dIFCommonPlayerDamageDigitColorsG[color_id];
        out->color_b = dIFCommonPlayerDamageDigitColorsB[color_id];
    }
    else
    {
        /* ifCommonPlayerDamageProcDisplay:815-823, expression-for-expression.
         * The DS sink consumes the already-resolved primitive colour instead
         * of reimplementing this float/truncation rule with integer math. */
        damage_scale = 1.0F - (source->damage / 300.0F);
        if (damage_scale < 0.0F)
        {
            damage_scale = 0.0F;
        }
        out->color_r = (u8)((s32)
            ((dIFCommonPlayerDamageDigitColorsR[color_id] - 0x64) *
             damage_scale) + 0x64);
        out->color_g = (u8)((s32)
            ((dIFCommonPlayerDamageDigitColorsG[color_id] - 0x14) *
             damage_scale) + 0x14);
        out->color_b = (u8)((s32)
            ((dIFCommonPlayerDamageDigitColorsB[color_id] - 0x14) *
             damage_scale) + 0x14);
    }
    out->is_update_anim = source->is_update_anim;
    out->char_count = (u8)char_count;
    out->visible = (u8)visible;

    for (i = 0u; i < NDS_BATTLE_HUD_DAMAGE_CHARS; i++)
    {
        out->chars[i].pos_x = source->chars[i].pos.x;
        out->chars[i].pos_y = source->chars[i].pos.y;
        out->chars[i].image_id = source->chars[i].image_id;
        out->chars[i].visible =
            (u8)((visible != FALSE) && (i < char_count));
    }
    return TRUE;
}

s32 ndsIFCommonBattleHudInterfaceVisible(void)
{
    /* Source visibility of the whole interface link.  The meters/timer/stocks
     * hide exactly when ifCommonInterfaceSetGObjFlagsAll(GOBJ_FLAG_HIDDEN) has
     * run -- at game end via ifCommonBattleInterfaceProcSet (which then sets
     * game_status = Set, ifcommon.c:2974/:3305) and from pause (:2884, status
     * Pause; Unpause restores the flags before returning to Go, :3089).  The
     * announce window between End and Set still shows the meters, so only
     * Set and Pause are hidden.  Outside VSBattle the native sub HUD must
     * never render: its mirrors freeze once the interface gobjs stop drawing,
     * which is what used to redraw the battle HUD over the Results screen. */
    return ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
            (gSCManagerBattleState != NULL) &&
            (gSCManagerBattleState->game_status != nSCBattleGameStatusSet) &&
            (gSCManagerBattleState->game_status != nSCBattleGameStatusPause))
               ? TRUE
               : FALSE;
}

void ndsIFCommonRecordHUDState(void)
{
    u32 active_mask = 0u;
    u32 show_damage_mask = 0u;
    u32 damage_flash_mask = 0u;
    u32 single_stock_mask = 0u;
    u32 cpu_player_mask = 0u;
    u32 player;

    if ((gSCManagerSceneData.scene_curr != nSCKindVSBattle) ||
        (gSCManagerBattleState == NULL))
    {
        return;
    }

    gNdsIFCommonHUDRecordCount++;

    for (player = 0u; player < (u32)GMCOMMON_PLAYERS_MAX; player++)
    {
        if (gSCManagerBattleState->players[player].pkind !=
            nFTPlayerKindNot)
        {
            active_mask |= 1u << player;
            ndsIFCommonRecordDamageState(player);
            ndsIFCommonRecordStockState(player);

            /* ifCommonPlayerStockInitInterface (source :1287-1308) tests
             * pkind first and never creates a stock GObj for an empty slot.
             * is_single_stockicon is therefore meaningful only for an active
             * player.  Empty transfer slots can retain TRUE in that field;
             * publishing it outside this branch made a 2-player match report
             * a four-player single-stock mask even though BattleShip created
             * only two stock interfaces. */
            if (gSCManagerBattleState->players[player].is_single_stockicon !=
                FALSE)
            {
                single_stock_mask |= 1u << player;
            }
        }
        if (sIFCommonPlayerDamageInterface[player].is_show_interface !=
            FALSE)
        {
            show_damage_mask |= 1u << player;
        }
        if (sIFCommonPlayerDamageInterface[player].color_id ==
            GMCOMMON_PLAYERS_MAX)
        {
            damage_flash_mask |= 1u << player;
        }
        if (gSCManagerBattleState->players[player].pkind ==
            nFTPlayerKindCom)
        {
            cpu_player_mask |= 1u << player;
        }
    }
    gNdsIFCommonHUDActivePlayerMask = active_mask;
    gNdsIFCommonHUDShowDamageMask = show_damage_mask;
    gNdsIFCommonHUDDamageFlashMask = damage_flash_mask;
    gNdsIFCommonHUDSingleStockMask = single_stock_mask;
    gNdsIFCommonHUDCPUPlayerMask = cpu_player_mask;
    gNdsIFCommonHUDP0FighterKind =
        (u32)gSCManagerBattleState->players[0].fkind;
    gNdsIFCommonHUDP1FighterKind =
        (u32)gSCManagerBattleState->players[1].fkind;
    gNdsIFCommonHUDP2FighterKind =
        (u32)gSCManagerBattleState->players[2].fkind;
    gNdsIFCommonHUDP3FighterKind =
        (u32)gSCManagerBattleState->players[3].fkind;
    gNdsIFCommonHUDP0Level =
        (u32)gSCManagerBattleState->players[0].level;
    gNdsIFCommonHUDP1Level =
        (u32)gSCManagerBattleState->players[1].level;
    gNdsIFCommonHUDP2Level =
        (u32)gSCManagerBattleState->players[2].level;
    gNdsIFCommonHUDP3Level =
        (u32)gSCManagerBattleState->players[3].level;
    /* P2-2 lower OBJ HUD: source stock sprites select their LUT from the live
     * fighter costume (`ifCommonPlayerStockMultiProcDisplay`:1051-1054).  Carry
     * that source field rather than re-deriving FFA/team colour in the DS sink. */
    gNdsIFCommonHUDP0Costume =
        (u32)gSCManagerBattleState->players[0].costume;
    gNdsIFCommonHUDP1Costume =
        (u32)gSCManagerBattleState->players[1].costume;
    gNdsIFCommonHUDP2Costume =
        (u32)gSCManagerBattleState->players[2].costume;
    gNdsIFCommonHUDP3Costume =
        (u32)gSCManagerBattleState->players[3].costume;
    gNdsIFCommonHUDP0LowerStock =
        (gSCManagerBattleState->players[0].stock_count < 0) ? S8_MAX :
        ((gSCManagerBattleState->players[0].is_single_stockicon != FALSE) ?
         1u : (u32)gSCManagerBattleState->players[0].stock_count + 1u);
    gNdsIFCommonHUDP1LowerStock =
        (gSCManagerBattleState->players[1].stock_count < 0) ? S8_MAX :
        ((gSCManagerBattleState->players[1].is_single_stockicon != FALSE) ?
         1u : (u32)gSCManagerBattleState->players[1].stock_count + 1u);
    gNdsIFCommonHUDP2LowerStock =
        (gSCManagerBattleState->players[2].stock_count < 0) ? S8_MAX :
        ((gSCManagerBattleState->players[2].is_single_stockicon != FALSE) ?
         1u : (u32)gSCManagerBattleState->players[2].stock_count + 1u);
    gNdsIFCommonHUDP3LowerStock =
        (gSCManagerBattleState->players[3].stock_count < 0) ? S8_MAX :
        ((gSCManagerBattleState->players[3].is_single_stockicon != FALSE) ?
         1u : (u32)gSCManagerBattleState->players[3].stock_count + 1u);
    gNdsIFCommonHUDTimeRemain = gSCManagerBattleState->time_remain;
    gNdsIFCommonHUDTimerLimit = sIFCommonTimerLimit;
    gNdsIFCommonHUDTimerStarted =
        (sIFCommonTimerIsStarted != FALSE) ? 1u : 0u;
    /* Same admission predicate as ifCommonTimerMakeDigits (:2445-2448).  A
     * zero time value alone cannot distinguish an absent/infinite timer from a
     * visible timer that actually reached 0:00. */
    gNdsIFCommonHUDTimerVisible =
        ((gSCManagerBattleState->game_rules & SCBATTLE_GAMERULE_TIME) &&
         (gSCManagerBattleState->time_limit != SCBATTLE_TIMELIMIT_INFINITE)) ?
            1u : 0u;
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
    else if (gobj->proc_display == ifCommonPlayerDamageProcDisplay)
    {
        /* P2-2's owner-approved screen split moves the steady VS HUD below:
         * timer, stock AND damage. The source damage GObj remains live -- its
         * proc/update state is still the authority recorded by
         * ndsIFCommonRecordHUDState -- but its source top-screen display must
         * not be composed a second time after the DS lower-screen sink has
         * consumed that state. Countdown/GO and other interface GObjs are not
         * admitted here and therefore keep their source top-screen route. */
        route = 4u;
        gNdsIFCommonHUDLowerDamageRouteCount++;
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
