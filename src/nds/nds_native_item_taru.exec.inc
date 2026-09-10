/* Barrel / Taru executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x7000 contains no 0xDE; both fixed material/geometry phases
 * are baked into the generated packet. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemTaru(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemTaruSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemTaruSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_TARU_FILE_END)
    {
        gNdsItemTaruSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemTaruSetup0(stats, &state,
        base + NDS_NATIVE_ITEM_TARU_TLUT0_OFFSET,
        base + NDS_NATIVE_ITEM_TARU_IMAGE0_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemTaru0Verts, sNdsNativeItemTaru0VertColors,
            sNdsNativeItemTaru0TriIndices,
            NDS_NATIVE_ITEM_TARU_VERTEX0_COUNT,
            NDS_NATIVE_ITEM_TARU_TRIANGLE0_COUNT,
            NDS_NATIVE_ITEM_TARU_CORNER0_COUNT,
            config, stats, &state,
            &gNdsItemTaruSubmitStep, &gNdsItemTaruAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemTaruSetup1(stats, &state,
        base + NDS_NATIVE_ITEM_TARU_TLUT1_OFFSET,
        base + NDS_NATIVE_ITEM_TARU_IMAGE1_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemTaru1Verts, sNdsNativeItemTaru1VertColors,
            sNdsNativeItemTaru1TriIndices,
            NDS_NATIVE_ITEM_TARU_VERTEX1_COUNT,
            NDS_NATIVE_ITEM_TARU_TRIANGLE1_COUNT,
            NDS_NATIVE_ITEM_TARU_CORNER1_COUNT,
            config, stats, &state,
            &gNdsItemTaruSubmitStep, &gNdsItemTaruAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemTaruFinish(stats);
    gNdsItemTaruSubmitStep = 9u;
    return TRUE;
}

#endif
