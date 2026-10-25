/* Native renderer, hierarchy submission: production run submit, owner root state,
 * deferred palettes, binding parents, cross palettes and the hierarchy matrices.
 * A slice of nds_renderer.c. */

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
    u32 submit_class;

    if ((run == NULL) || (run->triangle_count == 0u))
    {
        return ndsRendererNativeDirectReject(stats);
    }
    submit_class = ndsRendererNativeRunSubmitClass(run);
    run_index = (u32)(run - sNdsNativeFighterActiveTables->runs);
    if (ndsRendererNativePrepareProductionRun(
            run_index, epoch_policy,
            FALSE,
            config, stats, state) == FALSE)
    {
        return FALSE;
    }
    if ((submit_class == NDS_NATIVE_RUN_CROSS_MATRIX) &&
        (current_palette_slot > NDS_NATIVE_GX_MATRIX_SLOT_MAX))
    {
        return ndsRendererNativeDirectReject(stats);
    }
    if (submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
    {
        ndsRendererNativeEmitProductionCrossRun(
            run_index, (u32)run->triangle_count * 3u,
            state->texture_prepare_enabled,
            current_palette_slot, binding_palette_slots);
    }
    else
    {
        /* RAW runs emit DS-native primitive groups (strips + residual
         * triangles) compiled host-side. The first group's begin-batch already
         * ran in PrepareProductionRun; this walk re-begins only on type change
         * and leaves GL_TRIANGLE selected for whoever reuses the batch. Only
         * RAW runs come here -- the cross-matrix branch above owns its own
         * emitter -- which is why the group vertex refs are bare dense ids,
         * exactly as the raw emitters read the packed corners unmasked. */
        if (NDS_STRIP_ROUTE_ON())
        {
            ndsRendererNativeEmitProductionPrimitiveGroups(
                run_index, state->texture_prepare_enabled);
        }
        else
        if (state->texture_prepare_enabled != 0u)
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
    stats->triangle_count += run->triangle_count;
    if (ndsRendererNativeRunSubmitClass(run) == NDS_NATIVE_RUN_RAW_CURRENT)
    {
        *raw_triangle_count += run->triangle_count;
        *raw_reuse_count += run->triangle_count - 1u;
    }
    else if (ndsRendererNativeRunSubmitClass(run) == NDS_NATIVE_RUN_CROSS_MATRIX)
    {
        *cross_triangle_count += run->triangle_count;
        *cross_reuse_count += run->triangle_count - 1u;
    }
    else
    {
        return ndsRendererNativeDirectReject(stats);
    }
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
                &sNdsNativeFighterHighTables.dense_vertices[dense_id];
            const NDSNativePreparedDenseVertex *prepared =
                &sNdsNativeFighterHighTables.prepared_dense[dense_id];
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
                &sNdsNativeFighterHighTables.dense_vertices[dense_id];
            const NDSNativePreparedDenseVertex *prepared =
                &sNdsNativeFighterHighTables.prepared_dense[dense_id];
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
    sNdsRendererHardwareSubmitClassCounts[
        NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX] += triangle_count;
#if NDS_DIAG_COUNTERS
    {
        u32 reuse_count = (triangle_count != 0u) ?
            triangle_count - 1u : 0u;

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
    }
#endif
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    stats->hardware_projected_depth_triangle_count += triangle_count;
    sNdsRendererHardwareSubmitted = TRUE;
}

static s32 ndsRendererNativeSubmitRunDirect(
    const NDSNativeRun *run,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 emitted_triangles = 0u;
    u32 run_index;
    u32 i;

    if ((run == NULL) || (config == NULL) || (stats == NULL) ||
        (state == NULL) || (run->triangle_count == 0u))
    {
        return FALSE;
    }
    run_index = (u32)(run - sNdsNativeFighterHighTables.runs);
    if (ndsRendererNativeRunSubmitClass(run) == NDS_NATIVE_RUN_RAW_CURRENT)
    {
        const u16 *triangles =
            &sNdsNativeFighterHighTables.triangles[run->first_triangle];
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
    if (ndsRendererNativeRunSubmitClass(run) == NDS_NATIVE_RUN_CROSS_MATRIX)
    {
        const u16 *triangles =
            &sNdsNativeFighterHighTables.triangles[run->first_triangle];
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
    return FALSE;
}

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
    s32 native_snapshot_ready = FALSE;
    u32 native_raw_submitted = 0u;
    u32 native_cross_submitted = 0u;
    u32 i;

    stats->triangle_count += run->triangle_count;
    triangle = &sNdsNativeFighterHighTables.triangles[run->first_triangle];
    if ((ndsRendererNativeRunSubmitClass(run) == NDS_NATIVE_RUN_RAW_CURRENT) &&
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
    else if ((ndsRendererNativeRunSubmitClass(run) == NDS_NATIVE_RUN_CROSS_MATRIX) &&
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
        if (((ndsRendererNativeRunSubmitClass(run) == NDS_NATIVE_RUN_RAW_CURRENT) &&
             (native_raw_ready == FALSE)) ||
            ((ndsRendererNativeRunSubmitClass(run) == NDS_NATIVE_RUN_CROSS_MATRIX) &&
             (native_snapshot_ready == FALSE)) ||
            (NDS_RENDERER_BENCHMARK_MODE ==
             NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP))
        {
            if ((ndsRendererNativeRunSubmitClass(run) == NDS_NATIVE_RUN_RAW_CURRENT) &&
                (i != 0u) &&
                (ndsRendererFastRawStateEligible(state) == FALSE))
            {
                sNdsRendererFastFallbackCount[0]++;
            }
            ndsRendererNativeSubmitGenericTriangle(
                packed, command_index, command_half,
                config, stats, state);
            if ((ndsRendererNativeRunSubmitClass(run) == NDS_NATIVE_RUN_RAW_CURRENT) &&
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
            if (ndsRendererNativeRunSubmitClass(run) == NDS_NATIVE_RUN_CROSS_MATRIX)
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
            {
            if (sNdsRendererHardwareNoOracle == 0u)
            {
                ndsRendererRecordTransformedTriangle(
                    stats, state, packed);
            }
            ndsRendererFastEmitRawCommand(
                state, indices, 1u, state->texture_prepare_enabled);
            native_raw_submitted++;
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
}

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
    /* The fighter's colour animation (hurt flash, the challenger's black):
     * the shared owner state is initialised once per owner, so each root
     * takes this draw's modulate, as ndsRendererInitTraversalState does. */
    state->color_modulate = config->color_modulate;
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
    s32 shared_owner_state = FALSE;
#define state (*state_ptr)

    if ((asset_base == NULL) || (config == NULL) || (stats == NULL) ||
        (vertex_cache == NULL) || (slot > 1u))
    {
        return FALSE;
    }
    /* This legacy root entry point predates P2-2 and has no detail argument.
     * Keep its historical contract explicitly high-detail so a preceding low
     * owner cannot leave the shared serialized table view pointed elsewhere. */
    if (ndsRendererNativeSelectFighterRuntimeTables(slot, FALSE) == FALSE)
    {
        return FALSE;
    }
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
            &sNdsNativeFighterHighTables.epochs[root->first_epoch + epoch_index];

        if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
            ((materials == NULL) ||
             (epoch->material_slot >= material_count)))
        {
            return FALSE;
        }
    }

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
            &sNdsNativeFighterHighTables.epochs[root->first_epoch + epoch_index];
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
                &sNdsNativeFighterHighTables.runs[epoch->first_run + run_index];

            if ((callback != NULL) ||
                (sNdsRendererHardwareNoOracle == 0u) ||
                (ndsRendererNativeSubmitRunDirect(
                     run, config, stats, &state) == FALSE))
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
    if (shared_owner_state == FALSE)
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
    if ((shared_owner_state != FALSE) &&
        (stats->blocker != NDS_RENDERER_BLOCKER_NONE))
    {
        ndsRendererAbortNativeFighterOwner();
        return FALSE;
    }
    return TRUE;
#undef state
}
/* The swap VBlank's deferred palette rewrites, called from the platform's
 * scheduled wait (nds_platform.c): the KO pillar variants and the entry
 * ramps. */
void ndsRendererCommitDeferredPalettes(void)
{
    ndsRendererParticleEnvVariantCommit();
    ndsRendererEntryRampPaletteCommit();
}


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
#if NDS_LUIGI
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
#if NDS_DONKEY
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
#if NDS_CAPTAIN
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
#if NDS_LINK
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
#if NDS_MMARIO
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
#if NDS_NMARIO
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
#if NDS_NFOX
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
#define NDS_P4_OWNER_HIERARCHY(id_, T_, N_, n_, parent_, model_, main_)        \
    else if (slot == NDS_P4_RENDERER_OWNER_SLOT(id_))                           \
    {                                                                           \
        tables->roots = sNdsNative##T_##Roots;                                  \
        tables->schedule = sNdsNative##T_##JointSchedule;                       \
        tables->binding_joints = sNdsNative##T_##BindingJoints;                 \
        tables->cross_slots = sNdsNative##T_##CrossPaletteSlots;                \
        tables->root_count = sizeof(sNdsNative##T_##Roots) /                    \
            sizeof(sNdsNative##T_##Roots[0]);                                   \
        tables->joint_count = sizeof(sNdsNative##T_##JointSchedule) /           \
            sizeof(sNdsNative##T_##JointSchedule[0]);                           \
    }
    NDS_P4_CONTENT_ROWS(NDS_P4_OWNER_HIERARCHY)
#undef NDS_P4_OWNER_HIERARCHY
#if NDS_NDONKEY
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
#if NDS_NSAMUS
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
#if NDS_NLINK
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
#if NDS_NYOSHI
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
#if NDS_NCAPTAIN
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
#if NDS_NKIRBY
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
#if NDS_NPIKACHU
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
#if NDS_NPURIN
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
#if NDS_NNESS
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
#if NDS_1P_GAME
    else if (slot == 24u)
    {
        tables->roots = sNdsNativeBossRoots;
        tables->schedule = sNdsNativeBossJointSchedule;
        tables->binding_joints = sNdsNativeBossBindingJoints;
        tables->cross_slots = sNdsNativeBossCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeBossRoots) /
            sizeof(sNdsNativeBossRoots[0]);
        tables->joint_count = sizeof(sNdsNativeBossJointSchedule) /
            sizeof(sNdsNativeBossJointSchedule[0]);
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
    if (ndsRendererNativeFighterRootProgram(slot) == NDS_NATIVE_SKELETON_PROGRAM)
    {
        *count = ndsNativeSkeletonOwner(slot, 1u)->root_count;
        return sNdsSkeletonBindingParents;
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
#if NDS_LUIGI
    if (slot == 2u)
    {
        *count = (u32)(sizeof(sNdsNativeLuigiBindingParents) /
                       sizeof(sNdsNativeLuigiBindingParents[0]));
        return sNdsNativeLuigiBindingParents;
    }
#endif
#if NDS_DONKEY
    if (slot == 3u)
    {
        *count = (u32)(sizeof(sNdsNativeDonkeyBindingParents) /
                       sizeof(sNdsNativeDonkeyBindingParents[0]));
        return sNdsNativeDonkeyBindingParents;
    }
#endif
#if NDS_CAPTAIN
    if (slot == 4u)
    {
        *count = (u32)(sizeof(sNdsNativeCaptainBindingParents) /
                       sizeof(sNdsNativeCaptainBindingParents[0]));
        return sNdsNativeCaptainBindingParents;
    }
#endif
    if (slot == 5u)
    {
#if defined(NDS_NATIVE_SAMUS_ROOT_PROGRAMS_PRESENT)
        if (ndsRendererNativeFighterRootProgram(slot) == 1u)
        {
            *count = (u32)(sizeof(sNdsNativeSamusCatchBindingParents) /
                           sizeof(sNdsNativeSamusCatchBindingParents[0]));
            return sNdsNativeSamusCatchBindingParents;
        }
        if (ndsRendererNativeFighterRootProgram(slot) == 2u)
        {
            *count = (u32)(sizeof(sNdsNativeSamusMorphUnfoldBindingParents) /
                           sizeof(sNdsNativeSamusMorphUnfoldBindingParents[0]));
            return sNdsNativeSamusMorphUnfoldBindingParents;
        }
        if (ndsRendererNativeFighterRootProgram(slot) == 3u)
        {
            *count = (u32)(sizeof(sNdsNativeSamusMorphBallBindingParents) /
                           sizeof(sNdsNativeSamusMorphBallBindingParents[0]));
            return sNdsNativeSamusMorphBallBindingParents;
        }
        if (ndsRendererNativeFighterRootProgram(slot) == 4u)
        {
            *count = (u32)(sizeof(sNdsNativeSamusFSmashBindingParents) /
                           sizeof(sNdsNativeSamusFSmashBindingParents[0]));
            return sNdsNativeSamusFSmashBindingParents;
        }
#endif
        *count = (u32)(sizeof(sNdsNativeSamusBindingParents) /
                       sizeof(sNdsNativeSamusBindingParents[0]));
        return sNdsNativeSamusBindingParents;
    }
#if NDS_LINK
    if (slot == 6u)
    {
#if defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT)
        u32 program = ndsRendererNativeFighterRootProgram(slot);
        if (program == 1u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkEntryBindingParents) /
                           sizeof(sNdsNativeLinkEntryBindingParents[0]));
            return sNdsNativeLinkEntryBindingParents;
        }
        if (program == 2u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkCatchBindingParents) /
                           sizeof(sNdsNativeLinkCatchBindingParents[0]));
            return sNdsNativeLinkCatchBindingParents;
        }
        if (program == 3u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkSpecialNBindingParents) /
                           sizeof(sNdsNativeLinkSpecialNBindingParents[0]));
            return sNdsNativeLinkSpecialNBindingParents;
        }
        if (program == 4u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkClapsBindingParents) /
                           sizeof(sNdsNativeLinkClapsBindingParents[0]));
            return sNdsNativeLinkClapsBindingParents;
        }
        if (program == 5u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkSpecialNItemBindingParents) /
                           sizeof(sNdsNativeLinkSpecialNItemBindingParents[0]));
            return sNdsNativeLinkSpecialNItemBindingParents;
        }
#if defined(NDS_NATIVE_LINK_INTRO_PROGRAM_PRESENT)
        if (program == 6u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkIntroBindingParents) /
                           sizeof(sNdsNativeLinkIntroBindingParents[0]));
            return sNdsNativeLinkIntroBindingParents;
        }
#endif
#endif
        *count = (u32)(sizeof(sNdsNativeLinkBindingParents) /
                       sizeof(sNdsNativeLinkBindingParents[0]));
        return sNdsNativeLinkBindingParents;
    }
#endif
    if (slot == 7u)
    {
#if defined(NDS_NATIVE_PIKACHU_ROOT_PROGRAMS_PRESENT)
        if (ndsRendererNativeFighterRootProgram(slot) == 1u)
        {
            *count = (u32)(sizeof(sNdsNativePikachuAccessoryBindingParents) /
                           sizeof(sNdsNativePikachuAccessoryBindingParents[0]));
            return sNdsNativePikachuAccessoryBindingParents;
        }
#endif
        *count = (u32)(sizeof(sNdsNativePikachuBindingParents) /
                       sizeof(sNdsNativePikachuBindingParents[0]));
        return sNdsNativePikachuBindingParents;
    }
    if (slot == 8u)
    {
#if defined(NDS_NATIVE_YOSHI_ROOT_PROGRAMS_PRESENT)
        u32 program = ndsRendererNativeFighterRootProgram(slot);
        if (program == 1u)
        {
            *count = (u32)(sizeof(sNdsNativeYoshiCatchBindingParents) /
                           sizeof(sNdsNativeYoshiCatchBindingParents[0]));
            return sNdsNativeYoshiCatchBindingParents;
        }
        if (program == 2u)
        {
            *count = (u32)(sizeof(sNdsNativeYoshiThrowBindingParents) /
                           sizeof(sNdsNativeYoshiThrowBindingParents[0]));
            return sNdsNativeYoshiThrowBindingParents;
        }
#endif
        *count = (u32)(sizeof(sNdsNativeYoshiBindingParents) /
                       sizeof(sNdsNativeYoshiBindingParents[0]));
        return sNdsNativeYoshiBindingParents;
    }
    if (slot == 9u)
    {
#if defined(NDS_NATIVE_NESS_ROOT_PROGRAMS_PRESENT)
        /* Unlike Link's and Yoshi's dynamically inserted hidden parts, Ness's
         * joint 17 is an ordinary JointTree node that merely gains a display
         * list, so the generator publishes a real source parent schedule for
         * the Win3 vector rather than an all-255 live capture. */
        if (ndsRendererNativeFighterRootProgram(slot) == 1u)
        {
            *count = (u32)(sizeof(sNdsNativeNessWin3BindingParents) /
                           sizeof(sNdsNativeNessWin3BindingParents[0]));
            return sNdsNativeNessWin3BindingParents;
        }
        /* Slice 7: the yo-yo's joint 30 IS dynamically inserted (hidden
         * part 3), so this vector is the all-255 live capture. */
        if (ndsRendererNativeFighterRootProgram(slot) == 2u)
        {
            *count = (u32)(sizeof(sNdsNativeNessYoYoBindingParents) /
                           sizeof(sNdsNativeNessYoYoBindingParents[0]));
            return sNdsNativeNessYoYoBindingParents;
        }
        /* The forward smash's bat is on Win3's joint 17: a real source parent
         * schedule, like Win3's. */
        if (ndsRendererNativeFighterRootProgram(slot) == 3u)
        {
            *count = (u32)(sizeof(sNdsNativeNessFSmashBindingParents) /
                           sizeof(sNdsNativeNessFSmashBindingParents[0]));
            return sNdsNativeNessFSmashBindingParents;
        }
#endif
        *count = (u32)(sizeof(sNdsNativeNessBindingParents) /
                       sizeof(sNdsNativeNessBindingParents[0]));
        return sNdsNativeNessBindingParents;
    }
    if (slot == 10u)
    {
#if defined(NDS_NATIVE_PURIN_ROOT_PROGRAMS_PRESENT)
        if (ndsRendererNativeFighterRootProgram(slot) == 1u)
        {
            *count = (u32)(sizeof(sNdsNativePurinAccessoryBindingParents) /
                           sizeof(sNdsNativePurinAccessoryBindingParents[0]));
            return sNdsNativePurinAccessoryBindingParents;
        }
#endif
        *count = (u32)(sizeof(sNdsNativePurinBindingParents) /
                       sizeof(sNdsNativePurinBindingParents[0]));
        return sNdsNativePurinBindingParents;
    }
    if (slot == 11u)
    {
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
        u32 program = ndsRendererNativeFighterRootProgram(slot);
        static const u8 *const trio_parents[NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT] = {
#define NDS_KIRBY_TRIO_X(head_) sNdsNativeKirbyTrioHead##head_##BindingParents,
            NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(NDS_KIRBY_TRIO_X)
#undef NDS_KIRBY_TRIO_X
        };
        static const u8 trio_parent_counts[NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT] = {
#define NDS_KIRBY_TRIO_X(head_) \
            (u8)(sizeof(sNdsNativeKirbyTrioHead##head_##BindingParents) / \
                 sizeof(sNdsNativeKirbyTrioHead##head_##BindingParents[0])),
            NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(NDS_KIRBY_TRIO_X)
#undef NDS_KIRBY_TRIO_X
        };

        if ((program >= 1u) && (program <= NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT))
        {
            *count = (u32)trio_parent_counts[program - 1u];
            return trio_parents[program - 1u];
        }
        if (program == NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 1u)
        {
            *count = (u32)(sizeof(sNdsNativeKirbyStoneBindingParents) /
                           sizeof(sNdsNativeKirbyStoneBindingParents[0]));
            return sNdsNativeKirbyStoneBindingParents;
        }
        if (program == NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 2u)
        {
            *count = (u32)(sizeof(sNdsNativeKirbyCopyLinkBindingParents) /
                           sizeof(sNdsNativeKirbyCopyLinkBindingParents[0]));
            return sNdsNativeKirbyCopyLinkBindingParents;
        }
        if (program == NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 3u)
        {
            *count = (u32)(sizeof(sNdsNativeKirbyCopyTransitionBindingParents) /
                           sizeof(sNdsNativeKirbyCopyTransitionBindingParents[0]));
            return sNdsNativeKirbyCopyTransitionBindingParents;
        }
#endif
        *count = (u32)(sizeof(sNdsNativeKirbyBindingParents) /
                       sizeof(sNdsNativeKirbyBindingParents[0]));
        return sNdsNativeKirbyBindingParents;
    }
#if NDS_MMARIO
    if (slot == 12u)
    {
        *count = (u32)(sizeof(sNdsNativeMMarioBindingParents) /
                       sizeof(sNdsNativeMMarioBindingParents[0]));
        return sNdsNativeMMarioBindingParents;
    }
#endif
#if NDS_NMARIO
    if (slot == 13u)
    {
        *count = (u32)(sizeof(sNdsNativeNMarioBindingParents) /
                       sizeof(sNdsNativeNMarioBindingParents[0]));
        return sNdsNativeNMarioBindingParents;
    }
#endif
#if NDS_NFOX
    if (slot == 14u)
    {
        *count = (u32)(sizeof(sNdsNativeNFoxBindingParents) /
                       sizeof(sNdsNativeNFoxBindingParents[0]));
        return sNdsNativeNFoxBindingParents;
    }
#endif
#define NDS_P4_OWNER_PARENTS(id_, T_, N_, n_, parent_, model_, main_)          \
    if (slot == NDS_P4_RENDERER_OWNER_SLOT(id_))                                \
    {                                                                           \
        const NDSP4RootProgram *program_ = ndsP4CurrentRootProgram(slot);       \
                                                                                \
        if (program_ != NULL)                                                   \
        {                                                                       \
            *count = program_->owners[0]->root_count;                           \
            return program_->parents;                                           \
        }                                                                       \
        *count = (u32)(sizeof(sNdsNative##T_##BindingParents) /                 \
                       sizeof(sNdsNative##T_##BindingParents[0]));              \
        return sNdsNative##T_##BindingParents;                                  \
    }
    NDS_P4_CONTENT_ROWS(NDS_P4_OWNER_PARENTS)
#undef NDS_P4_OWNER_PARENTS
#if NDS_NDONKEY
    if (slot == 15u)
    {
        *count = (u32)(sizeof(sNdsNativeNDonkeyBindingParents) /
                       sizeof(sNdsNativeNDonkeyBindingParents[0]));
        return sNdsNativeNDonkeyBindingParents;
    }
#endif
#if NDS_NSAMUS
    if (slot == 16u)
    {
        *count = (u32)(sizeof(sNdsNativeNSamusBindingParents) /
                       sizeof(sNdsNativeNSamusBindingParents[0]));
        return sNdsNativeNSamusBindingParents;
    }
#endif
#if NDS_NLINK
    if (slot == 17u)
    {
        *count = (u32)(sizeof(sNdsNativeNLinkBindingParents) /
                       sizeof(sNdsNativeNLinkBindingParents[0]));
        return sNdsNativeNLinkBindingParents;
    }
#endif
#if NDS_NYOSHI
    if (slot == 18u)
    {
        *count = (u32)(sizeof(sNdsNativeNYoshiBindingParents) /
                       sizeof(sNdsNativeNYoshiBindingParents[0]));
        return sNdsNativeNYoshiBindingParents;
    }
#endif
#if NDS_NCAPTAIN
    if (slot == 19u)
    {
        *count = (u32)(sizeof(sNdsNativeNCaptainBindingParents) /
                       sizeof(sNdsNativeNCaptainBindingParents[0]));
        return sNdsNativeNCaptainBindingParents;
    }
#endif
#if NDS_NKIRBY
    if (slot == 20u)
    {
        *count = (u32)(sizeof(sNdsNativeNKirbyBindingParents) /
                       sizeof(sNdsNativeNKirbyBindingParents[0]));
        return sNdsNativeNKirbyBindingParents;
    }
#endif
#if NDS_NPIKACHU
    if (slot == 21u)
    {
        *count = (u32)(sizeof(sNdsNativeNPikachuBindingParents) /
                       sizeof(sNdsNativeNPikachuBindingParents[0]));
        return sNdsNativeNPikachuBindingParents;
    }
#endif
#if NDS_NPURIN
    if (slot == 22u)
    {
        *count = (u32)(sizeof(sNdsNativeNPurinBindingParents) /
                       sizeof(sNdsNativeNPurinBindingParents[0]));
        return sNdsNativeNPurinBindingParents;
    }
#endif
#if NDS_NNESS
    if (slot == 23u)
    {
        *count = (u32)(sizeof(sNdsNativeNNessBindingParents) /
                       sizeof(sNdsNativeNNessBindingParents[0]));
        return sNdsNativeNNessBindingParents;
    }
#endif
#if NDS_1P_GAME
    if (slot == 24u)
    {
        *count = (u32)(sizeof(sNdsNativeBossBindingParents) /
                       sizeof(sNdsNativeBossBindingParents[0]));
        return sNdsNativeBossBindingParents;
    }
#endif
    return NULL;
}

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
    if (ndsRendererNativeFighterRootProgram(slot) == NDS_NATIVE_SKELETON_PROGRAM)
    {
        *count = ndsNativeSkeletonOwner(slot, 1u)->root_count;
        return sNdsSkeletonCrossSlots;
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
#if NDS_LUIGI
    if (slot == 2u)
    {
        *count = (u32)(sizeof(sNdsNativeLuigiCrossPaletteSlots) /
                       sizeof(sNdsNativeLuigiCrossPaletteSlots[0]));
        return sNdsNativeLuigiCrossPaletteSlots;
    }
#endif
#if NDS_DONKEY
    if (slot == 3u)
    {
        *count = (u32)(sizeof(sNdsNativeDonkeyCrossPaletteSlots) /
                       sizeof(sNdsNativeDonkeyCrossPaletteSlots[0]));
        return sNdsNativeDonkeyCrossPaletteSlots;
    }
#endif
#if NDS_CAPTAIN
    if (slot == 4u)
    {
        *count = (u32)(sizeof(sNdsNativeCaptainCrossPaletteSlots) /
                       sizeof(sNdsNativeCaptainCrossPaletteSlots[0]));
        return sNdsNativeCaptainCrossPaletteSlots;
    }
#endif
    if (slot == 5u)
    {
#if defined(NDS_NATIVE_SAMUS_ROOT_PROGRAMS_PRESENT)
        if (ndsRendererNativeFighterRootProgram(slot) == 1u)
        {
            *count = (u32)(sizeof(sNdsNativeSamusCatchCrossPaletteSlots) /
                           sizeof(sNdsNativeSamusCatchCrossPaletteSlots[0]));
            return sNdsNativeSamusCatchCrossPaletteSlots;
        }
        if (ndsRendererNativeFighterRootProgram(slot) == 2u)
        {
            *count = (u32)(sizeof(sNdsNativeSamusMorphUnfoldCrossPaletteSlots) /
                           sizeof(sNdsNativeSamusMorphUnfoldCrossPaletteSlots[0]));
            return sNdsNativeSamusMorphUnfoldCrossPaletteSlots;
        }
        if (ndsRendererNativeFighterRootProgram(slot) == 3u)
        {
            *count = (u32)(sizeof(sNdsNativeSamusMorphBallCrossPaletteSlots) /
                           sizeof(sNdsNativeSamusMorphBallCrossPaletteSlots[0]));
            return sNdsNativeSamusMorphBallCrossPaletteSlots;
        }
        if (ndsRendererNativeFighterRootProgram(slot) == 4u)
        {
            *count = (u32)(sizeof(sNdsNativeSamusFSmashCrossPaletteSlots) /
                           sizeof(sNdsNativeSamusFSmashCrossPaletteSlots[0]));
            return sNdsNativeSamusFSmashCrossPaletteSlots;
        }
#endif
        *count = (u32)(sizeof(sNdsNativeSamusCrossPaletteSlots) /
                       sizeof(sNdsNativeSamusCrossPaletteSlots[0]));
        return sNdsNativeSamusCrossPaletteSlots;
    }
#if NDS_LINK
    if (slot == 6u)
    {
#if defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT)
        u32 program = ndsRendererNativeFighterRootProgram(slot);
        if (program == 1u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkEntryCrossPaletteSlots) /
                           sizeof(sNdsNativeLinkEntryCrossPaletteSlots[0]));
            return sNdsNativeLinkEntryCrossPaletteSlots;
        }
        if (program == 2u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkCatchCrossPaletteSlots) /
                           sizeof(sNdsNativeLinkCatchCrossPaletteSlots[0]));
            return sNdsNativeLinkCatchCrossPaletteSlots;
        }
        if (program == 3u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkSpecialNCrossPaletteSlots) /
                           sizeof(sNdsNativeLinkSpecialNCrossPaletteSlots[0]));
            return sNdsNativeLinkSpecialNCrossPaletteSlots;
        }
        if (program == 4u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkClapsCrossPaletteSlots) /
                           sizeof(sNdsNativeLinkClapsCrossPaletteSlots[0]));
            return sNdsNativeLinkClapsCrossPaletteSlots;
        }
        if (program == 5u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkSpecialNItemCrossPaletteSlots) /
                           sizeof(sNdsNativeLinkSpecialNItemCrossPaletteSlots[0]));
            return sNdsNativeLinkSpecialNItemCrossPaletteSlots;
        }
#if defined(NDS_NATIVE_LINK_INTRO_PROGRAM_PRESENT)
        if (program == 6u)
        {
            *count = (u32)(sizeof(sNdsNativeLinkIntroCrossPaletteSlots) /
                           sizeof(sNdsNativeLinkIntroCrossPaletteSlots[0]));
            return sNdsNativeLinkIntroCrossPaletteSlots;
        }
#endif
#endif
        *count = (u32)(sizeof(sNdsNativeLinkCrossPaletteSlots) /
                       sizeof(sNdsNativeLinkCrossPaletteSlots[0]));
        return sNdsNativeLinkCrossPaletteSlots;
    }
#endif
    if (slot == 7u)
    {
#if defined(NDS_NATIVE_PIKACHU_ROOT_PROGRAMS_PRESENT)
        if (ndsRendererNativeFighterRootProgram(slot) == 1u)
        {
            *count = (u32)(sizeof(sNdsNativePikachuAccessoryCrossPaletteSlots) /
                           sizeof(sNdsNativePikachuAccessoryCrossPaletteSlots[0]));
            return sNdsNativePikachuAccessoryCrossPaletteSlots;
        }
#endif
        *count = (u32)(sizeof(sNdsNativePikachuCrossPaletteSlots) /
                       sizeof(sNdsNativePikachuCrossPaletteSlots[0]));
        return sNdsNativePikachuCrossPaletteSlots;
    }
    if (slot == 8u)
    {
#if defined(NDS_NATIVE_YOSHI_ROOT_PROGRAMS_PRESENT)
        u32 program = ndsRendererNativeFighterRootProgram(slot);
        if (program == 1u)
        {
            *count = (u32)(sizeof(sNdsNativeYoshiCatchCrossPaletteSlots) /
                           sizeof(sNdsNativeYoshiCatchCrossPaletteSlots[0]));
            return sNdsNativeYoshiCatchCrossPaletteSlots;
        }
        if (program == 2u)
        {
            *count = (u32)(sizeof(sNdsNativeYoshiThrowCrossPaletteSlots) /
                           sizeof(sNdsNativeYoshiThrowCrossPaletteSlots[0]));
            return sNdsNativeYoshiThrowCrossPaletteSlots;
        }
#endif
        *count = (u32)(sizeof(sNdsNativeYoshiCrossPaletteSlots) /
                       sizeof(sNdsNativeYoshiCrossPaletteSlots[0]));
        return sNdsNativeYoshiCrossPaletteSlots;
    }
    if (slot == 9u)
    {
#if defined(NDS_NATIVE_NESS_ROOT_PROGRAMS_PRESENT)
        if (ndsRendererNativeFighterRootProgram(slot) == 1u)
        {
            *count = (u32)(sizeof(sNdsNativeNessWin3CrossPaletteSlots) /
                           sizeof(sNdsNativeNessWin3CrossPaletteSlots[0]));
            return sNdsNativeNessWin3CrossPaletteSlots;
        }
        if (ndsRendererNativeFighterRootProgram(slot) == 2u)
        {
            *count = (u32)(sizeof(sNdsNativeNessYoYoCrossPaletteSlots) /
                           sizeof(sNdsNativeNessYoYoCrossPaletteSlots[0]));
            return sNdsNativeNessYoYoCrossPaletteSlots;
        }
        if (ndsRendererNativeFighterRootProgram(slot) == 3u)
        {
            *count = (u32)(sizeof(sNdsNativeNessFSmashCrossPaletteSlots) /
                           sizeof(sNdsNativeNessFSmashCrossPaletteSlots[0]));
            return sNdsNativeNessFSmashCrossPaletteSlots;
        }
#endif
        *count = (u32)(sizeof(sNdsNativeNessCrossPaletteSlots) /
                       sizeof(sNdsNativeNessCrossPaletteSlots[0]));
        return sNdsNativeNessCrossPaletteSlots;
    }
    if (slot == 10u)
    {
#if defined(NDS_NATIVE_PURIN_ROOT_PROGRAMS_PRESENT)
        if (ndsRendererNativeFighterRootProgram(slot) == 1u)
        {
            *count = (u32)(sizeof(sNdsNativePurinAccessoryCrossPaletteSlots) /
                           sizeof(sNdsNativePurinAccessoryCrossPaletteSlots[0]));
            return sNdsNativePurinAccessoryCrossPaletteSlots;
        }
#endif
        *count = (u32)(sizeof(sNdsNativePurinCrossPaletteSlots) /
                       sizeof(sNdsNativePurinCrossPaletteSlots[0]));
        return sNdsNativePurinCrossPaletteSlots;
    }
    if (slot == 11u)
    {
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
        u32 program = ndsRendererNativeFighterRootProgram(slot);
        static const u8 *const trio_cross[NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT] = {
#define NDS_KIRBY_TRIO_X(head_) sNdsNativeKirbyTrioHead##head_##CrossPaletteSlots,
            NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(NDS_KIRBY_TRIO_X)
#undef NDS_KIRBY_TRIO_X
        };
        static const u8 trio_cross_counts[NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT] = {
#define NDS_KIRBY_TRIO_X(head_) \
            (u8)(sizeof(sNdsNativeKirbyTrioHead##head_##CrossPaletteSlots) / \
                 sizeof(sNdsNativeKirbyTrioHead##head_##CrossPaletteSlots[0])),
            NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(NDS_KIRBY_TRIO_X)
#undef NDS_KIRBY_TRIO_X
        };

        if ((program >= 1u) && (program <= NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT))
        {
            *count = (u32)trio_cross_counts[program - 1u];
            return trio_cross[program - 1u];
        }
        if (program == NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 1u)
        {
            *count = (u32)(sizeof(sNdsNativeKirbyStoneCrossPaletteSlots) /
                           sizeof(sNdsNativeKirbyStoneCrossPaletteSlots[0]));
            return sNdsNativeKirbyStoneCrossPaletteSlots;
        }
        if (program == NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 2u)
        {
            *count = (u32)(sizeof(sNdsNativeKirbyCopyLinkCrossPaletteSlots) /
                           sizeof(sNdsNativeKirbyCopyLinkCrossPaletteSlots[0]));
            return sNdsNativeKirbyCopyLinkCrossPaletteSlots;
        }
        if (program == NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 3u)
        {
            *count = (u32)(sizeof(sNdsNativeKirbyCopyTransitionCrossPaletteSlots) /
                           sizeof(sNdsNativeKirbyCopyTransitionCrossPaletteSlots[0]));
            return sNdsNativeKirbyCopyTransitionCrossPaletteSlots;
        }
#endif
        *count = (u32)(sizeof(sNdsNativeKirbyCrossPaletteSlots) /
                       sizeof(sNdsNativeKirbyCrossPaletteSlots[0]));
        return sNdsNativeKirbyCrossPaletteSlots;
    }
#if NDS_MMARIO
    if (slot == 12u)
    {
        *count = (u32)(sizeof(sNdsNativeMMarioCrossPaletteSlots) /
                       sizeof(sNdsNativeMMarioCrossPaletteSlots[0]));
        return sNdsNativeMMarioCrossPaletteSlots;
    }
#endif
#if NDS_NMARIO
    if (slot == 13u)
    {
        *count = (u32)(sizeof(sNdsNativeNMarioCrossPaletteSlots) /
                       sizeof(sNdsNativeNMarioCrossPaletteSlots[0]));
        return sNdsNativeNMarioCrossPaletteSlots;
    }
#endif
#if NDS_NFOX
    if (slot == 14u)
    {
        *count = (u32)(sizeof(sNdsNativeNFoxCrossPaletteSlots) /
                       sizeof(sNdsNativeNFoxCrossPaletteSlots[0]));
        return sNdsNativeNFoxCrossPaletteSlots;
    }
#endif
#define NDS_P4_OWNER_CROSS(id_, T_, N_, n_, parent_, model_, main_)            \
    if (slot == NDS_P4_RENDERER_OWNER_SLOT(id_))                                \
    {                                                                           \
        const NDSP4RootProgram *program_ = ndsP4CurrentRootProgram(slot);       \
                                                                                \
        if (program_ != NULL)                                                   \
        {                                                                       \
            *count = program_->owners[0]->root_count;                           \
            return program_->cross;                                             \
        }                                                                       \
        *count = (u32)(sizeof(sNdsNative##T_##CrossPaletteSlots) /              \
                       sizeof(sNdsNative##T_##CrossPaletteSlots[0]));           \
        return sNdsNative##T_##CrossPaletteSlots;                               \
    }
    NDS_P4_CONTENT_ROWS(NDS_P4_OWNER_CROSS)
#undef NDS_P4_OWNER_CROSS
#if NDS_NDONKEY
    if (slot == 15u)
    {
        *count = (u32)(sizeof(sNdsNativeNDonkeyCrossPaletteSlots) /
                       sizeof(sNdsNativeNDonkeyCrossPaletteSlots[0]));
        return sNdsNativeNDonkeyCrossPaletteSlots;
    }
#endif
#if NDS_NSAMUS
    if (slot == 16u)
    {
        *count = (u32)(sizeof(sNdsNativeNSamusCrossPaletteSlots) /
                       sizeof(sNdsNativeNSamusCrossPaletteSlots[0]));
        return sNdsNativeNSamusCrossPaletteSlots;
    }
#endif
#if NDS_NLINK
    if (slot == 17u)
    {
        *count = (u32)(sizeof(sNdsNativeNLinkCrossPaletteSlots) /
                       sizeof(sNdsNativeNLinkCrossPaletteSlots[0]));
        return sNdsNativeNLinkCrossPaletteSlots;
    }
#endif
#if NDS_NYOSHI
    if (slot == 18u)
    {
        *count = (u32)(sizeof(sNdsNativeNYoshiCrossPaletteSlots) /
                       sizeof(sNdsNativeNYoshiCrossPaletteSlots[0]));
        return sNdsNativeNYoshiCrossPaletteSlots;
    }
#endif
#if NDS_NCAPTAIN
    if (slot == 19u)
    {
        *count = (u32)(sizeof(sNdsNativeNCaptainCrossPaletteSlots) /
                       sizeof(sNdsNativeNCaptainCrossPaletteSlots[0]));
        return sNdsNativeNCaptainCrossPaletteSlots;
    }
#endif
#if NDS_NKIRBY
    if (slot == 20u)
    {
        *count = (u32)(sizeof(sNdsNativeNKirbyCrossPaletteSlots) /
                       sizeof(sNdsNativeNKirbyCrossPaletteSlots[0]));
        return sNdsNativeNKirbyCrossPaletteSlots;
    }
#endif
#if NDS_NPIKACHU
    if (slot == 21u)
    {
        *count = (u32)(sizeof(sNdsNativeNPikachuCrossPaletteSlots) /
                       sizeof(sNdsNativeNPikachuCrossPaletteSlots[0]));
        return sNdsNativeNPikachuCrossPaletteSlots;
    }
#endif
#if NDS_NPURIN
    if (slot == 22u)
    {
        *count = (u32)(sizeof(sNdsNativeNPurinCrossPaletteSlots) /
                       sizeof(sNdsNativeNPurinCrossPaletteSlots[0]));
        return sNdsNativeNPurinCrossPaletteSlots;
    }
#endif
#if NDS_NNESS
    if (slot == 23u)
    {
        *count = (u32)(sizeof(sNdsNativeNNessCrossPaletteSlots) /
                       sizeof(sNdsNativeNNessCrossPaletteSlots[0]));
        return sNdsNativeNNessCrossPaletteSlots;
    }
#endif
#if NDS_1P_GAME
    if (slot == 24u)
    {
        *count = (u32)(sizeof(sNdsNativeBossCrossPaletteSlots) /
                       sizeof(sNdsNativeBossCrossPaletteSlots[0]));
        return sNdsNativeBossCrossPaletteSlots;
    }
#endif
    return NULL;
}

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
                &sNdsNativeFighterHighTables.epochs[epoch_index];
            NDSNativeHierarchyPreparedEpoch *prepared_epoch =
                &execution->hierarchy_epochs[epoch_index];
            u32 run_offset;

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
            memset(&execution->hierarchy_runs[epoch_index], 0,
                   sizeof(execution->hierarchy_runs[epoch_index]));
            for (run_offset = 0u;
                 run_offset < epoch->run_count;
                 run_offset++)
            {
                u32 run_index = epoch->first_run + run_offset;

                if (ndsRendererNativePrepareHierarchyRun(
                        run_index,
                        sNdsNativeFighterHighTables.epoch_direct_policy[epoch_index],
                        TRUE, hierarchy->config, scratch, state,
                        &execution->hierarchy_runs[epoch_index]) == FALSE)
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
    unique_first = sNdsNativeFighterHighTables.run_first_unique[run_index];
    unique_count = sNdsNativeFighterHighTables.run_unique_count[run_index];
    for (unique_offset = 0u; unique_offset < unique_count; unique_offset++)
    {
        u32 dense_id = sNdsNativeFighterHighTables.run_unique_dense[
            unique_first + unique_offset];
        const NDSNativeDenseVertex *dense =
            &sNdsNativeFighterHighTables.dense_vertices[dense_id];
        NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterHighTables.prepared_dense[dense_id];
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
    u32 poly_fmt = prepared_run->poly_fmt;

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
            &sNdsNativeFighterHighTables.epochs[epoch_index];
        const NDSNativeHierarchyPreparedEpoch *prepared_epoch =
            &execution->hierarchy_epochs[epoch_index];
        u32 run_offset;

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
        (void)ndsRendererNativeShadeProductionActions(
            epoch, sNdsNativeFighterHighTables.epoch_direct_policy[epoch_index],
            FALSE, stats, state);

        for (run_offset = 0u;
             run_offset < epoch->run_count;
             run_offset++)
        {
            u32 run_index = epoch->first_run + run_offset;
            const NDSNativeRun *run =
                &sNdsNativeFighterActiveTables->runs[run_index];
            u32 submit_class = (u32)run->submit_class &
                NDS_NATIVE_RUN_SUBMIT_CLASS_MASK;
            const NDSNativeHierarchyPreparedRun *prepared_run =
                &execution->hierarchy_runs[epoch_index];

            ndsRendererNativePrepareHierarchyTexcoords(
                run_index, prepared_run);
            ndsRendererNativeBeginHierarchyBatch(
                stats, prepared_run, matrix_generation);
            if (submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
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
            stats->triangle_count += run->triangle_count;
            if (submit_class == NDS_NATIVE_RUN_RAW_CURRENT)
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
        }
    }
    ndsRendererNativeApplyStateSpan(
        root->tail_state_first, root->tail_state_count,
        root->tail_sync_count, asset_base, stats, state);
    stats->end_command_count++;
}
