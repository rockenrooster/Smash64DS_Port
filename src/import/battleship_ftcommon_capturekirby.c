/*
 * P2-3 Kirby. Inhale's VICTIM side.
 *
 * `nFTCommonStatusCaptureKirby`, `nFTCommonStatusCaptureWaitKirby`,
 * `nFTCommonStatusThrownKirbyStar` and `nFTCommonStatusThrownCopyStar` are
 * COMMON statuses, so the fighter in them is whoever Kirby inhaled, running
 * common callbacks the shared status table already names (inactive stubs
 * until this TU is built). Same shape as battleship_ftcommon_captureyoshi.c.
 * The grabber half is battleship_kirby.c (ftkirbyspecialn.c).
 */
#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <ft/ftstatus_callbacks.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <it/item.h>
#include <macros.h>
#include <mp/map.h>
#include <sys/audio.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* BattleShip ftcommon.h:277-289. */
#define FTCOMMON_CAPTUREKIRBY_WIGGLE_STICK_RANGE_MIN 53
#define FTCOMMON_CAPTUREKIRBY_WIGGLE_BUFFER_TICS_MAX 4
#define FTCOMMON_CAPTUREKIRBY_MAGNITUDE_MAX 220.0F
#define FTCOMMON_CAPTUREKIRBY_MAGNITUDE_MUL 0.45F
#define FTCOMMON_CAPTUREKIRBY_MAGNITUDE_ADD 0.55F
#define FTCOMMON_CAPTUREKIRBY_DIST_X_MIN 28.0F
#define FTCOMMON_CAPTUREKIRBY_DIST_Y_MIN 36.0F
#define FTCOMMON_CAPTUREKIRBY_WIGGLE_VEL 20.0F
#define FTCOMMON_THROWNKIRBYSTAR_BREAKOUT_INPUTS_MIN 3
#define FTCOMMON_THROWNKIRBYSTAR_DECELERATE 4.0F
#define FTCOMMON_THROWNKIRBYSTAR_RELEASE_VEL_X 22.0F
#define FTCOMMON_THROWNKIRBYSTAR_RELEASE_VEL_Y 70.0F

#include "battleship_kirby_common.h"

/* Source declarations kept narrow at the port ABI seam. */
void ftCommonCaptureKirbyProcPhysics(GObj *fighter_gobj);
void ftCommonCaptureWaitKirbyProcInterrupt(GObj *fighter_gobj);
void ftCommonCaptureWaitKirbyProcMap(GObj *fighter_gobj);
void ftCommonThrownKirbyStarProcUpdate(GObj *fighter_gobj);
void ftCommonThrownKirbyStarProcPhysics(GObj *fighter_gobj);
void ftCommonThrownKirbyStarProcStatus(GObj *fighter_gobj);
void ftCommonThrownCopyStarProcUpdate(GObj *fighter_gobj);
void ftCommonThrownCopyStarProcPhysics(GObj *fighter_gobj);
void ftCommonThrownCopyStarProcStatus(GObj *fighter_gobj);
void ftCommonCaptureWaitProcMap(GObj *fighter_gobj);
void ftCommonCaptureWaitSetStatus(GObj *fighter_gobj);
void mpCollisionGetFloorEdgeL(s32 line_id, Vec3f *object_pos);
void mpCollisionGetFloorEdgeR(s32 line_id, Vec3f *object_pos);

/* BattleShip fttypes.h:532-538, the same local mirror
 * battleship_ftcommon_catch.c keeps; the two knockback helpers are that TU's
 * base-named imports of ftcommoncapture.c. */
typedef struct NDSFTThrowReleaseDesc
{
    s32 angle;
    s32 knockback_scale;
    s32 knockback_weight;
    s32 knockback_base;
} FTThrowReleaseDesc;
void ndsBaseFTCommonCaptureApplyCatchKnockback(GObj *fighter_gobj,
                                               FTThrowReleaseDesc *throw_release);
void ndsBaseFTCommonCaptureApplyCaptureKnockback(GObj *fighter_gobj,
                                                 FTThrowReleaseDesc *throw_release);
#define ftCommonCaptureApplyCatchKnockback ndsBaseFTCommonCaptureApplyCatchKnockback
#define ftCommonCaptureApplyCaptureKnockback ndsBaseFTCommonCaptureApplyCaptureKnockback

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncapturekirby.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncapturewait.c"
