/*
 * Bounded BattleShip ftcommonattack100.c import for the NDS Mario/Fox
 * Attack12 -> Fox rapid-jab proof. Public symbols are remapped so the port
 * backend can guard the original path and keep full rapid-jab gameplay
 * deferred.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <sys/obj.h>

#define ftCommonAttack100StartProcUpdate \
    ndsBaseFTCommonAttack100StartProcUpdate
#define ftCommonAttack100StartSetStatus \
    ndsBaseFTCommonAttack100StartSetStatus
#define ftCommonAttack100LoopKirbyUpdateEffect \
    ndsBaseFTCommonAttack100LoopKirbyUpdateEffect
#define ftCommonAttack100LoopProcUpdate \
    ndsBaseFTCommonAttack100LoopProcUpdate
#define ftCommonAttack100LoopProcInterrupt \
    ndsBaseFTCommonAttack100LoopProcInterrupt
#define ftCommonAttack100LoopSetStatus \
    ndsBaseFTCommonAttack100LoopSetStatus
#define ftCommonAttack100EndSetStatus \
    ndsBaseFTCommonAttack100EndSetStatus
#define ftCommonAttack100StartCheckInterruptCommon \
    ndsBaseFTCommonAttack100StartCheckInterruptCommon

void ndsBaseFTCommonAttack100StartProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100StartSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopKirbyUpdateEffect(FTStruct *fp);
void ndsBaseFTCommonAttack100LoopProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100EndSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack100StartCheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattack100.c"

#undef ftCommonAttack100StartProcUpdate
#undef ftCommonAttack100StartSetStatus
#undef ftCommonAttack100LoopKirbyUpdateEffect
#undef ftCommonAttack100LoopProcUpdate
#undef ftCommonAttack100LoopProcInterrupt
#undef ftCommonAttack100LoopSetStatus
#undef ftCommonAttack100EndSetStatus
#undef ftCommonAttack100StartCheckInterruptCommon
