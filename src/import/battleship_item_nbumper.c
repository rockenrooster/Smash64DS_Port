/* P2-5 NBumper (item Bumper, nITKindNBumper). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itnbumper.c:11-652.
 *
 * This is the HELD/THROWN item bumper, not the stage bumper:
 * battleship_item_gbumper.c already owns the GBumper descriptor, procs and
 * maker (dITGBumperItemDesc, itGBumperCommonProcUpdate/Hit,
 * itGBumperMakeItem) and its 0xCF0 attribute token, and nothing defined
 * there is redefined here. Only the shared NBumper model-data rows are
 * reused, through this TU's own reloc tokens: the attribute row (0x69C),
 * the model-data base (0x7648), the wait MObjSub (0x7A38) and the wait
 * display list (0x7AF8) are decomp/BattleShip-main/include/reloc_data.us.h
 * :3796-:3799; the port's generated reloc header publishes none of the
 * NBumper tokens, so this TU owns all four uintptr_t tokens the same way
 * battleship_item_harisen.c:31-34 owns Harisen's (local tokens, no
 * generator involvement, no hand-edited generated file).
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
#include <gr/ground.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3796. */
uintptr_t llITCommonDataNBumperItemAttributes = 0x69Cu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3797. */
uintptr_t llITCommonDataNBumperDataStart = 0x7648u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3798. */
uintptr_t llITCommonDataNBumperWaitMObjSub = 0x7A38u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3799. */
uintptr_t llITCommonDataNBumperWaitDisplayList = 0x7AF8u;

extern void *gITManagerCommonData;

/* decomp sys/objman.h:98-:99. This TU does not include <sys/objman.h>;
 * battleship_item_harisen.c:38-42 carries the same kind of local externs
 * for unpublished gc helpers. */
extern MObj *gcAddMObjForDObj(DObj *dobj, MObjSub *mobjsub);
extern void gcRemoveMObjAll(DObj *dobj);

/* decomp sys/utils.h:8. No port header in this TU's chain publishes it;
 * battleship_item_link_core.c:205-207 carries the same kind of local
 * extern for syUtilsRandIntRange. */
extern f32 syUtilsArcTan2(f32 y, f32 x);

/* decomp itnbumper.h:8-32 verbatim. The port header does not publish
 * per-kind item procs yet, so the source header's declarations travel with
 * this TU. */
extern sb32 itNBumperFallProcUpdate(GObj *item_gobj);
extern sb32 itNBumperWaitProcMap(GObj *item_gobj);
extern sb32 itNBumperFallProcMap(GObj *item_gobj);
extern sb32 itNBumperThrownProcHit(GObj *item_gobj);
extern void itNBumperWaitSetStatus(GObj *item_gobj);
extern void itNBumperFallSetStatus(GObj *item_gobj);
extern void itNBumperHoldSetStatus(GObj *item_gobj);
extern sb32 itNBumperThrownProcUpdate(GObj *item_gobj);
extern sb32 itNBumperThrownProcMap(GObj *item_gobj);
extern sb32 itNBumperThrownProcShield(GObj *item_gobj);
extern sb32 itNBumperThrownProcReflector(GObj *item_gobj);
extern void itNBumperThrownSetStatus(GObj *item_gobj);
extern void itNBumperDroppedSetStatus(GObj *item_gobj);
extern void itNBumperAttachedSetModelPitch(GObj *item_gobj);
extern void itNBumperAttachedInitVars(GObj *item_gobj);
extern sb32 itNBumperAttachedProcHit(GObj *item_gobj);
extern sb32 itNBumperAttachedProcUpdate(GObj *item_gobj);
extern sb32 itNBumperAttachedProcMap(GObj *item_gobj);
extern sb32 itNBumperAttachedProcReflector(GObj *item_gobj);
extern void itNBumperAttachedSetStatus(GObj *item_gobj);
extern sb32 itNBumperHitAirProcUpdate(GObj *item_gobj);
extern void itNBumperHitAirSetStatus(GObj *item_gobj);
extern sb32 itNBumperGDisappearProcUpdate(GObj *item_gobj);
extern void itNBumperGDisappearSetStatus(GObj *item_gobj);
extern GObj* itNBumperMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

/* decomp itnbumper.c:11-34 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
/* 0x8018A690 */
ITDesc dITNBumperItemDesc =
{
    nITKindNBumper,                         /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataNBumperItemAttributes,   /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTra,                   /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                    /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itNBumperFallProcUpdate,                /* Proc Update */
    itNBumperFallProcMap,                   /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itnbumper.c:37-134 verbatim. */
/* 0x8018A6C4 */
ITStatusDesc dITNBumperStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itNBumperWaitProcMap,               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itNBumperFallProcUpdate,            /* Proc Update */
        itNBumperFallProcMap,               /* Proc Map */
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
        itNBumperThrownProcUpdate,          /* Proc Update */
        itNBumperThrownProcMap,             /* Proc Map */
        itNBumperThrownProcHit,             /* Proc Hit */
        itNBumperThrownProcShield,          /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        itNBumperThrownProcReflector,       /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itNBumperThrownProcUpdate,          /* Proc Update */
        itNBumperThrownProcMap,             /* Proc Map */
        itNBumperThrownProcHit,             /* Proc Hit */
        itNBumperThrownProcShield,          /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        itNBumperThrownProcReflector,       /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 5 (Ground Active Wait) */
    {
        itNBumperAttachedProcUpdate,        /* Proc Update */
        itNBumperAttachedProcMap,           /* Proc Map */
        itNBumperAttachedProcHit,           /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        itNBumperAttachedProcReflector,     /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 6 (Airborne after Ground Active Wait) */
    {
        itNBumperHitAirProcUpdate,          /* Proc Update */
        itNBumperThrownProcMap,             /* Proc Map */
        itNBumperThrownProcHit,             /* Proc Hit */
        itNBumperThrownProcShield,          /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        itNBumperThrownProcReflector,       /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 7 (Despawn) */
    {
        itNBumperGDisappearProcUpdate,      /* Proc Update */
        NULL,                               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itnbumper.c:142-153 verbatim. */
enum itNBumperStatus
{
    nITNBumperStatusWait,                   /* Ground neutral */
    nITNBumperStatusFall,                   /* Airborne neutral */
    nITNBumperStatusHold,                   /* Fighter hold */
    nITNBumperStatusThrown,                 /* Fighter throw */
    nITNBumperStatusDropped,                /* Fighter drop */
    nITNBumperStatusAttached,                /* Ground active */
    nITNBumperStatusHitAir,                   /* Airborne hit */
    nITNBumperStatusGDisappear,             /* Ground despawn */
    nITNBumperStatusEnumCount
};

/* decomp itnbumper.c:162-190 verbatim. */
/* 0x8017B430 */
sb32 itNBumperFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITBUMPER_GRAVITY_NORMAL, ITBUMPER_TVEL);

    if (ip->multi != 0)
    {
        dobj->scale.vec.f.x = dobj->scale.vec.f.y = dobj->scale.vec.f.z = (2.0F - (10 - ip->multi) * 0.1F);

        ip->multi--;
    }
    else dobj->scale.vec.f.x = dobj->scale.vec.f.y = dobj->scale.vec.f.z = 1.0F;

    if (!ip->item_vars.bumper.damage_all_delay)
    {
        itMainClearOwnerStats(item_gobj);

        ip->item_vars.bumper.damage_all_delay = -1;
    }
    if (ip->item_vars.bumper.damage_all_delay != -1)
    {
        ip->item_vars.bumper.damage_all_delay--;
    }
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itnbumper.c:193-198 verbatim. */
/* 0x8017B520 */
sb32 itNBumperWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itNBumperFallSetStatus);

    return FALSE;
}

/* decomp itnbumper.c:201-204 verbatim. */
/* 0x8017B548 */
sb32 itNBumperFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITBUMPER_MAP_REBOUND_COMMON, ITBUMPER_MAP_REBOUND_GROUND, itNBumperWaitSetStatus);
}

/* decomp itnbumper.c:207-228 verbatim. */
/* 0x8017B57C */
sb32 itNBumperThrownProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->scale.vec.f.x = 2.0F;
    dobj->scale.vec.f.y = 2.0F;
    dobj->scale.vec.f.z = 2.0F;

    ip->item_vars.bumper.hit_anim_length = ITBUMPER_HIT_ANIM_LENGTH;

    dobj->mobj->palette_id = 1.0F;

    ip->physics.vel_air.x = ITBUMPER_REBOUND_AIR_X * ip->hit_lr;
    ip->physics.vel_air.y = ITBUMPER_REBOUND_AIR_Y;

    ip->multi = ITBUMPER_HIT_SCALE;

    itNBumperHitAirSetStatus(item_gobj);

    return FALSE;
}

/* decomp itnbumper.c:231-235 verbatim. */
/* 0x8017B600 */
void itNBumperWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITNBumperStatusDescs, nITNBumperStatusWait);
}

/* decomp itnbumper.c:238-246 verbatim. */
/* 0x8017B634 */
void itNBumperFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITNBumperStatusDescs, nITNBumperStatusFall);
}

/* decomp itnbumper.c:249-252 verbatim. */
/* 0x8017B678 */
void itNBumperHoldSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITNBumperStatusDescs, nITNBumperStatusHold);
}

/* decomp itnbumper.c:255-274 verbatim. */
/* 0x8017B6A0 */
sb32 itNBumperThrownProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITBUMPER_GRAVITY_NORMAL, ITBUMPER_TVEL);

    if (!(ip->item_vars.bumper.damage_all_delay))
    {
        itMainClearOwnerStats(item_gobj);

        ip->item_vars.bumper.damage_all_delay = -1;
    }
    if (ip->item_vars.bumper.damage_all_delay != -1)
    {
        ip->item_vars.bumper.damage_all_delay--;
    }
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itnbumper.c:277-280 verbatim. */
/* 0x8017B720 */
sb32 itNBumperThrownProcMap(GObj *item_gobj)
{
    return itMapCheckMapReboundProcNoFloor(item_gobj, 0.8F, itNBumperAttachedSetStatus);
}

/* decomp itnbumper.c:283-289 verbatim. */
/* 0x8017B74C */
sb32 itNBumperThrownProcShield(GObj *item_gobj)
{
    itMainVelSetRebound(item_gobj);
    itMainClearOwnerStats(item_gobj);

    return FALSE;
}

/* decomp itnbumper.c:292-304 verbatim. */
/* 0x8017B778 */
sb32 itNBumperThrownProcReflector(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    FTStruct *fp = ftGetStruct(ip->owner_gobj);

    if ((ip->physics.vel_air.x * fp->lr) < 0.0F)
    {
        ip->physics.vel_air.x = -ip->physics.vel_air.x;
    }
    itMainClearOwnerStats(item_gobj);

    return FALSE;
}

/* decomp itnbumper.c:307-317 verbatim. */
/* 0x8017B7DC */
void itNBumperThrownSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->item_vars.bumper.damage_all_delay = ITBUMPER_DAMAGE_ALL_WAIT;

    ip->coll_data.map_coll.top = ITBUMPER_COLL_SIZE;
    ip->coll_data.map_coll.bottom = -ITBUMPER_COLL_SIZE;

    itMainSetStatus(item_gobj, dITNBumperStatusDescs, nITNBumperStatusThrown);
}

/* decomp itnbumper.c:320-330 verbatim. */
/* 0x8017B828 */
void itNBumperDroppedSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->item_vars.bumper.damage_all_delay = ITBUMPER_DAMAGE_ALL_WAIT;

    ip->coll_data.map_coll.top = ITBUMPER_COLL_SIZE;
    ip->coll_data.map_coll.bottom = -ITBUMPER_COLL_SIZE;

    itMainSetStatus(item_gobj, dITNBumperStatusDescs, nITNBumperStatusDropped);
}

/* decomp itnbumper.c:333-345 verbatim. */
/* 0x8017B874 */
void itNBumperAttachedSetModelPitch(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    s32 unused;
    Vec3f floor_angle;
    DObj *dobj = DObjGetStruct(item_gobj);

    floor_angle = ip->coll_data.floor_angle;

    ip->attach_line_id = ip->coll_data.floor_line_id;

    dobj->rotate.vec.f.z = syUtilsArcTan2(floor_angle.y, floor_angle.x) - F_CLC_DTOR32(90.0F);
}

/* decomp itnbumper.c:348-384 verbatim. */
/* 0x8017B8DC */
void itNBumperAttachedInitVars(GObj *item_gobj)
{
    s32 unused[2];
    DObj *dobj;
    ITStruct *ip;
    MObjSub *mobjsub;
    Gfx *dl;

    ip = itGetStruct(item_gobj);
    dobj = DObjGetStruct(item_gobj);

    ip->physics.vel_air.x = 0.0F;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.z = 0.0F;

    dl = itGetPData(ip, &llITCommonDataNBumperDataStart, &llITCommonDataNBumperWaitDisplayList); /* (uintptr_t)((uintptr_t)ip->attr->data - (intptr_t)&llITCommonDataNBumperDataStart) + (intptr_t)&llITCommonDataNBumperWaitDisplayList; Linker thing */

    dobj->dl = dl;

    mobjsub = itGetPData(ip, &llITCommonDataNBumperDataStart, &llITCommonDataNBumperWaitMObjSub); /* ((uintptr_t)((uintptr_t)ip->attr->data - (intptr_t)&llITCommonDataNBumperDataStart) + (intptr_t)&llITCommonDataNBumperWaitMObjSub); */

    gcRemoveMObjAll(dobj);
    gcAddMObjForDObj(dobj, mobjsub);

    dobj->scale.vec.f.x = dobj->scale.vec.f.y = dobj->scale.vec.f.z = 1.0F;

    ip->coll_data.map_coll.top = ITBUMPER_COLL_SIZE;
    ip->coll_data.map_coll.bottom = -ITBUMPER_COLL_SIZE;

    itNBumperAttachedSetModelPitch(item_gobj);

    ip->is_attach_surface = TRUE;

    ip->lifetime = ITBUMPER_LIFETIME;

    itMainClearOwnerStats(item_gobj);
}

/* decomp itnbumper.c:387-406 verbatim. */
/* 0x8017B9C8 */
sb32 itNBumperAttachedProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->scale.vec.f.x = 2.0F;
    dobj->scale.vec.f.z = 2.0F;

    ip->item_vars.bumper.hit_anim_length = ITBUMPER_HIT_ANIM_LENGTH;

    dobj->mobj->palette_id = 1.0F;

    ip->lr = -ip->hit_lr;

    ip->physics.vel_air.x = ip->hit_lr * ITBUMPER_REBOUND_VEL_X;

    ip->multi = ITBUMPER_HIT_SCALE;

    return FALSE;
}

/* decomp itnbumper.c:409-462 verbatim. */
/* 0x8017BA2C */
sb32 itNBumperAttachedProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITAttributes *attr = ip->attr;
    DObj *dobj = DObjGetStruct(item_gobj);
    Vec3f edge_pos;

    if ((ip->item_vars.bumper.hit_anim_length == 0) && (dobj->mobj->palette_id == 1.0F))
    {
        dobj->mobj->palette_id = 0.0F;
    }
    else ip->item_vars.bumper.hit_anim_length--;

    if (mpCollisionCheckExistLineID(ip->coll_data.floor_line_id) != FALSE)
    {
        if (ip->lr == -1)
        {
            mpCollisionGetFloorEdgeL(ip->coll_data.floor_line_id, &edge_pos);

            if (edge_pos.x >= (dobj->translate.vec.f.x - attr->map_coll_width))
            {
                ip->physics.vel_air.x = 0.0F;
            }
        }
        else
        {
            mpCollisionGetFloorEdgeR(ip->coll_data.floor_line_id, &edge_pos);

            if (edge_pos.x <= (dobj->translate.vec.f.x + attr->map_coll_width))
            {
                ip->physics.vel_air.x = 0.0F;
            }
        }
    }
    if (ip->multi < ITBUMPER_STOPVEL_WAIT)
    {
        ip->physics.vel_air.x = 0.0F;
    }
    if (ip->multi != 0)
    {
        dobj->scale.vec.f.x = dobj->scale.vec.f.z = 2.0F - ((10 - ip->multi) * 0.1F);

        ip->multi--;
    }
    else dobj->scale.vec.f.x = dobj->scale.vec.f.y = dobj->scale.vec.f.z = 1.0F;

    if (ip->lifetime == 0)
    {
        itNBumperGDisappearSetStatus(item_gobj);
    }
    ip->lifetime--;

    return FALSE;
}

/* decomp itnbumper.c:465-490 verbatim. */
/* 0x8017BBFC */
sb32 itNBumperAttachedProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *joint = DObjGetStruct(item_gobj);

    if (itMapCheckLRWallProcNoFloor(item_gobj, itNBumperDroppedSetStatus) != FALSE)
    {
        if (mpCollisionCheckExistLineID(ip->attach_line_id) == FALSE)
        {
            ip->is_attach_surface = FALSE;

            itNBumperDroppedSetStatus(item_gobj);

#if defined(REGION_US)
            joint->scale.vec.f.x = joint->scale.vec.f.y = joint->scale.vec.f.z = 1.0F;

            joint->mobj->palette_id = 0.0F;
#endif
        }
        else if (ip->multi == 0)
        {
            itNBumperAttachedSetModelPitch(item_gobj);
        }
    }
    return FALSE;
}

/* decomp itnbumper.c:493-515 verbatim. */
/* 0x8017BCC0 */
sb32 itNBumperAttachedProcReflector(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    FTStruct *fp = ftGetStruct(ip->owner_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->scale.vec.f.x = 2.0F;
    dobj->scale.vec.f.z = 2.0F;

    ip->item_vars.bumper.hit_anim_length = 3;

    dobj->mobj->palette_id = 1.0F;

    ip->physics.vel_air.x = (-fp->lr * ITBUMPER_REBOUND_VEL_X);

    ip->lr = fp->lr;

    ip->multi = ITBUMPER_HIT_SCALE;

    itMainClearOwnerStats(item_gobj);

    return FALSE;
}

/* decomp itnbumper.c:518-522 verbatim. */
/* 0x8017BD4C */
void itNBumperAttachedSetStatus(GObj *item_gobj)
{
    itNBumperAttachedInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITNBumperStatusDescs, nITNBumperStatusAttached);
}

/* decomp itnbumper.c:525-557 verbatim. */
/* 0x8017BD80 */
sb32 itNBumperHitAirProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if ((ip->item_vars.bumper.hit_anim_length == 0) && (dobj->mobj->palette_id == 1.0F))
    {
        dobj->mobj->palette_id = 0.0F;
    }
    else ip->item_vars.bumper.hit_anim_length--;

    itMainApplyGravityClampTVel(ip, ITBUMPER_GRAVITY_HIT, ITBUMPER_TVEL);

    if (ip->multi != 0)
    {
        dobj->scale.vec.f.x = dobj->scale.vec.f.y = dobj->scale.vec.f.z = (2.0F - (10 - ip->multi) * 0.1F);

        ip->multi--;
    }
    else dobj->scale.vec.f.x = dobj->scale.vec.f.y = dobj->scale.vec.f.z = 1;

    if (!ip->item_vars.bumper.damage_all_delay)
    {
        itMainClearOwnerStats(item_gobj);

        ip->item_vars.bumper.damage_all_delay = -1;
    }
    if (ip->item_vars.bumper.damage_all_delay != -1)
    {
        ip->item_vars.bumper.damage_all_delay--;
    }
    return FALSE;
}

/* decomp itnbumper.c:560-567 verbatim. */
/* 0x8017BEA0 */
void itNBumperHitAirSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->item_vars.bumper.damage_all_delay = ITBUMPER_DAMAGE_ALL_WAIT;

    itMainSetStatus(item_gobj, dITNBumperStatusDescs, nITNBumperStatusHitAir);
}

/* decomp itnbumper.c:570-587 verbatim. */
/* 0x8017BED4 */
sb32 itNBumperGDisappearProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->lifetime == 0)
    {
        return TRUE;
    }
    else if ((ip->lifetime % 2) != 0)
    {
        DObj *dobj = DObjGetStruct(item_gobj);

        dobj->flags ^= DOBJ_FLAG_HIDDEN;
    }
    ip->lifetime--;

    return FALSE;
}

/* decomp itnbumper.c:590-612 verbatim. */
/* 0x8017BF1C */
void itNBumperGDisappearSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->mobj->palette_id = 0;

    dobj->scale.vec.f.x = 1.0F;
    dobj->scale.vec.f.y = 1.0F;
    dobj->scale.vec.f.z = 1.0F;

    ip->lifetime = ITBUMPER_DESPAWN_TIMER;

    dobj->flags = DOBJ_FLAG_NONE;

    ip->attack_coll.attack_state = nGMAttackStateOff;

    ip->physics.vel_air.x = 0.0F;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.z = 0.0F;

    itMainSetStatus(item_gobj, dITNBumperStatusDescs, nITNBumperStatusGDisappear);
}

/* decomp itnbumper.c:615-652 verbatim, REGION_US arms honoured. */
/* 0x8017BF8C */
GObj* itNBumperMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITNBumperItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        DObj *dobj = DObjGetStruct(item_gobj);
        ITStruct *ip;
#if defined(REGION_US)
        Vec3f translate = dobj->translate.vec.f;
#endif

        ip = itGetStruct(item_gobj);

        ip->multi = 0;

        ip->attack_coll.interact_mask = GMHITCOLLISION_FLAG_FIGHTER;

        ip->attack_coll.can_rehit_shield = TRUE;

        dobj->mobj->palette_id = 0.0F;

        gcAddXObjForDObjFixed(dobj, 0x2E, 0);

#if defined(REGION_US)
        dobj->translate.vec.f = translate;
#else
        dobj->translate.vec.f = *pos;
#endif

        dobj->rotate.vec.f.z = 0.0F;

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
