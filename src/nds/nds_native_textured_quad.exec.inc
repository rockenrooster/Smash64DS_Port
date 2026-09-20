/* Fixed four-vertex/six-index GX kernel shared by the baked effect owners.
 * Keep one out-of-line copy: duplicated texture/color submission exhausted
 * the all-content arena when Sing was added. No display-list execution.
 * Return polygon alpha (0 means invisible), or -1 for a failed texture bind. */
#if NDS_RENDERER_HW_TRIANGLES
static s32 __attribute__((noinline, noclone)) ndsRendererNativeTexturedQuad(
    const NDSRendererInputVertex vertices[4], const u16 indices[6],
    const NDSRendererConfig *config, NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const NDSRendererTileState *tile;
    u32 material_color, use_material, use_vertex, alpha, tri, poly_fmt;
    u32 generation;
    s32 texture_offset;

    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, config, state) == FALSE) return -1;
    alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    if (alpha == 0u) return 0;
    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    generation = ndsRendererNextMatrixGeneration();
    ndsRendererLoadHardwareSplitMatrices(config->initial_projection,
        config->initial_modelview, generation);
    poly_fmt = ndsRendererHardwarePolyFmt(stats, alpha);
    ndsRendererHardwareBeginTriangleBatch(stats, TRUE,
        sNdsRendererHardwareBoundTextureName,
        poly_fmt,
        sNdsRendererHardwareMatrixMode, sNdsRendererHardwareMatrixGeneration);
    for (tri = 0u; tri < 2u; tri++)
    {
        u32 corner;
        for (corner = 0u; corner < 3u; corner++)
        {
            const NDSRendererInputVertex *v = &vertices[indices[tri * 3u + corner]];
            glColor(ndsRendererHardwarePackedVertexColor(stats, v, material_color,
                use_material, use_vertex, 0u, FALSE, 0u));
            glTexCoord2t16(
                ndsRendererHardwareTexCoord(v->s, stats->texture_scale_s, tile->uls, texture_offset),
                ndsRendererHardwareTexCoord(v->t, stats->texture_scale_t, tile->ult, texture_offset));
            glVertex3v16(ndsRendererHardwareVertexCoord(v->x, TRUE),
                ndsRendererHardwareVertexCoord(v->y, TRUE),
                ndsRendererHardwareVertexCoord(v->z, TRUE));
        }
        ndsRendererProfileRecordHardwareTriangle();
    }
    stats->hardware_triangle_count += 2u;
    if ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) != 0u)
        stats->hardware_zbuffer_triangle_count += 2u;
    stats->hardware_vertex_count += 6u;
    sNdsRendererHardwareSubmitted = TRUE;
    ndsRendererHardwareEndBatch();
    return (s32)alpha;
}
#endif
