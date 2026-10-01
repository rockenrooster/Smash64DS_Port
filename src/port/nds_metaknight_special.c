/* Native lowering of EXTRA MetaKnightSpecial.asm at 96621afea26a83305abaf81add07dcf5a9c5fe3e.
 * Normal animation/events, collision, status resets and physics stay in their
 * existing BattleShip owners. No MIPS addresses or donor pointers reach here.
 * Source numeric encodings and delay slots take precedence over comments.
 * Not accepted until the resolved tables, material seam and directed witnesses
 * are bound and qualified together with native models and motion events. */
#include <common.h>
#include <nds/nds_metaknight.h>
#include <nds/nds_p4_roster.h>
#include <sys/audio.h>
#include <PR/os.h>
#include <math.h>
#include <string.h>

/* These are already existing source owners; narrow declarations avoid the
 * decomp's broad sys/obj.h and ef/effect.h include graph. */
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);
sb32 mpCommonCheckFighterCeilHeavyCliff(GObj *fighter_gobj);
typedef struct LBParticle LBParticle;
LBParticle *efManagerDustHeavyDoubleMakeEffect(Vec3f *pos, s32 lr, f32 f_index);
void ndsBaseFTCommonJumpAerialUpdateModelYaw(FTStruct *fp);

/* Preserve the original common-input predicates while selecting this
 * character's resolved native setters before any legacy kind-indexed table. */
sb32 ndsMetaKnightCheckSpecialN(GObj *gobj)
{
    FTStruct *fp = ftGetStruct(gobj);
    if ((fp->input.pl.button_tap & fp->input.button_mask_b) &&
        fp->attr->is_have_specialn &&
        fp->input.pl.stick_range.y < FTCOMMON_SPECIALHI_STICK_RANGE_MIN &&
        fp->input.pl.stick_range.y > FTCOMMON_SPECIALLW_STICK_RANGE_MIN)
    {
        if (fp->input.pl.stick_range.x * fp->lr < FTCOMMON_SPECIALN_TURN_STICK_RANGE_MIN)
            ftParamSetStickLR(fp);
        ndsMetaKnightNSPGroundInitial(gobj);
        return TRUE;
    }
    return FALSE;
}

sb32 ndsMetaKnightCheckSpecialHi(GObj *gobj)
{
    FTStruct *fp = ftGetStruct(gobj);
    if ((fp->input.pl.button_tap & fp->input.button_mask_b) &&
        fp->attr->is_have_specialhi &&
        fp->input.pl.stick_range.y >= FTCOMMON_SPECIALHI_STICK_RANGE_MIN)
    {
        ndsMetaKnightUSPGroundInitial(gobj);
        return TRUE;
    }
    return FALSE;
}

sb32 ndsMetaKnightCheckSpecialLw(GObj *gobj)
{
    FTStruct *fp = ftGetStruct(gobj);
    if ((fp->input.pl.button_tap & fp->input.button_mask_b) &&
        fp->attr->is_have_speciallw &&
        fp->input.pl.stick_range.y <= FTCOMMON_SPECIALLW_STICK_RANGE_MIN)
    {
        ndsMetaKnightDSPGroundInitial(gobj);
        return TRUE;
    }
    return FALSE;
}

sb32 ndsMetaKnightCheckSpecialAir(GObj *gobj)
{
    FTStruct *fp = ftGetStruct(gobj);
    if (!(fp->input.pl.button_tap & fp->input.button_mask_b) ||
        ftHammerCheckHoldHammer(gobj)) return FALSE;
    if (fp->input.pl.stick_range.y >= FTCOMMON_SPECIALHI_STICK_RANGE_MIN)
    {
        if (!fp->attr->is_have_specialairhi) return FALSE;
        ndsMetaKnightUSPAirInitial(gobj);
    }
    else if (fp->input.pl.stick_range.y <= FTCOMMON_SPECIALLW_STICK_RANGE_MIN)
    {
        if (!fp->attr->is_have_specialairlw) return FALSE;
        ndsMetaKnightDSPAirInitial(gobj);
    }
    else
    {
        if (!fp->attr->is_have_specialairn) return FALSE;
        if (fp->input.pl.stick_range.x * fp->lr < FTCOMMON_SPECIALN_TURN_STICK_RANGE_MIN)
            ftParamSetStickLR(fp);
        ndsMetaKnightNSPAirInitial(gobj);
    }
    return TRUE;
}

static f32 ndsMetaKnightTemp1Float(const FTStruct *fp)
{
    f32 value;
    memcpy(&value, &fp->motion_vars.flags.flag0, sizeof(value));
    return value;
}

static void ndsMetaKnightSetTemp1Float(FTStruct *fp, f32 value)
{
    memcpy(&fp->motion_vars.flags.flag0, &value, sizeof(value));
}

static f32 ndsMetaKnightFloatBits(u32 bits)
{
    f32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void ndsMetaKnightChangeStatus(GObj *gobj, s32 status, f32 frame, f32 speed)
{
    ftMainSetStatus(gobj, status, frame, speed, FTSTATUS_PRESERVE_NONE);
}

void ndsMetaKnightInitPassiveVars(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    fp->passive_vars.metaknight.cape_environment = 0u;
    fp->passive_vars.metaknight.wing_state = -1;
}

void ndsMetaKnightSetCapeEnvironment(GObj *gobj, sb32 hidden)
{
    ftGetStruct(gobj)->passive_vars.metaknight.cape_environment =
        hidden ? 0xffffff00u : 0u;
}

sb32 ndsMetaKnightCapeHidden(const FTStruct *fp)
{
    return fp != NULL && (u32)fp->fkind == NDS_P4_RUNTIME_METAKNIGHT &&
        fp->passive_vars.metaknight.cape_environment == 0xffffff00u;
}

sb32 ndsMetaKnightWingIsVisible(const FTStruct *fp)
{
    return fp->passive_vars.metaknight.wing_state != -1;
}

void ndsMetaKnightOnActionChanged(FTStruct *fp, s32 next_status)
{
    sb32 carry;
    s32 joint;
    switch (next_status)
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
    case nFTCommonStatusDamageFlyTop:
    case nFTCommonStatusDamageFall:
    case nFTCommonStatusDamageFlyRoll:
    case nFTCommonStatusWallDamage:
    case nFTCommonStatusDamageE1:
    case nFTCommonStatusDamageE2:
    case nFTCommonStatusCaptureCaptain:
    case nFTCommonStatusLGunShootAir:
    case nFTCommonStatusFireFlowerShootAir:
        carry = TRUE;
        break;
    default:
        carry = FALSE;
        break;
    }
    if (carry != FALSE)
    {
        for (joint = 0x1E; joint <= 0x21; joint++)
        {
            ftParamSetModelPartID(fp->fighter_gobj, joint,
                                 fp->passive_vars.metaknight.wing_state);
        }
    }
    else
    {
        fp->passive_vars.metaknight.wing_state = -1;
        if (fp->joints[0x1E] != NULL)
        {
            /* Native modelpart IDs are authoritative after donor-to-native
             * modelpart mapping. Source reads wing DObj +0x50; that donor
             * rendering domain must be validated against this typed ID. */
            fp->passive_vars.metaknight.wing_state =
                fp->modelpart_status[0x1E - nFTPartsJointCommonStart].modelpart_id_curr;
        }
    }
}

sb32 ndsMetaKnightCaptureDKInterrupt(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return TRUE;
}

void ndsMetaKnightMultiJumpSetStatus(GObj *fighter_gobj, s32 input_source)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    s32 used = fp->jumps_used;
    (void)input_source; /* Source uses stick X and full stick Y for both paths. */
    ftMainSetStatus(fighter_gobj, nNDSMetaKnightStatusJump2 + used - 1,
                    0.0F, 1.0F, FTSTATUS_PRESERVE_PLAYERTAG);
    fp->physics.vel_air.x = fp->input.pl.stick_range.x * attr->jumpaerial_vel_x;
    if (used == 1)
    {
        fp->physics.vel_air.y = ((80.0F * attr->jump_height_mul) +
                                 attr->jump_height_base) * attr->jumpaerial_height;
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    }
    else
    {
        /* config.yaml height multipliers 3..6=60; jump_decay=80. */
        fp->physics.vel_air.y = 60.0F;
    }
    fp->jumps_used++;
    fp->motion_vars.flags.flag1 = 0;
    fp->is_special_interrupt = TRUE;
    fp->status_vars.common.jumpaerial.turn_tics =
        ((fp->input.pl.stick_range.x * fp->lr) < FTCOMMON_JUMPAERIAL_TURN_STICK_RANGE_MIN) ?
        FTCOMMON_JUMPAERIAL_TURN_FRAMES : 0;
    ndsBaseFTCommonJumpAerialUpdateModelYaw(fp);
}

sb32 ndsMetaKnightCheckJumpInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 input_source;
    if ((ftHammerCheckHoldHammer(fighter_gobj) != FALSE) ||
        (fp->jumps_used >= fp->attr->jumps_max)) return FALSE;
    if (fp->jumps_used == 1)
    {
        input_source = ftCommonKneeBendGetInputTypeCommon(fp);
    }
    else
    {
        if ((fp->status_id >= nNDSMetaKnightStatusJump2) &&
            (fp->status_id <= nNDSMetaKnightStatusJump6) &&
            (fp->motion_vars.flags.flag1 == 0)) return FALSE;
        if (fp->input.pl.stick_range.y >= FTCOMMON_JUMPAERIAL_STICK_RANGE_MIN)
            input_source = FTCOMMON_JUMPAERIAL_INPUT_TYPE_STICK;
        else if (fp->input.pl.button_hold & (R_CBUTTONS | L_CBUTTONS | D_CBUTTONS | U_CBUTTONS))
            input_source = FTCOMMON_JUMPAERIAL_INPUT_TYPE_BUTTON;
        else input_source = FTCOMMON_JUMPAERIAL_INPUT_TYPE_NONE;
    }
    if (input_source == FTCOMMON_JUMPAERIAL_INPUT_TYPE_NONE) return FALSE;
    ndsMetaKnightMultiJumpSetStatus(fighter_gobj, input_source);
    return TRUE;
}

void ndsMetaKnightAerialAttackPhysics(GObj *fighter_gobj)
{
    /* AerialAttackFastFall.fast_fall_check: the pinned optional toggle has
     * four FALSE defaults (Toggles.asm:2438). P4 preserves vanilla common
     * rules and excludes this unrelated optional Remix gameplay toggle. */
    ftPhysicsApplyAirVelDrift(fighter_gobj);
}

static void ndsMetaKnightJabFirstLoop(GObj *fighter_gobj)
{
    ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusJabLoop, 0.0F, 1.0F);
    ftGetStruct(fighter_gobj)->motion_vars.flags.flag0 = 1;
}

static void ndsMetaKnightJabRepeatLoop(GObj *fighter_gobj)
{
    FTStruct *fp;
    ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusJabLoop, 0.0F, 1.0F);
    fp = ftGetStruct(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->proc_shield = ndsMetaKnightJabLoopRecoil;
    fp->proc_hit = ndsMetaKnightJabLoopRecoil;
}

void ndsMetaKnightJabStartUpdate(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsMetaKnightJabFirstLoop);
}

void ndsMetaKnightJabLoopUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    if ((fp->motion_vars.flags.flag0 == 0) && !(fp->input.pl.button_hold & A_BUTTON))
        ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusJabEnd, 0.0F, 1.0F);
    else ftAnimEndCheckSetStatus(fighter_gobj, ndsMetaKnightJabRepeatLoop);
}

void ndsMetaKnightJabLoopRecoil(GObj *fighter_gobj)
{
    /* Source bltz tests at=0x41700000, not the loaded lr register: -15 in
     * both directions. Preserve the actual donor result for review. */
    ftGetStruct(fighter_gobj)->physics.vel_ground.x -= 15.0F;
}

void ndsMetaKnightTiltFUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 frame = fighter_gobj->anim_frame;
    if (frame == 2.0F) fp->motion_vars.flags.flag2 = 0;
    if (fp->input.pl.button_tap & A_BUTTON) fp->motion_vars.flags.flag2 = 1;
    if ((fp->motion_vars.flags.flag2 != 0) &&
        (frame > ((fp->status_id == nFTCommonStatusAttackS3) ? 10.0F : 7.0F)))
    {
        ndsMetaKnightChangeStatus(fighter_gobj,
            (fp->status_id == nFTCommonStatusAttackS3) ? nNDSMetaKnightStatusTiltF2 :
                                                      nNDSMetaKnightStatusTiltF3,
            0.0F, 1.0F);
        ftMainPlayAnimEventsAll(fighter_gobj);
    }
    else ftAnimEndSetWait(fighter_gobj);
}

static void ndsMetaKnightUSPInitial(GObj *fighter_gobj, s32 status)
{
    FTStruct *fp;
    ndsMetaKnightChangeStatus(fighter_gobj, status, 0.0F, 1.0F);
    fp = ftGetStruct(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->jumps_used = fp->attr->jumps_max;
}

void ndsMetaKnightUSPAirInitial(GObj *fighter_gobj)
{
    ndsMetaKnightUSPInitial(fighter_gobj, nNDSMetaKnightStatusUSPAirStart);
}

void ndsMetaKnightUSPGroundInitial(GObj *fighter_gobj)
{
    ndsMetaKnightUSPInitial(fighter_gobj, nNDSMetaKnightStatusUSP);
}

static void ndsMetaKnightUSPAirContinue(GObj *fighter_gobj)
{
    ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusUSPAir, 6.0F, 1.0F);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

void ndsMetaKnightUSPAirStartUpdate(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsMetaKnightUSPAirContinue);
}

void ndsMetaKnightUSPUpdate(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
        ftCommonFallSpecialSetStatus(fighter_gobj, 0.5F, TRUE, TRUE, TRUE, 1.0F, FALSE);
}

static void ndsMetaKnightUSPLand(GObj *fighter_gobj)
{
    ftCommonLandingFallSpecialSetStatus(fighter_gobj, FALSE, 1.0F);
}

void ndsMetaKnightUSPMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 frame = fighter_gobj->anim_frame;
    if (fp->ga == nMPKineticsGround) mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
    else if (frame < 13.0F) mpCommonCheckFighterCeilHeavyCliff(fighter_gobj);
    else
    {
        fp->coll_data.ignore_line_id = (frame <= 28.0F) ? fp->coll_data.floor_line_id : -1;
        mpCommonProcFighterCliff(fighter_gobj, ndsMetaKnightUSPLand);
    }
}

void ndsMetaKnightUSPPhysics(GObj *fighter_gobj)
{
    f32 frame = fighter_gobj->anim_frame;
    if (frame <= 7.0F)
    {
        ftPhysicsApplyGroundVelFriction(fighter_gobj);
        if (frame == 7.0F) mpCommonSetFighterAir(ftGetStruct(fighter_gobj));
    }
    else if (frame <= 48.0F) ftPhysicsApplyAirVelTransNAll(fighter_gobj);
    else ftPhysicsApplyAirVelDrift(fighter_gobj);
}

void ndsMetaKnightUSPInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *trans = fp->joints[nFTPartsJointTransN];
    s32 stick, absolute;
    f32 rotation;
    /* Branches target _rotate_model._end, then fall through to reapply the
     * saved rotation every frame. Input can increase it only at frame 7. */
    if (fighter_gobj->anim_frame == 7.0F)
    {
        stick = fp->input.pl.stick_range.x;
        absolute = (stick < 0) ? -stick : stick;
        if (absolute >= 50)
        {
            rotation = -(((stick - ((stick > 0) ? 50 : -50)) * 0.6F) * PI32 / 180.0F);
            if (fabsf(trans->rotate.vec.f.z) < fabsf(rotation))
            {
                trans->rotate.vec.f.z = rotation;
                ndsMetaKnightSetTemp1Float(fp, rotation);
            }
        }
    }
    trans->rotate.vec.f.z = ndsMetaKnightTemp1Float(fp);
}

void ndsMetaKnightNSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp;
    f32 velocity;
    ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusTornadoStart, 0.0F, 1.0F);
    fp = ftGetStruct(fighter_gobj);
    ndsMetaKnightSetTemp1Float(fp, 130.0F);
    fp->status_vars.metaknight.temp2 = 0;
    fp->physics.vel_air.x = 0.0F;
    fp->jumps_used = fp->attr->jumps_max;
    velocity = fp->physics.vel_air.y * ndsMetaKnightFloatBits(0x3ECC0000u);
    /* The store in bc1t's delay slot zeros descending speed as well. */
    fp->physics.vel_air.y = (velocity < 0.0F) ? 0.0F : velocity + 30.0F;
}

void ndsMetaKnightNSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    mpCommonSetFighterAir(fp);
    ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusTornadoStart, 0.0F, 1.0F);
    ndsMetaKnightSetTemp1Float(fp, 130.0F);
    fp->status_vars.metaknight.temp2 = 0;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 30.0F;
    fp->jumps_used = fp->attr->jumps_max;
}

static void ndsMetaKnightNSPStartLoop(GObj *fighter_gobj)
{
    ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusTornadoLoop, 0.0F, 130.0F);
}

void ndsMetaKnightNSPStartUpdate(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsMetaKnightNSPStartLoop);
}

void ndsMetaKnightNSPLoopUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 speed = ndsMetaKnightTemp1Float(fp);
    Vec3f dust;
    if (fp->status_total_tics < 60)
    {
        speed -= 2.0F;
        fp->status_vars.metaknight.temp2--;
        if ((fp->status_vars.metaknight.temp2 <= 0) && (fp->input.pl.button_tap & B_BUTTON))
        {
            fp->status_vars.metaknight.temp2 = 4;
            speed += 7.0F;
            ftPhysicsAddClampAirVelY(fp, 26.0F, 36.0F);
        }
    }
    else speed -= 3.0F;
    ndsMetaKnightSetTemp1Float(fp, speed);
    if (fighter_gobj->anim_frame <= speed) func_800269C0_275C0(0x104);
    /* Source CC is mask_prev, not mask_curr. */
    if (((fp->status_total_tics & 7u) == 0) && (fp->coll_data.mask_prev & MAP_FLAG_FLOOR))
    {
        dust.x = fp->coll_data.p_translate->x - fp->lr * 80.0F;
        dust.y = fp->coll_data.p_translate->y - 80.0F;
        dust.z = 0.0F;
        efManagerDustHeavyDoubleMakeEffect(&dust, -fp->lr, 1.0F);
    }
    gcSetAnimSpeed(fighter_gobj, speed);
    if (speed <= 60.0F)
    {
        fp->physics.vel_air.x *= 0.5F;
        ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusTornadoEndAir, 0.0F, 1.0F);
    }
}

void ndsMetaKnightNSPLoopPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 fast = fp->motion_scripts[1][0].p_script != NULL; /* Source 8AC. */
    ftPhysicsApplyGravityClampTVel(fp, 1.0F, 12.0F);
    ftPhysicsClampAirVelXStickRange(fp, 0, fast ? 0.3F : 0.06F, fast ? 80.0F : 60.0F);
    ftPhysicsApplyAirVelXFriction(fp, fp->attr);
}

void ndsMetaKnightNSPLoopMap(GObj *fighter_gobj)
{
    FTStruct *fp;
    mpCommonCheckFighterCeilHeavyCliff(fighter_gobj);
    fp = ftGetStruct(fighter_gobj);
    if (fp->coll_data.mask_curr & (MAP_FLAG_RWALL | MAP_FLAG_LWALL))
    {
        fp->physics.vel_ground.x *= -2.0F;
        fp->physics.vel_air.x *= -2.0F;
    }
}

static void ndsMetaKnightNSPEndLand(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    /* Donor air_to_ground does not initialize ftMainSetStatus's fifth stack
     * argument. Deterministic NONE is a candidate; a directed donor landing
     * trace must qualify the source flags before accepting this transition. */
    ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusTornadoEnd,
                             fighter_gobj->anim_frame, 1.0F);
}

void ndsMetaKnightNSPEndAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsMetaKnightNSPEndLand);
}

static void ndsMetaKnightCapeHide(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    fp->is_invisible = TRUE;
    fp->is_jostle_ignore = TRUE;
    ndsMetaKnightSetCapeEnvironment(fighter_gobj, TRUE);
    /* Source writes the low byte of passive hitstatus (5B8..5BB), leaving
     * any upper bits as they were. Existing hitstatus producers consume it. */
    fp->hitstatus = (fp->hitstatus & ~0xFF) | nGMHitStatusIntangible;
}

void ndsMetaKnightDSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp;
    ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusCapeStart, 0.0F, 1.0F);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp = ftGetStruct(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->status_vars.metaknight.temp2 = -1;
}

void ndsMetaKnightDSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp;
    ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusCapeAirStart, 0.0F, 1.0F);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp = ftGetStruct(fighter_gobj);
    fp->physics.vel_air.x *= 0.5F;
    fp->physics.vel_air.y *= 0.5F;
    fp->motion_vars.flags.flag0 = 0;
    fp->status_vars.metaknight.temp2 = -1;
}

static void ndsMetaKnightCapeFinish(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 air = fp->ga != nMPKineticsGround;
    s32 status, previous_lr;
    s32 stick = fp->input.pl.stick_range.x;
    if (!(fp->input.pl.button_hold & (A_BUTTON | B_BUTTON)))
    {
        status = air ? nNDSMetaKnightStatusCapeAirEnd : nNDSMetaKnightStatusCapeEnd;
        if ((stick <= -11) || (stick >= 11))
        {
            previous_lr = fp->lr;
            ftParamSetStickLR(fp);
            if (fp->lr != previous_lr) fp->physics.vel_ground.x = -fp->physics.vel_ground.x;
        }
    }
    else
    {
        if (air != FALSE)
        {
            fp->physics.vel_air.x = 0.0F;
            fp->physics.vel_air.y = 0.0F;
        }
        if ((stick >= -11) && (stick < 12))
            status = air ? nNDSMetaKnightStatusCapeAirN : nNDSMetaKnightStatusCapeN;
        else if ((fp->lr ^ stick) < 0)
            status = air ? nNDSMetaKnightStatusCapeAirF : nNDSMetaKnightStatusCapeF;
        else
        {
            fp->lr = -fp->lr;
            status = air ? nNDSMetaKnightStatusCapeAirB : nNDSMetaKnightStatusCapeB;
        }
    }
    ndsMetaKnightChangeStatus(fighter_gobj, status, 0.0F, 1.0F);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

void ndsMetaKnightDSPStartUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 x, y, magnitude;
    if (fighter_gobj->anim_frame == 13.0F)
    {
        gcSetAnimSpeed(fighter_gobj, 0.5F);
        x = fp->input.pl.stick_range.x;
        y = fp->input.pl.stick_range.y;
        /* Source sqrt.s then cvt.w.s uses nearest-even rounding. With integer
         * squared inputs, the <11 decision is equivalent to r^2 <= 110. */
        magnitude = (x * x) + (y * y);
        fp->status_vars.metaknight.temp2 = magnitude <= 110;
        if ((fp->ga == nMPKineticsGround) && (y > 0))
        {
            mpCommonSetFighterAir(fp);
            ndsMetaKnightChangeStatus(fighter_gobj, nNDSMetaKnightStatusCapeAirStart,
                                     fighter_gobj->anim_frame, 0.5F);
        }
        fp->jumps_used = fp->attr->jumps_max;
        fp->motion_vars.flags.flag0 = fp->attr->jumps_max; /* Actual delay slot. */
        if (fp->status_vars.metaknight.temp2 == 0)
            ndsMetaKnightSetTemp1Float(fp, atan2f((f32)y, (f32)(x * fp->lr)));
        ndsMetaKnightCapeHide(fighter_gobj);
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ndsMetaKnightCapeFinish);
}

void ndsMetaKnightDSPStartPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 angle, x, y;
    if (fp->status_vars.metaknight.temp2 != 0) return;
    angle = ndsMetaKnightTemp1Float(fp);
    x = 60.0F * cosf(angle);
    y = 60.0F * __sinf(angle);
    if (fp->ga == nMPKineticsGround)
    {
        fp->physics.vel_ground.x = x;
        fp->physics.vel_air.x = x * fp->lr;
    }
    else
    {
        fp->physics.vel_air.x = x * fp->lr;
        fp->physics.vel_air.y = y;
    }
}

static void ndsMetaKnightCapeSwitch(GObj *fighter_gobj, sb32 air)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 frame = fighter_gobj->anim_frame;
    f32 speed = ((DObj *)fighter_gobj->obj)->anim_speed;
    if (air != FALSE) mpCommonSetFighterAir(fp);
    else mpCommonSetFighterGround(fp);
    ndsMetaKnightChangeStatus(fighter_gobj,
        air ? nNDSMetaKnightStatusCapeAirStart : nNDSMetaKnightStatusCapeStart,
        frame, speed);
    if (fp->status_vars.metaknight.temp2 >= 0) ndsMetaKnightCapeHide(fighter_gobj);
}

static void ndsMetaKnightCapeLand(GObj *fighter_gobj)
{
    ndsMetaKnightCapeSwitch(fighter_gobj, FALSE);
}

static void ndsMetaKnightCapeFall(GObj *fighter_gobj)
{
    ndsMetaKnightCapeSwitch(fighter_gobj, TRUE);
}

void ndsMetaKnightDSPStartGroundMap(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 16.0F)
        mpCommonProcFighterOnFloor(fighter_gobj, ndsMetaKnightCapeFall);
    else mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
}

void ndsMetaKnightDSPStartAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterCliff(fighter_gobj, ndsMetaKnightCapeLand);
}

void ndsMetaKnightDSPAttackAirUpdate(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
        ftCommonFallSpecialSetStatus(fighter_gobj, 0.6F, TRUE, TRUE, TRUE, 1.0F, FALSE);
}

void ndsMetaKnightDSPAttackAirPhysics(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame >= 21.0F) ftPhysicsApplyAirVelFriction(fighter_gobj);
    else ftPhysicsApplyAirVelTransNAll(fighter_gobj);
}
