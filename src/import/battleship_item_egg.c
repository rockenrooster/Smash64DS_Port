/* P2-5 Egg (kind 3, nITKindEgg). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itegg.c:10-393.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0xACC) and the attack-event row (0xB14) live below
 * (decomp/BattleShip-main/include/reloc_data.us.h:3775, :3776); the port's
 * generated reloc header does not publish Egg tokens, so this TU owns its
 * uintptr_t tokens the same way battleship_item_harisen.c:32-35 owns
 * Harisen's pair (local tokens, no generator involvement, no hand-edited
 * generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITEGG_* tuning,
 * itMainMakeContainerItem, itMainUpdateAttackEvent) are referenced verbatim
 * and listed in the task report -- no values invented here.
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

/* decomp/BattleShip-main/include/reloc_data.us.h:3775. */
uintptr_t llITCommonDataEggItemAttributes = 0xACCu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3776. */
uintptr_t llITCommonDataEggAttackEvents = 0xB14u;

extern void *gITManagerCommonData;

/* The source's own itEggExplodeInitVars (itegg.c:359) reads the CAPSULE
 * attack-event row, with the source's own "Should this be
 * llITCommonDataEggAttackEvents?" remark kept verbatim below. That token is
 * owned by the sibling battleship_item_capsule.c TU
 * (reloc_data.us.h:3733), referenced here, never redefined or revalued. */
extern uintptr_t llITCommonDataCapsuleAttackEvents;

/* decomp itegg.h:8-26. The port publishes no per-kind item procs, so the
 * source header's declarations travel with this TU, exactly as the Tomato,
 * Bat and Harisen files carry theirs. */
sb32 itEggFallProcUpdate(GObj *item_gobj);
sb32 itEggWaitProcMap(GObj *item_gobj);
sb32 itEggCommonProcHit(GObj *item_gobj);
sb32 itEggFallProcMap(GObj *item_gobj);
void itEggWaitSetModelVars(GObj *item_gobj);
void itEggWaitSetStatus(GObj *item_gobj);
void itEggFallSetStatus(GObj *item_gobj);
void itEggHoldSetStatus(GObj *item_gobj);
sb32 itEggThrownProcUpdate(GObj *item_gobj);
sb32 itEggThrownProcMap(GObj *item_gobj);
void itEggThrownSetStatus(GObj *item_gobj);
sb32 func_ovl3_80181894(GObj *item_gobj);
sb32 itEggDroppedProcMap(GObj *item_gobj);
void itEggDroppedSetStatus(GObj *item_gobj);
sb32 itEggExplodeProcUpdate(GObj *item_gobj);
GObj *itEggMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
void itEggExplodeInitVars(GObj *item_gobj);
void itEggExplodeSetStatus(GObj *item_gobj);
void itEggExplodeMakeEffectGotoSetStatus(GObj *item_gobj);

/* No port header publishes these yet (cf. battleship_link_bomb.c:75-80 and
 * battleship_item_harisen.c:39-43, which declare their missing imports the
 * same way). itMainMakeContainerItem / itMainUpdateAttackEvent are the
 * port-missing helpers this container needs; the effect makers live in the
 * decomp efmanager.c this port imports whole into battleship_efmanager.c;
 * syUtilsRandIntRange follows the battleship_item_link_core.c:207
 * local-extern shape. */
extern sb32 itMainMakeContainerItem(GObj *parent_gobj);
extern void itMainUpdateAttackEvent(GObj *item_gobj, ITAttackEvent *ev);
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
extern GObj *efManagerQuakeMakeEffect(s32 id);
extern LBParticle *efManagerEggBreakMakeEffect(Vec3f *pos);
extern s32 syUtilsRandIntRange(s32 range);

/* decomp itegg.c:10-32 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITEggItemDesc =
{
    nITKindEgg,                             /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataEggItemAttributes,       /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyRSca,          /* Main matrix transformations */
        nGCMatrixKindNull,                   /* Secondary matrix transformations? */
        0                                    /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itEggFallProcUpdate,                    /* Proc Update */
    itEggFallProcMap,                       /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    itEggCommonProcHit                      /* Proc Damage */
};

/* decomp itegg.c:34-107 verbatim. */
ITStatusDesc dITEggStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itEggWaitProcMap,                   /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itEggCommonProcHit                  /* Proc Damage */
    },

    /* Status 1 (Air Fall Wait) */
    {
        itEggFallProcUpdate,                /* Proc Update */
        itEggFallProcMap,                   /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itEggCommonProcHit                  /* Proc Damage */
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
        itEggThrownProcUpdate,              /* Proc Update */
        itEggThrownProcMap,                 /* Proc Map */
        itEggCommonProcHit,                 /* Proc Hit */
        itEggCommonProcHit,                 /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itEggCommonProcHit,                 /* Proc Set-Off */
        itEggCommonProcHit,                 /* Proc Reflector */
        itEggCommonProcHit                  /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itEggFallProcUpdate,                /* Proc Update */
        itEggDroppedProcMap,                /* Proc Map */
        itEggCommonProcHit,                 /* Proc Hit */
        itEggCommonProcHit,                 /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itEggCommonProcHit,                 /* Proc Set-Off */
        itEggCommonProcHit,                 /* Proc Reflector */
        itEggCommonProcHit                  /* Proc Damage */
    },

    /* Status 5 (Neutral Explosion) */
    {
        itEggExplodeProcUpdate,             /* Proc Update */
        NULL,                               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itegg.c:115-124 verbatim. */
enum itEggStatus
{
    nITEggStatusWait,
    nITEggStatusFall,
    nITEggStatusHold,
    nITEggStatusThrown,
    nITEggStatusDropped,
    nITEggStatusExplode,
    nITEggStatusEnumCount
};

/* decomp itegg.c:133-144 verbatim. */
sb32 itEggFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITEGG_GRAVITY, ITEGG_TVEL);
    itVisualsUpdateSpin(item_gobj);

    dobj->child->rotate.vec.f.z = dobj->rotate.vec.f.z;

    return FALSE;
}

/* decomp itegg.c:147-152 verbatim. */
sb32 itEggWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itEggFallSetStatus);

    return FALSE;
}

/* decomp itegg.c:155-166 verbatim. */
sb32 itEggCommonProcHit(GObj *item_gobj)
{
    if (itMainMakeContainerItem(item_gobj) != FALSE)
    {
        efManagerEggBreakMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f);

        return TRUE;
    }
    else itEggExplodeMakeEffectGotoSetStatus(item_gobj);

    return FALSE;
}

/* decomp itegg.c:169-172 verbatim. */
sb32 itEggFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITEGG_MAP_REBOUND_COMMON, ITEGG_MAP_REBOUND_GROUND, itEggWaitSetStatus);
}

/* decomp itegg.c:175-182 verbatim. */
void itEggWaitSetModelVars(GObj *item_gobj)
{
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->scale.vec.f.x = dobj->scale.vec.f.y = dobj->scale.vec.f.z = 1.0F;

    dobj->child->rotate.vec.f.z = dobj->rotate.vec.f.z;
}

/* decomp itegg.c:185-190 verbatim. */
void itEggWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itEggWaitSetModelVars(item_gobj);
    itMainSetStatus(item_gobj, dITEggStatusDescs, nITEggStatusWait);
}

/* decomp itegg.c:193-206 verbatim. */
void itEggFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    ip->attack_coll.attack_state = nGMAttackStateOff;

    ip->is_damage_all = TRUE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITEggStatusDescs, nITEggStatusFall);
}

/* decomp itegg.c:209-212 verbatim. */
void itEggHoldSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITEggStatusDescs, nITEggStatusHold);
}

/* decomp itegg.c:215-226 verbatim. */
sb32 itEggThrownProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITEGG_GRAVITY, ITEGG_TVEL);
    itVisualsUpdateSpin(item_gobj);

    dobj->child->rotate.vec.f.z = dobj->rotate.vec.f.z;

    return FALSE;
}

/* decomp itegg.c:229-242 verbatim. */
sb32 itEggThrownProcMap(GObj *item_gobj)
{
    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_MAIN_MASK) != FALSE)
    {
        if (itMainMakeContainerItem(item_gobj) != FALSE)
        {
            efManagerEggBreakMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f);

            return TRUE;
        }
        else itEggExplodeMakeEffectGotoSetStatus(item_gobj);
    }
    return FALSE;
}

/* decomp itegg.c:245-254 verbatim. */
void itEggThrownSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_damage_all = TRUE;

    ip->damage_coll.hitstatus = nGMHitStatusNormal;

    itMainSetStatus(item_gobj, dITEggStatusDescs, nITEggStatusThrown);
}

/* decomp itegg.c:257-262 verbatim (unused in source). */
sb32 func_ovl3_80181894(GObj *item_gobj) // Unused
{
    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp itegg.c:265-268 verbatim. */
sb32 itEggDroppedProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITEGG_MAP_REBOUND_COMMON, ITEGG_MAP_REBOUND_GROUND, itEggWaitSetStatus);
}

/* decomp itegg.c:271-280 verbatim. */
void itEggDroppedSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_damage_all = TRUE;

    ip->damage_coll.hitstatus = nGMHitStatusNormal;

    itMainSetStatus(item_gobj, dITEggStatusDescs, nITEggStatusDropped);
}

/* decomp itegg.c:283-298 verbatim. */
sb32 itEggExplodeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi++;

    if (ip->multi == ITEGG_EXPLODE_EFFECT_WAIT)
    {
        efManagerEggBreakMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f);

        return TRUE;
    }
    itMainUpdateAttackEvent(item_gobj, itGetAttackEvent(dITEggItemDesc, &llITCommonDataEggAttackEvents));

    return FALSE;
}

/* decomp itegg.c:301-333 verbatim. */
GObj* itEggMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITEggItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        DObj *dobj = DObjGetStruct(item_gobj);
        ITStruct *egg_ip = itGetStruct(item_gobj);

        egg_ip->is_unused_item_bool = TRUE;

        egg_ip->arrow_gobj = ifCommonItemArrowMakeInterface(egg_ip);

        gcAddXObjForDObjFixed(dobj->child, 0x2E, 0);

        dobj->translate.vec.f = *pos;

        if (flags & ITEM_FLAG_PARENT_ITEM)
        {
            ITStruct *spawn_ip = itGetStruct(parent_gobj);

            if ((spawn_ip->kind == nITKindMLucky) && (syUtilsRandIntRange(2) == 0))
            {
                dobj->child->rotate.vec.f.y = F_CST_DTOR32(180.0F);

                egg_ip->physics.vel_air.x = -egg_ip->physics.vel_air.x;

                egg_ip->lr = -egg_ip->lr;
            }
        }
    }
    return item_gobj;
}

/* decomp itegg.c:336-360 verbatim. */
void itEggExplodeInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = 0;

    ip->event_id = 0;

    ip->attack_coll.fgm_id = nSYAudioFGMExplodeL;
    ip->attack_coll.throw_mul = ITEM_THROW_DEFAULT;

    func_800269C0_275C0(nSYAudioFGMExplodeL);

    ip->attack_coll.can_rehit_item = TRUE;
    ip->attack_coll.can_hop = FALSE;
    ip->attack_coll.can_reflect = FALSE;
    ip->attack_coll.can_setoff = FALSE;
    ip->attack_coll.element = nGMHitElementFire;

    ip->damage_coll.hitstatus = nGMHitStatusNone;

    itMainClearOwnerStats(item_gobj);
    itMainRefreshAttackColl(item_gobj);
    itMainUpdateAttackEvent(item_gobj, itGetAttackEvent(dITEggItemDesc, &llITCommonDataCapsuleAttackEvents)); // Should this be llITCommonDataEggAttackEvents?
}

/* decomp itegg.c:363-367 verbatim. */
void itEggExplodeSetStatus(GObj *item_gobj)
{
    itEggExplodeInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITEggStatusDescs, nITEggStatusExplode);
}

/* decomp itegg.c:370-393 verbatim. */
void itEggExplodeMakeEffectGotoSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    LBParticle *ep;

    ip->attack_coll.attack_state = nGMAttackStateOff;

    ip->physics.vel_air.x = 0.0F;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.z = 0.0F;

    ep = efManagerSparkleWhiteMultiExplodeMakeEffect(&dobj->translate.vec.f);

    if (ep != NULL)
    {
        ep->xf->scale.x = ep->xf->scale.y = ep->xf->scale.z = ITEGG_EXPLODE_EFFECT_SCALE;
    }
    efManagerQuakeMakeEffect(1);

    DObjGetStruct(item_gobj)->flags = DOBJ_FLAG_HIDDEN;

    itEggExplodeSetStatus(item_gobj);
}

#endif /* NDS_P2_ITEM_CORE */
