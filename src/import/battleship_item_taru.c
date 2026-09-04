/* P2-5 Barrel (kind 1, nITKindTaru). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/ittaru.c:10-464.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x634) and the attack-event row (0x67C) live below
 * (decomp/BattleShip-main/include/reloc_data.us.h:3757, :3758); the port's
 * generated reloc header does not publish Taru tokens, so this TU owns its
 * uintptr_t tokens the same way battleship_item_harisen.c:32-35 owns
 * Harisen's pair (local tokens, no generator involvement, no hand-edited
 * generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITTARU_* tuning, the
 * item_vars.taru union member, itMainMakeContainerItem,
 * itMainUpdateAttackEvent) are referenced verbatim and listed in the task
 * report -- no values invented here.
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
#include <math.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3757. */
uintptr_t llITCommonDataTaruItemAttributes = 0x634u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3758. */
uintptr_t llITCommonDataTaruAttackEvents = 0x67Cu;

extern void *gITManagerCommonData;

/* decomp ittaru.h:8-29. The port publishes no per-kind item procs, so the
 * source header's declarations travel with this TU, exactly as the Tomato,
 * Bat and Harisen files carry theirs. itBoxContainerSmashMakeEffect is
 * owned by the sibling battleship_item_box.c TU (decomp itbox.h:9). */
sb32 itTaruFallProcUpdate(GObj *item_gobj);
sb32 itTaruWaitProcMap(GObj *item_gobj);
sb32 itTaruCommonProcHit(GObj *item_gobj);
sb32 itTaruCommonProcDamage(GObj *item_gobj);
sb32 itTaruFallProcMap(GObj *item_gobj);
void itTaruWaitSetStatus(GObj *item_gobj);
void itTaruFallSetStatus(GObj *item_gobj);
void itTaruHoldSetStatus(GObj *item_gobj);
sb32 itTaruThrownCheckMapCollision(GObj *item_gobj, f32 common_rebound);
void itTaruRollSetStatus(GObj *item_gobj);
sb32 itTaruThrownProcMap(GObj *item_gobj);
void itTaruThrownInitVars(GObj *item_gobj);
void itTaruThrownSetStatus(GObj *item_gobj);
sb32 func_ovl3_80179F50(GObj *item_gobj);
void itTaruDroppedSetStatus(GObj *item_gobj);
sb32 itTaruExplodeProcUpdate(GObj *item_gobj);
sb32 itTaruRollProcUpdate(GObj *item_gobj);
sb32 itTaruRollProcMap(GObj *item_gobj);
GObj *itTaruMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
void itTaruExplodeInitVars(GObj *item_gobj);
void itTaruExplodeSetStatus(GObj *item_gobj);
void itTaruExplodeMakeEffectGotoSetStatus(GObj *item_gobj);
void itBoxContainerSmashMakeEffect(Vec3f *pos);

/* No port header publishes these yet (cf. battleship_link_bomb.c:75-80 and
 * battleship_item_harisen.c:39-43, which declare their missing imports the
 * same way). itMainMakeContainerItem / itMainUpdateAttackEvent are the
 * port-missing helpers this container needs; the effect makers live in the
 * decomp efmanager.c this port imports whole into battleship_efmanager.c;
 * lbCommonReflect2D follows the battleship_item_link_core.c:73-77
 * local-extern shape; syUtils* follow its :207 shape. */
extern sb32 itMainMakeContainerItem(GObj *parent_gobj);
extern void itMainUpdateAttackEvent(GObj *item_gobj, ITAttackEvent *ev);
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
extern GObj *efManagerQuakeMakeEffect(s32 id);
extern Vec3f *lbCommonReflect2D(Vec3f *dst, Vec3f *p);
extern s32 syUtilsRandIntRange(s32 range);
extern f32 syUtilsArcTan2(f32 y, f32 x);

/* decomp ittaru.c:10-32 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITTaruItemDesc =
{
    nITKindTaru,                            /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataTaruItemAttributes,      /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyR,            /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itTaruFallProcUpdate,                   /* Proc Update */
    itTaruFallProcMap,                      /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp ittaru.c:34-119 verbatim. */
ITStatusDesc dITTaruStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itTaruWaitProcMap,                  /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itTaruCommonProcDamage              /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itTaruFallProcUpdate,               /* Proc Update */
        itTaruFallProcMap,                  /* Proc Map */
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
        itTaruFallProcUpdate,               /* Proc Update */
        itTaruThrownProcMap,                /* Proc Map */
        itTaruCommonProcHit,                /* Proc Hit */
        itTaruCommonProcHit,                /* Proc Shield */
        NULL,                               /* Proc Hop */
        itTaruCommonProcHit,                /* Proc Set-Off */
        itTaruCommonProcHit,                /* Proc Reflector */
        itTaruCommonProcDamage              /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itTaruFallProcUpdate,               /* Proc Update */
        itTaruThrownProcMap,                /* Proc Map */
        itTaruCommonProcHit,                /* Proc Hit */
        itTaruCommonProcHit,                /* Proc Shield */
        NULL,                               /* Proc Hop */
        itTaruCommonProcHit,                /* Proc Set-Off */
        itTaruCommonProcHit,                /* Proc Reflector */
        itTaruCommonProcDamage              /* Proc Damage */
    },

    /* Status 5 (Neutral Explosion) */
    {
        itTaruExplodeProcUpdate,            /* Proc Update */
        NULL,                               /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        NULL                                /* Proc Damage */
    },

    /* Status 6 (Ground Roll) */
    {
        itTaruRollProcUpdate,               /* Proc Update */
        itTaruRollProcMap,                  /* Proc Map */
        itTaruCommonProcHit,                /* Proc Hit */
        itTaruCommonProcHit,                /* Proc Shield */
        NULL,                               /* Proc Hop */
        itTaruCommonProcHit,                /* Proc Set-Off */
        itTaruCommonProcHit,                /* Proc Reflector */
        itTaruCommonProcDamage              /* Proc Damage */
    }
};

/* decomp ittaru.c:127-137 verbatim. */
enum itTaruStatus
{
    nITTaruStatusWait,
    nITTaruStatusFall,
    nITTaruStatusHold,
    nITTaruStatusThrown,
    nITTaruStatusDropped,
    nITTaruStatusExplode,
    nITTaruStatusRoll,
    nITTaruStatusEnumCount
};

/* decomp ittaru.c:146-157 verbatim. */
sb32 itTaruFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITTARU_GRAVITY, ITTARU_TVEL);

    DObjGetStruct(item_gobj)->rotate.vec.f.z += ip->item_vars.taru.roll_rotate_step;

    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

/* decomp ittaru.c:160-165 verbatim. */
sb32 itTaruWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itTaruFallSetStatus);

    return FALSE;
}

/* decomp ittaru.c:168-181 verbatim. */
sb32 itTaruCommonProcHit(GObj *item_gobj)
{
    func_800269C0_275C0(nSYAudioFGMContainerSmash);

    itBoxContainerSmashMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f);

    if (itMainMakeContainerItem(item_gobj) != FALSE)
    {
        return TRUE;
    }
    else itTaruExplodeMakeEffectGotoSetStatus(item_gobj);

    return FALSE;
}

/* decomp ittaru.c:184-193 verbatim. */
sb32 itTaruCommonProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->percent_damage >= ITTARU_HEALTH_MAX)
    {
        return itTaruCommonProcHit(item_gobj);
    }
    else return FALSE;
}

/* decomp ittaru.c:196-199 verbatim. */
sb32 itTaruFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITTARU_MAP_REBOUND_COMMON, ITTARU_MAP_REBOUND_GROUND, itTaruWaitSetStatus);
}

/* decomp ittaru.c:202-206 verbatim. */
void itTaruWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITTaruStatusDescs, nITTaruStatusWait);
}

/* decomp ittaru.c:209-217 verbatim. */
void itTaruFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITTaruStatusDescs, nITTaruStatusFall);
}

/* decomp ittaru.c:220-223 verbatim. */
void itTaruHoldSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITTaruStatusDescs, nITTaruStatusHold);
}

/* decomp ittaru.c:226-241 verbatim. */
sb32 itTaruThrownCheckMapCollision(GObj *item_gobj, f32 common_rebound)
{
    s32 unused;
    ITStruct *ip;
    sb32 is_collide_floor = itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR);

    if (itMapCheckCollideAllRebound(item_gobj, (MAP_FLAG_CEIL | MAP_FLAG_RWALL | MAP_FLAG_LWALL), common_rebound, NULL) != FALSE)
    {
        itMainSetSpinVelLR(item_gobj);
    }
    if (is_collide_floor != FALSE)
    {
        return TRUE;
    }
    else return FALSE;
}

/* decomp ittaru.c:244-253 verbatim. */
void itTaruRollSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->lifetime = ITTARU_LIFETIME;

    ip->physics.vel_air.y = 0.0F;

    itMainSetStatus(item_gobj, dITTaruStatusDescs, nITTaruStatusRoll);
}

/* decomp ittaru.c:256-283 verbatim. */
sb32 itTaruThrownProcMap(GObj *item_gobj)
{
    if (itTaruThrownCheckMapCollision(item_gobj, 0.5F) != FALSE)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        if (ip->physics.vel_air.y >= 90.0F)       // Is it even possible to meet this condition? Didn't they mean ABSF(ip->physics.vel_air.y)?
        {
            itTaruCommonProcHit(item_gobj);       // This causes the barrel to smash on impact when landing from too high; doesn't seem possible to trigger

            return TRUE;
        }
        else if (ip->physics.vel_air.y < 30.0F)
        {
            itTaruRollSetStatus(item_gobj);
        }
        else
        {
            lbCommonReflect2D(&ip->physics.vel_air, &ip->coll_data.floor_angle);

            ip->physics.vel_air.y *= 0.2F;

            itMainSetSpinVelLR(item_gobj);
        }
        itMainClearOwnerStats(item_gobj);
    }
    return FALSE;
}

/* decomp ittaru.c:286-294 verbatim. */
void itTaruThrownInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    DObjGetStruct(item_gobj)->child->rotate.vec.f.x = F_CST_DTOR32(90.0F);

    ip->coll_data.map_coll.top = ip->coll_data.map_coll.width;
    ip->coll_data.map_coll.bottom = -ip->coll_data.map_coll.width;
}

/* decomp ittaru.c:297-301 verbatim. */
void itTaruThrownSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITTaruStatusDescs, nITTaruStatusThrown);
    itTaruThrownInitVars(item_gobj);
}

/* decomp ittaru.c:304-309 verbatim (unused in source). */
sb32 func_ovl3_80179F50(GObj *item_gobj) // Unused
{
    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp ittaru.c:312-316 verbatim. */
void itTaruDroppedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITTaruStatusDescs, nITTaruStatusDropped);
    itTaruThrownInitVars(item_gobj);
}

/* decomp ittaru.c:319-332 verbatim. */
sb32 itTaruExplodeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi++;

    if (ip->multi == ITTARU_EXPLODE_LIFETIME)
    {
        return TRUE;
    }
    else itMainUpdateAttackEvent(item_gobj, itGetAttackEvent(dITTaruItemDesc, &llITCommonDataTaruAttackEvents));

    return FALSE;
}

/* decomp ittaru.c:335-370 verbatim. */
sb32 itTaruRollProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    f32 roll_rotate_step;
    f32 sqrt_vel;

    ip->physics.vel_air.x += (-(syUtilsArcTan2(ip->coll_data.floor_angle.y, ip->coll_data.floor_angle.x) - F_CLC_DTOR32(90.0F)) * ITTARU_MUL_VEL_X);

    ip->lr = (ip->physics.vel_air.x >= 0.0F) ? +1 : -1;

    sqrt_vel = sqrtf(SQUARE(ip->physics.vel_air.x) + SQUARE(ip->physics.vel_air.y));

    if (sqrt_vel < ITTARU_VEL_MIN)
    {
        ip->lifetime--;

        if (ip->lifetime < ITTARU_DESPAWN_FLASH_START)
        {
            if (ip->lifetime == 0)
            {
                return TRUE;
            }
            else if ((ip->lifetime % 2) != 0)
            {
                DObjGetStruct(item_gobj)->flags ^= DOBJ_FLAG_HIDDEN;
            }
        }
    }
    roll_rotate_step = ((ip->lr == -1) ? ITTARU_ROLL_ROTATE_MUL : -ITTARU_ROLL_ROTATE_MUL) * sqrt_vel;

    ip->item_vars.taru.roll_rotate_step = roll_rotate_step;

    DObjGetStruct(item_gobj)->rotate.vec.f.z += roll_rotate_step;

    return FALSE;
}

/* decomp ittaru.c:373-386 verbatim. */
sb32 itTaruRollProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestLRWallCheckFloor(item_gobj) == FALSE)
    {
        itMainSetStatus(item_gobj, dITTaruStatusDescs, nITTaruStatusDropped);
    }
    else if (ip->coll_data.mask_curr & (MAP_FLAG_RWALL | MAP_FLAG_LWALL))
    {
        return itTaruCommonProcHit(item_gobj);
    }
    return FALSE;
}

/* decomp ittaru.c:389-406 verbatim. */
GObj* itTaruMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITTaruItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        ip->item_vars.taru.roll_rotate_step = 0.0F;

        ip->is_damage_all = TRUE;

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

/* decomp ittaru.c:409-431 verbatim. */
void itTaruExplodeInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = 0;
    ip->event_id = 0;

    ip->attack_coll.fgm_id = nSYAudioFGMExplodeL;

    ip->attack_coll.can_rehit_item = TRUE;
    ip->attack_coll.can_reflect = FALSE;

    ip->attack_coll.throw_mul = ITEM_THROW_DEFAULT;
    ip->attack_coll.element = nGMHitElementFire;

    ip->attack_coll.can_setoff = FALSE;

    ip->damage_coll.hitstatus = nGMHitStatusNone;

    itMainClearOwnerStats(item_gobj);
    itMainRefreshAttackColl(item_gobj);
    itMainUpdateAttackEvent(item_gobj, itGetAttackEvent(dITTaruItemDesc, &llITCommonDataTaruAttackEvents));
}

/* decomp ittaru.c:434-438 verbatim. */
void itTaruExplodeSetStatus(GObj *item_gobj)
{
    itTaruExplodeInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITTaruStatusDescs, nITTaruStatusExplode);
}

/* decomp ittaru.c:441-464 verbatim. */
void itTaruExplodeMakeEffectGotoSetStatus(GObj *item_gobj)
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
        pc->xf->scale.x = pc->xf->scale.y = pc->xf->scale.z = ITTARU_EXPLODE_SCALE;
    }
    efManagerQuakeMakeEffect(1);

    DObjGetStruct(item_gobj)->flags = DOBJ_FLAG_HIDDEN;

    itTaruExplodeSetStatus(item_gobj);
}

#endif /* NDS_P2_ITEM_CORE */
