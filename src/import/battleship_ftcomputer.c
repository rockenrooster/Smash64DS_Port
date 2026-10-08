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
/* The objective walk's recover branch skips the double-jump up special for
 * Yoshi and Jigglypuff (ftcomputer.c:5133), a compare Remix extends to J
 * Yoshi and Marina only (yoshishared.asm yoshi_cpu_fix_1), so a content
 * aliasing Yoshi (Bowser) recovers with it. The file also has a
 * `case nFTKindYoshi:` (4116), which an NDS_P4_PARENT_KIND view cannot
 * reach, so the port's copy runs the walk with the content's fkind out of
 * range instead. */
#define NDS_P4_WALK_FTStruct ndsBaseFTComputerFollowObjectiveWalk(FTStruct
#define NDS_P4_WALK_fp ndsP4FTComputerFollowObjectiveWalk(fp
#define NDS_P4_WALK_this_fp ndsP4FTComputerFollowObjectiveWalk(this_fp
#define ftComputerFollowObjectiveWalk(arg) NDS_P4_WALK_##arg)
static void ndsP4FTComputerFollowObjectiveWalk(FTStruct *fp);
/* Remix's reflect AI (nds_p4.h NDS_P4_COMPUTER_REFLECT_*): the recover
 * objective's jump range, the incoming-attack scan, the default objective's
 * reflector hold and the item objective's reflector-target test all compare
 * the fighter's own id, which for a content is not its parent's. */
#define NDS_P4_JUMP_RANGE_FTStruct ndsBaseFTComputerRecoverJumpRange(FTStruct
#define NDS_P4_JUMP_RANGE_fp ndsP4FTComputerRecoverJumpRange(fp
#define func_ovl3_801346D4(arg) NDS_P4_JUMP_RANGE_##arg)
static void ndsP4FTComputerRecoverJumpRange(FTStruct *fp);
#define NDS_P4_HAZARD_FTStruct ndsBaseFTComputerCheckHazard(FTStruct
#define NDS_P4_HAZARD_fp ndsP4FTComputerCheckHazard(fp
#define func_ovl3_80135B78(arg) NDS_P4_HAZARD_##arg)
static sb32 ndsP4FTComputerCheckHazard(FTStruct *fp);
#define NDS_P4_USE_ITEM_FTStruct ndsBaseFTComputerFollowObjectiveUseItem(FTStruct
#define NDS_P4_USE_ITEM_fp ndsP4FTComputerFollowObjectiveUseItem(fp
#define ftComputerFollowObjectiveUseItem(arg) NDS_P4_USE_ITEM_##arg)
static void ndsP4FTComputerFollowObjectiveUseItem(FTStruct *fp);
#define ftComputerProcDefault ndsBaseFTComputerProcDefault
#endif
#define ftComputerSetupAll ndsBaseFTComputerSetupAll
#define ftComputerProcessAll ndsBaseFTComputerProcessAll
#define ftComputerSetFighterDamageDetectSize \
    ndsBaseFTComputerSetFighterDamageDetectSize

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcomputer.c"

#if NDS_P4
#undef ftComputerFollowObjectiveWalk
#undef ftComputerFollowObjectiveRecover
#undef ftComputerProcessObjective
#undef ftComputerUpdateInputs
#undef func_ovl3_80138AA8
#undef func_ovl3_801346D4
#undef func_ovl3_80135B78
#undef ftComputerFollowObjectiveUseItem
#undef ftComputerProcDefault
#include <nds/nds_p4.h>

/* ftcomputer.c:4960 for a P4 content. Its fkind reads are three compares
 * (Kirby/Jigglypuff 5026, Yoshi/Jigglypuff 5133, Giant DK 5145) and its
 * callees read none, so it runs with the content's fkind past every vanilla
 * kind, as the content's own id is on the N64. */
static void ndsP4FTComputerFollowObjectiveWalk(FTStruct *fp)
{
    if (fp->nds_p4_content != 0u)
    {
        s32 fkind = fp->fkind;

        fp->fkind = NDS_P4_FOREIGN_FKIND;
        ndsBaseFTComputerFollowObjectiveWalk(fp);
        fp->fkind = fkind;
        return;
    }
    ndsBaseFTComputerFollowObjectiveWalk(fp);
}

void ftComputerFollowObjectiveWalk(FTStruct *fp)
{
    ndsP4FTComputerFollowObjectiveWalk(fp);
}

/* ftcomputer.c:4701, the recover objective's jump range: its fkind reads
 * are compares Remix leaves on the fighter's own id (Fox, DK and Giant DK
 * halve the range, Ness flips it, Metal Mario and Giant DK zero it) and it
 * calls nothing, so a content runs it with its fkind out of range. */
static void ndsP4FTComputerRecoverJumpRange(FTStruct *fp)
{
    if (fp->nds_p4_content != 0u)
    {
        s32 fkind = fp->fkind;

        fp->fkind = NDS_P4_FOREIGN_FKIND;
        ndsBaseFTComputerRecoverJumpRange(fp);
        fp->fkind = fkind;
        return;
    }
    ndsBaseFTComputerRecoverJumpRange(fp);
}

void func_ovl3_801346D4(FTStruct *fp)
{
    ndsP4FTComputerRecoverJumpRange(fp);
}

/* ftcomputer.c:5308, the incoming-attack scan: Fox and Ness flag the
 * counterattack's reflect (is_opponent_ra). Remix sends its reflectors' ids
 * Fox's way (extend_projectile_reflect_initial_) and any other content's
 * to the plain case; the scan calls nothing that reads fkind. Its item half
 * reads Character.fighter_reflect instead (extend_item_reflect_initial_),
 * which differs for Dedede alone: Remix flags his item hazards, which the
 * counterattack clears without an input (apply_reflect_input_ has no case
 * for him) and which keeps func_ovl3_801361BC from arming his item shield;
 * this port arms it. */
static sb32 ndsP4FTComputerCheckHazard(FTStruct *fp)
{
    if (fp->nds_p4_content != 0u)
    {
        s32 fkind = fp->fkind;
        sb32 result;

        fp->fkind = ndsP4ComputerReflectKind(fp);
        result = ndsBaseFTComputerCheckHazard(fp);
        fp->fkind = fkind;
        return result;
    }
    return ndsBaseFTComputerCheckHazard(fp);
}

sb32 func_ovl3_80135B78(FTStruct *fp)
{
    return ndsP4FTComputerCheckHazard(fp);
}

/* ftcomputer.c:6627, the item objective: a level-5 CPU throws a shooting
 * item at Fox or Ness rather than fire it, a test of the target's id that
 * Remix leaves unpatched, so no content target meets it. The objective
 * reads no fighter's fkind but that target's (its walk and target search
 * read only its own, and a content's own runs out of range there anyway),
 * so every content in the match is out of range while it runs. */
static void ndsP4FTComputerFollowObjectiveUseItem(FTStruct *fp)
{
    s32 kinds[GMCOMMON_PLAYERS_MAX];
    FTStruct *contents[GMCOMMON_PLAYERS_MAX];
    u32 count = 0u;
    GObj *gobj;
    u32 i;

    for (gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
         (gobj != NULL) && (count < GMCOMMON_PLAYERS_MAX); gobj = gobj->link_next)
    {
        FTStruct *other_fp = ftGetStruct(gobj);

        if (other_fp->nds_p4_content != 0u)
        {
            contents[count] = other_fp;
            kinds[count++] = other_fp->fkind;
            other_fp->fkind = NDS_P4_FOREIGN_FKIND;
        }
    }
    ndsBaseFTComputerFollowObjectiveUseItem(fp);
    for (i = 0u; i < count; i++)
    {
        contents[i]->fkind = kinds[i];
    }
}

void ftComputerFollowObjectiveUseItem(FTStruct *fp)
{
    ndsP4FTComputerFollowObjectiveUseItem(fp);
}

/* ftcomputer.c:6274, the default objective, for a P4 content: the
 * source's, but the reflector hold (a Fox or Ness CPU in its down-special
 * releases B) takes Remix's view of the fighter (maintain_reflect_input_:
 * Fox's branch for its reflectors, none for any other content), so Peach
 * and Sonic, whose own down-specials occupy Fox's status ids, keep B held.
 * ftComputerProcessObjective installs it per frame (the behavior step
 * reassigns the source's). */
static s32 ndsP4FTComputerProcDefault(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTComputer *com = &fp->computer;
    s32 process_result = ftComputerGetObjectiveStatus(fighter_gobj);

    if ((process_result == 0) || (process_result == 1))
    {
        return process_result;
    }
    if (ftComputerCheckEvadeDistance(fp) != FALSE)
    {
        com->objective = nFTComputerObjectiveEvade;
        return TRUE;
    }
    if ((fp->fkind != nFTKindMMario) && (fp->fkind != nFTKindGDonkey))
    {
        if (func_ovl3_80135B78(fp) != FALSE)
        {
            com->objective = nFTComputerObjectiveCounterAttack;
            return TRUE;
        }
    }
    switch (ndsP4ComputerReflectKind(fp))
    {
    case nFTKindFox:
    case nFTKindNFox:
        if ((fp->status_id >= nFTFoxStatusSpecialLwScopeStart) && (fp->status_id <= nFTFoxStatusSpecialLwScopeEnd))
        {
            ftComputerSetCommandWaitShort(fp, nFTComputerInputStickNButtonBRelease);
            return FALSE;
        }
        break;

    case nFTKindNess:
    case nFTKindNNess:
        if ((fp->status_id >= nFTNessStatusSpecialLwScopeStart) && (fp->status_id <= nFTNessStatusSpecialLwScopeEnd))
        {
            ftComputerSetCommandWaitShort(fp, nFTComputerInputStickNButtonBRelease);
            return FALSE;
        }
        break;
    }
    if ((fp->status_id >= nFTCommonStatusGuardStart) && (fp->status_id <= nFTCommonStatusGuardEnd))
    {
        ftComputerSetCommandWaitShort(fp, 0x24);
        return FALSE;
    }
    else ftComputerCheckFindTarget(fp);

    if (com->target_dist < 350.0F)
    {
        com->objective = nFTComputerObjectiveAttack;
        return TRUE;
    }
    else if (ftComputerCheckFindItem(fp) != FALSE)
    {
        if (com->target_dist < (400.0F * (fp->level + 3)))
        {
            s32 track_wait = (gSCManagerBattleState->game_type == nSCBattleGameType1PGame) ? (-fp->level * 35) + 315 : (-fp->level * 25) + 225;

            com->item_track_wait++;

            if (track_wait < com->item_track_wait)
            {
                com->objective = nFTComputerObjectiveTrackItem;
                return TRUE;
            }
        }
    }
    else com->item_track_wait = 0;

    if (fp->item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(fp->item_gobj);

        switch (ip->type)
        {
        case nITTypeDamage:
        case nITTypeShoot:
        case nITTypeThrow:
            com->objective = nFTComputerObjectiveUseItem;
            return TRUE;

        default:
            com->objective = nFTComputerObjectiveAttack;
            return TRUE;
        }
    }
    else com->objective = com->objective_base;
    return TRUE;
}

s32 ftComputerProcDefault(GObj *fighter_gobj)
{
    return ndsBaseFTComputerProcDefault(fighter_gobj);
}

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
    if ((fp->nds_p4_content != 0u) &&
        (fp->computer.proc_com == ndsBaseFTComputerProcDefault))
    {
        fp->computer.proc_com = ndsP4FTComputerProcDefault;
    }
    ndsBaseFTComputerProcessObjective(fp);
    ndsP4ComputerPostProcess(fp);
}

void ftComputerProcessObjective(FTStruct *fp)
{
    ndsP4FTComputerProcessObjective(fp);
}

/* The input interpreter; a P4 content runs the port's copy with Remix's
 * extensions (ndsP4ComputerRunInputs). */
static void ndsP4FTComputerUpdateInputs(FTStruct *fp)
{
    if (ndsP4ComputerRunInputs(fp) == FALSE)
    {
        ndsBaseFTComputerUpdateInputs(fp);
    }
}

void ftComputerUpdateInputs(FTStruct *this_fp)
{
    ndsP4FTComputerUpdateInputs(this_fp);
}

/* func_ovl3_80138AA8, the long-range special. Remix's ai_long_range row
 * sends a content to one of two cases of the source's fkind switch, after
 * the source's lead-in (its two random draws: the reaction delay and the
 * reflector-target roll): 0x80138ECC returns FALSE, 0x80138D24 is the
 * projectile users' walk and shoot, the source's case copied below. A
 * content keeping its parent's case runs the source. */
static sb32 ndsP4FTComputerLongRange(FTStruct *this_fp, sb32 is_delay)
{
    u32 mode = ndsP4ComputerLongRange(this_fp);
    FTComputer *com = &this_fp->computer;
    FTStruct *target_fp = com->target_user;
    Vec3f pos;
    Vec3f ga_last;
    s32 stand_line_id;
    s32 fkind;

    if (mode == NDS_P4_COMPUTER_LONG_RANGE_PARENT)
    {
        /* The source's reflector-target roll tests the target for Fox and
         * Ness; Remix's (improve_remix_charged_NSP_2) names Fox's reflector
         * users among the contents, so a content target is seen as its
         * reflect kind while the source runs. */
        if ((target_fp != NULL) && (target_fp->nds_p4_content != 0u))
        {
            s32 target_fkind = target_fp->fkind;
            sb32 result;

            target_fp->fkind = ndsP4ComputerReflectKind(target_fp);
            result = ndsBaseFTComputerLongRange(this_fp, is_delay);
            target_fp->fkind = target_fkind;
            return result;
        }
        return ndsBaseFTComputerLongRange(this_fp, is_delay);
    }
    if (DISTANCE(this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y, com->target_pos.y) >= 400.0F)
    {
        return FALSE;
    }
    if (com->unk_ftcom_0x35 == 0)
    {
        com->unk_ftcom_0x35 = 2.0F * (syUtilsRandFloat() * (FTCOMPUTER_LEVEL_MAX - this_fp->level));
    }
    if ((syUtilsRandFloat() < ((this_fp->level - 1) / 9.0F)) && (target_fp != NULL))
    {
        /* improve_remix_charged_NSP_2: Fox's reflector users hold the shot;
         * Ness's absorbers too, unless the shooter is Sheik. */
        s32 target_kind = ndsP4ComputerReflectKind(target_fp);

        if (target_kind == nFTKindFox)
        {
            return FALSE;
        }
        if (target_kind == nFTKindNess)
        {
#if NDS_P4_SHEIK
            if (ndsP4Content(this_fp) != NDS_P4_CONTENT_SHEIK)
#endif
            {
                return FALSE;
            }
        }
    }
    if (mode == NDS_P4_COMPUTER_LONG_RANGE_NONE)
    {
        return FALSE;
    }
    fkind = (this_fp->fkind == nFTKindKirby) ? this_fp->passive_vars.kirby.copy_id : this_fp->fkind;

    if ((fkind == nFTKindSamus) &&
        ((this_fp->status_id == nFTSamusStatusSpecialNStart) ||
         (this_fp->status_id == nFTSamusStatusSpecialAirNStart) ||
         (this_fp->status_id == nFTSamusStatusSpecialNLoop) ||
         (this_fp->status_id == nFTSamusStatusSpecialAirNEnd)))
    {
        return FALSE;
    }
    com->unk_ftcom_0x35++;

    if (com->unk_ftcom_0x35 >= 5)
    {
        ftComputerFollowObjectiveWalk(this_fp);
        return TRUE;
    }
    pos = this_fp->joints[nFTPartsJointTopN]->translate.vec.f;
    pos.x = com->target_pos.x;
    pos.y = com->target_pos.y;

    if (com->target_pos.x < this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x)
    {
        if ((mpCollisionCheckRWallLineCollisionSame(&this_fp->joints[nFTPartsJointTopN]->translate.vec.f, &pos, &ga_last, &stand_line_id, NULL, NULL) != FALSE) ||
            (mpCollisionCheckFloorLineCollisionSame(&this_fp->joints[nFTPartsJointTopN]->translate.vec.f, &pos, &ga_last, &stand_line_id, NULL, NULL) != FALSE) ||
            (mpCollisionCheckCeilLineCollisionSame(&this_fp->joints[nFTPartsJointTopN]->translate.vec.f, &pos, &ga_last, &stand_line_id, NULL, NULL) != FALSE))
        {
            return FALSE;
        }
    }
    else if ((mpCollisionCheckLWallLineCollisionSame(&this_fp->joints[nFTPartsJointTopN]->translate.vec.f, &pos, &ga_last, &stand_line_id, NULL, NULL) != FALSE) ||
             (mpCollisionCheckFloorLineCollisionSame(&this_fp->joints[nFTPartsJointTopN]->translate.vec.f, &pos, &ga_last, &stand_line_id, NULL, NULL) != FALSE) ||
             (mpCollisionCheckCeilLineCollisionSame(&this_fp->joints[nFTPartsJointTopN]->translate.vec.f, &pos, &ga_last, &stand_line_id, NULL, NULL) != FALSE))
    {
        return FALSE;
    }
    if (is_delay == FALSE)
    {
        ftComputerSetCommandWaitLong(this_fp, nFTComputerInputStickSmashAutoXButtonB);
    }
    else ftComputerSetCommandWaitLong(this_fp, nFTComputerInputStickTiltAutoXNYD5SmashAutoXButtonB);

    return TRUE;
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
