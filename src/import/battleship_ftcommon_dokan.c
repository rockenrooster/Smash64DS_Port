/*
 * Mushroom Kingdom warp-pipe common statuses, P2-4 stage 7.
 *
 * `nFTCommonStatusDokanStart/Wait/End/Walk` run the pipe enter, traverse, exit
 * and wall-interior walk callbacks the shared status table already names
 * (decomp ftcommonstatus.h:1267-1327). A fighter that entered a pipe used to
 * freeze there: the enter path lived in reloc_backend_compat_shims.c while
 * every Dokan proc was still an inactive weak stub.
 *
 * This TU imports decomp ft/ftcommon/ftcommondokan.c verbatim. Only the enter
 * interrupt keeps a port wrapper: ftCommonDokanStartCheckInterruptCommon is
 * remapped to ndsBaseFTCommonDokanStartCheckInterruptCommon so the shims
 * wrapper can keep its DownWaitLoop proof gate and its deferred-interrupt
 * fallback around the source test.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <macros.h>
#include <sys/audio.h>
#include <sys/obj.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* BattleShip ft/ftcommon.h:133-141. Port include/ft/fighter.h publishes no
 * FTCOMMON_DOKAN_* constants, so they live here with their source. */
#ifndef FTCOMMON_DOKAN_STICK_RANGE_MIN
#define FTCOMMON_DOKAN_STICK_RANGE_MIN (-53)
#define FTCOMMON_DOKAN_BUFFER_TICS_MAX 4
#define FTCOMMON_DOKAN_PLAYERTAG_WAIT 20
#define FTCOMMON_DOKAN_TURN_STOP_WAIT_DEFAULT 8
#define FTCOMMON_DOKAN_TURN_STEP (F_CLC_DTOR32(90.0F) / FTCOMMON_DOKAN_TURN_STOP_WAIT_DEFAULT)
#define FTCOMMON_DOKAN_POS_ADJUST 25.0F
#define FTCOMMON_DOKAN_DETECT_WIDTH 200.0F
#define FTCOMMON_DOKAN_POS_ADJUST_WAIT 30.0F
#define FTCOMMON_DOKAN_EXIT_WAIT 30.0F
#endif

/* Source declarations kept narrow at the port ABI seam. */
f32 syUtilsRandFloat(void);
f32 gcGetInterpValueCubic(f32 length_invert, f32 length, f32 value_base, f32 value_target, f32 rate_base, f32 rate_target);
sb32 mpCollisionCheckProjectRWall(Vec3f *position, s32 *project_line_id, f32 *ga_dist, u32 *stand_coll_flags, Vec3f *angle);
void ftCommonDokanWaitSetStatus(GObj *fighter_gobj);
void ftCommonDokanEndSetStatus(GObj *fighter_gobj);
void ftCommonDokanWalkSetStatus(GObj *fighter_gobj);

#define ftCommonDokanStartCheckInterruptCommon ndsBaseFTCommonDokanStartCheckInterruptCommon
sb32 ndsBaseFTCommonDokanStartCheckInterruptCommon(GObj *fighter_gobj);

#if NDS_P2_STAGE_INISHIE
/* Owned by src/import/battleship_grinishie_ground.c (decomp grinishie.c:402)
 * wherever Mushroom Kingdom is built. */
void grInishiePakkunSetWaitFighter(void);
#else
static inline void grInishiePakkunSetWaitFighter(void)
{
}
#endif

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondokan.c"

#undef ftCommonDokanStartCheckInterruptCommon
