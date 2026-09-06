#include <nds/nds_preview_pack.h>

static inline void ndsRendererExecuteTriangleCommand(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 op,
    u32 w0,
    u32 w1)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    u32 triangle_submit_start;
#endif
#if !NDS_RENDERER_HW_TRIANGLES
    (void)config;
#endif

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->triangle_command_count++);
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->render_command_count++);
    if (op == NDS_RENDERER_OP_TRI1)
    {
        u32 packed = ndsGBIDecodeF3DEX2Tri1(w0);

        stats->triangle_count++;
#if NDS_RENDERER_HW_TRIANGLES
        if (sNdsRendererHardwareNoOracle == 0u)
#endif
        ndsRendererRecordTransformedTriangle(stats, state, packed);
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        triangle_submit_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_tri2_half = 0u;
#endif
        NDS_EFFECT_PHASE_TRI(
            ndsRendererSubmitHardwareTriangle(stats, config, state, packed));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererProfileTriangleSubmitTicks +=
            cpuGetTiming() - triangle_submit_start;
#endif
#endif
        return;
    }

    stats->triangle_count += 2u;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererHardwareNoOracle == 0u)
    {
#endif
        ndsRendererRecordTransformedTriangle(
            stats, state, ndsGBIDecodeF3DEX2Tri2First(w0));
        ndsRendererRecordTransformedTriangle(
            stats, state, ndsGBIDecodeF3DEX2Tri2Second(w1));
#if NDS_RENDERER_HW_TRIANGLES
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    triangle_submit_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->semantic_tri2_half = 0u;
#endif
    NDS_EFFECT_PHASE_TRI(ndsRendererSubmitHardwareTriangle(
        stats, config, state, ndsGBIDecodeF3DEX2Tri2First(w0)));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->semantic_tri2_half = 1u;
#endif
    NDS_EFFECT_PHASE_TRI(ndsRendererSubmitHardwareTriangle(
        stats, config, state, ndsGBIDecodeF3DEX2Tri2Second(w1)));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileTriangleSubmitTicks +=
        cpuGetTiming() - triangle_submit_start;
#endif
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static inline s32 ndsRendererFastRawStateEligible(
    const NDSRendererTraversalState *state)
{
    const u32 required_flags =
        NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD |
        NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED;
    const u32 forbidden_flags =
        NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH |
        NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH |
        NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH;

    return ((state != NULL) &&
            (state->texture_prepare_valid != 0u) &&
            (state->texture_prepare_source_zbuffered != 0u) &&
            (state->texture_prepare_decal_depth == 0u) &&
            (state->texture_prepare_prim_depth == 0u) &&
            (state->texture_prepare_alpha_constant != 0u) &&
            (state->texture_prepare_poly_alpha != 0u) &&
            ((state->texture_prepare_vertex_flags & required_flags) ==
             required_flags) &&
            ((state->texture_prepare_vertex_flags & forbidden_flags) == 0u) &&
            (state->matrix_valid != 0u) &&
            (state->matrix_generation != 0u) &&
            (sNdsRendererHardwareMatrixLoaded != 0u) &&
            (sNdsRendererHardwareMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareMatrixGeneration ==
             state->matrix_generation) &&
            (sNdsRendererHardwareTriangleBatchOpen != 0u) &&
            (sNdsRendererHardwareTriangleBatchTextured ==
             state->texture_prepare_enabled) &&
            (sNdsRendererHardwareTriangleBatchTextureName ==
             state->texture_prepare_name) &&
            (sNdsRendererHardwareTriangleBatchPolyFmt ==
             state->texture_prepare_poly_fmt) &&
            (sNdsRendererHardwareTriangleBatchMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
             state->matrix_generation)) ? TRUE : FALSE;
}

static inline s32 ndsRendererFastDecodeTriangle(
    u32 packed, u32 *indices, u32 *required_mask)
{
    u32 i0;
    u32 i1;
    u32 i2;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);
    if ((i0 >= NDS_RENDERER_MAX_VTX) ||
        (i1 >= NDS_RENDERER_MAX_VTX) ||
        (i2 >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    indices[0] = i0;
    indices[1] = i1;
    indices[2] = i2;
    *required_mask |= (1u << i0) | (1u << i1) | (1u << i2);
    return TRUE;
}

static u32 ndsRendererDirectRawPlanHash(const Gfx *source)
{
    uintptr_t value = (uintptr_t)source;

    return (u32)(((value >> 3) ^ (value >> 11)) &
                 (NDS_RENDERER_DIRECT_RAW_PLAN_COUNT - 1u));
}

static NDSRendererDirectRawPlan *ndsRendererDirectRawFindPlan(
    const Gfx *source, u32 max_commands)
{
    NDSRendererDirectRawPlan *empty = NULL;
    u32 slot = ndsRendererDirectRawPlanHash(source);
    u32 probe;
    u32 command_count = 0u;
    u32 triangle_count = 0u;
    u32 entry_offset;
    u32 i;

    if ((source == NULL) || (max_commands == 0u))
    {
        return NULL;
    }
    for (probe = 0u; probe < NDS_RENDERER_DIRECT_RAW_PLAN_COUNT; probe++)
    {
        NDSRendererDirectRawPlan *plan =
            &sNdsRendererDirectRawPlans[
                (slot + probe) & (NDS_RENDERER_DIRECT_RAW_PLAN_COUNT - 1u)];

        if (plan->source == source)
        {
            const Gfx *last;

            if ((plan->command_count == 0u) ||
                (plan->command_count > max_commands))
            {
                return NULL;
            }
            last = source + plan->command_count - 1u;
            if ((source->words.w0 != plan->first_w0) ||
                (source->words.w1 != plan->first_w1) ||
                (last->words.w0 != plan->last_w0) ||
                (last->words.w1 != plan->last_w1))
            {
                return NULL;
            }
            return plan;
        }
        if (plan->source == NULL)
        {
            empty = plan;
            break;
        }
    }
    if (empty == NULL)
    {
        return NULL;
    }

    while (command_count < max_commands)
    {
        u32 op = source[command_count].words.w0 >> 24;

        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            break;
        }
        triangle_count += (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        command_count++;
    }
    if ((command_count == 0u) ||
        (command_count > 0xffffu) ||
        (triangle_count > 0xffffu) ||
        ((sNdsRendererDirectRawEntryCount + command_count) >
         NDS_RENDERER_DIRECT_RAW_ENTRY_COUNT))
    {
        return NULL;
    }

    entry_offset = sNdsRendererDirectRawEntryCount;
    for (i = 0u; i < command_count; i++)
    {
        const Gfx *command = source + i;
        NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[entry_offset + i];
        u32 w0 = command->words.w0;
        u32 w1 = command->words.w1;
        u32 op = w0 >> 24;
        u32 packed[2];
        u32 indices[6];
        u32 required_mask = 0u;
        u32 command_triangles =
            (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        u32 index;

        packed[0] = (op == NDS_RENDERER_OP_TRI1) ?
            ndsGBIDecodeF3DEX2Tri1(w0) :
            ndsGBIDecodeF3DEX2Tri2First(w0);
        packed[1] = (command_triangles == 2u) ?
            ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
        if ((ndsRendererFastDecodeTriangle(
                 packed[0], &indices[0], &required_mask) == FALSE) ||
            ((command_triangles == 2u) &&
             (ndsRendererFastDecodeTriangle(
                  packed[1], &indices[3], &required_mask) == FALSE)))
        {
            return NULL;
        }
        entry->required_mask = required_mask;
        entry->triangle_count = (u8)command_triangles;
        entry->reserved = 0u;
        for (index = 0u; index < (command_triangles * 3u); index++)
        {
            entry->indices[index] = (u8)indices[index];
        }
        for (; index < 6u; index++)
        {
            entry->indices[index] = 0u;
        }
    }

    sNdsRendererDirectRawEntryCount += command_count;
    empty->entry_offset = (u16)entry_offset;
    empty->command_count = (u16)command_count;
    empty->triangle_count = (u16)triangle_count;
    empty->reserved = 0u;
    empty->first_w0 = source->words.w0;
    empty->first_w1 = source->words.w1;
    empty->last_w0 = source[command_count - 1u].words.w0;
    empty->last_w1 = source[command_count - 1u].words.w1;
    empty->source = source;
    return empty;
}

static void NDS_RENDERER_FAST_RUN_CODE ndsRendererFastPrepareRawSlots(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 required_mask,
    u32 textured)
{
    u32 missing_color = required_mask &
        ~state->prepared_vertex_color_valid_mask;
    u32 context_flags = state->texture_prepare_vertex_flags;

    while (missing_color != 0u)
    {
        u32 vertex_index = (u32)__builtin_ctz(missing_color);
        u32 vertex_mask = 1u << vertex_index;
        const NDSRendererInputVertex *vtx =
            &state->input_vertices[vertex_index];

        state->prepared_vertex_colors[vertex_index] =
            ndsRendererHardwarePackedVertexColor(
                stats, vtx, state->texture_prepare_material_color,
                context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL,
                context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX,
                state->vertex_colors[vertex_index],
                ((state->vertex_color_valid_mask & vertex_mask) != 0u) ?
                    TRUE : FALSE,
                state->color_modulate);
        state->prepared_vertex_color_valid_mask |= vertex_mask;
        missing_color &= ~vertex_mask;
    }

    if (textured != 0u)
    {
        u32 missing_texcoord = required_mask &
            ~state->prepared_texcoord_valid_mask;

        while (missing_texcoord != 0u)
        {
            u32 vertex_index = (u32)__builtin_ctz(missing_texcoord);
            u32 vertex_mask = 1u << vertex_index;
            const NDSRendererInputVertex *vtx =
                &state->input_vertices[vertex_index];

            state->prepared_texcoord_s[vertex_index] =
                ndsRendererHardwareTexCoord(
                    vtx->s, state->texture_prepare_scale_s,
                    state->texture_prepare_origin_s,
                    state->texture_prepare_offset);
            state->prepared_texcoord_t[vertex_index] =
                ndsRendererHardwareTexCoord(
                    vtx->t, state->texture_prepare_scale_t,
                    state->texture_prepare_origin_t,
                    state->texture_prepare_offset);
            state->prepared_texcoord_valid_mask |= vertex_mask;
            missing_texcoord &= ~vertex_mask;
        }
    }
}

static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererFastRawFallbackCommand(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 op,
    u32 w0,
    u32 w1)
{
    ndsRendererExecuteTriangleCommand(stats, config, state, op, w0, w1);
}

static inline void ndsRendererFastEmitRawUntexturedVertex(
    const NDSRendererTraversalState *state, u32 vertex_index)
{
    const NDSRendererInputVertex *vtx =
        &state->input_vertices[vertex_index];

    glColor(state->prepared_vertex_colors[vertex_index]);
    glVertex3v16(
        (v16)((s32)vtx->x * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->y * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->z * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
}

static inline void ndsRendererFastEmitRawTexturedVertex(
    const NDSRendererTraversalState *state, u32 vertex_index)
{
    const NDSRendererInputVertex *vtx =
        &state->input_vertices[vertex_index];

    glColor(state->prepared_vertex_colors[vertex_index]);
    glTexCoord2t16(state->prepared_texcoord_s[vertex_index],
                  state->prepared_texcoord_t[vertex_index]);
    glVertex3v16(
        (v16)((s32)vtx->x * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->y * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->z * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
}

static inline void ndsRendererFastEmitRawCommand(
    const NDSRendererTraversalState *state,
    const u32 *indices,
    u32 triangle_count,
    u32 textured)
{
    u32 triangle_index;

    if (textured != 0u)
    {
        for (triangle_index = 0u;
             triangle_index < triangle_count;
             triangle_index++)
        {
            const u32 *triangle = &indices[triangle_index * 3u];

            ndsRendererFastEmitRawTexturedVertex(state, triangle[0]);
            ndsRendererFastEmitRawTexturedVertex(state, triangle[1]);
            ndsRendererFastEmitRawTexturedVertex(state, triangle[2]);
        }
    }
    else
    {
        for (triangle_index = 0u;
             triangle_index < triangle_count;
             triangle_index++)
        {
            const u32 *triangle = &indices[triangle_index * 3u];

            ndsRendererFastEmitRawUntexturedVertex(state, triangle[0]);
            ndsRendererFastEmitRawUntexturedVertex(state, triangle[1]);
            ndsRendererFastEmitRawUntexturedVertex(state, triangle[2]);
        }
    }
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererFastCommitRawSemanticTriangle(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 packed,
    const u32 *indices)
{
    NDSRendererSemanticEvent event;
    u32 vertex_number;

    memset(&event, 0, sizeof(event));
    event.owner = (u32)sNdsRendererProfileOwner;
    event.owner_occurrence =
        sNdsRendererSemanticSourceProvenance.owner_occurrence;
    event.list_ordinal =
        sNdsRendererSemanticSourceProvenance.list_ordinal;
    event.branch_path = state->semantic_branch_path;
    event.command_index = state->semantic_command_index;
    event.tri2_half = state->semantic_tri2_half;
    event.outcome = NDS_RENDERER_SEMANTIC_EMITTED;
    event.packed = packed;
    event.submit_class =
        NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX;
    event.source_state_hash = ndsRendererSemanticSourceStateHash(stats);
    event.raw_snapshot_id = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    event.context_flags = state->texture_prepare_vertex_flags;
    event.source_zbuffered = TRUE;
    event.zbuffered = TRUE;
    event.raw_submit = TRUE;
    event.poly_alpha = state->texture_prepare_poly_alpha;
    event.poly_fmt = state->texture_prepare_poly_fmt;
    event.alpha_key = ndsRendererHardwareAlphaStateKey(stats);
    event.fog_key = ndsRendererHardwareFogStateKey(stats);
    event.fog_color = stats->fog_color;
    event.fog_min = stats->fog_min;
    event.fog_max = stats->fog_max;
    event.fog_status = stats->fog_status;
    event.texture_name = state->texture_prepare_name;
    event.texture_key_hash = state->texture_prepare_key_hash;
    event.texture_params = state->texture_prepare_params;
    event.matrix_loaded = sNdsRendererHardwareMatrixLoaded;
    event.matrix_mode = sNdsRendererHardwareMatrixMode;
    event.matrix_generation = sNdsRendererHardwareMatrixGeneration;
    event.matrix_signature = sNdsRendererHardwareMatrixSignature;
    event.no_z_depth_before = sNdsRendererHardwareProjectedDepth;
    event.no_z_depth_after = sNdsRendererHardwareProjectedDepth;
    event.no_z_background_before =
        sNdsRendererHardwareProjectedBackground;
    event.no_z_background_after =
        sNdsRendererHardwareProjectedBackground;

    ndsRendererHardwareRecordOracleTriangle(
        state, indices[0], indices[1], indices[2]);
    stats->hardware_oracle_triangle_count++;
    for (vertex_number = 0u; vertex_number < 3u; vertex_number++)
    {
        u32 vertex_index = indices[vertex_number];
        const NDSRendererInputVertex *vtx =
            &state->input_vertices[vertex_index];
        NDSRendererSemanticVertex *vertex = &event.vertex[vertex_number];

        event.vertex_index[vertex_number] = vertex_index;
        event.vertex_matrix_snapshot[vertex_number] =
            state->vertex_matrix_snapshot[vertex_index];
        event.vertex_clip_snapshot[vertex_number] =
            state->vertex_clip_snapshot[vertex_index];
        vertex->x = (v16)((s32)vtx->x *
            (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)));
        vertex->y = (v16)((s32)vtx->y *
            (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)));
        vertex->z = (v16)((s32)vtx->z *
            (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)));
        vertex->color = state->prepared_vertex_colors[vertex_index];
        vertex->valid_flags =
            NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID |
            NDS_RENDERER_SEMANTIC_VERTEX_COLOR_VALID;
        if (state->texture_prepare_enabled != 0u)
        {
            vertex->s = state->prepared_texcoord_s[vertex_index];
            vertex->t = state->prepared_texcoord_t[vertex_index];
            vertex->valid_flags |= NDS_RENDERER_SEMANTIC_VERTEX_ST_VALID;
            ndsRendererProfileTextureCoord(vertex->s, vertex->t);
            ndsRendererProfileTextureSample(vertex->s, vertex->t);
        }
        ndsRendererProfileVertexRange(
            vtx, vertex->x, vertex->y, vertex->z);
    }
    ndsRendererSemanticCommitEvent(&event);
}
#endif

static inline void ndsRendererFastAccountRawTriangles(
    NDSRendererStats *stats,
    u32 triangle_count,
    u32 reuse_count)
{
    sNdsRendererHardwareSubmitClassCounts[
        NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX] += triangle_count;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    {
        NDSRendererProfileOwnerHotLedger *owner =
            ndsRendererProfileCurrentOwner();

        if (owner != NULL)
        {
            owner->submit_class_count[
                NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX] +=
                    triangle_count;
        }
    }
    gNdsRendererProfileRawCurrentCandidateCount += triangle_count;
    gNdsRendererProfileHardwareBatchReuseCount += reuse_count;
    gNdsRendererProfileTexturePrepareReuseCount += reuse_count;
    gNdsRendererProfileHardwareTriangles += triangle_count;
    gNdsRendererProfileHardwareVertices += triangle_count * 3u;
    if ((gNdsRendererProfileHardwareTriangles > 2048u) ||
        (gNdsRendererProfileHardwareVertices > 6144u))
    {
        gNdsRendererProfileHardwareOverLimit = 1u;
    }
#else
    sNdsRendererRuntimeFrameSummary.raw_current_candidate_count +=
        triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices +=
        triangle_count * 3u;
#endif
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    stats->hardware_zbuffer_triangle_count += triangle_count;
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount += triangle_count;
#endif
    sNdsRendererHardwareSubmitted = TRUE;
}

static inline void ndsRendererFastEmitDirectRawEntry(
    const NDSRendererTraversalState *state,
    const NDSRendererDirectRawEntry *entry,
    u32 textured)
{
    u32 vertex_count = (u32)entry->triangle_count * 3u;
    u32 vertex_index;

    if (textured != 0u)
    {
        for (vertex_index = 0u; vertex_index < vertex_count; vertex_index++)
        {
            ndsRendererFastEmitRawTexturedVertex(
                state, entry->indices[vertex_index]);
        }
    }
    else
    {
        for (vertex_index = 0u; vertex_index < vertex_count; vertex_index++)
        {
            ndsRendererFastEmitRawUntexturedVertex(
                state, entry->indices[vertex_index]);
        }
    }
}

static s32 NDS_RENDERER_FAST_RUN_CODE ndsRendererExecuteDirectRawRemainder(
    const Gfx **dl_io,
    u32 *list_index_io,
    u32 immutable_command_count,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    NDSRendererCommandCallback callback)
{
    const Gfx *source;
    NDSRendererDirectRawPlan *plan;
    u32 first_index;
    u32 remaining_commands;
    u32 available_mask;
    u32 required_mask = 0u;
    u32 entry_number;
    u32 textured;

    if ((callback != NULL) ||
        (ndsRendererFastRawStateEligible(state) == FALSE))
    {
        return FALSE;
    }
    first_index = *list_index_io + 1u;
    if ((first_index >= immutable_command_count) ||
        (first_index >= config->max_list_commands))
    {
        return FALSE;
    }
    remaining_commands = immutable_command_count - first_index;
    if (remaining_commands > (config->max_list_commands - first_index))
    {
        remaining_commands = config->max_list_commands - first_index;
    }
    source = *dl_io + 1;
    plan = ndsRendererDirectRawFindPlan(source, remaining_commands);
    if ((plan == NULL) ||
        ((stats->command_count + plan->command_count) > config->max_commands))
    {
        return FALSE;
    }

    available_mask = state->input_vertex_valid_mask &
        state->raw_vertex_fit_mask & state->current_transform_vertex_mask;
    for (entry_number = 0u;
         entry_number < plan->command_count;
         entry_number++)
    {
        const NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[
                plan->entry_offset + entry_number];

        if ((available_mask & entry->required_mask) != entry->required_mask)
        {
            return FALSE;
        }
        required_mask |= entry->required_mask;
    }

    textured = state->texture_prepare_enabled;
    ndsRendererFastPrepareRawSlots(stats, state, required_mask, textured);
    for (entry_number = 0u;
         entry_number < plan->command_count;
         entry_number++)
    {
        const Gfx *command = source + entry_number;
        const NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[
                plan->entry_offset + entry_number];

        stats->command_count++;
        NDS_RENDERER_RECORD_PROOF_ONLY(
            stats->triangle_command_count++;
            stats->render_command_count++;
        );
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_command_index = first_index + entry_number;
        state->semantic_tri2_half = 0u;
        sNdsRendererProfileTrustedCommandCount++;
        sNdsRendererProfileTriangleRunReuseCount++;
#endif
        ndsRendererFastEmitDirectRawEntry(state, entry, textured);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        {
            u32 w0 = command->words.w0;
            u32 w1 = command->words.w1;
            u32 packed[2];
            u32 triangle_index;

            packed[0] = (entry->triangle_count == 1u) ?
                ndsGBIDecodeF3DEX2Tri1(w0) :
                ndsGBIDecodeF3DEX2Tri2First(w0);
            packed[1] = (entry->triangle_count == 2u) ?
                ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
            for (triangle_index = 0u;
                 triangle_index < entry->triangle_count;
                 triangle_index++)
            {
                u32 indices[3];
                u32 base = triangle_index * 3u;

                indices[0] = entry->indices[base];
                indices[1] = entry->indices[base + 1u];
                indices[2] = entry->indices[base + 2u];
                if (sNdsRendererHardwareNoOracle == 0u)
                {
                    ndsRendererRecordTransformedTriangle(
                        stats, state, packed[triangle_index]);
                }
                state->semantic_tri2_half = triangle_index;
                ndsRendererFastCommitRawSemanticTriangle(
                    stats, state, packed[triangle_index], indices);
            }
        }
#else
        (void)command;
#endif
    }

    stats->triangle_count += plan->triangle_count;
    ndsRendererFastAccountRawTriangles(
        stats, plan->triangle_count, plan->triangle_count);
    sNdsRendererFastRunCount++;
    sNdsRendererFastTriangleCount += plan->triangle_count;
    if ((u32)sNdsRendererRuntimeOwner <
        (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererFastOwnerTriangleCount[
            (u32)sNdsRendererRuntimeOwner] += plan->triangle_count;
    }
    *dl_io = source + plan->command_count - 1u;
    *list_index_io = first_index + plan->command_count - 1u;
    return TRUE;
}

static void NDS_RENDERER_FAST_RUN_CODE ndsRendererExecuteFastRawCurrentRun(
    const Gfx **dl_io,
    u32 *list_index_io,
    u32 immutable_command_count,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 depth,
    NDSRendererCommandCallback callback,
    void *callback_user)
{
    const Gfx *dl = *dl_io;
    u32 list_index = *list_index_io;
    u32 fast_triangles = 0u;
    u32 fallback_state = 0u;
    u32 fallback_vertex = 0u;
    u32 fallback_command = 0u;
    u32 fast_command_count = 0u;

    if (ndsRendererExecuteDirectRawRemainder(
            dl_io, list_index_io, immutable_command_count,
            config, stats, state, callback) != FALSE)
    {
        return;
    }

    while (((list_index + 1u) < immutable_command_count) &&
           ((list_index + 1u) < config->max_list_commands))
    {
        const Gfx *next_dl = dl + 1;
        u32 w0 = next_dl->words.w0;
        u32 w1 = next_dl->words.w1;
        u32 op = w0 >> 24;
        u32 packed[2];
        u32 indices[6];
        u32 required_mask = 0u;
        u32 triangle_count;
        s32 decode_ok;
        s32 state_ok;
        s32 vertex_ok;
        NDSRendererCommand command;

        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            break;
        }
        if (stats->command_count >= config->max_commands)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
            break;
        }

        list_index++;
        dl = next_dl;
        stats->command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_command_index = list_index;
        state->semantic_tri2_half = 0u;
        sNdsRendererProfileTrustedCommandCount++;
        sNdsRendererProfileTriangleRunReuseCount++;
#endif
        if (callback != NULL)
        {
            memset(&command, 0, sizeof(command));
            command.dl = dl;
            command.w0 = w0;
            command.w1 = w1;
            command.op = op;
            command.depth = depth;
            command.list_index = list_index;
            command.transformed_vertices = state->vertices;
            command.transformed_vertex_valid_mask = state->vertex_valid_mask;
            command.matrix_valid = state->matrix_valid;
            if (callback(&command, callback_user) == FALSE)
            {
                ndsRendererRecordUnsupported(stats, op);
                stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
                break;
            }
        }
        triangle_count = (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        packed[0] = (op == NDS_RENDERER_OP_TRI1) ?
            ndsGBIDecodeF3DEX2Tri1(w0) :
            ndsGBIDecodeF3DEX2Tri2First(w0);
        packed[1] = (triangle_count == 2u) ?
            ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
        decode_ok = ndsRendererFastDecodeTriangle(
            packed[0], &indices[0], &required_mask);
        if ((decode_ok != FALSE) && (triangle_count == 2u))
        {
            decode_ok = ndsRendererFastDecodeTriangle(
                packed[1], &indices[3], &required_mask);
        }
        state_ok = ndsRendererFastRawStateEligible(state);
        vertex_ok = ((decode_ok != FALSE) &&
                     ((state->input_vertex_valid_mask & required_mask) ==
                      required_mask) &&
                     ((state->raw_vertex_fit_mask & required_mask) ==
                      required_mask) &&
                     ((state->current_transform_vertex_mask & required_mask) ==
                      required_mask)) ? TRUE : FALSE;

        if ((state_ok != FALSE) && (vertex_ok != FALSE))
        {
            u32 textured = state->texture_prepare_enabled;
            u32 triangle_index;

            NDS_RENDERER_RECORD_PROOF_ONLY(
                stats->triangle_command_count++;
                stats->render_command_count++;
            );
            stats->triangle_count += triangle_count;
            ndsRendererFastPrepareRawSlots(
                stats, state, required_mask, textured);
            for (triangle_index = 0u;
                 triangle_index < triangle_count;
                 triangle_index++)
            {
                const u32 *triangle = &indices[triangle_index * 3u];

                if (sNdsRendererHardwareNoOracle == 0u)
                {
                    ndsRendererRecordTransformedTriangle(
                        stats, state, packed[triangle_index]);
                }
                ndsRendererFastEmitRawCommand(
                    state, triangle, 1u, textured);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                state->semantic_tri2_half = triangle_index;
                ndsRendererFastCommitRawSemanticTriangle(
                    stats, state, packed[triangle_index], triangle);
#endif
            }
            fast_triangles += triangle_count;
            fast_command_count++;
        }
        else
        {
            if (decode_ok == FALSE)
            {
                fallback_command++;
            }
            else if (state_ok == FALSE)
            {
                fallback_state++;
            }
            else
            {
                fallback_vertex++;
            }
            ndsRendererFastRawFallbackCommand(
                stats, config, state, op, w0, w1);
        }
    }

    if (fast_triangles != 0u)
    {
        ndsRendererFastAccountRawTriangles(
            stats, fast_triangles, fast_triangles);
        sNdsRendererFastRunCount++;
        sNdsRendererFastTriangleCount += fast_triangles;
        if ((u32)sNdsRendererRuntimeOwner <
            (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
        {
            sNdsRendererFastOwnerTriangleCount[
                (u32)sNdsRendererRuntimeOwner] += fast_triangles;
        }
    }
    (void)fast_command_count;
    sNdsRendererFastFallbackCount[0] += fallback_state;
    sNdsRendererFastFallbackCount[1] += fallback_vertex;
    sNdsRendererFastFallbackCount[2] += fallback_command;
    *dl_io = dl;
    *list_index_io = list_index;
}

static inline void ndsRendererNativeSourceBoundary(
    NDSRendererTraversalState *state)
{
    ndsRendererHardwareEndBatch();
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_projected_xy_valid_mask = 0u;
    state->prepared_projected_source_z_valid_mask = 0u;
}

static s32 ndsRendererNativePrepareDirectRun(
    const NDSNativeRun *run,
    u32 first_vertex,
    s32 projected_run,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const NDSRendererInputVertex *v0;
    const NDSRendererTileState *render_tile;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 material_color;
    u32 poly_alpha;
    u32 poly_fmt;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 implicit_texture_on;
    s32 use_texture;
    s32 texture_offset;
    u32 available_mask;

    if ((run == NULL) || (config == NULL) || (stats == NULL) ||
        (state == NULL) || (first_vertex >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u) ||
        ((stats->othermode_l & NDS_RENDERER_ZMODE_DEC) ==
         NDS_RENDERER_ZMODE_DEC) ||
        (ndsRendererHardwareUsePrimDepth(stats) != FALSE) ||
        ((projected_run == FALSE) &&
         (ndsRendererHardwareRawMatrixCompatible(state) == FALSE)))
    {
        return FALSE;
    }
    available_mask = state->input_vertex_valid_mask &
        state->raw_vertex_fit_mask;
    if (projected_run == FALSE)
    {
        available_mask &= state->current_transform_vertex_mask;
    }
    if ((available_mask & run->required_mask) != run->required_mask)
    {
        return FALSE;
    }
    v0 = &state->input_vertices[first_vertex];
    if (projected_run != FALSE)
    {
        u32 missing = run->required_mask;

        while (missing != 0u)
        {
            u32 index = (u32)__builtin_ctz(missing);

            if (ndsRendererEnsureTransformedVertex(
                    stats, state, index) == FALSE)
            {
                return FALSE;
            }
            missing &= ~(1u << index);
        }
    }
    state->texture_prepare_source_zbuffered = TRUE;
    state->texture_prepare_decal_depth = FALSE;
    state->texture_prepare_prim_depth = FALSE;

#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (state->texture_prepare_valid == 0u)
#endif
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
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
    }
    if (state->texture_prepare_valid == 0u)
    {
        poly_alpha = ndsRendererHardwareAlpha(stats, v0);
        state->texture_prepare_alpha_constant =
            (ndsRendererHardwareAlphaUsesVertex(stats) == FALSE) ?
                TRUE : FALSE;
        if ((state->texture_prepare_alpha_constant == 0u) ||
            (poly_alpha == 0u))
        {
            return FALSE;
        }
        state->texture_prepare_poly_alpha = poly_alpha;
        state->texture_prepare_poly_fmt =
            ndsRendererHardwarePolyFmt(stats, poly_alpha);

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

    state->texture_prepare_vertex_flags =
        (state->texture_prepare_vertex_flags &
         NDS_RENDERER_VERTEX_CONTEXT_PREPARED_MASK) |
        ((projected_run != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH :
             (NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD |
              NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED));
    ndsRendererFastPrepareRawSlots(
        stats, state, run->required_mask,
        state->texture_prepare_enabled);
    if (projected_run != FALSE)
    {
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
        poly_fmt = state->texture_prepare_poly_fmt;
        ndsRendererHardwareBeginTriangleBatch(
            stats, (use_texture != FALSE) ? TRUE : FALSE,
            state->texture_prepare_name, poly_fmt,
            sNdsRendererHardwareMatrixMode,
            sNdsRendererHardwareMatrixGeneration);
        return TRUE;
    }
    ndsRendererLoadHardwareMatrices(state, TRUE);
    poly_fmt = state->texture_prepare_poly_fmt;
    ndsRendererHardwareBeginTriangleBatch(
        stats, (use_texture != FALSE) ? TRUE : FALSE,
        state->texture_prepare_name, poly_fmt,
        sNdsRendererHardwareMatrixMode,
        sNdsRendererHardwareMatrixGeneration);
    return ndsRendererFastRawStateEligible(state);
}

/* R2-03 E43. The per-delta census inside ndsRendererNativeApplyStateDelta sits
 * inside E38's span bracket and runs 134.5 times a frame on the before-span
 * alone, so the bracket prices the instrument as well as the replay. This arm
 * keeps the brackets and drops the per-delta block, which is what E26 must be
 * sized against. */
#define NDS_R2_DELTA_CENSUS \
    (NDS_TASK91_DRAW_PHASE_CENSUS && !NDS_R2_SPAN_LEAN_TIMING)

#if NDS_TASK91_DRAW_PHASE_CENSUS
/* R2-03 E20 falsifier state. Indexed by NDSNativeStateDelta::effect; 16 covers
 * the eleven cases the switch handles with room to spare. */
#define NDS_R2_DELTA_EFFECT_MAX 16u
/* R2-03 E25c. Indexed by NDS_NATIVE_STATE_*; 2..14 are the live effects. */
u32 gNdsR2DeltaEffectCounts[16];
u32 gNdsR2SpanIdenticalOperands;
u32 gNdsR2SpanIdenticalGeometry;
u32 gNdsR2SpanMaterialInvalidations;
/* R2-03 E27 is REFUTED and its probe is gone. It counted material applications
 * arriving with the texture prepare still valid -- the full re-prepares a split
 * validity would avoid -- and measured 2.0/frame against 28.0 material
 * applications: 26 of 28 invalidate a prepare the before-span deltas had already
 * dirtied. ~1,800 ticks, below the noise floor. See
 * docs/optimization/ClaudeOpus5_R203_E28_DeadSoftLight_20260728.md. The probe
 * also read state.texture_prepare_valid here, which the M3 stage falsifier
 * rejects as an unclassified read in this function. */
static u32 sNdsR2DeltaLastW0[NDS_R2_DELTA_EFFECT_MAX];
static u32 sNdsR2DeltaLastW1[NDS_R2_DELTA_EFFECT_MAX];
static u8 sNdsR2DeltaLastValid[NDS_R2_DELTA_EFFECT_MAX];
#endif

/* P2-2 placement-only reclaim: this 72-byte root preamble is the companion to
 * the 152-byte production preamble moved out above.  Both execute at root
 * granularity; keeping the per-run prepare/emit loops resident is the higher
 * value use of the fixed 32 KiB ITCM budget. */
static void __attribute__((noinline, optimize("Os")))
ndsRendererNativeApplyRootLightPreamble(
    const NDSNativeRoot *root, NDSRendererStats *stats)
{
    const u32 (*preambles)[2];
    u32 preamble_index;

    if (root->light_preamble == 0u)
    {
        return;
    }
    /* Each source gSPLightColor expands to its A/B G_MW_LIGHTCOL pair. */
    preamble_index = (u32)root->light_preamble;
    preambles = sNdsNativeFighterActiveOwner->root_light_preambles;
    if ((preambles == NULL) ||
        (preamble_index >= sNdsNativeFighterActiveOwner->root_light_preamble_count))
    {
        return;
    }
    stats->light_color_1 = preambles[preamble_index][0];
    stats->light_color_2 = preambles[preamble_index][1];
    stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_1_MASK |
        NDS_RENDERER_LIGHT_COLOR_2_MASK;
    stats->light_color_command_count += 4u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileLightColorCommands += 4u;
#endif
}

static void NDS_TASK82_ITCM_CODE
ndsRendererNativeApplyStateDelta(
    const NDSNativeStateDelta *delta,
    const u8 *asset_base,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    if ((delta == NULL) || (stats == NULL) || (state == NULL))
    {
        return;
    }
#if NDS_R2_DELTA_CENSUS
    /* R2-03 E20 falsifier. E20 counted applications that repeat a delta index
     * within a frame; that is an upper bound, because a repeat is only elidable
     * if it writes what is already there. Every case below writes stats purely
     * from delta->w0/w1, so identical operands to the previous application of
     * the same effect means identical writes -- with the one exception noted
     * for GEOMETRY, whose result also depends on the prior geometry_mode.
     *
     * Validity is cleared whenever a material is applied, because materials
     * write the same stats fields and would make a stale operand match unsafe.
     * That makes this count conservative, which is the right direction. */
    if ((u32)delta->effect < NDS_R2_DELTA_EFFECT_MAX)
    {
        u32 e = (u32)delta->effect;

        if ((sNdsR2DeltaLastValid[e] != 0u) &&
            (sNdsR2DeltaLastW0[e] == delta->w0) &&
            (sNdsR2DeltaLastW1[e] == delta->w1))
        {
            gNdsR2SpanIdenticalOperands++;
            if (delta->effect == NDS_NATIVE_STATE_GEOMETRY)
            {
                gNdsR2SpanIdenticalGeometry++;
            }
        }
        sNdsR2DeltaLastW0[e] = delta->w0;
        sNdsR2DeltaLastW1[e] = delta->w1;
        sNdsR2DeltaLastValid[e] = 1u;
    }
    /* R2-03 E25c. E25b showed the replay's cost is the texture-prepare
     * invalidation it triggers, not the write. Whether that can be replaced by
     * a cheap value check depends on WHICH effects dominate: OTHERMODE,
     * COMBINE, GEOMETRY and PRIM are a handful of scalars a run can compare,
     * but TILE, IMAGE and TEXTURE move the 20-word tile state, which is too
     * expensive to compare per run. This splits the 194.4 applications by
     * effect so the validity key can be designed against the data. */
    if (delta->effect < 16u)
    {
        gNdsR2DeltaEffectCounts[delta->effect]++;
    }
#endif
    switch (delta->effect)
    {
    case NDS_NATIVE_STATE_OTHERMODE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordOtherMode(
            stats, delta->w0 >> 24, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_COMBINE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordSetCombine(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_TEXTURE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordTextureState(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_GEOMETRY:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->geometry_mode =
            (stats->geometry_mode & delta->w0) | delta->w1;
        stats->geometry_clear_mask = delta->w0;
        stats->geometry_set_mask = delta->w1;
        stats->geometry_command_count++;
        break;
    case NDS_NATIVE_STATE_IMAGE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordSetImage(
            stats, delta->w0,
#if NDS_P2_1P_GAME
            (u32)(uintptr_t)ndsRelocNativeAssetAddress(asset_base, delta->w1));
#else
            (u32)(uintptr_t)(asset_base + delta->w1));
#endif
        break;
    case NDS_NATIVE_STATE_TILE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordSetTile(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_LOAD_TLUT:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordLoadTlut(stats, delta->w1);
        break;
    case NDS_NATIVE_STATE_LOAD_BLOCK:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordLoadBlock(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_TILE_SIZE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordSetTileSize(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_PRIM:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        NDS_FIGHTER_PACKET_HOOK(sNdsFighterPacketRecorder.prim_overridden = 1u);
        stats->prim_color = delta->w1;
        stats->prim_min_level = (delta->w0 >> 8) & 0xffu;
        stats->prim_lod_fraction = delta->w0 & 0xffu;
        stats->color_command_count++;
        break;
    /* P2-3f5. G_SETBLENDCOLOR on a FIGHTER root. The native stage program has
     * carried this effect since Task 26 (`NDS_TASK26_BLEND`, and the stage span
     * applier below); no fighter owner produced one until Captain Falcon, whose
     * high-detail root 6 brackets its two draws with G_AC_THRESHOLD and a blend
     * colour. blend_color IS the alpha reference the threshold compares
     * against -- `ndsRendererHardwareApplyAlphaTest` reads exactly these two
     * fields -- so dropping it would silently pick up whatever reference the
     * previous draw left behind. */
    case NDS_NATIVE_STATE_BLEND:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->blend_color = delta->w1;
        stats->color_command_count++;
        break;
    case NDS_NATIVE_STATE_LIGHT_COLOR:
        ndsRendererApplyMatrixMoveWordCommand(
            stats, state, delta->w0, delta->w1);
        break;
    default:
        break;
    }
}

#if NDS_TASK91_DRAW_PHASE_CENSUS
/* R2-03 E20. E19 established that the state spans cannot be priced by deleting
 * them -- they are load-bearing, and removing them takes the emit with them. So
 * this asks R2-02 F's question instead: how much of the replay is REDUNDANT.
 * There are 70 fighter state deltas; a frame stamp per delta says how many of
 * the applications in a frame are re-applying a delta that frame already
 * applied. That fraction, not the phase total, is what a guard could actually
 * elide -- it prices the achievable cut rather than the whole phase. */
u32 gNdsR2SpanCalls;
u32 gNdsR2SpanDeltasApplied;
u32 gNdsR2SpanDeltaRepeats;
/* R2-03 E38. Splits the replay across the material that sits between the two
 * spans, which decides how much of E26 is worth building. */
u32 gNdsR2SpanBeforeTicks;
u32 gNdsR2SpanBeforeDeltas;
u32 gNdsR2SpanAfterTicks;
u32 gNdsR2SpanAfterDeltas;
#if NDS_R2_DELTA_CENSUS
static u32 sNdsR2DeltaLastFrame[70];
#endif
#endif

static void
ndsRendererNativeApplyStateSpan(
    u16 first,
    u32 count,
    u32 sync_count,
    const u8 *asset_base,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 i;

    if ((count == 0u) && (sync_count == 0u))
    {
        return;
    }
    ndsRendererNativeSourceBoundary(state);
    stats->sync_command_count += sync_count;
    if ((count == 0u) || (first == NDS_NATIVE_STATE_NONE))
    {
        return;
    }
#if NDS_TASK91_DRAW_PHASE_CENSUS
    gNdsR2SpanCalls++;
#endif
    for (i = 0u; i < count; i++)
    {
        u32 delta_index =
            sNdsNativeFighterActiveTables->state_sequence[first + i];

#if NDS_R2_DELTA_CENSUS
        {
            /* +1 so an untouched entry (0) can never alias frame serial 0. */
            u32 stamp = sNdsRendererHardwareFrameSerial + 1u;

            gNdsR2SpanDeltasApplied++;
            if (delta_index <
                (sizeof(sNdsR2DeltaLastFrame) /
                 sizeof(sNdsR2DeltaLastFrame[0])))
            {
                if (sNdsR2DeltaLastFrame[delta_index] == stamp)
                {
                    gNdsR2SpanDeltaRepeats++;
                }
                sNdsR2DeltaLastFrame[delta_index] = stamp;
            }
        }
#endif
        ndsRendererNativeApplyStateDelta(
            &sNdsNativeFighterActiveTables->state_deltas[delta_index],
            asset_base, stats, state);
    }
}

static void ndsRendererNativeApplyMaterial(
    const NDSRendererNativeMaterial *material,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 effects;

    if ((material == NULL) || (stats == NULL) || (state == NULL))
    {
        return;
    }
    ndsRendererNativeSourceBoundary(state);
#if NDS_TASK91_DRAW_PHASE_CENSUS
    {
        u32 e;

        gNdsR2SpanMaterialInvalidations++;
        for (e = 0u; e < NDS_R2_DELTA_EFFECT_MAX; e++)
        {
            sNdsR2DeltaLastValid[e] = 0u;
        }
    }
#endif
    effects = material->effects;
    /* Root DE call + generated segment-E table DE jump. The root command is
     * already included in the generated source count; only the table word
     * and typed branch body are additional commands. */
    stats->command_count += (u32)material->command_count + 1u;
    stats->branch_command_count += 2u;
    stats->branch_call_count++;
    stats->branch_jump_count++;
    stats->segment_resolve_count++;
    stats->end_command_count++;
    stats->sync_command_count += material->sync_count;

    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->palette_image_w0, material->palette_image);
        ndsRendererRecordSetImage(
            stats, material->palette_image_w0, material->palette_image);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->palette_tile_w0, material->palette_tile_w1);
        ndsRendererRecordSetTile(
            stats, material->palette_tile_w0, material->palette_tile_w1);
        ndsRendererTextureSourceHashCommand(
            stats, NDS_RENDERER_OP_LOADTLUT << 24,
            material->palette_tlut_w1);
        ndsRendererRecordLoadTlut(stats, material->palette_tlut_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LIGHT1) != 0u)
    {
        ndsRendererRecordLightColor(stats, 1u, material->light1);
        ndsRendererRecordLightColor(stats, 1u, material->light1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LIGHT2) != 0u)
    {
        ndsRendererRecordLightColor(stats, 2u, material->light2);
        ndsRendererRecordLightColor(stats, 2u, material->light2);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_PRIM) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        NDS_FIGHTER_PACKET_HOOK(sNdsFighterPacketRecorder.prim_overridden = 1u);
        stats->prim_color = material->prim_w1;
        stats->prim_min_level = (material->prim_w0 >> 8) & 0xffu;
        stats->prim_lod_fraction = material->prim_w0 & 0xffu;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_ENV) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->env_color = material->env_color;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_BLEND) != 0u)
    {
        stats->blend_color = material->blend_color;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->block_image_w0, material->block_image);
        ndsRendererRecordSetImage(
            stats, material->block_image_w0, material->block_image);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LOAD_BLOCK) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->load_block_w0, material->load_block_w1);
        ndsRendererRecordLoadBlock(
            stats, material->load_block_w0, material->load_block_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->current_image_w0, material->current_image);
        ndsRendererRecordSetImage(
            stats, material->current_image_w0, material->current_image);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->render_tile_size_w0,
            material->render_tile_size_w1);
        ndsRendererRecordSetTileSize(
            stats, material->render_tile_size_w0,
            material->render_tile_size_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_SCROLL_TILE_SIZE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->scroll_tile_size_w0,
            material->scroll_tile_size_w1);
        ndsRendererRecordSetTileSize(
            stats, material->scroll_tile_size_w0,
            material->scroll_tile_size_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_TEXTURE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->texture_w0, material->texture_w1);
        ndsRendererRecordTextureState(
            stats, material->texture_w0, material->texture_w1);
    }
}

/* The hierarchy candidate replays the complete retained owner into private
 * scratch before its first GX command.  The shared state/material helpers only
 * touch GX through their source-boundary batch close, so mask the batch-open
 * bit around those exact helpers and restore it immediately.  Existing mode-8
 * execution keeps calling the original helpers byte-for-byte. */
static void ndsRendererNativeApplyStateSpanPreflight(
    u16 first,
    u32 count,
    u32 sync_count,
    const u8 *asset_base,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 batch_open = sNdsRendererHardwareTriangleBatchOpen;

    sNdsRendererHardwareTriangleBatchOpen = FALSE;
    ndsRendererNativeApplyStateSpan(
        first, count, sync_count, asset_base, stats, state);
    sNdsRendererHardwareTriangleBatchOpen = batch_open;
}

static void ndsRendererNativeApplyMaterialPreflight(
    const NDSRendererNativeMaterial *material,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 batch_open = sNdsRendererHardwareTriangleBatchOpen;

    sNdsRendererHardwareTriangleBatchOpen = FALSE;
    ndsRendererNativeApplyMaterial(material, stats, state);
    sNdsRendererHardwareTriangleBatchOpen = batch_open;
}

#if NDS_R2_IMPACT_WAVE_NATIVE
static s32 ndsRendererHardwareBindImpactWaveTexture(
    NDSRendererStats *stats, const NDSRendererTileState *render_tile,
    u32 variant)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 name;
    u32 params;

    if ((stats == NULL) || (render_tile == NULL) ||
        (variant >= NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT))
    {
        return FALSE;
    }
    name = sNdsRendererImpactWaveTextureName[variant];
    if (name == 0u)
    {
        return FALSE;
    }

    params = ndsRendererHardwareTextureParams(
        stats, render_tile, NDS_RENDERER_IMPACT_WAVE_TEX_WIDTH,
        NDS_RENDERER_IMPACT_WAVE_TEX_HEIGHT);
    ndsRendererHardwareBindTextureName(stats, name);
    ndsRendererHardwareApplyTextureParams(
        ndsRendererHardwareMergeTextureParams(params));
    /* This owner never occupies a generic cache entry. The texel index stream
     * and all five colour palettes are ROM constants uploaded at scene entry. */
    sNdsRendererHardwareActiveTextureEntry = NULL;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
    stats->hardware_texture_width = NDS_RENDERER_IMPACT_WAVE_TEX_WIDTH;
    stats->hardware_texture_height = NDS_RENDERER_IMPACT_WAVE_TEX_HEIGHT;
    gNdsImpactWaveNativeTextureBindCount++;
    return TRUE;
#else
    (void)stats;
    (void)render_tile;
    (void)variant;
    return FALSE;
#endif
}
#endif

s32 ndsRendererSubmitNativeImpactWave(
    const NDSRendererInputVertex *vertices, u32 vertex_count,
    const u8 *triangle_indices, u32 triangle_count,
    const Gfx *source_setup,
    const NDSRendererNativeMaterial *material,
    u32 variant,
    const NDSRendererConfig *config,
    NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
#if NDS_RENDERER_HW_TRIANGLES && NDS_R2_IMPACT_WAVE_NATIVE
    NDSRendererTraversalVertexStorage vertex_storage;
    NDSRendererTraversalState state;
    const NDSRendererTileState *render_tile;
    u32 required_mask;
    u32 material_color;
    u32 poly_alpha;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 use_material_color;
    u32 use_vertex_color;
    u32 implicit_texture_on;
    u32 use_texture;
    s32 texture_offset;
    v16 projected_x[18];
    v16 projected_y[18];
    u32 i;

    /* This is deliberately a closed owner, not a general mesh API. Keeping the
     * exact source cardinalities here means a bad AOT table falls back to the
     * interpreter rather than emitting a partly-valid cosmetic. */
    if ((vertices == NULL) || (vertex_count != 18u) ||
        (triangle_indices == NULL) || (triangle_count != 16u) ||
        (source_setup == NULL) || (material == NULL) ||
        (variant >= NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT) ||
        (sNdsRendererImpactWaveTextureName[variant] == 0u) ||
        (config == NULL) || (stats == NULL))
    {
        return FALSE;
    }
    required_mask = (1u << vertex_count) - 1u;
    for (i = 0u; i < (triangle_count * 3u); i++)
    {
        if ((u32)triangle_indices[i] >= vertex_count)
        {
            return FALSE;
        }
    }

    ndsRendererInitTraversalState(
        &state, config, stats, &vertex_storage, NULL, 0u);
    if (state.matrix_valid == 0u)
    {
        return FALSE;
    }

    /* Transform the immutable source ring once. The generic path loads the
     * same 18 vertices at command 19 and transforms them before its eight TRI2
     * commands. Reject before touching GX if any corner needs near clipping;
     * the generic fallback already owns the uncommon clipped case. */
    for (i = 0u; i < vertex_count; i++)
    {
        u32 mask = 1u << i;
        NDSRendererClipVertex20p12 *out = &state.vertices[i];

        state.input_vertices[i] = vertices[i];
        state.input_vertex_valid_mask |= mask;
        state.current_transform_vertex_mask |= mask;
        ndsRendererTransformVertex20p12(&state.matrix, &vertices[i], out);
        if (ndsRendererHardwareClipZWInsideNearPlane(out->z, out->w) == FALSE)
        {
            return FALSE;
        }
        /* ImpactWave reuses the same 18 ring vertices across 48 triangle
         * corners. The generic projected path perspective-divides X/Y for
         * every corner, even when that vertex was already emitted by the
         * previous TRI2. Do the invariant divide once per unique source vertex
         * here; painter Z remains per-triangle below. This cuts the ring's
         * X/Y perspective divides from 96 to 36 per draw without changing a
         * coordinate or the source's submission order. */
        projected_x[i] = ndsRendererHardwareProjectToV16(
            (s64)out->x * NDS_RENDERER_HW_PROJECTED_VERTEX, out->w);
        projected_y[i] = ndsRendererHardwareProjectToV16(
            (s64)out->y * NDS_RENDERER_HW_PROJECTED_VERTEX, out->w);
        state.vertex_valid_mask |= mask;
        stats->matrix_transform_count++;
        stats->transformed_vertex_count++;
        if (stats->transformed_vertex_count == 1u)
        {
            stats->first_transformed_x = out->x;
            stats->first_transformed_y = out->y;
            stats->first_transformed_z = out->z;
            stats->first_transformed_w = out->w;
        }
        ndsRendererProfileRecordCPUTransform();
        ndsRendererProfileRecordSourceVertexLoad();
    }
    if (stats->vertex_count < vertex_count)
    {
        stats->vertex_count = vertex_count;
    }

    /* Only now cross the side-effect boundary. The 35-command source list is
     * fixed; replay its state mutations directly and let the existing typed
     * MObj material owner supply the dynamic segment-E body. This preserves the
     * source command order without scanning opcodes/branches/reloc ownership. */
    ndsRendererHardwareEndBatch();
    if (stats->first_opcode == 0u)
    {
        stats->first_opcode = source_setup[0].words.w0 >> 24;
    }
    stats->command_count += 35u;
    stats->sync_command_count += 8u;

#define NDS_IMPACT_HASH(index_) \
    ndsRendererTextureSourceHashCommand( \
        stats, source_setup[(index_)].words.w0, source_setup[(index_)].words.w1)
#define NDS_IMPACT_INVALIDATE() NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(&state)

    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(1u);
    ndsRendererRecordOtherMode(
        stats, source_setup[1].words.w0 >> 24,
        source_setup[1].words.w0, source_setup[1].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(2u);
    ndsRendererRecordOtherMode(
        stats, source_setup[2].words.w0 >> 24,
        source_setup[2].words.w0, source_setup[2].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(3u);
    ndsRendererRecordSetCombine(
        stats, source_setup[3].words.w0, source_setup[3].words.w1);
    stats->blend_color = source_setup[4].words.w1;
    stats->color_command_count++;
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(6u);
    ndsRendererRecordSetTile(
        stats, source_setup[6].words.w0, source_setup[6].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(7u);
    ndsRendererRecordSetTile(
        stats, source_setup[7].words.w0, source_setup[7].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(8u);
    ndsRendererRecordSetTileSize(
        stats, source_setup[8].words.w0, source_setup[8].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(9u);
    ndsRendererRecordSetImage(
        stats, source_setup[9].words.w0, source_setup[9].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(11u);
    ndsRendererRecordLoadTlut(stats, source_setup[11].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(13u);
    ndsRendererRecordSetImage(
        stats, source_setup[13].words.w0, source_setup[13].words.w1);

    ndsRendererNativeApplyMaterial(material, stats, &state);

    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(16u);
    ndsRendererRecordLoadBlock(
        stats, source_setup[16].words.w0, source_setup[16].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(18u);
    stats->geometry_mode =
        (stats->geometry_mode & source_setup[18].words.w0) |
        source_setup[18].words.w1;
    stats->geometry_clear_mask = source_setup[18].words.w0;
    stats->geometry_set_mask = source_setup[18].words.w1;
    stats->geometry_command_count++;

    /* VTX command 19 observes lighting already cleared, so its shade result is
     * simply the source's white RGBA. Compute through the shared helper anyway
     * rather than baking a renderer-specific packed-colour convention. */
    for (i = 0u; i < vertex_count; i++)
    {
        state.vertex_colors[i] = ndsRendererHardwareLitShadeColorPrepared(
            stats, &state.input_vertices[i], NULL);
    }
    state.vertex_color_valid_mask = required_mask;

    material_color = ndsRendererHardwareColorSource(stats);
    use_material_color =
        (ndsRendererHardwareUseMaterialColor(stats) != FALSE) ? TRUE : FALSE;
    use_vertex_color =
        (ndsRendererHardwareUseVertexColor(stats) != FALSE) ? TRUE : FALSE;
    state.texture_prepare_material_color = material_color;
    state.texture_prepare_vertex_flags =
        ((use_material_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
        ((use_vertex_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);

    poly_alpha = ndsRendererHardwareAlpha(stats, &state.input_vertices[0]);
    state.texture_prepare_alpha_constant =
        (ndsRendererHardwareAlphaUsesVertex(stats) == FALSE) ? TRUE : FALSE;
    state.texture_prepare_poly_alpha = poly_alpha;
    state.texture_prepare_poly_fmt = ndsRendererHardwarePolyFmt(stats, poly_alpha);
    render_tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    implicit_texture_on =
        ndsRendererHardwareTextureImplicitStateOn(stats) ? TRUE : FALSE;
    /* The source state above remains the oracle for UV/filter/alpha semantics,
     * but PIXELS are no longer resolved from it. DL_0x7C28's 16x32 sampled CI4 indices and
     * PRIM/ENV colour combine were compiled AOT into DS PAL16. A valid native
     * ImpactWave therefore reaches a resident-name bind here and never enters
     * ndsRendererHardwareBindTexture / the generic N64 conversion cache. */
    use_texture =
        (ndsRendererHardwareUseTexture(stats) != FALSE) &&
        (ndsRendererHardwareBindImpactWaveTexture(
             stats, render_tile, variant) != FALSE) ? TRUE : FALSE;
    if (use_texture == FALSE)
    {
        return FALSE;
    }
    state.texture_prepare_valid = TRUE;
    state.texture_prepare_enabled = use_texture;
    state.texture_prepare_name =
        (use_texture != FALSE) ? sNdsRendererHardwareBoundTextureName : 0u;
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
    state.texture_prepare_scale_s = texture_scale_s;
    state.texture_prepare_scale_t = texture_scale_t;
    state.texture_prepare_origin_s = render_tile->uls;
    state.texture_prepare_origin_t = render_tile->ult;
    state.texture_prepare_offset = texture_offset;
    if (use_texture != FALSE)
    {
        state.texture_prepare_vertex_flags |=
            NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
    }
#if NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY
    if (use_texture != FALSE)
    {
        state.texture_prepare_vertex_flags &=
            ~(NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL |
              NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX);
    }
#endif
    /* Deliberately omit SOURCE_CLIP_DEPTH/ZBUFFERED: this is source non-Z
     * geometry and must retain the port's per-triangle painter-depth emulation. */
    ndsRendererFastPrepareRawSlots(
        stats, &state, required_mask, use_texture);

    if (poly_alpha != 0u)
    {
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
        ndsRendererHardwareBeginTriangleBatch(
            stats, use_texture, state.texture_prepare_name,
            state.texture_prepare_poly_fmt,
            sNdsRendererHardwareMatrixMode,
            sNdsRendererHardwareMatrixGeneration);

        for (i = 0u; i < triangle_count; i++)
        {
            const u8 *tri = &triangle_indices[i * 3u];
            s32 depth = ndsRendererHardwareNextProjectedDepth();
            u32 corner;

            for (corner = 0u; corner < 3u; corner++)
            {
                u32 index = (u32)tri[corner];
                v16 out_z = ndsRendererHardwareClampS64ToV16(depth);

                glColor(state.prepared_vertex_colors[index]);
                if (use_texture != FALSE)
                {
                    glTexCoord2t16(state.prepared_texcoord_s[index],
                                  state.prepared_texcoord_t[index]);
                }
                ndsRendererProfileHWVertexRange(
                    projected_x[index], projected_y[index], out_z);
                glVertex3v16(projected_x[index], projected_y[index], out_z);
            }
            sNdsRendererHardwareSubmitted = TRUE;
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
            sNdsRendererBenchmarkTriangleCount++;
#endif
            stats->triangle_count++;
            stats->transformed_triangle_count++;
            stats->hardware_triangle_count++;
            stats->hardware_vertex_count += 3u;
            stats->hardware_projected_depth_triangle_count++;
            ndsRendererProfileRecordProjectedSubmit();
            ndsRendererProfileRecordHardwareTriangle();
        }
    }
    ndsRendererHardwareEndBatch();

    /* Source commands 28..34 restore the sticky state inherited by the next
     * effect list. Preserve them even though they draw no geometry. */
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(30u);
    stats->geometry_mode =
        (stats->geometry_mode & source_setup[30].words.w0) |
        source_setup[30].words.w1;
    stats->geometry_clear_mask = source_setup[30].words.w0;
    stats->geometry_set_mask = source_setup[30].words.w1;
    stats->geometry_command_count++;
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(31u);
    ndsRendererRecordOtherMode(
        stats, source_setup[31].words.w0 >> 24,
        source_setup[31].words.w0, source_setup[31].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(32u);
    ndsRendererRecordOtherMode(
        stats, source_setup[32].words.w0 >> 24,
        source_setup[32].words.w0, source_setup[32].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(33u);
    ndsRendererRecordOtherMode(
        stats, source_setup[33].words.w0 >> 24,
        source_setup[33].words.w0, source_setup[33].words.w1);
    stats->end_command_count++;

#undef NDS_IMPACT_INVALIDATE
#undef NDS_IMPACT_HASH
    return TRUE;
#else
    (void)vertices;
    (void)vertex_count;
    (void)triangle_indices;
    (void)triangle_count;
    (void)source_setup;
    (void)material;
    (void)variant;
    (void)config;
    (void)stats;
    return FALSE;
#endif
}

#if NDS_R2_REBIRTH_HALO_NATIVE
#if NDS_R2_REBIRTH_HALO_PHASE_PROFILE
volatile u32 gNdsRebirthHaloPhaseTicks[8];
#define NDS_REBIRTH_PHASE_MARK(slot, before) \
    do { gNdsRebirthHaloPhaseTicks[(slot)] += cpuGetTiming() - (before); } while (0)
#else
#define NDS_REBIRTH_PHASE_MARK(slot, before) do { (void)(before); } while (0)
#endif
static s32 ndsRendererHardwareBindRebirthHaloTexture(
    NDSRendererStats *stats, const NDSRendererTileState *render_tile,
    u32 texture_slot, u32 upload_width, u32 upload_height, u32 format)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 name;
    u32 params;

    if ((stats == NULL) || (render_tile == NULL) ||
        (texture_slot >= NDS_REBIRTH_HALO_TEXTURE_COUNT))
    {
        return FALSE;
    }
    name = sNdsRendererRebirthHaloTextureName[texture_slot];
    if (name == 0u)
    {
        return FALSE;
    }
    params = ndsRendererHardwareTextureParams(
        stats, render_tile, upload_width, upload_height);
    ndsRendererHardwareBindTextureName(stats, name);
    ndsRendererHardwareApplyTextureParams(
        ndsRendererHardwareMergeTextureParams(params));
    sNdsRendererHardwareActiveTextureEntry = NULL;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = format;
    stats->hardware_texture_width = upload_width;
    stats->hardware_texture_height = upload_height;
    gNdsRebirthHaloNativeTextureBindCount++;
    return TRUE;
#else
    (void)stats;
    (void)render_tile;
    (void)texture_slot;
    (void)upload_width;
    (void)upload_height;
    (void)format;
    return FALSE;
#endif
}

static void ndsRendererRebirthHaloGeometry(NDSRendererStats *stats, u32 mode)
{
    stats->geometry_mode = mode;
    stats->geometry_clear_mask = 0u;
    stats->geometry_set_mask = mode;
    stats->geometry_command_count++;
}

static void ndsRendererRebirthHaloSetLightColors(
    NDSRendererStats *stats, u32 light1, u32 light2)
{
    ndsRendererRecordLightColor(stats, 1u, light1);
    ndsRendererRecordLightColor(stats, 2u, light2);
}

static s32 ndsRendererRebirthHaloPrepareGroup(
    u32 group_index,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 *use_texture_out)
{
    const NDSRendererTileState *render_tile = NULL;
    u32 texture_slot = NDS_REBIRTH_HALO_TEXTURE_COUNT;
    u32 upload_width = 0u;
    u32 upload_height = 0u;
    u32 format = 0u;
    u32 material_color;
    u32 use_material_color;
    u32 use_vertex_color;
    u32 implicit_texture_on;
    u32 use_texture = FALSE;
    u32 texture_scale_s;
    u32 texture_scale_t;
    s32 texture_offset;
    u32 poly_alpha;

    if ((stats == NULL) || (state == NULL) || (use_texture_out == NULL) ||
        (group_index >= NDS_REBIRTH_HALO_GROUP_COUNT))
    {
        return FALSE;
    }
    NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);

    switch (group_index)
    {
    case 0u: /* main untextured lit body */
        ndsRendererRebirthHaloGeometry(stats, 0x00220404u);
        ndsRendererRebirthHaloSetLightColors(
            stats, 0xffffff00u, 0xb3b3b300u);
        ndsRendererRecordSetCombine(stats, 0xfcffffffu, 0xfffe7d3eu);
        ndsRendererRecordTextureState(stats, 0xd7000000u, 0x00000000u);
        break;
    case 1u: /* main CI4 E58 */
        ndsRendererRebirthHaloGeometry(stats, 0x00220404u);
        ndsRendererRebirthHaloSetLightColors(
            stats, 0xffffff00u, 0xb3b3b300u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00008000u);
        ndsRendererRecordSetCombine(stats, 0xfc127e24u, 0xfffff3f9u);
        ndsRendererRecordSetTile(stats, 0xf5400200u, 0x0008c230u);
        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x0009c01cu);
        texture_slot = NDS_REBIRTH_HALO_TEXTURE_E58;
        upload_width = NDS_REBIRTH_HALO_E58_WIDTH;
        upload_height = NDS_REBIRTH_HALO_E58_HEIGHT;
        format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
        break;
    case 2u: /* main CI4 DD0 */
        ndsRendererRebirthHaloGeometry(stats, 0x00220004u);
        ndsRendererRebirthHaloSetLightColors(
            stats, 0xffffff00u, 0xcccccc00u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00008000u);
        ndsRendererRecordSetCombine(stats, 0xfc127e24u, 0xfffff3f9u);
        ndsRendererRecordSetTile(stats, 0xf5400200u, 0x00090030u);
        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x004fc03cu);
        texture_slot = NDS_REBIRTH_HALO_TEXTURE_DD0;
        upload_width = NDS_REBIRTH_HALO_DD0_WIDTH;
        upload_height = NDS_REBIRTH_HALO_DD0_HEIGHT;
        format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
        break;
    case 3u: /* main CI4 BC8, source lighting already cleared */
        ndsRendererRebirthHaloGeometry(stats, 0x00000004u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00008000u);
        ndsRendererRecordSetCombine(stats, 0xfc127e24u, 0xfffff3f9u);
        ndsRendererRecordSetTile(stats, 0xf5400400u, 0x000d4350u);
        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x000fc0fcu);
        texture_slot = NDS_REBIRTH_HALO_TEXTURE_BC8;
        upload_width = NDS_REBIRTH_HALO_BC8_WIDTH;
        upload_height = NDS_REBIRTH_HALO_BC8_HEIGHT;
        format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
        break;
    case 4u: /* white beam: I4 source coverage -> A5I3 */
        ndsRendererRebirthHaloGeometry(stats, 0x00000000u);
        ndsRendererRecordSetCombine(stats, 0xfcffffffu, 0xfffdf2f9u);
        stats->prim_color = 0xffffffffu;
        stats->color_command_count++;
        ndsRendererRecordSetTile(stats, 0xf5800200u, 0x00090230u);
        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x0001c03cu);
        texture_slot = NDS_REBIRTH_HALO_TEXTURE_BEAM;
        upload_width = NDS_REBIRTH_HALO_BEAM_WIDTH;
        upload_height = NDS_REBIRTH_HALO_BEAM_HEIGHT;
        format = NDS_RENDERER_HW_TEXTURE_FMT_I16;
        break;
    case 5u: /* four CI4 edge leaves, collapsed into one native group */
        ndsRendererRebirthHaloGeometry(stats, 0x00000004u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00008000u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_L,
            0xe2001e01u, 0x00000001u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_L,
            0xe200001cu, 0x00553078u);
        ndsRendererRecordSetCombine(stats, 0xfc127e24u, 0xfffff3f9u);
        stats->blend_color = 0x00000020u;
        stats->color_command_count++;
        ndsRendererRecordSetTile(stats, 0xf5400600u, 0x00090260u);
        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x0009c03cu);
        texture_slot = NDS_REBIRTH_HALO_TEXTURE_A40;
        upload_width = NDS_REBIRTH_HALO_A40_WIDTH;
        upload_height = NDS_REBIRTH_HALO_A40_HEIGHT;
        format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
        break;
    default:
        return FALSE;
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material_color =
        (ndsRendererHardwareUseMaterialColor(stats) != FALSE) ? TRUE : FALSE;
    use_vertex_color =
        (ndsRendererHardwareUseVertexColor(stats) != FALSE) ? TRUE : FALSE;
    state->texture_prepare_material_color = material_color;
    state->texture_prepare_vertex_flags =
        ((use_material_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
        ((use_vertex_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);

    poly_alpha = ndsRendererHardwareAlpha(stats, &state->input_vertices[0]);
    state->texture_prepare_alpha_constant =
        (ndsRendererHardwareAlphaUsesVertex(stats) == FALSE) ? TRUE : FALSE;
    state->texture_prepare_poly_alpha = poly_alpha;
    state->texture_prepare_poly_fmt = ndsRendererHardwarePolyFmt(stats, poly_alpha);
    implicit_texture_on =
        ndsRendererHardwareTextureImplicitStateOn(stats) ? TRUE : FALSE;
    if (texture_slot < NDS_REBIRTH_HALO_TEXTURE_COUNT)
    {
        render_tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
        use_texture = ndsRendererHardwareBindRebirthHaloTexture(
            stats, render_tile, texture_slot, upload_width, upload_height,
            format);
        if (use_texture == FALSE)
        {
            return FALSE;
        }
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled = TRUE;
        state->texture_prepare_name = sNdsRendererHardwareBoundTextureName;
        texture_scale_s = stats->texture_scale_s;
        texture_scale_t = stats->texture_scale_t;
        if (implicit_texture_on != FALSE)
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
        state->texture_prepare_vertex_flags |=
            NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        ndsRendererProfileRecordTexturePrepare();
    }
    else
    {
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled = FALSE;
        state->texture_prepare_name = 0u;
    }
    *use_texture_out = use_texture;
    return TRUE;
}

static void ndsRendererRebirthHaloFinishRoot(
    u32 root_offset, NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    if (root_offset == 0x2378u)
    {
        ndsRendererRebirthHaloGeometry(stats, 0x00220004u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00000000u);
    }
    else if (root_offset == 0x2a88u)
    {
        ndsRendererRebirthHaloGeometry(stats, 0x00220004u);
    }
    else if (root_offset == 0x27e8u)
    {
        ndsRendererRebirthHaloGeometry(stats, 0x00220004u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00000000u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_L,
            0xe2001e01u, 0x00000000u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_L,
            0xe200001cu, 0x00552078u);
    }
    NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
}

#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
static s32 ndsRendererRebirthHaloBoundsInsideNearPlane(
    const NDSRendererMatrix20p12 *matrix,
    const NDSRebirthHaloBounds *bounds)
{
    u32 corner;

    for (corner = 0u; corner < 8u; corner++)
    {
        NDSRendererInputVertex input;
        NDSRendererClipVertex20p12 clip;

        /* The transform reads only XYZ. Avoid clearing the texture/normal tail
         * of this temporary eight times per admission test. */
        input.x = ((corner & 1u) != 0u) ? bounds->max_x : bounds->min_x;
        input.y = ((corner & 2u) != 0u) ? bounds->max_y : bounds->min_y;
        input.z = ((corner & 4u) != 0u) ? bounds->max_z : bounds->min_z;
        ndsRendererTransformVertex20p12(matrix, &input, &clip);
        gNdsRebirthHaloFullOffloadBoundCornerCount++;
        if (ndsRendererHardwareClipZWInsideNearPlane(clip.z, clip.w) == FALSE)
        {
            gNdsRebirthHaloFullOffloadBoundRejectCount++;
            return FALSE;
        }
    }
    return TRUE;
}

/* Conservative union per native RebirthHalo root.  Groups 0..3 share the
 * 0x2378 child transform, so their four AABBs do not need four separate
 * eight-corner clip tests.  The other two roots each own one generated group. */
static const NDSRebirthHaloBounds sNdsRebirthHaloRootBounds[3] = {
    { -276, -90, -239, 276, 60, 239 },
    { -242, 60, -209, 242, 240, 209 },
    { -310, -59, -310, 310, 0, 310 },
};

static void ndsRendererRebirthHaloLoadNoZMatrix(
    const NDSRendererMatrix20p12 *raw_matrix,
    s16 projected_z)
{
    NDSRendererMatrix20p12 matrix = *raw_matrix;
    m4x4 hardware;
    u32 row;

    /* Same source-no-Z contract used by the native Dream Land owner: retain
     * GX-owned X/Y/W transform and replace only clip Z with painter_z * W.
     * This is why the vertex can stay in model space without changing overlap
     * against fighters or the stage. */
    for (row = 0u; row < 4u; row++)
    {
        matrix.m[row][2] = (s32)ndsRendererRoundShiftS64(
            (s64)matrix.m[row][3] * projected_z, 12u);
    }
    ndsRendererCopyMtx20p12ToM4x4(&matrix, &hardware);
    glLoadMatrix4x4(&hardware);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixLoaded = FALSE;
}

#if NDS_R2_REBIRTH_HALO_SPLIT_MTX && NDS_R2_REBIRTH_HALO_SPLIT_NOZ
static void ndsRendererRebirthHaloLoadSplitNoZProjection(
    const NDSRendererMatrix20p12 *projection,
    s16 projected_z)
{
    NDSRendererMatrix20p12 matrix = *projection;
    m4x4 hardware;
    u32 row;

    /* Row-vector convention: clip.z is the dot against projection column 2,
     * clip.w against column 3.  Setting Zcol = painter_z * Wcol therefore gives
     * clip.z = painter_z * clip.w for every vertex while leaving X/Y/W and the
     * modelview/vector matrix untouched. */
    for (row = 0u; row < 4u; row++)
    {
        matrix.m[row][2] = (s32)ndsRendererRoundShiftS64(
            (s64)matrix.m[row][3] * projected_z, 12u);
    }
    ndsRendererCopyMtx20p12ToM4x4(&matrix, &hardware);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&hardware);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixLoaded = FALSE;
}
#endif

#if NDS_R2_REBIRTH_HALO_HW_LIGHT
#define NDS_REBIRTH_HALO_NORMAL_PACK(x, y, z) \
    ((((u32)(x)) & 0x3ffu) | \
     (((((u32)(y)) & 0x3ffu)) << 10) | \
     (((((u32)(z)) & 0x3ffu)) << 20))

static s32 ndsRendererRebirthHaloNormalComponent(s32 source)
{
    s32 scaled = (source * 0x1ff) / 127;

    if (scaled > 511) { scaled = 511; }
    if (scaled < -512) { scaled = -512; }
    return scaled;
}

static u16 ndsRendererRebirthHaloMaterialColor15(
    u32 light_color, u32 material_color, u32 use_material,
    u32 color_modulate)
{
    u32 r = (use_material != 0u) ?
        ndsRendererHardwareScaleMaterialChannel5(
            (light_color >> 24) & 0xffu,
            (material_color >> 24) & 0xffu) :
        ((light_color >> 27) & 0x1fu);
    u32 g = (use_material != 0u) ?
        ndsRendererHardwareScaleMaterialChannel5(
            (light_color >> 16) & 0xffu,
            (material_color >> 16) & 0xffu) :
        ((light_color >> 19) & 0x1fu);
    u32 b = (use_material != 0u) ?
        ndsRendererHardwareScaleMaterialChannel5(
            (light_color >> 8) & 0xffu,
            (material_color >> 8) & 0xffu) :
        ((light_color >> 11) & 0x1fu);

    return ndsRendererHardwareModulatePackedColor(
        RGB15(r, g, b), color_modulate);
}
#endif

#if NDS_R2_REBIRTH_HALO_PACKED_FIFO
#define NDS_REBIRTH_HALO_PACKET_WORDS 256u
typedef struct NDSRebirthHaloPacket
{
    u32 words[NDS_REBIRTH_HALO_PACKET_WORDS];
    u16 word_count;
    u8 valid;
    u8 pending_count;
    u8 pending_opcode[4];
    u8 pending_param_count[4];
    u8 pending_dynamic_color[4];
    u8 dynamic_color_count;
    u16 dynamic_color_word_offset[36];
    u32 pending_param[4][2];
} NDSRebirthHaloPacket;

static NDSRebirthHaloPacket
    sNdsRebirthHaloPackets[NDS_REBIRTH_HALO_GROUP_COUNT]
    __attribute__((aligned(32)));
volatile u32 gNdsRebirthHaloPackedBuildCount;
volatile u32 gNdsRebirthHaloPackedSubmitCount;
volatile u32 gNdsRebirthHaloPackedWordCount;

static s32 ndsRendererRebirthHaloPacketPushWord(
    NDSRebirthHaloPacket *packet, u32 word)
{
    if ((packet == NULL) ||
        ((u32)packet->word_count >= NDS_REBIRTH_HALO_PACKET_WORDS))
    {
        return FALSE;
    }
    packet->words[packet->word_count++] = word;
    return TRUE;
}

static s32 ndsRendererRebirthHaloPacketFlush(NDSRebirthHaloPacket *packet)
{
    u32 command_word = 0u;
    u32 command;

    if ((packet == NULL) || (packet->pending_count == 0u))
    {
        return TRUE;
    }
    for (command = 0u; command < (u32)packet->pending_count; command++)
    {
        command_word |=
            (u32)packet->pending_opcode[command] << (command * 8u);
    }
    if (ndsRendererRebirthHaloPacketPushWord(packet, command_word) == FALSE)
    {
        return FALSE;
    }
    for (command = 0u; command < (u32)packet->pending_count; command++)
    {
        u32 parameter;
        for (parameter = 0u;
             parameter < (u32)packet->pending_param_count[command];
             parameter++)
        {
            if ((parameter == 0u) &&
                (packet->pending_dynamic_color[command] != 0u))
            {
                if ((u32)packet->dynamic_color_count >= 36u)
                {
                    return FALSE;
                }
                packet->dynamic_color_word_offset[packet->dynamic_color_count++] =
                    packet->word_count;
            }
            if (ndsRendererRebirthHaloPacketPushWord(
                    packet, packet->pending_param[command][parameter]) == FALSE)
            {
                return FALSE;
            }
        }
    }
    packet->pending_count = 0u;
    return TRUE;
}

static s32 ndsRendererRebirthHaloPacketCommand(
    NDSRebirthHaloPacket *packet, u8 opcode,
    u32 parameter_count, u32 parameter0, u32 parameter1,
    u32 dynamic_color)
{
    u32 slot;

    if ((packet == NULL) || (parameter_count > 2u))
    {
        return FALSE;
    }
    slot = packet->pending_count;
    if (slot >= 4u)
    {
        if (ndsRendererRebirthHaloPacketFlush(packet) == FALSE)
        {
            return FALSE;
        }
        slot = 0u;
    }
    packet->pending_opcode[slot] = opcode;
    packet->pending_param_count[slot] = (u8)parameter_count;
    packet->pending_dynamic_color[slot] =
        (dynamic_color != FALSE) ? 1u : 0u;
    packet->pending_param[slot][0] = parameter0;
    packet->pending_param[slot][1] = parameter1;
    packet->pending_count = (u8)(slot + 1u);
    if (packet->pending_count == 4u)
    {
        return ndsRendererRebirthHaloPacketFlush(packet);
    }
    return TRUE;
}

static s32 ndsRendererRebirthHaloBuildPackedGroup(
    u32 group_index, const NDSRebirthHaloGroup *group,
    NDSRendererStats *stats, NDSRendererTraversalState *state,
    const NDSRendererHardwareLightDirection *prepared_direction,
    u32 use_texture, u32 use_hw_light)
{
    NDSRebirthHaloPacket *packet;
    u32 context_flags;
    u32 corner_count;
    u32 corner;

    if ((group_index >= NDS_REBIRTH_HALO_GROUP_COUNT) ||
        (group == NULL) || (stats == NULL) || (state == NULL))
    {
        return FALSE;
    }
    packet = &sNdsRebirthHaloPackets[group_index];
    if (packet->valid != 0u)
    {
        return TRUE;
    }
    packet->word_count = 0u;
    packet->pending_count = 0u;
    packet->dynamic_color_count = 0u;
    context_flags = state->texture_prepare_vertex_flags;
    corner_count = (u32)group->triangle_count * 3u;

    for (corner = 0u; corner < corner_count; corner++)
    {
        u32 source_index = (u32)group->first_vertex + corner;
        const NDSRendererInputVertex *vtx =
            &sNdsRebirthHaloVertices[source_index];
        v16 x = ndsRendererHardwareVertexCoord(vtx->x, TRUE);
        v16 y = ndsRendererHardwareVertexCoord(vtx->y, TRUE);
        v16 z = ndsRendererHardwareVertexCoord(vtx->z, TRUE);

#if NDS_R2_REBIRTH_HALO_HW_LIGHT
        if (use_hw_light != FALSE)
        {
            u32 normal = NDS_REBIRTH_HALO_NORMAL_PACK(
                ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->r),
                ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->g),
                ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->b));
            if (ndsRendererRebirthHaloPacketCommand(
                    packet, FIFO_NORMAL, 1u, normal, 0u, FALSE) == FALSE)
            {
                return FALSE;
            }
        }
        else
#endif
        {
            u32 shade = ndsRendererHardwareLitShadeColorPrepared(
                stats, vtx, prepared_direction);
            u16 packed_color = ndsRendererHardwarePackedVertexColor(
                stats, vtx, state->texture_prepare_material_color,
                (s32)(context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL),
                (s32)(context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX),
                shade, TRUE, state->color_modulate);
            if (ndsRendererRebirthHaloPacketCommand(
                    packet, FIFO_COLOR, 1u, (u32)packed_color, 0u,
                    (prepared_direction != NULL) ? TRUE : FALSE) == FALSE)
            {
                return FALSE;
            }
        }
        if (use_texture != FALSE)
        {
            t16 s = ndsRendererHardwareTexCoord(
                vtx->s, state->texture_prepare_scale_s,
                state->texture_prepare_origin_s,
                state->texture_prepare_offset);
            t16 t = ndsRendererHardwareTexCoord(
                vtx->t, state->texture_prepare_scale_t,
                state->texture_prepare_origin_t,
                state->texture_prepare_offset);
            if (ndsRendererRebirthHaloPacketCommand(
                    packet, FIFO_TEX_COORD, 1u,
                    (u32)TEXTURE_PACK(s, t), 0u, FALSE) == FALSE)
            {
                return FALSE;
            }
        }
        if (ndsRendererRebirthHaloPacketCommand(
                packet, FIFO_VERTEX16, 2u,
                ((u32)(u16)x) | ((u32)(u16)y << 16),
                (u32)(s32)z, FALSE) == FALSE)
        {
            return FALSE;
        }
    }
    if (ndsRendererRebirthHaloPacketFlush(packet) == FALSE)
    {
        return FALSE;
    }
    DC_FlushRange(packet->words, (u32)packet->word_count * sizeof(u32));
    packet->valid = 1u;
    gNdsRebirthHaloPackedBuildCount++;
    gNdsRebirthHaloPackedWordCount += packet->word_count;
    return TRUE;
}

static s32 ndsRendererRebirthHaloPatchPackedColors(
    u32 group_index, const NDSRebirthHaloGroup *group,
    NDSRendererStats *stats, NDSRendererTraversalState *state,
    const NDSRendererHardwareLightDirection *prepared_direction)
{
    NDSRebirthHaloPacket *packet;
    u32 context_flags;
    u32 corner_count;
    u32 corner;

    if ((group_index >= NDS_REBIRTH_HALO_GROUP_COUNT) ||
        (group == NULL) || (stats == NULL) || (state == NULL))
    {
        return FALSE;
    }
    packet = &sNdsRebirthHaloPackets[group_index];
    corner_count = (u32)group->triangle_count * 3u;
    if (packet->dynamic_color_count == 0u)
    {
        return TRUE;
    }
    if ((prepared_direction == NULL) ||
        ((u32)packet->dynamic_color_count != corner_count))
    {
        return FALSE;
    }
    context_flags = state->texture_prepare_vertex_flags;
    for (corner = 0u; corner < corner_count; corner++)
    {
        u32 source_index = (u32)group->first_vertex + corner;
        const NDSRendererInputVertex *vtx =
            &sNdsRebirthHaloVertices[source_index];
        u32 shade = ndsRendererHardwareLitShadeColorPrepared(
            stats, vtx, prepared_direction);
        u16 packed_color = ndsRendererHardwarePackedVertexColor(
            stats, vtx, state->texture_prepare_material_color,
            (s32)(context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL),
            (s32)(context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX),
            shade, TRUE, state->color_modulate);
        packet->words[packet->dynamic_color_word_offset[corner]] =
            (u32)packed_color;
    }
    DC_FlushRange(packet->words, (u32)packet->word_count * sizeof(u32));
    return TRUE;
}

#if NDS_R2_REBIRTH_HALO_PACKED_PROJECTED
static s32 ndsRendererRebirthHaloBuildPackedProjectedGroup(
    u32 group_index, const NDSRebirthHaloGroup *group,
    NDSRendererStats *stats, NDSRendererTraversalState *state,
    const NDSRendererHardwareLightDirection *prepared_direction,
    u32 use_texture)
{
    NDSRebirthHaloPacket *packet;
    u32 context_flags;
    u32 triangle;

    if ((group_index >= NDS_REBIRTH_HALO_GROUP_COUNT) ||
        (group == NULL) || (stats == NULL) || (state == NULL))
    {
        return FALSE;
    }
    packet = &sNdsRebirthHaloPackets[group_index];
    packet->valid = 0u;
    packet->word_count = 0u;
    packet->pending_count = 0u;
    packet->dynamic_color_count = 0u;
    context_flags = state->texture_prepare_vertex_flags;

    for (triangle = 0u; triangle < (u32)group->triangle_count; triangle++)
    {
        s32 depth = ndsRendererHardwareNextProjectedDepth();
        u32 corner;

        for (corner = 0u; corner < 3u; corner++)
        {
            u32 source_index =
                (u32)group->first_vertex + triangle * 3u + corner;
            const NDSRendererInputVertex *vtx =
                &sNdsRebirthHaloVertices[source_index];
            NDSRendererClipVertex20p12 clip;
            u32 shade = ndsRendererHardwareLitShadeColorPrepared(
                stats, vtx, prepared_direction);
            u16 packed_color = ndsRendererHardwarePackedVertexColor(
                stats, vtx, state->texture_prepare_material_color,
                (s32)(context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL),
                (s32)(context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX),
                shade, TRUE, state->color_modulate);
            v16 projected_x;
            v16 projected_y;
            v16 out_z = ndsRendererHardwareClampS64ToV16(depth);

            if (ndsRendererRebirthHaloPacketCommand(
                    packet, FIFO_COLOR, 1u, (u32)packed_color, 0u, FALSE) == FALSE)
            {
                return FALSE;
            }
            if (use_texture != FALSE)
            {
                t16 s = ndsRendererHardwareTexCoord(
                    vtx->s, state->texture_prepare_scale_s,
                    state->texture_prepare_origin_s,
                    state->texture_prepare_offset);
                t16 t = ndsRendererHardwareTexCoord(
                    vtx->t, state->texture_prepare_scale_t,
                    state->texture_prepare_origin_t,
                    state->texture_prepare_offset);
                if (ndsRendererRebirthHaloPacketCommand(
                        packet, FIFO_TEX_COORD, 1u,
                        (u32)TEXTURE_PACK(s, t), 0u, FALSE) == FALSE)
                {
                    return FALSE;
                }
            }
            ndsRendererTransformVertex20p12(&state->matrix, vtx, &clip);
            projected_x = ndsRendererHardwareProjectToV16(
                (s64)clip.x * NDS_RENDERER_HW_PROJECTED_VERTEX, clip.w);
            projected_y = ndsRendererHardwareProjectToV16(
                (s64)clip.y * NDS_RENDERER_HW_PROJECTED_VERTEX, clip.w);
            stats->matrix_transform_count++;
            stats->transformed_vertex_count++;
            ndsRendererProfileRecordCPUTransform();
            ndsRendererProfileRecordSourceVertexLoad();
            ndsRendererProfileHWVertexRange(projected_x, projected_y, out_z);
            if (ndsRendererRebirthHaloPacketCommand(
                    packet, FIFO_VERTEX16, 2u,
                    ((u32)(u16)projected_x) |
                        ((u32)(u16)projected_y << 16),
                    (u32)(s32)out_z, FALSE) == FALSE)
            {
                return FALSE;
            }
        }
    }
    if (ndsRendererRebirthHaloPacketFlush(packet) == FALSE)
    {
        return FALSE;
    }
    DC_FlushRange(packet->words, (u32)packet->word_count * sizeof(u32));
    packet->valid = 1u;
    gNdsRebirthHaloPackedBuildCount++;
    gNdsRebirthHaloPackedWordCount += packet->word_count;
    return TRUE;
}
#endif

static void ndsRendererRebirthHaloSubmitPackedGroup(u32 group_index)
{
    const NDSRebirthHaloPacket *packet = &sNdsRebirthHaloPackets[group_index];
#if NDS_R2_REBIRTH_HALO_PACKED_FIFO == 2
    while ((DMA_CR(0) & DMA_BUSY) != 0u) { }
    DMA_SRC(0) = (u32)packet->words;
    DMA_DEST(0) = (u32)&GFX_FIFO;
    DMA_CR(0) = DMA_FIFO | packet->word_count;
    while ((DMA_CR(0) & DMA_BUSY) != 0u) { }
#else
    u32 word;
    for (word = 0u; word < (u32)packet->word_count; word++)
    {
        GFX_FIFO = packet->words[word];
    }
#endif
    gNdsRebirthHaloPackedSubmitCount++;
}
#endif
#endif
#endif

s32 ndsRendererSubmitNativeRebirthHalo(
    u32 root_offset,
    const NDSRendererConfig *config,
    NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
#if NDS_RENDERER_HW_TRIANGLES && NDS_R2_REBIRTH_HALO_NATIVE
    NDSRendererTraversalVertexStorage vertex_storage;
    NDSRendererTraversalState state;
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
#if !NDS_R2_REBIRTH_HALO_SPLIT_MTX
    NDSRendererMatrix20p12 rebirth_raw_matrix;
#endif
#endif
    u32 first_group;
    u32 last_group;
    u32 group_index;
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
    u32 root_bounds_index;
#endif
#if NDS_R2_REBIRTH_HALO_PHASE_PROFILE
    u32 rebirth_phase_t0 = cpuGetTiming();
#endif

    if ((config == NULL) || (stats == NULL))
    {
        return FALSE;
    }
    if (root_offset == 0x2378u)
    {
        first_group = 0u;
        last_group = 4u;
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
        root_bounds_index = 0u;
#endif
    }
    else if (root_offset == 0x2a88u)
    {
        first_group = 4u;
        last_group = 5u;
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
        root_bounds_index = 1u;
#endif
    }
    else if (root_offset == 0x27e8u)
    {
        first_group = 5u;
        last_group = 6u;
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
        root_bounds_index = 2u;
#endif
    }
    else
    {
        return FALSE;
    }
    for (group_index = first_group; group_index < last_group; group_index++)
    {
        u32 slot = (group_index == 1u) ? NDS_REBIRTH_HALO_TEXTURE_E58 :
                   (group_index == 2u) ? NDS_REBIRTH_HALO_TEXTURE_DD0 :
                   (group_index == 3u) ? NDS_REBIRTH_HALO_TEXTURE_BC8 :
                   (group_index == 4u) ? NDS_REBIRTH_HALO_TEXTURE_BEAM :
                   (group_index == 5u) ? NDS_REBIRTH_HALO_TEXTURE_A40 :
                                         NDS_REBIRTH_HALO_TEXTURE_COUNT;
        if ((slot < NDS_REBIRTH_HALO_TEXTURE_COUNT) &&
            (sNdsRendererRebirthHaloTextureName[slot] == 0u))
        {
            return FALSE;
        }
    }

    ndsRendererInitTraversalState(
        &state, config, stats, &vertex_storage, NULL, 0u);
    if (state.matrix_valid == 0u)
    {
        return FALSE;
    }

    /* Preflight before the first GX side effect.  The accepted native path
     * tested every triangle corner here, then transformed every corner a
     * second time to draw it.  The full-offload path proves a conservative
     * immutable AABB instead: the near predicate is linear in clip space, so
     * all eight AABB corners inside implies every enclosed source vertex is
     * inside.  Anything touching the danger band still falls back before GX. */
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
    if (ndsRendererRebirthHaloBoundsInsideNearPlane(
            &state.matrix, &sNdsRebirthHaloRootBounds[root_bounds_index]) == FALSE)
    {
        return FALSE;
    }
#else
    for (group_index = first_group; group_index < last_group; group_index++)
    {
        const NDSRebirthHaloGroup *group = &sNdsRebirthHaloGroups[group_index];
        u32 corner_count = (u32)group->triangle_count * 3u;
        u32 corner;

        for (corner = 0u; corner < corner_count; corner++)
        {
            NDSRendererClipVertex20p12 clip;
            const NDSRendererInputVertex *vtx =
                &sNdsRebirthHaloVertices[(u32)group->first_vertex + corner];
            ndsRendererTransformVertex20p12(&state.matrix, vtx, &clip);
            if (ndsRendererHardwareClipZWInsideNearPlane(clip.z, clip.w) == FALSE)
            {
                return FALSE;
            }
        }
    }
#endif

#if NDS_R2_REBIRTH_HALO_PHASE_PROFILE
    NDS_REBIRTH_PHASE_MARK(0u, rebirth_phase_t0);
    rebirth_phase_t0 = cpuGetTiming();
#endif

    ndsRendererHardwareEndBatch();
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
    /* Two lab routes are deliberately kept side by side.  The composed route
     * preserves the accepted owner's CPU multiply and synthetic no-Z contract.
     * SPLIT_MTX retries the earlier experiment whose screenshot was externally
     * contaminated: the DS receives source projection and modelview separately
     * and owns their multiply, projection, divide, clipping, and natural Z. */
#if NDS_R2_REBIRTH_HALO_SPLIT_MTX
    if ((state.projection_valid == 0u) || (state.modelview_valid == 0u))
    {
        return FALSE;
    }
    ndsRendererLoadHardwareSplitMatrices(
        &state.projection, &state.modelview, state.matrix_generation);
#else
    ndsRendererLoadHardwareRawComposedMatrix(
        &state.matrix, state.matrix_generation);
    ndsRendererBuildRawHardwareMatrix(&state.matrix, &rebirth_raw_matrix);
#endif
    gNdsRebirthHaloFullOffloadRootCount++;
#endif
#if NDS_R2_REBIRTH_HALO_PHASE_PROFILE
    NDS_REBIRTH_PHASE_MARK(1u, rebirth_phase_t0);
#endif
    for (group_index = first_group; group_index < last_group; group_index++)
    {
        const NDSRebirthHaloGroup *group = &sNdsRebirthHaloGroups[group_index];
        const NDSRendererHardwareLightDirection *prepared_direction = NULL;
        NDSRendererHardwareLightDirection direction;
        u32 use_texture;
        u32 triangle;
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
        u32 use_gx_group =
            ((NDS_R2_REBIRTH_HALO_GX_GROUP_MASK & (1u << group_index)) != 0u) ?
                TRUE : FALSE;
#endif
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD && \
    (NDS_R2_REBIRTH_HALO_HW_LIGHT || NDS_R2_REBIRTH_HALO_PACKED_FIFO)
        u32 use_hw_light = FALSE;
#endif

        /* Seed slot 0 before deriving the group's alpha/material preparation. */
        state.input_vertices[0] =
            sNdsRebirthHaloVertices[(u32)group->first_vertex];
#if NDS_R2_REBIRTH_HALO_PHASE_PROFILE
        rebirth_phase_t0 = cpuGetTiming();
#endif
        if (ndsRendererRebirthHaloPrepareGroup(
                group_index, stats, &state, &use_texture) == FALSE)
        {
            return FALSE;
        }
#if NDS_R2_REBIRTH_HALO_PHASE_PROFILE
        NDS_REBIRTH_PHASE_MARK(2u, rebirth_phase_t0);
        rebirth_phase_t0 = cpuGetTiming();
#endif
        if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
            ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
        {
            ndsRendererHardwarePrepareLitDirection(
                stats,
                (state.modelview_valid != 0u) ? &state.modelview : NULL,
                &direction);
            prepared_direction = &direction;
        }
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
        if (use_gx_group == FALSE)
        {
            ndsRendererLoadHardwareMatrices(NULL, FALSE);
        }
#else
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
#endif
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD && NDS_R2_REBIRTH_HALO_HW_LIGHT
        {
            use_hw_light = ((group_index <= 1u) &&
                            (prepared_direction != NULL)) ? TRUE : FALSE;
            if (use_hw_light != FALSE)
            {
                s32 lx = ndsRendererRebirthHaloNormalComponent(-prepared_direction->x);
                s32 ly = ndsRendererRebirthHaloNormalComponent(-prepared_direction->y);
                s32 lz = ndsRendererRebirthHaloNormalComponent(-prepared_direction->z);
                u32 use_material =
                    (state.texture_prepare_vertex_flags &
                     NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL) != 0u;
                u32 diffuse = ndsRendererRebirthHaloMaterialColor15(
                    stats->light_color_1, state.texture_prepare_material_color,
                    use_material, state.color_modulate);
                u32 ambient = ndsRendererRebirthHaloMaterialColor15(
                    stats->light_color_2, state.texture_prepare_material_color,
                    use_material, state.color_modulate);

                /* prepared_direction is already transformed/normalised exactly
                 * like the accepted software shade.  GFX_LIGHT_VECTOR itself is
                 * transformed by the current vector matrix when written, so
                 * bracket the write with identity and restore the split
                 * modelview before any NORMAL command is submitted. */
                ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadIdentity();
                GFX_LIGHT_VECTOR = NDS_REBIRTH_HALO_NORMAL_PACK(lx, ly, lz);
                glPopMatrix(1);
                ndsRendererHardwareWriteDiffuseAmbient(
                    diffuse | (ambient << 16));
                state.texture_prepare_poly_fmt |= POLY_FORMAT_LIGHT0;
            }
        }
#endif
#if NDS_R2_REBIRTH_HALO_PHASE_PROFILE
        NDS_REBIRTH_PHASE_MARK(3u, rebirth_phase_t0);
        rebirth_phase_t0 = cpuGetTiming();
#endif
        ndsRendererHardwareBeginTriangleBatch(
            stats, use_texture, state.texture_prepare_name,
            state.texture_prepare_poly_fmt,
            sNdsRendererHardwareMatrixMode,
            sNdsRendererHardwareMatrixGeneration);
#if NDS_R2_REBIRTH_HALO_PHASE_PROFILE
        NDS_REBIRTH_PHASE_MARK(4u, rebirth_phase_t0);
        rebirth_phase_t0 = cpuGetTiming();
#endif

#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD && NDS_R2_REBIRTH_HALO_PACKED_FIFO
        /* Packet topology/geometry/texcoords are immutable. Software-lit groups
         * record the COLOR parameter offsets once, then patch only those live
         * words before DMA; hardware-lit packets need no per-frame patching. */
        if ((use_gx_group != FALSE) &&
            ndsRendererRebirthHaloBuildPackedGroup(
                group_index, group, stats, &state, prepared_direction,
                use_texture, use_hw_light) != FALSE &&
            ndsRendererRebirthHaloPatchPackedColors(
                group_index, group, stats, &state, prepared_direction) != FALSE)
        {
#if NDS_R2_REBIRTH_HALO_PHASE_PROFILE
            NDS_REBIRTH_PHASE_MARK(5u, rebirth_phase_t0);
            rebirth_phase_t0 = cpuGetTiming();
#endif
            ndsRendererRebirthHaloSubmitPackedGroup(group_index);
#if NDS_R2_REBIRTH_HALO_PHASE_PROFILE
            NDS_REBIRTH_PHASE_MARK(6u, rebirth_phase_t0);
            rebirth_phase_t0 = cpuGetTiming();
#endif
            sNdsRendererHardwareSubmitted = TRUE;
            stats->triangle_count += group->triangle_count;
            stats->transformed_triangle_count += group->triangle_count;
            stats->hardware_triangle_count += group->triangle_count;
            stats->hardware_vertex_count += (u32)group->triangle_count * 3u;
            ndsRendererHardwareEndBatch();
#if NDS_R2_REBIRTH_HALO_PHASE_PROFILE
            NDS_REBIRTH_PHASE_MARK(7u, rebirth_phase_t0);
#endif
            continue;
        }
#if NDS_R2_REBIRTH_HALO_PACKED_PROJECTED
        if ((use_gx_group == FALSE) &&
            ndsRendererRebirthHaloBuildPackedProjectedGroup(
                group_index, group, stats, &state, prepared_direction,
                use_texture) != FALSE)
        {
            u32 profile_triangle;
            ndsRendererRebirthHaloSubmitPackedGroup(group_index);
            sNdsRendererHardwareSubmitted = TRUE;
            stats->triangle_count += group->triangle_count;
            stats->transformed_triangle_count += group->triangle_count;
            stats->hardware_triangle_count += group->triangle_count;
            stats->hardware_vertex_count += (u32)group->triangle_count * 3u;
            stats->hardware_projected_depth_triangle_count +=
                group->triangle_count;
            for (profile_triangle = 0u;
                 profile_triangle < (u32)group->triangle_count;
                 profile_triangle++)
            {
                ndsRendererProfileRecordProjectedSubmit();
                ndsRendererProfileRecordHardwareTriangle();
            }
            ndsRendererHardwareEndBatch();
            continue;
        }
#endif
#endif

        for (triangle = 0u; triangle < (u32)group->triangle_count; triangle++)
        {
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
            if (use_gx_group != FALSE)
            {
                u32 context_flags = state.texture_prepare_vertex_flags;
                u32 corner;
#if NDS_R2_REBIRTH_HALO_SPLIT_MTX
#if NDS_R2_REBIRTH_HALO_SPLIT_NOZ
                s16 depth = (s16)ndsRendererHardwareNextProjectedDepth();

                ndsRendererRebirthHaloLoadSplitNoZProjection(
                    &state.projection, depth);
#endif
#else
                s16 depth = (s16)ndsRendererHardwareNextProjectedDepth();

                ndsRendererRebirthHaloLoadNoZMatrix(&rebirth_raw_matrix, depth);
#endif

                /* Every source corner is already immutable/AOT and the group
                 * state above is constant for the whole run.  Bypass the
                 * generic three-slot cache/mask machinery completely. */
                for (corner = 0u; corner < 3u; corner++)
                {
                    u32 source_index =
                        (u32)group->first_vertex + triangle * 3u + corner;
                    const NDSRendererInputVertex *vtx =
                        &sNdsRebirthHaloVertices[source_index];
#if NDS_R2_REBIRTH_HALO_HW_LIGHT
                    if (use_hw_light != FALSE)
                    {
                        glNormal(NDS_REBIRTH_HALO_NORMAL_PACK(
                            ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->r),
                            ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->g),
                            ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->b)));
                    }
                    else
                    {
#endif
                    u32 shade = ndsRendererHardwareLitShadeColorPrepared(
                        stats, vtx, prepared_direction);
                    u16 packed_color = ndsRendererHardwarePackedVertexColor(
                        stats, vtx, state.texture_prepare_material_color,
                        (s32)(context_flags &
                              NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL),
                        (s32)(context_flags &
                              NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX),
                        shade, TRUE, state.color_modulate);

                    glColor(packed_color);
#if NDS_R2_REBIRTH_HALO_HW_LIGHT
                    }
#endif
                    if (use_texture != FALSE)
                    {
                        glTexCoord2t16(
                            ndsRendererHardwareTexCoord(
                                vtx->s, state.texture_prepare_scale_s,
                                state.texture_prepare_origin_s,
                                state.texture_prepare_offset),
                            ndsRendererHardwareTexCoord(
                                vtx->t, state.texture_prepare_scale_t,
                                state.texture_prepare_origin_t,
                                state.texture_prepare_offset));
                    }
                    glVertex3v16(
                        ndsRendererHardwareVertexCoord(vtx->x, TRUE),
                        ndsRendererHardwareVertexCoord(vtx->y, TRUE),
                        ndsRendererHardwareVertexCoord(vtx->z, TRUE));
                }
            }
            else
#endif
            {
            v16 projected_x[3];
            v16 projected_y[3];
            u16 packed_color[3];
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD && NDS_R2_REBIRTH_HALO_HW_LIGHT
            u32 packed_normal[3];
#endif
            t16 prepared_s[3];
            t16 prepared_t[3];
            u32 context_flags = state.texture_prepare_vertex_flags;
            u32 corner;
            s32 depth = ndsRendererHardwareNextProjectedDepth();

            for (corner = 0u; corner < 3u; corner++)
            {
                u32 source_index = (u32)group->first_vertex + triangle * 3u + corner;
                const NDSRendererInputVertex *vtx =
                    &sNdsRebirthHaloVertices[source_index];
                NDSRendererClipVertex20p12 clip;
                u32 shade = ndsRendererHardwareLitShadeColorPrepared(
                    stats, vtx, prepared_direction);

                packed_color[corner] = ndsRendererHardwarePackedVertexColor(
                    stats, vtx, state.texture_prepare_material_color,
                    (s32)(context_flags &
                          NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL),
                    (s32)(context_flags &
                          NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX),
                    shade, TRUE, state.color_modulate);
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD && NDS_R2_REBIRTH_HALO_HW_LIGHT
                if (use_hw_light != FALSE)
                {
                    packed_normal[corner] = NDS_REBIRTH_HALO_NORMAL_PACK(
                        ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->r),
                        ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->g),
                        ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->b));
                }
#endif
                if (use_texture != FALSE)
                {
                    prepared_s[corner] = ndsRendererHardwareTexCoord(
                        vtx->s, state.texture_prepare_scale_s,
                        state.texture_prepare_origin_s,
                        state.texture_prepare_offset);
                    prepared_t[corner] = ndsRendererHardwareTexCoord(
                        vtx->t, state.texture_prepare_scale_t,
                        state.texture_prepare_origin_t,
                        state.texture_prepare_offset);
                }
                ndsRendererTransformVertex20p12(&state.matrix, vtx, &clip);
                projected_x[corner] = ndsRendererHardwareProjectToV16(
                    (s64)clip.x * NDS_RENDERER_HW_PROJECTED_VERTEX, clip.w);
                projected_y[corner] = ndsRendererHardwareProjectToV16(
                    (s64)clip.y * NDS_RENDERER_HW_PROJECTED_VERTEX, clip.w);
                stats->matrix_transform_count++;
                stats->transformed_vertex_count++;
                ndsRendererProfileRecordCPUTransform();
                ndsRendererProfileRecordSourceVertexLoad();
            }
            for (corner = 0u; corner < 3u; corner++)
            {
                v16 out_z = ndsRendererHardwareClampS64ToV16(depth);

#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD && NDS_R2_REBIRTH_HALO_HW_LIGHT
                if (use_hw_light != FALSE)
                {
                    glNormal(packed_normal[corner]);
                }
                else
#endif
                {
                    glColor(packed_color[corner]);
                }
                if (use_texture != FALSE)
                {
                    glTexCoord2t16(prepared_s[corner], prepared_t[corner]);
                }
                ndsRendererProfileHWVertexRange(
                    projected_x[corner], projected_y[corner], out_z);
                glVertex3v16(projected_x[corner], projected_y[corner], out_z);
            }
            }
            sNdsRendererHardwareSubmitted = TRUE;
            stats->triangle_count++;
            stats->transformed_triangle_count++;
            stats->hardware_triangle_count++;
            stats->hardware_vertex_count += 3u;
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
            if (use_gx_group == FALSE)
            {
                stats->hardware_projected_depth_triangle_count++;
                ndsRendererProfileRecordProjectedSubmit();
            }
#else
            stats->hardware_projected_depth_triangle_count++;
            ndsRendererProfileRecordProjectedSubmit();
#endif
            ndsRendererProfileRecordHardwareTriangle();
        }
        ndsRendererHardwareEndBatch();
    }
    ndsRendererRebirthHaloFinishRoot(root_offset, stats, &state);
    stats->end_command_count++;
    return TRUE;
#else
    (void)root_offset;
    (void)config;
    (void)stats;
    return FALSE;
#endif
}
#undef NDS_REBIRTH_PHASE_MARK

static s32 ndsRendererNativeVisitSourceCommand(
    const u8 *root_base,
    u32 command_index,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const Gfx *dl;
    NDSRendererCommand command;

    if (callback == NULL)
    {
        return TRUE;
    }
    dl = (const Gfx *)(root_base + (command_index * sizeof(*dl)));
    memset(&command, 0, sizeof(command));
    command.dl = dl;
    command.w0 = dl->words.w0;
    command.w1 = dl->words.w1;
    command.op = command.w0 >> 24;
    command.list_index = command_index;
    command.transformed_vertices = state->vertices;
    command.transformed_vertex_valid_mask = state->vertex_valid_mask;
    command.matrix_valid = state->matrix_valid;
    if (callback(&command, callback_user) == FALSE)
    {
        ndsRendererRecordUnsupported(stats, command.op);
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return FALSE;
    }
    return TRUE;
}

static void ndsRendererNativeLoadVertexBlock(
    const u8 *src,
    u32 v0,
    u32 count,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const NDSRendererHardwareLightDirection *prepared_light_direction = NULL;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    const u32 *prepared_light_shade_lut = NULL;
#endif
    u32 matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    u32 i;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    stats->source_vertex_count += count;
#endif
    if ((v0 + count) > stats->vertex_count)
    {
        stats->vertex_count = v0 + count;
    }
    if (state->matrix_valid != 0u)
    {
        matrix_snapshot = ndsRendererAcquireCurrentMatrixSnapshot(state);
    }
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        if (state->prepared_light_direction_valid == 0u)
        {
            ndsRendererHardwarePrepareLitDirection(
                stats,
                (state->modelview_valid != 0u) ? &state->modelview : NULL,
                &state->prepared_light_direction);
            state->prepared_light_direction_valid = TRUE;
        }
        prepared_light_direction = &state->prepared_light_direction;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((stats->light_color_mask &
             (NDS_RENDERER_LIGHT_COLOR_1_MASK |
              NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
            (NDS_RENDERER_LIGHT_COLOR_1_MASK |
             NDS_RENDERER_LIGHT_COLOR_2_MASK))
        {
            prepared_light_shade_lut = ndsRendererHardwareGetLightShadeLut(
                stats->light_color_1, stats->light_color_2);
        }
#endif
    }
    for (i = 0u; i < count; i++)
    {
        u32 index = v0 + i;
        u32 mask = 1u << index;
        NDSRendererInputVertex *input = &state->input_vertices[index];

        ndsRendererDecodeInputVertex(input, src + (i * 16u));
        state->input_vertex_valid_mask |= mask;
        if (ndsRendererHardwareRawVertexFits(input) != FALSE)
        {
            state->raw_vertex_fit_mask |= mask;
        }
        else
        {
            state->raw_vertex_fit_mask &= ~mask;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (prepared_light_shade_lut != NULL)
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorLut(
                    input, prepared_light_direction,
                    prepared_light_shade_lut);
        }
        else
#endif
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorPrepared(
                    stats, input, prepared_light_direction);
        }
        state->vertex_color_valid_mask |= mask;
        state->vertex_matrix_snapshot[index] = (u8)matrix_snapshot;
        state->vertex_clip_snapshot[index] =
            NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
        state->vertex_valid_mask &= ~mask;
        state->current_transform_vertex_mask &= ~mask;
        if (state->matrix_valid != 0u)
        {
            state->current_transform_vertex_mask |= mask;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (state->matrix_valid != 0u)
        {
            (void)ndsRendererTransformCachedVertex(
                stats, state, index, &state->matrix, matrix_snapshot);
        }
#else
        if ((state->matrix_valid != 0u) &&
            (matrix_snapshot == NDS_RENDERER_MATRIX_SNAPSHOT_INVALID))
        {
            (void)ndsRendererTransformCachedVertex(
                stats, state, index, &state->matrix, matrix_snapshot);
        }
#endif
    }
    sNdsRendererHardwareSourceVertexLoadCount += count;
}

static void ndsRendererNativeApplyVertexActions(
    const NDSNativeEpoch *epoch,
    const u8 *asset_base,
    const u8 *root_base,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 i;

    if (epoch->action_count == 0u)
    {
        return;
    }
    ndsRendererNativeSourceBoundary(state);
    for (i = 0u; i < epoch->action_count; i++)
    {
        const NDSNativeVertexAction *action =
            &sNdsNativeFighterVertexActions[epoch->first_action + i];

        if (ndsRendererNativeVisitSourceCommand(
                root_base, action->command_index, callback, callback_user,
                stats, state) == FALSE)
        {
            return;
        }
        if (action->kind == NDS_NATIVE_VERTEX_BLOCK)
        {
            ndsRendererNativeLoadVertexBlock(
                asset_base + action->source_offset,
                action->index, action->count, stats, state);
        }
        else if ((action->kind == NDS_NATIVE_MODIFY_ST) &&
                 (action->index < NDS_RENDERER_MAX_VTX) &&
                 ((state->input_vertex_valid_mask &
                   (1u << action->index)) != 0u))
        {
            state->input_vertices[action->index].s = action->s;
            state->input_vertices[action->index].t = action->t;
        }
    }
}

static void ndsRendererNativeSubmitGenericTriangle(
    u32 packed,
    u32 command_index,
    u32 tri2_half,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->semantic_command_index = command_index;
    state->semantic_tri2_half = tri2_half;
#else
    (void)command_index;
    (void)tri2_half;
#endif
    if (sNdsRendererHardwareNoOracle == 0u)
    {
        ndsRendererRecordTransformedTriangle(
            stats, state, packed);
    }
    NDS_EFFECT_PHASE_TRI(ndsRendererSubmitHardwareTriangle(
        stats, config, state, packed));
}

static inline u32 ndsRendererNativeDecodeTriangle(
    u16 encoded,
    u32 indices[3])
{
    u32 compact = (u32)encoded & 0x7fffu;

    indices[0] = (compact >> 10) & 31u;
    indices[1] = (compact >> 5) & 31u;
    indices[2] = compact & 31u;
    return (indices[0] << 17) |
           (indices[1] << 9) |
           (indices[2] << 1);
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static u32 ndsRendererNativeNormalizeGeometryMode(u32 mode)
{
    u32 source_cull = mode &
        (NDS_NATIVE_SOURCE_GEOM_CULL_FRONT |
         NDS_NATIVE_SOURCE_GEOM_CULL_BACK);

    if ((source_cull & NDS_NATIVE_SOURCE_GEOM_CULL_FRONT) != 0u)
    {
        mode |= NDS_RENDERER_GEOM_CULL_FRONT;
    }
    if ((source_cull & NDS_NATIVE_SOURCE_GEOM_CULL_BACK) != 0u)
    {
        mode |= NDS_RENDERER_GEOM_CULL_BACK;
    }
    return mode &
        ~(NDS_NATIVE_SOURCE_GEOM_CULL_FRONT |
          NDS_NATIVE_SOURCE_GEOM_CULL_BACK);
}

static s32 ndsRendererNativeDirectReject(NDSRendererStats *stats)
{
    if (stats != NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
    }
    return FALSE;
}

#if NDS_LAB_NO_CULL
/* BUGS.md #10 / P2-3r17 seam probe. One binary, four arms, cycled by SELECT --
 * the only DS key the battle input map leaves unbound -- so a single
 * capture-melonds.ps1 session photographs the CONTROL and a candidate arm at
 * the same camera, from the same ROM, with the arm printed on the HUD.
 *
 * The two failures this shape exists to prevent have both actually happened on
 * this row. 2026-07-27's "culling REFUTED" was worthless because the probe only
 * patched ndsRendererNativeBeginHierarchyBatch, which the production owner
 * (mode 8/9) never calls: it looked exactly like a probe that had run and
 * found nothing. And comparing two separate builds/captures put a pose and a
 * camera between the arms. Arm 2 INVERTS the cull, so a probe that is not
 * reaching the geometry cannot be mistaken for one that is -- the fighter must
 * render visibly inside-out.
 *
 *   0  shipped
 *   1  POLY_CULL_NONE   -- both faces drawn. Splits "the geometry never
 *                          reached the GX" from "the GX culled it", which no
 *                          counter can tell apart.
 *   2  POLY_CULL_FRONT  -- inverted; the arm that proves the probe fires.
 *   3  strips off       -- the Task 56 primitive-group emitter is bypassed for
 *                          the raw corner emitters. Only exists when
 *                          NDS_R2_STRIP_ROUTE compiled both emitters in.
 *
 * AND A CULL ARM IS NOT AN ORACLE FOR MISSING GEOMETRY. Drawing both faces
 * fills a hole's COLOUR in without closing the hole, so an arm judged on "does
 * it look better" reports a fix that is not there -- which is how P2-3r17 read
 * a fix into arm 1 before the owner falsified the whole cull family in one
 * sentence. Judge only on whether the seam is GONE, and only against a
 * frame-locked control.
 *
 * Fighters only: un-culling the stage as well blows past the polygon limits and
 * hangs the ROM. */
#if NDS_FIGHTER_PACKET_LIVE
/* THE TRAP THIS PROBE EXISTS TO AVOID, AND IT CAUGHT ME TOO (2026-08-25).
 * With NDS_R2_FIGHTER_PACKET the shipped fighter draw is a DMA replay of a
 * recorded GX stream: ndsFighterPacketTryReplay returns before the production
 * execute, so nothing below runs on a hit, and the recorder tees
 * `state->texture_prepare_poly_fmt` -- the UNPATCHED value -- into the packet
 * before the begin-batch site the probe patches. Arms 1 and 2 then produce
 * BYTE-IDENTICAL frames, which reads exactly like "culling makes no
 * difference". Build the probe with NDS_R2_FIGHTER_PACKET=0. */
#error "NDS_LAB_NO_CULL needs NDS_R2_FIGHTER_PACKET=0; a packet replay ignores every arm"
#endif
#if NDS_R2_STRIP_ROUTE && (NDS_TASK56_FIGHTER_PRIMITIVES >= 1) && \
    (NDS_RENDERER_PROFILE_LEVEL < 2) && NDS_RENDERER_HW_TRIANGLES
#define NDS_LAB_SEAM_ARM_COUNT 5u
#define NDS_LAB_SEAM_STRIP_ARM 3u
#define NDS_LAB_SEAM_SOURCE_WORLD_ARM 4u
#else
#define NDS_LAB_SEAM_ARM_COUNT 4u
#define NDS_LAB_SEAM_STRIP_ARM 0xffu
#define NDS_LAB_SEAM_SOURCE_WORLD_ARM 3u
#endif
/* DTCM, not cached main RAM. CSS preview rendering can read this before the
 * battle-start GDB selector writes it; an aligned private main-RAM cache line
 * still retains that old zero behind melonDS's physical-memory write. Keeping
 * the lab control in DTCM makes both SELECT and exact-frame GDB selection
 * coherent without a production cache flush. */
volatile u32 gNdsLabSeamArm
    __attribute__((section(".dtcm.bss"), aligned(32)));

static inline u32 ndsRendererNativeLabSeamPolyFmt(u32 poly_fmt)
{
    u32 arm = gNdsLabSeamArm;

    if (arm == 2u)
    {
        return (poly_fmt & ~(u32)POLY_CULL_NONE) | (u32)POLY_CULL_FRONT;
    }
    if (arm == 1u)
    {
        return (poly_fmt & ~(u32)POLY_CULL_NONE) | (u32)POLY_CULL_NONE;
    }
    return poly_fmt;
}
#endif

/* Captain is the first production fighter whose source roots use
 * G_AC_THRESHOLD. Keep the DS register writes out of the packed ITCM submit
 * body: they occur only when a batch begins, not per emitted vertex, and the
 * hot region has no spare capacity for libnds' inlined glEnable/glAlphaFunc
 * sequence. Semantics are the same mapping used by the generic renderer. */
static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererNativeApplyProductionAlphaTest(const NDSRendererStats *stats)
{
    if ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) ==
        NDS_RENDERER_ALPHA_COMPARE_THRESHOLD)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc((stats->blend_color & 0xffu) >> 4);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }
}

static inline void ndsRendererNativeBeginDirectBatch(
    const NDSRendererStats *stats,
    u32 textured,
    u32 texture_name,
    u32 poly_fmt,
    u32 matrix_generation)
{
    u32 alpha_key =
        (stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) |
        ((stats->blend_color & 0xffu) << 8);
#if NDS_LAB_NO_CULL
    /* THE production owner's batch. Mode 8 and mode 9 both come here and never
     * reach the hierarchy batch, so this is the site the probe has to patch. */
    poly_fmt = ndsRendererNativeLabSeamPolyFmt(poly_fmt);
#endif
    if ((sNdsRendererHardwareTriangleBatchOpen != 0u) &&
        (sNdsRendererHardwareTriangleBatchTextured == textured) &&
        (sNdsRendererHardwareTriangleBatchTextureName == texture_name) &&
        (sNdsRendererHardwareTriangleBatchPolyFmt == poly_fmt) &&
        (sNdsRendererHardwareTriangleBatchAlphaKey == alpha_key) &&
        (sNdsRendererHardwareTriangleBatchMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
        (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
         matrix_generation))
    {
        ndsRendererProfileRecordBatchReuse();
        return;
    }

    ndsRendererHardwareEndBatch();
    glEnable(GL_TEXTURE_2D);
    if (textured == 0u)
    {
        ndsRendererHardwareBindNoTexture(NULL);
    }
    /* BattleShip's Captain model contains real G_AC_THRESHOLD epochs. The
     * generic DS renderer maps that source state to the hardware alpha test,
     * with G_SETBLENDCOLOR's low byte as the threshold. Production used to
     * reject the epoch and fall back after GX had already started. Keep the
     * same mapping here so the source run stays on the native owner. */
    ndsRendererNativeApplyProductionAlphaTest(stats);
    glDisable(GL_FOG);
    ndsRendererHardwareSetPolyFmt(poly_fmt);
    glBegin(GL_TRIANGLE);
    ndsRendererProfileRecordBatchBegin();

    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = textured;
    sNdsRendererHardwareTriangleBatchTextureName = texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = alpha_key;
    sNdsRendererHardwareTriangleBatchFogKey = 0u;
    sNdsRendererHardwareTriangleBatchMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED;
    sNdsRendererHardwareTriangleBatchMatrixGeneration = matrix_generation;
}

/* GX-compose's owner-approved shipping arm once re-admitted this 152-byte
 * root-level helper after the 2026-08-17 re-knapsack. P2-2's source-required
 * High/Low owner selection needs a little of that space back. This is the
 * lowest-value retained admission with a recorded price (1,123 I-cache-fill
 * tk/fr at rank-80, 7.4/byte), and moving it changes placement only: the exact
 * same helper still runs once per root from main RAM. Keep the per-run prepare,
 * shade and emit loops in zero-wait ITCM instead. */
static void ndsRendererNativeApplyProductionPreamble(
    const NDSRendererNativeFighterPreamble *preamble,
    NDSRendererStats *stats)
{
    if ((preamble == NULL) || (stats == NULL) ||
        ((preamble->flags & NDS_RENDERER_NATIVE_PREAMBLE_VALID) == 0u))
    {
        return;
    }

    stats->geometry_mode = ndsRendererNativeNormalizeGeometryMode(
        preamble->geometry_mode);
    stats->othermode_h =
        (stats->othermode_h & ~NDS_RENDERER_CYCLETYPE_MASK) |
        (preamble->cycle_type & NDS_RENDERER_CYCLETYPE_MASK);
    stats->othermode_l = preamble->render_mode;
    stats->prim_color = preamble->prim_color;
    stats->env_color = preamble->env_color;

    if ((preamble->flags &
         NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID) != 0u)
    {
        stats->light_dir_x = preamble->light_dir_x;
        stats->light_dir_y = preamble->light_dir_y;
        stats->light_dir_z = preamble->light_dir_z;
        stats->light_dir_mask = NDS_RENDERER_LIGHT_DIR_1_MASK;
    }
}

/* Hoisted from the shade block below so BindProductionRoot can test it: with the
 * skip active the fighter path never prepares a software light direction, which
 * is what removes the last reader of the traversal state's own modelview. */
#if NDS_R2_FIGHTER_HW_LIGHT && !NDS_RENDERER_M2_DETAILED_LEDGER && \
    !NDS_R2_FIGHTER_SOFT_LIGHT_KEEP
#define NDS_R2_SHADE_SKIP_SOFT_LIGHT 1
#else
#define NDS_R2_SHADE_SKIP_SOFT_LIGHT 0
#endif

#if NDS_R2_FIGHTER_GX_COMPOSE && !NDS_R2_SHADE_SKIP_SOFT_LIGHT
/* Slice 43's other half of the assertion at the GX compose loader. Without the
 * skip, BindProductionRoot below copies `input->modelview_matrix` into the
 * traversal state for the software light -- and under GX compose that pointer is
 * the seed, not this root's world. */
#error "NDS_R2_FIGHTER_GX_COMPOSE requires NDS_R2_SHADE_SKIP_SOFT_LIGHT"
#endif

static void ndsRendererNativeBindProductionRoot(
    NDSRendererTraversalState *state,
    const NDSRendererNativeFighterRoot *input,
    NDSRendererStats *stats)
{
    ndsRendererNativeApplyProductionPreamble(input->preamble, stats);
    state->modelview_stack_depth = 0u;
    state->vertex_valid_mask = 0u;
    state->input_vertex_valid_mask = 0u;
    state->vertex_color_valid_mask = 0u;
    state->current_transform_vertex_mask = 0u;
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_projected_xy_valid_mask = 0u;
    state->prepared_projected_source_z_valid_mask = 0u;
    state->prepared_light_direction_valid = 0u;
    state->texture_prepare_valid = 0u;
    state->projection_valid = 0u;
#if NDS_R2_FIGHTER_HW_MTX && NDS_R2_SHADE_SKIP_SOFT_LIGHT
    /* E16b traced every consumer of the composed matrix and found only the
     * hardware load, which the split loader replaces. Finishing that trace for
     * the two halves: on this path the split loader takes them from `input`
     * directly (its call site is the next statement after this one, with
     * `input` in scope), and the only other readers of the traversal state's
     * modelview are the software light preparation -- compiled out whenever
     * NDS_R2_SHADE_SKIP_SOFT_LIGHT -- plus the generic command executor's
     * matrix stack and ComposeMatrix, none of which the production owner
     * enters. So these were two 64-byte struct copies per root with nothing
     * downstream to read them: 12,422 executions at 416 cycles a call, 5,958
     * ticks/frame, 79% of BindProductionRoot's whole cost.
     *
     * The valid flags go FALSE rather than TRUE because that is now the honest
     * answer: a reader that appears later gets "no matrix here" instead of
     * whichever owner's matrix was copied in last. `matrix_valid` below is a
     * different flag and is still what PrepareProductionRun tests. */
    /* Ordinary GX-compose owners leave the finished modelview only on GX. A
     * declined owner has the adapter's exact CPU fallback, while Link carries
     * the same compose as an explicit CPU mirror solely because source
     * G_TEXTURE_GEN consumes the finished current modelview. */
#if NDS_R2_FIGHTER_GX_COMPOSE
    if ((input->gx_valid == 0u) ||
        (input->gx_modelview_mirror_valid != 0u))
    {
        state->modelview = *input->modelview_matrix;
        state->modelview_valid = TRUE;
    }
    else
    {
        state->modelview_valid = FALSE;
    }
#else
    state->modelview_valid = FALSE;
#endif
    state->projection_valid = FALSE;
#elif NDS_R2_FIGHTER_HW_MTX
    state->modelview = *input->modelview_matrix;
    state->modelview_valid = TRUE;
    state->projection = *input->projection_matrix;
    state->projection_valid = TRUE;
#else
    state->modelview = *input->modelview_matrix;
    state->modelview_valid = TRUE;
    state->matrix = *input->composed_matrix;
#endif
    state->matrix_valid = TRUE;
    state->matrix_word_valid = FALSE;
    state->matrix_generation = ndsRendererNextMatrixGeneration();
    stats->hardware_matrix_seed_count++;
}

/* One execute owns this renderer state at a time.  Resolve BattleShip's live
 * model-part DL selection in the cold preflight, then let the ITCM executor
 * consume a pointer table instead of repeating owner/variant dispatch inside
 * every root iteration.  The preflight is already the fail-closed boundary:
 * the executor is entered only after all entries below are non-NULL and their
 * epochs/materials validate. */
static const NDSNativeRoot *
    sNdsNativeProductionResolvedRoots[NDS_NATIVE_FIGHTER_ROOT_MAX];

static s32 ndsRendererNativePreflightProductionOwner(
    u32 slot,
    u32 use_low_detail,
    const void *asset_base,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count,
    NDSRendererCommandCallback callback,
    NDSRendererStats *stats)
{
    u32 root_count;
    u32 root_index;
    if ((slot >= NDS_NATIVE_FIGHTER_OWNER_COUNT) ||
        (asset_base == NULL) || (inputs == NULL) ||
        (stats == NULL) || (callback != NULL) ||
        (sNdsNativeFighterOwnerExecution.active != 0u))
    {
        return FALSE;
    }
    root_count = sNdsNativeFighterActiveOwner->root_count;
    if ((input_count != root_count) ||
        (root_count > NDS_NATIVE_FIGHTER_ROOT_MAX))
    {
        return FALSE;
    }
    for (root_index = 0u; root_index < root_count; root_index++)
    {
        const NDSRendererNativeFighterRoot *input = &inputs[root_index];
        const NDSNativeRoot *root = ndsRendererNativeFighterResolveRoot(
            sNdsNativeFighterActiveOwner, slot, use_low_detail,
            root_index, input->root_offset);
        u32 epoch_index;

        if ((root == NULL) ||
#if NDS_R2_FIGHTER_HW_MTX
            (input->projection_matrix == NULL) ||
#else
            (input->composed_matrix == NULL) ||
#endif
            (input->modelview_matrix == NULL) ||
            (input->config == NULL) ||
            (input->preamble == NULL) ||
            ((input->preamble->flags &
              NDS_RENDERER_NATIVE_PREAMBLE_VALID) == 0u))
        {
            return FALSE;
        }
        sNdsNativeProductionResolvedRoots[root_index] = root;
#if NDS_RENDERER_M2_DETAILED_LEDGER
        if ((input->owner_generation == 0u) ||
            (input->owner_generation != inputs[0].owner_generation))
        {
            return FALSE;
        }
#endif
        for (epoch_index = 0u;
             epoch_index < root->epoch_count;
             epoch_index++)
        {
            const NDSNativeEpoch *epoch =
                &sNdsNativeFighterActiveTables->epochs[
                    root->first_epoch + epoch_index];

            if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
                ((input->materials == NULL) ||
                 (epoch->material_slot >= input->material_count)))
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

#if NDS_R2_FIGHTER_SHADE_PROOF
/* R2-03 E1 falsifier. ndsRendererNativeShadeProductionActions is 48,422
 * ticks/frame -- the largest non-idle, non-soft-float function in the frame
 * (R2-03 E0) -- and it re-lights every one of the 541 fighter dense vertices
 * every frame. Whether it needs to is a question about the data, and R2-02 E3
 * is the standing reminder to answer that with a counter rather than an
 * argument.
 *
 * Two hashes, because they imply different cuts. The INPUT hash covers
 * everything the shaded colour is a function of besides the constant tables:
 * the epoch policy, the material and modulate colours, the light masks and
 * colours, and the prepared light direction. The OUTPUT hash covers the
 * packed_color and shaded_rgba the loop actually writes.
 *
 * Input constant  -> the whole loop is a memo on a dozen words.
 * Input moves, output constant -> RGB15 quantisation is absorbing the motion,
 *   and the memo wants a quantised key.
 * Both move -> there is nothing to memoise and the lever is the per-vertex
 *   math instead. That is a real answer too, and it costs one build to get. */
volatile u32 gNdsR2ShadeInputHash;
volatile u32 gNdsR2ShadeInputChangeCount;
volatile u32 gNdsR2ShadeOutputHash;
volatile u32 gNdsR2ShadeOutputChangeCount;
volatile u32 gNdsR2ShadeCallCount;
volatile u32 gNdsR2ShadeFrameCount;
static u32 sNdsR2ShadeFrameInputHash = 2166136261u;
static u32 sNdsR2ShadeFrameCallCount;

#define NDS_R2_SHADE_HASH(hash, value) \
    ((hash) = (((hash) ^ (u32)(value)) * 16777619u))

static void ndsRendererR2FighterShadeProofFrame(void)
{
    u32 output = 2166136261u;
    u32 i;

    for (i = 0u; i < sNdsNativeFighterActiveTables->dense_count; i++)
    {
        /* R2-03 E29 removed both fields under NDS_R2_FIGHTER_HW_LIGHT. The
         * output hash covers what the per-vertex loop writes, and under that
         * flag the loop is compiled out, so the hash falls back to the vertex
         * words the emit does read. Only comparable within one build. */
#if NDS_R2_FIGHTER_HW_LIGHT
        NDS_R2_SHADE_HASH(
            output, sNdsNativeFighterActiveTables->prepared_dense[i].gx_xy);
        NDS_R2_SHADE_HASH(
            output, sNdsNativeFighterActiveTables->prepared_dense[i].gx_z);
#else
        NDS_R2_SHADE_HASH(
            output,
            sNdsNativeFighterActiveTables->prepared_dense[i].shaded_rgba);
        NDS_R2_SHADE_HASH(
            output,
            sNdsNativeFighterActiveTables->prepared_dense[i].packed_color);
#endif
    }
    if (gNdsR2ShadeFrameCount != 0u)
    {
        if (sNdsR2ShadeFrameInputHash != gNdsR2ShadeInputHash)
        {
            gNdsR2ShadeInputChangeCount++;
        }
        if (output != gNdsR2ShadeOutputHash)
        {
            gNdsR2ShadeOutputChangeCount++;
        }
    }
    gNdsR2ShadeInputHash = sNdsR2ShadeFrameInputHash;
    gNdsR2ShadeOutputHash = output;
    gNdsR2ShadeCallCount = sNdsR2ShadeFrameCallCount;
    gNdsR2ShadeFrameCount++;
    sNdsR2ShadeFrameInputHash = 2166136261u;
    sNdsR2ShadeFrameCallCount = 0u;
}
#endif

#if NDS_TASK91_DRAW_PHASE_CENSUS
/* R2-03 E16 premise check. The software shade computes
 * `ambient + diffuse * dot(normal, light_dir) / 127` per vertex, which is the
 * DS geometry engine's hardware lighting equation, on the CPU, while E14
 * measured that engine idle. Before designing anything around that, the premise
 * has to hold on the data: lighting must actually be ON for fighter epochs, and
 * the vertices must actually be going through the lit path rather than the
 * pass-through or the shared-colour copy. Counters, not timers, so this barely
 * perturbs. */
u32 gNdsR2ShadeLitEpochs;
u32 gNdsR2ShadeUnlitEpochs;
u32 gNdsR2ShadeVerticesLit;
u32 gNdsR2ShadeVerticesCopied;
u32 gNdsR2ShadeLutEpochs;
u32 gNdsR2ShadeMaterialEpochs;
/* R2-03 E16a. How often the light and material terms actually move. */
u32 gNdsR2LightEpochs;
u32 gNdsR2LightDirChanges;
u32 gNdsR2LightColorChanges;
u32 gNdsR2LightMaterialChanges;

/* Deliberately outside .itcm.native_fighter and noinline: the shade function is
 * ITCM-resident and full, and inlining this probe overflowed the region by 100
 * bytes. A census arm must not change what fits in ITCM, or it measures a
 * different build's cache behaviour than the one it is reasoning about.
 *
 * The question: GFX_LIGHT_VECTOR stores the vector transformed by the vector
 * matrix at write time, so it can only be written while that matrix is identity
 * -- once per root, before the root's modelview is loaded. That is legal only if
 * the light direction is constant across the epochs of a root. The colours
 * decide the same for GFX_LIGHT_COLOR, and the material terms decide whether
 * GFX_DIFFUSE_AMBIENT has to be written per epoch. */
static void __attribute__((noinline)) ndsRendererR2E16aLightCensus(
    const NDSRendererStats *stats,
    u32 material_color,
    u32 color_modulate)
{
    static s32 last_dir[3];
    static u32 last_color[2];
    static u32 last_material[2];
    static u8 last_valid;

    gNdsR2LightEpochs++;
    if (last_valid != 0u)
    {
        if ((stats->light_dir_x != last_dir[0]) ||
            (stats->light_dir_y != last_dir[1]) ||
            (stats->light_dir_z != last_dir[2]))
        {
            gNdsR2LightDirChanges++;
        }
        if ((stats->light_color_1 != last_color[0]) ||
            (stats->light_color_2 != last_color[1]))
        {
            gNdsR2LightColorChanges++;
        }
        if ((material_color != last_material[0]) ||
            (color_modulate != last_material[1]))
        {
            gNdsR2LightMaterialChanges++;
        }
    }
    last_dir[0] = stats->light_dir_x;
    last_dir[1] = stats->light_dir_y;
    last_dir[2] = stats->light_dir_z;
    last_color[0] = stats->light_color_1;
    last_color[1] = stats->light_color_2;
    last_material[0] = material_color;
    last_material[1] = color_modulate;
    last_valid = 1u;
}
#endif

#if NDS_R2_FIGHTER_HW_LIGHT
/* R2-03 E16. The fighter's per-vertex shade evaluates
 *
 *     colour = light_color_2 + (light_color_1 * dot(N, L)) / 127
 *
 * which is term for term what the DS geometry engine computes in hardware from
 * GFX_NORMAL, and E14 measured that engine idle on every one of 946 fighter
 * submissions. E1 refuted memoising the result because the light is transformed
 * into each animating joint's local space every frame -- the exact problem the
 * hardware solves for free.
 *
 * The mapping is exact rather than approximate. With the DS's row-vector
 * convention a normal submitted under vector matrix V is dotted as N.V.L_stored,
 * and the software computes N^T.M.L; with V = M (which E17's split load already
 * establishes, mode 2 writing position *and* vector) and the light written while
 * the vector matrix is identity, the two are the same product. So light 0's
 * colour is white and the source's two light colours become the *material*
 * diffuse and ambient, folded once per epoch with the material colour and the
 * damage-flash modulate -- 46.4 register writes a frame instead of 484 vertex
 * shades.
 *
 * E16a measured the terms that decide the placement: the light direction changes
 * 0 times in 22,296 epochs, so GFX_LIGHT_VECTOR is written once per owner
 * execute while the vector matrix is identity; the colours move on 32% of epochs
 * and the material on 72%, so GFX_DIFFUSE_AMBIENT is per epoch. */
/* R2-03 E29. Beside sNdsNativeFighterPreparedDense in DTCM: the emit reads one
 * word from each per corner, 1,878 times a frame, in the same random order, so
 * the two tables thrash the same 4 KB data cache against each other. Built once
 * at load by ARM9 code and read only by ARM9 code -- never a DMA endpoint, never
 * touched by the ARM7 or IPC, per the check-task20-dtcm-layout.ps1 gate. */
static u32 sNdsNativeFighterDenseNormals[
    sizeof(sNdsNativeFighterDenseVertices) /
        sizeof(sNdsNativeFighterDenseVertices[0])]
    __attribute__((section(".dtcm.fighter")));
/* The low-detail prepared table is intentionally main-RAM because the DTCM
 * budget cannot hold both detail sets.  Keep its equally cold one-time normal
 * bake beside it in main RAM too; the hot emit sees only the selected pointer. */
static u32 sNdsNativeFighterDenseNormalsLow[
    sizeof(sNdsNativeFighterDenseVerticesLow) /
        sizeof(sNdsNativeFighterDenseVerticesLow[0])];
#if NDS_P2_LUIGI
#if !NDS_NATIVE_OWNER_IMAGE_LUIGI
/* P2-3: Luigi owns independent generated geometry, so its one-time normal
 * bake cannot alias Mario/Fox's dense-ID namespace. Keep both detail tables in
 * cached main RAM; the P2-2 ITCM/DTCM pack has only 96 B of DTCM slack. */
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeLuigiFighterDenseNormals[
    NDS_NATIVE_IMAGE_LUIGI_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeLuigiFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_LUIGI_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_DONKEY
#if !NDS_NATIVE_OWNER_IMAGE_DONKEY
/* Donkey is another independent dense-ID namespace.  As with Luigi, these are
 * one-time CPU-built/read-only tables in cached main RAM, never DMA endpoints;
 * do not consume the already-packed P2-2 DTCM budget to admit a fighter. */
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeDonkeyFighterDenseNormals[
    NDS_NATIVE_IMAGE_DONKEY_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeDonkeyFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_DONKEY_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_CAPTAIN
#if !NDS_NATIVE_OWNER_IMAGE_CAPTAIN
/* Every P2-3 owner has an independent dense-ID namespace. Falcon used to
 * alias Mario/Fox's normal cache because his first owner landing never added
 * this pair; that makes the cache's contents depend on which kind drew first.
 * Keep the one-time table owner-local just like the geometry it indexes. */
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeCaptainFighterDenseNormals[
    NDS_NATIVE_IMAGE_CAPTAIN_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeCaptainFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_CAPTAIN_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_SAMUS
#if !NDS_NATIVE_OWNER_IMAGE_SAMUS
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeSamusFighterDenseNormals[
    NDS_NATIVE_IMAGE_SAMUS_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeSamusFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_SAMUS_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_LINK
#if !NDS_NATIVE_OWNER_IMAGE_LINK
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeLinkFighterDenseNormals[
    NDS_NATIVE_IMAGE_LINK_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeLinkFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_LINK_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_PIKACHU
#if !NDS_NATIVE_OWNER_IMAGE_PIKACHU
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativePikachuFighterDenseNormals[
    NDS_NATIVE_IMAGE_PIKACHU_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativePikachuFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_PIKACHU_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_YOSHI
#if !NDS_NATIVE_OWNER_IMAGE_YOSHI
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeYoshiFighterDenseNormals[
    NDS_NATIVE_IMAGE_YOSHI_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeYoshiFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_YOSHI_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_NESS
#if !NDS_NATIVE_OWNER_IMAGE_NESS
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeNessFighterDenseNormals[
    NDS_NATIVE_IMAGE_NESS_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNessFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NESS_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_PURIN
#if !NDS_NATIVE_OWNER_IMAGE_PURIN
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativePurinFighterDenseNormals[
    NDS_NATIVE_IMAGE_PURIN_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativePurinFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_PURIN_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_KIRBY
#if !NDS_NATIVE_OWNER_IMAGE_KIRBY
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeKirbyFighterDenseNormals[
    NDS_NATIVE_IMAGE_KIRBY_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeKirbyFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_KIRBY_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_MMARIO
static u32 sNdsNativeMMarioFighterDenseNormals[
    NDS_NATIVE_IMAGE_MMARIO_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeMMarioFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_MMARIO_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NMARIO
static u32 sNdsNativeNMarioFighterDenseNormals[
    NDS_NATIVE_IMAGE_NMARIO_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNMarioFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NMARIO_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NFOX
static u32 sNdsNativeNFoxFighterDenseNormals[
    NDS_NATIVE_IMAGE_NFOX_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNFoxFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NFOX_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NDONKEY
static u32 sNdsNativeNDonkeyFighterDenseNormals[
    NDS_NATIVE_IMAGE_NDONKEY_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNDonkeyFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NDONKEY_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NSAMUS
static u32 sNdsNativeNSamusFighterDenseNormals[
    NDS_NATIVE_IMAGE_NSAMUS_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNSamusFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NSAMUS_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NLINK
static u32 sNdsNativeNLinkFighterDenseNormals[
    NDS_NATIVE_IMAGE_NLINK_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNLinkFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NLINK_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NYOSHI
static u32 sNdsNativeNYoshiFighterDenseNormals[
    NDS_NATIVE_IMAGE_NYOSHI_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNYoshiFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NYOSHI_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NCAPTAIN
static u32 sNdsNativeNCaptainFighterDenseNormals[
    NDS_NATIVE_IMAGE_NCAPTAIN_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNCaptainFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NCAPTAIN_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NKIRBY
static u32 sNdsNativeNKirbyFighterDenseNormals[
    NDS_NATIVE_IMAGE_NKIRBY_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNKirbyFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NKIRBY_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NPIKACHU
static u32 sNdsNativeNPikachuFighterDenseNormals[
    NDS_NATIVE_IMAGE_NPIKACHU_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNPikachuFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NPIKACHU_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NPURIN
static u32 sNdsNativeNPurinFighterDenseNormals[
    NDS_NATIVE_IMAGE_NPURIN_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNPurinFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NPURIN_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NNESS
static u32 sNdsNativeNNessFighterDenseNormals[
    NDS_NATIVE_IMAGE_NNESS_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNNessFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NNESS_LOW_DENSE_VERTICES_COUNT];
#endif
static u8 sNdsNativeFighterDenseNormalsBuilt;
static u8 sNdsNativeFighterDenseNormalsBuiltLow;
#if NDS_P2_LUIGI
#if !NDS_NATIVE_OWNER_IMAGE_LUIGI
static u8 sNdsNativeLuigiFighterDenseNormalsBuilt;
static u8 sNdsNativeLuigiFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_DONKEY
#if !NDS_NATIVE_OWNER_IMAGE_DONKEY
static u8 sNdsNativeDonkeyFighterDenseNormalsBuilt;
static u8 sNdsNativeDonkeyFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_CAPTAIN
#if !NDS_NATIVE_OWNER_IMAGE_CAPTAIN
static u8 sNdsNativeCaptainFighterDenseNormalsBuilt;
static u8 sNdsNativeCaptainFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_SAMUS
#if !NDS_NATIVE_OWNER_IMAGE_SAMUS
static u8 sNdsNativeSamusFighterDenseNormalsBuilt;
static u8 sNdsNativeSamusFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_LINK
#if !NDS_NATIVE_OWNER_IMAGE_LINK
static u8 sNdsNativeLinkFighterDenseNormalsBuilt;
static u8 sNdsNativeLinkFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_PIKACHU
#if !NDS_NATIVE_OWNER_IMAGE_PIKACHU
static u8 sNdsNativePikachuFighterDenseNormalsBuilt;
static u8 sNdsNativePikachuFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_YOSHI
#if !NDS_NATIVE_OWNER_IMAGE_YOSHI
static u8 sNdsNativeYoshiFighterDenseNormalsBuilt;
static u8 sNdsNativeYoshiFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_NESS
#if !NDS_NATIVE_OWNER_IMAGE_NESS
static u8 sNdsNativeNessFighterDenseNormalsBuilt;
static u8 sNdsNativeNessFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_PURIN
#if !NDS_NATIVE_OWNER_IMAGE_PURIN
static u8 sNdsNativePurinFighterDenseNormalsBuilt;
static u8 sNdsNativePurinFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_KIRBY
#if !NDS_NATIVE_OWNER_IMAGE_KIRBY
static u8 sNdsNativeKirbyFighterDenseNormalsBuilt;
static u8 sNdsNativeKirbyFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_MMARIO
static u8 sNdsNativeMMarioFighterDenseNormalsBuilt;
static u8 sNdsNativeMMarioFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NMARIO
static u8 sNdsNativeNMarioFighterDenseNormalsBuilt;
static u8 sNdsNativeNMarioFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NFOX
static u8 sNdsNativeNFoxFighterDenseNormalsBuilt;
static u8 sNdsNativeNFoxFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NDONKEY
static u8 sNdsNativeNDonkeyFighterDenseNormalsBuilt;
static u8 sNdsNativeNDonkeyFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NSAMUS
static u8 sNdsNativeNSamusFighterDenseNormalsBuilt;
static u8 sNdsNativeNSamusFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NLINK
static u8 sNdsNativeNLinkFighterDenseNormalsBuilt;
static u8 sNdsNativeNLinkFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NYOSHI
static u8 sNdsNativeNYoshiFighterDenseNormalsBuilt;
static u8 sNdsNativeNYoshiFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NCAPTAIN
static u8 sNdsNativeNCaptainFighterDenseNormalsBuilt;
static u8 sNdsNativeNCaptainFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NKIRBY
static u8 sNdsNativeNKirbyFighterDenseNormalsBuilt;
static u8 sNdsNativeNKirbyFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NPIKACHU
static u8 sNdsNativeNPikachuFighterDenseNormalsBuilt;
static u8 sNdsNativeNPikachuFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NPURIN
static u8 sNdsNativeNPurinFighterDenseNormalsBuilt;
static u8 sNdsNativeNPurinFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NNESS
static u8 sNdsNativeNNessFighterDenseNormalsBuilt;
static u8 sNdsNativeNNessFighterDenseNormalsBuiltLow;
#endif
static u32 *sNdsNativeFighterActiveDenseNormals =
    sNdsNativeFighterDenseNormals;
static u8 *sNdsNativeFighterActiveDenseNormalsBuilt =
    &sNdsNativeFighterDenseNormalsBuilt;
/* P2-3f49: imaged owners never bake at runtime -- their words arrive inside
 * the bound NitroFS image -- so their Built pointer aims here, permanently
 * set, and the production bake call skips them. Only Mario/Fox still bake,
 * into the pair above. */
static u8 sNdsNativeImageDenseNormalsReady = 1u;

/* Selection is one owner-level operation, never an inner-loop operation.  Keep
 * it out of `.itcm.native_fighter`: inlining the table/normal pointer fan-out
 * into the production owner wastes scarce ITCM on a once-per-draw branch. */
static s32 __attribute__((noinline, optimize("Os"),
                          section(".text.native_fighter_select")))
ndsRendererNativeSelectFighterRuntimeTables(u32 slot, u32 use_low_detail)
{
    const NDSNativeFighterOwnerRuntime *owner =
        ndsRendererNativeFighterOwnerForDetail(slot, use_low_detail);

    if (owner == NULL)
    {
        return FALSE;
    }
    sNdsNativeFighterActiveOwner = owner;
    sNdsNativeFighterActiveTables = owner->tables;
#if NDS_P2_LUIGI
    if (slot == 2u)
    {
#if NDS_NATIVE_OWNER_IMAGE_LUIGI
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeLuigiFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeLuigiFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeLuigiFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeLuigiFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_DONKEY
    if (slot == 3u)
    {
#if NDS_NATIVE_OWNER_IMAGE_DONKEY
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeDonkeyFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeDonkeyFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeDonkeyFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeDonkeyFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_CAPTAIN
    if (slot == 4u)
    {
#if NDS_NATIVE_OWNER_IMAGE_CAPTAIN
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeCaptainFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeCaptainFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeCaptainFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeCaptainFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_SAMUS
    if (slot == 5u)
    {
#if NDS_NATIVE_OWNER_IMAGE_SAMUS
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeSamusFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeSamusFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeSamusFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeSamusFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_KIRBY
    if (slot == 11u)
    {
#if NDS_NATIVE_OWNER_IMAGE_KIRBY
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeKirbyFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeKirbyFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeKirbyFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeKirbyFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_MMARIO
    if (slot == 12u)
    {
#if NDS_NATIVE_OWNER_IMAGE_MMARIO
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeMMarioFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeMMarioFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeMMarioFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeMMarioFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NMARIO
    if (slot == 13u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NMARIO
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNMarioFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNMarioFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNMarioFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNMarioFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NFOX
    if (slot == 14u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NFOX
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNFoxFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNFoxFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNFoxFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNFoxFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NDONKEY
    if (slot == 15u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NDONKEY
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNDonkeyFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNDonkeyFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNDonkeyFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNDonkeyFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NSAMUS
    if (slot == 16u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NSAMUS
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNSamusFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNSamusFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNSamusFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNSamusFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NLINK
    if (slot == 17u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NLINK
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNLinkFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNLinkFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNLinkFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNLinkFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NYOSHI
    if (slot == 18u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NYOSHI
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNYoshiFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNYoshiFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNYoshiFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNYoshiFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NCAPTAIN
    if (slot == 19u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NCAPTAIN
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNCaptainFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNCaptainFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNCaptainFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNCaptainFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NKIRBY
    if (slot == 20u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NKIRBY
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNKirbyFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNKirbyFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNKirbyFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNKirbyFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NPIKACHU
    if (slot == 21u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NPIKACHU
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNPikachuFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNPikachuFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNPikachuFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNPikachuFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NPURIN
    if (slot == 22u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NPURIN
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNPurinFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNPurinFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNPurinFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNPurinFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NNESS
    if (slot == 23u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NNESS
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNNessFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNNessFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNNessFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNNessFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_PURIN
    if (slot == 10u)
    {
#if NDS_NATIVE_OWNER_IMAGE_PURIN
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativePurinFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativePurinFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativePurinFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativePurinFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NESS
    if (slot == 9u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NESS
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNessFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNessFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNessFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNessFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_YOSHI
    if (slot == 8u)
    {
#if NDS_NATIVE_OWNER_IMAGE_YOSHI
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeYoshiFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeYoshiFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeYoshiFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeYoshiFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_PIKACHU
    if (slot == 7u)
    {
#if NDS_NATIVE_OWNER_IMAGE_PIKACHU
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativePikachuFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativePikachuFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativePikachuFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativePikachuFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_LINK
    if (slot == 6u)
    {
#if NDS_NATIVE_OWNER_IMAGE_LINK
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeLinkFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeLinkFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeLinkFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeLinkFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
    if (use_low_detail != 0u)
    {
        sNdsNativeFighterActiveDenseNormals = sNdsNativeFighterDenseNormalsLow;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeFighterDenseNormalsBuiltLow;
    }
    else
    {
        sNdsNativeFighterActiveDenseNormals = sNdsNativeFighterDenseNormals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeFighterDenseNormalsBuilt;
    }
    return TRUE;
}

static const NDSEntryEffectRoot *ndsRendererEntryEffectRoot(
    u32 owner_asset_id, u32 root_offset)
{
    u32 first = (owner_asset_id == 356u) ? 0u :
                (owner_asset_id == 161u) ? NDS_ENTRY_EFFECT_FOX_ROOT_FIRST :
                (owner_asset_id == 355u) ? NDS_ENTRY_EFFECT_DONKEY_ROOT_FIRST :
                (owner_asset_id == 349u) ? NDS_ENTRY_EFFECT_SAMUS_ROOT_FIRST :
                (owner_asset_id == 350u) ? NDS_ENTRY_EFFECT_CAPTAIN_ROOT_FIRST :
                (owner_asset_id == 353u) ? NDS_ENTRY_EFFECT_LINK_ROOT_FIRST :
                (owner_asset_id == 324u) ?
                    NDS_ENTRY_EFFECT_LINK_SPIN_WEAPON_ROOT_FIRST :
                (owner_asset_id == 325u) ?
                    NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST :
                                           NDS_ENTRY_EFFECT_ROOT_COUNT;
    u32 last = (owner_asset_id == 356u) ? NDS_ENTRY_EFFECT_MARIO_ROOT_COUNT :
               (owner_asset_id == 161u) ? NDS_ENTRY_EFFECT_DONKEY_ROOT_FIRST :
               (owner_asset_id == 355u) ? NDS_ENTRY_EFFECT_SAMUS_ROOT_FIRST :
               (owner_asset_id == 349u) ? NDS_ENTRY_EFFECT_CAPTAIN_ROOT_FIRST :
               (owner_asset_id == 350u) ? NDS_ENTRY_EFFECT_LINK_ROOT_FIRST :
               (owner_asset_id == 353u) ?
                   NDS_ENTRY_EFFECT_LINK_SPIN_WEAPON_ROOT_FIRST :
               (owner_asset_id == 324u) ?
                   NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST :
               (owner_asset_id == 325u) ? NDS_ENTRY_EFFECT_ROOT_COUNT : first;
    u32 i;

    for (i = first; i < last; i++)
    {
        if (sNdsEntryEffectRoots[i].source_offset == root_offset)
        {
            return &sNdsEntryEffectRoots[i];
        }
    }
    return NULL;
}

#if NDS_P2_LINK_SPECIAL_TOUR
/* Lab-only exact-root query for the controller-driven Link acceptance tour.
 * The renderer already keeps one draw counter per generated native root; this
 * accessor lets the guest prove the exact immutable packet that was submitted
 * without halting melonDS on every renderer call. */
__attribute__((used))
u32 ndsRendererEntryEffectNativeRootDrawCount(u32 owner_asset_id,
                                               u32 root_offset)
{
    const NDSEntryEffectRoot *root =
        ndsRendererEntryEffectRoot(owner_asset_id, root_offset);
    u32 root_index;

    if (root == NULL)
    {
        return 0u;
    }
    root_index = (u32)(root - sNdsEntryEffectRoots);
    return (root_index < NDS_ENTRY_EFFECT_ROOT_COUNT) ?
        gNdsEntryEffectNativeRootDraws[root_index] : 0u;
}
#endif

/* Rehydrate one exact decoded source corner from the compact immutable packet.
 * No numeric conversion happens here: every dictionary entry is the same s16
 * or u8 value the old flat NDSRendererInputVertex table stored.  Keeping this
 * reconstruction at the native owner preserves all downstream lighting,
 * projection and GX submission code while avoiding ~10 KiB of repeated entry
 * data in ARM9 RAM. */
static s32 ndsRendererEntryEffectVertex(u32 corner,
                                        NDSRendererInputVertex *out)
{
    u32 position_index;
    u32 s_index;
    u32 t_index;
    u32 color_index;
    const NDSEntryEffectPosition *position;
    const NDSEntryEffectColor *color;

    if ((out == NULL) || (corner >= NDS_ENTRY_EFFECT_VERTEX_COUNT))
    {
        return FALSE;
    }
    position_index = sNdsEntryEffectCornerPosition[corner];
    s_index = sNdsEntryEffectCornerS[corner];
    t_index = sNdsEntryEffectCornerT[corner];
    color_index = sNdsEntryEffectCornerColor[corner];
    if ((position_index >= NDS_ENTRY_EFFECT_POSITION_COUNT) ||
        (s_index >= NDS_ENTRY_EFFECT_S_COUNT) ||
        (t_index >= NDS_ENTRY_EFFECT_T_COUNT) ||
        (color_index >= NDS_ENTRY_EFFECT_COLOR_COUNT))
    {
        return FALSE;
    }
    position = &sNdsEntryEffectPositions[position_index];
    color = &sNdsEntryEffectColors[color_index];
    out->x = position->x;
    out->y = position->y;
    out->z = position->z;
    out->s = sNdsEntryEffectS[s_index];
    out->t = sNdsEntryEffectT[t_index];
    out->r = color->r;
    out->g = color->g;
    out->b = color->b;
    out->a = color->a;
    return TRUE;
}

/* DS-native landed entry-prop immutable presentation owner.
 *
 * Source ownership deliberately stops at the DObj: BattleShip continues to
 * animate/re-parent/sort the live tree, so the pipe rise and Arwing fly-by use
 * the original timing and transforms.  At each leaf, however, this function
 * consumes only build-generated DS vertices and resident DS textures.  No N64
 * command scan, vertex-cache emulation, texture/TLUT conversion, or software
 * lighting occurs here.  The generator also rejects a packet if any emitted
 * source triangle has G_LIGHTING set; the explicit mask below is a second
 * runtime fence against lighting state leaking from a previous fighter draw. */
#if NDS_ENTRY_EFFECT_DIAG
/* P2-3r6 lab instrument. The owner reports Mario's entry pipe draws its rim and
 * not its body, and every cheap check says the body IS submitted: root 0x04c0
 * takes 60 draws a match with fallback 0, valid config and matrix pointers, and
 * the same combine/othermode/texture as the rim. What has never been read
 * reliably is the MATRIX each root is submitted under -- a gdb read of the
 * caller's stack matrix returns dcache residue on this fork, which already cost
 * one wrong "garbage matrix" conclusion. So the renderer records it into
 * globals, where a stub read is sound, one row per root index.
 *
 * Lab only: NDS_ENTRY_EFFECT_DIAG defaults to 0 and the shipped ROM pays
 * nothing. */
__attribute__((used)) volatile s32 gNdsEntryEffectRootModelview[2][16];
__attribute__((used)) volatile s32 gNdsEntryEffectRootProjection[2][4];
__attribute__((used)) volatile u32 gNdsEntryEffectRootPolyFmt[2];
__attribute__((used)) volatile u32 gNdsEntryEffectRootGeom[2];
__attribute__((used)) volatile u32 gNdsEntryEffectRootTriangles[2];
__attribute__((used)) volatile s32 gNdsEntryEffectRootFirstVertex[2][3];

static void ndsRendererEntryEffectDiagMatrices(u32 root_index,
                                               const NDSRendererConfig *config)
{
    u32 i;
    u32 j;

    if (root_index >= 2u)
    {
        return;
    }
    for (i = 0u; i < 4u; i++)
    {
        for (j = 0u; j < 4u; j++)
        {
            gNdsEntryEffectRootModelview[root_index][(i * 4u) + j] =
                config->initial_modelview->m[i][j];
        }
        gNdsEntryEffectRootProjection[root_index][i] =
            config->initial_projection->m[3][i];
    }
}
#endif

static void ndsRendererEntryEffectApplyLiveMaterial(
    const NDSRendererNativeMaterial *material, NDSRendererStats *stats)
{
    u32 effects;

    if ((material == NULL) || (stats == NULL))
    {
        return;
    }
    effects = material->effects;

    /* The generated root already counts the source segment-E G_DL itself.
     * Replay only the typed branch body plus its generated branch slot here,
     * mirroring gcDrawMObjForDObj without executing a generic mini-DL. */
    stats->command_count += (u32)material->command_count + 1u;
    stats->branch_command_count += 2u;
    stats->branch_call_count++;
    stats->branch_jump_count++;
    stats->segment_resolve_count++;
    stats->end_command_count++;
    stats->sync_command_count += material->sync_count;

    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LIGHT1) != 0u)
    {
        ndsRendererRecordLightColor(stats, 1u, material->light1);
        ndsRendererRecordLightColor(stats, 1u, material->light1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LIGHT2) != 0u)
    {
        ndsRendererRecordLightColor(stats, 2u, material->light2);
        ndsRendererRecordLightColor(stats, 2u, material->light2);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_PRIM) != 0u)
    {
        stats->prim_color = material->prim_w1;
        stats->prim_min_level = (material->prim_w0 >> 8) & 0xffu;
        stats->prim_lod_fraction = material->prim_w0 & 0xffu;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE) != 0u)
    {
        ndsRendererRecordSetTileSize(
            stats, material->render_tile_size_w0,
            material->render_tile_size_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_TEXTURE) != 0u)
    {
        ndsRendererRecordTextureState(
            stats, material->texture_w0, material->texture_w1);
    }
}

s32 ndsRendererSubmitNativeEntryEffect(
    u32 owner_asset_id, u32 root_offset,
    const NDSRendererNativeMaterial *materials, u32 material_count,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    const NDSEntryEffectRoot *root;
    u32 group_offset;
    u32 matrix_generation;
    u32 root_index;
    NDSRendererHardwareLightDirection
        light_direction_by_root[NDS_ENTRY_EFFECT_ROOT_COUNT];
    u32 light_direction_valid_mask = 0u;

    if ((config == NULL) || (stats == NULL) ||
        (config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        return FALSE;
    }
    root = ndsRendererEntryEffectRoot(owner_asset_id, root_offset);
    if ((root == NULL) || (root->group_count == 0u) ||
        ((u32)root->first_group + root->group_count >
         NDS_ENTRY_EFFECT_GROUP_COUNT))
    {
        return FALSE;
    }
    root_index = (u32)(root - &sNdsEntryEffectRoots[0]);
    if (root_index >= NDS_ENTRY_EFFECT_ROOT_COUNT)
    {
        return FALSE;
    }
    /* The first root of either source effect begins a new synchronous DObj
     * traversal. Only matrices captured during THIS traversal may satisfy a
     * later vertex-cache provenance read. */
    if ((root_index == 0u) ||
        (root_index == NDS_ENTRY_EFFECT_FOX_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_DONKEY_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_SAMUS_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_CAPTAIN_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_LINK_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST))
    {
        sNdsRendererEntryEffectModelviewValidMask = 0u;
    }

    /* Fail closed before the first GX write. Besides resident texture names,
     * validate every generated compact-table index and sparse matrix override.
     * The old flat packet could only fail its contiguous corner bound; the
     * dictionary packet has more indices, so all of them are fenced here once
     * and the hot submit loop can remain branch-light. */
    for (group_offset = 0u; group_offset < root->group_count; group_offset++)
    {
        const NDSEntryEffectGroup *group =
            &sNdsEntryEffectGroups[(u32)root->first_group + group_offset];
        u32 corner_count = (u32)group->triangle_count * 3u;
        u32 corner;
        u32 override_index;
        u32 previous_override = 0xffffffffu;

        if ((group->root_index != root_index) ||
            ((u32)group->first_vertex + corner_count >
             NDS_ENTRY_EFFECT_VERTEX_COUNT) ||
            (group->geometry_state >= NDS_ENTRY_EFFECT_GEOMETRY_STATE_COUNT) ||
            (group->combine_state >= NDS_ENTRY_EFFECT_COMBINE_STATE_COUNT) ||
            (group->othermode_state >= NDS_ENTRY_EFFECT_OTHERMODE_STATE_COUNT) ||
            (group->prim_color_index >= NDS_ENTRY_EFFECT_PRIM_COLOR_COUNT) ||
            (group->env_color_index >= NDS_ENTRY_EFFECT_ENV_COLOR_COUNT) ||
            (group->light_state >= NDS_ENTRY_EFFECT_LIGHT_STATE_COUNT) ||
            ((group->material_slot != 0xffu) &&
             ((materials == NULL) ||
              ((u32)group->material_slot >= material_count))) ||
            ((u32)group->matrix_override_first + group->matrix_override_count >
             NDS_ENTRY_EFFECT_CROSS_MATRIX_CORNER_COUNT))
        {
            return FALSE;
        }
        for (corner = 0u; corner < corner_count; corner++)
        {
            u32 vertex_index = (u32)group->first_vertex + corner;

            if ((sNdsEntryEffectCornerPosition[vertex_index] >=
                 NDS_ENTRY_EFFECT_POSITION_COUNT) ||
                (sNdsEntryEffectCornerS[vertex_index] >=
                 NDS_ENTRY_EFFECT_S_COUNT) ||
                (sNdsEntryEffectCornerT[vertex_index] >=
                 NDS_ENTRY_EFFECT_T_COUNT) ||
                (sNdsEntryEffectCornerColor[vertex_index] >=
                 NDS_ENTRY_EFFECT_COLOR_COUNT))
            {
                return FALSE;
            }
        }
        for (override_index = 0u;
             override_index < group->matrix_override_count;
             override_index++)
        {
            u32 table_index =
                (u32)group->matrix_override_first + override_index;
            u32 override_corner =
                sNdsEntryEffectMatrixOverrideCorner[table_index];
            u32 source_root =
                sNdsEntryEffectMatrixOverrideRoot[table_index];

            if ((override_corner >= corner_count) ||
                ((previous_override != 0xffffffffu) &&
                 (override_corner <= previous_override)) ||
                (source_root >= NDS_ENTRY_EFFECT_ROOT_COUNT) ||
                (source_root >= root_index) ||
                ((sNdsRendererEntryEffectModelviewValidMask &
                  (1u << source_root)) == 0u))
            {
                return FALSE;
            }
            previous_override = override_corner;
        }

        if (group->texture_slot == NDS_ENTRY_EFFECT_TEXTURE_NONE)
        {
            continue;
        }
        if ((group->texture_slot >= NDS_ENTRY_EFFECT_TEXTURE_COUNT) ||
            (sNdsRendererEntryEffectTextureName[group->texture_slot] == 0u))
        {
            return FALSE;
        }
    }

    if (owner_asset_id == 353u)
    {
        u32 expected_effects;

        if ((materials == NULL) || (material_count != 1u))
        {
            return FALSE;
        }
        switch (root_offset)
        {
        case 0x02d8u:
            /* Entry Wave MObjSub flags 0x32a0: LIGHT1 | LIGHT2 | PRIM,
             * source 0x20 render-tile sizing, and G_TEXTURE scaling. */
            expected_effects =
                NDS_RENDERER_NATIVE_MATERIAL_LIGHT1 |
                NDS_RENDERER_NATIVE_MATERIAL_LIGHT2 |
                NDS_RENDERER_NATIVE_MATERIAL_PRIM |
                NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE |
                NDS_RENDERER_NATIVE_MATERIAL_TEXTURE;
            break;
        case 0x0698u:
            /* Entry Beam MObjSub flags 0x3200. */
            expected_effects =
                NDS_RENDERER_NATIVE_MATERIAL_LIGHT1 |
                NDS_RENDERER_NATIVE_MATERIAL_LIGHT2 |
                NDS_RENDERER_NATIVE_MATERIAL_PRIM;
            break;
        case 0x1100u:
            /* Attached Spin effect MObjSub flags 0x0200. */
            expected_effects = NDS_RENDERER_NATIVE_MATERIAL_PRIM;
            break;
        default:
            return FALSE;
        }
        if (materials[0].effects != expected_effects)
        {
            return FALSE;
        }
    }

    /* LinkModel+0x11680 is a deliberately closed dynamic-material owner.
     * BattleShip builds segment 0xE from exactly nine MObjs and every one has
     * MOBJ_FLAG_PRIMCOLOR only. Refuse any broader live material state rather
     * than letting this specialization silently omit source presentation. */
    if (owner_asset_id == 324u)
    {
        u32 material_index;

        if ((root_offset != 0x11680u) || (materials == NULL) ||
            (material_count != 9u))
        {
            return FALSE;
        }
        for (material_index = 0u; material_index < material_count;
             material_index++)
        {
            if (materials[material_index].effects !=
                NDS_RENDERER_NATIVE_MATERIAL_PRIM)
            {
                return FALSE;
            }
        }
    }

    ndsRendererHardwareEndBatch();
    sNdsRendererEntryEffectModelview[root_index] = *config->initial_modelview;
    ndsRendererMtxMul20p12(
        config->initial_modelview, config->initial_projection,
        &sNdsRendererEntryEffectComposed[root_index]);
    sNdsRendererEntryEffectModelviewValidMask |= 1u << root_index;
    matrix_generation = ndsRendererNextMatrixGeneration();
    ndsRendererLoadHardwareSplitMatrices(
        config->initial_projection, config->initial_modelview,
        matrix_generation);
#if NDS_ENTRY_EFFECT_DIAG
    ndsRendererEntryEffectDiagMatrices(
        (u32)(root - &sNdsEntryEffectRoots[0]), config);
#endif

    for (group_offset = 0u; group_offset < root->group_count; group_offset++)
    {
        const NDSEntryEffectGroup *group =
            &sNdsEntryEffectGroups[(u32)root->first_group + group_offset];
        u32 use_texture =
            (group->texture_slot != NDS_ENTRY_EFFECT_TEXTURE_NONE) ? TRUE : FALSE;
        u32 texture_name = 0u;
        u32 material_color;
        s32 use_material_color;
        s32 use_vertex_color;
        u32 poly_fmt;
        u32 corner_count = (u32)group->triangle_count * 3u;
        u32 corner;
        u32 projected_group =
            (group->matrix_override_count != 0u) ? TRUE : FALSE;
        u32 matrix_override_cursor = 0u;
        const NDSEntryEffectPairState *geometry_state =
            &sNdsEntryEffectGeometryStates[group->geometry_state];
        const NDSEntryEffectPairState *combine_state =
            &sNdsEntryEffectCombineStates[group->combine_state];
        const NDSEntryEffectPairState *othermode_state =
            &sNdsEntryEffectOthermodeStates[group->othermode_state];
        const NDSEntryEffectPairState *light_state =
            &sNdsEntryEffectLightColors[group->light_state];
        u32 light_mask = sNdsEntryEffectLightMasks[group->light_state];
        s32 lit;

        /* The RSP transforms a vertex when G_VTX loads its cache slot.  A later
         * display list may then reuse that slot after the modelview changes.
         * Mario's pipe body does exactly that: its first twelve triangles mix
         * vertices loaded under the rim and body matrices.  GX cannot reproduce
         * that by changing its matrix between vertices of one open primitive;
         * the generic renderer handles the same source shape by CPU-projecting
         * the mixed-snapshot triangle.  If a generated state group contains any
         * such corner, project the whole small group from each corner's recorded
         * load-time matrix.  This keeps primitive submission legal and preserves
         * the generic path's load-time transform semantics.
         * Ordinary entry groups remain on the raw split-matrix path.  The
         * generated sparse override count is exactly the old per-corner test. */

        /* A bit the list neither cleared nor set is inherited from the battle
         * display's state, exactly as the RSP had it. Masking G_LIGHTING off
         * here drew the pipe's normals as colours: a black pipe body. */
        stats->geometry_mode =
            (config->initial_geometry_mode & ~geometry_state->b) |
            geometry_state->a;
        stats->othermode_h = othermode_state->a;
        stats->othermode_l = othermode_state->b;
        stats->prim_color = sNdsEntryEffectPrimColors[group->prim_color_index];
        stats->env_color = sNdsEntryEffectEnvColors[group->env_color_index];
        ndsRendererRecordSetCombine(stats, combine_state->a, combine_state->b);
        if ((light_mask & 1u) != 0u)
        {
            stats->light_color_1 = light_state->a;
            stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_1_MASK;
        }
        if ((light_mask & 2u) != 0u)
        {
            stats->light_color_2 = light_state->b;
            stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_2_MASK;
        }
        if (group->material_slot != 0xffu)
        {
            const NDSRendererNativeMaterial *material =
                &materials[group->material_slot];

            /* Source segment 0xE executes immediately before these triangles.
             * The admission contracts above make this a closed typed replay:
             * LinkSpecial2 may carry live light/PRIM/texture-scale state and
             * LinkModel Spin carries PRIM only. */
            ndsRendererEntryEffectApplyLiveMaterial(material, stats);
        }
        lit = ndsRendererHardwareLitShadeCombine(stats);
        if (lit != FALSE)
        {
            ndsRendererHardwarePrepareLitDirection(
                stats, config->initial_modelview,
                &light_direction_by_root[root_index]);
            light_direction_valid_mask |= 1u << root_index;
        }

        if (use_texture != FALSE)
        {
            const NDSEntryEffectTexture *texture =
                &sNdsEntryEffectTextures[group->texture_slot];
            NDSRendererTileState tile = {0};
            u32 params;

            texture_name =
                sNdsRendererEntryEffectTextureName[group->texture_slot];
            tile.set_seen = TRUE;
            tile.width = texture->width;
            tile.height = texture->height;
            tile.cms = group->cms;
            tile.cmt = group->cmt;
            tile.masks = group->masks;
            tile.maskt = group->maskt;
            params = ndsRendererHardwareTextureParams(
                stats, &tile, texture->width, texture->height);
            ndsRendererHardwareBindTextureName(stats, texture_name);
            ndsRendererHardwareApplyTextureParams(
                ndsRendererHardwareMergeTextureParams(params));
            sNdsRendererHardwareActiveTextureEntry = NULL;
            stats->hardware_texture_ready_count++;
            gNdsEntryEffectNativeTextureBindCount++;
        }

        material_color = ndsRendererHardwareColorSource(stats);
        use_material_color = ndsRendererHardwareUseMaterialColor(stats);
        use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
        poly_fmt = ndsRendererHardwarePolyFmt(stats, 31u);
        /* Lit groups are shaded on the CPU below, like the native fighter
         * owner; POLY_FORMAT_LIGHT0 must stay absent even if a previous
         * hardware-lit owner left a light vector in GX state. */
        poly_fmt &= ~((u32)POLY_FORMAT_LIGHT0);
        if (projected_group != FALSE)
        {
            ndsRendererLoadHardwareMatrices(NULL, FALSE);
        }
        else
        {
            ndsRendererLoadHardwareSplitMatrices(
                config->initial_projection, config->initial_modelview,
                matrix_generation);
        }
        ndsRendererHardwareBeginTriangleBatch(
            stats, use_texture, texture_name, poly_fmt,
            sNdsRendererHardwareMatrixMode,
            sNdsRendererHardwareMatrixGeneration);

        for (corner = 0u; corner < corner_count; corner++)
        {
            u32 vertex_index = (u32)group->first_vertex + corner;
            NDSRendererInputVertex vertex;
            const NDSRendererInputVertex *vtx = &vertex;
            u32 source_root = root_index;
            u16 packed_color;

            if (matrix_override_cursor < group->matrix_override_count)
            {
                u32 table_index =
                    (u32)group->matrix_override_first + matrix_override_cursor;

                if (sNdsEntryEffectMatrixOverrideCorner[table_index] == corner)
                {
                    source_root =
                        sNdsEntryEffectMatrixOverrideRoot[table_index];
                    matrix_override_cursor++;
                }
            }
            if (ndsRendererEntryEffectVertex(vertex_index, &vertex) == FALSE)
            {
                /* Prevalidation above makes this unreachable unless generated
                 * data is corrupted after the fence. End the open batch before
                 * refusing so the adapter can fall back safely. */
                ndsRendererHardwareEndBatch();
                return FALSE;
            }

            if (lit != FALSE)
            {
                if ((light_direction_valid_mask & (1u << source_root)) == 0u)
                {
                    ndsRendererHardwarePrepareLitDirection(
                        stats, &sNdsRendererEntryEffectModelview[source_root],
                        &light_direction_by_root[source_root]);
                    light_direction_valid_mask |= 1u << source_root;
                }
                /* r,g,b hold the source normal; shade it against the seeded
                 * battle light under the matrix that was live when the RSP
                 * cache slot was loaded, not necessarily this DObj's matrix. */
                packed_color = ndsRendererHardwarePackedResolvedColor(
                    ndsRendererHardwareLitShadeColorPrepared(
                        stats, vtx, &light_direction_by_root[source_root]),
                    material_color, use_material_color, 0u);
            }
            else
            {
                u32 vertex_color =
                    ((u32)vtx->r << 24) | ((u32)vtx->g << 16) |
                    ((u32)vtx->b << 8) | (u32)vtx->a;

                packed_color = ndsRendererHardwarePackedVertexColor(
                    stats, vtx, material_color,
                    use_material_color, use_vertex_color,
                    vertex_color, TRUE, 0u);
            }
            glColor(packed_color);
            if (use_texture != FALSE)
            {
                /* Already converted from N64 s10.5 + tile/scale state to the
                 * DS t16 coordinate in the generator. */
                glTexCoord2t16((t16)vtx->s, (t16)vtx->t);
            }
            if (projected_group != FALSE)
            {
                NDSRendererClipVertex20p12 clip;

                ndsRendererTransformVertex20p12(
                    &sNdsRendererEntryEffectComposed[source_root], vtx, &clip);
                stats->matrix_transform_count++;
                stats->transformed_vertex_count++;
                ndsRendererHardwareClipVertex(
                    &clip, clip.z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                    , NULL
#endif
                    );
            }
            else
            {
                glVertex3v16(
                    ndsRendererHardwareVertexCoord(vtx->x, TRUE),
                    ndsRendererHardwareVertexCoord(vtx->y, TRUE),
                    ndsRendererHardwareVertexCoord(vtx->z, TRUE));
            }
        }
        if (projected_group != FALSE)
        {
            ndsRendererHardwareEnterProjectedForeground();
        }
#if NDS_ENTRY_EFFECT_DIAG
        {
            u32 diag_root = (u32)(root - &sNdsEntryEffectRoots[0]);

            if (diag_root < 2u)
            {
                NDSRendererInputVertex first_vertex;
                const NDSRendererInputVertex *first = &first_vertex;

                if (ndsRendererEntryEffectVertex(
                        group->first_vertex, &first_vertex) == FALSE)
                {
                    return FALSE;
                }

                gNdsEntryEffectRootPolyFmt[diag_root] = poly_fmt;
                gNdsEntryEffectRootGeom[diag_root] = stats->geometry_mode;
                gNdsEntryEffectRootTriangles[diag_root] +=
                    group->triangle_count;
                gNdsEntryEffectRootFirstVertex[diag_root][0] =
                    ndsRendererHardwareVertexCoord(first->x, TRUE);
                gNdsEntryEffectRootFirstVertex[diag_root][1] =
                    ndsRendererHardwareVertexCoord(first->y, TRUE);
                gNdsEntryEffectRootFirstVertex[diag_root][2] =
                    ndsRendererHardwareVertexCoord(first->z, TRUE);
            }
        }
#endif
        sNdsRendererHardwareSubmitted = TRUE;
        stats->triangle_count += group->triangle_count;
        stats->transformed_triangle_count += group->triangle_count;
        stats->hardware_triangle_count += group->triangle_count;
        stats->hardware_vertex_count += corner_count;
        /* This owner submits ordinary model-space vertices through the live
         * projection/modelview pair.  They therefore use GX's normal Z-buffer
         * depth path just like the generic source-DL interpreter.  Keep the
         * depth census coherent with hardware_triangle_count; omitting this
         * made a working pipe/Arwing look like thousands of unclassified stage
         * triangles to the exact realtime verifier. */
        if (projected_group != FALSE)
        {
            stats->hardware_projected_depth_triangle_count +=
                group->triangle_count;
        }
        else
        {
            stats->hardware_zbuffer_triangle_count += group->triangle_count;
        }
        ndsRendererHardwareEndBatch();
    }
    gNdsEntryEffectNativeDrawCount++;
    {
        if (root_index < NDS_ENTRY_EFFECT_ROOT_COUNT)
        {
            gNdsEntryEffectNativeRootDraws[root_index]++;
        }
    }
    return TRUE;
#else
    (void)owner_asset_id;
    (void)root_offset;
    (void)materials;
    (void)material_count;
    (void)config;
    (void)stats;
    return FALSE;
#endif
}

s32 ndsRendererHardwarePrepareEntryEffectTextures(void)
{
#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    u32 i;

    for (i = 0u; i < NDS_ENTRY_EFFECT_TEXTURE_COUNT; i++)
    {
        const NDSEntryEffectTexture *texture = &sNdsEntryEffectTextures[i];
        s32 prepared;

        if (sNdsRendererEntryEffectTextureName[i] != 0u)
        {
            continue;
        }
        if ((texture->texels == NULL) ||
            (texture->width == 0u) || (texture->height == 0u))
        {
            return FALSE;
        }
        if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_PAL16)
        {
            if (texture->palette_entries != 16u)
            {
                return FALSE;
            }
            /* Unlike IFCommon's PAL16 atlases, an entry texture does not
             * necessarily reserve index 0 for transparency. Mario's pipe uses
             * index 0 as opaque green in both CI4 images; forcing COLOR0 there
             * removes whole texel rows from otherwise valid polygons. */
            prepared = ndsRendererHardwarePrepareIFCommonAtlas(
                texture->width, texture->height, GL_RGB16,
                texture->palette, 16u,
                ndsRendererEntryEffectTextureFill, (void *)texture,
                &sNdsRendererEntryEffectTextureName[i],
                texture->color0_transparent);
        }
        else if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_A5I3)
        {
            if (texture->palette_entries != 8u)
            {
                return FALSE;
            }
            prepared = ndsRendererHardwarePrepareIFCommonCloudAtlas(
                texture->width, texture->height, texture->palette,
                ndsRendererEntryEffectTextureFill, (void *)texture,
                &sNdsRendererEntryEffectTextureName[i]);
        }
        else if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_RGBA)
        {
            if ((texture->palette != NULL) || (texture->palette_entries != 0u))
            {
                return FALSE;
            }
            prepared = ndsRendererHardwarePrepareIFCommonAtlas(
                texture->width, texture->height, GL_RGBA,
                NULL, 0u,
                ndsRendererEntryEffectTextureFill, (void *)texture,
                &sNdsRendererEntryEffectTextureName[i], FALSE);
        }
        else
        {
            return FALSE;
        }
        if (prepared == FALSE)
        {
            return FALSE;
        }
        gNdsEntryEffectNativeTexturePrepareCount++;
    }
    return TRUE;
#else
    return FALSE;
#endif
}

/* libnds's NORMAL_PACK does not mask its z argument -- it is
 * `(x & 0x3FF) | ((y & 0x3FF) << 10) | (z << 20)` -- so a negative z sign-
 * extends into bits 30 and 31. Those bits are unused in GFX_NORMAL but are the
 * *light number* in GFX_LIGHT_VECTOR, so NORMAL_PACK(0, 0, -511) writes
 * 0xE0100000: light 3, which POLY_FORMAT never enables. Light 0 kept its
 * power-on zero vector, every dot product was zero, and the fighters rendered
 * with ambient only. Mask all three components. */
#define NDS_R2_NORMAL_PACK(x, y, z) \
    ((((u32)(x)) & 0x3ffu) | \
     (((((u32)(y)) & 0x3ffu)) << 10) | \
     (((((u32)(z)) & 0x3ffu)) << 20))

/* Source normals are s8 with 1.0 == 127; the DS normal is 10-bit signed with
 * 1.0 == 0x1ff. Load time, so the exact scale costs nothing. */
static s32 ndsRendererR2NormalComponent(s32 source)
{
    s32 scaled = (source * 0x1ff) / 127;

    if (scaled > 511) { scaled = 511; }
    if (scaled < -512) { scaled = -512; }
    return scaled;
}

static void __attribute__((noinline)) ndsRendererR2BuildDenseNormals(void)
{
    u32 count = sNdsNativeFighterActiveTables->dense_count;
    u32 index;

    for (index = 0u; index < count; index++)
    {
        u32 rgba = sNdsNativeFighterActiveTables->dense_vertices[index].rgba;
        s32 nx = ndsRendererR2NormalComponent((s32)(s8)(rgba >> 24));
        s32 ny = ndsRendererR2NormalComponent((s32)(s8)(rgba >> 16));
        s32 nz = ndsRendererR2NormalComponent((s32)(s8)(rgba >> 8));

        sNdsNativeFighterActiveDenseNormals[index] =
            NDS_R2_NORMAL_PACK((int)nx, (int)ny, (int)nz);
    }
    *sNdsNativeFighterActiveDenseNormalsBuilt = 1u;
}

/* One channel of the material term, reproducing the software path's arithmetic
 * so the only intended difference is that the engine evaluates the dot product.
 * `base` is the source light colour byte; `material` is the epoch's material
 * byte when the policy uses one. */
static inline u32 ndsRendererR2MaterialChannel(
    u32 base, u32 material, u32 use_material)
{
    if (use_material != 0u)
    {
        return ndsRendererHardwareScaleMaterialChannel5(base, material);
    }
    return (base >> 3) & 0x1fu;
}

/* Cleared per owner execute; set once the light vector has been written. */
static u8 sNdsR2LightVectorWritten;

#if NDS_R2_UNLIT_VERTEX_EPOCH
/* R2-03 E49. Set per epoch by the shade pass, read by the run prepare and by the
 * four emit loops -- all of which run after it for the same epoch.
 *
 * E48 measured the rule the native owner was missing: when the generic path sees
 * a valid vertex colour and no material it emits that colour raw and never
 * lights it (273 of 273 vertices on hitlag frame 911, material 0,
 * use_material_color FALSE). The native owner decided `epoch_lit` from
 * `geometry_mode & LIGHTING` alone and ran the geometry engine's light, which is
 * E32's dark-maroon hurt flash. */
static u8 sNdsR2EpochUnlitVertexColor;

/* Byte-for-byte the expression ndsRendererHardwarePackedResolvedColor uses on
 * its no-material route, so the two paths cannot drift. */
static inline u16 ndsRendererR2DenseVertexColor15(u32 dense_id)
{
    u32 rgba = sNdsNativeFighterActiveTables->dense_vertices[dense_id].rgba;

    return RGB15((u8)((rgba >> 27) & 0x1fu),
                 (u8)((rgba >> 19) & 0x1fu),
                 (u8)((rgba >> 11) & 0x1fu));
}
#endif
/* Engagement proof. The ambient-only bisect showed the material path working
 * end to end while the diffuse term stayed zero, which leaves only the light
 * vector -- and "the write never ran" and "the write ran and did not stick" are
 * different bugs. */
u32 gNdsR2LightVectorWrites;

/* GFX_LIGHT_VECTOR stores the vector transformed by the vector matrix at write
 * time, so it has to be written under an identity vector matrix. The owner
 * preamble looked like the place for that -- it already loads identity before
 * the first root -- but stats->light_dir_* is only populated by the epoch state
 * deltas, so at preamble time it is still zero. A zero light vector makes every
 * dot product zero, which is exactly the black-silhouette failure the first
 * build showed and the diffuse-only diagnostic confirmed.
 *
 * So it is written here instead, the first time an epoch actually has a
 * direction, bracketed by push/identity/pop so the root's matrices survive.
 * E16a measured the direction changing 0 times in 22,296 epochs, so this runs
 * once per execute and the bracket is not on any hot path.
 *
 * The vector is normalised because the hardware normalises nothing: the software
 * path rescales the transformed light to length 127 before the dot, and the
 * joint modelviews are rigid, so normalising before the transform is the same
 * thing. It is negated because the hardware's diffuse term is max(0, -L.N)
 * where the software clamps a positive L.N.
 *
 * Deliberately outside .itcm.native_fighter and noinline -- the shade function
 * is ITCM-resident and full, and the region overflowed once already this task. */
static void __attribute__((noinline)) ndsRendererR2WriteLightVector(
    const NDSRendererStats *stats)
{
    s32 x = stats->light_dir_x;
    s32 y = stats->light_dir_y;
    s32 z = stats->light_dir_z;
    s64 length_squared = ((s64)x * x) + ((s64)y * y) + ((s64)z * z);
    s32 nx = 0;
    s32 ny = 0;
    s32 nz = 0;

    /* The DS hardware square-root and division units, not sqrtf. Partly because
     * this runs twice a frame and they are free here, and partly because
     * check-gbi-decode-fixtures asserts the renderer holds exactly one sqrtf
     * site, isolated inside ndsRendererHardwarePrepareLitDirection -- a second
     * one is how the software light normalization creeps back in. */
    if (length_squared > 0)
    {
        u32 length = ndsR2HwMathSqrt64((u64)length_squared);

        if (length != 0u)
        {
            nx = (s32)ndsR2HwMathDiv64(-(s64)x * 511, (s32)length);
            ny = (s32)ndsR2HwMathDiv64(-(s64)y * 511, (s32)length);
            nz = (s32)ndsR2HwMathDiv64(-(s64)z * 511, (s32)length);
        }
    }

    ndsRendererHardwareEndBatch();
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    GFX_LIGHT_VECTOR = NDS_R2_NORMAL_PACK((int)nx, (int)ny, (int)nz);
    glPopMatrix(1);
    NDS_FIGHTER_PACKET_HOOK(ndsFighterPacketRecordLightVector(
        NDS_R2_NORMAL_PACK((int)nx, (int)ny, (int)nz)));
    gNdsR2LightVectorWrites++;
    sNdsR2LightVectorWritten = 1u;
}

static u16 NDS_R2_ITCM_PACK2_CODE ndsRendererR2MaterialColor15(
    u32 light_color, u32 material_color, u32 use_material, u32 color_modulate)
{
    u32 r = ndsRendererR2MaterialChannel(
        (light_color >> 24) & 0xffu, (material_color >> 24) & 0xffu,
        use_material);
    u32 g = ndsRendererR2MaterialChannel(
        (light_color >> 16) & 0xffu, (material_color >> 16) & 0xffu,
        use_material);
    u32 b = ndsRendererR2MaterialChannel(
        (light_color >> 8) & 0xffu, (material_color >> 8) & 0xffu,
        use_material);

    return ndsRendererHardwareModulatePackedColor(
        RGB15(r, g, b), color_modulate);
}
#endif

#if NDS_R2_FIGHTER_EPOCH_STATE_PROOF
/* R2-03 E34. Decides whether E26's baked per-epoch state is even possible.
 *
 * E26 proposes replacing the 194.4 state-delta applications a frame with a
 * table indexed by epoch. That is sound only if the state an epoch hands to its
 * runs is a function of the epoch index. It may not be:
 * ndsRendererNativeApplyMaterial writes prim/env/blend colour and texture state
 * from `input->materials[]`, which is live, so a material applied in epoch N
 * survives into epoch N+1's state through any field N+1's before-span does not
 * itself rewrite. E26's §2a correction saw the material as a per-epoch problem;
 * this is the harder version, where contamination propagates forward.
 *
 * `noinline` and deliberately outside .itcm.native_fighter: ITCM has ~1 KB free
 * and E16 has already been caught overflowing it with an inline probe. */
#define NDS_R2_EPOCH_STATE_MAX 64u
#define NDS_R2_EPOCH_TILE_MASK 7u
#define NDS_R2_EPOCH_HASH(hash, value) \
    ((hash) = (((hash) ^ (u32)(value)) * 16777619u))
static u32 sNdsR2EpochStateHash[NDS_R2_EPOCH_STATE_MAX];
static u8 sNdsR2EpochStateSeen[NDS_R2_EPOCH_STATE_MAX];
static u8 sNdsR2EpochStateEverChanged[NDS_R2_EPOCH_STATE_MAX];
u32 gNdsR2EpochStateSamples;
u32 gNdsR2EpochStateChanges;
u32 gNdsR2EpochStateUnstableEpochs;

static void __attribute__((noinline))
ndsRendererR2EpochStateProof(u32 epoch_index, const NDSRendererStats *stats)
{
    u32 hash;

    if (epoch_index >= NDS_R2_EPOCH_STATE_MAX)
    {
        return;
    }
    /* ndsRendererSemanticSourceStateHash is behind PROFILE_LEVEL >= 2 and the
     * tick-HUD build is level 0, so hash the fields E26 would actually bake --
     * the ones PrepareProductionRun and the shade read -- rather than the whole
     * semantic state. Narrower is also the right question: a field nothing reads
     * moving does not stop the fold. */
    hash = 2166136261u;
    NDS_R2_EPOCH_HASH(hash, stats->geometry_mode);
    NDS_R2_EPOCH_HASH(hash, stats->othermode_h);
    NDS_R2_EPOCH_HASH(hash, stats->othermode_l);
    NDS_R2_EPOCH_HASH(hash, stats->texture_combine_w0);
    NDS_R2_EPOCH_HASH(hash, stats->texture_combine_w1);
#if NDS_R2_FIGHTER_EPOCH_STATE_PROOF < 2
    /* Level 2 drops the two colours the material writes live. If the change
     * count goes to zero, they are the whole of the 0.48% and the fold is clean
     * with colour as its only runtime input. */
    NDS_R2_EPOCH_HASH(hash, stats->env_color);
    NDS_R2_EPOCH_HASH(hash, stats->prim_color);
#endif
    NDS_R2_EPOCH_HASH(hash, stats->texture_state_flags);
    NDS_R2_EPOCH_HASH(hash, stats->texture_tile);
    NDS_R2_EPOCH_HASH(hash, stats->texture_on);
    NDS_R2_EPOCH_HASH(hash, stats->texture_scale_s);
    NDS_R2_EPOCH_HASH(hash, stats->texture_scale_t);
    NDS_R2_EPOCH_HASH(hash, stats->light_color_1);
    NDS_R2_EPOCH_HASH(hash, stats->light_color_2);
    NDS_R2_EPOCH_HASH(hash, (u32)stats->light_dir_x);
    NDS_R2_EPOCH_HASH(hash, (u32)stats->light_dir_y);
    NDS_R2_EPOCH_HASH(hash, (u32)stats->light_dir_z);
    {
        const u32 *tile =
            (const u32 *)&stats->texture_tiles[stats->texture_tile &
                                               NDS_R2_EPOCH_TILE_MASK];
        u32 w;

        for (w = 0u; w < (sizeof(NDSRendererTileState) / sizeof(u32)); w++)
        {
            NDS_R2_EPOCH_HASH(hash, tile[w]);
        }
    }
    gNdsR2EpochStateSamples++;
    if (sNdsR2EpochStateSeen[epoch_index] == 0u)
    {
        sNdsR2EpochStateSeen[epoch_index] = 1u;
        sNdsR2EpochStateHash[epoch_index] = hash;
        return;
    }
    if (sNdsR2EpochStateHash[epoch_index] != hash)
    {
        gNdsR2EpochStateChanges++;
        if (sNdsR2EpochStateEverChanged[epoch_index] == 0u)
        {
            sNdsR2EpochStateEverChanged[epoch_index] = 1u;
            gNdsR2EpochStateUnstableEpochs++;
        }
        sNdsR2EpochStateHash[epoch_index] = hash;
    }
}
#endif

/* R2-03 E28. Once E16 moved the fighter's diffuse term onto the geometry
 * engine, the software light direction and the shade LUT became dead: both are
 * read only inside the per-dense-vertex loop that `hardware_lit` skips. The
 * exception is the M2 detailed ledger, which reads
 * state->prepared_light_direction after the shade returns, so it keeps the
 * software preparation.
 *
 * NDS_R2_FIGHTER_SHADE_PROOF hashes `prepared_direction`; with the skip active
 * that field is NULL, so the proof is only comparable against another skipped
 * build. It is a lab flag and never ships with this one.
 *
 * The #define itself now lives above ndsRendererNativeBindProductionRoot,
 * because that function's matrix copies are gated on it. */

static s32 NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativeShadeProductionActions(
    const NDSNativeEpoch *epoch,
    u32 epoch_policy,
    u32 packet_mode,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const NDSNativeDirectPolicy *policy =
        &sNdsNativeFighterDirectPolicies[
            epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK];
    const NDSRendererHardwareLightDirection *prepared_direction = NULL;
    const u32 *shade_lut = NULL;
    /* R2-03 E47. The native owner baked both of these; the generic path derives
     * both from `stats` per draw, and on hitlag frames they disagree.
     *
     * `ndsRendererHardwareColorSource` picks `env_color` whenever the combiner
     * outputs ENVIRONMENT and `prim_color` only otherwise, and returns 0 when
     * the epoch has no combiner at all. The native owner used `prim_color`
     * unconditionally. `ndsRendererHardwareUseMaterialColor` is likewise a
     * predicate over the live combiner -- for a lit-shade combine it reduces to
     * `UsesLitPrimitiveModulate(stats)` -- while the policy flag is fixed at
     * generation time.
     *
     * E34 measured `prim_color`/`env_color` as the only per-epoch state that
     * varies at runtime, and Task 39's hurt flash is what varies them, so
     * hitlag frames are exactly where a baked answer goes wrong: the struck
     * fighter came out dark maroon where the generic path draws light grey
     * (E32, `artifacts/visibility/e32-compare-480.png`). E41 had already
     * excluded the fold arithmetic and E16's hardware lighting by three-way
     * capture, and E36 excluded `color_modulate`.
     *
     * This runs once per epoch (46.4/frame), not per vertex, so deriving it is
     * not on any hot path. A rendering-correctness fix owed regardless of E32 --
     * E32 only decides whether these frames reach the native owner at all. */
#if NDS_R2_MATERIAL_DYNAMIC
    u32 use_material =
        (ndsRendererHardwareUseMaterialColor(stats) != FALSE) ?
            NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u;
    u32 material_color = ndsRendererHardwareColorSource(stats);
#else
    u32 use_material =
        policy->vertex_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL;
    u32 material_color = (use_material != 0u) ? stats->prim_color : 0u;
#endif
    u32 action_offset;
    /* Identical to `prepared_direction != NULL` in every build that prepares
     * the direction, and still correct in the build that skips it. */
    u32 epoch_lit = FALSE;
#if NDS_R2_FIGHTER_HW_LIGHT
    u32 hardware_lit = FALSE;
#endif

#if NDS_R2_MATERIAL_DYNAMIC
    (void)policy;
#endif
#if NDS_R2_UNLIT_VERTEX_EPOCH
    /* R2-03 E49. Same two predicates the generic path keys on, evaluated once
     * per epoch (46.4/frame) rather than per vertex. */
    sNdsR2EpochUnlitVertexColor =
        ((ndsRendererHardwareUseVertexColor(stats) != FALSE) &&
         (ndsRendererHardwareUseMaterialColor(stats) == FALSE)) ? 1u : 0u;
#endif
    if (epoch->action_count == 0u)
    {
        return TRUE;
    }
    ndsRendererNativeSourceBoundary(state);
#if NDS_R2_FLASH_PROBE
    /* R2-03 E59, owner side. Same fields at the native owner's shade entry, so
     * one frame's snapshot carries BOTH paths: E54 showed only one fighter falls
     * back per hitlag frame, so on frame 911 the struck fighter runs generic
     * while the other still runs the owner. Slots 12..14 vs 15..18 is therefore
     * a same-frame comparison, not a cross-frame one. */
    gNdsR2FlashLive[15] = stats->light_color_1;
    gNdsR2FlashLive[16] = stats->light_color_2;
    gNdsR2FlashLive[17] = stats->light_color_mask;
    gNdsR2FlashLive[18] = stats->geometry_mode;
    gNdsR2FlashLive[19]++;
#endif
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        epoch_lit = TRUE;
#if !NDS_R2_SHADE_SKIP_SOFT_LIGHT
        if (state->prepared_light_direction_valid == 0u)
        {
            ndsRendererHardwarePrepareLitDirection(
                stats, &state->modelview,
                &state->prepared_light_direction);
            state->prepared_light_direction_valid = TRUE;
        }
        prepared_direction = &state->prepared_light_direction;
        if ((stats->light_color_mask &
             (NDS_RENDERER_LIGHT_COLOR_1_MASK |
              NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
            (NDS_RENDERER_LIGHT_COLOR_1_MASK |
             NDS_RENDERER_LIGHT_COLOR_2_MASK))
        {
            if (packet_mode != 0u)
            {
                shade_lut = ndsRendererHardwareFindLightShadeLut(
                    stats->light_color_1, stats->light_color_2);
                if (shade_lut == NULL)
                {
                    return FALSE;
                }
            }
            else
            {
                shade_lut = ndsRendererHardwareGetLightShadeLut(
                    stats->light_color_1, stats->light_color_2);
            }
        }
#endif
    }

#if NDS_R2_FIGHTER_SHADE_PROOF
    {
        u32 hash = sNdsR2ShadeFrameInputHash;
        u32 i;

        NDS_R2_SHADE_HASH(hash, epoch_policy);
        NDS_R2_SHADE_HASH(hash, packet_mode);
        NDS_R2_SHADE_HASH(hash, epoch->first_action);
        NDS_R2_SHADE_HASH(hash, epoch->action_count);
        NDS_R2_SHADE_HASH(hash, stats->prim_color);
        NDS_R2_SHADE_HASH(hash, stats->geometry_mode);
        NDS_R2_SHADE_HASH(hash, stats->light_dir_mask);
        NDS_R2_SHADE_HASH(hash, stats->light_color_mask);
        NDS_R2_SHADE_HASH(hash, stats->light_color_1);
        NDS_R2_SHADE_HASH(hash, stats->light_color_2);
        NDS_R2_SHADE_HASH(hash, state->color_modulate);
        NDS_R2_SHADE_HASH(hash, (prepared_direction != NULL) ? 1u : 0u);
        NDS_R2_SHADE_HASH(hash, (shade_lut != NULL) ? 1u : 0u);
        if (prepared_direction != NULL)
        {
            const u32 *direction = (const u32 *)prepared_direction;

            for (i = 0u;
                 i < sizeof(NDSRendererHardwareLightDirection) / sizeof(u32);
                 i++)
            {
                NDS_R2_SHADE_HASH(hash, direction[i]);
            }
        }
        sNdsR2ShadeFrameInputHash = hash;
        sNdsR2ShadeFrameCallCount++;
    }
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    if (epoch_lit != FALSE)
    {
        gNdsR2ShadeLitEpochs++;
    }
    else
    {
        gNdsR2ShadeUnlitEpochs++;
    }
    if (shade_lut != NULL) { gNdsR2ShadeLutEpochs++; }
    if (use_material != 0u) { gNdsR2ShadeMaterialEpochs++; }
    ndsRendererR2E16aLightCensus(stats, material_color, state->color_modulate);
#endif
#if NDS_R2_FIGHTER_HW_LIGHT
    /* R2-03 E16. The per-dense-vertex shading below becomes one register write:
     * the engine evaluates the dot product per vertex from GFX_NORMAL.
     *
     * This sets a flag rather than returning, because the action walk below is
     * not only shading -- it also advances stats->vertex_count and
     * gNdsRendererProfileSourceVertexLoadCount. An earlier revision returned
     * here and drew identical geometry while leaving that load count at zero,
     * which the Boundary harness caught as the complete-stage owner having
     * entered the generic transform path. Only the inner loop is skipped. */
    if ((epoch_lit != FALSE)
#if NDS_R2_UNLIT_VERTEX_EPOCH
        /* The polygon attribute drops POLY_FORMAT_LIGHT0 for this epoch, so the
         * engine ignores diffuse/ambient -- writing them would be dead FIFO
         * traffic, and the light vector write is a push/identity/pop bracket. */
        && (sNdsR2EpochUnlitVertexColor == 0u)
#endif
        )
    {
        u32 diffuse;
        u32 ambient;

        if (sNdsR2LightVectorWritten == 0u)
        {
            ndsRendererR2WriteLightVector(stats);
        }
        diffuse = ndsRendererR2MaterialColor15(
            stats->light_color_1, material_color, use_material,
            state->color_modulate);
        ambient = ndsRendererR2MaterialColor15(
            stats->light_color_2, material_color, use_material,
            state->color_modulate);

        ndsRendererHardwareWriteDiffuseAmbient(diffuse | (ambient << 16));
        NDS_FIGHTER_PACKET_HOOK(
            ndsFighterPacketRecordDiffuseAmbient(
                diffuse | (ambient << 16),
                stats->light_color_1, stats->light_color_2,
                material_color, use_material));
        hardware_lit = TRUE;
    }
#endif
#if NDS_R2_FIGHTER_SHADE_SKIP
    /* R2-03 E18. Prices E16's ceiling directly instead of inferring it from a
     * bracket that also contains the epoch preamble. Skips the per-vertex
     * lighting outright, leaving sNdsNativeFighterPreparedDense holding whatever
     * the previous frame left -- so the fighters draw with stale, visibly wrong
     * colours. Lab only; the engagement proof is the screenshot. */
    return TRUE;
#endif
    for (action_offset = 0u;
         action_offset < epoch->action_count;
         action_offset++)
    {
        u32 action_index = epoch->first_action + action_offset;
        const NDSNativeVertexAction *action =
            &sNdsNativeFighterActiveTables->vertex_actions[action_index];
#if !NDS_R2_FIGHTER_HW_LIGHT
        u32 span =
            sNdsNativeFighterActiveTables->action_dense_spans[action_index];
        u32 dense_first = span & NDS_NATIVE_DENSE_ID_MASK;
        u32 dense_count = span >> NDS_NATIVE_DENSE_SPAN_COUNT_SHIFT;
        u32 dense_offset;
#endif

        if (action->kind == NDS_NATIVE_VERTEX_BLOCK)
        {
            u32 vertex_end = (u32)action->index + (u32)action->count;

            if (vertex_end > stats->vertex_count)
            {
                stats->vertex_count = vertex_end;
            }
            if (packet_mode == 0u)
            {
                sNdsRendererHardwareSourceVertexLoadCount += action->count;
            }
        }
#if NDS_R2_FIGHTER_HW_LIGHT
        /* R2-03 E29 / P2-2. The software shade span has no consumer in the
         * hardware-light owner. Do not even retain its generated Low-detail
         * alias/span tables in this configuration; the action itself remains
         * because source vertex-count accounting is part of the contract. */
        (void)hardware_lit;
#else
        for (dense_offset = 0u;
             dense_offset < dense_count;
             dense_offset++)
        {
            u32 dense_id = dense_first + dense_offset;
            u32 color_source =
                sNdsNativeFighterActiveTables->dense_color_source[dense_id];

            if (color_source != dense_id)
            {
#if NDS_TASK91_DRAW_PHASE_CENSUS
                gNdsR2ShadeVerticesCopied++;
#endif
                sNdsNativeFighterActiveTables->prepared_dense[
                    dense_id].shaded_rgba =
                    sNdsNativeFighterActiveTables->prepared_dense[
                        color_source].shaded_rgba;
            }
            else
            {
#if NDS_TASK91_DRAW_PHASE_CENSUS
                gNdsR2ShadeVerticesLit++;
#endif
                const NDSNativeDenseVertex *dense =
                    &sNdsNativeFighterActiveTables->dense_vertices[dense_id];
                NDSRendererInputVertex input;

                input.r = (u8)(dense->rgba >> 24);
                input.g = (u8)(dense->rgba >> 16);
                input.b = (u8)(dense->rgba >> 8);
                input.a = (u8)dense->rgba;
                if (shade_lut != NULL)
                {
                    sNdsNativeFighterActiveTables->prepared_dense[
                        dense_id].shaded_rgba =
                        ndsRendererHardwareLitShadeColorLut(
                            &input, prepared_direction, shade_lut);
                }
                else
                {
                    sNdsNativeFighterActiveTables->prepared_dense[
                        dense_id].shaded_rgba =
                        ndsRendererHardwareLitShadeColorPrepared(
                            stats, &input, prepared_direction);
                }
            }
            {
                NDSNativePreparedDenseVertex *prepared =
                    &sNdsNativeFighterActiveTables->prepared_dense[dense_id];
                u32 color = prepared->shaded_rgba;

                if (use_material != 0u)
                {
                    u32 r = ndsRendererHardwareScaleMaterialChannel5(
                        (color >> 24) & 0xffu,
                        (material_color >> 24) & 0xffu);
                    u32 g = ndsRendererHardwareScaleMaterialChannel5(
                        (color >> 16) & 0xffu,
                        (material_color >> 16) & 0xffu);
                    u32 b = ndsRendererHardwareScaleMaterialChannel5(
                        (color >> 8) & 0xffu,
                        (material_color >> 8) & 0xffu);

                    prepared->packed_color = RGB15(r, g, b);
                }
                else
                {
                    prepared->packed_color =
                        RGB15((color >> 27) & 0x1fu,
                              (color >> 19) & 0x1fu,
                              (color >> 11) & 0x1fu);
                }
                prepared->packed_color =
                    ndsRendererHardwareModulatePackedColor(
                        prepared->packed_color, state->color_modulate);
            }
        }
#endif /* !NDS_R2_FIGHTER_HW_LIGHT */
    }
    return TRUE;
}

/* sNdsNativeFighterRuns[67]. Shared by the E5 proof and the E12 memo, so it
 * lives outside both flags' guards. */
#define NDS_R2_RUN_MEMO_MAX 67u
#if NDS_R2_FIGHTER_RUN_PROOF
/* R2-03 E5 falsifier. The switch plan's section 7 for this phase asks for a
 * per-epoch generated submit "consuming only baked facts ... no
 * PrepareProductionRun policy re-checks, no per-frame texture identity proof".
 * That is E1a's cut moved to the fighter, and R2-02 E1a was worth 94,784
 * ticks/frame on a table of the same shape. The question is whether those facts
 * are constant per epoch, and the fighter differs from the stage in a way that
 * could decide it: its materials are live (Task 39's hurt flash writes colour)
 * where Dream Land's are not.
 *
 * Hooked here rather than in the hierarchy preflight. Canonical mode 9
 * (NATIVE_COMPLETE_STAGE) selects native_owner_production_mode and leaves
 * native_owner_hierarchy_mode FALSE, so hierarchy_runs[] is mode 7's table and
 * the shipping build never writes it -- an earlier revision of this falsifier
 * hooked that table and honestly reported zero calls. This is the function the
 * live path calls, once per run per frame, with hierarchy_run == NULL: the
 * facts are recomputed and consumed on the spot, never stored. That recompute
 * is the cost R2-03 exists to remove.
 *
 * Three hashes, because they imply different cuts and R2-02 E3 is the standing
 * reminder to answer a data question with a counter rather than an argument.
 *
 *   STABLE constant -> the texture and geometry facts are a per-epoch memo,
 *     which is exactly E1a.
 *   STABLE constant but MATERIAL moves -> bake the table and write colour per
 *     frame. Still E1a's shape: it kept the table and recomputed the one field
 *     that moved.
 *   STABLE moves -> refuted for this build, and the cost has to come out of the
 *     per-run work itself.
 *
 * FULL adds the texture cache entry pointer, which rotates for reasons
 * unrelated to what is drawn; STABLE omits it deliberately. */
volatile u32 gNdsR2RunFullHash;
volatile u32 gNdsR2RunFullChangeCount;
volatile u32 gNdsR2RunStableHash;
volatile u32 gNdsR2RunStableChangeCount;
volatile u32 gNdsR2RunMaterialHash;
volatile u32 gNdsR2RunMaterialChangeCount;
volatile u32 gNdsR2RunCallCount;
volatile u32 gNdsR2RunFrameCount;
static u32 sNdsR2RunFrameFullHash = 2166136261u;
static u32 sNdsR2RunFrameStableHash = 2166136261u;
static u32 sNdsR2RunFrameMaterialHash = 2166136261u;
static u32 sNdsR2RunFrameCallCount;

/* The per-frame hashes fold every prepared run in submission order, so they
 * cannot separate "a run's facts changed" from "a different set of runs was
 * prepared". The first sweep saw exactly two whole-frame values with 67 and 37
 * runs, and 67 is the whole of sNdsNativeFighterRuns[] -- which points at the
 * set changing, not the facts. That distinction decides the memo's key, so
 * measure it directly: keep one digest per run index and count the calls whose
 * facts differ from the copy already stored for that index.
 *
 * MISS == 0 after the fills means the facts are a property of the run index
 * alone, the memo needs no per-frame revalidation, and R2-03's baked table can
 * be generated rather than discovered. */
volatile u32 gNdsR2RunMemoHitCount;
volatile u32 gNdsR2RunMemoMissCount;
volatile u32 gNdsR2RunMemoFillCount;
volatile u32 gNdsR2RunMemoOutOfRange;
volatile u32 gNdsR2RunEntryCount;
/* E11. Level 2 splits the call into the four spans a memo would treat
 * differently: validation (skippable outright -- E5 proved it never rejects),
 * texture prepare (a live GX bind a memo must still pay, split by whether the
 * caller's per-DObj reuse flag was already set), the UV loop (E5 measured it
 * tiny), and the tail. Sizing before designing, per E8. */
volatile u32 gNdsR2RunValidateTicks;
volatile u32 gNdsR2RunTexPrepTicks;
volatile u32 gNdsR2RunTexPrepCount;
volatile u32 gNdsR2RunTexReuseTicks;
volatile u32 gNdsR2RunTexReuseCount;
volatile u32 gNdsR2RunUvTicks;
volatile u32 gNdsR2RunTailTicks;
volatile u32 gNdsR2RunSuccessCount;
static u32 sNdsR2RunMemoHash[NDS_R2_RUN_MEMO_MAX];
static u8 sNdsR2RunMemoValid[NDS_R2_RUN_MEMO_MAX];



/* The facts being constant is not on its own a licence to skip the UV loop.
 * 28 of the 541 dense vertices belong to more than one run (13..18 overlap), so
 * each sharing run rewrites them and the emit between two prepares reads
 * whatever the last one left. Skipping is safe only if those writes are
 * idempotent -- if no write ever changes the value it lands on.
 *
 * Measure that directly rather than infer it from the per-run digests: compare
 * before storing, and count the writes that actually changed something. CHANGE
 * settling to zero after the first fill means the whole loop is recomputing
 * values that are already there. */
/* PrepareProductionRun is NDS_RENDERER_NATIVE_FIGHTER_CODE, i.e. ITCM. Inlining
 * a proof body there overflowed the region by 100 bytes, so take the Task 29
 * census treatment: out of line, size-optimised, and in its own .text section.
 * The extra branch per run is measurement cost that never ships. */
#define NDS_R2_RUN_PROOF_CODE \
    __attribute__((noinline, noclone, cold, optimize("Os"), \
                   section(".text.r2_run_proof")))

#define NDS_R2_UV_PROOF_MAX 541u
volatile u32 gNdsR2UvWriteCount;
volatile u32 gNdsR2UvChangeCount;
volatile u32 gNdsR2UvFillCount;
volatile u32 gNdsR2UvOutOfRange;
static s16 sNdsR2UvS[NDS_R2_UV_PROOF_MAX];
static s16 sNdsR2UvT[NDS_R2_UV_PROOF_MAX];
static u8 sNdsR2UvValid[NDS_R2_UV_PROOF_MAX];

static void NDS_R2_RUN_PROOF_CODE ndsRendererR2FighterUvProofWrite(
    u32 dense_id,
    s16 s,
    s16 t)
{
    gNdsR2UvWriteCount++;
    if (dense_id >= NDS_R2_UV_PROOF_MAX)
    {
        gNdsR2UvOutOfRange++;
    }
    else if (sNdsR2UvValid[dense_id] == 0u)
    {
        sNdsR2UvValid[dense_id] = 1u;
        sNdsR2UvS[dense_id] = s;
        sNdsR2UvT[dense_id] = t;
        gNdsR2UvFillCount++;
    }
    else if ((sNdsR2UvS[dense_id] != s) || (sNdsR2UvT[dense_id] != t))
    {
        sNdsR2UvS[dense_id] = s;
        sNdsR2UvT[dense_id] = t;
        gNdsR2UvChangeCount++;
    }
}

#define NDS_R2_RUN_HASH(hash, value)     ((hash) = (((hash) ^ (u32)(value)) * 16777619u))

static void NDS_R2_RUN_PROOF_CODE ndsRendererR2FighterRunProofCall(
    u32 run_index,
    const NDSRendererTraversalState *state,
    const NDSRendererHardwareResolvedTexture *resolved)
{
    u32 full = sNdsR2RunFrameFullHash;
    u32 stable = sNdsR2RunFrameStableHash;
    u32 material = sNdsR2RunFrameMaterialHash;
    u32 fields[13];
    u32 i;

    fields[0] = run_index;
    fields[1] = state->texture_prepare_name;
    fields[2] = resolved->params;
    fields[3] = resolved->format;
    fields[4] = resolved->width;
    fields[5] = resolved->height;
    fields[6] = state->texture_prepare_poly_fmt;
    fields[7] = state->texture_prepare_scale_s;
    fields[8] = state->texture_prepare_scale_t;
    fields[9] = state->texture_prepare_origin_s;
    fields[10] = state->texture_prepare_origin_t;
    fields[11] = (u32)state->texture_prepare_offset;
    fields[12] = state->texture_prepare_vertex_flags;
    for (i = 0u; i < 13u; i++)
    {
        NDS_R2_RUN_HASH(full, fields[i]);
        NDS_R2_RUN_HASH(stable, fields[i]);
    }
    NDS_R2_RUN_HASH(full, state->texture_prepare_enabled);
    NDS_R2_RUN_HASH(stable, state->texture_prepare_enabled);
    /* The live field the STABLE hash deliberately excludes. */
    NDS_R2_RUN_HASH(material, run_index);
    NDS_R2_RUN_HASH(material, state->texture_prepare_material_color);
    NDS_R2_RUN_HASH(full, state->texture_prepare_material_color);
    /* A pointer into the hardware texture cache, which rotates for reasons
     * unrelated to what is drawn. */
    NDS_R2_RUN_HASH(full, (u32)(uintptr_t)resolved->entry);
    {
        /* Same field set as STABLE plus the colour, but seeded per run so the
         * digest describes this run alone rather than the frame's sequence. */
        u32 own = 2166136261u;

        for (i = 0u; i < 13u; i++)
        {
            NDS_R2_RUN_HASH(own, fields[i]);
        }
        NDS_R2_RUN_HASH(own, state->texture_prepare_enabled);
        NDS_R2_RUN_HASH(own, state->texture_prepare_material_color);
        if (run_index >= NDS_R2_RUN_MEMO_MAX)
        {
            gNdsR2RunMemoOutOfRange++;
        }
        else if (sNdsR2RunMemoValid[run_index] == 0u)
        {
            sNdsR2RunMemoValid[run_index] = 1u;
            sNdsR2RunMemoHash[run_index] = own;
            gNdsR2RunMemoFillCount++;
        }
        else if (sNdsR2RunMemoHash[run_index] != own)
        {
            sNdsR2RunMemoHash[run_index] = own;
            gNdsR2RunMemoMissCount++;
        }
        else
        {
            gNdsR2RunMemoHitCount++;
        }
    }
    sNdsR2RunFrameFullHash = full;
    sNdsR2RunFrameStableHash = stable;
    sNdsR2RunFrameMaterialHash = material;
    sNdsR2RunFrameCallCount++;
}

static void ndsRendererR2FighterRunProofFrame(void)
{
    if (gNdsR2RunFrameCount != 0u)
    {
        if (sNdsR2RunFrameFullHash != gNdsR2RunFullHash)
        {
            gNdsR2RunFullChangeCount++;
        }
        if (sNdsR2RunFrameStableHash != gNdsR2RunStableHash)
        {
            gNdsR2RunStableChangeCount++;
        }
        if (sNdsR2RunFrameMaterialHash != gNdsR2RunMaterialHash)
        {
            gNdsR2RunMaterialChangeCount++;
        }
    }
    gNdsR2RunFullHash = sNdsR2RunFrameFullHash;
    gNdsR2RunStableHash = sNdsR2RunFrameStableHash;
    gNdsR2RunMaterialHash = sNdsR2RunFrameMaterialHash;
    gNdsR2RunCallCount = sNdsR2RunFrameCallCount;
    gNdsR2RunFrameCount++;
    sNdsR2RunFrameFullHash = 2166136261u;
    sNdsR2RunFrameStableHash = 2166136261u;
    sNdsR2RunFrameMaterialHash = 2166136261u;
    sNdsR2RunFrameCallCount = 0u;
}
#endif

/* E12 counters live outside the flag so a build with the memo off reads five
 * honest zeros instead of whatever the linker left at those addresses. Two
 * probes this cycle read an ARM opcode (0xEA80003B) out of a garbage-collected
 * counter and had to be re-run; the verifier reads these, so it cannot afford
 * that ambiguity. Only the increments are conditional.
 *
 * `volatile` is not enough. It stops the compiler from folding accesses, not
 * the linker from dropping an object nothing references -- and at MEMO=1
 * nothing writes VerifyFail, because only the level-2 verify does. `retain`
 * emits SHF_GNU_RETAIN so --gc-sections keeps it and a zero reads as zero. */
#define NDS_R2_TEXMEMO_COUNTER __attribute__((used, retain))
volatile u32 NDS_R2_TEXMEMO_COUNTER gNdsR2TexMemoHitCount;
volatile u32 NDS_R2_TEXMEMO_COUNTER gNdsR2TexMemoMissCount;
volatile u32 NDS_R2_TEXMEMO_COUNTER gNdsR2TexMemoFillCount;
volatile u32 NDS_R2_TEXMEMO_COUNTER gNdsR2TexMemoStaleCount;
volatile u32 NDS_R2_TEXMEMO_COUNTER gNdsR2TexMemoVerifyFail;

#if NDS_R2_FIGHTER_RUN_MEMO
/* E12. E5 proved every field below is invariant in run_index over a whole
 * canonical match, and E11 measured what recomputing them costs: 45.3 of 60.9
 * calls take the full texture prepare at 1,013 ticks, because the caller resets
 * texture_prepare_valid per DObj. The work skipped is SyncTextureTile, the
 * ~30-field key build, its hash, and the cache lookup.
 *
 * What is NOT skipped, because the resolver's cache-hit tail has live effects
 * the frame depends on: refreshing last_used_frame (this is the eviction LRU --
 * dropping it would let the entry the memo points at be reclaimed), the name and
 * param binds, the active-entry pointer, the pinned static-texture hit, and the
 * four stats fields.
 *
 * E5 deliberately excluded resolved->entry from its STABLE hash: "a pointer into
 * the hardware texture cache, which rotates for reasons unrelated to what is
 * drawn". So identity being stable is not residency being stable, and the memo
 * revalidates before trusting itself using readiness, name, and the cache key
 * generation. The generation is the identity fence: libnds may recycle a
 * deleted GL name for different texels in the same slot. A stale entry falls
 * through to the full path and refills. */
typedef struct NDSR2RunTextureMemo
{
    /* Slot index, not a pointer: sNdsRendererHardwareActiveTextureEntry is a
     * const pointer and the memo must refresh last_used_frame, so an index into
     * the cache array is both writable and cheaper to validate than a cast. */
    u32 entry_generation;
    /* `run_index` identifies immutable generated geometry, not a live fighter.
     * Mario/Mario (or Fox/Fox) therefore executes the same run indices twice in
     * one frame.  The full texture resolver keys costume-sensitive bakes on live
     * PRIM/ENV state; without the owner key below, the second fighter could reuse
     * the first fighter's baked texture merely because the cache entry was still
     * resident.  The key also separates High/Low generated programs and later
     * roster owners.  Texture-cache `key_generation` below is the scene/VRAM
     * lifetime fence, so duplicating taskman generation here would add a hot
     * compare without adding ownership information. */
    u32 owner_key;
    u16 name;
    u16 width;
    u16 height;
    u16 scale_s;
    u16 scale_t;
    u16 origin_s;
    u16 origin_t;
    s16 offset;
    u8 slot_plus1;
    u8 format;
    u8 valid;
} NDSR2RunTextureMemo;
_Static_assert(sizeof(NDSR2RunTextureMemo) == 28u,
               "run texture memo must retain the compact DS field layout");

/* One live fighter instance per source player slot.  A single row per run was
 * correct for one fighter but pathological for multiplayer: slot 1 replaced
 * slot 0's owner_key, slot 2 replaced slot 1's, and so on, so the next frame
 * every fighter missed and rebuilt the same resident texture identity again.
 *
 * texture_memo_owner_key already carries the source player slot in bits 10:9
 * (renderer_adapter_fighter.c) and retains owner/detail/costume/shade in the
 * row itself.  Indexing by those two slot bits therefore gives the four live
 * instances independent residency records without weakening any identity
 * check. */
#define NDS_R2_RUN_TEXMEMO_PLAYER_COUNT 4u
static NDSR2RunTextureMemo
    sNdsR2RunTextureMemo[NDS_R2_RUN_MEMO_MAX]
                          [NDS_R2_RUN_TEXMEMO_PLAYER_COUNT];

static inline NDSR2RunTextureMemo *ndsRendererR2RunTextureMemoFor(
    u32 run_index, u32 owner_key)
{
    return &sNdsR2RunTextureMemo[run_index][(owner_key >> 9) & 3u];
}

/* The cheap tail of the resolver's cache-hit path, replayed from the memo. */
static s32 __attribute__((noinline)) ndsRendererR2RunTextureMemoApply(
    u32 run_index,
    NDSRendererStats *stats,
    u32 *texture_name,
    u32 *scale_s,
    u32 *scale_t,
    u32 *origin_s,
    u32 *origin_t,
    s32 *offset)
{
    NDSR2RunTextureMemo *memo;
    NDSRendererHardwareTextureCacheEntry *entry;

    if (run_index >= NDS_R2_RUN_MEMO_MAX)
    {
        return FALSE;
    }
    memo = ndsRendererR2RunTextureMemoFor(
        run_index, sNdsNativeFighterOwnerExecution.texture_memo_owner_key);
    if (memo->valid == 0u)
    {
        gNdsR2TexMemoMissCount++;
        return FALSE;
    }
    if (memo->owner_key !=
        sNdsNativeFighterOwnerExecution.texture_memo_owner_key)
    {
        /* This is an identity miss, not a stale hardware-cache entry.  Leave
         * the resident texture untouched and let the source-equivalent full
         * resolver below choose the live instance's key, then refill. */
        gNdsR2TexMemoMissCount++;
        return FALSE;
    }
    entry = &sNdsRendererHardwareTextureCache[memo->slot_plus1 - 1u];
    if ((entry->ready == FALSE) || ((u32)entry->name != memo->name) ||
        (entry->key_generation != memo->entry_generation))
    {
        memo->valid = 0u;
        gNdsR2TexMemoStaleCount++;
        return FALSE;
    }

    entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    if (entry->pinned != 0u)
    {
        ndsRendererHardwareBindTextureName(stats, memo->name);
        ndsRendererHardwareApplyTextureParams(entry->params);
        sNdsRendererHardwareActiveTextureEntry = entry;
        ndsRendererHardwareRecordBattleStaticTextureHit(entry);
    }
    else if (sNdsRendererHardwareActiveTextureEntry != entry)
    {
        ndsRendererHardwareBindTextureName(stats, memo->name);
        ndsRendererHardwareApplyTextureParams(entry->params);
        sNdsRendererHardwareActiveTextureEntry = entry;
    }
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = memo->format;
    stats->hardware_texture_width = memo->width;
    stats->hardware_texture_height = memo->height;

    *texture_name = memo->name;
    *scale_s = memo->scale_s;
    *scale_t = memo->scale_t;
    *origin_s = memo->origin_s;
    *origin_t = memo->origin_t;
    *offset = memo->offset;
    gNdsR2TexMemoHitCount++;
    return TRUE;
}

static void __attribute__((noinline)) ndsRendererR2RunTextureMemoFill(
    u32 run_index,
    const NDSRendererStats *stats,
    u32 texture_name,
    u32 scale_s,
    u32 scale_t,
    u32 origin_s,
    u32 origin_t,
    s32 offset)
{
    NDSR2RunTextureMemo *memo;
    const NDSRendererHardwareTextureCacheEntry *entry =
        sNdsRendererHardwareActiveTextureEntry;
    u32 slot;

    /* Only bake an entry the resolver actually landed on. A run whose bind took
     * the stage-site shortcut or the no-texture path leaves no active entry, and
     * baking one from a neighbouring run's state is exactly the class of bug the
     * revalidation above cannot catch. */
    if ((run_index >= NDS_R2_RUN_MEMO_MAX) || (entry == NULL) ||
        (entry->ready == FALSE) || ((u32)entry->name != texture_name) ||
        (texture_name == 0u))
    {
        return;
    }
    slot = (u32)(entry - sNdsRendererHardwareTextureCache);
    if (slot >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT)
    {
        return;
    }
    /* Keeps the level-2 counter linked at level 1. `volatile` stops the
     * compiler folding this away and `retain` was accepted but did not survive
     * this linker, so the symbol needs a genuine reference or --gc-sections
     * drops it -- and the verifier then reads whatever sits at its address
     * (0xEA80003D, an ARM opcode, on the run that caught this). Nine fills a
     * match, so the load/store is free. */
    gNdsR2TexMemoVerifyFail = gNdsR2TexMemoVerifyFail;
    memo = ndsRendererR2RunTextureMemoFor(
        run_index, sNdsNativeFighterOwnerExecution.texture_memo_owner_key);
    /* Preserve the full resolver for values outside the compact memo's
     * representation. This only declines caching; it never truncates a draw.
     * Texture parameters already live in the generation-checked cache entry. */
    if ((slot >= 255u) || (texture_name > 0xffffu) ||
        (stats->hardware_texture_format > 0xffu) ||
        (stats->hardware_texture_width > 0xffffu) ||
        (stats->hardware_texture_height > 0xffffu) ||
        (scale_s > 0xffffu) || (scale_t > 0xffffu) ||
        (origin_s > 0xffffu) || (origin_t > 0xffffu) ||
        (offset < -32768) || (offset > 32767))
    {
        memo->valid = 0u;
        return;
    }
    memo->slot_plus1 = slot + 1u;
    memo->name = texture_name;
    memo->entry_generation = entry->key_generation;
    memo->format = stats->hardware_texture_format;
    memo->width = stats->hardware_texture_width;
    memo->height = stats->hardware_texture_height;
    memo->scale_s = scale_s;
    memo->scale_t = scale_t;
    memo->origin_s = origin_s;
    memo->origin_t = origin_t;
    memo->offset = offset;
    memo->owner_key = sNdsNativeFighterOwnerExecution.texture_memo_owner_key;
    memo->valid = 1u;
    gNdsR2TexMemoFillCount++;
}

#if NDS_R2_FIGHTER_RUN_MEMO >= 2
/* Level 2 never takes the memo branch. It lets the full path run and then asks
 * whether the memo would have answered differently, so a disagreement is
 * counted instead of drawn. E8 was refuted by a key that was subtly incomplete
 * three times running, each caught by a verify arm exactly like this one. */
static void __attribute__((noinline)) ndsRendererR2RunTextureMemoVerify(
    u32 run_index,
    const NDSRendererStats *stats,
    u32 texture_name,
    u32 scale_s,
    u32 scale_t,
    u32 origin_s,
    u32 origin_t,
    s32 offset)
{
    const NDSR2RunTextureMemo *memo;
    const NDSRendererHardwareTextureCacheEntry *entry;

    if (run_index >= NDS_R2_RUN_MEMO_MAX)
    {
        return;
    }
    memo = ndsRendererR2RunTextureMemoFor(
        run_index, sNdsNativeFighterOwnerExecution.texture_memo_owner_key);
    if (memo->valid == 0u)
    {
        return;
    }
    if (memo->owner_key !=
        sNdsNativeFighterOwnerExecution.texture_memo_owner_key)
    {
        /* A different live instance is not a memo disagreement.  Level 2 runs
         * the full resolver precisely so the new identity can replace this row
         * after verification. */
        return;
    }
    entry = &sNdsRendererHardwareTextureCache[memo->slot_plus1 - 1u];
    if ((entry->ready == FALSE) || ((u32)entry->name != memo->name) ||
        (entry->key_generation != memo->entry_generation))
    {
        /* A stale entry is not a mismatch: the live path would have refilled
         * it. Counted separately so the two are never conflated. */
        gNdsR2TexMemoStaleCount++;
        return;
    }
    if ((memo->name != texture_name) ||
        (memo->format != stats->hardware_texture_format) ||
        (memo->width != stats->hardware_texture_width) ||
        (memo->height != stats->hardware_texture_height) ||
        (memo->scale_s != scale_s) || (memo->scale_t != scale_t) ||
        (memo->origin_s != origin_s) || (memo->origin_t != origin_t) ||
        (memo->offset != offset))
    {
        gNdsR2TexMemoVerifyFail++;
    }
    else
    {
        gNdsR2TexMemoHitCount++;
    }
}
#endif
#endif

/* Cycle 110, Requirement 3: the prepared dense UVs are IMMUTABLE fighter state,
 * not per-frame work.
 *
 * Whole-match proof at NDS_R2_FIGHTER_RUN_PROOF=2: the UV loop performed
 * **246,736 writes** and produced **106 distinct dense vertices**, with
 * `gNdsR2UvChangeCount == 0` — not one of those writes ever changed a value —
 * and `gNdsR2UvOutOfRange == 0`, so the proof array covered every id. 154 writes
 * a frame to re-derive a constant.
 *
 * `dense->s/t` is baked asset data and these five are the entire remaining
 * dependency, so recording the ones that produced a run's current contents says
 * exactly when the loop would rewrite what is already there. Compared, not
 * hashed: five compares are cheaper than five multiplies and leave no collision
 * to argue about. 1,340 bytes of bss for 67 runs, and nothing new is cached —
 * `sNdsNativeFighterPreparedDense` already existed and already held this answer.
 *
 * What a per-run key cannot see is two runs sharing a dense id with different
 * inputs. That is a property of the BAKED `sNdsNativeFighterRunUniqueDense`
 * table, not of the frame, and `gNdsR2UvChangeCount` is its detector. */
typedef struct NDSNativeFighterRunUvInputs
{
    u32 scale_s;
    u32 scale_t;
    u32 origin_s;
    u32 origin_t;
    s32 offset;
    /* High and low programs reuse run indices but do not reuse dense IDs.  The
     * source can cross that detail boundary between a 2-player and 3+ player
     * scene without changing the texture metrics, so table identity is part of
     * the memo key. */
    const NDSNativeFighterRuntimeTables *tables;
    /* The arena fence, same key the fighter material block uses. A restart
     * rewinds the taskman heap and could reload a different dense table behind
     * the same run index with the same texture metrics; without this the stamp
     * would skip on the strength of metrics that no longer describe the
     * vertices. One compare, and the P1 milestone restarts from Results. */
    u32 heap_generation;
} NDSNativeFighterRunUvInputs;

static NDSNativeFighterRunUvInputs
    sNdsNativeFighterRunUvInputs[NDS_R2_RUN_MEMO_MAX];
static u8 sNdsNativeFighterRunUvValid[NDS_R2_RUN_MEMO_MAX];
#if NDS_TICK_HUD
/* Engagement proof. Skip should be ~99.96% of calls; a Build rate above the
 * 106 first-fills means the invariant above stopped holding. */
volatile u32 gNdsR2RunUvSkip;
volatile u32 gNdsR2RunUvBuild;
#endif

#if NDS_P2_LINK
typedef struct NDSNativeTexgenDirectionQ15
{
    s32 x;
    s32 y;
    s32 z;
} NDSNativeTexgenDirectionQ15;

/* P2-3f31. BattleShip Fast3D's G_TEXTURE_GEN path transforms each LookAt
 * direction by the current modelview, normalizes it, dots the source s8 vertex
 * normal against that direction, clamps the dot to [-1, 1], then evaluates
 *
 *     tc = ((dot + 1) / 4) * gSPTexture.scale
 *
 * for regular (non-LINEAR) texgen. Link is the first production owner to make
 * that state observable: one epoch in each detail level.
 *
 * Keep the transformed direction at Q15 rather than quantizing it back to s8.
 * The source uses float coefficients after normalization, while the DS source
 * modelview is already exact Q20.12 fixed point. This keeps all available DS
 * precision and uses the ARM9 hardware sqrt/div unit already owned by this
 * renderer. */
static s32 ndsRendererNativePrepareTexgenDirectionQ15(
    const Light *look_at,
    const NDSRendererMatrix20p12 *modelview,
    NDSNativeTexgenDirectionQ15 *out)
{
    s32 dx;
    s32 dy;
    s32 dz;
    s64 transformed_x;
    s64 transformed_y;
    s64 transformed_z;
    u64 length_squared;
    u32 length;

    if ((look_at == NULL) || (modelview == NULL) || (out == NULL))
    {
        return FALSE;
    }
    dx = (s32)look_at->l.dir[0];
    dy = (s32)look_at->l.dir[1];
    dz = (s32)look_at->l.dir[2];
    transformed_x =
        (s64)dx * modelview->m[0][0] +
        (s64)dy * modelview->m[0][1] +
        (s64)dz * modelview->m[0][2];
    transformed_y =
        (s64)dx * modelview->m[1][0] +
        (s64)dy * modelview->m[1][1] +
        (s64)dz * modelview->m[1][2];
    transformed_z =
        (s64)dx * modelview->m[2][0] +
        (s64)dy * modelview->m[2][1] +
        (s64)dz * modelview->m[2][2];
    length_squared =
        (u64)(transformed_x * transformed_x) +
        (u64)(transformed_y * transformed_y) +
        (u64)(transformed_z * transformed_z);
    if (length_squared == 0u)
    {
        return FALSE;
    }
    length = ndsR2HwMathSqrt64(length_squared);
    if ((length == 0u) || (length > (u32)INT_MAX))
    {
        return FALSE;
    }
    out->x = ndsR2HwMathDiv64(transformed_x * 32767, (s32)length);
    out->y = ndsR2HwMathDiv64(transformed_y * 32767, (s32)length);
    out->z = ndsR2HwMathDiv64(transformed_z * 32767, (s32)length);
    return TRUE;
}

static s32 ndsRendererNativeTexgenCoord(
    s32 nx,
    s32 ny,
    s32 nz,
    const NDSNativeTexgenDirectionQ15 *direction,
    u32 texture_scale)
{
    const s64 unit = (s64)127 * 32767;
    s64 dot =
        (s64)nx * direction->x +
        (s64)ny * direction->y +
        (s64)nz * direction->z;

    if (dot > unit)
    {
        dot = unit;
    }
    else if (dot < -unit)
    {
        dot = -unit;
    }
    /* Fast3D produces an N64 s/t value first and the DS FIFO uses 1/16-texel
     * coordinates where N64 source tc is 1/32 texel. The existing ordinary UV
     * path's >>17 is exactly (source * scale >>16) / 2. Since dot+unit is
     * non-negative, folding those two truncations is exactly division by
     * 8*unit here. */
    return ndsR2HwMathDiv64(
        (dot + unit) * (s64)(texture_scale & 0xffffu),
        (s32)(8 * unit));
}
#endif

/* The production UV memo miss is a setup/invalidated-data path, not steady
 * state. c200 executes the equality/validity guard but no instruction in this
 * rebuild body during the 1,600-frame gate window. Keep the exact writes and
 * proof hooks, but do not reserve ITCM for a branch the hot path does not take. */
static s32 __attribute__((noinline, cold))
ndsRendererNativeRebuildProductionRunUv(
    u32 run_index,
    u32 unique_first,
    u32 unique_count,
    const NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    NDSNativeFighterRunUvInputs *uv)
{
    u32 unique_offset;
#if NDS_P2_LINK
    u32 use_texgen =
        ((stats != NULL) &&
         ((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN) != 0u)) ?
            TRUE : FALSE;
    NDSNativeTexgenDirectionQ15 lookat_x;
    NDSNativeTexgenDirectionQ15 lookat_y;

    if (use_texgen != FALSE)
    {
        const LookAt *look_at = ndsR2CameraCurrentLookAt();

        if (((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN_LINEAR) != 0u) ||
            (look_at == NULL) || (state == NULL) ||
            (state->modelview_valid == 0u) ||
            (ndsRendererNativePrepareTexgenDirectionQ15(
                 &look_at->l[0], &state->modelview,
                 &lookat_x) == FALSE) ||
            (ndsRendererNativePrepareTexgenDirectionQ15(
                 &look_at->l[1], &state->modelview,
                 &lookat_y) == FALSE))
        {
            return FALSE;
        }
    }
#endif

    for (unique_offset = 0u;
         unique_offset < unique_count;
         unique_offset++)
    {
        u32 dense_id = sNdsNativeFighterActiveTables->run_unique_dense[
            unique_first + unique_offset];
        const NDSNativeDenseVertex *dense =
            &sNdsNativeFighterActiveTables->dense_vertices[dense_id];
        NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterActiveTables->prepared_dense[dense_id];
#if NDS_P2_LINK
        s32 scaled_s;
        s32 scaled_t;

        if (use_texgen != FALSE)
        {
            u32 rgba = dense->rgba;
            s32 nx = (s32)(s8)(rgba >> 24);
            s32 ny = (s32)(s8)(rgba >> 16);
            s32 nz = (s32)(s8)(rgba >> 8);

            scaled_s = ndsRendererNativeTexgenCoord(
                nx, ny, nz, &lookat_x, state->texture_prepare_scale_s);
            scaled_t = ndsRendererNativeTexgenCoord(
                nx, ny, nz, &lookat_y, state->texture_prepare_scale_t);
        }
        else
        {
            scaled_s =
                ((s32)dense->s *
                 (s32)state->texture_prepare_scale_s) >> 17;
            scaled_t =
                ((s32)dense->t *
                 (s32)state->texture_prepare_scale_t) >> 17;
        }
#else
        s32 scaled_s =
            ((s32)dense->s *
             (s32)state->texture_prepare_scale_s) >> 17;
        s32 scaled_t =
            ((s32)dense->t *
             (s32)state->texture_prepare_scale_t) >> 17;
#endif

        prepared->s = (s16)(
            scaled_s -
            ((s32)state->texture_prepare_origin_s << 2) +
            state->texture_prepare_offset);
        prepared->t = (s16)(
            scaled_t -
            ((s32)state->texture_prepare_origin_t << 2) +
            state->texture_prepare_offset);
#if NDS_R2_FIGHTER_RUN_PROOF
        ndsRendererR2FighterUvProofWrite(dense_id, prepared->s,
                                         prepared->t);
#endif
    }
    if (uv != NULL)
    {
        uv->scale_s = state->texture_prepare_scale_s;
        uv->scale_t = state->texture_prepare_scale_t;
        uv->origin_s = state->texture_prepare_origin_s;
        uv->origin_t = state->texture_prepare_origin_t;
        uv->offset = state->texture_prepare_offset;
        uv->tables = sNdsNativeFighterActiveTables;
        uv->heap_generation = gNdsTaskmanHeapGeneration;
        sNdsNativeFighterRunUvValid[run_index] = 1u;
    }
#if NDS_TICK_HUD
    gNdsR2RunUvBuild++;
#endif
    return TRUE;
}

static inline __attribute__((always_inline)) s32
ndsRendererNativePrepareProductionRunCore(
    u32 run_index,
    u32 epoch_policy,
    u32 packet_mode,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    NDSNativeHierarchyPreparedRun *hierarchy_run)
{
    const NDSNativeDirectPolicy *policy;
    const NDSRendererTileState *render_tile;
    u32 family = epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK;
    u32 expected_geometry_cull =
        ((epoch_policy & NDS_NATIVE_DIRECT_POLICY_CULL_NONE) != 0u) ?
            0u : NDS_RENDERER_GEOM_CULL_BACK;
    u32 expected_poly_cull =
        ((epoch_policy & NDS_NATIVE_DIRECT_POLICY_CULL_NONE) != 0u) ?
            POLY_CULL_NONE : POLY_CULL_BACK;
    u32 geometry_cull;
    u32 material_color;
    u32 use_texture;
    u32 texture_scale_s = 0u;
    u32 texture_scale_t = 0u;
    u32 texture_origin_s = 0u;
    u32 texture_origin_t = 0u;
    u32 unique_first;
    u32 unique_count;
    s32 texture_offset = 0;
    NDSRendererHardwareResolvedTexture resolved_texture;
#if NDS_R2_FIGHTER_RUN_PROOF >= 2
    u32 t_r2e11_phase;
    u32 t_r2e11_tex_valid = (state != NULL) ? state->texture_prepare_valid : 0u;
#endif

    memset(&resolved_texture, 0, sizeof(resolved_texture));

#if NDS_R2_FIGHTER_RUN_PROOF
    /* Counted at entry, because the proof hook sits before the single
     * `return TRUE` and so cannot see a rejected call. entries - hook calls is
     * the reject count, and the memo's safety depends on it: a run that is
     * sometimes accepted and sometimes rejected must not be baked. */
    gNdsR2RunEntryCount++;
#endif
#if NDS_R2_FIGHTER_RUN_PROOF >= 2
    /* E11 sizing. The census puts this function at 22,205 ticks/frame self time
     * over ~67 runs -- 331 per call -- and E5 already proved every output is
     * invariant in run_index. Before designing a memo, split the call: the
     * policy validation is what a memo could skip outright, the texture-prepare
     * block has a live GX side effect a memo must still pay, and the UV loop E5
     * measured is tiny. E8 was refuted because its key cost as much as the work
     * it skipped; this one is keyed on an integer index, so the question is only
     * how much work there is to skip. */
    t_r2e11_phase = cpuGetTiming();
#endif
    if ((config == NULL) || (stats == NULL) || (state == NULL) ||
        (family >= (sizeof(sNdsNativeFighterDirectPolicies) /
                    sizeof(sNdsNativeFighterDirectPolicies[0]))))
    {
        return ndsRendererNativeDirectReject(stats);
    }
    policy = &sNdsNativeFighterDirectPolicies[family];
    geometry_cull = stats->geometry_mode &
        (NDS_RENDERER_GEOM_CULL_FRONT | NDS_RENDERER_GEOM_CULL_BACK);
    if (((stats->geometry_mode &
          (NDS_RENDERER_GEOM_ZBUFFER | NDS_RENDERER_GEOM_LIGHTING)) !=
         (NDS_RENDERER_GEOM_ZBUFFER | NDS_RENDERER_GEOM_LIGHTING)) ||
        ((stats->geometry_mode &
          (NDS_RENDERER_GEOM_FOG |
           NDS_RENDERER_GEOM_TEXTURE_GEN_LINEAR)) != 0u) ||
#if NDS_P2_LINK
        ((((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN) != 0u) &&
          ((policy->textured == 0u) || (state->modelview_valid == 0u)))) ||
#else
        ((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN) != 0u) ||
#endif
        (geometry_cull != expected_geometry_cull) ||
        ((((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) != 0u) &&
          ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) !=
           NDS_RENDERER_ALPHA_COMPARE_THRESHOLD))) ||
        ((stats->othermode_l & NDS_RENDERER_ZMODE_MASK) ==
         NDS_RENDERER_ZMODE_DEC) ||
        ((stats->othermode_l & NDS_RENDERER_ZSOURCE_MASK) != 0u) ||
        (stats->texture_combine_w0 != policy->combine_w0) ||
        (stats->texture_combine_w1 != policy->combine_w1) ||
        ((family != NDS_NATIVE_DIRECT_POLICY_LIT_ONLY) &&
         (stats->env_color != 0xffffffffu)) ||
        (state->matrix_valid == 0u) ||
        (state->matrix_generation == 0u))
    {
        return ndsRendererNativeDirectReject(stats);
    }

#if NDS_R2_FIGHTER_RUN_PROOF >= 2
    gNdsR2RunValidateTicks += cpuGetTiming() - t_r2e11_phase;
    t_r2e11_phase = cpuGetTiming();
#endif
    material_color =
        ((policy->vertex_flags &
          NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL) != 0u) ?
            stats->prim_color : 0u;
    state->texture_prepare_source_zbuffered = TRUE;
    state->texture_prepare_decal_depth = FALSE;
    state->texture_prepare_prim_depth = FALSE;
    state->texture_prepare_material_color = material_color;
    state->texture_prepare_vertex_flags = policy->vertex_flags;
    if (state->texture_prepare_valid == 0u)
    {
        u32 texture_name = 0u;

        use_texture = FALSE;
#if NDS_R2_FIGHTER_RUN_MEMO == 1
        if ((policy->textured != 0u) && (hierarchy_run == NULL) &&
            (ndsRendererR2RunTextureMemoApply(
                 run_index, stats, &texture_name,
                 &texture_scale_s, &texture_scale_t,
                 &texture_origin_s, &texture_origin_t,
                 &texture_offset) != FALSE))
        {
            use_texture = TRUE;
        }
        else
#endif
        if (policy->textured != 0u)
        {
            u32 render_tile_index =
                ((stats->texture_state_flags &
                  NDS_RENDERER_TEXTURE_STATE_SEEN) != 0u) ?
                    (stats->texture_tile & 0x7u) :
                    NDS_RENDERER_RENDER_TILE;
            u32 implicit_texture_on = FALSE;
            u32 required_texture_mask =
                NDS_RENDERER_TEXTURE_SETTIMG |
                NDS_RENDERER_TEXTURE_SETTILE |
                NDS_RENDERER_TEXTURE_SETTILESIZE;

            render_tile = &stats->texture_tiles[render_tile_index];
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_ON) == 0u)
            {
                implicit_texture_on =
                    (((stats->texture_mask & required_texture_mask) ==
                      required_texture_mask) &&
                     ((stats->texture_mask &
                       (NDS_RENDERER_TEXTURE_LOADBLOCK |
                        NDS_RENDERER_TEXTURE_LOADTILE)) != 0u) &&
                     (stats->texture_image != 0u) &&
                     (stats->texture_load_texels != 0u) &&
                     (render_tile->set_seen != 0u) &&
                     (render_tile->size_seen != 0u) &&
                     (render_tile->line != 0u) &&
                     (render_tile->width != 0u) &&
                     (render_tile->height != 0u)) ? TRUE : FALSE;
            }
            use_texture = (hierarchy_run != NULL) ?
                ndsRendererHardwareResolveResidentTexture(
                    stats, config, state, &resolved_texture) :
                ndsRendererHardwareBindTexture(stats, config, state);
            if (use_texture == FALSE)
            {
                return ndsRendererNativeDirectReject(stats);
            }
            texture_name = (hierarchy_run != NULL) ?
                resolved_texture.name : sNdsRendererHardwareBoundTextureName;
            texture_scale_s = stats->texture_scale_s;
            texture_scale_t = stats->texture_scale_t;
            if (implicit_texture_on != FALSE)
            {
                if ((stats->texture_state_flags &
                     NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
                {
                    texture_scale_s =
                        NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
                }
                if ((stats->texture_state_flags &
                     NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
                {
                    texture_scale_t =
                        NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
                }
            }
            texture_origin_s = render_tile->uls;
            texture_origin_t = render_tile->ult;
            texture_offset =
                ((stats->othermode_h & NDS_RENDERER_TEXTFILT_MASK) !=
                 NDS_RENDERER_TF_POINT) ?
                    NDS_RENDERER_TEXCOORD_FILTER_OFFSET : 0;
#if NDS_R2_FIGHTER_RUN_MEMO
            if (hierarchy_run == NULL)
            {
#if NDS_R2_FIGHTER_RUN_MEMO >= 2
                ndsRendererR2RunTextureMemoVerify(
                    run_index, stats, texture_name,
                    texture_scale_s, texture_scale_t,
                    texture_origin_s, texture_origin_t, texture_offset);
#endif
                ndsRendererR2RunTextureMemoFill(
                    run_index, stats, texture_name,
                    texture_scale_s, texture_scale_t,
                    texture_origin_s, texture_origin_t, texture_offset);
            }
#endif
        }
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled = use_texture;
        state->texture_prepare_name = texture_name;
        state->texture_prepare_alpha_constant = TRUE;
        state->texture_prepare_poly_alpha = 31u;
        state->texture_prepare_poly_fmt =
            expected_poly_cull | POLY_ALPHA(31u) |
#if NDS_R2_FIGHTER_HW_LIGHT
            /* R2-03 E16. Enables the one hardware light the fighter needs.
             * R2-03 E49: except on an epoch the generic path would draw from
             * its raw vertex colour, where lighting is what produced E32's
             * dark-maroon hurt flash. This is the only site in the renderer
             * that sets a light bit. */
#if NDS_R2_UNLIT_VERTEX_EPOCH
            ((sNdsR2EpochUnlitVertexColor != 0u) ? 0u : POLY_FORMAT_LIGHT0) |
#else
            POLY_FORMAT_LIGHT0 |
#endif
#endif
            POLY_ID(stats->texture_combine_count &
                    NDS_RENDERER_POLY_ID_MASK);
        state->texture_prepare_scale_s = texture_scale_s;
        state->texture_prepare_scale_t = texture_scale_t;
        state->texture_prepare_origin_s = texture_origin_s;
        state->texture_prepare_origin_t = texture_origin_t;
        state->texture_prepare_offset = texture_offset;
        /* The bind this block just performed (memo or full resolver) is the
         * texture the runs under it draw with; record it, the polygon
         * attributes and the BEGIN after the params are applied so the packet
         * carries the final TEXIMAGE_PARAM word. */
        NDS_FIGHTER_PACKET_HOOK(ndsFighterPacketRecordPrepare(
            use_texture, state->texture_prepare_poly_fmt));
        if (packet_mode == 0u)
        {
            ndsRendererProfileRecordTexturePrepare();
        }
    }
    else
    {
        use_texture = state->texture_prepare_enabled;
        if ((use_texture != FALSE) != (policy->textured != 0u))
        {
            return ndsRendererNativeDirectReject(stats);
        }
        /* The cull bits belong to THIS epoch's policy, not to whichever epoch
         * last did a full prepare. Everything else in texture_prepare_poly_fmt
         * is a function of state the validity flag already covers; the cull is
         * not, because it is read from `epoch_policy` two lines into this
         * function while the flag is keyed on source geometry bits.
         *
         * P2-3r17 measured that this cannot currently fire: a cull change
         * arrives as NDS_NATIVE_STATE_GEOMETRY, which invalidates the prepare,
         * and every owner's live geometry mode is validated against the same
         * policy above -- Donkey High has one CULL_NONE epoch (22) and Mario
         * High none. It is repaired anyway because "unreachable" here is a
         * property of today's generated tables, not of this function, and one
         * mask/or per run against ~53 runs a frame is not a price worth
         * leaving a latent wrong-attribute path for. It is NOT the P2-3r17
         * seam: the owner falsified the whole cull family by observing that
         * POLY_CULL_NONE fills the holes' colour in without closing them. */
        state->texture_prepare_poly_fmt =
            (state->texture_prepare_poly_fmt & ~(u32)POLY_CULL_NONE) |
            expected_poly_cull;
        if (packet_mode == 0u)
        {
            ndsRendererProfileRecordTexturePrepareReuse();
        }
    }
    if ((hierarchy_run != NULL) && (policy->textured != 0u) &&
        (resolved_texture.entry == NULL))
    {
        if (ndsRendererHardwareResolveResidentTexture(
                stats, config, state, &resolved_texture) == FALSE)
        {
            return ndsRendererNativeDirectReject(stats);
        }
        state->texture_prepare_name = resolved_texture.name;
    }
#if NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY
    if (use_texture != FALSE)
    {
        state->texture_prepare_vertex_flags &=
            ~(NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL |
              NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX);
    }
#endif

#if NDS_R2_FIGHTER_RUN_PROOF >= 2
    if (t_r2e11_tex_valid == 0u)
    {
        gNdsR2RunTexPrepTicks += cpuGetTiming() - t_r2e11_phase;
        gNdsR2RunTexPrepCount++;
    }
    else
    {
        gNdsR2RunTexReuseTicks += cpuGetTiming() - t_r2e11_phase;
        gNdsR2RunTexReuseCount++;
    }
    t_r2e11_phase = cpuGetTiming();
#endif
    unique_first = sNdsNativeFighterActiveTables->run_first_unique[run_index];
    unique_count = sNdsNativeFighterActiveTables->run_unique_count[run_index];
    /* Hierarchy preflight records immutable UV policy only; commit evaluates
     * the live dense UVs once.  Mode-8 keeps its original immediate path. */
    if ((policy->textured != 0u) && (hierarchy_run == NULL))
    {
        /* See sNdsNativeFighterRunUvInputs: these five plus baked vertex data
         * are the whole dependency, so equal inputs mean the loop below would
         * write back exactly what is already there. */
        NDSNativeFighterRunUvInputs *uv =
            (run_index < NDS_R2_RUN_MEMO_MAX) ?
                &sNdsNativeFighterRunUvInputs[run_index] : NULL;

        if ((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN) != 0u)
        {
            /* Texgen depends on this root's live modelview and the camera
             * LookAt vectors. It is not a run-index memo unless those become
             * part of the key. Link's texgen run owns all 28 of its dense IDs
             * in both detail levels, so rebuilding it cannot poison a normal-UV
             * run's prepared storage. */
            if (ndsRendererNativeRebuildProductionRunUv(
                    run_index, unique_first, unique_count,
                    stats, state, NULL) == FALSE)
            {
                return ndsRendererNativeDirectReject(stats);
            }
            if (run_index < NDS_R2_RUN_MEMO_MAX)
            {
                sNdsNativeFighterRunUvValid[run_index] = 0u;
            }
        }
        else if ((uv != NULL) &&
            (sNdsNativeFighterRunUvValid[run_index] != 0u) &&
            (uv->scale_s == state->texture_prepare_scale_s) &&
            (uv->scale_t == state->texture_prepare_scale_t) &&
            (uv->origin_s == state->texture_prepare_origin_s) &&
            (uv->origin_t == state->texture_prepare_origin_t) &&
            (uv->offset == state->texture_prepare_offset) &&
            (uv->tables == sNdsNativeFighterActiveTables) &&
            (uv->heap_generation == gNdsTaskmanHeapGeneration))
        {
#if NDS_TICK_HUD
            gNdsR2RunUvSkip++;
#endif
        }
        else
        {
            if (ndsRendererNativeRebuildProductionRunUv(
                    run_index, unique_first, unique_count,
                    stats, state, uv) == FALSE)
            {
                return ndsRendererNativeDirectReject(stats);
            }
        }
    }

#if NDS_R2_FIGHTER_RUN_PROOF >= 2
    gNdsR2RunUvTicks += cpuGetTiming() - t_r2e11_phase;
    t_r2e11_phase = cpuGetTiming();
#endif
    if (hierarchy_run != NULL)
    {
        hierarchy_run->texture_entry = resolved_texture.entry;
        hierarchy_run->texture_name = state->texture_prepare_name;
        hierarchy_run->texture_params = resolved_texture.params;
        hierarchy_run->texture_format = resolved_texture.format;
        hierarchy_run->texture_width = resolved_texture.width;
        hierarchy_run->texture_height = resolved_texture.height;
        hierarchy_run->poly_fmt = state->texture_prepare_poly_fmt;
        hierarchy_run->scale_s = state->texture_prepare_scale_s;
        hierarchy_run->scale_t = state->texture_prepare_scale_t;
        hierarchy_run->origin_s = state->texture_prepare_origin_s;
        hierarchy_run->origin_t = state->texture_prepare_origin_t;
        hierarchy_run->texture_offset = state->texture_prepare_offset;
        hierarchy_run->vertex_flags = state->texture_prepare_vertex_flags;
        hierarchy_run->textured = state->texture_prepare_enabled;
    }
    else if (packet_mode == 0u)
    {
        ndsRendererNativeBeginDirectBatch(
            stats, policy->textured, state->texture_prepare_name,
            state->texture_prepare_poly_fmt, state->matrix_generation);
    }
#if NDS_R2_FIGHTER_RUN_PROOF >= 2
    gNdsR2RunTailTicks += cpuGetTiming() - t_r2e11_phase;
    gNdsR2RunSuccessCount++;
#endif
#if NDS_R2_FIGHTER_RUN_PROOF
    ndsRendererR2FighterRunProofCall(run_index, state, &resolved_texture);
#endif
    return TRUE;
}

/* The production owner always passes hierarchy_run == NULL, while the
 * hierarchy preflight always passes a real destination.  Specialize those two
 * cases at the call site so the production copy does not keep hierarchy-only
 * branches resident in ITCM.  The source body remains shared and identical. */
static s32 NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativePrepareProductionRun(
    u32 run_index,
    u32 epoch_policy,
    u32 packet_mode,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    return ndsRendererNativePrepareProductionRunCore(
        run_index, epoch_policy, packet_mode, config, stats, state, NULL);
}

static s32 NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE
ndsRendererNativePrepareHierarchyRun(
    u32 run_index,
    u32 epoch_policy,
    u32 packet_mode,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    NDSNativeHierarchyPreparedRun *hierarchy_run)
{
    return ndsRendererNativePrepareProductionRunCore(
        run_index, epoch_policy, packet_mode, config, stats, state,
        hierarchy_run);
}


#if NDS_LAB_CULL_PROBE
/* BUGS.md #10 probe. Paints each fighter run a distinct colour so a capture
 * says which joint owns the pixels bordering the hole, and which joint is
 * absent from it. Every earlier attempt guessed at that from the model data;
 * this reads it off the screen. Lab only. */
static u16 ndsRendererNativeLabRunTint(u32 run_index)
{
    static const u16 palette[8] = {
        0x001fu, 0x03e0u, 0x7c00u, 0x03ffu,
        0x7c1fu, 0x7fe0u, 0x421fu, 0x7fffu
    };

    return palette[(run_index >> NDS_LAB_TINT_SHIFT) & 7u];
}
#endif

static void NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE
ndsRendererNativeEmitProductionRawTexturedRun(
    u32 run_index,
    u32 corner_count)
{
    const u16 *corner =
        &sNdsNativeFighterActiveTables->packed_corners[
            sNdsNativeFighterActiveTables->run_first_corner[run_index]];
    u32 remaining = corner_count;

    while (remaining-- != 0u)
    {
        u32 dense_id = *corner++;
        const NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterActiveTables->prepared_dense[dense_id];

#if NDS_LAB_CULL_PROBE
        ndsRendererHardwareWriteColorWord(
            ndsRendererNativeLabRunTint(run_index));
#elif NDS_R2_FIGHTER_HW_LIGHT
        /* R2-03 E16. One FIFO word either way; the engine lights it.
         * R2-03 E49: unless the epoch draws its raw vertex colour. */
        #if NDS_R2_UNLIT_VERTEX_EPOCH
        if (sNdsR2EpochUnlitVertexColor != 0u)
        {
            ndsRendererHardwareWriteFighterColorWord(
                ndsRendererR2DenseVertexColor15(dense_id));
        }
        else
        #endif
        ndsRendererHardwareWriteNormalWord(
            sNdsNativeFighterActiveDenseNormals[dense_id]);
#else
        ndsRendererHardwareWriteFighterColorWord(prepared->packed_color);
#endif
        ndsRendererHardwareWriteFighterTexCoordWord(
            (u32)(u16)prepared->s |
            ((u32)(u16)prepared->t << 16));
        ndsRendererHardwareWriteFighterVertex16Words(
            prepared->gx_xy, prepared->gx_z);
    }
}

static void NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE
ndsRendererNativeEmitProductionRawUntexturedRun(
    u32 run_index,
    u32 corner_count)
{
    const u16 *corner =
        &sNdsNativeFighterActiveTables->packed_corners[
            sNdsNativeFighterActiveTables->run_first_corner[run_index]];
    u32 remaining = corner_count;

    while (remaining-- != 0u)
    {
        u32 dense_id = *corner++;
        const NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterActiveTables->prepared_dense[dense_id];

#if NDS_LAB_CULL_PROBE
        ndsRendererHardwareWriteFighterColorWord(
            ndsRendererNativeLabRunTint(run_index));
#elif NDS_R2_FIGHTER_HW_LIGHT
#if NDS_R2_UNLIT_VERTEX_EPOCH
        if (sNdsR2EpochUnlitVertexColor != 0u)
        {
            ndsRendererHardwareWriteFighterColorWord(
                ndsRendererR2DenseVertexColor15(dense_id));
        }
        else
#endif
        ndsRendererHardwareWriteNormalWord(
            sNdsNativeFighterActiveDenseNormals[dense_id]);
#else
        ndsRendererHardwareWriteFighterColorWord(prepared->packed_color);
#endif
        ndsRendererHardwareWriteFighterVertex16Words(
            prepared->gx_xy, prepared->gx_z);
    }
}

#if (NDS_TASK56_FIGHTER_PRIMITIVES >= 1) && \
    (NDS_RENDERER_PROFILE_LEVEL < 2) && NDS_RENDERER_HW_TRIANGLES
/* The one-binary A/B for the strips, same instrument as gNdsR2AnimCutRoute.
 * .data and aligned(32) so it owns its cache line and no neighbouring
 * counter's write-back can stamp the poke. Default 1 = strips, so an unpoked
 * ROM is the candidate; `-SetGlobals gNdsR2FighterStripRoute=0` is the raw
 * control, in the SAME binary at the SAME placement. Compile-time gated
 * because the route is an instrument, not a feature: at NDS_R2_STRIP_ROUTE=0
 * the test folds to a constant and the unselected emitter is dead-coded. */
#if NDS_R2_STRIP_ROUTE
volatile u32 gNdsR2FighterStripRoute
    __attribute__((section(".data"), aligned(32))) = 1u;
#define NDS_R2_STRIP_ROUTE_ON() (gNdsR2FighterStripRoute != 0u)
#else
#define NDS_R2_STRIP_ROUTE_ON() (1)
#endif

#if NDS_LAB_NO_CULL
/* The seam probe's one control surface. Defined here because arm 3 drives the
 * strip route above; the poly-format arms are read at the two begin-batch
 * sites. Returns the arm now selected so the caller can label the screen -- an
 * unlabelled arm is how a probe capture becomes unattributable evidence. */
u32 ndsRendererLabSeamAdvanceArm(void)
{
    u32 arm = gNdsLabSeamArm + 1u;

    if (arm >= NDS_LAB_SEAM_ARM_COUNT)
    {
        arm = 0u;
    }
    gNdsLabSeamArm = arm;
#if NDS_LAB_SEAM_STRIP_ARM != 0xffu
    gNdsR2FighterStripRoute = (arm == NDS_LAB_SEAM_STRIP_ARM) ? 0u : 1u;
#endif
    return arm;
}

u32 ndsRendererLabSeamArm(void)
{
    return gNdsLabSeamArm;
}
#endif

/* Task 56: emit a RAW run's triangles as DS-native primitive groups
 * (GL_TRIANGLE_STRIP + residual GL_TRIANGLES) compiled host-side by the
 * generator. 626 triangles submitted as 1,878 individual GL_TRIANGLES corners
 * become 1,014 strip corners in 163 groups -- **46.0% fewer vertex
 * submissions**, and the c115 per-PC census says a fighter corner costs ~40
 * cycles of which ~28 is the GX write itself, so the vertices ARE the cost.
 *
 * Two things about this path were wrong when it was first killed:
 *
 *  - the generator emitted 35.6% of the triangles with reversed winding, so a
 *    third of the fighter was culled away with no assert to say so. Fixed in
 *    `_stripify_run`; `check_fighter_primitive_streams.py` is the standing
 *    proof and expands every group back into oriented triangles.
 *  - this function was `cold` and `optimize("Os")` in `.main` while its raw
 *    siblings sat in ITCM at eleven instructions a corner, and it branched on
 *    `textured` once per VERTEX. A 46% vertex cut cannot survive being paid
 *    for at twice the per-vertex rate. It now has the same placement and the
 *    same inner-loop shape as the raw emitters, with the type branch hoisted
 *    to the group.
 *
 * The batch is open from PrepareProductionRun with GL_TRIANGLE, and every
 * other emitter -- the cross-matrix run, the raw runs, the next run reusing
 * this batch -- assumes the primitive type is still GL_TRIANGLE, because
 * batches are REUSED without re-issuing glBegin. So this restores GL_TRIANGLE
 * before returning rather than tracking the type globally: one FIFO word per
 * run against 53 run emissions a frame, and no invariant left for a future
 * caller to violate. */
static void NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativeEmitProductionPrimitiveGroups(
    u32 run_index,
    u32 textured)
{
    u32 g = sNdsNativeFighterActiveTables->primitive_group_first[run_index];
    u32 remaining_groups =
        sNdsNativeFighterActiveTables->primitive_group_count[run_index];
    u32 current_type = (u32)GL_TRIANGLE;

    while (remaining_groups-- != 0u)
    {
        u32 gtype = sNdsNativeFighterActiveTables->primitive_group_type[g];
        const u16 *vref = &sNdsNativeFighterActiveTables->primitive_vertices[
            sNdsNativeFighterActiveTables->primitive_group_first_vertex[g]];
        u32 remaining =
            sNdsNativeFighterActiveTables->primitive_group_vertex_count[g];

        g++;
        /* A new group needs its own BEGIN unless it is a GL_TRIANGLE group
         * following a GL_TRIANGLE group -- separate triangles concatenate
         * harmlessly, strips do NOT.
         *
         * The inherited condition was `gtype != current_type`, which skipped
         * the BEGIN between ADJACENT STRIPS and silently welded them into one
         * list: two bogus bridging triangles, and every triangle after the join
         * carrying the wrong parity, so it was culled. The tables have six
         * consecutive strip groups in the first run alone, and the owner saw it
         * immediately as missing geometry on both fighters. It cost a shipped
         * regression because `check_fighter_primitive_streams.py` expanded each
         * group INDEPENDENTLY -- it proved the data and assumed this policy.
         * The checker now models the policy; keep the two in step. */
        if ((gtype != current_type) || (gtype != (u32)GL_TRIANGLE))
        {
            glBegin((GL_GLBEGIN_ENUM)gtype);
            current_type = gtype;
        }
        if (textured != 0u)
        {
            while (remaining-- != 0u)
            {
                u32 dense_id = *vref++;
                const NDSNativePreparedDenseVertex *prepared =
                    &sNdsNativeFighterActiveTables->prepared_dense[dense_id];

#if NDS_LAB_CULL_PROBE
                /* BUGS.md #10 probe, same arm the raw emitters carry. It was
                 * missing here, which would have made a probe build silently
                 * useless for the one path that needs localising. */
                ndsRendererHardwareWriteFighterColorWord(
                    ndsRendererNativeLabRunTint(run_index));
#elif NDS_R2_FIGHTER_HW_LIGHT
                #if NDS_R2_UNLIT_VERTEX_EPOCH
                if (sNdsR2EpochUnlitVertexColor != 0u)
                {
                    ndsRendererHardwareWriteFighterColorWord(
                        ndsRendererR2DenseVertexColor15(dense_id));
                }
                else
                #endif
                ndsRendererHardwareWriteNormalWord(
                    sNdsNativeFighterActiveDenseNormals[dense_id]);
#else
                ndsRendererHardwareWriteFighterColorWord(
                    prepared->packed_color);
#endif
                ndsRendererHardwareWriteFighterTexCoordWord(
                    (u32)(u16)prepared->s |
                    ((u32)(u16)prepared->t << 16));
                ndsRendererHardwareWriteFighterVertex16Words(
                    prepared->gx_xy, prepared->gx_z);
            }
        }
        else
        {
            while (remaining-- != 0u)
            {
                u32 dense_id = *vref++;
                const NDSNativePreparedDenseVertex *prepared =
                    &sNdsNativeFighterActiveTables->prepared_dense[dense_id];

#if NDS_LAB_CULL_PROBE
                /* BUGS.md #10 probe, same arm the raw emitters carry. It was
                 * missing here, which would have made a probe build silently
                 * useless for the one path that needs localising. */
                ndsRendererHardwareWriteFighterColorWord(
                    ndsRendererNativeLabRunTint(run_index));
#elif NDS_R2_FIGHTER_HW_LIGHT
                #if NDS_R2_UNLIT_VERTEX_EPOCH
                if (sNdsR2EpochUnlitVertexColor != 0u)
                {
                    ndsRendererHardwareWriteFighterColorWord(
                        ndsRendererR2DenseVertexColor15(dense_id));
                }
                else
                #endif
                ndsRendererHardwareWriteNormalWord(
                    sNdsNativeFighterActiveDenseNormals[dense_id]);
#else
                ndsRendererHardwareWriteFighterColorWord(
                    prepared->packed_color);
#endif
                ndsRendererHardwareWriteFighterVertex16Words(
                    prepared->gx_xy, prepared->gx_z);
            }
        }
    }
    if (current_type != (u32)GL_TRIANGLE)
    {
        glBegin(GL_TRIANGLE);
    }
}
#endif

static void NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativeEmitProductionCrossRun(
    u32 run_index,
    u32 corner_count,
    u32 textured,
    u32 current_palette_slot,
    const u8 *binding_palette_slots)
{
    const u16 *corner =
        &sNdsNativeFighterActiveTables->packed_corners[
            sNdsNativeFighterActiveTables->run_first_corner[run_index]];
    u32 active_palette_slot = current_palette_slot;
    u32 remaining = corner_count;

    while (remaining-- != 0u)
    {
        u32 packed = *corner++;
        u32 dense_id = packed & NDS_NATIVE_DENSE_ID_MASK;
        const NDSNativeDenseVertex *dense =
            &sNdsNativeFighterActiveTables->dense_vertices[dense_id];
        const NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterActiveTables->prepared_dense[dense_id];
        u32 palette_slot;

        if (binding_palette_slots != NULL)
        {
            palette_slot = binding_palette_slots[dense->matrix_binding];
        }
        else
        {
            palette_slot =
                packed >> NDS_NATIVE_PACKED_CORNER_MATRIX_SHIFT;
        }
        if (palette_slot == NDS_NATIVE_GX_MATRIX_CURRENT)
        {
            palette_slot = current_palette_slot;
        }
        if (palette_slot != active_palette_slot)
        {
            glRestoreMatrix((int)palette_slot);
            active_palette_slot = palette_slot;
        }
#if NDS_R2_FIGHTER_HW_LIGHT
        #if NDS_R2_UNLIT_VERTEX_EPOCH
        if (sNdsR2EpochUnlitVertexColor != 0u)
        {
            ndsRendererHardwareWriteFighterColorWord(
                ndsRendererR2DenseVertexColor15(dense_id));
        }
        else
        #endif
        ndsRendererHardwareWriteNormalWord(
            sNdsNativeFighterActiveDenseNormals[dense_id]);
#else
        ndsRendererHardwareWriteFighterColorWord(prepared->packed_color);
#endif
        if (textured != 0u)
        {
            ndsRendererHardwareWriteFighterTexCoordWord(
                (u32)(u16)prepared->s |
                ((u32)(u16)prepared->t << 16));
        }
        ndsRendererHardwareWriteFighterVertex16Words(
            prepared->gx_xy, prepared->gx_z);
    }
    if (active_palette_slot != current_palette_slot)
    {
        glRestoreMatrix((int)current_palette_slot);
    }
}

#if NDS_FIGHTER_PACKET_LIVE && (NDS_TASK56_FIGHTER_PRIMITIVES >= 1)
#if NDS_LAB_CULL_PROBE
#error "NDS_R2_FIGHTER_PACKET does not carry the lab cull tint"
#endif
/* Record-frame twins of the two production emitters: the hardware receives
 * exactly the words the plain emitters push, and the packet receives the same
 * words packed. Main RAM on purpose -- they run only on the frame a packet is
 * (re)recorded. */
static inline void ndsFighterPacketEmitCornerShade(u32 dense_id)
{
#if NDS_R2_FIGHTER_HW_LIGHT
#if NDS_R2_UNLIT_VERTEX_EPOCH
    if (sNdsR2EpochUnlitVertexColor != 0u)
    {
        u32 color = ndsRendererR2DenseVertexColor15(dense_id);

        ndsRendererHardwareWriteFighterColorWord(color);
        ndsFighterPacketCmd1(FIFO_COLOR, color);
        return;
    }
#endif
    {
        u32 normal = sNdsNativeFighterActiveDenseNormals[dense_id];

        ndsRendererHardwareWriteNormalWord(normal);
        ndsFighterPacketCmd1(FIFO_NORMAL, normal);
    }
#else
    {
        u32 color =
            sNdsNativeFighterActiveTables->prepared_dense[dense_id].packed_color;

        ndsRendererHardwareWriteFighterColorWord(color);
        ndsFighterPacketCmd1(FIFO_COLOR, color);
    }
#endif
}

static inline void ndsFighterPacketEmitCornerTail(
    const NDSNativePreparedDenseVertex *prepared, u32 textured)
{
    if (textured != 0u)
    {
        u32 st = (u32)(u16)prepared->s | ((u32)(u16)prepared->t << 16);

        ndsRendererHardwareWriteFighterTexCoordWord(st);
        ndsFighterPacketCmd1(FIFO_TEX_COORD, st);
    }
    ndsRendererHardwareWriteFighterVertex16Words(
        prepared->gx_xy, prepared->gx_z);
    ndsFighterPacketCmd2(FIFO_VERTEX16, prepared->gx_xy, prepared->gx_z);
}

static void NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE
ndsRendererNativeEmitProductionPrimitiveGroupsPacket(
    u32 run_index,
    u32 textured)
{
    u32 g = sNdsNativeFighterActiveTables->primitive_group_first[run_index];
    u32 remaining_groups =
        sNdsNativeFighterActiveTables->primitive_group_count[run_index];
    u32 current_type = (u32)GL_TRIANGLE;

    while (remaining_groups-- != 0u)
    {
        u32 gtype = sNdsNativeFighterActiveTables->primitive_group_type[g];
        const u16 *vref = &sNdsNativeFighterActiveTables->primitive_vertices[
            sNdsNativeFighterActiveTables->primitive_group_first_vertex[g]];
        u32 remaining =
            sNdsNativeFighterActiveTables->primitive_group_vertex_count[g];

        g++;
        if ((gtype != current_type) || (gtype != (u32)GL_TRIANGLE))
        {
            glBegin((GL_GLBEGIN_ENUM)gtype);
            ndsFighterPacketCmd1(FIFO_BEGIN, gtype);
            current_type = gtype;
        }
        while (remaining-- != 0u)
        {
            u32 dense_id = *vref++;

            ndsFighterPacketEmitCornerShade(dense_id);
            ndsFighterPacketEmitCornerTail(
                &sNdsNativeFighterActiveTables->prepared_dense[dense_id],
                textured);
        }
    }
    if (current_type != (u32)GL_TRIANGLE)
    {
        glBegin(GL_TRIANGLE);
        ndsFighterPacketCmd1(FIFO_BEGIN, (u32)GL_TRIANGLE);
    }
}

static void NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE
ndsRendererNativeEmitProductionCrossRunPacket(
    u32 run_index,
    u32 corner_count,
    u32 textured,
    u32 current_palette_slot,
    const u8 *binding_palette_slots)
{
    const u16 *corner =
        &sNdsNativeFighterActiveTables->packed_corners[
            sNdsNativeFighterActiveTables->run_first_corner[run_index]];
    u32 active_palette_slot = current_palette_slot;
    u32 remaining = corner_count;

    while (remaining-- != 0u)
    {
        u32 packed = *corner++;
        u32 dense_id = packed & NDS_NATIVE_DENSE_ID_MASK;
        const NDSNativeDenseVertex *dense =
            &sNdsNativeFighterActiveTables->dense_vertices[dense_id];
        u32 palette_slot;

        if (binding_palette_slots != NULL)
        {
            palette_slot = binding_palette_slots[dense->matrix_binding];
        }
        else
        {
            palette_slot = packed >> NDS_NATIVE_PACKED_CORNER_MATRIX_SHIFT;
        }
        if (palette_slot == NDS_NATIVE_GX_MATRIX_CURRENT)
        {
            palette_slot = current_palette_slot;
        }
        if (palette_slot != active_palette_slot)
        {
            glRestoreMatrix((int)palette_slot);
            ndsFighterPacketCmd1(REG2ID(MATRIX_RESTORE), palette_slot);
            active_palette_slot = palette_slot;
        }
        ndsFighterPacketEmitCornerShade(dense_id);
        ndsFighterPacketEmitCornerTail(
            &sNdsNativeFighterActiveTables->prepared_dense[dense_id],
            textured);
    }
    if (active_palette_slot != current_palette_slot)
    {
        glRestoreMatrix((int)current_palette_slot);
        ndsFighterPacketCmd1(REG2ID(MATRIX_RESTORE), current_palette_slot);
    }
}
#endif

static inline void ndsRendererNativeAccountGXCrossTriangles(
    NDSRendererStats *stats,
    u32 triangle_count,
    u32 reuse_count)
{
    sNdsRendererHardwareSubmitClassCounts[
        NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX] += triangle_count;
    sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices +=
        triangle_count * 3u;
    if ((sNdsRendererRuntimeFrameSummary.hardware_triangles > 2048u) ||
        (sNdsRendererRuntimeFrameSummary.hardware_vertices > 6144u))
    {
        sNdsRendererRuntimeFrameSummary.hardware_over_limit = 1u;
    }
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    stats->hardware_zbuffer_triangle_count += triangle_count;
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount += triangle_count;
#endif
    sNdsRendererHardwareSubmitted = TRUE;
}

#if NDS_TASK91_DRAW_PHASE_CENSUS
/* R2-03 E15. E14 put ndsRendererExecuteNativeFighterOwnerProduction at 279,617
 * ticks/frame -- 56% of the fighter, 447 ticks per hardware triangle, with the
 * geometry engine measured idle. This function is the whole of it: prepare the
 * run, then emit it as either a CROSS_MATRIX run or a Task 56 primitive group.
 * Which of the three carries the cost decides whether the answer is a captured
 * command stream (R2-02 E2's lever), a cheaper prepare, or moving work onto the
 * idle hardware -- so it is measured before any of them is proposed.
 *
 * `Calls` is the liveness counter. The first cut of this experiment
 * instrumented ndsRendererNativeSubmitRunDirect, which is the non-production
 * path and never executes in the canonical mode, and every counter read a
 * perfectly plausible zero. */
u32 gNdsR2SubmitCalls;
u32 gNdsR2SubmitPrepTicks;
u32 gNdsR2SubmitRawEmitTicks;
u32 gNdsR2SubmitRawCalls;
u32 gNdsR2SubmitRawTriangles;
u32 gNdsR2SubmitCrossEmitTicks;
u32 gNdsR2SubmitCrossCalls;
u32 gNdsR2SubmitCrossTriangles;
u32 gNdsR2SubmitTotalTicks;
/* E15b. The run loop turned out to be only 37% of the execute, so the rest is
 * the per-root and per-epoch preamble that runs before any triangle is emitted.
 * These name it. */
u32 gNdsR2ExecPreflightTicks;
u32 gNdsR2ExecRootTicks;
u32 gNdsR2ExecStateTicks;
u32 gNdsR2ExecShadeTicks;
u32 gNdsR2ExecEpochCalls;
#endif

#if NDS_FIGHTER_PACKET_LIVE
extern u16 gSYFramebufferSets[1][231][320];

static u32 ndsFighterPacketMix(u32 hash, u32 value)
{
    return (hash ^ value) * 16777619u;
}

/* Every input the packet's static words were derived from, cheap enough to
 * hash per fighter per frame: the adapter's material identity (live MObj keys
 * plus the colour modulate), the owner/costume/detail/instance key, the
 * generated program and arena generation, each root's display preamble minus
 * the patched light direction, each root's chain shape, and the texture cache
 * placement fences. */
static void ndsFighterPacketBuildKey(
    u32 texture_memo_owner_key,
    u32 packet_key,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count,
    u32 *key)
{
    u32 preamble_hash = 2166136261u;
    u32 shape_hash = 2166136261u;
    u32 i;

    for (i = 0u; i < input_count; i++)
    {
        const NDSRendererNativeFighterRoot *input = &inputs[i];
        const NDSRendererNativeFighterPreamble *preamble = input->preamble;

        preamble_hash = ndsFighterPacketMix(preamble_hash, input->root_offset);
        preamble_hash = ndsFighterPacketMix(preamble_hash,
                                            preamble->geometry_mode);
        preamble_hash = ndsFighterPacketMix(preamble_hash,
                                            preamble->cycle_type);
        preamble_hash = ndsFighterPacketMix(preamble_hash,
                                            preamble->render_mode);
        /* prim_color and the colour modulate are deliberately NOT here: the
         * shade sites re-derive their DIF_AMB words from them on replay. */
        preamble_hash = ndsFighterPacketMix(preamble_hash,
                                            preamble->env_color);
        preamble_hash = ndsFighterPacketMix(preamble_hash, preamble->flags);
        preamble_hash = ndsFighterPacketMix(
            preamble_hash, input->config->initial_geometry_mode);
        preamble_hash = ndsFighterPacketMix(preamble_hash,
                                            input->material_count);
        shape_hash = ndsFighterPacketMix(
            shape_hash,
            (u32)input->gx_parent_slot |
                ((u32)input->gx_store_slot << 8) |
                ((u32)input->gx_local_count << 16) |
                ((u32)input->gx_seed_is_identity << 24) |
                ((u32)input->gx_valid << 25));
    }
    key[0] = packet_key;
    key[1] = texture_memo_owner_key;
    key[2] = (u32)(uintptr_t)sNdsNativeFighterActiveTables ^
             (gNdsTaskmanHeapGeneration * 0x9E3779B1u);
    key[3] = preamble_hash;
    key[4] = shape_hash ^ (input_count << 26);
    /* The global texture fence is only part of the key for a packet whose
     * textures could not be validated individually (needs_fence). */
    key[5] = sNdsRendererHardwareTextureKeyGeneration ^
             (sNdsRendererRuntimeTextureCacheEvictCount << 16);
}

static void NDS_FIGHTER_PACKET_COLD_CODE ndsFighterPacketNoteTextureEntry(void)
{
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;
    NDSFighterPacket *packet = rec->packet;
    const NDSRendererHardwareTextureCacheEntry *entry =
        sNdsRendererHardwareActiveTextureEntry;
    u32 slot;
    u32 i;

    if (packet == NULL)
    {
        return;
    }
    if (entry == NULL)
    {
        packet->needs_fence = 1u;
        return;
    }
    slot = (u32)(entry - sNdsRendererHardwareTextureCache);
    if (slot >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT)
    {
        packet->needs_fence = 1u;
        return;
    }
    for (i = 0u; i < (u32)packet->texture_count; i++)
    {
        if (packet->textures[i].slot_plus1 == slot + 1u)
        {
            return;
        }
    }
    if ((u32)packet->texture_count >= NDS_FIGHTER_PACKET_TEXTURE_MAX)
    {
        packet->needs_fence = 1u;
        return;
    }
    packet->textures[packet->texture_count].slot_plus1 = (u16)(slot + 1u);
    packet->textures[packet->texture_count].name = (u16)entry->name;
    packet->textures[packet->texture_count].key_generation =
        entry->key_generation;
    packet->texture_count++;
}

static s32 ndsFighterPacketTexturesResident(const NDSFighterPacket *packet)
{
    u32 i;

    for (i = 0u; i < (u32)packet->texture_count; i++)
    {
        const NDSFighterPacketTexture *texture = &packet->textures[i];
        const NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[texture->slot_plus1 - 1u];

        if ((entry->ready == FALSE) ||
            ((u32)(u16)entry->name != (u32)texture->name) ||
            (entry->key_generation != texture->key_generation))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* A packet replay binds these textures directly through its recorded FIFO
 * words, so it bypasses ndsRendererHardwareBindTextureName and the normal
 * texture-prepare path that refresh `last_used_frame`. Without an explicit
 * touch, a texture used by a fighter on every packet-hit frame looks cold to
 * the shared dynamic-cache LRU and is eventually evicted by stage/effect work.
 * That invalidates the packet on the next fighter draw and forces a complete
 * production re-record -- exactly the alternating four-CPU FTR tail.
 *
 * The residency predicate has already proved slot/name/generation identity;
 * this changes cache bookkeeping only. Pinned entries accept the same touch
 * harmlessly, while dynamic entries now reflect the GX use that actually
 * occurred on this frame. */
static void ndsFighterPacketTouchTextures(const NDSFighterPacket *packet)
{
    u32 i;

    for (i = 0u; i < (u32)packet->texture_count; i++)
    {
        const NDSFighterPacketTexture *texture = &packet->textures[i];
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[texture->slot_plus1 - 1u];

        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    }
}

/* Re-derive every shade site's DIF_AMB word for the live prim/modulate.
 * Skipped when neither moved since the words were last derived. */
static void ndsFighterPacketApplyTint(
    NDSFighterPacket *packet, const NDSRendererNativeFighterRoot *inputs)
{
    u32 modulate = inputs[0].config->color_modulate;
    u32 prim_hash = 2166136261u;
    u32 i;

    for (i = 0u; i < packet->root_count; i++)
    {
        prim_hash = ndsFighterPacketMix(prim_hash,
                                        inputs[i].preamble->prim_color);
    }
    if ((packet->tint_modulate == modulate) &&
        (packet->tint_prim_hash == prim_hash))
    {
        return;
    }
    for (i = 0u; i < packet->site_count; i++)
    {
        const NDSFighterPacketShadeSite *site = &packet->sites[i];
        u32 material_color = site->material_color;
        u32 diffuse;
        u32 ambient;

        if ((site->prim_from_root != 0u) &&
            ((u32)site->root < packet->root_count))
        {
            material_color = inputs[site->root].preamble->prim_color;
        }
        diffuse = ndsRendererR2MaterialColor15(
            site->light_color_1, material_color, site->use_material,
            modulate);
        ambient = ndsRendererR2MaterialColor15(
            site->light_color_2, material_color, site->use_material,
            modulate);
        packet->words[site->index] = diffuse | (ambient << 16);
    }
    packet->tint_modulate = modulate;
    packet->tint_prim_hash = prim_hash;
}

/* ndsRendererR2WriteLightVector's normalisation, same hardware units. */
static u32 ndsFighterPacketLightWord(s32 x, s32 y, s32 z)
{
    s64 length_squared = ((s64)x * x) + ((s64)y * y) + ((s64)z * z);
    s32 nx = 0;
    s32 ny = 0;
    s32 nz = 0;

    if (length_squared > 0)
    {
        u32 length = ndsR2HwMathSqrt64((u64)length_squared);

        if (length != 0u)
        {
            nx = (s32)ndsR2HwMathDiv64(-(s64)x * 511, (s32)length);
            ny = (s32)ndsR2HwMathDiv64(-(s64)y * 511, (s32)length);
            nz = (s32)ndsR2HwMathDiv64(-(s64)z * 511, (s32)length);
        }
    }
    return NDS_R2_NORMAL_PACK((int)nx, (int)ny, (int)nz);
}

static void NDS_FIGHTER_PACKET_COLD_CODE ndsFighterPacketAbortRecord(void)
{
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;

    sNdsFighterPacketRecording = 0u;
    if (rec->packet != NULL)
    {
        rec->packet->valid = 0u;
    }
    gNdsFighterPacketFaults++;
}

static void NDS_FIGHTER_PACKET_COLD_CODE ndsFighterPacketFinishRecord(
    u32 run_count,
    u32 triangle_count,
    u32 raw_triangles,
    u32 raw_reuse,
    u32 cross_triangles,
    u32 cross_reuse)
{
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;
    NDSFighterPacket *packet = rec->packet;

    sNdsFighterPacketRecording = 0u;
    if (packet == NULL)
    {
        return;
    }
    if ((rec->fault == 0u) && (rec->header_valid != 0u) &&
        (rec->header_params == 0u))
    {
        if (rec->count < rec->capacity)
        {
            rec->words[rec->count++] = 0u;
        }
        else
        {
            rec->fault = 1u;
        }
    }
    if ((rec->fault != 0u) || (rec->count == 0u))
    {
        packet->valid = 0u;
        gNdsFighterPacketFaults++;
        return;
    }
    packet->word_count = rec->count;
    if (packet->needs_fence == 0u)
    {
        packet->key[5] = 0u;
    }
    packet->run_count = run_count;
    packet->triangle_count = triangle_count;
    packet->raw_triangles = raw_triangles;
    packet->raw_reuse = raw_reuse;
    packet->cross_triangles = cross_triangles;
    packet->cross_reuse = cross_reuse;
#if NDS_RENDERER_FRAME_SUMMARY_COUNTERS
    {
        /* The raw/cross credits add `reuse` to both reuse counters on a hit
         * as they did during this record, so the stored residual excludes
         * them (clamped: the window cannot accrue less than it credited). */
        u32 credited = raw_reuse + cross_reuse;
        u32 reuse = sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count -
            sNdsFighterPacketRecordBase[1];
        u32 prepare_reuse =
            sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count -
            sNdsFighterPacketRecordBase[4];

        packet->batch_begin =
            sNdsRendererRuntimeFrameSummary.hardware_batch_begin_count -
            sNdsFighterPacketRecordBase[0];
        packet->batch_reuse = (reuse > credited) ? (reuse - credited) : 0u;
        packet->batch_end =
            sNdsRendererRuntimeFrameSummary.hardware_batch_end_count -
            sNdsFighterPacketRecordBase[2];
        packet->prepare_begin =
            sNdsRendererRuntimeFrameSummary.texture_prepare_count -
            sNdsFighterPacketRecordBase[3];
        packet->prepare_reuse =
            (prepare_reuse > credited) ? (prepare_reuse - credited) : 0u;
        packet->matrix_loads =
            sNdsRendererRuntimeFrameSummary.matrix_load_count -
            sNdsFighterPacketRecordBase[5];
        packet->texture_binds =
            sNdsRendererRuntimeFrameSummary.texture_binds -
            sNdsFighterPacketRecordBase[6];
    }
#else
    packet->batch_begin = 0u;
    packet->batch_reuse = 0u;
    packet->batch_end = 0u;
    packet->prepare_begin = 0u;
    packet->prepare_reuse = 0u;
    packet->matrix_loads = 0u;
    packet->texture_binds = 0u;
#endif
    /* The dense-vertex walk counts one source vertex load per vertex word
     * it emits; the replay emits the same words. */
    packet->vertex_loads = sNdsRendererHardwareSourceVertexLoadCount -
        sNdsFighterPacketRecordBase[7];
    DC_FlushRange(packet->words, packet->word_count * sizeof(u32));
    if (packet->word_count > gNdsFighterPacketWordsMax)
    {
        gNdsFighterPacketWordsMax = packet->word_count;
    }
    packet->valid = 1u;
}

/* The hit predicate, shared by the replay and the adapter's pre-check so the
 * two can never disagree about a frame. */
static s32 ndsFighterPacketMatches(
    const NDSFighterPacket *packet, const u32 *key, u32 input_count)
{
    u32 fence = (packet->needs_fence != 0u) ? key[5] : 0u;

    return ((packet->valid != 0u) && (packet->root_count == input_count) &&
            (packet->key[0] == key[0]) && (packet->key[1] == key[1]) &&
            (packet->key[2] == key[2]) && (packet->key[3] == key[3]) &&
            (packet->key[4] == key[4]) && (packet->key[5] == fence) &&
            (ndsFighterPacketTexturesResident(packet) != FALSE)) ? TRUE : FALSE;
}

/* P2-2p4. The adapter asks before it prepares materials: on a hit the replay
 * reads none of them, and the material rows, snapshots and validation were
 * 48K ticks a frame of work whose only consumer was the record path. The
 * answer is exact because the replay evaluates the same predicate on the same
 * inputs a moment later. */
s32 ndsRendererFighterPacketPrecheck(
    u32 slot,
    u32 use_low_detail,
    u32 texture_memo_owner_key,
    u32 packet_key,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count)
{
    u32 battle_slot = (texture_memo_owner_key >> 9) & 3u;
    u32 key[NDS_FIGHTER_PACKET_KEY_WORDS];

#if NDS_RENDERER_FRAME_SUMMARY_COUNTERS
    /* Window start for a record this frame: everything the slot's draw
     * accrues from here to the finish is what a replay hit skips. */
    sNdsFighterPacketRecordBase[0] =
        sNdsRendererRuntimeFrameSummary.hardware_batch_begin_count;
    sNdsFighterPacketRecordBase[1] =
        sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count;
    sNdsFighterPacketRecordBase[2] =
        sNdsRendererRuntimeFrameSummary.hardware_batch_end_count;
    sNdsFighterPacketRecordBase[3] =
        sNdsRendererRuntimeFrameSummary.texture_prepare_count;
    sNdsFighterPacketRecordBase[4] =
        sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count;
    sNdsFighterPacketRecordBase[5] =
        sNdsRendererRuntimeFrameSummary.matrix_load_count;
    sNdsFighterPacketRecordBase[6] =
        sNdsRendererRuntimeFrameSummary.texture_binds;
#endif
    sNdsFighterPacketRecordBase[7] = sNdsRendererHardwareSourceVertexLoadCount;
    if ((inputs == NULL) || (input_count == 0u) ||
        (input_count > NDS_FIGHTER_PACKET_ROOT_MAX) ||
        (ndsRendererNativeSelectFighterRuntimeTables(
             slot, use_low_detail) == FALSE))
    {
        return FALSE;
    }
    ndsFighterPacketBuildKey(texture_memo_owner_key, packet_key,
                             inputs, input_count, key);
    return ndsFighterPacketMatches(&sNdsFighterPackets[battle_slot], key,
                                   input_count);
}

/* The per-frame path. A hit patches the moving words, flushes, DMAs the
 * packet and leaves every CPU-side GX tracker invalidated so the next writer
 * re-issues its state. A miss arms a record into the slot's arena region and
 * resets the same trackers, so the record frame's first root writes -- and
 * the packet captures -- every word a self-contained replay needs. Returns 1
 * on a hit, 0 when the caller must run the ordinary path. */
static s32 __attribute__((noinline)) ndsFighterPacketTryReplay(
    u32 owner_slot,
    u32 use_low_detail,
    u32 texture_memo_owner_key,
    u32 packet_key,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count,
    NDSRendererStats *stats,
    u32 *out_hardware_started)
{
    u32 battle_slot = (texture_memo_owner_key >> 9) & 3u;
    NDSFighterPacket *packet = &sNdsFighterPackets[battle_slot];
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;
    u32 key[NDS_FIGHTER_PACKET_KEY_WORDS];
    u32 region_words;
    u32 region_base;
    u32 i;

    sNdsFighterPacketRecording = 0u;
    rec->packet = NULL;
#if NDS_P2_CAPTAIN
    /* Captain HIGH is the only generated Captain detail containing source
     * G_SETOTHERMODE_L / G_AC_THRESHOLD plus G_SETBLENDCOLOR. Alpha-test
     * enable/reference are DS register state, not geometry-FIFO commands, so a
     * FIFO-only packet cannot replay that HIGH-detail cutout epoch faithfully.
     *
     * BattleShip selects LOW detail for 3+ fighters. The source-derived LOW
     * program contains zero 0xe2 (SETOTHERMODE_L) and zero 0xf9
     * (SETBLENDCOLOR) deltas -- the owner generator records this distinction
     * explicitly, and its Captain comment names HIGH as the first detail to
     * contain either opcode. LOW therefore has no out-of-FIFO alpha register
     * state to preserve, and may use the same packet replay contract as the
     * other low-detail fighters. Keep only HIGH on the direct path. */
    if ((owner_slot == 4u) &&
        (use_low_detail == 0u))
    {
        gNdsFighterPacketDeclines++;
        return 0;
    }
#endif
#if NDS_P2_LINK
    /* P2-3f31. Link's G_TEXTURE_GEN coordinates depend on the live current
     * modelview and LookAt vectors. Packet replay patches matrices, lights and
     * tint, but its recorded TEX_COORD words are otherwise immutable. Until
     * texgen coordinates are explicit patch sites, replaying Link would freeze
     * source-derived UVs from the record frame. Decline before arming a record;
     * every other owner keeps the existing packet path unchanged. */
    if (owner_slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_LINK)
    {
        gNdsFighterPacketDeclines++;
        return 0;
    }
#else
    (void)owner_slot;
#endif
    if ((input_count == 0u) || (input_count > NDS_FIGHTER_PACKET_ROOT_MAX))
    {
        gNdsFighterPacketDeclines++;
        return 0;
    }
    ndsFighterPacketBuildKey(texture_memo_owner_key, packet_key,
                             inputs, input_count, key);
    if (ndsFighterPacketMatches(packet, key, input_count) != FALSE)
    {
        u32 *words = packet->words;

        ndsFighterPacketTouchTextures(packet);
        ndsFighterPacketApplyTint(packet, inputs);
        if (packet->projection_index != NDS_FIGHTER_PACKET_INDEX_NONE)
        {
            ndsFighterPacketStoreMatrix4x4(
                &words[packet->projection_index],
                inputs[0].projection_matrix);
        }
        for (i = 0u; i < input_count; i++)
        {
            const NDSRendererNativeFighterRoot *input = &inputs[i];
            const NDSFighterPacketRoot *root = &packet->roots[i];
            u32 j;

            if (root->seed_index != NDS_FIGHTER_PACKET_INDEX_NONE)
            {
                ndsFighterPacketStoreMatrix4x4(
                    &words[root->seed_index], input->gx_seed);
            }
            for (j = 0u; j < (u32)root->local_count; j++)
            {
                if (root->local_index[j] != NDS_FIGHTER_PACKET_INDEX_NONE)
                {
                    ndsFighterPacketStoreMatrix4x3(
                        &words[root->local_index[j]], &input->gx_locals[j]);
                }
            }
        }
        if ((packet->light_index != NDS_FIGHTER_PACKET_INDEX_NONE) &&
            (packet->light_valid != 0u) &&
            ((u32)packet->light_root < input_count))
        {
            const NDSRendererNativeFighterPreamble *preamble =
                inputs[packet->light_root].preamble;

            if ((preamble->flags &
                 NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID) != 0u)
            {
                words[packet->light_index] = ndsFighterPacketLightWord(
                    preamble->light_dir_x, preamble->light_dir_y,
                    preamble->light_dir_z);
            }
        }
        DC_FlushRange(words, packet->word_count * sizeof(u32));

        ndsRendererHardwareEndBatch();
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_FOG);
        while ((DMA_CR(0) & DMA_BUSY) != 0u) { }
        DMA_SRC(0) = (u32)(uintptr_t)words;
        DMA_DEST(0) = (u32)(uintptr_t)&GFX_FIFO;
        DMA_CR(0) = DMA_FIFO | packet->word_count;
        /* Not waited here: the next FIFO writer waits (P2-2p3). */
        sNdsFighterPacketDmaPending = 1u;

        ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
        sNdsRendererHardwareBoundTextureName = 0u;
        sNdsRendererHardwareActiveTextureEntry = NULL;
        sNdsR2GxLastProjection = NULL;
        sNdsRendererHardwareMatrixMode =
            NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED;
        sNdsRendererHardwareMatrixGeneration = ndsRendererNextMatrixGeneration();
        sNdsRendererHardwareMatrixLoaded = FALSE;

        if (stats->first_opcode == 0u)
        {
            stats->first_opcode = NDS_RENDERER_OP_RDPPIPESYNC;
        }
        if (packet->raw_triangles != 0u)
        {
            ndsRendererFastAccountRawTriangles(
                stats, packet->raw_triangles, packet->raw_reuse);
        }
        if (packet->cross_triangles != 0u)
        {
            ndsRendererNativeAccountGXCrossTriangles(
                stats, packet->cross_triangles, packet->cross_reuse);
        }
#if NDS_RENDERER_FRAME_SUMMARY_COUNTERS
        /* The replayed stream carries the record frame's BEGIN/END pairs and
         * texture binds verbatim; credit them as the record frame did. */
        sNdsRendererRuntimeFrameSummary.hardware_batch_begin_count +=
            packet->batch_begin;
        sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count +=
            packet->batch_reuse;
        sNdsRendererRuntimeFrameSummary.hardware_batch_end_count +=
            packet->batch_end;
        sNdsRendererRuntimeFrameSummary.texture_prepare_count +=
            packet->prepare_begin;
        sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count +=
            packet->prepare_reuse;
        sNdsRendererRuntimeFrameSummary.matrix_load_count +=
            packet->matrix_loads;
        sNdsRendererRuntimeFrameSummary.texture_binds +=
            packet->texture_binds;
#endif
        sNdsRendererHardwareSourceVertexLoadCount += packet->vertex_loads;
        stats->triangle_count += packet->triangle_count;
        sNdsRendererFastRunCount += packet->run_count;
        sNdsRendererFastTriangleCount += packet->triangle_count;
        if ((u32)sNdsRendererRuntimeOwner <
            (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
        {
            sNdsRendererFastOwnerTriangleCount[
                (u32)sNdsRendererRuntimeOwner] += packet->triangle_count;
        }
        *out_hardware_started = TRUE;
        gNdsFighterPacketHits++;
        return 1;
    }

    /* Miss: arm a record into this slot's region. Two-fighter (High detail)
     * matches get half the arena each; three or more (the source's Low
     * JointTree) a quarter. */
    if (packet->valid != 0u)
    {
        /* Which key words moved, for the churn census: a stale packet that
         * re-records every frame costs the record path, not the replay. */
        for (i = 0u; i < NDS_FIGHTER_PACKET_KEY_WORDS; i++)
        {
            u32 want = ((i == 5u) && (packet->needs_fence == 0u)) ? 0u : key[i];

            if (packet->key[i] != want)
            {
                gNdsFighterPacketMissWord[i]++;
            }
        }
        if (packet->root_count != input_count)
        {
            gNdsFighterPacketMissWord[NDS_FIGHTER_PACKET_KEY_WORDS]++;
        }
        if (ndsFighterPacketTexturesResident(packet) == FALSE)
        {
            gNdsFighterPacketMissWord[NDS_FIGHTER_PACKET_KEY_WORDS + 1u]++;
        }
    }
    packet->valid = 0u;
    region_words = (use_low_detail != 0u) ?
        (NDS_FIGHTER_PACKET_ARENA_WORDS / 4u) :
        (NDS_FIGHTER_PACKET_ARENA_WORDS / 2u);
    region_base = battle_slot * region_words;
    if (region_base + region_words > NDS_FIGHTER_PACKET_ARENA_WORDS)
    {
        gNdsFighterPacketDeclines++;
        return 0;
    }
    for (i = 0u; i < NDS_FIGHTER_PACKET_KEY_WORDS; i++)
    {
        packet->key[i] = key[i];
    }
    packet->words = (u32 *)(void *)&gSYFramebufferSets[0][0][0] + region_base;
    packet->word_capacity = region_words;
    packet->word_count = 0u;
    packet->root_count = input_count;
    packet->projection_index = NDS_FIGHTER_PACKET_INDEX_NONE;
    packet->light_index = NDS_FIGHTER_PACKET_INDEX_NONE;
    packet->light_root = 0u;
    packet->light_valid = 0u;
    packet->needs_fence = 0u;
    packet->texture_count = 0u;
    packet->site_count = 0u;
    packet->tint_modulate = inputs[0].config->color_modulate;
    packet->tint_prim_hash = 2166136261u;
    for (i = 0u; i < input_count; i++)
    {
        packet->tint_prim_hash = ndsFighterPacketMix(
            packet->tint_prim_hash, inputs[i].preamble->prim_color);
    }
    rec->packet = packet;
    rec->inputs = inputs;
    rec->prim_overridden = 0u;
    rec->words = packet->words;
    rec->count = 0u;
    rec->capacity = packet->word_capacity;
    rec->cmd_slot = 4u;
    rec->cmd_word = 0u;
    rec->header_params = 0u;
    rec->header_valid = 0u;
    rec->fault = 0u;
    rec->current_root = 0u;
    /* Self-contained stream: forget every GX tracker so the first root
     * re-issues -- and the packet captures -- its matrix mode, texture and
     * polygon attributes. */
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsFighterPacketRecording = 1u;
    gNdsFighterPacketRecords++;
    return 0;
}

void ndsRendererFighterPacketDmaWait(void)
{
    ndsFighterPacketDmaWait();
}

void ndsRendererFighterPacketInvalidateAll(void)
{
    u32 i;

    /* A preview/costume rebuild happens on the update side, normally between
     * draws, but wait anyway: the borrowed arena must not be considered free
     * for a new recording while DMA0 can still be consuming an old stream. */
    ndsFighterPacketDmaWait();
    sNdsFighterPacketRecording = 0u;
    sNdsFighterPacketRecorder.packet = NULL;
    for (i = 0u; i < NDS_FIGHTER_PACKET_SLOTS; i++)
    {
        sNdsFighterPackets[i].valid = 0u;
    }
}

void ndsRendererFighterPacketInvalidateSlot(u32 slot)
{
    if (slot >= NDS_FIGHTER_PACKET_SLOTS)
    {
        ndsRendererFighterPacketInvalidateAll();
        return;
    }
    /* Player-select rebuilds happen on the update side, between draws.  Still
     * honor the packet arena's DMA lifetime before making one region reusable;
     * a future caller reaching this seam earlier must not race DMA0. */
    ndsFighterPacketDmaWait();
    sNdsFighterPacketRecording = 0u;
    sNdsFighterPacketRecorder.packet = NULL;
    sNdsFighterPackets[slot].valid = 0u;
}

void ndsRendererFighterPacketRelease(void)
{
    u16 *arena = &gSYFramebufferSets[0][0][0];
    u32 i;

    ndsRendererFighterPacketInvalidateAll();
    /* scmanager.c's boot clear, GPACK_RGBA5551(0, 0, 0, 1), over the words
     * the packets borrowed, so the Results photo wipe reads what it always
     * read. */
    for (i = 0u; i < NDS_FIGHTER_PACKET_ARENA_WORDS * 2u; i++)
    {
        arena[i] = 0x0001u;
    }
}
#else
void ndsRendererFighterPacketDmaWait(void)
{
}

void ndsRendererFighterPacketInvalidateAll(void)
{
}

void ndsRendererFighterPacketInvalidateSlot(u32 slot)
{
    (void)slot;
}

void ndsRendererFighterPacketRelease(void)
{
}

s32 ndsRendererFighterPacketPrecheck(
    u32 slot,
    u32 use_low_detail,
    u32 texture_memo_owner_key,
    u32 packet_key,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count)
{
    (void)slot;
    (void)use_low_detail;
    (void)texture_memo_owner_key;
    (void)packet_key;
    (void)inputs;
    (void)input_count;
    return FALSE;
}
#endif

static s32 ndsRendererNativeSubmitProductionRun(
    const NDSNativeRun *run,
    u32 epoch_policy,
    u32 current_palette_slot,
    const u8 *binding_palette_slots,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 *raw_triangle_count,
    u32 *raw_reuse_count,
    u32 *cross_triangle_count,
    u32 *cross_reuse_count)
{
    u32 run_index;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    volatile NDSRendererOwnerProfile *m2_owner =
        ndsRendererProfileM2Owner();
    u32 m2_phase_start = 0u;
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    u32 e15_t0;
    u32 e15_mark;
#endif

    if ((run == NULL) || (run->triangle_count == 0u))
    {
        return ndsRendererNativeDirectReject(stats);
    }
#if NDS_TASK91_DRAW_PHASE_CENSUS
    e15_t0 = cpuGetTiming();
    e15_mark = e15_t0;
    gNdsR2SubmitCalls++;
#endif
    run_index = (u32)(run - sNdsNativeFighterActiveTables->runs);
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP
    (void)epoch_policy;
    (void)current_palette_slot;
    (void)binding_palette_slots;
    (void)config;
    (void)state;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_phase_start = cpuGetTiming();
#endif
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_phase_start = cpuGetTiming();
#endif
    if (ndsRendererNativePrepareProductionRun(
            run_index, epoch_policy,
            FALSE,
            config, stats, state) == FALSE)
    {
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        if (m2_owner != NULL)
        {
            m2_owner->m2_run_prepare_ticks +=
                cpuGetTiming() - m2_phase_start;
            m2_owner->m2_run_prepare_count++;
        }
#endif
        return FALSE;
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    if (m2_owner != NULL)
    {
        m2_owner->m2_run_prepare_ticks +=
            cpuGetTiming() - m2_phase_start;
        m2_owner->m2_run_prepare_count++;
    }
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    e15_mark = cpuGetTiming();
    gNdsR2SubmitPrepTicks += e15_mark - e15_t0;
#endif
    if ((run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX) &&
        (current_palette_slot > NDS_NATIVE_GX_MATRIX_SLOT_MAX))
    {
        return ndsRendererNativeDirectReject(stats);
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_phase_start = cpuGetTiming();
#endif
    if (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
    {
#if NDS_FIGHTER_PACKET_LIVE && (NDS_TASK56_FIGHTER_PRIMITIVES >= 1)
        if (sNdsFighterPacketRecording != 0u)
        {
            ndsRendererNativeEmitProductionCrossRunPacket(
                run_index, (u32)run->triangle_count * 3u,
                state->texture_prepare_enabled,
                current_palette_slot, binding_palette_slots);
        }
        else
#endif
        ndsRendererNativeEmitProductionCrossRun(
            run_index, (u32)run->triangle_count * 3u,
            state->texture_prepare_enabled,
            current_palette_slot, binding_palette_slots);
#if NDS_TASK91_DRAW_PHASE_CENSUS
        gNdsR2SubmitCrossEmitTicks += cpuGetTiming() - e15_mark;
        gNdsR2SubmitCrossCalls++;
        gNdsR2SubmitCrossTriangles += run->triangle_count;
#endif
    }
    else
    {
#if (NDS_TASK56_FIGHTER_PRIMITIVES >= 1) && \
    (NDS_RENDERER_PROFILE_LEVEL < 2) && NDS_RENDERER_HW_TRIANGLES
        /* Task 56: RAW runs emit DS-native primitive groups (strips + residual
         * triangles) compiled host-side. The first group's begin-batch already
         * ran in PrepareProductionRun; this walk re-begins only on type change
         * and leaves GL_TRIANGLE selected for whoever reuses the batch. Only
         * RAW runs come here -- the cross-matrix branch above owns its own
         * emitter -- which is why the group vertex refs are bare dense ids,
         * exactly as the raw emitters read the packed corners unmasked. */
        if (NDS_R2_STRIP_ROUTE_ON())
        {
#if NDS_FIGHTER_PACKET_LIVE
            if (sNdsFighterPacketRecording != 0u)
            {
                ndsRendererNativeEmitProductionPrimitiveGroupsPacket(
                    run_index, state->texture_prepare_enabled);
            }
            else
#endif
            ndsRendererNativeEmitProductionPrimitiveGroups(
                run_index, state->texture_prepare_enabled);
        }
        else
#endif
        if (state->texture_prepare_enabled != 0u)
        {
            /* The raw emitters have no packet twin: a record frame that
             * reaches them faults the packet and keeps drawing. */
            NDS_FIGHTER_PACKET_HOOK(sNdsFighterPacketRecorder.fault = 1u);
            ndsRendererNativeEmitProductionRawTexturedRun(
                run_index, (u32)run->triangle_count * 3u);
        }
        else
        {
            NDS_FIGHTER_PACKET_HOOK(sNdsFighterPacketRecorder.fault = 1u);
            ndsRendererNativeEmitProductionRawUntexturedRun(
                run_index, (u32)run->triangle_count * 3u);
        }
#if NDS_TASK91_DRAW_PHASE_CENSUS
        gNdsR2SubmitRawEmitTicks += cpuGetTiming() - e15_mark;
        gNdsR2SubmitRawCalls++;
        gNdsR2SubmitRawTriangles += run->triangle_count;
#endif
    }
#else
    (void)epoch_policy;
    (void)current_palette_slot;
    (void)binding_palette_slots;
    (void)config;
    (void)state;
    return ndsRendererNativeDirectReject(stats);
#endif
    stats->triangle_count += run->triangle_count;
    if (run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT)
    {
        *raw_triangle_count += run->triangle_count;
        *raw_reuse_count += run->triangle_count - 1u;
    }
    else if (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
    {
        *cross_triangle_count += run->triangle_count;
        *cross_reuse_count += run->triangle_count - 1u;
    }
    else
    {
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        if (m2_owner != NULL)
        {
            m2_owner->m2_corner_emit_account_ticks +=
                cpuGetTiming() - m2_phase_start;
            m2_owner->m2_corner_emit_run_count++;
        }
#endif
        return ndsRendererNativeDirectReject(stats);
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    if (m2_owner != NULL)
    {
        m2_owner->m2_corner_emit_account_ticks +=
            cpuGetTiming() - m2_phase_start;
        m2_owner->m2_corner_emit_run_count++;
    }
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    gNdsR2SubmitTotalTicks += cpuGetTiming() - e15_t0;
#endif
    return TRUE;
}


static void NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE
ndsRendererNativeEmitDenseRawRun(
    u32 run_index,
    u32 corner_count,
    const NDSRendererTraversalState *state,
    u32 textured)
{
    const u16 *corner =
        &sNdsNativeFighterDenseCorners[
            sNdsNativeFighterRunFirstCorner[run_index]];
    u32 remaining = corner_count;

    if (textured != 0u)
    {
        while (remaining-- != 0u)
        {
            u32 dense_id = *corner++;
            const NDSNativeDenseVertex *vertex =
                &sNdsNativeFighterDenseVertices[dense_id];
            const NDSNativePreparedDenseVertex *prepared =
                &sNdsNativeFighterPreparedDense[dense_id];
            u32 slot = vertex->cache_slot;

            ndsRendererHardwareWriteFighterColorWord(
                state->prepared_vertex_colors[slot]);
            ndsRendererHardwareWriteFighterTexCoordWord(
                (u32)(u16)state->prepared_texcoord_s[slot] |
                ((u32)(u16)state->prepared_texcoord_t[slot] << 16));
            ndsRendererHardwareWriteFighterVertex16Words(
                prepared->gx_xy, prepared->gx_z);
        }
    }
    else
    {
        while (remaining-- != 0u)
        {
            u32 dense_id = *corner++;
            const NDSNativeDenseVertex *vertex =
                &sNdsNativeFighterDenseVertices[dense_id];
            const NDSNativePreparedDenseVertex *prepared =
                &sNdsNativeFighterPreparedDense[dense_id];
            u32 slot = vertex->cache_slot;

            ndsRendererHardwareWriteFighterColorWord(
                state->prepared_vertex_colors[slot]);
            ndsRendererHardwareWriteFighterVertex16Words(
                prepared->gx_xy, prepared->gx_z);
        }
    }
}

static inline void ndsRendererNativeAccountProjectedCrossTriangle(
    NDSRendererStats *stats,
    u32 triangle_count)
{
    u32 reuse_count = (triangle_count != 0u) ?
        triangle_count - 1u : 0u;

    sNdsRendererHardwareSubmitClassCounts[
        NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX] += triangle_count;
    sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count += triangle_count;
    sNdsRendererRuntimeFrameSummary.projected_submit_fallback_count +=
        triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices +=
        triangle_count * 3u;
    if ((sNdsRendererRuntimeFrameSummary.hardware_triangles > 2048u) ||
        (sNdsRendererRuntimeFrameSummary.hardware_vertices > 6144u))
    {
        sNdsRendererRuntimeFrameSummary.hardware_over_limit = 1u;
    }
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    stats->hardware_projected_depth_triangle_count += triangle_count;
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount += triangle_count;
#endif
    sNdsRendererHardwareSubmitted = TRUE;
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
static s32 ndsRendererNativeSubmitRunDirect(
    const NDSNativeRun *run,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP
    if ((run == NULL) || (config == NULL) || (stats == NULL) ||
        (state == NULL) || (run->triangle_count == 0u))
    {
        return FALSE;
    }
    /* This benchmark measures everything surrounding triangle transport.
     * Consume the generated run as one unit instead of paying the generic
     * per-triangle fallback loop that production never executes. */
    stats->triangle_count += run->triangle_count;
    if (run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT)
    {
        ndsRendererFastAccountRawTriangles(
            stats, run->triangle_count,
            (run->triangle_count != 0u) ? run->triangle_count - 1u : 0u);
    }
    else
    {
        ndsRendererNativeAccountProjectedCrossTriangle(
            stats, run->triangle_count);
    }
    return TRUE;
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
    u32 emitted_triangles = 0u;
    u32 run_index;
    u32 i;

    if ((run == NULL) || (config == NULL) || (stats == NULL) ||
        (state == NULL) || (run->triangle_count == 0u))
    {
        return FALSE;
    }
    run_index = (u32)(run - sNdsNativeFighterRuns);
    if (run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT)
    {
        const u16 *triangles =
            &sNdsNativeFighterTriangles[run->first_triangle];
        u32 first_indices[3];

        (void)ndsRendererNativeDecodeTriangle(
            triangles[0], first_indices);
        if (ndsRendererNativePrepareDirectRun(
                run, first_indices[0], FALSE,
                config, stats, state) == FALSE)
        {
            return FALSE;
        }
        stats->triangle_count += run->triangle_count;
        ndsRendererNativeEmitDenseRawRun(
            run_index,
            (u32)run->triangle_count * 3u,
            state, state->texture_prepare_enabled);
        ndsRendererFastAccountRawTriangles(
            stats, run->triangle_count, run->triangle_count - 1u);
        return TRUE;
    }
    if (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
    {
        const u16 *triangles =
            &sNdsNativeFighterTriangles[run->first_triangle];
        u32 first_indices[3];

        (void)ndsRendererNativeDecodeTriangle(triangles[0], first_indices);
        if (ndsRendererNativePrepareDirectRun(
                run, first_indices[0], TRUE,
                config, stats, state) == FALSE)
        {
            return FALSE;
        }
        stats->triangle_count += run->triangle_count;
        for (i = 0u; i < run->triangle_count; i++)
        {
            u32 indices[3];

            (void)ndsRendererNativeDecodeTriangle(
                triangles[i], indices);
            if (ndsRendererHardwareTriangleInsideNearPlane(
                    &state->vertices[indices[0]],
                    &state->vertices[indices[1]],
                    &state->vertices[indices[2]]) == FALSE)
            {
                /* BUGS.md #10. This is the path fighters actually draw
                 * through, and it dropped the triangles the source RSP
                 * clips -- after triangle_count above already counted the
                 * whole run, which is why every instrument read 320/320
                 * while Mario's joint seams opened up. Clip and fan. */
                u32 fanned = ndsRendererHardwareSubmitNearClippedTriangle(
                    stats, state, indices[0], indices[1], indices[2], 0);

                if (fanned == 0u)
                {
                    ndsRendererProfileRecordNearPlaneTriangleReject();
                    ndsRendererProfileRecordSubmitClass(
                        NDS_RENDERER_HW_SUBMIT_REJECT);
                    continue;
                }
                emitted_triangles += fanned;
                continue;
            }
            ndsRendererHardwareSubmitVertex(
                stats, state, indices[0], 0);
            ndsRendererHardwareSubmitVertex(
                stats, state, indices[1], 0);
            ndsRendererHardwareSubmitVertex(
                stats, state, indices[2], 0);
            emitted_triangles++;
        }
        ndsRendererHardwareEnterProjectedForeground();
        if (emitted_triangles != 0u)
        {
            ndsRendererNativeAccountProjectedCrossTriangle(
                stats, emitted_triangles);
        }
        return TRUE;
    }
#else
    (void)run;
    (void)config;
    (void)stats;
    (void)state;
#endif
    return FALSE;
}
#endif

static void ndsRendererNativeSubmitRun(
    const NDSNativeRun *run,
    const u8 *root_base,
    NDSRendererCommandCallback callback,
    void *callback_user,
    u32 *last_callback_command,
    u32 *source_command_index,
    u32 *tri2_half,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const u16 *triangle;
    s32 native_raw_ready = FALSE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    s32 native_snapshot_ready = FALSE;
    u32 native_raw_submitted = 0u;
    u32 native_cross_submitted = 0u;
#endif
    u32 i;

    stats->triangle_count += run->triangle_count;
    triangle = &sNdsNativeFighterTriangles[run->first_triangle];
    if ((run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT) &&
        (run->triangle_count != 0u) &&
        (NDS_RENDERER_BENCHMARK_MODE !=
         NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP))
    {
        u32 first_indices[3];

        (void)ndsRendererNativeDecodeTriangle(
            triangle[0], first_indices);
        native_raw_ready = ndsRendererNativePrepareDirectRun(
            run, first_indices[0], FALSE,
            config, stats, state);
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    else if ((run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX) &&
             (run->triangle_count != 0u) &&
             (NDS_RENDERER_BENCHMARK_MODE !=
              NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP))
    {
        u32 first_indices[3];

        (void)ndsRendererNativeDecodeTriangle(
            triangle[0], first_indices);
        native_snapshot_ready = ndsRendererNativePrepareDirectRun(
            run, first_indices[0], TRUE,
            config, stats, state);
    }
#endif
    for (i = 0u; i < run->triangle_count; i++)
    {
        u16 encoded = triangle[i];
        u32 indices[3];
        u32 packed = ndsRendererNativeDecodeTriangle(encoded, indices);
        u32 command_index = *source_command_index;
        u32 command_half = *tri2_half;

        if (*last_callback_command != command_index)
        {
            if (ndsRendererNativeVisitSourceCommand(
                    root_base, command_index,
                    callback, callback_user, stats, state) == FALSE)
            {
                return;
            }
            *last_callback_command = command_index;
        }
        if (((run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT) &&
             (native_raw_ready == FALSE)) ||
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ((run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX) &&
             (native_snapshot_ready == FALSE)) ||
#else
            (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX) ||
#endif
            (NDS_RENDERER_BENCHMARK_MODE ==
             NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP))
        {
            if ((run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT) &&
                (i != 0u) &&
                (ndsRendererFastRawStateEligible(state) == FALSE))
            {
                sNdsRendererFastFallbackCount[0]++;
            }
            ndsRendererNativeSubmitGenericTriangle(
                packed, command_index, command_half,
                config, stats, state);
            if ((run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT) &&
                (i == 0u) &&
                (ndsRendererFastRawStateEligible(state) != FALSE))
            {
                ndsRendererFastPrepareRawSlots(
                    stats, state, run->required_mask,
                    state->texture_prepare_enabled);
                native_raw_ready = TRUE;
            }
        }
        else
        {
#if NDS_RENDERER_PROFILE_LEVEL < 2
            if (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
            {
                if (ndsRendererHardwareTriangleInsideNearPlane(
                        &state->vertices[indices[0]],
                        &state->vertices[indices[1]],
                        &state->vertices[indices[2]]) == FALSE)
                {
                    /* BUGS.md #10, same defect as the run emitter above. */
                    u32 fanned =
                        ndsRendererHardwareSubmitNearClippedTriangle(
                            stats, state, indices[0], indices[1],
                            indices[2], 0);

                    if (fanned == 0u)
                    {
                        ndsRendererProfileRecordNearPlaneTriangleReject();
                        ndsRendererProfileRecordSubmitClass(
                            NDS_RENDERER_HW_SUBMIT_REJECT);
                        continue;
                    }
                    ndsRendererHardwareEnterProjectedForeground();
                    native_cross_submitted += fanned;
                    continue;
                }
                ndsRendererHardwareSubmitVertex(
                    stats, state, indices[0], 0);
                ndsRendererHardwareSubmitVertex(
                    stats, state, indices[1], 0);
                ndsRendererHardwareSubmitVertex(
                    stats, state, indices[2], 0);
                ndsRendererHardwareEnterProjectedForeground();
                native_cross_submitted++;
            }
            else
#endif
            {
            if (sNdsRendererHardwareNoOracle == 0u)
            {
                ndsRendererRecordTransformedTriangle(
                    stats, state, packed);
            }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            state->semantic_command_index = command_index;
            state->semantic_tri2_half = command_half;
#endif
            ndsRendererFastEmitRawCommand(
                state, indices, 1u, state->texture_prepare_enabled);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            ndsRendererFastCommitRawSemanticTriangle(
                stats, state, packed, indices);
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
            native_raw_submitted++;
#else
            ndsRendererFastAccountRawTriangles(
                stats, 1u, (i == 0u) ? 0u : 1u);
#endif
            }
        }
        if ((encoded & 0x8000u) != 0u)
        {
            *tri2_half = 1u;
        }
        else
        {
            (*source_command_index)++;
            *tri2_half = 0u;
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (native_raw_submitted != 0u)
    {
        ndsRendererFastAccountRawTriangles(
            stats, native_raw_submitted,
            native_raw_submitted - 1u);
    }
    if (native_cross_submitted != 0u)
    {
        ndsRendererNativeAccountProjectedCrossTriangle(
            stats, native_cross_submitted);
    }
#endif
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static void ndsRendererNativeBindOwnerRootState(
    NDSRendererTraversalState *state,
    const NDSRendererConfig *config,
    NDSRendererStats *stats)
{
    state->modelview_stack_depth = 0u;
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_projected_xy_valid_mask = 0u;
    state->prepared_projected_source_z_valid_mask = 0u;
    state->prepared_light_direction_valid = 0u;
    state->texture_prepare_valid = 0u;
    state->projection_valid = 0u;
    state->modelview_valid = 0u;
    state->matrix_valid = 0u;
    state->matrix_word_valid = 0u;
    if ((stats != NULL) && (config->initial_geometry_mode != 0u))
    {
        stats->geometry_mode = config->initial_geometry_mode;
    }
    if (config->initial_projection != NULL)
    {
        state->projection = *config->initial_projection;
        state->projection_valid = TRUE;
    }
    if (config->initial_modelview != NULL)
    {
        state->modelview = *config->initial_modelview;
        state->modelview_valid = TRUE;
    }
    ndsRendererComposeMatrix(state);
    if ((stats != NULL) && (state->matrix_valid != 0u))
    {
        stats->hardware_matrix_seed_count++;
    }
}
#endif

static s32 ndsRendererExecuteNativeFighterRootHardware(
    u32 slot,
    u32 root_ordinal,
    const void *asset_base_ptr,
    u32 root_offset,
    const NDSRendererNativeMaterial *materials,
    u32 material_count,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
    const NDSNativeRoot *roots;
    const NDSNativeRoot *root;
    const u8 *asset_base = asset_base_ptr;
    const u8 *root_base;
    u32 root_count;
    u32 epoch_index;
    u32 native_triangle_count = 0u;
    u32 native_run_count = 0u;
    u32 last_callback_command = 0xffffffffu;
    NDSRendererTraversalState local_state;
    NDSRendererTraversalState *state_ptr = &local_state;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    s32 shared_owner_state = FALSE;
#endif
#define state (*state_ptr)

    if ((asset_base == NULL) || (config == NULL) || (stats == NULL) ||
        (vertex_cache == NULL) || (slot > 1u))
    {
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    /* This legacy root entry point predates P2-2 and has no detail argument.
     * Keep its historical contract explicitly high-detail so a preceding low
     * owner cannot leave the shared serialized table view pointed elsewhere. */
    if (ndsRendererNativeSelectFighterRuntimeTables(slot, FALSE) == FALSE)
    {
        return FALSE;
    }
#endif
    if (slot == 0u)
    {
        roots = sNdsNativeMarioRoots;
        root_count = sizeof(sNdsNativeMarioRoots) /
            sizeof(sNdsNativeMarioRoots[0]);
    }
    else
    {
        roots = sNdsNativeFoxRoots;
        root_count = sizeof(sNdsNativeFoxRoots) /
            sizeof(sNdsNativeFoxRoots[0]);
    }
    if ((root_ordinal >= root_count) ||
        (roots[root_ordinal].root_offset != root_offset))
    {
        return FALSE;
    }
    root = &roots[root_ordinal];
    for (epoch_index = 0u; epoch_index < root->epoch_count; epoch_index++)
    {
        const NDSNativeEpoch *epoch =
            &sNdsNativeFighterEpochs[root->first_epoch + epoch_index];

        if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
            ((materials == NULL) ||
             (epoch->material_slot >= material_count)))
        {
            return FALSE;
        }
    }

#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (sNdsNativeFighterOwnerExecution.active != 0u)
    {
        if ((sNdsNativeFighterOwnerExecution.slot != slot) ||
            (sNdsNativeFighterOwnerExecution.stats != stats) ||
            (sNdsNativeFighterOwnerExecution.vertex_cache != vertex_cache))
        {
            ndsRendererAbortNativeFighterOwner();
            return FALSE;
        }
        state_ptr = &sNdsNativeFighterOwnerExecution.traversal;
        shared_owner_state = TRUE;
        ndsRendererNativeBindOwnerRootState(&state, config, stats);
    }
    else
#endif
    {
        ndsRendererInitTraversalState(
            &state, config, stats, NULL,
            vertex_cache->matrix_snapshots,
            vertex_cache->matrix_snapshot_count);
        state.vertices = vertex_cache->transformed_vertices;
        state.vertex_valid_mask = vertex_cache->transformed_valid_mask;
        state.input_vertices = vertex_cache->input_vertices;
        state.input_vertex_valid_mask = vertex_cache->input_valid_mask;
        state.raw_vertex_fit_mask = vertex_cache->raw_vertex_fit_mask;
        state.vertex_colors = vertex_cache->vertex_colors;
        state.vertex_color_valid_mask =
            vertex_cache->vertex_color_valid_mask;
        state.vertex_matrix_snapshot = vertex_cache->vertex_matrix_snapshot;
        state.vertex_clip_snapshot = vertex_cache->vertex_clip_snapshot;
    }
    root_base = asset_base + root_offset;
    if (stats->first_opcode == 0u)
    {
        stats->first_opcode = NDS_RENDERER_OP_RDPPIPESYNC;
    }
    stats->command_count += root->source_command_count;
    ndsRendererNativeApplyRootLightPreamble(root, stats);
    for (epoch_index = 0u; epoch_index < root->epoch_count; epoch_index++)
    {
        const NDSNativeEpoch *epoch =
            &sNdsNativeFighterEpochs[root->first_epoch + epoch_index];
        u32 run_index;
        u32 source_command_index =
            epoch->first_triangle_command_index;
        u32 tri2_half = 0u;

        ndsRendererNativeApplyStateSpan(
            epoch->before_state_first, epoch->before_state_count,
            epoch->before_sync_count,
            asset_base, stats, &state);
        if (epoch->material_slot != NDS_NATIVE_MATERIAL_NONE)
        {
            ndsRendererNativeApplyMaterial(
                &materials[epoch->material_slot], stats, &state);
        }
        ndsRendererNativeApplyStateSpan(
            epoch->after_state_first, epoch->after_state_count,
            epoch->after_sync_count,
            asset_base, stats, &state);
        ndsRendererNativeApplyVertexActions(
            epoch, asset_base, root_base,
            callback, callback_user, stats, &state);
        if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            break;
        }
        for (run_index = 0u; run_index < epoch->run_count; run_index++)
        {
            const NDSNativeRun *run =
                &sNdsNativeFighterRuns[epoch->first_run + run_index];

#if NDS_RENDERER_PROFILE_LEVEL < 2
            if ((callback != NULL) ||
                (sNdsRendererHardwareNoOracle == 0u) ||
                (ndsRendererNativeSubmitRunDirect(
                     run, config, stats, &state) == FALSE))
#endif
            {
                ndsRendererNativeSubmitRun(
                    run, root_base, callback, callback_user,
                    &last_callback_command,
                    &source_command_index, &tri2_half,
                    config, stats, &state);
            }
            native_triangle_count += run->triangle_count;
            native_run_count++;
            if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
            {
                break;
            }
        }
        if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            break;
        }
    }
    if (stats->blocker == NDS_RENDERER_BLOCKER_NONE)
    {
        ndsRendererNativeApplyStateSpan(
            root->tail_state_first, root->tail_state_count,
            root->tail_sync_count,
            asset_base, stats, &state);
        stats->end_command_count++;
    }
    ndsRendererHardwareEndBatch();
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (shared_owner_state == FALSE)
#endif
    {
        vertex_cache->transformed_valid_mask = state.vertex_valid_mask;
        vertex_cache->input_valid_mask = state.input_vertex_valid_mask;
        vertex_cache->raw_vertex_fit_mask = state.raw_vertex_fit_mask;
        vertex_cache->vertex_color_valid_mask =
            state.vertex_color_valid_mask;
        vertex_cache->matrix_snapshot_count = state.matrix_snapshot_count;
    }
    if (native_triangle_count != 0u)
    {
        sNdsRendererFastRunCount += native_run_count;
        sNdsRendererFastTriangleCount += native_triangle_count;
        if ((u32)sNdsRendererRuntimeOwner <
            (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
        {
            sNdsRendererFastOwnerTriangleCount[
                (u32)sNdsRendererRuntimeOwner] += native_triangle_count;
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if ((shared_owner_state != FALSE) &&
        (stats->blocker != NDS_RENDERER_BLOCKER_NONE))
    {
        ndsRendererAbortNativeFighterOwner();
        return FALSE;
    }
#endif
    return TRUE;
#undef state
}
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
typedef struct NDSNativeHierarchyTables
{
    const NDSNativeRoot *roots;
    const u16 *schedule;
    const u8 *binding_joints;
    const u8 *cross_slots;
    u32 root_count;
    u32 joint_count;
} NDSNativeHierarchyTables;

static s32 ndsRendererNativeGetHierarchyTables(
    u32 slot, NDSNativeHierarchyTables *tables)
{
    if ((slot >= NDS_NATIVE_FIGHTER_OWNER_COUNT) || (tables == NULL))
    {
        return FALSE;
    }
    if (slot == 0u)
    {
        tables->roots = sNdsNativeMarioRoots;
        tables->schedule = sNdsNativeMarioJointSchedule;
        tables->binding_joints = sNdsNativeMarioBindingJoints;
        tables->cross_slots = sNdsNativeMarioCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeMarioRoots) /
            sizeof(sNdsNativeMarioRoots[0]);
        tables->joint_count = sizeof(sNdsNativeMarioJointSchedule) /
            sizeof(sNdsNativeMarioJointSchedule[0]);
    }
    else if (slot == 1u)
    {
        tables->roots = sNdsNativeFoxRoots;
        tables->schedule = sNdsNativeFoxJointSchedule;
        tables->binding_joints = sNdsNativeFoxBindingJoints;
        tables->cross_slots = sNdsNativeFoxCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeFoxRoots) /
            sizeof(sNdsNativeFoxRoots[0]);
        tables->joint_count = sizeof(sNdsNativeFoxJointSchedule) /
            sizeof(sNdsNativeFoxJointSchedule[0]);
    }
#if NDS_P2_LUIGI
    else if (slot == 2u)
    {
        tables->roots = sNdsNativeLuigiRoots;
        tables->schedule = sNdsNativeLuigiJointSchedule;
        tables->binding_joints = sNdsNativeLuigiBindingJoints;
        tables->cross_slots = sNdsNativeLuigiCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeLuigiRoots) /
            sizeof(sNdsNativeLuigiRoots[0]);
        tables->joint_count = sizeof(sNdsNativeLuigiJointSchedule) /
            sizeof(sNdsNativeLuigiJointSchedule[0]);
    }
#endif
#if NDS_P2_DONKEY
    else if (slot == 3u)
    {
        tables->roots = sNdsNativeDonkeyRoots;
        tables->schedule = sNdsNativeDonkeyJointSchedule;
        tables->binding_joints = sNdsNativeDonkeyBindingJoints;
        tables->cross_slots = sNdsNativeDonkeyCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeDonkeyRoots) /
            sizeof(sNdsNativeDonkeyRoots[0]);
        tables->joint_count = sizeof(sNdsNativeDonkeyJointSchedule) /
            sizeof(sNdsNativeDonkeyJointSchedule[0]);
    }
#endif
#if NDS_P2_CAPTAIN
    else if (slot == 4u)
    {
        tables->roots = sNdsNativeCaptainRoots;
        tables->schedule = sNdsNativeCaptainJointSchedule;
        tables->binding_joints = sNdsNativeCaptainBindingJoints;
        tables->cross_slots = sNdsNativeCaptainCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeCaptainRoots) /
            sizeof(sNdsNativeCaptainRoots[0]);
        tables->joint_count = sizeof(sNdsNativeCaptainJointSchedule) /
            sizeof(sNdsNativeCaptainJointSchedule[0]);
    }
#endif
#if NDS_P2_SAMUS
    else if (slot == 5u)
    {
        tables->roots = sNdsNativeSamusRoots;
        tables->schedule = sNdsNativeSamusJointSchedule;
        tables->binding_joints = sNdsNativeSamusBindingJoints;
        tables->cross_slots = sNdsNativeSamusCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeSamusRoots) /
            sizeof(sNdsNativeSamusRoots[0]);
        tables->joint_count = sizeof(sNdsNativeSamusJointSchedule) /
            sizeof(sNdsNativeSamusJointSchedule[0]);
    }
#endif
#if NDS_P2_LINK
    else if (slot == 6u)
    {
        tables->roots = sNdsNativeLinkRoots;
        tables->schedule = sNdsNativeLinkJointSchedule;
        tables->binding_joints = sNdsNativeLinkBindingJoints;
        tables->cross_slots = sNdsNativeLinkCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeLinkRoots) /
            sizeof(sNdsNativeLinkRoots[0]);
        tables->joint_count = sizeof(sNdsNativeLinkJointSchedule) /
            sizeof(sNdsNativeLinkJointSchedule[0]);
    }
#endif
#if NDS_P2_PIKACHU
    else if (slot == 7u)
    {
        tables->roots = sNdsNativePikachuRoots;
        tables->schedule = sNdsNativePikachuJointSchedule;
        tables->binding_joints = sNdsNativePikachuBindingJoints;
        tables->cross_slots = sNdsNativePikachuCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativePikachuRoots) /
            sizeof(sNdsNativePikachuRoots[0]);
        tables->joint_count = sizeof(sNdsNativePikachuJointSchedule) /
            sizeof(sNdsNativePikachuJointSchedule[0]);
    }
#endif
#if NDS_P2_YOSHI
    else if (slot == 8u)
    {
        tables->roots = sNdsNativeYoshiRoots;
        tables->schedule = sNdsNativeYoshiJointSchedule;
        tables->binding_joints = sNdsNativeYoshiBindingJoints;
        tables->cross_slots = sNdsNativeYoshiCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeYoshiRoots) /
            sizeof(sNdsNativeYoshiRoots[0]);
        tables->joint_count = sizeof(sNdsNativeYoshiJointSchedule) /
            sizeof(sNdsNativeYoshiJointSchedule[0]);
    }
#endif
#if NDS_P2_NESS
    else if (slot == 9u)
    {
        tables->roots = sNdsNativeNessRoots;
        tables->schedule = sNdsNativeNessJointSchedule;
        tables->binding_joints = sNdsNativeNessBindingJoints;
        tables->cross_slots = sNdsNativeNessCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNessRoots) /
            sizeof(sNdsNativeNessRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNessJointSchedule) /
            sizeof(sNdsNativeNessJointSchedule[0]);
    }
#endif
#if NDS_P2_PURIN
    else if (slot == 10u)
    {
        tables->roots = sNdsNativePurinRoots;
        tables->schedule = sNdsNativePurinJointSchedule;
        tables->binding_joints = sNdsNativePurinBindingJoints;
        tables->cross_slots = sNdsNativePurinCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativePurinRoots) /
            sizeof(sNdsNativePurinRoots[0]);
        tables->joint_count = sizeof(sNdsNativePurinJointSchedule) /
            sizeof(sNdsNativePurinJointSchedule[0]);
    }
#endif
#if NDS_P2_KIRBY
    else if (slot == 11u)
    {
        tables->roots = sNdsNativeKirbyRoots;
        tables->schedule = sNdsNativeKirbyJointSchedule;
        tables->binding_joints = sNdsNativeKirbyBindingJoints;
        tables->cross_slots = sNdsNativeKirbyCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeKirbyRoots) /
            sizeof(sNdsNativeKirbyRoots[0]);
        tables->joint_count = sizeof(sNdsNativeKirbyJointSchedule) /
            sizeof(sNdsNativeKirbyJointSchedule[0]);
    }
#endif
#if NDS_P2_MMARIO
    else if (slot == 12u)
    {
        tables->roots = sNdsNativeMMarioRoots;
        tables->schedule = sNdsNativeMMarioJointSchedule;
        tables->binding_joints = sNdsNativeMMarioBindingJoints;
        tables->cross_slots = sNdsNativeMMarioCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeMMarioRoots) /
            sizeof(sNdsNativeMMarioRoots[0]);
        tables->joint_count = sizeof(sNdsNativeMMarioJointSchedule) /
            sizeof(sNdsNativeMMarioJointSchedule[0]);
    }
#endif
#if NDS_P2_NMARIO
    else if (slot == 13u)
    {
        tables->roots = sNdsNativeNMarioRoots;
        tables->schedule = sNdsNativeNMarioJointSchedule;
        tables->binding_joints = sNdsNativeNMarioBindingJoints;
        tables->cross_slots = sNdsNativeNMarioCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNMarioRoots) /
            sizeof(sNdsNativeNMarioRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNMarioJointSchedule) /
            sizeof(sNdsNativeNMarioJointSchedule[0]);
    }
#endif
#if NDS_P2_NFOX
    else if (slot == 14u)
    {
        tables->roots = sNdsNativeNFoxRoots;
        tables->schedule = sNdsNativeNFoxJointSchedule;
        tables->binding_joints = sNdsNativeNFoxBindingJoints;
        tables->cross_slots = sNdsNativeNFoxCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNFoxRoots) /
            sizeof(sNdsNativeNFoxRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNFoxJointSchedule) /
            sizeof(sNdsNativeNFoxJointSchedule[0]);
    }
#endif
#if NDS_P2_NDONKEY
    else if (slot == 15u)
    {
        tables->roots = sNdsNativeNDonkeyRoots;
        tables->schedule = sNdsNativeNDonkeyJointSchedule;
        tables->binding_joints = sNdsNativeNDonkeyBindingJoints;
        tables->cross_slots = sNdsNativeNDonkeyCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNDonkeyRoots) /
            sizeof(sNdsNativeNDonkeyRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNDonkeyJointSchedule) /
            sizeof(sNdsNativeNDonkeyJointSchedule[0]);
    }
#endif
#if NDS_P2_NSAMUS
    else if (slot == 16u)
    {
        tables->roots = sNdsNativeNSamusRoots;
        tables->schedule = sNdsNativeNSamusJointSchedule;
        tables->binding_joints = sNdsNativeNSamusBindingJoints;
        tables->cross_slots = sNdsNativeNSamusCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNSamusRoots) /
            sizeof(sNdsNativeNSamusRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNSamusJointSchedule) /
            sizeof(sNdsNativeNSamusJointSchedule[0]);
    }
#endif
#if NDS_P2_NLINK
    else if (slot == 17u)
    {
        tables->roots = sNdsNativeNLinkRoots;
        tables->schedule = sNdsNativeNLinkJointSchedule;
        tables->binding_joints = sNdsNativeNLinkBindingJoints;
        tables->cross_slots = sNdsNativeNLinkCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNLinkRoots) /
            sizeof(sNdsNativeNLinkRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNLinkJointSchedule) /
            sizeof(sNdsNativeNLinkJointSchedule[0]);
    }
#endif
#if NDS_P2_NYOSHI
    else if (slot == 18u)
    {
        tables->roots = sNdsNativeNYoshiRoots;
        tables->schedule = sNdsNativeNYoshiJointSchedule;
        tables->binding_joints = sNdsNativeNYoshiBindingJoints;
        tables->cross_slots = sNdsNativeNYoshiCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNYoshiRoots) /
            sizeof(sNdsNativeNYoshiRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNYoshiJointSchedule) /
            sizeof(sNdsNativeNYoshiJointSchedule[0]);
    }
#endif
#if NDS_P2_NCAPTAIN
    else if (slot == 19u)
    {
        tables->roots = sNdsNativeNCaptainRoots;
        tables->schedule = sNdsNativeNCaptainJointSchedule;
        tables->binding_joints = sNdsNativeNCaptainBindingJoints;
        tables->cross_slots = sNdsNativeNCaptainCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNCaptainRoots) /
            sizeof(sNdsNativeNCaptainRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNCaptainJointSchedule) /
            sizeof(sNdsNativeNCaptainJointSchedule[0]);
    }
#endif
#if NDS_P2_NKIRBY
    else if (slot == 20u)
    {
        tables->roots = sNdsNativeNKirbyRoots;
        tables->schedule = sNdsNativeNKirbyJointSchedule;
        tables->binding_joints = sNdsNativeNKirbyBindingJoints;
        tables->cross_slots = sNdsNativeNKirbyCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNKirbyRoots) /
            sizeof(sNdsNativeNKirbyRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNKirbyJointSchedule) /
            sizeof(sNdsNativeNKirbyJointSchedule[0]);
    }
#endif
#if NDS_P2_NPIKACHU
    else if (slot == 21u)
    {
        tables->roots = sNdsNativeNPikachuRoots;
        tables->schedule = sNdsNativeNPikachuJointSchedule;
        tables->binding_joints = sNdsNativeNPikachuBindingJoints;
        tables->cross_slots = sNdsNativeNPikachuCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNPikachuRoots) /
            sizeof(sNdsNativeNPikachuRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNPikachuJointSchedule) /
            sizeof(sNdsNativeNPikachuJointSchedule[0]);
    }
#endif
#if NDS_P2_NPURIN
    else if (slot == 22u)
    {
        tables->roots = sNdsNativeNPurinRoots;
        tables->schedule = sNdsNativeNPurinJointSchedule;
        tables->binding_joints = sNdsNativeNPurinBindingJoints;
        tables->cross_slots = sNdsNativeNPurinCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNPurinRoots) /
            sizeof(sNdsNativeNPurinRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNPurinJointSchedule) /
            sizeof(sNdsNativeNPurinJointSchedule[0]);
    }
#endif
#if NDS_P2_NNESS
    else if (slot == 23u)
    {
        tables->roots = sNdsNativeNNessRoots;
        tables->schedule = sNdsNativeNNessJointSchedule;
        tables->binding_joints = sNdsNativeNNessBindingJoints;
        tables->cross_slots = sNdsNativeNNessCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeNNessRoots) /
            sizeof(sNdsNativeNNessRoots[0]);
        tables->joint_count = sizeof(sNdsNativeNNessJointSchedule) /
            sizeof(sNdsNativeNNessJointSchedule[0]);
    }
#endif
    else
    {
        return FALSE;
    }
    return TRUE;
}

/* The baked nearest-BOUND-ancestor index for each matrix binding, so the owner
 * adapter can compose world matrices in one forward pass instead of walking each
 * binding to the root through a linear-probed hash. The generator guarantees
 * `binding_parents[i] < i` (it derives them from a preorder joint list), which
 * is what makes a single forward pass correct.
 *
 * Note the table skips UNBOUND joints: entry i is the nearest ancestor that is
 * itself a binding, not the DObj parent. A consumer therefore still walks the
 * live chain from binding i up to `bindings[binding_parents[i]]` -- that walk is
 * one to three joints instead of the full root depth, and needs no cache. */
const u8 *ndsRendererNativeFighterBindingParents(u32 slot, u32 *count)
{
    if ((slot >= NDS_NATIVE_FIGHTER_OWNER_COUNT) || (count == NULL))
    {
        return NULL;
    }
    if (slot == 0u)
    {
        *count = (u32)(sizeof(sNdsNativeMarioBindingParents) /
                       sizeof(sNdsNativeMarioBindingParents[0]));
        return sNdsNativeMarioBindingParents;
    }
    if (slot == 1u)
    {
        *count = (u32)(sizeof(sNdsNativeFoxBindingParents) /
                       sizeof(sNdsNativeFoxBindingParents[0]));
        return sNdsNativeFoxBindingParents;
    }
#if NDS_P2_LUIGI
    if (slot == 2u)
    {
        *count = (u32)(sizeof(sNdsNativeLuigiBindingParents) /
                       sizeof(sNdsNativeLuigiBindingParents[0]));
        return sNdsNativeLuigiBindingParents;
    }
#endif
#if NDS_P2_DONKEY
    if (slot == 3u)
    {
        *count = (u32)(sizeof(sNdsNativeDonkeyBindingParents) /
                       sizeof(sNdsNativeDonkeyBindingParents[0]));
        return sNdsNativeDonkeyBindingParents;
    }
#endif
#if NDS_P2_CAPTAIN
    if (slot == 4u)
    {
        *count = (u32)(sizeof(sNdsNativeCaptainBindingParents) /
                       sizeof(sNdsNativeCaptainBindingParents[0]));
        return sNdsNativeCaptainBindingParents;
    }
#endif
#if NDS_P2_SAMUS
    if (slot == 5u)
    {
        *count = (u32)(sizeof(sNdsNativeSamusBindingParents) /
                       sizeof(sNdsNativeSamusBindingParents[0]));
        return sNdsNativeSamusBindingParents;
    }
#endif
#if NDS_P2_LINK
    if (slot == 6u)
    {
        *count = (u32)(sizeof(sNdsNativeLinkBindingParents) /
                       sizeof(sNdsNativeLinkBindingParents[0]));
        return sNdsNativeLinkBindingParents;
    }
#endif
#if NDS_P2_PIKACHU
    if (slot == 7u)
    {
        *count = (u32)(sizeof(sNdsNativePikachuBindingParents) /
                       sizeof(sNdsNativePikachuBindingParents[0]));
        return sNdsNativePikachuBindingParents;
    }
#endif
#if NDS_P2_YOSHI
    if (slot == 8u)
    {
        *count = (u32)(sizeof(sNdsNativeYoshiBindingParents) /
                       sizeof(sNdsNativeYoshiBindingParents[0]));
        return sNdsNativeYoshiBindingParents;
    }
#endif
#if NDS_P2_NESS
    if (slot == 9u)
    {
        *count = (u32)(sizeof(sNdsNativeNessBindingParents) /
                       sizeof(sNdsNativeNessBindingParents[0]));
        return sNdsNativeNessBindingParents;
    }
#endif
#if NDS_P2_PURIN
    if (slot == 10u)
    {
        *count = (u32)(sizeof(sNdsNativePurinBindingParents) /
                       sizeof(sNdsNativePurinBindingParents[0]));
        return sNdsNativePurinBindingParents;
    }
#endif
#if NDS_P2_KIRBY
    if (slot == 11u)
    {
        *count = (u32)(sizeof(sNdsNativeKirbyBindingParents) /
                       sizeof(sNdsNativeKirbyBindingParents[0]));
        return sNdsNativeKirbyBindingParents;
    }
#endif
#if NDS_P2_MMARIO
    if (slot == 12u)
    {
        *count = (u32)(sizeof(sNdsNativeMMarioBindingParents) /
                       sizeof(sNdsNativeMMarioBindingParents[0]));
        return sNdsNativeMMarioBindingParents;
    }
#endif
#if NDS_P2_NMARIO
    if (slot == 13u)
    {
        *count = (u32)(sizeof(sNdsNativeNMarioBindingParents) /
                       sizeof(sNdsNativeNMarioBindingParents[0]));
        return sNdsNativeNMarioBindingParents;
    }
#endif
#if NDS_P2_NFOX
    if (slot == 14u)
    {
        *count = (u32)(sizeof(sNdsNativeNFoxBindingParents) /
                       sizeof(sNdsNativeNFoxBindingParents[0]));
        return sNdsNativeNFoxBindingParents;
    }
#endif
#if NDS_P2_NDONKEY
    if (slot == 15u)
    {
        *count = (u32)(sizeof(sNdsNativeNDonkeyBindingParents) /
                       sizeof(sNdsNativeNDonkeyBindingParents[0]));
        return sNdsNativeNDonkeyBindingParents;
    }
#endif
#if NDS_P2_NSAMUS
    if (slot == 16u)
    {
        *count = (u32)(sizeof(sNdsNativeNSamusBindingParents) /
                       sizeof(sNdsNativeNSamusBindingParents[0]));
        return sNdsNativeNSamusBindingParents;
    }
#endif
#if NDS_P2_NLINK
    if (slot == 17u)
    {
        *count = (u32)(sizeof(sNdsNativeNLinkBindingParents) /
                       sizeof(sNdsNativeNLinkBindingParents[0]));
        return sNdsNativeNLinkBindingParents;
    }
#endif
#if NDS_P2_NYOSHI
    if (slot == 18u)
    {
        *count = (u32)(sizeof(sNdsNativeNYoshiBindingParents) /
                       sizeof(sNdsNativeNYoshiBindingParents[0]));
        return sNdsNativeNYoshiBindingParents;
    }
#endif
#if NDS_P2_NCAPTAIN
    if (slot == 19u)
    {
        *count = (u32)(sizeof(sNdsNativeNCaptainBindingParents) /
                       sizeof(sNdsNativeNCaptainBindingParents[0]));
        return sNdsNativeNCaptainBindingParents;
    }
#endif
#if NDS_P2_NKIRBY
    if (slot == 20u)
    {
        *count = (u32)(sizeof(sNdsNativeNKirbyBindingParents) /
                       sizeof(sNdsNativeNKirbyBindingParents[0]));
        return sNdsNativeNKirbyBindingParents;
    }
#endif
#if NDS_P2_NPIKACHU
    if (slot == 21u)
    {
        *count = (u32)(sizeof(sNdsNativeNPikachuBindingParents) /
                       sizeof(sNdsNativeNPikachuBindingParents[0]));
        return sNdsNativeNPikachuBindingParents;
    }
#endif
#if NDS_P2_NPURIN
    if (slot == 22u)
    {
        *count = (u32)(sizeof(sNdsNativeNPurinBindingParents) /
                       sizeof(sNdsNativeNPurinBindingParents[0]));
        return sNdsNativeNPurinBindingParents;
    }
#endif
#if NDS_P2_NNESS
    if (slot == 23u)
    {
        *count = (u32)(sizeof(sNdsNativeNNessBindingParents) /
                       sizeof(sNdsNativeNNessBindingParents[0]));
        return sNdsNativeNNessBindingParents;
    }
#endif
    return NULL;
}

#if NDS_R2_FIGHTER_GX_COMPOSE
/* Slice 43. The same baked table the cross-run emitter reads through
 * `binding_palette_slots`, exported so the adapter can reuse a binding's
 * existing slot as its parent-store instead of allocating a second one. Most
 * entries are the 31 sentinel: the table exists for cross-matrix corners, not
 * for every binding. */
const u8 *ndsRendererNativeFighterCrossPaletteSlots(u32 slot, u32 *count)
{
    if ((slot >= NDS_NATIVE_FIGHTER_OWNER_COUNT) || (count == NULL))
    {
        return NULL;
    }
    if (slot == 0u)
    {
        *count = (u32)(sizeof(sNdsNativeMarioCrossPaletteSlots) /
                       sizeof(sNdsNativeMarioCrossPaletteSlots[0]));
        return sNdsNativeMarioCrossPaletteSlots;
    }
    if (slot == 1u)
    {
        *count = (u32)(sizeof(sNdsNativeFoxCrossPaletteSlots) /
                       sizeof(sNdsNativeFoxCrossPaletteSlots[0]));
        return sNdsNativeFoxCrossPaletteSlots;
    }
#if NDS_P2_LUIGI
    if (slot == 2u)
    {
        *count = (u32)(sizeof(sNdsNativeLuigiCrossPaletteSlots) /
                       sizeof(sNdsNativeLuigiCrossPaletteSlots[0]));
        return sNdsNativeLuigiCrossPaletteSlots;
    }
#endif
#if NDS_P2_DONKEY
    if (slot == 3u)
    {
        *count = (u32)(sizeof(sNdsNativeDonkeyCrossPaletteSlots) /
                       sizeof(sNdsNativeDonkeyCrossPaletteSlots[0]));
        return sNdsNativeDonkeyCrossPaletteSlots;
    }
#endif
#if NDS_P2_CAPTAIN
    if (slot == 4u)
    {
        *count = (u32)(sizeof(sNdsNativeCaptainCrossPaletteSlots) /
                       sizeof(sNdsNativeCaptainCrossPaletteSlots[0]));
        return sNdsNativeCaptainCrossPaletteSlots;
    }
#endif
#if NDS_P2_SAMUS
    if (slot == 5u)
    {
        *count = (u32)(sizeof(sNdsNativeSamusCrossPaletteSlots) /
                       sizeof(sNdsNativeSamusCrossPaletteSlots[0]));
        return sNdsNativeSamusCrossPaletteSlots;
    }
#endif
#if NDS_P2_LINK
    if (slot == 6u)
    {
        *count = (u32)(sizeof(sNdsNativeLinkCrossPaletteSlots) /
                       sizeof(sNdsNativeLinkCrossPaletteSlots[0]));
        return sNdsNativeLinkCrossPaletteSlots;
    }
#endif
#if NDS_P2_PIKACHU
    if (slot == 7u)
    {
        *count = (u32)(sizeof(sNdsNativePikachuCrossPaletteSlots) /
                       sizeof(sNdsNativePikachuCrossPaletteSlots[0]));
        return sNdsNativePikachuCrossPaletteSlots;
    }
#endif
#if NDS_P2_YOSHI
    if (slot == 8u)
    {
        *count = (u32)(sizeof(sNdsNativeYoshiCrossPaletteSlots) /
                       sizeof(sNdsNativeYoshiCrossPaletteSlots[0]));
        return sNdsNativeYoshiCrossPaletteSlots;
    }
#endif
#if NDS_P2_NESS
    if (slot == 9u)
    {
        *count = (u32)(sizeof(sNdsNativeNessCrossPaletteSlots) /
                       sizeof(sNdsNativeNessCrossPaletteSlots[0]));
        return sNdsNativeNessCrossPaletteSlots;
    }
#endif
#if NDS_P2_PURIN
    if (slot == 10u)
    {
        *count = (u32)(sizeof(sNdsNativePurinCrossPaletteSlots) /
                       sizeof(sNdsNativePurinCrossPaletteSlots[0]));
        return sNdsNativePurinCrossPaletteSlots;
    }
#endif
#if NDS_P2_KIRBY
    if (slot == 11u)
    {
        *count = (u32)(sizeof(sNdsNativeKirbyCrossPaletteSlots) /
                       sizeof(sNdsNativeKirbyCrossPaletteSlots[0]));
        return sNdsNativeKirbyCrossPaletteSlots;
    }
#endif
#if NDS_P2_MMARIO
    if (slot == 12u)
    {
        *count = (u32)(sizeof(sNdsNativeMMarioCrossPaletteSlots) /
                       sizeof(sNdsNativeMMarioCrossPaletteSlots[0]));
        return sNdsNativeMMarioCrossPaletteSlots;
    }
#endif
#if NDS_P2_NMARIO
    if (slot == 13u)
    {
        *count = (u32)(sizeof(sNdsNativeNMarioCrossPaletteSlots) /
                       sizeof(sNdsNativeNMarioCrossPaletteSlots[0]));
        return sNdsNativeNMarioCrossPaletteSlots;
    }
#endif
#if NDS_P2_NFOX
    if (slot == 14u)
    {
        *count = (u32)(sizeof(sNdsNativeNFoxCrossPaletteSlots) /
                       sizeof(sNdsNativeNFoxCrossPaletteSlots[0]));
        return sNdsNativeNFoxCrossPaletteSlots;
    }
#endif
#if NDS_P2_NDONKEY
    if (slot == 15u)
    {
        *count = (u32)(sizeof(sNdsNativeNDonkeyCrossPaletteSlots) /
                       sizeof(sNdsNativeNDonkeyCrossPaletteSlots[0]));
        return sNdsNativeNDonkeyCrossPaletteSlots;
    }
#endif
#if NDS_P2_NSAMUS
    if (slot == 16u)
    {
        *count = (u32)(sizeof(sNdsNativeNSamusCrossPaletteSlots) /
                       sizeof(sNdsNativeNSamusCrossPaletteSlots[0]));
        return sNdsNativeNSamusCrossPaletteSlots;
    }
#endif
#if NDS_P2_NLINK
    if (slot == 17u)
    {
        *count = (u32)(sizeof(sNdsNativeNLinkCrossPaletteSlots) /
                       sizeof(sNdsNativeNLinkCrossPaletteSlots[0]));
        return sNdsNativeNLinkCrossPaletteSlots;
    }
#endif
#if NDS_P2_NYOSHI
    if (slot == 18u)
    {
        *count = (u32)(sizeof(sNdsNativeNYoshiCrossPaletteSlots) /
                       sizeof(sNdsNativeNYoshiCrossPaletteSlots[0]));
        return sNdsNativeNYoshiCrossPaletteSlots;
    }
#endif
#if NDS_P2_NCAPTAIN
    if (slot == 19u)
    {
        *count = (u32)(sizeof(sNdsNativeNCaptainCrossPaletteSlots) /
                       sizeof(sNdsNativeNCaptainCrossPaletteSlots[0]));
        return sNdsNativeNCaptainCrossPaletteSlots;
    }
#endif
#if NDS_P2_NKIRBY
    if (slot == 20u)
    {
        *count = (u32)(sizeof(sNdsNativeNKirbyCrossPaletteSlots) /
                       sizeof(sNdsNativeNKirbyCrossPaletteSlots[0]));
        return sNdsNativeNKirbyCrossPaletteSlots;
    }
#endif
#if NDS_P2_NPIKACHU
    if (slot == 21u)
    {
        *count = (u32)(sizeof(sNdsNativeNPikachuCrossPaletteSlots) /
                       sizeof(sNdsNativeNPikachuCrossPaletteSlots[0]));
        return sNdsNativeNPikachuCrossPaletteSlots;
    }
#endif
#if NDS_P2_NPURIN
    if (slot == 22u)
    {
        *count = (u32)(sizeof(sNdsNativeNPurinCrossPaletteSlots) /
                       sizeof(sNdsNativeNPurinCrossPaletteSlots[0]));
        return sNdsNativeNPurinCrossPaletteSlots;
    }
#endif
#if NDS_P2_NNESS
    if (slot == 23u)
    {
        *count = (u32)(sizeof(sNdsNativeNNessCrossPaletteSlots) /
                       sizeof(sNdsNativeNNessCrossPaletteSlots[0]));
        return sNdsNativeNNessCrossPaletteSlots;
    }
#endif
    return NULL;
}
#endif

static void ndsRendererNativeMatrix3From20p12(
    const NDSRendererMatrix20p12 *source,
    NDSNativeMatrix3x3 *matrix)
{
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            matrix->m[row][col] = source->m[row][col];
        }
    }
}

static void ndsRendererNativeMatrix3Mul20p12(
    const NDSNativeMatrix3x3 *lhs,
    const NDSNativeMatrix3x3 *rhs,
    NDSNativeMatrix3x3 *out)
{
    NDSNativeMatrix3x3 result;
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            s64 sum = (s64)lhs->m[row][0] * rhs->m[0][col] +
                (s64)lhs->m[row][1] * rhs->m[1][col] +
                (s64)lhs->m[row][2] * rhs->m[2][col];

            result.m[row][col] = ndsRendererClampS64ToS32(
                ndsRendererRoundShiftS64(
                    sum, NDS_RENDERER_DS_MTX_FRAC_BITS));
        }
    }
    *out = result;
}

static void ndsRendererNativeMatrix3To20p12(
    const NDSNativeMatrix3x3 *source,
    NDSRendererMatrix20p12 *matrix)
{
    u32 row;
    u32 col;

    ndsRendererMtxIdentity20p12(matrix);
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            matrix->m[row][col] = source->m[row][col];
        }
    }
}

static s32 ndsRendererNativeHierarchyMatrixIsAffine(
    const NDSRendererMatrix20p12 *matrix)
{
    return ((matrix != NULL) &&
            (matrix->m[0][3] == 0) &&
            (matrix->m[1][3] == 0) &&
            (matrix->m[2][3] == 0) &&
            (matrix->m[3][3] ==
             (1 << NDS_RENDERER_DS_MTX_FRAC_BITS))) ? TRUE : FALSE;
}


static s32 ndsRendererNativePreflightFighterHierarchy(
    u32 slot,
    const u8 *asset_base,
    const NDSRendererNativeFighterHierarchy *hierarchy,
    NDSRendererCommandCallback callback,
    NDSRendererStats *stats,
    NDSNativeHierarchyTables *tables)
{
    NDSNativeFighterOwnerExecution *execution =
        &sNdsNativeFighterOwnerExecution;
    NDSRendererTraversalState *state = &execution->traversal;
    NDSRendererStats *scratch = &execution->preflight_stats;
    NDSNativeMatrix3x3 camera;
    NDSNativeMatrix3x3 identity;
    u32 binding_seen = 0u;
    u32 joint_index;
    u32 root_index;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    volatile NDSRendererOwnerProfile *m2_owner =
        ndsRendererProfileM2Owner();
#endif

    if ((asset_base == NULL) || (hierarchy == NULL) || (callback != NULL) ||
        (stats == NULL) || (tables == NULL) ||
        (hierarchy->projection == NULL) ||
        (hierarchy->camera_modelview == NULL) ||
        (hierarchy->joint_locals == NULL) ||
        (hierarchy->joint_parents == NULL) ||
        (hierarchy->joint_bindings == NULL) ||
        (hierarchy->roots == NULL) || (hierarchy->config == NULL) ||
        (sNdsNativeFighterOwnerExecution.active != 0u) ||
        (stats->blocker != NDS_RENDERER_BLOCKER_NONE) ||
        (ndsRendererNativeGetHierarchyTables(slot, tables) == FALSE) ||
        (hierarchy->joint_count != tables->joint_count) ||
        (hierarchy->root_count != tables->root_count) ||
        (tables->joint_count > NDS_NATIVE_FIGHTER_HIERARCHY_JOINT_MAX) ||
        (tables->root_count > NDS_NATIVE_FIGHTER_HIERARCHY_BINDING_MAX) ||
        (ndsRendererNativeHierarchyMatrixIsAffine(
             hierarchy->camera_modelview) == FALSE))
    {
        return FALSE;
    }
    ndsRendererNativeMatrix3From20p12(
        hierarchy->camera_modelview, &camera);
    memset(&identity, 0, sizeof(identity));
    identity.m[0][0] = 1 << NDS_RENDERER_DS_MTX_FRAC_BITS;
    identity.m[1][1] = 1 << NDS_RENDERER_DS_MTX_FRAC_BITS;
    identity.m[2][2] = 1 << NDS_RENDERER_DS_MTX_FRAC_BITS;
    for (joint_index = 0u; joint_index < tables->joint_count; joint_index++)
    {
        u32 packed = tables->schedule[joint_index];
        u32 parent = packed & 31u;
        u32 binding = (packed >> 5) & 31u;
        NDSNativeMatrix3x3 local;
        const NDSNativeMatrix3x3 *parent_world;

        if ((hierarchy->joint_parents[joint_index] != parent) ||
            (hierarchy->joint_bindings[joint_index] != binding) ||
            (ndsRendererNativeHierarchyMatrixIsAffine(
                 &hierarchy->joint_locals[joint_index]) == FALSE) ||
            ((parent != 31u) && (parent >= joint_index)) ||
            ((binding != 31u) && (binding >= tables->root_count)))
        {
            return FALSE;
        }
        ndsRendererNativeMatrix3From20p12(
            &hierarchy->joint_locals[joint_index], &local);
        /* Match BattleShip's fixed-point association exactly: build each
         * source world from identity, then apply the camera once at a bound
         * root.  Seeding camera into the recurrence is algebraically equal in
         * real arithmetic but rounds differently in 20.12. */
        parent_world = (parent == 31u) ? &identity :
            &execution->hierarchy_world[parent];
        ndsRendererNativeMatrix3Mul20p12(
            &local, parent_world,
            &execution->hierarchy_world[joint_index]);
        if (binding != 31u)
        {
            if (((binding_seen >> binding) & 1u) != 0u)
            {
                return FALSE;
            }
            binding_seen |= 1u << binding;
        }
    }
    if (binding_seen != ((1u << tables->root_count) - 1u))
    {
        return FALSE;
    }
    for (root_index = 0u; root_index < tables->root_count; root_index++)
    {
        u32 binding_joint = tables->binding_joints[root_index];

        if ((binding_joint >= tables->joint_count) ||
            (hierarchy->joint_bindings[binding_joint] != root_index))
        {
            return FALSE;
        }
        /* All source-world descendants are complete, so the binding joints
         * can now hold their exact once-composed lighting matrices in place. */
        ndsRendererNativeMatrix3Mul20p12(
            &execution->hierarchy_world[binding_joint], &camera,
            &execution->hierarchy_world[binding_joint]);
    }

    *scratch = *stats;
    ndsRendererInitTraversalState(
        state, hierarchy->config, scratch, NULL, NULL, 0u);
    for (root_index = 0u; root_index < tables->root_count; root_index++)
    {
        const NDSNativeRoot *root = &tables->roots[root_index];
        const NDSRendererNativeFighterRoot *input =
            &hierarchy->roots[root_index];
        /* Hoisted so the consumed-fields manifest can still see the flags
         * read: its arrow scanner attributes `input->preamble->flags` to
         * `input.preamble` and then loses the second hop entirely, so a
         * chained read would silently drop a classified field. */
        const NDSRendererNativeFighterPreamble *preamble = input->preamble;
        NDSRendererMatrix20p12 light_modelview;
        u32 epoch_offset;

        if ((input->root_offset != root->root_offset) ||
            (input->composed_matrix != NULL) ||
            (input->modelview_matrix != NULL) ||
            (input->config != hierarchy->config) ||
            (preamble == NULL) ||
            ((preamble->flags &
              NDS_RENDERER_NATIVE_PREAMBLE_VALID) == 0u))
        {
            return FALSE;
        }
        ndsRendererNativeMatrix3To20p12(
            &execution->hierarchy_world[
                tables->binding_joints[root_index]],
            &light_modelview);
        ndsRendererNativeApplyProductionPreamble(preamble, scratch);
        ndsRendererNativeApplyRootLightPreamble(root, scratch);
        state->vertex_valid_mask = 0u;
        state->input_vertex_valid_mask = 0u;
        state->vertex_color_valid_mask = 0u;
        state->current_transform_vertex_mask = 0u;
        state->prepared_vertex_color_valid_mask = 0u;
        state->prepared_texcoord_valid_mask = 0u;
        state->prepared_light_direction_valid = 0u;
        state->texture_prepare_valid = 0u;
        state->modelview = light_modelview;
        state->modelview_valid = TRUE;
        state->matrix = light_modelview;
        state->matrix_valid = TRUE;
        state->matrix_generation = root_index + 1u;

        for (epoch_offset = 0u;
             epoch_offset < root->epoch_count;
             epoch_offset++)
        {
            u32 epoch_index = root->first_epoch + epoch_offset;
            const NDSNativeEpoch *epoch =
                &sNdsNativeFighterEpochs[epoch_index];
            NDSNativeHierarchyPreparedEpoch *prepared_epoch =
                &execution->hierarchy_epochs[epoch_index];
            u32 run_offset;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            u32 m2_lighting_start;
#endif

            if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
                ((input->materials == NULL) ||
                 (epoch->material_slot >= input->material_count)))
            {
                return FALSE;
            }
            ndsRendererNativeApplyStateSpanPreflight(
                epoch->before_state_first, epoch->before_state_count,
                epoch->before_sync_count, asset_base, scratch, state);
            if (epoch->material_slot != NDS_NATIVE_MATERIAL_NONE)
            {
                ndsRendererNativeApplyMaterialPreflight(
                    &input->materials[epoch->material_slot], scratch, state);
            }
            ndsRendererNativeApplyStateSpanPreflight(
                epoch->after_state_first, epoch->after_state_count,
                epoch->after_sync_count, asset_base, scratch, state);
            if (scratch->blocker != NDS_RENDERER_BLOCKER_NONE)
            {
                return FALSE;
            }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            m2_lighting_start = cpuGetTiming();
#endif
            prepared_epoch->light_direction_valid = FALSE;
            if ((epoch->action_count != 0u) &&
                ((scratch->geometry_mode &
                  NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
                ((scratch->light_dir_mask &
                  NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
            {
                if (state->prepared_light_direction_valid == 0u)
                {
                    ndsRendererHardwarePrepareLitDirection(
                        scratch, &state->modelview,
                        &state->prepared_light_direction);
                    state->prepared_light_direction_valid = TRUE;
                }
                prepared_epoch->light_direction =
                    state->prepared_light_direction;
                prepared_epoch->light_direction_valid = TRUE;
                if ((scratch->light_color_mask &
                     (NDS_RENDERER_LIGHT_COLOR_1_MASK |
                      NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
                    (NDS_RENDERER_LIGHT_COLOR_1_MASK |
                     NDS_RENDERER_LIGHT_COLOR_2_MASK))
                {
                    /* Populate the bounded CPU shade cache before GX.  Commit
                     * may look the pair up again after cache rotation, but the
                     * lookup/build has no failure branch. */
                    (void)ndsRendererHardwareGetLightShadeLut(
                        scratch->light_color_1,
                        scratch->light_color_2);
                }
            }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            if (m2_owner != NULL)
            {
                m2_owner->m2_lighting_shading_ticks +=
                    cpuGetTiming() - m2_lighting_start;
            }
#endif
            memset(&execution->hierarchy_runs[epoch_index], 0,
                   sizeof(execution->hierarchy_runs[epoch_index]));
            for (run_offset = 0u;
                 run_offset < epoch->run_count;
                 run_offset++)
            {
                u32 run_index = epoch->first_run + run_offset;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                u32 m2_run_start = cpuGetTiming();
                s32 run_ready;
#endif

#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                run_ready = ndsRendererNativePrepareHierarchyRun(
                    run_index,
                    sNdsNativeFighterEpochDirectPolicy[epoch_index],
                    TRUE, hierarchy->config, scratch, state,
                    &execution->hierarchy_runs[epoch_index]);
                if (m2_owner != NULL)
                {
                    m2_owner->m2_run_prepare_ticks +=
                        cpuGetTiming() - m2_run_start;
                }
                if (run_ready == FALSE)
#else
                if (ndsRendererNativePrepareHierarchyRun(
                        run_index,
                        sNdsNativeFighterEpochDirectPolicy[epoch_index],
                        TRUE, hierarchy->config, scratch, state,
                        &execution->hierarchy_runs[epoch_index]) == FALSE)
#endif
                {
                    return FALSE;
                }
            }
        }
        ndsRendererNativeApplyStateSpanPreflight(
            root->tail_state_first, root->tail_state_count,
            root->tail_sync_count, asset_base, scratch, state);
        if (scratch->blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void ndsRendererNativeBuildHierarchyHardwareAffine(
    const NDSRendererMatrix20p12 *source,
    m4x4 *hardware)
{
    NDSRendererMatrix20p12 scaled = *source;
    u32 col;

    for (col = 0u; col < 3u; col++)
    {
        scaled.m[3][col] = ndsRendererRoundShiftS32Signed(
            scaled.m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    ndsRendererCopyMtx20p12ToM4x4(&scaled, hardware);
}

static void ndsRendererNativePrepareHierarchyTexcoords(
    u32 run_index,
    const NDSNativeHierarchyPreparedRun *prepared_run)
{
    u32 unique_first;
    u32 unique_count;
    u32 unique_offset;

    if ((prepared_run == NULL) || (prepared_run->textured == 0u))
    {
        return;
    }
    unique_first = sNdsNativeFighterRunFirstUnique[run_index];
    unique_count = sNdsNativeFighterRunUniqueCount[run_index];
    for (unique_offset = 0u; unique_offset < unique_count; unique_offset++)
    {
        u32 dense_id = sNdsNativeFighterRunUniqueDense[
            unique_first + unique_offset];
        const NDSNativeDenseVertex *dense =
            &sNdsNativeFighterDenseVertices[dense_id];
        NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterPreparedDense[dense_id];
        s32 scaled_s = ((s32)dense->s *
            (s32)prepared_run->scale_s) >> 17;
        s32 scaled_t = ((s32)dense->t *
            (s32)prepared_run->scale_t) >> 17;

        prepared->s = (s16)(scaled_s -
            ((s32)prepared_run->origin_s << 2) +
            prepared_run->texture_offset);
        prepared->t = (s16)(scaled_t -
            ((s32)prepared_run->origin_t << 2) +
            prepared_run->texture_offset);
    }
}

static inline void ndsRendererNativeBeginHierarchyBatch(
    NDSRendererStats *stats,
    const NDSNativeHierarchyPreparedRun *prepared_run,
    u32 matrix_generation)
{
    u32 texture_name = (prepared_run->textured != 0u) ?
        prepared_run->texture_name : 0u;
#if NDS_LAB_NO_CULL
    /* The hierarchy batch is mode 10's; the seam probe patches it too so an arm
     * means the same thing whichever owner path a capture is on. See
     * ndsRendererNativeLabSeamPolyFmt for the arm table and the two failures
     * this shape exists to prevent. */
    u32 poly_fmt = ndsRendererNativeLabSeamPolyFmt(prepared_run->poly_fmt);
#else
    u32 poly_fmt = prepared_run->poly_fmt;
#endif

    if ((sNdsRendererHardwareTriangleBatchOpen != 0u) &&
        (sNdsRendererHardwareTriangleBatchTextured ==
         prepared_run->textured) &&
        (sNdsRendererHardwareTriangleBatchTextureName == texture_name) &&
        (sNdsRendererHardwareTriangleBatchPolyFmt == poly_fmt) &&
        (sNdsRendererHardwareTriangleBatchMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY) &&
        (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
         matrix_generation))
    {
        ndsRendererProfileRecordBatchReuse();
        return;
    }

    ndsRendererHardwareEndBatch();
    glEnable(GL_TEXTURE_2D);
    if (prepared_run->textured != 0u)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            prepared_run->texture_entry;

        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
        entry->params = prepared_run->texture_params;
        ndsRendererHardwareBindTextureName(stats, prepared_run->texture_name);
        ndsRendererHardwareApplyTextureParams(prepared_run->texture_params);
        sNdsRendererHardwareActiveTextureEntry = entry;
        if (entry->pinned != 0u)
        {
            ndsRendererHardwareRecordBattleStaticTextureHit(entry);
        }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = prepared_run->texture_format;
        stats->hardware_texture_width = prepared_run->texture_width;
        stats->hardware_texture_height = prepared_run->texture_height;
    }
    else
    {
        ndsRendererHardwareBindNoTexture(NULL);
    }
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    ndsRendererHardwareSetPolyFmt(poly_fmt);
    glBegin(GL_TRIANGLE);
    ndsRendererProfileRecordBatchBegin();

    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = prepared_run->textured;
    sNdsRendererHardwareTriangleBatchTextureName = texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = 0u;
    sNdsRendererHardwareTriangleBatchFogKey = 0u;
    sNdsRendererHardwareTriangleBatchMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY;
    sNdsRendererHardwareTriangleBatchMatrixGeneration = matrix_generation;
}

static void ndsRendererNativeCommitHierarchyRoot(
    const u8 *asset_base,
    const NDSNativeRoot *root,
    const NDSRendererNativeFighterRoot *input,
    u32 binding_joint,
    u32 current_palette_slot,
    u32 matrix_generation,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 *run_count,
    u32 *triangle_count)
{
    NDSNativeFighterOwnerExecution *execution =
        &sNdsNativeFighterOwnerExecution;
    NDSRendererMatrix20p12 light_modelview;
    u32 epoch_offset;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    volatile NDSRendererOwnerProfile *m2_owner =
        ndsRendererProfileM2Owner();
#endif

    ndsRendererNativeMatrix3To20p12(
        &execution->hierarchy_world[binding_joint], &light_modelview);
    ndsRendererNativeApplyProductionPreamble(input->preamble, stats);
    state->vertex_valid_mask = 0u;
    state->input_vertex_valid_mask = 0u;
    state->vertex_color_valid_mask = 0u;
    state->current_transform_vertex_mask = 0u;
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_light_direction_valid = 0u;
    state->texture_prepare_valid = 0u;
    state->modelview = light_modelview;
    state->modelview_valid = TRUE;
    state->matrix = light_modelview;
    state->matrix_valid = TRUE;
    state->matrix_generation = matrix_generation;
    if (stats->first_opcode == 0u)
    {
        stats->first_opcode = NDS_RENDERER_OP_RDPPIPESYNC;
    }
    stats->command_count += root->source_command_count;
    ndsRendererNativeApplyRootLightPreamble(root, stats);
    for (epoch_offset = 0u;
         epoch_offset < root->epoch_count;
         epoch_offset++)
    {
        u32 epoch_index = root->first_epoch + epoch_offset;
        const NDSNativeEpoch *epoch =
            &sNdsNativeFighterEpochs[epoch_index];
        const NDSNativeHierarchyPreparedEpoch *prepared_epoch =
            &execution->hierarchy_epochs[epoch_index];
        u32 run_offset;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        u32 m2_lighting_start;
#endif

        ndsRendererNativeApplyStateSpan(
            epoch->before_state_first, epoch->before_state_count,
            epoch->before_sync_count, asset_base, stats, state);
        if (epoch->material_slot != NDS_NATIVE_MATERIAL_NONE)
        {
            ndsRendererNativeApplyMaterial(
                &input->materials[epoch->material_slot], stats, state);
        }
        ndsRendererNativeApplyStateSpan(
            epoch->after_state_first, epoch->after_state_count,
            epoch->after_sync_count, asset_base, stats, state);
        state->prepared_light_direction =
            prepared_epoch->light_direction;
        state->prepared_light_direction_valid =
            prepared_epoch->light_direction_valid;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        m2_lighting_start = cpuGetTiming();
#endif
        (void)ndsRendererNativeShadeProductionActions(
            epoch, sNdsNativeFighterEpochDirectPolicy[epoch_index],
            FALSE, stats, state);
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        if (m2_owner != NULL)
        {
            m2_owner->m2_lighting_shading_ticks +=
                cpuGetTiming() - m2_lighting_start;
        }
#endif

        for (run_offset = 0u;
             run_offset < epoch->run_count;
             run_offset++)
        {
            u32 run_index = epoch->first_run + run_offset;
            const NDSNativeRun *run =
                &sNdsNativeFighterRuns[run_index];
            const NDSNativeHierarchyPreparedRun *prepared_run =
                &execution->hierarchy_runs[epoch_index];
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            u32 m2_phase_start = cpuGetTiming();
#endif

            ndsRendererNativePrepareHierarchyTexcoords(
                run_index, prepared_run);
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            if (m2_owner != NULL)
            {
                m2_owner->m2_run_prepare_ticks +=
                    cpuGetTiming() - m2_phase_start;
            }
            m2_phase_start = cpuGetTiming();
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
            ndsRendererNativeBeginHierarchyBatch(
                stats, prepared_run, matrix_generation);
            if (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
            {
                ndsRendererNativeEmitProductionCrossRun(
                    run_index, (u32)run->triangle_count * 3u,
                    prepared_run->textured, current_palette_slot, NULL);
            }
            else
            {
                if (prepared_run->textured != 0u)
                {
                    ndsRendererNativeEmitProductionRawTexturedRun(
                        run_index, (u32)run->triangle_count * 3u);
                }
                else
                {
                    ndsRendererNativeEmitProductionRawUntexturedRun(
                        run_index, (u32)run->triangle_count * 3u);
                }
            }
#endif
            stats->triangle_count += run->triangle_count;
            if (run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT)
            {
                ndsRendererFastAccountRawTriangles(
                    stats, run->triangle_count,
                    run->triangle_count - 1u);
            }
            else
            {
                ndsRendererNativeAccountGXCrossTriangles(
                    stats, run->triangle_count,
                    run->triangle_count - 1u);
            }
            (*run_count)++;
            *triangle_count += run->triangle_count;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            if (m2_owner != NULL)
            {
                m2_owner->m2_corner_emit_account_ticks +=
                    cpuGetTiming() - m2_phase_start;
            }
#endif
        }
    }
    ndsRendererNativeApplyStateSpan(
        root->tail_state_first, root->tail_state_count,
        root->tail_sync_count, asset_base, stats, state);
    stats->end_command_count++;
}
#endif
