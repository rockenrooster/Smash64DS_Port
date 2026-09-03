/* P2-5 Home-Run Bat (kind 8, nITKindBat). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itbat.c:11-239.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0x1D8 (decomp/BattleShip-main/include/reloc_data.us.h:3738); the port's
 * generated reloc header does not publish a Bat token, so this TU owns its
 * uintptr_t token the same way battleship_item_gbumper.c:28 owns GBumper's
 * (local token, no generator involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <if/interface.h>
#include <gm/gmsound.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3738. */
uintptr_t llITCommonDataBatItemAttributes = 0x1D8u;

extern void *gITManagerCommonData;

/* The source header's per-kind declarations, carried with the TU: the
 * port publishes no per-kind item procs, and the descriptor above names
 * them before their definitions. Same shape as the Tomato file. */
sb32 itBatFallProcUpdate(GObj *item_gobj);
sb32 itBatWaitProcMap(GObj *item_gobj);
sb32 itBatFallProcMap(GObj *item_gobj);
void itBatWaitSetStatus(GObj *item_gobj);
void itBatFallSetStatus(GObj *item_gobj);
void itBatHoldSetStatus(GObj *item_gobj);
sb32 itBatThrownProcUpdate(GObj *item_gobj);
sb32 itBatThrownProcMap(GObj *item_gobj);
sb32 itBatThrownProcHit(GObj *item_gobj);
void itBatThrownSetStatus(GObj *item_gobj);
sb32 itBatDroppedProcMap(GObj *item_gobj);
void itBatDroppedSetStatus(GObj *item_gobj);
GObj *itBatMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);


/* decomp itbat.c:11-33 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITBatItemDesc =
{
    nITKindBat,                             /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataBatItemAttributes,       /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyR,            /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itBatFallProcUpdate,                    /* Proc Update */
    itBatFallProcMap,                       /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itbat.c:36-97 verbatim. */
ITStatusDesc dITBatStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itBatWaitProcMap,                   /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itBatFallProcUpdate,                /* Proc Update */
        itBatFallProcMap,                   /* Proc Map */
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
        itBatThrownProcUpdate,              /* Proc Update */
        itBatThrownProcMap,                 /* Proc Map */
        itBatThrownProcHit,                 /* Proc Hit */
        itBatThrownProcHit,                 /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itBatThrownProcHit,                 /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itBatFallProcUpdate,                /* Proc Update */
        itBatDroppedProcMap,                /* Proc Map */
        itBatThrownProcHit,                 /* Proc Hit */
        itBatThrownProcHit,                 /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itBatThrownProcHit,                 /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itbat.c:105-113 verbatim. */
enum itBatStatus
{
    nITBatStatusWait,
    nITBatStatusFall,
    nITBatStatusHold,
    nITBatStatusThrown,
    nITBatStatusDropped,
    nITBatStatusEnumCount
};

/* decomp itbat.c:122-130 verbatim. */
sb32 itBatFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITBAT_GRAVITY, ITBAT_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itbat.c:133-138 verbatim. */
sb32 itBatWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itBatFallSetStatus);

    return FALSE;
}

/* decomp itbat.c:141-146 verbatim. */
sb32 itBatFallProcMap(GObj *item_gobj)
{
    itMapCheckDestroyDropped(item_gobj, ITBAT_MAP_REBOUND_COMMON, ITBAT_MAP_REBOUND_GROUND, itBatWaitSetStatus);

    return FALSE;
}

/* decomp itbat.c:149-153 verbatim. */
void itBatWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITBatStatusDescs, nITBatStatusWait);
}

/* decomp itbat.c:156-164 verbatim. */
void itBatFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITBatStatusDescs, nITBatStatusFall);
}

/* decomp itbat.c:167-172 verbatim. */
void itBatHoldSetStatus(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->rotate.vec.f.y = F_CST_DTOR32(0.0F);

    itMainSetStatus(item_gobj, dITBatStatusDescs, nITBatStatusHold);
}

/* decomp itbat.c:175-181 verbatim. */
sb32 itBatThrownProcUpdate(GObj *item_gobj)
{
    itMainApplyGravityClampTVel(itGetStruct(item_gobj), ITBAT_GRAVITY, ITBAT_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itbat.c:184-187 verbatim. */
sb32 itBatThrownProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITBAT_MAP_REBOUND_COMMON, ITBAT_MAP_REBOUND_GROUND, itBatWaitSetStatus);
}

/* decomp itbat.c:190-199 verbatim. */
sb32 itBatThrownProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp itbat.c:202-207 verbatim. */
void itBatThrownSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITBatStatusDescs, nITBatStatusThrown);

    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(90.0F);
}

/* decomp itbat.c:210-213 verbatim. */
sb32 itBatDroppedProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITBAT_MAP_REBOUND_COMMON, ITBAT_MAP_REBOUND_GROUND, itBatWaitSetStatus);
}

/* decomp itbat.c:216-221 verbatim. */
void itBatDroppedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITBatStatusDescs, nITBatStatusDropped);

    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(90.0F);
}

/* decomp itbat.c:224-239 verbatim. */
GObj* itBatMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITBatItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        DObjGetStruct(item_gobj)->rotate.vec.f.y = F_CST_DTOR32(90.0F);

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
