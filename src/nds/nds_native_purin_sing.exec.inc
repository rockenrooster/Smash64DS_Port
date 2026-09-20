/* Purin Sing: four source quads, live DObj transforms and PRIM animation. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PURIN

volatile u32 gNdsPurinSingNativeDraws[4];

/* Match the live guard renderer's max(150, half live radius) camera bias.
 * View-space eye is the origin; only translation changes, never source pose,
 * material animation or lifetime. Use the existing DS integer math units. */
static void ndsNativePurinSingGuardDepth(NDSRendererMatrix20p12 *view, s32 radius)
{
    s32 position[3];
    u64 distance_squared = 0u;
    u32 scale = 0u, distance, bias, axis, i;
    for (i = 0u; i < 3u; i++)
    {
        position[i] = view->m[3][i] >> 12;
        distance_squared += (u64)((s64)position[i] * position[i]);
    }
    distance = ndsR2HwMathSqrt64(distance_squared);
    if (distance <= 1u) return;
    for (axis = 0u; axis < 2u; axis++)
    {
        u64 length_squared = 0u;
        u32 length;
        for (i = 0u; i < 3u; i++)
            length_squared += (u64)((s64)view->m[axis][i] * view->m[axis][i]);
        length = ndsR2HwMathSqrt64(length_squared);
        if (length > scale) scale = length;
    }
    bias = (u32)(((u64)scale * (u32)radius) >> 13);
    if (bias < 150u) bias = 150u;
    for (i = 0u; i < 3u; i++)
        view->m[3][i] -= (s32)ndsR2HwMathDiv64(
            (s64)position[i] * bias * 4096, (s32)distance);
}

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
    ndsNativePurinSingGuardDepth(&front_view,
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
