/*
 * P4 Sheik: native ports of the donor's special routines and of the patch
 * Remix makes on her character id. Source: JSsixtyfour/smashremix 5e04fe7,
 * src/Sheik/SheikSpecial.asm and Wario.asm (body_slam_recoil_), read as
 * assembled (scripts/p4/mipsdis.py): the OS.copy_segment blocks are the
 * original game's code. Sheik runs Captain's status code
 * (fp->fkind == nFTKindCaptain); her needle charge runs on Samus's charge
 * routines.
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x00D port                             player
 *   0x044 direction                        lr
 *   0x0CC/0x0D2 collision flags            coll_data.mask_prev (last frame's
 *                                          walls: ftmain moves the current
 *                                          mask there before the map
 *                                          callback), coll_data.mask_stat
 *   0x0EC/0x144 floor line, ignored line   coll_data.floor_line_id,
 *                                          coll_data.ignore_line_id
 *   0x148 jumps used                       jumps_used
 *   0x14C kinetic state                    ga
 *   0x17C/0x180/0x184 temp variables 1-3   motion_vars.flags.flag0/1/2
 *   0x18D bit 0                            is_invisible
 *   0x18D & 0x07                           clears is_absorb, absorb_lr,
 *                                          is_goto_attack100 and is_fastfall
 *   0x1B4/0x1B6/0x1B8 A, B, shield masks   input.button_mask_a/b/z
 *   0x1BE buttons tapped                   input.pl.button_tap (the byte
 *                                          the fish's buffer keeps is its
 *                                          high one)
 *   0x1C2/0x1C3 stick x/y                  input.pl.stick_range.x/y
 *   0x5BB                                  hitstatus's low byte
 *   0x924 part 0xC (hand)                  joints[15]
 *   0x9EC on hit routine                   proc_damage
 *   0xADC/0xAE0                            passive_vars, words 0-1: the
 *                                          fish's lift spent (Captain's
 *                                          falcon_punch_unk, which his
 *                                          landing case clears, and Samus's
 *                                          charge level, which her charge
 *                                          damage routine clears) and the
 *                                          needle charge
 *   0xB18-0xB20                            status_vars, words 0-2: the
 *                                          vanish's timer and angle (word
 *                                          2); the needles' shoot choice,
 *                                          charge timer and Samus's charge
 *                                          shot (word 2, always NULL here)
 *   attributes + 0x4C/0x50/0x58/0x5C/0x64  air_accel, air_speed_max_x,
 *                                          gravity, tvel_base, jumps_max
 *
 * The needle is her own weapon on her special file 1 (S6, below). S4
 * owns the vanish's per-port environment colour (CharEnvColor.asm), and the
 * gfx-routine table owns the full charge's flash (GFXRoutine.id.SHEIK_CHARGE,
 * also re-run at full charge by samusshared.asm kirby_power_check_flash_):
 * the DS colanim table declines Remix's ids. Kirby's copy of the needles is
 * S8's.
 */
#include <nds/nds_p4.h>

#if NDS_P4_SHEIK

#if !NDS_P2_SAMUS
#error "P4 Sheik runs Samus's charge routines (NDS_P2_SAMUS)"
#endif

#include <ef/effect.h>
#include <macros.h>
#include <sys/audio.h>
#include <sys/obj.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

void ftSamusSpecialNProcDamage(GObj *fighter_gobj);
void ftSamusSpecialNStartInitStatusVars(FTStruct *fp);
sb32 ftSamusSpecialHiProcPass(GObj *fighter_gobj);
s32 ndsBaseFTCommonEscapeGetStatus(FTStruct *fp);
void ndsBaseFTCommonEscapeSetStatus(GObj *fighter_gobj, s32 status_id,
                                    s32 itemthrow_buffer_tics);
f32 syUtilsArcTan2(f32 y, f32 x);
f32 __sinf(f32);
f32 __cosf(f32);

/* Sheik.Action (her action array; 0xEF on are add_new_action's). */
#define SHEIK_STATUS_USPG_BEGIN 0xE4
#define SHEIK_STATUS_USPG_MOVE 0xE5
#define SHEIK_STATUS_USPG_END 0xE6
#define SHEIK_STATUS_USPA_BEGIN 0xE7
#define SHEIK_STATUS_USPA_MOVE 0xE8
#define SHEIK_STATUS_USPA_END 0xE9
#define SHEIK_STATUS_NSPG_BEGIN 0xEA
#define SHEIK_STATUS_NSPG_CHARGE 0xEB
#define SHEIK_STATUS_NSPG_SHOOT 0xEC
#define SHEIK_STATUS_NSPA_BEGIN 0xED
#define SHEIK_STATUS_NSPA_CHARGE 0xEE
#define SHEIK_STATUS_NSPA_SHOOT 0xEF
#define SHEIK_STATUS_DSP_BEGIN 0xF0
#define SHEIK_STATUS_DSP_ATTACK 0xF1
#define SHEIK_STATUS_DSP_LANDING 0xF2
#define SHEIK_STATUS_DSP_RECOIL 0xF3

/* Joypad.A and Joypad.B in button_tap. */
#define SHEIK_BUTTON_A 0x8000u
#define SHEIK_BUTTON_B 0x4000u

/* SheikUSP (the vanish). */
#define SHEIK_USP_DEFAULT_ANGLE 0x3FC90FDBu /* pi / 2 */
#define SHEIK_USP_LANDING_FSM 0x3EBD3000u   /* "0.37" */
#define SHEIK_USP_INITIAL_SPEED 75.0F
#define SHEIK_USP_SPEED 280.0F
#define SHEIK_USP_TIMER 12
#define SHEIK_USP_MOVE_TIMER 6
#define SHEIK_USP_STICK_MIN 11
#define SHEIK_USP_END_GROUND_MUL 0.25F
#define SHEIK_USP_END_AIR_MUL 0.109375F     /* 0x3DE00000, "0.109" */
#define SHEIK_USP_FALL_DRIFT 0.9375F
#define SHEIK_USP_FALL_LANDING 0.5F

/* SheikNSP (the needles). */
#define SHEIK_NSP_CHARGE_MAX 6
#define SHEIK_NSP_CHARGE_INT 18
#define SHEIK_NSP_HAND_JOINT 15
#define SHEIK_NSP_FGM_THROW 0x410           /* Sheik.FGM.NSP_THROW */
#define SHEIK_NSP_COLANIM_CHARGE 0x6D       /* GFXRoutine.id.SHEIK_CHARGE */
/* "argument 4": colanim; the charge's transitions add the loop sound. */
#define SHEIK_PRESERVE_NSP 0x0002u
#define SHEIK_PRESERVE_NSP_CHARGE 0x0802u

/* SheikDSP (Bouncing Fish). */
#define SHEIK_DSP_Y_SPEED 50.0F
#define SHEIK_DSP_RECOIL_Y_SPEED 65.0F
#define SHEIK_DSP_GRAVITY_MUL 0x3E6B851Fu   /* 0.23 */
#define SHEIK_DSP_STICK_MIN 8
#define SHEIK_DSP_SPEED 40.0F               /* HORIZONTAL_MAX */
#define SHEIK_DSP_MIN_SPEED 20.0F
#define SHEIK_DSP_RECOIL_SPEED 30.0F
#define SHEIK_DSP_ATTACK_FRAME 10.0F
#define SHEIK_DSP_RECOIL_FRAME_MIN 15.0F
#define SHEIK_DSP_RECOIL_FRAME_MAX 40.0F
#define SHEIK_DSP_TAP_B 0x40u               /* B_PRESSED, the taps' high byte */
#define SHEIK_PRESERVE_DSP_ATTACK 0x0003u   /* hit, colanim */
/* check_recoil_'s status change takes its argument 4 from the stack slot
 * where it saved a1: attack_collision_'s _end label, 0x80566ACC (effect,
 * fast fall, slope contour, texture part, throw pointer, loop sound,
 * after-image, rumble). */
#define SHEIK_PRESERVE_DSP_RECOIL 0x80566ACCu

_Static_assert((MAP_FLAG_LWALL | MAP_FLAG_RWALL) == 0x21, "SheikDSP WALL_COLLISION");

static f32 ndsP4SheikBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

static s32 *ndsP4SheikStatusS32(FTStruct *fp, s32 word)
{
    return &((s32 *)(void *)&fp->status_vars)[word];
}

static f32 *ndsP4SheikStatusF32(FTStruct *fp, s32 word)
{
    return &((f32 *)(void *)&fp->status_vars)[word];
}

static s32 *ndsP4SheikPassiveS32(FTStruct *fp, s32 word)
{
    return &((s32 *)(void *)&fp->passive_vars)[word];
}

static void ndsP4SheikClearFlags(FTStruct *fp)
{
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

static void ndsP4SheikClearFastFall(FTStruct *fp)
{
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
}

/* ---- Up special (the vanish) ---- */

static void ndsP4SheikUSPMoveInitial(GObj *fighter_gobj);

/* SheikUSP.ground_begin_initial_ (ground_usp). */
void ndsP4SheikUSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_USPG_BEGIN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ndsP4SheikClearFlags(fp);
    fp->physics.vel_ground.x *= 0.5F;
}

/* SheikUSP.air_begin_initial_ (air_usp). */
void ndsP4SheikUSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_USPA_BEGIN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ndsP4SheikClearFlags(fp);
    fp->physics.vel_air.x *= 0.5F;
    fp->physics.vel_air.y = SHEIK_USP_INITIAL_SPEED;
    ndsP4SheikClearFastFall(fp);
}

/* SheikUSP.begin_main_ (0xE4/0xE7 update). */
void ndsP4SheikUSPBeginMain(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4SheikUSPMoveInitial);
}

/* SheikUSP.ground_begin_transition_. */
static void ndsP4SheikUSPBeginAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_USPG_BEGIN, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
}

/* SheikUSP.air_begin_transition_. */
static void ndsP4SheikUSPBeginGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_USPA_BEGIN, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
    ftPhysicsClampAirVelXMax(fp);
}

/* SheikUSP.ground_begin_collision_ (0xE4 map). */
void ndsP4SheikUSPBeginGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4SheikUSPBeginGroundToAir);
}

/* SheikUSP.air_begin_collision_ (0xE7 map). */
void ndsP4SheikUSPBeginAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterCliff(fighter_gobj, ndsP4SheikUSPBeginAirToGround);
}

/* The move statuses' vanish: invisible and intangible (the donor writes the
 * low byte of the hit status). */
static void ndsP4SheikUSPVanish(FTStruct *fp)
{
    fp->is_invisible = TRUE;
    fp->hitstatus = (fp->hitstatus & ~0xFF) | nGMHitStatusIntangible;
}

/* SheikUSP.apply_movement_: once the timer is below 6, 280 along the angle
 * (status word 2). */
static void ndsP4SheikUSPApplyMovement(FTStruct *fp)
{
    f32 angle;
    f32 vel_x;
    f32 vel_y;

    if (*ndsP4SheikStatusS32(fp, 0) >= SHEIK_USP_MOVE_TIMER)
    {
        return;
    }
    angle = *ndsP4SheikStatusF32(fp, 2);
    vel_x = SHEIK_USP_SPEED * __cosf(angle);
    vel_y = SHEIK_USP_SPEED * __sinf(angle);

    if (fp->ga == nMPKineticsGround)
    {
        fp->physics.vel_ground.x = vel_x;
        fp->physics.vel_air.x = vel_x * fp->lr;
    }
    else
    {
        fp->physics.vel_air.x = vel_x * fp->lr;
        fp->physics.vel_air.y = vel_y;
    }
}

/* SheikUSP.move_initial_ (the begin's end of motion): turn to a held stick,
 * aim along it (straight up under 11 of tilt), vanish for 12 frames and
 * spend the mid-air jumps. A level stick on the ground keeps her grounded;
 * a grounded start may drop through the platform under her. */
static void ndsP4SheikUSPMoveInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 stick_x = fp->input.pl.stick_range.x;
    s32 stick_y = fp->input.pl.stick_range.y;
    s32 ga;
    sb32 is_default_angle;
    s32 status_id;

    if (ABS(stick_x) >= SHEIK_USP_STICK_MIN)
    {
        ftParamSetStickLR(fp);
    }
    ga = fp->ga;
    /* round(sqrt(x * x + y * y)) < 11, cvt.w.s rounding to nearest. */
    is_default_angle = (((stick_x * stick_x) + (stick_y * stick_y)) <= 110) ? TRUE : FALSE;

    status_id = SHEIK_STATUS_USPA_MOVE;
    if (is_default_angle != FALSE)
    {
        mpCommonSetFighterAir(fp);
    }
    else if (ga == nMPKineticsGround)
    {
        if ((stick_y == 0) && (stick_x != 0))
        {
            status_id = SHEIK_STATUS_USPG_MOVE;
        }
        else mpCommonSetFighterAir(fp);
    }
    fp->physics.vel_ground.x = 0.0F;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 0.0F;
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);

    fp->jumps_used = fp->attr->jumps_max;
    *ndsP4SheikStatusF32(fp, 2) = (is_default_angle != FALSE)
        ? ndsP4SheikBitsToF32(SHEIK_USP_DEFAULT_ANGLE)
        : syUtilsArcTan2((f32)stick_y, (f32)(stick_x * fp->lr));

    *ndsP4SheikStatusS32(fp, 0) = SHEIK_USP_TIMER;
    ndsP4SheikUSPApplyMovement(fp);
    ndsP4SheikUSPVanish(fp);

    if (ga == nMPKineticsGround)
    {
        fp->coll_data.ignore_line_id = fp->coll_data.floor_line_id;
    }
}

/* SheikUSP.ground_end_initial_. */
static void ndsP4SheikUSPGroundEndInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_USPG_END, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->physics.vel_ground.x *= SHEIK_USP_END_GROUND_MUL;
}

/* SheikUSP.air_end_initial_. */
static void ndsP4SheikUSPAirEndInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_USPA_END, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->physics.vel_air.x *= SHEIK_USP_END_AIR_MUL;
    fp->physics.vel_air.y *= SHEIK_USP_END_AIR_MUL;
    fp->motion_vars.flags.flag1 = 0;
}

/* SheikUSP.move_main_ (0xE5/0xE8 update): the end when the timer runs out. */
void ndsP4SheikUSPMoveMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *timer = ndsP4SheikStatusS32(fp, 0);

    *timer -= 1;
    if (*timer != 0)
    {
        return;
    }
    if (fp->ga == nMPKineticsGround)
    {
        ndsP4SheikUSPGroundEndInitial(fighter_gobj);
    }
    else ndsP4SheikUSPAirEndInitial(fighter_gobj);
}

/* SheikUSP.move_physics_ (0xE5/0xE8 physics). */
void ndsP4SheikUSPMovePhysics(GObj *fighter_gobj)
{
    ndsP4SheikUSPApplyMovement(ftGetStruct(fighter_gobj));
}

/* SheikUSP.ground_move_transition_: on with the same angle and timer. */
static void ndsP4SheikUSPMoveAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_USPG_MOVE, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
    ndsP4SheikUSPVanish(fp);
}

/* SheikUSP.air_move_transition_: off an edge she goes on level. */
static void ndsP4SheikUSPMoveGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_USPA_MOVE, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
    ftPhysicsClampAirVelXMax(fp);
    *ndsP4SheikStatusF32(fp, 2) = 0.0F;
    ndsP4SheikUSPVanish(fp);
}

/* SheikUSP.ground_move_collision_ (0xE5 map). */
void ndsP4SheikUSPMoveGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4SheikUSPMoveGroundToAir);
}

/* SheikUSP.air_move_collision_ (0xE8 map). */
void ndsP4SheikUSPMoveAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterCliff(fighter_gobj, ndsP4SheikUSPMoveAirToGround);
}

/* SheikUSP.end_invisibility_: hidden while the script holds temp
 * variable 3. */
static void ndsP4SheikUSPEndInvisibility(FTStruct *fp)
{
    fp->is_invisible = (fp->motion_vars.flags.flag2 != 0) ? TRUE : FALSE;
}

/* SheikUSP.air_end_main_ (0xE9 update). */
void ndsP4SheikUSPAirEndMain(GObj *fighter_gobj)
{
    ndsP4SheikUSPEndInvisibility(ftGetStruct(fighter_gobj));

    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, SHEIK_USP_FALL_DRIFT, FALSE, TRUE, FALSE,
                                     SHEIK_USP_FALL_LANDING, TRUE);
    }
}

/* SheikUSP.ground_end_main_ (0xE6 update). */
void ndsP4SheikUSPGroundEndMain(GObj *fighter_gobj)
{
    ndsP4SheikUSPEndInvisibility(ftGetStruct(fighter_gobj));
    ftAnimEndSetWait(fighter_gobj);
}

/* SheikUSP.end_collision_ (0xE6/0xE9 map): Samus's Screw Attack map with
 * her landing lag, no interrupt, and the cliff-edge stop on the ground. */
void ndsP4SheikUSPEndMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsAir)
    {
        if (fp->physics.vel_air.y >= 0.0F)
        {
            mpCommonCheckFighterProject(fighter_gobj);
        }
        else if (mpCommonCheckFighterPassCliff(fighter_gobj, ftSamusSpecialHiProcPass) != FALSE)
        {
            if (fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK)
            {
                ftCommonCliffCatchSetStatus(fighter_gobj);
            }
            else ftCommonLandingFallSpecialSetStatus(fighter_gobj, FALSE,
                                                     ndsP4SheikBitsToF32(SHEIK_USP_LANDING_FSM));
        }
    }
    else mpCommonProcFighterOnCliffEdge(fighter_gobj);
}

/* SheikUSP.end_physics_ (0xE9 physics): drift once the script sets temp
 * variable 2. */
void ndsP4SheikUSPEndPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    if (fp->motion_vars.flags.flag1 != 0)
    {
        ftPhysicsClampAirVelXStickRange(fp, 8, attr->air_accel, attr->air_speed_max_x);
    }
    ftPhysicsApplyGravityDefault(fp, attr);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

/* ---- Neutral special (the needles) ---- */

static s32 *ndsP4SheikCharge(FTStruct *fp)
{
    return ndsP4SheikPassiveS32(fp, 1);
}

/* SheikNSP.begin_initial_: a full charge shoots when the begin ends, any
 * other charges. */
static void ndsP4SheikNSPInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftSamusSpecialNStartInitStatusVars(fp);
    *ndsP4SheikStatusS32(fp, 0) = (*ndsP4SheikCharge(fp) == SHEIK_NSP_CHARGE_MAX) ? TRUE : FALSE;
}

/* SheikNSP.ground_begin_initial_ (ground_nsp). */
void ndsP4SheikNSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4SheikNSPInitial(fighter_gobj, SHEIK_STATUS_NSPG_BEGIN);
}

/* SheikNSP.air_begin_initial_ (air_nsp). */
void ndsP4SheikNSPAirInitial(GObj *fighter_gobj)
{
    ndsP4SheikNSPInitial(fighter_gobj, SHEIK_STATUS_NSPA_BEGIN);
}

/* SheikNSP.ground_charge_initial_ / air_charge_initial_. */
static void ndsP4SheikNSPChargeInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    *ndsP4SheikStatusS32(fp, 1) = SHEIK_NSP_CHARGE_INT;
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, SHEIK_PRESERVE_NSP);
}

/* SheikNSP.ground_shoot_initial_: at least one needle. */
static void ndsP4SheikNSPGroundShootInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *charge = ndsP4SheikCharge(fp);

    fp->motion_vars.flags.flag0 = 0;
    if (*charge == 0)
    {
        *charge = 1;
    }
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_NSPG_SHOOT, 0.0F, 1.0F, SHEIK_PRESERVE_NSP);
    fp->proc_damage = ftSamusSpecialNProcDamage;
}

/* SheikNSP.air_shoot_initial_: Samus's aerial end's first lines, then as
 * the ground's. */
static void ndsP4SheikNSPAirShootInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *charge = ndsP4SheikCharge(fp);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
        ftPhysicsClampAirVelXMax(fp);
    }
    fp->motion_vars.flags.flag0 = 0;
    if (*charge == 0)
    {
        *charge = 1;
    }
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_NSPA_SHOOT, 0.0F, 1.0F, SHEIK_PRESERVE_NSP);
    fp->proc_damage = ftSamusSpecialNProcDamage;
}

/* SheikNSP.begin_main_ (0xEA/0xED update). */
void ndsP4SheikNSPBeginMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 is_shoot;

    if (!(fighter_gobj->anim_frame <= 0.0F))
    {
        return;
    }
    is_shoot = *ndsP4SheikStatusS32(fp, 0);

    if (fp->ga == nMPKineticsGround)
    {
        if (is_shoot != FALSE)
        {
            ndsP4SheikNSPGroundShootInitial(fighter_gobj);
        }
        else ndsP4SheikNSPChargeInitial(fighter_gobj, SHEIK_STATUS_NSPG_CHARGE);
    }
    else if (is_shoot != FALSE)
    {
        ndsP4SheikNSPAirShootInitial(fighter_gobj);
    }
    else ndsP4SheikNSPChargeInitial(fighter_gobj, SHEIK_STATUS_NSPA_CHARGE);
}

/* The begin's interrupts: B or A asks for the shot; the shield button
 * returns the begin. */
static sb32 ndsP4SheikNSPBeginInterrupt(FTStruct *fp)
{
    u16 tap = fp->input.pl.button_tap;

    if ((tap & fp->input.button_mask_b) || (tap & fp->input.button_mask_a))
    {
        *ndsP4SheikStatusS32(fp, 0) = TRUE;
    }
    return (fp->input.pl.button_tap & fp->input.button_mask_z) ? TRUE : FALSE;
}

/* SheikNSP.ground_begin_interrupt_ (0xEA interrupt). */
void ndsP4SheikNSPBeginGroundInterrupt(GObj *fighter_gobj)
{
    if (ndsP4SheikNSPBeginInterrupt(ftGetStruct(fighter_gobj)) != FALSE)
    {
        ftCommonWaitSetStatus(fighter_gobj);
    }
}

/* SheikNSP.air_begin_interrupt_ (0xED interrupt). */
void ndsP4SheikNSPBeginAirInterrupt(GObj *fighter_gobj)
{
    if (ndsP4SheikNSPBeginInterrupt(ftGetStruct(fighter_gobj)) != FALSE)
    {
        ftCommonFallSetStatus(fighter_gobj);
    }
}

/* SheikNSP.air_begin_transition_. */
static void ndsP4SheikNSPBeginGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftPhysicsClampAirVelXMax(fp);
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_NSPA_BEGIN, fighter_gobj->anim_frame, 1.0F,
                    SHEIK_PRESERVE_NSP);
    fp->proc_damage = ftSamusSpecialNProcDamage;
}

/* SheikNSP.ground_begin_transition_. */
static void ndsP4SheikNSPBeginAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_NSPG_BEGIN, fighter_gobj->anim_frame, 1.0F,
                    SHEIK_PRESERVE_NSP);
    fp->proc_damage = ftSamusSpecialNProcDamage;
}

/* SheikNSP.ground_begin_collision_ (0xEA map). */
void ndsP4SheikNSPBeginGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4SheikNSPBeginGroundToAir);
}

/* SheikNSP.air_begin_collision_ (0xED map). */
void ndsP4SheikNSPBeginAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SheikNSPBeginAirToGround);
}

/* SheikNSP.charge_main_ (0xEB/0xEE update): a needle every 18 frames up
 * to 6; the sixth flashes and stands her up. The donor then gives Samus's
 * charge shot (status word 2) the level; hers is always NULL. */
void ndsP4SheikNSPChargeMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *timer = ndsP4SheikStatusS32(fp, 1);
    s32 *charge = ndsP4SheikCharge(fp);

    *timer -= 1;
    if (*timer != 0)
    {
        return;
    }
    *timer = SHEIK_NSP_CHARGE_INT;
    if (*charge >= SHEIK_NSP_CHARGE_MAX)
    {
        return;
    }
    *charge += 1;
    if (*charge == SHEIK_NSP_CHARGE_MAX)
    {
        ftParamCheckSetFighterColAnimID(fighter_gobj, SHEIK_NSP_COLANIM_CHARGE, 0);
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* SheikNSP.ground_charge_interrupt_ (0xEB interrupt): Samus's charge
 * interrupt without the charge shot. As assembled the roll's item-throw
 * buffer is a2, still the fighter object: a throw stays open the whole
 * roll. */
void ndsP4SheikNSPChargeGroundInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id;

    if ((fp->input.pl.button_tap & fp->input.button_mask_b) ||
        (fp->input.pl.button_tap & fp->input.button_mask_a))
    {
        ndsP4SheikNSPGroundShootInitial(fighter_gobj);
        return;
    }
    status_id = ndsBaseFTCommonEscapeGetStatus(fp);

    if (status_id != -1)
    {
        ndsBaseFTCommonEscapeSetStatus(fighter_gobj, status_id, (s32)(uintptr_t)fighter_gobj);
    }
    else if (fp->input.pl.button_tap & fp->input.button_mask_z)
    {
        ftCommonWaitSetStatus(fighter_gobj);
    }
}

/* SheikNSP.air_charge_interrupt_ (0xEE interrupt): the raw A and B bits. */
void ndsP4SheikNSPChargeAirInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->input.pl.button_tap & (SHEIK_BUTTON_B | SHEIK_BUTTON_A))
    {
        ndsP4SheikNSPAirShootInitial(fighter_gobj);
    }
    else if (fp->input.pl.button_tap & fp->input.button_mask_z)
    {
        ftCommonFallSetStatus(fighter_gobj);
    }
}

/* SheikNSP.ground_charge_transition_. */
static void ndsP4SheikNSPChargeAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_NSPG_CHARGE, fighter_gobj->anim_frame, 1.0F,
                    SHEIK_PRESERVE_NSP_CHARGE);
    fp->proc_damage = ftSamusSpecialNProcDamage;
}

/* SheikNSP.air_charge_transition_. */
static void ndsP4SheikNSPChargeGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftPhysicsClampAirVelXMax(fp);
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_NSPA_CHARGE, fighter_gobj->anim_frame, 1.0F,
                    SHEIK_PRESERVE_NSP_CHARGE);
    fp->proc_damage = ftSamusSpecialNProcDamage;
}

/* SheikNSP.ground_charge_collision_ (0xEB map). */
void ndsP4SheikNSPChargeGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4SheikNSPChargeGroundToAir);
}

/* SheikNSP.air_charge_collision_ (0xEE map). */
void ndsP4SheikNSPChargeAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SheikNSPChargeAirToGround);
}

/* ---- The needle (needle_projectile_struct) ----
 *
 * Her own weapon on her special file 1 (Sheik file 6, generated as
 * gNdsP4SheikSpecial1): WPAttributes at 0, Ray Gun ammo's map routine,
 * Master Hand's bullet bounce off shields, Samus's bomb reflector and
 * Remix's update and destruction below. Remix's projectile id is a Remix
 * one; the DS renderer keys native weapon owners on the kind, so it takes a
 * P4 kind. */
#define SHEIK_NEEDLE_LIFETIME 18                /* NEEDLE_DURATION */
#define SHEIK_NEEDLE_SPEED 225.0F
#define SHEIK_NEEDLE_SPEED_MAX 250.0F
#define SHEIK_NEEDLE_GRAVITY 0.0F
#define SHEIK_NEEDLE_ANGLE_AIR -0.785398F       /* float32 -0.785398 */
#define SHEIK_NEEDLE_ROLL_AIR 0.78539753F       /* 0x3F490FD8 */
#define SHEIK_NEEDLE_YAW_AIR -1.5707964F        /* 0xBFC90FDB, a word of item data */

extern void *gNdsP4SheikSpecial1;

/* Makers that found her needle file missing: counted, never a fault. */
__attribute__((used)) volatile u32 gNdsP4SheikArticleMisses;

LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
sb32 itLGunWeaponAmmoProcMap(GObj *weapon_gobj);
sb32 wpSamusBombProcReflector(GObj *weapon_gobj);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);

/* needle_destruction_ (hit, shield, clang, absorb): smoke, destroyed. */
static sb32 ndsP4SheikNeedleProcDestroy(GObj *weapon_gobj)
{
    (void)efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

    return TRUE;
}

/* needle_main_ (update): smoke at the end of its 18 frames; until then no
 * gravity, its speed capped at 250. */
static sb32 ndsP4SheikNeedleProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        (void)efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

        return TRUE;
    }
    wpMainApplyGravityClampTVel(wp, SHEIK_NEEDLE_GRAVITY, SHEIK_NEEDLE_SPEED_MAX);

    return FALSE;
}

/* wpBossBulletProcHop (shield bounce), which Remix's desc names. */
static sb32 ndsP4SheikNeedleProcHop(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    syVectorRotateAbout3D(&wp->physics.vel_air, &wp->shield_collide_dir,
                          wp->shield_collide_angle * 2);
    wpMainReflectorRotateWeaponModel(weapon_gobj);

    return FALSE;
}

static WPDesc sNdsP4SheikNeedleWeaponDesc = {
    0x00,
    NDS_P4_WP_KIND_SHEIK_NEEDLE,
    &gNdsP4SheikSpecial1,
    0x0,
    { nGCMatrixKindTraRotRpyRSca, 0x47, 0 },    /* 0x12470000 */
    ndsP4SheikNeedleProcUpdate,
    itLGunWeaponAmmoProcMap,
    ndsP4SheikNeedleProcDestroy,
    ndsP4SheikNeedleProcDestroy,
    ndsP4SheikNeedleProcHop,
    ndsP4SheikNeedleProcDestroy,
    wpSamusBombProcReflector,
    ndsP4SheikNeedleProcDestroy
};

/* SheikNSP.needle_stage_setting_: the throw's sound, then a needle level
 * from the ground, 45 degrees down from the air, at 225 a frame. Its aerial
 * tilt is Remix's test of what sinf leaves in v0, the angle's bits (zero
 * only for the ground's 0): the needle rolls 45 degrees, its yaw then set
 * from its heading as on the ground. */
static void ndsP4SheikNSPMakeNeedle(GObj *fighter_gobj, Vec3f *pos)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *weapon_gobj;
    WPStruct *wp;
    DObj *dobj;
    f32 angle;

    func_800269C0_275C0(SHEIK_NSP_FGM_THROW);

    if (gNdsP4SheikSpecial1 == NULL)
    {
        gNdsP4SheikArticleMisses++;
        return;
    }
    weapon_gobj = wpManagerMakeWeapon(fighter_gobj, &sNdsP4SheikNeedleWeaponDesc, pos,
                                      WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER);
    if (weapon_gobj == NULL)
    {
        return;
    }
    wp = wpGetStruct(weapon_gobj);
    wp->lifetime = SHEIK_NEEDLE_LIFETIME;

    angle = (fp->ga == nMPKineticsAir) ? SHEIK_NEEDLE_ANGLE_AIR : 0.0F;

    wp->physics.vel_air.z = 0.0F;
    wp->physics.vel_air.x = __cosf(angle) * SHEIK_NEEDLE_SPEED * fp->lr;
    wp->physics.vel_air.y = __sinf(angle) * SHEIK_NEEDLE_SPEED;

    dobj = DObjGetStruct(weapon_gobj);

    if (angle != 0.0F)
    {
        dobj->rotate.vec.f.y = SHEIK_NEEDLE_YAW_AIR;
        dobj->rotate.vec.f.x = SHEIK_NEEDLE_ROLL_AIR;
    }
    if (dobj->mobj != NULL)
    {
        dobj->mobj->palette_id = 0.0F;
    }
    wpMainVelSetModelPitch(weapon_gobj);
}

/* SheikNSP.shoot_main_ (0xEC/0xEF update): temp variable 1 throws a needle
 * from her hand while any are charged. */
void ndsP4SheikNSPShootMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag0 != 0)
    {
        s32 *charge = ndsP4SheikCharge(fp);

        fp->motion_vars.flags.flag0 = 0;

        if (*charge != 0)
        {
            Vec3f pos;

            *charge -= 1;
            pos.x = pos.y = pos.z = 0.0F;
            gmCollisionGetFighterPartsWorldPosition(fp->joints[SHEIK_NSP_HAND_JOINT], &pos);
            pos.z = 0.0F;
            ndsP4SheikNSPMakeNeedle(fighter_gobj, &pos);
        }
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        fp->motion_vars.flags.flag0 = 0;
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* SheikNSP.air_shoot_transition_ (Samus's first lines: no clamp). */
static void ndsP4SheikNSPShootGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_NSPA_SHOOT, fighter_gobj->anim_frame, 1.0F,
                    SHEIK_PRESERVE_NSP);
    fp->proc_damage = ftSamusSpecialNProcDamage;
}

/* SheikNSP.ground_shoot_collision_ (0xEC map). */
void ndsP4SheikNSPShootGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4SheikNSPShootGroundToAir);
}

/* ---- Down special (Bouncing Fish) ---- */

/* SheikDSP.button_press_buffer: each port's taps (high byte) from the last
 * frame the fish's main or recoil ran. */
static u8 sNdsP4SheikDSPTaps[GMCOMMON_PLAYERS_MAX];

static u32 ndsP4SheikDSPBufferTaps(FTStruct *fp)
{
    u8 taps = (u8)(fp->input.pl.button_tap >> 8);
    u8 last = sNdsP4SheikDSPTaps[fp->player];

    sNdsP4SheikDSPTaps[fp->player] = taps;
    return taps | last;
}

/* SheikDSP.dsp_on_hit_: a hit gives the lift back. */
static void ndsP4SheikDSPProcDamage(GObj *fighter_gobj)
{
    *ndsP4SheikPassiveS32(ftGetStruct(fighter_gobj), 0) = 0;
}

/* SheikDSP.initial_ (ground_dsp, air_dsp): airborne, lifted once until she
 * lands or is hit; the donor plays no events here. */
void ndsP4SheikDSPInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *lift_spent = ndsP4SheikPassiveS32(fp, 0);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
    }
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_DSP_BEGIN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);

    if (*lift_spent == 0)
    {
        fp->physics.vel_air.y = SHEIK_DSP_Y_SPEED;
    }
    *lift_spent = 1;
    fp->proc_damage = ndsP4SheikDSPProcDamage;
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 1;
    ndsP4SheikClearFastFall(fp);
}

/* SheikDSP.attack_transition. */
static void ndsP4SheikDSPAttackSetStatus(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_DSP_ATTACK, 0.0F, 1.0F, SHEIK_PRESERVE_DSP_ATTACK);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* SheikDSP.main_ (0xF0 update): the attack at the end of the motion, or
 * past frame 10 on a B tap this frame or last. After the end-of-motion
 * transition the donor reads the frame through whatever register the new
 * status left; the attack it would start again is the one just set. */
void ndsP4SheikDSPMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 taps = ndsP4SheikDSPBufferTaps(fp);

    if (ftAnimEndCheckSetStatus(fighter_gobj, ndsP4SheikDSPAttackSetStatus) != FALSE)
    {
        return;
    }
    if (!(fighter_gobj->anim_frame <= SHEIK_DSP_ATTACK_FRAME) && (taps & SHEIK_DSP_TAP_B))
    {
        ndsP4SheikDSPAttackSetStatus(fighter_gobj);
    }
}

/* The fish's fall: a quarter-strength gravity until the script sets temp
 * variable 2. */
static void ndsP4SheikDSPApplyGravity(FTStruct *fp, FTAttributes *attr)
{
    f32 gravity = attr->gravity;

    if (fp->motion_vars.flags.flag1 == 0)
    {
        gravity = attr->gravity * ndsP4SheikBitsToF32(SHEIK_DSP_GRAVITY_MUL);
    }
    ftPhysicsApplyGravityClampTVel(fp, gravity, attr->tvel_base);
}

/* SheikDSP.physics_ (0xF0/0xF1 physics): forward at 40 with no stick, else
 * the stick steers within 20-40 forward. */
void ndsP4SheikDSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    s32 stick_x;
    f32 vel_x;

    ndsP4SheikDSPApplyGravity(fp, attr);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) != FALSE)
    {
        return;
    }
    stick_x = fp->input.pl.stick_range.x;

    if (ABS(stick_x) < SHEIK_DSP_STICK_MIN)
    {
        fp->physics.vel_air.x = (fp->lr >= 0) ? SHEIK_DSP_SPEED : -SHEIK_DSP_SPEED;
        return;
    }
    vel_x = fp->physics.vel_air.x + (f32)stick_x;
    fp->physics.vel_air.x = vel_x;

    if (fp->lr >= 0)
    {
        if (SHEIK_DSP_SPEED < vel_x)
        {
            fp->physics.vel_air.x = SHEIK_DSP_SPEED;
        }
        else if (vel_x < SHEIK_DSP_MIN_SPEED)
        {
            fp->physics.vel_air.x = SHEIK_DSP_MIN_SPEED;
        }
    }
    else if (vel_x < -SHEIK_DSP_SPEED)
    {
        fp->physics.vel_air.x = -SHEIK_DSP_SPEED;
    }
    else if (-SHEIK_DSP_MIN_SPEED < vel_x)
    {
        fp->physics.vel_air.x = -SHEIK_DSP_MIN_SPEED;
    }
}

/* SheikDSP.recoil_physics_ (0xF3 physics): back at 30 with no stick. As
 * assembled the stick's clamps are inverted: facing right, anything
 * faster than -40 becomes -40 and the rest -20 (mirrored facing left); the
 * deceleration clamp's exit is commented out in the donor. */
void ndsP4SheikDSPRecoilPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    s32 stick_x;
    f32 vel_x;

    ndsP4SheikDSPApplyGravity(fp, attr);
    ftPhysicsCheckClampAirVelXDecMax(fp, attr);
    stick_x = fp->input.pl.stick_range.x;

    if (ABS(stick_x) < SHEIK_DSP_STICK_MIN)
    {
        fp->physics.vel_air.x = (fp->lr >= 0) ? -SHEIK_DSP_RECOIL_SPEED : SHEIK_DSP_RECOIL_SPEED;
        return;
    }
    vel_x = fp->physics.vel_air.x + -(f32)stick_x;
    fp->physics.vel_air.x = vel_x;

    if (fp->lr >= 0)
    {
        if (-SHEIK_DSP_SPEED < vel_x)
        {
            fp->physics.vel_air.x = -SHEIK_DSP_SPEED;
        }
        else if (vel_x < -SHEIK_DSP_MIN_SPEED)
        {
            fp->physics.vel_air.x = -SHEIK_DSP_MIN_SPEED;
        }
    }
    else if (vel_x < SHEIK_DSP_SPEED)
    {
        fp->physics.vel_air.x = SHEIK_DSP_SPEED;
    }
    else if (SHEIK_DSP_MIN_SPEED < vel_x)
    {
        fp->physics.vel_air.x = SHEIK_DSP_MIN_SPEED;
    }
}

/* SheikDSP.air_to_ground_: its status change's argument 4 is a stack word
 * the donor never writes; the DS preserves nothing. */
static void ndsP4SheikDSPAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, SHEIK_STATUS_DSP_LANDING, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

/* SheikDSP.air_collision_ (0xF0 map). */
void ndsP4SheikDSPAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SheikDSPAirToGround);
}

/* SheikDSP.attack_collision_ (0xF1 map): last frame's wall or any contact
 * (temp variable 1, gNdsP4SheikOverrides) recoils her (check_recoil_);
 * else the landing, then a second pass for a ledge. */
void ndsP4SheikDSPAttackMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->coll_data.mask_prev & (MAP_FLAG_LWALL | MAP_FLAG_RWALL))
    {
        fp->motion_vars.flags.flag0 = 1;
    }
    if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->physics.vel_air.y = SHEIK_DSP_RECOIL_Y_SPEED;
        fp->motion_vars.flags.flag0 = 0;
        fp->motion_vars.flags.flag1 = 0;
        ftMainSetStatus(fighter_gobj, SHEIK_STATUS_DSP_RECOIL, 0.0F, 1.0F, SHEIK_PRESERVE_DSP_RECOIL);
        return;
    }
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SheikDSPAirToGround);

    if ((mpCommonCheckFighterCeilHeavyCliff(fighter_gobj) != FALSE) &&
        (fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK))
    {
        ftCommonCliffCatchSetStatus(fighter_gobj);
    }
}

/* SheikDSP.recoil_main_ (0xF3 update): falls at the end of the motion;
 * from frame 15 to 40 a B tap (this frame or last) turns her around into
 * another attack. */
void ndsP4SheikDSPRecoilMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 taps = ndsP4SheikDSPBufferTaps(fp);

    if (ftAnimEndCheckSetStatus(fighter_gobj, ftCommonFallSetStatus) != FALSE)
    {
        return;
    }
    if (!(fighter_gobj->anim_frame <= SHEIK_DSP_RECOIL_FRAME_MAX) ||
        (fighter_gobj->anim_frame <= SHEIK_DSP_RECOIL_FRAME_MIN) ||
        !(taps & SHEIK_DSP_TAP_B))
    {
        return;
    }
    fp->lr = (fp->lr != 1) ? 1 : -1;
    ndsP4SheikDSPAttackSetStatus(fighter_gobj);
}

/* Wario.asm body_slam_recoil_, at ftMainSetHitInteractStats' head: any
 * contact in the fish's attack marks the recoil. */
static void ndsP4SheikOnHitInteract(FTStruct *fp, s32 attack_type)
{
    (void)attack_type;

    if (fp->status_id == SHEIK_STATUS_DSP_ATTACK)
    {
        fp->motion_vars.flags.flag0 = 1;
    }
}

const NDSP4Overrides gNdsP4SheikOverrides = {
    .on_hit_interact = ndsP4SheikOnHitInteract,
};

#endif /* NDS_P4_SHEIK */
