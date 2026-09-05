/* P2-6 step 7 (Boss). Master Hand attacks 2: fist slam, drill, swoops.
 *
 * Source import: textual includes of
 * decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossgootsubusuup.c,
 * ftbossgootsubusuwait.c, ftbossgootsubusuend.c, ftbossgootsubusudown.c,
 * ftbossdrill.c, ftbossokukouki.c,
 * ftbossokuhikouki1.c, ftbossokuhikouki2.c, ftbossokuhikouki3.c,
 * following battleship_yoshi.c (per-fighter behavior TUs).
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c): this include
 * owns every symbol of these 9 files under its source name; no renamed
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

s32 syUtilsRandIntRange(s32 range);
u16 syUtilsRandUShort(void);
extern f32 syVectorNorm3D(Vec3f *dst);
extern f32 syVectorMag3D(Vec3f *src);
extern Vec3f *syVectorSub3D(Vec3f *dst, Vec3f *sub);
extern Vec3f *syVectorScale3D(Vec3f *dst, f32 scale);
extern void syVectorDiff3D(Vec3f *dst, Vec3f *a, Vec3f *b);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossgootsubusuup.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossgootsubusuwait.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossgootsubusuend.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossgootsubusudown.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossdrill.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossokukouki.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossokuhikouki1.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossokuhikouki2.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossokuhikouki3.c"

#endif /* NDS_P2_1P_GAME */
