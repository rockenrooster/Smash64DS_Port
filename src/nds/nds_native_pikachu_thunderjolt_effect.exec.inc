/* Pikachu Thunder Jolt EFFECT GX executor.  TEXTUAL INCLUDE ONLY.
 * File 342 (PikachuSpecial3) root 0x2170 is twenty-nine Gfx words drawing one
 * textured quad.  It is NOT either Thunder Jolt weapon: it is the effect the
 * jolt spawns beside itself, `efManagerPikachuThunderJoltMakeEffect`
 * (efmanager.c:4536) building `dEFManagerThunderJoltEffectDesc`
 * (efmanager.c:670) from `llPikachuSpecial3ThunderJoltDObjDesc` at 0x2258.
 * That is why the failure record named a `nGCCommonKindEffect` GObj in the
 * same asset as both jolts.
 *
 * The list is the air jolt's shell with ONE word changed.  Word 17 is a
 * segment-0xE call where the air jolt bakes a second SETTIMG, so the IMAGE is
 * live -- the Mushroom Kingdom Pakkun contract -- and everything else bakes:
 * combiner, blender, three tiles, the 16-entry RGBA5551 TLUT at 0x1c70,
 * texture state, tile size, load block, both geometry modes and all four
 * other-mode words.  The DObjDesc carries an MObjSub (0x20a0, head 1 valid)
 * and a MatAnimJoint (0x2350), which is why the material is non-NULL here
 * where the air jolt's is NULL.
 *
 * WORD ORDER.  The generator deliberately stops its setup at word 16, because
 * the source binds palette/TLUT/texture-state/tile-size, THEN takes the live
 * image at word 17, and only then loads the block at word 19.  So the material
 * is applied between Setup and LoadBlock here.  Do not fold them: a load block
 * issued before its image is a different program.
 *
 * There is no display-list scan or execution on this path. */
#if NDS_RENDERER_HW_TRIANGLES

sb32 ndsRendererSubmitNativePikachuThunderJoltEffect(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *actor_base = (const u8 *)actor_base_ptr;
    const void *tlut;
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_THUNDERJOLTFX_VERTEX_COUNT];
    const NDSRendererTileState *tile;
    u32 material_color;
    u32 use_material;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    s32 texture_offset;
    u32 corner;
    u32 i;

    if ((actor_base == NULL) || (material == NULL) || (config == NULL) ||
        (stats == NULL))
    {
        gNdsThunderJoltFxSubmitStep = 11u;
        return FALSE;
    }
    gNdsThunderJoltFxProjection = (u32)(uintptr_t)config->initial_projection;
    gNdsThunderJoltFxModelview = (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsThunderJoltFxSubmitStep = 12u;
        return FALSE;
    }
    if ((actor_bytes < NDS_NATIVE_THUNDERJOLTFX_TLUT_END) ||
        (actor_bytes < (NDS_NATIVE_THUNDERJOLTFX_ROOT +
                        NDS_NATIVE_THUNDERJOLTFX_DL_BYTES)))
    {
        gNdsThunderJoltFxSubmitStep = 2u;
        return FALSE;
    }
    /* The closed material contract, re-checked here rather than trusted from
     * the adapter: anything broader than the measured CURRENT_IMAGE would mean
     * this specialization is silently dropping source presentation. */
    if (material->effects != NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
    {
        gNdsThunderJoltFxEffectsRejected = material->effects;
        gNdsThunderJoltFxSubmitStep = 4u;
        return FALSE;
    }
    tlut = actor_base + NDS_NATIVE_THUNDERJOLTFX_TLUT_OFFSET;
    gNdsThunderJoltFxTlut = (u32)(uintptr_t)tlut;

    for (i = 0u; i < NDS_NATIVE_THUNDERJOLTFX_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeThunderJoltFxVerts[i * 5u];
        u32 rgba = sNdsNativeThunderJoltFxVertColors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeThunderJoltFxSetup(stats, tlut);
    ndsRendererNativeApplyMaterial(material, stats, &state);
    ndsNativeThunderJoltFxLoadBlock(stats);
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, config, &state) == FALSE)
    {
        gNdsThunderJoltFxSubmitStep = 3u;
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
    gNdsThunderJoltFxAlpha = poly_alpha;
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
             corner < NDS_NATIVE_THUNDERJOLTFX_CORNER_COUNT; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeThunderJoltFxTriIndices[corner]];

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
        for (i = 0u; i < NDS_NATIVE_THUNDERJOLTFX_TRIANGLE_COUNT; i++)
        {
            ndsRendererProfileRecordHardwareTriangle();
        }
        stats->hardware_triangle_count +=
            NDS_NATIVE_THUNDERJOLTFX_TRIANGLE_COUNT;
        stats->hardware_zbuffer_triangle_count +=
            NDS_NATIVE_THUNDERJOLTFX_TRIANGLE_COUNT;
        stats->hardware_vertex_count +=
            NDS_NATIVE_THUNDERJOLTFX_CORNER_COUNT;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    ndsNativeThunderJoltFxFinish(stats);
    gNdsThunderJoltFxSubmitStep = 9u;
    return TRUE;
}

#endif
