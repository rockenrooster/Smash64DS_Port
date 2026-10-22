/* Map collision, sweeps and walls: the float edges of the fixed-point
 * kernels, the sweep groups and their rejects, segment crossings and the
 * wall collisions. A slice of reloc_backend.c. */

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
