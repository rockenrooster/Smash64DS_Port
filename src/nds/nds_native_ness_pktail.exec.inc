#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS
volatile u32 gNdsNessPKTailNativeDraws;

s32 ndsRendererSubmitNativeNessPKTail(const void *image,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererInputVertex vertices[4];
    NDSRendererTraversalState state;
    s32 drawn;
    u32 i;
    if (image == NULL || config == NULL || stats == NULL ||
        config->initial_projection == NULL || config->initial_modelview == NULL) return FALSE;
    for (i = 0u; i < 4u; i++)
    {
        const s16 *v = sNessPKTailVerts + i * 5u;
        u32 c = sNessPKTailColors[i];
        vertices[i] = (NDSRendererInputVertex){v[0],v[1],v[2],v[3],v[4],c>>24,c>>16,c>>8,c};
    }
    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNessPKTailSetup(stats, image);
    drawn = ndsRendererNativeTexturedQuadIa8(vertices, sNessPKTailIndices, (const u8 *)image,
        config, stats, &state);
    if (drawn < 0) return FALSE;
    if (drawn > 0) gNdsNessPKTailNativeDraws++;
    ndsNessPKTailFinish(stats);
    return TRUE;
}
#endif
