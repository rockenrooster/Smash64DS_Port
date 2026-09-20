/* Yoshi Egg Lay victim-egg native executor. TEXTUAL INCLUDE ONLY. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_YOSHI
#if NDS_P2_YOSHI_BUG_PROOF
volatile u32 gNdsYoshiBugProofEggLayDrawCount;
#endif

sb32 ndsRendererSubmitNativeYoshiEggLay(
    const void *palette, const void *image,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererTraversalState state;
    s32 drawn;
    NDSRendererInputVertex vertices[NDS_NATIVE_YOSHI_EGGLAY_VERTEX_COUNT];
    u32 i;

    if ((palette == NULL) || (image == NULL) || (config == NULL) ||
        (stats == NULL) || (config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_NATIVE_YOSHI_EGGLAY_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeYoshiEggLayVerts[i * 5u];
        u32 rgba = sNdsNativeYoshiEggLayVertColors[i];
        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }
    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeYoshiEggLaySetup(stats, palette, image);
    drawn = ndsRendererNativeTexturedQuad(vertices, sNdsNativeYoshiEggLayTriIndices,
        config, stats, &state);
    if (drawn < 0) return FALSE;
#if NDS_P2_YOSHI_BUG_PROOF
    if (drawn > 0) gNdsYoshiBugProofEggLayDrawCount++;
#endif
    ndsNativeYoshiEggLayFinish(stats);
    return TRUE;
}

#endif
