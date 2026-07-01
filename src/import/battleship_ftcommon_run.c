/*
 * Bounded BattleShip ftcommonrun.c import for the NDS Mario/Fox
 * Dash -> Run proof.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>

#define ftCommonRunProcInterrupt ndsBaseFTCommonRunProcInterrupt
#define ftCommonRunSetStatus ndsBaseFTCommonRunSetStatus
#define ftCommonRunCheckInterruptDash ndsBaseFTCommonRunCheckInterruptDash

void ndsBaseFTCommonRunProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonRunSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonRunCheckInterruptDash(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonrun.c"

#undef ftCommonRunProcInterrupt
#undef ftCommonRunSetStatus
#undef ftCommonRunCheckInterruptDash
