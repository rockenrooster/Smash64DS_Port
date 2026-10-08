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
 * interrupt's item-throw test, its one call in the file. A content's
 * down-air hit routine (Crash.asm dair_bounce_) replaces the source's where
 * the aerial start has just set it, before its events (the file's one
 * ftMainPlayAnimEventsAll). */
#define ftCommonLightThrowCheckItemTypeThrow(fp_) ndsP4AirLightThrowCheck(fp_)
#define ftMainPlayAnimEventsAll(g_) (ndsP4AttackAirStart(g_), ftMainPlayAnimEventsAll(g_))
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattackair.c"
#if NDS_P4
#undef ftCommonLightThrowCheckItemTypeThrow
#undef ftMainPlayAnimEventsAll
#endif

#undef ftCommonAttackAirLwProcHit
#undef ftCommonAttackAirLwProcUpdate
#undef ftCommonAttackAirProcMap
#undef ftCommonAttackAirCheckInterruptCommon
