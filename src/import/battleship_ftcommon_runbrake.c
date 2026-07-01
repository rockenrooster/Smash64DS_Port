/*
 * Bounded BattleShip ftcommonrunbrake.c import for the NDS Mario/Fox
 * Run -> RunBrake proof.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>

#define ftCommonRunBrakeProcInterrupt ndsBaseFTCommonRunBrakeProcInterrupt
#define ftCommonRunBrakeProcPhysics ndsBaseFTCommonRunBrakeProcPhysics
#define ftCommonRunBrakeSetStatus ndsBaseFTCommonRunBrakeSetStatus
#define ftCommonRunBrakeCheckInterruptRun ndsBaseFTCommonRunBrakeCheckInterruptRun
#define ftCommonRunBrakeCheckInterruptTurnRun ndsBaseFTCommonRunBrakeCheckInterruptTurnRun

void ndsBaseFTCommonRunBrakeProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonRunBrakeProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonRunBrakeSetStatus(GObj *fighter_gobj, u32 flag);
sb32 ndsBaseFTCommonRunBrakeCheckInterruptRun(GObj *fighter_gobj);
sb32 ndsBaseFTCommonRunBrakeCheckInterruptTurnRun(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonrunbrake.c"

#undef ftCommonRunBrakeProcInterrupt
#undef ftCommonRunBrakeProcPhysics
#undef ftCommonRunBrakeSetStatus
#undef ftCommonRunBrakeCheckInterruptRun
#undef ftCommonRunBrakeCheckInterruptTurnRun
