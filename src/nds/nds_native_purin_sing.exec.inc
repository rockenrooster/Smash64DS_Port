/* Purin Sing: four source quads, live DObj transforms and PRIM animation. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PURIN

volatile u32 gNdsPurinSingNativeDraws[4];

sb32 ndsRendererSubmitNativePurinSing(
    u32 root_index, const NDSRendererNativeMaterial *material,
    const void *image, const NDSRendererConfig *config,
    NDSRendererStats *stats)
{
    NDSRendererTraversalState state;
    NDSRendererConfig front_config;
    NDSRendererMatrix20p12 front_view;
    s32 drawn;
    NDSRendererInputVertex vertices[NDS_NATIVE_PURIN_SING_VERTEX_COUNT];
    u32 vertex;
    u32 base;

    if ((image == NULL) || (material == NULL) || (config == NULL) || (stats == NULL) ||
        (root_index >= NDS_NATIVE_PURIN_SING_ROOT_COUNT) ||
        (config->initial_projection == NULL) ||
        (config->initial_modelview == NULL) ||
        (material->effects != NDS_NATIVE_PURIN_SING_MATERIAL_EFFECTS))
    {
        return FALSE;
    }

    base = root_index * NDS_NATIVE_PURIN_SING_VERTEX_COUNT;
    for (vertex = 0u; vertex < NDS_NATIVE_PURIN_SING_VERTEX_COUNT; vertex++)
    {
        const s16 *v = &sNdsNativePurinSingVerts[(base + vertex) * 5u];
        u32 rgba = sNdsNativePurinSingColors[base + vertex];

        vertices[vertex] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativePurinSingSetup(root_index, image, material, stats, &state);
    front_config = *config;
    front_view = *config->initial_modelview;
    ndsRendererNativeQuadGuardDepth(&front_view,
        (vertices[0].x < 0) ? -vertices[0].x : vertices[0].x);
    front_config.initial_modelview = &front_view;

    drawn = ndsRendererNativeTexturedQuad(vertices, sNdsNativePurinSingTriangles,
        &front_config, stats, &state);
    if (drawn < 0) return FALSE;
    if (drawn > 0) gNdsPurinSingNativeDraws[root_index]++;
    ndsNativePurinSingFinish(root_index, stats);
    return TRUE;
}

#endif
