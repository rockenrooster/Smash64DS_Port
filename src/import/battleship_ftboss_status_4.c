/* P2-6 step 7 (Boss). Master Hand attacks 4: finger gun.
 *
 * Source import: textual includes of
 * decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossyubideppou1.c,
 * ftbossyubideppou2.c, ftbossyubideppou3.c,
 * following battleship_yoshi.c (per-fighter behavior TUs). The gun fires
 * wpBossBulletNormal/HardMakeWeapon, owned by battleship_wpbossbullet.c
 * (same flag); decls below, no second bodies here.
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c): this include
 * owns every symbol of these 3 files under its source name; no renamed
 * private copies. Core + status table live in battleship_ftboss.c.
 */

#if NDS_P2_1P_GAME

#include <ft/fighter.h>
#include <gm/generic.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <sc/scene.h>
#include <ft/ftchar/ftboss/ftboss.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* Finger-gun bullets, owned by battleship_wpbossbullet.c (same flag). */
GObj *wpBossBulletNormalMakeWeapon(GObj *fighter_gobj, Vec3f *pos);
GObj *wpBossBulletHardMakeWeapon(GObj *fighter_gobj, Vec3f *pos);

s32 syUtilsRandIntRange(s32 range);
u16 syUtilsRandUShort(void);
extern f32 syVectorNorm3D(Vec3f *dst);
extern f32 syVectorMag3D(Vec3f *src);
extern Vec3f *syVectorSub3D(Vec3f *dst, Vec3f *sub);
extern Vec3f *syVectorScale3D(Vec3f *dst, f32 scale);
extern void syVectorDiff3D(Vec3f *dst, Vec3f *a, Vec3f *b);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossyubideppou1.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossyubideppou2.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossyubideppou3.c"

#endif /* NDS_P2_1P_GAME */
