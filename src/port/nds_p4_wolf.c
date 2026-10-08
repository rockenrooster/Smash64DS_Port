/*
 * P4 Wolf: native ports of the donor's special routines. Source:
 * JSsixtyfour/smashremix 5e04fe7, src/Wolf/WolfSpecial.asm, read as
 * assembled (scripts/p4/mipsdis.py). Wolf runs Fox's status code
 * (fp->fkind == nFTKindFox).
 *
 * His articles live in his own special files (Remix define_character WOLF
 * files 6-9, loaded into gNdsP4WolfSpecial1-4 by the generator's
 * OWN_SPECIAL_FILES): his shot's attributes (special 1, 0xB5A) and its
 * graphic plus the Fire Wolf slash (special 4, 0xB5B), his reflector
 * (special 2, 0xB76) and the Wolfen (special 3, 0xB77). Remix points Fox's
 * reflector and Arwing makers at them on his character id (Wolf.asm); here
 * that is gNdsP4WolfOverrides.
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

#include <ef/effect.h>
#include <wp/weapon.h>

sb32 ftMarioSpecialHiProcPass(GObj *fighter_gobj);
void ftFoxSpecialHiHoldInitStatusVars(GObj *fighter_gobj);
void ftCaptainSpecialHiProcInterrupt(GObj *fighter_gobj);
void gcDrawDObjDLHead1(GObj *gobj);
void gcDrawDObjTreeForGObj(GObj *gobj);
void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);
void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints, f32 anim_frame);
void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint, f32 anim_frame);
void gcPlayAnimAll(GObj *gobj);
void lbCommonAddDObjAnimJointAll(DObj *root_dobj, AObjEvent32 **anim_joints, f32 anim_frame);
sb32 itLGunWeaponAmmoProcMap(GObj *weapon_gobj);
sb32 itLGunWeaponAmmoProcHit(GObj *weapon_gobj);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);

#ifndef lbRelocGetFileData
#define lbRelocGetFileData(type, file, offset) \
    ((type)((uintptr_t)(file) + (intptr_t)(offset)))
#endif
#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* His own special files (generated, OWN_SPECIAL_FILES). */
extern void *gNdsP4WolfSpecial1;
extern void *gNdsP4WolfSpecial2;
extern void *gNdsP4WolfSpecial3;
extern void *gNdsP4WolfSpecial4;

/* Makers that found a file or a model joint missing: counted, never a
 * fault (a lab probe reads them). */
__attribute__((used)) volatile u32 gNdsP4WolfArticleMisses;

/* ---- Fire Wolf's slash (WolfUSP main_2) ----
 *
 * captainshared.asm slash_anim_struct_WOLF: Falcon Punch's effect on his
 * projectile-graphic file at the offsets Remix appended (display list
 * 0x8F0, MObjSub 0xA90, MatAnimJoint 0xABC), held on joint 16, the bone
 * get_punch_bone_ gives Wolf. Its display is Remix's Size wrapper of
 * lbCommonDObjScaleXProcDisplay at scale 1; the port's definition of that
 * routine is empty, so the single DObj goes through display-list head 1 as
 * the Captain bridge in battleship_efmanager.c sends Falcon Punch. */
static EFDesc sNdsP4WolfSlashEffectDesc = {
    EFFECT_FLAG_USERDATA,                       /* 0x020F0000 */
    15,
    &gNdsP4WolfSpecial4,
    { 0x50, nGCMatrixKindRotRpyR, 0x00 },       /* 0x501C0000 */
    { nGCMatrixKindNull, nGCMatrixKindNull, 0x00 },
    efManagerNoEjectProcUpdate,
    gcDrawDObjDLHead1,
    0x08F0, 0x0A90, 0x0000, 0x0ABC
};

#define WOLF_SLASH_JOINT 16

static GObj *ndsP4WolfSlashMakeEffect(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *effect_gobj;
    EFStruct *ep;
    DObj *dobj;

    if ((gNdsP4WolfSpecial4 == NULL) || (fp->joints[WOLF_SLASH_JOINT] == NULL))
    {
        gNdsP4WolfArticleMisses++;
        return NULL;
    }
    effect_gobj = efManagerMakeEffectForce(&sNdsP4WolfSlashEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);
    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);
    dobj->user_data.p = fp->joints[WOLF_SLASH_JOINT];
    dobj->rotate.vec.f.y = fp->lr * F_CLC_DTOR32(-90.0F);

    return effect_gobj;
}

/* ---- Reflector (Wolf.asm wolf_reflect_graphic_struct) ----
 *
 * Fox's reflector effect on his reflector file: DObjDesc 0x288 and the
 * start, loop, hit and end AnimJoints of wolf_reflector_struct, which
 * Remix's copy of Fox's update reads in place of
 * dEFManagerFoxReflectorAnimJointOffsets. */
static const intptr_t sNdsP4WolfReflectorAnimJoints[4] = {
    0x041C, 0x04C4, 0x05E8, 0x06FC
};

static void ndsP4WolfReflectorSetAnimID(GObj *effect_gobj, s32 anim_id)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    ep->effect_vars.reflector.index = anim_id;

    gcAddAnimJointAll(effect_gobj,
        lbRelocGetFileData(AObjEvent32**, gNdsP4WolfSpecial2,
                           sNdsP4WolfReflectorAnimJoints[anim_id]), 0.0F);
    gcPlayAnimAll(effect_gobj);
}

/* wolf_reflector_graphic_routine: efManagerFoxReflectorProcUpdate with the
 * table above. */
static void ndsP4WolfReflectorProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    gcPlayAnimAll(effect_gobj);

    if (effect_gobj->anim_frame <= 0.0F)
    {
        switch (ep->effect_vars.reflector.index)
        {
        case 1:
            break;

        case 0:
        case 2:
            ndsP4WolfReflectorSetAnimID(effect_gobj, 1);
            break;

        case 3:
            efManagerSetPrevStructAlloc(ep);
            gcEjectGObj(effect_gobj);
            return;
        }
    }
    if (ep->effect_vars.reflector.status != 4)
    {
        ndsP4WolfReflectorSetAnimID(effect_gobj, ep->effect_vars.reflector.status);

        ep->effect_vars.reflector.status = 4;
    }
}

static EFDesc sNdsP4WolfReflectorEffectDesc = {
    0x4 | EFFECT_FLAG_USERDATA,                 /* 0x060F0000 */
    15,
    &gNdsP4WolfSpecial2,
    { 0x4F, nGCMatrixKindNull, 0x00 },          /* Fox's transforms */
    { nGCMatrixKindTra, 0x2C, 0x00 },
    ndsP4WolfReflectorProcUpdate,
    gcDrawDObjTreeForGObj,                      /* 0x80014038 */
    0x0288, 0x0000, 0x041C, 0x0000
};

/* efManagerFoxReflectorMakeEffect with Wolf's description. */
static GObj *ndsP4WolfReflectorMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;

    if (gNdsP4WolfSpecial2 == NULL)
    {
        gNdsP4WolfArticleMisses++;
        return NULL;
    }
    effect_gobj = efManagerMakeEffectForce(&sNdsP4WolfReflectorEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    DObjGetStruct(effect_gobj)->user_data.p =
        ftGetStruct(fighter_gobj)->joints[nFTPartsJointTopN];

    ep->effect_vars.reflector.index = 0;
    ep->effect_vars.reflector.status = 4;

    return effect_gobj;
}

/* ---- Wolfen entry (Wolf.asm wolfen_entry, _2, _3) ----
 *
 * Fox's Arwing maker on the Wolfen file (entry_anim_struct_WOLF, DObjDesc
 * 0x2610), the craft's animated child from it at 0x284C, and the fly-in
 * from his reflector file: 0xF74 facing right, 0xB24 facing left. The
 * update is Remix's Size wrapper of Fox's, at scale 1. */
static EFDesc sNdsP4WolfEntryWolfenEffectDesc = {
    0x4 | EFFECT_FLAG_USERDATA | 0x1,           /* 0x070A0000 */
    10,
    &gNdsP4WolfSpecial3,
    { nGCMatrixKindTraRotRpyR, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    efManagerFoxEntryArwingProcUpdate,
    gcDrawDObjTreeDLLinksForGObj,
    0x2610, 0x0000, 0x0000, 0x0000
};

static GObj *ndsP4WolfEntryWolfenMakeEffect(FTStruct *fp, Vec3f *pos, s32 lr)
{
    GObj *effect_gobj;
    DObj *dobj;
    DObj *what;
    s32 i;

    (void)fp;
    if ((gNdsP4WolfSpecial3 == NULL) || (gNdsP4WolfSpecial2 == NULL))
    {
        gNdsP4WolfArticleMisses++;
        return NULL;
    }
    effect_gobj = efManagerMakeEffectNoForce(&sNdsP4WolfEntryWolfenEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    /* Fox's walk to the craft's animated part, which the Wolfen tree
     * keeps: child, child, child, six siblings on, child. */
    what = dobj->child;
    for (i = 0; (what != NULL) && (i < 2); i++)
    {
        what = what->child;
    }
    for (i = 0; (what != NULL) && (i < 6); i++)
    {
        what = what->sib_next;
    }
    what = (what != NULL) ? what->child : NULL;

    if (what != NULL)
    {
        gcAddXObjForDObjFixed(what, 0x2C, 0);
        gcAddDObjAnimJoint(what,
            lbRelocGetFileData(AObjEvent32*, gNdsP4WolfSpecial3, 0x284C), 0.0F);
    }
    else gNdsP4WolfArticleMisses++;

    lbCommonAddDObjAnimJointAll(dobj->child,
        lbRelocGetFileData(AObjEvent32**, gNdsP4WolfSpecial2,
                           (lr == +1) ? 0x0F74 : 0x0B24), 0.0F);

    gcPlayAnimAll(effect_gobj);

    dobj->translate.vec.f = *pos;

    efManagerSortZNeg(dobj->child);

    return effect_gobj;
}

const NDSP4Overrides gNdsP4WolfOverrides = {
    .fox_reflector = ndsP4WolfReflectorMakeEffect,
    .fox_entry_arwing = ndsP4WolfEntryWolfenMakeEffect,
};

/* ---- Blaster (WolfNSP.main) ----
 *
 * His own weapon (_blaster_projectile_struct): a single DObj from his shot
 * attributes (special 1 + 0), Ray Gun ammo's map and hit routines, Master
 * Hand's bullet bounce off shields, and Remix's update and reflect
 * routines below. Remix gives it weapon kind 0 (Mario's fireball), which
 * no game code reads; the DS renderer keys its native weapon owners on
 * the kind, so his shot takes a P4 kind past the source's 0x1F. */
#define WOLF_BLASTER_KIND (nWPKindMonsterEnd + 1)
#define WOLF_BLASTER_JOINT 16
#define WOLF_BLASTER_LIFETIME 100
#define WOLF_BLASTER_SPEED_MAX 200.0F
#define WOLF_BLASTER_SPEED 22.0F
#define WOLF_BLASTER_ACCEL 1.03125F             /* lui 0x3F84 */
#define WOLF_BLASTER_ANGLE_GROUND 0.0F
#define WOLF_BLASTER_ANGLE_AIR 0.0F
#define WOLF_BLASTER_REFLECT_SCALE 1.5707964F   /* 0x3FC90FDB */

/* Remix keeps the speed cap's multiplier in the weapon's first state word
 * (its "free space" at WPStruct + 0x29C): 1, or pi/2 once Wolf reflects
 * it. */
static f32 *ndsP4WolfBlasterCapScale(WPStruct *wp)
{
    return (f32 *)(void *)&wp->weapon_vars;
}

/* blaster_duration (update): speed up 1/32 a frame to the cap either way,
 * and stretch the shot with its speed. No lifetime count, as in the donor:
 * the shot lasts until it hits something or leaves the stage. */
static sb32 ndsP4WolfBlasterProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    DObj *dobj = DObjGetStruct(weapon_gobj);
    f32 vel = wp->physics.vel_air.x * WOLF_BLASTER_ACCEL;
    f32 cap = WOLF_BLASTER_SPEED_MAX * *ndsP4WolfBlasterCapScale(wp);
    f32 scale_x;

    if (vel > cap)
    {
        vel = cap;
    }
    else if (vel <= -cap)
    {
        vel = -cap;
    }
    wp->physics.vel_air.x = vel;

    scale_x = ((ABSF(vel) + (3.0F * WOLF_BLASTER_SPEED)) * 0.25F) / WOLF_BLASTER_SPEED;
    dobj->scale.vec.f.x = scale_x;
    dobj->scale.vec.f.y = 1.0F / ((scale_x * 0.5F) + 0.5F);

    return FALSE;
}

/* wpBossBulletProcHop (shield bounce), which Remix's desc names. */
static sb32 ndsP4WolfBlasterProcHop(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    syVectorRotateAbout3D(&wp->physics.vel_air, &wp->shield_collide_dir,
                          wp->shield_collide_angle * 2);
    wpMainReflectorRotateWeaponModel(weapon_gobj);

    return FALSE;
}

/* blaster_reflection: a shot Wolf reflects gets a higher cap and pi/2 its
 * speed; anyone's reflection renews its 100 frames and turns it. */
static sb32 ndsP4WolfBlasterProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);
    f32 scale = (ndsP4Content(fp) == NDS_P4_ID_WOLF) ?
                WOLF_BLASTER_REFLECT_SCALE : 1.0F;

    *ndsP4WolfBlasterCapScale(wp) = scale;
    wp->lifetime = WOLF_BLASTER_LIFETIME;
    wp->physics.vel_air.x *= scale;

    wpMainReflectorSetLR(wp, fp);

    /* The donor's _branch/_left: DObj + 0x34 = +-pi/2 by the new heading,
     * which is wpMainVelSetModelPitch. */
    wpMainVelSetModelPitch(weapon_gobj);

    return FALSE;
}

static WPDesc sNdsP4WolfBlasterWeaponDesc = {
    0x00,
    WOLF_BLASTER_KIND,
    &gNdsP4WolfSpecial1,
    0x0,
    { nGCMatrixKindTraRotRpyRSca, 0x48, 0 },    /* 0x12480000 */
    ndsP4WolfBlasterProcUpdate,
    itLGunWeaponAmmoProcMap,
    itLGunWeaponAmmoProcHit,
    itLGunWeaponAmmoProcHit,
    ndsP4WolfBlasterProcHop,
    itLGunWeaponAmmoProcHit,
    ndsP4WolfBlasterProcReflector,
    itLGunWeaponAmmoProcHit
};

/* projectile_stage_setting: the shot from the hand, level, at 22 units a
 * frame in his facing. */
static GObj *ndsP4WolfBlasterMakeWeapon(GObj *fighter_gobj, Vec3f *pos)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *weapon_gobj;
    WPStruct *wp;
    f32 angle;

    if (gNdsP4WolfSpecial1 == NULL)
    {
        gNdsP4WolfArticleMisses++;
        return NULL;
    }
    weapon_gobj = wpManagerMakeWeapon(fighter_gobj, &sNdsP4WolfBlasterWeaponDesc,
                                      pos, WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER);
    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);

    *ndsP4WolfBlasterCapScale(wp) = 1.0F;
    wp->lifetime = WOLF_BLASTER_LIFETIME;

    angle = (fp->ga == nMPKineticsAir) ? WOLF_BLASTER_ANGLE_AIR : WOLF_BLASTER_ANGLE_GROUND;

    wp->physics.vel_air.z = 0.0F;
    wp->physics.vel_air.x = cosf(angle) * WOLF_BLASTER_SPEED * fp->lr;
    wp->physics.vel_air.y = sinf(angle) * WOLF_BLASTER_SPEED;

    /* The fireball struct's palette index (0), as Mario's maker sets it. */
    if (DObjGetStruct(weapon_gobj)->mobj != NULL)
    {
        DObjGetStruct(weapon_gobj)->mobj->palette_id = 0.0F;
    }
    wpMainVelSetModelPitch(weapon_gobj);

    return weapon_gobj;
}

/* WolfNSP.main (0xE1/0xE2 update): one shot from joint 16 when the script
 * sets temp variable 1, then wait or fall at the animation's end. */
void ndsP4WolfNSPMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag0 != 0)
    {
        Vec3f pos;

        fp->motion_vars.flags.flag0 = 0;
        pos.x = pos.y = pos.z = 0.0F;

        if (fp->joints[WOLF_BLASTER_JOINT] != NULL)
        {
            gmCollisionGetFighterPartsWorldPosition(fp->joints[WOLF_BLASTER_JOINT], &pos);
            ndsP4WolfBlasterMakeWeapon(fighter_gobj, &pos);
        }
        else gNdsP4WolfArticleMisses++;
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
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

/* WolfUSP.main_2 (0xE6/0xE8 update): Fox's Fire Fox end with the slash
 * made while temp variable 1 is 0 (stopped when the script sets it to 2),
 * Wolf's landing lag, and the interrupt flag taken from temp variable 1.
 * The donor's Pokemon Stadium announcer call is outside the port. */
void ndsP4WolfUSPMain2(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 flag0 = fp->motion_vars.flags.flag0;

    if (flag0 == 0)
    {
        ndsP4WolfSlashMakeEffect(fighter_gobj);
        fp->is_effect_attach = TRUE;
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
