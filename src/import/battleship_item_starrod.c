/* P2 Star Rod (nITKindStarRod). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itstarrod.c:12-405.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x48C), the star weapon-attribute row (0x4D4) and the smash
 * weapon-attribute row (0x508) live below
 * (decomp/BattleShip-main/include/reloc_data.us.h:3750, :3751, :3752); the
 * port's generated reloc header does not publish StarRod tokens, so this TU
 * owns its uintptr_t tokens the same way battleship_item_lgun.c owns LGun's
 * (local tokens, no generator involvement, no hand-edited generated file).
 *
 * This is the third AMMO SHOOTER: the item carries its ammunition count in
 * ip->multi (ITSTARROD_AMMO_MAX) and owns a wp/ projectile via
 * itStarRodWeaponStarMakeWeapon / itStarRodMakeStar -- the item-owns-a-weapon
 * pattern the Poke Ball Pokemon reuse later. The smash swing swaps the live
 * weapon descriptor's attribute row, exactly as the source does. The star's
 * per-weapon countdown lives in the decomp wpStarRodWeaponVarsStar payload.
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITSTARROD_* tuning,
 * itMapCheckDestroyLanding, wpStarRodWeaponVarsStar,
 * efManagerStarRodSparkMakeEffect, efManagerStarSplashMakeEffect) are
 * referenced verbatim and listed in the task report -- no values invented
 * here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <ef/effect.h>
#include <if/interface.h>
#include <gm/gmsound.h>
#include <wp/weapon.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3750. */
uintptr_t llITCommonDataStarRodItemAttributes = 0x48Cu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3751. */
uintptr_t llITCommonDataStarRodWeaponAttributes = 0x4D4u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3752. */
uintptr_t llITCommonDataStarRodSmashWeaponAttributes = 0x508u;

extern void *gITManagerCommonData;

/* decomp itstarrod.h:8-27. The port publishes no per-kind item procs, so the
 * source header's declarations travel with this TU, exactly as the Box file
 * carries its own. */
sb32 itStarRodFallProcUpdate(GObj *item_gobj);
sb32 itStarRodWaitProcMap(GObj *item_gobj);
sb32 itStarRodFallProcMap(GObj *item_gobj);
void itStarRodWaitSetStatus(GObj *item_gobj);
void itStarRodFallSetStatus(GObj *item_gobj);
void itStarRodHoldSetStatus(GObj *item_gobj);
sb32 itStarRodThrownProcUpdate(GObj *item_gobj);
sb32 itStarRodThrownProcMap(GObj *item_gobj);
sb32 itStarRodThrownProcHit(GObj *item_gobj);
void itStarRodThrownSetStatus(GObj *item_gobj);
sb32 itStarRodDroppedProcMap(GObj *item_gobj);
void itStarRodDroppedSetStatus(GObj *item_gobj);
GObj* itStarRodMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
sb32 itStarRodWeaponStarProcUpdate(GObj *weapon_gobj);
sb32 itStarRodWeaponStarProcMap(GObj *weapon_gobj);
sb32 itStarRodWeaponStarProcHit(GObj *weapon_gobj);
sb32 itStarRodWeaponStarProcHop(GObj *weapon_gobj);
sb32 itStarRodWeaponStarProcReflector(GObj *weapon_gobj);
GObj* itStarRodWeaponStarMakeWeapon(GObj *fighter_gobj, Vec3f *pos, ub8 is_smash);
void itStarRodMakeStar(GObj *fighter_gobj, Vec3f *pos, ub8 is_smash);

/* No port header publishes these yet (cf. battleship_item_box.c:80-87, which
 * declares its missing imports the same way). itMapCheckDestroyLanding lives
 * in the decomp it/itmap.c this port imports whole into
 * battleship_item_link_core.c:1737. The star makers live in the decomp
 * ef/efmanager.c (efmanager.c:3386, :4721) this port has not imported yet, so
 * their source signatures travel with this TU. wpMapTestAllCheckCollEnd and
 * the SparkleWhite scaler are proven present in the linked ROM text;
 * syUtils and syVector helpers follow the link_core.c:207 / vector.h:42 shapes. */
extern sb32 itMapCheckDestroyLanding(GObj *item_gobj, f32 common_rebound);
extern sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);
extern LBParticle *efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale);
extern GObj *efManagerStarRodSparkMakeEffect(Vec3f *pos, s32 lr);
extern LBGenerator *efManagerStarSplashMakeEffect(Vec3f *pos, s32 lr);
extern f32 syUtilsArcTan2(f32 y, f32 x);
extern s32 syUtilsRandIntRange(s32 range);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);

/* decomp itstarrod.c:12-34 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITStarRodItemDesc =
{
    nITKindStarRod,                         /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataStarRodItemAttributes,   /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyR,            /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itStarRodFallProcUpdate,                /* Proc Update */
    itStarRodFallProcMap,                   /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itstarrod.c:36-97 verbatim. */
ITStatusDesc dITStarRodStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itStarRodWaitProcMap,               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itStarRodFallProcUpdate,            /* Proc Update */
        itStarRodFallProcMap,               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 2 (Fighter Hold) */
    {
        NULL,                               /* Proc Update */
        NULL,                               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 3 (Fighter Throw) */
    {
        itStarRodThrownProcUpdate,          /* Proc Update */
        itStarRodThrownProcMap,             /* Proc Map */
        itStarRodThrownProcHit,             /* Proc Hit */
        itStarRodThrownProcHit,             /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itStarRodThrownProcHit,             /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itStarRodFallProcUpdate,            /* Proc Update */
        itStarRodDroppedProcMap,            /* Proc Map */
        itStarRodThrownProcHit,             /* Proc Hit */
        itStarRodThrownProcHit,             /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itStarRodThrownProcHit,             /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itstarrod.c:99-121 verbatim, adapted only for the port's WPDesc
 * shape (o_attributes is intptr_t here; the token address travels the
 * same way). */
WPDesc dITStarRodWeaponStarWeaponDesc =
{
    0x00,                                   /* Render flags? */
    nWPKindStarRodStar,                     /* Weapon Kind */
    &gITManagerCommonData,                  /* Pointer to character's loaded files? */
    &llITCommonDataStarRodWeaponAttributes, /* Offset of weapon attributes in loaded files */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyRSca,         /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    itStarRodWeaponStarProcUpdate,          /* Proc Update */
    itStarRodWeaponStarProcMap,             /* Proc Map */
    itStarRodWeaponStarProcHit,             /* Proc Hit */
    itStarRodWeaponStarProcHit,             /* Proc Shield */
    itStarRodWeaponStarProcHop,             /* Proc Hop */
    itStarRodWeaponStarProcHit,             /* Proc Set-Off */
    itStarRodWeaponStarProcReflector,       /* Proc Reflector */
    itStarRodWeaponStarProcHit              /* Proc Absorb */
};

/* decomp itstarrod.c:129-137 verbatim. */
enum itStarRodStatus
{
    nITStarRodStatusWait,
    nITStarRodStatusFall,
    nITStarRodStatusHold,
    nITStarRodStatusThrown,
    nITStarRodStatusDropped,
    nITStarRodStatusEnumCount
};

/* decomp itstarrod.c:146-154 verbatim. */
sb32 itStarRodFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITSTARROD_GRAVITY, ITSTARROD_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itstarrod.c:157-162 verbatim. */
sb32 itStarRodWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itStarRodFallSetStatus);

    return FALSE;
}

/* decomp itstarrod.c:165-170 verbatim. */
sb32 itStarRodFallProcMap(GObj *item_gobj)
{
    itMapCheckDestroyDropped(item_gobj, ITSTARROD_MAP_REBOUND_COMMON, ITSTARROD_MAP_REBOUND_GROUND, itStarRodWaitSetStatus);

    return FALSE;
}

/* decomp itstarrod.c:173-177 verbatim. */
void itStarRodWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITStarRodStatusDescs, nITStarRodStatusWait);
}

/* decomp itstarrod.c:180-188 verbatim. */
void itStarRodFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITStarRodStatusDescs, nITStarRodStatusFall);
}

/* decomp itstarrod.c:191-196 verbatim. */
void itStarRodHoldSetStatus(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->rotate.vec.f.y = F_CST_DTOR32(0.0F);

    itMainSetStatus(item_gobj, dITStarRodStatusDescs, nITStarRodStatusHold);
}

/* decomp itstarrod.c:199-207 verbatim. */
sb32 itStarRodThrownProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITSTARROD_GRAVITY, ITSTARROD_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itstarrod.c:210-213 verbatim. */
sb32 itStarRodThrownProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITSTARROD_MAP_REBOUND_COMMON, ITSTARROD_MAP_REBOUND_GROUND, itStarRodWaitSetStatus);
}

/* decomp itstarrod.c:216-225 verbatim. */
sb32 itStarRodThrownProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp itstarrod.c:228-232 verbatim. */
void itStarRodThrownSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITStarRodStatusDescs, nITStarRodStatusThrown);
    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(90.0F);
}

/* decomp itstarrod.c:235-244 verbatim. */
sb32 itStarRodDroppedProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        return itMapCheckDestroyLanding(item_gobj, ITSTARROD_MAP_REBOUND_COMMON);
    }
    else return itMapCheckDestroyDropped(item_gobj, ITSTARROD_MAP_REBOUND_COMMON, ITSTARROD_MAP_REBOUND_GROUND, itStarRodWaitSetStatus);
}

/* decomp itstarrod.c:247-251 verbatim. */
void itStarRodDroppedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITStarRodStatusDescs, nITStarRodStatusDropped);
    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(90.0F);
}

/* decomp itstarrod.c:254-269 verbatim. */
GObj* itStarRodMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITStarRodItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        ip->multi = ITSTARROD_AMMO_MAX;

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

/* decomp itstarrod.c:272-302 verbatim. */
sb32 itStarRodWeaponStarProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    Vec3f pos;
    DObj *dobj;

    if (wp->weapon_vars.star.lifetime == 0)
    {
        DObjGetStruct(weapon_gobj)->flags = DOBJ_FLAG_HIDDEN;

        efManagerSparkleWhiteScaleMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

        return TRUE;
    }

    wp->weapon_vars.star.lifetime--;

    dobj = DObjGetStruct(weapon_gobj);

    dobj->rotate.vec.f.z += (-0.2F * wp->lr);

    if (wp->weapon_vars.star.lifetime % 2)
    {
        pos.x = DObjGetStruct(weapon_gobj)->translate.vec.f.x;
        pos.y = syUtilsRandIntRange(250) + (DObjGetStruct(weapon_gobj)->translate.vec.f.y - 125.0F);
        pos.z = 0.0F;

        efManagerStarRodSparkMakeEffect(&pos, wp->lr * -1.0F);
    }
    return FALSE;
}

/* decomp itstarrod.c:305-318 verbatim. */
sb32 itStarRodWeaponStarProcMap(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if (wpMapTestAllCheckCollEnd(weapon_gobj) != FALSE)
    {
        efManagerStarSplashMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, wp->lr);

        func_800269C0_275C0(nSYAudioFGMStarMapCollide);

        return TRUE;
    }
    else return FALSE;
}

/* decomp itstarrod.c:321-328 verbatim. */
sb32 itStarRodWeaponStarProcHit(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    efManagerStarSplashMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, wp->lr);

    return TRUE;
}

/* decomp itstarrod.c:331-347 verbatim. */
sb32 itStarRodWeaponStarProcHop(GObj *weapon_gobj)
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

/* decomp itstarrod.c:350-363 verbatim. */
sb32 itStarRodWeaponStarProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);

    wpMainReflectorSetLR(wp, fp);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.z = syUtilsArcTan2(wp->physics.vel_air.y, wp->physics.vel_air.x);
    DObjGetStruct(weapon_gobj)->scale.vec.f.x = 1.0F;

    wp->lr = -wp->lr;

    return FALSE;
}

/* decomp itstarrod.c:366-395 verbatim. */
GObj* itStarRodWeaponStarMakeWeapon(GObj *fighter_gobj, Vec3f *pos, ub8 is_smash)
{
    GObj *weapon_gobj;
    DObj *dobj;
    WPStruct *wp;

    if (is_smash == TRUE)
    {
        dITStarRodWeaponStarWeaponDesc.o_attributes = (intptr_t)&llITCommonDataStarRodSmashWeaponAttributes; /* Set attribute data on smash input - Linker thing */
    }
    weapon_gobj = wpManagerMakeWeapon(fighter_gobj, &dITStarRodWeaponStarWeaponDesc, pos, (WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER));

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(weapon_gobj);
    wp = wpGetStruct(weapon_gobj);

    wp->physics.vel_air.x = ((!(is_smash)) ? ITSTARROD_AMMO_TILTVEL_X : ITSTARROD_AMMO_SMASH_VEL_X) * wp->lr;

    wp->weapon_vars.star.lifetime = (!(is_smash)) ? ITSTARROD_AMMO_TILT_LIFETIME : ITSTARROD_AMMO_SMASH_LIFETIME; /* Why float lol */

    gcAddXObjForDObjFixed(dobj, 0x2E, 0);

    dobj->translate.vec.f = *pos;
    dobj->translate.vec.f.z = 0.0F;

    return weapon_gobj;
}

/* decomp itstarrod.c:398-405 verbatim. */
void itStarRodMakeStar(GObj *fighter_gobj, Vec3f *pos, ub8 is_smash)
{
    ITStruct *ip = itGetStruct(ftGetStruct(fighter_gobj)->item_gobj);

    itStarRodWeaponStarMakeWeapon(fighter_gobj, pos, is_smash);

    ip->multi--;
}

#endif /* NDS_P2_ITEM_CORE */
