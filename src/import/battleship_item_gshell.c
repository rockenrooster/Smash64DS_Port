/* P2-5 GShell (Green Shell, nITKindGShell). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itgshell.c:10-589.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x53C), the spin anim bank base (0x5F88), the spin anim joint (0x6018)
 * and the spin matanim joint (0x6048) are
 * decomp/BattleShip-main/include/reloc_data.us.h:3753 and :3791-:3793;
 * the port's generated reloc header publishes none of the GShell tokens,
 * so this TU owns all four uintptr_t tokens the same way
 * battleship_item_harisen.c:31-34 owns Harisen's (local tokens, no
 * generator involvement, no hand-edited generated file).
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

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3753. */
uintptr_t llITCommonDataGShellItemAttributes = 0x53Cu;
/* decomp/BattleShip-main/include/reloc_data.us.h:3791. */
uintptr_t llITCommonDataShellDataStart = 0x5F88u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3792. */
uintptr_t llITCommonDataShellAnimJoint = 0x6018u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3793. */
uintptr_t llITCommonDataShellMatAnimJoint = 0x6048u;

extern void *gITManagerCommonData;

/* decomp sys/objanim.h:16, :19 and :52. No port header in this TU's chain
 * publishes them; battleship_item_harisen.c:38-42 carries the same kind of
 * local externs. */
extern void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                                f32 anim_frame);
extern void gcAddMObjMatAnimJoint(MObj *mobj, AObjEvent32 *matanim_joint,
                                   f32 anim_frame);
extern void gcPlayAnimAll(GObj *gobj);

/* decomp sys/utils.h:19-:20. No port header in this TU's chain publishes
 * them; battleship_item_link_core.c:205-207 carries the same kind of local
 * extern for syUtilsRandIntRange. */
extern f32 syUtilsRandFloat(void);
extern s32 syUtilsRandIntRange(s32 range);

/* decomp ef/efmanager.h:38. Same shape as the decomp prototype. */
extern LBParticle *efManagerDustLightMakeEffect(Vec3f *pos, s32 lr,
                                                 f32 scale);
/* Same shape as battleship_link_bomb.c:80. */
extern void func_800269C0_275C0(u16 sfx_id);

/* decomp itgshell.h:8-32 verbatim. The port header does not publish per-kind
 * item procs yet, so the source header's declarations travel with this TU. */
extern void itGShellSpinUpdateEffect(GObj *item_gobj);
extern void itGShellSpinAddAnim(GObj *item_gobj);
extern void itGShellCommonClearAnim(GObj *item_gobj);
extern sb32 itGShellFallProcUpdate(GObj *item_gobj);
extern sb32 itGShellWaitProcMap(GObj *item_gobj);
extern sb32 itGShellFallProcMap(GObj *item_gobj);
extern void itGShellWaitInitVars(GObj *item_gobj);
extern void itGShellWaitSetStatus(GObj *item_gobj);
extern void itGShellFallSetStatus(GObj *item_gobj);
extern sb32 itGShellCommonProcDamage(GObj *item_gobj);
extern void itGShellHoldSetStatus(GObj *item_gobj);
extern sb32 itGShellThrownProcMap(GObj *item_gobj);
extern sb32 itGShellThrownProcUpdate(GObj *item_gobj);
extern void itGShellThrownSetStatus(GObj *item_gobj);
extern void itGShellDroppedSetStatus(GObj *item_gobj);
extern sb32 itGShellSpinProcUpdate(GObj *item_gobj);
extern sb32 itGShellSpinProcMap(GObj *item_gobj);
extern sb32 itGShellCommonProcHit(GObj *item_gobj);
extern sb32 itGShellSpinProcDamage(GObj *item_gobj);
extern void itGShellSpinInitVars(GObj *item_gobj);
extern void itGShellSpinSetStatus(GObj *item_gobj);
extern void itGShellSpinAirInitVars(GObj *item_gobj);
extern void itGShellSpinAirSetStatus(GObj *item_gobj);
extern GObj* itGShellMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
extern sb32 itGShellCommonProcShield(GObj *item_gobj);

/* decomp itgshell.c:10-32 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITGShellItemDesc =
{
    nITKindGShell,                          /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataGShellItemAttributes,    /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindNull,                  /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateOff,                      /* Hitbox Update State */
    itGShellFallProcUpdate,                 /* Proc Update */
    itGShellFallProcMap,                    /* Proc Map */
    NULL,                                   /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itgshell.c:34-119 verbatim. */
ITStatusDesc dITGShellStatusDescs[/* */] =
{
    /* Status 0 (Ground Wait) */
    {
        NULL,                               /* Proc Update */
        itGShellWaitProcMap,                /* Proc Map */
        NULL,                               /* Proc Hit */
        NULL,                               /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        NULL,                               /* Proc Reflector */
        itGShellCommonProcDamage            /* Proc Damage */
    },

    /* Status 1 (Air Wait Fall) */
    {
        itGShellFallProcUpdate,             /* Proc Update */
        itGShellFallProcMap,                /* Proc Map */
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
        itGShellThrownProcUpdate,           /* Proc Update */
        itGShellThrownProcMap,              /* Proc Map */
        itGShellCommonProcHit,              /* Proc Hit */
        itGShellCommonProcShield,           /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itGShellCommonProcShield,           /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        itGShellCommonProcDamage            /* Proc Damage */
    },

    /* Status 4 (Fighter Drop) */
    {
        itGShellFallProcUpdate,             /* Proc Update */
        itGShellThrownProcMap,              /* Proc Map */
        itGShellCommonProcHit,              /* Proc Hit */
        itGShellCommonProcShield,           /* Proc Shield */
        itMainCommonProcHop,                /* Proc Hop */
        itGShellCommonProcShield,           /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        itGShellCommonProcDamage            /* Proc Damage */
    },

    /* Status 5 (Ground Spin) */
    {
        itGShellSpinProcUpdate,             /* Proc Update */
        itGShellSpinProcMap,                /* Proc Map */
        itGShellCommonProcHit,              /* Proc Hit */
        itGShellCommonProcHit,              /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        itGShellSpinProcDamage              /* Proc Damage */
    },

    /* Status 6 (Air Spin) */
    {
        itGShellFallProcUpdate,             /* Proc Update */
        itGShellThrownProcMap,              /* Proc Map */
        itGShellCommonProcHit,              /* Proc Hit */
        itGShellCommonProcHit,              /* Proc Shield */
        NULL,                               /* Proc Hop */
        NULL,                               /* Proc Set-Off */
        itMainCommonProcReflector,          /* Proc Reflector */
        itGShellSpinProcDamage              /* Proc Damage */
    }
};

/* decomp itgshell.c:127-137 verbatim. */
enum itGShellStatus
{
    nITGShellStatusWait,
    nITGShellStatusFall,
    nITGShellStatusHold,
    nITGShellStatusThrown,
    nITGShellStatusDropped,
    nITGShellStatusSpin,
    nITGShellStatusSpinAir,
    nITGShellStatusEnumCount
};

/* decomp itgshell.c:146-163 verbatim. */
/* 0x801785E0 */
void itGShellSpinUpdateEffect(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    Vec3f pos;

    if (ip->item_vars.shell.dust_effect_int == 0)
    {
        pos = dobj->translate.vec.f;

        pos.y += ip->attr->map_coll_bottom;

        efManagerDustLightMakeEffect(&pos, ip->lr, 1.0F);

        ip->item_vars.shell.dust_effect_int = ITGSHELL_EFFECT_SPAWN_INT;
    }
    ip->item_vars.shell.dust_effect_int--;
}

/* decomp itgshell.c:166-175 verbatim. */
/* 0x80178670 */
void itGShellSpinAddAnim(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    s32 unused[2];

    gcAddDObjAnimJoint(dobj, itGetPData(ip, &llITCommonDataShellDataStart, &llITCommonDataShellAnimJoint), 0.0F);
    gcAddMObjMatAnimJoint(dobj->mobj, itGetPData(ip, &llITCommonDataShellDataStart, &llITCommonDataShellMatAnimJoint), 0.0F);
    gcPlayAnimAll(item_gobj);
}

/* decomp itgshell.c:178-182 verbatim. */
/* 0x80178704 */
void itGShellCommonClearAnim(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->mobj->matanim_joint.event32 = NULL;
    DObjGetStruct(item_gobj)->anim_joint.event32 = NULL;
}

/* decomp itgshell.c:185-192 verbatim. */
/* 0x8017871C */
sb32 itGShellFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITGSHELL_GRAVITY, ITGSHELL_TVEL);

    return FALSE;
}

/* decomp itgshell.c:195-200 verbatim. */
/* 0x8017874C */
sb32 itGShellWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itGShellFallSetStatus);

    return FALSE;
}

/* decomp itgshell.c:203-214 verbatim. */
/* 0x80178774 */
sb32 itGShellFallProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->item_vars.shell.health == 0)
    {
        return itMapCheckDestroyLanding(item_gobj, ITGSHELL_MAP_REBOUND_COMMON);
    }
    else itMapCheckDestroyDropped(item_gobj, ITGSHELL_MAP_REBOUND_COMMON, ITGSHELL_MAP_REBOUND_GROUND, itGShellWaitSetStatus);

    return FALSE;
}

/* decomp itgshell.c:217-260 verbatim. */
/* 0x801787CC */
void itGShellWaitInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMapSetGround(ip);

    if (ABSF(ip->physics.vel_air.x) < ITGSHELL_STOP_VEL_X)
    {
        itMainSetGroundAllowPickup(item_gobj);

        ip->item_vars.shell.is_damage = FALSE;

        ip->is_damage_all = TRUE;

        ip->damage_coll.hitstatus = nGMHitStatusNormal;
        ip->attack_coll.attack_state = nGMAttackStateOff;

        ip->physics.vel_air.x = 0.0F;

        itGShellCommonClearAnim(item_gobj);
        itMainSetStatus(item_gobj, dITGShellStatusDescs, nITGShellStatusWait);
    }
    else if (ip->item_vars.shell.is_damage != FALSE)
    {
        ip->attack_coll.attack_state = nGMAttackStateNew;

        itProcessUpdateAttackPositions(item_gobj);
        itGShellSpinSetStatus(item_gobj);
    }
    else
    {
        itMainSetGroundAllowPickup(item_gobj);

        ip->is_damage_all = TRUE;

        ip->damage_coll.hitstatus = nGMHitStatusNormal;
        ip->attack_coll.attack_state = nGMAttackStateOff;

        ip->physics.vel_air.x = 0.0F;

        itGShellCommonClearAnim(item_gobj);
        itMainSetStatus(item_gobj, dITGShellStatusDescs, nITGShellStatusWait);
    }
}

/* decomp itgshell.c:263-266 verbatim. */
/* 0x80178910 */
void itGShellWaitSetStatus(GObj *item_gobj)
{
    itGShellWaitInitVars(item_gobj);
}

/* decomp itgshell.c:269-281 verbatim. */
/* 0x80178930 */
void itGShellFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->damage_coll.hitstatus = nGMHitStatusNone;

    ip->attack_coll.attack_state = nGMAttackStateOff;

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITGShellStatusDescs, nITGShellStatusFall);
}

/* decomp itgshell.c:284-319 verbatim. */
/* 0x8017897C */
sb32 itGShellCommonProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->physics.vel_air.x = (ip->damage_queue * ITGSHELL_DAMAGE_MUL_NORMAL * -ip->damage_lr);

    if (ABSF(ip->physics.vel_air.x) > ITGSHELL_STOP_VEL_X)
    {
        ip->item_vars.shell.is_damage = TRUE;

        ip->attack_coll.attack_state = nGMAttackStateNew;

        itProcessUpdateAttackPositions(item_gobj);

        ip->damage_coll.hitstatus = nGMHitStatusNone;

        itMainCopyDamageStats(item_gobj);

        if (ip->ga != nMPKineticsGround)
        {
            itGShellSpinAirSetStatus(item_gobj);
        }
        else itGShellSpinSetStatus(item_gobj);
    }
    else
    {
        ip->physics.vel_air.x = 0.0F;

        if (ip->ga != nMPKineticsGround)
        {
            itGShellFallSetStatus(item_gobj);
        }
        else itGShellWaitSetStatus(item_gobj);
    }
    return FALSE;
}

/* decomp itgshell.c:322-327 verbatim. */
/* 0x80178A90 */
void itGShellHoldSetStatus(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->rotate.vec.f.y = 0.0F;

    itMainSetStatus(item_gobj, dITGShellStatusDescs, nITGShellStatusHold);
}

/* decomp itgshell.c:330-335 verbatim. */
/* 0x80178AC4 */
sb32 itGShellThrownProcMap(GObj *item_gobj)
{
    itMapCheckLanding(item_gobj, ITGSHELL_MAP_REBOUND_COMMON, ITGSHELL_MAP_REBOUND_GROUND, itGShellWaitSetStatus);

    return FALSE;
}

/* decomp itgshell.c:338-345 verbatim. */
/* 0x80178AF8 */
sb32 itGShellThrownProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITGSHELL_GRAVITY, ITGSHELL_TVEL);

    return FALSE;
}

/* decomp itgshell.c:348-356 verbatim. */
/* 0x80178B28 */
void itGShellThrownSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->item_vars.shell.health = 1;
    ip->item_vars.shell.is_damage = TRUE;

    itMainSetStatus(item_gobj, dITGShellStatusDescs, nITGShellStatusThrown);
}

/* decomp itgshell.c:359-367 verbatim. */
/* 0x80178B60 */
void itGShellDroppedSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->item_vars.shell.health = 1;
    ip->item_vars.shell.is_damage = TRUE;

    itMainSetStatus(item_gobj, dITGShellStatusDescs, nITGShellStatusDropped);
}

/* decomp itgshell.c:370-393 verbatim. */
/* 0x80178B98 */
sb32 itGShellSpinProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itGShellSpinUpdateEffect(item_gobj);

    if (!(ip->item_vars.shell.damage_all_delay))
    {
        ip->is_damage_all = TRUE;

        ip->item_vars.shell.damage_all_delay = -1;
    }
    if (ip->item_vars.shell.damage_all_delay != -1)
    {
        ip->item_vars.shell.damage_all_delay--;
    }
    if (ip->lifetime == 0)
    {
        return TRUE;
    }
    else ip->lifetime--;

    return FALSE;
}

/* decomp itgshell.c:396-412 verbatim. */
/* 0x80178C10 */
sb32 itGShellSpinProcMap(GObj *item_gobj)
{
    /*
     * OVERSIGHT (?): This sets the state of the shell to "Fall" when transitioning from the grounded spinning state,
     * causing the shell's hitbox to deactivate when flying off platforms.
     *
     * Solution: itMapCheckLRWallProcNoFloor(item_gobj, itGShellSpinAirSetStatus);
     */
    itMapCheckLRWallProcNoFloor(item_gobj, itGShellFallSetStatus);

    if (itMapCheckCollideAllRebound(item_gobj, (MAP_FLAG_CEIL | MAP_FLAG_RWALL | MAP_FLAG_LWALL), 0.2F, NULL) != FALSE)
    {
        itMainSetSpinVelLR(item_gobj);
        itMainClearOwnerStats(item_gobj);
    }
    return FALSE;
}

/* decomp itgshell.c:415-432 verbatim. */
/* 0x80178C6C */
sb32 itGShellCommonProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->damage_coll.hitstatus = nGMHitStatusNormal;

    ip->item_vars.shell.health = syUtilsRandIntRange(ITGSHELL_HEALTH_MAX);

    ip->physics.vel_air.y = ITGSHELL_REBOUND_VEL_Y;

    ip->physics.vel_air.x = syUtilsRandFloat() * (-ip->physics.vel_air.x * ITGSHELL_REBOUND_MUL_X);

    itMainClearOwnerStats(item_gobj);
    itGShellCommonClearAnim(item_gobj);
    itGShellFallSetStatus(item_gobj);

    return FALSE;
}

/* decomp itgshell.c:435-465 verbatim. */
/* 0x80178CF8 */
sb32 itGShellSpinProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->physics.vel_air.x += (ip->damage_queue * ITGSHELL_DAMAGE_MUL_ADD * -ip->damage_lr);

    if (ABSF(ip->physics.vel_air.x) > ITGSHELL_STOP_VEL_X)
    {
        ip->attack_coll.attack_state = nGMAttackStateNew;

        itProcessUpdateAttackPositions(item_gobj);
        itMainCopyDamageStats(item_gobj);

        if (ip->ga != nMPKineticsGround)
        {
            itGShellSpinAirSetStatus(item_gobj);
        }
        else itGShellSpinSetStatus(item_gobj);
    }
    else
    {
        ip->physics.vel_air.x = 0.0F;

        if (ip->ga != nMPKineticsGround)
        {
            itGShellFallSetStatus(item_gobj);
        }
        else itGShellWaitSetStatus(item_gobj);
    }
    return FALSE;
}

/* decomp itgshell.c:468-501 verbatim. */
/* 0x80178E04 */
void itGShellSpinInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    ip->pickup_wait = ITEM_PICKUP_WAIT_DEFAULT;

    if (ip->physics.vel_air.x > ITGSHELL_CLAMP_VEL_X)
    {
        ip->physics.vel_air.x = ITGSHELL_CLAMP_VEL_X;
    }
    if (ip->physics.vel_air.x < -ITGSHELL_CLAMP_VEL_X)
    {
        ip->physics.vel_air.x = -ITGSHELL_CLAMP_VEL_X;
    }
    ip->physics.vel_air.y = 0.0F;

    if (ip->physics.vel_air.x < 0.0F)
    {
        ip->lr = -1;
    }
    else ip->lr = +1;

    ip->item_vars.shell.dust_effect_int = ITGSHELL_EFFECT_SPAWN_INT;
    ip->item_vars.shell.damage_all_delay = ITGSHELL_DAMAGE_ALL_WAIT;

    itGShellSpinAddAnim(item_gobj);

    ip->is_damage_all = FALSE;

    itMainRefreshAttackColl(item_gobj);
    func_800269C0_275C0(nSYAudioFGMBombHeiWalkStart);
}

/* decomp itgshell.c:504-508 verbatim. */
/* 0x80178EDC */
void itGShellSpinSetStatus(GObj *item_gobj)
{
    itGShellSpinInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITGShellStatusDescs, nITGShellStatusSpin);
}

/* decomp itgshell.c:511-532 verbatim. */
/* 0x80178F10 */
void itGShellSpinAirInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->physics.vel_air.x > ITGSHELL_CLAMP_VEL_X)
    {
        ip->physics.vel_air.x = ITGSHELL_CLAMP_VEL_X;
    }
    if (ip->physics.vel_air.x < -ITGSHELL_CLAMP_VEL_X)
    {
        ip->physics.vel_air.x = -ITGSHELL_CLAMP_VEL_X;
    }
    if (ip->physics.vel_air.x < 0.0F)
    {
        ip->lr = -1;
    }
    else ip->lr = +1;

    ip->is_damage_all = FALSE;

    itMainRefreshAttackColl(item_gobj);
}

/* decomp itgshell.c:535-539 verbatim. */
/* 0x80178FA8 */
void itGShellSpinAirSetStatus(GObj *item_gobj)
{
    itGShellSpinAirInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITGShellStatusDescs, nITGShellStatusSpinAir);
}

/* decomp itgshell.c:542-581 verbatim, REGION_US arms honoured. */
/* 0x80178FDC */
GObj* itGShellMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITGShellItemDesc, pos, vel, flags);

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

        dobj->mobj->palette_id = 1.0F;

        ip = itGetStruct(item_gobj);

        ip->attack_coll.can_rehit_shield = TRUE;

        ip->item_vars.shell.health = 1;
        ip->item_vars.shell.is_damage = FALSE;

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);

        ip->lifetime = ITGSHELL_LIFETIME;
    }
    return item_gobj;
}

/* decomp itgshell.c:584-589 verbatim. */
/* 0x801790F4 */
sb32 itGShellCommonProcShield(GObj *item_gobj)
{
    itMainVelSetRebound(item_gobj);

    return FALSE;
}

#endif /* NDS_P2_ITEM_CORE */
