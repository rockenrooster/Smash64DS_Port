/* P2-6 step 7 (Boss). Master Hand attacks 1: finger-walk, pokes, slaps.
 *
 * Source import: textual includes of
 * decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosswalk.c,
 * ftbosswalkloop.c, ftbosswalkwait.c, ftbosswalkshoot.c,
 * ftbosstsutsuku1.c, ftbosstsutsuku2.c, ftbosstsutsuku3.c,
 * ftbosshippataku.c, ftbossharau.c,
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

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosswalk.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosswalkloop.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosswalkwait.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosswalkshoot.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosstsutsuku1.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosstsutsuku2.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosstsutsuku3.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosshippataku.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossharau.c"

#endif /* NDS_P2_1P_GAME */
