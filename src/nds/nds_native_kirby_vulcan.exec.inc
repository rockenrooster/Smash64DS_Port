/* Two immutable Vulcan meshes; source effect motion/lifetime remain live. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_KIRBY
static u32 sVulcanTextureNames[5];
static u32 sVulcanTextureGeneration;
volatile u32 gNdsKirbyVulcanNativeDraws[2];

static s32 ndsVulcanFill(u8 *pixels, u32 bytes, void *data)
{
    u32 bank = (u32)(uintptr_t)data;
    u32 wanted = (bank == 4u) ? 256u : 512u;
    if (bank >= 5u || bytes < wanted) return FALSE;
    memcpy(pixels, sVulcanPixels[bank], wanted);
    return TRUE;
}

s32 ndsRendererPrepareNativeKirbyVulcan(void)
{
    u32 i;
    if (sVulcanTextureGeneration != gNdsTaskmanHeapGeneration)
    {
        memset(sVulcanTextureNames, 0, sizeof(sVulcanTextureNames));
        sVulcanTextureGeneration = gNdsTaskmanHeapGeneration;
    }
    for (i = 0u; i < 5u; i++)
    {
        if (sVulcanTextureNames[i] == 0u &&
            !ndsRendererHardwarePrepareIFCommonCloudAtlas(16u, (i == 4u) ? 16u : 32u,
                sVulcanPalettes[i], ndsVulcanFill, (void *)(uintptr_t)i,
                &sVulcanTextureNames[i])) return FALSE;
    }
    return TRUE;
}

s32 ndsRendererSubmitNativeKirbyVulcan(u32 root,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererInputVertex vertices[14];
    NDSRendererMatrix20p12 composed;
    const s16 *positions = (root == 0u) ? sVulcanVertices0 : sVulcanVertices1;
    const u32 *colors = (root == 0u) ? sVulcanColors0 : sVulcanColors1;
    const u16 *indices = (root == 0u) ? sVulcanIndices0 : sVulcanIndices1;
    u32 vertex_count = (root == 0u) ? 14u : 4u;
    u32 triangle_count = (root == 0u) ? 12u : 2u;
    u32 first_bank = (root == 0u) ? 0u : 4u;
    u32 bank_end = (root == 0u) ? 4u : 5u;
    u32 i, bank, alpha, material, use_material, use_vertex;
    const NDSRendererTileState *tile;
    s32 offset;
    if (root > 1u || config == NULL || stats == NULL ||
        config->initial_projection == NULL || config->initial_modelview == NULL ||
        !ndsRendererPrepareNativeKirbyVulcan()) return FALSE;
    for (i = 0u; i < vertex_count; i++)
    {
        const s16 *v = positions + i * 5u;
        u32 c = colors[i];
        vertices[i] = (NDSRendererInputVertex){v[0],v[1],v[2],v[3],v[4],c>>24,c>>16,c>>8,c};
    }
    if (root == 0u) ndsVulcanSetup0(stats); else ndsVulcanSetup1(stats);
    /* efDisplayCLDProcDisplay clears Z on source link 15. The shared effect
     * adapter supplies a depth-enabled seed, so own this immutable layer rule. */
    stats->geometry_mode &= ~NDS_RENDERER_GEOM_ZBUFFER;
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    material = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    offset = ndsRendererHardwareTextureFilterOffset(stats);
    ndsRendererMtxMul20p12(config->initial_modelview, config->initial_projection, &composed);
    ndsRendererLoadHardwareMatrices(NULL, FALSE);
    for (bank = first_bank; bank < bank_end && alpha != 0u; bank++)
    {
        u32 corner;
        s32 painter_depth = 0;
        u32 params = ndsRendererHardwareTextureParams(stats, tile, 16u, (bank == 4u) ? 16u : 32u);
        ndsRendererHardwareBindTextureName(stats, sVulcanTextureNames[bank]);
        ndsRendererHardwareApplyTextureParams(ndsRendererHardwareMergeTextureParams(params));
        sNdsRendererHardwareActiveTextureEntry = NULL;
        stats->hardware_texture_ready_count++;
        ndsRendererHardwareBeginTriangleBatch(stats, TRUE, sVulcanTextureNames[bank],
            ndsRendererHardwarePolyFmt(stats, alpha), sNdsRendererHardwareMatrixMode,
            sNdsRendererHardwareMatrixGeneration);
        for (corner = 0u; corner < triangle_count * 3u; corner++)
        {
            const NDSRendererInputVertex *v = &vertices[indices[corner]];
            NDSRendererClipVertex20p12 clip;
            if ((corner % 3u) == 0u) painter_depth = ndsRendererHardwareNextProjectedDepth();
            glColor(ndsRendererHardwarePackedVertexColor(stats, v, material,
                use_material, use_vertex, 0u, FALSE, 0u));
            glTexCoord2t16(ndsRendererHardwareTexCoord(v->s, stats->texture_scale_s, tile->uls, offset),
                ndsRendererHardwareTexCoord(v->t, stats->texture_scale_t, tile->ult, offset));
            ndsRendererTransformVertex20p12(&composed, v, &clip);
            stats->matrix_transform_count++;
            stats->transformed_vertex_count++;
            ndsRendererHardwareClipVertex(&clip, painter_depth
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                , NULL
#endif
                );
        }
        for (i = 0u; i < triangle_count; i++) ndsRendererProfileRecordHardwareTriangle();
        stats->hardware_triangle_count += triangle_count;
        stats->hardware_vertex_count += triangle_count * 3u;
        if (stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER)
            stats->hardware_zbuffer_triangle_count += triangle_count;
        sNdsRendererHardwareSubmitted = TRUE;
        ndsRendererHardwareEndBatch();
    }
    ndsRendererHardwareEnterProjectedForeground();
    if (alpha != 0u) gNdsKirbyVulcanNativeDraws[root]++;
    if (root == 0u) ndsVulcanFinish0(stats); else ndsVulcanFinish1(stats);
    return TRUE;
}
#endif
