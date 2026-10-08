/*
 * P4 Wolf: native ports of the donor's special routines. Source:
 * JSsixtyfour/smashremix 5e04fe7, src/Wolf/WolfSpecial.asm, read as
 * assembled (scripts/p4/mipsdis.py). Wolf runs Fox's status code
 * (fp->fkind == nFTKindFox). Still on lab stand-ins: WolfNSP.main, which
 * makes his own shot, and WolfUSP's slash effect (S6 articles: both live
 * in his own special files).
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x17C/0x180/0x184 temp variables 1-3  motion_vars.flags.flag0/1/2
 *   0x1BE button_pressed high byte        input.pl.button_tap >> 8 (0x40 = B)
 *   0x1C3 stick y                         input.pl.stick_range.y
 *   0x18D & 0x07                          clears is_absorb, absorb_lr,
 *                                         is_goto_attack100 and is_fastfall
 *   0x18F | 0x10                          is_effect_attach
 *   0xA88 bit 31                          colanim.is_use_color1
 *   0xB28                                 status_vars.fox.speciallw.gravity_delay
 *   attributes + 0x4C/0x50/0x58/0x5C/0x64 air_accel, air_speed_max_x,
 *                                         gravity, tvel_base, jumps_max
 */
#include <nds/nds_p4.h>

#if NDS_P4_WOLF

sb32 ftMarioSpecialHiProcPass(GObj *fighter_gobj);
void ftFoxSpecialHiHoldInitStatusVars(GObj *fighter_gobj);
void ftCaptainSpecialHiProcInterrupt(GObj *fighter_gobj);

/* Remix's slash (captainshared.asm slash_anim_struct_WOLF) is Falcon
 * Punch's effect description on Wolf's own projectile-graphic file (his
 * special 4, 0xB5B, at offsets Remix appended). Lab builds load Fox's
 * file in that slot, so until Wolf's own special files land (S6) the
 * effect is a counted stand-in: the move runs without its flame. */
static GObj *ndsP4WolfSlashMakeEffect(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    gNdsP4LabSpecialStandIns++;
    return NULL;
}

/* WolfUSP constants, as assembled. Each is a `lui` upper half; the landing
 * lag of the collision copy keeps the low half of Mario's Super Jump
 * routine it was copied from (0x3E805C29, not the commented 0.25), while
 * main_2's is the bare upper half. */
#define WOLF_USP_X_SPEED 220.0F                 /* lui 0x435C */
#define WOLF_USP_Y_SPEED 120.0F                 /* lui 0x42F0 */
#define WOLF_USP_Y_INPUT 0.796875F              /* lui 0x3F4C */
#define WOLF_USP_X_FROM_Y 0.21875F              /* lui 0x3E60 */
#define WOLF_USP_END_SLOW 0.875F                /* lui 0x3F60 */
#define WOLF_USP_MOVE_AIR_ACCEL 0.0234375F      /* lui 0x3CC0 */
#define WOLF_USP_END_AIR_ACCEL 0.0078125F       /* lui 0x3C00 */
#define WOLF_USP_LANDING_BITS 0x3E805C29u
#define WOLF_USP_FALL_LANDING 0.25F             /* lui 0x3E80 */
#define WOLF_USP_SHORTEN_FRAME 18.0F            /* lui 0x4190 */
#define WOLF_USP_B_PRESSED 0x40u
#define WOLF_DSP_GRAVITY 2.0F                   /* lui 0x4000 */

/* physics_'s temp variable 3 (flag2) phases. */
enum
{
    WOLF_USP_BEGIN = 1,
    WOLF_USP_BEGIN_MOVE = 2,
    WOLF_USP_MOVE = 3,
    WOLF_USP_END_MOVE = 4
};

/* WolfUSP.button_press_buffer: last frame's tap high byte per port, never
 * reset, as in the donor. */
static u8 sNdsP4WolfButtonBuffer[GMCOMMON_PLAYERS_MAX];

static f32 ndsP4WolfBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

/* initial_ground / initial_air: the special starters. The ground one enters
 * 0xE4 and the air one 0xE3, both aerial, as the donor names them. */
static void ndsP4WolfUSPInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
    }
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, 0);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = WOLF_USP_BEGIN;
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
    fp->physics.vel_air.y = fp->attr->gravity;
}

/* WolfUSP.initial_ground (ground_usp). */
void ndsP4WolfUSPInitialGround(GObj *fighter_gobj)
{
    ndsP4WolfUSPInitial(fighter_gobj, 0xE4);
}

/* WolfUSP.initial_air (air_usp). */
void ndsP4WolfUSPInitialAir(GObj *fighter_gobj)
{
    ndsP4WolfUSPInitial(fighter_gobj, 0xE3);
}

/* usp_2_transition_ground / _air: part 2, keeping the hitbox (flags 3). */
static void ndsP4WolfUSPTransition(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, 3);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftFoxSpecialHiHoldInitStatusVars(fighter_gobj);
}

static void ndsP4WolfUSPTransitionGround(GObj *fighter_gobj)
{
    ndsP4WolfUSPTransition(fighter_gobj, 0xE8);
}

static void ndsP4WolfUSPTransitionAir(GObj *fighter_gobj)
{
    ndsP4WolfUSPTransition(fighter_gobj, 0xE6);
}

/* main_ground / main_air: part 2 at the animation's end, or after frame 18
 * on a B press this frame or the last. */
static void ndsP4WolfUSPMain(GObj *fighter_gobj, void (*transition)(GObj *))
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u8 *buffer = &sNdsP4WolfButtonBuffer[fp->player & 3u];
    u8 tap = (u8)(fp->input.pl.button_tap >> 8);
    u32 pressed = (u32)tap | *buffer;

    *buffer = tap;
    ftAnimEndCheckSetStatus(fighter_gobj, transition);

    if (fighter_gobj->anim_frame <= WOLF_USP_SHORTEN_FRAME)
    {
        return;
    }
    if ((pressed & WOLF_USP_B_PRESSED) != 0u)
    {
        transition(fighter_gobj);
    }
}

/* WolfUSP.main_ground (0xE3 update). */
void ndsP4WolfUSPMainGround(GObj *fighter_gobj)
{
    ndsP4WolfUSPMain(fighter_gobj, ndsP4WolfUSPTransitionGround);
}

/* WolfUSP.main_air (0xE4 update). */
void ndsP4WolfUSPMainAir(GObj *fighter_gobj)
{
    ndsP4WolfUSPMain(fighter_gobj, ndsP4WolfUSPTransitionAir);
}

/* WolfUSP.change_direction_ (interrupt): Falcon Dive's turn when the
 * script sets temp variable 2 to 2. */
void ndsP4WolfUSPChangeDirection(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->motion_vars.flags.flag1 == 2)
    {
        ftCaptainSpecialHiProcInterrupt(fighter_gobj);
    }
}

/* WolfUSP.air_control_: Fox's drift, slower while moving. */
static void ndsP4WolfUSPAirControl(FTStruct *fp, FTAttributes *attr)
{
    f32 accel = attr->air_accel;

    if (fp->motion_vars.flags.flag2 == WOLF_USP_MOVE)
    {
        accel = WOLF_USP_MOVE_AIR_ACCEL;
    }
    else if (fp->motion_vars.flags.flag2 == WOLF_USP_END_MOVE)
    {
        accel = WOLF_USP_END_AIR_ACCEL;
    }
    ftPhysicsClampAirVelXStickRange(fp, 8, accel, attr->air_speed_max_x);
}

/* WolfUSP.physics_: ftPhysicsApplyAirVelDrift with no drift while
 * beginning, then the phase steps of temp variable 3. */
void ndsP4WolfUSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    (fp->is_fastfall) ? ftPhysicsApplyFastFall(fp, attr) :
                        ftPhysicsApplyGravityDefault(fp, attr);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        if (fp->motion_vars.flags.flag2 == WOLF_USP_BEGIN)
        {
            (void)ftPhysicsCheckClampAirVelXDecMax(fp, attr);
        }
        else ndsP4WolfUSPAirControl(fp, attr);

        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
    if (fp->motion_vars.flags.flag2 == WOLF_USP_BEGIN)
    {
        fp->physics.vel_air.x = 0.0F;
        fp->physics.vel_air.y = 0.0F;
    }
    if (fp->motion_vars.flags.flag2 == WOLF_USP_BEGIN_MOVE)
    {
        /* The donor loads the facing, then overwrites it with stick Y. */
        f32 vel_y = WOLF_USP_Y_SPEED +
                    ((f32)fp->input.pl.stick_range.y * WOLF_USP_Y_INPUT);
        f32 vel_x = WOLF_USP_X_SPEED - (WOLF_USP_X_FROM_Y * vel_y);

        fp->physics.vel_air.y = vel_y;
        fp->physics.vel_air.x = (f32)fp->lr * vel_x;
        fp->motion_vars.flags.flag2 = WOLF_USP_MOVE;
        fp->jumps_used = (u8)attr->jumps_max;
        return;
    }
    if (fp->motion_vars.flags.flag2 == WOLF_USP_MOVE)
    {
        fp->physics.vel_air.y += attr->gravity;
    }
    if (fp->motion_vars.flags.flag2 == WOLF_USP_END_MOVE)
    {
        fp->colanim.is_use_color1 = FALSE;
        fp->physics.vel_air.x *= WOLF_USP_END_SLOW;
        fp->physics.vel_air.y *= WOLF_USP_END_SLOW;

        (fp->is_fastfall) ? ftPhysicsApplyFastFall(fp, attr) :
                            ftPhysicsApplyGravityDefault(fp, attr);

        if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
        {
            ftPhysicsClampAirVelXStickDefault(fp, attr);
            ftPhysicsApplyAirVelXFriction(fp, attr);
        }
    }
}

/* WolfUSP.collision_: Mario's Super Jump map routine with Wolf's landing
 * lag. */
void ndsP4WolfUSPMap(GObj *fighter_gobj)
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
                fighter_gobj, FALSE, ndsP4WolfBitsToF32(WOLF_USP_LANDING_BITS));
        }
    }
    else mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
}

/* WolfUSP.main_2 (0xE6/0xE8 update): Fox's Fire Fox end with Falcon Punch's
 * flame while temp variable 1 is 0 (stopped when the script sets it to 2),
 * Wolf's landing lag, and the interrupt flag taken from temp variable 1.
 * The donor's Pokemon Stadium announcer call is outside the port. */
void ndsP4WolfUSPMain2(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 flag0 = fp->motion_vars.flags.flag0;

    if (flag0 == 0)
    {
        if (ndsP4WolfSlashMakeEffect(fighter_gobj) != NULL)
        {
            fp->is_effect_attach = TRUE;
        }
        /* `sb 1, 0x17C`: the big-endian high byte of the word. */
        fp->motion_vars.flags.flag0 = (flag0 & 0x00FFFFFFu) | 0x01000000u;
    }
    if (flag0 == 2)
    {
        ftParamProcStopEffect(fighter_gobj);
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, 1.0F, FALSE, TRUE, FALSE,
                                     WOLF_USP_FALL_LANDING,
                                     (flag0 != 0) ? TRUE : FALSE);
    }
}

/* WolfDSP.physics_: Fox's reflector physics with a gravity of 2. */
void ndsP4WolfDSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    if (fp->status_vars.fox.speciallw.gravity_delay != 0)
    {
        fp->status_vars.fox.speciallw.gravity_delay--;
    }
    else ftPhysicsApplyGravityClampTVel(fp, WOLF_DSP_GRAVITY, attr->tvel_base);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

/* WolfNSP.air_to_ground_: landing keeps the shot's ground status (0xE1)
 * at the same frame and hitboxes. */
static void ndsP4WolfNSPAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, 0xE1, fighter_gobj->anim_frame, 1.0F, 1);
}

/* WolfNSP.air_collision_ (0xE2 map). */
void ndsP4WolfNSPAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4WolfNSPAirToGround);
}

#endif /* NDS_P4_WOLF */
