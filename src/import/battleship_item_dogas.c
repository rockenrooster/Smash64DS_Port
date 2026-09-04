/* P2 Dogas / Koffing (kind nITKindDogas). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itmonster/itdogas.c:1-315.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0xBF8 (reloc_data.us.h:3780), the smog weapon row is 0xC40 (:3781), and
 * the monster anim node base is 0x12820 (:3826) via AnimJoint 0x128DC
 * (:3827); the port's generated reloc header does not publish Dogas tokens,
 * so this TU owns its uintptr_t tokens the same way battleship_item_gbumper.c
 * owns GBumper's (local tokens, no generator involvement, no hand-edited
 * generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITDOGAS_ and ITMONSTER_ tuning
 * is present in include/it/item.h; itGetMonsterAnimNode, the SFX/voice IDs,
 * and the smog weapon-vars slot are not) are referenced verbatim and listed
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

/* decomp/BattleShip-main/include/reloc_data.us.h:3780. */
uintptr_t llITCommonDataDogasItemAttributes = 0xBF8u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3781. */
uintptr_t llITCommonDataDogasSmogWeaponAttributes = 0xC40u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3826. */
uintptr_t llITCommonDataDogasDataStart = 0x12820u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3827. */
uintptr_t llITCommonDataDogasAnimJoint = 0x128DCu;

extern void *gITManagerCommonData;

/* decomp sys/objanim.h:16 and :52. No port header in this TU's chain
 * publishes them; battleship_item_bombhei.c:53-55 carries the same kind of
 * local externs. */
extern void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                               f32 anim_frame);
extern void gcPlayAnimAll(GObj *gobj);

/* decomp sys/utils.h:19-:20. Same seam as
 * battleship_item_bombhei.c:60-61. */
extern f32 syUtilsRandFloat(void);
extern s32 syUtilsRandIntRange(s32 range);

/* Same shape as battleship_link_bomb.c:80. */
extern void func_800269C0_275C0(u16 sfx_id);

/* decomp itdogas.h:8-18 verbatim. The port publishes no per-kind item procs,
 * so the source header's declarations travel with this TU, exactly as the
 * Tomato and Star files carry theirs. */
extern sb32 itDogasDisappearProcUpdate(GObj *item_gobj);
extern void itDogasDisappearSetStatus(GObj *item_gobj);
extern void itDogasAttackUpdateSmog(GObj *item_gobj);
extern sb32 itDogasAttackProcUpdate(GObj *item_gobj);
extern void itDogasAttackInitVars(GObj *item_gobj);
extern void itDogasAttackSetStatus(GObj *item_gobj);
extern sb32 itDogasCommonProcUpdate(GObj *item_gobj);
extern sb32 itDogasCommonProcMap(GObj *item_gobj);
extern GObj* itDogasMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
extern sb32 itDogasWeaponSmogProcUpdate(GObj *weapon_gobj);
extern GObj* itDogasWeaponSmogMakeWeapon(GObj *item_gobj, Vec3f *pos, Vec3f *vel);

// 0x8018B2C0
// decomp itdogas.c:12-34 verbatim, adapted only for the port's ITDesc shape
// (o_attributes is const void * here, lbRelocGetFileData takes the token the
// same way).
ITDesc dITDogasItemDesc =
{
    nITKindDogas,                           // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataDogasItemAttributes,     // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindNull,                   // Main matrix transformations
        nGCMatrixKindNull,                   // Secondary matrix transformations?
        0,                                   // ???
    },

    nGMAttackStateOff,                      // Hitbox Update State
    itDogasCommonProcUpdate,                // Proc Update
    itDogasCommonProcMap,                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x80182BF4
// decomp itdogas.c:37-62 verbatim.
ITStatusDesc dITDogasStatusDescs[/* */] =
{
    // Status 0 (Neutral Active)
    {
        itDogasAttackProcUpdate,            // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 1 (Neutral Disappear)
    {
        itDogasDisappearProcUpdate,         // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// 0x8018B334
// decomp itdogas.c:65-87 verbatim.
WPDesc dITDogasWeaponSmogWeaponDesc =
{
    0x03,                                   // Render flags?
    nWPKindDogasSmog,                       // Weapon Kind
    &gITManagerCommonData,                    // Pointer to weapon's loaded files?
    &llITCommonDataDogasSmogWeaponAttributes,    // Offset of weapon attributes in loaded files

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,             // Main matrix transformations
        nGCMatrixKindNull,                   // Secondary matrix transformations?
        0,                                  // ???
    },

    itDogasWeaponSmogProcUpdate,            // Proc Update
    NULL,                                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Absorb
};

// decomp itdogas.c:95-100 verbatim.
enum itDogasStatus
{
    itDogasStatusAttack,
    itDogasStatusDisappear,
    itDogasStatusEnumCount
};

// 0x80182C80
// decomp itdogas.c:109-120 verbatim.
sb32 itDogasDisappearProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        return TRUE;
    }
    ip->multi--;

    return FALSE;
}

// 0x80182CA8
// decomp itdogas.c:123-130 verbatim.
void itDogasDisappearSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = ITDOGAS_DESPAWN_WAIT;

    itMainSetStatus(item_gobj, dITDogasStatusDescs, itDogasStatusDisappear);
}

// 0x80182CDC
// decomp itdogas.c:133-166 verbatim.
void itDogasAttackUpdateSmog(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    Vec3f pos;
    Vec3f vel;

    if (ip->item_vars.dogas.smog_spawn_wait <= 0)
    {
        vel.x = ITDOGAS_SMOG_VEL;
        vel.y = ITDOGAS_SMOG_VEL;
        vel.z = 0.0F;

        pos = dobj->translate.vec.f;

        pos.x += (syUtilsRandFloat() * ITDOGAS_SMOG_MUL_OFF_X) - ITDOGAS_SMOG_SUB_OFF_X;
        pos.y += (syUtilsRandFloat() * ITDOGAS_SMOG_MUL_OFF_Y) - ITDGOAS_SMOG_SUB_OFF_Y;

        if (pos.x < dobj->translate.vec.f.x)
        {
            vel.x = -vel.x;
        }
        if (pos.y < dobj->translate.vec.f.y)
        {
            vel.y = -vel.y;
        }
        itDogasWeaponSmogMakeWeapon(item_gobj, &pos, &vel);
        func_800269C0_275C0(nSYAudioFGMDogasSmog);

        ip->item_vars.dogas.smog_spawn_wait = ITDOGAS_SMOG_SPAWN_WAIT;

        ip->multi--;
    }
}

// 0x80182E1C
// decomp itdogas.c:169-184 verbatim.
sb32 itDogasAttackProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itDogasAttackUpdateSmog(item_gobj);

    if (ip->multi == 0)
    {
        itDogasDisappearSetStatus(item_gobj);

        return FALSE;
    }
    ip->item_vars.dogas.smog_spawn_wait--;

    return FALSE;
}

// 0x80182E78
// decomp itdogas.c:187-205 verbatim.
void itDogasAttackInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    ip->multi = ITDOGAS_SMOG_SPAWN_COUNT;

    ip->item_vars.dogas.smog_spawn_wait = 0;

    if (ip->kind == nITKindDogas)
    {
        ip->item_vars.dogas.pos = dobj->translate.vec.f;

        gcAddDObjAnimJoint(dobj->child, itGetPData(ip, &llITCommonDataDogasDataStart, &llITCommonDataDogasAnimJoint), 0.0F);

        gcPlayAnimAll(item_gobj);
        func_800269C0_275C0(nSYAudioVoiceMBallDogasAppear);
    }
}

// 0x80182F0C
// decomp itdogas.c:208-212 verbatim.
void itDogasAttackSetStatus(GObj *item_gobj)
{
    itDogasAttackInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITDogasStatusDescs, itDogasStatusAttack);
}

// 0x80182F40
// decomp itdogas.c:215-228 verbatim.
sb32 itDogasCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        ip->physics.vel_air.x = ip->physics.vel_air.y = 0.0F;

        itDogasAttackSetStatus(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x80182F94
// decomp itdogas.c:231-240 verbatim.
sb32 itDogasCommonProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR) != FALSE)
    {
        ip->physics.vel_air.y = 0.0F;
    }
    return FALSE;
}

// 0x80182FD4
// decomp itdogas.c:243-271 verbatim.
GObj* itDogasMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITDogasItemDesc, pos, vel, flags);
    DObj *dobj;
    ITStruct *ip;

    if (item_gobj != NULL)
    {
        dobj = DObjGetStruct(item_gobj);

        gcAddXObjForDObjFixed(dobj, 0x28, 0);
        gcAddXObjForDObjFixed(dobj->child, nGCMatrixKindTraRotRpyRSca, 0);

        dobj->translate.vec.f = *pos;

        ip = itGetStruct(item_gobj);

        dobj->translate.vec.f.y -= ip->attr->map_coll_bottom;

        ip->multi = ITMONSTER_RISE_STOP_WAIT;

        ip->physics.vel_air.x = 0.0F;
        ip->physics.vel_air.z = 0.0F;
        ip->physics.vel_air.y = ITMONSTER_RISE_VEL_Y;

        gcAddDObjAnimJoint(dobj->child, itGetMonsterAnimNode(ip, &llITCommonDataDogasDataStart), 0.0F);
    }
    return item_gobj;
}

// 0x801830DC
// decomp itdogas.c:274-286 verbatim.
sb32 itDogasWeaponSmogProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    DObj *dobj = DObjGetStruct(weapon_gobj)->child;

    wp->attack_coll.size = dobj->scale.vec.f.x * wp->weapon_vars.smog.attr->size;

    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x80183144
// decomp itdogas.c:289-315 verbatim.
GObj* itDogasWeaponSmogMakeWeapon(GObj *item_gobj, Vec3f *pos, Vec3f *vel)
{
    WPDesc *weapon_desc = &dITDogasWeaponSmogWeaponDesc;
    GObj *weapon_gobj = wpManagerMakeWeapon(item_gobj, &dITDogasWeaponSmogWeaponDesc, pos, WEAPON_FLAG_PARENT_ITEM);
    DObj *dobj;
    WPStruct *wp;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);

    wp->lifetime = ITDOGAS_SMOG_LIFETIME;

    wp->weapon_vars.smog.attr = (WPAttributes*) ((uintptr_t)*weapon_desc->p_weapon + (intptr_t)weapon_desc->o_attributes); // Dude I had a stroke trying to match this

    dobj = DObjGetStruct(weapon_gobj);

    wp->physics.vel_air = *vel;

    gcAddXObjForDObjFixed(dobj->child, 0x2C, 0);

    dobj->translate.vec.f = *pos;

    return weapon_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
