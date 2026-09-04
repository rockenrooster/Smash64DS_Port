/* P2 Nyars / Meowth (kind nITKindNyars). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itmonster/itnyars.c:1-300.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0x880 (reloc_data.us.h:3766), the coin weapon row is 0x8C8 (:3767), and
 * the monster anim node base is 0xC130 (:3815); the port's generated reloc
 * header does not publish Nyars tokens, so this TU owns its uintptr_t
 * tokens the same way battleship_item_gbumper.c owns GBumper's (local
 * tokens, no generator involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITNYARS_ and ITMONSTER_ tuning
 * is present in include/it/item.h; itGetMonsterAnimNode, the SFX/voice IDs,
 * and the coin weapon-vars slot are not) are referenced verbatim and listed
 * in the task report -- no values invented here.
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

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3766. */
uintptr_t llITCommonDataNyarsItemAttributes = 0x880u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3767. */
uintptr_t llITCommonDataNyarsCoinWeaponAttributes = 0x8C8u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3815. */
uintptr_t llITCommonDataNyarsAnimJoint = 0xC130u;

extern void *gITManagerCommonData;

/* decomp sys/objanim.h:16. No port header in this TU's chain publishes it;
 * battleship_item_bombhei.c:53-55 carries the same kind of local externs. */
extern void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                               f32 anim_frame);

/* decomp sys/vector.h:40, :42 and sys/utils.h:8. Same seam as
 * battleship_item_lgun.c:77-79. */
extern Vec3f *syVectorRotate3D(Vec3f *dst, s32 axis, f32 angle);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
extern f32 syUtilsArcTan2(f32 y, f32 x);
#ifndef SYVECTOR_AXIS_Z
#define SYVECTOR_AXIS_Z 4
#endif

/* decomp ef/efmanager.h:71. The port defines it
 * (battleship_efmanager.c:2242); no port header in this TU's chain
 * publishes it, so the prototype travels here verbatim. */
extern LBParticle *efManagerDamageCoinMakeEffect(Vec3f *pos);

/* Same shape as battleship_link_bomb.c:80. */
extern void func_800269C0_275C0(u16 sfx_id);

/* decomp itnyars.h:8-19 verbatim. The port publishes no per-kind item procs,
 * so the source header's declarations travel with this TU, exactly as the
 * Tomato and Star files carry theirs. */
extern sb32 itNyarsAttackProcUpdate(GObj *item_gobj);
extern void itNyarsAttackInitVars(GObj *item_gobj);
extern void itNyarsAttackSetStatus(GObj *item_gobj);
extern sb32 itNyarsCommonProcUpdate(GObj *item_gobj);
extern sb32 itNyarsCommonProcMap(GObj *item_gobj);
extern GObj* itNyarsMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
extern sb32 itNyarsWeaponCoinProcUpdate(GObj *weapon_gobj);
extern sb32 itNyarsWeaponCoinProcHit(GObj *weapon_gobj);
extern sb32 itNyarsWeaponCoinProcHop(GObj *weapon_gobj);
extern sb32 itNyarsWeaponCoinProcReflector(GObj *weapon_gobj);
extern GObj* itNyarsWeaponCoinMakeWeapon(GObj *item_gobj, u8 coin_number, f32 rotate_angle);
extern void itNyarsAttackMakeCoin(GObj *item_gobj, f32 angle);

// 0x8018ACA0
// decomp itnyars.c:13-35 verbatim, adapted only for the port's ITDesc shape
// (o_attributes is const void * here, lbRelocGetFileData takes the token the
// same way).
ITDesc dITNyarsItemDesc =
{
    nITKindNyars,                           // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataNyarsItemAttributes,     // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyRSca,         // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    nGMAttackStateNew,                      // Hitbox Update State
    itNyarsCommonProcUpdate,                // Proc Update
    itNyarsCommonProcMap,                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x8018ACD4
// decomp itnyars.c:38-51 verbatim.
ITStatusDesc dITNyarsStatusDescs[/* */] =
{
    // Status 0 (Neutral Attack)
    {
        itNyarsAttackProcUpdate,            // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// 0x8018ACF4
// decomp itnyars.c:54-76 verbatim.
WPDesc dITNyarsWeaponCoinWeaponDesc =
{
    0x01,                                     // Render flags?
    nWPKindNyarsCoin,                         // Weapon Kind
    &gITManagerCommonData,                    // Pointer to character's loaded files?
    &llITCommonDataNyarsCoinWeaponAttributes, // Offset of weapon attributes in loaded files

    // DObj transformation struct
    {
        nGCMatrixKindNull,                  // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    itNyarsWeaponCoinProcUpdate,            // Proc Update
    NULL,                                   // Proc Map
    itNyarsWeaponCoinProcHit,               // Proc Hit
    itNyarsWeaponCoinProcHit,               // Proc Shield
    itNyarsWeaponCoinProcHop,               // Proc Hop
    itNyarsWeaponCoinProcHit,               // Proc Set-Off
    itNyarsWeaponCoinProcReflector,         // Proc Reflector
    itNyarsWeaponCoinProcHit                // Proc Absorb
};

// decomp itnyars.c:84-88 verbatim.
enum itNyarsStatus
{
    nITNyarsStatusAttack,
    nITNyarsStatusEnumCount
};

// 0x8017EEB0
// decomp itnyars.c:97-126 verbatim.
sb32 itNyarsAttackProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        return TRUE;
    }
    if (ip->multi == ip->item_vars.nyars.coin_spawn_wait)
    {
        itNyarsAttackMakeCoin(item_gobj, ip->item_vars.nyars.coin_rotate_step * ITNYARS_COIN_ANGLE_STEP);

        ip->item_vars.nyars.coin_rotate_step++;
        ip->item_vars.nyars.coin_spawn_wait = ip->multi - ITNYARS_COIN_SPAWN_WAIT;

        func_800269C0_275C0(nSYAudioFGMNyarsCoin);
    }
    if (ip->item_vars.nyars.model_rotate_wait == 0)
    {
        dobj->rotate.vec.f.y += F_CST_DTOR32(180.0F);

        ip->item_vars.nyars.model_rotate_wait = ITNYARS_MODEL_ROTATE_WAIT;
    }
    ip->item_vars.nyars.model_rotate_wait--;

    ip->multi--;

    return FALSE;
}

// 0x8017EFA0
// decomp itnyars.c:129-138 verbatim.
void itNyarsAttackInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = ITNYARS_LIFETIME;

    ip->item_vars.nyars.coin_spawn_wait = ip->multi - (ITNYARS_COIN_SPAWN_WAIT / 2);
    ip->item_vars.nyars.coin_rotate_step = 0;
    ip->item_vars.nyars.model_rotate_wait = ITNYARS_MODEL_ROTATE_WAIT;
}

// 0x8017EFC4
// decomp itnyars.c:141-145 verbatim.
void itNyarsAttackSetStatus(GObj *item_gobj)
{
    itNyarsAttackInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITNyarsStatusDescs, nITNyarsStatusAttack);
}

// 0x8017EFF8
// decomp itnyars.c:148-161 verbatim.
sb32 itNyarsCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        ip->physics.vel_air.x = ip->physics.vel_air.y = 0.0F;

        itNyarsAttackSetStatus(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x8017F04C
// decomp itnyars.c:164-173 verbatim.
sb32 itNyarsCommonProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR) != FALSE)
    {
        ip->physics.vel_air.y = 0.0F;
    }
    return FALSE;
}

// 0x8017F08C
// decomp itnyars.c:176-202 verbatim.
GObj* itNyarsMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITNyarsItemDesc, pos, vel, flags);
    DObj *dobj;
    ITStruct *ip;

    if (item_gobj != NULL)
    {
        dobj = DObjGetStruct(item_gobj);

        gcAddXObjForDObjFixed(dobj, 0x48, 0);

        dobj->translate.vec.f = *pos;

        ip = itGetStruct(item_gobj);

        ip->multi = ITMONSTER_RISE_STOP_WAIT;

        ip->physics.vel_air.x = ip->physics.vel_air.z = 0.0F;
        ip->physics.vel_air.y = ITMONSTER_RISE_VEL_Y;

        dobj->translate.vec.f.y -= ip->attr->map_coll_bottom;

        gcAddDObjAnimJoint(dobj, itGetMonsterAnimNode(ip, &llITCommonDataNyarsAnimJoint), 0.0F);
    }
    return item_gobj;
}

// 0x8017F17C
// decomp itnyars.c:205-216 verbatim.
sb32 itNyarsWeaponCoinProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if (wp->weapon_vars.coin.lifetime == 0)
    {
        return TRUE;
    }
    wp->weapon_vars.coin.lifetime--;

    return FALSE;
}

// 0x8017F1A4
// decomp itnyars.c:219-224 verbatim.
sb32 itNyarsWeaponCoinProcHit(GObj *weapon_gobj)
{
    efManagerDamageCoinMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f);

    return TRUE;
}

// 0x8017F1CC
// decomp itnyars.c:227-243 verbatim.
sb32 itNyarsWeaponCoinProcHop(GObj *weapon_gobj)
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

// 0x8017F274
// decomp itnyars.c:246-259 verbatim.
sb32 itNyarsWeaponCoinProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);

    wpMainReflectorSetLR(wp, fp);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.z = syUtilsArcTan2(wp->physics.vel_air.y, wp->physics.vel_air.x);
    DObjGetStruct(weapon_gobj)->scale.vec.f.x = 1.0F;

    wp->lr = -wp->lr;

    return FALSE;
}

// 0x8017F2E4
// decomp itnyars.c:262-289 verbatim.
GObj* itNyarsWeaponCoinMakeWeapon(GObj *item_gobj, u8 coin_number, f32 rotate_angle)
{
    WPStruct *wp;
    GObj *weapon_gobj = wpManagerMakeWeapon(item_gobj, &dITNyarsWeaponCoinWeaponDesc, &DObjGetStruct(item_gobj)->translate.vec.f, WEAPON_FLAG_PARENT_ITEM);
    DObj *dobj;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);

    wp->weapon_vars.coin.lifetime = ITNYARS_COIN_LIFETIME;

    wp->physics.vel_air.y = wp->physics.vel_air.z = 0.0F;
    wp->physics.vel_air.x = ITNYARS_COIN_VEL_X;

    syVectorRotate3D(&wp->physics.vel_air, SYVECTOR_AXIS_Z, F_CLC_DTOR32((coin_number * ITNYARS_COIN_ANGLE_DIFF) + rotate_angle));

    dobj = DObjGetStruct(weapon_gobj);

    gcAddXObjForDObjFixed(dobj, nGCMatrixKindTraRotRpyRSca, 0);
    gcAddXObjForDObjFixed(dobj, 0x46, 0);

    dobj->translate.vec.f = DObjGetStruct(item_gobj)->translate.vec.f;

    return weapon_gobj;
}

// 0x8017F408
// decomp itnyars.c:292-300 verbatim.
void itNyarsAttackMakeCoin(GObj *item_gobj, f32 angle)
{
    s32 coin_count;

    for (coin_count = 0; coin_count < ITNYARS_COIN_SPAWN_MAX; coin_count++)
    {
        itNyarsWeaponCoinMakeWeapon(item_gobj, coin_count, angle);
    }
}

#endif /* NDS_P2_ITEM_CORE */
