/*
 * Bounded BattleShip ftcommonjump.c import for the NDS Mario/Fox
 * KneeBend -> JumpF proof.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <macros.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>

#define ftCommonJumpProcInterrupt ndsBaseFTCommonJumpProcInterrupt
#define ftCommonJumpGetJumpForceButton ndsBaseFTCommonJumpGetJumpForceButton
#define ftCommonJumpSetStatus ndsBaseFTCommonJumpSetStatus

void ndsBaseFTCommonJumpProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonJumpGetJumpForceButton(s32 stick_range_x,
                                           s32 *jump_vel_x,
                                           s32 *jump_vel_y,
                                           sb32 is_shorthop);
void ndsBaseFTCommonJumpSetStatus(GObj *fighter_gobj);

#if NDS_P4
#include <nds/nds_p4.h>
/* P4: Remix's check_float_ (PeachSpecial.asm) runs in place of this
 * interrupt's jump check, after the aerial check: it is tried where the
 * aerial check ends (ndsP4AirJumpCheck). */
#define ftCommonAttackAirCheckInterruptCommon(g_) \
    (ftCommonAttackAirCheckInterruptCommon(g_) || ndsP4AirJumpCheck(g_))
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonjump.c"
#if NDS_P4
#undef ftCommonAttackAirCheckInterruptCommon
#endif

#undef ftCommonJumpProcInterrupt
#undef ftCommonJumpGetJumpForceButton
#undef ftCommonJumpSetStatus
