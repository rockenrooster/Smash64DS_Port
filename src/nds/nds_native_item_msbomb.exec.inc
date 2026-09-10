/* Motion-Sensor Bomb executor. TEXTUAL INCLUDE ONLY.
 * File 86 roots 0x37A0 and 0x38B0 contain no 0xDE and are fully baked. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemMSBomb(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemMSBombSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemMSBombSubmitStep = 12u;
        return FALSE;
    }
    if ((file_bytes < NDS_NATIVE_ITEM_MSBOMB_FILE_END) ||
        ((root_offset != NDS_NATIVE_ITEM_MSBOMB_ROOT0) &&
         (root_offset != NDS_NATIVE_ITEM_MSBOMB_ROOT1)))
    {
        gNdsItemMSBombSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    if (root_offset == NDS_NATIVE_ITEM_MSBOMB_ROOT0)
    {
        ndsNativeItemMSBombSetup0(stats, &state,
            base + NDS_NATIVE_ITEM_MSBOMB_TLUT0_OFFSET,
            base + NDS_NATIVE_ITEM_MSBOMB_IMAGE0_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemMSBomb0Verts, sNdsNativeItemMSBomb0VertColors,
                sNdsNativeItemMSBomb0TriIndices,
                NDS_NATIVE_ITEM_MSBOMB_VERTEX0_COUNT,
                NDS_NATIVE_ITEM_MSBOMB_TRIANGLE0_COUNT,
                NDS_NATIVE_ITEM_MSBOMB_CORNER0_COUNT,
                config, stats, &state,
                &gNdsItemMSBombSubmitStep, &gNdsItemMSBombAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemMSBombFinish0(stats);
    }
    else
    {
        ndsNativeItemMSBombSetup1(stats,
            base + NDS_NATIVE_ITEM_MSBOMB_TLUT1_OFFSET,
            base + NDS_NATIVE_ITEM_MSBOMB_IMAGE1_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemMSBomb1Verts, sNdsNativeItemMSBomb1VertColors,
                sNdsNativeItemMSBomb1TriIndices,
                NDS_NATIVE_ITEM_MSBOMB_VERTEX1_COUNT,
                NDS_NATIVE_ITEM_MSBOMB_TRIANGLE1_COUNT,
                NDS_NATIVE_ITEM_MSBOMB_CORNER1_COUNT,
                config, stats, &state,
                &gNdsItemMSBombSubmitStep, &gNdsItemMSBombAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemMSBombFinish1(stats);
    }
    gNdsItemMSBombSubmitStep = 9u;
    return TRUE;
}

#endif
