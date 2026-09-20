#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PIKACHU
volatile u32 gNdsPikachuThunderNativeDraws[3];
volatile u32 gNdsPikachuThunderNativeRoleMask;

s32 ndsRendererSubmitNativePikachuThunder(u32 root, u32 role,
    const NDSRendererNativeMaterial *material, const void *palette, const void *image,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    static const s16 *const positions[] = {sThunderVerts0, sThunderVerts1, sThunderVerts2};
    static const u32 *const colors[] = {sThunderColors0, sThunderColors1, sThunderColors2};
    static const u16 *const indices[] = {sThunderIndices0, sThunderIndices1, sThunderIndices2};
    NDSRendererInputVertex vertices[4];
    NDSRendererTraversalState state;
    s32 drawn;
    u32 i;
    if (root >= 3u || config == NULL || stats == NULL ||
        config->initial_projection == NULL || config->initial_modelview == NULL) return FALSE;
    if (root == 1u)
    {
        if (palette == NULL || image == NULL || material != NULL) return FALSE;
    }
    else if (material == NULL || material->effects != ((root == 0u) ?
             NDS_NATIVE_PIKACHU_THUNDER_MATERIAL : NDS_NATIVE_PIKACHU_SHOCK_MATERIAL)) return FALSE;
    for (i = 0u; i < 4u; i++)
    {
        const s16 *v = positions[root] + i * 5u;
        u32 c = colors[root][i];
        vertices[i] = (NDSRendererInputVertex){v[0],v[1],v[2],v[3],v[4],c>>24,c>>16,c>>8,c};
    }
    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    switch (root)
    {
    case 0u: ndsThunderSetup0(stats, &state, material, palette, image); break;
    case 1u: ndsThunderSetup1(stats, &state, material, palette, image); break;
    default: ndsThunderSetup2(stats, &state, material, palette, image); break;
    }
    drawn = ndsRendererNativeTexturedQuad(vertices, indices[root], config, stats, &state);
    if (drawn < 0) return FALSE;
    if (drawn > 0)
    {
        gNdsPikachuThunderNativeDraws[root]++;
        gNdsPikachuThunderNativeRoleMask |= role;
    }
    switch (root)
    {
    case 0u: ndsThunderFinish0(stats); break;
    case 1u: ndsThunderFinish1(stats); break;
    default: ndsThunderFinish2(stats); break;
    }
    return TRUE;
}
#endif
