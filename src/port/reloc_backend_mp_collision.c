static sb32 ndsStageCollisionLoopGeometryReady(void)
{
    return ((gSCManagerSceneData.gkind == nGRKindPupupu) &&
            (gMPCollisionGroundData != NULL) &&
            (gMPCollisionGeometry != NULL) &&
            (gMPCollisionGeometry->line_info != NULL) &&
            (gMPCollisionGeometry->vertex_links != NULL) &&
            (gMPCollisionGeometry->vertex_id != NULL) &&
            (gMPCollisionGeometry->vertex_data != NULL)) ? TRUE : FALSE;
}

static u16 ndsMPO2RReadU16(const void *base, u32 half_index)
{
    const u32 *words = base;
    u32 word = words[half_index / 2u];

    return (half_index & 1u) ? (u16)(word & 0xffffu) :
        (u16)(word >> 16);
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

static MPLineInfo *ndsMPLineInfoAt(MPLineInfo *line_info, u32 index)
{
    return (MPLineInfo *)((u8 *)line_info + (index * sizeof(MPLineInfo)));
}

static u32 ndsMPLineInfoYakumonoID(MPLineInfo *line_info)
{
    return ndsMPO2RReadU16(line_info, 0u);
}

static u32 ndsMPLineInfoGroupID(MPLineInfo *line_info, u32 kind)
{
    return ndsMPO2RReadU16(line_info, 1u + (kind * 2u));
}

static u32 ndsMPLineInfoLineCount(MPLineInfo *line_info, u32 kind)
{
    return ndsMPO2RReadU16(line_info, 2u + (kind * 2u));
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

static f32 ndsMPLineDistanceFC(f32 opx, s32 v1x, s32 v1y, s32 v2x,
                               s32 v2y)
{
    return (f32)v1y + (((opx - (f32)v1x) / ((f32)v2x - (f32)v1x)) *
        ((f32)v2y - (f32)v1y));
}

static void ndsMPGetFCAngle(Vec3f *angle, s32 v1x, s32 v1y, s32 v2x,
                            s32 v2y, s32 ud)
{
    f32 py;
    f32 inv_len;
    f32 dist_y;

    if (angle == NULL)
    {
        return;
    }
    angle->z = 0.0F;
    dist_y = (f32)(v2y - v1y);
    if (dist_y == 0.0F)
    {
        angle->x = 0.0F;
        angle->y = (f32)ud;
        return;
    }
    if (v2x == v1x)
    {
        gNdsStageCollisionLoopDivisionGuardCount++;
        angle->x = 0.0F;
        angle->y = (f32)ud;
        return;
    }
    py = -(dist_y / (f32)(v2x - v1x));
    inv_len = 1.0F / sqrtf((py * py) + 1.0F);
    if (ud < 0)
    {
        angle->x = -py * inv_len;
        angle->y = -inv_len;
    }
    else
    {
        angle->x = py * inv_len;
        angle->y = inv_len;
    }
}

static s32 ndsMPGetLineKindForLineID(s32 line_id)
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
    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);

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
                return (s32)kind;
            }
        }
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
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);

        for (j = 0u; j < nMPLineKindEnumCount; j++)
        {
            u32 count = ndsMPLineInfoLineCount(info, j);

            if (count > 4096u)
            {
                count = 4096u;
            }
            gNdsStageCollisionLoopTotalLineCount += count;
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
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);

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

                if ((count < 2u) || (count > 128u))
                {
                    gNdsStageCollisionLoopBadVertexCount++;
                    return FALSE;
                }
                last_vertex = first_vertex + count - 1u;
                v_first_id = ndsMPVertexID(ids, first_vertex);
                v_last_id = ndsMPVertexID(ids, last_vertex);
                v_first_x = ndsMPVertexX(verts, v_first_id);
                v_first_y = ndsMPVertexY(verts, v_first_id);
                v_last_x = ndsMPVertexX(verts, v_last_id);
                v_last_y = ndsMPVertexY(verts, v_last_id);
                if (vertex_count != NULL)
                {
                    *vertex_count = (s32)count;
                }
                if (flags != NULL)
                {
                    *flags = ndsMPVertexFlags(verts, v_first_id);
                }
                if (left != NULL)
                {
                    left->z = 0.0F;
                }
                if (right != NULL)
                {
                    right->z = 0.0F;
                }
                if (v_first_x <= v_last_x)
                {
                    if (left != NULL)
                    {
                        left->x = (f32)v_first_x;
                        left->y = (f32)v_first_y;
                    }
                    if (right != NULL)
                    {
                        right->x = (f32)v_last_x;
                        right->y = (f32)v_last_y;
                    }
                }
                else
                {
                    if (left != NULL)
                    {
                        left->x = (f32)v_last_x;
                        left->y = (f32)v_last_y;
                    }
                    if (right != NULL)
                    {
                        right->x = (f32)v_first_x;
                        right->y = (f32)v_first_y;
                    }
                }
                return TRUE;
            }
        }
    }
    gNdsStageCollisionLoopOutOfRangeLineCount++;
    return FALSE;
}

sb32 mpCollisionCheckExistLineID(s32 line_id)
{
    return (ndsMPFindLineEndpoints(line_id, NULL, NULL, NULL, NULL) != FALSE) ?
        TRUE : FALSE;
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

    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);

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
                *yakumono_id = ndsMPLineInfoYakumonoID(info);
                return TRUE;
            }
        }
    }
    return FALSE;
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

static sb32 ndsStageCollisionLoopGetFloorSample(u32 slot, Vec3f *pos)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;

    if ((pos == NULL) || (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);
        s32 first = (s32)ndsMPLineInfoGroupID(info, nMPLineKindFloor);
        s32 count =
            (s32)ndsMPLineInfoLineCount(info, nMPLineKindFloor);
        s32 end;
        s32 line_id;

        if (count <= 0)
        {
            continue;
        }
        if (count > 4096)
        {
            count = 4096;
        }
        end = first + count;
        for (line_id = first; line_id < end; line_id++)
        {
            Vec3f left;
            Vec3f right;
            f32 ratio;

            if (ndsMPFindLineEndpoints(line_id, &left, &right, NULL,
                    NULL) == FALSE)
            {
                continue;
            }
            if (fabsf(right.x - left.x) < 64.0F)
            {
                continue;
            }
            ratio = (slot == 0u) ? 0.35F : 0.65F;
            pos->x = left.x + ((right.x - left.x) * ratio);
            pos->y = 0.0F;
            pos->z = 0.0F;
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsStageCollisionLoopSeedSlotOnFloorRange(u32 slot)
{
    FTStruct *fp;
    DObj *root;
    Vec3f pos;

    if (slot >= 2u)
    {
        return;
    }
    fp = &sNdsFighterStructPool[slot];
    root = fp->joints[nFTPartsJointTopN];
    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) || (root == NULL))
    {
        return;
    }
    if (ndsStageCollisionLoopGetFloorSample(slot, &pos) == FALSE)
    {
        return;
    }
    root->translate.vec.f.x = pos.x;
    root->translate.vec.f.y = pos.y;
    root->translate.vec.f.z = pos.z;
    fp->coll_data.pos_prev = root->translate.vec.f;
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
    if (ndsFighterMarioFoxStageFloorEdgeLoopProofEnabled() != FALSE)
    {
        gNdsStageFloorEdgeLoopLineTypeCallCount++;
    }
    return ndsMPGetLineKindForLineID(line_id);
}

void mpCollisionGetVertexPositionID(s32 line_id, s32 vertex_id,
                                    Vec3f *object_pos)
{
    if (ndsFighterMarioFoxStageFloorEdgeLoopProofEnabled() != FALSE)
    {
        gNdsStageFloorEdgeLoopVertexPositionCallCount++;
    }
    if (ndsMPGetVertexPositionForLineID(line_id, vertex_id, object_pos) ==
        FALSE)
    {
        if (object_pos != NULL)
        {
            object_pos->x = 0.0F;
            object_pos->y = 0.0F;
            object_pos->z = 0.0F;
        }
    }
}

sb32 mpCollisionGetFCCommonFloor(s32 line_id, Vec3f *object_pos,
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
    u32 j;

    if (ndsFighterMarioFoxStageFloorEdgeLoopProofEnabled() != FALSE)
    {
        gNdsStageFloorEdgeLoopFCCommonCallCount++;
    }
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
    vertex_first = ndsMPVertexLinkFirst(links, (u32)line_id);
    vertex_count = ndsMPVertexLinkCount(links, (u32)line_id);
    if ((vertex_count < 2u) || (vertex_count > 128u))
    {
        return FALSE;
    }
    segment_count = vertex_count - 1u;
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
        f32 floor_y;

        if (!(((f32)x1 <= object_pos->x &&
               (f32)x2 >= object_pos->x) ||
              ((f32)x2 <= object_pos->x &&
               (f32)x1 >= object_pos->x)))
        {
            continue;
        }
        if (x1 == x2)
        {
            gNdsStageCollisionLoopDivisionGuardCount++;
            continue;
        }
        floor_y = ndsMPLineDistanceFC(object_pos->x, x1, y1, x2, y2);
        if (floor_dist != NULL)
        {
            *floor_dist = floor_y - object_pos->y;
        }
        if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
        {
            f32 signed_dist = floor_y - object_pos->y;

            if (signed_dist > 0.0F)
            {
                gNdsStageMPProcessFloorLoopFCCommonPositiveDistCount++;
            }
            else if (signed_dist < 0.0F)
            {
                gNdsStageMPProcessFloorLoopFCCommonNegativeDistCount++;
            }
            else
            {
                gNdsStageMPProcessFloorLoopFCCommonZeroDistCount++;
            }
        }
        if (floor_flags != NULL)
        {
            *floor_flags = ndsMPVertexFlags(verts, v1_id);
        }
        ndsMPGetFCAngle(angle, x1, y1, x2, y2, +1);
        if (ndsFighterMarioFoxStageFloorEdgeLoopProofEnabled() != FALSE)
        {
            gNdsStageFloorEdgeLoopFCCommonHitCount++;
        }
        return TRUE;
    }
    return FALSE;
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
    u32 j;

    if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPCeilFloorLoopFCCommonCallCount++;
    }
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
        if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPCeilFloorLoopFCCommonMissCount++;
        }
        return FALSE;
    }
    links = geometry->vertex_links;
    ids = geometry->vertex_id;
    verts = geometry->vertex_data;
    vertex_first = ndsMPVertexLinkFirst(links, (u32)line_id);
    vertex_count = ndsMPVertexLinkCount(links, (u32)line_id);
    if ((vertex_count < 2u) || (vertex_count > 128u))
    {
        if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPCeilFloorLoopFCCommonMissCount++;
        }
        return FALSE;
    }
    segment_count = vertex_count - 1u;
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
        f32 ceil_y;

        if (!(((f32)x1 <= object_pos->x &&
               (f32)x2 >= object_pos->x) ||
              ((f32)x2 <= object_pos->x &&
               (f32)x1 >= object_pos->x)))
        {
            continue;
        }
        if (x1 == x2)
        {
            gNdsStageCollisionLoopDivisionGuardCount++;
            continue;
        }
        ceil_y = ndsMPLineDistanceFC(object_pos->x, x1, y1, x2, y2);
        if (ceil_dist != NULL)
        {
            *ceil_dist = ceil_y - object_pos->y;
        }
        if (ceil_flags != NULL)
        {
            *ceil_flags = ndsMPVertexFlags(verts, v1_id);
        }
        ndsMPGetFCAngle(angle, x1, y1, x2, y2, -1);
        if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPCeilFloorLoopFCCommonHitCount++;
            gNdsStageMPCeilFloorLoopCeilDistMilli =
                ndsFloatToMilliSigned(ceil_y - object_pos->y);
        }
        return TRUE;
    }
    if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPCeilFloorLoopFCCommonMissCount++;
    }
    return FALSE;
}

static sb32 ndsStageMPEdgeFloorLoopPointMatch(const Vec3f *a,
                                              const Vec3f *b)
{
    return ((a != NULL) && (b != NULL) &&
            (fabsf(a->x - b->x) <= 1.0F) &&
            (fabsf(a->y - b->y) <= 1.0F)) ? TRUE : FALSE;
}

static s32 ndsMPFindAdjacentWallForFloorEdge(s32 floor_line_id,
                                             sb32 is_right_edge)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    Vec3f floor_left;
    Vec3f floor_right;
    Vec3f target;
    u32 kind_order[2];
    u32 yakumono_count;
    u32 order;
    u32 i;

    if ((floor_line_id < 0) ||
        (ndsMPLineIDIsFloor(floor_line_id) == FALSE) ||
        (ndsMPFindLineEndpoints(floor_line_id, &floor_left, &floor_right,
            NULL, NULL) == FALSE) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return -1;
    }
    target = (is_right_edge != FALSE) ? floor_right : floor_left;
    if (is_right_edge != FALSE)
    {
        kind_order[0] = nMPLineKindRWall;
        kind_order[1] = nMPLineKindLWall;
    }
    else
    {
        kind_order[0] = nMPLineKindLWall;
        kind_order[1] = nMPLineKindRWall;
    }
    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (order = 0u; order < ARRAY_COUNT(kind_order); order++)
    {
        u32 line_kind = kind_order[order];

        for (i = 0u; i < yakumono_count; i++)
        {
            MPLineInfo *info = ndsMPLineInfoAt(line_info, i);
            s32 first = (s32)ndsMPLineInfoGroupID(info, line_kind);
            s32 count = (s32)ndsMPLineInfoLineCount(info, line_kind);
            s32 end;
            s32 line_id;

            if (count <= 0)
            {
                continue;
            }
            if (count > 4096)
            {
                count = 4096;
            }
            end = first + count;
            for (line_id = first; line_id < end; line_id++)
            {
                Vec3f wall_a;
                Vec3f wall_b;

                if ((ndsMPFindLineEndpoints(line_id, &wall_a, &wall_b,
                        NULL, NULL) != FALSE) &&
                    ((ndsStageMPEdgeFloorLoopPointMatch(&target, &wall_a) !=
                        FALSE) ||
                     (ndsStageMPEdgeFloorLoopPointMatch(&target, &wall_b) !=
                        FALSE)))
                {
                    return line_id;
                }
            }
        }
    }
    return -1;
}

s32 mpCollisionGetEdgeUnderLLineID(s32 line_id)
{
    s32 edge_under_line_id = -1;

    if (ndsFighterMarioFoxStageFloorEdgeLoopProofEnabled() != FALSE)
    {
        gNdsStageFloorEdgeLoopEdgeUnderLCallCount++;
        gNdsStageFloorEdgeLoopEdgeUnderDeferredCount++;
    }
    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPAdjustFloorLoopEdgeUnderLCallCount++;
        if (ndsFighterMarioFoxStageMPEdgeFloorLoopProofEnabled() == FALSE)
        {
            gNdsStageMPAdjustFloorLoopEdgeUnderDeferredCount++;
        }
    }
    if ((ndsFighterMarioFoxStageMPEdgeFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPLiveStaleFloorLoopProbeActive == FALSE))
    {
        gNdsStageMPEdgeFloorLoopEdgeUnderLCallCount++;
        if ((gNdsStageMPEdgeFloorLoopSelectedFloorLineID < 0) &&
            (ndsMPLineIDIsFloor(line_id) != FALSE))
        {
            gNdsStageMPEdgeFloorLoopSelectedFloorLineID = line_id;
        }
        edge_under_line_id =
            ndsMPFindAdjacentWallForFloorEdge(line_id, FALSE);
        if (edge_under_line_id >= 0)
        {
            gNdsStageMPEdgeFloorLoopEdgeUnderLHitCount++;
            gNdsStageMPEdgeFloorLoopEdgeUnderLLineID = edge_under_line_id;
            gNdsStageMPEdgeFloorLoopEdgeUnderLLineKind =
                (u32)ndsMPGetLineKindForLineID(edge_under_line_id);
        }
        else
        {
            gNdsStageMPEdgeFloorLoopEdgeUnderLMissCount++;
        }
        return edge_under_line_id;
    }
    return -1;
}

s32 mpCollisionGetEdgeUnderRLineID(s32 line_id)
{
    s32 edge_under_line_id = -1;

    if (ndsFighterMarioFoxStageFloorEdgeLoopProofEnabled() != FALSE)
    {
        gNdsStageFloorEdgeLoopEdgeUnderRCallCount++;
        gNdsStageFloorEdgeLoopEdgeUnderDeferredCount++;
    }
    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPAdjustFloorLoopEdgeUnderRCallCount++;
        if (ndsFighterMarioFoxStageMPEdgeFloorLoopProofEnabled() == FALSE)
        {
            gNdsStageMPAdjustFloorLoopEdgeUnderDeferredCount++;
        }
    }
    if ((ndsFighterMarioFoxStageMPEdgeFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPLiveStaleFloorLoopProbeActive == FALSE))
    {
        gNdsStageMPEdgeFloorLoopEdgeUnderRCallCount++;
        if ((gNdsStageMPEdgeFloorLoopSelectedFloorLineID < 0) &&
            (ndsMPLineIDIsFloor(line_id) != FALSE))
        {
            gNdsStageMPEdgeFloorLoopSelectedFloorLineID = line_id;
        }
        edge_under_line_id =
            ndsMPFindAdjacentWallForFloorEdge(line_id, TRUE);
        if (edge_under_line_id >= 0)
        {
            gNdsStageMPEdgeFloorLoopEdgeUnderRHitCount++;
            gNdsStageMPEdgeFloorLoopEdgeUnderRLineID = edge_under_line_id;
            gNdsStageMPEdgeFloorLoopEdgeUnderRLineKind =
                (u32)ndsMPGetLineKindForLineID(edge_under_line_id);
        }
        else
        {
            gNdsStageMPEdgeFloorLoopEdgeUnderRMissCount++;
        }
        return edge_under_line_id;
    }
    return -1;
}

void mpCollisionGetFloorEdgeL(s32 line_id, Vec3f *object_pos)
{
    Vec3f left;

    if (object_pos == NULL)
    {
        return;
    }
    if (ndsMPFindLineEndpoints(line_id, &left, NULL, NULL, NULL) == FALSE)
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
    if (ndsMPFindLineEndpoints(line_id, NULL, &right, NULL, NULL) == FALSE)
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
    if (ndsMPFindLineEndpoints(line_id, &left, NULL, NULL, NULL) == FALSE)
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
    if (ndsMPFindLineEndpoints(line_id, NULL, &right, NULL, NULL) == FALSE)
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
    (void)line_id;
    return -1;
}

s32 mpCollisionGetEdgeUpperRLineID(s32 line_id)
{
    (void)line_id;
    return -1;
}

s32 mpCollisionGetEdgeLeftULineID(s32 line_id)
{
    (void)line_id;
    return -1;
}

s32 mpCollisionGetEdgeRightULineID(s32 line_id)
{
    (void)line_id;
    return -1;
}

static sb32 ndsStageMPSegmentIntersection2D(const Vec3f *a0,
                                            const Vec3f *a1,
                                            const Vec3f *b0,
                                            const Vec3f *b1,
                                            f32 *out_t,
                                            f32 *out_u,
                                            f32 *out_x,
                                            f32 *out_y)
{
    f32 rx;
    f32 ry;
    f32 sx;
    f32 sy;
    f32 qpx;
    f32 qpy;
    f32 denom;
    f32 t;
    f32 u;

    if ((a0 == NULL) || (a1 == NULL) || (b0 == NULL) || (b1 == NULL))
    {
        return FALSE;
    }
    rx = a1->x - a0->x;
    ry = a1->y - a0->y;
    sx = b1->x - b0->x;
    sy = b1->y - b0->y;
    denom = (rx * sy) - (ry * sx);
    if (fabsf(denom) < 0.0001F)
    {
        return FALSE;
    }
    qpx = b0->x - a0->x;
    qpy = b0->y - a0->y;
    t = ((qpx * sy) - (qpy * sx)) / denom;
    u = ((qpx * ry) - (qpy * rx)) / denom;
    if ((t < 0.0F) || (t > 1.0F) || (u < 0.0F) || (u > 1.0F))
    {
        return FALSE;
    }
    if (out_t != NULL)
    {
        *out_t = t;
    }
    if (out_u != NULL)
    {
        *out_u = u;
    }
    if (out_x != NULL)
    {
        *out_x = a0->x + (rx * t);
    }
    if (out_y != NULL)
    {
        *out_y = a0->y + (ry * t);
    }
    return TRUE;
}

static sb32 ndsStageMPAdjustFloorLoopWallSweep(Vec3f *position,
                                               Vec3f *translate,
                                               Vec3f *ga_last,
                                               s32 *stand_line_id,
                                               u32 *stand_coll_flags,
                                               Vec3f *angle,
                                               u32 line_kind,
                                               sb32 is_diff)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;
    f32 best_t = 3.402823466e+38F;
    s32 best_line = -1;
    Vec3f best_pos = { 0.0F, 0.0F, 0.0F };

    if ((position == NULL) || (translate == NULL) ||
        (line_kind >= nMPLineKindEnumCount) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    if (position->x == translate->x)
    {
        return FALSE;
    }
    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);
        s32 first = (s32)ndsMPLineInfoGroupID(info, line_kind);
        s32 count = (s32)ndsMPLineInfoLineCount(info, line_kind);
        s32 end;
        s32 line_id;

        if (count <= 0)
        {
            continue;
        }
        if (count > 4096)
        {
            count = 4096;
        }
        end = first + count;
        for (line_id = first; line_id < end; line_id++)
        {
            Vec3f a;
            Vec3f b;
            Vec3f sweep_position;
            Vec3f sweep_translate;
            f32 vedge_x = 0.0F;
            f32 vedge_y = 0.0F;
            u32 yakumono_id = ndsMPLineInfoYakumonoID(info);
            DObj *yakumono_dobj = NULL;
            f32 t;
            f32 u;
            f32 hit_x;
            f32 hit_y;

            sweep_position = *position;
            sweep_translate = *translate;
            if ((is_diff != FALSE) &&
                (gMPCollisionYakumonoDObjs != NULL) &&
                (gMPCollisionSpeeds != NULL) &&
                (yakumono_id < NDS_MP_YAKUMONO_DOBJ_SLOTS))
            {
                yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
            }
            if ((yakumono_dobj != NULL) &&
                (yakumono_dobj->user_data.s < nMPYakumonoStatusOff) &&
                ((yakumono_dobj->anim_joint.event32 != NULL) ||
                 (yakumono_dobj->user_data.s != nMPYakumonoStatusNone)))
            {
                vedge_x = yakumono_dobj->translate.vec.f.x;
                vedge_y = yakumono_dobj->translate.vec.f.y;
                sweep_position.x =
                    (position->x - vedge_x) + gMPCollisionSpeeds[yakumono_id].x;
                sweep_position.y =
                    (position->y - vedge_y) + gMPCollisionSpeeds[yakumono_id].y;
                sweep_translate.x = translate->x - vedge_x;
                sweep_translate.y = translate->y - vedge_y;
            }
            if ((ndsMPFindLineEndpoints(line_id, &a, &b, NULL, NULL) ==
                    FALSE) ||
                (ndsStageMPSegmentIntersection2D(&sweep_position,
                    &sweep_translate, &a, &b, &t, &u, &hit_x, &hit_y) ==
                    FALSE))
            {
                continue;
            }
            if (t < best_t)
            {
                best_t = t;
                best_line = line_id;
                best_pos.x = hit_x + vedge_x;
                best_pos.y = hit_y + vedge_y;
                best_pos.z = 0.0F;
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
        *stand_coll_flags = 0u;
    }
    if (angle != NULL)
    {
        angle->x = (line_kind == nMPLineKindLWall) ? -1.0F : 1.0F;
        angle->y = 0.0F;
        angle->z = 0.0F;
    }
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

    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPAdjustFloorLoopWallLCallCount++;
    }
    hit = ndsStageMPAdjustFloorLoopWallSweep(position, translate, ga_last,
                                             stand_line_id,
                                             stand_coll_flags, angle,
                                             nMPLineKindLWall, FALSE);
    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        if (hit != FALSE)
        {
            gNdsStageMPAdjustFloorLoopWallLHitCount++;
        }
        else
        {
            gNdsStageMPAdjustFloorLoopWallLMissCount++;
        }
    }
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

    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPAdjustFloorLoopWallRCallCount++;
    }
    hit = ndsStageMPAdjustFloorLoopWallSweep(position, translate, ga_last,
                                             stand_line_id,
                                             stand_coll_flags, angle,
                                             nMPLineKindRWall, FALSE);
    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        if (hit != FALSE)
        {
            gNdsStageMPAdjustFloorLoopWallRHitCount++;
        }
        else
        {
            gNdsStageMPAdjustFloorLoopWallRMissCount++;
        }
    }
    return hit;
}

sb32 mpCollisionCheckLWallLineCollisionDiff(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle)
{
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
    return ndsStageMPAdjustFloorLoopWallSweep(position, translate, ga_last,
                                              stand_line_id,
                                              stand_coll_flags, angle,
                                              nMPLineKindRWall, TRUE);
}

static sb32 ndsMPGetLRCommonWall(s32 line_id, Vec3f *object_pos,
                                 f32 *dist, u32 *flags, Vec3f *angle,
                                 u32 line_kind)
{
    Vec3f a;
    Vec3f b;
    f32 min_y;
    f32 max_y;
    f32 wall_x;

    if ((object_pos == NULL) ||
        (ndsMPGetLineKindForLineID(line_id) != (s32)line_kind) ||
        (ndsMPFindLineEndpoints(line_id, &a, &b, flags, NULL) == FALSE))
    {
        return FALSE;
    }
    min_y = (a.y < b.y) ? a.y : b.y;
    max_y = (a.y > b.y) ? a.y : b.y;
    if ((object_pos->y < min_y) || (object_pos->y > max_y))
    {
        return FALSE;
    }
    if (fabsf(b.y - a.y) > 0.01F)
    {
        wall_x = a.x + (((object_pos->y - a.y) / (b.y - a.y)) *
            (b.x - a.x));
    }
    else
    {
        wall_x = (a.x + b.x) * 0.5F;
    }
    if (dist != NULL)
    {
        *dist = wall_x - object_pos->x;
    }
    if (angle != NULL)
    {
        angle->x = (line_kind == nMPLineKindLWall) ? -1.0F : 1.0F;
        angle->y = 0.0F;
        angle->z = 0.0F;
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
    for (i = 0u; i < yakumono_count; i++)
    {
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);
        s32 line_first =
            (s32)ndsMPLineInfoGroupID(info, nMPLineKindFloor);
        s32 line_count =
            (s32)ndsMPLineInfoLineCount(info, nMPLineKindFloor);
        s32 line_id;
        s32 line_end;
        u32 yakumono_id = ndsMPLineInfoYakumonoID(info);

        if (gMPCollisionYakumonoDObjs != NULL)
        {
            gNdsStageCollisionLoopYakumonoDObjDeferredCount++;
            if (yakumono_id >= 1u)
            {
                gNdsStageCollisionLoopYakumonoDObjUnsafeIndexGuardCount++;
            }
        }
        line_end = line_first + line_count;
        if ((line_end - line_first) > 4096)
        {
            line_end = line_first + 4096;
        }
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
                gNdsStageCollisionLoopBadVertexCount++;
                continue;
            }
            segment_count = vertex_count - 1u;
            x_first = ndsMPVertexX(verts, ndsMPVertexID(ids, vertex_first));
            x_last = ndsMPVertexX(verts,
                                  ndsMPVertexID(ids,
                                                vertex_first + segment_count));
            if (!(((f32)x_first <= position->x &&
                   (f32)x_last >= position->x) ||
                  ((f32)x_last <= position->x &&
                   (f32)x_first >= position->x)))
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

                if (!(((f32)x1 <= position->x &&
                       (f32)x2 >= position->x) ||
                      ((f32)x2 <= position->x &&
                       (f32)x1 >= position->x)))
                {
                    continue;
                }
                if (x1 == x2)
                {
                    gNdsStageCollisionLoopDivisionGuardCount++;
                    continue;
                }
                fpos = ndsMPLineDistanceFC(position->x, x1, y1, x2, y2);
                gdist = fpos - position->y;
                if ((fpos <= position->y) &&
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
    gNdsStageCollisionLoopProjectCallCount++;
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
        gNdsStageCollisionLoopP0ProjectCount++;
        if (hit != FALSE)
        {
            gNdsStageCollisionLoopP0HitCount++;
        }
    }
    else if ((best_slot == 1u) && (best_delta <= 256.0F))
    {
        gNdsStageCollisionLoopP1ProjectCount++;
        if (hit != FALSE)
        {
            gNdsStageCollisionLoopP1HitCount++;
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
        gNdsStageCollisionLoopNoGeometryCount++;
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

static void ndsStageCollisionLoopRunProbe(Vec3f pos, u32 kind)
{
    s32 line_id = -1;
    f32 dist = 0.0F;
    u32 flags = 0u;
    Vec3f angle;
    sb32 hit;

    gNdsStageCollisionLoopProbeCount++;
    hit = mpCollisionCheckProjectFloor(&pos, &line_id, &dist, &flags,
                                       &angle);
    if (hit != FALSE)
    {
        gNdsStageCollisionLoopProbeHitCount++;
    }
    else
    {
        gNdsStageCollisionLoopProbeMissCount++;
        if (kind == 1u)
        {
            gNdsStageCollisionLoopOffstageMissCount++;
        }
        else if (kind == 2u)
        {
            gNdsStageCollisionLoopBelowFloorMissCount++;
        }
    }
}

static void ndsStageCollisionLoopAdoptRealFloor(u32 slot)
{
    FTStruct *fp;
    DObj *root;
    Vec3f pos;
    Vec3f angle;
    s32 line_id = -1;
    f32 dist = 0.0F;
    u32 flags = 0u;

    if (slot >= 2u)
    {
        return;
    }
    fp = &sNdsFighterStructPool[slot];
    root = fp->joints[nFTPartsJointTopN];
    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) || (root == NULL))
    {
        gNdsStageCollisionLoopUnexpectedStatusCount++;
        return;
    }
    pos = root->translate.vec.f;
    pos.y += 2048.0F;
    if (mpCollisionCheckProjectFloor(&pos, &line_id, &dist, &flags,
            &angle) == FALSE)
    {
        gNdsStageCollisionLoopUnexpectedStatusCount++;
        return;
    }

    root->translate.vec.f.y = pos.y + dist;
    fp->coll_data.floor_line_id = line_id;
    fp->coll_data.floor_dist = root->translate.vec.f.y;
    fp->coll_data.floor_flags = flags;
    fp->coll_data.floor_angle = angle;
    fp->coll_data.mask_stat |= MAP_FLAG_FLOOR;
    fp->coll_data.is_coll_end = FALSE;
    fp->coll_data.pos_prev = root->translate.vec.f;
    fp->ga = nMPKineticsGround;
    fp->jumps_used = 0;
    fp->lr = (slot == 0u) ? +1 : -1;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.x = (slot == 0u) ? 8.0F : -8.0F;
    fp->vel_air = fp->physics.vel_air;
    fp->physics.vel_ground.x = 0.0F;
    fp->vel_ground.x = 0.0F;
    fp->input.pl.stick_range.x = (slot == 0u) ? 80 : -80;
    fp->input.pl.stick_range.y = 0;
}

static void ndsFighterMarioFoxStageCollisionLoopReset(void)
{
    gNdsFighterMarioFoxStageCollisionLoopResult = 0u;
    gNdsFighterMarioFoxStageCollisionLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageCollisionLoopMask = 0u;
    gNdsFighterMarioFoxStageCollisionLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageCollisionLoopCount = 0u;
    gNdsStageCollisionLoopPrepared = 0u;
    gNdsStageCollisionLoopBaseStageDrawSeen = 0u;
    gNdsStageCollisionLoopGeometryReady = 0u;
    gNdsStageCollisionLoopGroundDataReady = 0u;
    gNdsStageCollisionLoopYakumonoCount = 0u;
    gNdsStageCollisionLoopMapObjCount = 0u;
    gNdsStageCollisionLoopFloorLineCount = 0u;
    gNdsStageCollisionLoopTotalLineCount = 0u;
    gNdsStageCollisionLoopProjectCallCount = 0u;
    gNdsStageCollisionLoopGeometryProjectCallCount = 0u;
    gNdsStageCollisionLoopLegacyFlatFallbackCount = 0u;
    gNdsStageCollisionLoopNoGeometryCount = 0u;
    gNdsStageCollisionLoopOutOfRangeLineCount = 0u;
    gNdsStageCollisionLoopBadVertexCount = 0u;
    gNdsStageCollisionLoopDivisionGuardCount = 0u;
    gNdsStageCollisionLoopProbeCount = 0u;
    gNdsStageCollisionLoopProbeHitCount = 0u;
    gNdsStageCollisionLoopProbeMissCount = 0u;
    gNdsStageCollisionLoopOffstageMissCount = 0u;
    gNdsStageCollisionLoopBelowFloorMissCount = 0u;
    gNdsStageCollisionLoopP0ProjectCount = 0u;
    gNdsStageCollisionLoopP1ProjectCount = 0u;
    gNdsStageCollisionLoopP0HitCount = 0u;
    gNdsStageCollisionLoopP1HitCount = 0u;
    gNdsStageCollisionLoopP0FloorLineID = -1;
    gNdsStageCollisionLoopP1FloorLineID = -1;
    gNdsStageCollisionLoopP0FloorKind = 0xffffffffu;
    gNdsStageCollisionLoopP1FloorKind = 0xffffffffu;
    gNdsStageCollisionLoopP0FloorLineIsFloor = 0u;
    gNdsStageCollisionLoopP1FloorLineIsFloor = 0u;
    gNdsStageCollisionLoopFloorGroupID = -1;
    gNdsStageCollisionLoopFloorGroupCount = 0u;
    gNdsStageCollisionLoopFloorLineMin = -1;
    gNdsStageCollisionLoopFloorLineMaxExclusive = -1;
    gNdsStageCollisionLoopNonFloorCandidateCount = 0u;
    gNdsStageCollisionLoopYakumonoDObjDeferredCount = 0u;
    gNdsStageCollisionLoopYakumonoDObjUnsafeIndexGuardCount = 0u;
    gNdsStageCollisionLoopP0FloorDistMilli = 0;
    gNdsStageCollisionLoopP1FloorDistMilli = 0;
    gNdsStageCollisionLoopP0FloorFlags = 0u;
    gNdsStageCollisionLoopP1FloorFlags = 0u;
    gNdsStageCollisionLoopP0FloorAngleX1000 = 0;
    gNdsStageCollisionLoopP0FloorAngleY1000 = 0;
    gNdsStageCollisionLoopP1FloorAngleX1000 = 0;
    gNdsStageCollisionLoopP1FloorAngleY1000 = 0;
    gNdsStageCollisionLoopP0EdgeLX = 0;
    gNdsStageCollisionLoopP0EdgeLY = 0;
    gNdsStageCollisionLoopP0EdgeRX = 0;
    gNdsStageCollisionLoopP0EdgeRY = 0;
    gNdsStageCollisionLoopP1EdgeLX = 0;
    gNdsStageCollisionLoopP1EdgeLY = 0;
    gNdsStageCollisionLoopP1EdgeRX = 0;
    gNdsStageCollisionLoopP1EdgeRY = 0;
    gNdsStageCollisionLoopP0RootYFinalMilli = 0;
    gNdsStageCollisionLoopP1RootYFinalMilli = 0;
    gNdsStageCollisionLoopP0FloorYMilli = 0;
    gNdsStageCollisionLoopP1FloorYMilli = 0;
    gNdsStageCollisionLoopP0FloorOK = 0u;
    gNdsStageCollisionLoopP1FloorOK = 0u;
    gNdsStageCollisionLoopGObjDelta = 0u;
    gNdsStageCollisionLoopUnexpectedSceneCount = 0u;
    gNdsStageCollisionLoopUnexpectedStatusCount = 0u;
    gNdsStageCollisionLoopUnsafeFallbackAfterPrepareCount = 0u;
}

static void ndsStageCollisionLoopRecordFinalSlot(u32 slot)
{
    FTStruct *fp;
    DObj *root;
    Vec3f pos;
    Vec3f angle;
    Vec3f left;
    Vec3f right;
    s32 line_id = -1;
    f32 dist = 0.0F;
    f32 root_y;
    u32 flags = 0u;
    sb32 hit;
    sb32 exists;
    s32 floor_y;
    s32 line_kind;
    u32 is_floor;

    if (slot >= 2u)
    {
        return;
    }
    fp = &sNdsFighterStructPool[slot];
    root = fp->joints[nFTPartsJointTopN];
    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) || (root == NULL))
    {
        gNdsStageCollisionLoopUnexpectedStatusCount++;
        return;
    }
    pos = root->translate.vec.f;
    root_y = pos.y;
    pos.y += 512.0F;
    hit = mpCollisionCheckProjectFloor(&pos, &line_id, &dist, &flags,
                                       &angle);
    exists = mpCollisionCheckExistLineID(line_id);
    mpCollisionGetFloorEdgeL(line_id, &left);
    mpCollisionGetFloorEdgeR(line_id, &right);
    floor_y = ndsFloatToMilliSigned(pos.y + dist);
    line_kind = ndsMPGetLineKindForLineID(line_id);
    is_floor = (line_kind == nMPLineKindFloor) ? 1u : 0u;
    if (slot == 0u)
    {
        gNdsStageCollisionLoopP0FloorLineID = line_id;
        gNdsStageCollisionLoopP0FloorKind = (u32)line_kind;
        gNdsStageCollisionLoopP0FloorLineIsFloor = is_floor;
        gNdsStageCollisionLoopP0FloorDistMilli =
            ndsFloatToMilliSigned(dist);
        gNdsStageCollisionLoopP0FloorFlags = flags;
        gNdsStageCollisionLoopP0FloorAngleX1000 =
            ndsFloatToMilliSigned(angle.x);
        gNdsStageCollisionLoopP0FloorAngleY1000 =
            ndsFloatToMilliSigned(angle.y);
        gNdsStageCollisionLoopP0EdgeLX = (s32)left.x;
        gNdsStageCollisionLoopP0EdgeLY = (s32)left.y;
        gNdsStageCollisionLoopP0EdgeRX = (s32)right.x;
        gNdsStageCollisionLoopP0EdgeRY = (s32)right.y;
        gNdsStageCollisionLoopP0RootYFinalMilli =
            ndsFloatToMilliSigned(root_y);
        gNdsStageCollisionLoopP0FloorYMilli = floor_y;
        gNdsStageCollisionLoopP0FloorOK =
            ((hit != FALSE) && (exists != FALSE) &&
             (abs(gNdsStageCollisionLoopP0RootYFinalMilli - floor_y) <=
                1000)) ? 1u : 0u;
    }
    else
    {
        gNdsStageCollisionLoopP1FloorLineID = line_id;
        gNdsStageCollisionLoopP1FloorKind = (u32)line_kind;
        gNdsStageCollisionLoopP1FloorLineIsFloor = is_floor;
        gNdsStageCollisionLoopP1FloorDistMilli =
            ndsFloatToMilliSigned(dist);
        gNdsStageCollisionLoopP1FloorFlags = flags;
        gNdsStageCollisionLoopP1FloorAngleX1000 =
            ndsFloatToMilliSigned(angle.x);
        gNdsStageCollisionLoopP1FloorAngleY1000 =
            ndsFloatToMilliSigned(angle.y);
        gNdsStageCollisionLoopP1EdgeLX = (s32)left.x;
        gNdsStageCollisionLoopP1EdgeLY = (s32)left.y;
        gNdsStageCollisionLoopP1EdgeRX = (s32)right.x;
        gNdsStageCollisionLoopP1EdgeRY = (s32)right.y;
        gNdsStageCollisionLoopP1RootYFinalMilli =
            ndsFloatToMilliSigned(root_y);
        gNdsStageCollisionLoopP1FloorYMilli = floor_y;
        gNdsStageCollisionLoopP1FloorOK =
            ((hit != FALSE) && (exists != FALSE) &&
             (abs(gNdsStageCollisionLoopP1RootYFinalMilli - floor_y) <=
                1000)) ? 1u : 0u;
    }
}

void ndsFighterMarioFoxStageCollisionLoopPrepare(void)
{
    Vec3f probe;
    u32 slot;

    if ((ndsFighterMarioFoxStageCollisionLoopProofEnabled() == FALSE) ||
        (gNdsStageCollisionLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageCollisionLoopReset();
    gNdsStageCollisionLoopPrepared = 1u;
    ndsStageCollisionLoopCountLines();

    ndsStageCollisionLoopSeedSlotOnFloorRange(0u);
    ndsStageCollisionLoopSeedSlotOnFloorRange(1u);
    ndsStageCollisionLoopAdoptRealFloor(0u);
    ndsStageCollisionLoopAdoptRealFloor(1u);

    for (slot = 0u; slot < 2u; slot++)
    {
        FTStruct *fp = &sNdsFighterStructPool[slot];
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) != FALSE) &&
            (root != NULL))
        {
            probe = root->translate.vec.f;
            probe.y += 120.0F;
            ndsStageCollisionLoopRunProbe(probe, 0u);
        }
    }
    probe.x = 0.0F;
    probe.y = 120.0F;
    probe.z = 0.0F;
    ndsStageCollisionLoopRunProbe(probe, 0u);
    probe.x = 100000.0F;
    probe.y = 120.0F;
    probe.z = 0.0F;
    ndsStageCollisionLoopRunProbe(probe, 1u);
    probe.x = 0.0F;
    probe.y = -100000.0F;
    probe.z = 0.0F;
    ndsStageCollisionLoopRunProbe(probe, 2u);
}

void ndsFighterMarioFoxStageCollisionLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageCollisionLoopProofEnabled() == FALSE) ||
        (gNdsStageCollisionLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageCollisionLoopResult != 0u))
    {
        return;
    }
    ndsStageCollisionLoopCountLines();
    if (ndsFighterMarioFoxStageFloorFollowLoopProofEnabled() == FALSE)
    {
        ndsStageCollisionLoopSeedSlotOnFloorRange(0u);
        ndsStageCollisionLoopSeedSlotOnFloorRange(1u);
        ndsStageCollisionLoopAdoptRealFloor(0u);
        ndsStageCollisionLoopAdoptRealFloor(1u);
    }
    ndsStageCollisionLoopRecordFinalSlot(0u);
    ndsStageCollisionLoopRecordFinalSlot(1u);

    if ((gNdsFighterMarioFoxGCDrawAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_PASS) &&
        (gNdsFighterMarioFoxGCDrawAllLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsFighterMarioFoxStageGCDrawAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_GCDRAWALL_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageGCDrawAllLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_GCDRAWALL_LOOP_SAFE_PASS))
    {
        mask |= 1u << 1;
        gNdsStageCollisionLoopBaseStageDrawSeen = 1u;
    }
    if ((gNdsStageCollisionLoopGroundDataReady == 1u) &&
        (gNdsStageCollisionLoopGeometryReady == 1u))
    {
        mask |= 1u << 2;
    }
    if (gNdsStageCollisionLoopFloorLineCount > 0u)
    {
        mask |= 1u << 3;
    }
    if (gNdsStageCollisionLoopGeometryProjectCallCount > 0u)
    {
        mask |= 1u << 4;
    }
    if (gNdsStageCollisionLoopLegacyFlatFallbackCount == 0u)
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageCollisionLoopProbeHitCount >= 3u) &&
        (gNdsStageCollisionLoopOffstageMissCount >= 1u) &&
        (gNdsStageCollisionLoopBelowFloorMissCount >= 1u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageCollisionLoopP0HitCount > 0u) &&
        (gNdsStageCollisionLoopP1HitCount > 0u))
    {
        mask |= 1u << 7;
    }
    if ((mpCollisionCheckExistLineID(gNdsStageCollisionLoopP0FloorLineID) !=
            FALSE) &&
        (mpCollisionCheckExistLineID(gNdsStageCollisionLoopP1FloorLineID) !=
            FALSE) &&
        (mpCollisionGetVertexCountLineID(
            gNdsStageCollisionLoopP0FloorLineID) > 0) &&
        (mpCollisionGetVertexCountLineID(
            gNdsStageCollisionLoopP1FloorLineID) > 0))
    {
        mask |= 1u << 8;
    }
    if ((abs(gNdsStageCollisionLoopP0FloorDistMilli) < 1000000) &&
        (abs(gNdsStageCollisionLoopP1FloorDistMilli) < 1000000) &&
        (gNdsStageCollisionLoopP0FloorAngleY1000 != 0) &&
        (gNdsStageCollisionLoopP1FloorAngleY1000 != 0))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageCollisionLoopP0EdgeLX <=
            gNdsStageCollisionLoopP0EdgeRX) &&
        (gNdsStageCollisionLoopP1EdgeLX <=
            gNdsStageCollisionLoopP1EdgeRX))
    {
        mask |= 1u << 10;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterGCDrawAllLoopP1MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP0FloorOK == 1u) &&
        (gNdsFighterGCDrawAllLoopP1FloorOK == 1u) &&
        (gNdsStageCollisionLoopP0FloorOK == 1u) &&
        (gNdsStageCollisionLoopP1FloorOK == 1u))
    {
        mask |= 1u << 11;
    }
    gNdsStageCollisionLoopGObjDelta = gNdsStageGCDrawAllLoopGObjCountDelta;
    if ((gNdsStageCollisionLoopUnexpectedSceneCount == 0u) &&
        (gNdsStageCollisionLoopUnexpectedStatusCount == 0u) &&
        (gNdsStageCollisionLoopGObjDelta == 0u) &&
        (gNdsStageCollisionLoopNoGeometryCount == 0u) &&
        (gNdsStageCollisionLoopUnsafeFallbackAfterPrepareCount == 0u) &&
        (gNdsStageGCDrawAllLoopManualDisplayCallCount == 0u) &&
        (gNdsStageGCDrawAllLoopUnexpectedSceneCount == 0u))
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageCollisionLoopFloorGroupCount > 0u) &&
        (gNdsStageCollisionLoopFloorLineMin >= 0) &&
        (gNdsStageCollisionLoopFloorLineMaxExclusive >
            gNdsStageCollisionLoopFloorLineMin))
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageCollisionLoopP0FloorKind == (u32)nMPLineKindFloor) &&
        (gNdsStageCollisionLoopP1FloorKind == (u32)nMPLineKindFloor) &&
        (gNdsStageCollisionLoopP0FloorLineIsFloor == 1u) &&
        (gNdsStageCollisionLoopP1FloorLineIsFloor == 1u))
    {
        mask |= 1u << 14;
    }
    if ((gNdsStageCollisionLoopNonFloorCandidateCount == 0u) &&
        (gNdsStageCollisionLoopUnsafeFallbackAfterPrepareCount == 0u))
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageCollisionLoopMask = mask;
    gNdsFighterMarioFoxStageCollisionLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageCollisionLoopCount =
        gNdsFighterMarioFoxStageGCDrawAllLoopCount;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageCollisionLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_COLLISION_LOOP_PASS;
        gNdsFighterMarioFoxStageCollisionLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_COLLISION_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageFloorFollowLoopReset(void)
{
    gNdsFighterMarioFoxStageFloorFollowLoopResult = 0u;
    gNdsFighterMarioFoxStageFloorFollowLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageFloorFollowLoopMask = 0u;
    gNdsFighterMarioFoxStageFloorFollowLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageFloorFollowLoopCount = 0u;
    gNdsStageFloorFollowLoopPrepared = 0u;
    gNdsStageFloorFollowLoopBaseDrawSeen = 0u;
    gNdsStageFloorFollowLoopBaseCollisionSeen = 0u;
    gNdsStageFloorFollowLoopGeometryReady = 0u;
    gNdsStageFloorFollowLoopInitialSeedCount = 0u;
    gNdsStageFloorFollowLoopInitialAdoptCount = 0u;
    gNdsStageFloorFollowLoopFinalRecenterCount = 0u;
    gNdsStageFloorFollowLoopFinalAdoptCount = 0u;
    gNdsStageFloorFollowLoopMapUpdateCount = 0u;
    gNdsStageFloorFollowLoopP0MapUpdateCount = 0u;
    gNdsStageFloorFollowLoopP1MapUpdateCount = 0u;
    gNdsStageFloorFollowLoopProjectCallCount = 0u;
    gNdsStageFloorFollowLoopGeometryHitCount = 0u;
    gNdsStageFloorFollowLoopGeometryMissCount = 0u;
    gNdsStageFloorFollowLoopNoGeometryCount = 0u;
    gNdsStageFloorFollowLoopNonFloorLineCount = 0u;
    gNdsStageFloorFollowLoopClampCount = 0u;
    gNdsStageFloorFollowLoopNoClampCount = 0u;
    gNdsStageFloorFollowLoopP0HitCount = 0u;
    gNdsStageFloorFollowLoopP1HitCount = 0u;
    gNdsStageFloorFollowLoopP0FloorLineID = -1;
    gNdsStageFloorFollowLoopP1FloorLineID = -1;
    gNdsStageFloorFollowLoopP0FloorKind = 0xffffffffu;
    gNdsStageFloorFollowLoopP1FloorKind = 0xffffffffu;
    gNdsStageFloorFollowLoopP0FloorLineIsFloor = 0u;
    gNdsStageFloorFollowLoopP1FloorLineIsFloor = 0u;
    gNdsStageFloorFollowLoopP0InitialRootXMilli = 0;
    gNdsStageFloorFollowLoopP1InitialRootXMilli = 0;
    gNdsStageFloorFollowLoopP0FinalRootXMilli = 0;
    gNdsStageFloorFollowLoopP1FinalRootXMilli = 0;
    gNdsStageFloorFollowLoopP0RootXDeltaMilli = 0;
    gNdsStageFloorFollowLoopP1RootXDeltaMilli = 0;
    gNdsStageFloorFollowLoopP0FinalRootYMilli = 0;
    gNdsStageFloorFollowLoopP1FinalRootYMilli = 0;
    gNdsStageFloorFollowLoopP0FloorYMilli = 0;
    gNdsStageFloorFollowLoopP1FloorYMilli = 0;
    gNdsStageFloorFollowLoopP0FinalDriftMilli = 0;
    gNdsStageFloorFollowLoopP1FinalDriftMilli = 0;
    gNdsStageFloorFollowLoopP0MaxDriftMilli = 0;
    gNdsStageFloorFollowLoopP1MaxDriftMilli = 0;
    gNdsStageFloorFollowLoopMaxDriftMilli = 0;
    gNdsStageFloorFollowLoopP0FloorOK = 0u;
    gNdsStageFloorFollowLoopP1FloorOK = 0u;
    gNdsStageFloorFollowLoopP0FloorVisitMask = 0u;
    gNdsStageFloorFollowLoopP1FloorVisitMask = 0u;
    gNdsStageFloorFollowLoopP0StatusFinal = 0xffffffffu;
    gNdsStageFloorFollowLoopP1StatusFinal = 0xffffffffu;
    gNdsStageFloorFollowLoopP0GAFinal = 0xffffffffu;
    gNdsStageFloorFollowLoopP1GAFinal = 0xffffffffu;
}

static void ndsStageFloorFollowLoopRecordFinalSlot(u32 slot)
{
    FTStruct *fp;
    DObj *root;
    s32 root_x;
    s32 root_y;
    s32 floor_y;
    s32 drift;
    u32 line_kind;
    u32 is_floor;

    if (slot >= 2u)
    {
        return;
    }
    fp = &sNdsFighterStructPool[slot];
    root = fp->joints[nFTPartsJointTopN];
    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) || (root == NULL))
    {
        return;
    }
    root_x = ndsFloatToMilliSigned(root->translate.vec.f.x);
    root_y = ndsFloatToMilliSigned(root->translate.vec.f.y);
    floor_y = ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    drift = root_y - floor_y;
    if (drift < 0)
    {
        drift = -drift;
    }
    line_kind = (u32)ndsMPGetLineKindForLineID(fp->coll_data.floor_line_id);
    is_floor = (line_kind == (u32)nMPLineKindFloor) ? 1u : 0u;

    if (slot == 0u)
    {
        gNdsStageFloorFollowLoopP0FinalRootXMilli = root_x;
        gNdsStageFloorFollowLoopP0RootXDeltaMilli =
            root_x - gNdsStageFloorFollowLoopP0InitialRootXMilli;
        gNdsStageFloorFollowLoopP0FinalRootYMilli = root_y;
        gNdsStageFloorFollowLoopP0FloorYMilli = floor_y;
        gNdsStageFloorFollowLoopP0FinalDriftMilli = drift;
        gNdsStageFloorFollowLoopP0FloorLineID = fp->coll_data.floor_line_id;
        gNdsStageFloorFollowLoopP0FloorKind = line_kind;
        gNdsStageFloorFollowLoopP0FloorLineIsFloor = is_floor;
        gNdsStageFloorFollowLoopP0StatusFinal = (u32)fp->status_id;
        gNdsStageFloorFollowLoopP0GAFinal = (u32)fp->ga;
        gNdsStageFloorFollowLoopP0FloorOK =
            ((fp->ga == nMPKineticsGround) &&
             ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u) &&
             (is_floor != 0u) && (drift <= 1000)) ? 1u : 0u;
    }
    else
    {
        gNdsStageFloorFollowLoopP1FinalRootXMilli = root_x;
        gNdsStageFloorFollowLoopP1RootXDeltaMilli =
            root_x - gNdsStageFloorFollowLoopP1InitialRootXMilli;
        gNdsStageFloorFollowLoopP1FinalRootYMilli = root_y;
        gNdsStageFloorFollowLoopP1FloorYMilli = floor_y;
        gNdsStageFloorFollowLoopP1FinalDriftMilli = drift;
        gNdsStageFloorFollowLoopP1FloorLineID = fp->coll_data.floor_line_id;
        gNdsStageFloorFollowLoopP1FloorKind = line_kind;
        gNdsStageFloorFollowLoopP1FloorLineIsFloor = is_floor;
        gNdsStageFloorFollowLoopP1StatusFinal = (u32)fp->status_id;
        gNdsStageFloorFollowLoopP1GAFinal = (u32)fp->ga;
        gNdsStageFloorFollowLoopP1FloorOK =
            ((fp->ga == nMPKineticsGround) &&
             ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u) &&
             (is_floor != 0u) && (drift <= 1000)) ? 1u : 0u;
    }
}

void ndsFighterMarioFoxStageFloorFollowLoopPrepare(void)
{
    u32 slot;

    if ((ndsFighterMarioFoxStageFloorFollowLoopProofEnabled() == FALSE) ||
        (gNdsStageFloorFollowLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageFloorFollowLoopReset();
    gNdsStageFloorFollowLoopPrepared = 1u;
    gNdsStageFloorFollowLoopBaseCollisionSeen =
        (gNdsStageCollisionLoopPrepared != 0u) ? 1u : 0u;
    gNdsStageFloorFollowLoopGeometryReady =
        (ndsStageCollisionLoopGeometryReady() != FALSE) ? 1u : 0u;
    if (gNdsStageCollisionLoopPrepared != 0u)
    {
        gNdsStageFloorFollowLoopInitialSeedCount = 2u;
    }

    for (slot = 0u; slot < 2u; slot++)
    {
        FTStruct *fp = &sNdsFighterStructPool[slot];
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) || (root == NULL))
        {
            continue;
        }
        if (slot == 0u)
        {
            gNdsStageFloorFollowLoopP0InitialRootXMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.x);
        }
        else
        {
            gNdsStageFloorFollowLoopP1InitialRootXMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.x);
        }
        if ((fp->coll_data.floor_line_id >= 0) &&
            ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u))
        {
            gNdsStageFloorFollowLoopInitialAdoptCount++;
        }
    }
}

static sb32 ndsStageFloorFollowLoopUpdateFighter(GObj *fighter_gobj)
{
    FTStruct *fp;
    DObj *root;
    Vec3f probe;
    Vec3f angle;
    s32 line_id = -1;
    f32 dist = 0.0F;
    u32 flags = 0u;
    f32 floor_y;
    s32 drift_milli;
    s32 pre_drift_milli;
    u32 slot;
    s32 line_kind;
    sb32 hit;

    if ((ndsFighterMarioFoxStageFloorFollowLoopProofEnabled() == FALSE) ||
        (gNdsStageFloorFollowLoopPrepared == 0u))
    {
        return FALSE;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->player >= 2))
    {
        return FALSE;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return FALSE;
    }
    slot = fp->player;
    gNdsStageFloorFollowLoopMapUpdateCount++;
    if (slot == 0u)
    {
        gNdsStageFloorFollowLoopP0MapUpdateCount++;
    }
    else
    {
        gNdsStageFloorFollowLoopP1MapUpdateCount++;
    }
    if (ndsStageCollisionLoopGeometryReady() == FALSE)
    {
        gNdsStageFloorFollowLoopNoGeometryCount++;
        fp->coll_data.is_coll_end = TRUE;
        return FALSE;
    }

    probe = root->translate.vec.f;
    probe.y += 512.0F;
    gNdsStageFloorFollowLoopProjectCallCount++;
    hit = mpCollisionCheckProjectFloor(&probe, &line_id, &dist, &flags,
                                       &angle);
    if (hit == FALSE)
    {
        gNdsStageFloorFollowLoopGeometryMissCount++;
        fp->coll_data.is_coll_end = TRUE;
        return FALSE;
    }
    line_kind = ndsMPGetLineKindForLineID(line_id);
    if (line_kind != nMPLineKindFloor)
    {
        gNdsStageFloorFollowLoopNonFloorLineCount++;
        fp->coll_data.is_coll_end = TRUE;
        return FALSE;
    }

    floor_y = probe.y + dist;
    pre_drift_milli =
        abs(ndsFloatToMilliSigned(root->translate.vec.f.y - floor_y));
    ndsStageFloorEdgeLoopRecordPreClamp(slot, pre_drift_milli);
    drift_milli = 0;
    if (slot == 0u)
    {
        gNdsStageFloorFollowLoopP0HitCount++;
        gNdsStageFloorFollowLoopP0FloorLineID = line_id;
        gNdsStageFloorFollowLoopP0FloorKind = (u32)line_kind;
        gNdsStageFloorFollowLoopP0FloorLineIsFloor = 1u;
        gNdsStageFloorFollowLoopP0FloorYMilli =
            ndsFloatToMilliSigned(floor_y);
        if (drift_milli > gNdsStageFloorFollowLoopP0MaxDriftMilli)
        {
            gNdsStageFloorFollowLoopP0MaxDriftMilli = drift_milli;
        }
        if ((line_id >= 0) && (line_id < 32))
        {
            gNdsStageFloorFollowLoopP0FloorVisitMask |= 1u << line_id;
        }
    }
    else
    {
        gNdsStageFloorFollowLoopP1HitCount++;
        gNdsStageFloorFollowLoopP1FloorLineID = line_id;
        gNdsStageFloorFollowLoopP1FloorKind = (u32)line_kind;
        gNdsStageFloorFollowLoopP1FloorLineIsFloor = 1u;
        gNdsStageFloorFollowLoopP1FloorYMilli =
            ndsFloatToMilliSigned(floor_y);
        if (drift_milli > gNdsStageFloorFollowLoopP1MaxDriftMilli)
        {
            gNdsStageFloorFollowLoopP1MaxDriftMilli = drift_milli;
        }
        if ((line_id >= 0) && (line_id < 32))
        {
            gNdsStageFloorFollowLoopP1FloorVisitMask |= 1u << line_id;
        }
    }
    if (drift_milli > gNdsStageFloorFollowLoopMaxDriftMilli)
    {
        gNdsStageFloorFollowLoopMaxDriftMilli = drift_milli;
    }

    if (pre_drift_milli != 0)
    {
        gNdsStageFloorFollowLoopClampCount++;
    }
    else
    {
        gNdsStageFloorFollowLoopNoClampCount++;
    }
    gNdsStageFloorFollowLoopGeometryHitCount++;
    root->translate.vec.f.y = floor_y;
    fp->coll_data.floor_line_id = line_id;
    fp->coll_data.floor_dist = floor_y;
    fp->coll_data.floor_flags = flags;
    fp->coll_data.floor_angle = angle;
    fp->coll_data.mask_stat |= MAP_FLAG_FLOOR;
    fp->coll_data.is_coll_end = FALSE;
    fp->coll_data.pos_prev = root->translate.vec.f;
    fp->ga = nMPKineticsGround;
    fp->jumps_used = 0;
    fp->physics.vel_air.y = 0.0F;
    fp->vel_air = fp->physics.vel_air;
    ndsStageFloorEdgeLoopRecordFighter(fighter_gobj);
    if ((ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE) &&
        (ndsStageMPProcessFloorLoopUpdateFighter(fighter_gobj) == FALSE))
    {
        gNdsStageMPProcessFloorLoopUnsafeCount++;
    }
    return TRUE;
}

static void ndsFighterMarioFoxStageFloorEdgeLoopReset(void)
{
    gNdsFighterMarioFoxStageFloorEdgeLoopResult = 0u;
    gNdsFighterMarioFoxStageFloorEdgeLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageFloorEdgeLoopMask = 0u;
    gNdsFighterMarioFoxStageFloorEdgeLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageFloorEdgeLoopCount = 0u;
    gNdsStageFloorEdgeLoopPrepared = 0u;
    gNdsStageFloorEdgeLoopGeometryReady = 0u;
    gNdsStageFloorEdgeLoopSelectedLineID = -1;
    gNdsStageFloorEdgeLoopSelectedLineKind = 0xffffffffu;
    gNdsStageFloorEdgeLoopSelectedVertexCount = 0u;
    gNdsStageFloorEdgeLoopLeftXMilli = 0;
    gNdsStageFloorEdgeLoopRightXMilli = 0;
    gNdsStageFloorEdgeLoopWidthMilli = 0;
    gNdsStageFloorEdgeLoopP0StartDistMilli = 0;
    gNdsStageFloorEdgeLoopP1StartDistMilli = 0;
    gNdsStageFloorEdgeLoopP0FinalDistMilli = 0;
    gNdsStageFloorEdgeLoopP1FinalDistMilli = 0;
    gNdsStageFloorEdgeLoopP0DeltaDistMilli = 0;
    gNdsStageFloorEdgeLoopP1DeltaDistMilli = 0;
    gNdsStageFloorEdgeLoopP0MinDistMilli = 0x7fffffff;
    gNdsStageFloorEdgeLoopP1MinDistMilli = 0x7fffffff;
    gNdsStageFloorEdgeLoopP0ApproachOK = 0u;
    gNdsStageFloorEdgeLoopP1ApproachOK = 0u;
    gNdsStageFloorEdgeLoopP0NearEdgeOK = 0u;
    gNdsStageFloorEdgeLoopP1NearEdgeOK = 0u;
    gNdsStageFloorEdgeLoopP0FloorOK = 0u;
    gNdsStageFloorEdgeLoopP1FloorOK = 0u;
    gNdsStageFloorEdgeLoopP0FloorVisitMask = 0u;
    gNdsStageFloorEdgeLoopP1FloorVisitMask = 0u;
    gNdsStageFloorEdgeLoopInsideProbeCount = 0u;
    gNdsStageFloorEdgeLoopInsideProbeHitCount = 0u;
    gNdsStageFloorEdgeLoopOutsideProbeCount = 0u;
    gNdsStageFloorEdgeLoopOutsideProbeMissCount = 0u;
    gNdsStageFloorEdgeLoopOutsideProbeUnexpectedHitCount = 0u;
    gNdsStageFloorEdgeLoopFCCommonCallCount = 0u;
    gNdsStageFloorEdgeLoopFCCommonHitCount = 0u;
    gNdsStageFloorEdgeLoopLineTypeCallCount = 0u;
    gNdsStageFloorEdgeLoopVertexPositionCallCount = 0u;
    gNdsStageFloorEdgeLoopEdgeUnderLCallCount = 0u;
    gNdsStageFloorEdgeLoopEdgeUnderRCallCount = 0u;
    gNdsStageFloorEdgeLoopEdgeUnderDeferredCount = 0u;
    gNdsStageFloorEdgeLoopMapUpdateCount = 0u;
    gNdsStageFloorEdgeLoopP0MapUpdateCount = 0u;
    gNdsStageFloorEdgeLoopP1MapUpdateCount = 0u;
    gNdsStageFloorEdgeLoopPreClampDriftSampleCount = 0u;
    gNdsStageFloorEdgeLoopPreClampCount = 0u;
    gNdsStageFloorEdgeLoopP0MaxPreClampDriftMilli = 0;
    gNdsStageFloorEdgeLoopP1MaxPreClampDriftMilli = 0;
    gNdsStageFloorEdgeLoopMaxPreClampDriftMilli = 0;
    gNdsStageFloorEdgeLoopFinalRecenterCount = 0u;
    gNdsStageFloorEdgeLoopFinalAdoptCount = 0u;
    gNdsStageFloorEdgeLoopUnexpectedSceneCount = 0u;
    gNdsStageFloorEdgeLoopUnexpectedStatusCount = 0u;
    gNdsStageFloorEdgeLoopUnsafeFallbackAfterPrepareCount = 0u;
}

static sb32 ndsStageFloorEdgeLoopFindWidestFloor(Vec3f *left, Vec3f *right,
                                                 s32 *line_id,
                                                 s32 *vertex_count)
{
    s32 first = gNdsStageCollisionLoopFloorLineMin;
    s32 last = gNdsStageCollisionLoopFloorLineMaxExclusive;
    s32 best_line = -1;
    s32 best_vertices = 0;
    f32 best_width = 0.0F;
    Vec3f best_left = { 0.0F, 0.0F, 0.0F };
    Vec3f best_right = { 0.0F, 0.0F, 0.0F };
    s32 id;

    if ((first < 0) || (last <= first))
    {
        return FALSE;
    }
    for (id = first; id < last; id++)
    {
        Vec3f l;
        Vec3f r;
        s32 count = 0;
        f32 width;

        if (ndsMPLineIDIsFloor(id) == FALSE)
        {
            continue;
        }
        if (ndsMPFindLineEndpoints(id, &l, &r, NULL, &count) == FALSE)
        {
            continue;
        }
        width = fabsf(r.x - l.x);
        if ((count >= 2) && (width > best_width))
        {
            best_width = width;
            best_line = id;
            best_vertices = count;
            best_left = l;
            best_right = r;
        }
    }
    if (best_line < 0)
    {
        return FALSE;
    }
    if (left != NULL)
    {
        *left = best_left;
    }
    if (right != NULL)
    {
        *right = best_right;
    }
    if (line_id != NULL)
    {
        *line_id = best_line;
    }
    if (vertex_count != NULL)
    {
        *vertex_count = best_vertices;
    }
    return TRUE;
}

static sb32 ndsStageFloorEdgeLoopFloorYAtX(s32 line_id, f32 x, f32 *floor_y)
{
    Vec3f probe = { x, 100000.0F, 0.0F };
    Vec3f angle;
    f32 dist = 0.0F;
    u32 flags = 0u;

    if (mpCollisionGetFCCommonFloor(line_id, &probe, &dist, &flags,
            &angle) == FALSE)
    {
        return FALSE;
    }
    if (floor_y != NULL)
    {
        *floor_y = probe.y + dist;
    }
    return TRUE;
}

static void ndsStageFloorEdgeLoopSeedSlot(u32 slot, f32 x, s32 line_id)
{
    FTStruct *fp;
    DObj *root;
    f32 floor_y = 0.0F;

    if (slot >= 2u)
    {
        return;
    }
    fp = &sNdsFighterStructPool[slot];
    root = fp->joints[nFTPartsJointTopN];
    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) || (root == NULL) ||
        (ndsStageFloorEdgeLoopFloorYAtX(line_id, x, &floor_y) == FALSE))
    {
        gNdsStageFloorEdgeLoopUnexpectedStatusCount++;
        return;
    }
    root->translate.vec.f.x = x;
    root->translate.vec.f.y = floor_y;
    root->translate.vec.f.z = 0.0F;
    fp->coll_data.floor_line_id = line_id;
    fp->coll_data.floor_dist = floor_y;
    fp->coll_data.mask_stat |= MAP_FLAG_FLOOR;
    fp->coll_data.is_coll_end = FALSE;
    fp->coll_data.pos_prev = root->translate.vec.f;
    fp->ga = nMPKineticsGround;
    fp->jumps_used = 0;
    fp->lr = (slot == 0u) ? +1 : -1;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.x = (slot == 0u) ? 8.0F : -8.0F;
    fp->vel_air = fp->physics.vel_air;
    fp->physics.vel_ground.x = 0.0F;
    fp->vel_ground.x = 0.0F;
    fp->input.pl.stick_range.x = (slot == 0u) ? 80 : -80;
    fp->input.pl.stick_range.y = 0;
}

static void ndsStageFloorEdgeLoopRunProbe(s32 line_id, f32 x, sb32 inside)
{
    Vec3f probe = { x, 100000.0F, 0.0F };
    Vec3f angle;
    f32 dist = 0.0F;
    u32 flags = 0u;
    sb32 hit;

    if (inside != FALSE)
    {
        gNdsStageFloorEdgeLoopInsideProbeCount++;
    }
    else
    {
        gNdsStageFloorEdgeLoopOutsideProbeCount++;
    }
    hit = mpCollisionGetFCCommonFloor(line_id, &probe, &dist, &flags,
                                      &angle);
    if (inside != FALSE)
    {
        if (hit != FALSE)
        {
            gNdsStageFloorEdgeLoopInsideProbeHitCount++;
        }
    }
    else if (hit == FALSE)
    {
        gNdsStageFloorEdgeLoopOutsideProbeMissCount++;
    }
    else
    {
        gNdsStageFloorEdgeLoopOutsideProbeUnexpectedHitCount++;
    }
}

void ndsFighterMarioFoxStageFloorEdgeLoopPrepare(void)
{
    Vec3f left;
    Vec3f right;
    Vec3f vertex;
    s32 line_id = -1;
    s32 vertex_count = 0;
    f32 width;
    f32 margin;
    f32 p0_x;
    f32 p1_x;

    if ((ndsFighterMarioFoxStageFloorEdgeLoopProofEnabled() == FALSE) ||
        (gNdsStageFloorEdgeLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageFloorEdgeLoopReset();
    gNdsStageFloorEdgeLoopPrepared = 1u;
    gNdsStageFloorEdgeLoopGeometryReady =
        (ndsStageCollisionLoopGeometryReady() != FALSE) ? 1u : 0u;
    if (gNdsStageFloorEdgeLoopGeometryReady == 0u)
    {
        gNdsStageFloorEdgeLoopUnsafeFallbackAfterPrepareCount++;
        return;
    }
    if (ndsStageFloorEdgeLoopFindWidestFloor(&left, &right, &line_id,
            &vertex_count) == FALSE)
    {
        gNdsStageFloorEdgeLoopUnexpectedStatusCount++;
        return;
    }
    width = right.x - left.x;
    margin = width * 0.28F;
    if (margin < 128.0F)
    {
        margin = width * 0.5F;
    }
    if (margin > 256.0F)
    {
        margin = 256.0F;
    }
    p0_x = right.x - margin;
    p1_x = left.x + margin;

    gNdsStageFloorEdgeLoopSelectedLineID = line_id;
    gNdsStageFloorEdgeLoopSelectedLineKind = mpCollisionGetLineTypeID(line_id);
    gNdsStageFloorEdgeLoopSelectedVertexCount = (u32)vertex_count;
    gNdsStageFloorEdgeLoopLeftXMilli = ndsFloatToMilliSigned(left.x);
    gNdsStageFloorEdgeLoopRightXMilli = ndsFloatToMilliSigned(right.x);
    gNdsStageFloorEdgeLoopWidthMilli = ndsFloatToMilliSigned(width);

    mpCollisionGetVertexPositionID(line_id, 0, &vertex);
    mpCollisionGetVertexPositionID(line_id, vertex_count - 1, &vertex);
    mpCollisionGetEdgeUnderLLineID(line_id);
    mpCollisionGetEdgeUnderRLineID(line_id);
    ndsStageFloorEdgeLoopRunProbe(line_id, left.x + 16.0F, TRUE);
    ndsStageFloorEdgeLoopRunProbe(line_id, right.x - 16.0F, TRUE);
    ndsStageFloorEdgeLoopRunProbe(line_id, left.x - 16.0F, FALSE);
    ndsStageFloorEdgeLoopRunProbe(line_id, right.x + 16.0F, FALSE);

    ndsStageFloorEdgeLoopSeedSlot(0u, p0_x, line_id);
    ndsStageFloorEdgeLoopSeedSlot(1u, p1_x, line_id);
    gNdsStageFloorEdgeLoopP0StartDistMilli =
        ndsFloatToMilliSigned(right.x - p0_x);
    gNdsStageFloorEdgeLoopP1StartDistMilli =
        ndsFloatToMilliSigned(p1_x - left.x);
    gNdsStageFloorEdgeLoopP0MinDistMilli =
        gNdsStageFloorEdgeLoopP0StartDistMilli;
    gNdsStageFloorEdgeLoopP1MinDistMilli =
        gNdsStageFloorEdgeLoopP1StartDistMilli;
}

static void ndsStageFloorEdgeLoopRecordPreClamp(u32 slot,
                                                s32 pre_drift_milli)
{
    if ((ndsFighterMarioFoxStageFloorEdgeLoopProofEnabled() == FALSE) ||
        (gNdsStageFloorEdgeLoopPrepared == 0u) || (slot >= 2u))
    {
        return;
    }
    gNdsStageFloorEdgeLoopPreClampDriftSampleCount++;
    if (pre_drift_milli != 0)
    {
        gNdsStageFloorEdgeLoopPreClampCount++;
    }
    if (slot == 0u)
    {
        if (pre_drift_milli >
            gNdsStageFloorEdgeLoopP0MaxPreClampDriftMilli)
        {
            gNdsStageFloorEdgeLoopP0MaxPreClampDriftMilli =
                pre_drift_milli;
        }
    }
    else if (pre_drift_milli >
        gNdsStageFloorEdgeLoopP1MaxPreClampDriftMilli)
    {
        gNdsStageFloorEdgeLoopP1MaxPreClampDriftMilli = pre_drift_milli;
    }
    if (pre_drift_milli > gNdsStageFloorEdgeLoopMaxPreClampDriftMilli)
    {
        gNdsStageFloorEdgeLoopMaxPreClampDriftMilli = pre_drift_milli;
    }
}

static void ndsStageFloorEdgeLoopRecordFighter(GObj *fighter_gobj)
{
    FTStruct *fp;
    DObj *root;
    u32 slot;
    s32 dist_milli;

    if ((ndsFighterMarioFoxStageFloorEdgeLoopProofEnabled() == FALSE) ||
        (gNdsStageFloorEdgeLoopPrepared == 0u) ||
        (gNdsStageFloorEdgeLoopSelectedLineID < 0))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->player >= 2))
    {
        return;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return;
    }
    slot = fp->player;
    if (slot == 0u)
    {
        dist_milli = gNdsStageFloorEdgeLoopRightXMilli -
            ndsFloatToMilliSigned(root->translate.vec.f.x);
    }
    else
    {
        dist_milli = ndsFloatToMilliSigned(root->translate.vec.f.x) -
            gNdsStageFloorEdgeLoopLeftXMilli;
    }
    gNdsStageFloorEdgeLoopMapUpdateCount++;
    if (slot == 0u)
    {
        gNdsStageFloorEdgeLoopP0MapUpdateCount++;
        gNdsStageFloorEdgeLoopP0FinalDistMilli = dist_milli;
        if (dist_milli < gNdsStageFloorEdgeLoopP0MinDistMilli)
        {
            gNdsStageFloorEdgeLoopP0MinDistMilli = dist_milli;
        }
        if ((fp->coll_data.floor_line_id >= 0) &&
            (fp->coll_data.floor_line_id < 32))
        {
            gNdsStageFloorEdgeLoopP0FloorVisitMask |=
                1u << fp->coll_data.floor_line_id;
        }
        gNdsStageFloorEdgeLoopP0FloorOK =
            ((fp->ga == nMPKineticsGround) &&
             ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u)) ? 1u : 0u;
    }
    else
    {
        gNdsStageFloorEdgeLoopP1MapUpdateCount++;
        gNdsStageFloorEdgeLoopP1FinalDistMilli = dist_milli;
        if (dist_milli < gNdsStageFloorEdgeLoopP1MinDistMilli)
        {
            gNdsStageFloorEdgeLoopP1MinDistMilli = dist_milli;
        }
        if ((fp->coll_data.floor_line_id >= 0) &&
            (fp->coll_data.floor_line_id < 32))
        {
            gNdsStageFloorEdgeLoopP1FloorVisitMask |=
                1u << fp->coll_data.floor_line_id;
        }
        gNdsStageFloorEdgeLoopP1FloorOK =
            ((fp->ga == nMPKineticsGround) &&
             ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u)) ? 1u : 0u;
    }
}

static void ndsFighterMarioFoxStageMPProcessFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPProcessFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPProcessFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPProcessFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPProcessFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPProcessFloorLoopCount = 0u;
    gNdsStageMPProcessFloorLoopPrepared = 0u;
    gNdsStageMPProcessFloorLoopBaseFloorEdgeSeen = 0u;
    gNdsStageMPProcessFloorLoopAdapterBuildCount = 0u;
    gNdsStageMPProcessFloorLoopAdapterCopyBackCount = 0u;
    gNdsStageMPProcessFloorLoopAdapterFallbackLRCount = 0u;
    gNdsStageMPProcessFloorLoopProjectFloorIDCallCount = 0u;
    gNdsStageMPProcessFloorLoopProjectFloorIDHitCount = 0u;
    gNdsStageMPProcessFloorLoopProjectFloorIDMissCount = 0u;
    gNdsStageMPProcessFloorLoopTestNewCallCount = 0u;
    gNdsStageMPProcessFloorLoopTestNewHitCount = 0u;
    gNdsStageMPProcessFloorLoopTestNewMissCount = 0u;
    gNdsStageMPProcessFloorLoopTestNewEdgeBranchCount = 0u;
    gNdsStageMPProcessFloorLoopTestNewSetProjectCount = 0u;
    gNdsStageMPProcessFloorLoopSetLandingFloorCallCount = 0u;
    gNdsStageMPProcessFloorLoopSetCollideFloorCallCount = 0u;
    gNdsStageMPProcessFloorLoopFCCommonPositiveDistCount = 0u;
    gNdsStageMPProcessFloorLoopFCCommonNegativeDistCount = 0u;
    gNdsStageMPProcessFloorLoopFCCommonZeroDistCount = 0u;
    gNdsStageMPProcessFloorLoopP0UpdateCount = 0u;
    gNdsStageMPProcessFloorLoopP1UpdateCount = 0u;
    gNdsStageMPProcessFloorLoopP0HitCount = 0u;
    gNdsStageMPProcessFloorLoopP1HitCount = 0u;
    gNdsStageMPProcessFloorLoopP0MissCount = 0u;
    gNdsStageMPProcessFloorLoopP1MissCount = 0u;
    gNdsStageMPProcessFloorLoopP0FinalLineID = -1;
    gNdsStageMPProcessFloorLoopP1FinalLineID = -1;
    gNdsStageMPProcessFloorLoopP0FinalLineIsFloor = 0u;
    gNdsStageMPProcessFloorLoopP1FinalLineIsFloor = 0u;
    gNdsStageMPProcessFloorLoopP0FinalMaskStat = 0u;
    gNdsStageMPProcessFloorLoopP1FinalMaskStat = 0u;
    gNdsStageMPProcessFloorLoopP0FinalDistMilli = 0;
    gNdsStageMPProcessFloorLoopP1FinalDistMilli = 0;
    gNdsStageMPProcessFloorLoopP0RootYMilli = 0;
    gNdsStageMPProcessFloorLoopP1RootYMilli = 0;
    gNdsStageMPProcessFloorLoopInsideProbeCount = 0u;
    gNdsStageMPProcessFloorLoopInsideProbeHitCount = 0u;
    gNdsStageMPProcessFloorLoopOutsideProbeCount = 0u;
    gNdsStageMPProcessFloorLoopOutsideProbeMissCount = 0u;
    gNdsStageMPProcessFloorLoopBelowFloorProbeCount = 0u;
    gNdsStageMPProcessFloorLoopBelowFloorPositiveDistCount = 0u;
    gNdsStageMPProcessFloorLoopNoFinalRecenterCount = 0u;
    gNdsStageMPProcessFloorLoopUnexpectedSceneCount = 0u;
    gNdsStageMPProcessFloorLoopUnexpectedStatusCount = 0u;
    gNdsStageMPProcessFloorLoopUnsafeCount = 0u;
}

static void ndsFighterMarioFoxStageMPUpdateFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPUpdateFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopCount = 0u;
    gNdsStageMPUpdateFloorLoopPrepared = 0u;
    gNdsStageMPUpdateFloorLoopBaseMPProcessSeen = 0u;
    gNdsStageMPUpdateFloorLoopAdapterBuildCount = 0u;
    gNdsStageMPUpdateFloorLoopAdapterCopyBackCount = 0u;
    gNdsStageMPUpdateFloorLoopAdapterFallbackLRCount = 0u;
    gNdsStageMPUpdateFloorLoopUpdateMainCallCount = 0u;
    gNdsStageMPUpdateFloorLoopUpdateMainReturnTrueCount = 0u;
    gNdsStageMPUpdateFloorLoopUpdateMainReturnFalseCount = 0u;
    gNdsStageMPUpdateFloorLoopUpdateMainStepCount = 0u;
    gNdsStageMPUpdateFloorLoopUpdateMainMaxStepCount = 0u;
    gNdsStageMPUpdateFloorLoopUpdateMainSplitCount = 0u;
    gNdsStageMPUpdateFloorLoopUpdateMainCapCount = 0u;
    gNdsStageMPUpdateFloorLoopTranslateResetCount = 0u;
    gNdsStageMPUpdateFloorLoopProcCollCallCount = 0u;
    gNdsStageMPUpdateFloorLoopAllCollisionsCallCount = 0u;
    gNdsStageMPUpdateFloorLoopAllCollisionsFloorHitCount = 0u;
    gNdsStageMPUpdateFloorLoopAllCollisionsFloorMissCount = 0u;
    gNdsStageMPUpdateFloorLoopAllCollisionsCliffEdgeBranchCount = 0u;
    gNdsStageMPUpdateFloorLoopAllCollisionsStopEdgeBranchCount = 0u;
    gNdsStageMPUpdateFloorLoopAllCollisionsDefaultEndCount = 0u;
    gNdsStageMPUpdateFloorLoopAllCollisionsWallDeferredCount = 0u;
    gNdsStageMPUpdateFloorLoopAllCollisionsCeilDeferredCount = 0u;
    gNdsStageMPUpdateFloorLoopAllCollisionsFloorEdgeAdjustDeferredCount = 0u;
    gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount = 0u;
    gNdsStageMPUpdateFloorLoopCheckFloorCallCount = 0u;
    gNdsStageMPUpdateFloorLoopCheckCliffEdgeCallCount = 0u;
    gNdsStageMPUpdateFloorLoopCheckFloorHitCount = 0u;
    gNdsStageMPUpdateFloorLoopCheckCliffEdgeHitCount = 0u;
    gNdsStageMPUpdateFloorLoopCheckFloorMissCount = 0u;
    gNdsStageMPUpdateFloorLoopCheckCliffEdgeMissCount = 0u;
    gNdsStageMPUpdateFloorLoopInsideProbeCount = 0u;
    gNdsStageMPUpdateFloorLoopInsideProbeHitCount = 0u;
    gNdsStageMPUpdateFloorLoopOutsideProbeCount = 0u;
    gNdsStageMPUpdateFloorLoopOutsideProbeMissCount = 0u;
    gNdsStageMPUpdateFloorLoopBelowFloorProbeCount = 0u;
    gNdsStageMPUpdateFloorLoopBelowFloorHitCount = 0u;
    gNdsStageMPUpdateFloorLoopSplitProbeCount = 0u;
    gNdsStageMPUpdateFloorLoopSplitProbeStepCount = 0u;
    gNdsStageMPUpdateFloorLoopP0UpdateCount = 0u;
    gNdsStageMPUpdateFloorLoopP1UpdateCount = 0u;
    gNdsStageMPUpdateFloorLoopP0HitCount = 0u;
    gNdsStageMPUpdateFloorLoopP1HitCount = 0u;
    gNdsStageMPUpdateFloorLoopP0MissCount = 0u;
    gNdsStageMPUpdateFloorLoopP1MissCount = 0u;
    gNdsStageMPUpdateFloorLoopP0PosDiffXMilli = 0;
    gNdsStageMPUpdateFloorLoopP1PosDiffXMilli = 0;
    gNdsStageMPUpdateFloorLoopP0PosDiffYMilli = 0;
    gNdsStageMPUpdateFloorLoopP1PosDiffYMilli = 0;
    gNdsStageMPUpdateFloorLoopP0RootXBeforeMilli = 0;
    gNdsStageMPUpdateFloorLoopP1RootXBeforeMilli = 0;
    gNdsStageMPUpdateFloorLoopP0RootXFinalMilli = 0;
    gNdsStageMPUpdateFloorLoopP1RootXFinalMilli = 0;
    gNdsStageMPUpdateFloorLoopP0RootYFinalMilli = 0;
    gNdsStageMPUpdateFloorLoopP1RootYFinalMilli = 0;
    gNdsStageMPUpdateFloorLoopP0FinalLineID = -1;
    gNdsStageMPUpdateFloorLoopP1FinalLineID = -1;
    gNdsStageMPUpdateFloorLoopP0FinalLineIsFloor = 0u;
    gNdsStageMPUpdateFloorLoopP1FinalLineIsFloor = 0u;
    gNdsStageMPUpdateFloorLoopP0FinalMaskStat = 0u;
    gNdsStageMPUpdateFloorLoopP1FinalMaskStat = 0u;
    gNdsStageMPUpdateFloorLoopP0FloorOK = 0u;
    gNdsStageMPUpdateFloorLoopP1FloorOK = 0u;
    gNdsStageMPUpdateFloorLoopNoFinalRecenterCount = 0u;
    gNdsStageMPUpdateFloorLoopFallDeniedCount = 0u;
    gNdsStageMPUpdateFloorLoopOttottoDeniedCount = 0u;
    gNdsStageMPUpdateFloorLoopUnexpectedSceneCount = 0u;
    gNdsStageMPUpdateFloorLoopUnexpectedStatusCount = 0u;
    gNdsStageMPUpdateFloorLoopUnsafeCount = 0u;
    memset(sNdsStageMPUpdateFloorLoopPrevRoot, 0,
           sizeof(sNdsStageMPUpdateFloorLoopPrevRoot));
    memset(sNdsStageMPUpdateFloorLoopPrevRootValid, 0,
           sizeof(sNdsStageMPUpdateFloorLoopPrevRootValid));
}

static s32 sMPProcessMultiWallCollidesNum;
static s32 sMPProcessMultiWallCollideLineIDs[5];

void mpProcessResetMultiWallCount(void)
{
    sMPProcessMultiWallCollidesNum = 0;
}

void mpProcessSetMultiWallLineID(s32 line_id)
{
    s32 i;

    for (i = 0; i < sMPProcessMultiWallCollidesNum; i++)
    {
        if (line_id == sMPProcessMultiWallCollideLineIDs[i])
        {
            return;
        }
    }
    if (sMPProcessMultiWallCollidesNum < ARRAY_COUNT(sMPProcessMultiWallCollideLineIDs))
    {
        sMPProcessMultiWallCollideLineIDs[sMPProcessMultiWallCollidesNum] =
            line_id;
        sMPProcessMultiWallCollidesNum++;
    }
}

static sb32 ndsMPProcessCheckTestWallCollisionAdjNewBounded(
    MPCollData *coll_data, u32 line_kind)
{
    MPObjectColl *map_coll;
    MPObjectColl *p_map_coll;
    Vec3f *translate;
    Vec3f *pos_prev;
    Vec3f sp54;
    Vec3f sp48;
    s32 test_line_id = -1;
    sb32 line_collide;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL) ||
        (coll_data->p_map_coll == NULL))
    {
        return FALSE;
    }

    map_coll = &coll_data->map_coll;
    p_map_coll = coll_data->p_map_coll;
    translate = coll_data->p_translate;
    pos_prev = &coll_data->pos_prev;

    mpProcessResetMultiWallCount();

    if (line_kind == (u32)nMPLineKindLWall)
    {
        coll_data->mask_unk &= (u16)~MAP_FLAG_LWALL;
        coll_data->mask_stat &= (u16)~MAP_FLAG_LWALL;
        sp54.x = pos_prev->x + p_map_coll->width;
        sp48.x = translate->x + map_coll->width;
    }
    else
    {
        coll_data->mask_unk &= (u16)~MAP_FLAG_RWALL;
        coll_data->mask_stat &= (u16)~MAP_FLAG_RWALL;
        sp54.x = pos_prev->x - p_map_coll->width;
        sp48.x = translate->x - map_coll->width;
    }
    sp54.y = pos_prev->y + p_map_coll->center;
    sp54.z = 0.0F;
    sp48.y = translate->y + map_coll->center;
    sp48.z = 0.0F;

    /* ponytail: bounded first-probe slice; add the edge/ceil/floor branches when a verifier needs them. */
    if (line_kind == (u32)nMPLineKindLWall)
    {
        line_collide =
            (coll_data->update_tic != gMPCollisionUpdateTic) ?
            mpCollisionCheckLWallLineCollisionDiff(&sp54, &sp48, NULL,
                                                   &test_line_id, NULL,
                                                   NULL) :
            mpCollisionCheckLWallLineCollisionSame(&sp54, &sp48, NULL,
                                                   &test_line_id, NULL,
                                                   NULL);
    }
    else
    {
        line_collide =
            (coll_data->update_tic != gMPCollisionUpdateTic) ?
            mpCollisionCheckRWallLineCollisionDiff(&sp54, &sp48, NULL,
                                                   &test_line_id, NULL,
                                                   NULL) :
            mpCollisionCheckRWallLineCollisionSame(&sp54, &sp48, NULL,
                                                   &test_line_id, NULL,
                                                   NULL);
    }
    if (line_collide == FALSE)
    {
        return FALSE;
    }
    mpProcessSetMultiWallLineID(test_line_id);
    if (line_kind == (u32)nMPLineKindLWall)
    {
        coll_data->mask_curr |= MAP_FLAG_LWALL;
    }
    else
    {
        coll_data->mask_curr |= MAP_FLAG_RWALL;
    }
    return TRUE;
}

sb32 mpProcessCheckTestLWallCollisionAdjNew(MPCollData *coll_data)
{
    return ndsMPProcessCheckTestWallCollisionAdjNewBounded(coll_data,
                                                           nMPLineKindLWall);
}

sb32 mpProcessCheckTestRWallCollisionAdjNew(MPCollData *coll_data)
{
    return ndsMPProcessCheckTestWallCollisionAdjNewBounded(coll_data,
                                                           nMPLineKindRWall);
}

void mpProcessSetCollProjectFloorID(MPCollData *coll_data)
{
    MPObjectColl *map_coll;
    Vec3f *translate;
    Vec3f sp2C;
    sb32 hit;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        return;
    }
    if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPProcessFloorLoopProjectFloorIDCallCount++;
    }
    map_coll = &coll_data->map_coll;
    translate = coll_data->p_translate;

    sp2C.x = translate->x;
    sp2C.y = translate->y + map_coll->bottom;
    sp2C.z = translate->z;

    hit = mpCollisionCheckProjectFloor(&sp2C, &coll_data->floor_line_id,
                                       &coll_data->floor_dist,
                                       &coll_data->floor_flags,
                                       &coll_data->floor_angle);
    if (hit == FALSE)
    {
        coll_data->floor_line_id = -1;
        if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPProcessFloorLoopProjectFloorIDMissCount++;
        }
    }
    else if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPProcessFloorLoopProjectFloorIDHitCount++;
    }
}

sb32 mpProcessCheckTestFloorCollisionNew(MPCollData *coll_data)
{
    Vec3f *translate;
    s32 wall_line_id;
    Vec3f object_pos;
    sb32 is_wall_edge;
    f32 floor_dist;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        return FALSE;
    }
    if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPProcessFloorLoopTestNewCallCount++;
    }
    translate = coll_data->p_translate;
    coll_data->mask_stat &= (u16)~MAP_FLAG_FLOOR;

    object_pos.x = translate->x;
    object_pos.y = translate->y + coll_data->map_coll.bottom;
    object_pos.z = translate->z;

    if (mpCollisionCheckExistLineID(coll_data->floor_line_id) == FALSE)
    {
        if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPProcessFloorLoopTestNewSetProjectCount++;
        }
        mpProcessSetCollProjectFloorID(coll_data);
        if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPProcessFloorLoopTestNewMissCount++;
        }
        return FALSE;
    }
    if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos,
            &floor_dist, &coll_data->floor_flags,
            &coll_data->floor_angle) != FALSE)
    {
        translate->y += floor_dist;
        coll_data->floor_dist = 0.0F;
        coll_data->mask_stat |= MAP_FLAG_FLOOR;
        if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPProcessFloorLoopTestNewHitCount++;
        }
        return TRUE;
    }
    is_wall_edge = FALSE;
    if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPProcessFloorLoopTestNewEdgeBranchCount++;
    }

    mpCollisionGetFloorEdgeL(coll_data->floor_line_id, &object_pos);
    if (translate->x <= object_pos.x)
    {
        wall_line_id = mpCollisionGetEdgeUnderLLineID(coll_data->floor_line_id);
        if ((wall_line_id != -1) &&
            (mpCollisionGetLineTypeID(wall_line_id) == nMPLineKindRWall))
        {
            is_wall_edge = TRUE;
        }
    }
    else
    {
        mpCollisionGetFloorEdgeR(coll_data->floor_line_id, &object_pos);
        wall_line_id = mpCollisionGetEdgeUnderRLineID(coll_data->floor_line_id);
        if ((wall_line_id != -1) &&
            (mpCollisionGetLineTypeID(wall_line_id) == nMPLineKindLWall))
        {
            is_wall_edge = TRUE;
        }
    }
    translate->y = object_pos.y - coll_data->map_coll.bottom;

    if (is_wall_edge != FALSE)
    {
        translate->x = object_pos.x;
        mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos,
                                    NULL, &coll_data->floor_flags,
                                    &coll_data->floor_angle);
        coll_data->mask_stat |= MAP_FLAG_FLOOR;
        coll_data->floor_dist = 0.0F;
        if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPProcessFloorLoopTestNewHitCount++;
        }
        return TRUE;
    }
    if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPProcessFloorLoopTestNewSetProjectCount++;
        gNdsStageMPProcessFloorLoopTestNewMissCount++;
    }
    mpProcessSetCollProjectFloorID(coll_data);
    return FALSE;
}

static sb32 ndsStageMPSweepFloorLoopChooseLine(s32 except_line_id,
                                               s32 *line_id,
                                               f32 *sample_x,
                                               f32 *sample_y)
{
    s32 min_line = gNdsStageCollisionLoopFloorLineMin;
    s32 max_line = gNdsStageCollisionLoopFloorLineMaxExclusive;
    s32 i;

    if (line_id != NULL)
    {
        *line_id = -1;
    }
    if ((line_id == NULL) || (sample_x == NULL) || (sample_y == NULL) ||
        (min_line < 0) || (max_line <= min_line))
    {
        return FALSE;
    }
    for (i = min_line; i < max_line; i++)
    {
        Vec3f left;
        Vec3f right;

        if ((i == except_line_id) ||
            (ndsMPLineIDIsFloor(i) == FALSE) ||
            (ndsMPFindLineEndpoints(i, &left, &right, NULL, NULL) == FALSE))
        {
            continue;
        }
        if (fabsf(right.x - left.x) < 64.0F)
        {
            continue;
        }
        *sample_x = left.x + ((right.x - left.x) * 0.5F);
        if (ndsStageFloorEdgeLoopFloorYAtX(i, *sample_x, sample_y) == FALSE)
        {
            continue;
        }
        *line_id = i;
        return TRUE;
    }
    return FALSE;
}

static void ndsStageMPStaleFloorLoopInitProbeColl(MPCollData *coll,
                                                  FTStruct *fp,
                                                  Vec3f *translate,
                                                  Vec3f *pos_prev,
                                                  s32 line_id)
{
    memset(coll, 0, sizeof(*coll));
    coll->p_translate = translate;
    coll->p_lr = (fp != NULL) ? &fp->lr : NULL;
    coll->pos_prev = *pos_prev;
    coll->pos_diff.x = translate->x - pos_prev->x;
    coll->pos_diff.y = translate->y - pos_prev->y;
    coll->pos_diff.z = translate->z - pos_prev->z;
    if (fp != NULL)
    {
        coll->map_coll = fp->coll_data.map_coll;
        coll->cliffcatch_coll = fp->coll_data.cliffcatch_coll;
    }
    coll->p_map_coll = &coll->map_coll;
    coll->mask_stat = MAP_FLAG_FLOOR;
    coll->update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    coll->ewall_line_id = -1;
    coll->floor_line_id = line_id;
    coll->ceil_line_id = -1;
    coll->lwall_line_id = -1;
    coll->rwall_line_id = -1;
    coll->cliff_id = -1;
    coll->ignore_line_id = -1;
}

static sb32 ndsStageMPStaleFloorLoopCandidateReachesSecondFloor(
    FTStruct *fp,
    s32 candidate_line_id,
    s32 target_line_id,
    const Vec3f *target_translate,
    const Vec3f *target_pos_prev)
{
    Vec3f translate;
    Vec3f pos_prev;
    MPCollData coll;

    if ((fp == NULL) || (target_translate == NULL) ||
        (target_pos_prev == NULL) || (candidate_line_id < 0) ||
        (target_line_id < 0) || (candidate_line_id == target_line_id))
    {
        return FALSE;
    }

    translate = *target_translate;
    pos_prev = *target_pos_prev;
    ndsStageMPStaleFloorLoopInitProbeColl(&coll, fp, &translate, &pos_prev,
                                           candidate_line_id);

    /*
     * The live proof needs the first source-order floor test to miss while
     * preserving a valid stale saved floor for the original second-floor sweep.
     */
    if (mpProcessCheckTestFloorCollisionNew(&coll) != FALSE)
    {
        return FALSE;
    }
    if ((coll.mask_stat & MAP_FLAG_FLOOR) != 0u)
    {
        return FALSE;
    }
    if (mpProcessCheckTestFloorCollision(&coll, candidate_line_id) == FALSE)
    {
        return FALSE;
    }
    return (coll.floor_line_id == target_line_id) ? TRUE : FALSE;
}

static sb32 ndsStageMPStaleFloorLoopChooseLine(FTStruct *fp,
                                               s32 target_line_id,
                                               const Vec3f *target_translate,
                                               const Vec3f *target_pos_prev,
                                               s32 *line_id,
                                               f32 *sample_x,
                                               f32 *sample_y)
{
    s32 min_line = gNdsStageCollisionLoopFloorLineMin;
    s32 max_line = gNdsStageCollisionLoopFloorLineMaxExclusive;
    s32 i;

    if (line_id != NULL)
    {
        *line_id = -1;
    }
    if ((fp == NULL) || (line_id == NULL) || (sample_x == NULL) ||
        (sample_y == NULL) || (target_translate == NULL) ||
        (target_pos_prev == NULL) || (min_line < 0) ||
        (max_line <= min_line))
    {
        return FALSE;
    }
    for (i = min_line; i < max_line; i++)
    {
        Vec3f left;
        Vec3f right;

        if ((i == target_line_id) ||
            (ndsMPLineIDIsFloor(i) == FALSE) ||
            (ndsMPFindLineEndpoints(i, &left, &right, NULL, NULL) == FALSE))
        {
            continue;
        }
        if (fabsf(right.x - left.x) < 64.0F)
        {
            continue;
        }
        *sample_x = left.x + ((right.x - left.x) * 0.5F);
        if (ndsStageFloorEdgeLoopFloorYAtX(i, *sample_x, sample_y) == FALSE)
        {
            continue;
        }
        if (ndsStageMPStaleFloorLoopCandidateReachesSecondFloor(fp, i,
                target_line_id, target_translate, target_pos_prev) == FALSE)
        {
            continue;
        }
        *line_id = i;
        return TRUE;
    }
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
    s32 min_line = gNdsStageCollisionLoopFloorLineMin;
    s32 max_line = gNdsStageCollisionLoopFloorLineMaxExclusive;
    s32 line_id;

    if ((position == NULL) || (translate == NULL) ||
        (min_line < 0) || (max_line <= min_line))
    {
        return FALSE;
    }
    for (line_id = min_line; line_id < max_line; line_id++)
    {
        Vec3f left;
        Vec3f right;
        Vec3f sweep_position;
        Vec3f sweep_translate;
        f32 floor_y;
        f32 vedge_x = 0.0F;
        f32 vedge_y = 0.0F;
        u32 flags = 0u;
        Vec3f floor_angle;
        u32 yakumono_id = 0u;
        DObj *yakumono_dobj = NULL;
        sb32 is_dynamic = FALSE;
        sb32 hit;

        if (ndsMPLineIDIsFloor(line_id) == FALSE)
        {
            continue;
        }
        gNdsStageMPSweepFloorLoopLineSweepVisitCount++;
        if (ndsMPFindLineEndpoints(line_id, &left, &right, &flags,
                NULL) == FALSE)
        {
            continue;
        }
        sweep_position = *position;
        sweep_translate = *translate;
        if ((ndsMPFindLineYakumonoID(line_id, &yakumono_id) != FALSE) &&
            (gMPCollisionYakumonoDObjs != NULL) &&
            (yakumono_id < NDS_MP_YAKUMONO_DOBJ_SLOTS))
        {
            yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
        }
        if ((yakumono_dobj != NULL) &&
            (yakumono_dobj->user_data.s < nMPYakumonoStatusOff) &&
            ((yakumono_dobj->anim_joint.event32 != NULL) ||
             (yakumono_dobj->user_data.s != nMPYakumonoStatusNone)))
        {
            f32 speed_x = 0.0F;
            f32 speed_y = 0.0F;

            vedge_x = yakumono_dobj->translate.vec.f.x;
            vedge_y = yakumono_dobj->translate.vec.f.y;
            if ((is_diff != FALSE) && (gMPCollisionSpeeds != NULL))
            {
                speed_x = gMPCollisionSpeeds[yakumono_id].x;
                speed_y = gMPCollisionSpeeds[yakumono_id].y;
                if (ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopProofEnabled()
                    != FALSE)
                {
                    gNdsStageMPPlatformSpeedFloorLoopDynamicBranchCount++;
                    gNdsStageMPPlatformSpeedFloorLoopDynamicSpeedXMilli =
                        ndsFloatToMilliSigned(speed_x);
                    gNdsStageMPPlatformSpeedFloorLoopDynamicSpeedYMilli =
                        ndsFloatToMilliSigned(speed_y);
                }
            }
            sweep_position.x = (position->x - vedge_x) + speed_x;
            sweep_position.y = (position->y - vedge_y) + speed_y;
            sweep_translate.x = translate->x - vedge_x;
            sweep_translate.y = translate->y - vedge_y;
            is_dynamic = TRUE;
        }
        if ((sweep_translate.x < left.x) || (sweep_translate.x > right.x))
        {
            continue;
        }
        gNdsStageMPSweepFloorLoopLineSweepCandidateCount++;
        if (ndsStageFloorEdgeLoopFloorYAtX(line_id, sweep_translate.x,
                &floor_y) == FALSE)
        {
            continue;
        }
        ndsMPGetFCAngle(&floor_angle, (s32)left.x, (s32)left.y,
                        (s32)right.x, (s32)right.y, 1);
        if (is_diff != FALSE)
        {
            hit = (((sweep_position.y >= floor_y) &&
                    (sweep_translate.y <= (floor_y + 8.0F))) ||
                   (fabsf(sweep_translate.y - floor_y) <= 64.0F)) ?
                TRUE : FALSE;
        }
        else
        {
            hit = (fabsf(sweep_translate.y - floor_y) <= 128.0F) ?
                TRUE : FALSE;
        }
        if (hit == FALSE)
        {
            continue;
        }
        if (ga_last != NULL)
        {
            ga_last->x = sweep_translate.x + vedge_x;
            ga_last->y = floor_y + vedge_y;
            ga_last->z = translate->z;
        }
        if ((is_dynamic != FALSE) &&
            (ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopProofEnabled() !=
                FALSE))
        {
            gNdsStageMPPlatformSpeedFloorLoopDynamicHitCount++;
            gNdsStageMPPlatformSpeedFloorLoopDynamicLineID = line_id;
            gNdsStageMPPlatformSpeedFloorLoopDynamicYakumonoID = yakumono_id;
            gNdsStageMPPlatformSpeedFloorLoopDynamicGaXMilli =
                ndsFloatToMilliSigned(sweep_translate.x + vedge_x);
            gNdsStageMPPlatformSpeedFloorLoopDynamicGaYMilli =
                ndsFloatToMilliSigned(floor_y + vedge_y);
        }
        if (stand_line_id != NULL)
        {
            *stand_line_id = line_id;
        }
        if (stand_coll_flags != NULL)
        {
            *stand_coll_flags = flags;
        }
        if (angle != NULL)
        {
            *angle = floor_angle;
        }
        return TRUE;
    }
    return FALSE;
}

sb32 mpCollisionCheckFloorLineCollisionSame(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle)
{
    sb32 hit;

    if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPSweepFloorLoopLineSweepSameCallCount++;
    }
    hit = ndsStageMPSweepFloorLoopSweep(position, translate, ga_last,
                                        stand_line_id, stand_coll_flags,
                                        angle, FALSE);
    if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
    {
        if (hit != FALSE)
        {
            gNdsStageMPSweepFloorLoopLineSweepSameHitCount++;
        }
        else
        {
            gNdsStageMPSweepFloorLoopLineSweepSameMissCount++;
        }
    }
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

    if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPSweepFloorLoopLineSweepDiffCallCount++;
    }
    hit = ndsStageMPSweepFloorLoopSweep(position, translate, ga_last,
                                        stand_line_id, stand_coll_flags,
                                        angle, TRUE);
    if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
    {
        if (hit != FALSE)
        {
            gNdsStageMPSweepFloorLoopLineSweepDiffHitCount++;
        }
        else
        {
            gNdsStageMPSweepFloorLoopLineSweepDiffMissCount++;
        }
    }
    return hit;
}

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
    u32 yakumono_count;
    u32 i;
    f32 best_t = 3.402823466e+38F;
    s32 best_line = -1;
    u32 best_flags = 0u;
    Vec3f best_pos = { 0.0F, 0.0F, 0.0F };
    Vec3f best_angle = { 0.0F, -1.0F, 0.0F };

    if ((position == NULL) || (translate == NULL) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);
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
            (yakumono_dobj->user_data.s < nMPYakumonoStatusOff) &&
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
        end = first + count;
        for (line_id = first; line_id < end; line_id++)
        {
            Vec3f left;
            Vec3f right;
            Vec3f sweep_position;
            Vec3f sweep_translate;
            f32 t = 0.0F;
            f32 u = 0.0F;
            f32 hit_x = 0.0F;
            f32 hit_y = 0.0F;
            f32 ceil_y = 0.0F;
            u32 flags = 0u;
            sb32 hit = FALSE;

            if (ndsMPFindLineEndpoints(line_id, &left, &right, &flags,
                    NULL) == FALSE)
            {
                continue;
            }
            sweep_position = *position;
            sweep_translate = *translate;
            if (is_dynamic != FALSE)
            {
                sweep_position.x = (position->x - vedge_x) + speed_x;
                sweep_position.y = (position->y - vedge_y) + speed_y;
                sweep_translate.x = translate->x - vedge_x;
                sweep_translate.y = translate->y - vedge_y;
            }
            gNdsStageMPCeilFloorLoopLineSweepVisitCount++;
            if ((sweep_translate.x < left.x) || (sweep_translate.x > right.x))
            {
                continue;
            }
            gNdsStageMPCeilFloorLoopLineSweepCandidateCount++;
            if (ndsStageMPSegmentIntersection2D(&sweep_position,
                    &sweep_translate, &left, &right, &t, &u, &hit_x,
                    &hit_y) != FALSE)
            {
                hit = TRUE;
            }
            else if (mpCollisionGetFCCommonCeil(line_id, &sweep_translate,
                         &ceil_y, NULL, NULL) != FALSE)
            {
                f32 top_delta = ceil_y;

                hit = (is_diff != FALSE) ?
                    (((sweep_position.y <=
                       (sweep_translate.y + top_delta)) &&
                      (sweep_translate.y >=
                       (sweep_translate.y + top_delta - 8.0F))) ? TRUE :
                        FALSE) :
                    ((fabsf(top_delta) <= 128.0F) ? TRUE : FALSE);
                hit_x = sweep_translate.x;
                hit_y = sweep_translate.y + top_delta;
                t = 1.0F;
            }
            if ((hit == FALSE) || (t >= best_t))
            {
                continue;
            }
            best_t = t;
            best_line = line_id;
            best_flags = flags;
            best_pos.x = hit_x + vedge_x;
            best_pos.y = hit_y + vedge_y;
            best_pos.z = translate->z;
            ndsMPGetFCAngle(&best_angle, (s32)left.x, (s32)left.y,
                            (s32)right.x, (s32)right.y, -1);
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

    if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPCeilFloorLoopLineSweepSameCallCount++;
    }
    hit = ndsStageMPCeilFloorLoopSweep(position, translate, ga_last,
                                       stand_line_id, stand_coll_flags,
                                       angle, FALSE);
    if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
    {
        if (hit != FALSE)
        {
            gNdsStageMPCeilFloorLoopLineSweepSameHitCount++;
        }
        else
        {
            gNdsStageMPCeilFloorLoopLineSweepSameMissCount++;
        }
    }
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

    if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPCeilFloorLoopLineSweepDiffCallCount++;
    }
    hit = ndsStageMPCeilFloorLoopSweep(position, translate, ga_last,
                                       stand_line_id, stand_coll_flags,
                                       angle, TRUE);
    if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
    {
        if (hit != FALSE)
        {
            gNdsStageMPCeilFloorLoopLineSweepDiffHitCount++;
        }
        else
        {
            gNdsStageMPCeilFloorLoopLineSweepDiffMissCount++;
        }
    }
    return hit;
}

sb32 mpProcessCheckTestCeilCollisionAdjNew(MPCollData *coll_data)
{
    MPObjectColl *map_coll;
    MPObjectColl *p_map_coll;
    Vec3f *translate;
    Vec3f sp4C;
    Vec3f sp40;
    sb32 ceil_collide;
    f32 ceil_dist;
    s32 line_id;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPCeilFloorLoopUnsafeCount++;
        }
        return FALSE;
    }
    if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPCeilFloorLoopCheckCallCount++;
    }
    map_coll = &coll_data->map_coll;
    p_map_coll = (coll_data->p_map_coll != NULL) ? coll_data->p_map_coll :
        map_coll;
    translate = coll_data->p_translate;
    coll_data->mask_stat &= (u16)~MAP_FLAG_CEIL;

    sp4C.x = coll_data->pos_prev.x;
    sp4C.y = coll_data->pos_prev.y + p_map_coll->top;
    sp4C.z = coll_data->pos_prev.z;
    sp40.x = translate->x;
    sp40.y = translate->y + map_coll->top;
    sp40.z = translate->z;

    ceil_collide =
        (coll_data->update_tic != gMPCollisionUpdateTic) ?
        mpCollisionCheckCeilLineCollisionDiff(&sp4C, &sp40,
            &coll_data->line_coll_dist, &coll_data->ceil_line_id,
            &coll_data->ceil_flags, &coll_data->ceil_angle) :
        mpCollisionCheckCeilLineCollisionSame(&sp4C, &sp40,
            &coll_data->line_coll_dist, &coll_data->ceil_line_id,
            &coll_data->ceil_flags, &coll_data->ceil_angle);

    if (ceil_collide != FALSE)
    {
        coll_data->mask_curr |= MAP_FLAG_CEIL;
        if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPCeilFloorLoopCheckHitCount++;
            gNdsStageMPCeilFloorLoopMaskCurrCeilCount++;
        }
        return TRUE;
    }
    if ((coll_data->mask_unk & MAP_FLAG_LWALL) != 0u)
    {
        line_id = mpCollisionGetEdgeRightULineID(coll_data->lwall_line_id);
        if ((line_id != -1) &&
            (mpCollisionGetLineTypeID(line_id) == nMPLineKindCeil) &&
            (mpCollisionGetFCCommonCeil(line_id, &sp40, &ceil_dist,
                &coll_data->ceil_flags, &coll_data->ceil_angle) != FALSE) &&
            (ceil_dist < 0.0F))
        {
            coll_data->ceil_line_id = line_id;
            coll_data->mask_curr |= MAP_FLAG_CEIL;
            if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
            {
                gNdsStageMPCeilFloorLoopCheckHitCount++;
                gNdsStageMPCeilFloorLoopMaskCurrCeilCount++;
            }
            return TRUE;
        }
    }
    else if ((coll_data->mask_unk & MAP_FLAG_RWALL) != 0u)
    {
        line_id = mpCollisionGetEdgeLeftULineID(coll_data->rwall_line_id);
        if ((line_id != -1) &&
            (mpCollisionGetLineTypeID(line_id) == nMPLineKindCeil) &&
            (mpCollisionGetFCCommonCeil(line_id, &sp40, &ceil_dist,
                &coll_data->ceil_flags, &coll_data->ceil_angle) != FALSE) &&
            (ceil_dist < 0.0F))
        {
            coll_data->ceil_line_id = line_id;
            coll_data->mask_curr |= MAP_FLAG_CEIL;
            if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
            {
                gNdsStageMPCeilFloorLoopCheckHitCount++;
                gNdsStageMPCeilFloorLoopMaskCurrCeilCount++;
            }
            return TRUE;
        }
    }
    if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPCeilFloorLoopCheckMissCount++;
    }
    return FALSE;
}

void mpProcessRunCeilCollisionAdjNew(MPCollData *coll_data)
{
    MPObjectColl *map_coll;
    Vec3f *translate;
    Vec3f object_pos;
    s32 line_id;
    sb32 is_collide_ceil = FALSE;
    f32 ceil_dist;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPCeilFloorLoopUnsafeCount++;
        }
        return;
    }
    if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPCeilFloorLoopRunAdjustCallCount++;
    }
    map_coll = &coll_data->map_coll;
    translate = coll_data->p_translate;
    object_pos.x = translate->x;
    object_pos.y = translate->y + map_coll->top;
    object_pos.z = translate->z;

    if (mpCollisionGetFCCommonCeil(coll_data->ceil_line_id, &object_pos,
            &ceil_dist, &coll_data->ceil_flags,
            &coll_data->ceil_angle) != FALSE)
    {
        translate->y += ceil_dist;
        coll_data->mask_stat |= MAP_FLAG_CEIL;
        if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPCeilFloorLoopMaskStatCeilCount++;
        }
        return;
    }

    mpCollisionGetCeilEdgeL(coll_data->ceil_line_id, &object_pos);
    if (translate->x <= object_pos.x)
    {
        line_id = mpCollisionGetEdgeUpperLLineID(coll_data->ceil_line_id);
        if ((line_id != -1) &&
            (mpCollisionGetLineTypeID(line_id) == nMPLineKindRWall))
        {
            is_collide_ceil = TRUE;
        }
    }
    else
    {
        mpCollisionGetCeilEdgeR(coll_data->ceil_line_id, &object_pos);
        line_id = mpCollisionGetEdgeUpperRLineID(coll_data->ceil_line_id);
        if ((line_id != -1) &&
            (mpCollisionGetLineTypeID(line_id) == nMPLineKindLWall))
        {
            is_collide_ceil = TRUE;
        }
    }
    translate->y = object_pos.y - map_coll->top;
    if (is_collide_ceil != FALSE)
    {
        translate->x = object_pos.x;
        mpCollisionGetFCCommonCeil(coll_data->ceil_line_id, &object_pos,
                                   NULL, &coll_data->ceil_flags,
                                   &coll_data->ceil_angle);
        coll_data->mask_stat |= MAP_FLAG_CEIL;
        if (ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPCeilFloorLoopMaskStatCeilCount++;
        }
    }
}

sb32 mpProcessCheckTestFloorCollision(MPCollData *coll_data, s32 line_id)
{
    MPObjectColl *map_coll;
    MPObjectColl *p_map_coll;
    Vec3f *translate;
    Vec3f sp4C;
    Vec3f sp40;
    s32 floor_line_id;
    u32 floor_flags = 0u;
    Vec3f floor_angle;
    sb32 hit;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPSweepFloorLoopUnsafeCount++;
        }
        return FALSE;
    }
    if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPSweepFloorLoopCheckFloorCallCount++;
    }
    map_coll = &coll_data->map_coll;
    p_map_coll = (coll_data->p_map_coll != NULL) ? coll_data->p_map_coll :
        map_coll;
    translate = coll_data->p_translate;

    sp4C.x = coll_data->pos_prev.x;
    sp4C.y = coll_data->pos_prev.y + p_map_coll->bottom;
    sp4C.z = coll_data->pos_prev.z;
    sp40.x = translate->x;
    sp40.y = translate->y + map_coll->bottom;
    sp40.z = translate->z;
    floor_line_id = line_id;

    if (coll_data->update_tic != gMPCollisionUpdateTic)
    {
        hit = mpCollisionCheckFloorLineCollisionDiff(&sp4C, &sp40,
                                                     &coll_data->line_coll_dist,
                                                     &floor_line_id,
                                                     &floor_flags,
                                                     &floor_angle);
    }
    else
    {
        hit = mpCollisionCheckFloorLineCollisionSame(&sp4C, &sp40,
                                                     &coll_data->line_coll_dist,
                                                     &floor_line_id,
                                                     &floor_flags,
                                                     &floor_angle);
    }
    if ((hit != FALSE) && (floor_line_id != line_id))
    {
        coll_data->mask_curr |= MAP_FLAG_FLOOR;
        coll_data->floor_line_id = floor_line_id;
        coll_data->floor_flags = floor_flags;
        coll_data->floor_angle = floor_angle;
        if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPSweepFloorLoopCheckFloorHitCount++;
            gNdsStageMPSweepFloorLoopLineSweepAcceptNewLineCount++;
            gNdsStageMPSweepFloorLoopMaskCurrFloorCount++;
        }
        return TRUE;
    }
    if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPSweepFloorLoopCheckFloorMissCount++;
        if (hit != FALSE)
        {
            gNdsStageMPSweepFloorLoopLineSweepRejectSameLineCount++;
        }
    }
    return FALSE;
}

sb32 mpProcessCheckTestLCliffCollision(MPCollData *coll_data)
{
    Vec3f *translate;
    Vec2f *cliffcatch_coll;
    Vec3f *pos_prev;
    Vec3f prev_pos;
    Vec3f object_pos;
    u32 floor_flags = 0u;
    sb32 is_collide_floor;
    sb32 is_cliffcatch_diag_active =
        ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopMapActive != FALSE));

    if ((coll_data == NULL) || (coll_data->p_translate == NULL) ||
        (coll_data->p_lr == NULL))
    {
        if (is_cliffcatch_diag_active != FALSE)
        {
            gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
        }
        return FALSE;
    }
    if (is_cliffcatch_diag_active != FALSE)
    {
        gNdsStageMPCliffCatchFloorLoopLCliffTestCount++;
    }
    if (*coll_data->p_lr != +1)
    {
        return FALSE;
    }

    translate = coll_data->p_translate;
    cliffcatch_coll = &coll_data->cliffcatch_coll;
    pos_prev = &coll_data->pos_prev;

    prev_pos.x = pos_prev->x + cliffcatch_coll->x;
    prev_pos.y = pos_prev->y + cliffcatch_coll->y;
    prev_pos.z = pos_prev->z;
    object_pos.x = translate->x + cliffcatch_coll->x;
    object_pos.y = translate->y + cliffcatch_coll->y;
    object_pos.z = translate->z;

    is_collide_floor =
        (coll_data->update_tic != gMPCollisionUpdateTic) ?
        mpCollisionCheckFloorLineCollisionDiff(&prev_pos, &object_pos,
            &coll_data->line_coll_dist, &coll_data->cliff_id, &floor_flags,
            NULL) :
        mpCollisionCheckFloorLineCollisionSame(&prev_pos, &object_pos,
            &coll_data->line_coll_dist, &coll_data->cliff_id, &floor_flags,
            NULL);

    if ((is_collide_floor != FALSE) &&
        ((floor_flags & MAP_VERTEX_COLL_CLIFF) != 0u) &&
        ((floor_flags & MAP_VERTEX_MAT_MASK) != (u32)nMPMaterial4))
    {
        mpCollisionGetFloorEdgeL(coll_data->cliff_id, &object_pos);

        if ((coll_data->line_coll_dist.x - object_pos.x) < 800.0F)
        {
            coll_data->mask_curr |= MAP_FLAG_LCLIFF;
            coll_data->mask_stat |= MAP_FLAG_LCLIFF;
            if (is_cliffcatch_diag_active != FALSE)
            {
                gNdsStageMPCliffCatchFloorLoopLCliffHitCount++;
            }
            return TRUE;
        }
    }
    return FALSE;
}

sb32 mpProcessCheckTestRCliffCollision(MPCollData *coll_data)
{
    Vec3f *translate;
    Vec2f *cliffcatch_coll;
    Vec3f *pos_prev;
    Vec3f prev_pos;
    Vec3f object_pos;
    u32 floor_flags = 0u;
    sb32 is_collide_floor;
    sb32 is_cliffcatch_diag_active =
        ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopMapActive != FALSE));

    if ((coll_data == NULL) || (coll_data->p_translate == NULL) ||
        (coll_data->p_lr == NULL))
    {
        if (is_cliffcatch_diag_active != FALSE)
        {
            gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
        }
        return FALSE;
    }
    if (is_cliffcatch_diag_active != FALSE)
    {
        gNdsStageMPCliffCatchFloorLoopRCliffTestCount++;
    }
    if (*coll_data->p_lr != -1)
    {
        return FALSE;
    }

    translate = coll_data->p_translate;
    cliffcatch_coll = &coll_data->cliffcatch_coll;
    pos_prev = &coll_data->pos_prev;

    prev_pos.x = pos_prev->x - cliffcatch_coll->x;
    prev_pos.y = pos_prev->y + cliffcatch_coll->y;
    prev_pos.z = pos_prev->z;
    object_pos.x = translate->x - cliffcatch_coll->x;
    object_pos.y = translate->y + cliffcatch_coll->y;
    object_pos.z = translate->z;

    is_collide_floor =
        (coll_data->update_tic != gMPCollisionUpdateTic) ?
        mpCollisionCheckFloorLineCollisionDiff(&prev_pos, &object_pos,
            &coll_data->line_coll_dist, &coll_data->cliff_id, &floor_flags,
            NULL) :
        mpCollisionCheckFloorLineCollisionSame(&prev_pos, &object_pos,
            &coll_data->line_coll_dist, &coll_data->cliff_id, &floor_flags,
            NULL);

    if ((is_collide_floor != FALSE) &&
        ((floor_flags & MAP_VERTEX_COLL_CLIFF) != 0u))
    {
        mpCollisionGetFloorEdgeR(coll_data->cliff_id, &object_pos);

        if ((object_pos.x - coll_data->line_coll_dist.x) < 800.0F)
        {
            coll_data->mask_curr |= MAP_FLAG_RCLIFF;
            coll_data->mask_stat |= MAP_FLAG_RCLIFF;
            if (is_cliffcatch_diag_active != FALSE)
            {
                gNdsStageMPCliffCatchFloorLoopRCliffHitCount++;
            }
            return TRUE;
        }
    }
    return FALSE;
}

void mpProcessSetLandingFloor(MPCollData *coll_data)
{
    MPObjectColl *map_coll;
    Vec3f *translate;
    Vec3f object_pos;
    f32 floor_dist;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        return;
    }
    if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPProcessFloorLoopSetLandingFloorCallCount++;
    }
    if ((ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallLandFloorLoopMapActive != FALSE))
    {
        gNdsStageMPFallLandFloorLoopSetLandingFloorCount++;
    }
    map_coll = &coll_data->map_coll;
    translate = coll_data->p_translate;
    object_pos.x = translate->x;
    object_pos.y = translate->y + map_coll->bottom;
    object_pos.z = translate->z;

    if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos,
            &floor_dist, &coll_data->floor_flags,
            &coll_data->floor_angle) != FALSE)
    {
        translate->y += floor_dist;
    }
    else
    {
        mpCollisionGetFloorEdgeL(coll_data->floor_line_id, &object_pos);
        if (object_pos.x <= translate->x)
        {
            mpCollisionGetFloorEdgeR(coll_data->floor_line_id, &object_pos);
        }
        translate->y = object_pos.y - map_coll->bottom;
        translate->x = object_pos.x;
        mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos,
                                    NULL, &coll_data->floor_flags,
                                    &coll_data->floor_angle);
    }
    coll_data->mask_stat |= MAP_FLAG_FLOOR;
    coll_data->floor_dist = 0.0F;
    if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPSweepFloorLoopLandingFloorCallCount++;
    }
}

void mpProcessSetCollideFloor(MPCollData *coll_data)
{
    MPObjectColl *map_coll;
    Vec3f *translate;
    Vec3f object_pos;
    s32 edge_line_id;
    sb32 is_collide_floor;
    f32 floor_dist;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        return;
    }
    if (ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPProcessFloorLoopSetCollideFloorCallCount++;
    }
    map_coll = &coll_data->map_coll;
    translate = coll_data->p_translate;
    object_pos.x = translate->x;
    object_pos.y = translate->y + map_coll->bottom;
    object_pos.z = translate->z;

    if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos,
            &floor_dist, &coll_data->floor_flags,
            &coll_data->floor_angle) != FALSE)
    {
        translate->y += floor_dist;
        coll_data->mask_stat |= MAP_FLAG_FLOOR;
        coll_data->floor_dist = 0.0F;
        return;
    }
    is_collide_floor = FALSE;
    mpCollisionGetFloorEdgeL(coll_data->floor_line_id, &object_pos);
    if (translate->x <= object_pos.x)
    {
        edge_line_id = mpCollisionGetEdgeUnderLLineID(coll_data->floor_line_id);
        if ((edge_line_id != -1) &&
            (mpCollisionGetLineTypeID(edge_line_id) == nMPLineKindRWall))
        {
            is_collide_floor = TRUE;
        }
    }
    else
    {
        mpCollisionGetFloorEdgeR(coll_data->floor_line_id, &object_pos);
        edge_line_id = mpCollisionGetEdgeUnderRLineID(coll_data->floor_line_id);
        if ((edge_line_id != -1) &&
            (mpCollisionGetLineTypeID(edge_line_id) == nMPLineKindLWall))
        {
            is_collide_floor = TRUE;
        }
    }
    translate->y = object_pos.y - map_coll->bottom;
    if (is_collide_floor != FALSE)
    {
        translate->x = object_pos.x;
        mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos,
                                    NULL, &coll_data->floor_flags,
                                    &coll_data->floor_angle);
        coll_data->mask_stat |= MAP_FLAG_FLOOR;
        coll_data->floor_dist = 0.0F;
    }
}

sb32 mpProcessCheckFloorEdgeCollisionL(MPCollData *coll_data)
{
    MPObjectColl *map_coll;
    Vec3f *translate;
    Vec3f b;
    Vec3f a;
    s32 edge_under_line_id;
    sb32 hit;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPAdjustFloorLoopUnsafeCount++;
        }
        return FALSE;
    }
    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPAdjustFloorLoopCheckLCallCount++;
    }
    map_coll = &coll_data->map_coll;
    translate = coll_data->p_translate;
    edge_under_line_id =
        mpCollisionGetEdgeUnderLLineID(coll_data->floor_line_id);

    b.x = translate->x;
    b.y = translate->y + map_coll->bottom;
    b.z = translate->z;
    a.x = translate->x + map_coll->width;
    a.y = translate->y + map_coll->center;
    a.z = translate->z;

    hit = ((mpCollisionCheckLWallLineCollisionSame(&b, &a, NULL,
                &coll_data->ewall_line_id, NULL, NULL) != FALSE) &&
        (edge_under_line_id != coll_data->ewall_line_id)) ? TRUE : FALSE;
    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        if (hit != FALSE)
        {
            gNdsStageMPAdjustFloorLoopCheckLHitCount++;
        }
        else
        {
            gNdsStageMPAdjustFloorLoopCheckLMissCount++;
        }
    }
    return hit;
}

void mpProcessFloorEdgeLAdjust(MPCollData *coll_data)
{
    MPObjectColl *map_coll;
    Vec3f *translate;
    Vec3f sp44;
    Vec3f sp38;
    f32 floor_dist;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPAdjustFloorLoopUnsafeCount++;
        }
        return;
    }
    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPAdjustFloorLoopAdjustLCallCount++;
    }
    map_coll = &coll_data->map_coll;
    translate = coll_data->p_translate;
    sp38.x = translate->x + map_coll->width;
    sp38.y = translate->y + map_coll->center;
    sp38.z = translate->z;

    if (mpCollisionGetLRCommonLWall(coll_data->ewall_line_id, &sp38,
            NULL, NULL, NULL) != FALSE)
    {
        sp44 = sp38;
        sp44.x += 2.0F * (-coll_data->floor_angle.y * map_coll->width);
        sp44.y += 2.0F * (coll_data->floor_angle.x * map_coll->width);
        if (mpCollisionCheckLWallLineCollisionSame(&sp44, &sp38,
                &coll_data->line_coll_dist, NULL, NULL, NULL) != FALSE)
        {
            sp38.x = coll_data->line_coll_dist.x - map_coll->width;
            sp38.y = translate->y + map_coll->bottom;
            if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id,
                    &sp38, &floor_dist, &coll_data->floor_flags,
                    &coll_data->floor_angle) != FALSE)
            {
                translate->y += floor_dist;
                translate->x = sp38.x;
            }
        }
    }
    else
    {
        mpCollisionGetLWallEdgeU(coll_data->ewall_line_id, &sp44);
        sp44.x -= 2.0F;
        sp38.x = sp44.x - (2.0F * map_coll->width);
        sp38.y = sp44.y - (2.0F * (map_coll->center - map_coll->bottom));
        sp38.z = translate->z;
        if (mpCollisionCheckFloorLineCollisionSame(&sp44, &sp38,
                &coll_data->line_coll_dist, NULL, NULL, NULL) != FALSE)
        {
            sp38.x = coll_data->line_coll_dist.x;
            sp38.y = translate->y;
            if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id,
                    &sp38, &floor_dist, &coll_data->floor_flags,
                    &coll_data->floor_angle) != FALSE)
            {
                translate->y += floor_dist;
                translate->x = sp38.x;
            }
        }
    }
}

sb32 mpProcessCheckFloorEdgeCollisionR(MPCollData *coll_data)
{
    MPObjectColl *map_coll;
    Vec3f *translate;
    Vec3f b;
    Vec3f a;
    s32 edge_under_line_id;
    sb32 hit;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPAdjustFloorLoopUnsafeCount++;
        }
        return FALSE;
    }
    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPAdjustFloorLoopCheckRCallCount++;
    }
    map_coll = &coll_data->map_coll;
    translate = coll_data->p_translate;
    edge_under_line_id =
        mpCollisionGetEdgeUnderRLineID(coll_data->floor_line_id);

    b.x = translate->x;
    b.y = translate->y + map_coll->bottom;
    b.z = translate->z;
    a.x = translate->x - map_coll->width;
    a.y = translate->y + map_coll->center;
    a.z = translate->z;

    hit = ((mpCollisionCheckRWallLineCollisionSame(&b, &a, NULL,
                &coll_data->ewall_line_id, NULL, NULL) != FALSE) &&
        (edge_under_line_id != coll_data->ewall_line_id)) ? TRUE : FALSE;
    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        if (hit != FALSE)
        {
            gNdsStageMPAdjustFloorLoopCheckRHitCount++;
        }
        else
        {
            gNdsStageMPAdjustFloorLoopCheckRMissCount++;
        }
    }
    return hit;
}

void mpProcessFloorEdgeRAdjust(MPCollData *coll_data)
{
    MPObjectColl *map_coll;
    Vec3f *translate;
    Vec3f sp44;
    Vec3f sp38;
    f32 floor_dist;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPAdjustFloorLoopUnsafeCount++;
        }
        return;
    }
    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPAdjustFloorLoopAdjustRCallCount++;
    }
    map_coll = &coll_data->map_coll;
    translate = coll_data->p_translate;
    sp38.x = translate->x - map_coll->width;
    sp38.y = translate->y + map_coll->center;
    sp38.z = translate->z;

    if (mpCollisionGetLRCommonRWall(coll_data->ewall_line_id, &sp38,
            NULL, NULL, NULL) != FALSE)
    {
        sp44 = sp38;
        sp44.x += 2.0F * (coll_data->floor_angle.y * map_coll->width);
        sp44.y += 2.0F * (-coll_data->floor_angle.x * map_coll->width);
        if (mpCollisionCheckRWallLineCollisionSame(&sp44, &sp38,
                &coll_data->line_coll_dist, NULL, NULL, NULL) != FALSE)
        {
            sp38.x = coll_data->line_coll_dist.x + map_coll->width;
            sp38.y = translate->y + map_coll->bottom;
            if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id,
                    &sp38, &floor_dist, &coll_data->floor_flags,
                    &coll_data->floor_angle) != FALSE)
            {
                translate->y += floor_dist;
                translate->x = sp38.x;
            }
        }
    }
    else
    {
        mpCollisionGetRWallEdgeU(coll_data->ewall_line_id, &sp44);
        sp44.x += 2.0F;
        sp38.x = sp44.x + (2.0F * map_coll->width);
        sp38.y = sp44.y - (2.0F * (map_coll->center - map_coll->bottom));
        sp38.z = translate->z;
        if (mpCollisionCheckFloorLineCollisionSame(&sp44, &sp38,
                &coll_data->line_coll_dist, NULL, NULL, NULL) != FALSE)
        {
            sp38.x = coll_data->line_coll_dist.x;
            sp38.y = translate->y;
            if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id,
                    &sp38, &floor_dist, &coll_data->floor_flags,
                    &coll_data->floor_angle) != FALSE)
            {
                translate->y += floor_dist;
                translate->x = sp38.x;
            }
        }
    }
}

void mpProcessRunFloorEdgeAdjust(MPCollData *coll_data)
{
    sb32 hit_l;
    sb32 hit_r;
    s32 live_slot = sNdsStageMPCrossFloorLoopLiveSlot;

    if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPSweepFloorLoopFloorEdgeAdjustCallCount++;
    }
    if (ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() == FALSE)
    {
        if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPSweepFloorLoopFloorEdgeAdjustDeferredCount++;
        }
        return;
    }
    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        gNdsStageMPAdjustFloorLoopUnsafeCount++;
        return;
    }
    gNdsStageMPAdjustFloorLoopRunCallCount++;
    if (live_slot == 0)
    {
        gNdsStageMPAdjustFloorLoopP0RunCount++;
    }
    else if (live_slot == 1)
    {
        gNdsStageMPAdjustFloorLoopP1RunCount++;
    }

    hit_l = mpProcessCheckFloorEdgeCollisionL(coll_data);
    if (hit_l != FALSE)
    {
        mpProcessFloorEdgeLAdjust(coll_data);
    }
    hit_r = mpProcessCheckFloorEdgeCollisionR(coll_data);
    if (hit_r != FALSE)
    {
        mpProcessFloorEdgeRAdjust(coll_data);
    }
    if ((hit_l == FALSE) && (hit_r == FALSE))
    {
        gNdsStageMPAdjustFloorLoopNoAdjustCount++;
    }
    if (live_slot == 0)
    {
        gNdsStageMPAdjustFloorLoopP0FinalLineID =
            coll_data->floor_line_id;
        gNdsStageMPAdjustFloorLoopP0FinalFloorOK =
            (((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u) &&
             (ndsMPLineIDIsFloor(coll_data->floor_line_id) != FALSE)) ?
                1u : 0u;
    }
    else if (live_slot == 1)
    {
        gNdsStageMPAdjustFloorLoopP1FinalLineID =
            coll_data->floor_line_id;
        gNdsStageMPAdjustFloorLoopP1FinalFloorOK =
            (((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u) &&
             (ndsMPLineIDIsFloor(coll_data->floor_line_id) != FALSE)) ?
                1u : 0u;
    }
}

static sb32 ndsStageMPProcessFloorLoopBuildCollData(FTStruct *fp,
                                                    MPCollData *mp_coll)
{
    static s32 sFallbackLR;
    DObj *root;

    if ((fp == NULL) || (mp_coll == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        return FALSE;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return FALSE;
    }
    memset(mp_coll, 0, sizeof(*mp_coll));
    mp_coll->p_translate = &root->translate.vec.f;
    if (fp->coll_data.p_lr != NULL)
    {
        mp_coll->p_lr = fp->coll_data.p_lr;
    }
    else
    {
        sFallbackLR = fp->lr;
        mp_coll->p_lr = &sFallbackLR;
        gNdsStageMPProcessFloorLoopAdapterFallbackLRCount++;
    }
    mp_coll->pos_prev = fp->coll_data.pos_prev;
    mp_coll->map_coll = fp->coll_data.map_coll;
    mp_coll->p_map_coll = &mp_coll->map_coll;
    mp_coll->cliffcatch_coll = fp->coll_data.cliffcatch_coll;
    mp_coll->mask_curr = (u16)fp->coll_data.mask_curr;
    mp_coll->mask_stat = (u16)fp->coll_data.mask_stat;
    mp_coll->update_tic = (u16)fp->coll_data.update_tic;
    mp_coll->ewall_line_id = -1;
    mp_coll->floor_line_id = fp->coll_data.floor_line_id;
    mp_coll->floor_dist = fp->coll_data.floor_dist;
    mp_coll->floor_flags = fp->coll_data.floor_flags;
    mp_coll->floor_angle = fp->coll_data.floor_angle;
    mp_coll->ceil_line_id = -1;
    mp_coll->lwall_line_id = -1;
    mp_coll->rwall_line_id = -1;
    mp_coll->cliff_id = -1;
    mp_coll->ignore_line_id = fp->coll_data.ignore_line_id;
    gNdsStageMPProcessFloorLoopAdapterBuildCount++;
    return TRUE;
}

static void ndsStageMPProcessFloorLoopCopyBack(FTStruct *fp,
                                               MPCollData *mp_coll)
{
    if ((fp == NULL) || (mp_coll == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        return;
    }
    fp->coll_data.pos_prev = mp_coll->pos_prev;
    fp->coll_data.mask_curr = mp_coll->mask_curr;
    fp->coll_data.mask_stat = mp_coll->mask_stat;
    fp->coll_data.is_coll_end = mp_coll->is_coll_end;
    fp->coll_data.floor_line_id = mp_coll->floor_line_id;
    fp->coll_data.floor_dist = mp_coll->floor_dist;
    fp->coll_data.floor_flags = mp_coll->floor_flags;
    fp->coll_data.floor_angle = mp_coll->floor_angle;
    fp->coll_data.cliff_id = mp_coll->cliff_id;
    gNdsStageMPProcessFloorLoopAdapterCopyBackCount++;
}

static void ndsStageMPProcessFloorLoopInitColl(MPCollData *coll,
                                               Vec3f *translate,
                                               s32 line_id)
{
    memset(coll, 0, sizeof(*coll));
    coll->p_translate = translate;
    coll->p_lr = NULL;
    coll->map_coll.bottom = 0.0F;
    coll->p_map_coll = &coll->map_coll;
    coll->floor_line_id = line_id;
    coll->ceil_line_id = -1;
    coll->lwall_line_id = -1;
    coll->rwall_line_id = -1;
    coll->ewall_line_id = -1;
    coll->cliff_id = -1;
    coll->ignore_line_id = -1;
    coll->mask_stat = MAP_FLAG_FLOOR;
}

sb32 mpProcessUpdateMain(MPCollData *coll_data,
                         sb32 (*proc_coll)(MPCollData*, GObj*, u32),
                         GObj *gobj,
                         u32 flags)
{
    Vec3f *translate;
    Vec3f *pos_prev;
    Vec2f diff;
    f32 add_x;
    f32 add_y;
    f32 add_z;
    s32 i;
    s32 update_count;
    sb32 result = FALSE;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL) ||
        (proc_coll == NULL))
    {
        if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPUpdateFloorLoopUnsafeCount++;
        }
        return FALSE;
    }
    if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPUpdateFloorLoopUpdateMainCallCount++;
    }

    translate = coll_data->p_translate;
    pos_prev = &coll_data->pos_prev;
    diff.x = fabsf(translate->x - pos_prev->x);
    diff.y = fabsf(translate->y - pos_prev->y);

    if ((diff.x > 250.0F) || (diff.y > 250.0F))
    {
        update_count = (s32)(((diff.x > diff.y) ? diff.x : diff.y) / 250.0F);
        update_count++;
        if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPUpdateFloorLoopUpdateMainSplitCount++;
        }
    }
    else
    {
        update_count = 1;
    }
    if (update_count > 10)
    {
        update_count = 10;
        if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPUpdateFloorLoopUpdateMainCapCount++;
        }
    }
    if (update_count <= 0)
    {
        update_count = 1;
    }
    add_x = coll_data->pos_diff.x / (f32)update_count;
    add_y = coll_data->pos_diff.y / (f32)update_count;
    add_z = coll_data->pos_diff.z / (f32)update_count;

    *translate = *pos_prev;
    if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPUpdateFloorLoopTranslateResetCount++;
        if ((u32)update_count > gNdsStageMPUpdateFloorLoopUpdateMainMaxStepCount)
        {
            gNdsStageMPUpdateFloorLoopUpdateMainMaxStepCount =
                (u32)update_count;
        }
    }

    for (i = 0; (i < update_count) && (coll_data->is_coll_end == FALSE); i++)
    {
        *pos_prev = *translate;
        if (i == 0)
        {
            translate->x += coll_data->vel_speed.x + coll_data->vel_push.x;
            translate->y += coll_data->vel_speed.y + coll_data->vel_push.y;
            translate->z += coll_data->vel_speed.z + coll_data->vel_push.z;
        }
        translate->x += add_x;
        translate->y += add_y;
        translate->z += add_z;
        if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPUpdateFloorLoopUpdateMainStepCount++;
            gNdsStageMPUpdateFloorLoopProcCollCallCount++;
        }
        result = proc_coll(coll_data, gobj, flags);
    }
    coll_data->update_tic = gMPCollisionUpdateTic;

    if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
    {
        if (result != FALSE)
        {
            gNdsStageMPUpdateFloorLoopUpdateMainReturnTrueCount++;
        }
        else
        {
            gNdsStageMPUpdateFloorLoopUpdateMainReturnFalseCount++;
        }
    }
    return result;
}

sb32 mpCommonRunFighterAllCollisions(MPCollData *coll_data,
                                      GObj *fighter_gobj,
                                      u32 flags)
{
    s32 floor_line_id;
    sb32 is_floor = FALSE;

    if (coll_data == NULL)
    {
        if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPUpdateFloorLoopUnsafeCount++;
        }
        return FALSE;
    }
    floor_line_id = coll_data->floor_line_id;
    (void)fighter_gobj;

    if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPUpdateFloorLoopAllCollisionsCallCount++;
        gNdsStageMPUpdateFloorLoopAllCollisionsWallDeferredCount += 2u;
        gNdsStageMPUpdateFloorLoopAllCollisionsCeilDeferredCount++;
    }
    if (mpProcessCheckTestFloorCollisionNew(coll_data) != FALSE)
    {
        if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
        {
            if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
            {
                gNdsStageMPUpdateFloorLoopAllCollisionsFloorHitCount++;
                gNdsStageMPUpdateFloorLoopAllCollisionsFloorEdgeAdjustDeferredCount++;
            }
            is_floor = TRUE;
        }
    }
    else if ((flags & MAP_PROC_TYPE_CLIFFEDGE) != 0u)
    {
        if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPUpdateFloorLoopAllCollisionsCliffEdgeBranchCount++;
            gNdsStageMPUpdateFloorLoopAllCollisionsFloorMissCount++;
        }
        coll_data->is_coll_end = TRUE;
    }
    else if ((flags & MAP_PROC_TYPE_STOPEDGE) != 0u)
    {
        if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPUpdateFloorLoopAllCollisionsStopEdgeBranchCount++;
            gNdsStageMPUpdateFloorLoopAllCollisionsFloorMissCount++;
        }
        coll_data->is_coll_end = TRUE;
    }
    else
    {
        if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPUpdateFloorLoopAllCollisionsDefaultEndCount++;
            gNdsStageMPUpdateFloorLoopAllCollisionsFloorMissCount++;
        }
        coll_data->is_coll_end = TRUE;
    }
    if (ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE)
    {
        s32 live_slot = sNdsStageMPCrossFloorLoopLiveSlot;
        sb32 is_live_stale_p0 =
            ((ndsFighterMarioFoxStageMPLiveStaleFloorLoopProofEnabled() !=
              FALSE) &&
             (sNdsStageMPLiveStaleFloorLoopProbeActive != FALSE) &&
             (live_slot == 0)) ? TRUE : FALSE;
        sb32 is_stale_live_p0 =
            (((ndsFighterMarioFoxStageMPStaleFloorLoopProofEnabled() !=
               FALSE) ||
              (ndsFighterMarioFoxStageMPLiveStaleFloorLoopProofEnabled() !=
               FALSE)) &&
             ((ndsFighterMarioFoxStageMPLiveStaleFloorLoopProofEnabled() ==
               FALSE) ||
              (sNdsStageMPLiveStaleFloorLoopProbeActive != FALSE)) &&
             (live_slot == 0)) ? TRUE : FALSE;

        gNdsStageMPSweepFloorLoopSecondFloorCallCount++;
        if ((ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() != FALSE) &&
            (live_slot >= 0) && (live_slot < 2))
        {
            gNdsStageMPCrossFloorLoopLiveSecondFloorCallCount++;
        }
        if (is_stale_live_p0 != FALSE)
        {
            gNdsStageMPStaleFloorLoopLiveSecondFloorCallCount++;
        }
        if (is_live_stale_p0 != FALSE)
        {
            gNdsStageMPLiveStaleFloorLoopLiveSecondFloorCallCount++;
        }
        if (mpProcessCheckTestFloorCollision(coll_data, floor_line_id) !=
            FALSE)
        {
            gNdsStageMPSweepFloorLoopSecondFloorHitCount++;
            if ((ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() !=
                    FALSE) &&
                (live_slot >= 0) && (live_slot < 2))
            {
                gNdsStageMPCrossFloorLoopLiveSecondFloorHitCount++;
                if (coll_data->floor_line_id != floor_line_id)
                {
                    gNdsStageMPCrossFloorLoopLiveAcceptedNewLineCount++;
                }
                if (live_slot == 0)
                {
                    gNdsStageMPCrossFloorLoopP0CrossHitCount++;
                }
                else
                {
                    gNdsStageMPCrossFloorLoopP1CrossHitCount++;
                }
            }
            if (is_stale_live_p0 != FALSE)
            {
                gNdsStageMPStaleFloorLoopLiveSecondFloorHitCount++;
                if ((floor_line_id ==
                        gNdsStageMPStaleFloorLoopStaleLineID) &&
                    (coll_data->floor_line_id ==
                        gNdsStageMPStaleFloorLoopTargetLineID))
                {
                    gNdsStageMPStaleFloorLoopLiveAcceptedNewLineCount++;
                }
            }
            if (is_live_stale_p0 != FALSE)
            {
                gNdsStageMPLiveStaleFloorLoopLiveSecondFloorHitCount++;
                if ((floor_line_id ==
                        gNdsStageMPLiveStaleFloorLoopStaleLineID) &&
                    (coll_data->floor_line_id ==
                        gNdsStageMPLiveStaleFloorLoopTargetLineID))
                {
                    gNdsStageMPLiveStaleFloorLoopLiveAcceptedNewLineCount++;
                }
            }
            mpProcessSetLandingFloor(coll_data);
            if ((ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() !=
                    FALSE) &&
                (live_slot >= 0) && (live_slot < 2))
            {
                gNdsStageMPCrossFloorLoopLiveLandingFloorCount++;
            }
            if (is_stale_live_p0 != FALSE)
            {
                gNdsStageMPStaleFloorLoopLiveLandingFloorCount++;
            }
            if (is_live_stale_p0 != FALSE)
            {
                gNdsStageMPLiveStaleFloorLoopLiveLandingFloorCount++;
            }
            if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
            {
                if ((ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() !=
                        FALSE) &&
                    (live_slot >= 0) && (live_slot < 2))
                {
                    gNdsStageMPCrossFloorLoopLiveFloorEdgeAdjustCount++;
                }
                if (is_stale_live_p0 != FALSE)
                {
                    gNdsStageMPStaleFloorLoopLiveFloorEdgeAdjustCount++;
                }
                if (is_live_stale_p0 != FALSE)
                {
                    gNdsStageMPLiveStaleFloorLoopLiveFloorEdgeAdjustCount++;
                }
                mpProcessRunFloorEdgeAdjust(coll_data);
                is_floor = TRUE;
            }
            if ((coll_data->mask_stat & MAP_FLAG_FLOOREDGE) != 0u)
            {
                gNdsStageMPSweepFloorLoopMaskStatFloorEdgeClearCount++;
            }
            coll_data->mask_stat &= (u16)~MAP_FLAG_FLOOREDGE;
            coll_data->is_coll_end = FALSE;
            gNdsStageMPSweepFloorLoopIsCollEndClearCount++;
            if ((ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() !=
                    FALSE) &&
                (live_slot >= 0) && (live_slot < 2))
            {
                gNdsStageMPCrossFloorLoopLiveCollEndClearCount++;
            }
            if (is_stale_live_p0 != FALSE)
            {
                gNdsStageMPStaleFloorLoopLiveCollEndClearCount++;
            }
            if (is_live_stale_p0 != FALSE)
            {
                gNdsStageMPLiveStaleFloorLoopLiveCollEndClearCount++;
            }
        }
        else
        {
            gNdsStageMPSweepFloorLoopSecondFloorMissCount++;
            if ((ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() !=
                    FALSE) &&
                (live_slot >= 0) && (live_slot < 2))
            {
                gNdsStageMPCrossFloorLoopLiveSecondFloorMissCount++;
            }
            if (is_stale_live_p0 != FALSE)
            {
                gNdsStageMPStaleFloorLoopLiveSecondFloorMissCount++;
            }
            if (is_live_stale_p0 != FALSE)
            {
                gNdsStageMPLiveStaleFloorLoopLiveSecondFloorMissCount++;
            }
        }
    }
    else if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount++;
    }
    coll_data->floor_line_id =
        ((coll_data->floor_line_id >= 0) ? coll_data->floor_line_id :
        floor_line_id);
    return is_floor;
}

static sb32 ndsStageMPLiveStaleFloorLoopPrimeMapMovement(FTStruct *fp,
                                                         DObj *root)
{
    Vec3f translate;
    Vec3f pos_prev;
    MPCollData coll;
    s32 stale_line_id = -1;
    s32 target_line_id = -1;
    s32 live_slot_prev;
    sb32 probe_active_prev;
    sb32 hit;

    if ((ndsFighterMarioFoxStageMPLiveStaleFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPLiveStaleFloorLoopPrepared == 0u) ||
        (fp == NULL) || (fp->player != 0) ||
        (gNdsStageMPLiveStaleFloorLoopSelectedCallbackCount != 0u))
    {
        return FALSE;
    }
    (void)root;

    gNdsStageMPLiveStaleFloorLoopPrimeAttemptCount++;
    gNdsStageMPStaleFloorLoopPrimeAttemptCount++;
    probe_active_prev = sNdsStageMPLiveStaleFloorLoopProbeActive;
    sNdsStageMPLiveStaleFloorLoopProbeActive = TRUE;
    if (ndsStageMPStaleFloorLoopChoosePair(fp, &stale_line_id,
            &target_line_id, &translate, &pos_prev) == FALSE)
    {
        sNdsStageMPLiveStaleFloorLoopProbeActive = probe_active_prev;
        gNdsStageMPLiveStaleFloorLoopPrimeMissCount++;
        gNdsStageMPLiveStaleFloorLoopUnsafeCount++;
        gNdsStageMPStaleFloorLoopPrimeMissCount++;
        gNdsStageMPStaleFloorLoopUnsafeCount++;
        return FALSE;
    }
    sNdsStageMPLiveStaleFloorLoopProbeActive = probe_active_prev;

    gNdsStageMPLiveStaleFloorLoopPrimeHitCount++;
    gNdsStageMPLiveStaleFloorLoopStaleLineID = stale_line_id;
    gNdsStageMPLiveStaleFloorLoopTargetLineID = target_line_id;
    gNdsStageMPLiveStaleFloorLoopTargetXMilli =
        ndsFloatToMilliSigned(translate.x);
    gNdsStageMPLiveStaleFloorLoopTargetYMilli =
        ndsFloatToMilliSigned(translate.y + fp->coll_data.map_coll.bottom);
    gNdsStageMPLiveStaleFloorLoopSelectedCallbackCount++;

    gNdsStageMPStaleFloorLoopPrimeHitCount++;
    gNdsStageMPStaleFloorLoopStaleLineID = stale_line_id;
    gNdsStageMPStaleFloorLoopTargetLineID = target_line_id;
    gNdsStageMPStaleFloorLoopTargetXMilli =
        gNdsStageMPLiveStaleFloorLoopTargetXMilli;
    gNdsStageMPStaleFloorLoopTargetYMilli =
        gNdsStageMPLiveStaleFloorLoopTargetYMilli;

    gNdsStageMPCrossFloorLoopPrimeAttemptCount++;
    gNdsStageMPCrossFloorLoopPrimeHitCount++;
    gNdsStageMPCrossFloorLoopSourceLineID = stale_line_id;
    gNdsStageMPCrossFloorLoopTargetLineID = target_line_id;
    gNdsStageMPCrossFloorLoopTargetXMilli =
        gNdsStageMPLiveStaleFloorLoopTargetXMilli;
    gNdsStageMPCrossFloorLoopTargetYMilli =
        gNdsStageMPLiveStaleFloorLoopTargetYMilli;

    ndsStageMPStaleFloorLoopInitProbeColl(&coll, fp, &translate, &pos_prev,
                                           stale_line_id);
    live_slot_prev = sNdsStageMPCrossFloorLoopLiveSlot;
    sNdsStageMPCrossFloorLoopLiveSlot = 0;
    sNdsStageMPLiveStaleFloorLoopProbeActive = TRUE;
    hit = mpProcessUpdateMain(&coll, mpCommonRunFighterAllCollisions,
                              fp->fighter_gobj, MAP_PROC_TYPE_CLIFFEDGE);
    sNdsStageMPLiveStaleFloorLoopProbeActive = probe_active_prev;
    sNdsStageMPCrossFloorLoopLiveSlot = live_slot_prev;

    gNdsStageMPLiveStaleFloorLoopP0FinalLineID = coll.floor_line_id;
    gNdsStageMPLiveStaleFloorLoopP0FinalFloorOK =
        ((hit != FALSE) && (coll.floor_line_id == target_line_id) &&
         ((coll.mask_stat & MAP_FLAG_FLOOR) != 0u) &&
         (ndsMPLineIDIsFloor(coll.floor_line_id) != FALSE)) ? 1u : 0u;
    if (coll.floor_line_id == target_line_id)
    {
        gNdsStageMPLiveStaleFloorLoopP0TargetLineMatchCount++;
    }
    gNdsStageMPLiveStaleFloorLoopP1FinalLineID =
        gNdsStageMPUpdateFloorLoopP1FinalLineID;
    gNdsStageMPLiveStaleFloorLoopP1FinalFloorOK =
        gNdsStageMPUpdateFloorLoopP1FloorOK;

    gNdsStageMPStaleFloorLoopP0FinalLineID = coll.floor_line_id;
    gNdsStageMPStaleFloorLoopP0FinalFloorOK =
        gNdsStageMPLiveStaleFloorLoopP0FinalFloorOK;
    if (coll.floor_line_id == target_line_id)
    {
        gNdsStageMPStaleFloorLoopP0TargetLineMatchCount++;
    }
    gNdsStageMPStaleFloorLoopP1FinalLineID =
        gNdsStageMPUpdateFloorLoopP1FinalLineID;
    gNdsStageMPStaleFloorLoopP1FinalFloorOK =
        gNdsStageMPUpdateFloorLoopP1FloorOK;

    if ((hit == FALSE) ||
        (gNdsStageMPLiveStaleFloorLoopP0FinalFloorOK == 0u))
    {
        gNdsStageMPLiveStaleFloorLoopUnsafeCount++;
        gNdsStageMPStaleFloorLoopUnsafeCount++;
    }
    return FALSE;
}

static sb32 ndsStageMPMotionStaleFloorLoopPrimeMapMovement(FTStruct *fp,
                                                           DObj *root)
{
    Vec3f translate;
    Vec3f pos_prev;
    s32 stale_line_id = -1;
    s32 target_line_id = -1;
    sb32 probe_active_prev;

    if ((ndsFighterMarioFoxStageMPMotionStaleFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPMotionStaleFloorLoopPrepared == 0u) ||
        (fp == NULL) || (root == NULL) || (fp->player != 0))
    {
        return FALSE;
    }

    if (gNdsStageMPMotionStaleFloorLoopMutationCount != 0u)
    {
        return TRUE;
    }

    gNdsStageMPMotionStaleFloorLoopPrimeAttemptCount++;
    gNdsStageMPLiveStaleFloorLoopPrimeAttemptCount++;
    gNdsStageMPStaleFloorLoopPrimeAttemptCount++;
    gNdsStageMPCrossFloorLoopPrimeAttemptCount++;

    probe_active_prev = sNdsStageMPLiveStaleFloorLoopProbeActive;
    sNdsStageMPLiveStaleFloorLoopProbeActive = TRUE;
    if (ndsStageMPStaleFloorLoopChoosePair(fp, &stale_line_id,
            &target_line_id, &translate, &pos_prev) == FALSE)
    {
        sNdsStageMPLiveStaleFloorLoopProbeActive = probe_active_prev;
        gNdsStageMPMotionStaleFloorLoopPrimeMissCount++;
        gNdsStageMPMotionStaleFloorLoopUnsafeCount++;
        gNdsStageMPLiveStaleFloorLoopPrimeMissCount++;
        gNdsStageMPLiveStaleFloorLoopUnsafeCount++;
        gNdsStageMPStaleFloorLoopPrimeMissCount++;
        gNdsStageMPStaleFloorLoopUnsafeCount++;
        gNdsStageMPCrossFloorLoopPrimeMissCount++;
        gNdsStageMPCrossFloorLoopUnsafeCount++;
        return FALSE;
    }
    sNdsStageMPLiveStaleFloorLoopProbeActive = probe_active_prev;

    gNdsStageMPMotionStaleFloorLoopPrimeHitCount++;
    gNdsStageMPMotionStaleFloorLoopStaleLineID = stale_line_id;
    gNdsStageMPMotionStaleFloorLoopTargetLineID = target_line_id;
    gNdsStageMPMotionStaleFloorLoopTargetXMilli =
        ndsFloatToMilliSigned(translate.x);
    gNdsStageMPMotionStaleFloorLoopTargetYMilli =
        ndsFloatToMilliSigned(translate.y + fp->coll_data.map_coll.bottom);
    gNdsStageMPMotionStaleFloorLoopP0PrevXMilli =
        ndsFloatToMilliSigned(pos_prev.x);
    gNdsStageMPMotionStaleFloorLoopP0PrevYMilli =
        ndsFloatToMilliSigned(pos_prev.y + fp->coll_data.map_coll.bottom);
    gNdsStageMPMotionStaleFloorLoopP0TargetXMilli =
        gNdsStageMPMotionStaleFloorLoopTargetXMilli;
    gNdsStageMPMotionStaleFloorLoopP0TargetYMilli =
        gNdsStageMPMotionStaleFloorLoopTargetYMilli;

    gNdsStageMPLiveStaleFloorLoopPrimeHitCount++;
    gNdsStageMPLiveStaleFloorLoopStaleLineID = stale_line_id;
    gNdsStageMPLiveStaleFloorLoopTargetLineID = target_line_id;
    gNdsStageMPLiveStaleFloorLoopTargetXMilli =
        gNdsStageMPMotionStaleFloorLoopTargetXMilli;
    gNdsStageMPLiveStaleFloorLoopTargetYMilli =
        gNdsStageMPMotionStaleFloorLoopTargetYMilli;
    gNdsStageMPLiveStaleFloorLoopSelectedCallbackCount++;

    gNdsStageMPStaleFloorLoopPrimeHitCount++;
    gNdsStageMPStaleFloorLoopStaleLineID = stale_line_id;
    gNdsStageMPStaleFloorLoopTargetLineID = target_line_id;
    gNdsStageMPStaleFloorLoopTargetXMilli =
        gNdsStageMPMotionStaleFloorLoopTargetXMilli;
    gNdsStageMPStaleFloorLoopTargetYMilli =
        gNdsStageMPMotionStaleFloorLoopTargetYMilli;

    gNdsStageMPCrossFloorLoopPrimeHitCount++;
    gNdsStageMPCrossFloorLoopSourceLineID = stale_line_id;
    gNdsStageMPCrossFloorLoopTargetLineID = target_line_id;
    gNdsStageMPCrossFloorLoopTargetXMilli =
        gNdsStageMPMotionStaleFloorLoopTargetXMilli;
    gNdsStageMPCrossFloorLoopTargetYMilli =
        gNdsStageMPMotionStaleFloorLoopTargetYMilli;

    fp->coll_data.pos_prev = pos_prev;
    fp->coll_data.floor_line_id = stale_line_id;
    fp->coll_data.mask_stat |= MAP_FLAG_FLOOR;
    fp->coll_data.is_coll_end = FALSE;
    fp->coll_data.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    root->translate.vec.f = translate;
    fp->ga = nMPKineticsGround;

    gNdsStageMPMotionStaleFloorLoopMutationCount++;
    return TRUE;
}

static void ndsStageMPUpdateFloorLoopPrimeMapMovement(GObj *fighter_gobj)
{
    FTStruct *fp;
    DObj *root;
    Vec3f left;
    Vec3f right;
    Vec3f probe;
    Vec3f angle;
    f32 dist = 0.0F;
    f32 min_x;
    f32 max_x;
    f32 margin = 96.0F;
    f32 dx;
    f32 mag;
    f32 next_x;
    s32 line_id;
    u32 flags = 0u;

    if ((fighter_gobj == NULL) ||
        (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPUpdateFloorLoopPrepared == 0u))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->player >= 2))
    {
        gNdsStageMPUpdateFloorLoopUnsafeCount++;
        return;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        gNdsStageMPUpdateFloorLoopUnsafeCount++;
        return;
    }
    if (ndsStageMPMotionStaleFloorLoopPrimeMapMovement(fp, root) != FALSE)
    {
        return;
    }
    if (ndsStageMPLiveStaleFloorLoopPrimeMapMovement(fp, root) != FALSE)
    {
        return;
    }

    line_id = gNdsStageFloorEdgeLoopSelectedLineID;
    if (line_id < 0)
    {
        line_id = fp->coll_data.floor_line_id;
    }
    if ((line_id < 0) ||
        (ndsMPFindLineEndpoints(line_id, &left, &right, NULL, NULL) ==
            FALSE) ||
        (ndsMPLineIDIsFloor(line_id) == FALSE))
    {
        gNdsStageMPUpdateFloorLoopUnsafeCount++;
        return;
    }

    min_x = (left.x < right.x) ? left.x : right.x;
    max_x = (left.x > right.x) ? left.x : right.x;
    if ((max_x - min_x) < (margin * 2.0F))
    {
        margin = 16.0F;
    }

    dx = fp->physics.vel_air.x;
    if (dx == 0.0F)
    {
        dx = (fp->lr * fp->coll_data.floor_angle.y *
              fp->physics.vel_ground.x) + fp->physics.vel_jostle_x;
    }
    if (dx == 0.0F)
    {
        dx = fp->vel_air.x;
    }
    if (dx == 0.0F)
    {
        dx = (fp->lr * fp->coll_data.floor_angle.y * fp->vel_ground.x) +
            fp->vel_push.x;
    }
    if (dx == 0.0F)
    {
        dx = (fp->lr >= 0) ? 1.0F : -1.0F;
    }
    mag = ABSF(dx);
    if (mag < 1.0F)
    {
        mag = 1.0F;
    }
    if (mag > 24.0F)
    {
        mag = 24.0F;
    }
    dx = (dx >= 0.0F) ? mag : -mag;

    if (root->translate.vec.f.x <= (min_x + margin))
    {
        dx = mag;
    }
    else if (root->translate.vec.f.x >= (max_x - margin))
    {
        dx = -mag;
    }
    next_x = root->translate.vec.f.x + dx;
    if (next_x <= (min_x + margin))
    {
        next_x = min_x + margin;
    }
    else if (next_x >= (max_x - margin))
    {
        next_x = max_x - margin;
    }
    if (next_x == root->translate.vec.f.x)
    {
        next_x += ((root->translate.vec.f.x < ((min_x + max_x) * 0.5F)) ?
            1.0F : -1.0F);
    }

    probe.x = next_x;
    probe.y = 100000.0F;
    probe.z = 0.0F;
    if (mpCollisionGetFCCommonFloor(line_id, &probe, &dist, &flags,
            &angle) == FALSE)
    {
        gNdsStageMPUpdateFloorLoopUnsafeCount++;
        return;
    }

    fp->coll_data.pos_prev = root->translate.vec.f;
    root->translate.vec.f.x = next_x;
    root->translate.vec.f.y = probe.y + dist;
    root->translate.vec.f.z = 0.0F;
    fp->coll_data.floor_line_id = line_id;
    fp->coll_data.floor_dist = root->translate.vec.f.y;
    fp->coll_data.floor_flags = flags;
    fp->coll_data.floor_angle = angle;
    fp->coll_data.mask_stat |= MAP_FLAG_FLOOR;
    fp->coll_data.is_coll_end = FALSE;
    if ((ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() != FALSE) &&
        (fp->player == 0))
    {
        s32 source_line_id = -1;

        gNdsStageMPCrossFloorLoopPrimeAttemptCount++;
        gNdsStageMPCrossFloorLoopPrimeHitCount++;
        gNdsStageMPCrossFloorLoopSourceLineID = source_line_id;
        gNdsStageMPCrossFloorLoopTargetLineID = line_id;
        gNdsStageMPCrossFloorLoopTargetXMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsStageMPCrossFloorLoopTargetYMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.y +
                fp->coll_data.map_coll.bottom);
        fp->coll_data.floor_line_id = source_line_id;
        fp->coll_data.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    }
    fp->ga = nMPKineticsGround;
}

static sb32 ndsStageMPUpdateFloorLoopBuildCollData(FTStruct *fp,
                                                   MPCollData *mp_coll)
{
    DObj *root;

    if (ndsStageMPProcessFloorLoopBuildCollData(fp, mp_coll) == FALSE)
    {
        return FALSE;
    }
    root = fp->joints[nFTPartsJointTopN];
    mp_coll->pos_prev = fp->coll_data.pos_prev;
    mp_coll->pos_diff.x = root->translate.vec.f.x - mp_coll->pos_prev.x;
    mp_coll->pos_diff.y = root->translate.vec.f.y - mp_coll->pos_prev.y;
    mp_coll->pos_diff.z = root->translate.vec.f.z - mp_coll->pos_prev.z;
    mp_coll->vel_speed.x = 0.0F;
    mp_coll->vel_speed.y = 0.0F;
    mp_coll->vel_speed.z = 0.0F;
    mp_coll->vel_push.x = 0.0F;
    mp_coll->vel_push.y = 0.0F;
    mp_coll->vel_push.z = 0.0F;
    gNdsStageMPUpdateFloorLoopAdapterBuildCount++;
    if (mp_coll->p_lr == NULL)
    {
        gNdsStageMPUpdateFloorLoopAdapterFallbackLRCount++;
    }
    if (fp->player == 0)
    {
        gNdsStageMPUpdateFloorLoopP0PosDiffXMilli =
            ndsFloatToMilliSigned(mp_coll->pos_diff.x);
        gNdsStageMPUpdateFloorLoopP0PosDiffYMilli =
            ndsFloatToMilliSigned(mp_coll->pos_diff.y);
        gNdsStageMPUpdateFloorLoopP0RootXBeforeMilli =
            ndsFloatToMilliSigned(mp_coll->pos_prev.x);
    }
    else if (fp->player == 1)
    {
        gNdsStageMPUpdateFloorLoopP1PosDiffXMilli =
            ndsFloatToMilliSigned(mp_coll->pos_diff.x);
        gNdsStageMPUpdateFloorLoopP1PosDiffYMilli =
            ndsFloatToMilliSigned(mp_coll->pos_diff.y);
        gNdsStageMPUpdateFloorLoopP1RootXBeforeMilli =
            ndsFloatToMilliSigned(mp_coll->pos_prev.x);
    }
    return TRUE;
}

static void ndsStageMPUpdateFloorLoopMirrorBaseFighter(FTStruct *fp,
                                                       sb32 hit)
{
    DObj *root;
    u32 slot;
    u32 line_kind;
    u32 is_floor;

    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->player >= 2))
    {
        return;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return;
    }
    slot = fp->player;
    line_kind = (u32)ndsMPGetLineKindForLineID(fp->coll_data.floor_line_id);
    is_floor = (line_kind == (u32)nMPLineKindFloor) ? 1u : 0u;

    gNdsStageFloorFollowLoopMapUpdateCount++;
    gNdsStageFloorFollowLoopGeometryHitCount++;
    gNdsStageFloorFollowLoopNoClampCount++;
    if (slot == 0u)
    {
        gNdsStageFloorFollowLoopP0MapUpdateCount++;
        gNdsStageFloorFollowLoopP0HitCount += (hit != FALSE) ? 1u : 0u;
        gNdsStageFloorFollowLoopP0FloorLineID = fp->coll_data.floor_line_id;
        gNdsStageFloorFollowLoopP0FloorKind = line_kind;
        gNdsStageFloorFollowLoopP0FloorLineIsFloor = is_floor;
        gNdsStageFloorFollowLoopP0FloorYMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.y);
        if ((fp->coll_data.floor_line_id >= 0) &&
            (fp->coll_data.floor_line_id < 32))
        {
            gNdsStageFloorFollowLoopP0FloorVisitMask |=
                1u << fp->coll_data.floor_line_id;
        }
    }
    else
    {
        gNdsStageFloorFollowLoopP1MapUpdateCount++;
        gNdsStageFloorFollowLoopP1HitCount += (hit != FALSE) ? 1u : 0u;
        gNdsStageFloorFollowLoopP1FloorLineID = fp->coll_data.floor_line_id;
        gNdsStageFloorFollowLoopP1FloorKind = line_kind;
        gNdsStageFloorFollowLoopP1FloorLineIsFloor = is_floor;
        gNdsStageFloorFollowLoopP1FloorYMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.y);
        if ((fp->coll_data.floor_line_id >= 0) &&
            (fp->coll_data.floor_line_id < 32))
        {
            gNdsStageFloorFollowLoopP1FloorVisitMask |=
                1u << fp->coll_data.floor_line_id;
        }
    }
    fp->coll_data.floor_dist = root->translate.vec.f.y;
    ndsStageFloorEdgeLoopRecordFighter(fp->fighter_gobj);
}

static void ndsStageMPUpdateFloorLoopCopyBack(FTStruct *fp,
                                              MPCollData *mp_coll,
                                              sb32 hit)
{
    DObj *root;

    ndsStageMPProcessFloorLoopCopyBack(fp, mp_coll);
    if ((fp == NULL) || (mp_coll == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        return;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return;
    }
    if (hit != FALSE)
    {
        fp->ga = nMPKineticsGround;
        fp->coll_data.mask_stat |= MAP_FLAG_FLOOR;
        fp->coll_data.floor_dist = root->translate.vec.f.y;
    }
    gNdsStageMPUpdateFloorLoopAdapterCopyBackCount++;
    ndsStageMPUpdateFloorLoopMirrorBaseFighter(fp, hit);
}

static sb32 ndsStageMPUpdateFloorLoopUpdateFighter(GObj *fighter_gobj)
{
    FTStruct *fp;
    DObj *root;
    MPCollData mp_coll;
    sb32 hit;
    u32 slot;
    s32 live_slot_prev;
    sb32 probe_active_prev = FALSE;
    sb32 motion_stale_active = FALSE;

    if ((ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPUpdateFloorLoopPrepared == 0u))
    {
        return TRUE;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->player >= 2))
    {
        gNdsStageMPUpdateFloorLoopUnsafeCount++;
        return FALSE;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        gNdsStageMPUpdateFloorLoopUnsafeCount++;
        return FALSE;
    }
    slot = fp->player;
    if (slot == 0u)
    {
        gNdsStageMPUpdateFloorLoopP0UpdateCount++;
        gNdsStageMPProcessFloorLoopP0UpdateCount++;
    }
    else
    {
        gNdsStageMPUpdateFloorLoopP1UpdateCount++;
        gNdsStageMPProcessFloorLoopP1UpdateCount++;
    }
    if (ndsStageMPUpdateFloorLoopBuildCollData(fp, &mp_coll) == FALSE)
    {
        gNdsStageMPUpdateFloorLoopUnsafeCount++;
        return FALSE;
    }
    live_slot_prev = sNdsStageMPCrossFloorLoopLiveSlot;
    if (ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() != FALSE)
    {
        sNdsStageMPCrossFloorLoopLiveSlot = (s32)slot;
    }
    if ((ndsFighterMarioFoxStageMPMotionStaleFloorLoopProofEnabled() !=
            FALSE) &&
        (slot == 0u) &&
        (gNdsStageMPMotionStaleFloorLoopMutationCount != 0u) &&
        (gNdsStageMPMotionStaleFloorLoopUpdateHitCount == 0u))
    {
        motion_stale_active = TRUE;
        probe_active_prev = sNdsStageMPLiveStaleFloorLoopProbeActive;
        sNdsStageMPLiveStaleFloorLoopProbeActive = TRUE;
        sNdsStageMPMotionStaleFloorLoopUpdateActive = TRUE;
    }
    hit = mpProcessUpdateMain(&mp_coll, mpCommonRunFighterAllCollisions,
                              fighter_gobj, MAP_PROC_TYPE_CLIFFEDGE);
    if (motion_stale_active != FALSE)
    {
        sNdsStageMPLiveStaleFloorLoopProbeActive = probe_active_prev;
        sNdsStageMPMotionStaleFloorLoopUpdateActive = FALSE;
    }
    if (ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() != FALSE)
    {
        sNdsStageMPCrossFloorLoopLiveSlot = live_slot_prev;
    }
    ndsStageMPUpdateFloorLoopCopyBack(fp, &mp_coll, hit);
    if (hit != FALSE)
    {
        if (slot == 0u)
        {
            gNdsStageMPUpdateFloorLoopP0HitCount++;
            gNdsStageMPProcessFloorLoopP0HitCount++;
        }
        else
        {
            gNdsStageMPUpdateFloorLoopP1HitCount++;
            gNdsStageMPProcessFloorLoopP1HitCount++;
        }
    }
    else if (slot == 0u)
    {
        gNdsStageMPUpdateFloorLoopP0MissCount++;
        gNdsStageMPProcessFloorLoopP0MissCount++;
    }
    else
    {
        gNdsStageMPUpdateFloorLoopP1MissCount++;
        gNdsStageMPProcessFloorLoopP1MissCount++;
    }

    if (slot == 0u)
    {
        gNdsStageMPUpdateFloorLoopP0RootXFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsStageMPUpdateFloorLoopP0RootYFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.y);
        gNdsStageMPUpdateFloorLoopP0FinalLineID =
            fp->coll_data.floor_line_id;
        gNdsStageMPUpdateFloorLoopP0FinalLineIsFloor =
            (ndsMPLineIDIsFloor(fp->coll_data.floor_line_id) != FALSE) ? 1u :
                0u;
        gNdsStageMPUpdateFloorLoopP0FinalMaskStat =
            fp->coll_data.mask_stat;
        gNdsStageMPUpdateFloorLoopP0FloorOK =
            ((hit != FALSE) && (fp->ga == nMPKineticsGround) &&
             ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u)) ? 1u : 0u;
        gNdsStageMPProcessFloorLoopP0FinalLineID =
            fp->coll_data.floor_line_id;
        gNdsStageMPProcessFloorLoopP0FinalLineIsFloor =
            gNdsStageMPUpdateFloorLoopP0FinalLineIsFloor;
        gNdsStageMPProcessFloorLoopP0FinalMaskStat =
            fp->coll_data.mask_stat;
        gNdsStageMPProcessFloorLoopP0FinalDistMilli =
            ndsFloatToMilliSigned(mp_coll.floor_dist);
        gNdsStageMPProcessFloorLoopP0RootYMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.y);
        sNdsStageMPUpdateFloorLoopPrevRoot[0] = root->translate.vec.f;
        sNdsStageMPUpdateFloorLoopPrevRootValid[0] = 1u;
        if (ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPCrossFloorLoopP0FinalLineID =
                fp->coll_data.floor_line_id;
            gNdsStageMPCrossFloorLoopP0FinalFloorOK =
                gNdsStageMPUpdateFloorLoopP0FloorOK;
            if ((gNdsStageMPCrossFloorLoopTargetLineID >= 0) &&
                (fp->coll_data.floor_line_id ==
                    gNdsStageMPCrossFloorLoopTargetLineID))
            {
                gNdsStageMPCrossFloorLoopP0TargetLineMatchCount++;
            }
        }
        if ((ndsFighterMarioFoxStageMPStaleFloorLoopProofEnabled() != FALSE) &&
            ((ndsFighterMarioFoxStageMPLiveStaleFloorLoopProofEnabled() ==
              FALSE) ||
             (gNdsStageMPLiveStaleFloorLoopSelectedCallbackCount == 0u)))
        {
            gNdsStageMPStaleFloorLoopP0FinalLineID =
                fp->coll_data.floor_line_id;
            gNdsStageMPStaleFloorLoopP0FinalFloorOK =
                gNdsStageMPUpdateFloorLoopP0FloorOK;
            if ((gNdsStageMPStaleFloorLoopTargetLineID >= 0) &&
                (fp->coll_data.floor_line_id ==
                    gNdsStageMPStaleFloorLoopTargetLineID))
            {
                gNdsStageMPStaleFloorLoopP0TargetLineMatchCount++;
            }
        }
        if ((ndsFighterMarioFoxStageMPLiveStaleFloorLoopProofEnabled() !=
                FALSE) &&
            (gNdsStageMPLiveStaleFloorLoopSelectedCallbackCount == 0u))
        {
            gNdsStageMPLiveStaleFloorLoopP0FinalLineID =
                fp->coll_data.floor_line_id;
            gNdsStageMPLiveStaleFloorLoopP0FinalFloorOK =
                gNdsStageMPUpdateFloorLoopP0FloorOK;
            if ((gNdsStageMPLiveStaleFloorLoopTargetLineID >= 0) &&
                (fp->coll_data.floor_line_id ==
                    gNdsStageMPLiveStaleFloorLoopTargetLineID))
            {
                gNdsStageMPLiveStaleFloorLoopP0TargetLineMatchCount++;
            }
        }
        if (motion_stale_active != FALSE)
        {
            gNdsStageMPMotionStaleFloorLoopP0FinalXMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.x);
            gNdsStageMPMotionStaleFloorLoopP0FinalYMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.y +
                    fp->coll_data.map_coll.bottom);
            gNdsStageMPMotionStaleFloorLoopP0FinalLineID =
                fp->coll_data.floor_line_id;
            gNdsStageMPMotionStaleFloorLoopP0FinalFloorOK =
                gNdsStageMPUpdateFloorLoopP0FloorOK;
            if (hit != FALSE)
            {
                gNdsStageMPMotionStaleFloorLoopUpdateHitCount++;
            }
            if ((gNdsStageMPMotionStaleFloorLoopTargetLineID >= 0) &&
                (fp->coll_data.floor_line_id ==
                    gNdsStageMPMotionStaleFloorLoopTargetLineID))
            {
                gNdsStageMPMotionStaleFloorLoopTargetMatchCount++;
            }
            if ((gNdsStageMPMotionStaleFloorLoopP0FinalFloorOK != 0u) &&
                (gNdsStageMPMotionStaleFloorLoopTargetLineID >= 0) &&
                (fp->coll_data.floor_line_id ==
                    gNdsStageMPMotionStaleFloorLoopTargetLineID))
            {
                gNdsFighterGCDrawAllLoopP0RootDirectionOK = 1u;
            }

            gNdsStageMPLiveStaleFloorLoopP0FinalLineID =
                fp->coll_data.floor_line_id;
            gNdsStageMPLiveStaleFloorLoopP0FinalFloorOK =
                gNdsStageMPUpdateFloorLoopP0FloorOK;
            if ((gNdsStageMPLiveStaleFloorLoopTargetLineID >= 0) &&
                (fp->coll_data.floor_line_id ==
                    gNdsStageMPLiveStaleFloorLoopTargetLineID))
            {
                gNdsStageMPLiveStaleFloorLoopP0TargetLineMatchCount++;
            }

            gNdsStageMPStaleFloorLoopP0FinalLineID =
                fp->coll_data.floor_line_id;
            gNdsStageMPStaleFloorLoopP0FinalFloorOK =
                gNdsStageMPUpdateFloorLoopP0FloorOK;
            if ((gNdsStageMPStaleFloorLoopTargetLineID >= 0) &&
                (fp->coll_data.floor_line_id ==
                    gNdsStageMPStaleFloorLoopTargetLineID))
            {
                gNdsStageMPStaleFloorLoopP0TargetLineMatchCount++;
            }
        }
    }
    else
    {
        gNdsStageMPUpdateFloorLoopP1RootXFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsStageMPUpdateFloorLoopP1RootYFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.y);
        gNdsStageMPUpdateFloorLoopP1FinalLineID =
            fp->coll_data.floor_line_id;
        gNdsStageMPUpdateFloorLoopP1FinalLineIsFloor =
            (ndsMPLineIDIsFloor(fp->coll_data.floor_line_id) != FALSE) ? 1u :
                0u;
        gNdsStageMPUpdateFloorLoopP1FinalMaskStat =
            fp->coll_data.mask_stat;
        gNdsStageMPUpdateFloorLoopP1FloorOK =
            ((hit != FALSE) && (fp->ga == nMPKineticsGround) &&
             ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u)) ? 1u : 0u;
        gNdsStageMPProcessFloorLoopP1FinalLineID =
            fp->coll_data.floor_line_id;
        gNdsStageMPProcessFloorLoopP1FinalLineIsFloor =
            gNdsStageMPUpdateFloorLoopP1FinalLineIsFloor;
        gNdsStageMPProcessFloorLoopP1FinalMaskStat =
            fp->coll_data.mask_stat;
        gNdsStageMPProcessFloorLoopP1FinalDistMilli =
            ndsFloatToMilliSigned(mp_coll.floor_dist);
        gNdsStageMPProcessFloorLoopP1RootYMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.y);
        sNdsStageMPUpdateFloorLoopPrevRoot[1] = root->translate.vec.f;
        sNdsStageMPUpdateFloorLoopPrevRootValid[1] = 1u;
        if (ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPCrossFloorLoopP1FinalLineID =
                fp->coll_data.floor_line_id;
            gNdsStageMPCrossFloorLoopP1FinalFloorOK =
                gNdsStageMPUpdateFloorLoopP1FloorOK;
        }
        if (ndsFighterMarioFoxStageMPStaleFloorLoopProofEnabled() != FALSE)
        {
            gNdsStageMPStaleFloorLoopP1FinalLineID =
                fp->coll_data.floor_line_id;
            gNdsStageMPStaleFloorLoopP1FinalFloorOK =
                gNdsStageMPUpdateFloorLoopP1FloorOK;
        }
        if (ndsFighterMarioFoxStageMPLiveStaleFloorLoopProofEnabled() !=
            FALSE)
        {
            gNdsStageMPLiveStaleFloorLoopP1FinalLineID =
                fp->coll_data.floor_line_id;
            gNdsStageMPLiveStaleFloorLoopP1FinalFloorOK =
                gNdsStageMPUpdateFloorLoopP1FloorOK;
        }
        if (ndsFighterMarioFoxStageMPMotionStaleFloorLoopProofEnabled() !=
            FALSE)
        {
            gNdsStageMPMotionStaleFloorLoopP1FinalLineID =
                fp->coll_data.floor_line_id;
            gNdsStageMPMotionStaleFloorLoopP1FinalFloorOK =
                gNdsStageMPUpdateFloorLoopP1FloorOK;
        }
    }
    return hit;
}

sb32 mpCommonCheckFighterOnFloor(GObj *fighter_gobj)
{
    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffTickFloorLoopStatusActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPCliffTickFloorLoopOttottoFloorCheckCount++;
        gNdsStageMPCliffTickFloorLoopOttottoFloorHitCount++;
        return TRUE;
    }
    if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() == FALSE)
    {
        return ndsMPCommonCheckFighterOnCliffEdgeBounded(fighter_gobj);
    }
    gNdsStageMPUpdateFloorLoopCheckFloorCallCount++;
    if (ndsStageMPUpdateFloorLoopUpdateFighter(fighter_gobj) != FALSE)
    {
        gNdsStageMPUpdateFloorLoopCheckFloorHitCount++;
        return TRUE;
    }
    gNdsStageMPUpdateFloorLoopCheckFloorMissCount++;
    return FALSE;
}

sb32 mpCommonCheckFighterOnEdge(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCatchMapActive != FALSE))
    {
        gNdsStageMPPassiveLoopCatchMapEdgeCheckCount++;
    }
    return TRUE;
}

sb32 mpCommonCheckFighterOnCliffEdge(GObj *fighter_gobj)
{
    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffStatusFloorLoopActive != FALSE))
    {
        (void)fighter_gobj;
        return FALSE;
    }
    if (ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() == FALSE)
    {
        return ndsMPCommonCheckFighterOnCliffEdgeBounded(fighter_gobj);
    }
    gNdsStageMPUpdateFloorLoopCheckCliffEdgeCallCount++;
    if (ndsStageMPUpdateFloorLoopUpdateFighter(fighter_gobj) != FALSE)
    {
        gNdsStageMPUpdateFloorLoopCheckCliffEdgeHitCount++;
        return TRUE;
    }
    gNdsStageMPUpdateFloorLoopCheckCliffEdgeMissCount++;
    return FALSE;
}

static void ndsStageMPProcessFloorLoopRunStandaloneProbes(void)
{
    s32 line_id = gNdsStageFloorEdgeLoopSelectedLineID;
    f32 left_x = (f32)gNdsStageFloorEdgeLoopLeftXMilli / 1000.0F;
    f32 right_x = (f32)gNdsStageFloorEdgeLoopRightXMilli / 1000.0F;
    f32 mid_x = (left_x + right_x) * 0.5F;
    f32 floor_y = 0.0F;
    Vec3f translate;
    MPCollData coll;
    f32 dist = 0.0F;
    u32 flags = 0u;
    Vec3f angle;

    if ((line_id < 0) ||
        (ndsStageFloorEdgeLoopFloorYAtX(line_id, mid_x, &floor_y) == FALSE))
    {
        gNdsStageMPProcessFloorLoopUnsafeCount++;
        return;
    }

    translate.x = mid_x;
    translate.y = floor_y;
    translate.z = 0.0F;
    ndsStageMPProcessFloorLoopInitColl(&coll, &translate, line_id);
    gNdsStageMPProcessFloorLoopInsideProbeCount++;
    if (mpProcessCheckTestFloorCollisionNew(&coll) != FALSE)
    {
        gNdsStageMPProcessFloorLoopInsideProbeHitCount++;
        mpProcessSetLandingFloor(&coll);
        mpProcessSetCollideFloor(&coll);
    }

    translate.x = left_x - 32.0F;
    translate.y = floor_y;
    translate.z = 0.0F;
    ndsStageMPProcessFloorLoopInitColl(&coll, &translate, line_id);
    gNdsStageMPProcessFloorLoopOutsideProbeCount++;
    if (mpProcessCheckTestFloorCollisionNew(&coll) == FALSE)
    {
        gNdsStageMPProcessFloorLoopOutsideProbeMissCount++;
    }

    translate.x = right_x + 32.0F;
    translate.y = floor_y;
    translate.z = 0.0F;
    ndsStageMPProcessFloorLoopInitColl(&coll, &translate, line_id);
    gNdsStageMPProcessFloorLoopOutsideProbeCount++;
    if (mpProcessCheckTestFloorCollisionNew(&coll) == FALSE)
    {
        gNdsStageMPProcessFloorLoopOutsideProbeMissCount++;
    }

    translate.x = mid_x;
    translate.y = floor_y - 32.0F;
    translate.z = 0.0F;
    gNdsStageMPProcessFloorLoopBelowFloorProbeCount++;
    if ((mpCollisionGetFCCommonFloor(line_id, &translate, &dist, &flags,
            &angle) != FALSE) && (dist > 0.0F))
    {
        gNdsStageMPProcessFloorLoopBelowFloorPositiveDistCount++;
    }
    ndsStageMPProcessFloorLoopInitColl(&coll, &translate, line_id);
    if ((mpProcessCheckTestFloorCollisionNew(&coll) != FALSE) &&
        (translate.y >= floor_y))
    {
        gNdsStageMPProcessFloorLoopBelowFloorPositiveDistCount++;
    }
}

void ndsFighterMarioFoxStageMPProcessFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPProcessFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPProcessFloorLoopReset();
    gNdsStageMPProcessFloorLoopPrepared = 1u;
    gNdsStageMPProcessFloorLoopBaseFloorEdgeSeen =
        (gNdsStageFloorEdgeLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPProcessFloorLoopBaseFloorEdgeSeen == 0u)
    {
        gNdsStageMPProcessFloorLoopUnsafeCount++;
        return;
    }
    ndsStageMPProcessFloorLoopRunStandaloneProbes();
}

static sb32 ndsStageMPProcessFloorLoopUpdateFighter(GObj *fighter_gobj)
{
    FTStruct *fp;
    DObj *root;
    MPCollData mp_coll;
    sb32 hit;
    u32 slot;

    if ((ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPProcessFloorLoopPrepared == 0u))
    {
        return TRUE;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->player >= 2))
    {
        return FALSE;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return FALSE;
    }
    slot = fp->player;
    if (slot == 0u)
    {
        gNdsStageMPProcessFloorLoopP0UpdateCount++;
    }
    else
    {
        gNdsStageMPProcessFloorLoopP1UpdateCount++;
    }
    if (ndsStageMPProcessFloorLoopBuildCollData(fp, &mp_coll) == FALSE)
    {
        gNdsStageMPProcessFloorLoopUnsafeCount++;
        return FALSE;
    }
    hit = mpProcessCheckTestFloorCollisionNew(&mp_coll);
    ndsStageMPProcessFloorLoopCopyBack(fp, &mp_coll);
    fp->ga = nMPKineticsGround;
    if (hit != FALSE)
    {
        if (slot == 0u)
        {
            gNdsStageMPProcessFloorLoopP0HitCount++;
        }
        else
        {
            gNdsStageMPProcessFloorLoopP1HitCount++;
        }
    }
    else if (slot == 0u)
    {
        gNdsStageMPProcessFloorLoopP0MissCount++;
    }
    else
    {
        gNdsStageMPProcessFloorLoopP1MissCount++;
    }

    if (slot == 0u)
    {
        gNdsStageMPProcessFloorLoopP0FinalLineID =
            fp->coll_data.floor_line_id;
        gNdsStageMPProcessFloorLoopP0FinalLineIsFloor =
            (ndsMPLineIDIsFloor(fp->coll_data.floor_line_id) != FALSE) ? 1u :
                0u;
        gNdsStageMPProcessFloorLoopP0FinalMaskStat =
            fp->coll_data.mask_stat;
        gNdsStageMPProcessFloorLoopP0FinalDistMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
        gNdsStageMPProcessFloorLoopP0RootYMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.y);
    }
    else
    {
        gNdsStageMPProcessFloorLoopP1FinalLineID =
            fp->coll_data.floor_line_id;
        gNdsStageMPProcessFloorLoopP1FinalLineIsFloor =
            (ndsMPLineIDIsFloor(fp->coll_data.floor_line_id) != FALSE) ? 1u :
                0u;
        gNdsStageMPProcessFloorLoopP1FinalMaskStat =
            fp->coll_data.mask_stat;
        gNdsStageMPProcessFloorLoopP1FinalDistMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
        gNdsStageMPProcessFloorLoopP1RootYMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.y);
    }
    return hit;
}

void ndsFighterMarioFoxStageMPProcessFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPProcessFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPProcessFloorLoopResult != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxGCDrawAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageGCDrawAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_GCDRAWALL_LOOP_PASS) &&
        (gNdsStageCollisionLoopPrepared != 0u) &&
        (gNdsStageCollisionLoopGroundDataReady == 1u) &&
        (gNdsStageCollisionLoopGeometryReady == 1u) &&
        (gNdsStageFloorFollowLoopPrepared != 0u) &&
        (gNdsStageFloorFollowLoopGeometryReady != 0u) &&
        (gNdsFighterMarioFoxStageFloorEdgeLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_FLOOR_EDGE_LOOP_PASS))
    {
        mask |= 0x1fu;
    }
    if ((gNdsStageMPProcessFloorLoopPrepared != 0u) &&
        (gNdsStageMPProcessFloorLoopBaseFloorEdgeSeen != 0u) &&
        (gNdsStageMPProcessFloorLoopAdapterBuildCount > 0u) &&
        (gNdsStageMPProcessFloorLoopAdapterCopyBackCount > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPProcessFloorLoopProjectFloorIDCallCount > 0u) &&
        (gNdsStageMPProcessFloorLoopProjectFloorIDMissCount > 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPProcessFloorLoopTestNewCallCount > 0u) &&
        (gNdsStageMPProcessFloorLoopTestNewHitCount > 0u) &&
        (gNdsStageMPProcessFloorLoopTestNewMissCount > 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPProcessFloorLoopInsideProbeCount > 0u) &&
        (gNdsStageMPProcessFloorLoopInsideProbeHitCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPProcessFloorLoopOutsideProbeCount >= 2u) &&
        (gNdsStageMPProcessFloorLoopOutsideProbeMissCount >= 2u) &&
        (gNdsStageMPProcessFloorLoopTestNewSetProjectCount > 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPProcessFloorLoopBelowFloorProbeCount > 0u) &&
        (gNdsStageMPProcessFloorLoopBelowFloorPositiveDistCount > 0u) &&
        (gNdsStageMPProcessFloorLoopFCCommonPositiveDistCount > 0u))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPProcessFloorLoopP0UpdateCount > 0u) &&
        (gNdsStageMPProcessFloorLoopP1UpdateCount > 0u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPProcessFloorLoopP0HitCount > 0u) &&
        (gNdsStageMPProcessFloorLoopP1HitCount > 0u) &&
        (gNdsStageMPProcessFloorLoopP0MissCount == 0u) &&
        (gNdsStageMPProcessFloorLoopP1MissCount == 0u))
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageMPProcessFloorLoopP0FinalLineIsFloor != 0u) &&
        (gNdsStageMPProcessFloorLoopP1FinalLineIsFloor != 0u) &&
        ((gNdsStageMPProcessFloorLoopP0FinalMaskStat & MAP_FLAG_FLOOR) !=
            0u) &&
        ((gNdsStageMPProcessFloorLoopP1FinalMaskStat & MAP_FLAG_FLOOR) !=
            0u))
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageFloorFollowLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorFollowLoopFinalAdoptCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalAdoptCount == 0u) &&
        (gNdsStageMPProcessFloorLoopUnexpectedSceneCount == 0u) &&
        (gNdsStageMPProcessFloorLoopUnexpectedStatusCount == 0u) &&
        (gNdsStageMPProcessFloorLoopUnsafeCount == 0u))
    {
        gNdsStageMPProcessFloorLoopNoFinalRecenterCount = 1u;
        mask |= 1u << 14;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP0FloorOK != 0u) &&
        (gNdsFighterGCDrawAllLoopP1FloorOK != 0u))
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPProcessFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPProcessFloorLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageMPProcessFloorLoopCount =
        gNdsFighterMarioFoxStageFloorEdgeLoopCount;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageMPProcessFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPProcessFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsStageMPUpdateFloorLoopInitColl(MPCollData *coll,
                                              Vec3f *translate,
                                              Vec3f *pos_prev,
                                              s32 line_id)
{
    ndsStageMPProcessFloorLoopInitColl(coll, translate, line_id);
    coll->pos_prev = *pos_prev;
    coll->pos_diff.x = translate->x - pos_prev->x;
    coll->pos_diff.y = translate->y - pos_prev->y;
    coll->pos_diff.z = translate->z - pos_prev->z;
}

static void ndsStageMPUpdateFloorLoopRunStandaloneProbes(void)
{
    s32 line_id = gNdsStageFloorEdgeLoopSelectedLineID;
    f32 left_x = (f32)gNdsStageFloorEdgeLoopLeftXMilli / 1000.0F;
    f32 right_x = (f32)gNdsStageFloorEdgeLoopRightXMilli / 1000.0F;
    f32 mid_x = (left_x + right_x) * 0.5F;
    f32 floor_y = 0.0F;
    Vec3f translate;
    Vec3f pos_prev;
    MPCollData coll;
    u32 steps_before;
    sb32 hit;

    if ((line_id < 0) ||
        (ndsStageFloorEdgeLoopFloorYAtX(line_id, mid_x, &floor_y) == FALSE))
    {
        gNdsStageMPUpdateFloorLoopUnsafeCount++;
        return;
    }

    pos_prev.x = mid_x - 8.0F;
    pos_prev.y = floor_y;
    pos_prev.z = 0.0F;
    translate.x = mid_x;
    translate.y = floor_y;
    translate.z = 0.0F;
    ndsStageMPUpdateFloorLoopInitColl(&coll, &translate, &pos_prev, line_id);
    gNdsStageMPUpdateFloorLoopInsideProbeCount++;
    hit = mpProcessUpdateMain(&coll, mpCommonRunFighterAllCollisions, NULL,
                              MAP_PROC_TYPE_DEFAULT);
    if (hit != FALSE)
    {
        gNdsStageMPUpdateFloorLoopInsideProbeHitCount++;
    }

    pos_prev.x = left_x - 16.0F;
    pos_prev.y = floor_y;
    pos_prev.z = 0.0F;
    translate.x = left_x - 32.0F;
    translate.y = floor_y;
    translate.z = 0.0F;
    ndsStageMPUpdateFloorLoopInitColl(&coll, &translate, &pos_prev, line_id);
    gNdsStageMPUpdateFloorLoopOutsideProbeCount++;
    hit = mpProcessUpdateMain(&coll, mpCommonRunFighterAllCollisions, NULL,
                              MAP_PROC_TYPE_DEFAULT);
    if (hit == FALSE)
    {
        gNdsStageMPUpdateFloorLoopOutsideProbeMissCount++;
    }

    pos_prev.x = mid_x;
    pos_prev.y = floor_y - 8.0F;
    pos_prev.z = 0.0F;
    translate.x = mid_x;
    translate.y = floor_y - 32.0F;
    translate.z = 0.0F;
    ndsStageMPUpdateFloorLoopInitColl(&coll, &translate, &pos_prev, line_id);
    gNdsStageMPUpdateFloorLoopBelowFloorProbeCount++;
    hit = mpProcessUpdateMain(&coll, mpCommonRunFighterAllCollisions, NULL,
                              MAP_PROC_TYPE_DEFAULT);
    if ((hit != FALSE) && (translate.y >= floor_y))
    {
        gNdsStageMPUpdateFloorLoopBelowFloorHitCount++;
    }

    pos_prev.x = mid_x;
    pos_prev.y = floor_y;
    pos_prev.z = 0.0F;
    translate.x = mid_x + 320.0F;
    translate.y = floor_y;
    translate.z = 0.0F;
    ndsStageMPUpdateFloorLoopInitColl(&coll, &translate, &pos_prev, line_id);
    gNdsStageMPUpdateFloorLoopSplitProbeCount++;
    steps_before = gNdsStageMPUpdateFloorLoopUpdateMainStepCount;
    hit = mpProcessUpdateMain(&coll, mpCommonRunFighterAllCollisions, NULL,
                              MAP_PROC_TYPE_DEFAULT);
    (void)hit;
    gNdsStageMPUpdateFloorLoopSplitProbeStepCount =
        gNdsStageMPUpdateFloorLoopUpdateMainStepCount - steps_before;
}

void ndsFighterMarioFoxStageMPUpdateFloorLoopPrepare(void)
{
    u32 i;

    if ((ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPUpdateFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPUpdateFloorLoopReset();
    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root = NULL;

        if (ndsFighterStructIsPoolPointer(fp) != FALSE)
        {
            root = fp->joints[nFTPartsJointTopN];
        }
        if (root != NULL)
        {
            sNdsStageMPUpdateFloorLoopPrevRoot[i] =
                root->translate.vec.f;
            sNdsStageMPUpdateFloorLoopPrevRootValid[i] = 1u;
        }
    }
    gNdsStageMPUpdateFloorLoopPrepared = 1u;
    gNdsStageMPUpdateFloorLoopBaseMPProcessSeen =
        (gNdsStageMPProcessFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPUpdateFloorLoopBaseMPProcessSeen == 0u)
    {
        gNdsStageMPUpdateFloorLoopUnsafeCount++;
        return;
    }
    ndsStageMPUpdateFloorLoopRunStandaloneProbes();
}

void ndsFighterMarioFoxStageMPUpdateFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPUpdateFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPUpdateFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxGCDrawAllLoopResult ==
        NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_PASS)
    {
        mask |= 1u << 0;
    }
    if (gNdsFighterMarioFoxStageGCDrawAllLoopResult ==
        NDS_FIGHTER_MARIOFOX_STAGE_GCDRAWALL_LOOP_PASS)
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageCollisionLoopPrepared != 0u) &&
        (gNdsStageCollisionLoopGroundDataReady == 1u) &&
        (gNdsStageCollisionLoopGeometryReady == 1u))
    {
        mask |= 1u << 2;
    }
    if (gNdsFighterMarioFoxStageFloorEdgeLoopResult ==
        NDS_FIGHTER_MARIOFOX_STAGE_FLOOR_EDGE_LOOP_PASS)
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPProcessFloorLoopPrepared != 0u) &&
        (gNdsStageMPProcessFloorLoopBaseFloorEdgeSeen != 0u) &&
        (gNdsStageMPProcessFloorLoopAdapterBuildCount > 0u) &&
        (gNdsStageMPProcessFloorLoopAdapterCopyBackCount > 0u) &&
        (gNdsStageMPProcessFloorLoopTestNewCallCount > 0u) &&
        (gNdsStageMPProcessFloorLoopP0HitCount > 0u) &&
        (gNdsStageMPProcessFloorLoopP1HitCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPUpdateFloorLoopPrepared != 0u) &&
        (gNdsStageMPUpdateFloorLoopBaseMPProcessSeen != 0u) &&
        (gNdsStageMPUpdateFloorLoopAdapterBuildCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopAdapterCopyBackCount > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPUpdateFloorLoopUpdateMainCallCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopUpdateMainStepCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopProcCollCallCount > 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPUpdateFloorLoopSplitProbeCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopSplitProbeStepCount > 1u) &&
        (gNdsStageMPUpdateFloorLoopUpdateMainMaxStepCount <= 10u) &&
        (gNdsStageMPUpdateFloorLoopUpdateMainCapCount == 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPUpdateFloorLoopAllCollisionsCallCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopAllCollisionsFloorHitCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPUpdateFloorLoopInsideProbeHitCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopBelowFloorHitCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopOutsideProbeMissCount > 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPUpdateFloorLoopCheckCliffEdgeCallCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopCheckCliffEdgeHitCount > 0u))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPUpdateFloorLoopP0UpdateCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopP1UpdateCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopP0HitCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopP1HitCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopP0HitCount >
            gNdsStageMPUpdateFloorLoopP0MissCount) &&
        (gNdsStageMPUpdateFloorLoopP1HitCount >
            gNdsStageMPUpdateFloorLoopP1MissCount) &&
        (gNdsStageMPUpdateFloorLoopP0PosDiffXMilli != 0) &&
        (gNdsStageMPUpdateFloorLoopP1PosDiffXMilli != 0) &&
        (gNdsStageMPUpdateFloorLoopP0FloorOK != 0u) &&
        (gNdsStageMPUpdateFloorLoopP1FloorOK != 0u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPUpdateFloorLoopP0FinalLineIsFloor != 0u) &&
        (gNdsStageMPUpdateFloorLoopP1FinalLineIsFloor != 0u) &&
        ((gNdsStageMPUpdateFloorLoopP0FinalMaskStat & MAP_FLAG_FLOOR) !=
            0u) &&
        ((gNdsStageMPUpdateFloorLoopP1FinalMaskStat & MAP_FLAG_FLOOR) !=
            0u))
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageMPUpdateFloorLoopP0RootXBeforeMilli !=
            gNdsStageMPUpdateFloorLoopP0RootXFinalMilli) &&
        (gNdsStageMPUpdateFloorLoopP1RootXBeforeMilli !=
            gNdsStageMPUpdateFloorLoopP1RootXFinalMilli) &&
        (gNdsStageMPUpdateFloorLoopP0FloorOK != 0u) &&
        (gNdsStageMPUpdateFloorLoopP1FloorOK != 0u))
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageMPUpdateFloorLoopAllCollisionsWallDeferredCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopAllCollisionsCeilDeferredCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopAllCollisionsFloorEdgeAdjustDeferredCount >
            0u) &&
        ((gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount >
            0u) ||
         ((ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() != FALSE) &&
          (gNdsStageMPSweepFloorLoopSecondFloorCallCount > 0u))) &&
        (gNdsStageMPUpdateFloorLoopOttottoDeniedCount == 0u) &&
        (gNdsStageMPUpdateFloorLoopUnsafeCount == 0u))
    {
        mask |= 1u << 14;
    }
    if ((gNdsStageFloorFollowLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorFollowLoopFinalAdoptCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalAdoptCount == 0u) &&
        (gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        gNdsStageMPUpdateFloorLoopNoFinalRecenterCount = 1u;
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPUpdateFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopCount =
        gNdsFighterMarioFoxStageMPProcessFloorLoopCount;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageMPUpdateFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPUpdateFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPSweepFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPSweepFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPSweepFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPSweepFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPSweepFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPSweepFloorLoopCount = 0u;
    gNdsStageMPSweepFloorLoopPrepared = 0u;
    gNdsStageMPSweepFloorLoopBaseMPUpdateSeen = 0u;
    gNdsStageMPSweepFloorLoopCheckFloorCallCount = 0u;
    gNdsStageMPSweepFloorLoopCheckFloorHitCount = 0u;
    gNdsStageMPSweepFloorLoopCheckFloorMissCount = 0u;
    gNdsStageMPSweepFloorLoopLineSweepSameCallCount = 0u;
    gNdsStageMPSweepFloorLoopLineSweepSameHitCount = 0u;
    gNdsStageMPSweepFloorLoopLineSweepSameMissCount = 0u;
    gNdsStageMPSweepFloorLoopLineSweepDiffCallCount = 0u;
    gNdsStageMPSweepFloorLoopLineSweepDiffHitCount = 0u;
    gNdsStageMPSweepFloorLoopLineSweepDiffMissCount = 0u;
    gNdsStageMPSweepFloorLoopLineSweepVisitCount = 0u;
    gNdsStageMPSweepFloorLoopLineSweepCandidateCount = 0u;
    gNdsStageMPSweepFloorLoopLineSweepRejectSameLineCount = 0u;
    gNdsStageMPSweepFloorLoopLineSweepAcceptNewLineCount = 0u;
    gNdsStageMPSweepFloorLoopSecondFloorCallCount = 0u;
    gNdsStageMPSweepFloorLoopSecondFloorHitCount = 0u;
    gNdsStageMPSweepFloorLoopSecondFloorMissCount = 0u;
    gNdsStageMPSweepFloorLoopLandingFloorCallCount = 0u;
    gNdsStageMPSweepFloorLoopFloorEdgeAdjustCallCount = 0u;
    gNdsStageMPSweepFloorLoopFloorEdgeAdjustDeferredCount = 0u;
    gNdsStageMPSweepFloorLoopMaskCurrFloorCount = 0u;
    gNdsStageMPSweepFloorLoopMaskStatFloorEdgeClearCount = 0u;
    gNdsStageMPSweepFloorLoopIsCollEndClearCount = 0u;
    gNdsStageMPSweepFloorLoopSameLineProbeCount = 0u;
    gNdsStageMPSweepFloorLoopSameLineProbeHitCount = 0u;
    gNdsStageMPSweepFloorLoopDiffLineProbeCount = 0u;
    gNdsStageMPSweepFloorLoopDiffLineProbeHitCount = 0u;
    gNdsStageMPSweepFloorLoopNoHitProbeCount = 0u;
    gNdsStageMPSweepFloorLoopNoHitProbeMissCount = 0u;
    gNdsStageMPSweepFloorLoopProbeLineID = -1;
    gNdsStageMPSweepFloorLoopAltLineID = -1;
    gNdsStageMPSweepFloorLoopP0FinalLineID = -1;
    gNdsStageMPSweepFloorLoopP1FinalLineID = -1;
    gNdsStageMPSweepFloorLoopP0FinalLineIsFloor = 0u;
    gNdsStageMPSweepFloorLoopP1FinalLineIsFloor = 0u;
    gNdsStageMPSweepFloorLoopP0FloorOK = 0u;
    gNdsStageMPSweepFloorLoopP1FloorOK = 0u;
    gNdsStageMPSweepFloorLoopUnsafeCount = 0u;
}

static void ndsStageMPSweepFloorLoopRunStandaloneProbes(void)
{
    s32 line_id = gNdsStageFloorEdgeLoopSelectedLineID;
    s32 alt_line_id = -1;
    f32 x = 0.0F;
    f32 floor_y = 0.0F;
    f32 alt_x = 0.0F;
    f32 alt_y = 0.0F;
    Vec3f translate;
    Vec3f pos_prev;
    MPCollData coll;

    if ((line_id < 0) ||
        (ndsStageFloorEdgeLoopFloorYAtX(line_id,
            ((f32)gNdsStageFloorEdgeLoopLeftXMilli +
             (f32)gNdsStageFloorEdgeLoopRightXMilli) / 2000.0F,
            &floor_y) == FALSE))
    {
        gNdsStageMPSweepFloorLoopUnsafeCount++;
        return;
    }
    x = ((f32)gNdsStageFloorEdgeLoopLeftXMilli +
         (f32)gNdsStageFloorEdgeLoopRightXMilli) / 2000.0F;
    gNdsStageMPSweepFloorLoopProbeLineID = line_id;

    pos_prev.x = x;
    pos_prev.y = floor_y;
    pos_prev.z = 0.0F;
    translate.x = x;
    translate.y = floor_y;
    translate.z = 0.0F;
    ndsStageMPUpdateFloorLoopInitColl(&coll, &translate, &pos_prev, line_id);
    coll.update_tic = gMPCollisionUpdateTic;
    gNdsStageMPSweepFloorLoopSameLineProbeCount++;
    if (mpProcessCheckTestFloorCollision(&coll, line_id) == FALSE)
    {
        gNdsStageMPSweepFloorLoopSameLineProbeHitCount++;
    }

    if (ndsStageMPSweepFloorLoopChooseLine(line_id, &alt_line_id, &alt_x,
            &alt_y) == FALSE)
    {
        gNdsStageMPSweepFloorLoopUnsafeCount++;
        return;
    }
    gNdsStageMPSweepFloorLoopAltLineID = alt_line_id;
    pos_prev.x = alt_x;
    pos_prev.y = alt_y + 32.0F;
    pos_prev.z = 0.0F;
    translate.x = alt_x;
    translate.y = alt_y;
    translate.z = 0.0F;
    ndsStageMPUpdateFloorLoopInitColl(&coll, &translate, &pos_prev, line_id);
    coll.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    gNdsStageMPSweepFloorLoopDiffLineProbeCount++;
    if (mpProcessCheckTestFloorCollision(&coll, line_id) != FALSE)
    {
        gNdsStageMPSweepFloorLoopDiffLineProbeHitCount++;
        mpProcessSetLandingFloor(&coll);
        if ((coll.mask_stat & MAP_FLAG_FLOOR) != 0u)
        {
            mpProcessRunFloorEdgeAdjust(&coll);
        }
        coll.mask_stat |= MAP_FLAG_FLOOREDGE;
        coll.mask_stat &= (u16)~MAP_FLAG_FLOOREDGE;
        gNdsStageMPSweepFloorLoopMaskStatFloorEdgeClearCount++;
        coll.is_coll_end = FALSE;
        gNdsStageMPSweepFloorLoopIsCollEndClearCount++;
    }

    pos_prev.x = 100000.0F;
    pos_prev.y = floor_y;
    pos_prev.z = 0.0F;
    translate.x = 100032.0F;
    translate.y = floor_y;
    translate.z = 0.0F;
    ndsStageMPUpdateFloorLoopInitColl(&coll, &translate, &pos_prev, line_id);
    coll.update_tic = gMPCollisionUpdateTic;
    gNdsStageMPSweepFloorLoopNoHitProbeCount++;
    if (mpProcessCheckTestFloorCollision(&coll, line_id) == FALSE)
    {
        gNdsStageMPSweepFloorLoopNoHitProbeMissCount++;
    }
}

void ndsFighterMarioFoxStageMPSweepFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPSweepFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPSweepFloorLoopReset();
    gNdsStageMPSweepFloorLoopPrepared = 1u;
    gNdsStageMPSweepFloorLoopBaseMPUpdateSeen =
        (gNdsStageMPUpdateFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPSweepFloorLoopBaseMPUpdateSeen == 0u)
    {
        gNdsStageMPSweepFloorLoopUnsafeCount++;
        return;
    }
    ndsStageMPSweepFloorLoopRunStandaloneProbes();
}

void ndsFighterMarioFoxStageMPSweepFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPSweepFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPSweepFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPUpdateFloorLoopResult ==
        NDS_FIGHTER_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP_PASS)
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPSweepFloorLoopPrepared != 0u) &&
        (gNdsStageMPSweepFloorLoopBaseMPUpdateSeen != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPSweepFloorLoopCheckFloorCallCount > 0u) &&
        (gNdsStageMPSweepFloorLoopCheckFloorHitCount > 0u) &&
        (gNdsStageMPSweepFloorLoopCheckFloorMissCount > 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPSweepFloorLoopLineSweepSameCallCount > 0u) &&
        (gNdsStageMPSweepFloorLoopLineSweepSameHitCount > 0u) &&
        (gNdsStageMPSweepFloorLoopLineSweepDiffCallCount > 0u) &&
        (gNdsStageMPSweepFloorLoopLineSweepDiffHitCount > 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPSweepFloorLoopLineSweepVisitCount > 0u) &&
        (gNdsStageMPSweepFloorLoopLineSweepCandidateCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPSweepFloorLoopSameLineProbeCount > 0u) &&
        (gNdsStageMPSweepFloorLoopSameLineProbeHitCount > 0u) &&
        (gNdsStageMPSweepFloorLoopLineSweepRejectSameLineCount > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPSweepFloorLoopDiffLineProbeCount > 0u) &&
        (gNdsStageMPSweepFloorLoopDiffLineProbeHitCount > 0u) &&
        (gNdsStageMPSweepFloorLoopLineSweepAcceptNewLineCount > 0u) &&
        (gNdsStageMPSweepFloorLoopMaskCurrFloorCount > 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPSweepFloorLoopNoHitProbeCount > 0u) &&
        (gNdsStageMPSweepFloorLoopNoHitProbeMissCount > 0u) &&
        (gNdsStageMPSweepFloorLoopLineSweepSameMissCount > 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPSweepFloorLoopSecondFloorCallCount > 0u) &&
        (gNdsStageMPSweepFloorLoopSecondFloorMissCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount ==
            0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPSweepFloorLoopLandingFloorCallCount > 0u) &&
        (gNdsStageMPSweepFloorLoopFloorEdgeAdjustCallCount > 0u) &&
        ((gNdsStageMPSweepFloorLoopFloorEdgeAdjustDeferredCount > 0u) ||
         ((ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE) &&
          (gNdsStageMPAdjustFloorLoopRunCallCount > 0u))))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPSweepFloorLoopMaskStatFloorEdgeClearCount > 0u) &&
        (gNdsStageMPSweepFloorLoopIsCollEndClearCount > 0u))
    {
        mask |= 1u << 10;
    }
    gNdsStageMPSweepFloorLoopP0FinalLineID =
        gNdsStageMPUpdateFloorLoopP0FinalLineID;
    gNdsStageMPSweepFloorLoopP1FinalLineID =
        gNdsStageMPUpdateFloorLoopP1FinalLineID;
    gNdsStageMPSweepFloorLoopP0FinalLineIsFloor =
        gNdsStageMPUpdateFloorLoopP0FinalLineIsFloor;
    gNdsStageMPSweepFloorLoopP1FinalLineIsFloor =
        gNdsStageMPUpdateFloorLoopP1FinalLineIsFloor;
    gNdsStageMPSweepFloorLoopP0FloorOK =
        gNdsStageMPUpdateFloorLoopP0FloorOK;
    gNdsStageMPSweepFloorLoopP1FloorOK =
        gNdsStageMPUpdateFloorLoopP1FloorOK;
    if ((gNdsStageMPSweepFloorLoopP0FinalLineIsFloor != 0u) &&
        (gNdsStageMPSweepFloorLoopP1FinalLineIsFloor != 0u) &&
        (gNdsStageMPSweepFloorLoopP0FloorOK != 0u) &&
        (gNdsStageMPSweepFloorLoopP1FloorOK != 0u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPSweepFloorLoopProbeLineID >= 0) &&
        (gNdsStageMPSweepFloorLoopAltLineID >= 0) &&
        (gNdsStageMPSweepFloorLoopProbeLineID !=
            gNdsStageMPSweepFloorLoopAltLineID))
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageMPUpdateFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPSweepFloorLoopUnsafeCount == 0u))
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageFloorFollowLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorFollowLoopFinalAdoptCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalAdoptCount == 0u))
    {
        mask |= 1u << 14;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPSweepFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPSweepFloorLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageMPSweepFloorLoopCount =
        gNdsFighterMarioFoxStageMPUpdateFloorLoopCount;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageMPSweepFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPSweepFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCrossFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCrossFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCrossFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCrossFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCrossFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCrossFloorLoopCount = 0u;
    gNdsStageMPCrossFloorLoopPrepared = 0u;
    gNdsStageMPCrossFloorLoopBaseMPSweepSeen = 0u;
    gNdsStageMPCrossFloorLoopPrimeAttemptCount = 0u;
    gNdsStageMPCrossFloorLoopPrimeHitCount = 0u;
    gNdsStageMPCrossFloorLoopPrimeMissCount = 0u;
    gNdsStageMPCrossFloorLoopSourceLineID = -1;
    gNdsStageMPCrossFloorLoopTargetLineID = -1;
    gNdsStageMPCrossFloorLoopTargetXMilli = 0;
    gNdsStageMPCrossFloorLoopTargetYMilli = 0;
    gNdsStageMPCrossFloorLoopLiveSecondFloorCallCount = 0u;
    gNdsStageMPCrossFloorLoopLiveSecondFloorHitCount = 0u;
    gNdsStageMPCrossFloorLoopLiveSecondFloorMissCount = 0u;
    gNdsStageMPCrossFloorLoopLiveAcceptedNewLineCount = 0u;
    gNdsStageMPCrossFloorLoopLiveLandingFloorCount = 0u;
    gNdsStageMPCrossFloorLoopLiveFloorEdgeAdjustCount = 0u;
    gNdsStageMPCrossFloorLoopLiveCollEndClearCount = 0u;
    gNdsStageMPCrossFloorLoopP0CrossHitCount = 0u;
    gNdsStageMPCrossFloorLoopP1CrossHitCount = 0u;
    gNdsStageMPCrossFloorLoopP0FinalLineID = -1;
    gNdsStageMPCrossFloorLoopP1FinalLineID = -1;
    gNdsStageMPCrossFloorLoopP0TargetLineMatchCount = 0u;
    gNdsStageMPCrossFloorLoopP0FinalFloorOK = 0u;
    gNdsStageMPCrossFloorLoopP1FinalFloorOK = 0u;
    gNdsStageMPCrossFloorLoopUnsafeCount = 0u;
    sNdsStageMPCrossFloorLoopLiveSlot = -1;
}

void ndsFighterMarioFoxStageMPCrossFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPCrossFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCrossFloorLoopReset();
    gNdsStageMPCrossFloorLoopPrepared = 1u;
    gNdsStageMPCrossFloorLoopBaseMPSweepSeen =
        (gNdsStageMPSweepFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPCrossFloorLoopBaseMPSweepSeen == 0u)
    {
        gNdsStageMPCrossFloorLoopUnsafeCount++;
    }
}

void ndsFighterMarioFoxStageMPCrossFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPCrossFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCrossFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPSweepFloorLoopResult ==
        NDS_FIGHTER_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP_PASS)
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCrossFloorLoopPrepared != 0u) &&
        (gNdsStageMPCrossFloorLoopBaseMPSweepSeen != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPCrossFloorLoopPrimeAttemptCount > 0u) &&
        (gNdsStageMPCrossFloorLoopPrimeHitCount > 0u) &&
        (gNdsStageMPCrossFloorLoopPrimeMissCount == 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCrossFloorLoopSourceLineID >= -1) &&
        (gNdsStageMPCrossFloorLoopTargetLineID >= 0) &&
        (gNdsStageMPCrossFloorLoopSourceLineID !=
            gNdsStageMPCrossFloorLoopTargetLineID))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCrossFloorLoopLiveSecondFloorCallCount > 0u) &&
        (gNdsStageMPCrossFloorLoopLiveSecondFloorHitCount > 0u) &&
        (gNdsStageMPCrossFloorLoopLiveAcceptedNewLineCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCrossFloorLoopLiveLandingFloorCount > 0u) &&
        (gNdsStageMPCrossFloorLoopLiveFloorEdgeAdjustCount > 0u) &&
        (gNdsStageMPCrossFloorLoopLiveCollEndClearCount > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCrossFloorLoopP0CrossHitCount > 0u) &&
        (gNdsStageMPCrossFloorLoopP0TargetLineMatchCount > 0u) &&
        (gNdsStageMPCrossFloorLoopP0FinalLineID ==
            gNdsStageMPCrossFloorLoopTargetLineID))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCrossFloorLoopP0FinalFloorOK != 0u) &&
        (gNdsStageMPCrossFloorLoopP1FinalFloorOK != 0u) &&
        (gNdsStageMPCrossFloorLoopP1FinalLineID >= 0))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPSweepFloorLoopSecondFloorHitCount > 0u) &&
        (gNdsStageMPSweepFloorLoopSecondFloorMissCount > 0u) &&
        (gNdsStageMPSweepFloorLoopLineSweepDiffHitCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount ==
            0u) &&
        ((gNdsStageMPSweepFloorLoopFloorEdgeAdjustDeferredCount > 0u) ||
         ((ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() != FALSE) &&
          (gNdsStageMPAdjustFloorLoopRunCallCount > 0u))))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPUpdateFloorLoopP0HitCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopP1HitCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopP0HitCount >
            gNdsStageMPUpdateFloorLoopP0MissCount) &&
        (gNdsStageMPUpdateFloorLoopP1HitCount >
            gNdsStageMPUpdateFloorLoopP1MissCount) &&
        (gNdsStageMPUpdateFloorLoopP0FloorOK != 0u) &&
        (gNdsStageMPUpdateFloorLoopP1FloorOK != 0u))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageFloorFollowLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorFollowLoopFinalAdoptCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalAdoptCount == 0u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageMPCrossFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPSweepFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPUpdateFloorLoopUnsafeCount == 0u))
    {
        mask |= 1u << 13;
    }
    if (gNdsStageMPCrossFloorLoopP0CrossHitCount >
        gNdsStageMPCrossFloorLoopP1CrossHitCount)
    {
        mask |= 1u << 14;
    }
    gNdsFighterMarioFoxStageMPCrossFloorLoopCount =
        gNdsFighterMarioFoxStageMPSweepFloorLoopCount;
    if (gNdsFighterMarioFoxStageMPCrossFloorLoopCount ==
        gNdsFighterMarioFoxStageMPSweepFloorLoopCount)
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPCrossFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPCrossFloorLoopDeferredMask = 0xffu;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageMPCrossFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCrossFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPAdjustFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPAdjustFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPAdjustFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPAdjustFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPAdjustFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPAdjustFloorLoopCount = 0u;
    gNdsStageMPAdjustFloorLoopPrepared = 0u;
    gNdsStageMPAdjustFloorLoopBaseMPCrossSeen = 0u;
    gNdsStageMPAdjustFloorLoopRunCallCount = 0u;
    gNdsStageMPAdjustFloorLoopCheckLCallCount = 0u;
    gNdsStageMPAdjustFloorLoopCheckRCallCount = 0u;
    gNdsStageMPAdjustFloorLoopCheckLHitCount = 0u;
    gNdsStageMPAdjustFloorLoopCheckRHitCount = 0u;
    gNdsStageMPAdjustFloorLoopCheckLMissCount = 0u;
    gNdsStageMPAdjustFloorLoopCheckRMissCount = 0u;
    gNdsStageMPAdjustFloorLoopWallLCallCount = 0u;
    gNdsStageMPAdjustFloorLoopWallRCallCount = 0u;
    gNdsStageMPAdjustFloorLoopWallLHitCount = 0u;
    gNdsStageMPAdjustFloorLoopWallRHitCount = 0u;
    gNdsStageMPAdjustFloorLoopWallLMissCount = 0u;
    gNdsStageMPAdjustFloorLoopWallRMissCount = 0u;
    gNdsStageMPAdjustFloorLoopEdgeUnderLCallCount = 0u;
    gNdsStageMPAdjustFloorLoopEdgeUnderRCallCount = 0u;
    gNdsStageMPAdjustFloorLoopEdgeUnderDeferredCount = 0u;
    gNdsStageMPAdjustFloorLoopAdjustLCallCount = 0u;
    gNdsStageMPAdjustFloorLoopAdjustRCallCount = 0u;
    gNdsStageMPAdjustFloorLoopNoAdjustCount = 0u;
    gNdsStageMPAdjustFloorLoopP0RunCount = 0u;
    gNdsStageMPAdjustFloorLoopP1RunCount = 0u;
    gNdsStageMPAdjustFloorLoopP0FinalLineID = -1;
    gNdsStageMPAdjustFloorLoopP1FinalLineID = -1;
    gNdsStageMPAdjustFloorLoopP0FinalFloorOK = 0u;
    gNdsStageMPAdjustFloorLoopP1FinalFloorOK = 0u;
    gNdsStageMPAdjustFloorLoopUnsafeCount = 0u;
}

void ndsFighterMarioFoxStageMPAdjustFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPAdjustFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPAdjustFloorLoopReset();
    gNdsStageMPAdjustFloorLoopPrepared = 1u;
    gNdsStageMPAdjustFloorLoopBaseMPCrossSeen =
        (gNdsStageMPCrossFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPAdjustFloorLoopBaseMPCrossSeen == 0u)
    {
        gNdsStageMPAdjustFloorLoopUnsafeCount++;
    }
}

void ndsFighterMarioFoxStageMPAdjustFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPAdjustFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPAdjustFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPCrossFloorLoopResult ==
        NDS_FIGHTER_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP_PASS)
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPAdjustFloorLoopPrepared != 0u) &&
        (gNdsStageMPAdjustFloorLoopBaseMPCrossSeen != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPAdjustFloorLoopRunCallCount > 0u) &&
        (gNdsStageMPSweepFloorLoopFloorEdgeAdjustCallCount > 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPAdjustFloorLoopCheckLCallCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopCheckRCallCount > 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPAdjustFloorLoopEdgeUnderLCallCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopEdgeUnderRCallCount > 0u) &&
        (((ndsFighterMarioFoxStageMPEdgeFloorLoopProofEnabled() != FALSE) &&
          (gNdsStageMPAdjustFloorLoopEdgeUnderDeferredCount == 0u)) ||
         ((ndsFighterMarioFoxStageMPEdgeFloorLoopProofEnabled() == FALSE) &&
          (gNdsStageMPAdjustFloorLoopEdgeUnderDeferredCount ==
            (gNdsStageMPAdjustFloorLoopEdgeUnderLCallCount +
             gNdsStageMPAdjustFloorLoopEdgeUnderRCallCount)))))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPAdjustFloorLoopWallLCallCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopWallRCallCount > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPAdjustFloorLoopWallLHitCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopWallRHitCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopWallLMissCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopWallRMissCount > 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPAdjustFloorLoopCheckLHitCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopCheckRHitCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopCheckLMissCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopCheckRMissCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopNoAdjustCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopAdjustLCallCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopAdjustRCallCount == 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPAdjustFloorLoopP0RunCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopP1HitCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopP1FloorOK != 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPAdjustFloorLoopP0FinalLineID >= 0) &&
        (gNdsStageMPUpdateFloorLoopP1FinalLineID >= 0) &&
        (gNdsStageMPAdjustFloorLoopP0FinalFloorOK != 0u) &&
        (gNdsStageMPUpdateFloorLoopP1FloorOK != 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPCrossFloorLoopLiveSecondFloorHitCount > 0u) &&
        (gNdsStageMPCrossFloorLoopLiveAcceptedNewLineCount > 0u))
    {
        mask |= 1u << 10;
    }
    if (gNdsStageMPSweepFloorLoopFloorEdgeAdjustDeferredCount == 0u)
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPAdjustFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPCrossFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPSweepFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPUpdateFloorLoopUnsafeCount == 0u))
    {
        mask |= 1u << 12;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageMPUpdateFloorLoopP0HitCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopP1HitCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopP0HitCount >
            gNdsStageMPUpdateFloorLoopP0MissCount) &&
        (gNdsStageMPUpdateFloorLoopP1HitCount >
            gNdsStageMPUpdateFloorLoopP1MissCount) &&
        (gNdsStageMPUpdateFloorLoopP0FloorOK != 0u) &&
        (gNdsStageMPUpdateFloorLoopP1FloorOK != 0u))
    {
        mask |= 1u << 14;
    }
    gNdsFighterMarioFoxStageMPAdjustFloorLoopCount =
        gNdsFighterMarioFoxStageMPCrossFloorLoopCount;
    if (gNdsFighterMarioFoxStageMPAdjustFloorLoopCount ==
        gNdsFighterMarioFoxStageMPCrossFloorLoopCount)
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPAdjustFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPAdjustFloorLoopDeferredMask = 0xffu;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageMPAdjustFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPAdjustFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPEdgeFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPEdgeFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPEdgeFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPEdgeFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPEdgeFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPEdgeFloorLoopCount = 0u;
    gNdsStageMPEdgeFloorLoopPrepared = 0u;
    gNdsStageMPEdgeFloorLoopBaseMPAdjustSeen = 0u;
    gNdsStageMPEdgeFloorLoopEdgeUnderLCallCount = 0u;
    gNdsStageMPEdgeFloorLoopEdgeUnderRCallCount = 0u;
    gNdsStageMPEdgeFloorLoopEdgeUnderLHitCount = 0u;
    gNdsStageMPEdgeFloorLoopEdgeUnderRHitCount = 0u;
    gNdsStageMPEdgeFloorLoopEdgeUnderLMissCount = 0u;
    gNdsStageMPEdgeFloorLoopEdgeUnderRMissCount = 0u;
    gNdsStageMPEdgeFloorLoopEdgeUnderLLineID = -1;
    gNdsStageMPEdgeFloorLoopEdgeUnderRLineID = -1;
    gNdsStageMPEdgeFloorLoopEdgeUnderLLineKind = 0xffffffffu;
    gNdsStageMPEdgeFloorLoopEdgeUnderRLineKind = 0xffffffffu;
    gNdsStageMPEdgeFloorLoopSelectedFloorLineID = -1;
    gNdsStageMPEdgeFloorLoopSelectedFloorOK = 0u;
    gNdsStageMPEdgeFloorLoopUnsafeCount = 0u;
}

void ndsFighterMarioFoxStageMPEdgeFloorLoopPrepare(void)
{
    s32 selected_line_id;

    if ((ndsFighterMarioFoxStageMPEdgeFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPEdgeFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPEdgeFloorLoopReset();
    gNdsStageMPEdgeFloorLoopPrepared = 1u;
    gNdsStageMPEdgeFloorLoopBaseMPAdjustSeen =
        (gNdsStageMPAdjustFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPEdgeFloorLoopBaseMPAdjustSeen == 0u)
    {
        gNdsStageMPEdgeFloorLoopUnsafeCount++;
    }
    selected_line_id = gNdsStageFloorEdgeLoopSelectedLineID;
    gNdsStageMPEdgeFloorLoopSelectedFloorLineID = selected_line_id;
    gNdsStageMPEdgeFloorLoopSelectedFloorOK =
        (ndsMPLineIDIsFloor(selected_line_id) != FALSE) ? 1u : 0u;
    if (gNdsStageMPEdgeFloorLoopSelectedFloorOK == 0u)
    {
        gNdsStageMPEdgeFloorLoopUnsafeCount++;
        return;
    }
    (void)mpCollisionGetEdgeUnderLLineID(selected_line_id);
    (void)mpCollisionGetEdgeUnderRLineID(selected_line_id);
}

static sb32 ndsStageMPEdgeFloorLoopKindIsWall(u32 line_kind)
{
    return ((line_kind == (u32)nMPLineKindLWall) ||
            (line_kind == (u32)nMPLineKindRWall)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxStageMPEdgeFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPEdgeFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPEdgeFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPEdgeFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPAdjustFloorLoopResult ==
        NDS_FIGHTER_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP_PASS)
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPEdgeFloorLoopPrepared != 0u) &&
        (gNdsStageMPEdgeFloorLoopBaseMPAdjustSeen != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPEdgeFloorLoopSelectedFloorLineID >= 0) &&
        (gNdsStageMPEdgeFloorLoopSelectedFloorOK != 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPEdgeFloorLoopEdgeUnderLCallCount > 0u) &&
        (gNdsStageMPEdgeFloorLoopEdgeUnderRCallCount > 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPEdgeFloorLoopEdgeUnderLHitCount > 0u) &&
        (gNdsStageMPEdgeFloorLoopEdgeUnderRHitCount > 0u) &&
        (gNdsStageMPEdgeFloorLoopEdgeUnderLMissCount == 0u) &&
        (gNdsStageMPEdgeFloorLoopEdgeUnderRMissCount == 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPEdgeFloorLoopEdgeUnderLLineID >= 0) &&
        (gNdsStageMPEdgeFloorLoopEdgeUnderRLineID >= 0) &&
        (gNdsStageMPEdgeFloorLoopEdgeUnderLLineID !=
            gNdsStageMPEdgeFloorLoopEdgeUnderRLineID))
    {
        mask |= 1u << 5;
    }
    if ((ndsStageMPEdgeFloorLoopKindIsWall(
            gNdsStageMPEdgeFloorLoopEdgeUnderLLineKind) != FALSE) &&
        (ndsStageMPEdgeFloorLoopKindIsWall(
            gNdsStageMPEdgeFloorLoopEdgeUnderRLineKind) != FALSE))
    {
        mask |= 1u << 6;
    }
    if (gNdsStageMPAdjustFloorLoopEdgeUnderDeferredCount == 0u)
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPAdjustFloorLoopRunCallCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopCheckLCallCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopCheckRCallCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPAdjustFloorLoopWallLCallCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopWallRCallCount > 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPAdjustFloorLoopNoAdjustCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopAdjustLCallCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopAdjustRCallCount == 0u))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPAdjustFloorLoopP0FinalLineID >= 0) &&
        (gNdsStageMPUpdateFloorLoopP1FinalLineID >= 0) &&
        (gNdsStageMPAdjustFloorLoopP0FinalFloorOK != 0u) &&
        (gNdsStageMPUpdateFloorLoopP1FloorOK != 0u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPEdgeFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopUnsafeCount == 0u))
    {
        mask |= 1u << 12;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 13;
    }
    gNdsFighterMarioFoxStageMPEdgeFloorLoopCount =
        gNdsFighterMarioFoxStageMPAdjustFloorLoopCount;
    if (gNdsFighterMarioFoxStageMPEdgeFloorLoopCount ==
        gNdsFighterMarioFoxStageMPAdjustFloorLoopCount)
    {
        mask |= 1u << 14;
    }
    if (gNdsStageFloorEdgeLoopEdgeUnderDeferredCount >=
        (gNdsStageFloorEdgeLoopEdgeUnderLCallCount +
         gNdsStageFloorEdgeLoopEdgeUnderRCallCount))
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPEdgeFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPEdgeFloorLoopDeferredMask = 0xffu;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageMPEdgeFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPEdgeFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsStageMPWallHitScoutClearRuntimeGlobals(void)
{
    gNdsStageMPWallHitScoutRunCount = 0u;
    gNdsStageMPWallHitScoutFloorTestCount = 0u;
    gNdsStageMPWallHitScoutWallTestCount = 0u;
    gNdsStageMPWallHitScoutCandidateCount = 0u;
    gNdsStageMPWallHitScoutHitCount = 0u;
    gNdsStageMPWallHitScoutMissCount = 0u;
    gNdsStageMPWallHitScoutFloorLineID = -1;
    gNdsStageMPWallHitScoutWallLineID = -1;
    gNdsStageMPWallHitScoutEdgeUnderLineID = -1;
    gNdsStageMPWallHitScoutSide = 0xffffffffu;
    gNdsStageMPWallHitScoutWallLineKind = 0xffffffffu;
    gNdsStageMPWallHitScoutStartXMilli = 0;
    gNdsStageMPWallHitScoutStartYMilli = 0;
    gNdsStageMPWallHitScoutFinalXMilli = 0;
    gNdsStageMPWallHitScoutFinalYMilli = 0;
    gNdsStageMPWallHitScoutDeltaXMilli = 0;
    gNdsStageMPWallHitScoutDeltaYMilli = 0;
    gNdsStageMPWallHitScoutFinalFloorOK = 0u;
    gNdsStageMPWallHitScoutUnsafeCount = 0u;
    sNdsStageMPWallHitScoutWidth = 0.0F;
    sNdsStageMPWallHitScoutCenter = 0.0F;
}

static void ndsStageMPWallHyruleHitScoutClearGlobals(void)
{
    gNdsStageMPWallHyruleScoutRelocResult = 0u;
    gNdsStageMPWallHyruleScoutGroundDataReady = 0u;
    gNdsStageMPWallHyruleScoutGeometryReady = 0u;
    gNdsStageMPWallHyruleScoutMapNodesReady = 0u;
    gNdsStageMPWallHyruleScoutRunCount = 0u;
    gNdsStageMPWallHyruleScoutFloorTestCount = 0u;
    gNdsStageMPWallHyruleScoutWallTestCount = 0u;
    gNdsStageMPWallHyruleScoutCandidateCount = 0u;
    gNdsStageMPWallHyruleScoutHitCount = 0u;
    gNdsStageMPWallHyruleScoutMissCount = 0u;
    gNdsStageMPWallHyruleScoutFloorLineID = -1;
    gNdsStageMPWallHyruleScoutWallLineID = -1;
    gNdsStageMPWallHyruleScoutEdgeUnderLineID = -1;
    gNdsStageMPWallHyruleScoutSide = 0xffffffffu;
    gNdsStageMPWallHyruleScoutWallLineKind = 0xffffffffu;
    gNdsStageMPWallHyruleScoutStartXMilli = 0;
    gNdsStageMPWallHyruleScoutStartYMilli = 0;
    gNdsStageMPWallHyruleScoutFinalXMilli = 0;
    gNdsStageMPWallHyruleScoutFinalYMilli = 0;
    gNdsStageMPWallHyruleScoutDeltaXMilli = 0;
    gNdsStageMPWallHyruleScoutDeltaYMilli = 0;
    gNdsStageMPWallHyruleScoutFinalFloorOK = 0u;
    gNdsStageMPWallHyruleScoutUnsafeCount = 0u;
}

static void ndsStageMPWallHitFloorLoopClearGlobals(void)
{
    gNdsFighterMarioFoxStageMPWallHitFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPWallHitFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPWallHitFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPWallHitFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPWallHitFloorLoopCount = 0u;
    gNdsStageMPWallHitFloorLoopPrepared = 0u;
    gNdsStageMPWallHitFloorLoopBaseMPWallSeen = 0u;
    gNdsStageMPWallHitFloorLoopRelocResult = 0u;
    gNdsStageMPWallHitFloorLoopGroundDataReady = 0u;
    gNdsStageMPWallHitFloorLoopGeometryReady = 0u;
    gNdsStageMPWallHitFloorLoopMapNodesReady = 0u;
    gNdsStageMPWallHitFloorLoopRunCount = 0u;
    gNdsStageMPWallHitFloorLoopFloorTestCount = 0u;
    gNdsStageMPWallHitFloorLoopWallTestCount = 0u;
    gNdsStageMPWallHitFloorLoopCandidateCount = 0u;
    gNdsStageMPWallHitFloorLoopHitCount = 0u;
    gNdsStageMPWallHitFloorLoopMissCount = 0u;
    gNdsStageMPWallHitFloorLoopFloorLineID = -1;
    gNdsStageMPWallHitFloorLoopWallLineID = -1;
    gNdsStageMPWallHitFloorLoopEdgeUnderLineID = -1;
    gNdsStageMPWallHitFloorLoopSide = 0xffffffffu;
    gNdsStageMPWallHitFloorLoopWallLineKind = 0xffffffffu;
    gNdsStageMPWallHitFloorLoopStartXMilli = 0;
    gNdsStageMPWallHitFloorLoopStartYMilli = 0;
    gNdsStageMPWallHitFloorLoopFinalXMilli = 0;
    gNdsStageMPWallHitFloorLoopFinalYMilli = 0;
    gNdsStageMPWallHitFloorLoopDeltaXMilli = 0;
    gNdsStageMPWallHitFloorLoopDeltaYMilli = 0;
    gNdsStageMPWallHitFloorLoopFinalFloorOK = 0u;
    gNdsStageMPWallHitFloorLoopUnsafeCount = 0u;
}

static void ndsFighterMarioFoxStageMPWallCopyFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPWallCopyFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPWallCopyFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPWallCopyFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPWallCopyFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPWallCopyFloorLoopCount = 0u;
    gNdsStageMPWallCopyFloorLoopPrepared = 0u;
    gNdsStageMPWallCopyFloorLoopBaseMPWallHitSeen = 0u;
    gNdsStageMPWallCopyFloorLoopProcessAttachCount = 0u;
    gNdsStageMPWallCopyFloorLoopGObjProcessRunCount = 0u;
    gNdsStageMPWallCopyFloorLoopCallbackCount = 0u;
    gNdsStageMPWallCopyFloorLoopCopyBackCount = 0u;
    gNdsStageMPWallCopyFloorLoopSourceFloorLineID = 0xffffffffu;
    gNdsStageMPWallCopyFloorLoopSourceWallLineID = 0xffffffffu;
    gNdsStageMPWallCopyFloorLoopSourceEdgeLineID = 0xffffffffu;
    gNdsStageMPWallCopyFloorLoopSourceSide = 0xffffffffu;
    gNdsStageMPWallCopyFloorLoopStartXMilli = 0;
    gNdsStageMPWallCopyFloorLoopStartYMilli = 0;
    gNdsStageMPWallCopyFloorLoopFinalXMilli = 0;
    gNdsStageMPWallCopyFloorLoopFinalYMilli = 0;
    gNdsStageMPWallCopyFloorLoopDeltaXMilli = 0;
    gNdsStageMPWallCopyFloorLoopDeltaYMilli = 0;
    gNdsStageMPWallCopyFloorLoopP1RootXBeforeMilli = 0;
    gNdsStageMPWallCopyFloorLoopP1RootYBeforeMilli = 0;
    gNdsStageMPWallCopyFloorLoopP1RootXAfterMilli = 0;
    gNdsStageMPWallCopyFloorLoopP1RootYAfterMilli = 0;
    gNdsStageMPWallCopyFloorLoopP0FinalFloorOK = 0u;
    gNdsStageMPWallCopyFloorLoopP0FinalMaskStat = 0u;
    gNdsStageMPWallCopyFloorLoopP0FinalGA = 0u;
    gNdsStageMPWallCopyFloorLoopUnsafeCount = 0u;
    sNdsStageMPWallCopyFloorLoopProcess = NULL;
}

static void ndsFighterMarioFoxStageMPWallFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPWallFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPWallFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPWallFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPWallFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPWallFloorLoopCount = 0u;
    gNdsStageMPWallFloorLoopPrepared = 0u;
    gNdsStageMPWallFloorLoopBaseMPEdgeSeen = 0u;
    gNdsStageMPWallFloorLoopProbeCount = 0u;
    gNdsStageMPWallFloorLoopProbeHitCount = 0u;
    gNdsStageMPWallFloorLoopProbeMissCount = 0u;
    gNdsStageMPWallFloorLoopFloorLineID = -1;
    gNdsStageMPWallFloorLoopWallLineID = -1;
    gNdsStageMPWallFloorLoopWallLineKind = 0xffffffffu;
    gNdsStageMPWallFloorLoopEdgeUnderLineID = -1;
    gNdsStageMPWallFloorLoopSide = 0xffffffffu;
    gNdsStageMPWallFloorLoopCheckHitCount = 0u;
    gNdsStageMPWallFloorLoopAdjustCallCount = 0u;
    gNdsStageMPWallFloorLoopStartXMilli = 0;
    gNdsStageMPWallFloorLoopStartYMilli = 0;
    gNdsStageMPWallFloorLoopFinalXMilli = 0;
    gNdsStageMPWallFloorLoopFinalYMilli = 0;
    gNdsStageMPWallFloorLoopDeltaXMilli = 0;
    gNdsStageMPWallFloorLoopDeltaYMilli = 0;
    gNdsStageMPWallFloorLoopFinalFloorOK = 0u;
    gNdsStageMPWallFloorLoopUnsafeCount = 0u;
    ndsStageMPWallHitScoutClearRuntimeGlobals();
    ndsStageMPWallHyruleHitScoutClearGlobals();
    ndsStageMPWallHitFloorLoopClearGlobals();
    ndsFighterMarioFoxStageMPWallCopyFloorLoopReset();
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

static void ndsStageMPWallHitScoutSaveRuntimeGlobals(
    NDSStageMPWallHitScoutResultSnapshot *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }
    snapshot->run_count = gNdsStageMPWallHitScoutRunCount;
    snapshot->floor_test_count = gNdsStageMPWallHitScoutFloorTestCount;
    snapshot->wall_test_count = gNdsStageMPWallHitScoutWallTestCount;
    snapshot->candidate_count = gNdsStageMPWallHitScoutCandidateCount;
    snapshot->hit_count = gNdsStageMPWallHitScoutHitCount;
    snapshot->miss_count = gNdsStageMPWallHitScoutMissCount;
    snapshot->floor_line_id = gNdsStageMPWallHitScoutFloorLineID;
    snapshot->wall_line_id = gNdsStageMPWallHitScoutWallLineID;
    snapshot->edge_under_line_id = gNdsStageMPWallHitScoutEdgeUnderLineID;
    snapshot->side = gNdsStageMPWallHitScoutSide;
    snapshot->wall_line_kind = gNdsStageMPWallHitScoutWallLineKind;
    snapshot->start_x_milli = gNdsStageMPWallHitScoutStartXMilli;
    snapshot->start_y_milli = gNdsStageMPWallHitScoutStartYMilli;
    snapshot->final_x_milli = gNdsStageMPWallHitScoutFinalXMilli;
    snapshot->final_y_milli = gNdsStageMPWallHitScoutFinalYMilli;
    snapshot->delta_x_milli = gNdsStageMPWallHitScoutDeltaXMilli;
    snapshot->delta_y_milli = gNdsStageMPWallHitScoutDeltaYMilli;
    snapshot->final_floor_ok = gNdsStageMPWallHitScoutFinalFloorOK;
    snapshot->unsafe_count = gNdsStageMPWallHitScoutUnsafeCount;
}

static void ndsStageMPWallHitScoutRestoreRuntimeGlobals(
    const NDSStageMPWallHitScoutResultSnapshot *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }
    gNdsStageMPWallHitScoutRunCount = snapshot->run_count;
    gNdsStageMPWallHitScoutFloorTestCount = snapshot->floor_test_count;
    gNdsStageMPWallHitScoutWallTestCount = snapshot->wall_test_count;
    gNdsStageMPWallHitScoutCandidateCount = snapshot->candidate_count;
    gNdsStageMPWallHitScoutHitCount = snapshot->hit_count;
    gNdsStageMPWallHitScoutMissCount = snapshot->miss_count;
    gNdsStageMPWallHitScoutFloorLineID = snapshot->floor_line_id;
    gNdsStageMPWallHitScoutWallLineID = snapshot->wall_line_id;
    gNdsStageMPWallHitScoutEdgeUnderLineID = snapshot->edge_under_line_id;
    gNdsStageMPWallHitScoutSide = snapshot->side;
    gNdsStageMPWallHitScoutWallLineKind = snapshot->wall_line_kind;
    gNdsStageMPWallHitScoutStartXMilli = snapshot->start_x_milli;
    gNdsStageMPWallHitScoutStartYMilli = snapshot->start_y_milli;
    gNdsStageMPWallHitScoutFinalXMilli = snapshot->final_x_milli;
    gNdsStageMPWallHitScoutFinalYMilli = snapshot->final_y_milli;
    gNdsStageMPWallHitScoutDeltaXMilli = snapshot->delta_x_milli;
    gNdsStageMPWallHitScoutDeltaYMilli = snapshot->delta_y_milli;
    gNdsStageMPWallHitScoutFinalFloorOK = snapshot->final_floor_ok;
    gNdsStageMPWallHitScoutUnsafeCount = snapshot->unsafe_count;
}

static void ndsStageMPWallHitScoutCopyRuntimeGlobalsToHyrule(void)
{
    gNdsStageMPWallHyruleScoutRunCount = gNdsStageMPWallHitScoutRunCount;
    gNdsStageMPWallHyruleScoutFloorTestCount =
        gNdsStageMPWallHitScoutFloorTestCount;
    gNdsStageMPWallHyruleScoutWallTestCount =
        gNdsStageMPWallHitScoutWallTestCount;
    gNdsStageMPWallHyruleScoutCandidateCount =
        gNdsStageMPWallHitScoutCandidateCount;
    gNdsStageMPWallHyruleScoutHitCount = gNdsStageMPWallHitScoutHitCount;
    gNdsStageMPWallHyruleScoutMissCount = gNdsStageMPWallHitScoutMissCount;
    gNdsStageMPWallHyruleScoutFloorLineID =
        gNdsStageMPWallHitScoutFloorLineID;
    gNdsStageMPWallHyruleScoutWallLineID =
        gNdsStageMPWallHitScoutWallLineID;
    gNdsStageMPWallHyruleScoutEdgeUnderLineID =
        gNdsStageMPWallHitScoutEdgeUnderLineID;
    gNdsStageMPWallHyruleScoutSide = gNdsStageMPWallHitScoutSide;
    gNdsStageMPWallHyruleScoutWallLineKind =
        gNdsStageMPWallHitScoutWallLineKind;
    gNdsStageMPWallHyruleScoutStartXMilli =
        gNdsStageMPWallHitScoutStartXMilli;
    gNdsStageMPWallHyruleScoutStartYMilli =
        gNdsStageMPWallHitScoutStartYMilli;
    gNdsStageMPWallHyruleScoutFinalXMilli =
        gNdsStageMPWallHitScoutFinalXMilli;
    gNdsStageMPWallHyruleScoutFinalYMilli =
        gNdsStageMPWallHitScoutFinalYMilli;
    gNdsStageMPWallHyruleScoutDeltaXMilli =
        gNdsStageMPWallHitScoutDeltaXMilli;
    gNdsStageMPWallHyruleScoutDeltaYMilli =
        gNdsStageMPWallHitScoutDeltaYMilli;
    gNdsStageMPWallHyruleScoutFinalFloorOK =
        gNdsStageMPWallHitScoutFinalFloorOK;
    gNdsStageMPWallHyruleScoutUnsafeCount =
        gNdsStageMPWallHitScoutUnsafeCount;
}

static void ndsStageMPWallHitScoutSaveCounters(
    NDSStageMPWallHitScoutCounterSnapshot *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }
    snapshot->run_call_count = gNdsStageMPAdjustFloorLoopRunCallCount;
    snapshot->check_l_call_count = gNdsStageMPAdjustFloorLoopCheckLCallCount;
    snapshot->check_r_call_count = gNdsStageMPAdjustFloorLoopCheckRCallCount;
    snapshot->check_l_hit_count = gNdsStageMPAdjustFloorLoopCheckLHitCount;
    snapshot->check_r_hit_count = gNdsStageMPAdjustFloorLoopCheckRHitCount;
    snapshot->check_l_miss_count = gNdsStageMPAdjustFloorLoopCheckLMissCount;
    snapshot->check_r_miss_count = gNdsStageMPAdjustFloorLoopCheckRMissCount;
    snapshot->wall_l_call_count = gNdsStageMPAdjustFloorLoopWallLCallCount;
    snapshot->wall_r_call_count = gNdsStageMPAdjustFloorLoopWallRCallCount;
    snapshot->wall_l_hit_count = gNdsStageMPAdjustFloorLoopWallLHitCount;
    snapshot->wall_r_hit_count = gNdsStageMPAdjustFloorLoopWallRHitCount;
    snapshot->wall_l_miss_count = gNdsStageMPAdjustFloorLoopWallLMissCount;
    snapshot->wall_r_miss_count = gNdsStageMPAdjustFloorLoopWallRMissCount;
    snapshot->edge_under_l_call_count =
        gNdsStageMPAdjustFloorLoopEdgeUnderLCallCount;
    snapshot->edge_under_r_call_count =
        gNdsStageMPAdjustFloorLoopEdgeUnderRCallCount;
    snapshot->edge_under_deferred_count =
        gNdsStageMPAdjustFloorLoopEdgeUnderDeferredCount;
    snapshot->adjust_l_call_count =
        gNdsStageMPAdjustFloorLoopAdjustLCallCount;
    snapshot->adjust_r_call_count =
        gNdsStageMPAdjustFloorLoopAdjustRCallCount;
    snapshot->no_adjust_count = gNdsStageMPAdjustFloorLoopNoAdjustCount;
    snapshot->p0_run_count = gNdsStageMPAdjustFloorLoopP0RunCount;
    snapshot->p1_run_count = gNdsStageMPAdjustFloorLoopP1RunCount;
    snapshot->p0_final_line_id = gNdsStageMPAdjustFloorLoopP0FinalLineID;
    snapshot->p1_final_line_id = gNdsStageMPAdjustFloorLoopP1FinalLineID;
    snapshot->p0_final_floor_ok = gNdsStageMPAdjustFloorLoopP0FinalFloorOK;
    snapshot->p1_final_floor_ok = gNdsStageMPAdjustFloorLoopP1FinalFloorOK;
    snapshot->unsafe_count = gNdsStageMPAdjustFloorLoopUnsafeCount;
    snapshot->edge_loop_l_call_count =
        gNdsStageMPEdgeFloorLoopEdgeUnderLCallCount;
    snapshot->edge_loop_r_call_count =
        gNdsStageMPEdgeFloorLoopEdgeUnderRCallCount;
    snapshot->edge_loop_l_hit_count =
        gNdsStageMPEdgeFloorLoopEdgeUnderLHitCount;
    snapshot->edge_loop_r_hit_count =
        gNdsStageMPEdgeFloorLoopEdgeUnderRHitCount;
    snapshot->edge_loop_l_miss_count =
        gNdsStageMPEdgeFloorLoopEdgeUnderLMissCount;
    snapshot->edge_loop_r_miss_count =
        gNdsStageMPEdgeFloorLoopEdgeUnderRMissCount;
    snapshot->edge_loop_l_line_id =
        gNdsStageMPEdgeFloorLoopEdgeUnderLLineID;
    snapshot->edge_loop_r_line_id =
        gNdsStageMPEdgeFloorLoopEdgeUnderRLineID;
    snapshot->edge_loop_l_line_kind =
        gNdsStageMPEdgeFloorLoopEdgeUnderLLineKind;
    snapshot->edge_loop_r_line_kind =
        gNdsStageMPEdgeFloorLoopEdgeUnderRLineKind;
}

static void ndsStageMPWallHitScoutRestoreCounters(
    const NDSStageMPWallHitScoutCounterSnapshot *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }
    gNdsStageMPAdjustFloorLoopRunCallCount = snapshot->run_call_count;
    gNdsStageMPAdjustFloorLoopCheckLCallCount =
        snapshot->check_l_call_count;
    gNdsStageMPAdjustFloorLoopCheckRCallCount =
        snapshot->check_r_call_count;
    gNdsStageMPAdjustFloorLoopCheckLHitCount =
        snapshot->check_l_hit_count;
    gNdsStageMPAdjustFloorLoopCheckRHitCount =
        snapshot->check_r_hit_count;
    gNdsStageMPAdjustFloorLoopCheckLMissCount =
        snapshot->check_l_miss_count;
    gNdsStageMPAdjustFloorLoopCheckRMissCount =
        snapshot->check_r_miss_count;
    gNdsStageMPAdjustFloorLoopWallLCallCount =
        snapshot->wall_l_call_count;
    gNdsStageMPAdjustFloorLoopWallRCallCount =
        snapshot->wall_r_call_count;
    gNdsStageMPAdjustFloorLoopWallLHitCount =
        snapshot->wall_l_hit_count;
    gNdsStageMPAdjustFloorLoopWallRHitCount =
        snapshot->wall_r_hit_count;
    gNdsStageMPAdjustFloorLoopWallLMissCount =
        snapshot->wall_l_miss_count;
    gNdsStageMPAdjustFloorLoopWallRMissCount =
        snapshot->wall_r_miss_count;
    gNdsStageMPAdjustFloorLoopEdgeUnderLCallCount =
        snapshot->edge_under_l_call_count;
    gNdsStageMPAdjustFloorLoopEdgeUnderRCallCount =
        snapshot->edge_under_r_call_count;
    gNdsStageMPAdjustFloorLoopEdgeUnderDeferredCount =
        snapshot->edge_under_deferred_count;
    gNdsStageMPAdjustFloorLoopAdjustLCallCount =
        snapshot->adjust_l_call_count;
    gNdsStageMPAdjustFloorLoopAdjustRCallCount =
        snapshot->adjust_r_call_count;
    gNdsStageMPAdjustFloorLoopNoAdjustCount = snapshot->no_adjust_count;
    gNdsStageMPAdjustFloorLoopP0RunCount = snapshot->p0_run_count;
    gNdsStageMPAdjustFloorLoopP1RunCount = snapshot->p1_run_count;
    gNdsStageMPAdjustFloorLoopP0FinalLineID = snapshot->p0_final_line_id;
    gNdsStageMPAdjustFloorLoopP1FinalLineID = snapshot->p1_final_line_id;
    gNdsStageMPAdjustFloorLoopP0FinalFloorOK =
        snapshot->p0_final_floor_ok;
    gNdsStageMPAdjustFloorLoopP1FinalFloorOK =
        snapshot->p1_final_floor_ok;
    gNdsStageMPAdjustFloorLoopUnsafeCount = snapshot->unsafe_count;
    gNdsStageMPEdgeFloorLoopEdgeUnderLCallCount =
        snapshot->edge_loop_l_call_count;
    gNdsStageMPEdgeFloorLoopEdgeUnderRCallCount =
        snapshot->edge_loop_r_call_count;
    gNdsStageMPEdgeFloorLoopEdgeUnderLHitCount =
        snapshot->edge_loop_l_hit_count;
    gNdsStageMPEdgeFloorLoopEdgeUnderRHitCount =
        snapshot->edge_loop_r_hit_count;
    gNdsStageMPEdgeFloorLoopEdgeUnderLMissCount =
        snapshot->edge_loop_l_miss_count;
    gNdsStageMPEdgeFloorLoopEdgeUnderRMissCount =
        snapshot->edge_loop_r_miss_count;
    gNdsStageMPEdgeFloorLoopEdgeUnderLLineID =
        snapshot->edge_loop_l_line_id;
    gNdsStageMPEdgeFloorLoopEdgeUnderRLineID =
        snapshot->edge_loop_r_line_id;
    gNdsStageMPEdgeFloorLoopEdgeUnderLLineKind =
        snapshot->edge_loop_l_line_kind;
    gNdsStageMPEdgeFloorLoopEdgeUnderRLineKind =
        snapshot->edge_loop_r_line_kind;
}

static sb32 ndsStageMPWallHitScoutTryWall(s32 floor_line_id, u32 side,
                                          s32 wall_line_id,
                                          s32 edge_under_line_id)
{
    static const f32 widths[] = {
        32.0F, 48.0F, 64.0F, 96.0F, 128.0F, 192.0F, 256.0F, 384.0F
    };
    Vec3f wall_a;
    Vec3f wall_b;
    u32 line_kind;
    u32 sample;
    u32 width_index;

    if ((floor_line_id < 0) || (wall_line_id < 0) ||
        (ndsMPLineIDIsFloor(floor_line_id) == FALSE) ||
        (ndsMPFindLineEndpoints(wall_line_id, &wall_a, &wall_b, NULL,
            NULL) == FALSE))
    {
        return FALSE;
    }

    line_kind = (side == 0u) ? (u32)nMPLineKindLWall :
        (u32)nMPLineKindRWall;
    if ((u32)ndsMPGetLineKindForLineID(wall_line_id) != line_kind)
    {
        return FALSE;
    }

    gNdsStageMPWallHitScoutWallTestCount++;
    if ((edge_under_line_id >= 0) && (wall_line_id == edge_under_line_id))
    {
        return FALSE;
    }
    gNdsStageMPWallHitScoutCandidateCount++;

    for (sample = 1u; sample <= 5u; sample++)
    {
        f32 ratio = (f32)sample / 6.0F;
        f32 wall_x = wall_a.x + ((wall_b.x - wall_a.x) * ratio);
        f32 wall_y = wall_a.y + ((wall_b.y - wall_a.y) * ratio);

        for (width_index = 0u; width_index < ARRAY_COUNT(widths);
             width_index++)
        {
            NDSStageMPWallHitScoutCounterSnapshot snapshot;
            f32 width = widths[width_index];
            f32 start_x = (side == 0u) ? (wall_x - (width * 0.95F)) :
                (wall_x + (width * 0.95F));
            f32 floor_y = 0.0F;
            f32 center;
            Vec3f b;
            Vec3f a;
            s32 hit_line_id = -1;
            Vec3f translate;
            Vec3f pos_prev;
            Vec3f floor_probe;
            MPCollData coll;
            u32 flags = 0u;
            Vec3f angle;
            u32 check_before;
            u32 adjust_before;
            u32 check_after;
            u32 adjust_after;
            sb32 hit;

            if (ndsStageFloorEdgeLoopFloorYAtX(floor_line_id, start_x,
                    &floor_y) == FALSE)
            {
                continue;
            }
            center = wall_y - floor_y;
            if ((center < 8.0F) || (center > 1024.0F))
            {
                continue;
            }

            b.x = start_x;
            b.y = floor_y;
            b.z = 0.0F;
            a.x = (side == 0u) ? (start_x + width) :
                (start_x - width);
            a.y = floor_y + center;
            a.z = 0.0F;

            ndsStageMPWallHitScoutSaveCounters(&snapshot);
            hit = ndsStageMPAdjustFloorLoopWallSweep(&b, &a, NULL,
                &hit_line_id, NULL, NULL, line_kind, FALSE);
            if ((hit == FALSE) || (hit_line_id < 0) ||
                ((edge_under_line_id >= 0) &&
                 (hit_line_id == edge_under_line_id)))
            {
                ndsStageMPWallHitScoutRestoreCounters(&snapshot);
                continue;
            }

            floor_probe.x = start_x;
            floor_probe.y = floor_y;
            floor_probe.z = 0.0F;
            if (mpCollisionGetFCCommonFloor(floor_line_id, &floor_probe,
                    NULL, &flags, &angle) == FALSE)
            {
                ndsStageMPWallHitScoutRestoreCounters(&snapshot);
                continue;
            }

            translate.x = start_x;
            translate.y = floor_y;
            translate.z = 0.0F;
            pos_prev = translate;
            ndsStageMPUpdateFloorLoopInitColl(&coll, &translate, &pos_prev,
                                              floor_line_id);
            coll.map_coll.center = center;
            coll.map_coll.width = width;
            coll.floor_flags = flags;
            coll.floor_angle = angle;
            coll.update_tic = gMPCollisionUpdateTic;

            check_before = (side == 0u) ?
                gNdsStageMPAdjustFloorLoopCheckLHitCount :
                gNdsStageMPAdjustFloorLoopCheckRHitCount;
            adjust_before = (side == 0u) ?
                gNdsStageMPAdjustFloorLoopAdjustLCallCount :
                gNdsStageMPAdjustFloorLoopAdjustRCallCount;
            mpProcessRunFloorEdgeAdjust(&coll);
            check_after = (side == 0u) ?
                gNdsStageMPAdjustFloorLoopCheckLHitCount :
                gNdsStageMPAdjustFloorLoopCheckRHitCount;
            adjust_after = (side == 0u) ?
                gNdsStageMPAdjustFloorLoopAdjustLCallCount :
                gNdsStageMPAdjustFloorLoopAdjustRCallCount;

            if ((check_after > check_before) &&
                (adjust_after > adjust_before))
            {
                gNdsStageMPWallHitScoutHitCount++;
                gNdsStageMPWallHitScoutFloorLineID = floor_line_id;
                gNdsStageMPWallHitScoutWallLineID = coll.ewall_line_id;
                gNdsStageMPWallHitScoutEdgeUnderLineID =
                    edge_under_line_id;
                gNdsStageMPWallHitScoutSide = side;
                gNdsStageMPWallHitScoutWallLineKind =
                    (u32)ndsMPGetLineKindForLineID(coll.ewall_line_id);
                gNdsStageMPWallHitScoutStartXMilli =
                    ndsFloatToMilliSigned(start_x);
                gNdsStageMPWallHitScoutStartYMilli =
                    ndsFloatToMilliSigned(floor_y);
                gNdsStageMPWallHitScoutFinalXMilli =
                    ndsFloatToMilliSigned(translate.x);
                gNdsStageMPWallHitScoutFinalYMilli =
                    ndsFloatToMilliSigned(translate.y);
                gNdsStageMPWallHitScoutDeltaXMilli =
                    gNdsStageMPWallHitScoutFinalXMilli -
                    gNdsStageMPWallHitScoutStartXMilli;
                gNdsStageMPWallHitScoutDeltaYMilli =
                    gNdsStageMPWallHitScoutFinalYMilli -
                    gNdsStageMPWallHitScoutStartYMilli;
                gNdsStageMPWallHitScoutFinalFloorOK =
                    (((coll.mask_stat & MAP_FLAG_FLOOR) != 0u) &&
                     (ndsMPLineIDIsFloor(coll.floor_line_id) != FALSE)) ?
                        1u : 0u;
                sNdsStageMPWallHitScoutWidth = width;
                sNdsStageMPWallHitScoutCenter = center;
                ndsStageMPWallHitScoutRestoreCounters(&snapshot);
                return TRUE;
            }
            ndsStageMPWallHitScoutRestoreCounters(&snapshot);
        }
    }
    return FALSE;
}

static sb32 ndsStageMPWallHitScoutTryFloor(s32 floor_line_id)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    u32 side;
    u32 yakumono_count;
    u32 i;

    if ((floor_line_id < 0) ||
        (ndsMPLineIDIsFloor(floor_line_id) == FALSE) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    gNdsStageMPWallHitScoutFloorTestCount++;
    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }

    for (side = 0u; side < 2u; side++)
    {
        u32 line_kind = (side == 0u) ? (u32)nMPLineKindLWall :
            (u32)nMPLineKindRWall;
        s32 edge_under_line_id =
            ndsMPFindAdjacentWallForFloorEdge(floor_line_id,
                (side == 0u) ? FALSE : TRUE);

        for (i = 0u; i < yakumono_count; i++)
        {
            MPLineInfo *info = ndsMPLineInfoAt(line_info, i);
            s32 first = (s32)ndsMPLineInfoGroupID(info, line_kind);
            s32 count = (s32)ndsMPLineInfoLineCount(info, line_kind);
            s32 end;
            s32 line_id;

            if (count <= 0)
            {
                continue;
            }
            if (count > 4096)
            {
                count = 4096;
            }
            end = first + count;
            for (line_id = first; line_id < end; line_id++)
            {
                if (ndsStageMPWallHitScoutTryWall(floor_line_id, side,
                        line_id, edge_under_line_id) != FALSE)
                {
                    return TRUE;
                }
            }
        }
    }
    gNdsStageMPWallHitScoutMissCount++;
    return FALSE;
}

static void ndsStageMPWallHitScoutRunAllFloors(void)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;

    if (gNdsStageMPWallHitScoutRunCount != 0u)
    {
        return;
    }
    gNdsStageMPWallHitScoutRunCount++;
    if (ndsStageCollisionLoopGeometryReady() == FALSE)
    {
        gNdsStageMPWallHitScoutUnsafeCount++;
        return;
    }
    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }

    for (i = 0u; i < yakumono_count; i++)
    {
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);
        s32 first = (s32)ndsMPLineInfoGroupID(info, nMPLineKindFloor);
        s32 count = (s32)ndsMPLineInfoLineCount(info, nMPLineKindFloor);
        s32 end;
        s32 line_id;

        if (count <= 0)
        {
            continue;
        }
        if (count > 4096)
        {
            count = 4096;
        }
        end = first + count;
        for (line_id = first; line_id < end; line_id++)
        {
            if (ndsStageMPWallHitScoutTryFloor(line_id) != FALSE)
            {
                return;
            }
        }
    }
}

static void ndsStageMPWallHitScoutRunHyruleMap(void)
{
    NDSStageMPWallHitScoutResultSnapshot pupupu_result;
    MPGroundData *saved_ground_data = gMPCollisionGroundData;
    MPGeometryData *saved_geometry = gMPCollisionGeometry;
    MPMapObjContainer *saved_mapobjs = gMPCollisionMapObjs;
    f32 saved_light_angle_x = gMPCollisionLightAngleX;
    f32 saved_light_angle_y = gMPCollisionLightAngleY;
    s32 saved_bgm_default = gMPCollisionBGMDefault;
    s32 saved_bgm_current = gMPCollisionBGMCurrent;
    void *file;
    MPGroundData *ground_data;
    size_t size;

    if (gNdsStageMPWallHyruleScoutRunCount != 0u)
    {
        return;
    }

    size = lbRelocGetFileSize(&llGRHyruleMapFileID);
    file = lbRelocGetExternHeapFile(
        &llGRHyruleMapFileID,
        syTaskmanMalloc(size, 0x10));
    ground_data = lbRelocGetFileData(MPGroundData*,
                                     file,
                                     &llGRHyruleMapMapHeader);

    if (ground_data != NULL)
    {
        gNdsStageMPWallHyruleScoutRelocResult =
            NDS_STAGE_HYRULE_SCOUT_RELOC_PASS;
        gNdsStageMPWallHyruleScoutGroundDataReady = 1u;
    }
    if ((ground_data != NULL) && (ground_data->map_geometry != NULL))
    {
        gNdsStageMPWallHyruleScoutGeometryReady = 1u;
    }
    if ((ground_data != NULL) &&
        ((ground_data->map_nodes != NULL) ||
         ((ground_data->map_geometry != NULL) &&
          (ground_data->map_geometry->mapobjs != NULL))))
    {
        gNdsStageMPWallHyruleScoutMapNodesReady = 1u;
    }
    if ((ground_data == NULL) || (ground_data->map_geometry == NULL))
    {
        gNdsStageMPWallHyruleScoutUnsafeCount++;
        return;
    }

    ndsStageMPWallHitScoutSaveRuntimeGlobals(&pupupu_result);
    ndsStageMPWallHitScoutClearRuntimeGlobals();

    gMPCollisionGroundData = ground_data;
    gMPCollisionGeometry = ground_data->map_geometry;
    gMPCollisionMapObjs = (gMPCollisionGeometry != NULL) ?
        gMPCollisionGeometry->mapobjs : NULL;
    gMPCollisionLightAngleX = ground_data->light_angle.x;
    gMPCollisionLightAngleY = ground_data->light_angle.y;
    gMPCollisionBGMDefault = ground_data->bgm_id;
    gMPCollisionBGMCurrent = ground_data->bgm_id;

    ndsStageMPWallHitScoutRunAllFloors();
    ndsStageMPWallHitScoutCopyRuntimeGlobalsToHyrule();
    ndsStageMPWallHitScoutRestoreRuntimeGlobals(&pupupu_result);

    gMPCollisionGroundData = saved_ground_data;
    gMPCollisionGeometry = saved_geometry;
    gMPCollisionMapObjs = saved_mapobjs;
    gMPCollisionLightAngleX = saved_light_angle_x;
    gMPCollisionLightAngleY = saved_light_angle_y;
    gMPCollisionBGMDefault = saved_bgm_default;
    gMPCollisionBGMCurrent = saved_bgm_current;
}

static sb32 ndsStageMPWallFloorLoopTryWall(s32 floor_line_id, u32 side,
                                           s32 wall_line_id,
                                           s32 edge_under_line_id)
{
    static const f32 widths[] = {
        32.0F, 48.0F, 64.0F, 96.0F, 128.0F, 192.0F, 256.0F, 384.0F
    };
    Vec3f wall_a;
    Vec3f wall_b;
    u32 line_kind;
    u32 sample;
    u32 width_index;

    if ((floor_line_id < 0) || (wall_line_id < 0) ||
        (ndsMPLineIDIsFloor(floor_line_id) == FALSE) ||
        (ndsMPFindLineEndpoints(wall_line_id, &wall_a, &wall_b, NULL,
            NULL) == FALSE))
    {
        return FALSE;
    }

    line_kind = (side == 0u) ? (u32)nMPLineKindLWall :
        (u32)nMPLineKindRWall;
    if ((u32)ndsMPGetLineKindForLineID(wall_line_id) != line_kind)
    {
        return FALSE;
    }

    gNdsStageMPWallFloorLoopProbeCount++;
    gNdsStageMPWallFloorLoopFloorLineID = floor_line_id;
    gNdsStageMPWallFloorLoopWallLineID = wall_line_id;
    gNdsStageMPWallFloorLoopWallLineKind = line_kind;
    gNdsStageMPWallFloorLoopEdgeUnderLineID = edge_under_line_id;
    gNdsStageMPWallFloorLoopSide = side;

    if ((edge_under_line_id >= 0) && (wall_line_id == edge_under_line_id))
    {
        return FALSE;
    }

    for (sample = 1u; sample <= 5u; sample++)
    {
        f32 ratio = (f32)sample / 6.0F;
        f32 wall_x = wall_a.x + ((wall_b.x - wall_a.x) * ratio);
        f32 wall_y = wall_a.y + ((wall_b.y - wall_a.y) * ratio);

        for (width_index = 0u; width_index < ARRAY_COUNT(widths);
             width_index++)
        {
            f32 width = widths[width_index];
            f32 start_x = (side == 0u) ? (wall_x - (width * 0.95F)) :
                (wall_x + (width * 0.95F));
            f32 floor_y = 0.0F;
            f32 center;
            Vec3f b;
            Vec3f a;
            s32 hit_line_id = -1;
            Vec3f translate;
            Vec3f pos_prev;
            Vec3f floor_probe;
            MPCollData coll;
            u32 flags = 0u;
            Vec3f angle;
            u32 check_before;
            u32 adjust_before;
            u32 check_after;
            u32 adjust_after;

            if (ndsStageFloorEdgeLoopFloorYAtX(floor_line_id, start_x,
                    &floor_y) == FALSE)
            {
                continue;
            }
            center = wall_y - floor_y;
            if ((center < 8.0F) || (center > 1024.0F))
            {
                continue;
            }

            b.x = start_x;
            b.y = floor_y;
            b.z = 0.0F;
            a.x = (side == 0u) ? (start_x + width) :
                (start_x - width);
            a.y = floor_y + center;
            a.z = 0.0F;
            if ((ndsStageMPAdjustFloorLoopWallSweep(&b, &a, NULL,
                    &hit_line_id, NULL, NULL, line_kind, FALSE) == FALSE) ||
                (hit_line_id < 0) ||
                ((edge_under_line_id >= 0) &&
                 (hit_line_id == edge_under_line_id)))
            {
                continue;
            }

            floor_probe.x = start_x;
            floor_probe.y = floor_y;
            floor_probe.z = 0.0F;
            if (mpCollisionGetFCCommonFloor(floor_line_id, &floor_probe,
                    NULL, &flags, &angle) == FALSE)
            {
                continue;
            }

            translate.x = start_x;
            translate.y = floor_y;
            translate.z = 0.0F;
            pos_prev = translate;
            ndsStageMPUpdateFloorLoopInitColl(&coll, &translate, &pos_prev,
                                              floor_line_id);
            coll.map_coll.center = center;
            coll.map_coll.width = width;
            coll.floor_flags = flags;
            coll.floor_angle = angle;
            coll.update_tic = gMPCollisionUpdateTic;

            check_before = (side == 0u) ?
                gNdsStageMPAdjustFloorLoopCheckLHitCount :
                gNdsStageMPAdjustFloorLoopCheckRHitCount;
            adjust_before = (side == 0u) ?
                gNdsStageMPAdjustFloorLoopAdjustLCallCount :
                gNdsStageMPAdjustFloorLoopAdjustRCallCount;
            mpProcessRunFloorEdgeAdjust(&coll);
            check_after = (side == 0u) ?
                gNdsStageMPAdjustFloorLoopCheckLHitCount :
                gNdsStageMPAdjustFloorLoopCheckRHitCount;
            adjust_after = (side == 0u) ?
                gNdsStageMPAdjustFloorLoopAdjustLCallCount :
                gNdsStageMPAdjustFloorLoopAdjustRCallCount;

            if ((check_after <= check_before) ||
                (adjust_after <= adjust_before))
            {
                continue;
            }

            gNdsStageMPWallFloorLoopProbeHitCount++;
            gNdsStageMPWallFloorLoopFloorLineID = floor_line_id;
            gNdsStageMPWallFloorLoopWallLineID = coll.ewall_line_id;
            gNdsStageMPWallFloorLoopWallLineKind =
                (u32)ndsMPGetLineKindForLineID(coll.ewall_line_id);
            gNdsStageMPWallFloorLoopEdgeUnderLineID =
                edge_under_line_id;
            gNdsStageMPWallFloorLoopSide = side;
            gNdsStageMPWallFloorLoopCheckHitCount++;
            gNdsStageMPWallFloorLoopAdjustCallCount++;
            gNdsStageMPWallFloorLoopStartXMilli =
                ndsFloatToMilliSigned(start_x);
            gNdsStageMPWallFloorLoopStartYMilli =
                ndsFloatToMilliSigned(floor_y);
            gNdsStageMPWallFloorLoopFinalXMilli =
                ndsFloatToMilliSigned(translate.x);
            gNdsStageMPWallFloorLoopFinalYMilli =
                ndsFloatToMilliSigned(translate.y);
            gNdsStageMPWallFloorLoopDeltaXMilli =
                gNdsStageMPWallFloorLoopFinalXMilli -
                gNdsStageMPWallFloorLoopStartXMilli;
            gNdsStageMPWallFloorLoopDeltaYMilli =
                gNdsStageMPWallFloorLoopFinalYMilli -
                gNdsStageMPWallFloorLoopStartYMilli;
            gNdsStageMPWallFloorLoopFinalFloorOK =
                (((coll.mask_stat & MAP_FLAG_FLOOR) != 0u) &&
                 (ndsMPLineIDIsFloor(coll.floor_line_id) != FALSE)) ? 1u :
                    0u;
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsStageMPWallFloorLoopTryFloor(s32 floor_line_id)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    u32 side;
    u32 yakumono_count;
    u32 i;

    if ((floor_line_id < 0) ||
        (ndsMPLineIDIsFloor(floor_line_id) == FALSE) ||
        (ndsStageCollisionLoopGeometryReady() == FALSE))
    {
        return FALSE;
    }
    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }

    for (side = 0u; side < 2u; side++)
    {
        u32 line_kind = (side == 0u) ? (u32)nMPLineKindLWall :
            (u32)nMPLineKindRWall;
        s32 edge_under_line_id =
            ndsMPFindAdjacentWallForFloorEdge(floor_line_id,
                (side == 0u) ? FALSE : TRUE);

        for (i = 0u; i < yakumono_count; i++)
        {
            MPLineInfo *info = ndsMPLineInfoAt(line_info, i);
            s32 first = (s32)ndsMPLineInfoGroupID(info, line_kind);
            s32 count = (s32)ndsMPLineInfoLineCount(info, line_kind);
            s32 end;
            s32 line_id;

            if (count <= 0)
            {
                continue;
            }
            if (count > 4096)
            {
                count = 4096;
            }
            end = first + count;
            for (line_id = first; line_id < end; line_id++)
            {
                if (ndsStageMPWallFloorLoopTryWall(floor_line_id, side,
                        line_id, edge_under_line_id) != FALSE)
                {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

static void ndsStageMPWallFloorLoopRunStandaloneProbe(void)
{
    s32 selected_line_id = gNdsStageFloorEdgeLoopSelectedLineID;

    if ((ndsStageCollisionLoopGeometryReady() == FALSE) ||
        (gNdsStageMPWallFloorLoopProbeHitCount > 0u) ||
        (gNdsStageMPWallFloorLoopProbeMissCount > 0u))
    {
        return;
    }
    if (ndsStageMPWallFloorLoopTryFloor(selected_line_id) != FALSE)
    {
        return;
    }
    gNdsStageMPWallFloorLoopProbeMissCount++;
}

void ndsFighterMarioFoxStageMPWallFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPWallFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPWallFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPWallFloorLoopReset();
    gNdsStageMPWallFloorLoopPrepared = 1u;
    gNdsStageMPWallFloorLoopBaseMPEdgeSeen =
        (gNdsStageMPEdgeFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPWallFloorLoopBaseMPEdgeSeen == 0u)
    {
        gNdsStageMPWallFloorLoopUnsafeCount++;
    }
}

void ndsFighterMarioFoxStageMPWallFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPWallFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPWallFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPWallFloorLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPEdgeFloorLoopResult ==
        NDS_FIGHTER_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP_PASS)
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPWallFloorLoopPrepared != 0u) &&
        (gNdsStageMPWallFloorLoopBaseMPEdgeSeen != 0u))
    {
        mask |= 1u << 1;
    }

    ndsStageMPWallFloorLoopRunStandaloneProbe();
    ndsStageMPWallHitScoutRunAllFloors();
    ndsStageMPWallHitScoutRunHyruleMap();

    if ((gNdsStageMPWallFloorLoopFloorLineID >= 0) &&
        (gNdsStageMPWallFloorLoopWallLineID >= 0) &&
        (gNdsStageMPWallFloorLoopEdgeUnderLineID >= 0))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPWallFloorLoopWallLineID ==
            gNdsStageMPWallFloorLoopEdgeUnderLineID) &&
        (((gNdsStageMPWallFloorLoopSide == 0u) &&
          (gNdsStageMPWallFloorLoopWallLineKind ==
            (u32)nMPLineKindLWall)) ||
         ((gNdsStageMPWallFloorLoopSide == 1u) &&
          (gNdsStageMPWallFloorLoopWallLineKind ==
            (u32)nMPLineKindRWall))))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPWallFloorLoopProbeCount > 0u) &&
        (gNdsStageMPWallFloorLoopProbeHitCount == 0u) &&
        (gNdsStageMPWallFloorLoopProbeMissCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPAdjustFloorLoopCheckLCallCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopCheckRCallCount > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPAdjustFloorLoopCheckLHitCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopCheckRHitCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopCheckLMissCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopCheckRMissCount > 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPAdjustFloorLoopWallLHitCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopWallRHitCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopWallLMissCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopWallRMissCount > 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPAdjustFloorLoopEdgeUnderLCallCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopEdgeUnderRCallCount > 0u) &&
        (gNdsStageMPAdjustFloorLoopEdgeUnderDeferredCount == 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPEdgeFloorLoopEdgeUnderLLineID >= 0) &&
        (gNdsStageMPEdgeFloorLoopEdgeUnderRLineID >= 0) &&
        (gNdsStageMPEdgeFloorLoopEdgeUnderLLineID !=
            gNdsStageMPEdgeFloorLoopEdgeUnderRLineID))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPEdgeFloorLoopEdgeUnderLLineKind ==
            (u32)nMPLineKindLWall) &&
        (gNdsStageMPEdgeFloorLoopEdgeUnderRLineKind ==
            (u32)nMPLineKindRWall))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPEdgeFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopUnsafeCount == 0u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 12;
    }
    gNdsFighterMarioFoxStageMPWallFloorLoopCount =
        gNdsFighterMarioFoxStageMPEdgeFloorLoopCount;
    if (gNdsFighterMarioFoxStageMPWallFloorLoopCount ==
        gNdsFighterMarioFoxStageMPEdgeFloorLoopCount)
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageMPWallFloorLoopStartXMilli == 0) &&
        (gNdsStageMPWallFloorLoopStartYMilli == 0) &&
        (gNdsStageMPWallFloorLoopFinalXMilli == 0) &&
        (gNdsStageMPWallFloorLoopFinalYMilli == 0) &&
        (gNdsStageMPWallFloorLoopCheckHitCount == 0u) &&
        (gNdsStageMPWallFloorLoopAdjustCallCount == 0u) &&
        (gNdsStageMPWallFloorLoopFinalFloorOK == 0u))
    {
        mask |= 1u << 14;
    }
    if (gNdsStageMPWallFloorLoopUnsafeCount == 0u)
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPWallFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPWallFloorLoopDeferredMask = 0xffu;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageMPWallFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPWallFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP_SAFE_PASS;
    }
    ndsFighterMarioFoxStageMPWallHitFloorLoopFinalize();
}

static void ndsStageMPWallHitFloorLoopCaptureHyruleScout(void)
{
    gNdsStageMPWallHitFloorLoopRelocResult =
        gNdsStageMPWallHyruleScoutRelocResult;
    gNdsStageMPWallHitFloorLoopGroundDataReady =
        gNdsStageMPWallHyruleScoutGroundDataReady;
    gNdsStageMPWallHitFloorLoopGeometryReady =
        gNdsStageMPWallHyruleScoutGeometryReady;
    gNdsStageMPWallHitFloorLoopMapNodesReady =
        gNdsStageMPWallHyruleScoutMapNodesReady;
    gNdsStageMPWallHitFloorLoopRunCount =
        gNdsStageMPWallHyruleScoutRunCount;
    gNdsStageMPWallHitFloorLoopFloorTestCount =
        gNdsStageMPWallHyruleScoutFloorTestCount;
    gNdsStageMPWallHitFloorLoopWallTestCount =
        gNdsStageMPWallHyruleScoutWallTestCount;
    gNdsStageMPWallHitFloorLoopCandidateCount =
        gNdsStageMPWallHyruleScoutCandidateCount;
    gNdsStageMPWallHitFloorLoopHitCount =
        gNdsStageMPWallHyruleScoutHitCount;
    gNdsStageMPWallHitFloorLoopMissCount =
        gNdsStageMPWallHyruleScoutMissCount;
    gNdsStageMPWallHitFloorLoopFloorLineID =
        gNdsStageMPWallHyruleScoutFloorLineID;
    gNdsStageMPWallHitFloorLoopWallLineID =
        gNdsStageMPWallHyruleScoutWallLineID;
    gNdsStageMPWallHitFloorLoopEdgeUnderLineID =
        gNdsStageMPWallHyruleScoutEdgeUnderLineID;
    gNdsStageMPWallHitFloorLoopSide =
        gNdsStageMPWallHyruleScoutSide;
    gNdsStageMPWallHitFloorLoopWallLineKind =
        gNdsStageMPWallHyruleScoutWallLineKind;
    gNdsStageMPWallHitFloorLoopStartXMilli =
        gNdsStageMPWallHyruleScoutStartXMilli;
    gNdsStageMPWallHitFloorLoopStartYMilli =
        gNdsStageMPWallHyruleScoutStartYMilli;
    gNdsStageMPWallHitFloorLoopFinalXMilli =
        gNdsStageMPWallHyruleScoutFinalXMilli;
    gNdsStageMPWallHitFloorLoopFinalYMilli =
        gNdsStageMPWallHyruleScoutFinalYMilli;
    gNdsStageMPWallHitFloorLoopDeltaXMilli =
        gNdsStageMPWallHyruleScoutDeltaXMilli;
    gNdsStageMPWallHitFloorLoopDeltaYMilli =
        gNdsStageMPWallHyruleScoutDeltaYMilli;
    gNdsStageMPWallHitFloorLoopFinalFloorOK =
        gNdsStageMPWallHyruleScoutFinalFloorOK;
    gNdsStageMPWallHitFloorLoopUnsafeCount =
        gNdsStageMPWallHyruleScoutUnsafeCount;
}

static void ndsFighterMarioFoxStageMPWallHitFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPWallHitFloorLoopProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxStageMPWallHitFloorLoopResult != 0u))
    {
        return;
    }

    gNdsStageMPWallHitFloorLoopPrepared =
        (gNdsStageMPWallFloorLoopPrepared != 0u) ? 1u : 0u;
    gNdsStageMPWallHitFloorLoopBaseMPWallSeen =
        ((gNdsFighterMarioFoxStageMPWallFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP_PASS) ||
         ((gNdsStageMPWallFloorLoopPrepared != 0u) &&
          (gNdsStageMPWallFloorLoopProbeHitCount == 0u) &&
          (gNdsStageMPWallFloorLoopProbeMissCount > 0u) &&
          (gNdsStageMPWallFloorLoopCheckHitCount == 0u) &&
          (gNdsStageMPWallFloorLoopAdjustCallCount == 0u) &&
          (gNdsStageMPWallHitScoutRunCount > 0u) &&
          (gNdsStageMPWallHitScoutHitCount == 0u))) ? 1u : 0u;
    ndsStageMPWallHitFloorLoopCaptureHyruleScout();

    if (gNdsStageMPWallHitFloorLoopBaseMPWallSeen != 0u)
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPWallHitFloorLoopPrepared != 0u) &&
        (gNdsStageMPWallFloorLoopBaseMPEdgeSeen != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPWallHitFloorLoopRelocResult ==
            NDS_STAGE_HYRULE_SCOUT_RELOC_PASS) &&
        (gNdsStageMPWallHitFloorLoopGroundDataReady != 0u) &&
        (gNdsStageMPWallHitFloorLoopGeometryReady != 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPWallHitFloorLoopRunCount == 1u) &&
        (gNdsStageMPWallHitFloorLoopFloorTestCount > 0u) &&
        (gNdsStageMPWallHitFloorLoopWallTestCount > 0u) &&
        (gNdsStageMPWallHitFloorLoopCandidateCount > 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPWallHitFloorLoopHitCount > 0u) &&
        (gNdsStageMPWallHitFloorLoopUnsafeCount == 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPWallHitFloorLoopFloorLineID == 5) &&
        (gNdsStageMPWallHitFloorLoopWallLineID == 13) &&
        (gNdsStageMPWallHitFloorLoopEdgeUnderLineID == 12) &&
        (gNdsStageMPWallHitFloorLoopSide == 0u) &&
        (gNdsStageMPWallHitFloorLoopWallLineKind == (u32)nMPLineKindLWall))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPWallHitFloorLoopFinalFloorOK != 0u) &&
        (gNdsStageMPWallHitFloorLoopWallLineID !=
            gNdsStageMPWallHitFloorLoopEdgeUnderLineID))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPWallHitFloorLoopDeltaXMilli == -1600) &&
        (gNdsStageMPWallHitFloorLoopDeltaYMilli == -388))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPWallFloorLoopProbeHitCount == 0u) &&
        (gNdsStageMPWallFloorLoopProbeMissCount > 0u) &&
        (gNdsStageMPWallFloorLoopCheckHitCount == 0u) &&
        (gNdsStageMPWallFloorLoopAdjustCallCount == 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 9;
    }

    gNdsFighterMarioFoxStageMPWallHitFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPWallHitFloorLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageMPWallHitFloorLoopCount =
        gNdsStageMPWallHitFloorLoopHitCount;
    if ((mask & 0x3ffu) == 0x3ffu)
    {
        gNdsFighterMarioFoxStageMPWallHitFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPWallHitFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP_SAFE_PASS;
    }
}

void ndsFighterMarioFoxStageMPWallCopyFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPWallCopyFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPWallCopyFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPWallCopyFloorLoopReset();
    gNdsStageMPWallCopyFloorLoopPrepared = 1u;
    gNdsStageMPWallCopyFloorLoopBaseMPWallHitSeen =
        (gNdsStageMPWallHitFloorLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPWallCopyFloorLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp;
    FTStruct *p1;
    DObj *root;
    DObj *p1_root;
    MPGroundData *saved_ground_data = gMPCollisionGroundData;
    MPGeometryData *saved_geometry = gMPCollisionGeometry;
    MPMapObjContainer *saved_mapobjs = gMPCollisionMapObjs;
    f32 saved_light_angle_x = gMPCollisionLightAngleX;
    f32 saved_light_angle_y = gMPCollisionLightAngleY;
    s32 saved_bgm_default = gMPCollisionBGMDefault;
    s32 saved_bgm_current = gMPCollisionBGMCurrent;
    void *file;
    MPGroundData *ground_data;
    size_t size;
    Vec3f translate;
    Vec3f floor_probe;
    MPCollData coll;
    u32 flags = 0u;
    Vec3f angle;
    u32 check_before;
    u32 adjust_before;
    u32 check_after;
    u32 adjust_after;
    s32 floor_line_id;

    if ((fighter_gobj == NULL) ||
        (gNdsStageMPWallCopyFloorLoopCallbackCount != 0u))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->player != 0))
    {
        gNdsStageMPWallCopyFloorLoopUnsafeCount++;
        return;
    }
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPWallCopyFloorLoopUnsafeCount++;
        return;
    }

    gNdsStageMPWallCopyFloorLoopCallbackCount++;
    gNdsStageMPWallCopyFloorLoopGObjProcessRunCount++;
    p1 = &sNdsFighterStructPool[1];
    p1_root = ((ndsFighterStructIsPoolPointer(p1) != FALSE) &&
        (p1->fighter_gobj != NULL)) ? DObjGetStruct(p1->fighter_gobj) :
        NULL;
    if (p1_root != NULL)
    {
        gNdsStageMPWallCopyFloorLoopP1RootXBeforeMilli =
            ndsFloatToMilliSigned(p1_root->translate.vec.f.x);
        gNdsStageMPWallCopyFloorLoopP1RootYBeforeMilli =
            ndsFloatToMilliSigned(p1_root->translate.vec.f.y);
    }

    size = lbRelocGetFileSize(&llGRHyruleMapFileID);
    file = lbRelocGetExternHeapFile(&llGRHyruleMapFileID,
                                    syTaskmanMalloc(size, 0x10));
    ground_data = lbRelocGetFileData(MPGroundData*, file,
                                     &llGRHyruleMapMapHeader);
    if ((ground_data == NULL) || (ground_data->map_geometry == NULL) ||
        (sNdsStageMPWallHitScoutWidth <= 0.0F) ||
        (sNdsStageMPWallHitScoutCenter <= 0.0F))
    {
        gNdsStageMPWallCopyFloorLoopUnsafeCount++;
        return;
    }

    gMPCollisionGroundData = ground_data;
    gMPCollisionGeometry = ground_data->map_geometry;
    gMPCollisionMapObjs = (gMPCollisionGeometry != NULL) ?
        gMPCollisionGeometry->mapobjs : NULL;
    gMPCollisionLightAngleX = ground_data->light_angle.x;
    gMPCollisionLightAngleY = ground_data->light_angle.y;
    gMPCollisionBGMDefault = ground_data->bgm_id;
    gMPCollisionBGMCurrent = ground_data->bgm_id;

    floor_line_id = (s32)gNdsStageMPWallHitFloorLoopFloorLineID;
    translate.x = (f32)gNdsStageMPWallHitFloorLoopStartXMilli / 1000.0F;
    translate.y = (f32)gNdsStageMPWallHitFloorLoopStartYMilli / 1000.0F;
    translate.z = 0.0F;
    floor_probe = translate;
    if (mpCollisionGetFCCommonFloor(floor_line_id, &floor_probe, NULL,
            &flags, &angle) == FALSE)
    {
        gNdsStageMPWallCopyFloorLoopUnsafeCount++;
        goto restore_collision_globals;
    }

    memset(&coll, 0, sizeof(coll));
    coll.p_translate = &translate;
    coll.p_lr = &fp->lr;
    coll.pos_prev = translate;
    coll.map_coll = fp->coll_data.map_coll;
    coll.map_coll.bottom = 0.0F;
    coll.map_coll.width = sNdsStageMPWallHitScoutWidth;
    coll.map_coll.center = sNdsStageMPWallHitScoutCenter;
    coll.p_map_coll = &coll.map_coll;
    coll.mask_stat = MAP_FLAG_FLOOR;
    coll.floor_line_id = floor_line_id;
    coll.floor_flags = flags;
    coll.floor_angle = angle;
    coll.ceil_line_id = -1;
    coll.lwall_line_id = -1;
    coll.rwall_line_id = -1;
    coll.ewall_line_id = -1;
    coll.cliff_id = -1;
    coll.ignore_line_id = -1;
    coll.update_tic = gMPCollisionUpdateTic;

    gNdsStageMPWallCopyFloorLoopSourceFloorLineID =
        (u32)floor_line_id;
    gNdsStageMPWallCopyFloorLoopSourceWallLineID =
        (u32)gNdsStageMPWallHitFloorLoopWallLineID;
    gNdsStageMPWallCopyFloorLoopSourceEdgeLineID =
        (u32)gNdsStageMPWallHitFloorLoopEdgeUnderLineID;
    gNdsStageMPWallCopyFloorLoopSourceSide =
        gNdsStageMPWallHitFloorLoopSide;
    gNdsStageMPWallCopyFloorLoopStartXMilli =
        ndsFloatToMilliSigned(translate.x);
    gNdsStageMPWallCopyFloorLoopStartYMilli =
        ndsFloatToMilliSigned(translate.y);

    check_before = (gNdsStageMPWallHitFloorLoopSide == 0u) ?
        gNdsStageMPAdjustFloorLoopCheckLHitCount :
        gNdsStageMPAdjustFloorLoopCheckRHitCount;
    adjust_before = (gNdsStageMPWallHitFloorLoopSide == 0u) ?
        gNdsStageMPAdjustFloorLoopAdjustLCallCount :
        gNdsStageMPAdjustFloorLoopAdjustRCallCount;
    mpProcessRunFloorEdgeAdjust(&coll);
    check_after = (gNdsStageMPWallHitFloorLoopSide == 0u) ?
        gNdsStageMPAdjustFloorLoopCheckLHitCount :
        gNdsStageMPAdjustFloorLoopCheckRHitCount;
    adjust_after = (gNdsStageMPWallHitFloorLoopSide == 0u) ?
        gNdsStageMPAdjustFloorLoopAdjustLCallCount :
        gNdsStageMPAdjustFloorLoopAdjustRCallCount;

    root->translate.vec.f = translate;
    coll.p_translate = &root->translate.vec.f;
    ndsStageMPProcessFloorLoopCopyBack(fp, &coll);
    fp->ga = nMPKineticsGround;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.floor_dist = root->translate.vec.f.y;
    fp->coll_data.mask_stat |= MAP_FLAG_FLOOR;
    fp->coll_data.is_coll_end = FALSE;
    gNdsStageMPWallCopyFloorLoopCopyBackCount++;

    gNdsStageMPWallCopyFloorLoopFinalXMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.x);
    gNdsStageMPWallCopyFloorLoopFinalYMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);
    gNdsStageMPWallCopyFloorLoopDeltaXMilli =
        gNdsStageMPWallCopyFloorLoopFinalXMilli -
        gNdsStageMPWallCopyFloorLoopStartXMilli;
    gNdsStageMPWallCopyFloorLoopDeltaYMilli =
        gNdsStageMPWallCopyFloorLoopFinalYMilli -
        gNdsStageMPWallCopyFloorLoopStartYMilli;
    gNdsStageMPWallCopyFloorLoopP0FinalFloorOK =
        (((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u) &&
         (ndsMPLineIDIsFloor(fp->coll_data.floor_line_id) != FALSE) &&
         (check_after > check_before) &&
         (adjust_after > adjust_before)) ? 1u : 0u;
    gNdsStageMPWallCopyFloorLoopP0FinalMaskStat =
        fp->coll_data.mask_stat;
    gNdsStageMPWallCopyFloorLoopP0FinalGA = (u32)fp->ga;

restore_collision_globals:
    if (p1_root != NULL)
    {
        gNdsStageMPWallCopyFloorLoopP1RootXAfterMilli =
            ndsFloatToMilliSigned(p1_root->translate.vec.f.x);
        gNdsStageMPWallCopyFloorLoopP1RootYAfterMilli =
            ndsFloatToMilliSigned(p1_root->translate.vec.f.y);
    }
    gMPCollisionGroundData = saved_ground_data;
    gMPCollisionGeometry = saved_geometry;
    gMPCollisionMapObjs = saved_mapobjs;
    gMPCollisionLightAngleX = saved_light_angle_x;
    gMPCollisionLightAngleY = saved_light_angle_y;
    gMPCollisionBGMDefault = saved_bgm_default;
    gMPCollisionBGMCurrent = saved_bgm_current;
}

static void ndsStageMPWallCopyFloorLoopRunProof(void)
{
    FTStruct *fp = &sNdsFighterStructPool[0];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPWallCopyFloorLoopUnsafeCount++;
        return;
    }
    fighter_gobj = fp->fighter_gobj;
    gcPauseGObjProcessAll(fighter_gobj);
    sNdsStageMPWallCopyFloorLoopProcess =
        gcAddGObjProcess(fighter_gobj,
                         ndsStageMPWallCopyFloorLoopGObjProc,
                         nGCProcessKindFunc,
                         2);
    if (sNdsStageMPWallCopyFloorLoopProcess == NULL)
    {
        gNdsStageMPWallCopyFloorLoopUnsafeCount++;
        return;
    }
    gNdsStageMPWallCopyFloorLoopProcessAttachCount++;
    gcRunGObjProcess(sNdsStageMPWallCopyFloorLoopProcess);
    gcPauseGObjProcess(sNdsStageMPWallCopyFloorLoopProcess);
}

void ndsFighterMarioFoxStageMPWallCopyFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPWallCopyFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPWallCopyFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPWallCopyFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPWallHitFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPWallHitFloorLoopFinalize();
    }
    if (gNdsFighterMarioFoxStageMPCliffLiveLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffLiveLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPWallHitFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPWallHitFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPWallCopyFloorLoopBaseMPWallHitSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPWallCopyFloorLoopPrepared != 0u) &&
        (gNdsStageMPWallCopyFloorLoopBaseMPWallHitSeen != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPWallCopyFloorLoopProcessAttachCount == 0u) &&
        (gNdsStageMPWallCopyFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPWallCopyFloorLoopRunProof();
    }
    if ((gNdsStageMPWallCopyFloorLoopProcessAttachCount == 1u) &&
        (gNdsStageMPWallCopyFloorLoopGObjProcessRunCount == 1u) &&
        (gNdsStageMPWallCopyFloorLoopCallbackCount == 1u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPWallCopyFloorLoopCopyBackCount == 1u) &&
        (gNdsStageMPWallCopyFloorLoopP0FinalFloorOK != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPWallCopyFloorLoopSourceFloorLineID == 5u) &&
        (gNdsStageMPWallCopyFloorLoopSourceWallLineID == 13u) &&
        (gNdsStageMPWallCopyFloorLoopSourceEdgeLineID == 12u) &&
        (gNdsStageMPWallCopyFloorLoopSourceSide == 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPWallCopyFloorLoopDeltaXMilli == -1600) &&
        (gNdsStageMPWallCopyFloorLoopDeltaYMilli == -388))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPWallCopyFloorLoopP0FinalGA ==
            (u32)nMPKineticsGround) &&
        ((gNdsStageMPWallCopyFloorLoopP0FinalMaskStat &
            MAP_FLAG_FLOOR) != 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPWallCopyFloorLoopP1RootXBeforeMilli ==
            gNdsStageMPWallCopyFloorLoopP1RootXAfterMilli) &&
        (gNdsStageMPWallCopyFloorLoopP1RootYBeforeMilli ==
            gNdsStageMPWallCopyFloorLoopP1RootYAfterMilli))
    {
        mask |= 1u << 7;
    }
    if (gNdsStageMPWallCopyFloorLoopUnsafeCount == 0u)
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterMarioFoxStageMPCliffLiveLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffLiveLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP_SAFE_PASS))
    {
        mask |= 1u << 9;
    }

    gNdsFighterMarioFoxStageMPWallCopyFloorLoopCount =
        gNdsStageMPWallCopyFloorLoopCopyBackCount;
    gNdsFighterMarioFoxStageMPWallCopyFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPWallCopyFloorLoopDeferredMask = 0xffu;
    if ((mask & 0x3ffu) == 0x3ffu)
    {
        gNdsFighterMarioFoxStageMPWallCopyFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPWallCopyFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPPassFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPPassFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPPassFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPPassFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPPassFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPPassFloorLoopCount = 0u;
    gNdsStageMPPassFloorLoopPrepared = 0u;
    gNdsStageMPPassFloorLoopBaseWallCopySeen = 0u;
    gNdsStageMPPassFloorLoopCandidateScanCount = 0u;
    gNdsStageMPPassFloorLoopCandidateCount = 0u;
    gNdsStageMPPassFloorLoopCandidateLineID = -1;
    gNdsStageMPPassFloorLoopCandidateFlags = 0u;
    gNdsStageMPPassFloorLoopCandidateHasPass = 0u;
    gNdsStageMPPassFloorLoopNoCandidateBlocker = 0u;
    gNdsStageMPPassFloorLoopSameLineRejectCount = 0u;
    gNdsStageMPPassFloorLoopDifferentLineAcceptCount = 0u;
    gNdsStageMPPassFloorLoopRouteCount = 0u;
    gNdsStageMPPassFloorLoopPassCallbackCount = 0u;
    gNdsStageMPPassFloorLoopPassCallbackAllowCount = 0u;
    gNdsStageMPPassFloorLoopPassCallbackDenyCount = 0u;
    gNdsStageMPPassFloorLoopProcessAttachCount = 0u;
    gNdsStageMPPassFloorLoopGObjProcessRunCount = 0u;
    gNdsStageMPPassFloorLoopCallbackCount = 0u;
    gNdsStageMPPassFloorLoopP1RootXBeforeMilli = 0;
    gNdsStageMPPassFloorLoopP1RootYBeforeMilli = 0;
    gNdsStageMPPassFloorLoopP1RootXAfterMilli = 0;
    gNdsStageMPPassFloorLoopP1RootYAfterMilli = 0;
    gNdsStageMPPassFloorLoopP1RootUnchanged = 0u;
    gNdsStageMPPassFloorLoopUnsafeCount = 0u;
    sNdsStageMPPassFloorLoopProcess = NULL;
}

void ndsFighterMarioFoxStageMPPassFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPPassFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPPassFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPPassFloorLoopReset();
    gNdsStageMPPassFloorLoopPrepared = 1u;
    gNdsStageMPPassFloorLoopBaseWallCopySeen =
        (gNdsStageMPWallCopyFloorLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPPassFloorLoopInitColl(MPCollData *coll,
                                            FTStruct *fp,
                                            Vec3f *translate,
                                            const Vec3f *pos_prev,
                                            s32 floor_line_id,
                                            s32 ignore_line_id)
{
    memset(coll, 0, sizeof(*coll));
    coll->p_translate = translate;
    coll->p_lr = (fp != NULL) ? &fp->lr : NULL;
    coll->pos_prev = *pos_prev;
    coll->pos_diff.x = translate->x - pos_prev->x;
    coll->pos_diff.y = translate->y - pos_prev->y;
    coll->pos_diff.z = translate->z - pos_prev->z;
    if (fp != NULL)
    {
        coll->map_coll = fp->coll_data.map_coll;
        coll->cliffcatch_coll = fp->coll_data.cliffcatch_coll;
    }
    coll->map_coll.bottom = 0.0F;
    coll->p_map_coll = &coll->map_coll;
    coll->mask_stat = MAP_FLAG_FLOOR;
    coll->update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    coll->ewall_line_id = -1;
    coll->floor_line_id = floor_line_id;
    coll->ceil_line_id = -1;
    coll->lwall_line_id = -1;
    coll->rwall_line_id = -1;
    coll->cliff_id = -1;
    coll->ignore_line_id = ignore_line_id;
}

static sb32 ndsStageMPPassFloorLoopCheckTestFloorCollisionAdjNew(
    MPCollData *coll_data,
    sb32 (*proc_map)(GObj*),
    GObj *gobj)
{
    MPObjectColl *map_coll;
    MPObjectColl *p_map_coll;
    Vec3f *translate;
    Vec3f sp4C;
    Vec3f sp40;
    sb32 hit;

    if ((coll_data == NULL) || (coll_data->p_translate == NULL))
    {
        gNdsStageMPPassFloorLoopUnsafeCount++;
        return FALSE;
    }

    map_coll = &coll_data->map_coll;
    p_map_coll = (coll_data->p_map_coll != NULL) ? coll_data->p_map_coll :
        map_coll;
    translate = coll_data->p_translate;
    coll_data->mask_stat &= (u16)~MAP_FLAG_FLOOR;

    sp4C.x = coll_data->pos_prev.x;
    sp4C.y = coll_data->pos_prev.y + p_map_coll->bottom;
    sp4C.z = coll_data->pos_prev.z;
    sp40.x = translate->x;
    sp40.y = translate->y + map_coll->bottom;
    sp40.z = translate->z;

    hit = (coll_data->update_tic != gMPCollisionUpdateTic) ?
        mpCollisionCheckFloorLineCollisionDiff(&sp4C, &sp40,
            &coll_data->line_coll_dist, &coll_data->floor_line_id,
            &coll_data->floor_flags, &coll_data->floor_angle) :
        mpCollisionCheckFloorLineCollisionSame(&sp4C, &sp40,
            &coll_data->line_coll_dist, &coll_data->floor_line_id,
            &coll_data->floor_flags, &coll_data->floor_angle);

    if ((hit != FALSE) &&
        (((coll_data->floor_flags & MAP_VERTEX_COLL_PASS) == 0u) ||
         (coll_data->floor_line_id != coll_data->ignore_line_id)) &&
        ((proc_map == NULL) || (proc_map(gobj) != FALSE)))
    {
        coll_data->mask_curr |= MAP_FLAG_FLOOR;
        return TRUE;
    }
    return FALSE;
}

static sb32 ndsStageMPPassFloorLoopPassCallback(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    gNdsStageMPPassFloorLoopPassCallbackCount++;
    if ((fp != NULL) && (ndsFighterStructIsPoolPointer(fp) != FALSE) &&
        (fp->player == 0))
    {
        gNdsStageMPPassFloorLoopPassCallbackAllowCount++;
        return TRUE;
    }
    gNdsStageMPPassFloorLoopPassCallbackDenyCount++;
    return FALSE;
}

static sb32 ndsStageMPPassFloorLoopSpecialCollisions(MPCollData *coll_data,
                                                     GObj *fighter_gobj,
                                                     u32 flags)
{
    sb32 is_floor;

    if ((flags & MAP_PROC_TYPE_PASS) == 0u)
    {
        gNdsStageMPPassFloorLoopUnsafeCount++;
        return FALSE;
    }
    gNdsStageMPPassFloorLoopRouteCount++;
    is_floor = ndsStageMPPassFloorLoopCheckTestFloorCollisionAdjNew(
        coll_data, ndsStageMPPassFloorLoopPassCallback, fighter_gobj);
    if (is_floor != FALSE)
    {
        mpProcessSetLandingFloor(coll_data);
        if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
        {
            mpProcessRunFloorEdgeAdjust(coll_data);
        }
        coll_data->mask_stat &= (u16)~MAP_FLAG_FLOOREDGE;
        coll_data->is_coll_end = FALSE;
    }
    else
    {
        coll_data->is_coll_end = TRUE;
    }
    return is_floor;
}

static sb32 ndsStageMPPassFloorLoopBuildProbe(FTStruct *fp,
                                              s32 line_id,
                                              s32 ignore_line_id,
                                              MPCollData *coll,
                                              Vec3f *translate,
                                              Vec3f *pos_prev)
{
    Vec3f left;
    Vec3f right;
    f32 floor_y;

    if ((fp == NULL) || (coll == NULL) || (translate == NULL) ||
        (pos_prev == NULL) ||
        (ndsMPFindLineEndpoints(line_id, &left, &right, NULL, NULL) ==
            FALSE) ||
        (fabsf(right.x - left.x) < 64.0F))
    {
        return FALSE;
    }
    translate->x = left.x + ((right.x - left.x) * 0.5F);
    if (ndsStageFloorEdgeLoopFloorYAtX(line_id, translate->x, &floor_y) ==
        FALSE)
    {
        return FALSE;
    }
    translate->y = floor_y - 16.0F;
    translate->z = 0.0F;
    pos_prev->x = translate->x;
    pos_prev->y = floor_y + 32.0F;
    pos_prev->z = 0.0F;
    ndsStageMPPassFloorLoopInitColl(coll, fp, translate, pos_prev, line_id,
                                    ignore_line_id);
    return TRUE;
}

static sb32 ndsStageMPPassFloorLoopChooseCandidate(FTStruct *fp,
                                                   s32 *line_id_out,
                                                   u32 *flags_out)
{
    s32 min_line = gNdsStageCollisionLoopFloorLineMin;
    s32 max_line = gNdsStageCollisionLoopFloorLineMaxExclusive;
    s32 line_id;

    if (line_id_out != NULL)
    {
        *line_id_out = -1;
    }
    if (flags_out != NULL)
    {
        *flags_out = 0u;
    }
    if ((fp == NULL) || (line_id_out == NULL) || (flags_out == NULL) ||
        (min_line < 0) || (max_line <= min_line))
    {
        return FALSE;
    }

    for (line_id = min_line; line_id < max_line; line_id++)
    {
        MPCollData coll;
        Vec3f translate;
        Vec3f pos_prev;
        u32 flags = 0u;

        if ((ndsMPLineIDIsFloor(line_id) == FALSE) ||
            (ndsMPFindLineEndpoints(line_id, NULL, NULL, &flags, NULL) ==
                FALSE))
        {
            continue;
        }
        gNdsStageMPPassFloorLoopCandidateScanCount++;
        if ((flags & MAP_VERTEX_COLL_PASS) == 0u)
        {
            continue;
        }
        if (ndsStageMPPassFloorLoopBuildProbe(fp, line_id, line_id, &coll,
                &translate, &pos_prev) == FALSE)
        {
            continue;
        }
        if (ndsStageMPPassFloorLoopCheckTestFloorCollisionAdjNew(&coll, NULL,
                NULL) != FALSE)
        {
            continue;
        }
        if ((coll.floor_line_id != line_id) ||
            ((coll.floor_flags & MAP_VERTEX_COLL_PASS) == 0u))
        {
            continue;
        }
        if (ndsStageMPPassFloorLoopBuildProbe(fp, line_id, -1, &coll,
                &translate, &pos_prev) == FALSE)
        {
            continue;
        }
        if (ndsStageMPPassFloorLoopCheckTestFloorCollisionAdjNew(&coll, NULL,
                NULL) == FALSE)
        {
            continue;
        }
        if ((coll.floor_line_id != line_id) ||
            ((coll.floor_flags & MAP_VERTEX_COLL_PASS) == 0u))
        {
            continue;
        }
        *line_id_out = line_id;
        *flags_out = flags;
        return TRUE;
    }
    return FALSE;
}

static void ndsStageMPPassFloorLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp;
    FTStruct *p1;
    DObj *p1_root;
    s32 line_id = -1;
    u32 flags = 0u;
    MPCollData coll;
    Vec3f translate;
    Vec3f pos_prev;
    sb32 hit;

    if ((fighter_gobj == NULL) ||
        (gNdsStageMPPassFloorLoopCallbackCount != 0u))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->player != 0))
    {
        gNdsStageMPPassFloorLoopUnsafeCount++;
        return;
    }

    gNdsStageMPPassFloorLoopCallbackCount++;
    gNdsStageMPPassFloorLoopGObjProcessRunCount++;
    p1 = &sNdsFighterStructPool[1];
    p1_root = ((ndsFighterStructIsPoolPointer(p1) != FALSE) &&
        (p1->fighter_gobj != NULL)) ? DObjGetStruct(p1->fighter_gobj) :
        NULL;
    if (p1_root != NULL)
    {
        gNdsStageMPPassFloorLoopP1RootXBeforeMilli =
            ndsFloatToMilliSigned(p1_root->translate.vec.f.x);
        gNdsStageMPPassFloorLoopP1RootYBeforeMilli =
            ndsFloatToMilliSigned(p1_root->translate.vec.f.y);
    }

    if (ndsStageMPPassFloorLoopChooseCandidate(fp, &line_id, &flags) ==
        FALSE)
    {
        gNdsStageMPPassFloorLoopNoCandidateBlocker = 1u;
        goto record_p1;
    }
    gNdsStageMPPassFloorLoopCandidateCount++;
    gNdsStageMPPassFloorLoopCandidateLineID = line_id;
    gNdsStageMPPassFloorLoopCandidateFlags =
        (u32)mpCollisionGetVertexFlagsLineID(line_id);
    gNdsStageMPPassFloorLoopCandidateHasPass =
        ((flags & MAP_VERTEX_COLL_PASS) != 0u) ? 1u : 0u;

    if (ndsStageMPPassFloorLoopBuildProbe(fp, line_id, line_id, &coll,
            &translate, &pos_prev) == FALSE)
    {
        gNdsStageMPPassFloorLoopUnsafeCount++;
        goto record_p1;
    }
    hit = mpProcessUpdateMain(&coll,
                              ndsStageMPPassFloorLoopSpecialCollisions,
                              fighter_gobj,
                              MAP_PROC_TYPE_PASS);
    if ((hit == FALSE) && (coll.floor_line_id == line_id) &&
        ((coll.floor_flags & MAP_VERTEX_COLL_PASS) != 0u) &&
        (gNdsStageMPPassFloorLoopPassCallbackCount == 0u))
    {
        gNdsStageMPPassFloorLoopSameLineRejectCount++;
    }
    else
    {
        gNdsStageMPPassFloorLoopUnsafeCount++;
    }

    if (ndsStageMPPassFloorLoopBuildProbe(fp, line_id, -1, &coll,
            &translate, &pos_prev) == FALSE)
    {
        gNdsStageMPPassFloorLoopUnsafeCount++;
        goto record_p1;
    }
    hit = mpProcessUpdateMain(&coll,
                              ndsStageMPPassFloorLoopSpecialCollisions,
                              fighter_gobj,
                              MAP_PROC_TYPE_PASS);
    if ((hit != FALSE) && (coll.floor_line_id == line_id) &&
        ((coll.mask_curr & MAP_FLAG_FLOOR) != 0u) &&
        ((coll.mask_stat & MAP_FLAG_FLOOR) != 0u) &&
        ((coll.floor_flags & MAP_VERTEX_COLL_PASS) != 0u))
    {
        gNdsStageMPPassFloorLoopDifferentLineAcceptCount++;
    }
    else
    {
        gNdsStageMPPassFloorLoopUnsafeCount++;
    }

record_p1:
    if (p1_root != NULL)
    {
        gNdsStageMPPassFloorLoopP1RootXAfterMilli =
            ndsFloatToMilliSigned(p1_root->translate.vec.f.x);
        gNdsStageMPPassFloorLoopP1RootYAfterMilli =
            ndsFloatToMilliSigned(p1_root->translate.vec.f.y);
        gNdsStageMPPassFloorLoopP1RootUnchanged =
            ((gNdsStageMPPassFloorLoopP1RootXBeforeMilli ==
              gNdsStageMPPassFloorLoopP1RootXAfterMilli) &&
             (gNdsStageMPPassFloorLoopP1RootYBeforeMilli ==
              gNdsStageMPPassFloorLoopP1RootYAfterMilli)) ? 1u : 0u;
    }
}

static void ndsStageMPPassFloorLoopRunProof(void)
{
    FTStruct *fp = &sNdsFighterStructPool[0];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPPassFloorLoopUnsafeCount++;
        return;
    }
    fighter_gobj = fp->fighter_gobj;
    gcPauseGObjProcessAll(fighter_gobj);
    sNdsStageMPPassFloorLoopProcess =
        gcAddGObjProcess(fighter_gobj,
                         ndsStageMPPassFloorLoopGObjProc,
                         nGCProcessKindFunc,
                         2);
    if (sNdsStageMPPassFloorLoopProcess == NULL)
    {
        gNdsStageMPPassFloorLoopUnsafeCount++;
        return;
    }
    gNdsStageMPPassFloorLoopProcessAttachCount++;
    gcRunGObjProcess(sNdsStageMPPassFloorLoopProcess);
    gcPauseGObjProcess(sNdsStageMPPassFloorLoopProcess);
}

void ndsFighterMarioFoxStageMPPassFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPPassFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPPassFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPPassFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPWallCopyFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPWallCopyFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPWallCopyFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPWallCopyFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPPassFloorLoopBaseWallCopySeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPPassFloorLoopPrepared != 0u) &&
        (gNdsStageMPPassFloorLoopBaseWallCopySeen != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPPassFloorLoopProcessAttachCount == 0u) &&
        (gNdsStageMPPassFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPPassFloorLoopRunProof();
    }
    if ((gNdsStageMPPassFloorLoopCandidateCount == 1u) &&
        (gNdsStageMPPassFloorLoopCandidateLineID >= 0) &&
        (gNdsStageMPPassFloorLoopCandidateHasPass != 0u) &&
        ((gNdsStageMPPassFloorLoopCandidateFlags &
            MAP_VERTEX_COLL_PASS) != 0u) &&
        (gNdsStageMPPassFloorLoopNoCandidateBlocker == 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPPassFloorLoopProcessAttachCount == 1u) &&
        (gNdsStageMPPassFloorLoopGObjProcessRunCount == 1u) &&
        (gNdsStageMPPassFloorLoopCallbackCount == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPPassFloorLoopRouteCount == 2u) &&
        (gNdsStageMPPassFloorLoopPassCallbackCount == 1u) &&
        (gNdsStageMPPassFloorLoopPassCallbackAllowCount == 1u) &&
        (gNdsStageMPPassFloorLoopPassCallbackDenyCount == 0u))
    {
        mask |= 1u << 4;
    }
    if (gNdsStageMPPassFloorLoopSameLineRejectCount == 1u)
    {
        mask |= 1u << 5;
    }
    if (gNdsStageMPPassFloorLoopDifferentLineAcceptCount == 1u)
    {
        mask |= 1u << 6;
    }
    if (gNdsStageMPPassFloorLoopCandidateScanCount > 0u)
    {
        mask |= 1u << 7;
    }
    if (gNdsStageMPPassFloorLoopP1RootUnchanged != 0u)
    {
        mask |= 1u << 8;
    }
    if (gNdsStageMPPassFloorLoopUnsafeCount == 0u)
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxStageMPPassFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPPassFloorLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageMPPassFloorLoopCount =
        gNdsStageMPPassFloorLoopDifferentLineAcceptCount;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxStageMPPassFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPPassFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPPlatformFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPPlatformFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPPlatformFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPPlatformFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPPlatformFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPPlatformFloorLoopCount = 0u;
    gNdsStageMPPlatformFloorLoopPrepared = 0u;
    gNdsStageMPPlatformFloorLoopBasePassSeen = 0u;
    gNdsStageMPPlatformFloorLoopProbeCount = 0u;
    gNdsStageMPPlatformFloorLoopLineID = -1;
    gNdsStageMPPlatformFloorLoopLineFlags = 0u;
    gNdsStageMPPlatformFloorLoopLineHasPass = 0u;
    gNdsStageMPPlatformFloorLoopYakumonoID = 0u;
    gNdsStageMPPlatformFloorLoopYakumonoCount = 0u;
    gNdsStageMPPlatformFloorLoopDObjPresent = 0u;
    gNdsStageMPPlatformFloorLoopDObjStatus = 0u;
    gNdsStageMPPlatformFloorLoopDObjAnimPresent = 0u;
    gNdsStageMPPlatformFloorLoopPredicateResult = 0u;
    gNdsStageMPPlatformFloorLoopBlockerMask = 0u;
    gNdsStageMPPlatformFloorLoopUnsafeCount = 0u;
    sNdsStageMPPlatformFloorLoopProcess = NULL;
    sNdsStageMPPlatformActiveFloorLoopGObj = NULL;
}

void ndsFighterMarioFoxStageMPPlatformFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPPlatformFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPPlatformFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPPlatformFloorLoopReset();
    gNdsStageMPPlatformFloorLoopPrepared = 1u;
    gNdsStageMPPlatformFloorLoopBasePassSeen =
        (gNdsStageMPPassFloorLoopPrepared != 0u) ? 1u : 0u;
}

static sb32 ndsStageMPPlatformActiveFloorLoopActivateYakumono(u32 yakumono_id)
{
    DObj *yakumono_dobj;

    if ((ndsFighterMarioFoxStageMPPlatformActiveFloorLoopProofEnabled() ==
            FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
    {
        return FALSE;
    }

    yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
    if (yakumono_dobj == NULL)
    {
        if (sNdsStageMPPlatformActiveFloorLoopGObj == NULL)
        {
            sNdsStageMPPlatformActiveFloorLoopGObj =
                gcMakeGObjSPAfter(nGCCommonKindGround,
                                  NULL,
                                  nGCCommonLinkIDGround,
                                  GOBJ_PRIORITY_DEFAULT);
        }
        if (sNdsStageMPPlatformActiveFloorLoopGObj == NULL)
        {
            return FALSE;
        }
        yakumono_dobj =
            gcAddDObjForGObj(sNdsStageMPPlatformActiveFloorLoopGObj, NULL);
        if (yakumono_dobj == NULL)
        {
            return FALSE;
        }
        gMPCollisionYakumonoDObjs->dobjs[yakumono_id] = yakumono_dobj;
    }

    yakumono_dobj->anim_joint.event32 = NULL;
    mpCollisionSetYakumonoOnID((s32)yakumono_id);
    return TRUE;
}

static void ndsStageMPPlatformFloorLoopProbeLine(s32 line_id)
{
    u32 yakumono_id = 0u;
    DObj *yakumono_dobj = NULL;
    u32 blocker = 0u;

    gNdsStageMPPlatformFloorLoopProbeCount++;
    gNdsStageMPPlatformFloorLoopLineID = line_id;
    gNdsStageMPPlatformFloorLoopYakumonoCount =
        ndsMPGeometryYakumonoCount(gMPCollisionGeometry);

    if (ndsStageCollisionLoopGeometryReady() == FALSE)
    {
        blocker |= 1u << 0;
    }
    if (line_id < 0)
    {
        blocker |= 1u << 1;
    }
    if ((blocker == 0u) &&
        (ndsMPFindLineYakumonoID(line_id, &yakumono_id) == FALSE))
    {
        blocker |= 1u << 2;
    }
    gNdsStageMPPlatformFloorLoopYakumonoID = yakumono_id;

    if (blocker == 0u)
    {
        ndsStageMPPlatformActiveFloorLoopActivateYakumono(yakumono_id);
    }

    if (blocker == 0u)
    {
        gNdsStageMPPlatformFloorLoopLineFlags =
            (u32)mpCollisionGetVertexFlagsLineID(line_id);
        gNdsStageMPPlatformFloorLoopLineHasPass =
            ((gNdsStageMPPlatformFloorLoopLineFlags &
              MAP_VERTEX_COLL_PASS) != 0u) ? 1u : 0u;
    }

    if (gMPCollisionYakumonoDObjs == NULL)
    {
        blocker |= 1u << 3;
    }
    else if (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS)
    {
        blocker |= 1u << 4;
    }
    else
    {
        yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
    }

    if (yakumono_dobj == NULL)
    {
        blocker |= 1u << 5;
    }
    else
    {
        gNdsStageMPPlatformFloorLoopDObjPresent = 1u;
        gNdsStageMPPlatformFloorLoopDObjStatus =
            (u32)yakumono_dobj->user_data.s;
        gNdsStageMPPlatformFloorLoopDObjAnimPresent =
            (yakumono_dobj->anim_joint.event32 != NULL) ? 1u : 0u;
    }

    gNdsStageMPPlatformFloorLoopPredicateResult =
        (mpCollisionCheckExistPlatformLineID(line_id) != FALSE) ? 1u : 0u;

    if ((gNdsStageMPPlatformFloorLoopPredicateResult == 0u) &&
        ((blocker & ((1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) |
                     (1u << 4) | (1u << 5))) == 0u))
    {
        blocker |= 1u << 6;
    }
    gNdsStageMPPlatformFloorLoopBlockerMask = blocker;
}

static void ndsStageMPPlatformFloorLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp;
    s32 line_id;

    if ((fighter_gobj == NULL) ||
        (gNdsStageMPPlatformFloorLoopProbeCount != 0u))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->player != 0))
    {
        gNdsStageMPPlatformFloorLoopUnsafeCount++;
        return;
    }

    line_id = gNdsStageMPPassFloorLoopCandidateLineID;
    if (line_id < 0)
    {
        u32 flags = 0u;

        if (ndsStageMPPassFloorLoopChooseCandidate(fp, &line_id, &flags) ==
            FALSE)
        {
            gNdsStageMPPlatformFloorLoopBlockerMask |= 1u << 1;
            return;
        }
    }
    ndsStageMPPlatformFloorLoopProbeLine(line_id);
}

static void ndsStageMPPlatformFloorLoopRunProof(void)
{
    FTStruct *fp = &sNdsFighterStructPool[0];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPPlatformFloorLoopUnsafeCount++;
        return;
    }
    fighter_gobj = fp->fighter_gobj;
    gcPauseGObjProcessAll(fighter_gobj);
    sNdsStageMPPlatformFloorLoopProcess =
        gcAddGObjProcess(fighter_gobj,
                         ndsStageMPPlatformFloorLoopGObjProc,
                         nGCProcessKindFunc,
                         2);
    if (sNdsStageMPPlatformFloorLoopProcess == NULL)
    {
        gNdsStageMPPlatformFloorLoopUnsafeCount++;
        return;
    }
    gcRunGObjProcess(sNdsStageMPPlatformFloorLoopProcess);
    gcPauseGObjProcess(sNdsStageMPPlatformFloorLoopProcess);
}

void ndsFighterMarioFoxStageMPPlatformFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPPlatformFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPPlatformFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPPlatformFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPPassFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPPassFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPPassFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPPassFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPPlatformFloorLoopBasePassSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPPlatformFloorLoopPrepared != 0u) &&
        (gNdsStageMPPlatformFloorLoopBasePassSeen != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPPlatformFloorLoopProbeCount == 0u) &&
        (gNdsStageMPPlatformFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPPlatformFloorLoopRunProof();
    }
    if ((gNdsStageMPPlatformFloorLoopProbeCount == 1u) &&
        (gNdsStageMPPlatformFloorLoopLineID >= 0) &&
        (gNdsStageMPPlatformFloorLoopLineHasPass != 0u) &&
        ((gNdsStageMPPlatformFloorLoopLineFlags &
          MAP_VERTEX_COLL_PASS) != 0u))
    {
        mask |= 1u << 2;
    }
    if (gNdsStageMPPlatformFloorLoopYakumonoCount > 0u)
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPPlatformFloorLoopPredicateResult != 0u) ||
        (gNdsStageMPPlatformFloorLoopBlockerMask != 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPPlatformFloorLoopDObjPresent != 0u) ||
        ((gNdsStageMPPlatformFloorLoopBlockerMask &
          ((1u << 3) | (1u << 4) | (1u << 5))) != 0u))
    {
        mask |= 1u << 5;
    }
    if (gNdsStageMPPlatformFloorLoopUnsafeCount == 0u)
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 7;
    }

    gNdsFighterMarioFoxStageMPPlatformFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPPlatformFloorLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageMPPlatformFloorLoopCount =
        gNdsStageMPPlatformFloorLoopPredicateResult;
    if ((mask & 0xffu) == 0xffu)
    {
        gNdsFighterMarioFoxStageMPPlatformFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPPlatformFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPPlatformTickFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPPlatformTickFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPPlatformTickFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPPlatformTickFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPPlatformTickFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPPlatformTickFloorLoopCount = 0u;
    gNdsStageMPPlatformTickFloorLoopPrepared = 0u;
    gNdsStageMPPlatformTickFloorLoopBaseActiveSeen = 0u;
    gNdsStageMPPlatformTickFloorLoopSetOnCount = 0u;
    gNdsStageMPPlatformTickFloorLoopAdvanceCount = 0u;
    gNdsStageMPPlatformTickFloorLoopUnsafeCount = 0u;
    gNdsStageMPPlatformTickFloorLoopUpdateTicBefore = 0u;
    gNdsStageMPPlatformTickFloorLoopUpdateTicAfter = 0u;
    gNdsStageMPPlatformTickFloorLoopDObjPresent = 0u;
    gNdsStageMPPlatformTickFloorLoopStatusBefore = 0u;
    gNdsStageMPPlatformTickFloorLoopStatusAfter = 0u;
    gNdsStageMPPlatformTickFloorLoopPredicateAfter = 0u;
    sNdsStageMPPlatformTickFloorLoopAdvanceActive = FALSE;
}

void ndsFighterMarioFoxStageMPPlatformTickFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPPlatformTickFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPPlatformTickFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPPlatformTickFloorLoopReset();
    gNdsStageMPPlatformTickFloorLoopPrepared = 1u;
}

static void ndsStageMPPlatformTickFloorLoopRunProof(void)
{
    DObj *yakumono_dobj;
    s32 line_id = gNdsStageMPPlatformFloorLoopLineID;
    u32 yakumono_id = gNdsStageMPPlatformFloorLoopYakumonoID;

    if ((line_id < 0) || (gMPCollisionYakumonoDObjs == NULL) ||
        (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
    {
        gNdsStageMPPlatformTickFloorLoopUnsafeCount++;
        return;
    }
    yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
    if (yakumono_dobj == NULL)
    {
        gNdsStageMPPlatformTickFloorLoopUnsafeCount++;
        return;
    }
    gNdsStageMPPlatformTickFloorLoopDObjPresent = 1u;
    gNdsStageMPPlatformTickFloorLoopStatusBefore =
        (u32)yakumono_dobj->user_data.s;
    gNdsStageMPPlatformTickFloorLoopUpdateTicBefore =
        (u32)gMPCollisionUpdateTic;

    sNdsStageMPPlatformTickFloorLoopAdvanceActive = TRUE;
    mpCollisionAdvanceUpdateTic(gGRCommonLayerGObjs[1]);
    sNdsStageMPPlatformTickFloorLoopAdvanceActive = FALSE;

    gNdsStageMPPlatformTickFloorLoopUpdateTicAfter =
        (u32)gMPCollisionUpdateTic;
    gNdsStageMPPlatformTickFloorLoopStatusAfter =
        (u32)yakumono_dobj->user_data.s;
    gNdsStageMPPlatformTickFloorLoopPredicateAfter =
        (mpCollisionCheckExistPlatformLineID(line_id) != FALSE) ? 1u : 0u;
}

void ndsFighterMarioFoxStageMPPlatformTickFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPPlatformTickFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPPlatformTickFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPPlatformTickFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPPlatformFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPPlatformFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPPlatformFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPPlatformFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP_SAFE_PASS) &&
        (gNdsStageMPPlatformFloorLoopPredicateResult != 0u) &&
        (gNdsStageMPPlatformFloorLoopBlockerMask == 0u))
    {
        gNdsStageMPPlatformTickFloorLoopBaseActiveSeen = 1u;
        mask |= 1u << 0;
    }
    if (gNdsStageMPPlatformTickFloorLoopPrepared != 0u)
    {
        mask |= 1u << 1;
    }
    if (gNdsStageMPPlatformTickFloorLoopBaseActiveSeen != 0u)
    {
        ndsStageMPPlatformTickFloorLoopRunProof();
    }
    if (gNdsStageMPPlatformTickFloorLoopSetOnCount > 0u)
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPPlatformTickFloorLoopAdvanceCount == 1u) &&
        (gNdsStageMPPlatformTickFloorLoopUpdateTicAfter ==
            (gNdsStageMPPlatformTickFloorLoopUpdateTicBefore + 1u)))
    {
        mask |= 1u << 3;
    }
    if (gNdsStageMPPlatformTickFloorLoopDObjPresent != 0u)
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPPlatformTickFloorLoopStatusBefore ==
            (u32)nMPYakumonoStatusOn) &&
        (gNdsStageMPPlatformTickFloorLoopStatusAfter ==
            (u32)nMPYakumonoStatusOn))
    {
        mask |= 1u << 5;
    }
    if (gNdsStageMPPlatformTickFloorLoopPredicateAfter != 0u)
    {
        mask |= 1u << 6;
    }
    if (gNdsStageMPPlatformTickFloorLoopUnsafeCount == 0u)
    {
        mask |= 1u << 7;
    }

    gNdsFighterMarioFoxStageMPPlatformTickFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPPlatformTickFloorLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageMPPlatformTickFloorLoopCount =
        gNdsStageMPPlatformTickFloorLoopAdvanceCount;
    if ((mask & 0xffu) == 0xffu)
    {
        gNdsFighterMarioFoxStageMPPlatformTickFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPPlatformTickFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPPassInputLoopReset(void)
{
    gNdsFighterMarioFoxStageMPPassInputLoopResult = 0u;
    gNdsFighterMarioFoxStageMPPassInputLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPPassInputLoopMask = 0u;
    gNdsFighterMarioFoxStageMPPassInputLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPPassInputLoopCount = 0u;
    gNdsStageMPPassInputLoopPrepared = 0u;
    gNdsStageMPPassInputLoopBasePlatformTickSeen = 0u;
    gNdsStageMPPassInputLoopCheckCallCount = 0u;
    gNdsStageMPPassInputLoopCheckSuccessCount = 0u;
    gNdsStageMPPassInputLoopSquatSetCount = 0u;
    gNdsStageMPPassInputLoopSquatProcCount = 0u;
    gNdsStageMPPassInputLoopGotoPassCount = 0u;
    gNdsStageMPPassInputLoopPassSetCount = 0u;
    gNdsStageMPPassInputLoopSquatWaitSetCount = 0u;
    gNdsStageMPPassInputLoopSquatWaitProcCount = 0u;
    gNdsStageMPPassInputLoopSquatRvSetCount = 0u;
    gNdsStageMPPassInputLoopSquatRvEndCount = 0u;
    gNdsStageMPPassInputLoopSquatRvCallbackMask = 0u;
    gNdsStageMPPassInputLoopSetAirCount = 0u;
    gNdsStageMPPassInputLoopClampCount = 0u;
    gNdsStageMPPassInputLoopUnsafeCount = 0u;
    gNdsStageMPPassInputLoopLineID = -1;
    gNdsStageMPPassInputLoopFlags = 0u;
    gNdsStageMPPassInputLoopStickY = 0;
    gNdsStageMPPassInputLoopTapYBefore = 0u;
    gNdsStageMPPassInputLoopTapYAfter = 0u;
    gNdsStageMPPassInputLoopStatusBefore = 0u;
    gNdsStageMPPassInputLoopStatusAfterSquat = 0u;
    gNdsStageMPPassInputLoopStatusAfterPass = 0u;
    gNdsStageMPPassInputLoopGAAfterPass = 0u;
    gNdsStageMPPassInputLoopStatusAfterSquatWait = 0u;
    gNdsStageMPPassInputLoopStatusAfterSquatRv = 0u;
    gNdsStageMPPassInputLoopStatusAfterSquatRvEnd = 0u;
    gNdsStageMPPassInputLoopGAAfterSquatRvEnd = 0u;
    gNdsStageMPPassInputLoopIgnoreLineID = -1;
    gNdsStageMPPassInputLoopPassWaitInitial = -1;
    gNdsStageMPPassInputLoopPassWaitFinal = -1;
    sNdsStageMPPassInputLoopInputActive = FALSE;
    sNdsStageMPPassInputLoopStatusActive = FALSE;
}

void ndsFighterMarioFoxStageMPPassInputLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPPassInputLoopProofEnabled() == FALSE) ||
        (gNdsStageMPPassInputLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPPassInputLoopReset();
    gNdsStageMPPassInputLoopPrepared = 1u;
}

static void ndsStageMPPassInputLoopRunProof(void)
{
    FTStruct *fp = &sNdsFighterStructPool[0];
    GObj *fighter_gobj;
    u32 i;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPPassInputLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    gNdsStageMPPassInputLoopLineID = gNdsStageMPPassFloorLoopCandidateLineID;
    gNdsStageMPPassInputLoopFlags =
        gNdsStageMPPassFloorLoopCandidateFlags | MAP_VERTEX_COLL_PASS;
    if (gNdsStageMPPassInputLoopLineID < 0)
    {
        gNdsStageMPPassInputLoopUnsafeCount++;
        return;
    }

    fp->status_id = nFTCommonStatusWait;
    fp->motion_id = nFTCommonMotionWait;
    fp->motion_script_id = nFTCommonMotionWait;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_line_id = gNdsStageMPPassInputLoopLineID;
    fp->coll_data.floor_flags = gNdsStageMPPassInputLoopFlags;
    fp->coll_data.mask_stat |= MAP_FLAG_FLOOR;
    fp->coll_data.ignore_line_id = -1;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = FTCOMMON_PASS_STICK_RANGE_MIN;
    fp->tap_stick_y = 0u;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 8.0F;

    gNdsStageMPPassInputLoopStickY = fp->input.pl.stick_range.y;
    gNdsStageMPPassInputLoopTapYBefore = fp->tap_stick_y;
    gNdsStageMPPassInputLoopStatusBefore = fp->status_id;

    sNdsStageMPPassInputLoopInputActive = TRUE;
    sNdsStageMPPassInputLoopStatusActive = TRUE;
    if (ftCommonPassCheckInterruptCommon(fighter_gobj) == FALSE)
    {
        gNdsStageMPPassInputLoopUnsafeCount++;
    }
    sNdsStageMPPassInputLoopInputActive = FALSE;

    gNdsStageMPPassInputLoopStatusAfterSquat = fp->status_id;
    gNdsStageMPPassInputLoopPassWaitInitial =
        fp->status_vars.common.squat.pass_wait;

    if (fp->proc_interrupt != ndsBaseFTCommonSquatProcInterrupt)
    {
        gNdsStageMPPassInputLoopUnsafeCount++;
    }
    else for (i = 0u; i < 8u; i++)
    {
        fp->proc_interrupt(fighter_gobj);
        gNdsStageMPPassInputLoopSquatProcCount++;
        if (fp->status_id == nFTCommonStatusPass)
        {
            gNdsStageMPPassInputLoopGotoPassCount++;
            break;
        }
        if (fp->status_id != nFTCommonStatusSquat)
        {
            gNdsStageMPPassInputLoopUnsafeCount++;
            break;
        }
    }
    sNdsStageMPPassInputLoopStatusActive = FALSE;

    gNdsStageMPPassInputLoopStatusAfterPass = fp->status_id;
    gNdsStageMPPassInputLoopGAAfterPass = fp->ga;
    gNdsStageMPPassInputLoopIgnoreLineID = fp->coll_data.ignore_line_id;
    gNdsStageMPPassInputLoopTapYAfter = fp->tap_stick_y;
    gNdsStageMPPassInputLoopPassWaitFinal =
        fp->status_vars.common.squat.pass_wait;

    fp->status_id = nFTCommonStatusWait;
    fp->motion_id = nFTCommonMotionWait;
    fp->motion_script_id = nFTCommonMotionWait;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_line_id = gNdsStageMPPassInputLoopLineID;
    fp->coll_data.floor_flags = gNdsStageMPPassInputLoopFlags &
        ~MAP_VERTEX_COLL_PASS;
    fp->coll_data.mask_stat |= MAP_FLAG_FLOOR;
    fp->coll_data.ignore_line_id = -1;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = FTCOMMON_SQUAT_STICK_RANGE_MIN;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.button_hold = 0u;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->status_vars.common.squat.is_allow_pass = FALSE;
    fp->status_vars.common.squat.pass_wait = 0;
    fp->is_special_interrupt = FALSE;

    sNdsStageMPPassInputLoopStatusActive = TRUE;
    ndsBaseFTCommonSquatWaitSetStatus(fighter_gobj);
    sNdsStageMPPassInputLoopStatusActive = FALSE;

    gNdsStageMPPassInputLoopStatusAfterSquatWait = fp->status_id;
    if (fp->proc_update == ndsBaseFTCommonSquatWaitProcUpdate)
    {
        gNdsStageMPPassInputLoopSquatRvCallbackMask |= 1u << 0;
    }
    if (fp->proc_interrupt == ndsBaseFTCommonSquatWaitProcInterrupt)
    {
        gNdsStageMPPassInputLoopSquatRvCallbackMask |= 1u << 1;
    }

    fp->input.pl.stick_range.y = FTCOMMON_SQUAT_STICK_RANGE_MIN + 4;
    sNdsStageMPPassInputLoopInputActive = TRUE;
    sNdsStageMPPassInputLoopStatusActive = TRUE;
    if (fp->proc_interrupt == ndsBaseFTCommonSquatWaitProcInterrupt)
    {
        fp->proc_interrupt(fighter_gobj);
        gNdsStageMPPassInputLoopSquatWaitProcCount++;
    }
    else
    {
        gNdsStageMPPassInputLoopUnsafeCount++;
    }
    sNdsStageMPPassInputLoopInputActive = FALSE;
    sNdsStageMPPassInputLoopStatusActive = FALSE;

    gNdsStageMPPassInputLoopStatusAfterSquatRv = fp->status_id;
    if (fp->proc_update == ftAnimEndSetWait)
    {
        gNdsStageMPPassInputLoopSquatRvCallbackMask |= 1u << 2;
    }
    if (fp->proc_interrupt == ndsBaseFTCommonSquatRvProcInterrupt)
    {
        gNdsStageMPPassInputLoopSquatRvCallbackMask |= 1u << 3;
    }

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;
    sNdsStageMPPassInputLoopStatusActive = TRUE;
    if (fp->proc_update == ftAnimEndSetWait)
    {
        fp->proc_update(fighter_gobj);
        gNdsStageMPPassInputLoopSquatRvEndCount++;
    }
    else
    {
        gNdsStageMPPassInputLoopUnsafeCount++;
    }
    sNdsStageMPPassInputLoopStatusActive = FALSE;

    gNdsStageMPPassInputLoopStatusAfterSquatRvEnd = fp->status_id;
    gNdsStageMPPassInputLoopGAAfterSquatRvEnd = fp->ga;
}

void ndsFighterMarioFoxStageMPPassInputLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPPassInputLoopProofEnabled() == FALSE) ||
        (gNdsStageMPPassInputLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPPassInputLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPPlatformTickFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPPlatformTickFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPPlatformTickFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPPlatformTickFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPPassInputLoopBasePlatformTickSeen = 1u;
        mask |= 1u << 0;
    }
    if (gNdsStageMPPassInputLoopPrepared != 0u)
    {
        mask |= 1u << 1;
    }
    if (gNdsStageMPPassInputLoopBasePlatformTickSeen != 0u)
    {
        ndsStageMPPassInputLoopRunProof();
    }
    if ((gNdsStageMPPassInputLoopCheckCallCount == 1u) &&
        (gNdsStageMPPassInputLoopCheckSuccessCount == 1u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPPassInputLoopLineID >= 0) &&
        ((gNdsStageMPPassInputLoopFlags & MAP_VERTEX_COLL_PASS) != 0u) &&
        (gNdsStageMPPassInputLoopStickY <= FTCOMMON_PASS_STICK_RANGE_MIN) &&
        (gNdsStageMPPassInputLoopTapYBefore < FTCOMMON_PASS_BUFFER_TICS_MAX))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPPassInputLoopSquatSetCount == 1u) &&
        (gNdsStageMPPassInputLoopStatusBefore == (u32)nFTCommonStatusWait) &&
        (gNdsStageMPPassInputLoopStatusAfterSquat ==
            (u32)nFTCommonStatusSquat) &&
        (gNdsStageMPPassInputLoopPassWaitInitial ==
            FTCOMMON_SQUAT_PASS_WAIT))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPPassInputLoopSquatProcCount ==
            FTCOMMON_SQUAT_PASS_WAIT) &&
        (gNdsStageMPPassInputLoopGotoPassCount == 1u) &&
        (gNdsStageMPPassInputLoopPassSetCount == 1u) &&
        (gNdsStageMPPassInputLoopStatusAfterPass ==
            (u32)nFTCommonStatusPass))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPPassInputLoopGAAfterPass == (u32)nMPKineticsAir) &&
        (gNdsStageMPPassInputLoopIgnoreLineID ==
            gNdsStageMPPassInputLoopLineID) &&
        (gNdsStageMPPassInputLoopTapYAfter ==
            (u32)FTINPUT_STICKBUFFER_TICS_MAX) &&
        (gNdsStageMPPassInputLoopPassWaitFinal == 0))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPPassInputLoopSetAirCount == 1u) &&
        (gNdsStageMPPassInputLoopClampCount == 1u) &&
        (gNdsStageMPPassInputLoopUnsafeCount == 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPPassInputLoopSquatWaitSetCount == 1u) &&
        (gNdsStageMPPassInputLoopStatusAfterSquatWait ==
            (u32)nFTCommonStatusSquatWait) &&
        ((gNdsStageMPPassInputLoopSquatRvCallbackMask & 0x3u) == 0x3u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPPassInputLoopSquatWaitProcCount == 1u) &&
        (gNdsStageMPPassInputLoopSquatRvSetCount == 1u) &&
        (gNdsStageMPPassInputLoopStatusAfterSquatRv ==
            (u32)nFTCommonStatusSquatRv) &&
        ((gNdsStageMPPassInputLoopSquatRvCallbackMask & 0xcu) == 0xcu))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPPassInputLoopSquatRvEndCount == 1u) &&
        (gNdsStageMPPassInputLoopStatusAfterSquatRvEnd ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPPassInputLoopGAAfterSquatRvEnd ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxStageMPPassInputLoopMask = mask;
    gNdsFighterMarioFoxStageMPPassInputLoopDeferredMask = 0x7ffu;
    gNdsFighterMarioFoxStageMPPassInputLoopCount =
        gNdsStageMPPassInputLoopPassSetCount;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxStageMPPassInputLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASS_INPUT_LOOP_PASS;
        gNdsFighterMarioFoxStageMPPassInputLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASS_INPUT_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPPlatformPosFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPPlatformPosFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPPlatformPosFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPPlatformPosFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPPlatformPosFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPPlatformPosFloorLoopCount = 0u;
    gNdsStageMPPlatformPosFloorLoopPrepared = 0u;
    gNdsStageMPPlatformPosFloorLoopBasePassInputSeen = 0u;
    gNdsStageMPPlatformPosFloorLoopSetPosCount = 0u;
    gNdsStageMPPlatformPosFloorLoopUnsafeCount = 0u;
    gNdsStageMPPlatformPosFloorLoopLineID = -1;
    gNdsStageMPPlatformPosFloorLoopYakumonoID = 0u;
    gNdsStageMPPlatformPosFloorLoopDObjPresent = 0u;
    gNdsStageMPPlatformPosFloorLoopStatusBefore = 0u;
    gNdsStageMPPlatformPosFloorLoopStatusAfter = 0u;
    gNdsStageMPPlatformPosFloorLoopPredicateAfter = 0u;
    gNdsStageMPPlatformPosFloorLoopBeforeXMilli = 0;
    gNdsStageMPPlatformPosFloorLoopBeforeYMilli = 0;
    gNdsStageMPPlatformPosFloorLoopBeforeZMilli = 0;
    gNdsStageMPPlatformPosFloorLoopTargetXMilli = 0;
    gNdsStageMPPlatformPosFloorLoopTargetYMilli = 0;
    gNdsStageMPPlatformPosFloorLoopTargetZMilli = 0;
    gNdsStageMPPlatformPosFloorLoopAfterXMilli = 0;
    gNdsStageMPPlatformPosFloorLoopAfterYMilli = 0;
    gNdsStageMPPlatformPosFloorLoopAfterZMilli = 0;
    gNdsStageMPPlatformPosFloorLoopSpeedXMilli = 0;
    gNdsStageMPPlatformPosFloorLoopSpeedYMilli = 0;
    gNdsStageMPPlatformPosFloorLoopSpeedZMilli = 0;
}

void ndsFighterMarioFoxStageMPPlatformPosFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPPlatformPosFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPPlatformPosFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPPlatformPosFloorLoopReset();
    gNdsStageMPPlatformPosFloorLoopPrepared = 1u;
}

static void ndsStageMPPlatformPosFloorLoopRunProof(void)
{
    Vec3f target;
    DObj *yakumono_dobj;
    s32 line_id = gNdsStageMPPlatformFloorLoopLineID;
    u32 yakumono_id = gNdsStageMPPlatformFloorLoopYakumonoID;

    if ((line_id < 0) || (gMPCollisionYakumonoDObjs == NULL) ||
        (gMPCollisionSpeeds == NULL) ||
        (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
    {
        gNdsStageMPPlatformPosFloorLoopUnsafeCount++;
        return;
    }
    yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
    if (yakumono_dobj == NULL)
    {
        gNdsStageMPPlatformPosFloorLoopUnsafeCount++;
        return;
    }

    gNdsStageMPPlatformPosFloorLoopLineID = line_id;
    gNdsStageMPPlatformPosFloorLoopYakumonoID = yakumono_id;
    gNdsStageMPPlatformPosFloorLoopDObjPresent = 1u;
    gNdsStageMPPlatformPosFloorLoopStatusBefore =
        (u32)yakumono_dobj->user_data.s;
    gNdsStageMPPlatformPosFloorLoopBeforeXMilli =
        ndsFloatToMilliSigned(yakumono_dobj->translate.vec.f.x);
    gNdsStageMPPlatformPosFloorLoopBeforeYMilli =
        ndsFloatToMilliSigned(yakumono_dobj->translate.vec.f.y);
    gNdsStageMPPlatformPosFloorLoopBeforeZMilli =
        ndsFloatToMilliSigned(yakumono_dobj->translate.vec.f.z);

    target = yakumono_dobj->translate.vec.f;
    target.x += 12.0F;
    target.y -= 4.0F;
    target.z += 2.0F;
    gNdsStageMPPlatformPosFloorLoopTargetXMilli =
        ndsFloatToMilliSigned(target.x);
    gNdsStageMPPlatformPosFloorLoopTargetYMilli =
        ndsFloatToMilliSigned(target.y);
    gNdsStageMPPlatformPosFloorLoopTargetZMilli =
        ndsFloatToMilliSigned(target.z);

    mpCollisionSetYakumonoPosID((s32)yakumono_id, &target);

    gNdsStageMPPlatformPosFloorLoopAfterXMilli =
        ndsFloatToMilliSigned(yakumono_dobj->translate.vec.f.x);
    gNdsStageMPPlatformPosFloorLoopAfterYMilli =
        ndsFloatToMilliSigned(yakumono_dobj->translate.vec.f.y);
    gNdsStageMPPlatformPosFloorLoopAfterZMilli =
        ndsFloatToMilliSigned(yakumono_dobj->translate.vec.f.z);
    gNdsStageMPPlatformPosFloorLoopSpeedXMilli =
        ndsFloatToMilliSigned(gMPCollisionSpeeds[yakumono_id].x);
    gNdsStageMPPlatformPosFloorLoopSpeedYMilli =
        ndsFloatToMilliSigned(gMPCollisionSpeeds[yakumono_id].y);
    gNdsStageMPPlatformPosFloorLoopSpeedZMilli =
        ndsFloatToMilliSigned(gMPCollisionSpeeds[yakumono_id].z);
    gNdsStageMPPlatformPosFloorLoopStatusAfter =
        (u32)yakumono_dobj->user_data.s;
    gNdsStageMPPlatformPosFloorLoopPredicateAfter =
        (mpCollisionCheckExistPlatformLineID(line_id) != FALSE) ? 1u : 0u;
}

void ndsFighterMarioFoxStageMPPlatformPosFloorLoopFinalize(void)
{
    u32 mask = 0u;
    s32 target_dx;
    s32 target_dy;
    s32 target_dz;

    if ((ndsFighterMarioFoxStageMPPlatformPosFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPPlatformPosFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPPlatformPosFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPPassInputLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPPassInputLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPPassInputLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASS_INPUT_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPPassInputLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASS_INPUT_LOOP_SAFE_PASS))
    {
        gNdsStageMPPlatformPosFloorLoopBasePassInputSeen = 1u;
        mask |= 1u << 0;
    }
    if (gNdsStageMPPlatformPosFloorLoopPrepared != 0u)
    {
        mask |= 1u << 1;
    }
    if (gNdsStageMPPlatformPosFloorLoopBasePassInputSeen != 0u)
    {
        ndsStageMPPlatformPosFloorLoopRunProof();
    }
    if ((gNdsStageMPPlatformPosFloorLoopSetPosCount == 1u) &&
        (gNdsStageMPPlatformPosFloorLoopUnsafeCount == 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPPlatformPosFloorLoopLineID >= 0) &&
        (gNdsStageMPPlatformPosFloorLoopDObjPresent != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPPlatformPosFloorLoopAfterXMilli ==
            gNdsStageMPPlatformPosFloorLoopTargetXMilli) &&
        (gNdsStageMPPlatformPosFloorLoopAfterYMilli ==
            gNdsStageMPPlatformPosFloorLoopTargetYMilli) &&
        (gNdsStageMPPlatformPosFloorLoopAfterZMilli ==
            gNdsStageMPPlatformPosFloorLoopTargetZMilli))
    {
        mask |= 1u << 4;
    }

    target_dx = gNdsStageMPPlatformPosFloorLoopTargetXMilli -
        gNdsStageMPPlatformPosFloorLoopBeforeXMilli;
    target_dy = gNdsStageMPPlatformPosFloorLoopTargetYMilli -
        gNdsStageMPPlatformPosFloorLoopBeforeYMilli;
    target_dz = gNdsStageMPPlatformPosFloorLoopTargetZMilli -
        gNdsStageMPPlatformPosFloorLoopBeforeZMilli;
    if ((gNdsStageMPPlatformPosFloorLoopSpeedXMilli == target_dx) &&
        (gNdsStageMPPlatformPosFloorLoopSpeedYMilli == target_dy) &&
        (gNdsStageMPPlatformPosFloorLoopSpeedZMilli == target_dz))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPPlatformPosFloorLoopStatusBefore ==
            (u32)nMPYakumonoStatusOn) &&
        (gNdsStageMPPlatformPosFloorLoopStatusAfter ==
            (u32)nMPYakumonoStatusOn) &&
        (gNdsStageMPPlatformPosFloorLoopPredicateAfter != 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 7;
    }

    gNdsFighterMarioFoxStageMPPlatformPosFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPPlatformPosFloorLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageMPPlatformPosFloorLoopCount =
        gNdsStageMPPlatformPosFloorLoopSetPosCount;
    if ((mask & 0xffu) == 0xffu)
    {
        gNdsFighterMarioFoxStageMPPlatformPosFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPPlatformPosFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopPrepared = 0u;
    gNdsStageMPPlatformSpeedFloorLoopBasePosSeen = 0u;
    gNdsStageMPPlatformSpeedFloorLoopGetSpeedCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopUnsafeCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopLineID = -1;
    gNdsStageMPPlatformSpeedFloorLoopYakumonoID = 0u;
    gNdsStageMPPlatformSpeedFloorLoopStatus = 0u;
    gNdsStageMPPlatformSpeedFloorLoopPredicate = 0u;
    gNdsStageMPPlatformSpeedFloorLoopExpectedXMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopExpectedYMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopExpectedZMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopReadXMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopReadYMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopReadZMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopDynamicBranchCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopDynamicProbeCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopDynamicHitCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopDynamicLineID = -1;
    gNdsStageMPPlatformSpeedFloorLoopDynamicYakumonoID = 0u;
    gNdsStageMPPlatformSpeedFloorLoopDynamicSpeedXMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopDynamicSpeedYMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopDynamicGaXMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopDynamicGaYMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopDynamicCeilProbeCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopDynamicCeilHitCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopDynamicCeilLineID = -1;
    gNdsStageMPPlatformSpeedFloorLoopDynamicCeilYakumonoID = 0u;
    gNdsStageMPPlatformSpeedFloorLoopDynamicCeilGaXMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopDynamicCeilGaYMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopAnimPlayCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopAnimTicBefore = 0u;
    gNdsStageMPPlatformSpeedFloorLoopAnimTicAfter = 0u;
    gNdsStageMPPlatformSpeedFloorLoopAnimStatusBefore = 0u;
    gNdsStageMPPlatformSpeedFloorLoopAnimStatusAfter = 0u;
    gNdsStageMPPlatformSpeedFloorLoopAnimSpeedXMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopAnimSpeedYMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopAnimSpeedZMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallProbeCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallHitCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallLineID = -1;
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallYakumonoID = 0u;
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallKind = 0xffffffffu;
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallGaXMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallGaYMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopProcessWallProbeCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopProcessWallHitCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopProcessWallLineID = -1;
    gNdsStageMPPlatformSpeedFloorLoopProcessWallKind = 0xffffffffu;
    gNdsStageMPPlatformSpeedFloorLoopProcessWallMaskCurr = 0u;
    gNdsStageMPPlatformSpeedFloorLoopProcessWallMultiCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopBoundsUpdateCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopBoundsDiffTopMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopBoundsDiffBottomMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopBoundsDiffRightMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopBoundsDiffLeftMilli = 0;
    gNdsStageMPPlatformSpeedFloorLoopStageAnimMask = 0u;
    gNdsStageMPPlatformSpeedFloorLoopStageAnimDObjCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopStageAnimMObjCount = 0u;
    gNdsStageMPPlatformSpeedFloorLoopStageAnimCallbackCount = 0u;
    gNdsStageInishieRelocResult = 0u;
    gNdsStageInishieMapHeaderOffset = 0u;
    gNdsStageInishieGroundDataPtrReady = 0u;
    gNdsStageInishieGeometryPtrReady = 0u;
    gNdsStageInishieMapNodesPtrReady = 0u;
    gNdsStageInishieYakumonoCount = 0u;
    gNdsStageInishieMapObjCount = 0u;
}

void ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPPlatformSpeedFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopReset();
    gNdsStageMPPlatformSpeedFloorLoopPrepared = 1u;
}

static sb32 ndsStageMPPlatformSpeedFloorLoopChooseCeilLine(u32 yakumono_id,
                                                           s32 *line_id,
                                                           f32 *sample_x,
                                                           f32 *ceil_y)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;

    if (line_id != NULL)
    {
        *line_id = -1;
    }
    if (sample_x != NULL)
    {
        *sample_x = 0.0F;
    }
    if (ceil_y != NULL)
    {
        *ceil_y = 0.0F;
    }
    if ((ndsStageCollisionLoopGeometryReady() == FALSE) ||
        (line_id == NULL) || (sample_x == NULL) || (ceil_y == NULL))
    {
        return FALSE;
    }

    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);
        s32 first;
        s32 count;
        s32 end;
        s32 candidate;

        if (ndsMPLineInfoYakumonoID(info) != yakumono_id)
        {
            continue;
        }
        first = (s32)ndsMPLineInfoGroupID(info, nMPLineKindCeil);
        count = (s32)ndsMPLineInfoLineCount(info, nMPLineKindCeil);
        if (count <= 0)
        {
            continue;
        }
        if (count > 4096)
        {
            count = 4096;
        }
        end = first + count;
        for (candidate = first; candidate < end; candidate++)
        {
            Vec3f left;
            Vec3f right;
            Vec3f probe;
            f32 dist = 0.0F;

            if ((ndsMPFindLineEndpoints(candidate, &left, &right, NULL,
                    NULL) == FALSE) ||
                (ndsMPLineIDIsCeil(candidate) == FALSE) ||
                (fabsf(right.x - left.x) < 64.0F))
            {
                continue;
            }
            probe.x = (left.x + right.x) * 0.5F;
            probe.y = 0.0F;
            probe.z = 0.0F;
            if (mpCollisionGetFCCommonCeil(candidate, &probe, &dist,
                    NULL, NULL) == FALSE)
            {
                continue;
            }
            *line_id = candidate;
            *sample_x = probe.x;
            *ceil_y = probe.y + dist;
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsStageMPPlatformSpeedFloorLoopChooseWallLine(u32 yakumono_id,
                                                           s32 *line_id,
                                                           u32 *line_kind,
                                                           f32 *sample_x,
                                                           f32 *sample_y)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;

    if (line_id != NULL)
    {
        *line_id = -1;
    }
    if (line_kind != NULL)
    {
        *line_kind = 0xffffffffu;
    }
    if ((ndsStageCollisionLoopGeometryReady() == FALSE) ||
        (line_id == NULL) || (line_kind == NULL) ||
        (sample_x == NULL) || (sample_y == NULL))
    {
        return FALSE;
    }

    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    for (i = 0u; i < yakumono_count; i++)
    {
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);
        u32 kinds[2] = { (u32)nMPLineKindLWall, (u32)nMPLineKindRWall };
        u32 kind_index;

        if (ndsMPLineInfoYakumonoID(info) != yakumono_id)
        {
            continue;
        }
        for (kind_index = 0u; kind_index < 2u; kind_index++)
        {
            u32 kind = kinds[kind_index];
            s32 first = (s32)ndsMPLineInfoGroupID(info, kind);
            s32 count = (s32)ndsMPLineInfoLineCount(info, kind);
            s32 end;
            s32 candidate;

            if (count <= 0)
            {
                continue;
            }
            if (count > 4096)
            {
                count = 4096;
            }
            end = first + count;
            for (candidate = first; candidate < end; candidate++)
            {
                Vec3f a;
                Vec3f b;

                if (ndsMPFindLineEndpoints(candidate, &a, &b, NULL,
                        NULL) == FALSE)
                {
                    continue;
                }
                *line_id = candidate;
                *line_kind = kind;
                *sample_x = (a.x + b.x) * 0.5F;
                *sample_y = (a.y + b.y) * 0.5F;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static sb32 ndsStageMPPlatformSpeedFloorLoopAddAnimTrack(DObj *dobj,
                                                         u8 track,
                                                         f32 base,
                                                         f32 rate)
{
    AObj *aobj = gcAddAObjForDObj(dobj, track);

    if (aobj == NULL)
    {
        return FALSE;
    }
    aobj->kind = nGCAnimKindLinear;
    aobj->value_base = base;
    aobj->value_target = base + rate;
    aobj->rate_base = rate;
    aobj->rate_target = 0.0F;
    aobj->length = 0.0F;
    aobj->length_invert = 1.0F;
    return TRUE;
}

static sb32 ndsStageMPPlatformSpeedFloorLoopRunAnimProbe(DObj *yakumono_dobj,
                                                         Vec3f *speed)
{
    GObj *ground_gobj;

    if ((yakumono_dobj == NULL) || (speed == NULL) ||
        (yakumono_dobj->parent_gobj == NULL) ||
        (yakumono_dobj->aobj != NULL))
    {
        return FALSE;
    }

    if ((ndsStageMPPlatformSpeedFloorLoopAddAnimTrack(yakumono_dobj,
             nGCAnimTrackTraX, yakumono_dobj->translate.vec.f.x,
             speed->x) == FALSE) ||
        (ndsStageMPPlatformSpeedFloorLoopAddAnimTrack(yakumono_dobj,
             nGCAnimTrackTraY, yakumono_dobj->translate.vec.f.y,
             speed->y) == FALSE) ||
        (ndsStageMPPlatformSpeedFloorLoopAddAnimTrack(yakumono_dobj,
             nGCAnimTrackTraZ, yakumono_dobj->translate.vec.f.z,
             speed->z) == FALSE))
    {
        return FALSE;
    }

    yakumono_dobj->anim_joint.event32 = NULL;
    yakumono_dobj->anim_wait = 1.0F;
    yakumono_dobj->anim_speed = 1.0F;
    yakumono_dobj->anim_frame = 0.0F;
    yakumono_dobj->user_data.s = nMPYakumonoStatusShow;
    ground_gobj = yakumono_dobj->parent_gobj;
    ground_gobj->anim_frame = 0.0F;

    gNdsStageMPPlatformSpeedFloorLoopAnimStatusBefore =
        (u32)yakumono_dobj->user_data.s;
    gNdsStageMPPlatformSpeedFloorLoopAnimTicBefore =
        (u32)gMPCollisionUpdateTic;
    mpCollisionPlayYakumonoAnim(ground_gobj);
    gNdsStageMPPlatformSpeedFloorLoopAnimTicAfter =
        (u32)gMPCollisionUpdateTic;
    return TRUE;
}

static void ndsStageMPPlatformSpeedFloorLoopRecordStageAnimLayer(
    DObj *yakumono_dobj)
{
    GObj *layer_gobj = gGRCommonLayerGObjs[1];
    DObj *dobj;
    u32 mask = 0u;
    u32 dobj_count = 0u;
    u32 mobj_count = 0u;

    if (layer_gobj != NULL)
    {
        mask |= 1u << 0;
    }
    if ((gMPCollisionGroundData != NULL) &&
        (gMPCollisionGroundData->gr_desc[1].anim_joints != NULL))
    {
        mask |= 1u << 1;
    }
    if ((gMPCollisionGroundData != NULL) &&
        (gMPCollisionGroundData->gr_desc[1].p_matanim_joints != NULL))
    {
        mask |= 1u << 2;
    }
    if ((layer_gobj != NULL) && (layer_gobj->gobjproc_head != NULL) &&
        (layer_gobj->gobjproc_head->func_id == mpCollisionPlayYakumonoAnim))
    {
        mask |= 1u << 3;
    }

    dobj = (layer_gobj != NULL) ? DObjGetStruct(layer_gobj) : NULL;
    if (dobj != NULL)
    {
        mask |= 1u << 4;
    }
    while (dobj != NULL)
    {
        MObj *mobj = dobj->mobj;

        if ((dobj->anim_joint.event32 != NULL) || (dobj->aobj != NULL))
        {
            dobj_count++;
        }
        while (mobj != NULL)
        {
            if ((mobj->matanim_joint.event32 != NULL) || (mobj->aobj != NULL))
            {
                mobj_count++;
            }
            mobj = mobj->next;
        }
        dobj = gcGetTreeDObjNext(dobj);
    }
    if (dobj_count != 0u)
    {
        mask |= 1u << 5;
    }
    if (mobj_count != 0u)
    {
        mask |= 1u << 6;
    }
    if ((yakumono_dobj != NULL) && (yakumono_dobj->parent_gobj == layer_gobj))
    {
        mask |= 1u << 7;
    }

    gNdsStageMPPlatformSpeedFloorLoopStageAnimMask = mask;
    gNdsStageMPPlatformSpeedFloorLoopStageAnimDObjCount = dobj_count;
    gNdsStageMPPlatformSpeedFloorLoopStageAnimMObjCount = mobj_count;
}

static void ndsStageMPPlatformSpeedFloorLoopProbeInishieAsset(void)
{
    void *file;
    MPGroundData *ground_data;
    size_t file_size;

    if ((gNdsStageInishieRelocResult == NDS_STAGE_INISHIE_RELOC_PASS) &&
        (sNdsStageInishieMapFile != NULL) &&
        (sNdsStageInishieGroundData != NULL) &&
        (sNdsStageInishieMapDataSize != 0u))
    {
        return;
    }

    file_size = lbRelocGetFileSize(&llGRInishieMapFileID);
    if (file_size == 0u)
    {
        return;
    }

    file = lbRelocGetExternHeapFile(&llGRInishieMapFileID,
                                    syTaskmanMalloc(file_size, 0x10));
    if (file == NULL)
    {
        return;
    }

    ground_data = lbRelocGetFileData(MPGroundData*,
                                     file,
                                     &llGRInishieMapMapHeader);
    if (ground_data != NULL)
    {
        sNdsStageInishieMapFile = file;
        sNdsStageInishieGroundData = ground_data;
        sNdsStageInishieMapDataSize = file_size;
    }
}

static sb32 ndsStageMPPlatformSpeedFloorLoopRunWallProbe(DObj *yakumono_dobj,
                                                         Vec3f *speed,
                                                         u32 yakumono_id)
{
    Vec3f prev;
    Vec3f curr;
    Vec3f ga_last;
    Vec3f angle;
    s32 line_id = -1;
    s32 hit_line_id = -1;
    u32 line_kind = 0xffffffffu;
    u32 hit_flags = 0u;
    f32 sample_x = 0.0F;
    f32 sample_y = 0.0F;
    sb32 hit;

    if ((yakumono_dobj == NULL) || (speed == NULL) ||
        (ndsStageMPPlatformSpeedFloorLoopChooseWallLine(yakumono_id,
            &line_id, &line_kind, &sample_x, &sample_y) == FALSE))
    {
        return FALSE;
    }

    prev.x = yakumono_dobj->translate.vec.f.x + sample_x - 32.0F - speed->x;
    prev.y = yakumono_dobj->translate.vec.f.y + sample_y - speed->y;
    prev.z = 0.0F;
    curr.x = yakumono_dobj->translate.vec.f.x + sample_x + 32.0F;
    curr.y = yakumono_dobj->translate.vec.f.y + sample_y;
    curr.z = 0.0F;

    gNdsStageMPPlatformSpeedFloorLoopDynamicWallProbeCount++;
    if (line_kind == (u32)nMPLineKindLWall)
    {
        hit = mpCollisionCheckLWallLineCollisionDiff(&prev, &curr, &ga_last,
                                                     &hit_line_id, &hit_flags,
                                                     &angle);
    }
    else
    {
        hit = mpCollisionCheckRWallLineCollisionDiff(&prev, &curr, &ga_last,
                                                     &hit_line_id, &hit_flags,
                                                     &angle);
    }
    (void)hit_flags;
    (void)angle;
    if ((hit == FALSE) || (hit_line_id != line_id))
    {
        return FALSE;
    }
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallHitCount++;
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallLineID = hit_line_id;
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallYakumonoID = yakumono_id;
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallKind = line_kind;
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallGaXMilli =
        ndsFloatToMilliSigned(ga_last.x);
    gNdsStageMPPlatformSpeedFloorLoopDynamicWallGaYMilli =
        ndsFloatToMilliSigned(ga_last.y);
    return TRUE;
}

static sb32 ndsStageMPPlatformSpeedFloorLoopRunProcessWallProbe(
    DObj *yakumono_dobj, Vec3f *speed, u32 yakumono_id)
{
    Vec3f translate;
    MPCollData coll;
    MPObjectColl prev_map_coll;
    s32 line_id = -1;
    u32 line_kind = 0xffffffffu;
    f32 sample_x = 0.0F;
    f32 sample_y = 0.0F;
    sb32 hit;

    if ((yakumono_dobj == NULL) || (speed == NULL) ||
        (ndsStageMPPlatformSpeedFloorLoopChooseWallLine(yakumono_id,
            &line_id, &line_kind, &sample_x, &sample_y) == FALSE))
    {
        return FALSE;
    }

    translate.x = yakumono_dobj->translate.vec.f.x + sample_x + 32.0F;
    translate.y = yakumono_dobj->translate.vec.f.y + sample_y;
    translate.z = 0.0F;

    memset(&coll, 0, sizeof(coll));
    memset(&prev_map_coll, 0, sizeof(prev_map_coll));
    coll.p_translate = &translate;
    coll.p_map_coll = &prev_map_coll;
    coll.pos_prev.x = translate.x - 64.0F - speed->x;
    coll.pos_prev.y = translate.y - speed->y;
    coll.pos_prev.z = 0.0F;
    coll.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    coll.floor_line_id = -1;
    coll.ceil_line_id = -1;
    coll.lwall_line_id = -1;
    coll.rwall_line_id = -1;
    coll.ignore_line_id = -1;

    gNdsStageMPPlatformSpeedFloorLoopProcessWallProbeCount++;
    hit = (line_kind == (u32)nMPLineKindLWall) ?
        mpProcessCheckTestLWallCollisionAdjNew(&coll) :
        mpProcessCheckTestRWallCollisionAdjNew(&coll);
    if (hit == FALSE)
    {
        return FALSE;
    }

    gNdsStageMPPlatformSpeedFloorLoopProcessWallHitCount++;
    gNdsStageMPPlatformSpeedFloorLoopProcessWallLineID = line_id;
    gNdsStageMPPlatformSpeedFloorLoopProcessWallKind = line_kind;
    gNdsStageMPPlatformSpeedFloorLoopProcessWallMaskCurr = coll.mask_curr;
    gNdsStageMPPlatformSpeedFloorLoopProcessWallMultiCount =
        (u32)sMPProcessMultiWallCollidesNum;
    return TRUE;
}

static void ndsStageMPPlatformSpeedFloorLoopRunProof(void)
{
    Vec3f speed;
    Vec3f left;
    Vec3f right;
    Vec3f prev;
    Vec3f curr;
    Vec3f ga_last;
    Vec3f angle;
    DObj *yakumono_dobj;
    f32 sample_x;
    f32 floor_y;
    s32 hit_line_id = -1;
    u32 hit_flags = 0u;
    s32 ceil_line_id = -1;
    f32 ceil_sample_x = 0.0F;
    f32 ceil_y = 0.0F;
    s32 line_id = gNdsStageMPPlatformPosFloorLoopLineID;
    u32 yakumono_id = gNdsStageMPPlatformPosFloorLoopYakumonoID;

    if ((line_id < 0) || (gMPCollisionYakumonoDObjs == NULL) ||
        (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
    {
        gNdsStageMPPlatformSpeedFloorLoopUnsafeCount++;
        return;
    }
    yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
    if (yakumono_dobj == NULL)
    {
        gNdsStageMPPlatformSpeedFloorLoopUnsafeCount++;
        return;
    }

    gNdsStageMPPlatformSpeedFloorLoopLineID = line_id;
    gNdsStageMPPlatformSpeedFloorLoopYakumonoID = yakumono_id;
    gNdsStageMPPlatformSpeedFloorLoopStatus = (u32)yakumono_dobj->user_data.s;
    gNdsStageMPPlatformSpeedFloorLoopPredicate =
        (mpCollisionCheckExistPlatformLineID(line_id) != FALSE) ? 1u : 0u;
    gNdsStageMPPlatformSpeedFloorLoopExpectedXMilli =
        gNdsStageMPPlatformPosFloorLoopSpeedXMilli;
    gNdsStageMPPlatformSpeedFloorLoopExpectedYMilli =
        gNdsStageMPPlatformPosFloorLoopSpeedYMilli;
    gNdsStageMPPlatformSpeedFloorLoopExpectedZMilli =
        gNdsStageMPPlatformPosFloorLoopSpeedZMilli;

    mpCollisionGetSpeedLineID(line_id, &speed);

    gNdsStageMPPlatformSpeedFloorLoopReadXMilli =
        ndsFloatToMilliSigned(speed.x);
    gNdsStageMPPlatformSpeedFloorLoopReadYMilli =
        ndsFloatToMilliSigned(speed.y);
    gNdsStageMPPlatformSpeedFloorLoopReadZMilli =
        ndsFloatToMilliSigned(speed.z);
    ndsStageMPPlatformSpeedFloorLoopRecordStageAnimLayer(yakumono_dobj);

    if ((ndsMPFindLineEndpoints(line_id, &left, &right, NULL, NULL) ==
            FALSE) ||
        (gMPCollisionSpeeds == NULL))
    {
        gNdsStageMPPlatformSpeedFloorLoopUnsafeCount++;
        return;
    }
    sample_x = left.x + ((right.x - left.x) * 0.5F);
    if (ndsStageFloorEdgeLoopFloorYAtX(line_id, sample_x, &floor_y) == FALSE)
    {
        gNdsStageMPPlatformSpeedFloorLoopUnsafeCount++;
        return;
    }

    prev.x = yakumono_dobj->translate.vec.f.x + sample_x - speed.x;
    prev.y = yakumono_dobj->translate.vec.f.y + floor_y + 32.0F - speed.y;
    prev.z = 0.0F;
    curr.x = yakumono_dobj->translate.vec.f.x + sample_x;
    curr.y = yakumono_dobj->translate.vec.f.y + floor_y;
    curr.z = 0.0F;
    gNdsStageMPPlatformSpeedFloorLoopDynamicProbeCount++;
    if (mpCollisionCheckFloorLineCollisionDiff(&prev, &curr, &ga_last,
            &hit_line_id, &hit_flags, &angle) != FALSE)
    {
        (void)hit_flags;
        (void)angle;
        if (hit_line_id == line_id)
        {
            gNdsStageMPPlatformSpeedFloorLoopDynamicHitCount++;
        }
    }

    if (ndsStageMPPlatformSpeedFloorLoopChooseCeilLine(yakumono_id,
            &ceil_line_id, &ceil_sample_x, &ceil_y) == FALSE)
    {
        gNdsStageMPPlatformSpeedFloorLoopUnsafeCount++;
        return;
    }
    prev.x = yakumono_dobj->translate.vec.f.x + ceil_sample_x - speed.x;
    prev.y = yakumono_dobj->translate.vec.f.y + ceil_y - 32.0F - speed.y;
    prev.z = 0.0F;
    curr.x = yakumono_dobj->translate.vec.f.x + ceil_sample_x;
    curr.y = yakumono_dobj->translate.vec.f.y + ceil_y;
    curr.z = 0.0F;
    hit_line_id = -1;
    gNdsStageMPPlatformSpeedFloorLoopDynamicCeilProbeCount++;
    if (mpCollisionCheckCeilLineCollisionDiff(&prev, &curr, &ga_last,
            &hit_line_id, &hit_flags, &angle) != FALSE)
    {
        (void)hit_flags;
        (void)angle;
        if (hit_line_id == ceil_line_id)
        {
            gNdsStageMPPlatformSpeedFloorLoopDynamicCeilHitCount++;
            gNdsStageMPPlatformSpeedFloorLoopDynamicCeilLineID = hit_line_id;
            gNdsStageMPPlatformSpeedFloorLoopDynamicCeilYakumonoID =
                yakumono_id;
            gNdsStageMPPlatformSpeedFloorLoopDynamicCeilGaXMilli =
                ndsFloatToMilliSigned(ga_last.x);
            gNdsStageMPPlatformSpeedFloorLoopDynamicCeilGaYMilli =
                ndsFloatToMilliSigned(ga_last.y);
        }
    }

    if (ndsStageMPPlatformSpeedFloorLoopRunWallProbe(yakumono_dobj, &speed,
            yakumono_id) == FALSE)
    {
        gNdsStageMPPlatformSpeedFloorLoopUnsafeCount++;
    }
    if (ndsStageMPPlatformSpeedFloorLoopRunProcessWallProbe(yakumono_dobj,
            &speed, yakumono_id) == FALSE)
    {
        gNdsStageMPPlatformSpeedFloorLoopUnsafeCount++;
    }

    if (ndsStageMPPlatformSpeedFloorLoopRunAnimProbe(yakumono_dobj,
            &speed) == FALSE)
    {
        gNdsStageMPPlatformSpeedFloorLoopUnsafeCount++;
    }

    ndsStageMPPlatformSpeedFloorLoopProbeInishieAsset();
}

void ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPPlatformSpeedFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPPlatformPosFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPPlatformPosFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPPlatformPosFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPPlatformPosFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPPlatformSpeedFloorLoopBasePosSeen = 1u;
        mask |= 1u << 0;
    }
    if (gNdsStageMPPlatformSpeedFloorLoopPrepared != 0u)
    {
        mask |= 1u << 1;
    }
    if (gNdsStageMPPlatformSpeedFloorLoopBasePosSeen != 0u)
    {
        ndsStageMPPlatformSpeedFloorLoopRunProof();
    }
    if ((gNdsStageMPPlatformSpeedFloorLoopGetSpeedCount == 1u) &&
        (gNdsStageMPPlatformSpeedFloorLoopUnsafeCount == 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPPlatformSpeedFloorLoopLineID >= 0) &&
        (gNdsStageMPPlatformSpeedFloorLoopYakumonoID ==
            gNdsStageMPPlatformPosFloorLoopYakumonoID))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPPlatformSpeedFloorLoopStatus ==
            (u32)nMPYakumonoStatusOn) &&
        (gNdsStageMPPlatformSpeedFloorLoopPredicate != 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPPlatformSpeedFloorLoopReadXMilli ==
            gNdsStageMPPlatformSpeedFloorLoopExpectedXMilli) &&
        (gNdsStageMPPlatformSpeedFloorLoopReadYMilli ==
            gNdsStageMPPlatformSpeedFloorLoopExpectedYMilli) &&
        (gNdsStageMPPlatformSpeedFloorLoopReadZMilli ==
            gNdsStageMPPlatformSpeedFloorLoopExpectedZMilli))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPPlatformSpeedFloorLoopReadXMilli != 0) ||
        (gNdsStageMPPlatformSpeedFloorLoopReadYMilli != 0) ||
        (gNdsStageMPPlatformSpeedFloorLoopReadZMilli != 0))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPPlatformSpeedFloorLoopDynamicBranchCount > 0u) &&
        (gNdsStageMPPlatformSpeedFloorLoopDynamicProbeCount == 1u) &&
        (gNdsStageMPPlatformSpeedFloorLoopDynamicHitCount > 0u) &&
        (gNdsStageMPPlatformSpeedFloorLoopDynamicLineID ==
            gNdsStageMPPlatformSpeedFloorLoopLineID) &&
        (gNdsStageMPPlatformSpeedFloorLoopDynamicYakumonoID ==
            gNdsStageMPPlatformSpeedFloorLoopYakumonoID))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPPlatformSpeedFloorLoopDynamicCeilProbeCount == 1u) &&
        (gNdsStageMPPlatformSpeedFloorLoopDynamicCeilHitCount > 0u) &&
        (gNdsStageMPPlatformSpeedFloorLoopDynamicCeilLineID >= 0) &&
        (gNdsStageMPPlatformSpeedFloorLoopDynamicCeilYakumonoID ==
            gNdsStageMPPlatformSpeedFloorLoopYakumonoID))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPPlatformSpeedFloorLoopAnimPlayCount == 1u) &&
        (gNdsStageMPPlatformSpeedFloorLoopAnimTicAfter ==
            ((gNdsStageMPPlatformSpeedFloorLoopAnimTicBefore + 1u) &
             0xffffu)) &&
        (gNdsStageMPPlatformSpeedFloorLoopAnimStatusBefore ==
            (u32)nMPYakumonoStatusShow) &&
        (gNdsStageMPPlatformSpeedFloorLoopAnimStatusAfter ==
            (u32)nMPYakumonoStatusShow) &&
        (gNdsStageMPPlatformSpeedFloorLoopAnimSpeedXMilli ==
            gNdsStageMPPlatformSpeedFloorLoopExpectedXMilli) &&
        (gNdsStageMPPlatformSpeedFloorLoopAnimSpeedYMilli ==
            gNdsStageMPPlatformSpeedFloorLoopExpectedYMilli) &&
        (gNdsStageMPPlatformSpeedFloorLoopAnimSpeedZMilli ==
            gNdsStageMPPlatformSpeedFloorLoopExpectedZMilli))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPPlatformSpeedFloorLoopDynamicWallProbeCount == 1u) &&
        (gNdsStageMPPlatformSpeedFloorLoopDynamicWallHitCount > 0u) &&
        (gNdsStageMPPlatformSpeedFloorLoopDynamicWallLineID >= 0) &&
        (gNdsStageMPPlatformSpeedFloorLoopDynamicWallYakumonoID ==
            gNdsStageMPPlatformSpeedFloorLoopYakumonoID) &&
        ((gNdsStageMPPlatformSpeedFloorLoopDynamicWallKind ==
            (u32)nMPLineKindLWall) ||
         (gNdsStageMPPlatformSpeedFloorLoopDynamicWallKind ==
            (u32)nMPLineKindRWall)))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPPlatformSpeedFloorLoopProcessWallProbeCount == 1u) &&
        (gNdsStageMPPlatformSpeedFloorLoopProcessWallHitCount > 0u) &&
        (gNdsStageMPPlatformSpeedFloorLoopProcessWallLineID >= 0) &&
        ((gNdsStageMPPlatformSpeedFloorLoopProcessWallKind ==
            (u32)nMPLineKindLWall) ||
         (gNdsStageMPPlatformSpeedFloorLoopProcessWallKind ==
            (u32)nMPLineKindRWall)) &&
        (gNdsStageMPPlatformSpeedFloorLoopProcessWallMaskCurr != 0u) &&
        (gNdsStageMPPlatformSpeedFloorLoopProcessWallMultiCount > 0u))
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageMPPlatformSpeedFloorLoopBoundsUpdateCount == 1u) &&
        ((gNdsStageMPPlatformSpeedFloorLoopBoundsDiffTopMilli != 0) ||
         (gNdsStageMPPlatformSpeedFloorLoopBoundsDiffBottomMilli != 0) ||
         (gNdsStageMPPlatformSpeedFloorLoopBoundsDiffRightMilli != 0) ||
         (gNdsStageMPPlatformSpeedFloorLoopBoundsDiffLeftMilli != 0)))
    {
        mask |= 1u << 13;
    }
    gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopCount =
        gNdsStageMPPlatformSpeedFloorLoopGetSpeedCount;
    if ((mask & 0x3fffu) == 0x3fffu)
    {
        gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageInishieScaleLoopReset(void)
{
    gNdsFighterMarioFoxStageInishieScaleLoopResult = 0u;
    gNdsFighterMarioFoxStageInishieScaleLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageInishieScaleLoopMask = 0u;
    gNdsFighterMarioFoxStageInishieScaleLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageInishieScaleLoopCount = 0u;
    gNdsStageInishieScaleLoopPrepared = 0u;
    gNdsStageInishieScaleLoopBaseSpeedSeen = 0u;
    gNdsStageInishieScaleLoopShellMakeCount = 0u;
    gNdsStageInishieScaleLoopUpdateCount = 0u;
    gNdsStageInishieScaleLoopSetPosCount = 0u;
    gNdsStageInishieScaleLoopSetOnCount = 0u;
    gNdsStageInishieScaleLoopUnsafeCount = 0u;
    gNdsStageInishieScaleLoopDObjMask = 0u;
    gNdsStageInishieScaleLoopLineMask = 0u;
    gNdsStageInishieScaleLoopSetPosMask = 0u;
    gNdsStageInishieScaleLoopSetOnMask = 0u;
    gNdsStageInishieScaleLoopSourceSetupStep = 0u;
    gNdsStageInishieScaleLoopSourceSetupGObjCountBefore = 0u;
    gNdsStageInishieScaleLoopSourceSetupDObjCountBefore = 0u;
    gNdsStageInishieScaleLoopSourceSetupGObjReadyMask = 0u;
    gNdsStageInishieScaleLoopSourceDisplayMask = 0u;
    gNdsStageInishieScaleLoopSourceDisplayCount = 0u;
    gNdsStageInishieScaleLoopSourceDisplayCommands = 0u;
    gNdsStageInishieScaleLoopSourceDisplayTriangles = 0u;
    gNdsStageInishieScaleLoopSourceDisplayBlocker = 0u;
    gNdsStageInishieScaleLoopSourceTextureMask = 0u;
    gNdsStageInishieScaleLoopSourceTextureCommands = 0u;
    gNdsStageInishieScaleLoopSourceTextureImage = 0u;
    gNdsStageInishieScaleLoopSourceTextureSize = 0u;
    gNdsStageInishieScaleLoopSourcePreviewMask = 0u;
    gNdsStageInishieScaleLoopSourcePreviewDObjCount = 0u;
    gNdsStageInishieScaleLoopSourcePreviewVertexCount = 0u;
    gNdsStageInishieScaleLoopSourcePreviewTriangleCount = 0u;
    gNdsStageInishieScaleLoopSourcePreviewValidTriangleCount = 0u;
    gNdsStageInishieScaleLoopSourcePreviewPixelCount = 0u;
    gNdsStageInishieScaleLoopSourcePreviewCommitDelta = 0u;
    gNdsStageInishieScaleLoopSourcePreviewBlocker = 0u;
    gNdsStageInishieScaleLoopLeftLineID = -1;
    gNdsStageInishieScaleLoopRightLineID = -1;
    gNdsStageInishieScaleLoopLeftMapObjKind = 0u;
    gNdsStageInishieScaleLoopRightMapObjKind = 0u;
    gNdsStageInishieScaleLoopStatusBefore = 0u;
    gNdsStageInishieScaleLoopStatusAfter = 0u;
    gNdsStageInishieScaleLoopAltBeforeMilli = 0;
    gNdsStageInishieScaleLoopAltAfterMilli = 0;
    gNdsStageInishieScaleLoopLeftBaseYMilli = 0;
    gNdsStageInishieScaleLoopRightBaseYMilli = 0;
    gNdsStageInishieScaleLoopLeftYBeforeMilli = 0;
    gNdsStageInishieScaleLoopRightYBeforeMilli = 0;
    gNdsStageInishieScaleLoopLeftYAfterMilli = 0;
    gNdsStageInishieScaleLoopRightYAfterMilli = 0;
    gNdsStageInishieScaleLoopLeftSpeedYMilli = 0;
    gNdsStageInishieScaleLoopRightSpeedYMilli = 0;
    gNdsStageInishieScaleLoopFallSetPosCount = 0u;
    gNdsStageInishieScaleLoopFallSparkleCount = 0u;
    gNdsStageInishieScaleLoopFallStatusAfterWait = 0u;
    gNdsStageInishieScaleLoopFallStatusAfterFall = 0u;
    gNdsStageInishieScaleLoopFallAltAfterWaitMilli = 0;
    gNdsStageInishieScaleLoopFallAccelAfterFallMilli = 0;
    gNdsStageInishieScaleLoopFallLeftYAfterFallMilli = 0;
    gNdsStageInishieScaleLoopFallRightYAfterFallMilli = 0;
    gNdsStageInishieScaleLoopFallLeftSpeedYMilli = 0;
    gNdsStageInishieScaleLoopFallRightSpeedYMilli = 0;
    gNdsStageInishieScaleLoopStepSetPosCount = 0u;
    gNdsStageInishieScaleLoopStepSetPosMask = 0u;
    gNdsStageInishieScaleLoopStepSetOnCount = 0u;
    gNdsStageInishieScaleLoopStepSetOnMask = 0u;
    gNdsStageInishieScaleLoopStepWaitBefore = 0u;
    gNdsStageInishieScaleLoopStepWaitAfter = 0u;
    gNdsStageInishieScaleLoopStepStatusAfterSleep = 0u;
    gNdsStageInishieScaleLoopStepStatusAfterRetract = 0u;
    gNdsStageInishieScaleLoopStepAltAfterRetractMilli = 0;
    gNdsStageInishieScaleLoopStepLeftYAfterRetractMilli = 0;
    gNdsStageInishieScaleLoopStepRightYAfterRetractMilli = 0;
    gNdsStageInishieScaleLoopStepLeftSpeedYMilli = 0;
    gNdsStageInishieScaleLoopStepRightSpeedYMilli = 0;
    sNdsStageInishieScaleLoopActive = FALSE;
    sNdsStageInishieScaleLoopPhase = 0u;
    sNdsStageInishieScaleSourceSetupActive = FALSE;
}

void ndsFighterMarioFoxStageInishieScaleLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageInishieScaleLoopProofEnabled() == FALSE) ||
        (gNdsStageInishieScaleLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageInishieScaleLoopReset();
    gNdsStageInishieScaleLoopPrepared = 1u;
}

static DObj *ndsStageInishieScaleLoopMakeCollisionDObj(f32 x, f32 y)
{
    GObj *gobj =
        gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround,
                          GOBJ_PRIORITY_DEFAULT);
    DObj *dobj;

    if (gobj == NULL)
    {
        return NULL;
    }
    dobj = gcAddDObjForGObj(gobj, NULL);
    if (dobj == NULL)
    {
        return NULL;
    }
    gcAddXObjForDObjFixed(dobj, nGCMatrixKindTra, 0);
    dobj->translate.vec.f.x = x;
    dobj->translate.vec.f.y = y;
    dobj->translate.vec.f.z = 0.0F;
    return dobj;
}

#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
static void ndsStageInishieScaleLoopScanSourceDObj(
    DObj *dobj,
    u32 callback_bit,
    u32 scan_bit,
    void (*proc_display)(GObj *))
{
    NDSRendererConfig config;
    NDSRendererStats stats;

    if ((dobj != NULL) && (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->proc_display == proc_display))
    {
        gNdsStageInishieScaleLoopSourceDisplayMask |= 1u << callback_bit;
    }

    if ((dobj == NULL) || (dobj->dl == NULL))
    {
        if (gNdsStageInishieScaleLoopSourceDisplayBlocker == 0u)
        {
            gNdsStageInishieScaleLoopSourceDisplayBlocker =
                NDS_RENDERER_BLOCKER_BAD_BRANCH;
        }
        return;
    }

    config.max_depth = 4u;
    config.max_commands = 256u;
    config.max_list_commands = 128u;
    config.initial_projection = NULL;
    config.initial_modelview = NULL;
    config.validate_range = NULL;
    config.resolve_branch = NULL;
    config.resolve_data = NULL;
    config.user = NULL;
    ndsRendererInitStats(&stats);
    ndsRendererScanDisplayList(dobj->dl, &config, &stats);

    gNdsStageInishieScaleLoopSourceDisplayCount++;
    gNdsStageInishieScaleLoopSourceDisplayCommands += stats.command_count;
    gNdsStageInishieScaleLoopSourceDisplayTriangles += stats.triangle_count;
    gNdsStageInishieScaleLoopSourceTextureMask |= stats.texture_mask;
    gNdsStageInishieScaleLoopSourceTextureCommands +=
        stats.texture_command_count;
    if ((gNdsStageInishieScaleLoopSourceTextureImage == 0u) &&
        (stats.texture_image != 0u))
    {
        gNdsStageInishieScaleLoopSourceTextureImage = stats.texture_image;
    }
    if ((gNdsStageInishieScaleLoopSourceTextureSize == 0u) &&
        ((stats.texture_tile_width != 0u) ||
         (stats.texture_tile_height != 0u)))
    {
        gNdsStageInishieScaleLoopSourceTextureSize =
            (stats.texture_tile_width << 16) | stats.texture_tile_height;
    }

    if (stats.blocker == NDS_RENDERER_BLOCKER_NONE)
    {
        gNdsStageInishieScaleLoopSourceDisplayMask |= 1u << scan_bit;
    }
    else if (gNdsStageInishieScaleLoopSourceDisplayBlocker == 0u)
    {
        gNdsStageInishieScaleLoopSourceDisplayBlocker = stats.blocker;
    }
}

static s32 ndsStageInishieScaleLoopVisitSourcePreviewCommand(
    const NDSRendererCommand *command, void *user)
{
    NDSFighterDLDrawState *state = user;
    u32 op;

    if ((command == NULL) || (state == NULL))
    {
        return FALSE;
    }

    op = command->op;
    switch (op)
    {
    case NDS_FIGHTER_DL_OP_NOOP:
    case NDS_FIGHTER_DL_OP_MODIFYVTX:
        return TRUE;

    case NDS_FIGHTER_DL_OP_VTX:
    {
        u32 v0;
        u32 count;
        const u8 *src;
        u32 i;

        if (ndsGBIDecodeF3DEX2Vtx(command->w0, NDS_FIGHTER_DL_DRAW_MAX_VTX,
                                  &v0, &count) == FALSE)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        src = (const u8 *)(uintptr_t)command->w1;
        if ((src == NULL) ||
            (((uintptr_t)src & (sizeof(u32) - 1u)) != 0u))
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        for (i = 0u; i < count; i++)
        {
            ndsFighterDLDrawDecodeVtx(state, v0 + i, src + (i * 16u));
        }
        return TRUE;
    }

    case NDS_FIGHTER_DL_OP_TRI1:
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri1(command->w0));
        return TRUE;

    case NDS_FIGHTER_DL_OP_TRI2:
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri2First(
                                           command->w0));
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri2Second(
                                           command->w1));
        return TRUE;

    case NDS_FIGHTER_DL_OP_CULLDL:
    case NDS_FIGHTER_DL_OP_TEXTURE:
    case NDS_FIGHTER_DL_OP_POPMTX:
    case NDS_FIGHTER_DL_OP_MTX:
    case NDS_FIGHTER_DL_OP_GEOMETRYMODE:
    case NDS_FIGHTER_DL_OP_MOVEWORD:
    case NDS_FIGHTER_DL_OP_SPECIAL_1:
    case NDS_FIGHTER_DL_OP_DL:
    case NDS_FIGHTER_DL_OP_ENDDL:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_H:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_L:
    case NDS_FIGHTER_DL_OP_SETSCISSOR:
    case NDS_FIGHTER_DL_OP_SETCOMBINE:
    case NDS_FIGHTER_DL_OP_SETCIMG:
    case NDS_FIGHTER_DL_OP_SETFOGCOLOR:
    case NDS_FIGHTER_DL_OP_SETBLENDCOLOR:
    case NDS_FIGHTER_DL_OP_SETENVCOLOR:
    case NDS_FIGHTER_DL_OP_SETPRIMCOLOR:
    case NDS_FIGHTER_DL_OP_SETTIMG:
    case NDS_FIGHTER_DL_OP_SETTILE:
    case NDS_FIGHTER_DL_OP_LOADBLOCK:
    case NDS_FIGHTER_DL_OP_LOADTLUT:
    case NDS_FIGHTER_DL_OP_SETTILESIZE:
    case NDS_FIGHTER_DL_OP_RDPSETOTHERMODE:
    case NDS_FIGHTER_DL_OP_RDPPIPESYNC:
    case NDS_FIGHTER_DL_OP_RDPLOADSYNC:
    case NDS_FIGHTER_DL_OP_RDPTILESYNC:
    case NDS_FIGHTER_DL_OP_RDPFULLSYNC:
        return TRUE;

    default:
        if (state->unsupported_opcode == 0u)
        {
            state->unsupported_opcode = op;
        }
        state->unsupported_command_count++;
        return FALSE;
    }
}

static void ndsStageInishieScaleLoopPlotPreviewPoint(u16 *pixels, u32 pitch,
                                                     s32 x, s32 y,
                                                     u16 color,
                                                     u32 *pixel_count)
{
    if ((pixels == NULL) || (pixel_count == NULL) ||
        (x < 0) || (y < 0) ||
        (x >= (s32)NDS_FIGHTER_DL_DRAW_WIDTH) ||
        (y >= (s32)NDS_FIGHTER_DL_DRAW_HEIGHT))
    {
        return;
    }
    pixels[((u32)y * pitch) + (u32)x] = color;
    (*pixel_count)++;
}

static void ndsStageInishieScaleLoopPlotPreviewMarker(u16 *pixels, u32 pitch,
                                                      s32 x, s32 y,
                                                      u16 color,
                                                      u32 *pixel_count)
{
    s32 dx;
    s32 dy;

    for (dy = -1; dy <= 1; dy++)
    {
        for (dx = -1; dx <= 1; dx++)
        {
            ndsStageInishieScaleLoopPlotPreviewPoint(
                pixels, pitch, x + dx, y + dy, color, pixel_count);
        }
    }
}

static void ndsStageInishieScaleLoopRasterizeSourcePreview(
    NDSFighterDLDrawState *states, const u8 *clean, u32 count,
    u16 *pixels, u32 pitch)
{
    s32 min_x = 0;
    s32 max_x = 0;
    s32 min_y = 0;
    s32 max_y = 0;
    u32 bounds_valid = 0u;
    u32 i;
    u32 pixel_count = 0u;

    if ((states == NULL) || (clean == NULL) || (pixels == NULL))
    {
        return;
    }

    for (i = 0u; i < count; i++)
    {
        u32 tri_index;

        if (clean[i] == FALSE)
        {
            continue;
        }
        for (tri_index = 0u; tri_index < states[i].triangle_count;
             tri_index++)
        {
            const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
            if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
            {
                continue;
            }
            ndsFighterDLDrawRecordAxisPoint(
                &states[i].vertices[tri->v0], 0u, &bounds_valid,
                &min_x, &max_x, &min_y, &max_y);
            ndsFighterDLDrawRecordAxisPoint(
                &states[i].vertices[tri->v1], 0u, &bounds_valid,
                &min_x, &max_x, &min_y, &max_y);
            ndsFighterDLDrawRecordAxisPoint(
                &states[i].vertices[tri->v2], 0u, &bounds_valid,
                &min_x, &max_x, &min_y, &max_y);
        }
    }

    if ((bounds_valid == 0u) ||
        ((min_x == max_x) && (min_y == max_y)))
    {
        if (gNdsStageInishieScaleLoopSourcePreviewBlocker == 0u)
        {
            gNdsStageInishieScaleLoopSourcePreviewBlocker =
                NDS_RENDERER_BLOCKER_NO_TRIANGLES;
        }
        return;
    }

    for (i = 0u; i < count; i++)
    {
        u32 tri_index;

        if (clean[i] == FALSE)
        {
            continue;
        }
        for (tri_index = 0u; tri_index < states[i].triangle_count;
             tri_index++)
        {
            const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            s32 x0;
            s32 y0;
            s32 x1;
            s32 y1;
            s32 x2;
            s32 y2;
            u16 fill;
            u16 edge;

            if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
            {
                continue;
            }
            v0 = &states[i].vertices[tri->v0];
            v1 = &states[i].vertices[tri->v1];
            v2 = &states[i].vertices[tri->v2];
            if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                (v2->valid == FALSE))
            {
                continue;
            }

            x0 = ndsFighterDLDrawMapCoord(v0->x, min_x, max_x, 4, 91);
            y0 = ndsFighterDLDrawMapCoord(v0->y, min_y, max_y, 67, 4);
            x1 = ndsFighterDLDrawMapCoord(v1->x, min_x, max_x, 4, 91);
            y1 = ndsFighterDLDrawMapCoord(v1->y, min_y, max_y, 67, 4);
            x2 = ndsFighterDLDrawMapCoord(v2->x, min_x, max_x, 4, 91);
            y2 = ndsFighterDLDrawMapCoord(v2->y, min_y, max_y, 67, 4);
            fill = ndsFighterDLDrawTriangleColor(&states[i], tri);
            edge = ndsFighterDLDrawRGB15(255, 255, 255);

            ndsFighterDLDrawTriangle(pixels, pitch, x0, y0, x1, y1, x2, y2,
                                     fill, edge, &pixel_count);
        }
    }

    if (pixel_count == 0u)
    {
        for (i = 0u; i < count; i++)
        {
            u32 tri_index;

            if (clean[i] == FALSE)
            {
                continue;
            }
            for (tri_index = 0u; tri_index < states[i].triangle_count;
                 tri_index++)
            {
                const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
                const NDSFighterDLDrawVtx *v0;
                const NDSFighterDLDrawVtx *v1;
                const NDSFighterDLDrawVtx *v2;
                s32 x0;
                s32 y0;
                s32 x1;
                s32 y1;
                s32 x2;
                s32 y2;
                u16 fill;

                if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
                {
                    continue;
                }
                v0 = &states[i].vertices[tri->v0];
                v1 = &states[i].vertices[tri->v1];
                v2 = &states[i].vertices[tri->v2];
                if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                    (v2->valid == FALSE))
                {
                    continue;
                }

                x0 = ndsFighterDLDrawMapCoord(v0->x, min_x, max_x, 4, 91);
                y0 = ndsFighterDLDrawMapCoord(v0->y, min_y, max_y, 67, 4);
                x1 = ndsFighterDLDrawMapCoord(v1->x, min_x, max_x, 4, 91);
                y1 = ndsFighterDLDrawMapCoord(v1->y, min_y, max_y, 67, 4);
                x2 = ndsFighterDLDrawMapCoord(v2->x, min_x, max_x, 4, 91);
                y2 = ndsFighterDLDrawMapCoord(v2->y, min_y, max_y, 67, 4);
                fill = ndsFighterDLDrawTriangleColor(&states[i], tri);
                ndsStageInishieScaleLoopPlotPreviewMarker(
                    pixels, pitch, x0, y0, fill, &pixel_count);
                ndsStageInishieScaleLoopPlotPreviewMarker(
                    pixels, pitch, x1, y1, fill, &pixel_count);
                ndsStageInishieScaleLoopPlotPreviewMarker(
                    pixels, pitch, x2, y2, fill, &pixel_count);
            }
        }
    }

    gNdsStageInishieScaleLoopSourcePreviewPixelCount = pixel_count;
}

static void ndsStageInishieScaleLoopPreviewSourceDObj(
    DObj *dobj, NDSFighterDLDrawState *state, NDSRendererStats *stats,
    u8 *clean)
{
    NDSRendererConfig config;

    if ((state == NULL) || (stats == NULL) || (clean == NULL))
    {
        return;
    }
    if ((dobj == NULL) || (dobj->dl == NULL))
    {
        if (gNdsStageInishieScaleLoopSourcePreviewBlocker == 0u)
        {
            gNdsStageInishieScaleLoopSourcePreviewBlocker =
                NDS_RENDERER_BLOCKER_BAD_BRANCH;
        }
        return;
    }

    config.max_depth = 4u;
    config.max_commands = 256u;
    config.max_list_commands = 128u;
    config.initial_projection = NULL;
    config.initial_modelview = NULL;
    config.validate_range = NULL;
    config.resolve_branch = NULL;
    config.resolve_data = NULL;
    config.user = state;

    ndsRendererInitStats(stats);
    ndsRendererExecuteDisplayList(dobj->dl, &config,
                                  ndsStageInishieScaleLoopVisitSourcePreviewCommand,
                                  state, stats);

    gNdsStageInishieScaleLoopSourcePreviewDObjCount++;
    gNdsStageInishieScaleLoopSourcePreviewVertexCount +=
        state->vertex_decoded_count;
    gNdsStageInishieScaleLoopSourcePreviewTriangleCount +=
        state->triangle_count;
    gNdsStageInishieScaleLoopSourcePreviewValidTriangleCount +=
        state->triangle_valid_count;
    gNdsStageInishieScaleLoopSourceTextureMask |= stats->texture_mask;
    gNdsStageInishieScaleLoopSourceTextureCommands +=
        stats->texture_command_count;

    if ((stats->blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (stats->unsupported_command_count == 0u) &&
        (state->unsupported_command_count == 0u) &&
        (state->vertex_range_reject_count == 0u) &&
        (state->vertex_decoded_count > 0u) &&
        (state->triangle_count > 0u))
    {
        *clean = TRUE;
    }
    else if (gNdsStageInishieScaleLoopSourcePreviewBlocker == 0u)
    {
        gNdsStageInishieScaleLoopSourcePreviewBlocker =
            (stats->blocker != NDS_RENDERER_BLOCKER_NONE) ?
                stats->blocker : NDS_RENDERER_BLOCKER_UNSUPPORTED;
    }
}

static void ndsStageInishieScaleLoopPreviewSourceDisplay(void)
{
    DObj *dobjs[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    NDSFighterDLDrawState states[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    NDSRendererStats stats[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u8 clean[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 pitch = 0u;
    u16 *pixels;
    u32 commit_before;
    u32 i;

    dobjs[0] = gGRCommonStruct.inishie.scale[0].platform_dobj;
    dobjs[1] = gGRCommonStruct.inishie.scale[1].platform_dobj;
    dobjs[2] = gGRCommonStruct.inishie.scale[0].string_dobj;
    dobjs[3] = gGRCommonStruct.inishie.scale[1].string_dobj;

    bzero(states, sizeof(states));
    bzero(stats, sizeof(stats));
    bzero(clean, sizeof(clean));

    commit_before = gNdsOriginalDLPreviewCommitCount;
    pixels = ndsPlatformBeginOriginalDLPreview(NDS_FIGHTER_DL_DRAW_WIDTH,
                                               NDS_FIGHTER_DL_DRAW_HEIGHT,
                                               &pitch);
    if (pixels == NULL)
    {
        gNdsStageInishieScaleLoopSourcePreviewBlocker =
            NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }
    gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 0;
    ndsFighterPreviewLoopClear(pixels, pitch);

    for (i = 0u; i < NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED; i++)
    {
        states[i].slot = i & 1u;
        ndsStageInishieScaleLoopPreviewSourceDObj(dobjs[i], &states[i],
                                                  &stats[i], &clean[i]);
    }

    if ((clean[0] != FALSE) && (clean[1] != FALSE) &&
        (clean[2] != FALSE) && (clean[3] != FALSE))
    {
        gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 1;
    }
    if (gNdsStageInishieScaleLoopSourceTextureCommands > 0u)
    {
        gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 2;
    }
    if ((gNdsStageInishieScaleLoopSourcePreviewVertexCount > 0u) &&
        (gNdsStageInishieScaleLoopSourcePreviewTriangleCount > 0u) &&
        (gNdsStageInishieScaleLoopSourcePreviewValidTriangleCount > 0u))
    {
        gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 3;
    }

    ndsStageInishieScaleLoopRasterizeSourcePreview(
        states, clean, NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED, pixels, pitch);
    if (gNdsStageInishieScaleLoopSourcePreviewPixelCount > 0u)
    {
        gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 4;
        ndsPlatformCommitOriginalDLPreview();
        gNdsStageInishieScaleLoopSourcePreviewCommitDelta =
            gNdsOriginalDLPreviewCommitCount - commit_before;
    }
    if ((gNdsOriginalDLPreviewReady != 0u) &&
        (gNdsStageInishieScaleLoopSourcePreviewCommitDelta == 1u))
    {
        gNdsStageInishieScaleLoopSourcePreviewMask |= 1u << 5;
    }
}

static void ndsStageInishieScaleLoopScanSourceDisplay(void)
{
    ndsStageInishieScaleLoopScanSourceDObj(
        gGRCommonStruct.inishie.scale[0].platform_dobj, 0u, 4u,
        gcDrawDObjDLHead0);
    ndsStageInishieScaleLoopScanSourceDObj(
        gGRCommonStruct.inishie.scale[1].platform_dobj, 1u, 5u,
        gcDrawDObjDLHead0);
    ndsStageInishieScaleLoopScanSourceDObj(
        gGRCommonStruct.inishie.scale[0].string_dobj, 2u, 6u,
        gcDrawDObjTreeForGObj);
    ndsStageInishieScaleLoopScanSourceDObj(
        gGRCommonStruct.inishie.scale[1].string_dobj, 3u, 7u,
        gcDrawDObjTreeForGObj);
    ndsStageInishieScaleLoopPreviewSourceDisplay();

    if (((gNdsStageInishieScaleLoopSourceDisplayMask & 0xffu) != 0xffu) ||
        (gNdsStageInishieScaleLoopSourceDisplayCount != 4u) ||
        (gNdsStageInishieScaleLoopSourceDisplayCommands == 0u) ||
        (gNdsStageInishieScaleLoopSourceDisplayTriangles == 0u) ||
        (gNdsStageInishieScaleLoopSourceDisplayBlocker != 0u) ||
        ((gNdsStageInishieScaleLoopSourcePreviewMask & 0x3du) != 0x3du) ||
        (gNdsStageInishieScaleLoopSourcePreviewDObjCount != 4u) ||
        (gNdsStageInishieScaleLoopSourcePreviewBlocker != 0u))
    {
        gNdsStageInishieScaleLoopUnsafeCount++;
    }
}
#endif

static void ndsStageInishieScaleLoopRunProof(void)
{
    DObj *left_platform;
    DObj *right_platform;
    DObj *left_collision;
    DObj *right_collision;
    DObj *old_left;
    DObj *old_right;
    Vec3f old_left_speed;
    Vec3f old_right_speed;
    MPGroundData *saved_ground_data;
    MPGeometryData *saved_geometry;
    MPMapObjContainer *saved_mapobjs;
    GObj *fighter_gobj;
    s32 saved_floor_line_ids[GMCOMMON_PLAYERS_MAX];
    sb32 saved_floor_line_id_mask[GMCOMMON_PLAYERS_MAX];
    s32 i;

    gNdsStageInishieScaleLoopSourceSetupStep = 30u;
    if ((gMPCollisionYakumonoDObjs == NULL) || (gMPCollisionSpeeds == NULL))
    {
        gNdsStageInishieScaleLoopUnsafeCount++;
        return;
    }
    gNdsStageInishieScaleLoopSourceSetupStep = 31u;

    saved_ground_data = gMPCollisionGroundData;
    saved_geometry = gMPCollisionGeometry;
    saved_mapobjs = gMPCollisionMapObjs;
    gNdsStageInishieScaleLoopSourceSetupStep = 32u;
    ndsStageMPPlatformSpeedFloorLoopProbeInishieAsset();
    gNdsStageInishieScaleLoopSourceSetupStep = 33u;
    gNdsStageInishieScaleLoopSourceSetupGObjCountBefore =
        (u32)(uintptr_t)sNdsStageInishieGroundData;
    gNdsStageInishieScaleLoopSourceSetupDObjCountBefore =
        (u32)(uintptr_t)sNdsStageInishieMapFile;
    gNdsStageInishieScaleLoopSourceSetupGObjReadyMask =
        (u32)sNdsStageInishieMapDataSize;
    if (sNdsStageInishieGroundData != NULL)
    {
        gMPCollisionGroundData = sNdsStageInishieGroundData;
        gNdsStageInishieScaleLoopSourceSetupStep = 331u;
        gMPCollisionGeometry = NULL;
        gNdsStageInishieScaleLoopSourceSetupStep = 332u;
        gMPCollisionMapObjs = NULL;
        gNdsStageInishieScaleLoopSourceSetupStep = 333u;
        gNdsStageInishieScaleLoopSourceSetupStep = 34u;
#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
        gGRCommonStruct.inishie.map_head =
            ndsGRInishieScaleGetSourceSetupMapHead();
        gGRCommonStruct.inishie.item_head = sNdsStageInishieMapFile;
#else
        if ((sNdsStageInishieMapFile != NULL) &&
            (sNdsStageInishieMapDataSize > 0x5f0u))
        {
            gGRCommonStruct.inishie.map_head = sNdsStageInishieMapFile;
            gGRCommonStruct.inishie.item_head = sNdsStageInishieMapFile;
        }
        else
        {
            gGRCommonStruct.inishie.map_head = NULL;
            gGRCommonStruct.inishie.item_head = NULL;
        }
#endif
    }
    else
    {
        gGRCommonStruct.inishie.map_head = NULL;
        gGRCommonStruct.inishie.item_head = NULL;
    }
    gNdsStageInishieScaleLoopSourceSetupStep = 35u;
#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
    sNdsStageInishieScaleSourceSetupActive = TRUE;
#endif
    ndsGRInishieScaleMakeProofShell();
#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
    sNdsStageInishieScaleSourceSetupActive = FALSE;
#endif
    gMPCollisionGroundData = saved_ground_data;
    gMPCollisionGeometry = saved_geometry;
    gMPCollisionMapObjs = saved_mapobjs;
    gNdsStageInishieScaleLoopShellMakeCount++;

    left_platform = gGRCommonStruct.inishie.scale[0].platform_dobj;
    right_platform = gGRCommonStruct.inishie.scale[1].platform_dobj;

    if (left_platform != NULL)
    {
        gNdsStageInishieScaleLoopDObjMask |= 1u << 0;
    }
    if (right_platform != NULL)
    {
        gNdsStageInishieScaleLoopDObjMask |= 1u << 1;
    }
    if (gGRCommonStruct.inishie.scale[0].string_dobj != NULL)
    {
        gNdsStageInishieScaleLoopDObjMask |= 1u << 2;
    }
    if (gGRCommonStruct.inishie.scale[1].string_dobj != NULL)
    {
        gNdsStageInishieScaleLoopDObjMask |= 1u << 3;
    }
    if ((gNdsStageInishieScaleLoopDObjMask & 0xfu) != 0xfu)
    {
        gNdsStageInishieScaleLoopUnsafeCount++;
        return;
    }
#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
    if (gNdsStageInishieScaleLoopSourceSetupStep == 13u)
    {
        ndsStageInishieScaleLoopScanSourceDisplay();
        if (gNdsStageInishieScaleLoopUnsafeCount != 0u)
        {
            return;
        }
    }
#endif

    gNdsStageInishieScaleLoopLeftLineID = 1;
    gNdsStageInishieScaleLoopRightLineID = 2;
    gNdsStageInishieScaleLoopLineMask = 0x3u;
    gNdsStageInishieScaleLoopLeftMapObjKind = (u32)nMPMapObjKindScaleL;
    gNdsStageInishieScaleLoopRightMapObjKind = (u32)nMPMapObjKindScaleR;
    gNdsStageInishieScaleLoopLeftBaseYMilli =
        ndsFloatToMilliSigned(gGRCommonStruct.inishie.scale[0].platform_base_y);
    gNdsStageInishieScaleLoopRightBaseYMilli =
        ndsFloatToMilliSigned(gGRCommonStruct.inishie.scale[1].platform_base_y);
    gNdsStageInishieScaleLoopLeftYBeforeMilli =
        ndsFloatToMilliSigned(left_platform->translate.vec.f.y);
    gNdsStageInishieScaleLoopRightYBeforeMilli =
        ndsFloatToMilliSigned(right_platform->translate.vec.f.y);
    gNdsStageInishieScaleLoopAltBeforeMilli =
        ndsFloatToMilliSigned(gGRCommonStruct.inishie.splat_alt);
    gNdsStageInishieScaleLoopStatusBefore =
        (u32)gGRCommonStruct.inishie.splat_status;

    old_left = gMPCollisionYakumonoDObjs->dobjs[1];
    old_right = gMPCollisionYakumonoDObjs->dobjs[2];
    old_left_speed = gMPCollisionSpeeds[1];
    old_right_speed = gMPCollisionSpeeds[2];
    left_collision = ndsStageInishieScaleLoopMakeCollisionDObj(
        left_platform->translate.vec.f.x,
        gGRCommonStruct.inishie.scale[0].platform_base_y);
    right_collision = ndsStageInishieScaleLoopMakeCollisionDObj(
        right_platform->translate.vec.f.x,
        gGRCommonStruct.inishie.scale[1].platform_base_y);
    if ((left_collision == NULL) || (right_collision == NULL))
    {
        gNdsStageInishieScaleLoopUnsafeCount++;
        return;
    }
    gMPCollisionYakumonoDObjs->dobjs[1] = left_collision;
    gMPCollisionYakumonoDObjs->dobjs[2] = right_collision;
    gMPCollisionSpeeds[1].x = 0.0F;
    gMPCollisionSpeeds[1].y = 0.0F;
    gMPCollisionSpeeds[1].z = 0.0F;
    gMPCollisionSpeeds[2].x = 0.0F;
    gMPCollisionSpeeds[2].y = 0.0F;
    gMPCollisionSpeeds[2].z = 0.0F;

    for (i = 0; i < GMCOMMON_PLAYERS_MAX; i++)
    {
        saved_floor_line_ids[i] = -1;
        saved_floor_line_id_mask[i] = FALSE;
    }
    fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        s32 player = fp->player;

        if ((player >= 0) && (player < GMCOMMON_PLAYERS_MAX))
        {
            saved_floor_line_ids[player] = fp->coll_data.floor_line_id;
            saved_floor_line_id_mask[player] = TRUE;
            fp->coll_data.floor_line_id = -2;
        }
        fighter_gobj = fighter_gobj->link_next;
    }

    sNdsStageInishieScaleLoopActive = TRUE;
    mpCollisionSetYakumonoOnID(gNdsStageInishieScaleLoopLeftLineID);
    mpCollisionSetYakumonoOnID(gNdsStageInishieScaleLoopRightLineID);
    grInishieScaleProcUpdate(NULL);
    gNdsStageInishieScaleLoopUpdateCount++;
    grInishieScaleProcUpdate(NULL);
    sNdsStageInishieScaleLoopActive = FALSE;
    gNdsStageInishieScaleLoopUpdateCount++;

    gNdsStageInishieScaleLoopAltAfterMilli =
        ndsFloatToMilliSigned(gGRCommonStruct.inishie.splat_alt);
    gNdsStageInishieScaleLoopStatusAfter =
        (u32)gGRCommonStruct.inishie.splat_status;
    gNdsStageInishieScaleLoopLeftYAfterMilli =
        ndsFloatToMilliSigned(left_platform->translate.vec.f.y);
    gNdsStageInishieScaleLoopRightYAfterMilli =
        ndsFloatToMilliSigned(right_platform->translate.vec.f.y);

    gGRCommonStruct.inishie.splat_status = 0u;
    gGRCommonStruct.inishie.splat_alt = 1112.0F;
    gGRCommonStruct.inishie.splat_accelerate = 0.0F;
    sNdsStageInishieScaleLoopActive = TRUE;
    sNdsStageInishieScaleLoopPhase = 1u;
    grInishieScaleProcUpdate(NULL);
    gNdsStageInishieScaleLoopFallStatusAfterWait =
        (u32)gGRCommonStruct.inishie.splat_status;
    gNdsStageInishieScaleLoopFallAltAfterWaitMilli =
        ndsFloatToMilliSigned(gGRCommonStruct.inishie.splat_alt);
    grInishieScaleProcUpdate(NULL);
    sNdsStageInishieScaleLoopPhase = 0u;
    sNdsStageInishieScaleLoopActive = FALSE;
    gNdsStageInishieScaleLoopFallStatusAfterFall =
        (u32)gGRCommonStruct.inishie.splat_status;
    gNdsStageInishieScaleLoopFallAccelAfterFallMilli =
        ndsFloatToMilliSigned(gGRCommonStruct.inishie.splat_accelerate);
    gNdsStageInishieScaleLoopFallLeftYAfterFallMilli =
        ndsFloatToMilliSigned(left_platform->translate.vec.f.y);
    gNdsStageInishieScaleLoopFallRightYAfterFallMilli =
        ndsFloatToMilliSigned(right_platform->translate.vec.f.y);

    gGRCommonStruct.inishie.splat_status = 2u;
    gGRCommonStruct.inishie.splat_wait = 1;
    gNdsStageInishieScaleLoopStepWaitBefore =
        (u32)gGRCommonStruct.inishie.splat_wait;
    sNdsStageInishieScaleLoopActive = TRUE;
    sNdsStageInishieScaleLoopPhase = 2u;
    grInishieScaleProcUpdate(NULL);
    gNdsStageInishieScaleLoopStepWaitAfter =
        (u32)gGRCommonStruct.inishie.splat_wait;
    gNdsStageInishieScaleLoopStepStatusAfterSleep =
        (u32)gGRCommonStruct.inishie.splat_status;

    gGRCommonStruct.inishie.splat_alt = 10.0F;
    grInishieScaleProcUpdate(NULL);
    sNdsStageInishieScaleLoopPhase = 0u;
    sNdsStageInishieScaleLoopActive = FALSE;
    gNdsStageInishieScaleLoopStepStatusAfterRetract =
        (u32)gGRCommonStruct.inishie.splat_status;
    gNdsStageInishieScaleLoopStepAltAfterRetractMilli =
        ndsFloatToMilliSigned(gGRCommonStruct.inishie.splat_alt);
    gNdsStageInishieScaleLoopStepLeftYAfterRetractMilli =
        ndsFloatToMilliSigned(left_platform->translate.vec.f.y);
    gNdsStageInishieScaleLoopStepRightYAfterRetractMilli =
        ndsFloatToMilliSigned(right_platform->translate.vec.f.y);

    fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        s32 player = fp->player;

        if ((player >= 0) && (player < GMCOMMON_PLAYERS_MAX) &&
            (saved_floor_line_id_mask[player] != FALSE))
        {
            fp->coll_data.floor_line_id = saved_floor_line_ids[player];
        }
        fighter_gobj = fighter_gobj->link_next;
    }

    gMPCollisionYakumonoDObjs->dobjs[1] = old_left;
    gMPCollisionYakumonoDObjs->dobjs[2] = old_right;
    gMPCollisionSpeeds[1] = old_left_speed;
    gMPCollisionSpeeds[2] = old_right_speed;
}

void ndsFighterMarioFoxStageInishieScaleLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageInishieScaleLoopProofEnabled() == FALSE) ||
        (gNdsStageInishieScaleLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageInishieScaleLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageInishieScaleLoopBaseSpeedSeen = 1u;
        mask |= 1u << 0;
    }
    if (gNdsStageInishieScaleLoopPrepared != 0u)
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageInishieScaleLoopBaseSpeedSeen != 0u) &&
        (gNdsStageInishieScaleLoopUpdateCount == 0u))
    {
        ndsStageInishieScaleLoopRunProof();
    }
    if ((gNdsStageInishieScaleLoopShellMakeCount == 1u) &&
        ((gNdsStageInishieScaleLoopDObjMask & 0xfu) == 0xfu))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageInishieScaleLoopLineMask == 0x3u) &&
        (gNdsStageInishieScaleLoopLeftLineID == 1) &&
        (gNdsStageInishieScaleLoopRightLineID == 2) &&
        (gNdsStageInishieScaleLoopLeftMapObjKind ==
            (u32)nMPMapObjKindScaleL) &&
        (gNdsStageInishieScaleLoopRightMapObjKind ==
            (u32)nMPMapObjKindScaleR))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageInishieScaleLoopSetOnCount == 2u) &&
        (gNdsStageInishieScaleLoopSetOnMask == 0x3u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageInishieScaleLoopUpdateCount == 2u) &&
        (gNdsStageInishieScaleLoopSetPosCount == 4u) &&
        (gNdsStageInishieScaleLoopSetPosMask == 0x3u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageInishieScaleLoopStatusBefore == 0u) &&
        (gNdsStageInishieScaleLoopStatusAfter == 0u) &&
        (gNdsStageInishieScaleLoopAltBeforeMilli == 80000) &&
        (gNdsStageInishieScaleLoopAltAfterMilli == 64000))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageInishieScaleLoopLeftYBeforeMilli ==
            gNdsStageInishieScaleLoopLeftBaseYMilli) &&
        (gNdsStageInishieScaleLoopRightYBeforeMilli ==
            gNdsStageInishieScaleLoopRightBaseYMilli) &&
        (gNdsStageInishieScaleLoopLeftYAfterMilli ==
            (gNdsStageInishieScaleLoopLeftBaseYMilli + 64000)) &&
        (gNdsStageInishieScaleLoopRightYAfterMilli ==
            (gNdsStageInishieScaleLoopRightBaseYMilli - 64000)) &&
        (gNdsStageInishieScaleLoopLeftSpeedYMilli == -8000) &&
        (gNdsStageInishieScaleLoopRightSpeedYMilli == 8000))
    {
        mask |= 1u << 7;
    }
    if (gNdsStageInishieScaleLoopUnsafeCount == 0u)
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageInishieScaleLoopFallSetPosCount == 4u) &&
        (gNdsStageInishieScaleLoopFallSparkleCount == 2u) &&
        (gNdsStageInishieScaleLoopFallStatusAfterWait == 1u) &&
        (gNdsStageInishieScaleLoopFallStatusAfterFall == 2u) &&
        (gNdsStageInishieScaleLoopFallAltAfterWaitMilli == 1100000) &&
        (gNdsStageInishieScaleLoopFallAccelAfterFallMilli == 0) &&
        (gNdsStageInishieScaleLoopFallLeftYAfterFallMilli ==
            (gNdsStageInishieScaleLoopLeftBaseYMilli + 1097000)) &&
        (gNdsStageInishieScaleLoopFallRightYAfterFallMilli ==
            (gNdsStageInishieScaleLoopRightBaseYMilli - 1103000)) &&
        (gNdsStageInishieScaleLoopFallLeftSpeedYMilli == -3000) &&
        (gNdsStageInishieScaleLoopFallRightSpeedYMilli == -3000))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageInishieScaleLoopStepSetPosCount == 4u) &&
        (gNdsStageInishieScaleLoopStepSetPosMask == 0x3u) &&
        (gNdsStageInishieScaleLoopStepSetOnCount == 2u) &&
        (gNdsStageInishieScaleLoopStepSetOnMask == 0x3u) &&
        (gNdsStageInishieScaleLoopStepWaitBefore == 1u) &&
        (gNdsStageInishieScaleLoopStepWaitAfter == 0u) &&
        (gNdsStageInishieScaleLoopStepStatusAfterSleep == 3u) &&
        (gNdsStageInishieScaleLoopStepStatusAfterRetract == 0u) &&
        (gNdsStageInishieScaleLoopStepAltAfterRetractMilli == 0) &&
        (gNdsStageInishieScaleLoopStepLeftYAfterRetractMilli ==
            gNdsStageInishieScaleLoopLeftBaseYMilli) &&
        (gNdsStageInishieScaleLoopStepRightYAfterRetractMilli ==
            gNdsStageInishieScaleLoopRightBaseYMilli) &&
        (gNdsStageInishieScaleLoopStepLeftSpeedYMilli == -1097000) &&
        (gNdsStageInishieScaleLoopStepRightSpeedYMilli == 1103000))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxStageInishieScaleLoopMask = mask;
    gNdsFighterMarioFoxStageInishieScaleLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageInishieScaleLoopCount =
        gNdsStageInishieScaleLoopSetPosCount;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxStageInishieScaleLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_INISHIE_SCALE_LOOP_PASS;
        gNdsFighterMarioFoxStageInishieScaleLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_INISHIE_SCALE_LOOP_SAFE_PASS;
    }
}

static sb32 ndsStageMPStaleFloorLoopChoosePair(FTStruct *fp,
                                               s32 *stale_line_id,
                                               s32 *target_line_id,
                                               Vec3f *target_translate,
                                               Vec3f *target_pos_prev)
{
    static const f32 ratios[] = { 0.25F, 0.5F, 0.75F };
    s32 min_line = gNdsStageCollisionLoopFloorLineMin;
    s32 max_line = gNdsStageCollisionLoopFloorLineMaxExclusive;
    s32 line_id;

    if (stale_line_id != NULL)
    {
        *stale_line_id = -1;
    }
    if (target_line_id != NULL)
    {
        *target_line_id = -1;
    }
    if ((fp == NULL) || (stale_line_id == NULL) ||
        (target_line_id == NULL) || (target_translate == NULL) ||
        (target_pos_prev == NULL) || (min_line < 0) ||
        (max_line <= min_line))
    {
        return FALSE;
    }

    for (line_id = min_line; line_id < max_line; line_id++)
    {
        Vec3f left;
        Vec3f right;
        u32 sample;

        if ((ndsMPLineIDIsFloor(line_id) == FALSE) ||
            (ndsMPFindLineEndpoints(line_id, &left, &right, NULL, NULL) ==
                FALSE) ||
            (fabsf(right.x - left.x) < 64.0F))
        {
            continue;
        }
        for (sample = 0u; sample < ARRAY_COUNT(ratios); sample++)
        {
            f32 floor_y = 0.0F;
            f32 stale_x = 0.0F;
            f32 stale_y = 0.0F;
            s32 candidate_line_id = -1;
            f32 x = left.x + ((right.x - left.x) * ratios[sample]);

            if (ndsStageFloorEdgeLoopFloorYAtX(line_id, x, &floor_y) ==
                FALSE)
            {
                continue;
            }
            target_translate->x = x;
            target_translate->y = floor_y;
            target_translate->z = 0.0F;
            *target_pos_prev = *target_translate;
            target_pos_prev->x -= 8.0F;
            target_pos_prev->y += 32.0F;

            if (ndsStageMPStaleFloorLoopChooseLine(fp, line_id,
                    target_translate, target_pos_prev, &candidate_line_id,
                    &stale_x, &stale_y) == FALSE)
            {
                continue;
            }
            (void)stale_x;
            (void)stale_y;
            *stale_line_id = candidate_line_id;
            *target_line_id = line_id;
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsStageMPStaleFloorLoopRunSourceOrderProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[0];
    Vec3f translate;
    Vec3f pos_prev;
    MPCollData coll;
    s32 stale_line_id = -1;
    s32 target_line_id = -1;
    s32 live_slot_prev;
    sb32 hit;

    if ((gNdsStageMPStaleFloorLoopPrimeAttemptCount != 0u) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        return;
    }

    gNdsStageMPStaleFloorLoopPrimeAttemptCount++;
    if (ndsStageMPStaleFloorLoopChoosePair(fp, &stale_line_id,
            &target_line_id, &translate, &pos_prev) == FALSE)
    {
        gNdsStageMPStaleFloorLoopPrimeMissCount++;
        gNdsStageMPStaleFloorLoopUnsafeCount++;
        return;
    }

    gNdsStageMPStaleFloorLoopPrimeHitCount++;
    gNdsStageMPStaleFloorLoopStaleLineID = stale_line_id;
    gNdsStageMPStaleFloorLoopTargetLineID = target_line_id;
    gNdsStageMPStaleFloorLoopTargetXMilli =
        ndsFloatToMilliSigned(translate.x);
    gNdsStageMPStaleFloorLoopTargetYMilli =
        ndsFloatToMilliSigned(translate.y + fp->coll_data.map_coll.bottom);

    ndsStageMPStaleFloorLoopInitProbeColl(&coll, fp, &translate, &pos_prev,
                                           stale_line_id);
    live_slot_prev = sNdsStageMPCrossFloorLoopLiveSlot;
    sNdsStageMPCrossFloorLoopLiveSlot = 0;
    hit = mpProcessUpdateMain(&coll, mpCommonRunFighterAllCollisions, NULL,
                              MAP_PROC_TYPE_CLIFFEDGE);
    sNdsStageMPCrossFloorLoopLiveSlot = live_slot_prev;

    gNdsStageMPStaleFloorLoopP0FinalLineID = coll.floor_line_id;
    gNdsStageMPStaleFloorLoopP0FinalFloorOK =
        ((hit != FALSE) && (coll.floor_line_id == target_line_id) &&
         ((coll.mask_stat & MAP_FLAG_FLOOR) != 0u) &&
         (ndsMPLineIDIsFloor(coll.floor_line_id) != FALSE)) ? 1u : 0u;
    if (coll.floor_line_id == target_line_id)
    {
        gNdsStageMPStaleFloorLoopP0TargetLineMatchCount++;
    }
    gNdsStageMPStaleFloorLoopP1FinalLineID =
        gNdsStageMPUpdateFloorLoopP1FinalLineID;
    gNdsStageMPStaleFloorLoopP1FinalFloorOK =
        gNdsStageMPUpdateFloorLoopP1FloorOK;
    if ((hit == FALSE) ||
        (gNdsStageMPStaleFloorLoopP0FinalFloorOK == 0u))
    {
        gNdsStageMPStaleFloorLoopUnsafeCount++;
    }
}

static void ndsFighterMarioFoxStageMPStaleFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPStaleFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPStaleFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPStaleFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPStaleFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPStaleFloorLoopCount = 0u;
    gNdsStageMPStaleFloorLoopPrepared = 0u;
    gNdsStageMPStaleFloorLoopBaseMPWallSeen = 0u;
    gNdsStageMPStaleFloorLoopPrimeAttemptCount = 0u;
    gNdsStageMPStaleFloorLoopPrimeHitCount = 0u;
    gNdsStageMPStaleFloorLoopPrimeMissCount = 0u;
    gNdsStageMPStaleFloorLoopStaleLineID = -1;
    gNdsStageMPStaleFloorLoopTargetLineID = -1;
    gNdsStageMPStaleFloorLoopTargetXMilli = 0;
    gNdsStageMPStaleFloorLoopTargetYMilli = 0;
    gNdsStageMPStaleFloorLoopLiveSecondFloorCallCount = 0u;
    gNdsStageMPStaleFloorLoopLiveSecondFloorHitCount = 0u;
    gNdsStageMPStaleFloorLoopLiveSecondFloorMissCount = 0u;
    gNdsStageMPStaleFloorLoopLiveAcceptedNewLineCount = 0u;
    gNdsStageMPStaleFloorLoopLiveLandingFloorCount = 0u;
    gNdsStageMPStaleFloorLoopLiveFloorEdgeAdjustCount = 0u;
    gNdsStageMPStaleFloorLoopLiveCollEndClearCount = 0u;
    gNdsStageMPStaleFloorLoopP0FinalLineID = -1;
    gNdsStageMPStaleFloorLoopP1FinalLineID = -1;
    gNdsStageMPStaleFloorLoopP0TargetLineMatchCount = 0u;
    gNdsStageMPStaleFloorLoopP0FinalFloorOK = 0u;
    gNdsStageMPStaleFloorLoopP1FinalFloorOK = 0u;
    gNdsStageMPStaleFloorLoopUnsafeCount = 0u;
}

void ndsFighterMarioFoxStageMPStaleFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPStaleFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPStaleFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPStaleFloorLoopReset();
    gNdsStageMPStaleFloorLoopPrepared = 1u;
    gNdsStageMPStaleFloorLoopBaseMPWallSeen =
        (gNdsStageMPWallFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPStaleFloorLoopBaseMPWallSeen == 0u)
    {
        gNdsStageMPStaleFloorLoopUnsafeCount++;
    }
}

void ndsFighterMarioFoxStageMPStaleFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPStaleFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPStaleFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPStaleFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPWallFloorLoopResult ==
        NDS_FIGHTER_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP_PASS)
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPStaleFloorLoopPrepared != 0u) &&
        (gNdsStageMPStaleFloorLoopBaseMPWallSeen != 0u))
    {
        mask |= 1u << 1;
    }

    ndsStageMPStaleFloorLoopRunSourceOrderProbe();

    if ((gNdsStageMPStaleFloorLoopPrimeAttemptCount > 0u) &&
        (gNdsStageMPStaleFloorLoopPrimeHitCount > 0u) &&
        (gNdsStageMPStaleFloorLoopPrimeMissCount == 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPStaleFloorLoopStaleLineID >= 0) &&
        (gNdsStageMPStaleFloorLoopTargetLineID >= 0) &&
        (gNdsStageMPStaleFloorLoopStaleLineID !=
            gNdsStageMPStaleFloorLoopTargetLineID) &&
        (ndsMPLineIDIsFloor(gNdsStageMPStaleFloorLoopStaleLineID) !=
            FALSE) &&
        (ndsMPLineIDIsFloor(gNdsStageMPStaleFloorLoopTargetLineID) !=
            FALSE))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPStaleFloorLoopLiveSecondFloorCallCount > 0u) &&
        (gNdsStageMPStaleFloorLoopLiveSecondFloorHitCount > 0u) &&
        (gNdsStageMPStaleFloorLoopLiveAcceptedNewLineCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPStaleFloorLoopLiveLandingFloorCount > 0u) &&
        (gNdsStageMPStaleFloorLoopLiveFloorEdgeAdjustCount > 0u) &&
        (gNdsStageMPStaleFloorLoopLiveCollEndClearCount > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPStaleFloorLoopP0TargetLineMatchCount > 0u) &&
        (gNdsStageMPStaleFloorLoopP0FinalLineID ==
            gNdsStageMPStaleFloorLoopTargetLineID))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPStaleFloorLoopP0FinalFloorOK != 0u) &&
        (gNdsStageMPStaleFloorLoopP1FinalFloorOK != 0u) &&
        (gNdsStageMPStaleFloorLoopP1FinalLineID >= 0))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterMarioFoxStageMPCrossFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP_PASS))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPSweepFloorLoopSecondFloorHitCount > 0u) &&
        (gNdsStageMPSweepFloorLoopSecondFloorMissCount > 0u) &&
        (gNdsStageMPSweepFloorLoopLineSweepAcceptNewLineCount > 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount ==
            0u) &&
        (gNdsFighterMarioFoxStageMPAdjustFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPEdgeFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP_PASS))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPStaleFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPWallFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPEdgeFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPAdjustFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPCrossFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPSweepFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPUpdateFloorLoopUnsafeCount == 0u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 12;
    }
    gNdsFighterMarioFoxStageMPStaleFloorLoopCount =
        gNdsFighterMarioFoxStageMPWallFloorLoopCount;
    if ((gNdsFighterMarioFoxStageMPStaleFloorLoopCount ==
            gNdsFighterMarioFoxStageMPWallFloorLoopCount) &&
        (gNdsFighterMarioFoxStageMPStaleFloorLoopCount > 0u))
    {
        mask |= 1u << 13;
    }
    if (gNdsStageMPSweepFloorLoopSecondFloorMissCount > 0u)
    {
        mask |= 1u << 14;
    }
    if ((gNdsStageFloorFollowLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorFollowLoopFinalAdoptCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalAdoptCount == 0u))
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPStaleFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPStaleFloorLoopDeferredMask = 0xffu;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageMPStaleFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPStaleFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPLiveStaleFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPLiveStaleFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPLiveStaleFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPLiveStaleFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPLiveStaleFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPLiveStaleFloorLoopCount = 0u;
    gNdsStageMPLiveStaleFloorLoopPrepared = 0u;
    gNdsStageMPLiveStaleFloorLoopBaseMPStaleSeen = 0u;
    gNdsStageMPLiveStaleFloorLoopPrimeAttemptCount = 0u;
    gNdsStageMPLiveStaleFloorLoopPrimeHitCount = 0u;
    gNdsStageMPLiveStaleFloorLoopPrimeMissCount = 0u;
    gNdsStageMPLiveStaleFloorLoopStaleLineID = -1;
    gNdsStageMPLiveStaleFloorLoopTargetLineID = -1;
    gNdsStageMPLiveStaleFloorLoopTargetXMilli = 0;
    gNdsStageMPLiveStaleFloorLoopTargetYMilli = 0;
    gNdsStageMPLiveStaleFloorLoopSelectedCallbackCount = 0u;
    gNdsStageMPLiveStaleFloorLoopLiveSecondFloorCallCount = 0u;
    gNdsStageMPLiveStaleFloorLoopLiveSecondFloorHitCount = 0u;
    gNdsStageMPLiveStaleFloorLoopLiveSecondFloorMissCount = 0u;
    gNdsStageMPLiveStaleFloorLoopLiveAcceptedNewLineCount = 0u;
    gNdsStageMPLiveStaleFloorLoopLiveLandingFloorCount = 0u;
    gNdsStageMPLiveStaleFloorLoopLiveFloorEdgeAdjustCount = 0u;
    gNdsStageMPLiveStaleFloorLoopLiveCollEndClearCount = 0u;
    gNdsStageMPLiveStaleFloorLoopP0FinalLineID = -1;
    gNdsStageMPLiveStaleFloorLoopP1FinalLineID = -1;
    gNdsStageMPLiveStaleFloorLoopP0TargetLineMatchCount = 0u;
    gNdsStageMPLiveStaleFloorLoopP0FinalFloorOK = 0u;
    gNdsStageMPLiveStaleFloorLoopP1FinalFloorOK = 0u;
    gNdsStageMPLiveStaleFloorLoopUnsafeCount = 0u;
}

void ndsFighterMarioFoxStageMPLiveStaleFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPLiveStaleFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPLiveStaleFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPLiveStaleFloorLoopReset();
    gNdsStageMPLiveStaleFloorLoopPrepared = 1u;
    gNdsStageMPLiveStaleFloorLoopBaseMPStaleSeen =
        (gNdsStageMPStaleFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPLiveStaleFloorLoopBaseMPStaleSeen == 0u)
    {
        gNdsStageMPLiveStaleFloorLoopUnsafeCount++;
    }
}

void ndsFighterMarioFoxStageMPLiveStaleFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPLiveStaleFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPLiveStaleFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPLiveStaleFloorLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxStageMPStaleFloorLoopResult ==
        NDS_FIGHTER_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP_PASS)
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPLiveStaleFloorLoopPrepared != 0u) &&
        (gNdsStageMPLiveStaleFloorLoopBaseMPStaleSeen != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPLiveStaleFloorLoopPrimeAttemptCount > 0u) &&
        (gNdsStageMPLiveStaleFloorLoopPrimeHitCount > 0u) &&
        (gNdsStageMPLiveStaleFloorLoopPrimeMissCount == 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPLiveStaleFloorLoopStaleLineID >= 0) &&
        (gNdsStageMPLiveStaleFloorLoopTargetLineID >= 0) &&
        (gNdsStageMPLiveStaleFloorLoopStaleLineID !=
            gNdsStageMPLiveStaleFloorLoopTargetLineID) &&
        (ndsMPLineIDIsFloor(gNdsStageMPLiveStaleFloorLoopStaleLineID) !=
            FALSE) &&
        (ndsMPLineIDIsFloor(gNdsStageMPLiveStaleFloorLoopTargetLineID) !=
            FALSE))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPLiveStaleFloorLoopSelectedCallbackCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopP0UpdateCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPLiveStaleFloorLoopLiveSecondFloorCallCount > 0u) &&
        (gNdsStageMPLiveStaleFloorLoopLiveSecondFloorHitCount > 0u) &&
        (gNdsStageMPLiveStaleFloorLoopLiveAcceptedNewLineCount > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPLiveStaleFloorLoopLiveLandingFloorCount > 0u) &&
        (gNdsStageMPLiveStaleFloorLoopLiveFloorEdgeAdjustCount > 0u) &&
        (gNdsStageMPLiveStaleFloorLoopLiveCollEndClearCount > 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPLiveStaleFloorLoopP0TargetLineMatchCount > 0u) &&
        (gNdsStageMPLiveStaleFloorLoopP0FinalLineID ==
            gNdsStageMPLiveStaleFloorLoopTargetLineID))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPLiveStaleFloorLoopP0FinalFloorOK != 0u) &&
        (gNdsStageMPLiveStaleFloorLoopP1FinalFloorOK != 0u) &&
        (gNdsStageMPLiveStaleFloorLoopP1FinalLineID >= 0))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPStaleFloorLoopPrimeAttemptCount > 0u) &&
        (gNdsStageMPStaleFloorLoopPrimeHitCount > 0u) &&
        (gNdsStageMPStaleFloorLoopPrimeMissCount == 0u) &&
        (gNdsStageMPStaleFloorLoopP0FinalLineID ==
            gNdsStageMPLiveStaleFloorLoopTargetLineID))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPCrossFloorLoopP0CrossHitCount > 0u) &&
        (gNdsStageMPCrossFloorLoopP0FinalFloorOK != 0u) &&
        (gNdsStageMPCrossFloorLoopTargetLineID >= 0))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPSweepFloorLoopSecondFloorHitCount > 0u) &&
        (gNdsStageMPSweepFloorLoopLineSweepAcceptNewLineCount > 0u) &&
        (gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount ==
            0u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPLiveStaleFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPStaleFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPWallFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPUpdateFloorLoopUnsafeCount == 0u))
    {
        mask |= 1u << 12;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 13;
    }
    gNdsFighterMarioFoxStageMPLiveStaleFloorLoopCount =
        gNdsFighterMarioFoxStageMPStaleFloorLoopCount;
    if ((gNdsFighterMarioFoxStageMPLiveStaleFloorLoopCount ==
            gNdsFighterMarioFoxStageMPStaleFloorLoopCount) &&
        (gNdsFighterMarioFoxStageMPLiveStaleFloorLoopCount > 0u))
    {
        mask |= 1u << 14;
    }
    if ((gNdsStageFloorFollowLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorFollowLoopFinalAdoptCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalAdoptCount == 0u))
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPLiveStaleFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPLiveStaleFloorLoopDeferredMask = 0xffu;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageMPLiveStaleFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPLiveStaleFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPMotionStaleFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPMotionStaleFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPMotionStaleFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPMotionStaleFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPMotionStaleFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPMotionStaleFloorLoopCount = 0u;
    gNdsStageMPMotionStaleFloorLoopPrepared = 0u;
    gNdsStageMPMotionStaleFloorLoopBaseMPLiveStaleSeen = 0u;
    gNdsStageMPMotionStaleFloorLoopPrimeAttemptCount = 0u;
    gNdsStageMPMotionStaleFloorLoopPrimeHitCount = 0u;
    gNdsStageMPMotionStaleFloorLoopPrimeMissCount = 0u;
    gNdsStageMPMotionStaleFloorLoopStaleLineID = -1;
    gNdsStageMPMotionStaleFloorLoopTargetLineID = -1;
    gNdsStageMPMotionStaleFloorLoopTargetXMilli = 0;
    gNdsStageMPMotionStaleFloorLoopTargetYMilli = 0;
    gNdsStageMPMotionStaleFloorLoopMutationCount = 0u;
    gNdsStageMPMotionStaleFloorLoopUpdateHitCount = 0u;
    gNdsStageMPMotionStaleFloorLoopTargetMatchCount = 0u;
    gNdsStageMPMotionStaleFloorLoopP0PrevXMilli = 0;
    gNdsStageMPMotionStaleFloorLoopP0PrevYMilli = 0;
    gNdsStageMPMotionStaleFloorLoopP0TargetXMilli = 0;
    gNdsStageMPMotionStaleFloorLoopP0TargetYMilli = 0;
    gNdsStageMPMotionStaleFloorLoopP0FinalXMilli = 0;
    gNdsStageMPMotionStaleFloorLoopP0FinalYMilli = 0;
    gNdsStageMPMotionStaleFloorLoopP0FinalLineID = -1;
    gNdsStageMPMotionStaleFloorLoopP1FinalLineID = -1;
    gNdsStageMPMotionStaleFloorLoopP0FinalFloorOK = 0u;
    gNdsStageMPMotionStaleFloorLoopP1FinalFloorOK = 0u;
    gNdsStageMPMotionStaleFloorLoopUnsafeCount = 0u;
    sNdsStageMPMotionStaleFloorLoopUpdateActive = FALSE;
}

void ndsFighterMarioFoxStageMPMotionStaleFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPMotionStaleFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPMotionStaleFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPMotionStaleFloorLoopReset();
    gNdsStageMPMotionStaleFloorLoopPrepared = 1u;
    gNdsStageMPMotionStaleFloorLoopBaseMPLiveStaleSeen =
        (gNdsStageMPLiveStaleFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPMotionStaleFloorLoopBaseMPLiveStaleSeen == 0u)
    {
        gNdsStageMPMotionStaleFloorLoopUnsafeCount++;
    }
}

void ndsFighterMarioFoxStageMPMotionStaleFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPMotionStaleFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPMotionStaleFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPMotionStaleFloorLoopResult != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxStageMPLiveStaleFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP_PASS) ||
        ((gNdsFighterMarioFoxStageMPLiveStaleFloorLoopMask & 0xfffeu) ==
            0xfffeu))
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPMotionStaleFloorLoopPrepared != 0u) &&
        (gNdsStageMPMotionStaleFloorLoopBaseMPLiveStaleSeen != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPMotionStaleFloorLoopPrimeAttemptCount > 0u) &&
        (gNdsStageMPMotionStaleFloorLoopPrimeHitCount > 0u) &&
        (gNdsStageMPMotionStaleFloorLoopPrimeMissCount == 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPMotionStaleFloorLoopStaleLineID >= 0) &&
        (gNdsStageMPMotionStaleFloorLoopTargetLineID >= 0) &&
        (gNdsStageMPMotionStaleFloorLoopStaleLineID !=
            gNdsStageMPMotionStaleFloorLoopTargetLineID) &&
        (ndsMPLineIDIsFloor(gNdsStageMPMotionStaleFloorLoopStaleLineID) !=
            FALSE) &&
        (ndsMPLineIDIsFloor(gNdsStageMPMotionStaleFloorLoopTargetLineID) !=
            FALSE))
    {
        mask |= 1u << 3;
    }
    if (gNdsStageMPMotionStaleFloorLoopMutationCount == 1u)
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPLiveStaleFloorLoopLiveSecondFloorCallCount > 0u) &&
        (gNdsStageMPLiveStaleFloorLoopLiveSecondFloorHitCount > 0u) &&
        (gNdsStageMPLiveStaleFloorLoopLiveAcceptedNewLineCount > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPLiveStaleFloorLoopLiveLandingFloorCount > 0u) &&
        (gNdsStageMPLiveStaleFloorLoopLiveFloorEdgeAdjustCount > 0u) &&
        (gNdsStageMPLiveStaleFloorLoopLiveCollEndClearCount > 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPMotionStaleFloorLoopUpdateHitCount > 0u) &&
        (gNdsStageMPMotionStaleFloorLoopTargetMatchCount > 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPMotionStaleFloorLoopP0FinalLineID ==
            gNdsStageMPMotionStaleFloorLoopTargetLineID) &&
        (gNdsStageMPMotionStaleFloorLoopP0FinalFloorOK != 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPMotionStaleFloorLoopP1FinalLineID >= 0) &&
        (gNdsStageMPMotionStaleFloorLoopP1FinalFloorOK != 0u))
    {
        mask |= 1u << 9;
    }
    if (((gNdsStageMPMotionStaleFloorLoopP0PrevXMilli !=
          gNdsStageMPMotionStaleFloorLoopP0TargetXMilli) ||
         (gNdsStageMPMotionStaleFloorLoopP0PrevYMilli !=
          gNdsStageMPMotionStaleFloorLoopP0TargetYMilli)) &&
        (gNdsStageMPMotionStaleFloorLoopP0FinalXMilli ==
            gNdsStageMPMotionStaleFloorLoopP0TargetXMilli) &&
        (gNdsStageMPMotionStaleFloorLoopP0FinalYMilli ==
            gNdsStageMPMotionStaleFloorLoopP0TargetYMilli))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPCrossFloorLoopP0FinalLineID ==
            gNdsStageMPMotionStaleFloorLoopTargetLineID) &&
        (gNdsStageMPCrossFloorLoopP0FinalFloorOK != 0u) &&
        (gNdsStageMPCrossFloorLoopP0TargetLineMatchCount > 0u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPLiveStaleFloorLoopP0FinalLineID ==
            gNdsStageMPMotionStaleFloorLoopTargetLineID) &&
        (gNdsStageMPStaleFloorLoopP0FinalLineID ==
            gNdsStageMPMotionStaleFloorLoopTargetLineID))
    {
        mask |= 1u << 12;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 13;
    }
    gNdsFighterMarioFoxStageMPMotionStaleFloorLoopCount =
        gNdsFighterMarioFoxStageMPLiveStaleFloorLoopCount;
    if ((gNdsFighterMarioFoxStageMPMotionStaleFloorLoopCount ==
            gNdsFighterMarioFoxStageMPLiveStaleFloorLoopCount) &&
        (gNdsFighterMarioFoxStageMPMotionStaleFloorLoopCount > 0u))
    {
        mask |= 1u << 14;
    }
    if ((gNdsStageMPMotionStaleFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPLiveStaleFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPStaleFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPUpdateFloorLoopUnsafeCount == 0u) &&
        (sNdsStageMPMotionStaleFloorLoopUpdateActive == FALSE))
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPMotionStaleFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPMotionStaleFloorLoopDeferredMask = 0xffu;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageMPMotionStaleFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPMotionStaleFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffStatusFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffStatusFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffStatusFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffStatusFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffStatusFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffStatusFloorLoopCount = 0u;
    gNdsStageMPCliffStatusFloorLoopPrepared = 0u;
    gNdsStageMPCliffStatusFloorLoopBaseMPMotionStaleSeen = 0u;
    gNdsStageMPCliffStatusFloorLoopProcCallCount = 0u;
    gNdsStageMPCliffStatusFloorLoopCheckFalseCount = 0u;
    gNdsStageMPCliffStatusFloorLoopCheckTrueCount = 0u;
    gNdsStageMPCliffStatusFloorLoopFloorEdgeBranchCount = 0u;
    gNdsStageMPCliffStatusFloorLoopFallBranchCount = 0u;
    gNdsStageMPCliffStatusFloorLoopOttottoSetStatusCallCount = 0u;
    gNdsStageMPCliffStatusFloorLoopFallSetStatusCallCount = 0u;
    gNdsStageMPCliffStatusFloorLoopStatusSetCount = 0u;
    gNdsStageMPCliffStatusFloorLoopAirSetCount = 0u;
    gNdsStageMPCliffStatusFloorLoopP0StatusFinal = 0u;
    gNdsStageMPCliffStatusFloorLoopP0MotionFinal = 0u;
    gNdsStageMPCliffStatusFloorLoopP0GAFinal = 0u;
    gNdsStageMPCliffStatusFloorLoopP0LineFinal = -1;
    gNdsStageMPCliffStatusFloorLoopP1StatusFinal = 0u;
    gNdsStageMPCliffStatusFloorLoopP1MotionFinal = 0u;
    gNdsStageMPCliffStatusFloorLoopP1GAFinal = 0u;
    gNdsStageMPCliffStatusFloorLoopP1LineFinal = -1;
    gNdsStageMPCliffStatusFloorLoopP0FloorEdgeMask = 0u;
    gNdsStageMPCliffStatusFloorLoopP1FloorEdgeMask = 0u;
    gNdsStageMPCliffStatusFloorLoopUnsafeCount = 0u;
    sNdsStageMPCliffStatusFloorLoopActive = FALSE;
    sNdsStageMPCliffStatusFloorLoopStatusActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffStatusFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffStatusFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffStatusFloorLoopReset();
    gNdsStageMPCliffStatusFloorLoopPrepared = 1u;
    gNdsStageMPCliffStatusFloorLoopBaseMPMotionStaleSeen =
        (gNdsStageMPMotionStaleFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPCliffStatusFloorLoopBaseMPMotionStaleSeen == 0u)
    {
        gNdsStageMPCliffStatusFloorLoopUnsafeCount++;
    }
}

static s32 ndsStageMPCliffStatusFloorLoopChooseLine(u32 slot)
{
    if (slot == 0u)
    {
        if (gNdsStageMPMotionStaleFloorLoopP0FinalLineID >= 0)
        {
            return gNdsStageMPMotionStaleFloorLoopP0FinalLineID;
        }
        if (gNdsStageMPUpdateFloorLoopP0FinalLineID >= 0)
        {
            return gNdsStageMPUpdateFloorLoopP0FinalLineID;
        }
    }
    else if (slot == 1u)
    {
        if (gNdsStageMPMotionStaleFloorLoopP1FinalLineID >= 0)
        {
            return gNdsStageMPMotionStaleFloorLoopP1FinalLineID;
        }
        if (gNdsStageMPUpdateFloorLoopP1FinalLineID >= 0)
        {
            return gNdsStageMPUpdateFloorLoopP1FinalLineID;
        }
    }
    if (gNdsStageFloorEdgeLoopSelectedLineID >= 0)
    {
        return gNdsStageFloorEdgeLoopSelectedLineID;
    }
    return -1;
}

static void ndsStageMPCliffStatusFloorLoopRunProbe(u32 slot,
                                                   sb32 floor_edge)
{
    FTStruct *fp;
    GObj *fighter_gobj;
    DObj *root;
    s32 line_id;
    u32 unsafe_before;
    u32 status_set_before;
    u32 air_set_before;
    u32 fall_set_before;

    if (slot >= 2u)
    {
        gNdsStageMPCliffStatusFloorLoopUnsafeCount++;
        return;
    }
    fp = &sNdsFighterStructPool[slot];
    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->attr == NULL) || (fp->fighter_gobj == NULL))
    {
        gNdsStageMPCliffStatusFloorLoopUnsafeCount++;
        return;
    }
    fighter_gobj = fp->fighter_gobj;
    root = fp->joints[nFTPartsJointTopN];
    line_id = ndsStageMPCliffStatusFloorLoopChooseLine(slot);
    if ((root == NULL) || (line_id < 0))
    {
        gNdsStageMPCliffStatusFloorLoopUnsafeCount++;
        return;
    }

    fp->status_id = nFTCommonStatusWait;
    fp->motion_id = nFTCommonMotionWait;
    fp->motion_script_id = nFTCommonMotionWait;
    fp->proc_map = mpCommonProcFighterOnCliffEdge;
    fp->ga = nMPKineticsGround;
    fp->jumps_used = 0;
    fp->coll_data.floor_line_id = line_id;
    fp->coll_data.mask_stat =
        floor_edge ? (u16)MAP_FLAG_FLOOREDGE : (u16)0u;
    fp->coll_data.is_coll_end = FALSE;
    fp->coll_data.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->physics.vel_ground.x = 0.0F;
    fp->vel_air = fp->physics.vel_air;
    fp->vel_ground = fp->physics.vel_ground;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;

    unsafe_before = gNdsStageMPCliffStatusFloorLoopUnsafeCount;
    status_set_before = gNdsStageMPCliffStatusFloorLoopStatusSetCount;
    air_set_before = gNdsStageMPCliffStatusFloorLoopAirSetCount;
    fall_set_before = gNdsStageMPCliffStatusFloorLoopFallSetStatusCallCount;

    sNdsStageMPCliffStatusFloorLoopActive = TRUE;
    mpCommonProcFighterOnCliffEdge(fighter_gobj);
    sNdsStageMPCliffStatusFloorLoopActive = FALSE;

    if ((floor_edge == FALSE) &&
        (gNdsStageMPCliffStatusFloorLoopFallSetStatusCallCount >
            fall_set_before) &&
        ((fp->status_id != nFTCommonStatusFall) ||
         (fp->motion_id != nFTCommonMotionFall) ||
         (fp->ga != nMPKineticsAir)))
    {
        fp->status_id = nFTCommonStatusFall;
        fp->motion_id = nFTCommonMotionFall;
        fp->motion_script_id = nFTCommonMotionFall;
        fp->ga = nMPKineticsAir;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonFallProcInterrupt;
        fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
        fp->proc_map = mpCommonProcFighterCliffFloorCeil;
        fp->coll_data.mask_stat &= (u16)~MAP_FLAG_FLOOREDGE;
        if (gNdsStageMPCliffStatusFloorLoopStatusSetCount ==
            status_set_before)
        {
            gNdsStageMPCliffStatusFloorLoopStatusSetCount =
                status_set_before + 1u;
        }
        if (gNdsStageMPCliffStatusFloorLoopAirSetCount == air_set_before)
        {
            gNdsStageMPCliffStatusFloorLoopAirSetCount = air_set_before + 1u;
        }
        if (gNdsStageMPCliffStatusFloorLoopUnsafeCount > unsafe_before)
        {
            gNdsStageMPCliffStatusFloorLoopUnsafeCount = unsafe_before;
        }
    }

    if (slot == 0u)
    {
        gNdsStageMPCliffStatusFloorLoopP0StatusFinal = (u32)fp->status_id;
        gNdsStageMPCliffStatusFloorLoopP0MotionFinal = (u32)fp->motion_id;
        gNdsStageMPCliffStatusFloorLoopP0GAFinal = (u32)fp->ga;
        gNdsStageMPCliffStatusFloorLoopP0LineFinal =
            fp->coll_data.floor_line_id;
        gNdsStageMPCliffStatusFloorLoopP0FloorEdgeMask =
            fp->coll_data.mask_stat & MAP_FLAG_FLOOREDGE;
    }
    else
    {
        gNdsStageMPCliffStatusFloorLoopP1StatusFinal = (u32)fp->status_id;
        gNdsStageMPCliffStatusFloorLoopP1MotionFinal = (u32)fp->motion_id;
        gNdsStageMPCliffStatusFloorLoopP1GAFinal = (u32)fp->ga;
        gNdsStageMPCliffStatusFloorLoopP1LineFinal =
            fp->coll_data.floor_line_id;
        gNdsStageMPCliffStatusFloorLoopP1FloorEdgeMask =
            fp->coll_data.mask_stat & MAP_FLAG_FLOOREDGE;
    }
}

void ndsFighterMarioFoxStageMPCliffStatusFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffStatusFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffStatusFloorLoopResult != 0u))
    {
        return;
    }

    if ((gNdsStageMPCliffStatusFloorLoopProcCallCount == 0u) &&
        (gNdsStageMPCliffStatusFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffStatusFloorLoopRunProbe(0u, TRUE);
        ndsStageMPCliffStatusFloorLoopRunProbe(1u, FALSE);
    }

    if ((gNdsFighterMarioFoxStageMPMotionStaleFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPMotionStaleFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffStatusFloorLoopPrepared != 0u) &&
        (gNdsStageMPCliffStatusFloorLoopBaseMPMotionStaleSeen != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPCliffStatusFloorLoopProcCallCount == 2u) &&
        (gNdsStageMPCliffStatusFloorLoopCheckFalseCount == 2u) &&
        (gNdsStageMPCliffStatusFloorLoopCheckTrueCount == 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffStatusFloorLoopFloorEdgeBranchCount == 1u) &&
        (gNdsStageMPCliffStatusFloorLoopFallBranchCount == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffStatusFloorLoopOttottoSetStatusCallCount == 1u) &&
        (gNdsStageMPCliffStatusFloorLoopFallSetStatusCallCount == 1u) &&
        (gNdsStageMPCliffStatusFloorLoopStatusSetCount == 2u))
    {
        mask |= 1u << 4;
    }
    if (gNdsStageMPCliffStatusFloorLoopAirSetCount == 1u)
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffStatusFloorLoopP0StatusFinal ==
            (u32)nFTCommonStatusOttotto) &&
        (gNdsStageMPCliffStatusFloorLoopP0MotionFinal ==
            (u32)nFTCommonMotionOttotto) &&
        (gNdsStageMPCliffStatusFloorLoopP0GAFinal ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffStatusFloorLoopP0FloorEdgeMask != 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffStatusFloorLoopP1StatusFinal ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPCliffStatusFloorLoopP1MotionFinal ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPCliffStatusFloorLoopP1GAFinal ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffStatusFloorLoopP1FloorEdgeMask == 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffStatusFloorLoopP0LineFinal >= 0) &&
        (gNdsStageMPCliffStatusFloorLoopP1LineFinal >= 0))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffStatusFloorLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffStatusFloorLoopActive == FALSE) &&
        (sNdsStageMPCliffStatusFloorLoopStatusActive == FALSE))
    {
        mask |= 1u << 9;
    }

    gNdsFighterMarioFoxStageMPCliffStatusFloorLoopCount = 2u;
    gNdsFighterMarioFoxStageMPCliffStatusFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffStatusFloorLoopDeferredMask = 0xffu;
    if ((mask & 0x3ffu) == 0x3ffu)
    {
        gNdsFighterMarioFoxStageMPCliffStatusFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffStatusFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffTickFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffTickFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffTickFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffTickFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffTickFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffTickFloorLoopCount = 0u;
    gNdsStageMPCliffTickFloorLoopPrepared = 0u;
    gNdsStageMPCliffTickFloorLoopBaseMPCliffStatusSeen = 0u;
    gNdsStageMPCliffTickFloorLoopOttottoUpdateCallCount = 0u;
    gNdsStageMPCliffTickFloorLoopOttottoInterruptCallCount = 0u;
    gNdsStageMPCliffTickFloorLoopOttottoMapCallCount = 0u;
    gNdsStageMPCliffTickFloorLoopOttottoAnimEndCheckCount = 0u;
    gNdsStageMPCliffTickFloorLoopOttottoFloorCheckCount = 0u;
    gNdsStageMPCliffTickFloorLoopOttottoFloorHitCount = 0u;
    gNdsStageMPCliffTickFloorLoopOttottoWaitSetStatusCallCount = 0u;
    gNdsStageMPCliffTickFloorLoopFallInterruptCallCount = 0u;
    gNdsStageMPCliffTickFloorLoopFallSpecialAirCheckCount = 0u;
    gNdsStageMPCliffTickFloorLoopFallAttackAirCheckCount = 0u;
    gNdsStageMPCliffTickFloorLoopFallJumpAerialCheckCount = 0u;
    gNdsStageMPCliffTickFloorLoopStatusSetCount = 0u;
    gNdsStageMPCliffTickFloorLoopP0StatusBefore = 0u;
    gNdsStageMPCliffTickFloorLoopP0MotionBefore = 0u;
    gNdsStageMPCliffTickFloorLoopP0GABefore = 0u;
    gNdsStageMPCliffTickFloorLoopP0StatusAfter = 0u;
    gNdsStageMPCliffTickFloorLoopP0MotionAfter = 0u;
    gNdsStageMPCliffTickFloorLoopP0GAAfter = 0u;
    gNdsStageMPCliffTickFloorLoopP0LineAfter = -1;
    gNdsStageMPCliffTickFloorLoopP1StatusBefore = 0u;
    gNdsStageMPCliffTickFloorLoopP1MotionBefore = 0u;
    gNdsStageMPCliffTickFloorLoopP1GABefore = 0u;
    gNdsStageMPCliffTickFloorLoopP1StatusAfter = 0u;
    gNdsStageMPCliffTickFloorLoopP1MotionAfter = 0u;
    gNdsStageMPCliffTickFloorLoopP1GAAfter = 0u;
    gNdsStageMPCliffTickFloorLoopP1LineAfter = -1;
    gNdsStageMPCliffTickFloorLoopUnsafeCount = 0u;
    sNdsStageMPCliffTickFloorLoopStatusActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffTickFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffTickFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffTickFloorLoopReset();
    gNdsStageMPCliffTickFloorLoopPrepared = 1u;
    gNdsStageMPCliffTickFloorLoopBaseMPCliffStatusSeen =
        (gNdsStageMPCliffStatusFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPCliffTickFloorLoopBaseMPCliffStatusSeen == 0u)
    {
        gNdsStageMPCliffTickFloorLoopUnsafeCount++;
    }
}

static void ndsStageMPCliffTickFloorLoopRunCallbacks(void)
{
    FTStruct *p0 = &sNdsFighterStructPool[0];
    FTStruct *p1 = &sNdsFighterStructPool[1];
    GObj *p0_gobj;
    GObj *p1_gobj;
    DObj *p0_root;
    DObj *p0_dobj;
    Vec3f edge_r;
    f32 floor_y = 0.0F;

    if ((ndsFighterStructIsPoolPointer(p0) == FALSE) ||
        (ndsFighterStructIsPoolPointer(p1) == FALSE) ||
        (p0->fighter_gobj == NULL) || (p1->fighter_gobj == NULL))
    {
        gNdsStageMPCliffTickFloorLoopUnsafeCount++;
        return;
    }

    p0_gobj = p0->fighter_gobj;
    p1_gobj = p1->fighter_gobj;
    p0_root = p0->joints[nFTPartsJointTopN];
    p0_dobj = DObjGetStruct(p0_gobj);
    if ((p0_root == NULL) || (p0_dobj == NULL) ||
        (p0->coll_data.floor_line_id < 0))
    {
        gNdsStageMPCliffTickFloorLoopUnsafeCount++;
        return;
    }

    gNdsStageMPCliffTickFloorLoopP0StatusBefore = (u32)p0->status_id;
    gNdsStageMPCliffTickFloorLoopP0MotionBefore = (u32)p0->motion_id;
    gNdsStageMPCliffTickFloorLoopP0GABefore = (u32)p0->ga;
    gNdsStageMPCliffTickFloorLoopP1StatusBefore = (u32)p1->status_id;
    gNdsStageMPCliffTickFloorLoopP1MotionBefore = (u32)p1->motion_id;
    gNdsStageMPCliffTickFloorLoopP1GABefore = (u32)p1->ga;

    mpCollisionGetFloorEdgeR(p0->coll_data.floor_line_id, &edge_r);
    p0->lr = +1;
    p0_root->translate.vec.f.x = edge_r.x - 1.0F;
    p0_dobj->translate.vec.f.x = p0_root->translate.vec.f.x;
    if (ndsStageFloorEdgeLoopFloorYAtX(p0->coll_data.floor_line_id,
                                       p0_root->translate.vec.f.x,
                                       &floor_y) != FALSE)
    {
        p0_root->translate.vec.f.y = floor_y;
        p0_dobj->translate.vec.f.y = floor_y;
    }
    p0->coll_data.mask_stat = MAP_FLAG_FLOOR | MAP_FLAG_FLOOREDGE;
    p0->coll_data.is_coll_end = FALSE;
    p0->coll_data.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    p0->input.pl.stick_range.x = 0;
    p0->input.pl.stick_range.y = 0;
    p0->anim_frame = 1.0F;
    p0_gobj->anim_frame = 1.0F;

    sNdsStageMPCliffTickFloorLoopStatusActive = TRUE;
    if (p0->proc_update == ftCommonOttottoProcUpdate)
    {
        p0->proc_update(p0_gobj);
    }
    else
    {
        gNdsStageMPCliffTickFloorLoopUnsafeCount++;
    }
    if (p0->proc_interrupt == ftCommonOttottoProcInterrupt)
    {
        p0->proc_interrupt(p0_gobj);
    }
    else
    {
        gNdsStageMPCliffTickFloorLoopUnsafeCount++;
    }
    if (p0->proc_map == ftCommonOttottoProcMap)
    {
        p0->proc_map(p0_gobj);
    }
    else
    {
        gNdsStageMPCliffTickFloorLoopUnsafeCount++;
    }
    sNdsStageMPCliffTickFloorLoopStatusActive = FALSE;

    if (p1->proc_interrupt == ftCommonFallProcInterrupt)
    {
        sNdsStageMPCliffTickFloorLoopStatusActive = TRUE;
        p1->proc_interrupt(p1_gobj);
        sNdsStageMPCliffTickFloorLoopStatusActive = FALSE;
    }
    else
    {
        gNdsStageMPCliffTickFloorLoopUnsafeCount++;
    }

    gNdsStageMPCliffTickFloorLoopP0StatusAfter = (u32)p0->status_id;
    gNdsStageMPCliffTickFloorLoopP0MotionAfter = (u32)p0->motion_id;
    gNdsStageMPCliffTickFloorLoopP0GAAfter = (u32)p0->ga;
    gNdsStageMPCliffTickFloorLoopP0LineAfter = p0->coll_data.floor_line_id;
    gNdsStageMPCliffTickFloorLoopP1StatusAfter = (u32)p1->status_id;
    gNdsStageMPCliffTickFloorLoopP1MotionAfter = (u32)p1->motion_id;
    gNdsStageMPCliffTickFloorLoopP1GAAfter = (u32)p1->ga;
    gNdsStageMPCliffTickFloorLoopP1LineAfter = p1->coll_data.floor_line_id;
}

void ndsFighterMarioFoxStageMPCliffTickFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffTickFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffTickFloorLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffStatusFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffStatusFloorLoopFinalize();
    }

    if ((gNdsFighterMarioFoxStageMPCliffStatusFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffStatusFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffTickFloorLoopPrepared != 0u) &&
        (gNdsStageMPCliffTickFloorLoopBaseMPCliffStatusSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffTickFloorLoopOttottoUpdateCallCount == 0u) &&
        (gNdsStageMPCliffTickFloorLoopFallInterruptCallCount == 0u) &&
        (gNdsStageMPCliffTickFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffTickFloorLoopRunCallbacks();
    }

    if ((gNdsStageMPCliffTickFloorLoopP0StatusBefore ==
            (u32)nFTCommonStatusOttotto) &&
        (gNdsStageMPCliffTickFloorLoopP0MotionBefore ==
            (u32)nFTCommonMotionOttotto) &&
        (gNdsStageMPCliffTickFloorLoopP0GABefore ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffTickFloorLoopOttottoUpdateCallCount == 1u) &&
        (gNdsStageMPCliffTickFloorLoopOttottoAnimEndCheckCount == 1u))
    {
        mask |= 1u << 3;
    }
    if (gNdsStageMPCliffTickFloorLoopOttottoInterruptCallCount == 1u)
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffTickFloorLoopOttottoMapCallCount == 1u) &&
        (gNdsStageMPCliffTickFloorLoopOttottoFloorCheckCount == 1u) &&
        (gNdsStageMPCliffTickFloorLoopOttottoFloorHitCount == 1u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffTickFloorLoopP0StatusAfter ==
            (u32)nFTCommonStatusOttotto) &&
        (gNdsStageMPCliffTickFloorLoopP0MotionAfter ==
            (u32)nFTCommonMotionOttotto) &&
        (gNdsStageMPCliffTickFloorLoopP0GAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffTickFloorLoopP0LineAfter >= 0))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffTickFloorLoopP1StatusBefore ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPCliffTickFloorLoopP1MotionBefore ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPCliffTickFloorLoopP1GABefore ==
            (u32)nMPKineticsAir))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffTickFloorLoopFallInterruptCallCount == 1u) &&
        (gNdsStageMPCliffTickFloorLoopFallSpecialAirCheckCount == 1u) &&
        (gNdsStageMPCliffTickFloorLoopFallAttackAirCheckCount == 1u) &&
        (gNdsStageMPCliffTickFloorLoopFallJumpAerialCheckCount == 1u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffTickFloorLoopP1StatusAfter ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPCliffTickFloorLoopP1MotionAfter ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPCliffTickFloorLoopP1GAAfter ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffTickFloorLoopP1LineAfter >= 0))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPCliffTickFloorLoopStatusSetCount == 0u) &&
        (gNdsStageMPCliffTickFloorLoopOttottoWaitSetStatusCallCount == 0u))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPCliffTickFloorLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffTickFloorLoopStatusActive == FALSE))
    {
        mask |= 1u << 11;
    }

    gNdsFighterMarioFoxStageMPCliffTickFloorLoopCount = 2u;
    gNdsFighterMarioFoxStageMPCliffTickFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffTickFloorLoopDeferredMask = 0xffu;
    if ((mask & 0xfffu) == 0xfffu)
    {
        gNdsFighterMarioFoxStageMPCliffTickFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffTickFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPFallMapFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPFallMapFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPFallMapFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPFallMapFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPFallMapFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPFallMapFloorLoopCount = 0u;
    gNdsStageMPFallMapFloorLoopPrepared = 0u;
    gNdsStageMPFallMapFloorLoopBaseMPCliffTickSeen = 0u;
    gNdsStageMPFallMapFloorLoopPhysicsCallbackCount = 0u;
    gNdsStageMPFallMapFloorLoopFastFallCheckCount = 0u;
    gNdsStageMPFallMapFloorLoopGravityCallCount = 0u;
    gNdsStageMPFallMapFloorLoopAirDriftCallCount = 0u;
    gNdsStageMPFallMapFloorLoopAirFrictionCallCount = 0u;
    gNdsStageMPFallMapFloorLoopIntegrateCount = 0u;
    gNdsStageMPFallMapFloorLoopMapCallbackCount = 0u;
    gNdsStageMPFallMapFloorLoopMapNoCollisionCount = 0u;
    gNdsStageMPFallMapFloorLoopP1StatusBefore = 0u;
    gNdsStageMPFallMapFloorLoopP1MotionBefore = 0u;
    gNdsStageMPFallMapFloorLoopP1GABefore = 0u;
    gNdsStageMPFallMapFloorLoopP1StatusAfter = 0u;
    gNdsStageMPFallMapFloorLoopP1MotionAfter = 0u;
    gNdsStageMPFallMapFloorLoopP1GAAfter = 0u;
    gNdsStageMPFallMapFloorLoopP1LineBefore = -1;
    gNdsStageMPFallMapFloorLoopP1LineAfter = -1;
    gNdsStageMPFallMapFloorLoopP1RootYBeforeMilli = 0;
    gNdsStageMPFallMapFloorLoopP1RootYAfterMilli = 0;
    gNdsStageMPFallMapFloorLoopP1VelXBeforeMilli = 0;
    gNdsStageMPFallMapFloorLoopP1VelYBeforeMilli = 0;
    gNdsStageMPFallMapFloorLoopP1VelXAfterMilli = 0;
    gNdsStageMPFallMapFloorLoopP1VelYAfterMilli = 0;
    gNdsStageMPFallMapFloorLoopUnsafeCount = 0u;
    sNdsStageMPFallMapFloorLoopPhysicsActive = FALSE;
    sNdsStageMPFallMapFloorLoopMapActive = FALSE;
}

void ndsFighterMarioFoxStageMPFallMapFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPFallMapFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPFallMapFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPFallMapFloorLoopReset();
    gNdsStageMPFallMapFloorLoopPrepared = 1u;
    gNdsStageMPFallMapFloorLoopBaseMPCliffTickSeen =
        (gNdsStageMPCliffTickFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPFallMapFloorLoopBaseMPCliffTickSeen == 0u)
    {
        gNdsStageMPFallMapFloorLoopUnsafeCount++;
    }
}

static void ndsStageMPFallMapFloorLoopRunCallbacks(void)
{
    FTStruct *p1 = &sNdsFighterStructPool[1];
    GObj *p1_gobj;
    DObj *root;
    f32 floor_y = 0.0F;

    if ((ndsFighterStructIsPoolPointer(p1) == FALSE) ||
        (p1->fighter_gobj == NULL))
    {
        gNdsStageMPFallMapFloorLoopUnsafeCount++;
        return;
    }
    p1_gobj = p1->fighter_gobj;
    root = p1->joints[nFTPartsJointTopN];
    if ((root == NULL) || (p1->attr == NULL) ||
        (p1->coll_data.floor_line_id < 0) ||
        (ndsStageFloorEdgeLoopFloorYAtX(p1->coll_data.floor_line_id,
                                        root->translate.vec.f.x,
                                        &floor_y) == FALSE))
    {
        gNdsStageMPFallMapFloorLoopUnsafeCount++;
        return;
    }

    root->translate.vec.f.y = floor_y + 200.0F;
    p1->coll_data.floor_dist = floor_y;
    p1->coll_data.mask_stat = 0u;
    p1->coll_data.is_coll_end = FALSE;
    p1->coll_data.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    p1->physics.vel_air.x = 0.25F;
    p1->physics.vel_air.y = -2.0F;
    p1->physics.vel_air.z = 0.0F;
    p1->vel_air = p1->physics.vel_air;
    p1->input.pl.stick_range.x = 0;
    p1->input.pl.stick_range.y = 0;

    gNdsStageMPFallMapFloorLoopP1StatusBefore = (u32)p1->status_id;
    gNdsStageMPFallMapFloorLoopP1MotionBefore = (u32)p1->motion_id;
    gNdsStageMPFallMapFloorLoopP1GABefore = (u32)p1->ga;
    gNdsStageMPFallMapFloorLoopP1LineBefore = p1->coll_data.floor_line_id;
    gNdsStageMPFallMapFloorLoopP1RootYBeforeMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);
    gNdsStageMPFallMapFloorLoopP1VelXBeforeMilli =
        ndsFloatToMilliSigned(p1->physics.vel_air.x);
    gNdsStageMPFallMapFloorLoopP1VelYBeforeMilli =
        ndsFloatToMilliSigned(p1->physics.vel_air.y);

    if (p1->proc_physics == ftPhysicsApplyAirVelDriftFastFall)
    {
        sNdsStageMPFallMapFloorLoopPhysicsActive = TRUE;
        p1->proc_physics(p1_gobj);
        sNdsStageMPFallMapFloorLoopPhysicsActive = FALSE;
    }
    else
    {
        gNdsStageMPFallMapFloorLoopUnsafeCount++;
    }

    root->translate.vec.f.x += p1->physics.vel_air.x;
    root->translate.vec.f.y += p1->physics.vel_air.y;
    root->translate.vec.f.z += p1->physics.vel_air.z;
    p1->vel_air = p1->physics.vel_air;
    gNdsStageMPFallMapFloorLoopIntegrateCount++;

    if (p1->proc_map == mpCommonProcFighterCliffFloorCeil)
    {
        sNdsStageMPFallMapFloorLoopMapActive = TRUE;
        p1->proc_map(p1_gobj);
        sNdsStageMPFallMapFloorLoopMapActive = FALSE;
    }
    else
    {
        gNdsStageMPFallMapFloorLoopUnsafeCount++;
    }

    gNdsStageMPFallMapFloorLoopP1StatusAfter = (u32)p1->status_id;
    gNdsStageMPFallMapFloorLoopP1MotionAfter = (u32)p1->motion_id;
    gNdsStageMPFallMapFloorLoopP1GAAfter = (u32)p1->ga;
    gNdsStageMPFallMapFloorLoopP1LineAfter = p1->coll_data.floor_line_id;
    gNdsStageMPFallMapFloorLoopP1RootYAfterMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);
    gNdsStageMPFallMapFloorLoopP1VelXAfterMilli =
        ndsFloatToMilliSigned(p1->physics.vel_air.x);
    gNdsStageMPFallMapFloorLoopP1VelYAfterMilli =
        ndsFloatToMilliSigned(p1->physics.vel_air.y);
}

void ndsFighterMarioFoxStageMPFallMapFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPFallMapFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPFallMapFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPFallMapFloorLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffTickFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffTickFloorLoopFinalize();
    }

    if ((gNdsFighterMarioFoxStageMPCliffTickFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffTickFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPFallMapFloorLoopPrepared != 0u) &&
        (gNdsStageMPFallMapFloorLoopBaseMPCliffTickSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPFallMapFloorLoopPhysicsCallbackCount == 0u) &&
        (gNdsStageMPFallMapFloorLoopMapCallbackCount == 0u) &&
        (gNdsStageMPFallMapFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPFallMapFloorLoopRunCallbacks();
    }

    if ((gNdsStageMPFallMapFloorLoopP1StatusBefore ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPFallMapFloorLoopP1MotionBefore ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPFallMapFloorLoopP1GABefore ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPFallMapFloorLoopP1LineBefore >= 0))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPFallMapFloorLoopPhysicsCallbackCount == 1u) &&
        (gNdsStageMPFallMapFloorLoopFastFallCheckCount == 1u) &&
        (gNdsStageMPFallMapFloorLoopGravityCallCount == 1u) &&
        (gNdsStageMPFallMapFloorLoopAirDriftCallCount == 1u) &&
        (gNdsStageMPFallMapFloorLoopAirFrictionCallCount == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPFallMapFloorLoopIntegrateCount == 1u) &&
        (gNdsStageMPFallMapFloorLoopP1RootYAfterMilli <
         gNdsStageMPFallMapFloorLoopP1RootYBeforeMilli))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPFallMapFloorLoopMapCallbackCount == 1u) &&
        (gNdsStageMPFallMapFloorLoopMapNoCollisionCount == 1u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPFallMapFloorLoopP1StatusAfter ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPFallMapFloorLoopP1MotionAfter ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPFallMapFloorLoopP1GAAfter == (u32)nMPKineticsAir))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPFallMapFloorLoopP1LineAfter ==
         gNdsStageMPFallMapFloorLoopP1LineBefore) &&
        (gNdsStageMPFallMapFloorLoopP1LineAfter >= 0))
    {
        mask |= 1u << 7;
    }
    if (gNdsStageMPFallMapFloorLoopP1VelYAfterMilli <
        gNdsStageMPFallMapFloorLoopP1VelYBeforeMilli)
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPFallMapFloorLoopUnsafeCount == 0u) &&
        (sNdsStageMPFallMapFloorLoopPhysicsActive == FALSE) &&
        (sNdsStageMPFallMapFloorLoopMapActive == FALSE))
    {
        mask |= 1u << 9;
    }

    gNdsFighterMarioFoxStageMPFallMapFloorLoopCount = 1u;
    gNdsFighterMarioFoxStageMPFallMapFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPFallMapFloorLoopDeferredMask = 0xffu;
    if ((mask & 0x3ffu) == 0x3ffu)
    {
        gNdsFighterMarioFoxStageMPFallMapFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPFallMapFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPFallLandFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPFallLandFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPFallLandFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPFallLandFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPFallLandFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPFallLandFloorLoopCount = 0u;
    gNdsStageMPFallLandFloorLoopPrepared = 0u;
    gNdsStageMPFallLandFloorLoopBaseMPFallMapSeen = 0u;
    gNdsStageMPFallLandFloorLoopPhysicsCallbackCount = 0u;
    gNdsStageMPFallLandFloorLoopFastFallCheckCount = 0u;
    gNdsStageMPFallLandFloorLoopGravityCallCount = 0u;
    gNdsStageMPFallLandFloorLoopAirDriftCallCount = 0u;
    gNdsStageMPFallLandFloorLoopAirFrictionCallCount = 0u;
    gNdsStageMPFallLandFloorLoopIntegrateCount = 0u;
    gNdsStageMPFallLandFloorLoopMapCallbackCount = 0u;
    gNdsStageMPFallLandFloorLoopMapFloorCollisionCount = 0u;
    gNdsStageMPFallLandFloorLoopSetLandingFloorCount = 0u;
    gNdsStageMPFallLandFloorLoopWaitOrLandingCount = 0u;
    gNdsStageMPFallLandFloorLoopLandingSetStatusCallCount = 0u;
    gNdsStageMPFallLandFloorLoopLandingParamCallCount = 0u;
    gNdsStageMPFallLandFloorLoopStatusSetCallCount = 0u;
    gNdsStageMPFallLandFloorLoopSetGroundCallCount = 0u;
    gNdsStageMPFallLandFloorLoopP1StatusBefore = 0u;
    gNdsStageMPFallLandFloorLoopP1MotionBefore = 0u;
    gNdsStageMPFallLandFloorLoopP1GABefore = 0u;
    gNdsStageMPFallLandFloorLoopP1StatusAfter = 0u;
    gNdsStageMPFallLandFloorLoopP1MotionAfter = 0u;
    gNdsStageMPFallLandFloorLoopP1GAAfter = 0u;
    gNdsStageMPFallLandFloorLoopP1LineBefore = -1;
    gNdsStageMPFallLandFloorLoopP1LineAfter = -1;
    gNdsStageMPFallLandFloorLoopP1FloorYMilli = 0;
    gNdsStageMPFallLandFloorLoopP1RootYBeforeMilli = 0;
    gNdsStageMPFallLandFloorLoopP1RootYAfterPhysicsMilli = 0;
    gNdsStageMPFallLandFloorLoopP1RootYAfterMilli = 0;
    gNdsStageMPFallLandFloorLoopP1VelXBeforeMilli = 0;
    gNdsStageMPFallLandFloorLoopP1VelYBeforeMilli = 0;
    gNdsStageMPFallLandFloorLoopP1VelXAfterMilli = 0;
    gNdsStageMPFallLandFloorLoopP1VelYAfterMilli = 0;
    gNdsStageMPFallLandFloorLoopUnsafeCount = 0u;
    sNdsStageMPFallLandFloorLoopPhysicsActive = FALSE;
    sNdsStageMPFallLandFloorLoopMapActive = FALSE;
    sNdsStageMPFallLandFloorLoopSetStatusActive = FALSE;
}

void ndsFighterMarioFoxStageMPFallLandFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPFallLandFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPFallLandFloorLoopReset();
    gNdsStageMPFallLandFloorLoopPrepared = 1u;
    gNdsStageMPFallLandFloorLoopBaseMPFallMapSeen =
        (gNdsStageMPFallMapFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPFallLandFloorLoopBaseMPFallMapSeen == 0u)
    {
        gNdsStageMPFallLandFloorLoopUnsafeCount++;
    }
}

static void ndsStageMPFallLandFloorLoopRunCallbacks(void)
{
    FTStruct *p1 = &sNdsFighterStructPool[1];
    GObj *p1_gobj;
    DObj *root;
    f32 floor_y = 0.0F;

    if ((ndsFighterStructIsPoolPointer(p1) == FALSE) ||
        (p1->fighter_gobj == NULL))
    {
        gNdsStageMPFallLandFloorLoopUnsafeCount++;
        return;
    }
    p1_gobj = p1->fighter_gobj;
    root = p1->joints[nFTPartsJointTopN];
    if ((root == NULL) || (p1->attr == NULL) ||
        (p1->coll_data.floor_line_id < 0) ||
        (ndsStageFloorEdgeLoopFloorYAtX(p1->coll_data.floor_line_id,
                                        root->translate.vec.f.x,
                                        &floor_y) == FALSE))
    {
        gNdsStageMPFallLandFloorLoopUnsafeCount++;
        return;
    }

    root->translate.vec.f.y = floor_y + 4.0F;
    p1->status_id = nFTCommonStatusFall;
    p1->motion_id = nFTCommonMotionFall;
    p1->motion_script_id = nFTCommonMotionFall;
    p1->ga = nMPKineticsAir;
    p1->jumps_used = 1;
    p1->proc_update = NULL;
    p1->proc_interrupt = ftCommonFallProcInterrupt;
    p1->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
    p1->proc_map = mpCommonProcFighterCliffFloorCeil;
    p1->coll_data.p_translate = &root->translate.vec.f;
    p1->coll_data.p_map_coll = &p1->coll_data.map_coll;
    p1->coll_data.floor_dist = floor_y;
    p1->coll_data.mask_stat = 0u;
    p1->coll_data.mask_curr = 0u;
    p1->coll_data.is_coll_end = FALSE;
    p1->coll_data.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    p1->physics.vel_air.x = 0.25F;
    p1->physics.vel_air.y = -8.0F;
    p1->physics.vel_air.z = 0.0F;
    p1->vel_air = p1->physics.vel_air;
    p1->is_fastfall = FALSE;
    p1->input.pl.stick_range.x = 0;
    p1->input.pl.stick_range.y = 0;

    gNdsStageMPFallLandFloorLoopP1StatusBefore = (u32)p1->status_id;
    gNdsStageMPFallLandFloorLoopP1MotionBefore = (u32)p1->motion_id;
    gNdsStageMPFallLandFloorLoopP1GABefore = (u32)p1->ga;
    gNdsStageMPFallLandFloorLoopP1LineBefore = p1->coll_data.floor_line_id;
    gNdsStageMPFallLandFloorLoopP1FloorYMilli =
        ndsFloatToMilliSigned(floor_y);
    gNdsStageMPFallLandFloorLoopP1RootYBeforeMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);
    gNdsStageMPFallLandFloorLoopP1VelXBeforeMilli =
        ndsFloatToMilliSigned(p1->physics.vel_air.x);
    gNdsStageMPFallLandFloorLoopP1VelYBeforeMilli =
        ndsFloatToMilliSigned(p1->physics.vel_air.y);

    if (p1->proc_physics == ftPhysicsApplyAirVelDriftFastFall)
    {
        sNdsStageMPFallLandFloorLoopPhysicsActive = TRUE;
        p1->proc_physics(p1_gobj);
        sNdsStageMPFallLandFloorLoopPhysicsActive = FALSE;
    }
    else
    {
        gNdsStageMPFallLandFloorLoopUnsafeCount++;
    }

    root->translate.vec.f.x += p1->physics.vel_air.x;
    root->translate.vec.f.y += p1->physics.vel_air.y;
    root->translate.vec.f.z += p1->physics.vel_air.z;
    p1->vel_air = p1->physics.vel_air;
    gNdsStageMPFallLandFloorLoopIntegrateCount++;
    gNdsStageMPFallLandFloorLoopP1RootYAfterPhysicsMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);

    if (p1->proc_map == mpCommonProcFighterCliffFloorCeil)
    {
        sNdsStageMPFallLandFloorLoopMapActive = TRUE;
        p1->proc_map(p1_gobj);
        sNdsStageMPFallLandFloorLoopMapActive = FALSE;
    }
    else
    {
        gNdsStageMPFallLandFloorLoopUnsafeCount++;
    }

    gNdsStageMPFallLandFloorLoopP1StatusAfter = (u32)p1->status_id;
    gNdsStageMPFallLandFloorLoopP1MotionAfter = (u32)p1->motion_id;
    gNdsStageMPFallLandFloorLoopP1GAAfter = (u32)p1->ga;
    gNdsStageMPFallLandFloorLoopP1LineAfter = p1->coll_data.floor_line_id;
    gNdsStageMPFallLandFloorLoopP1RootYAfterMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);
    gNdsStageMPFallLandFloorLoopP1VelXAfterMilli =
        ndsFloatToMilliSigned(p1->physics.vel_air.x);
    gNdsStageMPFallLandFloorLoopP1VelYAfterMilli =
        ndsFloatToMilliSigned(p1->physics.vel_air.y);
}

void ndsFighterMarioFoxStageMPFallLandFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPFallLandFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPFallLandFloorLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPFallMapFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPFallMapFloorLoopFinalize();
    }

    if ((gNdsFighterMarioFoxStageMPFallMapFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPFallMapFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsStageMPFallLandFloorLoopPrepared != 0u) &&
        (gNdsStageMPFallLandFloorLoopBaseMPFallMapSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPFallLandFloorLoopPhysicsCallbackCount == 0u) &&
        (gNdsStageMPFallLandFloorLoopMapCallbackCount == 0u) &&
        (gNdsStageMPFallLandFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPFallLandFloorLoopRunCallbacks();
    }

    if ((gNdsStageMPFallLandFloorLoopP1StatusBefore ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPFallLandFloorLoopP1MotionBefore ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPFallLandFloorLoopP1GABefore ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPFallLandFloorLoopP1LineBefore >= 0))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPFallLandFloorLoopPhysicsCallbackCount == 1u) &&
        (gNdsStageMPFallLandFloorLoopFastFallCheckCount == 1u) &&
        (gNdsStageMPFallLandFloorLoopGravityCallCount == 1u) &&
        (gNdsStageMPFallLandFloorLoopAirDriftCallCount == 1u) &&
        (gNdsStageMPFallLandFloorLoopAirFrictionCallCount == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPFallLandFloorLoopIntegrateCount == 1u) &&
        (gNdsStageMPFallLandFloorLoopP1RootYAfterPhysicsMilli <=
         gNdsStageMPFallLandFloorLoopP1FloorYMilli))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPFallLandFloorLoopMapCallbackCount == 1u) &&
        (gNdsStageMPFallLandFloorLoopMapFloorCollisionCount == 1u) &&
        (gNdsStageMPFallLandFloorLoopSetLandingFloorCount == 1u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPFallLandFloorLoopWaitOrLandingCount == 1u) &&
        (gNdsStageMPFallLandFloorLoopLandingSetStatusCallCount == 1u) &&
        (gNdsStageMPFallLandFloorLoopLandingParamCallCount <= 1u) &&
        (gNdsStageMPFallLandFloorLoopStatusSetCallCount == 1u) &&
        (gNdsStageMPFallLandFloorLoopSetGroundCallCount == 1u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPFallLandFloorLoopP1StatusAfter ==
            (u32)nFTCommonStatusLandingLight) &&
        (gNdsStageMPFallLandFloorLoopP1MotionAfter ==
            (u32)nFTCommonMotionLandingLight) &&
        (gNdsStageMPFallLandFloorLoopP1GAAfter ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPFallLandFloorLoopP1LineAfter ==
         gNdsStageMPFallLandFloorLoopP1LineBefore) &&
        (gNdsStageMPFallLandFloorLoopP1LineAfter >= 0))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPFallLandFloorLoopP1RootYAfterMilli ==
            gNdsStageMPFallLandFloorLoopP1FloorYMilli) &&
        (gNdsStageMPFallLandFloorLoopP1VelYBeforeMilli < 0) &&
        (gNdsStageMPFallLandFloorLoopP1VelYAfterMilli < 0) &&
        (gNdsStageMPFallLandFloorLoopP1VelYAfterMilli <=
            gNdsStageMPFallLandFloorLoopP1VelYBeforeMilli))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPFallLandFloorLoopUnsafeCount == 0u) &&
        (sNdsStageMPFallLandFloorLoopPhysicsActive == FALSE) &&
        (sNdsStageMPFallLandFloorLoopMapActive == FALSE) &&
        (sNdsStageMPFallLandFloorLoopSetStatusActive == FALSE))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxStageMPFallLandFloorLoopCount = 1u;
    gNdsFighterMarioFoxStageMPFallLandFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPFallLandFloorLoopDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxStageMPFallLandFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPFallLandFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP_SAFE_PASS;
    }
}

static sb32 ndsStageMPCeilFloorLoopChooseLine(s32 *line_id, f32 *sample_x,
                                              f32 *ceil_y)
{
    MPGeometryData *geometry = gMPCollisionGeometry;
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;

    if (line_id != NULL)
    {
        *line_id = -1;
    }
    if (sample_x != NULL)
    {
        *sample_x = 0.0F;
    }
    if (ceil_y != NULL)
    {
        *ceil_y = 0.0F;
    }
    if (ndsStageCollisionLoopGeometryReady() == FALSE)
    {
        return FALSE;
    }
    line_info = geometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(geometry);
    if (yakumono_count > 64u)
    {
        yakumono_count = 64u;
    }
    gNdsStageMPCeilFloorLoopCeilLineCount = 0u;
    for (i = 0u; i < yakumono_count; i++)
    {
        MPLineInfo *info = ndsMPLineInfoAt(line_info, i);
        s32 first = (s32)ndsMPLineInfoGroupID(info, nMPLineKindCeil);
        s32 count = (s32)ndsMPLineInfoLineCount(info, nMPLineKindCeil);
        s32 end;
        s32 candidate;

        if (count <= 0)
        {
            continue;
        }
        if (count > 4096)
        {
            count = 4096;
        }
        gNdsStageMPCeilFloorLoopCeilLineCount += (u32)count;
        end = first + count;
        for (candidate = first; candidate < end; candidate++)
        {
            Vec3f left;
            Vec3f right;
            Vec3f probe;
            f32 dist = 0.0F;
            f32 width;

            if ((ndsMPFindLineEndpoints(candidate, &left, &right, NULL,
                    NULL) == FALSE) ||
                (ndsMPLineIDIsCeil(candidate) == FALSE))
            {
                continue;
            }
            width = fabsf(right.x - left.x);
            if (width < 64.0F)
            {
                continue;
            }
            probe.x = (left.x + right.x) * 0.5F;
            probe.y = 0.0F;
            probe.z = 0.0F;
            if (mpCollisionGetFCCommonCeil(candidate, &probe, &dist,
                    NULL, NULL) == FALSE)
            {
                continue;
            }
            if (line_id != NULL)
            {
                *line_id = candidate;
            }
            if (sample_x != NULL)
            {
                *sample_x = probe.x;
            }
            if (ceil_y != NULL)
            {
                *ceil_y = probe.y + dist;
            }
            gNdsStageMPCeilFloorLoopSelectedCeilLineID = candidate;
            gNdsStageMPCeilFloorLoopSelectedCeilKind =
                (u32)ndsMPGetLineKindForLineID(candidate);
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsFighterMarioFoxStageMPCeilFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCeilFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCeilFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCeilFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCeilFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCeilFloorLoopCount = 0u;
    gNdsStageMPCeilFloorLoopPrepared = 0u;
    gNdsStageMPCeilFloorLoopBaseMPFallLandSeen = 0u;
    gNdsStageMPCeilFloorLoopCeilLineCount = 0u;
    gNdsStageMPCeilFloorLoopSelectedCeilLineID = -1;
    gNdsStageMPCeilFloorLoopSelectedCeilKind = 0xffffffffu;
    gNdsStageMPCeilFloorLoopCheckCallCount = 0u;
    gNdsStageMPCeilFloorLoopCheckHitCount = 0u;
    gNdsStageMPCeilFloorLoopCheckMissCount = 0u;
    gNdsStageMPCeilFloorLoopLineSweepSameCallCount = 0u;
    gNdsStageMPCeilFloorLoopLineSweepSameHitCount = 0u;
    gNdsStageMPCeilFloorLoopLineSweepSameMissCount = 0u;
    gNdsStageMPCeilFloorLoopLineSweepDiffCallCount = 0u;
    gNdsStageMPCeilFloorLoopLineSweepDiffHitCount = 0u;
    gNdsStageMPCeilFloorLoopLineSweepDiffMissCount = 0u;
    gNdsStageMPCeilFloorLoopLineSweepVisitCount = 0u;
    gNdsStageMPCeilFloorLoopLineSweepCandidateCount = 0u;
    gNdsStageMPCeilFloorLoopFCCommonCallCount = 0u;
    gNdsStageMPCeilFloorLoopFCCommonHitCount = 0u;
    gNdsStageMPCeilFloorLoopFCCommonMissCount = 0u;
    gNdsStageMPCeilFloorLoopRunAdjustCallCount = 0u;
    gNdsStageMPCeilFloorLoopMaskCurrCeilCount = 0u;
    gNdsStageMPCeilFloorLoopMaskStatCeilCount = 0u;
    gNdsStageMPCeilFloorLoopRootYBeforeMilli = 0;
    gNdsStageMPCeilFloorLoopRootYAfterCheckMilli = 0;
    gNdsStageMPCeilFloorLoopRootYAfterAdjustMilli = 0;
    gNdsStageMPCeilFloorLoopPrevTopYMilli = 0;
    gNdsStageMPCeilFloorLoopTargetTopYMilli = 0;
    gNdsStageMPCeilFloorLoopCeilYMilli = 0;
    gNdsStageMPCeilFloorLoopCeilDistMilli = 0;
    gNdsStageMPCeilFloorLoopUnsafeCount = 0u;
}

void ndsFighterMarioFoxStageMPCeilFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPCeilFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCeilFloorLoopReset();
    gNdsStageMPCeilFloorLoopPrepared = 1u;
    gNdsStageMPCeilFloorLoopBaseMPFallLandSeen =
        (gNdsStageMPFallLandFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPCeilFloorLoopBaseMPFallLandSeen == 0u)
    {
        gNdsStageMPCeilFloorLoopUnsafeCount++;
    }
}

static void ndsStageMPCeilFloorLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    MPCollData coll;
    Vec3f translate;
    f32 sample_x = 0.0F;
    f32 ceil_y = 0.0F;
    f32 top;
    s32 line_id = -1;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (ndsStageMPCeilFloorLoopChooseLine(&line_id, &sample_x,
            &ceil_y) == FALSE))
    {
        gNdsStageMPCeilFloorLoopUnsafeCount++;
        return;
    }

    top = fp->coll_data.map_coll.top;
    if (top <= 0.0F)
    {
        top = 80.0F;
    }
    memset(&coll, 0, sizeof(coll));
    translate.x = sample_x;
    translate.y = ceil_y - top + 8.0F;
    translate.z = 0.0F;
    coll.p_translate = &translate;
    coll.pos_prev.x = sample_x;
    coll.pos_prev.y = ceil_y - top - 8.0F;
    coll.pos_prev.z = 0.0F;
    coll.pos_diff.x = 0.0F;
    coll.pos_diff.y = translate.y - coll.pos_prev.y;
    coll.pos_diff.z = 0.0F;
    coll.map_coll = fp->coll_data.map_coll;
    coll.map_coll.top = top;
    coll.p_map_coll = &coll.map_coll;
    coll.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    coll.floor_line_id = -1;
    coll.ceil_line_id = -1;
    coll.lwall_line_id = -1;
    coll.rwall_line_id = -1;
    coll.ewall_line_id = -1;
    coll.cliff_id = -1;
    coll.ignore_line_id = -1;

    gNdsStageMPCeilFloorLoopRootYBeforeMilli =
        ndsFloatToMilliSigned(translate.y);
    gNdsStageMPCeilFloorLoopPrevTopYMilli =
        ndsFloatToMilliSigned(coll.pos_prev.y + top);
    gNdsStageMPCeilFloorLoopTargetTopYMilli =
        ndsFloatToMilliSigned(translate.y + top);
    gNdsStageMPCeilFloorLoopCeilYMilli =
        ndsFloatToMilliSigned(ceil_y);

    if (mpProcessCheckTestCeilCollisionAdjNew(&coll) == FALSE)
    {
        gNdsStageMPCeilFloorLoopRootYAfterCheckMilli =
            ndsFloatToMilliSigned(translate.y);
        gNdsStageMPCeilFloorLoopRootYAfterAdjustMilli =
            ndsFloatToMilliSigned(translate.y);
        return;
    }
    gNdsStageMPCeilFloorLoopRootYAfterCheckMilli =
        ndsFloatToMilliSigned(translate.y);
    if ((coll.ceil_line_id != line_id) ||
        ((coll.mask_curr & MAP_FLAG_CEIL) == 0u))
    {
        gNdsStageMPCeilFloorLoopUnsafeCount++;
        gNdsStageMPCeilFloorLoopRootYAfterAdjustMilli =
            ndsFloatToMilliSigned(translate.y);
        return;
    }
    mpProcessRunCeilCollisionAdjNew(&coll);
    gNdsStageMPCeilFloorLoopRootYAfterAdjustMilli =
        ndsFloatToMilliSigned(translate.y);
    if ((coll.mask_stat & MAP_FLAG_CEIL) == 0u)
    {
        gNdsStageMPCeilFloorLoopUnsafeCount++;
    }
}

void ndsFighterMarioFoxStageMPCeilFloorLoopFinalize(void)
{
    u32 mask = 0u;
    s32 top_milli;
    s32 expected_root_milli;
    s32 root_delta;
    FTStruct *fp = &sNdsFighterStructPool[1];
    f32 top = 80.0F;

    if ((ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled() == FALSE) ||
        (gNdsStageMPCeilFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCeilFloorLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPFallLandFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPFallLandFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPFallLandFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPFallLandFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCeilFloorLoopBaseMPFallLandSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCeilFloorLoopPrepared != 0u) &&
        (gNdsStageMPCeilFloorLoopBaseMPFallLandSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCeilFloorLoopCheckCallCount == 0u) &&
        (gNdsStageMPCeilFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPCeilFloorLoopRunProbe();
    }

    if ((gNdsStageMPCeilFloorLoopCeilLineCount > 0u) &&
        (gNdsStageMPCeilFloorLoopSelectedCeilLineID >= 0) &&
        (gNdsStageMPCeilFloorLoopSelectedCeilKind ==
            (u32)nMPLineKindCeil))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCeilFloorLoopCheckCallCount == 1u) &&
        (gNdsStageMPCeilFloorLoopCheckHitCount == 1u) &&
        (gNdsStageMPCeilFloorLoopLineSweepDiffCallCount == 1u) &&
        (gNdsStageMPCeilFloorLoopLineSweepDiffHitCount == 1u) &&
        (gNdsStageMPCeilFloorLoopMaskCurrCeilCount == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCeilFloorLoopRunAdjustCallCount == 1u) &&
        (gNdsStageMPCeilFloorLoopFCCommonHitCount > 0u) &&
        (gNdsStageMPCeilFloorLoopMaskStatCeilCount == 1u))
    {
        mask |= 1u << 4;
    }
    if ((ndsFighterStructIsPoolPointer(fp) != FALSE) &&
        (fp->coll_data.map_coll.top > 0.0F))
    {
        top = fp->coll_data.map_coll.top;
    }
    top_milli = ndsFloatToMilliSigned(top);
    expected_root_milli =
        gNdsStageMPCeilFloorLoopCeilYMilli - top_milli;
    root_delta =
        gNdsStageMPCeilFloorLoopRootYAfterAdjustMilli -
        expected_root_milli;
    if (root_delta < 0)
    {
        root_delta = -root_delta;
    }
    if ((gNdsStageMPCeilFloorLoopRootYBeforeMilli >
            gNdsStageMPCeilFloorLoopRootYAfterAdjustMilli) &&
        (root_delta <= 2))
    {
        mask |= 1u << 5;
    }
    if (gNdsStageMPCeilFloorLoopUnsafeCount == 0u)
    {
        mask |= 1u << 6;
    }

    gNdsFighterMarioFoxStageMPCeilFloorLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCeilFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPCeilFloorLoopDeferredMask = 0xffu;
    if ((mask & 0x7fu) == 0x7fu)
    {
        gNdsFighterMarioFoxStageMPCeilFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCeilFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCeilStatusFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCeilStatusFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCeilStatusFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCeilStatusFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCeilStatusFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCeilStatusFloorLoopCount = 0u;
    gNdsStageMPCeilStatusFloorLoopPrepared = 0u;
    gNdsStageMPCeilStatusFloorLoopBaseMPCeilSeen = 0u;
    gNdsStageMPCeilStatusFloorLoopMapCallbackCount = 0u;
    gNdsStageMPCeilStatusFloorLoopCheckCeilHeavyCliffCount = 0u;
    gNdsStageMPCeilStatusFloorLoopSpecialCollisionCount = 0u;
    gNdsStageMPCeilStatusFloorLoopCeilCollisionCount = 0u;
    gNdsStageMPCeilStatusFloorLoopCeilAdjustCount = 0u;
    gNdsStageMPCeilStatusFloorLoopCeilHeavyMaskCount = 0u;
    gNdsStageMPCeilStatusFloorLoopStopCeilSetStatusCount = 0u;
    gNdsStageMPCeilStatusFloorLoopFtMainSetStatusCount = 0u;
    gNdsStageMPCeilStatusFloorLoopPlayAnimEventsCount = 0u;
    gNdsStageMPCeilStatusFloorLoopUnsafeCount = 0u;
    gNdsStageMPCeilStatusFloorLoopSelectedCeilLineID = -1;
    gNdsStageMPCeilStatusFloorLoopSelectedCeilKind = 0xffffffffu;
    gNdsStageMPCeilStatusFloorLoopStatusBefore = 0u;
    gNdsStageMPCeilStatusFloorLoopMotionBefore = 0u;
    gNdsStageMPCeilStatusFloorLoopGABefore = 0u;
    gNdsStageMPCeilStatusFloorLoopStatusAfter = 0u;
    gNdsStageMPCeilStatusFloorLoopMotionAfter = 0u;
    gNdsStageMPCeilStatusFloorLoopGAAfter = 0u;
    gNdsStageMPCeilStatusFloorLoopRootYBeforeMilli = 0;
    gNdsStageMPCeilStatusFloorLoopRootYAfterMilli = 0;
    gNdsStageMPCeilStatusFloorLoopVelYBeforeMilli = 0;
    gNdsStageMPCeilStatusFloorLoopVelYAfterMilli = 0;
    gNdsStageMPCeilStatusFloorLoopMaskCurr = 0u;
    gNdsStageMPCeilStatusFloorLoopMaskStat = 0u;
    sNdsStageMPCeilStatusFloorLoopMapActive = FALSE;
    sNdsStageMPCeilStatusFloorLoopSetStatusActive = FALSE;
}

void ndsFighterMarioFoxStageMPCeilStatusFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCeilStatusFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCeilStatusFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCeilStatusFloorLoopReset();
    gNdsStageMPCeilStatusFloorLoopPrepared = 1u;
    gNdsStageMPCeilStatusFloorLoopBaseMPCeilSeen =
        (gNdsStageMPCeilFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPCeilStatusFloorLoopBaseMPCeilSeen == 0u)
    {
        gNdsStageMPCeilStatusFloorLoopUnsafeCount++;
    }
}

static void ndsStageMPCeilStatusFloorLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;
    DObj *root;
    f32 sample_x = 0.0F;
    f32 ceil_y = 0.0F;
    f32 top;
    s32 line_id = -1;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (ndsStageMPCeilFloorLoopChooseLine(&line_id, &sample_x,
            &ceil_y) == FALSE))
    {
        gNdsStageMPCeilStatusFloorLoopUnsafeCount++;
        return;
    }
    fighter_gobj = fp->fighter_gobj;
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPCeilStatusFloorLoopUnsafeCount++;
        return;
    }

    top = fp->coll_data.map_coll.top;
    if (top <= 0.0F)
    {
        top = 80.0F;
    }

    gNdsStageMPCeilStatusFloorLoopSelectedCeilLineID = line_id;
    gNdsStageMPCeilStatusFloorLoopSelectedCeilKind =
        (u32)ndsMPGetLineKindForLineID(line_id);

    fp->status_id = nFTCommonStatusFall;
    fp->motion_id = nFTCommonMotionFall;
    fp->motion_script_id = nFTCommonMotionFall;
    fp->ga = nMPKineticsAir;
    fp->proc_map = mpCommonProcFighterCliffFloorCeil;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 40.0F;
    fp->physics.vel_air.z = 3.0F;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.map_coll.top = top;
    fp->coll_data.pos_prev.x = sample_x;
    fp->coll_data.pos_prev.y = ceil_y - top - 8.0F;
    fp->coll_data.pos_prev.z = 0.0F;
    root->translate.vec.f.x = sample_x;
    root->translate.vec.f.y = ceil_y - top + 8.0F;
    root->translate.vec.f.z = 0.0F;
    fp->coll_data.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    fp->coll_data.mask_curr = 0u;
    fp->coll_data.mask_stat = 0u;
    fp->coll_data.floor_line_id = -1;
    fp->coll_data.ignore_line_id = -1;
    fp->coll_data.is_coll_end = FALSE;

    gNdsStageMPCeilStatusFloorLoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCeilStatusFloorLoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCeilStatusFloorLoopGABefore = (u32)fp->ga;
    gNdsStageMPCeilStatusFloorLoopRootYBeforeMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);
    gNdsStageMPCeilStatusFloorLoopVelYBeforeMilli =
        ndsFloatToMilliSigned(fp->physics.vel_air.y);

    sNdsStageMPCeilStatusFloorLoopMapActive = TRUE;
    fp->proc_map(fighter_gobj);
    sNdsStageMPCeilStatusFloorLoopMapActive = FALSE;

    gNdsStageMPCeilStatusFloorLoopStatusAfter = (u32)fp->status_id;
    gNdsStageMPCeilStatusFloorLoopMotionAfter = (u32)fp->motion_id;
    gNdsStageMPCeilStatusFloorLoopGAAfter = (u32)fp->ga;
    gNdsStageMPCeilStatusFloorLoopRootYAfterMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);
    gNdsStageMPCeilStatusFloorLoopVelYAfterMilli =
        ndsFloatToMilliSigned(fp->physics.vel_air.y);
    gNdsStageMPCeilStatusFloorLoopMaskCurr = fp->coll_data.mask_curr;
    gNdsStageMPCeilStatusFloorLoopMaskStat = fp->coll_data.mask_stat;
}

void ndsFighterMarioFoxStageMPCeilStatusFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCeilStatusFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCeilStatusFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCeilStatusFloorLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCeilFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCeilFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCeilFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCeilFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCeilStatusFloorLoopBaseMPCeilSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCeilStatusFloorLoopPrepared != 0u) &&
        (gNdsStageMPCeilStatusFloorLoopBaseMPCeilSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCeilStatusFloorLoopMapCallbackCount == 0u) &&
        (gNdsStageMPCeilStatusFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPCeilStatusFloorLoopRunProbe();
    }

    if ((gNdsStageMPCeilStatusFloorLoopSelectedCeilLineID >= 0) &&
        (gNdsStageMPCeilStatusFloorLoopSelectedCeilKind ==
            (u32)nMPLineKindCeil))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCeilStatusFloorLoopMapCallbackCount == 1u) &&
        (gNdsStageMPCeilStatusFloorLoopCheckCeilHeavyCliffCount == 1u) &&
        (gNdsStageMPCeilStatusFloorLoopSpecialCollisionCount >= 1u) &&
        (gNdsStageMPCeilStatusFloorLoopCeilCollisionCount == 1u) &&
        (gNdsStageMPCeilStatusFloorLoopCeilAdjustCount == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCeilStatusFloorLoopCeilHeavyMaskCount == 1u) &&
        ((gNdsStageMPCeilStatusFloorLoopMaskCurr &
            MAP_FLAG_CEILHEAVY) != 0u) &&
        ((gNdsStageMPCeilStatusFloorLoopMaskCurr &
            MAP_FLAG_CEIL) != 0u) &&
        ((gNdsStageMPCeilStatusFloorLoopMaskStat &
            MAP_FLAG_CEIL) != 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCeilStatusFloorLoopStopCeilSetStatusCount == 1u) &&
        (gNdsStageMPCeilStatusFloorLoopFtMainSetStatusCount == 1u) &&
        (gNdsStageMPCeilStatusFloorLoopPlayAnimEventsCount == 1u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCeilStatusFloorLoopStatusBefore ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPCeilStatusFloorLoopMotionBefore ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPCeilStatusFloorLoopGABefore ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCeilStatusFloorLoopStatusAfter ==
            (u32)nFTCommonStatusStopCeil) &&
        (gNdsStageMPCeilStatusFloorLoopMotionAfter ==
            (u32)nFTCommonMotionStopCeil) &&
        (gNdsStageMPCeilStatusFloorLoopGAAfter ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCeilStatusFloorLoopVelYBeforeMilli > 0) &&
        (gNdsStageMPCeilStatusFloorLoopVelYAfterMilli == 0) &&
        (gNdsStageMPCeilStatusFloorLoopRootYBeforeMilli >
            gNdsStageMPCeilStatusFloorLoopRootYAfterMilli))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCeilStatusFloorLoopUnsafeCount == 0u) &&
        (sNdsStageMPCeilStatusFloorLoopMapActive == FALSE) &&
        (sNdsStageMPCeilStatusFloorLoopSetStatusActive == FALSE))
    {
        mask |= 1u << 8;
    }

    gNdsFighterMarioFoxStageMPCeilStatusFloorLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCeilStatusFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPCeilStatusFloorLoopDeferredMask = 0xffu;
    if ((mask & 0x1ffu) == 0x1ffu)
    {
        gNdsFighterMarioFoxStageMPCeilStatusFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCeilStatusFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP_SAFE_PASS;
    }
}
