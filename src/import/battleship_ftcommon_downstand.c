#include <ft/fighter.h>

#define ftCommonDownStandProcInterrupt \
    ndsBaseFTCommonDownStandProcInterrupt
#define ftCommonDownStandSetStatus ndsBaseFTCommonDownStandSetStatus
#define ftCommonDownStandCheckInterruptCommon \
    ndsBaseFTCommonDownStandCheckInterruptCommon

void ndsBaseFTCommonDownStandProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDownStandSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDownStandCheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondownstand.c"

#undef ftCommonDownStandProcInterrupt
#undef ftCommonDownStandSetStatus
#undef ftCommonDownStandCheckInterruptCommon
