/* Capsule executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x03E0 branches at word 10 to relocated 0x0440; the generator
 * resolves and bakes that callee. Roots 0x0540/0x05E0 are sibling DObjs. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemCapsule(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemCapsuleSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemCapsuleSubmitStep = 12u;
        return FALSE;
    }
    if ((file_bytes < NDS_NATIVE_ITEM_CAPSULE_FILE_END) ||
        ((root_offset != NDS_NATIVE_ITEM_CAPSULE_HEADER_ROOT) &&
         (root_offset != NDS_NATIVE_ITEM_CAPSULE_SECOND_ROOT) &&
         (root_offset != NDS_NATIVE_ITEM_CAPSULE_THIRD_ROOT)))
    {
        gNdsItemCapsuleSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    if (root_offset == NDS_NATIVE_ITEM_CAPSULE_HEADER_ROOT)
    {
        ndsNativeItemCapsuleHeaderSetup(stats, &state);
        ndsNativeItemCapsuleRun0Setup(stats,
            base + NDS_NATIVE_ITEM_CAPSULE_TLUT0_OFFSET,
            base + NDS_NATIVE_ITEM_CAPSULE_IMAGE0_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemCapsule0Verts, sNdsNativeItemCapsule0VertColors,
                sNdsNativeItemCapsule0TriIndices,
                NDS_NATIVE_ITEM_CAPSULE_VERTEX0_COUNT,
                NDS_NATIVE_ITEM_CAPSULE_TRIANGLE0_COUNT,
                NDS_NATIVE_ITEM_CAPSULE_CORNER0_COUNT,
                config, stats, &state,
                &gNdsItemCapsuleSubmitStep, &gNdsItemCapsuleAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemCapsuleRun1Setup(stats);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemCapsule1Verts, sNdsNativeItemCapsule1VertColors,
                sNdsNativeItemCapsule1TriIndices,
                NDS_NATIVE_ITEM_CAPSULE_VERTEX1_COUNT,
                NDS_NATIVE_ITEM_CAPSULE_TRIANGLE1_COUNT,
                NDS_NATIVE_ITEM_CAPSULE_CORNER1_COUNT,
                config, stats, &state,
                &gNdsItemCapsuleSubmitStep, &gNdsItemCapsuleAlpha) == FALSE)
        {
            return FALSE;
        }
    }
    else if (root_offset == NDS_NATIVE_ITEM_CAPSULE_SECOND_ROOT)
    {
        ndsNativeItemCapsuleSecondSetup(stats,
            base + NDS_NATIVE_ITEM_CAPSULE_TLUT2_OFFSET,
            base + NDS_NATIVE_ITEM_CAPSULE_IMAGE2_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemCapsule2Verts, sNdsNativeItemCapsule2VertColors,
                sNdsNativeItemCapsule2TriIndices,
                NDS_NATIVE_ITEM_CAPSULE_VERTEX2_COUNT,
                NDS_NATIVE_ITEM_CAPSULE_TRIANGLE2_COUNT,
                NDS_NATIVE_ITEM_CAPSULE_CORNER2_COUNT,
                config, stats, &state,
                &gNdsItemCapsuleSubmitStep, &gNdsItemCapsuleAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemCapsuleSecondFinish(stats);
    }
    else
    {
        ndsNativeItemCapsuleThirdSetup(stats,
            base + NDS_NATIVE_ITEM_CAPSULE_TLUT3_OFFSET,
            base + NDS_NATIVE_ITEM_CAPSULE_IMAGE3_OFFSET);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemCapsule3Verts, sNdsNativeItemCapsule3VertColors,
                sNdsNativeItemCapsule3TriIndices,
                NDS_NATIVE_ITEM_CAPSULE_VERTEX3_COUNT,
                NDS_NATIVE_ITEM_CAPSULE_TRIANGLE3_COUNT,
                NDS_NATIVE_ITEM_CAPSULE_CORNER3_COUNT,
                config, stats, &state,
                &gNdsItemCapsuleSubmitStep, &gNdsItemCapsuleAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemCapsuleThirdFinish(stats);
    }
    gNdsItemCapsuleSubmitStep = 9u;
    return TRUE;
}

#endif
