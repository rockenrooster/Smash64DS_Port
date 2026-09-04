/* P2-5 Crate (kind 0, nITKindBox). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itbox.c:11-523.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x5CC), the attack-event row (0x614), the data start (0x6778) and the
 * smash-effect display list (0x68F0) live below
 * (decomp/BattleShip-main/include/reloc_data.us.h:3755, :3756, :3794, :3795);
 * the port's generated reloc header does not publish Box tokens, so this TU
 * owns its uintptr_t tokens the same way battleship_item_harisen.c:32-35
 * owns Harisen's attribute + data-start pair (local tokens, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITBOX_* / ITCONTAINER_*
 * tuning, itMainUpdateAttackEvent) are referenced verbatim and listed in the
 * task report -- no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <ef/effect.h>
#include <if/interface.h>
#include <gm/gmsound.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/objman.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3755. */
uintptr_t llITCommonDataBoxItemAttributes = 0x5CCu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3756. */
uintptr_t llITCommonDataBoxAttackEvents = 0x614u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3794. */
uintptr_t llITCommonDataBoxDataStart = 0x6778u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3795. */
uintptr_t llITCommonDataBoxEffectDisplayList = 0x68F0u;

extern void *gITManagerCommonData;

/* decomp itbox.h:8-28. The port publishes no per-kind item procs, so the
 * source header's declarations travel with this TU, exactly as the Tomato,
 * Bat and Harisen files carry theirs. */
void itBoxContainerSmashUpdateEffect(GObj *effect_gobj);
void itBoxContainerSmashMakeEffect(Vec3f *pos);
sb32 itBoxCommonCheckSpawnItems(GObj *item_gobj);
sb32 itBoxFallProcUpdate(GObj *item_gobj);
sb32 itBoxWaitProcMap(GObj *item_gobj);
sb32 itBoxCommonProcHit(GObj *item_gobj);
sb32 itBoxCommonProcDamage(GObj *item_gobj);
sb32 itBoxFallProcMap(GObj *item_gobj);
void itBoxWaitSetStatus(GObj *item_gobj);
void itBoxFallSetStatus(GObj *item_gobj);
void itBoxHoldSetStatus(GObj *item_gobj);
sb32 itBoxThrownProcMap(GObj *item_gobj);
void itBoxThrownSetStatus(GObj *item_gobj);
sb32 func_ovl3_801798B8(GObj *item_gobj);
sb32 itBoxDroppedProcMap(GObj *item_gobj);
void itBoxDroppedSetStatus(GObj *item_gobj);
sb32 itBoxExplodeProcUpdate(GObj *item_gobj);
GObj *itBoxMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
void itBoxExplodeInitVars(GObj *item_gobj);
void itBoxExplodeSetStatus(GObj *item_gobj);
void itBoxExplodeMakeEffectGotoSetStatus(GObj *item_gobj);

/* No port header publishes these yet (cf. battleship_link_bomb.c:75-80 and
 * battleship_item_harisen.c:39-43, which declare their missing imports the
 * same way). itMainUpdateAttackEvent is the port-missing helper the explode
 * update needs; the effect makers live in the decomp efmanager.c this port
 * imports whole into battleship_efmanager.c; syUtils* follow the
 * battleship_item_link_core.c:207 local-extern shape. */
extern void itMainUpdateAttackEvent(GObj *item_gobj, ITAttackEvent *ev);
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
extern GObj *efManagerQuakeMakeEffect(s32 id);
extern EFStruct *efManagerGetEffectNoForce(void);
extern void efManagerSetPrevStructAlloc(EFStruct *ep);
extern s32 syUtilsRandIntRange(s32 range);
extern f32 syUtilsRandFloat(void);
extern f32 syUtilsArcTan2(f32 y, f32 x);

/* decomp itbox.c:12-20 verbatim. */
Vec2f dITBoxItemSpawnVelocities[/* */] =
{
    {  0.0F, 48.0F },
    { -2.0F, 48.0F },
    {  2.0F, 48.0F },
    { -5.0F, 48.2F },
    {  0.0F, 48.2F },
    {  5.2F, 48.2F }
};

/* decomp itbox.c:23-45 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITBoxItemDesc =
{
    nITKindBox,                             /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataBoxItemAttributes,       /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyR,            /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itBoxFallProcUpdate,                    /* Proc Update */
    itBoxFallProcMap,                       /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itbox.c:48-121 verbatim. */
ITStatusDesc dITBoxStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itBoxWaitProcMap,                   /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itBoxCommonProcDamage               /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itBoxFallProcUpdate,                /* Proc Update */
        itBoxFallProcMap,                   /* Proc Map */
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
        itBoxFallProcUpdate,                /* Proc Update */
        itBoxThrownProcMap,                 /* Proc Map */
        itBoxCommonProcHit,                 /* Proc Hit */
        itBoxCommonProcHit,                 /* Proc Shield */
        NULL,                               /* Proc Hop */
        itBoxCommonProcHit,                 /* Proc Set-Off */
        itBoxCommonProcHit,                 /* Proc Reflector */
        itBoxCommonProcDamage               /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itBoxFallProcUpdate,                /* Proc Update */
        itBoxDroppedProcMap,                /* Proc Map */
        itBoxCommonProcHit,                 /* Proc Hit */
        itBoxCommonProcHit,                 /* Proc Shield */
        NULL,                               /* Proc Hop */
        itBoxCommonProcHit,                 /* Proc Set-Off */
        itBoxCommonProcHit,                 /* Proc Reflector */
        itBoxCommonProcDamage               /* Proc Damage */
    },

    /* Status 5 (Neutral Explosion) */
    {
        itBoxExplodeProcUpdate,             /* Proc Update */
        NULL,                               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itbox.c:129-138 verbatim. */
enum itBoxStatus
{
    nITBoxStatusWait,
    nITBoxStatusFall,
    nITBoxStatusHold,
    nITBoxStatusThrown,
    nITBoxStatusDropped,
    nITBoxStatusExplode,
    nITBoxStatusEnumCount
};

/* decomp itbox.c:147-173 verbatim. */
void itBoxContainerSmashUpdateEffect(GObj *effect_gobj) // Barrel/Crate smash GFX process
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *dobj = DObjGetStruct(effect_gobj);

    ep->effect_vars.container.lifetime--;

    if (ep->effect_vars.container.lifetime == 0)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
    }
    else while (dobj != NULL)
    {
        dobj->scale.vec.f.y -= 1.3F;

        dobj->translate.vec.f.x += dobj->scale.vec.f.x; // This makes no sense, seems this custom effect is very... custom
        dobj->translate.vec.f.y += dobj->scale.vec.f.y;
        dobj->translate.vec.f.z += dobj->scale.vec.f.z;

        dobj->rotate.vec.f.x += dobj->anim_wait; // ??? Seems to be rotation step, but only in this case? Otherwise -FLOAT32_MAX?
        dobj->rotate.vec.f.y += dobj->anim_speed;
        dobj->rotate.vec.f.z += dobj->anim_frame;

        dobj = dobj->sib_next;
    }
}

/* decomp itbox.c:176-217 verbatim, adapted only for the port's ITDesc
 * shape: o_attributes is the address of this TU's owned offset token, so
 * one dereference recovers the source's file offset; the DataStart /
 * EffectDisplayList terms keep the source's (intptr_t)&token shape exactly
 * as battleship_item_harisen.c:292 does. */
void itBoxContainerSmashMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    EFStruct *ep = efManagerGetEffectNoForce();
    DObj *dobj;
    s32 i;
    Gfx *dl;

    if (ep != NULL)
    {
        effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

        if (effect_gobj != NULL)
        {
            gcAddGObjDisplay(effect_gobj, gcDrawDObjTreeForGObj, 11, GOBJ_PRIORITY_DEFAULT, ~0);

            dl = (Gfx*) ((*(uintptr_t*) ((uintptr_t)*dITBoxItemDesc.p_file + *(uintptr_t*)dITBoxItemDesc.o_attributes) - (intptr_t)&llITCommonDataBoxDataStart) + (intptr_t)&llITCommonDataBoxEffectDisplayList);

            for (i = 0; i < ITCONTAINER_EFFECT_COUNT; i++)
            {
                dobj = gcAddDObjForGObj(effect_gobj, dl);

                gcAddXObjForDObjFixed(dobj, nGCMatrixKindTraRotRpyR, 0);

                dobj->translate.vec.f = *pos;

                dobj->scale.vec.f.x = (syUtilsRandFloat() * 48.0F) + -24.0F;
                dobj->scale.vec.f.y = (syUtilsRandFloat() * 50.0F) + 10.0F;
                dobj->scale.vec.f.z = (syUtilsRandFloat() * 32.0F) + -16.0F;

                dobj->anim_wait = F_CLC_DTOR32((syUtilsRandFloat() * 100.0F) + -50.0F);
                dobj->anim_speed = F_CLC_DTOR32((syUtilsRandFloat() * 100.0F) + -50.0F);
                dobj->anim_frame = F_CLC_DTOR32((syUtilsRandFloat() * 100.0F) + -50.0F);
            }
            ep->effect_vars.container.lifetime = ITCONTAINER_GFX_LIFETIME;

            effect_gobj->user_data.p = ep;

            gcAddGObjProcess(effect_gobj, itBoxContainerSmashUpdateEffect, 1, 3);
        }
    }
}

/* decomp itbox.c:220-303 verbatim. */
sb32 itBoxCommonCheckSpawnItems(GObj *item_gobj)
{
    s32 random, spawn_item_num, kind;
    s32 i, j;
    Vec2f *spawn_pos;
    Vec3f vel_identical;
    s32 unused;
    s32 weights_sum;
    s32 item_count;
    Vec3f vel_different;

    func_800269C0_275C0(nSYAudioFGMContainerSmash);

    itBoxContainerSmashMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f);

    if (gITManagerRandomWeights.weights_sum != 0)
    {
        kind = itMainGetWeightedItemKind(&gITManagerRandomWeights);

        if (kind <= nITKindCommonEnd)
        {
            random = syUtilsRandIntRange(5);

            if (random < 2)
            {
                spawn_item_num = 1;

                spawn_pos = &dITBoxItemSpawnVelocities[0];
            }
            else if (random < 3)
            {
                spawn_item_num = 2;

                spawn_pos = &dITBoxItemSpawnVelocities[1];
            }
            else
            {
                spawn_item_num = 3;

                spawn_pos = &dITBoxItemSpawnVelocities[3];
            }
            if (syUtilsRandIntRange(32) == 0) // 1 in 32 chance to spawn identical items
            {
                vel_identical.z = 0.0F;

                for (i = 0; i < spawn_item_num; i++)
                {
                    vel_identical.x = spawn_pos[i].x;
                    vel_identical.y = spawn_pos[i].y;

                    itManagerMakeItemSetupCommon(item_gobj, kind, &DObjGetStruct(item_gobj)->translate.vec.f, &vel_identical, (ITEM_FLAG_COLLPROJECT | ITEM_FLAG_PARENT_ITEM));
                }
            }
            else
            {
                weights_sum = gITManagerRandomWeights.weights_sum;
                item_count = gITManagerRandomWeights.valids_num - 1;

                gITManagerRandomWeights.weights_sum = gITManagerRandomWeights.blocks[item_count];
                gITManagerRandomWeights.valids_num--;

                vel_different.z = 0.0F;

                for (j = 0; j < spawn_item_num; j++)
                {
                    if (j != 0)
                    {
                        kind = itMainGetWeightedItemKind(&gITManagerRandomWeights);
                    }
                    vel_different.x = spawn_pos[j].x;
                    vel_different.y = spawn_pos[j].y;

                    itManagerMakeItemSetupCommon(item_gobj, kind, &DObjGetStruct(item_gobj)->translate.vec.f, &vel_different, (ITEM_FLAG_COLLPROJECT | ITEM_FLAG_PARENT_ITEM));
                }
                gITManagerRandomWeights.valids_num++;
                gITManagerRandomWeights.weights_sum = weights_sum;
            }
            func_800269C0_275C0(nSYAudioFGMFireFlowerShoot);

            return TRUE;
        }
    }
    return FALSE;
}

/* decomp itbox.c:306-314 verbatim. */
sb32 itBoxFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITBOX_GRAVITY, ITBOX_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itbox.c:317-322 verbatim. */
sb32 itBoxWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itBoxFallSetStatus);

    return FALSE;
}

/* decomp itbox.c:325-334 verbatim. */
sb32 itBoxCommonProcHit(GObj *item_gobj)
{
    if (itBoxCommonCheckSpawnItems(item_gobj) != FALSE)
    {
        return TRUE;
    }
    else itBoxExplodeMakeEffectGotoSetStatus(item_gobj);

    return FALSE;
}

/* decomp itbox.c:337-346 verbatim. */
sb32 itBoxCommonProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->percent_damage >= ITBOX_HEALTH_MAX)
    {
        return itBoxCommonProcHit(item_gobj);
    }
    else return FALSE;
}

/* decomp itbox.c:349-352 verbatim. */
sb32 itBoxFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITBOX_MAP_REBOUND_COMMON, ITBOX_MAP_REBOUND_GROUND, itBoxWaitSetStatus);
}

/* decomp itbox.c:355-363 verbatim. */
void itBoxWaitSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    DObjGetStruct(item_gobj)->rotate.vec.f.z = syUtilsArcTan2(ip->coll_data.floor_angle.y, ip->coll_data.floor_angle.x) - F_CST_DTOR32(90.0F);

    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITBoxStatusDescs, nITBoxStatusWait);
}

/* decomp itbox.c:366-374 verbatim. */
void itBoxFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITBoxStatusDescs, nITBoxStatusFall);
}

/* decomp itbox.c:377-383 verbatim. */
void itBoxHoldSetStatus(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->child->rotate.vec.f.z = 0.0F;
    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = 0.0F;

    itMainSetStatus(item_gobj, dITBoxStatusDescs, nITBoxStatusHold);
}

/* decomp itbox.c:386-397 verbatim. */
sb32 itBoxThrownProcMap(GObj *item_gobj)
{
    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_MAIN_MASK) != FALSE)
    {
        if (itBoxCommonCheckSpawnItems(item_gobj) != FALSE)
        {
            return TRUE;
        }
        else itBoxExplodeMakeEffectGotoSetStatus(item_gobj);
    }
    return FALSE;
}

/* decomp itbox.c:400-405 verbatim. */
void itBoxThrownSetStatus(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(90.0F);

    itMainSetStatus(item_gobj, dITBoxStatusDescs, nITBoxStatusThrown);
}

/* decomp itbox.c:408-413 verbatim (unused in source). */
sb32 func_ovl3_801798B8(GObj *item_gobj) // Unused
{
    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp itbox.c:416-419 verbatim. */
sb32 itBoxDroppedProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITBOX_MAP_REBOUND_COMMON, ITBOX_MAP_REBOUND_GROUND, itBoxWaitSetStatus);
}

/* decomp itbox.c:422-427 verbatim. */
void itBoxDroppedSetStatus(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(90.0F);

    itMainSetStatus(item_gobj, dITBoxStatusDescs, nITBoxStatusDropped);
}

/* decomp itbox.c:430-443 verbatim. */
sb32 itBoxExplodeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi++;

    if (ip->multi == ITBOX_EXPLODE_FRAME_END)
    {
        return TRUE;
    }
    else itMainUpdateAttackEvent(item_gobj, itGetAttackEvent(dITBoxItemDesc, &llITCommonDataBoxAttackEvents));

    return FALSE;
}

/* decomp itbox.c:446-462 verbatim. */
GObj* itBoxMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITBoxItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        DObjGetStruct(item_gobj)->rotate.vec.f.y = F_CST_DTOR32(90.0F);

        ip->is_damage_all = TRUE;
        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

/* decomp itbox.c:465-488 verbatim. */
void itBoxExplodeInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->event_id = 0;
    ip->multi = 0;

    ip->attack_coll.fgm_id = nSYAudioFGMExplodeL;

    ip->attack_coll.can_rehit_item = TRUE;
    ip->attack_coll.can_hop = FALSE;
    ip->attack_coll.can_reflect = FALSE;

    ip->attack_coll.throw_mul = ITEM_THROW_DEFAULT;
    ip->attack_coll.element = nGMHitElementFire;

    ip->attack_coll.can_setoff = FALSE;

    ip->damage_coll.hitstatus = nGMHitStatusNone;

    itMainClearOwnerStats(item_gobj);
    itMainRefreshAttackColl(item_gobj);
    itMainUpdateAttackEvent(item_gobj, itGetAttackEvent(dITBoxItemDesc, &llITCommonDataBoxAttackEvents));
}

/* decomp itbox.c:491-495 verbatim. */
void itBoxExplodeSetStatus(GObj *item_gobj)
{
    itBoxExplodeInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITBoxStatusDescs, nITBoxStatusExplode);
}

/* decomp itbox.c:498-523 verbatim. */
void itBoxExplodeMakeEffectGotoSetStatus(GObj *item_gobj)
{
    LBParticle *pc;
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    ip->physics.vel_air.x = 0.0F;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.z = 0.0F;

    pc = efManagerSparkleWhiteMultiExplodeMakeEffect(&dobj->translate.vec.f);

    if (pc != NULL)
    {
        pc->xf->scale.x =
        pc->xf->scale.y =
        pc->xf->scale.z = ITBOX_EXPLODE_SCALE;
    }
    efManagerQuakeMakeEffect(1);

    DObjGetStruct(item_gobj)->flags = DOBJ_FLAG_HIDDEN;

    itBoxExplodeSetStatus(item_gobj);
}

#endif /* NDS_P2_ITEM_CORE */
