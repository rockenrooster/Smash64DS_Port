/* Compile the original BattleShip controller translation unit unchanged, then
 * restore the one invariant the port had lost.
 *
 * `syControllerUpdateGlobalData` PUBLISHES the per-frame edge accumulators and
 * then CLEARS them (controller.c:177-183): `button_tap = unk04`, an assignment,
 * followed by `unk04 = unk08 = unk0C = 0`. A second publish in the same frame
 * therefore drains an already-empty accumulator and overwrites a real tap with
 * zero. On the original this cannot happen: game code never calls it directly,
 * it goes through `syControllerFuncRead` -> `syControllerParseEvent`, whose
 * `CONT_EVENT_UPDATE_GLOBAL_DATA` case publishes only when
 * `sSYControllerIsUpdateData` is set -- TRUE by a read, FALSE by a publish. The
 * flag IS the once-per-read interlock.
 *
 * The port bypasses it: nine sites across `taskman_seam.c`,
 * `reloc_backend_movement.c` and `reloc_backend_cliff_ledge.c` call
 * `syControllerReadDeviceData(); syControllerUpdateGlobalData();` directly, and
 * two of them can run in one frame. `button_hold` survives that (it is
 * `unk00`, which is not an accumulator and is not cleared) but `button_tap`,
 * `button_update` and `button_release` are destroyed -- which is why input
 * looked like it worked while every edge-triggered check silently failed.
 *
 * So keep the raw function for the decomp's own three call sites, which are all
 * either immediately preceded by a read (`CONT_EVENT_READ_CONT_DATA`) or
 * already flag-guarded (`CONT_EVENT_UPDATE_GLOBAL_DATA`, and the retrace path's
 * `sSYControllerWaitUpdate` handshake), and give the port the guarded one under
 * the public name. A single `read(); update();` pair behaves exactly as before;
 * only the second publish of the same read is suppressed, which is the original
 * behaviour restored, not a new one. */

#include <PR/os.h>
#include <ssb_types.h>

#include <nds/nds_controller.h>

#define syControllerUpdateGlobalData ndsSYControllerPublishGlobalData
#define syControllerReadDeviceData   ndsSYControllerSampleDeviceData
#include "../../decomp/BattleShip-main/decomp/src/sys/controller.c"
#undef syControllerUpdateGlobalData
#undef syControllerReadDeviceData

volatile u32 gNdsControllerPublishCount;
volatile u32 gNdsControllerPublishSuppressedCount;
volatile u32 gNdsControllerReadCount;
volatile u32 gNdsControllerEdgeSeenMask;
volatile u32 gNdsControllerPublishedTapMask;
volatile u32 gNdsControllerReadEdgeCount;
volatile u32 gNdsControllerPublishTapNonzeroCount;

/* Scope every counter above to one scene.
 *
 * They were run-global for exactly one measurement and that was already enough
 * to mislead: a soak that presses START on a schedule presses some of them while
 * the BATTLE is still running, so `EdgeSeenMask` and `PublishedTapMask` both came
 * back 0x1000 while the Results-scoped mask stayed 0. Sticky masks that span two
 * scenes answer "did this ever work anywhere", which is not the question. Results
 * calls this on its first tick, so afterwards every count and mask below is
 * Results only and divides cleanly by the Results tick count. */
void ndsControllerEdgeTelemetryReset(void)
{
    gNdsControllerPublishCount = 0u;
    gNdsControllerPublishSuppressedCount = 0u;
    gNdsControllerReadCount = 0u;
    gNdsControllerEdgeSeenMask = 0u;
    gNdsControllerPublishedTapMask = 0u;
    gNdsControllerReadEdgeCount = 0u;
    gNdsControllerPublishTapNonzeroCount = 0u;
}

/* Pass-through, but it records whether the rising edge is ever computed at all.
 * That is the one fact separating "the edge is produced and a second publish
 * eats it" from "`unk00` is already latched to the held value when the read
 * runs, so `unk02 = (button ^ unk00) & button` is structurally zero". Both
 * present identically as button_tap == 0 at the consumer. */
void syControllerReadDeviceData(void)
{
    ndsSYControllerSampleDeviceData();
    gNdsControllerReadCount++;
    gNdsControllerEdgeSeenMask |= sSYControllerDescs[0].unk02;
    if (sSYControllerDescs[0].unk02 != 0u)
    {
        gNdsControllerReadEdgeCount++;
    }
}

void syControllerUpdateGlobalData(void)
{
    if (sSYControllerIsUpdateData == FALSE)
    {
        /* No read since the last publish: the accumulators are already drained
         * and republishing would zero a live tap. */
        gNdsControllerPublishSuppressedCount++;
        return;
    }
    ndsSYControllerPublishGlobalData();
    gNdsControllerPublishCount++;
    gNdsControllerPublishedTapMask |= gSYControllerDevices[0].button_tap;
    if (gSYControllerDevices[0].button_tap != 0u)
    {
        /* Paired with the Results observer's own mask, this is the whole 2x2.
         * Nonzero here and zero at the observer means the tap IS delivered and
         * then destroyed inside `task_update`; zero here means it never survives
         * to the publish, i.e. the accumulator was drained by an earlier read in
         * the same iteration. */
        gNdsControllerPublishTapNonzeroCount++;
    }
}
