/* Bumper / NBumper executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x7558 has one word-10 segment-0xE palette hook. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemNBumper(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;
    u32 palette_ok = FALSE;
    u32 i;

    if ((base == NULL) || (material == NULL) ||
        (config == NULL) || (stats == NULL))
    {
        gNdsItemNBumperSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemNBumperSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_NBUMPER_FILE_END)
    {
        gNdsItemNBumperSubmitStep = 2u;
        return FALSE;
    }
    if (material->effects != NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
    {
        gNdsItemNBumperEffectsRejected = material->effects;
        gNdsItemNBumperSubmitStep = 13u;
        return FALSE;
    }
    for (i = 0u; i < NDS_NATIVE_ITEM_NBUMPER_PALETTE_COUNT; i++)
    {
        u32 expected = (u32)(uintptr_t)(
            base + sNdsNativeItemNBumperPaletteOffsets[i]);
        if (material->palette_image == expected)
        {
            palette_ok = TRUE;
        }
    }
    if (palette_ok == FALSE)
    {
        gNdsItemNBumperSubmitStep = 14u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemNBumperSetup(stats);
    ndsRendererNativeApplyMaterial(material, stats, &state);
    ndsNativeItemNBumperAfterMaterial(stats,
        base + NDS_NATIVE_ITEM_NBUMPER_IMAGE_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemNBumperVerts, sNdsNativeItemNBumperVertColors,
            sNdsNativeItemNBumperTriIndices,
            NDS_NATIVE_ITEM_NBUMPER_VERTEX_COUNT,
            NDS_NATIVE_ITEM_NBUMPER_TRIANGLE_COUNT,
            NDS_NATIVE_ITEM_NBUMPER_CORNER_COUNT,
            config, stats, &state,
            &gNdsItemNBumperSubmitStep, &gNdsItemNBumperAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemNBumperFinish(stats);
    gNdsItemNBumperSubmitStep = 9u;
    return TRUE;
}

#endif
