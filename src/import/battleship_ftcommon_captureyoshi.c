/*
 * P2-3 Yoshi. Egg Lay's VICTIM side.
 *
 * The source keeps this in ft/ftcommon, not in ft/ftchar/ftyoshi, and that is
 * the right ownership: `nFTCommonStatusCaptureYoshi` and
 * `nFTCommonStatusYoshiEgg` are COMMON statuses, so the fighter that ends up
 * in them is any fighter Yoshi swallowed, running common callbacks the shared
 * status table already names (until this TU is built they are
 * NDS_INACTIVE_STATUS_STUBs). Same shape as battleship_ftcommon_capturecaptain.c
 * for Falcon Dive.
 *
 * The grabber half is battleship_yoshi.c (ftyoshispecialn.c).
 */
#include <ef/effect.h>
#include <ft/fighter.h>
#include <ft/ftstatus_callbacks.h>
#include <gm/gmsound.h>
#include <it/item.h>
#include <macros.h>
#include <sys/audio.h>

/* BattleShip ftcommon.h:294-312, REGION_US arm. Restated beside the code that
 * consumes them rather than pulling the broad decomp header into the ABI. */
#define FTCOMMON_YOSHIEGG_INTANGIBLE_TIMER 12
#define FTCOMMON_YOSHIEGG_BREAKOUT_INPUTS_MIN 750
#define FTCOMMON_YOSHIEGG_ESCAPE_WAIT_MAX 250
#define FTCOMMON_YOSHIEGG_ESCAPE_WAIT_DEFAULT 15
#define FTCOMMON_YOSHIEGG_WIGGLE_STICK_RANGE_MIN 26
#define FTCOMMON_YOSHIEGG_LAY_VEL_X 20.0F
#define FTCOMMON_YOSHIEGG_LAY_VEL_Y 60.0F
#define FTCOMMON_YOSHIEGG_LAY_OFF_X 200.0F
#define FTCOMMON_YOSHIEGG_LAY_OFF_Y 90.0F
#define FTCOMMON_YOSHIEGG_DAMAGE_MUL 0.5F
#define FTCOMMON_YOSHIEGG_WIGGLE_GFX_RANGE_XY 22.0F
#define FTCOMMON_YOSHIEGG_WIGGLE_ANIM_SPEED 5.0F
#define FTCOMMON_YOSHIEGG_ESCAPE_OFF_Y 10.0F
#define FTCOMMON_YOSHIEGG_ESCAPE_VEL_Y 70.0F

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* The port's catch TU compiles the source pulled-victim helper under this
 * name (battleship_ftcommon_catch.c); Yoshi's victim reads the same body. */
#define ftCommonCapturePulledRotateScale \
    ndsBaseFTCommonCapturePulledRotateScale
void ndsBaseFTCommonCapturePulledRotateScale(GObj *fighter_gobj,
                                             Vec3f *this_pos,
                                             Vec3f *rotate);

/* Narrow declarations normally supplied by BattleShip's broad ft/ef include
 * graph and not yet in the port fighter header; every implementation is
 * already imported by the shared owners. */
GObj *efManagerYoshiEggLayMakeEffect(GObj *fighter_gobj);
LBParticle *efManagerYoshiEggExplodeMakeEffect(Vec3f *pos);
LBParticle *efManagerEggBreakMakeEffect(Vec3f *pos);
void ftParamSetTimedHitStatusIntangible(FTStruct *fp, s32 timer);
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);

/* Source ftcommoncaptureyoshi declarations kept narrow at the port ABI seam. */
void ftCommonCaptureYoshiProcPhysics(GObj *fighter_gobj);
void ftCommonCaptureYoshiProcCapture(GObj *fighter_gobj, GObj *capture_gobj);
void ftCommonYoshiEggMakeEffect(GObj *fighter_gobj);
void ftCommonYoshiEggProcUpdate(GObj *fighter_gobj);
void ftCommonYoshiEggProcInterrupt(GObj *fighter_gobj);
void ftCommonYoshiEggProcPhysics(GObj *fighter_gobj);
void ftCommonYoshiEggProcMap(GObj *fighter_gobj);
void ftCommonYoshiEggProcTrap(GObj *fighter_gobj);
void ftCommonYoshiEggSetDamageCollCollisions(GObj *fighter_gobj);
void ftCommonYoshiEggProcStatus(GObj *fighter_gobj);
void ftCommonYoshiEggSetStatus(GObj *fighter_gobj);

void ftKirbySpecialNApplyCaptureDamage(GObj *kirby_gobj, GObj *victim_gobj,
                                       s32 damage);

#if !NDS_P2_KIRBY
/* BattleShip ft/ftchar/ftkirby/ftkirbyspecialn.c:20 (0x80161CA0), verbatim.
 * The source egg calls Kirby's capture-damage helper for the 5% the victim
 * takes on being laid ("Br0h why", the source's own comment); when Kirby's
 * TU is not built, the one body the egg needs is carried here with the rest
 * of Yoshi's capture seam. */
void ftKirbySpecialNApplyCaptureDamage(GObj *kirby_gobj, GObj *victim_gobj,
                                       s32 damage);
void ftKirbySpecialNApplyCaptureDamage(GObj *kirby_gobj, GObj *victim_gobj,
                                       s32 damage)
{
    FTStruct *kirby_fp = ftGetStruct(kirby_gobj);
    FTStruct *victim_fp = ftGetStruct(victim_gobj);
    s32 star_damage_victim = ftParamGetStaledDamage(
        kirby_fp->player, damage, kirby_fp->motion_attack_id,
        kirby_fp->motion_count);

    damage = star_damage_victim;

    ftCommonDamageUpdateDamageColAnim(
        victim_gobj,
        ftParamGetCommonKnockback(
            victim_fp->percent_damage, star_damage_victim, star_damage_victim,
            0, 100, 0, victim_fp->attr->weight, kirby_fp->handicap,
            victim_fp->handicap),
        0);
    ftParamUpdateDamage(victim_fp, damage);
    ftParamUpdatePlayerBattleStats(kirby_fp->player, victim_fp->player, damage);
    ftParamUpdateStaleQueue(kirby_fp->player, victim_fp->player,
                            kirby_fp->motion_attack_id, kirby_fp->motion_count);
}
#endif

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncaptureyoshi.c"
