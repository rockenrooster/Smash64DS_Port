/*
 * Bounded BattleShip ftcommonattackair.c import for the NDS Mario/Fox
 * JumpF -> AttackAir proof. Public symbols are remapped so the port backend
 * can guard the original aerial-attack interrupt path.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <it/item.h>
#include <macros.h>
#include <sys/obj.h>

#define ftCommonAttackAirLwProcHit ndsBaseFTCommonAttackAirLwProcHit
#define ftCommonAttackAirLwProcUpdate ndsBaseFTCommonAttackAirLwProcUpdate
#define ftCommonAttackAirProcMap ndsBaseFTCommonAttackAirProcMap
#define ftCommonAttackAirCheckInterruptCommon \
    ndsBaseFTCommonAttackAirCheckInterruptCommon

void ndsBaseFTCommonAttackAirLwProcHit(GObj *fighter_gobj);
void ndsBaseFTCommonAttackAirLwProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttackAirProcMap(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackAirCheckInterruptCommon(GObj *fighter_gobj);

#if NDS_P4
#include <nds/nds_p4.h>
/* P4: Remix's prevent_item_throw_ (PeachSpecial.asm) answers the aerial
 * interrupt's item-throw test, its one call in the file. */
#define ftCommonLightThrowCheckItemTypeThrow(fp_) ndsP4AirLightThrowCheck(fp_)
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattackair.c"
#if NDS_P4
#undef ftCommonLightThrowCheckItemTypeThrow
#endif

#undef ftCommonAttackAirLwProcHit
#undef ftCommonAttackAirLwProcUpdate
#undef ftCommonAttackAirProcMap
#undef ftCommonAttackAirCheckInterruptCommon
