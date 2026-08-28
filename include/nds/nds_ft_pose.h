#ifndef NDS_FT_POSE_H
#define NDS_FT_POSE_H

/* P2-2p6 -- the fighter figatree pose engine (owner ruling 2026-08-23: "do
 * both" -- precomputed/compact pose evaluation AND 30 Hz pose evaluation).
 *
 * WHAT IT REPLACES. For every fighter joint bound to a figatree the source
 * runs, per 60 Hz tick, `ftAnimParseDObjFigatree` (the script cursor, a walk
 * of the joint's linked AObj list to rebuild a track table, one AObj per
 * written track) followed by `gcPlayDObjAnimJoint` (a second walk of the same
 * list, one Hermite/linear/step evaluation per node, one f32 store per track).
 * On the four-CPU arm that lane is ~170K ARM9 ticks per presented frame
 * (`artifacts/performance/2026-08-23_p2-2-fourcpu-profile128-promoted/`):
 * `gcPlayDObjAnimJoint` 38K, `ndsR2AnimValueQ` 36K, the parser 28K,
 * `ndsFTParamsInvalidateSubtree` 27K, `ftParamUpdateAnimKeys` 18K, the track
 * table rebuild 6K, and the list walks are memory stalls on 36-byte nodes
 * scattered through the arena.
 *
 * WHAT IT IS. The SAME script state machine and the SAME Q12 evaluator
 * (`ndsR2AnimEvalQ`, one body shared through `nds_anim_fixed.h`) over compact
 * per-fighter state: each bound joint owns a contiguous array of up to ten
 * `NdsFtPoseTrack`s (28 bytes, fixed point, no `next`, no list), indexed by a
 * ten-entry slot map the parser fills on a track's first write exactly where
 * the source would have called `gcAddAObjForDObj`. The script cursor, the
 * per-joint `anim_wait`/`anim_frame`/`anim_speed` clock and the GObj
 * `anim_frame` are the source's own DObj/GObj fields, read and written with
 * the source's own f32 arithmetic, so every frame-count the motion scripts
 * consume (`ms->script_wait`, `ftAnimEndCheckSetStatus`) is bit-identical.
 * The pose values are the Q12 values the shipped evaluator already produces.
 *
 * THE 30 Hz HALF (`NDS_FT_POSE_HOLD`). The clock runs every tick -- that is
 * gameplay frame data. The EVALUATION of the common (body) joints runs on the
 * last logic tick of each presented frame and on the tick a figatree is
 * attached (`ftMainSetStatus`'s own first play, the transition pose the owner
 * refused to lose on 2026-08-16), and is held on the other tick. The joints
 * below `nFTPartsJointCommonStart` -- TransN, XRotN, YRotN, the source's
 * materialised "hidden parts" that drive `fp->anim_vel` and the whole-body
 * rotation -- are evaluated every tick: `ftphysics.c:172/397-425` turns the
 * TransN delta into velocity, so holding it would change positions, not just a
 * hurtbox. The accepted deviation is exactly the board's: body hurtboxes read
 * one tick stale on the held tick. `ndsFtPoseEvalTick` is published by the
 * battle's update loop (`taskman_seam.c`).
 *
 * THE ORACLE (`NDS_FT_POSE_ORACLE`, lab only). The engine runs on a SHADOW
 * copy of every bound DObj and GObj while the generic path keeps the real
 * ones; after each tick the nine pose floats, the clock fields and the GObj
 * frame are compared bit for bit on the joints the engine evaluated. It is
 * the proof the rewrite is the same machine, not a screenshot. */

#include <ft/fighter.h>
#include <sys/objman.h>

#ifndef NDS_FT_POSE
#define NDS_FT_POSE 0
#endif
#ifndef NDS_FT_POSE_HOLD
#define NDS_FT_POSE_HOLD 0
#endif
#ifndef NDS_FT_POSE_ORACLE
#define NDS_FT_POSE_ORACLE 0
#endif

#define NDS_FT_POSE_TRACKS 10u
#define NDS_FT_POSE_FIGHTERS 4u
#define NDS_FT_POSE_NO_SLOT 0xffu
/* Tracks one fighter's current clip can hold. The roster-wide maximum per clip
 * is 127 (Kirby's FTKirbyAnim181; Mario 93, Fox 99, Luigi 81, DK 87 -- measured
 * over every AObj16 file in the bank, 2026-08-23), so 128 fits every clip of
 * every fighter with no runtime growth. A clip that needed more would count on
 * gNdsFtPoseTrackOverflow and animate through a scratch track, never crash. */
#define NDS_FT_POSE_POOL 128u

/* One joint track, 24 bytes. `length` is the Q12 phase. Keep the source
 * AObj's `length_invert` and `rate_base` as SEPARATE words: Cubic/Step read
 * `length_invert` (Q30 reciprocal / Q12 frame count), while Linear reads
 * `rate_linear_q` (Q16). This distinction is source-visible when a command
 * has no payload: ftAnimParseDObjFigatree deliberately leaves the untouched
 * field at its prior value. Samus Catch joint 33 is one such case
 * (SetValBlock TRAX|TRAZ with no payload), so aliasing the two fields makes
 * the default Q30 reciprocal look like an enormous Q16 linear rate.
 *
 * The four s16 words are the figatree's own authored arguments,
 * shifted into Q12 at evaluation by the track class (rotation <<3, translate
 * <<10/<<7, scale <<0) -- except the two non-shift classes (scale rates, whose
 * 2^-13 frac rounds, and TraI, whose frac is not a power of two), which the
 * parser stores already in Q12 because both fit an s16. The kernel therefore
 * sees the same Q12 integers the AObj path's `ndsR2AnimArgToQ` produced. `kind`
 * is the Q kind (5 Step, 6 Linear, 7 Cubic) or 0 for a slot the script created
 * but has not written a kind to (the source's nGCAnimKindNone). */
typedef struct NdsFtPoseTrack
{
    s32 length;
    s32 length_invert;
    s32 rate_linear_q;
    s16 value_base;
    s16 value_target;
    s16 rate_base;
    s16 rate_target;
    u8 kind;
    u8 track;
    u8 shift_val;
    u8 shift_rate;
} NdsFtPoseTrack;

typedef struct NdsFtPoseJoint
{
    DObj *dobj;                         /* the joint this entry animates (the
                                         * shadow copy under the oracle) */
    DObj *real;                         /* the live joint (== dobj outside the
                                         * oracle) */
    void *interpolate;                  /* TraI's SYInterpDesc, when bound */
    u8 active;                          /* any track created since the bind */
    u8 body;                            /* TRUE at/after nFTPartsJointCommonStart:
                                         * held on the 30 Hz off tick */
    u8 joint_id;                        /* fp->joints[] index, scale index */
    u8 pad;
    u8 slot_of_track[NDS_FT_POSE_TRACKS];   /* track id -> pool index, or
                                             * NO_SLOT */
    u16 last_eval;                      /* pose->tick of the last evaluation:
                                         * the held ticks are caught up from
                                         * here (see ndsFtPosePlay) */
    s32 wait_q;                         /* the joint's anim_wait, Q12 */
    s32 frame_q;                        /* the joint's anim_frame, Q12 */
} NdsFtPoseJoint;

typedef struct NdsFtPose
{
    GObj *gobj;                         /* owner, NULL while the slot is free */
    GObj *clock_gobj;                   /* gobj, or its shadow under the oracle */
    u32 heap_generation;                /* arena generation this was carved in */
    u32 capacity;                       /* joints[] length */
    u32 entry_count;                    /* entries bound by the last attach */
    u32 bound;                          /* a figatree is attached */
    u32 attach_pending;                 /* first update after a bind: evaluate */
    u32 body_evaluated;                 /* last update evaluated the body */
    u32 tick;                           /* updates since the bind */
    u32 pool_used;                      /* tracks handed out since the bind */
    u32 joint_mask_lo;                  /* bound fp->joints IDs 0..31 */
    u32 joint_mask_hi;                  /* bound fp->joints IDs 32..63 */
    s32 speed_q;                        /* this update's anim_speed, Q12 */
    s32 gobj_frame_q;                   /* the GObj anim_frame to publish */
    u32 gobj_frame_pending;             /* ... when a joint's clock wrote it */
    NdsFtPoseJoint *joints;             /* capacity entries */
    NdsFtPoseTrack *pool;               /* NDS_FT_POSE_POOL tracks */
} NdsFtPose;

/* Published by the battle update loop: 1 on the last source tick of a
 * presented frame (the pose the draw will show), 0 on the others. */
extern volatile u32 gNdsFtPoseEvalTick;

/* Counters. `used` so --gc-sections cannot drop them out from under a
 * verifier; every one is read by scripts/probe-fighter-anim-state.ps1 or the
 * oracle. */
extern volatile u32 gNdsFtPoseBinds;
extern volatile u32 gNdsFtPoseBindFull;
extern volatile u32 gNdsFtPoseUpdates;
extern volatile u32 gNdsFtPoseJointTicks;
extern volatile u32 gNdsFtPoseJointEvals;
extern volatile u32 gNdsFtPoseJointHolds;
extern volatile u32 gNdsFtPoseTrackEvals;
extern volatile u32 gNdsFtPoseStepped;
extern volatile u32 gNdsFtPoseUnbinds;
/* Slot ownership is deliberately separate from motion bind/unbind activity.
 * One live fighter can bind many motions into the same slot, and Release can
 * return an already-unbound slot.  These counters answer the fixed-pool
 * question directly instead of treating binds-unbinds as an ownership count. */
extern volatile u32 gNdsFtPoseSlotClaims;
extern volatile u32 gNdsFtPoseSlotReleases;
extern volatile u32 gNdsFtPoseSlotLive;
extern volatile u32 gNdsFtPoseSlotLiveMax;
extern volatile u32 gNdsFtPoseTrackOverflow;
extern volatile u32 gNdsFtPoseAObjLiveMax;
extern volatile u32 gNdsFtPoseOracleCompares;
extern volatile u32 gNdsFtPoseOracleMismatches;
extern volatile u32 gNdsFtPoseOracleFirstJoint;
extern volatile u32 gNdsFtPoseOracleFirstField;
extern volatile u32 gNdsFtPoseOracleFirstWant;
extern volatile u32 gNdsFtPoseOracleFirstGot;
extern volatile u32 gNdsFtPoseOracleFirstFrame;
/* Keep a second first-fault lane for pose fields 0..8.  The general oracle is
 * intentionally bit-exact and therefore records -0.0 vs +0.0 clock fields;
 * that useful strictness must not hide the first transform divergence when a
 * gameplay collision depends on the pose. */
extern volatile u32 gNdsFtPoseOraclePoseMismatches;
extern volatile u32 gNdsFtPoseOracleFirstPoseJoint;
extern volatile u32 gNdsFtPoseOracleFirstPoseField;
extern volatile u32 gNdsFtPoseOracleFirstPoseWant;
extern volatile u32 gNdsFtPoseOracleFirstPoseGot;
extern volatile u32 gNdsFtPoseOracleFirstPoseFrame;

/* Open an attach for the fighter owning `walk_root` (TopN->child): `count`
 * is the number of DObjs the caller's walk will visit. Returns TRUE when the
 * engine takes this attach (state carved from the arena on the fighter's
 * first attach, capacity count + 3 for the source's materialised hidden
 * parts); FALSE hands the attach to the generic path. */
sb32 ndsFtPoseBindBegin(DObj *walk_root, u32 count);

/* Bind entry `entry` (walk order) to `script`, the resolved AObjEvent16
 * script, or NULL for a NULL figatree entry. `anim_frame` is
 * ftMainSetStatus's frame_begin. Performs gcAddDObjAnimJoint's DObj setup
 * itself (kinds reset, cursor, CHANGED, frame) -- the admission test that
 * wrapper runs is what the caller already knows: this is a fighter figatree. */
void ndsFtPoseBindEntry(u32 entry, DObj *dobj, AObjEvent16 *script,
                        f32 anim_frame);

/* The figatree attach is complete: `entry_count` entries were bound. */
void ndsFtPoseBindEnd(u32 entry_count);

/* Something other than a figatree attach re-targeted this fighter's joint
 * animation (the event32 shield/guard pose); the engine steps aside until the
 * next figatree bind. */
void ndsFtPoseUnbind(GObj *gobj);

/* The fighter is being destroyed. Unlike Unbind, which retains ownership for
 * a live fighter whose joints were temporarily re-targeted, Release returns
 * the pose slot to the same-scene free pool so CSS destroy/rebuild cycles can
 * reuse its arena-backed storage. */
void ndsFtPoseRelease(GObj *gobj);

/* The per-tick update for one fighter. Returns TRUE when the engine owns this
 * fighter's figatree animation this tick (the caller must then skip the
 * generic parse/play and run only the MObj material animations), FALSE when
 * the generic path must run. `translate_scales` is ftParamUpdateAnimKeys'
 * own resolution of `fp->attr->translate_scales`, or NULL. */
sb32 ndsFtPoseUpdate(GObj *gobj, FTStruct *fp, Vec3f *translate_scales);

/* TRUE only for an indexed fighter joint that belongs to the current compact
 * figatree bind. BattleShip ATTACHES by TopN hierarchy walk but PLAYS by the
 * complete fp->joints[] table (ftparam.c:380/406). Hidden / alternate joints
 * outside the hierarchy therefore remain generic-player owned even while the
 * compact engine owns the hierarchy. */
sb32 ndsFtPoseOwnsJoint(GObj *gobj, u32 joint_id);

/* Re-apply the current pose without advancing (ftParamUpdateAnimKeys'
 * `motion_id == -2` arm, which the source plays with anim_wait forced to END).
 * Returns TRUE when the engine owns the fighter. */
sb32 ndsFtPoseReapply(GObj *gobj, FTStruct *fp, Vec3f *translate_scales);

#if NDS_FT_POSE_ORACLE
/* Lab: after the generic path has run on the live joints, compare them with
 * the engine's shadows (counters above). */
void ndsFtPoseOracleAfter(GObj *gobj);
#endif

/* TRUE when the engine evaluated this fighter's body pose on the current tick
 * (or the fighter is not engine-owned); FALSE on a held tick, which lets
 * ftParamsUpdateFighterPartsTransform skip its invalidation walk. */
sb32 ndsFtPoseBodyChangedThisTick(GObj *gobj);

#endif /* NDS_FT_POSE_H */
