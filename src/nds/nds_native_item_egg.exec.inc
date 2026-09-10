/* Egg executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x103B0 contains no 0xDE; the fixed material/geometry program
 * is baked while the source DObj animation arrives in initial_modelview. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemEgg(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemEggSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemEggSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_EGG_FILE_END)
    {
        gNdsItemEggSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemEggSetup(stats,
        base + NDS_NATIVE_ITEM_EGG_TLUT_OFFSET,
        base + NDS_NATIVE_ITEM_EGG_IMAGE_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemEggVerts, sNdsNativeItemEggVertColors,
            sNdsNativeItemEggTriIndices,
            NDS_NATIVE_ITEM_EGG_VERTEX_COUNT,
            NDS_NATIVE_ITEM_EGG_TRIANGLE_COUNT,
            NDS_NATIVE_ITEM_EGG_CORNER_COUNT,
            config, stats, &state,
            &gNdsItemEggSubmitStep, &gNdsItemEggAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemEggFinish(stats);
    gNdsItemEggSubmitStep = 9u;
    return TRUE;
}

#endif
