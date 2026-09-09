/* Bob-omb / BombHei executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x3310 has exactly one word-17 segment-0xE call. The source
 * MObj flags 0x0001 make CURRENT_IMAGE the only live material input. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemBombHei(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;
    u32 image_ok = FALSE;
    u32 i;

    if ((base == NULL) || (material == NULL) ||
        (config == NULL) || (stats == NULL))
    {
        gNdsItemBombHeiSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemBombHeiSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_BOMBHEI_FILE_END)
    {
        gNdsItemBombHeiSubmitStep = 2u;
        return FALSE;
    }
    if (material->effects != NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
    {
        gNdsItemBombHeiEffectsRejected = material->effects;
        gNdsItemBombHeiSubmitStep = 13u;
        return FALSE;
    }
    for (i = 0u; i < NDS_NATIVE_ITEM_BOMBHEI_IMAGE_COUNT; i++)
    {
        if (material->current_image ==
                (u32)(uintptr_t)(base + sNdsNativeItemBombHeiImageOffsets[i]))
        {
            image_ok = TRUE;
            break;
        }
    }
    if (image_ok == FALSE)
    {
        gNdsItemBombHeiSubmitStep = 14u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemBombHeiSetup(stats,
        base + NDS_NATIVE_ITEM_BOMBHEI_TLUT_OFFSET);
    ndsRendererNativeApplyMaterial(material, stats, &state);
    ndsNativeItemBombHeiAfterMaterial(stats);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemBombHeiVerts, sNdsNativeItemBombHeiVertColors,
            sNdsNativeItemBombHeiTriIndices,
            NDS_NATIVE_ITEM_BOMBHEI_VERTEX_COUNT,
            NDS_NATIVE_ITEM_BOMBHEI_TRIANGLE_COUNT,
            NDS_NATIVE_ITEM_BOMBHEI_CORNER_COUNT,
            config, stats, &state,
            &gNdsItemBombHeiSubmitStep, &gNdsItemBombHeiAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemBombHeiFinish(stats);
    gNdsItemBombHeiSubmitStep = 9u;
    return TRUE;
}

#endif
