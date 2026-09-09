/* Link's Bomb GX executor. TEXTUAL INCLUDE ONLY.
 * File 353 roots 0x16f8 and 0x17e8 are the TWO lists of ONE item.  LinkMain's
 * ITAttributes at 0x40 (llLinkMainBombItemAttributes, reloc_data.us.h:3709)
 * names DObjDesc 0x18d8, whose entry 1 draws 0x16f8 into DL head 0 and whose
 * entry 2 draws 0x17e8 into DL head 1; itDisplayColAnimXLU walks that tree
 * ONCE (itdisplay.c:305), so both arrive per drawn frame in that order.
 * ITAttributes.p_mobjsubs is NULL, so neither DObj has an MObj -- "material 0"
 * -- and each list carries its own complete binding out of file 353's own
 * internal fixups.  The adapter has already COMPARED both relocated SETTIMG
 * words against the loaded file's base, so nothing is re-derived here, and
 * there is no display-list scan or execution on this path.
 *
 * THE ENV COLOUR IS LIVE AND IS NOT OURS.  itDisplayColAnimXLU emits the
 * item's ColAnim env into both heads before the model (itdisplay.c:294-302),
 * and the body combiner's cycle 1 is (ENV - COMBINED)*ENV_ALPHA + COMBINED:
 * that IS the damage flash.  The item layer already seeded it into
 * render_stats (renderer_adapter_stage.c, sNdsRendererAdapterItemEnvColor)
 * before this call, so the body setup must not touch env.  The fuse list
 * carries its own prim/env words and overrides it, exactly as the source does. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_LINK

static sb32 ndsNativeLinkBombEmit(
    const s16 *verts, const u32 *colors,
    const NDSRendererConfig *config, NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    NDSRendererInputVertex vertices[NDS_NATIVE_LINK_BOMB_VERTEX_COUNT];
    const NDSRendererTileState *tile;
    u32 material_color;
    u32 use_material;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    s32 texture_offset;
    u32 corner;
    u32 i;

    for (i = 0u; i < NDS_NATIVE_LINK_BOMB_VERTEX_COUNT; i++)
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
    if (ndsRendererHardwareBindTexture(stats, config, state) == FALSE)
    {
        gNdsLinkBombSubmitStep = 3u;
        return FALSE;
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    /* All eight source vertex alpha bytes are 0 and the generator promotes
     * them to 0xff, because POLY_ALPHA 0 is WIREFRAME on DS rather than
     * invisible.  A zero here is therefore the item layer's blender, not the
     * asset; witness it rather than let an empty draw read as success. */
    gNdsLinkBombAlpha = poly_alpha;
    if (poly_alpha != 0u)
    {
        generation = ndsRendererNextMatrixGeneration();
        ndsRendererLoadHardwareSplitMatrices(
            config->initial_projection, config->initial_modelview, generation);
        ndsRendererHardwareBeginTriangleBatch(
            stats, TRUE, sNdsRendererHardwareBoundTextureName,
            ndsRendererHardwarePolyFmt(stats, poly_alpha),
            sNdsRendererHardwareMatrixMode, generation);
        for (corner = 0u; corner < NDS_NATIVE_LINK_BOMB_CORNER_COUNT; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeLinkBombTriIndices[corner]];

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
        for (i = 0u; i < NDS_NATIVE_LINK_BOMB_TRIANGLE_COUNT; i++)
        {
            ndsRendererProfileRecordHardwareTriangle();
        }
        stats->hardware_triangle_count += NDS_NATIVE_LINK_BOMB_TRIANGLE_COUNT;
        stats->hardware_zbuffer_triangle_count +=
            NDS_NATIVE_LINK_BOMB_TRIANGLE_COUNT;
        stats->hardware_vertex_count += NDS_NATIVE_LINK_BOMB_CORNER_COUNT;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    return TRUE;
}

sb32 ndsRendererSubmitNativeLinkBomb(
    u32 root_offset, const void *file_base, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base;
    NDSRendererTraversalState state;

    /* Split so the witness names WHICH argument was wrong; the adapter has
     * already proved the item kind, the NULL MObj and both relocated pointers
     * by the time it calls here, so a step-11 decline was never one of those. */
    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsLinkBombSubmitStep = 11u;
        return FALSE;
    }
    gNdsLinkBombRoot = root_offset;
    gNdsLinkBombProjection = (u32)(uintptr_t)config->initial_projection;
    gNdsLinkBombModelview = (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        /* Both NULL is a different failure from the split-camera case the
         * caller repairs on its config copy; keep it loud rather than
         * inventing a second identity here. */
        gNdsLinkBombSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_LINK_BOMB_FILE_END)
    {
        gNdsLinkBombSubmitStep = 2u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    if (root_offset == NDS_NATIVE_LINK_BOMB_BODY_ROOT)
    {
        const void *tlut = base + NDS_NATIVE_LINK_BOMB_TLUT_OFFSET;
        const void *image = base + NDS_NATIVE_LINK_BOMB_BODY_IMAGE_OFFSET;

        gNdsLinkBombTlut = (u32)(uintptr_t)tlut;
        gNdsLinkBombImage = (u32)(uintptr_t)image;
        ndsNativeLinkBombBodySetup(stats, tlut, image);
        if (ndsNativeLinkBombEmit(
                sNdsNativeLinkBombBodyVerts, sNdsNativeLinkBombBodyVertColors,
                config, stats, &state) == FALSE)
        {
            return FALSE;
        }
        ndsNativeLinkBombBodyFinish(stats);
    }
    else if (root_offset == NDS_NATIVE_LINK_BOMB_FUSE_ROOT)
    {
        const void *image = base + NDS_NATIVE_LINK_BOMB_FUSE_IMAGE_OFFSET;

        /* The fuse list is IA8: it loads no TLUT, and the source does not
         * reset one either.  stats->texture_tlut_image therefore still holds
         * whatever the body list latched, which is deterministic within a
         * build and irrelevant to an IA texel.  Do not invent a reset the
         * source never emits. */
        gNdsLinkBombTlut = 0u;
        gNdsLinkBombImage = (u32)(uintptr_t)image;
        ndsNativeLinkBombFuseSetup(stats, image);
        if (ndsNativeLinkBombEmit(
                sNdsNativeLinkBombFuseVerts, sNdsNativeLinkBombFuseVertColors,
                config, stats, &state) == FALSE)
        {
            return FALSE;
        }
        ndsNativeLinkBombFuseFinish(stats);
    }
    else
    {
        /* An admitted root this owner was not baked for.  Decline loudly
         * rather than draw the wrong quad. */
        gNdsLinkBombSubmitStep = 13u;
        return FALSE;
    }
    gNdsLinkBombSubmitStep = 9u;
    return TRUE;
}

#endif
