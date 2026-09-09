/* Star executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x1440 has two exact segment-E calls: word 8 targets
 * 0x0E000000 and word 23 targets 0x0E000008. Both source MObjs are
 * PALETTE_IMAGE-only, so each hook is translated through a separate typed
 * material snapshot and the executor refuses any broader effect word. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemStar(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material0,
    const NDSRendererNativeMaterial *material1,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    const void *image;
    NDSRendererTraversalState state;
    u32 palette0_ok = FALSE;
    u32 palette1_ok = FALSE;
    u32 i;

    if ((base == NULL) || (material0 == NULL) || (material1 == NULL) ||
        (config == NULL) || (stats == NULL))
    {
        gNdsItemStarSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemStarSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_STAR_FILE_END)
    {
        gNdsItemStarSubmitStep = 2u;
        return FALSE;
    }
    if (material0->effects != NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
    {
        gNdsItemStarEffectsRejected = material0->effects;
        gNdsItemStarSubmitStep = 13u;
        return FALSE;
    }
    if (material1->effects != NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
    {
        gNdsItemStarEffectsRejected = material1->effects;
        gNdsItemStarSubmitStep = 14u;
        return FALSE;
    }
    for (i = 0u; i < 2u; i++)
    {
        u32 expected = (u32)(uintptr_t)(base + sNdsNativeItemStarPaletteOffsets[i]);
        if (material0->palette_image == expected)
        {
            palette0_ok = TRUE;
        }
        if (material1->palette_image == expected)
        {
            palette1_ok = TRUE;
        }
    }
    if ((palette0_ok == FALSE) || (palette1_ok == FALSE))
    {
        gNdsItemStarSubmitStep = 15u;
        return FALSE;
    }

    image = base + NDS_NATIVE_ITEM_STAR_IMAGE_OFFSET;
    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemStarSetup(stats);
    ndsRendererNativeApplyMaterial(material0, stats, &state);
    ndsNativeItemStarAfterMaterial0(stats, image);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemStar0Verts, sNdsNativeItemStar0VertColors,
            sNdsNativeItemStar0TriIndices,
            NDS_NATIVE_ITEM_STAR_VERTEX0_COUNT,
            NDS_NATIVE_ITEM_STAR_TRIANGLE0_COUNT,
            NDS_NATIVE_ITEM_STAR_CORNER0_COUNT,
            config, stats, &state,
            &gNdsItemStarSubmitStep, &gNdsItemStarAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemStarBeforeMaterial1(stats);
    ndsRendererNativeApplyMaterial(material1, stats, &state);
    ndsNativeItemStarAfterMaterial1(stats);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemStar1Verts, sNdsNativeItemStar1VertColors,
            sNdsNativeItemStar1TriIndices,
            NDS_NATIVE_ITEM_STAR_VERTEX1_COUNT,
            NDS_NATIVE_ITEM_STAR_TRIANGLE1_COUNT,
            NDS_NATIVE_ITEM_STAR_CORNER1_COUNT,
            config, stats, &state,
            &gNdsItemStarSubmitStep, &gNdsItemStarAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemStarFinish(stats);
    gNdsItemStarSubmitStep = 9u;
    return TRUE;
}

#endif
