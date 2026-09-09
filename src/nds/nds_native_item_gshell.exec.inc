/* Green Shell executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x5EC0 has one exact word-12 call,
 * 0xDE000000 0x0E000000. Its source MObj flags 0x0005 produce exactly
 * PALETTE_IMAGE | PALETTE_TLUT | CURRENT_IMAGE; anything broader declines. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemGShell(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;
    const u32 expected_effects =
        NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE |
        NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT |
        NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;
    u32 image_ok = FALSE;
    u32 palette_ok = FALSE;
    u32 i;

    if ((base == NULL) || (material == NULL) ||
        (config == NULL) || (stats == NULL))
    {
        gNdsItemGShellSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemGShellSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_GSHELL_FILE_END)
    {
        gNdsItemGShellSubmitStep = 2u;
        return FALSE;
    }
    if (material->effects != expected_effects)
    {
        gNdsItemGShellEffectsRejected = material->effects;
        gNdsItemGShellSubmitStep = 13u;
        return FALSE;
    }
    for (i = 0u; i < 4u; i++)
    {
        if (material->current_image ==
                (u32)(uintptr_t)(base + sNdsNativeItemGShellImageOffsets[i]))
        {
            image_ok = TRUE;
            break;
        }
    }
    for (i = 0u; i < 2u; i++)
    {
        if (material->palette_image ==
                (u32)(uintptr_t)(base + sNdsNativeItemGShellPaletteOffsets[i]))
        {
            palette_ok = TRUE;
            break;
        }
    }
    if ((image_ok == FALSE) || (palette_ok == FALSE))
    {
        gNdsItemGShellSubmitStep = 14u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemGShellSetup(stats);
    ndsRendererNativeApplyMaterial(material, stats, &state);
    ndsNativeItemGShellAfterMaterial(stats);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemGShellVerts, sNdsNativeItemGShellVertColors,
            sNdsNativeItemGShellTriIndices,
            NDS_NATIVE_ITEM_GSHELL_VERTEX_COUNT,
            NDS_NATIVE_ITEM_GSHELL_TRIANGLE_COUNT,
            NDS_NATIVE_ITEM_GSHELL_CORNER_COUNT,
            config, stats, &state,
            &gNdsItemGShellSubmitStep, &gNdsItemGShellAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemGShellFinish(stats);
    gNdsItemGShellSubmitStep = 9u;
    return TRUE;
}

#endif
