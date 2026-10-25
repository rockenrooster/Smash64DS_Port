/* The production fighter owner's executor: a generated owner record's runs
 * (prepare, shade, emit) validated against their spans and run from ITCM;
 * a declined run is a correctness failure, torn down exactly but kept out
 * of ITCM. The fighter's colour animation (a hurt flash, the challenger's
 * black) modulates every root once a draw. A slice of nds_renderer.c. */

#if defined(__arm__)
#define NDS_RENDERER_NATIVE_FIGHTER_FAIL_CODE \
    __attribute__((noinline, cold, optimize("Os"), target("arm")))
#else
#define NDS_RENDERER_NATIVE_FIGHTER_FAIL_CODE \
    __attribute__((noinline, cold, optimize("Os")))
#endif

/* A production-run decline is a correctness failure, not steady-state work.
 * Keep its accounting/teardown exact, but do not rent scarce ITCM for a path
 * that a valid native owner never takes.  This also makes the full P2 roster's
 * 32 KiB ITCM fit independent of which owner tables are compiled in. */
static void NDS_RENDERER_NATIVE_FIGHTER_FAIL_CODE
ndsRendererNativeAbortProductionRun(
    NDSRendererStats *stats,
    u32 raw_triangle_count,
    u32 raw_reuse_count,
    u32 cross_triangle_count,
    u32 cross_reuse_count)
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
}

s32 NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererExecuteNativeFighterOwnerProduction(
    u32 slot,
    u32 use_low_detail,
    u32 texture_memo_owner_key,
    u32 packet_key,
    u32 packet_prechecked,
    const void *asset_base_ptr,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count,
    NDSRendererStats *stats,
    u32 *out_hardware_started)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
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
    if ((stats == NULL) ||
        (stats->blocker != NDS_RENDERER_BLOCKER_NONE))
    {
        return FALSE;
    }
    (void)packet_prechecked;
    if (ndsRendererNativePreflightProductionOwner(
            slot, (texture_memo_owner_key >> 9) & 3u,
            use_low_detail, asset_base, inputs, input_count,
            NULL, stats) == FALSE)
    {
        return FALSE;
    }
    (void)packet_key;
    root_count = sNdsNativeFighterActiveOwner->root_count;
    palette_slots = sNdsNativeFighterActiveOwner->cross_palette_slots;

    ndsRendererInitTraversalState(
        state, NULL, stats, NULL, NULL, 0u);
    /* The fighter's colour animation (a hurt flash, the challenger's black):
     * one modulate per draw, on every root's config. A record names it
     * (tint_modulate), so its shade words are derived at it too. */
    state->color_modulate =
        ((input_count != 0u) && (inputs[0].config != NULL)) ?
            inputs[0].config->color_modulate : 0u;
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
    /* Nothing outside this loop is known to leave GL_PROJECTION alone, so the
     * projection elide only claims what one execute can prove. */
    sNdsR2GxLastProjection = NULL;
    for (root_index = 0u; root_index < root_count; root_index++)
    {
        const NDSRendererNativeFighterRoot *input = &inputs[root_index];
        const u8 *root_asset_base = (input->asset_base != NULL) ?
            (const u8 *)input->asset_base : asset_base;
        const NDSNativeRoot *root =
            sNdsNativeProductionResolvedRoots[root_index];
        u32 palette_slot = palette_slots[root_index];
        u32 epoch_offset;

        /* Cold preflight resolved the live root together with the table image
         * that owns its local indices.  Ordinary roots point at the resident
         * owner; a copied Kirby binding 1 points at the single streamed hat
         * image.  Switch once per root so every existing hot helper keeps its
         * pointer-only ABI and no source-DL work re-enters the frame. */
        sNdsNativeFighterActiveTables =
            sNdsNativeProductionResolvedTables[root_index];
        sNdsNativeFighterActiveRootLightPreambles =
            sNdsNativeProductionResolvedLightPreambles[root_index];
        sNdsNativeFighterActiveRootLightPreambleCount =
            sNdsNativeProductionResolvedLightPreambleCounts[root_index];
        if (sNdsNativeFighterActiveTables->dense_normals != NULL)
        {
            sNdsNativeFighterActiveDenseNormals =
                (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        }

        ndsRendererNativeBindProductionRoot(state, input, stats);
        *out_hardware_started = TRUE;
        /* The capture is allowed to decline, and when it does the adapter has
         * composed on the CPU exactly as before, so the split loader is still
         * the fail-closed answer for this owner. */
        if (input->gx_valid != 0u)
        {
            ndsRendererLoadHardwareGxComposedMatrices(
                input, state->matrix_generation);
        }
        else
        {
            ndsRendererLoadHardwareSplitMatrices(
                input->projection_matrix, input->modelview_matrix,
                state->matrix_generation);
        }
        if (palette_slot <= NDS_NATIVE_GX_MATRIX_SLOT_MAX)
        {
            glStoreMatrix((int)palette_slot);
        }
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
                &sNdsNativeFighterActiveTables->epochs[epoch_index];
            u32 run_offset;

            ndsRendererNativeApplyStateSpan(
                epoch->before_state_first, epoch->before_state_count,
                epoch->before_sync_count,
                root_asset_base, stats, state);
            if (epoch->material_slot != NDS_NATIVE_MATERIAL_NONE)
            {
                ndsRendererNativeApplyMaterial(
                    &input->materials[epoch->material_slot], stats, state);
            }
            ndsRendererNativeApplyStateSpan(
                epoch->after_state_first, epoch->after_state_count,
                epoch->after_sync_count,
                root_asset_base, stats, state);
            (void)ndsRendererNativeShadeProductionActions(
                epoch,
                sNdsNativeFighterActiveTables->epoch_direct_policy[epoch_index],
                FALSE,
                stats, state);

            for (run_offset = 0u;
                 run_offset < epoch->run_count;
                 run_offset++)
            {
                const NDSNativeRun *run =
                    &sNdsNativeFighterActiveTables->runs[
                        epoch->first_run + run_offset];

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
                    ndsRendererNativeAbortProductionRun(
                        stats, raw_triangle_count, raw_reuse_count,
                        cross_triangle_count, cross_reuse_count);
                    return FALSE;
                }
                native_run_count++;
                native_triangle_count += run->triangle_count;
            }
        }
        ndsRendererNativeApplyStateSpan(
            root->tail_state_first, root->tail_state_count,
            root->tail_sync_count,
            root_asset_base, stats, state);
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
    sNdsRendererFastRunCount += native_run_count;
    sNdsRendererFastTriangleCount += native_triangle_count;
    if ((u32)sNdsRendererRuntimeOwner <
        (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererFastOwnerTriangleCount[
            (u32)sNdsRendererRuntimeOwner] += native_triangle_count;
    }
    return TRUE;
}

#undef NDS_RENDERER_NATIVE_FIGHTER_FAIL_CODE

static void ndsRendererClearNativeFighterOwner(void)
{
    ndsRendererHardwareEndBatch();
    sNdsNativeFighterOwnerExecution.stats = NULL;
    sNdsNativeFighterOwnerExecution.vertex_cache = NULL;
    sNdsNativeFighterOwnerExecution.slot = 0u;
    sNdsNativeFighterOwnerExecution.active = FALSE;
}

s32 ndsRendererBeginNativeFighterOwner(
    u32 slot,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
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
}

s32 ndsRendererEndNativeFighterOwner(
    u32 slot,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
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
}

void ndsRendererAbortNativeFighterOwner(void)
{
    ndsRendererClearNativeFighterOwner();
}

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
    dense_first = NDS_NATIVE_SPAN_FIRST(
        NDS_NATIVE_DENSE_WIDE(tables), tables->action_dense_spans[action_index]);
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
        /* A LEAN_TRIM image carries no dense_vertices: nothing in a lean
         * build reads their slots or bindings, so there is none to check. */
        for (i = 0u; (tables->dense_vertices != NULL) && (i < action->count);
             i++)
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
            ((tables->dense_vertices != NULL) &&
             ((tables->dense_vertices[dense_first].cache_slot !=
               action->index) ||
              (tables->dense_vertices[dense_first].matrix_binding >=
               NDS_NATIVE_ROOT_BINDING_COUNT))))
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
        (run_index >= tables->run_count))
    {
        return FALSE;
    }
    run = &tables->runs[run_index];
    corner_count = (u32)run->triangle_count * 3u;
    first_corner = (u32)run->first_triangle * 3u;
    if ((run->triangle_count == 0u) ||
        (((u32)run->submit_class & NDS_NATIVE_RUN_SUBMIT_CLASS_MASK) >
         NDS_NATIVE_RUN_CROSS_MATRIX) ||
        (ndsRendererNativeArraySpanFits(
             run->first_triangle, run->triangle_count,
             tables->triangle_count) == FALSE)
#if NDS_NATIVE_FIGHTER_IMAGE_HAS_PACKED_CORNERS
        ||
        (ndsRendererNativeArraySpanFits(
             first_corner, corner_count, tables->packed_corner_count) == FALSE))
#else
        )
#endif
    {
        return FALSE;
    }
#if NDS_NATIVE_FIGHTER_IMAGE_HAS_PACKED_CORNERS
    for (i = 0u; i < corner_count; i++)
    {
        /* Production consumes packed_corners. Its low bits are exactly the
         * dense id formerly mirrored in validation-only dense_corners; a
         * wide owner's raw-run corner is the whole id. */
        u32 dense_id = tables->packed_corners[first_corner + i];

        if ((NDS_NATIVE_DENSE_WIDE(tables) == FALSE) ||
            ((((u32)run->submit_class & NDS_NATIVE_RUN_SUBMIT_CLASS_MASK) ==
              NDS_NATIVE_RUN_CROSS_MATRIX)))
        {
            dense_id &= NDS_NATIVE_DENSE_ID_MASK;
        }

        if ((dense_id >= tables->dense_count) ||
            ((tables->dense_vertices != NULL) &&
             ((tables->dense_vertices[dense_id].cache_slot >=
               NDS_RENDERER_MAX_VTX) ||
              (tables->dense_vertices[dense_id].matrix_binding >=
               NDS_NATIVE_ROOT_BINDING_COUNT))))
        {
            return FALSE;
        }
    }
#else
    /* The primitive-stream production executes primitive_vertices, not packed_corners.
     * The host primitive-stream checker proves oriented source-triangle
     * equivalence. Runtime still validates every source triangle command below;
     * the duplicate packed stream is intentionally absent from this image. */
    (void)first_corner;
    (void)corner_count;
#endif
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


/* Focused native-owner admission diagnostic.  A validation decline otherwise
 * collapses to one Task-68 `Validate` bucket, which cannot distinguish a live
 * model-part root miss from a malformed generated span/material contract.
 *
 * These six words are compiled into EVERY configuration on purpose. They were
 * gated on NDS_TICK_HUD || NDS_P2_NESS, which meant the shipping and profile0
 * ROMs — the ones the native-only repair queue actually runs — carried no way
 * to say WHY an owner declined, and a gdb script naming them aborted on the
 * missing symbol instead of printing the answer (2026-09-08, the Results
 * Lose-pose probe: `builds/resume-20260907/results-fighter-first-native.txt`
 * is zero bytes for exactly that reason). The cost is 24 B of .bss and six
 * stores on a path that is already returning failure, which is nothing beside
 * a probe that cannot report. */
__attribute__((used)) volatile u32 gNdsNativeFighterValidateRejectCode;
__attribute__((used)) volatile u32 gNdsNativeFighterValidateRejectSlot;
__attribute__((used)) volatile u32 gNdsNativeFighterValidateRejectLow;
__attribute__((used)) volatile u32 gNdsNativeFighterValidateRejectRoot;
__attribute__((used)) volatile u32 gNdsNativeFighterValidateRejectObserved;
__attribute__((used)) volatile u32 gNdsNativeFighterValidateRejectExpected;
/* The six words above prove THAT binding N disagreed; they cannot show a live
 * topology shift, where one joint's dl went NULL and a different in-file root
 * appeared later in the walk. Link declines exactly that way (2026-09-08:
 * binding 5 observed 0x2828, which is the canonical binding-SIX root, while the
 * count stayed at the baked 19). Publish the whole observed vector so one stop
 * names both the joint that dropped out and the root that took its place. */
__attribute__((used)) volatile u32 gNdsNativeFighterValidateRejectCount;
__attribute__((used)) volatile u32
    gNdsNativeFighterValidateRejectOffsets[32];
/* The vector still needs a human to diff it against the ELF, and a vanish plus
 * appear pair keeps root_count equal, so code 3 never fires and code 4 names
 * only the first SHIFTED ordinal -- root 5 for Link, when the source event is
 * at joints 11 and 20. Publish the set difference too: the canonical binding
 * that has no observed offset, and the observed offset that has no canonical
 * binding. Those two words are the whole input a topology owner needs.
 * Quadratic with n <= 32, on a path that is already returning failure. */
__attribute__((used)) volatile u32 gNdsNativeFighterValidateRejectAbsentBinding;
__attribute__((used)) volatile u32 gNdsNativeFighterValidateRejectForeignIndex;
__attribute__((used)) volatile u32 gNdsNativeFighterValidateRejectForeignOffset;
static void ndsNativeFighterPublishRootSetDelta(
    const NDSNativeRoot *roots, u32 root_count, const u32 *root_offsets)
{
    u32 i;
    u32 j;

    gNdsNativeFighterValidateRejectAbsentBinding = 0xffffffffu;
    gNdsNativeFighterValidateRejectForeignIndex = 0xffffffffu;
    gNdsNativeFighterValidateRejectForeignOffset = 0xffffffffu;
    if ((roots == NULL) || (root_offsets == NULL))
    {
        return;
    }
    for (i = 0u; i < root_count; i++)
    {
        for (j = 0u; j < root_count; j++)
        {
            if (root_offsets[j] == roots[i].root_offset)
            {
                break;
            }
        }
        if (j == root_count)
        {
            gNdsNativeFighterValidateRejectAbsentBinding = i;
            break;
        }
    }
    for (i = 0u; i < root_count; i++)
    {
        for (j = 0u; j < root_count; j++)
        {
            if (roots[j].root_offset == root_offsets[i])
            {
                break;
            }
        }
        if (j == root_count)
        {
            gNdsNativeFighterValidateRejectForeignIndex = i;
            gNdsNativeFighterValidateRejectForeignOffset = root_offsets[i];
            break;
        }
    }
}
#define NDS_NATIVE_FIGHTER_VALIDATE_REJECT(code_, root_, observed_, expected_) \
    do { \
        gNdsNativeFighterValidateRejectCode = (code_); \
        gNdsNativeFighterValidateRejectSlot = slot; \
        gNdsNativeFighterValidateRejectLow = use_low_detail; \
        gNdsNativeFighterValidateRejectRoot = (root_); \
        gNdsNativeFighterValidateRejectObserved = (observed_); \
        gNdsNativeFighterValidateRejectExpected = (expected_); \
        gNdsNativeFighterValidateRejectCount = root_count; \
        if (root_offsets != NULL) \
        { \
            u32 nds_reject_i_; \
            for (nds_reject_i_ = 0u; \
                 (nds_reject_i_ < root_count) && (nds_reject_i_ < 32u); \
                 nds_reject_i_++) \
            { \
                gNdsNativeFighterValidateRejectOffsets[nds_reject_i_] = \
                    root_offsets[nds_reject_i_]; \
            } \
        } \
        return FALSE; \
    } while (0)

s32 ndsRendererValidateNativeFighterOwner(
    u32 slot,
    u32 battle_slot,
    u32 use_low_detail,
    u32 asset_data_size,
    u32 root_count,
    const u32 *root_offsets,
    const u32 *material_counts)
{
    const NDSNativeRoot *roots;
    const NDSNativeEpoch *epochs;
    u32 expected_count;
    u32 expected_asset_data_size;
    const NDSNativeFighterOwnerRuntime *owner;
    const NDSNativeFighterRuntimeTables *tables;
    u32 epoch_count;
    u32 action_count;
    u32 run_count;
    u32 root_index;

    if ((root_offsets == NULL) || (material_counts == NULL))
    {
        NDS_NATIVE_FIGHTER_VALIDATE_REJECT(1u, 0xffffffffu,
            (u32)(uintptr_t)root_offsets, (u32)(uintptr_t)material_counts);
    }
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
        const u32 (*root_light_preambles)[2];
        u32 root_light_preamble_count;
        u32 epoch_index;

        root = ndsRendererNativeFighterResolveRoot(
            owner, slot, battle_slot, use_low_detail, root_index,
            root_offsets[root_index]);
        if (root == NULL)
        {
            ndsNativeFighterPublishRootSetDelta(
                roots, root_count, root_offsets);
            NDS_NATIVE_FIGHTER_VALIDATE_REJECT(4u, root_index,
                root_offsets[root_index], roots[root_index].root_offset);
        }
        /* Validation runs before production selects the active owner. Resolve
         * ordinary roots against the owner being validated so a prior draw
         * cannot supply unrelated tables; copied Kirby hats still override it. */
        tables = ndsRendererNativeFighterTablesForResolvedRoot(
            root, owner, battle_slot, root_index);
        root_light_preambles =
            ndsRendererNativeFighterLightPreamblesForResolvedRoot(
                root, owner, battle_slot, root_index,
                &root_light_preamble_count);
        if (tables == NULL)
        {
            NDS_NATIVE_FIGHTER_VALIDATE_REJECT(
                5u, root_index, root_offsets[root_index], 0u);
        }
        epoch_count = tables->epoch_count;
        action_count = tables->vertex_action_count;
        run_count = tables->run_count;
        epochs = tables->epochs;
        if (((u32)root->light_preamble >= root_light_preamble_count) ||
            ((root->light_preamble != 0u) && (root_light_preambles == NULL)))
        {
            NDS_NATIVE_FIGHTER_VALIDATE_REJECT(5u, root_index,
                (u32)root->light_preamble, root_light_preamble_count);
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
        for (epoch_index = 0u;
             epoch_index < root->epoch_count;
             epoch_index++)
        {
            const NDSNativeEpoch *epoch =
                &epochs[root->first_epoch + epoch_index];
            u32 action_index;
            u32 run_index;
            u32 source_command_index =
                epoch->first_triangle_command_index;
            u32 tri2_half = 0u;

            if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
                (epoch->material_slot >= material_counts[root_index]))
            {
                NDS_NATIVE_FIGHTER_VALIDATE_REJECT(7u, root_index,
                    material_counts[root_index], epoch->material_slot);
            }
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
        }
    }
    return TRUE;
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
    return ndsRendererExecuteNativeFighterRootHardware(
        slot, root_ordinal, asset_base, root_offset,
        materials, material_count, config,
        callback, callback_user, stats, vertex_cache);
}
