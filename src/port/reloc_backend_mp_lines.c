/* Map collision, lines: the yakumono each line rides, line and vertex
 * queries, the topology's active lines and the floor, ceiling and wall
 * edges. A slice of reloc_backend.c, after reloc_backend_mp_collision.c. */

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

NDS_COLD u16 mpCollisionGetVertexFlagsLineID(s32 line_id)
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
NDS_COLD static sb32 __attribute__((noinline, cold))
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

NDS_COLD static int ndsMPReadTopologyLine(void *context, int line_id,
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
