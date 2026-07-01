/*
 * Bounded BattleShip ftcommonattackdash.c import for the NDS Mario/Fox
 * Dash -> Run proof. The public symbols are remapped so the port backend can
 * guard the original A-tap branch and keep item attack branches deferred.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <it/item.h>
#include <sys/obj.h>

#define ftCommonAttackDashSetStatus ndsBaseFTCommonAttackDashSetStatus
#define ftCommonAttackDashCheckInterruptCommon \
    ndsBaseFTCommonAttackDashCheckInterruptCommon

void ndsBaseFTCommonAttackDashSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackDashCheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattackdash.c"

#undef ftCommonAttackDashSetStatus
#undef ftCommonAttackDashCheckInterruptCommon
