#include <nds/arm9/videoGL.h>

#include <nds/nds_pupupu_water_draw.h>
#include <nds/nds_pupupu_water_residency.h>

volatile u32 gNdsPupupuWaterDrawAttemptCount;
volatile u32 gNdsPupupuWaterDrawSuccessCount;
volatile u32 gNdsPupupuWaterDrawFailCount;
volatile u32 gNdsPupupuWaterDrawPrimaryBatchCount;
volatile u32 gNdsPupupuWaterDrawSecondaryBatchCount;
volatile u32 gNdsPupupuWaterDrawCellCount;
volatile u32 gNdsPupupuWaterDrawTriangleCount;
volatile u32 gNdsPupupuWaterDrawVertexCount;
volatile u32 gNdsPupupuWaterDrawLastFailure;

_Static_assert(NDS_PUPUPU_WATER_TILED_PLAN_CELL_COUNT ==
                   NDS_PUPUPU_WATER_DRAW_CELL_COUNT,
               "water draw cell count changed");
_Static_assert(NDS_PUPUPU_WATER_TILED_TOTAL_TRIANGLES ==
                   NDS_PUPUPU_WATER_DRAW_TRIANGLE_COUNT,
               "water draw triangle count changed");
_Static_assert(NDS_PUPUPU_WATER_TILED_EMITTED_VERTICES ==
                   NDS_PUPUPU_WATER_DRAW_VERTEX_COUNT,
               "water draw vertex count changed");

static s32 ndsPupupuWaterDrawFail(u32 failure)
{
    gNdsPupupuWaterDrawFailCount++;
    gNdsPupupuWaterDrawLastFailure = failure;
    return FALSE;
}

static const NDSPupupuWaterTiledFrame *ndsPupupuWaterDrawCellFrame(
    const NDSPupupuWaterTiledPlanCell *cell,
    const NDSPupupuWaterTiledFrame *large_frame,
    const NDSPupupuWaterTiledFrame *small_frame)
{
    return (cell->owner == NDS_PUPUPU_WATER_TILED_OWNER_LARGE) ?
        large_frame : small_frame;
}

static s32 ndsPupupuWaterDrawPreflight(
    const NDSPupupuWaterTiledAssets *assets,
    const NDSPupupuWaterTiledFrame *large_frame,
    const NDSPupupuWaterTiledFrame *small_frame,
    u32 *has_secondary)
{
    NDSPupupuWaterTiledAssets expected_assets;
    u32 tile_index;

    if ((assets == NULL) || (large_frame == NULL) ||
        (small_frame == NULL) || (has_secondary == NULL))
    {
        return nNDSPupupuWaterDrawFailureInput;
    }
    if ((ndsPupupuWaterTiledGetAssets(&expected_assets) !=
         NDS_PUPUPU_WATER_TILED_OK) ||
        (assets->plan_cells != expected_assets.plan_cells) ||
        (assets->plan_vertices != expected_assets.plan_vertices) ||
        (assets->plan_cell_count != expected_assets.plan_cell_count) ||
        (assets->plan_vertex_count != expected_assets.plan_vertex_count))
    {
        return nNDSPupupuWaterDrawFailureAssets;
    }
    if ((large_frame->tile_ids == NULL) ||
        (large_frame->state_cell_count != 64u) ||
        (large_frame->state_columns != 4u) ||
        (large_frame->state_rows != 16u) ||
        (large_frame->reserved[0] != 0u) ||
        (large_frame->reserved[1] != 0u) ||
        (small_frame->tile_ids == NULL) ||
        (small_frame->state_cell_count != 8u) ||
        (small_frame->state_columns != 1u) ||
        (small_frame->state_rows != 8u) ||
        (small_frame->reserved[0] != 0u) ||
        (small_frame->reserved[1] != 0u))
    {
        return nNDSPupupuWaterDrawFailureFrame;
    }
    if ((large_frame->palette_index != small_frame->palette_index) ||
        (large_frame->palette_index >=
         NDS_PUPUPU_WATER_RESIDENCY_PALETTE_COUNT))
    {
        return nNDSPupupuWaterDrawFailurePalette;
    }
    if (gNdsPupupuWaterResidencyPrepared == FALSE)
    {
        return nNDSPupupuWaterDrawFailureResidency;
    }

    for (tile_index = 0u; tile_index < large_frame->state_cell_count;
         tile_index++)
    {
        if (large_frame->tile_ids[tile_index] >=
            NDS_PUPUPU_WATER_TILED_SECONDARY_TILE_FIRST)
        {
            return nNDSPupupuWaterDrawFailureTile;
        }
    }
    *has_secondary = FALSE;
    for (tile_index = 0u; tile_index < small_frame->state_cell_count;
         tile_index++)
    {
        u32 tile_id = small_frame->tile_ids[tile_index];

        if (tile_id >= NDS_PUPUPU_WATER_TILED_MASKED_TILE_COUNT)
        {
            return nNDSPupupuWaterDrawFailureTile;
        }
        *has_secondary |=
            tile_id >= NDS_PUPUPU_WATER_TILED_SECONDARY_TILE_FIRST;
    }
    return nNDSPupupuWaterDrawFailureNone;
}

static inline __attribute__((always_inline)) void ndsPupupuWaterDrawVertex(
    const NDSPupupuWaterTiledPlanVertex *vertex,
    s32 cell_s_q16, s32 cell_t_q16, s32 atlas_s_q4, s32 atlas_t_q4)
{
    glTexCoord2t16(
        (t16)(atlas_s_q4 + ((vertex->s_q16 - cell_s_q16) >> 12)),
        (t16)(atlas_t_q4 + ((vertex->t_q16 - cell_t_q16) >> 12)));
    glVertex3v16(
        (v16)(vertex->x_q12 >> 8), 0, (v16)(vertex->z_q12 >> 8));
}

static void ndsPupupuWaterDrawAtlas(
    const NDSPupupuWaterTiledAssets *assets,
    const NDSPupupuWaterTiledFrame *large_frame,
    const NDSPupupuWaterTiledFrame *small_frame,
    u32 secondary)
{
    u32 cell_index;

    glBegin(GL_TRIANGLE);
    for (cell_index = 0u; cell_index < assets->plan_cell_count; cell_index++)
    {
        const NDSPupupuWaterTiledPlanCell *cell =
            &assets->plan_cells[cell_index];
        const NDSPupupuWaterTiledFrame *frame = ndsPupupuWaterDrawCellFrame(
            cell, large_frame, small_frame);
        u32 tile_id = frame->tile_ids[cell->cell_index];
        u32 is_secondary =
            tile_id >= NDS_PUPUPU_WATER_TILED_SECONDARY_TILE_FIRST;
        u32 triangle;
        u32 local_tile;
        u32 column;
        u32 row;
        s32 cell_s_q16;
        s32 cell_t_q16;
        s32 atlas_s_q4;
        s32 atlas_t_q4;
        const NDSPupupuWaterTiledPlanVertex *vertices;

        if (is_secondary != secondary)
        {
            continue;
        }
        local_tile = is_secondary ?
            tile_id - NDS_PUPUPU_WATER_TILED_SECONDARY_TILE_FIRST : tile_id;
        column = local_tile & (is_secondary ? 7u : 15u);
        row = local_tile >> (is_secondary ? 3u : 4u);
        cell_s_q16 = (s32)cell->cell_x *
            (NDS_PUPUPU_WATER_TILED_TILE_WIDTH << 16);
        cell_t_q16 = (s32)cell->cell_y *
            (NDS_PUPUPU_WATER_TILED_TILE_HEIGHT << 16);
        atlas_s_q4 = (s32)column *
            (NDS_PUPUPU_WATER_TILED_TILE_WIDTH << 4);
        atlas_t_q4 = (s32)row *
            (NDS_PUPUPU_WATER_TILED_TILE_HEIGHT << 4);
        vertices = &assets->plan_vertices[cell->first_vertex];
        for (triangle = 0u; triangle < cell->triangle_count; triangle++)
        {
            ndsPupupuWaterDrawVertex(
                &vertices[0], cell_s_q16, cell_t_q16,
                atlas_s_q4, atlas_t_q4);
            ndsPupupuWaterDrawVertex(
                &vertices[triangle + 1u], cell_s_q16, cell_t_q16,
                atlas_s_q4, atlas_t_q4);
            ndsPupupuWaterDrawVertex(
                &vertices[triangle + 2u], cell_s_q16, cell_t_q16,
                atlas_s_q4, atlas_t_q4);
        }
    }
    glEnd();
}

s32 ndsPupupuWaterDrawPrepared(
    const NDSPupupuWaterTiledAssets *assets,
    const NDSPupupuWaterTiledFrame *large_frame,
    const NDSPupupuWaterTiledFrame *small_frame)
{
    u32 failure;
    u32 has_secondary;
    u32 palette_index;

    gNdsPupupuWaterDrawAttemptCount++;
    failure = ndsPupupuWaterDrawPreflight(
        assets, large_frame, small_frame, &has_secondary);
    if (failure != nNDSPupupuWaterDrawFailureNone)
    {
        return ndsPupupuWaterDrawFail(failure);
    }

    palette_index = large_frame->palette_index;
    (void)ndsPupupuWaterResidencyBindPrimary(palette_index);
    ndsPupupuWaterDrawAtlas(assets, large_frame, small_frame, FALSE);
    if (has_secondary != FALSE)
    {
        (void)ndsPupupuWaterResidencyBindSecondary(palette_index);
        ndsPupupuWaterDrawAtlas(assets, large_frame, small_frame, TRUE);
    }

    gNdsPupupuWaterDrawSuccessCount++;
    gNdsPupupuWaterDrawPrimaryBatchCount++;
    gNdsPupupuWaterDrawSecondaryBatchCount += has_secondary;
    gNdsPupupuWaterDrawCellCount += NDS_PUPUPU_WATER_DRAW_CELL_COUNT;
    gNdsPupupuWaterDrawTriangleCount +=
        NDS_PUPUPU_WATER_DRAW_TRIANGLE_COUNT;
    gNdsPupupuWaterDrawVertexCount += NDS_PUPUPU_WATER_DRAW_VERTEX_COUNT;
    gNdsPupupuWaterDrawLastFailure = nNDSPupupuWaterDrawFailureNone;
    return TRUE;
}
