/* P2-3 Ness weapon runtime: BattleShip PK Fire spark and PK Thunder verbatim. */
#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <it/item.h>
#include <mp/map.h>
#include <nds/nds_mpprocess_source.h>
#include <reloc_data.h>
#include <string.h>
#include <sys/audio.h>
#include <sys/taskman.h>
#include <wp/weapon.h>
#include "battleship_ness_common.h"

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* Relocation-symbol tokens: the descriptor stores the token address and
 * ndsRelocGetFileData reads the initialized offset back out of it against the
 * staged source file. Values are BattleShip's US relocation symbol table
 * (decomp/BattleShip-main/tools/reloc_data_symbols.us.txt:3867-3870). The two
 * PK Thunder WPAttributes overlap NessMain's file-handle words like Pikachu's;
 * PK Fire's sits at the head of NessSpecial1. reloc_backend_assets.c
 * normalizes and pins all three. */
uintptr_t llNessMainPKThunderWeaponAttributes = 0x0cu;
uintptr_t llNessMainPKThunderTrailWeaponAttributes = 0x40u;
uintptr_t llNessSpecial1PKFireWeaponAttributes = 0x00u;

/* Exact US constants from wp/wpvars.h:55-66. */
#define WPPKFIRE_LIFETIME 20
#define WPPKFIRE_POS_MUL 160.0F
#define WPPKTHUNDER_LIFETIME 160
#define WPPKTHUNDER_SPAWN_TRAIL_FRAME (WPPKTHUNDER_LIFETIME - 2)
#define WPPKTHUNDER_TURN_STICK_THRESHOLD 45
#define WPPKTHUNDER_ANGLE_STEP F_CLC_DTOR32(6.0F)
#define WPPKTHUNDER_ANGLE_DIV (45.0F / 6.0F)
#define WPPKTHUNDER_VEL 60.0F
#define WPPKTHUNDER_REFLECT_POS_Y_ADD 250.0F
#define WPPKTHUNDER_PARTS_COUNT 5
#define WPPKTHUNDER_TEXTURES_NUM 4

/* Narrow declarations normally supplied by BattleShip's broad include graph;
 * the implementations are already imported by the shared owners. */
f32 syUtilsArcTan2(f32 y, f32 x);
s32 syUtilsRandIntRange(s32 range);
Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
f32 syVectorNorm3D(Vec3f *dst);
f32 syVectorAngleDiff3D(Vec3f *a, Vec3f *b);
Vec3f *syVectorNormCross3D(Vec3f *dst, Vec3f *a, Vec3f *b);
LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
LBParticle *efManagerImpactShockMakeEffect(Vec3f *pos, s32 size);
void wpMainVelSetLR(GObj *weapon_gobj);
sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);
GObj *efManagerNessPKThunderTrailMakeEffect(GObj *fighter_gobj);
GObj *efManagerNessPKReflectTrailMakeEffect(GObj *weapon_gobj);
GObj *itNessPKFireMakeItem(GObj *weapon_gobj, Vec3f *pos, Vec3f *vel);

#include "../../decomp/BattleShip-main/decomp/src/wp/wpness/wpnesspkfire.c"
/* wpnesspkthunder.c:437 passes two arguments to the zero-argument
 * wpManagerGetGroupID (the source comments on it); the port ABI declares the
 * real prototype, so the call is routed through the declared form. */
#define wpManagerGetGroupID(...) (wpManagerGetGroupID)()
#include "../../decomp/BattleShip-main/decomp/src/wp/wpness/wpnesspkthunder.c"
#undef wpManagerGetGroupID
