/* Mushroom Kingdom Pakkun GX executor. TEXTUAL INCLUDE ONLY.
 * Immutable root/state/quad data is generated from file 155 root 0x0B40.
 * The source item keeps its DObj transform, MatAnim and MObj alive; only the
 * MOBJ_FLAG_ALPHA current-image hook is translated to typed native state. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_INISHIE

sb32 ndsRendererSubmitNativeInishiePakkun(
    const void *actor_base_ptr, u32 actor_bytes, const void *palette,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *actor_base = (const u8 *)actor_base_ptr;
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_INISHIE_PAKKUN_VERTEX_COUNT];
    const NDSRendererTileState *tile;
    u32 material_color;
    u32 use_material;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    s32 texture_offset;
    u32 frame;
    u32 corner;
    u32 i;

    /* Split so the witness names WHICH argument was wrong. The adapter has
     * already proved effects == CURRENT_IMAGE by the time it calls here, so
     * a step-1 decline was never going to be that one. */
    if ((actor_base == NULL) || (palette == NULL) || (material == NULL) ||
        (config == NULL) || (stats == NULL))
    {
        gNdsInishiePakkunSubmitStep = 11u;
        return FALSE;
    }
    gNdsInishiePakkunImageW0 = material->current_image_w0;
    gNdsInishiePakkunImage = material->current_image;
    gNdsInishiePakkunProjection = (u32)(uintptr_t)config->initial_projection;
    gNdsInishiePakkunModelview = (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsInishiePakkunSubmitStep = 12u;
        return FALSE;
    }
    if (material->effects != NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
    {
        gNdsInishiePakkunSubmitStep = 13u;
        return FALSE;
    }
    if (material->current_image_w0 != 0xfd500000u)
    {
        gNdsInishiePakkunSubmitStep = 14u;
        return FALSE;
    }
    if (material->current_image == 0u)
    {
        gNdsInishiePakkunSubmitStep = 15u;
        return FALSE;
    }

    frame = 3u;
    for (i = 0u; i < 3u; i++)
    {
        if ((const u8 *)(uintptr_t)material->current_image ==
            actor_base + sNdsNativeInishiePakkunFrameOffsets[i])
        {
            frame = i;
            break;
        }
    }
    if ((frame >= 3u) ||
        (actor_bytes < (u32)sNdsNativeInishiePakkunFrameOffsets[frame] +
                       (u32)sNdsNativeInishiePakkunFrameBytes[frame]))
    {
        gNdsInishiePakkunSubmitStep = 2u;
        return FALSE;
    }

    for (i = 0u; i < NDS_NATIVE_INISHIE_PAKKUN_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeInishiePakkunVerts[i * 5u];
        u32 rgba = sNdsNativeInishiePakkunVertColors[i];
        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeInishiePakkunSetup(stats, palette);
    /* Source word 17 is gSPDisplayList(0x0E000000).  The ALPHA-only MObj
     * emits one current-image command and EndDL; translate it through the
     * shared typed material path, never through a segment-E Gfx stream. */
    ndsRendererNativeApplyMaterial(material, stats, &state);
    ndsNativeInishiePakkunSetupTail(stats);
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, config, &state) == FALSE)
    {
        gNdsInishiePakkunSubmitStep = 3u;
        return FALSE;
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    if (poly_alpha != 0u)
    {
        generation = ndsRendererNextMatrixGeneration();
        ndsRendererLoadHardwareSplitMatrices(
            config->initial_projection, config->initial_modelview, generation);
        ndsRendererHardwareBeginTriangleBatch(
            stats, TRUE, sNdsRendererHardwareBoundTextureName,
            ndsRendererHardwarePolyFmt(stats, poly_alpha),
            sNdsRendererHardwareMatrixMode, generation);
        for (corner = 0u; corner < NDS_NATIVE_INISHIE_PAKKUN_CORNER_COUNT; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeInishiePakkunTriIndices[corner]];
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
        ndsRendererProfileRecordHardwareTriangle();
        ndsRendererProfileRecordHardwareTriangle();
        stats->hardware_triangle_count += NDS_NATIVE_INISHIE_PAKKUN_TRIANGLE_COUNT;
        stats->hardware_zbuffer_triangle_count += NDS_NATIVE_INISHIE_PAKKUN_TRIANGLE_COUNT;
        stats->hardware_vertex_count += NDS_NATIVE_INISHIE_PAKKUN_CORNER_COUNT;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    ndsNativeInishiePakkunFinish(stats);
    return TRUE;
}

#endif
