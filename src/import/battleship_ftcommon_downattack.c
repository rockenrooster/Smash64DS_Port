#include <ft/fighter.h>

#define ftCommonDownAttackSetStatus ndsBaseFTCommonDownAttackSetStatus
#define ftCommonDownAttackCheckInterruptDownBounce \
    ndsBaseFTCommonDownAttackCheckInterruptDownBounce
#define ftCommonDownAttackCheckInterruptDownWait \
    ndsBaseFTCommonDownAttackCheckInterruptDownWait

void ndsBaseFTCommonDownAttackSetStatus(GObj *fighter_gobj, s32 status_id);
sb32 ndsBaseFTCommonDownAttackCheckInterruptDownBounce(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDownAttackCheckInterruptDownWait(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondownattack.c"

#undef ftCommonDownAttackSetStatus
#undef ftCommonDownAttackCheckInterruptDownBounce
#undef ftCommonDownAttackCheckInterruptDownWait
