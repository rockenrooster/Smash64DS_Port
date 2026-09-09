/* Pikachu air Thunder Jolt GX executor. TEXTUAL INCLUDE ONLY.
 * File 342 (PikachuSpecial3) root 0x0270 is twenty-nine Gfx words drawing one
 * textured quad.  PikachuSpecial1's WPAttributes at offset 0
 * (llPikachuSpecial1ThunderJoltAirWeaponAttributes, reloc_data.us.h:3725) point
 * their `data` field straight at this display list rather than at a DObjDesc,
 * exactly as Samus's Charge Shot does, and their WPDesc flag word is 0.
 *
 * The attributes live in one file and the geometry in another, which is why
 * file 342 holds NO pointer to its own root: the single referrer in the whole
 * game image is file 244's external fixup at slot 0.
 *
 * p_mobjsubs and p_matanim_joints are both NULL, so no DObj here carries an
 * MObj -- "material 0" is that NULL and NOT "untextured".  The list owns its
 * whole material out of file 342's own internal fixups: a 16-entry RGBA5551
 * TLUT at 0x0008 and a CI4 image at 0x0030 whose 512 loaded bytes end exactly
 * where the vertex pool at 0x0230 begins.  File 342 has zero external fixups,
 * so nothing here is a segment-E hook and no cross-file binding needs checking.
 *
 * The program writes its own other-mode words and restores them, so unlike the
 * Sector Z laser it does not inherit the weapon pass's blender.  The only live
 * input is the DObj transform, which arrives through config's two matrices.
 * There is no display-list scan or execution on this path. */
#if NDS_RENDERER_HW_TRIANGLES

sb32 ndsRendererSubmitNativePikachuThunderJolt(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *actor_base = (const u8 *)actor_base_ptr;
    const void *tlut;
    const void *image;
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_THUNDERJOLT_VERTEX_COUNT];
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
     * already proved the asset, the root, the Weapon GObj and the NULL MObj by
     * the time it calls here, so a step-11 decline was never one of those. */
    if ((actor_base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsThunderJoltSubmitStep = 11u;
        return FALSE;
    }
    gNdsThunderJoltProjection = (u32)(uintptr_t)config->initial_projection;
    gNdsThunderJoltModelview = (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        /* Both NULL is a different failure from the split-camera case the
         * caller repairs on its config copy; keep it loud. */
        gNdsThunderJoltSubmitStep = 12u;
        return FALSE;
    }
    if ((actor_bytes < NDS_NATIVE_THUNDERJOLT_FILE_END) ||
        (actor_bytes < (NDS_NATIVE_THUNDERJOLT_ROOT +
                        NDS_NATIVE_THUNDERJOLT_DL_BYTES)))
    {
        gNdsThunderJoltSubmitStep = 2u;
        return FALSE;
    }
    tlut = actor_base + NDS_NATIVE_THUNDERJOLT_TLUT_OFFSET;
    image = actor_base + NDS_NATIVE_THUNDERJOLT_IMAGE_OFFSET;
    gNdsThunderJoltTlut = (u32)(uintptr_t)tlut;
    gNdsThunderJoltImage = (u32)(uintptr_t)image;

    for (i = 0u; i < NDS_NATIVE_THUNDERJOLT_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeThunderJoltVerts[i * 5u];
        u32 rgba = sNdsNativeThunderJoltVertColors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeThunderJoltSetup(stats, tlut, image);
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, config, &state) == FALSE)
    {
        gNdsThunderJoltSubmitStep = 3u;
        return FALSE;
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    /* All four source vertex alpha bytes are 0 and the generator promotes them
     * to 0xff, because POLY_ALPHA 0 is WIREFRAME on DS rather than invisible.
     * A zero here is therefore the weapon layer's blender, not the asset;
     * witness it rather than let a skipped draw read as success. */
    gNdsThunderJoltAlpha = poly_alpha;
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
             corner < NDS_NATIVE_THUNDERJOLT_CORNER_COUNT; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeThunderJoltTriIndices[corner]];

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
        for (i = 0u; i < NDS_NATIVE_THUNDERJOLT_TRIANGLE_COUNT; i++)
        {
            ndsRendererProfileRecordHardwareTriangle();
        }
        stats->hardware_triangle_count +=
            NDS_NATIVE_THUNDERJOLT_TRIANGLE_COUNT;
        stats->hardware_zbuffer_triangle_count +=
            NDS_NATIVE_THUNDERJOLT_TRIANGLE_COUNT;
        stats->hardware_vertex_count +=
            NDS_NATIVE_THUNDERJOLT_CORNER_COUNT;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    ndsNativeThunderJoltFinish(stats);
    gNdsThunderJoltSubmitStep = 9u;
    return TRUE;
}

#endif
