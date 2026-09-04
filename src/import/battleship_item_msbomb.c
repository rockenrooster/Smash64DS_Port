/* P2-5 MSBomb (Motion Sensor Bomb, nITKindMSBomb). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itmsbomb.c:12-639.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x3BC) and the explosion attack-event table (0x404) are
 * decomp/BattleShip-main/include/reloc_data.us.h:3746-:3747; the port's
 * generated reloc header publishes neither MSBomb token, so this TU owns
 * both uintptr_t tokens the same way battleship_item_harisen.c:31-34 owns
 * Harisen's (local tokens, no generator involvement, no hand-edited
 * generated file). The event table is read through the port's
 * itGetAttackEvent seam (include/it/item.h:426-428), the same call shape
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

/* decomp/BattleShip-main/include/reloc_data.us.h:3746. */
uintptr_t llITCommonDataMSBombItemAttributes = 0x3BCu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3747. */
uintptr_t llITCommonDataMSBombAttackEvents = 0x404u;

extern void *gITManagerCommonData;

/* decomp sys/utils.h:8 and sys/vector.h:33. No port header in this TU's
 * chain publishes them; battleship_item_link_core.c:205-207 carries the
 * same kind of local extern for syUtilsRandIntRange. */
extern f32 syUtilsArcTan2(f32 y, f32 x);
extern Vec3f *syVectorDiff3D(Vec3f *dst, Vec3f *src, Vec3f *sub);

/* decomp ef/efmanager.h:41, :64 and :69. Same shapes as
 * battleship_link_bomb.c:75-78. */
extern LBParticle *efManagerDustHeavyDoubleMakeEffect(Vec3f *pos, s32 lr,
                                                       f32 scale);
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
extern GObj *efManagerQuakeMakeEffect(s32 magnitude);
/* Same shape as battleship_link_bomb.c:80. */
extern void func_800269C0_275C0(u16 sfx_id);

/* decomp itmsbomb.h:8-35 verbatim. The port header does not publish per-kind
 * item procs yet, so the source header's declarations travel with this TU. */
extern sb32 itMSBombFallProcUpdate(GObj *item_gobj);
extern sb32 itMSBombWaitProcMap(GObj *item_gobj);
extern sb32 itMSBombFallProcMap(GObj *item_gobj);
extern void itMSBombWaitSetStatus(GObj *item_gobj);
extern void itMSBombFallSetStatus(GObj *item_gobj);
extern void itMSBombHoldSetStatus(GObj *item_gobj);
extern sb32 itMSBombThrownProcUpdate(GObj *item_gobj);
extern sb32 itMSBombThrownProcMap(GObj *item_gobj);
extern sb32 itMSBombCommonProcHit(GObj *item_gobj);
extern void itMSBombThrownSetStatus(GObj *item_gobj);
extern sb32 itMSBombDroppedProcMap(GObj *item_gobj);
extern void itMSBombDroppedSetStatus(GObj *item_gobj);
extern void itMSBombAttachedUpdateSurface(GObj *item_gobj);
extern void itMSBombAttachedInitVars(GObj *item_gobj);
extern void itMSBombExplodeMakeEffect(GObj *item_gobj);
extern void itMSBombExplodeInitStatusVars(GObj *item_gobj, sb32 is_make_effect);
extern sb32 itMSBombCommonProcDamage(GObj *item_gobj);
extern sb32 itMSBombAttachedProcUpdate(GObj *item_gobj);
extern void itMSBombAttachedSetStatus(GObj *item_gobj);
extern sb32 itMSBombAttachedProcMap(GObj *item_gobj);
extern void itMSBombExplodeUpdateAttackEvent(GObj *item_gobj);
extern void itMSBombDetachedInitVars(GObj *item_gobj);
extern sb32 itMSBombDetachedProcUpdate(GObj *item_gobj);
extern void itMSBombDetachedSetStatus(GObj *item_gobj);
extern void itMSBombExplodeInitVars(GObj *item_gobj);
extern sb32 itMSBombExplodeProcUpdate(GObj *item_gobj);
extern void itMSBombExplodeSetStatus(GObj *item_gobj);
extern GObj* itMSBombMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

/* decomp itmsbomb.c:12-34 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITMSBombItemDesc =
{
    nITKindMSBomb,                          /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataMSBombItemAttributes,    /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindNull,                  /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itMSBombFallProcUpdate,                 /* Proc Update */
    itMSBombFallProcMap,                    /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itmsbomb.c:36-133 verbatim. */
ITStatusDesc dITMSBombStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itMSBombWaitProcMap,                /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itMSBombFallProcUpdate,             /* Proc Update */
        itMSBombFallProcMap,                /* Proc Map */
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
        itMSBombThrownProcUpdate,           /* Proc Update */
        itMSBombThrownProcMap,              /* Proc Map */
        itMSBombCommonProcHit,              /* Proc Hit */
        itMSBombCommonProcHit,              /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itMSBombCommonProcHit,              /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itMSBombFallProcUpdate,             /* Proc Update */
        itMSBombDroppedProcMap,             /* Proc Map */
        itMSBombCommonProcHit,              /* Proc Hit */
        itMSBombCommonProcHit,              /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itMSBombCommonProcHit,              /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 5 (Ground Attach) */
    {
        itMSBombAttachedProcUpdate,         /* Proc Update */
        itMSBombAttachedProcMap,            /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itMSBombCommonProcDamage            /* Proc Damage */
    },

    /* Status 6 (Air Detach from Surface) */
    {
        itMSBombDetachedProcUpdate,         /* Proc Update */
        itMSBombDroppedProcMap,             /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itMSBombCommonProcDamage            /* Proc Damage */
    },

    /* Status 7 (Neutral Explosion) */
    {
        itMSBombExplodeProcUpdate,          /* Proc Update */
        NULL,                               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itmsbomb.c:142-153 verbatim. */
enum itMSBombStatus
{
    nITMSBombStatusWait,
    nITMSBombStatusFall,
    nITMSBombStatusHold,
    nITMSBombStatusThrown,
    nITMSBombStatusDropped,
    nITMSBombStatusAttached,
    nITMSBombStatusDetached,
    nITMSBombStatusExplode,
    nITMSBombStatusEnumCount
};

/* decomp itmsbomb.c:162-173 verbatim. */
/* 0x80176450 */
sb32 itMSBombFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITMSBOMB_GRAVITY, ITMSBOMB_TVEL);
    itVisualsUpdateSpin(item_gobj);

    dobj->child->sib_next->rotate.vec.f.z = dobj->rotate.vec.f.z;

    return FALSE;
}

/* decomp itmsbomb.c:176-181 verbatim. */
/* 0x801764A8 */
sb32 itMSBombWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itMSBombFallSetStatus);

    return FALSE;
}

/* decomp itmsbomb.c:184-187 verbatim. */
/* 0x801764D0 */
sb32 itMSBombFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITMSBOMB_MAP_REBOUND_COMMON, ITMSBOMB_MAP_REBOUND_GROUND, itMSBombWaitSetStatus);
}

/* decomp itmsbomb.c:190-194 verbatim. */
/* 0x80176504 */
void itMSBombWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITMSBombStatusDescs, nITMSBombStatusWait);
}

/* decomp itmsbomb.c:197-205 verbatim. */
/* 0x80176538 */
void itMSBombFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITMSBombStatusDescs, nITMSBombStatusFall);
}

/* decomp itmsbomb.c:208-211 verbatim. */
/* 0x8017657C */
void itMSBombHoldSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITMSBombStatusDescs, nITMSBombStatusHold);
}

/* decomp itmsbomb.c:214-225 verbatim. */
/* 0x801765A4 */
sb32 itMSBombThrownProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITMSBOMB_GRAVITY, ITMSBOMB_TVEL);
    itVisualsUpdateSpin(item_gobj);

    dobj->child->sib_next->rotate.vec.f.z = dobj->rotate.vec.f.z;

    return FALSE;
}

/* decomp itmsbomb.c:228-231 verbatim. */
/* 0x801765FC */
sb32 itMSBombThrownProcMap(GObj *item_gobj)
{
    return itMapCheckMapProcAll(item_gobj, itMSBombAttachedSetStatus);
}

/* decomp itmsbomb.c:234-239 verbatim. */
/* 0x80176620 */
sb32 itMSBombCommonProcHit(GObj *item_gobj)
{
    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp itmsbomb.c:242-252 verbatim. */
/* 0x80176644 */
void itMSBombThrownSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->coll_data.map_coll.top = ITMSBOMB_COLL_SIZE;
    ip->coll_data.map_coll.center = 0.0F;
    ip->coll_data.map_coll.bottom = -ITMSBOMB_COLL_SIZE;
    ip->coll_data.map_coll.width = ITMSBOMB_COLL_SIZE;

    itMainSetStatus(item_gobj, dITMSBombStatusDescs, nITMSBombStatusThrown);
}

/* decomp itmsbomb.c:255-258 verbatim. */
/* 0x80176694 */
sb32 itMSBombDroppedProcMap(GObj *item_gobj)
{
    return itMapCheckMapProcAll(item_gobj, itMSBombAttachedSetStatus);
}

/* decomp itmsbomb.c:261-271 verbatim. */
/* 0x801766B8 */
void itMSBombDroppedSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->coll_data.map_coll.top = ITMSBOMB_COLL_SIZE;
    ip->coll_data.map_coll.center = 0.0F;
    ip->coll_data.map_coll.bottom = -ITMSBOMB_COLL_SIZE;
    ip->coll_data.map_coll.width = ITMSBOMB_COLL_SIZE;

    itMainSetStatus(item_gobj, dITMSBombStatusDescs, nITMSBombStatusDropped);
}

/* decomp itmsbomb.c:274-312 verbatim. */
/* 0x80176708 */
void itMSBombAttachedUpdateSurface(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    MPCollData *coll_data = &ip->coll_data;
    Vec3f angle;
    DObj *dobj = DObjGetStruct(item_gobj);

    if ((coll_data->mask_curr & MAP_FLAG_CEIL) || (coll_data->mask_curr & MAP_FLAG_FLOOR))
    {
        if (coll_data->mask_curr & MAP_FLAG_CEIL)
        {
            angle = coll_data->ceil_angle;

            ip->attach_line_id = coll_data->ceil_line_id;
        }
        if (coll_data->mask_curr & MAP_FLAG_FLOOR)
        {
            angle = coll_data->floor_angle;

            ip->attach_line_id = coll_data->floor_line_id;
        }
    }
    else
    {
        if (coll_data->mask_curr & MAP_FLAG_LWALL)
        {
            angle = coll_data->lwall_angle;

            ip->attach_line_id = coll_data->lwall_line_id;
        }
        if (coll_data->mask_curr & MAP_FLAG_RWALL)
        {
            angle = coll_data->rwall_angle;

            ip->attach_line_id = coll_data->rwall_line_id;
        }
    }
    dobj->rotate.vec.f.z = syUtilsArcTan2(angle.y, angle.x) - F_CST_DTOR32(90.0F);
}

/* decomp itmsbomb.c:315-350 verbatim. */
/* 0x80176840 */
void itMSBombAttachedInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    ip->coll_data.map_coll.top = ITMSBOMB_COLL_SIZE;
    ip->coll_data.map_coll.center = 0.0F;
    ip->coll_data.map_coll.bottom = -ITMSBOMB_COLL_SIZE;
    ip->coll_data.map_coll.width = ITMSBOMB_COLL_SIZE;

    ip->physics.vel_air.x = ip->physics.vel_air.y = ip->physics.vel_air.z = 0;

    dobj->child->flags = DOBJ_FLAG_NONE;
    dobj->child->sib_next->flags = DOBJ_FLAG_HIDDEN;

    itMSBombAttachedUpdateSurface(item_gobj);

    ip->is_attach_surface = TRUE;

    ip->damage_coll.hitstatus = nGMHitStatusNormal;

    ip->attack_coll.attack_state = nGMAttackStateOff;

    if ((ip->player != -1) && (ip->player != GMCOMMON_PLAYERS_MAX))
    {
        GObj *fighter_gobj = gSCManagerBattleState->players[ip->player].fighter_gobj;

        if (fighter_gobj != NULL)
        {
            ftParamMakeRumble(ftGetStruct(fighter_gobj), 6, 0);
        }
    }
    func_800269C0_275C0(nSYAudioFGMMSBombAttach);

    itMainClearOwnerStats(item_gobj);
}

/* decomp itmsbomb.c:353-368 verbatim. */
/* 0x80176934 */
void itMSBombExplodeMakeEffect(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITAttributes *attr = ip->attr;
    DObj *dobj = DObjGetStruct(item_gobj);
    s32 unused[4];

    if (ip->coll_data.mask_curr & MAP_FLAG_FLOOR)
    {
        Vec3f translate = dobj->translate.vec.f;

        translate.y += attr->map_coll_bottom;

        efManagerDustHeavyDoubleMakeEffect(&translate, ip->lr, 1.0F);
    }
}

/* decomp itmsbomb.c:371-393 verbatim. */
/* 0x801769AC */
void itMSBombExplodeInitStatusVars(GObj *item_gobj, sb32 is_make_effect)
{
    LBParticle *pc;
    DObj *dobj = DObjGetStruct(item_gobj);

    if (is_make_effect != FALSE)
    {
        itMSBombExplodeMakeEffect(item_gobj);
    }
    pc = efManagerSparkleWhiteMultiExplodeMakeEffect(&dobj->translate.vec.f);

    if (pc != NULL)
    {
        pc->xf->scale.x = ITMSBOMB_EXPLODE_SCALE;
        pc->xf->scale.y = ITMSBOMB_EXPLODE_SCALE;
        pc->xf->scale.z = ITMSBOMB_EXPLODE_SCALE;
    }
    efManagerQuakeMakeEffect(1);
    itMainRefreshAttackColl(item_gobj);
    itMSBombExplodeSetStatus(item_gobj);

    DObjGetStruct(item_gobj)->flags = DOBJ_FLAG_HIDDEN;
}

/* decomp itmsbomb.c:396-402 verbatim. */
/* 0x80176A34 */
sb32 itMSBombCommonProcDamage(GObj *item_gobj)
{
    func_800269C0_275C0(nSYAudioFGMExplodeL);
    itMSBombExplodeInitStatusVars(item_gobj, FALSE);

    return FALSE;
}

/* decomp itmsbomb.c:405-445 verbatim. */
/* 0x80176A68 */
sb32 itMSBombAttachedProcUpdate(GObj *item_gobj)
{
    s32 unused[2];
    GObj *fighter_gobj;
    Vec3f *translate;
    Vec3f dist;
    Vec3f fighter_pos;
    DObj *item_dobj = DObjGetStruct(item_gobj);
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi < ITMSBOMB_DETECT_FIGHTER_DELAY)
    {
        ip->multi++;
    }
    else
    {
        fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

        translate = &item_dobj->translate.vec.f;

        while (fighter_gobj != NULL)
        {
            FTStruct *fp = ftGetStruct(fighter_gobj);
            DObj *fighter_dobj = DObjGetStruct(fighter_gobj);
            f32 var = fp->attr->map_coll.top * 0.5F;

            fighter_pos = fighter_dobj->translate.vec.f;

            fighter_pos.y += var;

            syVectorDiff3D(&dist, &fighter_pos, translate);

            if ((SQUARE(dist.x) + SQUARE(dist.y) + SQUARE(dist.z)) < ITMSBOMB_DETECT_FIGHTER_RADIUS)
            {
                itMSBombExplodeInitStatusVars(item_gobj, TRUE); /* We might want to break out of the loop here */
            }
            fighter_gobj = fighter_gobj->link_next;
        }
    }
    return FALSE;
}

/* decomp itmsbomb.c:448-452 verbatim. */
/* 0x80176B94 */
void itMSBombAttachedSetStatus(GObj *item_gobj)
{
    itMSBombAttachedInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITMSBombStatusDescs, nITMSBombStatusAttached);
}

/* decomp itmsbomb.c:455-466 verbatim. */
/* 0x80176BC8 */
sb32 itMSBombAttachedProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (mpCollisionCheckExistLineID(ip->attach_line_id) == FALSE)
    {
        ip->is_attach_surface = FALSE;

        itMSBombDetachedSetStatus(item_gobj);
    }
    return FALSE;
}

/* decomp itmsbomb.c:468-493 verbatim. */
void itMSBombExplodeUpdateAttackEvent(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITAttackEvent *ev = itGetAttackEvent(dITMSBombItemDesc, &llITCommonDataMSBombAttackEvents); /* (ITAttackEvent *)((uintptr_t)*dITMSBombItemDesc.p_file + &llITCommonDataMSBombAttackEvents); - Linker thing */

    if (ip->multi == ev[ip->event_id].timer)
    {
        ip->attack_coll.angle  = ev[ip->event_id].angle;
        ip->attack_coll.damage = ev[ip->event_id].damage;
        ip->attack_coll.size   = ev[ip->event_id].size;

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

/* decomp itmsbomb.c:496-504 verbatim. */
/* 0x80176D00 */
void itMSBombDetachedInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    ip->attack_coll.attack_state = nGMAttackStateOff;

    itMainClearOwnerStats(item_gobj);
}

/* decomp itmsbomb.c:507-549 verbatim. */
/* 0x80176D2C */
sb32 itMSBombDetachedProcUpdate(GObj *item_gobj)
{
    s32 unused[2];
    GObj *fighter_gobj;
    Vec3f *translate;
    Vec3f dist;
    Vec3f fighter_pos;
    DObj *item_dobj = DObjGetStruct(item_gobj);
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITMSBOMB_GRAVITY, ITMSBOMB_TVEL);

    if (ip->multi < ITMSBOMB_DETECT_FIGHTER_DELAY)
    {
        ip->multi++;
    }
    else
    {
        fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

        translate = &item_dobj->translate.vec.f;

        while (fighter_gobj != NULL)
        {
            FTStruct *fp = ftGetStruct(fighter_gobj);
            DObj *fighter_dobj = DObjGetStruct(fighter_gobj);
            f32 offset_y = fp->attr->map_coll.top * 0.5F;

            fighter_pos = fighter_dobj->translate.vec.f;

            fighter_pos.y += offset_y;

            syVectorDiff3D(&dist, &fighter_pos, translate);

            if ((SQUARE(dist.x) + SQUARE(dist.y) + SQUARE(dist.z)) < ITMSBOMB_DETECT_FIGHTER_RADIUS)
            {
                itMSBombExplodeInitStatusVars(item_gobj, FALSE); /* We might want to break out of the loop here */
            }
            fighter_gobj = fighter_gobj->link_next;
        }
    }
    return FALSE;
}

/* decomp itmsbomb.c:552-556 verbatim. */
/* 0x80176E68 */
void itMSBombDetachedSetStatus(GObj *item_gobj)
{
    itMSBombDetachedInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITMSBombStatusDescs, nITMSBombStatusDetached);
}

/* decomp itmsbomb.c:559-573 verbatim. */
/* 0x80176E9C */
void itMSBombExplodeInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = 0;

    ip->event_id = 0;

    ip->attack_coll.throw_mul = ITEM_THROW_DEFAULT;
    ip->attack_coll.fgm_id = nSYAudioFGMExplodeL;

    ip->damage_coll.hitstatus = nGMHitStatusNone;

    itMSBombExplodeUpdateAttackEvent(item_gobj);
}

/* decomp itmsbomb.c:576-589 verbatim. */
/* 0x80176EE4 */
sb32 itMSBombExplodeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMSBombExplodeUpdateAttackEvent(item_gobj);

    ip->multi++;

    if (ip->multi == ITMSBOMB_EXPLODE_LIFETIME)
    {
        return TRUE;
    }
    else return FALSE;
}

/* decomp itmsbomb.c:592-596 verbatim. */
/* 0x80176F2C */
void itMSBombExplodeSetStatus(GObj *item_gobj)
{
    itMSBombExplodeInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITMSBombStatusDescs, nITMSBombStatusExplode);
}

/* decomp itmsbomb.c:599-639 verbatim, REGION_US arms honoured. */
/* 0x80176F60 */
GObj* itMSBombMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITMSBombItemDesc, pos, vel, flags);
    DObj *dobj;
    ITStruct *ip;
#if defined(REGION_US)
    Vec3f translate;
#endif

    if (item_gobj != NULL)
    {
        dobj = DObjGetStruct(item_gobj);

        dobj->child->flags = DOBJ_FLAG_HIDDEN;
        dobj->child->sib_next->flags = DOBJ_FLAG_NONE;

#if defined(REGION_US)
        translate = dobj->translate.vec.f;
#endif

        gcAddXObjForDObjFixed(dobj, nGCMatrixKindTraRotRpyR, 0);
        gcAddXObjForDObjFixed(dobj->child->sib_next, 0x46, 0);

#if defined(REGION_US)
        dobj->translate.vec.f = translate;
#else
        dobj->translate.vec.f = *pos;
#endif

        ip = itGetStruct(item_gobj);

        ip->multi = 0;

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);

        dobj->rotate.vec.f.z = 0.0F;
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
