#include <nds/nds_ft_pose.h>

/* `used` on every counter: --gc-sections has removed diagnostic globals here
 * before and Boundary reported "Missing ELF symbol" rather than a flag. */
#define NDS_FT_POSE_COUNTER(name) \
    __attribute__((used)) volatile u32 name

NDS_FT_POSE_COUNTER(gNdsFtPoseEvalTick) = 1u;
NDS_FT_POSE_COUNTER(gNdsFtPoseBinds);
NDS_FT_POSE_COUNTER(gNdsFtPoseBindFull);
NDS_FT_POSE_COUNTER(gNdsFtPoseUpdates);
NDS_FT_POSE_COUNTER(gNdsFtPoseJointTicks);
NDS_FT_POSE_COUNTER(gNdsFtPoseJointEvals);
NDS_FT_POSE_COUNTER(gNdsFtPoseJointHolds);
NDS_FT_POSE_COUNTER(gNdsFtPoseTrackEvals);
NDS_FT_POSE_COUNTER(gNdsFtPoseStepped);
NDS_FT_POSE_COUNTER(gNdsFtPoseUnbinds);
NDS_FT_POSE_COUNTER(gNdsFtPoseSlotClaims);
NDS_FT_POSE_COUNTER(gNdsFtPoseSlotReleases);
NDS_FT_POSE_COUNTER(gNdsFtPoseSlotLive);
NDS_FT_POSE_COUNTER(gNdsFtPoseSlotLiveMax);
NDS_FT_POSE_COUNTER(gNdsFtPoseTrackOverflow);
NDS_FT_POSE_COUNTER(gNdsFtPoseAObjLiveMax);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleCompares);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleMismatches);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleFirstJoint);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleFirstField);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleFirstWant);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleFirstGot);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleFirstFrame);
NDS_FT_POSE_COUNTER(gNdsFtPoseOraclePoseMismatches);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleFirstPoseJoint);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleFirstPoseField);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleFirstPoseWant);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleFirstPoseGot);
NDS_FT_POSE_COUNTER(gNdsFtPoseOracleFirstPoseFrame);

#if NDS_FT_POSE

#include <stddef.h>
#include <string.h>
#include <sys/taskman.h>
#include <nds/nds_fcmp.h>
#include <nds/nds_anim_fixed.h>
#include <nds/nds_startup.h>

/* The runaway accounting the generic parser already keeps: a malformed script
 * is the same fault here, on the same counters, with its own mask bit. */
extern volatile u32 gNdsObjAnimRunawayCount;
extern volatile u32 gNdsObjAnimRunawayMask;
extern volatile u32 gNdsObjAnimRunawayScript;
extern volatile u32 gNdsObjAnimRunawayOpcode;
#define NDS_FT_POSE_RUNAWAY_BIT (1u << 8u)
#define NDS_FT_POSE_EVENT_LIMIT 4096u

/* `battleship_ftanim.c`'s Q writers, exported so the values a track carries
 * are the parser's own words. */
s32 ndsR2FtAnimTargetQ(s16 arg, s32 track, sb32 value_or_step);
s32 ndsR2FtAnimRecipQ30(u32 n);
/* decomp sys/interp.c, the TraI spline. */
void syInterpCubic(Vec3f *out, void *desc, f32 t);

/* The live AObj count, for sizing the AObj pool the engine displaces. */
u32 ndsR2AObjLiveCount(void);

static NdsFtPose sNdsFtPose[NDS_FT_POSE_FIGHTERS];
static NdsFtPose *sNdsFtPoseBinding;
/* Where an overflowing clip's tracks go: animated, evaluated, never stored
 * anywhere a second track could read, so a malformed or oversized clip degrades
 * to a held joint rather than a wild pointer. */
static NdsFtPoseTrack sNdsFtPoseScratchTrack;

#define NDS_FT_POSE_KIND_NONE 0u
#define NDS_FT_POSE_HIDDEN_SLACK 3u
/* `dobj->anim_wait` while a joint is running under the engine. THE CLOCK LIVES
 * IN THE JOINT (`wait_q`/`frame_q`, Q12 integers); the DObj field only carries
 * the source's three sentinels for the code that tests them -- the guard's
 * `!= AOBJ_ANIM_NULL`, ftParamUpdateAnimKeys' idle skip, the END hand-off --
 * and this non-sentinel value otherwise. Nothing outside the parser reads the
 * numeric anim_wait/anim_frame of a fighter joint; the GObj anim_frame the
 * motion scripts read is published once per update from the last joint
 * clock that wrote it, exactly the source's last-writer value.
 *
 * The Q12 clock is exact for every speed the source uses -- 1, 0.5, 1.5, 2,
 * ratios of small integers like the rebound's `rebound_anim_length /
 * attack_rebound` -- because a command boundary is crossed when the
 * accumulated speed reaches an integer wait, and with rational speeds of small
 * denominator the residues are multiples of 1/denominator, never within Q12's
 * 2.4e-4 of zero without being zero. The f32 chain it replaces cost two
 * soft-float helpers per joint per tick (~21K tk/fr on the four-CPU arm). */
#define NDS_FT_POSE_RUNNING 1.0F

/* `NDS_R2_AQ_VF - k` per track class, the shifts `ndsR2AnimArgToQ` applies:
 * values 1/512, 1/4, 1/4096 and rates 1/512, 1/32, 1/8192 read as powers of
 * two. Scale rates (2^-13, a negative shift that rounds) and TraI (not a power
 * of two) are stored pre-converted, so their eval shift is 0. */
static const u8 sNdsFtPoseShiftVal[NDS_FT_POSE_TRACKS] =
    { 3u, 3u, 3u, 0u, 10u, 10u, 10u, 0u, 0u, 0u };
static const u8 sNdsFtPoseShiftRate[NDS_FT_POSE_TRACKS] =
    { 3u, 3u, 3u, 0u, 7u, 7u, 7u, 0u, 0u, 0u };
/* Byte offset of each track's DObj pose float, in nGCAnimTrack order from
 * JointStart; TraI (slot 3) takes the interpolation arm and never reads it. */
static const u8 sNdsFtPoseStoreOffset[NDS_FT_POSE_TRACKS] =
{
    (u8)offsetof(DObj, rotate.vec.f.x),
    (u8)offsetof(DObj, rotate.vec.f.y),
    (u8)offsetof(DObj, rotate.vec.f.z),
    0u,
    (u8)offsetof(DObj, translate.vec.f.x),
    (u8)offsetof(DObj, translate.vec.f.y),
    (u8)offsetof(DObj, translate.vec.f.z),
    (u8)offsetof(DObj, scale.vec.f.x),
    (u8)offsetof(DObj, scale.vec.f.y),
    (u8)offsetof(DObj, scale.vec.f.z)
};

/* ---- lookup -------------------------------------------------------------- */

static void ndsFtPosePublishSlotOwnership(void)
{
    u32 live = 0u;
    u32 i;

    /* Arena generations retire the backing store wholesale.  Only slots whose
     * owner belongs to the CURRENT generation count as live; stale entries are
     * reusable by ndsFtPoseOpen and are not a leak. */
    for (i = 0u; i < NDS_FT_POSE_FIGHTERS; i++)
    {
        const NdsFtPose *pose = &sNdsFtPose[i];

        if ((pose->gobj != NULL) &&
            (pose->heap_generation == gNdsTaskmanHeapGeneration))
        {
            live++;
        }
    }
    gNdsFtPoseSlotLive = live;
    if (live > gNdsFtPoseSlotLiveMax)
    {
        gNdsFtPoseSlotLiveMax = live;
    }
}

static NdsFtPose *ndsFtPoseFind(const GObj *gobj)
{
    u32 i;

    if (gobj == NULL)
    {
        return NULL;
    }
    for (i = 0u; i < NDS_FT_POSE_FIGHTERS; i++)
    {
        NdsFtPose *pose = &sNdsFtPose[i];

        if ((pose->gobj == gobj) &&
            (pose->heap_generation == gNdsTaskmanHeapGeneration))
        {
            return pose;
        }
    }
    return NULL;
}

static NdsFtPose *ndsFtPoseOpen(GObj *gobj, u32 count)
{
    NdsFtPose *pose = ndsFtPoseFind(gobj);
    u32 capacity;
    u32 i;

    /* Also refresh the published live count after a scene-generation change,
     * before any stale slot is recycled below. */
    ndsFtPosePublishSlotOwnership();
    if (pose != NULL)
    {
        return (count <= pose->capacity) ? pose : NULL;
    }

    /* CSS preview fighters are destroyed and rebuilt repeatedly inside one
     * taskman arena generation. Reuse a released slot's backing storage before
     * allocating again, or those rebuilds consume the fixed pose slots and
     * eventually fall back to the much smaller generic AObj pool. */
    for (i = 0u; i < NDS_FT_POSE_FIGHTERS; i++)
    {
        pose = &sNdsFtPose[i];

        if ((pose->gobj == NULL) &&
            (pose->heap_generation == gNdsTaskmanHeapGeneration) &&
            (pose->joints != NULL) && (pose->pool != NULL) &&
            (count <= pose->capacity))
        {
            pose->gobj = gobj;
#if NDS_FT_POSE_ORACLE
            if (pose->clock_gobj == NULL)
            {
                pose->gobj = NULL;
                return NULL;
            }
            *pose->clock_gobj = *gobj;
#else
            pose->clock_gobj = gobj;
#endif
            pose->entry_count = 0u;
            pose->bound = 0u;
            pose->attach_pending = 0u;
            pose->body_evaluated = 1u;
            pose->pool_used = 0u;
            pose->joint_mask_lo = 0u;
            pose->joint_mask_hi = 0u;
            gNdsFtPoseSlotClaims++;
            ndsFtPosePublishSlotOwnership();
            return pose;
        }
    }

    /* A free slot, or one whose arena generation has been rewound out from
     * under it: both are reusable, the latter because its joints[] pointer is
     * dangling into a scene that no longer exists. */
    for (i = 0u; i < NDS_FT_POSE_FIGHTERS; i++)
    {
        pose = &sNdsFtPose[i];
        if ((pose->gobj == NULL) ||
            (pose->heap_generation != gNdsTaskmanHeapGeneration))
        {
            break;
        }
        pose = NULL;
    }
    if (pose == NULL)
    {
        return NULL;
    }
    capacity = count + NDS_FT_POSE_HIDDEN_SLACK;
    /* Scene-lifetime, from the taskman general heap exactly where the AObj
     * nodes this replaces would have been carved; rewound with the scene. The
     * AObj pool (`NDS_R2_AOBJ_POOL_COUNT`) shrinks by the same measure. */
    pose->joints = syTaskmanMalloc(sizeof(NdsFtPoseJoint) * capacity, 0x4u);
    pose->pool = syTaskmanMalloc(sizeof(NdsFtPoseTrack) * NDS_FT_POSE_POOL,
                                 0x4u);
    if ((pose->joints == NULL) || (pose->pool == NULL))
    {
        return NULL;
    }
    memset(pose->joints, 0, sizeof(NdsFtPoseJoint) * capacity);
    pose->pool_used = 0u;
    pose->gobj = gobj;
    pose->clock_gobj = gobj;
    pose->heap_generation = gNdsTaskmanHeapGeneration;
    pose->capacity = capacity;
    pose->entry_count = 0u;
    pose->bound = 0u;
    pose->attach_pending = 0u;
    pose->body_evaluated = 1u;
    pose->joint_mask_lo = 0u;
    pose->joint_mask_hi = 0u;
#if NDS_FT_POSE_ORACLE
    {
        /* The oracle's shadows: one DObj per entry plus the GObj whose
         * anim_frame the parser writes. Lab memory, arena-carved like the
         * state itself. */
        DObj *shadows = syTaskmanMalloc(sizeof(DObj) * capacity, 0x4u);
        GObj *shadow_gobj = syTaskmanMalloc(sizeof(GObj), 0x4u);

        if ((shadows == NULL) || (shadow_gobj == NULL))
        {
            pose->gobj = NULL;
            return NULL;
        }
        memset(shadows, 0, sizeof(DObj) * capacity);
        for (i = 0u; i < capacity; i++)
        {
            pose->joints[i].dobj = &shadows[i];
        }
        *shadow_gobj = *gobj;
        pose->clock_gobj = shadow_gobj;
    }
#endif
    gNdsFtPoseSlotClaims++;
    ndsFtPosePublishSlotOwnership();
    return pose;
}

/* ---- bind ---------------------------------------------------------------- */

sb32 ndsFtPoseBindBegin(DObj *walk_root, u32 count)
{
    GObj *gobj = (walk_root != NULL) ? walk_root->parent_gobj : NULL;
    NdsFtPose *pose = (gobj != NULL) ? ndsFtPoseOpen(gobj, count) : NULL;

    sNdsFtPoseBinding = pose;
    if (pose == NULL)
    {
        gNdsFtPoseBindFull++;
        return FALSE;
    }
    pose->bound = 0u;
    pose->entry_count = 0u;
    pose->pool_used = 0u;
    pose->joint_mask_lo = 0u;
    pose->joint_mask_hi = 0u;
    pose->tick = 0u;
#if NDS_FT_POSE_ORACLE
    /* lbCommonAddFighterPartsFigatree has just written frame_begin into the
     * live GObj; the shadow clock starts from the same value, or the attach
     * tick -- whose CHANGED branch does not write the GObj -- reads a stale
     * frame on the shadow and the oracle flags its own bookkeeping. */
    pose->clock_gobj->anim_frame = gobj->anim_frame;
#endif
    gNdsFtPoseBinds++;
    return TRUE;
}

void ndsFtPoseBindEntry(u32 entry, DObj *dobj, AObjEvent16 *script,
                        f32 anim_frame)
{
    NdsFtPose *pose = sNdsFtPoseBinding;
    NdsFtPoseJoint *joint;
    FTParts *parts;
    AObj *aobj;
    u32 i;

    if ((pose == NULL) || (entry >= pose->capacity) || (dobj == NULL))
    {
        return;
    }
    joint = &pose->joints[entry];
    parts = ftGetParts(dobj);
    joint->real = dobj;
#if !NDS_FT_POSE_ORACLE
    joint->dobj = dobj;
#endif
    joint->active = 0u;
    joint->last_eval = 0u;
    joint->interpolate = NULL;
    joint->joint_id = (parts != NULL) ? parts->joint_id : 0u;
    joint->body = ((parts == NULL) ||
                   (parts->joint_id >= nFTPartsJointCommonStart)) ? 1u : 0u;
    if (parts != NULL)
    {
        if (parts->joint_id < 32u)
        {
            pose->joint_mask_lo |= 1u << parts->joint_id;
        }
        else if (parts->joint_id < 64u)
        {
            pose->joint_mask_hi |= 1u << (parts->joint_id - 32u);
        }
    }
    for (i = 0u; i < NDS_FT_POSE_TRACKS; i++)
    {
        joint->slot_of_track[i] = NDS_FT_POSE_NO_SLOT;
    }
    if (script == NULL)
    {
        /* lbcommon.c:757's NULL entry: the joint simply does not animate. */
        dobj->anim_wait = AOBJ_ANIM_NULL;
#if NDS_FT_POSE_ORACLE
        *joint->dobj = *dobj;
        joint->dobj->parent_gobj = pose->clock_gobj;
#endif
        return;
    }
    /* gcAddDObjAnimJoint (sys/objanim.c:137), minus the port wrapper's
     * admission test: a fighter figatree is what this is by construction. The
     * AObj kinds are still reset so a generic player reaching one of these
     * joints finds nothing to evaluate. */
    for (aobj = dobj->aobj; aobj != NULL; aobj = aobj->next)
    {
        aobj->kind = nGCAnimKindNone;
    }
    dobj->anim_joint.event16 = script;
    dobj->anim_wait = AOBJ_ANIM_CHANGED;
    dobj->anim_frame = anim_frame;
    joint->frame_q = ndsR2F32ToFixed(anim_frame, NDS_R2_AQ_LF);
    joint->wait_q = 0;
#if NDS_FT_POSE_ORACLE
    *joint->dobj = *dobj;
    joint->dobj->parent_gobj = pose->clock_gobj;
#endif
}

void ndsFtPoseBindEnd(u32 entry_count)
{
    NdsFtPose *pose = sNdsFtPoseBinding;

    if (pose == NULL)
    {
        return;
    }
    pose->entry_count =
        (entry_count <= pose->capacity) ? entry_count : pose->capacity;
    pose->bound = 1u;
    pose->attach_pending = 1u;
    sNdsFtPoseBinding = NULL;
}

void ndsFtPoseUnbind(GObj *gobj)
{
    NdsFtPose *pose = ndsFtPoseFind(gobj);

    if ((pose != NULL) && (pose->bound != 0u))
    {
        pose->bound = 0u;
        gNdsFtPoseUnbinds++;
    }
}

void ndsFtPoseRelease(GObj *gobj)
{
    NdsFtPose *pose = ndsFtPoseFind(gobj);

    if (pose == NULL)
    {
        return;
    }
    if (pose->bound != 0u)
    {
        pose->bound = 0u;
        gNdsFtPoseUnbinds++;
    }
    pose->entry_count = 0u;
    pose->attach_pending = 0u;
    pose->pool_used = 0u;
    pose->joint_mask_lo = 0u;
    pose->joint_mask_hi = 0u;
    /* `gobj == NULL` is the NdsFtPose free-slot contract. Keep joints/pool and
     * oracle shadows resident so the next CSS rebuild reuses their arena. */
    pose->gobj = NULL;
    gNdsFtPoseSlotReleases++;
    ndsFtPosePublishSlotOwnership();
}

/* ---- the script cursor (ft/ftanim.c:73, in the port's Q form) ------------ */

/* Exactly (f32)v for v < 2^16 -- `ndsR2U16ToF32` in battleship_ftanim.c. */
static inline f32 ndsFtPoseU16ToF32(u32 v)
{
    u32 shift;
    u32 bits;
    f32 out;

    if (v == 0u)
    {
        return 0.0f;
    }
    shift = (u32)__builtin_clz(v);
    bits = (((127u + 31u) - shift) << 23) | (((v << shift) & 0x7fffffffu) >> 8);
    __builtin_memcpy(&out, &bits, sizeof(out));
    return out;
}

/* The slot a track writes through: gcAddAObjForDObj's creation site, with the
 * fresh node's `length_invert = 1.0F` carried in the Q form the converter in
 * battleship_ftanim.c writes for a kind-None AObj. */
static inline NdsFtPoseTrack *ndsFtPoseTrackFor(NdsFtPose *pose,
                                                NdsFtPoseJoint *joint, u32 i)
{
    u32 slot = joint->slot_of_track[i];
    NdsFtPoseTrack *t;

    if (slot != NDS_FT_POSE_NO_SLOT)
    {
        return &pose->pool[slot];
    }
    slot = pose->pool_used;
    if (slot >= NDS_FT_POSE_POOL)
    {
        gNdsFtPoseTrackOverflow++;
        t = &sNdsFtPoseScratchTrack;
    }
    else
    {
        pose->pool_used = slot + 1u;
        joint->slot_of_track[i] = (u8)slot;
        joint->active = 1u;
        t = &pose->pool[slot];
    }
    t->kind = NDS_FT_POSE_KIND_NONE;
    t->track = (u8)(i + nGCAnimTrackJointStart);
    t->shift_val = sNdsFtPoseShiftVal[i];
    t->shift_rate = sNdsFtPoseShiftRate[i];
    t->length = 0;
    /* gcAddAObjForDObj (decomp sys/objman.c:1187): these are distinct source
     * fields. A zero-payload command preserves whichever one it does not
     * write, so they cannot share storage in the compact player. */
    t->length_invert = 1 << NDS_R2_AQ_IF;
    t->rate_linear_q = 0;
    t->value_base = 0;
    t->value_target = 0;
    t->rate_base = 0;
    t->rate_target = 0;
    return t;
}

/* The authored s16 argument as the track stores it: raw for the power-of-two
 * classes, already Q12 for scale rates and TraI (see sNdsFtPoseShift*). */
static inline s16 ndsFtPoseArg(s16 arg, u32 i, sb32 value_or_step)
{
    if ((i == (u32)(nGCAnimTrackTraI - nGCAnimTrackJointStart)) ||
        ((value_or_step != 0) && (i >= 7u)))
    {
        return (s16)ndsR2FtAnimTargetQ(arg, (s32)i + nGCAnimTrackJointStart,
                                       value_or_step);
    }
    return arg;
}

/* `-anim_wait - anim_speed`, the phase a new segment starts at, minus the
 * catch-up the held ticks will add at the next evaluation.
 *
 * THE LAZY HOLD. On a held tick the player does not run at all -- no pool
 * line is touched -- and the next evaluation adds `speed x (tick - last_eval)`
 * to every live track, which is exactly the per-tick `length += speed` the
 * source would have done on each of those ticks. A track (re)written by the
 * parser DURING the hold at tick W expects adds for ticks W..C only, so the
 * uniform catch-up over-adds by `(W - last_eval - 1)` speeds; subtracting
 * those here at the write keeps the phase the source computes at C, exactly,
 * for every speed the Q12 clock represents. On an evaluated-every-tick joint
 * (W == last_eval + 1) the fold is zero and this is the parser's own formula. */
static inline s32 ndsFtPoseSegmentStart(const NdsFtPose *pose,
                                        const NdsFtPoseJoint *joint)
{
    s32 seg = -joint->wait_q - pose->speed_q;
    u32 held = (pose->tick - (u32)joint->last_eval) & 0xffffu;

    if (held > 1u)
    {
        seg -= pose->speed_q * (s32)(held - 1u);
    }
    return seg;
}

/* `anim_speed + anim_wait` over every live track: the script-exhausted tail. */
static void ndsFtPoseAdvanceTail(NdsFtPose *pose, NdsFtPoseJoint *joint)
{
    s32 tail_q = pose->speed_q + joint->wait_q;
    u32 held = (pose->tick - (u32)joint->last_eval) & 0xffffu;
    u32 i;

    /* The End tick's player adds nothing, so the ticks a held joint missed
     * are folded into the tail here -- the lazy hold's catch-up never runs on
     * an ended joint (see ndsFtPosePlay). `held` is 1 on an unheld joint. */
    if (held > 1u)
    {
        tail_q += pose->speed_q * (s32)(held - 1u);
    }

    for (i = 0u; i < NDS_FT_POSE_TRACKS; i++)
    {
        u32 slot = joint->slot_of_track[i];
        NdsFtPoseTrack *t;

        if (slot == NDS_FT_POSE_NO_SLOT)
        {
            continue;
        }
        t = &pose->pool[slot];
        if (t->kind != NDS_FT_POSE_KIND_NONE)
        {
            t->length += tail_q;
        }
    }
}

/* Returns the payload word and advances the cursor past the command word and
 * its optional payload -- the six textually identical reads of the source. */
#define NDS_FT_POSE_PAYLOAD()                                                 \
    (payload_u = ((pc++)->command.toggle != 0u) ? (pc++)->u : 0u)

#define NDS_FT_POSE_TARGET(vos) ndsFtPoseArg((pc++)->s, i, (vos))

static void ndsFtPoseParse(NdsFtPose *pose, NdsFtPoseJoint *joint, DObj *dobj)
{
    const AObjEvent16 *pc;
    u32 payload_u;
    u32 flags;
    u32 i;
    u32 events = 0u;
    s32 len_new;

    /* ft/ftanim.c:79-93, with the port's exact bit compares. The END sentinel
     * is only ever seen here on the tick after a HELD End: the source's player
     * turns END into NULL on the End tick itself, and the lazy hold defers
     * that (with the final evaluation) to the next evaluated tick. */
    if (NDS_FCMP_EQ_C(dobj->anim_wait, AOBJ_ANIM_END))
    {
        return;
    }
    if (NDS_FCMP_EQ_C(dobj->anim_wait, AOBJ_ANIM_CHANGED))
    {
        joint->wait_q = -joint->frame_q;
        dobj->anim_wait = NDS_FT_POSE_RUNNING;
    }
    else
    {
        joint->wait_q -= pose->speed_q;
        joint->frame_q += pose->speed_q;
        pose->gobj_frame_q = joint->frame_q;
        pose->gobj_frame_pending = 1u;
        if (joint->wait_q > 0)
        {
            return;
        }
    }
    gNdsFtPoseStepped++;
    pc = dobj->anim_joint.event16;
    do
    {
        u32 command_kind;

        if (pc == NULL)
        {
            ndsFtPoseAdvanceTail(pose, joint);
            joint->frame_q = joint->wait_q;
            pose->gobj_frame_q = joint->wait_q;
            pose->gobj_frame_pending = 1u;
            dobj->anim_wait = AOBJ_ANIM_END;
            dobj->anim_joint.event16 = NULL;
            return;
        }
        command_kind = pc->command.opcode;
        switch (command_kind)
        {
        case nGCAnimEvent16SetVal0RateBlock:
        case nGCAnimEvent16SetVal0Rate:
            flags = pc->command.flags;
            NDS_FT_POSE_PAYLOAD();
            len_new = ndsFtPoseSegmentStart(pose, joint);
            for (i = 0u; i < NDS_FT_POSE_TRACKS; i++, flags >>= 1)
            {
                NdsFtPoseTrack *t;

                if (flags == 0u)
                {
                    break;
                }
                if ((flags & 1u) == 0u)
                {
                    continue;
                }
                t = ndsFtPoseTrackFor(pose, joint, i);
                t->value_base = t->value_target;
                t->value_target = NDS_FT_POSE_TARGET(0);
                t->rate_base = t->rate_target;
                t->rate_target = 0;
                t->kind = NDS_R2_AQ_KIND_CUBIC;
                if (payload_u != 0u)
                {
                    t->length_invert = ndsR2FtAnimRecipQ30(payload_u);
                }
                t->length = len_new;
            }
            if (command_kind == nGCAnimEvent16SetVal0RateBlock)
            {
                joint->wait_q += (s32)payload_u << NDS_R2_AQ_LF;
            }
            break;

        case nGCAnimEvent16SetValBlock:
        case nGCAnimEvent16SetVal:
            flags = pc->command.flags;
            NDS_FT_POSE_PAYLOAD();
            len_new = ndsFtPoseSegmentStart(pose, joint);
            for (i = 0u; i < NDS_FT_POSE_TRACKS; i++, flags >>= 1)
            {
                NdsFtPoseTrack *t;

                if (flags == 0u)
                {
                    break;
                }
                if ((flags & 1u) == 0u)
                {
                    continue;
                }
                t = ndsFtPoseTrackFor(pose, joint, i);
                t->value_base = t->value_target;
                t->value_target = NDS_FT_POSE_TARGET(0);
                t->kind = NDS_R2_AQ_KIND_LINEAR;
                if (payload_u != 0u)
                {
                    /* Two Q12 integers divided, magnitude rounded to nearest,
                     * into the Q16 rate -- battleship_ftanim.c's own arm. */
                    s32 d = (((s32)t->value_target << t->shift_val) -
                             ((s32)t->value_base << t->shift_val))
                            << (NDS_R2_AQ_RF - NDS_R2_AQ_VF);
                    u32 h = payload_u >> 1;

                    t->rate_linear_q = (d < 0) ?
                        -(s32)(((u32)(-d) + h) / payload_u) :
                        (s32)(((u32)d + h) / payload_u);
                }
                t->length = len_new;
                t->rate_target = 0;
            }
            if (command_kind == nGCAnimEvent16SetValBlock)
            {
                joint->wait_q += (s32)payload_u << NDS_R2_AQ_LF;
            }
            break;

        case nGCAnimEvent16SetValRateBlock:
        case nGCAnimEvent16SetValRate:
            flags = pc->command.flags;
            NDS_FT_POSE_PAYLOAD();
            len_new = ndsFtPoseSegmentStart(pose, joint);
            for (i = 0u; i < NDS_FT_POSE_TRACKS; i++, flags >>= 1)
            {
                NdsFtPoseTrack *t;

                if (flags == 0u)
                {
                    break;
                }
                if ((flags & 1u) == 0u)
                {
                    continue;
                }
                t = ndsFtPoseTrackFor(pose, joint, i);
                t->value_base = t->value_target;
                t->value_target = NDS_FT_POSE_TARGET(0);
                t->rate_base = t->rate_target;
                t->rate_target = NDS_FT_POSE_TARGET(1);
                t->kind = NDS_R2_AQ_KIND_CUBIC;
                if (payload_u != 0u)
                {
                    t->length_invert = ndsR2FtAnimRecipQ30(payload_u);
                }
                t->length = len_new;
            }
            if (command_kind == nGCAnimEvent16SetValRateBlock)
            {
                joint->wait_q += (s32)payload_u << NDS_R2_AQ_LF;
            }
            break;

        case nGCAnimEvent16SetTargetRate:
            flags = pc->command.flags;
            NDS_FT_POSE_PAYLOAD();
            for (i = 0u; i < NDS_FT_POSE_TRACKS; i++, flags >>= 1)
            {
                NdsFtPoseTrack *t;

                if (flags == 0u)
                {
                    break;
                }
                if ((flags & 1u) == 0u)
                {
                    continue;
                }
                t = ndsFtPoseTrackFor(pose, joint, i);
                t->rate_target = NDS_FT_POSE_TARGET(1);
            }
            break;

        case nGCAnimEvent16Block:
            if ((pc++)->command.toggle != 0u)
            {
                joint->wait_q += (s32)((pc++)->u) << NDS_R2_AQ_LF;
            }
            break;

        case nGCAnimEvent16SetValAfterBlock:
        case nGCAnimEvent16SetValAfter:
            flags = pc->command.flags;
            NDS_FT_POSE_PAYLOAD();
            len_new = ndsFtPoseSegmentStart(pose, joint);
            for (i = 0u; i < NDS_FT_POSE_TRACKS; i++, flags >>= 1)
            {
                NdsFtPoseTrack *t;

                if (flags == 0u)
                {
                    break;
                }
                if ((flags & 1u) == 0u)
                {
                    continue;
                }
                t = ndsFtPoseTrackFor(pose, joint, i);
                t->value_base = t->value_target;
                t->value_target = NDS_FT_POSE_TARGET(0);
                t->kind = NDS_R2_AQ_KIND_STEP;
                /* Step's `length_invert` is a FRAME COUNT in length's scale. */
                t->length_invert = (s32)payload_u << NDS_R2_AQ_LF;
                t->length = len_new;
                t->rate_target = 0;
            }
            if (command_kind == nGCAnimEvent16SetValAfterBlock)
            {
                joint->wait_q += (s32)payload_u << NDS_R2_AQ_LF;
            }
            break;

        case nGCAnimEvent16Loop:
            pc++;
            pc += pc->s / 2;
            joint->frame_q = -joint->wait_q;
            pose->gobj_frame_q = -joint->wait_q;
            pose->gobj_frame_pending = 1u;
            /* `func_anim` has no writer anywhere in decomp/src or src/ (only
             * `= NULL`), so the source's `is_anim_root` callback is dead; the
             * generic parser keeps the call and it never fires. */
            break;

        case nGCAnimEvent1611:
            flags = pc->command.flags;
            NDS_FT_POSE_PAYLOAD();
            for (i = 0u; i < NDS_FT_POSE_TRACKS; i++, flags >>= 1)
            {
                NdsFtPoseTrack *t;

                if (flags == 0u)
                {
                    break;
                }
                if ((flags & 1u) == 0u)
                {
                    continue;
                }
                t = ndsFtPoseTrackFor(pose, joint, i);
                t->length += (s32)payload_u << NDS_R2_AQ_LF;
            }
            break;

        case nGCAnimEvent16SetTranslateInterp:
            pc++;
            (void)ndsFtPoseTrackFor(pose, joint, nGCAnimTrackTraI -
                                    nGCAnimTrackJointStart);
            joint->interpolate = (void *)(pc + (pc->s / 2));
            pc++;
            break;

        case nGCAnimEvent16End:
            ndsFtPoseAdvanceTail(pose, joint);
            joint->frame_q = joint->wait_q;
            pose->gobj_frame_q = joint->wait_q;
            pose->gobj_frame_pending = 1u;
            dobj->anim_wait = AOBJ_ANIM_END;
            dobj->anim_joint.event16 = (AObjEvent16 *)pc;
            return;

        case nGCAnimEvent16SetFlags:
            dobj->flags = (u8)pc->command.flags;
            if ((pc++)->command.toggle != 0u)
            {
                joint->wait_q += (s32)((pc++)->u) << NDS_R2_AQ_LF;
            }
            break;

        default:
            gNdsObjAnimRunawayCount++;
            gNdsObjAnimRunawayMask |= NDS_FT_POSE_RUNAWAY_BIT;
            gNdsObjAnimRunawayScript = (u32)(uintptr_t)pc;
            gNdsObjAnimRunawayOpcode = command_kind;
            dobj->anim_wait = AOBJ_ANIM_NULL;
            dobj->anim_joint.event16 = (AObjEvent16 *)pc;
            return;
        }
        if (++events >= NDS_FT_POSE_EVENT_LIMIT)
        {
            gNdsObjAnimRunawayCount++;
            gNdsObjAnimRunawayMask |= NDS_FT_POSE_RUNAWAY_BIT;
            gNdsObjAnimRunawayScript = (u32)(uintptr_t)pc;
            gNdsObjAnimRunawayOpcode = command_kind;
            dobj->anim_wait = AOBJ_ANIM_NULL;
            dobj->anim_joint.event16 = (AObjEvent16 *)pc;
            return;
        }
    }
    while (joint->wait_q <= 0);
    dobj->anim_joint.event16 = (AObjEvent16 *)pc;
}

/* ---- the player (sys/objanim.c:714 / lb/lbcommon.c:1261, Q form) --------- */

/* ARM, not Thumb: the evaluator's SMULLs and CLZ inline here. */
static void __attribute__((noinline, target("arm")))
ndsFtPosePlay(NdsFtPose *pose, NdsFtPoseJoint *joint, DObj *dobj,
              const Vec3f *tra_scale)
{
    /* The catch-up: one `length += speed` per tick since the last evaluation,
     * none on the End tick (the parser's tail advance already placed every
     * track at its segment end, and the source's player adds nothing when
     * anim_wait is END). `catch_up` is 1 on a joint evaluated every tick. */
    const u32 catch_up = NDS_FCMP_NE_C(dobj->anim_wait, AOBJ_ANIM_END) ?
        ((pose->tick - (u32)joint->last_eval) & 0xffffu) : 0u;
    const u32 play = ((dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM) == 0u) ?
        1u : 0u;
    const s32 add_q = (catch_up != 0u) ? pose->speed_q * (s32)catch_up : 0;
    u32 i;
    u32 evals = 0u;

    joint->last_eval = (u16)pose->tick;
    for (i = 0u; i < NDS_FT_POSE_TRACKS; i++)
    {
        u32 slot = joint->slot_of_track[i];
        NdsFtPoseTrack *t;
        f32 value;
        s32 rb_q;

        if (slot == NDS_FT_POSE_NO_SLOT)
        {
            continue;
        }
        t = &pose->pool[slot];
        if (t->kind == NDS_FT_POSE_KIND_NONE)
        {
            continue;
        }
        t->length += add_q;
        if (play == 0u)
        {
            continue;
        }
        rb_q = (t->kind == NDS_R2_AQ_KIND_LINEAR) ?
            t->rate_linear_q : ((s32)t->rate_base << t->shift_rate);
        value = ndsR2FixedToF32(
            ndsR2AnimEvalQ(t->length, t->length_invert,
                           (s32)t->value_base << t->shift_val,
                           (s32)t->value_target << t->shift_val, rb_q,
                           (s32)t->rate_target << t->shift_rate,
                           t->kind), NDS_R2_AQ_VF);
        evals++;
        /* The nine direct tracks store through an offset table (one load, one
         * store) rather than a ten-way switch; TraI and the translate-scaled
         * fighter (Luigi) take the slow arm. */
        if (t->track == nGCAnimTrackTraI)
        {
            if (NDS_FCMP_LT0(value))
            {
                value = 0.0F;
            }
            else if (NDS_FCMP_GT_C(value, 1.0F))
            {
                value = 1.0F;
            }
            syInterpCubic(&dobj->translate.vec.f, joint->interpolate, value);
            if (tra_scale != NULL)
            {
                dobj->translate.vec.f.x *= tra_scale->x;
                dobj->translate.vec.f.y *= tra_scale->y;
                dobj->translate.vec.f.z *= tra_scale->z;
            }
        }
        else
        {
            if ((tra_scale != NULL) && (t->track >= nGCAnimTrackTraX) &&
                (t->track <= nGCAnimTrackTraZ))
            {
                value *= (&tra_scale->x)[t->track - nGCAnimTrackTraX];
            }
            *(f32 *)((u8 *)dobj +
                     sNdsFtPoseStoreOffset[t->track - nGCAnimTrackJointStart]) =
                value;
        }
    }
    gNdsFtPoseTrackEvals += evals;
    if (NDS_FCMP_EQ_C(dobj->anim_wait, AOBJ_ANIM_END))
    {
        dobj->anim_wait = AOBJ_ANIM_NULL;
    }
}

/* ---- the oracle ---------------------------------------------------------- */

#if NDS_FT_POSE_ORACLE
static void ndsFtPoseOracleNote(u32 entry, u32 field, u32 want, u32 got,
                                const GObj *gobj)
{
    gNdsFtPoseOracleMismatches++;
    if (gNdsFtPoseOracleMismatches == 1u)
    {
        gNdsFtPoseOracleFirstJoint = entry;
        gNdsFtPoseOracleFirstField = field;
        gNdsFtPoseOracleFirstWant = want;
        gNdsFtPoseOracleFirstGot = got;
        __builtin_memcpy((void *)&gNdsFtPoseOracleFirstFrame,
                         &gobj->anim_frame, sizeof(u32));
    }
    if (field <= 8u)
    {
        gNdsFtPoseOraclePoseMismatches++;
        if (gNdsFtPoseOraclePoseMismatches == 1u)
        {
            gNdsFtPoseOracleFirstPoseJoint = entry;
            gNdsFtPoseOracleFirstPoseField = field;
            gNdsFtPoseOracleFirstPoseWant = want;
            gNdsFtPoseOracleFirstPoseGot = got;
            __builtin_memcpy((void *)&gNdsFtPoseOracleFirstPoseFrame,
                             &gobj->anim_frame, sizeof(u32));
        }
    }
}

static inline u32 ndsFtPoseBits(f32 v)
{
    u32 b;

    __builtin_memcpy(&b, &v, sizeof(b));
    return b;
}

/* Compare the shadow the engine wrote against the live joint the generic
 * path wrote, field by field, bit for bit. Fields: 0-2 rotate xyz, 3-5
 * translate xyz, 6-8 scale xyz, 9 anim_wait, 10 anim_frame, 11 flags, 12
 * the GObj anim_frame. */
static void ndsFtPoseOracleCompare(NdsFtPose *pose, u32 evaluate_body)
{
    u32 e;

    for (e = 0u; e < pose->entry_count; e++)
    {
        NdsFtPoseJoint *joint = &pose->joints[e];
        const DObj *shadow = joint->dobj;
        const DObj *real = joint->real;
        const f32 *sv;
        const f32 *rv;
        u32 f;

        if ((real == NULL) || (shadow == real))
        {
            continue;
        }
        gNdsFtPoseOracleCompares++;
        if ((joint->body == 0u) || (evaluate_body != 0u))
        {
            sv = &shadow->rotate.vec.f.x;
            rv = &real->rotate.vec.f.x;
            for (f = 0u; f < 3u; f++)
            {
                if (ndsFtPoseBits(sv[f]) != ndsFtPoseBits(rv[f]))
                {
                    ndsFtPoseOracleNote(e, f, ndsFtPoseBits(rv[f]),
                                        ndsFtPoseBits(sv[f]), pose->gobj);
                }
            }
            sv = &shadow->translate.vec.f.x;
            rv = &real->translate.vec.f.x;
            for (f = 0u; f < 3u; f++)
            {
                if (ndsFtPoseBits(sv[f]) != ndsFtPoseBits(rv[f]))
                {
                    ndsFtPoseOracleNote(e, 3u + f, ndsFtPoseBits(rv[f]),
                                        ndsFtPoseBits(sv[f]), pose->gobj);
                }
            }
            sv = &shadow->scale.vec.f.x;
            rv = &real->scale.vec.f.x;
            for (f = 0u; f < 3u; f++)
            {
                if (ndsFtPoseBits(sv[f]) != ndsFtPoseBits(rv[f]))
                {
                    ndsFtPoseOracleNote(e, 6u + f, ndsFtPoseBits(rv[f]),
                                        ndsFtPoseBits(sv[f]), pose->gobj);
                }
            }
        }
        {
            /* The clock: the engine's Q12 wait/frame against the live f32
             * fields, as the f32 the Q value converts to. A sentinel on the
             * shadow (NULL/END/CHANGED) is compared as itself. A HELD End: the
             * source's player turned END into NULL on the End tick; the lazy
             * hold keeps END until the joint's deferred final evaluation --
             * same clock, one tick of representation, and named here. */
            f32 wait_f = NDS_FCMP_EQ_C(shadow->anim_wait, NDS_FT_POSE_RUNNING) ?
                ndsR2FixedToF32(joint->wait_q, NDS_R2_AQ_LF) : shadow->anim_wait;
            u32 deferred_end =
                ((joint->body != 0u) && (evaluate_body == 0u) &&
                 NDS_FCMP_EQ_C(shadow->anim_wait, AOBJ_ANIM_END) &&
                 NDS_FCMP_EQ_C(real->anim_wait, AOBJ_ANIM_NULL)) ? 1u : 0u;

            if ((deferred_end == 0u) &&
                (ndsFtPoseBits(wait_f) != ndsFtPoseBits(real->anim_wait)))
            {
                ndsFtPoseOracleNote(e, 9u, ndsFtPoseBits(real->anim_wait),
                                    ndsFtPoseBits(wait_f), pose->gobj);
            }
            if (!NDS_FCMP_EQ_C(real->anim_wait, AOBJ_ANIM_NULL) ||
                !NDS_FCMP_EQ_C(shadow->anim_wait, AOBJ_ANIM_NULL))
            {
                f32 frame_f = ndsR2FixedToF32(joint->frame_q, NDS_R2_AQ_LF);

                if (ndsFtPoseBits(frame_f) != ndsFtPoseBits(real->anim_frame))
                {
                    ndsFtPoseOracleNote(e, 10u, ndsFtPoseBits(real->anim_frame),
                                        ndsFtPoseBits(frame_f), pose->gobj);
                }
            }
        }
        if (shadow->flags != real->flags)
        {
            ndsFtPoseOracleNote(e, 11u, real->flags, shadow->flags,
                                pose->gobj);
        }
    }
    if (ndsFtPoseBits(pose->clock_gobj->anim_frame) !=
        ndsFtPoseBits(pose->gobj->anim_frame))
    {
        ndsFtPoseOracleNote(0xffu, 12u, ndsFtPoseBits(pose->gobj->anim_frame),
                            ndsFtPoseBits(pose->clock_gobj->anim_frame),
                            pose->gobj);
    }
}
#endif

/* ---- the per-fighter update --------------------------------------------- */

static void ndsFtPoseRun(NdsFtPose *pose, Vec3f *translate_scales,
                         u32 evaluate_body, u32 advance)
{
    u32 e;

    /* gcSetAnimSpeed writes every DObj of the GObj, so the root's speed is
     * every joint's speed: one conversion per update instead of one per
     * joint. The oracle feeds the shadow GObj the live root. */
    pose->speed_q = ndsR2F32ToFixed(
        ((DObj *)pose->clock_gobj->obj)->anim_speed, NDS_R2_AQ_LF);
    pose->gobj_frame_pending = 0u;
    for (e = 0u; e < pose->entry_count; e++)
    {
        NdsFtPoseJoint *joint = &pose->joints[e];
        DObj *dobj = joint->dobj;
        const Vec3f *scale;

        if (dobj == NULL)
        {
            continue;
        }
#if NDS_FT_POSE_ORACLE
        /* The shadow's clock inputs are the live joint's: gcSetAnimSpeed and
         * the guard code write the real DObj between ticks. */
        dobj->anim_speed = joint->real->anim_speed;
        if (NDS_FCMP_EQ_C(joint->real->anim_wait, AOBJ_ANIM_NULL))
        {
            dobj->anim_wait = AOBJ_ANIM_NULL;
        }
        pose->clock_gobj->flags = pose->gobj->flags;
#endif
        /* The caller-side idle skip of ftParamUpdateAnimKeys: every arm of the
         * parser and the player is guarded by `anim_wait != NULL`. */
        if (NDS_FCMP_EQ_C(dobj->anim_wait, AOBJ_ANIM_NULL))
        {
            continue;
        }
        gNdsFtPoseJointTicks++;
        if (advance != 0u)
        {
            ndsFtPoseParse(pose, joint, dobj);
        }
        if ((joint->body == 0u) || (evaluate_body != 0u))
        {
            scale = (translate_scales != NULL) ?
                &translate_scales[joint->joint_id] : NULL;
            gNdsFtPoseJointEvals++;
            ndsFtPosePlay(pose, joint, dobj, scale);
        }
        else
        {
            /* Held: the clock ran, nothing else. The player catches the
             * track phases up at the next evaluation. */
            gNdsFtPoseJointHolds++;
        }
    }
    if (pose->gobj_frame_pending != 0u)
    {
        pose->clock_gobj->anim_frame =
            ndsR2FixedToF32(pose->gobj_frame_q, NDS_R2_AQ_LF);
    }
}

sb32 ndsFtPoseUpdate(GObj *gobj, FTStruct *fp, Vec3f *translate_scales)
{
    NdsFtPose *pose = ndsFtPoseFind(gobj);
    u32 evaluate_body;

    (void)fp;
    if ((pose == NULL) || (pose->bound == 0u))
    {
        return FALSE;
    }
#if NDS_FT_POSE_HOLD
    evaluate_body = ((gNdsFtPoseEvalTick != 0u) ||
                     (pose->attach_pending != 0u)) ? 1u : 0u;
#else
    evaluate_body = 1u;
#endif
    pose->attach_pending = 0u;
    pose->body_evaluated = evaluate_body;
    pose->tick++;
    gNdsFtPoseUpdates++;
    /* Volatile reads keep the hold-only counters linked on a HOLD=0 build:
     * --gc-sections drops an unreferenced `used` object, and a probe that
     * asks for the symbol then fails before it can read a zero. */
    (void)gNdsFtPoseJointHolds;
    (void)gNdsFtPoseEvalTick;
    (void)gNdsFtPoseOracleCompares;
    (void)gNdsFtPoseOracleMismatches;
    (void)gNdsFtPoseOracleFirstJoint;
    (void)gNdsFtPoseOracleFirstField;
    (void)gNdsFtPoseOracleFirstWant;
    (void)gNdsFtPoseOracleFirstGot;
    (void)gNdsFtPoseOracleFirstFrame;
    (void)gNdsFtPoseOraclePoseMismatches;
    (void)gNdsFtPoseOracleFirstPoseJoint;
    (void)gNdsFtPoseOracleFirstPoseField;
    (void)gNdsFtPoseOracleFirstPoseWant;
    (void)gNdsFtPoseOracleFirstPoseGot;
    (void)gNdsFtPoseOracleFirstPoseFrame;
    (void)gNdsFtPoseTrackOverflow;
    {
        u32 live = ndsR2AObjLiveCount();

        if (live > gNdsFtPoseAObjLiveMax)
        {
            gNdsFtPoseAObjLiveMax = live;
        }
    }
    ndsFtPoseRun(pose, translate_scales, evaluate_body, 1u);
#if NDS_FT_POSE_ORACLE
    /* The generic path has NOT run yet: the caller runs it next, then calls
     * ndsFtPoseOracleAfter. Report "not handled" so it does. */
    return FALSE;
#else
    return TRUE;
#endif
}

sb32 ndsFtPoseOwnsJoint(GObj *gobj, u32 joint_id)
{
    NdsFtPose *pose = ndsFtPoseFind(gobj);

    if ((pose == NULL) || (pose->bound == 0u) || (joint_id >= 64u))
    {
        return FALSE;
    }
    if (joint_id < 32u)
    {
        return ((pose->joint_mask_lo & (1u << joint_id)) != 0u) ? TRUE : FALSE;
    }
    return ((pose->joint_mask_hi & (1u << (joint_id - 32u))) != 0u) ? TRUE : FALSE;
}

sb32 ndsFtPoseReapply(GObj *gobj, FTStruct *fp, Vec3f *translate_scales)
{
    NdsFtPose *pose = ndsFtPoseFind(gobj);
    u32 e;

    (void)fp;
    if ((pose == NULL) || (pose->bound == 0u))
    {
        return FALSE;
    }
    /* ft/ftparam.c:430-471: every joint with an animation is played once with
     * anim_wait forced to END (no advance, values re-applied) and restored. */
    for (e = 0u; e < pose->entry_count; e++)
    {
        NdsFtPoseJoint *joint = &pose->joints[e];
        DObj *dobj = joint->dobj;
        FTParts *parts;
        f32 wait_bak;

        if (dobj == NULL)
        {
            continue;
        }
        parts = ftGetParts(joint->real);
        if ((parts == NULL) || (parts->is_have_anim == FALSE))
        {
            continue;
        }
        wait_bak = dobj->anim_wait;
        dobj->anim_wait = AOBJ_ANIM_END;
        ndsFtPosePlay(pose, joint, dobj,
                      (translate_scales != NULL) ?
                          &translate_scales[joint->joint_id] : NULL);
        dobj->anim_wait = wait_bak;
    }
#if NDS_FT_POSE_ORACLE
    return FALSE;
#else
    return TRUE;
#endif
}

#if NDS_FT_POSE_ORACLE
void ndsFtPoseOracleAfter(GObj *gobj)
{
    NdsFtPose *pose = ndsFtPoseFind(gobj);

    if ((pose == NULL) || (pose->bound == 0u))
    {
        return;
    }
    ndsFtPoseOracleCompare(pose, pose->body_evaluated);
}
#endif

sb32 ndsFtPoseBodyChangedThisTick(GObj *gobj)
{
    NdsFtPose *pose = ndsFtPoseFind(gobj);

    if ((pose == NULL) || (pose->bound == 0u))
    {
        return TRUE;
    }
    return (pose->body_evaluated != 0u) ? TRUE : FALSE;
}

#else /* !NDS_FT_POSE */

sb32 ndsFtPoseBindBegin(DObj *walk_root, u32 count)
{
    (void)walk_root;
    (void)count;
    return FALSE;
}

void ndsFtPoseBindEntry(u32 entry, DObj *dobj, AObjEvent16 *script,
                        f32 anim_frame)
{
    (void)entry;
    (void)dobj;
    (void)script;
    (void)anim_frame;
}

void ndsFtPoseBindEnd(u32 entry_count)
{
    (void)entry_count;
}

void ndsFtPoseUnbind(GObj *gobj)
{
    (void)gobj;
}

void ndsFtPoseRelease(GObj *gobj)
{
    (void)gobj;
}

sb32 ndsFtPoseUpdate(GObj *gobj, FTStruct *fp, Vec3f *translate_scales)
{
    (void)gobj;
    (void)fp;
    (void)translate_scales;
    return FALSE;
}

sb32 ndsFtPoseOwnsJoint(GObj *gobj, u32 joint_id)
{
    (void)gobj;
    (void)joint_id;
    return FALSE;
}

sb32 ndsFtPoseReapply(GObj *gobj, FTStruct *fp, Vec3f *translate_scales)
{
    (void)gobj;
    (void)fp;
    (void)translate_scales;
    return FALSE;
}

sb32 ndsFtPoseBodyChangedThisTick(GObj *gobj)
{
    (void)gobj;
    return TRUE;
}

#endif /* NDS_FT_POSE */
