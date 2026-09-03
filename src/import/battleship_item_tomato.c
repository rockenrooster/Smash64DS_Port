/* P2 tomato (Maxim Tomato, kind 4). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/ittomato.c:10-182.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0xB8 (reloc_data.us.h:3734); the port's generated reloc header does not
 * publish a Tomato token, so this TU owns its uintptr_t token the same way
 * battleship_item_gbumper.c owns GBumper's (local token, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <ft/fighter.h>
#include <if/interface.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3734. */
uintptr_t llITCommonDataTomatoItemAttributes = 0xB8u;

extern void *gITManagerCommonData;

/* decomp ittomato.h:8-15 verbatim. The port header does not publish per-kind
 * item procs yet, so the source header's declarations travel with this TU. */
extern sb32 itTomatoFallProcUpdate(GObj *item_gobj);
extern sb32 itTomatoWaitProcMap(GObj *item_gobj);
extern sb32 itTomatoFallProcMap(GObj *item_gobj);
extern void itTomatoWaitSetStatus(GObj *item_gobj);
extern void itTomatoFallSetStatus(GObj *item_gobj);
extern sb32 itTomatoDroppedProcMap(GObj *item_gobj);
extern void itTomatoDroppedSetStatus(GObj *item_gobj);
extern GObj* itTomatoMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

/* decomp ittomato.c:10-32 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITTomatoItemDesc =
{
    nITKindTomato,                          /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataTomatoItemAttributes,    /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyR,            /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itTomatoFallProcUpdate,                 /* Proc Update */
    itTomatoFallProcMap,                    /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp ittomato.c:34-71 verbatim. */
ITStatusDesc dITTomatoStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itTomatoWaitProcMap,                /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itTomatoFallProcUpdate,             /* Proc Update */
        itTomatoFallProcMap,                /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 2 (Fighter Drop) */
    {
        itTomatoFallProcUpdate,             /* Proc Update */
        itTomatoDroppedProcMap,             /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp ittomato.c:79-85 verbatim. */
enum itTomatoStatus
{
    nITTomatoStatusWait,
    nITTomatoStatusFall,
    nITTomatoStatusDropped,
    nITTomatoStatusEnumCount
};

/* decomp ittomato.c:94-102 verbatim. */
sb32 itTomatoFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITTOMATO_GRAVITY, ITTOMATO_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp ittomato.c:105-110 verbatim. */
sb32 itTomatoWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itTomatoFallSetStatus);

    return FALSE;
}

/* decomp ittomato.c:113-116 verbatim. */
sb32 itTomatoFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITTOMATO_MAP_REBOUND_COMMON, ITTOMATO_MAP_REBOUND_GROUND, itTomatoWaitSetStatus);
}

/* decomp ittomato.c:119-123 verbatim. */
void itTomatoWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITTomatoStatusDescs, nITTomatoStatusWait);
}

/* decomp ittomato.c:126-134 verbatim. */
void itTomatoFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITTomatoStatusDescs, nITTomatoStatusFall);
}

/* decomp ittomato.c:137-140 verbatim. */
sb32 itTomatoDroppedProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITTOMATO_MAP_REBOUND_COMMON, ITTOMATO_MAP_REBOUND_GROUND, itTomatoWaitSetStatus);
}

/* decomp ittomato.c:143-146 verbatim. */
void itTomatoDroppedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITTomatoStatusDescs, nITTomatoStatusDropped);
}

/* decomp ittomato.c:149-182 verbatim, REGION_US arms honoured. */
GObj* itTomatoMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITTomatoItemDesc, pos, vel, flags);
    DObj *joint;
#if defined(REGION_US)
    Vec3f translate;
#endif
    ITStruct *ip;

    if (item_gobj != NULL)
    {
#if defined(REGION_US)
        joint = DObjGetStruct(item_gobj);
        ip = itGetStruct(item_gobj);
        translate = joint->translate.vec.f;

        gcAddXObjForDObjFixed(joint, 0x2E, 0);

        joint->translate.vec.f = translate;
#else
        ip = itGetStruct(item_gobj);
        joint = DObjGetStruct(item_gobj);

        gcAddXObjForDObjFixed(joint, 0x2E, 0);

        joint->translate.vec.f = *pos;
#endif

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
