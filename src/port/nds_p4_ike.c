/*
 * P6 Ike: the routines of his authored specials (docs/P6/characters/ike.md,
 * move card I1): Quick Draw (neutral special), Aether (up special) and the
 * counter his taunt starts. Ike runs Captain's status code (fp->fkind ==
 * nFTKindCaptain) as Roy, his base, does. His down special stays Roy's
 * Flare Blade (Eruption) and his entry Roy's: nds_p4_roy.c, which this file
 * compiles when Roy's code is not compiled.
 *
 * The statuses are Ike's appended rows (scripts/p6/ike/spec.json "statuses";
 * each ground status's air twin follows it), entered from his special
 * starters (the spec's ground_nsp/air_nsp/ground_usp/air_usp rows) and, for
 * the counter, from the taunt (NDSP4Overrides.status_replace keeps Appeal in
 * fp->status_id). Their scripts (scripts/p6/ike/motions/) set the flags read
 * here: Aether's flag2 at the jump and flag1 while the rise has armour, the
 * counter's flag0 while its window is open.
 *
 * status_vars words: 0 the Quick Draw charge (frames held) or the counter's
 * hit, 1 the dash frames left.
 */
#include <nds/nds_p4.h>

#if NDS_P4_CODE_IKE

#include <macros.h>
#include <sys/obj.h>

#if !NDS_P4_CODE_ROY
#include "nds_p4_roy.c"
#endif

void ndsP4RoyEntryCase(FTStruct *fp);

/* scripts/p6/ike/spec.json "statuses". */
#define IKE_STATUS_ATTACK13 0xF9
#define IKE_STATUS_QD_CHARGE 0xFA
#define IKE_STATUS_QD_DASH 0xFC
#define IKE_STATUS_QD_SLASH 0xFE
#define IKE_STATUS_QD_END 0x100
#define IKE_STATUS_AETHER_RISE 0x102
#define IKE_STATUS_AETHER_RISE_AIR 0x103
#define IKE_STATUS_AETHER_FALL 0x104
#define IKE_STATUS_AETHER_LANDING 0x105
#define IKE_STATUS_COUNTER 0x106
#define IKE_STATUS_COUNTER_ATTACK 0x107
/* A Quick Draw status's air twin is the next id. */
#define IKE_QD_AIR 1

#define IKE_BUTTON_B 0x4000u
/* After-image, colour animation and hitboxes carried across a ground/air
 * switch (the raw word Roy's and Marth's ports pass). */
#define IKE_PRESERVE_SWITCH 0x2003u

/* Quick Draw (card defaults): up to 75 frames of charge (60 in the card;
 * owner playtest 2026-10-14, p6d: 1.25x as long to the farthest dash); a
 * dash of 6 to 20 frames by charge (owner p6v: the shortest 0.75x of its 8,
 * the longest kept) at four times his run speed (three times Roy's: owner
 * p6r slowed his run to 0.75); the slash on contact gains up to 7 damage by
 * charge (Ultimate's 6-13). */
#define IKE_QD_CHARGE_MAX 75
#define IKE_QD_DASH_MIN 6
#define IKE_QD_DASH_RANGE 14
#define IKE_QD_SPEED_MUL 4.0F
#define IKE_QD_SLASH_BONUS 7
#define IKE_QD_AIR_FALL_MAX 8.0F

/* Aether (owner p6p: "throws sword up forcefully, pause, jumps to sword,
 * pause for spin, comes back down"; owner p6r: Ultimate's timing): the leap
 * Ultimate's (d02specialhi2's Trans y over its frames 21-26, 1,594 units for
 * Ike's size, drawn over eight frames at 0.85 of its height), a unit a frame
 * after the script's take-off (flag2), then he hangs for the spin until the
 * clip ends; from the air 0.9 as high. scripts/p6/ike/anims recipe
 * aether_rise ("item_path" "rise") hangs Ragnell on the same table. The
 * plunge's speed and the stick drift during the rise (Final Cutter's,
 * ftkirbyspecialhi.c). */
static const f32 sNdsP4IkeAetherRise[] = { 96.0F, 250.0F, 280.0F, 240.0F, 190.0F, 140.0F, 100.0F, 59.0F };
#define IKE_AETHER_RISE_AIR_MUL 0.9F
#define IKE_AETHER_FALL_VEL 130.0F
#define IKE_AETHER_DRIFT_MUL 0.5F
#define IKE_AETHER_ARMOUR 200.0F
/* The counter's window holds him as Marth's does (0x48000000). */
#define IKE_COUNTER_ARMOUR 131072.0F

static s32 *ndsP4IkeVar(FTStruct *fp, s32 k)
{
    return &((s32 *)(void *)&fp->status_vars)[k];
}

static void ndsP4IkeSetStatus(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

/* A Quick Draw status's ground or air twin, at the same frame. */
static void ndsP4IkeQuickDrawToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id + IKE_QD_AIR, fighter_gobj->anim_frame, 1.0F,
                    IKE_PRESERVE_SWITCH);
    ftPhysicsClampAirVelXMax(fp);
}

static void ndsP4IkeQuickDrawToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id - IKE_QD_AIR, fighter_gobj->anim_frame, 1.0F,
                    IKE_PRESERVE_SWITCH);
}

void ndsP4IkeQuickDrawGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4IkeQuickDrawToAir);
}

void ndsP4IkeQuickDrawAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4IkeQuickDrawToGround);
}

/* The dash runs off an edge (the other statuses stop at one): it goes on in
 * the air and ends in special fall (card). */
void ndsP4IkeQuickDrawDashGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4IkeQuickDrawToAir);
}

/* ---- Quick Draw: charge ---- */

static void ndsP4IkeQuickDrawStart(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4IkeSetStatus(fighter_gobj, status_id);
    *ndsP4IkeVar(fp, 0) = 0;
    *ndsP4IkeVar(fp, 1) = 0;
}

/* ground_nsp. */
void ndsP4IkeQuickDrawGroundInitial(GObj *fighter_gobj)
{
    ndsP4IkeQuickDrawStart(fighter_gobj, IKE_STATUS_QD_CHARGE);
}

/* air_nsp: the fall stopped. */
void ndsP4IkeQuickDrawAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4IkeQuickDrawStart(fighter_gobj, IKE_STATUS_QD_CHARGE + IKE_QD_AIR);
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.x *= 0.5F;
}

static void ndsP4IkeQuickDrawDashStart(GObj *fighter_gobj);

/* Charge update: each frame held adds one; letting go of B, or the full
 * charge, starts the dash. */
void ndsP4IkeQuickDrawChargeMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *charge = ndsP4IkeVar(fp, 0);

    if (*charge < IKE_QD_CHARGE_MAX)
    {
        (*charge)++;
    }
    if ((*charge >= IKE_QD_CHARGE_MAX) || ((fp->input.pl.button_hold & IKE_BUTTON_B) == 0u))
    {
        ndsP4IkeQuickDrawDashStart(fighter_gobj);
    }
}

/* The air charge sinks slowly. */
void ndsP4IkeQuickDrawChargeAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsApplyAirVelFriction(fighter_gobj);
    if (fp->physics.vel_air.y > -IKE_QD_AIR_FALL_MAX)
    {
        fp->physics.vel_air.y -= 1.0F;
    }
}

/* ---- Quick Draw: dash, slash and end ---- */

static void ndsP4IkeQuickDrawSlashStart(GObj *fighter_gobj);

/* proc_hit / proc_shield while dashing: the dash cuts to the slash. */
static void ndsP4IkeQuickDrawContact(GObj *fighter_gobj)
{
    ndsP4IkeQuickDrawSlashStart(fighter_gobj);
}

static void ndsP4IkeQuickDrawDashStart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 charge = *ndsP4IkeVar(fp, 0);
    f32 speed = fp->attr->run_speed * IKE_QD_SPEED_MUL;
    s32 air = (fp->ga == nMPKineticsAir) ? IKE_QD_AIR : 0;

    ndsP4IkeSetStatus(fighter_gobj, IKE_STATUS_QD_DASH + air);
    *ndsP4IkeVar(fp, 0) = charge;
    *ndsP4IkeVar(fp, 1) = IKE_QD_DASH_MIN + (IKE_QD_DASH_RANGE * charge) / IKE_QD_CHARGE_MAX;
    if (air != 0)
    {
        fp->physics.vel_air.x = speed * (f32)fp->lr;
        fp->physics.vel_air.y = 0.0F;
    }
    else fp->physics.vel_ground.x = speed;

    fp->proc_hit = ndsP4IkeQuickDrawContact;
    fp->proc_shield = ndsP4IkeQuickDrawContact;
}

static void ndsP4IkeQuickDrawEndStart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 charge = *ndsP4IkeVar(fp, 0);

    ndsP4IkeSetStatus(fighter_gobj, IKE_STATUS_QD_END + ((fp->ga == nMPKineticsAir) ? IKE_QD_AIR : 0));
    *ndsP4IkeVar(fp, 0) = charge;
    fp->physics.vel_ground.x *= 0.25F;
    fp->physics.vel_air.x *= 0.25F;
}

/* Dash update: the dash runs its frames, then the miss's end. */
void ndsP4IkeQuickDrawDashMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *left = ndsP4IkeVar(fp, 1);

    if (--(*left) <= 0)
    {
        ndsP4IkeQuickDrawEndStart(fighter_gobj);
    }
}

/* Dash physics: the speed held, no friction on the ground, no gravity in
 * the air. */
void ndsP4IkeQuickDrawDashGroundPhysics(GObj *fighter_gobj)
{
    ftPhysicsSetGroundVelTransferAir(fighter_gobj);
}

void ndsP4IkeQuickDrawDashAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->physics.vel_air.y = 0.0F;
}

static void ndsP4IkeQuickDrawSlashStart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 charge = *ndsP4IkeVar(fp, 0);

    ndsP4IkeSetStatus(fighter_gobj, IKE_STATUS_QD_SLASH + ((fp->ga == nMPKineticsAir) ? IKE_QD_AIR : 0));
    *ndsP4IkeVar(fp, 0) = charge;
    fp->physics.vel_ground.x *= 0.25F;
    fp->physics.vel_air.x *= 0.25F;
    fp->physics.vel_air.y = 0.0F;
}

/* The end of a slash or a miss: wait on the ground, special fall in the
 * air (from the air or off an edge, card). */
static void ndsP4IkeQuickDrawFinish(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsAir)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, 1.0F, FALSE, TRUE, FALSE, 0.375F, FALSE);
    }
    else ftCommonWaitSetStatus(fighter_gobj);
}

/* Slash update: each hitbox the script opens gains the charge's bonus. */
void ndsP4IkeQuickDrawSlashMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 bonus = (IKE_QD_SLASH_BONUS * *ndsP4IkeVar(fp, 0)) / IKE_QD_CHARGE_MAX;
    s32 i;

    for (i = 0; i < 4; i++)
    {
        FTAttackColl *coll = &fp->attack_colls[i];

        if (coll->attack_state == nGMAttackStateNew)
        {
            coll->damage += bonus;
        }
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ndsP4IkeQuickDrawFinish(fighter_gobj);
    }
}

/* The miss's end lag. */
void ndsP4IkeQuickDrawEndMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ndsP4IkeQuickDrawFinish(fighter_gobj);
    }
}

/* ---- Aether ---- */

static void ndsP4IkeAetherStart(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4IkeSetStatus(fighter_gobj, status_id);
    fp->is_fastfall = FALSE;
    /* status_vars word 0: the leap's frames so far. */
    *ndsP4IkeVar(fp, 0) = 0;
}

/* ground_usp. */
void ndsP4IkeAetherGroundInitial(GObj *fighter_gobj)
{
    ndsP4IkeAetherStart(fighter_gobj, IKE_STATUS_AETHER_RISE);
}

/* air_usp: the fall stopped. */
void ndsP4IkeAetherAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4IkeAetherStart(fighter_gobj, IKE_STATUS_AETHER_RISE_AIR);
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.x *= 0.5F;
}

static void ndsP4IkeAetherFallStart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4IkeSetStatus(fighter_gobj, IKE_STATUS_AETHER_FALL);
    mpCommonSetFighterAir(fp);
    fp->physics.vel_air.x *= 0.25F;
    fp->physics.vel_air.y = -IKE_AETHER_FALL_VEL;
    fp->jumps_used = fp->attr->jumps_max;
}

static void ndsP4IkeAetherLandingStart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ndsP4IkeSetStatus(fighter_gobj, IKE_STATUS_AETHER_LANDING);
}

/* Rise update: armour while the script holds flag1; the clip's end starts
 * the plunge. */
void ndsP4IkeAetherRiseMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->knockback_resist_status = (fp->motion_vars.flags.flag1 != 0) ? IKE_AETHER_ARMOUR : 0.0F;
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        fp->knockback_resist_status = 0.0F;
        ndsP4IkeAetherFallStart(fighter_gobj);
    }
}

/* Rise physics: grounded until the script's jump (flag2 1), then the leap
 * (flag2 2: the table's speed a frame) and the hang at its top, a weak
 * drift throughout. */
void ndsP4IkeAetherRisePhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    s32 *leap = ndsP4IkeVar(fp, 0);
    f32 mul = (fp->status_id == IKE_STATUS_AETHER_RISE) ? 1.0F : IKE_AETHER_RISE_AIR_MUL;

    if (fp->motion_vars.flags.flag2 == 1)
    {
        mpCommonSetFighterAir(fp);
        fp->physics.vel_air.y = sNdsP4IkeAetherRise[0] * mul;
        fp->physics.vel_air.x = (f32)fp->lr * 8.0F;
        fp->motion_vars.flags.flag2 = 2;
        fp->jumps_used = attr->jumps_max;
        *leap = 1;
        return;
    }
    if (fp->ga == nMPKineticsGround)
    {
        ftPhysicsApplyGroundVelFriction(fighter_gobj);
        return;
    }
    if ((fp->motion_vars.flags.flag2 != 0) && (*leap < (s32)ARRAY_COUNT(sNdsP4IkeAetherRise)))
    {
        fp->physics.vel_air.y = sNdsP4IkeAetherRise[*leap] * mul;
        (*leap)++;
    }
    else
    {
        /* An air start hangs until the jump; the leap's top holds for the
         * spin. */
        fp->physics.vel_air.y = 0.0F;
    }

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsClampAirVelXStickRange(fp, FTPHYSICS_AIRDRIFT_CLAMP_RANGE_MIN,
                                        attr->air_accel * IKE_AETHER_DRIFT_MUL, attr->air_speed_max_x);
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

/* Rise and fall map (Final Cutter's): the ledge caught, the floor met
 * coming down lands. */
void ndsP4IkeAetherMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        if (mpCommonCheckFighterOnEdge(fighter_gobj) == FALSE)
        {
            fp->ga = nMPKineticsAir;
        }
    }
    else if (mpCommonCheckFighterCeilHeavyCliff(fighter_gobj) != FALSE)
    {
        if (fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK)
        {
            fp->knockback_resist_status = 0.0F;
            ftCommonCliffCatchSetStatus(fighter_gobj);
        }
        else if ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) && (fp->physics.vel_air.y < 0.0F))
        {
            fp->knockback_resist_status = 0.0F;
            ndsP4IkeAetherLandingStart(fighter_gobj);
        }
    }
}

/* Fall physics: the plunge's speed held, a little drift. */
void ndsP4IkeAetherFallPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    fp->physics.vel_air.y = -IKE_AETHER_FALL_VEL;
    ftPhysicsClampAirVelXStickRange(fp, FTPHYSICS_AIRDRIFT_CLAMP_RANGE_MIN,
                                    attr->air_accel * IKE_AETHER_DRIFT_MUL, attr->air_speed_max_x * 0.5F);
    ftPhysicsApplyAirVelXFriction(fp, attr);
}

/* ---- Counter (his taunt) ---- */

static s32 *ndsP4IkeCounterHit(FTStruct *fp)
{
    return ndsP4IkeVar(fp, 0);
}

/* Counter update (Marth's): armour while the script holds flag0; a counted
 * hit turns him to face it and strikes back. */
void ndsP4IkeCounterMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->knockback_resist_status = (fp->motion_vars.flags.flag0 != 0) ? IKE_COUNTER_ARMOUR : 0.0F;
    if (*ndsP4IkeCounterHit(fp) != 0)
    {
        fp->knockback_resist_status = 0.0F;
        fp->lr = fp->damage_lr;
        fp->joints[nFTPartsJointTopN]->rotate.vec.f.y = 1.5707964F * (f32)fp->lr;
        ndsP4IkeSetStatus(fighter_gobj, IKE_STATUS_COUNTER_ATTACK);
        return;
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        fp->knockback_resist_status = 0.0F;
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* ftParamUpdateDamage's head: a hit landing while the counter's window is
 * open deals nothing and is counted (Marth's detection patch). The taunt
 * keeps Appeal in status_id (status_replace). */
static s32 ndsP4IkeUpdateDamage(FTStruct *fp, s32 damage)
{
    if ((fp->status_id == nFTCommonStatusAppeal) && (fp->motion_vars.flags.flag0 != 0))
    {
        *ndsP4IkeCounterHit(fp) = 1;
        return 0;
    }
    return damage;
}

/* Every status change: the counter starts with no hit. */
void ndsP4IkeOnStatus(GObj *fighter_gobj, s32 status_id)
{
    if (status_id == IKE_STATUS_COUNTER)
    {
        *ndsP4IkeCounterHit(ftGetStruct(fighter_gobj)) = 0;
    }
}

/* Owner playtest p6i: the jab combo steps forward, the third strike most
 * (the 3DS reference's lunge). Over each jab's step (its animation frames
 * before the hit, Ultimate's timing since p6r) he moves `speed` units a tic
 * along his facing, before the tic's physics and map, so the floor and
 * walls take the step as any motion: about 25, 40 and 150 units. (A held
 * ground speed did not: the floor's friction, 6.4 a tic on Dream Land,
 * eats a jab-sized speed in a tic.) */
typedef struct NDSP4IkeJabStep
{
    s32 status_id;
    f32 from, to;
    f32 speed;
} NDSP4IkeJabStep;

static const NDSP4IkeJabStep sNdsP4IkeJabSteps[] = {
    { nFTCommonStatusAttack11, 0.0F, 3.0F, 8.5F },
    { nFTCommonStatusAttack12, 0.0F, 3.0F, 13.0F },
    { IKE_STATUS_ATTACK13, 1.0F, 5.0F, 37.5F },
};

static void ndsP4IkeBeforePhysicsMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 frame = fighter_gobj->anim_frame;
    s32 i;

    if (fp->ga != nMPKineticsGround)
    {
        return;
    }
    for (i = 0; i < ARRAY_COUNT(sNdsP4IkeJabSteps); i++)
    {
        const NDSP4IkeJabStep *step = &sNdsP4IkeJabSteps[i];

        if ((fp->status_id == step->status_id) && (frame >= step->from) && (frame < step->to))
        {
            DObjGetStruct(fighter_gobj)->translate.vec.f.x += step->speed * (f32)fp->lr;
        }
    }
}

/* The taunt is the counter (card). */
static const u16 sNdsP4IkeStatusReplace[] = {
    nFTCommonStatusAppeal, IKE_STATUS_COUNTER,
    0xFFFFu
};

const NDSP4Overrides gNdsP4IkeOverrides = {
    .entry_case = ndsP4RoyEntryCase,
    .update_damage = ndsP4IkeUpdateDamage,
    .status_replace = sNdsP4IkeStatusReplace,
    .before_physics_map = ndsP4IkeBeforePhysicsMap,
    /* Ultimate's motions; its hits collide in the stage's plane. */
    .planar_hits = 1u,
    /* They move Roy's locked shoulders, hips and toes (spec "anim_locks"). */
    .anim_locks_rest = 1u,
};

#endif /* NDS_P4_CODE_IKE */
