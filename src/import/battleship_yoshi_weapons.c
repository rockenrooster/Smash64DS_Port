/* P2-3 Yoshi weapon runtime: BattleShip Egg Throw and Yoshi Bomb stars verbatim. */
#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <nds/nds_mpprocess_source.h>
#include <reloc_data.h>
#include <string.h>
#include <sys/audio.h>
#include <sys/taskman.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* Relocation-symbol tokens follow the same DS adapter contract as Mario's
 * fireball: the descriptor stores the token address and ndsRelocGetFileData
 * reads the initialized offset back out of it against the staged source file.
 * Values are BattleShip's US relocation symbol table
 * (decomp/BattleShip-main/tools/reloc_data_symbols.us.txt:3879-3880). Both
 * WPAttributes overlap YoshiMain's file-handle words exactly as Samus's Bomb
 * and Pikachu's Thunder do; reloc_backend_assets.c normalizes and pins them. */
uintptr_t llYoshiMainEggThrowWeaponAttributes = 0x0cu;
uintptr_t llYoshiMainStarWeaponAttributes = 0x40u;

/* Exact US constants from wp/wpvars.h:71-94. */
#define WPEGGTHROW_LIFETIME 50
#define WPEGGTHROW_EXPLODE_LIFETIME 10
#define WPEGGTHROW_EXPLODE_SIZE 340.0F
#define WPEGGTHROW_TRAJECTORY_DIV 65.0F
#define WPEGGTHROW_TRAJECTORY_SUB_FORWARD F_CLC_DTOR32(73.0F)
#define WPEGGTHROW_TRAJECTORY_SUB_BEHIND F_CLC_DTOR32(107.0F)
#define WPEGGTHROW_ANGLE_MUL F_CLC_DTOR32(20.0F)
#define WPEGGTHROW_ANGLE_CLAMP F_CLC_DTOR32(6.0F)
#define WPEGGTHROW_VEL_ADD 50.0F
#define WPEGGTHROW_VEL_FORCE_MUL 2.3F
#define WPEGGTHROW_ANGLE_FORCE_MUL (-2.1F)
#define WPEGGTHROW_ANGLE_ADD (-1.5F)
#define WPEGGTHROW_GRAVITY 2.7F
#define WPEGGTHROW_TVEL 120.0F

#define WPYOSHISTAR_LIFETIME 16
#define WPYOSHISTAR_LIFETIME_SCALE_MUL 0.175F
#define WPYOSHISTAR_LIFETIME_SCALE_ADD 0.3F
#define WPYOSHISTAR_ROTATE_SPEED 0.24F
#define WPYOSHISTAR_VEL_CLAMP 1.8F
#define WPYOSHISTAR_ANGLE F_CLC_DTOR32(30.0F)
#define WPYOSHISTAR_VEL 30.0F
#define WPYOSHISTAR_OFF_X 300.0F
#define WPYOSHISTAR_OFF_Y 20.0F

/* wpYoshiEggThrowProcDisplay (wpyoshieggthrow.c:0x8016C444) is the source's
 * display hook: an RDP pipe sync, a black environment colour, then the shared
 * wpDisplayDLHead1. The DS renderer draws weapons through the native adapter
 * and has no RDP command stream, so the two gDP macros are inert here and the
 * hook reduces to the shared draw call. ACCEPTED DELTA (visual): the egg's
 * combiner environment colour is not forced to black. */
#define gDPPipeSync(pkt) ((void)0)
#define gDPSetEnvColor(pkt, r, g, b, a) ((void)0)

/* Narrow declarations normally supplied by BattleShip's broad wp/sys/ef
 * include graph. Their implementations are already imported by the shared
 * weapon/effect/system owners; Yoshi keeps the original calls and order. */
f32 syUtilsArcTan2(f32 y, f32 x);
Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
LBParticle *efManagerYoshiEggExplodeMakeEffect(Vec3f *pos);
LBParticle *efManagerEggBreakMakeEffect(Vec3f *pos);
LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos);
GObj *efManagerQuakeMakeEffect(s32 magnitude);
void wpDisplayDLHead1(GObj *weapon_gobj);
sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);

/* Source wpyoshi declarations kept narrow at the port ABI seam. */
sb32 wpYoshiEggThrowProcDead(GObj *weapon_gobj);
sb32 wpYoshiEggExplodeProcUpdate(GObj *weapon_gobj);
void wpYoshiEggHitInitVars(GObj *weapon_gobj);
void wpYoshiEggExpireInitVars(GObj *weapon_gobj);
void wpYoshiEggThrowInitVars(GObj *weapon_gobj);
sb32 wpYoshiEggThrowProcUpdate(GObj *weapon_gobj);
sb32 wpYoshiEggThrowProcMap(GObj *weapon_gobj);
sb32 wpYoshiEggThrowProcHit(GObj *weapon_gobj);
sb32 wpYoshiEggThrowProcHop(GObj *weapon_gobj);
sb32 wpYoshiEggThrowProcReflector(GObj *weapon_gobj);
void wpYoshiEggThrowProcDisplay(GObj *weapon_gobj);
GObj *wpYoshiEggThrowMakeWeapon(GObj *fighter_gobj, Vec3f *pos);

f32 wpYoshiStarGetScale(WPStruct *wp);
sb32 wpYoshiStarProcUpdate(GObj *weapon_gobj);
sb32 wpYoshiStarProcMap(GObj *weapon_gobj);
sb32 wpYoshiStarProcHit(GObj *weapon_gobj);
sb32 wpYoshiStarProcShield(GObj *weapon_gobj);
sb32 wpYoshiStarProcHop(GObj *weapon_gobj);
sb32 wpYoshiStarProcReflector(GObj *weapon_gobj);
GObj *wpYoshiStarMakeWeapon(GObj *fighter_gobj, Vec3f *pos, s32 lr);
GObj *wpYoshiStarMakeStars(GObj *fighter_gobj, Vec3f *pos);

#include "../../decomp/BattleShip-main/decomp/src/wp/wpyoshi/wpyoshieggthrow.c"
#include "../../decomp/BattleShip-main/decomp/src/wp/wpyoshi/wpyoshistar.c"
