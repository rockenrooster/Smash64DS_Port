/*
 * P4 Falco: native ports of the donor's Phantasm routines and Fire Bird
 * constants. Source: JSsixtyfour/smashremix 5e04fe7, src/Falco/Phantasm.asm and
 * src/Falco/Falco.asm. Falco runs Fox's status code (fp->fkind == nFTKindFox);
 * his action table replaces only neutral special's callbacks (0xE1/0xE2).
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x184 temp variable 3          motion_vars.flags.flag2 (SetFlag2, 0x5C)
 *   0x1BE button_pressed high byte input.pl.button_tap >> 8 (0x40 = B)
 *   0x18D & 0x07                   clears is_absorb, absorb_lr,
 *                                  is_goto_attack100 and is_fastfall
 *   0xA88 bit 31                   colanim.is_use_color1
 *   attributes + 0x58              attr->gravity
 */
#include <nds/nds_p4.h>
#include <ft/ftcomputer.h>

#if NDS_P4_FALCO

sb32 ftMarioSpecialHiProcPass(GObj *fighter_gobj);
void ftComputerSetCommandImmediate(FTStruct *fp, s32 index);
s32 syUtilsRandIntRange(s32 range);

/* Phantasm.asm constants. Each is a `lui` upper half; LANDING keeps the
 * low half of the Mario Super Jump routine it was copied from
 * (`ori a2, a2, 0x5C29` at 0x80156390's delay slot), so it is
 * 0x3EB35C29, not the commented 0.35. */
#define PHANTASM_X_SPEED 460.0F
#define PHANTASM_X_SPEED_END_AIR 30.0F
#define PHANTASM_X_SPEED_END_GROUND 60.0F
#define PHANTASM_Y_SPEED_INITIAL 50.0F
#define PHANTASM_SLOW_FALL_ADD 1.6015625F   /* lui 0x3FCD */
#define PHANTASM_LANDING_BITS 0x3EB35C29u
#define PHANTASM_B_PRESSED 0x4000u          /* lbu of the tap mask's high byte & 0x40 */

enum
{
    PHANTASM_INITIAL_SETUP = 1,
    PHANTASM_FREEZE_Y = 2,
    PHANTASM_MOVE = 3,
    PHANTASM_END_MOVE = 4,
    PHANTASM_SLOW_FALL = 5,
    PHANTASM_GROUND_MOVE = 2,
    PHANTASM_GROUND_END_MOVE = 3
};

/* Phantasm.button_press_buffer: last frame's tap mask per port, so a B press
 * on the frame before the dash phase still shortens it. Shared by both
 * versions and never reset, as in the donor. */
static u16 sNdsP4FalcoPhantasmTapBuffer[GMCOMMON_PLAYERS_MAX];

static u16 ndsP4FalcoPhantasmBufferedTap(FTStruct *fp)
{
    u32 port = fp->player & 3u;
    u16 tap = fp->input.pl.button_tap & 0xFF00u;
    u16 buffered = tap | sNdsP4FalcoPhantasmTapBuffer[port];

    sNdsP4FalcoPhantasmTapBuffer[port] = tap;
    return buffered;
}

static void ndsP4FalcoPhantasmEndHit(GObj *fighter_gobj, FTStruct *fp)
{
    fp->colanim.is_use_color1 = FALSE;
    ftParamClearAttackCollAll(fighter_gobj);
}

/* Phantasm.ground_subroutine_ (status 0xE1 interrupt slot). */
void ndsP4FalcoPhantasmGroundInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 phase = (s32)fp->motion_vars.flags.flag2;
    u16 tap = ndsP4FalcoPhantasmBufferedTap(fp);

    if (phase == PHANTASM_GROUND_MOVE)
    {
        fp->physics.vel_ground.x = PHANTASM_X_SPEED;

        if ((tap & PHANTASM_B_PRESSED) == 0u)
        {
            return;
        }
        fp->motion_vars.flags.flag2 = PHANTASM_GROUND_END_MOVE;
    }
    else if (phase != PHANTASM_GROUND_END_MOVE)
    {
        return;
    }
    fp->physics.vel_ground.x = PHANTASM_X_SPEED_END_GROUND;
    fp->motion_vars.flags.flag2 = 0;
    ndsP4FalcoPhantasmEndHit(fighter_gobj, fp);
}

/* Phantasm.air_subroutine_ (status 0xE2 interrupt slot). */
void ndsP4FalcoPhantasmAirInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 phase = (s32)fp->motion_vars.flags.flag2;
    u16 tap = ndsP4FalcoPhantasmBufferedTap(fp);
    sb32 end_move = FALSE;

    if (phase == PHANTASM_INITIAL_SETUP)
    {
        fp->is_absorb = FALSE;
        fp->absorb_lr = 0;
        fp->is_goto_attack100 = FALSE;
        fp->is_fastfall = FALSE;
        fp->physics.vel_air.x = 0.0F;
        fp->physics.vel_air.y = PHANTASM_Y_SPEED_INITIAL;
        return;
    }
    if (phase == PHANTASM_MOVE)
    {
        fp->physics.vel_air.x = PHANTASM_X_SPEED * (f32)fp->lr;

        if ((tap & PHANTASM_B_PRESSED) != 0u)
        {
            fp->motion_vars.flags.flag2 = PHANTASM_END_MOVE;
            end_move = TRUE;
        }
    }
    if ((phase == PHANTASM_FREEZE_Y) ||
        ((phase == PHANTASM_MOVE) && (end_move == FALSE)))
    {
        /* Writing 0 would still fall at one frame of gravity. */
        fp->physics.vel_air.y = fp->attr->gravity;
    }
    if ((phase == PHANTASM_END_MOVE) || (end_move != FALSE))
    {
        fp->physics.vel_air.x = PHANTASM_X_SPEED_END_AIR * (f32)fp->lr;
        fp->motion_vars.flags.flag2 = PHANTASM_SLOW_FALL;
        ndsP4FalcoPhantasmEndHit(fighter_gobj, fp);
        return;
    }
    if ((phase == PHANTASM_SLOW_FALL) && (fp->fkind != nFTKindKirby))
    {
        fp->physics.vel_air.y += PHANTASM_SLOW_FALL_ADD;
    }
}

/* Phantasm.air_physics_: no control until the slow-fall phase. */
void ndsP4FalcoPhantasmAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag2 == PHANTASM_SLOW_FALL)
    {
        ftPhysicsApplyAirVelDrift(fighter_gobj);
    }
    else ftPhysicsApplyAirVelFriction(fighter_gobj);
}

/* Phantasm.air_collision_: ftMarioSpecialHiProcMap with Phantasm's landing. */
void ndsP4FalcoPhantasmAirMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsAir)
    {
        if ((fp->motion_vars.flags.flag1 == 0) ||
            (fp->physics.vel_air.y >= 0.0F))
        {
            mpCommonCheckFighterProject(fighter_gobj);
        }
        else if (mpCommonCheckFighterPassCliff(fighter_gobj,
                     ftMarioSpecialHiProcPass) != FALSE)
        {
            if (fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK)
            {
                ftCommonCliffCatchSetStatus(fighter_gobj);
            }
            else
            {
                union { u32 u; f32 f; } landing;

                landing.u = PHANTASM_LANDING_BITS;
                ftCommonLandingFallSpecialSetStatus(fighter_gobj, FALSE,
                                                    landing.f);
            }
        }
    }
    else mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
}

/* Phantasm.set_variables_ replaces ftFoxSpecialNInitStatusVars for both
 * neutral specials: flag0 and flag1 cleared as before, flag2 = INITIAL_SETUP.
 * Neither Phantasm script writes flag2 before its first wait, so seeding it
 * when the status is installed (before Fox's init runs) is the same state. */
void ndsP4FalcoOnStatus(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((status_id == nFTFoxStatusSpecialN) ||
        (status_id == nFTFoxStatusSpecialAirN))
    {
        fp->motion_vars.flags.flag2 = PHANTASM_INITIAL_SETUP;
    }
}

/* Falco.asm recovery_logic, the CPU's: run after the recover objective's
 * walk (AI.asm custom_recovery_logic). In the air Phantasm it holds B (the
 * long version). Otherwise, with the nearer ledge under 2000 units away in
 * X, the fighter below it and the ledge-grab box reaching it, one time in
 * eight it targets the ledge and Phantasms toward it. Remix adds a CPU input
 * routine for that (AI.asm NSP_TOWARDS: B with the stick toward the target);
 * the vanilla routine for "neutral special toward the target"
 * (nFTComputerInputStickSmashAutoXButtonB, ftcomputer.c script 9) gives the
 * same inputs. Offsets: 0x24 status_id, 0x78 coll_data.p_translate,
 * 0x1CC+0x4C/0x54 computer.cliff_left_pos/cliff_right_pos, 0x9C8 attr,
 * attributes + 0xB0 cliffcatch_coll.y, 0x1C6 input.cp. */
void ndsP4FalcoComputerRecover(FTStruct *fp)
{
    FTComputer *com = &fp->computer;
    const Vec3f *pos = fp->coll_data.p_translate;
    f32 ledge_x;
    f32 ledge_y;

    if (fp->status_id == nFTFoxStatusSpecialAirN)
    {
        fp->input.cp.button_inputs |= B_BUTTON;
        return;
    }
    if (ABSF(com->cliff_left_pos.x - pos->x) <=
        ABSF(com->cliff_right_pos.x - pos->x))
    {
        ledge_x = com->cliff_left_pos.x;
        ledge_y = com->cliff_left_pos.y;
    }
    else
    {
        ledge_x = com->cliff_right_pos.x;
        ledge_y = com->cliff_right_pos.y;
    }
    if ((2000.0F <= ABSF(ledge_x - pos->x)) ||
        ((pos->y + fp->attr->cliffcatch_coll.y) <= ledge_y) ||
        !(pos->y <= ledge_y))
    {
        return;
    }
    if (syUtilsRandIntRange(8) != 0)
    {
        return;
    }
    com->target_pos.x = ledge_x;
    com->target_pos.y = ledge_y;
    ftComputerSetCommandImmediate(fp, nFTComputerInputStickSmashAutoXButtonB);
}

/* Falco.asm up_special_delay_ / up_special_velocity_1/2/3_. */
s32 ndsP4FoxFirefoxLaunchDelay(const FTStruct *fp, s32 fox_delay)
{
    return (ndsP4Content(fp) == NDS_P4_CONTENT_FALCO) ? 0x16 : fox_delay;
}

f32 ndsP4FoxFirefoxVel(const FTStruct *fp, f32 fox_vel)
{
    return (ndsP4Content(fp) == NDS_P4_CONTENT_FALCO) ? 98.0F : fox_vel;
}

f32 syUtilsArcTan2(f32 y, f32 x);

f32 ndsP4FoxSpecialHiArcTan2(FTStruct *fp, f32 y, f32 x, sb32 is_ground_launch)
{
    /* velocity_1_: the grounded launch has just stored Fox's literal. */
    if ((is_ground_launch != FALSE) &&
        (ndsP4Content(fp) == NDS_P4_CONTENT_FALCO) &&
        (fp->physics.vel_ground.x == 115.0F))
    {
        fp->physics.vel_ground.x = 98.0F;
    }
    return syUtilsArcTan2(y, x);
}

#endif /* NDS_P4_FALCO */
