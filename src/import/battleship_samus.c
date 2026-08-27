/*
 * P2-3 Samus runtime state machine.
 *
 * Keep BattleShip's Charge Shot, Screw Attack and Bomb status bodies as the
 * behavioral authority.  The DS port adapts only the surrounding ABI/assets;
 * charge persistence/cancel/release and movement ordering remain source code.
 */
#include <ft/fighter.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* The decomp explicitly marks Samus's two-argument Escape call as undefined:
 * ftCommonEscapeSetStatus actually takes a third itemthrow-buffer argument.
 * The surrounding common Escape paths pass 0 for the ordinary roll case, so
 * make that source intent deterministic on ARM instead of inheriting an
 * arbitrary r2 value. The source charge/cancel decision itself is unchanged. */
s32 ndsBaseFTCommonEscapeGetStatus(FTStruct *fp);
void ndsBaseFTCommonEscapeSetStatus(GObj *fighter_gobj, s32 status_id,
                                    s32 itemthrow_buffer_tics);
#define ftCommonEscapeGetStatus ndsBaseFTCommonEscapeGetStatus
#define ftCommonEscapeSetStatus(fighter_gobj, status_id) \
    ndsBaseFTCommonEscapeSetStatus((fighter_gobj), (status_id), 0)

/* Exact US constants from BattleShip ft/ftchar/ftsamus/ftsamus.h. */
#define FTSAMUS_CHARGE_JOINT 16
#define FTSAMUS_CHARGE_EFFECT_JOINT FTSAMUS_CHARGE_JOINT
#define FTSAMUS_CHARGE_MAX 7
#define FTSAMUS_CHARGE_INT 20
#define FTSAMUS_CHARGE_COLANIM_ID 0x6
#define FTSAMUS_CHARGE_COLANIM_LENGTH 0
#define FTSAMUS_CHARGE_OFF_X 180.0F
#define FTSAMUS_CHARGE_RECOIL_BASE 10.0F
#define FTSAMUS_CHARGE_RECOIL_MUL 2.0F
#define FTSAMUS_CHARGE_RECOIL_ADD 20.0F
#define FTSAMUS_SCREWATTACK_PASS_STICK_RANGE_MIN (-44)
#define FTSAMUS_SCREWATTACK_DRIFT_MUL 0.5F
#define FTSAMUS_SCREWATTACK_DRIFT_CLAMP 20.0F
#define FTSAMUS_SCREWATTACK_VEL_X_BASE 10.0F
#define FTSAMUS_SCREWATTACK_VEL_Y_BASE 62.0F
#define FTSAMUS_SCREWATTACK_FALLSPECIAL_DRIFT 0.66F
#define FTSAMUS_SCREWATTACK_LANDING_LAG 0.4F
#define FTSAMUS_BOMB_OFF_Y 60.0F
#define FTSAMUS_BOMB_VEL_Y_BASE 40.0F
#define FTSAMUS_BOMB_VEL_Y_SUB 10.0F
#define FTSAMUS_BOMB_DRIFT 0.66F

/* Source sibling declarations.  The original project gets these through the
 * broad ftsam​usfunctions.h/ftdef.h graph; the port deliberately keeps a
 * single compatible fighter ABI view. */
void ftSamusSpecialNProcDamage(GObj *fighter_gobj);
void ftSamusSpecialNDestroyChargeShot(FTStruct *fp);
void ftSamusSpecialNGetChargeShotPosition(FTStruct *fp, Vec3f *pos);
void ftSamusSpecialNSetChargeShotPosition(FTStruct *fp);
void ftSamusSpecialAirNEndSetStatus(GObj *fighter_gobj);
void ftSamusSpecialNEndSetStatus(GObj *fighter_gobj);
void ftSamusSpecialNLoopSetStatus(GObj *fighter_gobj);
void ftSamusSpecialAirNStartSwitchStatusGround(GObj *fighter_gobj);
void ftSamusSpecialNStartSwitchStatusAir(GObj *fighter_gobj);
void ftSamusSpecialAirNEndSwitchStatusGround(GObj *fighter_gobj);
void ftSamusSpecialNEndSwitchStatusAir(GObj *fighter_gobj);
f32 ftSamusSpecialNStartGetAnimSpeed(FTStruct *fp);
void ftSamusSpecialNStartInitStatusVars(FTStruct *fp);
void ftSamusSpecialLwMakeBomb(GObj *fighter_gobj);
void ftSamusSpecialAirLwSwitchStatusGround(GObj *fighter_gobj);
void ftSamusSpecialLwTransferStatusAir(GObj *fighter_gobj);
void ftSamusSpecialLwSwitchStatusAir(GObj *fighter_gobj);
void ftSamusSpecialLwInitStatusVars(FTStruct *fp);

GObj *wpSamusChargeShotMakeWeapon(GObj *fighter_gobj, Vec3f *pos,
                                  s32 charge_level, sb32 is_release);
GObj *wpSamusBombMakeWeapon(GObj *fighter_gobj, Vec3f *pos);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftsamus/ftsamusspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftsamus/ftsamusspecialhi.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftsamus/ftsamusspeciallw.c"

#undef ftCommonEscapeSetStatus
#undef ftCommonEscapeGetStatus
