/* Dream Land Bronto Burt GX executor (new, TEXTUAL INCLUDE ONLY).
 *
 * Included from inside src/nds/nds_renderer_native_owners.c AFTER the
 * Bronto packet include (see patch), so this file sees that TU's scope
 * (NDSRenderer*, ndsRendererHardware*, record setters) without a new link
 * unit or private renderer state of its own. Never compile standalone.
 * Requires the packet tables above it; fails the build otherwise.
 *
 * Mirrors ndsRendererSubmitNativeTaruCann (3-joint tree, single quad run,
 * live locals) with one difference: the source word-16 material hook is
 * replaced by e0's LIVE sprite frame
 * (mobj->sub.sprites[mobj->texture_id_curr], the exact image
 * objdisplay.c:1421-1430 submits). The MObj lives on e0 (the NULL-dl
 * intermediate: mobjlink_0x31F4 has exactly one entry, pairing node 0 of
 * the e0 walk), never on the drawable e1. The adapter slot owns that read
 * and passes the validated frame index; this executor emits the frame's exact
 * SetTextureImage words (w0 = 0xFD500000, the taru render-image
 * precedent) through the same record call the fighter production path
 * uses, so ndsRendererHardwareAlpha and the shared texture cache take the
 * exact production path. Frame >= 3 is a corrupt topology, not a frame:
 * fail closed, never a stale image.
 */
#if NDS_RENDERER_HW_TRIANGLES

#ifndef NDS_NATIVE_ACTOR_EF_BRONTO_EXEC_INC
#define NDS_NATIVE_ACTOR_EF_BRONTO_EXEC_INC

#include <nds/nds_native_actor_ef_bronto.h>

/* One Bronto instance: 1 drawable x 2 source tris = 2 live triangles.
 * frame is e0's MObj image index (0..2). The source's current material
 * animation binding leaves e0 unanimated, so normal play selects frame 0. */
sb32 ndsRendererSubmitNativeEfBronto(const void *packet_base,
    u32 packet_bytes, const NDSRendererNativeFighterHierarchy *hierarchy,
    const NDSRendererMatrix20p12 *drawable_mvp, u32 frame,
    u32 initial_geometry_mode, NDSRendererStats *stats)
{
    const u8 *asset_base = (const u8 *)packet_base;
    NDSRendererConfig config = { 0 };
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_ACTOR_EF_BRONTO_VERT_COUNT];
    const NDSRendererTileState *tile;
    u32 use_material;
    u32 use_vertex;
    s32 texture_offset;
    u32 generation;
    u32 material_color;
    u32 poly_alpha;
    u32 corner;
    u32 i;
    u32 img_off;
    u32 img_size;

    if ((packet_base == NULL) || (stats == NULL) || (hierarchy == NULL) ||
        (drawable_mvp == NULL) ||
        (hierarchy->joint_count != NDS_NATIVE_ACTOR_EF_BRONTO_JOINT_COUNT) ||
        (hierarchy->projection == NULL) ||
        (hierarchy->camera_modelview == NULL) ||
        (hierarchy->joint_locals == NULL) ||
        (hierarchy->joint_parents == NULL) ||
        (hierarchy->joint_bindings == NULL) ||
        (frame > NDS_NATIVE_ACTOR_EF_BRONTO_FRAME_MAX))
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_NATIVE_ACTOR_EF_BRONTO_JOINT_COUNT; i++)
    {
        if ((hierarchy->joint_parents[i] != sNdsNativeActorEfBrontoJointParents[i]) ||
            (hierarchy->joint_bindings[i] != sNdsNativeActorEfBrontoBindings[i]) ||
            (hierarchy->joint_locals[i].m[0][3] != 0) ||
            (hierarchy->joint_locals[i].m[1][3] != 0) ||
            (hierarchy->joint_locals[i].m[2][3] != 0) ||
            (hierarchy->joint_locals[i].m[3][3] != 4096))
        {
            return FALSE;
        }
    }
    /* Fail closed before emitting: the run must own the shared pair, and
     * both the static palette and the live frame image must sit inside
     * the asset range. */
    if ((sNdsNativeActorEfBrontoRuns[0] != 2u) ||
        (sNdsNativeActorEfBrontoRuns[1] != 0u) ||
        (sNdsNativeActorEfBrontoRuns[2] != NDS_NATIVE_ACTOR_EF_BRONTO_TRIANGLE_COUNT))
    {
        return FALSE;
    }
    img_off = sNdsNativeActorEfBrontoTextureEpoch[2u + frame * 2u];
    img_size = sNdsNativeActorEfBrontoTextureEpoch[2u + frame * 2u + 1u];
    if ((packet_bytes < sNdsNativeActorEfBrontoTextureEpoch[0] +
                         sNdsNativeActorEfBrontoTextureEpoch[1]) ||
        (packet_bytes < img_off + img_size))
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_NATIVE_ACTOR_EF_BRONTO_VERT_COUNT; i++)
    {
        const s16 *v = &sNdsNativeActorEfBrontoVerts[i * 5u];
        u32 rgba = sNdsNativeActorEfBrontoVertColors[i];
        vertices[i] = (NDSRendererInputVertex){ v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba };
    }
    config.initial_projection = hierarchy->projection;
    config.initial_modelview = hierarchy->camera_modelview;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    ndsRendererInitTraversalState(&state, &config, stats, NULL, NULL, 0u);
    ndsNativeActorEfBrontoSetupState(stats, asset_base);
    if ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u)
    {
        return FALSE;
    }
    /* Source word-16 hook, live: the MObj program's SetTextureImage over
     * the current sprite frame (objdisplay.c:1421-1430). */
    ndsRendererRecordSetImage(stats, 0xfd500000u,
        (u32)(uintptr_t)(asset_base + img_off));
    (void)img_size;
    ndsNativeActorEfBrontoSetupTail(stats, asset_base);
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, &config, &state) == FALSE)
    {
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
        ndsRendererLoadHardwareRawComposedMatrix(drawable_mvp, generation);
        ndsRendererHardwareBeginTriangleBatch(stats, TRUE,
            sNdsRendererHardwareBoundTextureName,
            ndsRendererHardwarePolyFmt(stats, poly_alpha),
            sNdsRendererHardwareMatrixMode, generation);
        for (corner = 0u; corner < NDS_NATIVE_ACTOR_EF_BRONTO_CORNER_COUNT; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeActorEfBrontoTriIndices[corner]];
            v16 x = ndsRendererHardwareVertexCoord(v->x, TRUE);
            v16 y = ndsRendererHardwareVertexCoord(v->y, TRUE);
            v16 z = ndsRendererHardwareVertexCoord(v->z, TRUE);

            glColor(ndsRendererHardwarePackedVertexColor(stats, v,
                material_color, use_material, use_vertex, 0u, FALSE, 0u));
            glTexCoord2t16(
                ndsRendererHardwareTexCoord(v->s, stats->texture_scale_s,
                    tile->uls, texture_offset),
                ndsRendererHardwareTexCoord(v->t, stats->texture_scale_t,
                    tile->ult, texture_offset));
            glVertex3v16(x, y, z);
        }
        ndsRendererProfileRecordHardwareTriangle();
        ndsRendererProfileRecordHardwareTriangle();
        stats->hardware_triangle_count += NDS_NATIVE_ACTOR_EF_BRONTO_TRIANGLE_COUNT;
        stats->hardware_zbuffer_triangle_count += NDS_NATIVE_ACTOR_EF_BRONTO_TRIANGLE_COUNT;
        stats->hardware_vertex_count += NDS_NATIVE_ACTOR_EF_BRONTO_CORNER_COUNT;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    ndsNativeActorEfBrontoFinishState(stats, asset_base);
    return TRUE;
}

#endif /* NDS_NATIVE_ACTOR_EF_BRONTO_EXEC_INC */
#endif
