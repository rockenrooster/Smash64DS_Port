/* Compile the original BattleShip interpolation helpers used by objanim.c.
 * This keeps animation playback on the original math path instead of a DS
 * compatibility approximation. */
#define syInterpCubic syInterpCubicSource
#define syInterpQuad syInterpQuadSource
#include "../../decomp/BattleShip-main/decomp/src/sys/interp.c"
#undef syInterpCubic
#undef syInterpQuad

/* P2-2p8: syInterpGetFracFrame without its repeated work.
 *
 * The arc-length reparametrisation bisects [0,1] until the interval is under
 * 1e-5 -- seventeen steps, each a nine-sample Simpson integral of the square
 * root of a quartic -- so one call is ~2,700 soft-float operations, ~60K ticks
 * on the ARM9. Samus's rolls (the fighter bank's only TraI bearers) evaluate
 * it on every roll tick, and Sector Z's Arwing on every tick it flies: 2,937
 * calls in one Sector Z match (2026-09-28).
 *
 * Everything below computes the source's exact result bits; it only declines
 * to repeat an evaluation whose inputs it has already seen:
 *
 * - The whole call: keyed on t's bits and on every value the call reads
 *   (kind, point count, length, the two keyframes around t, the segment's five
 *   quartic coefficients, and the segment index the source's scan finds), so a
 *   repeated t -- the same roll again -- returns the stored result.
 *
 * - The bisection path: every integral is over the left half [min, frac] of a
 *   dyadic interval, and depends only on those two values and the segment's
 *   coefficients. Consecutive ticks move t a little, so their bisections walk
 *   the same first nodes; the previous call's path for the same coefficients
 *   answers those nodes.
 *
 * - The Simpson samples: after a step left, the child integral's five even
 *   samples are the parent's samples 0..4; after a step right its first sample
 *   is the parent's last. A sample is reused only when its x bits match, and
 *   syInterpGetQuartSum itself computes every other one.
 *
 * The replica below is the source's text with those three lookups inserted;
 * every floating-point operation is the source's, in the source's order. The
 * linear kind (one divide), an unknown kind (the source reads an uninitialised
 * value), a point count outside [2, 64], or a t past the last keyframe (the
 * source's scan would leave the table) take the source call unchanged.
 * NDS_TICK_HUD builds carry an oracle (gNdsInterpFracOracle, boot-poked) that
 * also runs the source and counts any result that differs.
 *
 * Same-ROM A/B word: gNdsInterpFracMemo (1 on, 0 = every call is the source). */
#define NDS_INTERP_FRAC_MEMO_SETS 32u
#define NDS_INTERP_FRAC_MEMO_WAYS 2u
#define NDS_INTERP_FRAC_POINTS_MAX 64
#define NDS_INTERP_PATH_SLOTS 4u
/* The bisection stops once the interval is under 1e-5: seventeen halvings. */
#define NDS_INTERP_PATH_DEPTH 20u

typedef struct NDSInterpFracMemo
{
    u32 h1;
    u32 h2; /* forced odd when stored, so an empty entry never matches */
    u32 t_bits;
    u32 frac_bits;
} NDSInterpFracMemo;

typedef struct NDSInterpPathNode
{
    u32 min_bits;
    u32 frac_bits;
    u32 res_bits;
} NDSInterpPathNode;

typedef struct NDSInterpPath
{
    u32 cof[5];
    u32 depth; /* 0 = empty slot */
    u32 age;
    NDSInterpPathNode node[NDS_INTERP_PATH_DEPTH];
} NDSInterpPath;

static NDSInterpFracMemo
    sNdsInterpFracMemo[NDS_INTERP_FRAC_MEMO_SETS][NDS_INTERP_FRAC_MEMO_WAYS];
static NDSInterpPath sNdsInterpPaths[NDS_INTERP_PATH_SLOTS];
static u32 sNdsInterpPathClock;

volatile u32 gNdsInterpFracMemo __attribute__((used, section(".data"))) = 1u;
#if NDS_TICK_HUD
volatile u32 gNdsInterpFracOracle __attribute__((used, section(".data"))) = 0u;
__attribute__((used)) volatile u32 gNdsInterpFracMemoHits;
__attribute__((used)) volatile u32 gNdsInterpFracMemoMisses;
__attribute__((used)) volatile u32 gNdsInterpFracMemoBypass;
__attribute__((used)) volatile u32 gNdsInterpFracNodesReused;
__attribute__((used)) volatile u32 gNdsInterpFracNodesComputed;
__attribute__((used)) volatile u32 gNdsInterpFracSamplesReused;
__attribute__((used)) volatile u32 gNdsInterpFracOracleCompares;
__attribute__((used)) volatile u32 gNdsInterpFracOracleMismatches;
#define NDS_INTERP_FRAC_COUNT(counter) ((counter)++)
#else
#define NDS_INTERP_FRAC_COUNT(counter) ((void)0)
#endif

static inline u32 ndsInterpFracBits(f32 value)
{
    u32 bits;

    __builtin_memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static inline f32 ndsInterpFracFloat(u32 bits)
{
    f32 value;

    __builtin_memcpy(&value, &bits, sizeof(value));
    return value;
}

/* The nine samples of one Simpson integral, in syInterpGetCubicIntegralApprox's
 * x order: t, then t + k * factor for k = 1..7, then f. */
typedef struct NDSInterpSamples
{
    u32 count; /* 0 = none known */
    u32 x[9];
    u32 q[9];
} NDSInterpSamples;

static f32 ndsInterpQuart(f32 x, f32 *cof, const NDSInterpSamples *prev,
                          NDSInterpSamples *cur, u32 slot)
{
    const u32 x_bits = ndsInterpFracBits(x);
    f32 q;
    u32 i;

    for (i = 0u; i < prev->count; i++)
    {
        if (prev->x[i] == x_bits)
        {
            NDS_INTERP_FRAC_COUNT(gNdsInterpFracSamplesReused);
            cur->x[slot] = x_bits;
            cur->q[slot] = prev->q[i];
            return ndsInterpFracFloat(prev->q[i]);
        }
    }
    q = syInterpGetQuartSum(x, cof);
    cur->x[slot] = x_bits;
    cur->q[slot] = ndsInterpFracBits(q);
    return q;
}

/* syInterpGetCubicIntegralApprox (decomp sys/interp.c), operation for
 * operation, with each sample looked up among the parent integral's. */
static f32 ndsInterpCubicIntegral(f32 t, f32 f, f32 *cof,
                                  const NDSInterpSamples *prev,
                                  NDSInterpSamples *cur)
{
    f32 factor = (f - t) / 8;
    f32 sum = 0.0F;
    f32 time_scale = t + factor;
    f32 q_t;
    f32 q_f;
    s32 i;

    for (i = 2; i < 9; i++)
    {
        if (!(i & 1))
        {
            sum += 4.0F * ndsInterpQuart(time_scale, cof, prev, cur, (u32)i - 1u);
        }
        else sum += 2.0F * ndsInterpQuart(time_scale, cof, prev, cur, (u32)i - 1u);

        time_scale += factor;
    }
    q_t = ndsInterpQuart(t, cof, prev, cur, 0u);
    q_f = ndsInterpQuart(f, cof, prev, cur, 8u);
    cur->count = 9u;
    return ((q_t + sum + q_f) * factor) / 3.0F;
}

static NDSInterpPath *ndsInterpPathFor(const u32 cof_bits[5])
{
    NDSInterpPath *oldest = &sNdsInterpPaths[0];
    u32 i;

    sNdsInterpPathClock++;
    for (i = 0u; i < NDS_INTERP_PATH_SLOTS; i++)
    {
        NDSInterpPath *path = &sNdsInterpPaths[i];

        if ((path->depth != 0u) &&
            (__builtin_memcmp(path->cof, cof_bits, sizeof(path->cof)) == 0))
        {
            path->age = sNdsInterpPathClock;
            return path;
        }
        if ((path->depth == 0u) || (path->age < oldest->age))
        {
            oldest = path;
        }
    }
    __builtin_memcpy(oldest->cof, cof_bits, sizeof(oldest->cof));
    oldest->depth = 0u;
    oldest->age = sNdsInterpPathClock;
    return oldest;
}

/* syInterpGetFracFrame's Bezier/Catrom arm (decomp sys/interp.c), with the
 * segment index already found by the source's own scan. */
static f32 ndsInterpGetFracFrameReuse(SYInterpDesc *desc, f32 t, s32 id,
                                      const u32 cof_bits[5])
{
    f32 frac_frame;
    f32 time_scale;
    f32 min = 0.0F;
    f32 max = 1.0F;
    f32 res;
    f32 diff;
    f32 *cof = desc->quartics + (id * 5);
    NDSInterpPath *path = ndsInterpPathFor(cof_bits);
    NDSInterpSamples samples[2];
    u32 known = path->depth;
    u32 depth = 0u;
    u32 cur = 0u;
    u32 reuse = 1u;

    samples[0].count = 0u;
    samples[1].count = 0u;
    time_scale = (t - desc->keyframes[id]) * desc->length;

    do
    {
        frac_frame = (min + max) / 2.0F;

        if ((reuse != 0u) && (depth < known) &&
            (path->node[depth].min_bits == ndsInterpFracBits(min)) &&
            (path->node[depth].frac_bits == ndsInterpFracBits(frac_frame)))
        {
            res = ndsInterpFracFloat(path->node[depth].res_bits);
            /* The reused node's samples are not kept; the next computed
             * integral starts without a parent. */
            samples[cur].count = 0u;
            NDS_INTERP_FRAC_COUNT(gNdsInterpFracNodesReused);
        }
        else
        {
            reuse = 0u;
            res = ndsInterpCubicIntegral(min, frac_frame, cof, &samples[cur],
                                         &samples[cur ^ 1u]);
            cur ^= 1u;
            if (depth < NDS_INTERP_PATH_DEPTH)
            {
                path->node[depth].min_bits = ndsInterpFracBits(min);
                path->node[depth].frac_bits = ndsInterpFracBits(frac_frame);
                path->node[depth].res_bits = ndsInterpFracBits(res);
            }
            NDS_INTERP_FRAC_COUNT(gNdsInterpFracNodesComputed);
        }
        depth++;

        if (time_scale < (res + 0.00001F))
        {
            max = frac_frame;
        }
        else
        {
            min = frac_frame;
            time_scale -= res;
        }
        diff = (min < max) ? -(min - max) : min - max;

        if (diff < 0.00001F)
        {
            break;
        }
    }
    while ((res + 0.00001F) < time_scale || time_scale < (res - 0.00001F));

    /* The path now describes this call: its first `depth` nodes are exactly
     * the ones this bisection visited. */
    path->depth = (depth < NDS_INTERP_PATH_DEPTH) ? depth : NDS_INTERP_PATH_DEPTH;
    return ((f32) id + frac_frame) / ((f32) desc->points_num - 1.0F);
}

static f32 ndsInterpGetFracFrameMemo(SYInterpDesc *desc, f32 t)
{
    NDSInterpFracMemo *set;
    NDSInterpFracMemo hit;
    const f32 *point;
    u32 cof_bits[5];
    u32 h1 = 2166136261u;
    u32 h2 = 0x9e3779b9u;
    u32 t_bits;
    u32 word;
    u32 i;
    s32 points_num;
    s32 id;
    f32 frac;

    if ((gNdsInterpFracMemo == 0u) || (desc == NULL) ||
        ((desc->kind != nSYInterpKindBezierS3) &&
         (desc->kind != nSYInterpKindBezier) &&
         (desc->kind != nSYInterpKindCatrom)))
    {
        NDS_INTERP_FRAC_COUNT(gNdsInterpFracMemoBypass);
        return syInterpGetFracFrame(desc, t);
    }
    points_num = desc->points_num;
    if ((points_num < 2) || (points_num > NDS_INTERP_FRAC_POINTS_MAX) ||
        (desc->keyframes == NULL) || (desc->quartics == NULL) ||
        ((((uintptr_t)desc->keyframes | (uintptr_t)desc->quartics) & 3u) !=
         0u) ||
        !(desc->keyframes[points_num - 1] >= t))
    {
        NDS_INTERP_FRAC_COUNT(gNdsInterpFracMemoBypass);
        return syInterpGetFracFrame(desc, t);
    }
    /* The source's own scan (it cannot pass the last keyframe: that one was
     * just checked to be >= t). */
    id = 0;
    point = desc->keyframes;
    while (point[1] < t)
    {
        id++;
        point++;
    }
    __builtin_memcpy(cof_bits, desc->quartics + (id * 5), sizeof(cof_bits));

#define NDS_INTERP_FRAC_MIX(value)                                           \
    do                                                                       \
    {                                                                        \
        word = (value);                                                      \
        h1 = (h1 ^ word) * 16777619u;                                        \
        h2 = ((h2 << 5) | (h2 >> 27)) ^ (word * 0x85ebca6bu);                \
    } while (0)

    NDS_INTERP_FRAC_MIX((u32)desc->kind | ((u32)(u16)points_num << 16));
    NDS_INTERP_FRAC_MIX((u32)id);
    NDS_INTERP_FRAC_MIX(ndsInterpFracBits(desc->length));
    NDS_INTERP_FRAC_MIX(ndsInterpFracBits(desc->keyframes[id]));
    NDS_INTERP_FRAC_MIX(ndsInterpFracBits(desc->keyframes[id + 1]));
    for (i = 0u; i < 5u; i++)
    {
        NDS_INTERP_FRAC_MIX(cof_bits[i]);
    }
#undef NDS_INTERP_FRAC_MIX
    h2 |= 1u;

    t_bits = ndsInterpFracBits(t);
    set = sNdsInterpFracMemo[(h1 ^ t_bits ^ (t_bits >> 11)) &
                             (NDS_INTERP_FRAC_MEMO_SETS - 1u)];
    for (i = 0u; i < NDS_INTERP_FRAC_MEMO_WAYS; i++)
    {
        if ((set[i].h2 == h2) && (set[i].h1 == h1) &&
            (set[i].t_bits == t_bits))
        {
            hit = set[i];
            if (i != 0u)
            {
                /* Most recent first. */
                set[i] = set[0];
                set[0] = hit;
            }
            NDS_INTERP_FRAC_COUNT(gNdsInterpFracMemoHits);
            return ndsInterpFracFloat(hit.frac_bits);
        }
    }
    NDS_INTERP_FRAC_COUNT(gNdsInterpFracMemoMisses);
    frac = ndsInterpGetFracFrameReuse(desc, t, id, cof_bits);
#if NDS_TICK_HUD
    if (gNdsInterpFracOracle != 0u)
    {
        gNdsInterpFracOracleCompares++;
        if (ndsInterpFracBits(syInterpGetFracFrame(desc, t)) !=
            ndsInterpFracBits(frac))
        {
            gNdsInterpFracOracleMismatches++;
        }
    }
#endif
    for (i = NDS_INTERP_FRAC_MEMO_WAYS - 1u; i > 0u; i--)
    {
        set[i] = set[i - 1u];
    }
    set[0].h1 = h1;
    set[0].h2 = h2;
    set[0].t_bits = t_bits;
    set[0].frac_bits = ndsInterpFracBits(frac);
    return frac;
}

/* The source's two public entry points, word for word, with the memo in the
 * place of the direct call (decomp sys/interp.c). */
void syInterpCubic(Vec3f *out, SYInterpDesc *desc, f32 t)
{
    syInterpCubicSplineTimeFrac(out, desc, ndsInterpGetFracFrameMemo(desc, t));
}

void syInterpQuad(Vec3f *out, SYInterpDesc *desc, f32 t)
{
    syInterpQuadSplineTimeFrac(out, desc, ndsInterpGetFracFrameMemo(desc, t));
}
