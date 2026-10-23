/* Map collision, floors and ceilings: floor projection, the floor sweeps and
 * the stage's floor loop, the ceiling collisions and line-ID queries. A
 * slice of reloc_backend.c. */

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
        if (hit != FALSE)
        {
            gNdsStageCollisionLoopP0HitCount++;
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
    if (ndsStageMPUpdateFloorLoopUpdateFighter(fighter_gobj) != FALSE)
    {
        return TRUE;
    }
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
        return TRUE;
    }
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
NDS_COLD s32 mpCollisionGetLineCountType(s32 line_type)
{
    ndsMPCollisionEnsureLineGroups();
    return gMPCollisionLineGroups[line_type].line_count;
}

NDS_COLD void mpCollisionGetLineIDsTypeCount(s32 type, s32 count, s32 *line_ids)
{
    s32 i;

    ndsMPCollisionEnsureLineGroups();
    for (i = 0; i < count; i++)
    {
        line_ids[i] = gMPCollisionLineGroups[type].line_id[i];
    }
}

NDS_COLD void mpCollisionSetBGM(void)
{
    gMPCollisionBGMCurrent = gMPCollisionBGMDefault = gMPCollisionGroundData->bgm_id;
}
