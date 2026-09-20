/* Ness PK Fire native weapon executor. TEXTUAL INCLUDE ONLY.
 *
 * BattleShip keeps the weapon state machine, collision and live MObjs.  This
 * replaces only NessSpecial3 root 0x0168: two source triangles with segment-E
 * material slots 0 then 1.  Both material snapshots are LIGHT1|LIGHT2 only,
 * so their animated light colours are replayed at the exact source boundary;
 * no runtime N64 display-list traversal remains. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS

static sb32 ndsNativeNessPKFireEmitGroup(
    u32 group, const NDSRendererConfig *config, NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    NDSRendererInputVertex vertices[3];
    u32 material_color;
    u32 use_material;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    u32 i;

    if ((group >= NDS_NATIVE_NESS_PKFIRE_GROUP_COUNT) ||
        (config == NULL) || (stats == NULL) || (state == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < 3u; i++)
    {
        u32 source = group * 3u + i;
        const s16 *v = &sNdsNativeNessPKFireVerts[source * 5u];
        u32 rgba = sNdsNativeNessPKFireColors[source];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    /* Source word 3 is G_TEXTURE off and neither live material can turn it
     * back on.  A textured state here means the AOT contract was violated. */
    if (ndsRendererHardwareUseTexture(stats) != FALSE)
    {
        return FALSE;
    }
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);

    if (poly_alpha != 0u)
    {
        generation = ndsRendererNextMatrixGeneration();
        ndsRendererLoadHardwareSplitMatrices(
            config->initial_projection, config->initial_modelview, generation);
        ndsRendererHardwareBeginTriangleBatch(
            stats, FALSE, 0u, ndsRendererHardwarePolyFmt(stats, poly_alpha),
            sNdsRendererHardwareMatrixMode, generation);

        /* Source G_TRI1 is (2,1,0) for both groups. */
        for (i = 0u; i < 3u; i++)
        {
            const NDSRendererInputVertex *v = &vertices[2u - i];

            glColor(ndsRendererHardwarePackedVertexColor(
                stats, v, material_color, use_material, use_vertex,
                0u, FALSE, 0u));
            glVertex3v16(
                ndsRendererHardwareVertexCoord(v->x, TRUE),
                ndsRendererHardwareVertexCoord(v->y, TRUE),
                ndsRendererHardwareVertexCoord(v->z, TRUE));
        }
        ndsRendererProfileRecordHardwareTriangle();
        stats->hardware_triangle_count++;
        stats->hardware_zbuffer_triangle_count++;
        stats->hardware_vertex_count += 3u;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    return TRUE;
}

sb32 ndsRendererSubmitNativeNessPKFire(
    const NDSRendererNativeMaterial *materials, u32 material_count,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererTraversalState state;
    u32 group;

    if ((materials == NULL) ||
        (material_count != NDS_NATIVE_NESS_PKFIRE_GROUP_COUNT) ||
        (config == NULL) || (stats == NULL) ||
        (config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        return FALSE;
    }
    for (group = 0u; group < material_count; group++)
    {
        if (materials[group].effects != NDS_NATIVE_NESS_PKFIRE_MATERIAL_EFFECTS)
        {
            return FALSE;
        }
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeNessPKFireSetup(stats);
    for (group = 0u; group < NDS_NATIVE_NESS_PKFIRE_GROUP_COUNT; group++)
    {
        /* Source DE calls slot 0 before the first triangle and slot 1 before
         * the second; applying any material outside this loop changes lighting. */
        ndsRendererNativeApplyMaterial(&materials[group], stats, &state);
        if (ndsNativeNessPKFireEmitGroup(group, config, stats, &state) == FALSE)
        {
            return FALSE;
        }
    }
    ndsNativeNessPKFireFinish(stats);
    return TRUE;
}

#endif
