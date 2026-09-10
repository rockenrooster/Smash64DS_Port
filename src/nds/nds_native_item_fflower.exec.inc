/* Fire Flower executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x4520 branches at word 9 to relocated 0x4578. Root 0x4608
 * has a word-3 segment-0xE palette hook driven by its one live MObj. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemFFlower(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;
    u32 palette_ok = FALSE;
    u32 i;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemFFlowerSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemFFlowerSubmitStep = 12u;
        return FALSE;
    }
    if ((file_bytes < NDS_NATIVE_ITEM_FFLOWER_FILE_END) ||
        ((root_offset != NDS_NATIVE_ITEM_FFLOWER_BRANCH_ROOT) &&
         (root_offset != NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT)))
    {
        gNdsItemFFlowerSubmitStep = 2u;
        return FALSE;
    }
    if (root_offset == NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT)
    {
        if (material == NULL)
        {
            gNdsItemFFlowerSubmitStep = 13u;
            return FALSE;
        }
        if (material->effects != NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
        {
            gNdsItemFFlowerEffectsRejected = material->effects;
            gNdsItemFFlowerSubmitStep = 14u;
            return FALSE;
        }
        for (i = 0u; i < NDS_NATIVE_ITEM_FFLOWER_PALETTE_COUNT; i++)
        {
            u32 expected = (u32)(uintptr_t)(
                base + sNdsNativeItemFFlowerPaletteOffsets[i]);
            if (material->palette_image == expected)
            {
                palette_ok = TRUE;
            }
        }
        if (palette_ok == FALSE)
        {
            gNdsItemFFlowerSubmitStep = 15u;
            return FALSE;
        }
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    if (root_offset == NDS_NATIVE_ITEM_FFLOWER_BRANCH_ROOT)
    {
        ndsNativeItemFFlowerBranchSetup(stats, &state,
            base + NDS_NATIVE_ITEM_FFLOWER_BRANCH_TLUT_OFFSET,
            base + NDS_NATIVE_ITEM_FFLOWER_BRANCH_IMAGE_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemFFlower0Verts, sNdsNativeItemFFlower0VertColors,
                sNdsNativeItemFFlower0TriIndices,
                NDS_NATIVE_ITEM_FFLOWER_VERTEX0_COUNT,
                NDS_NATIVE_ITEM_FFLOWER_TRIANGLE0_COUNT,
                NDS_NATIVE_ITEM_FFLOWER_CORNER0_COUNT,
                config, stats, &state,
                &gNdsItemFFlowerSubmitStep, &gNdsItemFFlowerAlpha) == FALSE)
        {
            return FALSE;
        }
    }
    else
    {
        ndsNativeItemFFlowerLiveBeforeMaterial(stats);
        ndsRendererNativeApplyMaterial(material, stats, &state);
        ndsNativeItemFFlowerLiveAfterMaterial(stats,
            base + NDS_NATIVE_ITEM_FFLOWER_LIVE_IMAGE_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemFFlower1Verts, sNdsNativeItemFFlower1VertColors,
                sNdsNativeItemFFlower1TriIndices,
                NDS_NATIVE_ITEM_FFLOWER_VERTEX1_COUNT,
                NDS_NATIVE_ITEM_FFLOWER_TRIANGLE1_COUNT,
                NDS_NATIVE_ITEM_FFLOWER_CORNER1_COUNT,
                config, stats, &state,
                &gNdsItemFFlowerSubmitStep, &gNdsItemFFlowerAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemFFlowerLiveFinish(stats);
    }
    gNdsItemFFlowerSubmitStep = 9u;
    return TRUE;
}

#endif
