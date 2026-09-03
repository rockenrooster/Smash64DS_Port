/* P2-5 Beam Sword (kind 7, nITKindSword). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itsword.c:10-226.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0x190 (decomp/BattleShip-main/include/reloc_data.us.h:3737); the port's
 * generated reloc header does not publish a Sword token, so this TU owns its
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

/* decomp/BattleShip-main/include/reloc_data.us.h:3737. */
uintptr_t llITCommonDataSwordItemAttributes = 0x190u;

extern void *gITManagerCommonData;

/* The source header's per-kind declarations, carried with the TU: the
 * port publishes no per-kind item procs, and the descriptor above names
 * them before their definitions. Same shape as the Tomato file. */
sb32 itSwordFallProcUpdate(GObj *item_gobj);
sb32 itSwordWaitProcMap(GObj *item_gobj);
sb32 itSwordFallProcMap(GObj *item_gobj);
void itSwordWaitSetStatus(GObj *item_gobj);
void itSwordFallSetStatus(GObj *item_gobj);
void itSwordHoldSetStatus(GObj *item_gobj);
sb32 itSwordThrownProcMap(GObj *item_gobj);
sb32 itSwordThrownProcHit(GObj *item_gobj);
void itSwordThrownSetStatus(GObj *item_gobj);
sb32 itSwordDroppedProcMap(GObj *item_gobj);
void itSwordDroppedSetStatus(GObj *item_gobj);
GObj *itSwordMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);


/* decomp itsword.c:10-32 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITSwordItemDesc =
{
    nITKindSword,                           /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataSwordItemAttributes,     /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyRSca,         /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itSwordFallProcUpdate,                  /* Proc Update */
    itSwordFallProcMap,                     /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itsword.c:34-95 verbatim. */
ITStatusDesc dITSwordStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itSwordWaitProcMap,                 /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itSwordFallProcUpdate,              /* Proc Update */
        itSwordFallProcMap,                 /* Proc Map */
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
        itSwordFallProcUpdate,              /* Proc Update */
        itSwordThrownProcMap,               /* Proc Map */
        itSwordThrownProcHit,               /* Proc Hit */
        itSwordThrownProcHit,               /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itSwordThrownProcHit,               /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itSwordFallProcUpdate,              /* Proc Update */
        itSwordDroppedProcMap,              /* Proc Map */
        itSwordThrownProcHit,               /* Proc Hit */
        itSwordThrownProcHit,               /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itSwordThrownProcHit,               /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itsword.c:103-111 verbatim. */
enum itSwordStatus
{
    nITSwordStatusWait,
    nITSwordStatusFall,
    nITSwordStatusHold,
    nITSwordStatusThrown,
    nITSwordStatusDropped,
    nITSwordStatusEnumCount
};

/* decomp itsword.c:120-128 verbatim. */
sb32 itSwordFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITSWORD_GRAVITY, ITSWORD_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itsword.c:131-136 verbatim. */
sb32 itSwordWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itSwordFallSetStatus);

    return FALSE;
}

/* decomp itsword.c:139-142 verbatim. */
sb32 itSwordFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITSWORD_MAP_REBOUND_COMMON, ITSWORD_MAP_REBOUND_GROUND, itSwordWaitSetStatus);
}

/* decomp itsword.c:145-149 verbatim. */
void itSwordWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITSwordStatusDescs, nITSwordStatusWait);
}

/* decomp itsword.c:152-160 verbatim. */
void itSwordFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITSwordStatusDescs, nITSwordStatusFall);
}

/* decomp itsword.c:163-168 verbatim. */
void itSwordHoldSetStatus(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->rotate.vec.f.y = F_CST_DTOR32(0.0F);

    itMainSetStatus(item_gobj, dITSwordStatusDescs, nITSwordStatusHold);
}

/* decomp itsword.c:171-174 verbatim. */
sb32 itSwordThrownProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITSWORD_MAP_REBOUND_COMMON, ITSWORD_MAP_REBOUND_GROUND, itSwordWaitSetStatus);
}

/* decomp itsword.c:177-186 verbatim. */
sb32 itSwordThrownProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp itsword.c:189-194 verbatim. */
void itSwordThrownSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITSwordStatusDescs, nITSwordStatusThrown);

    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(90.0F);
}

/* decomp itsword.c:197-200 verbatim. */
sb32 itSwordDroppedProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITSWORD_MAP_REBOUND_COMMON, ITSWORD_MAP_REBOUND_GROUND, itSwordWaitSetStatus);
}

/* decomp itsword.c:203-208 verbatim. */
void itSwordDroppedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITSwordStatusDescs, nITSwordStatusDropped);

    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(90.0F);
}

/* decomp itsword.c:211-226 verbatim. */
GObj* itSwordMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITSwordItemDesc, pos, vel, flags);

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
