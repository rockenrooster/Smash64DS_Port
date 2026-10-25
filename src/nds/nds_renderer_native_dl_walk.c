/* Native renderer, the display-list walk: source commands, vertex blocks and
 * triangles, the fighters' dense normals, the production preamble, root binding
 * and owner preflight. A slice of nds_renderer.c. */

static s32 ndsRendererNativeVisitSourceCommand(
    const u8 *root_base,
    u32 command_index,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const Gfx *dl;
    NDSRendererCommand command;

    if (callback == NULL)
    {
        return TRUE;
    }
    dl = (const Gfx *)(root_base + (command_index * sizeof(*dl)));
    memset(&command, 0, sizeof(command));
    command.dl = dl;
    command.w0 = dl->words.w0;
    command.w1 = dl->words.w1;
    command.op = command.w0 >> 24;
    command.list_index = command_index;
    command.transformed_vertices = state->vertices;
    command.transformed_vertex_valid_mask = state->vertex_valid_mask;
    command.matrix_valid = state->matrix_valid;
    if (callback(&command, callback_user) == FALSE)
    {
        ndsRendererRecordUnsupported(stats, command.op);
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return FALSE;
    }
    return TRUE;
}

static void ndsRendererNativeLoadVertexBlock(
    const u8 *src,
    u32 v0,
    u32 count,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const NDSRendererHardwareLightDirection *prepared_light_direction = NULL;
    const u32 *prepared_light_shade_lut = NULL;
    u32 matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    u32 i;

    if ((v0 + count) > stats->vertex_count)
    {
        stats->vertex_count = v0 + count;
    }
    if (state->matrix_valid != 0u)
    {
        matrix_snapshot = ndsRendererAcquireCurrentMatrixSnapshot(state);
    }
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        if (state->prepared_light_direction_valid == 0u)
        {
            ndsRendererHardwarePrepareLitDirection(
                stats,
                (state->modelview_valid != 0u) ? &state->modelview : NULL,
                &state->prepared_light_direction);
            state->prepared_light_direction_valid = TRUE;
        }
        prepared_light_direction = &state->prepared_light_direction;
        if ((stats->light_color_mask &
             (NDS_RENDERER_LIGHT_COLOR_1_MASK |
              NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
            (NDS_RENDERER_LIGHT_COLOR_1_MASK |
             NDS_RENDERER_LIGHT_COLOR_2_MASK))
        {
            prepared_light_shade_lut = ndsRendererHardwareGetLightShadeLut(
                stats->light_color_1, stats->light_color_2);
        }
    }
    for (i = 0u; i < count; i++)
    {
        u32 index = v0 + i;
        u32 mask = 1u << index;
        NDSRendererInputVertex *input = &state->input_vertices[index];

        ndsRendererDecodeInputVertex(input, src + (i * 16u));
        state->input_vertex_valid_mask |= mask;
        if (ndsRendererHardwareRawVertexFits(input) != FALSE)
        {
            state->raw_vertex_fit_mask |= mask;
        }
        else
        {
            state->raw_vertex_fit_mask &= ~mask;
        }
        if (prepared_light_shade_lut != NULL)
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorLut(
                    input, prepared_light_direction,
                    prepared_light_shade_lut);
        }
        else
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorPrepared(
                    stats, input, prepared_light_direction);
        }
        state->vertex_color_valid_mask |= mask;
        state->vertex_matrix_snapshot[index] = (u8)matrix_snapshot;
        state->vertex_clip_snapshot[index] =
            NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
        state->vertex_valid_mask &= ~mask;
        state->current_transform_vertex_mask &= ~mask;
        if (state->matrix_valid != 0u)
        {
            state->current_transform_vertex_mask |= mask;
        }
        if ((state->matrix_valid != 0u) &&
            (matrix_snapshot == NDS_RENDERER_MATRIX_SNAPSHOT_INVALID))
        {
            (void)ndsRendererTransformCachedVertex(
                stats, state, index, &state->matrix, matrix_snapshot);
        }
    }
    sNdsRendererHardwareSourceVertexLoadCount += count;
}

static void ndsRendererNativeApplyVertexActions(
    const NDSNativeEpoch *epoch,
    const u8 *asset_base,
    const u8 *root_base,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 i;

    if (epoch->action_count == 0u)
    {
        return;
    }
    ndsRendererNativeSourceBoundary(state);
    for (i = 0u; i < epoch->action_count; i++)
    {
        const NDSNativeVertexAction *action =
            &sNdsNativeFighterHighTables.vertex_actions[epoch->first_action + i];

        if (ndsRendererNativeVisitSourceCommand(
                root_base, action->command_index, callback, callback_user,
                stats, state) == FALSE)
        {
            return;
        }
        if (action->kind == NDS_NATIVE_VERTEX_BLOCK)
        {
            ndsRendererNativeLoadVertexBlock(
                asset_base + action->source_offset,
                action->index, action->count, stats, state);
        }
        else if ((action->kind == NDS_NATIVE_MODIFY_ST) &&
                 (action->index < NDS_RENDERER_MAX_VTX) &&
                 ((state->input_vertex_valid_mask &
                   (1u << action->index)) != 0u))
        {
            state->input_vertices[action->index].s = action->s;
            state->input_vertices[action->index].t = action->t;
        }
    }
}

static void ndsRendererNativeSubmitGenericTriangle(
    u32 packed,
    u32 command_index,
    u32 tri2_half,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    (void)command_index;
    (void)tri2_half;
    if (sNdsRendererHardwareNoOracle == 0u)
    {
        ndsRendererRecordTransformedTriangle(
            stats, state, packed);
    }
    NDS_EFFECT_PHASE_TRI(ndsRendererSubmitHardwareTriangle(
        stats, config, state, packed));
}

static inline u32 ndsRendererNativeDecodeTriangle(
    u16 encoded,
    u32 indices[3])
{
    u32 compact = (u32)encoded & 0x7fffu;

    indices[0] = (compact >> 10) & 31u;
    indices[1] = (compact >> 5) & 31u;
    indices[2] = compact & 31u;
    return (indices[0] << 17) |
           (indices[1] << 9) |
           (indices[2] << 1);
}

static u32 ndsRendererNativeNormalizeGeometryMode(u32 mode)
{
    u32 source_cull = mode &
        (NDS_NATIVE_SOURCE_GEOM_CULL_FRONT |
         NDS_NATIVE_SOURCE_GEOM_CULL_BACK);

    if ((source_cull & NDS_NATIVE_SOURCE_GEOM_CULL_FRONT) != 0u)
    {
        mode |= NDS_RENDERER_GEOM_CULL_FRONT;
    }
    if ((source_cull & NDS_NATIVE_SOURCE_GEOM_CULL_BACK) != 0u)
    {
        mode |= NDS_RENDERER_GEOM_CULL_BACK;
    }
    return mode &
        ~(NDS_NATIVE_SOURCE_GEOM_CULL_FRONT |
          NDS_NATIVE_SOURCE_GEOM_CULL_BACK);
}

/* Every run-level production reject passes through here. The adapter turns
 * the owner's FALSE into a REJECTED_PROGRAM failure with no root and no
 * material, so this call is the only attribution left. It is recorded as the
 * helper's return address rather than a per-site line constant: the callers
 * live in the four-CPU build's full ITCM, where twelve inlined copies of a
 * literal-pool constant and a call overflowed the region by 312 B, while one
 * `bl` into this main-RAM helper is no larger than the store it replaced.
 * -O2 cross-jumps every reject exit of a function onto one `bl`, so the
 * address names the rejecting FUNCTION (resolve `site - 1` with addr2line on
 * the matching ELF) and the recorded state words name the predicate. The
 * record is a first-since-boot latch shared with the hierarchy preflight's
 * decline path; a count above one therefore means the first entry may be a
 * benign hierarchy decline rather than the fatal production reject. */
static s32 __attribute__((noinline))
ndsRendererNativeDirectReject(NDSRendererStats *stats)
{
    ndsRendererRecordNativeDirectReject(
        (u32)__builtin_return_address(0), stats);
    if (stats != NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
    }
    return FALSE;
}


/* Captain is the first production fighter whose source roots use
 * G_AC_THRESHOLD. Keep the DS register writes out of the packed ITCM submit
 * body: they occur only when a batch begins, not per emitted vertex, and the
 * hot region has no spare capacity for libnds' inlined glEnable/glAlphaFunc
 * sequence. Semantics are the same mapping used by the generic renderer. */
static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererNativeApplyProductionAlphaTest(const NDSRendererStats *stats)
{
    if ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) ==
        NDS_RENDERER_ALPHA_COMPARE_THRESHOLD)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc((stats->blend_color & 0xffu) >> 4);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }
}

static inline void ndsRendererNativeBeginDirectBatch(
    const NDSRendererStats *stats,
    u32 textured,
    u32 texture_name,
    u32 poly_fmt,
    u32 matrix_generation)
{
    u32 alpha_key =
        (stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) |
        ((stats->blend_color & 0xffu) << 8);
    if ((sNdsRendererHardwareTriangleBatchOpen != 0u) &&
        (sNdsRendererHardwareTriangleBatchTextured == textured) &&
        (sNdsRendererHardwareTriangleBatchTextureName == texture_name) &&
        (sNdsRendererHardwareTriangleBatchPolyFmt == poly_fmt) &&
        (sNdsRendererHardwareTriangleBatchAlphaKey == alpha_key) &&
        (sNdsRendererHardwareTriangleBatchMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
        (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
         matrix_generation))
    {
        ndsRendererProfileRecordBatchReuse();
        return;
    }

    ndsRendererHardwareEndBatch();
    glEnable(GL_TEXTURE_2D);
    if (textured == 0u)
    {
        ndsRendererHardwareBindNoTexture(NULL);
    }
    /* BattleShip's Captain model contains real G_AC_THRESHOLD epochs. The
     * generic DS renderer maps that source state to the hardware alpha test,
     * with G_SETBLENDCOLOR's low byte as the threshold. Production used to
     * reject the epoch and fall back after GX had already started. Keep the
     * same mapping here so the source run stays on the native owner. */
    ndsRendererNativeApplyProductionAlphaTest(stats);
    glDisable(GL_FOG);
    ndsRendererHardwareSetPolyFmt(poly_fmt);
    glBegin(GL_TRIANGLE);
    ndsRendererProfileRecordBatchBegin();

    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = textured;
    sNdsRendererHardwareTriangleBatchTextureName = texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = alpha_key;
    sNdsRendererHardwareTriangleBatchFogKey = 0u;
    sNdsRendererHardwareTriangleBatchMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED;
    sNdsRendererHardwareTriangleBatchMatrixGeneration = matrix_generation;
}

/* GX-compose's owner-approved shipping arm once re-admitted this 152-byte
 * root-level helper after the 2026-08-17 re-knapsack. P2-2's source-required
 * High/Low owner selection needs a little of that space back. This is the
 * lowest-value retained admission with a recorded price (1,123 I-cache-fill
 * tk/fr at rank-80, 7.4/byte), and moving it changes placement only: the exact
 * same helper still runs once per root from main RAM. Keep the per-run prepare,
 * shade and emit loops in zero-wait ITCM instead. */
static void ndsRendererNativeApplyProductionPreamble(
    const NDSRendererNativeFighterPreamble *preamble,
    NDSRendererStats *stats)
{
    if ((preamble == NULL) || (stats == NULL) ||
        ((preamble->flags & NDS_RENDERER_NATIVE_PREAMBLE_VALID) == 0u))
    {
        return;
    }

    stats->geometry_mode = ndsRendererNativeNormalizeGeometryMode(
        preamble->geometry_mode);
    stats->othermode_h =
        (stats->othermode_h & ~NDS_RENDERER_CYCLETYPE_MASK) |
        (preamble->cycle_type & NDS_RENDERER_CYCLETYPE_MASK);
    stats->othermode_l = preamble->render_mode;
    stats->prim_color = preamble->prim_color;
    stats->env_color = preamble->env_color;

    if ((preamble->flags &
         NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID) != 0u)
    {
        stats->light_dir_x = preamble->light_dir_x;
        stats->light_dir_y = preamble->light_dir_y;
        stats->light_dir_z = preamble->light_dir_z;
        stats->light_dir_mask = NDS_RENDERER_LIGHT_DIR_1_MASK;
    }
}

static void ndsRendererNativeBindProductionRoot(
    NDSRendererTraversalState *state,
    const NDSRendererNativeFighterRoot *input,
    NDSRendererStats *stats)
{
    ndsRendererNativeApplyProductionPreamble(input->preamble, stats);
    state->modelview_stack_depth = 0u;
    state->vertex_valid_mask = 0u;
    state->input_vertex_valid_mask = 0u;
    state->vertex_color_valid_mask = 0u;
    state->current_transform_vertex_mask = 0u;
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_projected_xy_valid_mask = 0u;
    state->prepared_projected_source_z_valid_mask = 0u;
    state->prepared_light_direction_valid = 0u;
    state->texture_prepare_valid = 0u;
    state->projection_valid = 0u;
    /* E16b traced every consumer of the composed matrix and found only the
     * hardware load, which the split loader replaces. Finishing that trace for
     * the two halves: on this path the split loader takes them from `input`
     * directly (its call site is the next statement after this one, with
     * `input` in scope), and the only other readers of the traversal state's
     * modelview are the software light preparation -- which the fighter path
     * never runs -- plus the generic command executor's
     * matrix stack and ComposeMatrix, none of which the production owner
     * enters. So these were two 64-byte struct copies per root with nothing
     * downstream to read them: 12,422 executions at 416 cycles a call, 5,958
     * ticks/frame, 79% of BindProductionRoot's whole cost.
     *
     * The valid flags go FALSE rather than TRUE because that is now the honest
     * answer: a reader that appears later gets "no matrix here" instead of
     * whichever owner's matrix was copied in last. `matrix_valid` below is a
     * different flag and is still what PrepareProductionRun tests. */
    /* Ordinary GX-compose owners leave the finished modelview only on GX. A
     * declined owner has the adapter's exact CPU fallback, while Link carries
     * the same compose as an explicit CPU mirror solely because source
     * G_TEXTURE_GEN consumes the finished current modelview. */
    if ((input->gx_valid == 0u) ||
        (input->gx_modelview_mirror_valid != 0u))
    {
        state->modelview = *input->modelview_matrix;
        state->modelview_valid = TRUE;
    }
    else
    {
        state->modelview_valid = FALSE;
    }
    state->projection_valid = FALSE;
    state->matrix_valid = TRUE;
    state->matrix_word_valid = FALSE;
    state->matrix_generation = ndsRendererNextMatrixGeneration();
    stats->hardware_matrix_seed_count++;
}

/* One execute owns this renderer state at a time.  Resolve BattleShip's live
 * model-part DL selection in the cold preflight, then let the ITCM executor
 * consume a pointer table instead of repeating owner/variant dispatch inside
 * every root iteration.  The preflight is already the fail-closed boundary:
 * the executor is entered only after all entries below are non-NULL and their
 * epochs/materials validate. */
static const NDSNativeRoot *
    sNdsNativeProductionResolvedRoots[NDS_NATIVE_FIGHTER_ROOT_MAX];
static const NDSNativeFighterRuntimeTables *
    sNdsNativeProductionResolvedTables[NDS_NATIVE_FIGHTER_ROOT_MAX];
static const u32 (*sNdsNativeProductionResolvedLightPreambles[
    NDS_NATIVE_FIGHTER_ROOT_MAX])[2];
static u32 sNdsNativeProductionResolvedLightPreambleCounts[
    NDS_NATIVE_FIGHTER_ROOT_MAX];

static s32 ndsRendererNativePreflightProductionOwner(
    u32 slot,
    u32 battle_slot,
    u32 use_low_detail,
    const void *asset_base,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count,
    NDSRendererCommandCallback callback,
    NDSRendererStats *stats)
{
    u32 root_count;
    u32 root_index;
    if ((slot >= NDS_NATIVE_FIGHTER_OWNER_COUNT) ||
        (asset_base == NULL) || (inputs == NULL) ||
        (stats == NULL) || (callback != NULL) ||
        (sNdsNativeFighterOwnerExecution.active != 0u))
    {
        return FALSE;
    }
    root_count = sNdsNativeFighterActiveOwner->root_count;
    if ((input_count != root_count) ||
        (root_count > NDS_NATIVE_FIGHTER_ROOT_MAX))
    {
        return FALSE;
    }
    for (root_index = 0u; root_index < root_count; root_index++)
    {
        const NDSRendererNativeFighterRoot *input = &inputs[root_index];
        const u8 *root_asset_base = (input->asset_base != NULL) ?
            (const u8 *)input->asset_base : (const u8 *)asset_base;
        const NDSNativeRoot *root = ndsRendererNativeFighterResolveRoot(
            sNdsNativeFighterActiveOwner, slot, battle_slot, use_low_detail,
            root_index, input->root_offset);
        const NDSNativeFighterRuntimeTables *tables;
        const u32 (*light_preambles)[2];
        u32 light_preamble_count;
        u32 epoch_index;

        if ((root == NULL) || (root_asset_base == NULL) ||
            (input->projection_matrix == NULL) ||
            (input->modelview_matrix == NULL) ||
            (input->config == NULL) ||
            (input->preamble == NULL) ||
            ((input->preamble->flags &
              NDS_RENDERER_NATIVE_PREAMBLE_VALID) == 0u))
        {
            return FALSE;
        }
        tables = ndsRendererNativeFighterTablesForResolvedRoot(
            root, sNdsNativeFighterActiveOwner, battle_slot, root_index);
        light_preambles = ndsRendererNativeFighterLightPreamblesForResolvedRoot(
            root, sNdsNativeFighterActiveOwner, battle_slot, root_index,
            &light_preamble_count);
        if ((tables == NULL) ||
            ((u32)root->light_preamble >= light_preamble_count))
        {
            return FALSE;
        }
        sNdsNativeProductionResolvedRoots[root_index] = root;
        sNdsNativeProductionResolvedTables[root_index] = tables;
        sNdsNativeProductionResolvedLightPreambles[root_index] = light_preambles;
        sNdsNativeProductionResolvedLightPreambleCounts[root_index] =
            light_preamble_count;
        for (epoch_index = 0u;
             epoch_index < root->epoch_count;
             epoch_index++)
        {
            const NDSNativeEpoch *epoch =
                &tables->epochs[
                    root->first_epoch + epoch_index];

            if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
                ((input->materials == NULL) ||
                 (epoch->material_slot >= input->material_count)))
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}



/* R2-03 E16. The fighter's per-vertex shade evaluates
 *
 *     colour = light_color_2 + (light_color_1 * dot(N, L)) / 127
 *
 * which is term for term what the DS geometry engine computes in hardware from
 * GFX_NORMAL, and E14 measured that engine idle on every one of 946 fighter
 * submissions. E1 refuted memoising the result because the light is transformed
 * into each animating joint's local space every frame -- the exact problem the
 * hardware solves for free.
 *
 * The mapping is exact rather than approximate. With the DS's row-vector
 * convention a normal submitted under vector matrix V is dotted as N.V.L_stored,
 * and the software computes N^T.M.L; with V = M (which E17's split load already
 * establishes, mode 2 writing position *and* vector) and the light written while
 * the vector matrix is identity, the two are the same product. So light 0's
 * colour is white and the source's two light colours become the *material*
 * diffuse and ambient, folded once per epoch with the material colour and the
 * damage-flash modulate -- 46.4 register writes a frame instead of 484 vertex
 * shades.
 *
 * E16a measured the terms that decide the placement: the light direction changes
 * 0 times in 22,296 epochs, so GFX_LIGHT_VECTOR is written once per owner
 * execute while the vector matrix is identity; the colours move on 32% of epochs
 * and the material on 72%, so GFX_DIFFUSE_AMBIENT is per epoch. */
/* R2-03 E29. Beside sNdsNativeFighterPreparedDense in DTCM: the emit reads one
 * word from each per corner, 1,878 times a frame, in the same random order, so
 * the two tables thrash the same 4 KB data cache against each other. Built once
 * at load by ARM9 code and read only by ARM9 code -- never a DMA endpoint, never
 * touched by the ARM7 or IPC, per the check-dtcm-layout.ps1 gate.
 * P8 R7: with the "mariofox" owner image the words arrive baked in it, as
 * every imaged owner's do, and neither table nor its bake exists. */
#if !NDS_NATIVE_OWNER_IMAGE_MARIOFOX
static u32 sNdsNativeFighterDenseNormals[
    sizeof(sNdsNativeFighterDenseVertices) /
        sizeof(sNdsNativeFighterDenseVertices[0])]
    __attribute__((section(".dtcm.fighter")));
/* The low-detail prepared table is intentionally main-RAM because the DTCM
 * budget cannot hold both detail sets.  Keep its equally cold one-time normal
 * bake beside it in main RAM too; the hot emit sees only the selected pointer. */
static u32 sNdsNativeFighterDenseNormalsLow[
    sizeof(sNdsNativeFighterDenseVerticesLow) /
        sizeof(sNdsNativeFighterDenseVerticesLow[0])];
#endif
#if NDS_P2_LUIGI
#if !NDS_NATIVE_OWNER_IMAGE_LUIGI
/* P2-3: Luigi owns independent generated geometry, so its one-time normal
 * bake cannot alias Mario/Fox's dense-ID namespace. Keep both detail tables in
 * cached main RAM; the P2-2 ITCM/DTCM pack has only 96 B of DTCM slack. */
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeLuigiFighterDenseNormalsBake[
    NDS_NATIVE_IMAGE_LUIGI_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeLuigiFighterDenseNormalsBakeLow[
    NDS_NATIVE_IMAGE_LUIGI_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_DONKEY
#if !NDS_NATIVE_OWNER_IMAGE_DONKEY
/* Donkey is another independent dense-ID namespace.  As with Luigi, these are
 * one-time CPU-built/read-only tables in cached main RAM, never DMA endpoints;
 * do not consume the already-packed P2-2 DTCM budget to admit a fighter. */
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeDonkeyFighterDenseNormalsBake[
    NDS_NATIVE_IMAGE_DONKEY_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeDonkeyFighterDenseNormalsBakeLow[
    NDS_NATIVE_IMAGE_DONKEY_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_CAPTAIN
#if !NDS_NATIVE_OWNER_IMAGE_CAPTAIN
/* Every P2-3 owner has an independent dense-ID namespace. Falcon used to
 * alias Mario/Fox's normal cache because his first owner landing never added
 * this pair; that makes the cache's contents depend on which kind drew first.
 * Keep the one-time table owner-local just like the geometry it indexes. */
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeCaptainFighterDenseNormalsBake[
    NDS_NATIVE_IMAGE_CAPTAIN_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeCaptainFighterDenseNormalsBakeLow[
    NDS_NATIVE_IMAGE_CAPTAIN_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_LINK
#if !NDS_NATIVE_OWNER_IMAGE_LINK
/* P2-3f49: the shipping build reads these words from the NitroFS owner
 * image instead; the arrays survive only for the VERIFY proof build. */
static u32 sNdsNativeLinkFighterDenseNormalsBake[
    NDS_NATIVE_IMAGE_LINK_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeLinkFighterDenseNormalsBakeLow[
    NDS_NATIVE_IMAGE_LINK_LOW_DENSE_VERTICES_COUNT];
#endif
#endif
#if NDS_P2_MMARIO
static u32 sNdsNativeMMarioFighterDenseNormals[
    NDS_NATIVE_IMAGE_MMARIO_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeMMarioFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_MMARIO_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NMARIO
static u32 sNdsNativeNMarioFighterDenseNormals[
    NDS_NATIVE_IMAGE_NMARIO_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNMarioFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NMARIO_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NFOX
static u32 sNdsNativeNFoxFighterDenseNormals[
    NDS_NATIVE_IMAGE_NFOX_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNFoxFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NFOX_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NDONKEY
static u32 sNdsNativeNDonkeyFighterDenseNormals[
    NDS_NATIVE_IMAGE_NDONKEY_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNDonkeyFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NDONKEY_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NSAMUS
static u32 sNdsNativeNSamusFighterDenseNormals[
    NDS_NATIVE_IMAGE_NSAMUS_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNSamusFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NSAMUS_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NLINK
static u32 sNdsNativeNLinkFighterDenseNormals[
    NDS_NATIVE_IMAGE_NLINK_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNLinkFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NLINK_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NYOSHI
static u32 sNdsNativeNYoshiFighterDenseNormals[
    NDS_NATIVE_IMAGE_NYOSHI_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNYoshiFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NYOSHI_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NCAPTAIN
static u32 sNdsNativeNCaptainFighterDenseNormals[
    NDS_NATIVE_IMAGE_NCAPTAIN_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNCaptainFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NCAPTAIN_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NKIRBY
static u32 sNdsNativeNKirbyFighterDenseNormals[
    NDS_NATIVE_IMAGE_NKIRBY_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNKirbyFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NKIRBY_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NPIKACHU
static u32 sNdsNativeNPikachuFighterDenseNormals[
    NDS_NATIVE_IMAGE_NPIKACHU_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNPikachuFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NPIKACHU_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NPURIN
static u32 sNdsNativeNPurinFighterDenseNormals[
    NDS_NATIVE_IMAGE_NPURIN_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNPurinFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NPURIN_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_NNESS
static u32 sNdsNativeNNessFighterDenseNormals[
    NDS_NATIVE_IMAGE_NNESS_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeNNessFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_NNESS_LOW_DENSE_VERTICES_COUNT];
#endif
#if NDS_P2_1P_GAME
static u32 sNdsNativeBossFighterDenseNormals[
    NDS_NATIVE_IMAGE_BOSS_HIGH_DENSE_VERTICES_COUNT];
static u32 sNdsNativeBossFighterDenseNormalsLow[
    NDS_NATIVE_IMAGE_BOSS_LOW_DENSE_VERTICES_COUNT];
#endif
#if !NDS_NATIVE_OWNER_IMAGE_MARIOFOX
static u8 sNdsNativeFighterDenseNormalsBuilt;
static u8 sNdsNativeFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_LUIGI
#if !NDS_NATIVE_OWNER_IMAGE_LUIGI
static u8 sNdsNativeLuigiFighterDenseNormalsBuilt;
static u8 sNdsNativeLuigiFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_DONKEY
#if !NDS_NATIVE_OWNER_IMAGE_DONKEY
static u8 sNdsNativeDonkeyFighterDenseNormalsBuilt;
static u8 sNdsNativeDonkeyFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_CAPTAIN
#if !NDS_NATIVE_OWNER_IMAGE_CAPTAIN
static u8 sNdsNativeCaptainFighterDenseNormalsBuilt;
static u8 sNdsNativeCaptainFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_LINK
#if !NDS_NATIVE_OWNER_IMAGE_LINK
static u8 sNdsNativeLinkFighterDenseNormalsBuilt;
static u8 sNdsNativeLinkFighterDenseNormalsBuiltLow;
#endif
#endif
#if NDS_P2_MMARIO
static u8 sNdsNativeMMarioFighterDenseNormalsBuilt;
static u8 sNdsNativeMMarioFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NMARIO
static u8 sNdsNativeNMarioFighterDenseNormalsBuilt;
static u8 sNdsNativeNMarioFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NFOX
static u8 sNdsNativeNFoxFighterDenseNormalsBuilt;
static u8 sNdsNativeNFoxFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NDONKEY
static u8 sNdsNativeNDonkeyFighterDenseNormalsBuilt;
static u8 sNdsNativeNDonkeyFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NSAMUS
static u8 sNdsNativeNSamusFighterDenseNormalsBuilt;
static u8 sNdsNativeNSamusFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NLINK
static u8 sNdsNativeNLinkFighterDenseNormalsBuilt;
static u8 sNdsNativeNLinkFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NYOSHI
static u8 sNdsNativeNYoshiFighterDenseNormalsBuilt;
static u8 sNdsNativeNYoshiFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NCAPTAIN
static u8 sNdsNativeNCaptainFighterDenseNormalsBuilt;
static u8 sNdsNativeNCaptainFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NKIRBY
static u8 sNdsNativeNKirbyFighterDenseNormalsBuilt;
static u8 sNdsNativeNKirbyFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NPIKACHU
static u8 sNdsNativeNPikachuFighterDenseNormalsBuilt;
static u8 sNdsNativeNPikachuFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NPURIN
static u8 sNdsNativeNPurinFighterDenseNormalsBuilt;
static u8 sNdsNativeNPurinFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_NNESS
static u8 sNdsNativeNNessFighterDenseNormalsBuilt;
static u8 sNdsNativeNNessFighterDenseNormalsBuiltLow;
#endif
#if NDS_P2_1P_GAME
#if !NDS_NATIVE_OWNER_IMAGE_BOSS
static u8 sNdsNativeBossFighterDenseNormalsBuilt;
static u8 sNdsNativeBossFighterDenseNormalsBuiltLow;
#endif
#endif
/* P2-3f49: imaged owners never bake at runtime -- their words arrive inside
 * the bound NitroFS image -- so their Built pointer aims here, permanently
 * set, and the production bake call skips them. Without the "mariofox" image
 * Mario/Fox still bake, into the pair above. */
static u8 sNdsNativeImageDenseNormalsReady = 1u;
#if NDS_NATIVE_OWNER_IMAGE_MARIOFOX
static u32 *sNdsNativeFighterActiveDenseNormals;
static u8 *sNdsNativeFighterActiveDenseNormalsBuilt =
    &sNdsNativeImageDenseNormalsReady;
#else
static u32 *sNdsNativeFighterActiveDenseNormals =
    sNdsNativeFighterDenseNormals;
static u8 *sNdsNativeFighterActiveDenseNormalsBuilt =
    &sNdsNativeFighterDenseNormalsBuilt;
#endif

/* Selection is one owner-level operation, never an inner-loop operation.  Keep
 * it out of `.itcm.native_fighter`: inlining the table/normal pointer fan-out
 * into the production owner wastes scarce ITCM on a once-per-draw branch. */
static s32 __attribute__((noinline, optimize("Os"),
                          section(".text.native_fighter_select")))
ndsRendererNativeSelectFighterRuntimeTables(u32 slot, u32 use_low_detail)
{
    const NDSNativeFighterOwnerRuntime *owner =
        ndsRendererNativeFighterOwnerForDetail(slot, use_low_detail);

    if (owner == NULL)
    {
        return FALSE;
    }
    sNdsNativeFighterActiveOwner = owner;
    sNdsNativeFighterActiveTables = owner->tables;
    sNdsNativeFighterActiveRootLightPreambles = owner->root_light_preambles;
    sNdsNativeFighterActiveRootLightPreambleCount =
        owner->root_light_preamble_count;
    if (ndsRendererNativeFighterRootProgram(slot) == NDS_NATIVE_SKELETON_PROGRAM)
    {
        sNdsNativeFighterActiveDenseNormals = (u32 *)owner->tables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt = &sNdsNativeImageDenseNormalsReady;
        return owner->tables->dense_normals != NULL;
    }
#if NDS_P2_LUIGI
    if (slot == 2u)
    {
#if NDS_NATIVE_OWNER_IMAGE_LUIGI
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeLuigiFighterDenseNormalsBakeLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeLuigiFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeLuigiFighterDenseNormalsBake;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeLuigiFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_DONKEY
    if (slot == 3u)
    {
#if NDS_NATIVE_OWNER_IMAGE_DONKEY
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeDonkeyFighterDenseNormalsBakeLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeDonkeyFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeDonkeyFighterDenseNormalsBake;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeDonkeyFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_CAPTAIN
    if (slot == 4u)
    {
#if NDS_NATIVE_OWNER_IMAGE_CAPTAIN
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeCaptainFighterDenseNormalsBakeLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeCaptainFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeCaptainFighterDenseNormalsBake;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeCaptainFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
    if (slot == 5u)
    {
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
        return TRUE;
    }
    if (slot == 11u)
    {
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
        return TRUE;
    }
#if NDS_P2_MMARIO
    if (slot == 12u)
    {
#if NDS_NATIVE_OWNER_IMAGE_MMARIO
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeMMarioFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeMMarioFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeMMarioFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeMMarioFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NMARIO
    if (slot == 13u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NMARIO
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNMarioFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNMarioFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNMarioFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNMarioFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NFOX
    if (slot == 14u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NFOX
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNFoxFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNFoxFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNFoxFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNFoxFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
    if ((slot > NDS_P4_RENDERER_OWNER_BASE) &&
        (slot <= NDS_P4_RENDERER_OWNER_SLOT(NDS_P4_CONTENT_MAX_ID)))
    {
        /* P4 donor owners are image-backed: the bake reads and writes the
         * image's own normals. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
        return TRUE;
    }
#if NDS_P2_NDONKEY
    if (slot == 15u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NDONKEY
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNDonkeyFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNDonkeyFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNDonkeyFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNDonkeyFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NSAMUS
    if (slot == 16u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NSAMUS
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNSamusFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNSamusFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNSamusFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNSamusFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NLINK
    if (slot == 17u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NLINK
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNLinkFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNLinkFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNLinkFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNLinkFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NYOSHI
    if (slot == 18u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NYOSHI
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNYoshiFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNYoshiFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNYoshiFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNYoshiFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NCAPTAIN
    if (slot == 19u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NCAPTAIN
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNCaptainFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNCaptainFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNCaptainFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNCaptainFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NKIRBY
    if (slot == 20u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NKIRBY
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNKirbyFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNKirbyFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNKirbyFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNKirbyFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NPIKACHU
    if (slot == 21u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NPIKACHU
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNPikachuFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNPikachuFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNPikachuFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNPikachuFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NPURIN
    if (slot == 22u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NPURIN
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNPurinFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNPurinFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNPurinFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNPurinFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_NNESS
    if (slot == 23u)
    {
#if NDS_NATIVE_OWNER_IMAGE_NNESS
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNNessFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNNessFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeNNessFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeNNessFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_P2_1P_GAME
    if (slot == 24u)
    {
#if NDS_NATIVE_OWNER_IMAGE_BOSS
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeBossFighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeBossFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeBossFighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeBossFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
    if (slot == 10u)
    {
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
        return TRUE;
    }
    if (slot == 9u)
    {
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
        return TRUE;
    }
    if (slot == 8u)
    {
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
        return TRUE;
    }
    if (slot == 7u)
    {
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
        return TRUE;
    }
#if NDS_P2_LINK
    if (slot == 6u)
    {
#if NDS_NATIVE_OWNER_IMAGE_LINK
        /* P2-3f49: precomputed words from the bound image; see the ready flag. */
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeLinkFighterDenseNormalsBakeLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeLinkFighterDenseNormalsBuiltLow;
        }
        else
        {
            sNdsNativeFighterActiveDenseNormals =
                sNdsNativeLinkFighterDenseNormalsBake;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNativeLinkFighterDenseNormalsBuilt;
        }
#endif
        return TRUE;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_MARIOFOX
    /* Mario/Fox (P8 R7): precomputed words from the bound "mariofox" image;
     * unbound (no ensure), the owner declines rather than draw. */
    sNdsNativeFighterActiveDenseNormals =
        (u32 *)sNdsNativeFighterActiveTables->dense_normals;
    sNdsNativeFighterActiveDenseNormalsBuilt = &sNdsNativeImageDenseNormalsReady;
    return sNdsNativeFighterActiveTables->dense_normals != NULL;
#else
    if (use_low_detail != 0u)
    {
        sNdsNativeFighterActiveDenseNormals = sNdsNativeFighterDenseNormalsLow;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeFighterDenseNormalsBuiltLow;
    }
    else
    {
        sNdsNativeFighterActiveDenseNormals = sNdsNativeFighterDenseNormals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeFighterDenseNormalsBuilt;
    }
    return TRUE;
#endif
}
