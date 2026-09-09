/* Mushroom Kingdom POW block GX executor. TEXTUAL INCLUDE ONLY.
 * File 155 root 0x10d0 is the ONE list nITKindPowerBlock draws.  BattleShip
 * keeps the live DObj transform and the AObj scale chains; this owner keeps
 * the immutable geometry and the list's own two-pass material.  The three
 * pointers are the source list's own SETTIMG words, already relocated by the
 * loader (TLUT into file 107 0x35f8, both CI4 images into file 155 itself),
 * so nothing is re-derived here.  There is no display-list scan or execution
 * on this path. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_INISHIE

sb32 ndsRendererSubmitNativeInishiePowblock(
    const void *tlut, const void *image_a, const void *image_b,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_INISHIE_POWBLOCK_VERTEX_COUNT];
    const NDSRendererTileState *tile;
    u32 material_color;
    u32 use_material;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    s32 texture_offset;
    u32 corner;
    u32 i;

    if ((tlut == NULL) || (image_a == NULL) || (image_b == NULL) ||
        (config == NULL) || (stats == NULL))
    {
        gNdsInishiePowblockSubmitStep = 11u;
        return FALSE;
    }
    gNdsInishiePowblockTlut = (u32)(uintptr_t)tlut;
    gNdsInishiePowblockImageA = (u32)(uintptr_t)image_a;
    gNdsInishiePowblockImageB = (u32)(uintptr_t)image_b;
    gNdsInishiePowblockProjection = (u32)(uintptr_t)config->initial_projection;
    gNdsInishiePowblockModelview = (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsInishiePowblockSubmitStep = 12u;
        return FALSE;
    }

    for (i = 0u; i < NDS_NATIVE_INISHIE_POWBLOCK_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeInishiePowblockVerts[i * 5u];
        u32 rgba = sNdsNativeInishiePowblockVertColors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    /* The source list emits no prim/env colour and no SETOTHERMODE_L: the
     * blender state belongs to the item pass that reached us.  Inheriting it
     * IS source behaviour. */
    ndsNativeInishiePowblockSetupA(stats, tlut, image_a);
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, config, &state) == FALSE)
    {
        gNdsInishiePowblockSubmitStep = 3u;
        return FALSE;
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    gNdsInishiePowblockAlpha = poly_alpha;
    if (poly_alpha != 0u)
    {
        generation = ndsRendererNextMatrixGeneration();
        ndsRendererLoadHardwareSplitMatrices(
            config->initial_projection, config->initial_modelview, generation);
        ndsRendererHardwareBeginTriangleBatch(
            stats, TRUE, sNdsRendererHardwareBoundTextureName,
            ndsRendererHardwarePolyFmt(stats, poly_alpha),
            sNdsRendererHardwareMatrixMode, generation);
        for (corner = 0u;
             corner < NDS_NATIVE_INISHIE_POWBLOCK_PASS_A_TRIANGLE_COUNT * 3u;
             corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeInishiePowblockTriA[corner]];

            glColor(ndsRendererHardwarePackedVertexColor(
                stats, v, material_color, use_material, use_vertex,
                0u, FALSE, 0u));
            glTexCoord2t16(
                ndsRendererHardwareTexCoord(v->s, stats->texture_scale_s,
                    tile->uls, texture_offset),
                ndsRendererHardwareTexCoord(v->t, stats->texture_scale_t,
                    tile->ult, texture_offset));
            glVertex3v16(
                ndsRendererHardwareVertexCoord(v->x, TRUE),
                ndsRendererHardwareVertexCoord(v->y, TRUE),
                ndsRendererHardwareVertexCoord(v->z, TRUE));
        }
        /* Second textured pass: re-tile, re-bind, second batch with the
         * same matrices -- the source draws both passes under one link. */
        ndsNativeInishiePowblockSetupB(stats, image_b);
        NDS_FIGHTER_PACKET_DMA_WAIT();
        ndsRendererHardwareEndBatch();
        if (ndsRendererHardwareBindTexture(stats, config, &state) == FALSE)
        {
            gNdsInishiePowblockSubmitStep = 4u;
            return FALSE;
        }
        tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
        ndsRendererHardwareBeginTriangleBatch(
            stats, TRUE, sNdsRendererHardwareBoundTextureName,
            ndsRendererHardwarePolyFmt(stats, poly_alpha),
            sNdsRendererHardwareMatrixMode, generation);
        for (corner = 0u;
             corner < NDS_NATIVE_INISHIE_POWBLOCK_PASS_B_TRIANGLE_COUNT * 3u;
             corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeInishiePowblockTriB[corner]];

            glColor(ndsRendererHardwarePackedVertexColor(
                stats, v, material_color, use_material, use_vertex,
                0u, FALSE, 0u));
            glTexCoord2t16(
                ndsRendererHardwareTexCoord(v->s, stats->texture_scale_s,
                    tile->uls, texture_offset),
                ndsRendererHardwareTexCoord(v->t, stats->texture_scale_t,
                    tile->ult, texture_offset));
            glVertex3v16(
                ndsRendererHardwareVertexCoord(v->x, TRUE),
                ndsRendererHardwareVertexCoord(v->y, TRUE),
                ndsRendererHardwareVertexCoord(v->z, TRUE));
        }
        for (i = 0u; i < NDS_NATIVE_INISHIE_POWBLOCK_TRIANGLE_COUNT; i++)
        {
            ndsRendererProfileRecordHardwareTriangle();
        }
        stats->hardware_triangle_count +=
            NDS_NATIVE_INISHIE_POWBLOCK_TRIANGLE_COUNT;
        stats->hardware_zbuffer_triangle_count +=
            NDS_NATIVE_INISHIE_POWBLOCK_TRIANGLE_COUNT;
        stats->hardware_vertex_count +=
            NDS_NATIVE_INISHIE_POWBLOCK_CORNER_COUNT;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    ndsNativeInishiePowblockFinish(stats);
    gNdsInishiePowblockSubmitStep = 9u;
    return TRUE;
}

#endif
