/* Compile the original BattleShip object-manager translation unit.
 * Provides real gcSetupObjman, gcRunAll, gcDrawAll, gcMakeGObjSPAfter,
 * gcAddGObjProcess, gcAddGObjDisplay, gcGetGObjSetNextAlloc, and the
 * free-list/object-pool bookkeeping used by startup scene setup. */

#include <sys/controller.h>

#include <nds/nds_controller.h>

/* R2-07 R3 bracket. `gcRunAll` IS the Results scene's `task_update`, so it is
 * the exact span between the controller publish (which measurably leaves
 * `button_tap` holding START) and `mnVSResultsCheckExit` inside it (which
 * measurably reads zero). The scene's setup table lives in a different
 * translation unit, so renaming the definition here leaves that table pointing
 * at this wrapper -- the same-file trap that makes this trick useless for
 * `mnVSResultsCheckExit` does not apply.
 *
 * Delete once the tap defect is closed; this is an instrument, not a seam. */
#define gcRunAll ndsBaseGcRunAll
#include "../../decomp/BattleShip-main/decomp/src/sys/objman.c"
#undef gcRunAll

volatile u32 gNdsGcRunAllTapAliveCount;
volatile u32 gNdsGcRunAllTapLostCount;
volatile u32 gNdsGcRunAllEntryTapMask;
volatile u32 gNdsGcRunAllExitTapMask;

void gcRunAll(void)
{
    u16 before = gSYControllerDevices[0].button_tap;
    u16 after;

    ndsBaseGcRunAll();
    after = gSYControllerDevices[0].button_tap;

    gNdsGcRunAllEntryTapMask |= before;
    gNdsGcRunAllExitTapMask |= after;
    if (before != 0u)
    {
        gNdsGcRunAllTapAliveCount++;
        if (after == 0u)
        {
            gNdsGcRunAllTapLostCount++;
        }
    }
}
