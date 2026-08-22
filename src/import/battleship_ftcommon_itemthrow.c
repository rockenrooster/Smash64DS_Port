/*
 * P2-3 Donkey Kong: the source cargo ladder calls the shared heavy-item
 * interrupt and the DK status table names common item-throw physics.
 *
 * The P2 production battle is explicitly items-off, so do not drag the whole
 * BattleShip item manager into the fighter admission slice just to satisfy
 * those two references. Keep the reachable behavior equivalent to
 * ftcommonitemthrow.c instead:
 *   - ItemThrow physics is exactly the source grounded/aerial branch.
 *   - Cargo's heavy-item interrupt is exactly false while item_gobj is NULL.
 *
 * The item-present branch deliberately remains owned by the later item-system
 * production slice. It must not claim an interrupt and transition through the
 * port's old no-op ftCommonItemThrowSetStatus seam. That would be a silent
 * behavior fork and would make cargo appear qualified when items are enabled.
 */
#include <ft/fighter.h>
#include <it/item.h>

void ftCommonItemThrowProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsAir)
    {
        ftPhysicsApplyAirVelDrift(fighter_gobj);
    }
    else
    {
        ftPhysicsApplyGroundVelFriction(fighter_gobj);
    }
}

sb32 ftCommonHeavyThrowCheckInterruptCommon(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    /* BattleShip ftcommonitemthrow.c:304. P2's items-off boundary keeps this
     * false. Do not approximate the true arm until itMainSetFighterThrow and
     * the common item-throw tables are admitted as source-faithful runtime. */
    if ((fp->item_gobj != NULL) &&
        (fp->input.pl.button_tap &
         (fp->input.button_mask_a | fp->input.button_mask_b)))
    {
        return FALSE;
    }
    return FALSE;
}
