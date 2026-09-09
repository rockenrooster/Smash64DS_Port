/* Pikachu ground Thunder Jolt GX executor. TEXTUAL INCLUDE ONLY.
 * File 342 DObjDesc 0x1888, reached from PikachuSpecial1's WPAttributes at
 * 0x34 (llPikachuSpecial1ThunderJoltGroundWeaponAttributes), has six drawable
 * children whose roots each draw ONE triangle and carry ONE segment-0xE call.
 * This is the segmented ground spark, sibling of the air jolt at root 0x0270.
 *
 * The live half is the CURRENT IMAGE and nothing else. A reconnaissance
 * witness read a material effects word of 0x200 --
 * NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE alone -- across all six roots,
 * and the source picks it with `mobj->texture_id_curr`
 * (wppikachuthunder.c:172 and :196, over the four textures at wpvars.h:50).
 * So the segment-0xE call is translated through the shared typed material
 * path exactly as the Mushroom Kingdom Pakkun does, never as a segment-E Gfx
 * stream, and every other word of the program is baked.
 *
 * The caller passes the root index, not the root offset: the generator already
 * flattened each root's three corners out of the source vertex cache in DRAW
 * order, which is why root 0x15c0's two G_VTX loads need no special case
 * here. */
#if NDS_RENDERER_HW_TRIANGLES

sb32 ndsRendererSubmitNativePikachuThunderGround(
    u32 root_index, const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_THUNDERGROUND_CORNER_COUNT];
    const NDSRendererTileState *tile;
    u32 material_color;
    u32 use_material;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    s32 texture_offset;
    u32 corner;
    u32 base;

    if ((material == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsThunderGroundSubmitStep = 11u;
        return FALSE;
    }
    if (root_index >= NDS_NATIVE_THUNDERGROUND_ROOT_COUNT)
    {
        /* An admitted root this owner was not baked for: decline loudly rather
         * than draw another segment's triangle in its place. */
        gNdsThunderGroundSubmitStep = 13u;
        return FALSE;
    }
    gNdsThunderGroundProjection = (u32)(uintptr_t)config->initial_projection;
    gNdsThunderGroundModelview = (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsThunderGroundSubmitStep = 12u;
        return FALSE;
    }
    /* The closed material contract, re-checked here rather than trusted from
     * the adapter: anything broader than the measured CURRENT_IMAGE would mean
     * this specialization is silently dropping source presentation. */
    if (material->effects != NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
    {
        gNdsThunderGroundEffectsRejected = material->effects;
        gNdsThunderGroundSubmitStep = 4u;
        return FALSE;
    }

    base = root_index * NDS_NATIVE_THUNDERGROUND_CORNER_COUNT;
    for (corner = 0u; corner < NDS_NATIVE_THUNDERGROUND_CORNER_COUNT; corner++)
    {
        const s16 *v = &sNdsNativeThunderGroundVerts[(base + corner) * 5u];
        u32 rgba = sNdsNativeThunderGroundVertColors[base + corner];

        vertices[corner] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeThunderGroundSetup(stats);
    ndsRendererNativeApplyMaterial(material, stats, &state);
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, config, &state) == FALSE)
    {
        gNdsThunderGroundSubmitStep = 3u;
        return FALSE;
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    /* A skipped draw must be visible in a counter rather than reading as a
     * success; POLY_ALPHA 0 is wireframe on DS, not invisible. */
    gNdsThunderGroundAlpha = poly_alpha;
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
             corner < NDS_NATIVE_THUNDERGROUND_CORNER_COUNT; corner++)
        {
            const NDSRendererInputVertex *v = &vertices[corner];

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
        stats->hardware_triangle_count += 1u;
        stats->hardware_zbuffer_triangle_count += 1u;
        stats->hardware_vertex_count += NDS_NATIVE_THUNDERGROUND_CORNER_COUNT;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    ndsNativeThunderGroundFinish(stats);
    gNdsThunderGroundSubmitStep = 9u;
    return TRUE;
}

#endif
