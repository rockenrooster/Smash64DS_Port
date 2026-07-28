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
 * instrumentation. `ndsR2Host*` are the Runtime 1 operations, unchanged, called
 * in the same order with the same arguments. R2-01's gate is tick-identical
 * gameplay and tick-HUD buckets that read the same as Runtime 1; a measurable
 * frame-cost change in EITHER direction is a defect in this file, not a win.
 */

/* This file is harness-specific by construction -- the folded constants below
 * are only constant under battle_playable -- so it binds to the canonical scene
 * harness config rather than inheriting the macros by luck.
 * check-harness-registry.ps1 enforces this pairing. */
#include "nds_scene_harness_config.h"

#include <nds/nds_scene_harness.h>
#include <nds/nds_r2_battle.h>

#if NDS_R2_PATH

#if NDS_DEV_SCENE_HARNESS != NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE
#error "src/nds/r2 is specialized for the battle_playable harness (mode 163)"
#endif

#if NDS_SCENE_MIP_CACHE_LAB
/* The Runtime 1 loop carries a mip-cache seeding excursion at i == 0. It is a
 * lab path (default 0) and deliberately has no R2 equivalent; failing closed is
 * correct, because silently dropping it would make the two paths differ in a
 * configuration nobody is watching. */
#error "NDS_R2_PATH does not implement the NDS_SCENE_MIP_CACHE_LAB seed path"
#endif

void ndsR2BattleRun(void)
{
    const u32 updates_per_present = ndsR2HostBattleUpdatesPerPresent();
    const u32 update_max = ndsR2HostBattleUpdateMax();
    u32 i = 0u;

    ndsR2HostBattlePrepare();

    while (i < update_max)
    {
        u32 update_in_iteration;
        u32 terminal_update = 0u;
        u32 stop_after_iteration = 0u;

        ndsR2HostBattleIterationBegin();

        /* Smash 64 slows uniformly under load and never repays a missed retrace
         * with a later logic burst. Run exactly `updates_per_present` unchanged
         * 60 Hz source ticks per presented frame; a three-VBlank draw is
         * measured as slowdown and the next frame starts cleanly. */
        for (update_in_iteration = 0u;
             update_in_iteration < updates_per_present;
             update_in_iteration++)
        {
            if (ndsR2HostBattleUpdateOnce(update_in_iteration) != 0u)
            {
                /* BattleShip syTaskmanRunTask checks LoadScene immediately
                 * after task_update and never draws the terminal update. */
                terminal_update = 1u;
                break;
            }
            i++;
            if (ndsR2HostBattleNaturalMotionPassed() != 0u)
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
        ndsR2HostBattlePresent();
        if (stop_after_iteration != 0u)
        {
            break;
        }
    }

    ndsR2HostBattleFinish();
}

#endif /* NDS_R2_PATH */
