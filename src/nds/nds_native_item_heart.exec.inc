/* Heart Container executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x0FF8 has no 0xDE; both material/geometry phases are baked. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemHeart(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemHeartSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemHeartSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_HEART_FILE_END)
    {
        gNdsItemHeartSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemHeartSetup0(stats,
        base + NDS_NATIVE_ITEM_HEART_TLUT_OFFSET,
        base + NDS_NATIVE_ITEM_HEART_IMAGE0_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemHeart0Verts, sNdsNativeItemHeart0VertColors,
            sNdsNativeItemHeart0TriIndices,
            NDS_NATIVE_ITEM_HEART_VERTEX0_COUNT,
            NDS_NATIVE_ITEM_HEART_TRIANGLE0_COUNT,
            NDS_NATIVE_ITEM_HEART_CORNER0_COUNT,
            config, stats, &state,
            &gNdsItemHeartSubmitStep, &gNdsItemHeartAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemHeartSetup1(stats,
        base + NDS_NATIVE_ITEM_HEART_IMAGE1_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemHeart1Verts, sNdsNativeItemHeart1VertColors,
            sNdsNativeItemHeart1TriIndices,
            NDS_NATIVE_ITEM_HEART_VERTEX1_COUNT,
            NDS_NATIVE_ITEM_HEART_TRIANGLE1_COUNT,
            NDS_NATIVE_ITEM_HEART_CORNER1_COUNT,
            config, stats, &state,
            &gNdsItemHeartSubmitStep, &gNdsItemHeartAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemHeartFinish(stats);
    gNdsItemHeartSubmitStep = 9u;
    return TRUE;
}

#endif
