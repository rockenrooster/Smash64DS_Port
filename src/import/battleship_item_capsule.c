/* P2-5 Capsule (kind 2, nITKindCapsule). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itcapsule.c:10-345.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x50) and the attack-event row (0x98) live below
 * (decomp/BattleShip-main/include/reloc_data.us.h:3732, :3733); the port's
 * generated reloc header does not publish Capsule tokens, so this TU owns
 * its uintptr_t tokens the same way battleship_item_harisen.c:32-35 owns
 * Harisen's pair (local tokens, no generator involvement, no hand-edited
 * generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITCAPSULE_* tuning,
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

/* decomp/BattleShip-main/include/reloc_data.us.h:3732. */
uintptr_t llITCommonDataCapsuleItemAttributes = 0x50u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3733. */
uintptr_t llITCommonDataCapsuleAttackEvents = 0x98u;

extern void *gITManagerCommonData;

/* decomp itcapsule.h:8-25. The port publishes no per-kind item procs, so
 * the source header's declarations travel with this TU, exactly as the
 * Tomato, Bat and Harisen files carry theirs. */
sb32 itCapsuleFallProcUpdate(GObj *item_gobj);
sb32 itCapsuleWaitProcMap(GObj *item_gobj);
sb32 itCapsuleCommonProcHit(GObj *item_gobj);
sb32 itCapsuleFallProcMap(GObj *item_gobj);
void itCapsuleWaitSetStatus(GObj *item_gobj);
void itCapsuleFallSetStatus(GObj *item_gobj);
void itCapsuleHoldSetStatus(GObj *item_gobj);
sb32 itCapsuleThrownProcUpdate(GObj *item_gobj);
sb32 itCapsuleThrownProcMap(GObj *item_gobj);
void itCapsuleThrownSetStatus(GObj *item_gobj);
sb32 func_ovl3_801741F0(GObj *item_gobj);
sb32 itCapsuleDroppedProcMap(GObj *item_gobj);
void itCapsuleDroppedSetStatus(GObj *item_gobj);
sb32 itCapsuleExplodeProcUpdate(GObj *item_gobj);
GObj *itCapsuleMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
void itCapsuleExplodeInitVars(GObj *item_gobj);
void itCapsuleExplodeSetStatus(GObj *item_gobj);
void itCapsuleExplodeMakeEffectGotoSetStatus(GObj *item_gobj);

/* No port header publishes these yet (cf. battleship_link_bomb.c:75-80 and
 * battleship_item_harisen.c:39-43, which declare their missing imports the
 * same way). itMainMakeContainerItem / itMainUpdateAttackEvent are the
 * port-missing helpers this container needs; the effect makers live in the
 * decomp efmanager.c this port imports whole into battleship_efmanager.c. */
extern sb32 itMainMakeContainerItem(GObj *parent_gobj);
extern void itMainUpdateAttackEvent(GObj *item_gobj, ITAttackEvent *ev);
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
extern GObj *efManagerQuakeMakeEffect(s32 id);

/* decomp itcapsule.c:10-32 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITCapsuleItemDesc =
{
    nITKindCapsule,                         /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataCapsuleItemAttributes,   /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyR,            /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itCapsuleFallProcUpdate,                /* Proc Update */
    itCapsuleFallProcMap,                   /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    itCapsuleCommonProcHit                  /* Proc Damage */
};

/* decomp itcapsule.c:34-107 verbatim. */
ITStatusDesc dITCapsuleStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itCapsuleWaitProcMap,               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itCapsuleCommonProcHit              /* Proc Damage */
    },

    /* Status 1 (Air Fall Wait) */
    {
        itCapsuleFallProcUpdate,            /* Proc Update */
        itCapsuleFallProcMap,               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itCapsuleCommonProcHit              /* Proc Damage */
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
        itCapsuleThrownProcUpdate,          /* Proc Update */
        itCapsuleThrownProcMap,             /* Proc Map */
        itCapsuleCommonProcHit,             /* Proc Hit */
        itCapsuleCommonProcHit,             /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itCapsuleCommonProcHit,             /* Proc Set-Off */
        itCapsuleCommonProcHit,             /* Proc Reflector */
        itCapsuleCommonProcHit              /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itCapsuleFallProcUpdate,            /* Proc Update */
        itCapsuleDroppedProcMap,            /* Proc Map */
        itCapsuleCommonProcHit,             /* Proc Hit */
        itCapsuleCommonProcHit,             /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itCapsuleCommonProcHit,             /* Proc Set-Off */
        itCapsuleCommonProcHit,             /* Proc Reflector */
        itCapsuleCommonProcHit              /* Proc Damage */
    },

    /* Status 5 (Fighter Hold) */
    {
        itCapsuleExplodeProcUpdate,         /* Proc Update */
        NULL,                               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    }
};

/* decomp itcapsule.c:115-124 verbatim. */
enum itCapsuleStatus
{
    nITCapsuleStatusWait,
    nITCapsuleStatusFall,
    nITCapsuleStatusHold,
    nITCapsuleStatusThrown,
    nITCapsuleStatusDropped,
    nITCapsuleStatusExplode,
    nITCapsuleStatusEnumCount
};

/* decomp itcapsule.c:133-141 verbatim. */
sb32 itCapsuleFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITCAPSULE_GRAVITY, ITCAPSULE_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itcapsule.c:144-149 verbatim. */
sb32 itCapsuleWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itCapsuleFallSetStatus);

    return FALSE;
}

/* decomp itcapsule.c:152-161 verbatim. */
sb32 itCapsuleCommonProcHit(GObj *item_gobj)
{
    if (itMainMakeContainerItem(item_gobj) != FALSE)
    {
        return TRUE;
    }
    else itCapsuleExplodeMakeEffectGotoSetStatus(item_gobj);

    return FALSE;
}

/* decomp itcapsule.c:164-167 verbatim. */
sb32 itCapsuleFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITCAPSULE_MAP_REBOUND_COMMON, ITCAPSULE_MAP_REBOUND_GROUND, itCapsuleWaitSetStatus);
}

/* decomp itcapsule.c:170-174 verbatim. */
void itCapsuleWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITCapsuleStatusDescs, nITCapsuleStatusWait);
}

/* decomp itcapsule.c:177-190 verbatim. */
void itCapsuleFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);

    ip->is_damage_all = TRUE;

    ip->damage_coll.hitstatus = nGMHitStatusNormal;

    itMainSetStatus(item_gobj, dITCapsuleStatusDescs, nITCapsuleStatusFall);
}

/* decomp itcapsule.c:193-196 verbatim. */
void itCapsuleHoldSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITCapsuleStatusDescs, nITCapsuleStatusHold);
}

/* decomp itcapsule.c:199-207 verbatim. */
sb32 itCapsuleThrownProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITCAPSULE_GRAVITY, ITCAPSULE_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp itcapsule.c:210-221 verbatim. */
sb32 itCapsuleThrownProcMap(GObj *item_gobj)
{
    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_MAIN_MASK) != FALSE)
    {
        if (itMainMakeContainerItem(item_gobj) != FALSE)
        {
            return TRUE;
        }
        else itCapsuleExplodeMakeEffectGotoSetStatus(item_gobj);
    }
    return FALSE;
}

/* decomp itcapsule.c:224-233 verbatim. */
void itCapsuleThrownSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_damage_all = TRUE;

    ip->damage_coll.hitstatus = nGMHitStatusNormal;

    itMainSetStatus(item_gobj, dITCapsuleStatusDescs, nITCapsuleStatusThrown);
}

/* decomp itcapsule.c:236-241 verbatim (unused in source). */
sb32 func_ovl3_801741F0(GObj *item_gobj) // Unused
{
    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp itcapsule.c:244-247 verbatim. */
sb32 itCapsuleDroppedProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITCAPSULE_MAP_REBOUND_COMMON, ITCAPSULE_MAP_REBOUND_GROUND, itCapsuleWaitSetStatus);
}

/* decomp itcapsule.c:250-253 verbatim. */
void itCapsuleDroppedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITCapsuleStatusDescs, nITCapsuleStatusDropped);
}

/* decomp itcapsule.c:256-269 verbatim. */
sb32 itCapsuleExplodeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi++;

    if (ip->multi == ITCAPSULE_EXPLODE_FRAME_END)
    {
        return TRUE;
    }
    itMainUpdateAttackEvent(item_gobj, itGetAttackEvent(dITCapsuleItemDesc, &llITCommonDataCapsuleAttackEvents)); // (ITAttackEvent*) ((uintptr_t)*dITCapsuleItemDesc.p_file + (intptr_t)&D_NF_00000098); Linker thing

    return FALSE;
}

/* decomp itcapsule.c:271-284 verbatim. */
GObj* itCapsuleMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITCapsuleItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

/* decomp itcapsule.c:287-312 verbatim. */
void itCapsuleExplodeInitVars(GObj *item_gobj)
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

    ip->attack_coll.element = nGMHitElementFire;

    ip->attack_coll.can_setoff = FALSE;

    ip->damage_coll.hitstatus = nGMHitStatusNone;

    itMainClearOwnerStats(item_gobj);
    itMainRefreshAttackColl(item_gobj);

    itMainUpdateAttackEvent(item_gobj, itGetAttackEvent(dITCapsuleItemDesc, &llITCommonDataCapsuleAttackEvents));
}

/* decomp itcapsule.c:315-319 verbatim. */
void itCapsuleExplodeSetStatus(GObj *item_gobj)
{
    itCapsuleExplodeInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITCapsuleStatusDescs, nITCapsuleStatusExplode);
}

/* decomp itcapsule.c:322-345 verbatim. */
void itCapsuleExplodeMakeEffectGotoSetStatus(GObj *item_gobj)
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
        ep->xf->scale.x = ep->xf->scale.y = ep->xf->scale.z = ITCAPSULE_EXPLODE_SCALE;
    }
    efManagerQuakeMakeEffect(1);

    DObjGetStruct(item_gobj)->flags = DOBJ_FLAG_HIDDEN;

    itCapsuleExplodeSetStatus(item_gobj);
}

#endif /* NDS_P2_ITEM_CORE */
