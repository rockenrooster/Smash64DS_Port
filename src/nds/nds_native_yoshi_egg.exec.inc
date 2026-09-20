/* Yoshi shared egg native executor. TEXTUAL INCLUDE ONLY.
 *
 * YoshiModel root 0xA860 is shared by the Up-B EggThrow weapon and the
 * shield/intro egg effects.  The root owns its CI4 palette/image/geometry and
 * has no MObj, so only the source DObj transform remains live. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_YOSHI
#if NDS_P2_YOSHI_BUG_PROOF
volatile u32 gNdsYoshiBugProofSharedEggDrawCount;
#endif

static u32 sNdsYoshiEggTextureName;
static u32 sNdsYoshiEggTextureGeneration;

static s32 ndsYoshiEggTextureFill(u8 *pixels, u32 bytes, void *user_data)
{
    (void)user_data;
    if ((pixels == NULL) || (bytes < NDS_NATIVE_YOSHI_EGG_IMAGE_BYTES))
    {
        return FALSE;
    }
    memcpy(pixels, sNdsNativeYoshiEggTexels,
           NDS_NATIVE_YOSHI_EGG_IMAGE_BYTES);
    return TRUE;
}

static s32 ndsYoshiEggEnsureTexture(void)
{
    if (sNdsYoshiEggTextureGeneration != gNdsTaskmanHeapGeneration)
    {
        sNdsYoshiEggTextureName = 0u;
        sNdsYoshiEggTextureGeneration = gNdsTaskmanHeapGeneration;
    }
    if (sNdsYoshiEggTextureName != 0u)
    {
        return TRUE;
    }
    return ndsRendererHardwarePrepareIFCommonPal16Atlas(
        64u, 64u, sNdsNativeYoshiEggPalette,
        ndsYoshiEggTextureFill, NULL, &sNdsYoshiEggTextureName);
}

static s32 ndsYoshiEggBindTexture(
    NDSRendererStats *stats, NDSRendererTraversalState *state)
{
    const NDSRendererTileState *tile;
    u32 params;

    if ((stats == NULL) || (state == NULL) ||
        (sNdsYoshiEggTextureName == 0u))
    {
        return FALSE;
    }
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    params = ndsRendererHardwareTextureParams(stats, tile, 64u, 64u);
    ndsRendererHardwareBindTextureName(stats, sNdsYoshiEggTextureName);
    ndsRendererHardwareApplyTextureParams(
        ndsRendererHardwareMergeTextureParams(params));
    sNdsRendererHardwareActiveTextureEntry = NULL;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
    stats->hardware_texture_width = tile->width;
    stats->hardware_texture_height = tile->height;
    state->texture_prepare_valid = TRUE;
    state->texture_prepare_enabled = TRUE;
    state->texture_prepare_name = sNdsYoshiEggTextureName;
    state->texture_prepare_scale_s = stats->texture_scale_s;
    state->texture_prepare_scale_t = stats->texture_scale_t;
    state->texture_prepare_origin_s = tile->uls;
    state->texture_prepare_origin_t = tile->ult;
    state->texture_prepare_offset = ndsRendererHardwareTextureFilterOffset(stats);
    state->texture_prepare_vertex_flags |= NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
    ndsRendererProfileRecordTexturePrepare();
    return TRUE;
}

sb32 ndsRendererSubmitNativeYoshiEgg(
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_YOSHI_EGG_VERTEX_COUNT];
    const NDSRendererTileState *tile;
    u32 material_color;
    u32 use_material;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    s32 texture_offset;
    u32 corner;
    u32 i;

    if ((config == NULL) || (stats == NULL) ||
        (config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        return FALSE;
    }

    for (i = 0u; i < NDS_NATIVE_YOSHI_EGG_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeYoshiEggVerts[i * 5u];
        u32 rgba = sNdsNativeYoshiEggVertColors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    /* The setup recorder preserves source RDP state. The two SETTIMG pointers
     * are identity-only here: compact FPC deliberately prunes both payloads and
     * the DS-ready AOT texture below owns the actual upload. */
    ndsNativeYoshiEggSetup(
        stats, sNdsNativeYoshiEggPalette, sNdsNativeYoshiEggTexels);
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if ((ndsYoshiEggEnsureTexture() == FALSE) ||
        (ndsYoshiEggBindTexture(stats, &state) == FALSE))
    {
        stats->hardware_texture_reject_count++;
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
        for (corner = 0u; corner < NDS_NATIVE_YOSHI_EGG_CORNER_COUNT; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeYoshiEggTriIndices[corner]];

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
        for (i = 0u; i < NDS_NATIVE_YOSHI_EGG_TRIANGLE_COUNT; i++)
        {
            ndsRendererProfileRecordHardwareTriangle();
        }
        stats->hardware_triangle_count += NDS_NATIVE_YOSHI_EGG_TRIANGLE_COUNT;
        if ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) != 0u)
        {
            stats->hardware_zbuffer_triangle_count +=
                NDS_NATIVE_YOSHI_EGG_TRIANGLE_COUNT;
        }
        stats->hardware_vertex_count += NDS_NATIVE_YOSHI_EGG_CORNER_COUNT;
        sNdsRendererHardwareSubmitted = TRUE;
#if NDS_P2_YOSHI_BUG_PROOF
        gNdsYoshiBugProofSharedEggDrawCount++;
#endif
    }
    ndsRendererHardwareEndBatch();
    ndsNativeYoshiEggFinish(stats);
    return TRUE;
}

#endif
