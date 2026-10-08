/*
 * P4 Sonic: native ports of the donor's special routines and of the patches
 * Remix makes on his character id. Source: JSsixtyfour/smashremix 5e04fe7,
 * src/Sonic/SonicSpecial.asm and Sonic.asm, read as assembled
 * (scripts/p4/mipsdis.py): the OS.copy_segment blocks are the original
 * game's code. Sonic runs Fox's status code (fp->fkind == nFTKindFox).
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x004 object                           fighter_gobj
 *   0x00C team                             team
 *   0x044 direction                        lr
 *   0x078 position pointer                 coll_data.p_translate
 *   0x0CC collision flags                  coll_data.mask_prev (last
 *                                          frame's walls)
 *   0x0EC floor line                       coll_data.floor_line_id
 *   0x0F4 floor flags                      coll_data.floor_flags
 *   0x148 jumps used                       jumps_used
 *   0x14C kinetic state                    ga
 *   0x17C/0x180/0x184 temp variables 1-3   motion_vars.flags.flag0/1/2
 *   0x18D & 0x07                           clears is_absorb, absorb_lr,
 *                                          is_goto_attack100 and is_fastfall
 *   0x1BC/0x1BE buttons held, tapped       input.pl.button_hold/button_tap
 *   0x1C3 stick y                          input.pl.stick_range.y
 *   0x294 + 0xC4 n                         attack_colls[n] (state, records)
 *   0x8F8/0x8FC parts 0x0 and 0x1          joints[4], joints[5]
 *   0xADC                                  passive_vars word 0: the spring
 *                                          is spent (until he lands, as
 *                                          Mario's landing case clears it,
 *                                          or is hit)
 *   0xB18-0xB28                            status_vars, words 0-4: the
 *                                          homing attack's target, its
 *                                          distance ahead, angle (word 2)
 *                                          and timer (word 4); the spin
 *                                          dash's charge (word 0); the
 *                                          spring's rise and grounded
 *                                          start (words 0-1)
 *   object + 0x74                          the top joint (DObjGetStruct)
 *
 * The spring is a projectile of his own file (spring_stage_setting_,
 * spring_main_, check_spring_bounce_): S6. Until it exists he takes a
 * spring that is already gone, which the donor reads as one destroyed
 * under him: he rises on the script's cue without it. Classic Sonic (the
 * character select's per-port toggle and its sounds) is not on the DS; the
 * homing attack's Super Sonic ranges are Super Sonic's, who is not a
 * content. Kirby's copy is S8's.
 */
#include <nds/nds_p4.h>

#if NDS_P4_SONIC

#include <ef/effect.h>
#include <it/item.h>
#include <macros.h>
#include <sc/scene.h>
#include <sys/obj.h>
#include <sys/objdef.h>

s32 ftCommonKneeBendGetInputTypeCommon(FTStruct *fp);
f32 ftKirbySpecialLwGetGroundAxisYaw(FTStruct *fp);
f32 syUtilsArcTan2(f32 y, f32 x);
f32 __sinf(f32);
f32 __cosf(f32);
f32 sqrtf(f32);

/* Sonic.Action (his action array; 0xF6 on are add_new_action's). */
#define SONIC_STATUS_USP 0xE4
#define SONIC_STATUS_DSPG_CHARGE 0xF6
#define SONIC_STATUS_DSPG_MOVE 0xF7
#define SONIC_STATUS_DSPG_END 0xF8
#define SONIC_STATUS_DSPA_CHARGE 0xF9
#define SONIC_STATUS_DSPA_MOVE 0xFA
#define SONIC_STATUS_DSPA_JUMP 0xFB
#define SONIC_STATUS_DSPA_END 0xFC
#define SONIC_STATUS_NSP_BEGIN 0xFD
#define SONIC_STATUS_NSP_MOVE 0xFE
#define SONIC_STATUS_NSP_LOCKED_MOVE 0xFF
#define SONIC_STATUS_NSPG_END 0x100
#define SONIC_STATUS_NSPA_END 0x101
#define SONIC_STATUS_NSPG_RECOIL 0x102
#define SONIC_STATUS_NSPA_RECOIL 0x103
#define SONIC_STATUS_NSP_BOUNCE 0x104

/* Joypad.A and Joypad.B in button_tap and button_hold. */
#define SONIC_BUTTON_A 0x8000u
#define SONIC_BUTTON_B 0x4000u

/* SonicNSP (the homing attack). */
#define SONIC_NSP_MAX_X_RANGE 2208.0F
#define SONIC_NSP_MIN_Y_RANGE 1000.0F
#define SONIC_NSP_SPEED 110.0F
#define SONIC_NSP_LOCKED_SPEED 130.0F
#define SONIC_NSP_RECOIL_X_SPEED -20.0F
#define SONIC_NSP_RECOIL_Y_SPEED 80.0F
#define SONIC_NSP_BOUNCE_Y_SPEED 60.0F
#define SONIC_NSP_BEGIN_Y_SPEED 12.0F
#define SONIC_NSP_TURN_SPEED 0x3E0EFA1Eu    /* 8 degrees */
#define SONIC_NSP_DEFAULT_ANGLE 0xBEBBA861u /* -21 degrees */
#define SONIC_NSP_MAX_ANGLE 0x3F1C61A6u     /* 35 degrees */
#define SONIC_NSP_MIN_ANGLE 0xBF1C61A6u
#define SONIC_NSP_FULL_TURN 0x40C90FE4u     /* "360 degrees" */
#define SONIC_NSP_HALF_TURN 0xC0490FD0u     /* "-180 degrees" */
#define SONIC_NSP_DURATION 12
#define SONIC_NSP_LOCKED_DURATION 20
/* check_for_targets_ skips a fighter in a KO status (action id < 7). */
#define SONIC_NSP_TARGET_STATUS_MIN 7
#define SONIC_PRESERVE_HIT 0x0001u          /* "continue hitbox" */

/* SonicUSP (the spring). */
#define SONIC_USP_Y_SPEED 129.0F
#define SONIC_USP_GROUND_LIFT 144.0F
#define SONIC_USP_SPRING_OFF_Y -228.0F
#define SONIC_USP_BODY_JOINT 4

/* SonicDSP (the spin dash). */
#define SONIC_DSP_MAX_CHARGE 24
#define SONIC_DSP_MAX_CHARGE_AIR 18
#define SONIC_DSP_CHARGE_TAP 3
#define SONIC_DSP_GROUND_CHARGE_MIN 6
#define SONIC_DSP_BASE_SPEED 46.0F
#define SONIC_DSP_MIN_SPEED 15.0F
#define SONIC_DSP_JUMP_SPEED 62.0F
#define SONIC_DSP_GRAVITY 2.25F
#define SONIC_DSP_SLOPE_ACCELERATION 3.5F
#define SONIC_DSP_MAX_FALL_SPEED 50.0F
#define SONIC_DSP_AIR_FRICTION 3.0F
#define SONIC_DSP_GROUND_TRACTION 0.25F
#define SONIC_DSP_AIR_CHARGE_Y_MUL 0.3125F  /* "0.31" */
#define SONIC_DSP_ANIM_SPEED_ADD 20.0F
#define SONIC_DSP_ANIM_SPEED_MUL 0x3C220000u /* "0.01" */
#define SONIC_DSP_STICK_DOWN -39
#define SONIC_DSP_FGM_CHARGE 0x3D8          /* SPINDASH_CHARGE */
#define SONIC_DSP_JOINT_0 4
#define SONIC_DSP_JOINT_1 5

static f32 ndsP4SonicBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

static s32 *ndsP4SonicStatusS32(FTStruct *fp, s32 word)
{
    return &((s32 *)(void *)&fp->status_vars)[word];
}

static f32 *ndsP4SonicStatusF32(FTStruct *fp, s32 word)
{
    return &((f32 *)(void *)&fp->status_vars)[word];
}

static s32 *ndsP4SonicSpringSpent(FTStruct *fp)
{
    return &((s32 *)(void *)&fp->passive_vars)[0];
}

static void ndsP4SonicClearFlags(FTStruct *fp)
{
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

static void ndsP4SonicClearFastFall(FTStruct *fp)
{
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
}

static void ndsP4SonicSetStatusEvents(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* The top joint's z rotation, along his movement. */
static void ndsP4SonicSetRotation(FTStruct *fp, f32 angle)
{
    DObjGetStruct(fp->fighter_gobj)->rotate.vec.f.z = (f32)fp->lr * angle;
}

/* ---- Neutral special (the homing attack) ---- */

/* SonicNSP.begin_initial_ (ground_nsp, air_nsp): airborne, a small hop,
 * no target. */
void ndsP4SonicNSPInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
    }
    ndsP4SonicSetStatusEvents(fighter_gobj, SONIC_STATUS_NSP_BEGIN);
    ndsP4SonicClearFlags(fp);
    *ndsP4SonicStatusS32(fp, 0) = 0;
    *ndsP4SonicStatusS32(fp, 1) = 0;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = SONIC_NSP_BEGIN_Y_SPEED;
    ndsP4SonicClearFastFall(fp);
}

/* SonicNSP.check_target_: a target ahead within 2208, nearer than the one
 * held, and within a cone 1000 high plus half the distance. */
static sb32 ndsP4SonicNSPCheckTarget(FTStruct *fp, DObj *target_dobj, f32 *x_diff_out)
{
    Vec3f *pos = fp->coll_data.p_translate;
    Vec3f *target_pos = &target_dobj->translate.vec.f;
    f32 x_diff = (target_pos->x - pos->x) * (f32)fp->lr;
    f32 y_diff;

    if (!(x_diff <= SONIC_NSP_MAX_X_RANGE) || !(0.0F <= x_diff))
    {
        return FALSE;
    }
    if ((*ndsP4SonicStatusS32(fp, 0) != 0) && !(x_diff <= *ndsP4SonicStatusF32(fp, 1)))
    {
        return FALSE;
    }
    y_diff = ABSF(target_pos->y - pos->y);

    if (!(y_diff <= (SONIC_NSP_MIN_Y_RANGE + (0.5F * x_diff))))
    {
        return FALSE;
    }
    *x_diff_out = x_diff;

    return TRUE;
}

/* SonicNSP.check_for_targets_: the nearest fighter ahead (teammates spared
 * with team attack off, KO statuses skipped), else the nearest item with
 * its hurtbox on (the hit status's low bit). */
static void ndsP4SonicNSPCheckForTargets(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *other_gobj;
    f32 x_diff;

    for (other_gobj = gGCCommonLinks[nGCCommonLinkIDFighter]; other_gobj != NULL;
         other_gobj = other_gobj->link_next)
    {
        FTStruct *other_fp;

        if (other_gobj == fighter_gobj)
        {
            continue;
        }
        other_fp = ftGetStruct(other_gobj);

        if ((gSCManagerBattleState->is_team_battle != FALSE) &&
            (gSCManagerBattleState->is_team_attack == FALSE) && (other_fp->team == fp->team))
        {
            continue;
        }
        if ((u32)other_fp->status_id < SONIC_NSP_TARGET_STATUS_MIN)
        {
            continue;
        }
        if (ndsP4SonicNSPCheckTarget(fp, DObjGetStruct(other_gobj), &x_diff) != FALSE)
        {
            *ndsP4SonicStatusS32(fp, 0) = (s32)(uintptr_t)other_gobj;
            *ndsP4SonicStatusF32(fp, 1) = x_diff;
        }
    }
    if (*ndsP4SonicStatusS32(fp, 0) != 0)
    {
        return;
    }
    for (other_gobj = gGCCommonLinks[nGCCommonLinkIDItem]; other_gobj != NULL;
         other_gobj = other_gobj->link_next)
    {
        if (!(itGetStruct(other_gobj)->damage_coll.hitstatus & 1))
        {
            continue;
        }
        if (ndsP4SonicNSPCheckTarget(fp, DObjGetStruct(other_gobj), &x_diff) != FALSE)
        {
            *ndsP4SonicStatusS32(fp, 0) = (s32)(uintptr_t)other_gobj;
            *ndsP4SonicStatusF32(fp, 1) = x_diff;
        }
    }
}

static GObj *ndsP4SonicNSPTarget(FTStruct *fp)
{
    return (GObj *)(uintptr_t)*ndsP4SonicStatusS32(fp, 0);
}

/* SonicNSP.locked_move_initial_: aimed at the target, within 35 degrees. */
static void ndsP4SonicNSPLockedMoveInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f *pos;
    Vec3f *target_pos;
    f32 angle;

    ndsP4SonicSetStatusEvents(fighter_gobj, SONIC_STATUS_NSP_LOCKED_MOVE);

    pos = fp->coll_data.p_translate;
    target_pos = &DObjGetStruct(ndsP4SonicNSPTarget(fp))->translate.vec.f;
    angle = syUtilsArcTan2(target_pos->y - pos->y, (target_pos->x - pos->x) * (f32)fp->lr);

    if (!(ndsP4SonicBitsToF32(SONIC_NSP_MIN_ANGLE) < angle))
    {
        angle = ndsP4SonicBitsToF32(SONIC_NSP_MIN_ANGLE);
    }
    else if (!(angle < ndsP4SonicBitsToF32(SONIC_NSP_MAX_ANGLE)))
    {
        angle = ndsP4SonicBitsToF32(SONIC_NSP_MAX_ANGLE);
    }
    *ndsP4SonicStatusF32(fp, 2) = angle;
    ndsP4SonicSetRotation(fp, angle);
    *ndsP4SonicStatusS32(fp, 4) = SONIC_NSP_LOCKED_DURATION;
}

/* SonicNSP.move_initial_: no target, 21 degrees down. */
static void ndsP4SonicNSPMoveInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 angle = ndsP4SonicBitsToF32(SONIC_NSP_DEFAULT_ANGLE);

    ndsP4SonicSetStatusEvents(fighter_gobj, SONIC_STATUS_NSP_MOVE);
    *ndsP4SonicStatusF32(fp, 2) = angle;
    ndsP4SonicSetRotation(fp, angle);
    *ndsP4SonicStatusS32(fp, 4) = SONIC_NSP_DURATION;
}

/* SonicNSP.begin_main_ (0xFD update): the script's temp variable 1 lets
 * him go (once B is let go) or sends him. */
void ndsP4SonicNSPBeginMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 cue = fp->motion_vars.flags.flag0;

    if (cue == 0)
    {
        return;
    }
    if ((cue == 1) && (fp->input.pl.button_hold & SONIC_BUTTON_B))
    {
        return;
    }
    ndsP4SonicNSPCheckForTargets(fighter_gobj);

    if (ndsP4SonicNSPTarget(fp) != NULL)
    {
        ndsP4SonicNSPLockedMoveInitial(fighter_gobj);
    }
    else ndsP4SonicNSPMoveInitial(fighter_gobj);
}

/* Character.get_hitbox_collision_flags_ & 0xF0: an active hitbox of his
 * has met a hurtbox, shield, reflector or absorber. */
static sb32 ndsP4SonicNSPHasHit(FTStruct *fp)
{
    s32 i;
    s32 j;

    for (i = 0; i < ARRAY_COUNT(fp->attack_colls); i++)
    {
        FTAttackColl *attack_coll = &fp->attack_colls[i];

        if (attack_coll->attack_state == nGMAttackStateOff)
        {
            continue;
        }
        for (j = 0; j < ARRAY_COUNT(attack_coll->attack_records); j++)
        {
            GMHitFlags *flags = &attack_coll->attack_records[j].victim_flags;

            if (flags->is_interact_hurt || flags->is_interact_shield ||
                flags->is_interact_reflect || flags->is_interact_absorb)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* SonicNSP.air_recoil_initial_. */
static void ndsP4SonicNSPAirRecoilInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4SonicSetStatusEvents(fighter_gobj, SONIC_STATUS_NSPA_RECOIL);
    fp->physics.vel_air.x = SONIC_NSP_RECOIL_X_SPEED * (f32)fp->lr;
    fp->physics.vel_air.y = SONIC_NSP_RECOIL_Y_SPEED;
}

/* SonicNSP.ground_end_initial_. */
static void ndsP4SonicNSPGroundEndInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4SonicSetStatusEvents(fighter_gobj, SONIC_STATUS_NSPG_END);
    fp->physics.vel_ground.x *= 0.5F;
}

/* SonicNSP.air_end_initial_. */
static void ndsP4SonicNSPAirEndInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4SonicSetStatusEvents(fighter_gobj, SONIC_STATUS_NSPA_END);
    fp->physics.vel_air.x *= 0.5F;
    fp->physics.vel_air.y *= 0.5F;
}

/* SonicNSP.move_main_ (0xFE/0xFF update): a hit recoils him; the timer
 * ends the move. */
void ndsP4SonicNSPMoveMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *timer;

    if (ndsP4SonicNSPHasHit(fp) != FALSE)
    {
        ndsP4SonicNSPAirRecoilInitial(fighter_gobj);
        return;
    }
    timer = ndsP4SonicStatusS32(fp, 4);
    *timer -= 1;

    if (*timer != 0)
    {
        return;
    }
    if (fp->ga == nMPKineticsGround)
    {
        ndsP4SonicNSPGroundEndInitial(fighter_gobj);
    }
    else ndsP4SonicNSPAirEndInitial(fighter_gobj);
}

/* SonicNSP.move_physics_ (0xFE/0xFF physics): with a target he turns up to
 * 8 degrees a frame towards it (the wrap adds a turn only below -180). */
void ndsP4SonicNSPMovePhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *target_gobj = ndsP4SonicNSPTarget(fp);
    f32 speed = SONIC_NSP_SPEED;
    f32 angle;

    if (target_gobj != NULL)
    {
        Vec3f *pos = fp->coll_data.p_translate;
        Vec3f *target_pos = &DObjGetStruct(target_gobj)->translate.vec.f;
        f32 full_turn = ndsP4SonicBitsToF32(SONIC_NSP_FULL_TURN);
        f32 half_turn = ndsP4SonicBitsToF32(SONIC_NSP_HALF_TURN);
        f32 turn = ndsP4SonicBitsToF32(SONIC_NSP_TURN_SPEED);
        f32 target_angle;
        f32 diff;

        speed = SONIC_NSP_LOCKED_SPEED;
        target_angle = syUtilsArcTan2(target_pos->y - pos->y, (target_pos->x - pos->x) * (f32)fp->lr);
        angle = *ndsP4SonicStatusF32(fp, 2);
        diff = target_angle - angle;

        if (!(half_turn < diff))
        {
            diff += full_turn;
        }
        if (!(turn < ABSF(diff)))
        {
            angle = target_angle;
        }
        else
        {
            if (!(0.0F < diff))
            {
                turn = -turn;
            }
            angle += turn;
        }
        if (!(half_turn < angle))
        {
            angle += full_turn;
        }
        *ndsP4SonicStatusF32(fp, 2) = angle;
        ndsP4SonicSetRotation(fp, angle);
    }
    angle = *ndsP4SonicStatusF32(fp, 2);
    fp->physics.vel_air.x = (speed * __cosf(angle)) * (f32)fp->lr;
    fp->physics.vel_air.y = speed * __sinf(angle);
}

/* SonicNSP.bounce_initial_: off the floor he lands on. */
static void ndsP4SonicNSPBounceInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4SonicSetStatusEvents(fighter_gobj, SONIC_STATUS_NSP_BOUNCE);
    fp->physics.vel_air.x *= 0.25F;
    fp->physics.vel_air.y = SONIC_NSP_BOUNCE_Y_SPEED;
}

/* SonicNSP.move_collision_ (0xFE/0xFF map). */
void ndsP4SonicNSPMoveMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SonicNSPBounceInitial);
}

/* The ends' and recoils' transitions keep the hitbox. */
static void ndsP4SonicNSPAirToGround(GObj *fighter_gobj, s32 status_id)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, status_id, fighter_gobj->anim_frame, 1.0F, SONIC_PRESERVE_HIT);
}

static void ndsP4SonicNSPGroundToAir(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, status_id, fighter_gobj->anim_frame, 1.0F, SONIC_PRESERVE_HIT);
    ftPhysicsClampAirVelXMax(fp);
}

/* SonicNSP.ground_end_transition_ / air_end_transition_ /
 * ground_recoil_transition_ / air_recoil_transition_. */
static void ndsP4SonicNSPEndAirToGround(GObj *fighter_gobj)
{
    ndsP4SonicNSPAirToGround(fighter_gobj, SONIC_STATUS_NSPG_END);
}

static void ndsP4SonicNSPEndGroundToAir(GObj *fighter_gobj)
{
    ndsP4SonicNSPGroundToAir(fighter_gobj, SONIC_STATUS_NSPA_END);
}

static void ndsP4SonicNSPRecoilAirToGround(GObj *fighter_gobj)
{
    ndsP4SonicNSPAirToGround(fighter_gobj, SONIC_STATUS_NSPG_RECOIL);
}

static void ndsP4SonicNSPRecoilGroundToAir(GObj *fighter_gobj)
{
    ndsP4SonicNSPGroundToAir(fighter_gobj, SONIC_STATUS_NSPA_RECOIL);
}

/* SonicNSP.ground_end_collision_ (0x100 map). */
void ndsP4SonicNSPEndGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4SonicNSPEndGroundToAir);
}

/* SonicNSP.air_end_collision_ (0x101 map). */
void ndsP4SonicNSPEndAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SonicNSPEndAirToGround);
}

/* SonicNSP.ground_recoil_collision_ (0x102 map). */
void ndsP4SonicNSPRecoilGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4SonicNSPRecoilGroundToAir);
}

/* SonicNSP.air_recoil_collision_ (0x103 map). */
void ndsP4SonicNSPRecoilAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SonicNSPRecoilAirToGround);
}

/* SonicNSP.destroyed_target_fix_, at gcEjectGObj's head: a fighter or item
 * object going away stops being any Sonic's target. */
void ndsP4SonicOnEjectGObj(GObj *gobj)
{
    GObj *fighter_gobj;

    if ((gobj == NULL) || ((gobj->id != nGCCommonKindFighter) && (gobj->id != nGCCommonKindItem)))
    {
        return;
    }
    for (fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter]; fighter_gobj != NULL;
         fighter_gobj = fighter_gobj->link_next)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((ndsP4Content(fp) == NDS_P4_ID_SONIC) && (ndsP4SonicNSPTarget(fp) == gobj))
        {
            *ndsP4SonicStatusS32(fp, 0) = 0;
        }
    }
}

/* ---- Up special (the spring) ---- */

/* The spring before S6: an object with no joint, which main_air_ reads as a
 * spring already destroyed. */
static GObj sNdsP4SonicNoSpring;

/* SonicUSP.spring_stage_setting_ (S6): the spring under him; temp
 * variable 3 holds it. */
static void ndsP4SonicUSPSpringMake(GObj *fighter_gobj, Vec3f *pos)
{
    (void)pos;
    ftGetStruct(fighter_gobj)->motion_vars.flags.flag2 = (s32)(uintptr_t)&sNdsP4SonicNoSpring;
}

/* SonicUSP.air_initial_ (air_usp). */
void ndsP4SonicUSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4SonicSetStatusEvents(fighter_gobj, SONIC_STATUS_USP);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 1;
    *ndsP4SonicStatusS32(fp, 0) = FALSE;
    *ndsP4SonicStatusS32(fp, 1) = FALSE;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 0.0F;
    ndsP4SonicClearFastFall(fp);
}

/* SonicUSP.ground_initial_ (ground_usp): airborne and up onto the spring;
 * temp variable 3's low bytes (big-endian 0x186/0x187 in the donor) keep
 * the floor line's low byte and whether the spring starts in the air (not
 * over a line). */
void ndsP4SonicUSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 floor_line_id;
    u32 spring_air;

    mpCommonSetFighterAir(fp);
    DObjGetStruct(fighter_gobj)->translate.vec.f.y += SONIC_USP_GROUND_LIFT;
    ndsP4SonicUSPAirInitial(fighter_gobj);
    *ndsP4SonicStatusS32(fp, 1) = TRUE;

    floor_line_id = fp->coll_data.floor_line_id;
    spring_air = (floor_line_id < 0) ? 1 : 0;
    fp->motion_vars.flags.flag2 = (s32)(((u32)fp->motion_vars.flags.flag2 & 0xFFFF0000u) |
                                        (((u32)floor_line_id & 0xFFu) << 8) | spring_air);
}

/* SonicUSP.main_air_ (0xE4 update): the spring first; on the script's
 * temp variable 1 it uncoils (S6) and he rises at 129, the spring spent
 * and, from the air, his jumps too. */
void ndsP4SonicUSPMainAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (((u32)fp->motion_vars.flags.flag2 >> 16) == 0)
    {
        Vec3f pos;

        pos.x = pos.y = pos.z = 0.0F;
        gmCollisionGetFighterPartsWorldPosition(fp->joints[SONIC_USP_BODY_JOINT], &pos);
        pos.z = 0.0F;
        pos.y += SONIC_USP_SPRING_OFF_Y;
        ndsP4SonicUSPSpringMake(fighter_gobj, &pos);
    }
    else if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->physics.vel_air.y = SONIC_USP_Y_SPEED;
        fp->motion_vars.flags.flag0 = 0;
        *ndsP4SonicStatusS32(fp, 0) = TRUE;
        *ndsP4SonicSpringSpent(fp) = TRUE;

        if (*ndsP4SonicStatusS32(fp, 1) == FALSE)
        {
            fp->jumps_used = fp->attr->jumps_max;
        }
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* SonicUSP.interrupt_ (0xE4 interrupt): once the script sets temp
 * variable 2, aerials or a mid-air jump. */
void ndsP4SonicUSPInterrupt(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->motion_vars.flags.flag1 == 0)
    {
        return;
    }
    if (ftCommonAttackAirCheckInterruptCommon(fighter_gobj) == FALSE)
    {
        ftCommonJumpAerialCheckInterruptCommon(fighter_gobj);
    }
}

/* SonicUSP.air_physics_ (0xE4 physics): still until he rises, then drift. */
void ndsP4SonicUSPAirPhysics(GObj *fighter_gobj)
{
    if (*ndsP4SonicStatusS32(ftGetStruct(fighter_gobj), 0) != FALSE)
    {
        ftPhysicsApplyAirVelDrift(fighter_gobj);
    }
}

/* ---- Down special (the spin dash) ---- */

/* SonicDSP.ground_charge_initial_ (ground_dsp). */
void ndsP4SonicDSPGroundChargeInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4SonicSetStatusEvents(fighter_gobj, SONIC_STATUS_DSPG_CHARGE);
    ndsP4SonicClearFlags(fp);
    fp->physics.vel_ground.x *= 0.5F;
    *ndsP4SonicStatusS32(fp, 0) = 0;
}

/* SonicDSP.air_charge_initial_ (air_dsp). */
void ndsP4SonicDSPAirChargeInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4SonicSetStatusEvents(fighter_gobj, SONIC_STATUS_DSPA_CHARGE);
    ndsP4SonicClearFlags(fp);
    fp->physics.vel_air.y *= SONIC_DSP_AIR_CHARGE_Y_MUL;
    ndsP4SonicClearFastFall(fp);
    *ndsP4SonicStatusS32(fp, 0) = 0;
}

/* SonicDSP.ground_move_initial_: 46 plus 4 a charge level. */
static void ndsP4SonicDSPGroundMoveInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 speed;

    ftMainSetStatus(fighter_gobj, SONIC_STATUS_DSPG_MOVE, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    speed = SONIC_DSP_BASE_SPEED + (f32)(*ndsP4SonicStatusS32(fp, 0) * 4);
    fp->physics.vel_ground.x = speed;
    fp->physics.vel_air.x = speed * (f32)fp->lr;
}

/* SonicDSP.ground_end_initial_ / air_end_initial_. */
static void ndsP4SonicDSPGroundEndInitial(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, SONIC_STATUS_DSPG_END, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

static void ndsP4SonicDSPAirEndInitial(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, SONIC_STATUS_DSPA_END, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

/* The charges' common part: an A or B tap adds 3 and replays the charge
 * sound (Sonic.asm spin_dash_fgm_patch_ stops the one playing), the
 * script's temp variable 2 adds its value, up to the maximum. TRUE when
 * the script's temp variable 1 lets the dash go: once the stick is not held
 * down and B is let go, or at once. */
static sb32 ndsP4SonicDSPCharge(FTStruct *fp, s32 max_charge)
{
    s32 *charge = ndsP4SonicStatusS32(fp, 0);
    s32 cue;

    if (fp->input.pl.button_tap & (SONIC_BUTTON_B | SONIC_BUTTON_A))
    {
        *charge += SONIC_DSP_CHARGE_TAP;
        ftParamStopLoopSFX(fp);
        ftParamPlayLoopSFX(fp, SONIC_DSP_FGM_CHARGE);
    }
    *charge += fp->motion_vars.flags.flag1;
    fp->motion_vars.flags.flag1 = 0;

    if (max_charge < *charge)
    {
        *charge = max_charge;
    }
    cue = fp->motion_vars.flags.flag0;

    if (cue == 0)
    {
        return FALSE;
    }
    if (cue == 1)
    {
        if ((fp->input.pl.stick_range.y < SONIC_DSP_STICK_DOWN) ||
            (fp->input.pl.button_hold & SONIC_BUTTON_B))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* SonicDSP.ground_charge_main_ (0xF6 update). */
void ndsP4SonicDSPGroundChargeMain(GObj *fighter_gobj)
{
    if (ndsP4SonicDSPCharge(ftGetStruct(fighter_gobj), SONIC_DSP_MAX_CHARGE) != FALSE)
    {
        ndsP4SonicDSPGroundMoveInitial(fighter_gobj);
    }
}

/* SonicDSP.air_charge_main_ (0xF9 update): the aerial charge only cancels. */
void ndsP4SonicDSPAirChargeMain(GObj *fighter_gobj)
{
    if (ndsP4SonicDSPCharge(ftGetStruct(fighter_gobj), SONIC_DSP_MAX_CHARGE_AIR) != FALSE)
    {
        ndsP4SonicDSPAirEndInitial(fighter_gobj);
    }
}

/* SonicDSP.ground_charge_transition_: landing with 6 or more dashes at
 * once. */
static void ndsP4SonicDSPChargeAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);

    if ((u32)*ndsP4SonicStatusS32(fp, 0) >= SONIC_DSP_GROUND_CHARGE_MIN)
    {
        ndsP4SonicDSPGroundMoveInitial(fighter_gobj);
    }
    else ftMainSetStatus(fighter_gobj, SONIC_STATUS_DSPG_CHARGE, fighter_gobj->anim_frame, 1.0F,
                         FTSTATUS_PRESERVE_NONE);
}

/* SonicDSP.air_charge_transition_. */
static void ndsP4SonicDSPChargeGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, SONIC_STATUS_DSPA_CHARGE, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
    ftPhysicsClampAirVelXMax(fp);
}

/* SonicDSP.ground_charge_collision_ (0xF6 map). */
void ndsP4SonicDSPChargeGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4SonicDSPChargeGroundToAir);
}

/* SonicDSP.air_charge_collision_ (0xF9 map). */
void ndsP4SonicDSPChargeAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SonicDSPChargeAirToGround);
}

/* The dash's animation speed, (speed + 20) * "0.01", on parts 0x0 and 0x1;
 * the top joint's speed of 1 is nudged so the next status resets it. */
static void ndsP4SonicDSPSetAnimSpeed(GObj *fighter_gobj, f32 speed)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *topn = DObjGetStruct(fighter_gobj);
    f32 anim_speed = ndsP4SonicBitsToF32(SONIC_DSP_ANIM_SPEED_MUL) * (speed + SONIC_DSP_ANIM_SPEED_ADD);

    if (topn->anim_speed == 1.0F)
    {
        topn->anim_speed = ndsP4SonicBitsToF32(0x3F800001u);
    }
    fp->joints[SONIC_DSP_JOINT_0]->anim_speed = anim_speed;
    fp->joints[SONIC_DSP_JOINT_1]->anim_speed = anim_speed;
}

/* SonicDSP.air_jump_initial_. */
static void ndsP4SonicDSPAirJumpInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, SONIC_STATUS_DSPA_JUMP, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    fp->physics.vel_air.y = SONIC_DSP_JUMP_SPEED;
    efManagerDustHeavyDoubleMakeEffect(fp->coll_data.p_translate, 1, 1.0F);
}

/* SonicDSP.ground_move_main_ (0xF7 update): slopes speed him up or slow
 * him (his top joint follows the floor's angle); under 15 he stops; a jump
 * input jumps out. */
void ndsP4SonicDSPGroundMoveMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 slope = __sinf(ftKirbySpecialLwGetGroundAxisYaw(fp));

    fp->physics.vel_ground.x += (SONIC_DSP_SLOPE_ACCELERATION * slope) * -(f32)fp->lr;
    ndsP4SonicDSPSetAnimSpeed(fighter_gobj, fp->physics.vel_ground.x);

    if (fp->physics.vel_ground.x <= SONIC_DSP_MIN_SPEED)
    {
        ndsP4SonicDSPGroundEndInitial(fighter_gobj);
    }
    else if (ftCommonKneeBendGetInputTypeCommon(fp) != 0)
    {
        ndsP4SonicDSPAirJumpInitial(fighter_gobj);
    }
}

/* SonicDSP.air_move_main_ (0xFA/0xFB update). */
void ndsP4SonicDSPAirMoveMain(GObj *fighter_gobj)
{
    FTStruct *fp;
    f32 speed;

    ftAnimEndSetFall(fighter_gobj);

    fp = ftGetStruct(fighter_gobj);
    speed = sqrtf((fp->physics.vel_air.x * fp->physics.vel_air.x) +
                  (fp->physics.vel_air.y * fp->physics.vel_air.y));
    ndsP4SonicDSPSetAnimSpeed(fighter_gobj, speed);
}

/* SonicDSP.air_move_interrupt_ (0xFA/0xFB interrupt): once the script sets
 * temp variable 3 he is actionable. */
void ndsP4SonicDSPAirMoveInterrupt(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->motion_vars.flags.flag2 != 0)
    {
        ftCommonJumpProcInterrupt(fighter_gobj);
    }
}

/* SonicDSP.ground_move_physics_ (0xF7 physics): the ground friction with a
 * traction of 0.25 in place of his. */
void ndsP4SonicDSPGroundMovePhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsSetGroundVelFriction(fp, dMPCollisionMaterialFrictions[fp->coll_data.floor_flags &
                                                                    MAP_VERTEX_MAT_MASK] *
                                          SONIC_DSP_GROUND_TRACTION);
    ftPhysicsSetGroundVelTransferAir(fighter_gobj);
}

/* SonicDSP.air_move_physics_: his own fall (2.25, at most 50) and an air
 * friction of 3 (the donor passes a stack slot as the attributes). */
static void ndsP4SonicDSPAirMovePhysicsLocked(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsApplyGravityClampTVel(fp, SONIC_DSP_GRAVITY, SONIC_DSP_MAX_FALL_SPEED);

    if (fp->physics.vel_air.x < 0.0F)
    {
        fp->physics.vel_air.x += SONIC_DSP_AIR_FRICTION;

        if (fp->physics.vel_air.x >= 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
    else
    {
        fp->physics.vel_air.x -= SONIC_DSP_AIR_FRICTION;

        if (fp->physics.vel_air.x <= 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
}

/* SonicDSP.air_movement_physics_ (0xFA/0xFB physics). */
void ndsP4SonicDSPAirMovementPhysics(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->motion_vars.flags.flag2 != 0)
    {
        ftPhysicsApplyAirVelDrift(fighter_gobj);
    }
    else ndsP4SonicDSPAirMovePhysicsLocked(fighter_gobj);
}

/* SonicDSP.air_move_initial_: off an edge. */
static void ndsP4SonicDSPAirMoveInitial(GObj *fighter_gobj)
{
    mpCommonSetFighterAir(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, SONIC_STATUS_DSPA_MOVE, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

/* SonicDSP.ground_move_collision_ (0xF7 map): a wall ahead ends the dash. */
void ndsP4SonicDSPMoveGroundMap(GObj *fighter_gobj)
{
    FTStruct *fp;
    u16 wall;

    if (mpCommonProcFighterOnFloor(fighter_gobj, ndsP4SonicDSPAirMoveInitial) == FALSE)
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    wall = fp->coll_data.mask_prev & ((fp->lr >= 0) ? MAP_FLAG_LWALL : MAP_FLAG_RWALL);

    if (wall != 0)
    {
        ndsP4SonicDSPGroundEndInitial(fighter_gobj);
    }
}

/* SonicDSP.ground_move_transition_. */
static void ndsP4SonicDSPMoveAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, SONIC_STATUS_DSPG_MOVE, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
}

/* SonicDSP.air_move_collision_ (0xFA/0xFB map): still rolling he lands
 * into the ground dash; actionable, as any aerial. */
void ndsP4SonicDSPMoveAirMap(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->motion_vars.flags.flag2 != 0)
    {
        mpCommonProcFighterCliffWaitOrLanding(fighter_gobj);
    }
    else mpCommonProcFighterLanding(fighter_gobj, ndsP4SonicDSPMoveAirToGround);
}

/* SonicDSP.ground_end_transition_ / air_end_transition_. */
static void ndsP4SonicDSPEndAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, SONIC_STATUS_DSPG_END, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
}

static void ndsP4SonicDSPEndGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, SONIC_STATUS_DSPA_END, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
    ftPhysicsClampAirVelXMax(fp);
}

/* SonicDSP.ground_end_collision_ (0xF8 map). */
void ndsP4SonicDSPEndGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4SonicDSPEndGroundToAir);
}

/* SonicDSP.air_end_collision_ (0xFC map). */
void ndsP4SonicDSPEndAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SonicDSPEndAirToGround);
}

/* SonicUSPRefresh (Character.on_hit, OnHit.asm at the damage routine's
 * head): a hit gives the spring back. */
static void ndsP4SonicOnDamage(FTStruct *fp)
{
    *ndsP4SonicSpringSpent(fp) = FALSE;
}

/* SonicUSP.action_check_patch_ (ftCommonSpecialAirCheckInterruptCommon's
 * head): no aerial specials while the spring is spent. */
static sb32 ndsP4SonicAirSpecialBlock(FTStruct *fp)
{
    return (*ndsP4SonicSpringSpent(fp) != FALSE) ? TRUE : FALSE;
}

const NDSP4Overrides gNdsP4SonicOverrides = {
    .on_damage = ndsP4SonicOnDamage,
    .air_special_block = ndsP4SonicAirSpecialBlock,
};

#endif /* NDS_P4_SONIC */
