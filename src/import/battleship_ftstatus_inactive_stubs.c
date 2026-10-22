#include <ft/fighter.h>

/* Source status tables name these callbacks, but their owning subsystems still
 * pull items, HUD, stage hazards, other fighters, or character specials. They
 * stay fenced until each original TU can be imported with its dependency chain.
 */
#define NDS_INACTIVE_STATUS_STUB(name)        \
    __attribute__((weak)) void name(          \
        GObj *fighter_gobj)                   \
    {                                         \
        (void)fighter_gobj;                   \
    }

NDS_INACTIVE_STATUS_STUB(ftCommonTaruCannProcUpdate)
NDS_INACTIVE_STATUS_STUB(ftCommonTaruCannProcInterrupt)
NDS_INACTIVE_STATUS_STUB(ftCommonTaruCannProcPhysics)
/* Dokan pipe statuses graduate here: src/import/battleship_ftcommon_dokan.c
 * owns the real bodies unconditionally, so no weak twin may shadow them. */
NDS_INACTIVE_STATUS_STUB(ftCommonGetProcUpdate)
NDS_INACTIVE_STATUS_STUB(ftCommonLightGetProcMap)
NDS_INACTIVE_STATUS_STUB(ftCommonHeavyGetProcMap)
NDS_INACTIVE_STATUS_STUB(ftCommonLiftWaitProcInterrupt)
NDS_INACTIVE_STATUS_STUB(ftCommonLiftTurnProcUpdate)
NDS_INACTIVE_STATUS_STUB(ftCommonLiftTurnProcInterrupt)
NDS_INACTIVE_STATUS_STUB(ftCommonItemThrowProcUpdate)
NDS_INACTIVE_STATUS_STUB(ftCommonHeavyThrowProcMap)
NDS_INACTIVE_STATUS_STUB(ftCommonHarisenSwingProcUpdate)
NDS_INACTIVE_STATUS_STUB(ftCommonStarRodSwingProcUpdate)
NDS_INACTIVE_STATUS_STUB(ftCommonLGunShootProcUpdate)
NDS_INACTIVE_STATUS_STUB(ftCommonLGunShootProcMap)
NDS_INACTIVE_STATUS_STUB(ftCommonLGunShootAirProcMap)
NDS_INACTIVE_STATUS_STUB(ftCommonFireFlowerShootProcUpdate)
NDS_INACTIVE_STATUS_STUB(ftCommonFireFlowerShootProcMap)
NDS_INACTIVE_STATUS_STUB(ftCommonFireFlowerShootAirProcMap)
NDS_INACTIVE_STATUS_STUB(ftCommonHammerWalkProcInterrupt)
NDS_INACTIVE_STATUS_STUB(ftCommonHammerTurnProcUpdate)
NDS_INACTIVE_STATUS_STUB(ftCommonHammerTurnProcInterrupt)
NDS_INACTIVE_STATUS_STUB(ftCommonHammerKneeBendProcUpdate)
NDS_INACTIVE_STATUS_STUB(ftCommonHammerKneeBendProcInterrupt)
NDS_INACTIVE_STATUS_STUB(ftCommonHammerFallProcInterrupt)
NDS_INACTIVE_STATUS_STUB(ftCommonHammerFallProcMap)
NDS_INACTIVE_STATUS_STUB(ftCommonHammerLandingProcUpdate)
/* P2-2 source-parity audit: these six callbacks are battle-reachable for
 * Mario/Fox through the normal guard-break path, so they may not be inert
 * fallbacks.  The source TUs are linked beside this file and own the complete
 * Fly -> Fall -> Down -> Stand -> FuraFura lifecycle. */
NDS_INACTIVE_STATUS_STUB(ftCommonThrownCommonStarProcMap)
#if !NDS_P2_CAPTAIN
/* P2-3f5: battleship_ftcommon_capturecaptain.c owns the real body wherever
 * Falcon is built. A strong definition wins over this weak twin at link either
 * way -- DK's four Special setters prove that on the shipped ELF -- but leaving
 * the twin unguarded would hide a build where the real TU silently did not
 * compile in, which is exactly the shape that has bitten this queue twice. */
NDS_INACTIVE_STATUS_STUB(ftCommonCaptureCaptainProcPhysics)
#endif
#if !NDS_P2_DONKEY
NDS_INACTIVE_STATUS_STUB(ftCommonCaptureShoulderedProcInterrupt)
#endif
#undef NDS_INACTIVE_STATUS_STUB
