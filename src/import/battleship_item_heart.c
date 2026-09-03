/* P2 heart (Heart, kind 5). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itheart.c:10-184.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0x100 (reloc_data.us.h:3735); the port's generated reloc header does not
 * publish a Heart token, so this TU owns its uintptr_t token the same way
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

/* decomp/BattleShip-main/include/reloc_data.us.h:3735. */
uintptr_t llITCommonDataHeartItemAttributes = 0x100u;

extern void *gITManagerCommonData;

/* decomp itheart.h:8-15 verbatim. The port header does not publish per-kind
 * item procs yet, so the source header's declarations travel with this TU. */
extern sb32 itHeartFallProcUpdate(GObj *item_gobj);
extern sb32 itHeartWaitProcMap(GObj *item_gobj);
extern sb32 itHeartFallProcMap(GObj *item_gobj);
extern void itHeartWaitSetStatus(GObj *item_gobj);
extern void itHeartFallSetStatus(GObj *item_gobj);
extern sb32 itHeartDroppedProcMap(GObj *item_gobj);
extern void itHeartDroppedSetStatus(GObj *item_gobj);
extern GObj* itHeartMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

/* decomp itheart.c:10-32 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITHeartItemDesc =
{
    nITKindHeart,                           /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataHeartItemAttributes,     /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyR,            /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itHeartFallProcUpdate,                  /* Proc Update */
    itHeartFallProcMap,                     /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itheart.c:34-71 verbatim. */
ITStatusDesc dITHeartStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itHeartWaitProcMap,                 /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itHeartFallProcUpdate,              /* Proc Update */
        itHeartFallProcMap,                 /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 2 (Fighter Drop) */
    {
        itHeartFallProcUpdate,              /* Proc Update */
        itHeartDroppedProcMap,              /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itheart.c:79-85 verbatim. */
enum itHeartStatus
{
    nITHeartStatusWait,
    nITHeartStatusFall,
    nITHeartStatusDropped,
    nITHeartStatusEnumCount
};

/* decomp itheart.c:94-102 verbatim. */
sb32 itHeartFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITHEART_GRAVITY, ITHEART_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itheart.c:105-110 verbatim. */
sb32 itHeartWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itHeartFallSetStatus);

    return FALSE;
}

/* decomp itheart.c:113-116 verbatim. */
sb32 itHeartFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITHEART_MAP_REBOUND_COMMON, ITHEART_MAP_REBOUND_GROUND, itHeartWaitSetStatus);
}

/* decomp itheart.c:119-123 verbatim. */
void itHeartWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITHeartStatusDescs, nITHeartStatusWait);
}

/* decomp itheart.c:126-134 verbatim. */
void itHeartFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITHeartStatusDescs, nITHeartStatusFall);
}

/* decomp itheart.c:137-140 verbatim. */
sb32 itHeartDroppedProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITHEART_MAP_REBOUND_COMMON, ITHEART_MAP_REBOUND_GROUND, itHeartWaitSetStatus);
}

/* decomp itheart.c:143-146 verbatim. */
void itHeartDroppedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITHeartStatusDescs, nITHeartStatusDropped);
}

/* decomp itheart.c:149-184 verbatim, REGION_US arms honoured. */
GObj* itHeartMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITHeartItemDesc, pos, vel, flags);
    DObj *dobj;
#if defined(REGION_US)
    Vec3f translate;
#endif
    ITStruct *ip;

    if (item_gobj != NULL)
    {
#if defined(REGION_US)
        dobj = DObjGetStruct(item_gobj);
        ip = itGetStruct(item_gobj);
        translate = dobj->translate.vec.f;

        gcAddXObjForDObjFixed(dobj, 0x2E, 0);

        dobj->translate.vec.f = translate;
#else
        ip = itGetStruct(item_gobj);
        dobj = DObjGetStruct(item_gobj);

        gcAddXObjForDObjFixed(dobj, 0x2E, 0);

        dobj->translate.vec.f = *pos;
#endif

        dobj->rotate.vec.f.z = 0.0F;

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
