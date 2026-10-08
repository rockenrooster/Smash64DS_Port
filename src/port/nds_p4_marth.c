/*
 * P4 Marth: native ports of the donor's special routines. Source:
 * JSsixtyfour/smashremix 5e04fe7, src/Marth/MarthSpecial.asm and Marth.asm,
 * read as assembled (scripts/p4/mipsdis.py): the OS.copy_segment blocks are
 * the original game's code, so their exact constants come from the build.
 * Marth runs Captain's status code (fp->fkind == nFTKindCaptain).
 *
 * Roy shares the up and neutral specials (MarthUSP/MarthNSP name him in
 * their own id tests); nds_p4_roy.c compiles this file when Marth is not
 * enabled. The counter (MarthDSP) is Marth's alone: Remix's damage hook
 * names only him.
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x17C/0x180/0x184 temp variables 1-3   motion_vars.flags.flag0/1/2
 *   0x18D & 0x07                           clears is_absorb, absorb_lr,
 *                                          is_goto_attack100 and is_fastfall
 *   0x1BE buttons tapped                   input.pl.button_tap
 *   0x1C2/0x1C3 stick x/y                  input.pl.stick_range.x/y
 *   0x7E8 armour                           knockback_resist_status
 *   0x7FC hit direction                    damage_lr
 *   0x8E8 + 4n                             joints[n]
 *   0xADC aerial neutral-B boost flag      passive_vars, first word (Mario's
 *                                          grounded_script clears it)
 *   0xB18 counter hit / 0xB20 NSP stage    status_vars, words 0 and 2
 *   attributes + 0x58/0x64                 gravity, jumps_max
 */
#include <nds/nds_p4.h>

#if NDS_P4_MARTH || NDS_P4_ROY

sb32 ftMarioSpecialHiProcPass(GObj *fighter_gobj);
void ftCaptainSpecialHiProcInterrupt(GObj *fighter_gobj);

/* Marth.Action (his action array; Roy's numbers are the same). */
#define MARTH_STATUS_USPG 0xDE
#define MARTH_STATUS_USPA 0xDF
#define MARTH_STATUS_NSPG_1 0xE0
#define MARTH_STATUS_NSPG_2_MID 0xE2
#define MARTH_STATUS_NSPG_3_MID 0xE5
#define MARTH_STATUS_NSPA_1 0xE7
#define MARTH_STATUS_NSPA_2_MID 0xE9
#define MARTH_STATUS_NSPA_3_MID 0xEC
#define MARTH_STATUS_DSPG 0xEF
#define MARTH_STATUS_DSPG_ATTACK 0xF0
#define MARTH_STATUS_DSPA 0xF1
#define MARTH_STATUS_DSPA_ATTACK 0xF2
/* The NSP air statuses are the ground ones plus 7; DSP's plus 2. */
#define MARTH_NSP_AIR_OFFSET 7
#define MARTH_DSP_AIR_OFFSET 2

/* MarthUSP temp variable 3 states. */
#define MARTH_USP_BEGIN 1
#define MARTH_USP_BEGIN_MOVE 2
#define MARTH_USP_MOVE 3
#define MARTH_USP_END_MOVE 4
#define MARTH_USP_END 5

/* Joypad.B in button_tap / button_hold. */
#define MARTH_BUTTON_B 0x4000u

/* Status-preserve flags the donor passes as raw words. */
#define MARTH_PRESERVE_NSP 0x2003u  /* after-image, colanim, hit */
#define MARTH_PRESERVE_DSP 0x2803u  /* the same and the loop SFX */

static f32 ndsP4MarthBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

static sb32 ndsP4MarthIsRoy(FTStruct *fp)
{
#if NDS_P4_ROY
    return (ndsP4Content(fp) == NDS_P4_CONTENT_ROY) ? TRUE : FALSE;
#else
    (void)fp;
    return FALSE;
#endif
}

/* The donor's `sb v1, 0x18D` with v1 &= 7: four flag bits cleared. */
static void ndsP4MarthClearFastFall(FTStruct *fp)
{
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
}

/* ---- Up special (Dolphin Slash) ---- */

/* MarthUSP.air_initial_ (air_usp). */
void ndsP4MarthUSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, MARTH_STATUS_USPA, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = MARTH_USP_BEGIN;
    ndsP4MarthClearFastFall(fp);
    /* "freeze y position": the y velocity takes the gravity value. */
    fp->physics.vel_air.y = fp->attr->gravity;
}

/* MarthUSP.ground_initial_ (ground_usp). */
void ndsP4MarthUSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, MARTH_STATUS_USPG, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = MARTH_USP_BEGIN;
}

/* MarthUSP.main_ (0xDE/0xDF update): Fox's Fire Fox end with his landing
 * lag (0.375) and no interrupt. */
void ndsP4MarthUSPMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, 1.0F, FALSE, TRUE, FALSE, 0.375F, FALSE);
    }
}

/* MarthUSP.change_direction_ (0xDE/0xDF interrupt): Captain's turn while
 * the script holds temp variable 2 at 2. */
void ndsP4MarthUSPChangeDirection(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 == 2)
    {
        ftCaptainSpecialHiProcInterrupt(fighter_gobj);
    }
}

/* MarthUSP.air_control_: the stick drift at 0.00977 and 24. */
static void ndsP4MarthUSPAirControl(FTStruct *fp, FTAttributes *attr)
{
    (void)attr;
    ftPhysicsClampAirVelXStickRange(fp, 8, ndsP4MarthBitsToF32(0x3C200000u), 24.0F);
}

/* MarthUSP.physics_ (0xDE/0xDF physics). */
void ndsP4MarthUSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    sb32 roy = ndsP4MarthIsRoy(fp);

    if (fp->ga == nMPKineticsGround)
    {
        ftPhysicsApplyGroundVelFriction(fighter_gobj);
        return;
    }
    /* The original air physics (copied), the drift replaced: none until
     * the ending, then his own. */
    if (fp->is_fastfall)
    {
        ftPhysicsApplyFastFall(fp, attr);
    }
    else ftPhysicsApplyGravityDefault(fp, attr);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        if (fp->motion_vars.flags.flag2 == MARTH_USP_END)
        {
            ndsP4MarthUSPAirControl(fp, attr);
        }
        else (void)ftPhysicsCheckClampAirVelXDecMax(fp, attr);

        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
    if ((fp->motion_vars.flags.flag2 == MARTH_USP_BEGIN) &&
        (fp->status_id != MARTH_STATUS_USPG))
    {
        fp->physics.vel_air.x *= 0.875F;
        fp->physics.vel_air.y = 0.0F;
    }
    if (fp->motion_vars.flags.flag2 == MARTH_USP_BEGIN_MOVE)
    {
        f32 speed_y;
        f32 add_x;
        f32 stick;

        if (roy != FALSE)
        {
            speed_y = (fp->status_id == MARTH_STATUS_USPG) ? 120.0F : 116.0F;
        }
        else speed_y = (fp->status_id == MARTH_STATUS_USPG) ? 420.0F : 400.0F;

        stick = (f32)fp->input.pl.stick_range.x * (f32)fp->lr;
        /* The donor compares against 10 in f2 and, below it, adds that
         * same 10 again: a stick short of 10 forward drives 20. */
        add_x = 10.0F;
        if (10.0F <= stick)
        {
            add_x = stick * ((roy != FALSE) ? 0.75F : 2.0F);
            speed_y -= 0.5F * add_x;
        }
        fp->physics.vel_air.x = (f32)fp->lr * (add_x + 10.0F);
        fp->physics.vel_air.y = speed_y;
        fp->motion_vars.flags.flag2 = MARTH_USP_MOVE;
        fp->jumps_used = (u8)attr->jumps_max;
    }
    if (fp->motion_vars.flags.flag2 == MARTH_USP_END_MOVE)
    {
        fp->physics.vel_air.x = (fp->physics.vel_air.x * 0.125F) + (10.0F * (f32)fp->lr);
        fp->physics.vel_air.y *= (roy != FALSE) ? 0.375F : 0.09375F;
        fp->motion_vars.flags.flag2 = MARTH_USP_END;
    }
}

/* MarthUSP.collision_ (0xDE/0xDF map): Mario's Super Jump map with his
 * landing lag. The `ori a2, a2, 0x5C29` of the copied delay slot keeps
 * the low half of Mario's constant: 0x3EC05C29, not 0.375. */
void ndsP4MarthUSPMap(GObj *fighter_gobj)
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
                                                     ndsP4MarthBitsToF32(0x3EC05C29u));
        }
    }
    else mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
}

/* ---- Neutral special (Dancing Blade) ---- */

/* MarthNSP.ground_shared_initial_ / air_shared_initial_. The Kirby
 * branches (his Roy hat's action ids) are S8's. */
static void ndsP4MarthNSPSharedInitial(GObj *fighter_gobj, s32 status_id, sb32 air)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
    if (air != FALSE)
    {
        u32 *boost = (u32 *)(void *)&fp->passive_vars;

        ndsP4MarthClearFastFall(fp);
        fp->physics.vel_air.x *= 0.875F;
        if (status_id != MARTH_STATUS_NSPA_1)
        {
            fp->physics.vel_air.y = 8.0F;           /* Y_SPEED_SECOND */
        }
        else if (*boost != 0u)
        {
            fp->physics.vel_air.y = 16.0F;          /* Y_SPEED_STALE */
        }
        else
        {
            fp->physics.vel_air.y = 36.0F;          /* Y_SPEED */
            *boost = TRUE;
        }
    }
}

static s32 *ndsP4MarthNSPStage(FTStruct *fp)
{
    return &((s32 *)(void *)&fp->status_vars)[2];
}

/* The second and third swings pick high, middle or low by the stick. */
static s32 ndsP4MarthNSPStickStatus(FTStruct *fp, s32 mid)
{
    s32 stick_y = fp->input.pl.stick_range.y;

    if (stick_y >= 40)
    {
        return mid - 1;
    }
    if (stick_y < -39)
    {
        return mid + 1;
    }
    return mid;
}

/* MarthNSP.ground_1_initial_ (ground_nsp). */
void ndsP4MarthNSPGround1Initial(GObj *fighter_gobj)
{
    *ndsP4MarthNSPStage(ftGetStruct(fighter_gobj)) = 0;
    ndsP4MarthNSPSharedInitial(fighter_gobj, MARTH_STATUS_NSPG_1, FALSE);
}

/* MarthNSP.air_1_initial_ (air_nsp). */
void ndsP4MarthNSPAir1Initial(GObj *fighter_gobj)
{
    *ndsP4MarthNSPStage(ftGetStruct(fighter_gobj)) = 0;
    ndsP4MarthNSPSharedInitial(fighter_gobj, MARTH_STATUS_NSPA_1, TRUE);
}

static void ndsP4MarthNSPNextStage(GObj *fighter_gobj, sb32 air)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *stage = ndsP4MarthNSPStage(fp);
    s32 mid;

    if (*stage == 0)
    {
        mid = (air != FALSE) ? MARTH_STATUS_NSPA_2_MID : MARTH_STATUS_NSPG_2_MID;
        *stage = 1;
    }
    else
    {
        mid = (air != FALSE) ? MARTH_STATUS_NSPA_3_MID : MARTH_STATUS_NSPG_3_MID;
        *stage = 2;
    }
    ndsP4MarthNSPSharedInitial(fighter_gobj, ndsP4MarthNSPStickStatus(fp, mid), air);
}

/* MarthNSP.ground_main_ / air_main_: B in the script's window starts the
 * next swing; the end of the animation goes to wait or fall. */
static void ndsP4MarthNSPMain(GObj *fighter_gobj, sb32 air)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp->motion_vars.flags.flag1 != 0) &&
        ((fp->input.pl.button_tap & MARTH_BUTTON_B) != 0u))
    {
        s32 stage = *ndsP4MarthNSPStage(fp);

        if ((stage == 0) || (stage == 1))
        {
            ndsP4MarthNSPNextStage(fighter_gobj, air);
            return;
        }
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

void ndsP4MarthNSPGroundMain(GObj *fighter_gobj)
{
    ndsP4MarthNSPMain(fighter_gobj, FALSE);
}

void ndsP4MarthNSPAirMain(GObj *fighter_gobj)
{
    ndsP4MarthNSPMain(fighter_gobj, TRUE);
}

/* MarthNSP.ground_to_air_ / air_to_ground_: the same swing on the other
 * side, the frame kept. */
static void ndsP4MarthNSPGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id + MARTH_NSP_AIR_OFFSET,
                    fighter_gobj->anim_frame, 1.0F, MARTH_PRESERVE_NSP);
    ftPhysicsClampAirVelXMax(fp);
}

static void ndsP4MarthNSPAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id - MARTH_NSP_AIR_OFFSET,
                    fighter_gobj->anim_frame, 1.0F, MARTH_PRESERVE_NSP);
}

/* MarthNSP.ground_collision_ / air_collision_. */
void ndsP4MarthNSPGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4MarthNSPGroundToAir);
}

void ndsP4MarthNSPAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4MarthNSPAirToGround);
}

#if NDS_P4_MARTH
/* ---- Down special (Counter), Marth's alone ---- */

static s32 *ndsP4MarthDSPHit(FTStruct *fp)
{
    return &((s32 *)(void *)&fp->status_vars)[0];
}

static void ndsP4MarthDSPInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
    *ndsP4MarthDSPHit(fp) = 0;
}

/* MarthDSP.ground_initial_ (ground_dsp). */
void ndsP4MarthDSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4MarthDSPInitial(fighter_gobj, MARTH_STATUS_DSPG);
}

/* MarthDSP.air_initial_ (air_dsp): the fall stopped, half the drift. */
void ndsP4MarthDSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4MarthDSPInitial(fighter_gobj, MARTH_STATUS_DSPA);
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.x *= 0.5F;
}

/* MarthDSP.ground_attack_initial_ / air_attack_initial_. Remix's Pokemon
 * Stadium announcer line is not in the port. */
static void ndsP4MarthDSPAttackInitial(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* MarthDSP.main_ (0xEF/0xF1 update): armour while the script holds temp
 * variable 1; a counted hit turns him to face it and strikes back. */
void ndsP4MarthDSPMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->knockback_resist_status = 0.0F;
    if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->knockback_resist_status = ndsP4MarthBitsToF32(0x48000000u);
    }
    if (*ndsP4MarthDSPHit(fp) != 0)
    {
        fp->lr = fp->damage_lr;
        fp->joints[nFTPartsJointTopN]->rotate.vec.f.y =
            ndsP4MarthBitsToF32(0x3FC90FDBu) * (f32)fp->lr;
        if (fp->ga != nMPKineticsGround)
        {
            ndsP4MarthDSPAttackInitial(fighter_gobj, MARTH_STATUS_DSPA_ATTACK);
        }
        else ndsP4MarthDSPAttackInitial(fighter_gobj, MARTH_STATUS_DSPG_ATTACK);
        return;
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* MarthDSP.air_physics_ (0xF1/0xF2): no control, a slow rise. */
void ndsP4MarthDSPAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsApplyAirVelFriction(fighter_gobj);
    fp->physics.vel_air.y += 1.5F;
}

static void ndsP4MarthDSPGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id + MARTH_DSP_AIR_OFFSET,
                    fighter_gobj->anim_frame, 1.0F, MARTH_PRESERVE_DSP);
    ftPhysicsClampAirVelXMax(fp);
}

static void ndsP4MarthDSPAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id - MARTH_DSP_AIR_OFFSET,
                    fighter_gobj->anim_frame, 1.0F, MARTH_PRESERVE_DSP);
}

/* MarthDSP.ground_collision_ / air_collision_. */
void ndsP4MarthDSPGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4MarthDSPGroundToAir);
}

void ndsP4MarthDSPAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4MarthDSPAirToGround);
}

/* MarthDSP.detection_patch_ (ftParamUpdateDamage's head): a hit landing
 * while the counter's window is open deals nothing and is counted. */
static s32 ndsP4MarthUpdateDamage(FTStruct *fp, s32 damage)
{
    if (((fp->status_id == MARTH_STATUS_DSPG) || (fp->status_id == MARTH_STATUS_DSPA)) &&
        (fp->motion_vars.flags.flag0 != 0))
    {
        *ndsP4MarthDSPHit(fp) = 1;
        return 0;
    }
    return damage;
}

const NDSP4Overrides gNdsP4MarthOverrides = {
    .update_damage = ndsP4MarthUpdateDamage,
};
#endif /* NDS_P4_MARTH */

#endif /* NDS_P4_MARTH || NDS_P4_ROY */
