/*
 * P4 MRGAW (EXTRA's 2D Mr. Game & Watch): native ports of the donor's
 * special routines and of the patches EXTRA makes on his character id.
 * Source: joaorb64/smashremix-plus-extra 96621af,
 * extra_characters/MRGAW/GameAndWatchSpecial.asm and main.asm, read as
 * assembled (scripts/p4/mipsdis.py). He runs Mario's status code
 * (fp->fkind == nFTKindMario): Chef (his neutral special) on Mario's
 * fireball statuses with a sausage of his own, Fire (up) a jump of his own,
 * Judge (down) a random hammer of nine.
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x0024 action                          status_id
 *   0x0044 / 0x0048 / 0x004C               lr / vel_air.x / .y
 *   0x0078 position (a pointer)            coll_data.p_translate
 *   0x0148 / 0x014C                        jumps_used / ga
 *   0x0178 "temp variable 1"               card_anim_frame_id
 *   0x017C/0x180/0x184 temp variables 1-3  motion_vars.flags.flag0/1/2
 *   0x01BC / 0x01BE / 0x01C2               input.pl.button_hold /
 *                                          button_tap / stick_range.x
 *   0x0294 + 0xC4 n                        attack_colls[n] (its records'
 *                                          flags: hurt, shield, reflect,
 *                                          absorb)
 *   0x08E8 / 0x0928 / 0x092C / 0x0958      joints[0] / [16] / [17] / [28]
 *   0x09C8 (+0x64)                         attr (jumps_max)
 *   0x0ADC up special bool                 passive_vars word 0 (Mario's
 *                                          landing case clears it)
 *   0x0AE4 / 0x0AE8 firemen origin         passive_vars words 2 / 3
 *   0x0B18 / 0x0B1C                        status_vars words 0 / 1
 *
 * His sausage is his own weapon on his special file 1 (MRGAW file 6,
 * MRGAW_FILE_PROJECTILE_HITBOX, its graphic in his special file 3).
 */
#include <nds/nds_p4.h>

#if NDS_P4_MRGAW

#include <ef/effect.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <it/item.h>
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
sb32 itLGunWeaponAmmoProcMap(GObj *weapon_gobj);
sb32 itLGunWeaponAmmoProcHit(GObj *weapon_gobj);
void mpCommonRunItemCollisionDefault(GObj *item_gobj, Vec3f *pos, MPCollData *coll_data);
s32 syUtilsRandIntRange(s32 range);
u16 syUtilsRandUShort(void);
f32 __sinf(f32);
f32 __cosf(f32);

static f32 ndsP4MrgawBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

static s32 *ndsP4MrgawPassiveWord(FTStruct *fp, u32 index)
{
    return &((s32 *)(void *)&fp->passive_vars)[index];
}

static s32 *ndsP4MrgawStatusWord(FTStruct *fp, u32 index)
{
    return &((s32 *)(void *)&fp->status_vars)[index];
}

/* main.asm's Action scope and his added actions, as exported. */
#define MRGAW_STATUS_APPEAR1 0xDD
#define MRGAW_STATUS_APPEAR2 0xDE
#define MRGAW_STATUS_CHEF 0xDF
#define MRGAW_STATUS_FIRE 0xE1
#define MRGAW_STATUS_JUDGE_BEGIN 0xE3
#define MRGAW_STATUS_JUDGE_BEGIN_AIR 0xE4
#define MRGAW_STATUS_JUDGE1 0xE5             /* DSP_Ground_1; _Air_n = +1 */
#define MRGAW_STATUS_JUDGE1_AIR 0xE6
#define MRGAW_STATUS_JUDGE7 0xF1
#define MRGAW_STATUS_JUDGE7_AIR 0xF2

#define MRGAW_CONTENT 17u                    /* scripts/p4/contents.json */
#define MRGAW_BUTTON_B 0x4000u               /* Joypad.B */
#define MRGAW_JOINT_WEAPON 17                /* 0x08F8 + 0xD * 4 */
#define MRGAW_JOINT_GRAB 28                  /* 0x08F8 + 0x18 * 4 */
#define MRGAW_JOINT_CHEF 16                  /* 0x0928 */

/* ---- 2D facing (GameAndWatchScale.adjust_scale_for_direction_) ---- */

/* ftMainProcPhysicsMap's head, every frame but his two entry actions: the
 * top joint, the weapon joint and (when there is one) the grab joint face
 * him by the sign of their x scale, the opposite of his direction. */
static void ndsP4MrgawBeforePhysicsMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 sign;
    DObj *joint;

    if ((fp->status_id == MRGAW_STATUS_APPEAR1) || (fp->status_id == MRGAW_STATUS_APPEAR2))
    {
        return;
    }
    sign = -(f32)fp->lr;

    joint = fp->joints[nFTPartsJointTopN];
    joint->scale.vec.f.x = ((joint->scale.vec.f.x < 0.0F) ? -joint->scale.vec.f.x :
                                                             joint->scale.vec.f.x) * sign;
    joint = fp->joints[MRGAW_JOINT_WEAPON];
    if (joint != NULL)
    {
        joint->scale.vec.f.x = ((joint->scale.vec.f.x < 0.0F) ? -joint->scale.vec.f.x :
                                                                 joint->scale.vec.f.x) * sign;
    }
    joint = fp->joints[MRGAW_JOINT_GRAB];
    if (joint != NULL)
    {
        joint->scale.vec.f.x = ((joint->scale.vec.f.x < 0.0F) ? -joint->scale.vec.f.x :
                                                                 joint->scale.vec.f.x) * sign;
    }
}

/* ---- Neutral special (Chef) ---- */

#define MRGAW_CHEF_RELEASE_FRAME 16.0F       /* RELEASE_FRAME 0x4180 */
#define MRGAW_CHEF_SPAWN_X 128.0F            /* lui 0x4300 */
#define MRGAW_CHEF_LIFETIME 110
#define MRGAW_CHEF_ROTATE_SPEED 0.125F
#define MRGAW_CHEF_ANGLE 1.1072F
#define MRGAW_CHEF_ANGLE_UP 0.3125F          /* lui 0x3EA0 */
#define MRGAW_CHEF_ANGLE_DOWN 0.375F         /* lui 0x3EC0 */
#define MRGAW_CHEF_SPEED 40.0F
#define MRGAW_CHEF_FALL (-1.03125F)          /* lui 0xBF84 */
#define MRGAW_CHEF_SCALE 0.75F               /* SAUSAGE_SCALE 0x3F40 */
#define MRGAW_CHEF_FGM 0x22                  /* medium punch */
#define MRGAW_CHEF_BKB 37
#define MRGAW_CHEF_KBS 35
#define MRGAW_CHEF_KB_ANGLE 80
#define MRGAW_CHEF_DAMAGE 5
#define MRGAW_CHEF_REFLECT_MUL 1.5707964F    /* 0x3FC90FDB */
#define MRGAW_CHEF_REFRESH_PUSH 28.0F        /* REFRESH_FRAME_PUSH 0x41E0 */
#define MRGAW_CHEF_REFRESH_HOLD 42.0F        /* REFRESH_FRAME_HOLD 0x4228 */
#define MRGAW_CHEF_REFRESH_START 12.0F       /* REFRESH_START 0x4140 */
#define MRGAW_CHEF_MAX_LOOPS 4

extern void *gNdsP4MrgawSpecial1;

/* Makers that found his sausage file missing: counted, never a fault. */
__attribute__((used)) volatile u32 gNdsP4MrgawArticleMisses;

/* GameAndWatchNSP.blaster_duration: gravity of its own, a spin and the
 * sausage's scale every frame. */
static sb32 ndsP4MrgawChefProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    DObj *dobj;

    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        return TRUE;
    }
    wp->physics.vel_air.y += MRGAW_CHEF_FALL;

    dobj = DObjGetStruct(weapon_gobj);
    dobj->rotate.vec.f.x += MRGAW_CHEF_ROTATE_SPEED;
    dobj->scale.vec.f.x = MRGAW_CHEF_SCALE;
    dobj->scale.vec.f.y = MRGAW_CHEF_SCALE;

    return FALSE;
}

/* wpBossBulletProcHop (Master Hand's bullet off a shield). */
static sb32 ndsP4MrgawChefProcHop(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    syVectorRotateAbout3D(&wp->physics.vel_air, &wp->shield_collide_dir,
                          wp->shield_collide_angle * 2);
    wpMainReflectorRotateWeaponModel(weapon_gobj);

    return FALSE;
}

/* GameAndWatchNSP.blaster_reflection: a fresh lifetime, its speed times
 * pi/2 when the reflector is MRGAW himself (the caller's FTStruct, v0), the
 * fireball's direction, and its model's y rotation set by its course. */
static sb32 ndsP4MrgawChefProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);
    f32 mul = (fp->nds_p4_content == MRGAW_CONTENT) ? MRGAW_CHEF_REFLECT_MUL : 1.0F;

    *(f32 *)(void *)&wp->weapon_vars = mul;
    wp->lifetime = MRGAW_CHEF_LIFETIME;
    wp->physics.vel_air.x *= mul;

    wpMainReflectorSetLR(wp, fp);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.y =
        (0.0F <= wp->physics.vel_air.x) ? MRGAW_CHEF_REFLECT_MUL : -MRGAW_CHEF_REFLECT_MUL;

    return FALSE;
}

static WPDesc sNdsP4MrgawChefWeaponDesc = {
    0x00,
    NDS_P4_WP_KIND_MRGAW_CHEF,
    &gNdsP4MrgawSpecial1,
    0x0,
    { nGCMatrixKindTra, 0x48, 0 },
    ndsP4MrgawChefProcUpdate,
    itLGunWeaponAmmoProcMap,
    itLGunWeaponAmmoProcHit,
    itLGunWeaponAmmoProcHit,
    ndsP4MrgawChefProcHop,
    itLGunWeaponAmmoProcHit,
    ndsP4MrgawChefProcReflector,
    itLGunWeaponAmmoProcHit
};

/* GameAndWatchNSP.projectile_stage_setting: the sausage at the chef's
 * joint, 40 along 1.1072 radians, steeper with the stick back (+0.3125),
 * flatter with it forward (-0.375), 5 damage, 80 degrees, 35 growth and 37
 * base, the medium punch sound. */
static GObj *ndsP4MrgawChefMakeWeapon(GObj *fighter_gobj, Vec3f *pos)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *weapon_gobj;
    WPStruct *wp;
    f32 stick;
    f32 angle;

    if (gNdsP4MrgawSpecial1 == NULL)
    {
        gNdsP4MrgawArticleMisses++;
        return NULL;
    }
    weapon_gobj = wpManagerMakeWeapon(fighter_gobj, &sNdsP4MrgawChefWeaponDesc, pos,
                                      WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER);
    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);
    *(f32 *)(void *)&wp->weapon_vars = 1.0F;
    wp->lifetime = MRGAW_CHEF_LIFETIME;
    wp->attack_coll.fgm_id = MRGAW_CHEF_FGM;
    wp->attack_coll.knockback_base = MRGAW_CHEF_BKB;
    wp->attack_coll.knockback_scale = MRGAW_CHEF_KBS;
    wp->attack_coll.angle = MRGAW_CHEF_KB_ANGLE;
    wp->attack_coll.element = nGMHitElementNormal;
    wp->attack_coll.damage = MRGAW_CHEF_DAMAGE;
    wp->physics.vel_air.z = 0.0F;

    stick = (f32)fp->input.pl.stick_range.x * (f32)fp->lr;
    angle = MRGAW_CHEF_ANGLE;
    if (stick < 0.0F)
    {
        angle += MRGAW_CHEF_ANGLE_UP;
    }
    else if (0.0F < stick)
    {
        angle -= MRGAW_CHEF_ANGLE_DOWN;
    }
    wp->physics.vel_air.x = __cosf(angle) * MRGAW_CHEF_SPEED * (f32)fp->lr;
    wp->physics.vel_air.y = __sinf(angle) * MRGAW_CHEF_SPEED;

    if (DObjGetStruct(weapon_gobj)->mobj != NULL)
    {
        DObjGetStruct(weapon_gobj)->mobj->palette_id = 0.0F;
    }
    wpMainVelSetModelPitch(weapon_gobj);

    return weapon_gobj;
}

/* GameAndWatchNSP.main (Chef, both): temp variable 1 from the script throws
 * a sausage from 128 ahead of joint 16; the end clears temp variable 3 and
 * waits or falls. */
void ndsP4MrgawNSPMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag0 != 0)
    {
        Vec3f pos;

        fp->motion_vars.flags.flag0 = 0;
        pos.x = MRGAW_CHEF_SPAWN_X;
        pos.y = 0.0F;
        pos.z = 0.0F;
        gmCollisionGetFighterPartsWorldPosition(fp->joints[MRGAW_JOINT_CHEF], &pos);
        (void)ndsP4MrgawChefMakeWeapon(fighter_gobj, &pos);
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        fp->motion_vars.flags.flag2 = 0;
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* GameAndWatchNSP.chef_refresh_ (interrupt, both): up to four more cooks,
 * B pressed after frame 28 up to 42 or held on frame 42 restarts the action
 * at frame 12. */
void ndsP4MrgawNSPRefresh(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 frame = fighter_gobj->anim_frame;

    if ((MRGAW_CHEF_MAX_LOOPS - (s32)fp->motion_vars.flags.flag1) <= 0)
    {
        return;
    }
    if (!(MRGAW_CHEF_REFRESH_PUSH < frame) || !(frame <= MRGAW_CHEF_REFRESH_HOLD))
    {
        return;
    }
    if (!(fp->input.pl.button_tap & MRGAW_BUTTON_B) &&
        ((frame != MRGAW_CHEF_REFRESH_HOLD) || !(fp->input.pl.button_hold & MRGAW_BUTTON_B)))
    {
        return;
    }
    ftMainSetStatus(fighter_gobj, fp->status_id, MRGAW_CHEF_REFRESH_START, 1.0F, 0);
    fp->motion_vars.flags.flag1++;
}

/* GameAndWatchNSP.air_to_ground_: grounded Chef from the same frame,
 * keeping its hitbox. */
static void ndsP4MrgawNSPAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, MRGAW_STATUS_CHEF, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_HIT);
}

/* GameAndWatchNSP.air_collision_ (ChefAir map): past frame 16 with B let go
 * he lands normally (temp variable 3 cleared), else into grounded Chef. */
void ndsP4MrgawNSPAirMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    void (*proc)(GObj *) = ndsP4MrgawNSPAirToGround;

    if ((MRGAW_CHEF_RELEASE_FRAME <= fighter_gobj->anim_frame) &&
        !(fp->input.pl.button_hold & MRGAW_BUTTON_B))
    {
        fp->motion_vars.flags.flag2 = 0;
        proc = mpCommonSetFighterWaitOrLanding;
    }
    mpCommonProcFighterLanding(fighter_gobj, proc);
}

/* ---- Up special (Fire) ---- */

#define MRGAW_USP_BEGIN 1
#define MRGAW_USP_BEGIN_MOVE 2
#define MRGAW_USP_MOVE 3
#define MRGAW_USP_END_MOVE 4
#define MRGAW_USP_END 5
#define MRGAW_USP_Y_SPEED 216.0F             /* AIR/GROUND_Y_SPEED 0x4358 */
#define MRGAW_USP_X_SPEED 0.1875F            /* X_SPEED 0x3E40 */
#define MRGAW_USP_X_SPEED_INIT 0.5625F       /* X_SPEED_INIT 0x3F10 */
#define MRGAW_USP_GRAVITY 7.0F               /* GRAV_ACCELERATION 0x40E0 */
#define MRGAW_USP_BEGIN_SLOW_X 0.125F        /* lui 0x3E00 */
#define MRGAW_USP_BEGIN_SLOW_Y 0.5F          /* END_AIR_ACCELERATION 0x3F00 */
#define MRGAW_USP_STICK_MIN 8.0F             /* lui 0x4100 */
#define MRGAW_USP_Y_FROM_X 0.0625F           /* lui 0x3D80 */
#define MRGAW_USP_DRIFT 0.65625F             /* lui 0x3F28 */
#define MRGAW_USP_FALL_LANDING 0.25F         /* LANDING_FSM 0x3E80 */
/* collision_ keeps the low half of Mario's constant (0x3E8F5C29). */
#define MRGAW_USP_LANDING_BITS 0x3E805C29u

/* GameAndWatchUSP.ground_initial_ / air_initial_ (the same code): once per
 * airtime (passive word 0), Fire with temp variable 3 at BEGIN, and the top
 * joint's position kept for the firemen. */
static void ndsP4MrgawUSPInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *topn;

    if (*ndsP4MrgawPassiveWord(fp, 0) != 0)
    {
        return;
    }
    *ndsP4MrgawPassiveWord(fp, 0) = TRUE;
    fp->motion_vars.flags.flag2 = MRGAW_USP_BEGIN;

    ftMainSetStatus(fighter_gobj, MRGAW_STATUS_FIRE, 0.0F, 1.0F, 0);
    ftMainPlayAnimEventsAll(fighter_gobj);

    fp = ftGetStruct(fighter_gobj);
    topn = fp->joints[nFTPartsJointTopN];
    if (topn != NULL)
    {
        *(f32 *)ndsP4MrgawPassiveWord(fp, 2) = topn->translate.vec.f.x;
        *(f32 *)ndsP4MrgawPassiveWord(fp, 3) = topn->translate.vec.f.y;
    }
}

void ndsP4MrgawUSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4MrgawUSPInitial(fighter_gobj);
}

void ndsP4MrgawUSPAirInitial(GObj *fighter_gobj)
{
    ndsP4MrgawUSPInitial(fighter_gobj);
}

/* GameAndWatchUSP.main_air_ (Fire, both): the movement flag and the up
 * special bool set; aerial jumps spent unless status word 1 is set (as
 * assembled: a word no Fire routine writes); special fall at the end. */
void ndsP4MrgawUSPMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    *ndsP4MrgawStatusWord(fp, 0) = TRUE;
    *ndsP4MrgawPassiveWord(fp, 0) = TRUE;
    if (*ndsP4MrgawStatusWord(fp, 1) == 0)
    {
        fp->jumps_used = (u8)fp->attr->jumps_max;
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, MRGAW_USP_DRIFT, TRUE, TRUE, FALSE,
                                     MRGAW_USP_FALL_LANDING, FALSE);
    }
}

/* GameAndWatchUSP.change_direction_ (interrupt): Falcon Dive's turn when the
 * script sets temp variable 2 to 2; then the grab joint (the firemen, a child
 * of the top joint) is held where Fire began. */
void ndsP4MrgawUSPChangeDirection(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *grab;
    DObj *topn;
    f32 scale;

    if (fp->motion_vars.flags.flag1 == 2)
    {
        ftCaptainSpecialHiProcInterrupt(fighter_gobj);
    }
    fp = ftGetStruct(fighter_gobj);
    grab = fp->joints[MRGAW_JOINT_GRAB];
    topn = fp->joints[nFTPartsJointTopN];
    if ((grab == NULL) || (topn == NULL))
    {
        return;
    }
    scale = topn->scale.vec.f.y;
    grab->translate.vec.f.x =
        (((topn->translate.vec.f.x - *(f32 *)ndsP4MrgawPassiveWord(fp, 2)) * (f32)fp->lr) /
         scale) - grab->translate.vec.f.x;
    grab->translate.vec.f.y =
        ((*(f32 *)ndsP4MrgawPassiveWord(fp, 3) - topn->translate.vec.f.y) / scale) +
        grab->translate.vec.f.y;
}

/* GameAndWatchUSP.physics_: grounded, friction only; in the air the air
 * drift's gravity and x clamp (its stick drift replaced by the clamp again:
 * air_control_ is commented out), then temp variable 3's phases: BEGIN slows
 * him, BEGIN_MOVE launches him once (216 up, less a sixteenth of a forward
 * stick's x; a stick under 8 forward keeps the 8 the test loaded), MOVE and
 * the launch take 7 of gravity, END_MOVE becomes END. */
void ndsP4MrgawUSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    s32 phase;

    if (fp->ga == nMPKineticsGround)
    {
        ftPhysicsApplyGroundVelFriction(fighter_gobj);
        return;
    }
    (fp->is_fastfall) ? ftPhysicsApplyFastFall(fp, attr) :
                        ftPhysicsApplyGravityDefault(fp, attr);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        (void)ftPhysicsCheckClampAirVelXDecMax(fp, attr);
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
    if (fp->motion_vars.flags.flag2 == MRGAW_USP_BEGIN)
    {
        fp->physics.vel_air.x = (fp->physics.vel_air.x * MRGAW_USP_BEGIN_SLOW_X) +
                                (MRGAW_USP_X_SPEED * (f32)fp->lr);
        fp->physics.vel_air.y *= MRGAW_USP_BEGIN_SLOW_Y;
    }
    phase = fp->motion_vars.flags.flag2;

    if ((phase == MRGAW_USP_MOVE) || (phase == MRGAW_USP_BEGIN_MOVE))
    {
        if (phase == MRGAW_USP_BEGIN_MOVE)
        {
            f32 stick = (f32)fp->input.pl.stick_range.x * (f32)fp->lr;
            f32 vel_x = MRGAW_USP_STICK_MIN;
            f32 vel_y = MRGAW_USP_Y_SPEED;

            if (MRGAW_USP_STICK_MIN <= stick)
            {
                vel_x = stick * MRGAW_USP_X_SPEED_INIT;
                vel_y -= MRGAW_USP_Y_FROM_X * vel_x;
            }
            fp->physics.vel_air.x = (f32)fp->lr * (vel_x + MRGAW_USP_X_SPEED);
            fp->physics.vel_air.y = vel_y;
            fp->motion_vars.flags.flag2 = MRGAW_USP_MOVE;
            fp->jumps_used = (u8)attr->jumps_max;
        }
        fp->physics.vel_air.y -= MRGAW_USP_GRAVITY;
    }
    if (fp->motion_vars.flags.flag2 == MRGAW_USP_END_MOVE)
    {
        fp->motion_vars.flags.flag2 = MRGAW_USP_END;
    }
}

/* GameAndWatchUSP.collision_: Mario's Super Jump map routine with his
 * landing lag. */
void ndsP4MrgawUSPMap(GObj *fighter_gobj)
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
                fighter_gobj, FALSE, ndsP4MrgawBitsToF32(MRGAW_USP_LANDING_BITS));
        }
    }
    else mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
}

/* MRGAWSpecialsRefresh (Character.on_hit: ftParamStopVoiceRunProcDamage's
 * head): a hit gives Fire back. */
static void ndsP4MrgawOnDamage(FTStruct *fp)
{
    *ndsP4MrgawPassiveWord(fp, 0) = FALSE;
}

/* ---- Down special (Judge) ---- */

#define MRGAW_JUDGE_COUNT 9
#define MRGAW_JUDGE1_FRAME 14.0F             /* lui 0x4160 */
#define MRGAW_JUDGE1_DAMAGE 12
#define MRGAW_JUDGE_X_MAX 20.0F              /* X_MAXSPEED 0x41A0 */
#define MRGAW_JUDGE_X_MUL 0.9375F            /* X_MULT 0x3F70 */
#define MRGAW_JUDGE_SWITCH_FLAGS 0x2803u     /* sword trails, 3C FGM, gfx, hitboxes */
#define MRGAW_JUDGE_FOOD_VEL_Y 30.0F         /* lui 0x41F0 */
#define MRGAW_JUDGE_FOOD_ODDS 100

static void ndsP4MrgawDSPInitVars(FTStruct *fp)
{
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

/* GameAndWatchDSP.ground_initial_: hit detection off (status word 0). */
void ndsP4MrgawDSPGroundInitial(GObj *fighter_gobj)
{
    FTStruct *fp;

    ftMainSetStatus(fighter_gobj, MRGAW_STATUS_JUDGE_BEGIN, 0.0F, 1.0F, 0);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp = ftGetStruct(fighter_gobj);
    ndsP4MrgawDSPInitVars(fp);
    *ndsP4MrgawStatusWord(fp, 0) = FALSE;
}

/* GameAndWatchDSP.air_initial_. */
void ndsP4MrgawDSPAirInitial(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, MRGAW_STATUS_JUDGE_BEGIN_AIR, 0.0F, 1.0F, 0);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ndsP4MrgawDSPInitVars(ftGetStruct(fighter_gobj));
}

/* Global.get_random_int_safe_: the seed advanced 1 to 64 times by a frame
 * count, then a draw. Remix counts the audio thread's tics
 * (dSYAudioCurrentTic, netplay lockstep); the DS counts the battle's logic
 * updates, which replays reproduce. */
static s32 ndsP4MrgawRandomSafe(s32 range)
{
    extern u32 dSYTaskmanUpdateCount;
    s32 n = (s32)(dSYTaskmanUpdateCount & 0x3Fu);

    do
    {
        (void)syUtilsRandUShort();
    }
    while (n-- != 0);

    return syUtilsRandIntRange(range);
}

/* GameAndWatchDSP.main (JudgeBegin, both): one of the nine judges, grounded
 * or aerial by his state. */
void ndsP4MrgawDSPMain(GObj *fighter_gobj)
{
    s32 judge = ndsP4MrgawRandomSafe(MRGAW_JUDGE_COUNT);
    s32 status_id;

    if ((judge < 0) || (judge >= MRGAW_JUDGE_COUNT))
    {
        judge = 0;
    }
    status_id = MRGAW_STATUS_JUDGE1 + (judge * 2);
    if (ftGetStruct(fighter_gobj)->ga != nMPKineticsGround)
    {
        status_id++;
    }
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, 0);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* GameAndWatchDSP.ground_to_air_: the aerial judge (id + 1) from the same
 * frame, keeping trails, sounds, effects and hitboxes, then the x clamp. */
static void ndsP4MrgawDSPGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id + 1, fighter_gobj->anim_frame, 1.0F,
                    MRGAW_JUDGE_SWITCH_FLAGS);
    ftPhysicsClampAirVelXMax(fp);
}

/* GameAndWatchDSP.air_to_ground_: the grounded judge (id - 1). */
static void ndsP4MrgawDSPAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id - 1, fighter_gobj->anim_frame, 1.0F,
                    MRGAW_JUDGE_SWITCH_FLAGS);
}

/* GameAndWatchDSP.ground_collision_: off an edge into the aerial judge. */
void ndsP4MrgawDSPGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4MrgawDSPGroundToAir);
}

/* GameAndWatchDSP.air_collision_: landing into the grounded judge. */
void ndsP4MrgawDSPAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4MrgawDSPAirToGround);
}

/* GameAndWatchDSP.air_physics_: the air friction physics, an x speed past
 * 20 cut by a sixteenth a frame. */
void ndsP4MrgawDSPAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 vel_x;

    ftPhysicsApplyAirVelFriction(fighter_gobj);

    vel_x = fp->physics.vel_air.x;
    if (!(((vel_x < 0.0F) ? -vel_x : vel_x) <= MRGAW_JUDGE_X_MAX))
    {
        fp->physics.vel_air.x = vel_x * MRGAW_JUDGE_X_MUL;
    }
}

/* attacks_main_'s hit test: any record of any live hitbox that met a
 * hurtbox, shield, reflector or absorber. */
static sb32 ndsP4MrgawDSPHitSomething(FTStruct *fp)
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

/* Remix's Dango (items/Dango.asm): the Maxim Tomato with a 10% heal. Its
 * attributes (appended to Remix's ITCommonData at 0x1010) and the model they
 * name (in the common items file Remix adds) are his item subset file,
 * special file 4 (generate_p4_fighter.py SUBSET_FILES): attributes at 0. */
#define MRGAW_DANGO_HEAL 10                  /* DAMAGE_RESTORE */

extern void *gNdsP4MrgawSpecial4;
extern LBParticle *efManagerItemSpawnSwirlMakeEffect(Vec3f *pos);
extern sb32 itTomatoFallProcUpdate(GObj *item_gobj);
extern sb32 itTomatoFallProcMap(GObj *item_gobj);
extern void itTomatoDroppedSetStatus(GObj *item_gobj);

/* A Dango maker that found the subset file missing: counted, no item. */
__attribute__((used)) volatile u32 gNdsP4MrgawDangoMisses;

/* Its item_info_array: the Tomato's matrices and routines, the Tomato's
 * status table (0x34-0x9C: 0x801744FC, 0x801744C0/0x80174524,
 * 0x801744C0/0x801745CC). */
static ITDesc sNdsP4MrgawDangoDesc = {
    NDS_P4_IT_KIND_MRGAW_DANGO, &gNdsP4MrgawSpecial4, (const void *)0x0,
    { nGCMatrixKindTraRotRpyR, nGCMatrixKindNull, 0x00 }, nGMAttackStateOff,
    itTomatoFallProcUpdate, itTomatoFallProcMap, NULL, NULL, NULL, NULL, NULL, NULL
};

/* itManagerMakeItemSetupCommon for its id (0x8016EA78, as _create_food calls
 * it): Item.spawn_custom_item_based_on_tomato_, itTomatoMakeItem (US) with
 * the Dango's info array, then the swirl and appear spin its
 * SHOW_GFX_WHEN_SPAWNED asks (extend_item_spawn_gfx_). */
static GObj *ndsP4MrgawMakeDango(Vec3f *pos, Vec3f *vel)
{
    GObj *item_gobj;
    DObj *joint;
    Vec3f translate;
    ITStruct *ip;

    if (gNdsP4MrgawSpecial4 == NULL)
    {
        gNdsP4MrgawDangoMisses++;
        return NULL;
    }
    item_gobj = itManagerMakeItem(NULL, &sNdsP4MrgawDangoDesc, pos, vel,
                                  ITEM_FLAG_PARENT_GROUND);
    if (item_gobj != NULL)
    {
        joint = DObjGetStruct(item_gobj);
        ip = itGetStruct(item_gobj);
        translate = joint->translate.vec.f;

        gcAddXObjForDObjFixed(joint, 0x2E, 0);

        joint->translate.vec.f = translate;

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);

        efManagerItemSpawnSwirlMakeEffect(pos);
        itMainSetAppearSpin(item_gobj, FALSE);
    }
    return item_gobj;
}

/* Item.asm's tables for the Dango: DROP_ITEM is the Tomato's
 * (0x801745FC), no throw or pre-pickup routine, and pickup_main_ heals 10
 * through the Tomato's case. */
static sb32 ndsP4MrgawItem(GObj *item_gobj, s32 which)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->kind != NDS_P4_IT_KIND_MRGAW_DANGO)
    {
        return FALSE;
    }
    if (which == NDS_P4_ITEM_DROPPED)
    {
        itTomatoDroppedSetStatus(item_gobj);
    }
    else if ((which == NDS_P4_ITEM_CONSUME) && (ip->owner_gobj != NULL))
    {
        ndsFTCommonGetEatHeal(ftGetStruct(ip->owner_gobj), item_gobj, MRGAW_DANGO_HEAL);
    }
    return TRUE;
}

/* attacks_main_'s _create_food: a food item over his top joint, rising at
 * 30: a Maxim Tomato one time in a hundred, else Remix's Dango; kept clear
 * of walls as a thrown item is. */
static void ndsP4MrgawDSPMakeFood(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;
    Vec3f vel;
    GObj *item_gobj;

    pos.x = pos.y = pos.z = 0.0F;
    gmCollisionGetFighterPartsWorldPosition(fp->joints[nFTPartsJointTopN], &pos);
    pos.z = 0.0F;
    vel.x = 0.0F;
    vel.y = MRGAW_JUDGE_FOOD_VEL_Y;
    vel.z = 0.0F;
    if (ndsP4MrgawRandomSafe(MRGAW_JUDGE_FOOD_ODDS) == 0)
    {
        item_gobj = itManagerMakeItemSetupCommon(NULL, nITKindTomato, &pos, &vel,
                                                 ITEM_FLAG_PARENT_GROUND);
    }
    else item_gobj = ndsP4MrgawMakeDango(&pos, &vel);
    if (item_gobj != NULL)
    {
        mpCommonRunItemCollisionDefault(item_gobj, fp->coll_data.p_translate, &fp->coll_data);
    }
}

/* GameAndWatchDSP.attacks_main_ (every judge): judge 1 costs him 12 on
 * frame 14; judge 7 makes one food the first frame its hammer has hit
 * something (card_anim_frame_id, Remix's "temp variable 1", marks it); at
 * the end the mark clears and he waits or falls. */
void ndsP4MrgawDSPAttacksMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = fp->status_id;

    if ((status_id == MRGAW_STATUS_JUDGE1) || (status_id == MRGAW_STATUS_JUDGE1_AIR))
    {
        if (fighter_gobj->anim_frame == MRGAW_JUDGE1_FRAME)
        {
            ftParamUpdateDamage(fp, MRGAW_JUDGE1_DAMAGE);
        }
    }
    else if ((status_id == MRGAW_STATUS_JUDGE7) || (status_id == MRGAW_STATUS_JUDGE7_AIR))
    {
        if ((fp->card_anim_frame_id == 0) && (ndsP4MrgawDSPHitSomething(fp) != FALSE))
        {
            ndsP4MrgawDSPMakeFood(fighter_gobj);
            fp->card_anim_frame_id = 1;
        }
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        fp->card_anim_frame_id = 0;
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

#if NDS_P2_KIRBY
static void ndsP4MrgawKirbyOnStatus(GObj *fighter_gobj, s32 status_id);
static void ndsP4MrgawKirbyColAnimEnd(GObj *fighter_gobj);
#endif

const NDSP4Overrides gNdsP4MrgawOverrides = {
    .before_physics_map = ndsP4MrgawBeforePhysicsMap,
    .on_damage = ndsP4MrgawOnDamage,
    .item = ndsP4MrgawItem,
#if NDS_P2_KIRBY
    .kirby_on_status = ndsP4MrgawKirbyOnStatus,
    .kirby_colanim_end = ndsP4MrgawKirbyColAnimEnd,
#endif
};

#if NDS_P2_KIRBY
/* ---- Kirby's copy (owner request 2026-10-12, "Brawl-style") ----
 *
 * EXTRA lets no Kirby copy MRGAW (define_character's bool_inhale_copy is
 * the config's kirby_hat, unset); the owner asked for one. Kirby's Chef
 * (scripts/p4/p4_kirby.py CUSTOM_POWERS): his fireball throw, MRGAW's
 * sausage from his hand at its flag, and Chef's refresh: B pressed past the
 * throw (or held at the end) cooks again from the start, four times more at
 * most. Kirby is drawn black while he holds it (ndsP4MrgawKirbySilhouette).
 *
 * The fireball motion's script (228_KirbyMainMotion.c dKirbyMainMotion_0x1D88)
 * makes the fireball's flash (colour animation nGMColAnimFighterMarioSpecialN,
 * which also ended the black figure), its sparkle and its FGM at frame 16;
 * the owner asked for no fireball sound (BUGS.md 2026-10-12). Chef runs the
 * script's toss alone: the flag at frame 16 (the food) and its rumble. */
#define MRGAW_KIRBY_HAND_JOINT 17   /* FTKIRBY_COPYMARIO_FIREBALL_SPAWN_JOINT */
#define MRGAW_KIRBY_REFRESH_FROM 12.0F
#define MRGAW_KIRBY_TOSS_FRAME 16

static const u32 sNdsP4MrgawKirbyNSPScript[] = {
    NDS_P4_MOTION_EVENT(nFTMotionEventAsyncWait, MRGAW_KIRBY_TOSS_FRAME),
    NDS_P4_MOTION_EVENT(nFTMotionEventSetFlag0, 1),
    NDS_P4_MOTION_EVENT(nFTMotionEventMakeRumble, 6), /* MakeRumble(0, 6) */
    NDS_P4_MOTION_EVENT(nFTMotionEventEnd, 0),
};

/* The status's own script from its frame: the toss, or nothing once past it
 * (a switch between ground and air keeps the frame). */
static void ndsP4MrgawKirbyNSPScript(GObj *fighter_gobj)
{
    ndsP4KirbySetMotionScript(ftGetStruct(fighter_gobj),
                              (fighter_gobj->anim_frame < (f32)MRGAW_KIRBY_TOSS_FRAME) ?
                                  sNdsP4MrgawKirbyNSPScript : NULL);
}

static void ndsP4MrgawKirbyNSPProcStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
}

static void ndsP4MrgawKirbyNSPSetStatus(GObj *fighter_gobj, s32 status_id)
{
    ftGetStruct(fighter_gobj)->proc_status = ndsP4MrgawKirbyNSPProcStatus;
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ndsP4MrgawKirbyNSPScript(fighter_gobj);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

void ndsP4MrgawKirbyNSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4MrgawKirbyNSPSetStatus(fighter_gobj, NDS_P4_KIRBY_STATUS_MRGAW_NSP_GROUND);
}

void ndsP4MrgawKirbyNSPAirInitial(GObj *fighter_gobj)
{
    ndsP4MrgawKirbyNSPSetStatus(fighter_gobj, NDS_P4_KIRBY_STATUS_MRGAW_NSP_AIR);
}

void ndsP4MrgawKirbyNSPUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag0 != 0)
    {
        Vec3f pos = { 0.0F, 0.0F, 0.0F };

        fp->motion_vars.flags.flag0 = 0;
        gmCollisionGetFighterPartsWorldPosition(fp->joints[MRGAW_KIRBY_HAND_JOINT], &pos);
        (void)ndsP4MrgawChefMakeWeapon(fighter_gobj, &pos);
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* The refresh: past frame 12 (the throw), B pressed, or held when the
 * animation ends, restarts the cook; flag1 counts them as MRGAW's does. */
void ndsP4MrgawKirbyNSPRefresh(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 frame = fighter_gobj->anim_frame;
    u32 loops = fp->motion_vars.flags.flag1;

    if ((MRGAW_CHEF_MAX_LOOPS - (s32)loops) <= 0)
    {
        return;
    }
    if (!(MRGAW_KIRBY_REFRESH_FROM < frame) &&
        !((frame <= 0.0F) && (fp->input.pl.button_hold & MRGAW_BUTTON_B)))
    {
        return;
    }
    if (!(fp->input.pl.button_tap & MRGAW_BUTTON_B) &&
        !((frame <= 0.0F) && (fp->input.pl.button_hold & MRGAW_BUTTON_B)))
    {
        return;
    }
    ftMainSetStatus(fighter_gobj, fp->status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ndsP4MrgawKirbyNSPScript(fighter_gobj);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = loops + 1u;
}

static void ndsP4MrgawKirbyNSPSwitchAir(GObj *fighter_gobj)
{
    mpCommonSetFighterAir(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, NDS_P4_KIRBY_STATUS_MRGAW_NSP_AIR, fighter_gobj->anim_frame,
                    1.0F, FTSTATUS_PRESERVE_NONE);
    ndsP4MrgawKirbyNSPScript(fighter_gobj);
}

static void ndsP4MrgawKirbyNSPSwitchGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, NDS_P4_KIRBY_STATUS_MRGAW_NSP_GROUND, fighter_gobj->anim_frame,
                    1.0F, FTSTATUS_PRESERVE_NONE);
    ndsP4MrgawKirbyNSPScript(fighter_gobj);
}

void ndsP4MrgawKirbyNSPGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4MrgawKirbyNSPSwitchAir);
}

void ndsP4MrgawKirbyNSPAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4MrgawKirbyNSPSwitchGround);
}

/* His model has no chef's hat (its Chef variant is a face cutout), so as in
 * Brawl Kirby holding this power is MRGAW's black figure: the colour
 * animation fields a hit flash uses (color1, a full blend to black), set
 * while no colour animation runs -- at every status change and every reset
 * of the colour animation (a flash's end), so it holds the whole time he has
 * the power (owner BUGS.md 2026-10-12: it had come back only at status
 * changes); nds_p4.c ndsP4KirbyClearSilhouette clears it with the power. */
static void ndsP4MrgawKirbySilhouette(GObj *fighter_gobj)
{
    GMColAnim *colanim = &ftGetStruct(fighter_gobj)->colanim;

    if ((colanim->cs[0].p_script == NULL) && (colanim->cs[1].p_script == NULL))
    {
        colanim->is_use_color1 = TRUE;
        colanim->color1.r = colanim->color1.g = colanim->color1.b = 0;
        colanim->color1.a = 0xFF;
        colanim->color1.ir = colanim->color1.ig = colanim->color1.ib = colanim->color1.ia = 0;
    }
}

static void ndsP4MrgawKirbyOnStatus(GObj *fighter_gobj, s32 status_id)
{
    (void)status_id;
    ndsP4MrgawKirbySilhouette(fighter_gobj);
}

/* ftParamResetStatUpdateColAnim's routine for a Kirby holding the power. */
static void ndsP4MrgawKirbyColAnimEnd(GObj *fighter_gobj)
{
    ndsP4MrgawKirbySilhouette(fighter_gobj);
}
#endif /* NDS_P2_KIRBY */

#endif /* NDS_P4_MRGAW */
