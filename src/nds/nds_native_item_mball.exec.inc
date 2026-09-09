/* Poke Ball / MBall executor. TEXTUAL INCLUDE ONLY.
 * Root 0x9250 contains no 0xDE and bakes completely. Root 0x9340 calls
 * 0xDE000000 0x0E000000 at word 16 and accepts exactly CURRENT_IMAGE from
 * its source MObj; the executor refuses any wider material effect word. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemMBall(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemMBallSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemMBallSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_MBALL_FILE_END)
    {
        gNdsItemMBallSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    if (root_offset == NDS_NATIVE_ITEM_MBALL_BAKED_ROOT)
    {
        const void *tlut;
        const void *image;

        if (material != NULL)
        {
            gNdsItemMBallSubmitStep = 13u;
            return FALSE;
        }
        tlut = base + NDS_NATIVE_ITEM_MBALL_BAKED_TLUT_OFFSET;
        image = base + NDS_NATIVE_ITEM_MBALL_BAKED_IMAGE_OFFSET;
        ndsNativeItemMBallBakedSetup(stats, tlut, image);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemMBallBakedVerts,
                sNdsNativeItemMBallBakedVertColors,
                sNdsNativeItemMBallBakedTriIndices,
                NDS_NATIVE_ITEM_MBALL_BAKED_VERTEX_COUNT,
                NDS_NATIVE_ITEM_MBALL_BAKED_TRIANGLE_COUNT,
                NDS_NATIVE_ITEM_MBALL_BAKED_CORNER_COUNT,
                config, stats, &state,
                &gNdsItemMBallSubmitStep, &gNdsItemMBallAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemMBallBakedFinish(stats);
    }
    else if (root_offset == NDS_NATIVE_ITEM_MBALL_LIVE_ROOT)
    {
        const void *tlut;
        u32 image_ok = FALSE;
        u32 i;

        if (material == NULL)
        {
            gNdsItemMBallSubmitStep = 14u;
            return FALSE;
        }
        if (material->effects != NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
        {
            gNdsItemMBallEffectsRejected = material->effects;
            gNdsItemMBallSubmitStep = 15u;
            return FALSE;
        }
        for (i = 0u; i < 8u; i++)
        {
            if (material->current_image ==
                    (u32)(uintptr_t)(base + sNdsNativeItemMBallLiveImageOffsets[i]))
            {
                image_ok = TRUE;
                break;
            }
        }
        if (image_ok == FALSE)
        {
            gNdsItemMBallSubmitStep = 16u;
            return FALSE;
        }
        tlut = base + NDS_NATIVE_ITEM_MBALL_LIVE_TLUT_OFFSET;
        ndsNativeItemMBallLiveSetup(stats, tlut);
        ndsRendererNativeApplyMaterial(material, stats, &state);
        ndsNativeItemMBallLiveAfterMaterial(stats);
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemMBallLiveVerts,
                sNdsNativeItemMBallLiveVertColors,
                sNdsNativeItemMBallLiveTriIndices,
                NDS_NATIVE_ITEM_MBALL_LIVE_VERTEX_COUNT,
                NDS_NATIVE_ITEM_MBALL_LIVE_TRIANGLE_COUNT,
                NDS_NATIVE_ITEM_MBALL_LIVE_CORNER_COUNT,
                config, stats, &state,
                &gNdsItemMBallSubmitStep, &gNdsItemMBallAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemMBallLiveFinish(stats);
    }
    else
    {
        gNdsItemMBallSubmitStep = 17u;
        return FALSE;
    }
    gNdsItemMBallSubmitStep = 9u;
    return TRUE;
}

#endif
