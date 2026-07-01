#include <ft/fighter.h>

#define ftCommonDamageFallProcInterrupt ndsBaseFTCommonDamageFallProcInterrupt
#define ftCommonDamageFallProcMap ndsBaseFTCommonDamageFallProcMap
#define ftCommonDamageFallClampRumble ndsBaseFTCommonDamageFallClampRumble
#define ftCommonDamageFallSetStatusFromDamage \
    ndsBaseFTCommonDamageFallSetStatusFromDamage
#define ftCommonDamageFallSetStatusFromCliffWait \
    ndsBaseFTCommonDamageFallSetStatusFromCliffWait
#define func_ovl3_801436F0 ndsBaseFuncOvl3_801436F0

void ndsBaseFTCommonDamageFallProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallClampRumble(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallSetStatusFromDamage(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallSetStatusFromCliffWait(GObj *fighter_gobj);
void ndsBaseFuncOvl3_801436F0(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondamagefall.c"

#undef ftCommonDamageFallProcInterrupt
#undef ftCommonDamageFallProcMap
#undef ftCommonDamageFallClampRumble
#undef ftCommonDamageFallSetStatusFromDamage
#undef ftCommonDamageFallSetStatusFromCliffWait
#undef func_ovl3_801436F0
