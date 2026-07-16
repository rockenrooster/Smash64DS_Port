/* Fenced whole BattleShip ft/ftcomputer.c import. */
#include <ft/ftcomputer.h>
#include <nds/nds_scene_harness.h>
#include <nds/nds_startup.h>
#include <string.h>

/* The published ROM is source-normal. Automated fast iteration explicitly
 * clears this at the pre-battle seam to skip CPU/countdown/timer work. */
volatile u32 gNdsBattlePlayableFoxCpuEnabled = 1u;

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifndef bzero
#define bzero(ptr, size) memset((ptr), 0, (size))
#endif

#define ftComputerSetupAll ndsBaseFTComputerSetupAll
#define ftComputerProcessAll ndsBaseFTComputerProcessAll
#define ftComputerSetFighterDamageDetectSize \
    ndsBaseFTComputerSetFighterDamageDetectSize

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcomputer.c"

#undef ftComputerSetupAll
#undef ftComputerProcessAll
#undef ftComputerSetFighterDamageDetectSize

static s32 ndsFTComputerXMilli(FTStruct *fp)
{
    DObj *root = fp->joints[nFTPartsJointTopN];

    return (root != NULL) ? (s32)(root->translate.vec.f.x * 1000.0F) : 0;
}

static void ndsFTComputerRecord(FTStruct *fp)
{
    FTComputer *com = &fp->computer;
    s32 x = ndsFTComputerXMilli(fp);
    u32 i;

    if (com->target_gobj != NULL && com->target_user != NULL)
    {
        gNdsFTComputerTargetFrames++;
    }
    if (com->objective < 32u)
    {
        gNdsFTComputerObjectiveMask |= 1u << com->objective;
    }
    if (com->behavior < 32u)
    {
        gNdsFTComputerBehaviorMask |= 1u << com->behavior;
    }
    if (com->input_kind != gNdsFTComputerFinalInputKind)
    {
        gNdsFTComputerInputChangeCount++;
    }
    if ((fp->input.cp.stick_range.x != 0) ||
        (fp->input.cp.stick_range.y != 0))
    {
        gNdsFTComputerStickFrames++;
    }
    if ((fp->input.cp.button_inputs & fp->input.button_mask_a) != 0u)
    {
        gNdsFTComputerButtonAFrames++;
    }
    if ((fp->input.cp.button_inputs & fp->input.button_mask_b) != 0u)
    {
        gNdsFTComputerButtonBFrames++;
    }
    if ((fp->input.cp.button_inputs & fp->input.button_mask_z) != 0u)
    {
        gNdsFTComputerButtonZFrames++;
    }
    if (fp->motion_attack_id != nFTMotionAttackIDNone)
    {
        gNdsFTComputerAttackFrames++;
    }
    for (i = 0u; i < ARRAY_COUNT(fp->attack_colls); i++)
    {
        if (fp->attack_colls[i].attack_state != nGMAttackStateOff)
        {
            gNdsFTComputerHitboxFrames++;
            break;
        }
    }
    if ((fp->status_id == nFTCommonStatusGuardOn) ||
        (fp->status_id == nFTCommonStatusGuard) ||
        (fp->status_id == nFTCommonStatusGuardOff))
    {
        gNdsFTComputerGuardFrames++;
    }
    if (com->objective == nFTComputerObjectiveRecover)
    {
        gNdsFTComputerRecoveryFrames++;
    }
    if ((u32)fp->status_id != gNdsFTComputerFinalStatus)
    {
        gNdsFTComputerStatusChangeCount++;
    }
    if (x < gNdsFTComputerMinXMilli)
    {
        gNdsFTComputerMinXMilli = x;
    }
    if (x > gNdsFTComputerMaxXMilli)
    {
        gNdsFTComputerMaxXMilli = x;
    }
    if ((gSCManagerBattleState != NULL) &&
        (gSCManagerBattleState->players[0].fighter_gobj != NULL))
    {
        FTStruct *mario = ftGetStruct(
            gSCManagerBattleState->players[0].fighter_gobj);

        if ((mario != NULL) &&
            ((u32)mario->percent_damage > gNdsFTComputerMarioDamageMax))
        {
            gNdsFTComputerMarioDamageMax = (u32)mario->percent_damage;
        }
    }
    gNdsFTComputerFinalStatus = (u32)fp->status_id;
    gNdsFTComputerFinalGA = (u32)fp->ga;
    gNdsFTComputerFinalInputKind = com->input_kind;
    gNdsFTComputerFinalXMilli = x;
}

void ftComputerSetupAll(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 x;

    ndsMPCollisionEnsureLineGroups();
    ndsBaseFTComputerSetupAll(fighter_gobj);
    gNdsFTComputerSetupCount++;
    gNdsFTComputerFloorLineCount =
        gMPCollisionLineGroups[nMPLineKindFloor].line_count;
    x = ndsFTComputerXMilli(fp);
    gNdsFTComputerStartXMilli = x;
    gNdsFTComputerMinXMilli = x;
    gNdsFTComputerMaxXMilli = x;
    gNdsFTComputerFinalStatus = (u32)fp->status_id;
    gNdsFTComputerFinalGA = (u32)fp->ga;
    gNdsFTComputerFinalInputKind = fp->computer.input_kind;
    gNdsFTComputerFinalXMilli = x;
}

void ftComputerProcessAll(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((gNdsSceneHarnessMode ==
         NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE_REALTIME) &&
        (fp->player == 1) &&
        (fp->fkind == nFTKindFox) &&
        (gNdsBattlePlayableFoxCpuEnabled == 0u))
    {
        fp->input.cp.button_inputs = 0u;
        fp->input.cp.stick_range.x = 0;
        fp->input.cp.stick_range.y = 0;
        return;
    }

    ndsBaseFTComputerProcessAll(fighter_gobj);
    gNdsFTComputerProcessCount++;
    ndsFTComputerRecord(fp);
}

void ftComputerSetFighterDamageDetectSize(GObj *fighter_gobj)
{
    ndsBaseFTComputerSetFighterDamageDetectSize(fighter_gobj);
    gNdsFTComputerDamageDetectCount++;
}
