/* P2 Fire Flower (nITKindFFlower). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itfflower.c:12-350.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x2E4), the flame weapon-attribute row (0x32C) and the flame-angle table
 * (0x360) live below
 * (decomp/BattleShip-main/include/reloc_data.us.h:3742, :3743, :3744); the
 * port's generated reloc header does not publish FFlower tokens, so this TU
 * owns its uintptr_t tokens the same way battleship_item_lgun.c owns LGun's
 * (local tokens, no generator involvement, no hand-edited generated file).
 *
 * This is the second AMMO SHOOTER: the item carries its ammunition count in
 * ip->multi (ITFFLOWER_AMMO_MAX) and owns a wp/ projectile via
 * itFFlowerWeaponFlameMakeWeapon / itFFlowerShootFlame -- the
 * item-owns-a-weapon pattern the Poke Ball Pokemon reuse later. The flame
 * angles are read out of the reloc file through the attribute-row pointer,
 * exactly as the source does.
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITFFLOWER_* tuning,
 * itMapCheckDestroyLanding, gITManagerParticleBankID) are referenced
 * verbatim and listed in the task report -- no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <common.h>
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

/* decomp/BattleShip-main/include/reloc_data.us.h:3742. */
uintptr_t llITCommonDataFFlowerItemAttributes = 0x2E4u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3743. */
uintptr_t llITCommonDataFFlowerFlameWeaponAttributes = 0x32Cu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3744. */
uintptr_t llITCommonDataFFlowerFlameAngles = 0x360u;

extern void *gITManagerCommonData;

/* decomp itfflower.h:8-25. The port publishes no per-kind item procs, so the
 * source header's declarations travel with this TU, exactly as the Box file
 * carries its own. */
sb32 itFFlowerFallProcUpdate(GObj *item_gobj);
sb32 itFFlowerWaitProcMap(GObj *item_gobj);
sb32 itFFlowerFallProcMap(GObj *item_gobj);
void itFFlowerWaitSetStatus(GObj *item_gobj);
void itFFlowerFallSetStatus(GObj *item_gobj);
void itFFlowerHoldSetStatus(GObj *item_gobj);
sb32 itFFlowerThrownProcMap(GObj *item_gobj);
sb32 itFFlowerCommonProcHit(GObj *item_gobj);
void itFFlowerThrownSetStatus(GObj *item_gobj);
sb32 itFFlowerDroppedProcMap(GObj *item_gobj);
void itFFlowerDroppedSetStatus(GObj *item_gobj);
GObj* itFFlowerMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
sb32 itFFlowerWeaponFlameProcUpdate(GObj *weapon_gobj);
sb32 itFFlowerWeaponFlameProcMap(GObj *weapon_gobj);
sb32 itFFlowerWeaponFlameProcHit(GObj *weapon_gobj);
sb32 itFFlowerWeaponFlameProcReflector(GObj *weapon_gobj);
GObj* itFFlowerWeaponFlameMakeWeapon(GObj *fighter_gobj, Vec3f *pos, Vec3f *vel);
void itFFlowerShootFlame(GObj *fighter_gobj, Vec3f *pos, s32 index, s32 ammo_sub);

/* No port header publishes these yet (cf. battleship_item_box.c:80-87, which
 * declares its missing imports the same way). itMapCheckDestroyLanding lives
 * in the decomp it/itmap.c this port imports whole into
 * battleship_item_link_core.c:1737; gITManagerParticleBankID is the decomp
 * it/itmanager.c:109 particle bank the flame reflector feeds. The effect
 * maker and wpMapTestAllCheckCollEnd are proven present in the linked ROM
 * text; lbParticleMakePosVel rides on <ef/effect.h>. */extern sb32 itMapCheckDestroyLanding(GObj *item_gobj, f32 common_rebound);
extern sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);
extern LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
extern LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos);
extern s32 gITManagerParticleBankID;

/* decomp itfflower.c:12-34 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITFFlowerItemDesc =
{
    nITKindFFlower,                         /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataFFlowerItemAttributes,   /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyR,            /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itFFlowerFallProcUpdate,                /* Proc Update */
    itFFlowerFallProcMap,                   /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itfflower.c:36-97 verbatim. */
ITStatusDesc dITFFlowerStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itFFlowerWaitProcMap,               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itFFlowerFallProcUpdate,            /* Proc Update */
        itFFlowerFallProcMap,               /* Proc Map */
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
        itFFlowerFallProcUpdate,            /* Proc Update */
        itFFlowerThrownProcMap,             /* Proc Map */
        itFFlowerCommonProcHit,             /* Proc Hit */
        itFFlowerCommonProcHit,             /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itFFlowerCommonProcHit,             /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itFFlowerFallProcUpdate,            /* Proc Update */
        itFFlowerDroppedProcMap,            /* Proc Map */
        itFFlowerCommonProcHit,             /* Proc Hit */
        itFFlowerCommonProcHit,             /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itFFlowerCommonProcHit,             /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itfflower.c:99-121 verbatim, adapted only for the port's WPDesc
 * shape (o_attributes is intptr_t here; the token address travels the
 * same way). */
WPDesc dITFFlowerWeaponFlameWeaponDesc =
{
    0x00,                                        /* Render flags? */
    nWPKindFFlowerFlame,                         /* Weapon Kind */
    &gITManagerCommonData,                       /* Pointer to character's loaded files? */
    &llITCommonDataFFlowerFlameWeaponAttributes, /* Offset of weapon attributes in loaded files */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyRSca,          /* Main matrix transformations */
        nGCMatrixKindNull,                   /* Secondary matrix transformations? */
        0                                    /* ??? */
    },

    itFFlowerWeaponFlameProcUpdate,         /* Proc Update */
    itFFlowerWeaponFlameProcMap,            /* Proc Map */
    itFFlowerWeaponFlameProcHit,            /* Proc Hit */
    itFFlowerWeaponFlameProcHit,            /* Proc Shield */
    NULL,                                   /* Proc Hop */
    itFFlowerWeaponFlameProcHit,            /* Proc Set-Off */
    itFFlowerWeaponFlameProcReflector,      /* Proc Reflector */
    NULL                                    /* Proc Absorb */
};

/* decomp itfflower.c:129-137 verbatim. */
enum itFFlowerStatus
{
    nITFFlowerStatusWait,
    nITFFlowerStatusFall,
    nITFFlowerStatusHold,
    nITFFlowerStatusThrown,
    nITFFlowerStatusDropped,
    nITFFlowerStatusEnumCount
};

/* decomp itfflower.c:146-154 verbatim. */
sb32 itFFlowerFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITFFLOWER_GRAVITY, ITFFLOWER_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itfflower.c:157-162 verbatim. */
sb32 itFFlowerWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itFFlowerFallSetStatus);

    return FALSE;
}

/* decomp itfflower.c:165-168 verbatim. */
sb32 itFFlowerFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITFFLOWER_MAP_REBOUND_COMMON, ITFFLOWER_MAP_REBOUND_GROUND, itFFlowerWaitSetStatus);
}

/* decomp itfflower.c:171-175 verbatim. */
void itFFlowerWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITFFlowerStatusDescs, nITFFlowerStatusWait);
}

/* decomp itfflower.c:178-186 verbatim. */
void itFFlowerFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITFFlowerStatusDescs, nITFFlowerStatusFall);
}

/* decomp itfflower.c:189-192 verbatim. */
void itFFlowerHoldSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITFFlowerStatusDescs, nITFFlowerStatusHold);
}

/* decomp itfflower.c:195-204 verbatim. */
sb32 itFFlowerThrownProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        return itMapCheckDestroyLanding(item_gobj, ITFFLOWER_MAP_REBOUND_COMMON);
    }
    else return itMapCheckDestroyDropped(item_gobj, ITFFLOWER_MAP_REBOUND_COMMON, ITFFLOWER_MAP_REBOUND_GROUND, itFFlowerWaitSetStatus);
}

/* decomp itfflower.c:207-216 verbatim. */
sb32 itFFlowerCommonProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp itfflower.c:219-222 verbatim. */
void itFFlowerThrownSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITFFlowerStatusDescs, nITFFlowerStatusThrown);
}

/* decomp itfflower.c:225-234 verbatim. */
sb32 itFFlowerDroppedProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        return itMapCheckDestroyLanding(item_gobj, ITFFLOWER_MAP_REBOUND_COMMON);
    }
    else return itMapCheckDestroyDropped(item_gobj, ITFFLOWER_MAP_REBOUND_COMMON, ITFFLOWER_MAP_REBOUND_GROUND, itFFlowerWaitSetStatus);
}

/* decomp itfflower.c:237-240 verbatim. */
void itFFlowerDroppedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITFFlowerStatusDescs, nITFFlowerStatusDropped);
}

/* decomp itfflower.c:243-258 verbatim. */
GObj* itFFlowerMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITFFlowerItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        ip->multi = ITFFLOWER_AMMO_MAX;

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

/* decomp itfflower.c:261-270 verbatim. */
sb32 itFFlowerWeaponFlameProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        return TRUE;
    }
    else return FALSE;
}

/* decomp itfflower.c:273-282 verbatim. */
sb32 itFFlowerWeaponFlameProcMap(GObj *weapon_gobj)
{
    if (wpMapTestAllCheckCollEnd(weapon_gobj) != FALSE)
    {
        efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

        return TRUE;
    }
    else return FALSE;
}

/* decomp itfflower.c:285-291 verbatim. */
sb32 itFFlowerWeaponFlameProcHit(GObj *weapon_gobj)
{
    func_800269C0_275C0(nSYAudioFGMExplodeS);
    efManagerSparkleWhiteMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f);

    return FALSE;
}

/* decomp itfflower.c:294-310 verbatim. */
sb32 itFFlowerWeaponFlameProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);
    Vec3f *translate;

    wp->lifetime = ITFFLOWER_AMMO_LIFETIME;

    wpMainReflectorSetLR(wp, fp);

    translate = &DObjGetStruct(weapon_gobj)->translate.vec.f;

    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 2, translate->x, translate->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);
    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0, translate->x, translate->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);

    return FALSE;
}

/* decomp itfflower.c:313-334 verbatim. */
GObj* itFFlowerWeaponFlameMakeWeapon(GObj *fighter_gobj, Vec3f *pos, Vec3f *vel)
{
    GObj *weapon_gobj = wpManagerMakeWeapon(fighter_gobj, &dITFFlowerWeaponFlameWeaponDesc, pos, (WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER));
    WPStruct *wp;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);

    wp->physics.vel_air.x = vel->x * wp->lr;
    wp->physics.vel_air.y = vel->y;
    wp->physics.vel_air.z = vel->z;

    wp->lifetime = ITFFLOWER_AMMO_LIFETIME;

    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 2, pos->x, pos->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);
    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0, pos->x, pos->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);

    return weapon_gobj;
}

/* decomp itfflower.c:337-350 verbatim. */
void itFFlowerShootFlame(GObj *fighter_gobj, Vec3f *pos, s32 index, s32 ammo_sub)
{
    ITStruct *ip = itGetStruct(ftGetStruct(fighter_gobj)->item_gobj);
    Vec3f vel;
    f32 *angle = (f32*) ((uintptr_t)*dITFFlowerItemDesc.p_file + (intptr_t) &llITCommonDataFFlowerFlameAngles);

    vel.x = __cosf(angle[index]) * ITFFLOWER_AMMO_VEL;
    vel.y = __sinf(angle[index]) * ITFFLOWER_AMMO_VEL;
    vel.z = 0.0F;

    itFFlowerWeaponFlameMakeWeapon(fighter_gobj, pos, &vel);

    ip->multi -= ammo_sub;
}

#endif /* NDS_P2_ITEM_CORE */
