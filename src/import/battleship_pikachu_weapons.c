/* P2-3 Pikachu weapon runtime: BattleShip Thunder Jolt and Thunder verbatim. */
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <nds/nds_mpprocess_source.h>
#include <reloc_data.h>
#include <string.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif
#ifndef bzero
#define bzero(ptr, size) memset((ptr), 0, (size))
#endif

/* Relocation-symbol tokens follow the same DS adapter contract as Mario's
 * fireball: the descriptor stores the token address and ndsRelocGetFileData
 * reads the initialized offset back out of it against the staged source file.
 * Values are BattleShip's US relocation symbol table
 * (decomp/BattleShip-main/tools/reloc_data_symbols.us.txt:3873-3877 and
 * :4391-4392). Thunder's two WPAttributes overlap PikachuMain's file-handle
 * words exactly as Samus's Bomb and Ness's PK Thunder do; Thunder Jolt's two
 * sit back to back at the head of PikachuSpecial1. */
uintptr_t llPikachuMainThunderHeadWeaponAttributes = 0x0cu;
uintptr_t llPikachuMainThunderTrailWeaponAttributes = 0x40u;
uintptr_t llPikachuSpecial1ThunderJoltAirWeaponAttributes = 0x00u;
uintptr_t llPikachuSpecial1ThunderJoltGroundWeaponAttributes = 0x34u;
uintptr_t llPikachuSpecial3ThunderJoltBAnimJoint = 0x1a20u;
uintptr_t llPikachuSpecial3ThunderJoltBMatAnimJoint = 0x1ae0u;

/* Exact US constants from wp/wpvars.h. */
#define WPPIKACHUJOLT_VEL 55.0F
#define WPPIKACHUJOLT_GRAVITY 0.0F
#define WPPIKACHUJOLT_LIFETIME 100
#define WPPIKACHUJOLT_TVEL 50.0F
#define WPPIKACHUJOLT_ROTATE_ANGLE_MAX F_CLC_DTOR32(100.0F)
#define WPPIKACHUJOLT_ANIM_PUSH_FRAME 7.5F
#define WPPIKACHUJOLT_COLL_GROUND 0
#define WPPIKACHUJOLT_COLL_RWALL 1
#define WPPIKACHUJOLT_COLL_CEIL 2
#define WPPIKACHUJOLT_COLL_LWALL 3

#define WPPIKACHUTHUNDER_TEXTURES_NUM 4
#define WPPIKACHUTHUNDER_SPAWN_LIFETIME 40
#define WPPIKACHUTHUNDER_TRAIL_LIFETIME 10
#define WPPIKACHUTHUNDER_EXPIRE 6

/* Narrow declarations normally supplied by BattleShip's broad wp/sys/ef
 * include graph. Their implementations are already imported by the shared
 * weapon/effect/system owners; Pikachu keeps the original calls and order. */
f32 syUtilsArcTan2(f32 y, f32 x);
s32 syUtilsRandIntRange(s32 range);
f32 syVectorAngleDiff3D(Vec3f *a, Vec3f *b);
Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);
void gcSetAllAnimSpeed(GObj *gobj, f32 anim_speed);
void gcPlayAnimAll(GObj *gobj);
GObj *efManagerPikachuThunderJoltMakeEffect(Vec3f *pos, f32 rotate);
GObj *efManagerPikachuThunderTrailMakeEffect(Vec3f *pos, s32 lifetime,
                                             s32 texture_index);
LBParticle *efManagerImpactShockMakeEffect(Vec3f *pos, s32 size);
LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos);
GObj *efManagerQuakeMakeEffect(s32 magnitude);
void wpMainPlayFGM(WPStruct *wp, u16 sfx_id);
void wpMainVelSetLR(GObj *weapon_gobj);
sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);
sb32 wpMapTestAllCheckFloor(GObj *weapon_gobj);

/* Source wppikachu declarations kept narrow at the port ABI seam. */
void wpPikachuThunderHeadSetDestroy(GObj *weapon_gobj, sb32 is_destroy);
void wpPikachuThunderHeadMakeTrailEffect(GObj *weapon_gobj, s32 arg1);
sb32 wpPikachuThunderHeadProcUpdate(GObj *weapon_gobj);
sb32 wpPikachuThunderHeadProcMap(GObj *weapon_gobj);
sb32 wpPikachuThunderHeadProcDead(GObj *weapon_gobj);
GObj *wpPikachuThunderHeadMakeWeapon(GObj *fighter_gobj, Vec3f *pos,
                                     Vec3f *vel);
sb32 wpPikachuThunderTrailProcUpdate(GObj *weapon_gobj);
sb32 wpPikachuThunderTrailProcHit(GObj *weapon_gobj);
GObj *wpPikachuThunderTrailMakeWeapon(GObj *weapon_gobj, Vec3f *pos);

sb32 wpPikachuThunderJoltAirProcUpdate(GObj *weapon_gobj);
sb32 wpPikachuThunderJoltAirProcMap(GObj *weapon_gobj);
sb32 wpPikachuThunderJoltAirProcHit(GObj *weapon_gobj);
sb32 wpPikachuThunderJoltAirProcHop(GObj *weapon_gobj);
sb32 wpPikachuThunderJoltAirProcReflector(GObj *weapon_gobj);
GObj *wpPikachuThunderJoltAirMakeWeapon(GObj *fighter_gobj, Vec3f *pos,
                                        Vec3f *vel);
void wpPikachuThunderJoltGroundAddAnim(GObj *weapon_gobj);
sb32 wpPikachuThunderJoltGroundProcUpdate(GObj *weapon_gobj);
s32 wpPikachuThunderJoltGroundGetStatus(GObj *weapon_gobj);
sb32 wpPikachuThunderJoltGroundCheckDestroy(GObj *weapon_gobj);
sb32 wpPikachuThunderJoltGroundProcMap(GObj *weapon_gobj);
sb32 wpPikachuThunderJoltGroundProcHit(GObj *weapon_gobj);
sb32 wpPikachuThunderJoltGroundProcReflector(GObj *weapon_gobj);
GObj *wpPikachuThunderJoltGroundMakeWeapon(GObj *prev_gobj, Vec3f *pos,
                                           s32 coll_type);

#include "../../decomp/BattleShip-main/decomp/src/wp/wppikachu/wppikachuthunder.c"
#include "../../decomp/BattleShip-main/decomp/src/wp/wppikachu/wppikachuthunderjolt.c"
