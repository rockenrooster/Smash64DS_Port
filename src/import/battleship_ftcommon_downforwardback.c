#include <ft/fighter.h>

#define ftCommonDownForwardOrBackSetStatus \
    ndsBaseFTCommonDownForwardOrBackSetStatus
#define ftCommonDownForwardOrBackCheckInterruptCommon \
    ndsBaseFTCommonDownForwardOrBackCheckInterruptCommon

void ndsBaseFTCommonDownForwardOrBackSetStatus(GObj *fighter_gobj,
                                               s32 status_id);
sb32 ndsBaseFTCommonDownForwardOrBackCheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondownforwardback.c"

#undef ftCommonDownForwardOrBackSetStatus
#undef ftCommonDownForwardOrBackCheckInterruptCommon
