/* P2 Mushroom Kingdom Piranha Plant (kind nITKindPakkun). Verbatim-adapted
 * from decomp/BattleShip-main/decomp/src/it/itground/itpakkun.c:12-347.
 *
 * Art/state live in the stage's own map file: the descriptor's file base is
 * gGRCommonStruct.inishie.item_head (port include/gr/ground.h:322-325) and
 * the reloc tokens below are decomp/BattleShip-main/include/reloc_data.us.h
 * :3925 (:0x120 attributes), :3926 (:0xCC8 appear anim joint), :3927 (:0xCF8
 * appear matanim joint) and :3928 (:0xE04 damaged matanim joint); the port's
 * generated reloc header does not publish Inishie tokens, so this TU owns
 * its uintptr_t tokens the same way battleship_item_gbumper.c owns
 * GBumper's (local tokens, no generator involvement, no hand-edited
 * generated file).
 *
 * One deliberate omission: itPakkunCommonSetWaitFighter
 * (decomp itpakkun.c:118-126) is NOT defined here. It already lives verbatim
 * in battleship_item_link_core.c:159-167 (declared by port
 * include/it/item.h:964) because Mushroom Kingdom calls it before this kind
 * lands; defining it twice is a link error.
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal
 * (ITPAKKUN_* rides on <it/item.h>; the 0x30 secondary matrix kind and the
 * 0x46 XObj kind are the source's literals, kept verbatim). Symbols the port
 * headers do not publish yet (the anim-joint helpers) are referenced verbatim
 * through local externs and listed in the task report -- no values invented
 * here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <common.h>
#include <gr/ground.h>
#include <reloc_data.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3925. */
uintptr_t llGRInishieMapPakkunItemAttributes = 0x120u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3926. */
uintptr_t llGRInishieMapPakkunAppearAnimJoint = 0xCC8u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3927. */
uintptr_t llGRInishieMapPakkunAppearMatAnimJoint = 0xCF8u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3928. */
uintptr_t llGRInishieMapPakkunDamagedMatAnimJoint = 0xE04u;

/* decomp sys/objanim.h. No port header in this TU's chain publishes them;
 * battleship_item_dogas.c:49-51 carries the same local externs. */
extern void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                               f32 anim_frame);
extern void gcAddMObjMatAnimJoint(MObj *mobj, AObjEvent32 *matanim_joint,
                                  f32 anim_frame);
extern void gcPlayAnimAll(GObj *gobj);

/* decomp itpakkun.h:8-20 verbatim, minus itPakkunCommonSetWaitFighter (see
 * file header: owned by battleship_item_link_core.c, declared by
 * <it/item.h>). The port publishes no per-kind item procs, so the source
 * header's declarations travel with this TU, exactly as the Tomato and Star
 * files carry theirs. */
extern void itPakkunWaitSetStatus(GObj *item_gobj);
extern void itPakkunAppearSetStatus(GObj *item_gobj);
extern void itPakkunDamagedSetStatus(GObj *item_gobj);
extern sb32 itPakkunCommonCheckNoFighter(GObj *item_gobj);
extern sb32 itPakkunWaitProcUpdate(GObj *item_gobj);
extern void itPakkunWaitInitVars(GObj *item_gobj);
extern void itPakkunAppearUpdateDamageColl(GObj *item_gobj);
extern sb32 itPakkunAppearProcUpdate(GObj *item_gobj);
extern sb32 itPakkunAppearProcDamage(GObj *item_gobj);
extern sb32 itPakkunDamagedProcUpdate(GObj *item_gobj);
extern sb32 itPakkunDamagedProcDead(GObj *item_gobj);
extern GObj* itPakkunMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

// decomp itpakkun.c:12-34 verbatim, adapted only for the port's ITDesc shape
// (o_attributes is const void * here, lbRelocGetFileData takes the token the
// same way).
ITDesc dITPakkunItemDesc =
{
    nITKindPakkun,                          // Item Kind
    &gGRCommonStruct.inishie.item_head,     // Pointer to item file data?
    &llGRInishieMapPakkunItemAttributes,    // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTra,                   // Main matrix transformations
        0x30,                               // Secondary matrix transformations?
        0                                   // ???
    },

    nGMAttackStateOff,                      // Hitbox Update State
    itPakkunWaitProcUpdate,                 // Proc Update
    NULL,                                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// decomp itpakkun.c:36-73 verbatim.
ITStatusDesc dITPakkunStatusDescs[/* */] =
{
    // Status 0 (Dokan Wait)
    {
        itPakkunWaitProcUpdate,             // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 1 (Dokan Appear)
    {
        itPakkunAppearProcUpdate,           // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        itPakkunAppearProcDamage            // Proc Damage
    },

    // Status 2 (Neutral Damage)
    {
        itPakkunDamagedProcUpdate,          // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },
};

// decomp itpakkun.c:81-87 verbatim.
enum itPakkunStatus
{
    nITPakkunStatusWait,
    nITPakkunStatusAppear,
    nITPakkunStatusDamaged,
    nITPakkunStatusEnumCount
};

// 0x8017CF20
// decomp itpakkun.c:96-101 verbatim.
void itPakkunWaitSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITPakkunStatusDescs, nITPakkunStatusWait);

    itGetStruct(item_gobj)->proc_dead = NULL;
}

// 0x8017CF58
// decomp itpakkun.c:104-107 verbatim.
void itPakkunAppearSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITPakkunStatusDescs, nITPakkunStatusAppear);
}

// 0x8017CF80
// decomp itpakkun.c:110-115 verbatim.
void itPakkunDamagedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITPakkunStatusDescs, nITPakkunStatusDamaged);

    itGetStruct(item_gobj)->proc_dead = itPakkunDamagedProcDead;
}

// 0x8017CFDC
// decomp itpakkun.c:129-160 verbatim.
sb32 itPakkunCommonCheckNoFighter(GObj *item_gobj)
{
    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);
        GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
        f32 it_pos_x = ip->item_vars.pakkun.pos.x;
        f32 it_pos_y = ip->item_vars.pakkun.pos.y;

        while (fighter_gobj != NULL)
        {
            FTStruct *fp = ftGetStruct(fighter_gobj);
            DObj *dobj = fp->joints[nFTPartsJointTopN];
            f32 dist_x, ft_pos_y;

            if (dobj->translate.vec.f.x < it_pos_x)
            {
                dist_x = -(dobj->translate.vec.f.x - it_pos_x);
            }
            else dist_x = (dobj->translate.vec.f.x - it_pos_x);

            ft_pos_y = dobj->translate.vec.f.y;

            if ((dist_x < ITPAKKUN_DETECT_SIZE_WIDTH) && (ft_pos_y > (it_pos_y + ITPAKKUN_DETECT_SIZE_BOTTOM)) && (ft_pos_y < (it_pos_y + ITPAKKUN_DETECT_SIZE_TOP)))
            {
                return FALSE;
            }
            fighter_gobj = fighter_gobj->link_next;
        }
    }
    return TRUE;
}

// 0x8017D0A4
// decomp itpakkun.c:163-189 verbatim.
sb32 itPakkunWaitProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->item_vars.pakkun.is_wait_fighter != FALSE)
    {
        ip->multi = ITPAKKUN_APPEAR_WAIT;
        ip->item_vars.pakkun.is_wait_fighter = FALSE;
    }
    if (--ip->multi == 0)
    {
        if (itPakkunCommonCheckNoFighter(item_gobj) != FALSE)
        {
            DObj *dobj = DObjGetStruct(item_gobj);

            gcAddDObjAnimJoint(dobj, lbRelocGetFileData(AObjEvent32*, gGRCommonStruct.inishie.map_head, &llGRInishieMapPakkunAppearAnimJoint), 0.0F);
            gcAddMObjMatAnimJoint(dobj->mobj, lbRelocGetFileData(AObjEvent32*, gGRCommonStruct.inishie.map_head, &llGRInishieMapPakkunAppearMatAnimJoint), 0.0F);
            gcPlayAnimAll(item_gobj);

            dobj->translate.vec.f.y += ip->item_vars.pakkun.pos.y;

            itPakkunAppearSetStatus(item_gobj);
        }
        else ip->multi = ITPAKKUN_APPEAR_WAIT;
    }
    return FALSE;
}

// 0x8017D190
// decomp itpakkun.c:192-204 verbatim.
void itPakkunWaitInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = ITPAKKUN_APPEAR_WAIT;

    itPakkunWaitSetStatus(item_gobj);

    ip->damage_coll.hitstatus = nGMHitStatusNone;
    ip->attack_coll.attack_state = nGMAttackStateOff;

    DObjGetStruct(item_gobj)->translate.vec.f.y = ip->item_vars.pakkun.pos.y;
}

// 0x8017D1DC
// decomp itpakkun.c:207-229 verbatim.
void itPakkunAppearUpdateDamageColl(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    f32 pos_y = DObjGetStruct(item_gobj)->translate.vec.f.y - ip->item_vars.pakkun.pos.y;
    f32 off_y = pos_y + ITPAKKUN_APPEAR_OFF_Y;

    if (off_y <= ITPAKKUN_CLAMP_OFF_Y)
    {
        ip->damage_coll.hitstatus = nGMHitStatusNone;
        ip->attack_coll.attack_state = nGMAttackStateOff;
    }
    else
    {
        if (ip->damage_coll.hitstatus == nGMHitStatusNone)
        {
            ip->damage_coll.hitstatus = nGMHitStatusNormal;

            itMainRefreshAttackColl(item_gobj);
        }
        ip->damage_coll.size.y = (off_y - ITPAKKUN_CLAMP_OFF_Y) * ITPAKKUN_HURT_SIZE_MUL_Y;
        ip->damage_coll.offset.y = (ip->damage_coll.size.y + ITPAKKUN_CLAMP_OFF_Y) - pos_y;
    }
}

// 0x8017D298
// decomp itpakkun.c:232-256 verbatim.
sb32 itPakkunAppearProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj;

    if (ip->item_vars.pakkun.is_wait_fighter != FALSE)
    {
        DObjGetStruct(item_gobj)->anim_wait = AOBJ_ANIM_NULL;

        itPakkunWaitInitVars(item_gobj);

        ip->item_vars.pakkun.is_wait_fighter = FALSE;
    }
    dobj = DObjGetStruct(item_gobj);

    if (dobj->anim_wait == AOBJ_ANIM_NULL)
    {
        itPakkunWaitInitVars(item_gobj);
    }
    else dobj->translate.vec.f.y += ip->item_vars.pakkun.pos.y;

    itPakkunAppearUpdateDamageColl(item_gobj);

    return FALSE;
}

// 0x8017D334
// decomp itpakkun.c:259-288 verbatim.
sb32 itPakkunAppearProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->damage_knockback >= ITPAKKUN_NDAMAGE_KNOCKBACK_MIN)
    {
        DObj *dobj = DObjGetStruct(item_gobj);
        f32 angle;

        dobj->xobjs[1]->kind = 0x46;

        dobj->rotate.vec.f.z = F_CST_DTOR32(180.0F);

        angle = ftCommonDamageGetKnockbackAngle(ip->damage_angle, ip->ga, ip->damage_knockback);

        ip->physics.vel_air.x = __cosf(angle) * ip->damage_knockback * -ip->damage_lr;
        ip->physics.vel_air.y = __sinf(angle) * ip->damage_knockback;

        ip->damage_coll.hitstatus = nGMHitStatusNone;
        ip->attack_coll.attack_state = nGMAttackStateOff;

        itPakkunDamagedSetStatus(item_gobj);

        dobj->anim_wait = AOBJ_ANIM_NULL;

        gcAddMObjMatAnimJoint(dobj->mobj, lbRelocGetFileData(AObjEvent32*, gGRCommonStruct.inishie.map_head, &llGRInishieMapPakkunDamagedMatAnimJoint), 0.0F);
        gcPlayAnimAll(item_gobj);
    }
    return FALSE;
}

// 0x8017D434
// decomp itpakkun.c:291-298 verbatim.
sb32 itPakkunDamagedProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITPAKKUN_GRAVITY, ITPAKKUN_TVEL);

    return FALSE;
}

// 0x8017D460
// decomp itpakkun.c:301-323 verbatim.
sb32 itPakkunDamagedProcDead(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->translate.vec.f = ip->item_vars.pakkun.pos;

    ip->multi = ITPAKKUN_REBIRTH_WAIT;

    ip->physics.vel_air.x = 0.0F;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.z = 0.0F;

    dobj->rotate.vec.f.z = 0.0F;

    dobj->mobj->anim_wait = AOBJ_ANIM_NULL;

    itPakkunWaitSetStatus(item_gobj);

    ip->item_vars.pakkun.is_wait_fighter = FALSE;

    return FALSE;
}

// 0x8017D4D8
// decomp itpakkun.c:326-347 verbatim.
GObj* itPakkunMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITPakkunItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        ip->item_vars.pakkun.pos = *pos;

        DObjGetStruct(item_gobj)->translate.vec.f = *pos;

        ip->multi = ITPAKKUN_APPEAR_WAIT;

        ip->is_allow_knockback = TRUE;

        ip->item_vars.pakkun.is_wait_fighter = FALSE;

        ip->attack_coll.can_rehit_shield = TRUE;
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
