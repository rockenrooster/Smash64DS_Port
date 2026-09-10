/* Crate / Box executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x6630 contains no 0xDE; both fixed material/geometry phases
 * are baked into the generated packet. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemBox(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemBoxSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemBoxSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_BOX_FILE_END)
    {
        gNdsItemBoxSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemBoxSetup0(stats,
        base + NDS_NATIVE_ITEM_BOX_TLUT0_OFFSET,
        base + NDS_NATIVE_ITEM_BOX_IMAGE0_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemBox0Verts, sNdsNativeItemBox0VertColors,
            sNdsNativeItemBox0TriIndices,
            NDS_NATIVE_ITEM_BOX_VERTEX0_COUNT,
            NDS_NATIVE_ITEM_BOX_TRIANGLE0_COUNT,
            NDS_NATIVE_ITEM_BOX_CORNER0_COUNT,
            config, stats, &state,
            &gNdsItemBoxSubmitStep, &gNdsItemBoxAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemBoxSetup1(stats,
        base + NDS_NATIVE_ITEM_BOX_TLUT1_OFFSET,
        base + NDS_NATIVE_ITEM_BOX_IMAGE1_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemBox1Verts, sNdsNativeItemBox1VertColors,
            sNdsNativeItemBox1TriIndices,
            NDS_NATIVE_ITEM_BOX_VERTEX1_COUNT,
            NDS_NATIVE_ITEM_BOX_TRIANGLE1_COUNT,
            NDS_NATIVE_ITEM_BOX_CORNER1_COUNT,
            config, stats, &state,
            &gNdsItemBoxSubmitStep, &gNdsItemBoxAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemBoxFinish(stats);
    gNdsItemBoxSubmitStep = 9u;
    return TRUE;
}

#endif
