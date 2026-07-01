#include <ft/fighter.h>

#define ftCommonPassiveStandSetStatus ndsBaseFTCommonPassiveStandSetStatus
#define ftCommonPassiveStandCheckInterruptDamage \
    ndsBaseFTCommonPassiveStandCheckInterruptDamage

void ndsBaseFTCommonPassiveStandSetStatus(GObj *fighter_gobj, s32 status_id);
sb32 ndsBaseFTCommonPassiveStandCheckInterruptDamage(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonpassivestand.c"

#undef ftCommonPassiveStandSetStatus
#undef ftCommonPassiveStandCheckInterruptDamage
