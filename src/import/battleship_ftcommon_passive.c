#include <ft/fighter.h>

#define ftCommonPassiveSetStatus ndsBaseFTCommonPassiveSetStatus
#define ftCommonPassiveCheckInterruptDamage \
    ndsBaseFTCommonPassiveCheckInterruptDamage

void ndsBaseFTCommonPassiveSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonPassiveCheckInterruptDamage(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonpassive.c"

#undef ftCommonPassiveSetStatus
#undef ftCommonPassiveCheckInterruptDamage
