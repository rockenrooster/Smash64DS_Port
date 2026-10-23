/* Map collision (mpcollision.c on the DS): the compact geometry views, line
 * and vertex records, their float and fixed caches and the line extents. A
 * slice of reloc_backend.c; its topic slices follow it there: mp_lines,
 * mp_walls, mp_floors. */
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

NDS_COLD void ndsMPCollisionInvalidateTopology(void)
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


NDS_COLD static void ndsMPVertexF32Reset(void)
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
NDS_COLD static f32 __attribute__((noinline, cold))
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

NDS_COLD static void ndsStageCollisionLoopCountLines(void)
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
