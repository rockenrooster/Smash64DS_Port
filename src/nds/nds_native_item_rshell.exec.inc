/* Red Shell executor. TEXTUAL INCLUDE ONLY.
 * The Red Shell uses the exact same file-86 DD/root/material tables as the
 * Green Shell, so its generated packet aliases the Green Shell packet data. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemRShell(
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
        gNdsItemRShellSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemRShellSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_RSHELL_FILE_END)
    {
        gNdsItemRShellSubmitStep = 2u;
        return FALSE;
    }
    if (material->effects != expected_effects)
    {
        gNdsItemRShellEffectsRejected = material->effects;
        gNdsItemRShellSubmitStep = 13u;
        return FALSE;
    }
    for (i = 0u; i < NDS_NATIVE_ITEM_RSHELL_IMAGE_COUNT; i++)
    {
        if (material->current_image ==
                (u32)(uintptr_t)(base + sNdsNativeItemRShellImageOffsets[i]))
        {
            image_ok = TRUE;
            break;
        }
    }
    for (i = 0u; i < NDS_NATIVE_ITEM_RSHELL_PALETTE_COUNT; i++)
    {
        if (material->palette_image ==
                (u32)(uintptr_t)(base + sNdsNativeItemRShellPaletteOffsets[i]))
        {
            palette_ok = TRUE;
            break;
        }
    }
    if ((image_ok == FALSE) || (palette_ok == FALSE))
    {
        gNdsItemRShellSubmitStep = 14u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemRShellSetup(stats);
    ndsRendererNativeApplyMaterial(material, stats, &state);
    ndsNativeItemRShellAfterMaterial(stats);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemRShellVerts, sNdsNativeItemRShellVertColors,
            sNdsNativeItemRShellTriIndices,
            NDS_NATIVE_ITEM_RSHELL_VERTEX_COUNT,
            NDS_NATIVE_ITEM_RSHELL_TRIANGLE_COUNT,
            NDS_NATIVE_ITEM_RSHELL_CORNER_COUNT,
            config, stats, &state,
            &gNdsItemRShellSubmitStep, &gNdsItemRShellAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemRShellFinish(stats);
    gNdsItemRShellSubmitStep = 9u;
    return TRUE;
}

#endif
