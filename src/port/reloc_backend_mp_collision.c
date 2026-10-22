#include "nds_scene_harness_config.h"
#include <nds/nds_task37_itcm.h>
#include <nds/nds_mp_floor_crossing.h>
#include <nds/nds_mp_topology.h>
#include <nds/nds_mpprocess_source.h>
#include <nds/nds_r2_collision_mtx.h>
#include <nds/nds_r2_hwmath_unit.h>
#include <nds/nds_fixed_convert.h>

MPLineGroup gMPCollisionLineGroups[nMPLineKindEnumCount];
MPVertexInfoContainer *gMPCollisionVertexInfo;
s32 gMPCollisionLinesNum;
static MPGeometryData *sNdsMPLineGroupGeometry;
static MPGeometryData *sNdsMPTopologyGeometry;
static MPGeometryData *sNdsMPLineGroupFailedGeometry;
static MPGeometryData *sNdsMPTopologyFailedGeometry;
static sb32 sNdsMPLineGroupsReady;
/* Per line kind, the yakumono groups that HAVE lines of that kind, in
 * yakumono order, decoded once with the topology (P2-2p8, 2026-09-28). The
 * sweeps walked every group through O2R halfword reads on every query to
 * find the one or two that do (Saffron's wall sweep: ~4 groups a call, ~100
 * calls a frame). `count` is already clamped to 4096 as the sweeps clamp;
 * each sweep still applies its own yakumono-id rule. Lives in the topology's
 * allocation, valid exactly while sNdsMPKindGroupGeometry is the geometry. */
typedef struct NDSMPKindGroup
{
    u16 first;
    u16 count;
    u32 yakumono_id;
    /* min coll_pos_prev / max coll_pos_next over the group's lines */
    s16 ext_lo;
    s16 ext_hi;
    /* The sweeps' group-reject backoff: checks that failed in a row, and
     * visits left to pass without checking. Performance only. */
    u8 reject_misses;
    u8 reject_skip;
    /* The group's lines' [coll_pos_prev, coll_pos_next], sorted by low end
     * and merged: sNdsMPGroupSpans[span_first, span_first + span_count). */
    u16 span_first;
    u16 span_count;
    u16 pad;
} NDSMPKindGroup;
typedef struct NDSMPGroupSpan
{
    s16 lo;
    s16 hi;
} NDSMPGroupSpan;
static NDSMPKindGroup *sNdsMPKindGroups[nMPLineKindEnumCount];
static u8 sNdsMPKindGroupCounts[nMPLineKindEnumCount];
static NDSMPGroupSpan *sNdsMPGroupSpans;
static MPGeometryData *sNdsMPKindGroupGeometry;
/* The group rejects' slack, in coll_pos units (see ndsMPSweepGroupReject). */
#define NDS_MP_WALL_SWEEP_SLACK 4
/* A group with more lines keeps one span, its [ext_lo, ext_hi]. */
#define NDS_MP_GROUP_SPAN_MAX_LINES 256

_Static_assert(sizeof(MPVertexInfo) == 10u,
               "BattleShip MPVertexInfo ABI must remain 10 bytes");
_Static_assert(sizeof(MPLineInfo) == 18u,
               "BattleShip MPLineInfo ABI must remain 18 bytes");

static sb32 ndsMPBuildTopologyCache(void);
static void ndsMPVertexF32Reset(void);

/* Inline (P2-2p8, 2026-10-04): ~1,000 calls a frame from main-RAM Thumb
 * reached this ITCM body through an interworking call (~11 cycles a call in
 * the late-window lab profile, 11.6K cycles a frame); inline it is its six
 * loads. */
static inline __attribute__((always_inline)) sb32
ndsStageCollisionLoopGeometryReady(void)
{
    /* WHETHER THE STAGE HAS GEOMETRY, NOT WHETHER IT IS ON A LIST.
     *
     * This used to require gkind to be Dream Land, later Dream Land or Yoshi's
     * Island. Every other stage answered FALSE, so ndsMPCollisionEnsureLineGroups
     * returned before building a single line group and the stage had NO
     * COLLISION AT ALL -- fighters fell through the floor the instant the match
     * began and KO'd on repeat. The owner heard exactly that on Peach's Castle
     * and Congo Jungle, and heard Yoshi's Island play correctly, which is the
     * whitelist showing through from the other side.
     *
     * The gkind test was never the substantive one. The four pointer checks
     * below are: a stage whose geometry is missing any of them is refused here
     * and gets the same early-out it always did, so a malformed import is no
     * worse off than before, while a sound one now gets its floors. */
    return ((gMPCollisionGroundData != NULL) &&
            (gMPCollisionGeometry != NULL) &&
            (gMPCollisionGeometry->line_info != NULL) &&
            (gMPCollisionGeometry->vertex_links != NULL) &&
            (gMPCollisionGeometry->vertex_id != NULL) &&
            (gMPCollisionGeometry->vertex_data != NULL)) ? TRUE : FALSE;
}

static u16 ndsMPO2RReadU16(const void *base, u32 half_index)
{
    return (u16)ndsMPO2RReadU16Kernel(base, half_index);
}

static s16 ndsMPO2RReadS16(const void *base, u32 half_index)
{
    return (s16)ndsMPO2RReadU16(base, half_index);
}

static u32 ndsMPGeometryYakumonoCount(MPGeometryData *geometry)
{
    return (geometry != NULL) ? ndsMPO2RReadU16(geometry, 0u) : 0u;
}

static u32 ndsMPGeometryMapObjCount(MPGeometryData *geometry)
{
    return (geometry != NULL) ?
        ndsMPO2RReadU16((u8 *)geometry + 20u, 0u) : 0u;
}

static sb32 ndsMPReadMapObj(s32 index, u16 *kind, s16 *x, s16 *y)
{
    const void *base;
    u32 half_index;
    u32 mapobj_count = ndsMPGeometryMapObjCount(gMPCollisionGeometry);
    u16 value_kind;
    s16 value_x;
    s16 value_y;

    gNdsStagePupupuMapObjSourceCount = mapobj_count;
    if ((index < 0) || ((u32)index >= mapobj_count) ||
        (gMPCollisionMapObjs == NULL))
    {
        return FALSE;
    }
    base = gMPCollisionMapObjs->mapobjs;
    if (((uintptr_t)base & (sizeof(u32) - 1u)) != 0u)
    {
        NDS_DIAG(gNdsStagePupupuMapObjUnalignedReadCount++);
        return FALSE;
    }
    half_index = (u32)index * 3u;
    value_kind = ndsMPO2RReadU16(base, half_index);
    value_x = ndsMPO2RReadS16(base, half_index + 1u);
    value_y = ndsMPO2RReadS16(base, half_index + 2u);
    if ((u32)value_kind < ARRAY_COUNT(gNdsStagePupupuMapObjSourceIndices))
    {
        u32 bit = 1u << value_kind;

        if ((gNdsStagePupupuMapObjDecodedMask & bit) == 0u)
        {
            gNdsStagePupupuMapObjSourceIndices[value_kind] = (u32)index;
            gNdsStagePupupuMapObjXs[value_kind] = value_x;
            gNdsStagePupupuMapObjYs[value_kind] = value_y;
            gNdsStagePupupuMapObjDecodedMask |= bit;
        }
        else if (gNdsStagePupupuMapObjSourceIndices[value_kind] != (u32)index)
        {
            gNdsStagePupupuMapObjDuplicateMask |= bit;
        }
    }
    if (kind != NULL)
    {
        *kind = value_kind;
    }
    if (x != NULL)
    {
        *x = value_x;
    }
    if (y != NULL)
    {
        *y = value_y;
    }
    return TRUE;
}

static NDSMPO2RHalfwordView ndsMPLineInfoAt(MPLineInfo *line_info,
                                             u32 index)
{
    return ndsMPO2RLineInfoView(line_info, index);
}

static u32 ndsMPLineInfoYakumonoID(NDSMPO2RHalfwordView line_info)
{
    return ndsMPO2RLineInfoYakumonoID(line_info);
}

static u32 ndsMPLineInfoGroupID(NDSMPO2RHalfwordView line_info, u32 kind)
{
    return ndsMPO2RLineInfoGroupID(line_info, kind);
}

static u32 ndsMPLineInfoLineCount(NDSMPO2RHalfwordView line_info, u32 kind)
{
    return ndsMPO2RLineInfoLineCount(line_info, kind);
}

static void ndsMPCollisionClearTopologySnapshot(void)
{
    gNdsCollisionRuntimeDiagnostics.topology_lines = 0u;
    gNdsCollisionRuntimeDiagnostics.topology_shared_directed = 0u;
    gNdsCollisionRuntimeDiagnostics.topology_orphan_endpoints = 0u;
    gNdsCollisionRuntimeDiagnostics.topology_reversed_lines = 0u;
    gNdsCollisionRuntimeDiagnostics.topology_ambiguous_endpoints = 0u;
    gNdsCollisionRuntimeDiagnostics.topology_hash = 0u;
    gNdsCollisionRuntimeDiagnostics.topology_last_line = -1;
    gNdsCollisionRuntimeDiagnostics.topology_last_prev = -1;
    gNdsCollisionRuntimeDiagnostics.topology_last_next = -1;
}

/* The geometry (and heap generation) ndsMPWallSweepStaticMiss last proved
 * ready; every topology reset or rebuild clears it. */
static MPGeometryData *sNdsMPWallMissReadyGeometry;
static u32 sNdsMPWallMissReadyGen;

/* The geometry and ground data ndsMPCollisionEnsureLineGroups last found
 * complete (groups built, topology built or refused); every topology reset
 * clears it. Saves its pointer checks on the ~40 asks a frame. */
static MPGeometryData *sNdsMPEnsureFastGeometry;
static const void *sNdsMPEnsureFastGround;

void ndsMPCollisionInvalidateTopology(void)
{
    u32 kind;

    sNdsMPEnsureFastGeometry = NULL;
    sNdsMPWallMissReadyGeometry = NULL;
    sNdsMPLineGroupGeometry = NULL;
    sNdsMPTopologyGeometry = NULL;
    sNdsMPLineGroupFailedGeometry = NULL;
    sNdsMPTopologyFailedGeometry = NULL;
    sNdsMPLineGroupsReady = FALSE;
    sNdsMPKindGroupGeometry = NULL;
    gMPCollisionVertexInfo = NULL;
    gMPCollisionLinesNum = 0;
    ndsMPCollisionClearTopologySnapshot();
    ndsMPVertexF32Reset();
    for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
    {
        gMPCollisionLineGroups[kind].line_count = 0u;
        gMPCollisionLineGroups[kind].line_id = NULL;
    }
}

void ndsMPCollisionEnsureLineGroups(void)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    /* Zeroed on the build path below, after the early-outs: the initialisers
     * were two memsets paid on every call that found the groups ready. */
    u32 line_counts[nMPLineKindEnumCount];
    u32 write_counts[nMPLineKindEnumCount];
    u32 yakumono_count;
    u32 i;
    u32 kind;

    if ((geometry != NULL) && (geometry == sNdsMPEnsureFastGeometry) &&
        ((const void *)gMPCollisionGroundData == sNdsMPEnsureFastGround))
    {
        return;
    }
    if (geometry != sNdsMPLineGroupGeometry)
    {
        ndsMPCollisionInvalidateTopology();
    }
    if (ndsStageCollisionLoopGeometryReady() == FALSE)
    {
        ndsMPCollisionInvalidateTopology();
        return;
    }
    if ((geometry == sNdsMPLineGroupGeometry) &&
        (sNdsMPLineGroupsReady != FALSE))
    {
        if (((geometry != sNdsMPTopologyGeometry) ||
             (gMPCollisionVertexInfo == NULL)) &&
            (sNdsMPTopologyFailedGeometry != geometry))
        {
            (void)ndsMPBuildTopologyCache();
        }
        if (((geometry == sNdsMPTopologyGeometry) &&
             (gMPCollisionVertexInfo != NULL)) ||
            (sNdsMPTopologyFailedGeometry == geometry))
        {
            sNdsMPEnsureFastGeometry = geometry;
            sNdsMPEnsureFastGround = gMPCollisionGroundData;
        }
        return;
    }
    if (sNdsMPLineGroupFailedGeometry == geometry)
    {
        return;
    }
    for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
    {
        gMPCollisionLineGroups[kind].line_count = 0u;
        gMPCollisionLineGroups[kind].line_id = NULL;
        line_counts[kind] = 0u;
        write_counts[kind] = 0u;
    }
    sNdsMPLineGroupGeometry = geometry;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        sNdsMPLineGroupFailedGeometry = geometry;
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_invalid++);
        return;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(geometry->line_info, i);

        for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
        {
            u32 first = ndsMPLineInfoGroupID(info, kind);
            u32 count = ndsMPLineInfoLineCount(info, kind);

            if ((count > 4096u) || (first > 4096u) ||
                ((first + count) > 4096u) ||
                ((line_counts[kind] + count) > 4096u))
            {
                sNdsMPLineGroupFailedGeometry = geometry;
                NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_invalid++);
                return;
            }
            line_counts[kind] += count;
        }
    }
    for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
    {
        gMPCollisionLineGroups[kind].line_count = (u16)line_counts[kind];
        if (line_counts[kind] != 0u)
        {
            gMPCollisionLineGroups[kind].line_id =
                syTaskmanMalloc(line_counts[kind] * sizeof(u16), 2u);
            if (gMPCollisionLineGroups[kind].line_id == NULL)
            {
                u32 clear_kind;

                for (clear_kind = 0u;
                     clear_kind < nMPLineKindEnumCount; clear_kind++)
                {
                    gMPCollisionLineGroups[clear_kind].line_count = 0u;
                    gMPCollisionLineGroups[clear_kind].line_id = NULL;
                }
                sNdsMPLineGroupFailedGeometry = geometry;
                NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_invalid++);
                return;
            }
        }
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(geometry->line_info, i);

        for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
        {
            u32 first = ndsMPLineInfoGroupID(info, kind);
            u32 count = ndsMPLineInfoLineCount(info, kind);
            u32 line_id;

            for (line_id = first; line_id < first + count; line_id++)
            {
                gMPCollisionLineGroups[kind].line_id[write_counts[kind]++] =
                    (u16)line_id;
            }
        }
    }
    sNdsMPLineGroupsReady = TRUE;
    (void)ndsMPBuildTopologyCache();
}

static u32 ndsMPVertexLinkFirst(MPVertexLinks *links, u32 line_id)
{
    return ndsMPO2RReadU16(links, line_id * 2u);
}

static u32 ndsMPVertexLinkCount(MPVertexLinks *links, u32 line_id)
{
    return ndsMPO2RReadU16(links, (line_id * 2u) + 1u);
}

static u32 ndsMPVertexID(MPVertexArray *ids, u32 index)
{
    return ndsMPO2RReadU16(ids, index);
}

static s32 ndsMPVertexX(MPVertexPosContainer *verts, u32 vertex_id)
{
    return ndsMPO2RReadS16(verts, vertex_id * 3u);
}

static s32 ndsMPVertexY(MPVertexPosContainer *verts, u32 vertex_id)
{
    return ndsMPO2RReadS16(verts, (vertex_id * 3u) + 1u);
}

static u32 ndsMPVertexFlags(MPVertexPosContainer *verts, u32 vertex_id)
{
    return ndsMPO2RReadU16(verts, (vertex_id * 3u) + 2u);
}

/* Cycle 109: memoise the s16 -> f32 vertex conversion. Stage collision geometry
 * does not move, and the sweep was re-converting it on every query.
 *
 * `__aeabi_i2f` is **149,821 calls and 2,516,993 cycles** across the collision
 * lane in the cycle-106 whole-match profile -- 73,530 of them inside
 * `ndsStageMPSweepFloorLoopSweep` alone, where they are **77% of that
 * function's soft float**. They come from one expression, the
 * `(f32)ndsMPVertexX/Y` pairs that build `v1`/`v2` per segment per call, on
 * vertices decoded from static s16 stage data. `PROJECT_GOAL.md`'s
 * compute-once rule applies exactly.
 *
 * This is NOT what R2-03 E51 refuted. That entry killed a
 * `line_id -> (group, kind)` table because the yakumono loop has a trip count
 * of one, i.e. there was no O(n) to remove. This removes repeated CONVERSION of
 * static data, not loop iterations -- and E51's own measurement (Dream Land: 1
 * yakumono, 7 lines) is what makes the table small enough to be free.
 *
 * Fail-closed in three ways, because this is the subsystem behind BUGS.md's
 * "fighters floating under the stage": an id at or past the cap falls back to
 * the live conversion, a geometry pointer change drops every entry, and an
 * un-populated entry is computed from the same accessors the old code used. The
 * cached value is bit-identical -- it is the same `(f32)` conversion of the same
 * s16, just performed once. */
#define NDS_MP_VERTEX_F32_MAX 128u

static f32 sNdsMPVertexF32X[NDS_MP_VERTEX_F32_MAX];
static f32 sNdsMPVertexF32Y[NDS_MP_VERTEX_F32_MAX];
static u8 sNdsMPVertexF32Valid[NDS_MP_VERTEX_F32_MAX];
static MPGeometryData *sNdsMPVertexF32Geometry;

u32 gNdsMPVertexF32Hits;
u32 gNdsMPVertexF32Fills;
u32 gNdsMPVertexF32Overflow;

/* Cycle 117: the O(1) whole-line reject that the SOURCE has and this port lost.
 *
 * `mpCollisionGetFCCommon` in `decomp/.../mp/mpcollision.c` reads the first and
 * last vertex x of a line, forms a span, and returns FALSE immediately when the
 * object is outside it -- BEFORE walking any segment. This port went straight
 * into the per-segment loop, and that loop reads four vertex coordinates through
 * `ndsMPO2RReadU16` and two memoised f32s per segment before it even tests
 * whether the object is over the segment. `func_ovl2_800F8FFC` and the floor
 * loops call it once per floor line, and an object is over at most one of them,
 * so nearly all of that work is spent proving a line irrelevant.
 *
 * This is not the source's test. Taking the span from the first and last vertex
 * assumes the line is monotonic in x; taking it from ALL of them is correct for
 * any line and is what makes the reject EXACTLY equivalent to running the loop:
 * the loop's gate is "object_x lies between this segment's two x", so an
 * object_x outside the whole line's [min, max] fails that gate for every
 * segment, and the loop body -- including the `x1 == x2` guard counter, which
 * sits after the gate -- never runs. Same answer, same side effects, no
 * epsilon, no approximation.
 *
 * The span is static per line in LOCAL space, which is the space both callers
 * compare in (the floor path subtracts the yakumono translate from object_x;
 * the ceiling path goes through `ndsMPLinePositionWorldToLocal`), so a moving
 * platform does not invalidate it. It is filled on first use and dropped with
 * the vertex memo whenever the geometry pointer changes, and a line id at or
 * past the cap falls through to the loop. */
#define NDS_MP_LINE_EXTENT_MAX 64u

static f32 sNdsMPLineExtentMinX[NDS_MP_LINE_EXTENT_MAX];
static f32 sNdsMPLineExtentMaxX[NDS_MP_LINE_EXTENT_MAX];
static f32 sNdsMPLineExtentMinY[NDS_MP_LINE_EXTENT_MAX];
static f32 sNdsMPLineExtentMaxY[NDS_MP_LINE_EXTENT_MAX];
/* max_y + 0.001 and min_y - 0.001, the sweep reject's bounds: the same two
 * float operations it made per line per query, made once at fill. */
static f32 sNdsMPLineExtentMaxYEps[NDS_MP_LINE_EXTENT_MAX];
static f32 sNdsMPLineExtentMinYEps[NDS_MP_LINE_EXTENT_MAX];
static u8 sNdsMPLineExtentValid[NDS_MP_LINE_EXTENT_MAX];
/* P2-2p8 (2026-10-05): the line's vertex x order, filled with its extent
 * (ndsMPSweepSegmentWindow). */
#define NDS_MP_LINE_XORDER_NONE 0u
#define NDS_MP_LINE_XORDER_UP 1u      /* x non-decreasing along the line */
#define NDS_MP_LINE_XORDER_DOWN 2u    /* x non-increasing */
static u8 sNdsMPLineXOrder[NDS_MP_LINE_EXTENT_MAX];
/* ...and its x extent as integers (vertices are s16). */
static s16 sNdsMPLineExtentMinXi[NDS_MP_LINE_EXTENT_MAX];
static s16 sNdsMPLineExtentMaxXi[NDS_MP_LINE_EXTENT_MAX];

u32 gNdsMPLineExtentRejects;
u32 gNdsMPLineExtentAdmits;
u32 gNdsMPLineExtentOverflow;
u32 gNdsMPLineSweepRejects;
u32 gNdsMPLineSweepAdmits;

/* Cycle 118: the endpoint memo slice 29's post-mortem asked for, which slice 30
 * unblocked.
 *
 * `ndsMPFindLineEndpoints` is 21,127,734 cycles in the c117 whole-match profile
 * -- **38,890 calls at 543 cycles each, 5,861 tk/fr** -- and its per-PC detail
 * says there is nothing inside it to delete: 270 distinct PCs, the hottest one
 * only **3.6%** of the function, and the top rows are `ldr r3,[sp,#64]` and
 * `ldr r2,[sp,#4]` at **19.5 and 19.3 cyc/insn**. That is a cold frame reload on
 * entry, not a loop. The only lever a flat function offers is not entering it.
 *
 * Its answer is a pure function of the line id and the current geometry: it
 * reads `line_info`, `vertex_links`, `vertex_id` and `vertex_data`, all static
 * stage data, and nothing else. So it memoises outright rather than
 * incrementally, which is what makes this cheap enough to be worth doing.
 *
 * Three restrictions, each one a failure this subsystem has already had:
 *
 *   - **HITS ONLY.** The two FALSE exits increment
 *     `gNdsStageCollisionLoopBadVertexCount` and `…OutOfRangeLineCount`, so
 *     serving a miss from a memo would silently stop counting them. The hit path
 *     increments nothing, which is exactly why memoising only the hit is
 *     observationally identical rather than merely close.
 *   - **No bind, ever.** Cycle 117 added `ndsMPVertexF32Bind` to THIS function
 *     and the match diverged reproducibly at frames 1015/1495/1686: a bind
 *     labels the cache from the caller's pointer, and this function is reachable
 *     from 28 sites. The memo is dropped by `ndsMPVertexF32Reset`, which slice
 *     30's setter calls at the moment the geometry stops being current. That is
 *     what makes this safe now and unsafe then; do not add a bind here.
 *   - **A line id at or past the cap falls through** to the live search, like
 *     every other table in this file.
 *
 * This is NOT the refuted table. R2-03 E51 killed a `line_id -> (group, kind)`
 * table because the yakumono loop's trip count is one on Dream Land (1 yakumono,
 * 7 lines) -- there was no O(n) to remove, and this profile agrees at 2.83 inner
 * iterations a call. What this removes is the ~60 instructions of link, vertex
 * id, coordinate and `(f32)` conversion work that follow the search. */
#define NDS_MP_LINE_ENDPOINT_MAX 64u

static f32 sNdsMPLineEndpointLeftX[NDS_MP_LINE_ENDPOINT_MAX];
static f32 sNdsMPLineEndpointLeftY[NDS_MP_LINE_ENDPOINT_MAX];
static f32 sNdsMPLineEndpointRightX[NDS_MP_LINE_ENDPOINT_MAX];
static f32 sNdsMPLineEndpointRightY[NDS_MP_LINE_ENDPOINT_MAX];
static u16 sNdsMPLineEndpointFlags[NDS_MP_LINE_ENDPOINT_MAX];
static u8 sNdsMPLineEndpointCount[NDS_MP_LINE_ENDPOINT_MAX];
static u8 sNdsMPLineEndpointValid[NDS_MP_LINE_ENDPOINT_MAX];

u32 gNdsMPLineEndpointHits;
u32 gNdsMPLineEndpointFills;
u32 gNdsMPLineEndpointOverflow;

/* Slice 36, the companion. `ndsMPFindLineYakumonoID` is the same shape --
 * `line_id -> yakumono_id` over the same static geometry -- and
 * `mpCollisionGetFCCommonFloor` calls it ONCE PER CALL, 45,372 times a match,
 * inside the largest remaining collision symbol (818 cycles a call, 10,294
 * tk/fr self, 365 distinct PCs with the hottest at 1.9%: flat, same as slice 35).
 *
 * Unlike the endpoint memo this one caches MISSES too, and it may: neither of
 * its exits touches a counter, so there is no observable the memo could stop
 * incrementing. That matters because a miss is the whole cost -- a missing line
 * walks every yakumono and every kind before returning FALSE, where a hit stops
 * at the match. Caching only hits would leave the expensive half alone. */
#define NDS_MP_LINE_YAKUMONO_UNKNOWN 0u
#define NDS_MP_LINE_YAKUMONO_HIT 1u
#define NDS_MP_LINE_YAKUMONO_MISS 2u

static u8 sNdsMPLineYakumonoState[NDS_MP_LINE_ENDPOINT_MAX];
static u8 sNdsMPLineYakumonoValue[NDS_MP_LINE_ENDPOINT_MAX];

u32 gNdsMPLineYakumonoHits;
u32 gNdsMPLineYakumonoFills;

/* Slice 37, the third scan. `ndsMPGetLineKindForLineID` runs the SAME nested
 * walk over the same static geometry, and `ndsMPLineIDIsFloor` / `…IsCeil` put
 * it on every `mpCollisionGetFCCommonFloor` and `…Ceil` call.
 *
 * Slice 36's entry originally retired this on R2-03 E51's grounds -- the loop's
 * trip count is one, so there is no O(n) to remove -- and the `--pc-detail` run
 * taken to confirm that said the opposite: **47,980 calls at 194 cycles each,
 * 9,329,165 cycles, 2,588 tk/fr**, a HIGHER per-call cost than slice 36's target
 * and 97% of its total. E51 is right about the loop and says nothing about the
 * function: what costs 194 cycles is the `bl`, a nine-register prologue,
 * `ndsStageCollisionLoopGeometryReady`, the per-kind O2R halfword reads and the
 * epilogue. Its hottest PC is the epilogue at 7.0% -- flat, like every other
 * owner in this lane.
 *
 * The kind is a small enum, so state and value share ONE byte rather than the
 * two slice 36 needed: 0 = not yet resolved, 1 = the function's `-1` answer, and
 * anything else is `kind + 2`. That lets the negative answer be cached as a
 * value instead of costing a separate state array, and the whole table is 64
 * bytes -- two cache lines for every line on the stage. */
#define NDS_MP_LINE_KIND_UNKNOWN 0u
#define NDS_MP_LINE_KIND_NONE 1u /* the -1 return, memoised */
#define NDS_MP_LINE_KIND_BIAS 2u /* stored value is kind + BIAS */

static u8 sNdsMPLineKindPlus1[NDS_MP_LINE_ENDPOINT_MAX];

u32 gNdsMPLineKindHits;
u32 gNdsMPLineKindFills;


static void ndsMPVertexF32Reset(void)
{
    u32 i;

    for (i = 0u; i < NDS_MP_VERTEX_F32_MAX; i++)
    {
        sNdsMPVertexF32Valid[i] = 0u;
    }
    for (i = 0u; i < NDS_MP_LINE_EXTENT_MAX; i++)
    {
        sNdsMPLineExtentValid[i] = 0u;
    }
    for (i = 0u; i < NDS_MP_LINE_ENDPOINT_MAX; i++)
    {
        sNdsMPLineEndpointValid[i] = 0u;
        sNdsMPLineYakumonoState[i] = (u8)NDS_MP_LINE_YAKUMONO_UNKNOWN;
        sNdsMPLineKindPlus1[i] = (u8)NDS_MP_LINE_KIND_UNKNOWN;
    }
    sNdsMPVertexF32Geometry = NULL;
}

/* Call once per query, not per vertex: rebinding drops the whole table. */
static void ndsMPVertexF32Bind(MPGeometryData *geometry)
{
    if (sNdsMPVertexF32Geometry != geometry)
    {
        ndsMPVertexF32Reset();
        sNdsMPVertexF32Geometry = geometry;
    }
}

/* Cycle 117: invalidate where the invariant BREAKS, not where it is read.
 *
 * `ndsMPVertexF32Bind` above labels the cache with a geometry pointer, but every
 * entry is filled from the `verts` pointer its CALLER passes -- and those are
 * not the same thing. `gMPCollisionGeometry` is saved, swapped and restored
 * around the alternate-geometry queries below, so a bind can label the cache
 * "geometry B" while a caller still holding A's `verts` fills it with A's
 * vertices; the next reader to bind B then finds the label already equal to B,
 * skips the reset, and reads A's coordinates as if they were B's.
 *
 * That is not a missing bind, it is a misplaced one. Cycle 117 found THREE
 * readers with no bind at all (`ndsStageMPSweepFloorLoopSweep`,
 * `ndsStageMPCeilFloorLoopSweep`, `ndsMPFindLineEndpoints`) and adding binds to
 * the first two landed while adding one to the third diverged the match --
 * reproducibly, frames 1015/1495/1686 -- because it made the mislabelling
 * interleaving reachable from 28 call sites.
 *
 * Assigning through this setter resets the cache at the exact moment the data
 * it describes stops being current, so no reader can forget and the label can
 * never disagree with the fill. Use it for EVERY write to
 * `gMPCollisionGeometry`, including the restores -- a restore changes which
 * geometry is current just as much as the swap did. */
static void ndsMPCollisionSetGeometry(MPGeometryData *geometry)
{
    if (gMPCollisionGeometry != geometry)
    {
        ndsMPVertexF32Reset();
        gMPCollisionGeometry = geometry;
    }
}

/* MUST inline. Left out-of-line, the memo cost a `bl` plus two pointer stores
 * and two reloads per vertex, which is worse than the two `__aeabi_i2f` calls it
 * replaced -- the cycle-109 measurement read neutral for exactly this reason,
 * and the disassembly showed `bl ndsMPVertexF32Get` rather than an inlined
 * body. Verify with objdump after touching this, not with ticks. */
static inline void ndsMPVertexF32Get(MPVertexPosContainer *verts, u32 vertex_id,
                              f32 *out_x, f32 *out_y)
{
    if (vertex_id >= NDS_MP_VERTEX_F32_MAX)
    {
        NDS_DIAG(gNdsMPVertexF32Overflow++);
        *out_x = (f32)ndsMPVertexX(verts, vertex_id);
        *out_y = (f32)ndsMPVertexY(verts, vertex_id);
        return;
    }
    if (sNdsMPVertexF32Valid[vertex_id] == 0u)
    {
        sNdsMPVertexF32X[vertex_id] = (f32)ndsMPVertexX(verts, vertex_id);
        sNdsMPVertexF32Y[vertex_id] = (f32)ndsMPVertexY(verts, vertex_id);
        sNdsMPVertexF32Valid[vertex_id] = 1u;
        NDS_DIAG(gNdsMPVertexF32Fills++);
    }
    else
    {
        NDS_DIAG(gNdsMPVertexF32Hits++);
    }
    *out_x = sNdsMPVertexF32X[vertex_id];
    *out_y = sNdsMPVertexF32Y[vertex_id];
}

/* Returns non-zero when no segment of `line_id` can contain `object_x`, so the
 * caller may skip its whole segment loop. Every uncertain case returns 0 and
 * runs the loop, because a wrong reject here is a fighter falling through the
 * stage. `ndsMPVertexF32Bind` must have been called for the geometry. */
static void ndsMPLineExtentFillSlow(MPVertexArray *ids,
                                    MPVertexPosContainer *verts, u32 line_id,
                                    u32 vertex_first, u32 vertex_count)
{
    f32 min_x;
    f32 max_x;
    f32 min_y;
    f32 max_y;
    f32 prev_x;
    u32 up = 1u;
    u32 down = 1u;
    u32 k;

    ndsMPVertexF32Get(verts, ndsMPVertexID(ids, vertex_first), &min_x, &min_y);
    max_x = min_x;
    max_y = min_y;
    prev_x = min_x;
    for (k = 1u; k < vertex_count; k++)
    {
        f32 vx;
        f32 vy;

        ndsMPVertexF32Get(verts, ndsMPVertexID(ids, vertex_first + k),
                          &vx, &vy);
        if (NDS_FCMP_LT(vx, prev_x))
        {
            up = 0u;
        }
        if (NDS_FCMP_GT(vx, prev_x))
        {
            down = 0u;
        }
        prev_x = vx;
        if (NDS_FCMP_LT(vx, min_x))
        {
            min_x = vx;
        }
        if (NDS_FCMP_GT(vx, max_x))
        {
            max_x = vx;
        }
        if (NDS_FCMP_LT(vy, min_y))
        {
            min_y = vy;
        }
        if (NDS_FCMP_GT(vy, max_y))
        {
            max_y = vy;
        }
    }
    sNdsMPLineExtentMinX[line_id] = min_x;
    sNdsMPLineExtentMaxX[line_id] = max_x;
    sNdsMPLineExtentMinY[line_id] = min_y;
    sNdsMPLineExtentMaxY[line_id] = max_y;
    sNdsMPLineExtentMaxYEps[line_id] = max_y + 0.001F;
    sNdsMPLineExtentMinYEps[line_id] = min_y - 0.001F;
    sNdsMPLineXOrder[line_id] = (u8)((up != 0u) ? NDS_MP_LINE_XORDER_UP :
        (down != 0u) ? NDS_MP_LINE_XORDER_DOWN : NDS_MP_LINE_XORDER_NONE);
    /* Exact: the extents are conversions of s16 vertices. */
    sNdsMPLineExtentMinXi[line_id] = (s16)(s32)min_x;
    sNdsMPLineExtentMaxXi[line_id] = (s16)(s32)max_x;
    sNdsMPLineExtentValid[line_id] = 1u;
}

/* MUST stay inline: the steady state is one byte load and a branch, and both
 * rejects run once per line per query. Left as one out-of-line function, GCC
 * emitted an UNCONDITIONAL `bl` to the fill on every call -- the whole cold
 * loop hoisted above the test that makes it cold -- which is the same mistake
 * `ndsMPVertexF32Get` documents one screen up. Check with objdump. */
static inline int ndsMPLineExtentReady(MPVertexArray *ids,
                                       MPVertexPosContainer *verts, u32 line_id,
                                       u32 vertex_first, u32 vertex_count)
{
    if (line_id >= NDS_MP_LINE_EXTENT_MAX)
    {
        NDS_DIAG(gNdsMPLineExtentOverflow++);
        return 0;
    }
    if (sNdsMPLineExtentValid[line_id] == 0u)
    {
        ndsMPLineExtentFillSlow(ids, verts, line_id, vertex_first,
                                vertex_count);
    }
    return 1;
}

/* 2026-09-09 this 252-byte reject helper left ITCM to fix a 56-byte link
 * overflow (~1,317 I-cache-fill ticks/frame of ITCM benefit at 32.60 calls a
 * frame). 2026-09-27: back in ITCM, in the room the old fighter matrix builder
 * left (census: ~1.3K ticks/frame of non-memory stall in main RAM). */
static int NDS_R2_ITCM_PACK2_CODE
ndsMPLineExtentRejects(MPVertexArray *ids, MPVertexPosContainer *verts,
                       u32 line_id, u32 vertex_first, u32 vertex_count,
                       f32 object_x)
{
    if (ndsMPLineExtentReady(ids, verts, line_id, vertex_first,
                             vertex_count) == 0)
    {
        return 0;
    }
    if (NDS_FCMP_LT(object_x, sNdsMPLineExtentMinX[line_id]) ||
        NDS_FCMP_GT(object_x, sNdsMPLineExtentMaxX[line_id]))
    {
        NDS_DIAG(gNdsMPLineExtentRejects++);
        return 1;
    }
    NDS_DIAG(gNdsMPLineExtentAdmits++);
    return 0;
}

/* The sweep's whole-line reject: `ndsMPFCSegmentCrossesKernel`'s own y
 * bounding-box gate, hoisted from the segment to the line.
 *
 * Both of the kernel's gate branches are symmetric in position/translate --
 * with `motion_dy > 0` the translate is the lower y and with it <= 0 the
 * position is -- so both say "the sweep's y span misses the segment's, allowing
 * 0.001 of slack". A line whose y span misses the sweep's therefore has every
 * one of its segments rejected by that gate.
 *
 * The kernel's FLAT branch (`sy == 0`) does not run that gate, so it is checked
 * separately rather than assumed: for a floor it requires
 * `translate_y < v1_y <= position_y + 0.001`, which places the segment's y
 * inside the sweep's span with the same slack, so this reject cannot fire on a
 * case it would accept. The `saw_flat_ascending_sweep` side effect above the
 * call is stricter still -- it wants v1.y inside the span with no slack at all.
 *
 * X is deliberately NOT part of this test even though the kernel gates on it
 * too. The flat branch's hit x is an extrapolation that can land marginally
 * outside the sweep's x span when the sweep is nearly vertical, by an amount
 * bounded by the sweep's aspect ratio rather than by epsilon, so an x reject
 * would be *nearly* exact -- and this subsystem is the one behind BUGS.md's
 * fighters floating under the stage. The y half is provably exact and, on Dream
 * Land, is the discriminating axis anyway: the main floor and the three
 * pass-through platforms sit at four distinct heights. */
/* P2-6 (2026-10-02): inline in the two sweeps that call it. Out of line in
 * ITCM it was a long call per line from their main-RAM Thumb (the Race: 256
 * calls a frame at 131 cycles each, 1p-pf12r-st11). */
/* The sweep's y span as compare keys (ndsFcmpKey), made once per group by
 * the sweeps instead of per line (2026-10-19): key_min_y is the lesser key of
 * position.y and translate.y, key_max_y the greater. */
static inline int ndsMPLineExtentSweepRejectsKeys(MPVertexArray *ids,
                                                  MPVertexPosContainer *verts,
                                                  u32 line_id, u32 vertex_first,
                                                  u32 vertex_count,
                                                  u32 key_min_y, u32 key_max_y)
{
    if (ndsMPLineExtentReady(ids, verts, line_id, vertex_first,
                             vertex_count) == 0)
    {
        return 0;
    }
    if ((ndsFcmpKey(sNdsMPLineExtentMaxYEps[line_id]) < key_min_y) ||
        (key_max_y < ndsFcmpKey(sNdsMPLineExtentMinYEps[line_id])))
    {
        NDS_DIAG(gNdsMPLineSweepRejects++);
        return 1;
    }
    NDS_DIAG(gNdsMPLineSweepAdmits++);
    return 0;
}

/* A flat segment's height is v1y, bit for bit (P2-2p8, 2026-10-06; owner:
 * "Software floating point should not exist"): with v1y == v2y the factor
 * (v2y - v1y) is +0, a finite quotient times +0 is a signed zero, and v1y plus
 * a signed zero is v1y -- v1y converts an s16, so it is never -0. The quotient
 * is finite when opx is finite and v1x != v2x, which the test requires (a
 * vertical segment keeps the expression, NaN and all). Every flat floor and
 * ceiling skips three subtracts, a divide, a multiply and an add. */
static inline s32 ndsMPFlatSegmentExact(f32 opx, u32 v1x_bits, u32 v2x_bits,
                                        u32 v1y_bits, u32 v2y_bits)
{
    return (v1y_bits == v2y_bits) && (v1x_bits != v2x_bits) &&
           ((ndsFcmpBits(opx) & 0x7F800000u) != 0x7F800000u);
}

static f32 ndsMPLineDistanceFC(f32 opx, s32 v1x, s32 v1y, s32 v2x,
                               s32 v2y)
{
    if (ndsMPFlatSegmentExact(opx, (u32)v1x, (u32)v2x, (u32)v1y, (u32)v2y))
    {
        return (f32)v1y;
    }
    return (f32)v1y + (((opx - (f32)v1x) / ((f32)v2x - (f32)v1x)) *
        ((f32)v2y - (f32)v1y));
}

/* The floor's height over x, less the point's y, in Q12 (P2-2p8, 2026-10-06;
 * owner: "Software floating point should not exist, fixed point only"; ruling
 * D13 re-baselines the digest). The vertices are the map's integers, so the
 * height v1y + (x - v1x)(v2y - v1y) / (v2x - v1x) is one 64-bit product and a
 * math-unit divide on the point's Q12 x (rounded toward zero, as the divide
 * does): the source's three float subtracts, divide, multiply and add, then
 * the subtract of y, were ~400 ticks a hit (13 hits a frame on Peach's
 * Castle). The point is taken at Q12 (round to nearest, 2^-12 units); a
 * coordinate past the Q12 range keeps the float expression. */
/* ndsMPFloorDistQ12's float form, for a point past the Q12 range. */
static f32 __attribute__((noinline, cold))
ndsMPFloorDistFloat(f32 object_x, f32 object_y, s32 v1x, s32 v1y, s32 v2x,
                    s32 v2y)
{
    return ndsMPLineDistanceFC(object_x, v1x, v1y, v2x, v2y) - object_y;
}

static f32 __attribute__((noinline, target("arm")))
ndsMPFloorDistQ12(f32 object_x, f32 object_y, s32 v1x, s32 v1y, s32 v2x,
                  s32 v2y)
{
    s32 x;
    s32 y;
    s64 h;

    x = ndsR2CollisionF32ToFixed(object_x, 12u);
    y = ndsR2CollisionF32ToFixed(object_y, 12u);
    if ((x == NDS_R2_COLLISION_F32_OVERFLOW) ||
        (y == NDS_R2_COLLISION_F32_OVERFLOW))
    {
        return ndsMPFloorDistFloat(object_x, object_y, v1x, v1y, v2x, v2y);
    }
    h = ((s64)v1y << 12) +
        ndsR2HwMathDivideFast(((s64)x - ((s64)v1x << 12)) *
                                  (s64)(v2y - v1y),
                              (s64)(v2x - v1x));
    return ndsFixed64ToF32(h - y, 12u);
}

/* The unit slope of a collision segment, memoized (P2-2p8, 2026-10-05; owner:
 * "Software floating point should not exist, fixed point only"). Every floor/
 * ceiling (FC) and wall (LR) normal below is built from py = -(a / b) and
 * inv_len = 1 / sqrtf(py * py + 1) over two integer vertex differences -- two
 * divides, a square root and four more float operations -- so the pair
 * (py * inv_len, inv_len) is a pure function of (a, b), and stage vertices are
 * static. A direct-mapped table serves every repeat bit for bit; only a
 * slope's first visit runs the float expressions, the same ones, so a cached
 * pair is exactly what they compute. Negating a product negates its operand's
 * product exactly, so the -py * inv_len arms are -px. No caller passes a zero
 * (both guard a == 0 and b == 0 first), so the zeroed table holds no false
 * key. */
#define NDS_MP_SLOPE_MEMO 32u

typedef struct NDSMPSlopeMemo
{
    s32 a;
    s32 b;
    f32 px;
    f32 il;
} NDSMPSlopeMemo;

static NDSMPSlopeMemo sNdsMPSlopeMemo[NDS_MP_SLOPE_MEMO];

static void ndsMPSlopeUnit(s32 a, s32 b, f32 *px, f32 *il)
{
    NDSMPSlopeMemo *m = &sNdsMPSlopeMemo[
        (((u32)a * 0x9E3779B1u) ^ ((u32)b * 0x85EBCA77u)) >> 27];

    if ((m->a != a) || (m->b != b))
    {
        const f32 py = -((f32)a / (f32)b);
        const f32 inv_len = 1.0F / sqrtf((py * py) + 1.0F);

        m->a = a;
        m->b = b;
        m->px = py * inv_len;
        m->il = inv_len;
    }
    *px = m->px;
    *il = m->il;
}

/* An s16 vertex coordinate held as an exact float (ndsMPVertexF32Get), back
 * to its integer from the bits. */
static inline s32 ndsMPVertexFloatInt(f32 v)
{
    u32 bits;
    u32 exponent;
    s32 magnitude;

    __builtin_memcpy(&bits, &v, sizeof(bits));
    exponent = (bits >> 23) & 0xFFu;
    if (exponent < 127u)
    {
        return 0;
    }
    magnitude = (s32)(((bits & 0x7FFFFFu) | 0x800000u) >> (150u - exponent));
    return ((bits & 0x80000000u) != 0u) ? -magnitude : magnitude;
}

/* `ud` is +1 or -1 at every call site, so (f32)ud is +-1.0F. */
static void ndsMPGetFCAngle(Vec3f *angle, s32 v1x, s32 v1y, s32 v2x,
                            s32 v2y, s32 ud)
{
    f32 px;
    f32 il;

    if (angle == NULL)
    {
        return;
    }
    angle->z = 0.0F;
    if (v2y == v1y)
    {
        angle->x = 0.0F;
        angle->y = (ud < 0) ? -1.0F : 1.0F;
        return;
    }
    if (v2x == v1x)
    {
        NDS_DIAG(gNdsStageCollisionLoopDivisionGuardCount++);
        angle->x = 0.0F;
        angle->y = (ud < 0) ? -1.0F : 1.0F;
        return;
    }
    ndsMPSlopeUnit(v2y - v1y, v2x - v1x, &px, &il);
    if (ud < 0)
    {
        angle->x = -px;
        angle->y = -il;
    }
    else
    {
        angle->x = px;
        angle->y = il;
    }
}

/* ndsMPGetFCAngle on cached vertex floats, which hold the s16 coordinates
 * exactly. */
static void ndsMPGetFCAnglef(Vec3f *angle, f32 v1x, f32 v1y, f32 v2x, f32 v2y,
                             s32 ud)
{
    ndsMPGetFCAngle(angle, ndsMPVertexFloatInt(v1x), ndsMPVertexFloatInt(v1y),
                    ndsMPVertexFloatInt(v2x), ndsMPVertexFloatInt(v2y), ud);
}

/* R2-03 E51 measured this scan and the table it suggested is REFUTED: Dream
 * Land reports gNdsStageCollisionLoopYakumonoCount = 1 and 7 lines total, so the
 * `i < yakumono_count` loop below has a trip count of ONE. Do not re-propose a
 * precomputed line_id -> (group, kind) table; there is no O(n) here to remove.
 * Board entry: docs/P1_EXECUTION_BOARD.md, R2-03 E51.
 *
 * Slice 37 is NOT that table, and the distinction is the whole point. E51 is
 * about the LOOP, and it is still right: there is no O(n) here. The memo below
 * removes what the profile actually charges -- 194 cycles a call over 47,980
 * calls, of which the loop is a small part and the `bl`, the nine-register
 * prologue, `ndsStageCollisionLoopGeometryReady`, the per-kind O2R halfword
 * reads and the epilogue are the rest. Slice 36's board entry retired this on
 * E51's authority before measuring it, and had to withdraw that. */
static s32 NDS_R2_ITCM_PACK2_CODE ndsMPGetLineKindForLineID(s32 line_id)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;
    u32 kind;

    if ((line_id < 0) || (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return -1;
    }
    /* Both outcomes memoised. Neither exit touches a counter, so a served answer
     * is observationally identical, and the -1 is the expensive one: it walks
     * every yakumono and every kind before returning. */
    if ((u32)line_id < NDS_MP_LINE_ENDPOINT_MAX)
    {
        u32 cached = sNdsMPLineKindPlus1[(u32)line_id];

        if (cached != NDS_MP_LINE_KIND_UNKNOWN)
        {
            NDS_DIAG(gNdsMPLineKindHits++);
            return (cached == NDS_MP_LINE_KIND_NONE) ?
                -1 : ((s32)cached - (s32)NDS_MP_LINE_KIND_BIAS);
        }
    }
    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);

        for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
        {
            s32 first = (s32)ndsMPLineInfoGroupID(info, kind);
            s32 count = (s32)ndsMPLineInfoLineCount(info, kind);
            s32 last;

            if (count <= 0)
            {
                continue;
            }
            if (count > 4096)
            {
                count = 4096;
            }
            last = first + count;
            if ((line_id >= first) && (line_id < last))
            {
                if (((u32)line_id < NDS_MP_LINE_ENDPOINT_MAX) &&
                    ((kind + NDS_MP_LINE_KIND_BIAS) <= 0xffu))
                {
                    sNdsMPLineKindPlus1[(u32)line_id] =
                        (u8)(kind + NDS_MP_LINE_KIND_BIAS);
                    NDS_DIAG(gNdsMPLineKindFills++);
                }
                return (s32)kind;
            }
        }
    }
    if ((u32)line_id < NDS_MP_LINE_ENDPOINT_MAX)
    {
        sNdsMPLineKindPlus1[(u32)line_id] = (u8)NDS_MP_LINE_KIND_NONE;
        NDS_DIAG(gNdsMPLineKindFills++);
    }
    return -1;
}

static sb32 ndsMPLineIDIsFloor(s32 line_id)
{
    return (ndsMPGetLineKindForLineID(line_id) == nMPLineKindFloor) ?
        TRUE : FALSE;
}

static sb32 ndsMPLineIDIsCeil(s32 line_id)
{
    return (ndsMPGetLineKindForLineID(line_id) == nMPLineKindCeil) ?
        TRUE : FALSE;
}

static void ndsStageCollisionLoopCountLines(void)
{
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;
    u32 j;

    gNdsStageCollisionLoopGroundDataReady =
        (gMPCollisionGroundData != NULL) ? 1u : 0u;
    gNdsStageCollisionLoopGeometryReady =
        (ndsStageCollisionLoopGeometryReady() != FALSE) ? 1u : 0u;
    gNdsStageCollisionLoopYakumonoCount = 0u;
    gNdsStageCollisionLoopMapObjCount = 0u;
    gNdsStageCollisionLoopFloorLineCount = 0u;
    gNdsStageCollisionLoopTotalLineCount = 0u;
    gNdsStageCollisionLoopFloorGroupID = -1;
    gNdsStageCollisionLoopFloorGroupCount = 0u;
    gNdsStageCollisionLoopFloorLineMin = -1;
    gNdsStageCollisionLoopFloorLineMaxExclusive = -1;

    if (gMPCollisionGeometry == NULL)
    {
        return;
    }
    gNdsStageCollisionLoopYakumonoCount =
        ndsMPGeometryYakumonoCount(gMPCollisionGeometry);
    gNdsStageCollisionLoopMapObjCount =
        ndsMPGeometryMapObjCount(gMPCollisionGeometry);
    if (gMPCollisionGeometry->line_info == NULL)
    {
        return;
    }
    line_info = gMPCollisionGeometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(gMPCollisionGeometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);

        for (j = 0u; j < nMPLineKindEnumCount; j++)
        {
            u32 count = ndsMPLineInfoLineCount(info, j);

            if (count > 4096u)
            {
                count = 4096u;
            }
            NDS_DIAG(gNdsStageCollisionLoopTotalLineCount += count);
            if (j == nMPLineKindFloor)
            {
                s32 first = (s32)ndsMPLineInfoGroupID(info, j);
                s32 last = first + (s32)count;

                gNdsStageCollisionLoopFloorLineCount += count;
                gNdsStageCollisionLoopFloorGroupCount += count;
                if (count > 0u)
                {
                    if (gNdsStageCollisionLoopFloorGroupID < 0)
                    {
                        gNdsStageCollisionLoopFloorGroupID = first;
                    }
                    if ((gNdsStageCollisionLoopFloorLineMin < 0) ||
                        (first < gNdsStageCollisionLoopFloorLineMin))
                    {
                        gNdsStageCollisionLoopFloorLineMin = first;
                    }
                    if ((gNdsStageCollisionLoopFloorLineMaxExclusive < 0) ||
                        (last >
                         gNdsStageCollisionLoopFloorLineMaxExclusive))
                    {
                        gNdsStageCollisionLoopFloorLineMaxExclusive = last;
                    }
                }
            }
        }
    }
}

static sb32 ndsMPFindLineEndpoints(s32 line_id, Vec3f *left, Vec3f *right,
                                   u32 *flags, s32 *vertex_count)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    MPVertexLinks *links;
    MPVertexArray *ids;
    MPVertexPosContainer *verts;
    u32 yakumono_count;
    u32 i;
    u32 kind;

    if (vertex_count != NULL)
    {
        *vertex_count = 0;
    }
    if (flags != NULL)
    {
        *flags = 0u;
    }
    if ((line_id < 0) || (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    /* The geometry-ready guard stays ABOVE this: a memo entry says what the
     * geometry held, never that it is still current to read. */
    if (((u32)line_id < NDS_MP_LINE_ENDPOINT_MAX) &&
        (sNdsMPLineEndpointValid[(u32)line_id] != 0u))
    {
        u32 memo = (u32)line_id;

        NDS_DIAG(gNdsMPLineEndpointHits++);
        if (vertex_count != NULL)
        {
            *vertex_count = (s32)sNdsMPLineEndpointCount[memo];
        }
        if (flags != NULL)
        {
            *flags = sNdsMPLineEndpointFlags[memo];
        }
        if (left != NULL)
        {
            left->x = sNdsMPLineEndpointLeftX[memo];
            left->y = sNdsMPLineEndpointLeftY[memo];
            left->z = 0.0F;
        }
        if (right != NULL)
        {
            right->x = sNdsMPLineEndpointRightX[memo];
            right->y = sNdsMPLineEndpointRightY[memo];
            right->z = 0.0F;
        }
        return TRUE;
    }
    line_info = geometry->line_info;
    links = geometry->vertex_links;
    ids = geometry->vertex_id;
    verts = geometry->vertex_data;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);

        for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
        {
            s32 first = (s32)ndsMPLineInfoGroupID(info, kind);
            s32 last = first + (s32)ndsMPLineInfoLineCount(info, kind);

            if ((line_id >= first) && (line_id < last))
            {
                u32 count = ndsMPVertexLinkCount(links, (u32)line_id);
                u32 first_vertex = ndsMPVertexLinkFirst(links, (u32)line_id);
                u32 last_vertex;
                u32 v_first_id;
                u32 v_last_id;
                s32 v_first_x;
                s32 v_first_y;
                s32 v_last_x;
                s32 v_last_y;
                s32 left_x;
                s32 left_y;
                s32 right_x;
                s32 right_y;
                u32 line_flags;

                if ((count < 2u) || (count > 128u))
                {
                    NDS_DIAG(gNdsStageCollisionLoopBadVertexCount++);
                    return FALSE;
                }
                last_vertex = first_vertex + count - 1u;
                v_first_id = ndsMPVertexID(ids, first_vertex);
                v_last_id = ndsMPVertexID(ids, last_vertex);
                v_first_x = ndsMPVertexX(verts, v_first_id);
                v_first_y = ndsMPVertexY(verts, v_first_id);
                v_last_x = ndsMPVertexX(verts, v_last_id);
                v_last_y = ndsMPVertexY(verts, v_last_id);
                /* Resolved into locals rather than straight into the caller's
                 * `Vec3f`s so the memo can be filled even on the calls that ask
                 * for only `flags`, or for nothing at all
                 * (`mpCollisionCheckExistLineID` passes five NULLs). Those are
                 * the calls the memo has to serve, so they are the ones that
                 * have to fill it. Same expression, same order, same values --
                 * `(f32)` of an s32 is exact, so a served entry is bit-identical
                 * to a recomputed one. */
                left_x = (v_first_x <= v_last_x) ? v_first_x : v_last_x;
                left_y = (v_first_x <= v_last_x) ? v_first_y : v_last_y;
                right_x = (v_first_x <= v_last_x) ? v_last_x : v_first_x;
                right_y = (v_first_x <= v_last_x) ? v_last_y : v_first_y;
                line_flags = ndsMPVertexFlags(verts, v_first_id);

                /* The fill is not route-gated, but do NOT read that as "both
                 * arms pay the same fill" -- the measurement says otherwise and
                 * an earlier version of this comment claimed it. The candidate
                 * returns from the fast path ABOVE, so it fills 10 times a
                 * match; the control never reads the memo, so it re-fills on
                 * every call, 48,060 times. The control is therefore slightly
                 * SLOWER than the true pre-slice baseline, and the measured
                 * -7,232 P95 overstates the win by roughly the cost of those
                 * fills (~200 tk/fr). Left un-gated anyway because gating it
                 * would put a second route test on the hot path to make an
                 * already-decisive number 3% prettier. */
                if ((u32)line_id < NDS_MP_LINE_ENDPOINT_MAX)
                {
                    u32 memo = (u32)line_id;

                    sNdsMPLineEndpointLeftX[memo] = (f32)left_x;
                    sNdsMPLineEndpointLeftY[memo] = (f32)left_y;
                    sNdsMPLineEndpointRightX[memo] = (f32)right_x;
                    sNdsMPLineEndpointRightY[memo] = (f32)right_y;
                    sNdsMPLineEndpointFlags[memo] = (u16)line_flags;
                    sNdsMPLineEndpointCount[memo] = (u8)count;
                    sNdsMPLineEndpointValid[memo] = 1u;
                    NDS_DIAG(gNdsMPLineEndpointFills++);
                }
                else
                {
                    NDS_DIAG(gNdsMPLineEndpointOverflow++);
                }

                if (vertex_count != NULL)
                {
                    *vertex_count = (s32)count;
                }
                if (flags != NULL)
                {
                    *flags = line_flags;
                }
                if (left != NULL)
                {
                    left->x = (f32)left_x;
                    left->y = (f32)left_y;
                    left->z = 0.0F;
                }
                if (right != NULL)
                {
                    right->x = (f32)right_x;
                    right->y = (f32)right_y;
                    right->z = 0.0F;
                }
                return TRUE;
            }
        }
    }
    NDS_DIAG(gNdsStageCollisionLoopOutOfRangeLineCount++);
    return FALSE;
}

static sb32 ndsMPFindLineYakumonoID(s32 line_id, u32 *yakumono_id);

/* mpcollision.c:4094-4113: a line exists only while its yakumono is not
 * switched off (Off or Hidden). Checking the geometry alone kept a fighter's
 * floor line "live" after the stage switched its platform off (a Yoshi's
 * Island cloud evaporating, the Sector Z Arwing leaving its lane), so
 * mpProcessCheckTestFloorCollisionNew skipped the source's fall: the floor
 * test failed on the off platform, and its edge branch set the fighter's y
 * from the (0, 0) edge an off line reports -- owner: the fighter "teleports
 * down, then falls". The AI, items and weapons ask the same question.
 * The group lookup (memoised, ITCM) is also the line-id validation. */
sb32 mpCollisionCheckExistLineID(s32 line_id)
{
    u32 yakumono_id;
    const DObj *yakumono_dobj;

    if ((ndsMPFindLineYakumonoID(line_id, &yakumono_id) == FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
    {
        return FALSE;
    }
    yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];

    return ((yakumono_dobj != NULL) &&
            (yakumono_dobj->user_data.s < nMPYakumonoStatusOff)) ? TRUE : FALSE;
}

static sb32 ndsMPFindLineYakumonoID(s32 line_id, u32 *yakumono_id)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;
    u32 kind;

    if (yakumono_id != NULL)
    {
        *yakumono_id = 0u;
    }
    if ((line_id < 0) || (yakumono_id == NULL) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }

    /* Slice 36. Both outcomes are memoised -- see the table's comment: neither
     * exit of this function touches a counter, so a served answer is
     * observationally identical, and the MISS is the expensive one to serve. */
    if ((u32)line_id < NDS_MP_LINE_ENDPOINT_MAX)
    {
        u32 state = sNdsMPLineYakumonoState[(u32)line_id];

        if (state != NDS_MP_LINE_YAKUMONO_UNKNOWN)
        {
            NDS_DIAG(gNdsMPLineYakumonoHits++);
            if (state == NDS_MP_LINE_YAKUMONO_MISS)
            {
                return FALSE;
            }
            *yakumono_id = sNdsMPLineYakumonoValue[(u32)line_id];
            return TRUE;
        }
    }

    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);

        for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
        {
            s32 first = (s32)ndsMPLineInfoGroupID(info, kind);
            s32 count = (s32)ndsMPLineInfoLineCount(info, kind);
            s32 last;

            if (count <= 0)
            {
                continue;
            }
            if (count > 4096)
            {
                count = 4096;
            }
            last = first + count;
            if ((line_id >= first) && (line_id < last))
            {
                u32 found = ndsMPLineInfoYakumonoID(info);

                *yakumono_id = found;
                /* Only ids that fit the u8 slot are stored. A yakumono id past
                 * 255 falls through to the search forever rather than being
                 * truncated into a DIFFERENT platform's dobj -- which is the
                 * shape of a fighter standing on the wrong thing, and is not a
                 * failure worth saving 543 cycles for. */
                if (((u32)line_id < NDS_MP_LINE_ENDPOINT_MAX) &&
                    (found <= 0xffu))
                {
                    sNdsMPLineYakumonoValue[(u32)line_id] = (u8)found;
                    sNdsMPLineYakumonoState[(u32)line_id] =
                        (u8)NDS_MP_LINE_YAKUMONO_HIT;
                    NDS_DIAG(gNdsMPLineYakumonoFills++);
                }
                return TRUE;
            }
        }
    }
    if ((u32)line_id < NDS_MP_LINE_ENDPOINT_MAX)
    {
        sNdsMPLineYakumonoState[(u32)line_id] = (u8)NDS_MP_LINE_YAKUMONO_MISS;
        NDS_DIAG(gNdsMPLineYakumonoFills++);
    }
    return FALSE;
}

static sb32 ndsMPGetLineYakumonoDObj(s32 line_id, DObj **yakumono_dobj)
{
    u32 yakumono_id = 0u;
    DObj *dobj;

    if (yakumono_dobj != NULL)
    {
        *yakumono_dobj = NULL;
    }
    if ((yakumono_dobj == NULL) ||
        (ndsMPFindLineYakumonoID(line_id, &yakumono_id) == FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
    {
        return FALSE;
    }
    dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
    if ((dobj == NULL) ||
        (dobj->user_data.s >= nMPYakumonoStatusOff))
    {
        return FALSE;
    }
    *yakumono_dobj = dobj;
    return TRUE;
}

static sb32 ndsMPLinePositionLocalToWorld(s32 line_id, Vec3f *position)
{
    DObj *yakumono_dobj;

    if ((position == NULL) ||
        (ndsMPGetLineYakumonoDObj(line_id, &yakumono_dobj) == FALSE))
    {
        return FALSE;
    }
    if ((yakumono_dobj->anim_joint.event32 != NULL) ||
        (yakumono_dobj->user_data.s != nMPYakumonoStatusNone))
    {
        position->x += yakumono_dobj->translate.vec.f.x;
        position->y += yakumono_dobj->translate.vec.f.y;
    }
    return TRUE;
}

static sb32 ndsMPLinePositionWorldToLocal(s32 line_id,
                                          const Vec3f *world_position,
                                          Vec3f *local_position)
{
    DObj *yakumono_dobj;

    if ((world_position == NULL) || (local_position == NULL) ||
        (ndsMPGetLineYakumonoDObj(line_id, &yakumono_dobj) == FALSE))
    {
        return FALSE;
    }
    *local_position = *world_position;
    if ((yakumono_dobj->anim_joint.event32 != NULL) ||
        (yakumono_dobj->user_data.s != nMPYakumonoStatusNone))
    {
        local_position->x -= yakumono_dobj->translate.vec.f.x;
        local_position->y -= yakumono_dobj->translate.vec.f.y;
    }
    return TRUE;
}

u16 mpCollisionGetVertexFlagsLineID(s32 line_id)
{
    u32 flags = 0u;

    if (ndsMPFindLineEndpoints(line_id, NULL, NULL, &flags, NULL) == FALSE)
    {
        return 0u;
    }
    return (u16)flags;
}

sb32 mpCollisionCheckExistPlatformLineID(s32 line_id)
{
    u32 yakumono_id = 0u;
    DObj *yakumono_dobj;

    if ((line_id == -1) || (line_id == -2) ||
        (ndsMPFindLineYakumonoID(line_id, &yakumono_id) == FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
    {
        return FALSE;
    }

    yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
    if (yakumono_dobj == NULL)
    {
        return FALSE;
    }
    return ((yakumono_dobj->user_data.s != nMPYakumonoStatusNone) ||
            (yakumono_dobj->anim_joint.event32 != NULL)) ? TRUE : FALSE;
}

s32 mpCollisionGetVertexCountLineID(s32 line_id)
{
    s32 vertex_count = 0;

    if (ndsMPFindLineEndpoints(line_id, NULL, NULL, NULL,
            &vertex_count) == FALSE)
    {
        return 0;
    }
    return vertex_count;
}

static sb32 ndsMPGetVertexPositionForLineID(s32 line_id, s32 vertex_id,
                                            Vec3f *object_pos)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPVertexLinks *links;
    MPVertexArray *ids;
    MPVertexPosContainer *verts;
    u32 vertex_first;
    u32 vertex_count;
    u32 v_id;

    if (object_pos != NULL)
    {
        object_pos->x = 0.0F;
        object_pos->y = 0.0F;
        object_pos->z = 0.0F;
    }
    if ((line_id < 0) || (vertex_id < 0) ||
        (ndsMPFindLineEndpoints(line_id, NULL, NULL, NULL,
            NULL) == FALSE) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    links = geometry->vertex_links;
    ids = geometry->vertex_id;
    verts = geometry->vertex_data;
    vertex_count = ndsMPVertexLinkCount(links, (u32)line_id);
    if (((u32)vertex_id >= vertex_count) || (vertex_count > 128u))
    {
        return FALSE;
    }
    vertex_first = ndsMPVertexLinkFirst(links, (u32)line_id);
    v_id = ndsMPVertexID(ids, vertex_first + (u32)vertex_id);
    if (object_pos != NULL)
    {
        object_pos->x = (f32)ndsMPVertexX(verts, v_id);
        object_pos->y = (f32)ndsMPVertexY(verts, v_id);
        object_pos->z = 0.0F;
    }
    return TRUE;
}

s32 mpCollisionGetLineTypeID(s32 line_id)
{
    return ndsMPGetLineKindForLineID(line_id);
}

void mpCollisionGetVertexPositionID(s32 line_id, s32 vertex_id,
                                    Vec3f *object_pos)
{
    if ((ndsMPGetVertexPositionForLineID(line_id, vertex_id, object_pos) ==
         FALSE) ||
        (ndsMPLinePositionLocalToWorld(line_id, object_pos) == FALSE))
    {
        if (object_pos != NULL)
        {
            object_pos->x = 0.0F;
            object_pos->y = 0.0F;
            object_pos->z = 0.0F;
        }
    }
}

static u32 ndsMPLinePointSegmentSearch(MPVertexArray *ids,
                                       MPVertexPosContainer *verts,
                                       s32 line_id, u32 vertex_first,
                                       u32 vertex_count, f32 object_x);
#define NDS_MP_SEGMENT_WINDOW_MIN 12u

/* The point queries' first segment worth testing (see
 * ndsMPLinePointSegmentSearch); the cheap tests stay at the call site, so a
 * short or unordered line pays no call. */
static inline u32 ndsMPLinePointSegmentStart(MPVertexArray *ids,
                                             MPVertexPosContainer *verts,
                                             s32 line_id, u32 vertex_first,
                                             u32 vertex_count, f32 object_x)
{
    if ((vertex_count < NDS_MP_SEGMENT_WINDOW_MIN) || (line_id < 0) ||
        ((u32)line_id >= NDS_MP_LINE_EXTENT_MAX) ||
        (sNdsMPLineXOrder[line_id] == NDS_MP_LINE_XORDER_NONE))
    {
        return 0u;
    }
    return ndsMPLinePointSegmentSearch(ids, verts, line_id, vertex_first,
                                       vertex_count, object_x);
}

/* ITCM (2026-09-27): second in the four-CPU census admission table after
 * the wall sweep (~7.1K ticks/frame non-memory stall at 1,484 B). */
sb32 NDS_R2_ITCM_PACK2_CODE mpCollisionGetFCCommonFloor(s32 line_id, Vec3f *object_pos,
                                 f32 *floor_dist, u32 *floor_flags,
                                 Vec3f *angle)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPVertexLinks *links;
    MPVertexArray *ids;
    MPVertexPosContainer *verts;
    u32 vertex_first;
    u32 vertex_count;
    u32 segment_count;
    u32 yakumono_id = 0u;
    DObj *yakumono_dobj = NULL;
    f32 object_x;
    f32 object_y;
    u32 j;

    if (floor_dist != NULL)
    {
        *floor_dist = 0.0F;
    }
    if (floor_flags != NULL)
    {
        *floor_flags = 0u;
    }
    if (angle != NULL)
    {
        angle->x = 0.0F;
        angle->y = 1.0F;
        angle->z = 0.0F;
    }
    if ((object_pos == NULL) || (line_id < 0) ||
        (ndsMPLineIDIsFloor(line_id) == FALSE) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    links = geometry->vertex_links;
    ids = geometry->vertex_id;
    verts = geometry->vertex_data;
    object_x = object_pos->x;
    object_y = object_pos->y;
    if ((ndsMPFindLineYakumonoID(line_id, &yakumono_id) != FALSE) &&
        (gMPCollisionYakumonoDObjs != NULL) &&
        (yakumono_id < NDS_MP_YAKUMONO_DOBJ_SLOTS))
    {
        yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
    }
    if ((yakumono_dobj == NULL) ||
        (yakumono_dobj->user_data.s >= nMPYakumonoStatusOff))
    {
        return FALSE;
    }
    if ((yakumono_dobj->anim_joint.event32 != NULL) ||
        (yakumono_dobj->user_data.s != nMPYakumonoStatusNone))
    {
        object_x -= yakumono_dobj->translate.vec.f.x;
        object_y -= yakumono_dobj->translate.vec.f.y;
    }
    vertex_first = ndsMPVertexLinkFirst(links, (u32)line_id);
    vertex_count = ndsMPVertexLinkCount(links, (u32)line_id);
    if ((vertex_count < 2u) || (vertex_count > 128u))
    {
        return FALSE;
    }
    segment_count = vertex_count - 1u;
    /* Cycle 109: same memo as the floor sweep. The gate below converted x1 and
     * x2 to f32 twice each per segment -- part of this function's 56,567
     * `__aeabi_i2f` calls (950,326 cycles) -- on static stage vertices. The s32
     * values are still read for `x1 == x2`, `ndsMPLineDistanceFC` and
     * `ndsMPGetFCAngle`, which want integers anyway. */
    ndsMPVertexF32Bind(geometry);
    if (ndsMPLineExtentRejects(ids, verts, (u32)line_id, vertex_first,
                               vertex_count, object_x) != 0)
    {
        return FALSE;
    }
    for (j = ndsMPLinePointSegmentStart(ids, verts, line_id, vertex_first,
                                        vertex_count, object_x);
         j < segment_count; j++)
    {
        u32 v1_index = vertex_first + j;
        u32 v2_index = v1_index + 1u;
        u32 v1_id = ndsMPVertexID(ids, v1_index);
        u32 v2_id = ndsMPVertexID(ids, v2_index);
        f32 fx1;
        f32 fx2;
        f32 fy1;
        f32 fy2;
        f32 floor_y;

        ndsMPVertexF32Get(verts, v1_id, &fx1, &fy1);
        ndsMPVertexF32Get(verts, v2_id, &fx2, &fy2);
        if (!((NDS_FCMP_LE(fx1, object_x) && NDS_FCMP_GE(fx2, object_x)) ||
              (NDS_FCMP_LE(fx2, object_x) && NDS_FCMP_GE(fx1, object_x))))
        {
            continue;
        }
        /* 2026-09-27: the accepted segment works on the cached floats (exact
         * conversions of the s16 vertices, so x1 == x2 iff fx1 == fx2 and
         * every operand below is the integer form's) instead of four O2R
         * reads and six __aeabi_i2f calls. Exact conversions are equal
         * exactly when their bits are. */
        if (ndsFcmpBits(fx1) == ndsFcmpBits(fx2))
        {
            NDS_DIAG(gNdsStageCollisionLoopDivisionGuardCount++);
            continue;
        }
        /* A flat segment's height is fy1 exactly (ndsMPFlatSegmentExact):
         * the source's one subtract is the whole answer. A slope takes the
         * Q12 height. */
        floor_y = (ndsFcmpBits(fy1) == ndsFcmpBits(fy2)) ?
            (fy1 - object_y) :
            ndsMPFloorDistQ12(object_x, object_y,
                              ndsMPVertexX(verts, v1_id),
                              ndsMPVertexY(verts, v1_id),
                              ndsMPVertexX(verts, v2_id),
                              ndsMPVertexY(verts, v2_id));
        if (floor_dist != NULL)
        {
            *floor_dist = floor_y;
        }
        if (floor_flags != NULL)
        {
            *floor_flags = ndsMPVertexFlags(verts, v1_id);
        }
        ndsMPGetFCAnglef(angle, fx1, fy1, fx2, fy2, +1);
        return TRUE;
    }
    return FALSE;
}

/* "Is there floor at or just above this point?" -- asked by the CPU AI for
 * every target it weighs (ftcomputer.c), so the same TopN position is asked by
 * up to three CPUs in a tick, and each ask ran mpCollisionGetFCCommonFloor
 * over every floor line (~32K ticks a frame on Dream Land, P2-2p8 2026-09-28;
 * same-ROM A/B median -10.9K Dream Land, -34K Mushroom Kingdom, -21K Hyrule,
 * replay identical). Out of ITCM: the memo's hits no longer need it there.
 *
 * The answer is a pure function of the point's x/y and, per floor line, its
 * yakumono DObj's status, animation presence and translate x/y (the only live
 * inputs mpCollisionGetFCCommonFloor reads; the vertices are static). So it is
 * memoised on the point's bits while a snapshot of every yakumono's state --
 * and the geometry it belongs to -- is bit-identical; any change empties the
 * memo. A served answer is the one the lines would give. */
#define NDS_MP_AI_FLOOR_MEMO_SLOTS 8u
typedef struct NDSMPAiFloorYakumono
{
    const DObj *dobj;
    s32 status;
    u32 animated;
    u32 x_bits;
    u32 y_bits;
} NDSMPAiFloorYakumono;
static MPGeometryData *sNdsMPAiFloorGeometry;
static u32 sNdsMPAiFloorYakumonoCount;
#define NDS_MP_AI_FLOOR_YAKUMONO_MAX 32u
static NDSMPAiFloorYakumono sNdsMPAiFloorYakumono[NDS_MP_AI_FLOOR_YAKUMONO_MAX];
static u32 sNdsMPAiFloorKey[NDS_MP_AI_FLOOR_MEMO_SLOTS][2];
static u8 sNdsMPAiFloorAnswer[NDS_MP_AI_FLOOR_MEMO_SLOTS];
static u32 sNdsMPAiFloorCount;
static u32 sNdsMPAiFloorNext;
/* The yakumono ids of the groups that own floor lines, per geometry
 * (2026-10-06): only their DObjs reach the answer -- the floor loop asks
 * mpCollisionGetFCCommonFloor about floor lines, and a line's yakumono is its
 * group's -- so the snapshot reads only them, and no longer re-reads every
 * group's line-info halfwords on every ask (Peach's Castle: ~14K cycles a
 * frame of snapshot). */
static MPGeometryData *sNdsMPAiFloorIdsGeometry;
static u32 sNdsMPAiFloorIdsCount;
static u8 sNdsMPAiFloorIds[NDS_MP_AI_FLOOR_YAKUMONO_MAX];

static inline u32 ndsMPAiFloorBits(f32 value)
{
    u32 bits;

    __builtin_memcpy(&bits, &value, sizeof(bits));
    return bits;
}

/* Fill sNdsMPAiFloorIds for `geometry`; FALSE when it has more groups than
 * the snapshot holds (no memo for this stage). */
static sb32 __attribute__((noinline, cold))
ndsMPAiFloorIdsFor(MPGeometryData *geometry)
{
    const u32 count = ndsMPGeometryYakumonoCount(geometry);
    u32 n = 0u;
    u32 i;

    sNdsMPAiFloorIdsGeometry = NULL;
    if (count > NDS_MP_AI_FLOOR_YAKUMONO_MAX)
    {
        return FALSE;
    }
    for (i = 0u; i < count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(geometry->line_info, i);

        if (ndsMPLineInfoLineCount(info, nMPLineKindFloor) == 0u)
        {
            continue;
        }
        sNdsMPAiFloorIds[n++] = (u8)((ndsMPLineInfoYakumonoID(info) < 0xffu) ?
            ndsMPLineInfoYakumonoID(info) : 0xffu);
    }
    sNdsMPAiFloorIdsCount = n;
    sNdsMPAiFloorIdsGeometry = geometry;
    return TRUE;
}

/* Refresh the yakumono snapshot; returns FALSE when it could not be taken
 * (the memo is then bypassed). Empties the memo on any change. */
static sb32 __attribute__((noinline)) ndsMPAiFloorSnapshot(void)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    u32 count;
    u32 i;
    sb32 same;

    if ((geometry == NULL) || (geometry->line_info == NULL) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        ((sNdsMPAiFloorIdsGeometry != geometry) &&
         (ndsMPAiFloorIdsFor(geometry) == FALSE)))
    {
        sNdsMPAiFloorCount = 0u;
        sNdsMPAiFloorGeometry = NULL;
        return FALSE;
    }
    count = sNdsMPAiFloorIdsCount;
    same = ((sNdsMPAiFloorGeometry == geometry) &&
            (sNdsMPAiFloorYakumonoCount == count)) ? TRUE : FALSE;
    for (i = 0u; i < count; i++)
    {
        NDSMPAiFloorYakumono now;
        NDSMPAiFloorYakumono *was = &sNdsMPAiFloorYakumono[i];
        u32 yakumono_id = sNdsMPAiFloorIds[i];

        now.dobj = (yakumono_id < NDS_MP_YAKUMONO_DOBJ_SLOTS) ?
            gMPCollisionYakumonoDObjs->dobjs[yakumono_id] : NULL;
        if (now.dobj != NULL)
        {
            now.status = now.dobj->user_data.s;
            now.animated = (now.dobj->anim_joint.event32 != NULL) ? 1u : 0u;
            now.x_bits = ndsMPAiFloorBits(now.dobj->translate.vec.f.x);
            now.y_bits = ndsMPAiFloorBits(now.dobj->translate.vec.f.y);
        }
        else
        {
            now.status = 0;
            now.animated = 0u;
            now.x_bits = 0u;
            now.y_bits = 0u;
        }
        if ((same == FALSE) || (was->dobj != now.dobj) ||
            (was->status != now.status) || (was->animated != now.animated) ||
            (was->x_bits != now.x_bits) || (was->y_bits != now.y_bits))
        {
            same = FALSE;
            *was = now;
        }
    }
    if (same == FALSE)
    {
        sNdsMPAiFloorGeometry = geometry;
        sNdsMPAiFloorYakumonoCount = count;
        sNdsMPAiFloorCount = 0u;
        sNdsMPAiFloorNext = 0u;
    }
    return TRUE;
}

sb32 func_ovl2_800F8FFC(Vec3f *position)
{
    MPLineGroup *floors = &gMPCollisionLineGroups[nMPLineKindFloor];
    u32 x_bits;
    u32 y_bits;
    sb32 memo;
    sb32 answer = FALSE;
    u32 i;

    if (position == NULL)
    {
        return FALSE;
    }
    ndsMPCollisionEnsureLineGroups();
    x_bits = ndsMPAiFloorBits(position->x);
    y_bits = ndsMPAiFloorBits(position->y);
    memo = ndsMPAiFloorSnapshot();
    if (memo != FALSE)
    {
        for (i = 0u; i < sNdsMPAiFloorCount; i++)
        {
            if ((sNdsMPAiFloorKey[i][0] == x_bits) &&
                (sNdsMPAiFloorKey[i][1] == y_bits))
            {
                return (sb32)sNdsMPAiFloorAnswer[i];
            }
        }
    }
    for (i = 0u; i < floors->line_count; i++)
    {
        f32 floor_dist;

        if ((mpCollisionGetFCCommonFloor(floors->line_id[i], position,
                 &floor_dist, NULL, NULL) != FALSE) &&
            NDS_FCMP_LT_C(floor_dist, 0.001F))
        {
            answer = TRUE;
            break;
        }
    }
    if (memo != FALSE)
    {
        u32 slot = sNdsMPAiFloorNext;

        sNdsMPAiFloorNext = (slot + 1u) & (NDS_MP_AI_FLOOR_MEMO_SLOTS - 1u);
        if (sNdsMPAiFloorCount < NDS_MP_AI_FLOOR_MEMO_SLOTS)
        {
            sNdsMPAiFloorCount++;
        }
        sNdsMPAiFloorKey[slot][0] = x_bits;
        sNdsMPAiFloorKey[slot][1] = y_bits;
        sNdsMPAiFloorAnswer[slot] = (u8)answer;
    }
    return answer;
}

sb32 mpCollisionGetFCCommonCeil(s32 line_id, Vec3f *object_pos,
                                f32 *ceil_dist, u32 *ceil_flags,
                                Vec3f *angle)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPVertexLinks *links;
    MPVertexArray *ids;
    MPVertexPosContainer *verts;
    u32 vertex_first;
    u32 vertex_count;
    u32 segment_count;
    Vec3f object_local;
    u32 j;

    if (ceil_dist != NULL)
    {
        *ceil_dist = 0.0F;
    }
    if (ceil_flags != NULL)
    {
        *ceil_flags = 0u;
    }
    if (angle != NULL)
    {
        angle->x = 0.0F;
        angle->y = -1.0F;
        angle->z = 0.0F;
    }
    if ((object_pos == NULL) || (line_id < 0) ||
        (ndsMPLineIDIsCeil(line_id) == FALSE) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    if (ndsMPLinePositionWorldToLocal(line_id, object_pos, &object_local) ==
        FALSE)
    {
        return FALSE;
    }
    links = geometry->vertex_links;
    ids = geometry->vertex_id;
    verts = geometry->vertex_data;
    vertex_first = ndsMPVertexLinkFirst(links, (u32)line_id);
    vertex_count = ndsMPVertexLinkCount(links, (u32)line_id);
    if ((vertex_count < 2u) || (vertex_count > 128u))
    {
        return FALSE;
    }
    segment_count = vertex_count - 1u;
    /* The ceiling path never bound the vertex memo, so it was still paying four
     * live `__aeabi_i2f` conversions a segment for the gate on static stage
     * data -- the exact cost cycle 109 removed from the floor path. The memo
     * stores the same `(f32)` of the same s16, so the gate is bit-identical. */
    ndsMPVertexF32Bind(geometry);
    if (ndsMPLineExtentRejects(ids, verts, (u32)line_id, vertex_first,
                               vertex_count, object_local.x) != 0)
    {
        return FALSE;
    }
    for (j = ndsMPLinePointSegmentStart(ids, verts, line_id, vertex_first,
                                        vertex_count, object_local.x);
         j < segment_count; j++)
    {
        u32 v1_index = vertex_first + j;
        u32 v2_index = v1_index + 1u;
        u32 v1_id = ndsMPVertexID(ids, v1_index);
        u32 v2_id = ndsMPVertexID(ids, v2_index);
        s32 x1;
        s32 y1;
        s32 x2;
        s32 y2;
        f32 fx1;
        f32 fx2;
        f32 unused_y;
        f32 ceil_y;

        ndsMPVertexF32Get(verts, v1_id, &fx1, &unused_y);
        ndsMPVertexF32Get(verts, v2_id, &fx2, &unused_y);
        if (!((NDS_FCMP_LE(fx1, object_local.x) &&
               NDS_FCMP_GE(fx2, object_local.x)) ||
              (NDS_FCMP_LE(fx2, object_local.x) &&
               NDS_FCMP_GE(fx1, object_local.x))))
        {
            continue;
        }
        x1 = ndsMPVertexX(verts, v1_id);
        y1 = ndsMPVertexY(verts, v1_id);
        x2 = ndsMPVertexX(verts, v2_id);
        y2 = ndsMPVertexY(verts, v2_id);
        if (x1 == x2)
        {
            NDS_DIAG(gNdsStageCollisionLoopDivisionGuardCount++);
            continue;
        }
        ceil_y = ndsMPLineDistanceFC(object_local.x, x1, y1, x2, y2);
        if (ceil_dist != NULL)
        {
            *ceil_dist = ceil_y - object_local.y;
        }
        if (ceil_flags != NULL)
        {
            *ceil_flags = ndsMPVertexFlags(verts, v1_id);
        }
        ndsMPGetFCAngle(angle, x1, y1, x2, y2, -1);
        return TRUE;
    }
    return FALSE;
}

static s32 ndsMPTopologyLineCount(void)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    u32 yakumono_count;
    u32 line_total = 0u;
    u32 i;
    u32 kind;

    if (ndsStageCollisionLoopGeometryReady() == FALSE)
    {
        return 0;
    }
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        return 0;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(geometry->line_info, i);

        for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
        {
            u32 first = ndsMPLineInfoGroupID(info, kind);
            u32 count = ndsMPLineInfoLineCount(info, kind);

            if ((count > 4096u) || (first > 4096u) ||
                ((first + count) > 4096u) ||
                ((line_total + count) > 4096u))
            {
                return 0;
            }
            line_total += count;
        }
    }
    return (s32)line_total;
}

static int ndsMPReadTopologyLine(void *context, int line_id,
                                 NDSMPTopologyRawLine *line)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    u32 first;
    u32 count;
    u32 first_vertex_id;
    u32 last_vertex_id;
    s32 kind;

    (void)context;
    if ((line == NULL) || (line_id < 0) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return 0;
    }
    kind = ndsMPGetLineKindForLineID(line_id);
    if ((kind < 0) || (kind >= nMPLineKindEnumCount))
    {
        return 0;
    }
    first = ndsMPVertexLinkFirst(geometry->vertex_links, (u32)line_id);
    count = ndsMPVertexLinkCount(geometry->vertex_links, (u32)line_id);
    if ((count < 2u) || (count > 128u))
    {
        return 0;
    }
    first_vertex_id = ndsMPVertexID(geometry->vertex_id, first);
    last_vertex_id = ndsMPVertexID(geometry->vertex_id,
                                  first + count - 1u);
    line->kind = kind;
    line->first_vertex_id = (int)first_vertex_id;
    line->last_vertex_id = (int)last_vertex_id;
    line->first_x = ndsMPVertexX(geometry->vertex_data, first_vertex_id);
    line->first_y = ndsMPVertexY(geometry->vertex_data, first_vertex_id);
    line->last_x = ndsMPVertexX(geometry->vertex_data, last_vertex_id);
    line->last_y = ndsMPVertexY(geometry->vertex_data, last_vertex_id);
    return 1;
}

static sb32 ndsMPTopologySourceLineIsActive(s32 line_id)
{
    u32 yakumono_id;
    DObj *yakumono_dobj;

    if ((ndsMPFindLineYakumonoID(line_id, &yakumono_id) == FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
    {
        return FALSE;
    }
    yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
    return ((yakumono_dobj != NULL) &&
            (yakumono_dobj->user_data.s < nMPYakumonoStatusOff)) ?
        TRUE : FALSE;
}

static sb32 ndsMPBuildTopologyCache(void)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPGeometryData *previous_geometry = sNdsMPTopologyGeometry;
    MPVertexInfoContainer *vertex_info;
    s32 line_count = ndsMPTopologyLineCount();
    u32 kind_group_total;
    u32 kind_group_offset;
    u32 hash = 2166136261u;
    u32 shared_directed = 0u;
    u32 orphan_endpoints = 0u;
    u32 reversed_lines = 0u;
    u32 ambiguous_endpoints = 0u;
    s32 line_id;

    if ((geometry == sNdsMPTopologyGeometry) &&
        (gMPCollisionVertexInfo != NULL) &&
        (gMPCollisionLinesNum == line_count))
    {
        return TRUE;
    }
    if (sNdsMPTopologyFailedGeometry == geometry)
    {
        return FALSE;
    }
    NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_build_attempts++);
    if ((previous_geometry != NULL) && (previous_geometry != geometry))
    {
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_rebuilds++);
    }
    gMPCollisionVertexInfo = NULL;
    gMPCollisionLinesNum = 0;
    sNdsMPTopologyGeometry = NULL;
    sNdsMPKindGroupGeometry = NULL;
    sNdsMPWallMissReadyGeometry = NULL;
    ndsMPCollisionClearTopologySnapshot();
    if ((geometry == NULL) || (line_count <= 0) ||
        (line_count > 4096))
    {
        sNdsMPTopologyFailedGeometry = geometry;
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_invalid++);
        return FALSE;
    }
    /* Validate the complete decoded graph before consuming task-arena space.
     * The runtime is single-threaded, so the identical post-allocation reads
     * cannot change underneath this pass. */
    for (line_id = 0; line_id < line_count; line_id++)
    {
        NDSMPTopologyRawLine raw;
        u32 yakumono_id;
        int edge_prev;
        int edge_next;
        int prev_matches;
        int next_matches;

        if ((ndsMPReadTopologyLine(NULL, line_id, &raw) == 0) ||
            (ndsMPFindLineYakumonoID(line_id, &yakumono_id) == FALSE) ||
            (yakumono_id > 255u) ||
            (ndsMPTopologyResolveEdgesKernel(NULL, line_count, line_id,
                ndsMPReadTopologyLine, &edge_prev, &edge_next,
                &prev_matches, &next_matches) == 0))
        {
            sNdsMPTopologyFailedGeometry = geometry;
            NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_invalid++);
            return FALSE;
        }
    }
    {
        u32 yakumono_count = ndsMPGeometryYakumonoCount(geometry);
        u32 kind;
        u32 y;

        if (yakumono_count > 64u)
        {
            yakumono_count = 64u;
        }
        kind_group_total = 0u;
        for (y = 0u; y < yakumono_count; y++)
        {
            NDSMPO2RHalfwordView info = ndsMPLineInfoAt(geometry->line_info, y);

            for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
            {
                if ((s32)ndsMPLineInfoLineCount(info, kind) > 0)
                {
                    kind_group_total++;
                }
            }
        }
    }
    kind_group_offset = (((u32)line_count * sizeof(MPVertexInfo)) + 3u) & ~3u;
    /* Spans: at most one per line, or one for a group past the table. */
    vertex_info = syTaskmanMalloc(
        kind_group_offset + (kind_group_total * sizeof(NDSMPKindGroup)) +
            (((u32)line_count + kind_group_total) * sizeof(NDSMPGroupSpan)),
        8u);
    if (vertex_info == NULL)
    {
        sNdsMPTopologyFailedGeometry = geometry;
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_invalid++);
        return FALSE;
    }
    for (line_id = 0; line_id < line_count; line_id++)
    {
        NDSMPTopologyRawLine raw;
        MPVertexInfo *info = &vertex_info->vertex_info[line_id];
        u32 yakumono_id;
        u32 first;
        u32 count;
        u32 vertex;
        s32 coll_min;
        s32 coll_max;

        (void)ndsMPReadTopologyLine(NULL, line_id, &raw);
        (void)ndsMPFindLineYakumonoID(line_id, &yakumono_id);
        first = ndsMPVertexLinkFirst(geometry->vertex_links, (u32)line_id);
        count = ndsMPVertexLinkCount(geometry->vertex_links, (u32)line_id);
        coll_min = coll_max = (raw.kind < nMPLineKindRWall) ?
            raw.first_y : raw.first_x;
        for (vertex = 1u; vertex < count; vertex++)
        {
            u32 vertex_id = ndsMPVertexID(geometry->vertex_id,
                                         first + vertex);
            s32 coll_pos = (raw.kind < nMPLineKindRWall) ?
                ndsMPVertexY(geometry->vertex_data, vertex_id) :
                ndsMPVertexX(geometry->vertex_data, vertex_id);

            if (coll_pos < coll_min)
            {
                coll_min = coll_pos;
            }
            if (coll_pos > coll_max)
            {
                coll_max = coll_pos;
            }
        }
        info->yakumono_id = (u8)yakumono_id;
        info->line_type = (u8)raw.kind;
        info->coll_pos_next = (s16)coll_max;
        info->coll_pos_prev = (s16)coll_min;
        info->edge_next_line_id = -1;
        info->edge_prev_line_id = -1;
    }
    for (line_id = 0; line_id < line_count; line_id++)
    {
        NDSMPTopologyRawLine raw;
        MPVertexInfo *info = &vertex_info->vertex_info[line_id];
        int edge_prev;
        int edge_next;
        int prev_matches;
        int next_matches;

        (void)ndsMPReadTopologyLine(NULL, line_id, &raw);
        (void)ndsMPTopologyResolveEdgesKernel(NULL, line_count, line_id,
            ndsMPReadTopologyLine, &edge_prev, &edge_next,
            &prev_matches, &next_matches);
        info->edge_prev_line_id = (s16)edge_prev;
        info->edge_next_line_id = (s16)edge_next;
        shared_directed += (u32)prev_matches + (u32)next_matches;
        orphan_endpoints += (prev_matches == 0) ? 1u : 0u;
        orphan_endpoints += (next_matches == 0) ? 1u : 0u;
        ambiguous_endpoints += (prev_matches > 1) ? 1u : 0u;
        ambiguous_endpoints += (next_matches > 1) ? 1u : 0u;
        if (((raw.kind < nMPLineKindRWall) &&
             (raw.last_x < raw.first_x)) ||
            ((raw.kind >= nMPLineKindRWall) &&
             (raw.last_y < raw.first_y)))
        {
            reversed_lines++;
        }
        hash = (hash ^ (u32)line_id) * 16777619u;
        hash = (hash ^ (u32)(edge_prev + 2)) * 16777619u;
        hash = (hash ^ (u32)(edge_next + 2)) * 16777619u;
    }
    {
        NDSMPKindGroup *groups =
            (NDSMPKindGroup *)((u8 *)vertex_info + kind_group_offset);
        NDSMPGroupSpan *spans = (NDSMPGroupSpan *)(groups + kind_group_total);
        u32 span_total = 0u;
        u32 yakumono_count = ndsMPGeometryYakumonoCount(geometry);
        u32 kind;
        u32 y;

        sNdsMPGroupSpans = spans;
        if (yakumono_count > 64u)
        {
            yakumono_count = 64u;
        }
        for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
        {
            u32 n = 0u;

            sNdsMPKindGroups[kind] = groups;
            for (y = 0u; y < yakumono_count; y++)
            {
                NDSMPO2RHalfwordView info =
                    ndsMPLineInfoAt(geometry->line_info, y);
                s32 count = (s32)ndsMPLineInfoLineCount(info, kind);

                if (count <= 0)
                {
                    continue;
                }
                groups->first = (u16)ndsMPLineInfoGroupID(info, kind);
                groups->count = (u16)((count > 4096) ? 4096 : count);
                groups->yakumono_id = ndsMPLineInfoYakumonoID(info);
                groups->reject_misses = 0u;
                groups->reject_skip = 0u;
                groups->span_first = 0u;
                groups->span_count = 0u;
                groups->pad = 0u;
                {
                    s32 lo = 0x7fff;
                    s32 hi = -0x8000;
                    s32 l;
                    sb32 past_table = FALSE;

                    for (l = (s32)groups->first;
                         l < (s32)groups->first + (s32)groups->count; l++)
                    {
                        if ((l < 0) || (l >= line_count))
                        {
                            /* A group past the table: never rejected. */
                            lo = -0x8000;
                            hi = 0x7fff;
                            past_table = TRUE;
                            break;
                        }
                        if (vertex_info->vertex_info[l].coll_pos_prev < lo)
                        {
                            lo = vertex_info->vertex_info[l].coll_pos_prev;
                        }
                        if (vertex_info->vertex_info[l].coll_pos_next > hi)
                        {
                            hi = vertex_info->vertex_info[l].coll_pos_next;
                        }
                    }
                    groups->ext_lo = (s16)lo;
                    groups->ext_hi = (s16)hi;
                    /* The spans (P2-6, 2026-10-02): each line's own
                     * [coll_pos_prev, coll_pos_next], insertion-sorted by low
                     * end, then merged wherever the gap between two is at
                     * most twice the reject's slack -- every query range is
                     * at least that wide, so none fits in such a gap and the
                     * merge changes no answer. A group past the table, or
                     * too long to sort here, keeps span_count 0: the reject
                     * then tests its [ext_lo, ext_hi] alone, as before. */
                    if ((past_table == FALSE) &&
                        (groups->count <= NDS_MP_GROUP_SPAN_MAX_LINES) &&
                        ((span_total + groups->count) <=
                         ((u32)line_count + kind_group_total)))
                    {
                        NDSMPGroupSpan *gs = spans + span_total;
                        u32 n_spans = 0u;
                        u32 r;
                        u32 w;

                        for (l = (s32)groups->first;
                             l < (s32)groups->first + (s32)groups->count; l++)
                        {
                            s16 a = vertex_info->vertex_info[l].coll_pos_prev;
                            s16 b = vertex_info->vertex_info[l].coll_pos_next;
                            u32 k = n_spans;

                            if (b < a)
                            {
                                s16 t = a;

                                a = b;
                                b = t;
                            }
                            while ((k > 0u) && (gs[k - 1u].lo > a))
                            {
                                gs[k] = gs[k - 1u];
                                k--;
                            }
                            gs[k].lo = a;
                            gs[k].hi = b;
                            n_spans++;
                        }
                        for (r = 1u, w = 0u; r < n_spans; r++)
                        {
                            if (((s32)gs[r].lo - (s32)gs[w].hi) <=
                                (2 * NDS_MP_WALL_SWEEP_SLACK))
                            {
                                if (gs[r].hi > gs[w].hi)
                                {
                                    gs[w].hi = gs[r].hi;
                                }
                            }
                            else
                            {
                                gs[++w] = gs[r];
                            }
                        }
                        groups->span_first = (u16)span_total;
                        groups->span_count = (u16)(w + 1u);
                        span_total += w + 1u;
                    }
                }
                groups++;
                n++;
            }
            sNdsMPKindGroupCounts[kind] = (u8)n;
        }
        sNdsMPKindGroupGeometry = geometry;
    }
    gMPCollisionVertexInfo = vertex_info;
    gMPCollisionLinesNum = line_count;
    sNdsMPTopologyGeometry = geometry;
    sNdsMPTopologyFailedGeometry = NULL;
    NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_build_successes++);
    gNdsCollisionRuntimeDiagnostics.topology_lines = (u32)line_count;
    gNdsCollisionRuntimeDiagnostics.topology_shared_directed =
        shared_directed;
    gNdsCollisionRuntimeDiagnostics.topology_orphan_endpoints =
        orphan_endpoints;
    gNdsCollisionRuntimeDiagnostics.topology_reversed_lines = reversed_lines;
    gNdsCollisionRuntimeDiagnostics.topology_ambiguous_endpoints =
        ambiguous_endpoints;
    gNdsCollisionRuntimeDiagnostics.topology_hash = hash;
    return TRUE;
}

static s32 ndsMPGetTopologyEdgeLineID(s32 line_id, sb32 get_next)
{
    MPVertexInfo *info;
    s32 result;

    NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_getter_calls++);
    /* P2-2p5. The ensure ran on every getter call and paid its two
     * line-count memsets plus ndsStageCollisionLoopGeometryReady before its
     * own early-out (census 2026-08-23: memset 8.5M cycles, the ready check
     * 2.9M at 68 bytes). A topology that is valid for the current geometry
     * is exactly what the ensure would leave in place: the gMPCollisionGeometry
     * assignment seam (compat_shims) invalidates both pointers, so a stale
     * equal pointer cannot reach here. Only build when it is not valid. */
    if ((sNdsMPTopologyGeometry != gMPCollisionGeometry) ||
        (gMPCollisionVertexInfo == NULL))
    {
        ndsMPCollisionEnsureLineGroups();
    }
    if ((sNdsMPTopologyGeometry != gMPCollisionGeometry) ||
        (sNdsMPTopologyFailedGeometry == gMPCollisionGeometry) ||
        (gMPCollisionVertexInfo == NULL) || (line_id < 0) ||
        (line_id >= gMPCollisionLinesNum))
    {
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_getter_invalid++);
        return -1;
    }
    info = &gMPCollisionVertexInfo->vertex_info[line_id];
    gNdsCollisionRuntimeDiagnostics.topology_last_line = line_id;
    gNdsCollisionRuntimeDiagnostics.topology_last_prev =
        info->edge_prev_line_id;
    gNdsCollisionRuntimeDiagnostics.topology_last_next =
        info->edge_next_line_id;
    if (ndsMPTopologySourceLineIsActive(line_id) == FALSE)
    {
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_inactive++);
        return -1;
    }
    result = (get_next != FALSE) ? info->edge_next_line_id :
        info->edge_prev_line_id;
    if (result >= 0)
    {
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_getter_hits++);
    }
    else
    {
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.topology_getter_no_edge++);
    }
    return result;
}

s32 mpCollisionGetEdgeUnderLLineID(s32 line_id)
{
    s32 edge_under_line_id;

    if (sNdsStageMPLiveStaleFloorLoopProbeActive != FALSE)
    {
        return -1;
    }
    edge_under_line_id = ndsMPGetTopologyEdgeLineID(line_id, FALSE);
    return edge_under_line_id;
}

s32 mpCollisionGetEdgeUnderRLineID(s32 line_id)
{
    s32 edge_under_line_id;

    if (sNdsStageMPLiveStaleFloorLoopProbeActive != FALSE)
    {
        return -1;
    }
    edge_under_line_id = ndsMPGetTopologyEdgeLineID(line_id, TRUE);
    return edge_under_line_id;
}

void mpCollisionGetFloorEdgeL(s32 line_id, Vec3f *object_pos)
{
    Vec3f left;

    if (object_pos == NULL)
    {
        return;
    }
    if ((ndsMPFindLineEndpoints(line_id, &left, NULL, NULL, NULL) == FALSE) ||
        (ndsMPLinePositionLocalToWorld(line_id, &left) == FALSE))
    {
        object_pos->x = 0.0F;
        object_pos->y = 0.0F;
        object_pos->z = 0.0F;
        return;
    }
    *object_pos = left;
}

void mpCollisionGetFloorEdgeR(s32 line_id, Vec3f *object_pos)
{
    Vec3f right;

    if (object_pos == NULL)
    {
        return;
    }
    if ((ndsMPFindLineEndpoints(line_id, NULL, &right, NULL, NULL) == FALSE) ||
        (ndsMPLinePositionLocalToWorld(line_id, &right) == FALSE))
    {
        object_pos->x = 0.0F;
        object_pos->y = 0.0F;
        object_pos->z = 0.0F;
        return;
    }
    *object_pos = right;
}

void mpCollisionGetCeilEdgeL(s32 line_id, Vec3f *object_pos)
{
    Vec3f left;

    if (object_pos == NULL)
    {
        return;
    }
    if ((ndsMPFindLineEndpoints(line_id, &left, NULL, NULL, NULL) == FALSE) ||
        (ndsMPLinePositionLocalToWorld(line_id, &left) == FALSE))
    {
        object_pos->x = 0.0F;
        object_pos->y = 0.0F;
        object_pos->z = 0.0F;
        return;
    }
    *object_pos = left;
}

void mpCollisionGetCeilEdgeR(s32 line_id, Vec3f *object_pos)
{
    Vec3f right;

    if (object_pos == NULL)
    {
        return;
    }
    if ((ndsMPFindLineEndpoints(line_id, NULL, &right, NULL, NULL) == FALSE) ||
        (ndsMPLinePositionLocalToWorld(line_id, &right) == FALSE))
    {
        object_pos->x = 0.0F;
        object_pos->y = 0.0F;
        object_pos->z = 0.0F;
        return;
    }
    *object_pos = right;
}

s32 mpCollisionGetEdgeUpperLLineID(s32 line_id)
{
    return ndsMPGetTopologyEdgeLineID(line_id, FALSE);
}

s32 mpCollisionGetEdgeUpperRLineID(s32 line_id)
{
    return ndsMPGetTopologyEdgeLineID(line_id, TRUE);
}

s32 mpCollisionGetEdgeLeftULineID(s32 line_id)
{
    return ndsMPGetTopologyEdgeLineID(line_id, TRUE);
}

s32 mpCollisionGetEdgeLeftDLineID(s32 line_id)
{
    return ndsMPGetTopologyEdgeLineID(line_id, FALSE);
}

s32 mpCollisionGetEdgeRightULineID(s32 line_id)
{
    return ndsMPGetTopologyEdgeLineID(line_id, TRUE);
}

s32 mpCollisionGetEdgeRightDLineID(s32 line_id)
{
    return ndsMPGetTopologyEdgeLineID(line_id, FALSE);
}

/* THE WALL SWEEP IS THE SOURCE'S, NOT AN APPROXIMATION OF IT.
 *
 * Until 2026-09-21 this was a generic two-sided segment intersection against
 * each wall's first and last vertex. Four things the source does were missing,
 * and each one is a playable defect on Peach's Castle (BUGS.md, stage rows):
 *
 *   - the "Same" tests ignored the yakumono translation, so a wall that lives
 *     on a positioned DObj -- the Castle's two blue side ramps are yakumono 3
 *     and 4 -- was tested at its LOCAL coordinates. mpProcessUpdateMain only
 *     runs the "Diff" form on the first of its sub-steps, so a fighter in
 *     knockback (more than 250 units a tic, several sub-steps) flew through
 *     the ramps on every sub-step after the first;
 *   - a slanted wall reported the normal of a vertical one, (+-1, 0);
 *   - a wall was hit from either side, where the source only tests the side
 *     the wall faces (an LWall when moving right, an RWall when moving left);
 *   - a start point exactly ON the wall missed, because a 0..1 parameter test
 *     has no tolerance and the source's flat test has 0.001.
 *
 * Transcribed from mpcollision.c: mpCollisionCheckLRSurfaceFlat (2078),
 * mpCollisionCheck{L,R}WallSurfaceTilt (2296 / 1790), mpCollisionGetLRAngle
 * (1755) and the four line sweeps (2138 / 2422 / 1597 / 1916). The one liberty
 * is the range reject: coll_pos_{prev,next} are s16, and for an integer c and a
 * real v, c < v iff c < ceil(v) and c > v iff c > floor(v), so the two bounds
 * are rounded once a group and the per-line test is an integer compare rather
 * than two soft-float ones. */
static sb32 __attribute__((noinline))
ndsMPCheckLRSurfaceFlat(s32 v1x, s32 v1y, s32 v2y, f32 vpdist_x, f32 vpdist_y,
                        f32 vtdist_x, f32 vtdist_y, f32 *hit_x, f32 *hit_y)
{
    f32 ddist_x = vpdist_x - vtdist_x;
    f32 vddist_x = (f32)v1x - vpdist_x;
    f32 ddist_y;
    f32 cross_y;
    s32 vnear;
    s32 vfar;

    if (ddist_x > 0.0F)
    {
        if ((((f32)v1x - 0.001F) > vpdist_x) || (vtdist_x >= (f32)v1x))
        {
            return FALSE;
        }
    }
    else if ((((f32)v1x + 0.001F) < vpdist_x) || (vtdist_x <= (f32)v1x))
    {
        return FALSE;
    }
    ddist_y = vpdist_y - vtdist_y;
    if ((v2y - v1y) < 0)
    {
        vnear = v2y;
        vfar = v1y;
    }
    else
    {
        vfar = v2y;
        vnear = v1y;
    }
    if (ddist_y > 0.0F)
    {
        if (((f32)vfar < vtdist_y) || (vpdist_y < (f32)vnear))
        {
            return FALSE;
        }
    }
    else if (((f32)vfar < vpdist_y) || (vtdist_y < (f32)vnear))
    {
        return FALSE;
    }
    cross_y = ((vddist_x / ddist_x) * ddist_y) + vpdist_y;
    if ((cross_y < (f32)vnear) || (cross_y > (f32)vfar))
    {
        return FALSE;
    }
    *hit_x = (f32)v1x;
    *hit_y = cross_y;
    return TRUE;
}

/* lr: -1 LWall, +1 RWall. The two source functions differ only in which side
 * of the line counts as "through it". */
static sb32 __attribute__((noinline))
ndsMPCheckWallSurfaceTilt(s32 v1x, s32 v1y, s32 v2x, s32 v2y, f32 d1x, f32 d1y,
                          f32 d2x, f32 d2y, s32 lr, f32 *dfx, f32 *dfy)
{
    s32 vdist_x = v2x - v1x;
    s32 vdist_y = v2y - v1y;
    f32 ddist_x = d1x - d2x;
    f32 ddist_y;
    s32 vnearx;
    s32 vfarx;
    s32 vneary;
    s32 vfary;
    f32 end_side;
    f32 start_x;
    f32 start_side;
    f32 vddist_x;
    f32 vddist_y;
    f32 numer;
    f32 scale;
    f32 vddiv;
    f32 vddistdiv;
    f32 vdscale;

    if (vdist_x < 0)
    {
        vnearx = v2x;
        vfarx = v1x;
    }
    else
    {
        vfarx = v2x;
        vnearx = v1x;
    }
    if (ddist_x > 0.0F)
    {
        if ((((f32)vfarx + 0.001F) < d2x) || (d1x < ((f32)vnearx - 0.001F)))
        {
            return FALSE;
        }
    }
    else if ((((f32)vfarx + 0.001F) < d1x) || (d2x < ((f32)vnearx - 0.001F)))
    {
        return FALSE;
    }
    ddist_y = d1y - d2y;
    if (vdist_y < 0)
    {
        vneary = v2y;
        vfary = v1y;
    }
    else
    {
        vfary = v2y;
        vneary = v1y;
    }
    if (ddist_y > 0.0F)
    {
        if (((f32)vfary < d2y) || (d1y < (f32)vneary))
        {
            return FALSE;
        }
    }
    else if (((f32)vfary < d1y) || (d2y < (f32)vneary))
    {
        return FALSE;
    }
    end_side = d2x - ((f32)v1x +
        (((d2y - (f32)v1y) / (f32)(v2y - v1y)) * (f32)vdist_x));
    if ((lr < 0) ? (end_side < 0.001F) : (end_side > -0.001F))
    {
        return FALSE;
    }
    start_x = (f32)v1x + (((d1y - (f32)v1y) / (f32)vdist_y) * (f32)vdist_x);
    start_side = d1x - start_x;
    if ((lr < 0) ? (start_side > -0.001F) : (start_side < 0.001F))
    {
        if (((lr < 0) ? (start_side < 0.001F) : (start_side > -0.001F)) &&
            (d1y <= (f32)vfary) && ((f32)vneary <= d1y))
        {
            *dfx = start_x;
            *dfy = d1y;
            return TRUE;
        }
        return FALSE;
    }
    vddist_x = (f32)v1x - d1x;
    vddist_y = (f32)v1y - d1y;
    numer = (ddist_y * vddist_x) - (ddist_x * vddist_y);
    scale = ((f32)vdist_y * ddist_x) - ((f32)vdist_x * ddist_y);
    vddiv = numer / scale;
    if (vddiv < 0.0F)
    {
        if (vddiv < -0.001F)
        {
            return FALSE;
        }
        numer = 0.0F;
    }
    else if (vddiv > 1.0F)
    {
        if (vddiv > 1.001F)
        {
            return FALSE;
        }
        numer = scale;
    }
    vddistdiv = (((f32)vdist_x * vddist_y) - ((f32)vdist_y * vddist_x)) / scale;
    if ((vddistdiv < -0.001F) || (vddistdiv > 1.001F))
    {
        return FALSE;
    }
    vdscale = 1.0F / scale;
    *dfx = (f32)v1x + ((numer * (f32)vdist_x) * vdscale);
    *dfy = (f32)v1y + ((numer * (f32)vdist_y) * vdscale);
    return TRUE;
}

/* `lr` is +1 or -1 at every call site; the slope comes from ndsMPSlopeUnit. */
static void ndsMPGetLRAngle(Vec3f *angle, s32 v1x, s32 v1y, s32 v2x, s32 v2y,
                            s32 lr)
{
    f32 px;
    f32 il;

    angle->z = 0.0F;
    if ((v2x == v1x) || (v2y == v1y))
    {
        if (v2x != v1x)
        {
            NDS_DIAG(gNdsStageCollisionLoopDivisionGuardCount++);
        }
        angle->x = (lr < 0) ? -1.0F : 1.0F;
        angle->y = 0.0F;
        return;
    }
    ndsMPSlopeUnit(v2x - v1x, v2y - v1y, &px, &il);
    if (lr < 0)
    {
        angle->x = -il;
        angle->y = -px;
    }
    else
    {
        angle->x = il;
        angle->y = px;
    }
}

/* (s32)v -- truncation toward zero, as the cast does -- and whether v has a
 * fractional part, on the bit pattern alone. Exact for |v| < 2^31 (biased
 * exponent < 158); returns FALSE for anything larger, infinite or NaN so the
 * caller keeps its soft-float form. */
static inline sb32 ndsMPF32TruncFrac(f32 v, s32 *trunc, u32 *frac)
{
    union
    {
        f32 f;
        u32 u;
    } b;
    u32 e;
    u32 m;
    s32 t;

    b.f = v;
    e = (b.u >> 23) & 0xffu;
    if (e >= 158u)
    {
        return FALSE;
    }
    if (e < 127u)
    {
        *trunc = 0;
        *frac = ((b.u & 0x7fffffffu) != 0u) ? 1u : 0u;
        return TRUE;
    }
    m = (b.u & 0x7fffffu) | 0x800000u;
    if (e >= 150u)
    {
        t = (s32)(m << (e - 150u));
        *frac = 0u;
    }
    else
    {
        u32 sh = 150u - e;

        t = (s32)(m >> sh);
        *frac = ((m & ((1u << sh) - 1u)) != 0u) ? 1u : 0u;
    }
    *trunc = ((b.u & 0x80000000u) != 0u) ? -t : t;
    return TRUE;
}

/* Sign bit clear. Only asked of values with a fractional part, which are
 * never zero, so it is exactly `v > 0`. */
#define NDS_MP_F32_POSITIVE(v) \
    ((((union { f32 f; u32 u; }){ .f = (v) }).u & 0x80000000u) == 0u)

volatile u32 gNdsMPWallSweepCalls;
volatile u32 gNdsMPWallSweepSegmentTests;
volatile u32 gNdsMPWallSweepHits;
/* Calls ndsMPWallSweepStaticMiss answered before the sweep (of the above). */
volatile u32 gNdsMPWallSweepStaticMisses;
extern volatile u32 gNdsTaskmanHeapGeneration;

/* The wall sweep visits every wall group of its kind on each call (Peach's
 * Castle: ~99 calls and ~290 group visits a frame), and each visit paid the
 * float preamble -- the yakumono offset, the +/-0.001 bounds, the integer
 * range -- before its line loop found nothing in range. A line is only
 * examined when its [coll_pos_prev, coll_pos_next] meets that range, so a
 * group whose lines' whole span misses a superset of the range examines
 * nothing: the superset comes from the operands' truncations with 4 units of
 * slack (each truncation is within 1, the bounds add 0.001, and the float
 * subtraction rounds far below a unit at these magnitudes). Such a group is
 * skipped before its preamble (P2-2p8, 2026-09-29). */
volatile u32 gNdsMPWallSweepGroupRejects;
#define NDS_MP_WALL_SWEEP_TRUNC_MAX (1 << 20)

/* trunc(v) when |v| < 2^20 (float spacing there is at most 1/8), else
 * FALSE. */
/* Out of ITCM again (2026-10-02): it and the AI process were the two
 * smallest of the four census admissions, and the tick-HUD gate ROM, whose
 * instrument keeps code in ITCM too, overflowed it by 144 B with all four. */
static sb32 __attribute__((noinline))
ndsMPWallSweepTrunc(f32 v, s32 *out)
{
    s32 t;
    u32 frac;

    if ((ndsMPF32TruncFrac(v, &t, &frac) == FALSE) ||
        (t >= NDS_MP_WALL_SWEEP_TRUNC_MAX) ||
        (t <= -NDS_MP_WALL_SWEEP_TRUNC_MAX))
    {
        return FALSE;
    }
    *out = t;
    return TRUE;
}

/* trunc of each yakumono's translate.x (which 0), speed.x (1), translate.y
 * (2) and speed.y (3), keyed by the float bits it came from: every group of
 * one yakumono and every call in a tick reads the same values.
 * Zero-initialised is 0.0f -> 0; INT32_MIN marks a value past the bound (its
 * groups are never rejected). 2 KB. */
static u32 sNdsMPWallSweepEdgeBits[NDS_MP_YAKUMONO_DOBJ_SLOTS][4];
static s32 sNdsMPWallSweepEdgeInt[NDS_MP_YAKUMONO_DOBJ_SLOTS][4];

/* The floor and ceiling sweeps' group reject (P2-2p8, 2026-09-29): a line
 * those sweeps test is one whose y span meets the sweep's y span within 0.001
 * (ndsMPLineExtentSweepRejectsKeys; the segment kernel's own y gate and the flat
 * branch's condition say the same), and a group's lines all lie inside the
 * group's [ext_lo, ext_hi] (their coll_pos, the y extents for these kinds).
 * The same truncated superset as the wall sweep's decides that no line of the
 * group can meet the sweep, and the group is skipped. */
volatile u32 gNdsMPSweepGroupRejects;

/* The floor and ceiling sweeps' per-segment x reject (P2-2p8, 2026-09-30).
 * ndsMPFCSegmentCrossesKernel's non-flat branch returns 0 when the segment's
 * x span [min(v1.x, v2.x), max(v1.x, v2.x)] misses the sweep's
 * [min(position.x, translate.x), max(...)] -- the same selections and the
 * same bit compares (nds_fcmp.h) on the same floats. Made in the sweep loop,
 * that test skips the call, its NULL guards and the two subtractions it opens
 * with; on Sector Z the kernel ran ~810 times a frame. A flat segment
 * (v1.y == v2.y) tests its crossing point instead of the span, so it always
 * goes to the kernel. */
volatile u32 gNdsMPSweepSegmentXRejects;

/* TRUE when a non-flat segment's x span misses the sweep's, with the sweep's
 * span as compare keys made once per group (2026-10-19; key_min the lesser of
 * position.x's and translate.x's keys, key_max the greater): the keys of the
 * segment's ends order as the floats do (nds_fcmp.h). */
static inline sb32 ndsMPSweepSegmentXMissesKeys(const Vec3f *v1, const Vec3f *v2,
                                                u32 key_min, u32 key_max)
{
    const u32 k1 = ndsFcmpKey(v1->x);
    const u32 k2 = ndsFcmpKey(v2->x);
    const u32 lo = (k1 < k2) ? k1 : k2;
    const u32 hi = (k1 > k2) ? k1 : k2;

    return ((hi < key_min) || (key_max < lo)) ? TRUE : FALSE;
}

/* ndsMPWallSweepTrunc's answer without its two calls: trunc(v) when
 * |v| < 2^20, else FALSE. */
static inline sb32 ndsMPWallSweepTruncInline(f32 v, s32 *out)
{
    u32 b;
    u32 e;
    s32 t;

    __builtin_memcpy(&b, &v, sizeof(b));
    e = (b >> 23) & 0xffu;
    if (e < 127u)
    {
        *out = 0;
        return TRUE;
    }
    if (e >= 147u)
    {
        return FALSE;
    }
    t = (s32)(((b & 0x7fffffu) | 0x800000u) >> (150u - e));
    *out = ((b & 0x80000000u) != 0u) ? -t : t;
    return TRUE;
}

/* The floor and ceiling sweeps' segment window (P2-2p8, 2026-10-05). The
 * per-segment x reject above still fetched both vertices of every segment and
 * keyed four floats to prove most of a long line irrelevant: Sector Z's floor
 * sweep ran ~570 instructions a line visit, 55K cycles a frame. On a line
 * whose vertex x never reverses (sNdsMPLineXOrder), the segments whose x span
 * meets an interval are one contiguous run, found by two binary searches, and
 * the sweep walks only that run.
 *
 * Exact, because the interval [lo, hi] contains everything a skipped segment
 * would need: a non-flat segment hits only where its x span meets the sweep's
 * [min, max] of position.x / translate.x (the reject above, and the kernel's
 * own gate), and lo/hi sit at least one unit outside that span (truncated
 * bounds less/plus one). A flat segment tests an extrapolated crossing x
 * instead (ndsMPFCSegmentCrossesKernel): for a floor it hits only while
 * position.y > translate.y, at position.x + t (position.x - translate.x) with
 * t in (-1, 0.001 / dy], so it can land outside the sweep's span by at most
 * 0.001 |dx| / dy. The window takes that as a margin when dy > 1 (truncated y
 * at least 3 apart): |dx| + 2 units, which also covers the truncation and the
 * rounding of the crossing; with dy <= 0 the flat branch never hits and needs
 * none; in between the line is walked whole. Ceilings mirror the sign. The
 * walked run is in the original order, so ties resolve as before. A long
 * ordered line whose whole x extent misses the window is skipped outright,
 * and the point queries (mpCollisionGetFCCommonFloor / ...Ceil) start their
 * walk where the point can first be bracketed (ndsMPLinePointSegmentSearch).
 * Only lines of at least NDS_MP_SEGMENT_WINDOW_MIN vertices take any of it:
 * on short or folded lines the bounds cost more than they saved. */
volatile u32 gNdsMPSweepLineXRejects;

/* [*lo, *hi] for one sweep (ud +1 floor, -1 ceiling), or FALSE to walk every
 * segment. */
static sb32 __attribute__((noinline))
ndsMPSweepWindowBounds(const Vec3f *position, const Vec3f *translate, s32 ud,
                       s32 *lo, s32 *hi)
{
    s32 px;
    s32 tx;
    s32 py;
    s32 ty;
    s32 dx;
    s32 margin;

    if ((ndsMPWallSweepTruncInline(position->x, &px) == FALSE) ||
        (ndsMPWallSweepTruncInline(translate->x, &tx) == FALSE) ||
        (ndsMPWallSweepTruncInline(position->y, &py) == FALSE) ||
        (ndsMPWallSweepTruncInline(translate->y, &ty) == FALSE))
    {
        return FALSE;
    }
    dx = (px > tx) ? (px - tx) : (tx - px);
    if ((ud > 0) ? NDS_FCMP_LE(position->y, translate->y) :
                   NDS_FCMP_GE(position->y, translate->y))
    {
        margin = 0;
    }
    else if (((ud > 0) ? (py - ty) : (ty - py)) >= 3)
    {
        margin = dx + 2;
    }
    else
    {
        return FALSE;
    }
    *lo = ((px < tx) ? px : tx) - 1 - margin;
    *hi = ((px > tx) ? px : tx) + 1 + margin;
    return TRUE;
}

static inline s32 ndsMPLineVertexXAt(MPVertexArray *ids,
                                     MPVertexPosContainer *verts,
                                     u32 vertex_first, u32 k)
{
    return ndsMPVertexX(verts, ndsMPVertexID(ids, vertex_first + k));
}

/* The run [*first, *last] of a monotonic line's segments whose x span meets
 * [lo, hi] (empty when *first > *last). Segment j spans vertices j, j + 1. */
static void __attribute__((noinline))
ndsMPLineSegmentWindow(MPVertexArray *ids, MPVertexPosContainer *verts,
                       u32 vertex_first, u32 vertex_count, u32 order,
                       s32 lo, s32 hi, u32 *first, u32 *last)
{
    const sb32 up = (order == NDS_MP_LINE_XORDER_UP) ? TRUE : FALSE;
    u32 a = 0u;
    u32 b = vertex_count;

    /* The first vertex inside the window's near edge (up: x >= lo, down:
     * x <= hi); every segment ending before it misses. */
    while (a < b)
    {
        const u32 m = (a + b) >> 1;
        const s32 x = ndsMPLineVertexXAt(ids, verts, vertex_first, m);

        if ((up != FALSE) ? (x < lo) : (x > hi))
        {
            a = m + 1u;
        }
        else
        {
            b = m;
        }
    }
    *first = (a == 0u) ? 0u : (a - 1u);
    /* The first vertex past the far edge (up: x > hi, down: x < lo); every
     * segment starting there or later misses. */
    a = 0u;
    b = vertex_count;
    while (a < b)
    {
        const u32 m = (a + b) >> 1;
        const s32 x = ndsMPLineVertexXAt(ids, verts, vertex_first, m);

        if ((up != FALSE) ? (x <= hi) : (x >= lo))
        {
            a = m + 1u;
        }
        else
        {
            b = m;
        }
    }
    if (a == 0u)
    {
        *first = 1u;
        *last = 0u;
        return;
    }
    *last = ((a - 1u) < (vertex_count - 2u)) ? (a - 1u) : (vertex_count - 2u);
}

/* The point queries' (mpCollisionGetFCCommonFloor / ...Ceil) first segment
 * worth testing for `object_x`. Their loop takes the first segment whose ends
 * bracket object_x; on a line whose x never reverses, every segment before
 * the one ending at the first vertex past object_x (up: x >= object_x, down:
 * x <= object_x) ends short of it, so starting there finds the same segment.
 * Same word as the sweeps' window; NaN and infinities walk the whole line. */
static u32 __attribute__((noinline))
ndsMPLinePointSegmentSearch(MPVertexArray *ids, MPVertexPosContainer *verts,
                            s32 line_id, u32 vertex_first, u32 vertex_count,
                            f32 object_x)
{
    sb32 up;
    u32 a = 0u;
    u32 b = vertex_count;

    if ((vertex_count < NDS_MP_SEGMENT_WINDOW_MIN) || (line_id < 0) ||
        ((u32)line_id >= NDS_MP_LINE_EXTENT_MAX) ||
        (sNdsMPLineExtentValid[line_id] == 0u) ||
        (sNdsMPLineXOrder[line_id] == NDS_MP_LINE_XORDER_NONE) ||
        (((ndsFcmpBits(object_x) >> 23) & 0xffu) == 0xffu))
    {
        return 0u;
    }
    up = (sNdsMPLineXOrder[line_id] == NDS_MP_LINE_XORDER_UP) ? TRUE : FALSE;
    while (a < b)
    {
        const u32 m = (a + b) >> 1;
        f32 fx;
        f32 fy;

        ndsMPVertexF32Get(verts, ndsMPVertexID(ids, vertex_first + m), &fx,
                          &fy);
        if ((up != FALSE) ? NDS_FCMP_LT(fx, object_x) :
                            NDS_FCMP_GT(fx, object_x))
        {
            a = m + 1u;
        }
        else
        {
            b = m;
        }
    }
    return (a == 0u) ? 0u : (a - 1u);
}

/* The segment run a sweep walks on `line_id` (`state`: 0 bounds not yet
 * computed for this group, 1 held in `lo` and `hi`, 2 none). On a long line
 * whose x never reverses: FALSE when the line's whole x extent misses the
 * window (every segment's span lies inside it, so none can meet the sweep),
 * else TRUE with the window's run. Any other line: TRUE with all of it. */
static inline sb32 ndsMPSweepSegmentRun(MPVertexArray *ids,
                                        MPVertexPosContainer *verts,
                                        s32 line_id, u32 vertex_first,
                                        u32 vertex_count,
                                        const Vec3f *sweep_position,
                                        const Vec3f *sweep_translate, s32 ud,
                                        u32 *state, s32 *lo, s32 *hi,
                                        u32 *first, u32 *last)
{
    *first = 0u;
    *last = vertex_count - 2u;
    /* Only a long line whose x never reverses pays for the bounds (Yoshi's
     * Island's short or folded lines paid them for nothing: +2.5K a frame). */
    if ((vertex_count < NDS_MP_SEGMENT_WINDOW_MIN) || (line_id < 0) ||
        ((u32)line_id >= NDS_MP_LINE_EXTENT_MAX) ||
        (sNdsMPLineExtentValid[line_id] == 0u) ||
        (sNdsMPLineXOrder[line_id] == NDS_MP_LINE_XORDER_NONE))
    {
        return TRUE;
    }
    if (*state == 0u)
    {
        *state = (ndsMPSweepWindowBounds(sweep_position, sweep_translate, ud,
                                         lo, hi) != FALSE) ? 1u : 2u;
    }
    if (*state != 1u)
    {
        return TRUE;
    }
    if (((s32)sNdsMPLineExtentMaxXi[line_id] < *lo) ||
        ((s32)sNdsMPLineExtentMinXi[line_id] > *hi))
    {
        NDS_DIAG(gNdsMPSweepLineXRejects++);
        return FALSE;
    }
    ndsMPLineSegmentWindow(ids, verts, vertex_first, vertex_count,
                           sNdsMPLineXOrder[line_id], *lo, *hi, first, last);
    return TRUE;
}

/* 1 = reject group `g`'s lines for this sweep, on axis `axis` (0 x for the
 * walls, 1 y for floors and ceilings). `state` is the call's lazy truncation
 * of position / translate on that axis (0 not yet, 1 held, 2 unusable).
 * A group whose checks keep failing (a moving platform the fighter stands
 * near) stops being checked for a while: the check is pure cost there
 * (Mushroom Kingdom's walls rejected ~6% of visits). Whether a group is
 * checked never changes what the sweep finds. */
static s32 ndsMPWallSweepEdgeTrunc(u32 slot, u32 which, f32 v);
#define NDS_MP_SWEEP_REJECT_MISS_LIMIT 8u
#define NDS_MP_SWEEP_REJECT_SKIP 32u

/* TRUE when no span of group `g` meets [lo, hi] (P2-6, 2026-10-02). A line
 * is examined only when its own [coll_pos_prev, coll_pos_next] meets the
 * sweep's range, so the group's whole extent was a loose stand-in: the Race
 * keeps all its walls in one group spanning the course, which no sweep ever
 * missed (3% of 184 calls a frame rejected). The spans are sorted and
 * disjoint; span_count 0 means none were built, and nothing is rejected. */
static inline sb32 ndsMPGroupSpansMiss(const NDSMPKindGroup *g, s32 lo,
                                       s32 hi)
{
    const NDSMPGroupSpan *s = sNdsMPGroupSpans + g->span_first;
    const NDSMPGroupSpan *end = s + g->span_count;

    if (g->span_count == 0u)
    {
        return FALSE;
    }
    for (; s < end; s++)
    {
        if ((s32)s->lo > hi)
        {
            return TRUE;
        }
        if ((s32)s->hi >= lo)
        {
            return FALSE;
        }
    }
    return TRUE;
}
/* ITCM (P2-6, 2026-10-02: census admission, 312 B). */
static sb32 __attribute__((noinline)) NDS_R2_ITCM_PACK2_CODE
ndsMPSweepGroupReject(u32 *state, s32 *ip0, s32 *it0, u32 axis,
                      const Vec3f *position, const Vec3f *translate,
                      NDSMPKindGroup *g, u32 yakumono_id, sb32 is_dynamic,
                      f32 vedge, f32 speed, sb32 has_speed)
{
    s32 ip;
    s32 it;

    if (g->reject_skip != 0u)
    {
        g->reject_skip--;
        return FALSE;
    }
    if (*state == 0u)
    {
        *state = ((ndsMPWallSweepTrunc((axis == 0u) ? position->x :
                                                      position->y,
                                       ip0) != FALSE) &&
                  (ndsMPWallSweepTrunc((axis == 0u) ? translate->x :
                                                      translate->y,
                                       it0) != FALSE)) ? 1u : 2u;
    }
    if (*state != 1u)
    {
        return FALSE;
    }
    ip = *ip0;
    it = *it0;
    if (is_dynamic != FALSE)
    {
        s32 ie = ndsMPWallSweepEdgeTrunc(yakumono_id, axis * 2u, vedge);
        s32 id = (has_speed != FALSE) ?
            ndsMPWallSweepEdgeTrunc(yakumono_id, axis * 2u + 1u, speed) : 0;

        if ((ie == INT32_MIN) || (id == INT32_MIN))
        {
            return FALSE;
        }
        ip = ip - ie + id;
        it = it - ie;
    }
    if ((((ip > it) ? ip : it) + NDS_MP_WALL_SWEEP_SLACK < (s32)g->ext_lo) ||
        (((ip < it) ? ip : it) - NDS_MP_WALL_SWEEP_SLACK > (s32)g->ext_hi) ||
        (ndsMPGroupSpansMiss(g, ((ip < it) ? ip : it) - NDS_MP_WALL_SWEEP_SLACK,
                             ((ip > it) ? ip : it) + NDS_MP_WALL_SWEEP_SLACK) !=
         FALSE))
    {
        g->reject_misses = 0u;
        if (axis == 0u)
        {
            NDS_DIAG(gNdsMPWallSweepGroupRejects++);
        }
        else
        {
            NDS_DIAG(gNdsMPSweepGroupRejects++);
        }
        return TRUE;
    }
    if (++g->reject_misses >= NDS_MP_SWEEP_REJECT_MISS_LIMIT)
    {
        g->reject_misses = 0u;
        g->reject_skip = (u8)NDS_MP_SWEEP_REJECT_SKIP;
    }
    return FALSE;
}

static s32 __attribute__((noinline))
ndsMPWallSweepEdgeTrunc(u32 slot, u32 which, f32 v)
{
    u32 bits;

    __builtin_memcpy(&bits, &v, sizeof(bits));
    if (bits != sNdsMPWallSweepEdgeBits[slot][which])
    {
        s32 t;

        sNdsMPWallSweepEdgeBits[slot][which] = bits;
        sNdsMPWallSweepEdgeInt[slot][which] =
            (ndsMPWallSweepTrunc(v, &t) != FALSE) ? t : INT32_MIN;
    }
    return sNdsMPWallSweepEdgeInt[slot][which];
}

/* P2-6 (2026-10-03): the groups one sweep call may examine, as a mask over
 * sNdsMPKindGroups[line_kind] (bit i = group i; all ones when the kind has
 * no groups for this geometry or more than 64, or a truncation fails). A
 * cleared bit is a group ndsMPSweepGroupReject's extent test rejects -- the
 * same truncations, yakumono offsets and slack -- so no line of it can meet
 * the sweep, and skipping it changes nothing the sweep finds (the reject's
 * own backoff only ever decides whether to look). The per-group reject paid
 * a call with eleven arguments and two edge lookups on every visit; Board
 * the Platforms visits ~22 dynamic wall groups per call, 24 calls a frame,
 * and rejects all of them (~100K ticks a frame on Purin's board). The
 * span test stays in the reject for the groups this mask keeps. */
#define NDS_MP_SWEEP_CANDIDATE_MIN 8u

static u64 __attribute__((noinline))
ndsMPSweepCandidates(u32 line_kind, u32 axis, const Vec3f *position,
                     const Vec3f *translate, sb32 is_diff)
{
    const NDSMPKindGroup *g;
    const sb32 has_speed =
        ((is_diff != FALSE) && (gMPCollisionSpeeds != NULL)) ? TRUE : FALSE;
    u64 mask = ~(u64)0;
    u32 count;
    u32 i;
    s32 ip;
    s32 it;

    if ((sNdsMPKindGroupGeometry != gMPCollisionGeometry) ||
        (gMPCollisionYakumonoDObjs == NULL))
    {
        return mask;
    }
    count = sNdsMPKindGroupCounts[line_kind];
    /* Under NDS_MP_SWEEP_CANDIDATE_MIN groups the per-group rejects are
     * already cheap; the VS stages stay out of the mask. */
    if ((count < NDS_MP_SWEEP_CANDIDATE_MIN) || (count > 64u) ||
        (ndsMPWallSweepTrunc((axis == 0u) ? position->x : position->y,
                             &ip) == FALSE) ||
        (ndsMPWallSweepTrunc((axis == 0u) ? translate->x : translate->y,
                             &it) == FALSE))
    {
        return mask;
    }
    g = sNdsMPKindGroups[line_kind];
    for (i = 0u; i < count; i++, g++)
    {
        const u32 yakumono_id = g->yakumono_id;
        const DObj *dobj = (yakumono_id < NDS_MP_YAKUMONO_DOBJ_SLOTS) ?
            gMPCollisionYakumonoDObjs->dobjs[yakumono_id] : NULL;
        s32 p = ip;
        s32 t = it;
        s32 lo;
        s32 hi;

        if ((dobj != NULL) &&
            ((dobj->anim_joint.event32 != NULL) ||
             (dobj->user_data.s != nMPYakumonoStatusNone)))
        {
            /* ndsMPSweepGroupReject's dynamic offsets: the yakumono's edge on
             * this axis and, for a Diff sweep, its speed. */
            const s32 ie = ndsMPWallSweepEdgeTrunc(
                yakumono_id, axis * 2u,
                (axis == 0u) ? dobj->translate.vec.f.x :
                               dobj->translate.vec.f.y);
            const s32 id = (has_speed != FALSE) ?
                ndsMPWallSweepEdgeTrunc(
                    yakumono_id, axis * 2u + 1u,
                    (axis == 0u) ? gMPCollisionSpeeds[yakumono_id].x :
                                   gMPCollisionSpeeds[yakumono_id].y) : 0;

            if ((ie == INT32_MIN) || (id == INT32_MIN))
            {
                continue;
            }
            p = p - ie + id;
            t = t - ie;
        }
        lo = ((p < t) ? p : t) - NDS_MP_WALL_SWEEP_SLACK;
        hi = ((p > t) ? p : t) + NDS_MP_WALL_SWEEP_SLACK;
        if ((hi < (s32)g->ext_lo) || (lo > (s32)g->ext_hi))
        {
            mask &= ~((u64)1u << i);
        }
    }
    return mask;
}

/* ud: +1 floor (crossed downward), -1 ceiling (crossed upward). */
static sb32 ndsMPFCSegmentCrosses(const Vec3f *position,
                                  const Vec3f *translate,
                                  const Vec3f *v1,
                                  const Vec3f *v2,
                                  s32 ud,
                                  f32 *hit_x,
                                  f32 *hit_y)
{
    if ((position == NULL) || (translate == NULL) || (v1 == NULL) ||
        (v2 == NULL) || (hit_x == NULL) || (hit_y == NULL))
    {
        return FALSE;
    }
    return (ndsMPFCSegmentCrossesKernel(
        position->x, position->y, translate->x, translate->y,
        v1->x, v1->y, v2->x, v2->y, (int)ud, hit_x, hit_y) != 0) ?
        TRUE : FALSE;
}

/* Main RAM again (2026-10-04). It was admitted to ITCM on 2026-09-27, when
 * every wall sweep ran it (~8.9K ticks/frame of non-memory stall at 1,644 B;
 * WORK-H paired median -6.7K). Since the all-reject fast path
 * (ndsMPWallSweepStaticMiss, 2026-10-02) answers most calls first, the late
 * gate census rents its 2,016 B at ~840 cycles a byte; the fast path and a
 * census section D pack take the bytes (linker/nds_hot_text.ld). */
static sb32
ndsStageMPAdjustFloorLoopWallSweep(Vec3f *position,
                                               Vec3f *translate,
                                               Vec3f *ga_last,
                                               s32 *stand_line_id,
                                               u32 *stand_coll_flags,
                                               Vec3f *angle,
                                               u32 line_kind,
                                               sb32 is_diff)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPVertexLinks *links;
    MPVertexArray *ids;
    MPVertexPosContainer *verts;
    NDSMPKindGroup *group;
    u32 group_count;
    u32 i;
    s32 lr = (line_kind == nMPLineKindLWall) ? -1 : +1;
    f32 line_project_pos = 3.402823466e+38F;
    sb32 found = FALSE;
    u32 reject_state = 0u;
    s32 ipx = 0;
    s32 itx = 0;
    u64 candidates;

    if ((position == NULL) || (translate == NULL) ||
        ((line_kind != nMPLineKindLWall) && (line_kind != nMPLineKindRWall)) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL))
    {
        return FALSE;
    }
    if ((sNdsMPTopologyGeometry != geometry) ||
        (gMPCollisionVertexInfo == NULL))
    {
        ndsMPCollisionEnsureLineGroups();
        if ((sNdsMPTopologyGeometry != geometry) ||
            (gMPCollisionVertexInfo == NULL))
        {
            return FALSE;
        }
    }
    if (sNdsMPKindGroupGeometry != geometry)
    {
        return FALSE;
    }
    NDS_DIAG(gNdsMPWallSweepCalls++);
    links = geometry->vertex_links;
    ids = geometry->vertex_id;
    verts = geometry->vertex_data;
    group = sNdsMPKindGroups[line_kind];
    group_count = sNdsMPKindGroupCounts[line_kind];
    candidates = ndsMPSweepCandidates(line_kind, 0u, position, translate,
                                      is_diff);
    for (i = 0u; i < group_count; i++, group++)
    {
        s32 first = (s32)group->first;
        s32 count = (s32)group->count;
        u32 yakumono_id = group->yakumono_id;
        DObj *yakumono_dobj;
        f32 vedge_x = 0.0F;
        f32 vedge_y = 0.0F;
        f32 dynamic_x = 0.0F;
        f32 vpdist_x = position->x;
        f32 vpdist_y = position->y;
        f32 vtdist_x = translate->x;
        f32 vtdist_y = translate->y;
        f32 vdist1;
        f32 vdist2;
        s32 range_lo;
        s32 range_hi;
        s32 y_lo;
        s32 y_hi;
        u32 frac_hi;
        s32 line_id;
        sb32 dynamic;

        if ((count <= 0) || (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS) ||
            (((candidates >> i) & 1u) == 0u))
        {
            continue;
        }
        if (count > 4096)
        {
            count = 4096;
        }
        yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
        if ((yakumono_dobj == NULL) ||
            (yakumono_dobj->user_data.s >= nMPYakumonoStatusOff))
        {
            continue;
        }
        dynamic = ((yakumono_dobj->anim_joint.event32 != NULL) ||
                   (yakumono_dobj->user_data.s != nMPYakumonoStatusNone)) ?
            TRUE : FALSE;
        /* The lazy truncations happen on the first group this call checks
         * (Zebes has no wall groups). */
        if ((reject_state != 2u) &&
            (ndsMPSweepGroupReject(
                 &reject_state, &ipx, &itx, 0u, position, translate, group,
                 yakumono_id, dynamic,
                 (dynamic != FALSE) ? yakumono_dobj->translate.vec.f.x : 0.0F,
                 ((dynamic != FALSE) && (is_diff != FALSE) &&
                  (gMPCollisionSpeeds != NULL)) ?
                     gMPCollisionSpeeds[yakumono_id].x : 0.0F,
                 ((is_diff != FALSE) && (gMPCollisionSpeeds != NULL)) ?
                     TRUE : FALSE) != FALSE))
        {
            continue;
        }
        if (dynamic != FALSE)
        {
            vedge_x = yakumono_dobj->translate.vec.f.x;
            vedge_y = yakumono_dobj->translate.vec.f.y;
            vpdist_x -= vedge_x;
            vpdist_y -= vedge_y;
            vtdist_x -= vedge_x;
            vtdist_y -= vedge_y;
            if ((is_diff != FALSE) && (gMPCollisionSpeeds != NULL))
            {
                dynamic_x = gMPCollisionSpeeds[yakumono_id].x;
                vpdist_x += dynamic_x;
                vpdist_y += gMPCollisionSpeeds[yakumono_id].y;
            }
        }
        if (vpdist_x < vtdist_x)
        {
            vdist1 = vpdist_x - 0.001F;
            vdist2 = vtdist_x + 0.001F;
        }
        else
        {
            vdist1 = vtdist_x - 0.001F;
            vdist2 = vpdist_x + 0.001F;
        }
        /* ceil(vdist1) and floor(vdist2) from the bits (2026-09-27): the
         * truncate-then-compare form cost an __aeabi_f2iz, an __aeabi_i2f and a
         * float compare per bound, per group, per call (~180 i2f a frame).
         * Truncation toward zero plus a fraction flag gives the same integers
         * for every |v| < 2^31; anything else keeps the soft-float form. */
        {
            u32 frac;

            if (ndsMPF32TruncFrac(vdist1, &range_lo, &frac) != FALSE)
            {
                if ((frac != 0u) && NDS_MP_F32_POSITIVE(vdist1))
                {
                    range_lo++;
                }
            }
            else
            {
                range_lo = (s32)vdist1;
                if ((f32)range_lo < vdist1)
                {
                    range_lo++;
                }
            }
            if (ndsMPF32TruncFrac(vdist2, &range_hi, &frac) != FALSE)
            {
                if ((frac != 0u) && !NDS_MP_F32_POSITIVE(vdist2))
                {
                    range_hi--;
                }
            }
            else
            {
                range_hi = (s32)vdist2;
                if ((f32)range_hi > vdist2)
                {
                    range_hi--;
                }
            }
        }
        /* P2-2p8 (2026-10-05, owner: fixed point only): the motion's y span
         * as integers, floor of its low end and ceiling of its high one, for
         * the per-segment box test below. A segment whose integer x span
         * misses [range_lo, range_hi] or whose y span misses [y_lo, y_hi]
         * fails both surface tests' own bound checks (the x one carries the
         * same 0.001 slack range_lo/hi do; the y one has none), so it is
         * skipped before any float compare: exact, not an approximation.
         * Out-of-range motion keeps every segment (y_lo > y_hi never). */
        {
            const f32 y_min = NDS_FCMP_LT(vpdist_y, vtdist_y) ? vpdist_y :
                                                                vtdist_y;
            const f32 y_max = NDS_FCMP_LT(vpdist_y, vtdist_y) ? vtdist_y :
                                                                vpdist_y;
            u32 frac;

            if ((ndsMPF32TruncFrac(y_min, &y_lo, &frac) == FALSE) ||
                (ndsMPF32TruncFrac(y_max, &y_hi, &frac_hi) == FALSE))
            {
                y_lo = INT32_MIN;
                y_hi = INT32_MAX;
            }
            else
            {
                if ((frac != 0u) && !NDS_MP_F32_POSITIVE(y_min))
                {
                    y_lo--;
                }
                if ((frac_hi != 0u) && NDS_MP_F32_POSITIVE(y_max))
                {
                    y_hi++;
                }
            }
        }
        for (line_id = first; line_id < (first + count); line_id++)
        {
            const MPVertexInfo *vinfo;
            u32 vertex_first;
            u32 vertex_count;
            u32 vertex;
            s32 vpos_x;
            s32 vpos_y;

            if (line_id >= gMPCollisionLinesNum)
            {
                break;
            }
            vinfo = &gMPCollisionVertexInfo->vertex_info[line_id];
            /* Lines are ordered by x within a group -- ascending for LWalls,
             * descending for RWalls -- so the far-side miss ends the group. */
            if (lr < 0)
            {
                if ((s32)vinfo->coll_pos_next < range_lo)
                {
                    continue;
                }
                if ((s32)vinfo->coll_pos_prev > range_hi)
                {
                    break;
                }
            }
            else
            {
                if ((s32)vinfo->coll_pos_prev > range_hi)
                {
                    continue;
                }
                if ((s32)vinfo->coll_pos_next < range_lo)
                {
                    break;
                }
            }
            vertex_first = ndsMPVertexLinkFirst(links, (u32)line_id);
            vertex_count = ndsMPVertexLinkCount(links, (u32)line_id);
            if ((vertex_count < 2u) || (vertex_count > 128u))
            {
                NDS_DIAG(gNdsStageCollisionLoopBadVertexCount++);
                continue;
            }
            vpos_x = ndsMPVertexX(verts, ndsMPVertexID(ids, vertex_first));
            vpos_y = ndsMPVertexY(verts, ndsMPVertexID(ids, vertex_first));
            for (vertex = vertex_first;
                 vertex < (vertex_first + vertex_count - 1u); vertex++)
            {
                s32 prev_x = vpos_x;
                s32 prev_y = vpos_y;
                u32 next_id = ndsMPVertexID(ids, vertex + 1u);
                f32 hit_x;
                f32 hit_y;
                f32 project;
                sb32 flat;

                vpos_x = ndsMPVertexX(verts, next_id);
                vpos_y = ndsMPVertexY(verts, next_id);
                flat = (prev_x == vpos_x) ? TRUE : FALSE;
                NDS_DIAG(gNdsMPWallSweepSegmentTests++);
                /* The integer box test (see y_lo above). One unit of slack
                 * on x covers the rounding of the tests' own +-0.001. */
                if ((((prev_x > vpos_x) ? prev_x : vpos_x) + 1 < range_lo) ||
                    (((prev_x < vpos_x) ? prev_x : vpos_x) - 1 > range_hi) ||
                    (((prev_y > vpos_y) ? prev_y : vpos_y) < y_lo) ||
                    (((prev_y < vpos_y) ? prev_y : vpos_y) > y_hi))
                {
                    continue;
                }
                if (flat != FALSE)
                {
                    if (((lr < 0) ? (vtdist_x > vpdist_x) :
                                    (vtdist_x < vpdist_x)) == FALSE)
                    {
                        continue;
                    }
                    if (ndsMPCheckLRSurfaceFlat(prev_x, prev_y, vpos_y,
                            vpdist_x, vpdist_y, vtdist_x, vtdist_y,
                            &hit_x, &hit_y) == FALSE)
                    {
                        continue;
                    }
                }
                else if (ndsMPCheckWallSurfaceTilt(prev_x, prev_y, vpos_x,
                             vpos_y, vpdist_x, vpdist_y, vtdist_x, vtdist_y,
                             lr, &hit_x, &hit_y) == FALSE)
                {
                    continue;
                }
                project = hit_x - (vpdist_x - dynamic_x);
                if (project < 0.0F)
                {
                    project = -project;
                }
                if (line_project_pos <= project)
                {
                    continue;
                }
                if (ga_last != NULL)
                {
                    ga_last->x = hit_x + vedge_x;
                    ga_last->y = hit_y + vedge_y;
                    ga_last->z = 0.0F;
                }
                if (stand_line_id != NULL)
                {
                    *stand_line_id = line_id;
                }
                if (angle != NULL)
                {
                    if (flat != FALSE)
                    {
                        angle->x = (f32)lr;
                        angle->y = 0.0F;
                        angle->z = 0.0F;
                    }
                    else
                    {
                        ndsMPGetLRAngle(angle, prev_x, prev_y, vpos_x, vpos_y,
                                        lr);
                    }
                }
                if (stand_coll_flags != NULL)
                {
                    *stand_coll_flags = ndsMPVertexFlags(
                        verts, ndsMPVertexID(ids, vertex));
                }
                line_project_pos = project;
                found = TRUE;
            }
        }
    }
    if (found != FALSE)
    {
        NDS_DIAG(gNdsMPWallSweepHits++);
    }
    return found;
}

/* The wall sweep's all-reject fast path (P2-6, 2026-10-02). With the spans,
 * the group reject proves ~94% of the Race's ~184 wall sweeps a frame find
 * nothing, but each still paid the sweep's whole entry: a nine-register
 * Thumb prologue with ~200 B of spills, the geometry checks, a 12-argument
 * call and two out-of-line truncations. This asks the same question first,
 * for a kind whose examined groups are all static, with the same truncation
 * and the same extent and span tests. It answers TRUE only when the sweep
 * would have rejected every group it examines -- the sweep then returns
 * FALSE having written nothing -- and makes the sweep's own state changes
 * (each rejected group's reject_misses, the two counters). Anything else (a
 * backed-off group, an operand past 2^20, geometry not current) goes to the
 * sweep untouched. For a static group the Same and Diff forms are the same
 * query. A dynamic group (an animated or moving yakumono: Saffron's door, the
 * moving platforms) takes ndsMPSweepGroupReject's own shift -- the range
 * minus the yakumono's X edge, plus its speed in the Diff form -- so it is
 * rejected exactly when the sweep would reject it (P2-2p8, 2026-10-04: on
 * Saffron half of ~128 wall sweeps a frame fell through to the full sweep
 * for a dynamic group nowhere near them). */
static sb32 __attribute__((noinline))
ndsMPWallSweepStaticMiss(const Vec3f *position, const Vec3f *translate,
                         u32 line_kind, sb32 is_diff)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    NDSMPKindGroup *group;
    u32 group_count;
    u32 rejected = 0u;
    u32 i;
    s32 ip;
    s32 it;
    s32 lo;
    s32 hi;

    if ((position == NULL) || (translate == NULL))
    {
        return FALSE;
    }
    /* P2-2p8 (2026-10-05): the geometry checks are proved once per geometry
     * and heap generation, not on each of ~100 calls a frame: within both,
     * only a topology reset or rebuild (which clears the proof) or a scene's
     * end (a new heap generation) can change what they test. */
    if ((geometry != sNdsMPWallMissReadyGeometry) ||
        (sNdsMPWallMissReadyGen != gNdsTaskmanHeapGeneration))
    {
        if ((geometry == NULL) ||
            (sNdsMPKindGroupGeometry != geometry) ||
            (sNdsMPTopologyGeometry != geometry) ||
            (gMPCollisionVertexInfo == NULL) ||
            (gMPCollisionYakumonoDObjs == NULL) ||
            (ndsStageCollisionLoopGeometryReady() == FALSE))
        {
            return FALSE;
        }
        sNdsMPWallMissReadyGeometry = geometry;
        sNdsMPWallMissReadyGen = gNdsTaskmanHeapGeneration;
    }
    if ((ndsMPWallSweepTruncInline(position->x, &ip) == FALSE) ||
        (ndsMPWallSweepTruncInline(translate->x, &it) == FALSE))
    {
        return FALSE;
    }
    lo = ((ip < it) ? ip : it) - NDS_MP_WALL_SWEEP_SLACK;
    hi = ((ip > it) ? ip : it) + NDS_MP_WALL_SWEEP_SLACK;
    group = sNdsMPKindGroups[line_kind];
    group_count = sNdsMPKindGroupCounts[line_kind];
    /* P2-2p8 (2026-10-05): one pass. Each group proved rejected takes the
     * sweep's reject bookkeeping (reject_misses = 0) as it is proved, instead
     * of a second pass after all are: if a later group sends the call to the
     * sweep, the sweep rejects the earlier ones again and writes the same 0
     * (or, past its candidate mask, skips them). reject_misses only paces the
     * backoff of a check that never changes what a sweep finds. */
    for (i = 0u; i < group_count; i++)
    {
        NDSMPKindGroup *g = &group[i];
        const DObj *yakumono_dobj;

        if ((g->count == 0u) ||
            (g->yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
        {
            continue;
        }
        yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[g->yakumono_id];
        if ((yakumono_dobj == NULL) ||
            (yakumono_dobj->user_data.s >= nMPYakumonoStatusOff))
        {
            continue;
        }
        if (g->reject_skip != 0u)
        {
            return FALSE;
        }
        if ((yakumono_dobj->anim_joint.event32 != NULL) ||
            (yakumono_dobj->user_data.s != nMPYakumonoStatusNone))
        {
            const s32 ie = ndsMPWallSweepEdgeTrunc(
                g->yakumono_id, 0u, yakumono_dobj->translate.vec.f.x);
            const s32 id =
                ((is_diff != FALSE) && (gMPCollisionSpeeds != NULL)) ?
                ndsMPWallSweepEdgeTrunc(
                    g->yakumono_id, 1u,
                    gMPCollisionSpeeds[g->yakumono_id].x) : 0;
            s32 dp;
            s32 dt;
            s32 dlo;
            s32 dhi;

            if ((ie == INT32_MIN) || (id == INT32_MIN))
            {
                return FALSE;
            }
            dp = ip - ie + id;
            dt = it - ie;
            dlo = ((dp < dt) ? dp : dt) - NDS_MP_WALL_SWEEP_SLACK;
            dhi = ((dp > dt) ? dp : dt) + NDS_MP_WALL_SWEEP_SLACK;
            if ((dhi >= (s32)g->ext_lo) && (dlo <= (s32)g->ext_hi) &&
                (ndsMPGroupSpansMiss(g, dlo, dhi) == FALSE))
            {
                return FALSE;
            }
        }
        else if ((hi >= (s32)g->ext_lo) && (lo <= (s32)g->ext_hi) &&
                 (ndsMPGroupSpansMiss(g, lo, hi) == FALSE))
        {
            return FALSE;
        }
        g->reject_misses = 0u;
        rejected++;
    }
    NDS_DIAG(gNdsMPWallSweepCalls++);
    NDS_DIAG(gNdsMPWallSweepGroupRejects += rejected);
    NDS_DIAG(gNdsMPWallSweepStaticMisses++);
    return TRUE;
}

/* An object's whole wall test in one integer reject (P2-2p8, 2026-10-06;
 * owner: "Replace ALL soft float with fixed point"). The source's
 * mpProcessCheckTest{L,R}WallCollision (mpprocess.c:467, 691) sweeps five
 * segments whose x ends are the previous and the current position and those
 * plus or minus the collision box's half width -- twenty float adds to build
 * them, ~200 a frame on Peach's Castle -- and nearly every sweep is answered
 * by ndsMPWallSweepStaticMiss. This asks that question once for an x range
 * holding all five: both positions truncated, widened by the larger box
 * width's truncation plus 3 (a truncated rounded sum lies within 2 of the
 * sum of the truncations, and one more covers the rounding at the edge).
 * A range that misses every group's extent and spans contains five ranges
 * that miss them too, so TRUE means each sweep would have returned FALSE
 * having written only the reject bookkeeping written here; a dynamic group is
 * widened by its speed as well, which covers both the Diff and the Same form.
 * Anything the static miss would not answer goes to the source test. */
sb32 ndsMPProcessWallTestMiss(const MPCollData *coll_data, u32 line_kind)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    const NDSMPKindGroup *end;
    NDSMPKindGroup *g;
    s32 ipp;
    s32 it;
    s32 iw;
    s32 ipw;
    s32 lo;
    s32 hi;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL) ||
        (coll_data->p_map_coll == NULL) ||
        (geometry != sNdsMPWallMissReadyGeometry) ||
        (sNdsMPWallMissReadyGen != gNdsTaskmanHeapGeneration) ||
        (ndsMPWallSweepTruncInline(coll_data->pos_prev.x, &ipp) == FALSE) ||
        (ndsMPWallSweepTruncInline(coll_data->p_translate->x, &it) ==
         FALSE) ||
        (ndsMPWallSweepTruncInline(coll_data->map_coll.width, &iw) ==
         FALSE) ||
        (ndsMPWallSweepTruncInline(coll_data->p_map_coll->width, &ipw) ==
         FALSE))
    {
        return FALSE;
    }
    iw = (iw < 0) ? -iw : iw;
    ipw = (ipw < 0) ? -ipw : ipw;
    iw = ((iw > ipw) ? iw : ipw) + 3;
    lo = ((ipp < it) ? ipp : it) - iw;
    hi = ((ipp > it) ? ipp : it) + iw;
    /* Every segment end must truncate in range, as each sweep requires. */
    if ((lo <= -(1 << 20)) || (hi >= (1 << 20)))
    {
        return FALSE;
    }
    lo -= NDS_MP_WALL_SWEEP_SLACK;
    hi += NDS_MP_WALL_SWEEP_SLACK;
    g = sNdsMPKindGroups[line_kind];
    end = g + sNdsMPKindGroupCounts[line_kind];
    for (; g < end; g++)
    {
        const DObj *yakumono_dobj;

        if ((g->count == 0u) ||
            (g->yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
        {
            continue;
        }
        yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[g->yakumono_id];
        if ((yakumono_dobj == NULL) ||
            (yakumono_dobj->user_data.s >= nMPYakumonoStatusOff))
        {
            continue;
        }
        if (g->reject_skip != 0u)
        {
            return FALSE;
        }
        if ((yakumono_dobj->anim_joint.event32 != NULL) ||
            (yakumono_dobj->user_data.s != nMPYakumonoStatusNone))
        {
            const s32 ie = ndsMPWallSweepEdgeTrunc(
                g->yakumono_id, 0u, yakumono_dobj->translate.vec.f.x);
            s32 id = (gMPCollisionSpeeds != NULL) ?
                ndsMPWallSweepEdgeTrunc(g->yakumono_id, 1u,
                                        gMPCollisionSpeeds[g->yakumono_id].x) :
                0;

            if ((ie == INT32_MIN) || (id == INT32_MIN))
            {
                return FALSE;
            }
            id = (id < 0) ? -id : id;
            if ((hi - ie + id >= (s32)g->ext_lo) &&
                (lo - ie - id <= (s32)g->ext_hi) &&
                (ndsMPGroupSpansMiss(g, lo - ie - id, hi - ie + id) == FALSE))
            {
                return FALSE;
            }
        }
        else if ((hi >= (s32)g->ext_lo) && (lo <= (s32)g->ext_hi) &&
                 (ndsMPGroupSpansMiss(g, lo, hi) == FALSE))
        {
            return FALSE;
        }
        g->reject_misses = 0u;
    }
    NDS_DIAG(gNdsMPWallSweepStaticMisses++);
    return TRUE;
}

sb32 mpCollisionCheckLWallLineCollisionSame(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle)
{
    sb32 hit;

    hit = (ndsMPWallSweepStaticMiss(position, translate,
                                    nMPLineKindLWall, FALSE) != FALSE) ? FALSE :
        ndsStageMPAdjustFloorLoopWallSweep(position, translate, ga_last,
                                           stand_line_id, stand_coll_flags,
                                           angle, nMPLineKindLWall, FALSE);
    return hit;
}

sb32 mpCollisionCheckRWallLineCollisionSame(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle)
{
    sb32 hit;

    hit = (ndsMPWallSweepStaticMiss(position, translate,
                                    nMPLineKindRWall, FALSE) != FALSE) ? FALSE :
        ndsStageMPAdjustFloorLoopWallSweep(position, translate, ga_last,
                                           stand_line_id, stand_coll_flags,
                                           angle, nMPLineKindRWall, FALSE);
    return hit;
}

sb32 mpCollisionCheckLWallLineCollisionDiff(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle)
{
    if (ndsMPWallSweepStaticMiss(position, translate, nMPLineKindLWall,
                                 TRUE) != FALSE)
    {
        return FALSE;
    }
    return ndsStageMPAdjustFloorLoopWallSweep(position, translate, ga_last,
                                              stand_line_id,
                                              stand_coll_flags, angle,
                                              nMPLineKindLWall, TRUE);
}

sb32 mpCollisionCheckRWallLineCollisionDiff(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle)
{
    if (ndsMPWallSweepStaticMiss(position, translate, nMPLineKindRWall,
                                 TRUE) != FALSE)
    {
        return FALSE;
    }
    return ndsStageMPAdjustFloorLoopWallSweep(position, translate, ga_last,
                                              stand_line_id,
                                              stand_coll_flags, angle,
                                              nMPLineKindRWall, TRUE);
}

/* mpcollision.c:315 mpCollisionGetLRCommon. The earlier form interpolated
 * between a wall's first and last vertex and always answered (+-1, 0); a
 * multi-segment wall and a slanted one both need the segment that spans the
 * object's y, and the source's 0.001 tolerance at the two ends. */
static sb32 ndsMPGetLRCommonWall(s32 line_id, Vec3f *object_pos,
                                 f32 *dist, u32 *flags, Vec3f *angle,
                                 u32 line_kind)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    Vec3f object_local;
    u32 vertex_first;
    u32 vertex_count;
    u32 vertex;
    u32 segment;
    s32 v1x;
    s32 v1y;
    s32 v2x;
    s32 v2y;
    f32 vnear;
    f32 vfar;
    f32 object_y;
    f32 wall_x;

    if ((object_pos == NULL) ||
        (ndsMPGetLineKindForLineID(line_id) != (s32)line_kind) ||
        (ndsMPLinePositionWorldToLocal(line_id, object_pos, &object_local) ==
         FALSE))
    {
        return FALSE;
    }
    vertex_first = ndsMPVertexLinkFirst(geometry->vertex_links, (u32)line_id);
    vertex_count = ndsMPVertexLinkCount(geometry->vertex_links, (u32)line_id);
    if ((vertex_count < 2u) || (vertex_count > 128u))
    {
        NDS_DIAG(gNdsStageCollisionLoopBadVertexCount++);
        return FALSE;
    }
    v1y = ndsMPVertexY(geometry->vertex_data,
                       ndsMPVertexID(geometry->vertex_id, vertex_first));
    v2y = ndsMPVertexY(geometry->vertex_data,
                       ndsMPVertexID(geometry->vertex_id,
                                     vertex_first + vertex_count - 1u));
    if (v2y < v1y)
    {
        vfar = (f32)v1y;
        vnear = (f32)v2y;
    }
    else
    {
        vfar = (f32)v2y;
        vnear = (f32)v1y;
    }
    object_y = object_local.y;
    if ((object_y <= (vnear - 0.001F)) || ((vfar + 0.001F) <= object_y))
    {
        return FALSE;
    }
    if (object_y <= vnear)
    {
        object_y = vnear;
    }
    else if (vfar <= object_y)
    {
        object_y = vfar;
    }
    segment = vertex_first;
    if (vertex_count != 2u)
    {
        for (vertex = vertex_first;
             vertex < (vertex_first + vertex_count - 1u); vertex++)
        {
            s32 y1 = ndsMPVertexY(geometry->vertex_data,
                                  ndsMPVertexID(geometry->vertex_id, vertex));
            s32 y2 = ndsMPVertexY(geometry->vertex_data,
                                  ndsMPVertexID(geometry->vertex_id,
                                                vertex + 1u));

            segment = vertex;
            if ((((f32)y1 <= object_y) && (object_y <= (f32)y2)) ||
                (((f32)y2 <= object_y) && (object_y <= (f32)y1)))
            {
                break;
            }
        }
    }
    v1x = ndsMPVertexX(geometry->vertex_data,
                       ndsMPVertexID(geometry->vertex_id, segment));
    v1y = ndsMPVertexY(geometry->vertex_data,
                       ndsMPVertexID(geometry->vertex_id, segment));
    v2x = ndsMPVertexX(geometry->vertex_data,
                       ndsMPVertexID(geometry->vertex_id, segment + 1u));
    v2y = ndsMPVertexY(geometry->vertex_data,
                       ndsMPVertexID(geometry->vertex_id, segment + 1u));
    if (flags != NULL)
    {
        *flags = ndsMPVertexFlags(geometry->vertex_data,
                                  ndsMPVertexID(geometry->vertex_id, segment));
    }
    if (v2y == v1y)
    {
        NDS_DIAG(gNdsStageCollisionLoopDivisionGuardCount++);
        wall_x = ((f32)v1x + (f32)v2x) * 0.5F;
    }
    else
    {
        /* mpCollisionGetLineDistanceLR */
        wall_x = (f32)v1x + (((object_y - (f32)v1y) /
            ((f32)v2y - (f32)v1y)) * ((f32)v2x - (f32)v1x));
    }
    if (dist != NULL)
    {
        *dist = wall_x - object_local.x;
    }
    if (angle != NULL)
    {
        ndsMPGetLRAngle(angle, v1x, v1y, v2x, v2y,
                        (line_kind == nMPLineKindLWall) ? -1 : +1);
    }
    return TRUE;
}

sb32 mpCollisionGetLRCommonLWall(s32 line_id, Vec3f *object_pos,
                                 f32 *dist, u32 *flags, Vec3f *angle)
{
    return ndsMPGetLRCommonWall(line_id, object_pos, dist, flags, angle,
                                nMPLineKindLWall);
}

sb32 mpCollisionGetLRCommonRWall(s32 line_id, Vec3f *object_pos,
                                 f32 *dist, u32 *flags, Vec3f *angle)
{
    return ndsMPGetLRCommonWall(line_id, object_pos, dist, flags, angle,
                                nMPLineKindRWall);
}

static void ndsMPGetWallEdgeU(s32 line_id, Vec3f *object_pos)
{
    Vec3f a;
    Vec3f b;

    if (object_pos == NULL)
    {
        return;
    }
    if (ndsMPFindLineEndpoints(line_id, &a, &b, NULL, NULL) == FALSE)
    {
        object_pos->x = 0.0F;
        object_pos->y = 0.0F;
        object_pos->z = 0.0F;
        return;
    }
    *object_pos = (a.y >= b.y) ? a : b;
    if (ndsMPLinePositionLocalToWorld(line_id, object_pos) == FALSE)
    {
        object_pos->x = 0.0F;
        object_pos->y = 0.0F;
        object_pos->z = 0.0F;
    }
}

void mpCollisionGetLWallEdgeU(s32 line_id, Vec3f *object_pos)
{
    ndsMPGetWallEdgeU(line_id, object_pos);
}

void mpCollisionGetRWallEdgeU(s32 line_id, Vec3f *object_pos)
{
    ndsMPGetWallEdgeU(line_id, object_pos);
}

static sb32 ndsMPProjectFloorGeometry(Vec3f *position, s32 *project_line_id,
                                      f32 *ga_dist, u32 *stand_coll_flags,
                                      Vec3f *angle)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    MPVertexLinks *links;
    MPVertexArray *ids;
    MPVertexPosContainer *verts;
    f32 line_project_pos = 3.402823466e+38F;
    s32 best_line = -1;
    f32 best_dist = 0.0F;
    u32 best_flags = 0u;
    Vec3f best_angle = { 0.0F, 1.0F, 0.0F };
    u32 yakumono_count;
    u32 i;

    if ((position == NULL) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    line_info = geometry->line_info;
    links = geometry->vertex_links;
    ids = geometry->vertex_id;
    verts = geometry->vertex_data;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    /* mpcollision.c:2651-2749, group for group. A group whose platform is
     * switched off (a Yoshi's Island cloud that evaporated) has no floor, and
     * a moving one (the clouds, which grYosterUpdateCloudSolid places every
     * update) is queried in its own frame: the point less the platform's
     * translation, as every other line query here does. This walk read every
     * group's vertices as world positions, so at spawn Yoshi's Island's cloud
     * line (local vertices about its own origin) projected under player 1's
     * entry point: the appear restored that line as the fighter's floor and
     * the first grounded update walked him onto the far cloud, which
     * evaporated under him during the countdown. */
    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);
        s32 line_first =
            (s32)ndsMPLineInfoGroupID(info, nMPLineKindFloor);
        s32 line_count =
            (s32)ndsMPLineInfoLineCount(info, nMPLineKindFloor);
        s32 line_id;
        s32 line_end;
        u32 yakumono_id = ndsMPLineInfoYakumonoID(info);
        const DObj *yakumono_dobj;
        f32 vpdist_x = position->x;
        f32 vpdist_y = position->y;
        s32 px_floor;
        s32 px_ceil;
        sb32 px_int;

        if ((line_count == 0) || (gMPCollisionYakumonoDObjs == NULL) ||
            (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
        {
            continue;
        }
        yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
        if ((yakumono_dobj == NULL) ||
            (yakumono_dobj->user_data.s >= nMPYakumonoStatusOff))
        {
            continue;
        }
        if ((yakumono_dobj->anim_joint.event32 != NULL) ||
            (yakumono_dobj->user_data.s != nMPYakumonoStatusNone))
        {
            vpdist_x = position->x - yakumono_dobj->translate.vec.f.x;
            vpdist_y = position->y - yakumono_dobj->translate.vec.f.y;
        }
        line_end = line_first + line_count;
        if ((line_end - line_first) > 4096)
        {
            line_end = line_first + 4096;
        }
        /* P2-2p8 (2026-10-05, owner: fixed point only): the bracket tests
         * `(f32)x <= px` and `(f32)x >= px` on integer vertices are exactly
         * `x <= floor(px)` and `x >= ceil(px)`, so they run on two integers
         * taken once per group from px's bits (an i2f and a float compare per
         * test before). A px the bits cannot bound (huge, inf, NaN) keeps the
         * float tests. */
        {
            u32 frac;

            px_int = ndsMPF32TruncFrac(vpdist_x, &px_floor, &frac);
            px_ceil = px_floor;
            if ((px_int != FALSE) && (frac != 0u))
            {
                if (NDS_MP_F32_POSITIVE(vpdist_x))
                {
                    px_ceil = px_floor + 1;
                }
                else
                {
                    px_floor = px_floor - 1;
                }
            }
        }
#define NDS_MP_BRACKETS(xa, xb)                                          \
    ((px_int != FALSE) ?                                                 \
        ((((xa) <= px_floor) && ((xb) >= px_ceil)) ||                    \
         (((xb) <= px_floor) && ((xa) >= px_ceil))) :                    \
        ((((f32)(xa) <= vpdist_x) && ((f32)(xb) >= vpdist_x)) ||         \
         (((f32)(xb) <= vpdist_x) && ((f32)(xa) >= vpdist_x))))
        for (line_id = line_first; line_id < line_end; line_id++)
        {
            u32 vertex_first = ndsMPVertexLinkFirst(links, (u32)line_id);
            u32 vertex_count = ndsMPVertexLinkCount(links, (u32)line_id);
            u32 segment_count;
            u32 j;
            s32 x_first;
            s32 x_last;

            if ((vertex_count < 2u) || (vertex_count > 128u))
            {
                NDS_DIAG(gNdsStageCollisionLoopBadVertexCount++);
                continue;
            }
            segment_count = vertex_count - 1u;
            x_first = ndsMPVertexX(verts, ndsMPVertexID(ids, vertex_first));
            x_last = ndsMPVertexX(verts,
                                  ndsMPVertexID(ids,
                                                vertex_first + segment_count));
            if (!NDS_MP_BRACKETS(x_first, x_last))
            {
                continue;
            }
            for (j = 0u; j < segment_count; j++)
            {
                u32 v1_index = vertex_first + j;
                u32 v2_index = v1_index + 1u;
                u32 v1_id = ndsMPVertexID(ids, v1_index);
                u32 v2_id = ndsMPVertexID(ids, v2_index);
                s32 x1 = ndsMPVertexX(verts, v1_id);
                s32 y1 = ndsMPVertexY(verts, v1_id);
                s32 x2 = ndsMPVertexX(verts, v2_id);
                s32 y2 = ndsMPVertexY(verts, v2_id);
                f32 fpos;
                f32 gdist;

                if (!NDS_MP_BRACKETS(x1, x2))
                {
                    continue;
                }
                /* The source takes the first segment that brackets x; a
                 * vertical one interpolates 0/0, and a NaN never compares
                 * below the point, so that line offers no floor. */
                if (x1 == x2)
                {
                    NDS_DIAG(gNdsStageCollisionLoopDivisionGuardCount++);
                    break;
                }
                fpos = ndsMPLineDistanceFC(vpdist_x, x1, y1, x2, y2);
                gdist = fpos - vpdist_y;
                if ((fpos <= vpdist_y) &&
                    (fabsf(gdist) < line_project_pos))
                {
                    best_line = line_id;
                    best_dist = gdist;
                    best_flags = ndsMPVertexFlags(verts, v1_id);
                    ndsMPGetFCAngle(&best_angle, x1, y1, x2, y2, +1);
                    line_project_pos = fabsf(gdist);
                }
                break;
            }
        }
#undef NDS_MP_BRACKETS
    }
    if (best_line < 0)
    {
        return FALSE;
    }
    if (ndsMPLineIDIsFloor(best_line) == FALSE)
    {
        gNdsStageCollisionLoopNonFloorCandidateCount++;
        return FALSE;
    }
    if (project_line_id != NULL)
    {
        *project_line_id = best_line;
    }
    if (ga_dist != NULL)
    {
        *ga_dist = best_dist;
    }
    if (stand_coll_flags != NULL)
    {
        *stand_coll_flags = best_flags;
    }
    if (angle != NULL)
    {
        *angle = best_angle;
    }
    return TRUE;
}

static void ndsStageCollisionLoopRecordProject(Vec3f *position, sb32 hit,
                                               s32 line_id, f32 dist,
                                               u32 flags, Vec3f *angle,
                                               sb32 used_geometry)
{
    u32 slot;
    u32 best_slot = 2u;
    f32 best_delta = 1000000.0F;

    (void)line_id;
    (void)dist;
    (void)flags;
    (void)angle;
    if (gNdsStageCollisionLoopPrepared == 0u)
    {
        return;
    }
    NDS_DIAG(gNdsStageCollisionLoopProjectCallCount++);
    if (used_geometry != FALSE)
    {
        gNdsStageCollisionLoopGeometryProjectCallCount++;
    }
    if (position == NULL)
    {
        return;
    }
    for (slot = 0u; slot < 2u; slot++)
    {
        FTStruct *fp = &sNdsFighterStructPool[slot];
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) != FALSE) &&
            (root != NULL))
        {
            f32 dx = fabsf(position->x - root->translate.vec.f.x);
            f32 dy = fabsf(position->y - root->translate.vec.f.y);
            f32 delta = dx + dy;

            if (delta < best_delta)
            {
                best_delta = delta;
                best_slot = slot;
            }
        }
    }
    if ((best_slot == 0u) && (best_delta <= 256.0F))
    {
        NDS_DIAG(gNdsStageCollisionLoopP0ProjectCount++);
        if (hit != FALSE)
        {
            gNdsStageCollisionLoopP0HitCount++;
        }
    }
    else if ((best_slot == 1u) && (best_delta <= 256.0F))
    {
        NDS_DIAG(gNdsStageCollisionLoopP1ProjectCount++);
        if (hit != FALSE)
        {
            NDS_DIAG(gNdsStageCollisionLoopP1HitCount++);
        }
    }
}

static sb32 ndsStageCollisionLoopCheckProjectFloor(
    Vec3f *pos, s32 *floor_line_id, f32 *floor_dist, u32 *floor_flags,
    Vec3f *floor_angle, sb32 *hit)
{
    s32 line_id = -1;
    f32 dist = 0.0F;
    u32 flags = 0u;
    Vec3f angle = { 0.0F, 1.0F, 0.0F };
    sb32 is_hit = FALSE;

    if (hit != NULL)
    {
        *hit = FALSE;
    }
    if (gNdsStageCollisionLoopPrepared == 0u)
    {
        return FALSE;
    }
    if (ndsStageCollisionLoopGeometryReady() == FALSE)
    {
        NDS_DIAG(gNdsStageCollisionLoopNoGeometryCount++);
        gNdsStageCollisionLoopUnsafeFallbackAfterPrepareCount++;
        if (floor_line_id != NULL)
        {
            *floor_line_id = -1;
        }
        if (floor_dist != NULL)
        {
            *floor_dist = 0.0F;
        }
        if (floor_flags != NULL)
        {
            *floor_flags = 0u;
        }
        if (floor_angle != NULL)
        {
            *floor_angle = angle;
        }
        return TRUE;
    }
    is_hit = ndsMPProjectFloorGeometry(pos, &line_id, &dist, &flags, &angle);
    ndsStageCollisionLoopRecordProject(pos, is_hit, line_id, dist, flags,
                                       &angle, TRUE);
    if (floor_line_id != NULL)
    {
        *floor_line_id = is_hit ? line_id : -1;
    }
    if (floor_dist != NULL)
    {
        *floor_dist = is_hit ? dist : 0.0F;
    }
    if (floor_flags != NULL)
    {
        *floor_flags = is_hit ? flags : 0u;
    }
    if (floor_angle != NULL)
    {
        *floor_angle = angle;
    }
    if (hit != NULL)
    {
        *hit = is_hit;
    }
    return TRUE;
}

void ndsStageCollisionLoopPrepareRuntime(void)
{
    if (gNdsStageCollisionLoopPrepared == 0u)
    {
        gNdsStageCollisionLoopPrepared = 1u;
    }
    ndsStageCollisionLoopCountLines();
}

static sb32 ndsStageFloorFollowLoopUpdateFighter(GObj *fighter_gobj)
{
    return FALSE;
}

static sb32 ndsStageMPSweepFloorLoopSweep(Vec3f *position,
                                          Vec3f *translate,
                                          Vec3f *ga_last,
                                          s32 *stand_line_id,
                                          u32 *stand_coll_flags,
                                          Vec3f *angle,
                                          sb32 is_diff)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    MPVertexLinks *links;
    MPVertexArray *ids;
    MPVertexPosContainer *verts;
    u32 yakumono_count;
    u32 i;
    f32 best_dist = 3.402823466e+38F;
    NDSMPKindGroup *floor_groups = NULL;
    s32 best_line = -1;
    u32 best_flags = 0u;
    sb32 best_is_reverse_endpoint = FALSE;
    sb32 best_is_flat_ascending = FALSE;
    sb32 saw_flat_ascending_sweep = FALSE;
    Vec3f best_pos = { 0.0F, 0.0F, 0.0F };
    Vec3f best_angle = { 0.0F, 1.0F, 0.0F };
    u32 reject_state = 0u;
    s32 ipy = 0;
    s32 ity = 0;
    u64 candidates = ~(u64)0;
    u32 window_state = 0u;
    s32 window_lo = 0;
    s32 window_hi = 0;

    if ((position == NULL) || (translate == NULL) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    NDS_DIAG(gNdsCollisionRuntimeDiagnostics.floor_sweep_calls++);
    line_info = geometry->line_info;
    links = geometry->vertex_links;
    ids = geometry->vertex_id;
    verts = geometry->vertex_data;
    /* Both sweeps read the cycle-109 vertex memo -- the floor one directly, the
     * ceiling one through the cycle-117 extent cache -- and NEITHER of them
     * bound it. Only the three point queries did. The memo survived that
     * because `gMPCollisionGeometry` is saved and restored around the
     * alternate-geometry queries and a point query almost always rebound it
     * first, so a stale read needed a specific ordering. The extent cache is
     * keyed by LINE ID rather than vertex id, so the same staleness becomes a
     * WRONG whole-line reject -- which is how this was found: four presented
     * frames of a 1600-frame gate run diverged, deterministically and in the
     * same four places on two consecutive runs. */
    ndsMPVertexF32Bind(geometry);
    if (sNdsMPKindGroupGeometry == geometry)
    {
        floor_groups = sNdsMPKindGroups[nMPLineKindFloor];
        yakumono_count = sNdsMPKindGroupCounts[nMPLineKindFloor];
        candidates = ndsMPSweepCandidates(nMPLineKindFloor, 1u, position,
                                          translate, is_diff);
    }
    else
    {
        yakumono_count = ndsMPGeometryYakumonoCount(geometry);
        if (yakumono_count > 64u)
        {
            yakumono_count = 64u;
        }
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        s32 first;
        s32 count;
        u32 yakumono_id;
        DObj *yakumono_dobj = NULL;
        f32 vedge_x = 0.0F;
        f32 vedge_y = 0.0F;
        f32 speed_x = 0.0F;
        f32 speed_y = 0.0F;
        sb32 is_dynamic = FALSE;
        s32 end;
        s32 line_id;
        Vec3f group_position;
        Vec3f group_translate;

        if (floor_groups != NULL)
        {
            if (((candidates >> i) & 1u) == 0u)
            {
                continue;
            }
            first = (s32)floor_groups[i].first;
            count = (s32)floor_groups[i].count;
            yakumono_id = floor_groups[i].yakumono_id;
        }
        else
        {
            NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);

            first = (s32)ndsMPLineInfoGroupID(info, nMPLineKindFloor);
            count = (s32)ndsMPLineInfoLineCount(info, nMPLineKindFloor);
            yakumono_id = ndsMPLineInfoYakumonoID(info);
        }
        if (count <= 0)
        {
            continue;
        }
        if (count > 4096)
        {
            count = 4096;
        }
        if ((gMPCollisionYakumonoDObjs != NULL) &&
            (yakumono_id < NDS_MP_YAKUMONO_DOBJ_SLOTS))
        {
            yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
        }
        if ((yakumono_dobj != NULL) &&
            (yakumono_dobj->user_data.s >= nMPYakumonoStatusOff))
        {
            continue;
        }
        if ((yakumono_dobj != NULL) &&
            ((yakumono_dobj->anim_joint.event32 != NULL) ||
             (yakumono_dobj->user_data.s != nMPYakumonoStatusNone)))
        {
            vedge_x = yakumono_dobj->translate.vec.f.x;
            vedge_y = yakumono_dobj->translate.vec.f.y;
            if ((is_diff != FALSE) && (gMPCollisionSpeeds != NULL))
            {
                speed_x = gMPCollisionSpeeds[yakumono_id].x;
                speed_y = gMPCollisionSpeeds[yakumono_id].y;
            }
            is_dynamic = TRUE;
        }
        if ((reject_state != 2u) && (floor_groups != NULL) &&
            (ndsMPSweepGroupReject(&reject_state, &ipy, &ity, 1u, position,
                                    translate, &floor_groups[i], yakumono_id,
                                    is_dynamic, vedge_y, speed_y,
                                    ((is_diff != FALSE) &&
                                     (gMPCollisionSpeeds != NULL)) ?
                                        TRUE : FALSE) != FALSE))
        {
            continue;
        }
        end = first + count;
        /* The dynamic offsets are the group's, not the line's: computed once
         * here (the same operations on the same operands), copied per line. */
        group_position = *position;
        group_translate = *translate;
        if (is_dynamic != FALSE)
        {
            group_position.x = (position->x - vedge_x) + speed_x;
            group_position.y = (position->y - vedge_y) + speed_y;
            group_translate.x = translate->x - vedge_x;
            group_translate.y = translate->y - vedge_y;
        }
        window_state = 0u;
        /* The sweep's bounds and their compare keys, once per group: neither
         * point changes within it (2026-10-19; they were re-keyed for every
         * line, ~27% of this function on Hyrule Temple). The flat-ascending
         * test's float compares become key compares: the keys order as the
         * floats do (NaN excluded, as everywhere in nds_fcmp.h), and its
         * min/max pairs pick the lesser and greater key. */
        const u32 key_px = ndsFcmpKey(group_position.x);
        const u32 key_tx = ndsFcmpKey(group_translate.x);
        const u32 key_py = ndsFcmpKey(group_position.y);
        const u32 key_ty = ndsFcmpKey(group_translate.y);
        const u32 key_min_x = (key_px < key_tx) ? key_px : key_tx;
        const u32 key_max_x = (key_px < key_tx) ? key_tx : key_px;
        const u32 key_min_y = (key_py < key_ty) ? key_py : key_ty;
        const u32 key_max_y = (key_py < key_ty) ? key_ty : key_py;
        for (line_id = first; line_id < end; line_id++)
        {
            u32 vertex_first = ndsMPVertexLinkFirst(links, (u32)line_id);
            u32 vertex_count = ndsMPVertexLinkCount(links, (u32)line_id);
            const Vec3f *sweep_position = &group_position;
            const Vec3f *sweep_translate = &group_translate;
            u32 j;
            u32 j_first;
            u32 j_last;

            NDS_DIAG(gNdsStageMPSweepFloorLoopLineSweepVisitCount++);
            if ((vertex_count < 2u) || (vertex_count > 128u))
            {
                NDS_DIAG(gNdsStageCollisionLoopBadVertexCount++);
                continue;
            }
            if (ndsMPLineExtentSweepRejectsKeys(ids, verts, (u32)line_id,
                                                vertex_first, vertex_count,
                                                key_min_y, key_max_y) != 0)
            {
                continue;
            }
            if (ndsMPSweepSegmentRun(ids, verts, line_id, vertex_first,
                                     vertex_count, sweep_position,
                                     sweep_translate, +1, &window_state,
                                     &window_lo, &window_hi, &j_first,
                                     &j_last) == FALSE)
            {
                continue;
            }
            for (j = j_first; (j <= j_last) && (j + 1u < vertex_count); j++)
            {
                u32 v1_id = ndsMPVertexID(ids, vertex_first + j);
                u32 v2_id = ndsMPVertexID(ids, vertex_first + j + 1u);
                Vec3f v1 = { 0.0F, 0.0F, 0.0F };
                Vec3f v2 = { 0.0F, 0.0F, 0.0F };
                f32 hit_x;
                f32 hit_y;
                f32 hit_dist;
                sb32 is_flat_ascending = FALSE;

                ndsMPVertexF32Get(verts, v1_id, &v1.x, &v1.y);
                ndsMPVertexF32Get(verts, v2_id, &v2.x, &v2.y);

                const sb32 seg_flat =
                    (ndsFcmpBits(v1.y) == ndsFcmpBits(v2.y)) ? TRUE : FALSE;

                if (seg_flat != FALSE)
                {
                    const u32 key_v1y = ndsFcmpKey(v1.y);
                    const u32 key_v1x = ndsFcmpKey(v1.x);
                    const u32 key_v2x = ndsFcmpKey(v2.x);

                    if ((key_ty > key_py) && (key_py <= key_v1y) &&
                        (key_ty >= key_v1y) &&
                        (key_min_x <= ((key_v1x > key_v2x) ? key_v1x : key_v2x)) &&
                        (key_max_x >= ((key_v1x < key_v2x) ? key_v1x : key_v2x)))
                    {
                        is_flat_ascending = TRUE;
                        saw_flat_ascending_sweep = TRUE;
                    }
                }
                if ((seg_flat == FALSE) &&
                    (ndsMPSweepSegmentXMissesKeys(&v1, &v2, key_min_x,
                                                  key_max_x) != FALSE))
                {
                    NDS_DIAG(gNdsMPSweepSegmentXRejects++);
                    continue;
                }

                if (ndsMPFCSegmentCrosses(
                        sweep_position, sweep_translate, &v1, &v2, +1,
                        &hit_x, &hit_y) == FALSE)
                {
                    continue;
                }
                NDS_DIAG(gNdsStageMPSweepFloorLoopLineSweepCandidateCount++);
                hit_dist = fabsf(hit_y - (sweep_position->y - speed_y));
                if (hit_dist >= best_dist)
                {
                    continue;
                }
                best_dist = hit_dist;
                best_line = line_id;
                best_flags = ndsMPVertexFlags(verts, v1_id);
                best_is_reverse_endpoint = (v2.x < v1.x) ? TRUE : FALSE;
                best_is_flat_ascending = is_flat_ascending;
                best_pos.x = hit_x + vedge_x;
                best_pos.y = hit_y + vedge_y;
                best_pos.z = 0.0F;
                ndsMPGetFCAngle(&best_angle, (s32)v1.x, (s32)v1.y,
                                (s32)v2.x, (s32)v2.y, 1);
            }
        }
    }
    if (saw_flat_ascending_sweep != FALSE)
    {
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.floor_flat_ascending_sweeps++);
    }
    if (best_line < 0)
    {
        return FALSE;
    }
    NDS_DIAG(gNdsCollisionRuntimeDiagnostics.floor_sweep_hits++);
    if (best_is_reverse_endpoint != FALSE)
    {
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.floor_reverse_endpoint_hits++);
    }
    if (best_is_flat_ascending != FALSE)
    {
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.floor_flat_ascending_accepts++);
    }
    if (ga_last != NULL)
    {
        *ga_last = best_pos;
    }
    if (stand_line_id != NULL)
    {
        *stand_line_id = best_line;
    }
    if (stand_coll_flags != NULL)
    {
        *stand_coll_flags = best_flags;
    }
    if (angle != NULL)
    {
        *angle = best_angle;
    }
    return TRUE;
}

sb32 mpCollisionCheckFloorLineCollisionSame(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle)
{
    sb32 hit;

    hit = ndsStageMPSweepFloorLoopSweep(position, translate, ga_last,
                                        stand_line_id, stand_coll_flags,
                                        angle, FALSE);
    return hit;
}

sb32 mpCollisionCheckFloorLineCollisionDiff(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle)
{
    sb32 hit;

    hit = ndsStageMPSweepFloorLoopSweep(position, translate, ga_last,
                                        stand_line_id, stand_coll_flags,
                                        angle, TRUE);
    return hit;
}

/* Mirror of ndsStageMPSweepFloorLoopSweep for nMPLineKindCeil, i.e. the source
 * mpCollisionCheckCeilLineCollisionSame/Diff pair.  A ceiling only collides when
 * the swept segment crosses it upward; an earlier revision accepted "the head is
 * within 8 (or 128) units of the ceiling plane" instead, which re-fired on every
 * frame the fighter spent under the stage and pinned him there because
 * mpProcessRunCeilCollisionAdjNew clamps translate->y back to the line each time
 * (BUGS.md: floating under the stage). */
static sb32 ndsStageMPCeilFloorLoopSweep(Vec3f *position,
                                         Vec3f *translate,
                                         Vec3f *ga_last,
                                         s32 *stand_line_id,
                                         u32 *stand_coll_flags,
                                         Vec3f *angle,
                                         sb32 is_diff)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    MPVertexLinks *links;
    MPVertexArray *ids;
    MPVertexPosContainer *verts;
    u32 yakumono_count;
    u32 i;
    f32 best_dist = 3.402823466e+38F;
    s32 best_line = -1;
    u32 best_flags = 0u;
    Vec3f best_pos = { 0.0F, 0.0F, 0.0F };
    Vec3f best_angle = { 0.0F, -1.0F, 0.0F };
    u32 reject_state = 0u;
    s32 ipy = 0;
    s32 ity = 0;
    NDSMPKindGroup *ceil_group = NULL;
    u32 ceil_groups_left = 0u;
    u32 window_state = 0u;
    s32 window_lo = 0;
    s32 window_hi = 0;

    if ((position == NULL) || (translate == NULL) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    line_info = geometry->line_info;
    links = geometry->vertex_links;
    ids = geometry->vertex_id;
    verts = geometry->vertex_data;
    /* Both sweeps read the cycle-109 vertex memo -- the floor one directly, the
     * ceiling one through the cycle-117 extent cache -- and NEITHER of them
     * bound it. Only the three point queries did. The memo survived that
     * because `gMPCollisionGeometry` is saved and restored around the
     * alternate-geometry queries and a point query almost always rebound it
     * first, so a stale read needed a specific ordering. The extent cache is
     * keyed by LINE ID rather than vertex id, so the same staleness becomes a
     * WRONG whole-line reject -- which is how this was found: four presented
     * frames of a 1600-frame gate run diverged, deterministically and in the
     * same four places on two consecutive runs. */
    ndsMPVertexF32Bind(geometry);
    if (sNdsMPKindGroupGeometry == geometry)
    {
        /* The ceiling groups, one per yakumono with ceiling lines, in the
         * order this loop meets those yakumonos. */
        ceil_group = sNdsMPKindGroups[nMPLineKindCeil];
        ceil_groups_left = sNdsMPKindGroupCounts[nMPLineKindCeil];
    }
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);
        s32 first = (s32)ndsMPLineInfoGroupID(info, nMPLineKindCeil);
        s32 count = (s32)ndsMPLineInfoLineCount(info, nMPLineKindCeil);
        u32 yakumono_id = ndsMPLineInfoYakumonoID(info);
        DObj *yakumono_dobj = NULL;
        f32 vedge_x = 0.0F;
        f32 vedge_y = 0.0F;
        f32 speed_x = 0.0F;
        f32 speed_y = 0.0F;
        sb32 is_dynamic = FALSE;
        s32 end;
        s32 line_id;
        Vec3f group_position;
        Vec3f group_translate;
        NDSMPKindGroup *group = NULL;

        if (count <= 0)
        {
            continue;
        }
        if (ceil_groups_left != 0u)
        {
            if ((ceil_group->first == (u16)first) &&
                (ceil_group->yakumono_id == yakumono_id))
            {
                group = ceil_group;
            }
            ceil_group++;
            ceil_groups_left--;
        }
        if (count > 4096)
        {
            count = 4096;
        }
        if ((gMPCollisionYakumonoDObjs != NULL) &&
            (yakumono_id < NDS_MP_YAKUMONO_DOBJ_SLOTS))
        {
            yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
        }
        if ((yakumono_dobj != NULL) &&
            (yakumono_dobj->user_data.s >= nMPYakumonoStatusOff))
        {
            continue;
        }
        if ((yakumono_dobj != NULL) &&
            ((yakumono_dobj->anim_joint.event32 != NULL) ||
             (yakumono_dobj->user_data.s != nMPYakumonoStatusNone)))
        {
            vedge_x = yakumono_dobj->translate.vec.f.x;
            vedge_y = yakumono_dobj->translate.vec.f.y;
            if ((is_diff != FALSE) && (gMPCollisionSpeeds != NULL))
            {
                speed_x = gMPCollisionSpeeds[yakumono_id].x;
                speed_y = gMPCollisionSpeeds[yakumono_id].y;
            }
            is_dynamic = TRUE;
        }
        if ((reject_state != 2u) && (group != NULL) &&
            (ndsMPSweepGroupReject(&reject_state, &ipy, &ity, 1u, position,
                                    translate, group, yakumono_id,
                                    is_dynamic, vedge_y, speed_y,
                                    ((is_diff != FALSE) &&
                                     (gMPCollisionSpeeds != NULL)) ?
                                        TRUE : FALSE) != FALSE))
        {
            continue;
        }
        end = first + count;
        /* The dynamic offsets are the group's, not the line's: computed once
         * here (the same operations on the same operands), copied per line. */
        group_position = *position;
        group_translate = *translate;
        if (is_dynamic != FALSE)
        {
            group_position.x = (position->x - vedge_x) + speed_x;
            group_position.y = (position->y - vedge_y) + speed_y;
            group_translate.x = translate->x - vedge_x;
            group_translate.y = translate->y - vedge_y;
        }
        window_state = 0u;
        /* The sweep's compare keys once per group, as the floor sweep makes
         * them (2026-10-19). */
        const u32 key_px = ndsFcmpKey(group_position.x);
        const u32 key_tx = ndsFcmpKey(group_translate.x);
        const u32 key_py = ndsFcmpKey(group_position.y);
        const u32 key_ty = ndsFcmpKey(group_translate.y);
        const u32 key_min_x = (key_px < key_tx) ? key_px : key_tx;
        const u32 key_max_x = (key_px < key_tx) ? key_tx : key_px;
        const u32 key_min_y = (key_py < key_ty) ? key_py : key_ty;
        const u32 key_max_y = (key_py < key_ty) ? key_ty : key_py;
        for (line_id = first; line_id < end; line_id++)
        {
            u32 vertex_first = ndsMPVertexLinkFirst(links, (u32)line_id);
            u32 vertex_count = ndsMPVertexLinkCount(links, (u32)line_id);
            const Vec3f *sweep_position = &group_position;
            const Vec3f *sweep_translate = &group_translate;
            u32 j;
            u32 j_first;
            u32 j_last;

            if ((vertex_count < 2u) || (vertex_count > 128u))
            {
                NDS_DIAG(gNdsStageCollisionLoopBadVertexCount++);
                continue;
            }
            if (ndsMPLineExtentSweepRejectsKeys(ids, verts, (u32)line_id,
                                                vertex_first, vertex_count,
                                                key_min_y, key_max_y) != 0)
            {
                continue;
            }
            if (ndsMPSweepSegmentRun(ids, verts, line_id, vertex_first,
                                     vertex_count, sweep_position,
                                     sweep_translate, -1, &window_state,
                                     &window_lo, &window_hi, &j_first,
                                     &j_last) == FALSE)
            {
                continue;
            }
            for (j = j_first; (j <= j_last) && (j + 1u < vertex_count); j++)
            {
                u32 v1_id = ndsMPVertexID(ids, vertex_first + j);
                u32 v2_id = ndsMPVertexID(ids, vertex_first + j + 1u);
                Vec3f v1 = { 0.0F, 0.0F, 0.0F };
                Vec3f v2 = { 0.0F, 0.0F, 0.0F };
                f32 hit_x;
                f32 hit_y;
                f32 hit_dist;

                ndsMPVertexF32Get(verts, v1_id, &v1.x, &v1.y);
                ndsMPVertexF32Get(verts, v2_id, &v2.x, &v2.y);
                NDS_DIAG(gNdsStageMPCeilFloorLoopLineSweepVisitCount++);
                if ((ndsFcmpBits(v1.y) != ndsFcmpBits(v2.y)) &&
                    (ndsMPSweepSegmentXMissesKeys(&v1, &v2, key_min_x,
                                                  key_max_x) != FALSE))
                {
                    NDS_DIAG(gNdsMPSweepSegmentXRejects++);
                    continue;
                }
                if (ndsMPFCSegmentCrosses(sweep_position, sweep_translate,
                        &v1, &v2, -1, &hit_x, &hit_y) == FALSE)
                {
                    continue;
                }
                NDS_DIAG(gNdsStageMPCeilFloorLoopLineSweepCandidateCount++);
                hit_dist = fabsf(hit_y - (sweep_position->y - speed_y));
                if (hit_dist >= best_dist)
                {
                    continue;
                }
                best_dist = hit_dist;
                best_line = line_id;
                best_flags = ndsMPVertexFlags(verts, v1_id);
                best_pos.x = hit_x + vedge_x;
                best_pos.y = hit_y + vedge_y;
                best_pos.z = 0.0F;
                ndsMPGetFCAngle(&best_angle, (s32)v1.x, (s32)v1.y,
                                (s32)v2.x, (s32)v2.y, -1);
            }
        }
    }
    if (best_line < 0)
    {
        return FALSE;
    }
    if (ga_last != NULL)
    {
        *ga_last = best_pos;
    }
    if (stand_line_id != NULL)
    {
        *stand_line_id = best_line;
    }
    if (stand_coll_flags != NULL)
    {
        *stand_coll_flags = best_flags;
    }
    if (angle != NULL)
    {
        *angle = best_angle;
    }
    return TRUE;
}

sb32 mpCollisionCheckCeilLineCollisionSame(Vec3f *position,
                                           Vec3f *translate,
                                           Vec3f *ga_last,
                                           s32 *stand_line_id,
                                           u32 *stand_coll_flags,
                                           Vec3f *angle)
{
    sb32 hit;

    hit = ndsStageMPCeilFloorLoopSweep(position, translate, ga_last,
                                       stand_line_id, stand_coll_flags,
                                       angle, FALSE);
    return hit;
}

sb32 mpCollisionCheckCeilLineCollisionDiff(Vec3f *position,
                                           Vec3f *translate,
                                           Vec3f *ga_last,
                                           s32 *stand_line_id,
                                           u32 *stand_coll_flags,
                                           Vec3f *angle)
{
    sb32 hit;

    hit = ndsStageMPCeilFloorLoopSweep(position, translate, ga_last,
                                       stand_line_id, stand_coll_flags,
                                       angle, TRUE);
    return hit;
}

static sb32 ndsMPCommonCheckSetFighterCliffEdgeLive(
    GObj *fighter_gobj, s32 floor_line_id)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    MPObjectColl *map_coll;
    Vec3f *translate;
    Vec3f edge_pos;
    Vec3f wall_pos;
    Vec3f angle;
    u32 flags;
    f32 floor_dist;

    if ((fp == NULL) || (fp->coll_data.p_translate == NULL) ||
        (mpCollisionCheckExistLineID(floor_line_id) == FALSE))
    {
        return FALSE;
    }
    map_coll = &fp->coll_data.map_coll;
    translate = fp->coll_data.p_translate;
    edge_pos.z = translate->z;
    mpCollisionGetFloorEdgeL(floor_line_id, &edge_pos);
    if (translate->x <= edge_pos.x)
    {
        if ((fp->lr == -1) && (fp->input.pl.stick_range.x > -60))
        {
            edge_pos.x += 40.0F;
            if (mpCollisionGetFCCommonFloor(
                    floor_line_id, &edge_pos, &floor_dist,
                    &flags, &angle) == FALSE)
            {
                return FALSE;
            }
            edge_pos.y += floor_dist;
            wall_pos.x = map_coll->width + edge_pos.x;
            wall_pos.y = (map_coll->center + edge_pos.y) -
                map_coll->bottom;
            wall_pos.z = edge_pos.z;
            if (mpCollisionCheckLWallLineCollisionSame(
                    &edge_pos, &wall_pos, NULL, NULL, NULL, NULL) == FALSE)
            {
                fp->lr = -1;
                goto set_ground;
            }
        }
    }
    else if ((fp->lr == +1) && (fp->input.pl.stick_range.x < 60))
    {
        mpCollisionGetFloorEdgeR(floor_line_id, &edge_pos);
        edge_pos.x -= 40.0F;
        if (mpCollisionGetFCCommonFloor(
                floor_line_id, &edge_pos, &floor_dist,
                &flags, &angle) == FALSE)
        {
            return FALSE;
        }
        edge_pos.y += floor_dist;
        wall_pos.x = edge_pos.x - map_coll->width;
        wall_pos.y = (map_coll->center + edge_pos.y) - map_coll->bottom;
        wall_pos.z = edge_pos.z;
        if (mpCollisionCheckRWallLineCollisionSame(
                &edge_pos, &wall_pos, NULL, NULL, NULL, NULL) == FALSE)
        {
            fp->lr = +1;
            goto set_ground;
        }
    }
    return FALSE;

set_ground:
    *translate = edge_pos;
    fp->coll_data.floor_line_id = floor_line_id;
    fp->coll_data.floor_flags = flags;
    fp->coll_data.floor_angle = angle;
    fp->coll_data.floor_dist = 0.0F;
    fp->coll_data.mask_stat |= MAP_FLAG_FLOOREDGE;
    return TRUE;
}

static sb32 ndsMPCommonCheckSetFighterEdgeLive(
    GObj *fighter_gobj, s32 floor_line_id)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    MPObjectColl *map_coll;
    Vec3f *translate;
    Vec3f edge_pos;
    Vec3f wall_start;
    Vec3f wall_end;
    Vec3f angle;
    u32 flags;
    f32 floor_dist;

    if ((fp == NULL) || (fp->coll_data.p_translate == NULL) ||
        (mpCollisionCheckExistLineID(floor_line_id) == FALSE))
    {
        return FALSE;
    }
    map_coll = &fp->coll_data.map_coll;
    translate = fp->coll_data.p_translate;
    edge_pos.z = translate->z;
    mpCollisionGetFloorEdgeL(floor_line_id, &edge_pos);
    if (translate->x <= edge_pos.x)
    {
        if (mpCollisionGetFCCommonFloor(
                floor_line_id, &edge_pos, &floor_dist,
                &flags, &angle) == FALSE)
        {
            return FALSE;
        }
        wall_start.x = edge_pos.x + 1.0F;
        wall_start.y = edge_pos.y + 1.0F;
        wall_start.z = edge_pos.z;
        wall_end.x = map_coll->width + edge_pos.x;
        wall_end.y = (map_coll->center + edge_pos.y) - map_coll->bottom;
        wall_end.z = edge_pos.z;
        if (mpCollisionCheckLWallLineCollisionSame(
                &wall_start, &wall_end, NULL, NULL, NULL, NULL) == FALSE)
        {
            goto set_ground;
        }
    }
    else
    {
        mpCollisionGetFloorEdgeR(floor_line_id, &edge_pos);
        if (mpCollisionGetFCCommonFloor(
                floor_line_id, &edge_pos, &floor_dist,
                &flags, &angle) == FALSE)
        {
            return FALSE;
        }
        wall_start.x = edge_pos.x - 1.0F;
        wall_start.y = edge_pos.y + 1.0F;
        wall_start.z = edge_pos.z;
        wall_end.x = edge_pos.x - map_coll->width;
        wall_end.y = (map_coll->center + edge_pos.y) - map_coll->bottom;
        wall_end.z = edge_pos.z;
        if (mpCollisionCheckRWallLineCollisionSame(
                &wall_start, &wall_end, NULL, NULL, NULL, NULL) == FALSE)
        {
            goto set_ground;
        }
    }
    return FALSE;

set_ground:
    *translate = edge_pos;
    fp->coll_data.floor_line_id = floor_line_id;
    fp->coll_data.floor_flags = flags;
    fp->coll_data.floor_angle = angle;
    fp->coll_data.floor_dist = 0.0F;
    return TRUE;
}

static sb32 ndsMPCommonRunFighterFloorEdgeLive(
    MPCollData *coll_data, GObj *fighter_gobj, u32 flags)
{
    s32 floor_line_id;
    sb32 is_floor = FALSE;

    if (coll_data == NULL)
    {
        return FALSE;
    }
    floor_line_id = coll_data->floor_line_id;
    /* mpcommon.c:162 opens with these two wall tests, and this function ran
     * without them while the wall sweep was still an approximation. They are
     * what stops a floor from carrying a fighter into a wall: Peach's Castle's
     * sliding board took a fighter standing still on its right half straight
     * through the tower (x 1320 -> 538 over 220 tics, 2026-09-21), because
     * once the board's end slid under them the floor-edge adjust had no floor
     * left to put them back on and nothing else looked at the wall. With the
     * tests restored the wall holds them, the floor test then misses, and the
     * stop-edge arm refuses the snap because a wall is in the way -- the
     * source's sequence, and the fighter drops off the board's end. */
    if (mpProcessCheckTestLWallCollision(coll_data) != FALSE)
    {
        mpProcessRunLWallCollision(coll_data);
        coll_data->is_coll_end = TRUE;
    }
    if (mpProcessCheckTestRWallCollision(coll_data) != FALSE)
    {
        mpProcessRunRWallCollision(coll_data);
        coll_data->is_coll_end = TRUE;
    }
    if (mpProcessCheckTestFloorCollisionNew(coll_data) != FALSE)
    {
        if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
        {
            mpProcessRunFloorEdgeAdjust(coll_data);
            is_floor = TRUE;
        }
    }
    else if ((flags & MAP_PROC_TYPE_CLIFFEDGE) != 0u)
    {
        (void)ndsMPCommonCheckSetFighterCliffEdgeLive(
            fighter_gobj, floor_line_id);
        coll_data->is_coll_end = TRUE;
    }
    else if ((flags & MAP_PROC_TYPE_STOPEDGE) != 0u)
    {
        if (ndsMPCommonCheckSetFighterEdgeLive(
                fighter_gobj, floor_line_id) != FALSE)
        {
            is_floor = TRUE;
        }
        else
        {
            coll_data->is_coll_end = TRUE;
        }
    }
    else
    {
        coll_data->is_coll_end = TRUE;
    }
    if (mpProcessCheckTestFloorCollision(coll_data, floor_line_id) != FALSE)
    {
        mpProcessSetLandingFloor(coll_data);
        if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
        {
            mpProcessRunFloorEdgeAdjust(coll_data);
            is_floor = TRUE;
        }
        coll_data->mask_stat &= (u16)~MAP_FLAG_FLOOREDGE;
        coll_data->is_coll_end = FALSE;
    }
    return is_floor;
}

sb32 mpCommonRunFighterAllCollisions(MPCollData *coll_data,
                                      GObj *fighter_gobj,
                                      u32 flags)
{
    s32 floor_line_id;
    sb32 is_floor = FALSE;

    if (coll_data == NULL)
    {
        return FALSE;
    }
    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        return ndsMPCommonRunFighterFloorEdgeLive(
            coll_data, fighter_gobj, flags);
    }
    floor_line_id = coll_data->floor_line_id;
    (void)fighter_gobj;

    if (mpProcessCheckTestFloorCollisionNew(coll_data) != FALSE)
    {
        if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
        {
            mpProcessRunFloorEdgeAdjust(coll_data);
            coll_data->mask_stat &= (u16)~MAP_FLAG_FLOOREDGE;
            coll_data->is_coll_end = FALSE;
            is_floor = TRUE;
        }
    }
    else if ((flags & MAP_PROC_TYPE_CLIFFEDGE) != 0u)
    {
        coll_data->is_coll_end = TRUE;
    }
    else if ((flags & MAP_PROC_TYPE_STOPEDGE) != 0u)
    {
        coll_data->is_coll_end = TRUE;
    }
    else
    {
        coll_data->is_coll_end = TRUE;
    }
    coll_data->floor_line_id =
        ((coll_data->floor_line_id >= 0) ? coll_data->floor_line_id :
        floor_line_id);
    return is_floor;
}

static sb32 ndsStageMPUpdateFloorLoopUpdateFighter(GObj *fighter_gobj)
{
    return TRUE;
}

sb32 mpCommonCheckFighterOnFloor(GObj *fighter_gobj)
{
    FTStruct *fp;

    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
        if ((fp == NULL) || (fp->coll_data.p_translate == NULL))
        {
            return FALSE;
        }
        return mpProcessUpdateMain(
            &fp->coll_data, mpCommonRunFighterAllCollisions,
            fighter_gobj, MAP_PROC_TYPE_DEFAULT);
    }
    return ndsMPCommonCheckFighterOnCliffEdgeBounded(fighter_gobj);
    NDS_DIAG(gNdsStageMPUpdateFloorLoopCheckFloorCallCount++);
    if (ndsStageMPUpdateFloorLoopUpdateFighter(fighter_gobj) != FALSE)
    {
        NDS_DIAG(gNdsStageMPUpdateFloorLoopCheckFloorHitCount++);
        return TRUE;
    }
    NDS_DIAG(gNdsStageMPUpdateFloorLoopCheckFloorMissCount++);
    return FALSE;
}

sb32 mpCommonCheckFighterOnEdge(GObj *fighter_gobj)
{
    FTStruct *fp;

    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
        if ((fp == NULL) || (fp->coll_data.p_translate == NULL))
        {
            return FALSE;
        }
        return mpProcessUpdateMain(
            &fp->coll_data, mpCommonRunFighterAllCollisions,
            fighter_gobj, MAP_PROC_TYPE_STOPEDGE);
    }
    return TRUE;
}

sb32 mpCommonCheckFighterOnCliffEdge(GObj *fighter_gobj)
{
    FTStruct *fp;

    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
        if ((fp == NULL) || (fp->coll_data.p_translate == NULL))
        {
            return FALSE;
        }
        return mpProcessUpdateMain(
            &fp->coll_data, mpCommonRunFighterAllCollisions,
            fighter_gobj, MAP_PROC_TYPE_CLIFFEDGE);
    }
    return ndsMPCommonCheckFighterOnCliffEdgeBounded(fighter_gobj);
    gNdsStageMPUpdateFloorLoopCheckCliffEdgeCallCount++;
    if (ndsStageMPUpdateFloorLoopUpdateFighter(fighter_gobj) != FALSE)
    {
        NDS_DIAG(gNdsStageMPUpdateFloorLoopCheckCliffEdgeHitCount++);
        return TRUE;
    }
    NDS_DIAG(gNdsStageMPUpdateFloorLoopCheckCliffEdgeMissCount++);
    return FALSE;
}

typedef struct NDSStageMPWallHitScoutCounterSnapshot {
    u32 run_call_count;
    u32 check_l_call_count;
    u32 check_r_call_count;
    u32 check_l_hit_count;
    u32 check_r_hit_count;
    u32 check_l_miss_count;
    u32 check_r_miss_count;
    u32 wall_l_call_count;
    u32 wall_r_call_count;
    u32 wall_l_hit_count;
    u32 wall_r_hit_count;
    u32 wall_l_miss_count;
    u32 wall_r_miss_count;
    u32 edge_under_l_call_count;
    u32 edge_under_r_call_count;
    u32 edge_under_deferred_count;
    u32 adjust_l_call_count;
    u32 adjust_r_call_count;
    u32 no_adjust_count;
    u32 p0_run_count;
    u32 p1_run_count;
    s32 p0_final_line_id;
    s32 p1_final_line_id;
    u32 p0_final_floor_ok;
    u32 p1_final_floor_ok;
    u32 unsafe_count;
    u32 edge_loop_l_call_count;
    u32 edge_loop_r_call_count;
    u32 edge_loop_l_hit_count;
    u32 edge_loop_r_hit_count;
    u32 edge_loop_l_miss_count;
    u32 edge_loop_r_miss_count;
    s32 edge_loop_l_line_id;
    s32 edge_loop_r_line_id;
    u32 edge_loop_l_line_kind;
    u32 edge_loop_r_line_kind;
} NDSStageMPWallHitScoutCounterSnapshot;

typedef struct NDSStageMPWallHitScoutResultSnapshot {
    u32 run_count;
    u32 floor_test_count;
    u32 wall_test_count;
    u32 candidate_count;
    u32 hit_count;
    u32 miss_count;
    s32 floor_line_id;
    s32 wall_line_id;
    s32 edge_under_line_id;
    u32 side;
    u32 wall_line_kind;
    s32 start_x_milli;
    s32 start_y_milli;
    s32 final_x_milli;
    s32 final_y_milli;
    s32 delta_x_milli;
    s32 delta_y_milli;
    u32 final_floor_ok;
    u32 unsafe_count;
} NDSStageMPWallHitScoutResultSnapshot;

/* Retired source-scale display/preview diagnostics live host-only in
 * src/host/graphics_reference/inishie_scale_reference.c. The ROM keeps only
 * live collision/scale gameplay; the retired path reported its own unsafe
 * counts and never a PASS. */

/* decomp mp/mpcollision.c:4023-4026 and :4176-4190 verbatim (2026-09-05). The 1P scenes are
 * the only callers -- Master Hand's statuses and the bonus boards count and
 * list the lines of one kind, and sc1pgame.c sets the ground's music -- so
 * the port never carried them; a NDS_P2_1P_GAME=1 link needs all three.
 *
 * THE GROUPS ARE BUILT LAZILY HERE, NOT BY THE LOADER.
 *
 * The source fills gMPCollisionLineGroups inside its own map loader, so its
 * getters may read the table cold. The port does not: mpCollisionInitGroundData
 * (reloc_backend_compat_shims.c) only assigns the geometry, and
 * ndsMPCollisionEnsureLineGroups (:180) builds the per-kind id arrays on first
 * demand -- which is why func_ovl2_800F8FFC (:1690) and the topology getter
 * (:2113) each call it before touching line_count/line_id. These two arrived
 * without that call, so on any scene whose first reader is one of them the
 * table is still the invalidate state: line_count 0 and line_id NULL.
 *
 * That is not a soft failure. ftBossCommonSetDefaultLineID
 * (ft/ftchar/ftboss/ftbosscommon.c:200-207) treats a zero floor count as a
 * fatal condition and spins in `while (TRUE)` printing "nothing under cll id",
 * so Master Hand would hang the console the moment he is built; and
 * sc1PBonusStageMakePlatforms (sc/sc1pmode/sc1pbonusstage.c:579-581) would find
 * zero floors, board no platforms and leave Board the Platforms unwinnable. The
 * ensure is idempotent and early-outs on a geometry it has already built, which
 * is exactly what the two existing call sites rely on. */
s32 mpCollisionGetLineCountType(s32 line_type)
{
    ndsMPCollisionEnsureLineGroups();
    return gMPCollisionLineGroups[line_type].line_count;
}

void mpCollisionGetLineIDsTypeCount(s32 type, s32 count, s32 *line_ids)
{
    s32 i;

    ndsMPCollisionEnsureLineGroups();
    for (i = 0; i < count; i++)
    {
        line_ids[i] = gMPCollisionLineGroups[type].line_id[i];
    }
}

void mpCollisionSetBGM(void)
{
    gMPCollisionBGMCurrent = gMPCollisionBGMDefault = gMPCollisionGroundData->bgm_id;
}
