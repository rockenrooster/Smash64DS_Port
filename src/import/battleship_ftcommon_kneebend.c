/*
 * Bounded BattleShip ftcommonkneebend.c import for the NDS Mario/Fox
 * RunBrake -> Wait -> KneeBend -> Jump proof.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>

#define ftCommonKneeBendProcUpdate ndsBaseFTCommonKneeBendProcUpdate
#define ftCommonKneeBendProcInterrupt ndsBaseFTCommonKneeBendProcInterrupt
#define ftCommonKneeBendSetStatusParam ndsBaseFTCommonKneeBendSetStatusParam
#define ftCommonKneeBendSetStatus ndsBaseFTCommonKneeBendSetStatus
#define ftCommonGuardKneeBendSetStatus ndsBaseFTCommonGuardKneeBendSetStatus
#define ftCommonKneeBendCheckButtonTap ndsBaseFTCommonKneeBendCheckButtonTap
#define ftCommonKneeBendGetInputTypeCommon ndsBaseFTCommonKneeBendGetInputTypeCommon
#define ftCommonKneeBendCheckInterruptCommon ndsBaseFTCommonKneeBendCheckInterruptCommon
#define ftCommonKneeBendGetInputTypeRun ndsBaseFTCommonKneeBendGetInputTypeRun
#define ftCommonKneeBendCheckInterruptRun ndsBaseFTCommonKneeBendCheckInterruptRun
#define ftCommonGuardKneeBendCheckInterruptGuard ndsBaseFTCommonGuardKneeBendCheckInterruptGuard

void ndsBaseFTCommonKneeBendProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonKneeBendProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonKneeBendSetStatusParam(GObj *fighter_gobj,
                                           s32 status_id,
                                           s32 input_source);
void ndsBaseFTCommonKneeBendSetStatus(GObj *fighter_gobj,
                                      s32 input_source);
void ndsBaseFTCommonGuardKneeBendSetStatus(GObj *fighter_gobj,
                                           s32 input_source);
sb32 ndsBaseFTCommonKneeBendCheckButtonTap(FTStruct *fp);
s32 ndsBaseFTCommonKneeBendGetInputTypeCommon(FTStruct *fp);
sb32 ndsBaseFTCommonKneeBendCheckInterruptCommon(GObj *fighter_gobj);
s32 ndsBaseFTCommonKneeBendGetInputTypeRun(FTStruct *fp);
sb32 ndsBaseFTCommonKneeBendCheckInterruptRun(GObj *fighter_gobj);
sb32 ndsBaseFTCommonGuardKneeBendCheckInterruptGuard(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonkneebend.c"

#undef ftCommonKneeBendProcUpdate
#undef ftCommonKneeBendProcInterrupt
#undef ftCommonKneeBendSetStatusParam
#undef ftCommonKneeBendSetStatus
#undef ftCommonGuardKneeBendSetStatus
#undef ftCommonKneeBendCheckButtonTap
#undef ftCommonKneeBendGetInputTypeCommon
#undef ftCommonKneeBendCheckInterruptCommon
#undef ftCommonKneeBendGetInputTypeRun
#undef ftCommonKneeBendCheckInterruptRun
#undef ftCommonGuardKneeBendCheckInterruptGuard
