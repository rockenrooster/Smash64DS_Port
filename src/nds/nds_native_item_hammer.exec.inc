/* Hammer executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x25F0 has no 0xDE opcode; its TLUT, image, state, and both
 * geometry phases are immutable source data and bake into this owner. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemHammer(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    const void *tlut;
    const void *image;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemHammerSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemHammerSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_HAMMER_FILE_END)
    {
        gNdsItemHammerSubmitStep = 2u;
        return FALSE;
    }

    tlut = base + NDS_NATIVE_ITEM_HAMMER_TLUT_OFFSET;
    image = base + NDS_NATIVE_ITEM_HAMMER_IMAGE_OFFSET;
    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemHammerSetup0(stats, &state, tlut, image);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemHammer0Verts, sNdsNativeItemHammer0VertColors,
            sNdsNativeItemHammer0TriIndices,
            NDS_NATIVE_ITEM_HAMMER_VERTEX0_COUNT,
            NDS_NATIVE_ITEM_HAMMER_TRIANGLE0_COUNT,
            NDS_NATIVE_ITEM_HAMMER_CORNER0_COUNT,
            config, stats, &state,
            &gNdsItemHammerSubmitStep, &gNdsItemHammerAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemHammerSetup1(stats, &state);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemHammer1Verts, sNdsNativeItemHammer1VertColors,
            sNdsNativeItemHammer1TriIndices,
            NDS_NATIVE_ITEM_HAMMER_VERTEX1_COUNT,
            NDS_NATIVE_ITEM_HAMMER_TRIANGLE1_COUNT,
            NDS_NATIVE_ITEM_HAMMER_CORNER1_COUNT,
            config, stats, &state,
            &gNdsItemHammerSubmitStep, &gNdsItemHammerAlpha) == FALSE)
    {
        return FALSE;
    }
    gNdsItemHammerSubmitStep = 9u;
    return TRUE;
}

#endif
