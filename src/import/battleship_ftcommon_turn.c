/*
 * Bounded BattleShip ftcommonturn.c import for the NDS Mario/Fox
 * Wait -> Turn -> Wait proof. Public symbols are remapped so the port
 * backend can guard the original path and leave unrelated movement branches
 * deferred.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>

#define ftCommonTurnProcUpdate ndsBaseFTCommonTurnProcUpdate
#define ftCommonTurnProcInterrupt ndsBaseFTCommonTurnProcInterrupt
#define ftCommonTurnSetStatus ndsBaseFTCommonTurnSetStatus
#define ftCommonTurnSetStatusCenter ndsBaseFTCommonTurnSetStatusCenter
#define ftCommonTurnSetStatusInvertLR ndsBaseFTCommonTurnSetStatusInvertLR
#define ftCommonTurnCheckInputSuccess ndsBaseFTCommonTurnCheckInputSuccess
#define ftCommonTurnCheckInterruptCommon ndsBaseFTCommonTurnCheckInterruptCommon

void ndsBaseFTCommonTurnProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonTurnProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonTurnSetStatus(GObj *fighter_gobj, s32 lr_dash);
void ndsBaseFTCommonTurnSetStatusCenter(GObj *fighter_gobj);
void ndsBaseFTCommonTurnSetStatusInvertLR(GObj *fighter_gobj);
sb32 ndsBaseFTCommonTurnCheckInputSuccess(GObj *fighter_gobj);
sb32 ndsBaseFTCommonTurnCheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonturn.c"

#undef ftCommonTurnProcUpdate
#undef ftCommonTurnProcInterrupt
#undef ftCommonTurnSetStatus
#undef ftCommonTurnSetStatusCenter
#undef ftCommonTurnSetStatusInvertLR
#undef ftCommonTurnCheckInputSuccess
#undef ftCommonTurnCheckInterruptCommon
