/* P2-6 step 7 (Boss). Master Hand core + status table.
 *
 * Source import: textual includes of
 * decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosscommon.c,
 * ftbossdefault.c, ftbosswait.c, ftbossmove.c, ftbossappear.c,
 * ftbossdeadcenter.c, ftbossdeadleft.c, ftbossdeadright.c
 * plus the status table
 * decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossstatus.h,
 * following battleship_yoshi.c / battleship_donkey.c (per-fighter behavior
 * TUs; ftboss.c data slots stay owned by battleship_ftchar_data_slots.c).
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c): this include
 * owns dFTBossSpecialStatusDescs and every symbol of the 8 core files under
 * its source name; no renamed private copies. Attack bodies live in
 * battleship_ftboss_status_{1..4}.c; their SetStatus/Proc decls below let the
 * table and the core AI (ftBossWaitDecideStatusComputer) link.
 *
 * Table publish: decomp keeps the table in ftbossstatus.h and publishes it
 * via ftmain.c:75,92 (dFTBossSpecialStatusDescs). Port ftmain is
 * reloc_backend_ftmain_runtime.c; wiring Boss kind -> this table there is the
 * orchestrator's seam (header/CFILES edits out of scope here).
 * Reloc tokens: ftboss behavior files use NO ll*FileID tokens (grep over the
 * dir: zero hits). Only wpbossbullet.c needs llBossMainMotionBullet* offsets.
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - nFTBossStatus* / nFTBossMotion* enums: restated below, verbatim from
 *   decomp ftboss.h:20-91 (port include/ft/fighter.h mirrors the boss structs
 *   but not these enums). Guarded so a later header promotion collides loudly.
 * - FTBOSS_ATTACK_WAIT_MAX / FTBOSS_ATTACK_WAIT_LEVEL_DIV: verbatim from
 *   decomp ftboss.h:6-7 (used by ftbosscommon.c:155).
 * - DObjGetStruct: same local macro as battleship_yoshi.c:17.
 * - syUtilsRandIntRange / syUtilsRandUShort / syVector*: local externs, same
 *   as battleship_sc1pgame_runtime.c:62 and battleship_captain.c:65-68 (no
 *   port header publishes them).
 * - mpCommonUpdateFighterProjectFloor: local extern; defined by port
 *   battleship_ftstatus_map_physics_shims.c (table's Proc Map arm).
 * - ftCommonAppearProcUpdate: local extern; defined by port
 *   battleship_ftcommon_entry.c:141 (Appear row's Proc Update arm; distinct
 *   from the weak ftCommonAppearSetStatus stub, no collision).
 * - Everything else (ftMainSetStatus, ftPhysics*, ftCommonTurn*, ftParam*,
 *   mpCollision*, gmCollision*, gMPCollisionBounds) comes from port headers.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   none in this TU. ftboss.c data slots stay in battleship_ftchar_data_slots.c.
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

/* decomp ft/ftchar/ftboss/ftboss.h:6-7 verbatim. */
#ifndef FTBOSS_ATTACK_WAIT_MAX
#define FTBOSS_ATTACK_WAIT_MAX 120
#endif
#ifndef FTBOSS_ATTACK_WAIT_LEVEL_DIV
#define FTBOSS_ATTACK_WAIT_LEVEL_DIV 100
#endif

/* decomp ft/ftchar/ftboss/ftboss.h:20-53 verbatim. Port fighter.h lacks them. */
#ifndef nFTBossMotionDefault
typedef enum ftBossMotion
{
    nFTBossMotionDefault = nFTCommonMotionSpecialStart,
    nFTBossMotionHippataku,
    nFTBossMotionHarau,
    nFTBossMotionOkuhikouki1,
    nFTBossMotionOkuhikouki2,
    nFTBossMotionOkuhikouki3,
    nFTBossMotionWalk,
    nFTBossMotionWalkLoop,
    nFTBossMotionWalkWait,
    nFTBossMotionWalkShoot,
    nFTBossMotionGootsubusuUp,
    nFTBossMotionGootsubusuWait,
    nFTBossMotionGootsubusuEnd,
    nFTBossMotionGootsubusuDown,
    nFTBossMotionTsutsuku1,
    nFTBossMotionTsutsuku3,
    nFTBossMotionTsutsuku2,
    nFTBossMotionDrill,
    nFTBossMotionOkukouki,
    nFTBossMotionYubideppou1,
    nFTBossMotionYubideppou3,
    nFTBossMotionYubideppou2,
    nFTBossMotionOkupunch1,
    nFTBossMotionOkupunch2,
    nFTBossMotionOkupunch3,
    nFTBossMotionOkutsubushi,
    nFTBossMotionDeadLeft,
    nFTBossMotionDeadCenter,
    nFTBossMotionDeadRight,
    nFTBossMotionAppear
} ftBossMotion;
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

/* Same local externs as battleship_sc1pgame_runtime.c:62 (range) and the
 * vector decls in battleship_captain.c:65-68; no port header publishes them. */
s32 syUtilsRandIntRange(s32 range);
u16 syUtilsRandUShort(void);
extern f32 syVectorNorm3D(Vec3f *dst);
extern f32 syVectorMag3D(Vec3f *src);
extern Vec3f *syVectorSub3D(Vec3f *dst, Vec3f *sub);
extern Vec3f *syVectorScale3D(Vec3f *dst, f32 scale);
extern void syVectorDiff3D(Vec3f *dst, Vec3f *a, Vec3f *b);

/* Table arms owned outside this TU (decls only; no port header carries them). */
void mpCommonUpdateFighterProjectFloor(GObj *fighter_gobj);
void ftCommonAppearProcUpdate(GObj *fighter_gobj);

/* Source ftbossfunctions.h decls, restated without pulling its ftdef.h
 * dependency (battleship_donkey.c pattern). Lets the table and the core
 * bodies call into the attack TUs while every body stays verbatim. */
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
void ftBossCommonUpdateFogColor(GObj *fighter_gobj);
void ftBossCommonSetUseFogColor(GObj *fighter_gobj);
void ftBossCommonSetDisableFogColor(GObj *fighter_gobj);
void ftBossCommonSetDefaultLineID(GObj *fighter_gobj);
void ftBossCommonUpdateDamageStats(GObj *fighter_gobj);
void ftBossDefaultProcInterrupt(GObj *fighter_gobj);
void ftBossDefaultSetStatus(GObj *fighter_gobj);
void ftBossWaitSetVelStickRange(GObj *fighter_gobj);
void ftBossWaitDecideStatusPlayer(GObj *fighter_gobj);
void ftBossWaitDecideStatusComputer(GObj *fighter_gobj);
void ftBossWaitProcInterrupt(GObj *fighter_gobj);
void ftBossWaitProcPhysics(GObj *fighter_gobj);
void ftBossWaitSetStatus(GObj *fighter_gobj);
void ftBossMoveProcPhysics(GObj *fighter_gobj);
void ftBossMoveProcMap(GObj *fighter_gobj);
void ftBossMoveSetStatus(GObj *fighter_gobj, void (*proc_setstatus)(GObj *), Vec3f *vel);
void ftBossHippatakuProcUpdate(GObj *fighter_gobj);
void ftBossHippatakuSetStatus(GObj *fighter_gobj);
void ftBossHarauResetStatus(GObj *fighter_gobj);
void ftBossHarauProcUpdate(GObj *fighter_gobj);
void ftBossHarauProcPhysics(GObj *fighter_gobj);
void ftBossHarauSetStatus(GObj *fighter_gobj);
void ftBossOkuhikouki1ProcUpdate(GObj *fighter_gobj);
void ftBossOkuhikouki1SetStatus(GObj *fighter_gobj);
void ftBossOkuhikouki2ProcUpdate(GObj *fighter_gobj);
void ftBossOkuhikouki2ProcPhysics(GObj *fighter_gobj);
void ftBossOkuhikouki2SetStatus(GObj *fighter_gobj);
void ftBossOkuhikouki3ProcUpdate(GObj *fighter_gobj);
void ftBossOkuhikouki3SetStatus(GObj *fighter_gobj);
void ftBossWalkProcUpdate(GObj *fighter_gobj);
void ftBossWalkSetStatus(GObj *fighter_gobj);
sb32 ftBossWalkLoopCheckPlayerInRange(GObj *fighter_gobj);
void ftBossWalkLoopProcPhysics(GObj *fighter_gobj);
void ftBossWalkLoopProcMap(GObj *fighter_gobj);
void ftBossWalkLoopSetStatus(GObj *fighter_gobj);
void ftBossWalkWaitProcUpdate(GObj *fighter_gobj);
void ftBossWalkWaitSetStatus(GObj *fighter_gobj);
void ftBossWalkShootProcUpdate(GObj *fighter_gobj);
void ftBossWalkShootSetStatus(GObj *fighter_gobj);
void ftBossGootsubusuUpProcPhysics(GObj *fighter_gobj);
void ftBossGootsubusuUpProcMap(GObj *fighter_gobj);
void ftBossGootsubusuUpSetStatus(GObj *fighter_gobj);
void ftBossGootsubusuWaitProcPhysics(GObj *fighter_gobj);
void ftBossGootsubusuWaitProcMap(GObj *fighter_gobj);
void ftBossGootsubusuWaitSetStatus(GObj *fighter_gobj);
void ftBossGootsubusuEndProcUpdate(GObj *fighter_gobj);
void ftBossGootsubusuEndSetStatus(GObj *fighter_gobj);
void ftBossGootsubusuDownProcMap(GObj *fighter_gobj);
void ftBossGootsubusuDownSetStatus(GObj *fighter_gobj);
void ftBossTsutsuku1ProcUpdate(GObj *fighter_gobj);
void ftBossTsutsuku1SetStatus(GObj *fighter_gobj);
void ftBossTsutsuku2ProcPhysics(GObj *fighter_gobj);
void ftBossTsutsuku2SetStatus(GObj *fighter_gobj);
void ftBossTsutsuku3ProcUpdate(GObj *fighter_gobj);
void ftBossTsutsuku3SetStatus(GObj *fighter_gobj);
void ftBossDrillProcUpdate(GObj *fighter_gobj);
void ftBossDrillProcPhysics(GObj *fighter_gobj);
void ftBossDrillProcPhysicsFollow(GObj *fighter_gobj);
void ftBossDrillProcMap(GObj *fighter_gobj);
void ftBossDrillSetStatus(GObj *fighter_gobj);
void ftBossOkukoukiProcUpdate(GObj *fighter_gobj);
void ftBossOkukoukiSetStatus(GObj *fighter_gobj);
void ftBossYubideppou1ProcUpdate(GObj *fighter_gobj);
void ftBossYubideppou1SetStatus(GObj *fighter_gobj);
void ftBossYubideppou2UpdatePosition(GObj *fighter_gobj);
void ftBossYubideppou2ProcPhysics(GObj *fighter_gobj);
void ftBossYubideppou2SetStatus(GObj *fighter_gobj);
void ftBossYubideppou3ProcUpdate(GObj *fighter_gobj);
void ftBossYubideppou3ProcPhysics(GObj *fighter_gobj);
void ftBossYubideppou3SetStatus(GObj *fighter_gobj);
void ftBossOkupunch1ProcUpdate(GObj *fighter_gobj);
void ftBossOkupunch1SetStatus(GObj *fighter_gobj);
void ftBossOkupunch2ProcUpdate(GObj *fighter_gobj);
void ftBossOkupunch2ProcPhysics(GObj *fighter_gobj);
void ftBossOkupunch2SetStatus(GObj *fighter_gobj);
void ftBossOkupunch3ProcUpdate(GObj *fighter_gobj);
void ftBossOkupunch3SetStatus(GObj *fighter_gobj);
void ftBossOkutsubushiProcUpdate(GObj *fighter_gobj);
void ftBossOkutsubushiProcPhysics(GObj *fighter_gobj);
void ftBossOkutsubushiSetStatus(GObj *fighter_gobj);
void ftBossOkutsubushiStartProcUpdate(GObj *fighter_gobj);
void ftBossOkutsubushiStartSetStatus(GObj *fighter_gobj);
void ftBossDeadLeftProcUpdate(GObj *fighter_gobj);
void ftBossDeadLeftSetStatus(GObj *fighter_gobj);
void ftBossDeadCenterProcPhysics(GObj *fighter_gobj);
void ftBossDeadCenterSetStatus(GObj *fighter_gobj);
void ftBossDeadRightSetStatus(GObj *fighter_gobj);
void ftBossAppearProcPhysics(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosscommon.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossdefault.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosswait.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossmove.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossappear.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossdeadcenter.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossdeadleft.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossdeadright.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossstatus.h"

#endif /* NDS_P2_1P_GAME */
