/*
 * P2-3 Donkey Kong runtime state machine.
 *
 * Keep BattleShip's function bodies as the behavioral authority.  The DS port
 * adapts the surrounding renderer/reloc/audio seams, but DK's Giant Punch,
 * Spinning Kong, Hand Slap and—most importantly—the cargo carry/throw ladder
 * retain the source callback ordering and shared ftcommon transitions.
 *
 * ftdonkey.c itself is already imported by battleship_ftchar_data_slots.c for
 * gFTDataDonkey* storage, so only the behavior TUs belong here.
 */
#include <ft/fighter.h>

/* Exact US constants from BattleShip ftdonkey.h.  Mirror only the constants
 * these source bodies consume; importing that header would also import the N64
 * ftdef/type universe and conflict with the DS ABI mirrors. */
#define FTDONKEY_CHARGE_EFFECT_JOINT 16
#define FTDONKEY_GIANTPUNCH_CHARGE_MAX 10
#define FTDONKEY_GIANTPUNCH_CHARGE_DAMAGE_MUL 2
#define FTDONKEY_GIANTPUNCH_CHARGE_COLANIM_ID 6
#define FTDONKEY_GIANTPUNCH_CHARGE_COLANIM_LENGTH 0
#define FTDONKEY_GIANTPUNCH_CHRAGE_ANIM_SPEED 2.0F
#define FTDONKEY_GIANTPUNCH_VEL_MUL 8.0F
#define FTDONKEY_SPINNINGKONG_GROUND_ACCEL 0.025F
#define FTDONKEY_SPINNINGKONG_GROUND_VEL_MAX 26.0F
#define FTDONKEY_SPINNINGKONG_AIR_VEL_Y 20.3F
#define FTDONKEY_SPINNINGKONG_AIR_ACCEL 0.05F
#define FTDONKEY_SPINNINGKONG_AIR_VEL_MAX 38.0F
#define FTDONKEY_SPINNINGKONG_START_GRAVITY_MUL 0.07F
#define FTDONKEY_SPINNINGKONG_END_GRAVITY_MUL 1.0F
#define FTDONKEY_SPINNINGKONG_FALLSPECIAL_DRIFT 1.0F
#define FTDONKEY_SPINNINGKONG_LANDING_LAG 0.3F
#define FTCOMMON_THROWFFALL_SKIPLANDING_VEL_Y_MAX (-20.0F)
#define FTCOMMON_THROWFF_TURN_STICK_RANGE_MIN 20
#define FTCOMMON_THROWFF_TURN_FRAMES 6

#define DObjGetStruct(gobj) ((DObj *)(gobj)->obj)
extern void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);
extern sb32 ftCommonEscapeCheckInterruptSpecialNDonkey(GObj *fighter_gobj);
extern sb32 ftCommonHeavyThrowCheckInterruptCommon(GObj *fighter_gobj);
extern sb32 ftCommonWaitCheckInputSuccess(GObj *fighter_gobj);
extern void ftCommonPassSetStatusParam(GObj *fighter_gobj, s32 status_id,
                                       f32 frame_begin, u32 flags);
extern sb32 ftCommonPassCheckInputSuccess(FTStruct *fp);

/* Source ftdonkeyfunctions.h declarations, restated without pulling its
 * ftdef.h dependency.  They let the source TUs call forward into siblings while
 * keeping every function body verbatim. */
void ftDonkeySpecialNProcDamage(GObj *fighter_gobj);
void ftDonkeySpecialNStartProcUpdate(GObj *fighter_gobj);
void ftDonkeySpecialAirNStartProcUpdate(GObj *fighter_gobj);
void ftDonkeySpecialNStartProcInterrupt(GObj *fighter_gobj);
void ftDonkeySpecialNStartProcMap(GObj *fighter_gobj);
void ftDonkeySpecialAirNStartProcMap(GObj *fighter_gobj);
void ftDonkeySpecialAirNStartSwitchStatusGround(GObj *fighter_gobj);
void ftDonkeySpecialNStartSwitchStatusAir(GObj *fighter_gobj);
void ftDonkeySpecialNLoopProcUpdate(GObj *fighter_gobj);
void ftDonkeySpecialNLoopProcInterrupt(GObj *fighter_gobj);
void ftDonkeySpecialNLoopProcMap(GObj *fighter_gobj);
void ftDonkeySpecialAirNLoopProcMap(GObj *fighter_gobj);
void ftDonkeySpecialNLoopSetProcDamageAnimSpeed(GObj *fighter_gobj);
void ftDonkeySpecialAirNLoopSwitchStatusGround(GObj *fighter_gobj);
void ftDonkeySpecialNLoopSwitchStatusAir(GObj *fighter_gobj);
void ftDonkeySpecialNLoopSetStatus(GObj *fighter_gobj);
void ftDonkeySpecialAirNLoopSetStatus(GObj *fighter_gobj);
void ftDonkeySpecialNEndProcUpdate(GObj *fighter_gobj);
void ftDonkeySpecialAirNEndProcMap(GObj *fighter_gobj);
void ftDonkeySpecialAirNEndSwitchStatusGround(GObj *fighter_gobj);
void ftDonkeySpecialNGetStatusChargeLevelReset(GObj *fighter_gobj);
void ftDonkeySpecialNEndSetStatus(GObj *fighter_gobj);
void ftDonkeySpecialAirNEndSetStatus(GObj *fighter_gobj);
void ftDonkeySpecialNInitStatusVars(GObj *fighter_gobj);
void ftDonkeySpecialNStartSetStatus(GObj *fighter_gobj);
void ftDonkeySpecialAirNStartSetStatus(GObj *fighter_gobj);
void ftDonkeySpecialHiProcUpdate(GObj *fighter_gobj);
void ftDonkeySpecialAirHiProcUpdate(GObj *fighter_gobj);
void ftDonkeySpecialHiProcPhysics(GObj *fighter_gobj);
void ftDonkeySpecialAirHiProcPhysics(GObj *fighter_gobj);
void ftDonkeySpecialHiProcMap(GObj *fighter_gobj);
void ftDonkeySpecialAirHiProcMap(GObj *fighter_gobj);
void ftDonkeySpecialAirHiSwitchStatusGround(GObj *fighter_gobj);
void ftDonkeySpecialHiSwitchStatusAir(GObj *fighter_gobj);
void ftDonkeySpecialHiSetStatusFlagGA(GObj *fighter_gobj, sb32 ga);
void ftDonkeySpecialHiSetStatus(GObj *fighter_gobj);
void ftDonkeySpecialAirHiSetStatus(GObj *fighter_gobj);
void ftDonkeySpecialLwStartProcUpdate(GObj *fighter_gobj);
void ftDonkeySpecialLwLoopProcUpdate(GObj *fighter_gobj);
void ftDonkeySpecialLwLoopProcInterrupt(GObj *fighter_gobj);
void ftDonkeySpecialLwLoopSetStatus(GObj *fighter_gobj);
void ftDonkeySpecialLwEndSetStatus(GObj *fighter_gobj);
void ftDonkeySpecialLwStartSetStatus(GObj *fighter_gobj);
void ftDonkeyThrowFWaitProcInterrupt(GObj *fighter_gobj);
void ftDonkeyThrowFCommonProcMap(GObj *fighter_gobj);
void ftDonkeyThrowFWaitSetStatus(GObj *fighter_gobj);
sb32 ftDonkeyThrowFWaitCheckInterruptThrowFWalk(GObj *fighter_gobj);
f32 ftDonkeyThrowFWalkGetWalkAnimLength(FTStruct *fp, s32 status_id);
void ftDonkeyThrowFWalkProcInterrupt(GObj *fighter_gobj);
void ftDonkeyThrowFWalkSetStatusParam(GObj *fighter_gobj, f32 frame_begin);
void ftDonkeyThrowFWalkSetStatusDefault(GObj *fighter_gobj);
sb32 ftDonkeyThrowFWalkCheckInterruptThrowFWait(GObj *fighter_gobj);
void ftDonkeyThrowFTurnProcUpdate(GObj *fighter_gobj);
void ftDonkeyThrowFTurnProcInterrupt(GObj *fighter_gobj);
void ftDonkeyThrowFTurnSetStatus(GObj *fighter_gobj);
sb32 ftDonkeyThrowFTurnCheckInterruptThrowFCommon(GObj *fighter_gobj);
void ftDonkeyThrowFKneeBendProcUpdate(GObj *fighter_gobj);
void ftDonkeyThrowFKneeBendProcInterrupt(GObj *fighter_gobj);
void ftDonkeyThrowFKneeBendSetStatus(GObj *fighter_gobj, s32 input_source);
sb32 ftDonkeyThrowFKneeBendCheckInterruptThrowFCommon(GObj *fighter_gobj);
void ftDonkeyThrowFFallProcInterrupt(GObj *fighter_gobj);
void ftDonkeyThrowFFallProcMap(GObj *fighter_gobj);
void ftDonkeyThrowFFallSetStatus(GObj *fighter_gobj);
void ftDonkeyThrowFJumpSetStatus(GObj *fighter_gobj);
void ftDonkeyThrowFFallSetStatusPass(GObj *fighter_gobj);
sb32 ftDonkeyThrowFFallCheckInterruptPass(GObj *fighter_gobj);
void ftDonkeyThrowFLandingProcUpdate(GObj *fighter_gobj);
void ftDonkeyThrowFLandingSetStatus(GObj *fighter_gobj);
void ftDonkeyThrowFDamageProcUpdate(GObj *fighter_gobj);
void ftDonkeyThrowFDamageSetStatus(GObj *fighter_gobj);
void ftDonkeyThrowFFProcUpdate(GObj *fighter_gobj);
void ftDonkeyThrowAirFFSwitchStatusGround(GObj *fighter_gobj);
void ftDonkeyThrowFFSwitchStatusAir(GObj *fighter_gobj);
void ftDonkeyThrowFFProcMap(GObj *fighter_gobj);
void ftDonkeyThrowAirFFProcMap(GObj *fighter_gobj);
void ftDonkeyThrowFFSetStatus(GObj *fighter_gobj, sb32 is_turn);
sb32 ftDonkeyThrowFFCheckInterruptThrowFCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeyspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeyspecialhi.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeyspeciallw.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeythrowfwait.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeythrowfwalk.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeythrowfturn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeythrowfkneebend.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeythrowffall.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeythrowflanding.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeythrowfdamage.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeythrowff.c"
