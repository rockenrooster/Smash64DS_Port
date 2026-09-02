/* P2-3 Pikachu runtime state machine: BattleShip specials verbatim.
 *
 * Thunder Jolt, Thunder and Quick Attack keep their source status bodies as
 * the behavioral authority. The DS port adapts only the surrounding ABI and
 * assets: zip angle selection, the two-segment pass rules, Thunder's cloud
 * spawn/self-hit interplay and every transition remain source code. */
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* Exact US constants from BattleShip ft/ftchar/ftpikachu/ftpikachu.h. */
#define FTPIKACHU_THUNDERJOLT_SPAWN_JOINT 11
#define FTPIKACHU_THUNDERJOLT_SPAWN_ANGLE F_CLC_DTOR32(-45.0F)
#define FTPIKACHU_THUNDERJOLTVEL 40.0F
#define FTPIKACHU_THUNDERJOLT_COLANIM_ID 0x3B
#define FTPIKACHU_THUNDERJOLT_COLANIM_LENGTH 0

#define FTPIKACHU_QUICKATTACK_BASE_JOINT 4
#define FTPIKACHU_QUICKATTACK_COLANIM_ID 0x39
#define FTPIKACHU_QUICKATTACK_COLANIM_LENGTH 0
#define FTPIKACHU_QUICKATTACK_SCALE_X 0.8F
#define FTPIKACHU_QUICKATTACK_SCALE_Y 0.8F
#define FTPIKACHU_QUICKATTACK_SCALE_Z 1.2F
#define FTPIKACHU_QUICKATTACK_HALT_ANGLE F_CLC_DTOR32(135.0F)
#define FTPIKACHU_QUICKATTACK_START_TIME 20
#define FTPIKACHU_QUICKATTACK_ZIP_TIME 5
#define FTPIKACHU_QUICKATTACK_PASS_BUFFER_MAX 2
#define FTPIKACHU_QUICKATTACK_STICK_RANGE_MIN 60.0F
#define FTPIKACHU_QUICKATTACK_ANGLE_DIFF_MIN F_CLC_DTOR32(42.0F)
#define FTPIKACHU_QUICKATTACK_VEL_BASE 3.0F
#define FTPIKACHU_QUICKATTACK_VEL_ADD 90.0F
#define FTPIKACHU_QUICKATTACK_VEL_MUL 0.9F
#define FTPIKACHU_QUICKATTACK_AIR_ACCEL_MUL 0.5F
#define FTPIKACHU_QUICKATTACK_AIR_SPEED_MUL 0.5F
#define FTPIKACHU_QUICKATTACK_VEL_Y_DIV 9.0F
#define FTPIKACHU_QUICKATTACK_VEL_BAK_MUL 0.2F
#define FTPIKACHU_QUICKATTACK_FALLSPECIAL_DRIFT 0.4F
#define FTPIKACHU_QUICKATTACK_LANDING_LAG 0.4F

#define FTPIKACHU_THUNDER_SPAWN_JOINT 11
#define FTPIKACHU_THUNDER_SPAWN_OFF_Y 500.0F
#define FTPIKACHU_THUNDER_VEL_Y -450.0F
#define FTPIKACHU_THUNDER_COLL_OFF_Y 225.0F
#define FTPIKACHU_THUNDER_COLLIDE_X 200.0F
#define FTPIKACHU_THUNDER_COLLIDE_Y 800.0F
#define FTPIKACHU_THUNDER_HIT_GRAVITY 0.5F
#define FTPIKACHU_THUNDER_HITVEL_Y 20.0F

/* Source sibling declarations. The original project gets these through the
 * broad ftpikachufunctions.h/ftdef.h graph; the port deliberately keeps a
 * single compatible fighter ABI view, so the three included bodies see one
 * another here. */
void ftPikachuSpecialNProcAccessory(GObj *fighter_gobj);
void ftPikachuSpecialNProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirNProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirNSwitchStatusGround(GObj *fighter_gobj);
void ftPikachuSpecialNSwitchStatusAir(GObj *fighter_gobj);
void ftPikachuSpecialNInitStatusVars(GObj *fighter_gobj);
void ftPikachuSpecialNSetStatus(GObj *fighter_gobj);
void ftPikachuSpecialAirNSetStatus(GObj *fighter_gobj);

void ftPikachuSpecialLwMakeThunder(GObj *fighter_gobj);
void ftPikachuSpecialLwStartUpdateThunder(GObj *fighter_gobj);
void ftPikachuSpecialLwStartProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialAirLwStartProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialLwStartProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirLwStartProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirLwStartSwitchStatusGround(GObj *fighter_gobj);
void ftPikachuSpecialLwStartSwitchStatusAir(GObj *fighter_gobj);
void ftPikachuSpecialLwStartInitStatusVars(GObj *fighter_gobj);
void ftPikachuSpecialLwStartSetStatus(GObj *fighter_gobj);
void ftPikachuSpecialAirLwStartSetStatus(GObj *fighter_gobj);
sb32 ftPikachuSpecialLwCheckCollideThunder(GObj *fighter_gobj);
void ftPikachuSpecialLwLoopProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialAirLwLoopProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialLwLoopProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirLwLoopProcMap(GObj *fighter_gobj);
void ftPikachuSpecialLwProcDamage(GObj *fighter_gobj);
void ftPikachuSpecialAirLwLoopSwitchStatusGround(GObj *fighter_gobj);
void ftPikachuSpecialLwLoopSwitchStatusAir(GObj *fighter_gobj);
void ftPikachuSpecialLwLoopUpdateThunder(GObj *fighter_gobj);
void ftPikachuSpecialLwLoopSetStatus(GObj *fighter_gobj);
void ftPikachuSpecialAirLwLoopSetStatus(GObj *fighter_gobj);
void ftPikachuSpecialLwHitProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialAirLwHitProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialAirLwHitProcPhysics(GObj *fighter_gobj);
void ftPikachuSpecialLwHitProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirLwHitProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirLwHitSwitchStatusGround(GObj *fighter_gobj);
void ftPikachuSpecialLwHitSwitchStatusAir(GObj *fighter_gobj);
void ftPikachuSpecialLwHitInitStatusVars(GObj *fighter_gobj);
void ftPikachuSpecialLwHitSetStatus(GObj *fighter_gobj);
void ftPikachuSpecialAirLwHitSetStatus(GObj *fighter_gobj);
void ftPikachuSpecialAirLwEndProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialLwEndProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirLwEndProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirLwEndSwitchStatusGround(GObj *fighter_gobj);
void ftPikachuSpecialLwEndSwitchStatusAir(GObj *fighter_gobj);
void ftPikachuSpecialLwClearProcDamage(GObj *fighter_gobj);
void ftPikachuSpecialLwEndSetStatus(GObj *fighter_gobj);
void ftPikachuSpecialAirLwEndSetStatus(GObj *fighter_gobj);

void ftPikachuSpecialHiStartProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialAirHiStartProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialAirHiStartProcPhysics(GObj *fighter_gobj);
void ftPikachuSpecialHiStartProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirHiStartProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirHiStartSwitchStatusGround(GObj *fighter_gobj);
void ftPikachuSpecialHiStartSwitchStatusAir(GObj *fighter_gobj);
void ftPikachuSpecialHiStartInitStatusVars(GObj *fighter_gobj);
void ftPikachuSpecialHiInitMiscVars(GObj *fighter_gobj);
void ftPikachuSpecialHiStartSetStatus(GObj *fighter_gobj);
void ftPikachuSpecialAirHiStartSetStatus(GObj *fighter_gobj);
void ftPikachuSpecialHiProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialAirHiProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialHiUpdateModelPitchScale(GObj *fighter_gobj);
void ftPikachuSpecialHiProcPhysics(GObj *fighter_gobj);
void ftPikachuSpecialAirHiProcPhysics(GObj *fighter_gobj);
void ftPikachuSpecialHiProcMap(GObj *fighter_gobj);
sb32 ftPikachuSpecialHiProcPass(GObj *fighter_gobj);
void ftPikachuSpecialAirHiProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirHiSwitchStatusGround(GObj *fighter_gobj);
void ftPikachuSpecialHiSwitchStatusAir(GObj *fighter_gobj);
void ftPikachuSpecialHiInitStatusVarsZip(GObj *fighter_gobj);
void ftPikachuSpecialHiSetStatus(GObj *fighter_gobj);
void ftPikachuSpecialAirHiSetStatus(GObj *fighter_gobj);
sb32 ftPikachuSpecialHiCheckGotoSubZip(GObj *fighter_gobj);
void ftPikachuSpecialHiEndProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialAirHiEndProcUpdate(GObj *fighter_gobj);
void ftPikachuSpecialHiEndProcPhysics(GObj *fighter_gobj);
void ftPikachuSpecialAirHiEndProcPhysics(GObj *fighter_gobj);
void ftPikachuSpecialHiEndProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirHiEndProcMap(GObj *fighter_gobj);
void ftPikachuSpecialAirHiEndSwitchStatusGround(GObj *fighter_gobj);
void ftPikachuSpecialHiEndSwitchStatusAir(GObj *fighter_gobj);
void ftPikachuSpecialHiEndBackupVel(GObj *fighter_gobj);
void ftPikachuSpecialHiEndSetStatus(GObj *fighter_gobj);
void ftPikachuSpecialAirHiEndSetStatus(GObj *fighter_gobj);

/* Article and helper declarations normally supplied by BattleShip's broad
 * wp/sys/ef include graph; the implementations live in the shared owners and
 * battleship_pikachu_weapons.c. */
GObj *wpPikachuThunderJoltAirMakeWeapon(GObj *fighter_gobj, Vec3f *pos,
                                        Vec3f *vel);
GObj *wpPikachuThunderJoltGroundMakeWeapon(GObj *prev_gobj, Vec3f *pos,
                                           s32 coll_type);
GObj *wpPikachuThunderHeadMakeWeapon(GObj *fighter_gobj, Vec3f *pos,
                                     Vec3f *vel);
void wpPikachuThunderHeadSetDestroy(GObj *weapon_gobj, sb32 is_destroy);
GObj *efManagerQuakeMakeEffect(s32 magnitude);
LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos);
f32 syUtilsArcTan2(f32 y, f32 x);
s32 syUtilsRandIntRange(s32 range);
f32 syVectorAngleDiff3D(Vec3f *a, Vec3f *b);
Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);
void gcSetAllAnimSpeed(GObj *gobj, f32 anim_speed);
void gcPlayAnimAll(GObj *gobj);
void ftPhysicsApplyGroundVelTransferAir(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftpikachu/ftpikachuspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftpikachu/ftpikachuspecialhi.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftpikachu/ftpikachuspeciallw.c"
