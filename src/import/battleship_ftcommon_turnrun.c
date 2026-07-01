/*
 * Bounded BattleShip ftcommonturnrun.c import for the NDS Mario/Fox
 * Run -> TurnRun proof.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>

#define ftCommonTurnRunProcUpdate ndsBaseFTCommonTurnRunProcUpdate
#define ftCommonTurnRunProcInterrupt ndsBaseFTCommonTurnRunProcInterrupt
#define ftCommonTurnRunSetStatus ndsBaseFTCommonTurnRunSetStatus
#define ftCommonTurnRunCheckInterruptRun ndsBaseFTCommonTurnRunCheckInterruptRun
#define ftCommonRunSetStatus ndsBaseFTCommonRunSetStatus
#define ftCommonRunBrakeCheckInterruptTurnRun ndsBaseFTCommonRunBrakeCheckInterruptTurnRun

void ndsBaseFTCommonTurnRunProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonTurnRunProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonTurnRunSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonTurnRunCheckInterruptRun(GObj *fighter_gobj);
void ndsBaseFTCommonRunSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonRunBrakeCheckInterruptTurnRun(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonturnrun.c"

#undef ftCommonTurnRunProcUpdate
#undef ftCommonTurnRunProcInterrupt
#undef ftCommonTurnRunSetStatus
#undef ftCommonTurnRunCheckInterruptRun
#undef ftCommonRunSetStatus
#undef ftCommonRunBrakeCheckInterruptTurnRun
