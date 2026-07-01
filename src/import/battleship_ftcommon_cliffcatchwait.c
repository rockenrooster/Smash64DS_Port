#include <ef/effect.h>
#include <ft/fighter.h>

#define ftCommonCliffCatchSetStatus ndsBaseFTCommonCliffCatchSetStatus

void ndsBaseFTCommonCliffCatchSetStatus(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncliffcatchwait.c"

#undef ftCommonCliffCatchSetStatus
