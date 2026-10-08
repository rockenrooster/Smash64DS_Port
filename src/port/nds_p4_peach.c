/*
 * P4 Peach: native ports of the donor's special routines and of Remix's
 * float, which patches the parent's common code on her character id.
 * Source: JSsixtyfour/smashremix 5e04fe7, src/Peach/PeachSpecial.asm and
 * Peach.asm, read as assembled (scripts/p4/mipsdis.py): the OS.copy_segment
 * blocks are the original game's code. Peach runs Fox's status code
 * (fp->fkind == nFTKindFox).
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x0CC collision flags                  coll_data.mask_prev (the map
 *                                          callback runs after ftmain moves
 *                                          mask_curr there and zeroes it:
 *                                          last frame's walls)
 *   0x17C/0x180/0x184 temp variables 1-3   motion_vars.flags.flag0/1/2
 *   0x18D & 0x07                           clears is_absorb, absorb_lr,
 *                                          is_goto_attack100 and is_fastfall
 *   0x1BC/0x1BE buttons held/tapped        input.pl.button_hold/button_tap
 *   0x1C3 stick y                          input.pl.stick_range.y
 *   0x9E4 collision routine                proc_map
 *   0x9F4/0x9F8 shield, hit routines       proc_shield, proc_hit
 *   0xADC float timer                      passive_vars, first word
 *   0xB18 smash input (Link's)             status_vars, first word
 *   attributes + 0x58/0x5C/0x64            gravity, tvel_base, jumps_max
 *
 * The float timer: 0 before her first float since she landed, 150 on the
 * frame a float starts, counting down while a jump button is held, and 1
 * once the float is used up or let go.
 *
 * Her down special (PeachDSP: the turnip pull) needs Remix's turnip item, an
 * S6 item article; it keeps its lab fallbacks until then.
 */
#include <nds/nds_p4.h>

#if NDS_P4_PEACH

#include <macros.h>

sb32 ftMarioSpecialHiProcPass(GObj *fighter_gobj);
void ftCaptainSpecialHiProcInterrupt(GObj *fighter_gobj);
void mpCommonProcFighterWaitOrLanding(GObj *fighter_gobj);

/* Peach.Action (her action array). */
#define PEACH_STATUS_FLOAT 0xDE
#define PEACH_STATUS_NSPG 0xDF
#define PEACH_STATUS_NSPA 0xE0
#define PEACH_STATUS_NSP_RECOIL 0xE1
#define PEACH_STATUS_USPG 0xE3
#define PEACH_STATUS_USPA 0xE4
#define PEACH_STATUS_USP_OPEN 0xE5
#define PEACH_STATUS_USP_FLOAT 0xE6
#define PEACH_STATUS_USP_CLOSE 0xE7
#define PEACH_STATUS_USP_FALL 0xE8

/* Remix's action ids for the common statuses her code names. */
#define PEACH_ACTION_JUMP_AERIAL_F 0x18
#define PEACH_ACTION_JUMP_AERIAL_B 0x19
#define PEACH_ACTION_ATTACK_AIR_N 0xD1
#define PEACH_ACTION_ATTACK_AIR_D 0xD5
_Static_assert(PEACH_ACTION_JUMP_AERIAL_F == nFTCommonStatusJumpAerialF, "JumpAerialF");
_Static_assert(PEACH_ACTION_JUMP_AERIAL_B == nFTCommonStatusJumpAerialB, "JumpAerialB");
_Static_assert(PEACH_ACTION_ATTACK_AIR_N == nFTCommonStatusAttackAirN, "AttackAirN");
_Static_assert(PEACH_ACTION_ATTACK_AIR_D == nFTCommonStatusAttackAirLw, "AttackAirD");

/* PeachFloat. */
#define PEACH_FLOAT_TIMER 150
#define PEACH_FLOAT_USED 1
#define PEACH_JUMP_BUTTONS (U_CBUTTONS | D_CBUTTONS | L_CBUTTONS | R_CBUTTONS)

/* PeachNSP (Peach Bomber) temp variable 3 states and speeds. */
#define PEACH_NSP_BEGIN_MOVE 1
#define PEACH_NSP_MOVE 2
#define PEACH_NSP_END_MOVE 3
#define PEACH_NSP_END 4
#define PEACH_NSP_X_SPEED 48.0F
#define PEACH_NSP_X_SMASH_SPEED 60.0F
#define PEACH_NSP_Y_SPEED_INITIAL 30.0F
#define PEACH_NSP_RECOIL_X_SPEED -34.0F
#define PEACH_NSP_RECOIL_Y_SPEED 40.0F
#define PEACH_NSP_AIR_FRICTION 1.0F
#define PEACH_NSP_CLANG_FRAME 35.0F

/* PeachUSP (the parasol) temp variable 3 states and constants (each a
 * `lui` upper half). */
#define PEACH_USP_BEGIN 1
#define PEACH_USP_BEGIN_MOVE 2
#define PEACH_USP_MOVE 3
#define PEACH_USP_END_MOVE 4
#define PEACH_USP_AIR_Y_SPEED 82.0F
#define PEACH_USP_GROUND_Y_SPEED 84.0F
#define PEACH_USP_X_SPEED 18.0F
#define PEACH_USP_FLOAT_GRAVITY 0.5F
#define PEACH_USP_FLOAT_FALL_SPEED 13.0F
#define PEACH_USP_AIR_SPEED 24.0F
#define PEACH_USP_LANDING_FSM 0.375F

/* "continue: special model parts" (her parasol and crown). */
#define PEACH_PRESERVE_MODELPART FTSTATUS_PRESERVE_MODELPART
/* Hit status and colanim, the transitions' word. */
#define PEACH_PRESERVE_HIT 0x3u

static f32 ndsP4PeachBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

static u32 *ndsP4PeachFloatTimer(FTStruct *fp)
{
    return (u32 *)(void *)&fp->passive_vars;
}

/* A float in progress: started and not yet used up. */
static sb32 ndsP4PeachFloating(FTStruct *fp)
{
    u32 timer = *ndsP4PeachFloatTimer(fp);

    return ((timer != 0u) && (timer != PEACH_FLOAT_USED)) ? TRUE : FALSE;
}

static void ndsP4PeachClearFastFall(FTStruct *fp)
{
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
}

/* ---- Float ---- */

/* PeachFloat.initial_. The source comments 0.375; the build multiplies the
 * fall by 0.25 (0x3E800000). */
static void ndsP4PeachFloatInitial(GObj *fighter_gobj)
{
    FTStruct *fp;

    ftMainSetStatus(fighter_gobj, PEACH_STATUS_FLOAT, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    fp = ftGetStruct(fighter_gobj);
    fp->physics.vel_air.y = (0.25F * fp->physics.vel_air.y) + 24.0F;
}

/* PeachFloat.main_ (0xDE update): a used-up float falls. */
void ndsP4PeachFloatMain(GObj *fighter_gobj)
{
    if (*ndsP4PeachFloatTimer(ftGetStruct(fighter_gobj)) == PEACH_FLOAT_USED)
    {
        ftCommonFallSetStatus(fighter_gobj);
    }
}

/* PeachFloat.interrupt_ (0xDE interrupt): specials and aerials; an aerial
 * keeps floating, any other status change ends the float. */
void ndsP4PeachFloatInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp;

    if ((ftCommonSpecialAirCheckInterruptCommon(fighter_gobj) == FALSE) &&
        (ftCommonAttackAirCheckInterruptCommon(fighter_gobj) == FALSE))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if (((u32)fp->status_id < PEACH_ACTION_ATTACK_AIR_N) ||
        ((u32)fp->status_id > PEACH_ACTION_ATTACK_AIR_D))
    {
        *ndsP4PeachFloatTimer(fp) = PEACH_FLOAT_USED;
    }
}

/* PeachFloat.handle_physics_ (ftPhysicsApplyGravityClampTVel's head): the
 * first frame and a spent or unstarted float fall as usual; otherwise the
 * timer runs and, while a jump button is held, she hangs (no gravity). */
static sb32 ndsP4PeachGravity(FTStruct *fp)
{
    u32 *timer = ndsP4PeachFloatTimer(fp);
    u32 t = *timer;

    if (t == 0u)
    {
        return FALSE;
    }
    if (t == PEACH_FLOAT_TIMER)
    {
        *timer = t - 1u;
        return FALSE;
    }
    if (t == PEACH_FLOAT_USED)
    {
        return FALSE;
    }
    *timer = t - 1u;
    if ((fp->input.pl.button_hold & PEACH_JUMP_BUTTONS) == 0)
    {
        *timer = PEACH_FLOAT_USED;
        return FALSE;
    }
    fp->physics.vel_air.y = 0.0F;
    return TRUE;
}

/* PeachFloat.check_float_ (the Jump, JumpAerial, Fall and Pass interrupts'
 * jump check): with a jump button held and no float yet, she floats at once
 * with the stick down, else once she is falling -- unless the press is a
 * double jump she still has, or her double jump is in its first 20 frames. */
static sb32 ndsP4PeachCheckFloat(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (*ndsP4PeachFloatTimer(fp) != 0u)
    {
        return FALSE;
    }
    if ((fp->input.pl.button_hold & PEACH_JUMP_BUTTONS) == 0)
    {
        return FALSE;
    }
    if (fp->input.pl.stick_range.y >= -39)
    {
        if ((fp->jumps_used != fp->attr->jumps_max) &&
            ((fp->input.pl.button_tap & PEACH_JUMP_BUTTONS) != 0))
        {
            return FALSE;
        }
        if (!(fp->physics.vel_air.y <= 0.0F))
        {
            return FALSE;
        }
        if (((fp->status_id == PEACH_ACTION_JUMP_AERIAL_F) ||
             (fp->status_id == PEACH_ACTION_JUMP_AERIAL_B)) &&
            (fighter_gobj->anim_frame <= 20.0F))
        {
            return FALSE;
        }
    }
    *ndsP4PeachFloatTimer(fp) = PEACH_FLOAT_TIMER;
    ndsP4PeachFloatInitial(fighter_gobj);
    return TRUE;
}

/* PeachFloat.fall_override_ (ftCommonFallSetStatus's status change): a
 * float in progress with a jump button held floats on (an aerial's end). */
static sb32 ndsP4PeachFallStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((ndsP4PeachFloating(fp) == FALSE) ||
        ((fp->input.pl.button_hold & PEACH_JUMP_BUTTONS) == 0))
    {
        return FALSE;
    }
    ndsP4PeachFloatInitial(fighter_gobj);
    return TRUE;
}

/* PeachFloat.prevent_item_throw_: no aerial item throw while floating. */
static sb32 ndsP4PeachAirItemThrowBlock(FTStruct *fp)
{
    return ndsP4PeachFloating(fp);
}

/* PeachFloat.end_float_on_hit_ (ftParamStopVoiceRunProcDamage's head). */
static void ndsP4PeachOnDamage(FTStruct *fp)
{
    if (*ndsP4PeachFloatTimer(fp) != 0u)
    {
        *ndsP4PeachFloatTimer(fp) = PEACH_FLOAT_USED;
    }
}

/* Peach.grounded_script_ (mpCommonSetFighterLandingParams' case): landing
 * gives the float back, a ledge catch does not (Peach.asm ledge_patch_ marks
 * the catch). The case then ends the switch, as the default does. */
static void ndsP4PeachOnLanding(FTStruct *fp, sb32 cliff)
{
    if (cliff == FALSE)
    {
        *ndsP4PeachFloatTimer(fp) = 0;
    }
}

/* Her status hooks after the source's status change:
 * PeachFloat.auto_cancel_aerials_ -- an aerial started while floating lands
 * without its landing lag (ftCommonAttackAirCheckInterruptCommon is the
 * only way into the aerial statuses but Link's down-air rehit);
 * Peach.asm restore_float_flag_ledge_action -- getting up from a ledge
 * (ftCommonCliffQuickOrSlowSetStatus, the only way into CliffQuick and
 * CliffSlow) gives the float back. */
void ndsP4PeachOnStatus(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((status_id >= nFTCommonStatusAttackAirN) && (status_id <= nFTCommonStatusAttackAirLw))
    {
        if (ndsP4PeachFloating(fp) != FALSE)
        {
            fp->proc_map = mpCommonProcFighterWaitOrLanding;
        }
    }
    else if ((status_id == nFTCommonStatusCliffQuick) || (status_id == nFTCommonStatusCliffSlow))
    {
        *ndsP4PeachFloatTimer(fp) = 0;
    }
}

/* ---- Neutral special (Peach Bomber) ---- */

static void ndsP4PeachNSPRecoilInitial(GObj *fighter_gobj);
static void ndsP4PeachNSPShieldClangHit(GObj *fighter_gobj);

static void ndsP4PeachNSPSetProcs(FTStruct *fp)
{
    fp->proc_shield = ndsP4PeachNSPShieldClangHit;
    fp->proc_hit = ndsP4PeachNSPRecoilInitial;
}

/* ftLinkSpecialNProcStatus (0x80163850), which her initials call: Link's
 * smash-boomerang test, here on status_vars' first word. */
static void ndsP4PeachNSPSmashTest(FTStruct *fp)
{
    s32 *is_smash = &((s32 *)(void *)&fp->status_vars)[0];

    fp->motion_vars.flags.flag0 = 0;
    if ((ABS(fp->input.pl.stick_range.x) >= 56) && (fp->hold_stick_x < 8))
    {
        *is_smash = TRUE;
        fp->stat_flags.is_smash_attack = TRUE;
    }
    else *is_smash = FALSE;
}

static void ndsP4PeachNSPInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, PEACH_PRESERVE_MODELPART);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ndsP4PeachNSPSmashTest(fp);
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

/* PeachNSP.ground_initial_ (ground_nsp). */
void ndsP4PeachNSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4PeachNSPInitial(fighter_gobj, PEACH_STATUS_NSPG);
    ndsP4PeachNSPSetProcs(ftGetStruct(fighter_gobj));
}

/* PeachNSP.air_initial_ (air_nsp): a small hop. */
void ndsP4PeachNSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4PeachNSPInitial(fighter_gobj, PEACH_STATUS_NSPA);
    ndsP4PeachClearFastFall(fp);
    fp->physics.vel_air.y = PEACH_NSP_Y_SPEED_INITIAL;
    ndsP4PeachNSPSetProcs(fp);
}

/* PeachNSP.recoil_initial_ (her hit routine, and her wall bounce). */
static void ndsP4PeachNSPRecoilInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
    }
    ftMainSetStatus(fighter_gobj, PEACH_STATUS_NSP_RECOIL, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    fp->physics.vel_air.x = PEACH_NSP_RECOIL_X_SPEED * (f32)fp->lr;
    fp->physics.vel_air.y = PEACH_NSP_RECOIL_Y_SPEED;
}

/* PeachNSP.shield_clang_hit_ (her shield routine): the move skips to its
 * end, frame 35 of the same status. */
static void ndsP4PeachNSPShieldClangHit(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, ftGetStruct(fighter_gobj)->status_id, PEACH_NSP_CLANG_FRAME,
                    1.0F, FTSTATUS_PRESERVE_NONE);
}

static f32 ndsP4PeachNSPSpeed(FTStruct *fp)
{
    return (((s32 *)(void *)&fp->status_vars)[0] != 0) ? PEACH_NSP_X_SMASH_SPEED
                                                         : PEACH_NSP_X_SPEED;
}

/* PeachNSP.ground_physics_ (0xDF physics). */
void ndsP4PeachNSPGroundPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    switch (fp->motion_vars.flags.flag2)
    {
    case PEACH_NSP_BEGIN_MOVE:
        fp->physics.vel_ground.x = ndsP4PeachNSPSpeed(fp);
        fp->motion_vars.flags.flag2 = PEACH_NSP_MOVE;
        ftPhysicsSetGroundVelTransferAir(fighter_gobj);
        break;

    case PEACH_NSP_MOVE:
        break;

    case PEACH_NSP_END_MOVE:
        fp->motion_vars.flags.flag2 = PEACH_NSP_END;
        break;

    default:
        ftPhysicsApplyGroundVelFriction(fighter_gobj);
        break;
    }
}

/* PeachNSP.air_end_physics_: the attribute gravity and fall cap, then the
 * air friction at 1.0 (the donor passes its stack as the attributes). */
static void ndsP4PeachNSPAirEndPhysics(FTStruct *fp)
{
    ftPhysicsApplyGravityClampTVel(fp, fp->attr->gravity, fp->attr->tvel_base);
    if (fp->physics.vel_air.x < 0.0F)
    {
        fp->physics.vel_air.x += PEACH_NSP_AIR_FRICTION;
        if (fp->physics.vel_air.x >= 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
    else
    {
        fp->physics.vel_air.x -= PEACH_NSP_AIR_FRICTION;
        if (fp->physics.vel_air.x <= 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
}

/* PeachNSP.air_physics_ (0xE0 physics). */
void ndsP4PeachNSPAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    switch (fp->motion_vars.flags.flag2)
    {
    case PEACH_NSP_BEGIN_MOVE:
        fp->physics.vel_air.x = ndsP4PeachNSPSpeed(fp) * (f32)fp->lr;
        fp->physics.vel_air.y = 0.0F;
        fp->motion_vars.flags.flag2 = PEACH_NSP_MOVE;
        break;

    case PEACH_NSP_MOVE:
        break;

    case PEACH_NSP_END_MOVE:
        fp->physics.vel_air.x *= 0.5F;
        fp->motion_vars.flags.flag2 = PEACH_NSP_END;
        ndsP4PeachNSPAirEndPhysics(fp);
        break;

    case PEACH_NSP_END:
        ndsP4PeachNSPAirEndPhysics(fp);
        break;

    default:
        ftPhysicsApplyAirVelDrift(fighter_gobj);
        break;
    }
}

/* PeachNSP.ground_to_air_ / air_to_ground_: the same swing on the other
 * side, the frame kept, her routines armed again. */
static void ndsP4PeachNSPGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, PEACH_STATUS_NSPA, fighter_gobj->anim_frame, 1.0F,
                    PEACH_PRESERVE_HIT);
    ndsP4PeachNSPSetProcs(fp);
}

static void ndsP4PeachNSPAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, PEACH_STATUS_NSPG, fighter_gobj->anim_frame, 1.0F,
                    PEACH_PRESERVE_HIT);
    ndsP4PeachNSPSetProcs(fp);
}

/* The wall the dash ran into last frame, on her facing's side. */
static sb32 ndsP4PeachNSPWall(FTStruct *fp)
{
    if (fp->motion_vars.flags.flag2 != PEACH_NSP_MOVE)
    {
        return FALSE;
    }
    return ((fp->coll_data.mask_prev & ((fp->lr >= 0) ? MAP_FLAG_LWALL : MAP_FLAG_RWALL)) != 0)
               ? TRUE : FALSE;
}

/* PeachNSP.ground_collision_ (0xDF map). */
void ndsP4PeachNSPGroundMap(GObj *fighter_gobj)
{
    if (mpCommonProcFighterOnFloor(fighter_gobj, ndsP4PeachNSPGroundToAir) == FALSE)
    {
        return;
    }
    if (ndsP4PeachNSPWall(ftGetStruct(fighter_gobj)) != FALSE)
    {
        ndsP4PeachNSPRecoilInitial(fighter_gobj);
    }
}

/* PeachNSP.air_collision_ (0xE0 map). */
void ndsP4PeachNSPAirMap(GObj *fighter_gobj)
{
    if (mpCommonProcFighterLanding(fighter_gobj, ndsP4PeachNSPAirToGround) != FALSE)
    {
        return;
    }
    if (ndsP4PeachNSPWall(ftGetStruct(fighter_gobj)) != FALSE)
    {
        ndsP4PeachNSPRecoilInitial(fighter_gobj);
    }
}

/* ---- Up special (the parasol) ---- */

static void ndsP4PeachUSPInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, PEACH_PRESERVE_MODELPART);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = PEACH_USP_BEGIN;
}

/* PeachUSP.air_initial_ (air_usp): the fall stopped at her gravity. */
void ndsP4PeachUSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4PeachUSPInitial(fighter_gobj, PEACH_STATUS_USPA);
    ndsP4PeachClearFastFall(fp);
    fp->physics.vel_air.y = fp->attr->gravity;
}

/* PeachUSP.ground_initial_ (ground_usp). */
void ndsP4PeachUSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4PeachUSPInitial(fighter_gobj, PEACH_STATUS_USPG);
}

/* PeachUSP.open_initial_ / float_initial_ / close_initial_. */
static void ndsP4PeachUSPOpenInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, PEACH_STATUS_USP_OPEN, 0.0F, 1.0F, PEACH_PRESERVE_MODELPART);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag1 = 1;
    ndsP4PeachClearFastFall(fp);
    fp->physics.vel_air.y = 0.0F;
}

static void ndsP4PeachUSPStageInitial(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, PEACH_PRESERVE_MODELPART);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftGetStruct(fighter_gobj)->motion_vars.flags.flag1 = 1;
}

/* PeachUSP.main_ (0xE3/0xE4 update): the parasol opens at the end. */
void ndsP4PeachUSPMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ndsP4PeachUSPOpenInitial(fighter_gobj);
    }
}

/* PeachUSP.open_main_ (0xE5 update): then floats. */
void ndsP4PeachUSPOpenMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ndsP4PeachUSPStageInitial(fighter_gobj, PEACH_STATUS_USP_FLOAT);
    }
}

/* PeachUSP.close_main_ (0xE7 update): Fox's Fire Fox end with her landing
 * lag and no interrupt; FallSpecial becomes her parasol fall (below). */
void ndsP4PeachUSPCloseMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, 1.0F, FALSE, TRUE, FALSE,
                                     PEACH_USP_LANDING_FSM, FALSE);
    }
}

/* PeachUSP.change_direction_ (0xE3/0xE4 interrupt). */
void ndsP4PeachUSPChangeDirection(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->motion_vars.flags.flag1 == 2)
    {
        ftCaptainSpecialHiProcInterrupt(fighter_gobj);
    }
}

/* PeachUSP.float_interrupt_ (0xE6 interrupt): the stick down closes it. */
void ndsP4PeachUSPFloatInterrupt(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->input.pl.stick_range.y < -39)
    {
        ndsP4PeachUSPStageInitial(fighter_gobj, PEACH_STATUS_USP_CLOSE);
    }
}

/* PeachUSP.fall_interrupt_ (0xE8 interrupt, in place of FallSpecial's): the
 * stick up opens it again. */
void ndsP4PeachUSPFallInterrupt(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->input.pl.stick_range.y >= 40)
    {
        ndsP4PeachUSPOpenInitial(fighter_gobj);
    }
}

/* PeachUSP.fall_special_patch_ (ftCommonFallSpecialSetStatus' status): the
 * parasol's close falls in her parasol fall. */
static s32 ndsP4PeachFallSpecialStatus(FTStruct *fp, s32 status_id)
{
    return (fp->status_id == PEACH_STATUS_USP_CLOSE) ? PEACH_STATUS_USP_FALL : status_id;
}

/* PeachUSP.physics_ (0xE3/0xE4 physics). */
void ndsP4PeachUSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    if (fp->ga == nMPKineticsGround)
    {
        ftPhysicsApplyGroundVelFriction(fighter_gobj);
        return;
    }
    /* The original air physics (copied), the drift only while moving. */
    if (fp->is_fastfall)
    {
        ftPhysicsApplyFastFall(fp, attr);
    }
    else ftPhysicsApplyGravityDefault(fp, attr);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        if (fp->motion_vars.flags.flag2 == PEACH_USP_MOVE)
        {
            ftPhysicsClampAirVelXStickRange(fp, 8, ndsP4PeachBitsToF32(0x3C240000u),
                                            PEACH_USP_AIR_SPEED);
        }
        else (void)ftPhysicsCheckClampAirVelXDecMax(fp, attr);

        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
    if ((fp->motion_vars.flags.flag2 == PEACH_USP_BEGIN) &&
        (fp->status_id != PEACH_STATUS_USPG))
    {
        fp->physics.vel_air.x *= 0.875F;
        fp->physics.vel_air.y = 0.0F;
    }
    if (fp->motion_vars.flags.flag2 == PEACH_USP_BEGIN_MOVE)
    {
        f32 speed_y = (fp->status_id == PEACH_STATUS_USPG) ? PEACH_USP_GROUND_Y_SPEED
                                                           : PEACH_USP_AIR_Y_SPEED;

        fp->motion_vars.flags.flag2 = PEACH_USP_MOVE;
        fp->jumps_used = (u8)attr->jumps_max;
        fp->physics.vel_air.x = (f32)fp->lr * PEACH_USP_X_SPEED;
        fp->physics.vel_air.y = speed_y;
    }
    if (fp->motion_vars.flags.flag2 == PEACH_USP_END_MOVE)
    {
        fp->physics.vel_air.x *= 0.875F;
    }
}

/* PeachUSP.float_physics_ (0xE5/0xE6 physics): the drift with the
 * parasol's gravity and fall cap and no fast fall. */
void ndsP4PeachUSPFloatPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ftPhysicsApplyGravityClampTVel(fp, PEACH_USP_FLOAT_GRAVITY, PEACH_USP_FLOAT_FALL_SPEED);
    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsClampAirVelXStickDefault(fp, attr);
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

/* PeachUSP.collision_ (0xE3-0xE7 map): Mario's Super Jump map with her
 * landing lag; the copied delay slot keeps the low half of Mario's constant
 * (0x3EC05C29). */
void ndsP4PeachUSPMap(GObj *fighter_gobj)
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
                                                     ndsP4PeachBitsToF32(0x3EC05C29u));
        }
    }
    else mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
}

const NDSP4Overrides gNdsP4PeachOverrides = {
    .gravity = ndsP4PeachGravity,
    .air_jump_check = ndsP4PeachCheckFloat,
    .fall_status = ndsP4PeachFallStatus,
    .air_item_throw_block = ndsP4PeachAirItemThrowBlock,
    .on_damage = ndsP4PeachOnDamage,
    .on_landing = ndsP4PeachOnLanding,
    .fall_special_status = ndsP4PeachFallSpecialStatus,
};

#endif /* NDS_P4_PEACH */
