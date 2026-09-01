/* P2-3 Link runtime state machine: BattleShip specials verbatim. */
#include <ef/effect.h>
#include <ft/fighter.h>
#include <it/item.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* Exact US constants from BattleShip ft/ftchar/ftlink/ftlink.h. */
#define FTLINK_BOOMERANG_SPAWN_JOINT nFTPartsJointTopN
#define FTLINK_BOOMERANG_SMASH_BUFFER 8
#define FTLINK_BOOMERANG_SMASH_STICK_MIN 56
#define FTLINK_SPINATTACK_SPAWN_JOINT nFTPartsJointTopN
#define FTLINK_SPINATTACK_EXTEND_POS_COUNT 4
#define FTLINK_SPINATTACK_FLAG_SIZE_1 0.0F
#define FTLINK_SPINATTACK_FLAG_SIZE_2 120.0F
#define FTLINK_SPINATTACK_FLAG_SIZE_3 100.0F
#define FTLINK_SPINATTACK_FLAG_SIZE_4 80.0F
#define FTLINK_SPINATTACK_GRAVITY_MUL 0.23F
#define FTLINK_SPINATTACK_AIR_DRIFT_MUL 0.5F
#define FTLINK_SPINATTACK_AIR_VEL_Y 69.0F
#define FTLINK_SPINATTACK_FALLSPECIAL_DRIFT 0.6F
#define FTLINK_SPINATTACK_LANDING_LAG 0.65F

/* BattleShip ftlinkspecialhi.c documents an original MIPS UB where Link's
 * fighter GObj is passed to wpProcessUpdateHitPositions as though it were a
 * weapon GObj.  The decomp explicitly states the corrected call has no
 * gameplay delta because the weapon performs the same hit-position update
 * later in its own process.  On ARM the FTStruct/WPStruct layout accident is
 * not an ABI contract, so select the source tree's own behavior-preserving
 * correction instead of depending on invalid cross-struct reads. */
#define AVOID_UB 1

GObj *wpLinkBoomerangMakeWeapon(GObj *fighter_gobj, Vec3f *pos);
GObj *wpLinkSpinAttackMakeWeapon(GObj *fighter_gobj, Vec3f *pos);
GObj *itLinkBombMakeItem(GObj *fighter_gobj, Vec3f *pos, Vec3f *vel);
GObj *efManagerLinkSpinAttackMakeEffect(GObj *fighter_gobj);
void ftLinkSpecialNSwitchStatusAir(GObj *fighter_gobj);
void ftLinkSpecialAirNSwitchStatusGround(GObj *fighter_gobj);
void ftLinkSpecialNEmptySwitchStatusAir(GObj *fighter_gobj);
void ftLinkSpecialAirNEmptySwitchStatusGround(GObj *fighter_gobj);
void ftLinkSpecialHiEndSetStatus(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftlink/ftlinkspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftlink/ftlinkspecialhi.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftlink/ftlinkspeciallw.c"

#undef AVOID_UB
