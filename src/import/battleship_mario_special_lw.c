#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW

#include <ft/fighter.h>

#ifndef FTMARIO_TORNADO_VEL_X_GROUND
#define FTMARIO_TORNADO_VEL_X_GROUND 0.025F
#endif
#ifndef FTMARIO_TORNADO_VEL_X_AIR
#define FTMARIO_TORNADO_VEL_X_AIR 0.03F
#endif
#ifndef FTMARIO_TORNADO_VEL_X_CLAMP
#define FTMARIO_TORNADO_VEL_X_CLAMP 17.0F
#endif
#ifndef FTMARIO_TORNADO_VEL_Y_CLAMP
#define FTMARIO_TORNADO_VEL_Y_CLAMP 40.0F
#endif
#ifndef FTMARIO_TORNADO_VEL_Y_BASE
#define FTMARIO_TORNADO_VEL_Y_BASE 15.0F
#endif
#ifndef FTMARIO_TORNADO_VEL_Y_TAP
#define FTMARIO_TORNADO_VEL_Y_TAP 22.0F
#endif

void ftMarioSpecialLwSwitchStatusAir(GObj *fighter_gobj);
void ftMarioSpecialAirLwSwitchStatusGround(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariospeciallw.c"

#endif
