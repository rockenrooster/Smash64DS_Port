/* Poké Ball Onix / Iwark executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0xA050 contains no 0xDE; the complete fixed material/geometry
 * program is baked into the generated packet. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemIwark(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemIwarkSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemIwarkSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_IWARK_FILE_END)
    {
        gNdsItemIwarkSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemIwarkSetup(stats,
        base + NDS_NATIVE_ITEM_IWARK_TLUT_OFFSET,
        base + NDS_NATIVE_ITEM_IWARK_IMAGE_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemIwarkVerts, sNdsNativeItemIwarkVertColors,
            sNdsNativeItemIwarkTriIndices,
            NDS_NATIVE_ITEM_IWARK_VERTEX_COUNT,
            NDS_NATIVE_ITEM_IWARK_TRIANGLE_COUNT,
            NDS_NATIVE_ITEM_IWARK_CORNER_COUNT,
            config, stats, &state,
            &gNdsItemIwarkSubmitStep, &gNdsItemIwarkAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemIwarkFinish(stats);
    gNdsItemIwarkSubmitStep = 9u;
    return TRUE;
}

#endif
