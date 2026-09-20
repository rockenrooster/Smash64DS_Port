/* Saffron City Hitokage GX executor. TEXTUAL INCLUDE ONLY.
 * File 159 root 0x18a0 is a thirty-word quad with one live-material word:
 * word 17 is exactly 0xDE000000 0x0E000000.  The palette, render state and
 * geometry are immutable and generated; CURRENT_IMAGE is the only live MObj
 * effect and is applied between the source's tile-size and load-block words.
 * The adapter proves the same contract before calling this executor, and the
 * executor repeats it so a broader material can never silently lose source
 * presentation. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemHitokage(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *actor_base = (const u8 *)actor_base_ptr;
    const void *tlut;
    NDSRendererTraversalState state;
    s32 drawn;
    NDSRendererInputVertex vertices[NDS_NATIVE_ITEM_HITOKAGE_VERTEX_COUNT];
    u32 i;

    if ((actor_base == NULL) || (material == NULL) || (config == NULL) ||
        (stats == NULL))
    {
        gNdsYamabukiHitokageSubmitStep = 11u;
        return FALSE;
    }
    gNdsYamabukiHitokageProjection =
        (u32)(uintptr_t)config->initial_projection;
    gNdsYamabukiHitokageModelview =
        (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsYamabukiHitokageSubmitStep = 12u;
        return FALSE;
    }
    if ((actor_bytes < NDS_NATIVE_ITEM_HITOKAGE_TLUT_END) ||
        (actor_bytes < (NDS_NATIVE_ITEM_HITOKAGE_ROOT +
                        NDS_NATIVE_ITEM_HITOKAGE_DL_BYTES)))
    {
        gNdsYamabukiHitokageSubmitStep = 2u;
        return FALSE;
    }
    if (material->effects != NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
    {
        gNdsYamabukiHitokageEffectsRejected = material->effects;
        gNdsYamabukiHitokageSubmitStep = 4u;
        return FALSE;
    }
    tlut = actor_base + NDS_NATIVE_ITEM_HITOKAGE_TLUT_OFFSET;
    gNdsYamabukiHitokageTlut = (u32)(uintptr_t)tlut;
    gNdsYamabukiHitokageImage = material->current_image;

    for (i = 0u; i < NDS_NATIVE_ITEM_HITOKAGE_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeItemHitokageVerts[i * 5u];
        u32 rgba = sNdsNativeItemHitokageVertColors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemHitokageSetup(stats, tlut);
    ndsRendererNativeApplyMaterial(material, stats, &state);
    ndsNativeItemHitokageLoadBlock(stats);
    drawn = ndsRendererNativeTexturedQuad(vertices, sNdsNativeItemHitokageTriIndices,
        config, stats, &state);
    if (drawn < 0)
    {
        gNdsYamabukiHitokageSubmitStep = 3u;
        return FALSE;
    }
    gNdsYamabukiHitokageAlpha = (u32)drawn;
    ndsNativeItemHitokageFinish(stats);
    gNdsYamabukiHitokageSubmitStep = 9u;
    return TRUE;
}

#endif
