/* The native renderer's common owners: the impact wave and the procedural
 * visual templates (ndsRendererSubmitNativeImpactWave and
 * ndsRendererSubmitNativeVisualEffect, called from the stage DL submit), and
 * the raw fast path's triangle decode and vertex emitters. A slice of
 * nds_renderer.c; its own slices (the rebirth halo through the hierarchy)
 * follow it there. */

#include <nds/nds_preview_pack.h>



static inline u32 ndsRendererNativeRunSubmitClass(const NDSNativeRun *run)
{
    return (u32)run->submit_class & NDS_NATIVE_RUN_SUBMIT_CLASS_MASK;
}

static inline u32 ndsRendererNativeRunUsesVertexColor(const NDSNativeRun *run)
{
    return ((u32)run->submit_class &
            NDS_NATIVE_RUN_FLAG_UNLIT_VERTEX_COLOR) != 0u;
}

static inline u32 ndsRendererNativeRunPolyAlpha(const NDSNativeRun *run)
{
    return (((u32)run->submit_class & NDS_NATIVE_RUN_ALPHA_MASK) >>
            NDS_NATIVE_RUN_ALPHA_SHIFT);
}

static inline s32 ndsRendererFastRawStateEligible(
    const NDSRendererTraversalState *state)
{
    const u32 required_flags =
        NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD |
        NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED;
    const u32 forbidden_flags =
        NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH |
        NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH |
        NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH;

    return ((state != NULL) &&
            (state->texture_prepare_valid != 0u) &&
            (state->texture_prepare_source_zbuffered != 0u) &&
            (state->texture_prepare_decal_depth == 0u) &&
            (state->texture_prepare_prim_depth == 0u) &&
            (state->texture_prepare_alpha_constant != 0u) &&
            (state->texture_prepare_poly_alpha != 0u) &&
            ((state->texture_prepare_vertex_flags & required_flags) ==
             required_flags) &&
            ((state->texture_prepare_vertex_flags & forbidden_flags) == 0u) &&
            (state->matrix_valid != 0u) &&
            (state->matrix_generation != 0u) &&
            (sNdsRendererHardwareMatrixLoaded != 0u) &&
            (sNdsRendererHardwareMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareMatrixGeneration ==
             state->matrix_generation) &&
            (sNdsRendererHardwareTriangleBatchOpen != 0u) &&
            (sNdsRendererHardwareTriangleBatchTextured ==
             state->texture_prepare_enabled) &&
            (sNdsRendererHardwareTriangleBatchTextureName ==
             state->texture_prepare_name) &&
            (sNdsRendererHardwareTriangleBatchPolyFmt ==
             state->texture_prepare_poly_fmt) &&
            (sNdsRendererHardwareTriangleBatchMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
             state->matrix_generation)) ? TRUE : FALSE;
}

static inline s32 ndsRendererFastDecodeTriangle(
    u32 packed, u32 *indices, u32 *required_mask)
{
    u32 i0;
    u32 i1;
    u32 i2;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);
    if ((i0 >= NDS_RENDERER_MAX_VTX) ||
        (i1 >= NDS_RENDERER_MAX_VTX) ||
        (i2 >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    indices[0] = i0;
    indices[1] = i1;
    indices[2] = i2;
    *required_mask |= (1u << i0) | (1u << i1) | (1u << i2);
    return TRUE;
}





static void NDS_RENDERER_FAST_RUN_CODE ndsRendererFastPrepareRawSlots(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 required_mask,
    u32 textured)
{
    u32 missing_color = required_mask &
        ~state->prepared_vertex_color_valid_mask;
    u32 context_flags = state->texture_prepare_vertex_flags;

    while (missing_color != 0u)
    {
        u32 vertex_index = (u32)__builtin_ctz(missing_color);
        u32 vertex_mask = 1u << vertex_index;
        const NDSRendererInputVertex *vtx =
            &state->input_vertices[vertex_index];

        state->prepared_vertex_colors[vertex_index] =
            ndsRendererHardwarePackedVertexColor(
                stats, vtx, state->texture_prepare_material_color,
                context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL,
                context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX,
                state->vertex_colors[vertex_index],
                ((state->vertex_color_valid_mask & vertex_mask) != 0u) ?
                    TRUE : FALSE,
                state->color_modulate);
        state->prepared_vertex_color_valid_mask |= vertex_mask;
        missing_color &= ~vertex_mask;
    }

    if (textured != 0u)
    {
        u32 missing_texcoord = required_mask &
            ~state->prepared_texcoord_valid_mask;

        while (missing_texcoord != 0u)
        {
            u32 vertex_index = (u32)__builtin_ctz(missing_texcoord);
            u32 vertex_mask = 1u << vertex_index;
            const NDSRendererInputVertex *vtx =
                &state->input_vertices[vertex_index];

            state->prepared_texcoord_s[vertex_index] =
                ndsRendererHardwareTexCoord(
                    vtx->s, state->texture_prepare_scale_s,
                    state->texture_prepare_origin_s,
                    state->texture_prepare_offset);
            state->prepared_texcoord_t[vertex_index] =
                ndsRendererHardwareTexCoord(
                    vtx->t, state->texture_prepare_scale_t,
                    state->texture_prepare_origin_t,
                    state->texture_prepare_offset);
            state->prepared_texcoord_valid_mask |= vertex_mask;
            missing_texcoord &= ~vertex_mask;
        }
    }
}



static inline void ndsRendererFastEmitRawUntexturedVertex(
    const NDSRendererTraversalState *state, u32 vertex_index)
{
    const NDSRendererInputVertex *vtx =
        &state->input_vertices[vertex_index];

    glColor(state->prepared_vertex_colors[vertex_index]);
    glVertex3v16(
        (v16)((s32)vtx->x * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->y * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->z * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
}

static inline void ndsRendererFastEmitRawTexturedVertex(
    const NDSRendererTraversalState *state, u32 vertex_index)
{
    const NDSRendererInputVertex *vtx =
        &state->input_vertices[vertex_index];

    glColor(state->prepared_vertex_colors[vertex_index]);
    glTexCoord2t16(state->prepared_texcoord_s[vertex_index],
                  state->prepared_texcoord_t[vertex_index]);
    glVertex3v16(
        (v16)((s32)vtx->x * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->y * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->z * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
}

static inline void ndsRendererFastEmitRawCommand(
    const NDSRendererTraversalState *state,
    const u32 *indices,
    u32 triangle_count,
    u32 textured)
{
    u32 triangle_index;

    if (textured != 0u)
    {
        for (triangle_index = 0u;
             triangle_index < triangle_count;
             triangle_index++)
        {
            const u32 *triangle = &indices[triangle_index * 3u];

            ndsRendererFastEmitRawTexturedVertex(state, triangle[0]);
            ndsRendererFastEmitRawTexturedVertex(state, triangle[1]);
            ndsRendererFastEmitRawTexturedVertex(state, triangle[2]);
        }
    }
    else
    {
        for (triangle_index = 0u;
             triangle_index < triangle_count;
             triangle_index++)
        {
            const u32 *triangle = &indices[triangle_index * 3u];

            ndsRendererFastEmitRawUntexturedVertex(state, triangle[0]);
            ndsRendererFastEmitRawUntexturedVertex(state, triangle[1]);
            ndsRendererFastEmitRawUntexturedVertex(state, triangle[2]);
        }
    }
}


static inline void ndsRendererFastAccountRawTriangles(
    NDSRendererStats *stats,
    u32 triangle_count,
    u32 reuse_count)
{
    sNdsRendererHardwareSubmitClassCounts[
        NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX] += triangle_count;
#if NDS_DIAG_COUNTERS
    sNdsRendererRuntimeFrameSummary.raw_current_candidate_count +=
        triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count += reuse_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count += reuse_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices += triangle_count * 3u;
#else
    (void)reuse_count;
#endif
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    stats->hardware_zbuffer_triangle_count += triangle_count;
    sNdsRendererHardwareSubmitted = TRUE;
}







static inline void ndsRendererNativeSourceBoundary(
    NDSRendererTraversalState *state)
{
    ndsRendererHardwareEndBatch();
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_projected_xy_valid_mask = 0u;
    state->prepared_projected_source_z_valid_mask = 0u;
}

static s32 ndsRendererNativePrepareDirectRun(
    const NDSNativeRun *run,
    u32 first_vertex,
    s32 projected_run,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const NDSRendererInputVertex *v0;
    const NDSRendererTileState *render_tile;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 material_color;
    u32 poly_alpha;
    u32 poly_fmt;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 implicit_texture_on;
    s32 use_texture;
    s32 texture_offset;
    u32 available_mask;

    if ((run == NULL) || (config == NULL) || (stats == NULL) ||
        (state == NULL) || (first_vertex >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u) ||
        ((stats->othermode_l & NDS_RENDERER_ZMODE_DEC) ==
         NDS_RENDERER_ZMODE_DEC) ||
        (ndsRendererHardwareUsePrimDepth(stats) != FALSE) ||
        ((projected_run == FALSE) &&
         (ndsRendererHardwareRawMatrixCompatible(state) == FALSE)))
    {
        return FALSE;
    }
    available_mask = state->input_vertex_valid_mask &
        state->raw_vertex_fit_mask;
    if (projected_run == FALSE)
    {
        available_mask &= state->current_transform_vertex_mask;
    }
    if ((available_mask & run->required_mask) != run->required_mask)
    {
        return FALSE;
    }
    v0 = &state->input_vertices[first_vertex];
    if (projected_run != FALSE)
    {
        u32 missing = run->required_mask;

        while (missing != 0u)
        {
            u32 index = (u32)__builtin_ctz(missing);

            if (ndsRendererEnsureTransformedVertex(
                    stats, state, index) == FALSE)
            {
                return FALSE;
            }
            missing &= ~(1u << index);
        }
    }
    state->texture_prepare_source_zbuffered = TRUE;
    state->texture_prepare_decal_depth = FALSE;
    state->texture_prepare_prim_depth = FALSE;

    if (state->texture_prepare_valid == 0u)
    {
        material_color = ndsRendererHardwareColorSource(stats);
        use_material_color = ndsRendererHardwareUseMaterialColor(stats);
        use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
        state->texture_prepare_material_color = material_color;
        state->texture_prepare_vertex_flags =
            ((use_material_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
            ((use_vertex_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);
    }
    if (state->texture_prepare_valid == 0u)
    {
        poly_alpha = ndsRendererHardwareAlpha(stats, v0);
        state->texture_prepare_alpha_constant =
            (ndsRendererHardwareAlphaUsesVertex(stats) == FALSE) ?
                TRUE : FALSE;
        if ((state->texture_prepare_alpha_constant == 0u) ||
            (poly_alpha == 0u))
        {
            return FALSE;
        }
        state->texture_prepare_poly_alpha = poly_alpha;
        state->texture_prepare_poly_fmt =
            ndsRendererHardwarePolyFmt(stats, poly_alpha);

        render_tile =
            &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
        implicit_texture_on =
            ndsRendererHardwareTextureImplicitStateOn(stats);
        use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ?
            ndsRendererHardwareBindTexture(stats, config, state) : FALSE;
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled =
            (use_texture != FALSE) ? TRUE : FALSE;
        state->texture_prepare_name = (use_texture != FALSE) ?
            sNdsRendererHardwareBoundTextureName : 0u;
        ndsRendererProfileRecordTexturePrepare();

        texture_scale_s = stats->texture_scale_s;
        texture_scale_t = stats->texture_scale_t;
        if ((use_texture != FALSE) && (implicit_texture_on != FALSE))
        {
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
            {
                texture_scale_s = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
            {
                texture_scale_t = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
        }
        texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
        state->texture_prepare_scale_s = texture_scale_s;
        state->texture_prepare_scale_t = texture_scale_t;
        state->texture_prepare_origin_s = render_tile->uls;
        state->texture_prepare_origin_t = render_tile->ult;
        state->texture_prepare_offset = texture_offset;
        if (use_texture != FALSE)
        {
            state->texture_prepare_vertex_flags |=
                NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        }
    }
    else
    {
        use_texture =
            (state->texture_prepare_enabled != 0u) ? TRUE : FALSE;
        ndsRendererProfileRecordTexturePrepareReuse();
    }

    state->texture_prepare_vertex_flags =
        (state->texture_prepare_vertex_flags &
         NDS_RENDERER_VERTEX_CONTEXT_PREPARED_MASK) |
        ((projected_run != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH :
             (NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD |
              NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED));
    ndsRendererFastPrepareRawSlots(
        stats, state, run->required_mask,
        state->texture_prepare_enabled);
    if (projected_run != FALSE)
    {
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
        poly_fmt = state->texture_prepare_poly_fmt;
        ndsRendererHardwareBeginTriangleBatch(
            stats, (use_texture != FALSE) ? TRUE : FALSE,
            state->texture_prepare_name, poly_fmt,
            sNdsRendererHardwareMatrixMode,
            sNdsRendererHardwareMatrixGeneration);
        return TRUE;
    }
    ndsRendererLoadHardwareMatrices(state, TRUE);
    poly_fmt = state->texture_prepare_poly_fmt;
    ndsRendererHardwareBeginTriangleBatch(
        stats, (use_texture != FALSE) ? TRUE : FALSE,
        state->texture_prepare_name, poly_fmt,
        sNdsRendererHardwareMatrixMode,
        sNdsRendererHardwareMatrixGeneration);
    return ndsRendererFastRawStateEligible(state);
}

/* P2-2 placement-only reclaim: this 72-byte root preamble is the companion to
 * the 152-byte production preamble moved out above.  Both execute at root
 * granularity; keeping the per-run prepare/emit loops resident is the higher
 * value use of the fixed 32 KiB ITCM budget. */
static void __attribute__((noinline, optimize("Os")))
ndsRendererNativeApplyRootLightPreamble(
    const NDSNativeRoot *root, NDSRendererStats *stats)
{
    const u32 (*preambles)[2];
    u32 preamble_index;

    if (root->light_preamble == 0u)
    {
        return;
    }
    /* Each source gSPLightColor expands to its A/B G_MW_LIGHTCOL pair. */
    preamble_index = (u32)root->light_preamble;
    preambles = sNdsNativeFighterActiveRootLightPreambles;
    if ((preambles == NULL) ||
        (preamble_index >= sNdsNativeFighterActiveRootLightPreambleCount))
    {
        return;
    }
    stats->light_color_1 = preambles[preamble_index][0];
    stats->light_color_2 = preambles[preamble_index][1];
    stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_1_MASK |
        NDS_RENDERER_LIGHT_COLOR_2_MASK;
    stats->light_color_command_count += 4u;
}

/* P2-2p8 2026-09-27 ITCM swap: out (noinline, so no ITCM caller pulls it
 * back in) for ndsStageGxDraw; census rent under ~1,300 cycles a byte. */
static void __attribute__((noinline))
ndsRendererNativeApplyStateDelta(
    const NDSNativeStateDelta *delta,
    const u8 *asset_base,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    if ((delta == NULL) || (stats == NULL) || (state == NULL))
    {
        return;
    }
    switch (delta->effect)
    {
    case NDS_NATIVE_STATE_OTHERMODE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordOtherMode(
            stats, delta->w0 >> 24, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_COMBINE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordSetCombine(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_TEXTURE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordTextureState(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_GEOMETRY:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->geometry_mode =
            (stats->geometry_mode & delta->w0) | delta->w1;
        stats->geometry_clear_mask = delta->w0;
        stats->geometry_set_mask = delta->w1;
        stats->geometry_command_count++;
        break;
    case NDS_NATIVE_STATE_IMAGE:
    {
        u32 foreign_asset = (u32)delta->reserved[0] |
                            ((u32)delta->reserved[1] << 8);
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        if (foreign_asset != 0u)
        {
            /* A missing source-owned span records a null image, so texture
             * preflight rejects it instead of retaining the preceding image
             * or interpreting the offset in the local model's address space. */
            const void *image = (delta->reserved[2] == 0u) ?
                ndsRelocNativeForeignImageAddress(
                    asset_base, foreign_asset - 1u, delta->w1) : NULL;
            ndsRendererRecordSetImage(stats, delta->w0, (u32)(uintptr_t)image);
            break;
        }
        ndsRendererRecordSetImage(
            stats, delta->w0,
#if NDS_P2_1P_GAME || NDS_P2_MENU_SHELL || NDS_P2_COMPACT_BATTLE_FIGHTERS
            (u32)(uintptr_t)ndsRelocNativeAssetAddress(asset_base, delta->w1));
#else
            (u32)(uintptr_t)(asset_base + delta->w1));
#endif
        break;
    }
    case NDS_NATIVE_STATE_TILE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordSetTile(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_LOAD_TLUT:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordLoadTlut(stats, delta->w1);
        break;
    case NDS_NATIVE_STATE_LOAD_BLOCK:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordLoadBlock(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_TILE_SIZE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordSetTileSize(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_PRIM:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->prim_color = delta->w1;
        stats->prim_min_level = (delta->w0 >> 8) & 0xffu;
        stats->prim_lod_fraction = delta->w0 & 0xffu;
        stats->color_command_count++;
        break;
    /* P2-3f5. G_SETBLENDCOLOR on a FIGHTER root. The native stage program has
     * carried this effect since Task 26 (`NDS_TASK26_BLEND`, and the stage span
     * applier below); no fighter owner produced one until Captain Falcon, whose
     * high-detail root 6 brackets its two draws with G_AC_THRESHOLD and a blend
     * colour. blend_color IS the alpha reference the threshold compares
     * against -- `ndsRendererHardwareApplyAlphaTest` reads exactly these two
     * fields -- so dropping it would silently pick up whatever reference the
     * previous draw left behind. */
    case NDS_NATIVE_STATE_BLEND:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->blend_color = delta->w1;
        stats->color_command_count++;
        break;
    case NDS_NATIVE_STATE_LIGHT_COLOR:
        ndsRendererApplyMatrixMoveWordCommand(
            stats, state, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_LOAD_TILE:
        /* Yoshi's Island loads its textures with G_LOADTILE (five deltas in
         * its packet, 2026-09-07); the record is the DL core's own. */
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordLoadTile(stats, delta->w0, delta->w1);
        break;
    default:
        break;
    }
}


static void
ndsRendererNativeApplyStateSpan(
    u16 first,
    u32 count,
    u32 sync_count,
    const u8 *asset_base,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 i;

    if ((count == 0u) && (sync_count == 0u))
    {
        return;
    }
    ndsRendererNativeSourceBoundary(state);
    stats->sync_command_count += sync_count;
    if ((count == 0u) || (first == NDS_NATIVE_STATE_NONE))
    {
        return;
    }
    for (i = 0u; i < count; i++)
    {
        u32 delta_index =
            sNdsNativeFighterActiveTables->state_sequence[first + i];

        ndsRendererNativeApplyStateDelta(
            &sNdsNativeFighterActiveTables->state_deltas[delta_index],
            asset_base, stats, state);
    }
}

static void ndsRendererNativeApplyMaterial(
    const NDSRendererNativeMaterial *material,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 effects;

    if ((material == NULL) || (stats == NULL) || (state == NULL))
    {
        return;
    }
    ndsRendererNativeSourceBoundary(state);
    effects = material->effects;
    /* Root DE call + generated segment-E table DE jump. The root command is
     * already included in the generated source count; only the table word
     * and typed branch body are additional commands. */
    stats->command_count += (u32)material->command_count + 1u;
    stats->branch_command_count += 2u;
    stats->branch_call_count++;
    stats->branch_jump_count++;
    stats->segment_resolve_count++;
    stats->end_command_count++;
    stats->sync_command_count += material->sync_count;

    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->palette_image_w0, material->palette_image);
        ndsRendererRecordSetImage(
            stats, material->palette_image_w0, material->palette_image);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->palette_tile_w0, material->palette_tile_w1);
        ndsRendererRecordSetTile(
            stats, material->palette_tile_w0, material->palette_tile_w1);
        ndsRendererTextureSourceHashCommand(
            stats, NDS_RENDERER_OP_LOADTLUT << 24,
            material->palette_tlut_w1);
        ndsRendererRecordLoadTlut(stats, material->palette_tlut_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LIGHT1) != 0u)
    {
        ndsRendererRecordLightColor(stats, 1u, material->light1);
        ndsRendererRecordLightColor(stats, 1u, material->light1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LIGHT2) != 0u)
    {
        ndsRendererRecordLightColor(stats, 2u, material->light2);
        ndsRendererRecordLightColor(stats, 2u, material->light2);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_PRIM) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->prim_color = material->prim_w1;
        stats->prim_min_level = (material->prim_w0 >> 8) & 0xffu;
        stats->prim_lod_fraction = material->prim_w0 & 0xffu;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_ENV) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->env_color = material->env_color;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_BLEND) != 0u)
    {
        stats->blend_color = material->blend_color;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->block_image_w0, material->block_image);
        ndsRendererRecordSetImage(
            stats, material->block_image_w0, material->block_image);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LOAD_BLOCK) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->load_block_w0, material->load_block_w1);
        ndsRendererRecordLoadBlock(
            stats, material->load_block_w0, material->load_block_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->current_image_w0, material->current_image);
        ndsRendererRecordSetImage(
            stats, material->current_image_w0, material->current_image);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->render_tile_size_w0,
            material->render_tile_size_w1);
        ndsRendererRecordSetTileSize(
            stats, material->render_tile_size_w0,
            material->render_tile_size_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_SCROLL_TILE_SIZE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->scroll_tile_size_w0,
            material->scroll_tile_size_w1);
        ndsRendererRecordSetTileSize(
            stats, material->scroll_tile_size_w0,
            material->scroll_tile_size_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_TEXTURE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->texture_w0, material->texture_w1);
        ndsRendererRecordTextureState(
            stats, material->texture_w0, material->texture_w1);
    }
}

/* The hierarchy candidate replays the complete retained owner into private
 * scratch before its first GX command.  The shared state/material helpers only
 * touch GX through their source-boundary batch close, so mask the batch-open
 * bit around those exact helpers and restore it immediately.  Existing mode-8
 * execution keeps calling the original helpers byte-for-byte. */
static void ndsRendererNativeApplyStateSpanPreflight(
    u16 first,
    u32 count,
    u32 sync_count,
    const u8 *asset_base,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 batch_open = sNdsRendererHardwareTriangleBatchOpen;

    sNdsRendererHardwareTriangleBatchOpen = FALSE;
    ndsRendererNativeApplyStateSpan(
        first, count, sync_count, asset_base, stats, state);
    sNdsRendererHardwareTriangleBatchOpen = batch_open;
}

static void ndsRendererNativeApplyMaterialPreflight(
    const NDSRendererNativeMaterial *material,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 batch_open = sNdsRendererHardwareTriangleBatchOpen;

    sNdsRendererHardwareTriangleBatchOpen = FALSE;
    ndsRendererNativeApplyMaterial(material, stats, state);
    sNdsRendererHardwareTriangleBatchOpen = batch_open;
}

static s32 ndsRendererHardwareBindImpactWaveTexture(
    NDSRendererStats *stats, const NDSRendererTileState *render_tile,
    u32 variant)
{
    u32 name;
    u32 params;

    if ((stats == NULL) || (render_tile == NULL) ||
        (variant >= NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT))
    {
        return FALSE;
    }
    name = sNdsRendererImpactWaveTextureName[variant];
    if (name == 0u)
    {
        return FALSE;
    }

    params = ndsRendererHardwareTextureParams(
        stats, render_tile, NDS_RENDERER_IMPACT_WAVE_TEX_WIDTH,
        NDS_RENDERER_IMPACT_WAVE_TEX_HEIGHT);
    ndsRendererHardwareBindTextureName(stats, name);
    ndsRendererHardwareApplyTextureParams(
        ndsRendererHardwareMergeTextureParams(params));
    /* This owner never occupies a generic cache entry. The texel index stream
     * and all five colour palettes are ROM constants uploaded at scene entry. */
    sNdsRendererHardwareActiveTextureEntry = NULL;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
    stats->hardware_texture_width = NDS_RENDERER_IMPACT_WAVE_TEX_WIDTH;
    stats->hardware_texture_height = NDS_RENDERER_IMPACT_WAVE_TEX_HEIGHT;
    NDS_DIAG(gNdsImpactWaveNativeTextureBindCount++);
    return TRUE;
}

s32 ndsRendererSubmitNativeImpactWave(
    const NDSRendererInputVertex *vertices, u32 vertex_count,
    const u8 *triangle_indices, u32 triangle_count,
    const Gfx *source_setup,
    const NDSRendererNativeMaterial *material,
    u32 variant,
    const NDSRendererConfig *config,
    NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    NDSRendererTraversalVertexStorage vertex_storage;
    NDSRendererTraversalState state;
    const NDSRendererTileState *render_tile;
    u32 required_mask;
    u32 material_color;
    u32 poly_alpha;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 use_material_color;
    u32 use_vertex_color;
    u32 implicit_texture_on;
    u32 use_texture;
    s32 texture_offset;
    u32 matrix_generation;
    u32 i;

    /* This is deliberately a closed owner, not a general mesh API. Keeping the
     * exact source cardinalities here means a bad AOT table falls back to the
     * interpreter rather than emitting a partly-valid cosmetic. */
    if ((vertices == NULL) || (vertex_count != 18u) ||
        (triangle_indices == NULL) || (triangle_count != 16u) ||
        (source_setup == NULL) || (material == NULL) ||
        (variant >= NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT) ||
        (sNdsRendererImpactWaveTextureName[variant] == 0u) ||
        (config == NULL) || (stats == NULL))
    {
        return FALSE;
    }
    required_mask = (1u << vertex_count) - 1u;
    for (i = 0u; i < (triangle_count * 3u); i++)
    {
        if ((u32)triangle_indices[i] >= vertex_count)
        {
            return FALSE;
        }
    }

    ndsRendererInitTraversalState(
        &state, config, stats, &vertex_storage, NULL, 0u);
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        return FALSE;
    }

    /* Only the source vertices are kept: GX performs world/view/projection
     * for the fixed body. The traversal still backs input/color/UV state. */
    for (i = 0u; i < vertex_count; i++)
    {
        u32 mask = 1u << i;

        state.input_vertices[i] = vertices[i];
        state.input_vertex_valid_mask |= mask;
        ndsRendererProfileRecordSourceVertexLoad();
    }
    if (stats->vertex_count < vertex_count)
    {
        stats->vertex_count = vertex_count;
    }

    /* The camera-loop interception may fall back to the source proc only
     * before a GX write, so the hardware side-effect boundary waits for the
     * admitted resident texture bind below. */
    if (stats->first_opcode == 0u)
    {
        stats->first_opcode = source_setup[0].words.w0 >> 24;
    }
    stats->command_count += 35u;
    stats->sync_command_count += 8u;

#define NDS_IMPACT_HASH(index_) \
    ndsRendererTextureSourceHashCommand( \
        stats, source_setup[(index_)].words.w0, source_setup[(index_)].words.w1)
#define NDS_IMPACT_INVALIDATE() NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(&state)

    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(1u);
    ndsRendererRecordOtherMode(
        stats, source_setup[1].words.w0 >> 24,
        source_setup[1].words.w0, source_setup[1].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(2u);
    ndsRendererRecordOtherMode(
        stats, source_setup[2].words.w0 >> 24,
        source_setup[2].words.w0, source_setup[2].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(3u);
    ndsRendererRecordSetCombine(
        stats, source_setup[3].words.w0, source_setup[3].words.w1);
    stats->blend_color = source_setup[4].words.w1;
    stats->color_command_count++;
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(6u);
    ndsRendererRecordSetTile(
        stats, source_setup[6].words.w0, source_setup[6].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(7u);
    ndsRendererRecordSetTile(
        stats, source_setup[7].words.w0, source_setup[7].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(8u);
    ndsRendererRecordSetTileSize(
        stats, source_setup[8].words.w0, source_setup[8].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(9u);
    ndsRendererRecordSetImage(
        stats, source_setup[9].words.w0, source_setup[9].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(11u);
    ndsRendererRecordLoadTlut(stats, source_setup[11].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(13u);
    ndsRendererRecordSetImage(
        stats, source_setup[13].words.w0, source_setup[13].words.w1);

    ndsRendererNativeApplyMaterialPreflight(material, stats, &state);

    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(16u);
    ndsRendererRecordLoadBlock(
        stats, source_setup[16].words.w0, source_setup[16].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(18u);
    stats->geometry_mode =
        (stats->geometry_mode & source_setup[18].words.w0) |
        source_setup[18].words.w1;
    stats->geometry_clear_mask = source_setup[18].words.w0;
    stats->geometry_set_mask = source_setup[18].words.w1;
    stats->geometry_command_count++;

    /* VTX command 19 observes lighting already cleared, so its shade result is
     * simply the source's white RGBA. Compute through the shared helper anyway
     * rather than baking a renderer-specific packed-colour convention. */
    for (i = 0u; i < vertex_count; i++)
    {
        state.vertex_colors[i] = ndsRendererHardwareLitShadeColorPrepared(
            stats, &state.input_vertices[i], NULL);
    }
    state.vertex_color_valid_mask = required_mask;

    material_color = ndsRendererHardwareColorSource(stats);
    use_material_color =
        (ndsRendererHardwareUseMaterialColor(stats) != FALSE) ? TRUE : FALSE;
    use_vertex_color =
        (ndsRendererHardwareUseVertexColor(stats) != FALSE) ? TRUE : FALSE;
    state.texture_prepare_material_color = material_color;
    state.texture_prepare_vertex_flags =
        ((use_material_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
        ((use_vertex_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);

    poly_alpha = ndsRendererHardwareAlpha(stats, &state.input_vertices[0]);
    state.texture_prepare_alpha_constant =
        (ndsRendererHardwareAlphaUsesVertex(stats) == FALSE) ? TRUE : FALSE;
    state.texture_prepare_poly_alpha = poly_alpha;
    state.texture_prepare_poly_fmt = ndsRendererHardwarePolyFmt(stats, poly_alpha);
    render_tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    implicit_texture_on =
        ndsRendererHardwareTextureImplicitStateOn(stats) ? TRUE : FALSE;
    /* The source state above remains the oracle for UV/filter/alpha semantics,
     * but PIXELS are no longer resolved from it. DL_0x7C28's 16x32 sampled CI4 indices and
     * PRIM/ENV colour combine were compiled AOT into DS PAL16. A valid native
     * ImpactWave therefore reaches a resident-name bind here and never enters
     * ndsRendererHardwareBindTexture / the generic N64 conversion cache. */
    use_texture =
        (ndsRendererHardwareUseTexture(stats) != FALSE) ? TRUE : FALSE;
    if ((use_texture == FALSE) ||
        (ndsRendererHardwareBindImpactWaveTexture(
             stats, render_tile, variant) == FALSE))
    {
        return FALSE;
    }
    state.texture_prepare_valid = TRUE;
    state.texture_prepare_enabled = use_texture;
    state.texture_prepare_name =
        (use_texture != FALSE) ? sNdsRendererHardwareBoundTextureName : 0u;
    ndsRendererProfileRecordTexturePrepare();

    texture_scale_s = stats->texture_scale_s;
    texture_scale_t = stats->texture_scale_t;
    if ((use_texture != FALSE) && (implicit_texture_on != FALSE))
    {
        if ((stats->texture_state_flags &
             NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
        {
            texture_scale_s = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
        }
        if ((stats->texture_state_flags &
             NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
        {
            texture_scale_t = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
        }
    }
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    state.texture_prepare_scale_s = texture_scale_s;
    state.texture_prepare_scale_t = texture_scale_t;
    state.texture_prepare_origin_s = render_tile->uls;
    state.texture_prepare_origin_t = render_tile->ult;
    state.texture_prepare_offset = texture_offset;
    if (use_texture != FALSE)
    {
        state.texture_prepare_vertex_flags |=
            NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
    }
    /* Raw-slot preparation supplies colour and UV only. GX receives source
     * positions below and performs the world/view/projection work. */
    ndsRendererFastPrepareRawSlots(
        stats, &state, required_mask, use_texture);

    if (poly_alpha != 0u)
    {
        matrix_generation = ndsRendererNextMatrixGeneration();
        ndsRendererLoadHardwareSplitMatrices(
            config->initial_projection, config->initial_modelview,
            matrix_generation);
        ndsRendererHardwareBeginTriangleBatch(
            stats, use_texture, state.texture_prepare_name,
            state.texture_prepare_poly_fmt,
            sNdsRendererHardwareMatrixMode, matrix_generation);

        for (i = 0u; i < triangle_count; i++)
        {
            const u8 *tri = &triangle_indices[i * 3u];
            u32 corner;

            for (corner = 0u; corner < 3u; corner++)
            {
                u32 index = (u32)tri[corner];
                const NDSRendererInputVertex *v = &vertices[index];
                v16 out_x = ndsRendererHardwareVertexCoord(v->x, TRUE);
                v16 out_y = ndsRendererHardwareVertexCoord(v->y, TRUE);
                v16 out_z = ndsRendererHardwareVertexCoord(v->z, TRUE);

                glColor(state.prepared_vertex_colors[index]);
                if (use_texture != FALSE)
                {
                    glTexCoord2t16(state.prepared_texcoord_s[index],
                                  state.prepared_texcoord_t[index]);
                }
                ndsRendererProfileHWVertexRange(out_x, out_y, out_z);
                glVertex3v16(out_x, out_y, out_z);
            }
            sNdsRendererHardwareSubmitted = TRUE;
            stats->triangle_count++;
            stats->hardware_triangle_count++;
            stats->hardware_vertex_count += 3u;
            stats->hardware_zbuffer_triangle_count++;
            ndsRendererProfileRecordHardwareTriangle();
        }
    }
    ndsRendererHardwareEndBatch();

    /* Source commands 28..34 restore the sticky state inherited by the next
     * effect list. Preserve them even though they draw no geometry. */
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(30u);
    stats->geometry_mode =
        (stats->geometry_mode & source_setup[30].words.w0) |
        source_setup[30].words.w1;
    stats->geometry_clear_mask = source_setup[30].words.w0;
    stats->geometry_set_mask = source_setup[30].words.w1;
    stats->geometry_command_count++;
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(31u);
    ndsRendererRecordOtherMode(
        stats, source_setup[31].words.w0 >> 24,
        source_setup[31].words.w0, source_setup[31].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(32u);
    ndsRendererRecordOtherMode(
        stats, source_setup[32].words.w0 >> 24,
        source_setup[32].words.w0, source_setup[32].words.w1);
    NDS_IMPACT_INVALIDATE();
    NDS_IMPACT_HASH(33u);
    ndsRendererRecordOtherMode(
        stats, source_setup[33].words.w0 >> 24,
        source_setup[33].words.w0, source_setup[33].words.w1);
    stats->end_command_count++;

#undef NDS_IMPACT_INVALIDATE
#undef NDS_IMPACT_HASH
    return TRUE;
}

/* ------------------------------------------------------------------------
 * THE PROCEDURAL VISUAL TEMPLATES, AS BAKED GEOMETRY.
 *
 * Three measured failure rows -- Dream Land 46/1,200, Hyrule Castle 54/1,200
 * (gkind 0x4), Congo Jungle 56/1,200 (gkind 0x2), and Mushroom Kingdom
 * 161/1,200 -- all read domain 2 STAGE, reason 1 NO_PROGRAM, GObj 0x3f3
 * Effect, asset 0xffff, root a RAM address. They are one class, not four bugs:
 * battleship_efmanager.c:593 mallocs seven templates out of the taskman arena
 * and :849 hands that RAM Gfx list to a DObj, so ndsRelocFindLoadedFileContaining
 * answers NULL and the recorder correctly publishes 0xffff and the raw pointer.
 * There was simply no owner.
 *
 * scripts/3d_vfx/check_visual_effect_templates.py recomputes these tables from
 * the builder arithmetic in battleship_efmanager.c and diffs this file; it is
 * the bake's oracle and must be re-run after any edit here. Nothing else
 * generates them, because there is no upstream asset -- this art is defined
 * in C.
 *
 * NO DISPLAY LIST IS READ AT ANY STEP. The executor takes a template index.
 * The arena tokens the templates still carry exist only so the DObj has a `dl`
 * for identity comparison and so ndsFighterDLScanRangeInTaskmanArena still
 * recognises the pointer -- moving them to const ROM would make
 * renderer_adapter_stage.c record BAD_ASSET before any owner arm runs. */
typedef struct NDSVisualEffectGroup
{
    u8 first_vertex;
    u8 vertex_count;
    u8 first_index;
    u8 triangle_count;
} NDSVisualEffectGroup;

static const NDSRendererInputVertex sNdsVisualEffectVertices[75] = {
    /* [0] Dust -- 7 vertices */
    {     0,     0,     0, 0, 0, 221, 208, 176, 255 },
    {  -170,   -25,     0, 0, 0, 128, 108,  84, 255 },
    {   -90,    55,     0, 0, 0, 128, 108,  84, 255 },
    {     0,    80,     0, 0, 0, 128, 108,  84, 255 },
    {    90,    55,     0, 0, 0, 128, 108,  84, 255 },
    {   170,   -25,     0, 0, 0, 128, 108,  84, 255 },
    {     0,   -65,     0, 0, 0, 128, 108,  84, 255 },
    /* [1] Normal -- 9 vertices */
    {     0,     0,     0, 0, 0, 255, 255, 255, 255 },
    {     0,   180,     0, 0, 0, 255, 208,  64, 255 },
    {    42,    42,     0, 0, 0, 255, 208,  64, 255 },
    {   180,     0,     0, 0, 0, 255, 208,  64, 255 },
    {    42,   -42,     0, 0, 0, 255, 208,  64, 255 },
    {     0,  -180,     0, 0, 0, 255, 208,  64, 255 },
    {   -42,   -42,     0, 0, 0, 255, 208,  64, 255 },
    {  -180,     0,     0, 0, 0, 255, 208,  64, 255 },
    {   -42,    42,     0, 0, 0, 255, 208,  64, 255 },
    /* [2] Fire -- 9 vertices */
    {     0,     0,     0, 0, 0, 255, 255, 144, 255 },
    {     0,   180,     0, 0, 0, 255,  74,  16, 255 },
    {    42,    42,     0, 0, 0, 255,  74,  16, 255 },
    {   180,     0,     0, 0, 0, 255,  74,  16, 255 },
    {    42,   -42,     0, 0, 0, 255,  74,  16, 255 },
    {     0,  -180,     0, 0, 0, 255,  74,  16, 255 },
    {   -42,   -42,     0, 0, 0, 255,  74,  16, 255 },
    {  -180,     0,     0, 0, 0, 255,  74,  16, 255 },
    {   -42,    42,     0, 0, 0, 255,  74,  16, 255 },
    /* [3] Electric -- 9 vertices */
    {     0,     0,     0, 0, 0, 255, 255, 255, 255 },
    {     0,   180,     0, 0, 0,  48, 144, 255, 255 },
    {    42,    42,     0, 0, 0,  48, 144, 255, 255 },
    {   180,     0,     0, 0, 0,  48, 144, 255, 255 },
    {    42,   -42,     0, 0, 0,  48, 144, 255, 255 },
    {     0,  -180,     0, 0, 0,  48, 144, 255, 255 },
    {   -42,   -42,     0, 0, 0,  48, 144, 255, 255 },
    {  -180,     0,     0, 0, 0,  48, 144, 255, 255 },
    {   -42,    42,     0, 0, 0,  48, 144, 255, 255 },
    /* [4] Sparkle -- 9 vertices */
    {     0,     0,     0, 0, 0, 255, 255, 255, 255 },
    {     0,   180,     0, 0, 0, 144, 232, 255, 255 },
    {    42,    42,     0, 0, 0, 144, 232, 255, 255 },
    {   180,     0,     0, 0, 0, 144, 232, 255, 255 },
    {    42,   -42,     0, 0, 0, 144, 232, 255, 255 },
    {     0,  -180,     0, 0, 0, 144, 232, 255, 255 },
    {   -42,   -42,     0, 0, 0, 144, 232, 255, 255 },
    {  -180,     0,     0, 0, 0, 144, 232, 255, 255 },
    {   -42,    42,     0, 0, 0, 144, 232, 255, 255 },
    /* [5] Wave -- 16 vertices, outer/inner interleaved */
    {     0,   180,     0, 0, 0,  96, 255, 128, 255 },
    {     0,   105,     0, 0, 0, 255, 255, 128, 255 },
    {   127,   127,     0, 0, 0,  96, 255, 128, 255 },
    {    74,    74,     0, 0, 0, 255, 255, 128, 255 },
    {   180,     0,     0, 0, 0,  96, 255, 128, 255 },
    {   105,     0,     0, 0, 0, 255, 255, 128, 255 },
    {   127,  -127,     0, 0, 0,  96, 255, 128, 255 },
    {    74,   -74,     0, 0, 0, 255, 255, 128, 255 },
    {     0,  -180,     0, 0, 0,  96, 255, 128, 255 },
    {     0,  -105,     0, 0, 0, 255, 255, 128, 255 },
    {  -127,  -127,     0, 0, 0,  96, 255, 128, 255 },
    {   -74,   -74,     0, 0, 0, 255, 255, 128, 255 },
    {  -180,     0,     0, 0, 0,  96, 255, 128, 255 },
    {  -105,     0,     0, 0, 0, 255, 255, 128, 255 },
    {  -127,   127,     0, 0, 0,  96, 255, 128, 255 },
    {   -74,    74,     0, 0, 0, 255, 255, 128, 255 },
    /* [6] Death -- 16 vertices, outer/inner interleaved */
    {     0,   180,     0, 0, 0, 255,  64,  96, 255 },
    {     0,   105,     0, 0, 0, 255, 255, 255, 255 },
    {   127,   127,     0, 0, 0, 255,  64,  96, 255 },
    {    74,    74,     0, 0, 0, 255, 255, 255, 255 },
    {   180,     0,     0, 0, 0, 255,  64,  96, 255 },
    {   105,     0,     0, 0, 0, 255, 255, 255, 255 },
    {   127,  -127,     0, 0, 0, 255,  64,  96, 255 },
    {    74,   -74,     0, 0, 0, 255, 255, 255, 255 },
    {     0,  -180,     0, 0, 0, 255,  64,  96, 255 },
    {     0,  -105,     0, 0, 0, 255, 255, 255, 255 },
    {  -127,  -127,     0, 0, 0, 255,  64,  96, 255 },
    {   -74,   -74,     0, 0, 0, 255, 255, 255, 255 },
    {  -180,     0,     0, 0, 0, 255,  64,  96, 255 },
    {  -105,     0,     0, 0, 0, 255, 255, 255, 255 },
    {  -127,   127,     0, 0, 0, 255,  64,  96, 255 },
    {   -74,    74,     0, 0, 0, 255, 255, 255, 255 },
};

/* Indices are LOCAL to their group, so the 32-slot traversal scratch and its
 * 32-bit valid mask bound the widest template (16) with room to spare. */
static const u8 sNdsVisualEffectTriangles[210] = {
    /* Dust -- 6 triangles */
     0, 1, 2,  0, 2, 3,  0, 3, 4,  0, 4, 5,
     0, 5, 6,  0, 6, 1,
    /* Normal -- 8 triangles */
     0, 1, 2,  0, 2, 3,  0, 3, 4,  0, 4, 5,
     0, 5, 6,  0, 6, 7,  0, 7, 8,  0, 8, 1,
    /* Fire -- 8 triangles */
     0, 1, 2,  0, 2, 3,  0, 3, 4,  0, 4, 5,
     0, 5, 6,  0, 6, 7,  0, 7, 8,  0, 8, 1,
    /* Electric -- 8 triangles */
     0, 1, 2,  0, 2, 3,  0, 3, 4,  0, 4, 5,
     0, 5, 6,  0, 6, 7,  0, 7, 8,  0, 8, 1,
    /* Sparkle -- 8 triangles */
     0, 1, 2,  0, 2, 3,  0, 3, 4,  0, 4, 5,
     0, 5, 6,  0, 6, 7,  0, 7, 8,  0, 8, 1,
    /* Wave -- 16 triangles */
     0, 2, 1,  1, 2, 3,  2, 4, 3,  3, 4, 5,
     4, 6, 5,  5, 6, 7,  6, 8, 7,  7, 8, 9,
     8,10, 9,  9,10,11, 10,12,11, 11,12,13,
    12,14,13, 13,14,15, 14, 0,15, 15, 0, 1,
    /* Death -- 16 triangles */
     0, 2, 1,  1, 2, 3,  2, 4, 3,  3, 4, 5,
     4, 6, 5,  5, 6, 7,  6, 8, 7,  7, 8, 9,
     8,10, 9,  9,10,11, 10,12,11, 11,12,13,
    12,14,13, 13,14,15, 14, 0,15, 15, 0, 1,
};

static const NDSVisualEffectGroup
sNdsVisualEffectGroups[NDS_VISUAL_EFFECT_TEMPLATE_COUNT] = {
    {  0,  7,   0,  6 }, /* Dust */
    {  7,  9,  18,  8 }, /* Normal */
    { 16,  9,  42,  8 }, /* Fire */
    { 25,  9,  66,  8 }, /* Electric */
    { 34,  9,  90,  8 }, /* Sparkle */
    { 43, 16, 114, 16 }, /* Wave */
    { 59, 16, 162, 16 }, /* Death */
};

/* G_CC_SHADE / G_CC_SHADE, and it is WRITTEN rather than inherited on purpose.
 * None of the seven source lists carries a G_SETCOMBINE at all
 * (battleship_efmanager.c:470-481 emits only geometry mode, texture-off and
 * G_VTX), so under the generic path the drawn colour of a hit spark was
 * whatever combine the previous list left sticky -- a different answer per
 * stage draw order. That is an unexplained state difference, so the owner
 * pins it. */
#define NDS_VISUAL_EFFECT_COMBINE_W0 0xfcffffffu
#define NDS_VISUAL_EFFECT_COMBINE_W1 0xfffe793cu

s32 ndsRendererSubmitNativeVisualEffect(
    u32 template_index,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    NDSRendererTraversalVertexStorage vertex_storage;
    NDSRendererTraversalState state;
    const NDSVisualEffectGroup *group;
    const NDSRendererInputVertex *vertices;
    const u8 *indices;
    u32 vertex_count;
    u32 triangle_count;
    u32 required_mask;
    u32 inside_mask;
    u32 poly_alpha;
    v16 projected_x[16];
    v16 projected_y[16];
    u32 i;

    /* A closed owner, like ImpactWave's: a bad table declines loudly rather
     * than emitting a partly-valid cosmetic. */
    if ((config == NULL) || (stats == NULL) ||
        (template_index >= NDS_VISUAL_EFFECT_TEMPLATE_COUNT))
    {
        return FALSE;
    }
    group = &sNdsVisualEffectGroups[template_index];
    vertex_count = (u32)group->vertex_count;
    triangle_count = (u32)group->triangle_count;
    if ((vertex_count == 0u) || (vertex_count > 16u) ||
        (triangle_count == 0u) ||
        (((u32)group->first_vertex + vertex_count) >
         (sizeof(sNdsVisualEffectVertices) /
          sizeof(sNdsVisualEffectVertices[0]))) ||
        (((u32)group->first_index + (triangle_count * 3u)) >
         sizeof(sNdsVisualEffectTriangles)))
    {
        return FALSE;
    }
    vertices = &sNdsVisualEffectVertices[group->first_vertex];
    indices = &sNdsVisualEffectTriangles[group->first_index];
    for (i = 0u; i < (triangle_count * 3u); i++)
    {
        if ((u32)indices[i] >= vertex_count)
        {
            return FALSE;
        }
    }

    ndsRendererInitTraversalState(
        &state, config, stats, &vertex_storage, NULL, 0u);
    if (state.matrix_valid == 0u)
    {
        return FALSE;
    }

    /* Transform once. A corner across the near plane is clipped below as the
     * RSP clips it: the 1P camera can sit inside a dust puff (Fox's Sector Z
     * walk, 2026-10-04: 12 frames of REJECTED_PROGRAM when this owner still
     * declined). Only the inside corners are divided. */
    required_mask = (1u << vertex_count) - 1u;
    inside_mask = 0u;
    for (i = 0u; i < vertex_count; i++)
    {
        u32 mask = 1u << i;
        NDSRendererClipVertex20p12 *out = &state.vertices[i];

        state.input_vertices[i] = vertices[i];
        state.input_vertex_valid_mask |= mask;
        state.current_transform_vertex_mask |= mask;
        ndsRendererTransformVertex20p12(&state.matrix, &vertices[i], out);
        if (ndsRendererHardwareClipZWInsideNearPlane(out->z, out->w) != FALSE)
        {
            /* One perspective divide per UNIQUE vertex. The star fan reuses
             * its centre eight times and the ring reuses every corner three
             * times, so per-corner division would cost 24 and 48 divides
             * against 9 and 16. */
            projected_x[i] = ndsRendererHardwareProjectToV16(
                (s64)out->x * NDS_RENDERER_HW_PROJECTED_VERTEX, out->w);
            projected_y[i] = ndsRendererHardwareProjectToV16(
                (s64)out->y * NDS_RENDERER_HW_PROJECTED_VERTEX, out->w);
            inside_mask |= mask;
        }
        state.vertex_valid_mask |= mask;
        stats->matrix_transform_count++;
        stats->transformed_vertex_count++;
        if (stats->transformed_vertex_count == 1u)
        {
            stats->first_transformed_x = out->x;
            stats->first_transformed_y = out->y;
            stats->first_transformed_z = out->z;
            stats->first_transformed_w = out->w;
        }
        ndsRendererProfileRecordCPUTransform();
        ndsRendererProfileRecordSourceVertexLoad();
    }
    if (stats->vertex_count < vertex_count)
    {
        stats->vertex_count = vertex_count;
    }

    ndsRendererHardwareEndBatch();
    NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(&state);

    /* The template's own state, written not scanned. Geometry mode is exactly
     * G_SHADE (battleship_efmanager.c:473 clears every bit then sets it), so
     * LIGHTING is off and ndsRendererHardwareLitShadeColorPrepared returns each
     * baked RGBA verbatim; G_TEXTURE is off (:475, w1 == 0). */
    stats->geometry_mode = NDS_RENDERER_GEOM_SHADE;
    stats->geometry_clear_mask = 0x00ffffffu;
    stats->geometry_set_mask = NDS_RENDERER_GEOM_SHADE;
    stats->geometry_command_count++;
    ndsRendererRecordTextureState(stats, 0xd7000000u, 0x00000000u);
    ndsRendererRecordSetCombine(stats, NDS_VISUAL_EFFECT_COMBINE_W0,
                                NDS_VISUAL_EFFECT_COMBINE_W1);
    stats->vertex_command_count++;
    stats->triangle_command_count += (triangle_count + 1u) / 2u;
    stats->command_count += 3u + ((triangle_count + 1u) / 2u) + 1u;
    stats->end_command_count++;

    for (i = 0u; i < vertex_count; i++)
    {
        state.vertex_colors[i] = ndsRendererHardwareLitShadeColorPrepared(
            stats, &state.input_vertices[i], NULL);
    }
    state.vertex_color_valid_mask = required_mask;
    state.texture_prepare_material_color = 0u;
    state.texture_prepare_vertex_flags = NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX;

    poly_alpha = ndsRendererHardwareAlpha(stats, &state.input_vertices[0]);
    if (poly_alpha == 0u)
    {
        /* Unreachable by construction -- every baked vertex carries a == 255
         * and the pinned combine takes alpha from SHADE -- but a zero here
         * would be an invisible draw, and an invisible draw must be a recorded
         * failure rather than a successful empty one. */
        return FALSE;
    }
    state.texture_prepare_valid = TRUE;
    state.texture_prepare_enabled = FALSE;
    state.texture_prepare_name = 0u;
    state.texture_prepare_alpha_constant = TRUE;
    state.texture_prepare_poly_alpha = poly_alpha;
    state.texture_prepare_poly_fmt =
        ndsRendererHardwarePolyFmt(stats, poly_alpha);

    /* Non-Z geometry: none of the seven lists sets G_ZBUFFER, so the port's
     * per-triangle painter depth stays the depth policy, exactly as ImpactWave
     * documents for the same class. */
    ndsRendererFastPrepareRawSlots(stats, &state, required_mask, FALSE);

    ndsRendererLoadHardwareMatrices(NULL, FALSE);
    ndsRendererHardwareBeginTriangleBatch(
        stats, FALSE, 0u, state.texture_prepare_poly_fmt,
        sNdsRendererHardwareMatrixMode,
        sNdsRendererHardwareMatrixGeneration);

    for (i = 0u; i < triangle_count; i++)
    {
        const u8 *tri = &indices[i * 3u];
        s32 depth = ndsRendererHardwareNextProjectedDepth();
        u32 corner;
        u32 tri_mask = (1u << tri[0]) | (1u << tri[1]) | (1u << tri[2]);

        if ((inside_mask & tri_mask) != tri_mask)
        {
            /* The fighter path's clipper: the front part fans at this
             * triangle's painter depth, which its NDC-depth emitter writes
             * exactly as the loop below does. */
            u32 fanned = ndsRendererHardwareSubmitNearClippedTriangle(
                stats, &state, (u32)tri[0], (u32)tri[1], (u32)tri[2], depth);

            NDS_DIAG(gNdsVisualEffectNearClipCount++);
            if (fanned != 0u)
            {
                sNdsRendererHardwareSubmitted = TRUE;
            }
            stats->triangle_count++;
            stats->transformed_triangle_count++;
            stats->hardware_triangle_count += fanned;
            stats->hardware_vertex_count += fanned * 3u;
            stats->hardware_projected_depth_triangle_count += fanned;
            continue;
        }
        for (corner = 0u; corner < 3u; corner++)
        {
            u32 index = (u32)tri[corner];
            v16 out_z = ndsRendererHardwareClampS64ToV16(depth);

            glColor(state.prepared_vertex_colors[index]);
            ndsRendererProfileHWVertexRange(
                projected_x[index], projected_y[index], out_z);
            glVertex3v16(projected_x[index], projected_y[index], out_z);
        }
        sNdsRendererHardwareSubmitted = TRUE;
        stats->triangle_count++;
        stats->transformed_triangle_count++;
        stats->hardware_triangle_count++;
        stats->hardware_vertex_count += 3u;
        stats->hardware_projected_depth_triangle_count++;
        ndsRendererProfileRecordProjectedSubmit();
        ndsRendererProfileRecordHardwareTriangle();
    }
    ndsRendererHardwareEndBatch();
    return TRUE;
}
