#include <ft/fighter.h>

#define ftCommonCliffEscapeCheckInterruptCommon \
    ndsBaseFTCommonCliffEscapeCheckInterruptCommon

sb32 ndsBaseFTCommonCliffEscapeCheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncliffescape.c"

#undef ftCommonCliffEscapeCheckInterruptCommon
