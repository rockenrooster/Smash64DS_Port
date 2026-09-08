/* Yoster cloud GX executor (new, TEXTUAL INCLUDE ONLY).
 *
 * Included from inside src/nds/nds_renderer_native_owners.c AFTER the
 * packet include (see patch), so this file sees that TU's scope
 * (NDSRenderer*, ndsRendererHardware*, record setters) without a new link
 * unit or private renderer state of its own. Never compile standalone.
 * Requires the packet tables above it; fails the build otherwise.
 *
 * Mirrors ndsRendererSubmitNativeTaruCann: validate the live 7-joint tree
 * against the shared packet, replay the source state words through the
 * same record calls the fighter production path uses, keep every live
 * local matrix (root translate is the yakumono collision pose, children
 * are live Tra+kind48 built by the adapter slot), forward each drawable's
 * LIVE prim alpha (Solid/Evaporate scripts run in the source MObj anims;
 * alpha 0 is the ordinary fully-evaporated state, not a broken topology)
 * through stats->prim_color so ndsRendererHardwareAlpha takes the exact
 * production path, and emit the real shared corners once per drawable
 * with live matrices. No generic DObj interpreter, no baked worlds.
 */
#if NDS_P2_STAGE_YOSTER && NDS_RENDERER_HW_TRIANGLES

#ifndef NDS_NATIVE_ACTOR_YOSTER_CLOUD_EXEC_INC
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_EXEC_INC

#include <nds/nds_native_actor_yoster_cloud.h>

/* One cloud instance: 3 drawables x 2 shared triangles = 6 live triangles
 * when solid. prim_alpha_* are the drawables' live MObj prim alphas
 * (mobj->sub.primcolor.s.a, the exact field objdisplay.c submits). */
sb32 ndsRendererSubmitNativeYosterCloud(const void *packet_base,
    u32 packet_bytes, const NDSRendererNativeFighterHierarchy *hierarchy,
    const NDSRendererMatrix20p12 *drawable_mvps,
    u32 prim_alpha_0, u32 prim_alpha_1, u32 prim_alpha_2,
    u32 initial_geometry_mode, NDSRendererStats *stats)
{
    const u8 *asset_base = (const u8 *)packet_base;
    NDSRendererConfig config = { 0 };
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_ACTOR_YOSTER_CLOUD_VERT_COUNT];
    const NDSRendererTileState *tile;
    u32 use_material;
    u32 use_vertex;
    s32 texture_offset;
    u32 generation;
    u32 alphas[3];
    u32 i;
    u32 d;

    alphas[0] = prim_alpha_0;
    alphas[1] = prim_alpha_1;
    alphas[2] = prim_alpha_2;
    if ((packet_base == NULL) || (stats == NULL) || (hierarchy == NULL) ||
        (drawable_mvps == NULL) ||
        (hierarchy->joint_count != NDS_NATIVE_ACTOR_YOSTER_CLOUD_JOINT_COUNT) ||
        (hierarchy->projection == NULL) ||
        (hierarchy->camera_modelview == NULL) ||
        (hierarchy->joint_locals == NULL) ||
        (hierarchy->joint_parents == NULL) ||
        (hierarchy->joint_bindings == NULL) ||
        (packet_bytes < NDS_NATIVE_ACTOR_YOSTER_CLOUD_SLAB_BYTES) ||
        (packet_bytes < sNdsNativeActorYosterCloudTextureEpoch[0] +
                        sNdsNativeActorYosterCloudTextureEpoch[1]))
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_NATIVE_ACTOR_YOSTER_CLOUD_JOINT_COUNT; i++)
    {
        if ((hierarchy->joint_parents[i] != sNdsNativeActorYosterCloudJointParents[i]) ||
            (hierarchy->joint_bindings[i] != sNdsNativeActorYosterCloudBindings[i]) ||
            (hierarchy->joint_locals[i].m[0][3] != 0) ||
            (hierarchy->joint_locals[i].m[1][3] != 0) ||
            (hierarchy->joint_locals[i].m[2][3] != 0) ||
            (hierarchy->joint_locals[i].m[3][3] != 4096))
        {
            return FALSE;
        }
    }
    if (alphas[0] > 255u || alphas[1] > 255u || alphas[2] > 255u)
    {
        return FALSE;
    }
    /* Fail closed before emitting: every run must own the shared pair. */
    for (d = 0u; d < NDS_NATIVE_ACTOR_YOSTER_CLOUD_DRAWABLE_COUNT; d++)
    {
        const u16 *run = &sNdsNativeActorYosterCloudRuns[d * 3u];

        if ((run[0] != 4u + d) || (run[1] != 0u) ||
            (run[2] != NDS_NATIVE_ACTOR_YOSTER_CLOUD_TRIANGLE_COUNT / 3u))
        {
            return FALSE;
        }
    }
    /* Fully evaporated on all three drawables: ordinary source state
     * (Evaporate script endpoint), same early-out shape as the barrel
     * hidden-subtree gate. */
    if ((alphas[0] | alphas[1] | alphas[2]) == 0u)
    {
        return TRUE;
    }
    for (i = 0u; i < NDS_NATIVE_ACTOR_YOSTER_CLOUD_VERT_COUNT; i++)
    {
        const s16 *v = &sNdsNativeActorYosterCloudVerts[i * 5u];
        u32 rgba = sNdsNativeActorYosterCloudVertColors[i];
        vertices[i] = (NDSRendererInputVertex){ v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba };
    }
    config.initial_projection = hierarchy->projection;
    config.initial_modelview = hierarchy->camera_modelview;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    ndsRendererInitTraversalState(&state, &config, stats, NULL, NULL, 0u);
    ndsNativeActorYosterCloudSetupState(stats, asset_base);
    /* Source MatAnim RGB pinned white by both scripts (154:182/184/202/204);
     * alpha stays live per drawable below. Seed before bind so the graded
     * A5I3 palette bakes the white endpoint with the live env colour. */
    stats->prim_color = 0xffffff00u;
    if ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u)
    {
        return FALSE;
    }
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, &config, &state) == FALSE)
    {
        return FALSE;
    }
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);

    for (d = 0u; d < NDS_NATIVE_ACTOR_YOSTER_CLOUD_DRAWABLE_COUNT; d++)
    {
        /* Run d lives on joint 4+d under mid 1+d under root 0; every local
         * is live (root = yakumono pose, drawables = Tra+kind48). */
        const u16 *run = &sNdsNativeActorYosterCloudRuns[d * 3u];
        u32 material_color;
        u32 poly_alpha;
        u32 corner;
        u32 first;
        u32 count;

        if (alphas[d] == 0u)
        {
            continue;
        }
        /* Segment-E material hook: the source invokes the drawable's live
         * material program here; its varying content is the animated prim,
         * RGB pinned white by both scripts, alpha live per drawable. */
        stats->prim_color = 0xffffff00u | (alphas[d] & 0xffu);
        material_color = ndsRendererHardwareColorSource(stats);
        poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
        if (poly_alpha == 0u)
        {
            continue;
        }
        generation = ndsRendererNextMatrixGeneration();
        ndsRendererLoadHardwareRawComposedMatrix(&drawable_mvps[d], generation);
        ndsRendererHardwareBeginTriangleBatch(stats, TRUE,
            sNdsRendererHardwareBoundTextureName,
            ndsRendererHardwarePolyFmt(stats, poly_alpha),
            sNdsRendererHardwareMatrixMode, generation);
        first = (u32)run[1] * 3u;
        count = (u32)run[2] * 3u;
        for (corner = first; corner < first + count; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeActorYosterCloudTriIndices[corner]];
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
        stats->hardware_triangle_count += run[2];
        stats->hardware_zbuffer_triangle_count += run[2];
        stats->hardware_vertex_count += (u32)run[2] * 3u;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    ndsNativeActorYosterCloudFinishState(stats, asset_base);
    return TRUE;
}

#endif /* NDS_NATIVE_ACTOR_YOSTER_CLOUD_EXEC_INC */
#endif
