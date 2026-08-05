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
#include <nds/nds_os.h>

#define gcRunAll ndsBaseGcRunAll
#define gcSetupObjman ndsBaseGcSetupObjman
#include "../../decomp/BattleShip-main/decomp/src/sys/objman.c"
#undef gcRunAll
#undef gcSetupObjman

extern void ndsBaseGcSetupObjman(GCSetup *setup);

/* Size the GObj thread stack pool for a DS coroutine, once, for every scene.
 *
 * This is the seam that makes the 2026-08-03 announcer freeze structurally
 * impossible. BattleShip gives a GObj thread 1536 bytes (`sizeof(u64) * 192`,
 * e.g. scvsbattle.c:47) drawn from the taskman arena at scene setup and
 * recycled through gcEjectGObjStack, and hands the top of that block to
 * `osCreateThread`. The port ignored it and mallocd a private 4 KiB coroutine
 * stack at the thread's FIRST START instead -- which, on a heap measured with
 * zero headroom, failed for the announcer at logic frame 390 and returned
 * silently while gcRunGObjProcess was already blocked in osRecvMesg waiting for
 * that thread to post. Raising the block here lets osCreateThread build the
 * coroutine in BattleShip's own pooled storage, so nothing is allocated at
 * thread-start time at all.
 *
 * `gcSetupObjman` is called from exactly one place -- taskman.c:1309, a
 * different translation unit -- so the rename interposes cleanly.
 *
 * Cost is arena, not heap, and the arena is where the headroom is: measured
 * 2026-08-03 at the frozen stop, arena headroom 149,840 bytes against a C heap
 * whose sbrk had reached its ceiling exactly. Roughly 2.7 KiB more per
 * concurrently live GObj thread, of which that battle had at most three.
 *
 * A setup that also preallocates the stack array is dropped rather than
 * re-carved: syTaskmanStartTask sized that array from the ORIGINAL stride, so
 * walking it at the raised one would run off the end. Every shipped setup
 * passes `gobjthreadstacks_num == 0` already, which makes gcGetGObjStackOfSize
 * grow the pool from the arena on demand -- the path this keeps. */
void gcSetupObjman(GCSetup *setup)
{
    GCSetup ds_setup = *setup;
    size_t needed = ndsOsGObjThreadBlockBytes();

    if (ds_setup.gobjthreadstack_size < needed)
    {
        ds_setup.gobjthreadstack_size = (needed + 7u) & ~(size_t)7u;
        ds_setup.gobjthreadstacks = NULL;
        ds_setup.gobjthreadstacks_num = 0;
    }
    ndsBaseGcSetupObjman(&ds_setup);
}

volatile u32 gNdsGcRunAllTapAliveCount;
volatile u32 gNdsGcRunAllTapLostCount;
volatile u32 gNdsGcRunAllEntryTapMask;
volatile u32 gNdsGcRunAllExitTapMask;

void gcRunAll(void)
{
    u16 before = gSYControllerDevices[0].button_tap;
    u16 after;

    /* Cycle 86 GCRA. This wrapper is the SOLE gateway to the whole simulation
     * inside the SRC bracket: decomp's scene update (scvsbattle.c:75) calls
     * ifCommonBattleUpdateInterfaceAll, whose game_status switch reaches
     * ifCommonBattleGoUpdateInterface, which ends here (ifcommon.c:2970). So the
     * span below is every GObj process -- both fighters' six-proc chains, the
     * camera, effects, items, weapons and interface objects -- and SBAS - GCRA
     * is exactly the work SRC does OUTSIDE the simulation. cpuGetTiming reads the
     * free-running timer pair and never resets it, so nesting this inside SRC's
     * own bracket is safe. Bracketing the existing port wrapper is what keeps
     * decomp/ untouched. */
#if NDS_TICK_HUD
    /* cpuGetTiming and the bucket global are forward-declared here rather than
     * reached by including nds/timers.h and nds/nds_startup.h. This TU's include
     * order is load bearing -- it compiles decomp's objman.c in place behind two
     * renames -- and two prototypes are a smaller change than two headers in
     * that chain. libnds nds/timers.h:255 is the authority for the signature;
     * include/nds/nds_startup.h is the authority for the global. Same technique
     * as battleship_lbparticle.c:1697. */
    extern u32 cpuGetTiming(void);
    extern volatile u32 gNdsTickHudSrcRunAllTicks;
    u32 runall_start = cpuGetTiming();
#endif

    ndsBaseGcRunAll();
#if NDS_TICK_HUD
    gNdsTickHudSrcRunAllTicks += cpuGetTiming() - runall_start;
#endif
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
