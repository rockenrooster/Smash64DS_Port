/* Fenced whole BattleShip ft/ftcomputer.c import. */
#include <ft/ftcomputer.h>
#include <nds/nds_scene_harness.h>
#include <nds/nds_startup.h>
#include <string.h>
#include "nds_build_config.h"

/* The published ROM is source-normal. Automated fast iteration explicitly
 * clears this at the pre-battle seam to skip CPU/countdown/timer work.
 *
 * NDS_R2_FOX_CPU_DEFAULT SEEDS IT FOR A HAND-PLAYED ROM, because until
 * 2026-08-06 nothing could. Clearing this needs a gdb write at
 * scVSBattleStartBattle, so every fast-iteration path was a scripted harness
 * (capture-melonds.ps1:327) and a ROM the owner launches himself always got the
 * level-3 Fox. That is the wrong default when the thing under inspection is a
 * visual effect the owner has to stand still and look at. Flag stays 1 so the
 * published battle ROM and every verifier are bit-identical to before; build
 * NDS_R2_FOX_CPU_DEFAULT=0 for a look-at-it ROM, which also skips the 3/2/1/GO
 * wait and freezes the match timer. */
#if NDS_R2_FOX_CPU_DEFAULT
volatile u32 gNdsBattlePlayableFoxCpuEnabled = 1u;
#else
volatile u32 gNdsBattlePlayableFoxCpuEnabled = 0u;
#endif

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifndef bzero
#define bzero(ptr, size) memset((ptr), 0, (size))
#endif

/* 2026-10-05: battleship_ftcomputer_fixed.c defines these three in fixed point;
 * weak here, so the decomp's own calls below reach those too. */
#pragma weak ftComputerCheckFindTarget
#pragma weak ftComputerCheckEvadeDistance
#pragma weak ftComputerCheckDetectTarget

#define ftComputerSetupAll ndsBaseFTComputerSetupAll
#define ftComputerProcessAll ndsBaseFTComputerProcessAll
#define ftComputerSetFighterDamageDetectSize \
    ndsBaseFTComputerSetFighterDamageDetectSize

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcomputer.c"

#undef ftComputerSetupAll
#undef ftComputerProcessAll
#undef ftComputerSetFighterDamageDetectSize

#if NDS_SHIP_TELEMETRY
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
#endif

void ftComputerSetupAll(GObj *fighter_gobj)
{
#if NDS_SHIP_TELEMETRY
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 x;
#endif

    ndsMPCollisionEnsureLineGroups();
    ndsBaseFTComputerSetupAll(fighter_gobj);
#if NDS_SHIP_TELEMETRY
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
#endif
}

/* P2-2p8 (2026-10-04, owner ruling D12d): CPU decisions at 30 Hz in VS
 * battles. ftComputerProcessAll (ftcomputer.c:7801) decides -- trait,
 * behaviour, objective -- on every source tick on which its input script is
 * idle (input_wait 0). On a batch's earlier tick (gNdsFtPoseEvalTick 0) the
 * same body runs without that decision: the behaviour-change countdown still
 * ticks, and ftComputerUpdateInputs has nothing to do while input_wait is 0,
 * so the inputs in force are held one tick longer. Each CPU decides at most
 * once a presentation; a one-tick batch (every tick is the last) keeps the
 * source rate, as do 1P battles. Changes CPU behaviour and the replay digest
 * (owner-approved); humans are unaffected. A/B word gNdsCpuDecide30Hz
 * (0 = every tick). */
volatile u32 gNdsCpuDecide30Hz __attribute__((used, section(".data"))) = 0u;
extern volatile u32 gNdsFtPoseEvalTick;
extern SCCommonData gSCManagerSceneData;

#if defined(NDS_LAB_FOURCPU_WORDS) && NDS_LAB_FOURCPU_WORDS
/* LAB ONLY: drives chosen CPU players' inputs so a probe can make any move on
 * demand (a special's effects, for cost and visual checks). The CPU still
 * decides; its inputs are replaced afterwards. gNdsLabForceInputSlots is a
 * player mask (0 = off). gNdsLabForceInput: N64 buttons in bits 0-15, stick x
 * (s8) in bits 16-23, stick y (s8) in bits 24-31. gNdsLabForceInputPeriod:
 * period in ticks (bits 0-15), held ticks (bits 16-31); outside the held
 * ticks the inputs are released, so each period starts with a fresh press. */
volatile u32 gNdsLabForceInput __attribute__((used)) = 0u;
volatile u32 gNdsLabForceInputSlots __attribute__((used)) = 0u;
volatile u32 gNdsLabForceInputPeriod __attribute__((used)) = 0u;
static u32 sNdsLabForceInputTicks[4];

static void __attribute__((noinline)) ndsLabForceInput(FTStruct *fp)
{
    u32 player = (u32)fp->player & 3u;
    u32 period = gNdsLabForceInputPeriod & 0xffffu;
    u32 held = gNdsLabForceInputPeriod >> 16;
    u32 tick = sNdsLabForceInputTicks[player]++;

    if ((period == 0u) || ((tick % period) < held))
    {
        fp->input.cp.button_inputs = (u16)gNdsLabForceInput;
        fp->input.cp.stick_range.x = (s8)(gNdsLabForceInput >> 16);
        fp->input.cp.stick_range.y = (s8)(gNdsLabForceInput >> 24);
    }
    else
    {
        fp->input.cp.button_inputs = 0u;
        fp->input.cp.stick_range.x = 0;
        fp->input.cp.stick_range.y = 0;
    }
}
#endif

void ftComputerProcessAll(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    /* Cycle 86 SCPU. The level-3 CPU decision path, reached once per CPU fighter
     * per logic update from ftMainProcUpdateInterrupt (decomp ft/ftmain.c:1269,
     * case nFTPlayerKindCom), so it is nested inside that proc and therefore
     * inside GCRA. This is the one sub-owner the two arms cannot share: the
     * gate arm runs BOTH fighters as CPU and Boundary runs one, so SCPU should
     * read about 2x on the gate arm. That ratio is the engagement proof.
     *
     * The guard is inverted into if/else rather than early-returning so the
     * bracket has a single exit and the paused-Fox path is still charged what it
     * costs -- mechanically identical (`if (c) { A; return; } B;` ==
     * `if (c) { A; } else { B; }`), the same technique cycle 85 used on
     * ndsR2AnimCachePreloadStep. cpuGetTiming is forward-declared for the reason
     * given in battleship_lbparticle.c:1697; libnds nds/timers.h:255 is the
     * authority for the signature. */
#if NDS_TICK_HUD && NDS_TICK_HUD_SRC_SPLIT
    extern u32 cpuGetTiming(void);
    u32 computer_start = cpuGetTiming();
#endif

    if ((gNdsSceneHarnessMode ==
         NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE_REALTIME) &&
        (fp->player == 1) &&
        (fp->fkind == nFTKindFox) &&
        (gNdsBattlePlayableFoxCpuEnabled == 0u))
    {
        fp->input.cp.button_inputs = 0u;
        fp->input.cp.stick_range.x = 0;
        fp->input.cp.stick_range.y = 0;
    }
    else if ((gNdsCpuDecide30Hz != 0u) && (gNdsFtPoseEvalTick == 0u) &&
             (fp->fkind != nFTKindBoss) && (fp->computer.input_wait == 0) &&
             (gSCManagerSceneData.scene_curr == nSCKindVSBattle))
    {
        /* The source body with its decision block skipped (see above). */
        if (fp->computer.behavior_change_wait != 0)
        {
            fp->computer.behavior_change_wait--;
        }
    }
    else
    {
        ndsBaseFTComputerProcessAll(fighter_gobj);
#if NDS_SHIP_TELEMETRY
        gNdsFTComputerProcessCount++;
        ndsFTComputerRecord(fp);
#endif
    }
#if defined(NDS_LAB_FOURCPU_WORDS) && NDS_LAB_FOURCPU_WORDS
    if (((gNdsLabForceInputSlots >> ((u32)fp->player & 3u)) & 1u) != 0u)
    {
        ndsLabForceInput(fp);
    }
#endif
#if NDS_TICK_HUD && NDS_TICK_HUD_SRC_SPLIT
    gNdsTickHudSrcComputerTicks += cpuGetTiming() - computer_start;
#endif
}

void ftComputerSetFighterDamageDetectSize(GObj *fighter_gobj)
{
    ndsBaseFTComputerSetFighterDamageDetectSize(fighter_gobj);
#if NDS_SHIP_TELEMETRY
    gNdsFTComputerDamageDetectCount++;
#endif
}
