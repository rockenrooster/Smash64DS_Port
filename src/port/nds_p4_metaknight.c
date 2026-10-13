/*
 * P4 Meta Knight (EXTRA): native ports of the donor's special routines and of
 * the patches EXTRA makes on his character id. Source:
 * joaorb64/smashremix-plus-extra 96621af, extra_characters/MetaKnight/
 * MetaKnightSpecial.asm and main.asm, read as assembled. Meta Knight runs
 * Jigglypuff's status code (fp->fkind == nFTKindPurin); every special, his
 * jab and his forward tilt are statuses of his own.
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x000D port                            player
 *   0x001C current frame (int)             status_total_tics
 *   0x0024 action                          status_id (the replaced action's
 *                                          own id: NDSP4Overrides.status_replace)
 *   0x0044 / 0x0048 / 0x004C / 0x0060      lr / vel_air.x / .y / vel_ground.x
 *   0x0078 position (a pointer)            coll_data.p_translate
 *   0x00CC / 0x00CE collision flags        coll_data.mask_prev / .mask_curr
 *   0x00EC / 0x0144                        coll_data.floor_line_id /
 *                                          .ignore_line_id
 *   0x0148 / 0x014C                        jumps_used / ga
 *   0x017C / 0x0184 temp variables 1 and 3 motion_vars.flags.flag0 / flag2
 *                                          (flag0 a float in his specials)
 *   0x018D | 0x01 / 0x018F | 0x08          is_invisible / is_jostle_ignore
 *   0x01BC / 0x01BE / 0x01C2 / 0x01C3      input.pl.button_hold /
 *                                          button_tap / stick_range.x / .y
 *   0x05BB hurtbox state (the low byte)    hitstatus
 *   0x08AC current moveset script          motion_scripts[1][0].p_script
 *   0x08EC                                 joints[nFTPartsJointTransN]
 *   0x09C8 (+0x64)                         attr (jumps_max)
 *   0x09F4 / 0x09F8                        proc_shield / proc_hit
 *   0x0AE4 wing state                      sNdsP4MetaknightWings (below)
 *   0x0B30 temp variable 2                 status_vars word 6
 *   root DObj + 0x78                       anim_speed
 */
#include <nds/nds_p4.h>

#if NDS_P4_METAKNIGHT

#include <ef/effect.h>
#include <sys/audio.h>
#if NDS_P2_KIRBY
#include "kirby/nds_p4_kirby.generated.h"
#endif

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

f32 syUtilsArcTan2(f32 y, f32 x);
f32 __sinf(f32);
f32 __cosf(f32);
f32 sqrtf(f32);
LBParticle *efManagerDustHeavyDoubleMakeEffect(Vec3f *pos, s32 lr, f32 f_index);

/* MetaKnight.Action (main.asm) and his added actions, as exported. */
#define MK_STATUS_USP 0xDC
#define MK_STATUS_USP_AIR_START 0xDD
#define MK_STATUS_USP_AIR 0xDE
#define MK_STATUS_TORNADO_START 0xE6
#define MK_STATUS_TORNADO_LOOP 0xE7
#define MK_STATUS_TORNADO_END 0xE8
#define MK_STATUS_TORNADO_END_AIR 0xEA
#define MK_STATUS_JAB_START 0xEC
#define MK_STATUS_JAB_LOOP 0xED
#define MK_STATUS_JAB_END 0xEE
#define MK_STATUS_TILTF 0xEF
#define MK_STATUS_TILTF2 0xF0
#define MK_STATUS_TILTF3 0xF1
#define MK_STATUS_CAPE_START 0xF2
#define MK_STATUS_CAPE_END 0xF3
#define MK_STATUS_CAPE_F 0xF4
#define MK_STATUS_CAPE_B 0xF5
#define MK_STATUS_CAPE_N 0xF6
#define MK_STATUS_CAPE_AIR_START 0xF7
#define MK_STATUS_CAPE_AIR_END 0xF8
#define MK_STATUS_CAPE_AIR_F 0xF9
#define MK_STATUS_CAPE_AIR_B 0xFA
#define MK_STATUS_CAPE_AIR_N 0xFB

#define MK_BUTTON_A 0x8000u         /* Joypad.A */
#define MK_BUTTON_B 0x4000u         /* Joypad.B */
#define MK_WING_JOINT 0x1E          /* first of the four wing bones */
#define MK_WINGS_HIDDEN (-1)
#define MK_PI32 3.14159274F         /* 0x8018C688 */
#define MK_FGM_TORNADO 0x104        /* "FGM id 104 (weak)" */
#define MK_HITSTATUS_INTANGIBLE 3   /* the byte at 0x05BB */
#define MK_ENV_CLOAK 0xFFFFFF00u    /* CharEnvColor: full transparency */
#define MK_CAPE_SPEED 60.0F         /* DSP.Start.SPEED (0x4270) */

static f32 *ndsP4MetaknightTemp1F(FTStruct *fp)
{
    return (f32 *)(void *)&fp->motion_vars.flags.flag0;
}

static s32 *ndsP4MetaknightTemp2(FTStruct *fp)
{
    return &((s32 *)(void *)&fp->status_vars)[6];
}

/* ---- Wings (OnActionChanged, initial_script_) ---- */

/* The donor's player-struct word 0x0AE4: the first wing bone's display list
 * when the last other action began, or -1 (hidden). */
static s32 sNdsP4MetaknightWings[GMCOMMON_PLAYERS_MAX];

/* MetaKnight.initial_script_ (spawn and rebirth): the wing state is hidden. */
static void ndsP4MetaknightOnInit(FTStruct *fp)
{
    sNdsP4MetaknightWings[fp->player & 3u] = MK_WINGS_HIDDEN;
}

/* MetaKnightSpecial.OnActionChanged (Character.on_action_changed, at the end
 * of ftMainSetStatus, with the replaced action's id): an aerial attack, a
 * fall, a landing or an aerial damage action takes the wing state the last
 * other action began with as the four bones' model part; any other action
 * records the first bone's current list. The donor passes the state itself,
 * a display list or -1; the bones have no part table, so any part but -1
 * draws their JointTree list, and part 0 here draws the same. */
void ndsP4MetaknightOnStatus(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *wings = &sNdsP4MetaknightWings[fp->player & 3u];
    s32 i;

    switch (status_id)
    {
    case nFTCommonStatusAttackAirN:
    case nFTCommonStatusAttackAirF:
    case nFTCommonStatusAttackAirB:
    case nFTCommonStatusAttackAirHi:
    case nFTCommonStatusAttackAirLw:
    case nFTCommonStatusFall:
    case nFTCommonStatusFallSpecial:
    case nFTCommonStatusLandingLight:
    case nFTCommonStatusLandingHeavy:
    case nFTCommonStatusDamageAir1:
    case nFTCommonStatusDamageAir2:
    case nFTCommonStatusDamageAir3:
    case nFTCommonStatusDamageFlyHi:
    case nFTCommonStatusDamageFlyLw:
    case nFTCommonStatusDamageFlyN:
    case nFTCommonStatusWallDamage:
    case nFTCommonStatusDamageFall:
    case nFTCommonStatusDamageFlyRoll:
    case nFTCommonStatusDamageFlyTop:
    case nFTCommonStatusDamageE1:
    case nFTCommonStatusDamageE2:
    case nFTCommonStatusCaptureCaptain:
    case nFTCommonStatusLGunShootAir:
    case nFTCommonStatusFireFlowerShootAir:
        for (i = 0; i < 4; i++)
        {
            ftParamSetModelPartID(fighter_gobj, MK_WING_JOINT + i,
                                  (*wings != MK_WINGS_HIDDEN) ? 0 : -1);
        }
        break;

    default:
        *wings = MK_WINGS_HIDDEN;
        if (fp->joints[MK_WING_JOINT] != NULL)
        {
            *wings = (s32)(uintptr_t)fp->joints[MK_WING_JOINT]->dl;
        }
        break;
    }
}

/* ---- Jab ---- */

/* Jab.Start.goto_next: the loop, its first run. */
static void ndsP4MetaknightJabStartGotoNext(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, MK_STATUS_JAB_LOOP, 0.0F, 1.0F, 0);
    ftGetStruct(fighter_gobj)->motion_vars.flags.flag0 = 1;
}

/* Jab.Start.update. */
void ndsP4MetaknightJabStartUpdate(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4MetaknightJabStartGotoNext);
}

/* Jab.Loop.recoil (proc_hit and proc_shield after the first loop): the
 * facing test reads the speed's own register, so the push is always back. */
static void ndsP4MetaknightJabRecoil(GObj *fighter_gobj)
{
    ftGetStruct(fighter_gobj)->physics.vel_ground.x += -15.0F;
}

/* Jab.Loop.loop: the next loop, with the recoil. */
static void ndsP4MetaknightJabLoopAgain(GObj *fighter_gobj)
{
    FTStruct *fp;

    ftMainSetStatus(fighter_gobj, MK_STATUS_JAB_LOOP, 0.0F, 1.0F, 0);
    fp = ftGetStruct(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->proc_shield = ndsP4MetaknightJabRecoil;
    fp->proc_hit = ndsP4MetaknightJabRecoil;
}

/* Jab.Loop.update: after the first run, letting go of A ends it. */
void ndsP4MetaknightJabLoopUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp->motion_vars.flags.flag0 == 0) &&
        !(fp->input.pl.button_hold & MK_BUTTON_A))
    {
        ftMainSetStatus(fighter_gobj, MK_STATUS_JAB_END, 0.0F, 1.0F, 0);
        return;
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4MetaknightJabLoopAgain);
}

/* ---- Forward tilt ---- */

/* TiltF.update (TILTF, which keeps the forward tilt's own action id, and
 * TILTF2): an A press after the window's frame chains the next hit -- from
 * the forward tilt (frame 11) TILTF2, from anything else (frame 7)
 * TILTF3. The window's reset tests frame 2.0 (0x40000000). */
void ndsP4MetaknightTiltFUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 is_tilt = (fp->status_id == nFTCommonStatusAttackS3) ? TRUE : FALSE;

    if (fighter_gobj->anim_frame == 2.0F)
    {
        fp->motion_vars.flags.flag2 = 0;
    }
    if (fp->input.pl.button_tap & MK_BUTTON_A)
    {
        fp->motion_vars.flags.flag2 = 1;
    }
    if ((fp->motion_vars.flags.flag2 != 0) &&
        (((is_tilt != FALSE) ? 11.0F : 7.0F) < fighter_gobj->anim_frame))
    {
        ftMainSetStatus(fighter_gobj,
                        (is_tilt != FALSE) ? MK_STATUS_TILTF2 : MK_STATUS_TILTF3,
                        0.0F, 1.0F, 0);
        ftMainPlayAnimEventsAll(fighter_gobj);
        return;
    }
    ftAnimEndSetWait(fighter_gobj);
}

/* ---- Up special (Shuttle Loop) ---- */

static void ndsP4MetaknightUSPStart(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp;

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, 0);
    fp = ftGetStruct(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->jumps_used = (u8)fp->attr->jumps_max;
}

/* USP.air_initial (air_usp). */
void ndsP4MetaknightUSPAirInitial(GObj *fighter_gobj)
{
    ndsP4MetaknightUSPStart(fighter_gobj, MK_STATUS_USP_AIR_START);
}

/* USP.ground_initial (ground_usp). */
void ndsP4MetaknightUSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4MetaknightUSPStart(fighter_gobj, MK_STATUS_USP);
}

/* USP.air_goto_next: the loop at frame 6, its hitbox out at once. */
static void ndsP4MetaknightUSPAirGotoNext(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, MK_STATUS_USP_AIR, 6.0F, 1.0F, 0);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* USP.air_start_update. */
void ndsP4MetaknightUSPAirStartUpdate(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4MetaknightUSPAirGotoNext);
}

/* USP.main: special fall at the end, drift 0.5, landing lag 1.0. */
void ndsP4MetaknightUSPMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, 0.5F, TRUE, TRUE, TRUE, 1.0F, FALSE);
    }
}

/* USP.air_to_ground. */
static void ndsP4MetaknightUSPAirToGround(GObj *fighter_gobj)
{
    ftCommonLandingFallSpecialSetStatus(fighter_gobj, FALSE, 1.0F);
}

/* USP.collision: grounded, no falling off; in the air no landing before
 * frame 13, drop-through platforms up to frame 28, then the cliff map. */
void ndsP4MetaknightUSPCollision(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 frame = fighter_gobj->anim_frame;

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
        return;
    }
    if (frame < 13.0F)
    {
        mpCommonCheckFighterCeilHeavyCliff(fighter_gobj);
        return;
    }
    fp->coll_data.ignore_line_id = (frame <= 28.0F) ? fp->coll_data.floor_line_id : -1;
    mpCommonProcFighterCliff(fighter_gobj, ndsP4MetaknightUSPAirToGround);
}

/* USP.physics: ground friction to frame 7 (airborne on it), the
 * animation's own motion to 48, then air drift. */
void ndsP4MetaknightUSPPhysics(GObj *fighter_gobj)
{
    f32 frame = fighter_gobj->anim_frame;

    if (frame <= 7.0F)
    {
        ftPhysicsApplyGroundVelFriction(fighter_gobj);
        if (fighter_gobj->anim_frame == 7.0F)
        {
            mpCommonSetFighterAir(ftGetStruct(fighter_gobj));
        }
    }
    else if (frame <= 48.0F)
    {
        ftPhysicsApplyAirVelTransNAll(fighter_gobj);
    }
    else
    {
        ftPhysicsApplyAirVelDrift(fighter_gobj);
    }
}

/* USP.interrupt: on frame 7 a stick past 50 tilts the top joint (Mario's
 * Super Jump Punch turn), kept in temp variable 1 and re-applied every frame
 * over the animation's own rotation. */
void ndsP4MetaknightUSPInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *transn = fp->joints[nFTPartsJointTransN];

    if (fighter_gobj->anim_frame == 7.0F)
    {
        s32 stick_x = fp->input.pl.stick_range.x;
        s32 mag = (stick_x < 0) ? -stick_x : stick_x;

        if (mag >= 50)
        {
            s32 clamp = (stick_x <= 0) ? -50 : 50;
            f32 rot = (f32)(stick_x - clamp) * 0.6F;

            rot = -((rot * MK_PI32) / 180.0F);
            if (((transn->rotate.vec.f.z < 0.0F) ? -transn->rotate.vec.f.z :
                                                   transn->rotate.vec.f.z) <
                ((rot < 0.0F) ? -rot : rot))
            {
                transn->rotate.vec.f.z = rot;
                *ndsP4MetaknightTemp1F(fp) = rot;
            }
        }
    }
    transn->rotate.vec.f.z = *ndsP4MetaknightTemp1F(fp);
}

/* ---- Neutral special (Mach Tornado) ---- */

static void ndsP4MetaknightNSPSetVars(FTStruct *fp)
{
    *ndsP4MetaknightTemp1F(fp) = 130.0F;
    *ndsP4MetaknightTemp2(fp) = 0;
}

/* NSP.air_initial (air_nsp): x stopped, y at 0.4 (0x3ECC) of itself and a
 * 30 hop unless falling. */
void ndsP4MetaknightNSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp;
    f32 y;

    ftMainSetStatus(fighter_gobj, MK_STATUS_TORNADO_START, 0.0F, 1.0F, 0);
    fp = ftGetStruct(fighter_gobj);
    ndsP4MetaknightNSPSetVars(fp);
    fp->physics.vel_air.x = 0.0F;
    fp->jumps_used = (u8)fp->attr->jumps_max;
    y = fp->physics.vel_air.y * 0.3984375F;
    fp->physics.vel_air.y = 0.0F;
    if (!(y < 0.0F))
    {
        fp->physics.vel_air.y = y + 30.0F;
    }
}

/* NSP.ground_initial (ground_nsp): airborne with a 30 hop. */
void ndsP4MetaknightNSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, MK_STATUS_TORNADO_START, 0.0F, 1.0F, 0);
    ndsP4MetaknightNSPSetVars(fp);
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 30.0F;
    fp->jumps_used = (u8)fp->attr->jumps_max;
}

/* NSP.Start.goto_next: the loop at 130 speed. */
static void ndsP4MetaknightNSPStartGotoNext(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, MK_STATUS_TORNADO_LOOP, 0.0F, 130.0F, 0);
}

/* NSP.Start.update. */
void ndsP4MetaknightNSPStartUpdate(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4MetaknightNSPStartGotoNext);
}

/* NSP.Loop.goto_next: half the x speed, the aerial end. */
static void ndsP4MetaknightNSPLoopGotoNext(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->physics.vel_air.x = fp->physics.vel_air.x / 2.0F;
    ftMainSetStatus(fighter_gobj, MK_STATUS_TORNADO_END_AIR, 0.0F, 1.0F, 0);
}

/* NSP.Loop.update: the spin slows by 2 a frame for 60 frames, B mashed every
 * 4 frames adding 7 and lifting him (26, at most 36), then by 3; a whoosh on
 * each full turn, dust under him every 8 frames on the ground; the spin is
 * the animation speed, and at 60 it ends. */
void ndsP4MetaknightNSPLoopUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 *speed = ndsP4MetaknightTemp1F(fp);
    s32 *cooldown = ndsP4MetaknightTemp2(fp);

    if ((s32)fp->status_total_tics < 60)
    {
        *speed -= 2.0F;
        *cooldown -= 1;
        if ((*cooldown <= 0) && (fp->input.pl.button_tap & MK_BUTTON_B))
        {
            *cooldown = 4;
            *speed += 7.0F;
            ftPhysicsAddClampAirVelY(fp, 26.0F, 36.0F);
        }
    }
    else
    {
        *speed -= 3.0F;
    }
    if (fighter_gobj->anim_frame <= *speed)
    {
        func_800269C0_275C0(MK_FGM_TORNADO);
    }
    if (((fp->status_total_tics & 7u) == 0u) &&
        (fp->coll_data.mask_prev & MAP_FLAG_FLOOR))
    {
        Vec3f pos;

        pos.x = fp->coll_data.p_translate->x + ((f32)fp->lr * -80.0F);
        pos.y = fp->coll_data.p_translate->y + -80.0F;
        pos.z = 0.0F;
        efManagerDustHeavyDoubleMakeEffect(&pos, -fp->lr, 1.0F);
    }
    gcSetAnimSpeed(fighter_gobj, *speed);
    if (*speed <= 60.0F)
    {
        ndsP4MetaknightNSPLoopGotoNext(fighter_gobj);
    }
}

/* NSP.Loop.physics: gravity 1.0 to 12, stick drift (0.3 to 80 while the
 * script runs, 0.06 to 60 after it), air friction. */
void ndsP4MetaknightNSPLoopPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsApplyGravityClampTVel(fp, 1.0F, 12.0F);
    if (fp->motion_scripts[1][0].p_script != NULL)
    {
        ftPhysicsClampAirVelXStickRange(fp, 0, 0.3F, 80.0F);
    }
    else
    {
        ftPhysicsClampAirVelXStickRange(fp, 0, 0.06F, 60.0F);
    }
    ftPhysicsApplyAirVelXFriction(fp, fp->attr);
}

/* NSP.Loop.collision: walls bounce him back at twice the speed. */
void ndsP4MetaknightNSPLoopCollision(GObj *fighter_gobj)
{
    FTStruct *fp;

    mpCommonCheckFighterCeilHeavyCliff(fighter_gobj);
    fp = ftGetStruct(fighter_gobj);
    if (fp->coll_data.mask_curr & (MAP_FLAG_LWALL | MAP_FLAG_RWALL))
    {
        fp->physics.vel_ground.x *= -2.0F;
        fp->physics.vel_air.x *= -2.0F;
    }
}

/* NSP.End.air_to_ground: the grounded end from the same frame (the change's
 * fifth argument is a stack word the routine never writes; 0 here). */
static void ndsP4MetaknightNSPEndAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, MK_STATUS_TORNADO_END,
                    fighter_gobj->anim_frame, 1.0F, 0);
}

/* NSP.End.air_collision (TORNADO_END_AIR's map). */
void ndsP4MetaknightNSPEndAirCollision(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4MetaknightNSPEndAirToGround);
}

/* ---- Down special (Dimensional Cape) ---- */

/* DSP.ground_initial (ground_dsp). */
void ndsP4MetaknightDSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp;

    ftMainSetStatus(fighter_gobj, MK_STATUS_CAPE_START, 0.0F, 1.0F, 0);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp = ftGetStruct(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    *ndsP4MetaknightTemp2(fp) = -1;
}

/* DSP.air_initial (air_dsp): the speed halved. */
void ndsP4MetaknightDSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp;

    ftMainSetStatus(fighter_gobj, MK_STATUS_CAPE_AIR_START, 0.0F, 1.0F, 0);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp = ftGetStruct(fighter_gobj);
    fp->physics.vel_air.x *= 0.5F;
    fp->physics.vel_air.y *= 0.5F;
    fp->motion_vars.flags.flag0 = 0;
    *ndsP4MetaknightTemp2(fp) = -1;
}

/* The cape's vanish: invisible, never jostled, fully transparent and
 * intangible (DSP.Start.update _visibility/_intangibility, and the two
 * map transitions while teleporting). */
static void ndsP4MetaknightDSPVanish(FTStruct *fp)
{
    fp->is_invisible = TRUE;
    fp->is_jostle_ignore = TRUE;
    gNdsP4EnvColor[fp->player & 3u] = MK_ENV_CLOAK;
    fp->hitstatus = MK_HITSTATUS_INTANGIBLE;
}

/* DSP.Start.goto_next: on the ground no button ends it (CAPE_END), A or B
 * attacks -- a stick held against his facing CAPE_F, else turned round
 * CAPE_B, near the middle CAPE_N; in the air the same with the speed
 * stopped. Ending turns him to a stick past 11, reversing his ground speed
 * when he turns. */
static void ndsP4MetaknightDSPStartGotoNext(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 is_air = (fp->ga != nMPKineticsGround) ? TRUE : FALSE;
    sb32 held = (fp->input.pl.button_hold & (MK_BUTTON_A | MK_BUTTON_B)) ? TRUE : FALSE;
    s32 stick_x = fp->input.pl.stick_range.x;
    s32 next;

    if (held != FALSE)
    {
        if (is_air != FALSE)
        {
            fp->physics.vel_air.x = 0.0F;
            fp->physics.vel_air.y = 0.0F;
        }
        if ((stick_x < -11) || (stick_x >= 12))
        {
            if ((fp->lr ^ stick_x) < 0)
            {
                next = (is_air != FALSE) ? MK_STATUS_CAPE_AIR_F : MK_STATUS_CAPE_F;
            }
            else
            {
                fp->lr = -fp->lr;
                next = (is_air != FALSE) ? MK_STATUS_CAPE_AIR_B : MK_STATUS_CAPE_B;
            }
        }
        else
        {
            next = (is_air != FALSE) ? MK_STATUS_CAPE_AIR_N : MK_STATUS_CAPE_N;
        }
    }
    else
    {
        next = (is_air != FALSE) ? MK_STATUS_CAPE_AIR_END : MK_STATUS_CAPE_END;
        if (((stick_x < 0) ? -stick_x : stick_x) >= 11)
        {
            s32 lr = fp->lr;

            ftParamSetStickLR(fp);
            if (fp->lr != lr)
            {
                fp->physics.vel_ground.x = -fp->physics.vel_ground.x;
            }
        }
    }
    ftMainSetStatus(fighter_gobj, next, 0.0F, 1.0F, 0);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* DSP.Start.update: on frame 13 (exactly) the cape closes: half speed; a
 * stick under 11 stays put (temp variable 2 = 1), else he moves along the
 * stick's angle (temp variable 1); grounded with the stick up he takes the
 * aerial start; his jumps are spent; he vanishes. The delay slot stores his
 * jump count as the angle word first, which only a stay-put cape keeps. */
void ndsP4MetaknightDSPStartUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fighter_gobj->anim_frame == 13.0F)
    {
        s32 stick_x;
        s32 stick_y;
        s32 mag;

        gcSetAnimSpeed(fighter_gobj, 0.5F);
        stick_x = fp->input.pl.stick_range.x;
        stick_y = fp->input.pl.stick_range.y;
        mag = (s32)sqrtf((f32)((stick_x * stick_x) + (stick_y * stick_y)));
        *ndsP4MetaknightTemp2(fp) = (mag < 11) ? 1 : 0;
        if ((fp->ga == nMPKineticsGround) && (stick_y > 0))
        {
            mpCommonSetFighterAir(fp);
            ftMainSetStatus(fighter_gobj, MK_STATUS_CAPE_AIR_START,
                            fighter_gobj->anim_frame, 0.5F, 0);
            fp = ftGetStruct(fighter_gobj);
        }
        fp->jumps_used = (u8)fp->attr->jumps_max;
        fp->motion_vars.flags.flag0 = (u32)fp->attr->jumps_max;
        if (*ndsP4MetaknightTemp2(fp) == 0)
        {
            *ndsP4MetaknightTemp1F(fp) =
                syUtilsArcTan2((f32)fp->input.pl.stick_range.y,
                               (f32)(fp->input.pl.stick_range.x * fp->lr));
        }
        ndsP4MetaknightDSPVanish(fp);
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4MetaknightDSPStartGotoNext);
}

/* DSP.Start.apply_movement_: 60 along the angle. */
static void ndsP4MetaknightDSPApplyMovement(FTStruct *fp)
{
    f32 angle = *ndsP4MetaknightTemp1F(fp);
    f32 x = MK_CAPE_SPEED * __cosf(angle);
    f32 y = MK_CAPE_SPEED * __sinf(angle);

    if (fp->ga == nMPKineticsGround)
    {
        fp->physics.vel_ground.x = x;
        fp->physics.vel_air.x = x * (f32)fp->lr;
    }
    else
    {
        fp->physics.vel_air.x = x * (f32)fp->lr;
        fp->physics.vel_air.y = y;
    }
}

/* DSP.Start.physics: moves once the cape has closed with a moving stick. */
void ndsP4MetaknightDSPStartPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (*ndsP4MetaknightTemp2(fp) == 0)
    {
        ndsP4MetaknightDSPApplyMovement(fp);
    }
}

/* DSP.Start.air_to_ground / ground_to_air: the other start at the same frame
 * and animation speed, vanished again if the cape had closed. */
static void ndsP4MetaknightDSPSwitch(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp;

    ftMainSetStatus(fighter_gobj, status_id, fighter_gobj->anim_frame,
                    DObjGetStruct(fighter_gobj)->anim_speed, 0);
    fp = ftGetStruct(fighter_gobj);
    if (*ndsP4MetaknightTemp2(fp) >= 0)
    {
        ndsP4MetaknightDSPVanish(fp);
    }
}

static void ndsP4MetaknightDSPAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ndsP4MetaknightDSPSwitch(fighter_gobj, MK_STATUS_CAPE_START);
}

static void ndsP4MetaknightDSPGroundToAir(GObj *fighter_gobj)
{
    mpCommonSetFighterAir(ftGetStruct(fighter_gobj));
    ndsP4MetaknightDSPSwitch(fighter_gobj, MK_STATUS_CAPE_AIR_START);
}

/* DSP.Start.air_collision. */
void ndsP4MetaknightDSPStartAirCollision(GObj *fighter_gobj)
{
    mpCommonProcFighterCliff(fighter_gobj, ndsP4MetaknightDSPAirToGround);
}

/* DSP.Start.ground_collision: he can slide off ledges up to frame 16. */
void ndsP4MetaknightDSPStartGroundCollision(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 16.0F)
    {
        mpCommonProcFighterOnFloor(fighter_gobj, ndsP4MetaknightDSPGroundToAir);
    }
    else
    {
        mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
    }
}

/* DSP.Attack.air_main (also TORNADO_END_AIR's update): special fall at the
 * end, drift 0.6. */
void ndsP4MetaknightDSPAttackAirMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, 0.6F, TRUE, TRUE, TRUE, 1.0F, FALSE);
    }
}

/* DSP.Attack.air_physics: the animation's motion to frame 21, then air
 * friction. */
void ndsP4MetaknightDSPAttackAirPhysics(GObj *fighter_gobj)
{
    if (21.0F <= fighter_gobj->anim_frame)
    {
        ftPhysicsApplyAirVelFriction(fighter_gobj);
    }
    else
    {
        ftPhysicsApplyAirVelTransNAll(fighter_gobj);
    }
}

/* ---- Patches on his id ---- */

/* MetaKnight.capture_action_fix_ (Character.custom_capture_dk_interrupt):
 * a ThrownDK victim he holds never mashes out. */
static sb32 ndsP4MetaknightShoulderedInterrupt(GObj *victim_gobj)
{
    (void)victim_gobj;
    return TRUE;
}

/* MetaKnight's action_replace_map_ (main.asm): the jab and the forward tilt
 * run his own statuses under their own ids. */
static const u16 sNdsP4MetaknightStatusReplace[] = {
    nFTCommonStatusAttack11, MK_STATUS_JAB_START,
    nFTCommonStatusAttackS3, MK_STATUS_TILTF,
    0xFFFF
};

const NDSP4Overrides gNdsP4MetaknightOverrides = {
    .on_init = ndsP4MetaknightOnInit,
    .shouldered_interrupt = ndsP4MetaknightShoulderedInterrupt,
    .status_replace = sNdsP4MetaknightStatusReplace,
    /* main.asm static_part: "withheld part 5 - left hand item", joint 12
     * (his light-item joint, a hidden part his item motions install). */
    .static_parts = { 0x00800000u, 0u },
};

#if NDS_P2_KIRBY
/* ---- Kirby's copy (owner request 2026-10-12, "Brawl-style") ----
 *
 * EXTRA lets no Kirby copy Meta Knight (define_character's bool_inhale_copy
 * is the config's kirby_hat, unset); the owner asked for one, then for Meta
 * Knight's whole neutral special (BUGS.md 2026-10-12). Kirby runs it on his
 * own motions (scripts/p4/p4_kirby.py CUSTOM_POWERS), his scripts replaced
 * by Meta Knight's:
 *  - the start: Meta Knight's 11 tics (from the ground airborne with his 30
 *    hop, in the air x stopped and y at 0.4 of itself plus 30 unless
 *    falling; his jumps used);
 *  - the loop: his spin, the loop animation's speed -- 130, slowing 2 a tic
 *    for 60 tics then 3, B mashed every 4 tics adding 7 and lifting him (26,
 *    at most 36) -- a whoosh at each turn of the animation's 358 frames,
 *    dust under him every 8 tics on the floor, ended at 60 with half the x
 *    speed; moveset_tornadoLoop's two hitboxes (its words verbatim: joint 0,
 *    which Kirby has too; the second from frame 1550), his gravity, drift
 *    and wall bounce; his tornado (his model's joint 34 part 1) drawn around
 *    Kirby, turning with the spin (renderer_adapter_fighter.c
 *    ndsFighterDrawKirbyTornadoSidecar);
 *  - the ends: his 54 tics, the grounded one with moveset_tornadoEnd's two
 *    effects, the aerial one into a special fall (drift 0.6), a landing
 *    taking the grounded one for the rest. */
#define MK_KIRBY_START_TICS 11
#define MK_KIRBY_LOOP_FRAMES 358.0F
#define MK_KIRBY_SECOND_HIT 1550.0F
#define MK_KIRBY_END_TICS 54
#define MK_KIRBY_END_EFFECT_TICS 10

typedef struct NDSP4MetaknightKirby
{
    f32 speed;        /* the loop animation's speed */
    f32 frame;        /* the loop animation's frame */
    f32 progress;     /* the loop's frames so far */
    s32 cooldown;     /* tics to the next mash */
    s32 end_tics;     /* the end's tics so far */
    u16 angle;        /* the tornado's turn */
    u8 second;        /* the second hitbox is out */
} NDSP4MetaknightKirby;

static NDSP4MetaknightKirby sNdsP4MetaknightKirby[GMCOMMON_PLAYERS_MAX];

/* moveset_tornadoLoop: a slash on joint 0, size 600 at y 300, 361 degrees,
 * 50 base; 12% with 80 growth, then from frame 1550 8% at size 520 with 70
 * growth. */
static const u32 sNdsP4MetaknightKirbyLoopHit1[] = {
    0x0C000183u, 0x02580000u, 0x012C0000u, 0x5A450003u, 0x004A1900u,
    NDS_P4_MOTION_EVENT(nFTMotionEventEnd, 0),
};
static const u32 sNdsP4MetaknightKirbyLoopHit2[] = {
    0x0C000103u, 0x02080000u, 0x012C0000u, 0x5A446003u, 0x000A1900u,
    NDS_P4_MOTION_EVENT(nFTMotionEventEnd, 0),
};
/* moveset_tornadoEnd's effects (its eye texture parts are Meta Knight's). */
static const u32 sNdsP4MetaknightKirbyEndScript[] = {
    0x98005800u, 0x00000000u, 0x00000000u, 0x00000000u,
    NDS_P4_MOTION_EVENT(nFTMotionEventAsyncWait, MK_KIRBY_END_EFFECT_TICS),
    0x98003400u, 0x0000FFA0u, 0x00000000u, 0x00000000u,
    NDS_P4_MOTION_EVENT(nFTMotionEventEnd, 0),
};

static NDSP4MetaknightKirby *ndsP4MetaknightKirbyState(const FTStruct *fp)
{
    return &sNdsP4MetaknightKirby[fp->player & 3];
}

/* A status of the power with its script in place of the motion's (Kirby's
 * own, whose first frame plays muted). */
static void ndsP4MetaknightKirbySetStatus(GObj *fighter_gobj, s32 status_id, const u32 *script)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    ub32 is_muted = fp->is_muted;

    fp->is_muted = TRUE;
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    fp->is_muted = is_muted;
    ftParamClearAttackCollAll(fighter_gobj);
    ndsP4KirbySetMotionScript(fp, script);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

static void ndsP4MetaknightKirbyNSPStart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4MetaknightKirbySetStatus(fighter_gobj, NDS_P4_KIRBY_STATUS_METAKNIGHT_NSP_START, NULL);
    fp->physics.vel_air.x = 0.0F;
    fp->jumps_used = (u8)fp->attr->jumps_max;
}

void ndsP4MetaknightKirbyNSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ndsP4MetaknightKirbyNSPStart(fighter_gobj);
    fp->physics.vel_air.y = 30.0F;
}

void ndsP4MetaknightKirbyNSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 y = fp->physics.vel_air.y * 0.3984375F;

    ndsP4MetaknightKirbyNSPStart(fighter_gobj);
    fp->physics.vel_air.y = (y < 0.0F) ? 0.0F : (y + 30.0F);
}

void ndsP4MetaknightKirbyNSPStartUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    NDSP4MetaknightKirby *mk;

    if ((s32)fp->status_total_tics < MK_KIRBY_START_TICS)
    {
        return;
    }
    ndsP4MetaknightKirbySetStatus(fighter_gobj, NDS_P4_KIRBY_STATUS_METAKNIGHT_NSP_LOOP,
                                  sNdsP4MetaknightKirbyLoopHit1);
    mk = ndsP4MetaknightKirbyState(fp);
    mk->speed = 130.0F;
    mk->frame = 0.0F;
    mk->progress = 0.0F;
    mk->cooldown = 0;
    mk->second = FALSE;
}

/* NSP.Loop.update on a counted spin: the animation advances by last tic's
 * speed before the update reads it, as Meta Knight's does. */
void ndsP4MetaknightKirbyNSPLoopUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    NDSP4MetaknightKirby *mk = ndsP4MetaknightKirbyState(fp);

    mk->frame += mk->speed;
    mk->progress += mk->speed;
    while (mk->frame >= MK_KIRBY_LOOP_FRAMES)
    {
        mk->frame -= MK_KIRBY_LOOP_FRAMES;
    }
    if ((s32)fp->status_total_tics < 60)
    {
        mk->speed -= 2.0F;
        mk->cooldown -= 1;
        if ((mk->cooldown <= 0) && (fp->input.pl.button_tap & MK_BUTTON_B))
        {
            mk->cooldown = 4;
            mk->speed += 7.0F;
            ftPhysicsAddClampAirVelY(fp, 26.0F, 36.0F);
        }
    }
    else
    {
        mk->speed -= 3.0F;
    }
    if (mk->frame <= mk->speed)
    {
        func_800269C0_275C0(MK_FGM_TORNADO);
    }
    if ((mk->second == FALSE) && (mk->progress >= MK_KIRBY_SECOND_HIT))
    {
        mk->second = TRUE;
        ndsP4KirbySetMotionScript(fp, sNdsP4MetaknightKirbyLoopHit2);
    }
    if (((fp->status_total_tics & 7u) == 0u) &&
        (fp->coll_data.mask_prev & MAP_FLAG_FLOOR))
    {
        Vec3f pos;

        pos.x = fp->coll_data.p_translate->x + ((f32)fp->lr * -80.0F);
        pos.y = fp->coll_data.p_translate->y + -80.0F;
        pos.z = 0.0F;
        efManagerDustHeavyDoubleMakeEffect(&pos, -fp->lr, 1.0F);
    }
    /* A twelfth of a turn a tic at the full spin. */
    mk->angle = (u16)(mk->angle + (s32)(mk->speed * (5461.0F / 130.0F)));
    if (mk->speed <= 60.0F)
    {
        fp->physics.vel_air.x = fp->physics.vel_air.x / 2.0F;
        ndsP4MetaknightKirbySetStatus(fighter_gobj, NDS_P4_KIRBY_STATUS_METAKNIGHT_NSP_END_AIR,
                                      NULL);
        mk->end_tics = 0;
    }
}

/* NSP.Loop.physics: the strong drift while the loop script runs (until its
 * second hitbox). */
void ndsP4MetaknightKirbyNSPLoopPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsApplyGravityClampTVel(fp, 1.0F, 12.0F);
    if (ndsP4MetaknightKirbyState(fp)->second == FALSE)
    {
        ftPhysicsClampAirVelXStickRange(fp, 0, 0.3F, 80.0F);
    }
    else
    {
        ftPhysicsClampAirVelXStickRange(fp, 0, 0.06F, 60.0F);
    }
    ftPhysicsApplyAirVelXFriction(fp, fp->attr);
}

void ndsP4MetaknightKirbyNSPEndUpdate(GObj *fighter_gobj)
{
    NDSP4MetaknightKirby *mk = ndsP4MetaknightKirbyState(ftGetStruct(fighter_gobj));

    if (++mk->end_tics >= MK_KIRBY_END_TICS)
    {
        ftCommonWaitSetStatus(fighter_gobj);
    }
}

void ndsP4MetaknightKirbyNSPEndAirUpdate(GObj *fighter_gobj)
{
    NDSP4MetaknightKirby *mk = ndsP4MetaknightKirbyState(ftGetStruct(fighter_gobj));

    if (++mk->end_tics >= MK_KIRBY_END_TICS)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, 0.6F, TRUE, TRUE, TRUE, 1.0F, FALSE);
    }
}

/* NSP.End.air_to_ground: the grounded end for the rest of the tics, its
 * effects only from its start. */
static void ndsP4MetaknightKirbyNSPEndAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    NDSP4MetaknightKirby *mk = ndsP4MetaknightKirbyState(fp);
    s32 end_tics = mk->end_tics;

    mpCommonSetFighterGround(fp);
    ndsP4MetaknightKirbySetStatus(fighter_gobj, NDS_P4_KIRBY_STATUS_METAKNIGHT_NSP_END,
                                  (end_tics == 0) ? sNdsP4MetaknightKirbyEndScript : NULL);
    mk->end_tics = end_tics;
}

void ndsP4MetaknightKirbyNSPEndAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4MetaknightKirbyNSPEndAirToGround);
}

sb32 ndsP4MetaknightKirbyTornado(const FTStruct *fp, u16 *angle)
{
    if ((fp->fkind != nFTKindKirby) ||
        (fp->status_id != NDS_P4_KIRBY_STATUS_METAKNIGHT_NSP_LOOP))
    {
        return FALSE;
    }
    *angle = ndsP4MetaknightKirbyState(fp)->angle;
    return TRUE;
}
#endif /* NDS_P2_KIRBY */

#endif /* NDS_P4_METAKNIGHT */
