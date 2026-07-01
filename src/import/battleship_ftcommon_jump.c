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

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonjump.c"

#undef ftCommonJumpProcInterrupt
#undef ftCommonJumpGetJumpForceButton
#undef ftCommonJumpSetStatus
