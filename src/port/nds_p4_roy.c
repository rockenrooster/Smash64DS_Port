/*
 * P4 Roy: native ports of the donor's down special (Flare Blade). Source:
 * JSsixtyfour/smashremix 5e04fe7, src/Roy/RoySpecial.asm and Roy.asm, read
 * as assembled (scripts/p4/mipsdis.py). Roy runs Captain's status code
 * (fp->fkind == nFTKindCaptain) and Marth's up and neutral specials
 * (nds_p4_marth.c), which this file compiles when Marth is not enabled.
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x17C/0x180/0x184 temp variables 1-3   motion_vars.flags.flag0/1/2
 *   0x1BC buttons held                     input.pl.button_hold
 *   0x294 + 0xC4n hitbox n                 attack_colls[n] (state +0,
 *                                          damage +0xC, shield damage +0x38)
 *   0xB18 charge level                     status_vars, first word
 */
#include <nds/nds_p4.h>

#if NDS_P4_ROY && !NDS_P4_MARTH
#include "nds_p4_marth.c"
#endif

#if NDS_P4_ROY

/* Roy.Action. */
#define ROY_STATUS_DSPG_BEGIN 0xEF
#define ROY_STATUS_DSPG_WAIT 0xF0
#define ROY_STATUS_DSPG_END 0xF1
#define ROY_STATUS_DSPG_STRONG_END 0xF2
#define ROY_STATUS_DSPA_BEGIN 0xF3
#define ROY_STATUS_DSPA_WAIT 0xF4
#define ROY_STATUS_DSPA_END 0xF5
#define ROY_STATUS_DSPA_STRONG_END 0xF6
/* The air statuses are the ground ones plus 4. */
#define ROY_DSP_AIR_OFFSET 4
#define ROY_DSP_MAX_CHARGE 21
#define ROY_BUTTON_B 0x4000u
/* After-image, loop SFX, colanim, hit. */
#define ROY_PRESERVE_DSP 0x2803u

static s32 *ndsP4RoyCharge(FTStruct *fp)
{
    return &((s32 *)(void *)&fp->status_vars)[0];
}

static void ndsP4RoySetStatus(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

static void ndsP4RoyDSPBeginInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4RoySetStatus(fighter_gobj, status_id);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
    *ndsP4RoyCharge(fp) = 0;
}

/* RoyDSP.ground_begin_initial_ (ground_dsp). */
void ndsP4RoyDSPGroundBeginInitial(GObj *fighter_gobj)
{
    ndsP4RoyDSPBeginInitial(fighter_gobj, ROY_STATUS_DSPG_BEGIN);
}

/* RoyDSP.air_begin_initial_ (air_dsp). */
void ndsP4RoyDSPAirBeginInitial(GObj *fighter_gobj)
{
    ndsP4RoyDSPBeginInitial(fighter_gobj, ROY_STATUS_DSPA_BEGIN);
}

static void ndsP4RoyDSPGroundWaitInitial(GObj *fighter_gobj)
{
    ndsP4RoySetStatus(fighter_gobj, ROY_STATUS_DSPG_WAIT);
}

static void ndsP4RoyDSPAirWaitInitial(GObj *fighter_gobj)
{
    ndsP4RoySetStatus(fighter_gobj, ROY_STATUS_DSPA_WAIT);
}

/* RoyDSP.ground_begin_main_ / air_begin_main_ (0xEF/0xF3 update). The
 * donor's early release (temp variable 2 set, B up) calls the end status
 * with whatever action id a1 held; DSP_BEGIN.bin never sets temp variable 2,
 * so that path is dead, and here it takes the plain end. */
static void ndsP4RoyDSPBeginMain(GObj *fighter_gobj, s32 end_status,
                                 void (*proc_wait)(GObj *))
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp->motion_vars.flags.flag1 != 0) &&
        ((fp->input.pl.button_hold & ROY_BUTTON_B) == 0u))
    {
        ndsP4RoySetStatus(fighter_gobj, end_status);
        return;
    }
    ftAnimEndCheckSetStatus(fighter_gobj, proc_wait);
}

void ndsP4RoyDSPGroundBeginMain(GObj *fighter_gobj)
{
    ndsP4RoyDSPBeginMain(fighter_gobj, ROY_STATUS_DSPG_END, ndsP4RoyDSPGroundWaitInitial);
}

void ndsP4RoyDSPAirBeginMain(GObj *fighter_gobj)
{
    ndsP4RoyDSPBeginMain(fighter_gobj, ROY_STATUS_DSPA_END, ndsP4RoyDSPAirWaitInitial);
}

/* RoyDSP.ground_wait_main_ / air_wait_main_ (0xF0/0xF4 update): each
 * script tick (temp variable 1) charges one level; the 21st level ends in
 * the strong swing, letting go of B in the plain one. */
static void ndsP4RoyDSPWaitMain(GObj *fighter_gobj, s32 strong_status, s32 end_status)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag0 != 0)
    {
        s32 *charge = ndsP4RoyCharge(fp);

        fp->motion_vars.flags.flag0 = 0;
        (*charge)++;
        if (*charge == ROY_DSP_MAX_CHARGE)
        {
            ndsP4RoySetStatus(fighter_gobj, strong_status);
            return;
        }
    }
    if ((fp->input.pl.button_hold & ROY_BUTTON_B) == 0u)
    {
        ndsP4RoySetStatus(fighter_gobj, end_status);
    }
}

void ndsP4RoyDSPGroundWaitMain(GObj *fighter_gobj)
{
    ndsP4RoyDSPWaitMain(fighter_gobj, ROY_STATUS_DSPG_STRONG_END, ROY_STATUS_DSPG_END);
}

void ndsP4RoyDSPAirWaitMain(GObj *fighter_gobj)
{
    ndsP4RoyDSPWaitMain(fighter_gobj, ROY_STATUS_DSPA_STRONG_END, ROY_STATUS_DSPA_END);
}

/* RoyDSP.end_main_ (0xF1/0xF2/0xF5/0xF6 update): the strong swing's
 * recoil (10% when the script sets temp variable 3), then each hitbox
 * that has just opened takes 2 damage a level and 8 + 2 a level shield
 * damage, halved for each next one; the end of the animation goes to
 * wait or fall. */
void ndsP4RoyDSPEndMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 charge;
    s32 shield;
    s32 i;

    if (fp->motion_vars.flags.flag2 != 0)
    {
        ftParamUpdateDamage(fp, 10);
        fp->motion_vars.flags.flag2 = 0;
    }
    charge = *ndsP4RoyCharge(fp);
    shield = charge + 8;
    for (i = 0; i < 4; i++)
    {
        FTAttackColl *coll = &fp->attack_colls[i];

        if (coll->attack_state == nGMAttackStateNew)
        {
            coll->damage += charge * 2;
            coll->shield_damage = shield;
            shield = (s32)((u32)shield >> 1);
        }
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

static void ndsP4RoyDSPGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id + ROY_DSP_AIR_OFFSET,
                    fighter_gobj->anim_frame, 1.0F, ROY_PRESERVE_DSP);
    ftPhysicsClampAirVelXMax(fp);
}

static void ndsP4RoyDSPAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id - ROY_DSP_AIR_OFFSET,
                    fighter_gobj->anim_frame, 1.0F, ROY_PRESERVE_DSP);
}

/* RoyDSP.ground_collision_ / air_collision_. */
void ndsP4RoyDSPGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4RoyDSPGroundToAir);
}

void ndsP4RoyDSPAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4RoyDSPAirToGround);
}

#endif /* NDS_P4_ROY */
