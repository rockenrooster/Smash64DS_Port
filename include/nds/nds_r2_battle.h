#ifndef NDS_R2_BATTLE_H
#define NDS_R2_BATTLE_H

/* Runtime 2 battle path -- R2-01.
 *
 * docs/Smash64DS_Runtime2_SwitchPlan.md is the plan of record. This header is
 * the whole seam between src/nds/r2 and Runtime 1: src/nds/r2 owns battle scene
 * flow, Runtime 1 still owns the 60 Hz gameplay tick and the draw. Later phases
 * swap the renderer underneath without touching the loop again.
 *
 * The host side lives in src/port/taskman_seam.c under the same flag. Nothing
 * here exists when NDS_R2_PATH is 0.
 */

/* Task 46 header idiom: this header does NOT #include nds_build_config.h (it is
 * force-included via CFLAGS -include). If the flag is undefined the include
 * order is broken and the build must fail loudly rather than silently compile
 * the R2 path out -- this project has paid for silent no-ops. */
#ifndef NDS_R2_PATH
#error "nds_r2_battle.h needs nds_build_config.h (CFLAGS -include)"
#endif

#include <nds/nds_startup.h>

#if NDS_R2_PATH

/* Scene entry. Replaces the battle_playable arm of the NDS_DEV_SCENE_HARNESS
 * chain; runs the match to its natural end and returns. */
void ndsR2BattleRun(void);

/* Runtime 1 operations the R2 loop drives. */
void ndsR2HostBattlePrepare(void);
void ndsR2HostBattleIterationBegin(void);
u32 ndsR2HostBattleUpdateOnce(u32 update_index);
void ndsR2HostBattlePresent(void);
void ndsR2HostBattleFinish(void);
u32 ndsR2HostBattleNaturalMotionPassed(void);
u32 ndsR2HostBattleUpdatesPerPresent(void);
u32 ndsR2HostBattleUpdateMax(void);

#endif /* NDS_R2_PATH */

#endif /* NDS_R2_BATTLE_H */
