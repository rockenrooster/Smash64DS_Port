s32 NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererExecuteNativeFighterOwnerProduction(
    u32 slot,
    u32 use_low_detail,
    u32 texture_memo_owner_key,
    u32 packet_key,
    const void *asset_base_ptr,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    u32 *out_hardware_started)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    const u8 *palette_slots;
    const u8 *binding_palette_slots = NULL;
    const u8 *asset_base = asset_base_ptr;
    NDSRendererTraversalState *state =
        &sNdsNativeFighterOwnerExecution.traversal;
    u32 root_count;
    u32 root_index;
    u32 native_run_count = 0u;
    u32 native_triangle_count = 0u;
    u32 raw_triangle_count = 0u;
    u32 raw_reuse_count = 0u;
    u32 cross_triangle_count = 0u;
    u32 cross_reuse_count = 0u;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    volatile NDSRendererOwnerProfile *m2_owner;
    u32 m2_total_start;
    u32 m2_lighting_before = 0u;
    u32 m2_root_gx_before = 0u;
    u32 m2_run_prepare_before = 0u;
    u32 m2_emit_account_before = 0u;
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    u32 e15b_mark;
#endif

    (void)callback_user;
    if (out_hardware_started == NULL)
    {
        return FALSE;
    }
    *out_hardware_started = FALSE;
    if (ndsRendererNativeSelectFighterRuntimeTables(
            slot, use_low_detail) == FALSE)
    {
        return FALSE;
    }
    /* The adapter packs generated owner, detail, battle instance and source
     * costume/shade once before entering this ITCM-resident executor. Keeping
     * that cold identity construction out of the native run loop preserves the
     * 32 KiB ITCM boundary while the hot memo still pays one word store here. */
    sNdsNativeFighterOwnerExecution.texture_memo_owner_key =
        texture_memo_owner_key;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_owner = ndsRendererProfileM2Owner();
    if (m2_owner != NULL)
    {
        m2_lighting_before = m2_owner->m2_lighting_shading_ticks;
        m2_root_gx_before = m2_owner->m2_root_gx_ticks;
        m2_run_prepare_before = m2_owner->m2_run_prepare_ticks;
        m2_emit_account_before = m2_owner->m2_corner_emit_account_ticks;
    }
    m2_total_start = cpuGetTiming();
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    e15b_mark = cpuGetTiming();
#endif
    if ((stats == NULL) ||
        (stats->blocker != NDS_RENDERER_BLOCKER_NONE) ||
        (ndsRendererNativePreflightProductionOwner(
             slot, use_low_detail, asset_base, inputs, input_count,
             callback, stats) == FALSE))
    {
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        ndsRendererProfileM2FinishProduction(
            m2_owner, m2_total_start,
            m2_lighting_before, m2_root_gx_before,
            m2_run_prepare_before, m2_emit_account_before, FALSE);
#endif
        return FALSE;
    }
#if NDS_TASK91_DRAW_PHASE_CENSUS
    gNdsR2ExecPreflightTicks += cpuGetTiming() - e15b_mark;
#endif
#if NDS_FIGHTER_PACKET_LIVE
    if (ndsFighterPacketTryReplay(
            slot, use_low_detail, texture_memo_owner_key, packet_key,
            inputs, input_count, stats, out_hardware_started) != 0)
    {
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        ndsRendererProfileM2FinishProduction(
            m2_owner, m2_total_start,
            m2_lighting_before, m2_root_gx_before,
            m2_run_prepare_before, m2_emit_account_before, TRUE);
#endif
        return TRUE;
    }
#else
    (void)packet_key;
#endif
    root_count = sNdsNativeFighterActiveOwner->root_count;
    palette_slots = sNdsNativeFighterActiveOwner->cross_palette_slots;

    ndsRendererInitTraversalState(
        state, NULL, stats, NULL, NULL, 0u);
#if NDS_R2_FIGHTER_HW_LIGHT
    /* R2-03 E16. Light 0's colour is white and the source's two light colours
     * become the material's diffuse and ambient, so the engine evaluates
     * exactly light_color_2 + light_color_1 * dot(N, L). The light vector
     * itself cannot be written here -- stats->light_dir_* is only populated by
     * the epoch state deltas -- so ndsRendererR2WriteLightVector does it on the
     * first epoch of this execute that has a direction. */
    if (*sNdsNativeFighterActiveDenseNormalsBuilt == 0u)
    {
        ndsRendererR2BuildDenseNormals();
    }
    sNdsR2LightVectorWritten = 0u;
    /* The light colour does not depend on any matrix, so unlike the vector it
     * belongs here rather than in the shade. Keeping them separate also stops
     * one test from conflating "the colour never arrived" with "the dot product
     * is zero". */
    GFX_LIGHT_COLOR = (u32)RGB15(31, 31, 31);
    NDS_FIGHTER_PACKET_HOOK(
        ndsFighterPacketCmd1(REG2ID(GFX_LIGHT_COLOR),
                             (u32)RGB15(31, 31, 31)));
#endif
#if NDS_R2_FIGHTER_GX_COMPOSE
    /* Nothing outside this loop is known to leave GL_PROJECTION alone, so the
     * projection elide only claims what one execute can prove. */
    sNdsR2GxLastProjection = NULL;
#endif
    for (root_index = 0u; root_index < root_count; root_index++)
    {
        const NDSRendererNativeFighterRoot *input = &inputs[root_index];
        const NDSNativeRoot *root =
            sNdsNativeProductionResolvedRoots[root_index];
        u32 palette_slot = palette_slots[root_index];
        u32 epoch_offset;

#if NDS_TASK91_DRAW_PHASE_CENSUS
        e15b_mark = cpuGetTiming();
#endif
        ndsRendererNativeBindProductionRoot(state, input, stats);
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        {
            u32 m2_root_gx_start = cpuGetTiming();
#endif
        *out_hardware_started = TRUE;
#if NDS_R2_FIGHTER_GX_COMPOSE
        /* The capture is allowed to decline, and when it does the adapter has
         * composed on the CPU exactly as before, so the split loader is still
         * the fail-closed answer for this owner. */
        if (input->gx_valid != 0u)
        {
#if NDS_FIGHTER_PACKET_LIVE
            if (sNdsFighterPacketRecording != 0u)
            {
                ndsFighterPacketLoadGxComposedRecord(
                    root_index, input, state->matrix_generation);
            }
            else
#endif
            ndsRendererLoadHardwareGxComposedMatrices(
                input, state->matrix_generation);
        }
        else
        {
            /* The split loader has no packet tee; fault the record and draw. */
            NDS_FIGHTER_PACKET_HOOK(sNdsFighterPacketRecorder.fault = 1u);
            ndsRendererLoadHardwareSplitMatrices(
                input->projection_matrix, input->modelview_matrix,
                state->matrix_generation);
        }
#elif NDS_R2_FIGHTER_HW_MTX
        /* Straight from the root, which BindProductionRoot no longer copies
         * into the traversal state -- see the trace there. */
        ndsRendererLoadHardwareSplitMatrices(
            input->projection_matrix, input->modelview_matrix,
            state->matrix_generation);
#else
        ndsRendererLoadHardwareRawComposedMatrix(
            &state->matrix, state->matrix_generation);
#endif
        if (palette_slot <= NDS_NATIVE_GX_MATRIX_SLOT_MAX)
        {
            glStoreMatrix((int)palette_slot);
            NDS_FIGHTER_PACKET_HOOK(
                ndsFighterPacketCmd1(REG2ID(MATRIX_STORE), palette_slot));
        }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            if (m2_owner != NULL)
            {
                m2_owner->m2_root_gx_ticks +=
                    cpuGetTiming() - m2_root_gx_start;
                m2_owner->m2_root_gx_count++;
            }
        }
#endif
#endif
        if (stats->first_opcode == 0u)
        {
            stats->first_opcode = NDS_RENDERER_OP_RDPPIPESYNC;
        }
        stats->command_count += root->source_command_count;
        ndsRendererNativeApplyRootLightPreamble(root, stats);
#if NDS_TASK91_DRAW_PHASE_CENSUS
        gNdsR2ExecRootTicks += cpuGetTiming() - e15b_mark;
#endif

        for (epoch_offset = 0u;
             epoch_offset < root->epoch_count;
             epoch_offset++)
        {
            u32 epoch_index = root->first_epoch + epoch_offset;
            const NDSNativeEpoch *epoch =
                &sNdsNativeFighterActiveTables->epochs[epoch_index];
            u32 run_offset;

#if NDS_TASK91_DRAW_PHASE_CENSUS
            e15b_mark = cpuGetTiming();
            gNdsR2ExecEpochCalls++;
#endif
#if !NDS_R2_FIGHTER_STATESPAN_SKIP
#if NDS_TASK91_DRAW_PHASE_CENSUS
            {
                /* R2-03 E38. E26 folds the two STATIC spans and must keep
                 * ApplyMaterial live (E34-b: materials are rebuilt from the live
                 * MObj every frame). The material sits BETWEEN the spans, so a
                 * fold of the before-span alone is straightforward while folding
                 * the after-span means re-applying it over live material writes.
                 * Which is worth building depends on how the replay's 65,026
                 * splits across the two, and nothing has measured that -- E20
                 * and E25 both counted the spans together. */
                u32 t_span = cpuGetTiming();
#endif
            ndsRendererNativeApplyStateSpan(
                epoch->before_state_first, epoch->before_state_count,
                epoch->before_sync_count,
                asset_base, stats, state);
#if NDS_TASK91_DRAW_PHASE_CENSUS
                gNdsR2SpanBeforeTicks += cpuGetTiming() - t_span;
                gNdsR2SpanBeforeDeltas += epoch->before_state_count;
            }
#endif
#endif
            if (epoch->material_slot != NDS_NATIVE_MATERIAL_NONE)
            {
                ndsRendererNativeApplyMaterial(
                    &input->materials[epoch->material_slot], stats, state);
            }
#if !NDS_R2_FIGHTER_STATESPAN_SKIP
#if NDS_TASK91_DRAW_PHASE_CENSUS
            {
                u32 t_span = cpuGetTiming();
#endif
            ndsRendererNativeApplyStateSpan(
                epoch->after_state_first, epoch->after_state_count,
                epoch->after_sync_count,
                asset_base, stats, state);
#if NDS_TASK91_DRAW_PHASE_CENSUS
                gNdsR2SpanAfterTicks += cpuGetTiming() - t_span;
                gNdsR2SpanAfterDeltas += epoch->after_state_count;
            }
#endif
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
            {
                u32 e15b_state_end = cpuGetTiming();

                gNdsR2ExecStateTicks += e15b_state_end - e15b_mark;
                e15b_mark = e15b_state_end;
            }
#endif
#if NDS_R2_FIGHTER_EPOCH_STATE_PROOF
            /* R2-03 E34. E26 wants to replace the state replay with a baked
             * per-epoch snapshot, which is only sound if the state an epoch
             * reaches is a function of the epoch index. It might not be:
             * ndsRendererNativeApplyMaterial writes the same stats fields from a
             * live input, so a material applied in epoch N can survive into
             * epoch N+1's snapshot through any field N+1's before-span does not
             * itself rewrite. That is a stronger objection than E26's own §2a
             * correction raised, and it decides whether the fold is a table or a
             * table plus a repair, so measure it before building either.
             *
             * Hashed AFTER the material and after-span, i.e. the complete state
             * an epoch hands to its runs, per epoch index, counting frames whose
             * value differs from the one already stored. */
            ndsRendererR2EpochStateProof(epoch_index, stats);
#endif
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            {
                u32 m2_lighting_start = cpuGetTiming();
#endif
            (void)ndsRendererNativeShadeProductionActions(
                epoch,
                sNdsNativeFighterActiveTables->epoch_direct_policy[epoch_index],
                FALSE,
                stats, state);
#if NDS_TASK91_DRAW_PHASE_CENSUS
            gNdsR2ExecShadeTicks += cpuGetTiming() - e15b_mark;
#endif
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                if (m2_owner != NULL)
                {
                    m2_owner->m2_lighting_shading_ticks +=
                        cpuGetTiming() - m2_lighting_start;
                    m2_owner->m2_lighting_epoch_count++;
                }
            }
#endif
#if NDS_RENDERER_M2_DETAILED_LEDGER
            ndsRendererM2ShadeCensusEpoch(
                slot, inputs[0].owner_generation, epoch_index, epoch,
                (state->prepared_light_direction_valid != 0u) ?
                    &state->prepared_light_direction : NULL,
                state->prepared_light_direction_valid, stats);
            ndsRendererM2ShadeRecordProduced(
                slot, inputs[0].owner_generation, epoch_index, epoch,
                sNdsNativeFighterActiveTables->epoch_direct_policy[epoch_index],
                stats);
#endif

            for (run_offset = 0u;
                 run_offset < epoch->run_count;
                 run_offset++)
            {
                const NDSNativeRun *run =
                    &sNdsNativeFighterActiveTables->runs[
                        epoch->first_run + run_offset];

#if NDS_RENDERER_SCREEN_SPACE_CENSUS
                ndsRendererScreenSpaceCensusFighterRun(
                    run, inputs, input_count, root_index);
#endif
                if (ndsRendererNativeSubmitProductionRun(
                        run,
                        sNdsNativeFighterActiveTables->epoch_direct_policy[
                            epoch_index],
                        palette_slot, binding_palette_slots,
                        input->config,
                        stats, state,
                        &raw_triangle_count, &raw_reuse_count,
                        &cross_triangle_count, &cross_reuse_count) == FALSE)
                {
                    if (raw_triangle_count != 0u)
                    {
                        ndsRendererFastAccountRawTriangles(
                            stats, raw_triangle_count, raw_reuse_count);
                    }
                    if (cross_triangle_count != 0u)
                    {
                        ndsRendererNativeAccountGXCrossTriangles(
                            stats, cross_triangle_count, cross_reuse_count);
                    }
                    ndsRendererHardwareEndBatch();
                    NDS_FIGHTER_PACKET_HOOK(ndsFighterPacketAbortRecord());
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                    ndsRendererProfileM2FinishProduction(
                        m2_owner, m2_total_start,
                        m2_lighting_before, m2_root_gx_before,
                        m2_run_prepare_before, m2_emit_account_before,
                        FALSE);
#endif
                    return FALSE;
                }
                native_run_count++;
                native_triangle_count += run->triangle_count;
            }
        }
        ndsRendererNativeApplyStateSpan(
            root->tail_state_first, root->tail_state_count,
            root->tail_sync_count,
            asset_base, stats, state);
        stats->end_command_count++;
    }
    if (raw_triangle_count != 0u)
    {
        ndsRendererFastAccountRawTriangles(
            stats, raw_triangle_count, raw_reuse_count);
    }
    if (cross_triangle_count != 0u)
    {
        ndsRendererNativeAccountGXCrossTriangles(
            stats, cross_triangle_count, cross_reuse_count);
    }
    ndsRendererHardwareEndBatch();
    NDS_FIGHTER_PACKET_HOOK(ndsFighterPacketFinishRecord(
        native_run_count, native_triangle_count,
        raw_triangle_count, raw_reuse_count,
        cross_triangle_count, cross_reuse_count));
    sNdsRendererFastRunCount += native_run_count;
    sNdsRendererFastTriangleCount += native_triangle_count;
    if ((u32)sNdsRendererRuntimeOwner <
        (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererFastOwnerTriangleCount[
            (u32)sNdsRendererRuntimeOwner] += native_triangle_count;
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    ndsRendererProfileM2FinishProduction(
        m2_owner, m2_total_start,
        m2_lighting_before, m2_root_gx_before,
        m2_run_prepare_before, m2_emit_account_before, TRUE);
#endif
    return TRUE;
#else
    (void)slot;
    (void)use_low_detail;
    (void)texture_memo_owner_key;
    (void)packet_key;
    (void)asset_base_ptr;
    (void)inputs;
    (void)input_count;
    (void)callback;
    (void)callback_user;
    (void)stats;
    if (out_hardware_started != NULL)
    {
        *out_hardware_started = FALSE;
    }
    return FALSE;
#endif
}

#if !NDS_RENDERER_HW_TRIANGLES
static void ndsRendererTextureSourceHashCommand(
    NDSRendererStats *stats, u32 w0, u32 w1)
{
    (void)stats;
    (void)w0;
    (void)w1;
}
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static void ndsRendererClearNativeFighterOwner(void)
{
    ndsRendererHardwareEndBatch();
    sNdsNativeFighterOwnerExecution.stats = NULL;
    sNdsNativeFighterOwnerExecution.vertex_cache = NULL;
    sNdsNativeFighterOwnerExecution.slot = 0u;
    sNdsNativeFighterOwnerExecution.active = FALSE;
}
#endif

s32 ndsRendererBeginNativeFighterOwner(
    u32 slot,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    NDSRendererTraversalState *state;

    if ((slot >= NDS_NATIVE_FIGHTER_OWNER_COUNT) ||
        (stats == NULL) || (vertex_cache == NULL) ||
        (sNdsNativeFighterOwnerExecution.active != 0u))
    {
        return FALSE;
    }
    state = &sNdsNativeFighterOwnerExecution.traversal;
    ndsRendererInitTraversalState(
        state, NULL, stats, NULL,
        vertex_cache->matrix_snapshots,
        vertex_cache->matrix_snapshot_count);
    state->vertices = vertex_cache->transformed_vertices;
    state->vertex_valid_mask = vertex_cache->transformed_valid_mask;
    state->input_vertices = vertex_cache->input_vertices;
    state->input_vertex_valid_mask = vertex_cache->input_valid_mask;
    state->raw_vertex_fit_mask = vertex_cache->raw_vertex_fit_mask;
    state->vertex_colors = vertex_cache->vertex_colors;
    state->vertex_color_valid_mask =
        vertex_cache->vertex_color_valid_mask;
    state->vertex_matrix_snapshot = vertex_cache->vertex_matrix_snapshot;
    state->vertex_clip_snapshot = vertex_cache->vertex_clip_snapshot;
    sNdsNativeFighterOwnerExecution.stats = stats;
    sNdsNativeFighterOwnerExecution.vertex_cache = vertex_cache;
    sNdsNativeFighterOwnerExecution.slot = slot;
    sNdsNativeFighterOwnerExecution.active = TRUE;
    return TRUE;
#else
    (void)slot;
    (void)stats;
    (void)vertex_cache;
    return FALSE;
#endif
}

s32 ndsRendererEndNativeFighterOwner(
    u32 slot,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    NDSRendererTraversalState *state;
    NDSRendererVertexCache *owner_vertex_cache;
    s32 identity_matches;

    if (sNdsNativeFighterOwnerExecution.active == 0u)
    {
        ndsRendererClearNativeFighterOwner();
        return FALSE;
    }
    identity_matches =
        ((sNdsNativeFighterOwnerExecution.slot == slot) &&
         (sNdsNativeFighterOwnerExecution.stats == stats) &&
         (sNdsNativeFighterOwnerExecution.vertex_cache == vertex_cache)) ?
            TRUE : FALSE;
    owner_vertex_cache =
        sNdsNativeFighterOwnerExecution.vertex_cache;
    if ((identity_matches == FALSE) || (owner_vertex_cache == NULL))
    {
        /* An identity mismatch is a caller integrity failure, but it must not
         * strand the singleton owner and poison the next fighter draw. */
        ndsRendererClearNativeFighterOwner();
        return FALSE;
    }
    state = &sNdsNativeFighterOwnerExecution.traversal;
    owner_vertex_cache->transformed_valid_mask = state->vertex_valid_mask;
    owner_vertex_cache->input_valid_mask = state->input_vertex_valid_mask;
    owner_vertex_cache->raw_vertex_fit_mask = state->raw_vertex_fit_mask;
    owner_vertex_cache->vertex_color_valid_mask =
        state->vertex_color_valid_mask;
    owner_vertex_cache->matrix_snapshot_count =
        state->matrix_snapshot_count;
    ndsRendererClearNativeFighterOwner();
    return TRUE;
#else
    (void)slot;
    (void)stats;
    (void)vertex_cache;
    return FALSE;
#endif
}

void ndsRendererAbortNativeFighterOwner(void)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    ndsRendererClearNativeFighterOwner();
#endif
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static s32 ndsRendererNativeArraySpanFits(
    u32 first, u32 count, u32 total)
{
    return ((first <= total) && (count <= (total - first))) ?
        TRUE : FALSE;
}

static s32 ndsRendererNativeAssetSpanFits(
    u32 offset, u32 count, u32 element_size, u32 asset_data_size)
{
    if ((element_size == 0u) || (offset > asset_data_size))
    {
        return FALSE;
    }
    return (count <= ((asset_data_size - offset) / element_size)) ?
        TRUE : FALSE;
}

static s32 ndsRendererValidateNativeStateSpan(
    const NDSNativeFighterRuntimeTables *tables,
    u16 first, u32 count, u32 asset_data_size)
{
    u32 i;

    if (tables == NULL)
    {
        return FALSE;
    }
    if (count == 0u)
    {
        return TRUE;
    }
    if ((first == NDS_NATIVE_STATE_NONE) ||
        (ndsRendererNativeArraySpanFits(
             first, count, tables->state_sequence_count) == FALSE))
    {
        return FALSE;
    }
    for (i = 0u; i < count; i++)
    {
        u32 delta_index = tables->state_sequence[first + i];
        const NDSNativeStateDelta *delta;

        if (delta_index >= tables->state_delta_count)
        {
            return FALSE;
        }
        delta = &tables->state_deltas[delta_index];
        switch (delta->effect)
        {
        case NDS_NATIVE_STATE_OTHERMODE:
        case NDS_NATIVE_STATE_COMBINE:
        case NDS_NATIVE_STATE_TEXTURE:
        case NDS_NATIVE_STATE_GEOMETRY:
        case NDS_NATIVE_STATE_TILE:
        case NDS_NATIVE_STATE_LOAD_TLUT:
        case NDS_NATIVE_STATE_LOAD_BLOCK:
        case NDS_NATIVE_STATE_TILE_SIZE:
        case NDS_NATIVE_STATE_PRIM:
        /* P2-3f5: G_SETBLENDCOLOR carries no index and no asset offset, so
         * there is nothing to bound-check -- but it still has to be admitted
         * here or the whole span is rejected and the owner falls back. */
        case NDS_NATIVE_STATE_BLEND:
            break;
        case NDS_NATIVE_STATE_IMAGE:
            if (delta->w1 >= asset_data_size)
            {
                return FALSE;
            }
            break;
        case NDS_NATIVE_STATE_LIGHT_COLOR:
        {
            u32 index =
                (delta->w0 >> NDS_RENDERER_MOVEWORD_INDEX_SHIFT) &
                NDS_RENDERER_MOVEWORD_INDEX_MASK;
            u32 offset = delta->w0 & NDS_RENDERER_MOVEWORD_OFFSET_MASK;

            if (((delta->w0 >> 24) != NDS_RENDERER_OP_MOVEWORD) ||
                (index != NDS_RENDERER_MOVEWORD_LIGHTCOL) ||
                ((offset != NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_A) &&
                 (offset != NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_B) &&
                 (offset != NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_A) &&
                 (offset != NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_B)))
            {
                return FALSE;
            }
            break;
        }
        default:
            return FALSE;
        }
    }
    return TRUE;
}

static s32 ndsRendererValidateNativeVertexAction(
    const NDSNativeFighterRuntimeTables *tables,
    u32 action_index,
    const NDSNativeRoot *root,
    u32 asset_data_size)
{
    const NDSNativeVertexAction *action;
    u32 dense_first;
    u32 i;

    if ((tables == NULL) || (root == NULL) ||
        (action_index >= tables->vertex_action_count))
    {
        return FALSE;
    }
    action = &tables->vertex_actions[action_index];
    /* P2-2: the first dense id is already baked into the low bits of the span
     * consumed by production shading. The historical action_dense_first table
     * duplicated that value solely for validation; validate the executable
     * representation itself. */
    dense_first =
        tables->action_dense_spans[action_index] & NDS_NATIVE_DENSE_ID_MASK;
    if (action->command_index >= root->source_command_count)
    {
        return FALSE;
    }
    if (action->kind == NDS_NATIVE_VERTEX_BLOCK)
    {
        if ((action->count == 0u) ||
            ((u32)action->index > NDS_RENDERER_MAX_VTX) ||
            ((u32)action->count >
             (NDS_RENDERER_MAX_VTX - (u32)action->index)) ||
            (ndsRendererNativeAssetSpanFits(
                 action->source_offset, action->count, 16u,
                 asset_data_size) == FALSE)
            ||
            (ndsRendererNativeArraySpanFits(
                 dense_first, action->count,
                 tables->dense_count) == FALSE))
        {
            return FALSE;
        }
        for (i = 0u; i < action->count; i++)
        {
            const NDSNativeDenseVertex *dense =
                &tables->dense_vertices[dense_first + i];

            if ((dense->cache_slot != ((u32)action->index + i)) ||
                (dense->matrix_binding >=
                 NDS_NATIVE_ROOT_BINDING_COUNT))
            {
                return FALSE;
            }
        }
        return TRUE;
    }
    if (action->kind == NDS_NATIVE_MODIFY_ST)
    {
        if ((action->index >= NDS_RENDERER_MAX_VTX) ||
            (dense_first >= tables->dense_count) ||
            (tables->dense_vertices[dense_first].cache_slot !=
             action->index) ||
            (tables->dense_vertices[dense_first].matrix_binding >=
             NDS_NATIVE_ROOT_BINDING_COUNT))
        {
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

static s32 ndsRendererValidateNativeRun(
    const NDSNativeFighterRuntimeTables *tables,
    u32 run_index,
    const NDSNativeRoot *root,
    u32 *source_command_index,
    u32 *tri2_half)
{
    const NDSNativeRun *run;
    u32 first_corner;
    u32 corner_count;
    u32 i;

    if ((tables == NULL) || (root == NULL) ||
        (source_command_index == NULL) || (tri2_half == NULL) ||
        (run_index >= tables->run_count) ||
        (run_index >= tables->run_first_corner_count))
    {
        return FALSE;
    }
    run = &tables->runs[run_index];
    corner_count = (u32)run->triangle_count * 3u;
    first_corner = tables->run_first_corner[run_index];
    if ((run->triangle_count == 0u) ||
        (run->submit_class > NDS_NATIVE_RUN_CROSS_MATRIX) ||
        (ndsRendererNativeArraySpanFits(
             run->first_triangle, run->triangle_count,
             tables->triangle_count) == FALSE) ||
        (ndsRendererNativeArraySpanFits(
             first_corner, corner_count, tables->packed_corner_count) == FALSE))
    {
        return FALSE;
    }
    for (i = 0u; i < corner_count; i++)
    {
        /* Production consumes packed_corners. Its low bits are exactly the
         * dense id formerly mirrored in validation-only dense_corners. */
        u32 dense_id =
            tables->packed_corners[first_corner + i] & NDS_NATIVE_DENSE_ID_MASK;

        if ((dense_id >= tables->dense_count) ||
            (tables->dense_vertices[dense_id].cache_slot >=
             NDS_RENDERER_MAX_VTX) ||
            (tables->dense_vertices[dense_id].matrix_binding >=
             NDS_NATIVE_ROOT_BINDING_COUNT))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < run->triangle_count; i++)
    {
        u32 encoded = tables->triangles[run->first_triangle + i];
        u32 compact = encoded & 0x7fffu;
        u32 required =
            (1u << ((compact >> 10) & 31u)) |
            (1u << ((compact >> 5) & 31u)) |
            (1u << (compact & 31u));

        if ((*source_command_index >= root->source_command_count) ||
            ((run->required_mask & required) != required))
        {
            return FALSE;
        }
        if ((encoded & 0x8000u) != 0u)
        {
            if (*tri2_half != 0u)
            {
                return FALSE;
            }
            *tri2_half = 1u;
        }
        else
        {
            (*source_command_index)++;
            *tri2_half = 0u;
        }
    }
    return TRUE;
}
#endif

#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER && NDS_RENDERER_HW_TRIANGLES
void ndsRendererProfileCensusNativeFighterSchedule(
    u32 slot,
    const u8 *joint_parents,
    const u8 *joint_bindings,
    u32 joint_count,
    u32 binding_count,
    u32 *schedule_match_count,
    u32 *binding_match_count)
{
    const u16 *schedule;
    const u8 *binding_joints;
    u32 expected_joint_count;
    u32 expected_binding_count;
    u32 joint_index;
    u32 binding_index;
    u32 schedule_matches = 0u;
    u32 binding_matches = 0u;

    if ((schedule_match_count == NULL) || (binding_match_count == NULL))
    {
        return;
    }
    *schedule_match_count = 0u;
    *binding_match_count = 0u;
    if ((slot > 1u) || (joint_parents == NULL) ||
        (joint_bindings == NULL))
    {
        return;
    }
    if (slot == 0u)
    {
        schedule = sNdsNativeMarioJointSchedule;
        binding_joints = sNdsNativeMarioBindingJoints;
        expected_joint_count = sizeof(sNdsNativeMarioJointSchedule) /
            sizeof(sNdsNativeMarioJointSchedule[0]);
        expected_binding_count = sizeof(sNdsNativeMarioBindingJoints) /
            sizeof(sNdsNativeMarioBindingJoints[0]);
    }
    else
    {
        schedule = sNdsNativeFoxJointSchedule;
        binding_joints = sNdsNativeFoxBindingJoints;
        expected_joint_count = sizeof(sNdsNativeFoxJointSchedule) /
            sizeof(sNdsNativeFoxJointSchedule[0]);
        expected_binding_count = sizeof(sNdsNativeFoxBindingJoints) /
            sizeof(sNdsNativeFoxBindingJoints[0]);
    }
    for (joint_index = 0u;
         (joint_index < joint_count) &&
         (joint_index < expected_joint_count);
         joint_index++)
    {
        u32 expected = schedule[joint_index];

        if (((expected & 31u) == joint_parents[joint_index]) &&
            (((expected >> 5) & 31u) == joint_bindings[joint_index]))
        {
            schedule_matches++;
        }
    }
    for (binding_index = 0u;
         (binding_index < binding_count) &&
         (binding_index < expected_binding_count);
         binding_index++)
    {
        u32 joint = binding_joints[binding_index];

        if ((joint < joint_count) &&
            (joint_bindings[joint] == binding_index))
        {
            binding_matches++;
        }
    }
    *schedule_match_count = schedule_matches;
    *binding_match_count = binding_matches;
}
#endif

#if NDS_TICK_HUD
/* Focused native-owner admission diagnostic.  A validation decline otherwise
 * collapses to one Task-68 `Validate` bucket, which cannot distinguish a live
 * model-part root miss from a malformed generated span/material contract.
 * Shipping builds compile this out completely. */
volatile u32 gNdsNativeFighterValidateRejectCode;
volatile u32 gNdsNativeFighterValidateRejectSlot;
volatile u32 gNdsNativeFighterValidateRejectLow;
volatile u32 gNdsNativeFighterValidateRejectRoot;
volatile u32 gNdsNativeFighterValidateRejectObserved;
volatile u32 gNdsNativeFighterValidateRejectExpected;
#define NDS_NATIVE_FIGHTER_VALIDATE_REJECT(code_, root_, observed_, expected_) \
    do { \
        gNdsNativeFighterValidateRejectCode = (code_); \
        gNdsNativeFighterValidateRejectSlot = slot; \
        gNdsNativeFighterValidateRejectLow = use_low_detail; \
        gNdsNativeFighterValidateRejectRoot = (root_); \
        gNdsNativeFighterValidateRejectObserved = (observed_); \
        gNdsNativeFighterValidateRejectExpected = (expected_); \
        return FALSE; \
    } while (0)
#else
#define NDS_NATIVE_FIGHTER_VALIDATE_REJECT(code_, root_, observed_, expected_) \
    do { \
        (void)(code_); (void)(root_); (void)(observed_); (void)(expected_); \
        return FALSE; \
    } while (0)
#endif

s32 ndsRendererValidateNativeFighterOwner(
    u32 slot,
    u32 use_low_detail,
    u32 asset_data_size,
    u32 root_count,
    const u32 *root_offsets,
    const u32 *material_counts)
{
#if NDS_RENDERER_HW_TRIANGLES
    const NDSNativeRoot *roots;
    const NDSNativeEpoch *epochs;
    u32 expected_count;
    u32 expected_asset_data_size;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    const NDSNativeFighterOwnerRuntime *owner;
    const NDSNativeFighterRuntimeTables *tables;
    u32 epoch_count;
    u32 action_count;
    u32 run_count;
#endif
    u32 root_index;

    if ((root_offsets == NULL) || (material_counts == NULL))
    {
        NDS_NATIVE_FIGHTER_VALIDATE_REJECT(1u, 0xffffffffu,
            (u32)(uintptr_t)root_offsets, (u32)(uintptr_t)material_counts);
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    owner = ndsRendererNativeFighterOwnerForDetail(slot, use_low_detail);
    if (owner == NULL)
    {
        NDS_NATIVE_FIGHTER_VALIDATE_REJECT(2u, 0xffffffffu, slot,
            use_low_detail);
    }
    tables = owner->tables;
    epoch_count = tables->epoch_count;
    action_count = tables->vertex_action_count;
    run_count = tables->run_count;
    expected_asset_data_size = owner->asset_data_size;
    roots = owner->roots;
    expected_count = owner->root_count;
    epochs = tables->epochs;
#else
    /* Semantic/profile builds retain the qualified Mario/Fox oracle. P2-3's
     * Luigi owner is a production-only admission until its own semantic lab is
     * explicitly added; never alias it onto Fox's tables. */
    if (slot > 1u)
    {
        NDS_NATIVE_FIGHTER_VALIDATE_REJECT(2u, 0xffffffffu, slot, 1u);
    }
    if (slot == 0u)
    {
        expected_asset_data_size = 0x7510u;
    }
    else
    {
        expected_asset_data_size = 0x7e50u;
    }
    if (use_low_detail != 0u)
    {
        roots = (slot == 0u) ? sNdsNativeMarioRootsLow : sNdsNativeFoxRootsLow;
        expected_count = (slot == 0u) ?
            (u32)(sizeof(sNdsNativeMarioRootsLow) /
                  sizeof(sNdsNativeMarioRootsLow[0])) :
            (u32)(sizeof(sNdsNativeFoxRootsLow) /
                  sizeof(sNdsNativeFoxRootsLow[0]));
        epochs = sNdsNativeFighterEpochsLow;
    }
    else
    {
        roots = (slot == 0u) ? sNdsNativeMarioRoots : sNdsNativeFoxRoots;
        expected_count = (slot == 0u) ?
            (u32)(sizeof(sNdsNativeMarioRoots) /
                  sizeof(sNdsNativeMarioRoots[0])) :
            (u32)(sizeof(sNdsNativeFoxRoots) /
                  sizeof(sNdsNativeFoxRoots[0]));
        epochs = sNdsNativeFighterEpochs;
    }
#endif
    if ((asset_data_size != expected_asset_data_size) ||
        (root_count != expected_count))
    {
        NDS_NATIVE_FIGHTER_VALIDATE_REJECT(3u, 0xffffffffu,
            (asset_data_size != expected_asset_data_size) ? asset_data_size : root_count,
            (asset_data_size != expected_asset_data_size) ? expected_asset_data_size : expected_count);
    }
    for (root_index = 0u; root_index < root_count; root_index++)
    {
        const NDSNativeRoot *root;
        u32 epoch_index;

#if NDS_RENDERER_PROFILE_LEVEL < 2
        root = ndsRendererNativeFighterResolveRoot(
            owner, slot, use_low_detail, root_index,
            root_offsets[root_index]);
        if (root == NULL)
        {
            NDS_NATIVE_FIGHTER_VALIDATE_REJECT(4u, root_index,
                root_offsets[root_index], roots[root_index].root_offset);
        }
#else
        root = &roots[root_index];
        if (root->root_offset != root_offsets[root_index])
        {
            NDS_NATIVE_FIGHTER_VALIDATE_REJECT(4u, root_index,
                root_offsets[root_index], root->root_offset);
        }
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((u32)root->light_preamble >= owner->root_light_preamble_count)
        {
            NDS_NATIVE_FIGHTER_VALIDATE_REJECT(5u, root_index,
                (u32)root->light_preamble, owner->root_light_preamble_count);
        }
        if (
            (ndsRendererNativeAssetSpanFits(
                 root->root_offset, root->source_command_count,
                 sizeof(Gfx), asset_data_size) == FALSE) ||
            (ndsRendererNativeArraySpanFits(
                 root->first_epoch, root->epoch_count,
                 epoch_count) == FALSE) ||
            (ndsRendererValidateNativeStateSpan(
                 tables,
                 root->tail_state_first, root->tail_state_count,
                 asset_data_size) == FALSE))
        {
            NDS_NATIVE_FIGHTER_VALIDATE_REJECT(6u, root_index,
                root->root_offset, root->source_command_count);
        }
#endif
        for (epoch_index = 0u;
             epoch_index < root->epoch_count;
             epoch_index++)
        {
            const NDSNativeEpoch *epoch =
                &epochs[root->first_epoch + epoch_index];
#if NDS_RENDERER_PROFILE_LEVEL < 2
            u32 action_index;
            u32 run_index;
            u32 source_command_index =
                epoch->first_triangle_command_index;
            u32 tri2_half = 0u;
#endif

            if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
                (epoch->material_slot >= material_counts[root_index]))
            {
                NDS_NATIVE_FIGHTER_VALIDATE_REJECT(7u, root_index,
                    material_counts[root_index], epoch->material_slot);
            }
#if NDS_RENDERER_PROFILE_LEVEL < 2
            if ((ndsRendererValidateNativeStateSpan(
                     tables,
                     epoch->before_state_first,
                     epoch->before_state_count,
                     asset_data_size) == FALSE) ||
                (ndsRendererValidateNativeStateSpan(
                     tables,
                     epoch->after_state_first,
                     epoch->after_state_count,
                     asset_data_size) == FALSE) ||
                (ndsRendererNativeArraySpanFits(
                     epoch->first_action, epoch->action_count,
                     action_count) == FALSE) ||
                (ndsRendererNativeArraySpanFits(
                     epoch->first_run, epoch->run_count,
                     run_count) == FALSE))
            {
                NDS_NATIVE_FIGHTER_VALIDATE_REJECT(8u, root_index,
                    root->first_epoch + epoch_index, root->root_offset);
            }
            for (action_index = 0u;
                 action_index < epoch->action_count;
                 action_index++)
            {
                if (ndsRendererValidateNativeVertexAction(
                        tables,
                        (u32)epoch->first_action + action_index,
                        root, asset_data_size) == FALSE)
                {
                    NDS_NATIVE_FIGHTER_VALIDATE_REJECT(9u, root_index,
                        (u32)epoch->first_action + action_index, root->root_offset);
                }
            }
            for (run_index = 0u;
                 run_index < epoch->run_count;
                 run_index++)
            {
                if (ndsRendererValidateNativeRun(
                        tables,
                        (u32)epoch->first_run + run_index,
                        root, &source_command_index,
                        &tri2_half) == FALSE)
                {
                    NDS_NATIVE_FIGHTER_VALIDATE_REJECT(10u, root_index,
                        (u32)epoch->first_run + run_index, root->root_offset);
                }
            }
            if (tri2_half != 0u)
            {
                NDS_NATIVE_FIGHTER_VALIDATE_REJECT(11u, root_index,
                    tri2_half, root->root_offset);
            }
#endif
        }
    }
    return TRUE;
#else
    (void)slot;
    (void)use_low_detail;
    (void)asset_data_size;
    (void)root_count;
    (void)root_offsets;
    (void)material_counts;
    return FALSE;
#endif
}

#undef NDS_NATIVE_FIGHTER_VALIDATE_REJECT

s32 ndsRendererExecuteNativeFighterRoot(
    u32 slot,
    u32 root_ordinal,
    const void *asset_base,
    u32 root_offset,
    const NDSRendererNativeMaterial *materials,
    u32 material_count,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
#if NDS_RENDERER_HW_TRIANGLES
    return ndsRendererExecuteNativeFighterRootHardware(
        slot, root_ordinal, asset_base, root_offset,
        materials, material_count, config,
        callback, callback_user, stats, vertex_cache);
#else
    (void)slot;
    (void)root_ordinal;
    (void)asset_base;
    (void)root_offset;
    (void)materials;
    (void)material_count;
    (void)config;
    (void)callback;
    (void)callback_user;
    (void)stats;
    (void)vertex_cache;
    return FALSE;
#endif
}

static void NDS_R2_CENSUS_EVICTED_CODE
ndsRendererScanList(const Gfx *dl,
                    const NDSRendererConfig *config,
                    NDSRendererStats *stats,
                    NDSRendererTraversalState *state,
                    u32 depth,
                    NDSRendererCommandCallback callback,
                    void *callback_user);

enum
{
    NDS_RENDERER_SCAN_COLD_PROCEED = 0,
    NDS_RENDERER_SCAN_COLD_CONTINUE = 1,
    NDS_RENDERER_SCAN_COLD_RETURN = 2
};

/* The generic command record is needed only when a command callback is active
 * or when a DL opcode must be resolved.  The canonical profile-0 battle does
 * neither in this scanner, so keep that record/branch machinery in main RAM
 * without changing its ordering or semantics for generic callers. */
static u32 __attribute__((noinline, cold))
ndsRendererScanColdCommand(const Gfx *dl,
                           const NDSRendererConfig *config,
                           NDSRendererStats *stats,
                           NDSRendererTraversalState *state,
                           u32 depth,
                           u32 list_index,
                           u32 w0,
                           u32 w1,
                           u32 op,
                           NDSRendererCommandCallback callback,
                           void *callback_user)
{
    NDSRendererCommand command;

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

    if (op == NDS_RENDERER_OP_DL)
    {
        command.raw_branch_dl = (const Gfx *)(uintptr_t)w1;
        command.resolved_branch_dl = command.raw_branch_dl;
        if (config->resolve_branch != NULL)
        {
            command.resolved_branch_dl = config->resolve_branch(
                command.raw_branch_dl,
                &command.branch_resolve_kind,
                config->user);
        }
        command.branch_is_jump =
            ((w0 & (1u << 16)) != 0) ? TRUE : FALSE;
    }

    if (stats->first_opcode == 0)
    {
        stats->first_opcode = op;
    }
    stats->command_count++;

    if ((callback != NULL) &&
        (callback(&command, callback_user) == FALSE))
    {
        ndsRendererRecordUnsupported(stats, op);
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return NDS_RENDERER_SCAN_COLD_RETURN;
    }
    if (op != NDS_RENDERER_OP_DL)
    {
        return NDS_RENDERER_SCAN_COLD_PROCEED;
    }

    {
        const Gfx *raw_branch = command.raw_branch_dl;
        const Gfx *branch = command.resolved_branch_dl;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        u32 parent_branch_path = state->semantic_branch_path;
        u32 child_branch_path;
#endif

        stats->branch_command_count++;
        if (stats->first_branch_dl == NULL)
        {
            stats->first_branch_dl = raw_branch;
        }
        if (stats->first_resolved_branch_dl == NULL)
        {
            stats->first_resolved_branch_dl = branch;
        }
        if (command.branch_resolve_kind == NDS_RENDERER_RESOLVE_SEGMENT)
        {
            stats->segment_resolve_count++;
        }
        if (ndsRendererValidateCommand(branch, config) == FALSE)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
            return NDS_RENDERER_SCAN_COLD_RETURN;
        }
        if ((w0 & (1u << 16)) != 0)
        {
            stats->branch_jump_count++;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
            child_branch_path = ndsRendererSemanticBranchPath(
                parent_branch_path, list_index, depth + 1u, TRUE);
            state->semantic_branch_path = child_branch_path;
#endif
            ndsRendererScanList(branch, config, stats, state, depth + 1u,
                                callback, callback_user);
            return NDS_RENDERER_SCAN_COLD_RETURN;
        }

        stats->branch_call_count++;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        child_branch_path = ndsRendererSemanticBranchPath(
            parent_branch_path, list_index, depth + 1u, FALSE);
        state->semantic_branch_path = child_branch_path;
#endif
        ndsRendererScanList(branch, config, stats, state, depth + 1u,
                            callback, callback_user);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        state->semantic_branch_path = parent_branch_path;
#endif
        return (stats->blocker != NDS_RENDERER_BLOCKER_NONE) ?
            NDS_RENDERER_SCAN_COLD_RETURN : NDS_RENDERER_SCAN_COLD_CONTINUE;
    }
}

/* These generic state opcodes are present for compatibility, but their
 * ScanList arms have zero PCs in the canonical whole-match census.  MOVEWORD
 * itself remains hot for its other callers; only this cold scanner dispatch is
 * moved out of ITCM. */
static void __attribute__((noinline, cold))
ndsRendererScanColdStateOpcode(const NDSRendererConfig *config,
                               NDSRendererStats *stats,
                               NDSRendererTraversalState *state,
                               u32 op, u32 w0, u32 w1)
{
    switch (op)
    {
    case NDS_RENDERER_OP_MTX:
        ndsRendererApplyMatrixCommand(config, stats, state, w0, w1);
        break;
    case NDS_RENDERER_OP_POPMTX:
        ndsRendererApplyPopMatrixCommand(stats, state, w1);
        break;
    case NDS_RENDERER_OP_MOVEWORD:
        ndsRendererApplyMatrixMoveWordCommand(stats, state, w0, w1);
        break;
    case NDS_RENDERER_OP_MOVEMEM:
        ndsRendererRecordLightMoveMem(config, stats, w0, w1);
        NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state);
        break;
    case NDS_RENDERER_OP_SPECIAL_1:
        ndsRendererApplyMvpRecalcCommand(stats, state, w0, w1);
        break;
    case NDS_RENDERER_OP_SETSCISSOR:
    case NDS_RENDERER_OP_SETCIMG:
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
        stats->ignored_state_command_count++;
        break;
    case NDS_RENDERER_OP_SETPRIMDEPTH:
        ndsRendererRecordPrimDepth(stats, w1);
        break;
    default:
        break;
    }
}

static void NDS_R2_CENSUS_EVICTED_CODE
ndsRendererScanList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats,
                                NDSRendererTraversalState *state,
                                u32 depth,
                                NDSRendererCommandCallback callback,
                                void *callback_user)
{
    u32 i;
    u32 immutable_command_count = 0u;

    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (depth > config->max_depth)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_TOO_DEEP;
        return;
    }
    if (ndsRendererValidateCommand(dl, config) == FALSE)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }
    if (depth > stats->max_depth_seen)
    {
        stats->max_depth_seen = depth;
    }
    if (config->immutable_command_span != NULL)
    {
        size_t immutable_bytes =
            config->immutable_command_span(dl, config->user);
        size_t immutable_count = immutable_bytes / sizeof(*dl);

        if (immutable_count > config->max_list_commands)
        {
            immutable_count = config->max_list_commands;
        }
        immutable_command_count = (u32)immutable_count;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (immutable_command_count != 0u)
        {
            sNdsRendererProfileImmutableListCount++;
        }
#endif
    }

    for (i = 0; i < config->max_list_commands; i++, dl++)
    {
        u32 w0;
        u32 w1;
        u32 op;

        if ((i >= immutable_command_count) &&
            (ndsRendererValidateCommand(dl, config) == FALSE))
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
            return;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (i < immutable_command_count)
        {
            sNdsRendererProfileTrustedCommandCount++;
        }
#endif
        if (stats->command_count >= config->max_commands)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
            return;
        }

        w0 = dl->words.w0;
        w1 = dl->words.w1;
        op = w0 >> 24;
#if NDS_RENDERER_HW_TRIANGLES
        state->source_command_site = dl;
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        state->semantic_command_index = i;
        state->semantic_tri2_half = 0u;
#endif
#if NDS_RENDERER_HW_TRIANGLES
        /* Preserve the source-command boundary: only adjacent TRI1/TRI2
         * opcodes may share a GX triangle group. In particular, close before
         * VTX/MODIFYVTX mutate the cached vertices and before any matrix,
         * texture, state, branch, sync, or ENDDL command. */
        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            ndsRendererHardwareEndBatch();
            /* VTX and matrix commands end the GX primitive group but cannot
             * change the prepared texture/material/depth epoch. The exact
             * state opcodes below invalidate that epoch at their mutation. */
            state->prepared_vertex_color_valid_mask = 0u;
            state->prepared_texcoord_valid_mask = 0u;
            state->prepared_projected_xy_valid_mask = 0u;
            state->prepared_projected_source_z_valid_mask = 0u;
        }
#endif
        if ((callback != NULL) || (op == NDS_RENDERER_OP_DL))
        {
            u32 cold_action = ndsRendererScanColdCommand(
                dl, config, stats, state, depth, i, w0, w1, op,
                callback, callback_user);

            if (cold_action == NDS_RENDERER_SCAN_COLD_RETURN)
            {
                return;
            }
            if (cold_action == NDS_RENDERER_SCAN_COLD_CONTINUE)
            {
                continue;
            }
        }
        else
        {
            if (stats->first_opcode == 0)
            {
                stats->first_opcode = op;
            }
            stats->command_count++;
        }

        switch (op)
        {
        case NDS_RENDERER_OP_NOOP:
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            break;

        case NDS_RENDERER_OP_MODIFYVTX:
            ndsRendererApplyModifyVertexCommand(stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_VTX:
            NDS_EFFECT_PHASE_VTX(
                ndsRendererApplyVertexCommand(config, stats, state, w0, w1));
            break;

        case NDS_RENDERER_OP_TRI1:
        case NDS_RENDERER_OP_TRI2:
        {
            ndsRendererExecuteTriangleCommand(
                stats, config, state, op, w0, w1);
#if NDS_RENDERER_HW_TRIANGLES
            /* Immutable adjacent TRI commands have no intervening source
             * state transition. Profile 0/1 has no command callback, so
             * replay the remainder of the run without rebuilding a generic
             * command record or re-entering the full opcode switch. */
#if NDS_RENDERER_PROFILE_LEVEL < 2
            if ((callback == NULL) && (sNdsRendererFastOwnerEnabled != 0u))
#else
            if (sNdsRendererFastOwnerEnabled != 0u)
#endif
            {
                ndsRendererExecuteFastRawCurrentRun(
                    &dl, &i, immutable_command_count,
                    config, stats, state, depth, callback, callback_user);
            }
#if NDS_RENDERER_PROFILE_LEVEL < 2
            else while ((callback == NULL) &&
                   ((i + 1u) < immutable_command_count) &&
                   ((i + 1u) < config->max_list_commands))
            {
                const Gfx *next_dl = dl + 1;
                u32 next_w0 = next_dl->words.w0;
                u32 next_op = next_w0 >> 24;

                if ((next_op != NDS_RENDERER_OP_TRI1) &&
                    (next_op != NDS_RENDERER_OP_TRI2))
                {
                    break;
                }
                if (stats->command_count >= config->max_commands)
                {
                    stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
                    return;
                }
                i++;
                dl = next_dl;
                stats->command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                sNdsRendererProfileTrustedCommandCount++;
                sNdsRendererProfileTriangleRunReuseCount++;
#endif
                ndsRendererExecuteTriangleCommand(
                    stats, config, state, next_op, next_w0,
                    next_dl->words.w1);
            }
#endif
#endif
            break;
        }

        case NDS_RENDERER_OP_ENDDL:
            stats->end_command_count++;
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            return;

        case NDS_RENDERER_OP_DL:
            /* Handled by ndsRendererScanColdCommand above. */
            break;

        case NDS_RENDERER_OP_TEXTURE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordTextureState(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_MTX:
        case NDS_RENDERER_OP_POPMTX:
        case NDS_RENDERER_OP_MOVEWORD:
        case NDS_RENDERER_OP_MOVEMEM:
        case NDS_RENDERER_OP_SPECIAL_1:
        case NDS_RENDERER_OP_SETSCISSOR:
        case NDS_RENDERER_OP_SETCIMG:
        case NDS_RENDERER_OP_SETPRIMDEPTH:
            ndsRendererScanColdStateOpcode(config, stats, state, op, w0, w1);
            break;

        case NDS_RENDERER_OP_GEOMETRYMODE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            stats->geometry_mode = (stats->geometry_mode & w0) | w1;
            stats->geometry_clear_mask = w0;
            stats->geometry_set_mask = w1;
            stats->geometry_command_count++;
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETCOMBINE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetCombine(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTIMG:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetImage(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTILE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetTile(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADTILE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadTile(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADBLOCK:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadBlock(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADTLUT:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadTlut(stats, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTILESIZE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetTileSize(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETFOGCOLOR:
            ndsRendererRecordFogColor(stats, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            stats->color_command_count++;
            break;

        case NDS_RENDERER_OP_SETBLENDCOLOR:
        case NDS_RENDERER_OP_SETENVCOLOR:
        case NDS_RENDERER_OP_SETPRIMCOLOR:
            if (op != NDS_RENDERER_OP_SETBLENDCOLOR)
            {
                NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            }
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            stats->color_command_count++;
            if (op == NDS_RENDERER_OP_SETPRIMCOLOR)
            {
                stats->prim_color = w1;
                stats->prim_min_level = (w0 >> 8) & 0xffu;
                stats->prim_lod_fraction = w0 & 0xffu;
            }
            else if (op == NDS_RENDERER_OP_SETENVCOLOR)
            {
                stats->env_color = w1;
            }
            else
            {
                stats->blend_color = w1;
            }
            break;

        case NDS_RENDERER_OP_RDPPIPESYNC:
        case NDS_RENDERER_OP_RDPLOADSYNC:
        case NDS_RENDERER_OP_RDPTILESYNC:
        case NDS_RENDERER_OP_RDPFULLSYNC:
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            stats->sync_command_count++;
            break;

        case NDS_RENDERER_OP_SETOTHERMODE_H:
        case NDS_RENDERER_OP_SETOTHERMODE_L:
        case NDS_RENDERER_OP_RDPSETOTHERMODE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordOtherMode(stats, op, w0, w1);
            break;

        case NDS_RENDERER_OP_CULLDL:
            ndsRendererRecordCull(stats, w0, w1);
            break;

        default:
            ndsRendererRecordUnsupported(stats, op);
            break;
        }
    }
}
