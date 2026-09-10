/* Star Rod executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x49B0 branches at word 11 to relocated 0x4A18; the generator
 * resolves and bakes that callee. Root 0x4AA0 is the second bake-only DObj. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemStarRod(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemStarRodSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemStarRodSubmitStep = 12u;
        return FALSE;
    }
    if ((file_bytes < NDS_NATIVE_ITEM_STARROD_FILE_END) ||
        ((root_offset != NDS_NATIVE_ITEM_STARROD_HEADER_ROOT) &&
         (root_offset != NDS_NATIVE_ITEM_STARROD_SECOND_ROOT)))
    {
        gNdsItemStarRodSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    if (root_offset == NDS_NATIVE_ITEM_STARROD_HEADER_ROOT)
    {
        ndsNativeItemStarRodHeaderSetup(stats, &state);
        ndsNativeItemStarRodRun0Setup(stats,
            base + NDS_NATIVE_ITEM_STARROD_TLUT0_OFFSET,
            base + NDS_NATIVE_ITEM_STARROD_IMAGE0_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemStarRod0Verts, sNdsNativeItemStarRod0VertColors,
                sNdsNativeItemStarRod0TriIndices,
                NDS_NATIVE_ITEM_STARROD_VERTEX0_COUNT,
                NDS_NATIVE_ITEM_STARROD_TRIANGLE0_COUNT,
                NDS_NATIVE_ITEM_STARROD_CORNER0_COUNT,
                config, stats, &state,
                &gNdsItemStarRodSubmitStep, &gNdsItemStarRodAlpha) == FALSE)
        {
            return FALSE;
        }
    }
    else
    {
        ndsNativeItemStarRodRun1Setup(stats,
            base + NDS_NATIVE_ITEM_STARROD_TLUT1_OFFSET,
            base + NDS_NATIVE_ITEM_STARROD_IMAGE1_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemStarRod1Verts, sNdsNativeItemStarRod1VertColors,
                sNdsNativeItemStarRod1TriIndices,
                NDS_NATIVE_ITEM_STARROD_VERTEX1_COUNT,
                NDS_NATIVE_ITEM_STARROD_TRIANGLE1_COUNT,
                NDS_NATIVE_ITEM_STARROD_CORNER1_COUNT,
                config, stats, &state,
                &gNdsItemStarRodSubmitStep, &gNdsItemStarRodAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemStarRodRun1Finish(stats);
    }
    gNdsItemStarRodSubmitStep = 9u;
    return TRUE;
}

#endif
