/*
 * P4 Banjo: native ports of the donor's special routines and of the patch
 * Remix makes on his character id. Source: JSsixtyfour/smashremix 5e04fe7,
 * src/Banjo/BanjoSpecial.asm and Wario.asm (body_slam_recoil_), read as
 * assembled (scripts/p4/mipsdis.py): the OS.copy_segment blocks are the
 * original game's code. Banjo runs Captain's status code
 * (fp->fkind == nFTKindCaptain).
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x044 direction                        lr
 *   0x0CC/0x0D2 collision flags            coll_data.mask_prev (last frame's
 *                                          walls), coll_data.mask_stat
 *   0x120/0x134 wall angles                coll_data.lwall_angle/rwall_angle
 *   0x148 jumps used                       jumps_used
 *   0x14C kinetic state                    ga
 *   0x17C/0x180/0x184 temp variables 1-3   motion_vars.flags.flag0/1/2
 *   0x18C bit 19                           is_fastfall
 *   0x18D & 0x07                           clears is_absorb, absorb_lr,
 *                                          is_goto_attack100 and is_fastfall
 *   0x1BE buttons tapped                   input.pl.button_tap (the Beak
 *                                          Bomb's buffer keeps its high byte)
 *   0x1C2 stick x                          input.pl.stick_range.x
 *   0x93C part (Kazooie's head)            joints[21]
 *   0xA04/0xA08/0xA0C                      proc_lagstart, proc_lagend,
 *                                          proc_status
 *   0xB18/0xB2C                            status_vars words 0 and 5: the
 *                                          Beak Bomb's tap buffer (Fox's
 *                                          launch delay once it attacks)
 *                                          and its grounded start
 *   object + 0x74                          the top joint (DObjGetStruct):
 *                                          translate 0x1C/0x20, rotate
 *                                          0x30 (x) and 0x38 (z)
 *   attributes + 0x58/0x64                 gravity, jumps_max
 *
 * His eggs are his own weapons on his special file 4 (Banjo file 9, S6,
 * below). S6 owns his entry (linkshared.asm entry_anim_struct_*_BANJO);
 * Kirby's copy is S8's.
 */
#include <nds/nds_p4.h>

#if NDS_P4_BANJO

#if !NDS_P2_SAMUS
#error "P4 Banjo's eggs reflect through Samus's bomb routine (NDS_P2_SAMUS)"
#endif

#include <ef/effect.h>
#include <macros.h>
#include <sys/audio.h>
#include <sys/obj.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

sb32 ftMarioSpecialHiProcPass(GObj *fighter_gobj);
void ftCaptainSpecialHiProcStatus(GObj *fighter_gobj);
void ftCaptainSpecialHiProcInterrupt(GObj *fighter_gobj);
void ftCaptainSpecialLwDecideMapCollide(GObj *fighter_gobj);
sb32 ftCaptainSpecialLwBoundCheckGoto(GObj *fighter_gobj);
void ftFoxSpecialHiHoldInitStatusVars(GObj *fighter_gobj);
void ftParamProcPauseEffect(GObj *fighter_gobj);
void ftParamProcResumeEffect(GObj *fighter_gobj);
f32 syUtilsArcTan2(f32 y, f32 x);

/* Banjo.Action (his action array; 0xEF on are add_new_action's). */
#define BANJO_STATUS_NSPG_BEGIN 0xE1
#define BANJO_STATUS_NSPG_FORWARD 0xE2
#define BANJO_STATUS_NSPG_BACKWARD 0xE3
#define BANJO_STATUS_NSPA_BEGIN 0xE4
#define BANJO_STATUS_NSPA_FORWARD 0xE5
#define BANJO_STATUS_NSPA_BACKWARD 0xE6
#define BANJO_STATUS_DSPA 0xE7
#define BANJO_STATUS_DSP_LAND 0xE8
#define BANJO_STATUS_DSPG 0xE9
#define BANJO_STATUS_DSPG_AIR 0xEA
#define BANJO_STATUS_DSPA_LOOP 0xEB
#define BANJO_STATUS_USP_BEGIN 0xEF
#define BANJO_STATUS_USP_ATTACK 0xF0
#define BANJO_STATUS_USP_ATTACK_END 0xF1
#define BANJO_STATUS_USP_RECOIL 0xF2
#define BANJO_STATUS_USP_WALL_SPLAT 0xF3

/* BanjoNSP (the eggs). */
#define BANJO_NSP_DEADZONE 6
#define BANJO_NSP_KAZOOIE_HEAD_JOINT 21
#define BANJO_NSP_FORWARD_OFF_X -40.0F
#define BANJO_PRESERVE_NSP 0x0002u          /* colanim */

/* BanjoUSP (the Beak Bomb). */
#define BANJO_USP_Y_SPEED 101.0F
#define BANJO_USP_Y_SPEED_AIR 90.0F
#define BANJO_USP_X_SPEED_BACK -30.0F
#define BANJO_USP_Y_SPEED_ATTACK 0.0F
#define BANJO_USP_X_SPEED_ATTACK 80.0F
#define BANJO_USP_RECOIL_X_SPEED -20.0F
#define BANJO_USP_RECOIL_Y_SPEED 48.0F
#define BANJO_USP_SPLAT_X_SPEED -15.0F
#define BANJO_USP_SPLAT_Y_SPEED 5.0F
#define BANJO_USP_FALL_DRIFT 1.0F
/* LANDING_FSM and BEGIN_LANDING_FSM ("1.5"): the special falls take the
 * upper halves; the landing maps, copies of Mario's, keep the low half of
 * his constant. */
#define BANJO_USP_FALL_LANDING 1.0F
#define BANJO_USP_BEGIN_FALL_LANDING 2.0F
#define BANJO_USP_LANDING_LAG 0x3F805C29u
#define BANJO_USP_BEGIN_LANDING_LAG 0x40005C29u
#define BANJO_USP_ATTACK_FRAME 10.0F
#define BANJO_USP_ATTACK_END_FRAME 39.0F
#define BANJO_USP_ATTACK_END_MUL 0.5F       /* "0.25" */
#define BANJO_USP_RECOIL_X_MUL 0.96875F
#define BANJO_USP_RECOIL_Y_ADD 1.25F
#define BANJO_USP_SPLAT_ROTATE_MUL 0.90625F
#define BANJO_USP_TAP_B 0x40u               /* B_PRESSED, the taps' high byte */
#define BANJO_PRESERVE_USP_ATTACK 0x0003u   /* hit, colanim */
/* begin_physics_'s temp variable 3. */
#define BANJO_USP_BEGIN 0
#define BANJO_USP_BEGIN_MOVE 1
#define BANJO_USP_MOVE 2
/* attack_physics_'s temp variable 3. */
#define BANJO_USP_ATTACK_BEGIN 0
#define BANJO_USP_ATTACK_REAR_BACK 1
#define BANJO_USP_ATTACK_BEGIN_MOVE 2
#define BANJO_USP_ATTACK_MOVE 3

/* BanjoDSP (Bill Drill and Beak Barge). */
#define BANJO_DSP_RISE_Y_SPEED 12.0F        /* AERIAL_INITIAL_Y_SPEED */
#define BANJO_DSP_Y_SPEED -110.0F
#define BANJO_DSP_BEGIN 1
#define BANJO_DSP_MOVE 2
#define BANJO_DSP_BEGIN_Y_MUL 0.75F
#define BANJO_DSP_X_MUL 0.875F

static f32 ndsP4BanjoBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

static s32 *ndsP4BanjoStatusS32(FTStruct *fp, s32 word)
{
    return &((s32 *)(void *)&fp->status_vars)[word];
}

static void ndsP4BanjoClearFlags(FTStruct *fp)
{
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

static void ndsP4BanjoClearFastFall(FTStruct *fp)
{
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
}

/* ---- Neutral special (the eggs) ---- */

/* BanjoNSP.begin_initial_. */
static void ndsP4BanjoNSPInitial(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* BanjoNSP.ground_begin_initial_ (ground_nsp). */
void ndsP4BanjoNSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4BanjoNSPInitial(fighter_gobj, BANJO_STATUS_NSPG_BEGIN);
}

/* BanjoNSP.air_begin_initial_ (air_nsp). */
void ndsP4BanjoNSPAirInitial(GObj *fighter_gobj)
{
    ndsP4BanjoNSPInitial(fighter_gobj, BANJO_STATUS_NSPA_BEGIN);
}

/* BanjoNSP.ground_shoot_forward_initial_ / ground_shoot_backward_initial_:
 * from the begin's frame (its end of motion). */
static void ndsP4BanjoNSPGroundShootInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    ftMainSetStatus(fighter_gobj, status_id, fighter_gobj->anim_frame, 1.0F, BANJO_PRESERVE_NSP);
}

/* BanjoNSP.air_shoot_forward_initial_ / air_shoot_backward_initial_. */
static void ndsP4BanjoNSPAirShootInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
        ftPhysicsClampAirVelXMax(fp);
    }
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, BANJO_PRESERVE_NSP);
}

/* BanjoNSP.begin_main_ (0xE1/0xE4 update): at the end of the motion the
 * egg goes forward unless the stick is held 6 or more against him, which
 * lays it behind. */
void ndsP4BanjoNSPBeginMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 stick_x;
    s32 stick_lr;
    sb32 is_forward;

    if (!(fighter_gobj->anim_frame <= 0.0F))
    {
        return;
    }
    stick_x = fp->input.pl.stick_range.x;

    if (stick_x >= 0)
    {
        stick_lr = ((stick_x - BANJO_NSP_DEADZONE) < 0) ? 0 : +1;
    }
    else stick_lr = ((stick_x + BANJO_NSP_DEADZONE) > 0) ? 0 : -1;

    is_forward = ((stick_lr == fp->lr) || (stick_lr == 0)) ? TRUE : FALSE;

    if (fp->ga == nMPKineticsGround)
    {
        ndsP4BanjoNSPGroundShootInitial(fighter_gobj, (is_forward != FALSE) ? BANJO_STATUS_NSPG_FORWARD
                                                                            : BANJO_STATUS_NSPG_BACKWARD);
    }
    else ndsP4BanjoNSPAirShootInitial(fighter_gobj, (is_forward != FALSE) ? BANJO_STATUS_NSPA_FORWARD
                                                                           : BANJO_STATUS_NSPA_BACKWARD);
}

/* BanjoNSP.air_begin_transition_. */
static void ndsP4BanjoNSPBeginGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftPhysicsClampAirVelXMax(fp);
    ftMainSetStatus(fighter_gobj, BANJO_STATUS_NSPA_BEGIN, fighter_gobj->anim_frame, 1.0F,
                    BANJO_PRESERVE_NSP);
}

/* BanjoNSP.ground_begin_transition_. */
static void ndsP4BanjoNSPBeginAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, BANJO_STATUS_NSPG_BEGIN, fighter_gobj->anim_frame, 1.0F,
                    BANJO_PRESERVE_NSP);
}

/* BanjoNSP.ground_begin_collision_ (0xE1 map). */
void ndsP4BanjoNSPBeginGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4BanjoNSPBeginGroundToAir);
}

/* BanjoNSP.air_begin_collision_ (0xE4 map). */
void ndsP4BanjoNSPBeginAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4BanjoNSPBeginAirToGround);
}

/* ---- The eggs (forward_egg_projectile_struct, backward_egg_projectile_struct) ----
 *
 * His own weapons on his special file 4 (Banjo file 9, generated as
 * gNdsP4BanjoSpecial4): one set of WPAttributes at 0 for both, the forward
 * egg on Ray Gun ammo's map routine, the backward one bouncing on Mario's
 * fireball map routine (its rebound and fire sparks, Mario's attribute row
 * 0, since the egg clears the fireball index); both bounce off shields as
 * Master Hand's bullet and reflect as Samus's bomb. Remix's projectile id
 * is a Remix one; the DS renderer keys native weapon owners on the kind. */
#define BANJO_EGG_HIT_FGM 0x28
#define BANJO_EGG_SPEED_MAX 250.0F

typedef struct NDSP4BanjoEggProperties
{
    s32 lifetime;
    f32 gravity;
    f32 rotate_step;
    f32 angle_ground;
    f32 angle_air;
    f32 speed;
} NDSP4BanjoEggProperties;

/* forward_egg_properties_struct (EGG_DURATION) and
 * backward_egg_properties_struct (BACKWARD_EGG_DURATION; its angle of 128 is
 * in radians as given). */
static const NDSP4BanjoEggProperties sNdsP4BanjoForwardEgg = { 30, 0.1F, 0.15F, 0.0F, 0.0F, 90.0F };
static const NDSP4BanjoEggProperties sNdsP4BanjoBackwardEgg = { 120, 1.2F, 0.1F, 128.0F, 128.0F, 28.0F };

extern void *gNdsP4BanjoSpecial4;

/* Makers that found his egg file missing: counted, never a fault. */
__attribute__((used)) volatile u32 gNdsP4BanjoArticleMisses;

f32 __sinf(f32);
f32 __cosf(f32);
LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
sb32 itLGunWeaponAmmoProcMap(GObj *weapon_gobj);
sb32 wpMarioFireballProcMap(GObj *weapon_gobj);
sb32 wpSamusBombProcReflector(GObj *weapon_gobj);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);

/* egg_destruction_ (hit, shield, clang, absorb): smoke, destroyed. */
static sb32 ndsP4BanjoEggProcDestroy(GObj *weapon_gobj)
{
    (void)efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

    return TRUE;
}

/* forward_egg_main_ / backward_egg_main_ (update): smoke at the end of its
 * life; until then its gravity, speed capped at 250, and a turn about x. */
static sb32 ndsP4BanjoEggUpdate(GObj *weapon_gobj, const NDSP4BanjoEggProperties *egg)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        (void)efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

        return TRUE;
    }
    wpMainApplyGravityClampTVel(wp, egg->gravity, BANJO_EGG_SPEED_MAX);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.x += egg->rotate_step;

    return FALSE;
}

static sb32 ndsP4BanjoForwardEggProcUpdate(GObj *weapon_gobj)
{
    return ndsP4BanjoEggUpdate(weapon_gobj, &sNdsP4BanjoForwardEgg);
}

static sb32 ndsP4BanjoBackwardEggProcUpdate(GObj *weapon_gobj)
{
    return ndsP4BanjoEggUpdate(weapon_gobj, &sNdsP4BanjoBackwardEgg);
}

/* wpBossBulletProcHop (shield bounce), which Remix's descs name. */
static sb32 ndsP4BanjoEggProcHop(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    syVectorRotateAbout3D(&wp->physics.vel_air, &wp->shield_collide_dir,
                          wp->shield_collide_angle * 2);
    wpMainReflectorRotateWeaponModel(weapon_gobj);

    return FALSE;
}

static WPDesc sNdsP4BanjoForwardEggWeaponDesc = {
    0x00,
    NDS_P4_WP_KIND_BANJO_EGG,
    &gNdsP4BanjoSpecial4,
    0x0,
    { nGCMatrixKindTraRotRpyRSca, 0x47, 0 },    /* 0x12470000 */
    ndsP4BanjoForwardEggProcUpdate,
    itLGunWeaponAmmoProcMap,
    ndsP4BanjoEggProcDestroy,
    ndsP4BanjoEggProcDestroy,
    ndsP4BanjoEggProcHop,
    ndsP4BanjoEggProcDestroy,
    wpSamusBombProcReflector,
    ndsP4BanjoEggProcDestroy
};

static WPDesc sNdsP4BanjoBackwardEggWeaponDesc = {
    0x00,
    NDS_P4_WP_KIND_BANJO_EGG,
    &gNdsP4BanjoSpecial4,
    0x0,
    { nGCMatrixKindTraRotRpyRSca, 0x47, 0 },    /* 0x12470000 */
    ndsP4BanjoBackwardEggProcUpdate,
    wpMarioFireballProcMap,
    ndsP4BanjoEggProcDestroy,
    ndsP4BanjoEggProcDestroy,
    ndsP4BanjoEggProcHop,
    ndsP4BanjoEggProcDestroy,
    wpSamusBombProcReflector,
    ndsP4BanjoEggProcDestroy
};

/* BanjoNSP.egg_stage_setting_ / egg_poop_stage_setting_: the egg, with the
 * fireball index cleared, a hit sound of 0x28 and a normal hit, flying at
 * its angle and speed in his facing. */
static void ndsP4BanjoEggMakeWeapon(GObj *fighter_gobj, Vec3f *pos, WPDesc *desc,
                                    const NDSP4BanjoEggProperties *egg)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *weapon_gobj;
    WPStruct *wp;
    DObj *dobj;
    f32 angle;

    if (gNdsP4BanjoSpecial4 == NULL)
    {
        gNdsP4BanjoArticleMisses++;
        return;
    }
    weapon_gobj = wpManagerMakeWeapon(fighter_gobj, desc, pos,
                                      WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER);
    if (weapon_gobj == NULL)
    {
        return;
    }
    wp = wpGetStruct(weapon_gobj);
    wp->lifetime = egg->lifetime;
    *(s32 *)(void *)&wp->weapon_vars = 0;
    wp->attack_coll.fgm_id = BANJO_EGG_HIT_FGM;
    wp->attack_coll.element = nGMHitElementNormal;

    angle = (fp->ga == nMPKineticsAir) ? egg->angle_air : egg->angle_ground;

    wp->physics.vel_air.z = 0.0F;
    wp->physics.vel_air.x = __cosf(angle) * egg->speed * fp->lr;
    wp->physics.vel_air.y = __sinf(angle) * egg->speed;

    dobj = DObjGetStruct(weapon_gobj);

    if (dobj->mobj != NULL)
    {
        dobj->mobj->palette_id = 0.0F;
    }
    wpMainVelSetModelPitch(weapon_gobj);
}

/* BanjoNSP.shoot_forward_main_ / shoot_backward_main_: temp variable 1
 * fires the egg from Kazooie's head (40 behind it forward). */
static void ndsP4BanjoNSPShootMain(GObj *fighter_gobj, sb32 is_forward)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag0 != 0)
    {
        Vec3f pos;

        fp->motion_vars.flags.flag0 = 0;
        pos.x = (is_forward != FALSE) ? BANJO_NSP_FORWARD_OFF_X : 0.0F;
        pos.y = pos.z = 0.0F;
        gmCollisionGetFighterPartsWorldPosition(fp->joints[BANJO_NSP_KAZOOIE_HEAD_JOINT], &pos);
        pos.z = 0.0F;

        if (is_forward != FALSE)
        {
            ndsP4BanjoEggMakeWeapon(fighter_gobj, &pos, &sNdsP4BanjoForwardEggWeaponDesc,
                                    &sNdsP4BanjoForwardEgg);
        }
        else ndsP4BanjoEggMakeWeapon(fighter_gobj, &pos, &sNdsP4BanjoBackwardEggWeaponDesc,
                                     &sNdsP4BanjoBackwardEgg);
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        fp->motion_vars.flags.flag0 = 0;
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* BanjoNSP.shoot_forward_main_ (0xE2/0xE5 update). */
void ndsP4BanjoNSPShootForwardMain(GObj *fighter_gobj)
{
    ndsP4BanjoNSPShootMain(fighter_gobj, TRUE);
}

/* BanjoNSP.shoot_backward_main_ (0xE3/0xE6 update). */
void ndsP4BanjoNSPShootBackwardMain(GObj *fighter_gobj)
{
    ndsP4BanjoNSPShootMain(fighter_gobj, FALSE);
}

/* BanjoNSP.air_shoot_physics_ (0xE5/0xE6 physics): drift once the script
 * sets temp variable 2. */
void ndsP4BanjoNSPAirShootPhysics(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->motion_vars.flags.flag1 != 0)
    {
        ftPhysicsApplyAirVelDrift(fighter_gobj);
    }
    else ftPhysicsApplyAirVelFriction(fighter_gobj);
}

/* BanjoNSP.ground_shoot_*_transition_: Samus's aerial end's first lines. */
static void ndsP4BanjoNSPShootAirToGround(GObj *fighter_gobj, s32 status_id)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, status_id, fighter_gobj->anim_frame, 1.0F, BANJO_PRESERVE_NSP);
}

/* BanjoNSP.air_shoot_*_transition_: Samus's ground end's first lines, no
 * clamp. */
static void ndsP4BanjoNSPShootGroundToAir(GObj *fighter_gobj, s32 status_id)
{
    mpCommonSetFighterAir(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, status_id, fighter_gobj->anim_frame, 1.0F, BANJO_PRESERVE_NSP);
}

static void ndsP4BanjoNSPShootForwardAirToGround(GObj *fighter_gobj)
{
    ndsP4BanjoNSPShootAirToGround(fighter_gobj, BANJO_STATUS_NSPG_FORWARD);
}

static void ndsP4BanjoNSPShootBackwardAirToGround(GObj *fighter_gobj)
{
    ndsP4BanjoNSPShootAirToGround(fighter_gobj, BANJO_STATUS_NSPG_BACKWARD);
}

static void ndsP4BanjoNSPShootForwardGroundToAir(GObj *fighter_gobj)
{
    ndsP4BanjoNSPShootGroundToAir(fighter_gobj, BANJO_STATUS_NSPA_FORWARD);
}

static void ndsP4BanjoNSPShootBackwardGroundToAir(GObj *fighter_gobj)
{
    ndsP4BanjoNSPShootGroundToAir(fighter_gobj, BANJO_STATUS_NSPA_BACKWARD);
}

/* BanjoNSP.ground_shoot_forward_collision_ (0xE2 map). */
void ndsP4BanjoNSPShootForwardGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4BanjoNSPShootForwardGroundToAir);
}

/* BanjoNSP.ground_shoot_backward_collision_ (0xE3 map). */
void ndsP4BanjoNSPShootBackwardGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4BanjoNSPShootBackwardGroundToAir);
}

/* BanjoNSP.air_shoot_forward_collision_ (0xE5 map). */
void ndsP4BanjoNSPShootForwardAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterCliff(fighter_gobj, ndsP4BanjoNSPShootForwardAirToGround);
}

/* BanjoNSP.air_shoot_backward_collision_ (0xE6 map). */
void ndsP4BanjoNSPShootBackwardAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterCliff(fighter_gobj, ndsP4BanjoNSPShootBackwardAirToGround);
}

/* ---- Up special (the Beak Bomb) ---- */

/* BanjoUSP.initial_ (ground_usp, air_usp): airborne, Captain's Falcon Dive
 * status vars on the status change (its proc_status: the jumps spent, its
 * timer), the taps buffer clear. The events' object comes back through
 * ftMainSetStatus' home slot for its first argument. */
void ndsP4BanjoUSPInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga != nMPKineticsGround)
    {
        *ndsP4BanjoStatusS32(fp, 5) = FALSE;
    }
    else
    {
        *ndsP4BanjoStatusS32(fp, 5) = TRUE;
        mpCommonSetFighterAir(fp);
    }
    fp->proc_status = ftCaptainSpecialHiProcStatus;
    ftMainSetStatus(fighter_gobj, BANJO_STATUS_USP_BEGIN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ndsP4BanjoClearFlags(fp);
    *ndsP4BanjoStatusS32(fp, 0) = 0;
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* BanjoUSP.usp_transition_attack_. */
static void ndsP4BanjoUSPAttackSetStatus(GObj *fighter_gobj)
{
    ndsP4BanjoClearFlags(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, BANJO_STATUS_USP_ATTACK, 0.0F, 1.0F, BANJO_PRESERVE_USP_ATTACK);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftFoxSpecialHiHoldInitStatusVars(fighter_gobj);
}

/* BanjoUSP.begin_main_ (0xEF update): a B tap any time in the begin
 * attacks once past frame 10; at its end, a special fall. */
void ndsP4BanjoUSPBeginMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *buffer = ndsP4BanjoStatusS32(fp, 0);
    f32 anim_frame = fighter_gobj->anim_frame;
    s32 taps = (s32)(u8)(fp->input.pl.button_tap >> 8) | *buffer;

    *buffer = taps;

    if (!(anim_frame <= BANJO_USP_ATTACK_FRAME) && (taps & BANJO_USP_TAP_B))
    {
        ndsP4BanjoUSPAttackSetStatus(fighter_gobj);
        return;
    }
    if (anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, BANJO_USP_FALL_DRIFT, FALSE, TRUE, FALSE,
                                     BANJO_USP_BEGIN_FALL_LANDING, FALSE);
    }
}

/* BanjoUSP.attack_end_initial_. */
static void ndsP4BanjoUSPAttackEndInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, BANJO_STATUS_USP_ATTACK_END, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->physics.vel_air.x *= BANJO_USP_ATTACK_END_MUL;
}

/* BanjoUSP.attack_main_ (0xF0 update): the end at frame 39 exactly. */
void ndsP4BanjoUSPAttackMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame == BANJO_USP_ATTACK_END_FRAME)
    {
        ndsP4BanjoUSPAttackEndInitial(fighter_gobj);
    }
}

/* BanjoUSP.attack_end_main_ (0xF1 update). */
void ndsP4BanjoUSPAttackEndMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, BANJO_USP_FALL_DRIFT, FALSE, TRUE, FALSE,
                                     BANJO_USP_FALL_LANDING, FALSE);
    }
}

/* BanjoUSP.begin_recoil_. */
static void ndsP4BanjoUSPRecoilSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
    }
    ftMainSetStatus(fighter_gobj, BANJO_STATUS_USP_RECOIL, 0.0F, 1.0F, BANJO_PRESERVE_USP_ATTACK);
}

/* BanjoUSP.attack_interupt_ (0xF0 interrupt): the script's turn window
 * (temp variable 2 == 2, Captain's Falcon Dive turn), then check_recoil_:
 * any contact (temp variable 1, gNdsP4BanjoOverrides) bounces him back. */
void ndsP4BanjoUSPAttackInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 == 2)
    {
        ftCaptainSpecialHiProcInterrupt(fighter_gobj);
    }
    if (fp->motion_vars.flags.flag0 != 0)
    {
        ndsP4BanjoUSPRecoilSetStatus(fighter_gobj);
        fp->motion_vars.flags.flag1 = 0;
        fp->physics.vel_air.x = BANJO_USP_RECOIL_X_SPEED * fp->lr;
        fp->physics.vel_air.y = BANJO_USP_RECOIL_Y_SPEED;
    }
}

/* BanjoUSP.recoil_move_ (0xF2 interrupt): slows and floats until the
 * script sets temp variable 1. */
void ndsP4BanjoUSPRecoilMove(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag0 == 0)
    {
        fp->physics.vel_air.x *= BANJO_USP_RECOIL_X_MUL;
        fp->physics.vel_air.y += BANJO_USP_RECOIL_Y_ADD;
    }
}

/* The begin's and the attack's gravity. Their donors then test v0 for the
 * air friction, which still holds the physics routine's own address there:
 * neither applies it (nor the attack its deceleration clamp). */
static void ndsP4BanjoUSPApplyGravity(FTStruct *fp, FTAttributes *attr)
{
    if (fp->is_fastfall)
    {
        ftPhysicsApplyFastFall(fp, attr);
    }
    else ftPhysicsApplyGravityDefault(fp, attr);
}

/* BanjoUSP.begin_physics_ (0xEF physics): held still, then the rise
 * (higher from a grounded start), spending the mid-air jumps. */
void ndsP4BanjoUSPBeginPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ndsP4BanjoUSPApplyGravity(fp, attr);

    if (fp->motion_vars.flags.flag2 == BANJO_USP_BEGIN)
    {
        fp->physics.vel_air.x = 0.0F;
        fp->physics.vel_air.y = 0.0F;
    }
    if (fp->motion_vars.flags.flag2 == BANJO_USP_BEGIN_MOVE)
    {
        fp->physics.vel_air.y = (*ndsP4BanjoStatusS32(fp, 5) == FALSE) ? BANJO_USP_Y_SPEED_AIR
                                                                      : BANJO_USP_Y_SPEED;
        fp->motion_vars.flags.flag2 = BANJO_USP_MOVE;
        fp->jumps_used = attr->jumps_max;
    }
    ftPhysicsClampAirVelXStickDefault(fp, attr);
}

/* BanjoUSP.attack_physics_ (0xF0 physics): held, reared back, then
 * forward at 80 with gravity cancelled. */
void ndsP4BanjoUSPAttackPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ndsP4BanjoUSPApplyGravity(fp, attr);

    if (fp->motion_vars.flags.flag2 == BANJO_USP_ATTACK_BEGIN)
    {
        fp->physics.vel_air.x = 0.0F;
        fp->physics.vel_air.y = 0.0F;
    }
    if (fp->motion_vars.flags.flag2 == BANJO_USP_ATTACK_REAR_BACK)
    {
        fp->physics.vel_air.y = 0.0F;
        fp->physics.vel_air.x = (f32)fp->lr * BANJO_USP_X_SPEED_BACK;
    }
    else
    {
        if (fp->motion_vars.flags.flag2 == BANJO_USP_ATTACK_BEGIN_MOVE)
        {
            fp->physics.vel_air.y = BANJO_USP_Y_SPEED_ATTACK;
            fp->physics.vel_air.x = (f32)fp->lr * BANJO_USP_X_SPEED_ATTACK;
            fp->motion_vars.flags.flag2 = BANJO_USP_ATTACK_MOVE;
        }
        if (fp->motion_vars.flags.flag2 != BANJO_USP_ATTACK_MOVE)
        {
            return;
        }
    }
    fp->physics.vel_air.y += attr->gravity;
}

/* BanjoUSP.recoil_physics_ (0xF2 physics): drift once the script sets
 * temp variable 1. */
void ndsP4BanjoUSPRecoilPhysics(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->motion_vars.flags.flag0 != 0)
    {
        ftPhysicsApplyAirVelDrift(fighter_gobj);
    }
    else ftPhysicsApplyAirVelFriction(fighter_gobj);
}

/* BanjoUSP.splat_physics_ (0xF3 physics): temp variable 3 pushes him off
 * the wall. */
void ndsP4BanjoUSPSplatPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag2 != 0)
    {
        fp->motion_vars.flags.flag2 = 0;
        fp->physics.vel_air.x = BANJO_USP_SPLAT_X_SPEED * fp->lr;
    }
    ndsP4BanjoUSPRecoilPhysics(fighter_gobj);
}

/* Mario's Super Jump Punch map with the Beak Bomb's landing lag. */
static void ndsP4BanjoUSPMapLag(GObj *fighter_gobj, u32 landing_lag)
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
            else ftCommonLandingFallSpecialSetStatus(fighter_gobj, FALSE, ndsP4BanjoBitsToF32(landing_lag));
        }
    }
    else mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
}

/* BanjoUSP.begin_collision_ (0xEF map). */
void ndsP4BanjoUSPBeginMap(GObj *fighter_gobj)
{
    ndsP4BanjoUSPMapLag(fighter_gobj, BANJO_USP_BEGIN_LANDING_LAG);
}

/* BanjoUSP.collision_ (0xF1/0xF3 map). */
void ndsP4BanjoUSPMap(GObj *fighter_gobj)
{
    ndsP4BanjoUSPMapLag(fighter_gobj, BANJO_USP_LANDING_LAG);
}

/* BanjoUSP.wall_splat_initial_: Ness's PK Thunder wall bounce's effects
 * (Remix's Size.asm hook there only scales the wave). */
static void ndsP4BanjoUSPWallSplatSetStatus(GObj *fighter_gobj, Vec3f *angle, Vec3f *pos)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, BANJO_STATUS_USP_WALL_SPLAT, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    efManagerImpactWaveMakeEffect(pos, SYVECTOR_AXIS_Z, syUtilsArcTan2(-angle->x, angle->y));
    efManagerQuakeMakeEffect(2);
    fp->physics.vel_air.y = BANJO_USP_SPLAT_Y_SPEED;
    fp->physics.vel_air.x = 0.0F;
    ndsP4BanjoClearFlags(fp);
}

/* BanjoUSP.attack_collision_ (0xF0 map): the map, then a wall ahead in
 * full flight (last frame's walls) splats him against it, his top joint
 * tilted to the wall's angle. */
void ndsP4BanjoUSPAttackMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *topn;
    Vec3f *angle;
    Vec3f pos;

    ndsP4BanjoUSPMap(fighter_gobj);

    if (fp->motion_vars.flags.flag2 != BANJO_USP_ATTACK_MOVE)
    {
        return;
    }
    if (fp->lr >= 0)
    {
        if (!(fp->coll_data.mask_prev & MAP_FLAG_LWALL))
        {
            return;
        }
        angle = &fp->coll_data.lwall_angle;
    }
    else
    {
        if (!(fp->coll_data.mask_prev & MAP_FLAG_RWALL))
        {
            return;
        }
        angle = &fp->coll_data.rwall_angle;
    }
    topn = DObjGetStruct(fighter_gobj);
    pos.x = topn->translate.vec.f.x;
    pos.y = topn->translate.vec.f.y;
    pos.z = 0.0F;
    ndsP4BanjoUSPWallSplatSetStatus(fighter_gobj, angle, &pos);
    DObjGetStruct(fighter_gobj)->rotate.vec.f.x = angle->y;
}

/* BanjoUSP.wall_splat_main_ (0xF3 update): a special fall at the end; the
 * script's temp variable 1 eases the tilt back. */
void ndsP4BanjoUSPWallSplatMain(GObj *fighter_gobj)
{
    FTStruct *fp;
    DObj *topn;
    f32 rotate_x;

    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, BANJO_USP_FALL_DRIFT, FALSE, TRUE, FALSE,
                                     BANJO_USP_BEGIN_FALL_LANDING, FALSE);
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    topn = DObjGetStruct(fighter_gobj);
    rotate_x = topn->rotate.vec.f.x * BANJO_USP_SPLAT_ROTATE_MUL;

    if (fp->motion_vars.flags.flag0 != 0)
    {
        topn->rotate.vec.f.x = rotate_x;
    }
}

/* ---- Down special (Beak Barge, Bill Drill) ---- */

/* BanjoDSP.ground_initial_ (ground_dsp). */
void ndsP4BanjoDSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, BANJO_STATUS_DSPG, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = BANJO_DSP_BEGIN;
}

/* BanjoDSP.air_initial_ (air_dsp). */
void ndsP4BanjoDSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, BANJO_STATUS_DSPA, 0.0F, 1.0F, FTSTATUS_PRESERVE_FASTFALL);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = BANJO_DSP_BEGIN;
    ndsP4BanjoClearFastFall(fp);
}

/* BanjoDSP.loop_initial_. */
static void ndsP4BanjoDSPLoopInitial(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, BANJO_STATUS_DSPA_LOOP, 0.0F, 1.0F, FTSTATUS_PRESERVE_HIT);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ndsP4BanjoClearFlags(ftGetStruct(fighter_gobj));
}

/* BanjoDSP.aerial_main_ (0xE7 update). */
void ndsP4BanjoDSPAerialMain(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4BanjoDSPLoopInitial);
}

/* BanjoDSP.air_move_ (0xE7/0xEB interrupt): the begin slows his rise, the
 * rest his drift. */
void ndsP4BanjoDSPAirMove(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag2 == BANJO_DSP_BEGIN)
    {
        fp->physics.vel_air.y *= BANJO_DSP_BEGIN_Y_MUL;
    }
    else fp->physics.vel_air.x *= BANJO_DSP_X_MUL;
}

/* BanjoDSP.physics_ (0xE7/0xEB physics): no drift once the script sets
 * temp variable 2; then a slow rise, or the drill's dive. */
void ndsP4BanjoDSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 != 0)
    {
        ftPhysicsApplyAirVelFriction(fighter_gobj);
    }
    else ftPhysicsApplyAirVelDrift(fighter_gobj);

    if ((fp->status_id == BANJO_STATUS_DSPA_LOOP) || (fp->motion_vars.flags.flag2 == BANJO_DSP_MOVE))
    {
        fp->physics.vel_air.y = BANJO_DSP_Y_SPEED;
    }
    else fp->physics.vel_air.y = BANJO_DSP_RISE_Y_SPEED;
}

/* BanjoDSP.begin_landing_: Captain's Falcon Kick landing, his status. */
static void ndsP4BanjoDSPLandingSetStatus(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, BANJO_STATUS_DSP_LAND, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

/* BanjoDSP.collision_ (0xE7/0xEB map): the drill lands into its landing,
 * the begin lands as any aerial. */
void ndsP4BanjoDSPMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
    }
    else if ((fp->motion_vars.flags.flag2 == BANJO_DSP_MOVE) ||
             (fp->status_id == BANJO_STATUS_DSPA_LOOP))
    {
        mpCommonProcFighterCliff(fighter_gobj, ndsP4BanjoDSPLandingSetStatus);
    }
    else mpCommonProcFighterCliffWaitOrLanding(fighter_gobj);
}

/* BanjoDSP.action_change_: Captain's Falcon Kick air status, his; the copy
 * restores the top joint's z rotation only (the source's TransN store is
 * not in it). */
static void ndsP4BanjoDSPGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 rotate_z = fp->joints[nFTPartsJointTopN]->rotate.vec.f.z;

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, BANJO_STATUS_DSPG_AIR, 0.0F, 1.0F, FTSTATUS_PRESERVE_EFFECT);
    fp->joints[nFTPartsJointTopN]->rotate.vec.f.z = rotate_z;
    fp->proc_lagstart = ftParamProcPauseEffect;
    fp->proc_lagend = ftParamProcResumeEffect;
}

/* BanjoDSP.grounded_collision_ (0xE9 map): Captain's Falcon Kick map with
 * status_check_, his air transition. */
void ndsP4BanjoDSPGroundMap(GObj *fighter_gobj)
{
    FTStruct *fp;

    ftCaptainSpecialLwDecideMapCollide(fighter_gobj);

    if (ftCaptainSpecialLwBoundCheckGoto(fighter_gobj) != FALSE)
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);

    if ((fp->motion_vars.flags.flag1 == 1) && (fp->ga == nMPKineticsAir))
    {
        ndsP4BanjoDSPGroundToAir(fighter_gobj);
    }
}

/* Wario.asm body_slam_recoil_, at ftMainSetHitInteractStats' head: any
 * contact in the Beak Bomb's attack marks the recoil. */
static void ndsP4BanjoOnHitInteract(FTStruct *fp, s32 attack_type)
{
    (void)attack_type;

    if (fp->status_id == BANJO_STATUS_USP_ATTACK)
    {
        fp->motion_vars.flags.flag0 = 1;
    }
}

const NDSP4Overrides gNdsP4BanjoOverrides = {
    .on_hit_interact = ndsP4BanjoOnHitInteract,
};

#endif /* NDS_P4_BANJO */
