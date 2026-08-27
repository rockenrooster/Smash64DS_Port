
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





typedef struct NDSFighterDLMultiDrawCollection {
    DObj *dobjs[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 indices[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 total_count;
    u32 selected_count;
    u32 selected_index_mask;
} NDSFighterDLMultiDrawCollection;










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
