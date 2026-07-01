#include <ft/fighter.h>
#include <mp/map.h>
#include <sys/obj.h>

#define ftCommonCliffClimbOrFallCheckInterruptCommon \
    ndsBaseFTCommonCliffClimbOrFallCheckInterruptCommon
#define ftCommonCliffCommon2UpdateCollData \
    ndsBaseFTCommonCliffCommon2UpdateCollData
#define ftCommonCliffCommon2InitStatusVars \
    ndsBaseFTCommonCliffCommon2InitStatusVars

sb32 ndsBaseFTCommonCliffClimbOrFallCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonCliffCommon2UpdateCollData(GObj *fighter_gobj);
void ndsBaseFTCommonCliffCommon2InitStatusVars(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncliffclimb.c"

#undef ftCommonCliffClimbOrFallCheckInterruptCommon
#undef ftCommonCliffCommon2UpdateCollData
#undef ftCommonCliffCommon2InitStatusVars
