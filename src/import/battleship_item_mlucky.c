/* P2 MLucky / Chansey (kind nITKindMLucky). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itmonster/itmlucky.c:1-369.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0xA84 (reloc_data.us.h:3774), the Lucky data-start base is 0x10000
 * (:3828) and the Lucky anim joint is 0x100BC (:3829); the port's generated
 * reloc header does not publish MLucky tokens, so this TU owns its
 * uintptr_t tokens the same way battleship_item_gbumper.c owns GBumper's
 * (local tokens, no generator involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (the syUtils and syVector
 * entry points, efManagerDustLightMakeEffect, and ITEM_TOGGLE_MASK_KIND
 * from decomp it/itdef.h:73) are referenced verbatim and listed in the task
 * report -- no values invented here. itGetMonsterAnimNode, itGetPData, the
 * monster SFX and voice IDs, the anim helpers, and func_800269C0_275C0 ride
 * on it/item.h, gm/gmsound.h, nds/nds_obj_anim.h, and sys/audio.h, so no
 * local externs are written for them.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/audio.h>
#include <nds/nds_obj_anim.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* The egg-toggle test (decomp it/itdef.h:73 verbatim). The port publishes
 * no item-toggle mask helper, so the one-line macro travels here. */
#ifndef ITEM_TOGGLE_MASK_KIND
#define ITEM_TOGGLE_MASK_KIND(kind) (1 << (kind))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3774. */
uintptr_t llITCommonDataMLuckyItemAttributes = 0xA84u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3828. */
uintptr_t llITCommonDataLuckyDataStart = 0x10000u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3829. */
uintptr_t llITCommonDataLuckyAnimJoint = 0x100BCu;

extern void *gITManagerCommonData;

/* decomp sys/utils.h:19. Same seam as battleship_item_bombhei.c:60-61. */
extern f32 syUtilsRandFloat(void);

/* decomp ef/efmanager.h:38. The port defines it (in the linked ELF); no
 * port header in this TU's chain publishes it, so the prototype travels
 * here verbatim, the same way battleship_item_bombhei.c:69 carries it. */
extern LBParticle *efManagerDustLightMakeEffect(Vec3f *pos, s32 lr, f32 f_index);

/* decomp itmlucky.h:8-23 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly
 * as the Nyars and Kabigon files carry theirs. */
extern void itMLuckyMakeEggInitVars(GObj *item_gobj);
extern sb32 itMLuckyFallProcUpdate(GObj *item_gobj);
extern sb32 itMLuckyFallProcMap(GObj *item_gobj);
extern void itMLuckyFallSetStatus(GObj *item_gobj);
extern sb32 itMLuckyAppearProcUpdate(GObj *item_gobj);
extern sb32 itMLuckyAppearProcMap(GObj *item_gobj);
extern void itMLuckyAppearSetStatus(GObj *item_gobj);
extern sb32 itMLuckyMakeEggProcUpdate(GObj *lucky_gobj);
extern sb32 itMLuckyMakeEggProcMap(GObj *item_gobj);
extern sb32 itMLuckyMakeEggProcDamage(GObj *item_gobj);
extern void itMLuckyMakeEggSetStatus(GObj *item_gobj);
extern sb32 itMLuckyDisappearProcUpdate(GObj *item_gobj);
extern void itMLuckyDisappearSetStatus(GObj *item_gobj);
extern sb32 itMLuckyCommonProcUpdate(GObj *item_gobj);
extern sb32 itMLuckyCommonProcMap(GObj *item_gobj);
extern GObj* itMLuckyMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

// 0x8018AFB0
// decomp itmlucky.c:12-34 verbatim, adapted only for the port's ITDesc
// shape (o_attributes is const void * here, lbRelocGetFileData takes the
// token the same way).
ITDesc dITMLuckyItemDesc =
{
    nITKindMLucky,                          // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataMLuckyItemAttributes,    // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    nGMAttackStateOff,                      // Hitbox Update State
    itMLuckyCommonProcUpdate,               // Proc Update
    itMLuckyCommonProcMap,                  // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x8018AFE4
// decomp itmlucky.c:37-86 verbatim.
ITStatusDesc dITMLuckyStatusDescs[/* */] =
{
    // Status 0 (Air Fall)
    {
        itMLuckyFallProcUpdate,             // Proc Update
        itMLuckyFallProcMap,                // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 1 (Neutral Appear)
    {
        itMLuckyAppearProcUpdate,           // Proc Update
        itMLuckyAppearProcMap,              // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 2 (Neutral Egg Spawn)
    {
        itMLuckyMakeEggProcUpdate,          // Proc Update
        itMLuckyMakeEggProcMap,             // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        itMLuckyMakeEggProcDamage           // Proc Damage
    },

    // Status 3 (Neutral Disappear)
    {
        itMLuckyDisappearProcUpdate,        // Proc Update
        itMLuckyMakeEggProcMap,             // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// decomp itmlucky.c:94-101 verbatim.
enum itMLuckyStatus
{
    nITMLuckyStatusFall,
    nITMLuckyStatusAppear,
    nITMLuckyStatusMakeEgg,
    nITMLuckyStatusDisappear,
    nITMLuckyStatusEnumCount
};

// 0x80180FC0
// decomp itmlucky.c:110-125 verbatim.
void itMLuckyMakeEggInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (ip->kind == nITKindMLucky)
    {
        gcAddDObjAnimJoint(dobj->child, itGetPData(ip, &llITCommonDataLuckyDataStart, &llITCommonDataLuckyAnimJoint), 0.0F);
        gcPlayAnimAll(item_gobj);
    }
    ip->damage_coll.hitstatus = nGMHitStatusNormal;

    ip->item_vars.mlucky.egg_spawn_wait = ITMLUCKY_EGG_SPAWN_WAIT_CONST;

    ip->multi = ITMLUCKY_EGG_SPAWN_COUNT;
}

// 0x80181048
// decomp itmlucky.c:128-135 verbatim.
sb32 itMLuckyFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITMLUCKY_GRAVITY, ITMLUCKY_TVEL);

    return FALSE;
}

// 0x80181074
// decomp itmlucky.c:138-155 verbatim.
sb32 itMLuckyFallProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMapTestAllCheckCollEnd(item_gobj);

    if (ip->coll_data.mask_curr & MAP_FLAG_FLOOR)
    {
        ip->physics.vel_air.y = 0.0F;

        if (ip->multi != 0)
        {
            itMLuckyMakeEggSetStatus(item_gobj);
        }
        else itMLuckyDisappearSetStatus(item_gobj);
    }
    return FALSE;
}

// 0x801810E0
// decomp itmlucky.c:158-166 verbatim.
void itMLuckyFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITMLuckyStatusDescs, nITMLuckyStatusFall);
}

// 0x80181124
// decomp itmlucky.c:169-176 verbatim.
sb32 itMLuckyAppearProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITMLUCKY_GRAVITY, ITMLUCKY_TVEL);

    return FALSE;
}

// 0x80181150
// decomp itmlucky.c:179-194 verbatim.
sb32 itMLuckyAppearProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMapTestAllCheckCollEnd(item_gobj);

    if (ip->coll_data.mask_curr & MAP_FLAG_FLOOR)
    {
        ip->physics.vel_air.y = 0.0F;

        itMLuckyMakeEggSetStatus(item_gobj);

        itMLuckyMakeEggInitVars(item_gobj);
    }
    return FALSE;
}

// 0x801811AC
// decomp itmlucky.c:197-206 verbatim.
void itMLuckyAppearSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->kind == nITKindMLucky)
    {
        func_800269C0_275C0(nSYAudioVoiceMBallLuckyAppear);
    }
    itMainSetStatus(item_gobj, dITMLuckyStatusDescs, nITMLuckyStatusAppear);
}

// 0x80181200
// decomp itmlucky.c:209-262 verbatim.
sb32 itMLuckyMakeEggProcUpdate(GObj *lucky_gobj)
{
    ITStruct *lucky_ip = itGetStruct(lucky_gobj), *egg_ip;
    DObj *dobj = DObjGetStruct(lucky_gobj);
    GObj *egg_gobj;
    s32 unused;
    Vec3f pos;
    Vec3f vel;

    if (lucky_ip->multi == 0)
    {
        itMLuckyDisappearSetStatus(lucky_gobj);

        return FALSE;
    }
    else
    {
        if (!lucky_ip->item_vars.mlucky.egg_spawn_wait)
        {
            if ((gSCManagerBattleState->item_toggles & ITEM_TOGGLE_MASK_KIND(nITKindEgg)) && (gSCManagerBattleState->item_appearance_rate != nSCBattleItemSwitchNone))
            {
                pos = dobj->translate.vec.f;

                vel.x = (syUtilsRandFloat() * ITMLUCKY_EGG_SPAWN_BASE_VEL) + ITMLUCKY_EGG_SPAWN_ADD_VEL_X;
                vel.y = (syUtilsRandFloat() * ITMLUCKY_EGG_SPAWN_BASE_VEL) + ITMLUCKY_EGG_SPAWN_ADD_VEL_Y;
                vel.z = 0.0F;

                egg_gobj = itManagerMakeItemSetupCommon(lucky_gobj, nITKindEgg, &pos, &vel, (ITEM_FLAG_COLLPROJECT | ITEM_FLAG_PARENT_ITEM));

                if (egg_gobj != NULL)
                {
                    egg_ip = itGetStruct(egg_gobj);

                    func_800269C0_275C0(nSYAudioFGMKirbySpecialLwStart);

                    lucky_ip->item_vars.mlucky.egg_spawn_wait = ITMLUCKY_EGG_SPAWN_WAIT_CONST;
                    lucky_ip->multi--;

                    efManagerDustLightMakeEffect(&pos, egg_ip->lr, 1.0F);
                }
            }
            else
            {
                lucky_ip->item_vars.mlucky.egg_spawn_wait = ITMLUCKY_EGG_SPAWN_WAIT_CONST;
                lucky_ip->multi--;
            }
        }
        if (lucky_ip->item_vars.mlucky.egg_spawn_wait > 0)
        {
            lucky_ip->item_vars.mlucky.egg_spawn_wait--;
        }
    }
    return FALSE;
}

// 0x80181368
// decomp itmlucky.c:265-270 verbatim.
sb32 itMLuckyMakeEggProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itMLuckyFallSetStatus);

    return FALSE;
}

// 0x80181390
// decomp itmlucky.c:273-280 verbatim.
sb32 itMLuckyMakeEggProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->item_vars.mlucky.egg_spawn_wait += ITMLUCKY_EGG_SPAWN_WAIT_ADD;

    return FALSE;
}

// 0x801813A8
// decomp itmlucky.c:283-286 verbatim.
void itMLuckyMakeEggSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITMLuckyStatusDescs, nITMLuckyStatusMakeEgg);
}

// 0x801813D0
// decomp itmlucky.c:289-300 verbatim.
sb32 itMLuckyDisappearProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->item_vars.mlucky.lifetime == 0)
    {
        return TRUE;
    }
    ip->item_vars.mlucky.lifetime--;

    return FALSE;
}

// 0x801813F8
// decomp itmlucky.c:303-312 verbatim.
void itMLuckyDisappearSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->item_vars.mlucky.lifetime = ITMLUCKY_LIFETIME;

    ip->damage_coll.hitstatus = nGMHitStatusNone;

    itMainSetStatus(item_gobj, dITMLuckyStatusDescs, nITMLuckyStatusDisappear);
}

// 0x80181430
// decomp itmlucky.c:315-328 verbatim.
sb32 itMLuckyCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        ip->physics.vel_air.y = 0.0F;

        itMLuckyAppearSetStatus(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x80181480
// decomp itmlucky.c:331-340 verbatim.
sb32 itMLuckyCommonProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR) != FALSE)
    {
        ip->physics.vel_air.y = 0.0F;
    }
    return FALSE;
}

// 0x801814C0
// decomp itmlucky.c:343-369 verbatim.
GObj* itMLuckyMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITMLuckyItemDesc, pos, vel, flags);
    DObj *dobj;
    ITStruct *ip;

    if (item_gobj != NULL)
    {
        dobj = DObjGetStruct(item_gobj);

        gcAddXObjForDObjFixed(dobj->child, 0x2C, 0);

        dobj->translate.vec.f = *pos;

        ip = itGetStruct(item_gobj);

        ip->multi = ITMONSTER_RISE_STOP_WAIT;

        ip->physics.vel_air.x = ip->physics.vel_air.z = 0.0F;
        ip->physics.vel_air.y = ITMONSTER_RISE_VEL_Y;

        dobj->translate.vec.f.y -= ip->attr->map_coll_bottom;

        gcAddDObjAnimJoint(dobj->child, itGetMonsterAnimNode(ip, &llITCommonDataLuckyDataStart), 0.0F);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
