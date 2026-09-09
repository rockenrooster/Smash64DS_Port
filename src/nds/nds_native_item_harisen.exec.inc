/* Fan / Harisen executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x20A0 contains no 0xDE; the complete fixed material and
 * geometry are baked into the generated packet. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemHarisen(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemHarisenSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemHarisenSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_HARISEN_FILE_END)
    {
        gNdsItemHarisenSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemHarisenSetup(stats, &state,
        base + NDS_NATIVE_ITEM_HARISEN_TLUT_OFFSET,
        base + NDS_NATIVE_ITEM_HARISEN_IMAGE_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemHarisenVerts, sNdsNativeItemHarisenVertColors,
            sNdsNativeItemHarisenTriIndices,
            NDS_NATIVE_ITEM_HARISEN_VERTEX_COUNT,
            NDS_NATIVE_ITEM_HARISEN_TRIANGLE_COUNT,
            NDS_NATIVE_ITEM_HARISEN_CORNER_COUNT,
            config, stats, &state,
            &gNdsItemHarisenSubmitStep, &gNdsItemHarisenAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemHarisenFinish(stats);
    gNdsItemHarisenSubmitStep = 9u;
    return TRUE;
}

#endif
