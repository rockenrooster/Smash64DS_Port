/* Castle Lakitu GX executor (new, TEXTUAL INCLUDE ONLY).
 *
 * Included from inside src/nds/nds_renderer_native_owners.c AFTER the
 * Lakitu packet include (see patch), so this file sees that TU's scope
 * (NDSRenderer*, ndsRendererHardware*, record setters) without a new link
 * unit or private renderer state of its own. Never compile standalone.
 * Requires the packet tables above it; fails the build otherwise.
 *
 * Mirrors ndsRendererSubmitNativeYosterCloud: validate the live 6-joint
 * tree against the shared packet, replay the source prelude words through
 * the same record calls the fighter production path uses, keep every live
 * local matrix (A = efGroundUpdatePhysics spawn pose, e0 = BB0 flight
 * sweep, e1 = C14 scale flap feeding e2's world, drawables = Tra+kind72
 * billboard built by the adapter slot), and emit the real per-epoch
 * corners once per drawable with live matrices. No generic DObj
 * interpreter, no baked worlds. The sub-DL call in DL_0x3F20 is already
 * flattened: run 0 carries the sub quad's epoch and corners.
 *
 * THREE-STAGE SUBMIT (partial-emission + cold-first-draw fix). The old
 * interleaved loop bound drawable 0 and emitted its triangles, then
 * discovered drawable 1/2's bind could fail; that FALSE made
 * reloc_backend_movement rerun the legacy tree over triangles GX had
 * already received. Stage 1 resolves every drawable's palette+image epoch
 * in the shared cache BEFORE any triangle, through
 * ndsRendererHardwareResolveResidentTexture -- the same pure preflight the
 * native-stage prepared run uses (resolved != NULL never uploads, so a
 * miss returns FALSE instead of allocating). A cold cache misses here, and
 * the previous shape returned FALSE straight into the generic fallback, so
 * the actor's first draw never stayed native.
 *
 * Discipline mirrors the prepared-native-stage owner: that path uploads
 * its pinned corpus once at stage entry (ndsBattlePrepareSceneTextures)
 * and per-frame PrepareRun does a pure resolve with a generation-fenced
 * commit. Lakitu has no stage-entry owner (Main owns the entry
 * allocators), so the executor carries the same two halves locally:
 * stage 2 is the bounded cold-upload pass -- one live
 * ndsRendererHardwareBindTexture per epoch, the TaruCann single-drawable
 * path generalized to three, zero triangles -- and stage 3 re-runs the
 * PURE resident preflight over all three epochs. The reverify is load-
 * bearing, not paranoia: a live bind uploads on miss and an upload can
 * evict, so three successful binds must NOT be assumed to leave all three
 * resident. Any stage-1/2/3 failure returns FALSE with ZERO hardware
 * emission -- the route counts it
 * (gNdsStageGCDrawAllLoopEfLakituRejectCount) and its legacy walk then
 * covers the actor exactly once. The warm steady state skips stage 2
 * entirely (three pure hits, no allocation, no I/O). Stage 4 emits with
 * the prepared-run commit sequence (entry LRU touch, BindTextureName,
 * ApplyTextureParams), which cannot fail and cannot evict. Every
 * per-drawable stats value the emission consumes (tile origins, scales,
 * filter offset, material flags, alpha) is captured in the final
 * successful pure pass while epoch d still owns the recorded tile state:
 * later epoch setups overwrite stats->texture_tiles, so pointers into it
 * must not survive across epochs.
 */
#if NDS_P2_STAGE_CASTLE && NDS_RENDERER_HW_TRIANGLES

#ifndef NDS_NATIVE_ACTOR_EF_LAKITU_EXEC_INC
#define NDS_NATIVE_ACTOR_EF_LAKITU_EXEC_INC

#include <nds/nds_native_actor_ef_lakitu.h>

/* One Lakitu instance: 3 drawables x 2 source tris = 6 live triangles. */
sb32 ndsRendererSubmitNativeEfLakitu(const void *packet_base,
    u32 packet_bytes, const NDSRendererNativeFighterHierarchy *hierarchy,
    const NDSRendererMatrix20p12 *drawable_mvps,
    u32 initial_geometry_mode, NDSRendererStats *stats)
{
    const u8 *asset_base = (const u8 *)packet_base;
    NDSRendererConfig config = { 0 };
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_ACTOR_EF_LAKITU_VERT_COUNT];
    NDSRendererHardwareResolvedTexture
        resolved[NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT];
    u32 texture_names[NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT];
    u32 tile_uls[NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT];
    u32 tile_ults[NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT];
    u32 scales_s[NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT];
    u32 scales_t[NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT];
    u32 material_colors[NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT];
    u32 use_materials[NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT];
    u32 use_vertexes[NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT];
    u32 poly_alphas[NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT];
    s32 texture_offsets[NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT];
    const NDSRendererTileState *tile;
    u32 generation;
    u32 i;
    u32 d;
    u32 attempt;
    s32 resident_ok = FALSE;

    if ((packet_base == NULL) || (stats == NULL) || (hierarchy == NULL) ||
        (drawable_mvps == NULL) ||
        (hierarchy->joint_count != NDS_NATIVE_ACTOR_EF_LAKITU_JOINT_COUNT) ||
        (hierarchy->projection == NULL) ||
        (hierarchy->camera_modelview == NULL) ||
        (hierarchy->joint_locals == NULL) ||
        (hierarchy->joint_parents == NULL) ||
        (hierarchy->joint_bindings == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_NATIVE_ACTOR_EF_LAKITU_JOINT_COUNT; i++)
    {
        if ((hierarchy->joint_parents[i] != sNdsNativeActorEfLakituJointParents[i]) ||
            (hierarchy->joint_bindings[i] != sNdsNativeActorEfLakituBindings[i]) ||
            (hierarchy->joint_locals[i].m[0][3] != 0) ||
            (hierarchy->joint_locals[i].m[1][3] != 0) ||
            (hierarchy->joint_locals[i].m[2][3] != 0) ||
            (hierarchy->joint_locals[i].m[3][3] != 4096))
        {
            return FALSE;
        }
    }
    /* Fail closed before emitting: every run must own its quad, and every
     * epoch (palette + image) must sit inside the asset range. No stale
     * epoch, no fallback texture. */
    for (d = 0u; d < NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT; d++)
    {
        const u16 *run = &sNdsNativeActorEfLakituRuns[d * 3u];
        u32 pal_off = sNdsNativeActorEfLakituTextureEpochs[d * 4u];
        u32 pal_size = sNdsNativeActorEfLakituTextureEpochs[d * 4u + 1u];
        u32 img_off = sNdsNativeActorEfLakituTextureEpochs[d * 4u + 2u];
        u32 img_size = sNdsNativeActorEfLakituTextureEpochs[d * 4u + 3u];

        if ((run[0] != 3u + d) || (run[1] != d * 2u) ||
            (run[2] != NDS_NATIVE_ACTOR_EF_LAKITU_TRIANGLE_COUNT / 3u))
        {
            return FALSE;
        }
        if ((packet_bytes < pal_off + pal_size) ||
            (packet_bytes < img_off + img_size))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_ACTOR_EF_LAKITU_VERT_COUNT; i++)
    {
        const s16 *v = &sNdsNativeActorEfLakituVerts[i * 5u];
        u32 rgba = sNdsNativeActorEfLakituVertColors[i];
        vertices[i] = (NDSRendererInputVertex){ v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba };
    }
    config.initial_projection = hierarchy->projection;
    config.initial_modelview = hierarchy->camera_modelview;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    ndsRendererInitTraversalState(&state, &config, stats, NULL, NULL, 0u);
    ndsNativeActorEfLakituSetupPrelude(stats, asset_base);
    if ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u)
    {
        return FALSE;
    }
    NDS_FIGHTER_PACKET_DMA_WAIT();
    /* Stages 1-3 -- bounded preflight, zero triangles. Run d lives on joint
     * 3+d (e2 under e1 under e0 under A, then e3/e4 under e0); every local
     * is live (A = spawn pose, e0 = sweep, e1 = scale flap, drawables =
     * Tra+kind72 billboard). Attempt 0 is the warm fast path (pure resolve
     * only); a miss drops into the bounded cold-upload pass and a second,
     * eviction-proof pure reverify. Two attempts are exact, not a retry
     * loop: attempt 1 either proves all three resident or fails closed. */
    for (attempt = 0u; attempt < 2u; attempt++)
    {
        resident_ok = TRUE;
        for (d = 0u; d < NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT; d++)
        {
            if (d == 0u)
            {
                ndsNativeActorEfLakituSetupEpoch0(stats, asset_base);
            }
            else if (d == 1u)
            {
                ndsNativeActorEfLakituSetupEpoch1(stats, asset_base);
            }
            else
            {
                ndsNativeActorEfLakituSetupEpoch2(stats, asset_base);
            }
            ndsRendererHardwareEndBatch();
            if (ndsRendererHardwareResolveResidentTexture(stats, &config,
                    &state, &resolved[d]) == FALSE)
            {
                resident_ok = FALSE;
                break;
            }
            if ((resolved[d].entry == NULL) || (resolved[d].name == 0u))
            {
                /* BindTextureName silently elides a zero name; a resolved
                 * hit always carries both, so this guard keeps a hidden
                 * missing draw fail-closed instead of skip-quiet. */
                resident_ok = FALSE;
                break;
            }
            texture_names[d] = resolved[d].name;
            tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
            tile_uls[d] = tile->uls;
            tile_ults[d] = tile->ult;
            scales_s[d] = stats->texture_scale_s;
            scales_t[d] = stats->texture_scale_t;
            texture_offsets[d] =
                ndsRendererHardwareTextureFilterOffset(stats);
            material_colors[d] = ndsRendererHardwareColorSource(stats);
            use_materials[d] = ndsRendererHardwareUseMaterialColor(stats);
            use_vertexes[d] = ndsRendererHardwareUseVertexColor(stats);
            poly_alphas[d] = ndsRendererHardwareAlpha(stats, &vertices[0]);
        }
        if (resident_ok != FALSE)
        {
            break;
        }
        if (attempt != 0u)
        {
            break;
        }
        /* Stage 2 -- cold upload, still zero triangles. One live bind per
         * epoch uploads what stage 1 found missing. A bind failure returns
         * FALSE here, before any triangle batch exists. */
        for (d = 0u; d < NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT; d++)
        {
            if (d == 0u)
            {
                ndsNativeActorEfLakituSetupEpoch0(stats, asset_base);
            }
            else if (d == 1u)
            {
                ndsNativeActorEfLakituSetupEpoch1(stats, asset_base);
            }
            else
            {
                ndsNativeActorEfLakituSetupEpoch2(stats, asset_base);
            }
            ndsRendererHardwareEndBatch();
            if (ndsRendererHardwareBindTexture(stats, &config, &state) ==
                FALSE)
            {
                return FALSE;
            }
        }
    }
    if (resident_ok == FALSE)
    {
        /* Attempt 1's pure reverify missed: stage 2 evicted a sibling (or
         * the source range is short in a way the upload could not fix).
         * Never return TRUE with a partial actor. */
        return FALSE;
    }
    /* Stage 4 -- emission. Every resource is resident; rebind by name the
     * way the native-stage prepared-run commit does. BindTextureName is
     * void, elides only an already-bound name, and never allocates, so
     * this loop has no failure path around the GX calls. */
    for (d = 0u; d < NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT; d++)
    {
        const u16 *run = &sNdsNativeActorEfLakituRuns[d * 3u];
        u32 first;
        u32 count;
        u32 corner;

        if (poly_alphas[d] == 0u)
        {
            continue;
        }
        resolved[d].entry->last_used_frame =
            sNdsRendererHardwareFrameSerial + 1u;
        resolved[d].entry->params = resolved[d].params;
        ndsRendererHardwareEndBatch();
        ndsRendererHardwareBindTextureName(stats, texture_names[d]);
        ndsRendererHardwareApplyTextureParams(resolved[d].params);
        sNdsRendererHardwareActiveTextureEntry = resolved[d].entry;
        if (resolved[d].entry->pinned != 0u)
        {
            ndsRendererHardwareRecordBattleStaticTextureHit(resolved[d].entry);
        }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = resolved[d].format;
        stats->hardware_texture_width = resolved[d].width;
        stats->hardware_texture_height = resolved[d].height;
        generation = ndsRendererNextMatrixGeneration();
        ndsRendererLoadHardwareRawComposedMatrix(&drawable_mvps[d], generation);
        ndsRendererHardwareBeginTriangleBatch(stats, TRUE,
            sNdsRendererHardwareBoundTextureName,
            ndsRendererHardwarePolyFmt(stats, poly_alphas[d]),
            sNdsRendererHardwareMatrixMode, generation);
        first = (u32)run[1] * 3u;
        count = (u32)run[2] * 3u;
        for (corner = first; corner < first + count; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeActorEfLakituTriIndices[corner]];
            v16 x = ndsRendererHardwareVertexCoord(v->x, TRUE);
            v16 y = ndsRendererHardwareVertexCoord(v->y, TRUE);
            v16 z = ndsRendererHardwareVertexCoord(v->z, TRUE);

            glColor(ndsRendererHardwarePackedVertexColor(stats, v,
                material_colors[d], use_materials[d], use_vertexes[d], 0u,
                FALSE, 0u));
            glTexCoord2t16(
                ndsRendererHardwareTexCoord(v->s, scales_s[d],
                    tile_uls[d], texture_offsets[d]),
                ndsRendererHardwareTexCoord(v->t, scales_t[d],
                    tile_ults[d], texture_offsets[d]));
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
    ndsNativeActorEfLakituFinishState(stats, asset_base);
    return TRUE;
}

#endif /* NDS_NATIVE_ACTOR_EF_LAKITU_EXEC_INC */
#endif
