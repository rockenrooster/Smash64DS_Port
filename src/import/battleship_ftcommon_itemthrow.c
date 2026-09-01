/*
 * NDS_PARTIAL_IMPORT: decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonitemthrow.c
 *
 * Donkey-only builds retain the bounded items-off subset that admitted DK
 * without pulling the item manager into that configuration. Link is the first
 * live shared-item client, so Link-enabled builds graduate this TU completely:
 * BattleShip owns the item-throw command/status variables, directional throw
 * selection, animation-event release, throw velocity/damage math and the
 * heavy-item interrupt. No fighter-local LinkBomb shortcut is allowed here.
 */
#if NDS_P2_LINK

#include <ft/fighter.h>
#include <it/item.h>

/* BattleShip ft/ftcommon/ftcommonget.c:128-136.  The full common item-throw
 * selector installs this callback for heavy throws; keep that callback source-
 * exact without importing the rest of the unrelated item-pickup state machine. */
void ftCommonHeavyGetProcDamage(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->item_gobj != NULL)
    {
        ftSetupDropItem(fp);
    }
}

/* BattleShip ft/ftcommondata.c:146-323. Keep the complete directional table:
 * once the source common throw selector is live, every reachable throw status
 * must use the source velocity/angle/damage/smash descriptor, not a Link-only
 * forward-throw fixture. */
FTItemThrow dFTCommonDataItemThrowDescs[] =
{
    { FALSE,  36, 110,  50 }, /* LightThrowDrop */
    { FALSE, 120,  10, 100 }, /* LightThrowDash */
    { FALSE,  60,  15, 100 }, /* LightThrowF */
    { FALSE,  60,  15, 100 }, /* LightThrowB */
    { FALSE,  65,  90, 100 }, /* LightThrowHi */
    { FALSE,  65, -70, 100 }, /* LightThrowLw */
    { TRUE,  110,   8, 100 }, /* LightThrowF4 */
    { TRUE,  110,   8, 100 }, /* LightThrowB4 */
    { TRUE,  110,  90, 100 }, /* LightThrowHi4 */
    { TRUE,  110, -70, 100 }, /* LightThrowLw4 */
    { FALSE,  75,   8, 100 }, /* LightThrowAirF */
    { FALSE,  75,   8, 100 }, /* LightThrowAirB */
    { FALSE,  80,  90, 100 }, /* LightThrowAirHi */
    { FALSE,  75, -90, 100 }, /* LightThrowAirLw */
    { TRUE,  120,   7, 100 }, /* LightThrowAirF4 */
    { TRUE,  120,   7, 100 }, /* LightThrowAirB4 */
    { TRUE,  120,  90, 100 }, /* LightThrowAirHi4 */
    { TRUE,  140, -90, 100 }, /* LightThrowAirLw4 */
    { FALSE,  70,  60, 100 }, /* HeavyThrowF */
    { FALSE,  70,  60, 100 }, /* HeavyThrowB */
    { TRUE,   90,  20, 100 }, /* HeavyThrowF4 */
    { TRUE,   90,  20, 100 }  /* HeavyThrowB4 */
};

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonitemthrow.c"

#else

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

#endif /* NDS_P2_LINK */
