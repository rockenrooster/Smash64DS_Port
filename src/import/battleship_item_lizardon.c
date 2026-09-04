/* P2 Lizardon / Charizard (kind nITKindLizardon). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itmonster/itlizardon.c:1-436.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x8FC), the flame weapon-attribute row (0x944), the data-start base
 * (0xD5C0), the anim joint (0xD658) and the matanim joint (0xD688) live
 * below (decomp/BattleShip-main/include/reloc_data.us.h:3768, :3769,
 * :3816-:3818); the port's generated reloc header does not publish Lizardon
 * tokens, so this TU owns its uintptr_t tokens the same way
 * battleship_item_gbumper.c owns GBumper's (local tokens, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * itGetMonsterAnimNode, itGetPData, the monster SFX and voice IDs, the anim
 * helpers, lbParticleMakePosVel, and func_800269C0_275C0 ride on it/item.h,
 * gm/gmsound.h, ef/effect.h, nds/nds_obj_anim.h, and sys/audio.h, so no
 * local externs are written for them. The syUtils, wpMap, effect and
 * particle-bank entry points below are referenced verbatim and listed in
 * the task report -- no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <wp/weapon.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <ef/effect.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/audio.h>
#include <nds/nds_obj_anim.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3768. */
uintptr_t llITCommonDataLizardonItemAttributes = 0x8FCu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3769. */
uintptr_t llITCommonDataLizardonFlameWeaponAttributes = 0x944u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3816. */
uintptr_t llITCommonDataLizardonDataStart = 0xD5C0u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3817. */
uintptr_t llITCommonDataLizardonAnimJoint = 0xD658u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3818. */
uintptr_t llITCommonDataLizardonMatAnimJoint = 0xD688u;

extern void *gITManagerCommonData;

/* decomp sys/utils.h:20. Same seam as
 * battleship_item_bombhei.c:60-61. */
extern s32 syUtilsRandIntRange(s32 range);
extern f32 syUtilsArcTan2(f32 y, f32 x);

/* decomp libultra trig. Same seam as battleship_captain.c:70-71. */
extern f32 __cosf(f32 x);
extern f32 __sinf(f32 x);

/* decomp wp/wpmap.h. Same seam as battleship_item_lgun.c:74. */
extern sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);

/* decomp ef/efmanager.h:39, :43 and :62. Same shapes as
 * battleship_item_fflower.c:83-84. */
extern LBParticle *efManagerDustHeavyMakeEffect(Vec3f *pos, s32 lr);
extern LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
extern LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos);

/* decomp it/itmanager.c:109. Same seam as
 * battleship_item_fflower.c:85. */
extern s32 gITManagerParticleBankID;

/* decomp itlizardon.h:8-26 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly
 * as the Tomato and Star files carry theirs. */
extern sb32 itLizardonFallUnusedProcUpdate(GObj *item_gobj);
extern sb32 itLizardonFallUnusedProcMap(GObj *item_gobj);
extern sb32 itLizardonFallUnusedSetStatus(GObj *item_gobj);
extern sb32 itLizardonFallProcUpdate(GObj *item_gobj);
extern sb32 itLizardonFallProcMap(GObj *item_gobj);
extern void itLizardonFallSetStatus(GObj *item_gobj);
extern sb32 itLizardonAttackProcUpdate(GObj *item_gobj);
extern sb32 itLizardonAttackProcMap(GObj *item_gobj);
extern void itLizardonAttackInitVars(GObj *item_gobj);
extern void itLizardonAttackSetStatus(GObj *item_gobj);
extern sb32 itLizardonCommonProcUpdate(GObj *item_gobj);
extern sb32 itLizardonCommonProcMap(GObj *item_gobj);
extern GObj* itLizardonMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
extern sb32 itLizardonWeaponFlameProcUpdate(GObj *weapon_gobj);
extern sb32 itLizardonWeaponFlameProcMap(GObj *weapon_gobj);
extern sb32 itLizardonWeaponFlameProcHit(GObj *weapon_gobj);
extern sb32 itLizardonWeaponFlameProcReflector(GObj *weapon_gobj);
extern GObj* itLizardonWeaponFlameMakeWeapon(GObj *item_gobj, Vec3f *pos, Vec3f *vel);
extern void itLizardonAttackMakeFlame(GObj *item_gobj, Vec3f *pos, s32 lr);

// 0x8018AD30
// decomp itlizardon.c:13-35 verbatim, adapted only for the port's ITDesc
// shape (o_attributes is const void * here, lbRelocGetFileData takes the
// token the same way).
ITDesc dITLizardonItemDesc =
{
    nITKindLizardon,                        // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataLizardonItemAttributes,  // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindNull,                  // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    nGMAttackStateNew,                      // Hitbox Update State
    itLizardonCommonProcUpdate,             // Proc Update
    itLizardonCommonProcMap,                // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x8018AD64
// decomp itlizardon.c:38-75 verbatim.
ITStatusDesc dITLizardonStatusDescs[/* */] =
{
    // Status 0 (Unused Fall)
    {
        itLizardonFallUnusedProcUpdate,     // Proc Update
        itLizardonFallUnusedProcMap,        // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 1 (Air Fall)
    {
        itLizardonFallProcUpdate,           // Proc Update
        itLizardonFallProcMap,              // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 2 (Neutral Attack)
    {
        itLizardonAttackProcUpdate,         // Proc Update
        itLizardonAttackProcMap,            // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// 0x8018ADC4
// decomp itlizardon.c:78-100 verbatim.
WPDesc dITLizardonWeaponFlameWeaponDesc =
{
    0x00,                                   // Render flags?
    nWPKindLizardonFlame,                   // Weapon Kind
    &gITManagerCommonData,                  // Pointer to character's loaded files?
    &llITCommonDataLizardonFlameWeaponAttributes,// Offset of weapon attributes in loaded files

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyRSca,         // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    itLizardonWeaponFlameProcUpdate,        // Proc Update
    itLizardonWeaponFlameProcMap,           // Proc Map
    itLizardonWeaponFlameProcHit,           // Proc Hit
    itLizardonWeaponFlameProcHit,           // Proc Shield
    NULL,                                   // Proc Hop
    itLizardonWeaponFlameProcHit,           // Proc Set-Off
    itLizardonWeaponFlameProcReflector,     // Proc Reflector
    NULL                                    // Proc Absorb
};

// decomp itlizardon.c:108-114 verbatim.
enum itLizardonStatus
{
    nITLizardonStatusFallUnused,            // Unused
    nITLizardonStatusFall,
    nITLizardonStatusAttack,
    nITLizardonStatusEnumCount
};

// 0x8017F470
// decomp itlizardon.c:123-130 verbatim.
sb32 itLizardonFallUnusedProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITLIZARDON_GRAVITY, ITLIZARDON_TVEL);

    return FALSE;
}

// 0x8017F49C
// decomp itlizardon.c:133-144 verbatim.
sb32 itLizardonFallUnusedProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMapCheckLanding(item_gobj, ITLIZARDON_MAP_REBOUND_COMMON, ITLIZARDON_MAP_REBOUND_GROUND, itLizardonAttackSetStatus);

    if (ip->coll_data.mask_curr & MAP_FLAG_FLOOR)
    {
        ip->physics.vel_air.y = 0.0F;
    }
    return FALSE;
}

// 0x8017F49C
// decomp itlizardon.c:147-155 verbatim.
sb32 itLizardonFallUnusedSetStatus(GObj *item_gobj) // Unused
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITLizardonStatusDescs, nITLizardonStatusFallUnused);
}

// 0x8017F53C
// decomp itlizardon.c:158-165 verbatim.
sb32 itLizardonFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITLIZARDON_GRAVITY, ITLIZARDON_TVEL);

    return FALSE;
}

// 0x8017F568
// decomp itlizardon.c:168-182 verbatim.
sb32 itLizardonFallProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMapTestAllCheckCollEnd(item_gobj);

    if (ip->coll_data.mask_curr & MAP_FLAG_FLOOR)
    {
        ip->physics.vel_air.y = 0.0F;

        itLizardonAttackSetStatus(item_gobj);
        itLizardonAttackInitVars(item_gobj);
    }
    return FALSE;
}

// 0x8017F5C4
// decomp itlizardon.c:185-188 verbatim.
void itLizardonFallSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITLizardonStatusDescs, nITLizardonStatusFall);
}

// 0x8017F5EC
// decomp itlizardon.c:191-241 verbatim.
sb32 itLizardonAttackProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    Vec3f pos = dobj->translate.vec.f;

    if (ip->kind == nITKindLizardon)
    {
        pos.y += ITLIZARDON_LIZARDON_FLAME_OFF_Y;

        pos.x += (ITLIZARDON_LIZARDON_FLAME_OFF_X * ip->lr);
    }
    else pos.x += (ITLIZARDON_OTHER_FLAME_OFF_X * ip->lr);

    if (ip->item_vars.lizardon.flame_spawn_wait == 0)
    {
        itLizardonAttackMakeFlame(item_gobj, &pos, ip->lr);

        ip->item_vars.lizardon.flame_spawn_wait = ITLIZARDON_FLAME_SPAWN_WAIT;
    }
    ip->item_vars.lizardon.flame_spawn_wait--;

    if (ip->multi == 0)
    {
        return TRUE;
    }
    if (ip->item_vars.lizardon.turn_wait == 0)
    {
        ip->item_vars.lizardon.turn_wait = ITLIZARDON_TURN_WAIT;

        ip->lr = -ip->lr;

        pos = dobj->translate.vec.f;

        pos.y += ip->attr->map_coll_bottom;

        pos.x += (ip->attr->map_coll_width + ITLIZARDON_DUST_OFF_X) * -ip->lr;

        efManagerDustHeavyMakeEffect(&pos, -ip->lr);

        if (ip->kind == nITKindPippi)
        {
            dobj->rotate.vec.f.y += F_CST_DTOR32(180.0F);
        }
    }
    ip->item_vars.lizardon.turn_wait--;

    ip->multi--;

    return FALSE;
}

// 0x8017F7E8
// decomp itlizardon.c:244-249 verbatim.
sb32 itLizardonAttackProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itLizardonFallSetStatus);

    return FALSE;
}

// 0x8017F810
// decomp itlizardon.c:252-277 verbatim.
void itLizardonAttackInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    s32 unused[2];
    void *addr;
    Vec3f pos;

    ip->item_vars.lizardon.turn_wait = ITLIZARDON_TURN_WAIT;

    pos = dobj->translate.vec.f;

    ip->item_vars.lizardon.pos = pos;
    ip->item_vars.lizardon.flame_spawn_wait = 0;

    ip->lr = -1;

    if (ip->kind == nITKindLizardon)
    {
        addr = (void*) ((uintptr_t)ip->attr->data - (intptr_t)&llITCommonDataLizardonDataStart);

        gcAddDObjAnimJoint(dobj, lbRelocGetFileData(AObjEvent32*, addr, &llITCommonDataLizardonAnimJoint), 0.0F);
        gcAddMObjMatAnimJoint(dobj->mobj, lbRelocGetFileData(AObjEvent32*, addr, &llITCommonDataLizardonMatAnimJoint), 0.0F);
        gcPlayAnimAll(item_gobj);
    }
}

// 0x8017F8E4
// decomp itlizardon.c:280-283 verbatim.
void itLizardonAttackSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITLizardonStatusDescs, nITLizardonStatusAttack);
}

// 0x8017F90C
// decomp itlizardon.c:286-305 verbatim.
sb32 itLizardonCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        ip->multi = ITLIZARDON_LIFETIME;

        ip->physics.vel_air.y = 0.0F;

        if (ip->kind == nITKindLizardon)
        {
            func_800269C0_275C0(nSYAudioVoiceMBallLizardonAppear);
        }
        itLizardonFallSetStatus(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x8017F98C
// decomp itlizardon.c:308-317 verbatim.
sb32 itLizardonCommonProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR) != FALSE)
    {
        ip->physics.vel_air.y = 0.0F;
    }
    return FALSE;
}

// 0x8017F9CC
// decomp itlizardon.c:320-347 verbatim.
GObj* itLizardonMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITLizardonItemDesc, pos, vel, flags);
    DObj *dobj;
    ITStruct *ip;

    if (item_gobj != NULL)
    {
        dobj = DObjGetStruct(item_gobj);

        gcAddXObjForDObjFixed(dobj, nGCMatrixKindTraRotRpyR, 0);
        gcAddXObjForDObjFixed(dobj, 0x48, 0);

        dobj->translate.vec.f = *pos;

        ip = itGetStruct(item_gobj);

        ip->multi = ITMONSTER_RISE_STOP_WAIT;

        ip->physics.vel_air.x = ip->physics.vel_air.z = 0.0F;
        ip->physics.vel_air.y = ITMONSTER_RISE_VEL_Y;

        dobj->translate.vec.f.y -= ip->attr->map_coll_bottom;

        gcAddDObjAnimJoint(dobj, itGetMonsterAnimNode(ip, &llITCommonDataLizardonDataStart), 0.0F);
    }
    return item_gobj;
}

// 0x8017FACC
// decomp itlizardon.c:350-359 verbatim.
sb32 itLizardonWeaponFlameProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x8017FAF8
// decomp itlizardon.c:362-371 verbatim.
sb32 itLizardonWeaponFlameProcMap(GObj *weapon_gobj)
{
    if (wpMapTestAllCheckCollEnd(weapon_gobj) != FALSE)
    {
        efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

        return TRUE;
    }
    else return FALSE;
}

// 0x8017FB3C
// decomp itlizardon.c:374-380 verbatim.
sb32 itLizardonWeaponFlameProcHit(GObj *weapon_gobj)
{
    func_800269C0_275C0(nSYAudioFGMExplodeS);
    efManagerSparkleWhiteMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f);

    return FALSE;
}

// 0x8017FB74
// decomp itlizardon.c:383-399 verbatim.
sb32 itLizardonWeaponFlameProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);
    Vec3f *translate;

    wp->lifetime = ITLIZARDON_FLAME_LIFETIME;

    wpMainReflectorSetLR(wp, fp);

    translate = &DObjGetStruct(weapon_gobj)->translate.vec.f;

    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 2, translate->x, translate->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);
    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0, translate->x, translate->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);

    return FALSE;
}

// 0x8017FC38
// decomp itlizardon.c:402-421 verbatim.
GObj* itLizardonWeaponFlameMakeWeapon(GObj *item_gobj, Vec3f *pos, Vec3f *vel)
{
    GObj *weapon_gobj = wpManagerMakeWeapon(item_gobj, &dITLizardonWeaponFlameWeaponDesc, pos, WEAPON_FLAG_PARENT_ITEM);
    WPStruct *ip;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    ip = wpGetStruct(weapon_gobj);

    ip->physics.vel_air = *vel;

    ip->lifetime = ITLIZARDON_FLAME_LIFETIME;

    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 2, pos->x, pos->y, 0.0F, ip->physics.vel_air.x, ip->physics.vel_air.y, 0.0F); // This needs to return something in v0 to match
    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0, pos->x, pos->y, 0.0F, ip->physics.vel_air.x, ip->physics.vel_air.y, 0.0F);

    return weapon_gobj;
}

// 0x8017FD2C
// decomp itlizardon.c:424-436 verbatim.
void itLizardonAttackMakeFlame(GObj *item_gobj, Vec3f *pos, s32 lr)
{
    ITStruct *ip = itGetStruct(item_gobj);
    Vec3f vel;

    vel.x = __cosf(ITLIZARDON_FLAME_ANGLE) * ITLIZARDON_FLAME_VEL * lr;
    vel.y = __sinf(ITLIZARDON_FLAME_ANGLE) * ITLIZARDON_FLAME_VEL;
    vel.z = 0.0F;

    itLizardonWeaponFlameMakeWeapon(item_gobj, pos, &vel);

    func_800269C0_275C0(nSYAudioFGMLizardonFlame);
}

#endif /* NDS_P2_ITEM_CORE */
