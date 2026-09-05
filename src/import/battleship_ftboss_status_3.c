/* P2-6 step 7 (Boss). Master Hand attacks 3: fist rockets, hand slap.
 *
 * Source import: textual includes of
 * decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossokupunch1.c,
 * ftbossokupunch2.c, ftbossokupunch3.c,
 * ftbossokutsubushi.c, ftbossokutsubushistart.c,
 * following battleship_yoshi.c (per-fighter behavior TUs).
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c): this include
 * owns every symbol of these 5 files under its source name; no renamed
 * private copies. Core + status table live in battleship_ftboss.c.
 */

#if NDS_P2_1P_GAME

#include <ft/fighter.h>
#include <gm/generic.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <sc/scene.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp ft/ftchar/ftboss/ftboss.h:55-91 verbatim. Port fighter.h lacks them. */
#ifndef nFTBossStatusDefault
typedef enum ftBossStatus
{
    nFTBossStatusDefault = nFTCommonStatusSpecialStart,
    nFTBossStatusWait,
    nFTBossStatusMove,
    nFTBossStatusHippataku,
    nFTBossStatusHarau,
    nFTBossStatusOkuhikouki1,
    nFTBossStatusOkuhikouki2,
    nFTBossStatusOkuhikouki3,
    nFTBossStatusWalk,
    nFTBossStatusWalkLoop,
    nFTBossStatusWalkWait,
    nFTBossStatusWalkShoot,
    nFTBossStatusGootsubusuUp,
    nFTBossStatusGootsubusuWait,
    nFTBossStatusGootsubusuEnd,
    nFTBossStatusGootsubusuDown,
    nFTBossStatusTsutsuku1,
    nFTBossStatusTsutsuku3,
    nFTBossStatusTsutsuku2,
    nFTBossStatusDrill,
    nFTBossStatusOkukouki,
    nFTBossStatusYubideppou1,
    nFTBossStatusYubideppou3,
    nFTBossStatusYubideppou2,
    nFTBossStatusOkupunch1,
    nFTBossStatusOkupunch2,
    nFTBossStatusOkupunch3,
    nFTBossStatusOkutsubushi,
    nFTBossStatusOkutsubushiStart,
    nFTBossStatusDeadLeft,
    nFTBossStatusDeadCenter,
    nFTBossStatusDeadRight,
    nFTBossStatusAppear
} ftBossStatus;
#endif

/* Cross-TU siblings (decls only; bodies in battleship_ftboss.c / _1,_2,_4). */
void ftBossCommonInvertLR(GObj *fighter_gobj);
void ftBossCommonCheckEdgeInvertLR(GObj *fighter_gobj);
void ftBossCommonCheckPlayerInvertLR(GObj *fighter_gobj);
void ftBossCommonRandEdgeLR(s32 line_id, Vec3f *pos);
void ftBossCommonGotoTargetEdge(GObj *fighter_gobj, Vec3f *pos);
void ftBossCommonSetPosOffsetY(GObj *fighter_gobj, Vec3f *pos, f32 off_y);
void ftBossCommonSetPosAddVelPlayer(GObj *fighter_gobj, Vec3f *pos, f32 vel_x, f32 vel_y);
void ftBossCommonSetPosAddVelAuto(GObj *fighter_gobj, Vec3f *pos, f32 vel_x, f32 vel_y);
void ftBossCommonGetPositionCenter(s32 var, Vec3f *pos_input);
void ftBossCommonSetNextAttackWait(GObj *fighter_gobj);
void ftBossCommonSetDefaultLineID(GObj *fighter_gobj);
void ftBossCommonUpdateDamageStats(GObj *fighter_gobj);
void ftBossWaitSetStatus(GObj *fighter_gobj);
void ftBossMoveSetStatus(GObj *fighter_gobj, void (*proc_setstatus)(GObj *), Vec3f *vel);

s32 syUtilsRandIntRange(s32 range);
u16 syUtilsRandUShort(void);
extern f32 syVectorNorm3D(Vec3f *dst);
extern f32 syVectorMag3D(Vec3f *src);
extern Vec3f *syVectorSub3D(Vec3f *dst, Vec3f *sub);
extern Vec3f *syVectorScale3D(Vec3f *dst, f32 scale);
extern void syVectorDiff3D(Vec3f *dst, Vec3f *a, Vec3f *b);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossokupunch1.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossokupunch2.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossokupunch3.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossokutsubushi.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossokutsubushistart.c"

#endif /* NDS_P2_1P_GAME */
