
static s32 ndsFighterDLDrawAxisCoord(const NDSFighterDLDrawVtx *vtx,
                                     u32 axis, u32 coord)
{
    if (axis == 0u)
    {
        return (coord == 0u) ? vtx->x : vtx->y;
    }
    if (axis == 1u)
    {
        return (coord == 0u) ? vtx->x : vtx->z;
    }
    return (coord == 0u) ? vtx->y : vtx->z;
}

static void ndsFighterDLDrawRecordAxisPoint(
    const NDSFighterDLDrawVtx *vtx, u32 axis, u32 *bounds_valid,
    s32 *min_a, s32 *max_a, s32 *min_b, s32 *max_b)
{
    s32 a;
    s32 b;

    if ((vtx == NULL) || (bounds_valid == NULL) || (min_a == NULL) ||
        (max_a == NULL) || (min_b == NULL) || (max_b == NULL) ||
        (vtx->valid == FALSE))
    {
        return;
    }

    a = ndsFighterDLDrawAxisCoord(vtx, axis, 0u);
    b = ndsFighterDLDrawAxisCoord(vtx, axis, 1u);
    if (*bounds_valid == 0u)
    {
        *min_a = *max_a = a;
        *min_b = *max_b = b;
        *bounds_valid = 1u;
        return;
    }
    if (a < *min_a) { *min_a = a; }
    if (a > *max_a) { *max_a = a; }
    if (b < *min_b) { *min_b = b; }
    if (b > *max_b) { *max_b = b; }
}

static u16 ndsFighterDLDrawRGB15(u8 r, u8 g, u8 b)
{
    return (u16)(0x8000u | ((u16)(r >> 3)) |
                 ((u16)(g >> 3) << 5) | ((u16)(b >> 3) << 10));
}

static s32 ndsFighterDLDrawEdge(s32 ax, s32 ay, s32 bx, s32 by,
                                s32 px, s32 py)
{
    return ((px - ax) * (by - ay)) - ((py - ay) * (bx - ax));
}

static void ndsFighterDLDrawTriangle(u16 *pixels, u32 pitch,
                                     s32 x0, s32 y0,
                                     s32 x1, s32 y1,
                                     s32 x2, s32 y2,
                                     u16 fill, u16 edge,
                                     u32 *pixel_count)
{
    s32 min_x = x0;
    s32 max_x = x0;
    s32 min_y = y0;
    s32 max_y = y0;
    s32 area;
    s32 x;
    s32 y;

    if ((pixels == NULL) || (pixel_count == NULL))
    {
        return;
    }
    if (x1 < min_x) { min_x = x1; }
    if (x2 < min_x) { min_x = x2; }
    if (x1 > max_x) { max_x = x1; }
    if (x2 > max_x) { max_x = x2; }
    if (y1 < min_y) { min_y = y1; }
    if (y2 < min_y) { min_y = y2; }
    if (y1 > max_y) { max_y = y1; }
    if (y2 > max_y) { max_y = y2; }
    if (min_x < 0) { min_x = 0; }
    if (min_y < 0) { min_y = 0; }
    if (max_x >= (s32)NDS_FIGHTER_DL_DRAW_WIDTH)
    {
        max_x = (s32)NDS_FIGHTER_DL_DRAW_WIDTH - 1;
    }
    if (max_y >= (s32)NDS_FIGHTER_DL_DRAW_HEIGHT)
    {
        max_y = (s32)NDS_FIGHTER_DL_DRAW_HEIGHT - 1;
    }

    area = ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2);
    if (area == 0)
    {
        return;
    }

    for (y = min_y; y <= max_y; y++)
    {
        for (x = min_x; x <= max_x; x++)
        {
            s32 w0 = ndsFighterDLDrawEdge(x1, y1, x2, y2, x, y);
            s32 w1 = ndsFighterDLDrawEdge(x2, y2, x0, y0, x, y);
            s32 w2 = ndsFighterDLDrawEdge(x0, y0, x1, y1, x, y);

            if (((w0 >= 0) && (w1 >= 0) && (w2 >= 0)) ||
                ((w0 <= 0) && (w1 <= 0) && (w2 <= 0)))
            {
                pixels[(y * (s32)pitch) + x] =
                    ((w0 == 0) || (w1 == 0) || (w2 == 0)) ? edge : fill;
                (*pixel_count)++;
            }
        }
    }
}

static s32 ndsFighterDLDrawMapCoord(s32 value, s32 min_value, s32 max_value,
                                    s32 out_min, s32 out_max)
{
    s32 range = max_value - min_value;
    s32 out_range = out_max - out_min;

    if (range == 0)
    {
        return out_min;
    }
    return out_min + (((value - min_value) * out_range) / range);
}

static u16 ndsFighterDLDrawTriangleColor(
    const NDSFighterDLDrawState *state, const NDSFighterDLDrawTri *tri)
{
    const NDSFighterDLDrawVtx *v0 = &state->vertices[tri->v0];
    const NDSFighterDLDrawVtx *v1 = &state->vertices[tri->v1];
    const NDSFighterDLDrawVtx *v2 = &state->vertices[tri->v2];
    u32 r = ((u32)v0->r + v1->r + v2->r) / 3u;
    u32 g = ((u32)v0->g + v1->g + v2->g) / 3u;
    u32 b = ((u32)v0->b + v1->b + v2->b) / 3u;

    if ((r == 0u) && (g == 0u) && (b == 0u))
    {
        if (state->slot == 0u)
        {
            r = 255u; g = 96u; b = 32u;
        }
        else
        {
            r = 224u; g = 255u; b = 64u;
        }
    }
    return ndsFighterDLDrawRGB15((u8)r, (u8)g, (u8)b);
}

static void ndsFighterDLDrawRasterizeState(NDSFighterDLDrawState *state,
                                           u16 *pixels, u32 pitch)
{
    u32 axis;
    u32 best_axis = 0xffffffffu;
    u32 best_area = 0u;
    u32 best_nondegenerate_count = 0u;
    u32 i;
    s32 min_a = 0;
    s32 max_a = 0;
    s32 min_b = 0;
    s32 max_b = 0;
    u32 bounds_valid = 0u;
    s32 box_min_x = (state->slot == 0u) ? 4 : 52;
    s32 box_max_x = (state->slot == 0u) ? 43 : 91;
    s32 box_min_y = 4;
    s32 box_max_y = 67;
    s32 screen_min_x = 0;
    s32 screen_max_x = 0;
    s32 screen_min_y = 0;
    s32 screen_max_y = 0;
    u32 screen_valid = 0u;
    u32 pixel_count = 0u;
    u32 drawn_count = 0u;
    u32 real_drawn_count = 0u;
    u32 marker_drawn_count = 0u;

    if ((state == NULL) || (pixels == NULL))
    {
        return;
    }

    for (axis = 0u; axis < 3u; axis++)
    {
        s32 axis_min_a = 0;
        s32 axis_max_a = 0;
        s32 axis_min_b = 0;
        s32 axis_max_b = 0;
        u32 axis_bounds_valid = 0u;
        u32 area_sum = 0u;
        u32 nondegenerate_count = 0u;

        for (i = 0u; i < state->triangle_count; i++)
        {
            const NDSFighterDLDrawTri *tri = &state->tris[i];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            if ((tri->v0 < NDS_FIGHTER_DL_DRAW_MAX_VTX) &&
                (tri->v1 < NDS_FIGHTER_DL_DRAW_MAX_VTX) &&
                (tri->v2 < NDS_FIGHTER_DL_DRAW_MAX_VTX) &&
                (state->vertices[tri->v0].valid != FALSE) &&
                (state->vertices[tri->v1].valid != FALSE) &&
                (state->vertices[tri->v2].valid != FALSE))
            {
                v0 = &state->vertices[tri->v0];
                v1 = &state->vertices[tri->v1];
                v2 = &state->vertices[tri->v2];
                ndsFighterDLDrawRecordAxisPoint(
                    v0, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v1, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v2, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
            }
        }
        if ((axis_bounds_valid == 0u) ||
            ((axis_min_a == axis_max_a) && (axis_min_b == axis_max_b)))
        {
            continue;
        }

        for (i = 0u; i < state->triangle_count; i++)
        {
            const NDSFighterDLDrawTri *tri = &state->tris[i];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            s32 x0;
            s32 y0;
            s32 x1;
            s32 y1;
            s32 x2;
            s32 y2;
            s32 area;

            if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
            {
                continue;
            }
            v0 = &state->vertices[tri->v0];
            v1 = &state->vertices[tri->v1];
            v2 = &state->vertices[tri->v2];
            if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                (v2->valid == FALSE))
            {
                continue;
            }
            x0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, axis, 0u),
                axis_min_a, axis_max_a, box_min_x, box_max_x);
            y0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, axis, 1u),
                axis_min_b, axis_max_b, box_max_y, box_min_y);
            x1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, axis, 0u),
                axis_min_a, axis_max_a, box_min_x, box_max_x);
            y1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, axis, 1u),
                axis_min_b, axis_max_b, box_max_y, box_min_y);
            x2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, axis, 0u),
                axis_min_a, axis_max_a, box_min_x, box_max_x);
            y2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, axis, 1u),
                axis_min_b, axis_max_b, box_max_y, box_min_y);

            area = ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2);
            if (area == 0)
            {
                continue;
            }
            nondegenerate_count++;
            area_sum += (area < 0) ? (u32)-area : (u32)area;
        }

        if ((best_axis > 2u) ||
            (nondegenerate_count > best_nondegenerate_count) ||
            ((nondegenerate_count == best_nondegenerate_count) &&
             (area_sum > best_area)))
        {
            best_area = area_sum;
            best_nondegenerate_count = nondegenerate_count;
            best_axis = axis;
            min_a = axis_min_a;
            max_a = axis_max_a;
            min_b = axis_min_b;
            max_b = axis_max_b;
            bounds_valid = 1u;
        }
    }

    if (best_axis > 2u)
    {
        return;
    }
    if ((bounds_valid == 0u) || ((min_a == max_a) && (min_b == max_b)))
    {
        return;
    }

    for (i = 0u; i < state->triangle_count; i++)
    {
        const NDSFighterDLDrawTri *tri = &state->tris[i];
        const NDSFighterDLDrawVtx *v0;
        const NDSFighterDLDrawVtx *v1;
        const NDSFighterDLDrawVtx *v2;
        s32 x0;
        s32 y0;
        s32 x1;
        s32 y1;
        s32 x2;
        s32 y2;
        u32 before;
        u32 marker_drawn = 0u;
        u16 fill;
        u16 edge;

        if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
            (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
            (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
        {
            continue;
        }
        v0 = &state->vertices[tri->v0];
        v1 = &state->vertices[tri->v1];
        v2 = &state->vertices[tri->v2];
        if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
            (v2->valid == FALSE))
        {
            continue;
        }

        x0 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v0, best_axis, 0u),
            min_a, max_a, box_min_x, box_max_x);
        y0 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v0, best_axis, 1u),
            min_b, max_b, box_max_y, box_min_y);
        x1 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v1, best_axis, 0u),
            min_a, max_a, box_min_x, box_max_x);
        y1 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v1, best_axis, 1u),
            min_b, max_b, box_max_y, box_min_y);
        x2 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v2, best_axis, 0u),
            min_a, max_a, box_min_x, box_max_x);
        y2 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v2, best_axis, 1u),
            min_b, max_b, box_max_y, box_min_y);
        fill = ndsFighterDLDrawTriangleColor(state, tri);
        edge = ndsFighterDLDrawRGB15(255, 255, 255);
        before = pixel_count;
        if (ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2) == 0)
        {
            s32 cx = (x0 + x1 + x2) / 3;
            s32 cy = (y0 + y1 + y2) / 3;

            ndsFighterDLDrawTriangle(pixels, pitch,
                                     cx - 4, cy - 3,
                                     cx + 4, cy - 3,
                                     cx, cy + 4,
                                     fill, edge, &pixel_count);
            marker_drawn = 1u;
            x0 = cx - 4;
            y0 = cy - 3;
            x1 = cx + 4;
            y1 = cy - 3;
            x2 = cx;
            y2 = cy + 4;
        }
        else
        {
            ndsFighterDLDrawTriangle(pixels, pitch, x0, y0, x1, y1, x2, y2,
                                     fill, edge, &pixel_count);
        }
        if (pixel_count != before)
        {
            drawn_count++;
            if (marker_drawn != 0u)
            {
                marker_drawn_count++;
            }
            else
            {
                real_drawn_count++;
            }
            if (screen_valid == 0u)
            {
                screen_min_x = screen_max_x = x0;
                screen_min_y = screen_max_y = y0;
                screen_valid = 1u;
            }
            if (x0 < screen_min_x) { screen_min_x = x0; }
            if (x1 < screen_min_x) { screen_min_x = x1; }
            if (x2 < screen_min_x) { screen_min_x = x2; }
            if (x0 > screen_max_x) { screen_max_x = x0; }
            if (x1 > screen_max_x) { screen_max_x = x1; }
            if (x2 > screen_max_x) { screen_max_x = x2; }
            if (y0 < screen_min_y) { screen_min_y = y0; }
            if (y1 < screen_min_y) { screen_min_y = y1; }
            if (y2 < screen_min_y) { screen_min_y = y2; }
            if (y0 > screen_max_y) { screen_max_y = y0; }
            if (y1 > screen_max_y) { screen_max_y = y1; }
            if (y2 > screen_max_y) { screen_max_y = y2; }
        }
    }

    if (state->slot == 0u)
    {
        gNdsFighterDLDrawP0Axis = best_axis;
        gNdsFighterDLDrawP0Area = best_area;
        gNdsFighterDLDrawP0MinA = min_a;
        gNdsFighterDLDrawP0MaxA = max_a;
        gNdsFighterDLDrawP0MinB = min_b;
        gNdsFighterDLDrawP0MaxB = max_b;
        gNdsFighterDLDrawP0ScreenMinX = screen_min_x;
        gNdsFighterDLDrawP0ScreenMaxX = screen_max_x;
        gNdsFighterDLDrawP0ScreenMinY = screen_min_y;
        gNdsFighterDLDrawP0ScreenMaxY = screen_max_y;
        gNdsFighterDLDrawP0PixelCount = pixel_count;
        gNdsFighterDLDrawP0TriangleDrawnCount = drawn_count;
        gNdsFighterDLDrawP0RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLDrawP0MarkerTriangleDrawnCount = marker_drawn_count;
    }
    else
    {
        gNdsFighterDLDrawP1Axis = best_axis;
        gNdsFighterDLDrawP1Area = best_area;
        gNdsFighterDLDrawP1MinA = min_a;
        gNdsFighterDLDrawP1MaxA = max_a;
        gNdsFighterDLDrawP1MinB = min_b;
        gNdsFighterDLDrawP1MaxB = max_b;
        gNdsFighterDLDrawP1ScreenMinX = screen_min_x;
        gNdsFighterDLDrawP1ScreenMaxX = screen_max_x;
        gNdsFighterDLDrawP1ScreenMinY = screen_min_y;
        gNdsFighterDLDrawP1ScreenMaxY = screen_max_y;
        gNdsFighterDLDrawP1PixelCount = pixel_count;
        gNdsFighterDLDrawP1TriangleDrawnCount = drawn_count;
        gNdsFighterDLDrawP1RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLDrawP1MarkerTriangleDrawnCount = marker_drawn_count;
    }
}

static void ndsFighterMarioFoxCopyDLDrawStats(
    u32 slot, const NDSFighterDLDrawState *state,
    const NDSRendererStats *stats)
{
    if ((state == NULL) || (stats == NULL))
    {
        return;
    }

    if (slot == 0u)
    {
        gNdsFighterDLDrawP0Blocker = stats->blocker;
        gNdsFighterDLDrawP0CommandCount = stats->command_count;
        gNdsFighterDLDrawP0FirstOpcode = stats->first_opcode;
        gNdsFighterDLDrawP0UnsupportedOpcode =
            (stats->unsupported_opcode != 0u) ? stats->unsupported_opcode :
                state->unsupported_opcode;
        gNdsFighterDLDrawP0UnsupportedCommandCount =
            stats->unsupported_command_count + state->unsupported_command_count;
        gNdsFighterDLDrawP0VertexDecodedCount = state->vertex_decoded_count;
        gNdsFighterDLDrawP0TriangleCount = state->triangle_count;
        gNdsFighterDLDrawP0TriangleValidCount = state->triangle_valid_count;
        gNdsFighterDLDrawP0ColorChecksum = state->color_checksum;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLDrawP1Blocker = stats->blocker;
        gNdsFighterDLDrawP1CommandCount = stats->command_count;
        gNdsFighterDLDrawP1FirstOpcode = stats->first_opcode;
        gNdsFighterDLDrawP1UnsupportedOpcode =
            (stats->unsupported_opcode != 0u) ? stats->unsupported_opcode :
                state->unsupported_opcode;
        gNdsFighterDLDrawP1UnsupportedCommandCount =
            stats->unsupported_command_count + state->unsupported_command_count;
        gNdsFighterDLDrawP1VertexDecodedCount = state->vertex_decoded_count;
        gNdsFighterDLDrawP1TriangleCount = state->triangle_count;
        gNdsFighterDLDrawP1TriangleValidCount = state->triangle_valid_count;
        gNdsFighterDLDrawP1ColorChecksum = state->color_checksum;
    }
    gNdsFighterDLDrawVertexRangeRejectCount +=
        state->vertex_range_reject_count;
}

static void ndsFighterMarioFoxDrawDLForSlot(u32 slot, FTStruct *fp,
                                            u16 *pixels, u32 pitch)
{
    DObj *root;
    DObj *selected;
    const Gfx *dl;
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config = {0};
    NDSRendererStats stats;
    NDSFighterDLDrawState state;
    NDSRendererMatrix20p12 initial_projection;
    NDSRendererMatrix20p12 initial_modelview;
    const NDSRendererMatrix20p12 *initial_projection_ptr;
    const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
    void *saved_graphics_heap_ptr;
#endif
    u32 root_x_before;
    u32 root_x_after;
    u32 unused_index;

    if ((slot > 1u) || (pixels == NULL) ||
        (ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        return;
    }

    root = fp->joints[nFTPartsJointTopN];
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    selected = ndsFighterFindFirstDObjWithDL(root, &unused_index);
    if (selected == NULL)
    {
        return;
    }

    dl = selected->dl;
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
        return;
    }

    bzero(&state, sizeof(state));
    state.primary_file = loaded;
    state.slot = slot;
#if NDS_RENDERER_HW_TRIANGLES
    saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
    ndsRendererAdapterPrepareMaterialSegment(selected, &state);
#endif
    ndsRendererAdapterPrepareInitialMatrices(selected,
                                             (gGCCurrentCamera != NULL) ?
                                                 CObjGetStruct(
                                                     gGCCurrentCamera) :
                                                 NULL,
                                             FALSE,
                                             &initial_projection,
                                             &initial_projection_ptr,
                                             &initial_modelview,
                                             &initial_modelview_ptr);

    config.max_depth = 8u;
    config.max_commands = 2048u;
    config.max_list_commands = 512u;
    config.initial_projection = initial_projection_ptr;
    config.initial_modelview = initial_modelview_ptr;
    config.initial_geometry_mode = 0u;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsFighterDLDrawValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLDrawResolveBranch;
    config.resolve_data = ndsFighterDLDrawResolveRendererData;
    config.user = &state;

    ndsRendererInitStats(&stats);
    ndsRendererExecuteDisplayList(dl,
                                  &config,
                                  ndsFighterMarioFoxVisitDLDrawCommand,
                                  &state,
                                  &stats);
#if NDS_RENDERER_HW_TRIANGLES
    ndsTaskmanSampleGraphicsHeap();
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
#endif
    ndsFighterMarioFoxCopyDLDrawStats(slot, &state, &stats);
    if ((stats.blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (state.unsupported_command_count == 0u) &&
        (state.vertex_range_reject_count == 0u))
    {
        ndsFighterDLDrawRasterizeState(&state, pixels, pitch);
    }

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    if (slot == 0u)
    {
        gNdsFighterDLDrawP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLDrawP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLDrawP0GAAfter = (u32)fp->ga;
        gNdsFighterDLDrawP0RootXBeforeBits = root_x_before;
        gNdsFighterDLDrawP0RootXAfterBits = root_x_after;
    }
    else
    {
        gNdsFighterDLDrawP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLDrawP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLDrawP1GAAfter = (u32)fp->ga;
        gNdsFighterDLDrawP1RootXBeforeBits = root_x_before;
        gNdsFighterDLDrawP1RootXAfterBits = root_x_after;
    }

    gNdsFighterMarioFoxDLDrawCount++;
}

static void ndsFighterMarioFoxRunDLDrawProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 pitch = 0u;
    u16 *pixels;

    if ((ndsFighterMarioFoxDLDrawProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLDrawResult != 0u))
    {
        return;
    }

    if ((gNdsFighterMarioFoxDLExecResult ==
            NDS_FIGHTER_MARIOFOX_DL_EXEC_PASS) &&
        (gNdsFighterMarioFoxDLExecSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_EXEC_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLExecMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLExecCount == 2u) &&
        (gNdsFighterDLExecP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLExecP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLExecP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLExecP1UnsupportedCommandCount == 0u))
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLDrawMask = mask;
        return;
    }

    gNdsFighterDLDrawPreviewCommitBefore = gNdsOriginalDLPreviewCommitCount;
    pixels = ndsPlatformBeginOriginalDLPreview(NDS_FIGHTER_DL_DRAW_WIDTH,
                                               NDS_FIGHTER_DL_DRAW_HEIGHT,
                                               &pitch);
    if (pixels != NULL)
    {
        gNdsFighterDLDrawPreviewWidth = NDS_FIGHTER_DL_DRAW_WIDTH;
        gNdsFighterDLDrawPreviewHeight = NDS_FIGHTER_DL_DRAW_HEIGHT;
        gNdsFighterDLDrawPreviewPitch = pitch;
        mask |= 1u << 1;
    }
    else
    {
        gNdsFighterMarioFoxDLDrawMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();
    ndsFighterMarioFoxDrawDLForSlot(0u, &sNdsFighterStructPool[0],
                                    pixels, pitch);
    ndsFighterMarioFoxDrawDLForSlot(1u, &sNdsFighterStructPool[1],
                                    pixels, pitch);
    if (gNdsFighterMarioFoxDLDrawCount == 2u)
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterDLDrawP0VertexDecodedCount > 0u) &&
        (gNdsFighterDLDrawP1VertexDecodedCount > 0u) &&
        (gNdsFighterDLDrawP0TriangleCount > 0u) &&
        (gNdsFighterDLDrawP1TriangleCount > 0u) &&
        (gNdsFighterDLDrawP0TriangleValidCount > 0u) &&
        (gNdsFighterDLDrawP1TriangleValidCount > 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLDrawP0Axis <= 2u) &&
        (gNdsFighterDLDrawP1Axis <= 2u) &&
        (gNdsFighterDLDrawP0Area > 0u) &&
        (gNdsFighterDLDrawP1Area > 0u) &&
        ((gNdsFighterDLDrawP0MaxA != gNdsFighterDLDrawP0MinA) ||
         (gNdsFighterDLDrawP0MaxB != gNdsFighterDLDrawP0MinB)) &&
        ((gNdsFighterDLDrawP1MaxA != gNdsFighterDLDrawP1MinA) ||
         (gNdsFighterDLDrawP1MaxB != gNdsFighterDLDrawP1MinB)))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLDrawP0RealTriangleDrawnCount > 0u) &&
        (gNdsFighterDLDrawP1RealTriangleDrawnCount > 0u))
    {
        mask |= 1u << 5;
    }

    gNdsFighterDLDrawTotalPixelCount =
        gNdsFighterDLDrawP0PixelCount + gNdsFighterDLDrawP1PixelCount;
    if ((gNdsFighterDLDrawP0PixelCount > 0u) &&
        (gNdsFighterDLDrawP1PixelCount > 0u) &&
        (gNdsFighterDLDrawTotalPixelCount > 0u) &&
        (gNdsFighterDLDrawP0ColorChecksum != 0u) &&
        (gNdsFighterDLDrawP1ColorChecksum != 0u))
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterDLDrawTotalPixelCount > 0u)
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterDLDrawPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterDLDrawPreviewCommitDelta =
            gNdsFighterDLDrawPreviewCommitAfter -
            gNdsFighterDLDrawPreviewCommitBefore;
        gNdsFighterDLDrawPreviewReady = gNdsOriginalDLPreviewReady;
    }
    if ((gNdsFighterDLDrawPreviewReady != 0u) &&
        (gNdsFighterDLDrawPreviewCommitDelta == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterDLDrawP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLDrawP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLDrawP1UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLDrawRangeRejectCount == 0u) &&
        (gNdsFighterDLDrawVertexRangeRejectCount == 0u))
    {
        mask |= 1u << 8;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLDrawGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterDLDrawP0StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLDrawP1StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLDrawP0MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLDrawP1MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLDrawP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLDrawP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLDrawP0RootXBeforeBits ==
            gNdsFighterDLDrawP0RootXAfterBits) &&
        (gNdsFighterDLDrawP1RootXBeforeBits ==
            gNdsFighterDLDrawP1RootXAfterBits) &&
        (gNdsFighterDLDrawGObjDelta == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterDLDrawDrawCallCount == 0u) &&
        (gNdsFighterDLDrawMatrixCallCount == 0u) &&
        (gNdsFighterDLDrawGameplayUpdateCount == 0u) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLDrawMask = mask;
    gNdsFighterMarioFoxDLDrawDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLDrawResult =
            NDS_FIGHTER_MARIOFOX_DL_DRAW_PASS;
        gNdsFighterMarioFoxDLDrawSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_DRAW_SAFE_PASS;
    }
}

typedef struct NDSFighterDLMultiDrawCollection {
    DObj *dobjs[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 indices[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 total_count;
    u32 selected_count;
    u32 selected_index_mask;
} NDSFighterDLMultiDrawCollection;

static void ndsFighterCollectDObjsWithDLRecursive(
    DObj *dobj, NDSFighterDLMultiDrawCollection *collection,
    u32 *traversal_index)
{
    while (dobj != NULL)
    {
        u32 current_index = (traversal_index != NULL) ? *traversal_index : 0u;

        if (traversal_index != NULL)
        {
            (*traversal_index)++;
        }

        if ((collection != NULL) && (dobj->dl != NULL))
        {
            if (collection->selected_count <
                NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED)
            {
                u32 selected = collection->selected_count;
                collection->dobjs[selected] = dobj;
                collection->indices[selected] = current_index;
                collection->selected_count++;
                if (current_index < 32u)
                {
                    collection->selected_index_mask |= 1u << current_index;
                }
            }
            collection->total_count++;
        }

        ndsFighterCollectDObjsWithDLRecursive(dobj->child,
                                              collection,
                                              traversal_index);
        dobj = dobj->sib_next;
    }
}

static void ndsFighterCollectDObjsWithDL(
    DObj *root, NDSFighterDLMultiDrawCollection *collection)
{
    u32 traversal_index = 0u;

    if (collection == NULL)
    {
        return;
    }
    bzero(collection, sizeof(*collection));
    ndsFighterCollectDObjsWithDLRecursive(root, collection,
                                          &traversal_index);
}

static s32 ndsFighterDLMultiDrawValidateRange(const Gfx *dl, size_t bytes,
                                              void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        gNdsFighterDLMultiDrawRangeRejectCount++;
        return FALSE;
    }
    return TRUE;
}

static const Gfx *ndsFighterDLMultiDrawResolveBranch(const Gfx *dl,
                                                     u32 *resolve_kind,
                                                     void *user)
{
    return ndsFighterDLDrawResolveBranch(dl, resolve_kind, user);
}

static void ndsFighterDLMultiDrawRecordScreenPoint(
    u32 slot, s32 x, s32 y, u32 *screen_valid,
    s32 *screen_min_x, s32 *screen_max_x,
    s32 *screen_min_y, s32 *screen_max_y)
{
    if ((screen_valid == NULL) || (screen_min_x == NULL) ||
        (screen_max_x == NULL) || (screen_min_y == NULL) ||
        (screen_max_y == NULL))
    {
        return;
    }

    (void)slot;
    if (x < 0) { x = 0; }
    if (x >= (s32)NDS_FIGHTER_DL_DRAW_WIDTH)
    {
        x = (s32)NDS_FIGHTER_DL_DRAW_WIDTH - 1;
    }
    if (y < 0) { y = 0; }
    if (y >= (s32)NDS_FIGHTER_DL_DRAW_HEIGHT)
    {
        y = (s32)NDS_FIGHTER_DL_DRAW_HEIGHT - 1;
    }

    if (*screen_valid == 0u)
    {
        *screen_min_x = *screen_max_x = x;
        *screen_min_y = *screen_max_y = y;
        *screen_valid = 1u;
        return;
    }
    if (x < *screen_min_x) { *screen_min_x = x; }
    if (x > *screen_max_x) { *screen_max_x = x; }
    if (y < *screen_min_y) { *screen_min_y = y; }
    if (y > *screen_max_y) { *screen_max_y = y; }
}

static void ndsFighterDLMultiDrawRasterizeStates(
    u32 slot, NDSFighterDLDrawState *states, const u8 *clean,
    u32 selected_count, u16 *pixels, u32 pitch)
{
    u32 axis;
    u32 best_axis = 0xffffffffu;
    u32 best_area = 0u;
    u32 best_nondegenerate_count = 0u;
    u32 i;
    s32 min_a = 0;
    s32 max_a = 0;
    s32 min_b = 0;
    s32 max_b = 0;
    u32 bounds_valid = 0u;
    s32 box_min_x = (slot == 0u) ? 4 : 52;
    s32 box_max_x = (slot == 0u) ? 43 : 91;
    s32 box_min_y = 4;
    s32 box_max_y = 67;
    s32 screen_min_x = 0;
    s32 screen_max_x = 0;
    s32 screen_min_y = 0;
    s32 screen_max_y = 0;
    u32 screen_valid = 0u;
    u32 pixel_count = 0u;
    u32 drawn_count = 0u;
    u32 real_drawn_count = 0u;
    u32 marker_drawn_count = 0u;
    u32 drawn_dobj_count = 0u;

    if ((states == NULL) || (clean == NULL) || (pixels == NULL))
    {
        return;
    }

    for (axis = 0u; axis < 3u; axis++)
    {
        s32 axis_min_a = 0;
        s32 axis_max_a = 0;
        s32 axis_min_b = 0;
        s32 axis_max_b = 0;
        u32 axis_bounds_valid = 0u;
        u32 area_sum = 0u;
        u32 nondegenerate_count = 0u;

        for (i = 0u; i < selected_count; i++)
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
                ndsFighterDLDrawRecordAxisPoint(
                    v0, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v1, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v2, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
            }
        }
        if ((axis_bounds_valid == 0u) ||
            ((axis_min_a == axis_max_a) && (axis_min_b == axis_max_b)))
        {
            continue;
        }

        for (i = 0u; i < selected_count; i++)
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
                s32 area;

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
                x0 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v0, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y0 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v0, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);
                x1 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v1, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y1 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v1, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);
                x2 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v2, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y2 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v2, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);

                area = ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2);
                if (area == 0)
                {
                    continue;
                }
                nondegenerate_count++;
                area_sum += (area < 0) ? (u32)-area : (u32)area;
            }
        }

        if ((best_axis > 2u) ||
            (nondegenerate_count > best_nondegenerate_count) ||
            ((nondegenerate_count == best_nondegenerate_count) &&
             (area_sum > best_area)))
        {
            best_area = area_sum;
            best_nondegenerate_count = nondegenerate_count;
            best_axis = axis;
            min_a = axis_min_a;
            max_a = axis_max_a;
            min_b = axis_min_b;
            max_b = axis_max_b;
            bounds_valid = 1u;
        }
    }

    if (best_axis > 2u)
    {
        return;
    }
    if ((bounds_valid == 0u) || ((min_a == max_a) && (min_b == max_b)))
    {
        return;
    }

    for (i = 0u; i < selected_count; i++)
    {
        u32 tri_index;
        u32 state_drawn = 0u;

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
            u32 before;
            u32 marker_drawn = 0u;
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

            x0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            x1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            x2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            fill = ndsFighterDLDrawTriangleColor(&states[i], tri);
            edge = ndsFighterDLDrawRGB15(255, 255, 255);
            before = pixel_count;
            if (ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2) == 0)
            {
                s32 cx = (x0 + x1 + x2) / 3;
                s32 cy = (y0 + y1 + y2) / 3;

                ndsFighterDLDrawTriangle(pixels, pitch,
                                         cx - 4, cy - 3,
                                         cx + 4, cy - 3,
                                         cx, cy + 4,
                                         fill, edge, &pixel_count);
                marker_drawn = 1u;
                x0 = cx - 4;
                y0 = cy - 3;
                x1 = cx + 4;
                y1 = cy - 3;
                x2 = cx;
                y2 = cy + 4;
            }
            else
            {
                ndsFighterDLDrawTriangle(pixels, pitch,
                                         x0, y0, x1, y1, x2, y2,
                                         fill, edge, &pixel_count);
            }
            if (pixel_count != before)
            {
                drawn_count++;
                if (marker_drawn != 0u)
                {
                    marker_drawn_count++;
                }
                else
                {
                    real_drawn_count++;
                }
                state_drawn = 1u;
                ndsFighterDLMultiDrawRecordScreenPoint(
                    slot, x0, y0, &screen_valid, &screen_min_x,
                    &screen_max_x, &screen_min_y, &screen_max_y);
                ndsFighterDLMultiDrawRecordScreenPoint(
                    slot, x1, y1, &screen_valid, &screen_min_x,
                    &screen_max_x, &screen_min_y, &screen_max_y);
                ndsFighterDLMultiDrawRecordScreenPoint(
                    slot, x2, y2, &screen_valid, &screen_min_x,
                    &screen_max_x, &screen_min_y, &screen_max_y);
            }
        }
        if (state_drawn != 0u)
        {
            drawn_dobj_count++;
        }
    }

    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0Axis = best_axis;
        gNdsFighterDLMultiDrawP0Area = best_area;
        gNdsFighterDLMultiDrawP0MinA = min_a;
        gNdsFighterDLMultiDrawP0MaxA = max_a;
        gNdsFighterDLMultiDrawP0MinB = min_b;
        gNdsFighterDLMultiDrawP0MaxB = max_b;
        gNdsFighterDLMultiDrawP0ScreenMinX = screen_min_x;
        gNdsFighterDLMultiDrawP0ScreenMaxX = screen_max_x;
        gNdsFighterDLMultiDrawP0ScreenMinY = screen_min_y;
        gNdsFighterDLMultiDrawP0ScreenMaxY = screen_max_y;
        gNdsFighterDLMultiDrawP0PixelCount = pixel_count;
        gNdsFighterDLMultiDrawP0TriangleDrawnCount = drawn_count;
        gNdsFighterDLMultiDrawP0RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLMultiDrawP0MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLMultiDrawP0DrawnDObjCount = drawn_dobj_count;
    }
    else
    {
        gNdsFighterDLMultiDrawP1Axis = best_axis;
        gNdsFighterDLMultiDrawP1Area = best_area;
        gNdsFighterDLMultiDrawP1MinA = min_a;
        gNdsFighterDLMultiDrawP1MaxA = max_a;
        gNdsFighterDLMultiDrawP1MinB = min_b;
        gNdsFighterDLMultiDrawP1MaxB = max_b;
        gNdsFighterDLMultiDrawP1ScreenMinX = screen_min_x;
        gNdsFighterDLMultiDrawP1ScreenMaxX = screen_max_x;
        gNdsFighterDLMultiDrawP1ScreenMinY = screen_min_y;
        gNdsFighterDLMultiDrawP1ScreenMaxY = screen_max_y;
        gNdsFighterDLMultiDrawP1PixelCount = pixel_count;
        gNdsFighterDLMultiDrawP1TriangleDrawnCount = drawn_count;
        gNdsFighterDLMultiDrawP1RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLMultiDrawP1MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLMultiDrawP1DrawnDObjCount = drawn_dobj_count;
    }
}

static void ndsFighterDLMultiDrawAccumulateStats(
    u32 slot, u32 selected_index, const NDSFighterDLDrawState *state,
    const NDSRendererStats *stats, u8 *clean)
{
    u32 blocker = (stats != NULL) ? stats->blocker : 0xffffffffu;
    u32 unsupported_opcode = 0u;
    u32 unsupported_count = 0u;
    u32 clean_selected;

    if ((state == NULL) || (stats == NULL) || (clean == NULL))
    {
        return;
    }

    unsupported_opcode = (stats->unsupported_opcode != 0u) ?
        stats->unsupported_opcode : state->unsupported_opcode;
    unsupported_count = stats->unsupported_command_count +
        state->unsupported_command_count;
    clean_selected =
        (blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (unsupported_opcode == 0u) &&
        (unsupported_count == 0u) &&
        (state->vertex_range_reject_count == 0u) &&
        (state->vertex_decoded_count != 0u) &&
        (state->triangle_valid_count != 0u);
    clean[selected_index] = (u8)clean_selected;

    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0AttemptCount++;
        if (clean_selected != FALSE)
        {
            gNdsFighterDLMultiDrawP0CleanCount++;
        }
        else
        {
            gNdsFighterDLMultiDrawP0FailedCount++;
        }
        if ((blocker != NDS_RENDERER_BLOCKER_NONE) &&
            (gNdsFighterDLMultiDrawP0FirstBlocker == 0u))
        {
            gNdsFighterDLMultiDrawP0FirstBlocker = blocker;
        }
        if (blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsFighterDLMultiDrawP0BlockerMask |=
                1u << (selected_index & 31u);
        }
        gNdsFighterDLMultiDrawP0CommandCount += stats->command_count;
        if (gNdsFighterDLMultiDrawP0FirstOpcode == 0u)
        {
            gNdsFighterDLMultiDrawP0FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLMultiDrawP0UnsupportedOpcode == 0u))
        {
            gNdsFighterDLMultiDrawP0UnsupportedOpcode = unsupported_opcode;
        }
        gNdsFighterDLMultiDrawP0UnsupportedCommandCount +=
            unsupported_count;
        gNdsFighterDLMultiDrawP0VertexDecodedCount +=
            state->vertex_decoded_count;
        gNdsFighterDLMultiDrawP0TriangleCount += state->triangle_count;
        gNdsFighterDLMultiDrawP0TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLMultiDrawP0ColorChecksum =
            (gNdsFighterDLMultiDrawP0ColorChecksum * 33u) ^
            state->color_checksum;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLMultiDrawP1AttemptCount++;
        if (clean_selected != FALSE)
        {
            gNdsFighterDLMultiDrawP1CleanCount++;
        }
        else
        {
            gNdsFighterDLMultiDrawP1FailedCount++;
        }
        if ((blocker != NDS_RENDERER_BLOCKER_NONE) &&
            (gNdsFighterDLMultiDrawP1FirstBlocker == 0u))
        {
            gNdsFighterDLMultiDrawP1FirstBlocker = blocker;
        }
        if (blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsFighterDLMultiDrawP1BlockerMask |=
                1u << (selected_index & 31u);
        }
        gNdsFighterDLMultiDrawP1CommandCount += stats->command_count;
        if (gNdsFighterDLMultiDrawP1FirstOpcode == 0u)
        {
            gNdsFighterDLMultiDrawP1FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLMultiDrawP1UnsupportedOpcode == 0u))
        {
            gNdsFighterDLMultiDrawP1UnsupportedOpcode = unsupported_opcode;
        }
        gNdsFighterDLMultiDrawP1UnsupportedCommandCount +=
            unsupported_count;
        gNdsFighterDLMultiDrawP1VertexDecodedCount +=
            state->vertex_decoded_count;
        gNdsFighterDLMultiDrawP1TriangleCount += state->triangle_count;
        gNdsFighterDLMultiDrawP1TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLMultiDrawP1ColorChecksum =
            (gNdsFighterDLMultiDrawP1ColorChecksum * 33u) ^
            state->color_checksum;
    }

    gNdsFighterDLMultiDrawVertexRangeRejectCount +=
        state->vertex_range_reject_count;
}

static void ndsFighterMarioFoxDLMultiDrawForSlot(u32 slot, FTStruct *fp,
                                                 u16 *pixels, u32 pitch)
{
    DObj *root;
    NDSFighterDLMultiDrawCollection collection;
    NDSFighterDLDrawState states[
        NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    NDSFighterDLDrawState persistent_state;
    NDSRendererStats stats;
    NDSRendererStats persistent_stats;
    u8 clean[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 root_x_before;
    u32 root_x_after;
    u32 i;

    if ((slot > 1u) || (pixels == NULL) ||
        (ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        return;
    }

    root = fp->joints[nFTPartsJointTopN];
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;

    ndsFighterCollectDObjsWithDL(root, &collection);
    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0CandidateCount = collection.total_count;
        gNdsFighterDLMultiDrawP0SelectedCount = collection.selected_count;
        gNdsFighterDLMultiDrawP0SelectedIndexMask =
            collection.selected_index_mask;
    }
    else
    {
        gNdsFighterDLMultiDrawP1CandidateCount = collection.total_count;
        gNdsFighterDLMultiDrawP1SelectedCount = collection.selected_count;
        gNdsFighterDLMultiDrawP1SelectedIndexMask =
            collection.selected_index_mask;
    }

    bzero(states, sizeof(states));
    bzero(&persistent_state, sizeof(persistent_state));
    bzero(&stats, sizeof(stats));
    ndsRendererInitStats(&persistent_stats);
    bzero(clean, sizeof(clean));

    for (i = 0u; i < collection.selected_count; i++)
    {
        const Gfx *dl = collection.dobjs[i]->dl;
        NDSRelocLoadedFile *loaded =
            ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
        NDSRendererConfig config = {0};
        NDSRendererMatrix20p12 initial_projection;
        NDSRendererMatrix20p12 initial_modelview;
        const NDSRendererMatrix20p12 *initial_projection_ptr;
        const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
        void *saved_graphics_heap_ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        u32 step_start;
#endif
#endif

        if ((loaded == NULL) &&
            (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
        {
            continue;
        }

        states[i].primary_file = loaded;
        states[i].slot = slot;
        ndsFighterDLDrawSeedPersistentState(&states[i],
                                            &persistent_state);
#if NDS_RENDERER_HW_TRIANGLES
        saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        step_start = cpuGetTiming();
#endif
        ndsRendererAdapterPrepareMaterialSegment(collection.dobjs[i],
                                                 &states[i]);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileMaterialTicks += cpuGetTiming() - step_start;
        step_start = cpuGetTiming();
#endif
#endif
        ndsRendererAdapterPrepareInitialMatrices(collection.dobjs[i],
                                                 (gGCCurrentCamera != NULL) ?
                                                     CObjGetStruct(
                                                         gGCCurrentCamera) :
                                                     NULL,
                                                 FALSE,
                                                 &initial_projection,
                                                 &initial_projection_ptr,
                                                 &initial_modelview,
                                                 &initial_modelview_ptr);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        gNdsRendererProfileMatrixTicks += cpuGetTiming() - step_start;
#endif
        config.max_depth = 8u;
        config.max_commands = 2048u;
        config.max_list_commands = 512u;
        config.initial_projection = initial_projection_ptr;
        config.initial_modelview = initial_modelview_ptr;
        config.initial_geometry_mode = 0u;
        config.texture_data_layout =
            NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
        config.validate_range = ndsFighterDLMultiDrawValidateRange;
        config.immutable_command_span =
            ndsRendererAdapterImmutableCommandSpan;
        config.resolve_branch = ndsFighterDLMultiDrawResolveBranch;
        config.resolve_data = ndsFighterDLDrawResolveRendererData;
        config.user = &states[i];

        ndsRendererInitStats(&stats);
        ndsFighterDLDrawCopyPersistentRendererState(&stats,
                                                    &persistent_stats);
        ndsRendererExecuteDisplayList(dl,
                                      &config,
                                      ndsFighterMarioFoxVisitDLDrawCommand,
                                      &states[i],
                                      &stats);
#if NDS_RENDERER_HW_TRIANGLES
        gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
#endif
        ndsFighterDLDrawCapturePersistentState(&persistent_state,
                                               &states[i]);
        ndsFighterDLDrawCopyPersistentRendererState(&persistent_stats,
                                                    &stats);
        ndsFighterDLMultiDrawAccumulateStats(slot, i, &states[i],
                                             &stats, clean);
    }

    ndsFighterDLMultiDrawRasterizeStates(slot, states, clean,
                                         collection.selected_count,
                                         pixels,
                                         pitch);

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLMultiDrawP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLMultiDrawP0GAAfter = (u32)fp->ga;
        gNdsFighterDLMultiDrawP0RootXBeforeBits = root_x_before;
        gNdsFighterDLMultiDrawP0RootXAfterBits = root_x_after;
    }
    else
    {
        gNdsFighterDLMultiDrawP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLMultiDrawP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLMultiDrawP1GAAfter = (u32)fp->ga;
        gNdsFighterDLMultiDrawP1RootXBeforeBits = root_x_before;
        gNdsFighterDLMultiDrawP1RootXAfterBits = root_x_after;
    }

    gNdsFighterMarioFoxDLMultiDrawCount++;
}

static void ndsFighterMarioFoxRunDLMultiDrawProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 pitch = 0u;
    u16 *pixels;

    if ((ndsFighterMarioFoxDLMultiDrawProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLMultiDrawResult != 0u))
    {
        return;
    }

    if (
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
        (gNdsFighterManagerResult == NDS_FIGHTER_MANAGER_PASS) &&
        ((gNdsFighterManagerWaitMask & 0x3u) == 0x3u)
#else
        (gNdsFighterMarioFoxDLDrawResult ==
            NDS_FIGHTER_MARIOFOX_DL_DRAW_PASS) &&
        (gNdsFighterMarioFoxDLDrawSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_DRAW_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLDrawMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLDrawCount == 2u) &&
        (gNdsFighterDLDrawP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLDrawP1UnsupportedCommandCount == 0u)
#endif
        )
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLMultiDrawMask = mask;
        return;
    }

    gNdsFighterDLMultiDrawPreviewCommitBefore =
        gNdsOriginalDLPreviewCommitCount;
    pixels = ndsPlatformBeginOriginalDLPreview(
        NDS_FIGHTER_DL_MULTI_DRAW_WIDTH,
        NDS_FIGHTER_DL_MULTI_DRAW_HEIGHT,
        &pitch);
    if (pixels != NULL)
    {
        gNdsFighterDLMultiDrawPreviewWidth =
            NDS_FIGHTER_DL_MULTI_DRAW_WIDTH;
        gNdsFighterDLMultiDrawPreviewHeight =
            NDS_FIGHTER_DL_MULTI_DRAW_HEIGHT;
        gNdsFighterDLMultiDrawPreviewPitch = pitch;
        mask |= 1u << 1;
    }
    else
    {
        gNdsFighterMarioFoxDLMultiDrawMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();
    ndsFighterMarioFoxDLMultiDrawForSlot(
        0u, ndsFighterMarioFoxProofStructForSlot(0u), pixels, pitch);
    ndsFighterMarioFoxDLMultiDrawForSlot(
        1u, ndsFighterMarioFoxProofStructForSlot(1u), pixels, pitch);
    if (gNdsFighterMarioFoxDLMultiDrawCount == 2u)
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterDLMultiDrawP0CandidateCount == 14u) &&
        (gNdsFighterDLMultiDrawP1CandidateCount == 18u) &&
        (gNdsFighterDLMultiDrawP0SelectedCount ==
            NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED) &&
        (gNdsFighterDLMultiDrawP1SelectedCount ==
            NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED) &&
        (gNdsFighterDLMultiDrawP0SelectedIndexMask != 0u) &&
        (gNdsFighterDLMultiDrawP1SelectedIndexMask != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLMultiDrawP0AttemptCount == 4u) &&
        (gNdsFighterDLMultiDrawP1AttemptCount == 4u) &&
        (gNdsFighterDLMultiDrawP0CleanCount == 4u) &&
        (gNdsFighterDLMultiDrawP1CleanCount == 4u) &&
        (gNdsFighterDLMultiDrawP0FailedCount == 0u) &&
        (gNdsFighterDLMultiDrawP1FailedCount == 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLMultiDrawP0VertexDecodedCount >
            gNdsFighterDLDrawP0VertexDecodedCount) &&
        (gNdsFighterDLMultiDrawP1VertexDecodedCount >
            gNdsFighterDLDrawP1VertexDecodedCount) &&
        (gNdsFighterDLMultiDrawP0TriangleCount >
            gNdsFighterDLDrawP0TriangleCount) &&
        (gNdsFighterDLMultiDrawP1TriangleCount >
            gNdsFighterDLDrawP1TriangleCount) &&
        (gNdsFighterDLMultiDrawP0TriangleValidCount >
            gNdsFighterDLDrawP0TriangleValidCount) &&
        (gNdsFighterDLMultiDrawP1TriangleValidCount >
            gNdsFighterDLDrawP1TriangleValidCount) &&
        (gNdsFighterDLMultiDrawP0RealTriangleDrawnCount >
            gNdsFighterDLDrawP0RealTriangleDrawnCount) &&
        (gNdsFighterDLMultiDrawP1RealTriangleDrawnCount >
            gNdsFighterDLDrawP1RealTriangleDrawnCount))
    {
        mask |= 1u << 5;
    }

    gNdsFighterDLMultiDrawTotalPixelCount =
        gNdsFighterDLMultiDrawP0PixelCount +
        gNdsFighterDLMultiDrawP1PixelCount;
    if ((gNdsFighterDLMultiDrawP0PixelCount >=
            gNdsFighterDLDrawP0PixelCount) &&
        (gNdsFighterDLMultiDrawP1PixelCount >=
            gNdsFighterDLDrawP1PixelCount) &&
        (gNdsFighterDLMultiDrawTotalPixelCount >=
            gNdsFighterDLDrawTotalPixelCount) &&
        (gNdsFighterDLMultiDrawP0ColorChecksum != 0u) &&
        (gNdsFighterDLMultiDrawP1ColorChecksum != 0u))
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterDLMultiDrawTotalPixelCount > 0u)
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterDLMultiDrawPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterDLMultiDrawPreviewCommitDelta =
            gNdsFighterDLMultiDrawPreviewCommitAfter -
            gNdsFighterDLMultiDrawPreviewCommitBefore;
        gNdsFighterDLMultiDrawPreviewReady = gNdsOriginalDLPreviewReady;
    }
    if ((gNdsFighterDLMultiDrawPreviewReady != 0u) &&
        (gNdsFighterDLMultiDrawPreviewCommitDelta == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterDLMultiDrawP0FirstBlocker ==
            NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLMultiDrawP1FirstBlocker ==
            NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLMultiDrawP0BlockerMask == 0u) &&
        (gNdsFighterDLMultiDrawP1BlockerMask == 0u) &&
        (gNdsFighterDLMultiDrawP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLMultiDrawP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLMultiDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLMultiDrawP1UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLMultiDrawRangeRejectCount == 0u) &&
        (gNdsFighterDLMultiDrawVertexRangeRejectCount == 0u))
    {
        mask |= 1u << 8;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLMultiDrawGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterDLMultiDrawP0StatusAfter ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLMultiDrawP1StatusAfter ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLMultiDrawP0MotionAfter ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLMultiDrawP1MotionAfter ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLMultiDrawP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLMultiDrawP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLMultiDrawP0RootXBeforeBits ==
            gNdsFighterDLMultiDrawP0RootXAfterBits) &&
        (gNdsFighterDLMultiDrawP1RootXBeforeBits ==
            gNdsFighterDLMultiDrawP1RootXAfterBits) &&
        (gNdsFighterDLMultiDrawGObjDelta == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterDLMultiDrawDrawCallCount == 0u) &&
        (gNdsFighterDLMultiDrawMatrixCallCount == 0u) &&
        (gNdsFighterDLMultiDrawGameplayUpdateCount == 0u) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLMultiDrawMask = mask;
    gNdsFighterMarioFoxDLMultiDrawDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLMultiDrawResult =
            NDS_FIGHTER_MARIOFOX_DL_MULTI_DRAW_PASS;
        gNdsFighterMarioFoxDLMultiDrawSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_MULTI_DRAW_SAFE_PASS;
    }
}

typedef struct NDSFighterDLAllDrawCollection {
    DObj *dobjs[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 indices[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 total_count;
    u32 selected_count;
    u32 selected_index_mask;
} NDSFighterDLAllDrawCollection;

#if NDS_TICK_HUD
/* CYCLE 98 -- does the DObj walk rebuild the same collection every frame?
 *
 * The board's seam table calls the walk "match-load constant for Mario/Fox --
 * poses move, topology does not", and flags that it needs proof no status or
 * motion alters the DObj set. This is that proof, and it costs one u32 of state
 * per slot: hash the collection's identity and compare it with the previous
 * frame's hash for the same slot.
 *
 * The dl pointer is in the hash deliberately. A collection whose DObj set is
 * unchanged but whose display lists have been re-pointed is NOT a constant for
 * any purpose a baked collection order would serve, and hashing only the DObj
 * pointers would report it as one. */
static u32 sNdsFtrPreWalkHash[GMCOMMON_PLAYERS_MAX];
static u32 sNdsFtrPreWalkSeen[GMCOMMON_PLAYERS_MAX];

static void ndsFtrPreWalkCensus(
    u32 slot, const NDSFighterDLAllDrawCollection *collection)
{
    u32 hash = 2166136261u;
    u32 i;

    if ((slot >= GMCOMMON_PLAYERS_MAX) || (collection == NULL))
    {
        return;
    }
#define NDS_FTR_PRE_MIX(v) \
    do { hash ^= (u32)(v); hash *= 16777619u; } while (0)
    NDS_FTR_PRE_MIX(collection->total_count);
    NDS_FTR_PRE_MIX(collection->selected_count);
    NDS_FTR_PRE_MIX(collection->selected_index_mask);
    for (i = 0u; (i < collection->selected_count) &&
                 (i < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED); i++)
    {
        const DObj *dobj = collection->dobjs[i];

        NDS_FTR_PRE_MIX(collection->indices[i]);
        NDS_FTR_PRE_MIX((uintptr_t)dobj);
        NDS_FTR_PRE_MIX((dobj != NULL) ? (uintptr_t)dobj->dl : 0u);
    }
#undef NDS_FTR_PRE_MIX
    if (sNdsFtrPreWalkSeen[slot] == 0u)
    {
        gNdsFtrPreWalkFirst++;
    }
    else if (sNdsFtrPreWalkHash[slot] == hash)
    {
        gNdsFtrPreWalkSame++;
    }
    else
    {
        gNdsFtrPreWalkVariant++;
    }
    sNdsFtrPreWalkHash[slot] = hash;
    sNdsFtrPreWalkSeen[slot] = 1u;
}
#endif

#define NDS_FIGHTER_DL_ALL_FAIL_BLOCKER 0x1u
#define NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_OPCODE 0x2u
#define NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_COUNT 0x4u
#define NDS_FIGHTER_DL_ALL_FAIL_VERTEX_RANGE 0x8u
#define NDS_FIGHTER_DL_ALL_FAIL_NO_VERTS 0x10u
#define NDS_FIGHTER_DL_ALL_FAIL_NO_VALID_TRIS 0x20u
#define NDS_FIGHTER_DL_ALL_FAIL_UNKNOWN 0x80000000u

static void ndsFighterCollectAllDObjsWithDLRecursive(
    DObj *dobj, NDSFighterDLAllDrawCollection *collection,
    u32 *traversal_index)
{
    while (dobj != NULL)
    {
        u32 current_index = (traversal_index != NULL) ? *traversal_index : 0u;

        if (traversal_index != NULL)
        {
            (*traversal_index)++;
        }

        if ((collection != NULL) && (dobj->dl != NULL))
        {
            if (collection->selected_count <
                NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED)
            {
                u32 selected = collection->selected_count;
                collection->dobjs[selected] = dobj;
                collection->indices[selected] = current_index;
                collection->selected_count++;
                if (current_index < 32u)
                {
                    collection->selected_index_mask |= 1u << current_index;
                }
            }
            collection->total_count++;
        }

        ndsFighterCollectAllDObjsWithDLRecursive(dobj->child,
                                                 collection,
                                                 traversal_index);
        dobj = dobj->sib_next;
    }
}

/* Four pointers, 16 bytes, and nothing else.
 *
 * This used to carry the render preamble too -- geometry mode, cycle type,
 * render mode, prim and env colour, the Light and its valid flag -- which made
 * it 56 bytes and put its two halves on different cache lines. The halves have
 * opposite access patterns: the pointers are read by three tight per-root loops
 * in the same pass that walks the collection, while the preamble is read once
 * per root by ndsRendererAdapterBuildNativeProductionInputs, a pass later,
 * after the matrix and material work has evicted it. c106 priced that eviction
 * at 2,966 ticks/frame on `root->preamble.geometry_mode = event->geometry_mode`
 * and 1,759 on `if (event->light_valid)`: ~110 cycles an event of pure miss.
 *
 * So the preamble moved to its own array, in the consumer's own struct layout,
 * written by the producer. 32 events is 512 bytes of pointers plus 768 of
 * preamble instead of 1,792 bytes interleaved, and the consumer's read is one
 * 24-byte struct copy out of a dense array rather than a field-by-field gather
 * across two cold lines. */
