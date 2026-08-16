#ifndef NDS_FTANIM_TRACK_H
#define NDS_FTANIM_TRACK_H

/*
 * Task 3 stage 3 -- the dense fighter-animation runtime.
 *
 * `ndsR2FtAnimParseDObjFigatree` re-derives AObj fields from static FIGATREE
 * ROM data 96.9 times a marginal frame; the PARSE half of the animation lane
 * measures 33,951 tk/fr of deletable work at rank-80 on
 * `build-c192-sitr-profile-gxc` after the irreducible animation clock (7,426)
 * is taken out (`artifacts/performance/2026-08-15_ftanim-track-pack/`).
 *
 * The replacement is a representation change, not an arithmetic cut. The host
 * AOT compiler (`scripts/generate_ftanim_track_pack.py`) turns each joint
 * script into typed rows -- `u16 kind:4 | track mask:10 | has_frames:1 |
 * block:1`, an optional `u16` frame count, a relative `s16` jump for Loop, and
 * the AUTHORED `s16` target words -- and emits them in FIGATREE ENTRY ORDER, so
 * a bind is one array index rather than a search.
 *
 * WHAT A CONVERTED JOINT NO LONGER DOES, per stepped call:
 *   - decode an N64 command word through a 15-way jump table;
 *   - scan `command.flags` bit by bit to find its tracks;
 *   - rebuild `track_aobjs[10]` by walking the DObj's AObj linked list;
 *   - migrate AObj nodes to the Q representation;
 *   - compute `ftAnimGetTargetValue` per track.
 * The first four move to the bind (~18 a frame against 42 stepped calls), the
 * last becomes one shift from a per-track table.
 *
 * WHAT IT STILL DOES, deliberately: the animation clock, the `anim_wait` /
 * `anim_frame` / `parent_gobj->anim_frame` writes, the `AOBJ_ANIM_END` /
 * `AOBJ_ANIM_NULL` sentinels, the `func_anim(-1)` / `func_anim(-2)` call sites
 * and the AObj fields `gcPlayDObjAnimJoint` reads. Gameplay reads all of those
 * (`ftanimend.c:6`, eight lines of `ftmain.c`), so they are not the lane.
 *
 * ROUTE: `gNdsFtAnimTrackDispatch` (a `volatile` word, default
 * `NDS_R2_FTANIM_TRACK_DISPATCH`). At 0 every joint takes the generic parser on
 * the SAME binary and at the SAME addresses, so an A/B has no placement floor.
 */

#include <sys/objtypes.h>

#ifndef NDS_R2_FTANIM_TRACK
#define NDS_R2_FTANIM_TRACK 0
#endif
#ifndef NDS_R2_FTANIM_TRACK_DISPATCH
#define NDS_R2_FTANIM_TRACK_DISPATCH 1
#endif
#ifndef NDS_R2_FTANIM_TRACK_ORACLE
#define NDS_R2_FTANIM_TRACK_ORACLE 0
#endif

/* One clip: `entry_count` figatree entries starting at `entry_first` in the
 * entry word array. Emitted ascending by `asset_id` so the lookup is a binary
 * search. */
typedef struct NDSFtAnimTrackClip
{
    u16 asset_id;
    u16 entry_count;
    u16 entry_first;
} NDSFtAnimTrackClip;

#ifdef __cplusplus
extern "C" {
#endif

extern volatile u32 gNdsFtAnimTrackDispatch;
extern volatile u32 gNdsFtAnimTrackBinds;        /* joints bound dense */
extern volatile u32 gNdsFtAnimTrackBindMiss;     /* clip absent from the pack */
extern volatile u32 gNdsFtAnimTrackBindFull;     /* pool exhausted (fail open) */
extern volatile u32 gNdsFtAnimTrackSteps;        /* stepped calls, dense */
extern volatile u32 gNdsFtAnimTrackEarlyOut;     /* clock-only calls, dense */
extern volatile u32 gNdsFtAnimTrackRowsRun;      /* rows consumed */
extern volatile u32 gNdsFtAnimTrackOracleRows;   /* decision points compared */
extern volatile u32 gNdsFtAnimTrackOracleBad;    /* decision points that differ */
extern volatile u32 gNdsFtAnimTrackOracleFirst;  /* first failing (kind<<16|op) */

/* Called once per `lbCommonAddFighterPartsFigatree`, before the tree walk.
 * Returns the pool base for this fighter, or -1 when the clip is not in the
 * pack or the pool is full -- in which case every joint takes the generic
 * path, which is the shipped behaviour. */
s32 ndsFtAnimTrackBeginClip(DObj *root_dobj, const void *figatree);

/* Entry `index` of the clip opened by the matching `BeginClip`. TRUE when the
 * joint was bound dense (and `dobj->anim_joint.event16` now holds the joint's
 * dense cursor block); FALSE leaves the joint exactly as the generic bind left
 * it. */
s32 ndsFtAnimTrackBindJoint(DObj *dobj, s32 base, s32 index);

/* TRUE when `p` is one of this module's cursor blocks. One range compare. */
s32 ndsFtAnimTrackIsDense(const void *p);

/* The dense replacement for `ndsR2FtAnimParseDObjFigatree`. */
void ndsFtAnimTrackStep(DObj *root_dobj);

#ifdef __cplusplus
}
#endif

#endif /* NDS_FTANIM_TRACK_H */
