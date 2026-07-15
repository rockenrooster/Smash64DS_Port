#ifndef SSB64_NDS_PUPUPU_WATER_DRAW_H
#define SSB64_NDS_PUPUPU_WATER_DRAW_H

#include <PR/ultratypes.h>

#include <nds/pupupu_water_tiled_aot.h>

#define NDS_PUPUPU_WATER_DRAW_CELL_COUNT 68u
#define NDS_PUPUPU_WATER_DRAW_TRIANGLE_COUNT 138u
#define NDS_PUPUPU_WATER_DRAW_VERTEX_COUNT 414u

enum NDSPupupuWaterDrawFailure
{
    nNDSPupupuWaterDrawFailureNone,
    nNDSPupupuWaterDrawFailureInput,
    nNDSPupupuWaterDrawFailureAssets,
    nNDSPupupuWaterDrawFailureFrame,
    nNDSPupupuWaterDrawFailurePalette,
    nNDSPupupuWaterDrawFailureResidency,
    nNDSPupupuWaterDrawFailureTile
};

/* The caller owns the exact live source parent matrix, polygon attributes,
 * white modulation color, lighting, and surrounding renderer state. Both
 * frames must be immutable views returned by ndsPupupuWaterTiledGetFrame for
 * the same live source fraction. This call preflights before its first GX
 * mutation, then emits primary-atlas cells in source order followed by the
 * optional secondary-atlas cells in source order. */
s32 ndsPupupuWaterDrawPrepared(
    const NDSPupupuWaterTiledAssets *assets,
    const NDSPupupuWaterTiledFrame *large_frame,
    const NDSPupupuWaterTiledFrame *small_frame);

extern volatile u32 gNdsPupupuWaterDrawAttemptCount;
extern volatile u32 gNdsPupupuWaterDrawSuccessCount;
extern volatile u32 gNdsPupupuWaterDrawFailCount;
extern volatile u32 gNdsPupupuWaterDrawPrimaryBatchCount;
extern volatile u32 gNdsPupupuWaterDrawSecondaryBatchCount;
extern volatile u32 gNdsPupupuWaterDrawCellCount;
extern volatile u32 gNdsPupupuWaterDrawTriangleCount;
extern volatile u32 gNdsPupupuWaterDrawVertexCount;
extern volatile u32 gNdsPupupuWaterDrawLastFailure;

#endif
