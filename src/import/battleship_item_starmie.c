/* P2 Starmie (kind nITKindStarmie). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itmonster/itstarmie.c:1-478.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0xB34 (reloc_data.us.h:3777), the Swift weapon row is 0xB7C (:3778),
 * the Starmie data-start base is 0x112A0 (:3824) and the Starmie matanim
 * joint is 0x11338 (:3825); the port's generated reloc header does not
 * publish Starmie tokens, so this TU owns its uintptr_t tokens the same
 * way battleship_item_gbumper.c owns GBumper's (local tokens, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (the syUtils and syVector
 * entry points, efManagerStarSplashMakeEffect, and
 * efManagerSparkleWhiteScaleMakeEffect) are referenced verbatim and listed
 * in the task report -- no values invented here. itGetMonsterAnimNode,
 * itGetPData, the monster SFX and voice IDs, the anim helpers, and
 * func_800269C0_275C0 ride on it/item.h, gm/gmsound.h, nds/nds_obj_anim.h,
 * and sys/audio.h, so no local externs are written for them.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <wp/weapon.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/objman.h>
#include <sys/audio.h>
#include <nds/nds_obj_anim.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3777. */
uintptr_t llITCommonDataStarmieItemAttributes = 0xB34u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3778. */
uintptr_t llITCommonDataStarmieSwiftWeaponAttributes = 0xB7Cu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3824. */
uintptr_t llITCommonDataStarmieDataStart = 0x112A0u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3825. */
uintptr_t llITCommonDataStarmieMatAnimJoint = 0x11338u;

extern void *gITManagerCommonData;

/* decomp sys/utils.h:20 and :8. Same seam as
 * battleship_item_bombhei.c:60-61. */
extern s32 syUtilsRandIntRange(s32 range);
extern f32 syUtilsArcTan2(f32 y, f32 x);

/* decomp sys/vector.h:33, :40 and :42. Same seam as
 * battleship_item_nyars.c:51-52. */
extern Vec3f *syVectorDiff3D(Vec3f *dst, Vec3f *src, Vec3f *sub);
extern Vec3f *syVectorRotate3D(Vec3f *dst, s32 axis, f32 angle);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
#ifndef SYVECTOR_AXIS_Z
#define SYVECTOR_AXIS_Z 4
#endif

/* decomp ef/efmanager.h:105 and :65. The port defines the Sparkle maker
 * (battleship_efmanager.c:2276); no port header in this TU's chain
 * publishes either prototype, so both travel here verbatim, the same way
 * battleship_item_starrod.c:87-89 carries StarSplash. */
extern LBGenerator *efManagerStarSplashMakeEffect(Vec3f *pos, s32 lr);
extern LBParticle *efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale);

/* decomp itstarmie.h:8-24 verbatim. The port publishes no per-kind item or
 * weapon procs, so the source header's declarations travel with this TU,
 * exactly as the Nyars file carries its coin procs. */
extern void itStarmieAttackUpdateSwift(GObj *item_gobj);
extern sb32 itStarmieAttackProcUpdate(GObj *item_gobj);
extern void itStarmieAttackInitVars(GObj *item_gobj);
extern void itStarmieAttackSetStatus(GObj *item_gobj);
extern sb32 itStarmieNFollowProcUpdate(GObj *item_gobj);
extern void itStarmieNFollowFindFollowPlayerLR(GObj *item_gobj, GObj *fighter_gobj);
extern void itStarmieNFollowInitVars(GObj *item_gobj);
extern void itStarmieNFollowSetStatus(GObj *item_gobj);
extern sb32 itStarmieCommonProcUpdate(GObj *item_gobj);
extern sb32 itStarmieCommonProcMap(GObj *item_gobj);
extern GObj* itStarmieMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
extern sb32 itStarmieWeaponSwiftProcUpdate(GObj *weapon_gobj);
extern sb32 itStarmieWeaponSwiftProcHit(GObj *weapon_gobj);
extern sb32 itStarmieWeaponSwiftProcHop(GObj *weapon_gobj);
extern sb32 itStarmieWeaponSwiftProcReflector(GObj *weapon_gobj);
extern GObj* itStarmieWeaponSwiftMakeWeapon(GObj *item_gobj, Vec3f *pos);
extern void itStarmieAttackMakeSwift(GObj *item_gobj, Vec3f *pos);

// 0x8018B170
// decomp itstarmie.c:13-35 verbatim, adapted only for the port's ITDesc
// shape (o_attributes is const void * here, lbRelocGetFileData takes the
// token the same way).
ITDesc dITStarmieItemDesc =
{
    nITKindStarmie,                         // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataStarmieItemAttributes,   // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    nGMAttackStateNew,                      // Hitbox Update State
    itStarmieCommonProcUpdate,              // Proc Update
    itStarmieCommonProcMap,                 // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x8018B1A4
// decomp itstarmie.c:38-63 verbatim.
ITStatusDesc dITStarmieStatusDescs[/* */] =
{
    // Status 0 (Neutral Follow)
    {
        itStarmieNFollowProcUpdate,         // Proc Update
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
        itStarmieAttackProcUpdate,          // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// 0x8018B1E4
// decomp itstarmie.c:66-88 verbatim.
WPDesc dITStarmieWeaponSwiftWeaponDesc =
{
    0x03,                                   // Render flags?
    nWPKindStarmieSwift,                    // Weapon Kind
    &gITManagerCommonData,                  // Pointer to character's loaded files?
    &llITCommonDataStarmieSwiftWeaponAttributes, // Offset of weapon attributes in loaded files

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyRSca,         // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    itStarmieWeaponSwiftProcUpdate,         // Proc Update
    NULL,                                   // Proc Map
    itStarmieWeaponSwiftProcHit,            // Proc Hit
    itStarmieWeaponSwiftProcHit,            // Proc Shield
    itStarmieWeaponSwiftProcHop,            // Proc Hop
    itStarmieWeaponSwiftProcHit,            // Proc Set-Off
    itStarmieWeaponSwiftProcReflector,      // Proc Reflector
    itStarmieWeaponSwiftProcHit             // Proc Absorb
};

// decomp itstarmie.c:96-101 verbatim.
enum itStarmieStatus
{
    nITStarmieStatusNFollow,
    nITStarmieStatusAttack,
    nITStarmieStatusEnumCount
};

// 0x80181C20
// decomp itstarmie.c:110-134 verbatim.
void itStarmieAttackUpdateSwift(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (ip->item_vars.starmie.swift_spawn_wait <= 0)
    {
        Vec3f pos = dobj->translate.vec.f;

        if (ip->kind == nITKindStarmie)
        {
            pos.x += ITSTARMIE_STARMIE_SWIFT_SPAWN_OFF_X * ip->lr;
            pos.y += ITSTARMIE_STARMIE_SWIFT_SPAWN_OFF_Y;
        }
        else pos.x += ITSTARMIE_OTHER_SWIFT_SPAWN_OFF_X * ip->lr;

        itStarmieAttackMakeSwift(item_gobj, &pos);

        func_800269C0_275C0(nSYAudioFGMMonsterShoot);

        ip->item_vars.starmie.swift_spawn_wait = (syUtilsRandIntRange(ITSTARMIE_SWIFT_SPAWN_WAIT_RANDOM) + ITSTARMIE_SWIFT_SPAWN_WAIT_CONST);

        ip->physics.vel_air.x = -ip->lr * ITSTARMIE_PUSH_VEL_X;
    }
}

// 0x80181D24
// decomp itstarmie.c:137-154 verbatim.
sb32 itStarmieAttackProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        return TRUE;
    }
    itStarmieAttackUpdateSwift(item_gobj);

    ip->item_vars.starmie.swift_spawn_wait--;

    ip->physics.vel_air.x += ip->item_vars.starmie.add_vel_x;

    ip->multi--;

    return FALSE;
}

// 0x80181D8C
// decomp itstarmie.c:157-173 verbatim.
void itStarmieAttackInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    s32 lr_bak = ip->lr;

    ip->lr = (ip->item_vars.starmie.victim_pos.x < dobj->translate.vec.f.x) ? -1 : +1;

    if (ip->lr != lr_bak)
    {
        dobj->rotate.vec.f.y += F_CST_DTOR32(180.0F);
    }
    ip->multi = ITSTARMIE_LIFETIME;

    ip->item_vars.starmie.swift_spawn_wait = 0;
    ip->item_vars.starmie.add_vel_x = ip->lr * ITSTARMIE_ADD_VEL_X;
}

// 0x80181E0C
// decomp itstarmie.c:176-180 verbatim.
void itStarmieAttackSetStatus(GObj *item_gobj)
{
    itStarmieAttackInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITStarmieStatusDescs, nITStarmieStatusAttack);
}

// 0x80181E40
// decomp itstarmie.c:183-203 verbatim.
sb32 itStarmieNFollowProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if ((ip->lr == +1) && (dobj->translate.vec.f.x >= ip->item_vars.starmie.target_pos.x))
    {
        ip->physics.vel_air.x = 0.0F;
        ip->physics.vel_air.y = 0.0F;

        itStarmieAttackSetStatus(item_gobj);
    }
    if ((ip->lr == -1) && (dobj->translate.vec.f.x <= ip->item_vars.starmie.target_pos.x))
    {
        ip->physics.vel_air.x = 0.0F;
        ip->physics.vel_air.y = 0.0F;

        itStarmieAttackSetStatus(item_gobj);
    }
    return FALSE;
}

// 0x80181EF4
// decomp itstarmie.c:206-249 verbatim.
void itStarmieNFollowFindFollowPlayerLR(GObj *item_gobj, GObj *fighter_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *item_dobj = DObjGetStruct(item_gobj);
    DObj *fighter_dobj = DObjGetStruct(fighter_gobj);
    Vec3f dist;
    Vec3f target_pos;
    Vec3f *victim_pos;

    target_pos = fighter_dobj->translate.vec.f;

    dist.x = fighter_dobj->translate.vec.f.x - item_dobj->translate.vec.f.x;

    target_pos.y += ITSTARMIE_TARGET_POS_OFF_Y - fp->coll_data.map_coll.bottom;

    target_pos.x -= (fp->coll_data.map_coll.width + ITSTARMIE_TARGET_POS_OFF_X) * ((dist.x < 0.0F) ? -1 : +1);

    victim_pos = &fighter_dobj->translate.vec.f;

    syVectorDiff3D(&dist, &target_pos, &item_dobj->translate.vec.f);

    ip->physics.vel_air.y = ip->physics.vel_air.z = 0.0F;
    ip->physics.vel_air.x = ITSTARMIE_FOLLOW_VEL_X;

    syVectorRotate3D(&ip->physics.vel_air, SYVECTOR_AXIS_Z, syUtilsArcTan2(dist.y, dist.x));

    ip->item_vars.starmie.target_pos = target_pos;

    ip->item_vars.starmie.victim_pos = *victim_pos;

    ip->lr = (dist.x < 0.0F) ? -1 : +1;

    if (ip->lr == +1)
    {
        item_dobj->rotate.vec.f.y = F_CST_DTOR32(180.0F);
    }
    if (ip->kind == nITKindStarmie)
    {
        gcAddMObjMatAnimJoint(item_dobj->mobj, itGetPData(ip, &llITCommonDataStarmieDataStart, &llITCommonDataStarmieMatAnimJoint), 0);

        gcPlayAnimAll(item_gobj);
    }
}

// 0x801820CC
// decomp itstarmie.c:252-316 verbatim.
void itStarmieNFollowInitVars(GObj *item_gobj)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
#if defined(REGION_US)
    ITStruct *ip = itGetStruct(item_gobj);
    GObj *victim_gobj;
    s32 unused2[2];
    DObj *dobj = DObjGetStruct(item_gobj);
    f32 square_xy;
    f32 dist_x;
    f32 dist_xy;
    Vec3f dist;
#else
    // TODO: regswap
    s32 unused1;
    GObj *victim_gobj;
    s32 unused2[2];
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    FTStruct *owner_fp = ftGetStruct(ip->owner_gobj);
    f32 square_xy;
    f32 dist_xy;
    Vec3f dist;
    f32 dist_x;
#endif
    s32 players = 0;

    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

#if defined(REGION_US)
        if ((fighter_gobj != ip->owner_gobj) && (fp->team != ip->team))
#else
        if ((fighter_gobj != ip->owner_gobj) && (fp->team != owner_fp->team))
#endif
        {
            syVectorDiff3D(&dist, &DObjGetStruct(fighter_gobj)->translate.vec.f, &dobj->translate.vec.f);

            if (players == 0)
            {
                dist_xy = SQUARE(dist.x) + SQUARE(dist.y);
            }
            players++;

            square_xy = SQUARE(dist.x) + SQUARE(dist.y);

            if (square_xy <= dist_xy)
            {
                dist_xy = square_xy;

                victim_gobj = fighter_gobj;
            }
        }
        fighter_gobj = fighter_gobj->link_next;

        continue;
    }
    itStarmieNFollowFindFollowPlayerLR(item_gobj, victim_gobj);

    if (ip->kind == nITKindStarmie)
    {
        func_800269C0_275C0(nSYAudioVoiceMBallStarmieAppear);
    }
}

// 0x801821E8
// decomp itstarmie.c:319-323 verbatim.
void itStarmieNFollowSetStatus(GObj *item_gobj)
{
    itStarmieNFollowInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITStarmieStatusDescs, nITStarmieStatusNFollow);
}

// 0x8018221C
// decomp itstarmie.c:326-339 verbatim.
sb32 itStarmieCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        ip->physics.vel_air.x = ip->physics.vel_air.y = 0.0F;

        itStarmieNFollowSetStatus(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x80182270
// decomp itstarmie.c:342-351 verbatim.
sb32 itStarmieCommonProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR) != FALSE)
    {
        ip->physics.vel_air.y = 0.0F;
    }
    return FALSE;
}

// 0x801822B0
// decomp itstarmie.c:354-379 verbatim.
GObj* itStarmieMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITStarmieItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        DObj *dobj = DObjGetStruct(item_gobj);
        ITStruct *ip = itGetStruct(item_gobj);

        ip->multi = ITMONSTER_RISE_STOP_WAIT;

        ip->physics.vel_air.x = ip->physics.vel_air.z = 0.0F;
        ip->physics.vel_air.y = ITMONSTER_RISE_VEL_Y;

        gcAddXObjForDObjFixed(dobj, 0x48, 0);

        dobj->translate.vec.f = *pos;

        dobj->translate.vec.f.y -= ip->attr->map_coll_bottom;

        gcAddDObjAnimJoint(dobj, itGetMonsterAnimNode(ip, &llITCommonDataStarmieDataStart), 0.0F);

        gcMoveGObjDLHead(item_gobj, 18, item_gobj->dl_link_priority);
    }
    return item_gobj;
}

// 0x801823B4
// decomp itstarmie.c:382-393 verbatim.
sb32 itStarmieWeaponSwiftProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    wp->physics.vel_air.x = wp->physics.vel_air.x; // Bruh

    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x801823E8
// decomp itstarmie.c:396-403 verbatim.
sb32 itStarmieWeaponSwiftProcHit(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    efManagerStarSplashMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, wp->lr);

    return TRUE;
}

// 0x80182418
// decomp itstarmie.c:406-422 verbatim.
sb32 itStarmieWeaponSwiftProcHop(GObj *weapon_gobj)
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

// 0x801824C0
// decomp itstarmie.c:425-438 verbatim.
sb32 itStarmieWeaponSwiftProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);

    wpMainReflectorSetLR(wp, fp);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.z = syUtilsArcTan2(wp->physics.vel_air.y, wp->physics.vel_air.x);
    DObjGetStruct(weapon_gobj)->scale.vec.f.x = 1.0F;

    wp->lr = -wp->lr;

    return FALSE;
}

// 0x80182530
// decomp itstarmie.c:441-472 verbatim.
GObj* itStarmieWeaponSwiftMakeWeapon(GObj *item_gobj, Vec3f *pos)
{
    ITStruct *ip = itGetStruct(item_gobj);
    GObj *weapon_gobj = wpManagerMakeWeapon(item_gobj, &dITStarmieWeaponSwiftWeaponDesc, pos, WEAPON_FLAG_PARENT_ITEM);
    DObj *dobj;
    s32 unused;
    WPStruct *wp;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);

    wp->lr = ip->lr;

    wp->physics.vel_air.x = wp->lr * ITSTARMIE_SWIFTVEL_X;

    dobj = DObjGetStruct(weapon_gobj);

    dobj->translate.vec.f = *pos;

    efManagerSparkleWhiteScaleMakeEffect(&dobj->translate.vec.f, 1.0F);

    wp->lifetime = ITSTARMIE_SWIFT_LIFETIME;

    if (wp->lr == +1)
    {
        dobj->rotate.vec.f.y = F_CST_DTOR32(180.0F);
    }
    return weapon_gobj;
}

// 0x80182608
// decomp itstarmie.c:475-478 verbatim.
void itStarmieAttackMakeSwift(GObj *item_gobj, Vec3f *pos)
{
    itStarmieWeaponSwiftMakeWeapon(item_gobj, pos);
}

#endif /* NDS_P2_ITEM_CORE */
