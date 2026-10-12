/*
 * P4 Mad Piano: native ports of the donor's special routines and of the
 * patches Remix makes on his character id. Source: JSsixtyfour/smashremix
 * 5e04fe7, src/Piano/PianoSpecial.asm and Piano.asm, src/fireball.asm (the
 * book), CustomGrabAction.asm and yoshishared.asm (the hooks below), read as
 * assembled (scripts/p4/mipsdis.py). Mad Piano runs Mario's status code
 * (fp->fkind == nFTKindMario): his neutral special is Mario's fireball
 * statuses with the book in the fireball's place, his up special a jump of
 * his own, his down special Ness's absorb fused with Yoshi's command grab.
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x014C kinetic state                   ga
 *   0x017C/0x180/0x184 temp variables 1-3  motion_vars.flags.flag0/1/2
 *   0x018D | 0x80 / & 0x07 / | 0x01        is_absorb / clears is_absorb,
 *                                          absorb_lr, is_goto_attack100 and
 *                                          is_fastfall / is_invisible
 *   0x0190 & ~0x02                         is_item_show
 *   0x01BC buttons held                    input.pl.button_hold
 *   0x01C2 stick x                         input.pl.stick_range.x
 *   0x0830 / 0x0840                        search_gobj / catch_gobj
 *   0x0850 absorb box                      special_coll
 *   0x0ADC bonus ammunition (a float)      sNdsP4PianoBonusAmmo (below)
 *   0x0B18 / 0x0B1C                        status_vars.ness.speciallw
 *                                          .release_lag / .is_release
 *   attributes + 0x4C/0x50/0x58/0x64       air_accel, air_speed_max_x,
 *                                          gravity, jumps_max
 *
 * His book is his own weapon on his special file 1 (Piano file 6,
 * PIANO_PROJECTILE_HITBOX, S6): Remix's struct_book fireball (fireball.asm
 * Book), made by Mario's fireball accessory through the fireball table
 * (NDSP4Overrides.fireball).
 */
#include <nds/nds_p4.h>

#if NDS_P4_PIANO

#if !NDS_P2_NESS
#error "P4 Mad Piano's down special runs Ness's absorb (NDS_P2_NESS)"
#endif
#if !NDS_P2_YOSHI
#error "P4 Mad Piano's down special runs Yoshi's command grab (NDS_P2_YOSHI)"
#endif

#include <ef/effect.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <wp/weapon.h>
#if NDS_P2_KIRBY
#include "kirby/nds_p4_kirby.generated.h"
#endif

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

void ftCaptainSpecialHiProcInterrupt(GObj *fighter_gobj);
sb32 ftMarioSpecialHiProcPass(GObj *fighter_gobj);
void ftNessSpecialLw_InitStatusVars(GObj *fighter_gobj);
void ftNessSpecialLw_UpdateReleaseLag(GObj *fighter_gobj);
void ftNessSpecialLwHitSetAbsorbTrue(GObj *fighter_gobj);
void ftYoshiSpecialNSetCatchParams(FTStruct *fp, void (*proc_catch)(GObj *));
sb32 wpMarioFireballProcHop(GObj *weapon_gobj);
f32 __sinf(f32);
f32 __cosf(f32);
extern f32 lbCommonMag2D(Vec3f *vec);
LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
LBParticle *efManagerDustLightMakeEffect(Vec3f *pos, s32 lr, f32 f_index);
LBParticle *efManagerDamageNormalLightMakeEffect(Vec3f *pos, s32 player, s32 size, sb32 is_static);
GObj *efManagerDamageSpawnMDustMakeEffect(Vec3f *pos, s32 lr);

static f32 ndsP4PianoBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

/* ---- Up special (Super Jump Chomp) ---- */

#define PIANO_USP_STATUS 0xE1
#define PIANO_USP_BEGIN 1
#define PIANO_USP_BEGIN_MOVE 2
#define PIANO_USP_MOVE 3
#define PIANO_USP_Y_SPEED 120.0F                /* lui 0x42F0 */
#define PIANO_USP_X_SPEED 50.0F                 /* lui 0x4248 */
#define PIANO_USP_BEGIN_SLOW 0.875F             /* lui 0x3F60 */
#define PIANO_USP_STICK_X 0.5F
#define PIANO_USP_Y_FROM_X 0.75F
#define PIANO_USP_MOVE_AIR_ACCEL_BITS 0x3C750000u
#define PIANO_USP_FALL_LANDING 0.15625F         /* LANDING_FSM 0x3E20 */
/* collision_ keeps the low half of Mario's constant (0x3E8F5C29). */
#define PIANO_USP_LANDING_BITS 0x3E205C29u

/* PianoUSP.initial_ (ground_usp, air_usp). */
void ndsP4PianoUSPInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga != nMPKineticsAir)
    {
        mpCommonSetFighterAir(fp);
    }
    ftMainSetStatus(fighter_gobj, PIANO_USP_STATUS, 0.0F, 1.0F, 0);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = PIANO_USP_BEGIN;
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
    fp->jumps_used = (u8)fp->attr->jumps_max;
    fp->physics.vel_air.y = fp->attr->gravity;
}

/* PianoUSP.main_ (0xE1 update): Fox's Fire Fox end, his landing lag and no
 * interrupt. */
void ndsP4PianoUSPMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, 1.0F, FALSE, TRUE, FALSE,
                                     PIANO_USP_FALL_LANDING, FALSE);
    }
}

/* PianoUSP.change_direction_ (interrupt): Falcon Dive's turn when the
 * script sets temp variable 2 to 2. */
void ndsP4PianoUSPChangeDirection(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->motion_vars.flags.flag1 == 2)
    {
        ftCaptainSpecialHiProcInterrupt(fighter_gobj);
    }
}

/* PianoUSP.air_control_: Mario's drift, slower once moving. */
static void ndsP4PianoUSPAirControl(FTStruct *fp, FTAttributes *attr)
{
    f32 accel = (fp->motion_vars.flags.flag2 == PIANO_USP_MOVE) ?
        ndsP4PianoBitsToF32(PIANO_USP_MOVE_AIR_ACCEL_BITS) : attr->air_accel;

    ftPhysicsClampAirVelXStickRange(fp, 8, accel, attr->air_speed_max_x);
}

/* PianoUSP.physics_: the air physics with no drift while beginning, then
 * temp variable 3's phases. */
void ndsP4PianoUSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    (fp->is_fastfall) ? ftPhysicsApplyFastFall(fp, attr) :
                        ftPhysicsApplyGravityDefault(fp, attr);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        if (fp->motion_vars.flags.flag2 == PIANO_USP_BEGIN)
        {
            (void)ftPhysicsCheckClampAirVelXDecMax(fp, attr);
        }
        else ndsP4PianoUSPAirControl(fp, attr);

        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
    if (fp->motion_vars.flags.flag2 == PIANO_USP_BEGIN)
    {
        fp->physics.vel_air.x *= PIANO_USP_BEGIN_SLOW;
        fp->physics.vel_air.y = 0.0F;
    }
    if (fp->motion_vars.flags.flag2 == PIANO_USP_BEGIN_MOVE)
    {
        f32 stick = (f32)fp->input.pl.stick_range.x * (f32)fp->lr;
        f32 vel_x = 0.0F;
        f32 vel_y = PIANO_USP_Y_SPEED;

        if (stick >= 0.0F)
        {
            vel_x = stick * PIANO_USP_STICK_X;
            vel_y -= PIANO_USP_Y_FROM_X * vel_x;
        }
        fp->physics.vel_air.x = (f32)fp->lr * (vel_x + PIANO_USP_X_SPEED);
        fp->physics.vel_air.y = vel_y;
        fp->motion_vars.flags.flag2 = PIANO_USP_MOVE;
    }
}

/* PianoUSP.collision_: Mario's Super Jump map routine with his landing lag. */
void ndsP4PianoUSPMap(GObj *fighter_gobj)
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
            else ftCommonLandingFallSpecialSetStatus(
                fighter_gobj, FALSE, ndsP4PianoBitsToF32(PIANO_USP_LANDING_BITS));
        }
    }
    else mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
}

/* ---- Down special (eat) ---- */

#define PIANO_DSP_GROUND_BEGIN 0xE5
#define PIANO_DSP_GROUND_WAIT 0xE6
#define PIANO_DSP_GROUND_ABSORB 0xE7
#define PIANO_DSP_GROUND_END 0xE8
#define PIANO_DSP_AIR_BEGIN 0xE9
#define PIANO_DSP_AIR_WAIT 0xEA
#define PIANO_DSP_AIR_ABSORB 0xEB
#define PIANO_DSP_AIR_END 0xEC
#define PIANO_DSP_GROUND_THROW 0xED
#define PIANO_DSP_AIR_THROW 0xEE
#define PIANO_DSP_WAIT_FLAGS (FTSTATUS_PRESERVE_LOOPSFX | FTSTATUS_PRESERVE_EFFECT)
#define PIANO_DSP_THROW_FLAGS (FTSTATUS_PRESERVE_EFFECT | FTSTATUS_PRESERVE_MODELPART)
#define PIANO_DSP_THROW_IMMUNE 0x3F
#define PIANO_DSP_THROW_ACCEL_BITS 0x3DCCCCCDu  /* "0.1" */
#define PIANO_DSP_THROW_SPEED 26.0F

/* PianoDSP.absorb_struct: Ness's absorb kind on joint 0. Its ninth word,
 * which Remix's eight-word struct leaves to whatever follows it, is never
 * read for an absorber. */
static FTSpecialColl sNdsP4PianoAbsorbColl = {
    nFTSpecialCollKindNessAbsorb, 0, { 0.0F, 330.0F, 330.0F }, { 350.0F, 350.0F, 350.0F }, 0
};

/* air_to_ground_table / ground_to_air_table: the other side's status, the
 * change's flags, the absorb flag and the command grab. */
typedef struct NDSP4PianoDSPSwitch
{
    u16 status_id;
    u16 flags;
    u16 is_absorb;
    u16 is_command_grab;
} NDSP4PianoDSPSwitch;

static const NDSP4PianoDSPSwitch sNdsP4PianoDSPToGround[4] = {
    { PIANO_DSP_GROUND_BEGIN, 0x0092, FALSE, FALSE },
    { PIANO_DSP_GROUND_WAIT, 0x0097, TRUE, TRUE },
    { PIANO_DSP_GROUND_ABSORB, 0x0097, TRUE, FALSE },
    { PIANO_DSP_GROUND_END, 0x0092, FALSE, FALSE },
};

static const NDSP4PianoDSPSwitch sNdsP4PianoDSPToAir[4] = {
    { PIANO_DSP_AIR_BEGIN, 0x0092, FALSE, FALSE },
    { PIANO_DSP_AIR_WAIT, 0x0097, TRUE, TRUE },
    { PIANO_DSP_AIR_ABSORB, 0x0097, TRUE, FALSE },
    { PIANO_DSP_AIR_END, 0x0092, FALSE, FALSE },
};

static void ndsP4PianoDSPGroundThrowInitial(GObj *fighter_gobj);
static void ndsP4PianoDSPAirThrowInitial(GObj *fighter_gobj);

/* ground_begin_initial_ / air_begin_initial_ (ground_dsp, air_dsp). */
static void ndsP4PianoDSPBeginInitial(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, 0);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftNessSpecialLw_InitStatusVars(fighter_gobj);
    ftGetStruct(fighter_gobj)->motion_vars.flags.flag1 = 0;
}

void ndsP4PianoDSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4PianoDSPBeginInitial(fighter_gobj, PIANO_DSP_GROUND_BEGIN);
}

void ndsP4PianoDSPAirInitial(GObj *fighter_gobj)
{
    ndsP4PianoDSPBeginInitial(fighter_gobj, PIANO_DSP_AIR_BEGIN);
}

/* absorb_setup_: the absorb flag and his absorb box. */
static void ndsP4PianoDSPAbsorbSetup(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->is_absorb = TRUE;
    fp->special_coll = &sNdsP4PianoAbsorbColl;
}

/* ground_wait_initial_ / air_wait_initial_: the open mouth, which grabs
 * like Yoshi's egg lay and absorbs like Ness's magnet. */
static void ndsP4PianoDSPWaitInitial(GObj *fighter_gobj, s32 status_id,
                                     void (*proc_catch)(GObj *))
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, PIANO_DSP_WAIT_FLAGS);
    ftYoshiSpecialNSetCatchParams(fp, proc_catch);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
    ftMainPlayAnimEventsAll(fighter_gobj);
    ndsP4PianoDSPAbsorbSetup(fighter_gobj);
}

static void ndsP4PianoDSPGroundWaitInitial(GObj *fighter_gobj)
{
    ndsP4PianoDSPWaitInitial(fighter_gobj, PIANO_DSP_GROUND_WAIT, ndsP4PianoDSPGroundThrowInitial);
}

static void ndsP4PianoDSPAirWaitInitial(GObj *fighter_gobj)
{
    ndsP4PianoDSPWaitInitial(fighter_gobj, PIANO_DSP_AIR_WAIT, ndsP4PianoDSPAirThrowInitial);
}

/* ground_absorb_initial_ / air_absorb_initial_: entered from Ness's absorb
 * routine (ground/air_absorb_initial_patch_, NDSP4Overrides.absorb_hit). */
static void ndsP4PianoDSPAbsorbInitial(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_EFFECT);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftNessSpecialLwHitSetAbsorbTrue(fighter_gobj);
    ftGetStruct(fighter_gobj)->motion_vars.flags.flag1 = 0;
}

/* ground_end_initial_ / air_end_initial_. */
static void ndsP4PianoDSPEndInitial(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, 0);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* ground_begin_main_ (0xE5 update) / air_begin_main_ (0xE9). */
void ndsP4PianoDSPGroundBeginMain(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4PianoDSPGroundWaitInitial);
}

void ndsP4PianoDSPAirBeginMain(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4PianoDSPAirWaitInitial);
}

/* ground_wait_main_ (0xE6) / air_wait_main_ (0xEA): Ness's release lag, then
 * the end once B is let go. */
static void ndsP4PianoDSPWaitMain(GObj *fighter_gobj, s32 end_status)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftNessSpecialLw_UpdateReleaseLag(fighter_gobj);

    if ((fp->status_vars.ness.speciallw.release_lag <= 0) &&
        (fp->status_vars.ness.speciallw.is_release != FALSE))
    {
        ndsP4PianoDSPEndInitial(fighter_gobj, end_status);
    }
}

void ndsP4PianoDSPGroundWaitMain(GObj *fighter_gobj)
{
    ndsP4PianoDSPWaitMain(fighter_gobj, PIANO_DSP_GROUND_END);
}

void ndsP4PianoDSPAirWaitMain(GObj *fighter_gobj)
{
    ndsP4PianoDSPWaitMain(fighter_gobj, PIANO_DSP_AIR_END);
}

/* ground_absorb_main_ (0xE7) / air_absorb_main_ (0xEB): once the script sets
 * temp variable 2, letting go of B ends in the wait or the fall; else the
 * mouth reopens at the animation's end. */
void ndsP4PianoDSPGroundAbsorbMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp->motion_vars.flags.flag1 != 0) && !(fp->input.pl.button_hold & B_BUTTON))
    {
        ftCommonWaitSetStatus(fighter_gobj);
        return;
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4PianoDSPGroundWaitInitial);
}

void ndsP4PianoDSPAirAbsorbMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp->motion_vars.flags.flag1 != 0) && !(fp->input.pl.button_hold & B_BUTTON))
    {
        ftCommonFallSetStatus(fighter_gobj);
        return;
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4PianoDSPAirWaitInitial);
}

/* ground_to_air_ / air_to_ground_: the same phase on the other side at the
 * same frame. */
static void ndsP4PianoDSPGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 index = (u32)(fp->status_id - PIANO_DSP_GROUND_BEGIN) & 3u;

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, sNdsP4PianoDSPToAir[index].status_id,
                    fighter_gobj->anim_frame, 1.0F, sNdsP4PianoDSPToAir[index].flags);
    ftPhysicsClampAirVelXMax(fp);

    if (sNdsP4PianoDSPToAir[index].is_absorb != FALSE)
    {
        fp->is_absorb = TRUE;
    }
    if (sNdsP4PianoDSPToAir[index].is_command_grab != FALSE)
    {
        ftYoshiSpecialNSetCatchParams(fp, ndsP4PianoDSPAirThrowInitial);
    }
}

static void ndsP4PianoDSPAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 index = (u32)(fp->status_id - PIANO_DSP_AIR_BEGIN) & 3u;

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, sNdsP4PianoDSPToGround[index].status_id,
                    fighter_gobj->anim_frame, 1.0F, sNdsP4PianoDSPToGround[index].flags);

    if (sNdsP4PianoDSPToGround[index].is_absorb != FALSE)
    {
        fp->is_absorb = TRUE;
    }
    if (sNdsP4PianoDSPToGround[index].is_command_grab != FALSE)
    {
        ftYoshiSpecialNSetCatchParams(fp, ndsP4PianoDSPGroundThrowInitial);
    }
}

/* ground_collision_ (0xE5-0xE8 map): off the edge into the air phase. */
void ndsP4PianoDSPGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4PianoDSPGroundToAir);
}

/* air_collision_ (0xE9-0xEC map): landing into the ground phase. */
void ndsP4PianoDSPAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4PianoDSPAirToGround);
}

/* command_throw_setup_: immune to every grab, and the fighter the search
 * found is the one held. */
static void ndsP4PianoDSPThrowSetup(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftParamSetCaptureImmuneMask(fp, PIANO_DSP_THROW_IMMUNE);
    fp->catch_gobj = fp->search_gobj;
}

/* cmd_throw_ground_initial_ / cmd_throw_air_initial_: the catch. */
static void ndsP4PianoDSPThrowInitial(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, PIANO_DSP_THROW_FLAGS);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ndsP4PianoDSPThrowSetup(fighter_gobj);
}

static void ndsP4PianoDSPGroundThrowInitial(GObj *fighter_gobj)
{
    ndsP4PianoDSPThrowInitial(fighter_gobj, PIANO_DSP_GROUND_THROW);
}

static void ndsP4PianoDSPAirThrowInitial(GObj *fighter_gobj)
{
    ndsP4PianoDSPThrowInitial(fighter_gobj, PIANO_DSP_AIR_THROW);
}

/* cmd_throw_hide_ (0xED/0xEE interrupt): the held fighter vanishes once the
 * script sets temp variable 1. */
void ndsP4PianoDSPThrowHide(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp->motion_vars.flags.flag0 != 0) && (fp->catch_gobj != NULL))
    {
        ftGetStruct(fp->catch_gobj)->is_invisible = TRUE;
    }
}

/* cmd_throw_ground_physics_ (0xED physics). */
void ndsP4PianoDSPGroundThrowPhysics(GObj *fighter_gobj)
{
    ftPhysicsApplyClampGroundVelStickRange(ftGetStruct(fighter_gobj), 0,
                                           ndsP4PianoBitsToF32(PIANO_DSP_THROW_ACCEL_BITS),
                                           PIANO_DSP_THROW_SPEED);
    ftPhysicsSetGroundVelTransferAir(fighter_gobj);
}

/* cmd_throw_ground_to_air_ / cmd_throw_air_to_ground_. */
static void ndsP4PianoDSPThrowGroundToAir(GObj *fighter_gobj)
{
    mpCommonSetFighterAir(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, PIANO_DSP_AIR_THROW, fighter_gobj->anim_frame, 1.0F,
                    PIANO_DSP_THROW_FLAGS);
}

static void ndsP4PianoDSPThrowAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, PIANO_DSP_GROUND_THROW, fighter_gobj->anim_frame, 1.0F,
                    PIANO_DSP_THROW_FLAGS);
}

/* cmd_throw_ground_collision_ (0xED map): sliding off the floor. */
void ndsP4PianoDSPGroundThrowMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4PianoDSPThrowGroundToAir);
}

/* cmd_throw_air_collision_ (0xEE map). */
void ndsP4PianoDSPAirThrowMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4PianoDSPThrowAirToGround);
}

/* ---- Patches on his id ---- */

/* PianoSpecial.asm absorb_behaviour_: 1 book per 3% he absorbs, at most 5,
 * spent by the next neutral special. Remix keeps it in the player struct
 * word Mario's code resets at every spawn and stock (on_init here). */
#define PIANO_BONUS_AMMO_MAX 5.0F
#define PIANO_BONUS_AMMO_RATE_BITS 0x3EAAAAABu
#define PIANO_ABSORB_HEAL 0.75F
static f32 sNdsP4PianoBonusAmmo[GMCOMMON_PLAYERS_MAX];

static void ndsP4PianoOnInit(FTStruct *fp)
{
    sNdsP4PianoBonusAmmo[fp->player & 3u] = 0.0F;
}

/* ground/air_absorb_initial_patch_: Ness's absorb routine sends him to his
 * own absorb phase (its turn to face the shot already done). */
static void ndsP4PianoAbsorbHit(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->ga == nMPKineticsGround)
    {
        ndsP4PianoDSPAbsorbInitial(fighter_gobj, PIANO_DSP_GROUND_ABSORB);
    }
    else ndsP4PianoDSPAbsorbInitial(fighter_gobj, PIANO_DSP_AIR_ABSORB);
}

/* absorb_behaviour_: heals 0.75 of the damage (the source twice it) and
 * banks a third of it as books. */
static s32 ndsP4PianoAbsorbHeal(FTStruct *fp, s32 damage)
{
    f32 *ammo = &sNdsP4PianoBonusAmmo[fp->player & 3u];
    f32 f_damage = (f32)damage;

    *ammo += f_damage * ndsP4PianoBitsToF32(PIANO_BONUS_AMMO_RATE_BITS);
    if (*ammo >= PIANO_BONUS_AMMO_MAX)
    {
        *ammo = PIANO_BONUS_AMMO_MAX;
    }
    return (s32)(f_damage * PIANO_ABSORB_HEAL);
}

/* Piano.asm capture_action_fix_ (custom_capture_dk_interrupt): his grab's
 * victim (ThrownDK, Shouldered here) is eaten while he holds it, and never
 * mashes out on its own. */
static sb32 ndsP4PianoShoulderedInterrupt(GObj *victim_gobj)
{
    FTStruct *victim_fp = ftGetStruct(victim_gobj);
    FTStruct *catch_fp = ftGetStruct(victim_fp->capture_gobj);

    if (catch_fp->status_id == nFTCommonStatusCatchWait)
    {
        victim_fp->is_invisible = TRUE;
    }
    return TRUE;
}

/* Piano.asm extra_part_visibility_ (every status change): his tongue
 * (joints 14-16: the donor indexes the joint array at 0x8E8 directly) and
 * held item show only in the grab, the grab pull and the item statuses. */
#define PIANO_TONGUE_JOINT_FIRST 14
#define PIANO_TONGUE_JOINT_LAST 16

void ndsP4PianoOnStatus(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 joint_id;

    if ((status_id == nFTCommonStatusCatch) || (status_id == nFTCommonStatusCatchPull) ||
        ((status_id >= nFTCommonStatusLightGet) && (status_id <= nFTCommonStatusHammerLanding)))
    {
        return;
    }
    fp->is_item_show = FALSE;

    /* The donor writes the part's -1 id, drops its MObjs and clears its
     * display list itself: ftParamSetModelPartID's -1 case, which also tells
     * the native owner the part is hidden. */
    for (joint_id = PIANO_TONGUE_JOINT_FIRST; joint_id <= PIANO_TONGUE_JOINT_LAST; joint_id++)
    {
        if (fp->joints[joint_id] != NULL)
        {
            ftParamSetModelPartID(fighter_gobj, joint_id, -1);
        }
    }
    fp->is_modelpart_modify = TRUE;
}

/* ---- The book (fireball.asm Book, struct_book) ---- */

#define PIANO_BOOK_LIFETIME 120
#define PIANO_BOOK_VEL_TERMINAL 60.0F
#define PIANO_BOOK_VEL_MIN 25.0F
#define PIANO_BOOK_GRAVITY 1.4F
#define PIANO_BOOK_REBOUND 0.55F
#define PIANO_BOOK_ROTATE_SPEED 0.139626F
#define PIANO_BOOK_VEL_BASE 50.0F
#define PIANO_BOOK_ANGLE 1.0472F                /* angle_info: 60 degrees */
#define PIANO_BOOK_SPREAD 0.436332F             /* 25 degrees */
#define PIANO_BOOK_SPREAD_STEP 0.01F            /* 0x3C23D70A */
#define PIANO_BOOK_SPREAD_GROWTH 0.25F
#define PIANO_BOOK_HIT_FGM 0x1F                 /* Book.FGM */
#define PIANO_BOOK_KIRBY_JOINT 7                /* kirby_fix_: part 0x3 */
/* kirby_fix_'s original line: Kirby's fireball hand, part 0xD. */
#define PIANO_BOOK_KIRBY_HAND_JOINT 17

extern void *gNdsP4PianoSpecial1;

/* Makers that found his book file missing: counted, never a fault. */
__attribute__((used)) volatile u32 gNdsP4PianoArticleMisses;

/* The fireball's update with the book's numbers. */
static sb32 ndsP4PianoBookProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

        return TRUE;
    }
    wpMainApplyGravityClampTVel(wp, PIANO_BOOK_GRAVITY, PIANO_BOOK_VEL_TERMINAL);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.x += PIANO_BOOK_ROTATE_SPEED;

    return FALSE;
}

/* The fireball's map routine; a bounce makes small smoke (bounce_effect_)
 * where the fireball grinds fire. */
static sb32 ndsP4PianoBookProcMap(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    Vec3f pos;

    wpMapTestAll(weapon_gobj);

    if (wpMapCheckAllRebound(weapon_gobj, MAP_FLAG_MAIN_MASK, PIANO_BOOK_REBOUND, &pos) != FALSE)
    {
        if (lbCommonMag2D(&wp->physics.vel_air) < PIANO_BOOK_VEL_MIN)
        {
            efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

            return TRUE;
        }
        wpMainVelSetModelPitch(weapon_gobj);

        efManagerDustLightMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, wp->lr, 1.0F);
    }
    return FALSE;
}

/* hit_effect_'s book arm (hurtbox, shield, set-off, absorb): its sound, a
 * puff of dust and the normal hit spark for the damage it dealt. */
static sb32 ndsP4PianoBookProcHit(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    Vec3f *pos = &DObjGetStruct(weapon_gobj)->translate.vec.f;

    func_800269C0_275C0(PIANO_BOOK_HIT_FGM);
    (void)efManagerDamageSpawnMDustMakeEffect(pos, wp->lr);
    (void)efManagerDamageNormalLightMakeEffect(pos, 0, wp->hit_normal_damage, FALSE);

    return TRUE;
}

/* The fireball's reflector routine with the book's lifetime. */
static sb32 ndsP4PianoBookProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);

    wp->lifetime = PIANO_BOOK_LIFETIME;

    wpMainReflectorSetLR(wp, fp);
    wpMainVelSetModelPitch(weapon_gobj);

    return FALSE;
}

static WPDesc sNdsP4PianoBookWeaponDesc = {
    0x00,
    NDS_P4_WP_KIND_PIANO_BOOK,
    &gNdsP4PianoSpecial1,
    0x0,
    { nGCMatrixKindTra, 0x47, 0 },
    ndsP4PianoBookProcUpdate,
    ndsP4PianoBookProcMap,
    ndsP4PianoBookProcHit,
    ndsP4PianoBookProcHit,
    wpMarioFireballProcHop,
    ndsP4PianoBookProcHit,
    ndsP4PianoBookProcReflector,
    ndsP4PianoBookProcHit
};

/* wpMarioFireballMakeWeapon with struct_book: the angle is Book.create_'s. */
static GObj *ndsP4PianoBookMakeWeapon(GObj *fighter_gobj, Vec3f *pos, f32 angle)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *weapon_gobj;
    WPStruct *wp;
    DObj *dobj;

    if (gNdsP4PianoSpecial1 == NULL)
    {
        gNdsP4PianoArticleMisses++;
        return NULL;
    }
    weapon_gobj = wpManagerMakeWeapon(fighter_gobj, &sNdsP4PianoBookWeaponDesc, pos,
                                      WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER);
    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);
    wp->lifetime = PIANO_BOOK_LIFETIME;

    wp->physics.vel_air.z = 0.0F;
    wp->physics.vel_air.x = PIANO_BOOK_VEL_BASE * __cosf(angle) * fp->lr;
    wp->physics.vel_air.y = PIANO_BOOK_VEL_BASE * __sinf(angle);

    dobj = DObjGetStruct(weapon_gobj);

    if (dobj->mobj != NULL)
    {
        dobj->mobj->palette_id = 0.0F;
    }
    wpMainVelSetModelPitch(weapon_gobj);

    return weapon_gobj;
}

/* Fireball.Book.create_ (his fireball row; Kirby's with his power, from
 * kirby_fix_'s joint): a book at a random spread about 60 degrees, and one
 * more per banked book, each spread a quarter wider. */
static GObj *ndsP4PianoFireball(GObj *fighter_gobj, Vec3f *pos)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *last = NULL;
    s32 count = 0;
    s32 i;

    if (fp->fkind == nFTKindKirby)
    {
        /* Kirby's attributes never make joint 7 (setup_parts 0xEF7CFFC0
         * skips DObjDesc 3), so the part Remix names is NULL here and the
         * position walk faulted on it (owner, play-1012y: Kirby with Mad
         * Piano's power, a data abort at his first book). Kirby's own
         * fireball hand stands in. */
        DObj *joint = fp->joints[PIANO_BOOK_KIRBY_JOINT];

        if (joint == NULL)
        {
            joint = fp->joints[PIANO_BOOK_KIRBY_HAND_JOINT];
        }
        pos->x = pos->y = pos->z = 0.0F;
        gmCollisionGetFighterPartsWorldPosition(joint, pos);
    }
    else
    {
        f32 *ammo = &sNdsP4PianoBonusAmmo[fp->player & 3u];

        count = (s32)*ammo;
        if (count != 0)
        {
            *ammo = 0.0F;
        }
    }
    for (i = 0; i <= count; i++)
    {
        f32 spread = (f32)(syUtilsRandIntRange(101) - 50) * PIANO_BOOK_SPREAD_STEP;

        spread *= ((f32)i * PIANO_BOOK_SPREAD_GROWTH) + 1.0F;
        last = ndsP4PianoBookMakeWeapon(fighter_gobj, pos,
                                        (PIANO_BOOK_SPREAD * spread) + PIANO_BOOK_ANGLE);
    }
    return last;
}

/* Reflect.asm maintain_ and apply_reflect_input_ _piano_absorb: his CPU
 * holds and presses his down special as Ness's, over its statuses 0xE5 up to
 * 0xEE (the eat through the grounded command throw). */
const NDSP4Computer gNdsP4PianoComputer = {
    .absorb_first = PIANO_DSP_GROUND_BEGIN,
    .absorb_end = PIANO_DSP_AIR_THROW,
};

const NDSP4Overrides gNdsP4PianoOverrides = {
    .on_init = ndsP4PianoOnInit,
    .absorb_hit = ndsP4PianoAbsorbHit,
    .absorb_heal = ndsP4PianoAbsorbHeal,
    .command_capture_status = nFTCommonStatusShouldered,
    .capture_status = nFTCommonStatusShouldered,
    .shouldered_interrupt = ndsP4PianoShoulderedInterrupt,
    .fireball = ndsP4PianoFireball,
    .kirby_fireball = ndsP4PianoFireball,
    .thrown_hidden = TRUE,
};

#endif /* NDS_P4_PIANO */
