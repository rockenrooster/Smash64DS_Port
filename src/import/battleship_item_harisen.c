/* P2-5 Fan/Harisen (kind 9, nITKindHarisen). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itharisen.c:11-266.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0x220 (decomp/BattleShip-main/include/reloc_data.us.h:3739) and the anim
 * bank base for the unused joint loader is 0x2198
 * (decomp/BattleShip-main/include/reloc_data.us.h:3785); the port's
 * generated reloc header publishes neither Harisen token, so this TU owns
 * both uintptr_t tokens the same way battleship_item_gbumper.c:28 owns
 * GBumper's (local tokens, no generator involvement, no hand-edited
 * generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3739. */
uintptr_t llITCommonDataHarisenItemAttributes = 0x220u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3785. */
uintptr_t llITCommonDataHarisenDataStart = 0x2198u;

extern void *gITManagerCommonData;

/* decomp itharisen.c:11-14 verbatim. */
intptr_t dITHarisenAnimJoint[/* */] =
{
    0x2250, 0x2270
};

/* decomp itharisen.c:16-38 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITHarisenItemDesc =
{
    nITKindHarisen,                         /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataHarisenItemAttributes,   /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyRSca,         /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itHarisenFallProcUpdate,                /* Proc Update */
    itHarisenFallProcMap,                   /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itharisen.c:40-101 verbatim. */
ITStatusDesc dITHarisenStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itHarisenWaitProcMap,               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itHarisenFallProcUpdate,            /* Proc Update */
        itHarisenFallProcMap,               /* Proc Map */
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
        itHarisenThrownProcUpdate,          /* Proc Update */
        itHarisenThrownProcMap,             /* Proc Map */
        itHarisenCommonProcHit,             /* Proc Hit */
        itHarisenCommonProcHit,             /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itHarisenCommonProcHit,             /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itHarisenFallProcUpdate,            /* Proc Update */
        itHarisenDroppedProcMap,            /* Proc Map */
        itHarisenCommonProcHit,             /* Proc Hit */
        itHarisenCommonProcHit,             /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itHarisenCommonProcHit,             /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itharisen.c:109-117 verbatim. */
enum itHarisenStatus
{
    nITHarisenStatusWait,
    nITHarisenStatusFall,
    nITHarisenStatusHold,
    nITHarisenStatusThrown,
    nITHarisenStatusDropped,
    nITHarisenStatusEnumCount
};

/* decomp itharisen.c:126-131 verbatim. */
void itHarisenCommonSetScale(GObj *item_gobj, f32 scale)
{
    DObjGetStruct(item_gobj)->scale.vec.f.x = scale;
    DObjGetStruct(item_gobj)->scale.vec.f.y = scale;
    DObjGetStruct(item_gobj)->scale.vec.f.z = scale;
}

/* decomp itharisen.c:134-142 verbatim. */
sb32 itHarisenFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITHARISEN_GRAVITY, ITHARISEN_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itharisen.c:145-150 verbatim. */
sb32 itHarisenWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itHarisenFallSetStatus);

    return FALSE;
}

/* decomp itharisen.c:153-158 verbatim. */
sb32 itHarisenFallProcMap(GObj *item_gobj)
{
    itMapCheckDestroyDropped(item_gobj, ITHARISEN_MAP_REBOUND_COMMON, ITHARISEN_MAP_REBOUND_GROUND, itHarisenWaitSetStatus);

    return FALSE;
}

/* decomp itharisen.c:161-165 verbatim. */
void itHarisenWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITHarisenStatusDescs, nITHarisenStatusWait);
}

/* decomp itharisen.c:168-176 verbatim. */
void itHarisenFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITHarisenStatusDescs, nITHarisenStatusFall);
}

/* decomp itharisen.c:179-188 verbatim. */
void itHarisenHoldSetStatus(GObj *item_gobj)
{
    DObj *dobj = DObjGetStruct(item_gobj);

    gcAddXObjForDObjFixed(dobj, nGCMatrixKindSca, 0);

    dobj->rotate.vec.f.y = 0.0F;

    itMainSetStatus(item_gobj, dITHarisenStatusDescs, nITHarisenStatusHold);
}

/* decomp itharisen.c:191-199 verbatim. */
sb32 itHarisenThrownProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITHARISEN_GRAVITY, ITHARISEN_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itharisen.c:202-205 verbatim. */
sb32 itHarisenThrownProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITHARISEN_MAP_REBOUND_COMMON, ITHARISEN_MAP_REBOUND_GROUND, itHarisenWaitSetStatus);
}

/* decomp itharisen.c:208-217 verbatim. */
sb32 itHarisenCommonProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp itharisen.c:220-225 verbatim. */
void itHarisenThrownSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITHarisenStatusDescs, nITHarisenStatusThrown);

    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(-90.0F);
}

/* decomp itharisen.c:228-231 verbatim. */
sb32 itHarisenDroppedProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITHARISEN_MAP_REBOUND_COMMON, ITHARISEN_MAP_REBOUND_GROUND, itHarisenWaitSetStatus);
}

/* decomp itharisen.c:234-239 verbatim. */
void itHarisenDroppedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITHarisenStatusDescs, nITHarisenStatusDropped);

    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(-90.0F);
}

/* decomp itharisen.c:242-248 verbatim (unused in source). */
void func_ovl3_80175408(GObj *item_gobj, s32 index) /* Unused */
{
    ITStruct *ip = itGetStruct(item_gobj);

    gcAddAnimJointAll(item_gobj, (((uintptr_t)ip->attr->data + dITHarisenAnimJoint[index]) - (intptr_t)&llITCommonDataHarisenDataStart), 0.0F);
    gcPlayAnimAll(item_gobj);
}

/* decomp itharisen.c:251-266 verbatim. */
GObj* itHarisenMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITHarisenItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        DObjGetStruct(item_gobj)->rotate.vec.f.y = F_CST_DTOR32(-90.0F);

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
