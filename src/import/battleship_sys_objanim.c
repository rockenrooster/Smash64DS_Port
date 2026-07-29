/* Compile the original BattleShip object-animation/setup translation unit.
 * Its public add entrypoints normalize O2R AObjEvent32 command words before
 * the unchanged original parser sees them. */
#include <nds/nds_reloc_assets.h>

#define gcAddDObjAnimJoint ndsBaseGcAddDObjAnimJoint
#define gcAddMObjMatAnimJoint ndsBaseGcAddMObjMatAnimJoint
#define gcAddMObjAll ndsBaseGcAddMObjAll
#define gcAddAnimJointAll ndsBaseGcAddAnimJointAll
#define gcAddMatAnimJointAll ndsBaseGcAddMatAnimJointAll
#define gcAddAnimAll ndsBaseGcAddAnimAll
#define gcAddCObjCamAnimJoint ndsBaseGcAddCObjCamAnimJoint
#define gcPlayMObjMatAnim ndsBaseGcPlayMObjMatAnim
#define gcPlayAnimAll ndsBaseGcPlayAnimAll
#if NDS_R2_ANIM_CENSUS || NDS_R2_CUBIC_FIXED
/* R2-03 E61/E64. Frees the name so the port-side player below is reached. Task
 * 95 proved this exact interposition end to end: the hot call is INTERNAL to
 * objanim.c (inside gcPlayAnimAll), so renaming the definition renames that
 * call with it and a port-side replacement actually runs. */
#define gcPlayDObjAnimJoint ndsBaseGcPlayDObjAnimJoint
#endif

#include "../../decomp/BattleShip-main/decomp/src/sys/objanim.c"

#undef gcAddDObjAnimJoint
#undef gcAddMObjMatAnimJoint
#undef gcAddMObjAll
#undef gcAddAnimJointAll
#undef gcAddMatAnimJointAll
#undef gcAddAnimAll
#undef gcAddCObjCamAnimJoint
#undef gcPlayMObjMatAnim
#undef gcPlayAnimAll

#if NDS_R2_CUBIC_FIXED
#undef gcPlayDObjAnimJoint

/* R2-03 E64 — the cubic in fixed point. Owner-authorized 2026-07-29.
 *
 * E60/E61 measured this: `gcPlayDObjAnimJoint` is 94,531 ticks/frame inclusive,
 * 60,509 of it soft-float, and 99.6% of that float is the CUBIC branch — 149.4
 * evaluations a frame at ~405 ticks each, which is 14 soft-float operations at
 * ~29 ticks apiece. Step (43.6% of nodes) and Linear (1.7%) are left exactly as
 * the original wrote them; they cost no float worth removing.
 *
 * The rewrite. With `t = length * length_invert` and `L = length`, the original
 *
 *     f16 = li², f12 = L², f18 = li·L², f14 = L·L²·li²,
 *     f20 = 2·f14·li, f22 = 3·f12·f16, f24 = f14 - f18
 *     value = vb·((f20-f22)+1) + vt·(f22-f20)
 *           + rb·((f24-f18)+L) + rt·f24
 *
 * is, exactly in real arithmetic, the standard cubic Hermite:
 *
 *     f20 = 2t³            f22 = 3t²            f18 = L·t
 *     f24 = L·t·(t-1)      (f24-f18)+L = L·(1-t)²
 *     value = vb·(2t³-3t²+1) + vt·(3t²-2t³) + rb·L·(1-t)² + rt·L·(t²-t)
 *
 * so this is a change of *representation*, not of the curve. What differs from
 * the original is rounding: Q12 truncation instead of MIPS single-precision at
 * every step. `PROJECT_GOAL.md` requires mechanical equivalence and lists
 * "fixed-point replacements" as an allowed technique; it does not require bit
 * exactness. The Task 9 state hash asserts the stronger property and is expected
 * to move — that is the authorized part of this change, not an accident.
 *
 * Why the cache is not optional. `value_base`/`value_target`/`rate_base`/
 * `rate_target` are `f32` in the AObj, so converting them per evaluation costs
 * four soft-float round trips and eats the entire win. They are constant between
 * parse events, which are rare, so they are converted once and reused. The
 * validity test is a compare of the five source float BIT PATTERNS — integer
 * work, no float — which is exact and needs no cooperation from the parser.
 * `length` does change every tick, so `t` still costs one real multiply. */

/* E64 arm A had a 256-entry Q12 conversion cache keyed on the source float bit
 * patterns. It WORKED -- 135,871 evaluations, 86.4% hit rate, zero saturations --
 * and the frame got worse anyway: WORK-H P95 +21,632, SRC P50 +17,792. Two
 * reasons, both about footprint rather than arithmetic, and both already written
 * down in this repo:
 *
 *   - 10,240 bytes of new BSS. "The noise floor is not measurement error, it is
 *     the price of adding data" -- and the floor here is 5,000-7,000.
 *   - the `.text.hot` member grew from 500 bytes to 1,824. Task 94's comment in
 *     `linker/nds_hot_text.ld` says that list is a curated 8 KiB working set and
 *     that perturbing one member re-addresses the other ten, which it measured
 *     at 6,144 WORK-H P50 on 122 of 128 frames.
 *
 * Arm B therefore spends nothing: no cache, no new BSS, 32-bit arithmetic
 * wherever the range allows, and the four per-node conversions done inline. The
 * hand-rolled converter is what makes that affordable -- `(s32)(v * 4096.0f)` is
 * two soft-float calls where this is a dozen integer ops. */

volatile u32 gNdsR2CubicEvals;
volatile u32 gNdsR2CubicSaturations;

static inline u32 ndsR2FloatBits(f32 v)
{
    u32 bits;

    __builtin_memcpy(&bits, &v, sizeof(bits));
    return bits;
}

/* f32 -> Q12, truncating toward zero. Hand-rolled rather than
 * `(s32)(v * 4096.0f)` because that is two soft-float calls (~50 ticks) where
 * this is a dozen integer ops. Saturates instead of wrapping: a wrapped joint
 * angle would be a visible teleport, a saturated one is a clamp. */
static inline s32 ndsR2F32ToQ12(f32 v)
{
    u32 bits = ndsR2FloatBits(v);
    s32 exp = (s32)((bits >> 23) & 0xffu);
    s32 mant;
    s32 shift;

    if (exp == 0)
    {
        return 0;   /* zero or subnormal: below Q12's resolution either way */
    }
    if (exp == 0xff)
    {
        gNdsR2CubicSaturations++;
        return ((bits & 0x80000000u) != 0u) ? -0x7fffffff : 0x7fffffff;
    }
    mant = (s32)((bits & 0x7fffffu) | 0x800000u);   /* 1.23 fixed */
    /* value = mant * 2^(exp-127-23); Q12 wants that scaled by 2^12. */
    shift = (exp - 127) - 23 + 12;
    if (shift >= 0)
    {
        if (shift > 7)
        {
            gNdsR2CubicSaturations++;
            mant = 0x7fffffff;
        }
        else
        {
            mant <<= shift;
        }
    }
    else if (shift < -31)
    {
        mant = 0;
    }
    else
    {
        mant >>= -shift;
    }
    return ((bits & 0x80000000u) != 0u) ? -mant : mant;
}

/* Q12 -> f32. One `__aeabi_i2f` plus an exact exponent adjust; subtracting
 * 12 biased exponents is an exact divide by 4096, so no second multiply and no
 * extra rounding. */
static inline f32 ndsR2Q12ToF32(s32 q)
{
    f32 f = (f32)q;
    u32 bits = ndsR2FloatBits(f);

    if ((bits & 0x7f800000u) != 0u)
    {
        bits -= (12u << 23);
    }
    __builtin_memcpy(&f, &bits, sizeof(f));
    return f;
}

static f32 ndsR2CubicValueFixed(const AObj *aobj)
{
    /* The one unavoidable real multiply: `length` advances every tick, so `t`
     * cannot be carried across evaluations. */
    s32 t = ndsR2F32ToQ12(aobj->length * aobj->length_invert);
    s32 length_q = ndsR2F32ToQ12(aobj->length);
    /* t is Q12 and normally in [0,1], so t*t peaks near 4096*4096 = 16.7M and
     * every intermediate below stays inside s32 without a 64-bit step. */
    s32 t2 = (t * t) >> 12;
    s32 t3 = (t2 * t) >> 12;
    s32 omt = 4096 - t;
    s32 omt2 = (omt * omt) >> 12;
    s32 h_vb = ((2 * t3) - (3 * t2)) + 4096;
    s32 h_vt = (3 * t2) - (2 * t3);
    /* These two carry a factor of `length`, which is unbounded, so they are the
     * only places that need the wider intermediate. SMULL/SMLAL make a 32x32->64
     * on ARM9 a single instruction; it is the 64-bit ADDS/ADCS chains that arm A
     * paid for, and there are none left here. */
    s32 h_rb = (s32)(((s64)length_q * omt2) >> 12);
    s32 h_rt = (s32)(((s64)length_q * (t2 - t)) >> 12);
    s64 acc = (s64)ndsR2F32ToQ12(aobj->value_base) * h_vb;

    gNdsR2CubicEvals++;
    acc += (s64)ndsR2F32ToQ12(aobj->value_target) * h_vt;
    acc += (s64)ndsR2F32ToQ12(aobj->rate_base) * h_rb;
    acc += (s64)ndsR2F32ToQ12(aobj->rate_target) * h_rt;
    return ndsR2Q12ToF32((s32)(acc >> 12));
}

/* The original body, with the cubic branch replaced. Step and Linear are the
 * decomp's own expressions verbatim, so the 43.6% + 1.7% of nodes that take
 * them are bit-identical to before this change. */
void gcPlayDObjAnimJoint(DObj *dobj)
{
    f32 value = 0.0f;
    AObj *aobj;

    if (dobj->anim_wait != AOBJ_ANIM_NULL)
    {
        aobj = dobj->aobj;

        while (aobj != NULL)
        {
            if (aobj->kind != nGCAnimKindNone)
            {
                if (dobj->anim_wait != AOBJ_ANIM_END)
                {
                    aobj->length += dobj->anim_speed;
                }
                if (!(dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM))
                {
                    switch (aobj->kind)
                    {
                    case nGCAnimKindLinear:
                        value = aobj->value_base +
                            (aobj->length * aobj->rate_base);
                        break;

                    case nGCAnimKindCubic:
                        value = ndsR2CubicValueFixed(aobj);
                        break;

                    case nGCAnimKindStep:
                        value = (aobj->length_invert <= aobj->length) ?
                            aobj->value_target : aobj->value_base;
                        break;

                    default:
                        break;
                    }
                    switch (aobj->track)
                    {
                    case nGCAnimTrackRotX: dobj->rotate.vec.f.x = value; break;
                    case nGCAnimTrackRotY: dobj->rotate.vec.f.y = value; break;
                    case nGCAnimTrackRotZ: dobj->rotate.vec.f.z = value; break;

                    case nGCAnimTrackTraI:
                        if (value < 0.0F)
                        {
                            value = 0.0F;
                        }
                        else if (value > 1.0F)
                        {
                            value = 1.0F;
                        }
                        syInterpCubic(&dobj->translate.vec.f,
                                      aobj->interpolate, value);
                        break;

                    case nGCAnimTrackTraX: dobj->translate.vec.f.x = value; break;
                    case nGCAnimTrackTraY: dobj->translate.vec.f.y = value; break;
                    case nGCAnimTrackTraZ: dobj->translate.vec.f.z = value; break;
                    case nGCAnimTrackScaX: dobj->scale.vec.f.x = value; break;
                    case nGCAnimTrackScaY: dobj->scale.vec.f.y = value; break;
                    case nGCAnimTrackScaZ: dobj->scale.vec.f.z = value; break;
                    default: break;
                    }
                }
            }
            aobj = aobj->next;
        }
        if (dobj->anim_wait == AOBJ_ANIM_END)
        {
            dobj->anim_wait = AOBJ_ANIM_NULL;
        }
    }
}
#endif

#if NDS_R2_ANIM_CENSUS
#undef gcPlayDObjAnimJoint

/* R2-03 E61. E60 priced this path at 146,942 ticks/frame inclusive -- larger
 * than the whole gap to the gate -- and 280 ticks per AObj node, which is what
 * a ~14-operation cubic costs in software float. Before any of that is
 * rewritten, three integers decide WHICH rewrite:
 *
 *   1. the kind mix. Cubic is ~14 float ops, Linear is 2, Step is 0. If the
 *      nodes are mostly Linear the arithmetic is not the target and E60's
 *      per-node arithmetic reading is wrong.
 *   2. anim_speed. `length` is a pure accumulator of it, so if it only ever
 *      takes 0 or 1 the pose is a function of an INTEGER frame index and a
 *      load-time table is bit-exact. Any other value and the index is
 *      continuous and no table can be exact.
 *   3. how many evaluations are discarded. The original computes `value`
 *      before checking GOBJ_FLAG_NOANIM... it does not, but it DOES skip the
 *      whole evaluation under that flag, so counting the skips separates
 *      "poses computed" from "poses used".
 *
 * Counting only -- the real work is delegated unchanged, so this cannot change
 * a value. Task 96 measured the chain (337.8 nodes/frame over 104.1 calls) with
 * the same interposition and its numbers are the cross-check. */
extern void ndsBaseGcPlayDObjAnimJoint(DObj *dobj);

volatile u32 gNdsR2AnimCensusCalls;
volatile u32 gNdsR2AnimCensusNodes;
volatile u32 gNdsR2AnimCensusKindNone;
volatile u32 gNdsR2AnimCensusKindLinear;
volatile u32 gNdsR2AnimCensusKindCubic;
volatile u32 gNdsR2AnimCensusKindStep;
volatile u32 gNdsR2AnimCensusKindOther;
volatile u32 gNdsR2AnimCensusSpeedOne;
volatile u32 gNdsR2AnimCensusSpeedZero;
volatile u32 gNdsR2AnimCensusSpeedOther;
volatile u32 gNdsR2AnimCensusSpeedOtherBits;
volatile u32 gNdsR2AnimCensusNoAnimSkips;
volatile u32 gNdsR2AnimCensusAnimEnd;
volatile u32 gNdsR2AnimCensusLongestChain;

void gcPlayDObjAnimJoint(DObj *dobj)
{
    const AObj *aobj;
    u32 chain = 0u;
    f32 speed;
    u32 speed_bits;
    u32 noanim;

    gNdsR2AnimCensusCalls++;
    if (dobj->anim_wait != AOBJ_ANIM_NULL)
    {
        speed = dobj->anim_speed;
        __builtin_memcpy(&speed_bits, &speed, sizeof(speed_bits));
        if (speed_bits == 0x3f800000u)
        {
            gNdsR2AnimCensusSpeedOne++;
        }
        else if ((speed_bits & 0x7fffffffu) == 0u)
        {
            gNdsR2AnimCensusSpeedZero++;
        }
        else
        {
            gNdsR2AnimCensusSpeedOther++;
            gNdsR2AnimCensusSpeedOtherBits = speed_bits;
        }
        if (dobj->anim_wait == AOBJ_ANIM_END)
        {
            gNdsR2AnimCensusAnimEnd++;
        }
        noanim = ((dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM) != 0) ? 1u : 0u;
        for (aobj = dobj->aobj; aobj != NULL; aobj = aobj->next)
        {
            chain++;
            if (aobj->kind == nGCAnimKindNone)
            {
                gNdsR2AnimCensusKindNone++;
                continue;
            }
            gNdsR2AnimCensusNodes++;
            if (noanim != 0u)
            {
                gNdsR2AnimCensusNoAnimSkips++;
            }
            switch (aobj->kind)
            {
            case nGCAnimKindLinear: gNdsR2AnimCensusKindLinear++; break;
            case nGCAnimKindCubic:  gNdsR2AnimCensusKindCubic++;  break;
            case nGCAnimKindStep:   gNdsR2AnimCensusKindStep++;   break;
            default:                gNdsR2AnimCensusKindOther++;  break;
            }
        }
        if (chain > gNdsR2AnimCensusLongestChain)
        {
            gNdsR2AnimCensusLongestChain = chain;
        }
    }
    ndsBaseGcPlayDObjAnimJoint(dobj);
}
#endif

#define NDS_AOBJ_EVENT32_NORMALIZED_MAX 1024u
#define NDS_AOBJ_EVENT32_PLAN_MAX 128u
#define NDS_AOBJ_EVENT32_BRANCH_DEPTH_MAX 16u

typedef enum NDSAObjEvent32OwnerKind
{
    nNDSAObjEvent32OwnerDObj,
    nNDSAObjEvent32OwnerMObj,
    nNDSAObjEvent32OwnerCObj
} NDSAObjEvent32OwnerKind;

typedef struct NDSAObjEvent32Normalized
{
    AObjEvent32 *command;
    u32 native_word;
} NDSAObjEvent32Normalized;

typedef struct NDSAObjEvent32Plan
{
    AObjEvent32 *command;
    u32 source_word;
    u32 native_word;
} NDSAObjEvent32Plan;

static NDSAObjEvent32Normalized
    sNdsAObjEvent32Normalized[NDS_AOBJ_EVENT32_NORMALIZED_MAX];
static NDSAObjEvent32Plan sNdsAObjEvent32Plan[NDS_AOBJ_EVENT32_PLAN_MAX];
static u32 sNdsAObjEvent32NormalizedCount;
static u32 sNdsAObjEvent32PlanCount;

volatile u32 gNdsAObjEvent32NormalizeScriptCount;
volatile u32 gNdsAObjEvent32NormalizeCommandCount;
volatile u32 gNdsAObjEvent32NormalizeReuseCount;
volatile u32 gNdsAObjEvent32NormalizeFailCount;
volatile u32 gNdsAObjEvent32NormalizeFirstSourceWord;
volatile u32 gNdsAObjEvent32NormalizeFirstNativeWord;
volatile u32 gNdsAObjEvent32NormalizeLastFailReason;
volatile u32 gNdsAObjEvent32NormalizeLastFailOwner;
volatile u32 gNdsAObjEvent32NormalizeLastFailAddress;
volatile u32 gNdsAObjEvent32NormalizeLastFailWord;
volatile u32 gNdsAObjEvent32NormalizeLastFailOpcode;
volatile u32 gNdsAObjEvent32NormalizeLastFailFlags;
volatile u32 gNdsAObjEvent32ColorCorrectionCount;

volatile u32 gNdsMObjSubAttachNormalizeCount;
volatile u32 gNdsMObjSubAttachNativeCount;
volatile u32 gNdsMObjSubAttachFailCount;
volatile u32 gNdsMObjSubAttachFirstSourceFlags;
volatile u32 gNdsMObjSubAttachFirstNativeFlags;

/* Preserve objanim.c:2429-2455 exactly except for the compatibility copy at
 * the MObj attachment boundary. gcAddMObjForDObj copies the full MObjSub, so
 * the normalized stack record cannot escape this call. */
void gcAddMObjAll(GObj *gobj, MObjSub ***p_mobjsubs)
{
    DObj *dobj = DObjGetStruct(gobj);

    while (dobj != NULL)
    {
        if (p_mobjsubs != NULL)
        {
            if (*p_mobjsubs != NULL)
            {
                MObjSub **mobjsubs = *p_mobjsubs;
                MObjSub *mobjsub = *mobjsubs;

                while (mobjsub != NULL)
                {
                    MObjSub normalized_mobjsub;
                    s32 normalize_result =
                        ndsRelocCopyMObjSubForAttachment(
                            &normalized_mobjsub, mobjsub);

                    if (normalize_result > 0)
                    {
                        if (gNdsMObjSubAttachNormalizeCount == 0u)
                        {
                            gNdsMObjSubAttachFirstSourceFlags =
                                mobjsub->flags;
                            gNdsMObjSubAttachFirstNativeFlags =
                                normalized_mobjsub.flags;
                        }
                        gNdsMObjSubAttachNormalizeCount++;
                    }
                    else if (normalize_result == 0)
                    {
                        gNdsMObjSubAttachNativeCount++;
                    }
                    else
                    {
                        gNdsMObjSubAttachFailCount++;
                        /* A malformed loaded record is neither a safe native
                         * attachment nor a valid O2R conversion. Fail closed;
                         * the canonical scene requires this path to stay at
                         * zero, so no source material is silently omitted. */
                        mobjsubs++;
                        mobjsub = *mobjsubs;
                        continue;
                    }

                    gcAddMObjForDObj(dobj, &normalized_mobjsub);

                    mobjsubs++;
                    mobjsub = *mobjsubs;
                }
            }
            p_mobjsubs++;
        }
        dobj = gcGetTreeDObjNext(dobj);
    }
}

extern s32 ndsRelocPointerRangeInLoadedFiles(const void *ptr, size_t size);
extern s32 ndsRelocPointerIsFighterAObj16(const void *ptr);

static u32 ndsAObjEvent32CountFlags(u32 flags)
{
    u32 count = 0u;

    while (flags != 0u)
    {
        count += flags & 1u;
        flags >>= 1;
    }
    return count;
}

static s32 ndsAObjEvent32FindNormalized(AObjEvent32 *command)
{
    u32 i;

    for (i = 0u; i < sNdsAObjEvent32NormalizedCount; i++)
    {
        if (sNdsAObjEvent32Normalized[i].command == command)
        {
            return (s32)i;
        }
    }
    return -1;
}

static s32 ndsAObjEvent32FindPlanned(AObjEvent32 *command)
{
    u32 i;

    for (i = 0u; i < sNdsAObjEvent32PlanCount; i++)
    {
        if (sNdsAObjEvent32Plan[i].command == command)
        {
            return (s32)i;
        }
    }
    return -1;
}

static sb32 ndsAObjEvent32Reject(u32 reason, AObjEvent32 *command,
                                 NDSAObjEvent32OwnerKind owner_kind,
                                 u32 source_word)
{
    gNdsAObjEvent32NormalizeLastFailReason = reason;
    gNdsAObjEvent32NormalizeLastFailOwner = (u32)owner_kind;
    gNdsAObjEvent32NormalizeLastFailAddress = (u32)(uintptr_t)command;
    gNdsAObjEvent32NormalizeLastFailWord = source_word;
    gNdsAObjEvent32NormalizeLastFailOpcode =
        (source_word >> 25) & 0x7fu;
    gNdsAObjEvent32NormalizeLastFailFlags =
        (source_word >> 15) & 0x03ffu;
    return FALSE;
}

/* objdef.h:272-281 defines the source word as opcode[31:25], flags[24:15],
 * payload[14:0]. ARM GCC allocates objtypes.h:94-107 bitfields from the low
 * bit, so only command words are repacked; following values and pointers stay
 * in their already-relocated O2R representation. */
static sb32 ndsAObjEvent32PlanStream(AObjEvent32 *script,
                                     NDSAObjEvent32OwnerKind owner_kind,
                                     u32 branch_depth)
{
    AObjEvent32 *command = script;

    if ((script == NULL) ||
        (branch_depth > NDS_AOBJ_EVENT32_BRANCH_DEPTH_MAX))
    {
        return ndsAObjEvent32Reject(1u, script, owner_kind, 0u);
    }

    while (TRUE)
    {
        AObjEvent32 *branch_target = NULL;
        u32 source_word;
        u32 opcode;
        u32 flags;
        u32 payload;
        u32 value_words = 0u;
        s32 normalized_index;
        sb32 is_end = FALSE;
        sb32 is_branch = FALSE;

        if (ndsRelocPointerRangeInLoadedFiles(command, sizeof(*command)) ==
            FALSE)
        {
            return ndsAObjEvent32Reject(2u, command, owner_kind, 0u);
        }

        normalized_index = ndsAObjEvent32FindNormalized(command);
        if (normalized_index >= 0)
        {
            return (command->u ==
                    sNdsAObjEvent32Normalized[normalized_index].native_word) ?
                       TRUE : ndsAObjEvent32Reject(3u, command, owner_kind,
                                                   command->u);
        }
        if (ndsAObjEvent32FindPlanned(command) >= 0)
        {
            return TRUE;
        }

        source_word = command->u;
        opcode = (source_word >> 25) & 0x7fu;
        flags = (source_word >> 15) & 0x03ffu;
        payload = source_word & 0x7fffu;

        switch (opcode)
        {
        case nGCAnimEvent32End:
            is_end = TRUE;
            break;

        case nGCAnimEvent32Jump:
        case nGCAnimEvent32SetAnim:
            value_words = 1u;
            is_branch = TRUE;
            break;

        case nGCAnimEvent32Wait:
        case ANIM_CMD_12:
            break;

        case nGCAnimEvent32SetValBlock:
        case nGCAnimEvent32SetVal:
        case nGCAnimEvent32SetTargetRate:
        case nGCAnimEvent32SetVal0RateBlock:
        case nGCAnimEvent32SetVal0Rate:
        case nGCAnimEvent32SetValAfterBlock:
        case nGCAnimEvent32SetValAfter:
            value_words = ndsAObjEvent32CountFlags(flags);
            break;

        case nGCAnimEvent32SetValRateBlock:
        case nGCAnimEvent32SetValRate:
            value_words = ndsAObjEvent32CountFlags(flags) * 2u;
            break;

        case nGCAnimEvent32SetInterp:
            if (owner_kind == nNDSAObjEvent32OwnerDObj)
            {
                value_words = 1u;
            }
            else if (owner_kind == nNDSAObjEvent32OwnerCObj)
            {
                value_words = ((flags & 0x08u) != 0u) +
                              ((flags & 0x80u) != 0u);
            }
            else
            {
                return ndsAObjEvent32Reject(4u, command, owner_kind,
                                            source_word);
            }
            break;

        case nGCAnimEvent32SetFlags:
        case ANIM_CMD_16:
            if (owner_kind != nNDSAObjEvent32OwnerDObj)
            {
                return ndsAObjEvent32Reject(5u, command, owner_kind,
                                            source_word);
            }
            break;

        case ANIM_CMD_17:
            if (owner_kind != nNDSAObjEvent32OwnerDObj)
            {
                return ndsAObjEvent32Reject(6u, command, owner_kind,
                                            source_word);
            }
            value_words = ndsAObjEvent32CountFlags(flags);
            break;

        case nGCAnimEvent32SetExtValAfterBlock:
        case nGCAnimEvent32SetExtValAfter:
        case nGCAnimEvent32SetExtValBlock:
        case nGCAnimEvent32SetExtVal:
            if (owner_kind != nNDSAObjEvent32OwnerMObj)
            {
                return ndsAObjEvent32Reject(7u, command, owner_kind,
                                            source_word);
            }
            value_words = ndsAObjEvent32CountFlags(flags);
            break;

        case ANIM_CMD_22:
            if (owner_kind != nNDSAObjEvent32OwnerMObj)
            {
                return ndsAObjEvent32Reject(8u, command, owner_kind,
                                            source_word);
            }
            value_words = ndsAObjEvent32CountFlags(flags & 0x1fu);
            break;

        case ANIM_CMD_23:
            if (owner_kind != nNDSAObjEvent32OwnerCObj)
            {
                return ndsAObjEvent32Reject(9u, command, owner_kind,
                                            source_word);
            }
            /* objanim.c:2811-2813 consumes this command, then skips two
             * payload words before parsing the next command. */
            value_words = 2u;
            break;

        default:
            return ndsAObjEvent32Reject(10u, command, owner_kind,
                                        source_word);
        }

        if ((sNdsAObjEvent32PlanCount >= NDS_AOBJ_EVENT32_PLAN_MAX) ||
            (ndsRelocPointerRangeInLoadedFiles(
                 command, (1u + value_words) * sizeof(*command)) == FALSE))
        {
            return ndsAObjEvent32Reject(11u, command, owner_kind,
                                        source_word);
        }

        sNdsAObjEvent32Plan[sNdsAObjEvent32PlanCount].command = command;
        sNdsAObjEvent32Plan[sNdsAObjEvent32PlanCount].source_word = source_word;
        sNdsAObjEvent32Plan[sNdsAObjEvent32PlanCount].native_word =
            opcode | (flags << 7) | (payload << 17);
        sNdsAObjEvent32PlanCount++;

        if (is_end != FALSE)
        {
            return TRUE;
        }
        if (is_branch != FALSE)
        {
            branch_target = command[1].p;
            return ndsAObjEvent32PlanStream(branch_target, owner_kind,
                                             branch_depth + 1u);
        }
        command += 1u + value_words;
    }
}

static sb32 ndsAObjEvent32NormalizeScript(
    AObjEvent32 *script, NDSAObjEvent32OwnerKind owner_kind)
{
    u32 i;
    s32 normalized_index;

    if (script == NULL)
    {
        return TRUE;
    }

    normalized_index = ndsAObjEvent32FindNormalized(script);
    if (normalized_index >= 0)
    {
        if (script->u ==
            sNdsAObjEvent32Normalized[normalized_index].native_word)
        {
            gNdsAObjEvent32NormalizeReuseCount++;
            return TRUE;
        }
        (void)ndsAObjEvent32Reject(3u, script, owner_kind, script->u);
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }

    sNdsAObjEvent32PlanCount = 0u;
    if (ndsAObjEvent32PlanStream(script, owner_kind, 0u) == FALSE)
    {
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }
    if ((sNdsAObjEvent32NormalizedCount + sNdsAObjEvent32PlanCount) >
        NDS_AOBJ_EVENT32_NORMALIZED_MAX)
    {
        (void)ndsAObjEvent32Reject(12u, script, owner_kind, script->u);
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }

    if ((gNdsAObjEvent32NormalizeCommandCount == 0u) &&
        (sNdsAObjEvent32PlanCount != 0u))
    {
        gNdsAObjEvent32NormalizeFirstSourceWord =
            sNdsAObjEvent32Plan[0].source_word;
        gNdsAObjEvent32NormalizeFirstNativeWord =
            sNdsAObjEvent32Plan[0].native_word;
    }

    for (i = 0u; i < sNdsAObjEvent32PlanCount; i++)
    {
        sNdsAObjEvent32Plan[i].command->u =
            sNdsAObjEvent32Plan[i].native_word;
        sNdsAObjEvent32Normalized[sNdsAObjEvent32NormalizedCount].command =
            sNdsAObjEvent32Plan[i].command;
        sNdsAObjEvent32Normalized[sNdsAObjEvent32NormalizedCount].native_word =
            sNdsAObjEvent32Plan[i].native_word;
        sNdsAObjEvent32NormalizedCount++;
    }

    gNdsAObjEvent32NormalizeScriptCount++;
    gNdsAObjEvent32NormalizeCommandCount += sNdsAObjEvent32PlanCount;
    return TRUE;
}

void ndsAObjEvent32ResetNormalizedScripts(void)
{
    sNdsAObjEvent32NormalizedCount = 0u;
    sNdsAObjEvent32PlanCount = 0u;
}

static u32 ndsAObjEvent32FloatBits(f32 value)
{
    union
    {
        f32 f;
        u32 u;
    } bits;

    bits.f = value;
    return bits.u;
}

static u8 ndsAObjEvent32LerpColorChannel(u32 base, u32 target,
                                         u32 shift, s32 interp)
{
    s32 base_channel = (s32)((base >> shift) & 0xffu);
    s32 target_channel = (s32)((target >> shift) & 0xffu);

    return (u8)((base_channel * (256 - interp) +
                 target_channel * interp) >> 8);
}

/* objanim.c:1363-1388 interpolates packed RGBA by multiplying carefully
 * spaced bytes inside a u32. That arithmetic depends on N64 big-endian byte
 * lanes. Keep the original player for timing/state, then rewrite only the
 * five color outputs from the source 0xRRGGBBAA payload bits. */
static void ndsAObjEvent32CorrectMObjColors(MObj *mobj, sb32 force)
{
    AObj *aobj;

    if ((mobj == NULL) ||
        ((force == FALSE) && (mobj->anim_wait == AOBJ_ANIM_NULL)))
    {
        return;
    }

    for (aobj = mobj->aobj; aobj != NULL; aobj = aobj->next)
    {
        SYColorPack color;
        u32 base;
        u32 target;
        s32 interp;

        if ((aobj->kind == nGCAnimKindNone) ||
            (aobj->track < nGCAnimTrackPrimColor) ||
            (aobj->track > nGCAnimTrackLight2Color))
        {
            continue;
        }

        base = ndsAObjEvent32FloatBits(aobj->value_base);
        target = ndsAObjEvent32FloatBits(aobj->value_target);

        if (aobj->kind == nGCAnimKindLinear)
        {
            interp = (s32)(aobj->length * aobj->length_invert * 256.0F);
            if (interp < 0)
            {
                interp = 0;
            }
            else if (interp > 256)
            {
                interp = 256;
            }

            color.s.r = ndsAObjEvent32LerpColorChannel(
                base, target, 24u, interp);
            color.s.g = ndsAObjEvent32LerpColorChannel(
                base, target, 16u, interp);
            color.s.b = ndsAObjEvent32LerpColorChannel(
                base, target, 8u, interp);
            color.s.a = ndsAObjEvent32LerpColorChannel(
                base, target, 0u, interp);
        }
        else if (aobj->kind == nGCAnimKindStep)
        {
            u32 packed = (aobj->length_invert <= aobj->length) ?
                             target : base;

            color.s.r = (u8)(packed >> 24);
            color.s.g = (u8)(packed >> 16);
            color.s.b = (u8)(packed >> 8);
            color.s.a = (u8)packed;
        }
        else
        {
            continue;
        }

        switch (aobj->track)
        {
        case nGCAnimTrackPrimColor:
            mobj->sub.primcolor = color;
            break;
        case nGCAnimTrackEnvColor:
            mobj->sub.envcolor = color;
            break;
        case nGCAnimTrackBlendColor:
            mobj->sub.blendcolor = color;
            break;
        case nGCAnimTrackLight1Color:
            mobj->sub.light1color = color;
            break;
        case nGCAnimTrackLight2Color:
            mobj->sub.light2color = color;
            break;
        default:
            continue;
        }
        gNdsAObjEvent32ColorCorrectionCount++;
    }
}

void gcPlayMObjMatAnim(MObj *mobj)
{
    sb32 was_active = ((mobj != NULL) &&
                       (mobj->anim_wait != AOBJ_ANIM_NULL));

    ndsBaseGcPlayMObjMatAnim(mobj);
    if (was_active != FALSE)
    {
        ndsAObjEvent32CorrectMObjColors(mobj, TRUE);
    }
}

static u32 ndsAObjEvent32CollectActiveMObjs(GObj *gobj, MObj **active_mobjs)
{
    DObj *dobj;
    u32 count = 0u;

    for (dobj = (gobj != NULL) ? DObjGetStruct(gobj) : NULL;
         dobj != NULL;
         dobj = gcGetTreeDObjNext(dobj))
    {
        MObj *mobj;

        for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
        {
            if (mobj->anim_wait != AOBJ_ANIM_NULL)
            {
                if (active_mobjs != NULL)
                {
                    active_mobjs[count] = mobj;
                }
                count++;
            }
        }
    }
    return count;
}

void gcPlayAnimAll(GObj *gobj)
{
    DObj *dobj;
    u32 active_count = ndsAObjEvent32CollectActiveMObjs(gobj, NULL);
    MObj *active_mobjs[(active_count != 0u) ? active_count : 1u];
    u32 i;

    (void)ndsAObjEvent32CollectActiveMObjs(gobj, active_mobjs);

    ndsBaseGcPlayAnimAll(gobj);

    for (dobj = (gobj != NULL) ? DObjGetStruct(gobj) : NULL;
         dobj != NULL;
         dobj = gcGetTreeDObjNext(dobj))
    {
        MObj *mobj;

        for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
        {
            ndsAObjEvent32CorrectMObjColors(mobj, FALSE);
        }
    }

    /* The original player changes END to NULL after writing the last frame.
     * Correct only objects that crossed that edge during this call. */
    for (i = 0u; i < active_count; i++)
    {
        MObj *mobj = active_mobjs[i];

        if (mobj->anim_wait == AOBJ_ANIM_NULL)
        {
            ndsAObjEvent32CorrectMObjColors(mobj, TRUE);
        }
    }
}

static sb32 ndsAObjEvent32NormalizeDObjTable(GObj *gobj,
                                             AObjEvent32 **anim_joints)
{
    DObj *dobj = DObjGetStruct(gobj);

    while ((dobj != NULL) && (anim_joints != NULL))
    {
        if ((*anim_joints != NULL) &&
            (ndsRelocPointerIsFighterAObj16(*anim_joints) == FALSE) &&
            (ndsAObjEvent32NormalizeScript(
                 *anim_joints, nNDSAObjEvent32OwnerDObj) == FALSE))
        {
            return FALSE;
        }
        anim_joints++;
        dobj = gcGetTreeDObjNext(dobj);
    }
    return TRUE;
}

static sb32 ndsAObjEvent32NormalizeMObjTable(
    GObj *gobj, AObjEvent32 ***p_matanim_joints)
{
    DObj *dobj = DObjGetStruct(gobj);

    while (dobj != NULL)
    {
        if ((p_matanim_joints != NULL) && (*p_matanim_joints != NULL))
        {
            AObjEvent32 **matanim_joints = *p_matanim_joints;
            MObj *mobj = dobj->mobj;

            while (mobj != NULL)
            {
                if (ndsAObjEvent32NormalizeScript(
                        *matanim_joints, nNDSAObjEvent32OwnerMObj) == FALSE)
                {
                    return FALSE;
                }
                matanim_joints++;
                mobj = mobj->next;
            }
        }
        if (p_matanim_joints != NULL)
        {
            p_matanim_joints++;
        }
        dobj = gcGetTreeDObjNext(dobj);
    }
    return TRUE;
}

void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                        f32 anim_frame)
{
    if ((anim_joint == NULL) ||
        (ndsRelocPointerIsFighterAObj16(anim_joint) != FALSE) ||
        (ndsAObjEvent32NormalizeScript(
             anim_joint, nNDSAObjEvent32OwnerDObj) != FALSE))
    {
        ndsBaseGcAddDObjAnimJoint(dobj, anim_joint, anim_frame);
    }
}

void gcAddMObjMatAnimJoint(MObj *mobj, AObjEvent32 *matanim_joint,
                           f32 anim_frame)
{
    if (ndsAObjEvent32NormalizeScript(
            matanim_joint, nNDSAObjEvent32OwnerMObj) != FALSE)
    {
        ndsBaseGcAddMObjMatAnimJoint(mobj, matanim_joint, anim_frame);
    }
}

void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints,
                       f32 anim_frame)
{
    if ((gobj != NULL) &&
        (ndsAObjEvent32NormalizeDObjTable(gobj, anim_joints) != FALSE))
    {
        ndsBaseGcAddAnimJointAll(gobj, anim_joints, anim_frame);
    }
}

void gcAddMatAnimJointAll(GObj *gobj, AObjEvent32 ***p_matanim_joints,
                          f32 anim_frame)
{
    if ((gobj != NULL) &&
        (ndsAObjEvent32NormalizeMObjTable(gobj, p_matanim_joints) != FALSE))
    {
        ndsBaseGcAddMatAnimJointAll(gobj, p_matanim_joints, anim_frame);
    }
}

void gcAddAnimAll(GObj *gobj, AObjEvent32 **anim_joints,
                  AObjEvent32 ***p_matanim_joints, f32 anim_frame)
{
    if ((gobj != NULL) &&
        (ndsAObjEvent32NormalizeDObjTable(gobj, anim_joints) != FALSE) &&
        (ndsAObjEvent32NormalizeMObjTable(gobj, p_matanim_joints) != FALSE))
    {
        ndsBaseGcAddAnimAll(gobj, anim_joints, p_matanim_joints, anim_frame);
    }
}

void gcAddCObjCamAnimJoint(CObj *cobj, AObjEvent32 *camanim_joint,
                           f32 anim_frame)
{
    if (ndsAObjEvent32NormalizeScript(
            camanim_joint, nNDSAObjEvent32OwnerCObj) != FALSE)
    {
        ndsBaseGcAddCObjCamAnimJoint(cobj, camanim_joint, anim_frame);
    }
}
