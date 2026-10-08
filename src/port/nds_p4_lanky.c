/*
 * P4 Lanky: native ports of the donor's special routines and of the patch
 * Remix makes on his character id. Source: JSsixtyfour/smashremix 5e04fe7,
 * src/Lanky/LankySpecial.asm and Lanky.asm, read as assembled
 * (scripts/p4/mipsdis.py): the OS.copy_segment blocks are the original
 * game's code. Lanky runs Mario's status code (fp->fkind == nFTKindMario).
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x044 direction                        lr
 *   0x054/0x058 knockback velocity         physics.vel_damage_air.x/y
 *   0x0EC/0x144 floor line, ignored line   coll_data.floor_line_id,
 *                                          coll_data.ignore_line_id
 *   0x14C kinetic state                    ga
 *   0x17C/0x180/0x184 temp variables 1-3   motion_vars.flags.flag0/1/2
 *   0x1B8/0x1BA shield, taunt masks        input.button_mask_z/l
 *   0x1BE/0x1C0 buttons tapped, released   input.pl.button_tap/release
 *   0x1C2/0x1C3 stick x/y                  input.pl.stick_range.x/y
 *   0x268/0x269 stick tap timers           tap_stick_x/y
 *   0xB18-0xB24                            status_vars, words 0-3: the
 *                                          balloon's timer; the grape ammo
 *                                          (word 2); the handstand jump's
 *                                          force, frame, input type and
 *                                          short hop; the cancel's buffered
 *                                          taps, tap timers and jump flag
 *   attributes + 0x34-0x40/0x64            kneebend_anim_length, jump_vel_x,
 *                                          jump_height_mul/base, jumps_max
 *
 * S6 owns his grape (the Grape Shooter's projectile, grape_stage_setting_)
 * and his entry barrel (dkshared.asm barrel_alternate). The 1P mode's taunt
 * bonus for his handstand taunt is not VS.
 */
#include <nds/nds_p4.h>

#if NDS_P4_LANKY

#include <macros.h>
#include <sys/obj.h>

s32 ftCommonKneeBendGetInputTypeCommon(FTStruct *fp);
sb32 ftCommonPassCheckInputSuccess(FTStruct *fp);
void ftCommonJumpGetJumpForceButton(s32 stick_range_x, s32 *jump_vel_x, s32 *jump_vel_y,
                                    sb32 is_shorthop);
sb32 ftMarioSpecialHiProcPass(GObj *fighter_gobj);
void ftCommonDamageFlyRollUpdateModelPitch(GObj *fighter_gobj);
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);
f32 syUtilsArcTan2(f32 y, f32 x);
f32 __sinf(f32);
f32 __cosf(f32);
f32 sqrtf(f32);

/* Lanky.Action (his action array). */
#define LANKY_STATUS_NSPG 0xDF
#define LANKY_STATUS_NSPA 0xE0
#define LANKY_STATUS_USPG_BEGIN 0xE5
#define LANKY_STATUS_USPA_BEGIN 0xE6
#define LANKY_STATUS_USP_MOVE 0xE7
#define LANKY_STATUS_USP_TURN 0xE8
#define LANKY_STATUS_USP_END 0xE9
#define LANKY_STATUS_USP_DAMAGE 0xEA
#define LANKY_STATUS_DSPG_BEGIN 0xEB
#define LANKY_STATUS_DSPG_WAIT 0xEC
#define LANKY_STATUS_DSPG_END 0xED
#define LANKY_STATUS_DSPG_CANCEL 0xEE
#define LANKY_STATUS_DSP_TURN 0xEF
#define LANKY_STATUS_DSP_MOVE 0xF0
#define LANKY_STATUS_DSP_TAUNT 0xF1
#define LANKY_STATUS_DSP_LANDING 0xF2
#define LANKY_STATUS_DSP_JUMP_SQUAT 0xF3
#define LANKY_STATUS_DSP_JUMP 0xF4
#define LANKY_STATUS_DSP_PLAT_DROP 0xF5
#define LANKY_STATUS_DSPA_BEGIN 0xF6
#define LANKY_STATUS_DSPA_WAIT 0xF7
#define LANKY_STATUS_DSPA_END 0xF8
#define LANKY_STATUS_DSPA_CANCEL 0xF9

/* Remix's action ids for the common statuses his patch names. */
_Static_assert(0x032 == nFTCommonStatusDamageE2, "DamageElec2");
_Static_assert(0x033 == nFTCommonStatusDamageFlyHi, "DamageFlyHigh");
_Static_assert(0x037 == nFTCommonStatusDamageFlyRoll, "DamageFlyRoll");
_Static_assert(FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON == 2, "kneebend button type");

/* Joypad.A and Joypad.B in button_tap. */
#define LANKY_BUTTON_A 0x8000u
#define LANKY_BUTTON_B 0x4000u

/* "continue: 3C FGM, gfx routines, hitboxes" and "gfx routines,
 * hitboxes". */
#define LANKY_PRESERVE_BALLOON 0x0803u
#define LANKY_PRESERVE_BALLOON_CANCEL 0x0003u

/* LankyNSP (the Grape Shooter). */
#define LANKY_NSP_AMMO 3
#define LANKY_NSP_REFIRE_FRAME 14.0F

/* LankyUSP (the balloon). */
#define LANKY_USP_Y_SPEED 16.0F
#define LANKY_USP_END_Y_SPEED 50.0F
#define LANKY_USP_AIR_SPEED 24.0F
#define LANKY_USP_LANDING_FSM 0.375F
#define LANKY_USP_MAX_TIME 120

/* LankyDSP (OrangStand). */
#define LANKY_DSP_MIN_SPEED 8.0F
#define LANKY_DSP_MAX_SPEED 54.0F
#define LANKY_DSP_ACCELERATION 10.0F
#define LANKY_DSP_NEUTRAL_ACCELERATION 4.0F
#define LANKY_DSP_G_SPEED_MULTIPLIER 0.8125F

static f32 ndsP4LankyBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

static s32 *ndsP4LankyStatusS32(FTStruct *fp, s32 word)
{
    return &((s32 *)(void *)&fp->status_vars)[word];
}

static f32 *ndsP4LankyStatusF32(FTStruct *fp, s32 word)
{
    return &((f32 *)(void *)&fp->status_vars)[word];
}

static void ndsP4LankySetStatus(GObj *fighter_gobj, s32 status_id, f32 frame_begin, u32 flags)
{
    ftMainSetStatus(fighter_gobj, status_id, frame_begin, 1.0F, flags);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

static void ndsP4LankyClearFlags(FTStruct *fp)
{
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

/* The stick held 11 or more against his facing (sign bits differ). */
static sb32 ndsP4LankyStickAgainst(FTStruct *fp)
{
    s32 stick_x = fp->input.pl.stick_range.x;

    return (((u32)stick_x & 0x80000000u) != ((u32)fp->lr & 0x80000000u)) ? TRUE : FALSE;
}

/* ---- Neutral special (the Grape Shooter) ---- */

/* LankyNSP.ground_initial_ / air_initial_: three more shots after this. */
static void ndsP4LankyNSPInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4LankySetStatus(fighter_gobj, status_id, 0.0F, FTSTATUS_PRESERVE_NONE);
    ndsP4LankyClearFlags(fp);
    *ndsP4LankyStatusS32(fp, 2) = LANKY_NSP_AMMO;
}

void ndsP4LankyNSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4LankyNSPInitial(fighter_gobj, LANKY_STATUS_NSPG);
}

void ndsP4LankyNSPAirInitial(GObj *fighter_gobj)
{
    ndsP4LankyNSPInitial(fighter_gobj, LANKY_STATUS_NSPA);
}

/* LankyNSP.main_ (0xDF/0xE0 update): temp variable 1 shoots the grape
 * (S6); in the script's window (temp variable 2) a B tap spends a shot and
 * restarts the swing at frame 14; the end of the animation waits. */
void ndsP4LankyNSPMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->motion_vars.flags.flag0 = 0;
    if ((fp->motion_vars.flags.flag1 != 0) &&
        ((fp->input.pl.button_tap & LANKY_BUTTON_B) != 0))
    {
        s32 *ammo = ndsP4LankyStatusS32(fp, 2);
        s32 left = *ammo;

        *ammo = left - 1;
        if (left == 0)
        {
            *ammo = 0;
        }
        else
        {
            ndsP4LankySetStatus(fighter_gobj,
                                (fp->ga == nMPKineticsGround) ? LANKY_STATUS_NSPG : LANKY_STATUS_NSPA,
                                LANKY_NSP_REFIRE_FRAME, FTSTATUS_PRESERVE_NONE);
            fp->motion_vars.flags.flag1 = 0;
        }
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* ---- Up special (the balloon) ---- */

/* LankyUSP.ground_initial_ (ground_usp). */
void ndsP4LankyUSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4LankySetStatus(fighter_gobj, LANKY_STATUS_USPG_BEGIN, 0.0F, FTSTATUS_PRESERVE_NONE);
    ndsP4LankyClearFlags(ftGetStruct(fighter_gobj));
}

/* LankyUSP.air_initial_ (air_usp): the fall stopped. The donor masks a
 * register it never loaded with byte 0x18D before storing it back; the
 * port clears the four flags the other air starts clear and keeps the
 * byte's other three. */
void ndsP4LankyUSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4LankySetStatus(fighter_gobj, LANKY_STATUS_USPA_BEGIN, 0.0F, FTSTATUS_PRESERVE_NONE);
    ndsP4LankyClearFlags(fp);
    fp->physics.vel_air.y = 0.0F;
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
}

/* LankyUSP.move_initial_: inflated, airborne, his jumps spent. */
static void ndsP4LankyUSPMoveInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4LankySetStatus(fighter_gobj, LANKY_STATUS_USP_MOVE, 0.0F, LANKY_PRESERVE_BALLOON);
    fp->jumps_used = (u8)fp->attr->jumps_max;
    fp->ga = nMPKineticsAir;
}

/* LankyUSP.turn_intial_. */
static void ndsP4LankyUSPTurnInitial(GObj *fighter_gobj)
{
    ndsP4LankySetStatus(fighter_gobj, LANKY_STATUS_USP_TURN, 0.0F, LANKY_PRESERVE_BALLOON);
}

/* LankyUSP.end_initial_: the burst, 50 up. */
static void ndsP4LankyUSPEndInitial(GObj *fighter_gobj, u32 flags)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4LankySetStatus(fighter_gobj, LANKY_STATUS_USP_END, 0.0F, flags);
    fp->physics.vel_air.y = LANKY_USP_END_Y_SPEED;
    fp->motion_vars.flags.flag1 = 1;
    fp->motion_vars.flags.flag2 = 1;
}

/* LankyUSP.begin_main_ (0xE5/0xE6 update): the balloon's 120 frames. */
void ndsP4LankyUSPBeginMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        *ndsP4LankyStatusS32(ftGetStruct(fighter_gobj), 0) = LANKY_USP_MAX_TIME;
        ndsP4LankyUSPMoveInitial(fighter_gobj);
    }
}

/* The balloon's timer and B: TRUE when it burst. */
static sb32 ndsP4LankyUSPCheckEnd(GObj *fighter_gobj, FTStruct *fp)
{
    s32 *timer = ndsP4LankyStatusS32(fp, 0);

    *timer -= 1;
    if (*timer == 0)
    {
        ndsP4LankyUSPEndInitial(fighter_gobj, LANKY_PRESERVE_BALLOON);
        return TRUE;
    }
    if ((fp->input.pl.button_tap & LANKY_BUTTON_B) != 0)
    {
        ndsP4LankyUSPEndInitial(fighter_gobj, LANKY_PRESERVE_BALLOON_CANCEL);
        return TRUE;
    }
    return FALSE;
}

/* LankyUSP.main_ (0xE7 update): the stick against his facing turns him. */
void ndsP4LankyUSPMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ndsP4LankyUSPCheckEnd(fighter_gobj, fp) != FALSE)
    {
        return;
    }
    if ((ABS(fp->input.pl.stick_range.x) >= 11) && (ndsP4LankyStickAgainst(fp) != FALSE))
    {
        ndsP4LankyUSPTurnInitial(fighter_gobj);
    }
}

/* LankyUSP.turn_main_ (0xE8 update): temp variable 1 flips his facing;
 * the turn ends back in the move. */
void ndsP4LankyUSPTurnMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->lr = -fp->lr;
        fp->motion_vars.flags.flag0 = 0;
    }
    if (ndsP4LankyUSPCheckEnd(fighter_gobj, fp) != FALSE)
    {
        return;
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ndsP4LankyUSPMoveInitial(fighter_gobj);
    }
}

/* LankyUSP.end_main_ (0xE9 update): Fox's Fire Fox end with his landing lag
 * and no interrupt. */
void ndsP4LankyUSPEndMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, 1.0F, FALSE, TRUE, FALSE,
                                     LANKY_USP_LANDING_FSM, FALSE);
    }
}

/* LankyUSP.physics_ (0xE7/0xE8 physics): his drift and air friction; the
 * balloon rises at 16. */
void ndsP4LankyUSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    /* LankyUSP.air_control_. */
    ftPhysicsClampAirVelXStickRange(fp, 8, ndsP4LankyBitsToF32(0x3C240000u), LANKY_USP_AIR_SPEED);
    ftPhysicsApplyAirVelXFriction(fp, fp->attr);
    fp->physics.vel_air.y = LANKY_USP_Y_SPEED;
}

/* LankyUSP.damage_physics_ (0xEA physics): knocked out of the balloon,
 * his flight bends toward the stick, 6 degrees a frame. */
void ndsP4LankyUSPDamagePhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 stick_y;
    f32 stick_x;
    f32 stick_angle;
    f32 kb_y;
    f32 kb_x;
    f32 kb_vel;
    f32 kb_angle;
    f32 diff;
    f32 turn = ndsP4LankyBitsToF32(0x3DD67770u);
    f32 two_pi = ndsP4LankyBitsToF32(0x40C90FE4u);
    f32 pi = ndsP4LankyBitsToF32(0x40490FD0u);

    if (*ndsP4LankyStatusS32(fp, 0) == 0)
    {
        ftPhysicsApplyAirVelDriftFastFall(fighter_gobj);
    }
    stick_y = (f32)fp->input.pl.stick_range.y;
    stick_x = (f32)fp->input.pl.stick_range.x;
    if (10.0F <= sqrtf((stick_y * stick_y) + (stick_x * stick_x)))
    {
        stick_angle = syUtilsArcTan2(stick_y, stick_x);
        kb_y = fp->physics.vel_damage_air.y;
        kb_x = fp->physics.vel_damage_air.x;
        kb_vel = sqrtf((kb_y * kb_y) + (kb_x * kb_x));
        kb_angle = syUtilsArcTan2(kb_y, kb_x);

        diff = (stick_angle + two_pi) - (kb_angle + two_pi);
        if (!(0.0F < diff))
        {
            diff += two_pi;
        }
        if (turn < diff)
        {
            if (!(diff < pi))
            {
                turn = -turn;
            }
            kb_angle += turn;
            fp->physics.vel_damage_air.x = kb_vel * __cosf(kb_angle);
            fp->physics.vel_damage_air.y = kb_vel * __sinf(kb_angle);
        }
    }
    ftCommonDamageFlyRollUpdateModelPitch(fighter_gobj);
}

static void ndsP4LankyUSPBeginGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, LANKY_STATUS_USPA_BEGIN, fighter_gobj->anim_frame, 1.0F,
                    LANKY_PRESERVE_BALLOON);
    ftPhysicsClampAirVelXMax(fp);
}

static void ndsP4LankyUSPBeginAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, LANKY_STATUS_USPG_BEGIN, fighter_gobj->anim_frame, 1.0F,
                    LANKY_PRESERVE_BALLOON);
}

/* LankyUSP.begin_ground_collision_ / begin_air_collision_. */
void ndsP4LankyUSPBeginGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4LankyUSPBeginGroundToAir);
}

void ndsP4LankyUSPBeginAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4LankyUSPBeginAirToGround);
}

/* LankyUSP.collision_ (0xE7-0xE9 map): Mario's Super Jump map with his
 * landing lag; the copied delay slot keeps the low half of Mario's constant
 * (0x3EC05C29). */
void ndsP4LankyUSPMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsAir)
    {
        if ((fp->motion_vars.flags.flag1 == 0) || (fp->physics.vel_air.y >= 0.0F))
        {
            mpCommonCheckFighterProject(fighter_gobj);
        }
        else if (mpCommonCheckFighterPassCliff(fighter_gobj, ftMarioSpecialHiProcPass) != FALSE)
        {
            if (fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK)
            {
                ftCommonCliffCatchSetStatus(fighter_gobj);
            }
            else ftCommonLandingFallSpecialSetStatus(fighter_gobj, FALSE,
                                                     ndsP4LankyBitsToF32(0x3EC05C29u));
        }
    }
    else mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
}

/* LankyUSP.damage_patch_ (ftCommonDamageInitDamageVars' status change):
 * knocked out of the balloon, a DamageFly status (or the one an electric
 * hit leads to) becomes his balloon damage. */
static s32 ndsP4LankyDamageStatus(FTStruct *fp, s32 status_id, s32 *status_id_after)
{
    if ((fp->status_id != LANKY_STATUS_USPG_BEGIN) && (fp->status_id != LANKY_STATUS_USPA_BEGIN) &&
        (fp->status_id != LANKY_STATUS_USP_TURN) && (fp->status_id != LANKY_STATUS_USP_MOVE))
    {
        return status_id;
    }
    if (status_id == nFTCommonStatusDamageE2)
    {
        if (((u32)*status_id_after >= nFTCommonStatusDamageFlyHi) &&
            ((u32)*status_id_after <= nFTCommonStatusDamageFlyRoll))
        {
            *status_id_after = LANKY_STATUS_USP_DAMAGE;
        }
        return status_id;
    }
    if (((u32)status_id >= nFTCommonStatusDamageFlyHi) &&
        ((u32)status_id <= nFTCommonStatusDamageFlyRoll))
    {
        return LANKY_STATUS_USP_DAMAGE;
    }
    return status_id;
}

/* ---- Down special (OrangStand) ---- */

static void ndsP4LankyDSPSetStatus(GObj *fighter_gobj, s32 status_id)
{
    ndsP4LankySetStatus(fighter_gobj, status_id, 0.0F, FTSTATUS_PRESERVE_NONE);
}

/* LankyDSP.ground_initial_ (ground_dsp) / air_initial_ (air_dsp). */
void ndsP4LankyDSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4LankyDSPSetStatus(fighter_gobj, LANKY_STATUS_DSPG_BEGIN);
    ndsP4LankyClearFlags(ftGetStruct(fighter_gobj));
}

void ndsP4LankyDSPAirInitial(GObj *fighter_gobj)
{
    ndsP4LankyDSPSetStatus(fighter_gobj, LANKY_STATUS_DSPA_BEGIN);
    ndsP4LankyClearFlags(ftGetStruct(fighter_gobj));
}

/* LankyDSP.move_initial_: walking on his hands at 0.625 a stick unit. */
static void ndsP4LankyDSPMoveInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4LankyDSPSetStatus(fighter_gobj, LANKY_STATUS_DSP_MOVE);
    fp->physics.vel_air.x = (f32)fp->input.pl.stick_range.x * 0.625F;
    fp->physics.vel_ground.x = fp->physics.vel_air.x * (f32)fp->lr;
}

/* LankyDSP.wait_initial_: in the air his air wait; on the ground the wait,
 * or straight into a move or a turn with the stick held. */
static void ndsP4LankyDSPWaitInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga != nMPKineticsGround)
    {
        ndsP4LankyDSPSetStatus(fighter_gobj, LANKY_STATUS_DSPA_WAIT);
    }
    else if (ABS(fp->input.pl.stick_range.x) < 11)
    {
        ndsP4LankyDSPSetStatus(fighter_gobj, LANKY_STATUS_DSPG_WAIT);
    }
    else if (ndsP4LankyStickAgainst(fp) != FALSE)
    {
        ndsP4LankyDSPSetStatus(fighter_gobj, LANKY_STATUS_DSP_TURN);
    }
    else ndsP4LankyDSPMoveInitial(fighter_gobj);
}

/* LankyDSP.end_initial_: off his hands, 16 forward and the rise cut. */
static void ndsP4LankyDSPEndInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = (fp->ga == nMPKineticsGround) ? LANKY_STATUS_DSPG_END : LANKY_STATUS_DSPA_END;
    union { u32 u; f32 f; } push;

    push.u = 0x41800000u | ((u32)fp->lr & 0x80000000u);
    fp->physics.vel_air.x += push.f;
    fp->physics.vel_air.y *= 0.375F;
    ndsP4LankyDSPSetStatus(fighter_gobj, status_id);
}

/* LankyDSP.cancel_initial_: out of the handstand, keeping what he pressed
 * (replayed when it ends); a jump squat's cancel remembers the jump. */
static void ndsP4LankyDSPCancelInitial(GObj *fighter_gobj, sb32 jump_squat)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 tap_x;
    u32 tap_y;

    ndsP4LankyDSPSetStatus(fighter_gobj, (fp->ga != nMPKineticsGround) ? LANKY_STATUS_DSPA_CANCEL
                                                                       : LANKY_STATUS_DSPG_CANCEL);
    *ndsP4LankyStatusS32(fp, 0) = fp->input.pl.button_tap;
    tap_x = fp->tap_stick_x;
    tap_y = fp->tap_stick_y;
    *ndsP4LankyStatusS32(fp, 1) = (tap_x < 7u) ? 1 : (s32)tap_x;
    *ndsP4LankyStatusS32(fp, 2) = (tap_y < 6u) ? 1 : (s32)tap_y;
    if (jump_squat != FALSE)
    {
        *ndsP4LankyStatusS32(fp, 2) = 1;
    }
    *ndsP4LankyStatusS32(fp, 3) = jump_squat;
}

/* LankyDSP.jumpsquat_initial_ (ftDonkeyThrowFKneeBendSetStatus with his
 * status and a frame speed of 1). */
static void ndsP4LankyDSPJumpSquatInitial(GObj *fighter_gobj, s32 input_type)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, LANKY_STATUS_DSP_JUMP_SQUAT, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    *ndsP4LankyStatusF32(fp, 0) = (f32)fp->input.pl.stick_range.y;
    *ndsP4LankyStatusF32(fp, 1) = 0.0F;
    *ndsP4LankyStatusS32(fp, 2) = input_type;
    *ndsP4LankyStatusS32(fp, 3) = FALSE;
}

/* LankyDSP.jump_initial_: airborne, then the rest of
 * ftDonkeyThrowFJumpSetStatus with his status and a frame speed of 1. */
static void ndsP4LankyDSPJumpInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    s32 vel_x;
    s32 vel_y;

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, LANKY_STATUS_DSP_JUMP, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    if (*ndsP4LankyStatusS32(fp, 2) == FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON)
    {
        ftCommonJumpGetJumpForceButton(fp->input.pl.stick_range.x, &vel_x, &vel_y,
                                       *ndsP4LankyStatusS32(fp, 3));
    }
    else
    {
        vel_x = fp->input.pl.stick_range.x;
        vel_y = (s32)*ndsP4LankyStatusF32(fp, 0);
    }
    fp->physics.vel_air.y = ((f32)vel_y * attr->jump_height_mul) + attr->jump_height_base;
    fp->physics.vel_air.x = (f32)vel_x * attr->jump_vel_x;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
}

/* LankyDSP.plat_drop_initial_: through the platform from his hands. */
static void ndsP4LankyDSPPlatDropInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ndsP4LankyDSPSetStatus(fighter_gobj, LANKY_STATUS_DSP_PLAT_DROP);
    ftPhysicsClampAirVelXMax(fp);
    fp->coll_data.ignore_line_id = fp->coll_data.floor_line_id;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->physics.vel_air.y = 0.0F;
}

/* The handstand's shared inputs, in the donor's order: down with B ends
 * it; A or B, or shield where `shield` holds, cancels it; taunt, jump and
 * platform drop where the caller allows. TRUE when it changed status. */
static sb32 ndsP4LankyDSPCheckInputs(GObj *fighter_gobj, FTStruct *fp, sb32 ground)
{
    u16 tap = fp->input.pl.button_tap;
    s32 jump;

    if ((fp->input.pl.stick_range.y < -39) && ((tap & LANKY_BUTTON_B) != 0))
    {
        ndsP4LankyDSPEndInitial(fighter_gobj);
        return TRUE;
    }
    if (((tap & (LANKY_BUTTON_A | LANKY_BUTTON_B)) != 0) ||
        ((ground != FALSE) && ((fp->input.button_mask_z & tap) != 0)))
    {
        ndsP4LankyDSPCancelInitial(fighter_gobj, FALSE);
        return TRUE;
    }
    if (ground == FALSE)
    {
        return FALSE;
    }
    if ((fp->input.button_mask_l & tap) != 0)
    {
        ndsP4LankyDSPSetStatus(fighter_gobj, LANKY_STATUS_DSP_TAUNT);
        return TRUE;
    }
    jump = ftCommonKneeBendGetInputTypeCommon(fp);
    if (jump != 0)
    {
        ndsP4LankyDSPJumpSquatInitial(fighter_gobj, jump);
        return TRUE;
    }
    if (((fp->coll_data.floor_flags & 0x4000u) != 0u) &&
        (ftCommonPassCheckInputSuccess(fp) != FALSE))
    {
        ndsP4LankyDSPPlatDropInitial(fighter_gobj);
        return TRUE;
    }
    return FALSE;
}

/* LankyDSP.begin_main_ (0xEB/0xF6 update). */
void ndsP4LankyDSPBeginMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ndsP4LankyDSPWaitInitial(fighter_gobj);
    }
}

/* LankyDSP.ground_wait_main_ (0xEC update). */
void ndsP4LankyDSPGroundWaitMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ndsP4LankyDSPCheckInputs(fighter_gobj, fp, TRUE) != FALSE)
    {
        return;
    }
    if (ABS(fp->input.pl.stick_range.x) < 11)
    {
        return;
    }
    if (ndsP4LankyStickAgainst(fp) != FALSE)
    {
        ndsP4LankyDSPSetStatus(fighter_gobj, LANKY_STATUS_DSP_TURN);
    }
    else ndsP4LankyDSPMoveInitial(fighter_gobj);
}

/* LankyDSP.air_wait_main_ (0xF7 update): ends, cancels or double jumps. */
void ndsP4LankyDSPAirWaitMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ndsP4LankyDSPCheckInputs(fighter_gobj, fp, FALSE) != FALSE)
    {
        return;
    }
    ftCommonJumpAerialCheckInterruptCommon(fighter_gobj);
}

/* LankyDSP.move_main_ (0xF0 update): the walk's frame speed follows his
 * speed (0x3CC00000 a unit, plus 0x3DCD0000); a released stick stops him
 * in the wait once he has stopped. */
void ndsP4LankyDSPMoveMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    gcSetAnimSpeed(fighter_gobj, (ndsP4LankyBitsToF32(0x3CC00000u) * fp->physics.vel_ground.x) +
                                     ndsP4LankyBitsToF32(0x3DCD0000u));
    if (ndsP4LankyDSPCheckInputs(fighter_gobj, fp, TRUE) != FALSE)
    {
        return;
    }
    if (ABS(fp->input.pl.stick_range.x) < 11)
    {
        union { f32 f; u32 u; } vel;

        vel.f = fp->physics.vel_air.x;
        if (vel.u == 0u)
        {
            ndsP4LankyDSPWaitInitial(fighter_gobj);
        }
        return;
    }
    if (ndsP4LankyStickAgainst(fp) != FALSE)
    {
        ndsP4LankyDSPSetStatus(fighter_gobj, LANKY_STATUS_DSP_TURN);
    }
}

/* LankyDSP.turn_main_ (0xEF update): temp variable 2 flips his facing; the
 * turn ends in the wait. */
void ndsP4LankyDSPTurnMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 != 0)
    {
        fp->lr = -fp->lr;
        fp->motion_vars.flags.flag1 = 0;
    }
    if (ndsP4LankyDSPCheckInputs(fighter_gobj, fp, TRUE) != FALSE)
    {
        return;
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ndsP4LankyDSPWaitInitial(fighter_gobj);
    }
}

/* LankyDSP.jumpsquat_main_ (0xF3 update; ftDonkeyThrowFKneeBendProcUpdate):
 * a jump button let go in the first 3 frames short-hops. */
void ndsP4LankyDSPJumpSquatMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 *frame = ndsP4LankyStatusF32(fp, 1);

    *frame += 1.0F;
    if ((*ndsP4LankyStatusS32(fp, 2) == FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON) &&
        (*frame <= 3.0F) &&
        ((fp->input.pl.button_release & (U_CBUTTONS | D_CBUTTONS | L_CBUTTONS | R_CBUTTONS)) != 0))
    {
        *ndsP4LankyStatusS32(fp, 3) = TRUE;
    }
    if (fp->attr->kneebend_anim_length <= *frame)
    {
        ndsP4LankyDSPJumpInitial(fighter_gobj);
    }
}

/* LankyDSP.jumpsquat_interrupt_ (0xF3 interrupt): up with A or B cancels
 * into the jump's attack. */
void ndsP4LankyDSPJumpSquatInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp->input.pl.stick_range.y >= 53) &&
        ((fp->input.pl.button_tap & (LANKY_BUTTON_A | LANKY_BUTTON_B)) != 0))
    {
        ndsP4LankyDSPCancelInitial(fighter_gobj, TRUE);
    }
}

/* LankyDSP.jump_main_ (0xF4/0xF5 update). */
void ndsP4LankyDSPJumpMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ndsP4LankyDSPCheckInputs(fighter_gobj, fp, FALSE) != FALSE)
    {
        return;
    }
    if (ftCommonJumpAerialCheckInterruptCommon(fighter_gobj) != FALSE)
    {
        return;
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ndsP4LankyDSPWaitInitial(fighter_gobj);
    }
}

/* LankyDSP.landing_main_ (0xF1/0xF2 update). */
void ndsP4LankyDSPLandingMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ndsP4LankyDSPWaitInitial(fighter_gobj);
    }
}

/* LankyDSP.cancel_main_ (0xEE/0xF9 update): the taps pressed since the
 * cancel began are replayed into the wait (with the stick up for a jump
 * squat's cancel), and the tap timers it kept. */
void ndsP4LankyDSPCancelMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *taps = ndsP4LankyStatusS32(fp, 0);

    *taps = (u16)(*taps | fp->input.pl.button_tap);
    if (fighter_gobj->anim_frame > 0.0F)
    {
        return;
    }
    if (*ndsP4LankyStatusS32(fp, 3) != FALSE)
    {
        fp->input.pl.stick_range.y = 80;
    }
    fp->tap_stick_x = (u8)*ndsP4LankyStatusS32(fp, 1);
    fp->tap_stick_y = (u8)*ndsP4LankyStatusS32(fp, 2);
    fp->input.pl.button_tap = (u16)*taps;
    mpCommonSetFighterWaitOrFall(fighter_gobj);
}

/* LankyDSP.ground_physics_ (0xEC/0xEF/0xF0 physics): his hands steer him,
 * the target rising with the stick's square from 8 (0x3C380000 a unit
 * squared, times 0.8125), accelerating by 10; a neutral stick slows him at
 * 4 and stops him under 8. The steering keeps the velocity within 2 of the
 * target (the donor's `mov.s f2, f4` does nothing). */
void ndsP4LankyDSPGroundPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 accel = LANKY_DSP_ACCELERATION;
    f32 target = 0.0F;
    f32 vel = fp->physics.vel_air.x;
    f32 diff;
    s32 stick_x = fp->input.pl.stick_range.x;

    if (ABS(stick_x) < 11)
    {
        accel = LANKY_DSP_NEUTRAL_ACCELERATION;
        if (ABS(vel) <= LANKY_DSP_MIN_SPEED)
        {
            *ndsP4LankyStatusS32(fp, 2) = -1;
            fp->physics.vel_air.x = 0.0F;
            return;
        }
    }
    else
    {
        f32 stick = (f32)stick_x;

        target = ((stick * ABS(stick)) * ndsP4LankyBitsToF32(0x3C380000u)) +
                 ((stick_x < 0) ? -LANKY_DSP_MIN_SPEED : LANKY_DSP_MIN_SPEED);
        target *= LANKY_DSP_G_SPEED_MULTIPLIER;
    }
    diff = target - vel;
    if (2.0F <= ABS(diff))
    {
        union { f32 f; u32 u; } d;

        d.f = diff;
        vel = ((d.u & 0x80000000u) == 0u) ? (vel + accel) : (vel - accel);
    }
    if (!(ABS(vel) <= LANKY_DSP_MAX_SPEED))
    {
        union { f32 f; u32 u; } v, m;

        v.f = vel;
        m.f = LANKY_DSP_MAX_SPEED;
        m.u |= v.u & 0x80000000u;
        vel = m.f;
    }
    fp->physics.vel_air.x = vel;
    fp->physics.vel_ground.x = vel * (f32)fp->lr;
}

static void ndsP4LankyDSPGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ndsP4LankyDSPWaitInitial(fighter_gobj);
    ftPhysicsClampAirVelXMax(fp);
}

static void ndsP4LankyDSPAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ndsP4LankyDSPSetStatus(fighter_gobj, LANKY_STATUS_DSP_LANDING);
}

/* LankyDSP.ground_collision_ / air_collision_. */
void ndsP4LankyDSPGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4LankyDSPGroundToAir);
}

void ndsP4LankyDSPAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterCliff(fighter_gobj, ndsP4LankyDSPAirToGround);
}

const NDSP4Overrides gNdsP4LankyOverrides = {
    .damage_status = ndsP4LankyDamageStatus,
};

#endif /* NDS_P4_LANKY */
