s32 ndsRendererExecuteNativeFighterOwnerHierarchy(
    u32 slot,
    const void *asset_base_ptr,
    const NDSRendererNativeFighterHierarchy *hierarchy,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    u32 *out_hardware_started)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    const u8 *asset_base = asset_base_ptr;
    NDSNativeHierarchyTables tables;
    NDSRendererTraversalState *state =
        &sNdsNativeFighterOwnerExecution.traversal;
    u32 native_run_count = 0u;
    u32 native_triangle_count = 0u;
    u32 matrix_generation = 1u;
    u32 joint_index;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    volatile NDSRendererOwnerProfile *m2_owner =
        ndsRendererProfileM2Owner();
    u32 m2_total_start = cpuGetTiming();
    u32 m2_lighting_before = (m2_owner != NULL) ?
        m2_owner->m2_lighting_shading_ticks : 0u;
    u32 m2_root_gx_before = (m2_owner != NULL) ?
        m2_owner->m2_root_gx_ticks : 0u;
    u32 m2_run_prepare_before = (m2_owner != NULL) ?
        m2_owner->m2_run_prepare_ticks : 0u;
    u32 m2_emit_before = (m2_owner != NULL) ?
        m2_owner->m2_corner_emit_account_ticks : 0u;
#endif

    (void)callback_user;
    if (out_hardware_started == NULL)
    {
        return FALSE;
    }
    *out_hardware_started = FALSE;
    /* Mode 7 is intentionally high-detail-only.  No shipping P2 target uses it;
     * forcing the historical view here is safer than silently feeding low
     * tables into a hierarchy schedule generated only for the high JointTree. */
    if (ndsRendererNativeSelectFighterRuntimeTables(slot, FALSE) == FALSE)
    {
        return FALSE;
    }
    if (ndsRendererNativePreflightFighterHierarchy(
            slot, asset_base, hierarchy, callback, stats, &tables) == FALSE)
    {
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        ndsRendererProfileM2FinishProduction(
            m2_owner, m2_total_start, m2_lighting_before,
            m2_root_gx_before, m2_run_prepare_before,
            m2_emit_before, FALSE);
#endif
        return FALSE;
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    if (m2_owner != NULL)
    {
        m2_owner->m2_run_prepare_count += (slot == 0u) ? 30u : 37u;
        m2_owner->m2_lighting_epoch_count += (slot == 0u) ? 18u : 31u;
    }
#endif

    ndsRendererInitTraversalState(
        state, NULL, stats, NULL, NULL, 0u);
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
    {
        m4x4 camera_hardware;

        ndsRendererNativeBuildHierarchyHardwareAffine(
            hierarchy->camera_modelview, &camera_hardware);
        *out_hardware_started = TRUE;
        ndsRendererHardwareEndBatch();
        ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
        {
            m4x4 projection_hardware;
            ndsRendererCopyMtx20p12ToM4x4(
                hierarchy->projection, &projection_hardware);
            glLoadMatrix4x4(&projection_hardware);
        }
        ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
        glLoadMatrix4x4(&camera_hardware);
        ndsRendererProfileRecordMatrixLoad();
        sNdsRendererHardwareMatrixMode =
            NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY;
        sNdsRendererHardwareMatrixGeneration = matrix_generation;
        sNdsRendererHardwareMatrixLoaded = TRUE;
        stats->hardware_matrix_seed_count++;
    }
#endif

    for (joint_index = 0u; joint_index < tables.joint_count; joint_index++)
    {
        u32 packed = tables.schedule[joint_index];
        u32 binding = (packed >> 5) & 31u;
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
        m4x4 local_hardware;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        u32 m2_gx_start = cpuGetTiming();
#endif

        ndsRendererHardwareEndBatch();
        ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
        if ((packed & 0x8000u) != 0u)
        {
            glPushMatrix();
            stats->matrix_push_count++;
        }
        ndsRendererNativeBuildHierarchyHardwareAffine(
            &hierarchy->joint_locals[joint_index], &local_hardware);
        glMultMatrix4x4(&local_hardware);
        matrix_generation = ndsRendererNextMatrixGeneration();
        sNdsRendererHardwareMatrixMode =
            NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY;
        sNdsRendererHardwareMatrixGeneration = matrix_generation;
        sNdsRendererHardwareMatrixLoaded = TRUE;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        if (m2_owner != NULL)
        {
            m2_owner->m2_root_gx_ticks += cpuGetTiming() - m2_gx_start;
        }
#endif
#else
        matrix_generation++;
#endif
        if (binding != 31u)
        {
            u32 palette_slot = tables.cross_slots[binding];

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            u32 m2_gx_start = cpuGetTiming();
#endif
            if (palette_slot <= NDS_NATIVE_GX_MATRIX_SLOT_MAX)
            {
                glStoreMatrix((int)palette_slot);
            }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            if (m2_owner != NULL)
            {
                m2_owner->m2_root_gx_ticks +=
                    cpuGetTiming() - m2_gx_start;
                m2_owner->m2_root_gx_count++;
            }
#endif
#endif
            ndsRendererNativeCommitHierarchyRoot(
                asset_base, &tables.roots[binding],
                &hierarchy->roots[binding],
                tables.binding_joints[binding], palette_slot,
                matrix_generation, stats, state,
                &native_run_count, &native_triangle_count);
        }
        {
            u32 next_parent = (joint_index + 1u < tables.joint_count) ?
                (tables.schedule[joint_index + 1u] & 31u) : 31u;
            u32 cursor = joint_index;

            while (cursor != next_parent)
            {
                u32 cursor_packed = tables.schedule[cursor];
                u32 parent = cursor_packed & 31u;

                if ((cursor_packed & 0x8000u) != 0u)
                {
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                    u32 m2_gx_start = cpuGetTiming();
#endif
                    ndsRendererHardwareEndBatch();
                    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
                    glPopMatrix(1);
                    stats->matrix_pop_count++;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                    if (m2_owner != NULL)
                    {
                        m2_owner->m2_root_gx_ticks +=
                            cpuGetTiming() - m2_gx_start;
                    }
#endif
#endif
                }
                cursor = parent;
            }
        }
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
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    if (m2_owner != NULL)
    {
        m2_owner->m2_corner_emit_run_count += native_run_count;
    }
    ndsRendererProfileM2FinishProduction(
        m2_owner, m2_total_start, m2_lighting_before,
        m2_root_gx_before, m2_run_prepare_before,
        m2_emit_before, TRUE);
#endif
    return TRUE;
#else
    (void)slot;
    (void)asset_base_ptr;
    (void)hierarchy;
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
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static s32 ndsRendererNativeStageValidateStateSpanTopology(
    const NDSNativeStageStateSpan *span)
{
    u32 i;

    if ((span == NULL) ||
        ((u32)span->first_state + span->state_count >
         NDS_NATIVE_STAGE_STATE_SEQUENCE_COUNT))
    {
        return FALSE;
    }
    for (i = 0u; i < span->state_count; i++)
    {
        u32 delta_index = sNdsNativeStageStateSequence[
            (u32)span->first_state + i];
        const NDSNativeStageStateDelta *delta;

        if (delta_index >= NDS_NATIVE_STAGE_STATE_DELTA_COUNT)
        {
            return FALSE;
        }
        delta = &sNdsNativeStageStateDeltas[delta_index];
        if (delta->effect == NDS_NATIVE_STATE_MATERIAL)
        {
            if ((delta->material_event >=
                 NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT) ||
                (delta->material_command >=
                 sNdsNativeStageMaterialEvents[
                     delta->material_event].source_command_count))
            {
                return FALSE;
            }
            continue;
        }
        if (delta->effect == NDS_NATIVE_STATE_BLEND)
        {
            continue;
        }
        if ((delta->effect < NDS_NATIVE_STATE_OTHERMODE) ||
            (delta->effect > NDS_NATIVE_STATE_PRIM))
        {
            return FALSE;
        }
        if ((delta->effect == NDS_NATIVE_STATE_IMAGE) &&
            ((delta->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
             (delta->w1 >= sNdsNativeStageAssets[
                 delta->asset_index].payload_size)))
        {
            return FALSE;
        }
    }
    return TRUE;
}

#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
static u32 ndsRendererNativeStageGeneratedHashU32(u32 hash, u32 value)
{
    u32 shift;

    for (shift = 0u; shift < 32u; shift += 8u)
    {
        hash ^= (value >> shift) & 0xffu;
        hash *= 16777619u;
    }
    return hash;
}

/* This runs only behind the existing generation/stamp full validation.  It
 * binds the compact Task-26 rows to the exact production packet and to the
 * Task-14 dense first-visit plan consumed by PrepareRun. */
static s32 ndsRendererNativeStageValidateGeneratedSegment0(u32 inject_fault)
{
    const NDSNativeStageGeneratedCertificate *certificate =
        &sNdsNativeStageSegment0ColdCertificate;
    const NDSNativeStageSegment *segment = &sNdsNativeStageSegments[0];
    u32 hot_hash = 2166136261u;
    u32 dense_hash = 2166136261u;
    u32 state_count = 0u;
    u32 sync_count = 0u;
    u32 state_cursor = certificate->first_state;
    u32 triangle_count = 0u;
    u64 epoch_mask = 0u;
    u32 i;

    if ((certificate->source_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_SOURCE_CHECKSUM) ||
        (certificate->table_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_TABLE_CHECKSUM) ||
        (certificate->hot_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_HOT_CHECKSUM) ||
        (certificate->prepared_dense_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_CHECKSUM) ||
        (certificate->segment_index != 0u) ||
        (certificate->first_dobj != segment->first_dobj) ||
        (certificate->dobj_count != segment->dobj_count) ||
        (certificate->owner != segment->owner) ||
        (certificate->link != segment->link) ||
        (certificate->submit_class !=
         NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ||
        (certificate->first_binding != segment->first_binding) ||
        (certificate->binding_count != segment->binding_count) ||
        (certificate->first_run != segment->first_run) ||
        (certificate->run_count != segment->run_count) ||
        (certificate->first_texture_epoch != 0u) ||
        (certificate->triangle_count != 54u) ||
        (certificate->texture_epoch_count != 22u) ||
        (certificate->live_operand_mask !=
         (((u32)1u << NDS_NATIVE_STAGE_LIVE_OPERAND_ASSET_BASES) |
          ((u32)1u << NDS_NATIVE_STAGE_LIVE_OPERAND_BINDING_COMPOSED) |
          ((u32)1u << NDS_NATIVE_STAGE_LIVE_OPERAND_CONFIG))) ||
        (certificate->asset_base_mask != 3u) ||
        (certificate->material_count != 0u) ||
        (certificate->final_tail_span != 73u) ||
        (certificate->prepared_dense_count !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_COUNT) ||
        (certificate->prepared_dense_offset_count !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_OFFSET_COUNT) ||
        (sNdsNativeStageValidationCache.prepared_dense_offsets[0] != 0u) ||
        (sNdsNativeStageValidationCache.prepared_dense_offsets[
             NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT] !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_COUNT))
    {
        return FALSE;
    }

    hot_hash = ndsRendererNativeStageGeneratedHashU32(
        hot_hash, 0x4d335348u);
    hot_hash = ndsRendererNativeStageGeneratedHashU32(
        hot_hash, NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT);
    for (i = 0u; i < NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT; i++)
    {
        const NDSNativeStageGeneratedRun *generated =
            &sNdsNativeStageSegment0HotRuns[i];
        u32 run_index = generated->run_index;
        const NDSNativeStageRun *run;
        const NDSNativeStageStateSpan *span;

        if ((inject_fault != 0u) && (i == 0u))
        {
            run_index ^= 1u;
        }
        hot_hash = ndsRendererNativeStageGeneratedHashU32(
            hot_hash, run_index);
        hot_hash = ndsRendererNativeStageGeneratedHashU32(
            hot_hash, generated->binding_composed_index);
        if (run_index != (u32)certificate->first_run + i)
        {
            return FALSE;
        }
        run = &sNdsNativeStageRuns[run_index];
        span = &sNdsNativeStageStateSpans[run_index];
        if ((run->binding_index != generated->binding_composed_index) ||
            (run->submit_class != certificate->submit_class) ||
            (run->flags != 0u) ||
            (run->texture_epoch < certificate->first_texture_epoch) ||
            (run->texture_epoch >=
             (u32)certificate->first_texture_epoch +
                 certificate->texture_epoch_count))
        {
            return FALSE;
        }
        if ((span->state_count != 0u) &&
            (span->first_state != state_cursor))
        {
            return FALSE;
        }
        state_cursor += span->state_count;
        state_count += span->state_count;
        sync_count += span->sync_count;
        triangle_count += run->triangle_count;
        epoch_mask |= (u64)1u << run->texture_epoch;
    }
    {
        const NDSNativeStageStateSpan *tail =
            &sNdsNativeStageStateSpans[certificate->final_tail_span];

        if ((tail->state_count != 0u) &&
            (tail->first_state != state_cursor))
        {
            return FALSE;
        }
        state_cursor += tail->state_count;
        state_count += tail->state_count;
        sync_count += tail->sync_count;
    }
    if ((hot_hash != certificate->hot_checksum) ||
        (state_count != certificate->state_count) ||
        (sync_count != certificate->sync_count) ||
        (state_cursor !=
         (u32)certificate->first_state + certificate->state_count) ||
        (triangle_count != certificate->triangle_count) ||
        (epoch_mask != (((u64)1u << certificate->texture_epoch_count) - 1u)))
    {
        return FALSE;
    }

    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, 0x4d33535du);
    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, certificate->prepared_dense_offset_count);
    for (i = 0u; i < certificate->prepared_dense_offset_count; i++)
    {
        dense_hash = ndsRendererNativeStageGeneratedHashU32(
            dense_hash,
            sNdsNativeStageValidationCache.prepared_dense_offsets[i]);
    }
    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, 0x4d33535eu);
    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, certificate->prepared_dense_count);
    for (i = 0u; i < certificate->prepared_dense_count; i++)
    {
        dense_hash = ndsRendererNativeStageGeneratedHashU32(
            dense_hash,
            sNdsNativeStageValidationCache.prepared_dense_indices[i]);
    }
    if (dense_hash != certificate->prepared_dense_checksum)
    {
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL == 1
    if (inject_fault == 0u)
    {
        gNdsRendererM3GeneratedSegment0CertificateValidationCount++;
    }
#endif
    return TRUE;
}
#endif

static s32 ndsRendererNativeStageValidateTopologyFull(
    const NDSRendererNativeStageFrame *frame,
    NDSNativeStageTopologySummary *summary)
{
    u32 prepared_dense_mask[
        (NDS_NATIVE_STAGE_MAX_DENSE_VERTEX_COUNT + 31u) / 32u];
    u32 prepared_dense_count = 0u;
    u32 i;

    if ((frame == NULL) || (summary == NULL) ||
        (frame->topology_generation == 0u) ||
        (frame->topology_stamp == 0u))
    {
        return FALSE;
    }
    memset(summary, 0, sizeof(*summary));
    memset(prepared_dense_mask, 0, sizeof(prepared_dense_mask));
    for (i = 0u; i < NDS_NATIVE_STAGE_DOBJ_COUNT; i++)
    {
        const NDSNativeStageDObj *expected = &sNdsNativeStageDObjs[i];
        const NDSRendererNativeStageDObj *live = &frame->dobjs[i];

        if ((live->identity == NULL) ||
            (live->parent_index != expected->parent_index) ||
            (live->binding_index != expected->binding_index) ||
            (live->transform_flags != expected->transform_flags) ||
            (live->owner != expected->owner) ||
            (live->depth != expected->depth))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_ASSET_COUNT; i++)
    {
        if ((frame->asset_bases[i] == NULL) ||
            (sNdsNativeStageAssets[i].payload_size == 0u))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_SEGMENT_COUNT; i++)
    {
        const NDSNativeStageSegment *segment = &sNdsNativeStageSegments[i];
        u32 binding_offset;

        if (((u32)segment->first_dobj + segment->dobj_count >
             NDS_NATIVE_STAGE_DOBJ_COUNT) ||
            ((u32)segment->first_binding + segment->binding_count >
             NDS_NATIVE_STAGE_BINDING_COUNT) ||
            ((u32)segment->first_run + segment->run_count >
             NDS_NATIVE_STAGE_RUN_COUNT) ||
            (segment->reserved != 0u))
        {
            return FALSE;
        }
        for (binding_offset = 0u;
             binding_offset < segment->binding_count;
             binding_offset++)
        {
            u32 binding_index =
                (u32)segment->first_binding + binding_offset;
            const NDSNativeStageBinding *binding =
                &sNdsNativeStageBindings[binding_index];

            if ((binding->first_run < segment->first_run) ||
                ((u32)binding->first_run + binding->run_count >
                 (u32)segment->first_run + segment->run_count))
            {
                return FALSE;
            }
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_BINDING_COUNT; i++)
    {
        const NDSNativeStageBinding *binding = &sNdsNativeStageBindings[i];

        if ((binding->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
            (binding->root_offset >= sNdsNativeStageAssets[
                 binding->asset_index].payload_size) ||
            ((u32)binding->first_vertex + binding->source_vertex_count >
             NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT) ||
            ((u32)binding->first_run + binding->run_count >
             NDS_NATIVE_STAGE_RUN_COUNT) ||
            ((u32)binding->first_epoch + binding->texture_epoch_count >
             NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT) ||
            ((binding->material_event != 0xffu) &&
             (binding->material_event >=
              NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT)) ||
            (frame->binding_display_lists[i] !=
             (const void *)((const u8 *)frame->asset_bases[
                 binding->asset_index] + binding->root_offset)))
        {
            return FALSE;
        }
        {
            u32 run_offset;

            for (run_offset = 0u; run_offset < binding->run_count;
                 run_offset++)
            {
                if (sNdsNativeStageRuns[
                        (u32)binding->first_run + run_offset].binding_index != i)
                {
                    return FALSE;
                }
            }
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT; i++)
    {
        const NDSNativeStageTextureEpoch *epoch =
            &sNdsNativeStageTextureEpochs[i];

        if ((epoch->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
            (epoch->source_command_offset >= sNdsNativeStageAssets[
                 epoch->asset_index].payload_size) ||
            (epoch->policy_index >= NDS_NATIVE_STAGE_STATE_POLICY_COUNT) ||
            ((epoch->material_event != 0xffu) &&
             (epoch->material_event >=
              NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT)))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT; i++)
    {
        const NDSNativeStageMaterialEvent *event =
            &sNdsNativeStageMaterialEvents[i];

        if ((event->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
            (event->mobj_offset >= sNdsNativeStageAssets[
                 event->asset_index].payload_size) ||
            (event->binding_index >= NDS_NATIVE_STAGE_BINDING_COUNT) ||
            (event->segment_index >= NDS_NATIVE_STAGE_SEGMENT_COUNT) ||
            (event->material_slot >=
             NDS_RENDERER_NATIVE_STAGE_MATERIAL_COUNT) ||
            (event->source_command_count == 0u))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_STATE_SPAN_COUNT; i++)
    {
        if (ndsRendererNativeStageValidateStateSpanTopology(
                &sNdsNativeStageStateSpans[i]) == FALSE)
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT; i++)
    {
        if (sNdsNativeStageVertices[i].matrix_binding >=
            NDS_NATIVE_STAGE_BINDING_COUNT)
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_RUN_COUNT; i++)
    {
        const NDSNativeStageRun *run = &sNdsNativeStageRuns[i];
        const NDSNativeStageTextureEpoch *epoch;
        u32 corner_count = (u32)run->triangle_count * 3u;
        u32 corner_offset;
        u32 run_alpha = UINT_MAX;

        if ((run->triangle_count == 0u) ||
            (run->binding_index >= NDS_NATIVE_STAGE_BINDING_COUNT) ||
            (run->texture_epoch >= NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT) ||
            (run->state_policy >= NDS_NATIVE_STAGE_STATE_POLICY_COUNT) ||
            ((run->flags &
              ~NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX) != 0u) ||
            ((u32)run->first_corner + corner_count >
             NDS_NATIVE_STAGE_CORNER_COUNT))
        {
            return FALSE;
        }
        epoch = &sNdsNativeStageTextureEpochs[run->texture_epoch];
        if (epoch->policy_index != run->state_policy)
        {
            return FALSE;
        }
        if (run->submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
        {
            summary->raw_triangles += run->triangle_count;
        }
        else if (run->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
        {
            summary->projected_no_z_triangles += run->triangle_count;
        }
        else if (run->submit_class ==
                 NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
        {
            summary->projected_range_triangles += run->triangle_count;
        }
        else
        {
            return FALSE;
        }
        sNdsNativeStageValidationCache.prepared_dense_offsets[i] =
            (u16)prepared_dense_count;
        for (corner_offset = 0u; corner_offset < corner_count; corner_offset++)
        {
            u32 dense_index = sNdsNativeStageCorners[
                (u32)run->first_corner + corner_offset];
            const NDSNativeStageDenseVertex *dense;

            if (dense_index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT)
            {
                return FALSE;
            }
            dense = &sNdsNativeStageVertices[dense_index];
            if ((run->submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) &&
                (dense->matrix_binding != run->binding_index))
            {
                return FALSE;
            }
            if (run_alpha == UINT_MAX)
            {
                run_alpha = dense->rgba & 0xffu;
            }
            else if (run_alpha != (dense->rgba & 0xffu))
            {
                return FALSE;
            }
            if ((prepared_dense_mask[dense_index / 32u] &
                 ((u32)1u << (dense_index & 31u))) == 0u)
            {
                if (prepared_dense_count >=
                    NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT)
                {
                    return FALSE;
                }
                prepared_dense_mask[dense_index / 32u] |=
                    (u32)1u << (dense_index & 31u);
                sNdsNativeStageValidationCache.prepared_dense_indices[
                    prepared_dense_count++] = (u16)dense_index;
            }
            if (run->submit_class ==
                NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
            {
                s32 x = ndsRendererNativeStageVertexShift(dense->x, 1u);
                s32 y = ndsRendererNativeStageVertexShift(dense->y, 1u);
                s32 z = ndsRendererNativeStageVertexShift(dense->z, 1u);

                if ((dense->matrix_binding != run->binding_index) ||
                    (x < -2048) || (x > 2047) ||
                    (y < -2048) || (y > 2047) ||
                    (z < -2048) || (z > 2047))
                {
                    return FALSE;
                }
            }
        }
        if (run_alpha == UINT_MAX)
        {
            return FALSE;
        }
        if ((run->flags &
             NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX) != 0u)
        {
            if ((run->submit_class !=
                 NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ||
                (run->triangle_count != 2u))
            {
                return FALSE;
            }
            summary->cross_runs++;
            summary->cross_triangles += run->triangle_count;
            for (corner_offset = 0u;
                 corner_offset < corner_count;
                 corner_offset++)
            {
                u32 dense_index = sNdsNativeStageCorners[
                    (u32)run->first_corner + corner_offset];

                if (sNdsNativeStageVertices[dense_index].matrix_binding !=
                    run->binding_index)
                {
                    summary->cross_foreign_corners++;
                }
            }
        }
    }
    sNdsNativeStageValidationCache.prepared_dense_offsets[
        NDS_NATIVE_STAGE_RUN_COUNT] = (u16)prepared_dense_count;
#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
    if ((NDS_NATIVE_STAGE_STATE_SPAN_COUNT !=
         NDS_NATIVE_STAGE_RUN_COUNT + NDS_NATIVE_STAGE_BINDING_COUNT) ||
        (prepared_dense_count != NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT) ||
        (summary->raw_triangles != NDS_NATIVE_STAGE_SUBMIT_RAW_TRIANGLES) ||
        (summary->projected_no_z_triangles !=
         NDS_NATIVE_STAGE_SUBMIT_NO_Z_TRIANGLES) ||
        (summary->projected_range_triangles !=
         NDS_NATIVE_STAGE_SUBMIT_RANGE_TRIANGLES) ||
        (summary->cross_runs != NDS_NATIVE_STAGE_CROSS_MATRIX_RUN_COUNT) ||
        (summary->cross_triangles !=
         NDS_NATIVE_STAGE_CROSS_MATRIX_TRIANGLE_COUNT) ||
        (summary->cross_foreign_corners !=
         NDS_NATIVE_STAGE_CROSS_MATRIX_FOREIGN_CORNER_COUNT))
    {
        return FALSE;
    }
    /* The straight-line segment-0 program is one stage's specialisation
     * (Dream Land's).  A stage without one is not invalid -- it just runs the
     * general per-run path -- so its certificate must not be checked against
     * another stage's tables. */
    return (NDS_NATIVE_STAGE_HAS_GENERATED_SEGMENT0 != 0u) ?
        ndsRendererNativeStageValidateGeneratedSegment0(FALSE) : TRUE;
#else
    return ((NDS_NATIVE_STAGE_STATE_SPAN_COUNT ==
             NDS_NATIVE_STAGE_RUN_COUNT + NDS_NATIVE_STAGE_BINDING_COUNT) &&
            (prepared_dense_count == NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT) &&
            (summary->raw_triangles == NDS_NATIVE_STAGE_SUBMIT_RAW_TRIANGLES) &&
            (summary->projected_no_z_triangles ==
             NDS_NATIVE_STAGE_SUBMIT_NO_Z_TRIANGLES) &&
            (summary->projected_range_triangles ==
             NDS_NATIVE_STAGE_SUBMIT_RANGE_TRIANGLES) &&
            (summary->cross_runs ==
             NDS_NATIVE_STAGE_CROSS_MATRIX_RUN_COUNT) &&
            (summary->cross_triangles ==
             NDS_NATIVE_STAGE_CROSS_MATRIX_TRIANGLE_COUNT) &&
            (summary->cross_foreign_corners ==
             NDS_NATIVE_STAGE_CROSS_MATRIX_FOREIGN_CORNER_COUNT)) ?
        TRUE : FALSE;
#endif
}

static s32 ndsRendererNativeStageValidateTopology(
    const NDSRendererNativeStageFrame *frame,
    NDSNativeStageTopologySummary *summary)
{
    s32 injected_fault = FALSE;

#if NDS_RENDERER_M3_PHASE0_PROFILE
    if ((sNdsNativeStageValidationCache.valid != FALSE) &&
        (sNdsNativeStageTopologyFaultInjected == FALSE))
    {
        sNdsNativeStageValidationCache.stamp ^= 1u;
        sNdsNativeStageTopologyFaultInjected = TRUE;
        gNdsRendererM3TopologyFaultInjectionCount++;
        injected_fault = TRUE;
    }
#endif
    if ((sNdsNativeStageValidationCache.valid != FALSE) &&
        (sNdsNativeStageValidationCache.generation ==
         frame->topology_generation) &&
        (sNdsNativeStageValidationCache.stamp == frame->topology_stamp))
    {
        *summary = sNdsNativeStageValidationCache.summary;
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3TopologyCacheHitCount++;
#endif
        return TRUE;
    }
    if (sNdsNativeStageValidationCache.valid != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3TopologyStampMismatchCount++;
#endif
    }
    sNdsNativeStageValidationCache.valid = FALSE;
    if (ndsRendererNativeStageValidateTopologyFull(frame, summary) == FALSE)
    {
        return FALSE;
    }
    sNdsNativeStageValidationCache.summary = *summary;
    sNdsNativeStageValidationCache.generation = frame->topology_generation;
    sNdsNativeStageValidationCache.stamp = frame->topology_stamp;
    sNdsNativeStageValidationCache.valid = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3TopologyFullValidationCount++;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    if (injected_fault != FALSE)
    {
        gNdsRendererM3TopologyFaultRevalidationCount++;
    }
#else
    (void)injected_fault;
#endif
    return TRUE;
}

static s32 ndsRendererNativeStageApplyStateSpan(
    const NDSNativeStageStateSpan *span,
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 i;

    if ((span == NULL) || (frame == NULL) || (stats == NULL) ||
        (state == NULL))
    {
        return FALSE;
    }
    stats->sync_command_count += span->sync_count;
    for (i = 0u; i < span->state_count; i++)
    {
        u32 delta_index = sNdsNativeStageStateSequence[
            (u32)span->first_state + i];
        const NDSNativeStageStateDelta *delta =
            &sNdsNativeStageStateDeltas[delta_index];
        if (delta->effect == NDS_NATIVE_STATE_MATERIAL)
        {
            if (delta->material_command == 0u)
            {
                ndsRendererNativeApplyMaterialPreflight(
                    &frame->materials[delta->material_event], stats, state);
            }
            continue;
        }
        if (delta->effect == NDS_NATIVE_STATE_BLEND)
        {
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            stats->blend_color = delta->w1;
            stats->color_command_count++;
            continue;
        }
        {
            NDSNativeStateDelta native_delta;
            const u8 *asset_base = frame->asset_bases[0];

            native_delta.w0 = delta->w0;
            native_delta.w1 = delta->w1;
            native_delta.effect = delta->effect;
            native_delta.reserved[0] = 0u;
            native_delta.reserved[1] = 0u;
            native_delta.reserved[2] = 0u;
            if (delta->effect == NDS_NATIVE_STATE_IMAGE)
            {
                asset_base = frame->asset_bases[delta->asset_index];
            }
            ndsRendererNativeApplyStateDelta(
                &native_delta, asset_base, stats, state);
        }
    }
    return TRUE;
}

static s32 ndsRendererNativeStagePolicyMatches(
    const NDSNativeStageStatePolicy *policy,
    const NDSRendererStats *stats)
{
    return ((policy != NULL) && (stats != NULL) &&
            (stats->texture_combine_w0 == policy->combine_w0) &&
            (stats->texture_combine_w1 == policy->combine_w1) &&
            (stats->othermode_h == policy->othermode_h) &&
            (stats->othermode_l == policy->othermode_l) &&
            (stats->geometry_mode == policy->geometry_mode)) ? TRUE : FALSE;
}

static void ndsRendererNativeStageInputVertex(
    const NDSNativeStageDenseVertex *dense,
    NDSRendererInputVertex *input)
{
    input->x = dense->x;
    input->y = dense->y;
    input->z = dense->z;
    input->s = dense->s;
    input->t = dense->t;
    input->r = (u8)(dense->rgba >> 24);
    input->g = (u8)(dense->rgba >> 16);
    input->b = (u8)(dense->rgba >> 8);
    input->a = (u8)dense->rgba;
}

#if NDS_TASK103_STAGE_RUN_PHASE
/* Task 103 E6. Splits ndsRendererNativeStagePrepareRun into its head (policy
 * match, the two memsets, texture resolve) and its per-dense-vertex loop, and
 * counts both the dense vertices touched and the subset that take the
 * camera-dependent near-plane transform.
 *
 * The point of the split: the loop's colour and texcoord terms read only
 * compile-time vertex data plus per-run material state, while only near_inside
 * reads frame->binding_composed, which the moving camera changes every frame.
 * The gap between DenseCount and NearCount is therefore the size of the memo,
 * and this measures it before anyone proposes one. */
volatile u32 gNdsTask103RunHeadTicks;
volatile u32 gNdsTask103RunDenseTicks;
volatile u32 gNdsTask103RunDenseCount;
volatile u32 gNdsTask103RunNearCount;
/* R2-02 F1/F3. The four actor segments (1/2/3/6 -- Whispy's eyes and mouth,
 * both flower beds) cost 43,998 ticks/frame for 21 triangles, nearly double
 * segment 4's 22,843 for 76, so the split is by *branch*: which matrix path
 * BeginRun takes, which of EmitNoZTriangle's three paths a triangle lands on,
 * and where BeginRun's non-matrix half goes.
 *
 * F0's adjacent-run redundancy comparison lived here too and has been removed.
 * It answered its question once -- 1.0 of 21 runs repeats the previous run's
 * state, 18 rebind a texture -- and it read six prepared_run fields inside
 * ndsRendererCommitNativeStageSegment, whose consumed-field closure the stage
 * falsifier polices. Classifying them would have asserted an immutability F0
 * itself disproved. */
volatile u32 gNdsTask103BeginMtxTicks[4];
volatile u32 gNdsTask103BeginMtxCount[4];
volatile u32 gNdsTask103NoZPath[4];
volatile u32 gNdsTask103NoZWorldTicks;
volatile u32 gNdsTask103NoZProjTicks;
/* F3. The matrix branch is 12,341 of BeginRun's 28,880; the other 16,539 over
 * 21 runs is this tail. Split it, because the texture bind is the one part a
 * specialized actor path could not avoid -- the DS still has to rebind on 18 of
 * 21 runs -- and that sets the floor any rewrite has to beat. */
volatile u32 gNdsTask103BeginEndBatchTicks;
volatile u32 gNdsTask103BeginTexTicks;
volatile u32 gNdsTask103BeginTailTicks;
#endif

static s32 ndsRendererNativeStagePrepareRun(
    u32 run_index,
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u64 *epoch_mask)
{
    const NDSNativeStageRun *run = &sNdsNativeStageRuns[run_index];
    const NDSNativeStageTextureEpoch *epoch;
    const NDSNativeStageStatePolicy *policy;
    NDSNativeStagePreparedRun *prepared =
        &sNdsNativeStageOwnerExecution.runs[run_index];
    const NDSRendererTileState *render_tile;
    NDSRendererHardwareResolvedTexture resolved;
    u32 implicit_texture_on;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 material_color;
    s32 alpha_uses_vertex;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 texture_offset;
    u32 first_visit_offset;
    u32 first_visit_end;
    u32 dense_offset;
    u32 alpha = UINT_MAX;
    u32 use_texture;
#if NDS_TASK103_STAGE_RUN_PHASE
    u32 task103_run_entry;
    u32 task103_run_mark;
#endif
#if NDS_TASK36_REJECT_TRACE
    gNdsRendererTask36PrepareRunRejectReason = 0u;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 vertex_prepare_start;
    u32 residual_vertex_ticks_start;
    u32 residual_near_ticks_start;
    u32 residual_near_count_start;
#endif

    /* R2-07 leg A. THE write seam for runs[]: this function is its only writer,
     * so dropping the certificate here removes the class rather than an
     * instance (slice 30). The drop beside gNdsR2StagePrepareBuildCount is the
     * instance and is deliberately kept -- redundant but free, and deleting it
     * would be a second change with its own failure mode, which is slice 30's
     * own precedent for exactly this situation.
     *
     * The seam is the one that matters because the instance is not complete in
     * every configuration: with NDS_R2_STAGE_ROUTE_PROBE on, a segment can be
     * forced generic and re-prepared while r2_reuse is still 1, i.e. without
     * the rebuild branch ever running. That path is `#define ... FALSE` in
     * every shipping and gate target, so this line changes no measured
     * behaviour -- it stops the guard depending on a probe flag being off. */
    ndsRendererNativeStagePreparedTextureProofDrop();
#if NDS_TASK103_STAGE_RUN_PHASE
    task103_run_entry = cpuGetTiming();
#endif
    epoch = &sNdsNativeStageTextureEpochs[run->texture_epoch];
    policy = &sNdsNativeStageStatePolicies[run->state_policy];
    if (ndsRendererNativeStagePolicyMatches(policy, stats) == FALSE)
    {
#if NDS_TASK36_REJECT_TRACE
        gNdsRendererTask36PrepareRunRejectReason = 1u;
        NDS_R2_STAGE_REJECT_COUNT(1);
#endif
        return FALSE;
    }

    memset(prepared, 0, sizeof(*prepared));
    memset(&resolved, 0, sizeof(resolved));
    use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ?
        TRUE : FALSE;
    implicit_texture_on = ndsRendererHardwareTextureImplicitStateOn(stats);
    texture_scale_s = stats->texture_scale_s;
    texture_scale_t = stats->texture_scale_t;
    render_tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
#if NDS_R2_STAGE_ROUTE_PROBE
    gNdsR2StageTextureProbeRun = run_index;
#endif
    if ((use_texture != FALSE) &&
        (ndsRendererHardwareResolveStageSourceFrameTexture(
             stats, frame->config, state, &resolved) == FALSE))
    {
#if NDS_R2_STAGE_ROUTE_PROBE
        gNdsR2StageTextureProbeRun = 0xffffffffu;
#endif
#if NDS_TASK36_REJECT_TRACE
        gNdsRendererTask36PrepareRunRejectReason = 2u;
        NDS_R2_STAGE_REJECT_COUNT(2);
#endif
        return FALSE;
    }
#if NDS_R2_STAGE_ROUTE_PROBE
    gNdsR2StageTextureProbeRun = 0xffffffffu;
#endif
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
    prepared->texture_entry = resolved.entry;
    prepared->texture_name = resolved.name;
    prepared->texture_params = resolved.params;
    prepared->texture_generation = (resolved.entry != NULL) ?
        resolved.entry->key_generation : 0u;
    prepared->texture_format = (u8)resolved.format;
    prepared->texture_width = (u16)resolved.width;
    prepared->texture_height = (u16)resolved.height;
    prepared->textured = (u8)use_texture;
    prepared->alpha_test =
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) ==
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD) ? TRUE : FALSE;
    prepared->alpha_ref = (u8)((stats->blend_color & 0xffu) >> 4);

    material_color = ndsRendererHardwareColorSource(stats);
    alpha_uses_vertex = ndsRendererHardwareAlphaUsesVertex(stats);
    use_material_color = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
    if (alpha_uses_vertex == FALSE)
    {
        alpha = ndsRendererHardwareAlpha(stats, NULL);
    }
    if (alpha_uses_vertex != FALSE)
    {
        u32 dense_index = sNdsNativeStageCorners[run->first_corner];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];

        alpha = (dense->rgba & 0xffu) >> 3;
    }
    first_visit_offset =
        sNdsNativeStageValidationCache.prepared_dense_offsets[run_index];
    first_visit_end =
        sNdsNativeStageValidationCache.prepared_dense_offsets[run_index + 1u];
    if ((first_visit_offset > first_visit_end) ||
        (first_visit_end > NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT))
    {
#if NDS_TASK36_REJECT_TRACE
        gNdsRendererTask36PrepareRunRejectReason = 3u;
        NDS_R2_STAGE_REJECT_COUNT(3);
#endif
        return FALSE;
    }
#if NDS_RENDERER_M3_PHASE0_PROFILE
    residual_vertex_ticks_start = gNdsRendererM3Phase0VertexPrepareTicks;
    residual_near_ticks_start = gNdsRendererM3Phase0NearTransformTicks;
    residual_near_count_start = gNdsRendererM3Phase0NearTransformCount;
    vertex_prepare_start = ndsRendererM3Phase0Tick();
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
    /* E6. Everything up to here is per-run head work, of which the texture
     * resolve is the known-expensive part (Task 98: ~1,621/bind). The loop
     * below is per dense vertex, and its colour and texcoord terms depend only
     * on compile-time vertex data plus per-run material state -- only
     * near_inside reads the camera-composed matrix. Splitting head from loop
     * sizes the memo before anyone proposes it. */
    gNdsTask103RunHeadTicks += cpuGetTiming() - task103_run_entry;
    task103_run_mark = cpuGetTiming();
#endif
    for (dense_offset = first_visit_offset;
         dense_offset < first_visit_end;
         dense_offset++)
    {
        u32 dense_index =
            sNdsNativeStageValidationCache.prepared_dense_indices[
                dense_offset];
        const NDSNativeStageDenseVertex *dense;
        NDSNativeStagePreparedDense *prepared_dense;

        if (dense_index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT)
        {
#if NDS_TASK36_REJECT_TRACE
            gNdsRendererTask36PrepareRunRejectReason = 4u;
            NDS_R2_STAGE_REJECT_COUNT(4);
#endif
            return FALSE;
        }
        dense = &sNdsNativeStageVertices[dense_index];
        prepared_dense = &sNdsNativeStagePreparedDense[dense_index];
#if NDS_RENDERER_M3_PHASE0_PROFILE
        gNdsRendererM3Phase0PreparedDenseCount++;
#endif
#if NDS_R2_FLASH_PROBE
        gNdsR2FlashRawPending = 0u;
#endif
        prepared_dense->packed_color =
            ndsRendererHardwarePackedValidVertexColor(
                material_color, use_material_color,
                use_vertex_color, dense->rgba,
                frame->config->color_modulate);
        if (use_texture != FALSE)
        {
            prepared_dense->s = ndsRendererHardwareTexCoord(
                dense->s, texture_scale_s, render_tile->uls,
                texture_offset);
            prepared_dense->t = ndsRendererHardwareTexCoord(
                dense->t, texture_scale_t, render_tile->ult,
                texture_offset);
        }
        if (run->submit_class ==
                  NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
        {
#if NDS_TASK36_HW_COMPOSE
            if ((frame->rigid_binding_mask &
                 ((u64)1u << dense->matrix_binding)) != 0u)
            {
                /* GX performs the rigid near-plane clip after hardware
                 * composition; dynamic bindings keep the exact CPU clip. */
                prepared_dense->near_inside = TRUE;
            }
            else
#endif
            {
            NDSRendererInputVertex input;
            NDSRendererClipVertex20p12 clip;

            ndsRendererNativeStageInputVertex(dense, &input);
#if NDS_RENDERER_M3_PHASE0_PROFILE
            u32 near_transform_start = ndsRendererM3Phase0Tick();
#endif

            ndsRendererTransformVertex20p12(
                &frame->binding_composed[dense->matrix_binding],
                &input, &clip);
#if NDS_RENDERER_M3_PHASE0_PROFILE
            ndsRendererM3Phase0FinishSpan(
                &gNdsRendererM3Phase0NearTransformTicks,
                near_transform_start);
            gNdsRendererM3Phase0NearTransformCount++;
#endif
            if (clip.w == 0)
            {
#if NDS_TASK36_REJECT_TRACE
                gNdsRendererTask36PrepareRunRejectReason = 5u;
                NDS_R2_STAGE_REJECT_COUNT(5);
#endif
                return FALSE;
            }
            if (ndsRendererHardwareClipZWInsideNearPlane(
                    clip.z, clip.w) != FALSE)
            {
                prepared_dense->near_inside = TRUE;
            }
            else
            {
                prepared_dense->near_inside = FALSE;
            }
#if NDS_TASK103_STAGE_RUN_PHASE
            gNdsTask103RunNearCount++;
#endif
            }
        }
#if NDS_TASK103_STAGE_RUN_PHASE
        gNdsTask103RunDenseCount++;
#endif
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103RunDenseTicks += cpuGetTiming() - task103_run_mark;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0FinishSpan(
        &gNdsRendererM3Phase0VertexPrepareTicks, vertex_prepare_start);
    if (run_index >= NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT)
    {
        gNdsRendererM3ResidualVertexTicks +=
            gNdsRendererM3Phase0VertexPrepareTicks -
            residual_vertex_ticks_start;
        gNdsRendererM3ResidualNearTicks +=
            gNdsRendererM3Phase0NearTransformTicks -
            residual_near_ticks_start;
        gNdsRendererM3ResidualDenseCount +=
            first_visit_end - first_visit_offset;
        gNdsRendererM3ResidualNearCount +=
            gNdsRendererM3Phase0NearTransformCount -
            residual_near_count_start;
    }
#endif
    if (alpha == UINT_MAX)
    {
#if NDS_TASK36_REJECT_TRACE
        gNdsRendererTask36PrepareRunRejectReason = 6u;
        NDS_R2_STAGE_REJECT_COUNT(6);
#endif
        return FALSE;
    }
    prepared->poly_fmt = ndsRendererHardwarePolyFmt(stats, alpha);
    *epoch_mask |= (u64)1u << run->texture_epoch;
    return TRUE;
}

#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
static s32 ndsRendererNativeStagePrepareGeneratedSegment0(
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u64 *epoch_mask)
{
    const NDSNativeStageGeneratedCertificate *certificate =
        &sNdsNativeStageSegment0ColdCertificate;

#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3GeneratedSegment0AttemptCount++;
#endif
    if ((frame == NULL) || (stats == NULL) || (state == NULL) ||
        (epoch_mask == NULL) ||
        (sNdsNativeStageValidationCache.valid == FALSE))
    {
        goto fail;
    }
#define NDS_TASK26_SYNC(count) \
    do { \
        stats->sync_command_count += (u32)(count); \
    } while (0)
#define NDS_TASK26_OTHERMODE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordOtherMode( \
            stats, ((u32)(w0)) >> 24, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_COMBINE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetCombine(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_TEXTURE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordTextureState(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_GEOMETRY(clear_mask, set_mask) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        stats->geometry_mode = \
            (stats->geometry_mode & (u32)(clear_mask)) | (u32)(set_mask); \
        stats->geometry_clear_mask = (u32)(clear_mask); \
        stats->geometry_set_mask = (u32)(set_mask); \
        stats->geometry_command_count++; \
    } while (0)
#define NDS_TASK26_IMAGE(asset_index, w0, offset) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetImage( \
            stats, (u32)(w0), \
            (u32)(uintptr_t)(frame->asset_bases[(u32)(asset_index)] + \
                             (u32)(offset))); \
    } while (0)
#define NDS_TASK26_TILE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetTile(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_LOAD_TLUT(w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordLoadTlut(stats, (u32)(w1)); \
    } while (0)
#define NDS_TASK26_LOAD_BLOCK(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordLoadBlock(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_TILE_SIZE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetTileSize(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_BLEND(w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        stats->blend_color = (u32)(w1); \
        stats->color_command_count++; \
    } while (0)
#if NDS_RENDERER_M3_PHASE0_PROFILE
#define NDS_TASK26_RUN(run_index) \
    do { \
        u32 task26_prepare_start = ndsRendererM3Phase0Tick(); \
        s32 task26_prepare_result = ndsRendererNativeStagePrepareRun( \
            (u32)(run_index), frame, stats, state, epoch_mask); \
        ndsRendererM3Phase0FinishSpan( \
            &gNdsRendererM3Phase0PrepareRunTicks, task26_prepare_start); \
        if (task26_prepare_result == FALSE) \
        { \
            goto fail; \
        } \
    } while (0)
#else
#define NDS_TASK26_RUN(run_index) \
    do { \
        if (ndsRendererNativeStagePrepareRun( \
                (u32)(run_index), frame, stats, state, epoch_mask) == FALSE) \
        { \
            goto fail; \
        } \
    } while (0)
#endif
    NDS_NATIVE_STAGE_SEGMENT0_GENERATED_PROGRAM;
#undef NDS_TASK26_RUN
#undef NDS_TASK26_BLEND
#undef NDS_TASK26_TILE_SIZE
#undef NDS_TASK26_LOAD_BLOCK
#undef NDS_TASK26_LOAD_TLUT
#undef NDS_TASK26_TILE
#undef NDS_TASK26_IMAGE
#undef NDS_TASK26_GEOMETRY
#undef NDS_TASK26_TEXTURE
#undef NDS_TASK26_COMBINE
#undef NDS_TASK26_OTHERMODE
#undef NDS_TASK26_SYNC
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3GeneratedSegment0SuccessCount++;
    gNdsRendererM3GeneratedSegment0RunCount += certificate->run_count;
    gNdsRendererM3GeneratedSegment0TriangleCount +=
        certificate->triangle_count;
    gNdsRendererM3GeneratedSegment0EpochCount +=
        certificate->texture_epoch_count;
    gNdsRendererM3GeneratedSegment0MaterialCount +=
        certificate->material_count;
#endif
    return TRUE;

fail:
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3GeneratedSegment0PreGxFallbackCount++;
#endif
    return FALSE;
}

#if NDS_RENDERER_M3_PHASE0_PROFILE
static void ndsRendererNativeStageHashGeneratedSegment0Outputs(
    const NDSRendererStats *stats,
    const NDSRendererTraversalState *state,
    u64 epoch_mask,
    u32 *out_hash_a,
    u32 *out_hash_b,
    u32 *out_field_count)
{
    u32 hash_a = 2166136261u;
    u32 hash_b = 0x9e3779b9u;
    u32 field_count = 0u;
    u32 i;

#define NDS_TASK26_HASH_FIELD(value) \
    do { \
        u32 task26_value = (u32)(value); \
        hash_a = (hash_a ^ task26_value) * 16777619u; \
        hash_b ^= task26_value + 0x9e3779b9u + \
            (hash_b << 6) + (hash_b >> 2); \
        hash_b = ((hash_b << 7) | (hash_b >> 25)) * 0x85ebca6bu; \
        field_count++; \
    } while (0)

    for (i = 0u; i < NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT; i++)
    {
        u32 run_index = sNdsNativeStageSegment0HotRuns[i].run_index;
        const NDSNativeStagePreparedRun *prepared =
            &sNdsNativeStageOwnerExecution.runs[run_index];

        NDS_TASK26_HASH_FIELD((uintptr_t)prepared->texture_entry);
        NDS_TASK26_HASH_FIELD(prepared->texture_name);
        NDS_TASK26_HASH_FIELD(prepared->texture_params);
        NDS_TASK26_HASH_FIELD(prepared->texture_generation);
        NDS_TASK26_HASH_FIELD(prepared->poly_fmt);
        NDS_TASK26_HASH_FIELD(prepared->texture_width);
        NDS_TASK26_HASH_FIELD(prepared->texture_height);
        NDS_TASK26_HASH_FIELD(prepared->texture_format);
        NDS_TASK26_HASH_FIELD(prepared->textured);
        NDS_TASK26_HASH_FIELD(prepared->alpha_test);
        NDS_TASK26_HASH_FIELD(prepared->alpha_ref);
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_COUNT; i++)
    {
        u32 dense_index =
            sNdsNativeStageValidationCache.prepared_dense_indices[i];
        const NDSNativeStagePreparedDense *prepared =
            &sNdsNativeStagePreparedDense[dense_index];

        NDS_TASK26_HASH_FIELD(prepared->packed_color);
        NDS_TASK26_HASH_FIELD((u16)prepared->s);
        NDS_TASK26_HASH_FIELD((u16)prepared->t);
        NDS_TASK26_HASH_FIELD((u16)prepared->near_inside);
    }
    NDS_TASK26_HASH_FIELD(stats->sync_command_count);
    NDS_TASK26_HASH_FIELD(stats->state_command_count);
    NDS_TASK26_HASH_FIELD(stats->othermode_command_count);
    NDS_TASK26_HASH_FIELD(stats->ignored_state_command_count);
    NDS_TASK26_HASH_FIELD(stats->first_othermode_opcode);
    NDS_TASK26_HASH_FIELD(stats->first_othermode_w0);
    NDS_TASK26_HASH_FIELD(stats->first_othermode_w1);
    NDS_TASK26_HASH_FIELD(stats->othermode_h);
    NDS_TASK26_HASH_FIELD(stats->othermode_l);
    NDS_TASK26_HASH_FIELD(stats->geometry_mode);
    NDS_TASK26_HASH_FIELD(stats->geometry_clear_mask);
    NDS_TASK26_HASH_FIELD(stats->geometry_set_mask);
    NDS_TASK26_HASH_FIELD(stats->geometry_command_count);
    NDS_TASK26_HASH_FIELD(stats->texture_mask);
    NDS_TASK26_HASH_FIELD(stats->texture_command_count);
    NDS_TASK26_HASH_FIELD(stats->texture_state_flags);
    NDS_TASK26_HASH_FIELD(stats->texture_image);
    NDS_TASK26_HASH_FIELD(stats->texture_combine_w0);
    NDS_TASK26_HASH_FIELD(stats->texture_combine_w1);
    NDS_TASK26_HASH_FIELD(stats->texture_combine_count);
    NDS_TASK26_HASH_FIELD(stats->color_command_count);
    NDS_TASK26_HASH_FIELD(stats->blend_color);
    NDS_TASK26_HASH_FIELD(stats->texture_source_hash1);
    NDS_TASK26_HASH_FIELD(stats->texture_source_hash2);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_valid);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_enabled);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_name);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_material_color);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_scale_s);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_scale_t);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_origin_s);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_origin_t);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_offset);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_vertex_flags);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_source_zbuffered);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_decal_depth);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_prim_depth);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_alpha_constant);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_poly_alpha);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_poly_fmt);
    NDS_TASK26_HASH_FIELD((u32)epoch_mask);
#undef NDS_TASK26_HASH_FIELD

    *out_hash_a = hash_a;
    *out_hash_b = hash_b;
    *out_field_count = field_count;
}
#endif
#endif

static void ndsRendererNativeStageAccountRun(
    NDSRendererStats *stats, u32 submit_class, u32 triangle_count)
{
    u32 reuse_count = (triangle_count != 0u) ? triangle_count - 1u : 0u;

    sNdsRendererHardwareSubmitClassCounts[submit_class] += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count += reuse_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count += reuse_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices += triangle_count * 3u;
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        sNdsRendererRuntimeFrameSummary.raw_current_candidate_count +=
            triangle_count;
        stats->hardware_zbuffer_triangle_count += triangle_count;
    }
    else
    {
        sNdsRendererRuntimeFrameSummary.projected_submit_fallback_count +=
            triangle_count;
        stats->hardware_projected_depth_triangle_count += triangle_count;
    }
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    sNdsRendererHardwareSubmitted = TRUE;
}

static inline void ndsRendererNativeStageWriteColor(u32 value)
{
    ndsRendererHardwareWriteColorWord(value);
}

static inline void ndsRendererNativeStageWriteTexCoord(u32 value)
{
    ndsRendererHardwareWriteTexCoordWord(value);
}

static inline void ndsRendererNativeStageWriteVertex16(u32 xy, u32 z)
{
    ndsRendererHardwareWriteVertex16Words(xy, z);
}

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
static u32 ndsRendererBenchmarkSegment0NormalizeAssetAddress(
    u32 address, u32 *valid)
{
    u32 asset_index;

    if (address == 0u)
    {
        return 0u;
    }
    for (asset_index = 0u;
         asset_index < NDS_RENDERER_NATIVE_STAGE_ASSET_COUNT;
         asset_index++)
    {
        u32 base = (u32)(uintptr_t)
            sNdsRendererBenchmarkSegment0AssetBases[asset_index];
        u32 offset = address - base;

        if ((base != 0u) && (address >= base) &&
            (offset < sNdsNativeStageAssets[asset_index].payload_size) &&
            (offset < 0x01000000u))
        {
            return ((asset_index + 1u) << 24) | offset;
        }
    }
    *valid = FALSE;
    return 0xffffffffu;
}

static void ndsRendererBenchmarkSegment0HashTextureKey(
    const NDSRendererHardwareTextureCacheEntry *entry,
    u32 *valid,
    u32 *image,
    u32 *tlut,
    u32 *texel1,
    u32 *out_hash_a,
    u32 *out_hash_b)
{
    NDSRendererHardwareTextureKey normalized;
    const u32 *words;
    u32 hash_a = 2166136261u;
    u32 hash_b = 0x9e3779b9u;
    u32 i;

    ndsRendererHardwareEntryCopyKey(entry, &normalized);
    normalized.image = ndsRendererBenchmarkSegment0NormalizeAssetAddress(
        normalized.image, valid);
    normalized.tlut_image =
        ndsRendererBenchmarkSegment0NormalizeAssetAddress(
            normalized.tlut_image, valid);
    normalized.texel1_image =
        ndsRendererBenchmarkSegment0NormalizeAssetAddress(
            normalized.texel1_image, valid);
    *image = normalized.image;
    *tlut = normalized.tlut_image;
    *texel1 = normalized.texel1_image;
    words = (const u32 *)&normalized;
    for (i = 0u; i < (sizeof(normalized) / sizeof(u32)); i++)
    {
        hash_a = ndsRendererBenchmarkSinkHashWordA(hash_a, words[i]);
        hash_b = ndsRendererBenchmarkSinkHashWordB(hash_b, words[i]);
    }
    *out_hash_a = hash_a;
    *out_hash_b = hash_b;
}

static void ndsRendererBenchmarkSegment0ArmRun(
    u32 run_offset,
    const NDSNativeStageRun *run,
    const NDSNativeStagePreparedRun *prepared)
{
    const NDSRendererHardwareTextureCacheEntry *entry;
    const NDSNativeStageTextureEpoch *epoch;
    u32 image = 0u;
    u32 tlut = 0u;
    u32 texel1 = 0u;
    u32 key_hash_a = 0u;
    u32 key_hash_b = 0u;
    u32 valid = TRUE;

    if ((run_offset >= NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT) ||
        (run == NULL) || (prepared == NULL) ||
        (run->texture_epoch >= NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT))
    {
        sNdsRendererBenchmarkSegment0SinkArmFaults++;
        sNdsRendererBenchmarkSegment0TextureValid = FALSE;
        return;
    }
    epoch = &sNdsNativeStageTextureEpochs[run->texture_epoch];
    entry = prepared->texture_entry;
    if (prepared->textured != 0u)
    {
        if (ndsRendererNativeStagePreparedTextureValid(prepared) == FALSE)
        {
            valid = FALSE;
        }
        if (entry != NULL)
        {
            ndsRendererBenchmarkSegment0HashTextureKey(
                entry, &valid, &image, &tlut, &texel1,
                &key_hash_a, &key_hash_b);
        }
    }
    else if ((entry != NULL) || (prepared->texture_name != 0u))
    {
        valid = FALSE;
    }
    sNdsRendererBenchmarkSegment0TextureEpochPlus1 =
        (u32)run->texture_epoch + 1u;
    sNdsRendererBenchmarkSegment0TextureEpochSourceOffset =
        epoch->source_command_offset;
    sNdsRendererBenchmarkSegment0TextureEpochMetadata =
        (u32)epoch->asset_index |
        ((u32)epoch->policy_index << 8) |
        ((u32)epoch->material_event << 16) |
        ((u32)epoch->flags << 24);
    sNdsRendererBenchmarkSegment0TextureImage = image;
    sNdsRendererBenchmarkSegment0TextureTlut = tlut;
    sNdsRendererBenchmarkSegment0TextureTexel1 = texel1;
    sNdsRendererBenchmarkSegment0TextureKeyHashA = key_hash_a;
    sNdsRendererBenchmarkSegment0TextureKeyHashB = key_hash_b;
    sNdsRendererBenchmarkSegment0TextureDescriptor =
        ((u32)prepared->texture_format & 0xffu) |
        (((u32)prepared->texture_width & 0xfffu) << 8) |
        (((u32)prepared->texture_height & 0xfffu) << 20);
    sNdsRendererBenchmarkSegment0TextureFlags =
        (u32)prepared->textured |
        ((u32)prepared->alpha_test << 8) |
        ((u32)prepared->alpha_ref << 16);
    sNdsRendererBenchmarkSegment0TextureParams = prepared->texture_params;
    sNdsRendererBenchmarkSegment0TexturePolyFmt = prepared->poly_fmt;
    sNdsRendererBenchmarkSegment0TextureBinding = run->binding_index;
    sNdsRendererBenchmarkSegment0TextureValid = valid;
    gNdsRendererBenchmarkSegment0RunRawTextureName[run_offset] =
        prepared->texture_name;
    gNdsRendererBenchmarkSegment0RunTextureEpochPlus1[run_offset] =
        sNdsRendererBenchmarkSegment0TextureEpochPlus1;
    gNdsRendererBenchmarkSegment0RunTextureImage[run_offset] = image;
    gNdsRendererBenchmarkSegment0RunTextureTlut[run_offset] = tlut;
    gNdsRendererBenchmarkSegment0RunTextureKeyHashA[run_offset] = key_hash_a;
    gNdsRendererBenchmarkSegment0RunTextureKeyHashB[run_offset] = key_hash_b;
    gNdsRendererBenchmarkSegment0RunTextureDescriptor[run_offset] =
        sNdsRendererBenchmarkSegment0TextureDescriptor;
    gNdsRendererBenchmarkSegment0RunTextureParams[run_offset] =
        prepared->texture_params;
    if (valid == FALSE)
    {
        sNdsRendererBenchmarkSegment0SinkArmFaults++;
    }
}

static void ndsRendererBenchmarkSegment0CheckpointRun(u32 run_offset)
{
    if (run_offset >= NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT)
    {
        sNdsRendererBenchmarkSegment0SinkArmFaults++;
        return;
    }
    gNdsRendererBenchmarkSegment0RunWords[run_offset] =
        sNdsRendererBenchmarkSegment0SinkWords;
    gNdsRendererBenchmarkSegment0RunHashA[run_offset] =
        sNdsRendererBenchmarkSegment0SinkHashA;
    gNdsRendererBenchmarkSegment0RunHashB[run_offset] =
        sNdsRendererBenchmarkSegment0SinkHashB;
}
#endif

#if NDS_TASK36_HW_COMPOSE
static s32 ndsRendererNativeStageTask36BindingIsRigid(u32 binding_index)
{
    return ((binding_index < NDS_NATIVE_STAGE_BINDING_COUNT) &&
            ((sNdsNativeStageOwnerExecution.rigid_binding_mask &
              ((u64)1u << binding_index)) != 0u)) ? TRUE : FALSE;
}

static s32 ndsRendererNativeStageTask36BuildWorld(
    u32 binding_index, u32 coordinate_shift, m4x4 *hardware)
{
    NDSRendererMatrix20p12 scaled;
    u32 row;
    u32 col;

    if ((binding_index >= NDS_NATIVE_STAGE_BINDING_COUNT) ||
        (sNdsNativeStageOwnerExecution.binding_world == NULL) ||
        (hardware == NULL))
    {
        return FALSE;
    }
    scaled = sNdsNativeStageOwnerExecution.binding_world[binding_index];
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            s64 value = (s64)scaled.m[row][col] << coordinate_shift;

            if ((value > INT_MAX) || (value < INT_MIN))
            {
                return FALSE;
            }
            scaled.m[row][col] = (s32)value;
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        scaled.m[3][col] = ndsRendererRoundShiftS32Signed(
            scaled.m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    ndsRendererCopyMtx20p12ToM4x4(&scaled, hardware);
    return TRUE;
}

static u32 ndsRendererNativeStageTask36TriangleShift(
    const NDSNativeStageRun *run, u32 triangle_offset)
{
    u32 first_corner = (u32)run->first_corner + triangle_offset * 3u;
    u32 coordinate_shift = 0u;
    u32 corner_offset;

    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];
        u32 vertex_shift = sNdsNativeStageVertices[dense_index].packed_cache_shift >>
            NDS_NATIVE_STAGE_COORDINATE_SHIFT;

        if (vertex_shift > coordinate_shift)
        {
            coordinate_shift = vertex_shift;
        }
    }
    return coordinate_shift;
}

static s32 ndsRendererNativeStageTask36BeginSegment(void)
{
    m4x4 projection_hardware;
    m4x4 camera_hardware;

    if ((sNdsNativeStageOwnerExecution.projection == NULL) ||
        (sNdsNativeStageOwnerExecution.camera_modelview == NULL) ||
        (sNdsNativeStageOwnerExecution.binding_world == NULL))
    {
        return FALSE;
    }
    ndsRendererHardwareEndBatch();
    ndsRendererCopyMtx20p12ToM4x4(
        sNdsNativeStageOwnerExecution.projection, &projection_hardware);
    ndsRendererNativeBuildHierarchyHardwareAffine(
        sNdsNativeStageOwnerExecution.camera_modelview, &camera_hardware);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hardware);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadMatrix4x4(&camera_hardware);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE;
    sNdsRendererHardwareMatrixGeneration = ndsRendererNextMatrixGeneration();
    sNdsRendererHardwareMatrixLoaded = TRUE;
    sNdsNativeStageOwnerExecution.task36_binding = UINT_MAX;
    sNdsNativeStageOwnerExecution.task36_coordinate_shift = UINT_MAX;
    sNdsNativeStageOwnerExecution.task36_local_pushed = FALSE;
    sNdsNativeStageOwnerExecution.task36_segment_active = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36CameraLoadCount++;
#endif
    return TRUE;
}

static s32 ndsRendererNativeStageTask36EnsureWorld(
    u32 binding_index, u32 coordinate_shift)
{
    m4x4 world_hardware;

    if ((sNdsNativeStageOwnerExecution.task36_segment_active == FALSE) ||
        (ndsRendererNativeStageTask36BindingIsRigid(binding_index) == FALSE) ||
        (ndsRendererNativeStageTask36BuildWorld(
             binding_index, coordinate_shift, &world_hardware) == FALSE))
    {
        return FALSE;
    }
    if ((sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE) &&
        (sNdsNativeStageOwnerExecution.task36_binding == binding_index) &&
        (sNdsNativeStageOwnerExecution.task36_coordinate_shift ==
         coordinate_shift))
    {
        return TRUE;
    }
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    if (sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE)
    {
        glPopMatrix(1);
    }
    glPushMatrix();
    glMultMatrix4x4(&world_hardware);
    sNdsNativeStageOwnerExecution.task36_binding = binding_index;
    sNdsNativeStageOwnerExecution.task36_coordinate_shift = coordinate_shift;
    sNdsNativeStageOwnerExecution.task36_local_pushed = TRUE;
    sNdsRendererHardwareMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE;
    sNdsRendererHardwareMatrixGeneration = ndsRendererNextMatrixGeneration();
    sNdsRendererHardwareMatrixLoaded = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    {
        u64 binding_bit = (u64)1u << binding_index;

        if ((sNdsNativeStageOwnerExecution.task36_seen_binding_mask &
             binding_bit) == 0u)
        {
            sNdsNativeStageOwnerExecution.task36_seen_binding_mask |=
                binding_bit;
            gNdsRendererTask36HardwareComposedDObjCount++;
        }
    }
    gNdsRendererTask36WorldMultCount++;
#endif
    return TRUE;
}

#if NDS_TASK51_STAGE_NATIVE
/* Task 51: emit the baked constant world matrix for a non-rigid stage binding
 * via MTX_MULT4x3 under the once-loaded camera view, instead of the per-frame
 * CPU projection x view x model compose in ndsRendererNativeStageLoadNoZMatrix.
 *
 * The baked matrices come from the host generator
 * (sNdsNativeStageBakedWorldMatrices, NDSRendererMatrix20p12 s20.12 row-major
 * m[row][col] with translation in m[3][0..2]). The DS MTX_MULT4x3 command
 * consumes a column-major 4x3 (4 columns of 3 elements: the decomp's
 * Matrix4x3_FromTranslation puts the identity diagonal at indices 0,4,8 and
 * translation at 9,10,11), so the conversion transposes the rotation and
 * places translation in column 3. The Task 49 differ Tier-2 (<= 1.0 screen-px)
 * gates that this matches the CPU-composed oracle. Mirrors Task 36's
 * EnsureWorld (PUSH + mult) but uses the 12-word 4x3 and the generator-baked
 * constant instead of a runtime recomputed scaled world. */
static s32 ndsRendererNativeStageTask51EnsureWorld(u32 binding_index)
{
    const s32 *baked;
    m4x3 world_hardware;

    if ((binding_index >= NDS_NATIVE_STAGE_BAKED_WORLD_COUNT) ||
        (sNdsNativeStageOwnerExecution.task36_segment_active == FALSE))
    {
        return FALSE;
    }
    /* Reuse Task 36's segment bracket: projection + view were loaded once at
     * BeginSegment, so we only need to push the per-binding world onto the
     * modelview stack. */
    if (sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE)
    {
        glPopMatrix(1);
    }
    baked = sNdsNativeStageBakedWorldMatrices[binding_index];
    /* Column-major 4x3: col c, row r -> m[c*3 + r]. baked[row*4+col] is
     * row-major m[row][col]. Translation m[3][0..2] -> column 3. */
    world_hardware.m[0] = baked[0];  /* col0,row0 = m[0][0] */
    world_hardware.m[1] = baked[4];  /* col0,row1 = m[1][0] */
    world_hardware.m[2] = baked[8];  /* col0,row2 = m[2][0] */
    world_hardware.m[3] = baked[1];  /* col1,row0 = m[0][1] */
    world_hardware.m[4] = baked[5];  /* col1,row1 = m[1][1] */
    world_hardware.m[5] = baked[9];  /* col1,row2 = m[2][1] */
    world_hardware.m[6] = baked[2];  /* col2,row0 = m[0][2] */
    world_hardware.m[7] = baked[6];  /* col2,row1 = m[1][2] */
    world_hardware.m[8] = baked[10]; /* col2,row2 = m[2][2] */
    world_hardware.m[9] = baked[12]; /* col3,row0 = m[3][0] (translate x) */
    world_hardware.m[10] = baked[13];/* col3,row1 = m[3][1] (translate y) */
    world_hardware.m[11] = baked[14];/* col3,row2 = m[3][2] (translate z) */
    glPushMatrix();
    glMultMatrix4x3(&world_hardware);
    sNdsNativeStageOwnerExecution.task36_binding = binding_index;
    sNdsNativeStageOwnerExecution.task36_coordinate_shift = 0u;
    sNdsNativeStageOwnerExecution.task36_local_pushed = TRUE;
    sNdsRendererHardwareMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE;
    sNdsRendererHardwareMatrixGeneration = ndsRendererNextMatrixGeneration();
    sNdsRendererHardwareMatrixLoaded = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    {
        u64 binding_bit = (u64)1u << binding_index;

        if ((sNdsNativeStageOwnerExecution.task36_seen_binding_mask &
             binding_bit) == 0u)
        {
            sNdsNativeStageOwnerExecution.task36_seen_binding_mask |=
                binding_bit;
            gNdsRendererTask36HardwareComposedDObjCount++;
        }
    }
    gNdsRendererTask36WorldMultCount++;
#endif
    return TRUE;
}
#endif

static void ndsRendererNativeStageTask36LoadNoZProjection(s16 projected_z)
{
    NDSRendererMatrix20p12 projection =
        *sNdsNativeStageOwnerExecution.projection;
    m4x4 projection_hardware;
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        projection.m[row][2] = (s32)ndsRendererRoundShiftS64(
            (s64)projection.m[row][3] * projected_z, 12u);
    }
    ndsRendererCopyMtx20p12ToM4x4(&projection, &projection_hardware);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hardware);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    ndsRendererProfileRecordMatrixLoad();
#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererM3Phase0NoZMatrixCount++;
#endif
}

static void ndsRendererNativeStageTask36EndSegment(void)
{
    if (sNdsNativeStageOwnerExecution.task36_segment_active == FALSE)
    {
        return;
    }
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    if (sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE)
    {
        glPopMatrix(1);
    }
    sNdsNativeStageOwnerExecution.task36_local_pushed = FALSE;
    sNdsNativeStageOwnerExecution.task36_segment_active = FALSE;
    sNdsRendererHardwareMatrixLoaded = FALSE;
}
#endif

static s32 NDS_R2_ITCM_PACK2_CODE ndsRendererNativeStageBeginRun(
    const NDSNativeStageRun *native_run,
    const NDSNativeStagePreparedRun *run,
    u32 submit_class,
    u32 segment_owner,
    NDSRendererStats *stats,
    u32 replay)
{
    u32 poly_fmt;

    if ((native_run == NULL) || (stats == NULL) || (run == NULL))
    {
        return FALSE;
    }
    /* R2-07 leg A. This is the defensive last gate at the point of use, and it
     * is kept -- but the caller has already proved the table this run belongs
     * to (Commit proves the stage table, the replay entry proves the replay
     * table), so while that certificate is current the per-run proof is a
     * second reading of the same two cold arrays for an answer already known.
     * `replay` picks the certificate because it picks the table `run` points
     * into. No certificate current -> the original proof, unchanged. */
    if (((replay == FALSE) ?
             ndsRendererNativeStagePreparedTextureProofCurrent() :
             ndsRendererTask36ReplayTextureProofCurrent()) == FALSE)
    {
        if (ndsRendererNativeStagePreparedTextureValid(run) == FALSE)
        {
            return FALSE;
        }
    }
    poly_fmt = run->poly_fmt;

    /* Dream Land's four static layer owners are closed front-facing stage
     * surfaces.  Keep actor owners (Whispy and flowers) two-sided. */
    if ((sNdsNativeStagePacketActive->gkind == NDS_NATIVE_STAGE_GKIND_PUPUPU) &&
        (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) &&
        (segment_owner < NDS_RENDERER_NATIVE_STAGE_STATIC_OWNER_COUNT) &&
        ((poly_fmt & POLY_CULL_NONE) == POLY_CULL_NONE))
    {
        poly_fmt &= ~((u32)POLY_CULL_NONE);
        poly_fmt |= POLY_CULL_BACK;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    u32 t103_phase = cpuGetTiming();
#endif
    ndsRendererHardwareEndBatch();
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103BeginEndBatchTicks += cpuGetTiming() - t103_phase;
#endif
    if (replay == FALSE)
    {
#if NDS_TASK103_STAGE_RUN_PHASE
    u32 t103_mtx_start = cpuGetTiming();
    u32 t103_mtx_class = 3u;
#endif
#if NDS_TASK36_HW_COMPOSE
    if (ndsRendererNativeStageTask36BindingIsRigid(
            native_run->binding_index) != FALSE)
    {
        u32 coordinate_shift =
            (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
                1u :
            (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ?
                ndsRendererNativeStageTask36TriangleShift(native_run, 0u) : 0u;

        if ((sNdsNativeStageOwnerExecution.task36_segment_active == FALSE) &&
            (ndsRendererNativeStageTask36BeginSegment() == FALSE))
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
            return FALSE;
        }
        if (ndsRendererNativeStageTask36EnsureWorld(
                native_run->binding_index, coordinate_shift) == FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
            return FALSE;
        }
#if NDS_TASK103_STAGE_RUN_PHASE
        t103_mtx_class = 0u;
#endif
    }
    else
#endif
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        /* Each run owns its matrix. A shared literal generation would let
         * the hardware matrix cache reuse the preceding binding's transform. */
        ndsRendererLoadHardwareRawComposedMatrix(
            &sNdsNativeStageOwnerExecution.binding_composed[
                native_run->binding_index], ndsRendererNextMatrixGeneration());
#if NDS_TASK103_STAGE_RUN_PHASE
        t103_mtx_class = 1u;
#endif
    }
    else if (submit_class ==
             NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
    {
        NDSRendererMatrix20p12 projection;
        NDSRendererMatrix20p12 modelview;

        if (ndsRendererBuildShiftedRawHardwareMatrix(
                &sNdsNativeStageOwnerExecution.binding_composed[
                    native_run->binding_index], &modelview, 1u) == FALSE)
        {
            return FALSE;
        }
        ndsRendererMtxIdentity20p12(&projection);
        ndsRendererLoadHardwareMatrixPair(
            &projection, &modelview, NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED,
            ndsRendererNextMatrixGeneration(), TRUE);
#if NDS_TASK103_STAGE_RUN_PHASE
        t103_mtx_class = 2u;
#endif
    }
    else
    {
        /* The actor runs land here. Every triangle of such a run then goes
         * through Task 51's EnsureWorld + LoadNoZProjection, which replaces
         * both matrices -- so measure whether this load survives to a vertex
         * at all. */
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103BeginMtxTicks[t103_mtx_class] += cpuGetTiming() - t103_mtx_start;
    gNdsTask103BeginMtxCount[t103_mtx_class]++;
#endif
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    t103_phase = cpuGetTiming();
#endif
    glEnable(GL_TEXTURE_2D);
    if (run->textured != 0u)
    {
        run->texture_entry->last_used_frame =
            sNdsRendererHardwareFrameSerial + 1u;
        run->texture_entry->params = run->texture_params;
        ndsRendererHardwareBindTextureName(stats, run->texture_name);
        ndsRendererHardwareApplyTextureParams(run->texture_params);
        sNdsRendererHardwareActiveTextureEntry = run->texture_entry;
        if (run->texture_entry->pinned != 0u)
        {
            ndsRendererHardwareRecordBattleStaticTextureHit(
                run->texture_entry);
        }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = run->texture_format;
        stats->hardware_texture_width = run->texture_width;
        stats->hardware_texture_height = run->texture_height;
    }
    else
    {
        ndsRendererHardwareBindNoTexture(NULL);
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103BeginTexTicks += cpuGetTiming() - t103_phase;
    t103_phase = cpuGetTiming();
#endif
    if (run->alpha_test != 0u)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(run->alpha_ref);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }
    glDisable(GL_FOG);
    ndsRendererHardwareSetPolyFmt(poly_fmt);
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103BeginTailTicks += cpuGetTiming() - t103_phase;
#endif
    if (replay == FALSE)
    {
        glBegin(GL_TRIANGLE);
        ndsRendererProfileRecordBatchBegin();
        sNdsRendererHardwareTriangleBatchOpen = TRUE;
        sNdsRendererHardwareTriangleBatchTextured = run->textured;
        sNdsRendererHardwareTriangleBatchTextureName = run->texture_name;
        sNdsRendererHardwareTriangleBatchPolyFmt = poly_fmt;
        sNdsRendererHardwareTriangleBatchAlphaKey = run->alpha_test;
        sNdsRendererHardwareTriangleBatchFogKey = 0u;
        sNdsRendererHardwareTriangleBatchMatrixMode =
#if NDS_TASK36_HW_COMPOSE
            (ndsRendererNativeStageTask36BindingIsRigid(
                 native_run->binding_index) != FALSE) ?
                NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE :
#endif
            ((submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) ||
             (submit_class ==
              NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)) ?
                NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED :
                NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY;
        sNdsRendererHardwareTriangleBatchMatrixGeneration =
#if NDS_TASK36_HW_COMPOSE
            (ndsRendererNativeStageTask36BindingIsRigid(
                 native_run->binding_index) != FALSE) ?
                sNdsRendererHardwareMatrixGeneration :
#endif
            (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) ?
                1u :
            (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
                2u : 0u;
    }
    return TRUE;
}

#if NDS_TASK36_HW_COMPOSE == 2
#if NDS_TASK103_STAGE_RUN_PHASE
/* Task 103. The stage bucket is ~89% fixed (Task 99) and Task 100 refuted the
 * last proposed currency for that remainder, so ~331,300 ticks/frame are still
 * unattributed -- ~6,135 over each of the 54 runs. These four counters split
 * the replay run into its three spans and normalise by run and word count, so
 * the two live explanations separate without removing a single run. Removing
 * runs is what Task 99 arm C did, and it disarmed the capture-once replay for
 * +109,888.
 *
 * Cumulative, not per-frame: sample with a two-stop GDB delta over a known
 * window, the way scripts/census-aobj-chain-layout.ps1 does. Lab only. */
volatile u32 gNdsTask103BeginTicks;
volatile u32 gNdsTask103PushTicks;
volatile u32 gNdsTask103TailTicks;
volatile u32 gNdsTask103RunCount;
volatile u32 gNdsTask103WordCount;
/* E1. The first run of this census found the whole replay function is only
 * 59,553 of a ~370,000 stage bucket, so 84% of the stage's cost is outside the
 * path five tasks have been optimising. These cover the other branch of the
 * same loop -- the generic emit for runs the replay does not serve -- and the
 * whole loop body, so "inside the run loop" and "outside it" separate too. */
volatile u32 gNdsTask103GenericTicks;
volatile u32 gNdsTask103GenericRunCount;
volatile u32 gNdsTask103GenericTriangles;
volatile u32 gNdsTask103IterTicks;
volatile u32 gNdsTask103IterCount;
/* E7 (R2-02 E3 sizing). GenericTicks aggregates all five replay-ineligible
 * segments into one number, and the two candidate cuts want different halves
 * of it: segment 4 is a *static* layer owner that is simply absent from
 * NDS_TASK36_REPLAY_SEGMENT_MASK, while 1/2/3/6 are the actor owners the switch
 * plan puts on a specialized update+draw path. GenericBeginTicks splits the
 * per-run matrix/state head from the vertex emit, because 21 runs carrying only
 * ~103 triangles cannot be paying 3,168 ticks each for the triangles. */
volatile u32 gNdsTask103GenericBeginTicks;
/* R2-04 E0. Generic emit is 21 runs for 103 triangles, and BeginRun's fixed
 * state sequence is 1,157 of the 3,129 ticks each run costs. If consecutive
 * runs carry identical state, that sequence is being re-issued for nothing and
 * the runs could be merged into one batch -- which is the only way to attack a
 * per-run cost when the run count is the problem.
 *
 * Count it before designing it. Redundant means every field BeginRun would
 * write matches the previous generic run: submit class, poly_fmt, texture
 * identity and params, alpha test, and the matrix binding. */
volatile u32 gNdsTask103GenericSegTicks[NDS_NATIVE_STAGE_MAX_SEGMENT_COUNT];
volatile u32 gNdsTask103GenericSegRuns[NDS_NATIVE_STAGE_MAX_SEGMENT_COUNT];
volatile u32 gNdsTask103GenericSegTris[NDS_NATIVE_STAGE_MAX_SEGMENT_COUNT];
/* E2's counters live in reloc_backend_renderer_dl.c, next to the call site
 * they wrap.
 *
 * E5. E4 put 160,588 ticks/frame -- 41% of the stage bucket and 12% of all
 * frame work -- inside ndsRendererPrepareNativeStageOwner at one call per
 * frame. These split it into the steps it actually runs: the one-shot topology
 * validation, the per-segment stats/traversal reset, the Task 36 prepared-
 * segment reuse check, and the per-run state-span and prepare-run work for the
 * segments that reuse misses. */
volatile u32 gNdsTask103OwnValidateTicks;
volatile u32 gNdsTask103OwnInitTicks;
volatile u32 gNdsTask103OwnInitCount;
volatile u32 gNdsTask103OwnReuseTicks;
volatile u32 gNdsTask103OwnReuseCount;
volatile u32 gNdsTask103OwnReuseMissCount;
volatile u32 gNdsTask103OwnStateSpanTicks;
volatile u32 gNdsTask103OwnStateSpanCount;
volatile u32 gNdsTask103OwnPrepareRunTicks;
volatile u32 gNdsTask103OwnPrepareRunCount;
/* E6's four counters are defined ahead of ndsRendererNativeStagePrepareRun. */
#endif
static s32 NDS_RENDERER_FAST_RUN_CODE NDS_TASK82_ITCM_CODE
ndsRendererTask36ReplayRun(
    u32 run_index,
    const NDSNativeStageRun *native_run,
    u32 segment_owner,
    NDSRendererStats *stats)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;
    const NDSRendererTask36ReplayRun *run;
    const u32 *words;
    u32 i;
#if NDS_TASK103_STAGE_RUN_PHASE
    u32 task103_t0;
    u32 task103_t1;
    u32 task103_t2;
#endif

    if ((run_index >= NDS_NATIVE_STAGE_RUN_COUNT) ||
        (native_run == NULL) || (stats == NULL))
    {
        return FALSE;
    }
    run = &owner->runs[run_index];
    if ((run->valid == FALSE) || (run->word_count == 0u) ||
        ((u32)run->word_offset + (u32)run->word_count > owner->word_count))
    {
        return FALSE;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    task103_t0 = cpuGetTiming();
#endif
    if (ndsRendererNativeStageBeginRun(
            native_run, &run->prepared, native_run->submit_class,
            segment_owner, stats, TRUE) == FALSE)
    {
        return FALSE;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    task103_t1 = cpuGetTiming();
#endif
    words = &owner->words[run->word_offset];
#if NDS_R2_STAGE_DMA
    /* R2-02 E2. The replay is a flat push of a captured GX command stream out
     * of a 32-byte-aligned buffer in main RAM -- roughly 4,200 words a frame
     * across the 21 runs, which the CPU loop drags through the data cache one
     * word at a time. Task 103 timed that at 9.51 ticks per word. GXFIFO DMA is
     * what the hardware provides for exactly this: it reads main RAM directly,
     * so the words stop being cache-line fills, and it self-throttles on FIFO
     * space instead of stalling the core on each store.
     *
     * The switch plan's §3.3 is explicit that a traffic optimization is judged
     * by cache lines that stopped being touched, and this stops ~16 KB/frame of
     * them.
     *
     * Coherency is already satisfied: the capture path DC_FlushRange()s
     * owner->words when it completes, and replay never writes the buffer.
     *
     * Channel 0 and this exact idiom are what libnds glCallList uses. Only
     * channel 0 is polled, not all four: a DMA on another channel cannot
     * interleave GX commands unless it is also in GXFIFO mode, and nothing else
     * here is. */
    if (run->word_count != 0u)
    {
        while ((DMA_CR(0) & DMA_BUSY) != 0u) { }
        DMA_SRC(0) = (u32)words;
        DMA_DEST(0) = (u32)&GFX_FIFO;
        DMA_CR(0) = DMA_FIFO | run->word_count;
        while ((DMA_CR(0) & DMA_BUSY) != 0u) { }
    }
    (void)i;
#else
    for (i = 0u; i < run->word_count; i++)
    {
        GFX_FIFO = words[i];
    }
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
    task103_t2 = cpuGetTiming();
#endif
    /* Report the stack state the stream actually left, not TRUE unconditionally:
     * ndsRendererNativeStageTask36EndSegment pops on this flag, and an actor
     * segment's stream never pushed. */
    sNdsNativeStageOwnerExecution.task36_local_pushed = run->local_pushed;
    sNdsNativeStageOwnerExecution.task36_binding = native_run->binding_index;
    sNdsRendererHardwareMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE;
    sNdsRendererHardwareMatrixGeneration = ndsRendererNextMatrixGeneration();
    sNdsRendererHardwareMatrixLoaded = TRUE;
    ndsRendererProfileRecordBatchBegin();
    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = run->prepared.textured;
    sNdsRendererHardwareTriangleBatchTextureName = run->prepared.texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = run->prepared.poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = run->prepared.alpha_test;
    sNdsRendererHardwareTriangleBatchFogKey = 0u;
    sNdsRendererHardwareTriangleBatchMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE;
    sNdsRendererHardwareTriangleBatchMatrixGeneration =
        sNdsRendererHardwareMatrixGeneration;
    ndsRendererHardwareEndBatch();
#if NDS_TASK103_STAGE_RUN_PHASE
    /* The tail is the bookkeeping stores plus ndsRendererHardwareEndBatch. It
     * closes here rather than at the return so the profile-level-1 counters
     * below, which the tick-HUD ROM does not compile, can never enter it. */
    gNdsTask103BeginTicks += task103_t1 - task103_t0;
    gNdsTask103PushTicks += task103_t2 - task103_t1;
    gNdsTask103TailTicks += cpuGetTiming() - task103_t2;
    gNdsTask103RunCount++;
    gNdsTask103WordCount += run->word_count;
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
    {
        u64 binding_bit = (u64)1u << native_run->binding_index;

        if ((sNdsNativeStageOwnerExecution.task36_seen_binding_mask &
             binding_bit) == 0u)
        {
            sNdsNativeStageOwnerExecution.task36_seen_binding_mask |=
                binding_bit;
            gNdsRendererTask36HardwareComposedDObjCount++;
        }
    }
    gNdsRendererTask36WorldMultCount += run->world_mult_count;
    gNdsRendererTask36ReplayRunCount++;
    gNdsRendererTask36ReplayWordCount += run->word_count;
#endif
    return TRUE;
}
#endif

static inline void ndsRendererNativeStageEmitVertex(
    const NDSNativeStageDenseVertex *dense,
    const NDSNativeStagePreparedDense *prepared,
    const NDSNativeStagePreparedRun *run,
    u32 submit_class)
{
    ndsRendererNativeStageWriteColor(prepared->packed_color);
    if (run->textured != 0u)
    {
        ndsRendererNativeStageWriteTexCoord(
            (u32)(u16)prepared->s | ((u32)(u16)prepared->t << 16));
    }
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        ndsRendererNativeStageWriteVertex16(
            (u32)(u16)((s32)dense->x *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) |
            ((u32)(u16)((s32)dense->y *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) << 16),
            (u16)((s32)dense->z *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
    }
    else
    {
        ndsRendererNativeStageWriteVertex16(
            (u32)(u16)(ndsRendererNativeStageVertexShift(dense->x, 1u) *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) |
            ((u32)(u16)(ndsRendererNativeStageVertexShift(dense->y, 1u) *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) << 16),
            (u16)(ndsRendererNativeStageVertexShift(dense->z, 1u) *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
    }
}

static void ndsRendererNativeStageSetNoZColumn(
    NDSRendererMatrix20p12 *matrix,
    s16 projected_z)
{
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        matrix->m[row][2] = (s32)ndsRendererRoundShiftS64(
            (s64)matrix->m[row][3] * projected_z, 12u);
    }
}

static void NDS_TASK82_ITCM_CODE ndsRendererNativeStageLoadNoZMatrix(
    u32 binding_index,
    u32 coordinate_shift,
    s16 projected_z)
{
    NDSRendererMatrix20p12 matrix;
    m4x4 hardware;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 matrix_start = ndsRendererM3Phase0Tick();
#endif

    if (coordinate_shift == 0u)
    {
        ndsRendererBuildRawHardwareMatrix(
            &sNdsNativeStageOwnerExecution.binding_composed[binding_index],
            &matrix);
    }
    else
    {
        (void)ndsRendererBuildShiftedRawHardwareMatrix(
            &sNdsNativeStageOwnerExecution.binding_composed[binding_index],
            &matrix, coordinate_shift);
    }
    ndsRendererNativeStageSetNoZColumn(&matrix, projected_z);
    ndsRendererCopyMtx20p12ToM4x4(&matrix, &hardware);
    glLoadMatrix4x4(&hardware);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixLoaded = FALSE;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0FinishSpan(
        &gNdsRendererM3Phase0NoZMatrixTicks, matrix_start);
    gNdsRendererM3Phase0NoZMatrixCount++;
#endif
}

static void NDS_R2_ITCM_PACK2_CODE ndsRendererNativeStageEmitNoZVertex(
    const NDSNativeStageDenseVertex *dense,
    const NDSNativeStagePreparedDense *prepared,
    const NDSNativeStagePreparedRun *run,
    u32 coordinate_shift)
{
    s32 x = ndsRendererNativeStageVertexShift(dense->x, coordinate_shift);
    s32 y = ndsRendererNativeStageVertexShift(dense->y, coordinate_shift);
    s32 z = ndsRendererNativeStageVertexShift(dense->z, coordinate_shift);

    ndsRendererNativeStageWriteColor(prepared->packed_color);
    if (run->textured != 0u)
    {
        ndsRendererNativeStageWriteTexCoord(
            (u32)(u16)prepared->s | ((u32)(u16)prepared->t << 16));
    }
    ndsRendererNativeStageWriteVertex16(
        (u32)(u16)(x * 16) | ((u32)(u16)(y * 16) << 16),
        (u16)(z * 16));
}

static void ndsRendererNativeStageEmitClippedVertex(
    const NDSRendererProjectedClipVertex *vertex,
    const NDSNativeStagePreparedRun *run,
    s16 projected_z)
{
    NDSRendererMatrix20p12 matrix;
    m4x4 hardware;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 matrix_start = ndsRendererM3Phase0Tick();
#endif

    memset(&matrix, 0, sizeof(matrix));
    matrix.m[3][0] = ndsRendererRoundShiftS32Signed(vertex->clip.x, 8u);
    matrix.m[3][1] = ndsRendererRoundShiftS32Signed(vertex->clip.y, 8u);
    matrix.m[3][3] = ndsRendererRoundShiftS32Signed(vertex->clip.w, 8u);
    ndsRendererNativeStageSetNoZColumn(&matrix, projected_z);
    ndsRendererCopyMtx20p12ToM4x4(&matrix, &hardware);
    glLoadMatrix4x4(&hardware);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixLoaded = FALSE;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0FinishSpan(
        &gNdsRendererM3Phase0NoZMatrixTicks, matrix_start);
    gNdsRendererM3Phase0NoZMatrixCount++;
#endif

    ndsRendererNativeStageWriteColor(vertex->packed_color);
    if (run->textured != 0u)
    {
        ndsRendererNativeStageWriteTexCoord(
            (u32)(u16)vertex->s | ((u32)(u16)vertex->t << 16));
    }
    ndsRendererNativeStageWriteVertex16(0u, 0u);
}

static u32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererNativeStageEmitNearClippedTriangle(
    const NDSNativeStageRun *run,
    const NDSNativeStagePreparedRun *prepared_run,
    u32 triangle_offset,
    s16 projected_z)
{
    NDSRendererProjectedClipVertex input[3];
    NDSRendererProjectedClipVertex clipped[4];
    u32 corner_offset;
    u32 clipped_count;
    u32 fan_index;

    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[
            (u32)run->first_corner + triangle_offset * 3u + corner_offset];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];
        const NDSNativeStagePreparedDense *prepared =
            &sNdsNativeStagePreparedDense[dense_index];
        NDSRendererInputVertex source;

        ndsRendererNativeStageInputVertex(dense, &source);
        ndsRendererTransformVertex20p12(
            &sNdsNativeStageOwnerExecution.binding_composed[
                dense->matrix_binding],
            &source, &input[corner_offset].clip);
        input[corner_offset].s = prepared->s;
        input[corner_offset].t = prepared->t;
        input[corner_offset].packed_color = prepared->packed_color;
    }
    clipped_count = ndsRendererHardwareClipTriangleNearPlane(input, clipped);
    if (clipped_count < 3u)
    {
        ndsRendererProfileRecordNearPlaneTriangleReject();
        ndsRendererProfileRecordSubmitClass(NDS_RENDERER_HW_SUBMIT_REJECT);
        return 0u;
    }
    for (fan_index = 1u; fan_index + 1u < clipped_count; fan_index++)
    {
        ndsRendererNativeStageEmitClippedVertex(
            &clipped[0], prepared_run, projected_z);
        ndsRendererNativeStageEmitClippedVertex(
            &clipped[fan_index], prepared_run, projected_z);
        ndsRendererNativeStageEmitClippedVertex(
            &clipped[fan_index + 1u], prepared_run, projected_z);
    }
    return clipped_count - 2u;
}

static u32 __attribute__((noinline))
ndsRendererNativeStageEmitNoZTriangle(
    const NDSNativeStageRun *run,
    const NDSNativeStagePreparedRun *prepared_run,
    u32 triangle_offset,
    s16 projected_z)
{
    u32 first_corner = (u32)run->first_corner + triangle_offset * 3u;
    u32 inside_count = 0u;
    u32 corner_offset;
    u32 first_dense_index;
    u32 binding_index;
    u32 coordinate_shift = 0u;
    s32 one_binding = TRUE;

#if NDS_TASK36_HW_COMPOSE
    if (ndsRendererNativeStageTask36BindingIsRigid(run->binding_index) != FALSE)
    {
        coordinate_shift = ndsRendererNativeStageTask36TriangleShift(
            run, triangle_offset);
        for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
        {
            u32 dense_index =
                sNdsNativeStageCorners[first_corner + corner_offset];

            if (sNdsNativeStageVertices[dense_index].matrix_binding !=
                run->binding_index)
            {
#if NDS_RENDERER_PROFILE_LEVEL == 1
                gNdsRendererM3PostArmFailureCount++;
#endif
                return 0u;
            }
        }
        if (ndsRendererNativeStageTask36EnsureWorld(
                run->binding_index, coordinate_shift) == FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
            return 0u;
        }
        ndsRendererNativeStageTask36LoadNoZProjection(projected_z);
        for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
        {
            u32 dense_index =
                sNdsNativeStageCorners[first_corner + corner_offset];

            ndsRendererNativeStageEmitNoZVertex(
                &sNdsNativeStageVertices[dense_index],
                &sNdsNativeStagePreparedDense[dense_index],
                prepared_run, coordinate_shift);
        }
#if NDS_TASK103_STAGE_RUN_PHASE
        gNdsTask103NoZPath[0]++;
#endif
        return 1u;
    }
#endif
#if NDS_TASK51_STAGE_NATIVE
    /* Task 51: non-rigid bindings use the baked constant world matrix via
     * MTX_MULT4x3 instead of the per-frame CPU compose in LoadNoZMatrix.
     * Reuses Task 36's NoZ projection loader and segment bracket; only the
     * per-binding world-matrix emit changes (12-word 4x3 vs 16-word load). */
    if ((ndsRendererNativeStageTask36BindingIsRigid(run->binding_index) == FALSE) &&
        ((sNdsNativeStagePacketActive->camera_binding_mask &
          ((u64)1u << run->binding_index)) == 0u))
    {
        u32 t51_shift;

        for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
        {
            u32 dense_index =
                sNdsNativeStageCorners[first_corner + corner_offset];

            if (sNdsNativeStageVertices[dense_index].matrix_binding !=
                run->binding_index)
            {
                break;
            }
        }
        if (corner_offset == 3u)
        {
            s32 t51_ok;
#if NDS_TASK103_STAGE_RUN_PHASE
            u32 t103_span = cpuGetTiming();
#endif

            t51_shift = ndsRendererNativeStageTask36TriangleShift(
                run, triangle_offset);
            t51_ok = ndsRendererNativeStageTask51EnsureWorld(
                run->binding_index);
#if NDS_TASK103_STAGE_RUN_PHASE
            gNdsTask103NoZWorldTicks += cpuGetTiming() - t103_span;
#endif
            if (t51_ok != FALSE)
            {
#if NDS_TASK103_STAGE_RUN_PHASE
                t103_span = cpuGetTiming();
#endif
                ndsRendererNativeStageTask36LoadNoZProjection(projected_z);
#if NDS_TASK103_STAGE_RUN_PHASE
                gNdsTask103NoZProjTicks += cpuGetTiming() - t103_span;
#endif
                for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
                {
                    u32 dense_index =
                        sNdsNativeStageCorners[first_corner + corner_offset];

                    ndsRendererNativeStageEmitNoZVertex(
                        &sNdsNativeStageVertices[dense_index],
                        &sNdsNativeStagePreparedDense[dense_index],
                        prepared_run, t51_shift);
                }
#if NDS_TASK103_STAGE_RUN_PHASE
                gNdsTask103NoZPath[1]++;
#endif
                return 1u;
            }
        }
    }
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103NoZPath[2]++;
#endif

    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];

        inside_count +=
            (sNdsNativeStagePreparedDense[dense_index].near_inside != FALSE) ?
                1u : 0u;
    }
    if (inside_count == 0u)
    {
        ndsRendererProfileRecordNearPlaneTriangleReject();
        ndsRendererProfileRecordSubmitClass(NDS_RENDERER_HW_SUBMIT_REJECT);
        return 0u;
    }
    if (inside_count != 3u)
    {
        return ndsRendererNativeStageEmitNearClippedTriangle(
            run, prepared_run, triangle_offset, projected_z);
    }

    first_dense_index = sNdsNativeStageCorners[first_corner];
    binding_index = sNdsNativeStageVertices[first_dense_index].matrix_binding;
    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];
        u32 vertex_shift = dense->packed_cache_shift >>
            NDS_NATIVE_STAGE_COORDINATE_SHIFT;

        if (dense->matrix_binding != binding_index)
        {
            one_binding = FALSE;
        }
        if (vertex_shift > coordinate_shift)
        {
            coordinate_shift = vertex_shift;
        }
    }
    if (one_binding != FALSE)
    {
        ndsRendererNativeStageLoadNoZMatrix(
            binding_index, coordinate_shift, projected_z);
    }
    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];
        const NDSNativeStagePreparedDense *prepared =
            &sNdsNativeStagePreparedDense[dense_index];
        u32 vertex_shift = (one_binding != FALSE) ?
            coordinate_shift :
            (dense->packed_cache_shift >>
             NDS_NATIVE_STAGE_COORDINATE_SHIFT);

        if (one_binding == FALSE)
        {
            ndsRendererNativeStageLoadNoZMatrix(
                dense->matrix_binding, vertex_shift, projected_z);
        }
        ndsRendererNativeStageEmitNoZVertex(
            dense, prepared, prepared_run, vertex_shift);
    }
    return 1u;
}

#if NDS_DREAMLAND_CARD_CULL
/* Task 63 §5 — backdrop-card cull visualization.
 *
 * E0 measured that Dream Land's static stage has no geometry to reduce at full
 * material fidelity (9.1% ceiling, below the 15% gate). The only remaining
 * lever is deleting whole projected-no-Z backdrop cards, which is a visible
 * scenery loss and therefore the owner's decision, not the compiler's. This
 * instrument exists so that decision can be made by looking rather than by
 * reading a coverage table.
 *
 * One bit per entry in sNdsNativeStageRuns[] (54 runs, so two words). The
 * mask is baked at build time from NDS_DREAMLAND_CARD_CULL_MASK0/1 so that the
 * Task 36 replay stream is captured with the cull already applied -- poking it
 * over GDB after capture would be replayed away. It stays volatile so a GDB
 * poke plus a capture invalidation can still drive it interactively. At the
 * default 0 nothing is suppressed and the frame is identical to a flag=0
 * build, which is what makes the A/B trustworthy. Never shipped:
 * NDS_DREAMLAND_CARD_CULL defaults to 0 and the published ROM never sets it. */
volatile u32 gNdsDreamLandCardCullMask[2] = {
    NDS_DREAMLAND_CARD_CULL_MASK0,
    NDS_DREAMLAND_CARD_CULL_MASK1,
};
volatile u32 gNdsDreamLandCardCullSkippedRuns;
volatile u32 gNdsDreamLandCardCullSkippedTris;
#endif /* NDS_DREAMLAND_CARD_CULL */

#if NDS_DREAMLAND_DS_MESH
/* Task 62: generated Dream Land DS-native static 3D mesh. Draws the exact
 * source-world geometry with source material attributes from the baked blob
 * (scripts/stages/dreamland/generate_dreamland_ds_mesh.py), replacing the four static segments.
 * Default-off keeps the shipping path byte-identical. */

/* Task 62 engagement counters (shared HUD row). Gated only by the feature
 * flag, not by PROFILE_LEVEL, so the counter survives in the published
 * profile-0 hwtri ROM that must report engagement. */
volatile u32 gNdsDreamLandDSSubmittedVertices;
volatile u32 gNdsDreamLandDSGroups;
volatile u32 gNdsDreamLandDSWords;

/* Draws one generated Dream Land DS-native static segment. The generated
 * source-run and dense indices retain exact topology and material attributes;
 * the live binding matrices retain the platform transforms that are not
 * represented by the host world-space IR. */
static u32 ndsRendererDreamLandDrawStatic3D(
    NDSRendererStats *stats,
    u32 source_segment)
{
    u32 group_index;
    u32 segment_triangles = 0u;
    u32 segment_groups = 0u;
    u32 submitted_vertices = 0u;
    u32 gx_words = 0u;

    for (group_index = 0u; group_index < NDS_DREAMLAND_DS_GROUP_COUNT;
         group_index++)
    {
        if (sNdsDreamLandDSGroupSourceSegment[group_index] !=
            source_segment)
        {
            continue;
        }
        u32 first = sNdsDreamLandDSGroupFirstVertex[group_index];
        u32 count = sNdsDreamLandDSGroupVertexCount[group_index];
        u32 submit_class =
            sNdsDreamLandDSGroupSubmitClass[group_index];
        u32 source_run = sNdsDreamLandDSGroupSourceRun[group_index];
        const NDSNativeStageRun *native_run;
        const NDSNativeStagePreparedRun *prepared_run;
        u32 triangle_count = count / 3u;
        u32 triangle_offset;
        u32 emitted_triangles = 0u;

        if ((sNdsDreamLandDSGroupPrim[group_index] !=
             NDS_DREAMLAND_DS_PRIM_TRIANGLES) ||
            (source_run >= NDS_NATIVE_STAGE_RUN_COUNT) ||
            (count == 0u) ||
            ((count % 3u) != 0u) ||
            (first + count > NDS_DREAMLAND_DS_VERTEX_COUNT) ||
            ((submit_class !=
              NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) &&
             (submit_class !=
              NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) &&
             (submit_class !=
              NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)))
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
            continue;
        }
        native_run = &sNdsNativeStageRuns[source_run];
        prepared_run = &sNdsNativeStageOwnerExecution.runs[source_run];
        if ((native_run->submit_class != submit_class) ||
            (native_run->triangle_count != triangle_count))
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
            continue;
        }
        if (ndsRendererNativeStageBeginRun(
                native_run, prepared_run, submit_class,
                sNdsNativeStageSegments[source_segment].owner, stats,
                FALSE) == FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
            continue;
        }
        gx_words += 1u;
        for (triangle_offset = 0u;
             triangle_offset < triangle_count;
             triangle_offset++)
        {
            if (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                u32 emitted = ndsRendererNativeStageEmitNoZTriangle(
                    native_run, prepared_run, triangle_offset,
                    ndsRendererHardwareNextProjectedDepth());

                emitted_triangles += emitted;
                submitted_vertices += emitted * 3u;
                gx_words += emitted * ((prepared_run->textured != 0u) ?
                    4u : 3u);
                continue;
            }
            {
                u32 corner_offset;

                for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
                {
                    u32 idx = first + triangle_offset * 3u + corner_offset;
                    u32 dense_index = sNdsDreamLandDSSourceDense[idx];

                    if (dense_index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT)
                    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
                        gNdsRendererM3PostArmFailureCount++;
#endif
                        continue;
                    }
                    ndsRendererNativeStageEmitVertex(
                        &sNdsNativeStageVertices[dense_index],
                        &sNdsNativeStagePreparedDense[dense_index],
                        prepared_run, submit_class);
                    submitted_vertices++;
                    gx_words += (prepared_run->textured != 0u) ? 4u : 3u;
                }
                emitted_triangles++;
                ndsRendererHardwareEnterProjectedForeground();
            }
        }
        ndsRendererHardwareEndBatch();
        ndsRendererNativeStageAccountRun(
            stats,
            (submit_class ==
             NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
                NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX :
                submit_class,
            emitted_triangles);
        segment_triangles += triangle_count;
        segment_groups++;
    }

#if NDS_TASK36_HW_COMPOSE
    ndsRendererNativeStageTask36EndSegment();
#endif
    if (stats != NULL)
    {
        stats->triangle_count += segment_triangles;
    }
    sNdsRendererHardwareSubmitted = TRUE;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    sNdsRendererHardwareMatrixGeneration = 0u;
    gNdsDreamLandDSSubmittedVertices += submitted_vertices;
    gNdsDreamLandDSGroups += segment_groups;
    gNdsDreamLandDSWords += gx_words;
    return segment_triangles;
}
#endif /* NDS_DREAMLAND_DS_MESH */

#if NDS_R2_STAGE_ACTORS_PROOF
/* R2-02 E3 falsifier. Admitting whispy_eyes, whispy_mouth, flowers_back and
 * flowers_front to the Task 36 replay bakes their command stream once and
 * replays it for the rest of the match, so the cut is correct only if nothing
 * those fifteen runs emit changes between frames. Three of the four inputs are
 * already settled: Task 51 replaced their per-frame compose with a MULT4x3 of
 * the constant sNdsNativeStageBakedWorldMatrices, and layer0/2/3 have been
 * replaying their own per-triangle no-Z projection loads correctly for many
 * tasks, which is a standing proof that frame->projection and the
 * projected-depth sequence do not move either.
 *
 * The fourth input is the prepared dense vertex data -- the packed colours and
 * texture coordinates the material state produces -- and Whispy has a material
 * animation, so "it looked right in one screenshot" is not an answer. This
 * hashes exactly the prepared run and prepared dense data those fifteen runs
 * consume, once a frame, and counts the frames on which the hash differs from
 * the previous one.
 *
 * It answered its question -- 0 changes in 1,828 frames -- and R2-02 E3 and E4
 * still failed, because the prepared data was never the part that moves. The
 * actor segments' *matrices* are. Read this proof as covering vertex data only,
 * and never as a licence to widen a replay or rigid mask. */
volatile u32 gNdsR2ActorPreparedHash;
volatile u32 gNdsR2ActorPreparedChangeCount;
volatile u32 gNdsR2ActorPreparedFrameCount;

static void ndsRendererR2ActorPreparedProof(void)
{
    static const u8 actor_segments[4] = {1u, 2u, 3u, 6u};
    u32 hash = 2166136261u;
    u32 slot;

    for (slot = 0u; slot < 4u; slot++)
    {
        const NDSNativeStageSegment *segment =
            &sNdsNativeStageSegments[actor_segments[slot]];
        u32 run_offset;

        for (run_offset = 0u; run_offset < segment->run_count; run_offset++)
        {
            u32 run_index = (u32)segment->first_run + run_offset;
            const NDSNativeStageRun *run = &sNdsNativeStageRuns[run_index];
            const u32 *run_words =
                (const u32 *)&sNdsNativeStageOwnerExecution.runs[run_index];
            u32 corner_count = (u32)run->triangle_count * 3u;
            u32 corner;
            u32 word;

            for (word = 0u;
                 word < sizeof(NDSNativeStagePreparedRun) / sizeof(u32);
                 word++)
            {
                hash = (hash ^ run_words[word]) * 16777619u;
            }
            for (corner = 0u; corner < corner_count; corner++)
            {
                u32 dense_index =
                    sNdsNativeStageCorners[(u32)run->first_corner + corner];
                const u32 *dense_words =
                    (const u32 *)&sNdsNativeStagePreparedDense[dense_index];

                for (word = 0u;
                     word < sizeof(NDSNativeStagePreparedDense) / sizeof(u32);
                     word++)
                {
                    hash = (hash ^ dense_words[word]) * 16777619u;
                }
            }
        }
    }
    if ((gNdsR2ActorPreparedFrameCount != 0u) &&
        (hash != gNdsR2ActorPreparedHash))
    {
        gNdsR2ActorPreparedChangeCount++;
    }
    gNdsR2ActorPreparedHash = hash;
    gNdsR2ActorPreparedFrameCount++;
}
#endif

/* The forced-generic route only exists where the Task 36 replay owner does; the
 * r2_reuse gates below are compiled on NDS_R2_STAGE_DIRECT alone, which is a
 * wider condition, so they need a definition in both cases. */
#if NDS_TASK36_HW_COMPOSE == 2
#define NDS_TASK36_FORCED_GENERIC(seg) \
    (ndsRendererTask36SegmentForcedGeneric(seg) != FALSE)
#else
#define NDS_TASK36_FORCED_GENERIC(seg) (((void)(seg)), 0)
#endif


s32 ndsRendererPrepareNativeStageOwner(
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    NDSRendererTraversalState *state =
        &sNdsNativeStageOwnerExecution.traversal;
    NDSNativeStageTopologySummary topology;
    /* Resolve the packet for the loaded stage kind exactly once per
     * preparation.  A kind with no baked packet declines here rather than
     * drawing another stage's geometry. */
    const s32 packet_selected = ndsRendererNativeStageSelectPacket();
    u64 epoch_mask = 0u;
    u32 segment_index;
    s32 accepted = FALSE;

    if (packet_selected == FALSE)
    {
        sNdsNativeStageOwnerExecution.active = FALSE;
        return FALSE;
    }
#if NDS_R2_STAGE_DIRECT
    /* R2-02 E1a. Reuse the prepared run table when it was built for this exact
     * config and asset set. epoch_mask must be restored with it: PrepareRun
     * accumulates it and the Task 36 replay capture and segment-0 hash consume
     * it after the loop. */
    u32 r2_reuse =
        ((sNdsNativeStageOwnerExecution.r2_prepared_valid != 0u) &&
         (ndsRendererNativeStagePreparedTexturesProven() != FALSE) &&
         (sNdsNativeStageOwnerExecution.r2_prepared_topology_generation ==
          frame->topology_generation) &&
         (sNdsNativeStageOwnerExecution.r2_prepared_topology_stamp ==
          frame->topology_stamp) &&
         (sNdsNativeStageOwnerExecution.r2_prepared_config == frame->config) &&
         (memcmp(sNdsNativeStageOwnerExecution.r2_prepared_asset_bases,
                 frame->asset_bases,
                 sizeof(sNdsNativeStageOwnerExecution.r2_prepared_asset_bases))
          == 0)) ? 1u : 0u;

    if (r2_reuse != 0u)
    {
        epoch_mask = sNdsNativeStageOwnerExecution.r2_prepared_epoch_mask;
        gNdsR2StagePrepareReuseCount++;
    }
    else
    {
        gNdsR2StagePrepareBuildCount++;
        /* R2-07 leg A. The table about to be written is not the table the
         * standing proof was taken on, and a rebuild is not an epoch event, so
         * the proof must be dropped here or a fresh run would inherit its
         * predecessor's certificate. The next consult re-sweeps and re-stamps. */
        ndsRendererNativeStagePreparedTextureProofDrop();
#if NDS_R2_STAGE_ROUTE_PROBE
        /* WHICH of the five key components missed. BuildCount alone cannot
         * separate "the previous frame's owner rejected and zeroed valid" from
         * "the topology moved" from "an asset base changed", and the two
         * existing reject-reason words are LATCHES reset at the top of every
         * prepare -- read at end of run they say only what the last frame did,
         * which was a Results frame reporting 0/0 while the battle had rebuilt
         * 197 times. These count. */
        if (sNdsNativeStageOwnerExecution.r2_prepared_valid == 0u)
        {
            gNdsR2StageKeyMissInvalid++;
        }
        else if (sNdsNativeStageOwnerExecution.r2_prepared_topology_generation !=
                 frame->topology_generation)
        {
            gNdsR2StageKeyMissGeneration++;
        }
        else if (sNdsNativeStageOwnerExecution.r2_prepared_topology_stamp !=
                 frame->topology_stamp)
        {
            gNdsR2StageKeyMissStamp++;
        }
        else if (sNdsNativeStageOwnerExecution.r2_prepared_config !=
                 frame->config)
        {
            gNdsR2StageKeyMissConfig++;
        }
        else
        {
            gNdsR2StageKeyMissAssets++;
        }
#endif
    }
#endif
#if NDS_TASK36_REJECT_TRACE
    u32 task36_reject_reason = 1u;

    gNdsRendererTask36RendererRejectReason = 0u;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 phase0_preflight_start;
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
    u32 task103_own_entry = cpuGetTiming();
    u32 task103_own_mark;
#endif

#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3PreflightAttemptCount++;
    gNdsRendererM3SegmentCount = 0u;
    gNdsRendererM3SegmentMask = 0u;
    gNdsRendererM3DObjCount = 0u;
    gNdsRendererM3BindingCount = 0u;
    gNdsRendererM3RunCount = 0u;
    gNdsRendererM3TriangleCount = 0u;
    gNdsRendererM3ResidentEpochCount = 0u;
    gNdsRendererM3MaterialShadowCount = 0u;
    gNdsRendererM3MaterialCommitCount = 0u;
    gNdsRendererM3CrossRunCount = 0u;
    gNdsRendererM3CrossTriangleCount = 0u;
    gNdsRendererM3CrossForeignCornerCount = 0u;
#if NDS_TASK36_HW_COMPOSE
    gNdsRendererTask36HardwareComposedDObjCount = 0u;
    gNdsRendererTask36CameraLoadCount = 0u;
    gNdsRendererTask36WorldMultCount = 0u;
#endif
#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
    gNdsRendererM3GeneratedSegment0AttemptCount = 0u;
    gNdsRendererM3GeneratedSegment0SuccessCount = 0u;
    gNdsRendererM3GeneratedSegment0PreGxFallbackCount = 0u;
    gNdsRendererM3GeneratedSegment0RunCount = 0u;
    gNdsRendererM3GeneratedSegment0TriangleCount = 0u;
    gNdsRendererM3GeneratedSegment0EpochCount = 0u;
    gNdsRendererM3GeneratedSegment0MaterialCount = 0u;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererM3GeneratedSegment0ShadowDenseCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowStateEntryCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowSyncCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowFieldComparisonCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowMismatchCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowFaultInjectedCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowFaultRejectedCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowLiveFaultInjectedCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowLiveFaultRejectedCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowLiveFaultRevalidatedCount = 0u;
#endif
#endif
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0Reset();
    phase0_preflight_start = ndsRendererM3Phase0Tick();
#endif
    sNdsNativeStageOwnerExecution.active = FALSE;
    sNdsNativeStageOwnerExecution.binding_composed = NULL;
#if NDS_TASK36_HW_COMPOSE
    sNdsNativeStageOwnerExecution.projection = NULL;
    sNdsNativeStageOwnerExecution.camera_modelview = NULL;
    sNdsNativeStageOwnerExecution.binding_world = NULL;
    sNdsNativeStageOwnerExecution.rigid_binding_mask = 0u;
    sNdsNativeStageOwnerExecution.task36_seen_binding_mask = 0u;
    sNdsNativeStageOwnerExecution.task36_local_pushed = FALSE;
    sNdsNativeStageOwnerExecution.task36_segment_active = FALSE;
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    memset(sNdsRendererBenchmarkSegment0AssetBases, 0,
           sizeof(sNdsRendererBenchmarkSegment0AssetBases));
#endif
    if ((gNdsRendererFastRunMode !=
         NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE) ||
        (frame == NULL) || (stats == NULL) ||
        (frame->dobjs == NULL) ||
        (frame->binding_display_lists == NULL) ||
        (frame->projection == NULL) ||
#if NDS_TASK36_HW_COMPOSE
        (frame->camera_modelview == NULL) ||
        (frame->binding_world == NULL) ||
#endif
        (frame->binding_composed == NULL) ||
        (frame->materials == NULL) || (frame->config == NULL) ||
        (NDS_NATIVE_STAGE_PRODUCTION_PACKET_ABI != 0x4d335031u) ||
        (ndsRendererNativeStageValidateTopology(frame, &topology) == FALSE))
    {
        goto done;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    task103_own_mark = cpuGetTiming();
    gNdsTask103OwnValidateTicks += task103_own_mark - task103_own_entry;
#endif
#if NDS_TASK36_REJECT_TRACE
    task36_reject_reason = 2u;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3MeasureResidualKey(frame);
#endif
#if NDS_TASK36_HW_COMPOSE == 2
    ndsRendererTask36ReplayBeginFrame(frame);
#endif

#if NDS_DREAMLAND_DS_MESH
    /* The generated static mesh is submitted from segment 0's display commit,
     * not during preparation. Keep preflighting the native owner so the map
     * segments that own Whispy and the flowers remain live. */
    if ((frame->projection == NULL) || (frame->camera_modelview == NULL))
    {
        goto done;
    }
#endif

    for (segment_index = 0u;
         segment_index < NDS_NATIVE_STAGE_SEGMENT_COUNT;
         segment_index++)
    {
        const NDSNativeStageSegment *segment =
            &sNdsNativeStageSegments[segment_index];
        u32 binding_offset;
#if NDS_R2_STAGE_DIRECT
        /* R2-07 E2. Queried once per segment rather than once per run: the
         * route is constant across the segment and each query records an
         * observation, so per-run calls would only add noise. */
        const s32 segment_forced_generic =
            NDS_TASK36_FORCED_GENERIC(segment_index) ? TRUE : FALSE;
#endif

#if NDS_TASK103_STAGE_RUN_PHASE
        task103_own_mark = cpuGetTiming();
#endif
#if (NDS_TASK36_HW_COMPOSE == 2) && NDS_TASK104_STAGE_STATS_ELISION
        /* Task 104: tested before the clear rather than after it. The
         * eligibility predicate reads nothing out of the incoming stats — only
         * the replay owner's own flags and the segment mask — so moving it is
         * order-independent. What changes is that a hit no longer clears 1,292
         * bytes on behalf of a copy that overwrote every one of them. */
        if (ndsRendererTask36ReplayUsePreparedSegment(
                segment_index,
                &sNdsNativeStageOwnerExecution.preflight_stats,
                &epoch_mask) != FALSE)
        {
#if NDS_TASK103_STAGE_RUN_PHASE
            gNdsTask103OwnReuseTicks += cpuGetTiming() - task103_own_mark;
            gNdsTask103OwnReuseCount++;
#endif
            continue;
        }
#endif
#if NDS_R2_STAGE_PREFLIGHT && (NDS_TASK36_HW_COMPOSE == 2) && \
    NDS_R2_STAGE_DIRECT
        /* R2-02 E8, and the switch plan's §7 instruction taken literally: "no
         * generic preflight, no stats temporaries". For the five segments the
         * Task 36 replay does not serve, this loop body has no consumer once
         * E1a's prepared run table is valid.
         *
         * Every output is accounted for:
         *   runs[]         -- E1a reuses it wholesale; PrepareRun is already
         *                     skipped below on exactly this condition.
         *   epoch_mask     -- restored from the same memo above.
         *   preflight_stats/traversal -- ndsRendererTask36ReplayCapturePrepared
         *                     Segment early-returns for an ineligible segment,
         *                     the next segment reinitialises both, and
         *                     `sNdsNativeStageOwnerExecution.traversal` is read
         *                     nowhere outside this function (the commit path
         *                     consumes runs[], not the traversal). The single
         *                     member that escapes the loop is
         *                     `sync_command_count`, memoised as
         *                     r2_prepared_sync_count.
         *
         * Eligible segments are deliberately excluded rather than relying on
         * the replay having hit: on a capture frame `frame_replay` is FALSE, so
         * they must run the full body to produce the stats being captured.
         *
         * Cost removed, measured on the graduated program: ndsRendererInitStats
         * plus ndsRendererInitTraversalState 13,565 ticks/frame over these five
         * segments, and 21 run-level plus 16 binding-level state spans. */
        if ((r2_reuse != 0u) && (segment_forced_generic == FALSE) &&
            (ndsRendererTask36ReplaySegmentEligible(segment_index) == FALSE))
        {
            gNdsR2StagePreflightElideCount++;
            continue;
        }
#endif
        ndsRendererInitStats(&sNdsNativeStageOwnerExecution.preflight_stats);
        sNdsNativeStageOwnerExecution.preflight_stats.geometry_mode =
            segment->initial_geometry;
        ndsRendererInitTraversalState(
            state, frame->config,
            &sNdsNativeStageOwnerExecution.preflight_stats,
            NULL, NULL, 0u);
#if NDS_TASK103_STAGE_RUN_PHASE
        gNdsTask103OwnInitTicks += cpuGetTiming() - task103_own_mark;
        gNdsTask103OwnInitCount++;
        task103_own_mark = cpuGetTiming();
#endif
#if NDS_TASK36_HW_COMPOSE == 2
#if !NDS_TASK104_STAGE_STATS_ELISION
        if (ndsRendererTask36ReplayUsePreparedSegment(
                segment_index,
                &sNdsNativeStageOwnerExecution.preflight_stats,
                &epoch_mask) != FALSE)
        {
#if NDS_TASK103_STAGE_RUN_PHASE
            gNdsTask103OwnReuseTicks += cpuGetTiming() - task103_own_mark;
            gNdsTask103OwnReuseCount++;
#endif
            continue;
        }
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
        gNdsTask103OwnReuseTicks += cpuGetTiming() - task103_own_mark;
        gNdsTask103OwnReuseMissCount++;
#endif
#endif
#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE && \
    !NDS_RENDERER_M3_PHASE0_PROFILE
        if ((segment_index == 0u) &&
            (NDS_NATIVE_STAGE_HAS_GENERATED_SEGMENT0 != 0u))
        {
            if (ndsRendererNativeStagePrepareGeneratedSegment0(
                    frame,
                    &sNdsNativeStageOwnerExecution.preflight_stats,
                    state, &epoch_mask) == FALSE)
            {
#if NDS_TASK36_REJECT_TRACE
                task36_reject_reason = 100u;
#endif
                goto done;
            }
#if NDS_TASK36_HW_COMPOSE == 2
            ndsRendererTask36ReplayCapturePreparedSegment(
                segment_index,
                &sNdsNativeStageOwnerExecution.preflight_stats,
                epoch_mask);
#endif
            continue;
        }
#endif
        for (binding_offset = 0u;
             binding_offset < segment->binding_count;
             binding_offset++)
        {
            u32 binding_index = (u32)segment->first_binding + binding_offset;
            const NDSNativeStageBinding *binding =
                &sNdsNativeStageBindings[binding_index];
            u32 run_offset;

            for (run_offset = 0u;
                 run_offset < binding->run_count;
                 run_offset++)
            {
                u32 run_index = (u32)binding->first_run + run_offset;

#if NDS_TASK103_STAGE_RUN_PHASE
                task103_own_mark = cpuGetTiming();
#endif
                if (ndsRendererNativeStageApplyStateSpan(
                         &sNdsNativeStageStateSpans[run_index], frame,
                         &sNdsNativeStageOwnerExecution.preflight_stats,
                         state) == FALSE)
                {
#if NDS_TASK36_REJECT_TRACE
                    task36_reject_reason = 200u + run_index;
#endif
                    goto done;
                }
#if NDS_RENDERER_M3_PHASE0_PROFILE
                {
                    u32 residual_prepare_ticks_start =
                        gNdsRendererM3Phase0PrepareRunTicks;
                    u32 prepare_run_start = ndsRendererM3Phase0Tick();
                    s32 prepare_run_result =
#if NDS_R2_STAGE_DIRECT
                        ((r2_reuse != 0u) &&
                         (segment_forced_generic == FALSE)) ? TRUE :
#endif
                        ndsRendererNativeStagePrepareRun(
                        run_index, frame,
                        &sNdsNativeStageOwnerExecution.preflight_stats,
                        state, &epoch_mask);

                    ndsRendererM3Phase0FinishSpan(
                        &gNdsRendererM3Phase0PrepareRunTicks,
                        prepare_run_start);
                    if (run_index >=
                        NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT)
                    {
                        gNdsRendererM3ResidualPrepareTicks +=
                            gNdsRendererM3Phase0PrepareRunTicks -
                            residual_prepare_ticks_start;
                        gNdsRendererM3ResidualRunCount++;
                    }
                    if (prepare_run_result == FALSE)
                    {
#if NDS_TASK36_REJECT_TRACE
                        task36_reject_reason = 300u + run_index;
#endif
                        goto done;
                    }
                }
#else
#if NDS_TASK103_STAGE_RUN_PHASE
                gNdsTask103OwnStateSpanTicks += cpuGetTiming() - task103_own_mark;
                gNdsTask103OwnStateSpanCount++;
                task103_own_mark = cpuGetTiming();
#endif
#if NDS_R2_STAGE_DIRECT
                if ((r2_reuse == 0u) || (segment_forced_generic != FALSE))
#endif
                if (ndsRendererNativeStagePrepareRun(
                        run_index, frame,
                        &sNdsNativeStageOwnerExecution.preflight_stats,
                        state, &epoch_mask) == FALSE)
                {
#if NDS_TASK36_REJECT_TRACE
                    task36_reject_reason = 300u + run_index;
#endif
                    goto done;
                }
#if NDS_TASK103_STAGE_RUN_PHASE
                gNdsTask103OwnPrepareRunTicks += cpuGetTiming() - task103_own_mark;
                gNdsTask103OwnPrepareRunCount++;
#endif
#endif
            }
            if (ndsRendererNativeStageApplyStateSpan(
                    &sNdsNativeStageStateSpans[
                        NDS_NATIVE_STAGE_RUN_COUNT + binding_index],
                    frame, &sNdsNativeStageOwnerExecution.preflight_stats,
                    state) == FALSE)
            {
#if NDS_TASK36_REJECT_TRACE
                task36_reject_reason = 400u + binding_index;
#endif
                goto done;
            }
        }
#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE && \
    NDS_RENDERER_M3_PHASE0_PROFILE
        if (segment_index == 0u)
        {
            u32 current_hash_a;
            u32 current_hash_b;
            u32 current_field_count;
            u32 generated_hash_a;
            u32 generated_hash_b;
            u32 generated_field_count;
            u64 generated_epoch_mask = 0u;
            NDSRendererNativeStageFrame live_fault_frame;
            NDSNativeStageTopologySummary live_fault_topology;

            ndsRendererNativeStageHashGeneratedSegment0Outputs(
                &sNdsNativeStageOwnerExecution.preflight_stats, state,
                epoch_mask, &current_hash_a, &current_hash_b,
                &current_field_count);
            gNdsRendererM3GeneratedSegment0ShadowFaultInjectedCount++;
            if (ndsRendererNativeStageValidateGeneratedSegment0(TRUE) ==
                FALSE)
            {
                gNdsRendererM3GeneratedSegment0ShadowFaultRejectedCount++;
            }
            else
            {
                gNdsRendererM3GeneratedSegment0ShadowMismatchCount++;
                goto done;
            }
            live_fault_frame = *frame;
            live_fault_frame.asset_bases[1] =
                (const void *)((const u8 *)frame->asset_bases[1] + 8u);
            gNdsRendererM3GeneratedSegment0ShadowLiveFaultInjectedCount++;
            if (ndsRendererNativeStageValidateTopologyFull(
                    &live_fault_frame, &live_fault_topology) == FALSE)
            {
                gNdsRendererM3GeneratedSegment0ShadowLiveFaultRejectedCount++;
            }
            else
            {
                gNdsRendererM3GeneratedSegment0ShadowMismatchCount++;
                goto done;
            }
            if ((ndsRendererNativeStageValidateTopologyFull(
                     frame, &live_fault_topology) == FALSE) ||
                (memcmp(&live_fault_topology, &topology,
                        sizeof(live_fault_topology)) != 0))
            {
                gNdsRendererM3GeneratedSegment0ShadowMismatchCount++;
                goto done;
            }
            gNdsRendererM3GeneratedSegment0ShadowLiveFaultRevalidatedCount++;

            ndsRendererInitStats(
                &sNdsNativeStageOwnerExecution.preflight_stats);
            sNdsNativeStageOwnerExecution.preflight_stats.geometry_mode =
                segment->initial_geometry;
            ndsRendererInitTraversalState(
                state, frame->config,
                &sNdsNativeStageOwnerExecution.preflight_stats,
                NULL, NULL, 0u);
            if (ndsRendererNativeStagePrepareGeneratedSegment0(
                    frame,
                    &sNdsNativeStageOwnerExecution.preflight_stats,
                    state, &generated_epoch_mask) == FALSE)
            {
                gNdsRendererM3GeneratedSegment0ShadowMismatchCount++;
                goto done;
            }
            ndsRendererNativeStageHashGeneratedSegment0Outputs(
                &sNdsNativeStageOwnerExecution.preflight_stats, state,
                generated_epoch_mask, &generated_hash_a,
                &generated_hash_b, &generated_field_count);
            gNdsRendererM3GeneratedSegment0ShadowDenseCount =
                NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_COUNT;
            gNdsRendererM3GeneratedSegment0ShadowStateEntryCount =
                sNdsNativeStageSegment0ColdCertificate.state_count;
            gNdsRendererM3GeneratedSegment0ShadowSyncCount =
                sNdsNativeStageSegment0ColdCertificate.sync_count;
            gNdsRendererM3GeneratedSegment0ShadowFieldComparisonCount =
                generated_field_count;
            if ((current_hash_a != generated_hash_a) ||
                (current_hash_b != generated_hash_b) ||
                (current_field_count != generated_field_count) ||
                (generated_epoch_mask != epoch_mask))
            {
                gNdsRendererM3GeneratedSegment0ShadowMismatchCount++;
                goto done;
            }
            epoch_mask = generated_epoch_mask;
        }
#endif
#if NDS_TASK36_HW_COMPOSE == 2
        ndsRendererTask36ReplayCapturePreparedSegment(
            segment_index,
            &sNdsNativeStageOwnerExecution.preflight_stats,
            epoch_mask);
#endif
    }
#if NDS_TASK36_REJECT_TRACE
    task36_reject_reason = 3u;
#endif
    if ((epoch_mask != (((u64)1u <<
                         NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT) - 1u)) ||
        (topology.raw_triangles != NDS_NATIVE_STAGE_SUBMIT_RAW_TRIANGLES) ||
        (topology.projected_no_z_triangles != NDS_NATIVE_STAGE_SUBMIT_NO_Z_TRIANGLES) ||
        (topology.projected_range_triangles != NDS_NATIVE_STAGE_SUBMIT_RANGE_TRIANGLES) ||
        (topology.cross_runs != NDS_NATIVE_STAGE_CROSS_MATRIX_RUN_COUNT) ||
        (topology.cross_triangles !=
         NDS_NATIVE_STAGE_CROSS_MATRIX_TRIANGLE_COUNT) ||
        (topology.cross_foreign_corners !=
         NDS_NATIVE_STAGE_CROSS_MATRIX_FOREIGN_CORNER_COUNT))
    {
        goto done;
    }
#if NDS_TASK36_REJECT_TRACE
    task36_reject_reason = 4u;
#endif

#if NDS_R2_STAGE_ACTORS_PROOF
    ndsRendererR2ActorPreparedProof();
#endif
    sNdsNativeStageOwnerExecution.binding_composed = frame->binding_composed;
#if NDS_TASK36_HW_COMPOSE
    sNdsNativeStageOwnerExecution.projection = frame->projection;
    sNdsNativeStageOwnerExecution.camera_modelview = frame->camera_modelview;
    sNdsNativeStageOwnerExecution.binding_world = frame->binding_world;
    sNdsNativeStageOwnerExecution.rigid_binding_mask =
        frame->rigid_binding_mask;
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    memcpy(sNdsRendererBenchmarkSegment0AssetBases, frame->asset_bases,
           sizeof(sNdsRendererBenchmarkSegment0AssetBases));
#endif
    ndsRendererInitStats(stats);
    stats->command_count = NDS_NATIVE_STAGE_SOURCE_COMMAND_COUNT;
    stats->vertex_count = NDS_NATIVE_STAGE_SOURCE_VERTEX_COUNT;
    stats->vertex_command_count = NDS_NATIVE_STAGE_VERTEX_COMMAND_COUNT;
    stats->triangle_command_count = NDS_NATIVE_STAGE_TRIANGLE_COMMAND_COUNT;
#if NDS_R2_STAGE_DIRECT
    /* R2-02 E8: taken from the memo when the loop body was elided, so the
     * result does not depend on whether the last segment was replay-served. */
    stats->sync_command_count = (r2_reuse != 0u) ?
        sNdsNativeStageOwnerExecution.r2_prepared_sync_count :
        sNdsNativeStageOwnerExecution.preflight_stats.sync_command_count;
#else
    stats->sync_command_count =
        sNdsNativeStageOwnerExecution.preflight_stats.sync_command_count;
#endif
    sNdsNativeStageOwnerExecution.stats = stats;
    sNdsNativeStageOwnerExecution.next_segment = 0u;
    sNdsNativeStageOwnerExecution.active = TRUE;
#if NDS_DREAMLAND_DS_MESH
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3PreflightSuccessCount++;
    gNdsRendererM3ResidentEpochCount =
        NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT;
    gNdsRendererM3CrossRunCount = topology.cross_runs;
    gNdsRendererM3CrossTriangleCount = topology.cross_triangles;
    gNdsRendererM3CrossForeignCornerCount =
        topology.cross_foreign_corners;
#endif
    accepted = TRUE;
#if NDS_R2_STAGE_DIRECT
    /* The table is only publishable as reusable once the whole owner prepare
     * has accepted -- a run that rejected mid-loop leaves runs[] torn. */
    sNdsNativeStageOwnerExecution.r2_prepared_config = frame->config;
    sNdsNativeStageOwnerExecution.r2_prepared_topology_generation =
        frame->topology_generation;
    sNdsNativeStageOwnerExecution.r2_prepared_topology_stamp =
        frame->topology_stamp;
    memcpy(sNdsNativeStageOwnerExecution.r2_prepared_asset_bases,
           frame->asset_bases,
           sizeof(sNdsNativeStageOwnerExecution.r2_prepared_asset_bases));
    sNdsNativeStageOwnerExecution.r2_prepared_epoch_mask = epoch_mask;
    sNdsNativeStageOwnerExecution.r2_prepared_sync_count =
        stats->sync_command_count;
    sNdsNativeStageOwnerExecution.r2_prepared_valid = 1u;
#endif
#if NDS_TASK36_HW_COMPOSE == 2
    ndsRendererTask36ReplayStartCapture(frame);
#endif

done:
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0FinishSpan(
        &gNdsRendererM3Phase0PreflightTicks, phase0_preflight_start);
#endif
    if (accepted == FALSE)
    {
#if NDS_TASK36_HW_COMPOSE == 2
        if (sNdsRendererTask36ReplayOwner.frame_capture != FALSE)
        {
            sNdsRendererTask36ReplayOwner.capture_fault = TRUE;
            ndsRendererTask36ReplayFinishFrame();
        }
#endif
#if NDS_TASK36_REJECT_TRACE
        gNdsRendererTask36RendererRejectReason = task36_reject_reason;
#endif
        sNdsNativeStageOwnerExecution.stats = NULL;
        sNdsNativeStageOwnerExecution.binding_composed = NULL;
#if NDS_TASK36_HW_COMPOSE
        sNdsNativeStageOwnerExecution.projection = NULL;
        sNdsNativeStageOwnerExecution.camera_modelview = NULL;
        sNdsNativeStageOwnerExecution.binding_world = NULL;
        sNdsNativeStageOwnerExecution.rigid_binding_mask = 0u;
#endif
        sNdsNativeStageOwnerExecution.next_segment = 0u;
        sNdsNativeStageOwnerExecution.active = FALSE;
#if NDS_R2_STAGE_DIRECT
        /* Any fallback invalidates the table: runs[] may be torn, and the
         * reason it rejected may be exactly that the topology moved. */
        sNdsNativeStageOwnerExecution.r2_prepared_valid = 0u;
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3PreflightFallbackCount++;
#endif
    }
    return accepted;
}

void ndsRendererResetNativeStageValidationCache(void)
{
    memset(&sNdsNativeStageValidationCache, 0,
           sizeof(sNdsNativeStageValidationCache));
#if NDS_TASK36_HW_COMPOSE == 2
    ndsRendererTask36ReplayReset();
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    sNdsRendererM3ResidualKeyValid = FALSE;
#endif
}

#if NDS_RENDER_ECONOMY
static u32 ndsRendererEconomySkipNativeStageSegment(
    const NDSNativeStageSegment *segment,
    NDSRendererStats *stats)
{
    u32 segment_triangles = 0u;
    u32 run_offset;

    ndsRendererHardwareEndBatch();
    for (run_offset = 0u; run_offset < segment->run_count; run_offset++)
    {
        const NDSNativeStageRun *run = &sNdsNativeStageRuns[
            (u32)segment->first_run + run_offset];
        u32 triangle_offset;

        for (triangle_offset = 0u;
             triangle_offset < run->triangle_count;
             triangle_offset++)
        {
            if (run->submit_class ==
                NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                (void)ndsRendererHardwareNextProjectedDepth();
            }
            else
            {
                ndsRendererHardwareEnterProjectedForeground();
            }
        }
        stats->triangle_count += run->triangle_count;
        segment_triangles += run->triangle_count;
    }
    gNdsRendererEconomyAppliedOwnerMask |= (u32)1u << segment->owner;
    gNdsRendererEconomySkippedRunCount += segment->run_count;
    gNdsRendererEconomySkippedTriangleCount += segment_triangles;
    return segment_triangles;
}
#endif

s32 NDS_R2_ITCM_PACK2_CODE ndsRendererCommitNativeStageSegment(u32 segment_index)
{
    NDSRendererStats *stats = sNdsNativeStageOwnerExecution.stats;
    const NDSNativeStageSegment *segment;

    NDS_FIGHTER_PACKET_DMA_WAIT();
    u32 run_offset;
    u32 segment_triangles = 0u;
    /* P2-4n1 step 7. A DLLink packet's runs sit in DObj preorder with each
     * DObj's links in array order, which is the order the source PARSES them;
     * the source DRAWS them per display head, 0 then 2 then 1 then 3
     * (syTaskmanUpdateDLBuffers chains the heads in that order at every
     * camera-group boundary). Every run is self-contained by the time it is
     * committed -- its matrices come from its binding and its state from
     * prepared_run, both resolved at prepare -- so emitting the segment's runs
     * head by head is a pure reordering. Layer packets have no head array and
     * take exactly one pass in their original order. What this does NOT
     * reproduce is the interleave with other owners drawn in the same camera
     * group; that delta is recorded on P2-4n1 and is measured at acceptance. */
    static const u8 head_order[4] = { 0u, 2u, 1u, 3u };
    const u8 *binding_heads = NDS_NATIVE_STAGE_BINDING_HEADS;
    const u32 head_pass_count = (binding_heads != NULL) ? 4u : 1u;
    u32 head_pass;
#if NDS_TASK36_HW_COMPOSE == 2
    u32 task36_capture_segment = FALSE;
    u32 task36_replay_segment = FALSE;
#endif
#if NDS_RENDERER_SCREEN_SPACE_CENSUS
    u32 census_owner_start = 0u;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 commit_start;
#endif

    if (sNdsNativeStageOwnerExecution.active == FALSE)
    {
        return FALSE;
    }
    if ((segment_index >= NDS_NATIVE_STAGE_SEGMENT_COUNT) ||
        (segment_index != sNdsNativeStageOwnerExecution.next_segment) ||
        (stats == NULL))
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3PostArmFailureCount++;
#endif
        return TRUE;
    }
    segment = &sNdsNativeStageSegments[segment_index];
    /* The prepared table outlives the cache entries it names. Validate before
     * this segment's first GX or renderer-state write so a recycled slot falls
     * back as one source-owned segment, never as a mixed native/source segment
     * after earlier runs have already emitted. BeginRun repeats this check as
     * the defensive last gate at the point of use.
     *
     * R2-07 leg A: the per-segment sweep is now the whole-table proof, which is
     * one word compare while the epoch holds. It is deliberately WIDER than the
     * loop it replaces -- a broken run in any segment now rejects every segment
     * rather than only its own. That is the conservative direction (the whole
     * stage falls back to source together, which is exactly the mixed-segment
     * outcome the check exists to prevent), and on the measured arm it is
     * unreachable: the cache does not move during a match. */
    if (ndsRendererNativeStagePreparedTexturesProven() == FALSE)
    {
        return FALSE;
    }
#if NDS_DREAMLAND_DS_MESH
    /* Generator owner order is layer0,map0,map1,map2,layer1,layer2,map3,layer3.
     * Replace each static stage_geometry segment in place and leave every
     * stage_actors map segment live so the original painter order is retained. */
    if ((segment_index == 0u) || (segment_index == 4u) ||
        (segment_index == 5u) || (segment_index == 7u))
    {
        u32 group_count_before = gNdsDreamLandDSGroups;
        u32 segment_groups;

        sNdsRendererRuntimeOwner = NDS_RENDERER_PROFILE_OWNER_STAGE;
        segment_triangles = ndsRendererDreamLandDrawStatic3D(
            stats,
            segment_index);
        segment_groups = gNdsDreamLandDSGroups - group_count_before;
        sNdsRendererFastRunCount += segment_groups;
        sNdsRendererFastTriangleCount += segment_triangles;
        sNdsRendererFastOwnerTriangleCount[
            NDS_RENDERER_PROFILE_OWNER_STAGE] += segment_triangles;
        sNdsNativeStageOwnerExecution.next_segment++;
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3SegmentCount++;
        gNdsRendererM3SegmentMask |= (u32)1u << segment_index;
        gNdsRendererM3RunCount += segment_groups;
        gNdsRendererM3TriangleCount += segment_triangles;
#endif
        sNdsRendererRuntimeOwner = NDS_RENDERER_PROFILE_OWNER_NONE;
        return TRUE;
    }
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    if (segment_index == 0u)
    {
        ndsRendererBenchmarkSegment0SinkBegin();
    }
#endif
    sNdsRendererRuntimeOwner = NDS_RENDERER_PROFILE_OWNER_STAGE;
#if NDS_RENDERER_SCREEN_SPACE_CENSUS
    ndsRendererScreenSpaceCensusStageSegment(segment);
    if (gNdsRendererScreenSpaceCensusArmed != 0u)
    {
        census_owner_start = cpuGetTiming();
    }
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    commit_start = ndsRendererM3Phase0Tick();
#endif
#if NDS_TASK34_STAGE_STREAM_CENSUS
    ndsRendererTask34StageStreamBeginSegment(segment_index);
#endif
#if NDS_TASK36_HW_COMPOSE == 2
    task36_capture_segment =
        (sNdsRendererTask36ReplayOwner.frame_capture != FALSE) &&
        (ndsRendererTask36ReplaySegmentEligible(segment_index) != FALSE);
    task36_replay_segment =
        (sNdsRendererTask36ReplayOwner.frame_replay != FALSE) &&
        (ndsRendererTask36ReplaySegmentEligible(segment_index) != FALSE);
    if ((task36_capture_segment || task36_replay_segment) &&
        (ndsRendererNativeStageTask36BeginSegment() == FALSE))
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3PostArmFailureCount++;
#endif
        return TRUE;
    }
#endif
#if NDS_RENDER_ECONOMY
    if ((gNdsRendererEconomyActiveOwnerMask &
         ((u32)1u << segment->owner)) != 0u)
    {
        segment_triangles = ndsRendererEconomySkipNativeStageSegment(
            segment, stats);
    }
    else
#endif
    for (head_pass = 0u; head_pass < head_pass_count; head_pass++)
    for (run_offset = 0u; run_offset < segment->run_count; run_offset++)
    {
        u32 run_index = (u32)segment->first_run + run_offset;
        const NDSNativeStageRun *run = &sNdsNativeStageRuns[run_index];
        const NDSNativeStagePreparedRun *prepared_run =
            &sNdsNativeStageOwnerExecution.runs[run_index];
        u32 emitted_triangles = 0u;
        u32 triangle_offset;
        if ((binding_heads != NULL) &&
            (binding_heads[run->binding_index] != head_order[head_pass]))
        {
            continue;
        }
#if NDS_TASK103_STAGE_RUN_PHASE
        u32 task103_iter_start = cpuGetTiming();
        u32 task103_generic_start = 0u;
        u32 task103_generic_armed = 0u;
#endif
#if NDS_DREAMLAND_CARD_CULL
        /* Task 63 §5 visualization: suppress whole stage runs named by the
         * mask so the owner can see what an authorised scenery reduction
         * costs. The mask is written over GDB by the capture harness; at its
         * default 0 nothing is culled and this renders exactly like flag=0. */
        if ((gNdsDreamLandCardCullMask[run_index >> 5u] &
             ((u32)1u << (run_index & 31u))) != 0u)
        {
            gNdsDreamLandCardCullSkippedRuns++;
            gNdsDreamLandCardCullSkippedTris += run->triangle_count;
            continue;
        }
#endif
#if NDS_TASK34_STAGE_STREAM_CENSUS
        u32 task34_dobj_index;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
        u32 phase_start = ndsRendererM3Phase0Tick();
#endif

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
        if (segment_index == 0u)
        {
            ndsRendererBenchmarkSegment0ArmRun(
                run_offset, run, prepared_run);
        }
#endif
#if NDS_TASK34_STAGE_STREAM_CENSUS
        for (task34_dobj_index = 0u;
             task34_dobj_index < NDS_NATIVE_STAGE_DOBJ_COUNT;
             task34_dobj_index++)
        {
            if (sNdsNativeStageDObjs[task34_dobj_index].binding_index ==
                run->binding_index)
            {
                break;
            }
        }
        ndsRendererTask34StageStreamSetDObj(
            (task34_dobj_index < NDS_NATIVE_STAGE_DOBJ_COUNT) ?
                task34_dobj_index : NDS_TASK34_STAGE_STREAM_DOBJ_NONE);
#endif
#if NDS_TASK36_HW_COMPOSE == 2
        if (task36_replay_segment != FALSE)
        {
            if (ndsRendererTask36ReplayRun(
                    run_index, run, segment->owner, stats) == FALSE)
            {
#if NDS_RENDERER_PROFILE_LEVEL == 1
                gNdsRendererM3PostArmFailureCount++;
#endif
                return TRUE;
            }
#if NDS_RENDERER_M3_PHASE0_PROFILE
            ndsRendererM3Phase0FinishSpan(
                &gNdsRendererM3Phase0RunTransitionTicks, phase_start);
            phase_start = ndsRendererM3Phase0Tick();
#endif
            if (run->submit_class ==
                NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                sNdsRendererHardwareProjectedDepth -=
                    (s32)run->triangle_count *
                    NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;
            }
            else
            {
                ndsRendererHardwareEnterProjectedForeground();
            }
            emitted_triangles = run->triangle_count;
#if NDS_RENDERER_M3_PHASE0_PROFILE
            if (run->submit_class ==
                NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
            {
                ndsRendererM3Phase0FinishSpan(
                    &gNdsRendererM3Phase0RawEmitTicks, phase_start);
            }
            else if (run->submit_class ==
                     NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
            {
                ndsRendererM3Phase0FinishSpan(
                    &gNdsRendererM3Phase0RangeEmitTicks, phase_start);
            }
            else
            {
                ndsRendererM3Phase0FinishSpan(
                    &gNdsRendererM3Phase0NoZEmitTicks, phase_start);
            }
            phase_start = ndsRendererM3Phase0Tick();
#endif
            goto task36_account_run;
        }
        if (task36_capture_segment != FALSE)
        {
            ndsRendererTask36ReplayCaptureBeginRun(run_index);
        }
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
        task103_generic_start = cpuGetTiming();
        task103_generic_armed = 1u;
#endif
        if (ndsRendererNativeStageBeginRun(
                run, prepared_run, run->submit_class, segment->owner, stats,
                FALSE) == FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
            return TRUE;
        }
#if NDS_TASK103_STAGE_RUN_PHASE
        gNdsTask103GenericBeginTicks += cpuGetTiming() - task103_generic_start;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
        ndsRendererM3Phase0FinishSpan(
            &gNdsRendererM3Phase0RunTransitionTicks, phase_start);
        phase_start = ndsRendererM3Phase0Tick();
#endif
        for (triangle_offset = 0u;
             triangle_offset < run->triangle_count;
             triangle_offset++)
        {
            s16 no_z = (run->submit_class ==
                NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ?
                ndsRendererHardwareNextProjectedDepth() : 0;
            u32 corner_offset;

            if (run->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                emitted_triangles += ndsRendererNativeStageEmitNoZTriangle(
                    run, prepared_run, triangle_offset, no_z);
                continue;
            }

            for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
            {
                u32 dense_index = sNdsNativeStageCorners[
                    (u32)run->first_corner + triangle_offset * 3u +
                    corner_offset];
                const NDSNativeStageDenseVertex *dense =
                    &sNdsNativeStageVertices[dense_index];
                const NDSNativeStagePreparedDense *prepared =
                    &sNdsNativeStagePreparedDense[dense_index];
                ndsRendererNativeStageEmitVertex(
                    dense, prepared, prepared_run, run->submit_class);
            }
            emitted_triangles++;
            if (run->submit_class !=
                NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                ndsRendererHardwareEnterProjectedForeground();
            }
        }
#if NDS_RENDERER_M3_PHASE0_PROFILE
        if (run->submit_class ==
            NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
        {
            ndsRendererM3Phase0FinishSpan(
                &gNdsRendererM3Phase0RawEmitTicks, phase_start);
        }
        else if (run->submit_class ==
                 NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
        {
            ndsRendererM3Phase0FinishSpan(
                &gNdsRendererM3Phase0RangeEmitTicks, phase_start);
        }
        else
        {
            ndsRendererM3Phase0FinishSpan(
                &gNdsRendererM3Phase0NoZEmitTicks, phase_start);
        }
        phase_start = ndsRendererM3Phase0Tick();
#endif
        ndsRendererHardwareEndBatch();
#if NDS_TASK103_STAGE_RUN_PHASE
        if (task103_generic_armed != 0u)
        {
            u32 task103_generic_span =
                cpuGetTiming() - task103_generic_start;

            gNdsTask103GenericTicks += task103_generic_span;
            gNdsTask103GenericRunCount++;
            gNdsTask103GenericTriangles += run->triangle_count;
            gNdsTask103GenericSegTicks[segment_index] += task103_generic_span;
            gNdsTask103GenericSegRuns[segment_index]++;
            gNdsTask103GenericSegTris[segment_index] += run->triangle_count;
        }
#endif
#if NDS_TASK36_HW_COMPOSE == 2
        if (task36_capture_segment != FALSE)
        {
            ndsRendererTask36ReplayCaptureEndRun(run_index);
        }
task36_account_run:
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
        if (segment_index == 0u)
        {
            ndsRendererBenchmarkSegment0CheckpointRun(run_offset);
        }
#endif
        ndsRendererNativeStageAccountRun(
            stats,
            (run->submit_class ==
             NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
                NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX :
                run->submit_class,
            emitted_triangles);
        stats->triangle_count += run->triangle_count;
        segment_triangles += run->triangle_count;
#if NDS_RENDERER_M3_PHASE0_PROFILE
        ndsRendererM3Phase0FinishSpan(
            &gNdsRendererM3Phase0AccountingTicks, phase_start);
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
        gNdsTask103IterTicks += cpuGetTiming() - task103_iter_start;
        gNdsTask103IterCount++;
#endif
    }
#if NDS_TASK36_HW_COMPOSE
    ndsRendererNativeStageTask36EndSegment();
#endif
#if NDS_TASK36_HW_COMPOSE == 2
    if (task36_capture_segment != FALSE)
    {
        sNdsRendererTask36ReplayOwner.captured_segment_mask |=
            1u << segment_index;
    }
    if (task36_replay_segment != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask36ReplaySegmentCount++;
#endif
    }
#endif
#if NDS_TASK34_STAGE_STREAM_CENSUS
    ndsRendererTask34StageStreamEndSegment();
#endif
    sNdsRendererFastRunCount += segment->run_count;
    sNdsRendererFastTriangleCount += segment_triangles;
    sNdsRendererFastOwnerTriangleCount[
        NDS_RENDERER_PROFILE_OWNER_STAGE] += segment_triangles;
    sNdsNativeStageOwnerExecution.next_segment++;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3SegmentCount++;
    gNdsRendererM3SegmentMask |= (u32)1u << segment_index;
    gNdsRendererM3RunCount += segment->run_count;
    gNdsRendererM3TriangleCount += segment_triangles;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0FinishSpan(
        &gNdsRendererM3Phase0CommitTicks, commit_start);
#endif
#if NDS_RENDERER_SCREEN_SPACE_CENSUS
    if (census_owner_start != 0u)
    {
        gNdsRendererScreenSpaceStageOwnerTicks[segment->owner] +=
            cpuGetTiming() - census_owner_start;
    }
#endif
    sNdsRendererRuntimeOwner = NDS_RENDERER_PROFILE_OWNER_NONE;
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    if (segment_index == 0u)
    {
        ndsRendererBenchmarkSegment0SinkEnd();
        sNdsRendererBenchmarkSegment0TextureValid = FALSE;
    }
    ndsRendererBenchmarkSinkEndOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
    return TRUE;
}

void ndsRendererFinishNativeStageOwner(void)
{
#if NDS_TASK36_HW_COMPOSE == 2
    ndsRendererTask36ReplayFinishFrame();
#endif
    if (sNdsNativeStageOwnerExecution.active != FALSE)
    {
#if NDS_TASK36_HW_COMPOSE
        ndsRendererNativeStageTask36EndSegment();
#endif
        ndsRendererHardwareEndBatch();
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
        ndsRendererBenchmarkSinkEndOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
        if ((sNdsNativeStageOwnerExecution.next_segment !=
             NDS_NATIVE_STAGE_SEGMENT_COUNT) ||
            (sNdsNativeStageOwnerExecution.stats == NULL) ||
            (sNdsNativeStageOwnerExecution.stats->triangle_count !=
#if NDS_DREAMLAND_DS_MESH
             (NDS_DREAMLAND_DS_TRIANGLE_COUNT + 27u)))
#else
             NDS_NATIVE_STAGE_TRIANGLE_COUNT))
#endif
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
        }
    }
    sNdsNativeStageOwnerExecution.stats = NULL;
    sNdsNativeStageOwnerExecution.binding_composed = NULL;
#if NDS_TASK36_HW_COMPOSE
    sNdsNativeStageOwnerExecution.projection = NULL;
    sNdsNativeStageOwnerExecution.camera_modelview = NULL;
    sNdsNativeStageOwnerExecution.binding_world = NULL;
    sNdsNativeStageOwnerExecution.rigid_binding_mask = 0u;
#endif
    sNdsNativeStageOwnerExecution.next_segment = 0u;
    sNdsNativeStageOwnerExecution.active = FALSE;
    sNdsRendererRuntimeOwner = NDS_RENDERER_PROFILE_OWNER_NONE;
#if NDS_DREAMLAND_DS_MESH
#endif
}
#else
s32 ndsRendererPrepareNativeStageOwner(
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    (void)frame;
    (void)stats;
    return FALSE;
}

s32 ndsRendererCommitNativeStageSegment(u32 segment_index)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    (void)segment_index;
    return FALSE;
}

void ndsRendererFinishNativeStageOwner(void)
{
}

void ndsRendererResetNativeStageValidationCache(void)
{
}
#endif
