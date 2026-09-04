/* P2 Tosakinto / Goldeen (kind nITKindTosakinto). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itmonster/ittosakinto.c:1-253.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0x7F0 (reloc_data.us.h:3764); the splash-anim joints ride on the DataStart
 * 0xB708 (reloc_data.us.h:3811) via AnimJoint 0xB7CC (:3812) and MatAnimJoint
 * 0xB90C (:3813). The port's generated reloc header does not publish
 * Tosakinto tokens, so this TU owns its uintptr_t tokens the same way
 * battleship_item_gbumper.c owns GBumper's (local tokens, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITTOSAKINTO_*/ITMONSTER_*
 * tuning, itGetPData/itGetMonsterAnimNode, SFX/voice IDs, map/anim helpers)
 * are referenced verbatim and listed in the task report -- no values invented
 * here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3764. */
uintptr_t llITCommonDataTosakintoItemAttributes = 0x7F0u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3811. */
uintptr_t llITCommonDataTosakintoDataStart = 0xB708u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3812. */
uintptr_t llITCommonDataTosakintoAnimJoint = 0xB7CCu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3813. */
uintptr_t llITCommonDataTosakintoMatAnimJoint = 0xB90Cu;

extern void *gITManagerCommonData;

/* decomp ittosakinto.h:8-17 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly as
 * the Tomato and Star files carry theirs. */
extern sb32 itTosakintoAppearProcUpdate(GObj *item_gobj);
extern sb32 itTosakintoAppearProcMap(GObj *item_gobj);
extern void itTosakintoAppearSetStatus(GObj *item_gobj);
extern sb32 itTosakintoBounceProcUpdate(GObj *item_gobj);
extern sb32 itTosakintoBounceProcMap(GObj *item_gobj);
extern void itTosakintoBounceInitVars(GObj *item_gobj);
extern void itTosakintoBounceSetStatus(GObj *item_gobj);
extern sb32 itTosakintoCommonProcUpdate(GObj *item_gobj);
extern sb32 itTosakintoCommonProcMap(GObj *item_gobj);
extern GObj* itTosakintoMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

/* decomp ittosakinto.c:11-33 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITTosakintoItemDesc =
{
    nITKindTosakinto,                       // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataTosakintoItemAttributes, // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindNull,                   // Main matrix transformations
        nGCMatrixKindNull,                   // Secondary matrix transformations?
        0,                                   // ???
    },

    nGMAttackStateOff,                      // Hitbox Update State
    itTosakintoCommonProcUpdate,            // Proc Update
    itTosakintoCommonProcMap,               // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x8018ABF4
// decomp ittosakinto.c:36-61 verbatim.
ITStatusDesc dITTosakintoStatusDescs[/* */] =
{
    // Status 0 (Neutral Appear)
    {
        itTosakintoAppearProcUpdate,        // Proc Update
        itTosakintoAppearProcMap,           // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 1 (Neutral Splash)
    {
        itTosakintoBounceProcUpdate,        // Proc Update
        itTosakintoBounceProcMap,           // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// decomp ittosakinto.c:69-74 verbatim.
enum itTosakintoStatus
{
    nITTosakintoStatusAppear,
    nITTosakintoStatusBounce,
    nITTosakintoStatusEnumCount
};

// 0x8017E7A0
// decomp ittosakinto.c:83-90 verbatim.
sb32 itTosakintoAppearProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITTOSAKINTO_GRAVITY, ITTOSAKINTO_TVEL);

    return FALSE;
}

// 0x8017E7CC
// decomp ittosakinto.c:93-108 verbatim.
sb32 itTosakintoAppearProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMapTestAllCheckCollEnd(item_gobj);

    if (ip->coll_data.mask_curr & MAP_FLAG_FLOOR)
    {
        ip->physics.vel_air.y = ITTOSAKINTO_FLAP_VEL_Y;

        itTosakintoBounceSetStatus(item_gobj);

        func_800269C0_275C0(nSYAudioFGMTosakintoSplash);
    }
    return FALSE;
}

// 0x8017E828
// decomp ittosakinto.c:111-122 verbatim.
void itTosakintoAppearSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = ITTOSAKINTO_LIFETIME;

    if (ip->kind == nITKindTosakinto)
    {
        func_800269C0_275C0(nSYAudioVoiceMBallTosakintoAppear);
    }
    itMainSetStatus(item_gobj, dITTosakintoStatusDescs, nITTosakintoStatusAppear);
}

// 0x8017E880
// decomp ittosakinto.c:125-138 verbatim.
sb32 itTosakintoBounceProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITTOSAKINTO_GRAVITY, ITTOSAKINTO_TVEL);

    if (ip->multi == 0)
    {
        return TRUE;
    }
    ip->multi--;

    return FALSE;
}

// 0x8017E8CC
// decomp ittosakinto.c:141-158 verbatim.
sb32 itTosakintoBounceProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMapTestAllCheckCollEnd(item_gobj);

    if (ip->coll_data.mask_curr & MAP_FLAG_FLOOR)
    {
        ip->physics.vel_air.y = ITTOSAKINTO_FLAP_VEL_Y;

        if (syUtilsRandIntRange(2) != 0)
        {
            ip->physics.vel_air.x = -ip->physics.vel_air.x;
        }
        func_800269C0_275C0(nSYAudioFGMTosakintoSplash);
    }
    return FALSE;
}

// 0x8017E93C
// decomp ittosakinto.c:161-186 verbatim.
void itTosakintoBounceInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    void *anim_joint;
    void *matanim_joint;
    s32 unused;

    ip->item_vars.tosakinto.pos = dobj->translate.vec.f;

    ip->physics.vel_air.y = ITTOSAKINTO_FLAP_VEL_Y;
    ip->physics.vel_air.x = ITTOSAKINTO_FLAP_VEL_X;

    if (ip->kind == nITKindTosakinto)
    {
        anim_joint = itGetPData(ip, &llITCommonDataTosakintoDataStart, &llITCommonDataTosakintoAnimJoint);

        gcAddDObjAnimJoint(dobj->child, anim_joint, 0.0F);

        matanim_joint = itGetPData(ip, &llITCommonDataTosakintoDataStart, &llITCommonDataTosakintoMatAnimJoint);

        gcAddMObjMatAnimJoint(dobj->child->mobj, matanim_joint, 0.0F);

        gcPlayAnimAll(item_gobj);
    }
}

// 0x8017EA14
// decomp ittosakinto.c:189-193 verbatim.
void itTosakintoBounceSetStatus(GObj *item_gobj)
{
    itTosakintoBounceInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITTosakintoStatusDescs, nITTosakintoStatusBounce);
}

// 0x8017EA48
// decomp ittosakinto.c:196-209 verbatim.
sb32 itTosakintoCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        ip->physics.vel_air.y = 0.0F;

        itTosakintoAppearSetStatus(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x8017EA98
// decomp ittosakinto.c:212-221 verbatim.
sb32 itTosakintoCommonProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR) != FALSE)
    {
        ip->physics.vel_air.y = 0.0F;
    }
    return FALSE;
}

// 0x8017EAD8
// decomp ittosakinto.c:224-253 verbatim.
GObj* itTosakintoMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITTosakintoItemDesc, pos, vel, flags);
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

        ip->physics.vel_air.x = ip->physics.vel_air.z = 0.0F;
        ip->physics.vel_air.y = ITMONSTER_RISE_VEL_Y;

        dobj->translate.vec.f.y -= ip->attr->map_coll_bottom;

        gcAddDObjAnimJoint(dobj, itGetMonsterAnimNode(ip, &llITCommonDataTosakintoDataStart), 0.0F);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
