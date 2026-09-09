/* Saffron City Marumine GX executor. TEXTUAL INCLUDE ONLY.
 * File 159 root 0x06a0 is thirty Gfx words drawing ONE camera-facing textured
 * quad: four vertices at 0x0660, a 16-entry RGBA16 TLUT at 0x0438 and a CI4
 * 32x32 image at 0x0460, all three internal to file 159 (the file has zero
 * external fixups).  GRYamabukiMap's ITAttributes at 0x104 has p_mobjsubs
 * NULL, so the DObj carries no MObj -- "material 0" is that NULL, NOT
 * "untextured" -- and there is no segment-E hook anywhere in the program.
 *
 * NOTHING HERE IS LIVE.  is_display_colanim is clear, so itDisplayOPAProcDisplay
 * (itdisplay.c:189) emits no ColAnim env before the model, and the combiner is
 * (TEXEL0 - 0) * SHADE + 0 with alpha TEXEL0_A in BOTH cycles: no PRIM and no
 * ENV are read at all.  The item layer's seeded prim/env in render_stats
 * therefore cannot change this draw, and this owner must not invent a colour of
 * its own.  The only live input is the DObj transform, which arrives through
 * config's two matrices.  There is no display-list scan or execution on this
 * path.
 *
 * The adapter has already COMPARED both relocated SETTIMG words against the
 * loaded file's own base, so nothing is re-derived here. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI

sb32 ndsRendererSubmitNativeYamabukiMarumine(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *actor_base = (const u8 *)actor_base_ptr;
    const void *tlut;
    const void *image;
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_MARUMINE_VERTEX_COUNT];
    const NDSRendererTileState *tile;
    u32 material_color;
    u32 use_material;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    s32 texture_offset;
    u32 corner;
    u32 i;

    /* Split so the witness names WHICH argument was wrong.  The adapter has
     * already proved the item kind, the NULL MObj, the DL head and both
     * relocated pointers by the time it calls here, so a step-11 decline was
     * never going to be one of those. */
    if ((actor_base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsYamabukiMarumineSubmitStep = 11u;
        return FALSE;
    }
    gNdsYamabukiMarumineProjection = (u32)(uintptr_t)config->initial_projection;
    gNdsYamabukiMarumineModelview = (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        /* Both NULL is a different failure from the split-camera case the
         * caller repairs on its config copy; keep it loud rather than
         * inventing a second identity here. */
        gNdsYamabukiMarumineSubmitStep = 12u;
        return FALSE;
    }
    if ((actor_bytes < NDS_NATIVE_MARUMINE_FILE_END) ||
        (actor_bytes < (NDS_NATIVE_MARUMINE_ROOT +
                        NDS_NATIVE_MARUMINE_DL_BYTES)))
    {
        gNdsYamabukiMarumineSubmitStep = 2u;
        return FALSE;
    }
    tlut = actor_base + NDS_NATIVE_MARUMINE_TLUT_OFFSET;
    image = actor_base + NDS_NATIVE_MARUMINE_IMAGE_OFFSET;
    gNdsYamabukiMarumineTlut = (u32)(uintptr_t)tlut;
    gNdsYamabukiMarumineImage = (u32)(uintptr_t)image;

    for (i = 0u; i < NDS_NATIVE_MARUMINE_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeMarumineVerts[i * 5u];
        u32 rgba = sNdsNativeMarumineVertColors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    /* The program writes its own three other-mode words, so it does NOT
     * inherit the item layer's blender: it sets it and restores it, exactly as
     * the source list does. */
    ndsNativeMarumineSetup(stats, tlut, image);
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, config, &state) == FALSE)
    {
        gNdsYamabukiMarumineSubmitStep = 3u;
        return FALSE;
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    /* The four source vertex alpha bytes are 0 and the generator promotes them
     * to 0xff, because POLY_ALPHA 0 is WIREFRAME on DS rather than invisible.
     * A zero here is therefore the inherited item-layer blender's doing, not
     * the asset's; witness it rather than let a skipped draw read as success. */
    gNdsYamabukiMarumineAlpha = poly_alpha;
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
             corner < NDS_NATIVE_MARUMINE_CORNER_COUNT; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeMarumineTriIndices[corner]];

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
        for (i = 0u; i < NDS_NATIVE_MARUMINE_TRIANGLE_COUNT; i++)
        {
            ndsRendererProfileRecordHardwareTriangle();
        }
        stats->hardware_triangle_count +=
            NDS_NATIVE_MARUMINE_TRIANGLE_COUNT;
        stats->hardware_zbuffer_triangle_count +=
            NDS_NATIVE_MARUMINE_TRIANGLE_COUNT;
        stats->hardware_vertex_count +=
            NDS_NATIVE_MARUMINE_CORNER_COUNT;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    ndsNativeMarumineFinish(stats);
    gNdsYamabukiMarumineSubmitStep = 9u;
    return TRUE;
}

#endif
