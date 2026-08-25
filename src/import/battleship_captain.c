/*
 * P2-3 Captain Falcon runtime state machine.
 *
 * Keep BattleShip's function bodies as the behavioral authority.  Falcon Punch,
 * Falcon Kick and Falcon Dive all read and write the same FTStruct the shared
 * ftcommon ladders do, so the source callback ordering is the specification;
 * the DS port adapts only the surrounding renderer/reloc/audio seams.
 *
 * Falcon Dive's VICTIM side is a separate translation unit
 * (battleship_ftcommon_capturecaptain.c) because the source puts it in
 * ft/ftcommon, not in ft/ftchar/ftcaptain -- it is a common capture status that
 * every fighter can end up in.
 *
 * ftcaptain.c itself is already imported by battleship_ftchar_data_slots.c for
 * gFTDataCaptain* storage, so only the behavior TUs belong here.
 */
#include <ft/fighter.h>
#include <macros.h>

/* Exact US constants from BattleShip ftcaptain.h.  Mirror only the constants
 * these source bodies consume; importing that header would also import the N64
 * ftdef/type universe and conflict with the DS ABI mirrors.
 *
 * REGION_US arm for the two Falcon Dive drift multipliers (the JP build uses
 * 1.2F / 0.84F).
 *
 * FTCAPTAIN_FALCONPUNCH_VEL_MUL IS `0.92` -- a DOUBLE -- in the source header,
 * and `vel_air.y *= 0.92` therefore compiles to f32->f64, an aeabi double
 * multiply, and f64->f32 on every aerial Falcon Punch tick.  ARM9 has no
 * hardware double, and this build is Thumb, so that is three soft-float library
 * calls per axis per tick to buy sub-ulp agreement with an N64 constant that
 * was itself only ever applied to an f32.  Restated as `0.92F`.
 */
#define FTCAPTAIN_FALCONPUNCH_VEL_BASE 65.0F
#define FTCAPTAIN_FALCONPUNCH_VEL_MUL 0.92F

#define FTCAPTAIN_FALCONDIVE_UNK_TIMER 15
#define FTCAPTAIN_FALCONDIVE_TURN_STICK_RANGE_MIN 18
#define FTCAPTAIN_FALCONDIVE_FALLSPECIAL_DRIFT 0.72F
#define FTCAPTAIN_FALCONDIVE_AIR_ACCEL_MUL 1.1F
#define FTCAPTAIN_FALCONDIVE_AIR_SPEED_MAX_MUL 0.8F
#define FTCAPTAIN_FALCONDIVE_LANDING_LAG 0.65F

#define FTCAPTAIN_FALCONKICK_VEL_SCALE_APPLY_TIME 6
#define FTCAPTAIN_FALCONKICK_VEL_SCALE_DIV 2.0F

/* BattleShip ftcommon.h:314-315.  Falcon Dive's grabber writes the victim's
 * capture flags directly, so the two masks the victim TU owns are needed here
 * as well; they are restated identically in
 * battleship_ftcommon_capturecaptain.c, which is where they are read. */
#define FTCOMMON_CAPTURECAPTAIN_MASK_THROW (1 << 1)
#define FTCOMMON_CAPTURECAPTAIN_MASK_NOUPDATE (1 << 2)

#define DObjGetStruct(gobj) ((DObj *)(gobj)->obj)

/* decomp ftparam.h:67,69 and mpcommon.c:692.  ftstatus_callbacks.h declares the
 * map proc but is a status-table seam, not something a fighter TU includes. */
extern void ftParamProcPauseEffect(GObj *effect_gobj);
extern void ftParamProcResumeEffect(GObj *fighter_gobj);
extern void mpCommonProcFighterProject(GObj *fighter_gobj);

/* decomp src/sys/vector.h.  vector.c is compiled straight out of the overlay
 * (Makefile BATTLESHIP_SYS), so these are the real bodies; declare them here
 * rather than including the decomp header and its type universe. */
extern f32 syVectorNorm3D(Vec3f *dst);
extern f32 syVectorMag3D(Vec3f *src);
extern Vec3f *syVectorSub3D(Vec3f *dst, Vec3f *sub);
extern Vec3f *syVectorScale3D(Vec3f *dst, f32 scale);

extern f32 __sinf(f32 x);
extern f32 __cosf(f32 x);
extern f32 syUtilsArcTan2(f32 x, f32 y);

extern GObj *efManagerCaptainFalconPunchMakeEffect(GObj *fighter_gobj);
extern GObj *efManagerCaptainFalconKickMakeEffect(GObj *fighter_gobj);
extern GObj *efManagerQuakeMakeEffect(s32 kind);

/* Falcon Dive's victim side, battleship_ftcommon_capturecaptain.c. */
void ftCommonCaptureCaptainUpdatePositions(GObj *fighter_gobj,
                                           GObj *capture_gobj, Vec3f *pos);
void ftCommonCaptureCaptainProcCapture(GObj *fighter_gobj,
                                       GObj *capture_gobj);

/* Source ftcaptainfunctions.h declarations, restated without pulling its
 * ftdef.h dependency.  They let the source TUs call forward into siblings while
 * keeping every function body verbatim. */
void ftCaptainSpecialNUpdateEffect(GObj *fighter_gobj);
f32 ftCaptainSpecialNGetAngle(s32 stick_y);
void ftCaptainSpecialNProcPhysics(GObj *fighter_gobj);
void ftCaptainSpecialAirNProcPhysics(GObj *fighter_gobj);
void ftCaptainSpecialAirNSwitchStatusGround(GObj *fighter_gobj);
void ftCaptainSpecialNSwitchStatusAir(GObj *fighter_gobj);
void ftCaptainSpecialNProcMap(GObj *fighter_gobj);
void ftCaptainSpecialAirNProcMap(GObj *fighter_gobj);
void ftCaptainSpecialNInitStatusVars(GObj *fighter_gobj);
void func_ovl3_8015FB54(void);
void ftCaptainSpecialNSetStatus(GObj *fighter_gobj);
void ftCaptainSpecialAirNSetStatus(GObj *fighter_gobj);

void ftCaptainSpecialLwUpdateEffect(GObj *fighter_gobj);
void ftCaptainSpecialLwSetAir(GObj *fighter_gobj);
void ftCaptainSpecialLwSetGround(GObj *fighter_gobj);
void ftCaptainSpecialLwDecideMapCollide(GObj *fighter_gobj);
void ftCaptainSpecialLwDecideSetEndStatus(GObj *fighter_gobj);
void ftCaptainSpecialLwProcUpdate(GObj *fighter_gobj);
void ftCaptainSpecialLwProcPhysics(GObj *fighter_gobj);
void ftCaptainSpecialLwLandingProcPhysics(GObj *fighter_gobj);
void ftCaptainSpecialAirLwProcPhysics(GObj *fighter_gobj);
void ftCaptainSpecialLwBoundProcPhysics(GObj *fighter_gobj);
sb32 ftCaptainSpecialLwBoundCheckGoto(GObj *fighter_gobj);
sb32 ftCaptainSpecialLwAirCheckAirGoto(GObj *fighter_gobj);
void ftCaptainSpecialLwProcMap(GObj *fighter_gobj);
void ftCaptainSpecialLwAirProcMap(GObj *fighter_gobj);
void ftCaptainSpecialAirLwProcMap(GObj *fighter_gobj);
void ftCaptainSpecialLwProcHit(GObj *fighter_gobj);
void ftCaptainSpecialLwProcStatus(GObj *fighter_gobj);
void ftCaptainSpecialLwAirSetStatus(GObj *fighter_gobj);
void ftCaptainSpecialLwLandingSetStatus(GObj *fighter_gobj);
void ftCaptainSpecialLwSetStatus(GObj *fighter_gobj);
void jtgt_ovl3_801601A0(GObj *fighter_gobj);
void ftCaptainSpecialAirLwSetStatus(GObj *fighter_gobj);

void ftCaptainSpecialHiSetCatchParams(FTStruct *fp);
void ftCaptainSpecialHiProcUpdate(GObj *fighter_gobj);
void ftCaptainSpecialHiCatchProcUpdate(GObj *fighter_gobj);
void ftCaptainSpecialHiProcInterrupt(GObj *fighter_gobj);
void ftCaptainSpecialHiProcPhysics(GObj *fighter_gobj);
void ftCaptainSpecialHiCatchProcPhysics(GObj *fighter_gobj);
void ftCaptainSpecialHiProcMap(GObj *fighter_gobj);
void ftCaptainSpecialHiProcStatus(GObj *fighter_gobj);
void ftCaptainSpecialHiSetStatus(GObj *fighter_gobj);
void ftCaptainSpecialHiProcCatch(GObj *fighter_gobj);
void ftCaptainSpecialHiThrowSetStatus(GObj *fighter_gobj);
void ftCaptainSpecialAirHiSetStatus(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftcaptain/ftcaptainspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftcaptain/ftcaptainspeciallw.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftcaptain/ftcaptainspecialhi.c"
