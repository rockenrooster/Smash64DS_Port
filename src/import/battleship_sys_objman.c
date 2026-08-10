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
/* Cycle 109: give the AObj free list the contiguous block BattleShip already
 * asks for, and that nothing has ever supplied.
 *
 * `gcSetupObjman` threads `setup->aobjs[0 .. aobjs_num-1]` into `sGCAnimHead`
 * in ascending address order (objman.c:2462-2475) -- the same pooling the
 * comment above describes for GObj thread stacks. `aobjs_num` is **zero in
 * every scene**: an `rg` over `src/`, `sc/` and `vs/` finds no writer at all.
 * So `sGCAnimHead` starts NULL and `gcGetAObjSetNextAlloc` takes its
 * one-AObj-at-a-time fallback (objman.c:640-645), meaning every AObj in the
 * game is an individual 36-byte `syTaskmanMalloc` interleaved with everything
 * else the scene allocates.
 *
 * That scatter is the measured top cost in BOTH hot animation functions. Per
 * the cycle-106 whole-match profile, walking the resulting linked list costs
 * `aobj->kind` **24.1 cyc/ex over 143,916 executions = 20.9% of
 * `gcPlayDObjAnimJoint`**, and `aobj->track` **29.6 cyc/ex = 6.7% of
 * `ftAnimParseDObjFigatree`** -- about 6.2M cycles of pointer chasing between
 * them, on a machine whose non-idle CPI is 2.85. 143,916 visits over ~400
 * census regions is ~360 live AObj; at 36 bytes that is 12,960 bytes against a
 * 4 KB D-cache, so the set can never be resident and each scattered node is a
 * fresh miss. Contiguity cannot make it resident either, but 36-byte nodes in
 * ascending order share 32-byte lines, so a sequential walk stops paying two
 * lines per node.
 *
 * This changes no struct, no data format and no arithmetic -- allocation
 * locality only, so behavior is bit-identical.
 *
 * Heap cost is roughly neutral by construction: it replaces ~360 individual
 * 36-byte allocations (each carrying allocator overhead) with one block of the
 * same count. Undersizing is safe and degrades exactly to today's behavior --
 * `gcGetAObjSetNextAlloc` still mallocs a single AObj when the list runs dry --
 * so the count below is sized from a real run rather than from this estimate.
 * `ndsR2AObjLiveCount()` returns decomp's own `sGCAnimsActiveNum`, so sampling
 * it at ring stops gives the live peak with no hot-path code and no new bytes
 * on the traversal itself -- 512 is the starting guess against ~360 observed,
 * and it should be trimmed to the measured peak plus margin. */
#define NDS_R2_AOBJ_POOL_COUNT 512

u32 gNdsR2AObjPoolCount;
u32 gNdsR2AObjPoolBytes;
u32 gNdsR2AObjPoolDeclines;

u32 ndsR2AObjLiveCount(void)
{
    return sGCAnimsActiveNum;
}

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
    if (ds_setup.aobjs_num == 0)
    {
        /* Re-carved every setup, deliberately. The arena is reset between
         * scenes, so a block cached across one would dangle -- this mirrors how
         * the stack pool is handled rather than holding a static pointer. */
        AObj *block = syTaskmanMalloc(
            sizeof(AObj) * (size_t)NDS_R2_AOBJ_POOL_COUNT, 0x4);

        if (block != NULL)
        {
            ds_setup.aobjs = block;
            ds_setup.aobjs_num = NDS_R2_AOBJ_POOL_COUNT;
            gNdsR2AObjPoolCount = NDS_R2_AOBJ_POOL_COUNT;
            gNdsR2AObjPoolBytes =
                (u32)(sizeof(AObj) * (size_t)NDS_R2_AOBJ_POOL_COUNT);
        }
        else
        {
            /* Arena too tight for the block: leave the scene exactly as it was
             * and let the per-AObj fallback run. Never a boot failure. */
            gNdsR2AObjPoolDeclines++;
        }
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
