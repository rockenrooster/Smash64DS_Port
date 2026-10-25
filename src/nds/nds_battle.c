/*
 * Runtime 2 -- battle scene flow (R2-01).
 *
 * docs/Smash64DS_Runtime2_SwitchPlan.md is the plan of record. R2-01's job is
 * to isolate the scene-flow seam ONCE, so R2-02 (Dream Land direct runtime) and
 * R2-03 (fighter direct draw) can swap renderers under a stable roof instead of
 * each re-deriving where the battle loop lives.
 *
 * What this is
 * ------------
 * The Runtime 1 battle loop lives inside a ~30-condition
 * `#if NDS_DEV_SCENE_HARNESS == ...` chain in src/port/taskman_seam.c, shared
 * between battle_playable and every mp-collision development harness. It
 * carries two runtime flags -- `is_battle_playable` and
 * `use_realtime_presentation` -- and branches on them roughly a dozen times per
 * iteration.
 *
 * For the Boundary configuration (battle_playable_realtime, mode 163,
 * NDS_HARNESS_FAST_LOGIC == 0) BOTH flags are compile-time constants: 1 and 1.
 * This file is that loop with the constants folded, which is Runtime 2's whole
 * thesis applied to the smallest possible subject -- the discovery is deleted,
 * the behavior is not. Every branch removed here was provably not taken in the
 * shipped configuration.
 *
 * What this is NOT
 * ----------------
 * It does not change the gameplay tick, the renderer, the pacing policy, or the
 * instrumentation. `ndsHost*` are the Runtime 1 operations, unchanged, called
 * in the same order with the same arguments. R2-01's gate is tick-identical
 * gameplay and tick-HUD buckets that read the same as Runtime 1; a measurable
 * frame-cost change in EITHER direction is a defect in this file, not a win.
 */

/* This file is harness-specific by construction -- the folded constants below
 * are only constant under battle_playable -- so it binds to the canonical scene
 * harness config rather than inheriting the macros by luck. */
#include "nds_scene_harness_config.h"

#include <nds/nds_scene_harness.h>
#include <nds/nds_battle.h>
#include <nds/nds_freeze_diagnostics.h>
#include <nds/nds_net_session.h>
#include <port/coroutine.h>

extern volatile u32 gNdsFtPoseEvalTick;
extern volatile u32 gNdsMObjTickMul;

#if NDS_PATH

#if NDS_DEV_SCENE_HARNESS != NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE
#error "src/nds/r2 is specialized for the battle_playable harness (mode 163)"
#endif


/* P2-2p8 (2026-09-27): each source tick runs on the DTCM hot stack
 * (src/port/coroutine.c). The top 1.5 KB of the gameplay coroutine's
 * main-RAM stack -- the update's own base frames -- took ~525 of the
 * frame's ~637 stack line fills: every return into a frame whose line
 * the update's data had evicted. The tick does no I/O into a stack buffer
 * (storage reads bounce any destination outside main RAM:
 * ndsAudioStorageReadCard) and hands no stack address to DMA or the ARM7;
 * a coroutine it switches to runs any hot-stack call of its own in place
 * (gNdsDtcmHotStackBusy). Its deepest reach stays under the draw
 * subtrees' 3,740 B. */
static unsigned int ndsBattleUpdateOnHotStack(void *arg)
{
    return ndsHostBattleUpdateOnce((u32)(uintptr_t)arg);
}

void ndsBattleRun(void)
{
    const u32 updates_per_present = ndsHostBattleUpdatesPerPresent();
    const u32 update_max = ndsHostBattleUpdateMax();
    u32 i = 0u;

    ndsHostBattlePrepare();
    /* P3: a lockstep match runs a batch only once every human's records for
     * both of its ticks are here, and installs them tick by tick. */
    ndsNetBattleBegin();

    while (i < update_max)
    {
        u32 update_in_iteration;
        u32 terminal_update = 0u;
        u32 stop_after_iteration = 0u;

        ndsHostBattleIterationBegin();
        ndsNetBattleGate();

        /* Smash 64 slows uniformly under load and never repays a missed retrace
         * with a later logic burst. Run exactly `updates_per_present` unchanged
         * 60 Hz source ticks per presented frame; a three-VBlank draw is
         * measured as slowdown and the next frame starts cleanly. */
        for (update_in_iteration = 0u;
             update_in_iteration < updates_per_present;
             update_in_iteration++)
        {
            ndsNetBattleInstallTick(update_in_iteration);
            if (ndsDtcmHotStackRun(ndsBattleUpdateOnHotStack,
                                   (void *)(uintptr_t)update_in_iteration) !=
                0u)
            {
                /* BattleShip syTaskmanRunTask checks LoadScene immediately
                 * after task_update and never draws the terminal update. */
                terminal_update = 1u;
                break;
            }
            i++;
            if (ndsHostBattleNaturalMotionPassed() != 0u)
            {
                if (NDS_DEV_LIVE_INPUT_PREVIEW != 0)
                {
                    continue;
                }
                if (i < update_max)
                {
                    continue;
                }
                stop_after_iteration = 1u;
                break;
            }
        }

        if (terminal_update != 0u)
        {
            break;
        }
        ndsNetBattleBatchDone();
        ndsHostBattlePresent();
        if (stop_after_iteration != 0u)
        {
            break;
        }
    }

    ndsNetBattleEnd();
    /* The freeze watchdog watches battle frames only (Results and the menus
     * load between frames for seconds). */
    NDS_FREEZE_DIAGNOSTICS_DISARM();
    ndsHostBattleFinish();
    /* P2-2p6: outside this loop every source tick is a presented one for the
     * fighter pose engine (menu previews, Results), so leave its eval word set. */
    gNdsFtPoseEvalTick = 1u;
    /* ... and material animations at the source rate. */
    gNdsMObjTickMul = 1u;
}

#endif /* NDS_PATH */
