/* P2 Iwark / Onix (kind nITKindIwark). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itmonster/itiwark.c:1-473.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x72C), the rock weapon-attribute row (0x774), the data-start base
 * (0xA140) and the display list (0xA640) live below
 * (decomp/BattleShip-main/include/reloc_data.us.h:3761, :3762, :3808,
 * :3809); the port's generated reloc header does not publish Iwark tokens,
 * so this TU owns its uintptr_t tokens the same way
 * battleship_item_gbumper.c owns GBumper's (local tokens, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * itGetMonsterAnimNode, itGetPData, the monster SFX and voice IDs, the anim
 * helpers, and func_800269C0_275C0 ride on it/item.h, gm/gmsound.h,
 * nds/nds_obj_anim.h, and sys/audio.h, so no local externs are written for
 * them. The ITIWARK_ half of the tuning rides on it/item.h; the WPIWARK_
 * weapon half has no port header yet, so the decomp it/itvars.h:282-292
 * values travel with this TU, every literal the source's own. The port's
 * WPStruct carries coin, hydro and smog but not the source rock payload
 * (decomp wp/wpvars.h:270-278), so that layout travels here as
 * NdsWpIwarkRockVars and overlays weapon_vars through ndsWpIwarkRock; field
 * names and order are the source's, no values invented here. The syUtils,
 * syVector, lbCommon, wpMap and effect entry points below are referenced
 * verbatim and listed in the task report.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <wp/weapon.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <gr/ground.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/audio.h>
#include <nds/nds_obj_anim.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3761. */
uintptr_t llITCommonDataWarkItemAttributes = 0x72Cu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3762. */
uintptr_t llITCommonDataWarkRockWeaponAttributes = 0x774u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3808. */
uintptr_t llITCommonDataWarkDataStart = 0xA140u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3809. */
uintptr_t llITCommonDataWarkDisplayList = 0xA640u;

extern void *gITManagerCommonData;

/* decomp it/itvars.h:282-292 verbatim. include/it/item.h carries the
 * ITIWARK_ half but not this WPIWARK_ weapon half, so the source values
 * travel with this TU; every literal below is the source's own. */
#define WPIWARK_ROCK_RANDOM_VEL_MAX 3
#define WPIWARK_ROCK_GRAVITY 2.0F
#define WPIWARK_ROCK_TVEL 200.0F
#define WPIWARK_ROCK_ROTATE_STEP (-0.5F)
#define WPIWARK_ROCK_VEL_Y_START_A (-100.0F)
#define WPIWARK_ROCK_VEL_Y_START_B (-50.0F)
#define WPIWARK_ROCK_VEL_Y_START_C 0.0F
#define WPIWARK_ROCK_COLLIDE_MUL_VEL_Y 0.1F
#define WPIWARK_ROCK_COLLIDE_ADD_VEL_Y (-150.0F)

/* decomp wp/wpvars.h:270-278 verbatim. The port's WPStruct has no rock
 * member, so the source layout travels here and overlays the union through
 * ndsWpIwarkRock; the union is big enough (raw[32]) and the fields are only
 * ever reached through this accessor, so behavior matches the source. */
typedef struct NdsWpIwarkRockVars
{
    s32 unk_0x0;
    s32 floor_line_id;
    s32 unk_0x8;
    s32 unk_0xC;
    GObj *owner_gobj;
} NdsWpIwarkRockVars;
#define ndsWpIwarkRock(wp) (*(NdsWpIwarkRockVars *)&(wp)->weapon_vars)

/* decomp sys/utils.h:19-:20. Same seam as
 * battleship_item_bombhei.c:60-61. */
extern f32 syUtilsRandFloat(void);
extern s32 syUtilsRandIntRange(s32 range);

/* decomp sys/vector.h:40 and :42. Same seam as
 * battleship_item_lgun.c:77-79. */
extern Vec3f *syVectorRotate3D(Vec3f *dst, s32 axis, f32 angle);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
extern f32 syUtilsArcTan2(f32 y, f32 x);
#ifndef SYVECTOR_AXIS_Z
#define SYVECTOR_AXIS_Z 4
#endif

/* decomp lb/lbcommon.h:20-:21. Same seam as
 * battleship_item_link_core.c:76-77. */
extern Vec3f *lbCommonScale2D(Vec3f *dst, f32 scale);
extern Vec3f *lbCommonReflect2D(Vec3f *dst, Vec3f *p);

/* decomp wp/wpmap.h. Same seam as battleship_item_lgun.c:74. */
extern sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);

/* decomp ef/efmanager.h:41, :38 and :69. Same shapes as
 * battleship_item_bombhei.c:67-72. */
extern LBParticle *efManagerDustHeavyDoubleMakeEffect(Vec3f *pos, s32 lr, f32 scale);
extern LBParticle *efManagerDustLightMakeEffect(Vec3f *pos, s32 lr, f32 scale);
extern GObj *efManagerQuakeMakeEffect(s32 magnitude);

/* decomp itiwark.h:8-22 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly
 * as the Tomato and Star files carry theirs. */
extern void itIwarkAttackUpdateRock(GObj *iwark_gobj);
extern sb32 itIwarkAttackProcUpdate(GObj *item_gobj);
extern void itIwarkAttackInitVars(GObj *item_gobj);
extern void itIwarkAttackSetStatus(GObj *item_gobj);
extern sb32 itIwarkFlyProcUpdate(GObj *item_gobj);
extern void itIwarkFlySetStatus(GObj *item_gobj);
extern sb32 itIwarkCommonProcUpdate(GObj *item_gobj);
extern sb32 itIwarkCommonProcMap(GObj *item_gobj);
extern GObj* itIwarkMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
extern sb32 itIwarkWeaponRockProcDead(GObj *weapon_gobj);
extern sb32 itIwarkWeaponRockProcUpdate(GObj *weapon_gobj);
extern sb32 itIwarkWeaponRockProcMap(GObj *weapon_gobj);
extern sb32 itIwarkWeaponRockProcHop(GObj *weapon_gobj);
extern sb32 itIwarkWeaponRockProcReflector(GObj *weapon_gobj);
extern GObj* itIwarkWeaponRockMakeWeapon(GObj *parent_gobj, Vec3f *pos, u8 random);

// 0x8018AA90
// decomp itiwark.c:13-35 verbatim, adapted only for the port's ITDesc shape
// (o_attributes is const void * here, lbRelocGetFileData takes the token the
// same way).
ITDesc dITIwarkItemDesc =
{
    nITKindIwark,                           // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataWarkItemAttributes,      // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindNull,                  // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    nGMAttackStateNew,                      // Hitbox Update State
    itIwarkCommonProcUpdate,                // Proc Update
    itIwarkCommonProcMap,                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x8018AAC4
// decomp itiwark.c:38-63 verbatim.
ITStatusDesc dITIwarkStatusDescs[/* */] =
{
    // Status 0 (Neutral Fly)
    {
        itIwarkFlyProcUpdate,               // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 1 (Neutral Attack)
    {
        itIwarkAttackProcUpdate,            // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// 0x8018AB04
// decomp itiwark.c:66-88 verbatim.
WPDesc dITIwarkWeaponRockWeaponDesc =
{
    0x01,                                   // Render flags?
    nWPKindIwarkRock,                       // Weapon Kind
    &gITManagerCommonData,                  // Pointer to weapon's loaded files?
    &llITCommonDataWarkRockWeaponAttributes,// Offset of weapon attributes in loaded files

    // DObj transformation struct
    {
        nGCMatrixKindNull,                  // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    itIwarkWeaponRockProcUpdate,            // Proc Update
    itIwarkWeaponRockProcMap,               // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    itIwarkWeaponRockProcHop,               // Proc Hop
    NULL,                                   // Proc Set-Off
    itIwarkWeaponRockProcReflector,         // Proc Reflector
    NULL                                    // Proc Absorb
};

// decomp itiwark.c:96-101 verbatim.
enum itIwarkStatus
{
    nITIwarkStatusFly,
    nITIwarkStatusAttack,
    nITIwarkStatusEnumCount
};

// 0x8017D740
// decomp itiwark.c:110-144 verbatim.
void itIwarkAttackUpdateRock(GObj *iwark_gobj)
{
    ITStruct *ip = itGetStruct(iwark_gobj);
    DObj *dobj = DObjGetStruct(iwark_gobj);

    if (ip->item_vars.iwark.rock_spawn_wait <= 0)
    {
        WPStruct *wp;
        GObj *rock_gobj;
        Vec3f pos = dobj->translate.vec.f;

        pos.x += (ITIWARK_ROCK_SPAWN_OFF_X_MUL * syUtilsRandFloat()) + ITIWARK_ROCK_SPAWN_OFF_X_ADD;

        rock_gobj = itIwarkWeaponRockMakeWeapon(iwark_gobj, &pos, syUtilsRandIntRange(WPIWARK_ROCK_RANDOM_VEL_MAX));

        if (rock_gobj != NULL)
        {
            wp = wpGetStruct(rock_gobj);

        #if !defined (DAIRANTOU_OPT0)
            ndsWpIwarkRock(wp).unk_0xC = ip->item_vars.iwark.rock_spawn_max - ip->item_vars.iwark.rock_spawn_remain;
        #endif

            ip->item_vars.iwark.rock_spawn_remain--;

        #if !defined (DAIRANTOU_OPT0)
            if (ip->item_vars.iwark.rock_spawn_remain == 0)
            {
                ndsWpIwarkRock(wp).unk_0xC = -1;
            }
        #endif
            ip->item_vars.iwark.rock_spawn_wait = syUtilsRandIntRange(ITIWARK_ROCK_SPAWN_WAIT_MAX) + ITIWARK_ROCK_SPAWN_WAIT_MIN;
        }
    }
}

// 0x8017D820
// decomp itiwark.c:147-193 verbatim.
sb32 itIwarkAttackProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
#if defined(REGION_US)
    f32 pos_y = gMPCollisionGroundData->map_bound_top - ITIWARK_FLY_STOP_Y;

    if (dobj->translate.vec.f.y >= pos_y)
    {
        dobj->translate.vec.f.y = pos_y;
#else
    if (dobj->translate.vec.f.y >= gMPCollisionGroundData->map_bound_top - ITIWARK_FLY_STOP_Y)
    {
#endif

        ip->physics.vel_air.y = 0.0F;

        if (ip->item_vars.iwark.rock_spawn_remain != 0)
        {
            itIwarkAttackUpdateRock(item_gobj);
        }
        else if (ip->item_vars.iwark.rock_spawn_count == ip->item_vars.iwark.rock_spawn_max)
        {
            return TRUE;
        }
        if ((ip->item_vars.iwark.rumble_wait == 0) && (ip->item_vars.iwark.rumble_frame != 0))
        {
            efManagerQuakeMakeEffect(0);

            ip->item_vars.iwark.rumble_wait = ITIWARK_ROCK_RUMBLE_WAIT;
        }
        if (ip->item_vars.iwark.rumble_frame != 0)
        {
            ip->item_vars.iwark.rumble_wait--;
        }
        ip->item_vars.iwark.rock_spawn_wait--;
    }
    if (ip->multi == ITIWARK_MODEL_ROTATE_WAIT)
    {
        dobj->rotate.vec.f.y += F_CST_DTOR32(180.0F);

        ip->multi = 0;
    }
    ip->multi++;

    return FALSE;
}

// 0x8017D948
// decomp itiwark.c:196-234 verbatim.
void itIwarkAttackInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    Gfx *dl;
    Vec3f pos;

#if defined(REGION_US)
    ip->ga = nMPKineticsAir;
#endif

    ip->physics.vel_air.y = ITIWARK_FLY_VEL_Y;

    ip->item_vars.iwark.rock_spawn_remain = syUtilsRandIntRange(ITIWARK_ROCK_SPAWN_COUNT_RANDOM) + ITIWARK_ROCK_SPAWN_COUNT_MIN;
    ip->item_vars.iwark.rock_spawn_max = ip->item_vars.iwark.rock_spawn_remain;
    ip->item_vars.iwark.rock_spawn_count = 0;
    ip->item_vars.iwark.rock_spawn_wait = 0;
    ip->item_vars.iwark.rumble_frame = 0;
    ip->item_vars.iwark.rumble_wait = 0;

    ip->multi = 0;

    pos = dobj->translate.vec.f;

    if (ip->kind == nITKindIwark)
    {
        dobj->dl = dl = (Gfx*) itGetPData(ip, &llITCommonDataWarkDataStart, &llITCommonDataWarkDisplayList);

        pos.y += ITIWARK_IWARK_ADD_POS_Y;
    }
    else pos.y += ITIWARK_OTHER_ADD_POS_Y;

    efManagerDustHeavyDoubleMakeEffect(&pos, -1, 1.0F);

    if (ip->kind == nITKindIwark)
    {
        func_800269C0_275C0(nSYAudioVoiceMBallIwarkAppear);
    }
}

// 0x8017DA60
// decomp itiwark.c:237-241 verbatim.
void itIwarkAttackSetStatus(GObj *item_gobj)
{
    itIwarkAttackInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITIwarkStatusDescs, nITIwarkStatusAttack);
}

// 0x8017DA94
// decomp itiwark.c:244-255 verbatim.
sb32 itIwarkFlyProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        itIwarkAttackSetStatus(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x8017DAD8
// decomp itiwark.c:258-267 verbatim.
void itIwarkFlySetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = ITIWARK_FLY_WAIT;

    ip->physics.vel_air.x = ip->physics.vel_air.y = 0.0F;

    itMainSetStatus(item_gobj, dITIwarkStatusDescs, nITIwarkStatusFly);
}

// 0x8017DB18
// decomp itiwark.c:270-281 verbatim.
sb32 itIwarkCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        itIwarkFlySetStatus(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x8017DB5C
// decomp itiwark.c:284-295 verbatim.
sb32 itIwarkCommonProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR) != FALSE)
    {
        ip->physics.vel_air.y = 0.0F;

        itMapSetGround(ip);
    }
    return FALSE;
}

// 0x8017DBA0
// decomp itiwark.c:298-329 verbatim.
GObj* itIwarkMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITIwarkItemDesc, pos, vel, flags);
    DObj *dobj;
    ITStruct *ip;

    if (item_gobj != NULL)
    {
        itMainClearOwnerStats(item_gobj);

        dobj = DObjGetStruct(item_gobj);

        gcAddXObjForDObjFixed(dobj, nGCMatrixKindTraRotRpyR, 0);
        gcAddXObjForDObjFixed(dobj, 0x48, 0);

        dobj->translate.vec.f = *pos;

        ip = itGetStruct(item_gobj);

        ip->multi = ITMONSTER_RISE_STOP_WAIT;

        ip->attack_coll.interact_mask = GMHITCOLLISION_FLAG_FIGHTER;

        ip->physics.vel_air.x = ip->physics.vel_air.z = 0.0F;
        ip->physics.vel_air.y = ITMONSTER_RISE_VEL_Y;

        dobj->translate.vec.f.y -= ip->attr->map_coll_bottom;

        gcAddDObjAnimJoint(dobj, itGetMonsterAnimNode(ip, &llITCommonDataWarkDataStart), 0.0F);
    }
    return item_gobj;
}

// 0x8017DCAC
// decomp itiwark.c:332-340 verbatim.
sb32 itIwarkWeaponRockProcDead(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    ITStruct *ip = itGetStruct(ndsWpIwarkRock(wp).owner_gobj);

    ip->item_vars.iwark.rock_spawn_count++;

    return TRUE;
}

// 0x8017DCCC
// decomp itiwark.c:343-355 verbatim.
sb32 itIwarkWeaponRockProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    DObj *dobj;

    wpMainApplyGravityClampTVel(wp, WPIWARK_ROCK_GRAVITY, WPIWARK_ROCK_TVEL);

    dobj = DObjGetStruct(weapon_gobj);

    dobj->rotate.vec.f.z += WPIWARK_ROCK_ROTATE_STEP;

    return FALSE;
}

// 0x8017DD18
// decomp itiwark.c:358-389 verbatim.
sb32 itIwarkWeaponRockProcMap(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    ITStruct *ip = itGetStruct(ndsWpIwarkRock(wp).owner_gobj);
    MPCollData *coll_data = &wp->coll_data;
    Vec3f pos = DObjGetStruct(weapon_gobj)->translate.vec.f;
    s32 line_id = ndsWpIwarkRock(wp).floor_line_id;

    wpMapTestAllCheckCollEnd(weapon_gobj);

    if (coll_data->mask_curr & MAP_FLAG_FLOOR)
    {
        if (line_id != coll_data->floor_line_id)
        {
            lbCommonReflect2D(&wp->physics.vel_air, &coll_data->floor_angle);
            lbCommonScale2D(&wp->physics.vel_air, WPIWARK_ROCK_COLLIDE_MUL_VEL_Y);

            ndsWpIwarkRock(wp).floor_line_id = coll_data->floor_line_id;

            func_800269C0_275C0(nSYAudioFGMIwarkRockMake);

            pos.y += WPIWARK_ROCK_COLLIDE_ADD_VEL_Y;

            efManagerDustLightMakeEffect(&pos, wp->lr, 1.0F);

            wp->lr = -wp->lr;

            ip->item_vars.iwark.rumble_frame++;
        }
    }
    return FALSE;
}

// 0x8017DE10
// decomp itiwark.c:392-408 verbatim.
sb32 itIwarkWeaponRockProcHop(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    syVectorRotateAbout3D(&wp->physics.vel_air, &wp->shield_collide_dir, wp->shield_collide_angle * 2);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.z = syUtilsArcTan2(wp->physics.vel_air.y, wp->physics.vel_air.x);
    DObjGetStruct(weapon_gobj)->scale.vec.f.x = 1.0F;

    if (wp->physics.vel_air.x > 0.0F)
    {
        wp->lr = +1;
    }
    else wp->lr = -1;

    return FALSE;
}

// 0x8017DEB8
// decomp itiwark.c:411-424 verbatim.
sb32 itIwarkWeaponRockProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);

    wpMainReflectorSetLR(wp, fp);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.z = syUtilsArcTan2(wp->physics.vel_air.y, wp->physics.vel_air.x);
    DObjGetStruct(weapon_gobj)->scale.vec.f.x = 1.0F;

    wp->lr = -wp->lr;

    return FALSE;
}

// 0x8017DF28
// decomp itiwark.c:427-473 verbatim.
GObj* itIwarkWeaponRockMakeWeapon(GObj *parent_gobj, Vec3f *pos, u8 random)
{
    u32 random32;
    GObj *weapon_gobj = wpManagerMakeWeapon(parent_gobj, &dITIwarkWeaponRockWeaponDesc, pos, WEAPON_FLAG_PARENT_ITEM);
    DObj *dobj;
    f32 vel_y;
    WPStruct *wp;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);

    ndsWpIwarkRock(wp).floor_line_id = -1;

    random32 = random;

    if (random32 == 0)
    {
        wp->physics.vel_air.y = WPIWARK_ROCK_VEL_Y_START_A;
    }
    else wp->physics.vel_air.y = vel_y = (random32 == 1) ? WPIWARK_ROCK_VEL_Y_START_B : WPIWARK_ROCK_VEL_Y_START_C;

    if (syUtilsRandIntRange(2) == 0)
    {
        wp->lr = -1;
    }
    else wp->lr = +1;

    dobj = DObjGetStruct(weapon_gobj);

    gcAddXObjForDObjFixed(dobj, nGCMatrixKindTraRotRpyR, 0);
    gcAddXObjForDObjFixed(dobj, 0x46, 0);

    dobj->translate.vec.f = *pos;

    dobj->child->mobj->texture_id_curr = random;

    ndsWpIwarkRock(wp).owner_gobj = parent_gobj;

    wp->is_hitlag_victim = TRUE;

    wp->proc_dead = itIwarkWeaponRockProcDead;

    return weapon_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
