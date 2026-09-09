/* Peach's Castle bumper GX executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x7558 is a fixed two-triangle quad over ONE resident 32x32
 * CI4 image; the live half is the MOBJ_FLAG_PALETTE selection between file
 * 86's 0x7260 and 0x7238.  There is no display-list scan or execution on this
 * path: every state word below came from the generator, and the only runtime
 * input is the typed material snapshot the adapter already built.
 *
 * THE ORDER IS THE DESIGN.  ndsRendererNativeApplyMaterial emits the source's
 * word-10 palette SETTIMG; ndsNativeCastleBumperSetupTail then emits the
 * source's word-12 LOADTLUT, which copies stats->texture_image into
 * stats->texture_tlut_image (nds_renderer_dl_core.c:1058).  Because
 * texture_tlut_image is part of the hardware texture cache key
 * (nds_renderer_textures_effects.c:10607), the two palettes get two cache
 * entries over one resident image and no second image is ever baked.  Swap
 * those two calls and both palettes key to ONE entry: the three-tick hit
 * flash silently stops appearing and nothing counts it. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_CASTLE

sb32 ndsRendererSubmitNativeCastleBumper(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *actor_base = (const u8 *)actor_base_ptr;
    const void *image;
    const void *palette0;
    const void *palette1;
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_CASTLE_BUMPER_VERTEX_COUNT];
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
     * already proved the kind, the MObj flags and the effects by the time it
     * calls here, so a step-11 or step-13 decline was never going to be one
     * of those. */
    if ((actor_base == NULL) || (material == NULL) || (config == NULL) ||
        (stats == NULL))
    {
        gNdsCastleBumperSubmitStep = 11u;
        return FALSE;
    }
    gNdsCastleBumperPaletteImage = material->palette_image;
    gNdsCastleBumperProjection = (u32)(uintptr_t)config->initial_projection;
    gNdsCastleBumperModelview = (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        /* Both NULL is a different failure from the split-camera case the
         * caller repairs on its config copy; keep it loud rather than
         * inventing a second identity here. */
        gNdsCastleBumperSubmitStep = 12u;
        return FALSE;
    }
    if (material->effects != NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
    {
        gNdsCastleBumperSubmitStep = 13u;
        return FALSE;
    }
    if (actor_bytes < NDS_NATIVE_CASTLE_BUMPER_IMAGE_END)
    {
        gNdsCastleBumperSubmitStep = 2u;
        return FALSE;
    }
    /* Direct source offsets, the same way the Pakkun owner addresses its own
     * frames. The compact-preview translation seam exists only under
     * NDS_P2_1P_GAME, and Peach's Castle is a VS stage; if a campaign build
     * ever creates this item, that seam has to be taken here as well. */
    image = actor_base + NDS_NATIVE_CASTLE_BUMPER_IMAGE_OFFSET;
    palette0 = actor_base + NDS_NATIVE_CASTLE_BUMPER_PALETTE0_OFFSET;
    palette1 = actor_base + NDS_NATIVE_CASTLE_BUMPER_PALETTE1_OFFSET;
    /* palette_id is an f32 the source writes only 0 or 1 into (itgbumper.c:50,
     * :76, :125).  Anything else would index past mobj->sub.palettes and hand
     * the TLUT loader a foreign pointer, so two exact addresses are the whole
     * legal set.  This also proves the material really selected THIS object's
     * palettes rather than a stale snapshot. */
    if (((const void *)(uintptr_t)material->palette_image != palette0) &&
        ((const void *)(uintptr_t)material->palette_image != palette1))
    {
        gNdsCastleBumperSubmitStep = 14u;
        return FALSE;
    }
    gNdsCastleBumperPaletteId =
        (((const void *)(uintptr_t)material->palette_image == palette1) ?
             1u : 0u);

    for (i = 0u; i < NDS_NATIVE_CASTLE_BUMPER_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeCastleBumperVerts[i * 5u];
        u32 rgba = sNdsNativeCastleBumperVertColors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeCastleBumperSetup(stats);
    /* Source word 10 is gSPDisplayList(0x0E000000).  The PALETTE-only MObj
     * emits one gDPSetTextureImage and EndDL (objdisplay.c:1261-1265);
     * translate it through the shared typed material path, never through a
     * segment-E Gfx stream.  MUST precede SetupTail's LOADTLUT. */
    ndsRendererNativeApplyMaterial(material, stats, &state);
    ndsNativeCastleBumperSetupTail(stats, image);
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, config, &state) == FALSE)
    {
        gNdsCastleBumperSubmitStep = 3u;
        return FALSE;
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    /* The four source vertex alpha bytes are 0 and the generator promotes them
     * to 0xFF, because POLY_ALPHA 0 is WIREFRAME on DS rather than invisible.
     * A zero here is therefore the inherited item-layer blender's doing, not
     * the asset's; witness it rather than let an empty draw read as success. */
    gNdsCastleBumperAlpha = poly_alpha;
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
             corner < NDS_NATIVE_CASTLE_BUMPER_CORNER_COUNT; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeCastleBumperTriIndices[corner]];

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
        for (i = 0u; i < NDS_NATIVE_CASTLE_BUMPER_TRIANGLE_COUNT; i++)
        {
            ndsRendererProfileRecordHardwareTriangle();
        }
        stats->hardware_triangle_count +=
            NDS_NATIVE_CASTLE_BUMPER_TRIANGLE_COUNT;
        stats->hardware_zbuffer_triangle_count +=
            NDS_NATIVE_CASTLE_BUMPER_TRIANGLE_COUNT;
        stats->hardware_vertex_count +=
            NDS_NATIVE_CASTLE_BUMPER_CORNER_COUNT;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    ndsNativeCastleBumperFinish(stats);
    gNdsCastleBumperSubmitStep = 9u;
    return TRUE;
}

#endif
