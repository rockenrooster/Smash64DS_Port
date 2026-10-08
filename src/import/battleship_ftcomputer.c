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

#if NDS_P4
/* P4: Remix hooks the recover objective after its walk (AI.asm
 * custom_recovery_logic). The definition `(FTStruct *fp)` pastes to the
 * base name, the decomp's call `(fp)` to the port's copy below, which adds
 * that call. */
#define NDS_P4_RECOVER_FTStruct ndsBaseFTComputerFollowObjectiveRecover(FTStruct
#define NDS_P4_RECOVER_fp ndsP4FTComputerFollowObjectiveRecover(fp
#define ftComputerFollowObjectiveRecover(arg) NDS_P4_RECOVER_##arg)
static void ndsP4FTComputerFollowObjectiveRecover(FTStruct *fp);
/* The same seam for the objective step (Remix runs cpu_post_process after
 * it), the input interpreter (Remix's stick-X values) and the long-range
 * special (Remix's ai_long_range row). */
#define NDS_P4_OBJECTIVE_FTStruct ndsBaseFTComputerProcessObjective(FTStruct
#define NDS_P4_OBJECTIVE_fp ndsP4FTComputerProcessObjective(fp
#define ftComputerProcessObjective(arg) NDS_P4_OBJECTIVE_##arg)
static void ndsP4FTComputerProcessObjective(FTStruct *fp);
#define NDS_P4_INPUTS_FTStruct ndsBaseFTComputerUpdateInputs(FTStruct
#define NDS_P4_INPUTS_fp ndsP4FTComputerUpdateInputs(fp
#define ftComputerUpdateInputs(arg) NDS_P4_INPUTS_##arg)
static void ndsP4FTComputerUpdateInputs(FTStruct *fp);
#define NDS_P4_LONG_RANGE_FTStruct ndsBaseFTComputerLongRange(FTStruct
#define NDS_P4_LONG_RANGE_fp ndsP4FTComputerLongRange(fp
#define func_ovl3_80138AA8(arg, ...) NDS_P4_LONG_RANGE_##arg, __VA_ARGS__)
static sb32 ndsP4FTComputerLongRange(FTStruct *fp, sb32 is_delay);
#endif
#define ftComputerSetupAll ndsBaseFTComputerSetupAll
#define ftComputerProcessAll ndsBaseFTComputerProcessAll
#define ftComputerSetFighterDamageDetectSize \
    ndsBaseFTComputerSetFighterDamageDetectSize

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcomputer.c"

#if NDS_P4
#undef ftComputerFollowObjectiveRecover
#undef ftComputerProcessObjective
#undef ftComputerUpdateInputs
#undef func_ovl3_80138AA8
#include <nds/nds_p4.h>

/* ftcomputer.c:6535, unchanged, with Remix's recovery_logic call after the
 * walk (AI.asm custom_recovery_logic, at 0x80137FBC). */
static void ndsP4FTComputerFollowObjectiveRecover(FTStruct *fp)
{
    FTComputer *com = &fp->computer;

    if (ftComputerCheckTryCancelSpecialN(fp) == FALSE)
    {
        func_ovl3_80134964(fp);

#if defined(REGION_US)
        if (fp->fkind == nFTKindPikachu)
        {
            switch (fp->status_id)
            {
            case nFTPikachuStatusSpecialAirHiStart:
                com->target_pos.x = fp->joints[nFTPartsJointTopN]->translate.vec.f.x;
                com->target_pos.y = fp->joints[nFTPartsJointTopN]->translate.vec.f.y + 1100.0F;
                break;
            case nFTPikachuStatusSpecialAirHi:
                com->target_pos.x = 0.0F;
                com->target_pos.y = fp->joints[nFTPartsJointTopN]->translate.vec.f.y;
                break;
            }
        }
#endif

        ftComputerFollowObjectiveWalk(fp);
        ndsP4ComputerRecover(fp);
    }
}

void ftComputerFollowObjectiveRecover(FTStruct *fp)
{
    ndsP4FTComputerFollowObjectiveRecover(fp);
}

/* ftComputerProcessAll's objective step, then Remix's cpu_post_process
 * (AI.asm; patched in at 0x8013A884, between the objective and the
 * inputs). */
static void ndsP4FTComputerProcessObjective(FTStruct *fp)
{
    ndsBaseFTComputerProcessObjective(fp);
    ndsP4ComputerPostProcess(fp);
}

void ftComputerProcessObjective(FTStruct *fp)
{
    ndsP4FTComputerProcessObjective(fp);
}

/* The input interpreter; a Remix routine's directional stick X is stored
 * after its run (ndsP4ComputerStickX). */
static void ndsP4FTComputerUpdateInputs(FTStruct *fp)
{
    s32 stick_x = ndsP4ComputerStickX(fp);

    ndsBaseFTComputerUpdateInputs(fp);

    if (stick_x != NDS_P4_COMPUTER_STICK_KEEP)
    {
        fp->input.cp.stick_range.x = stick_x;
    }
}

void ftComputerUpdateInputs(FTStruct *this_fp)
{
    ndsP4FTComputerUpdateInputs(this_fp);
}

/* func_ovl3_80138AA8, the long-range special. Remix sends a content whose
 * ai_long_range is NONE from the fkind jump table to its FALSE return
 * (0x80138ECC), after the source's lead-in: that keeps its two random
 * draws (the reaction delay and the reflector-target roll). */
static sb32 ndsP4FTComputerLongRange(FTStruct *this_fp, sb32 is_delay)
{
    const NDSP4Computer *c = ndsP4Computer(this_fp);
    FTComputer *com = &this_fp->computer;

    if ((c == NULL) || (c->long_range_none == FALSE))
    {
        return ndsBaseFTComputerLongRange(this_fp, is_delay);
    }
    if (DISTANCE(this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y, com->target_pos.y) < 400.0F)
    {
        if (com->unk_ftcom_0x35 == 0)
        {
            com->unk_ftcom_0x35 = 2.0F * (syUtilsRandFloat() * (FTCOMPUTER_LEVEL_MAX - this_fp->level));
        }
        syUtilsRandFloat();
    }
    return FALSE;
}

sb32 func_ovl3_80138AA8(FTStruct *this_fp, sb32 is_delay)
{
    return ndsP4FTComputerLongRange(this_fp, is_delay);
}
#endif
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
