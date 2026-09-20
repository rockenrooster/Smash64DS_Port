/* Saffron City Fushigibana GX executor. TEXTUAL INCLUDE ONLY.
 * File 159 root 0x2250 is a thirty-word quad with one live-material word:
 * word 17 is exactly 0xDE000000 0x0E000000.  The palette, render state and
 * geometry are immutable and generated; CURRENT_IMAGE is the only live MObj
 * effect and is applied between the source's tile-size and load-block words.
 * The exact material contract is checked here as well as by the adapter. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

sb32 ndsRendererSubmitNativeItemFushigibana(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *actor_base = (const u8 *)actor_base_ptr;
    const void *tlut;
    NDSRendererTraversalState state;
    s32 drawn;
    NDSRendererInputVertex vertices[NDS_NATIVE_ITEM_FUSHIGIBANA_VERTEX_COUNT];
    u32 i;

    if ((actor_base == NULL) || (material == NULL) || (config == NULL) ||
        (stats == NULL))
    {
        gNdsYamabukiFushigibanaSubmitStep = 11u;
        return FALSE;
    }
    gNdsYamabukiFushigibanaProjection =
        (u32)(uintptr_t)config->initial_projection;
    gNdsYamabukiFushigibanaModelview =
        (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsYamabukiFushigibanaSubmitStep = 12u;
        return FALSE;
    }
    if ((actor_bytes < NDS_NATIVE_ITEM_FUSHIGIBANA_TLUT_END) ||
        (actor_bytes < (NDS_NATIVE_ITEM_FUSHIGIBANA_ROOT +
                        NDS_NATIVE_ITEM_FUSHIGIBANA_DL_BYTES)))
    {
        gNdsYamabukiFushigibanaSubmitStep = 2u;
        return FALSE;
    }
    if (material->effects != NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
    {
        gNdsYamabukiFushigibanaEffectsRejected = material->effects;
        gNdsYamabukiFushigibanaSubmitStep = 4u;
        return FALSE;
    }
    tlut = actor_base + NDS_NATIVE_ITEM_FUSHIGIBANA_TLUT_OFFSET;
    gNdsYamabukiFushigibanaTlut = (u32)(uintptr_t)tlut;
    gNdsYamabukiFushigibanaImage = material->current_image;

    for (i = 0u; i < NDS_NATIVE_ITEM_FUSHIGIBANA_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeItemFushigibanaVerts[i * 5u];
        u32 rgba = sNdsNativeItemFushigibanaVertColors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemFushigibanaSetup(stats, tlut);
    ndsRendererNativeApplyMaterial(material, stats, &state);
    ndsNativeItemFushigibanaLoadBlock(stats);
    drawn = ndsRendererNativeTexturedQuad(vertices, sNdsNativeItemFushigibanaTriIndices,
        config, stats, &state);
    if (drawn < 0)
    {
        gNdsYamabukiFushigibanaSubmitStep = 3u;
        return FALSE;
    }
    gNdsYamabukiFushigibanaAlpha = (u32)drawn;
    ndsNativeItemFushigibanaFinish(stats);
    gNdsYamabukiFushigibanaSubmitStep = 9u;
    return TRUE;
}

#endif
