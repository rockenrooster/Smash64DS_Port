/* Home-Run Bat executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x1BF0 branches at word 9 to relocated 0x1C48; the generator
 * resolves and bakes that callee. Root 0x1D48 is the second bake-only DObj. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemBat(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemBatSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemBatSubmitStep = 12u;
        return FALSE;
    }
    if ((file_bytes < NDS_NATIVE_ITEM_BAT_FILE_END) ||
        ((root_offset != NDS_NATIVE_ITEM_BAT_HEADER_ROOT) &&
         (root_offset != NDS_NATIVE_ITEM_BAT_SECOND_ROOT)))
    {
        gNdsItemBatSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    if (root_offset == NDS_NATIVE_ITEM_BAT_HEADER_ROOT)
    {
        ndsNativeItemBatHeaderSetup(stats);
        ndsNativeItemBatRun0Setup(stats,
            base + NDS_NATIVE_ITEM_BAT_TLUT0_OFFSET,
            base + NDS_NATIVE_ITEM_BAT_IMAGE0_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemBat0Verts, sNdsNativeItemBat0VertColors,
                sNdsNativeItemBat0TriIndices,
                NDS_NATIVE_ITEM_BAT_VERTEX0_COUNT,
                NDS_NATIVE_ITEM_BAT_TRIANGLE0_COUNT,
                NDS_NATIVE_ITEM_BAT_CORNER0_COUNT,
                config, stats, &state,
                &gNdsItemBatSubmitStep, &gNdsItemBatAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemBatRun1Setup(stats,
            base + NDS_NATIVE_ITEM_BAT_TLUT1_OFFSET,
            base + NDS_NATIVE_ITEM_BAT_IMAGE1_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemBat1Verts, sNdsNativeItemBat1VertColors,
                sNdsNativeItemBat1TriIndices,
                NDS_NATIVE_ITEM_BAT_VERTEX1_COUNT,
                NDS_NATIVE_ITEM_BAT_TRIANGLE1_COUNT,
                NDS_NATIVE_ITEM_BAT_CORNER1_COUNT,
                config, stats, &state,
                &gNdsItemBatSubmitStep, &gNdsItemBatAlpha) == FALSE)
        {
            return FALSE;
        }
    }
    else
    {
        ndsNativeItemBatSecondSetup(stats,
            base + NDS_NATIVE_ITEM_BAT_TLUT0_OFFSET,
            base + NDS_NATIVE_ITEM_BAT_IMAGE0_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemBat2Verts, sNdsNativeItemBat2VertColors,
                sNdsNativeItemBat2TriIndices,
                NDS_NATIVE_ITEM_BAT_VERTEX2_COUNT,
                NDS_NATIVE_ITEM_BAT_TRIANGLE2_COUNT,
                NDS_NATIVE_ITEM_BAT_CORNER2_COUNT,
                config, stats, &state,
                &gNdsItemBatSubmitStep, &gNdsItemBatAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemBatSecondFinish(stats);
    }
    gNdsItemBatSubmitStep = 9u;
    return TRUE;
}

#endif
