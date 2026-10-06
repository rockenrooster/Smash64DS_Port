/* Compile the original BattleShip interpolation helpers used by objanim.c.
 * This keeps animation playback on the original math path instead of a DS
 * compatibility approximation. */
#define syInterpCubic syInterpCubicSource
#define syInterpQuad syInterpQuadSource
#include "../../decomp/BattleShip-main/decomp/src/sys/interp.c"
#undef syInterpCubic
#undef syInterpQuad

#include <nds/nds_interp_fixed.h>

/* syInterpGetFracFrame for the moving paths (Sector Z's Arwing, Samus's
 * rolls, the Board the Platforms boards).
 *
 * The Bezier/Catrom arm runs in fixed point (include/nds/nds_interp_fixed.h,
 * 2026-10-05; it replaced a bit-exact soft-float kernel, the Arwing's
 * captured flight table and a scene-heap result table, which bought
 * nothing in front of it). Around it:
 *
 * - A result memo keyed on t's bits and on every value the call reads (kind,
 *   point count, length, the two keyframes around t, the segment's five
 *   quartic coefficients, and the segment index the source's scan finds), so
 *   a repeated t -- the same roll again -- returns the stored result.
 *
 * - One bisection path per moving owner (the call's output vector): the
 *   nodes its previous call on the same segment visited. Consecutive ticks
 *   move t a little, so their bisections walk the same first nodes.
 *
 * The linear kind (one divide), an unknown kind (the source reads an
 * uninitialised value), a point count outside [2, 64], a t past the last
 * keyframe (the source's scan would leave the table) and anything outside the
 * fixed arm's domain take the source call unchanged. */
#define NDS_INTERP_FRAC_MEMO_SETS 32u
#define NDS_INTERP_FRAC_MEMO_WAYS 2u
#define NDS_INTERP_FRAC_POINTS_MAX 64
/* One slot per moving owner: the path a TraI owner's previous call walked is
 * what its next call shares. Board the Platforms runs up to ten Bezier
 * platforms a frame (Yoshi's board: ten scripts on two paths). */
#define NDS_INTERP_PATH_SLOTS 16u

typedef struct NDSInterpFracMemo
{
    u32 h1;
    u32 h2; /* forced odd when stored, so an empty entry never matches */
    u32 t_bits;
    u32 frac_bits;
} NDSInterpFracMemo;

static NDSInterpFracMemo
    sNdsInterpFracMemo[NDS_INTERP_FRAC_MEMO_SETS][NDS_INTERP_FRAC_MEMO_WAYS];
static NDSIfxPath sNdsInterpPaths[NDS_INTERP_PATH_SLOTS];
/* The output vector of the call that last wrote each slot: the moving owner. */
static const void *sNdsInterpPathOwners[NDS_INTERP_PATH_SLOTS];
static u32 sNdsInterpPathClock;

__attribute__((used)) volatile u32 gNdsInterpFracMemoHits;
__attribute__((used)) volatile u32 gNdsInterpFracMemoMisses;
__attribute__((used)) volatile u32 gNdsInterpFracSourceCalls;

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

/* The owner's own slot when it is still on these coefficients; else the
 * owner's slot re-keyed (it moved to another segment), else the oldest slot.
 * A path only ever names nodes to reuse on an exact (depth, min) match, so the
 * choice changes cost, never a result. */
static NDSIfxPath *ndsInterpPathFor(const u32 cof_bits[5], const void *owner)
{
    NDSIfxPath *target = NULL;
    NDSIfxPath *oldest = &sNdsInterpPaths[0];
    u32 target_index = 0u;
    u32 oldest_index = 0u;
    u32 i;

    sNdsInterpPathClock++;
    for (i = 0u; i < NDS_INTERP_PATH_SLOTS; i++)
    {
        NDSIfxPath *path = &sNdsInterpPaths[i];

        if (sNdsInterpPathOwners[i] == owner)
        {
            if ((path->depth != 0u) &&
                (__builtin_memcmp(path->cof, cof_bits, sizeof(path->cof)) ==
                 0))
            {
                path->age = sNdsInterpPathClock;
                return path;
            }
            if (target == NULL)
            {
                target = path;
                target_index = i;
            }
        }
        if ((path->depth == 0u) || (path->age < oldest->age))
        {
            oldest = path;
            oldest_index = i;
        }
    }
    if (target == NULL)
    {
        target = oldest;
        target_index = oldest_index;
    }
    __builtin_memcpy(target->cof, cof_bits, sizeof(target->cof));
    target->depth = 0u;
    target->age = sNdsInterpPathClock;
    sNdsInterpPathOwners[target_index] = owner;
    return target;
}

static f32 ndsInterpGetFracFrameMemo(SYInterpDesc *desc, f32 t,
                                     const void *owner)
{
    NDSInterpFracMemo *set;
    NDSInterpFracMemo hit;
    const f32 *point;
    u32 cof_bits[5];
    u32 frac_bits;
    u32 h1 = 2166136261u;
    u32 h2 = 0x9e3779b9u;
    u32 t_bits;
    u32 word;
    u32 i;
    s32 points_num;
    s32 id;

    if ((desc == NULL) ||
        ((desc->kind != nSYInterpKindBezierS3) &&
         (desc->kind != nSYInterpKindBezier) &&
         (desc->kind != nSYInterpKindCatrom)))
    {
        NDS_DIAG(gNdsInterpFracSourceCalls++);
        return syInterpGetFracFrame(desc, t);
    }
    points_num = desc->points_num;
    if ((points_num < 2) || (points_num > NDS_INTERP_FRAC_POINTS_MAX) ||
        (desc->keyframes == NULL) || (desc->quartics == NULL) ||
        ((((uintptr_t)desc->keyframes | (uintptr_t)desc->quartics) & 3u) !=
         0u) ||
        !(desc->keyframes[points_num - 1] >= t))
    {
        NDS_DIAG(gNdsInterpFracSourceCalls++);
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
            NDS_DIAG(gNdsInterpFracMemoHits++);
            return ndsInterpFracFloat(hit.frac_bits);
        }
    }
    NDS_DIAG(gNdsInterpFracMemoMisses++);
    if (ndsIfxFracFrame(t_bits, ndsInterpFracBits(desc->keyframes[id]),
                        ndsInterpFracBits(desc->length), cof_bits, (u32)id,
                        (u32)points_num, ndsInterpPathFor(cof_bits, owner),
                        &frac_bits) == 0)
    {
        NDS_DIAG(gNdsInterpFracSourceCalls++);
        frac_bits = ndsInterpFracBits(syInterpGetFracFrame(desc, t));
    }
    for (i = NDS_INTERP_FRAC_MEMO_WAYS - 1u; i > 0u; i--)
    {
        set[i] = set[i - 1u];
    }
    set[0].h1 = h1;
    set[0].h2 = h2;
    set[0].t_bits = t_bits;
    set[0].frac_bits = frac_bits;
    return ndsInterpFracFloat(frac_bits);
}

/* The source's two public entry points, word for word, with the memo in the
 * place of the direct call (decomp sys/interp.c). */
void syInterpCubic(Vec3f *out, SYInterpDesc *desc, f32 t)
{
    syInterpCubicSplineTimeFrac(out, desc,
                                ndsInterpGetFracFrameMemo(desc, t, out));
}

void syInterpQuad(Vec3f *out, SYInterpDesc *desc, f32 t)
{
    syInterpQuadSplineTimeFrac(out, desc,
                               ndsInterpGetFracFrameMemo(desc, t, out));
}
