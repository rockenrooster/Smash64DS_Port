/* Shared fixed-geometry submit primitive for the five common-item owners in
 * this batch. TEXTUAL INCLUDE ONLY. State/material ownership remains in the
 * per-item executors; this helper only converts an already-baked vertex/index
 * packet into DS GX triangles under the supplied traversal state. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

static sb32 ndsNativeItemWave1Emit(
    const s16 *verts, const u32 *colors, const u16 *indices,
    u32 vertex_count, u32 triangle_count, u32 corner_count,
    const NDSRendererConfig *config, NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    volatile u32 *submit_step, volatile u32 *alpha_witness)
{
    NDSRendererInputVertex vertices[24];
    const NDSRendererTileState *tile;
    u32 material_color;
    u32 use_material;
    u32 use_texture;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    s32 texture_offset;
    u32 corner;
    u32 i;

    if ((verts == NULL) || (colors == NULL) || (indices == NULL) ||
        (config == NULL) || (stats == NULL) || (state == NULL) ||
        (submit_step == NULL) || (alpha_witness == NULL) ||
        (vertex_count == 0u) || (vertex_count > 24u))
    {
        if (submit_step != NULL)
        {
            *submit_step = 4u;
        }
        return FALSE;
    }

    for (i = 0u; i < vertex_count; i++)
    {
        const s16 *v = &verts[i * 5u];
        u32 rgba = colors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    /* Some baked item runs are source-untextured.  Beam Sword joint 1 is the
     * first natural witness: its source DL explicitly issues G_TEXTURE off and
     * combines SHADE only.  Requiring a texture bind here rejects that valid
     * run even though there is deliberately no image to bind.  Match the
     * generic native prepare path: bind only when the decoded source state
     * actually uses a texture, and submit an untextured DS batch otherwise. */
    use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ? TRUE : FALSE;
    if ((use_texture != FALSE) &&
        (ndsRendererHardwareBindTexture(stats, config, state) == FALSE))
    {
        *submit_step = 3u;
        return FALSE;
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    *alpha_witness = poly_alpha;

    if (poly_alpha != 0u)
    {
        generation = ndsRendererNextMatrixGeneration();
        ndsRendererLoadHardwareSplitMatrices(
            config->initial_projection, config->initial_modelview, generation);
        ndsRendererHardwareBeginTriangleBatch(
            stats, use_texture,
            (use_texture != FALSE) ? sNdsRendererHardwareBoundTextureName : 0u,
            ndsRendererHardwarePolyFmt(stats, poly_alpha),
            sNdsRendererHardwareMatrixMode, generation);
        for (corner = 0u; corner < corner_count; corner++)
        {
            const NDSRendererInputVertex *v = &vertices[indices[corner]];

            glColor(ndsRendererHardwarePackedVertexColor(
                stats, v, material_color, use_material, use_vertex,
                0u, FALSE, 0u));
            if (use_texture != FALSE)
            {
                glTexCoord2t16(
                    ndsRendererHardwareTexCoord(v->s, stats->texture_scale_s,
                        tile->uls, texture_offset),
                    ndsRendererHardwareTexCoord(v->t, stats->texture_scale_t,
                        tile->ult, texture_offset));
            }
            glVertex3v16(
                ndsRendererHardwareVertexCoord(v->x, TRUE),
                ndsRendererHardwareVertexCoord(v->y, TRUE),
                ndsRendererHardwareVertexCoord(v->z, TRUE));
        }
        for (i = 0u; i < triangle_count; i++)
        {
            ndsRendererProfileRecordHardwareTriangle();
        }
        stats->hardware_triangle_count += triangle_count;
        stats->hardware_zbuffer_triangle_count += triangle_count;
        stats->hardware_vertex_count += corner_count;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    return TRUE;
}

#endif
