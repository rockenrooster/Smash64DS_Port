static void ndsRendererRecordTransformedTriangle(
    NDSRendererStats *stats,
    const NDSRendererTraversalState *state,
    u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;

    if (stats == NULL)
    {
        return;
    }
    if (ndsRendererTransformedTriangleReady(state, packed, &i0, &i1, &i2) ==
        FALSE)
    {
        return;
    }

    if (stats->transformed_triangle_count == 0u)
    {
        stats->first_transformed_tri_v0 = i0;
        stats->first_transformed_tri_v1 = i1;
        stats->first_transformed_tri_v2 = i2;
    }
    stats->transformed_triangle_count++;
}

#if NDS_RENDERER_HW_TRIANGLES
static s32 ndsRendererRoundShiftS32Signed(s32 value, u32 shift)
{
    if (shift == 0u)
    {
        return value;
    }
    return (s32)ndsRendererRoundShiftS64(value, shift);
}

static s32 ndsRendererNativeStageVertexShift(s16 value, u32 shift)
{
    u32 magnitude;

    if (shift == 0u)
    {
        return value;
    }
    magnitude = (value < 0) ? (u32)-(s32)value : (u32)value;
    magnitude = (magnitude + (1u << (shift - 1u))) >> shift;
    return (value < 0) ? -(s32)magnitude : (s32)magnitude;
}

static v16 ndsRendererHardwareCoordToV16(s16 value)
{
    const u32 shift = 12u - NDS_RENDERER_HW_WORLD_UNIT_SHIFT;
    s32 scaled = (s32)value << shift;

    if (scaled > 32767)
    {
        return (v16)32767;
    }
    if (scaled < -32768)
    {
        return (v16)-32768;
    }
    return (v16)scaled;
}

static v16 ndsRendererHardwareVertexCoord(s16 value, u32 scale_world)
{
    if (scale_world == 0u)
    {
        return (v16)value;
    }
    return ndsRendererHardwareCoordToV16(value);
}

static v16 ndsRendererHardwareClampS64ToV16(s64 value)
{
    if (value > 32767)
    {
        return (v16)32767;
    }
    if (value < -32768)
    {
        return (v16)-32768;
    }
    return (v16)value;
}

#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static v16 __attribute__((noinline, optimize("Os")))
ndsRendererHardwareProjectToV16(s64 numerator, s32 denominator)
#else
static inline v16 ndsRendererHardwareProjectToV16(
    s64 numerator, s32 denominator)
#endif
{
    s64 low_product;
    s64 high_product;
    v16 result;

    if (denominator == 0)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary |=
            NDS_RENDERER_HW_DIVISION_ZERO_DENOMINATOR;
#endif
        return 0;
    }

    /* The DS 64/32 divider returns a signed 32-bit quotient. Pre-clamp the
     * exact C result into v16 range so the hardware operation cannot overflow
     * its result register. Negative W reverses both product comparisons. */
    low_product = (s64)-32768 * (s64)denominator;
    high_product = (s64)32767 * (s64)denominator;
    if (((denominator > 0) && (numerator < low_product)) ||
        ((denominator < 0) && (numerator > low_product)))
    {
        result = (v16)-32768;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary +=
            NDS_RENDERER_HW_DIVISION_PRECLAMP_LOW_ONE;
#endif
    }
    else if (((denominator > 0) && (numerator > high_product)) ||
             ((denominator < 0) && (numerator < high_product)))
    {
        result = (v16)32767;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary +=
            NDS_RENDERER_HW_DIVISION_PRECLAMP_HIGH_ONE;
#endif
    }
    else
    {
#if defined(__arm__)
        /* ndsR2HwMathDiv64, not libnds's div64: identical DIV_64_32 sequence
         * minus the leading poll, which waits out a stale quotient nobody
         * reads. Graded bit-identical over 65,536 operands on four builds. */
        result = (v16)ndsR2HwMathDiv64(numerator, denominator);
#else
        result = ndsRendererHardwareClampS64ToV16(
            numerator / denominator);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary++;
#endif
    }

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (result != ndsRendererHardwareClampS64ToV16(
                      numerator / denominator))
    {
        sNdsRendererHardwareDivideSummary |=
            NDS_RENDERER_HW_DIVISION_MISMATCH;
    }
#endif
    return result;
}

#if NDS_RENDERER_SCREEN_SPACE_CENSUS
static void ndsRendererScreenSpaceCensusReset(void)
{
    memset((void *)gNdsRendererScreenSpaceCensus, 0,
           sizeof(gNdsRendererScreenSpaceCensus));
    memset((void *)gNdsRendererScreenSpaceStageOwnerTicks, 0,
           sizeof(gNdsRendererScreenSpaceStageOwnerTicks));
    gNdsRendererScreenSpaceCensusFrameCount = 0u;
    gNdsRendererScreenSpaceCensusOverflowCount = 0u;
}

static void ndsRendererScreenSpaceCensusTriangle(
    u32 owner,
    u32 part,
    u32 identity,
    const NDSRendererClipVertex20p12 clip[3])
{
    volatile NDSRendererScreenSpaceCensusRow *row;
    s32 x_q4[3];
    s32 y_q4[3];
    s64 cross;
    u64 area2_q8;
    u32 i;

    if ((gNdsRendererScreenSpaceCensusArmed == 0u) ||
        (owner >= NDS_RENDERER_PROFILE_OWNER_COUNT) ||
        (part >= NDS_RENDERER_SCREEN_SPACE_CENSUS_PART_COUNT))
    {
        if ((gNdsRendererScreenSpaceCensusArmed != 0u) &&
            ((owner >= NDS_RENDERER_PROFILE_OWNER_COUNT) ||
             (part >= NDS_RENDERER_SCREEN_SPACE_CENSUS_PART_COUNT)))
        {
            gNdsRendererScreenSpaceCensusOverflowCount++;
        }
        return;
    }
    row = &gNdsRendererScreenSpaceCensus[owner][part];
    if (row->identity == 0u)
    {
        row->identity = identity;
    }
    row->triangle_count++;
    for (i = 0u; i < 3u; i++)
    {
        v16 projected_x;
        v16 projected_y;

        if (clip[i].w <= 0)
        {
            row->invalid_count++;
            return;
        }
        projected_x = ndsRendererHardwareProjectToV16(
            (s64)clip[i].x * 4096, clip[i].w);
        projected_y = ndsRendererHardwareProjectToV16(
            (s64)clip[i].y * 4096, clip[i].w);
        /* 256x192 viewport, retained as Q4 screen coordinates. */
        x_q4[i] = (s32)projected_x / 2;
        y_q4[i] = ((s32)projected_y * 3) / 8;
    }
    cross =
        (s64)(x_q4[1] - x_q4[0]) * (y_q4[2] - y_q4[0]) -
        (s64)(y_q4[1] - y_q4[0]) * (x_q4[2] - x_q4[0]);
    area2_q8 = (cross < 0) ? (u64)-cross : (u64)cross;
    if (area2_q8 < 512u)
    {
        row->area_lt_1px_count++;
    }
    if (area2_q8 < 2048u)
    {
        row->area_lt_4px_count++;
    }
    row->area2_q8_sum += area2_q8;
}

static void ndsRendererScreenSpaceCensusStageSegment(
    const NDSNativeStageSegment *segment)
{
    u32 run_offset;

    if ((gNdsRendererScreenSpaceCensusArmed == 0u) || (segment == NULL))
    {
        return;
    }
    for (run_offset = 0u; run_offset < segment->run_count; run_offset++)
    {
        const NDSNativeStageRun *run = &sNdsNativeStageRuns[
            (u32)segment->first_run + run_offset];
        u32 triangle_offset;

        for (triangle_offset = 0u;
             triangle_offset < run->triangle_count;
             triangle_offset++)
        {
            NDSRendererClipVertex20p12 clip[3];
            u32 corner_offset;

            for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
            {
                u32 dense_index = sNdsNativeStageCorners[
                    (u32)run->first_corner + triangle_offset * 3u +
                    corner_offset];
                const NDSNativeStageDenseVertex *dense =
                    &sNdsNativeStageVertices[dense_index];
                NDSRendererInputVertex input = {0};

                input.x = dense->x;
                input.y = dense->y;
                input.z = dense->z;
                ndsRendererTransformVertex20p12(
                    &sNdsNativeStageOwnerExecution.binding_composed[
                        dense->matrix_binding],
                    &input, &clip[corner_offset]);
            }
            ndsRendererScreenSpaceCensusTriangle(
                NDS_RENDERER_PROFILE_OWNER_STAGE,
                run->binding_index,
                sNdsNativeStageBindings[run->binding_index].root_offset,
                clip);
        }
    }
}

static void ndsRendererScreenSpaceCensusFighterRun(
    const NDSNativeRun *run,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count,
    u32 root_index)
{
    u32 run_index;
    u32 triangle_offset;

    if ((gNdsRendererScreenSpaceCensusArmed == 0u) || (run == NULL) ||
        (inputs == NULL) || (root_index >= input_count))
    {
        return;
    }
    run_index = (u32)(run - sNdsNativeFighterActiveTables->runs);
    for (triangle_offset = 0u;
         triangle_offset < run->triangle_count;
         triangle_offset++)
    {
        NDSRendererClipVertex20p12 clip[3];
        u32 corner_offset;

        for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
        {
            u32 packed = sNdsNativeFighterActiveTables->packed_corners[
                sNdsNativeFighterActiveTables->run_first_corner[run_index] +
                triangle_offset * 3u + corner_offset];
            u32 dense_id = packed & NDS_NATIVE_DENSE_ID_MASK;
            const NDSNativeDenseVertex *dense =
                &sNdsNativeFighterActiveTables->dense_vertices[dense_id];
            const NDSNativePreparedDenseVertex *prepared =
                &sNdsNativeFighterActiveTables->prepared_dense[dense_id];
            u32 matrix_binding = dense->matrix_binding;
            NDSRendererInputVertex input = {0};

            if (matrix_binding >= input_count)
            {
                matrix_binding = root_index;
            }
            input.x = (s16)(prepared->gx_xy & 0xffffu) / 16;
            input.y = (s16)(prepared->gx_xy >> 16) / 16;
            input.z = (s16)prepared->gx_z / 16;
            ndsRendererTransformVertex20p12(
                inputs[matrix_binding].composed_matrix,
                &input, &clip[corner_offset]);
        }
        ndsRendererScreenSpaceCensusTriangle(
            (u32)sNdsRendererRuntimeOwner,
            root_index,
            inputs[root_index].root_offset,
            clip);
    }
}
#endif

static inline v16 ndsRendererHardwareSourceDepthToV16(
    s64 numerator, s32 denominator)
{
    v16 depth = ndsRendererHardwareProjectToV16(numerator, denominator);

    /* Reserve 128 strictly ordered v16 depths at each endpoint for no-Z
     * painter primitives.  Canonical source Z is already inside this central
     * range; the clamp only prevents camera extremes from entering a painter
     * band in the DS's otherwise shared depth channel. */
    if (depth < NDS_RENDERER_HW_SOURCE_DEPTH_MIN)
    {
        return (v16)NDS_RENDERER_HW_SOURCE_DEPTH_MIN;
    }
    if (depth > NDS_RENDERER_HW_SOURCE_DEPTH_MAX)
    {
        return (v16)NDS_RENDERER_HW_SOURCE_DEPTH_MAX;
    }
    return depth;
}
#endif

static void ndsRendererProfileVertexRange(
    const NDSRendererInputVertex *vtx, v16 x, v16 y, v16 z)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (vtx == NULL)
    {
        return;
    }

    if (vtx->x < gNdsRendererProfileRawVertexMinX) { gNdsRendererProfileRawVertexMinX = vtx->x; }
    if (vtx->x > gNdsRendererProfileRawVertexMaxX) { gNdsRendererProfileRawVertexMaxX = vtx->x; }
    if (vtx->y < gNdsRendererProfileRawVertexMinY) { gNdsRendererProfileRawVertexMinY = vtx->y; }
    if (vtx->y > gNdsRendererProfileRawVertexMaxY) { gNdsRendererProfileRawVertexMaxY = vtx->y; }
    if (vtx->z < gNdsRendererProfileRawVertexMinZ) { gNdsRendererProfileRawVertexMinZ = vtx->z; }
    if (vtx->z > gNdsRendererProfileRawVertexMaxZ) { gNdsRendererProfileRawVertexMaxZ = vtx->z; }
    if ((s32)x < gNdsRendererProfileHWVertexMinX) { gNdsRendererProfileHWVertexMinX = x; }
    if ((s32)x > gNdsRendererProfileHWVertexMaxX) { gNdsRendererProfileHWVertexMaxX = x; }
    if ((s32)y < gNdsRendererProfileHWVertexMinY) { gNdsRendererProfileHWVertexMinY = y; }
    if ((s32)y > gNdsRendererProfileHWVertexMaxY) { gNdsRendererProfileHWVertexMaxY = y; }
    if ((s32)z < gNdsRendererProfileHWVertexMinZ) { gNdsRendererProfileHWVertexMinZ = z; }
    if ((s32)z > gNdsRendererProfileHWVertexMaxZ) { gNdsRendererProfileHWVertexMaxZ = z; }
#else
    (void)vtx;
#endif
    if ((x == (v16)32767) || (x == (v16)-32768) ||
        (y == (v16)32767) || (y == (v16)-32768) ||
        (z == (v16)32767) || (z == (v16)-32768))
    {
        ndsRendererProfileRecordVertexSaturate();
    }
}

static void ndsRendererProfileHWVertexRange(v16 x, v16 y, v16 z)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if ((s32)x < gNdsRendererProfileHWVertexMinX) { gNdsRendererProfileHWVertexMinX = x; }
    if ((s32)x > gNdsRendererProfileHWVertexMaxX) { gNdsRendererProfileHWVertexMaxX = x; }
    if ((s32)y < gNdsRendererProfileHWVertexMinY) { gNdsRendererProfileHWVertexMinY = y; }
    if ((s32)y > gNdsRendererProfileHWVertexMaxY) { gNdsRendererProfileHWVertexMaxY = y; }
    if ((s32)z < gNdsRendererProfileHWVertexMinZ) { gNdsRendererProfileHWVertexMinZ = z; }
    if ((s32)z > gNdsRendererProfileHWVertexMaxZ) { gNdsRendererProfileHWVertexMaxZ = z; }
#endif
    if ((x == (v16)32767) || (x == (v16)-32768) ||
        (y == (v16)32767) || (y == (v16)-32768) ||
        (z == (v16)32767) || (z == (v16)-32768))
    {
        ndsRendererProfileRecordVertexSaturate();
    }
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererAbsDiffS32(s32 lhs, s32 rhs)
{
    s64 diff = (s64)lhs - (s64)rhs;

    if (diff < 0)
    {
        diff = -diff;
    }
    return (diff > (s64)0xffffffffu) ? 0xffffffffu : (u32)diff;
}

static void ndsRendererHardwareRecordOracleVertex(
    const NDSRendererTraversalState *state, u32 index)
{
    const NDSRendererMatrixSnapshot *snapshot;
    const NDSRendererMatrix20p12 *matrix = NULL;
    NDSRendererClipVertex20p12 expected;
    const NDSRendererClipVertex20p12 *actual;
    u32 dx;
    u32 dy;
    u32 dz;
    u32 dw;
    u32 max_delta;

    if ((state == NULL) ||
        (index >= NDS_RENDERER_MAX_VTX) ||
        ((state->input_vertex_valid_mask & (1u << index)) == 0u) ||
        ((state->vertex_valid_mask & (1u << index)) == 0u) ||
        (state->vertex_clip_snapshot[index] !=
         state->vertex_matrix_snapshot[index]))
    {
        return;
    }

    snapshot = ndsRendererGetMatrixSnapshot(
        state, state->vertex_matrix_snapshot[index]);
    if (snapshot != NULL)
    {
        matrix = &snapshot->matrix;
    }
    else if (((state->current_transform_vertex_mask & (1u << index)) != 0u) &&
             (state->matrix_valid != 0u))
    {
        /* Bounded-table overflow is eagerly transformed at VTX load. */
        matrix = &state->matrix;
    }
    if (matrix == NULL)
    {
        return;
    }

    ndsRendererTransformVertex20p12(matrix,
                                    &state->input_vertices[index],
                                    &expected);
    actual = &state->vertices[index];
    dx = ndsRendererAbsDiffS32(expected.x, actual->x);
    dy = ndsRendererAbsDiffS32(expected.y, actual->y);
    dz = ndsRendererAbsDiffS32(expected.z, actual->z);
    dw = ndsRendererAbsDiffS32(expected.w, actual->w);
    max_delta = dx;
    if (dy > max_delta) { max_delta = dy; }
    if (dz > max_delta) { max_delta = dz; }
    if (dw > max_delta) { max_delta = dw; }

    gNdsRendererProfileOracleSamples++;
    if (max_delta > gNdsRendererProfileOracleMaxDelta)
    {
        gNdsRendererProfileOracleMaxDelta = max_delta;
    }
    if (max_delta > NDS_RENDERER_HW_ORACLE_EPSILON)
    {
        gNdsRendererProfileOracleMismatches++;
    }
}

static void ndsRendererHardwareRecordOracleTriangle(
    const NDSRendererTraversalState *state, u32 i0, u32 i1, u32 i2)
{
    ndsRendererHardwareRecordOracleVertex(state, i0);
    ndsRendererHardwareRecordOracleVertex(state, i1);
    ndsRendererHardwareRecordOracleVertex(state, i2);
}
#endif

static s32 ndsRendererCombineUsesColor(u32 w0, u32 w1, u32 source)
{
    return ((((w0 >> 20) & 0x0fu) == source) ||
            (((w1 >> 28) & 0x0fu) == source) ||
            (((w0 >> 15) & 0x1fu) == source) ||
            (((w1 >> 15) & 0x07u) == source) ||
            (((w0 >> 5) & 0x0fu) == source) ||
            (((w1 >> 24) & 0x0fu) == source) ||
            (((w0 >> 0) & 0x1fu) == source) ||
            (((w1 >> 6) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererCombineOutputUsesColor(u32 w0, u32 w1, u32 source)
{
    return ((((w0 >> 15) & 0x1fu) == source) ||
            (((w1 >> 15) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererCombineSecondOutputUsesColor(u32 w0, u32 w1,
                                                   u32 source)
{
    return ((((w0 >> 0) & 0x1fu) == source) ||
            (((w1 >> 6) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareUseSecondCycle(const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            ((stats->othermode_h & NDS_RENDERER_CYCLETYPE_MASK) ==
             NDS_RENDERER_CYC_2CYCLE)) ? TRUE : FALSE;
}

static u32 ndsRendererHardwarePrimEnvTexel0BlendMode(
    const NDSRendererStats *stats)
{
    u32 w0;
    u32 w1;
    u32 color_a;
    u32 color_b;
    u32 color_c;
    u32 color_d;
    u32 alpha_a;
    u32 alpha_b;
    u32 alpha_c;
    u32 alpha_d;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return NDS_RENDERER_PRIM_ENV_BLEND_NONE;
    }
    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    if (ndsRendererHardwareUseSecondCycle(stats) != FALSE)
    {
        color_a = (w0 >> 5) & 0x0fu;
        color_b = (w1 >> 24) & 0x0fu;
        color_c = w0 & 0x1fu;
        color_d = (w1 >> 6) & 0x07u;
        alpha_a = (w1 >> 21) & 0x07u;
        alpha_b = (w1 >> 3) & 0x07u;
        alpha_c = (w1 >> 18) & 0x07u;
        alpha_d = w1 & 0x07u;
    }
    else
    {
        color_a = (w0 >> 20) & 0x0fu;
        color_b = (w1 >> 28) & 0x0fu;
        color_c = (w0 >> 15) & 0x1fu;
        color_d = (w1 >> 15) & 0x07u;
        alpha_a = (w0 >> 12) & 0x07u;
        alpha_b = (w1 >> 12) & 0x07u;
        alpha_c = (w0 >> 9) & 0x07u;
        alpha_d = (w1 >> 9) & 0x07u;
    }
    /* BattleShip's G_CC_BLENDPE family is a texture-weighted endpoint lerp:
     * (PRIMITIVE - ENVIRONMENT) * TEXEL0 + ENVIRONMENT. Decode the active
     * cycle so both raw forms remain data-independent: one multiplies source
     * coverage by primitive alpha, while the other passes source alpha. */
    if ((color_a == NDS_RENDERER_CCMUX_PRIMITIVE) &&
        (color_b == NDS_RENDERER_CCMUX_ENVIRONMENT) &&
        (color_c == NDS_RENDERER_CCMUX_TEXEL0) &&
        (color_d == NDS_RENDERER_CCMUX_ENVIRONMENT))
    {
        if ((alpha_a == NDS_RENDERER_ACMUX_TEXEL0) &&
            (alpha_b == NDS_RENDERER_ACMUX_0) &&
            (alpha_c == NDS_RENDERER_ACMUX_PRIMITIVE) &&
            (alpha_d == NDS_RENDERER_ACMUX_0))
        {
            return NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA;
        }
        if ((alpha_a == NDS_RENDERER_ACMUX_0) &&
            (alpha_b == NDS_RENDERER_ACMUX_0) &&
            (alpha_c == NDS_RENDERER_ACMUX_0) &&
            (alpha_d == NDS_RENDERER_ACMUX_TEXEL0))
        {
            return NDS_RENDERER_PRIM_ENV_BLEND_SOURCE_ALPHA;
        }
        return NDS_RENDERER_PRIM_ENV_BLEND_NONE;
    }
    /* THE DEGENERATE CONSTANT FORM, and it is a separate branch rather than a
     * loosened gate above. (0,0,0,PRIMITIVE) for colour multiplies nothing --
     * it selects PRIM flat -- while (0,0,0,TEXEL0) for alpha passes source
     * coverage through untouched. The lerp gate cannot be widened to admit it
     * without also admitting every other constant-d combine in the game. */
    if ((color_a == NDS_RENDERER_CCMUX_ZERO_AB) &&
        (color_b == NDS_RENDERER_CCMUX_ZERO_AB) &&
        (color_c == NDS_RENDERER_CCMUX_ZERO_C) &&
        (color_d == NDS_RENDERER_CCMUX_PRIMITIVE) &&
        (alpha_a == NDS_RENDERER_ACMUX_0) &&
        (alpha_b == NDS_RENDERER_ACMUX_0) &&
        (alpha_c == NDS_RENDERER_ACMUX_0) &&
        (alpha_d == NDS_RENDERER_ACMUX_TEXEL0))
    {
        return NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA;
    }
    return NDS_RENDERER_PRIM_ENV_BLEND_NONE;
}

/* TRUE when the recognised combine's ALPHA mux is (0,0,0,TEXEL0): source
 * coverage passes through untouched, and the bake has already placed it in the
 * texel's alpha bit.
 *
 * SOURCE_ALPHA and PRIM_RGB_TEXEL0_ALPHA differ ONLY in their colour mux --
 * their alpha muxes are byte-identical -- so every alpha-side decision has to
 * treat them the same. Keying an alpha site on one mode alone would let the
 * other multiply vertex alpha into coverage the texel already carries. The two
 * call sites below are both unreachable for the rebirth-halo beam (its
 * othermode_l 0x552078 takes their opaque early return first), so this closes a
 * latent asymmetry rather than fixing a measured defect. */
static s32 ndsRendererHardwareBlendModeKeepsTexelCoverage(u32 mode)
{
    return ((mode == NDS_RENDERER_PRIM_ENV_BLEND_SOURCE_ALPHA) ||
            (mode == NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA)) ?
        TRUE : FALSE;
}

static s32 ndsRendererHardwareUsesTexel01Lerp(
    const NDSRendererStats *stats)
{
    u32 w0;
    u32 w1;

    if ((stats == NULL) ||
        (stats->texture_combine_count == 0u) ||
        (ndsRendererHardwareUseSecondCycle(stats) == FALSE))
    {
        return FALSE;
    }

    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    /* BattleShip's animated Pupupu water uses G_CC_TEMPLERP for color and
     * alpha, followed by COMBINED * SHADE / COMBINED * PRIMITIVE.
     * Decode the semantic mux rather than keying this DS adaptation to a
     * stage address or raw display-list pointer. */
    return ((((w0 >> 20) & 0x0fu) == NDS_RENDERER_CCMUX_TEXEL1) &&
            (((w1 >> 28) & 0x0fu) == NDS_RENDERER_CCMUX_TEXEL0) &&
            (((w0 >> 15) & 0x1fu) ==
             NDS_RENDERER_CCMUX_PRIM_LOD_FRAC) &&
            (((w1 >> 15) & 0x07u) == NDS_RENDERER_CCMUX_TEXEL0) &&
            (((w0 >> 12) & 0x07u) == NDS_RENDERER_ACMUX_TEXEL1) &&
            (((w1 >> 12) & 0x07u) == NDS_RENDERER_ACMUX_TEXEL0) &&
            (((w0 >> 9) & 0x07u) == NDS_RENDERER_ACMUX_1) &&
            (((w1 >> 9) & 0x07u) == NDS_RENDERER_ACMUX_TEXEL0) &&
            (((w0 >> 5) & 0x0fu) == NDS_RENDERER_CCMUX_COMBINED) &&
            (((w1 >> 24) & 0x0fu) == NDS_RENDERER_CCMUX_ZERO_AB) &&
            (((w0 >> 0) & 0x1fu) == NDS_RENDERER_CCMUX_SHADE) &&
            (((w1 >> 6) & 0x07u) == NDS_RENDERER_CCMUX_ZERO_D) &&
            (((w1 >> 21) & 0x07u) == NDS_RENDERER_ACMUX_COMBINED) &&
            (((w1 >> 3) & 0x07u) == NDS_RENDERER_ACMUX_0) &&
            (((w1 >> 18) & 0x07u) == NDS_RENDERER_ACMUX_PRIMITIVE) &&
            (((w1 >> 0) & 0x07u) == NDS_RENDERER_ACMUX_0)) ? TRUE : FALSE;
}

static const NDSRendererTextureLoadState *
ndsRendererHardwareFindTextureLoadForTmem(const NDSRendererStats *stats,
                                           u32 tmem)
{
    const NDSRendererTextureLoadState *best = NULL;
    u32 i;

    if (stats == NULL)
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT; i++)
    {
        const NDSRendererTextureLoadState *load = &stats->texture_loads[i];

        if ((load->valid != 0u) && (load->load_tmem == tmem) &&
            ((best == NULL) || (load->sequence > best->sequence)))
        {
            best = load;
        }
    }
    return best;
}

static s32 ndsRendererHardwareSecondCyclePassesCombined(
    const NDSRendererStats *stats)
{
    u32 w0;
    u32 w1;

    if (stats == NULL)
    {
        return FALSE;
    }
    if (ndsRendererHardwareUseSecondCycle(stats) == FALSE)
    {
        return TRUE;
    }
    if (stats->env_color != 0xffffffffu)
    {
        return FALSE;
    }

    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    /* BattleShip's normal fighter mode uses (COMBINED - 0) * ENV + 0.
     * With the opaque-white environment selected at ftdisplaymain.c:1192-1196,
     * cycle 2 preserves the source material/shade result from cycle 1. */
    return ((((w0 >> 5) & 0x0fu) == NDS_RENDERER_CCMUX_COMBINED) &&
            (((w1 >> 24) & 0x0fu) == NDS_RENDERER_CCMUX_ZERO_AB) &&
            (((w0 >> 0) & 0x1fu) == NDS_RENDERER_CCMUX_ENVIRONMENT) &&
            (((w1 >> 6) & 0x07u) == NDS_RENDERER_CCMUX_ZERO_D)) ? TRUE :
                                                                  FALSE;
}

static s32 ndsRendererHardwareOutputUsesColor(const NDSRendererStats *stats,
                                              u32 source)
{
    u32 w0;
    u32 w1;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return FALSE;
    }
    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    if (ndsRendererHardwareUseSecondCycle(stats) == FALSE)
    {
        return ndsRendererCombineOutputUsesColor(w0, w1, source);
    }
    if (ndsRendererCombineSecondOutputUsesColor(w0, w1, source) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererCombineSecondOutputUsesColor(
            w0, w1, NDS_RENDERER_CCMUX_COMBINED) != FALSE)
    {
        return ndsRendererCombineOutputUsesColor(w0, w1, source);
    }
    return FALSE;
}

static s32 ndsRendererCombineUsesAlpha(u32 w0, u32 w1, u32 source)
{
    return ((((w0 >> 9) & 0x07u) == source) ||
            (((w1 >> 9) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererCombineSecondOutputUsesAlpha(u32 w1, u32 source)
{
    return ((((w1 >> 18) & 0x07u) == source) ||
            (((w1 >> 0) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareOutputUsesAlpha(const NDSRendererStats *stats,
                                              u32 source)
{
    u32 w0;
    u32 w1;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return FALSE;
    }
    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    if (ndsRendererHardwareUseSecondCycle(stats) == FALSE)
    {
        return ndsRendererCombineUsesAlpha(w0, w1, source);
    }
    if (ndsRendererCombineSecondOutputUsesAlpha(w1, source) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererCombineSecondOutputUsesAlpha(
            w1, NDS_RENDERER_ACMUX_COMBINED) != FALSE)
    {
        return ndsRendererCombineUsesAlpha(w0, w1, source);
    }
    return FALSE;
}

/* b == d IS NOT A DECAL WHEN THE TEXTURE IS THE WEIGHT.
 *
 * The `b0 == d0` test is sm64-nds's g_setcombine rule, transcribed whole
 * (decomp/sm64-nds/src/nds/nds_renderer.c:928): it forces POLY_DECAL, and when
 * `a0` is PRIMITIVE it additionally sets `use_texture = false`. Both halves are
 * wrong for `(a - b) * TEXEL0 + b`, which is not a decal at all but a LERP from
 * b to a with the texture as its weight -- dropping the texture collapses it to
 * the flat colour b, and DS decal mode replaces the texel-weighted blend with
 * "texel where opaque, polygon colour where not".
 *
 * SSB64 writes exactly that shape as G_CC_BLENDPE -- (PRIMITIVE - ENVIRONMENT)
 * * TEXEL0 + ENVIRONMENT -- and it is the standard translucent-effect combine,
 * not one asset's quirk: the same two word pairs (0xFC309661/0x552EFF7F and
 * 0xFC30FE61/0x55FEF379) appear in FTManagerCommon (the shield),
 * EFCommonEffects1/2/3 (rebirth halo, impact wave), FoxSpecial3 and
 * ITCommonObject. So the shield's 16x32 IA8 circular alpha never reached its
 * polygon: gNdsEffectRendererTextureReadyCount AND TextureRejectCount both read
 * 0, because ndsRendererHardwareUseTexture refused before any bind was
 * attempted, and the quad drew as a flat env-coloured SQUARE.
 *
 * Requiring the multiplier to be TEXEL0/TEXEL1 keeps every genuinely untextured
 * decal (c = SHADE, PRIM_A, TEXEL0_A, LOD_FRAC, ...) on the sm64-nds rule. The
 * relocData corpus was censused for the blast radius: 284 of the 3,210 b0 == d0
 * combines in 2,132 files change, and inside the P1 battle only FTManagerCommon,
 * EFCommonEffects1/2/3, FoxSpecial3 and ITCommonObject carry one -- the Mario
 * and Fox models and the Dream Land stage carry none. */
static s32 ndsRendererHardwareUseDecal(const NDSRendererStats *stats)
{
    u32 w0;
    u32 w1;
    u32 multiplier;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return FALSE;
    }
    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    if (((w1 >> 28) & 0x0fu) != ((w1 >> 15) & 0x07u))
    {
        return FALSE;
    }
    multiplier = (w0 >> 15) & 0x1fu;
    return (((multiplier == NDS_RENDERER_CCMUX_TEXEL0) ||
             (multiplier == NDS_RENDERER_CCMUX_TEXEL1)) ? FALSE : TRUE);
}

static s32 ndsRendererHardwareUsePrimDepth(const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            ((stats->othermode_l & NDS_RENDERER_ZSOURCE_MASK) ==
             NDS_RENDERER_ZSOURCE_PRIM)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwarePrimitiveDecal(const NDSRendererStats *stats)
{
    if (ndsRendererHardwareUseDecal(stats) == FALSE)
    {
        return FALSE;
    }
    return ((((stats->texture_combine_w0 >> 20) & 0x0fu) ==
             NDS_RENDERER_CCMUX_PRIMITIVE) ? TRUE : FALSE);
}

static void ndsRendererHardwareRecordUseTextureReject(
    const NDSRendererStats *stats,
    u32 reason)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    switch (reason)
    {
    case NDS_RENDERER_HW_USETEX_REJECT_NO_STATS:
        gNdsRendererProfileUseTextureRejectNoStatsCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_STATE_OFF:
        gNdsRendererProfileUseTextureRejectStateOffCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_NO_COMBINE:
        gNdsRendererProfileUseTextureRejectNoCombineCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_PRIMITIVE_DECAL:
        gNdsRendererProfileUseTextureRejectPrimitiveDecalCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_NO_TEXEL0:
        gNdsRendererProfileUseTextureRejectNoTexel0Count++;
        break;
    default:
        break;
    }
    if (gNdsRendererProfileUseTextureRejectFirstReason == 0u)
    {
        gNdsRendererProfileUseTextureRejectFirstReason = reason;
        if (stats != NULL)
        {
            gNdsRendererProfileUseTextureRejectFirstFlags =
                stats->texture_state_flags;
            gNdsRendererProfileUseTextureRejectFirstW0 =
                stats->texture_combine_w0;
            gNdsRendererProfileUseTextureRejectFirstW1 =
                stats->texture_combine_w1;
            gNdsRendererProfileUseTextureRejectFirstGeometry =
                stats->geometry_mode;
        }
    }
#else
    (void)stats;
    (void)reason;
#endif
}

static s32 ndsRendererHardwareTextureImplicitStateOn(
    const NDSRendererStats *stats)
{
    const NDSRendererTileState *render_tile;
    u32 required_mask;

    if ((stats == NULL) ||
        ((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_ON) != 0u))
    {
        return FALSE;
    }

    required_mask = NDS_RENDERER_TEXTURE_SETTIMG |
        NDS_RENDERER_TEXTURE_SETTILE |
        NDS_RENDERER_TEXTURE_SETTILESIZE;
    if (((stats->texture_mask & required_mask) != required_mask) ||
        ((stats->texture_mask &
          (NDS_RENDERER_TEXTURE_LOADBLOCK | NDS_RENDERER_TEXTURE_LOADTILE)) ==
         0u) ||
        (stats->texture_image == 0u) ||
        (stats->texture_load_texels == 0u))
    {
        return FALSE;
    }

    render_tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    return ((render_tile->set_seen != 0u) &&
            (render_tile->size_seen != 0u) &&
            (render_tile->line != 0u) &&
            (render_tile->width != 0u) &&
            (render_tile->height != 0u)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareUseTexture(const NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        ndsRendererHardwareRecordUseTextureReject(
            NULL, NDS_RENDERER_HW_USETEX_REJECT_NO_STATS);
        return FALSE;
    }
    if ((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_ON) == 0u)
    {
        if (ndsRendererHardwareTextureImplicitStateOn(stats) == FALSE)
        {
            ndsRendererHardwareRecordUseTextureReject(
                stats, NDS_RENDERER_HW_USETEX_REJECT_STATE_OFF);
            return FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileUseTextureImplicitOnCount++;
#endif
    }
    if (stats->texture_combine_count == 0u)
    {
        ndsRendererHardwareRecordUseTextureReject(
            stats, NDS_RENDERER_HW_USETEX_REJECT_NO_COMBINE);
        return FALSE;
    }
    if (ndsRendererHardwarePrimitiveDecal(stats) != FALSE)
    {
        ndsRendererHardwareRecordUseTextureReject(
            stats, NDS_RENDERER_HW_USETEX_REJECT_PRIMITIVE_DECAL);
        return FALSE;
    }
    if (ndsRendererCombineUsesColor(
            stats->texture_combine_w0, stats->texture_combine_w1,
            NDS_RENDERER_CCMUX_TEXEL0) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererHardwareOutputUsesAlpha(
            stats, NDS_RENDERER_ACMUX_TEXEL0) != FALSE)
    {
        return TRUE;
    }
    ndsRendererHardwareRecordUseTextureReject(
        stats, NDS_RENDERER_HW_USETEX_REJECT_NO_TEXEL0);
    return FALSE;
}

static s32 ndsRendererHardwareUsesLitPrimitiveModulate(
    const NDSRendererStats *stats)
{
    u32 a;
    u32 b;
    u32 c;
    u32 d;

    if ((stats == NULL) || (stats->texture_combine_count == 0u) ||
        (ndsRendererHardwareSecondCyclePassesCombined(stats) == FALSE))
    {
        return FALSE;
    }

    a = (stats->texture_combine_w0 >> 20) & 0x0fu;
    b = (stats->texture_combine_w1 >> 28) & 0x0fu;
    c = (stats->texture_combine_w0 >> 15) & 0x1fu;
    d = (stats->texture_combine_w1 >> 15) & 0x07u;
    if ((b != NDS_RENDERER_CCMUX_ZERO_AB) ||
        (d != NDS_RENDERER_CCMUX_ZERO_D))
    {
        return FALSE;
    }
    return (((a == NDS_RENDERER_CCMUX_PRIMITIVE) &&
             (c == NDS_RENDERER_CCMUX_SHADE)) ||
            ((a == NDS_RENDERER_CCMUX_SHADE) &&
             (c == NDS_RENDERER_CCMUX_PRIMITIVE))) ? TRUE : FALSE;
}

static u32 ndsRendererHardwareColorSource(const NDSRendererStats *stats)
{
    if (ndsRendererHardwarePrimEnvTexel0BlendMode(stats) !=
        NDS_RENDERER_PRIM_ENV_BLEND_NONE)
    {
        /* The converted texture already contains both captured endpoints. */
        return 0xffffffffu;
    }
    if (ndsRendererHardwareUsesLitPrimitiveModulate(stats) != FALSE)
    {
        return stats->prim_color;
    }
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if (ndsRendererHardwareOutputUsesColor(
                stats, NDS_RENDERER_CCMUX_ENVIRONMENT) != FALSE)
        {
            return stats->env_color;
        }
        if (ndsRendererHardwareOutputUsesColor(
                stats, NDS_RENDERER_CCMUX_PRIMITIVE) != FALSE)
        {
            return stats->prim_color;
        }
    }
    return 0u;
}

static s32 ndsRendererHardwareLitShadeCombine(const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            (stats->texture_combine_count != 0u) &&
            ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
            (ndsRendererCombineUsesColor(stats->texture_combine_w0,
                                         stats->texture_combine_w1,
                                         NDS_RENDERER_CCMUX_SHADE) != FALSE)) ?
        TRUE : FALSE;
}

static s32 ndsRendererHardwareUseMaterialColor(const NDSRendererStats *stats)
{
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if (ndsRendererHardwareLitShadeCombine(stats) != FALSE)
        {
            return ndsRendererHardwareUsesLitPrimitiveModulate(stats);
        }
        return ((ndsRendererHardwareOutputUsesColor(
                     stats, NDS_RENDERER_CCMUX_ENVIRONMENT) != FALSE) ||
                (ndsRendererHardwareOutputUsesColor(
                     stats, NDS_RENDERER_CCMUX_PRIMITIVE) != FALSE)) ?
            TRUE : FALSE;
    }
    return FALSE;
}

static s32 ndsRendererHardwareUseVertexColor(const NDSRendererStats *stats)
{
    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return TRUE;
    }
    if (ndsRendererHardwareLitShadeCombine(stats) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererHardwareUseMaterialColor(stats) != FALSE)
    {
        return FALSE;
    }
    return ndsRendererCombineUsesColor(
        stats->texture_combine_w0, stats->texture_combine_w1,
        NDS_RENDERER_CCMUX_SHADE);
}

static s32 ndsRendererHardwareBlendAlphaUsesMemory(
    const NDSRendererStats *stats)
{
    u32 mode;

    if (stats == NULL)
    {
        return FALSE;
    }
    mode = stats->othermode_l;
    if (((mode >> NDS_RENDERER_BLEND_ALPHA_CYCLE1_SHIFT) &
         NDS_RENDERER_BLEND_ALPHA_BITS_MASK) == NDS_RENDERER_G_BL_A_MEM)
    {
        return TRUE;
    }
    return ((ndsRendererHardwareUseSecondCycle(stats) != FALSE) &&
            (((mode >> NDS_RENDERER_BLEND_ALPHA_CYCLE2_SHIFT) &
              NDS_RENDERER_BLEND_ALPHA_BITS_MASK) ==
             NDS_RENDERER_G_BL_A_MEM)) ? TRUE : FALSE;
}

static u32 ndsRendererHardwareAlpha(const NDSRendererStats *stats,
                                    const NDSRendererInputVertex *vtx)
{
    u32 alpha = 0xffu;

    if (vtx != NULL)
    {
        alpha = vtx->a;
    }
    if ((stats != NULL) &&
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) !=
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD) &&
        ((stats->othermode_l & NDS_RENDERER_FORCE_BL) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_CVG_X_ALPHA) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_ZMODE_MASK) !=
         NDS_RENDERER_ZMODE_XLU))
    {
        return 31u;
    }
    if (ndsRendererHardwareBlendAlphaUsesMemory(stats) != FALSE)
    {
        return 31u;
    }
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if (ndsRendererHardwareBlendModeKeepsTexelCoverage(
                ndsRendererHardwarePrimEnvTexel0BlendMode(stats)) != FALSE)
        {
            /* GL_RGBA retains TEXEL0 coverage; do not multiply vertex alpha. */
            return 31u;
        }
        if (ndsRendererHardwareOutputUsesAlpha(
                stats, NDS_RENDERER_ACMUX_PRIMITIVE) != FALSE)
        {
            alpha = stats->prim_color & 0xffu;
        }
        else if (ndsRendererHardwareOutputUsesAlpha(
                     stats, NDS_RENDERER_ACMUX_ENVIRONMENT) != FALSE)
        {
            alpha = stats->env_color & 0xffu;
        }
        else if ((ndsRendererHardwareOutputUsesAlpha(
                      stats, NDS_RENDERER_ACMUX_TEXEL0) == FALSE) &&
                 (ndsRendererHardwareOutputUsesAlpha(
                      stats, NDS_RENDERER_ACMUX_SHADE) == FALSE))
        {
            alpha = (ndsRendererHardwareOutputUsesAlpha(
                         stats, NDS_RENDERER_ACMUX_0) != FALSE) ?
                0u : 0xffu;
        }
    }
    return alpha >> 3;
}

static s32 ndsRendererHardwareAlphaUsesVertex(
    const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) !=
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD) &&
        ((stats->othermode_l & NDS_RENDERER_FORCE_BL) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_CVG_X_ALPHA) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_ZMODE_MASK) !=
         NDS_RENDERER_ZMODE_XLU))
    {
        return FALSE;
    }
    if (ndsRendererHardwareBlendAlphaUsesMemory(stats) != FALSE)
    {
        return FALSE;
    }
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if (ndsRendererHardwareBlendModeKeepsTexelCoverage(
                ndsRendererHardwarePrimEnvTexel0BlendMode(stats)) != FALSE)
        {
            return FALSE;
        }
        if ((ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_PRIMITIVE) != FALSE) ||
            (ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_ENVIRONMENT) != FALSE))
        {
            return FALSE;
        }
        if ((ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_TEXEL0) == FALSE) &&
            (ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_SHADE) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* Task 37: 100 bytes, 3.60 cycles per instruction, on the per-polygon submit
 * path. */
static u32 NDS_R2_ITCM_PACK2_EVICTED_PLAIN_CODE ndsRendererHardwarePolyFmt(
    const NDSRendererStats *stats, u32 alpha)
{
    u32 poly_id = (stats != NULL) ?
        (stats->texture_combine_count & NDS_RENDERER_POLY_ID_MASK) : 0u;
    u32 poly_fmt = POLY_CULL_NONE | POLY_ALPHA(alpha) | POLY_ID(poly_id);
    u32 mode = (stats != NULL) ? stats->geometry_mode : 0u;

    if (ndsRendererHardwareUseDecal(stats) != FALSE)
    {
        poly_fmt |= POLY_DECAL;
    }
    if ((mode & NDS_RENDERER_GEOM_FOG) != 0u)
    {
        poly_fmt |= POLY_FOG;
    }
    if ((mode & NDS_RENDERER_GEOM_CULL_FRONT) != 0u)
    {
        poly_fmt &= ~((u32)POLY_CULL_BACK);
    }
    if ((mode & NDS_RENDERER_GEOM_CULL_BACK) != 0u)
    {
        poly_fmt &= ~((u32)POLY_CULL_FRONT);
    }
    return poly_fmt;
}

static void ndsRendererHardwareApplyAlphaTest(const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) ==
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD))
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc((stats->blend_color & 0xffu) >> 4);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }
}

static void ndsRendererHardwareApplyFog(const NDSRendererStats *stats)
{
    s32 range;
    s32 shift;
    s32 density;
    s32 inc;
    s32 i;
    u32 color;

    if ((stats == NULL) ||
        (stats->fog_status == 0u) ||
        (stats->fog_max <= stats->fog_min))
    {
        glDisable(GL_FOG);
        return;
    }

    range = stats->fog_max - stats->fog_min;
    shift = 0;
    for (i = 500; (i >= range) && (i > 0); i >>= 1)
    {
        shift++;
    }

    density = 0;
    inc = (((128 * 1000) << 1) / (range * 32) + 1) >> (shift + 1);
    if (inc < 1)
    {
        inc = 1;
    }
    for (i = 0; i < 32; i++)
    {
        glFogDensity(i, density);
        density += inc;
        if (density > 127)
        {
            density = 127;
        }
    }

    color = stats->fog_color;
    glFogShift(shift);
    glFogOffset((stats->fog_min * 0x7fff / 1000) - (0x400 >> shift));
    glFogColor((color >> 27) & 0x1fu, (color >> 19) & 0x1fu,
               (color >> 11) & 0x1fu, (color >> 3) & 0x1fu);
    glEnable(GL_FOG);
}

static s32 ndsRendererHardwareTextureFilterOffset(
    const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->othermode_h & NDS_RENDERER_TEXTFILT_MASK) !=
         NDS_RENDERER_TF_POINT))
    {
        return NDS_RENDERER_TEXCOORD_FILTER_OFFSET;
    }
    return 0;
}

static s32 ndsRendererHardwareUseTextureMatrix(
    const NDSRendererStats *stats)
{
    return ((stats == NULL) ||
            ((stats->othermode_h & NDS_RENDERER_TEXTPERSP_MASK) ==
             NDS_RENDERER_TP_PERSP)) ? TRUE : FALSE;
}

static s16 ndsRendererHardwareTexCoord(s16 coord, u32 scale, u32 origin,
                                       s32 offset)
{
    s64 scaled_t16 = ((s64)coord * (s64)scale) >> 17;
    s64 origin_t16 = (s64)origin << 2;

    return (s16)(scaled_t16 - origin_t16 + offset);
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererProfileTextureCoord(s16 s, s16 t)
{
    gNdsRendererProfileTexturedVertexCount++;
    if (s < gNdsRendererProfileTextureCoordMinS)
    {
        gNdsRendererProfileTextureCoordMinS = s;
    }
    if (s > gNdsRendererProfileTextureCoordMaxS)
    {
        gNdsRendererProfileTextureCoordMaxS = s;
    }
    if (t < gNdsRendererProfileTextureCoordMinT)
    {
        gNdsRendererProfileTextureCoordMinT = t;
    }
    if (t > gNdsRendererProfileTextureCoordMaxT)
    {
        gNdsRendererProfileTextureCoordMaxT = t;
    }
}
#endif

static u32 ndsRendererHardwareColorByte(u32 color, u32 shift)
{
    return (color >> shift) & 0xffu;
}

static inline u32 ndsRendererHardwareScaleMaterialChannel5(
    u32 shaded, u32 material)
{
    u32 numerator = (shaded * material) + 127u;

    /* For every 16-bit numerator, floor(n / 255) is exactly
     * (n + 1 + (n >> 8)) >> 8.  Fold the following RGB15 divide by eight
     * into the same shift so the native owner does not issue three wide
     * constant divisions for every prepared vertex. */
    return (numerator + 1u + (numerator >> 8)) >> 11;
}

static u8 ndsRendererHardwareClampColor(s32 value)
{
    if (value < 0)
    {
        return 0u;
    }
    if (value > 255)
    {
        return 0xffu;
    }
    return (u8)value;
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static const u32 *
ndsRendererHardwareFindLightShadeLut(u32 diffuse, u32 ambient)
{
    u32 i;

    for (i = 0u; i < NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT; i++)
    {
        const NDSRendererHardwareLightShadeCacheEntry *entry =
            &sNdsRendererHardwareLightShadeCache[i];

        if ((entry->valid != 0u) &&
            (entry->diffuse == diffuse) &&
            (entry->ambient == ambient))
        {
            return entry->rgb;
        }
    }
    return NULL;
}

static const u32 * __attribute__((noinline))
ndsRendererHardwareGetLightShadeLut(u32 diffuse, u32 ambient)
{
    NDSRendererHardwareLightShadeCacheEntry *entry;
    const u32 *resident;
    u32 i;

    /* Cache only the exact RGB function of the two source light colors and
     * diffuse numerator. Vertex normals, direction, and alpha stay live. */
#if NDS_TASK90_SHADE_CENSUS
    gNdsTask90LutGetCalls++;
    gNdsTask90LutTraceDiffuse[gNdsTask90LutTraceNext] = diffuse;
    gNdsTask90LutTraceAmbient[gNdsTask90LutTraceNext] = ambient;
    gNdsTask90LutTraceNext =
        (gNdsTask90LutTraceNext + 1u) % NDS_TASK90_LUT_TRACE_COUNT;
#endif
    resident = ndsRendererHardwareFindLightShadeLut(diffuse, ambient);
    if (resident != NULL)
    {
        return resident;
    }
#if NDS_TASK90_SHADE_CENSUS
    gNdsTask90LutBuilds++;
#endif

    entry = &sNdsRendererHardwareLightShadeCache[
        sNdsRendererHardwareLightShadeCacheNext];
    sNdsRendererHardwareLightShadeCacheNext =
        (sNdsRendererHardwareLightShadeCacheNext + 1u) &
        (NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT - 1u);
    entry->valid = FALSE;
    entry->diffuse = diffuse;
    entry->ambient = ambient;
    for (i = 0u; i < NDS_RENDERER_HW_LIGHT_SHADE_LUT_COUNT; i++)
    {
        s32 r = (s32)ndsRendererHardwareColorByte(ambient, 24) +
            (s32)((ndsRendererHardwareColorByte(diffuse, 24) * i) / 127u);
        s32 g = (s32)ndsRendererHardwareColorByte(ambient, 16) +
            (s32)((ndsRendererHardwareColorByte(diffuse, 16) * i) / 127u);
        s32 b = (s32)ndsRendererHardwareColorByte(ambient, 8) +
            (s32)((ndsRendererHardwareColorByte(diffuse, 8) * i) / 127u);

        entry->rgb[i] =
            ((u32)ndsRendererHardwareClampColor(r) << 24) |
            ((u32)ndsRendererHardwareClampColor(g) << 16) |
            ((u32)ndsRendererHardwareClampColor(b) << 8);
    }
    entry->valid = TRUE;
    return entry->rgb;
}
#endif

static u32 ndsRendererHardwareLightColor(NDSRendererStats *stats, u32 mask,
                                         u32 color, u32 fallback)
{
    if ((stats == NULL) || ((stats->light_color_mask & mask) == 0u))
    {
        if (stats != NULL)
        {
            stats->light_fallback_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileLightFallbackCount++;
#endif
        }
        return fallback;
    }
    return color;
}

static void ndsRendererHardwarePrepareLitDirection(
    const NDSRendererStats *stats,
    const NDSRendererMatrix20p12 *modelview,
    NDSRendererHardwareLightDirection *out)
{
    s32 light_x;
    s32 light_y;
    s32 light_z;

    if (out == NULL)
    {
        return;
    }

    light_x = (stats != NULL) ? stats->light_dir_x : 0;
    light_y = (stats != NULL) ? stats->light_dir_y : 0;
    light_z = (stats != NULL) ? stats->light_dir_z : 0;
    if ((stats != NULL) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u) &&
        (modelview != NULL))
    {
#if NDS_SCENE_MIP_CACHE_LAB && (NDS_RENDERER_PROFILE_LEVEL < 2)
        s64 transformed_x =
            (s64)light_x * modelview->m[0][0] +
            (s64)light_y * modelview->m[0][1] +
            (s64)light_z * modelview->m[0][2];
        s64 transformed_y =
            (s64)light_x * modelview->m[1][0] +
            (s64)light_y * modelview->m[1][1] +
            (s64)light_z * modelview->m[1][2];
        s64 transformed_z =
            (s64)light_x * modelview->m[2][0] +
            (s64)light_y * modelview->m[2][1] +
            (s64)light_z * modelview->m[2][2];
        s64 length_squared =
            (transformed_x * transformed_x) +
            (transformed_y * transformed_y) +
            (transformed_z * transformed_z);

        if (length_squared > 0)
        {
            u32 length = ndsR2HwMathSqrt64((u64)length_squared);

            if (length != 0u)
            {
                light_x = (s32)ndsR2HwMathDiv64(transformed_x * 127,
                                               (s32)length);
                light_y = (s32)ndsR2HwMathDiv64(transformed_y * 127,
                                               (s32)length);
                light_z = (s32)ndsR2HwMathDiv64(transformed_z * 127,
                                               (s32)length);
            }
        }
#else
        f32 transformed_x = (f32)(
            (s64)light_x * modelview->m[0][0] +
            (s64)light_y * modelview->m[0][1] +
            (s64)light_z * modelview->m[0][2]);
        f32 transformed_y = (f32)(
            (s64)light_x * modelview->m[1][0] +
            (s64)light_y * modelview->m[1][1] +
            (s64)light_z * modelview->m[1][2]);
        f32 transformed_z = (f32)(
            (s64)light_x * modelview->m[2][0] +
            (s64)light_y * modelview->m[2][1] +
            (s64)light_z * modelview->m[2][2]);
        f32 length = sqrtf((transformed_x * transformed_x) +
                           (transformed_y * transformed_y) +
                           (transformed_z * transformed_z));

        if (length > 0.0F)
        {
            light_x = (s32)((transformed_x * 127.0F) / length);
            light_y = (s32)((transformed_y * 127.0F) / length);
            light_z = (s32)((transformed_z * 127.0F) / length);
        }
#endif
    }

    out->x = light_x;
    out->y = light_y;
    out->z = light_z;
}

static u32 ndsRendererHardwareLitDiffuseNumer(
    const NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction)
{
    s32 light_x;
    s32 light_y;
    s32 light_z;
    s32 dot;

    if ((stats == NULL) || (vtx == NULL) ||
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) == 0u))
    {
        return 127u;
    }

    light_x = (direction != NULL) ? direction->x : stats->light_dir_x;
    light_y = (direction != NULL) ? direction->y : stats->light_dir_y;
    light_z = (direction != NULL) ? direction->z : stats->light_dir_z;

    dot = ((s32)(s8)vtx->r * light_x) +
        ((s32)(s8)vtx->g * light_y) +
        ((s32)(s8)vtx->b * light_z);
    if (dot <= 0)
    {
        return 0u;
    }
    if (dot > (127 * 127))
    {
        return 127u;
    }
    return ndsRendererHardwareDivideLitDotBy127((u32)dot);
}

#if NDS_R2_FLASH_PROBE
/* R2-03 E59. The probe's storage is defined ~100 lines below, next to the rest
 * of the flash slots; this shade function is the first user of it. */
extern volatile u32 gNdsR2FlashLive[];
#endif

static u32 NDS_R2_ITCM_PACK2_EVICTED_CODE
ndsRendererHardwareLitShadeColorPrepared(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction)
{
    u32 light_1;
    u32 light_2;
    u32 ambient;
    u32 diffuse;
    u32 diffuse_numer;
    s32 r;
    s32 g;
    s32 b;

    if (vtx == NULL)
    {
        return 0xffffffffu;
    }
    if ((stats == NULL) ||
        ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) == 0u))
    {
        return ((u32)vtx->r << 24) | ((u32)vtx->g << 16) |
            ((u32)vtx->b << 8) | (u32)vtx->a;
    }

    light_1 = ndsRendererHardwareLightColor(
        stats, NDS_RENDERER_LIGHT_COLOR_1_MASK, stats->light_color_1,
        NDS_RENDERER_LIGHT_COLOR_1_FALLBACK);
    light_2 = ndsRendererHardwareLightColor(
        stats, NDS_RENDERER_LIGHT_COLOR_2_MASK, stats->light_color_2,
        NDS_RENDERER_LIGHT_COLOR_2_FALLBACK);
#if NDS_R2_FLASH_PROBE
    /* R2-03 E59. E58 established the emitted colour is built from these two, so
     * "is the flash a light-colour change?" is answered by whether they move
     * between a hitlag frame and an ordinary one. This is the GENERIC path's
     * resolved pair, after the mask/fallback logic -- the value that actually
     * produced the greys and reds E55 sampled. */
    gNdsR2FlashLive[12] = light_1;
    gNdsR2FlashLive[13] = light_2;
    gNdsR2FlashLive[14] = stats->light_color_mask;
#endif
    diffuse = light_1;
    ambient = light_2;

    diffuse_numer = ndsRendererHardwareLitDiffuseNumer(stats, vtx, direction);
    r = (s32)ndsRendererHardwareColorByte(ambient, 24) +
        (s32)((ndsRendererHardwareColorByte(diffuse, 24) * diffuse_numer) /
              127u);
    g = (s32)ndsRendererHardwareColorByte(ambient, 16) +
        (s32)((ndsRendererHardwareColorByte(diffuse, 16) * diffuse_numer) /
              127u);
    b = (s32)ndsRendererHardwareColorByte(ambient, 8) +
        (s32)((ndsRendererHardwareColorByte(diffuse, 8) * diffuse_numer) /
              127u);

    return ((u32)ndsRendererHardwareClampColor(r) << 24) |
        ((u32)ndsRendererHardwareClampColor(g) << 16) |
        ((u32)ndsRendererHardwareClampColor(b) << 8) | (u32)vtx->a;
}

static u32 ndsRendererHardwareLitShadeColor(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererMatrix20p12 *modelview)
{
    NDSRendererHardwareLightDirection direction;
    const NDSRendererHardwareLightDirection *prepared_direction = NULL;

    if ((stats != NULL) &&
        ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        ndsRendererHardwarePrepareLitDirection(stats, modelview, &direction);
        prepared_direction = &direction;
    }
    return ndsRendererHardwareLitShadeColorPrepared(
        stats, vtx, prepared_direction);
}

static inline u16 ndsRendererHardwareModulatePackedColor(
    u16 color, u32 color_modulate)
{
#if NDS_TASK39_FX_FLASH
    u32 alpha = color_modulate & 0xffu;

    if (alpha != 0u)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        u32 start = cpuGetTiming();
#endif
        u32 inverse = 255u - alpha;
        u32 red = (((u32)(color & 0x1fu) * inverse) +
                   (((color_modulate >> 27) & 0x1fu) * alpha) + 127u) /
                  255u;
        u32 green = (((u32)((color >> 5) & 0x1fu) * inverse) +
                     (((color_modulate >> 19) & 0x1fu) * alpha) + 127u) /
                    255u;
        u32 blue = (((u32)((color >> 10) & 0x1fu) * inverse) +
                    (((color_modulate >> 11) & 0x1fu) * alpha) + 127u) /
                   255u;

        color = RGB15(red, green, blue);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        ndsTask39EffectsAddDrawTicks(cpuGetTiming() - start);
#endif
    }
#else
    (void)color_modulate;
#endif
    return color;
}

#if NDS_R2_FLASH_PROBE
/* R2-03 E48. Which branch of the generic colour path draws the hurt flash.
 *
 * Four hypotheses have been spent guessing at E32's regression -- E36
 * (color_modulate), E41 (fold arithmetic, then E16's hardware lighting), E42
 * (USE_VERTEX) and E47 (the material derivation) -- and each cost a build. The
 * campaign's own standing rule is to bracket the thing rather than reason about
 * it, so this records what the generic path actually does instead.
 *
 * Counters are per presented frame and latched at two named frames, because a
 * cumulative total is dominated by stage geometry and says nothing about the
 * fighter. Frame A is a hitlag frame and frame B an ordinary one; the branch
 * that differs between the two snapshots is the flash. Same matched-control
 * shape R2-03 E35 used to attribute the SRC excursion.
 *
 * Slots: 0 material-only (generic emits the material colour), 1 no-vertex
 * (emits RGB15(31,31,31) -- pure white), 2 resolved (shade combined with
 * material), 3 lit-shade recompute, 4 total calls, 5 last material colour,
 * 6 last packed result, 7 last flags: bit0 use_material, bit1 use_vertex,
 * bit2 vertex_color_valid.
 *
 * R2-03 E50 adds the uniformity question E49 left open. E49 proved the baked
 * dense colours cannot carry the flash (they are `static const`), so the only
 * cheap fix left is a per-epoch constant colour -- and that is viable only if
 * the flash really is one colour across the fighter. Slots 8..11 answer it
 * directly: 8 min vertex colour, 9 max, 10 the first one seen, 11 how many
 * differed from the first. Slot 11 == 0 means uniform and the constant-colour
 * fix is on; anything else and it is per-vertex and that option dies too. */
#define NDS_R2_FLASH_SLOTS 20u
#define NDS_R2_FLASH_SLOT_MIN 8u
/* R2-03 E55: per-vertex samples in call order, for the two-hitlag-frame ratio
 * test described at the record site. 24 is enough to separate "constant ratio"
 * from "scattered" and small enough to read in one GDB stop. */
#define NDS_R2_FLASH_VTX 24u
volatile u32 gNdsR2FlashLive[NDS_R2_FLASH_SLOTS];
volatile u32 gNdsR2FlashSnapA[NDS_R2_FLASH_SLOTS];
volatile u32 gNdsR2FlashSnapB[NDS_R2_FLASH_SLOTS];
volatile u32 gNdsR2FlashVtxLive[NDS_R2_FLASH_VTX];
volatile u32 gNdsR2FlashVtxA[NDS_R2_FLASH_VTX];
volatile u32 gNdsR2FlashVtxB[NDS_R2_FLASH_VTX];
/* R2-03 E58. The RAW decoded source colour for the same sampled vertices.
 * gNdsR2FlashVtx* holds state->vertex_colors[], which is LitShade(source) --
 * lighting sits between the flash and that value, and E50 mistook the lighting's
 * per-vertex variation for the flash's. `vtx` at this call site is the decoded
 * NDSRendererInputVertex, i.e. the source bytes the flash actually rewrites, so
 * this array answers the question with no lighting in the way: is the flashed
 * INPUT one constant? Recorded in the same branch at the same stride as the lit
 * array, so index k names one vertex in both and they cannot desync. */
volatile u32 gNdsR2FlashRawLive[NDS_R2_FLASH_VTX];
volatile u32 gNdsR2FlashRawA[NDS_R2_FLASH_VTX];
/* The recording branch lives in ndsRendererHardwarePackedValidVertexColor, which
 * is `static inline` and does not take the input vertex. Rather than thread a
 * probe-only parameter through four call sites, the caller that HAS the vertex
 * parks it here immediately before delegating. The only other caller of that
 * function is the native stage path, which E48 proved never reaches the
 * recording branch (0 calls on an ordinary frame); it clears this anyway, so a
 * stage contribution would read as 0 and be visible rather than silently stale. */
volatile u32 gNdsR2FlashRawPending;
/* Writable over GDB so a run can be re-aimed without a rebuild. E55 pointed B at
 * a SECOND hitlag frame because the slots it read (0..14) are generic-path only
 * and the generic path has 0 calls on an ordinary frame (E48), so an
 * ordinary-frame snapshot was empty by construction.
 *
 * E59 adds owner-side slots 15..19, which run on EVERY frame, so B goes back to
 * an ordinary frame and each snapshot answers a different question:
 *   A(12..14) vs A(15..17) -- do the two paths see different light state on the
 *                             same frame? (E54: only one fighter falls back, so
 *                             911 runs both paths at once.)
 *   A(15..18) vs B(15..18) -- does the OWNER's own light state move during
 *                             hitlag at all? B(12..14) is 0 by construction. */
volatile u32 gNdsR2FlashFrameA = 911u;
volatile u32 gNdsR2FlashFrameB = 904u;
volatile u32 gNdsR2FlashLatchedA;
volatile u32 gNdsR2FlashLatchedB;

void ndsRendererR2FlashProbeFrameEnd(u32 presented_frame)
{
    u32 i;

    if (presented_frame == gNdsR2FlashFrameA)
    {
        for (i = 0u; i < NDS_R2_FLASH_SLOTS; i++)
        {
            gNdsR2FlashSnapA[i] = gNdsR2FlashLive[i];
        }
        for (i = 0u; i < NDS_R2_FLASH_VTX; i++)
        {
            gNdsR2FlashVtxA[i] = gNdsR2FlashVtxLive[i];
            gNdsR2FlashRawA[i] = gNdsR2FlashRawLive[i];
        }
        gNdsR2FlashLatchedA++;
    }
    else if (presented_frame == gNdsR2FlashFrameB)
    {
        for (i = 0u; i < NDS_R2_FLASH_SLOTS; i++)
        {
            gNdsR2FlashSnapB[i] = gNdsR2FlashLive[i];
        }
        for (i = 0u; i < NDS_R2_FLASH_VTX; i++)
        {
            gNdsR2FlashVtxB[i] = gNdsR2FlashVtxLive[i];
        }
        gNdsR2FlashLatchedB++;
    }
    for (i = 0u; i < NDS_R2_FLASH_SLOTS; i++)
    {
        gNdsR2FlashLive[i] = 0u;
    }
    for (i = 0u; i < NDS_R2_FLASH_VTX; i++)
    {
        gNdsR2FlashVtxLive[i] = 0u;
        gNdsR2FlashRawLive[i] = 0u;
    }
    /* Min starts at the top so the first sample always wins. */
    gNdsR2FlashLive[NDS_R2_FLASH_SLOT_MIN] = 0xffffffffu;
}
#endif

static inline u16 ndsRendererHardwarePackedResolvedColor(
    u32 color,
    u32 material_color,
    s32 use_material_color,
    u32 color_modulate)
{
    u16 packed;

    if (use_material_color != FALSE)
    {
        u32 r = ((ndsRendererHardwareColorByte(color, 24) *
                  ndsRendererHardwareColorByte(material_color, 24)) + 127u) /
            255u;
        u32 g = ((ndsRendererHardwareColorByte(color, 16) *
                  ndsRendererHardwareColorByte(material_color, 16)) + 127u) /
            255u;
        u32 b = ((ndsRendererHardwareColorByte(color, 8) *
                  ndsRendererHardwareColorByte(material_color, 8)) + 127u) /
            255u;

        packed = RGB15((u8)(r >> 3), (u8)(g >> 3), (u8)(b >> 3));
    }
    else
    {
        packed = RGB15((u8)((color >> 27) & 0x1fu),
                       (u8)((color >> 19) & 0x1fu),
                       (u8)((color >> 11) & 0x1fu));
    }
    return ndsRendererHardwareModulatePackedColor(packed,
                                                   color_modulate);
}

static inline u16 ndsRendererHardwarePackedValidVertexColor(
    u32 material_color,
    s32 use_material_color,
    s32 use_vertex_color,
    u32 vertex_color,
    u32 color_modulate)
{
    if ((use_material_color != FALSE) && (use_vertex_color == FALSE))
    {
        u16 packed = ndsRendererHardwareModulatePackedColor(
            RGB15((u8)((material_color >> 27) & 0x1fu),
                  (u8)((material_color >> 19) & 0x1fu),
                  (u8)((material_color >> 11) & 0x1fu)),
            color_modulate);

#if NDS_R2_FLASH_PROBE
        gNdsR2FlashLive[0]++;
        gNdsR2FlashLive[5] = material_color;
        gNdsR2FlashLive[6] = packed;
        gNdsR2FlashLive[7] = 1u;
#endif
        return packed;
    }
    if (use_vertex_color == FALSE)
    {
        u16 packed = ndsRendererHardwareModulatePackedColor(
            RGB15(31u, 31u, 31u), color_modulate);

#if NDS_R2_FLASH_PROBE
        gNdsR2FlashLive[1]++;
        gNdsR2FlashLive[5] = material_color;
        gNdsR2FlashLive[6] = packed;
        gNdsR2FlashLive[7] = 0u;
#endif
        return packed;
    }
#if NDS_R2_FLASH_PROBE
    gNdsR2FlashLive[2]++;
    gNdsR2FlashLive[5] = material_color;
    gNdsR2FlashLive[7] = 2u | ((use_material_color != FALSE) ? 1u : 0u);
    /* E50: is the flash one colour, or per vertex? */
    if (vertex_color < gNdsR2FlashLive[NDS_R2_FLASH_SLOT_MIN])
    {
        gNdsR2FlashLive[NDS_R2_FLASH_SLOT_MIN] = vertex_color;
    }
    if (vertex_color > gNdsR2FlashLive[9])
    {
        gNdsR2FlashLive[9] = vertex_color;
    }
    if (gNdsR2FlashLive[2] == 1u)
    {
        gNdsR2FlashLive[10] = vertex_color;
    }
    else if (vertex_color != gNdsR2FlashLive[10])
    {
        gNdsR2FlashLive[11]++;
    }
    /* R2-03 E55. Per-vertex pairs across TWO hitlag frames, in call order.
     *
     * E50 killed the constant-colour route by showing 172/273 vertices differ.
     * What is still open is whether the flash is a one-parameter transform of
     * the base colour -- `flashed = base + (255 - base) * t` -- because then the
     * native owner reproduces it exactly from the baked table with one lerp and
     * E32 keeps its measured -51,136 (E54).
     *
     * The test needs no baked-table mapping, and it works because HITLAG FREEZES
     * THE POSE: frames 909..913 draw the same vertices with the same normals, so
     * the per-vertex lighting factor is identical between them and cancels. If
     * both frames are lerps of one base with t_A and t_B, then for every vertex
     *
     *     (255 - A_k) / (255 - B_k) = (1 - t_A) / (1 - t_B)
     *
     * a single constant. Constant across k confirms the route; scattered refutes
     * it. If A_k == B_k for all k the flash does not ramp within the burst, which
     * is a null result for this test and is worth knowing before spending more.
     *
     * Call order is deterministic for a frozen pose, so index k names the same
     * vertex in both snapshots. */
    {
        /* Stride, not prefix. The first 24 of 273 were all pure grey, which is
         * the whole finding -- so sampling the prefix again would only re-confirm
         * the part already known. Every 11th call spreads the 24 slots across the
         * entire vertex set and is what tests whether E50's non-grey minimum
         * (0x240F11FF) is a real minority or came from elsewhere. */
        u32 call = gNdsR2FlashLive[2] - 1u;

        if ((call % 11u) == 0u)
        {
            u32 slot = call / 11u;

            if (slot < NDS_R2_FLASH_VTX)
            {
                gNdsR2FlashVtxLive[slot] = vertex_color;
                gNdsR2FlashRawLive[slot] = gNdsR2FlashRawPending;
            }
        }
    }
#endif
    return ndsRendererHardwarePackedResolvedColor(
        vertex_color, material_color, use_material_color,
        color_modulate);
}

static u16 ndsRendererHardwarePackedVertexColor(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    u32 material_color,
    s32 use_material_color,
    s32 use_vertex_color,
    u32 vertex_color,
    s32 vertex_color_valid,
    u32 color_modulate)
{
    u32 color;

#if NDS_R2_FLASH_PROBE
    gNdsR2FlashLive[4]++;
    gNdsR2FlashRawPending = (vtx != NULL) ?
        (((u32)vtx->r << 24) | ((u32)vtx->g << 16) |
         ((u32)vtx->b << 8) | (u32)vtx->a) : 0u;
#endif
    if (vertex_color_valid != FALSE)
    {
        return ndsRendererHardwarePackedValidVertexColor(
            material_color, use_material_color,
            use_vertex_color, vertex_color, color_modulate);
    }
    if ((use_material_color != FALSE) && (use_vertex_color == FALSE))
    {
        return ndsRendererHardwarePackedValidVertexColor(
            material_color, use_material_color,
            use_vertex_color, vertex_color, color_modulate);
    }
    if (use_vertex_color == FALSE)
    {
        return ndsRendererHardwarePackedValidVertexColor(
            material_color, use_material_color,
            use_vertex_color, vertex_color, color_modulate);
    }
    color = ndsRendererHardwareLitShadeColor(stats, vtx, NULL);
#if NDS_R2_FLASH_PROBE
    gNdsR2FlashLive[3]++;
    gNdsR2FlashLive[5] = material_color;
    gNdsR2FlashLive[7] = 4u | ((use_material_color != FALSE) ? 1u : 0u);
#endif
    return ndsRendererHardwarePackedResolvedColor(
        color, material_color, use_material_color, color_modulate);
}

static const void *ndsRendererResolveTextureDataPointer(
    const NDSRendererConfig *config, const void *ptr, size_t bytes)
{
    if (ptr == NULL)
    {
        return NULL;
    }
    if ((config != NULL) && (config->resolve_data != NULL))
    {
        return config->resolve_data(ptr, bytes, config->user);
    }
    if ((config != NULL) && (config->validate_range != NULL) &&
        (config->validate_range((const Gfx *)ptr, bytes, config->user) ==
         FALSE))
    {
        return NULL;
    }
    return ptr;
}

static s32 ndsRendererHardwareTextureKeyEqual(
    const NDSRendererHardwareTextureKey *a,
    const NDSRendererHardwareTextureKey *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return FALSE;
    }
    return (memcmp(a, b, sizeof(*a)) == 0) ? TRUE : FALSE;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererProfileTextureKeyHashFull(
    const NDSRendererHardwareTextureKey *key)
{
    const u32 *words = (const u32 *)key;
    u32 hash = 0u;
    u32 i;

    if (key == NULL)
    {
        return 0u;
    }
    for (i = 0u; i < (sizeof(*key) / sizeof(*words)); i++)
    {
        hash = ndsRendererProfileHashU32(hash, words[i]);
    }
    return hash;
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
static u32 ndsRendererHardwareTextureKeyHash(
    const NDSRendererHardwareTextureKey *key)
{
    u32 hash;

    if (key == NULL)
    {
        return 0u;
    }
    /* This is an index fingerprint, not the equality oracle.  Mix the
     * high-entropy image/material identity and animated tile state, then keep
     * the full 236-byte comparison on every candidate hit.  Hashing all 59
     * words cost more ARM9 time than the open-address lookup saved. */
    hash = key->image ^ (key->tlut_image * 0x9e3779b9u);
    hash ^= key->texel1_image * 0x85ebca6bu;
    hash ^= (key->width << 16) ^ key->height;
    hash ^= (key->format << 28) ^ (key->size << 24) ^ key->flags;
    hash ^= (key->load_uls << 16) ^ key->load_ult ^ key->load_lrs;
    hash ^= (key->tile_uls << 16) ^ key->tile_ult ^ key->tile_lrs;
    hash ^= (key->texel1_load_uls << 16) ^ key->texel1_load_ult ^
        key->texel1_load_lrs;
    hash ^= (key->texel1_tile_uls << 16) ^ key->texel1_tile_ult ^
        key->texel1_tile_lrs;
    hash ^= key->prim_lod_fraction * 0xc2b2ae35u;
    hash ^= key->combine_w0 ^ (key->combine_w1 * 0x27d4eb2fu);
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    return hash;
}

static void ndsRendererHardwareTextureLookupInsert(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    u32 slot_value;
    u32 slot;
    u32 probe;

    if ((entry == NULL) || (entry->ready == 0u))
    {
        return;
    }
    slot_value = (u32)(entry - sNdsRendererHardwareTextureCache) + 1u;
    slot = entry->key_hash & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    for (probe = 0u; probe < NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT; probe++)
    {
        u32 value = sNdsRendererHardwareTextureLookup[slot];

        if (value == slot_value)
        {
            return;
        }
        if (value == NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
        {
            sNdsRendererHardwareTextureLookup[slot] = (u8)slot_value;
            return;
        }
        slot = (slot + 1u) & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    }
}

static void ndsRendererHardwareTextureLookupRemove(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    u32 slot_value;
    u32 slot;
    u32 probe;

    if ((entry == NULL) || (entry->ready == 0u))
    {
        return;
    }
    slot_value = (u32)(entry - sNdsRendererHardwareTextureCache) + 1u;
    slot = entry->key_hash & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    for (probe = 0u; probe < NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT; probe++)
    {
        u32 value = sNdsRendererHardwareTextureLookup[slot];

        if (value == NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
        {
            return;
        }
        if (value == slot_value)
        {
            /* Repair the remainder of this linear-probe cluster instead of
             * leaving tombstones that animated water keys could accumulate
             * over the complete timed match. The table is twice the cache size,
             * so an empty terminator is guaranteed. */
            sNdsRendererHardwareTextureLookup[slot] =
                NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY;
            slot = (slot + 1u) &
                (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
            while (sNdsRendererHardwareTextureLookup[slot] !=
                   NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
            {
                value = sNdsRendererHardwareTextureLookup[slot];
                sNdsRendererHardwareTextureLookup[slot] =
                    NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY;
                ndsRendererHardwareTextureLookupInsert(
                    &sNdsRendererHardwareTextureCache[value - 1u]);
                slot = (slot + 1u) &
                    (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
            }
            return;
        }
        slot = (slot + 1u) & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    }
}
#endif

static s32 ndsRendererHardwareTexel1RefreshCompatible(
    const NDSRendererHardwareTextureKey *a,
    const NDSRendererHardwareTextureKey *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return FALSE;
    }
    /* The exact Pupupu material animates fraction, image IDs, and tile
     * origins. All of those change converted pixels, but not the resident DS
     * RGBA allocation. Width/height and format distinguish its large and
     * small water surfaces; same-frame reuse is excluded by the caller. */
    return ((a->data_layout == b->data_layout) &&
            (a->format == b->format) &&
            (a->size == b->size) &&
            (a->width == b->width) &&
            (a->height == b->height) &&
            (a->combine_w0 == b->combine_w0) &&
            (a->combine_w1 == b->combine_w1)) ? TRUE : FALSE;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static s32 ndsRendererHardwareTextureKeyWouldLegacyAlias(
    const NDSRendererHardwareTextureKey *a,
    const NDSRendererHardwareTextureKey *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return FALSE;
    }
    return ((a->image == b->image) &&
            (a->image_width == b->image_width) &&
            (a->tlut_image == b->tlut_image) &&
            (a->tlut_count == b->tlut_count) &&
            (a->format == b->format) &&
            (a->size == b->size) &&
            (a->width == b->width) &&
            (a->height == b->height) &&
            (a->render_tile == b->render_tile) &&
            (a->render_tmem == b->render_tmem) &&
            (a->render_palette == b->render_palette) &&
            (a->render_tile_cms == b->render_tile_cms) &&
            (a->render_tile_cmt == b->render_tile_cmt) &&
            (a->render_tile_masks == b->render_tile_masks) &&
            (a->render_tile_maskt == b->render_tile_maskt) &&
            (a->render_tile_shifts == b->render_tile_shifts) &&
            (a->render_tile_shiftt == b->render_tile_shiftt) &&
            (a->load_tile == b->load_tile) &&
            (a->load_uls == b->load_uls) &&
            (a->load_ult == b->load_ult) &&
            (a->load_lrs == b->load_lrs) &&
            (a->load_dxt == b->load_dxt) &&
            (a->load_texels == b->load_texels) &&
            (a->tile_uls == b->tile_uls) &&
            (a->tile_ult == b->tile_ult) &&
            (a->line == b->line) &&
            (a->flags == b->flags) &&
            (a->texel1_image == b->texel1_image) &&
            (a->texel1_image_format == b->texel1_image_format) &&
            (a->texel1_image_size == b->texel1_image_size) &&
            (a->texel1_image_width == b->texel1_image_width) &&
            (a->texel1_load_kind == b->texel1_load_kind) &&
            (a->texel1_render_tmem == b->texel1_render_tmem) &&
            (a->texel1_render_line == b->texel1_render_line) &&
            (a->texel1_render_palette == b->texel1_render_palette) &&
            (a->texel1_render_tile_cms == b->texel1_render_tile_cms) &&
            (a->texel1_render_tile_cmt == b->texel1_render_tile_cmt) &&
            (a->texel1_render_tile_masks == b->texel1_render_tile_masks) &&
            (a->texel1_render_tile_maskt == b->texel1_render_tile_maskt) &&
            (a->texel1_render_tile_shifts == b->texel1_render_tile_shifts) &&
            (a->texel1_render_tile_shiftt == b->texel1_render_tile_shiftt) &&
            (a->texel1_load_tile == b->texel1_load_tile) &&
            (a->texel1_load_uls == b->texel1_load_uls) &&
            (a->texel1_load_ult == b->texel1_load_ult) &&
            (a->texel1_load_lrs == b->texel1_load_lrs) &&
            (a->texel1_load_dxt == b->texel1_load_dxt) &&
            (a->texel1_load_texels == b->texel1_load_texels) &&
            (a->texel1_tile_uls == b->texel1_tile_uls) &&
            (a->texel1_tile_ult == b->texel1_tile_ult) &&
            (a->prim_lod_fraction == b->prim_lod_fraction) &&
            (a->combine_w0 == b->combine_w0) &&
            (a->combine_w1 == b->combine_w1)) ? TRUE : FALSE;
}
#endif

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindTexture(const NDSRendererHardwareTextureKey *key,
                               u32 key_hash)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 slot;
    u32 probe;
#else
    u32 i;
#endif

    if (key == NULL)
    {
        return NULL;
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeFrameSummary.texture_lookup_call_count++;
    entry = (NDSRendererHardwareTextureCacheEntry *)
        sNdsRendererHardwareActiveTextureEntry;
    if ((entry != NULL) && (entry->ready != 0u) &&
        (entry->key_hash == key_hash) &&
        (ndsRendererHardwareEntryKeyEqual(entry, key) != FALSE))
    {
        sNdsRendererRuntimeFrameSummary.texture_lookup_active_hit_count++;
        return entry;
    }

    slot = key_hash & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    for (probe = 0u; probe < NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT; probe++)
    {
        u32 value = sNdsRendererHardwareTextureLookup[slot];

        sNdsRendererRuntimeFrameSummary.texture_lookup_probe_count++;
        if (value == NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
        {
            break;
        }
        entry = &sNdsRendererHardwareTextureCache[value - 1u];
        if ((entry->ready != 0u) && (entry->key_hash == key_hash) &&
            (ndsRendererHardwareEntryKeyEqual(entry, key) != FALSE))
        {
            sNdsRendererRuntimeFrameSummary
                .texture_lookup_table_hit_count++;
            return entry;
        }
        slot = (slot + 1u) & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    }
#else
    (void)key_hash;
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureKey resident;

        if (sNdsRendererHardwareTextureCache[i].ready == 0u)
        {
            continue;
        }
        if (ndsRendererHardwareEntryKeyEqual(
                &sNdsRendererHardwareTextureCache[i], key) != FALSE)
        {
            return &sNdsRendererHardwareTextureCache[i];
        }
        ndsRendererHardwareEntryCopyKey(
            &sNdsRendererHardwareTextureCache[i], &resident);
        if (ndsRendererHardwareTextureKeyWouldLegacyAlias(
                &resident, key) != FALSE)
        {
            ndsRendererProfileRecordTextureAliasAvoid();
        }
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeFrameSummary.texture_lookup_miss_count++;
#endif
    return NULL;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindStageSourceFrameTexture(
    const NDSRendererHardwareTextureKey *key)
{
    u32 i;

    if (key == NULL)
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];
        NDSRendererHardwareTextureKey source_frame;

        if ((entry->ready == 0u) || (entry->pinned != 0u))
        {
            continue;
        }
        ndsRendererHardwareEntryCopyKey(entry, &source_frame);
        source_frame.image = key->image;
        if (ndsRendererHardwareTextureKeyEqual(&source_frame, key) != FALSE)
        {
            return entry;
        }
    }
    return NULL;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindTexel1RefreshTexture(
    const NDSRendererHardwareTextureKey *key)
{
    u32 i;

    if ((key == NULL) || (key->texel1_image == 0u))
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];
        /* Only ever a dynamic slot: this refuses pinned entries, and the static
         * partition holds nothing else. */
        const NDSRendererHardwareTextureKey *resident =
            ndsRendererHardwareEntryDynamicKey(entry);

        if ((resident != NULL) && (entry->ready != 0u) &&
            (entry->pinned == 0u) &&
            (resident->texel1_image != 0u) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)) &&
            (ndsRendererHardwareTexel1RefreshCompatible(
                 resident, key) != FALSE))
        {
            return entry;
        }
    }
    return NULL;
}

static s32 ndsRendererHardwareReplaceTextureData(
    NDSRendererHardwareTextureCacheEntry *entry,
    const void *texture,
    u32 staged_bytes,
    u32 texture_bytes,
    const u8 *row_map,
    u32 row_bytes,
    u32 row_count)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_REPLACE_REFRESH);
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    if ((entry == NULL) || (entry->name == 0) ||
        (texture == NULL) || (staged_bytes == 0u) ||
        (texture_bytes == 0u) ||
        ((row_map == NULL) && ((texture_bytes % staged_bytes) != 0u)) ||
        ((row_map != NULL) &&
         ((row_bytes == 0u) || (row_count == 0u) ||
          (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
          ((staged_bytes % row_bytes) != 0u) ||
          (texture_bytes != (row_bytes * row_count)))))
    {
        return FALSE;
    }
    ndsRendererBenchmarkSinkWord((u32)entry->name);
    ndsRendererBenchmarkSinkWord(staged_bytes);
    ndsRendererBenchmarkSinkWord(texture_bytes);
    return TRUE;
#else
    void *vram_address;
    uintptr_t vram_first;
    uintptr_t vram_last;
    u32 vram_state;
    u32 offset;
    u32 i;

    if ((entry == NULL) || (entry->name == 0) ||
        (texture == NULL) || (staged_bytes == 0u) ||
        (texture_bytes == 0u) ||
        ((row_map == NULL) && ((texture_bytes % staged_bytes) != 0u)) ||
        ((row_map != NULL) &&
         ((row_bytes == 0u) || (row_count == 0u) ||
          (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
          ((staged_bytes % row_bytes) != 0u) ||
          (texture_bytes != (row_bytes * row_count)))))
    {
        return FALSE;
    }
    if (row_map != NULL)
    {
        u32 staged_rows = staged_bytes / row_bytes;

        for (i = 0u; i < row_count; i++)
        {
            if (row_map[i] >= staged_rows)
            {
                return FALSE;
            }
        }
    }
    vram_address = glGetTexturePointer(entry->name);
    if (vram_address == NULL)
    {
        return FALSE;
    }

    /* sm64-nds updates allocated textures by temporarily exposing every
     * owning texture bank to the CPU, then restoring the primary mapping. */
    ndsRendererHardwareEndBatch();
    vram_state = VRAM_CR;
    vram_first = (uintptr_t)vram_address;
    vram_last = vram_first + texture_bytes - 1u;
    if ((vram_first < (uintptr_t)VRAM_B) &&
        (vram_last >= (uintptr_t)VRAM_A))
    {
        vramSetBankA(VRAM_A_LCD);
    }
    if ((vram_first < (uintptr_t)VRAM_C) &&
        (vram_last >= (uintptr_t)VRAM_B))
    {
        vramSetBankB(VRAM_B_LCD);
    }
    if ((vram_first < (uintptr_t)VRAM_D) &&
        (vram_last >= (uintptr_t)VRAM_C))
    {
        vramSetBankC(VRAM_C_LCD);
    }
    if ((vram_first < (uintptr_t)VRAM_E) &&
        (vram_last >= (uintptr_t)VRAM_D))
    {
        vramSetBankD(VRAM_D_LCD);
    }

    DC_FlushRange(texture, staged_bytes);
    if (row_map == NULL)
    {
        for (offset = 0u; offset < texture_bytes; offset += staged_bytes)
        {
            dmaCopyWords(0, texture, (u8 *)vram_address + offset,
                         staged_bytes);
        }
    }
    else
    {
        /* The first occurrence of every distinct rendered row is stored once.
         * Adjacent first occurrences remain adjacent in the staging buffer, so
         * coalesce those runs and issue individual copies only for repeats. */
        for (i = 0u; i < row_count; )
        {
            u32 run = 1u;
            u32 source_row = row_map[i];

            while (((i + run) < row_count) &&
                   (row_map[i + run] == (source_row + run)))
            {
                run++;
            }
            dmaCopyWords(0, (const u8 *)texture + (source_row * row_bytes),
                         (u8 *)vram_address + (i * row_bytes),
                         run * row_bytes);
            i += run;
        }
    }
    vramRestorePrimaryBanks(vram_state);
    return TRUE;
#endif
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static s32 ndsRendererHardwareTextureRefreshUses(
    const u16 *pixels)
{
    u32 i;

    for (i = 0u; i < sNdsRendererHardwareTextureRefreshCount; i++)
    {
        if (sNdsRendererHardwareTextureRefreshQueue[i].pixels == pixels)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static s32 ndsRendererHardwareQueueTextureRefresh(
    NDSRendererHardwareTextureCacheEntry *entry,
    const u16 *pixels,
    u32 staged_bytes,
    u32 texture_bytes,
    const u8 *row_map,
    u32 row_bytes,
    u32 row_count)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_REPLACE_REFRESH);
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    (void)entry;
    (void)pixels;
    (void)staged_bytes;
    (void)texture_bytes;
    (void)row_map;
    (void)row_bytes;
    (void)row_count;
    return FALSE;
#else
    NDSRendererHardwareTextureRefresh *refresh;

    if ((entry == NULL) || (entry->name == 0) || (pixels == NULL) ||
        (staged_bytes == 0u) || (texture_bytes == 0u) ||
        ((row_map == NULL) && ((texture_bytes % staged_bytes) != 0u)) ||
        ((row_map != NULL) &&
         ((row_bytes == 0u) || (row_count == 0u) ||
          (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
          ((staged_bytes % row_bytes) != 0u) ||
          (texture_bytes != (row_bytes * row_count)))) ||
        (sNdsRendererHardwareTextureRefreshCount >=
         NDS_RENDERER_HW_TEXTURE_REFRESH_QUEUE_COUNT) ||
        (glGetTexturePointer(entry->name) == NULL))
    {
        return FALSE;
    }
    refresh = &sNdsRendererHardwareTextureRefreshQueue[
        sNdsRendererHardwareTextureRefreshCount++];
    refresh->entry = entry;
    refresh->pixels = pixels;
    refresh->staged_bytes = staged_bytes;
    refresh->texture_bytes = texture_bytes;
    refresh->row_bytes = row_bytes;
    refresh->row_count = row_count;
    if (row_map != NULL)
    {
        memcpy(refresh->row_map, row_map, row_count);
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureVBlankQueuedUploads++;
    gNdsRendererProfileTextureVBlankQueuedBytes += texture_bytes;
#endif
    return TRUE;
#endif
}
#endif

u32 ndsRendererHardwareCommitPendingTextureRefreshes(void)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 committed = 0u;
    u32 i;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 start;
    u32 start_line;

    start_line = REG_VCOUNT;
    start = cpuGetTiming();
    gNdsRendererProfileTextureVBlankStartLine = start_line;
#endif

    for (i = 0u; i < sNdsRendererHardwareTextureRefreshCount; i++)
    {
        NDSRendererHardwareTextureRefresh *refresh =
            &sNdsRendererHardwareTextureRefreshQueue[i];

        if (ndsRendererHardwareReplaceTextureData(
                refresh->entry, refresh->pixels, refresh->staged_bytes,
                refresh->texture_bytes,
                (refresh->row_count != 0u) ? refresh->row_map : NULL,
                refresh->row_bytes, refresh->row_count) != FALSE)
        {
            committed++;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        else
        {
            gNdsRendererProfileTextureVBlankFallbackCount++;
        }
#endif
        refresh->entry = NULL;
        refresh->pixels = NULL;
        refresh->staged_bytes = 0u;
        refresh->texture_bytes = 0u;
        refresh->row_bytes = 0u;
        refresh->row_count = 0u;
    }
    sNdsRendererHardwareTextureRefreshCount = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureVBlankCommittedUploads += committed;
    gNdsRendererProfileTextureVBlankCommitTicks += cpuGetTiming() - start;
    gNdsRendererProfileTextureVBlankEndLine = REG_VCOUNT;
    if ((committed != 0u) &&
        ((start_line < 192u) ||
         (gNdsRendererProfileTextureVBlankEndLine < 192u)))
    {
        gNdsRendererProfileTextureVBlankOutsideCount++;
    }
#endif
    return committed;
#else
    /* Profile 2 keeps its large semantic/oracle ledger resident. Shipping and
     * coarse builds own the compact VBlank staging buffers; the forensic build
     * retains the exact synchronous upload as its independent oracle route. */
    return 0u;
#endif
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareReleaseTexture(
    NDSRendererHardwareTextureCacheEntry *entry)
{
    if (entry == NULL)
    {
        return NULL;
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareTextureLookupRemove(entry);
#endif
    ndsRendererHardwareEndBatch();
    /* Unconditional. This is hygiene, NOT the second-entry texture failure --
     * making it unconditional was tried against that failure and changed
     * nothing, so do not cite it as the cause.
     *
     * It is still correct on its own terms: the cache holds a NAME, libnds
     * recycles names through its dealloc pool once deleted (measured
     * deallocTexSize 0 on a first entry, 19 on a second), and any delete can
     * move libnds's own active texture. Clearing only on a match can therefore
     * leave the skip armed for a number that now belongs to a different
     * texture -- the same shape as the prepared-run cache and the
     * sNdsIFCommonPreparedFile latch. Cost is one redundant bind per release. */
    sNdsRendererHardwareBoundTextureName = 0u;
    if (sNdsRendererHardwareActiveTextureEntry == entry)
    {
        sNdsRendererHardwareActiveTextureEntry = NULL;
    }
    if (entry->name != 0)
    {
        ndsRendererHardwareFencedGlDeleteTextures(1, &entry->name);
    }
    /* The key is no longer inside the entry, so zeroing the entry no longer
     * zeroes it. Every reader gates on `ready`, but the particle atlas keeps a
     * slot ready with a deliberately blank key, so a released slot must really
     * be blank rather than merely unreachable. */
    ndsRendererHardwareEntryClearKey(entry);
    /* R2-07 leg A. This is the moment an entry stops being what a prepared run
     * recorded, so it is where the invalidation is published -- see the note on
     * ndsRendererNativeStagePreparedTexturesProven. Stamping the same counter a
     * re-key stamps makes it a complete epoch rather than a partial one; every
     * existing `entry->key_generation == recorded` compare keeps its meaning
     * because entries carry their own stamped value and the counter is only
     * ever advanced. */
    sNdsRendererHardwareTextureKeyGeneration++;
    if (sNdsRendererHardwareTextureKeyGeneration == 0u)
    {
        sNdsRendererHardwareTextureKeyGeneration++;
    }
    gNdsR2TextureEpochBumpCount++;
    memset(entry, 0, sizeof(*entry));
    return entry;
}

static s32 ndsRendererHardwareEvictTexture(
    const NDSRendererHardwareTextureCacheEntry *exclude)
{
    u32 i;

    /* The static partition is pinned by construction, so an eviction sweep that
     * walked it would only ever be counting slots it may not take. */
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT; i++)
    {
        u32 index = NDS_RENDERER_HW_TEXTURE_STATIC_COUNT +
            (sNdsRendererHardwareTextureCacheNext %
             NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT);
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[index];

        sNdsRendererHardwareTextureCacheNext++;
        if ((entry != exclude) && (entry->name != 0) &&
            (entry->pinned == 0u) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)))
        {
            ndsRendererProfileRecordTextureEvict();
            (void)ndsRendererHardwareReleaseTexture(entry);
            return TRUE;
        }
    }
    return FALSE;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareAllocTexture(void)
{
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 i;
    u32 index;

    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_ALLOC);

    /* Dynamic slots only. Slots below STATIC_COUNT belong to the generated
     * corpus one-for-one with its record indices and have no pool key to write,
     * so handing one to a runtime texture would leave it keyless. The static
     * prepare binds its own slots directly. */
    for (i = NDS_RENDERER_HW_TEXTURE_STATIC_COUNT;
         i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        if (sNdsRendererHardwareTextureCache[i].ready == 0u)
        {
            entry = &sNdsRendererHardwareTextureCache[i];
            return (entry->name != 0) ?
                ndsRendererHardwareReleaseTexture(entry) : entry;
        }
    }
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT; i++)
    {
        index = NDS_RENDERER_HW_TEXTURE_STATIC_COUNT +
            (sNdsRendererHardwareTextureCacheNext %
             NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT);
        sNdsRendererHardwareTextureCacheNext++;
        entry = &sNdsRendererHardwareTextureCache[index];
        if ((entry->pinned == 0u) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)))
        {
            /* libnds allocates texture VRAM in glTexImage2D. Delete the old
             * allocation, but never recycle a texture referenced this frame. */
            ndsRendererProfileRecordTextureEvict();
            return ndsRendererHardwareReleaseTexture(entry);
        }
    }
    return NULL;
}

#if NDS_R2_FOX_GUN_OVERLAY
static void ndsRendererHardwareResetFoxGunTextureState(void);
static void ndsRendererHardwareReleaseFoxGunTexture(void);
#endif

void ndsRendererHardwareDiscardTextureCache(void)
{
    if ((sNdsRendererBattleStaticTexturePrepared != 0u) ||
        (sNdsRendererBattleStaticTextureArmed != 0u))
    {
        gNdsRendererBattleStaticTextureViolationCount++;
    }
    sNdsRendererBattleStaticTexturePrepared = FALSE;
#if NDS_RENDERER_HW_TRIANGLES
    u32 i;

    ndsRendererHardwareEndBatch();
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        if ((sNdsRendererHardwareTextureCache[i].name != 0) ||
            (sNdsRendererHardwareTextureCache[i].ready != 0u))
        {
            (void)ndsRendererHardwareReleaseTexture(
                &sNdsRendererHardwareTextureCache[i]);
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    memset(sNdsRendererHardwareTextureLookup, 0,
           sizeof(sNdsRendererHardwareTextureLookup));
    memset(sNdsRendererHardwareTextureRefreshQueue, 0,
           sizeof(sNdsRendererHardwareTextureRefreshQueue));
    sNdsRendererHardwareTextureRefreshCount = 0u;
#endif
    if (sNdsRendererHardwareNoTextureName != 0)
    {
        ndsRendererHardwareFencedGlDeleteTextures(
            1, &sNdsRendererHardwareNoTextureName);
        sNdsRendererHardwareNoTextureName = 0;
    }
    /* The beam's dedicated name is not a cache entry, so the loop above does
     * not reach it -- and glResetTextures runs immediately after this in
     * ndsRendererHardwareResetSceneTextureVram, which would leave the identity
     * below claiming residency for a name that no longer exists. */
    ndsRendererHardwareReleaseIFCommonCloudAtlas(
        &sNdsRendererHardwarePrimRgbTexel0AlphaName);
    sNdsRendererHardwarePrimRgbTexel0AlphaImage = 0u;
    sNdsRendererHardwarePrimRgbTexel0AlphaExtent = 0u;
    sNdsRendererHardwarePrimRgbTexel0AlphaPrim = 0u;
#if NDS_R2_IMPACT_WAVE_NATIVE
    for (i = 0u; i < NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT; i++)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(
            &sNdsRendererImpactWaveTextureName[i]);
        sNdsRendererImpactWaveTextureName[i] = 0u;
    }
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    for (i = 0u; i < NDS_REBIRTH_HALO_TEXTURE_COUNT; i++)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(
            &sNdsRendererRebirthHaloTextureName[i]);
        sNdsRendererRebirthHaloTextureName[i] = 0u;
    }
#endif
    for (i = 0u; i < NDS_ENTRY_EFFECT_TEXTURE_COUNT; i++)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(
            &sNdsRendererEntryEffectTextureName[i]);
        sNdsRendererEntryEffectTextureName[i] = 0u;
    }
    sNdsRendererHardwareTextureCacheNext = 0u;
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
#if NDS_R2_FOX_GUN_OVERLAY
    /* Fox's gun is a direct scene-owned GL name, not a material-cache entry.
     * Delete it before glResetTextures and clear the fast-path identity so a
     * rematch cannot accept a stale/recycled name. */
    ndsRendererHardwareReleaseFoxGunTexture();
#endif
    ndsRendererHardwareResetSourceCaches();
#endif
    sNdsRendererBattleStaticTextureArmed = FALSE;
}

/* Texture VRAM is a SCENE-owned resource, and this is the one call that says so.
 *
 * R2-07 E3/E4 root-caused the second-entry stage corruption here: libnds
 * allocates texture VRAM inside `glTexImage2D`, nothing in the port owned the
 * lifetime of that allocator across a scene boundary, and a second entry into
 * `nSCKindVSBattle` (Sudden Death, or a START rematch) therefore rebuilt its 24
 * pinned statics into a pool whose occupancy and free-list shape were inherited
 * from the match that had just ended. Measured consequence: `glTexImage2D`
 * refused a 4,096-byte upload with 268,800 contiguous free bytes reported, which
 * failed `PrepareRun` for run 42, rejected the whole native stage owner, and
 * dropped the stage onto the generic renderer every frame -- correct geometry,
 * untextured pond, `STG` 2.76M, 4.2 FPS.
 *
 * Two lesser owners had the same shape and are also settled by resetting rather
 * than by hand: `ndsIFCommonNativeOamDiscardTextures` zeroes the four atlas
 * NAMES without deleting them (so their blocks would leak one entry at a time),
 * and the earlier fix that released the atlases to restore entry one's
 * allocation ORDER only reproduced a layout instead of guaranteeing one.
 *
 * So every entry into the battle scene now starts from an empty allocator, which
 * makes entry N byte-for-byte allocation-equivalent to entry 1 by construction
 * instead of by argument. The caller must release its own software owners first
 * (`ndsIFCommonNativeOamReleaseCloudTextures`) -- `glResetTextures` invalidates
 * every name, so anything still holding one has to have let go.
 *
 * Cost is scene-entry only: one cache teardown plus the re-upload the static and
 * atlas prepares were already doing. It buys the standing law this campaign paid
 * for twice: a resource that survives a scene boundary must be re-derived from
 * something the boundary actually moves, never from pointers, names, sizes, or
 * an allocation order a rewound allocator happens to reproduce. */
volatile u32 gNdsRendererSceneTextureVramResetEnable = 1u;
volatile u32 gNdsRendererSceneTextureVramResetCount;

void ndsRendererHardwareResetSceneTextureVram(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    /* Runtime, not a build flag: the control arm has to be the same binary.
     * This ROM's pacing is cache-placement sensitive and separately linked arms
     * have already confused two comparisons on this row. */
    if (gNdsRendererSceneTextureVramResetEnable == 0u)
    {
        return;
    }
    /* DiscardTextureCache ends the batch, deletes every cache name and the
     * no-texture name, and clears the lookup, refresh queue and active
     * binding. glResetTextures then drops libnds's own texture and palette
     * metadata and rebuilds both block allocators. */
    ndsRendererHardwareDiscardTextureCache();
    glResetTextures();
    /* Software state that would otherwise reference names glResetTextures has
     * just invalidated. The prepared-run cache only exists at
     * NDS_R2_STAGE_DIRECT; this function is scoped to NDS_RENDERER_HW_TRIANGLES,
     * which is the weaker condition, and the audio-FGM harness is a config that
     * has the second without the first. */
#if NDS_R2_STAGE_DIRECT
    sNdsNativeStageOwnerExecution.r2_prepared_valid = 0u;
#endif
#if NDS_TASK36_HW_COMPOSE == 2
    ndsRendererTask36ReplayReset();
#endif
    gNdsRendererSceneTextureVramResetCount++;
#endif
}

s32 ndsRendererHardwareUploadSceneMipCache(const u16 *mip0,
                                            const u16 *mip1,
                                            const u16 *mip2)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
    const u16 *pixels[NDS_RENDERER_SCENE_MIP_COUNT] = {
        mip0, mip1, mip2
    };
    u32 i;

    if ((mip0 == NULL) || (mip1 == NULL) || (mip2 == NULL))
    {
        return FALSE;
    }
    ndsRendererHardwareDiscardTextureCache();
    for (i = 0u; i < NDS_RENDERER_SCENE_MIP_COUNT; i++)
    {
        if (sNdsRendererSceneMipTextureNames[i] != 0)
        {
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsRendererSceneMipTextureNames[i]);
            sNdsRendererSceneMipTextureNames[i] = 0;
        }
        if (ndsRendererHardwareFencedGlGenTextures(
                1, &sNdsRendererSceneMipTextureNames[i]) == 0)
        {
            break;
        }
        ndsRendererHardwareBindTextureState(
            sNdsRendererSceneMipTextureNames[i]);
        if (ndsRendererHardwareFencedGlTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA,
                TEXTURE_SIZE_128, TEXTURE_SIZE_128, 0,
                TEXGEN_TEXCOORD, pixels[i]) == 0)
        {
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsRendererSceneMipTextureNames[i]);
            sNdsRendererSceneMipTextureNames[i] = 0;
            break;
        }
    }
    if (i != NDS_RENDERER_SCENE_MIP_COUNT)
    {
        u32 j;

        for (j = 0u; j < NDS_RENDERER_SCENE_MIP_COUNT; j++)
        {
            if (sNdsRendererSceneMipTextureNames[j] != 0)
            {
                ndsRendererHardwareFencedGlDeleteTextures(
                    1, &sNdsRendererSceneMipTextureNames[j]);
                sNdsRendererSceneMipTextureNames[j] = 0;
            }
        }
        sNdsRendererHardwareBoundTextureName = 0u;
        return FALSE;
    }
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;
#else
    (void)mip0;
    (void)mip1;
    (void)mip2;
    return FALSE;
#endif
}

s32 ndsRendererHardwareDrawSceneMipCache(u32 mip_index,
                                          const s32 *tex_s_q4,
                                          const s32 *tex_t_q4,
                                          u32 columns,
                                          u32 rows)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
    v16 vertex_x[64];
    v16 vertex_y[64];
    u32 vertex_count;
    u32 row;
    u32 column;
    u32 triangle_count;

    if ((mip_index >= NDS_RENDERER_SCENE_MIP_COUNT) ||
        (sNdsRendererSceneMipTextureNames[mip_index] == 0) ||
        (tex_s_q4 == NULL) || (tex_t_q4 == NULL) ||
        (columns < 2u) || (rows < 2u))
    {
        return FALSE;
    }
    vertex_count = columns * rows;
    if (vertex_count > (sizeof(vertex_x) / sizeof(vertex_x[0])))
    {
        return FALSE;
    }
    for (row = 0u; row < rows; row++)
    {
        for (column = 0u; column < columns; column++)
        {
            u32 index = (row * columns) + column;

            vertex_x[index] = (v16)(-4096 +
                (s32)((8192u * column) / (columns - 1u)));
            vertex_y[index] = (v16)(4096 -
                (s32)((8192u * row) / (rows - 1u)));
        }
    }

    ndsRendererHardwareEndBatch();
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    ndsRendererHardwareBindTextureState(
        sNdsRendererSceneMipTextureNames[mip_index]);
    ndsRendererHardwareSetPolyFmt(
        POLY_CULL_NONE | POLY_ALPHA(31) | POLY_ID(63));
    glColor(RGB15(31, 31, 31));
    glBegin(GL_TRIANGLE);
    for (row = 0u; row + 1u < rows; row++)
    {
        for (column = 0u; column + 1u < columns; column++)
        {
            u32 i00 = (row * columns) + column;
            u32 i10 = i00 + 1u;
            u32 i01 = i00 + columns;
            u32 i11 = i01 + 1u;

#define NDS_SCENE_MIP_EMIT(index) do { \
    glTexCoord2t16((t16)tex_s_q4[(index)], \
                   (t16)tex_t_q4[(index)]); \
    glVertex3v16(vertex_x[(index)], vertex_y[(index)], (v16)4090); \
} while (0)
            NDS_SCENE_MIP_EMIT(i00);
            NDS_SCENE_MIP_EMIT(i01);
            NDS_SCENE_MIP_EMIT(i10);
            NDS_SCENE_MIP_EMIT(i10);
            NDS_SCENE_MIP_EMIT(i01);
            NDS_SCENE_MIP_EMIT(i11);
#undef NDS_SCENE_MIP_EMIT
        }
    }
    triangle_count = (columns - 1u) * (rows - 1u) * 2u;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices +=
        triangle_count * 3u;
    sNdsRendererHardwareSubmitted = TRUE;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    sNdsRendererHardwareMatrixGeneration = 0u;
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;
#else
    (void)mip_index;
    (void)tex_s_q4;
    (void)tex_t_q4;
    (void)columns;
    (void)rows;
    return FALSE;
#endif
}

static s32 ndsRendererHardwareTextureSizeEnum(u32 size, int *out)
{
    int value;

    if (out == NULL)
    {
        return FALSE;
    }
    switch (size)
    {
    case 8u: value = TEXTURE_SIZE_8; break;
    case 16u: value = TEXTURE_SIZE_16; break;
    case 32u: value = TEXTURE_SIZE_32; break;
    case 64u: value = TEXTURE_SIZE_64; break;
    case 128u: value = TEXTURE_SIZE_128; break;
    case 256u: value = TEXTURE_SIZE_256; break;
    default:
        return FALSE;
    }
    *out = value;
    return TRUE;
}

static s32 ndsRendererHardwarePrepareIFCommonAtlas(
    u32 width, u32 height, u32 texture_format,
    const u16 *palette, u32 palette_entries,
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name,
    u32 color0_transparent)
{
#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    int size_x;
    int size_y;
    int name = 0;
    u32 bytes;
    u32 param;
    u32 upload_attempts = 0u;
    u8 *pixels = (u8 *)sNdsRendererHardwareTextureScratch;

    if ((fill == NULL) || (texture_name == NULL) ||
        (((texture_format == GL_RGB8_A5) &&
          ((palette == NULL) || (palette_entries == 0u) || (palette_entries > 8u))) ||
         ((texture_format == GL_RGB32_A3) && (palette_entries > 32u)) ||
         ((texture_format == GL_RGB16) && (palette_entries > 16u)) ||
         (((texture_format == GL_RGB32_A3) || (texture_format == GL_RGB16)) &&
          ((palette == NULL) || (palette_entries == 0u))) ||
         ((texture_format == GL_RGBA) &&
          ((palette != NULL) || (palette_entries != 0u))) ||
         ((texture_format != GL_RGB8_A5) &&
          (texture_format != GL_RGB32_A3) &&
          (texture_format != GL_RGB16) &&
          (texture_format != GL_RGBA))) ||
        (height == 0u) || (width > (UINT32_MAX / height)) ||
        (ndsRendererHardwareTextureSizeEnum(width, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(height, &size_y) == FALSE))
    {
        return FALSE;
    }
    /* GL_RGB16 is the only 4bpp format here -- two texels to the byte -- and it
     * carries no alpha in the texel, so index 0 reads as opaque black unless
     * COLOR0_TRANSPARENT is set. The other two are 8bpp with the alpha bits in
     * the texel itself and must NOT get the flag, or their index 0 would stop
     * being a usable colour. */
    if (texture_format == GL_RGB16)
    {
        bytes = (width * height) / 2u;
        param = (u32)TEXGEN_TEXCOORD;
        if (color0_transparent != FALSE)
        {
            param |= (u32)GL_TEXTURE_COLOR0_TRANSPARENT;
        }
    }
    else if (texture_format == GL_RGBA)
    {
        bytes = width * height * 2u;
        param = (u32)TEXGEN_TEXCOORD;
    }
    else
    {
        bytes = width * height;
        param = (u32)TEXGEN_TEXCOORD;
    }
    if ((bytes > sizeof(sNdsRendererHardwareTextureScratch)) ||
        (fill(pixels, bytes, user_data) == FALSE))
    {
        return FALSE;
    }
    if (*texture_name != 0u)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(texture_name);
    }
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
    {
        return FALSE;
    }
    ndsRendererHardwareBindTextureState(name);
    while (ndsRendererHardwareFencedGlTexImage2D(
               GL_TEXTURE_2D, 0, (int)texture_format, size_x, size_y, 0,
               (int)param, pixels) == 0)
    {
        ndsRendererHardwareFencedGlDeleteTextures(1, &name);
        name = 0;
        upload_attempts++;
        if ((upload_attempts >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT) ||
            (ndsRendererHardwareEvictTexture(NULL) == FALSE) ||
            (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0))
        {
            return FALSE;
        }
        ndsRendererHardwareBindTextureState(name);
    }
    if (texture_format != GL_RGBA)
    {
        glColorTableEXT(GL_TEXTURE_2D, 0, (int)palette_entries, 0, 0, palette);
    }
    *texture_name = (u32)name;
    sNdsRendererHardwareBoundTextureName = (u32)name;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;
#else
    (void)width;
    (void)height;
    (void)texture_format;
    (void)palette;
    (void)palette_entries;
    (void)fill;
    (void)user_data;
    (void)texture_name;
    return FALSE;
#endif
}

s32 ndsRendererHardwarePrepareIFCommonCloudAtlas(
    u32 width, u32 height, const u16 palette[8],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name)
{
    return ndsRendererHardwarePrepareIFCommonAtlas(
        width, height, GL_RGB8_A5, palette, 8u,
        fill, user_data, texture_name, FALSE);
}

s32 ndsRendererHardwarePrepareIFCommonA3I5Atlas(
    u32 width, u32 height, const u16 palette[32],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name)
{
    return ndsRendererHardwarePrepareIFCommonAtlas(
        width, height, GL_RGB32_A3, palette, 32u,
        fill, user_data, texture_name, FALSE);
}

/* Source CI4 with an RGBA5551 TLUT whose entry 0 has alpha 0 -- Mario's
 * fireball is exactly this -- maps onto GL_RGB16 with NO loss: 16 palette
 * entries, 4bpp, and index 0 clear. The two formats above both had to trade
 * index bits against alpha bits; this one does not, so it is the format to
 * reach for whenever the source asset is CI4 and its palette carries the
 * transparency. `fill` writes PACKED nibbles here, two texels per byte, low
 * nibble first -- not one byte per texel as the A5I3/A3I5 fills do. */
s32 ndsRendererHardwarePrepareIFCommonPal16Atlas(
    u32 width, u32 height, const u16 palette[16],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name)
{
    return ndsRendererHardwarePrepareIFCommonAtlas(
        width, height, GL_RGB16, palette, 16u,
        fill, user_data, texture_name, TRUE);
}

#if NDS_R2_IMPACT_WAVE_NATIVE
static s32 ndsRendererImpactWaveTexelFill(u8 *pixels, u32 bytes,
                                          void *user_data)
{
    (void)user_data;
    if ((pixels == NULL) || (bytes < NDS_RENDERER_IMPACT_WAVE_TEX_BYTES))
    {
        return FALSE;
    }
    memcpy(pixels, sNdsRendererImpactWaveTexels,
           NDS_RENDERER_IMPACT_WAVE_TEX_BYTES);
    return TRUE;
}
#endif

#if NDS_R2_REBIRTH_HALO_NATIVE
typedef struct NDSRebirthHaloTextureFill
{
    const u8 *pixels;
    u32 bytes;
} NDSRebirthHaloTextureFill;

static s32 ndsRendererRebirthHaloTextureFill(u8 *pixels, u32 bytes,
                                              void *user_data)
{
    const NDSRebirthHaloTextureFill *fill =
        (const NDSRebirthHaloTextureFill *)user_data;

    if ((pixels == NULL) || (fill == NULL) || (fill->pixels == NULL) ||
        (bytes < fill->bytes))
    {
        return FALSE;
    }
    memset(pixels, 0, bytes);
    memcpy(pixels, fill->pixels, fill->bytes);
    return TRUE;
}
#endif

static s32 ndsRendererEntryEffectTextureFill(u8 *pixels, u32 bytes,
                                              void *user_data)
{
    const NDSEntryEffectTexture *texture =
        (const NDSEntryEffectTexture *)user_data;
    u32 decoded_bytes;

    if ((pixels == NULL) || (texture == NULL) ||
        (texture->texels == NULL))
    {
        return FALSE;
    }
    if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_PAL16)
    {
        decoded_bytes = ((u32)texture->width * texture->height) >> 1;
    }
    else if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_A5I3)
    {
        decoded_bytes = (u32)texture->width * texture->height;
    }
    else if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_RGBA)
    {
        decoded_bytes = (u32)texture->width * texture->height * 2u;
    }
    else
    {
        return FALSE;
    }
    if (bytes < decoded_bytes)
    {
        return FALSE;
    }
    memset(pixels, 0, bytes);
    if (texture->compression == NDS_ENTRY_EFFECT_COMPRESSION_RAW)
    {
        if (texture->texel_bytes != decoded_bytes)
        {
            return FALSE;
        }
        memcpy(pixels, texture->texels, decoded_bytes);
    }
    else if (texture->compression == NDS_ENTRY_EFFECT_COMPRESSION_LZ10)
    {
        /* The generated stream carries the exact DS BIOS LZ10 header.  This
         * fill buffer already exists as the temporary upload destination, so
         * decoding here costs no persistent RAM and no gameplay-frame work. */
        if ((texture->texel_bytes < 4u) ||
            (texture->texels[0] != 0x10u) ||
            ((((u32)texture->texels[1]) |
              ((u32)texture->texels[2] << 8) |
              ((u32)texture->texels[3] << 16)) != decoded_bytes))
        {
            return FALSE;
        }
        swiDecompressLZSSWram(texture->texels, pixels);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

s32 ndsRendererHardwarePrepareImpactWaveTextures(void)
{
#if NDS_R2_IMPACT_WAVE_NATIVE && NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    u32 variant;

    for (variant = 0u; variant < NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT;
         variant++)
    {
        if (sNdsRendererImpactWaveTextureName[variant] != 0u)
        {
            continue;
        }
        if (ndsRendererHardwarePrepareIFCommonPal16Atlas(
                NDS_RENDERER_IMPACT_WAVE_TEX_WIDTH,
                NDS_RENDERER_IMPACT_WAVE_TEX_HEIGHT,
                sNdsRendererImpactWavePalettes[variant],
                ndsRendererImpactWaveTexelFill, NULL,
                &sNdsRendererImpactWaveTextureName[variant]) == FALSE)
        {
            return FALSE;
        }
        gNdsImpactWaveNativeTexturePrepareCount++;
    }
    return TRUE;
#else
    return FALSE;
#endif
}

s32 ndsRendererHardwarePrepareRebirthHaloTextures(void)
{
#if NDS_R2_REBIRTH_HALO_NATIVE && NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    static const struct
    {
        const u8 *pixels;
        u32 bytes;
        const u16 *palette;
        u32 width;
        u32 height;
        u32 a5i3;
    } textures[NDS_REBIRTH_HALO_TEXTURE_COUNT] = {
        { sNdsRebirthHaloTexelsA40, sizeof(sNdsRebirthHaloTexelsA40),
          sNdsRebirthHaloPaletteA40,
          NDS_REBIRTH_HALO_A40_WIDTH, NDS_REBIRTH_HALO_A40_HEIGHT, FALSE },
        { sNdsRebirthHaloTexelsBC8, sizeof(sNdsRebirthHaloTexelsBC8),
          sNdsRebirthHaloPaletteBC8,
          NDS_REBIRTH_HALO_BC8_WIDTH, NDS_REBIRTH_HALO_BC8_HEIGHT, FALSE },
        { sNdsRebirthHaloTexelsDD0, sizeof(sNdsRebirthHaloTexelsDD0),
          sNdsRebirthHaloPaletteDD0,
          NDS_REBIRTH_HALO_DD0_WIDTH, NDS_REBIRTH_HALO_DD0_HEIGHT, FALSE },
        { sNdsRebirthHaloTexelsE58, sizeof(sNdsRebirthHaloTexelsE58),
          sNdsRebirthHaloPaletteE58,
          NDS_REBIRTH_HALO_E58_WIDTH, NDS_REBIRTH_HALO_E58_HEIGHT, FALSE },
        { sNdsRebirthHaloTexelsBeam, sizeof(sNdsRebirthHaloTexelsBeam),
          sNdsRebirthHaloPaletteBeam,
          NDS_REBIRTH_HALO_BEAM_WIDTH, NDS_REBIRTH_HALO_BEAM_HEIGHT, TRUE },
    };
    u32 i;

    for (i = 0u; i < NDS_REBIRTH_HALO_TEXTURE_COUNT; i++)
    {
        NDSRebirthHaloTextureFill fill;

        if (sNdsRendererRebirthHaloTextureName[i] != 0u)
        {
            continue;
        }
        fill.pixels = textures[i].pixels;
        fill.bytes = textures[i].bytes;
        if (((textures[i].a5i3 != FALSE) ?
                 ndsRendererHardwarePrepareIFCommonCloudAtlas(
                     textures[i].width, textures[i].height,
                     textures[i].palette,
                     ndsRendererRebirthHaloTextureFill, &fill,
                     &sNdsRendererRebirthHaloTextureName[i]) :
                 ndsRendererHardwarePrepareIFCommonPal16Atlas(
                     textures[i].width, textures[i].height,
                     textures[i].palette,
                     ndsRendererRebirthHaloTextureFill, &fill,
                     &sNdsRendererRebirthHaloTextureName[i])) == FALSE)
        {
            return FALSE;
        }
        gNdsRebirthHaloNativeTexturePrepareCount++;
    }
    return TRUE;
#else
    return FALSE;
#endif
}

void ndsRendererHardwareReleaseIFCommonCloudAtlas(u32 *texture_name)
{
#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    int name;

    if ((texture_name == NULL) || (*texture_name == 0u))
    {
        return;
    }
    name = (int)*texture_name;
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareFencedGlDeleteTextures(1, &name);
    if (sNdsRendererHardwareBoundTextureName == *texture_name)
    {
        sNdsRendererHardwareBoundTextureName = 0u;
    }
    sNdsRendererHardwareActiveTextureEntry = NULL;
    *texture_name = 0u;
#else
    if (texture_name != NULL)
    {
        *texture_name = 0u;
    }
#endif
}

static v16 ndsRendererHardwareIFCommonScreenX(s32 pixel_q16)
{
    s64 scaled = (s64)pixel_q16 * 32;
    s64 rounded = (scaled >= 0) ? (scaled + 0x8000) >> 16 :
                                     -(((-scaled) + 0x8000) >> 16);

    rounded -= 4096;
    if (rounded < -32768)
    {
        rounded = -32768;
    }
    else if (rounded > 32767)
    {
        rounded = 32767;
    }
    return (v16)rounded;
}

static v16 ndsRendererHardwareIFCommonScreenY(s32 pixel_q16)
{
    s64 scaled = (s64)pixel_q16 * 128;
    const s64 denominator = 3 * 65536;
    s64 rounded = (scaled >= 0) ?
        (scaled + (denominator / 2)) / denominator :
        -(((-scaled) + (denominator / 2)) / denominator);

    rounded = 4096 - rounded;
    if (rounded < -32768)
    {
        rounded = -32768;
    }
    else if (rounded > 32767)
    {
        rounded = 32767;
    }
    return (v16)rounded;
}

s32 ndsRendererHardwareDrawIFCommonCloudAtlas(
    u32 texture_name, s32 x_q16, s32 y_q16,
    s32 width_q16, s32 height_q16,
    u32 texture_x, u32 texture_y, u32 texture_width,
    u32 texture_height, u32 poly_id)
{
#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    NDSRendererIFCommonCloudDraw *draw;

    if ((texture_name == 0u) || (width_q16 <= 0) || (height_q16 <= 0) ||
        (texture_width == 0u) || (texture_height == 0u) ||
        (texture_x + texture_width > 256u) ||
        (texture_y + texture_height > 128u) || (poly_id > 63u) ||
        (sNdsRendererIFCommonCloudQueueCount >=
         NDS_RENDERER_IFCOMMON_CLOUD_QUEUE_COUNT))
    {
        return FALSE;
    }
    draw = &sNdsRendererIFCommonCloudQueue[
        sNdsRendererIFCommonCloudQueueCount++];
    draw->texture_name = texture_name;
    draw->x_q16 = x_q16;
    draw->y_q16 = y_q16;
    draw->width_q16 = width_q16;
    draw->height_q16 = height_q16;
    draw->texture_x = texture_x;
    draw->texture_y = texture_y;
    draw->texture_width = texture_width;
    draw->texture_height = texture_height;
    draw->poly_id = poly_id;
    gNdsRendererIFCommonCloudQueuedCount++;
    return TRUE;
#else
    (void)texture_name;
    (void)x_q16;
    (void)y_q16;
    (void)width_q16;
    (void)height_q16;
    (void)texture_x;
    (void)texture_y;
    (void)texture_width;
    (void)texture_height;
    (void)poly_id;
    return FALSE;
#endif
}

static void ndsRendererHardwareEmitIFCommonClouds(void)
{
#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    u32 draw_index;

    if (sNdsRendererIFCommonCloudQueueCount == 0u)
    {
        return;
    }

    /* Emit IFCommon traffic, Contour, and Light quads at the final renderer
     * boundary in the order queued by the source pass. */
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glColor(RGB15(31, 31, 31));
    for (draw_index = 0u;
         draw_index < sNdsRendererIFCommonCloudQueueCount; draw_index++)
    {
        const NDSRendererIFCommonCloudDraw *draw =
            &sNdsRendererIFCommonCloudQueue[draw_index];
        v16 left = ndsRendererHardwareIFCommonScreenX(draw->x_q16);
        v16 right = ndsRendererHardwareIFCommonScreenX(
            draw->x_q16 + draw->width_q16);
        v16 top = ndsRendererHardwareIFCommonScreenY(draw->y_q16);
        v16 bottom = ndsRendererHardwareIFCommonScreenY(
            draw->y_q16 + draw->height_q16);
        t16 tex_left = (t16)(draw->texture_x << 4);
        t16 tex_right = (t16)((draw->texture_x + draw->texture_width) << 4);
        t16 tex_top = (t16)(draw->texture_y << 4);
        t16 tex_bottom = (t16)((draw->texture_y + draw->texture_height) << 4);
        /* Opaque A3 texels update depth on DS. Give each later source SObj a
         * one-step-nearer depth so frame, lamps, contour, and Light compose in
         * painter order instead of the first quad masking every successor. */
        v16 depth = (v16)(-4080 - (s32)draw_index);

        ndsRendererHardwareBindTextureState((int)draw->texture_name);
        ndsRendererHardwareSetPolyFmt(
            POLY_CULL_NONE | POLY_ALPHA(31) | POLY_ID(draw->poly_id));
        glBegin(GL_TRIANGLE);
#define NDS_IFCOMMON_CLOUD_VERTEX(s, t, x, y) do { \
    glTexCoord2t16((s), (t)); \
    glVertex3v16((x), (y), depth); \
} while (0)
        NDS_IFCOMMON_CLOUD_VERTEX(tex_left, tex_top, left, top);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_left, tex_bottom, left, bottom);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_right, tex_top, right, top);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_right, tex_top, right, top);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_left, tex_bottom, left, bottom);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_right, tex_bottom, right, bottom);
#undef NDS_IFCOMMON_CLOUD_VERTEX
        gNdsRendererIFCommonCloudEmittedCount++;
    }
    sNdsRendererHardwareBoundTextureName =
        sNdsRendererIFCommonCloudQueue[
            sNdsRendererIFCommonCloudQueueCount - 1u].texture_name;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    sNdsRendererHardwareMatrixGeneration = 0u;
    sNdsRendererHardwareSubmitted = TRUE;
    sNdsRendererIFCommonCloudQueueCount = 0u;
#endif
}

static u32 ndsRendererHardwareTextureNextPow2(u32 value)
{
    u32 out = 8u;

    while ((out < value) && (out < NDS_RENDERER_HW_TEXTURE_MAX_WIDTH))
    {
        out <<= 1;
    }
    return out;
}

static s32 ndsRendererHardwareTextureMaskedClampNeedsWrap(
    u32 mode, u32 mask, u32 upload_extent, u32 tile_extent)
{
    u32 mask_extent;
    u32 sampler_extent;

    /* RDP mask repeat/mirror can occur before the logical tile clamp edge. */
    if (((mode & NDS_RENDERER_TX_CLAMP) == 0u) || (mask == 0u) ||
        (mask >= 31u) || (upload_extent == 0u) || (tile_extent == 0u))
    {
        return FALSE;
    }
    mask_extent = 1u << mask;
    if ((upload_extent != mask_extent) || (tile_extent <= mask_extent))
    {
        return FALSE;
    }
    sampler_extent = upload_extent;
    if ((mode & NDS_RENDERER_TX_MIRROR) != 0u)
    {
        sampler_extent <<= 1;
    }
    return (((mode & NDS_RENDERER_TX_MIRROR) != 0u) ||
            (sampler_extent != tile_extent)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareTextureMaterializesMaskedClamp(
    u32 mode, u32 mask, u32 source_extent, u32 tile_extent)
{
    u32 mask_extent;

    if (((mode & NDS_RENDERER_TX_CLAMP) == 0u) || (mask == 0u) ||
        (mask >= 31u) || (source_extent == 0u) ||
        (tile_extent > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH))
    {
        return FALSE;
    }
    mask_extent = 1u << mask;
    return ((tile_extent > mask_extent) &&
            (source_extent >= mask_extent) &&
            (source_extent <= tile_extent)) ? TRUE : FALSE;
}

static u32 ndsRendererHardwareTextureMaskedAddress(
    u32 coord, u32 mode, u32 mask)
{
    u32 mask_extent = 1u << mask;
    u32 period = coord >> mask;
    u32 local = coord & (mask_extent - 1u);

    if (((mode & NDS_RENDERER_TX_MIRROR) != 0u) &&
        ((period & 1u) != 0u))
    {
        local = mask_extent - 1u - local;
    }
    return local;
}

static u32 ndsRendererHardwareTextureParams(
    const NDSRendererStats *stats,
    const NDSRendererTileState *render_tile,
    u32 upload_width,
    u32 upload_height)
{
    u32 params;
    s32 wrap_s;
    s32 wrap_t;

    if (render_tile == NULL)
    {
        return TEXGEN_OFF;
    }

    params = (ndsRendererHardwareUseTextureMatrix(stats) != FALSE) ?
        TEXGEN_TEXCOORD : TEXGEN_OFF;
    wrap_s = ((render_tile->cms & NDS_RENDERER_TX_CLAMP) == 0u) ||
        ndsRendererHardwareTextureMaskedClampNeedsWrap(
            render_tile->cms, render_tile->masks, upload_width,
            render_tile->width);
    wrap_t = ((render_tile->cmt & NDS_RENDERER_TX_CLAMP) == 0u) ||
        ndsRendererHardwareTextureMaskedClampNeedsWrap(
            render_tile->cmt, render_tile->maskt, upload_height,
            render_tile->height);
    if (wrap_s != FALSE)
    {
        params |= GL_TEXTURE_WRAP_S;
    }
    if ((wrap_s != FALSE) &&
        ((render_tile->cms & NDS_RENDERER_TX_MIRROR) != 0u))
    {
        params |= GL_TEXTURE_FLIP_S;
    }
    if (wrap_t != FALSE)
    {
        params |= GL_TEXTURE_WRAP_T;
    }
    if ((wrap_t != FALSE) &&
        ((render_tile->cmt & NDS_RENDERER_TX_MIRROR) != 0u))
    {
        params |= GL_TEXTURE_FLIP_T;
    }
    return params;
}

static u32 ndsRendererHardwareMergeTextureParams(u32 params)
{
    u32 current = (u32)glGetTexParameter();

    current &= ~NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK;
    current |= params & NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK;
    return current;
}

static void NDS_R2_ITCM_PACK2_CODE ndsRendererHardwareApplyTextureParams(u32 params)
{
    if (((sNdsRendererGXStateShadow.valid_mask &
          NDS_RENDERER_GX_STATE_TEXTURE_PARAMS) != 0u) &&
        (sNdsRendererGXStateShadow.texture_params == params))
    {
#if NDS_RENDERER_M3_PHASE0_PROFILE
        gNdsRendererM3G2TextureParamSkipCount++;
#endif
        return;
    }
    glTexParameter(GL_TEXTURE_2D, (int)params);
    sNdsRendererGXStateShadow.texture_params = params;
    sNdsRendererGXStateShadow.valid_mask |=
        NDS_RENDERER_GX_STATE_TEXTURE_PARAMS;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererM3G2TextureParamWriteCount++;
#endif
}

#define NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT 128u

typedef struct NDSRendererStageTextureSite
{
    const Gfx *site;
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 state_hash1;
    u32 state_hash2;
    u32 entry_generation;
    u32 format;
    u32 size;
    u32 width;
    u32 height;
    u32 uses_texel1;
    u32 prim_lod_fraction;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 semantic_key_hash;
    u32 semantic_params;
    u32 texel1_tile_state;
    u32 texel1_primary_state;
    u32 texel1_image0;
    u32 texel1_image1;
#endif
} NDSRendererStageTextureSite;

static NDSRendererStageTextureSite
    sNdsRendererStageTextureSites[NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT];
static u32 sNdsRendererStageTextureSiteNext;

_Static_assert(
    sizeof(sNdsRendererStageTextureSites) <= (12u * 1024u),
    "stage texture-site plans must stay below 12 KiB");

static void NDS_TASK82_ITCM_CODE ndsRendererHardwareBindTextureName(
    NDSRendererStats *stats,
    u32 texture_name);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererProfileTextureCacheEntry(
    const NDSRendererHardwareTextureCacheEntry *entry);
static void ndsRendererProfileTexturePixel(u16 color, u32 *green_texels,
                                           u32 *nonwhite_texels);
static void ndsRendererProfileTextureFormat(
    volatile u32 *mask,
    u32 format,
    u32 size);
static void ndsRendererRecordTextureLaneUse(
    const NDSRendererConfig *config,
    u32 format,
    u32 size);
#endif

static u32 ndsRendererStageTextureSiteSlot(const Gfx *site)
{
    u32 value = (u32)(uintptr_t)site;

    value ^= value >> 11;
    value ^= value >> 19;
    return value & (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
}

static NDSRendererStageTextureSite *ndsRendererStageTextureSiteFind(
    const NDSRendererTraversalState *state,
    const NDSRendererStats *stats)
{
    u32 slot;
    u32 probe;

    if ((state == NULL) || (stats == NULL) ||
        (state->source_command_site == NULL))
    {
        return NULL;
    }
    slot = ndsRendererStageTextureSiteSlot(state->source_command_site);
    for (probe = 0u; probe < NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT; probe++)
    {
        NDSRendererStageTextureSite *plan =
            &sNdsRendererStageTextureSites[slot];

        if (plan->site == NULL)
        {
            return NULL;
        }
        if ((plan->site == state->source_command_site) &&
            (plan->state_hash1 == stats->texture_source_hash1) &&
            (plan->state_hash2 == stats->texture_source_hash2))
        {
            return plan;
        }
        slot = (slot + 1u) &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
    }
    return NULL;
}

static s32 ndsRendererStageTextureSiteTryBind(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    const NDSRendererConfig *config)
{
    NDSRendererStageTextureSite *plan;
    NDSRendererHardwareTextureCacheEntry *entry;

    if (sNdsRendererStageTextureSitesEnabled == 0u)
    {
        return FALSE;
    }
    gNdsG1SiteConsults++;
    plan = ndsRendererStageTextureSiteFind(state, stats);
    if (plan == NULL)
    {
        return FALSE;
    }
    entry = plan->entry;
    if ((entry == NULL) || (entry->ready == 0u) ||
        (entry->key_generation != plan->entry_generation) ||
        ((plan->uses_texel1 != 0u) &&
         (plan->prim_lod_fraction != stats->prim_lod_fraction)))
    {
        return FALSE;
    }

    entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    if (entry->pinned != 0u)
    {
        ndsRendererHardwareRecordBattleStaticTextureHit(entry);
    }
    if (sNdsRendererHardwareActiveTextureEntry != entry)
    {
        ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        ndsRendererHardwareApplyTextureParams(entry->params);
        sNdsRendererHardwareActiveTextureEntry = entry;
    }
    gNdsG1SiteHits++;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = plan->format;
    stats->hardware_texture_width = plan->width;
    stats->hardware_texture_height = plan->height;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererSemanticLastTextureKeyHash = plan->semantic_key_hash;
    sNdsRendererSemanticLastTextureParams = plan->semantic_params;
    ndsRendererRecordTextureLaneUse(config, plan->format, plan->size);
    if (plan->uses_texel1 != 0u)
    {
        ndsRendererRecordTextureLaneUse(config, plan->format, plan->size);
        ndsRendererProfileRecordTexel1Composite();
        gNdsRendererProfileTexel1LastTileState = plan->texel1_tile_state;
        gNdsRendererProfileTexel1LastPrimaryState =
            plan->texel1_primary_state;
        gNdsRendererProfileTexel1LastFraction = plan->prim_lod_fraction;
        gNdsRendererProfileTexel1LastImage0 = plan->texel1_image0;
        gNdsRendererProfileTexel1LastImage1 = plan->texel1_image1;
    }
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureBindFormatMask,
        plan->format, plan->size);
    ndsRendererProfileTextureCacheEntry(entry);
#endif
    return TRUE;
}

static void ndsRendererStageTextureSiteRemember(
    const NDSRendererTraversalState *state,
    const NDSRendererStats *stats,
    NDSRendererHardwareTextureCacheEntry *entry,
    u32 format,
    u32 size,
    u32 width,
    u32 height)
{
    NDSRendererStageTextureSite *plan = NULL;
    NDSRendererStageTextureSite *empty = NULL;
    u32 slot;
    u32 probe;

    if ((sNdsRendererStageTextureSitesEnabled == 0u) ||
        (state == NULL) || (state->source_command_site == NULL) ||
        (stats == NULL) || (entry == NULL) || (entry->ready == 0u))
    {
        return;
    }
    slot = ndsRendererStageTextureSiteSlot(state->source_command_site);
    for (probe = 0u; probe < NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT; probe++)
    {
        NDSRendererStageTextureSite *candidate =
            &sNdsRendererStageTextureSites[slot];

        if (candidate->site == state->source_command_site)
        {
            plan = candidate;
            break;
        }
        if ((empty == NULL) && (candidate->site == NULL))
        {
            empty = candidate;
        }
        slot = (slot + 1u) &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
    }
    if (plan == NULL)
    {
        plan = empty;
        if (plan != NULL)
        {
            gNdsG1SiteOccupancy++;
        }
    }
    if (plan == NULL)
    {
        plan = &sNdsRendererStageTextureSites[
            sNdsRendererStageTextureSiteNext++ &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u)];
        /* Capacity thrash: the table is full and this slot belongs to a
         * DIFFERENT site, so it is being taken from it. Near zero after warm-up
         * means the working set fits in 128 entries; a large count means this
         * memo is repeating the refuted one's failure with a different key. */
        if ((plan->site != NULL) &&
            (plan->site != state->source_command_site))
        {
            gNdsG1SiteOverwrites++;
        }
    }
    gNdsG1SiteRemembers++;
    plan->site = state->source_command_site;
    plan->entry = entry;
    plan->state_hash1 = stats->texture_source_hash1;
    plan->state_hash2 = stats->texture_source_hash2;
    plan->entry_generation = entry->key_generation;
    plan->format = format;
    plan->size = size;
    plan->width = width;
    plan->height = height;
    plan->uses_texel1 = (ndsRendererHardwareEntryKeyWord(
                             entry,
                             NDS_RENDERER_HW_TEXTURE_KEY_WORD(texel1_image)) !=
                         0u) ? TRUE : FALSE;
    plan->prim_lod_fraction = ndsRendererHardwareEntryKeyWord(
        entry, NDS_RENDERER_HW_TEXTURE_KEY_WORD(prim_lod_fraction));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    plan->semantic_key_hash = sNdsRendererSemanticLastTextureKeyHash;
    plan->semantic_params = sNdsRendererSemanticLastTextureParams;
    plan->texel1_tile_state = gNdsRendererProfileTexel1LastTileState;
    plan->texel1_primary_state = gNdsRendererProfileTexel1LastPrimaryState;
    plan->texel1_image0 = gNdsRendererProfileTexel1LastImage0;
    plan->texel1_image1 = gNdsRendererProfileTexel1LastImage1;
#endif
}

static void ndsRendererTextureSourceHashCommand(
    NDSRendererStats *stats,
    u32 w0,
    u32 w1)
{
    u32 hash1;
    u32 hash2;

    if ((stats == NULL) ||
        (sNdsRendererRuntimeOwner != NDS_RENDERER_PROFILE_OWNER_STAGE))
    {
        return;
    }

    hash1 = stats->texture_source_hash1;
    hash1 ^= w0 ^ ((w1 << 16) | (w1 >> 16));
    hash1 *= 16777619u;
    stats->texture_source_hash1 = hash1;

    hash2 = stats->texture_source_hash2 + w1 + 0x9e3779b9u;
    hash2 = (hash2 << 7) | (hash2 >> 25);
    stats->texture_source_hash2 = hash2 ^ w0 ^ 0x85ebca6bu;
}

static u32 ndsRendererHardwareAlphaStateKey(const NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return 0u;
    }
    return (stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) |
           ((stats->blend_color & 0xffu) << 8);
}

static u32 ndsRendererHardwareFogStateKey(const NDSRendererStats *stats)
{
    if ((stats == NULL) ||
        (stats->fog_status == 0u) ||
        (stats->fog_max <= stats->fog_min))
    {
        return 0u;
    }
    return ((u32)stats->fog_min & 0x3ffu) |
           (((u32)stats->fog_max & 0x3ffu) << 10) |
           ((stats->fog_color & 0xfffu) << 20);
}

static void NDS_TASK82_ITCM_CODE ndsRendererHardwareBindTextureName(
    NDSRendererStats *stats,
    u32 texture_name)
{
#if NDS_TASK107_RENDER_STATE_CENSUS
    u32 i;
    s32 seen = FALSE;

    gNdsTask107BindRequests++;
    if (sNdsTask107BindFrameSerial != sNdsRendererHardwareFrameSerial)
    {
        sNdsTask107BindFrameSerial = sNdsRendererHardwareFrameSerial;
        sNdsTask107BindNameCount = 0u;
    }
    if (texture_name == 0u)
    {
        gNdsTask107BindZeroNameExits++;
        return;
    }
    if (sNdsRendererHardwareBoundTextureName == texture_name)
    {
        gNdsTask107BindCurrentNameElisions++;
        return;
    }
    gNdsTask107BindIssues++;
    for (i = 0u; i < sNdsTask107BindNameCount; i++)
    {
        if (sNdsTask107BindNames[i] == texture_name)
        {
            seen = TRUE;
            break;
        }
    }
    if (seen != FALSE)
    {
        gNdsTask107BindRevisitIssues++;
    }
    else if (sNdsTask107BindNameCount < NDS_TASK107_BIND_NAME_CAPACITY)
    {
        sNdsTask107BindNames[sNdsTask107BindNameCount++] = texture_name;
    }
    else
    {
        gNdsTask107BindNameSetOverflow++;
    }
    {
        ndsRendererHardwareEndBatch();
        ndsRendererHardwareBindTextureState((int)texture_name);
        sNdsRendererHardwareBoundTextureName = texture_name;
        ndsRendererProfileRecordTextureBind();
        if (stats != NULL)
        {
            stats->hardware_texture_bind_count++;
        }
    }
#else
    if (texture_name == 0u)
    {
        return;
    }
    if (sNdsRendererHardwareBoundTextureName != texture_name)
    {
        ndsRendererHardwareEndBatch();
        ndsRendererHardwareBindTextureState((int)texture_name);
        sNdsRendererHardwareBoundTextureName = texture_name;
        ndsRendererProfileRecordTextureBind();
        if (stats != NULL)
        {
            stats->hardware_texture_bind_count++;
        }
    }
#endif
}

static void ndsRendererHardwareReleaseBattleStaticTextureEntries(void)
{
    u32 i;

    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];

        if (entry->pinned != 0u)
        {
            (void)ndsRendererHardwareReleaseTexture(entry);
        }
    }
}

/* Materialise a generated static key against the CURRENT reloc bases.
 *
 * Generated records deliberately store source asset IDs + offsets rather than
 * taskman-heap addresses.  The initial preload used to be the only place that
 * converted those offsets to pointers, which quietly made a pinned texture's
 * identity depend on the reloc file never moving afterward. Dream Land does
 * replace stage files during scene construction, however: the converted DS
 * payload remains perfectly valid in VRAM while the same source O2R asset is
 * reloaded at a new heap address. Keep pointer construction in one helper so
 * initial preload and mutation refresh obey exactly the same bounds contract. */
static s32 ndsRendererHardwareBuildBattleStaticTextureKey(
    const NDSBattlePlayableStaticTextureRecord *record,
    NDSRendererHardwareTextureKey *key)
{
    const void *image_base;
    const void *tlut_base;
    u32 image_size;
    u32 tlut_size;
    u32 texel1_offset;

    if ((record == NULL) || (key == NULL) ||
        (ndsRelocGetLoadedAssetView(
             record->image_asset_id, &image_base, &image_size) == FALSE) ||
        (ndsRelocGetLoadedAssetView(
             record->tlut_asset_id, &tlut_base, &tlut_size) == FALSE) ||
        (record->image_offset >= image_size) ||
        (record->tlut_offset >= tlut_size) ||
        ((uintptr_t)image_base >
         (uintptr_t)(0xffffffffu - record->image_offset)) ||
        ((uintptr_t)tlut_base >
         (uintptr_t)(0xffffffffu - record->tlut_offset)) ||
        (record->key_words[
             NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD] !=
         record->image_offset) ||
        (record->key_words[
             NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD] !=
         record->tlut_offset))
    {
        return FALSE;
    }
    texel1_offset = record->key_words[
        NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD];
    if ((texel1_offset != 0u) &&
        ((texel1_offset >= image_size) ||
         ((uintptr_t)image_base >
          (uintptr_t)(0xffffffffu - texel1_offset))))
    {
        return FALSE;
    }

    memcpy(key, record->key_words, sizeof(*key));
    key->image = (u32)(uintptr_t)((const u8 *)image_base +
                                  record->image_offset);
    key->tlut_image = (u32)(uintptr_t)((const u8 *)tlut_base +
                                       record->tlut_offset);
    key->texel1_image = (texel1_offset != 0u) ?
        (u32)(uintptr_t)((const u8 *)image_base + texel1_offset) : 0u;
    return ((key->width == record->logical_width) &&
            (key->height == record->logical_height)) ? TRUE : FALSE;
}

s32 ndsRendererHardwareRefreshBattleStaticTexturePointers(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 record_index;
    u32 refreshed = 0u;

    if (sNdsRendererBattleStaticTexturePrepared == 0u)
    {
        return TRUE;
    }
    for (record_index = 0u;
         record_index < ndsBattlePlayableStaticTextureKeyCount();
         record_index++)
    {
        const NDSBattlePlayableStaticTextureRecord *record =
            ndsBattlePlayableStaticTextureRecordAt(record_index);
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[record_index];
        NDSRendererHardwareTextureKey key;
        u32 slot = ndsRendererHardwareEntrySlot(entry);
        u32 *pointers;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        u32 key_hash;
#endif

        if ((record == NULL) || (slot != record_index) ||
            (entry->ready == 0u) || (entry->pinned == 0u) ||
            (entry->static_record_plus1 != record_index + 1u) ||
            (ndsRendererHardwareBuildBattleStaticTextureKey(record, &key) ==
             FALSE))
        {
            return FALSE;
        }
        pointers = sNdsRendererHardwareStaticKeyPointers[slot];
        if ((pointers[0] == key.image) &&
            (pointers[1] == key.tlut_image) &&
            (pointers[2] == key.texel1_image))
        {
            continue;
        }

#if NDS_RENDERER_PROFILE_LEVEL < 2
        /* Remove under the OLD pointer-derived hash before changing identity. */
        ndsRendererHardwareTextureLookupRemove(entry);
        key_hash = ndsRendererHardwareTextureKeyHash(&key);
#endif
        ndsRendererHardwareEntrySetStaticKey(entry, &key);
        sNdsRendererHardwareTextureKeyGeneration++;
        if (sNdsRendererHardwareTextureKeyGeneration == 0u)
        {
            sNdsRendererHardwareTextureKeyGeneration++;
        }
        entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        entry->key_hash = key_hash;
        ndsRendererHardwareTextureLookupInsert(entry);
#endif
        refreshed++;
    }
    if (refreshed != 0u)
    {
        /* Any caller retaining the old entry identity must revalidate through
         * key_generation; the active bind has the same rule but clearing this
         * pointer also prevents a stale same-entry fast path in this frame. */
        sNdsRendererHardwareActiveTextureEntry = NULL;
        gNdsRendererBattleStaticTextureRefreshCount++;
        gNdsRendererBattleStaticTextureRefreshedEntryCount += refreshed;
    }
    return TRUE;
#else
    return TRUE;
#endif
}

s32 ndsRendererHardwarePrepareBattleStaticTextures(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    FILE *file = NULL;
    long payload_size;
    u32 record_index;

    if (gNdsRendererBattleStaticTextureEnabled == 0u)
    {
        return FALSE;
    }
    if (sNdsRendererBattleStaticTexturePrepared != 0u)
    {
        return TRUE;
    }

    gNdsRendererBattleStaticTexturePrepareCount++;
    gNdsRendererBattleStaticTexturePrepareFailCount = 0u;
    gNdsRendererBattleStaticTexturePreparedCount = 0u;
    gNdsRendererBattleStaticTexturePreparedBytes = 0u;
    gNdsRendererBattleStaticTextureArmCount = 0u;
    gNdsRendererBattleStaticTexturePinnedHitCount = 0u;
    gNdsRendererBattleStaticTextureSeenMask = 0u;
    gNdsRendererBattleStaticTextureOwnerMask = 0u;
    gNdsRendererBattleStaticTextureViolationCount = 0u;
    gNdsRendererBattleStaticTextureTeardownCount = 0u;
    gNdsRendererBattleStaticTextureFirstAddress = 0u;
    gNdsRendererBattleStaticTextureEndAddress = 0u;
    gNdsRendererBattleStaticTextureAllocationSpanBytes = 0u;
    gNdsRendererBattleStaticTextureBankMask = 0u;
    /* Reset with the family they belong to, so a per-scene reading is a per-
     * scene reading. The first-reject latch in particular is only meaningful
     * against the scene it was armed in. */
    gNdsRendererStageOwnerRejectCount = 0u;
    gNdsRendererStageOwnerLastRejectReason = 0u;
    gNdsRendererStageOwnerFirstRejectReason = 0u;
    gNdsRendererStageOwnerAbortCount = 0u;
    gNdsRendererStageOwnerPostArmRejectCount = 0u;
    gNdsRendererStaticTexturePreparedNow = 0u;
    sNdsRendererBattleStaticTextureArmed = FALSE;

    /* A battle owns the cache from this point forward. Starting empty makes
     * the generated residency and its exact VRAM span deterministic. */
    ndsRendererHardwareDiscardTextureCache();
    file = ndsRendererHardwareFencedTextureFopen(
        NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_PAYLOAD_PATH, "rb");
    if (file == NULL)
    {
        goto fail;
    }
    if ((ndsRendererHardwareFencedTextureFseek(file, 0, SEEK_END) != 0) ||
        ((payload_size = ndsRendererHardwareFencedTextureFtell(file)) < 0) ||
        ((u32)payload_size !=
         ndsBattlePlayableStaticTexturePayloadBytes()) ||
        (ndsRendererHardwareFencedTextureFseek(file, 0, SEEK_SET) != 0))
    {
        goto fail;
    }
    /* ONE read for every palette, before the record loop. See the note beside
     * sNdsRendererStaticTexturePaletteBlock: doing this per record put 22 extra
     * NitroFS seeks and reads inside the scene load. */
    {
        u32 palette_block_bytes =
            ndsBattlePlayableStaticTexturePaletteBlockBytes();

        if ((palette_block_bytes >
             sizeof(sNdsRendererStaticTexturePaletteBlock)) ||
            (ndsRendererHardwareFencedTextureFseek(
                 file,
                 (long)ndsBattlePlayableStaticTexturePaletteBlockOffset(),
                 SEEK_SET) != 0) ||
            (ndsRendererHardwareFencedTextureFread(
                 sNdsRendererStaticTexturePaletteBlock, 1,
                 palette_block_bytes, file) != palette_block_bytes))
        {
            goto fail;
        }
    }

    for (record_index = 0u;
         record_index < ndsBattlePlayableStaticTextureKeyCount();
         record_index++)
    {
        const NDSBattlePlayableStaticTextureRecord *record =
            ndsBattlePlayableStaticTextureRecordAt(record_index);
        NDSRendererHardwareTextureKey key;
        NDSRendererHardwareTextureCacheEntry *entry;
        u32 key_hash;
        int size_x;
        int size_y;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        u32 green_texels = 0u;
        u32 nonwhite_texels = 0u;
        u32 x;
        u32 y;
#endif

        if ((record == NULL) || (record->reserved != 0u) ||
            (record->payload_bytes == 0u) ||
            (record->payload_bytes >
             sizeof(sNdsRendererHardwareTextureScratch)) ||
            (record->payload_offset >
             ndsBattlePlayableStaticTexturePayloadBytes()) ||
            (record->payload_bytes >
             ndsBattlePlayableStaticTexturePayloadBytes() -
                 record->payload_offset) ||
            (record->payload_bytes !=
             ndsRendererStaticTextureSpanBytes(record)) ||
            (ndsRendererStaticTextureSpanBytes(record) == 0u) ||
            (ndsRendererHardwareTextureSizeEnum(
                 record->upload_width, &size_x) == FALSE) ||
            (ndsRendererHardwareTextureSizeEnum(
                 record->upload_height, &size_y) == FALSE) ||
            (ndsRendererHardwareBuildBattleStaticTextureKey(record, &key) ==
             FALSE))
        {
            goto fail;
        }

#if NDS_RENDERER_PROFILE_LEVEL < 2
        key_hash = ndsRendererHardwareTextureKeyHash(&key);
#else
        key_hash = 0u;
#endif
        if (ndsRendererHardwareFindTexture(&key, key_hash) != NULL)
        {
            goto fail;
        }
        if ((ndsRendererHardwareFencedTextureFseek(
                 file, (long)record->payload_offset, SEEK_SET) != 0) ||
            (ndsRendererHardwareFencedTextureFread(
                 sNdsRendererHardwareTextureScratch, 1,
                 record->payload_bytes, file) != record->payload_bytes))
        {
            goto fail;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        /* The profile scan reads halfwords, so it only means anything for a
         * direct-colour span. A paletted one is indices. */
        if (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_RGBA)
        {
            for (y = 0u; y < record->logical_height; y++)
            {
                for (x = 0u; x < record->logical_width; x++)
                {
                    ndsRendererProfileTexturePixel(
                        sNdsRendererHardwareTextureScratch[
                            (y * record->upload_width) + x],
                        &green_texels, &nonwhite_texels);
                }
            }
        }
#endif
        if (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL16)
        {
            u32 palette_bytes = (u32)record->palette_entries * sizeof(u16);

            u32 block_index;

            if ((record->palette_entries == 0u) ||
                (record->palette_entries >
                 (u16)(sizeof(sNdsRendererStaticTexturePalette) /
                       sizeof(sNdsRendererStaticTexturePalette[0]))) ||
                (record->palette_offset <
                 ndsBattlePlayableStaticTexturePaletteBlockOffset()))
            {
                goto fail;
            }
            block_index = record->palette_offset -
                ndsBattlePlayableStaticTexturePaletteBlockOffset();
            if ((block_index >
                 ndsBattlePlayableStaticTexturePaletteBlockBytes()) ||
                (palette_bytes >
                 ndsBattlePlayableStaticTexturePaletteBlockBytes() -
                     block_index))
            {
                goto fail;
            }
            memcpy(sNdsRendererStaticTexturePalette,
                   &sNdsRendererStaticTexturePaletteBlock[block_index],
                   palette_bytes);
        }

        /* THE SLOT IS THE RECORD INDEX. That identity is what lets a static
         * entry drop its resident key: reading the other 56 words back out of
         * ROM needs the record, and reading the three pointer words needs
         * sNdsRendererHardwareStaticKeyPointers, and both are addressed by this
         * one number. It also makes the corpus immune to the runtime allocator,
         * which no longer touches slots below STATIC_COUNT at all. */
        if (record_index >= NDS_RENDERER_HW_TEXTURE_STATIC_COUNT)
        {
            goto fail;
        }
        entry = &sNdsRendererHardwareTextureCache[record_index];
        if ((entry->ready != 0u) || (entry->name != 0))
        {
            entry = ndsRendererHardwareReleaseTexture(entry);
        }
        ndsRendererHardwareRecordBattleTextureFence(
            NDS_RENDERER_BATTLE_TEXTURE_FENCE_ALLOC);
        if ((entry->name == 0) &&
            (ndsRendererHardwareFencedGlGenTextures(
                 1, &entry->name) == 0))
        {
            goto fail;
        }
        ndsRendererHardwareBindTextureName(NULL, (u32)entry->name);
        {
            /* Colour 0 is the transparent one ONLY when the generator put a
             * transparent colour there; an image with no transparent texel
             * uses all sixteen entries and index 0 is real art. Reading the
             * palette rather than carrying a flag keeps the two in step. */
            int params = TEXGEN_TEXCOORD;
            GL_TEXTURE_TYPE_ENUM type = GL_RGBA;

            if (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL16)
            {
                type = GL_RGB16;
                if (sNdsRendererStaticTexturePalette[0] == 0u)
                {
                    params |= GL_TEXTURE_COLOR0_TRANSPARENT;
                }
            }
            if (ndsRendererHardwareFencedGlTexImage2D(
                    GL_TEXTURE_2D, 0, type, size_x, size_y, 0,
                    params,
                    sNdsRendererHardwareTextureScratch) == 0)
            {
                (void)ndsRendererHardwareReleaseTexture(entry);
                goto fail;
            }
            if (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL16)
            {
                glColorTableEXT(GL_TEXTURE_2D, 0,
                                (int)record->palette_entries, 0, 0,
                                sNdsRendererStaticTexturePalette);
            }
        }
        {
            uintptr_t first = (uintptr_t)glGetTexturePointer(entry->name);
            uintptr_t end = first + record->payload_bytes;

            if ((first == 0u) || (end <= first) ||
                (end > (uintptr_t)0xffffffffu))
            {
                (void)ndsRendererHardwareReleaseTexture(entry);
                goto fail;
            }
            if ((gNdsRendererBattleStaticTextureFirstAddress == 0u) ||
                (first < gNdsRendererBattleStaticTextureFirstAddress))
            {
                gNdsRendererBattleStaticTextureFirstAddress = (u32)first;
            }
            if (end > gNdsRendererBattleStaticTextureEndAddress)
            {
                gNdsRendererBattleStaticTextureEndAddress = (u32)end;
            }
            if ((first < (uintptr_t)VRAM_B) &&
                (end > (uintptr_t)VRAM_A))
            {
                gNdsRendererBattleStaticTextureBankMask |= 1u << 0;
            }
            if ((first < (uintptr_t)VRAM_C) &&
                (end > (uintptr_t)VRAM_B))
            {
                gNdsRendererBattleStaticTextureBankMask |= 1u << 1;
            }
        }

        ndsRendererHardwareEntrySetStaticKey(entry, &key);
        sNdsRendererHardwareTextureKeyGeneration++;
        if (sNdsRendererHardwareTextureKeyGeneration == 0u)
        {
            sNdsRendererHardwareTextureKeyGeneration++;
        }
        entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        entry->key_hash = key_hash;
#endif
        entry->params = (u32)glGetTexParameter();
        entry->source_texels = (u32)record->logical_width *
            (u32)record->logical_height;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        entry->green_texels = green_texels;
        entry->nonwhite_texels = nonwhite_texels;
#endif
        entry->profile_width = record->upload_width;
        entry->profile_height = record->upload_height;
        entry->last_used_frame = 0u;
        entry->pinned = TRUE;
        entry->static_record_plus1 = record_index + 1u;
        entry->static_owner_mask = record->owner_mask;
        entry->ready = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ndsRendererHardwareTextureLookupInsert(entry);
#endif
        gNdsRendererBattleStaticTexturePreparedCount++;
        gNdsRendererBattleStaticTexturePreparedBytes +=
            record->payload_bytes;
    }

    if (ndsRendererHardwareFencedTextureFclose(file) != 0)
    {
        file = NULL;
        goto fail;
    }
    file = NULL;
    /* THE EXPECTED BANK MASK IS DERIVED, AND PINNING IT AT 3 COST A CYCLE.
     *
     * The span starts at VRAM_A and runs preparedBytes, so which banks it
     * covers is arithmetic: always A, and B only once it passes A's 128 KiB.
     * The literal 3 was true of one corpus size and read as a correctness
     * check. On 2026-08-03 the corpus was repacked to DS paletted -- lossless,
     * same pixels, 136,192 -> 61,696 bytes -- and the whole residency lifecycle
     * failed with actual=(212,1,1,1,0,0): the prepare reached here with a
     * perfectly good 24-texture span that no longer straddled the boundary, and
     * this line rejected it. The renderer then fell back to ordinary texture
     * resolution, which still LOOKS right and still runs at speed, so the
     * failure was invisible outside the verifier.
     *
     * That mis-attribution also poisoned the atlas measurement it was running
     * beside: a stage whose static textures never became resident renders
     * untextured, which is exactly the symptom the 32,768-byte sheet was
     * blamed for. Derive the mask, and a size change stops being a failure. */
    {
        u32 expected_bank_mask =
            (ndsBattlePlayableStaticTexturePreparedBytes() >
             ((u32)VRAM_B - (u32)VRAM_A)) ? 3u : 1u;

        if ((gNdsRendererBattleStaticTexturePreparedCount !=
             ndsBattlePlayableStaticTextureKeyCount()) ||
            (gNdsRendererBattleStaticTexturePreparedBytes !=
             ndsBattlePlayableStaticTexturePreparedBytes()) ||
            ((gNdsRendererBattleStaticTextureAllocationSpanBytes =
              gNdsRendererBattleStaticTextureEndAddress -
              gNdsRendererBattleStaticTextureFirstAddress) !=
             ndsBattlePlayableStaticTexturePreparedBytes()) ||
            (gNdsRendererBattleStaticTextureFirstAddress != (u32)VRAM_A) ||
            (gNdsRendererBattleStaticTextureEndAddress !=
             ((u32)VRAM_A + ndsBattlePlayableStaticTexturePreparedBytes())) ||
            (gNdsRendererBattleStaticTextureBankMask != expected_bank_mask))
        {
            goto fail;
        }
    }
    sNdsRendererBattleStaticTexturePrepared = TRUE;
    gNdsRendererStaticTexturePreparedNow = 1u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;

fail:
    if (file != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(file);
    }
    ndsRendererHardwareReleaseBattleStaticTextureEntries();
    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
    gNdsRendererBattleStaticTexturePreparedCount = 0u;
    gNdsRendererBattleStaticTexturePreparedBytes = 0u;
    gNdsRendererBattleStaticTextureFirstAddress = 0u;
    gNdsRendererBattleStaticTextureEndAddress = 0u;
    gNdsRendererBattleStaticTextureAllocationSpanBytes = 0u;
    gNdsRendererBattleStaticTextureBankMask = 0u;
    gNdsRendererBattleStaticTexturePrepareFailCount++;
    return FALSE;
#else
    if (gNdsRendererBattleStaticTextureEnabled != 0u)
    {
        gNdsRendererBattleStaticTexturePrepareCount++;
        gNdsRendererBattleStaticTexturePrepareFailCount++;
    }
    return FALSE;
#endif
}

#if NDS_R2_PARTICLE_RUNTIME
/* R2-07 particle draw path -- the upload half.
 *
 * ONE atlas, one GL name, and therefore ONE bind for every particle in a
 * frame. That is the whole reason it is an atlas: GL names are the binding
 * constraint (the cache holds NDS_RENDERER_HW_TEXTURE_CACHE_COUNT = 48 and the
 * battle's static set pins 24 of them, against 31 admitted particle frames),
 * and a per-frame scheme would break the triangle batch on every texture
 * change. The generator's measured-safe 64x64 allocation fits the existing
 * scratch and leaves the stage texture resolver on its prepared path.
 *
 * Pinned like the static set, so the cache's LRU cannot evict it mid-match. It
 * is allocated AFTER them and therefore lands above their VRAM span, which is
 * why this does not disturb the static set's first/end address assertions. */
volatile u32 gNdsRendererParticleAtlasPrepareCount;
volatile u32 gNdsRendererParticleAtlasFailCount;
volatile u32 gNdsRendererParticleAtlasBytes;
volatile u32 gNdsRendererWhispyNativePrepareCount;
volatile u32 gNdsRendererWhispyNativeFailCount;
volatile u32 gNdsRendererWhispyNativeBytes;
#if NDS_R2_FOX_BLASTER_GLOW_AOT
volatile u32 gNdsRendererFoxBlasterGlowPrepareCount;
volatile u32 gNdsRendererFoxBlasterGlowFailCount;
volatile u32 gNdsRendererFoxBlasterGlowBytes;
#endif
#if NDS_R2_FOX_GUN_OVERLAY
/* BUGS.md "Fox's pistol model is missing". DrawCount is the engagement proof:
 * the model-part STATE half already rises (gNdsFighterModelPartOnCount), so a
 * zero here means the overlay was skipped, not that the gun was never asked
 * for -- exactly the distinction this row took three cycles to make. */
volatile u32 gNdsRendererFoxGunPrepareCount;
volatile u32 gNdsRendererFoxGunFailCount;
volatile u32 gNdsRendererFoxGunBytes;
volatile u32 gNdsRendererFoxGunDrawCount;
volatile u32 gNdsRendererFoxGunTriangleCount;
#endif

/* Sheets already uploaded when a later one fails are direct scene-owned GL
 * names. Reclaim those names here so a partial prepare cannot leak texture or
 * palette VRAM for the rest of the scene. */
static void ndsRendererParticleAtlasReleaseSheets(void)
{
    u32 sheet;

    for (sheet = 0u; sheet < NDS_PARTICLE_QUAD_ATLAS_SHEETS; sheet++)
    {
        if (sNdsRendererParticleAtlasName[sheet] != 0)
        {
            if (sNdsRendererHardwareBoundTextureName ==
                (u32)sNdsRendererParticleAtlasName[sheet])
            {
                sNdsRendererHardwareBoundTextureName = 0u;
            }
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsRendererParticleAtlasName[sheet]);
            sNdsRendererParticleAtlasName[sheet] = 0;
        }
    }
}

#if NDS_R2_WHISPY_NATIVE_TEXTURES
/* Three source textures, three hardware-native encodings selected AOT by the
 * generator: A5I3 for IA16 dust, A3I5 for I8 air, and lossless PAL16 for leaf
 * frame zero. Upload happens once at scene prepare; the hot path receives only
 * GL names and full-texture UVs. */
static s32 ndsRendererHardwarePrepareWhispyNativeTextures(void)
{
    FILE *file = NULL;
    u32 texture;

    gNdsRendererWhispyNativePrepareCount++;
    file = ndsRendererHardwareFencedTextureFopen(
        NDS_WHISPY_NATIVE_ASSET_PATH, "rb");
    if (file == NULL)
    {
        goto fail;
    }
    for (texture = 0u; texture < NDS_WHISPY_NATIVE_TEXTURE_COUNT; texture++)
    {
        int name = 0;
        u32 format;
        u32 width;
        u32 height;
        u32 texel_offset;
        u32 texel_bytes;
        u32 palette_offset;
        u32 palette_entries;
        int gl_format;
        int params = TEXGEN_TEXCOORD;
        int size_x;
        int size_y;
#if NDS_R2_WHISPY_NATIVE_AOT
        int palette_format = -1;
#endif

        switch (texture)
        {
        case 0u:
            format = NDS_WHISPY_NATIVE_TEXTURE_0_FORMAT;
            width = NDS_WHISPY_NATIVE_TEXTURE_0_WIDTH;
            height = NDS_WHISPY_NATIVE_TEXTURE_0_HEIGHT;
            texel_offset = NDS_WHISPY_NATIVE_TEXTURE_0_TEXEL_OFFSET;
            texel_bytes = NDS_WHISPY_NATIVE_TEXTURE_0_TEXEL_BYTES;
            palette_offset = NDS_WHISPY_NATIVE_TEXTURE_0_PALETTE_OFFSET;
            palette_entries = NDS_WHISPY_NATIVE_TEXTURE_0_PALETTE_ENTRIES;
            break;
        case 1u:
            format = NDS_WHISPY_NATIVE_TEXTURE_1_FORMAT;
            width = NDS_WHISPY_NATIVE_TEXTURE_1_WIDTH;
            height = NDS_WHISPY_NATIVE_TEXTURE_1_HEIGHT;
            texel_offset = NDS_WHISPY_NATIVE_TEXTURE_1_TEXEL_OFFSET;
            texel_bytes = NDS_WHISPY_NATIVE_TEXTURE_1_TEXEL_BYTES;
            palette_offset = NDS_WHISPY_NATIVE_TEXTURE_1_PALETTE_OFFSET;
            palette_entries = NDS_WHISPY_NATIVE_TEXTURE_1_PALETTE_ENTRIES;
            break;
        default:
            format = NDS_WHISPY_NATIVE_TEXTURE_2_FORMAT;
            width = NDS_WHISPY_NATIVE_TEXTURE_2_WIDTH;
            height = NDS_WHISPY_NATIVE_TEXTURE_2_HEIGHT;
            texel_offset = NDS_WHISPY_NATIVE_TEXTURE_2_TEXEL_OFFSET;
            texel_bytes = NDS_WHISPY_NATIVE_TEXTURE_2_TEXEL_BYTES;
            palette_offset = NDS_WHISPY_NATIVE_TEXTURE_2_PALETTE_OFFSET;
            palette_entries = NDS_WHISPY_NATIVE_TEXTURE_2_PALETTE_ENTRIES;
            break;
        }
        if ((texel_bytes > sizeof(sNdsRendererHardwareTextureScratch)) ||
            (palette_entries > NDS_PARTICLE_QUAD_PALETTE_ENTRIES) ||
            (ndsRendererHardwareTextureSizeEnum(width, &size_x) == FALSE) ||
            (ndsRendererHardwareTextureSizeEnum(height, &size_y) == FALSE) ||
            (ndsRendererHardwareFencedTextureFseek(
                 file, (long)texel_offset, SEEK_SET) != 0) ||
            (ndsRendererHardwareFencedTextureFread(
                 sNdsRendererHardwareTextureScratch, 1, texel_bytes, file) !=
             texel_bytes) ||
            (ndsRendererHardwareFencedTextureFseek(
                 file, (long)palette_offset, SEEK_SET) != 0) ||
            (ndsRendererHardwareFencedTextureFread(
                 sNdsRendererParticleAtlasPalette, sizeof(u16),
                 palette_entries, file) != palette_entries))
        {
            goto fail;
        }
        if (format == NDS_PARTICLE_FORMAT_A5I3)
        {
            gl_format = GL_RGB8_A5;
        }
        else if (format == NDS_PARTICLE_FORMAT_A3I5)
        {
            gl_format = GL_RGB32_A3;
        }
        else if (format == NDS_PARTICLE_FORMAT_PAL16)
        {
            gl_format = GL_RGB16;
            params |= GL_TEXTURE_COLOR0_TRANSPARENT;
        }
        else
        {
            goto fail;
        }

        /* Immutable AOT owners have their own lifetime/name table already. Do
         * not consume a dynamic material-cache entry merely to hold that GL
         * name: the four-player texture demand needs those entries for the
         * source fighter materials touched in the current frame. */
        if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
        {
            goto fail;
        }
        sNdsRendererWhispyNativeName[texture] = name;
        ndsRendererHardwareBindTextureName(NULL, (u32)name);
        if (ndsRendererHardwareFencedGlTexImage2D(
                GL_TEXTURE_2D, 0, gl_format, size_x, size_y, 0, params,
                sNdsRendererHardwareTextureScratch) == 0)
        {
            goto fail;
        }
        glColorTableEXT(GL_TEXTURE_2D, 0, (int)palette_entries, 0, 0,
                        sNdsRendererParticleAtlasPalette);
#if NDS_R2_WHISPY_NATIVE_AOT
        /* GFX_PAL_FORMAT is a write-only GX command port, not readable state.
         * Ask libnds for the palette object's encoded PLTT_BASE word so the
         * direct binder and packet stream reproduce glBindTexture exactly. */
        glGetColorTableParameterEXT(
            GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT, &palette_format);
        if (palette_format < 0)
        {
            goto fail;
        }
#endif
#if NDS_R2_WHISPY_NATIVE_AOT
        sNdsRendererWhispyNativeBinding[texture].texture_name =
            (u32)name;
        sNdsRendererWhispyNativeBinding[texture].texture_format =
            (u32)glGetTexParameter();
        sNdsRendererWhispyNativeBinding[texture].palette_format =
            (u32)palette_format;
        sNdsRendererWhispyNativeBinding[texture].palette_name =
            (s32)glGlob->activePalette;
        sNdsRendererWhispyNativeBinding[texture].valid = TRUE;
#endif
    }
    if (ndsRendererHardwareFencedTextureFclose(file) != 0)
    {
        file = NULL;
        goto fail;
    }
    gNdsRendererWhispyNativeBytes = NDS_WHISPY_NATIVE_ASSET_BYTES;
    return TRUE;

fail:
    if (file != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(file);
    }
    gNdsRendererWhispyNativeFailCount++;
    return FALSE;
}
#endif

#if NDS_R2_FOX_BLASTER_GLOW_AOT
/* EFCommon texture 27 is already an exact DS PAL16 payload in the generated
 * particle asset: 16x8 CI4 + fourteen RGBA5551 colours becomes 64 texel bytes
 * plus 28 palette bytes with zero model error. Script 0x62 immediately issues
 * MASKT, making that stored image the TOP HALF of a 16x16 disc. Give it its
 * own immutable GL name so the DS can do the same reflection in TEXIMAGE_PARAM
 * (WRAP_T|FLIP_T); an atlas cell cannot wrap without sampling its neighbours.
 */
static s32 ndsRendererHardwarePrepareFoxBlasterGlowTexture(void)
{
    const NDSParticleTexture *texture =
        &gNdsParticleTextures[27u];
    FILE *file = NULL;
    int name = 0;
    u32 texel_bytes;
    int palette_format = -1;
    int size_x;
    int size_y;
    int params = TEXGEN_TEXCOORD | GL_TEXTURE_COLOR0_TRANSPARENT |
                 GL_TEXTURE_WRAP_T | GL_TEXTURE_FLIP_T;

    gNdsRendererFoxBlasterGlowPrepareCount++;
    texel_bytes = ((u32)texture->width * (u32)texture->height) >> 1;
    if ((texture->width != 16u) || (texture->height != 8u) ||
        (texture->ds_format != NDS_PARTICLE_FORMAT_PAL16) ||
        (texture->palette_entries != 14u) ||
        (texture->data_offset == NDS_PARTICLE_TEXTURE_UNPACKED) ||
        (texture->palette_offset == NDS_PARTICLE_TEXTURE_UNPACKED) ||
        (texel_bytes != 64u) ||
        (texel_bytes > sizeof(sNdsRendererHardwareTextureScratch)) ||
        ((u32)texture->palette_entries >
         NDS_PARTICLE_QUAD_PALETTE_ENTRIES) ||
        (ndsRendererHardwareTextureSizeEnum(texture->width, &size_x) ==
         FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(texture->height, &size_y) ==
         FALSE))
    {
        goto fail;
    }
    file = ndsRendererHardwareFencedTextureFopen(
        NDS_PARTICLE_TEXTURE_ASSET_PATH, "rb");
    if ((file == NULL) ||
        (ndsRendererHardwareFencedTextureFseek(
             file, (long)texture->data_offset, SEEK_SET) != 0) ||
        (ndsRendererHardwareFencedTextureFread(
             sNdsRendererHardwareTextureScratch, 1, texel_bytes, file) !=
         texel_bytes) ||
        (ndsRendererHardwareFencedTextureFseek(
             file,
             (long)(NDS_PARTICLE_PALETTE_ASSET_OFFSET +
                    ((u32)texture->palette_offset * sizeof(u16))),
             SEEK_SET) != 0) ||
        (ndsRendererHardwareFencedTextureFread(
             sNdsRendererParticleAtlasPalette, sizeof(u16),
             texture->palette_entries, file) != texture->palette_entries))
    {
        goto fail;
    }
    if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
    {
        goto fail;
    }
    sNdsRendererFoxBlasterGlowName = name;
    ndsRendererHardwareBindTextureName(NULL, (u32)name);
    if (ndsRendererHardwareFencedGlTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB16, size_x, size_y, 0, params,
            sNdsRendererHardwareTextureScratch) == 0)
    {
        goto fail;
    }
    glColorTableEXT(GL_TEXTURE_2D, 0, texture->palette_entries, 0, 0,
                    sNdsRendererParticleAtlasPalette);
    glGetColorTableParameterEXT(
        GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT, &palette_format);
    if (palette_format < 0)
    {
        goto fail;
    }
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT].texture_name = (u32)name;
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT].texture_format =
            (u32)glGetTexParameter();
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT].palette_format =
        (u32)palette_format;
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT].palette_name =
        (s32)glGlob->activePalette;
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT].valid = TRUE;
    if (ndsRendererHardwareFencedTextureFclose(file) != 0)
    {
        file = NULL;
        goto fail;
    }
    file = NULL;
    gNdsRendererFoxBlasterGlowBytes =
        texel_bytes + ((u32)texture->palette_entries * sizeof(u16));
    return TRUE;

fail:
    if (file != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(file);
    }
    if (sNdsRendererFoxBlasterGlowName != 0)
    {
        ndsRendererHardwareFencedGlDeleteTextures(
            1, &sNdsRendererFoxBlasterGlowName);
    }
    sNdsRendererFoxBlasterGlowName = 0;
    memset(&sNdsRendererWhispyNativeBinding[
               NDS_WHISPY_NATIVE_TEXTURE_COUNT], 0,
           sizeof(sNdsRendererWhispyNativeBinding[0]));
    gNdsRendererFoxBlasterGlowBytes = 0u;
    gNdsRendererFoxBlasterGlowFailCount++;
    return FALSE;
}
#endif


s32 ndsRendererHardwarePrepareParticleAtlas(void)
{
    FILE *file = NULL;
    int size_x;
    int size_y;
    u32 sheet;

    if (sNdsRendererParticleAtlasPrepared != 0u)
    {
        return TRUE;
    }
    gNdsRendererParticleAtlasPrepareCount++;
    /* ONE SHEET AT A TIME against the scratch. The sheet size is fixed at the
     * 8,192-byte allocation that has never been refused and the SHEET COUNT is
     * what grew, so this reads and uploads NDS_PARTICLE_QUAD_ATLAS_SHEETS times
     * rather than asking the allocator for one block four times the size --
     * which is the request that broke stage texture resolves at 16,384 and at
     * 32,768. The palette is shared and gets its own small read below. */
    if ((NDS_PARTICLE_QUAD_SHEET_BYTES >
          sizeof(sNdsRendererHardwareTextureScratch)) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_PARTICLE_QUAD_ATLAS_WIDTH, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_PARTICLE_QUAD_ATLAS_HEIGHT, &size_y) == FALSE))
    {
        goto fail;
    }
    file = ndsRendererHardwareFencedTextureFopen(
        NDS_PARTICLE_QUAD_ASSET_PATH, "rb");
    if (file == NULL)
    {
        goto fail;
    }
    if ((ndsRendererHardwareFencedTextureFseek(
             file, (long)NDS_PARTICLE_QUAD_PALETTE_OFFSET, SEEK_SET) != 0) ||
        (ndsRendererHardwareFencedTextureFread(
             sNdsRendererParticleAtlasPalette, 1,
             sizeof(sNdsRendererParticleAtlasPalette), file) !=
         sizeof(sNdsRendererParticleAtlasPalette)))
    {
        goto fail;
    }

    for (sheet = 0u; sheet < NDS_PARTICLE_QUAD_ATLAS_SHEETS; sheet++)
    {
        int name = 0;

        if ((ndsRendererHardwareFencedTextureFseek(
                 file, (long)(sheet * NDS_PARTICLE_QUAD_SHEET_BYTES),
                 SEEK_SET) != 0) ||
            (ndsRendererHardwareFencedTextureFread(
                 sNdsRendererHardwareTextureScratch, 1,
                 NDS_PARTICLE_QUAD_SHEET_BYTES, file) !=
             NDS_PARTICLE_QUAD_SHEET_BYTES))
        {
            goto fail;
        }

        if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
        {
            goto fail;
        }
        sNdsRendererParticleAtlasName[sheet] = name;
        ndsRendererHardwareBindTextureName(NULL, (u32)name);
        /* A3I5, one byte per texel: 32 palette entries and 8 alpha levels.
         *
         * Not GL_RGBA, for the reason that has not changed -- RGB555+A1 gives
         * particles a SINGLE alpha bit, so every soft-edged sprite draws as a
         * hard blob, and it costs two bytes a texel besides.
         *
         * A5I3 (8 entries, 32 alpha levels) held this slot until 2026-08-03 and
         * its reasoning was sound while the sheet carried SHAPE and the batch
         * supplied COLOUR through glColor per quad, exactly as the RDP's prim
         * colour did. Fox's reflector is the asset that does not fit that
         * division: it is two flat tones with no shape at all, so eight shared
         * entries could not hold its specific blues alongside every other
         * effect's greys, and the owner filed it as the wrong asset. Colour has
         * no other source here; alpha resolution does -- 8 levels is four times
         * what the RGB555+A1 failure had, spread over cells that are now at
         * their source size instead of halved. */
        if (ndsRendererHardwareFencedGlTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGB32_A3, size_x, size_y, 0,
                TEXGEN_TEXCOORD, sNdsRendererHardwareTextureScratch) == 0)
        {
            goto fail;
        }
        /* PER SHEET, INSIDE THE LOOP, AND THAT PLACEMENT IS THE WHOLE POINT.
         * glColorTableEXT attaches the palette to the texture name that is
         * BOUND RIGHT NOW, so one call outside this loop would leave three of
         * the four sheets indexing whatever palette memory happened to follow.
         * Four calls means four 64-byte blocks in VRAM F/G -- 256 bytes, and
         * not the allocator that was ever refusing.
         *
         * EACH SHEET NOW GETS ITS OWN COLOURS, which is a change to the ASSET
         * and not to this loop's cost: until 2026-08-14 all four calls handed
         * over the same 32 entries, so the atlas paid for 128 palette slots and
         * used 32. Making the generator emit one table per sheet is what let
         * the packer seat shield-break texture 4 and side-KO textures 11 and 14
         * without the extra texels pulling the shared k-means centres off every
         * texture already on the sheet -- measured, that sharing cost 29 of 31
         * admitted textures accuracy, and per-sheet tables instead make 31 of
         * 36 strictly better than the old build with none worse.
         *
         * Read separately above, so no byte-offset arithmetic into a u16 buffer
         * -- which is what used to hand the hardware texels as a palette when
         * the offset was applied through the wrong pointer type. */
        glColorTableEXT(
            GL_TEXTURE_2D, 0, NDS_PARTICLE_QUAD_PALETTE_ENTRIES, 0, 0,
            &sNdsRendererParticleAtlasPalette[
                sheet * NDS_PARTICLE_QUAD_PALETTE_ENTRIES]);
    }

    if (ndsRendererHardwareFencedTextureFclose(file) != 0)
    {
        file = NULL;
        goto fail;
    }
    file = NULL;
#if NDS_R2_WHISPY_NATIVE_TEXTURES
    if (ndsRendererHardwarePrepareWhispyNativeTextures() == FALSE)
    {
        goto fail;
    }
#endif
#if NDS_R2_FOX_BLASTER_GLOW_AOT
    if (ndsRendererHardwarePrepareFoxBlasterGlowTexture() == FALSE)
    {
        goto fail;
    }
#endif
    sNdsRendererParticleAtlasPrepared = TRUE;
    gNdsRendererParticleAtlasBytes = NDS_PARTICLE_QUAD_ASSET_BYTES;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;

fail:
    if (file != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(file);
    }
    ndsRendererParticleAtlasReleaseSheets();
    ndsRendererHardwareDiscardParticleAtlas();
    gNdsRendererParticleAtlasFailCount++;
    return FALSE;
}

/* 0 is "no atlas", so a sheet index the table never emits fails closed rather
 * than binding sheet 0's texels under another sheet's coordinates. */
u32 ndsRendererHardwareParticleAtlasNameForSheet(u32 sheet)
{
    if ((sNdsRendererParticleAtlasPrepared == 0u) ||
        (sheet >= NDS_PARTICLE_QUAD_ATLAS_SHEETS))
    {
        return 0u;
    }
    return (u32)sNdsRendererParticleAtlasName[sheet];
}

u32 ndsRendererHardwareParticleAtlasName(void)
{
    return ndsRendererHardwareParticleAtlasNameForSheet(0u);
}

u32 ndsRendererHardwareFoxBlasterGlowName(void)
{
#if NDS_R2_FOX_BLASTER_GLOW_AOT
    if (sNdsRendererParticleAtlasPrepared != 0u)
    {
        return (u32)sNdsRendererFoxBlasterGlowName;
    }
#endif
    return 0u;
}

u32 ndsRendererHardwareWhispyNativeName(u32 texture_id)
{
#if NDS_R2_WHISPY_NATIVE_TEXTURES
    if ((sNdsRendererParticleAtlasPrepared != 0u) &&
        (texture_id < NDS_WHISPY_NATIVE_TEXTURE_COUNT))
    {
        return (u32)sNdsRendererWhispyNativeName[texture_id];
    }
#else
    (void)texture_id;
#endif
    return 0u;
}

void ndsRendererHardwareDiscardParticleAtlas(void)
{
    u32 sheet;

    ndsRendererParticleAtlasReleaseSheets();
#if NDS_R2_WHISPY_NATIVE_TEXTURES
    for (sheet = 0u; sheet < NDS_WHISPY_NATIVE_TEXTURE_COUNT; sheet++)
    {
        if (sNdsRendererWhispyNativeName[sheet] != 0)
        {
            if (sNdsRendererHardwareBoundTextureName ==
                (u32)sNdsRendererWhispyNativeName[sheet])
            {
                sNdsRendererHardwareBoundTextureName = 0u;
            }
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsRendererWhispyNativeName[sheet]);
        }
        sNdsRendererWhispyNativeName[sheet] = 0;
#if NDS_R2_WHISPY_NATIVE_AOT
        sNdsRendererWhispyNativeBinding[sheet].texture_name = 0u;
        sNdsRendererWhispyNativeBinding[sheet].texture_format = 0u;
        sNdsRendererWhispyNativeBinding[sheet].palette_format = 0u;
        sNdsRendererWhispyNativeBinding[sheet].palette_name = 0;
        sNdsRendererWhispyNativeBinding[sheet].valid = FALSE;
#endif
    }
#endif
#if NDS_R2_FOX_BLASTER_GLOW_AOT
    if (sNdsRendererFoxBlasterGlowName != 0)
    {
        if (sNdsRendererHardwareBoundTextureName ==
            (u32)sNdsRendererFoxBlasterGlowName)
        {
            sNdsRendererHardwareBoundTextureName = 0u;
        }
        ndsRendererHardwareFencedGlDeleteTextures(
            1, &sNdsRendererFoxBlasterGlowName);
    }
    sNdsRendererFoxBlasterGlowName = 0;
    memset(&sNdsRendererWhispyNativeBinding[
               NDS_WHISPY_NATIVE_TEXTURE_COUNT], 0,
           sizeof(sNdsRendererWhispyNativeBinding[0]));
#endif
    sNdsRendererParticleAtlasPrepared = FALSE;
    gNdsRendererParticleAtlasBytes = 0u;
    gNdsRendererWhispyNativeBytes = 0u;
#if NDS_R2_FOX_BLASTER_GLOW_AOT
    gNdsRendererFoxBlasterGlowBytes = 0u;
#endif
}

/* BUGS.md "VFX get x flattened around stage edges ... why is there a limit
 * anyways???" and "Star KO twinkle not playing in correct spot".
 *
 * THE LIMIT IS v16, AND IT IS ONE BUG WEARING TWO FACES. At the stock x16
 * factor the reach is 32767/16 = 2047.9 world units. Every vertex past that
 * SATURATES on the rail, so a quad straddling it has its far corners pinned to
 * the same coordinate and collapses along that axis -- the owner's "x flattened
 * around stage edges", measured at 1,118 of 5,590 quads a match on Whispy's
 * wind alone, because the live camera reaches 3,148. The same rail is why the
 * Star KO twinkle draws in the wrong place: ftcommondead.c recedes the dying
 * fighter to z = -14,999 and the sparkle spawns AT it, so the fighter (drawn
 * through the ordinary matrix path, which has 20.12 of range) recedes correctly
 * while its twinkle stops dead at -2,047.9 and hangs near the camera. The
 * fighter's own draw never had this limit; only the particle pass did.
 *
 * The range has to come from SCALE, since v16 is 16 bits and no encoding buys
 * it back. A FIXED coarser factor was the previous shape of this and it is the
 * wrong trade: it charges every ordinary hit spark a permanent loss of sub-unit
 * resolution to pay for the one effect a match that leaves the stage.
 *
 * So the factor is chosen per BATCH and escalates only when a quad actually
 * needs it. Each quad's furthest corner picks the smallest shift that keeps it
 * off the rail; if that is coarser than what the batch is running, the pass
 * swaps its compensating modelview scale and continues. Vertices already queued
 * were transformed when they were submitted and are unaffected. An ordinary
 * frame never escalates and keeps the full x16 precision it has always had; a
 * Star KO frame escalates once and spends 0.5 world units of precision on
 * particles that are 15,000 units away. Monotonic within a frame, reset with
 * the batch, so the choice is deterministic rather than order-dependent noise.
 *
 * NDS_RENDERER_PARTICLE_MAX_SCALE_SHIFT 4 reaches +/-32,767 world units, past
 * every blast zone and past the DeadUpStar recession. */
#define NDS_RENDERER_PARTICLE_MAX_SCALE_SHIFT 4u

/* World coordinate -> v16 at the batch's current scale. The scene's modelview
 * carries a NDS_RENDERER_HW_WORLD_UNIT_SHIFT (=8) scale, so at shift 0 one
 * world unit is 2^(12-8) = 16 in vertex space -- the same relation
 * ndsRendererHardwareCoordToV16 applies to integer source vertices, restated
 * for the f32 the particle interpreter holds. Getting this wrong does not
 * fail; it draws the effects at 1/256 or 256x scale, which is why it is
 * spelled out rather than folded into a literal. */
#define NDS_RENDERER_PARTICLE_UNIT_SHIFT NDS_RENDERER_HW_WORLD_UNIT_SHIFT

/* Counted, not assumed: Clamp must read 0 on a fixed build. It is the direct
 * measurement of the reported symptom, so a non-zero value means a quad was
 * still drawn on the rail. */
volatile u32 gNdsParticleWorldClampCount;
volatile u32 gNdsParticleScaleEscalations;
volatile u32 gNdsParticleScaleShiftMax;

static u32 sNdsRendererParticleScaleShift;

/* A world magnitude past this cannot be on screen under any camera this game
 * builds, and converting it would overflow the s32 below -- which is undefined
 * rather than merely wrong. Saturating here keeps the conversion total. */
#define NDS_RENDERER_PARTICLE_WORLD_LIMIT 131072.0F

/* No libm call for three magnitudes per quad; the sign bit is the whole job. */
#define NDS_FABS(v) (((v) < 0.0F) ? -(v) : (v))

static s32 ndsRendererParticleWorldFixed(f32 value)
{
    if (value > NDS_RENDERER_PARTICLE_WORLD_LIMIT)
    {
        value = NDS_RENDERER_PARTICLE_WORLD_LIMIT;
    }
    else if (value < -NDS_RENDERER_PARTICLE_WORLD_LIMIT)
    {
        value = -NDS_RENDERER_PARTICLE_WORLD_LIMIT;
    }
    return (s32)(value * (f32)(1 << (12u -
                                     NDS_RENDERER_PARTICLE_UNIT_SHIFT)));
}

static v16 ndsRendererParticleWorldToV16(f32 value, u32 shift)
{
    s32 scaled = ndsRendererParticleWorldFixed(value) / (s32)(1 << shift);

    if (scaled > 32767) { gNdsParticleWorldClampCount++; return (v16)32767; }
    if (scaled < -32768) { gNdsParticleWorldClampCount++; return (v16)-32768; }
    return (v16)scaled;
}

/* The smallest shift that keeps `extent` (a world magnitude) off the rail. */
static u32 ndsRendererParticleScaleShiftFor(f32 extent)
{
    u32 shift = 0u;
    s32 scaled;

    if (!(extent > 0.0F))
    {
        return 0u;
    }
    scaled = ndsRendererParticleWorldFixed(extent);
    while ((shift < NDS_RENDERER_PARTICLE_MAX_SCALE_SHIFT) &&
           ((scaled / (s32)(1 << shift)) > 32767))
    {
        shift++;
    }
    return shift;
}

static u32 sNdsRendererParticleQuadOpen;
/* The POLY_ALPHA currently latched into the open group, so a quad only pays for
 * a new group when its alpha actually differs. Reset with the batch. */
static u32 sNdsRendererParticleQuadAlpha;
/* How many times a frame's particle pass had to start a new primitive group
 * because the alpha bucket moved. Engagement proof for the per-particle fade:
 * 0 with particles drawing means every live particle shared one alpha, which is
 * what the bug looked like. */
volatile u32 gNdsParticleQuadAlphaBreaks;
/* Companion to the above for the four-sheet atlas: how many times the pass had
 * to rebind because the next quad's cell was packed on a different sheet. */
volatile u32 gNdsParticleQuadSheetBreaks;
/* The sheet currently bound into the open group, so a quad only pays for a
 * rebind when its sheet actually differs. Reset with the batch. */
static u32 sNdsRendererParticleQuadTexture;

/* gNdsParticleMatrixModeSeen records the mode the batch INHERITED, latched
 * before the camera load replaces it -- so it stays 0x12 by design and is the
 * evidence of what the bug was, not a pass/fail. The regression guard is
 * gNdsParticleCameraLoads == gNdsParticleBatchOpens: every batch must load its
 * own camera. gNdsParticleCamT* samples the loaded translation so a silently
 * empty matrix cannot masquerade as a working one. */
volatile u32 gNdsParticleMatrixModeSeen;
volatile u32 gNdsParticleMatrixLoadedSeen;
volatile u32 gNdsParticleBatchOpens;
volatile u32 gNdsParticleCameraLoads;
volatile s32 gNdsParticleCamTx;
volatile s32 gNdsParticleCamTy;
volatile s32 gNdsParticleCamTz;

/* THE PARTICLE PASS HAD NO MATRIX OF ITS OWN.
 *
 * It never loaded one, so every quad rendered under whatever the previously
 * drawn object happened to leave in the hardware. Measured over a match:
 * gNdsParticleMatrixModeSeen = 0x12, i.e. bits 1 and 4 -- PROJECTED_IDENTITY
 * and STAGE_HW_COMPOSE -- across 599 batch opens, varying frame to frame.
 *
 * That is the whole family of "effects are in the wrong place" reports at once.
 * Under PROJECTED_IDENTITY the modelview is identity, so world coordinates are
 * read as view coordinates and the effect renders at the eye: the owner's "the
 * KO effect plays too close to the camera instead of at the fighters' z depth".
 * Under STAGE_HW_COMPOSE it inherits a stage segment's local matrix and lands
 * wherever that segment is: "stray VFX across the stage", and Whispy's wind
 * appearing far from the tree even though its emitter measures source-exact.
 *
 * The positions were never wrong -- the space they were drawn in was. So the
 * game hands the pass its own camera and the batch loads it, exactly as any
 * other root does. Set from lbParticleDrawTextures, which has the game headers
 * this file deliberately does not. */
/* Defined below with the rest of the matrix plumbing; the particle batch is
 * the one caller that sits above it in the file. */
static void ndsRendererCopyMtx20p12ToM4x4(
    const NDSRendererMatrix20p12 *src, m4x4 *dst);
#if NDS_R2_FOX_GUN_OVERLAY
/* Declared here for ndsRendererSubmitFoxGun, which sits above the definition.
 * It is the ONLY correct way to hand this renderer a CPU-composed MVP: it
 * forces GL_PROJECTION to identity and divides the composed homogeneous row by
 * NDS_RENDERER_HW_WORLD_UNIT_SHIFT, which is the other half of the x16 vertex
 * encoding. A submit that loads the MVP itself gets neither. */
static void ndsRendererLoadHardwareRawComposedMatrix(
    const NDSRendererMatrix20p12 *composed, u32 generation);
#endif

static NDSRendererMatrix20p12 sNdsRendererParticleProjection;
static NDSRendererMatrix20p12 sNdsRendererParticleModelview;
static u32 sNdsRendererParticleCameraValid;

void NDS_FIGHTER_PACKET_EVICT(NDS_R2_ITCM_PACK2_CODE)
ndsRendererSetParticleCamera(const NDSRendererMatrix20p12 *projection,
                                  const NDSRendererMatrix20p12 *modelview)
{
    if ((projection == NULL) || (modelview == NULL))
    {
        sNdsRendererParticleCameraValid = FALSE;
        return;
    }
    ndsRendererMatrixCopy20p12(&sNdsRendererParticleProjection, projection);
    ndsRendererMatrixCopy20p12(&sNdsRendererParticleModelview, modelview);
    sNdsRendererParticleCameraValid = TRUE;
}

/* Load the already-normalized world camera into GX. The generic particle
 * opener uses this directly; the Fox blaster's projected no-Z variant below
 * mirrors its modelview normalization while replacing only clip Z. */
static s32 ndsRendererLoadParticleCameraMatrices(void)
{
    NDSRendererMatrix20p12 scaled_modelview;
    m4x4 projection_hw;
    m4x4 modelview_hw;
    u32 col;

    if (sNdsRendererParticleCameraValid == FALSE)
    {
        return FALSE;
    }
    ndsRendererMatrixCopy20p12(&scaled_modelview,
                               &sNdsRendererParticleModelview);
    for (col = 0u; col < 4u; col++)
    {
        scaled_modelview.m[3][col] = ndsRendererRoundShiftS32Signed(
            scaled_modelview.m[3][col],
            NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    ndsRendererCopyMtx20p12ToM4x4(&sNdsRendererParticleProjection,
                                  &projection_hw);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hw);
    ndsRendererCopyMtx20p12ToM4x4(&scaled_modelview, &modelview_hw);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadMatrix4x4(&modelview_hw);
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    gNdsParticleCameraLoads++;
    gNdsParticleCamTx = scaled_modelview.m[3][0];
    gNdsParticleCamTy = scaled_modelview.m[3][1];
    gNdsParticleCamTz = scaled_modelview.m[3][2];
    return TRUE;
}

#if NDS_R2_FOX_BLASTER_QUAD
static s32 ndsRendererHardwareNextProjectedDepth(void);
static void ndsRendererHardwareBindNoTexture(NDSRendererStats *stats);

/* Fox's source list is a projected no-Z weapon. Preserve the cached camera's
 * X/Y/W transform and replace only clip Z with the renderer's painter depth,
 * exactly like the promoted split-matrix Rebirth Halo path. Loading the raw
 * world camera instead leaves the translucent muzzle glow in front of the
 * beam even though the source renderer submits the beam over it. */
static s32 ndsRendererLoadFoxBlasterNoZCameraMatrices(void)
{
    NDSRendererMatrix20p12 projection = sNdsRendererParticleProjection;
    NDSRendererMatrix20p12 scaled_modelview;
    m4x4 projection_hw;
    m4x4 modelview_hw;
    s16 projected_z;
    u32 row;

    if (sNdsRendererParticleCameraValid == FALSE)
    {
        return FALSE;
    }
    projected_z = (s16)ndsRendererHardwareNextProjectedDepth();
    for (row = 0u; row < 4u; row++)
    {
        projection.m[row][2] = (s32)ndsRendererRoundShiftS64(
            (s64)projection.m[row][3] * projected_z, 12u);
    }
    ndsRendererMatrixCopy20p12(&scaled_modelview,
                               &sNdsRendererParticleModelview);
    for (row = 0u; row < 4u; row++)
    {
        scaled_modelview.m[3][row] = ndsRendererRoundShiftS32Signed(
            scaled_modelview.m[3][row],
            NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    ndsRendererCopyMtx20p12ToM4x4(&projection, &projection_hw);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hw);
    ndsRendererCopyMtx20p12ToM4x4(&scaled_modelview, &modelview_hw);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadMatrix4x4(&modelview_hw);
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    gNdsParticleCameraLoads++;
    gNdsParticleCamTx = scaled_modelview.m[3][0];
    gNdsParticleCamTy = scaled_modelview.m[3][1];
    gNdsParticleCamTz = scaled_modelview.m[3][2];
    return TRUE;
}
#endif

#if NDS_R2_WHISPY_NATIVE_AOT
static NDS_RENDERER_FAST_RUN_CODE void
ndsRendererPrepareWhispyQuadState(u32 texture_name, u32 poly_alpha,
                                  u32 texture_slot, u32 submit_route);
static void ndsRendererFlushWhispyNativePacket(void);
#endif

/* One quad, camera-facing, in world space. `right` and `up` are the camera
 * basis the caller derived from the CObj -- computed there because that is
 * where the source's own draw reads the camera, and because it is per-frame
 * work that must not be repeated per particle.
 *
 * The batch is opened lazily on the first quad and closed by
 * ndsRendererEndParticleQuads, so the whole particle pass is ONE glBegin and
 * ONE texture bind however many particles it carries. */
s32 ndsRendererSubmitParticleQuad(u32 atlas_name, const Vec3f *pos, f32 size,
                                  u32 color, u8 alpha,
                                  const Vec3f *right, const Vec3f *up,
                                  u32 mirror_mask,
                                  u32 atlas_x, u32 atlas_y,
                                  u32 atlas_w, u32 atlas_h)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    f32 rx;
    f32 ry;
    f32 rz;
    f32 ux;
    f32 uy;
    f32 uz;
    u32 poly_alpha;

#if NDS_R2_WHISPY_NATIVE_AOT
    /* A generic particle is an ordering fence for route 4: all earlier Whispy
     * commands must reach GX before this source-list entry is submitted. */
    ndsRendererFlushWhispyNativePacket();
#endif
    if ((atlas_name == 0u) || (pos == NULL) || (right == NULL) || (up == NULL))
    {
        return FALSE;
    }
    /* BUGS.md "Whispy blow VFX ... emitted objects turn flat at end of
     * lifetime".
     *
     * A source particle fades: lbparticle.c ramps pc->primcolor toward
     * target_primcolor over primcolor_target_length frames, ALPHA INCLUDED, and
     * the RDP blends with it. This path dropped that channel on the floor --
     * the caller packed r/g/b into a BGR555 word and the pass ran the whole
     * batch at POLY_ALPHA(31). So every particle stayed 100% opaque for its
     * entire life and the tail, which should dissolve, instead sat there as a
     * hard flat blob at exactly the moment the owner noticed it.
     *
     * DS vertex colour carries no alpha, so per-particle alpha has to be
     * POLYGON_ATTR's, which latches at glBegin. The batch therefore reopens
     * when the alpha BUCKET changes rather than staying one glBegin for the
     * pass; particles in a burst share a ramp, so that is far fewer than one
     * per quad, and gNdsParticleQuadAlphaBreaks measures it rather than
     * assuming it.
     *
     * 8-bit source alpha to the hardware's 5 bits, and never to 0: POLY_ALPHA(0)
     * is WIREFRAME on this hardware, not invisible. A fully transparent particle
     * is skipped outright, which is also the cheaper answer. */
    if (alpha == 0u)
    {
        return FALSE;
    }
    poly_alpha = ((u32)alpha >> 3) & 31u;
    if (poly_alpha == 0u)
    {
        poly_alpha = 1u;
    }
#if NDS_R2_WHISPY_NATIVE_AOT
    ndsRendererPrepareWhispyQuadState(atlas_name, poly_alpha, 0u, 2u);
#else
    if ((sNdsRendererParticleQuadOpen != 0u) &&
        ((poly_alpha != sNdsRendererParticleQuadAlpha) ||
         (atlas_name != sNdsRendererParticleQuadTexture)))
    {
        /* NO glEnd() -- ndsRendererEndParticleQuads explains why at length: an
         * extra FIFO write desynchronises this command stream and hung the ROM
         * at GXSTAT=0e008900. POLYGON_ATTR latches at glBegin, so re-issuing
         * the format and starting the next group is the whole operation.
         *
         * A SHEET CHANGE COSTS EXACTLY THE SAME THING, which is what made
         * splitting the atlas across four 8,192-byte allocations affordable:
         * TEXIMAGE_PARAM latches per group just as POLYGON_ATTR does, so
         * rebinding and reopening the group is a rebind plus a glBegin, not a
         * flush. Counted separately from the alpha breaks so the two costs stay
         * attributable -- if SheetBreaks ever approaches the quad count, the
         * pack order and not the sheet count is what needs revisiting. */
        if (atlas_name != sNdsRendererParticleQuadTexture)
        {
            ndsRendererHardwareBindTextureName(NULL, atlas_name);
            sNdsRendererParticleQuadTexture = atlas_name;
            gNdsParticleQuadSheetBreaks++;
        }
        ndsRendererHardwareSetPolyFmt(
            POLY_ALPHA(poly_alpha) | POLY_CULL_NONE | POLY_ID(0));
        if (poly_alpha != sNdsRendererParticleQuadAlpha)
        {
            gNdsParticleQuadAlphaBreaks++;
        }
        sNdsRendererParticleQuadAlpha = poly_alpha;
        glBegin(GL_QUAD);
    }
    if (sNdsRendererParticleQuadOpen == 0u)
    {
        /* TEMPORARY, BUGS.md rows 1/4/6/KO-VFX. The particle pass loads NO
         * matrix, so every quad is drawn under whatever the last object left
         * active. If that is a fighter bone or a stage segment rather than the
         * scene's own camera matrix, particles land in that object's space --
         * which is exactly "effects don't play at correct locations" and "the
         * KO effect plays too close to the camera instead of at the fighters'
         * z depth". Latch the mode so the guess is measured, not assumed. */
        gNdsParticleMatrixModeSeen |= 1u << (sNdsRendererHardwareMatrixMode & 7u);
        gNdsParticleMatrixLoadedSeen = sNdsRendererHardwareMatrixLoaded;
        gNdsParticleBatchOpens++;
        ndsRendererHardwareEndBatch();
        /* Same translation shift every other root load applies
         * (ndsRendererLoadHardwareSplitMatrices): vertices arrive at world
         * x16, so the shared loader brings the camera translation down by the
         * matching WORLD_UNIT_SHIFT. A missing camera preserves the old
         * behavior of proceeding without a camera reload. */
        (void)ndsRendererLoadParticleCameraMatrices();
        /* The scale that compensates the vertex factor, so quads land where
         * they always did and only their representable RANGE changes. Pushed
         * unconditionally even at shift 0 (scale 1) so the pop in
         * ndsRendererEndParticleQuads is symmetric whether or not the batch
         * ever escalated, and so an escalation is a pop/push of a matrix that
         * is already on the stack rather than a special first case. */
        sNdsRendererParticleScaleShift = 0u;
        glPushMatrix();
        glEnable(GL_TEXTURE_2D);
        ndsRendererHardwareBindTextureName(NULL, atlas_name);
        sNdsRendererParticleQuadTexture = atlas_name;
        /* Translucent, unlit, both faces: a billboard has no meaningful
         * winding and the source draws these with the RDP's blender rather
         * than the lighting pipeline. */
        ndsRendererHardwareSetPolyFmt(
            POLY_ALPHA(poly_alpha) | POLY_CULL_NONE | POLY_ID(0));
        sNdsRendererParticleQuadAlpha = poly_alpha;
        glBegin(GL_QUAD);
        sNdsRendererParticleQuadOpen = TRUE;
    }
#endif

    rx = right->x * size;
    ry = right->y * size;
    rz = right->z * size;
    ux = up->x * size;
    uy = up->y * size;
    uz = up->z * size;

    /* The furthest corner this quad will reach, per axis, without evaluating
     * all four: |centre| + |right leg| + |up leg| bounds every combination of
     * the two signs. If it does not fit the batch's current factor, coarsen the
     * factor now -- before any of this quad's vertices are queued. */
    {
        f32 ex = NDS_FABS(pos->x) + NDS_FABS(rx) + NDS_FABS(ux);
        f32 ey = NDS_FABS(pos->y) + NDS_FABS(ry) + NDS_FABS(uy);
        f32 ez = NDS_FABS(pos->z) + NDS_FABS(rz) + NDS_FABS(uz);
        f32 extent = (ex > ey) ? ex : ey;
        u32 needed;

        if (ez > extent) { extent = ez; }
        needed = ndsRendererParticleScaleShiftFor(extent);
        if (needed > sNdsRendererParticleScaleShift)
        {
            /* Vertices already queued were transformed against the matrix that
             * was live when they were submitted, so swapping it now moves only
             * what follows. Pop back to the camera modelview, push the coarser
             * compensation, continue the same primitive group -- no glEnd, for
             * the reason ndsRendererEndParticleQuads states at length. */
            s32 factor = inttof32(1 << needed);

            glPopMatrix(1);
            glPushMatrix();
            glScalef32(factor, factor, factor);
            sNdsRendererParticleScaleShift = needed;
            gNdsParticleScaleEscalations++;
            if (needed > gNdsParticleScaleShiftMax)
            {
                gNdsParticleScaleShiftMax = needed;
            }
        }
    }
    glColor((rgb)color);

    mirror_mask &= 3u;
    if (mirror_mask == 0u)
    {
        u32 corner;

        /* Counter-clockwise from the bottom-left, with T increasing downward
         * to match the atlas rows. */
        for (corner = 0u; corner < 4u; corner++)
        {
            f32 sx = ((corner == 0u) || (corner == 3u)) ? -1.0f : 1.0f;
            f32 sy = (corner < 2u) ? -1.0f : 1.0f;
            u32 texel_s = atlas_x + (((corner == 0u) || (corner == 3u))
                                         ? 0u : atlas_w);
            u32 texel_t = atlas_y + ((corner < 2u) ? atlas_h : 0u);
            u32 shift = sNdsRendererParticleScaleShift;

            glTexCoord2t16((t16)(texel_s << 4), (t16)(texel_t << 4));
            glVertex3v16(
                ndsRendererParticleWorldToV16(pos->x + (rx * sx) + (ux * sy),
                                              shift),
                ndsRendererParticleWorldToV16(pos->y + (ry * sx) + (uy * sy),
                                              shift),
                ndsRendererParticleWorldToV16(pos->z + (rz * sx) + (uz * sy),
                                              shift));
        }
    }
    else
    {
        /* N64 lbParticle does NOT stretch a MASKS/MASKT source fragment over
         * the rectangle. It doubles dsdx/dtdy and sets G_TX_MIRROR, making a
         * triangle wave across the SAME rectangle:
         *
         *   S:  left 0 -> centre W -> right 0
         *   T:  top  0 -> centre H -> bottom 0
         *
         * The DS can mirror a whole texture name, but this name is an atlas;
         * hardware wrap would cross the cell boundary into another effect.
         * Build the 3x3 world grid once using only +/- adds (the expensive
         * right/up * size products above remain once per particle), then emit
         * 2 or 4 atlas-cell quads with the exact triangle-wave UVs. */
        f32 right_x[3] = { -rx, 0.0F, rx };
        f32 right_y[3] = { -ry, 0.0F, ry };
        f32 right_z[3] = { -rz, 0.0F, rz };
        f32 up_x[3] = { -ux, 0.0F, ux };
        f32 up_y[3] = { -uy, 0.0F, uy };
        f32 up_z[3] = { -uz, 0.0F, uz };
        v16 grid_x[3][3];
        v16 grid_y[3][3];
        v16 grid_z[3][3];
        u32 s_edge[3];
        u32 t_edge[3];
        u32 s_uv_q4[3];
        u32 t_uv_q4[3];
        u32 s_parts;
        u32 t_parts;
        u32 row;
        u32 column;
        u32 shift = sNdsRendererParticleScaleShift;

        for (row = 0u; row < 3u; row++)
        {
            for (column = 0u; column < 3u; column++)
            {
                grid_x[row][column] = ndsRendererParticleWorldToV16(
                    pos->x + right_x[column] + up_x[row], shift);
                grid_y[row][column] = ndsRendererParticleWorldToV16(
                    pos->y + right_y[column] + up_y[row], shift);
                grid_z[row][column] = ndsRendererParticleWorldToV16(
                    pos->z + right_z[column] + up_z[row], shift);
            }
        }

        if ((mirror_mask & 1u) != 0u)
        {
            s_parts = 2u;
            s_edge[0] = 0u; s_edge[1] = 1u; s_edge[2] = 2u;
            s_uv_q4[0] = atlas_x << 4;
            /* Atlas cells have no padding. The N64 mirror fold duplicates the
             * source's last texel; sampling exactly at atlas_x+atlas_w could
             * name the NEXT cell at the fold. Stay one 1/16-texel unit inside
             * the source cell so both halves meet on its own edge texel. */
            s_uv_q4[1] = ((atlas_x + atlas_w) << 4) - 1u;
            s_uv_q4[2] = atlas_x << 4;
        }
        else
        {
            s_parts = 1u;
            s_edge[0] = 0u; s_edge[1] = 2u;
            s_uv_q4[0] = atlas_x << 4;
            s_uv_q4[1] = (atlas_x + atlas_w) << 4;
        }
        if ((mirror_mask & 2u) != 0u)
        {
            t_parts = 2u;
            t_edge[0] = 0u; t_edge[1] = 1u; t_edge[2] = 2u;
            /* Grid row 0 is the BOTTOM. Source T increases downward: bottom
             * and top are both 0 under mirror, and the centre is H. */
            t_uv_q4[0] = atlas_y << 4;
            t_uv_q4[1] = ((atlas_y + atlas_h) << 4) - 1u;
            t_uv_q4[2] = atlas_y << 4;
        }
        else
        {
            t_parts = 1u;
            t_edge[0] = 0u; t_edge[1] = 2u;
            t_uv_q4[0] = (atlas_y + atlas_h) << 4;
            t_uv_q4[1] = atlas_y << 4;
        }

        for (row = 0u; row < t_parts; row++)
        {
            for (column = 0u; column < s_parts; column++)
            {
                u32 s0 = s_edge[column];
                u32 s1 = s_edge[column + 1u];
                u32 t0 = t_edge[row];
                u32 t1 = t_edge[row + 1u];

                glTexCoord2t16((t16)s_uv_q4[column],
                               (t16)t_uv_q4[row]);
                glVertex3v16(grid_x[t0][s0], grid_y[t0][s0], grid_z[t0][s0]);
                glTexCoord2t16((t16)s_uv_q4[column + 1u],
                               (t16)t_uv_q4[row]);
                glVertex3v16(grid_x[t0][s1], grid_y[t0][s1], grid_z[t0][s1]);
                glTexCoord2t16((t16)s_uv_q4[column + 1u],
                               (t16)t_uv_q4[row + 1u]);
                glVertex3v16(grid_x[t1][s1], grid_y[t1][s1], grid_z[t1][s1]);
                glTexCoord2t16((t16)s_uv_q4[column],
                               (t16)t_uv_q4[row + 1u]);
                glVertex3v16(grid_x[t1][s0], grid_y[t1][s0], grid_z[t1][s0]);
            }
        }
    }
    return TRUE;
}

#if NDS_R2_WHISPY_NATIVE_AOT
#define NDS_RENDERER_WHISPY_BASIS_SHIFT 14u
#define NDS_RENDERER_WHISPY_SIZE_SHIFT 8u
#define NDS_RENDERER_WHISPY_COORD_SHIFT 12u
#define NDS_RENDERER_WHISPY_LEG_SHIFT \
    (NDS_RENDERER_WHISPY_BASIS_SHIFT + NDS_RENDERER_WHISPY_SIZE_SHIFT - \
     NDS_RENDERER_WHISPY_COORD_SHIFT)

static s32 sNdsRendererWhispyRight[3];
static s32 sNdsRendererWhispyUp[3];
static u32 sNdsRendererWhispyBasisValid;
static u32 sNdsRendererWhispyLeanBindingMask;
static u32 sNdsRendererWhispyLegCacheValid;
static s32 sNdsRendererWhispyLegCacheSizeQ8;
static u32 sNdsRendererWhispyLegCacheMirrorMask;
static s32 sNdsRendererWhispyLegCacheRight[3];
static s32 sNdsRendererWhispyLegCacheUp[3];

/* Same-order command offload.  Forty-eight is the source particle-pool cap;
 * 1,024 words covers its worst case (16 words per quad plus every possible
 * texture/alpha transition) with room to spare.  Overflow still has an
 * intentional policy: publish the current packet, then continue in the empty
 * buffer.  No source list pointer or particle lifetime is ever rewritten. */
#define NDS_RENDERER_WHISPY_PACKET_WORDS 1024u
typedef struct NDSRendererWhispyPacket
{
    u32 words[NDS_RENDERER_WHISPY_PACKET_WORDS];
    u32 word_count;
    u32 final_texture_slot;
    u32 final_poly_alpha;
    u32 lean_counters;
    u32 lean_quads;
    u32 lean_state_groups;
    u32 lean_sheet_breaks;
    u32 lean_alpha_breaks;
} NDSRendererWhispyPacket;

static NDSRendererWhispyPacket sNdsRendererWhispyPacket
    __attribute__((aligned(32)));
volatile u32 gNdsWhispyAOTTier3FastBinds;
volatile u32 gNdsWhispyAOTTier3BindFallbacks;
volatile u32 gNdsWhispyAOTTier4PacketQuads;
volatile u32 gNdsWhispyAOTTier4PacketStateGroups;
volatile u32 gNdsWhispyAOTTier4PacketFlushes;
volatile u32 gNdsWhispyAOTTier4PacketWords;
volatile u32 gNdsWhispyAOTTier4PacketFallbacks;

static const NDSRendererWhispyNativeBinding *
ndsRendererWhispyNativeBindingFor(u32 texture_name, u32 texture_slot)
{
    const NDSRendererWhispyNativeBinding *binding;

    if (texture_slot >=
        (NDS_WHISPY_NATIVE_TEXTURE_COUNT + NDS_R2_FOX_BLASTER_GLOW_AOT))
    {
        return NULL;
    }
    binding = &sNdsRendererWhispyNativeBinding[texture_slot];
    if ((binding->valid == FALSE) ||
        (binding->texture_name == 0u) ||
        (binding->texture_name != texture_name))
    {
        return NULL;
    }
    return binding;
}

/* Exact glBindTexture state without libnds's DynamicArray lookup.  The cached
 * words and active palette/name were captured while this same immutable GL
 * object was bound at scene preparation, so both libnds's software state and
 * the DS registers advance together. */
static NDS_RENDERER_FAST_RUN_CODE sb32
ndsRendererBindWhispyNativeTextureFast(u32 texture_name, u32 texture_slot)
{
    const NDSRendererWhispyNativeBinding *binding =
        ndsRendererWhispyNativeBindingFor(texture_name, texture_slot);

    if (binding == NULL)
    {
        gNdsWhispyAOTTier3BindFallbacks++;
        ndsRendererHardwareBindTextureName(NULL, texture_name);
        return FALSE;
    }
    if (sNdsRendererHardwareBoundTextureName != texture_name)
    {
        ndsRendererHardwareEndBatch();
        GFX_TEX_FORMAT = binding->texture_format;
        GFX_PAL_FORMAT = binding->palette_format;
        glGlob->activeTexture = (int)texture_name;
        glGlob->activePalette = (int)binding->palette_name;
        sNdsRendererHardwareBoundTextureName = texture_name;
        ndsRendererHardwareInvalidateGXState(
            NDS_RENDERER_GX_STATE_TEXTURE_PARAMS);
        ndsRendererProfileRecordTextureBind();
        gNdsWhispyAOTTier3FastBinds++;
    }
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;
}

static NDS_RENDERER_FAST_RUN_CODE void
ndsRendererFlushWhispyNativePacket(void)
{
    NDSRendererWhispyPacket *packet = &sNdsRendererWhispyPacket;
    const NDSRendererWhispyNativeBinding *binding;
    u32 word_count = packet->word_count;

    if (word_count == 0u)
    {
        return;
    }
    DC_FlushRange(packet->words, word_count * sizeof(u32));
    while ((DMA_CR(0) & DMA_BUSY) != 0u) { }
    DMA_SRC(0) = (u32)packet->words;
    DMA_DEST(0) = (u32)&GFX_FIFO;
    DMA_CR(0) = DMA_FIFO | word_count;
    while ((DMA_CR(0) & DMA_BUSY) != 0u) { }

    binding = ndsRendererWhispyNativeBindingFor(
        sNdsRendererParticleQuadTexture, packet->final_texture_slot);
    if (binding != NULL)
    {
        glGlob->activeTexture = (int)binding->texture_name;
        glGlob->activePalette = (int)binding->palette_name;
        sNdsRendererHardwareBoundTextureName = binding->texture_name;
    }
    else
    {
        /* This cannot occur after admission, but invalidating both software
         * trackers makes a later ordinary bind re-establish real hardware
         * state instead of inheriting a packet whose identity was lost. */
        glGlob->activeTexture = 0;
        glGlob->activePalette = 0;
        sNdsRendererHardwareBoundTextureName = 0u;
        gNdsWhispyAOTTier4PacketFallbacks++;
    }
    ndsRendererHardwareInvalidateGXState(
        NDS_RENDERER_GX_STATE_TEXTURE_PARAMS);
    sNdsRendererGXStateShadow.poly_fmt =
        POLY_ALPHA(packet->final_poly_alpha) |
        POLY_CULL_NONE | POLY_ID(0);
    sNdsRendererGXStateShadow.valid_mask |= NDS_RENDERER_GX_STATE_POLY_FMT;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    if (packet->lean_counters != FALSE)
    {
        gNdsWhispyAOTTier4PacketQuads += packet->lean_quads;
        gNdsWhispyAOTTier4PacketStateGroups += packet->lean_state_groups;
        gNdsParticleQuadSheetBreaks += packet->lean_sheet_breaks;
        gNdsParticleQuadAlphaBreaks += packet->lean_alpha_breaks;
    }
    gNdsWhispyAOTTier4PacketFlushes++;
    gNdsWhispyAOTTier4PacketWords += word_count;
    packet->word_count = 0u;
    packet->lean_quads = 0u;
    packet->lean_state_groups = 0u;
    packet->lean_sheet_breaks = 0u;
    packet->lean_alpha_breaks = 0u;
}

static u32 *ndsRendererWhispyPacketReserve(u32 words)
{
    NDSRendererWhispyPacket *packet = &sNdsRendererWhispyPacket;
    u32 *out;

    if (words > NDS_RENDERER_WHISPY_PACKET_WORDS)
    {
        gNdsWhispyAOTTier4PacketFallbacks++;
        return NULL;
    }
    if (packet->word_count + words > NDS_RENDERER_WHISPY_PACKET_WORDS)
    {
        ndsRendererFlushWhispyNativePacket();
    }
    out = &packet->words[packet->word_count];
    packet->word_count += words;
    return out;
}

/* Append only the state that actually changed, followed by the glBegin that
 * latches it.  The source particle order is untouched: this is the same
 * command sequence the immediate path emits, merely packed in cached RAM. */
static inline __attribute__((always_inline)) sb32
ndsRendererAppendWhispyPacketState(u32 texture_name, u32 texture_slot,
                                   u32 poly_alpha, u32 submit_route)
{
    const NDSRendererWhispyNativeBinding *binding =
        (submit_route >= 7u) ?
        &sNdsRendererWhispyNativeBinding[texture_slot] :
        ndsRendererWhispyNativeBindingFor(texture_name, texture_slot);
    u32 texture_changed =
        (texture_name != sNdsRendererParticleQuadTexture) ? TRUE : FALSE;
    u32 alpha_changed =
        (poly_alpha != sNdsRendererParticleQuadAlpha) ? TRUE : FALSE;
    u32 *out;

    if (binding == NULL)
    {
        gNdsWhispyAOTTier4PacketFallbacks++;
        return FALSE;
    }
    if ((texture_changed == FALSE) && (alpha_changed == FALSE))
    {
        return TRUE;
    }
    if ((texture_changed != FALSE) && (alpha_changed != FALSE))
    {
        out = ndsRendererWhispyPacketReserve(5u);
        if (out == NULL) { return FALSE; }
        out[0] = FIFO_COMMAND_PACK(
            FIFO_TEX_FORMAT, FIFO_PAL_FORMAT,
            FIFO_POLY_FORMAT, FIFO_BEGIN);
        out[1] = binding->texture_format;
        out[2] = binding->palette_format;
        out[3] = POLY_ALPHA(poly_alpha) | POLY_CULL_NONE | POLY_ID(0);
        out[4] = GL_QUAD;
    }
    else if (texture_changed != FALSE)
    {
        out = ndsRendererWhispyPacketReserve(4u);
        if (out == NULL) { return FALSE; }
        out[0] = FIFO_COMMAND_PACK(
            FIFO_TEX_FORMAT, FIFO_PAL_FORMAT, FIFO_BEGIN, FIFO_NOP);
        out[1] = binding->texture_format;
        out[2] = binding->palette_format;
        out[3] = GL_QUAD;
    }
    else
    {
        out = ndsRendererWhispyPacketReserve(3u);
        if (out == NULL) { return FALSE; }
        out[0] = FIFO_COMMAND_PACK(
            FIFO_POLY_FORMAT, FIFO_BEGIN, FIFO_NOP, FIFO_NOP);
        out[1] = POLY_ALPHA(poly_alpha) | POLY_CULL_NONE | POLY_ID(0);
        out[2] = GL_QUAD;
    }

    if (texture_changed != FALSE)
    {
        sNdsRendererParticleQuadTexture = texture_name;
        if (sNdsRendererWhispyPacket.lean_counters != FALSE)
        {
            sNdsRendererWhispyPacket.lean_sheet_breaks++;
        }
        else
        {
            gNdsParticleQuadSheetBreaks++;
        }
        ndsRendererProfileRecordTextureBind();
    }
    if (alpha_changed != FALSE)
    {
        sNdsRendererParticleQuadAlpha = poly_alpha;
        if (sNdsRendererWhispyPacket.lean_counters != FALSE)
        {
            sNdsRendererWhispyPacket.lean_alpha_breaks++;
        }
        else
        {
            gNdsParticleQuadAlphaBreaks++;
        }
    }
    sNdsRendererWhispyPacket.final_texture_slot = texture_slot;
    sNdsRendererWhispyPacket.final_poly_alpha = poly_alpha;
    if (sNdsRendererWhispyPacket.lean_counters != FALSE)
    {
        sNdsRendererWhispyPacket.lean_state_groups++;
    }
    else
    {
        gNdsWhispyAOTTier4PacketStateGroups++;
    }
    return TRUE;
}

/* COLOR + four (TEXCOORD, VERTEX16) pairs.  There are nine commands and
 * thirteen parameter words, packed into exactly sixteen FIFO words. */
static inline __attribute__((always_inline)) sb32
ndsRendererAppendWhispyPacketQuad(u32 color,
                                  const v16 vertex[4][3],
                                  u32 texture_w, u32 texture_h,
                                  u32 texture_slot, u32 poly_alpha)
{
    u32 *out = ndsRendererWhispyPacketReserve(16u);

    if (out == NULL)
    {
        return FALSE;
    }
    out[0] = FIFO_COMMAND_PACK(
        FIFO_COLOR, FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD);
    out[1] = color;
    out[2] = TEXTURE_PACK((t16)0, (t16)(texture_h << 4));
    out[3] = (u32)(u16)vertex[0][0] |
             ((u32)(u16)vertex[0][1] << 16);
    out[4] = (u32)(s32)vertex[0][2];
    out[5] = TEXTURE_PACK((t16)(texture_w << 4),
                          (t16)(texture_h << 4));

    out[6] = FIFO_COMMAND_PACK(
        FIFO_VERTEX16, FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD);
    out[7] = (u32)(u16)vertex[1][0] |
             ((u32)(u16)vertex[1][1] << 16);
    out[8] = (u32)(s32)vertex[1][2];
    out[9] = TEXTURE_PACK((t16)(texture_w << 4), (t16)0);
    out[10] = (u32)(u16)vertex[2][0] |
              ((u32)(u16)vertex[2][1] << 16);
    out[11] = (u32)(s32)vertex[2][2];
    out[12] = TEXTURE_PACK((t16)0, (t16)0);

    out[13] = FIFO_COMMAND_PACK(
        FIFO_VERTEX16, FIFO_NOP, FIFO_NOP, FIFO_NOP);
    out[14] = (u32)(u16)vertex[3][0] |
              ((u32)(u16)vertex[3][1] << 16);
    out[15] = (u32)(s32)vertex[3][2];

    sNdsRendererWhispyPacket.final_texture_slot = texture_slot;
    sNdsRendererWhispyPacket.final_poly_alpha = poly_alpha;
    if (sNdsRendererWhispyPacket.lean_counters != FALSE)
    {
        sNdsRendererWhispyPacket.lean_quads++;
    }
    else
    {
        gNdsWhispyAOTTier4PacketQuads++;
    }
    return TRUE;
}

/* Keep a scale escalation in the same ordered FIFO stream as the vertices it
 * separates.  Flushing before every escalation made route 4 wait on DMA 32
 * extra times in the focused Whispy window even though GX can consume the
 * pop/push/scale commands in-order itself. */
static NDS_RENDERER_FAST_RUN_CODE sb32
ndsRendererAppendWhispyPacketScale(u32 scale_shift)
{
    u32 *out = ndsRendererWhispyPacketReserve(5u);
    s32 factor;

    if (out == NULL)
    {
        return FALSE;
    }
    factor = inttof32(1 << scale_shift);
    out[0] = FIFO_COMMAND_PACK(
        REG2ID(MATRIX_POP), REG2ID(MATRIX_PUSH),
        REG2ID(MATRIX_SCALE), FIFO_NOP);
    out[1] = 1u;
    out[2] = (u32)factor;
    out[3] = (u32)factor;
    out[4] = (u32)factor;
    return TRUE;
}

/* When the last commands in the pass are packeted Whispy quads, put the
 * balancing matrix pop behind those vertices in that same stream.  Returning
 * FALSE tells the ordinary end path that an immediate/generic draw followed
 * the packet and therefore still owns the pop. */
static NDS_RENDERER_FAST_RUN_CODE sb32
ndsRendererFinishWhispyNativePacket(void)
{
    u32 *out;

    if (sNdsRendererWhispyPacket.word_count == 0u)
    {
        return FALSE;
    }
    out = ndsRendererWhispyPacketReserve(2u);
    if (out == NULL)
    {
        return FALSE;
    }
    out[0] = FIFO_COMMAND_PACK(
        REG2ID(MATRIX_POP), FIFO_NOP, FIFO_NOP, FIFO_NOP);
    out[1] = 1u;
    ndsRendererFlushWhispyNativePacket();
    return TRUE;
}

/* ARM946E-S has no FPU. GCC lowers even `value * 4096.0F` followed by an int
 * cast to two libgcc calls, so the first fixed submit still paid eight soft-
 * float calls per quad. Decode the finite IEEE-754 value directly and shift
 * its mantissa into the requested binary fixed domain. The result is the same
 * truncation toward zero as a C cast; failure is the generic-fallback seam. */
static sb32 ndsRendererWhispyFloatToFixed(f32 value,
                                          u32 fraction_bits,
                                          s32 *fixed)
{
    union
    {
        f32 f;
        u32 u;
    } bits;
    u32 exponent;
    u32 mantissa;
    u32 magnitude;
    u32 limit;
    u32 sign;
    s32 shift;

    bits.f = value;
    sign = bits.u >> 31;
    exponent = (bits.u >> 23) & 0xFFu;
    mantissa = bits.u & 0x7FFFFFu;
    if ((fixed == NULL) || (exponent == 0xFFu))
    {
        return FALSE;
    }
    if (exponent == 0u)
    {
        if (mantissa == 0u)
        {
            *fixed = 0;
            return TRUE;
        }
        shift = -126 - 23 + (s32)fraction_bits;
    }
    else
    {
        mantissa |= 0x800000u;
        shift = (s32)exponent - 127 - 23 + (s32)fraction_bits;
    }
    limit = (sign != 0u) ? 0x80000000u : 0x7FFFFFFFu;
    if (shift >= 0)
    {
        if ((shift >= 32) ||
            (mantissa > (limit >> (u32)shift)))
        {
            return FALSE;
        }
        magnitude = mantissa << (u32)shift;
    }
    else if (shift <= -32)
    {
        magnitude = 0u;
    }
    else
    {
        magnitude = mantissa >> (u32)-shift;
    }
    if (sign != 0u)
    {
        *fixed = (magnitude == 0x80000000u) ? INT_MIN : -(s32)magnitude;
    }
    else
    {
        *fixed = (s32)magnitude;
    }
    return TRUE;
}

s32 ndsRendererParticlePositionToQ12(const Vec3f *pos,
                                     s32 fixed_center_q12[3])
{
    if ((pos == NULL) || (fixed_center_q12 == NULL) ||
        (ndsRendererWhispyFloatToFixed(
             pos->x, NDS_RENDERER_WHISPY_COORD_SHIFT,
             &fixed_center_q12[0]) == FALSE) ||
        (ndsRendererWhispyFloatToFixed(
             pos->y, NDS_RENDERER_WHISPY_COORD_SHIFT,
             &fixed_center_q12[1]) == FALSE) ||
        (ndsRendererWhispyFloatToFixed(
             pos->z, NDS_RENDERER_WHISPY_COORD_SHIFT,
             &fixed_center_q12[2]) == FALSE))
    {
        return FALSE;
    }
    return TRUE;
}

static sb32 ndsRendererWhispyBasisComponent(f32 value, s32 *fixed)
{
    if ((ndsRendererWhispyFloatToFixed(
             value, NDS_RENDERER_WHISPY_BASIS_SHIFT, fixed) == FALSE) ||
        (*fixed < -(s32)(1u << NDS_RENDERER_WHISPY_BASIS_SHIFT)) ||
        (*fixed > (s32)(1u << NDS_RENDERER_WHISPY_BASIS_SHIFT)))
    {
        return FALSE;
    }
    return TRUE;
}

void ndsRendererSetWhispyNativeBasis(const Vec3f *right, const Vec3f *up)
{
    sNdsRendererWhispyBasisValid = FALSE;
    sNdsRendererWhispyLeanBindingMask = 0u;
    sNdsRendererWhispyLegCacheValid = FALSE;
    if ((right == NULL) || (up == NULL) ||
        (ndsRendererWhispyBasisComponent(
             right->x, &sNdsRendererWhispyRight[0]) == FALSE) ||
        (ndsRendererWhispyBasisComponent(
             right->y, &sNdsRendererWhispyRight[1]) == FALSE) ||
        (ndsRendererWhispyBasisComponent(
             right->z, &sNdsRendererWhispyRight[2]) == FALSE) ||
        (ndsRendererWhispyBasisComponent(
             up->x, &sNdsRendererWhispyUp[0]) == FALSE) ||
        (ndsRendererWhispyBasisComponent(
             up->y, &sNdsRendererWhispyUp[1]) == FALSE) ||
        (ndsRendererWhispyBasisComponent(
             up->z, &sNdsRendererWhispyUp[2]) == FALSE))
    {
        return;
    }
    sNdsRendererWhispyBasisValid = TRUE;
}

/* Mirror ndsRendererSubmitParticleQuad's state transitions without entering
 * its float corner builder. Keeping these exact transitions preserves source
 * draw order, per-particle alpha, texture changes, matrix ownership, and the
 * no-glEnd FIFO contract. */
static NDS_RENDERER_FAST_RUN_CODE void
ndsRendererPrepareWhispyQuadState(u32 texture_name, u32 poly_alpha,
                                  u32 texture_slot, u32 submit_route)
{
    if ((sNdsRendererParticleQuadOpen != 0u) &&
        ((poly_alpha != sNdsRendererParticleQuadAlpha) ||
         (texture_name != sNdsRendererParticleQuadTexture)))
    {
        if (texture_name != sNdsRendererParticleQuadTexture)
        {
            if (submit_route >= 3u)
            {
                (void)ndsRendererBindWhispyNativeTextureFast(
                    texture_name, texture_slot);
            }
            else
            {
                ndsRendererHardwareBindTextureName(NULL, texture_name);
            }
            sNdsRendererParticleQuadTexture = texture_name;
            gNdsParticleQuadSheetBreaks++;
        }
        ndsRendererHardwareSetPolyFmt(
            POLY_ALPHA(poly_alpha) | POLY_CULL_NONE | POLY_ID(0));
        if (poly_alpha != sNdsRendererParticleQuadAlpha)
        {
            gNdsParticleQuadAlphaBreaks++;
        }
        sNdsRendererParticleQuadAlpha = poly_alpha;
        glBegin(GL_QUAD);
    }
    if (sNdsRendererParticleQuadOpen == 0u)
    {
        gNdsParticleMatrixModeSeen |=
            1u << (sNdsRendererHardwareMatrixMode & 7u);
        gNdsParticleMatrixLoadedSeen = sNdsRendererHardwareMatrixLoaded;
        gNdsParticleBatchOpens++;
        ndsRendererHardwareEndBatch();
        if (sNdsRendererParticleCameraValid != FALSE)
        {
            NDSRendererMatrix20p12 scaled_modelview;
            m4x4 projection_hw;
            m4x4 modelview_hw;
            u32 col;

            ndsRendererMatrixCopy20p12(
                &scaled_modelview, &sNdsRendererParticleModelview);
            for (col = 0u; col < 4u; col++)
            {
                scaled_modelview.m[3][col] =
                    ndsRendererRoundShiftS32Signed(
                        scaled_modelview.m[3][col],
                        NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
            }
            ndsRendererCopyMtx20p12ToM4x4(
                &sNdsRendererParticleProjection, &projection_hw);
            ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
            glLoadMatrix4x4(&projection_hw);
            ndsRendererCopyMtx20p12ToM4x4(
                &scaled_modelview, &modelview_hw);
            ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
            glLoadMatrix4x4(&modelview_hw);
            sNdsRendererHardwareMatrixLoaded = FALSE;
            sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
            gNdsParticleCameraLoads++;
            gNdsParticleCamTx = scaled_modelview.m[3][0];
            gNdsParticleCamTy = scaled_modelview.m[3][1];
            gNdsParticleCamTz = scaled_modelview.m[3][2];
        }
        sNdsRendererParticleScaleShift = 0u;
        glPushMatrix();
        glEnable(GL_TEXTURE_2D);
        if (submit_route >= 3u)
        {
            (void)ndsRendererBindWhispyNativeTextureFast(
                texture_name, texture_slot);
        }
        else
        {
            ndsRendererHardwareBindTextureName(NULL, texture_name);
        }
        sNdsRendererParticleQuadTexture = texture_name;
        ndsRendererHardwareSetPolyFmt(
            POLY_ALPHA(poly_alpha) | POLY_CULL_NONE | POLY_ID(0));
        sNdsRendererParticleQuadAlpha = poly_alpha;
        glBegin(GL_QUAD);
        sNdsRendererParticleQuadOpen = TRUE;
    }
}

static s32 ndsRendererWhispyAbsS32(s32 value)
{
    return (value < 0) ? -value : value;
}

static s32 ndsRendererWhispyTruncShiftS32(s32 value, u32 shift)
{
    if (value < 0)
    {
        return -(s32)(((u32)-value) >> shift);
    }
    return (s32)((u32)value >> shift);
}

static v16 ndsRendererWhispyCoordToV16(s32 coord_q12, u32 scale_shift)
{
    s32 value = ndsRendererWhispyTruncShiftS32(
        coord_q12,
        (NDS_RENDERER_WHISPY_COORD_SHIFT - 4u) + scale_shift);

    if (value > 32767)
    {
        gNdsParticleWorldClampCount++;
        return (v16)32767;
    }
    if (value < -32768)
    {
        gNdsParticleWorldClampCount++;
        return (v16)-32768;
    }
    return (v16)value;
}

static inline __attribute__((always_inline)) v16
ndsRendererWhispyCoordToV16Unchecked(s32 coord_q12, u32 scale_shift)
{
    return (v16)ndsRendererWhispyTruncShiftS32(
        coord_q12,
        (NDS_RENDERER_WHISPY_COORD_SHIFT - 4u) + scale_shift);
}

NDS_RENDERER_FAST_RUN_CODE
s32 ndsRendererSubmitWhispyNativeQuad(u32 texture_name,
                                      u32 texture_slot,
                                      const Vec3f *pos, f32 size,
                                      const s32 fixed_center_q12[3],
                                      s32 fixed_size_q8,
                                      u32 color, u8 alpha,
                                      u32 mirror_mask,
                                      u32 texture_w, u32 texture_h,
                                      u32 submit_route)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    s32 center[3];
    s32 right[3];
    s32 up[3];
    v16 packed_vertex[4][3];
    s32 extent;
    s32 size_q8;
    u32 axis;
    u32 corner;
    u32 needed = 0u;
    u32 coordinates_safe = FALSE;
    u32 poly_alpha;

    /* These bounds make every 32-bit basis product total. Whispy's source
     * sizes top out at 350; an altered script or camera uses the generic submit
     * instead of wrapping this closed representation. */
    if ((sNdsRendererWhispyBasisValid == FALSE) ||
        (texture_name == 0u) ||
        ((fixed_center_q12 == NULL) && (pos == NULL)))
    {
        return -1;
    }
    if (alpha == 0u)
    {
        return 0;
    }
    poly_alpha = ((u32)alpha >> 3) & 31u;
    if (poly_alpha == 0u) { poly_alpha = 1u; }
    if (submit_route >= 7u)
    {
        u32 binding_bit;

        if (texture_slot >= NDS_WHISPY_NATIVE_TEXTURE_COUNT)
        {
            gNdsWhispyAOTTier4PacketFallbacks++;
            return -1;
        }
        binding_bit = 1u << texture_slot;
        if (((sNdsRendererWhispyLeanBindingMask & binding_bit) == 0u) &&
            (ndsRendererWhispyNativeBindingFor(
                 texture_name, texture_slot) == NULL))
        {
            gNdsWhispyAOTTier4PacketFallbacks++;
            return -1;
        }
        sNdsRendererWhispyLeanBindingMask |= binding_bit;
    }
    else if ((submit_route >= 3u) &&
             (ndsRendererWhispyNativeBindingFor(
                  texture_name, texture_slot) == NULL))
    {
        if (submit_route >= 4u)
        {
            gNdsWhispyAOTTier4PacketFallbacks++;
        }
        return -1;
    }
    if (submit_route < 4u)
    {
        /* Supports debugger route changes without allowing an older packet to
         * cross the first immediate-mode quad. Normal runs never take this
         * branch with a non-empty packet. */
        ndsRendererFlushWhispyNativePacket();
    }
    else if (sNdsRendererWhispyPacket.word_count == 0u)
    {
        sNdsRendererWhispyPacket.lean_counters =
            (submit_route >= 6u) ? TRUE : FALSE;
    }

    if (fixed_center_q12 != NULL)
    {
        center[0] = fixed_center_q12[0];
        center[1] = fixed_center_q12[1];
        center[2] = fixed_center_q12[2];
        size_q8 = fixed_size_q8;
    }
    else if ((ndsRendererWhispyFloatToFixed(
                  pos->x, NDS_RENDERER_WHISPY_COORD_SHIFT,
                  &center[0]) == FALSE) ||
             (ndsRendererWhispyFloatToFixed(
                  pos->y, NDS_RENDERER_WHISPY_COORD_SHIFT,
                  &center[1]) == FALSE) ||
             (ndsRendererWhispyFloatToFixed(
                  pos->z, NDS_RENDERER_WHISPY_COORD_SHIFT,
                  &center[2]) == FALSE) ||
             (ndsRendererWhispyFloatToFixed(
                  size, NDS_RENDERER_WHISPY_SIZE_SHIFT,
                  &size_q8) == FALSE))
    {
        return -1;
    }
    if ((size_q8 <= 0) ||
        (size_q8 >= (s32)(512u << NDS_RENDERER_WHISPY_SIZE_SHIFT)) ||
        (center[0] < -(s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)) ||
        (center[0] >  (s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)) ||
        (center[1] < -(s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)) ||
        (center[1] >  (s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)) ||
        (center[2] < -(s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)) ||
        (center[2] >  (s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)))
    {
        return -1;
    }
    if ((submit_route >= 7u) &&
        (sNdsRendererWhispyLegCacheValid != FALSE) &&
        (sNdsRendererWhispyLegCacheSizeQ8 == size_q8) &&
        (sNdsRendererWhispyLegCacheMirrorMask == (mirror_mask & 3u)))
    {
        for (axis = 0u; axis < 3u; axis++)
        {
            right[axis] = sNdsRendererWhispyLegCacheRight[axis];
            up[axis] = sNdsRendererWhispyLegCacheUp[axis];
        }
    }
    else
    {
        s32 leg_size_q8 = size_q8;

        if ((mirror_mask & 1u) != 0u) { leg_size_q8 = -leg_size_q8; }
        for (axis = 0u; axis < 3u; axis++)
        {
            right[axis] = (sNdsRendererWhispyRight[axis] * leg_size_q8) /
                          (s32)(1u << NDS_RENDERER_WHISPY_LEG_SHIFT);
        }
        if ((mirror_mask & 1u) != 0u) { leg_size_q8 = -leg_size_q8; }
        if ((mirror_mask & 2u) != 0u) { leg_size_q8 = -leg_size_q8; }
        for (axis = 0u; axis < 3u; axis++)
        {
            up[axis] = (sNdsRendererWhispyUp[axis] * leg_size_q8) /
                       (s32)(1u << NDS_RENDERER_WHISPY_LEG_SHIFT);
        }
        if (submit_route >= 7u)
        {
            sNdsRendererWhispyLegCacheSizeQ8 = size_q8;
            sNdsRendererWhispyLegCacheMirrorMask = mirror_mask & 3u;
            for (axis = 0u; axis < 3u; axis++)
            {
                sNdsRendererWhispyLegCacheRight[axis] = right[axis];
                sNdsRendererWhispyLegCacheUp[axis] = up[axis];
            }
            sNdsRendererWhispyLegCacheValid = TRUE;
        }
    }

    extent = ndsRendererWhispyAbsS32(center[0]) +
             ndsRendererWhispyAbsS32(right[0]) +
             ndsRendererWhispyAbsS32(up[0]);
    for (axis = 1u; axis < 3u; axis++)
    {
        s32 candidate = ndsRendererWhispyAbsS32(center[axis]) +
                        ndsRendererWhispyAbsS32(right[axis]) +
                        ndsRendererWhispyAbsS32(up[axis]);
        if (candidate > extent) { extent = candidate; }
    }
    if (submit_route >= 7u)
    {
        needed = sNdsRendererParticleScaleShift;
    }
    while ((needed < NDS_RENDERER_PARTICLE_MAX_SCALE_SHIFT) &&
           ((extent / (s32)(1u <<
               ((NDS_RENDERER_WHISPY_COORD_SHIFT - 4u) + needed))) > 32767))
    {
        needed++;
    }
    if (submit_route >= 7u)
    {
        coordinates_safe =
            ((extent / (s32)(1u <<
                ((NDS_RENDERER_WHISPY_COORD_SHIFT - 4u) + needed))) <=
             32767) ? TRUE : FALSE;
    }

    if (submit_route >= 4u)
    {
        if (sNdsRendererParticleQuadOpen == 0u)
        {
            /* Establish the camera/matrix stack once. The first material bind
             * uses route 3's exact direct-register transition; every later
             * state/quad command in this contiguous run goes through DMA. */
            ndsRendererPrepareWhispyQuadState(
                texture_name, poly_alpha, texture_slot, 3u);
        }
    }
    else
    {
        ndsRendererPrepareWhispyQuadState(
            texture_name, poly_alpha, texture_slot, submit_route);
    }
    if (needed > sNdsRendererParticleScaleShift)
    {
        if (submit_route >= 4u)
        {
            if (ndsRendererAppendWhispyPacketScale(needed) == FALSE)
            {
                return -1;
            }
        }
        else
        {
            s32 factor = inttof32(1 << needed);

            glPopMatrix(1);
            glPushMatrix();
            glScalef32(factor, factor, factor);
        }
        sNdsRendererParticleScaleShift = needed;
        gNdsParticleScaleEscalations++;
        if (needed > gNdsParticleScaleShiftMax)
        {
            gNdsParticleScaleShiftMax = needed;
        }
    }
    if (submit_route >= 4u)
    {
        if (ndsRendererAppendWhispyPacketState(
                texture_name, texture_slot, poly_alpha,
                submit_route) == FALSE)
        {
            return -1;
        }
        if ((submit_route >= 7u) && (coordinates_safe != FALSE))
        {
            for (corner = 0u; corner < 4u; corner++)
            {
                s32 right_sign =
                    ((corner == 0u) || (corner == 3u)) ? -1 : 1;
                s32 up_sign = (corner < 2u) ? -1 : 1;
                u32 whispy_shift = sNdsRendererParticleScaleShift;
                u32 component;

                for (component = 0u; component < 3u; component++)
                {
                    packed_vertex[corner][component] =
                        ndsRendererWhispyCoordToV16Unchecked(
                            center[component] +
                            (right_sign * right[component]) +
                            (up_sign * up[component]), whispy_shift);
                }
            }
        }
        else
        {
            for (corner = 0u; corner < 4u; corner++)
            {
                s32 right_sign =
                    ((corner == 0u) || (corner == 3u)) ? -1 : 1;
                s32 up_sign = (corner < 2u) ? -1 : 1;
                u32 whispy_shift = sNdsRendererParticleScaleShift;
                u32 component;

                for (component = 0u; component < 3u; component++)
                {
                    packed_vertex[corner][component] =
                        ndsRendererWhispyCoordToV16(
                            center[component] +
                            (right_sign * right[component]) +
                            (up_sign * up[component]), whispy_shift);
                }
            }
        }
        if (ndsRendererAppendWhispyPacketQuad(
                color, packed_vertex, texture_w, texture_h,
                texture_slot, poly_alpha) == FALSE)
        {
            return -1;
        }
    }
    else
    {
        glColor((rgb)color);

        /* Four fixed corners, still in source draw order. A small integer loop
         * is materially cheaper in ARM9 code/RAM than cloning the diagnostic-
         * aware GX wrappers four times. */
        for (corner = 0u; corner < 4u; corner++)
        {
            s32 right_sign =
                ((corner == 0u) || (corner == 3u)) ? -1 : 1;
            s32 up_sign = (corner < 2u) ? -1 : 1;
            u32 texel_s =
                ((corner == 0u) || (corner == 3u)) ? 0u : texture_w;
            u32 texel_t = (corner < 2u) ? texture_h : 0u;
            u32 whispy_shift = sNdsRendererParticleScaleShift;

            glTexCoord2t16((t16)(texel_s << 4), (t16)(texel_t << 4));
            glVertex3v16(
                ndsRendererWhispyCoordToV16(
                    center[0] + (right_sign * right[0]) +
                        (up_sign * up[0]), whispy_shift),
                ndsRendererWhispyCoordToV16(
                    center[1] + (right_sign * right[1]) +
                        (up_sign * up[1]), whispy_shift),
                ndsRendererWhispyCoordToV16(
                    center[2] + (right_sign * right[2]) +
                        (up_sign * up[2]), whispy_shift));
        }
    }
    return 1;
}
#else
void ndsRendererSetWhispyNativeBasis(const Vec3f *right, const Vec3f *up)
{
    (void)right; (void)up;
}

s32 ndsRendererSubmitWhispyNativeQuad(u32 texture_name,
                                      u32 texture_slot,
                                      const Vec3f *pos, f32 size,
                                      const s32 fixed_center_q12[3],
                                      s32 fixed_size_q8,
                                      u32 color, u8 alpha,
                                      u32 mirror_mask,
                                      u32 texture_w, u32 texture_h,
                                      u32 submit_route)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    (void)texture_name; (void)pos; (void)size; (void)color; (void)alpha;
    (void)fixed_center_q12; (void)fixed_size_q8;
    (void)texture_slot; (void)mirror_mask; (void)texture_w; (void)texture_h;
    (void)submit_route;
    return -1;
}
#endif

void ndsRendererEndParticleQuads(void)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    u32 whispy_packet_popped = FALSE;

#if NDS_R2_WHISPY_NATIVE_AOT
    /* Route 4 leaves the final source-ordered packet in cached RAM until this
     * natural pass boundary. The balancing pop rides after its last vertex. */
    whispy_packet_popped = ndsRendererFinishWhispyNativePacket();
#endif
    if (sNdsRendererParticleQuadOpen != 0u)
    {
        /* NO glEnd(). Task 29 removed libnds's dummy glEnd FIFO writes from
         * this renderer and check-gbi-decode-fixtures.ps1 pins the count at
         * one, because a primitive group ends when the NEXT glBegin starts --
         * an extra FIFO write desynchronises the command stream instead of
         * closing anything. The first build of this emitter called it and hung
         * the ROM with GXSTAT=0e008900, geometry-engine-busy forever.
         *
         * So the group is left open exactly as ndsRendererHardwareEndBatch
         * leaves its own, and the tracker is told nothing is open so a later
         * reuse check cannot match stale state. */
        sNdsRendererParticleQuadOpen = FALSE;
        sNdsRendererParticleQuadAlpha = 0u;
        sNdsRendererParticleQuadTexture = 0u;
        ndsRendererHardwareEndBatch();
        /* AFTER EndBatch, not before. The comment above is explicit that this
         * group is left open exactly as EndBatch leaves its own and that an
         * extra FIFO write here hung the ROM at GXSTAT=0e008900; popping the
         * matrix before the batch is flushed would retroactively move vertices
         * already queued against it. Pairs with the unconditional push at batch
         * open, at whatever shift the pass escalated to. */
        if (whispy_packet_popped == FALSE)
        {
            glPopMatrix(1);
        }
        sNdsRendererParticleScaleShift = 0u;
        /* The bound-texture name is LEFT ALONE. Clearing it here forced the
         * next binder to re-issue every frame, and the stage's prepared-run
         * reuse rides on that state: rebuilds went 2 -> 197 a match purely
         * from this line. The tracker already knows the atlas is bound, and
         * the next real bind compares against it correctly. */
        sNdsRendererHardwareActiveTextureEntry = NULL;
    }
#if NDS_R2_WHISPY_NATIVE_AOT
    sNdsRendererWhispyBasisValid = FALSE;
#endif
}

#if NDS_R2_FOX_BLASTER_QUAD
/* ARM946E-S has no FPU. The beam's source DObj is already a closed four-vertex
 * contract, so paying __aeabi_fmul/__aeabi_f2iz for every coordinate would
 * throw away most of the native-submit gain. Decode finite IEEE-754 directly
 * into Q12, with the same truncation toward zero as a C cast. */
static sb32 ndsRendererFoxBlasterFloatToQ12(f32 value, s32 *fixed)
{
    union { f32 f; u32 u; } bits;
    u32 exponent;
    u32 mantissa;
    u32 magnitude;
    u32 limit;
    u32 sign;
    s32 shift;

    bits.f = value;
    sign = bits.u >> 31;
    exponent = (bits.u >> 23) & 0xffu;
    mantissa = bits.u & 0x7fffffu;
    if ((fixed == NULL) || (exponent == 0xffu))
    {
        return FALSE;
    }
    if (exponent == 0u)
    {
        if (mantissa == 0u)
        {
            *fixed = 0;
            return TRUE;
        }
        shift = -126 - 23 + 12;
    }
    else
    {
        mantissa |= 0x800000u;
        shift = (s32)exponent - 127 - 23 + 12;
    }
    limit = (sign != 0u) ? 0x80000000u : 0x7fffffffu;
    if (shift >= 0)
    {
        if ((shift >= 32) || (mantissa > (limit >> (u32)shift)))
        {
            return FALSE;
        }
        magnitude = mantissa << (u32)shift;
    }
    else if (shift <= -32)
    {
        magnitude = 0u;
    }
    else
    {
        magnitude = mantissa >> (u32)-shift;
    }
    *fixed = (sign != 0u) ?
        ((magnitude == 0x80000000u) ? INT_MIN : -(s32)magnitude) :
        (s32)magnitude;
    return TRUE;
}

static u32 ndsRendererFoxBlasterAbsQ12(s32 value)
{
    return (value < 0) ? (u32)(-(s64)value) : (u32)value;
}

static v16 ndsRendererFoxBlasterQ12ToV16(s32 value, u32 scale_shift)
{
    u32 shift = 8u + scale_shift;

    if (value < 0)
    {
        return (v16)-(s32)((u32)(-(s64)value) >> shift);
    }
    return (v16)((u32)value >> shift);
}

/* relocData 316, decoded from the source bytes:
 *
 *   (  0,  24, 0), (  0, -26, 0),
 *   (-30, -26, 0), (-30,  24, 0), RGBA (219,0,134,0)
 *
 * Its combine mode uses shade RGB with full combiner alpha, texture disabled.
 * The DObj owns TraRotRpyRSca; the admitted path is exactly rotate.z 0 or pi,
 * selected from the source velocity by the caller. No texture, DL decode,
 * Vtx staging, matrix build, trig, or software-float multiply remains here. */
NDS_RENDERER_FAST_RUN_CODE
s32 ndsRendererSubmitFoxBlasterQuad(const Vec3f *translate,
                                    f32 scale_x, f32 scale_y,
                                    s32 facing)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    s32 tx;
    s32 ty;
    s32 tz;
    s32 sx;
    s32 sy;
    s32 x0;
    s32 x1;
    s32 y0;
    s32 y1;
    s32 span_x;
    s32 span_y0;
    s32 span_y1;
    v16 vx0;
    v16 vx1;
    v16 vy0;
    v16 vy1;
    v16 vz;
    u32 max_q12;
    u32 scale_shift = 0u;

    if ((translate == NULL) || ((facing != 1) && (facing != -1)) ||
        (ndsRendererFoxBlasterFloatToQ12(translate->x, &tx) == FALSE) ||
        (ndsRendererFoxBlasterFloatToQ12(translate->y, &ty) == FALSE) ||
        (ndsRendererFoxBlasterFloatToQ12(translate->z, &tz) == FALSE) ||
        (ndsRendererFoxBlasterFloatToQ12(scale_x, &sx) == FALSE) ||
        (ndsRendererFoxBlasterFloatToQ12(scale_y, &sy) == FALSE) ||
        (sx <= 0) || (sy <= 0) || (sx > (INT_MAX / 30)) ||
        (sy > (INT_MAX / 26)))
    {
        return FALSE;
    }
    /* OWNER DECISION 2026-08-13 -- the approved draw-only bore offset, and this
     * is the whole of it for the beam. `translate` is const and is the weapon
     * DObj's own world position: it is not written here, so spawn, attack_pos,
     * hitbox and collision are untouched by construction.
     *
     * The raise lands on the DECODED translation, before span_x/span_y0/span_y1
     * apply the source scale to the quad's four source vertices. scale.x runs
     * 1.0 -> 53.33 over a shot's first ten ticks; folding the offset in later
     * would stretch the raise by that same 53x as the beam grew. */
    if (ty > (INT_MAX - NDS_FOX_BLASTER_BORE_OFFSET_Y_Q12))
    {
        return FALSE;
    }
    ty += NDS_FOX_BLASTER_BORE_OFFSET_Y_Q12;
    span_x = 30 * sx;
    span_y0 = 24 * sy;
    span_y1 = 26 * sy;
    if (facing > 0)
    {
        if ((tx < (INT_MIN + span_x)) ||
            (ty > (INT_MAX - span_y0)) ||
            (ty < (INT_MIN + span_y1)))
        {
            return FALSE;
        }
        x0 = tx;
        x1 = tx - span_x;
        y0 = ty + span_y0;
        y1 = ty - span_y1;
    }
    else
    {
        /* Rz(pi): local -X becomes world +X and local Y changes sign. */
        if ((tx > (INT_MAX - span_x)) ||
            (ty < (INT_MIN + span_y0)) ||
            (ty > (INT_MAX - span_y1)))
        {
            return FALSE;
        }
        x0 = tx;
        x1 = tx + span_x;
        y0 = ty - span_y0;
        y1 = ty + span_y1;
    }
    max_q12 = ndsRendererFoxBlasterAbsQ12(x0);
#define NDS_FOX_BLASTER_MAX_Q12(value) do { \
    u32 magnitude__ = ndsRendererFoxBlasterAbsQ12((value)); \
    if (magnitude__ > max_q12) { max_q12 = magnitude__; } \
} while (0)
    NDS_FOX_BLASTER_MAX_Q12(x1);
    NDS_FOX_BLASTER_MAX_Q12(y0);
    NDS_FOX_BLASTER_MAX_Q12(y1);
    NDS_FOX_BLASTER_MAX_Q12(tz);
#undef NDS_FOX_BLASTER_MAX_Q12
    while ((scale_shift < NDS_RENDERER_PARTICLE_MAX_SCALE_SHIFT) &&
           ((max_q12 >> (8u + scale_shift)) > 32767u))
    {
        scale_shift++;
    }
    if ((max_q12 >> (8u + scale_shift)) > 32767u)
    {
        return FALSE;
    }

#if NDS_R2_WHISPY_NATIVE_AOT
    ndsRendererFlushWhispyNativePacket();
#endif
    /* A direct weapon display cannot borrow an open particle primitive. Flush
     * it first if ordering ever places one here, then own one balanced matrix
     * push for this source quad. */
    ndsRendererEndParticleQuads();
    ndsRendererHardwareEndBatch();
    if (ndsRendererLoadFoxBlasterNoZCameraMatrices() == FALSE)
    {
        return FALSE;
    }
    glPushMatrix();
    if (scale_shift != 0u)
    {
        s32 factor = inttof32(1 << scale_shift);
        glScalef32(factor, factor, factor);
    }
    /* Match the interpreted renderer's GL_NOTEXTURE TEXIMAGE_PARAM contract.
     * Clearing DISP3DCNT's global texture bit is not the same GX material
     * state and visibly shifts this thin saturated beam on DS. The shared
     * no-texture name carries no texel payload and is normally already hot. */
    glEnable(GL_TEXTURE_2D);
    ndsRendererHardwareBindNoTexture(NULL);
    /* The generic source renderer derives ID 1 from this list's single
     * combine command. Keep that exact ID: the spawn glow uses particle ID 0,
     * and collapsing both to 0 changes their DS translucent-overlap result. */
    ndsRendererHardwareSetPolyFmt(
        POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(1));
    vx0 = ndsRendererFoxBlasterQ12ToV16(x0, scale_shift);
    vx1 = ndsRendererFoxBlasterQ12ToV16(x1, scale_shift);
    vy0 = ndsRendererFoxBlasterQ12ToV16(y0, scale_shift);
    vy1 = ndsRendererFoxBlasterQ12ToV16(y1, scale_shift);
    vz = ndsRendererFoxBlasterQ12ToV16(tz, scale_shift);
    /* The source quadrangle decodes to triangles 0/1/2 and 0/2/3. A DS
     * GL_QUAD covers the same mathematical area but rasterizes this five-pixel
     * beam differently, so retain the source triangle split exactly. */
    glBegin(GL_TRIANGLES);
    /* GX BEGIN establishes the primitive's vertex state. Match the generic
     * path and write COLOR after it; writing COLOR first inherits the prior
     * primitive's latched color on hardware. */
    glColor(RGB15(27, 0, 16));
    glVertex3v16(vx0, vy0, vz);
    glVertex3v16(vx0, vy1, vz);
    glVertex3v16(vx1, vy1, vz);
    glVertex3v16(vx0, vy0, vz);
    glVertex3v16(vx1, vy1, vz);
    glVertex3v16(vx1, vy0, vz);
    ndsRendererHardwareEndBatch();
    glPopMatrix(1);
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    return TRUE;
}
#endif /* NDS_R2_FOX_BLASTER_QUAD */

#if NDS_R2_FOX_GUN_OVERLAY
/* Fox's blaster geometry, uploaded once. Unlike the glow this reads no asset
 * file: nds_fox_gun.c carries the source's own CI4 32x16 and its 16 RGBA5551
 * colours in the DS channel order, so the "conversion" is a memcpy into VRAM.
 * Its own immutable GL name is scene-owned directly, because a 22-triangle
 * part that appears and disappears with a motion script must not consume a
 * dynamic material-cache slot against the fighter's own materials. */
static int sNdsRendererFoxGunName;
static int sNdsRendererFoxGunPaletteFormat = -1;

static void ndsRendererHardwareResetFoxGunTextureState(void)
{
    sNdsRendererFoxGunName = 0;
    sNdsRendererFoxGunPaletteFormat = -1;
    gNdsRendererFoxGunBytes = 0u;
}

static void ndsRendererHardwareReleaseFoxGunTexture(void)
{
    if (sNdsRendererFoxGunName != 0)
    {
        if (sNdsRendererHardwareBoundTextureName ==
            (u32)sNdsRendererFoxGunName)
        {
            sNdsRendererHardwareBoundTextureName = 0u;
        }
        ndsRendererHardwareFencedGlDeleteTextures(
            1, &sNdsRendererFoxGunName);
    }
    ndsRendererHardwareResetFoxGunTextureState();
}

static s32 ndsRendererHardwarePrepareFoxGunTexture(void)
{
    const u8 *texels;
    const u16 *palette;
    u32 texel_bytes = 0u;
    u32 palette_entries = 0u;
    int size_x;
    int size_y;
    int name = 0;
    /* No WRAP_S/WRAP_T: MiscData315's G_SETTILE (command 10) sets cmS and cmT
     * to G_TX_CLAMP, and libnds spells clamp as the absence of those bits. It
     * matters at the seam -- the baked texcoords touch the exact far edge
     * (s=512 of 512, t=1024 of 1024), which repeat wraps back to texel 0. */
    int params = TEXGEN_TEXCOORD;

    if (sNdsRendererFoxGunName != 0)
    {
        return TRUE;
    }
    gNdsRendererFoxGunPrepareCount++;
    texels = ndsFoxGunTexels(&texel_bytes);
    palette = ndsFoxGunPalette(&palette_entries);
    if ((texels == NULL) || (palette == NULL) ||
        (texel_bytes != ((NDS_FOX_GUN_TEXTURE_WIDTH *
                          NDS_FOX_GUN_TEXTURE_HEIGHT) >> 1)) ||
        (palette_entries != 16u) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_FOX_GUN_TEXTURE_WIDTH, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_FOX_GUN_TEXTURE_HEIGHT, &size_y) == FALSE))
    {
        goto fail;
    }
    if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
    {
        goto fail;
    }
    sNdsRendererFoxGunName = name;
    ndsRendererHardwareBindTextureName(NULL, (u32)name);
    if (ndsRendererHardwareFencedGlTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB16, size_x, size_y, 0, params,
            texels) == 0)
    {
        goto fail;
    }
    glColorTableEXT(GL_TEXTURE_2D, 0, (int)palette_entries, 0, 0, palette);
    glGetColorTableParameterEXT(
        GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT,
        &sNdsRendererFoxGunPaletteFormat);
    if (sNdsRendererFoxGunPaletteFormat < 0)
    {
        goto fail;
    }
    gNdsRendererFoxGunBytes =
        texel_bytes + (palette_entries * sizeof(u16));
    return TRUE;

fail:
    ndsRendererHardwareReleaseFoxGunTexture();
    gNdsRendererFoxGunFailCount++;
    return FALSE;
}

/* Submit the blaster.
 *
 * `composed` is the FINAL matrix, built by the caller exactly the way a fighter
 * root's is: joint 17's world matrix, times the frame camera modelview, times
 * the projection.
 *
 * THE MATRIX GOES THROUGH ndsRendererLoadHardwareRawComposedMatrix, NOT
 * glLoadMatrix4x4. Loading it by hand is what made this row look fixed while the
 * gun was invisible, and it got two separate things wrong:
 *
 *   * The world-unit half of the encoding. Vertices are submitted as
 *     `source << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)` -- the x16 below --
 *     and the composed matrix's complete homogeneous row 3 must be divided by
 *     the same 256 (ndsRendererBuildRawHardwareMatrix). Applying one half and
 *     not the other renders the mesh at 1/256 scale, correctly placed and
 *     correctly oriented and far too small to rasterize. MEASURED on
 *     build-c128-foxgun with no rebuild: the 44 corners spanned 0.036 x 0.032
 *     px around (143.97, 47.90), 44/44 inside the viewport, 0 behind the
 *     camera. The same captured matrix with the shift gives 9.245 x 8.191 px.
 *     Evidence: artifacts/verification/2026-08-12_fox-gun-matrix.txt, replayed
 *     by scripts/fox_gun_screen_bounds.py.
 *   * GL_PROJECTION. A composed MVP in GL_MODELVIEW is only right when the
 *     projection is identity, and it is NOT identity here -- this target builds
 *     NDS_R2_FIGHTER_HW_MTX=1, so the fighter that draws immediately before
 *     leaves the camera projection loaded. The pair loader sets identity.
 *
 * This runs immediately after the fighter's own production run, so it inherits
 * a live GX context and owns only what it changes: one matrix pair, one texture
 * bind, one poly format, one colour, one batch. It never touches the fighter's
 * baked stream, which is the whole point of an overlay.
 */
s32 ndsRendererSubmitFoxGun(const NDSRendererMatrix20p12 *composed)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    const NDSFoxGunVertex *vertices;
    const u8 (*triangles)[3];
    u32 vertex_count = 0u;
    u32 triangle_count = 0u;
    u32 index;

    if (composed == NULL)
    {
        return FALSE;
    }
    if (ndsRendererHardwarePrepareFoxGunTexture() == FALSE)
    {
        return FALSE;
    }
    vertices = ndsFoxGunVertices(&vertex_count);
    triangles = ndsFoxGunTriangles(&triangle_count);
    if ((vertices == NULL) || (triangles == NULL) ||
        (vertex_count == 0u) || (triangle_count == 0u))
    {
        return FALSE;
    }

    ndsRendererHardwareEndBatch();
    ndsRendererLoadHardwareRawComposedMatrix(
        composed, ndsRendererNextMatrixGeneration());

    glEnable(GL_TEXTURE_2D);
    ndsRendererHardwareBindTextureName(NULL, (u32)sNdsRendererFoxGunName);
    /* CULL_NONE, not CULL_BACK. The source triangles wind for the N64's
     * front-face convention, and the DS's is the other one, so culling here
     * would show the gun's inside faces or nothing at all. Twenty-two triangles
     * do not pay for getting that wrong; tighten it only against a screenshot
     * pair that proves which faces the DS keeps. */
    ndsRendererHardwareSetPolyFmt(
        POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(0));
    glBegin(GL_TRIANGLES);
    /* AFTER glBegin, and never omitted. The source combiner for this display
     * list is TEXEL0 x SHADE (G_SETCOMBINE at command 6 of MiscData315's list),
     * so the DS's MODULATE needs a latched colour; with none the gun inherits
     * whatever the fighter's last vertex left in GX and can come out black.
     * White is the honest stand-in: it shows the source palette unmodulated.
     * The source's directional term (light 0xb3b3b3 over ambient 0x808080,
     * G_MOVEWORD commands 1..4) is NOT reproduced, because hardware lighting
     * would have to rotate these normals by the vector matrix and this path
     * deliberately loads a composed MVP there -- R2-03 E16b's rule that normals
     * must never be rotated by the projection. Recorded as a fidelity delta. */
    glColor(RGB15(31, 31, 31));
    for (index = 0u; index < triangle_count; index++)
    {
        u32 corner;

        for (corner = 0u; corner < 3u; corner++)
        {
            const NDSFoxGunVertex *vertex =
                &vertices[triangles[index][corner]];

            glTexCoord2t16(
                (t16)(vertex->st[0] >> NDS_FOX_GUN_TEXCOORD_SHIFT),
                (t16)(vertex->st[1] >> NDS_FOX_GUN_TEXCOORD_SHIFT));
            glVertex3v16(
                (v16)(vertex->pos[0] * NDS_FOX_GUN_VERTEX_SCALE),
                (v16)(vertex->pos[1] * NDS_FOX_GUN_VERTEX_SCALE),
                (v16)(vertex->pos[2] * NDS_FOX_GUN_VERTEX_SCALE));
        }
    }
    ndsRendererHardwareEndBatch();
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    gNdsRendererFoxGunDrawCount++;
    gNdsRendererFoxGunTriangleCount += triangle_count;
    return TRUE;
}
#endif /* NDS_R2_FOX_GUN_OVERLAY */

/* DEBUG-ONLY world-space collision-diamond, for tuning the fireball's
 * stage-collision box. The DS geometry engine has no line primitive, so the
 * diamond is drawn as two filled translucent triangles (a flat diamond shape)
 * through the same particle camera matrix and scale the quad batch uses, so it
 * lands exactly where the collision probes reach. Must be called while a
 * particle quad batch is OPEN -- glBegin(GL_TRIANGLE) closes the preceding quad
 * group on this hardware (no glEnd needed) and the caller's
 * ndsRendererEndParticleQuads flushes everything. The diamond is axis-aligned
 * in world space: cx/cy/cz is the fireball translate; top/center/bottom/width
 * are the MPObjectColl offsets. */
void ndsRendererSubmitDebugDiamond(f32 cx, f32 cy, f32 cz,
                                   f32 top, f32 center,
                                   f32 bottom, f32 width)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
#if NDS_R2_WHISPY_NATIVE_AOT
    ndsRendererFlushWhispyNativePacket();
#endif
    if (sNdsRendererParticleQuadOpen == 0u)
    {
        return;
    }
    /* Translucent green, unlit, both faces: a visible overlay that does not
     * fully occlude the fireball sprite beneath it. Disable the texture so the
     * triangles draw as flat colour. */
    glDisable(GL_TEXTURE_2D);
    ndsRendererHardwareSetPolyFmt(POLY_ALPHA(10) | POLY_CULL_NONE |
                                  POLY_ID(0));
    glColor(RGB15(0, 31, 0));
    glBegin(GL_TRIANGLE);
    /* The batch pushed a matrix scaled by the current shift; emit vertices in
     * the same world-fixed / shift units the quad submitter uses so they land
     * in the same place. */
    {
        s32 sx = ndsRendererParticleWorldToV16(cx, sNdsRendererParticleScaleShift);
        s32 sc = ndsRendererParticleWorldToV16(cy + center,
                                               sNdsRendererParticleScaleShift);
        s32 st = ndsRendererParticleWorldToV16(cy + top,
                                               sNdsRendererParticleScaleShift);
        s32 sb = ndsRendererParticleWorldToV16(cy + bottom,
                                               sNdsRendererParticleScaleShift);
        s32 sw = ndsRendererParticleWorldToV16(width,
                                               sNdsRendererParticleScaleShift);
        s32 sz = ndsRendererParticleWorldToV16(cz,
                                               sNdsRendererParticleScaleShift);
        /* Two triangles: top/right/center, center/bottom/left -- a flat diamond
         * spanning top..bottom and -width..+width. */
        glVertex3v16(sx, st, sz);
        glVertex3v16(sx + sw, sc, sz);
        glVertex3v16(sx - sw, sc, sz);

        glVertex3v16(sx - sw, sc, sz);
        glVertex3v16(sx + sw, sc, sz);
        glVertex3v16(sx, sb, sz);
    }
    /* Leave the triangle group open; ndsRendererEndParticleQuads flushes it. Do
     * NOT re-enable GL_TEXTURE_2D here -- the batch is about to close. */
}
#endif /* NDS_R2_PARTICLE_RUNTIME */

void ndsRendererHardwareArmBattleStaticTextures(void)
{
    if ((gNdsRendererBattleStaticTextureEnabled != 0u) &&
        (sNdsRendererBattleStaticTexturePrepared != 0u) &&
        (sNdsRendererBattleStaticTextureArmed == 0u))
    {
        memset((void *)gNdsRendererBattleTextureFenceCounts, 0,
               sizeof(gNdsRendererBattleTextureFenceCounts));
        gNdsRendererBattleTextureFenceFirstClassPlus1 = 0u;
        gNdsRendererBattleTextureFenceFirstFrame = 0u;
        sNdsRendererBattleStaticTextureArmed = TRUE;
        gNdsRendererBattleStaticTextureArmCount++;
    }
}

void ndsRendererHardwareDiscardBattleStaticTextures(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 was_prepared = sNdsRendererBattleStaticTexturePrepared;

    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
    ndsRendererHardwareDiscardTextureCache();
    if (was_prepared != 0u)
    {
        gNdsRendererBattleStaticTextureTeardownCount++;
    }
#endif
    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
    gNdsRendererStaticTexturePreparedNow = 0u;
}

void ndsRendererHardwareAbortBattleStaticTextures(void)
{
    /* Counted unconditionally and before anything else can early-out: this is
     * the entry point that makes the scene's textures unrecoverable, so "was it
     * called" must never depend on a build flag. See the row 6 note above. */
    gNdsRendererStageOwnerAbortCount++;
#if NDS_RENDERER_HW_TRIANGLES
    u32 was_prepared = sNdsRendererBattleStaticTexturePrepared;

    /* This deliberately retains the armed bit through every pinned delete.
     * DiscardTextureCache records the violation and clears the bit last. */
    ndsRendererHardwareDiscardTextureCache();
    if (was_prepared != 0u)
    {
        gNdsRendererBattleStaticTextureTeardownCount++;
    }
#endif
    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
    gNdsRendererStaticTexturePreparedNow = 0u;
}

static void ndsRendererHardwareBindNoTexture(NDSRendererStats *stats)
{
    if (sNdsRendererHardwareNoTextureName == 0)
    {
        ndsRendererHardwareEndBatch();
        if (ndsRendererHardwareFencedGlGenTextures(
                1, &sNdsRendererHardwareNoTextureName) == 0)
        {
            return;
        }
        ndsRendererHardwareBindTextureState(
            sNdsRendererHardwareNoTextureName);
        sNdsRendererHardwareBoundTextureName =
            (u32)sNdsRendererHardwareNoTextureName;
        ndsRendererHardwareFencedGlTexImage2D(
            GL_TEXTURE_2D, 0, GL_NOTEXTURE, 0, 0, 0,
            TEXGEN_TEXCOORD, NULL);
    }
    else
    {
        ndsRendererHardwareBindTextureName(
            stats, (u32)sNdsRendererHardwareNoTextureName);
    }
    sNdsRendererHardwareActiveTextureEntry = NULL;
}

static u16 ndsRendererHardwareConvertRgba16(u16 n64_color,
                                            s32 preserve_transparent_rgb)
{
    u16 red;
    u16 green;
    u16 blue;
    u16 alpha;

    if (((n64_color & 1u) == 0u) &&
        (preserve_transparent_rgb == FALSE))
    {
        return 0u;
    }

    red = (u16)((n64_color >> 11) & 0x1fu);
    green = (u16)((n64_color >> 6) & 0x1fu);
    blue = (u16)((n64_color >> 1) & 0x1fu);
    alpha = (n64_color & 1u) ? (1u << 15) : 0u;
    return (u16)(alpha | red | (green << 5) | (blue << 10));
}

static u16 ndsRendererHardwareConvertRgba32(u32 rgba)
{
    u8 red = (u8)(rgba >> 24);
    u8 green = (u8)(rgba >> 16);
    u8 blue = (u8)(rgba >> 8);
    u8 alpha = (u8)rgba;

    if (alpha == 0u)
    {
        return 0u;
    }
    return (u16)((1u << 15) |
                 ((u16)(red >> 3)) |
                 ((u16)(green >> 3) << 5) |
                 ((u16)(blue >> 3) << 10));
}

static u16 ndsRendererHardwareConvertI(u8 intensity)
{
    u16 v;

    v = (u16)(intensity >> 3);
    return (u16)((1u << 15) | v | (v << 5) | (v << 10));
}

static u16 ndsRendererHardwareConvertI16(u16 value)
{
    u8 intensity = (u8)(value >> 8);

    if (intensity == 0u)
    {
        intensity = (u8)value;
    }
    return ndsRendererHardwareConvertI(intensity);
}

static u16 ndsRendererHardwareConvertIA(u8 intensity, u8 alpha)
{
    u16 v;

    if (alpha == 0u)
    {
        return 0u;
    }
    v = (u16)(intensity >> 3);
    return (u16)((1u << 15) | v | (v << 5) | (v << 10));
}

/* Losslessly repack a resolved RGB5A1 image into the DS's native 16-colour
 * format when the final image actually uses at most sixteen visible colours.
 *
 * This is the runtime twin of generate_battle_playable_static_textures.py's
 * repack_paletted(): the renderer has already resolved every N64 texture,
 * palette, material and combiner contribution into canonical RGB5A1 pixels, so
 * this changes representation only. Alpha-zero RGB is intentionally collapsed
 * to palette index 0 because those colour bits are not visible; every opaque
 * halfword remains exact. GL_RGB16 then stores two texels per byte instead of
 * one 16-bit halfword per texel, returning 75% of the texture-VRAM footprint.
 *
 * `pixels` is packed in place. The packed write cursor is always behind the
 * unread u16 cursor, so a separate scratch allocation is unnecessary. Returns
 * the palette entry count, or zero when the image needs direct colour. */
static u32 ndsRendererHardwarePackResolvedPal16(
    u16 *pixels, u32 pixel_count, u16 palette[16], u32 *color0_transparent)
{
    u8 *packed = (u8 *)pixels;
    u32 palette_count = 0u;
    u32 i;

    if ((pixels == NULL) || (palette == NULL) ||
        (color0_transparent == NULL) || (pixel_count == 0u))
    {
        return 0u;
    }

    for (i = 0u; i < pixel_count; i++)
    {
        u16 color = ((pixels[i] & 0x8000u) != 0u) ? pixels[i] : 0u;
        u32 index;

        for (index = 0u; index < palette_count; index++)
        {
            if (palette[index] == color)
            {
                break;
            }
        }
        if (index == palette_count)
        {
            if (palette_count >= 16u)
            {
                return 0u;
            }
            palette[palette_count++] = color;
        }
    }

    /* COLOR0_TRANSPARENT applies to index zero only. Keep the generator's
     * invariant that a transparent image has canonical 0 in that slot. */
    *color0_transparent = FALSE;
    for (i = 0u; i < palette_count; i++)
    {
        if (palette[i] == 0u)
        {
            u16 first = palette[0];

            palette[0] = 0u;
            palette[i] = first;
            *color0_transparent = TRUE;
            break;
        }
    }

    for (i = 0u; i < pixel_count; i++)
    {
        u16 color = ((pixels[i] & 0x8000u) != 0u) ? pixels[i] : 0u;
        u32 index;

        for (index = 0u; index < palette_count; index++)
        {
            if (palette[index] == color)
            {
                break;
            }
        }
        if ((i & 1u) != 0u)
        {
            packed[i >> 1] |= (u8)(index << 4);
        }
        else
        {
            packed[i >> 1] = (u8)index;
        }
    }
    return palette_count;
}

static u32 ndsRendererHardwareTextureLinePixels(u32 size, u32 line)
{
    switch (size)
    {
    case NDS_RENDERER_HW_TEXTURE_SIZ_4B:
        return line * 16u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_8B:
        return line * 8u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_16B:
        return line * 4u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_32B:
        return line * 2u;
    default:
        return 0u;
    }
}

/* Task 37: 132 bytes at 8.54 cycles per instruction, called from every texture
 * bind and resolve. Small, hot, and reached from many different call sites --
 * the shape that never stays resident in a shared instruction cache. */
static u32 NDS_R2_ITCM_PACK2_EVICTED_PLAIN_CODE ndsRendererHardwareTextureSourceBytes(
    u32 format, u32 size, u32 texels)
{
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_RGBA16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            return texels * sizeof(u16);
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
            texels * sizeof(u32) : 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_I16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ?
            texels * sizeof(u16) : 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_IA)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ?
            texels * sizeof(u16) : 0u;
    }
    return 0u;
}

static u32 ndsRendererHardwareTextureSourceWidthPixels(u32 render_size,
                                                       u32 image_size,
                                                       u32 image_width)
{
    if ((render_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (image_size == NDS_RENDERER_HW_TEXTURE_SIZ_8B))
    {
        return image_width * 2u;
    }
    return image_width;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererHardwareTextureFormatBit(u32 format, u32 size);
#endif

static NDSRendererTextureDataLayout ndsRendererTextureDataLayout(
    const NDSRendererConfig *config)
{
    if (config == NULL)
    {
        return NDS_RENDERER_TEXTURE_DATA_NATIVE;
    }
    if (config->texture_data_layout ==
        NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED)
    {
        return NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    }
    return NDS_RENDERER_TEXTURE_DATA_NATIVE;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererTexturePackMap(
    NDSRendererTextureDataLayout layout, u32 stride)
{
    u32 map = 0u;
    u32 i;

    for (i = 0u; i < 4u; i++)
    {
        u32 physical = i;

        if (layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED)
        {
            physical = i ^ stride;
        }
        map |= (physical & 0xffu) << (i * 8u);
    }
    return map;
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererRecordTextureLaneCount(
    NDSRendererTextureDataLayout layout, u32 is_halfword,
    u32 format, u32 size, u32 count)
{
    u32 format_bit = ndsRendererHardwareTextureFormatBit(format, size);

    gNdsRendererProfileTextureLaneLayoutMask |= 1u << (u32)layout;
    if (is_halfword != 0u)
    {
        gNdsRendererProfileTextureLaneHalfwordAccessCount += count;
        gNdsRendererProfileTextureLaneHalfwordFormatMask |= format_bit;
        gNdsRendererProfileTextureLaneHalfwordMap =
            ndsRendererTexturePackMap(layout, 1u);
    }
    else
    {
        gNdsRendererProfileTextureLaneByteAccessCount += count;
        gNdsRendererProfileTextureLaneByteFormatMask |= format_bit;
        gNdsRendererProfileTextureLaneByteMap =
            ndsRendererTexturePackMap(layout, 3u);
    }
}
#endif

static u32 ndsRendererTextureLogicalByteIndex(
    NDSRendererTextureDataLayout layout, u32 logical_index)
{
    return (layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ?
        (logical_index ^ 3u) : logical_index;
}

static u32 ndsRendererTextureLogicalHalfwordIndex(
    NDSRendererTextureDataLayout layout, u32 logical_index)
{
    return (layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ?
        (logical_index ^ 1u) : logical_index;
}

static u32 ndsRendererTexturePhysicalByteSpan(
    NDSRendererTextureDataLayout layout, u32 logical_bytes)
{
    if ((layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) &&
        (logical_bytes != 0u))
    {
        return (logical_bytes + 3u) & ~3u;
    }
    return logical_bytes;
}

static u8 ndsRendererReadTextureByte(
    const NDSRendererConfig *config, const u8 *texels, u32 logical_index,
    u32 format, u32 size)
{
    NDSRendererTextureDataLayout layout = ndsRendererTextureDataLayout(config);

    /* The conversion loop aggregates this access after rasterization. Do not
     * update volatile diagnostics for every converted texel: animated
     * TEXEL0/TEXEL1 water reads this path tens of thousands of times per
     * frame, while the lane contract is invariant for the conversion. */
    (void)format;
    (void)size;
    return texels[ndsRendererTextureLogicalByteIndex(layout, logical_index)];
}

static u8 ndsRendererReadTexturePackedNibble(
    const NDSRendererConfig *config, const u8 *texels, u32 logical_texel_index,
    u32 format, u32 size)
{
    u8 packed = ndsRendererReadTextureByte(
        config, texels, logical_texel_index >> 1, format, size);

    return ((logical_texel_index & 1u) == 0u) ?
        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
}

static u32 ndsRendererHardwareCiPaletteEntriesUsed(
    const NDSRendererConfig *config,
    const u8 *texels,
    u32 size,
    u32 source_width,
    u32 source_origin_s,
    u32 source_origin_t,
    u32 source_read_width,
    u32 source_read_height,
    u32 palette_base)
{
    u32 max_index = 0u;
    u32 x;
    u32 y;

    if ((texels == NULL) || (source_width == 0u) ||
        (source_read_width == 0u) || (source_read_height == 0u))
    {
        return palette_base;
    }
    for (y = 0u; y < source_read_height; y++)
    {
        u32 row = (source_origin_t + y) * source_width;

        for (x = 0u; x < source_read_width; x++)
        {
            u32 source_index = row + source_origin_s + x;
            u32 palette_index =
                (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
                    ndsRendererReadTexturePackedNibble(
                        config, texels, source_index,
                        NDS_RENDERER_HW_TEXTURE_FMT_CI, size) :
                    ndsRendererReadTextureByte(
                        config, texels, source_index,
                        NDS_RENDERER_HW_TEXTURE_FMT_CI, size);

            if (palette_index > max_index)
            {
                max_index = palette_index;
            }
        }
    }
    return palette_base + max_index + 1u;
}

static u16 ndsRendererReadTextureHalfword(
    const NDSRendererConfig *config, const u16 *data, u32 logical_index,
    u32 format, u32 size)
{
    NDSRendererTextureDataLayout layout = ndsRendererTextureDataLayout(config);

    (void)format;
    (void)size;
    return data[ndsRendererTextureLogicalHalfwordIndex(layout, logical_index)];
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererRecordTextureLaneUseCount(
    const NDSRendererConfig *config, u32 format, u32 size, u32 count)
{
    NDSRendererTextureDataLayout layout = ndsRendererTextureDataLayout(config);

    if ((size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ||
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B))
    {
        ndsRendererRecordTextureLaneCount(
            layout, FALSE, format, size, count);
    }
    else if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
    {
        ndsRendererRecordTextureLaneCount(
            layout, TRUE, format, size, count);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        ndsRendererRecordTextureLaneCount(
            layout, TRUE, format, NDS_RENDERER_HW_TEXTURE_SIZ_16B, count);
    }
}

static void ndsRendererRecordTextureLaneUse(
    const NDSRendererConfig *config, u32 format, u32 size)
{
    ndsRendererRecordTextureLaneUseCount(config, format, size, 1u);
}
#endif

static u16 ndsRendererHardwarePaletteColor(
    const NDSRendererConfig *config, const u16 *palette, u32 index, u32 count,
    s32 preserve_transparent_rgb)
{
    if ((palette == NULL) || (index >= count))
    {
        return 0u;
    }
    return ndsRendererHardwareConvertRgba16(
        ndsRendererReadTextureHalfword(
            config, palette, index, NDS_RENDERER_HW_TEXTURE_FMT_CI,
            NDS_RENDERER_HW_TEXTURE_SIZ_16B),
        preserve_transparent_rgb);
}

static u16 ndsRendererHardwareTextureColor(
    const NDSRendererConfig *config,
    u32 format,
    u32 size,
    const u8 *texels,
    const u16 *palette,
    u32 palette_count,
    u32 palette_base,
    u32 index,
    s32 preserve_transparent_rgb)
{
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        u32 palette_index;

        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            palette_index = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);
        }
        else
        {
            palette_index = ndsRendererReadTextureByte(
                config, texels, index, format, size);
        }
        palette_index += palette_base;
        return ndsRendererHardwarePaletteColor(config, palette, palette_index,
                                               palette_count,
                                               preserve_transparent_rgb);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_RGBA16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B)
        {
            u32 rgba;

            memcpy(&rgba, &texels[index * sizeof(rgba)], sizeof(rgba));
            return ndsRendererHardwareConvertRgba32(rgba);
        }
        return ndsRendererHardwareConvertRgba16(
            ndsRendererReadTextureHalfword(
                config, (const u16 *)texels, index, format, size),
            preserve_transparent_rgb);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_IA)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            u8 value = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);
            u8 intensity = (u8)(((value >> 1) & 0x07u) * 0x24u);
            u8 alpha = (value & 1u) ? 0xffu : 0u;

            return ndsRendererHardwareConvertIA(intensity, alpha);
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            u8 value = ndsRendererReadTextureByte(
                config, texels, index, format, size);
            u8 intensity = (u8)((value >> 4) * 0x11u);
            u8 alpha = (u8)((value & 0x0fu) * 0x11u);

            return ndsRendererHardwareConvertIA(intensity, alpha);
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            u16 value = ndsRendererReadTextureHalfword(
                config, (const u16 *)texels, index, format, size);

            return ndsRendererHardwareConvertIA((u8)(value >> 8),
                                                (u8)value);
        }
        return 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_I16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            u8 intensity4 = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);

            return ndsRendererHardwareConvertI((u8)(intensity4 * 0x11u));
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return ndsRendererHardwareConvertI(
                ndsRendererReadTextureByte(config, texels, index,
                                           format, size));
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            return ndsRendererHardwareConvertI16(
                ndsRendererReadTextureHalfword(
                    config, (const u16 *)texels, index, format, size));
        }
    }
    return 0u;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static s32 ndsRendererTextureColorNonWhite(u16 color)
{
    u32 r;
    u32 g;
    u32 b;

    if ((color & (1u << 15)) == 0u)
    {
        return FALSE;
    }
    r = color & 0x1fu;
    g = (color >> 5) & 0x1fu;
    b = (color >> 10) & 0x1fu;
    return ((r < 29u) || (g < 29u) || (b < 29u)) ? TRUE : FALSE;
}

static s32 ndsRendererTextureColorDominantGreen(u16 color)
{
    u32 r;
    u32 g;
    u32 b;

    if ((color & (1u << 15)) == 0u)
    {
        return FALSE;
    }
    r = color & 0x1fu;
    g = (color >> 5) & 0x1fu;
    b = (color >> 10) & 0x1fu;
    return ((g >= 10u) && (g > (r + 2u)) && (g > (b + 2u))) ?
        TRUE : FALSE;
}

static void ndsRendererProfileTexturePixel(u16 color, u32 *green_texels,
                                           u32 *nonwhite_texels)
{
    if (ndsRendererTextureColorNonWhite(color) != FALSE)
    {
        if (nonwhite_texels != NULL)
        {
            (*nonwhite_texels)++;
        }
    }
    if (ndsRendererTextureColorDominantGreen(color) != FALSE)
    {
        if (green_texels != NULL)
        {
            (*green_texels)++;
        }
    }
}

static void ndsRendererProfileTextureCacheEntry(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    if ((entry == NULL) || (entry->ready == FALSE))
    {
        return;
    }
    gNdsRendererProfileTextureSourceTexels += entry->source_texels;
    gNdsRendererProfileTextureGreenTexels += entry->green_texels;
    gNdsRendererProfileTextureNonWhiteTexels += entry->nonwhite_texels;
}

static s32 ndsRendererHardwareTextureWrapCoord(s32 coord, u32 size,
                                               u32 wrap, u32 mirror)
{
    s32 period;

    if (size == 0u)
    {
        return 0;
    }
    if (wrap == 0u)
    {
        if (coord < 0)
        {
            return 0;
        }
        if ((u32)coord >= size)
        {
            return (s32)size - 1;
        }
        return coord;
    }

    period = (s32)((mirror != 0u) ? size * 2u : size);
    if (period <= 0)
    {
        return 0;
    }
    coord %= period;
    if (coord < 0)
    {
        coord += period;
    }
    if ((mirror != 0u) && ((u32)coord >= size))
    {
        coord = ((s32)size * 2) - 1 - coord;
    }
    return coord;
}

static void ndsRendererProfileTextureSample(s16 s, s16 t)
{
    const NDSRendererHardwareTextureCacheEntry *entry =
        sNdsRendererHardwareActiveTextureEntry;
    s32 sample_s;
    s32 sample_t;

    if ((entry == NULL) ||
        (entry->profile_width == 0u) ||
        (entry->profile_height == 0u))
    {
        return;
    }

    sample_s = ndsRendererHardwareTextureWrapCoord(
        ((s32)s) >> 4, entry->profile_width,
        (entry->params & GL_TEXTURE_WRAP_S) != 0u,
        (entry->params & GL_TEXTURE_FLIP_S) != 0u);
    sample_t = ndsRendererHardwareTextureWrapCoord(
        ((s32)t) >> 4, entry->profile_height,
        (entry->params & GL_TEXTURE_WRAP_T) != 0u,
        (entry->params & GL_TEXTURE_FLIP_T) != 0u);
    if (((u32)sample_s >= entry->profile_width) ||
        ((u32)sample_t >= entry->profile_height))
    {
        return;
    }

    gNdsRendererProfileTextureSampleCount++;
    if (entry->nonwhite_texels != 0u)
    {
        gNdsRendererProfileTextureSampleNonWhiteCount++;
    }
    if (entry->green_texels != 0u)
    {
        gNdsRendererProfileTextureSampleGreenCount++;
    }
}

static u32 ndsRendererHardwareTextureFormatBit(u32 format, u32 size)
{
    u32 index = (format * 4u) + size;

    return (index < 32u) ? (1u << index) : 0u;
}

static void ndsRendererProfileTextureFormat(volatile u32 *mask,
                                            u32 format, u32 size)
{
    if (mask != NULL)
    {
        *mask |= ndsRendererHardwareTextureFormatBit(format, size);
    }
}
#endif

static void ndsRendererHardwareRejectTexture(NDSRendererStats *stats,
                                             u32 format, u32 size,
                                             u32 reason)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_MANIFEST_FALLBACK);
    if (stats != NULL)
    {
        stats->hardware_texture_reject_count++;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureRejectFormatMask, format, size);
    gNdsRendererProfileTextureRejectReasonMask |= reason;
#elif NDS_R2_STAGE_ROUTE_PROBE
    /* R2-07 E2. The reason mask is the last link in the chain the probe is
     * following: owner reject 342 -> PrepareRun reason 2 -> this resolve
     * refusing. The mask word is already in the ELF; only its writer was gated
     * on profile level 2, which also drags in the oracle comparisons. Take the
     * mask alone. */
    gNdsRendererProfileTextureRejectReasonMask |= reason;
    /* TEXIMAGE means the eviction retry loop ran out of things to evict, and
     * that has two very different causes with two different fixes: texture VRAM
     * genuinely full (bytes), or every cache slot pinned/touched-this-frame so
     * nothing MAY be evicted (slots). Census the cache at the first rejection
     * -- first only, because later ones happen after the loop has already
     * released entries and would describe the aftermath. */
    if (gNdsR2TexRejectCensusValid == 0u)
    {
        u32 census_index;

        gNdsR2TexRejectCensusValid = 1u;
        for (census_index = 0u;
             census_index < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT;
             census_index++)
        {
            const NDSRendererHardwareTextureCacheEntry *census_entry =
                &sNdsRendererHardwareTextureCache[census_index];

            if (census_entry->name == 0)
            {
                gNdsR2TexRejectCensusFree++;
                continue;
            }
            gNdsR2TexRejectCensusLive++;
            if (census_entry->pinned != 0u)
            {
                gNdsR2TexRejectCensusPinned++;
            }
            else if (census_entry->last_used_frame ==
                     (sNdsRendererHardwareFrameSerial + 1u))
            {
                gNdsR2TexRejectCensusThisFrame++;
            }
            else
            {
                gNdsR2TexRejectCensusEvictable++;
            }
        }
    }
    (void)format;
    (void)size;
#else
    (void)format;
    (void)size;
    (void)reason;
#endif
}

static s32 ndsRendererHardwarePrepareTexel1Source(
    const NDSRendererStats *stats,
    const NDSRendererConfig *config,
    u32 primary_format,
    u32 primary_size,
    u32 primary_width,
    u32 primary_height,
    NDSRendererHardwareTexel1Source *out)
{
    const NDSRendererTileState *tile;
    const NDSRendererTextureLoadState *load;
    u32 loaded_bytes;
    u32 width;
    u32 height;
    u32 texels;
    u32 source_read_width;
    u32 source_read_height;
    u32 source_last_index;
    u32 source_bytes;
    u32 source_physical_bytes;
    u32 source_width;
    u32 source_origin_s;
    u32 source_origin_t;
    s32 materialize_s;
    s32 materialize_t;

    if (out == NULL)
    {
        return FALSE;
    }
    memset(out, 0, sizeof(*out));
    if ((stats == NULL) ||
        (ndsRendererHardwareUsesTexel01Lerp(stats) == FALSE) ||
        (ndsRendererActiveTextureTile(stats) != NDS_RENDERER_RENDER_TILE))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_ACTIVE_TILE);
        return FALSE;
    }

    tile = &stats->texture_tiles[NDS_RENDERER_RENDER_TILE_1];
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexel1LastTileState =
        (tile->set_seen & 1u) |
        ((tile->size_seen & 1u) << 1) |
        ((tile->line & 0x1ffu) << 2) |
        ((tile->format & 0x7u) << 11) |
        ((tile->size & 0x3u) << 14) |
        ((tile->shifts & 0xfu) << 16) |
        ((tile->shiftt & 0xfu) << 20);
    gNdsRendererProfileTexel1LastPrimaryState =
        (primary_format & 0x7u) |
        ((primary_size & 0x3u) << 3) |
        ((primary_width & 0xffu) << 8) |
        ((primary_height & 0xffu) << 16);
#endif
    if ((primary_format != NDS_RENDERER_HW_TEXTURE_FMT_CI) ||
        (primary_size != NDS_RENDERER_HW_TEXTURE_SIZ_4B) ||
        (tile->set_seen == 0u) || (tile->size_seen == 0u) ||
        (tile->line == 0u) || (tile->shifts != 0u) ||
        (tile->shiftt != 0u) ||
        (tile->format != primary_format) || (tile->size != primary_size))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_TILE_STATE);
        return FALSE;
    }
    load = ndsRendererHardwareFindTextureLoadForTmem(stats, tile->tmem);
    if ((load == NULL) || (load->image == 0u) ||
        (load->load_texels == 0u))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_LOAD_STATE);
        return FALSE;
    }

    loaded_bytes = (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
        load->load_texels * sizeof(u32) :
        load->load_texels * sizeof(u16);
    width = tile->width;
    height = tile->height;
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        (ndsRendererHardwareTextureSourceBytes(
             primary_format, primary_size, width * height) > loaded_bytes))
    {
        width = ndsRendererHardwareTextureLinePixels(primary_size,
                                                     tile->line);
        texels = load->load_texels * sizeof(u16);
        if (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            texels *= 2u;
        }
        else if ((primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ||
                 (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_32B))
        {
            texels /= 2u;
        }
        height = (width != 0u) ? texels / width : 0u;
    }
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_DIMENSIONS);
        return FALSE;
    }

    out->source_extent_width = width;
    out->source_extent_height = height;
    materialize_s = ndsRendererHardwareTextureMaterializesMaskedClamp(
        tile->cms, tile->masks, width, tile->width);
    materialize_t = ndsRendererHardwareTextureMaterializesMaskedClamp(
        tile->cmt, tile->maskt, height, tile->height);
    if (materialize_s != FALSE)
    {
        width = tile->width;
    }
    if (materialize_t != FALSE)
    {
        height = tile->height;
    }
    if ((width != primary_width) || (height != primary_height))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_PAIR_SIZE);
        return FALSE;
    }

    if (load->load_kind == NDS_RENDERER_TEXTURE_LOADTILE)
    {
        source_origin_s = load->load_uls >> 2;
        source_origin_t = load->load_ult >> 2;
        source_width = ndsRendererHardwareTextureSourceWidthPixels(
            primary_size, load->image_size, load->image_width);
    }
    else
    {
        u32 dxt = load->load_dxt;

        source_origin_s = 0u;
        source_origin_t = 0u;
        source_width = out->source_extent_width;
        if (dxt != 0u)
        {
            u32 qwords =
                (NDS_RENDERER_G_TX_DXT_ONE + dxt - 1u) / dxt;

            source_width = ndsRendererHardwareTextureLinePixels(
                primary_size, qwords);
        }
    }
    source_read_width = (materialize_s != FALSE) ?
        (1u << tile->masks) : width;
    source_read_height = (materialize_t != FALSE) ?
        (1u << tile->maskt) : height;
    if ((source_width == 0u) ||
        (source_origin_s >= source_width) ||
        (source_read_width > (source_width - source_origin_s)) ||
        (source_read_width > out->source_extent_width) ||
        (source_read_height > out->source_extent_height))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_RANGE);
        return FALSE;
    }

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererRecordTextureLaneUse(config, primary_format, primary_size);
#endif
    source_last_index =
        ((source_origin_t + source_read_height - 1u) * source_width) +
        source_origin_s + source_read_width - 1u;
    source_bytes = ndsRendererHardwareTextureSourceBytes(
        primary_format, primary_size, source_last_index + 1u);
    if (source_bytes == 0u)
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_BYTES);
        return FALSE;
    }
    source_physical_bytes = ndsRendererTexturePhysicalByteSpan(
        ndsRendererTextureDataLayout(config), source_bytes);
    out->texels = ndsRendererResolveTextureDataPointer(
        config, (const void *)(uintptr_t)load->image,
        source_physical_bytes);
    if (out->texels == NULL)
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_PTR);
        return FALSE;
    }

    out->load = load;
    out->render_tile = tile;
    out->format = primary_format;
    out->size = primary_size;
    out->width = width;
    out->height = height;
    out->source_width = source_width;
    out->source_texels = source_last_index + 1u;
    out->source_origin_s = source_origin_s;
    out->source_origin_t = source_origin_t;
    out->palette_base = (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
        tile->palette * 16u : 0u;
    out->materialize_s = materialize_s;
    out->materialize_t = materialize_t;
    return TRUE;
}

static s32 ndsRendererHardwareTileOriginDelta(u32 primary, u32 secondary)
{
    s32 delta = (s32)((primary - secondary) & 0x0fffu);

    if ((delta & 0x0800) != 0)
    {
        delta -= 0x1000;
    }
    return delta;
}

static s32 ndsRendererHardwareQuarterToTexel(s32 coord)
{
    if (coord < 0)
    {
        return -(((-coord) + 2) >> 2);
    }
    return (coord + 2) >> 2;
}

static u32 ndsRendererHardwareTextureAddressCoord(
    s32 coord, u32 logical_extent, u32 source_extent, u32 mode, u32 mask)
{
    s32 period;
    s32 local;
    u32 mask_extent;

    if (source_extent == 0u)
    {
        return 0u;
    }
    /* The common interior case is already in the first source/mask period.
     * Return it directly instead of paying signed divide/modulo for every
     * TEXEL1 pixel; edge, wrap and mirror coordinates retain the full path. */
    if ((coord >= 0) && ((u32)coord < source_extent) &&
        ((logical_extent == 0u) || ((u32)coord < logical_extent)) &&
        ((mask == 0u) || (mask >= 31u) ||
         ((u32)coord < (1u << mask))))
    {
        return (u32)coord;
    }
    if ((mode & NDS_RENDERER_TX_CLAMP) != 0u)
    {
        if (coord < 0)
        {
            coord = 0;
        }
        else if ((logical_extent != 0u) &&
                 ((u32)coord >= logical_extent))
        {
            coord = (s32)logical_extent - 1;
        }
    }
    if ((mask != 0u) && (mask < 31u))
    {
        mask_extent = 1u << mask;
        period = coord / (s32)mask_extent;
        local = coord % (s32)mask_extent;
        if (local < 0)
        {
            local += (s32)mask_extent;
            period--;
        }
        if (((mode & NDS_RENDERER_TX_MIRROR) != 0u) &&
            ((period & 1) != 0))
        {
            local = (s32)mask_extent - 1 - local;
        }
        return ((u32)local < source_extent) ? (u32)local :
            (source_extent - 1u);
    }
    local = coord % (s32)source_extent;
    if (local < 0)
    {
        local += (s32)source_extent;
    }
    if ((mode & NDS_RENDERER_TX_CLAMP) != 0u)
    {
        return ((u32)coord < source_extent) ? (u32)coord :
            (source_extent - 1u);
    }
    return (u32)local;
}

static u32 ndsRendererHardwareTexel1SourceIndex(
    const NDSRendererHardwareTexel1Source *source,
    s32 origin_delta_s,
    s32 origin_delta_t,
    u32 x,
    u32 y)
{
    s32 source_x;
    s32 source_y;
    u32 addressed_x;
    u32 addressed_y;
    u32 index;

    source_x = (s32)x + origin_delta_s;
    source_y = (s32)y + origin_delta_t;
    addressed_x = ndsRendererHardwareTextureAddressCoord(
        source_x, source->render_tile->width,
        source->source_extent_width, source->render_tile->cms,
        source->render_tile->masks);
    addressed_y = ndsRendererHardwareTextureAddressCoord(
        source_y, source->render_tile->height,
        source->source_extent_height, source->render_tile->cmt,
        source->render_tile->maskt);
    index = ((source->source_origin_t + addressed_y) *
             source->source_width) + source->source_origin_s + addressed_x;
    return index;
}

static u16 ndsRendererHardwareTexel1Color(
    const NDSRendererHardwareTexel1Source *source,
    const NDSRendererConfig *config,
    const u16 *palette,
    u32 palette_count,
    s32 origin_delta_s,
    s32 origin_delta_t,
    u32 x,
    u32 y)
{
    u32 index;

    if ((source == NULL) || (source->render_tile == NULL) ||
        (source->texels == NULL))
    {
        return 0u;
    }
    index = ndsRendererHardwareTexel1SourceIndex(
        source, origin_delta_s, origin_delta_t, x, y);
    return ndsRendererHardwareTextureColor(
        config, source->format, source->size, source->texels, palette,
        palette_count, source->palette_base, index, TRUE);
}

static u32 ndsRendererHardwareAlphaCoverageThreshold(u32 x, u32 y)
{
    static const u8 bayer4x4[16] = {
        0u, 8u, 2u, 10u,
        12u, 4u, 14u, 6u,
        3u, 11u, 1u, 9u,
        15u, 7u, 13u, 5u
    };

    return ((u32)bayer4x4[((y & 3u) << 2) | (x & 3u)] << 4) + 8u;
}

static u32 ndsRendererHardwareExpand5To8(u32 value)
{
    value &= 0x1fu;
    return (value << 3) | (value >> 2);
}

static u32 ndsRendererHardwareBlendTexel01Value(u16 texel0, u16 texel1,
                                                u32 fraction)
{
    u32 inverse;
    u32 red;
    u32 green;
    u32 blue;
    u32 alpha_coverage;
    u32 texel0_red;
    u32 texel0_green;
    u32 texel0_blue;
    u32 texel1_red;
    u32 texel1_green;
    u32 texel1_blue;

    if (fraction > 0xffu)
    {
        fraction = 0xffu;
    }
    inverse = 0x100u - fraction;
    texel0_red = ndsRendererHardwareExpand5To8(texel0 >> 0);
    texel0_green = ndsRendererHardwareExpand5To8(texel0 >> 5);
    texel0_blue = ndsRendererHardwareExpand5To8(texel0 >> 10);
    texel1_red = ndsRendererHardwareExpand5To8(texel1 >> 0);
    texel1_green = ndsRendererHardwareExpand5To8(texel1 >> 5);
    texel1_blue = ndsRendererHardwareExpand5To8(texel1 >> 10);
    red = (((texel0_red * inverse) + (texel1_red * fraction)) >> 8) >> 3;
    green = (((texel0_green * inverse) +
              (texel1_green * fraction)) >> 8) >> 3;
    blue = (((texel0_blue * inverse) +
             (texel1_blue * fraction)) >> 8) >> 3;
    /* G_RM_AA_TEX_EDGE2 converts the fractional alpha lerp to coverage. DS
     * direct-color textures expose A1 only, so retain the same mean coverage
     * with an ordered 4x4 decision instead of unioning both silhouettes. */
    alpha_coverage = ((((texel0 >> 15) & 1u) * 0x100u * inverse) +
                      (((texel1 >> 15) & 1u) * 0x100u * fraction)) >> 8;
    return red | (green << 5) | (blue << 10) |
        (alpha_coverage << NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT);
}

static u16 ndsRendererHardwareResolveTexel01Value(u32 value, u32 x, u32 y)
{
    u32 alpha_coverage =
        value >> NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT;
    u32 alpha = (alpha_coverage >
                 ndsRendererHardwareAlphaCoverageThreshold(x, y)) ? 1u : 0u;

    return (u16)((value & NDS_RENDERER_HW_TEXEL01_RGB_MASK) |
                 (alpha << 15));
}

static u16 ndsRendererHardwareBlendTexel01(u16 texel0, u16 texel1,
                                           u32 fraction, u32 x, u32 y)
{
    return ndsRendererHardwareResolveTexel01Value(
        ndsRendererHardwareBlendTexel01Value(texel0, texel1, fraction),
        x, y);
}

static u16 ndsRendererHardwareBlendPrimEnvTexel0(u16 texel0,
                                                 u32 primitive,
                                                 u32 environment)
{
    u32 red_weight = (texel0 >> 0) & 0x1fu;
    u32 green_weight = (texel0 >> 5) & 0x1fu;
    u32 blue_weight = (texel0 >> 10) & 0x1fu;
    u32 primitive_red = (primitive >> 27) & 0x1fu;
    u32 primitive_green = (primitive >> 19) & 0x1fu;
    u32 primitive_blue = (primitive >> 11) & 0x1fu;
    u32 environment_red = (environment >> 27) & 0x1fu;
    u32 environment_green = (environment >> 19) & 0x1fu;
    u32 environment_blue = (environment >> 11) & 0x1fu;
    u32 red;
    u32 green;
    u32 blue;

    /* DS direct-color textures expose RGB5+A1. Interpolate the captured N64
     * endpoint colours at that exact RGB5 ceiling and retain source coverage;
     * polygon alpha supplies PRIMITIVE alpha for the modulated raw form. */
    red = ((environment_red * (31u - red_weight)) +
           (primitive_red * red_weight) + 15u) / 31u;
    green = ((environment_green * (31u - green_weight)) +
             (primitive_green * green_weight) + 15u) / 31u;
    blue = ((environment_blue * (31u - blue_weight)) +
            (primitive_blue * blue_weight) + 15u) / 31u;
    return (u16)((texel0 & 0x8000u) | red | (green << 5) | (blue << 10));
}

/* rgb = PRIMITIVE, alpha = TEXEL0 (NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_
 * TEXEL0_ALPHA). The colour is flat, so the whole job is recovering the source
 * coverage that the I-format arm has already folded away.
 *
 * ndsRendererHardwareConvertI writes v = intensity >> 3 into all three RGB5
 * lanes and sets the alpha bit unconditionally, so the coverage this combine
 * wants is still readable from any one lane -- and reading it back here is why
 * that shared converter is left alone. It serves every I-format texture in the
 * game; the defect is one caller's combine being unrecognised, not the
 * converter being wrong.
 *
 * The destination is RGB555+A1, so coverage quantises to one bit. Round to
 * nearest (>= 16 of 31) rather than "any non-zero": the beam's identity is its
 * bright core, and a threshold of 1 would promote the entire faint outer ramp
 * to fully opaque and draw a blob wider than the source. The lost fade is the
 * hard-edge tradeoff the owner is being asked to judge; GL_RGB8_A5 is what
 * would recover it. */
static u16 ndsRendererHardwarePrimRgbTexel0Alpha(u16 texel0, u32 primitive)
{
    u32 coverage = texel0 & 0x1fu;
    u32 red = (primitive >> 27) & 0x1fu;
    u32 green = (primitive >> 19) & 0x1fu;
    u32 blue = (primitive >> 11) & 0x1fu;
    u16 opaque = (coverage >= 16u) ? (u16)0x8000u : (u16)0u;

    return (u16)(opaque | red | (green << 5) | (blue << 10));
}

static void __attribute__((noinline))
ndsRendererHardwareBuildTexel01Ci4Lut(
    const NDSRendererConfig *config,
    const u16 *palette,
    u32 palette_count,
    u32 palette0_base,
    u32 palette1_base,
    u32 fraction)
{
    static const u16 alpha_phase_prefix[17] = {
        0x0000u, 0x0001u, 0x0401u, 0x0405u, 0x0505u, 0x0525u,
        0x8525u, 0x85a5u, 0xa5a5u, 0xa5a7u, 0xada7u, 0xadafu,
        0xafafu, 0xafbfu, 0xefbfu, 0xefffu, 0xffffu
    };
    u16 palette0[16];
    u16 palette1[16];
    u32 index0;
    u32 index1;

    for (index0 = 0u; index0 < 16u; index0++)
    {
        palette0[index0] = ndsRendererHardwarePaletteColor(
            config, palette, palette0_base + index0, palette_count, TRUE);
        palette1[index0] = ndsRendererHardwarePaletteColor(
            config, palette, palette1_base + index0, palette_count, TRUE);
    }
    if ((sNdsRendererHardwareTexel01Ci4LutKeyValid != 0u) &&
        (sNdsRendererHardwareTexel01Ci4LutFraction == fraction) &&
        (memcmp(sNdsRendererHardwareTexel01Ci4LutPalette0,
                palette0, sizeof(palette0)) == 0) &&
        (memcmp(sNdsRendererHardwareTexel01Ci4LutPalette1,
                palette1, sizeof(palette1)) == 0))
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererProfileCi4LutReuseCount++;
#endif
        return;
    }
    memcpy(sNdsRendererHardwareTexel01Ci4LutPalette0,
           palette0, sizeof(palette0));
    memcpy(sNdsRendererHardwareTexel01Ci4LutPalette1,
           palette1, sizeof(palette1));
    sNdsRendererHardwareTexel01Ci4LutFraction = fraction;
    sNdsRendererHardwareTexel01Ci4LutKeyValid = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileCi4LutBuildCount++;
#endif
    for (index0 = 0u; index0 < 16u; index0++)
    {
        for (index1 = 0u; index1 < 16u; index1++)
        {
            u32 value = ndsRendererHardwareBlendTexel01Value(
                palette0[index0], palette1[index1], fraction);
            u32 alpha_coverage =
                value >> NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT;
            u32 alpha_prefix_count = (alpha_coverage + 7u) >> 4;
            u32 lut_index = (index0 << 4) | index1;
            u16 rgb;
            u16 alpha_phase_mask;

            if (alpha_prefix_count > 16u)
            {
                alpha_prefix_count = 16u;
            }
            rgb = (u16)(value & NDS_RENDERER_HW_TEXEL01_RGB_MASK);
            alpha_phase_mask = alpha_phase_prefix[alpha_prefix_count];
            sNdsRendererHardwareTexel01Ci4PairLut[lut_index] =
                (u32)rgb | ((u32)alpha_phase_mask << 16);
        }
    }
}

static inline u16 ndsRendererHardwareResolveTexel01Ci4Lut(
    u32 index0, u32 index1, u32 x, u32 y)
{
    u32 phase = ((y & 3u) << 2) | (x & 3u);
    u32 lut_index = (index0 << 4) | index1;
    u32 pair = sNdsRendererHardwareTexel01Ci4PairLut[lut_index];

    return (u16)((pair & NDS_RENDERER_HW_TEXEL01_RGB_MASK) |
        (((pair >> (16u + phase)) & 1u) << 15));
}

static inline u8 ndsRendererHardwareReadCi4Direct(
    const u8 *texels, u32 logical_texel_index, u32 byte_lane_xor)
{
    u8 packed = texels[(logical_texel_index >> 1) ^ byte_lane_xor];

    return ((logical_texel_index & 1u) == 0u) ?
        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static const u8 *ndsRendererHardwareGetCi4Indices(
    const u8 *source, u32 source_texels, u32 byte_lane_xor,
    const u8 *protected_indices)
{
    NDSRendererHardwareCi4IndexCacheEntry *entry;
    u32 replace_index;
    u32 i;

    if ((source == NULL) || (source_texels == 0u) ||
        (source_texels > NDS_RENDERER_HW_CI4_INDEX_CACHE_TEXELS))
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT; i++)
    {
        entry = &sNdsRendererHardwareCi4IndexCache[i];
        if ((entry->valid != 0u) &&
            (entry->source == source) &&
            (entry->source_texels == source_texels) &&
            (entry->byte_lane_xor == byte_lane_xor))
        {
            ndsRendererProfileRecordCi4IndexCacheReuse();
            return entry->indices;
        }
    }

    replace_index = sNdsRendererHardwareCi4IndexCacheNext;
    if (sNdsRendererHardwareCi4IndexCache[replace_index].indices ==
        protected_indices)
    {
        replace_index = (replace_index + 1u) &
            (NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT - 1u);
    }
    entry = &sNdsRendererHardwareCi4IndexCache[replace_index];
    sNdsRendererHardwareCi4IndexCacheNext =
        (replace_index + 1u) &
        (NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT - 1u);
    entry->valid = FALSE;
    entry->source = source;
    entry->source_texels = source_texels;
    entry->byte_lane_xor = byte_lane_xor;
    for (i = 0u; i < source_texels; i++)
    {
        entry->indices[i] = ndsRendererHardwareReadCi4Direct(
            source, i, byte_lane_xor);
    }
    entry->valid = TRUE;
    ndsRendererProfileRecordCi4IndexCacheBuild();
    return entry->indices;
}

static u32 ndsRendererHardwareBuildCi4RepresentativeMap(
    const u8 *source0, const u8 *source1, u8 *representative, u32 count)
{
    u32 i;
    u32 unique = 0u;

    /* The exact 18-bit class key plus one reserves zero as empty; the upper
     * field stores the first coordinate. At most 128 coordinates enter the
     * 256-slot table, so linear probing always reaches an empty terminator. */
    memset(sNdsRendererHardwareTexel01Ci4ClassTable, 0,
           sizeof(sNdsRendererHardwareTexel01Ci4ClassTable));
    for (i = 0u; i < count; i++)
    {
        u32 key = ((i & 3u) << 16) |
            ((u32)source1[i] << 8) | source0[i];
        u32 stored_key = key + 1u;
        u32 slot = (key * 0x9e3779b1u) >> 24;

        while (sNdsRendererHardwareTexel01Ci4ClassTable[slot] != 0u)
        {
            u32 entry = sNdsRendererHardwareTexel01Ci4ClassTable[slot];

            if ((entry & NDS_RENDERER_HW_CI4_CLASS_KEY_MASK) == stored_key)
            {
                representative[i] =
                    (u8)(entry >> NDS_RENDERER_HW_CI4_CLASS_INDEX_SHIFT);
                break;
            }
            slot = (slot + 1u) &
                (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT - 1u);
        }
        if (sNdsRendererHardwareTexel01Ci4ClassTable[slot] == 0u)
        {
            sNdsRendererHardwareTexel01Ci4ClassTable[slot] =
                (i << NDS_RENDERER_HW_CI4_CLASS_INDEX_SHIFT) | stored_key;
            representative[i] = (u8)i;
            unique++;
        }
    }
    return unique;
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
static s32 ndsRendererHardwareStageUniqueTextureRows(
    const u16 *source,
    u16 *staging,
    u32 staging_bytes,
    u8 *row_map,
    u32 row_bytes,
    u32 row_count,
    u32 *staged_bytes)
{
    u32 staging_rows;
    u32 unique_rows = 0u;
    u32 y;

    if ((source == NULL) || (staging == NULL) || (row_map == NULL) ||
        (staged_bytes == NULL) || (row_bytes == 0u) || (row_count == 0u) ||
        (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        ((row_bytes & (sizeof(u16) - 1u)) != 0u))
    {
        return FALSE;
    }
    staging_rows = staging_bytes / row_bytes;
    if ((staging_rows == 0u) ||
        (staging_rows > NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS))
    {
        return FALSE;
    }

    if (sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid == 0u)
    {
        return FALSE;
    }
    for (y = 0u; y < row_count; y++)
    {
        u32 representative =
            sNdsRendererHardwareTexel01Ci4RepresentativeT[y];

        if ((representative >= row_count) || (representative > y))
        {
            return FALSE;
        }
        if (representative == y)
        {
            if (unique_rows >= staging_rows)
            {
                return FALSE;
            }
            memcpy((u8 *)staging + (unique_rows * row_bytes),
                   (const u8 *)source + (y * row_bytes), row_bytes);
            row_map[y] = (u8)unique_rows;
            unique_rows++;
        }
        else
        {
            row_map[y] = row_map[representative];
        }
    }
    *staged_bytes = unique_rows * row_bytes;
    return TRUE;
}
#endif

static s32 NDS_TASK82_EVICTED_HOT_CODE
ndsRendererHardwareConvertTexel01Ci4Direct(
    const NDSRendererConfig *config,
    const u8 *texels0,
    u32 source0_texels,
    u32 source0_width,
    u32 source0_origin_s,
    u32 source0_origin_t,
    const NDSRendererTileState *render_tile0,
    s32 materialize0_s,
    s32 materialize0_t,
    const NDSRendererHardwareTexel1Source *source1,
    s32 origin1_delta_s,
    s32 origin1_delta_t,
    u32 width,
    u32 height,
    u32 upload_width,
    u8 *compact_row_map,
    u32 *compact_staged_bytes,
    u32 *green_texels,
    u32 *nonwhite_texels)
{
    u32 byte_lane_xor =
        (ndsRendererTextureDataLayout(config) ==
         NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ? 3u : 0u;
    const NDSRendererTileState *render_tile1 = source1->render_tile;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    const u8 *indices0 = ndsRendererHardwareGetCi4Indices(
        texels0, source0_texels, byte_lane_xor, NULL);
    const u8 *indices1 = ndsRendererHardwareGetCi4Indices(
        source1->texels, source1->source_texels, byte_lane_xor, indices0);
#else
    (void)source0_texels;
#endif
    u32 x;
    u32 y;

#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid = FALSE;
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    (void)green_texels;
    (void)nonwhite_texels;
#else
    (void)compact_row_map;
    (void)compact_staged_bytes;
#endif
    /* Animated tile origins can wrap or mirror TEXEL1. Resolve those exact
     * addressing rules once per S coordinate, then reuse them for every row. */
    for (x = 0u; x < width; x++)
    {
        sNdsRendererHardwareTexel01Ci4Source0S[x] = (u8)(
            (materialize0_s != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    x, render_tile0->cms, render_tile0->masks) : x);
        sNdsRendererHardwareTexel01Ci4Source1S[x] = (u8)
            ndsRendererHardwareTextureAddressCoord(
                (s32)x + origin1_delta_s, render_tile1->width,
                source1->source_extent_width, render_tile1->cms,
                render_tile1->masks);
    }
    for (y = 0u; y < height; y++)
    {
        sNdsRendererHardwareTexel01Ci4Source0T[y] = (u8)(
            (materialize0_t != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    y, render_tile0->cmt, render_tile0->maskt) : y);
        sNdsRendererHardwareTexel01Ci4Source1T[y] = (u8)
            ndsRendererHardwareTextureAddressCoord(
                (s32)y + origin1_delta_t, render_tile1->height,
                source1->source_extent_height, render_tile1->cmt,
                render_tile1->maskt);
    }

#if NDS_RENDERER_PROFILE_LEVEL < 2
    if ((indices0 != NULL) && (indices1 != NULL))
    {
        u32 texels = width * height;

        /* Large clamped/masked animated tiles revisit the same pair of source
         * coordinates and ordered-coverage phase many times. Index the first
         * exact representative of each separable S/T class; forward X expansion
         * reads only earlier representatives. Cold output copies repeated rows
         * in reverse Y order, while warm staging records their exact row map. */
        if (texels >= 4096u)
        {
            u32 unique_s = 0u;
            u32 unique_t = 0u;
            u32 unique_texels;

            unique_s = ndsRendererHardwareBuildCi4RepresentativeMap(
                sNdsRendererHardwareTexel01Ci4Source0S,
                sNdsRendererHardwareTexel01Ci4Source1S,
                sNdsRendererHardwareTexel01Ci4RepresentativeS, width);
            unique_t = ndsRendererHardwareBuildCi4RepresentativeMap(
                sNdsRendererHardwareTexel01Ci4Source0T,
                sNdsRendererHardwareTexel01Ci4Source1T,
                sNdsRendererHardwareTexel01Ci4RepresentativeT, height);

            unique_texels = unique_s * unique_t;
            if ((unique_texels * 2u) <= texels)
            {
                s32 compact_output =
                    (compact_row_map != NULL) &&
                    (compact_staged_bytes != NULL) &&
                    (width == upload_width) &&
                    (height <= NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) &&
                    (unique_t <=
                     NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS) &&
                    ((unique_t * upload_width * sizeof(u16)) <=
                     sizeof(sNdsRendererHardwareTextureRefreshLarge));
                u16 *destination = (compact_output != FALSE) ?
                    sNdsRendererHardwareTextureRefreshLarge :
                    sNdsRendererHardwareTextureScratch;
                u32 unique_row = 0u;

                /* Warm animated-water refreshes already upload a compact set
                 * of unique rows during VBlank.  Produce that exact compact
                 * representation here instead of expanding repeated rows into
                 * the 32 KiB scratch arena and immediately copying the unique
                 * rows back into the 16 KiB refresh staging buffer.  Cold
                 * uploads use the same loop with the full scratch destination. */
                for (y = 0u; y < height; y++)
                {
                    u32 representative_y =
                        sNdsRendererHardwareTexel01Ci4RepresentativeT[y];
                    u32 source0_row;
                    u32 source1_row;
                    u32 dst_index;

                    if (representative_y != y)
                    {
                        if (compact_output != FALSE)
                        {
                            compact_row_map[y] =
                                compact_row_map[representative_y];
                        }
                        continue;
                    }
                    if (compact_output != FALSE)
                    {
                        compact_row_map[y] = (u8)unique_row;
                    }
                    source0_row = ((source0_origin_t +
                        sNdsRendererHardwareTexel01Ci4Source0T[y]) *
                        source0_width) + source0_origin_s;
                    source1_row = ((source1->source_origin_t +
                        sNdsRendererHardwareTexel01Ci4Source1T[y]) *
                        source1->source_width) + source1->source_origin_s;
                    dst_index = ((compact_output != FALSE) ?
                        unique_row : y) * upload_width;

                    for (x = 0u; x < width; x++)
                    {
                        u32 index0;
                        u32 index1;
                        u32 representative_x =
                            sNdsRendererHardwareTexel01Ci4RepresentativeS[x];

                        if (representative_x != x)
                        {
                            destination[dst_index + x] =
                                destination[dst_index + representative_x];
                            continue;
                        }
                        index0 = indices0[source0_row +
                            sNdsRendererHardwareTexel01Ci4Source0S[x]];
                        index1 = indices1[source1_row +
                            sNdsRendererHardwareTexel01Ci4Source1S[x]];
                        destination[dst_index + x] =
                            ndsRendererHardwareResolveTexel01Ci4Lut(
                                index0, index1, x, y);
                    }
                    unique_row++;
                }
                if (compact_output != FALSE)
                {
                    *compact_staged_bytes =
                        unique_row * upload_width * sizeof(u16);
                }
                else
                {
                    for (y = height; y != 0u; )
                    {
                        u32 representative_y;

                        y--;
                        representative_y =
                            sNdsRendererHardwareTexel01Ci4RepresentativeT[y];
                        if (representative_y != y)
                        {
                            memcpy(&sNdsRendererHardwareTextureScratch[
                                       y * upload_width],
                                   &sNdsRendererHardwareTextureScratch[
                                       representative_y * upload_width],
                                   width * sizeof(u16));
                        }
                    }
                }
                ndsRendererProfileRecordCi4RepresentativeReuse(
                    unique_texels, texels - unique_texels);
                sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid = TRUE;
                return compact_output;
            }
        }

        for (y = 0u; y < height; y++)
        {
            u32 source0_y = (materialize0_t != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    y, render_tile0->cmt, render_tile0->maskt) : y;
            u32 source1_y = ndsRendererHardwareTextureAddressCoord(
                (s32)y + origin1_delta_t, render_tile1->height,
                source1->source_extent_height, render_tile1->cmt,
                render_tile1->maskt);
            u32 source0_row =
                ((source0_origin_t + source0_y) * source0_width) +
                source0_origin_s;
            u32 source1_row =
                ((source1->source_origin_t + source1_y) *
                 source1->source_width) + source1->source_origin_s;
            u32 dst_index = y * upload_width;

            for (x = 0u; x < width; x++)
            {
                u32 index0 = indices0[source0_row +
                    sNdsRendererHardwareTexel01Ci4Source0S[x]];
                u32 index1 = indices1[source1_row +
                    sNdsRendererHardwareTexel01Ci4Source1S[x]];

                sNdsRendererHardwareTextureScratch[dst_index + x] =
                    ndsRendererHardwareResolveTexel01Ci4Lut(
                        index0, index1, x, y);
            }
        }
        return FALSE;
    }
#endif
    for (y = 0u; y < height; y++)
    {
        u32 source0_y = (materialize0_t != FALSE) ?
            ndsRendererHardwareTextureMaskedAddress(
                y, render_tile0->cmt, render_tile0->maskt) : y;
        u32 source1_y = ndsRendererHardwareTextureAddressCoord(
            (s32)y + origin1_delta_t, render_tile1->height,
            source1->source_extent_height, render_tile1->cmt,
            render_tile1->maskt);
        u32 source0_row =
            ((source0_origin_t + source0_y) * source0_width) +
            source0_origin_s;
        u32 source1_row =
            ((source1->source_origin_t + source1_y) *
             source1->source_width) + source1->source_origin_s;
        u32 dst_index = y * upload_width;

        for (x = 0u; x < width; x++)
        {
            u32 source0_index = source0_row +
                sNdsRendererHardwareTexel01Ci4Source0S[x];
            u32 source1_index = source1_row +
                sNdsRendererHardwareTexel01Ci4Source1S[x];
            u32 index0 = ndsRendererHardwareReadCi4Direct(
                texels0, source0_index, byte_lane_xor);
            u32 index1 = ndsRendererHardwareReadCi4Direct(
                source1->texels, source1_index, byte_lane_xor);
            u16 color = ndsRendererHardwareResolveTexel01Ci4Lut(
                index0, index1, x, y);

            sNdsRendererHardwareTextureScratch[dst_index + x] = color;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            {
                u16 reference = ndsRendererHardwareBlendTexel01(
                    sNdsRendererHardwareTexel01Ci4LutPalette0[index0],
                    sNdsRendererHardwareTexel01Ci4LutPalette1[index1],
                    sNdsRendererHardwareTexel01Ci4LutFraction, x, y);

                gNdsRendererProfileTexturePairOracleChecks++;
                if (color != reference)
                {
                    gNdsRendererProfileTexturePairOracleMismatches++;
                }
            }
            ndsRendererProfileTexturePixel(
                color, green_texels, nonwhite_texels);
#endif
        }
    }
    return FALSE;
}

/* THE REBIRTH-HALO BEAM'S EDGE, AND WHY IT GETS A TEXTURE OF ITS OWN.
 *
 * Its list (85.vpk0.bin 0x28a0) sets G_SETCOMBINE FCFFFFFF FFFDF2F9 -- rgb is
 * constant PRIMITIVE, alpha is pure TEXEL0 -- over the I4 tile declared at
 * 0x28b8 (fmt=I, siz=4b, 8x16, clamp on both axes) with PRIM white and opaque
 * at 0x28a8. So the source image carries COVERAGE and nothing else, in sixteen
 * levels, and the generic cache uploads every entry as GL_RGBA: RGB555 plus a
 * SINGLE alpha bit. ndsRendererHardwarePrimRgbTexel0Alpha therefore had to
 * threshold sixteen levels down to two, and the beam drew hard-edged where the
 * source fades.
 *
 * GL_RGB8_A5 is one byte a texel -- three index bits and FIVE alpha bits. The
 * sixteen source levels land in thirty-two slots injectively, so this recovers
 * the ramp exactly rather than approximating it, and the alpha stored here is
 * bit-identical to the 5-bit coverage the RGB555 bake already computed before
 * discarding it: (n * 0x11) >> 3 equals round(n * 31 / 15) at all sixteen
 * inputs. The combine can show one colour, so one palette holds PRIM.
 *
 * A DEDICATED NAME, NOT A PER-ENTRY CACHE FORMAT. The texture cache uploads one
 * format for all of its entries; teaching it per-entry formats to serve a single
 * 128-texel surface would put four just-closed rows at risk for a cosmetic one.
 * This costs one name and consumes no cache entry -- it RETURNS one to a pool
 * the post-KO scene already over-subscribes, and halves this surface's VRAM.
 *
 * The palette rides the texture NAME, not the bind. libnds documents
 * glColorTableEXT as setting the palette on the currently bound texture and
 * glDeleteTextures as deleting "associated palettes", and glAssignColorTable
 * exists precisely to share one between names. So it is attached once at
 * prepare, exactly as the cloud atlases and particle sheets already do, and the
 * bind path needs nothing. */
typedef struct NDSRendererPrimRgbTexel0AlphaFill
{
    const NDSRendererConfig *config;
    const u8 *texels;
    u32 source_width;
    u32 source_origin_s;
    u32 source_origin_t;
    u32 width;
    u32 height;
    u32 upload_width;
    u32 upload_height;
} NDSRendererPrimRgbTexel0AlphaFill;

static s32 ndsRendererHardwarePrimRgbTexel0AlphaFill(
    u8 *pixels, u32 bytes, void *user_data)
{
    const NDSRendererPrimRgbTexel0AlphaFill *fill =
        (const NDSRendererPrimRgbTexel0AlphaFill *)user_data;
    u32 x;
    u32 y;

    if ((pixels == NULL) || (fill == NULL) || (fill->texels == NULL) ||
        (fill->upload_width == 0u) || (fill->upload_height == 0u) ||
        (fill->width > fill->upload_width) ||
        (fill->height > fill->upload_height) ||
        (bytes < (fill->upload_width * fill->upload_height)))
    {
        return FALSE;
    }
    /* The power-of-two tail lies outside the tile and must read as absent.
     * Left uncleared it would be palette entry 0 at full coverage -- an opaque
     * block of PRIM exactly where the old one-bit threshold used to put one. */
    memset(pixels, 0, bytes);
    for (y = 0u; y < fill->height; y++)
    {
        for (x = 0u; x < fill->width; x++)
        {
            u32 index = ((fill->source_origin_t + y) * fill->source_width) +
                fill->source_origin_s + x;
            u32 intensity = ndsRendererReadTexturePackedNibble(
                fill->config, fill->texels, index,
                NDS_RENDERER_HW_TEXTURE_FMT_I16,
                NDS_RENDERER_HW_TEXTURE_SIZ_4B);

            /* alpha5 in bits 3-7, palette index in bits 0-2. */
            pixels[(y * fill->upload_width) + x] =
                (u8)(((intensity * 0x11u) >> 3) << 3);
        }
    }
    return TRUE;
}

static u32 ndsRendererHardwarePrimRgbTexel0AlphaExtentOf(u32 upload_width,
                                                         u32 upload_height)
{
    return (upload_width << 16) | (upload_height & 0xffffu);
}

static s32 ndsRendererHardwarePrimRgbTexel0AlphaResident(
    const NDSRendererStats *stats, u32 primary_image,
    u32 upload_width, u32 upload_height)
{
    return ((sNdsRendererHardwarePrimRgbTexel0AlphaName != 0u) &&
            (sNdsRendererHardwarePrimRgbTexel0AlphaImage == primary_image) &&
            (sNdsRendererHardwarePrimRgbTexel0AlphaExtent ==
                 ndsRendererHardwarePrimRgbTexel0AlphaExtentOf(
                     upload_width, upload_height)) &&
            (sNdsRendererHardwarePrimRgbTexel0AlphaPrim ==
                 (stats->prim_color & 0xffffff00u))) ? TRUE : FALSE;
}

static s32 ndsRendererHardwarePreparePrimRgbTexel0AlphaTexture(
    const NDSRendererStats *stats, const NDSRendererConfig *config,
    const u8 *texels_src, u32 primary_image, u32 source_width,
    u32 source_origin_s, u32 source_origin_t, u32 width, u32 height,
    u32 upload_width, u32 upload_height)
{
    NDSRendererPrimRgbTexel0AlphaFill fill;
    u32 prim = stats->prim_color & 0xffffff00u;
    u16 color = (u16)(((prim >> 27) & 0x1fu) |
                      (((prim >> 19) & 0x1fu) << 5) |
                      (((prim >> 11) & 0x1fu) << 10));
    u16 palette[8];
    u32 i;

    fill.config = config;
    fill.texels = texels_src;
    fill.source_width = source_width;
    fill.source_origin_s = source_origin_s;
    fill.source_origin_t = source_origin_t;
    fill.width = width;
    fill.height = height;
    fill.upload_width = upload_width;
    fill.upload_height = upload_height;
    /* Every index resolves to PRIM. The combine has no second colour, and the
     * fill only ever writes index 0, so the remaining seven exist to make a
     * stray index harmless rather than black. */
    for (i = 0u; i < 8u; i++)
    {
        palette[i] = color;
    }
    if (ndsRendererHardwarePrepareIFCommonCloudAtlas(
            upload_width, upload_height, palette,
            ndsRendererHardwarePrimRgbTexel0AlphaFill, &fill,
            &sNdsRendererHardwarePrimRgbTexel0AlphaName) == FALSE)
    {
        sNdsRendererHardwarePrimRgbTexel0AlphaImage = 0u;
        sNdsRendererHardwarePrimRgbTexel0AlphaExtent = 0u;
        sNdsRendererHardwarePrimRgbTexel0AlphaPrim = 0u;
        return FALSE;
    }
    sNdsRendererHardwarePrimRgbTexel0AlphaImage = primary_image;
    sNdsRendererHardwarePrimRgbTexel0AlphaExtent =
        ndsRendererHardwarePrimRgbTexel0AlphaExtentOf(upload_width,
                                                      upload_height);
    sNdsRendererHardwarePrimRgbTexel0AlphaPrim = prim;
    gNdsRendererPrimRgbTexel0AlphaPrepareCount++;
    return TRUE;
}

static void ndsRendererHardwareBindPrimRgbTexel0AlphaTexture(
    NDSRendererStats *stats, u32 params, u32 format, u32 width, u32 height)
{
    ndsRendererHardwareBindTextureName(
        stats, sNdsRendererHardwarePrimRgbTexel0AlphaName);
    ndsRendererHardwareApplyTextureParams(
        ndsRendererHardwareMergeTextureParams(params));
    /* Not a cache entry, so nothing may be left pointing into the cache. */
    sNdsRendererHardwareActiveTextureEntry = NULL;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = format;
    stats->hardware_texture_width = width;
    stats->hardware_texture_height = height;
    gNdsRendererPrimRgbTexel0AlphaBindCount++;
}

static s32 ndsRendererHardwareResolveOrBindTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    NDSRendererHardwareResolvedTexture *resolved,
    s32 allow_stage_source_frame)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 texture_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 convert_start;
    u32 upload_start;
#endif
#if NDS_TICK_HUD
    /* OUTSIDE the PROFILE_LEVEL guard on purpose. Its two uses are guarded on
     * NDS_TICK_HUD alone, and the tick-HUD build is PROFILE_LEVEL 0 -- nesting
     * this declaration inside the level-1 block compiled it out from under
     * them. */
    u32 tickhud_upload_mark = 0u;
#endif
    NDSRendererHardwareTextureKey key;
    NDSRendererHardwareTextureCacheEntry *entry;
    NDSRendererHardwareTextureCacheEntry *fraction_entry;
    NDSRendererHardwareTexel1Source texel1_source;
    const NDSRendererTextureLoadState *primary_load;
    const u8 *texels_src;
    const u16 *tlut_src;
    u32 width;
    u32 height;
    u32 format;
    u32 size;
    u32 upload_width;
    u32 upload_height;
    u32 upload_bytes;
    u32 resident_upload_bytes;
    u32 staged_bytes;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 staged_row_bytes = 0u;
    u32 staged_row_count = 0u;
    u8 staged_row_map[NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
#endif
    u32 texels;
    u32 bytes;
    u32 loaded_bytes;
    u32 source_extent_width;
    u32 source_extent_height;
    u32 source_width;
    u32 source_read_width;
    u32 source_read_height;
    u32 source_origin_s;
    u32 source_origin_t;
    u32 source_last_index;
    u32 source_texels;
    u32 source_bytes;
    u32 source_physical_bytes;
    u32 palette_base;
    u32 tlut_physical_bytes;
    u32 params;
    u32 key_hash;
    u32 render_tile_index;
    u32 render_tile_flags;
    u32 upload_attempts;
    u32 primary_image;
    u32 primary_image_format;
    u32 primary_image_size;
    u32 primary_image_width;
    u32 primary_load_kind;
    u32 primary_load_tile;
    u32 primary_load_uls;
    u32 primary_load_ult;
    u32 primary_load_lrs;
    u32 primary_load_dxt;
    u32 primary_load_texels;
    u32 prim_env_blend_mode;
    s32 materialize_s;
    s32 materialize_t;
    s32 wants_texel1;
    s32 use_texel1 = FALSE;
    s32 use_texel1_ci4_lut = FALSE;
    s32 use_texel1_ci4_direct = FALSE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    s32 queue_texture_refresh = FALSE;
    s32 compact_row_output = FALSE;
#endif
    s32 texel1_origin_delta_s = 0;
    s32 texel1_origin_delta_t = 0;
    const NDSRendererTileState *render_tile;
    u32 green_texels = 0u;
    u32 nonwhite_texels = 0u;
    u16 resident_palette[16];
    u32 resident_palette_entries = 0u;
    u32 resident_color0_transparent = FALSE;
    GL_TEXTURE_TYPE_ENUM resident_texture_type = GL_RGBA;
    int size_x;
    int size_y;
    u32 x;
    u32 y;
    u16 *upload_buffer = sNdsRendererHardwareTextureScratch;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererSemanticLastTextureKeyHash = 0u;
    sNdsRendererSemanticLastTextureParams = 0u;
#endif
    if (stats == NULL)
    {
        return FALSE;
    }
    /* Hierarchy preflight must remain a pure resident lookup.  The stage-site
     * shortcut performs the live bind/apply side effects, so only the normal
     * execution path may take it. */
    if ((resolved == NULL) &&
        (ndsRendererStageTextureSiteTryBind(stats, state, config) != FALSE))
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
        return TRUE;
    }
#if NDS_TASK107_RENDER_STATE_CENSUS
    ndsRendererTask107RecordTextureSync(stats, NDS_TASK107_SYNC_RESOLVE);
#endif
    ndsRendererSyncTextureTile(stats);
    render_tile_index = ndsRendererActiveTextureTile(stats);
    render_tile = &stats->texture_tiles[render_tile_index];
    wants_texel1 = ndsRendererHardwareUsesTexel01Lerp(stats);
    prim_env_blend_mode =
        ndsRendererHardwarePrimEnvTexel0BlendMode(stats);
    primary_load = NULL;
    primary_image = stats->texture_image;
    primary_image_format = stats->texture_format;
    primary_image_size = stats->texture_size;
    primary_image_width = stats->texture_image_width;
    primary_load_kind = stats->texture_load_kind;
    primary_load_tile = stats->texture_load_tile;
    primary_load_uls = stats->texture_load_block_uls;
    primary_load_ult = stats->texture_load_block_ult;
    primary_load_lrs = stats->texture_load_block_lrs;
    primary_load_dxt = stats->texture_load_block_dxt;
    primary_load_texels = stats->texture_load_texels;
    if (wants_texel1 != FALSE)
    {
        /* A two-texture combiner consumes the images resident at each render
         * tile's TMEM address. SETTIMG is mutable, so the last global image is
         * not necessarily TEXEL0 after both LOADBLOCK commands have run. */
        primary_load = ndsRendererHardwareFindTextureLoadForTmem(
            stats, render_tile->tmem);
        if ((primary_load == NULL) || (primary_load->image == 0u) ||
            (primary_load->load_texels == 0u))
        {
            ndsRendererProfileRecordTexel1Reject();
            ndsRendererProfileRecordTexel1RejectReason(
                NDS_RENDERER_HW_TEXEL1_REJECT_LOAD_STATE);
            ndsRendererHardwareRejectTexture(
                stats, stats->texture_format, stats->texture_size,
                NDS_RENDERER_HW_TEXREJECT_MISSING_STATE);
            return FALSE;
        }
        primary_image = primary_load->image;
        primary_image_format = primary_load->image_format;
        primary_image_size = primary_load->image_size;
        primary_image_width = primary_load->image_width;
        primary_load_kind = primary_load->load_kind;
        primary_load_tile = primary_load->load_tile;
        primary_load_uls = primary_load->load_uls;
        primary_load_ult = primary_load->load_ult;
        primary_load_lrs = primary_load->load_lrs;
        primary_load_dxt = primary_load->load_dxt;
        primary_load_texels = primary_load->load_texels;
    }
    render_tile_flags = 0u;
    if (render_tile->set_seen != 0u)
    {
        render_tile_flags |= NDS_RENDERER_TILE_RENDER_SEEN |
            render_tile->flags;
    }
    if (stats->texture_tiles[NDS_RENDERER_LOAD_TILE].set_seen != 0u)
    {
        render_tile_flags |= NDS_RENDERER_TILE_LOAD_SEEN;
    }
    if ((((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_ON) == 0u) &&
         (ndsRendererHardwareTextureImplicitStateOn(stats) == FALSE)) ||
        (primary_image == 0u) ||
        (render_tile->line == 0u) ||
        (primary_load_texels == 0u))
    {
        ndsRendererHardwareRejectTexture(
            stats, stats->texture_format, stats->texture_size,
            NDS_RENDERER_HW_TEXREJECT_MISSING_STATE);
        return FALSE;
    }

    if (render_tile->set_seen != 0u)
    {
        format = render_tile->format;
        size = render_tile->size;
    }
    else
    {
        format = stats->texture_format;
        size = stats->texture_size;
    }

    if ((format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        ((size != NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
         (size != NDS_RENDERER_HW_TEXTURE_SIZ_8B)))
    {
        if (stats->texture_tlut_count <= 16u)
        {
            size = NDS_RENDERER_HW_TEXTURE_SIZ_4B;
        }
        else if (stats->texture_tlut_count <= 256u)
        {
            size = NDS_RENDERER_HW_TEXTURE_SIZ_8B;
        }
        else
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_CI_SIZE);
            return FALSE;
        }
    }
    if ((format != NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_RGBA16) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_IA) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_I16))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_UNSUPPORTED_FORMAT);
        return FALSE;
    }

    loaded_bytes = (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
        primary_load_texels * sizeof(u32) :
        primary_load_texels * sizeof(u16);
    width = render_tile->width;
    height = render_tile->height;
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        (ndsRendererHardwareTextureSourceBytes(format, size, width * height) >
         loaded_bytes))
    {
        width = ndsRendererHardwareTextureLinePixels(
            size, render_tile->line);
        texels = primary_load_texels * sizeof(u16);
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            texels *= 2u;
        }
        else if ((size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ||
                 (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B))
        {
            texels /= 2u;
        }
        height = (width != 0u) ? texels / width : 0u;
    }
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_DIMENSIONS);
        return FALSE;
    }
    source_extent_width = width;
    source_extent_height = height;
    materialize_s = ndsRendererHardwareTextureMaterializesMaskedClamp(
        render_tile->cms, render_tile->masks, source_extent_width,
        render_tile->width);
    materialize_t = ndsRendererHardwareTextureMaterializesMaskedClamp(
        render_tile->cmt, render_tile->maskt, source_extent_height,
        render_tile->height);
    if (materialize_s != FALSE)
    {
        width = render_tile->width;
    }
    if (materialize_t != FALSE)
    {
        height = render_tile->height;
    }

    upload_width = ndsRendererHardwareTextureNextPow2(width);
    upload_height = ndsRendererHardwareTextureNextPow2(height);
    if ((upload_width < width) || (upload_height < height) ||
        (ndsRendererHardwareTextureSizeEnum(upload_width, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(upload_height, &size_y) == FALSE))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_UPLOAD_SIZE);
        return FALSE;
    }

    /* Steady state for the rebirth-halo beam. Its dedicated A5I3 texture is
     * already resident, so answer here rather than rebuilding a ~59-field key
     * and taking a cache lookup that must miss: this surface never occupies a
     * cache entry.
     *
     * LIVE BINDS ONLY. The hierarchy preflight hands back an entry pointer its
     * caller revalidates and re-resolves when NULL, and this surface has no
     * entry to give. The beam is a generic effect list and reaches the resolver
     * through ndsRendererHardwareBindTexture, so preflight is not expected here
     * at all; if it ever arrives it falls through to the generic RGBA path and
     * gets the previous behaviour instead of a reject. */
    if ((resolved == NULL) &&
        (prim_env_blend_mode ==
             NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA) &&
        (format == NDS_RENDERER_HW_TEXTURE_FMT_I16) &&
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (ndsRendererHardwarePrimRgbTexel0AlphaResident(
             stats, primary_image, upload_width, upload_height) != FALSE))
    {
        ndsRendererHardwareBindPrimRgbTexel0AlphaTexture(
            stats,
            ndsRendererHardwareTextureParams(stats, render_tile, upload_width,
                                             upload_height),
            format, width, height);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
        return TRUE;
    }

    if (primary_load_kind == NDS_RENDERER_TEXTURE_LOADTILE)
    {
        source_origin_s = primary_load_uls >> 2;
        source_origin_t = primary_load_ult >> 2;
        source_width = ndsRendererHardwareTextureSourceWidthPixels(
            size, primary_image_size, primary_image_width);
    }
    else
    {
        u32 dxt = primary_load_dxt;

        source_origin_s = 0u;
        source_origin_t = 0u;
        source_width = source_extent_width;
        if (dxt != 0u)
        {
            u32 qwords = (NDS_RENDERER_G_TX_DXT_ONE + dxt - 1u) / dxt;

            /* BattleShip gbi.h:3291,3309-3317 encodes DXT as the rounded
             * 1.11 reciprocal of 64-bit source words per row. LOADBLOCK's
             * SETTIMG width is one, so DXT owns the DRAM row stride. */
            source_width = ndsRendererHardwareTextureLinePixels(size, qwords);
        }
    }
    source_read_width = (materialize_s != FALSE) ?
        (1u << render_tile->masks) : width;
    source_read_height = (materialize_t != FALSE) ?
        (1u << render_tile->maskt) : height;
    if ((source_width == 0u) ||
        (source_origin_s >= source_width) ||
        (source_read_width > (source_width - source_origin_s)))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_RANGE);
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererRecordTextureLaneUse(config, format, size);
#endif
    source_last_index =
        ((source_origin_t + source_read_height - 1u) * source_width) +
        source_origin_s + source_read_width - 1u;
    source_texels = source_last_index + 1u;
    source_bytes = ndsRendererHardwareTextureSourceBytes(format, size,
                                                        source_texels);
    if (source_bytes == 0u)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }
    source_physical_bytes = ndsRendererTexturePhysicalByteSpan(
        ndsRendererTextureDataLayout(config), source_bytes);
    if (source_physical_bytes < source_bytes)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }

    if (wants_texel1 != FALSE)
    {
        if (ndsRendererHardwarePrepareTexel1Source(
                stats, config, format, size, width, height,
                &texel1_source) != FALSE)
        {
            use_texel1 = TRUE;
            texel1_origin_delta_s = ndsRendererHardwareQuarterToTexel(
                ndsRendererHardwareTileOriginDelta(
                    render_tile->uls, texel1_source.render_tile->uls));
            texel1_origin_delta_t = ndsRendererHardwareQuarterToTexel(
                ndsRendererHardwareTileOriginDelta(
                    render_tile->ult, texel1_source.render_tile->ult));
            ndsRendererProfileRecordTexel1Composite();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileTexel1LastFraction =
                stats->prim_lod_fraction;
            gNdsRendererProfileTexel1LastImage0 = primary_image;
            gNdsRendererProfileTexel1LastImage1 =
                texel1_source.load->image;
#endif
        }
        else
        {
            ndsRendererProfileRecordTexel1Reject();
        }
    }

    memset(&key, 0, sizeof(key));
    key.image = primary_image;
    key.image_format = primary_image_format;
    key.image_size = primary_image_size;
    key.image_width = primary_image_width;
    key.tlut_image = stats->texture_tlut_image;
    key.tlut_count = stats->texture_tlut_count;
    key.data_layout = (config != NULL) ?
        (u32)config->texture_data_layout :
        (u32)NDS_RENDERER_TEXTURE_DATA_NATIVE;
    key.format = format;
    key.size = size;
    key.width = width;
    key.height = height;
    key.render_tile = render_tile_index;
    key.render_tmem = render_tile->tmem;
    key.render_palette = render_tile->palette;
    key.render_tile_cms = render_tile->cms;
    key.render_tile_cmt = render_tile->cmt;
    key.render_tile_masks = render_tile->masks;
    key.render_tile_maskt = render_tile->maskt;
    key.render_tile_shifts = render_tile->shifts;
    key.render_tile_shiftt = render_tile->shiftt;
    key.load_tile = primary_load_tile;
    key.load_uls = primary_load_uls;
    key.load_ult = primary_load_ult;
    key.load_lrs = primary_load_lrs;
    key.load_dxt = primary_load_dxt;
    key.load_texels = primary_load_texels;
    key.tile_uls = render_tile->uls;
    key.tile_ult = render_tile->ult;
    key.tile_lrs = render_tile->lrs;
    key.tile_lrt = render_tile->lrt;
    key.line = render_tile->line;
    key.flags = render_tile_flags | (primary_load_kind << 8);
    if (use_texel1 != FALSE)
    {
        const NDSRendererTextureLoadState *load = texel1_source.load;
        const NDSRendererTileState *tile = texel1_source.render_tile;

        key.texel1_image = load->image;
        key.texel1_image_format = load->image_format;
        key.texel1_image_size = load->image_size;
        key.texel1_image_width = load->image_width;
        key.texel1_load_kind = load->load_kind;
        key.texel1_render_tmem = tile->tmem;
        key.texel1_render_line = tile->line;
        key.texel1_render_palette = tile->palette;
        key.texel1_render_tile_cms = tile->cms;
        key.texel1_render_tile_cmt = tile->cmt;
        key.texel1_render_tile_masks = tile->masks;
        key.texel1_render_tile_maskt = tile->maskt;
        key.texel1_render_tile_shifts = tile->shifts;
        key.texel1_render_tile_shiftt = tile->shiftt;
        key.texel1_load_tile = load->load_tile;
        key.texel1_load_uls = load->load_uls;
        key.texel1_load_ult = load->load_ult;
        key.texel1_load_lrs = load->load_lrs;
        key.texel1_load_dxt = load->load_dxt;
        key.texel1_load_texels = load->load_texels;
        key.texel1_tile_uls = tile->uls;
        key.texel1_tile_ult = tile->ult;
        key.texel1_tile_lrs = tile->lrs;
        key.texel1_tile_lrt = tile->lrt;
        key.prim_lod_fraction = stats->prim_lod_fraction;
        key.combine_w0 = stats->texture_combine_w0;
        key.combine_w1 = stats->texture_combine_w1;
    }
    else if (prim_env_blend_mode ==
             NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA)
    {
        /* ITS OWN BIT, BECAUSE THE EXISTING ONE DOES NOT SEPARATE THE BAKES.
         * Modes 1 and 2 both bake through ndsRendererHardwareBlendPrimEnvTexel0,
         * so one flag was enough to tell "baked" from "raw". This mode bakes
         * differently, so sharing the flag would let a beam and a BLENDPE
         * surface with equal prim/env collide on one cache entry and serve
         * whichever was converted first.
         *
         * PRIM RGB is already keyed by the assignment below and must stay
         * keyed: it is now the texel colour itself, not a blend endpoint. ENV
         * is dropped rather than copied -- this combine never reads it, and
         * keying on it would force a re-bake every time an unrelated list
         * moved the env colour. */
        key.flags |= NDS_RENDERER_HW_TEXTURE_KEY_PRIM_RGB_TEXEL0_ALPHA;
        key.combine_w0 = stats->prim_color & 0xffffff00u;
        key.combine_w1 = 0u;
    }
    else if (prim_env_blend_mode != NDS_RENDERER_PRIM_ENV_BLEND_NONE)
    {
        /* TEXEL1 and this bake are mutually exclusive. Reuse the key's
         * variant tail without growing its 236-byte DS cache footprint. Alpha
         * is excluded because it remains live polygon state, not texel RGB. */
        key.flags |= NDS_RENDERER_HW_TEXTURE_KEY_PRIM_ENV_BLEND;
        key.combine_w0 = stats->prim_color & 0xffffff00u;
        key.combine_w1 = stats->env_color & 0xffffff00u;
    }

#if NDS_RENDERER_PROFILE_LEVEL < 2
    key_hash = ndsRendererHardwareTextureKeyHash(&key);
#else
    key_hash = 0u;
#endif
#if NDS_TASK93_TEXKEY_CENSUS
    /* Task 93 E0. Everything above this line is the key rebuild -- ~59 stats
     * fields, the tile sync, and the extent arithmetic -- and it runs before
     * the resident-entry lookup can say the answer was already known. This
     * records the request sequence so a front cache is sized from the trace
     * rather than from a hit rate, as Task 90 did for the light-shade LUT. */
    gNdsTask93BindCalls++;
    if (resolved != NULL) { gNdsTask93PreflightCalls++; }
    if (key_hash == gNdsTask93LastKeyHash) { gNdsTask93ConsecutiveRepeat++; }
    gNdsTask93LastKeyHash = key_hash;
    gNdsTask93KeyTrace[gNdsTask93KeyTraceNext] = key_hash;
    gNdsTask93KeyTraceNext =
        (gNdsTask93KeyTraceNext + 1u) % NDS_TASK93_KEY_TRACE_COUNT;
#endif

    fraction_entry = NULL;
    entry = ndsRendererHardwareFindTexture(&key, key_hash);
    params = ndsRendererHardwareTextureParams(stats, render_tile,
                                               upload_width, upload_height);
#if NDS_R2_STAGE_ROUTE_PROBE
    {
        u32 source_frame_tried = 0u;
#endif
    if ((entry == NULL) && (allow_stage_source_frame != FALSE) &&
        (sNdsRendererBattleStaticTextureArmed != 0u))
    {
        /* Dynamic Pupupu materials keep their source animation and geometry,
         * but reuse the first resident source image when a later image was not
         * prepared before GO. Every other renderer-key word must still match. */
#if NDS_R2_STAGE_ROUTE_PROBE
        source_frame_tried = 1u;
#endif
        entry = ndsRendererHardwareFindStageSourceFrameTexture(&key);
    }
#if NDS_R2_STAGE_ROUTE_PROBE
        if ((resolved != NULL) && (entry == NULL))
        {
            gNdsR2StageTextureMissCount++;
            if (gNdsR2StageTextureMissCount == 1u)
            {
                gNdsR2StageTextureMissRun = gNdsR2StageTextureProbeRun;
                gNdsR2StageTextureMissHash = key_hash;
                gNdsR2StageTextureMissArmed =
                    (sNdsRendererBattleStaticTextureArmed != 0u) ? 1u : 0u;
                gNdsR2StageTextureMissSourceFrameTried = source_frame_tried;
                memcpy((void *)gNdsR2StageTextureMissKeyWords,
                       &key, sizeof(key));
                (void)ndsRelocGetLoadedPointerProvenance(
                    (const void *)(uintptr_t)key.image,
                    (u32 *)&gNdsR2StageTextureMissImageAsset,
                    (u32 *)&gNdsR2StageTextureMissImageOffset);
                (void)ndsRelocGetLoadedPointerProvenance(
                    (const void *)(uintptr_t)key.tlut_image,
                    (u32 *)&gNdsR2StageTextureMissTlutAsset,
                    (u32 *)&gNdsR2StageTextureMissTlutOffset);
            }
        }
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererSemanticLastTextureKeyHash =
        ndsRendererProfileTextureKeyHashFull(&key);
    sNdsRendererSemanticLastTextureParams = params;
#endif
    if (entry != NULL)
    {
        if (resolved != NULL)
        {
            resolved->entry = entry;
            resolved->name = (u32)entry->name;
            resolved->params = (entry->params &
                ~NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK) |
                (params & NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK);
            resolved->format = format;
            resolved->width = width;
            resolved->height = height;
            return TRUE;
        }
        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
        if (entry->pinned != 0u)
        {
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
            /* The offline key deliberately excludes DS sampler bits. Resolve
             * them from the first live BattleShip state, then let the existing
             * site cache reuse the exact entry. */
            entry->params = ndsRendererHardwareMergeTextureParams(params);
            ndsRendererHardwareApplyTextureParams(entry->params);
            sNdsRendererHardwareActiveTextureEntry = entry;
            ndsRendererHardwareRecordBattleStaticTextureHit(entry);
        }
        else if (sNdsRendererHardwareActiveTextureEntry != entry)
        {
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
            ndsRendererHardwareApplyTextureParams(entry->params);
            sNdsRendererHardwareActiveTextureEntry = entry;
        }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = format;
        stats->hardware_texture_width = width;
        stats->hardware_texture_height = height;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererProfileTextureFormat(
            &gNdsRendererProfileTextureBindFormatMask, format, size);
        ndsRendererProfileTextureCacheEntry(entry);
#endif
        ndsRendererStageTextureSiteRemember(
            state, stats, entry, format, size, width, height);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
        return TRUE;
    }
    if (resolved != NULL)
    {
        return FALSE;
    }
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_MANIFEST_FALLBACK);
    if (use_texel1 != FALSE)
    {
        fraction_entry =
            ndsRendererHardwareFindTexel1RefreshTexture(&key);
    }

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD
    if ((sNdsRendererHardwareFrameSerial != 0u) &&
        (fraction_entry != NULL))
    {
        /* The first completed frame populates the exact resident allocations.
         * Subsequent animated-water key changes retain normal lookup, live
         * params, texture binding, batches, and geometry while deliberately
         * keeping the resident pixels from that warm frame. This benchmark
         * branch occurs before source resolution, conversion, or VRAM upload. */
        entry = fraction_entry;
        ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ndsRendererHardwareTextureLookupRemove(entry);
#endif
        ndsRendererHardwareEntrySetKey(entry, &key);
        sNdsRendererHardwareTextureKeyGeneration++;
        if (sNdsRendererHardwareTextureKeyGeneration == 0u)
        {
            sNdsRendererHardwareTextureKeyGeneration++;
        }
        entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        entry->key_hash = key_hash;
#endif
        entry->params = ndsRendererHardwareMergeTextureParams(params);
        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
        entry->ready = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ndsRendererHardwareTextureLookupInsert(entry);
#endif
        sNdsRendererHardwareActiveTextureEntry = entry;
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = format;
        stats->hardware_texture_width = width;
        stats->hardware_texture_height = height;
        ndsRendererHardwareApplyTextureParams(entry->params);
        sNdsRendererBenchmarkSuppressedTextureUploads++;
        sNdsRendererBenchmarkSuppressedTextureUploadBytes +=
            upload_width * upload_height * sizeof(u16);
        ndsRendererStageTextureSiteRemember(
            state, stats, entry, format, size, width, height);
        return TRUE;
    }
#endif

    texels = width * height;
    bytes = ndsRendererHardwareTextureSourceBytes(
        format, size, source_read_width * source_read_height);
    if ((bytes == 0u) || (bytes > loaded_bytes))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }
    texels_src = ndsRendererResolveTextureDataPointer(
        config, (const void *)(uintptr_t)primary_image,
        source_physical_bytes);
    if (texels_src == NULL)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_PTR);
        return FALSE;
    }

    /* First sight of the beam this scene: build its dedicated A5I3 texture from
     * the source in hand. Deliberately here rather than at scene load -- the
     * discriminator is semantic (this combine over an I4 tile) and the extent
     * comes from the tile, so nothing hardcodes an asset address or depends on
     * load order. On failure the generic path continues and produces the
     * previous one-bit result rather than dropping the draw. */
    if ((resolved == NULL) && (use_texel1 == FALSE) &&
        (prim_env_blend_mode ==
             NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA) &&
        (format == NDS_RENDERER_HW_TEXTURE_FMT_I16) &&
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (ndsRendererHardwarePreparePrimRgbTexel0AlphaTexture(
             stats, config, texels_src, primary_image, source_width,
             source_origin_s, source_origin_t, width, height,
             upload_width, upload_height) != FALSE))
    {
        ndsRendererHardwareBindPrimRgbTexel0AlphaTexture(
            stats, params, format, width, height);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
        return TRUE;
    }
    tlut_src = NULL;
    palette_base = 0u;
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        u32 palette_entries;

        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            palette_base = render_tile->palette * 16u;
        }
        /* LOADTLUT is allowed to load fewer than the format's full 16/256
         * entries. Mario's source Fireball list deliberately loads 13 CI4
         * entries because its 16x16 image uses only indices 0..12. Validate
         * the exact indices reachable by this tile instead of rejecting that
         * legal BattleShip state or reading beyond the loaded palette. Keep
         * the common full-TLUT path scan-free; only partial source TLUTs pay
         * the bounded index census. */
        palette_entries = palette_base +
            ((size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ? 16u : 256u);
        if ((use_texel1 != FALSE) &&
            (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI))
        {
            u32 texel1_palette_entries = texel1_source.palette_base +
                ((texel1_source.size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
                    16u : 256u);

            if (texel1_palette_entries > palette_entries)
            {
                palette_entries = texel1_palette_entries;
            }
        }
        if (stats->texture_tlut_count < palette_entries)
        {
            ndsRendererHardwareRecordBattleTextureFence(
                NDS_RENDERER_BATTLE_TEXTURE_FENCE_PALETTE_DECODE);
            palette_entries = ndsRendererHardwareCiPaletteEntriesUsed(
                config, texels_src, size, source_width,
                source_origin_s, source_origin_t,
                source_read_width, source_read_height, palette_base);
            if ((use_texel1 != FALSE) &&
                (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI))
            {
                u32 texel1_read_width =
                    (texel1_source.materialize_s != FALSE) ?
                        (1u << texel1_source.render_tile->masks) :
                        texel1_source.width;
                u32 texel1_read_height =
                    (texel1_source.materialize_t != FALSE) ?
                        (1u << texel1_source.render_tile->maskt) :
                        texel1_source.height;
                u32 texel1_palette_entries;

                ndsRendererHardwareRecordBattleTextureFence(
                    NDS_RENDERER_BATTLE_TEXTURE_FENCE_PALETTE_DECODE);
                texel1_palette_entries =
                    ndsRendererHardwareCiPaletteEntriesUsed(
                        config, texel1_source.texels, texel1_source.size,
                        texel1_source.source_width,
                        texel1_source.source_origin_s,
                        texel1_source.source_origin_t,
                        texel1_read_width, texel1_read_height,
                        texel1_source.palette_base);

                if (texel1_palette_entries > palette_entries)
                {
                    palette_entries = texel1_palette_entries;
                }
            }
        }
        if ((stats->texture_tlut_image == 0u) ||
            (stats->texture_tlut_count < palette_entries))
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_TLUT);
            return FALSE;
        }
        tlut_physical_bytes = ndsRendererTexturePhysicalByteSpan(
            ndsRendererTextureDataLayout(config),
            palette_entries * sizeof(u16));
        tlut_src = ndsRendererResolveTextureDataPointer(
            config, (const void *)(uintptr_t)stats->texture_tlut_image,
            tlut_physical_bytes);
        if (tlut_src == NULL)
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_TLUT_PTR);
            return FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererProfileTextureFormat(
            &gNdsRendererProfileTexturePaletteFormatMask, format, size);
#endif
    }

    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_CONVERT);
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_PALETTE_DECODE);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    convert_start = cpuGetTiming();
#endif
    upload_bytes = upload_width * upload_height * sizeof(u16);
    resident_upload_bytes = upload_bytes;
    staged_bytes = upload_bytes;
#if (NDS_RENDERER_PROFILE_LEVEL < 2) && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    if (fraction_entry != NULL)
    {
        if ((upload_bytes <=
             sizeof(sNdsRendererHardwareTextureRefreshSmall)) &&
            (ndsRendererHardwareTextureRefreshUses(
                 sNdsRendererHardwareTextureRefreshSmall) == FALSE))
        {
            upload_buffer =
                sNdsRendererHardwareTextureRefreshSmall;
            queue_texture_refresh = TRUE;
        }
        else if ((upload_bytes >
                  sizeof(sNdsRendererHardwareTextureRefreshSmall)) &&
                 (ndsRendererHardwareTextureRefreshUses(
                      sNdsRendererHardwareTextureRefreshLarge) == FALSE))
        {
            upload_buffer =
                sNdsRendererHardwareTextureRefreshLarge;
            queue_texture_refresh = TRUE;
        }
    }
#endif
    if ((use_texel1 != FALSE) &&
        (format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (texel1_source.size == NDS_RENDERER_HW_TEXTURE_SIZ_4B))
    {
        /* The source pond's two CI4 inputs can produce only 16x16 palette
         * pairs for a given primitive LOD fraction. Precompute exact RGB and
         * the 16-bit ordered-coverage mask once per pair so the animated pixel
         * loop avoids per-pixel color blending. */
        ndsRendererHardwareBuildTexel01Ci4Lut(
            config, tlut_src, stats->texture_tlut_count, palette_base,
            texel1_source.palette_base, stats->prim_lod_fraction);
        use_texel1_ci4_lut = TRUE;
        use_texel1_ci4_direct = TRUE;
    }

    /* Only the power-of-two rectangle handed to libnds is observable.  The
     * shared scratch arena is sized for the worst 128x128 texture, but smaller
     * animated uploads must not pay to clear the unused tail every frame. */
    if ((width != upload_width) || (height != upload_height))
    {
        memset(sNdsRendererHardwareTextureScratch, 0, upload_bytes);
    }
    if (use_texel1_ci4_direct != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL < 2
        compact_row_output = ndsRendererHardwareConvertTexel01Ci4Direct(
            config, texels_src, source_texels, source_width, source_origin_s,
            source_origin_t, render_tile, materialize_s, materialize_t,
            &texel1_source, texel1_origin_delta_s, texel1_origin_delta_t,
            width, height, upload_width,
            ((upload_buffer == sNdsRendererHardwareTextureRefreshLarge) &&
             (queue_texture_refresh != FALSE) &&
             (width == upload_width) && (height == upload_height)) ?
                staged_row_map : NULL,
            &staged_bytes, &green_texels, &nonwhite_texels);
#else
        (void)ndsRendererHardwareConvertTexel01Ci4Direct(
            config, texels_src, source_texels, source_width, source_origin_s,
            source_origin_t, render_tile, materialize_s, materialize_t,
            &texel1_source, texel1_origin_delta_s, texel1_origin_delta_t,
            width, height, upload_width, NULL, NULL,
            &green_texels, &nonwhite_texels);
#endif
        ndsRendererProfileRecordTextureCi4Direct(width * height);
    }
    else
    {
        for (y = 0u; y < height; y++)
        {
            u32 source_y = (materialize_t != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    y, render_tile->cmt, render_tile->maskt) : y;

            for (x = 0u; x < width; x++)
            {
                u32 source_x = (materialize_s != FALSE) ?
                    ndsRendererHardwareTextureMaskedAddress(
                        x, render_tile->cms, render_tile->masks) : x;
                u32 src_index =
                    ((source_origin_t + source_y) * source_width) +
                    source_origin_s + source_x;
                u32 dst_index = (y * upload_width) + x;
                u16 color;

                if (use_texel1_ci4_lut != FALSE)
                {
                    u32 index0 = ndsRendererReadTexturePackedNibble(
                        config, texels_src, src_index, format, size);
                    u32 source1_index = ndsRendererHardwareTexel1SourceIndex(
                        &texel1_source, texel1_origin_delta_s,
                        texel1_origin_delta_t, x, y);
                    u32 index1 = ndsRendererReadTexturePackedNibble(
                        config, texel1_source.texels, source1_index,
                        texel1_source.format, texel1_source.size);

                    color = ndsRendererHardwareResolveTexel01Ci4Lut(
                        index0, index1, x, y);
                }
                else
                {
                    color = ndsRendererHardwareTextureColor(
                        config, format, size, texels_src, tlut_src,
                        stats->texture_tlut_count, palette_base, src_index,
                        use_texel1);
                    if (use_texel1 != FALSE)
                    {
                        u16 color1 = ndsRendererHardwareTexel1Color(
                            &texel1_source, config, tlut_src,
                            stats->texture_tlut_count,
                            texel1_origin_delta_s, texel1_origin_delta_t,
                            x, y);

                        color = ndsRendererHardwareBlendTexel01(
                            color, color1, stats->prim_lod_fraction, x, y);
                    }
                    else if (prim_env_blend_mode ==
                             NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA)
                    {
                        color = ndsRendererHardwarePrimRgbTexel0Alpha(
                            color, stats->prim_color);
                    }
                    else if (prim_env_blend_mode !=
                             NDS_RENDERER_PRIM_ENV_BLEND_NONE)
                    {
                        color = ndsRendererHardwareBlendPrimEnvTexel0(
                            color, stats->prim_color, stats->env_color);
                    }
                }
                sNdsRendererHardwareTextureScratch[dst_index] = color;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                ndsRendererProfileTexturePixel(color, &green_texels,
                                               &nonwhite_texels);
#endif
            }
        }
    }
    /* Preserve the canonical lane-observation totals while paying one volatile
     * update per converted texture instead of one per texel. */
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererRecordTextureLaneUseCount(config, format, size, texels);
    if (use_texel1 != FALSE)
    {
        ndsRendererRecordTextureLaneUseCount(
            config, texel1_source.format, texel1_source.size, texels);
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureConvertFormatMask, format, size);
#endif

    /* TEXEL1 is the one path that may refresh an existing direct-colour
     * allocation in place when only the blend fraction changes. Keep that
     * refresh representation unchanged. Every other miss is immutable for its
     * exact 236-byte key, so compact the already-resolved final image whenever
     * it fits losslessly in PAL16. This is especially important in 3+/4-player
     * battles: BattleShip correctly selects Low fighter detail there, and the
     * fourth owner's late material must not fail merely because earlier dynamic
     * images consumed texture VRAM as 16bpp direct colour. */
    if ((use_texel1 == FALSE) &&
        (upload_buffer == sNdsRendererHardwareTextureScratch))
    {
        resident_palette_entries = ndsRendererHardwarePackResolvedPal16(
            sNdsRendererHardwareTextureScratch,
            upload_width * upload_height,
            resident_palette, &resident_color0_transparent);
        if (resident_palette_entries != 0u)
        {
            resident_texture_type = GL_RGB16;
            resident_upload_bytes = (upload_width * upload_height + 1u) >> 1;
            if (resident_color0_transparent != FALSE)
            {
                params |= (u32)GL_TEXTURE_COLOR0_TRANSPARENT;
            }
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureConvertTicks += cpuGetTiming() - convert_start;

    upload_start = cpuGetTiming();
#endif
#if NDS_TICK_HUD
    /* R2-07: the same span, re-marked, because upload_start above only exists
     * at NDS_RENDERER_PROFILE_LEVEL >= 1 and the tick-HUD build is level 0. */
    tickhud_upload_mark = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (upload_buffer == sNdsRendererHardwareTextureRefreshLarge)
    {
        staged_row_bytes = upload_width * sizeof(u16);
        staged_row_count = upload_height;
        if ((compact_row_output == FALSE) &&
            (ndsRendererHardwareStageUniqueTextureRows(
                sNdsRendererHardwareTextureScratch, upload_buffer,
                sizeof(sNdsRendererHardwareTextureRefreshLarge),
                staged_row_map, staged_row_bytes, staged_row_count,
                &staged_bytes) == FALSE))
        {
            upload_buffer = sNdsRendererHardwareTextureScratch;
            queue_texture_refresh = FALSE;
            staged_bytes = upload_bytes;
            staged_row_bytes = 0u;
            staged_row_count = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            gNdsRendererProfileTextureVBlankFallbackCount++;
#endif
        }
    }
    else if (upload_buffer != sNdsRendererHardwareTextureScratch)
    {
        memcpy(upload_buffer, sNdsRendererHardwareTextureScratch,
               staged_bytes);
    }
#endif
    entry = fraction_entry;
    if (entry != NULL)
    {
        s32 refresh_ready = FALSE;

#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (queue_texture_refresh != FALSE)
        {
            refresh_ready = ndsRendererHardwareQueueTextureRefresh(
                entry, upload_buffer, staged_bytes, upload_bytes,
                (staged_row_count != 0u) ? staged_row_map : NULL,
                staged_row_bytes, staged_row_count);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            if (refresh_ready == FALSE)
            {
                gNdsRendererProfileTextureVBlankFallbackCount++;
            }
#endif
        }
#endif
        if ((refresh_ready == FALSE) &&
            (ndsRendererHardwareReplaceTextureData(
                 entry, upload_buffer, staged_bytes, upload_bytes, NULL, 0u,
                 0u) != FALSE))
        {
            refresh_ready = TRUE;
        }
        if (refresh_ready == FALSE)
        {
            entry = ndsRendererHardwareReleaseTexture(entry);
            entry = NULL;
        }
        else
        {
            ndsRendererProfileRecordTexel1Refresh();
        }
    }
    if (entry == NULL)
    {
        entry = ndsRendererHardwareAllocTexture();
        if (entry == NULL)
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size, NDS_RENDERER_HW_TEXREJECT_ALLOC);
            return FALSE;
        }
        if (entry->name == 0)
        {
            if (ndsRendererHardwareFencedGlGenTextures(
                    1, &entry->name) == 0)
            {
                ndsRendererHardwareRejectTexture(
                    stats, format, size, NDS_RENDERER_HW_TEXREJECT_GENTEX);
                return FALSE;
            }
        }

        ndsRendererHardwareEndBatch();
        ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        upload_attempts = 0u;
        while (ndsRendererHardwareFencedGlTexImage2D(
                   GL_TEXTURE_2D, 0, resident_texture_type, size_x, size_y, 0,
                   params, sNdsRendererHardwareTextureScratch) == 0)
        {
            (void)ndsRendererHardwareReleaseTexture(entry);
            upload_attempts++;
            if ((upload_attempts >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT) ||
                (ndsRendererHardwareEvictTexture(entry) == FALSE))
            {
                ndsRendererHardwareRejectTexture(
                    stats, format, size,
                    NDS_RENDERER_HW_TEXREJECT_TEXIMAGE);
                return FALSE;
            }
            if (ndsRendererHardwareFencedGlGenTextures(
                    1, &entry->name) == 0)
            {
                ndsRendererHardwareRejectTexture(
                    stats, format, size,
                    NDS_RENDERER_HW_TEXREJECT_GENTEX);
                return FALSE;
            }
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        }
        if (resident_palette_entries != 0u)
        {
            glColorTableEXT(GL_TEXTURE_2D, 0, (int)resident_palette_entries,
                            0, 0, resident_palette);
        }
    }
    ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureUploadTicks += cpuGetTiming() - upload_start;
#endif
#if NDS_TICK_HUD
    gNdsMiscTexUploadTicks += cpuGetTiming() - tickhud_upload_mark;
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareTextureLookupRemove(entry);
#endif
    ndsRendererHardwareEntrySetKey(entry, &key);
    sNdsRendererHardwareTextureKeyGeneration++;
    if (sNdsRendererHardwareTextureKeyGeneration == 0u)
    {
        sNdsRendererHardwareTextureKeyGeneration++;
    }
    entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    entry->key_hash = key_hash;
#endif
    entry->params = ndsRendererHardwareMergeTextureParams(params);
    entry->source_texels = texels;
    entry->green_texels = green_texels;
    entry->nonwhite_texels = nonwhite_texels;
    entry->profile_width = upload_width;
    entry->profile_height = upload_height;
    entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    entry->ready = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareTextureLookupInsert(entry);
#endif
    sNdsRendererHardwareActiveTextureEntry = entry;
    stats->hardware_texture_upload_count++;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = format;
    stats->hardware_texture_width = width;
    stats->hardware_texture_height = height;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureBindFormatMask, format, size);
#endif
    ndsRendererHardwareApplyTextureParams(entry->params);
    ndsRendererProfileRecordTextureUpload(resident_upload_bytes);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureCacheEntry(entry);
#endif
    ndsRendererStageTextureSiteRemember(
        state, stats, entry, format, size, width, height);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
    return TRUE;
}

/* R2-08: the three thin wrappers below are the whole call surface of
 * ndsRendererHardwareResolveOrBindTexture, so bracketing them charges the
 * texture-RESOLVE phase (distinct from upload, which the previous probe already
 * eliminated at 6.7 ticks/frame) without touching a function that has a dozen
 * early returns. Charged only while the effect tree submit is on the stack. */
static s32 ndsRendererHardwareBindTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state)
{
#if NDS_TICK_HUD
    if (gNdsEffectPhaseActive != 0u)
    {
        u32 phase_mark = cpuGetTiming();
        s32 phase_result = ndsRendererHardwareResolveOrBindTexture(
            stats, config, state, NULL, FALSE);
        u32 phase_ticks = cpuGetTiming() - phase_mark;

        gNdsEffectPhaseTexTicks += phase_ticks;
        /* The in-Exec twin, added cycle 91. This site and the stage-source-frame
         * site below both charged gNdsEffectPhaseTexTicks with no twin, so
         * gNdsEffectPhaseTexInExecTicks read 0 for a whole cycle and the texture
         * sub-share folded silently into the derived traversal residual. */
        if (sNdsEffectPacketArmed != 0u)
        {
            gNdsEffectPhaseTexInExecTicks += phase_ticks;
        }
        return phase_result;
    }
#endif
    return ndsRendererHardwareResolveOrBindTexture(
        stats, config, state, NULL, FALSE);
}

static s32 ndsRendererHardwareResolveResidentTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    NDSRendererHardwareResolvedTexture *resolved)
{
    if (resolved == NULL)
    {
        return FALSE;
    }
    memset(resolved, 0, sizeof(*resolved));
#if NDS_TICK_HUD
    if (gNdsEffectPhaseActive != 0u)
    {
        u32 phase_mark = cpuGetTiming();
        s32 phase_result = ndsRendererHardwareResolveOrBindTexture(
            stats, config, state, resolved, FALSE);
        u32 phase_ticks = cpuGetTiming() - phase_mark;

        gNdsEffectPhaseTexTicks += phase_ticks;
        /* The in-Exec twin. gNdsEffectPhaseTexTicks is armed by the whole-tree
         * flag, so it can accumulate outside the interpreter call and cannot
         * close an identity against Exec; this one is armed by the same flag as
         * the Vtx/Tri spans, so all four nest in the same window. */
        if (sNdsEffectPacketArmed != 0u)
        {
            gNdsEffectPhaseTexInExecTicks += phase_ticks;
        }
        return phase_result;
    }
#endif
    return ndsRendererHardwareResolveOrBindTexture(
        stats, config, state, resolved, FALSE);
}

static s32 ndsRendererHardwareResolveStageSourceFrameTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    NDSRendererHardwareResolvedTexture *resolved)
{
    if (resolved == NULL)
    {
        return FALSE;
    }
    memset(resolved, 0, sizeof(*resolved));
#if NDS_TICK_HUD
    if (gNdsEffectPhaseActive != 0u)
    {
        u32 phase_mark = cpuGetTiming();
        s32 phase_result = ndsRendererHardwareResolveOrBindTexture(
            stats, config, state, resolved, TRUE);
        u32 phase_ticks = cpuGetTiming() - phase_mark;

        gNdsEffectPhaseTexTicks += phase_ticks;
        /* The in-Exec twin, added cycle 91; see ndsRendererHardwareBindTexture. */
        if (sNdsEffectPacketArmed != 0u)
        {
            gNdsEffectPhaseTexInExecTicks += phase_ticks;
        }
        return phase_result;
    }
#endif
    return ndsRendererHardwareResolveOrBindTexture(
        stats, config, state, resolved, TRUE);
}

static void ndsRendererHardwareEndBatch(void);

static void ndsRendererCopyMtx20p12ToM4x4(
    const NDSRendererMatrix20p12 *src, m4x4 *dst)
{
    u32 row;
    u32 col;

    if ((src == NULL) || (dst == NULL))
    {
        return;
    }

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            dst->m[(row * 4u) + col] = src->m[row][col];
        }
    }
}

/* The copy above writes element i to element i: `NDSRendererMatrix20p12` is
 * `s32 m[4][4]` and libnds' `m4x4` is `int m[16]`, both row-major, both 64
 * bytes. A loader that only wants to hand the matrix to `glLoadMatrix4x4` does
 * not need the intermediate at all -- and the two per-root loaders were paying
 * for two of them, 128 bytes of stack traffic a call, on a function the census
 * prices at 1,064 cycles a call across 12,431 calls with no hot spot above 3.9%
 * and almost all of it MEMORY stall (it is absent from the census's non-mem
 * stall ranking entirely). Deleting the intermediate deletes the traffic.
 *
 * The access is through `int` lvalues on an object whose elements are `int`, so
 * this is not a type-punned read; the static assertions below are what keep the
 * two layouts from drifting apart silently. */
_Static_assert(sizeof(m4x4) == sizeof(NDSRendererMatrix20p12),
               "m4x4 and NDSRendererMatrix20p12 must be the same size");
_Static_assert(sizeof(((m4x4 *)0)->m[0]) ==
                   sizeof(((NDSRendererMatrix20p12 *)0)->m[0][0]),
               "m4x4 and NDSRendererMatrix20p12 elements must match");

static inline const m4x4 *ndsRendererMtx20p12AsM4x4(
    const NDSRendererMatrix20p12 *src)
{
    return (const m4x4 *)src;
}

static void ndsRendererBuildRawHardwareMatrix(
    const NDSRendererMatrix20p12 *composed,
    NDSRendererMatrix20p12 *hardware)
{
    u32 col;

    if ((composed == NULL) || (hardware == NULL))
    {
        return;
    }

    /* Source coordinates are submitted as source / 256 in DS 4.12. Keep
     * composed rows 0..2 unchanged and divide the complete homogeneous row 3
     * by the same factor. The GX clip vector is then CPU clip / 256, preserving
     * X/W, Y/W, and Z/W for arbitrary composed/matrix-word state. */
    *hardware = *composed;
    for (col = 0u; col < 4u; col++)
    {
        hardware->m[3][col] = ndsRendererRoundShiftS32Signed(
            hardware->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
}

static s32 ndsRendererBuildShiftedRawHardwareMatrix(
    const NDSRendererMatrix20p12 *composed,
    NDSRendererMatrix20p12 *hardware,
    u32 coordinate_shift)
{
    u32 row;
    u32 col;

    if ((composed == NULL) || (hardware == NULL))
    {
        return FALSE;
    }
    *hardware = *composed;
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            s64 scaled = (s64)hardware->m[row][col] << coordinate_shift;

            if ((scaled > INT_MAX) || (scaled < INT_MIN))
            {
                return FALSE;
            }
            hardware->m[row][col] = (s32)scaled;
        }
    }
    for (col = 0u; col < 4u; col++)
    {
        hardware->m[3][col] = ndsRendererRoundShiftS32Signed(
            hardware->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    return TRUE;
}

static void ndsRendererLoadHardwareMatrixPair(
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    u32 mode, u32 generation, u32 scale_world)
{
    (void)scale_world;

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode == mode) &&
        (sNdsRendererHardwareMatrixGeneration == generation))
    {
        return;
    }

    ndsRendererHardwareEndBatch();
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(ndsRendererMtx20p12AsM4x4(projection));
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadMatrix4x4(ndsRendererMtx20p12AsM4x4(modelview));

    ndsRendererProfileRecordMatrixLoad();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileMatrixScaleWorld = scale_world;
    gNdsRendererProfileProjectionM00 = projection->m[0][0];
    gNdsRendererProfileProjectionM11 = projection->m[1][1];
    gNdsRendererProfileProjectionM22 = projection->m[2][2];
    gNdsRendererProfileProjectionM32 = projection->m[3][2];
    gNdsRendererProfileModelviewM00 = modelview->m[0][0];
    gNdsRendererProfileModelviewM11 = modelview->m[1][1];
    gNdsRendererProfileModelviewM22 = modelview->m[2][2];
    gNdsRendererProfileModelviewM30 = modelview->m[3][0];
    gNdsRendererProfileModelviewM31 = modelview->m[3][1];
    gNdsRendererProfileModelviewM32 = modelview->m[3][2];
#endif

    sNdsRendererHardwareMatrixMode = mode;
    sNdsRendererHardwareMatrixGeneration = generation;
    sNdsRendererHardwareMatrixLoaded = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererHardwareMatrixSignature =
        ndsRendererProfileHashMatrixPair(
            projection, modelview, mode, generation);
#endif
}

static void ndsRendererLoadHardwareRawComposedMatrix(
    const NDSRendererMatrix20p12 *composed, u32 generation)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
        (sNdsRendererHardwareMatrixGeneration == generation))
    {
        return;
    }

    ndsRendererMtxIdentity20p12(&projection);
    ndsRendererBuildRawHardwareMatrix(composed, &modelview);
    ndsRendererLoadHardwareMatrixPair(
        &projection, &modelview, NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED,
        generation, TRUE);
}

#if NDS_R2_FIGHTER_HW_MTX
/* R2-03 E16b. The composed load hands the geometry engine one CPU-built
 * modelview x projection with an identity projection. libnds `GL_MODELVIEW` is
 * DS matrix mode 2, position AND vector, so a vector matrix is already being
 * written on every load -- but what lands in it is the *composed* MVP, and
 * normals must never be rotated by the projection. That is what makes hardware
 * lighting impossible today, not a missing mode.
 *
 * This loads the two matrices separately, so the vector matrix holds the
 * modelview alone and the engine performs the multiply for positions.
 *
 * Exactness: ndsRendererBuildRawHardwareMatrix divides row 3 of the composed
 * matrix by the world-unit shift. Under the row-vector convention
 * C[3] = M[3] x P, so scaling M's row 3 before the multiply yields the same
 * row 3 afterwards -- the scaling commutes with the right-multiply. The only
 * difference from the composed path is that the engine rounds the product in
 * its own internal precision rather than the CPU's 20.12, which is sub-pixel
 * and a screenshot question. */
/* noinline is load-bearing, not style: the caller
 * ndsRendererExecuteNativeFighterOwnerProduction lives in `.itcm.native_fighter`
 * and ITCM has no room -- inlining this overflowed the region by 72 bytes. It
 * runs once per root, so an out-of-line call costs nothing measurable. */
#if NDS_TASK91_DRAW_PHASE_CENSUS
/* R2-03 E22. The per-root bracket is ~40,000/frame and E17 already took 17,600
 * of it. What remains is ~28 matrix loads a frame, deduplicated by a generation
 * counter. E21's rule applies directly: the generation is the TARGET identity,
 * and the question is WRITE identity -- how many loads that the generation check
 * lets through are re-loading a matrix the hardware already holds. Only that
 * fraction is elidable by a content compare. */
u32 gNdsR2MtxLoadCalls;
u32 gNdsR2MtxLoadElidedByGeneration;
u32 gNdsR2MtxLoadPerformed;
u32 gNdsR2MtxLoadIdenticalContent;
u32 gNdsR2MtxLoadIdenticalProjection;
u32 gNdsR2MtxLoadIdenticalModelview;
static NDSRendererMatrix20p12 sNdsR2MtxLastProjection;
static NDSRendererMatrix20p12 sNdsR2MtxLastModelview;
static u8 sNdsR2MtxLastValid;
#endif

/* Capture-free matrix registers for the fighter-exclusive split loader.
 *
 * glMatrixMode and glLoadMatrix4x4 are #defined in this TU to the Task 29
 * wrappers, which funnel every write through ndsRendererTask29GXRecord. At the
 * shipped config that funnel still carries the NDS_TASK36_HW_COMPOSE == 2
 * replay recorder, and under the tick HUD an sNdsEffectPacketArmed test as
 * well: 791 + 454 ticks/frame over 161,603 executions in the c106 profile, for
 * a path neither recorder can ever observe. Task 36 capture is armed only
 * around a stage run (ndsRendererCommitNativeStageSegment); both callers of the
 * split loader are fighter-side. Same argument as the fighter emit writers in
 * slice 1 -- see the pointer above ndsRendererHardwareWriteColorWord.
 *
 * The modelview variant also folds the world-unit scaling into the FIFO write.
 * Scaling touches row 3 only, so copying the whole 64-byte matrix to change
 * four words was 1,794 ticks/frame over 124,310 executions, plus the 364 the
 * scaling loop spent reading the copy back. */
static inline void ndsRendererHardwareFighterSetMatrixMode(u32 mode)
{
#if NDS_RENDERER_M3_PHASE0_PROFILE
    /* Leave the M3 state shadow authoritative when that profile is built; its
     * own bookkeeping dwarfs the census hook there. */
    ndsRendererHardwareSetMatrixMode((int)mode);
#else
    MATRIX_CONTROL = mode;
#endif
}

static inline void ndsRendererHardwareFighterLoadMatrix4x4(
    const NDSRendererMatrix20p12 *matrix)
{
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        MATRIX_LOAD4x4 = matrix->m[row][0];
        MATRIX_LOAD4x4 = matrix->m[row][1];
        MATRIX_LOAD4x4 = matrix->m[row][2];
        MATRIX_LOAD4x4 = matrix->m[row][3];
    }
}

static inline void ndsRendererHardwareFighterLoadModelviewWorldScaled(
    const NDSRendererMatrix20p12 *matrix)
{
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        MATRIX_LOAD4x4 = matrix->m[row][0];
        MATRIX_LOAD4x4 = matrix->m[row][1];
        MATRIX_LOAD4x4 = matrix->m[row][2];
        MATRIX_LOAD4x4 = matrix->m[row][3];
    }
    for (col = 0u; col < 4u; col++)
    {
        MATRIX_LOAD4x4 = ndsRendererRoundShiftS32Signed(
            matrix->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
}

static void __attribute__((noinline)) NDS_R2_ITCM_PACK2_EVICTED_PLAIN_CODE
ndsRendererLoadHardwareSplitMatrices(
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    u32 generation)
{
    if ((projection == NULL) || (modelview == NULL))
    {
        return;
    }
#if NDS_TASK91_DRAW_PHASE_CENSUS
    gNdsR2MtxLoadCalls++;
    /* Anyone else loading a matrix invalidates the content memo, or the compare
     * below would report "same as my last load" while the hardware holds
     * someone else's matrix. Conservative direction. */
    if ((sNdsRendererHardwareMatrixLoaded == 0u) ||
        (sNdsRendererHardwareMatrixMode !=
         NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED))
    {
        sNdsR2MtxLastValid = 0u;
    }
#endif
    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
        (sNdsRendererHardwareMatrixGeneration == generation))
    {
#if NDS_TASK91_DRAW_PHASE_CENSUS
        gNdsR2MtxLoadElidedByGeneration++;
#endif
        return;
    }

#if NDS_TASK91_DRAW_PHASE_CENSUS
    {
    NDSRendererMatrix20p12 scaled_modelview;
    u32 col;

    /* The census memo compares the matrix the hardware actually receives, so it
     * has to materialise what the FIFO writer scales on the fly. It is a
     * diagnostic and pays for its own copy. */
    scaled_modelview = *modelview;
    for (col = 0u; col < 4u; col++)
    {
        scaled_modelview.m[3][col] = ndsRendererRoundShiftS32Signed(
            scaled_modelview.m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }

    gNdsR2MtxLoadPerformed++;
    if (sNdsR2MtxLastValid != 0u)
    {
        /* Scored separately: the loader writes two matrices per call and only
         * the modelview is per-root. A joint compare reports "nothing is
         * redundant" while the projection half may be redundant every time. */
        u32 same_projection = (memcmp(&sNdsR2MtxLastProjection, projection,
                                      sizeof(NDSRendererMatrix20p12)) == 0);
        u32 same_modelview = (memcmp(&sNdsR2MtxLastModelview, &scaled_modelview,
                                     sizeof(NDSRendererMatrix20p12)) == 0);
        gNdsR2MtxLoadIdenticalProjection += same_projection;
        gNdsR2MtxLoadIdenticalModelview += same_modelview;
        gNdsR2MtxLoadIdenticalContent += (same_projection & same_modelview);
    }
    sNdsR2MtxLastProjection = *projection;
    sNdsR2MtxLastModelview = scaled_modelview;
    sNdsR2MtxLastValid = 1u;
    }
#endif

    ndsRendererHardwareEndBatch();
    /* R2-03 E23 tried skipping this half when unchanged -- E22 measured 29 of
     * the 30 per-root loads a frame re-push an identical projection. Engaged on
     * 93.8% of loads and worth -3,008 FTR P50, under the placement floor. The
     * FIFO writes are simply cheap; see the E22/E23 write-up. Reverted. */
    ndsRendererHardwareFighterSetMatrixMode(GL_PROJECTION);
    ndsRendererHardwareFighterLoadMatrix4x4(projection);
    ndsRendererHardwareFighterSetMatrixMode(GL_MODELVIEW);
    ndsRendererHardwareFighterLoadModelviewWorldScaled(modelview);

    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED;
    sNdsRendererHardwareMatrixGeneration = generation;
    sNdsRendererHardwareMatrixLoaded = TRUE;
}

#if NDS_R2_FIGHTER_GX_COMPOSE
/* The adapter stops composing at all under this flag, so `modelview_matrix` is
 * the seed rather than a world. BindProductionRoot only leaves it unread when
 * HW_MTX and SHADE_SKIP_SOFT_LIGHT both hold; without them the software light
 * preparation would take a root's transform from a matrix that is no longer
 * one. The SHADE_SKIP half cannot be tested here -- it is #defined 4,000 lines
 * further down, next to the code it guards -- so it is asserted there. */
#if !NDS_R2_FIGHTER_HW_MTX
#error "NDS_R2_FIGHTER_GX_COMPOSE requires NDS_R2_FIGHTER_HW_MTX"
#endif

u32 gNdsR2GxComposeRoots;
u32 gNdsR2GxComposeMults;
u32 gNdsR2GxComposeRestores;
u32 gNdsR2GxComposeStores;
u32 gNdsR2GxComposeProjectionSkips;

/* Reset at the top of every execute, which is what makes pointer equality a
 * sound test for "the hardware already holds this projection". */
static const NDSRendererMatrix20p12 *sNdsR2GxLastProjection;

static const NDSRendererMatrix20p12 sNdsR2GxIdentity20p12 =
{
    {
        { 1 << NDS_RENDERER_DS_MTX_FRAC_BITS, 0, 0, 0 },
        { 0, 1 << NDS_RENDERER_DS_MTX_FRAC_BITS, 0, 0 },
        { 0, 0, 1 << NDS_RENDERER_DS_MTX_FRAC_BITS, 0 },
        { 0, 0, 0, 1 << NDS_RENDERER_DS_MTX_FRAC_BITS },
    }
};

static inline void ndsRendererHardwareFighterMultMatrix4x3(
    const NDSRendererMatrix20p12 *matrix)
{
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        MATRIX_MULT4x3 = matrix->m[row][0];
        MATRIX_MULT4x3 = matrix->m[row][1];
        MATRIX_MULT4x3 = matrix->m[row][2];
    }
}

/* diag(1, 1, 1, s) x current, s being the world-unit shift. MTX_MULT is a LEFT
 * multiply, so this scales row 3 -- translation and w together -- of whatever
 * chain has been composed, which is precisely what
 * ndsRendererHardwareFighterLoadModelviewWorldScaled writes today. It has to be
 * a separate final multiply rather than folded into the factors: scaling row 3
 * of every matrix in a chain does NOT compose to scaling the product's row 3,
 * because each factor's own m[3][3] then multiplies the next one's scaled row. */
static inline void ndsRendererHardwareFighterMultMatrixWorldScaled(
    const NDSRendererMatrix20p12 *matrix)
{
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        MATRIX_MULT4x4 = matrix->m[row][0];
        MATRIX_MULT4x4 = matrix->m[row][1];
        MATRIX_MULT4x4 = matrix->m[row][2];
        MATRIX_MULT4x4 = matrix->m[row][3];
    }
    for (col = 0u; col < 4u; col++)
    {
        MATRIX_MULT4x4 = ndsRendererRoundShiftS32Signed(
            matrix->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
}

/* Slice 43. The geometry engine composes the joint chain: restore the parent
 * binding's finished world from the palette (or seed a root binding), multiply
 * this binding's chain in the order the CPU pass used, leave the UNSCALED world
 * where a child or a cross-run corner can find it, then apply the world-unit
 * scale for the draw. GL_MODELVIEW is DS matrix mode 2, so every one of these
 * commands acts on the position and vector matrices together -- which is what
 * NDS_R2_FIGHTER_HW_LIGHT needs, and the scale is a no-op on the 3x3 vector
 * matrix because it only touches row 3. */
#if NDS_FIGHTER_PACKET_LIVE
/* Record-frame tees for the loader below. Each records the parameter index the
 * replay patches every frame; the world-unit scale is a constant. */
static void ndsFighterPacketRecordProjection(
    const NDSRendererMatrix20p12 *projection)
{
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;
    u32 i;

    ndsFighterPacketCmd1(REG2ID(MATRIX_CONTROL), (u32)GL_PROJECTION);
    i = ndsFighterPacketCmd(REG2ID(MATRIX_LOAD4x4), 16u);
    if ((rec->fault == 0u) && (rec->packet != NULL))
    {
        ndsFighterPacketStoreMatrix4x4(&rec->words[i], projection);
        if (rec->packet->projection_index == NDS_FIGHTER_PACKET_INDEX_NONE)
        {
            rec->packet->projection_index = (u16)i;
        }
    }
}

static void ndsFighterPacketRecordSeed(const NDSRendererMatrix20p12 *seed)
{
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;
    u32 i = ndsFighterPacketCmd(REG2ID(MATRIX_LOAD4x4), 16u);

    if ((rec->fault == 0u) && (rec->packet != NULL) &&
        (rec->current_root < NDS_FIGHTER_PACKET_ROOT_MAX))
    {
        ndsFighterPacketStoreMatrix4x4(&rec->words[i], seed);
        rec->packet->roots[rec->current_root].seed_index = (u16)i;
    }
}

static void ndsFighterPacketRecordLocal(
    u32 local, const NDSRendererMatrix20p12 *matrix)
{
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;
    u32 i = ndsFighterPacketCmd(REG2ID(MATRIX_MULT4x3), 12u);

    if ((rec->fault == 0u) && (rec->packet != NULL) &&
        (rec->current_root < NDS_FIGHTER_PACKET_ROOT_MAX) &&
        (local < NDS_FIGHTER_PACKET_LOCAL_MAX))
    {
        ndsFighterPacketStoreMatrix4x3(&rec->words[i], matrix);
        rec->packet->roots[rec->current_root].local_index[local] = (u16)i;
    }
}

static void ndsFighterPacketRecordWorldScaled(
    const NDSRendererMatrix20p12 *matrix)
{
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;
    u32 i = ndsFighterPacketCmd(REG2ID(MATRIX_MULT4x4), 16u);

    if (rec->fault == 0u)
    {
        u32 *dst = &rec->words[i];
        u32 row;
        u32 col;

        for (row = 0u; row < 3u; row++)
        {
            *dst++ = (u32)matrix->m[row][0];
            *dst++ = (u32)matrix->m[row][1];
            *dst++ = (u32)matrix->m[row][2];
            *dst++ = (u32)matrix->m[row][3];
        }
        for (col = 0u; col < 4u; col++)
        {
            *dst++ = (u32)ndsRendererRoundShiftS32Signed(
                matrix->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
        }
    }
}
#endif

static void __attribute__((noinline)) NDS_R2_ITCM_PACK2_CODE
ndsRendererLoadHardwareGxComposedMatrices(
    const NDSRendererNativeFighterRoot *input, u32 generation)
{
    u32 i;

    if ((input == NULL) || (input->projection_matrix == NULL) ||
        (input->gx_seed == NULL) ||
        ((input->gx_local_count != 0u) && (input->gx_locals == NULL)))
    {
        return;
    }

    ndsRendererHardwareEndBatch();
    /* R2-03 E22 measured 29 of the 30 per-root loads a frame re-pushing an
     * IDENTICAL projection, and E23 measured skipping it at -3,008 -- under the
     * floor on its own, so it was reverted. It is not on its own here: this
     * slice's own gate put the FIFO word at ~24 cycles rather than the 12.2 E23
     * implied, which makes 31 elided 17-word pushes worth about twice what E23
     * priced. Pointer equality is sound because the cache is reset at the top of
     * each execute and the adapter cannot rewrite the projection buffer inside
     * one -- nothing between two roots changes GL_PROJECTION either, the light
     * vector write brackets GL_MODELVIEW only. */
    if (input->projection_matrix != sNdsR2GxLastProjection)
    {
        ndsRendererHardwareFighterSetMatrixMode(GL_PROJECTION);
        ndsRendererHardwareFighterLoadMatrix4x4(input->projection_matrix);
        sNdsR2GxLastProjection = input->projection_matrix;
    }
    else
    {
        gNdsR2GxComposeProjectionSkips++;
    }
    ndsRendererHardwareFighterSetMatrixMode(GL_MODELVIEW);

    if (input->gx_parent_slot >= NDS_RENDERER_FIGHTER_GX_SLOT_NONE)
    {
        if (input->gx_seed_is_identity != 0u)
        {
            MATRIX_IDENTITY = 0;
        }
        else
        {
            ndsRendererHardwareFighterLoadMatrix4x4(input->gx_seed);
        }
    }
    else
    {
        MATRIX_RESTORE = input->gx_parent_slot;
        gNdsR2GxComposeRestores++;
    }
    for (i = 0u; i < (u32)input->gx_local_count; i++)
    {
        ndsRendererHardwareFighterMultMatrix4x3(&input->gx_locals[i]);
    }
    gNdsR2GxComposeMults += (u32)input->gx_local_count;
    if (input->gx_store_slot < NDS_RENDERER_FIGHTER_GX_SLOT_NONE)
    {
        MATRIX_STORE = input->gx_store_slot;
        gNdsR2GxComposeStores++;
    }
    ndsRendererHardwareFighterMultMatrixWorldScaled(&sNdsR2GxIdentity20p12);
    gNdsR2GxComposeRoots++;

    ndsRendererProfileRecordMatrixLoad();
    /* The generation memo cannot elide anything here: every binding leaves a
     * different matrix, and the restore/store pair is state the memo does not
     * model. Report "not loaded" so no later reader believes it knows what the
     * hardware holds. */
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED;
    sNdsRendererHardwareMatrixGeneration = generation;
    sNdsRendererHardwareMatrixLoaded = FALSE;
}

#if NDS_FIGHTER_PACKET_LIVE
/* Record-frame twin of the loader above, selected per root by the production
 * execute: the hardware receives exactly the words the loader pushes, and the
 * packet records them with the parameter indices the replay patches. Main RAM
 * and cold -- it runs only on the frame a packet is recorded -- which is what
 * keeps the record path out of the full ITCM region. */
static void NDS_FIGHTER_PACKET_COLD_CODE
ndsFighterPacketLoadGxComposedRecord(
    u32 root_index, const NDSRendererNativeFighterRoot *input, u32 generation)
{
    u32 i;

    ndsFighterPacketBeginRoot(root_index, input);
    if ((input->projection_matrix == NULL) || (input->gx_seed == NULL) ||
        ((input->gx_local_count != 0u) && (input->gx_locals == NULL)))
    {
        sNdsFighterPacketRecorder.fault = 1u;
        return;
    }
    ndsRendererHardwareEndBatch();
    if (input->projection_matrix != sNdsR2GxLastProjection)
    {
        ndsRendererHardwareFighterSetMatrixMode(GL_PROJECTION);
        ndsRendererHardwareFighterLoadMatrix4x4(input->projection_matrix);
        sNdsR2GxLastProjection = input->projection_matrix;
        ndsFighterPacketRecordProjection(input->projection_matrix);
    }
    else
    {
        gNdsR2GxComposeProjectionSkips++;
    }
    ndsRendererHardwareFighterSetMatrixMode(GL_MODELVIEW);
    ndsFighterPacketCmd1(REG2ID(MATRIX_CONTROL), (u32)GL_MODELVIEW);
    if (input->gx_parent_slot >= NDS_RENDERER_FIGHTER_GX_SLOT_NONE)
    {
        if (input->gx_seed_is_identity != 0u)
        {
            MATRIX_IDENTITY = 0;
            ndsFighterPacketCmd0(REG2ID(MATRIX_IDENTITY));
        }
        else
        {
            ndsRendererHardwareFighterLoadMatrix4x4(input->gx_seed);
            ndsFighterPacketRecordSeed(input->gx_seed);
        }
    }
    else
    {
        MATRIX_RESTORE = input->gx_parent_slot;
        ndsFighterPacketCmd1(REG2ID(MATRIX_RESTORE),
                             (u32)input->gx_parent_slot);
        gNdsR2GxComposeRestores++;
    }
    for (i = 0u; i < (u32)input->gx_local_count; i++)
    {
        ndsRendererHardwareFighterMultMatrix4x3(&input->gx_locals[i]);
        ndsFighterPacketRecordLocal(i, &input->gx_locals[i]);
    }
    gNdsR2GxComposeMults += (u32)input->gx_local_count;
    if (input->gx_store_slot < NDS_RENDERER_FIGHTER_GX_SLOT_NONE)
    {
        MATRIX_STORE = input->gx_store_slot;
        ndsFighterPacketCmd1(REG2ID(MATRIX_STORE),
                             (u32)input->gx_store_slot);
        gNdsR2GxComposeStores++;
    }
    ndsRendererHardwareFighterMultMatrixWorldScaled(&sNdsR2GxIdentity20p12);
    ndsFighterPacketRecordWorldScaled(&sNdsR2GxIdentity20p12);
    gNdsR2GxComposeRoots++;

    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED;
    sNdsRendererHardwareMatrixGeneration = generation;
    sNdsRendererHardwareMatrixLoaded = FALSE;
}

#endif
#endif
#endif

static void ndsRendererLoadHardwareMatrices(
    const NDSRendererTraversalState *state, u32 scale_world)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;

    if ((state != NULL) && (scale_world != 0u) &&
        (state->matrix_valid != 0u))
    {
        if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
            (sNdsRendererHardwareMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareMatrixGeneration ==
             state->matrix_generation))
        {
            return;
        }
        ndsRendererLoadHardwareRawComposedMatrix(
            &state->matrix, state->matrix_generation);
        return;
    }

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY) &&
        (sNdsRendererHardwareMatrixGeneration == 0u))
    {
        return;
    }

    ndsRendererMtxIdentity20p12(&projection);
    ndsRendererMtxIdentity20p12(&modelview);
    ndsRendererLoadHardwareMatrixPair(
        &projection, &modelview,
        NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY, 0u, FALSE);
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u64 ndsRendererHardwareAbsS64(s64 value)
{
    return (value < 0) ? (u64)(-(value + 1)) + 1u : (u64)value;
}

static u64 ndsRendererHardwareAbsDiffS64(s64 lhs, s64 rhs)
{
    if ((lhs < 0) == (rhs < 0))
    {
        return (lhs >= rhs) ? (u64)(lhs - rhs) : (u64)(rhs - lhs);
    }
    return ndsRendererHardwareAbsS64(lhs) + ndsRendererHardwareAbsS64(rhs);
}

static u32 ndsRendererHardwarePosTestCrossError(
    s32 hardware_axis, s32 hardware_w,
    s32 cpu_axis, s32 cpu_w)
{
    s64 lhs = (s64)hardware_axis * cpu_w;
    s64 rhs = (s64)cpu_axis * hardware_w;
    u64 error = ndsRendererHardwareAbsDiffS64(lhs, rhs);
    u64 scale = ndsRendererHardwareAbsS64(cpu_axis) +
                ndsRendererHardwareAbsS64(cpu_w);
    u64 normalized;

    if (scale == 0u)
    {
        scale = 1u;
    }
    normalized = (error / scale) + ((error % scale) != 0u);
    return (normalized > UINT_MAX) ? UINT_MAX : (u32)normalized;
}

static u32 ndsRendererHardwarePosTestInside(s32 axis, s32 w)
{
    return (ndsRendererHardwareAbsS64(axis) <=
            ndsRendererHardwareAbsS64(w)) ? TRUE : FALSE;
}

static void ndsRendererHardwareQueueRawMatrixPosTestValues(
    const NDSRendererMatrix20p12 *matrix, u32 generation,
    const NDSRendererInputVertex *input,
    const NDSRendererClipVertex20p12 *clip, u32 matrix_word)
{
    NDSRendererHardwarePendingPosTest *probe;

    if ((matrix == NULL) || (input == NULL) || (clip == NULL) ||
        (generation == 0u) ||
        (generation == sNdsRendererHardwarePendingPosTestLastGeneration))
    {
        return;
    }
    sNdsRendererHardwarePendingPosTestLastGeneration =
        generation;
    if (sNdsRendererHardwarePendingPosTestCount >=
        NDS_RENDERER_HW_POS_TEST_MAX)
    {
        gNdsRendererProfileMatrixPosTestDropped++;
        return;
    }

    probe = &sNdsRendererHardwarePendingPosTests[
        sNdsRendererHardwarePendingPosTestCount++];
    probe->matrix = *matrix;
    probe->input = *input;
    probe->clip = *clip;
    probe->generation = generation;
    probe->matrix_word = matrix_word;
}

static void ndsRendererHardwareQueueRawMatrixPosTest(
    const NDSRendererTraversalState *state, u32 index)
{
    if ((state == NULL) || (index >= NDS_RENDERER_MAX_VTX) ||
        (state->matrix_valid == 0u))
    {
        return;
    }
    ndsRendererHardwareQueueRawMatrixPosTestValues(
        &state->matrix, state->matrix_generation,
        &state->input_vertices[index], &state->vertices[index],
        state->matrix_word_valid);
}

static void ndsRendererHardwareQueueSnapshotMatrixPosTest(
    const NDSRendererTraversalState *state, u32 snapshot_id, u32 index)
{
    const NDSRendererMatrixSnapshot *snapshot =
        ndsRendererGetMatrixSnapshot(state, snapshot_id);

    if ((snapshot == NULL) || (index >= NDS_RENDERER_MAX_VTX))
    {
        return;
    }
    ndsRendererHardwareQueueRawMatrixPosTestValues(
        &snapshot->matrix, snapshot->generation,
        &state->input_vertices[index], &state->vertices[index], FALSE);
}

static void ndsRendererHardwareQueueMatrixWordPosTestFixture(void)
{
    NDSRendererHardwarePendingPosTest *base;
    NDSRendererHardwarePendingPosTest *probe;
    NDSRendererTraversalState state;
    NDSRendererStats stats;
    NDSRendererMatrix20p12 target_matrix;
    Mtx target_raw;
    const u32 *target_words;
    u32 *current_words;
    u32 i;

    if (sNdsRendererHardwarePendingPosTestCount == 0u)
    {
        return;
    }
    for (i = 0u; i < sNdsRendererHardwarePendingPosTestCount; i++)
    {
        if (sNdsRendererHardwarePendingPosTests[i].matrix_word != 0u)
        {
            return;
        }
    }

    /*
     * The current Pupupu frame does not naturally issue G_MW_MATRIX. Derive
     * one backend-only fixture from its first eligible matrix so profile 2
     * still proves the exact MVP-recalc + matrix-word reconstruction used by
     * BattleShip. This runs after the submitted triangle batch has closed and
     * cannot alter production geometry.
     */
    base = &sNdsRendererHardwarePendingPosTests[0];
    memset(&state, 0, sizeof(state));
    memset(&stats, 0, sizeof(stats));
    state.matrix = base->matrix;
    state.matrix_valid = TRUE;
    ndsRendererApplyMvpRecalcCommand(&stats, &state, 1u, 0u);

    target_matrix = state.matrix;
    if (target_matrix.m[3][0] <=
        (INT_MAX - NDS_RENDERER_HW_POS_TEST_MATRIX_WORD_DELTA))
    {
        target_matrix.m[3][0] +=
            NDS_RENDERER_HW_POS_TEST_MATRIX_WORD_DELTA;
    }
    else
    {
        target_matrix.m[3][0] -=
            NDS_RENDERER_HW_POS_TEST_MATRIX_WORD_DELTA;
    }
    ndsRendererMtxStoreDS20p12ToN64(&target_matrix, &target_raw);
    target_words = (const u32 *)&target_raw.m[0][0];
    current_words = (u32 *)&state.matrix_word_raw.m[0][0];
    for (i = 0u; i < NDS_RENDERER_MATRIX_WORD_COUNT; i++)
    {
        if (current_words[i] != target_words[i])
        {
            ndsRendererApplyMatrixMoveWordCommand(
                &stats, &state,
                (NDS_RENDERER_MOVEWORD_MATRIX <<
                 NDS_RENDERER_MOVEWORD_INDEX_SHIFT) |
                    (i * NDS_RENDERER_MATRIX_WORD_BYTES),
                target_words[i]);
            current_words = (u32 *)&state.matrix_word_raw.m[0][0];
        }
    }
    if ((stats.matrix_mvp_recalc_count != 1u) ||
        (stats.matrix_move_word_count == 0u))
    {
        gNdsRendererProfileMatrixPosTestDropped++;
        return;
    }

    if (sNdsRendererHardwarePendingPosTestCount <
        NDS_RENDERER_HW_POS_TEST_MAX)
    {
        probe = &sNdsRendererHardwarePendingPosTests[
            sNdsRendererHardwarePendingPosTestCount++];
    }
    else
    {
        probe = &sNdsRendererHardwarePendingPosTests[
            NDS_RENDERER_HW_POS_TEST_MAX - 1u];
    }
    probe->matrix = state.matrix;
    probe->input = base->input;
    ndsRendererTransformVertex20p12(&probe->matrix, &probe->input,
                                    &probe->clip);
    probe->generation = state.matrix_generation;
    probe->matrix_word = TRUE;
}

static void ndsRendererHardwareRunRawMatrixPosTests(void)
{
    u32 i;

    ndsRendererHardwareQueueMatrixWordPosTestFixture();

    for (i = 0u; i < sNdsRendererHardwarePendingPosTestCount; i++)
    {
        const NDSRendererHardwarePendingPosTest *probe =
            &sNdsRendererHardwarePendingPosTests[i];
        v16 x = ndsRendererHardwareVertexCoord(probe->input.x, TRUE);
        v16 y = ndsRendererHardwareVertexCoord(probe->input.y, TRUE);
        v16 z = ndsRendererHardwareVertexCoord(probe->input.z, TRUE);
        s32 hardware_x;
        s32 hardware_y;
        s32 hardware_z;
        s32 hardware_w;
        u32 error_x;
        u32 error_y;
        u32 error_z;
        u32 max_error;
        u32 w_sign_mismatch;
        u32 clip_mismatch;

        ndsRendererLoadHardwareRawComposedMatrix(
            &probe->matrix, probe->generation);
        PosTest(x, y, z);
        hardware_x = PosTestXresult();
        hardware_y = PosTestYresult();
        hardware_z = PosTestZresult();
        hardware_w = PosTestWresult();
        error_x = ndsRendererHardwarePosTestCrossError(
            hardware_x, hardware_w, probe->clip.x, probe->clip.w);
        error_y = ndsRendererHardwarePosTestCrossError(
            hardware_y, hardware_w, probe->clip.y, probe->clip.w);
        error_z = ndsRendererHardwarePosTestCrossError(
            hardware_z, hardware_w, probe->clip.z, probe->clip.w);
        max_error = error_x;
        if (error_y > max_error) { max_error = error_y; }
        if (error_z > max_error) { max_error = error_z; }
        w_sign_mismatch = (((hardware_w < 0) != (probe->clip.w < 0)) ||
                           ((hardware_w == 0) != (probe->clip.w == 0))) ?
                              TRUE : FALSE;
        clip_mismatch =
            ((ndsRendererHardwarePosTestInside(hardware_x, hardware_w) !=
              ndsRendererHardwarePosTestInside(probe->clip.x,
                                               probe->clip.w)) ||
             (ndsRendererHardwarePosTestInside(hardware_y, hardware_w) !=
              ndsRendererHardwarePosTestInside(probe->clip.y,
                                               probe->clip.w)) ||
             (ndsRendererHardwarePosTestInside(hardware_z, hardware_w) !=
              ndsRendererHardwarePosTestInside(probe->clip.z,
                                               probe->clip.w))) ? TRUE : FALSE;

        gNdsRendererProfileMatrixPosTestSamples++;
        if (probe->matrix_word != 0u)
        {
            gNdsRendererProfileMatrixPosTestMatrixWordSamples++;
        }
        if (max_error > gNdsRendererProfileMatrixPosTestMaxError)
        {
            gNdsRendererProfileMatrixPosTestMaxError = max_error;
        }
        if (w_sign_mismatch != FALSE)
        {
            gNdsRendererProfileMatrixPosTestWSignMismatches++;
        }
        if (clip_mismatch != FALSE)
        {
            gNdsRendererProfileMatrixPosTestClipMismatches++;
        }
        if ((max_error > NDS_RENDERER_HW_POS_TEST_TOLERANCE) ||
            (w_sign_mismatch != FALSE) || (clip_mismatch != FALSE))
        {
            gNdsRendererProfileMatrixPosTestMismatches++;
        }
    }
    sNdsRendererHardwarePendingPosTestCount = 0u;
    sNdsRendererHardwarePendingPosTestLastGeneration = 0u;
}
#endif

static s32 ndsRendererHardwareNextProjectedDepth(void)
{
    /* The stored counter is scaled by STEP so each painter primitive must
     * consume one complete submitted-v16 depth value.  Subtracting one here
     * made six consecutive no-Z triangles share a depth after division,
     * allowing an earlier stage triangle to reject a later grass/bush draw. */
    sNdsRendererHardwareProjectedDepth -=
        NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;
    return sNdsRendererHardwareProjectedDepth /
        NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;
}

#if NDS_TICK_HUD
/* G3 step 5. Slots consumed in the background band before this frame's first
 * source-Z triangle flipped the counter. Captured at the transition because the
 * counter is reloaded there and the value is otherwise unrecoverable. */
static u32 sNdsPainterSlotBgUsed;

static u32 ndsRendererHardwarePainterSlotsUsed(s32 start)
{
    s32 used = (start - sNdsRendererHardwareProjectedDepth) /
        NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;

    return (used > 0) ? (u32)used : 0u;
}
#endif

static void ndsRendererHardwareEnterProjectedForeground(void)
{
    if (sNdsRendererHardwareProjectedBackground == FALSE)
    {
        return;
    }

#if NDS_TICK_HUD
    sNdsPainterSlotBgUsed = ndsRendererHardwarePainterSlotsUsed(
        NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START);
#endif
    /* The DS cannot disable depth testing per polygon. Mirror sm64-nds'
     * source G_ZBUFFER transition: early no-Z background draws count down
     * from the far endpoint, then the first source-Z triangle moves later
     * no-Z painter passes in front of the source depth range. */
    sNdsRendererHardwareProjectedDepth =
        NDS_RENDERER_HW_PROJECTED_DEPTH_FOREGROUND_START;
    sNdsRendererHardwareProjectedBackground = FALSE;
}

#if NDS_TICK_HUD
/* Folded once per renderer-owned hardware frame, immediately before the depth
 * counter is reset, so gNdsPainterSlotFrames doubles as the engagement proof
 * (it must track the frame serial). Derived from the counter rather than
 * incremented inside ndsRendererHardwareNextProjectedDepth because the M3
 * replay path decrements it in bulk without calling the accessor. */
static void ndsRendererHardwarePainterSlotFoldFrame(void)
{
    u32 bg_used;
    u32 fg_used;
    u32 total;

    if (sNdsRendererHardwareProjectedBackground != FALSE)
    {
        bg_used = ndsRendererHardwarePainterSlotsUsed(
            NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START);
        fg_used = 0u;
    }
    else
    {
        bg_used = sNdsPainterSlotBgUsed;
        fg_used = ndsRendererHardwarePainterSlotsUsed(
            NDS_RENDERER_HW_PROJECTED_DEPTH_FOREGROUND_START);
    }
    total = bg_used + fg_used;

    gNdsPainterSlotFrames++;
    gNdsPainterSlotBgSum += bg_used;
    gNdsPainterSlotFgSum += fg_used;
    if (bg_used > gNdsPainterSlotBgMax)
    {
        gNdsPainterSlotBgMax = bg_used;
    }
    if (fg_used > gNdsPainterSlotFgMax)
    {
        gNdsPainterSlotFgMax = fg_used;
    }
    if (total > gNdsPainterSlotTotalMax)
    {
        gNdsPainterSlotTotalMax = total;
    }
    if (bg_used > 128u)
    {
        gNdsPainterSlotBgOverBand++;
    }
    if (fg_used > 128u)
    {
        gNdsPainterSlotFgOverBand++;
    }
    sNdsPainterSlotBgUsed = 0u;
}
#endif

static void ndsRendererHardwareClipVertex(
    const NDSRendererClipVertex20p12 *vtx, s32 z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    , NDSRendererSemanticVertex *semantic_vertex
#endif
    )
{
    v16 x;
    v16 y;
    v16 out_z;

    if ((vtx == NULL) || (vtx->w == 0))
    {
        return;
    }

    x = ndsRendererHardwareProjectToV16(
        (s64)vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    y = ndsRendererHardwareProjectToV16(
        (s64)vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    out_z = ndsRendererHardwareSourceDepthToV16(
        (s64)z * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        semantic_vertex->x = x;
        semantic_vertex->y = y;
        semantic_vertex->z = out_z;
        semantic_vertex->valid_flags |=
            NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID;
    }
#endif
    ndsRendererProfileHWVertexRange(x, y, out_z);
    glVertex3v16(x, y, out_z);
}

static void ndsRendererHardwareClipVertexNdcDepth(
    const NDSRendererClipVertex20p12 *vtx, s32 z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    , NDSRendererSemanticVertex *semantic_vertex
#endif
    )
{
    v16 x;
    v16 y;
    v16 out_z;

    if ((vtx == NULL) || (vtx->w == 0))
    {
        return;
    }
    x = ndsRendererHardwareProjectToV16(
        (s64)vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    y = ndsRendererHardwareProjectToV16(
        (s64)vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    out_z = ndsRendererHardwareClampS64ToV16(z);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        semantic_vertex->x = x;
        semantic_vertex->y = y;
        semantic_vertex->z = out_z;
        semantic_vertex->valid_flags |=
            NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID;
    }
#endif
    ndsRendererProfileHWVertexRange(x, y, out_z);
    glVertex3v16(x, y, out_z);
}

static void NDS_R2_ITCM_PACK2_CODE ndsRendererHardwareEndBatch(void)
{
    if (sNdsRendererHardwareTriangleBatchOpen != 0u)
    {
        /* libnds documents glEnd() as a dummy FIFO write. A later glBegin()
         * starts the next primitive group, so only restore state owned by the
         * logical source-triangle batch here. */
        glDisable(GL_ALPHA_TEST);
        ndsRendererProfileRecordBatchEnd();
        sNdsRendererHardwareTriangleBatchOpen = FALSE;
        sNdsRendererHardwareTriangleBatchTextured = FALSE;
        sNdsRendererHardwareTriangleBatchTextureName = 0u;
        sNdsRendererHardwareTriangleBatchPolyFmt = 0u;
        sNdsRendererHardwareTriangleBatchAlphaKey = 0u;
        sNdsRendererHardwareTriangleBatchFogKey = 0u;
        sNdsRendererHardwareTriangleBatchMatrixMode =
            NDS_RENDERER_HW_MATRIX_MODE_NONE;
        sNdsRendererHardwareTriangleBatchMatrixGeneration = 0u;
    }
}

static void ndsRendererHardwareBeginTriangleBatch(
    const NDSRendererStats *stats,
    u32 textured,
    u32 texture_name,
    u32 poly_fmt,
    u32 matrix_mode,
    u32 matrix_generation)
{
    u32 alpha_key;
    u32 fog_key;

    /* An open GX batch can only contain adjacent TRI1/TRI2 source commands.
     * Every opcode capable of changing alpha, fog, texture, or matrix state
     * closes it in ndsRendererScanList before mutation. Keep the per-triangle
     * reuse check to the values that can differ through vertex selection. */
    if ((sNdsRendererHardwareTriangleBatchOpen != 0u) &&
        (sNdsRendererHardwareTriangleBatchTextured == textured) &&
        (sNdsRendererHardwareTriangleBatchTextureName == texture_name) &&
        (sNdsRendererHardwareTriangleBatchPolyFmt == poly_fmt) &&
        (sNdsRendererHardwareTriangleBatchMatrixMode == matrix_mode) &&
        (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
         matrix_generation))
    {
        ndsRendererProfileRecordBatchReuse();
        return;
    }

    alpha_key = ndsRendererHardwareAlphaStateKey(stats);
    fog_key = ndsRendererHardwareFogStateKey(stats);
    ndsRendererHardwareEndBatch();
    if (textured != 0u)
    {
        glEnable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        ndsRendererHardwareBindNoTexture(NULL);
    }
    ndsRendererHardwareApplyAlphaTest(stats);
    ndsRendererHardwareApplyFog(stats);
    ndsRendererHardwareSetPolyFmt(poly_fmt);
    glBegin(GL_TRIANGLE);
    ndsRendererProfileRecordBatchBegin();

    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = textured;
    sNdsRendererHardwareTriangleBatchTextureName = texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = alpha_key;
    sNdsRendererHardwareTriangleBatchFogKey = fog_key;
    sNdsRendererHardwareTriangleBatchMatrixMode = matrix_mode;
    sNdsRendererHardwareTriangleBatchMatrixGeneration = matrix_generation;
}

/* c200's whole-match PC census never enters either of these position-emission
 * branches.  Keep the generic renderer exact for callers that need them, but
 * do not make the hot vertex submitter carry their inlined bodies in ITCM. */
static void __attribute__((noinline, cold))
ndsRendererHardwareSubmitVertexRawZCold(
    const NDSRendererInputVertex *vtx, u32 scale_world)
{
    v16 x = ndsRendererHardwareVertexCoord(vtx->x, scale_world);
    v16 y = ndsRendererHardwareVertexCoord(vtx->y, scale_world);
    v16 z = ndsRendererHardwareVertexCoord(vtx->z, scale_world);

    ndsRendererProfileVertexRange(vtx, x, y, z);
    glVertex3v16(x, y, z);
}

static void __attribute__((noinline, cold))
ndsRendererHardwareSubmitVertexDecalCold(
    const NDSRendererClipVertex20p12 *clip_vtx, s32 projected_z)
{
    (void)projected_z;
    if (clip_vtx != NULL)
    {
        ndsRendererHardwareClipVertex(
            clip_vtx, clip_vtx->z - NDS_RENDERER_HW_DECAL_DEPTH_BIAS);
    }
}

static void NDS_R2_ITCM_PACK2_EVICTED_CODE
ndsRendererHardwareSubmitVertex(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 vertex_index,
    s32 projected_z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    , NDSRendererSemanticVertex *semantic_vertex
#endif
    )
{
    const NDSRendererInputVertex *vtx;
    const NDSRendererClipVertex20p12 *clip_vtx;
    u32 material_color;
    u32 scale_s;
    u32 scale_t;
    u32 texture_origin_s;
    u32 texture_origin_t;
    u32 context_flags;
    u32 scale_world;
    u32 vertex_color;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 vertex_color_valid;
    s32 use_texture;
    s32 texture_offset;
    s32 zbuffered;
    s32 decal_depth;
    s32 prim_depth;
    s32 source_clip_depth;
    u16 hardware_color;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        memset(semantic_vertex, 0, sizeof(*semantic_vertex));
    }
#endif
    if ((stats == NULL) || (state == NULL) ||
        (vertex_index >= NDS_RENDERER_MAX_VTX))
    {
        return;
    }
    vtx = &state->input_vertices[vertex_index];
    clip_vtx = &state->vertices[vertex_index];
    material_color = state->texture_prepare_material_color;
    scale_s = state->texture_prepare_scale_s;
    scale_t = state->texture_prepare_scale_t;
    texture_origin_s = state->texture_prepare_origin_s;
    texture_origin_t = state->texture_prepare_origin_t;
    context_flags = state->texture_prepare_vertex_flags;
    scale_world = context_flags & NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD;
    vertex_color = state->vertex_colors[vertex_index];
    use_material_color =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL;
    use_vertex_color =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX;
    vertex_color_valid =
        ((state->vertex_color_valid_mask & (1u << vertex_index)) != 0u) ?
            TRUE : FALSE;
    use_texture = context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
    texture_offset = state->texture_prepare_offset;
    zbuffered = context_flags & NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED;
    decal_depth = context_flags & NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH;
    prim_depth = context_flags & NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH;
    source_clip_depth =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH;

#if NDS_RENDERER_PROFILE_LEVEL < 2
    if ((state != NULL) &&
        (vertex_index < NDS_RENDERER_MAX_VTX) &&
        ((state->prepared_vertex_color_valid_mask &
          (1u << vertex_index)) != 0u))
    {
        hardware_color = state->prepared_vertex_colors[vertex_index];
    }
    else
#endif
    {
        hardware_color = ndsRendererHardwarePackedVertexColor(
            stats, vtx, material_color, use_material_color,
            use_vertex_color, vertex_color, vertex_color_valid,
            state->color_modulate);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((state != NULL) && (vertex_index < NDS_RENDERER_MAX_VTX))
        {
            state->prepared_vertex_colors[vertex_index] = hardware_color;
            state->prepared_vertex_color_valid_mask |= 1u << vertex_index;
        }
#endif
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        semantic_vertex->color = hardware_color;
        semantic_vertex->valid_flags |=
            NDS_RENDERER_SEMANTIC_VERTEX_COLOR_VALID;
    }
#endif
    glColor(hardware_color);
    if (use_texture != FALSE)
    {
        s16 s;
        s16 t;

#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((state != NULL) &&
            (vertex_index < NDS_RENDERER_MAX_VTX) &&
            ((state->prepared_texcoord_valid_mask &
              (1u << vertex_index)) != 0u))
        {
            s = state->prepared_texcoord_s[vertex_index];
            t = state->prepared_texcoord_t[vertex_index];
        }
        else
#endif
        {
            s = ndsRendererHardwareTexCoord(vtx->s, scale_s,
                                            texture_origin_s,
                                            texture_offset);
            t = ndsRendererHardwareTexCoord(vtx->t, scale_t,
                                            texture_origin_t,
                                            texture_offset);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            if ((state != NULL) && (vertex_index < NDS_RENDERER_MAX_VTX))
            {
                state->prepared_texcoord_s[vertex_index] = s;
                state->prepared_texcoord_t[vertex_index] = t;
                state->prepared_texcoord_valid_mask |= 1u << vertex_index;
            }
#endif
        }

#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererProfileTextureCoord(s, t);
        ndsRendererProfileTextureSample(s, t);
        if (semantic_vertex != NULL)
        {
            semantic_vertex->s = s;
            semantic_vertex->t = t;
            semantic_vertex->valid_flags |=
                NDS_RENDERER_SEMANTIC_VERTEX_ST_VALID;
        }
#endif
        glTexCoord2t16(s, t);
    }
    if ((zbuffered != FALSE) &&
        (decal_depth == FALSE) &&
        (prim_depth == FALSE))
    {
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ndsRendererHardwareSubmitVertexRawZCold(vtx, scale_world);
#else
        v16 x = ndsRendererHardwareVertexCoord(vtx->x, scale_world);
        v16 y = ndsRendererHardwareVertexCoord(vtx->y, scale_world);
        v16 z = ndsRendererHardwareVertexCoord(vtx->z, scale_world);
        if (semantic_vertex != NULL)
        {
            semantic_vertex->x = x;
            semantic_vertex->y = y;
            semantic_vertex->z = z;
            semantic_vertex->valid_flags |=
                NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID;
        }
        ndsRendererProfileVertexRange(vtx, x, y, z);
        glVertex3v16(x, y, z);
#endif
    }
    else if (prim_depth != FALSE)
    {
        ndsRendererHardwareClipVertex(clip_vtx, projected_z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                      , semantic_vertex
#endif
                                      );
    }
    else if (decal_depth != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ndsRendererHardwareSubmitVertexDecalCold(clip_vtx, projected_z);
#else
        if (clip_vtx != NULL)
        {
            ndsRendererHardwareClipVertex(
                clip_vtx, clip_vtx->z - NDS_RENDERER_HW_DECAL_DEPTH_BIAS,
                semantic_vertex);
        }
#endif
    }
    else
    {
        /* X, Y, Z, and W must come from the same composed matrix snapshot.
         * Matrix-word updates can make projection/modelview fields stale. */
        if ((source_clip_depth != FALSE) && (clip_vtx != NULL) &&
            (clip_vtx->w != 0))
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            s32 depth = ndsRendererHardwareSourceDepthToV16(
                (s64)clip_vtx->z * NDS_RENDERER_HW_PROJECTED_VERTEX,
                clip_vtx->w);

            if (stats->hardware_projected_depth_sample_count == 0u)
            {
                stats->hardware_projected_depth_min = depth;
                stats->hardware_projected_depth_max = depth;
                stats->hardware_projected_w_min = clip_vtx->w;
                stats->hardware_projected_w_max = clip_vtx->w;
            }
            else
            {
                if (depth < stats->hardware_projected_depth_min)
                {
                    stats->hardware_projected_depth_min = depth;
                }
                if (depth > stats->hardware_projected_depth_max)
                {
                    stats->hardware_projected_depth_max = depth;
                }
                if (clip_vtx->w < stats->hardware_projected_w_min)
                {
                    stats->hardware_projected_w_min = clip_vtx->w;
                }
                if (clip_vtx->w > stats->hardware_projected_w_max)
                {
                    stats->hardware_projected_w_max = clip_vtx->w;
                }
            }
            stats->hardware_projected_depth_sample_count++;
#endif
            projected_z = clip_vtx->z;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((state != NULL) &&
            (vertex_index < NDS_RENDERER_MAX_VTX) &&
            (clip_vtx != NULL) && (clip_vtx->w != 0))
        {
            u32 vertex_mask = 1u << vertex_index;
            v16 x;
            v16 y;
            v16 z;

            if ((state->prepared_projected_xy_valid_mask & vertex_mask) !=
                0u)
            {
                x = state->prepared_projected_x[vertex_index];
                y = state->prepared_projected_y[vertex_index];
            }
            else
            {
                x = ndsRendererHardwareProjectToV16(
                    (s64)clip_vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX,
                    clip_vtx->w);
                y = ndsRendererHardwareProjectToV16(
                    (s64)clip_vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX,
                    clip_vtx->w);
                state->prepared_projected_x[vertex_index] = x;
                state->prepared_projected_y[vertex_index] = y;
                state->prepared_projected_xy_valid_mask |= vertex_mask;
            }
            if (source_clip_depth != FALSE)
            {
                if ((state->prepared_projected_source_z_valid_mask &
                     vertex_mask) != 0u)
                {
                    z = state->prepared_projected_source_z[vertex_index];
                }
                else
                {
                    z = ndsRendererHardwareSourceDepthToV16(
                        (s64)projected_z *
                            NDS_RENDERER_HW_PROJECTED_VERTEX,
                        clip_vtx->w);
                    state->prepared_projected_source_z[vertex_index] = z;
                    state->prepared_projected_source_z_valid_mask |=
                        vertex_mask;
                }
            }
            else
            {
                z = ndsRendererHardwareClampS64ToV16(projected_z);
            }
            glVertex3v16(x, y, z);
        }
        else if (source_clip_depth != FALSE)
        {
            ndsRendererHardwareClipVertex(clip_vtx, projected_z);
        }
        else
        {
            ndsRendererHardwareClipVertexNdcDepth(clip_vtx, projected_z);
        }
#else
        if (source_clip_depth != FALSE)
        {
            ndsRendererHardwareClipVertex(
                clip_vtx, projected_z, semantic_vertex);
        }
        else
        {
            ndsRendererHardwareClipVertexNdcDepth(
                clip_vtx, projected_z, semantic_vertex);
        }
#endif
    }
}

static s32 ndsRendererInputTriangleReady(
    const NDSRendererTraversalState *state, u32 packed,
    u32 *out_i0, u32 *out_i1, u32 *out_i2)
{
    u32 i0;
    u32 i1;
    u32 i2;
    u32 mask;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);
    if (out_i0 != NULL) { *out_i0 = i0; }
    if (out_i1 != NULL) { *out_i1 = i1; }
    if (out_i2 != NULL) { *out_i2 = i2; }

    if ((state == NULL) ||
        (i0 >= NDS_RENDERER_MAX_VTX) ||
        (i1 >= NDS_RENDERER_MAX_VTX) ||
        (i2 >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }

    mask = (1u << i0) | (1u << i1) | (1u << i2);
    return ((state->input_vertex_valid_mask & mask) == mask) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareClipZWInsideNearPlane(s32 z, s32 w)
{
    return ((w > 0) && (((s64)z + (s64)w) >= 0)) ? TRUE : FALSE;
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static s32 __attribute__((optimize("Os"))) ndsRendererHardwareClipLerpQ16(
    s32 from, s32 to, s32 ratio_q16)
{
    s64 delta = (s64)to - (s64)from;

    return (s32)((s64)from + ((delta * ratio_q16) >> 16));
}

static u16 __attribute__((optimize("Os")))
ndsRendererHardwareClipLerpColorQ16(
    u16 from, u16 to, s32 ratio_q16)
{
    u32 r = (u32)ndsRendererHardwareClipLerpQ16(
        from & 31u, to & 31u, ratio_q16);
    u32 g = (u32)ndsRendererHardwareClipLerpQ16(
        (from >> 5) & 31u, (to >> 5) & 31u, ratio_q16);
    u32 b = (u32)ndsRendererHardwareClipLerpQ16(
        (from >> 10) & 31u, (to >> 10) & 31u, ratio_q16);

    return (u16)(r | (g << 5) | (b << 10));
}

static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererHardwareClipNearIntersection(
    const NDSRendererProjectedClipVertex *from,
    const NDSRendererProjectedClipVertex *to,
    NDSRendererProjectedClipVertex *out)
{
    s32 from_distance = (s32)(
        ((s64)from->clip.z + (s64)from->clip.w) / 4);
    s32 to_distance = (s32)(
        ((s64)to->clip.z + (s64)to->clip.w) / 4);
    s32 denominator = from_distance - to_distance;
    s32 ratio_q16;

    if (denominator == 0)
    {
        *out = *from;
        return;
    }
#if defined(__arm__)
    ratio_q16 = (s32)ndsR2HwMathDiv64((s64)from_distance * 65536, denominator);
#else
    ratio_q16 = (s32)(((s64)from_distance * 65536) / denominator);
#endif
    out->clip.x = ndsRendererHardwareClipLerpQ16(
        from->clip.x, to->clip.x, ratio_q16);
    out->clip.y = ndsRendererHardwareClipLerpQ16(
        from->clip.y, to->clip.y, ratio_q16);
    out->clip.w = ndsRendererHardwareClipLerpQ16(
        from->clip.w, to->clip.w, ratio_q16);
    out->clip.z = -out->clip.w;
    out->s = ndsRendererHardwareClipLerpQ16(
        from->s, to->s, ratio_q16);
    out->t = ndsRendererHardwareClipLerpQ16(
        from->t, to->t, ratio_q16);
    out->packed_color = ndsRendererHardwareClipLerpColorQ16(
        from->packed_color, to->packed_color, ratio_q16);
}

static u32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererHardwareClipTriangleNearPlane(
    const NDSRendererProjectedClipVertex input[3],
    NDSRendererProjectedClipVertex output[4])
{
    const NDSRendererProjectedClipVertex *previous = &input[2];
    s32 previous_inside = ndsRendererHardwareClipZWInsideNearPlane(
        previous->clip.z, previous->clip.w);
    u32 input_index;
    u32 output_count = 0u;

    for (input_index = 0u; input_index < 3u; input_index++)
    {
        const NDSRendererProjectedClipVertex *current = &input[input_index];
        s32 current_inside = ndsRendererHardwareClipZWInsideNearPlane(
            current->clip.z, current->clip.w);

        if (current_inside != FALSE)
        {
            if (previous_inside == FALSE)
            {
                ndsRendererHardwareClipNearIntersection(
                    previous, current, &output[output_count++]);
            }
            output[output_count++] = *current;
        }
        else if (previous_inside != FALSE)
        {
            ndsRendererHardwareClipNearIntersection(
                previous, current, &output[output_count++]);
        }
        previous = current;
        previous_inside = current_inside;
    }
    return output_count;
}

static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererHardwareEmitClippedVertex(
    const NDSRendererProjectedClipVertex *vertex,
    u32 context_flags,
    s32 projected_z)
{
    glColor(vertex->packed_color);
    if ((context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE) != 0u)
    {
        glTexCoord2t16((s16)vertex->s, (s16)vertex->t);
    }
    /* Same depth choice ndsRendererHardwareSubmitVertex makes, on the clipped
     * vertex instead of the input one.  Its remaining branch -- raw
     * model-space glVertex3v16 -- cannot occur here: it needs a zbuffered,
     * non-decal, non-prim triangle, and a CPU-projected one of those has
     * already had zbuffered cleared by the caller. */
    if ((context_flags & NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH) != 0u)
    {
        ndsRendererHardwareClipVertex(&vertex->clip, projected_z);
    }
    else if ((context_flags & NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH) != 0u)
    {
        ndsRendererHardwareClipVertex(
            &vertex->clip,
            vertex->clip.z - NDS_RENDERER_HW_DECAL_DEPTH_BIAS);
    }
    else if ((context_flags &
              NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH) != 0u)
    {
        ndsRendererHardwareClipVertex(&vertex->clip, vertex->clip.z);
    }
    else
    {
        ndsRendererHardwareClipVertexNdcDepth(&vertex->clip, projected_z);
    }
}

/* The source RSP clips at the near plane before the perspective divide, so a
 * triangle that straddles it still draws its front part.  The port used to
 * drop the whole triangle here, which is invisible in the automated capture
 * (the fixed camera never crosses the plane) but opens holes in geometry the
 * player can push into the camera -- BUGS.md #10.  Clip it the way the native
 * stage path already does, reusing that path's proven clipper. */
static u32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererHardwareSubmitNearClippedTriangle(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 i0, u32 i1, u32 i2,
    s32 projected_z)
{
    NDSRendererProjectedClipVertex input[3];
    NDSRendererProjectedClipVertex clipped[4];
    u32 corner_index[3];
    u32 context_flags = state->texture_prepare_vertex_flags;
    u32 corner;
    u32 clipped_count;
    u32 fan_index;
    u32 emitted;

    corner_index[0] = i0;
    corner_index[1] = i1;
    corner_index[2] = i2;
    for (corner = 0u; corner < 3u; corner++)
    {
        u32 vertex_index = corner_index[corner];
        u32 vertex_mask = 1u << vertex_index;
        const NDSRendererInputVertex *vtx =
            &state->input_vertices[vertex_index];

        input[corner].clip = state->vertices[vertex_index];
        if ((state->prepared_vertex_color_valid_mask & vertex_mask) != 0u)
        {
            input[corner].packed_color =
                state->prepared_vertex_colors[vertex_index];
        }
        else
        {
            input[corner].packed_color = ndsRendererHardwarePackedVertexColor(
                stats, vtx, state->texture_prepare_material_color,
                (s32)(context_flags &
                      NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL),
                (s32)(context_flags &
                      NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX),
                state->vertex_colors[vertex_index],
                ((state->vertex_color_valid_mask & vertex_mask) != 0u) ?
                    TRUE : FALSE,
                state->color_modulate);
        }
        if ((state->prepared_texcoord_valid_mask & vertex_mask) != 0u)
        {
            input[corner].s = state->prepared_texcoord_s[vertex_index];
            input[corner].t = state->prepared_texcoord_t[vertex_index];
        }
        else
        {
            input[corner].s = ndsRendererHardwareTexCoord(
                vtx->s, state->texture_prepare_scale_s,
                state->texture_prepare_origin_s,
                state->texture_prepare_offset);
            input[corner].t = ndsRendererHardwareTexCoord(
                vtx->t, state->texture_prepare_scale_t,
                state->texture_prepare_origin_t,
                state->texture_prepare_offset);
        }
    }
    clipped_count = ndsRendererHardwareClipTriangleNearPlane(input, clipped);
    if (clipped_count < 3u)
    {
        /* Wholly behind the plane: nothing to draw, and the source would not
         * have drawn it either. */
        return 0u;
    }
    emitted = 0u;
    for (fan_index = 1u; fan_index + 1u < clipped_count; fan_index++)
    {
        /* The vertex emitters drop a w==0 vertex silently, which would leave
         * this triangle two vertices long and shift every later triangle in
         * the batch.  The old near-plane reject made that unreachable by
         * requiring w>0 on all three corners; keep it unreachable. */
        if ((clipped[0].clip.w == 0) ||
            (clipped[fan_index].clip.w == 0) ||
            (clipped[fan_index + 1u].clip.w == 0))
        {
            continue;
        }
        ndsRendererHardwareEmitClippedVertex(
            &clipped[0], context_flags, projected_z);
        ndsRendererHardwareEmitClippedVertex(
            &clipped[fan_index], context_flags, projected_z);
        ndsRendererHardwareEmitClippedVertex(
            &clipped[fan_index + 1u], context_flags, projected_z);
        emitted++;
    }
    return emitted;
}
#endif

static s32 ndsRendererHardwareTriangleInsideNearPlane(
    const NDSRendererClipVertex20p12 *v0,
    const NDSRendererClipVertex20p12 *v1,
    const NDSRendererClipVertex20p12 *v2)
{
    if ((v0 == NULL) || (v1 == NULL) || (v2 == NULL))
    {
        return FALSE;
    }
    return ((ndsRendererHardwareClipZWInsideNearPlane(v0->z, v0->w) != FALSE) &&
            (ndsRendererHardwareClipZWInsideNearPlane(v1->z, v1->w) != FALSE) &&
            (ndsRendererHardwareClipZWInsideNearPlane(v2->z, v2->w) != FALSE)) ?
        TRUE : FALSE;
}

static s32 ndsRendererHardwareRawMatrixCompatible(
    const NDSRendererTraversalState *state)
{
    return ((state != NULL) && (state->matrix_valid != 0u) &&
            (state->matrix_generation != 0u)) ? TRUE : FALSE;
}

/* The bin macro is defined by the G3 capture block above, which is itself
 * nested inside the GX-record configuration guard. This classifier is not, so
 * the fallback has to be unconditional rather than an #else on that block --
 * otherwise a configuration that compiles the classifier without the capture
 * fails at the first bin call rather than simply not counting. */

static NDSRendererHWSubmitClass ndsRendererHardwareClassifySubmit(
    const NDSRendererTraversalState *state,
    u32 i0, u32 i1, u32 i2,
    s32 source_zbuffered, s32 decal_depth, s32 prim_depth,
    u32 *out_snapshot_id)
{
    u32 mask;
    u32 snapshot_id;

    if (out_snapshot_id != NULL)
    {
        *out_snapshot_id = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    }

    if (source_zbuffered == FALSE)
    {
        NDS_EFFECT_SUBMIT_BIN(0u);
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z;
    }
    if (decal_depth != FALSE)
    {
        NDS_EFFECT_SUBMIT_BIN(1u);
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL;
    }
    if (prim_depth != FALSE)
    {
        NDS_EFFECT_SUBMIT_BIN(2u);
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH;
    }
    mask = (1u << i0) | (1u << i1) | (1u << i2);
    if ((state->raw_vertex_fit_mask & mask) != mask)
    {
        ndsRendererProfileRecordRawCurrentRangeReject();
        NDS_EFFECT_SUBMIT_BIN(3u);
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX;
    }

    if ((state->current_transform_vertex_mask & mask) == mask)
    {
        if (ndsRendererHardwareRawMatrixCompatible(state) == FALSE)
        {
            /* Bin 4, NOT bin 3: both returns are the same enum value, so the
             * class alone cannot say whether the raw path was refused because
             * the vertices did not fit the v16 range or because the matrix was
             * unusable. Those are different repairs, so they get different
             * bins. */
            NDS_EFFECT_SUBMIT_BIN(4u);
            return NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX;
        }
        ndsRendererProfileRecordRawCurrentCandidate();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererHardwareQueueRawMatrixPosTest(state, i0);
#endif
        NDS_EFFECT_SUBMIT_BIN(5u);
        return NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX;
    }

    snapshot_id = state->vertex_matrix_snapshot[i0];
    if ((snapshot_id != NDS_RENDERER_MATRIX_SNAPSHOT_INVALID) &&
        (state->vertex_matrix_snapshot[i1] == snapshot_id) &&
        (state->vertex_matrix_snapshot[i2] == snapshot_id) &&
        (ndsRendererGetMatrixSnapshot(state, snapshot_id) != NULL))
    {
        if (out_snapshot_id != NULL)
        {
            *out_snapshot_id = snapshot_id;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererHardwareQueueSnapshotMatrixPosTest(
            state, snapshot_id, i0);
#endif
        NDS_EFFECT_SUBMIT_BIN(6u);
        return NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX;
    }

    ndsRendererProfileRecordRawCrossMatrix();
    NDS_EFFECT_SUBMIT_BIN(7u);
    return NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX;
}

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP
static void __attribute__((noinline)) NDS_RENDERER_HOT_CODE
ndsRendererSubmitHardwareTriangle(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 packed)
{
    (void)stats;
    (void)config;
    (void)state;
    (void)packed;
    sNdsRendererBenchmarkTriangleCount++;
}
#else
/* The c200 canonical census never takes either raw-matrix load arm or the
 * z-buffer accounting arm below.  Keep those exact generic-renderer paths
 * available in main RAM while leaving the projected/no-Z path contiguous. */
static void __attribute__((noinline, cold))
ndsRendererSubmitHardwareTriangleRawMatrixCold(
    NDSRendererTraversalState *state,
    NDSRendererHWSubmitClass submit_class,
    const NDSRendererMatrixSnapshot *raw_snapshot,
    s32 scale_world)
{
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        ndsRendererLoadHardwareMatrices(state, scale_world);
    }
    else if (raw_snapshot != NULL)
    {
        ndsRendererLoadHardwareRawComposedMatrix(
            &raw_snapshot->matrix, raw_snapshot->generation);
    }
}

static void __attribute__((noinline, cold))
ndsRendererSubmitHardwareTriangleDepthStatsCold(
    NDSRendererStats *stats, s32 decal_depth, s32 prim_depth)
{
    stats->hardware_zbuffer_triangle_count++;
    if (decal_depth != FALSE)
    {
        stats->hardware_decal_depth_triangle_count++;
    }
    if (prim_depth != FALSE)
    {
        stats->hardware_prim_depth_triangle_count++;
    }
}

static void NDS_R2_ITCM_PACK2_EVICTED_CODE
ndsRendererSubmitHardwareTriangle(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;
    const NDSRendererInputVertex *v0;
    const NDSRendererTileState *render_tile;
    s32 use_texture;
    s32 implicit_texture_on;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 scale_world;
    u32 material_color;
    u32 poly_alpha;
    u32 poly_fmt;
    u32 texture_name;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 texture_offset;
    s32 zbuffered;
    s32 source_zbuffered;
    s32 decal_depth;
    s32 prim_depth;
    s32 transformed_ready;
    s32 projected_submit;
    s32 raw_submit;
    s32 source_clip_depth;
    s32 no_oracle;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    s32 near_clipped = FALSE;
#endif
    u32 raw_snapshot_id = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    const NDSRendererMatrixSnapshot *raw_snapshot = NULL;
    NDSRendererHWSubmitClass submit_class;
    s32 projected_z[3] = { 0, 0, 0 };
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 vertex_submit_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    NDSRendererSemanticEvent semantic_event;
#endif

    if (stats == NULL)
    {
        return;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    memset(&semantic_event, 0, sizeof(semantic_event));
    semantic_event.owner = (u32)sNdsRendererProfileOwner;
    semantic_event.owner_occurrence =
        sNdsRendererSemanticSourceProvenance.owner_occurrence;
    semantic_event.list_ordinal =
        sNdsRendererSemanticSourceProvenance.list_ordinal;
    semantic_event.branch_path = (state != NULL) ?
        state->semantic_branch_path :
        sNdsRendererSemanticSourceProvenance.root_branch_path;
    semantic_event.command_index = (state != NULL) ?
        state->semantic_command_index : 0u;
    semantic_event.tri2_half = (state != NULL) ?
        state->semantic_tri2_half : 0u;
    semantic_event.outcome = NDS_RENDERER_SEMANTIC_INPUT_REJECT;
    semantic_event.packed = packed;
    semantic_event.submit_class = NDS_RENDERER_HW_SUBMIT_REJECT;
    semantic_event.source_state_hash =
        ndsRendererSemanticSourceStateHash(stats);
    semantic_event.raw_snapshot_id =
        NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    semantic_event.no_z_depth_before =
        sNdsRendererHardwareProjectedDepth;
    semantic_event.no_z_depth_after =
        sNdsRendererHardwareProjectedDepth;
    semantic_event.no_z_background_before =
        sNdsRendererHardwareProjectedBackground;
    semantic_event.no_z_background_after =
        sNdsRendererHardwareProjectedBackground;
    semantic_event.fog_color = stats->fog_color;
    semantic_event.fog_min = stats->fog_min;
    semantic_event.fog_max = stats->fog_max;
    semantic_event.fog_status = stats->fog_status;
#endif
    if (ndsRendererInputTriangleReady(state, packed, &i0, &i1, &i2) == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        semantic_event.vertex_index[0] = i0;
        semantic_event.vertex_index[1] = i1;
        semantic_event.vertex_index[2] = i2;
        ndsRendererSemanticCommitEvent(&semantic_event);
#endif
        stats->hardware_oracle_reject_count++;
        ndsRendererProfileRecordSubmitClass(
            NDS_RENDERER_HW_SUBMIT_REJECT);
        return;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.vertex_index[0] = i0;
    semantic_event.vertex_index[1] = i1;
    semantic_event.vertex_index[2] = i2;
    semantic_event.vertex_matrix_snapshot[0] =
        state->vertex_matrix_snapshot[i0];
    semantic_event.vertex_matrix_snapshot[1] =
        state->vertex_matrix_snapshot[i1];
    semantic_event.vertex_matrix_snapshot[2] =
        state->vertex_matrix_snapshot[i2];
    semantic_event.vertex_clip_snapshot[0] =
        state->vertex_clip_snapshot[i0];
    semantic_event.vertex_clip_snapshot[1] =
        state->vertex_clip_snapshot[i1];
    semantic_event.vertex_clip_snapshot[2] =
        state->vertex_clip_snapshot[i2];
#endif
    no_oracle = (sNdsRendererHardwareNoOracle != 0u) ? TRUE : FALSE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (state->texture_prepare_valid != 0u)
    {
        source_zbuffered =
            (state->texture_prepare_source_zbuffered != 0u) ? TRUE : FALSE;
        zbuffered = source_zbuffered;
        decal_depth =
            (state->texture_prepare_decal_depth != 0u) ? TRUE : FALSE;
        prim_depth =
            (state->texture_prepare_prim_depth != 0u) ? TRUE : FALSE;
    }
    else
#endif
    {
        zbuffered =
            ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) != 0u) ?
                TRUE : FALSE;
        source_zbuffered = zbuffered;
        decal_depth = ((zbuffered != FALSE) &&
                       ((stats->othermode_l & NDS_RENDERER_ZMODE_DEC) ==
                        NDS_RENDERER_ZMODE_DEC)) ? TRUE : FALSE;
        prim_depth = ((zbuffered != FALSE) &&
                      (ndsRendererHardwareUsePrimDepth(stats) != FALSE)) ?
            TRUE : FALSE;
        state->texture_prepare_source_zbuffered =
            (source_zbuffered != FALSE) ? TRUE : FALSE;
        state->texture_prepare_decal_depth =
            (decal_depth != FALSE) ? TRUE : FALSE;
        state->texture_prepare_prim_depth =
            (prim_depth != FALSE) ? TRUE : FALSE;
    }
    v0 = &state->input_vertices[i0];
    submit_class = ndsRendererHardwareClassifySubmit(
        state, i0, i1, i2, source_zbuffered, decal_depth, prim_depth,
        &raw_snapshot_id);
    raw_submit =
        ((submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) ||
         (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX)) ?
             TRUE : FALSE;
    projected_submit =
        ((source_zbuffered != FALSE) && (raw_submit == FALSE)) ? TRUE : FALSE;
    source_clip_depth = projected_submit;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.submit_class = (u32)submit_class;
    semantic_event.raw_snapshot_id = raw_snapshot_id;
    semantic_event.source_zbuffered =
        (source_zbuffered != FALSE) ? TRUE : FALSE;
    semantic_event.zbuffered = (zbuffered != FALSE) ? TRUE : FALSE;
    semantic_event.raw_submit = (raw_submit != FALSE) ? TRUE : FALSE;
    semantic_event.projected_submit =
        (projected_submit != FALSE) ? TRUE : FALSE;
    semantic_event.decal_depth = (decal_depth != FALSE) ? TRUE : FALSE;
    semantic_event.prim_depth = (prim_depth != FALSE) ? TRUE : FALSE;
    semantic_event.source_clip_depth =
        (source_clip_depth != FALSE) ? TRUE : FALSE;
#endif
    transformed_ready = TRUE;
    if (raw_submit == FALSE)
    {
        transformed_ready =
            ((ndsRendererEnsureTransformedVertex(stats, state, i0) != FALSE) &&
             (ndsRendererEnsureTransformedVertex(stats, state, i1) != FALSE) &&
             (ndsRendererEnsureTransformedVertex(stats, state, i2) != FALSE)) ?
                 TRUE : FALSE;
    }
    if (transformed_ready == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        semantic_event.outcome = NDS_RENDERER_SEMANTIC_TRANSFORM_REJECT;
        semantic_event.submit_class = NDS_RENDERER_HW_SUBMIT_REJECT;
        ndsRendererSemanticCommitEvent(&semantic_event);
#endif
        stats->hardware_oracle_reject_count++;
        ndsRendererProfileRecordSubmitClass(
            NDS_RENDERER_HW_SUBMIT_REJECT);
        return;
    }
    if ((raw_submit == FALSE) &&
        (ndsRendererHardwareTriangleInsideNearPlane(
             &state->vertices[i0], &state->vertices[i1],
             &state->vertices[i2]) == FALSE))
    {
        /* The source RSP clips before the perspective divide, so a triangle
         * that straddles its near plane still draws its front part.  Clip it
         * the same way rather than dropping it (BUGS.md #10); emitting the
         * raw post-divide vertices is what would create a screen-spanning
         * primitive, and the clipper never does that. */
#if NDS_RENDERER_PROFILE_LEVEL < 2
        near_clipped = TRUE;
#else
        if (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
        {
            (void)ndsRendererHardwareNextProjectedDepth();
        }
        if (source_zbuffered != FALSE)
        {
            ndsRendererHardwareEnterProjectedForeground();
        }
        semantic_event.outcome = NDS_RENDERER_SEMANTIC_TRANSFORM_REJECT;
        semantic_event.submit_class = NDS_RENDERER_HW_SUBMIT_REJECT;
        ndsRendererSemanticCommitEvent(&semantic_event);
        ndsRendererProfileRecordNearPlaneTriangleReject();
        ndsRendererProfileRecordSubmitClass(NDS_RENDERER_HW_SUBMIT_REJECT);
        return;
#endif
    }
    if (no_oracle == FALSE)
    {
        stats->hardware_oracle_triangle_count++;
    }
    if (raw_snapshot_id != NDS_RENDERER_MATRIX_SNAPSHOT_INVALID)
    {
        raw_snapshot = ndsRendererGetMatrixSnapshot(state, raw_snapshot_id);
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (sNdsRendererHardwareNoOracle == 0u)
    {
        ndsRendererHardwareRecordOracleTriangle(state, i0, i1, i2);
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (state->texture_prepare_valid == 0u)
    {
        material_color = ndsRendererHardwareColorSource(stats);
        use_material_color = ndsRendererHardwareUseMaterialColor(stats);
        use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
        state->texture_prepare_material_color = material_color;
        state->texture_prepare_vertex_flags =
            ((use_material_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
            ((use_vertex_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);
    }
#else
    material_color = ndsRendererHardwareColorSource(stats);
    use_material_color = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
    state->texture_prepare_material_color = material_color;
    state->texture_prepare_vertex_flags =
        ((use_material_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
        ((use_vertex_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);
    if (stats->texture_combine_count != 0u)
    {
        if (ndsRendererHardwareLitShadeCombine(stats) != FALSE)
        {
            gNdsRendererProfileLitShadeCombineCount++;
        }
        if (use_material_color != FALSE)
        {
            gNdsRendererProfileMaterialCombineCount++;
        }
    }
#endif
    if (state->texture_prepare_valid == 0u)
    {
        poly_alpha = ndsRendererHardwareAlpha(stats, v0);
        state->texture_prepare_alpha_constant =
            (ndsRendererHardwareAlphaUsesVertex(stats) == FALSE) ?
                TRUE : FALSE;
        if (state->texture_prepare_alpha_constant != 0u)
        {
            state->texture_prepare_poly_alpha = poly_alpha;
            state->texture_prepare_poly_fmt =
                ndsRendererHardwarePolyFmt(stats, poly_alpha);
        }
    }
    else if (state->texture_prepare_alpha_constant != 0u)
    {
        poly_alpha = state->texture_prepare_poly_alpha;
    }
    else
    {
        poly_alpha = ndsRendererHardwareAlpha(stats, v0);
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.poly_alpha = poly_alpha;
    semantic_event.alpha_key = ndsRendererHardwareAlphaStateKey(stats);
    semantic_event.fog_key = ndsRendererHardwareFogStateKey(stats);
#endif
    if (poly_alpha == 0u)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        semantic_event.outcome = NDS_RENDERER_SEMANTIC_ALPHA_ZERO;
        semantic_event.poly_fmt =
            ndsRendererHardwarePolyFmt(stats, poly_alpha);
        ndsRendererSemanticCommitEvent(&semantic_event);
#endif
        return;
    }
    ndsRendererProfileRecordSubmitClass(submit_class);
    if (projected_submit != FALSE)
    {
        ndsRendererProfileRecordProjectedSubmit();
    }
    if ((raw_submit == FALSE) &&
        (submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL) &&
        (submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH))
    {
        zbuffered = FALSE;
        decal_depth = FALSE;
        prim_depth = FALSE;
    }
    scale_world = (raw_submit != FALSE) ? TRUE : FALSE;
    if (state->texture_prepare_valid == 0u)
    {
        render_tile =
            &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
        implicit_texture_on =
            ndsRendererHardwareTextureImplicitStateOn(stats);
        use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ?
            ndsRendererHardwareBindTexture(stats, config, state) : FALSE;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->texture_prepare_key_hash = (use_texture != FALSE) ?
            sNdsRendererSemanticLastTextureKeyHash : 0u;
        state->texture_prepare_params = (use_texture != FALSE) ?
            sNdsRendererSemanticLastTextureParams : 0u;
#endif
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled =
            (use_texture != FALSE) ? TRUE : FALSE;
        state->texture_prepare_name = (use_texture != FALSE) ?
            sNdsRendererHardwareBoundTextureName : 0u;
        ndsRendererProfileRecordTexturePrepare();

        texture_scale_s = stats->texture_scale_s;
        texture_scale_t = stats->texture_scale_t;
        if ((use_texture != FALSE) && (implicit_texture_on != FALSE))
        {
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
            {
                texture_scale_s = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
            {
                texture_scale_t = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
        }
        texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
        state->texture_prepare_scale_s = texture_scale_s;
        state->texture_prepare_scale_t = texture_scale_t;
        state->texture_prepare_origin_s = render_tile->uls;
        state->texture_prepare_origin_t = render_tile->ult;
        state->texture_prepare_offset = texture_offset;
        if (use_texture != FALSE)
        {
            state->texture_prepare_vertex_flags |=
                NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        }
    }
    else
    {
        use_texture =
            (state->texture_prepare_enabled != 0u) ? TRUE : FALSE;
        ndsRendererProfileRecordTexturePrepareReuse();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (use_texture != FALSE)
        {
            state->texture_prepare_vertex_flags |=
                NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        }
#endif
    }
#if NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY
    if (use_texture != FALSE)
    {
        state->texture_prepare_vertex_flags &=
            ~(NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL |
              NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX);
    }
#endif
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        ndsRendererSubmitHardwareTriangleRawMatrixCold(
            state, submit_class, raw_snapshot, scale_world);
    }
    else if (raw_snapshot != NULL)
    {
        ndsRendererSubmitHardwareTriangleRawMatrixCold(
            state, submit_class, raw_snapshot, scale_world);
    }
    else
    {
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
    }
    if (prim_depth != FALSE)
    {
        projected_z[0] = projected_z[1] = projected_z[2] =
            (s32)(stats->prim_depth & 0xffffu);
    }
    else if (source_zbuffered != FALSE)
    {
        /* Source-Z projected submissions use the composed clip Z below and
         * must not consume the synthetic no-Z painter counter. */
        projected_z[0] = projected_z[1] = projected_z[2] = 0;
    }
    else
    {
        projected_z[0] = projected_z[1] = projected_z[2] =
            ndsRendererHardwareNextProjectedDepth();
    }
    poly_fmt = (state->texture_prepare_alpha_constant != 0u) ?
        state->texture_prepare_poly_fmt :
        ndsRendererHardwarePolyFmt(stats, poly_alpha);
    texture_name = state->texture_prepare_name;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.poly_fmt = poly_fmt;
    semantic_event.texture_name = texture_name;
    semantic_event.texture_key_hash = state->texture_prepare_key_hash;
    semantic_event.texture_params = state->texture_prepare_params;
    semantic_event.matrix_loaded = sNdsRendererHardwareMatrixLoaded;
    semantic_event.matrix_mode = sNdsRendererHardwareMatrixMode;
    semantic_event.matrix_generation =
        sNdsRendererHardwareMatrixGeneration;
    semantic_event.matrix_signature =
        sNdsRendererHardwareMatrixSignature;
    semantic_event.projected_z[0] = projected_z[0];
    semantic_event.projected_z[1] = projected_z[1];
    semantic_event.projected_z[2] = projected_z[2];
#endif
    ndsRendererHardwareBeginTriangleBatch(
        stats, (use_texture != FALSE) ? TRUE : FALSE,
        texture_name, poly_fmt, sNdsRendererHardwareMatrixMode,
        sNdsRendererHardwareMatrixGeneration);
    state->texture_prepare_vertex_flags =
        (state->texture_prepare_vertex_flags &
         NDS_RENDERER_VERTEX_CONTEXT_PREPARED_MASK) |
        ((scale_world != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD : 0u) |
        ((zbuffered != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED : 0u) |
        ((decal_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH : 0u) |
        ((prim_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH : 0u) |
        ((source_clip_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH : 0u);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.context_flags = state->texture_prepare_vertex_flags;
    semantic_event.zbuffered = (zbuffered != FALSE) ? TRUE : FALSE;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    vertex_submit_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (near_clipped != FALSE)
    {
        if (ndsRendererHardwareSubmitNearClippedTriangle(
                stats, state, i0, i1, i2, projected_z[0]) == 0u)
        {
            ndsRendererProfileRecordNearPlaneTriangleReject();
        }
    }
    else
#endif
    {
    ndsRendererHardwareSubmitVertex(stats, state, i0, projected_z[0]
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                    , &semantic_event.vertex[0]
#endif
                                    );
    ndsRendererHardwareSubmitVertex(stats, state, i1, projected_z[1]
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                    , &semantic_event.vertex[1]
#endif
                                    );
    ndsRendererHardwareSubmitVertex(stats, state, i2, projected_z[2]
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                    , &semantic_event.vertex[2]
#endif
                                    );
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileVertexSubmitTicks +=
        cpuGetTiming() - vertex_submit_start;
#endif
    if (source_zbuffered != FALSE)
    {
        ndsRendererHardwareEnterProjectedForeground();
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.outcome = NDS_RENDERER_SEMANTIC_EMITTED;
    semantic_event.no_z_depth_after = sNdsRendererHardwareProjectedDepth;
    semantic_event.no_z_background_after =
        sNdsRendererHardwareProjectedBackground;
    ndsRendererSemanticCommitEvent(&semantic_event);
#endif

    sNdsRendererHardwareSubmitted = TRUE;
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount++;
#endif
    stats->hardware_triangle_count++;
    stats->hardware_vertex_count += 3u;
    ndsRendererProfileRecordHardwareTriangle();
    if (zbuffered != FALSE)
    {
        ndsRendererSubmitHardwareTriangleDepthStatsCold(
            stats, decal_depth, prim_depth);
    }
    else
    {
        stats->hardware_projected_depth_triangle_count++;
    }
}
#endif
#endif

#if !NDS_RENDERER_HW_TRIANGLES
/* Shared scene code owns teardown calls even in the software build.  Keep the
 * hardware-only texture API link-complete without allocating GX state. */
volatile u32 gNdsRendererBattleStaticTextureEnabled = 0u;
/* Same reason, and the reason this block exists at all: scVSBattleStartBattle
 * calls the scene texture-VRAM reset unconditionally, so the software build
 * needs the symbol. The counters stay real so a soak against either build reads
 * the same field rather than dropping it. */
volatile u32 gNdsRendererSceneTextureVramResetEnable = 1u;
volatile u32 gNdsRendererSceneTextureVramResetCount;

void ndsRendererHardwareResetSceneTextureVram(void)
{
    gNdsRendererSceneTextureVramResetCount++;
}

s32 ndsRendererHardwarePrepareBattleStaticTextures(void)
{
    return FALSE;
}

#if NDS_R2_PARTICLE_RUNTIME
volatile u32 gNdsRendererParticleAtlasPrepareCount;
volatile u32 gNdsRendererParticleAtlasFailCount;
volatile u32 gNdsRendererParticleAtlasBytes;
volatile u32 gNdsRendererWhispyNativePrepareCount;
volatile u32 gNdsRendererWhispyNativeFailCount;
volatile u32 gNdsRendererWhispyNativeBytes;
#if NDS_R2_FOX_BLASTER_GLOW_AOT
volatile u32 gNdsRendererFoxBlasterGlowPrepareCount;
volatile u32 gNdsRendererFoxBlasterGlowFailCount;
volatile u32 gNdsRendererFoxBlasterGlowBytes;
#endif
/* The v16 rail counters' software-renderer twins. `battleship_lbparticle.c`
 * reads the clamp count under NDS_R2_PARTICLE_DRAW, which defaults ON while
 * NDS_RENDERER_HW_TRIANGLES defaults OFF -- so without these three the DEFAULT
 * configuration is the one that fails to link, which is exactly the trap
 * ndsRendererSetParticleCamera fell into on 2026-08-02. They stay real so a
 * soak against either build reads the same field rather than dropping it. */
volatile u32 gNdsParticleWorldClampCount;
volatile u32 gNdsParticleScaleEscalations;
volatile u32 gNdsParticleScaleShiftMax;

/* The particle atlas is a hardware-texture object; the software renderer has
 * no texture cache to hold it, so the draw seam gets a name of 0 and declines
 * rather than the whole configuration failing to link. */
s32 ndsRendererHardwarePrepareParticleAtlas(void)
{
    gNdsRendererParticleAtlasPrepareCount++;
    gNdsRendererParticleAtlasFailCount++;
    return FALSE;
}

u32 ndsRendererHardwareParticleAtlasNameForSheet(u32 sheet)
{
    (void)sheet;
    return 0u;
}

u32 ndsRendererHardwareParticleAtlasName(void)
{
    return 0u;
}

u32 ndsRendererHardwareFoxBlasterGlowName(void)
{
    return 0u;
}

u32 ndsRendererHardwareWhispyNativeName(u32 texture_id)
{
    (void)texture_id;
    return 0u;
}

void ndsRendererHardwareDiscardParticleAtlas(void)
{
}

/* lbParticleDrawTextures hands the pass its camera unconditionally -- it has no
 * business knowing which renderer is under it -- so this half of the seam must
 * exist in both configurations. It did not, and NDS_RENDERER_HW_TRIANGLES=0
 * with NDS_R2_PARTICLE_RUNTIME=1 is the DEFAULT, i.e. the published
 * smash64ds.nds: `make` with no overrides failed at link on this symbol alone.
 * The three particle entry points below are one seam; add or remove them
 * together. */
void ndsRendererSetParticleCamera(const NDSRendererMatrix20p12 *projection,
                                  const NDSRendererMatrix20p12 *modelview)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    (void)projection; (void)modelview;
}

s32 ndsRendererSubmitParticleQuad(u32 atlas_name, const Vec3f *pos, f32 size,
                                  u32 color, u8 alpha,
                                  const Vec3f *right, const Vec3f *up,
                                  u32 mirror_mask,
                                  u32 atlas_x, u32 atlas_y,
                                  u32 atlas_w, u32 atlas_h)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    (void)atlas_name; (void)pos; (void)size; (void)color; (void)alpha;
    (void)right; (void)up; (void)mirror_mask;
    (void)atlas_x; (void)atlas_y; (void)atlas_w; (void)atlas_h;
    return FALSE;
}

void ndsRendererSetWhispyNativeBasis(const Vec3f *right, const Vec3f *up)
{
    (void)right; (void)up;
}

s32 ndsRendererParticlePositionToQ12(const Vec3f *pos,
                                     s32 fixed_center_q12[3])
{
    (void)pos;
    (void)fixed_center_q12;
    return FALSE;
}

s32 ndsRendererSubmitWhispyNativeQuad(u32 texture_name,
                                      u32 texture_slot,
                                      const Vec3f *pos, f32 size,
                                      const s32 fixed_center_q12[3],
                                      s32 fixed_size_q8,
                                      u32 color, u8 alpha,
                                      u32 mirror_mask,
                                      u32 texture_w, u32 texture_h,
                                      u32 submit_route)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    (void)texture_name; (void)pos; (void)size; (void)color; (void)alpha;
    (void)fixed_center_q12; (void)fixed_size_q8;
    (void)texture_slot; (void)mirror_mask; (void)texture_w; (void)texture_h;
    (void)submit_route;
    return -1;
}

void ndsRendererEndParticleQuads(void)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
}
#endif

void ndsRendererHardwareArmBattleStaticTextures(void)
{
}

void ndsRendererHardwareDiscardBattleStaticTextures(void)
{
}

void ndsRendererHardwareAbortBattleStaticTextures(void)
{
}
#endif
