/* The fighter half of item pickup. Verbatim from
 * decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonget.c:12-232.
 *
 * Items have spawned, bounced, exploded and been counted for a while now, and
 * no fighter could touch one. `itMainSetFighterHold` was ported with the item
 * core and had no caller; every proc in the source's Get status was a weak
 * `NDS_INACTIVE_STATUS_STUB`; and `ftCommonGetCheckInterruptCommon` -- the
 * function every ground attack calls to ask "is there an item here instead?"
 * -- was a compat shim returning FALSE. So the answer was always no.
 *
 * This TU is that chain: find the nearest item in range, enter LightGet or
 * HeavyGet, and on the animation's flag1 frame put it in the fighter's hand.
 * The stubs it replaces are weak, so a strong definition here wins the link
 * without editing that file; `ftCommonGetCheckInterruptCommon` is NOT weak and
 * its shim is gated off in reloc_backend_compat_shims.c.
 *
 * Gated on NDS_P2_ITEM_CORE: without items there is nothing to pick up, and
 * the weak stubs remain the right answer.
 */
#if NDS_P2_ITEM_CORE

#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <it/item.h>
#include <sc/scene.h>
#include <sys/objdef.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp ft/ftcommon.h:188-189. */
#define FTCOMMON_GET_MASK_LIGHT (1 << 0)
#define FTCOMMON_GET_MASK_HEAVY (1 << 1)

/* decomp sc/sc1pmode/sc1pgame.c. The 1P bonuses the consume path increments;
 * P2-6 owns the mode that reads them, so they live beside their only writer
 * until then, exactly as gSC1PGameBonusMewCatcher does in
 * battleship_item_map_core.c. */
__attribute__((used)) u8 gSC1PGameBonusTomatoCount;
__attribute__((used)) u8 gSC1PGameBonusHeartCount;

/* decomp ft/ftcommon.h:191-192. Eight frames for a half turn, so the step is
 * -180 degrees over the count. */
#define FTCOMMON_LIFT_TURN_FRAMES 8
#define FTCOMMON_LIFT_TURN_STEP (-(F_CLC_DTOR32(180.0F) / FTCOMMON_LIFT_TURN_FRAMES))

extern void ftSetupDropItem(FTStruct *fp);
extern void ftCommonFallSetStatus(GObj *fighter_gobj);
extern void ftCommonWaitSetStatus(GObj *fighter_gobj);
extern void ftHammerSetStatusHammerWait(GObj *fighter_gobj);
extern sb32 mpCommonCheckFighterOnFloor(GObj *fighter_gobj);
extern sb32 mpCommonCheckFighterOnEdge(GObj *fighter_gobj);
extern sb32 ftCommonHeavyThrowCheckInterruptCommon(GObj *fighter_gobj);
#if NDS_P2_DONKEY
extern void ftDonkeyThrowFWaitSetStatus(GObj *fighter_gobj);
#endif

void ftCommonLightGetProcDamage(GObj *fighter_gobj);
void ftCommonHeavyGetProcDamage(GObj *fighter_gobj);
void ftCommonLiftWaitSetStatus(GObj *fighter_gobj);
void ftCommonLiftTurnSetStatus(GObj *fighter_gobj);
sb32 ftCommonLiftTurnCheckInterruptLiftWait(GObj *fighter_gobj);

// 0x80145990
// decomp ftcommonget.c:12-78 verbatim.
GObj *ftCommonGetFindItem(GObj *fighter_gobj, u8 pickup_mask)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *pickup_gobj = NULL;
    FTItemPickup *item_pickup = &fp->attr->item_pickup;
    GObj *item_gobj = gGCCommonLinks[nGCCommonLinkIDItem];
    f32 closest_item_dist = F32_MAX;
    sb32 is_pickup;
    f32 current_item_dist;
    Vec2f pickup_range;

    while (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        if (ip->is_allow_pickup)
        {
            if (fp->coll_data.floor_line_id == ip->coll_data.floor_line_id)
            {
                Vec3f *ft_translate = &DObjGetStruct(fighter_gobj)->translate.vec.f;
                Vec3f *it_translate = &DObjGetStruct(item_gobj)->translate.vec.f;
                MPObjectColl *map_coll = &ip->coll_data.map_coll;

                is_pickup = FALSE;

                if ((ip->weight == nITWeightLight) && (pickup_mask & FTCOMMON_GET_MASK_LIGHT))
                {
                    pickup_range.x = ft_translate->x + (fp->lr * item_pickup->pickup_offset_light.x);
                    pickup_range.y = ft_translate->y + item_pickup->pickup_offset_light.y;

                    if ((((pickup_range.x - item_pickup->pickup_range_light.x) - map_coll->width) < it_translate->x) && (it_translate->x < (item_pickup->pickup_range_light.x + pickup_range.x + map_coll->width)))
                    {
                        if ((((pickup_range.y - item_pickup->pickup_range_light.y) - map_coll->top) < it_translate->y) && (it_translate->y < ((item_pickup->pickup_range_light.y + pickup_range.y) - map_coll->bottom)))
                        {
                            is_pickup = TRUE;
                        }
                    }
                }
                if ((ip->weight == nITWeightHeavy) && (pickup_mask & FTCOMMON_GET_MASK_HEAVY))
                {
                    pickup_range.x = ft_translate->x + (fp->lr * item_pickup->pickup_offset_heavy.x);
                    pickup_range.y = ft_translate->y + item_pickup->pickup_offset_heavy.y;

                    if ((((pickup_range.x - item_pickup->pickup_range_heavy.x) - map_coll->width) < it_translate->x) && (it_translate->x < (item_pickup->pickup_range_heavy.x + pickup_range.x + map_coll->width)))
                    {
                        if ((((pickup_range.y - item_pickup->pickup_range_heavy.y) - map_coll->top) < it_translate->y) && (it_translate->y < ((item_pickup->pickup_range_heavy.y + pickup_range.y) - map_coll->bottom)))
                        {
                            is_pickup = TRUE;
                        }
                    }
                }
                if (is_pickup != FALSE)
                {
                    current_item_dist = (pickup_range.x < it_translate->x) ? -(pickup_range.x - it_translate->x) : (pickup_range.x - it_translate->x);

                    if (current_item_dist < closest_item_dist)
                    {
                        closest_item_dist = current_item_dist;
                        pickup_gobj = item_gobj;
                    }
                }
            }
        }
        item_gobj = item_gobj->link_next;
    }
    return pickup_gobj;
}

// 0x80145BE4
// decomp ftcommonget.c:81-124 verbatim. The consume path: Tomato and Heart
// heal and are destroyed, the Hammer starts its timer and its music.
void ftCommonLightGetProcDamage(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *item_gobj = fp->item_gobj;

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        if (ip->type == nITTypeConsume)
        {
            switch (ip->kind)
            {
            case nITKindTomato:
                ftParamSetHealDamage(fp, ITTOMATO_DAMAGE_HEAL);
                itMainDestroyItem(item_gobj);

                if ((gSCManagerBattleState->game_type == nSCBattleGameType1PGame) && (fp->player == gSCManagerSceneData.player) && (gSC1PGameBonusTomatoCount < U8_MAX))
                {
                    gSC1PGameBonusTomatoCount++;
                }
                break;

            case nITKindHeart:
                ftParamSetHealDamage(fp, ITHEART_DAMAGE_HEAL);
                itMainDestroyItem(item_gobj);

                if ((gSCManagerBattleState->game_type == nSCBattleGameType1PGame) && (fp->player == gSCManagerSceneData.player) && (gSC1PGameBonusHeartCount < U8_MAX))
                {
                    gSC1PGameBonusHeartCount++;
                }
                break;

            case nITKindHammer:
                fp->hammer_tics = ITHAMMER_TIME;

                ftParamTryPlayItemMusic(nSYAudioBGMHammer);
                break;

            default:
                break;
            }
        }
    }
}

// 0x80145D28
// decomp ftcommonget.c:128-136 verbatim. A heavy item is dropped rather than
// consumed when its holder is hit.
void ftCommonHeavyGetProcDamage(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->item_gobj != NULL)
    {
        ftSetupDropItem(fp);
    }
}

/* Witnesses. A pickup is a chain of five questions and only the last one is
 * visible on screen, so each is counted where it is answered:
 * ndsFtGetSearchCount   -- an attack asked whether an item is in reach
 * ndsFtGetFoundCount    -- the search returned one
 * ndsFtGetStatusCount   -- the fighter entered LightGet or HeavyGet
 * ndsFtGetHoldCount     -- the animation reached flag1 and the item was held
 * ndsFtGetLastKind      -- which kind, so "held nothing" and "held the wrong
 *                          thing" are different readings. */
__attribute__((used)) volatile u32 gNdsFtGetSearchCount;
__attribute__((used)) volatile u32 gNdsFtGetFoundCount;
__attribute__((used)) volatile u32 gNdsFtGetStatusCount;
__attribute__((used)) volatile u32 gNdsFtGetHoldCount;
__attribute__((used)) volatile u32 gNdsFtGetLastKind;

// 0x80145D70
// decomp ftcommonget.c:139-191 verbatim.
void ftCommonGetProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *item_gobj;

    if (fp->motion_vars.flags.flag1 != 0)
    {
        fp->motion_vars.flags.flag1 = 0;

        item_gobj = ftCommonGetFindItem(fighter_gobj, ((fp->status_id == nFTCommonStatusHeavyGet) ? FTCOMMON_GET_MASK_HEAVY : FTCOMMON_GET_MASK_LIGHT));

        if (item_gobj != NULL)
        {
            itMainSetFighterHold(item_gobj, fighter_gobj);
            gNdsFtGetHoldCount++;
            gNdsFtGetLastKind = (u32)itGetStruct(item_gobj)->kind;
        }
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        if (fp->status_id == nFTCommonStatusHeavyGet)
        {
            if (fp->item_gobj != NULL)
            {
#if NDS_P2_DONKEY
                if ((fp->fkind == nFTKindDonkey) || (fp->fkind == nFTKindNDonkey) || (fp->fkind == nFTKindGDonkey))
                {
                    ftDonkeyThrowFWaitSetStatus(fighter_gobj);
                }
                else ftCommonLiftWaitSetStatus(fighter_gobj);
#else
                /* DK's own lift-wait is his TU's; without him the source's
                 * else-branch is the only reachable one. */
                ftCommonLiftWaitSetStatus(fighter_gobj);
#endif
            }
            else ftCommonWaitSetStatus(fighter_gobj);
        }
        else
        {
            item_gobj = fp->item_gobj;

            if (item_gobj != NULL)
            {
                ITStruct *ip = itGetStruct(item_gobj);

                if (ip->type == nITTypeConsume)
                {
                    ftCommonLightGetProcDamage(fighter_gobj);

                    if (ip->kind == nITKindHammer)
                    {
                        ftHammerSetStatusHammerWait(fighter_gobj);

                        return;
                    }
                }
            }
            ftCommonWaitSetStatus(fighter_gobj);
        }
    }
}

// 0x80145ED8
// decomp ftcommonget.c:195-202 verbatim.
void ftCommonLightGetProcMap(GObj *fighter_gobj)
{
    if (mpCommonCheckFighterOnFloor(fighter_gobj) == FALSE)
    {
        ftCommonLightGetProcDamage(fighter_gobj);
        ftCommonFallSetStatus(fighter_gobj);
    }
}

// 0x80145F10
// decomp ftcommonget.c:205-217 verbatim.
void ftCommonHeavyGetProcMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (mpCommonCheckFighterOnFloor(fighter_gobj) == FALSE)
    {
        if (fp->item_gobj != NULL)
        {
            ftSetupDropItem(fp);
        }
        ftCommonFallSetStatus(fighter_gobj);
    }
}

// 0x80145F74
// decomp ftcommonget.c:220-232 verbatim.
void ftCommonHeavyThrowProcMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (mpCommonCheckFighterOnEdge(fighter_gobj) == FALSE)
    {
        if (fp->item_gobj != NULL)
        {
            ftSetupDropItem(fp);
        }
        ftCommonFallSetStatus(fighter_gobj);
    }
}

// 0x80145FD8
// decomp ftcommonget.c:235-250 verbatim.
void ftCommonGetSetStatus(GObj *fighter_gobj, GObj *item_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    ITStruct *ip = itGetStruct(item_gobj);

    fp->motion_vars.flags.flag1 = 0;

    ftMainSetStatus(fighter_gobj, ((ip->weight == nITWeightHeavy) ? nFTCommonStatusHeavyGet : nFTCommonStatusLightGet), 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);

    if (fp->status_id == nFTCommonStatusHeavyGet)
    {
        fp->proc_damage = ftCommonHeavyGetProcDamage;
    }
    else fp->proc_damage = ftCommonLightGetProcDamage;

    gNdsFtGetStatusCount++;
}

// 0x80146064
// decomp ftcommonget.c:253-269 verbatim. Every ground attack calls this first:
// with an item in reach, the attack becomes a pickup.
sb32 ftCommonGetCheckInterruptCommon(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    gNdsFtGetSearchCount++;

    if (fp->item_gobj == NULL)
    {
        GObj *item_gobj = ftCommonGetFindItem(fighter_gobj, (FTCOMMON_GET_MASK_LIGHT | FTCOMMON_GET_MASK_HEAVY));

        if (item_gobj != NULL)
        {
            gNdsFtGetFoundCount++;
            ftCommonGetSetStatus(fighter_gobj, item_gobj);

            return TRUE;
        }
    }
    return FALSE;
}


/* The heavy branch, decomp ftcommonget.c:272-348 verbatim. A heavy item is a
 * container, so without this the four crates would be scenery: LightGet is the
 * only status reachable and the search mask would have to drop
 * FTCOMMON_GET_MASK_HEAVY, which is a gameplay cut rather than a deferral.
 * Every symbol it needs already existed. */

// 0x801460B8
void ftCommonLiftWaitProcInterrupt(GObj *fighter_gobj)
{
    if (ftCommonHeavyThrowCheckInterruptCommon(fighter_gobj) == FALSE)
    {
        ftCommonLiftTurnCheckInterruptLiftWait(fighter_gobj);
    }
}

// 0x801460E8
void ftCommonLiftWaitSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusLiftWait, 0.0F, 1.0F, FTSTATUS_PRESERVE_SLOPECONTOUR);

    fp->proc_damage = ftCommonHeavyGetProcDamage;
}

// 0x80146130
void ftCommonLiftTurnUpdateModelYaw(FTStruct *fp)
{
    fp->status_vars.common.lift.turn_tics--;

    fp->joints[nFTPartsJointTopN]->rotate.vec.f.y += FTCOMMON_LIFT_TURN_STEP;

    ftParamsUpdateFighterPartsTransformAll(fp->joints[nFTPartsJointTopN]);

    if (fp->status_vars.common.lift.turn_tics == (FTCOMMON_LIFT_TURN_FRAMES / 2))
    {
        fp->lr = -fp->lr;
        fp->physics.vel_ground.x = -fp->physics.vel_ground.x;
    }
}

// 0x801461A8
void ftCommonLiftTurnProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftCommonLiftTurnUpdateModelYaw(fp);

    if (fp->status_vars.common.lift.turn_tics == 0)
    {
        ftCommonLiftWaitSetStatus(fighter_gobj);
    }
}

// 0x801461E8
void ftCommonLiftTurnProcInterrupt(GObj *fighter_gobj)
{
    ftCommonHeavyThrowCheckInterruptCommon(fighter_gobj);
}

// 0x80146208
void ftCommonLiftTurnSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusLiftTurn, 0.0F, 1.0F, FTSTATUS_PRESERVE_SLOPECONTOUR);

    fp->proc_damage = ftCommonHeavyGetProcDamage;

    fp->status_vars.common.lift.turn_tics = FTCOMMON_LIFT_TURN_FRAMES;

    ftCommonLiftTurnUpdateModelYaw(fp);
}

// 0x8014625C
sb32 ftCommonLiftTurnCheckInterruptLiftWait(GObj *fighter_gobj)
{
    if (ftCommonTurnCheckInputSuccess(fighter_gobj) != FALSE)
    {
        ftCommonLiftTurnSetStatus(fighter_gobj);

        return TRUE;
    }
    else return FALSE;
}

#endif /* NDS_P2_ITEM_CORE */
