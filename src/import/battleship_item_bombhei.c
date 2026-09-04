/* P2-5 BombHei (Bob-Omb, nITKindBombHei). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itbombhei.c:10-752.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x424), the explosion attack-event table (0x46C), the model-data base
 * (0x33F8), the walk display lists (0x3310/0x34C0) and the walk matanim
 * joint (0x35B8) are decomp/BattleShip-main/include/reloc_data.us.h:3748-
 * :3749 and :3786-:3789; the port's generated reloc header publishes none
 * of the BombHei tokens, so this TU owns all six uintptr_t tokens the same
 * way battleship_item_harisen.c:31-34 owns Harisen's (local tokens, no
 * generator involvement, no hand-edited generated file). The event table
 * is read through the port's itGetAttackEvent seam
 * (include/it/item.h:426-428), the same call shape
 * battleship_link_bomb.c:617 uses.
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <ft/fighter.h>
#include <if/interface.h>
#include <gm/gmsound.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <gr/ground.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3748. */
uintptr_t llITCommonDataBombHeiItemAttributes = 0x424u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3749. */
uintptr_t llITCommonDataBombHeiAttackEvents = 0x46Cu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3786. */
uintptr_t llITCommonDataBombHeiDataStart = 0x33F8u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3787. */
uintptr_t llITCommonDataBombHeiWalkRightDisplayList = 0x3310u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3788. */
uintptr_t llITCommonDataBombHeiWalkLeftDisplayList = 0x34C0u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3789. */
uintptr_t llITCommonDataBombHeiWalkMatAnimJoint = 0x35B8u;

extern void *gITManagerCommonData;

/* decomp sys/objanim.h:19 and :52. No port header in this TU's chain
 * publishes them; battleship_item_harisen.c:38-42 carries the same kind of
 * local externs. */
extern void gcAddMObjMatAnimJoint(MObj *mobj, AObjEvent32 *matanim_joint,
                                   f32 anim_frame);
extern void gcPlayAnimAll(GObj *gobj);

/* decomp sys/utils.h:19-:20 and sys/vector.h:33. No port header in this
 * TU's chain publishes them; battleship_item_link_core.c:205-207 carries
 * the same kind of local extern for syUtilsRandIntRange. */
extern f32 syUtilsRandFloat(void);
extern s32 syUtilsRandIntRange(s32 range);
extern Vec3f *syVectorDiff3D(Vec3f *dst, Vec3f *src, Vec3f *sub);

/* decomp ef/efmanager.h:38 and :41. Same shapes as the decomp
 * prototypes; the quake/sparkle pair below match
 * battleship_link_bomb.c:75-78. */
extern LBParticle *efManagerDustHeavyDoubleMakeEffect(Vec3f *pos, s32 lr,
                                                       f32 scale);
extern LBParticle *efManagerDustLightMakeEffect(Vec3f *pos, s32 lr,
                                                 f32 scale);
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
extern GObj *efManagerQuakeMakeEffect(s32 magnitude);
/* Same shape as battleship_link_bomb.c:80. */
extern void func_800269C0_275C0(u16 sfx_id);

/* decomp itbombhei.h:8-44 verbatim. The port header does not publish
 * per-kind item procs yet, so the source header's declarations travel with
 * this TU. */
extern void itBombHeiCommonSetExplode(GObj *item_gobj, u8 unused_arg);
extern void itBombHeiCommonSetWalkLR(GObj *item_gobj, ub8 lr);
extern void itBombHeiCommonCheckMakeDustEffect(GObj *item_gobj, u8 override);
extern void itBombHeiCommonSetHitStatusNormal(GObj *item_gobj);
extern void itBombHeiCommonSetHitStatusNone(GObj *item_gobj);
extern sb32 itBombHeiFallProcUpdate(GObj *item_gobj);
extern s32 itBombHeiWalkGetLR(GObj *item_gobj);
extern sb32 itBombHeiWaitProcUpdate(GObj *item_gobj);
extern sb32 itBombHeiWaitProcMap(GObj *item_gobj);
extern sb32 itBombHeiCommonProcHit(GObj *item_gobj);
extern sb32 itBombHeiFallProcMap(GObj *item_gobj);
extern void itBombHeiWaitSetStatus(GObj *item_gobj);
extern void itBombHeiFallSetStatus(GObj *item_gobj);
extern void itBombHeiHoldSetStatus(GObj *item_gobj);
extern sb32 itBombHeiThrownProcUpdate(GObj *item_gobj);
extern sb32 itBombHeiThrownProcMap(GObj *item_gobj);
extern void itBombHeiThrownSetStatus(GObj *item_gobj);
extern sb32 itBombHeiDroppedProcMap(GObj *item_gobj);
extern void itBombHeiDroppedSetStatus(GObj *item_gobj);
extern void itBombHeiWalkUpdateEffect(GObj *item_gobj);
extern sb32 itBombHeiWalkProcUpdate(GObj *item_gobj);
extern sb32 itBombHeiWalkProcMap(GObj *item_gobj);
extern void itBombHeiWalkInitVars(GObj *item_gobj);
extern void itBombHeiWalkSetStatus(GObj *item_gobj);
extern void itBombHeiCommonClearVelSetExplode(GObj *item_gobj, u8 unused);
extern void itBombHeiCommonUpdateAttackEvent(GObj *item_gobj);
extern sb32 itBombHeiExplodeMapProcUpdate(GObj *item_gobj);
extern sb32 itBombHeiExplodeCommonProcHit(GObj *item_gobj);
extern void itBombHeiExplodeMapSetStatus(GObj *item_gobj);
extern void itBombHeiExplodeInitVars(GObj *item_gobj);
extern sb32 itBombHeiExplodeProcUpdate(GObj *item_gobj);
extern void itBombHeiExplodeSetStatus(GObj *item_gobj);
extern sb32 itBombHeiExplodeWaitProcUpdate(GObj *item_gobj);
extern sb32 itBombHeiExplodeWaitProcMap(GObj *item_gobj);
extern void itBombHeiExplodeWaitInitVars(GObj *item_gobj);
extern void itBombHeiExplodeWaitSetStatus(GObj *item_gobj);
extern GObj *itBombHeiMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

/* decomp itbombhei.c:10-15 verbatim. The (intptr_t) casts bridge the port's
 * local-token ownership: the source's linker tokens are addresses, while
 * these tokens are variables holding offsets, so their addresses are taken
 * the same way battleship_item_harisen.c:64-67 keeps raw bank offsets. */
intptr_t dITBombHeiDisplayListOffsets[/* */] =
{
    (intptr_t)&llITCommonDataBombHeiWalkRightDisplayList,
    (intptr_t)&llITCommonDataBombHeiWalkLeftDisplayList
};

/* decomp itbombhei.c:18-40 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
/* 0x80189F98 */
ITDesc dITBombHeiItemDesc =
{
    nITKindBombHei,                         /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataBombHeiItemAttributes,   /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTra,                   /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itBombHeiFallProcUpdate,                /* Proc Update */
    itBombHeiFallProcMap,                   /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itbombhei.c:43-152 verbatim. */
/* 0x80189FCC */
ITStatusDesc dITBombHeiStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        itBombHeiWaitProcUpdate,            /* Proc Update */
        itBombHeiWaitProcMap,               /* Proc Map */
        itBombHeiCommonProcHit,             /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itBombHeiCommonProcHit              /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itBombHeiFallProcUpdate,            /* Proc Update */
        itBombHeiFallProcMap,               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itBombHeiCommonProcHit              /* Proc Damage */
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
        itBombHeiThrownProcUpdate,          /* Proc Update */
        itBombHeiThrownProcMap,             /* Proc Map */
        itBombHeiCommonProcHit,             /* Proc Hit */
        itBombHeiCommonProcHit,             /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itBombHeiCommonProcHit,             /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        itBombHeiCommonProcHit              /* Proc Damage */
    },

    /* Status 4 (Fighter Throw) */
    {
        itBombHeiFallProcUpdate,            /* Proc Update */
        itBombHeiDroppedProcMap,            /* Proc Map */
        itBombHeiCommonProcHit,             /* Proc Hit */
        itBombHeiCommonProcHit,             /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itBombHeiCommonProcHit,             /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        itBombHeiCommonProcHit              /* Proc Damage */
    },

    /* Status 5 (Ground Walk) */
    {
        itBombHeiWalkProcUpdate,            /* Proc Update */
        itBombHeiWalkProcMap,               /* Proc Map */
        itBombHeiExplodeCommonProcHit,      /* Proc Hit */
        itBombHeiExplodeCommonProcHit,      /* Proc Shield */
        NULL,                               /* Proc Hop */
        itBombHeiExplodeCommonProcHit,      /* Proc Set-Off */
        itBombHeiExplodeCommonProcHit,      /* Proc Reflector */
        itBombHeiExplodeCommonProcHit       /* Proc Damage */
    },

    /* Status 6 (Map Collision Explosion) */
    {
        itBombHeiExplodeMapProcUpdate,      /* Proc Update */
        NULL,                               /* Proc Map */
        itBombHeiExplodeMapProcUpdate,      /* Proc Hit */
        itBombHeiExplodeMapProcUpdate,      /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        itBombHeiExplodeMapProcUpdate,      /* Proc Reflector */
        itBombHeiExplodeMapProcUpdate       /* Proc Damage */
    },

    /* Status 7 (Neutral / Hit Explosion) */
    {
        itBombHeiExplodeProcUpdate,         /* Proc Update */
        NULL,                               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 8 (Ground Walk Explosion Stall) */
    {
        itBombHeiExplodeWaitProcUpdate,     /* Proc Update */
        itBombHeiExplodeWaitProcMap,        /* Proc Map */
        itBombHeiExplodeCommonProcHit,      /* Proc Hit */
        itBombHeiExplodeCommonProcHit,      /* Proc Shield */
        NULL,                               /* Proc Hop */
        itBombHeiExplodeCommonProcHit,      /* Proc Set-Off */
        itBombHeiExplodeCommonProcHit,      /* Proc Reflector */
        itBombHeiExplodeCommonProcHit       /* Proc Damage */
    }
};

/* decomp itbombhei.c:160-172 verbatim. */
enum itBombHeiStatus
{
    nITBombHeiStatusWait,
    nITBombHeiStatusFall,
    nITBombHeiStatusHold,
    nITBombHeiStatusThrown,
    nITBombHeiStatusDropped,
    nITBombHeiStatusWalk,
    nITBombHeiStatusExplodeMap,             /* Explode on map collision */
    nITBombHeiStatusExplode,               /* Neutral explosion */
    nITBombHeiStatusExplodeWait,            /* Stall until explosion */
    nITBombHeiStatusEnumCount
};

/* decomp itbombhei.c:181-207 verbatim. */
/* 0x80177060 */
void itBombHeiCommonSetExplode(GObj *item_gobj, u8 unused_arg)
{
    s32 unused;
    DObj *dobj = DObjGetStruct(item_gobj);
    ITStruct *ip = itGetStruct(item_gobj);
    LBParticle *pc;

    itBombHeiCommonSetHitStatusNone(item_gobj);

    pc = efManagerSparkleWhiteMultiExplodeMakeEffect(&dobj->translate.vec.f);

    if (pc != NULL)
    {
        pc->xf->scale.x = ITBOMBHEI_EXPLODE_SCALE;
        pc->xf->scale.y = ITBOMBHEI_EXPLODE_SCALE;
        pc->xf->scale.z = ITBOMBHEI_EXPLODE_SCALE;
    }
    efManagerQuakeMakeEffect(1);

    DObjGetStruct(item_gobj)->flags = DOBJ_FLAG_HIDDEN;

    ip->attack_coll.fgm_id = nSYAudioFGMExplodeL;

    itMainRefreshAttackColl(item_gobj);
    itMainClearOwnerStats(item_gobj);
    itBombHeiExplodeSetStatus(item_gobj);
}

/* decomp itbombhei.c:210-231 verbatim. */
/* 0x80177104 */
void itBombHeiCommonSetWalkLR(GObj *item_gobj, ub8 lr)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    Gfx *dll = itGetPData(ip, &llITCommonDataBombHeiDataStart, &llITCommonDataBombHeiWalkLeftDisplayList);  /* (void*)((uintptr_t)((uintptr_t)ip->attr->data - (uintptr_t)&llITCommonDataBombHeiDataStart) + &llITCommonDataBombHeiWalkLeftDisplayList); */
    Gfx *dlr = itGetPData(ip, &llITCommonDataBombHeiDataStart, &llITCommonDataBombHeiWalkRightDisplayList); /* (void*)((uintptr_t)((uintptr_t)ip->attr->data - (uintptr_t)&llITCommonDataBombHeiDataStart) + &llITCommonDataBombHeiWalkRightDisplayList); */

    if (lr != 0)
    {
        ip->lr = +1;
        ip->physics.vel_air.x = ITBOMBHEI_WALK_VEL_X;

        dobj->dl = dlr;
    }
    else
    {
        ip->lr = -1;
        ip->physics.vel_air.x = -ITBOMBHEI_WALK_VEL_X;

        dobj->dl = dll;
    }
}

/* decomp itbombhei.c:234-249 verbatim. */
/* 0x80177180 */
void itBombHeiCommonCheckMakeDustEffect(GObj *item_gobj, u8 override)
{
    s32 unused[4];
    ITStruct *ip = itGetStruct(item_gobj);
    ITAttributes *attr = ip->attr;
    DObj *dobj = DObjGetStruct(item_gobj);

    if ((ip->coll_data.mask_curr & MAP_FLAG_FLOOR) || (override != FALSE))
    {
        Vec3f pos = dobj->translate.vec.f;

        pos.y += attr->map_coll_bottom;

        efManagerDustHeavyDoubleMakeEffect(&pos, ip->lr, 1.0F);
    }
}

/* decomp itbombhei.c:252-257 verbatim. */
/* 0x80177208 */
void itBombHeiCommonSetHitStatusNormal(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->damage_coll.hitstatus = nGMHitStatusNormal;
}

/* decomp itbombhei.c:260-265 verbatim. */
/* 0x80177218 */
void itBombHeiCommonSetHitStatusNone(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->damage_coll.hitstatus = nGMHitStatusNone;
}

/* decomp itbombhei.c:268-276 verbatim. */
/* 0x80177224 */
sb32 itBombHeiFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITBOMBHEI_GRAVITY, ITBOMBHEI_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itbombhei.c:279-304 verbatim. */
/* 0x80177260 */
s32 itBombHeiWalkGetLR(GObj *item_gobj)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    Vec3f *translate;
    s32 lr;
    s32 ret_lr = 0;
    Vec3f dist;
    DObj *item_dobj = DObjGetStruct(item_gobj);
    DObj *fighter_dobj;

    while (fighter_gobj != NULL)
    {
        translate = &item_dobj->translate.vec.f;

        fighter_dobj = DObjGetStruct(fighter_gobj);

        syVectorDiff3D(&dist, translate, &fighter_dobj->translate.vec.f);

        lr = (dist.x < 0.0F) ? -1 : +1;

        fighter_gobj = fighter_gobj->link_next;

        ret_lr += lr;
    }
    return ret_lr; /* I assume this is getting the number of players on either side of the Bob-Omb so it starts moving towards the most crowded area */
}

/* decomp itbombhei.c:307-340 verbatim. */
/* 0x80177304 */
sb32 itBombHeiWaitProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    void *dll = itGetPData(ip, &llITCommonDataBombHeiDataStart, &llITCommonDataBombHeiWalkLeftDisplayList);
    s32 lr;

    if (ip->multi == ITBOMBHEI_WALK_WAIT)
    {
        lr = itBombHeiWalkGetLR(item_gobj);

        if (lr == 0)
        {
            lr = syUtilsRandIntRange(2) - 1;
        }
        if (lr < 0)
        {
            ip->lr = +1;
            ip->physics.vel_air.x = ITBOMBHEI_WALK_VEL_X;
        }
        else
        {
            ip->physics.vel_air.x = -ITBOMBHEI_WALK_VEL_X;

            dobj->dl = dll;

            ip->lr = -1;
        }
        itBombHeiWalkSetStatus(item_gobj);
    }
    ip->multi++;

    return FALSE;
}

/* decomp itbombhei.c:343-348 verbatim. */
/* 0x801773F4 */
sb32 itBombHeiWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itBombHeiFallSetStatus);

    return FALSE;
}

/* decomp itbombhei.c:351-356 verbatim. */
/* 0x8017741C */
sb32 itBombHeiCommonProcHit(GObj *item_gobj)
{
    itBombHeiCommonClearVelSetExplode(item_gobj, TRUE);

    return FALSE;
}

/* decomp itbombhei.c:359-362 verbatim. */
/* 0x80177440 */
sb32 itBombHeiFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITBOMBHEI_MAP_REBOUND_COMMON, ITBOMBHEI_MAP_REBOUND_GROUND, itBombHeiWaitSetStatus);
}

/* decomp itbombhei.c:365-370 verbatim. */
/* 0x80177474 */
void itBombHeiWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itBombHeiCommonSetHitStatusNormal(item_gobj);
    itMainSetStatus(item_gobj, dITBombHeiStatusDescs, nITBombHeiStatusWait);
}

/* decomp itbombhei.c:373-382 verbatim. */
/* 0x801774B0 */
void itBombHeiFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itBombHeiCommonSetHitStatusNormal(item_gobj);
    itMainSetStatus(item_gobj, dITBombHeiStatusDescs, nITBombHeiStatusFall);
}

/* decomp itbombhei.c:385-389 verbatim. */
/* 0x801774FC */
void itBombHeiHoldSetStatus(GObj *item_gobj)
{
    itBombHeiCommonSetHitStatusNone(item_gobj);
    itMainSetStatus(item_gobj, dITBombHeiStatusDescs, nITBombHeiStatusHold);
}

/* decomp itbombhei.c:392-400 verbatim. */
/* 0x80177530 */
sb32 itBombHeiThrownProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITBOMBHEI_GRAVITY, ITBOMBHEI_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itbombhei.c:403-406 verbatim. */
/* 0x8017756C */
sb32 itBombHeiThrownProcMap(GObj *item_gobj)
{
    return itMapCheckMapProcAll(item_gobj, itBombHeiExplodeMapSetStatus);
}

/* decomp itbombhei.c:409-413 verbatim. */
/* 0x80177590 */
void itBombHeiThrownSetStatus(GObj *item_gobj)
{
    itBombHeiCommonSetHitStatusNormal(item_gobj);
    itMainSetStatus(item_gobj, dITBombHeiStatusDescs, nITBombHeiStatusThrown);
}

/* decomp itbombhei.c:416-419 verbatim. */
/* 0x801775C4 */
sb32 itBombHeiDroppedProcMap(GObj *item_gobj)
{
    return itMapCheckMapProcAll(item_gobj, itBombHeiExplodeMapSetStatus);
}

/* decomp itbombhei.c:422-426 verbatim. */
/* 0x801775E8 */
void itBombHeiDroppedSetStatus(GObj *item_gobj)
{
    itBombHeiCommonSetHitStatusNormal(item_gobj);
    itMainSetStatus(item_gobj, dITBombHeiStatusDescs, nITBombHeiStatusDropped);
}

/* decomp itbombhei.c:429-445 verbatim. */
/* 0x8017761C */
void itBombHeiWalkUpdateEffect(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (ip->item_vars.bombhei.smoke_delay == 0)
    {
        Vec3f pos = dobj->translate.vec.f;

        pos.y += 120.0F;

        efManagerDustLightMakeEffect(&pos, ip->lr, 1.0F);

        ip->item_vars.bombhei.smoke_delay = ITBOMBHEI_SMOKE_WAIT;
    }
    ip->item_vars.bombhei.smoke_delay--;
}

/* decomp itbombhei.c:448-487 verbatim. */
/* 0x801776A0 */
sb32 itBombHeiWalkProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITAttributes *attr = ip->attr;
    DObj *dobj = DObjGetStruct(item_gobj);
    Vec3f pos;

    itBombHeiWalkUpdateEffect(item_gobj);

    if (mpCollisionCheckExistLineID(ip->coll_data.floor_line_id) != FALSE)
    {
        if (ip->lr == -1)
        {
            mpCollisionGetFloorEdgeL(ip->coll_data.floor_line_id, &pos);

            if (pos.x >= (dobj->translate.vec.f.x - attr->map_coll_width))
            {
                itBombHeiCommonSetWalkLR(item_gobj, 1);
            }
        }
        else
        {
            mpCollisionGetFloorEdgeR(ip->coll_data.floor_line_id, &pos);

            if (pos.x <= (dobj->translate.vec.f.x + attr->map_coll_width))
            {
                itBombHeiCommonSetWalkLR(item_gobj, 0);
            }
        }
    }
    if (ip->multi == ITBOMBHEI_FLASH_WAIT)
    {
        ip->physics.vel_air.x = ip->physics.vel_air.y = ip->physics.vel_air.z = 0.0F;

        itBombHeiExplodeWaitSetStatus(item_gobj);
    }
    ip->multi++;

    return FALSE;
}

/* decomp itbombhei.c:490-505 verbatim. */
/* 0x801777D8 */
sb32 itBombHeiWalkProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMapCheckLRWallProcNoFloor(item_gobj, itBombHeiDroppedSetStatus);

    if (ip->coll_data.mask_curr & MAP_FLAG_LWALL)
    {
        itBombHeiCommonSetWalkLR(item_gobj, 0);
    }
    if (ip->coll_data.mask_curr & MAP_FLAG_RWALL)
    {
        itBombHeiCommonSetWalkLR(item_gobj, 1);
    }
    return FALSE;
}

/* decomp itbombhei.c:508-554 verbatim. */
/* 0x80177848 */
void itBombHeiWalkInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITAttributes *attr = ip->attr;
    DObj *dobj = DObjGetStruct(item_gobj);
    AObjEvent32 *matanim_joint;
    s32 unused;
    Vec3f pos;

    ip->is_allow_pickup = FALSE;

    ip->multi = 0;

    ip->item_vars.bombhei.smoke_delay = ITBOMBHEI_SMOKE_WAIT;

    itMainRefreshAttackColl(item_gobj);

    matanim_joint = itGetPData(ip, &llITCommonDataBombHeiDataStart, &llITCommonDataBombHeiWalkMatAnimJoint);

    gcAddMObjMatAnimJoint(dobj->mobj, matanim_joint, 0.0F);
    gcPlayAnimAll(item_gobj);

    if (mpCollisionCheckExistLineID(ip->coll_data.floor_line_id) != FALSE)
    {
        if (ip->lr == -1)
        {
            mpCollisionGetFloorEdgeL(ip->coll_data.floor_line_id, &pos);

            if (pos.x >= (dobj->translate.vec.f.x - attr->map_coll_width))
            {
                itBombHeiCommonSetWalkLR(item_gobj, 1);
            }
        }
        else
        {
            mpCollisionGetFloorEdgeR(ip->coll_data.floor_line_id, &pos);

            if (pos.x <= (dobj->translate.vec.f.x + attr->map_coll_width))
            {
                itBombHeiCommonSetWalkLR(item_gobj, 0);
            }
        }
    }
    itMainClearOwnerStats(item_gobj);

    func_800269C0_275C0(nSYAudioFGMBombHeiFuse);
}

/* decomp itbombhei.c:557-562 verbatim. */
/* 0x801779A8 */
void itBombHeiWalkSetStatus(GObj *item_gobj)
{
    itBombHeiCommonSetHitStatusNormal(item_gobj);
    itBombHeiWalkInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITBombHeiStatusDescs, nITBombHeiStatusWalk);
}

/* decomp itbombhei.c:565-574 verbatim. */
/* 0x801779E4 */
void itBombHeiCommonClearVelSetExplode(GObj *item_gobj, u8 unused)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->physics.vel_air.x = ip->physics.vel_air.y = ip->physics.vel_air.z = 0.0F;

    itBombHeiCommonSetExplode(item_gobj, unused);

    func_800269C0_275C0(nSYAudioFGMExplodeL);
}

/* decomp itbombhei.c:577-602 verbatim. */
/* 0x80177A24 */
void itBombHeiCommonUpdateAttackEvent(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITAttackEvent *ev = itGetAttackEvent(dITBombHeiItemDesc, &llITCommonDataBombHeiAttackEvents);

    if (ip->multi == ev[ip->event_id].timer)
    {
        ip->attack_coll.angle = ev[ip->event_id].angle;
        ip->attack_coll.damage = ev[ip->event_id].damage;
        ip->attack_coll.size = ev[ip->event_id].size;

        ip->attack_coll.can_rehit_item = TRUE;
        ip->attack_coll.can_hop = FALSE;
        ip->attack_coll.can_reflect = FALSE;
        ip->attack_coll.can_setoff = FALSE;

        ip->attack_coll.element = nGMHitElementFire;

        ip->event_id++;

        if (ip->event_id == 4)
        {
            ip->event_id = 3;
        }
    }
}

/* decomp itbombhei.c:605-611 verbatim. */
/* 0x80177B10 */
sb32 itBombHeiExplodeMapProcUpdate(GObj *item_gobj)
{
    itBombHeiCommonCheckMakeDustEffect(item_gobj, FALSE);
    itBombHeiCommonClearVelSetExplode(item_gobj, TRUE);

    return FALSE;
}

/* decomp itbombhei.c:614-620 verbatim. */
/* 0x80177B44 */
sb32 itBombHeiExplodeCommonProcHit(GObj *item_gobj)
{
    itBombHeiCommonCheckMakeDustEffect(item_gobj, TRUE);
    itBombHeiCommonClearVelSetExplode(item_gobj, FALSE);

    return FALSE;
}

/* decomp itbombhei.c:623-627 verbatim. */
/* 0x80177B78 */
void itBombHeiExplodeMapSetStatus(GObj *item_gobj)
{
    itBombHeiCommonSetHitStatusNormal(item_gobj);
    itMainSetStatus(item_gobj, dITBombHeiStatusDescs, nITBombHeiStatusExplodeMap);
}

/* decomp itbombhei.c:630-640 verbatim. */
/* 0x80177BAC */
void itBombHeiExplodeInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = 0;

    ip->attack_coll.throw_mul = ITEM_THROW_DEFAULT;

    ip->event_id = 0;

    itBombHeiCommonUpdateAttackEvent(item_gobj);
}

/* decomp itbombhei.c:643-657 verbatim. */
/* 0x80177BE8 */
sb32 itBombHeiExplodeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itBombHeiCommonUpdateAttackEvent(item_gobj);

    ip->multi++;

    if (ip->multi == ITBOMBHEI_EXPLODE_LIFETIME)
    {
        return TRUE;
    }
    else return FALSE;
}

/* decomp itbombhei.c:660-664 verbatim. */
/* 0x80177C30 */
void itBombHeiExplodeSetStatus(GObj *item_gobj)
{
    itBombHeiExplodeInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITBombHeiStatusDescs, nITBombHeiStatusExplode);
}

/* decomp itbombhei.c:667-682 verbatim. */
/* 0x80177C64 */
sb32 itBombHeiExplodeWaitProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itBombHeiWalkUpdateEffect(item_gobj);

    if (ip->multi == ITBOMBHEI_EXPLODE_WAIT)
    {
        itBombHeiCommonCheckMakeDustEffect(item_gobj, TRUE);
        itBombHeiCommonClearVelSetExplode(item_gobj, 0);
        func_800269C0_275C0(nSYAudioFGMExplodeL);
    }
    ip->multi++;

    return FALSE;
}

/* decomp itbombhei.c:685-690 verbatim. */
/* 0x80177D00 */
sb32 itBombHeiExplodeWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itBombHeiDroppedSetStatus);

    return FALSE;
}

/* decomp itbombhei.c:693-703 verbatim. */
/* 0x80177D28 */
void itBombHeiExplodeWaitInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    ip->multi = 0;

    dobj->mobj->matanim_joint.event32 = NULL;

    itMainCheckSetColAnimID(item_gobj, nGMColAnimItemBombHeiCritical, ITBOMBHEI_EXPLODE_COLANIM_DURATION);
}

/* decomp itbombhei.c:706-711 verbatim. */
/* 0x80177D60 */
void itBombHeiExplodeWaitSetStatus(GObj *item_gobj)
{
    itBombHeiCommonSetHitStatusNormal(item_gobj);
    itBombHeiExplodeWaitInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITBombHeiStatusDescs, nITBombHeiStatusExplodeWait);
}

/* decomp itbombhei.c:714-752 verbatim, REGION_US arms honoured. */
/* 0x80177D9C */
GObj* itBombHeiMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITBombHeiItemDesc, pos, vel, flags);
    DObj *dobj;
    ITStruct *ip;
#if defined(REGION_US)
    Vec3f translate;
#endif

    if (item_gobj != NULL)
    {
        dobj = DObjGetStruct(item_gobj);

#if defined(REGION_US)
        translate = dobj->translate.vec.f;
#endif

        ip = itGetStruct(item_gobj);

        ip->multi = 0;

        itMainClearOwnerStats(item_gobj);

        gcAddXObjForDObjFixed(dobj, 0x2E, 0);

#if defined(REGION_US)
        dobj->translate.vec.f = translate;
#else
        dobj->translate.vec.f = *pos;
#endif

        ip->is_unused_item_bool = TRUE;

        dobj->rotate.vec.f.z = 0.0F;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
