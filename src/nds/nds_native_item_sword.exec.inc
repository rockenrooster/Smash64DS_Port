/* Beam Sword executor. TEXTUAL INCLUDE ONLY.
 * File 86 roots 0x17D8 and 0x1850 contain no 0xDE opcode. The blade and
 * hilt therefore bake all source presentation; no typed material is accepted. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemSword(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemSwordSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemSwordSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_SWORD_FILE_END)
    {
        gNdsItemSwordSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    if (root_offset == NDS_NATIVE_ITEM_SWORD_BLADE_ROOT)
    {
        ndsNativeItemSwordBladeSetup(stats, &state);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemSwordBladeVerts,
                sNdsNativeItemSwordBladeVertColors,
                sNdsNativeItemSwordBladeTriIndices,
                NDS_NATIVE_ITEM_SWORD_BLADE_VERTEX_COUNT,
                NDS_NATIVE_ITEM_SWORD_BLADE_TRIANGLE_COUNT,
                NDS_NATIVE_ITEM_SWORD_BLADE_CORNER_COUNT,
                config, stats, &state,
                &gNdsItemSwordSubmitStep, &gNdsItemSwordAlpha) == FALSE)
        {
            return FALSE;
        }
    }
    else if (root_offset == NDS_NATIVE_ITEM_SWORD_HILT_ROOT)
    {
        const void *image = base + NDS_NATIVE_ITEM_SWORD_HILT_IMAGE_OFFSET;

        ndsNativeItemSwordHiltSetup(stats, image);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemSwordHiltVerts,
                sNdsNativeItemSwordHiltVertColors,
                sNdsNativeItemSwordHiltTriIndices,
                NDS_NATIVE_ITEM_SWORD_HILT_VERTEX_COUNT,
                NDS_NATIVE_ITEM_SWORD_HILT_TRIANGLE_COUNT,
                NDS_NATIVE_ITEM_SWORD_HILT_CORNER_COUNT,
                config, stats, &state,
                &gNdsItemSwordSubmitStep, &gNdsItemSwordAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemSwordHiltFinish(stats);
    }
    else
    {
        gNdsItemSwordSubmitStep = 13u;
        return FALSE;
    }
    gNdsItemSwordSubmitStep = 9u;
    return TRUE;
}

#endif
