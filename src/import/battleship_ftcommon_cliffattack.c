#include <ft/fighter.h>

#define ftCommonCliffAttackCheckInterruptCommon \
    ndsBaseFTCommonCliffAttackCheckInterruptCommon

sb32 ndsBaseFTCommonCliffAttackCheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncliffattack.c"

#undef ftCommonCliffAttackCheckInterruptCommon
