/* Compile the original BattleShip interpolation helpers used by objanim.c.
 * This keeps animation playback on the original math path instead of a DS
 * compatibility approximation. */
#define syInterpCubic syInterpCubicSource
#define syInterpQuad syInterpQuadSource
#include "../../decomp/BattleShip-main/decomp/src/sys/interp.c"
#undef syInterpCubic
#undef syInterpQuad

#include <nds/nds_interp_exact.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_startup.h>
#include <sys/taskman.h>
#include <sc/scene.h>

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
/* One slot per moving owner: the path a TraI owner's previous call walked is
 * what its next call shares. Four slots keyed by segment served Samus's roll
 * and the Arwing; Board the Platforms runs up to ten Bezier platforms a frame
 * (Yoshi's board: ten scripts on two paths), which turned four slots over every
 * call, so each one bisected from scratch. */
#define NDS_INTERP_PATH_SLOTS 16u
#define NDS_INTERP_PATH_DEPTH NDS_IX_PATH_DEPTH

typedef struct NDSInterpFracMemo
{
    u32 h1;
    u32 h2; /* forced odd when stored, so an empty entry never matches */
    u32 t_bits;
    u32 frac_bits;
} NDSInterpFracMemo;

/* A path node's integral depends only on the segment's coefficients and the
 * node's interval, so the float replica below and the integer kernel
 * (nds_interp_exact.h) share one path table. */
typedef NDSIxPath NDSInterpPath;

static NDSInterpFracMemo
    sNdsInterpFracMemo[NDS_INTERP_FRAC_MEMO_SETS][NDS_INTERP_FRAC_MEMO_WAYS];
static NDSInterpPath sNdsInterpPaths[NDS_INTERP_PATH_SLOTS];
/* The output vector of the call that last wrote each slot: the moving owner. */
static const void *sNdsInterpPathOwners[NDS_INTERP_PATH_SLOTS];
static u32 sNdsInterpPathClock;

volatile u32 gNdsInterpFracMemo __attribute__((used, section(".data"))) = 1u;
/* P2-2p8 (2026-09-30): the bisection on bit patterns in ARM state
 * (include/nds/nds_interp_exact.h), host-proved bit-exact against the
 * source. Same-ROM A/B word: 1 = the integer kernel, 0 = the float replica. */
volatile u32 gNdsInterpFracKernel __attribute__((used, section(".data"))) = 1u;
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

#if defined(NDS_INTERP_FRAC_CAPTURE) && NDS_INTERP_FRAC_CAPTURE
#include <nds/arm9/cache.h>
/* LAB ONLY (Makefile NDS_INTERP_FRAC_CAPTURE): every memo-path call's segment
 * key (the memo's h1/h2), t and result, in call order, for the Sector Z Arwing
 * flight-table generator. The count keeps running past the end so a dump can
 * tell a full buffer from a complete one. */
#define NDS_INTERP_CAPTURE_MAX 8192u
__attribute__((used)) u32 gNdsInterpCapture[NDS_INTERP_CAPTURE_MAX][4];
__attribute__((used)) volatile u32 gNdsInterpCaptureCount;

static void ndsInterpCaptureRecord(u32 h1, u32 h2, u32 t_bits, u32 frac_bits)
{
    u32 n = gNdsInterpCaptureCount;

    if (n < NDS_INTERP_CAPTURE_MAX)
    {
        gNdsInterpCapture[n][0] = h1;
        gNdsInterpCapture[n][1] = h2;
        gNdsInterpCapture[n][2] = t_bits;
        gNdsInterpCapture[n][3] = frac_bits;
        /* The debugger reads memory, not the data cache. */
        DC_FlushRange(&gNdsInterpCapture[n][0], sizeof(gNdsInterpCapture[n]));
    }
    gNdsInterpCaptureCount = n + 1u;
    DC_FlushRange((const void *)&gNdsInterpCaptureCount,
                  sizeof(gNdsInterpCaptureCount));
}
#define NDS_INTERP_CAPTURE(h1, h2, t_bits, frac_bits) \
    ndsInterpCaptureRecord((h1), (h2), (t_bits), (frac_bits))
#else
#define NDS_INTERP_CAPTURE(h1, h2, t_bits, frac_bits) ((void)0)
#endif

/* P2-2p8 (2026-09-30): the Sector Z Arwing's flight table.
 *
 * The Arwing is a moving platform, so its TraI path is gameplay state on every
 * tick, and each call of its bisection costs ~62 quartics through libgcc. But
 * it flies one of eight authored patterns (grsector.c
 * dGRSectorArwingSectorDescs), each started from frame 0, so every flight of a
 * pattern asks the same (segment, t) questions. A lab capture of all eight
 * (scripts/stages/generate_sector_arwing_frac.py) holds the device's own answer
 * to each, keyed exactly as the memo below is: the segment hash h1/h2 over
 * everything syInterpGetFracFrame reads, and t's bits. An entry is used only
 * when all three match; anything else -- another stage, another animation, a t
 * the capture never saw -- is computed as before, so the table cannot change a
 * result. ndsGRSectorSetupInitAll loads it into the scene heap; residency is
 * keyed on gNdsTaskmanHeapGeneration. Same-ROM A/B word gNdsArwingFracTable
 * (0 = never consult it). */
#define NDS_ARWING_FRAC_MAGIC 0x46575241u /* 'ARWF' */
#define NDS_ARWING_FRAC_VERSION 1u
#define NDS_ARWING_FRAC_MAX_BYTES 0x20000u
/* The checked allocator halts on an overflow, and the rest of the match still
 * allocates after stage setup (fighters' effects, items, the GObj cap latch at
 * 25,600 free): the table is loaded only when this much stays free after it. */
#define NDS_ARWING_FRAC_HEAP_MARGIN 0x18000u

typedef struct NDSArwingFracSegment
{
    u32 h1;
    u32 h2;
    u32 first;
    u32 count;
} NDSArwingFracSegment;

volatile u32 gNdsArwingFracTable __attribute__((used, section(".data"))) = 1u;
#if NDS_TICK_HUD
__attribute__((used)) volatile u32 gNdsArwingFracHits;
__attribute__((used)) volatile u32 gNdsArwingFracLoads;
__attribute__((used)) volatile u32 gNdsArwingFracBytes;
__attribute__((used)) volatile u32 gNdsArwingFracFreeAtLoad;
#endif
static const NDSArwingFracSegment *sNdsArwingFracSegments;
static const u32 *sNdsArwingFracEntries; /* {t_bits, frac_bits} pairs */
static u32 sNdsArwingFracSegmentCount;
static u32 sNdsArwingFracGeneration;
static u32 sNdsArwingFracSeg;   /* the segment that answered last */
static u32 sNdsArwingFracEntry; /* the entry that answered last */

void ndsInterpArwingFracLoad(void)
{
    NdsRelocAssetStream stream;
    u32 header[4];
    u32 bytes;
    u32 free_bytes;
    u8 *body;

    sNdsArwingFracSegments = NULL;
    sNdsArwingFracEntries = NULL;
    sNdsArwingFracSegmentCount = 0u;
    if (ndsRelocAssetStreamOpen(&stream,
                                "nitro:/stages/sector_arwing_frac.bin") == FALSE)
    {
        return;
    }
    if ((ndsRelocAssetStreamRead(&stream, 0u, header, sizeof(header)) ==
         FALSE) ||
        (header[0] != NDS_ARWING_FRAC_MAGIC) ||
        (header[1] != NDS_ARWING_FRAC_VERSION) || (header[2] == 0u) ||
        (header[3] == 0u) || (header[2] > 0x1000u) || (header[3] > 0x4000u))
    {
        ndsRelocAssetStreamClose(&stream);
        return;
    }
    bytes = header[2] * (u32)sizeof(NDSArwingFracSegment) + header[3] * 8u;
    free_bytes = (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
                       (uintptr_t)gSYTaskmanGeneralHeap.ptr);
#if NDS_TICK_HUD
    gNdsArwingFracFreeAtLoad = free_bytes;
#endif
    if ((bytes > NDS_ARWING_FRAC_MAX_BYTES) ||
        (free_bytes < bytes + NDS_ARWING_FRAC_HEAP_MARGIN))
    {
        ndsRelocAssetStreamClose(&stream);
        return;
    }
    body = syTaskmanMalloc((size_t)bytes, 0x4u);
    if ((body == NULL) ||
        (ndsRelocAssetStreamRead(&stream, (u32)sizeof(header), body, bytes) ==
         FALSE))
    {
        ndsRelocAssetStreamClose(&stream);
        return;
    }
    ndsRelocAssetStreamClose(&stream);
    sNdsArwingFracSegments = (const NDSArwingFracSegment *)body;
    sNdsArwingFracEntries =
        (const u32 *)(body + header[2] * (u32)sizeof(NDSArwingFracSegment));
    sNdsArwingFracSegmentCount = header[2];
    sNdsArwingFracGeneration = gNdsTaskmanHeapGeneration;
    sNdsArwingFracSeg = 0u;
    sNdsArwingFracEntry = 0u;
#if NDS_TICK_HUD
    gNdsArwingFracLoads++;
    gNdsArwingFracBytes = bytes;
#endif
}

static sb32 ndsInterpArwingFracLookup(u32 h1, u32 h2, u32 t_bits,
                                      u32 *frac_bits)
{
    const NDSArwingFracSegment *seg;
    u32 end;
    u32 lo;
    u32 hi;
    u32 i;

    if ((sNdsArwingFracSegments == NULL) ||
        (sNdsArwingFracGeneration != gNdsTaskmanHeapGeneration))
    {
        return FALSE;
    }
    seg = &sNdsArwingFracSegments[sNdsArwingFracSeg];
    if ((seg->h1 != h1) || (seg->h2 != h2))
    {
        for (i = 0u; i < sNdsArwingFracSegmentCount; i++)
        {
            if ((sNdsArwingFracSegments[i].h1 == h1) &&
                (sNdsArwingFracSegments[i].h2 == h2))
            {
                break;
            }
        }
        if (i == sNdsArwingFracSegmentCount)
        {
            return FALSE;
        }
        sNdsArwingFracSeg = i;
        seg = &sNdsArwingFracSegments[i];
        sNdsArwingFracEntry = seg->first;
    }
    end = seg->first + seg->count;
    /* A flight asks in increasing t: this entry or the next one first. */
    i = sNdsArwingFracEntry;
    if ((i >= seg->first) && (i < end))
    {
        if (sNdsArwingFracEntries[i * 2u] == t_bits)
        {
            *frac_bits = sNdsArwingFracEntries[i * 2u + 1u];
            return TRUE;
        }
        if ((i + 1u < end) && (sNdsArwingFracEntries[(i + 1u) * 2u] == t_bits))
        {
            sNdsArwingFracEntry = i + 1u;
            *frac_bits = sNdsArwingFracEntries[(i + 1u) * 2u + 1u];
            return TRUE;
        }
    }
    /* The generator sorts each segment by t's bits (unsigned). */
    lo = seg->first;
    hi = end;
    while (lo < hi)
    {
        u32 mid = lo + ((hi - lo) >> 1);

        if (sNdsArwingFracEntries[mid * 2u] < t_bits)
        {
            lo = mid + 1u;
        }
        else
        {
            hi = mid;
        }
    }
    if ((lo < end) && (sNdsArwingFracEntries[lo * 2u] == t_bits))
    {
        sNdsArwingFracEntry = lo;
        *frac_bits = sNdsArwingFracEntries[lo * 2u + 1u];
        return TRUE;
    }
    return FALSE;
}

/* The scene-scale memo (P2-6, 2026-10-02). The 64-entry memo above holds a
 * few ticks of a few owners; Board the Platforms runs up to ten Bezier
 * platforms on looping scripts, so every tick of every platform missed and
 * bisected (Luigi's board: the platforms' arc-length solves were 43% of the
 * CPU's frame, ~580K ticks). Each platform's script replays the same t values
 * on every loop, so a table that holds one loop answers every later one.
 * Entries are keyed exactly as the small memo's (h1, h2 over every input the
 * function reads, and t's bits) and store the result the function computed
 * for those inputs, so a hit cannot change a result. Allocated from the scene
 * heap on the first miss, as large as fits (4,096 entries, 64 KB, down to
 * 1,024) while the margin stays free after it: 96 KB in a battle (a
 * four-fighter match never has the table's worth beyond that), 32 KB on a
 * bonus board (practice or campaign), which holds one fighter and no items
 * (the boards keep 91-345 KB free). Dropped with the heap generation. Open addressing over a
 * short probe; a full neighbourhood overwrites its home slot. Same-ROM A/B
 * word gNdsInterpBigMemo (0 = never consult or allocate it). */
#define NDS_INTERP_BIG_MEMO_ENTRIES_MAX 4096u
#define NDS_INTERP_BIG_MEMO_ENTRIES_MIN 1024u
#define NDS_INTERP_BIG_MEMO_PROBE 8u
#define NDS_INTERP_BIG_MEMO_HEAP_MARGIN 0x18000u
#define NDS_INTERP_BIG_MEMO_BONUS_MARGIN 0x8000u

volatile u32 gNdsInterpBigMemo __attribute__((used, section(".data"))) = 1u;
__attribute__((used)) volatile u32 gNdsInterpBigMemoHits;
__attribute__((used)) volatile u32 gNdsInterpBigMemoFills;
__attribute__((used)) volatile u32 gNdsInterpBigMemoAllocs;
static NDSInterpFracMemo *sNdsInterpBigMemo;
static u32 sNdsInterpBigMemoMask; /* entries - 1 */
static u32 sNdsInterpBigMemoGeneration;
/* Heap generation + 1 of the last allocation attempt (0 = none yet). */
static u32 sNdsInterpBigMemoTried;

static NDSInterpFracMemo *ndsInterpBigMemoTable(sb32 allocate)
{
    const u32 margin =
        ((gSCManagerBattleState != NULL) &&
         (gSCManagerBattleState->gkind >= nGRKindBonusStageStart) &&
         (gSCManagerBattleState->gkind <= nGRKindBonusStageEnd)) ?
            NDS_INTERP_BIG_MEMO_BONUS_MARGIN : NDS_INTERP_BIG_MEMO_HEAP_MARGIN;
    u32 entries = NDS_INTERP_BIG_MEMO_ENTRIES_MAX;
    u32 bytes;
    u32 free_bytes;

    if (gNdsInterpBigMemo == 0u)
    {
        return NULL;
    }
    if ((sNdsInterpBigMemo != NULL) &&
        (sNdsInterpBigMemoGeneration == gNdsTaskmanHeapGeneration))
    {
        return sNdsInterpBigMemo;
    }
    sNdsInterpBigMemo = NULL;
    if ((allocate == FALSE) ||
        (sNdsInterpBigMemoTried == gNdsTaskmanHeapGeneration + 1u))
    {
        return NULL;
    }
    sNdsInterpBigMemoTried = gNdsTaskmanHeapGeneration + 1u;
    if ((uintptr_t)gSYTaskmanGeneralHeap.end <
        (uintptr_t)gSYTaskmanGeneralHeap.ptr)
    {
        return NULL;
    }
    free_bytes = (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
                       (uintptr_t)gSYTaskmanGeneralHeap.ptr);
    while ((entries >= NDS_INTERP_BIG_MEMO_ENTRIES_MIN) &&
           (free_bytes <
            entries * (u32)sizeof(NDSInterpFracMemo) + margin))
    {
        entries >>= 1;
    }
    if (entries < NDS_INTERP_BIG_MEMO_ENTRIES_MIN)
    {
        return NULL;
    }
    bytes = entries * (u32)sizeof(NDSInterpFracMemo);
    sNdsInterpBigMemo = syTaskmanMalloc((size_t)bytes, 0x4u);
    if (sNdsInterpBigMemo == NULL)
    {
        return NULL;
    }
    /* h2 is stored odd, so a zeroed entry never matches. */
    __builtin_memset(sNdsInterpBigMemo, 0, bytes);
    sNdsInterpBigMemoMask = entries - 1u;
    sNdsInterpBigMemoGeneration = gNdsTaskmanHeapGeneration;
    gNdsInterpBigMemoAllocs++;
    return sNdsInterpBigMemo;
}

static inline u32 ndsInterpBigMemoHome(u32 h1, u32 t_bits)
{
    return ((h1 ^ (t_bits * 0x9e3779b1u)) >> 20) & sNdsInterpBigMemoMask;
}

static sb32 ndsInterpBigMemoLookup(u32 h1, u32 h2, u32 t_bits,
                                   u32 *frac_bits)
{
    const NDSInterpFracMemo *table = ndsInterpBigMemoTable(FALSE);
    u32 slot;
    u32 i;

    if (table == NULL)
    {
        return FALSE;
    }
    slot = ndsInterpBigMemoHome(h1, t_bits);
    for (i = 0u; i < NDS_INTERP_BIG_MEMO_PROBE; i++)
    {
        const NDSInterpFracMemo *e =
            &table[(slot + i) & sNdsInterpBigMemoMask];

        if (e->h2 == 0u)
        {
            return FALSE;
        }
        if ((e->h2 == h2) && (e->h1 == h1) && (e->t_bits == t_bits))
        {
            *frac_bits = e->frac_bits;
            gNdsInterpBigMemoHits++;
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsInterpBigMemoStore(u32 h1, u32 h2, u32 t_bits, u32 frac_bits)
{
    NDSInterpFracMemo *table = ndsInterpBigMemoTable(TRUE);
    NDSInterpFracMemo *e;
    u32 slot;
    u32 i;

    if (table == NULL)
    {
        return;
    }
    slot = ndsInterpBigMemoHome(h1, t_bits);
    e = &table[slot];
    for (i = 0u; i < NDS_INTERP_BIG_MEMO_PROBE; i++)
    {
        NDSInterpFracMemo *probe =
            &table[(slot + i) & sNdsInterpBigMemoMask];

        if (probe->h2 == 0u)
        {
            e = probe;
            break;
        }
    }
    e->h1 = h1;
    e->h2 = h2;
    e->t_bits = t_bits;
    e->frac_bits = frac_bits;
    gNdsInterpBigMemoFills++;
}

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

/* The owner's own slot when it is still on these coefficients; else the
 * owner's slot re-keyed (it moved to another segment), else the oldest slot.
 * A path is only ever a hint -- each node is reused only on an exact
 * (min, frac) match -- so the choice changes cost, never a result. */
static NDSInterpPath *ndsInterpPathFor(const u32 cof_bits[5],
                                       const void *owner)
{
    NDSInterpPath *target = NULL;
    NDSInterpPath *oldest = &sNdsInterpPaths[0];
    u32 target_index = 0u;
    u32 oldest_index = 0u;
    u32 i;

    sNdsInterpPathClock++;
    for (i = 0u; i < NDS_INTERP_PATH_SLOTS; i++)
    {
        NDSInterpPath *path = &sNdsInterpPaths[i];

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

/* syInterpGetFracFrame's Bezier/Catrom arm (decomp sys/interp.c), with the
 * segment index already found by the source's own scan. */
static f32 ndsInterpGetFracFrameReuse(SYInterpDesc *desc, f32 t, s32 id,
                                      const u32 cof_bits[5],
                                      const void *owner)
{
    f32 frac_frame;
    f32 time_scale;
    f32 min = 0.0F;
    f32 max = 1.0F;
    f32 res;
    f32 diff;
    f32 *cof = desc->quartics + (id * 5);
    NDSInterpPath *path = ndsInterpPathFor(cof_bits, owner);
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

/* The same Bezier/Catrom arm on bit patterns: the source's time_scale, then
 * the integer bisection, then the source's final divide. */
static f32 ndsInterpGetFracFrameKernel(SYInterpDesc *desc, f32 t, s32 id,
                                       const u32 cof_bits[5],
                                       const void *owner)
{
    f32 time_scale = (t - desc->keyframes[id]) * desc->length;
    u32 reused = 0u;
    u32 computed = 0u;
    u32 frac_bits;

    frac_bits = ndsIxBisect(cof_bits, ndsInterpFracBits(time_scale),
                            ndsInterpPathFor(cof_bits, owner), &reused,
                            &computed);
#if NDS_TICK_HUD
    gNdsInterpFracNodesReused += reused;
    gNdsInterpFracNodesComputed += computed;
#endif
    return ((f32) id + ndsInterpFracFloat(frac_bits)) /
           ((f32) desc->points_num - 1.0F);
}

static f32 ndsInterpGetFracFrameMemo(SYInterpDesc *desc, f32 t,
                                     const void *owner)
{
    NDSInterpFracMemo *set;
    NDSInterpFracMemo hit;
    const f32 *point;
    u32 cof_bits[5];
    u32 table_bits;
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
            NDS_INTERP_CAPTURE(h1, h2, t_bits, hit.frac_bits);
            return ndsInterpFracFloat(hit.frac_bits);
        }
    }
    NDS_INTERP_FRAC_COUNT(gNdsInterpFracMemoMisses);
    if (ndsInterpBigMemoLookup(h1, h2, t_bits, &table_bits) != FALSE)
    {
        frac = ndsInterpFracFloat(table_bits);
    }
    else if ((gNdsArwingFracTable != 0u) &&
        (ndsInterpArwingFracLookup(h1, h2, t_bits, &table_bits) != FALSE))
    {
#if NDS_TICK_HUD
        gNdsArwingFracHits++;
#endif
        frac = ndsInterpFracFloat(table_bits);
    }
    else
    {
        frac = (gNdsInterpFracKernel != 0u) ?
            ndsInterpGetFracFrameKernel(desc, t, id, cof_bits, owner) :
            ndsInterpGetFracFrameReuse(desc, t, id, cof_bits, owner);
        ndsInterpBigMemoStore(h1, h2, t_bits, ndsInterpFracBits(frac));
    }
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
    NDS_INTERP_CAPTURE(h1, h2, t_bits, set[0].frac_bits);
    return frac;
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
