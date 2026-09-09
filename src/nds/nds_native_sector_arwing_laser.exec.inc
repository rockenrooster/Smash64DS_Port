/* Sector Z Arwing laser GX executor. TEXTUAL INCLUDE ONLY.
 * File 153 root 0x1c50 is the ONE list nWPKindArwingLaser2D and
 * nWPKindArwingLaser3D both draw (262_GRSectorMap.c:95,125).  BattleShip keeps
 * the live DObj transform; this owner keeps the immutable geometry and the
 * list's own texture binding.  The two pointers are the source list's own
 * SETTIMG words, already relocated into file 161 by the loader's external
 * fixup pass (reloc_backend_assets.c:8420), so nothing is re-derived here.
 * There is no display-list scan or execution on this path. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_SECTOR

sb32 ndsRendererSubmitNativeSectorArwingLaser(
    const void *tlut, const void *image,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_SECTOR_LASER_VERTEX_COUNT];
    const NDSRendererTileState *tile;
    u32 material_color;
    u32 use_material;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    s32 texture_offset;
    u32 corner;
    u32 i;

    /* Split so the witness names WHICH argument was wrong; the adapter has
     * already proved both texture pointers land in file 161 by the time it
     * calls here, so a step-11 decline was never going to be one of those. */
    if ((tlut == NULL) || (image == NULL) || (config == NULL) ||
        (stats == NULL))
    {
        gNdsSectorLaserSubmitStep = 11u;
        return FALSE;
    }
    gNdsSectorLaserTlut = (u32)(uintptr_t)tlut;
    gNdsSectorLaserImage = (u32)(uintptr_t)image;
    gNdsSectorLaserProjection = (u32)(uintptr_t)config->initial_projection;
    gNdsSectorLaserModelview = (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        /* Both NULL is a different failure from the split-camera case the
         * caller repairs on its config copy; keep it loud rather than
         * inventing a second identity here. */
        gNdsSectorLaserSubmitStep = 12u;
        return FALSE;
    }

    for (i = 0u; i < NDS_NATIVE_SECTOR_LASER_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeSectorLaserVerts[i * 5u];
        u32 rgba = sNdsNativeSectorLaserVertColors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    /* The source list emits no G_SETOTHERMODE_L, no prim and no env colour:
     * the blender, colours and cull state belong to the link-14 weapon pass
     * that reached us.  Do not seed them here -- inheriting them IS source
     * behaviour (wpDisplayDLHead1, wpmanager.c:270). */
    ndsNativeSectorLaserSetup(stats, tlut, image);
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, config, &state) == FALSE)
    {
        gNdsSectorLaserSubmitStep = 3u;
        return FALSE;
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    /* All sixteen TLUT entries are opaque and the combiner takes alpha from
     * TEXEL0 only, so a zero here is the inherited blender's doing, not the
     * asset's.  Witness it rather than let an empty draw read as success. */
    gNdsSectorLaserAlpha = poly_alpha;
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
             corner < NDS_NATIVE_SECTOR_LASER_CORNER_COUNT; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeSectorLaserTriIndices[corner]];

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
        for (i = 0u; i < NDS_NATIVE_SECTOR_LASER_TRIANGLE_COUNT; i++)
        {
            ndsRendererProfileRecordHardwareTriangle();
        }
        stats->hardware_triangle_count +=
            NDS_NATIVE_SECTOR_LASER_TRIANGLE_COUNT;
        stats->hardware_zbuffer_triangle_count +=
            NDS_NATIVE_SECTOR_LASER_TRIANGLE_COUNT;
        stats->hardware_vertex_count +=
            NDS_NATIVE_SECTOR_LASER_CORNER_COUNT;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    ndsNativeSectorLaserFinish(stats);
    gNdsSectorLaserSubmitStep = 9u;
    return TRUE;
}

#endif
