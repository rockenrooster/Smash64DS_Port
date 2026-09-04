/* P2-5 RShell (Red Shell, nITKindRShell). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itrshell.c:10-763.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x584), the spin anim bank base (0x5F88), the spin anim joint (0x6018)
 * and the spin matanim joint (0x6048) are
 * decomp/BattleShip-main/include/reloc_data.us.h:3754 and :3791-:3793;
 * the port's generated reloc header publishes none of the RShell tokens,
 * so this TU owns all four uintptr_t tokens the same way
 * battleship_item_harisen.c:31-34 owns Harisen's (local tokens, no
 * generator involvement, no hand-edited generated file). The spin anim
 * bank is shared with the Green Shell (same 0x5F88/0x6018/0x6048 rows);
 * each shell TU owns its own token variables, so no TU shares or
 * redefines another's.
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

/* decomp/BattleShip-main/include/reloc_data.us.h:3754. */
uintptr_t llITCommonDataRShellItemAttributes = 0x584u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3791. */
/* The three Shell tokens are SHARED with the green shell and defined there
 * (battleship_item_gshell.c): one data block serves both, exactly as
 * reloc_data.us.h names them once. Defining them here too is a duplicate
 * symbol at link. */
extern uintptr_t llITCommonDataShellDataStart;
/* decomp/BattleShip-main/include/reloc_data.us.h:3792. */
extern uintptr_t llITCommonDataShellAnimJoint;
/* decomp/BattleShip-main/include/reloc_data.us.h:3793. */
extern uintptr_t llITCommonDataShellMatAnimJoint;

extern void *gITManagerCommonData;

/* decomp sys/objanim.h:16, :19 and :52. No port header in this TU's chain
 * publishes them; battleship_item_harisen.c:38-42 carries the same kind of
 * local externs. */
extern void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                                f32 anim_frame);
extern void gcAddMObjMatAnimJoint(MObj *mobj, AObjEvent32 *matanim_joint,
                                   f32 anim_frame);
extern void gcPlayAnimAll(GObj *gobj);

/* decomp sys/utils.h:20. No port header in this TU's chain publishes it;
 * battleship_item_link_core.c:205-207 carries the same kind of local
 * extern for syUtilsRandIntRange. */
extern s32 syUtilsRandIntRange(s32 range);

/* decomp sys/vector.h:33. No port header in this TU's chain publishes it. */
extern Vec3f *syVectorDiff3D(Vec3f *dst, Vec3f *src, Vec3f *sub);

/* decomp it/itmain.h:34 (defined itmain.c:252-265). No port header declares
 * it and no port TU defines it yet; the ELF has no such symbol. Carried as
 * a true extern (same signature) so this TU references the seam correctly. */
extern void itMainCopyDamageStats(GObj *item_gobj);

/* decomp ef/efmanager.h:38. Same shape as the decomp prototype. */
extern LBParticle *efManagerDustLightMakeEffect(Vec3f *pos, s32 lr,
                                                 f32 scale);
/* Same shape as battleship_link_bomb.c:80. */
extern void func_800269C0_275C0(u16 sfx_id);

/* decomp itrshell.h:8-36 verbatim. The port header does not publish per-kind
 * item procs yet, so the source header's declarations travel with this TU. */
extern void itRShellSpinUpdateFollowPlayer(GObj *item_gobj, GObj *fighter_gobj);
extern void itRShellSpinSearchFollowPlayer(GObj *item_gobj);
extern void itRShellSpinUpdateGFX(GObj *item_gobj);
extern void itRShellSpinAddAnim(GObj *item_gobj);
extern void itRShellCommonClearAnim(GObj *item_gobj);
extern sb32 itRShellFallProcUpdate(GObj *item_gobj);
extern sb32 itRShellWaitProcMap(GObj *item_gobj);
extern sb32 itRShellFallProcMap(GObj *item_gobj);
extern void itRShellCommonSetStatusWaitOrSpin(GObj *item_gobj);
extern void itRShellCommonProcStatusWaitOrSpin(GObj *item_gobj);
extern void itRShellFallSetStatus(GObj *item_gobj);
extern sb32 itRShellCommonProcDamage(GObj *item_gobj);
extern void itRShellHoldSetStatus(GObj *item_gobj);
extern void itRShellThrownSetStatus(GObj *item_gobj);
extern void itRShellDroppedSetStatus(GObj *item_gobj);
extern sb32 itRShellThrownProcMap(GObj *item_gobj);
extern void itRShellSpinEdgeInvertVelLR(GObj *item_gobj, ub8 lr);
extern void itRShellSpinCheckCollisionEdge(GObj *item_gobj);
extern sb32 itRShellSpinProcUpdate(GObj *item_gobj);
extern sb32 itRShellSpinProcMap(GObj *item_gobj);
extern sb32 itRShellCommonProcHit(GObj *item_gobj);
extern sb32 itRShellSpinProcDamage(GObj *item_gobj);
extern void itRShellSpinInitVars(GObj *item_gobj);
extern void itRShellSpinSetStatus(GObj *item_gobj);
extern void itRShellSpinAirInitVars(GObj *item_gobj);
extern void itRShellSpinAirSetStatus(GObj *item_gobj);
extern GObj* itRShellMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
extern sb32 itRShellCommonProcShield(GObj *item_gobj);
extern sb32 itRShellCommonProcReflector(GObj *item_gobj);

/* decomp itrshell.c:10-32 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITRShellItemDesc =
{
    nITKindRShell,                          /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataRShellItemAttributes,    /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindNull,                  /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itRShellFallProcUpdate,                 /* Proc Update */
    itRShellFallProcMap,                    /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    itRShellCommonProcDamage                /* Proc Damage */
};

/* decomp itrshell.c:34-119 verbatim. */
ITStatusDesc dITRShellStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itRShellWaitProcMap,                /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itRShellCommonProcDamage            /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itRShellFallProcUpdate,             /* Proc Update */
        itRShellFallProcMap,                /* Proc Map */
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
        itRShellFallProcUpdate,             /* Proc Update */
        itRShellThrownProcMap,              /* Proc Map */
        itRShellCommonProcHit,              /* Proc Hit */
        itRShellCommonProcShield,           /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itRShellCommonProcShield,           /* Proc Set-Off */
        itRShellCommonProcReflector,        /* Proc Reflector */
        itRShellCommonProcDamage            /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itRShellFallProcUpdate,             /* Proc Update */
        itRShellThrownProcMap,              /* Proc Map */
        itRShellCommonProcHit,              /* Proc Hit */
        itRShellCommonProcShield,           /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itRShellCommonProcShield,           /* Proc Set-Off */
        itRShellCommonProcReflector,        /* Proc Reflector */
        itRShellCommonProcDamage            /* Proc Damage */
    },

    /* Status 5 (Ground Spin) */
    {
        itRShellSpinProcUpdate,             /* Proc Update */
        itRShellSpinProcMap,                /* Proc Map */
        itRShellCommonProcHit,              /* Proc Hit */
        itRShellCommonProcHit,              /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        itRShellCommonProcReflector,        /* Proc Reflector */
        itRShellSpinProcDamage              /* Proc Damage */
    },

    /* Status 6 (Ground Spin) */
    {
        itRShellFallProcUpdate,             /* Proc Update */
        itRShellThrownProcMap,              /* Proc Map */
        itRShellCommonProcHit,              /* Proc Hit */
        itRShellCommonProcHit,              /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        itRShellCommonProcReflector,        /* Proc Reflector */
        itRShellCommonProcDamage            /* Proc Damage */
    }
};

/* decomp itrshell.c:127-137 verbatim. */
enum itRShellStatus
{
    nITRShellStatusWait,
    nITRShellStatusFall,
    nITRShellStatusHold,
    nITRShellStatusThrown,
    nITRShellStatusDropped,
    nITRShellStatusSpin,
    nITRShellStatusSpinAir,
    nITRShellStatusEnumCount
};

/* decomp itrshell.c:146-188 verbatim. */
/* 0x8017A3A0 */
void itRShellSpinUpdateFollowPlayer(GObj *item_gobj, GObj *fighter_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    f32 vel_x;
    f32 dist_x;
    s32 lr_vel;
    s32 lr_dist;

    if (ip->ga == nMPKineticsGround)
    {
        dist_x = (DObjGetStruct(fighter_gobj)->translate.vec.f.x - DObjGetStruct(item_gobj)->translate.vec.f.x);

        lr_dist = (dist_x < 0.0F) ? -1 : +1;

        vel_x = lr_dist * ITRSHELL_MUL_VEL_X;

        ip->item_vars.shell.vel_x = vel_x;

        ip->physics.vel_air.x += vel_x;

        lr_vel = (ip->physics.vel_air.x < 0.0F) ? -1 : +1;

        lr_dist = (ip->item_vars.shell.vel_x < 0.0F) ? -1 : +1;

        if (lr_dist == lr_vel)
        {
            if (ABSF(ip->physics.vel_air.x) > ITRSHELL_CLAMP_VEL_X)
            {
                ip->physics.vel_air.x = ip->lr * ITRSHELL_CLAMP_VEL_X;
            }
        }
        if (ip->attack_coll.attack_state == nGMAttackStateOff)
        {
            if (ABSF(ip->physics.vel_air.x) <= ITRSHELL_HIT_INITVEL_X)
            {
                ip->attack_coll.attack_state = nGMAttackStateNew;

                itProcessUpdateAttackPositions(item_gobj);
            }
        }
        ip->lr = (ip->physics.vel_air.x < 0.0F) ? -1 : +1;
    }
}

/* decomp itrshell.c:191-224 verbatim. */
/* 0x8017A534 */
void itRShellSpinSearchFollowPlayer(GObj *item_gobj)
{
    s32 unused;
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    GObj *nearest_gobj;
    DObj *dobj = DObjGetStruct(item_gobj);
    Vec3f *translate = &dobj->translate.vec.f;
    s32 ft_count = 0;
    f32 next_dist;
    f32 nearest_dist;
    Vec3f dist;

    while (fighter_gobj != NULL)
    {
        syVectorDiff3D(&dist, &DObjGetStruct(fighter_gobj)->translate.vec.f, translate);

        if (ft_count == 0)
        {
            nearest_dist = SQUARE(dist.x) + SQUARE(dist.y);
        }
        next_dist = SQUARE(dist.x) + SQUARE(dist.y);

        if (nearest_dist >= next_dist)
        {
            nearest_dist = next_dist;

            nearest_gobj = fighter_gobj;
        }
        fighter_gobj = fighter_gobj->link_next;

        ft_count++;
    }
    itRShellSpinUpdateFollowPlayer(item_gobj, nearest_gobj);
}

/* decomp itrshell.c:227-243 verbatim. */
/* 0x8017A610 */
void itRShellSpinUpdateGFX(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (ip->item_vars.shell.dust_effect_int == 0)
    {
        Vec3f pos = dobj->translate.vec.f;

        pos.y += ip->attr->map_coll_bottom;

        efManagerDustLightMakeEffect(&pos, ip->lr, 1.0F);

        ip->item_vars.shell.dust_effect_int = ITRSHELL_EFFECT_SPAWN_INT;
    }
    ip->item_vars.shell.dust_effect_int--;
}

/* decomp itrshell.c:246-255 verbatim. */
/* 0x8017A6A0 */
void itRShellSpinAddAnim(GObj *item_gobj) /* Identical to Green Shell function */
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    s32 unused[2];

    gcAddDObjAnimJoint(dobj, itGetPData(ip, &llITCommonDataShellDataStart, &llITCommonDataShellAnimJoint), 0.0F);
    gcAddMObjMatAnimJoint(dobj->mobj, itGetPData(ip, &llITCommonDataShellDataStart, &llITCommonDataShellMatAnimJoint), 0.0F);
    gcPlayAnimAll(item_gobj);
}

/* decomp itrshell.c:258-262 verbatim. */
/* 0x8017A734 */
void itRShellCommonClearAnim(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->mobj->matanim_joint.event32 = NULL;
    DObjGetStruct(item_gobj)->anim_joint.event32 = NULL;
}

/* decomp itrshell.c:265-282 verbatim. */
/* 0x8017A74C */
sb32 itRShellFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITRSHELL_GRAVITY, ITRSHELL_TVEL);

    if (!(ip->item_vars.shell.damage_all_delay))
    {
        itMainClearOwnerStats(item_gobj);

        ip->item_vars.shell.damage_all_delay = -1;
    }
    if (ip->item_vars.shell.damage_all_delay != -1)
    {
        ip->item_vars.shell.damage_all_delay--;
    }
    return FALSE;
}

/* decomp itrshell.c:285-290 verbatim. */
/* 0x8017A7C4 */
sb32 itRShellWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itRShellFallSetStatus);

    return FALSE;
}

/* decomp itrshell.c:293-304 verbatim. */
/* 0x8017A7EC */
sb32 itRShellFallProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->item_vars.shell.health == 0)
    {
        return itMapCheckDestroyLanding(item_gobj, ITRSHELL_MAP_REBOUND_COMMON);
    }
    itMapCheckDestroyDropped(item_gobj, ITRSHELL_MAP_REBOUND_COMMON, ITRSHELL_MAP_REBOUND_GROUND, itRShellCommonProcStatusWaitOrSpin);

    return FALSE;
}

/* decomp itrshell.c:307-349 verbatim. */
/* 0x8017A83C */
void itRShellCommonSetStatusWaitOrSpin(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMapSetGround(ip);

    if (ABSF(ip->physics.vel_air.x) < ITRSHELL_STOP_VEL_X)
    {
        itMainSetGroundAllowPickup(item_gobj);

        ip->item_vars.shell.is_damage = FALSE;
        ip->physics.vel_air.x = 0.0F;

        itMainClearOwnerStats(item_gobj);

        ip->damage_coll.hitstatus = nGMHitStatusNormal;
        ip->attack_coll.attack_state = nGMAttackStateOff;

        itRShellCommonClearAnim(item_gobj);
        itMainSetStatus(item_gobj, dITRShellStatusDescs, nITRShellStatusWait);
    }
    else if (ip->item_vars.shell.is_damage != FALSE)
    {
        ip->attack_coll.attack_state = nGMAttackStateNew;

        itProcessUpdateAttackPositions(item_gobj);
        itRShellSpinSetStatus(item_gobj);
    }
    else
    {
        itMainSetGroundAllowPickup(item_gobj);

        ip->physics.vel_air.x = 0.0F;

        itMainClearOwnerStats(item_gobj);

        ip->damage_coll.hitstatus = nGMHitStatusNormal;
        ip->attack_coll.attack_state = nGMAttackStateOff;

        itRShellCommonClearAnim(item_gobj);
        itMainSetStatus(item_gobj, dITRShellStatusDescs, nITRShellStatusWait);
    }
}

/* decomp itrshell.c:352-355 verbatim. */
/* 0x8017A964 */
void itRShellCommonProcStatusWaitOrSpin(GObj *item_gobj)
{
    itRShellCommonSetStatusWaitOrSpin(item_gobj);
}

/* decomp itrshell.c:358-368 verbatim. */
/* 0x8017A984 */
void itRShellFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    ip->damage_coll.hitstatus = nGMHitStatusNormal;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITRShellStatusDescs, nITRShellStatusFall);
}

/* decomp itrshell.c:371-399 verbatim. */
/* 0x8017A9D0 */
sb32 itRShellCommonProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->physics.vel_air.x = ip->damage_queue * ITRSHELL_DAMAGE_MUL_NORMAL * (-ip->damage_lr);

    if (ABSF(ip->physics.vel_air.x) > ITRSHELL_STOP_VEL_X)
    {
        ip->item_vars.shell.is_damage = TRUE;

        ip->attack_coll.attack_state = nGMAttackStateNew;

        itProcessUpdateAttackPositions(item_gobj);
        itMainCopyDamageStats(item_gobj);

        if (ip->ga != nMPKineticsGround)
        {
            itRShellSpinAirSetStatus(item_gobj);
        }
        else itRShellSpinSetStatus(item_gobj);
    }
    else
    {
        ip->physics.vel_air.x = 0.0F;

        ip->attack_coll.attack_state = nGMAttackStateOff;
    }
    return FALSE;
}

/* decomp itrshell.c:402-407 verbatim. */
/* 0x8017AABC */
void itRShellHoldSetStatus(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->rotate.vec.f.y = F_CST_DTOR32(0.0F);

    itMainSetStatus(item_gobj, dITRShellStatusDescs, nITRShellStatusHold);
}

/* decomp itrshell.c:410-422 verbatim. */
/* 0x8017AAF0 */
void itRShellThrownSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->item_vars.shell.health = 1;
    ip->item_vars.shell.is_damage = TRUE;
    ip->item_vars.shell.damage_all_delay = ITRSHELL_DAMAGE_ALL_WAIT;

    ip->times_thrown = 0;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITRShellStatusDescs, nITRShellStatusThrown);
}

/* decomp itrshell.c:425-437 verbatim. */
/* 0x8017AB48 */
void itRShellDroppedSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->item_vars.shell.health = 1;
    ip->item_vars.shell.is_damage = TRUE;
    ip->item_vars.shell.damage_all_delay = ITRSHELL_DAMAGE_ALL_WAIT;

    ip->times_thrown = 0;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITRShellStatusDescs, nITRShellStatusDropped);
}

/* decomp itrshell.c:440-455 verbatim. */
/* 0x8017ABA0 */
sb32 itRShellThrownProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapCheckLanding(item_gobj, 0.25F, 0.5F, itRShellSpinSetStatus) != FALSE)
    {
        if (ip->physics.vel_air.x < 0.0F)
        {
            ip->lr = -1;
        }
        else ip->lr = +1;

        ip->physics.vel_air.x = ((ip->lr * -8.0F) + -10.0F) * 0.7F;
    }
    return FALSE;
}

/* decomp itrshell.c:458-471 verbatim. */
/* 0x8017AC40 - 0 = left, 1 = right */
void itRShellSpinEdgeInvertVelLR(GObj *item_gobj, ub8 lr)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->physics.vel_air.x = -ip->physics.vel_air.x;

    ip->item_vars.shell.vel_x = -ip->item_vars.shell.vel_x;

    if (lr != 0)
    {
        ip->lr = +1;
    }
    else ip->lr = -1;
}

/* decomp itrshell.c:474-502 verbatim. */
/* 0x8017AC84 */
void itRShellSpinCheckCollisionEdge(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITAttributes *attr = ip->attr;
    DObj *joint = DObjGetStruct(item_gobj);
    Vec3f pos;

    if (mpCollisionCheckExistLineID(ip->coll_data.floor_line_id) != FALSE)
    {
        if (ip->lr == -1)
        {
            mpCollisionGetFloorEdgeL(ip->coll_data.floor_line_id, &pos);

            if (pos.x >= (joint->translate.vec.f.x - attr->map_coll_width))
            {
                itRShellSpinEdgeInvertVelLR(item_gobj, 1);
            }
        }
        else
        {
            mpCollisionGetFloorEdgeR(ip->coll_data.floor_line_id, &pos);

            if (pos.x <= (joint->translate.vec.f.x + attr->map_coll_width))
            {
                itRShellSpinEdgeInvertVelLR(item_gobj, 0);
            }
        }
    }
}

/* decomp itrshell.c:505-520 verbatim. */
/* 0x8017AD7C */
sb32 itRShellSpinProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itRShellSpinUpdateGFX(item_gobj);
    itRShellSpinSearchFollowPlayer(item_gobj);
    itRShellSpinCheckCollisionEdge(item_gobj);

    if (ip->lifetime == 0)
    {
        return TRUE;
    }
    else ip->lifetime--;

    return FALSE;
}

/* decomp itrshell.c:523-537 verbatim. */
/* 0x8017ADD4 */
sb32 itRShellSpinProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if ((itMapCheckLRWallProcNoFloor(item_gobj, itRShellSpinAirSetStatus) != FALSE) && (ip->coll_data.mask_curr & (MAP_FLAG_RWALL | MAP_FLAG_LWALL)))
    {
        ip->physics.vel_air.x = -ip->physics.vel_air.x;

        itMainSetSpinVelLR(item_gobj);
        itMainClearOwnerStats(item_gobj);

        ip->item_vars.shell.vel_x = -ip->item_vars.shell.vel_x;
    }
    return FALSE;
}

/* decomp itrshell.c:540-565 verbatim. */
/* 0x8017AE48 */
sb32 itRShellCommonProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->item_vars.shell.interact--;

    if (ip->item_vars.shell.interact == 0)
    {
        return TRUE;
    }
    ip->damage_coll.hitstatus = nGMHitStatusNormal;

    ip->item_vars.shell.health = syUtilsRandIntRange(ITRSHELL_HEALTH_MAX);

    ip->physics.vel_air.x = ((ip->physics.vel_air.x * -1.0F) + (ITRSHELL_RECOIL_VEL_X * ip->hit_lr)) * ITRSHELL_RECOIL_MUL_X;

    itRShellCommonClearAnim(item_gobj);

    if (ip->ga != nMPKineticsGround)
    {
        itRShellSpinAirSetStatus(item_gobj);
    }
    else itRShellSpinSetStatus(item_gobj);

    return FALSE;
}

/* decomp itrshell.c:568-593 verbatim. */
/* 0x8017AF18 */
sb32 itRShellSpinProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->item_vars.shell.interact--;

    if (ip->item_vars.shell.interact == 0)
    {
        return TRUE;
    }
    ip->physics.vel_air.x += (ip->damage_queue * 2.0F) * -ip->damage_lr;

    if (ABSF(ip->physics.vel_air.x) > ITRSHELL_STOP_VEL_X)
    {
        ip->attack_coll.attack_state = nGMAttackStateNew;

        itProcessUpdateAttackPositions(item_gobj);
        itMainCopyDamageStats(item_gobj);
        itRShellSpinSetStatus(item_gobj);
    }
    else
    {
        ip->attack_coll.attack_state = nGMAttackStateOff;
    }
    return FALSE;
}

/* decomp itrshell.c:596-633 verbatim. */
/* 0x8017AFEC */
void itRShellSpinInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;
    ip->pickup_wait = ITEM_PICKUP_WAIT_DEFAULT;

    if (ip->physics.vel_air.x > ITRSHELL_CLAMP_VEL_X)
    {
        ip->physics.vel_air.x = ITRSHELL_CLAMP_VEL_X;
    }
    if (ip->physics.vel_air.x < -ITRSHELL_CLAMP_VEL_X)
    {
        ip->physics.vel_air.x = -ITRSHELL_CLAMP_VEL_X;
    }
    ip->physics.vel_air.y = 0.0F;

    if (ip->physics.vel_air.x < 0.0F)
    {
        ip->lr = -1;
    }
    else ip->lr = +1;

    if (ip->item_vars.shell.is_setup_vars == FALSE)
    {
        ip->lifetime = ITRSHELL_LIFETIME;

        ip->item_vars.shell.is_setup_vars = TRUE;

        ip->item_vars.shell.interact = ITRSHELL_INTERACT_MAX;
    }
    ip->item_vars.shell.dust_effect_int = ITRSHELL_EFFECT_SPAWN_INT;

    itRShellSpinAddAnim(item_gobj);
    func_800269C0_275C0(nSYAudioFGMBombHeiWalkStart);
    itMainClearOwnerStats(item_gobj);
    itMapSetGround(ip);
}

/* decomp itrshell.c:636-640 verbatim. */
/* 0x8017B0D4 */
void itRShellSpinSetStatus(GObj *item_gobj)
{
    itRShellSpinInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITRShellStatusDescs, nITRShellStatusSpin);
}

/* decomp itrshell.c:643-665 verbatim. */
/* 0x8017B108 */
void itRShellSpinAirInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    if (ip->physics.vel_air.x > ITRSHELL_CLAMP_AIR_X)
    {
        ip->physics.vel_air.x = ITRSHELL_CLAMP_AIR_X;
    }
    if (ip->physics.vel_air.x < -ITRSHELL_CLAMP_AIR_X)
    {
        ip->physics.vel_air.x = -ITRSHELL_CLAMP_AIR_X;
    }
    if (ip->physics.vel_air.x < 0.0F)
    {
        ip->lr = -1;
    }
    else ip->lr = +1;

    itMainClearOwnerStats(item_gobj);
    itMapSetAir(ip);
}

/* decomp itrshell.c:668-672 verbatim. */
/* 0x8017B1A4 */
void itRShellSpinAirSetStatus(GObj *item_gobj)
{
    itRShellSpinAirInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITRShellStatusDescs, nITRShellStatusSpinAir);
}

/* decomp itrshell.c:675-715 verbatim, REGION_US arms honoured. */
/* 0x8017B1D8 */
GObj* itRShellMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITRShellItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        DObj *dobj = DObjGetStruct(item_gobj);
        ITStruct *ip;
#if defined(REGION_US)
        Vec3f translate = dobj->translate.vec.f;
#endif

        dobj->rotate.vec.f.y = F_CST_DTOR32(90.0F);

        gcAddXObjForDObjFixed(dobj, nGCMatrixKindTraRotRpyR, 0);
        gcAddXObjForDObjFixed(dobj, 0x48, 0);

#if defined(REGION_US)
        dobj->translate.vec.f = translate;
#else
        dobj->translate.vec.f = *pos;
#endif

        dobj->mobj->palette_id = 0.0F;

        ip = itGetStruct(item_gobj);

        ip->attack_coll.can_rehit_shield = TRUE;

        ip->item_vars.shell.health = 1;
        ip->item_vars.shell.is_setup_vars = FALSE;
        ip->item_vars.shell.is_damage = FALSE;
        ip->item_vars.shell.damage_all_delay = -1;
        ip->item_vars.shell.vel_x = 0;

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

/* decomp itrshell.c:718-723 verbatim. */
/* 0x8017B2F8 */
sb32 itRShellCommonProcShield(GObj *item_gobj)
{
    itMainVelSetRebound(item_gobj);

    return FALSE;
}

/* decomp itrshell.c:726-763 verbatim. */
/* 0x8017B31C */
sb32 itRShellCommonProcReflector(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *item_dobj = DObjGetStruct(item_gobj), *fighter_dobj = DObjGetStruct(ip->owner_gobj);

    ip->item_vars.shell.interact--;

    if (ip->item_vars.shell.interact == 0)
    {
        return TRUE;
    }

    if (item_dobj->translate.vec.f.x < fighter_dobj->translate.vec.f.x)
    {
        ip->lr = -1;

        if (ip->physics.vel_air.x >= 0.0F)
        {
            ip->physics.vel_air.x = -ip->physics.vel_air.x;
            ip->item_vars.shell.vel_x = -ip->item_vars.shell.vel_x;
        }
    }
    else
    {
        ip->lr = +1;

        if (ip->physics.vel_air.x < 0.0F)
        {
            ip->physics.vel_air.x = -ip->physics.vel_air.x;
            ip->item_vars.shell.vel_x = -ip->item_vars.shell.vel_x;
        }
    }
    ip->physics.vel_air.x += (ITRSHELL_ADD_VEL_X * ip->lr);

    itMainClearOwnerStats(item_gobj);

    return FALSE;
}

#endif /* NDS_P2_ITEM_CORE */
