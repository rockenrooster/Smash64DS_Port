/* Host-only software fighter visualization; not an ARM9 implementation unit. */
#if defined(ARM9) || defined(ARM7) || defined(__NDS__)
#error Software reference rasterization is host-only
#endif


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





static void ndsFighterDLAllDrawRecordScreenPoint(
    s32 x, s32 y, u32 *screen_valid,
    s32 *screen_min_x, s32 *screen_max_x,
    s32 *screen_min_y, s32 *screen_max_y)
{
    if ((screen_valid == NULL) || (screen_min_x == NULL) ||
        (screen_max_x == NULL) || (screen_min_y == NULL) ||
        (screen_max_y == NULL))
    {
        return;
    }

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

static void ndsFighterDLAllDrawRasterizeStates(
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
                                         cx - 5, cy - 3,
                                         cx + 5, cy - 3,
                                         cx, cy + 5,
                                         fill, edge, &pixel_count);
                marker_drawn = 1u;
                x0 = cx - 5;
                y0 = cy - 3;
                x1 = cx + 5;
                y1 = cy - 3;
                x2 = cx;
                y2 = cy + 5;
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
                ndsFighterDLAllDrawRecordScreenPoint(
                    x0, y0, &screen_valid, &screen_min_x, &screen_max_x,
                    &screen_min_y, &screen_max_y);
                ndsFighterDLAllDrawRecordScreenPoint(
                    x1, y1, &screen_valid, &screen_min_x, &screen_max_x,
                    &screen_min_y, &screen_max_y);
                ndsFighterDLAllDrawRecordScreenPoint(
                    x2, y2, &screen_valid, &screen_min_x, &screen_max_x,
                    &screen_min_y, &screen_max_y);
            }
        }
        if (state_drawn != 0u)
        {
            drawn_dobj_count++;
        }
    }

    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0Axis = best_axis;
        gNdsFighterDLAllDrawP0Area = best_area;
        gNdsFighterDLAllDrawP0MinA = min_a;
        gNdsFighterDLAllDrawP0MaxA = max_a;
        gNdsFighterDLAllDrawP0MinB = min_b;
        gNdsFighterDLAllDrawP0MaxB = max_b;
        gNdsFighterDLAllDrawP0ScreenMinX = screen_min_x;
        gNdsFighterDLAllDrawP0ScreenMaxX = screen_max_x;
        gNdsFighterDLAllDrawP0ScreenMinY = screen_min_y;
        gNdsFighterDLAllDrawP0ScreenMaxY = screen_max_y;
        gNdsFighterDLAllDrawP0PixelCount = pixel_count;
        gNdsFighterDLAllDrawP0TriangleDrawnCount = drawn_count;
        gNdsFighterDLAllDrawP0RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLAllDrawP0MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLAllDrawP0DrawnDObjCount = drawn_dobj_count;
    }
    else
    {
        gNdsFighterDLAllDrawP1Axis = best_axis;
        gNdsFighterDLAllDrawP1Area = best_area;
        gNdsFighterDLAllDrawP1MinA = min_a;
        gNdsFighterDLAllDrawP1MaxA = max_a;
        gNdsFighterDLAllDrawP1MinB = min_b;
        gNdsFighterDLAllDrawP1MaxB = max_b;
        gNdsFighterDLAllDrawP1ScreenMinX = screen_min_x;
        gNdsFighterDLAllDrawP1ScreenMaxX = screen_max_x;
        gNdsFighterDLAllDrawP1ScreenMinY = screen_min_y;
        gNdsFighterDLAllDrawP1ScreenMaxY = screen_max_y;
        gNdsFighterDLAllDrawP1PixelCount = pixel_count;
        gNdsFighterDLAllDrawP1TriangleDrawnCount = drawn_count;
        gNdsFighterDLAllDrawP1RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLAllDrawP1MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLAllDrawP1DrawnDObjCount = drawn_dobj_count;
    }
}
