/* P2 Spear / Beedrill (kind nITKindSpear). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itmonster/itspear.c:1-428.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x98C), the Spear swarm weapon-attribute row (0x9D4), the Pippi swarm
 * weapon-attribute row (0xCBC), the data-start base (0xDF38), the anim
 * joint (0xDFFC) and the matanim joint (0xE12C) live below
 * (decomp/BattleShip-main/include/reloc_data.us.h:3770, :3771, :3783,
 * :3819-:3821); the port's generated reloc header does not publish Spear
 * tokens, so this TU owns its uintptr_t tokens the same way
 * battleship_item_gbumper.c owns GBumper's (local tokens, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * itGetMonsterAnimNode, itGetPData, the monster SFX and voice IDs, the anim
 * helpers, the display helpers, and func_800269C0_275C0 ride on it/item.h,
 * gm/gmsound.h, nds/nds_obj_anim.h, and sys/audio.h, so no local externs
 * are written for them. The syUtils and wpDisplayMain entry points below
 * are referenced verbatim and listed in the task report -- no values
 * invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <wp/weapon.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <gr/ground.h>
#include <sys/develop.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/objman.h>
#include <sys/audio.h>
#include <nds/nds_obj_anim.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3770. */
uintptr_t llITCommonDataSpearItemAttributes = 0x98Cu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3771. */
uintptr_t llITCommonDataSpearSwarmWeaponAttributes = 0x9D4u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3783. */
uintptr_t llITCommonDataPippiSwarmWeaponAttributes = 0xCBCu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3819. */
uintptr_t llITCommonDataSpearDataStart = 0xDF38u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3820. */
uintptr_t llITCommonDataSpearAnimJoint = 0xDFFCu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3821. */
uintptr_t llITCommonDataSpearMatAnimJoint = 0xE12Cu;

extern void *gITManagerCommonData;

/* decomp sys/utils.h:19-:20. Same seam as
 * battleship_item_bombhei.c:60-61. */
extern f32 syUtilsRandFloat(void);
extern s32 syUtilsRandIntRange(s32 range);

/* decomp wp/wpdisplay.h. Same seam as
 * battleship_fox_blaster.c:38. */
extern void wpDisplayMain(GObj *weapon_gobj, void (*proc_display)(GObj *));

/* decomp itspear.h:8-22 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly
 * as the Tomato and Star files carry theirs. */
extern void itSpearFlyCallSwarmMember(GObj *item_gobj);
extern sb32 itSpearAppearProcUpdate(GObj *item_gobj);
extern void itSpearAppearInitVars(GObj *item_gobj);
extern void itSpearAppearSetStatus(GObj *item_gobj);
extern sb32 itSpearFlyProcUpdate(GObj *item_gobj);
extern void itSpearFlyInitVars(GObj *item_gobj);
extern void itSpearFlySetStatus(GObj *item_gobj);
extern sb32 itSpearCommonProcUpdate(GObj *item_gobj);
extern sb32 itSpearCommonProcMap(GObj *item_gobj);
extern GObj* itSpearMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
extern sb32 itSpearWeaponSwarmProcUpdate(GObj *weapon_gobj);
extern void itPippiWeaponSwarmRenderSwarm(GObj *item_gobj);
extern void itPippiWeaponSwarmProcDisplay(GObj *item_gobj);
extern GObj* itSpearWeaponSwarmMakeWeapon(GObj *item_gobj, Vec3f *pos, s32 kind);
extern void itSpearFlyMakeSwarm(GObj *item_gobj, Vec3f *pos, s32 kind);

// 0x8018AE00
// decomp itspear.c:13-35 verbatim, adapted only for the port's ITDesc shape
// (o_attributes is const void * here, lbRelocGetFileData takes the token the
// same way).
ITDesc dITSpearItemDesc =
{
    nITKindSpear,                           // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataSpearItemAttributes,     // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    nGMAttackStateNew,                      // Hitbox Update State
    itSpearCommonProcUpdate,                // Proc Update
    itSpearCommonProcMap,                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x8018AE34
// decomp itspear.c:38-63 verbatim.
ITStatusDesc dITSpearStatusDescs[/* */] =
{
    // Status 0 (Neutral Appear)
    {
        itSpearAppearProcUpdate,            // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 1 (Neutral Fly)
    {
        itSpearFlyProcUpdate,               // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// 0x8018AE74
// decomp itspear.c:66-88 verbatim.
WPDesc dITSpearWeaponSwarmWeaponDesc =
{
    0x01,                                     // Render flags?
    nWPKindSpearSwarm,                        // Weapon Kind
    &gITManagerCommonData,                    // Pointer to character's loaded files?
    &llITCommonDataSpearSwarmWeaponAttributes,// Offset of weapon attributes in loaded files

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    itSpearWeaponSwarmProcUpdate,           // Proc Update
    NULL,                                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Absorb
};

// 0x8018AEA8
// decomp itspear.c:91-113 verbatim.
WPDesc dITPippiWeaponSwarmWeaponDesc =
{
    0x01,                                     // Render flags?
    nWPKindSpearSwarm,                        // Weapon Kind
    &gITManagerCommonData,                    // Pointer to character's loaded files?
    &llITCommonDataPippiSwarmWeaponAttributes,// Offset of weapon attributes in loaded files

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    itSpearWeaponSwarmProcUpdate,           // Proc Update
    NULL,                                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Absorb
};

// decomp itspear.c:121-126 verbatim.
enum itSpearStatus
{
    nITSpearStatusAppear,
    nITSpearStatusFly,
    nITSpearStatusEnumCount
};

// 0x8017FDC0
// decomp itspear.c:135-154 verbatim.
void itSpearFlyCallSwarmMember(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (ip->item_vars.spear.spear_spawn_wait <= 0)
    {
        Vec3f pos = dobj->translate.vec.f;
        s32 unused;

        pos.y = ip->item_vars.spear.spear_spawn_pos_y;

        pos.y += (ITSPEAR_SPAWN_OFF_Y_MUL * syUtilsRandFloat()) + ITSPEAR_SPAWN_OFF_Y_ADD;

        itSpearFlyMakeSwarm(item_gobj, &pos, ip->kind);

        ip->item_vars.spear.spear_spawn_count--;
        ip->item_vars.spear.spear_spawn_wait = syUtilsRandIntRange(ITSPEAR_SPAWN_WAIT_RANDOM) + ITSPEAR_SPAWN_WAIT_CONST;
    }
}

// 0x8017FE70
// decomp itspear.c:157-168 verbatim.
sb32 itSpearAppearProcUpdate(GObj *item_gobj)
{
    DObj *dobj = DObjGetStruct(item_gobj);

    if (item_gobj->anim_frame == ITSPEAR_SWARM_CALL_WAIT)
    {
        dobj->child->anim_joint.event32 = NULL;

        itSpearFlySetStatus(item_gobj);
    }
    return FALSE;
}

// 0x8017FEB8
// decomp itspear.c:171-195 verbatim.
void itSpearAppearInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    ip->multi = 0;

    ip->physics.vel_air.y = 0;

    if (ip->kind == nITKindSpear)
    {
        void *anim_joint;
        void *matanim_joint;

        anim_joint = itGetPData(ip, &llITCommonDataSpearDataStart, &llITCommonDataSpearAnimJoint);

        gcAddDObjAnimJoint(dobj->child, anim_joint, 0.0F);

        matanim_joint = itGetPData(ip, &llITCommonDataSpearDataStart, &llITCommonDataSpearMatAnimJoint);

        gcAddMObjMatAnimJoint(dobj->child->mobj, matanim_joint, 0.0F);
        gcPlayAnimAll(item_gobj);
        func_800269C0_275C0(nSYAudioVoiceMBallSpearAppear);
    }
}

// 0x8017FF74
// decomp itspear.c:198-202 verbatim.
void itSpearAppearSetStatus(GObj *item_gobj)
{
    itSpearAppearInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITSpearStatusDescs, nITSpearStatusAppear);
}

// 0x8017FFA8
// decomp itspear.c:205-247 verbatim.
sb32 itSpearFlyProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITSPEAR_GRAVITY, ITSPEAR_TVEL);

    ip->physics.vel_air.x += ITSPEAR_SWARM_CALL_VEL_X * ip->lr;

    if (ip->lr == +1)
    {
        if (dobj->translate.vec.f.x >= (gMPCollisionGroundData->map_bound_right - ITSPEAR_SWARM_CALL_OFF_X))
        {
            ip->physics.vel_air.x = 0.0F;
            ip->physics.vel_air.y = 0.0F;

            if (ip->item_vars.spear.spear_spawn_count != 0)
            {
                itSpearFlyCallSwarmMember(item_gobj);
            }
            else return TRUE;

            ip->item_vars.spear.spear_spawn_wait--;
        }
    }
    if (ip->lr == -1)
    {
        if (dobj->translate.vec.f.x <= (gMPCollisionGroundData->map_bound_left + ITSPEAR_SWARM_CALL_OFF_X))
        {
            ip->physics.vel_air.x = 0.0F;
            ip->physics.vel_air.y = 0.0F;

            if (ip->item_vars.spear.spear_spawn_count != 0)
            {
                itSpearFlyCallSwarmMember(item_gobj);
            }
            else return TRUE;

            ip->item_vars.spear.spear_spawn_wait--;
        }
    }
    return FALSE;
}

// 0x8018010C
// decomp itspear.c:250-264 verbatim.
void itSpearFlyInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->physics.vel_air.y = ITSPEAR_SWARM_CALL_VEL_Y;

    ip->item_vars.spear.spear_spawn_pos_y = DObjGetStruct(item_gobj)->translate.vec.f.y;
    ip->item_vars.spear.spear_spawn_wait = 0;
    ip->item_vars.spear.spear_spawn_count = ITSPEAR_SPAWN_COUNT;

    if (ip->kind == nITKindSpear)
    {
        func_800269C0_275C0(nSYAudioVoiceMBallSpearSwarm);
    }
}

// 0x80180160
// decomp itspear.c:267-271 verbatim.
void itSpearFlySetStatus(GObj *item_gobj)
{
    itSpearFlyInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITSpearStatusDescs, nITSpearStatusFly);
}

// 0x80180194
// decomp itspear.c:274-285 verbatim.
sb32 itSpearCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        itSpearAppearSetStatus(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x801801D8
// decomp itspear.c:288-297 verbatim.
sb32 itSpearCommonProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR) != FALSE)
    {
        ip->physics.vel_air.y = 0.0F;
    }
    return FALSE;
}

// 0x80180218
// decomp itspear.c:300-339 verbatim.
GObj* itSpearMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITSpearItemDesc, pos, vel, flags);
    DObj *dobj;
    ITStruct *ip;

    if (item_gobj != NULL)
    {
        ip = itGetStruct(item_gobj);

        itMainClearOwnerStats(item_gobj);

        dobj = DObjGetStruct(item_gobj);

        gcAddXObjForDObjFixed(dobj->child, 0x48, 0);

        dobj->translate.vec.f = *pos;

        if (syUtilsRandIntRange(2) == 0)
        {
            dobj->child->rotate.vec.f.y = F_CST_DTOR32(180.0F);

            ip->lr = -1;

        }
        else ip->lr = +1;

        ip->multi = ITMONSTER_RISE_STOP_WAIT;

        ip->attack_coll.interact_mask = GMHITCOLLISION_FLAG_FIGHTER;

        ip->physics.vel_air.x = ip->physics.vel_air.z = 0.0F;
        ip->physics.vel_air.y = ITMONSTER_RISE_VEL_Y;

        dobj->translate.vec.f.y -= ip->attr->map_coll_bottom;

        gcAddDObjAnimJoint(dobj->child, itGetMonsterAnimNode(ip, &llITCommonDataSpearDataStart), 0.0F);
    }
    return item_gobj;
}

// 0x80180354
// decomp itspear.c:342-356 verbatim.
sb32 itSpearWeaponSwarmProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    DObj *dobj = DObjGetStruct(weapon_gobj);

    if ((wp->lr == +1) && (dobj->translate.vec.f.x >= (gMPCollisionGroundData->map_bound_right - ITSPEAR_SWARM_CALL_OFF_X)))
    {
        return TRUE;
    }
    else if ((wp->lr == -1) && (dobj->translate.vec.f.x <= (gMPCollisionGroundData->map_bound_left + ITSPEAR_SWARM_CALL_OFF_X)))
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x80180400
// decomp itspear.c:359-368 verbatim.
void itPippiWeaponSwarmRenderSwarm(GObj *item_gobj)
{
    gDPPipeSync(gSYTaskmanDLHeads[0]++);

    gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);

    gcDrawDObjTreeForGObj(item_gobj);

    gDPPipeSync(gSYTaskmanDLHeads[0]++);
}

// 0x80180480
// decomp itspear.c:371-374 verbatim.
void itPippiWeaponSwarmProcDisplay(GObj *item_gobj)
{
    wpDisplayMain(item_gobj, itPippiWeaponSwarmRenderSwarm);
}

// 0x801804A4
// decomp itspear.c:377-422 verbatim.
GObj* itSpearWeaponSwarmMakeWeapon(GObj *item_gobj, Vec3f *pos, s32 kind)
{
    ITStruct *ip = itGetStruct(item_gobj);
    GObj *weapon_gobj = wpManagerMakeWeapon(item_gobj, ((kind == nITKindSpear) ? &dITSpearWeaponSwarmWeaponDesc : &dITPippiWeaponSwarmWeaponDesc), pos, WEAPON_FLAG_PARENT_ITEM);
    DObj *dobj;
    s32 unused;
    WPStruct *wp;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);

    wp->lr = -ip->lr;

    wp->physics.vel_air.x = wp->lr * ITSPEAR_SWARM_FLY_VEL_X;

    dobj = DObjGetStruct(weapon_gobj);

    if (kind == nITKindSpear)
    {
        gcAddXObjForDObjFixed(dobj->child->child, 0x48, 0);

        if (wp->lr == -1)
        {
            dobj->child->child->rotate.vec.f.y = F_CST_DTOR32(180.0F);
        }
    }
    else
    {
        weapon_gobj->proc_display = itPippiWeaponSwarmProcDisplay;

        gcAddXObjForDObjFixed(dobj->child, 0x48, 0);

        if (wp->lr == +1)
        {
            dobj->child->rotate.vec.f.y = F_CST_DTOR32(180.0F);
        }
    }
    dobj->translate.vec.f = *pos;

    wp->is_hitlag_victim = TRUE;

    return weapon_gobj;
}

// 0x80180608
// decomp itspear.c:425-428 verbatim.
void itSpearFlyMakeSwarm(GObj *item_gobj, Vec3f *pos, s32 kind)
{
    itSpearWeaponSwarmMakeWeapon(item_gobj, pos, kind);
}

#endif /* NDS_P2_ITEM_CORE */
