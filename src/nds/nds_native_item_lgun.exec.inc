/* Ray Gun / LGun executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x3DB0 contains no 0xDE. Its three image/geometry phases are
 * immutable and are submitted from generated DS-native arrays. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemLGun(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemLGunSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemLGunSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_LGUN_FILE_END)
    {
        gNdsItemLGunSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemLGunSetup0(stats, &state,
        base + NDS_NATIVE_ITEM_LGUN_TLUT_OFFSET,
        base + NDS_NATIVE_ITEM_LGUN_IMAGE0_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemLGun0Verts, sNdsNativeItemLGun0VertColors,
            sNdsNativeItemLGun0TriIndices,
            NDS_NATIVE_ITEM_LGUN_VERTEX0_COUNT,
            NDS_NATIVE_ITEM_LGUN_TRIANGLE0_COUNT,
            NDS_NATIVE_ITEM_LGUN_CORNER0_COUNT,
            config, stats, &state,
            &gNdsItemLGunSubmitStep, &gNdsItemLGunAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemLGunSetup1(stats,
        base + NDS_NATIVE_ITEM_LGUN_IMAGE1_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemLGun1Verts, sNdsNativeItemLGun1VertColors,
            sNdsNativeItemLGun1TriIndices,
            NDS_NATIVE_ITEM_LGUN_VERTEX1_COUNT,
            NDS_NATIVE_ITEM_LGUN_TRIANGLE1_COUNT,
            NDS_NATIVE_ITEM_LGUN_CORNER1_COUNT,
            config, stats, &state,
            &gNdsItemLGunSubmitStep, &gNdsItemLGunAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemLGunSetup2(stats,
        base + NDS_NATIVE_ITEM_LGUN_IMAGE2_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemLGun2Verts, sNdsNativeItemLGun2VertColors,
            sNdsNativeItemLGun2TriIndices,
            NDS_NATIVE_ITEM_LGUN_VERTEX2_COUNT,
            NDS_NATIVE_ITEM_LGUN_TRIANGLE2_COUNT,
            NDS_NATIVE_ITEM_LGUN_CORNER2_COUNT,
            config, stats, &state,
            &gNdsItemLGunSubmitStep, &gNdsItemLGunAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemLGunFinish(stats);
    gNdsItemLGunSubmitStep = 9u;
    return TRUE;
}

#endif
