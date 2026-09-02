/* P2-3 Yoshi runtime state machine: BattleShip specials verbatim.
 *
 * Egg Lay (the grabber half of a two-body capture), Egg Throw and Yoshi Bomb
 * keep their source status bodies as the behavioral authority. The DS port
 * adapts only the surrounding ABI and assets: the catch-kind mask, the egg
 * article's throw-force/stick handoff, the ground-pound clamps and every
 * transition remain source code. The victim half of Egg Lay is
 * battleship_ftcommon_captureyoshi.c. */
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* Exact US constants from BattleShip ft/ftchar/ftyoshi/ftyoshi.h.
 * FTYOSHI_JUMPAERIAL_KNOCKBACK_RESIST already lives in the port fighter header
 * because the shared jump-aerial seam consumes it. */
#define FTYOSHI_EGGTHROW_JOINT nFTPartsJointYRotN
#define FTYOSHI_YOSHIBOMB_STAR_SPAWN_JOINT 0
#define FTYOSHI_YOSHIBOMB_VEL_X_CLAMP 30.0F
#define FTYOSHI_YOSHIBOMB_VEL_Y_CLAMP -150.0F

/* Source sibling declarations. The original project gets these through the
 * broad ftyoshifunctions.h/ftdef.h graph; the port deliberately keeps a
 * single compatible fighter ABI view, so the three included bodies see one
 * another here. */
void ftYoshiSpecialNSetCatchParams(FTStruct *fp, void (*proc_catch)(GObj *));
void ftYoshiSpecialNCatchUpdateProcStatus(GObj *fighter_gobj,
                                          void (*proc_status)(GObj *));
void ftYoshiSpecialNCatchProcUpdate(GObj *fighter_gobj);
void ftYoshiSpecialAirNCatchProcUpdate(GObj *fighter_gobj);
void ftYoshiSpecialNCatchUpdateCaptureVars(FTStruct *fp);
void ftYoshiSpecialNReleaseProcUpdate(GObj *fighter_gobj);
void ftYoshiSpecialAirNReleaseProcUpdate(GObj *fighter_gobj);
void ftYoshiSpecialNProcMap(GObj *fighter_gobj);
void ftYoshiSpecialAirNProcMap(GObj *fighter_gobj);
void ftYoshiSpecialNCatchProcMap(GObj *fighter_gobj);
void ftYoshiSpecialAirNCatchProcMap(GObj *fighter_gobj);
void ftYoshiSpecialNReleaseProcMap(GObj *fighter_gobj);
void ftYoshiSpecialAirNReleaseProcMap(GObj *fighter_gobj);
void ftYoshiSpecialNProcStatus(GObj *fighter_gobj);
void ftYoshiSpecialAirNProcStatus(GObj *fighter_gobj);
void ftYoshiSpecialAirNSwitchStatusGround(GObj *fighter_gobj);
void ftYoshiSpecialNSwitchStatusAir(GObj *fighter_gobj);
void ftYoshiSpecialAirNCatchSwitchStatusGround(GObj *fighter_gobj);
void ftYoshiSpecialNCatchSwitchStatusAir(GObj *fighter_gobj);
void ftYoshiSpecialAirNReleaseSwitchStatusGround(GObj *fighter_gobj);
void ftYoshiSpecialNReleaseSwitchStatusAir(GObj *fighter_gobj);
void ftYoshiSpecialNSetStatus(GObj *fighter_gobj);
void ftYoshiSpecialAirNSetStatus(GObj *fighter_gobj);
void ftYoshiSpecialNCatchInitStatusVars(GObj *fighter_gobj);
void ftYoshiSpecialNCatchProcCatch(GObj *fighter_gobj);
void ftYoshiSpecialAirNCatchProcCatch(GObj *fighter_gobj);
void ftYoshiSpecialNReleaseInitStatusVars(GObj *fighter_gobj);
void ftYoshiSpecialNReleaseSetStatus(GObj *fighter_gobj);
void ftYoshiSpecialAirNReleaseSetStatus(GObj *fighter_gobj);

void ftYoshiSpecialHiProcDamage(GObj *fighter_gobj);
void ftYoshiSpecialHiGetEggPosition(FTStruct *fp, Vec3f *pos);
void ftYoshiSpecialHiUpdateEggVectors(FTStruct *fp);
void ftYoshiSpecialHiUpdateEggVars(GObj *fighter_gobj);
void ftYoshiSpecialHiUpdateEggThrowForce(GObj *fighter_gobj);
void ftYoshiSpecialHiProcUpdate(GObj *fighter_gobj);
void ftYoshiSpecialAirHiProcUpdate(GObj *fighter_gobj);
void ftYoshiSpecialHiProcPhysics(GObj *fighter_gobj);
void ftYoshiSpecialAirHiProcPhysics(GObj *fighter_gobj);
void ftYoshiSpecialAirHiSwitchStatusGround(GObj *fighter_gobj);
void ftYoshiSpecialHiSwitchStatusAir(GObj *fighter_gobj);
void ftYoshiSpecialHiProcMap(GObj *fighter_gobj);
void ftYoshiSpecialAirHiProcMap(GObj *fighter_gobj);
void ftYoshiSpecialHiInitStatusVars(GObj *fighter_gobj);
void ftYoshiSpecialHiSetStatus(GObj *fighter_gobj);
void ftYoshiSpecialAirHiSetStatus(GObj *fighter_gobj);

void ftYoshiSpecialLwStartProcUpdate(GObj *fighter_gobj);
void ftYoshiSpecialLwLandingProcUpdate(GObj *fighter_gobj);
void ftYoshiSpecialAirLwLoopProcPhysics(GObj *fighter_gobj);
void ftYoshiSpecialLwStartProcMap(GObj *fighter_gobj);
void ftYoshiSpecialAirLwLoopProcMap(GObj *fighter_gobj);
void ftYoshiSpecialLwStartProcStatus(GObj *fighter_gobj);
void ftYoshiSpecialLwStartSetStatus(GObj *fighter_gobj);
void ftYoshiSpecialAirLwStartSetStatus(GObj *fighter_gobj);
void ftYoshiSpecialAirLwLoopSetStatus(GObj *fighter_gobj);

/* Article, victim-side and helper declarations normally supplied by
 * BattleShip's broad wp/mp/ft include graph; the implementations live in the
 * shared owners, battleship_yoshi_weapons.c and
 * battleship_ftcommon_captureyoshi.c. */
GObj *wpYoshiEggThrowMakeWeapon(GObj *fighter_gobj, Vec3f *pos);
GObj *wpYoshiStarMakeStars(GObj *fighter_gobj, Vec3f *pos);
void ftCommonCaptureYoshiProcCapture(GObj *fighter_gobj, GObj *capture_gobj);
/* mpcommon.c:665, the ceiling-heavy check without the cliff arm; the port's
 * compat shims carry both beside each other. */
sb32 mpCommonCheckFighterCeilHeavy(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftyoshi/ftyoshispecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftyoshi/ftyoshispecialhi.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftyoshi/ftyoshispeciallw.c"
