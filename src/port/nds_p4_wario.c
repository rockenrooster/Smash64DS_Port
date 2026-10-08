/*
 * P4 Wario: native ports of the donor's special routines. Source:
 * JSsixtyfour/smashremix 5e04fe7, src/Wario/WarioSpecial.asm and Wario.asm,
 * read as assembled (scripts/p4/mipsdis.py): the OS.copy_segment blocks are
 * the original game's code. Wario runs Mario's status code
 * (fp->fkind == nFTKindMario).
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x00D port                             player
 *   0x060 ground x velocity                physics.vel_ground.x
 *   0x078 position pointer                 coll_data.p_translate
 *   0x0CC collision flags                  coll_data.mask_prev (the map
 *                                          callback runs after ftmain moves
 *                                          mask_curr there and zeroes it:
 *                                          last frame's walls)
 *   0x0F4 floor flags                      coll_data.floor_flags
 *   0x17C/0x180/0x184 temp variables 1-3   motion_vars.flags.flag0/1/2
 *   0x18D & 0x07                           clears is_absorb, absorb_lr,
 *                                          is_goto_attack100 and is_fastfall
 *   0x1C3 stick y                          input.pl.stick_range.y
 *   0x5BC shoulder hurtbox state           damage_colls[0].hitstatus
 *   attributes + 0x54/0x58/0x64            air_friction, gravity, jumps_max
 * The routines Remix runs as a status's interrupt (the *_move_ ones) read
 * the player struct from a2, which the caller left there; here it is the
 * fighter's own.
 */
#include <nds/nds_p4.h>

#if NDS_P4_WARIO

#include <ef/effect.h>
#include <sys/audio.h>

sb32 ftMarioSpecialHiProcPass(GObj *fighter_gobj);
void ftCaptainSpecialHiProcInterrupt(GObj *fighter_gobj);
f32 ftCaptainSpecialNGetAngle(s32 stick_y);
s32 ftCommonKneeBendGetInputTypeCommon(FTStruct *fp);
f32 __sinf(f32);

/* Wario's action array. */
#define WARIO_STATUS_NSP_GROUND 0xDF
#define WARIO_STATUS_NSP_AIR 0xE0
#define WARIO_STATUS_USP 0xE1
#define WARIO_STATUS_DSP_LANDING 0xE2
#define WARIO_STATUS_DSP_GROUND 0xE3
#define WARIO_STATUS_DSP_AIR 0xE4
#define WARIO_STATUS_NSP_RECOIL_GROUND 0xE5
#define WARIO_STATUS_NSP_RECOIL_AIR 0xE6

/* Temp variable 3 states. */
#define WARIO_BEGIN 1
#define WARIO_MOVE 2
#define WARIO_USP_BEGIN_MOVE 2
#define WARIO_USP_MOVE 3
#define WARIO_USP_END_MOVE 4

/* WarioNSP constants (each a `lui` upper half). */
#define WARIO_NSP_X_SPEED 64.0F
#define WARIO_NSP_Y_SPEED 30.0F
#define WARIO_NSP_JUMP_SPEED 68.0F
#define WARIO_NSP_RECOIL_X_SPEED -40.0F
#define WARIO_NSP_RECOIL_Y_SPEED 40.0F
#define WARIO_NSP_GRAVITY 2.75F
#define WARIO_NSP_MAX_FALL_SPEED 48.0F
#define WARIO_NSP_AIR_FRICTION 2.0F
#define WARIO_NSP_GROUND_TRACTION 0.5F
#define WARIO_FGM_RECOIL 0x117
#define WARIO_FGM_JUMP 0x5E

/* WarioUSP / WarioDSP constants. */
#define WARIO_USP_Y_SPEED 64.0F
#define WARIO_DSP_Y_SPEED -80.0F
#define WARIO_DSP_INITIAL_Y_SPEED 180.0F
#define WARIO_DSP_INITIAL_X_SPEED 90.0F

/* Hit-status preserve: hit and colanim. */
#define WARIO_PRESERVE_HIT 0x3u

static f32 ndsP4WarioBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

static void ndsP4WarioClearFastFall(FTStruct *fp)
{
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
}

/* Jim's special collision flag, one a port: keeps a grounded Body Slam
 * that lands against a wall from recoiling off it. */
static u8 sNdsP4WarioJimFlag[GMCOMMON_PLAYERS_MAX];

/* ---- Neutral special (Body Slam) ---- */

/* WarioNSP.ground_initial_ (ground_nsp). */
void ndsP4WarioNSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, WARIO_STATUS_NSP_GROUND, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = WARIO_BEGIN;
}

/* WarioNSP.air_initial_ (air_nsp): the fast fall kept (preserve 8) and
 * then cleared, the fall slowed to his gravity. */
void ndsP4WarioNSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, WARIO_STATUS_NSP_AIR, 0.0F, 1.0F, FTSTATUS_PRESERVE_FASTFALL);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = WARIO_BEGIN;
    ndsP4WarioClearFastFall(fp);
    fp->physics.vel_air.y = WARIO_NSP_GRAVITY;
}

/* WarioNSP.ground_move_ (0xDF interrupt). */
void ndsP4WarioNSPGroundMove(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag2 == WARIO_BEGIN)
    {
        fp->physics.vel_ground.x *= 0.875F;
    }
    if (fp->motion_vars.flags.flag2 == WARIO_MOVE)
    {
        fp->physics.vel_ground.x = WARIO_NSP_X_SPEED;
    }
}

/* WarioNSP.ground_physics_ (0xDF physics): the ground friction with his
 * traction (0.5) in place of the attribute's. */
void ndsP4WarioNSPGroundPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 material = fp->coll_data.floor_flags & MAP_VERTEX_MAT_MASK;

    ftPhysicsSetGroundVelFriction(fp, dMPCollisionMaterialFrictions[material & 0x0fu] *
                                          WARIO_NSP_GROUND_TRACTION);
    ftPhysicsSetGroundVelTransferAir(fighter_gobj);
}

static void ndsP4WarioNSPGroundToAir(GObj *fighter_gobj)
{
    mpCommonSetFighterAir(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, WARIO_STATUS_NSP_AIR, fighter_gobj->anim_frame, 1.0F,
                    WARIO_PRESERVE_HIT);
}

static void ndsP4WarioNSPAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, WARIO_STATUS_NSP_GROUND, fighter_gobj->anim_frame, 1.0F,
                    WARIO_PRESERVE_HIT);
}

/* WarioNSP.begin_recoil_: his bounce off what he hit. */
static void ndsP4WarioNSPBeginRecoil(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    func_800269C0_275C0(WARIO_FGM_RECOIL);
    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
    }
    ftMainSetStatus(fighter_gobj, WARIO_STATUS_NSP_RECOIL_AIR, 0.0F, 1.0F, WARIO_PRESERVE_HIT);
}

/* WarioNSP.check_recoil_: TRUE when the recoil flag started the bounce
 * (the donor then returns to its caller's end). */
static sb32 ndsP4WarioNSPCheckRecoil(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag0 == 0)
    {
        return FALSE;
    }
    ndsP4WarioNSPBeginRecoil(fighter_gobj);
    fp->motion_vars.flags.flag1 = 0;
    fp->physics.vel_air.x = WARIO_NSP_RECOIL_X_SPEED * (f32)fp->lr;
    fp->physics.vel_air.y = WARIO_NSP_RECOIL_Y_SPEED;
    return TRUE;
}

/* The wall Body Slam ran into last frame, on his facing's side. */
static u32 ndsP4WarioWall(FTStruct *fp)
{
    return fp->coll_data.mask_prev & ((fp->lr >= 0) ? MAP_FLAG_LWALL : MAP_FLAG_RWALL);
}

/* WarioNSP.ground_collision_ (0xDF map). */
void ndsP4WarioNSPGroundMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u8 *jim = &sNdsP4WarioJimFlag[fp->player & 3];

    if (fp->motion_vars.flags.flag2 != WARIO_MOVE)
    {
        *jim = 0;
    }
    else if (ndsP4WarioWall(fp) == 0u)
    {
        *jim = 0;
    }
    else if (*jim == 0)
    {
        fp->motion_vars.flags.flag0 = 1;
    }
    if (ndsP4WarioNSPCheckRecoil(fighter_gobj) != FALSE)
    {
        return;
    }
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4WarioNSPGroundToAir);

    /* A jump out of the slam's run: the air slam, 68 up, the double dust
     * and its sound. (Kirby's copy is S8's.) */
    if ((fp->status_id == WARIO_STATUS_NSP_GROUND) &&
        (fp->motion_vars.flags.flag2 == WARIO_MOVE) &&
        (ftCommonKneeBendGetInputTypeCommon(fp) != 0))
    {
        ndsP4WarioNSPGroundToAir(fighter_gobj);
        fp->physics.vel_air.y = WARIO_NSP_JUMP_SPEED;
        efManagerDustHeavyDoubleMakeEffect(fp->coll_data.p_translate, 1, 1.0F);
        func_800269C0_275C0(WARIO_FGM_JUMP);
    }
}

/* WarioNSP.air_move_ (0xE0 interrupt): the slam's hang, its dash, and the
 * script's lift (temp variable 2) angled by the stick like Falcon Punch. */
void ndsP4WarioNSPAirMove(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag2 == WARIO_BEGIN)
    {
        fp->physics.vel_air.x *= 0.875F;
        fp->physics.vel_air.y = WARIO_NSP_GRAVITY;
    }
    if (fp->motion_vars.flags.flag2 == WARIO_MOVE)
    {
        fp->physics.vel_air.x = WARIO_NSP_X_SPEED * (f32)fp->lr;
    }
    if (fp->motion_vars.flags.flag1 != 0)
    {
        f32 angle;

        fp->motion_vars.flags.flag1 = 0;
        angle = ftCaptainSpecialNGetAngle(fp->input.pl.stick_range.y);
        fp->physics.vel_air.y = ((__sinf(angle) * WARIO_NSP_X_SPEED) * 0.75F) + WARIO_NSP_Y_SPEED;
    }
}

/* WarioNSP.air_physics_ (0xE0 physics): his gravity and fall cap, then
 * the air friction at 2.0 (the donor passes its stack as the attributes,
 * the friction at 0x54). */
void ndsP4WarioNSPAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsApplyGravityClampTVel(fp, WARIO_NSP_GRAVITY, WARIO_NSP_MAX_FALL_SPEED);
    if (fp->physics.vel_air.x < 0.0F)
    {
        fp->physics.vel_air.x += WARIO_NSP_AIR_FRICTION;
        if (fp->physics.vel_air.x >= 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
    else
    {
        fp->physics.vel_air.x -= WARIO_NSP_AIR_FRICTION;
        if (fp->physics.vel_air.x <= 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
}

/* WarioNSP.air_collision_ (0xE0 map). */
void ndsP4WarioNSPAirMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u8 *jim = &sNdsP4WarioJimFlag[fp->player & 3];

    if ((fp->motion_vars.flags.flag2 != WARIO_MOVE) || (ndsP4WarioWall(fp) == 0u))
    {
        *jim = 0;
    }
    else *jim = TRUE;

    if (ndsP4WarioNSPCheckRecoil(fighter_gobj) != FALSE)
    {
        return;
    }
    mpCommonProcFighterCliff(fighter_gobj, ndsP4WarioNSPAirToGround);
}

/* WarioNSP.recoil_move_ (0xE6 interrupt): until the script frees him
 * (temp variable 2), the drift fades and the fall slows. The donor adds
 * 1.25 for Remix's Wario id and 0.55 for the Kirby and Polygon copies. */
void ndsP4WarioNSPRecoilMove(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 == 0)
    {
        fp->physics.vel_air.x *= 0.96875F;
        fp->physics.vel_air.y += 1.25F;
    }
}

/* WarioNSP.recoil_physics_ (0xE6 physics). */
void ndsP4WarioNSPRecoilPhysics(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->motion_vars.flags.flag1 != 0)
    {
        ftPhysicsApplyAirVelDrift(fighter_gobj);
    }
    else ftPhysicsApplyAirVelFriction(fighter_gobj);
}

static void ndsP4WarioNSPRecoilToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, WARIO_STATUS_NSP_RECOIL_GROUND, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
}

static void ndsP4WarioNSPRecoilToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, WARIO_STATUS_NSP_RECOIL_AIR, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
    ftPhysicsClampAirVelXMax(fp);
}

/* WarioNSP.recoil_ground_collision_ (0xE5 map). */
void ndsP4WarioNSPRecoilGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4WarioNSPRecoilToAir);
}

/* WarioNSP.recoil_air_collision_ (0xE6 map). */
void ndsP4WarioNSPRecoilAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterCliff(fighter_gobj, ndsP4WarioNSPRecoilToGround);
}

/* Wario.body_slam_recoil_ (ftMainSetHitInteractStats's head): Body Slam
 * meeting a hurtbox or a shield, or clanging while his shoulder is not
 * invincible, bounces him. */
static void ndsP4WarioOnHitInteract(FTStruct *fp, s32 attack_type)
{
    if ((fp->status_id != WARIO_STATUS_NSP_GROUND) && (fp->status_id != WARIO_STATUS_NSP_AIR))
    {
        return;
    }
    if ((attack_type == nGMHitTypeDamage) || (attack_type == nGMHitTypeShield) ||
        ((attack_type == nGMHitTypeAttack) &&
         (fp->damage_colls[0].hitstatus != nGMHitStatusInvincible)))
    {
        fp->motion_vars.flags.flag0 = 1;
    }
}

/* ---- Up special (Corkscrew) ---- */

/* WarioUSP.initial_ (ground_usp, air_usp): airborne, then the status;
 * the donor plays no events here. */
void ndsP4WarioUSPInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
    }
    ftMainSetStatus(fighter_gobj, WARIO_STATUS_USP, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = WARIO_BEGIN;
    ndsP4WarioClearFastFall(fp);
    fp->physics.vel_air.y = fp->attr->gravity;
}

/* WarioUSP.main_ (0xE1 update): Fox's Fire Fox end with his landing lag
 * (0.25) and no interrupt. */
void ndsP4WarioUSPMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, 1.0F, FALSE, TRUE, FALSE, 0.25F, FALSE);
    }
}

/* WarioUSP.change_direction_ (0xE1 interrupt): Captain's turn while the
 * script holds temp variable 2 at 2. */
void ndsP4WarioUSPChangeDirection(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->motion_vars.flags.flag1 == 2)
    {
        ftCaptainSpecialHiProcInterrupt(fighter_gobj);
    }
}

/* WarioUSP.air_control_: the attribute drift, slower while moving. */
static void ndsP4WarioUSPAirControl(FTStruct *fp, FTAttributes *attr)
{
    f32 accel = attr->air_accel;

    if (fp->motion_vars.flags.flag2 == WARIO_USP_MOVE)
    {
        accel = ndsP4WarioBitsToF32(0x3CC00000u);
    }
    else if (fp->motion_vars.flags.flag2 == WARIO_USP_END_MOVE)
    {
        accel = ndsP4WarioBitsToF32(0x3C000000u);
    }
    ftPhysicsClampAirVelXStickRange(fp, 8, accel, attr->air_speed_max_x);
}

/* WarioUSP.physics_ (0xE1 physics). */
void ndsP4WarioUSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    if (fp->is_fastfall)
    {
        ftPhysicsApplyFastFall(fp, attr);
    }
    else ftPhysicsApplyGravityDefault(fp, attr);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        if (fp->motion_vars.flags.flag2 == WARIO_BEGIN)
        {
            (void)ftPhysicsCheckClampAirVelXDecMax(fp, attr);
        }
        else ndsP4WarioUSPAirControl(fp, attr);

        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
    if (fp->motion_vars.flags.flag2 == WARIO_BEGIN)
    {
        fp->physics.vel_air.x *= 0.875F;
        fp->physics.vel_air.y = 0.0F;
    }
    if (fp->motion_vars.flags.flag2 == WARIO_USP_BEGIN_MOVE)
    {
        f32 speed_y = WARIO_USP_Y_SPEED;
        f32 add_x = 0.0F;
        f32 stick = (f32)fp->input.pl.stick_range.x * (f32)fp->lr;

        /* The donor compares against 0 in f2: a stick short of forward
         * keeps that 0. */
        if (0.0F <= stick)
        {
            add_x = stick * 0.5F;
            speed_y -= ndsP4WarioBitsToF32(0x3E600000u) * add_x;
        }
        fp->physics.vel_air.x = (f32)fp->lr * add_x;
        fp->physics.vel_air.y = speed_y;
        fp->motion_vars.flags.flag2 = WARIO_USP_MOVE;
        fp->jumps_used = (u8)attr->jumps_max;
        return;
    }
    if (fp->motion_vars.flags.flag2 == WARIO_USP_MOVE)
    {
        fp->physics.vel_air.y += attr->gravity;
    }
    if (fp->motion_vars.flags.flag2 == WARIO_USP_END_MOVE)
    {
        fp->physics.vel_air.x *= 0.875F;
        fp->physics.vel_air.y = 0.0F;
    }
}

/* WarioUSP.collision_ (0xE1 map): Mario's Super Jump map with his landing
 * lag; the copied delay slot keeps the low half of Mario's constant
 * (0x3E805C29). */
void ndsP4WarioUSPMap(GObj *fighter_gobj)
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
                                                     ndsP4WarioBitsToF32(0x3E805C29u));
        }
    }
    else mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
}

/* ---- Down special (Ground Pound) ---- */

/* WarioDSP.ground_initial_ (ground_dsp). */
void ndsP4WarioDSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, WARIO_STATUS_DSP_GROUND, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = WARIO_BEGIN;
}

/* WarioDSP.air_initial_ (air_dsp): the fast fall kept, then cleared. */
void ndsP4WarioDSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, WARIO_STATUS_DSP_AIR, 0.0F, 1.0F, FTSTATUS_PRESERVE_FASTFALL);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = WARIO_BEGIN;
    ndsP4WarioClearFastFall(fp);
}

/* WarioDSP.ground_move_ (0xE3 interrupt): the hop (temp variable 1) puts
 * him in the air at 90 forward, 180 up. */
void ndsP4WarioDSPGroundMove(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->physics.vel_air.x *= 0.875F;
    if (fp->motion_vars.flags.flag2 == WARIO_BEGIN)
    {
        fp->physics.vel_air.y *= 0.875F;
    }
    if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->motion_vars.flags.flag0 = 0;
        fp->physics.vel_air.x = WARIO_DSP_INITIAL_X_SPEED * (f32)fp->lr;
        fp->physics.vel_air.y = WARIO_DSP_INITIAL_Y_SPEED;
        mpCommonSetFighterAir(fp);
    }
}

/* WarioDSP.air_move_ (0xE4 interrupt). */
void ndsP4WarioDSPAirMove(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->physics.vel_air.x *= 0.875F;
    if (fp->motion_vars.flags.flag2 == WARIO_BEGIN)
    {
        fp->physics.vel_air.y *= 0.875F;
    }
}

/* WarioDSP.physics_ (0xE3/0xE4 physics): control only once the script
 * allows it (temp variable 2); no fall while beginning; the pound's drop
 * at -80. */
void ndsP4WarioDSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 != 0)
    {
        ftPhysicsApplyAirVelDrift(fighter_gobj);
    }
    else ftPhysicsApplyAirVelFriction(fighter_gobj);

    if (fp->motion_vars.flags.flag2 == WARIO_BEGIN)
    {
        /* The donor tests the float's sign bit: -0.0 is cleared too. */
        union { f32 f; u32 u; } y;

        y.f = fp->physics.vel_air.y;
        if ((y.u & 0x80000000u) != 0u)
        {
            fp->physics.vel_air.y = 0.0F;
            return;
        }
    }
    if (fp->motion_vars.flags.flag2 == WARIO_MOVE)
    {
        fp->physics.vel_air.y = WARIO_DSP_Y_SPEED;
    }
}

/* WarioDSP.begin_landing_: Falcon Kick's landing setter with his status. */
static void ndsP4WarioDSPBeginLanding(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, WARIO_STATUS_DSP_LANDING, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

/* WarioDSP.collision_ (0xE3/0xE4 map): the pound lands in its own status;
 * anything else lands as usual. */
void ndsP4WarioDSPMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
    }
    else if (fp->motion_vars.flags.flag2 == WARIO_MOVE)
    {
        mpCommonProcFighterCliff(fighter_gobj, ndsP4WarioDSPBeginLanding);
    }
    else mpCommonProcFighterCliffWaitOrLanding(fighter_gobj);
}

const NDSP4Overrides gNdsP4WarioOverrides = {
    .on_hit_interact = ndsP4WarioOnHitInteract,
};

#endif /* NDS_P4_WARIO */
