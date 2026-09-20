#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_SAMUS
volatile u32 gNdsSamusBombNativeDraws;

s32 ndsRendererSubmitNativeSamusBomb(const NDSRendererNativeMaterial *material,
    const void *image, const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererInputVertex vertices[4];
    NDSRendererTraversalState state;
    s32 drawn;
    u32 i;
    if (material == NULL || image == NULL || config == NULL || stats == NULL ||
        config->initial_projection == NULL || config->initial_modelview == NULL ||
        material->effects != NDS_NATIVE_SAMUS_BOMB_MATERIAL) return FALSE;
    for (i = 0u; i < 4u; i++)
    {
        const s16 *v = sSamusBombVerts + i * 5u;
        u32 c = sSamusBombColors[i];
        vertices[i] = (NDSRendererInputVertex){v[0],v[1],v[2],v[3],v[4],c>>24,c>>16,c>>8,c};
    }
    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsSamusBombSetup(stats, &state, material, image);
    drawn = ndsRendererNativeTexturedQuad(vertices, sSamusBombIndices, config, stats, &state);
    if (drawn < 0) return FALSE;
    if (drawn > 0) gNdsSamusBombNativeDraws++;
    ndsSamusBombFinish(stats);
    return TRUE;
}
#endif
