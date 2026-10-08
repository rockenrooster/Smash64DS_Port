/*
 * Fenced BattleShip normal-moveset imports for the Mario/Fox tilt, smash,
 * and aerial-interrupt slice. Public symbols are remapped so existing port
 * seams can route to the original exactly once while the fence is off by
 * default.
 */
#include <PR/ultratypes.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <ft/ftdata_file_slots.h>
#include <it/item.h>
#include <reloc_data.h>
#include <sys/obj.h>
#include <nds/nds_p4_contents.h>
#if NDS_P4
#include <nds/nds_p4.h>
#endif

sb32 itMainCheckShootNoAmmo(GObj *item_gobj);
#if !NDS_P2_ITEM_CORE
/* The real body (BattleShip it/itmain.c:281) lives in
 * battleship_item_link_core.c, which only configurations with the item core
 * link. Without items no fighter can hold a shoot item, so the source branch
 * is unreachable and the answer is always "not spent". Weak so the real body
 * wins wherever it is linked. */
__attribute__((weak)) sb32 itMainCheckShootNoAmmo(GObj *item_gobj)
{
    (void)item_gobj;
    return FALSE;
}
#endif
void func_ovl2_800EE018(DObj *main_dobj, Vec3f *vec);
void ndsBaseFTCommonSquatWaitSetStatus(GObj *fighter_gobj);

__attribute__((weak)) uintptr_t llNessMainMotionAttackS4ReflectorFTSpecialColl;

__attribute__((weak)) GObj *
efManagerPikachuThunderShockMakeEffect(GObj *fighter_gobj, Vec3f *pos,
                                       s32 frame)
{
    (void)fighter_gobj;
    (void)pos;
    (void)frame;
    return NULL;
}

__attribute__((weak)) void ftParamProcPauseEffect(GObj *effect_gobj)
{
    (void)effect_gobj;
}

__attribute__((weak)) void ftParamProcResumeEffect(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

#define ftCommonAttackS3SetStatus ndsBaseFTCommonAttackS3SetStatus
#define ftCommonAttackS3CheckInterruptCommon \
    ndsBaseFTCommonAttackS3CheckInterruptCommon

void ndsBaseFTCommonAttackS3SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS3CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattacks3.c"

#undef ftCommonAttackS3SetStatus
#undef ftCommonAttackS3CheckInterruptCommon

#define ftCommonAttackHi3SetStatus ndsBaseFTCommonAttackHi3SetStatus
#define ftCommonAttackHi3CheckInterruptCommon \
    ndsBaseFTCommonAttackHi3CheckInterruptCommon

void ndsBaseFTCommonAttackHi3SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackHi3CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattackhi3.c"

#undef ftCommonAttackHi3SetStatus
#undef ftCommonAttackHi3CheckInterruptCommon

#define ftCommonAttackLw3ProcUpdate ndsBaseFTCommonAttackLw3ProcUpdate
#define ftCommonAttackLw3ProcInterrupt \
    ndsBaseFTCommonAttackLw3ProcInterrupt
#define ftCommonAttackLw3CheckInterruptSelf \
    ndsBaseFTCommonAttackLw3CheckInterruptSelf
#define ftCommonAttackLw3InitStatusVars \
    ndsBaseFTCommonAttackLw3InitStatusVars
#define ftCommonAttackLw3SetStatus ndsBaseFTCommonAttackLw3SetStatus
#define ftCommonAttackLw3CheckInterruptCommon \
    ndsBaseFTCommonAttackLw3CheckInterruptCommon
#define ftCommonSquatWaitSetStatus ndsBaseFTCommonSquatWaitSetStatus

void ndsBaseFTCommonAttackLw3ProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttackLw3ProcInterrupt(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackLw3CheckInterruptSelf(GObj *fighter_gobj);
void ndsBaseFTCommonAttackLw3InitStatusVars(GObj *fighter_gobj);
void ndsBaseFTCommonAttackLw3SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackLw3CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattacklw3.c"

#undef ftCommonAttackLw3ProcUpdate
#undef ftCommonAttackLw3ProcInterrupt
#undef ftCommonAttackLw3CheckInterruptSelf
#undef ftCommonAttackLw3InitStatusVars
#undef ftCommonAttackLw3SetStatus
#undef ftCommonAttackLw3CheckInterruptCommon
#undef ftCommonSquatWaitSetStatus

#define ftCommonAttackS4ProcUpdate ndsBaseFTCommonAttackS4ProcUpdate
#define ftCommonAttackS4SetStatus ndsBaseFTCommonAttackS4SetStatus
#define ftCommonAttackS4CheckInterruptDash \
    ndsBaseFTCommonAttackS4CheckInterruptDash
#define ftCommonAttackS4CheckInterruptTurn \
    ndsBaseFTCommonAttackS4CheckInterruptTurn
#define ftCommonAttackS4CheckInterruptCommon \
    ndsBaseFTCommonAttackS4CheckInterruptCommon

void ndsBaseFTCommonAttackS4ProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttackS4SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS4CheckInterruptDash(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS4CheckInterruptTurn(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS4CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattacks4.c"

#undef ftCommonAttackS4ProcUpdate
#undef ftCommonAttackS4SetStatus
#undef ftCommonAttackS4CheckInterruptDash
#undef ftCommonAttackS4CheckInterruptTurn
#undef ftCommonAttackS4CheckInterruptCommon

#define ftCommonAttackHi4SetStatus ndsBaseFTCommonAttackHi4SetStatus
#define ftCommonAttackHi4CheckInputSuccess \
    ndsBaseFTCommonAttackHi4CheckInputSuccess
#define ftCommonAttackHi4CheckInterruptMain \
    ndsBaseFTCommonAttackHi4CheckInterruptMain
#define ftCommonAttackHi4CheckInterruptKneeBend \
    ndsBaseFTCommonAttackHi4CheckInterruptKneeBend
#define ftCommonAttackHi4CheckInterruptCommon \
    ndsBaseFTCommonAttackHi4CheckInterruptCommon

void ndsBaseFTCommonAttackHi4SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackHi4CheckInputSuccess(FTStruct *fp);
sb32 ndsBaseFTCommonAttackHi4CheckInterruptMain(FTStruct *fp);
sb32 ndsBaseFTCommonAttackHi4CheckInterruptKneeBend(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackHi4CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattackhi4.c"

#undef ftCommonAttackHi4SetStatus
#undef ftCommonAttackHi4CheckInputSuccess
#undef ftCommonAttackHi4CheckInterruptMain
#undef ftCommonAttackHi4CheckInterruptKneeBend
#undef ftCommonAttackHi4CheckInterruptCommon

#define ftCommonAttackLw4SetStatus ndsBaseFTCommonAttackLw4SetStatus
#define ftCommonAttackLw4CheckInputSuccess \
    ndsBaseFTCommonAttackLw4CheckInputSuccess
#define ftCommonAttackLw4CheckInterruptMain \
    ndsBaseFTCommonAttackLw4CheckInterruptMain
#define ftCommonAttackLw4CheckInterruptSquat \
    ndsBaseFTCommonAttackLw4CheckInterruptSquat
#define ftCommonAttackLw4CheckInterruptCommon \
    ndsBaseFTCommonAttackLw4CheckInterruptCommon

void ndsBaseFTCommonAttackLw4SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackLw4CheckInputSuccess(FTStruct *fp);
sb32 ndsBaseFTCommonAttackLw4CheckInterruptMain(FTStruct *fp);
sb32 ndsBaseFTCommonAttackLw4CheckInterruptSquat(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackLw4CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattacklw4.c"

#undef ftCommonAttackLw4SetStatus
#undef ftCommonAttackLw4CheckInputSuccess
#undef ftCommonAttackLw4CheckInterruptMain
#undef ftCommonAttackLw4CheckInterruptSquat
#undef ftCommonAttackLw4CheckInterruptCommon

#define ftCommonJumpAerialUpdateModelYaw \
    ndsBaseFTCommonJumpAerialUpdateModelYaw
#define ftCommonJumpAerialProcUpdate ndsBaseFTCommonJumpAerialProcUpdate
#define ftCommonJumpAerialProcInterrupt \
    ndsBaseFTCommonJumpAerialProcInterrupt
#define ftYoshiJumpAerialProcPhysics ndsBaseFTYoshiJumpAerialProcPhysics
#define ftNessJumpAerialProcPhysics ndsBaseFTNessJumpAerialProcPhysics
#define ftCommonJumpAerialProcPhysics \
    ndsBaseFTCommonJumpAerialProcPhysics
#define ftCommonJumpAerialSetStatus ndsBaseFTCommonJumpAerialSetStatus
#define ftCommonJumpAerialMultiSetStatus \
    ndsBaseFTCommonJumpAerialMultiSetStatus
#define ftCommonJumpAerialMultiCheckJumpButtonHold \
    ndsBaseFTCommonJumpAerialMultiCheckJumpButtonHold
#define ftCommonJumpAerialMultiGetJumpInputType \
    ndsBaseFTCommonJumpAerialMultiGetJumpInputType
#define ftCommonJumpAerialCheckInterruptCommon \
    ndsBaseFTCommonJumpAerialCheckInterruptCommon

void ndsBaseFTCommonJumpAerialUpdateModelYaw(FTStruct *fp);
void ndsBaseFTCommonJumpAerialProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonJumpAerialProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTYoshiJumpAerialProcPhysics(GObj *fighter_gobj);
void ndsBaseFTNessJumpAerialProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonJumpAerialProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonJumpAerialSetStatus(GObj *fighter_gobj,
                                        s32 input_source);
void ndsBaseFTCommonJumpAerialMultiSetStatus(GObj *fighter_gobj,
                                             s32 input_source);
sb32 ndsBaseFTCommonJumpAerialMultiCheckJumpButtonHold(FTStruct *fp);
s32 ndsBaseFTCommonJumpAerialMultiGetJumpInputType(FTStruct *fp);
sb32 ndsBaseFTCommonJumpAerialCheckInterruptCommon(GObj *fighter_gobj);

/* P4: Yoshi's double jump (its physics, armour and turn). Remix extends the
 * compares to J Yoshi only (yoshi_dj_fix_1, joshi_armor), so a content
 * aliasing Yoshi (Bowser) jumps like everyone else (nds_p4.h
 * NDS_P4_PARENT_KIND). */
#if NDS_P4
#define nFTKindYoshi NDS_P4_PARENT_KIND(fp, nFTKindYoshi)
/* P4: Remix's check_float_ (PeachSpecial.asm) runs in place of the
 * JumpAerial interrupt's jump check, after the aerial check: it is tried
 * where the aerial check ends (ndsP4AirJumpCheck). */
#define ftCommonAttackAirCheckInterruptCommon(g_) \
    (ftCommonAttackAirCheckInterruptCommon(g_) || ndsP4AirJumpCheck(g_))
/* P4: Remix's jump_fix_1-5 (jigglypuffkirbyshared.asm) take Dedede's id
 * down Kirby's multi-jump branches of the jump check, the multi-jump status
 * and the aerial physics, with his own heights. A content keeps its
 * parent's kind, so these three source bodies (and the interrupt, the
 * check's one caller in this file) are renamed; the routines under the
 * port's names below run them, or the Kirby branch for a content whose
 * overrides carry multi_jump_vel. */
#undef ftCommonJumpAerialProcInterrupt
#define ftCommonJumpAerialProcInterrupt ndsSrcFTCommonJumpAerialProcInterrupt
#undef ftCommonJumpAerialProcPhysics
#define ftCommonJumpAerialProcPhysics ndsSrcFTCommonJumpAerialProcPhysics
#undef ftCommonJumpAerialCheckInterruptCommon
#define ftCommonJumpAerialCheckInterruptCommon ndsSrcFTCommonJumpAerialCheckInterruptCommon
void ndsSrcFTCommonJumpAerialProcInterrupt(GObj *fighter_gobj);
void ndsSrcFTCommonJumpAerialProcPhysics(GObj *fighter_gobj);
sb32 ndsSrcFTCommonJumpAerialCheckInterruptCommon(GObj *fighter_gobj);
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonjumpaerial.c"
#if NDS_P4
#undef nFTKindYoshi
#undef ftCommonAttackAirCheckInterruptCommon
#endif

#undef ftCommonJumpAerialUpdateModelYaw
#undef ftCommonJumpAerialProcUpdate
#undef ftCommonJumpAerialProcInterrupt
#undef ftYoshiJumpAerialProcPhysics
#undef ftNessJumpAerialProcPhysics
#undef ftCommonJumpAerialProcPhysics
#undef ftCommonJumpAerialSetStatus
#undef ftCommonJumpAerialMultiSetStatus
#undef ftCommonJumpAerialMultiCheckJumpButtonHold
#undef ftCommonJumpAerialMultiGetJumpInputType
#undef ftCommonJumpAerialCheckInterruptCommon

#if NDS_P4
/* ftCommonJumpAerialMultiSetStatus' Kirby case (decomp 0x8013FF38) with the
 * content's heights (jump_fix_1 and jump_fix_2). Remix's Size.asm patch on
 * the first jump's height (adjust_jumping_height_multiplier_kirby) leaves
 * the player's port times 4 in the stick buffer where the source writes its
 * maximum. */
static void ndsP4FTCommonJumpAerialMultiSetStatus(GObj *fighter_gobj, s32 input_source,
                                                  const f32 *vel)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    s32 stick_range_x;
    s32 stick_range_y = I_CONTROLLER_RANGE_MAX;

    (void)input_source;

    ftMainSetStatus(fighter_gobj, fp->jumps_used + nFTKirbyStatusJumpAerialF1 - 1, 0.0F, 1.0F,
                    FTSTATUS_PRESERVE_PLAYERTAG);

    stick_range_x = fp->input.pl.stick_range.x;

    fp->physics.vel_air.x = stick_range_x * attr->jumpaerial_vel_x;

    if (fp->jumps_used == 1)
    {
        fp->physics.vel_air.y = (((stick_range_y * attr->jump_height_mul) + attr->jump_height_base) * attr->jumpaerial_height);

        fp->tap_stick_y = fp->player * 4;
    }
    else fp->physics.vel_air.y = vel[fp->jumps_used - 2] * (stick_range_y / F_CONTROLLER_RANGE_MAX);

    fp->jumps_used++;

    fp->motion_vars.flags.flag1 = 0;

    fp->is_special_interrupt = TRUE;

    if ((fp->input.pl.stick_range.x * fp->lr) < FTCOMMON_JUMPAERIAL_TURN_STICK_RANGE_MIN)
    {
        fp->status_vars.common.jumpaerial.turn_tics = FTCOMMON_JUMPAERIAL_TURN_FRAMES;
    }
    else fp->status_vars.common.jumpaerial.turn_tics = 0;

    ndsBaseFTCommonJumpAerialUpdateModelYaw(fp);
}

/* ftCommonJumpAerialCheckInterruptCommon (decomp 0x8014019C): its Kirby
 * branch for a multi-jump content (jump_fix_3 and jump_fix_5). */
sb32 ndsBaseFTCommonJumpAerialCheckInterruptCommon(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const f32 *vel = ndsP4MultiJumpVelocities(fp);
    s32 input_source;

    if (vel == NULL)
    {
        return ndsSrcFTCommonJumpAerialCheckInterruptCommon(fighter_gobj);
    }
    if (ftHammerCheckHoldHammer(fighter_gobj) != FALSE)
    {
        return FALSE;
    }
    if (fp->jumps_used < fp->attr->jumps_max)
    {
        if (fp->jumps_used == 1)
        {
            input_source = ftCommonKneeBendGetInputTypeCommon(fp);

            if (input_source != FTCOMMON_JUMPAERIAL_INPUT_TYPE_NONE)
            {
                ndsP4FTCommonJumpAerialMultiSetStatus(fighter_gobj, input_source, vel);

                return TRUE;
            }
        }
        else if ((fp->status_id < nFTKirbyStatusJumpAerialF1) || (fp->status_id > nFTKirbyStatusJumpAerialF5) || (fp->motion_vars.flags.flag1 != 0))
        {
            input_source = ndsBaseFTCommonJumpAerialMultiGetJumpInputType(fp);

            if (input_source != FTCOMMON_JUMPAERIAL_INPUT_TYPE_NONE)
            {
                ndsP4FTCommonJumpAerialMultiSetStatus(fighter_gobj, input_source, vel);

                return TRUE;
            }
        }
    }
    return FALSE;
}

/* ftCommonJumpAerialProcInterrupt (decomp 0x8013FB2C), as compiled above
 * (with Peach's float check), calling the check above. */
void ndsBaseFTCommonJumpAerialProcInterrupt(GObj *fighter_gobj)
{
    if ((ftCommonSpecialAirCheckInterruptCommon(fighter_gobj) == FALSE) &&
        ((ftCommonAttackAirCheckInterruptCommon(fighter_gobj) || ndsP4AirJumpCheck(fighter_gobj)) == FALSE))
    {
        ndsBaseFTCommonJumpAerialCheckInterruptCommon(fighter_gobj);
    }
}

/* ftCommonJumpAerialProcPhysics (decomp 0x8013FC4C): its Kirby case for a
 * multi-jump content (jump_fix_4). */
void ndsBaseFTCommonJumpAerialProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    if (ndsP4MultiJumpVelocities(fp) == NULL)
    {
        ndsSrcFTCommonJumpAerialProcPhysics(fighter_gobj);
        return;
    }
    ftPhysicsCheckSetFastFall(fp);

    (fp->is_fastfall) ? ftPhysicsApplyFastFall(fp, attr) : ftPhysicsApplyGravityDefault(fp, attr);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsClampAirVelXStickRange(fp, FTPHYSICS_AIRDRIFT_CLAMP_RANGE_MIN, attr->air_accel * FTKIRBY_JUMPAERIAL_VEL_MUL, attr->air_speed_max_x * FTKIRBY_JUMPAERIAL_VEL_MUL);
    }
    ftPhysicsApplyAirVelXFriction(fp, attr);
}
#endif
