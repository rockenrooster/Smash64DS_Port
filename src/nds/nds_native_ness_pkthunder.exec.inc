/* Ness PK Thunder native weapon executor. TEXTUAL INCLUDE ONLY.
 *
 * BattleShip keeps the weapon state machine, steering, collision, DObj
 * transforms, material animation and source trail_id colour selection. This
 * replaces only NessModel roots 0x7BD0 (head) and 0x8A98 (trail): two source
 * triangles each, with the live CURRENT_IMAGE MObj preserved. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS

#if NDS_P2_NESS_VFX_PROOF
volatile u32 gNdsNessPKThunderNativeProofDraw[3];
#endif

sb32 ndsRendererSubmitNativeNessPKThunder(
    u32 root_index, const NDSRendererNativeMaterial *material,
    u32 trail_color_index, const NDSRendererConfig *config,
    NDSRendererStats *stats)
{
    NDSRendererTraversalState state;
    s32 drawn;
    NDSRendererInputVertex vertices[NDS_NATIVE_NESS_PKTHUNDER_VERTEX_COUNT];
    u32 vertex;
    u32 base;

    if ((material == NULL) || (config == NULL) || (stats == NULL) ||
        (root_index >= NDS_NATIVE_NESS_PKTHUNDER_ROOT_COUNT) ||
        (config->initial_projection == NULL) ||
        (config->initial_modelview == NULL) ||
        (material->effects != NDS_NATIVE_NESS_PKTHUNDER_MATERIAL_EFFECTS))
    {
        return FALSE;
    }
    if ((root_index == NDS_NATIVE_NESS_PKTHUNDER_TRAIL_INDEX) &&
        (trail_color_index >= NDS_NATIVE_NESS_PKTHUNDER_TRAIL_COLOR_COUNT))
    {
        return FALSE;
    }

    base = root_index * NDS_NATIVE_NESS_PKTHUNDER_VERTEX_COUNT;
    for (vertex = 0u; vertex < NDS_NATIVE_NESS_PKTHUNDER_VERTEX_COUNT; vertex++)
    {
        const s16 *v = &sNdsNativeNessPKThunderVerts[(base + vertex) * 5u];
        u32 rgba = sNdsNativeNessPKThunderColors[base + vertex];

        vertices[vertex] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeNessPKThunderSetup(root_index, stats);
    ndsRendererNativeApplyMaterial(material, stats, &state);
    if (root_index == NDS_NATIVE_NESS_PKTHUNDER_TRAIL_INDEX)
    {
        stats->prim_color = sNdsNativeNessPKThunderTrailPrim[trail_color_index];
        stats->env_color = sNdsNativeNessPKThunderTrailEnv[trail_color_index];
    }

    drawn = ndsRendererNativeTexturedQuad(vertices, sNdsNativeNessPKThunderTriangles,
        config, stats, &state);
    if (drawn < 0) return FALSE;
#if NDS_P2_NESS_VFX_PROOF
    if (drawn > 0) gNdsNessPKThunderNativeProofDraw[root_index]++;
#endif
    ndsNativeNessPKThunderFinish(stats);
    return TRUE;
}

#endif
