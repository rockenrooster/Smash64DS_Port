/*
 * P4: the source's jab and rapid-jab files (ftcommonattack1.c,
 * ftcommonattack100.c) compiled a second time, for P4 contents.
 *
 * On the N64 a Remix fighter's character id matches none of the vanilla
 * kinds these files test, except where Remix replaced a kind test with a
 * table indexed by the id (Character.asm jab_3, jab_3_timer, jab_3_action,
 * rapid_jab and the rapid jab's four action rows). A content here carries
 * its parent's fkind, so this copy redefines every kind the files name past
 * every real kind: each raw test fails and each switch takes its default,
 * as the content's own id did (Remix left the jab-3 test in
 * ftCommonAttack1CheckInterruptCommon and the Captain tests of the
 * Attack12/13 updates alone). The seven functions Remix tabled are this
 * file's own versions below, reading the content's generated rows
 * (nds_p4.h NDSP4Jab). The source's copies (battleship_ftcommon_attack1.c,
 * battleship_ftcommon_attack100.c) send a content to these entry points.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <it/item.h>
#include <sys/obj.h>
#include <nds/nds_p4_contents.h>

#if NDS_P4
#include <nds/nds_p4.h>

/* Past FTKind (it ends at nFTKindGDonkey) and NDS_P4_FOREIGN_FKIND, and
 * distinct, so the switches keep distinct cases. */
#define nFTKindMario 0x80
#define nFTKindMMario 0x81
#define nFTKindNMario 0x82
#define nFTKindLuigi 0x83
#define nFTKindNLuigi 0x84
#define nFTKindCaptain 0x85
#define nFTKindNCaptain 0x86
#define nFTKindLink 0x87
#define nFTKindNLink 0x88
#define nFTKindNess 0x89
#define nFTKindNNess 0x8A
#define nFTKindPikachu 0x8B
#define nFTKindNPikachu 0x8C
#define nFTKindFox 0x8D
#define nFTKindNFox 0x8E
#define nFTKindKirby 0x8F
#define nFTKindNKirby 0x90
#define nFTKindPurin 0x91
#define nFTKindNPurin 0x92

/* Every source function as ndsP4Jab<Name> (nds_p4.h). */
#define ftCommonAttack11ProcUpdate ndsP4JabAttack11ProcUpdate
#define ftCommonAttack12ProcUpdate ndsP4JabAttack12ProcUpdate
#define ftCommonAttack13ProcUpdate ndsP4JabAttack13ProcUpdate
#define ftCommonAttack11ProcInterrupt ndsP4JabAttack11ProcInterrupt
#define ftCommonAttack12ProcInterrupt ndsP4JabAttack12ProcInterrupt
#define ftCommonAttack13ProcInterrupt ndsP4JabAttack13ProcInterrupt
#define ftCommonAttack11ProcStatus ndsP4JabAttack11ProcStatus
#define ftCommonAttack11SetStatus ndsP4JabAttack11SetStatus
#define ftCommonAttack1CheckInterruptCommon ndsP4JabAttack1CheckInterruptCommon
#define ftCommonAttack11CheckGoto ndsP4JabAttack11CheckGoto
#define ftCommonAttack12CheckGoto ndsP4JabAttack12CheckGoto
#define ftCommonAttack100StartProcUpdate ndsP4JabAttack100StartProcUpdate
#define ftCommonAttack100LoopKirbyUpdateEffect \
    ndsP4JabAttack100LoopKirbyUpdateEffect
#define ftCommonAttack100LoopProcUpdate ndsP4JabAttack100LoopProcUpdate
#define ftCommonAttack100LoopProcInterrupt ndsP4JabAttack100LoopProcInterrupt

/* The seven Remix tabled: the source's definition (its first token GObj)
 * takes an unused ndsP4JabSource name, and every call, always
 * `(fighter_gobj)`, reaches this file's version. The bare name
 * ftCommonAttack100LoopSetStatus (a callback, ftcommonattack100.c:33) stays
 * the public one, which sends a content here too. */
#define NDS_P4_JAB_A12SET_GObj ndsP4JabSourceAttack12SetStatus(GObj
#define NDS_P4_JAB_A12SET_fighter_gobj ndsP4JabAttack12SetStatus(fighter_gobj
#define ftCommonAttack12SetStatus(arg_) NDS_P4_JAB_A12SET_##arg_)
#define NDS_P4_JAB_A13SET_GObj ndsP4JabSourceAttack13SetStatus(GObj
#define NDS_P4_JAB_A13SET_fighter_gobj ndsP4JabAttack13SetStatus(fighter_gobj
#define ftCommonAttack13SetStatus(arg_) NDS_P4_JAB_A13SET_##arg_)
#define NDS_P4_JAB_A13GOTO_GObj ndsP4JabSourceAttack13CheckGoto(GObj
#define NDS_P4_JAB_A13GOTO_fighter_gobj ndsP4JabAttack13CheckGoto(fighter_gobj
#define ftCommonAttack13CheckGoto(arg_) NDS_P4_JAB_A13GOTO_##arg_)
#define NDS_P4_JAB_START_GObj ndsP4JabSourceAttack100StartSetStatus(GObj
#define NDS_P4_JAB_START_fighter_gobj \
    ndsP4JabAttack100StartSetStatus(fighter_gobj
#define ftCommonAttack100StartSetStatus(arg_) NDS_P4_JAB_START_##arg_)
#define NDS_P4_JAB_LOOP_GObj ndsP4JabSourceAttack100LoopSetStatus(GObj
#define NDS_P4_JAB_LOOP_fighter_gobj ndsP4JabAttack100LoopSetStatus(fighter_gobj
#define ftCommonAttack100LoopSetStatus(arg_) NDS_P4_JAB_LOOP_##arg_)
#define NDS_P4_JAB_END_GObj ndsP4JabSourceAttack100EndSetStatus(GObj
#define NDS_P4_JAB_END_fighter_gobj ndsP4JabAttack100EndSetStatus(fighter_gobj
#define ftCommonAttack100EndSetStatus(arg_) NDS_P4_JAB_END_##arg_)
#define NDS_P4_JAB_RAPID_GObj \
    ndsP4JabSourceAttack100StartCheckInterruptCommon(GObj
#define NDS_P4_JAB_RAPID_fighter_gobj \
    ndsP4JabAttack100StartCheckInterruptCommon(fighter_gobj
#define ftCommonAttack100StartCheckInterruptCommon(arg_) \
    NDS_P4_JAB_RAPID_##arg_)

/* battleship_ftcommon_attack100.c's Kirby effect offset. */
#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llKirbyMainMotionftKirbyAttack100Effect NDS_RELOC_LVALUE(0x1220u)

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattack1.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattack100.c"

#undef ftCommonAttack12SetStatus
#undef ftCommonAttack13SetStatus
#undef ftCommonAttack13CheckGoto
#undef ftCommonAttack100StartSetStatus
#undef ftCommonAttack100LoopSetStatus
#undef ftCommonAttack100EndSetStatus
#undef ftCommonAttack100StartCheckInterruptCommon

/* ftcommonattack1.c:152, jab_3_timer's row in place of the kind switch
 * (every vanilla case stores 24 frames; Lanky's routine 42). */
void ndsP4JabAttack12SetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 followup = ndsP4JabRows(fp)->jab3_followup;

    if (ftCommonGetCheckInterruptCommon(fighter_gobj) == FALSE)
    {
        ftMainSetStatus(fighter_gobj, nFTCommonStatusAttack12, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
        ftMainPlayAnimEventsAll(fighter_gobj);

        fp->motion_vars.flags.flag1 = 0;

        fp->status_vars.common.attack1.is_goto_followup = FALSE;

        fp->attack1_status_id = fp->status_id;

        if (followup != 0.0F)
        {
            fp->attack1_followup_frames = followup;
        }
    }
}

/* ftcommonattack1.c:199, jab_3_action's row (Bowser's, Banjo's and
 * Dedede's own third jabs). Its DISABLED row sets no status here. */
void ndsP4JabAttack13SetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = ndsP4JabRows(fp)->jab3_status;

    if (ftCommonGetCheckInterruptCommon(fighter_gobj) == FALSE)
    {
        if (status_id == 0)
        {
            return;
        }
        ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
        ftMainPlayAnimEventsAll(fighter_gobj);

        fp->motion_vars.flags.flag1 = 0;
        fp->status_vars.common.attack1.is_goto_followup = FALSE;
        fp->attack1_status_id = fp->status_id;
    }
}

/* ftcommonattack1.c:376, jab_3's row in place of the kind test. */
sb32 ndsP4JabAttack13CheckGoto(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ndsP4JabRows(fp)->jab3 == FALSE)
    {
        return FALSE;
    }
    if (fp->attack1_followup_frames != 0.0F)
    {
        fp->attack1_followup_frames -= DObjGetStruct(fighter_gobj)->anim_speed;

        if (fp->input.pl.button_tap & fp->input.button_mask_a)
        {
            if (fp->motion_vars.flags.flag1 != 0)
            {
                ndsP4JabAttack13SetStatus(fighter_gobj);

                return TRUE;
            }
            fp->status_vars.common.attack1.is_goto_followup = TRUE;
        }
    }
    return FALSE;
}

/* ftcommonattack100.c:37, rapid_jab_begin_action's row. */
void ndsP4JabAttack100StartSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = ndsP4JabRows(fp)->rapid_start;

    if (ftCommonGetCheckInterruptCommon(fighter_gobj) == FALSE)
    {
        if (status_id == 0)
        {
            return;
        }
        ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
        ftMainPlayAnimEventsAll(fighter_gobj);

        fp->status_vars.common.attack100.is_anim_end = FALSE;
        fp->status_vars.common.attack100.is_goto_loop = FALSE;

        fp->motion_vars.flags.flag1 = 0;
        fp->motion_vars.flags.flag2 = 0;
    }
}

/* ftcommonattack100.c:150, rapid_jab_loop_action's row. */
void ndsP4JabAttack100LoopSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = ndsP4JabRows(fp)->rapid_loop;

    if (status_id == 0)
    {
        return;
    }
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ndsP4JabAttack100LoopKirbyUpdateEffect(fp);
}

/* ftcommonattack100.c:187, rapid_jab_ending_action's row. */
void ndsP4JabAttack100EndSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = ndsP4JabRows(fp)->rapid_end;

    if (status_id == 0)
    {
        return;
    }
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

/* ftcommonattack100.c:223, rapid_jab's row in place of the kind test and
 * rapid_jab_unknown's (how many presses, counted in which status) in place
 * of the kind switch. Its DISABLED row starts nothing here (the N64 reads
 * the count and status from stale stack words). */
sb32 ndsP4JabAttack100StartCheckInterruptCommon(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Jab *jab = ndsP4JabRows(fp);
    s32 status_id;
    s32 inputs_min;

    if (jab->rapid == FALSE)
    {
        return FALSE;
    }
    if ((fp->input.pl.button_tap & fp->input.button_mask_a) || (fp->input.pl.button_release & fp->input.button_mask_a))
    {
        fp->attack1_input_count++;

        inputs_min = jab->rapid_inputs;
        status_id = jab->rapid_count_status;

        if ((inputs_min != 0) && (fp->attack1_input_count >= inputs_min))
        {
            if ((status_id == fp->status_id) && (fp->motion_vars.flags.flag1 != 0))
            {
                ndsP4JabAttack100StartSetStatus(fighter_gobj);

                return TRUE;
            }
            else fp->is_goto_attack100 = TRUE;
        }
    }
    return FALSE;
}

#endif /* NDS_P4 */
