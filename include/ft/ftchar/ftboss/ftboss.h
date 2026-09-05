#ifndef _FTBOSS_H_
#define _FTBOSS_H_

/* Master Hand (nFTKindBoss). Port mirror of decomp ft/ftchar/ftboss/ftboss.h:
 * 6-7 and 20-91 (the attack-wait constants and the motion / status enums,
 * verbatim) plus the ftbossfunctions.h declarations restated without its
 * ft/ftdef.h dependency (the battleship_donkey.c pattern), so the five boss
 * translation units under src/import share one copy instead of five. The
 * per-status vars (ftBossInfo, FTBossPassiveVars, FTBossStatusVars) already
 * live in include/ft/fighter.h. Include after <ft/fighter.h>. */

#include <ft/fighter.h>

/* decomp ft/ftchar/ftboss/ftboss.h:6-7 verbatim. */
#define FTBOSS_ATTACK_WAIT_MAX 120
#define FTBOSS_ATTACK_WAIT_LEVEL_DIV 100

/* decomp ft/ftchar/ftboss/ftboss.h:20-53 verbatim. */
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

/* decomp ft/ftchar/ftboss/ftboss.h:55-91 verbatim. */
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

/* decomp ft/ftchar/ftboss/ftbossfunctions.h, restated. */
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

#endif /* _FTBOSS_H_ */
