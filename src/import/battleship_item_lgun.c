/* P2 Ray Gun (nITKindLGun). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itlgun.c:12-369.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x268) and the ammo weapon-attribute row (0x2B0) live below
 * (decomp/BattleShip-main/include/reloc_data.us.h:3740, :3741); the port's
 * generated reloc header does not publish LGun tokens, so this TU owns its
 * uintptr_t tokens the same way battleship_item_box.c:38-45 owns Box's
 * (local tokens, no generator involvement, no hand-edited generated file).
 *
 * This is the first AMMO SHOOTER: the item carries its ammunition count in
 * ip->multi (ITLGUN_AMMO_MAX) and owns a wp/ projectile via
 * itLGunWeaponAmmoMakeWeapon / itLGunMakeAmmo -- the item-owns-a-weapon
 * pattern the Poke Ball Pokemon reuse later.
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITLGUN_* tuning,
 * itMapCheckDestroyLanding) are referenced verbatim and listed in the
 * task report -- no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <ef/effect.h>
#include <if/interface.h>
#include <wp/weapon.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3740. */
uintptr_t llITCommonDataLGunItemAttributes = 0x268u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3741. */
uintptr_t llITCommonDataLGunAmmoWeaponAttributes = 0x2B0u;

extern void *gITManagerCommonData;

/* decomp itlgun.h:8-26. The port publishes no per-kind item procs, so the
 * source header's declarations travel with this TU, exactly as the Box file
 * carries its own. */
sb32 itLGunFallProcUpdate(GObj *item_gobj);
sb32 itLGunWaitProcMap(GObj *item_gobj);
sb32 itLGunFallProcMap(GObj *item_gobj);
void itLGunWaitSetStatus(GObj *item_gobj);
void itLGunFallSetStatus(GObj *item_gobj);
void itLGunHoldSetStatus(GObj *item_gobj);
sb32 itLGunThrownProcMap(GObj *item_gobj);
sb32 itLGunCommonProcHit(GObj *item_gobj);
void itLGunThrownSetStatus(GObj *item_gobj);
sb32 itLGunDroppedProcMap(GObj *item_gobj);
void itLGunDroppedSetStatus(GObj *item_gobj);
GObj* itLGunMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
sb32 itLGunWeaponAmmoProcUpdate(GObj *weapon_gobj);
sb32 itLGunWeaponAmmoProcMap(GObj *weapon_gobj);
sb32 itLGunWeaponAmmoProcHit(GObj *weapon_gobj);
sb32 itLGunWeaponAmmoProcHop(GObj *weapon_gobj);
sb32 itLGunWeaponAmmoProcReflector(GObj *weapon_gobj);
GObj* itLGunWeaponAmmoMakeWeapon(GObj *fighter_gobj, Vec3f *pos);
void itLGunMakeAmmo(GObj *fighter_gobj, Vec3f *pos);

/* No port header publishes these yet (cf. battleship_fox_blaster.c:40 and
 * battleship_item_box.c:80-87, which declare their missing imports the same
 * way). itMapCheckDestroyLanding is the port-missing helper the empty-gun
 * thrown/dropped maps need; it lives in the decomp it/itmap.c this port
 * imports whole into battleship_item_link_core.c:1737. The effect makers and
 * wpMapTestAllCheckCollEnd are proven present in the linked ROM text;
 * syUtils*/syVector* follow the link_core.c:207 / vector.h:42 shapes. */
extern sb32 itMapCheckDestroyLanding(GObj *item_gobj, f32 common_rebound);
extern sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);
extern LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
extern LBParticle *efManagerImpactShockMakeEffect(Vec3f *pos, s32 size);
extern f32 syUtilsArcTan2(f32 y, f32 x);
extern u16 syUtilsRandUShort(void);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);

/* decomp itlgun.c:12-34 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITLGunItemDesc =
{
    nITKindLGun,                            /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataLGunItemAttributes,      /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyR,            /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itLGunFallProcUpdate,                   /* Proc Update */
    itLGunFallProcMap,                      /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itlgun.c:36-97 verbatim. */
ITStatusDesc dITLGunStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itLGunWaitProcMap,                  /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itLGunFallProcUpdate,               /* Proc Update */
        itLGunFallProcMap,                  /* Proc Map */
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
        itLGunFallProcUpdate,               /* Proc Update */
        itLGunThrownProcMap,                /* Proc Map */
        itLGunCommonProcHit,                /* Proc Hit */
        itLGunCommonProcHit,                /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itLGunCommonProcHit,                /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itLGunFallProcUpdate,               /* Proc Update */
        itLGunDroppedProcMap,               /* Proc Map */
        itLGunCommonProcHit,                /* Proc Hit */
        itLGunCommonProcHit,                /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itLGunCommonProcHit,                /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itlgun.c:99-121 verbatim, adapted only for the port's WPDesc
 * shape (o_attributes is intptr_t here; the token address travels the
 * same way). */
WPDesc dITLGunAmmoWeaponDesc =
{
    0x00,                                   /* Render flags? */
    nWPKindLGunAmmo,                        /* Weapon Kind */
    &gITManagerCommonData,                  /* Pointer to character's loaded files? */
    &llITCommonDataLGunAmmoWeaponAttributes,/* Offset of weapon attributes in loaded files */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyRSca,         /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    itLGunWeaponAmmoProcUpdate,             /* Proc Update */
    itLGunWeaponAmmoProcMap,                /* Proc Map */
    itLGunWeaponAmmoProcHit,                /* Proc Hit */
    itLGunWeaponAmmoProcHit,                /* Proc Shield */
    itLGunWeaponAmmoProcHop,                /* Proc Hop */
    itLGunWeaponAmmoProcHit,                /* Proc Set-Off */
    itLGunWeaponAmmoProcReflector,          /* Proc Reflector */
    itLGunWeaponAmmoProcHit                 /* Proc Absorb */
};

/* decomp itlgun.c:129-137 verbatim. */
enum itLGunStatus
{
    nITLGunStatusWait,
    nITLGunStatusFall,
    nITLGunStatusHold,
    nITLGunStatusThrown,
    nITLGunStatusDropped,
    nITLGunStatusEnumCount
};

/* decomp itlgun.c:146-154 verbatim. */
sb32 itLGunFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITLGUN_GRAVITY, ITLGUN_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itlgun.c:157-162 verbatim. */
sb32 itLGunWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itLGunFallSetStatus);

    return FALSE;
}

/* decomp itlgun.c:165-168 verbatim. */
sb32 itLGunFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITLGUN_MAP_REBOUND_COMMON, ITLGUN_MAP_REBOUND_GROUND, itLGunWaitSetStatus);
}

/* decomp itlgun.c:171-175 verbatim. */
void itLGunWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITLGunStatusDescs, nITLGunStatusWait);
}

/* decomp itlgun.c:178-186 verbatim. */
void itLGunFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITLGunStatusDescs, nITLGunStatusFall);
}

/* decomp itlgun.c:189-194 verbatim. */
void itLGunHoldSetStatus(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->rotate.vec.f.y = F_CST_DTOR32(0.0F);

    itMainSetStatus(item_gobj, dITLGunStatusDescs, nITLGunStatusHold);
}

/* decomp itlgun.c:197-206 verbatim. */
sb32 itLGunThrownProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        return itMapCheckDestroyLanding(item_gobj, ITLGUN_MAP_REBOUND_COMMON);
    }
    else return itMapCheckDestroyDropped(item_gobj, ITLGUN_MAP_REBOUND_COMMON, ITLGUN_MAP_REBOUND_GROUND, itLGunWaitSetStatus);
}

/* decomp itlgun.c:209-218 verbatim. */
sb32 itLGunCommonProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp itlgun.c:221-228 verbatim. */
void itLGunThrownSetStatus(GObj *item_gobj)
{
    s32 lr = ftGetStruct(itGetStruct(item_gobj)->owner_gobj)->lr;

    itMainSetStatus(item_gobj, dITLGunStatusDescs, nITLGunStatusThrown);

    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = (lr == -1) ? F_CST_DTOR32(-90.0F) : F_CST_DTOR32(90.0F);
}

/* decomp itlgun.c:231-240 verbatim. */
sb32 itLGunDroppedProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        return itMapCheckDestroyLanding(item_gobj, ITLGUN_MAP_REBOUND_COMMON);
    }
    else return itMapCheckDestroyDropped(item_gobj, ITLGUN_MAP_REBOUND_COMMON, ITLGUN_MAP_REBOUND_GROUND, itLGunWaitSetStatus);
}

/* decomp itlgun.c:243-250 verbatim. */
void itLGunDroppedSetStatus(GObj *item_gobj)
{
    s32 lr = ftGetStruct(itGetStruct(item_gobj)->owner_gobj)->lr;

    itMainSetStatus(item_gobj, dITLGunStatusDescs, nITLGunStatusDropped);

    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = (lr == -1) ? F_CST_DTOR32(-90.0F) : F_CST_DTOR32(90.0F);
}

/* decomp itlgun.c:253-270 verbatim. */
GObj* itLGunMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITLGunItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        ip->multi = ITLGUN_AMMO_MAX;

        DObjGetStruct(item_gobj)->rotate.vec.f.y = ((syUtilsRandUShort() % 2) != 0) ? F_CST_DTOR32(90.0F) : F_CST_DTOR32(-90.0F);

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

/* decomp itlgun.c:273-291 verbatim. */
sb32 itLGunWeaponAmmoProcUpdate(GObj *weapon_gobj)
{
    DObj *dobj = DObjGetStruct(weapon_gobj);

    if (dobj->scale.vec.f.x < ITLGUN_AMMO_CLAMP_SCALE_X)
    {
        dobj->scale.vec.f.x += ITLGUN_AMMO_STEP_SCALE_X;

    #if !defined (DAIRANTOU_OPT0)
        dobj = DObjGetStruct(weapon_gobj); /* Y tho lol */
    #endif

        if (dobj->scale.vec.f.x > ITLGUN_AMMO_CLAMP_SCALE_X)
        {
            dobj->scale.vec.f.x = ITLGUN_AMMO_CLAMP_SCALE_X;
        }
    }
    return FALSE;
}

/* decomp itlgun.c:294-303 verbatim. */
sb32 itLGunWeaponAmmoProcMap(GObj *weapon_gobj)
{
    if (wpMapTestAllCheckCollEnd(weapon_gobj) != FALSE)
    {
        efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

        return TRUE;
    }
    else return FALSE;
}

/* decomp itlgun.c:306-313 verbatim. */
sb32 itLGunWeaponAmmoProcHit(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    efManagerImpactShockMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, wp->attack_coll.damage);

    return TRUE;
}

/* decomp itlgun.c:316-326 verbatim. */
sb32 itLGunWeaponAmmoProcHop(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    syVectorRotateAbout3D(&wp->physics.vel_air, &wp->shield_collide_dir, wp->shield_collide_angle * 2);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.z = syUtilsArcTan2(wp->physics.vel_air.y, wp->physics.vel_air.x);
    DObjGetStruct(weapon_gobj)->scale.vec.f.x = 1.0F;

    return FALSE;
}

/* decomp itlgun.c:329-340 verbatim. */
sb32 itLGunWeaponAmmoProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);

    wpMainReflectorSetLR(wp, fp);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.z = syUtilsArcTan2(wp->physics.vel_air.y, wp->physics.vel_air.x);
    DObjGetStruct(weapon_gobj)->scale.vec.f.x = 1.0F;

    return FALSE;
}

/* decomp itlgun.c:343-359 verbatim. */
GObj* itLGunWeaponAmmoMakeWeapon(GObj *fighter_gobj, Vec3f *pos)
{
    GObj *weapon_gobj = wpManagerMakeWeapon(fighter_gobj, &dITLGunAmmoWeaponDesc, pos, (WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER));
    WPStruct *wp;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);

    wp->physics.vel_air.x = wp->lr * ITLGUN_AMMO_VEL_X;

    DObjGetStruct(weapon_gobj)->rotate.vec.f.z = syUtilsArcTan2(wp->physics.vel_air.y, wp->physics.vel_air.x);

    return weapon_gobj;
}

/* decomp itlgun.c:362-369 verbatim. */
void itLGunMakeAmmo(GObj *fighter_gobj, Vec3f *pos)
{
    ITStruct *ip = itGetStruct(ftGetStruct(fighter_gobj)->item_gobj);

    itLGunWeaponAmmoMakeWeapon(fighter_gobj, pos);

    ip->multi--;
}

#endif /* NDS_P2_ITEM_CORE */
