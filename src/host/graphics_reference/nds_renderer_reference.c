/* Host reference only; include after the shared renderer units in a host oracle. */
#if defined(ARM9) || defined(ARM7) || defined(__NDS__)
#error Reference graphics implementations cannot be compiled into a Nintendo DS ROM
#endif
#include "nds_renderer_reference.h"

static void NDS_R2_CENSUS_EVICTED_CODE
ndsRendererScanList(const Gfx *dl,
                    const NDSRendererConfig *config,
                    NDSRendererStats *stats,
                    NDSRendererTraversalState *state,
                    u32 depth,
                    NDSRendererCommandCallback callback,
                    void *callback_user);

enum
{
    NDS_RENDERER_SCAN_COLD_PROCEED = 0,
    NDS_RENDERER_SCAN_COLD_CONTINUE = 1,
    NDS_RENDERER_SCAN_COLD_RETURN = 2
};

/* The generic command record is needed only when a command callback is active
 * or when a DL opcode must be resolved.  The canonical profile-0 battle does
 * neither in this scanner, so keep that record/branch machinery in main RAM
 * without changing its ordering or semantics for generic callers. */
static u32 __attribute__((noinline, cold))
ndsRendererScanColdCommand(const Gfx *dl,
                           const NDSRendererConfig *config,
                           NDSRendererStats *stats,
                           NDSRendererTraversalState *state,
                           u32 depth,
                           u32 list_index,
                           u32 w0,
                           u32 w1,
                           u32 op,
                           NDSRendererCommandCallback callback,
                           void *callback_user)
{
    NDSRendererCommand command;

    memset(&command, 0, sizeof(command));
    command.dl = dl;
    command.w0 = w0;
    command.w1 = w1;
    command.op = op;
    command.depth = depth;
    command.list_index = list_index;
    command.transformed_vertices = state->vertices;
    command.transformed_vertex_valid_mask = state->vertex_valid_mask;
    command.matrix_valid = state->matrix_valid;

    if (op == NDS_RENDERER_OP_DL)
    {
        command.raw_branch_dl = (const Gfx *)(uintptr_t)w1;
        command.resolved_branch_dl = command.raw_branch_dl;
        if (config->resolve_branch != NULL)
        {
            command.resolved_branch_dl = config->resolve_branch(
                command.raw_branch_dl,
                &command.branch_resolve_kind,
                config->user);
        }
        command.branch_is_jump =
            ((w0 & (1u << 16)) != 0) ? TRUE : FALSE;
    }

    if (stats->first_opcode == 0)
    {
        stats->first_opcode = op;
    }
    stats->command_count++;

    if ((callback != NULL) &&
        (callback(&command, callback_user) == FALSE))
    {
        ndsRendererRecordUnsupported(stats, op);
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return NDS_RENDERER_SCAN_COLD_RETURN;
    }
    if (op != NDS_RENDERER_OP_DL)
    {
        return NDS_RENDERER_SCAN_COLD_PROCEED;
    }

    {
        const Gfx *raw_branch = command.raw_branch_dl;
        const Gfx *branch = command.resolved_branch_dl;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        u32 parent_branch_path = state->semantic_branch_path;
        u32 child_branch_path;
#endif

        stats->branch_command_count++;
        if (stats->first_branch_dl == NULL)
        {
            stats->first_branch_dl = raw_branch;
        }
        if (stats->first_resolved_branch_dl == NULL)
        {
            stats->first_resolved_branch_dl = branch;
        }
        if (command.branch_resolve_kind == NDS_RENDERER_RESOLVE_SEGMENT)
        {
            stats->segment_resolve_count++;
        }
        if (ndsRendererValidateCommand(branch, config) == FALSE)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
            return NDS_RENDERER_SCAN_COLD_RETURN;
        }
        if ((w0 & (1u << 16)) != 0)
        {
            stats->branch_jump_count++;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
            child_branch_path = ndsRendererSemanticBranchPath(
                parent_branch_path, list_index, depth + 1u, TRUE);
            state->semantic_branch_path = child_branch_path;
#endif
            ndsRendererScanList(branch, config, stats, state, depth + 1u,
                                callback, callback_user);
            return NDS_RENDERER_SCAN_COLD_RETURN;
        }

        stats->branch_call_count++;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        child_branch_path = ndsRendererSemanticBranchPath(
            parent_branch_path, list_index, depth + 1u, FALSE);
        state->semantic_branch_path = child_branch_path;
#endif
        ndsRendererScanList(branch, config, stats, state, depth + 1u,
                            callback, callback_user);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        state->semantic_branch_path = parent_branch_path;
#endif
        return (stats->blocker != NDS_RENDERER_BLOCKER_NONE) ?
            NDS_RENDERER_SCAN_COLD_RETURN : NDS_RENDERER_SCAN_COLD_CONTINUE;
    }
}

/* These generic state opcodes are present for compatibility, but their
 * ScanList arms have zero PCs in the canonical whole-match census.  MOVEWORD
 * itself remains hot for its other callers; only this cold scanner dispatch is
 * moved out of ITCM. */
static void __attribute__((noinline, cold))
ndsRendererScanColdStateOpcode(const NDSRendererConfig *config,
                               NDSRendererStats *stats,
                               NDSRendererTraversalState *state,
                               u32 op, u32 w0, u32 w1)
{
    switch (op)
    {
    case NDS_RENDERER_OP_MTX:
        ndsRendererApplyMatrixCommand(config, stats, state, w0, w1);
        break;
    case NDS_RENDERER_OP_POPMTX:
        ndsRendererApplyPopMatrixCommand(stats, state, w1);
        break;
    case NDS_RENDERER_OP_MOVEWORD:
        ndsRendererApplyMatrixMoveWordCommand(stats, state, w0, w1);
        break;
    case NDS_RENDERER_OP_MOVEMEM:
        ndsRendererRecordLightMoveMem(config, stats, w0, w1);
        NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state);
        break;
    case NDS_RENDERER_OP_SPECIAL_1:
        ndsRendererApplyMvpRecalcCommand(stats, state, w0, w1);
        break;
    case NDS_RENDERER_OP_SETSCISSOR:
    case NDS_RENDERER_OP_SETCIMG:
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
        stats->ignored_state_command_count++;
        break;
    case NDS_RENDERER_OP_SETPRIMDEPTH:
        ndsRendererRecordPrimDepth(stats, w1);
        break;
    default:
        break;
    }
}

static void NDS_R2_CENSUS_EVICTED_CODE
ndsRendererScanList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats,
                                NDSRendererTraversalState *state,
                                u32 depth,
                                NDSRendererCommandCallback callback,
                                void *callback_user)
{
    u32 i;
    u32 immutable_command_count = 0u;

    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (depth > config->max_depth)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_TOO_DEEP;
        return;
    }
    if (ndsRendererValidateCommand(dl, config) == FALSE)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }
    if (depth > stats->max_depth_seen)
    {
        stats->max_depth_seen = depth;
    }
    if (config->immutable_command_span != NULL)
    {
        size_t immutable_bytes =
            config->immutable_command_span(dl, config->user);
        size_t immutable_count = immutable_bytes / sizeof(*dl);

        if (immutable_count > config->max_list_commands)
        {
            immutable_count = config->max_list_commands;
        }
        immutable_command_count = (u32)immutable_count;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (immutable_command_count != 0u)
        {
            sNdsRendererProfileImmutableListCount++;
        }
#endif
    }

    for (i = 0; i < config->max_list_commands; i++, dl++)
    {
        u32 w0;
        u32 w1;
        u32 op;

        if ((i >= immutable_command_count) &&
            (ndsRendererValidateCommand(dl, config) == FALSE))
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
            return;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (i < immutable_command_count)
        {
            sNdsRendererProfileTrustedCommandCount++;
        }
#endif
        if (stats->command_count >= config->max_commands)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
            return;
        }

        w0 = dl->words.w0;
        w1 = dl->words.w1;
        op = w0 >> 24;
#if NDS_RENDERER_HW_TRIANGLES
        state->source_command_site = dl;
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        state->semantic_command_index = i;
        state->semantic_tri2_half = 0u;
#endif
#if NDS_RENDERER_HW_TRIANGLES
        /* Preserve the source-command boundary: only adjacent TRI1/TRI2
         * opcodes may share a GX triangle group. In particular, close before
         * VTX/MODIFYVTX mutate the cached vertices and before any matrix,
         * texture, state, branch, sync, or ENDDL command. */
        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            ndsRendererHardwareEndBatch();
            /* VTX and matrix commands end the GX primitive group but cannot
             * change the prepared texture/material/depth epoch. The exact
             * state opcodes below invalidate that epoch at their mutation. */
            state->prepared_vertex_color_valid_mask = 0u;
            state->prepared_texcoord_valid_mask = 0u;
            state->prepared_projected_xy_valid_mask = 0u;
            state->prepared_projected_source_z_valid_mask = 0u;
        }
#endif
        if ((callback != NULL) || (op == NDS_RENDERER_OP_DL))
        {
            u32 cold_action = ndsRendererScanColdCommand(
                dl, config, stats, state, depth, i, w0, w1, op,
                callback, callback_user);

            if (cold_action == NDS_RENDERER_SCAN_COLD_RETURN)
            {
                return;
            }
            if (cold_action == NDS_RENDERER_SCAN_COLD_CONTINUE)
            {
                continue;
            }
        }
        else
        {
            if (stats->first_opcode == 0)
            {
                stats->first_opcode = op;
            }
            stats->command_count++;
        }

        switch (op)
        {
        case NDS_RENDERER_OP_NOOP:
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            break;

        case NDS_RENDERER_OP_MODIFYVTX:
            ndsRendererApplyModifyVertexCommand(stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_VTX:
            NDS_EFFECT_PHASE_VTX(
                ndsRendererApplyVertexCommand(config, stats, state, w0, w1));
            break;

        case NDS_RENDERER_OP_TRI1:
        case NDS_RENDERER_OP_TRI2:
        {
            ndsRendererExecuteTriangleCommand(
                stats, config, state, op, w0, w1);
#if NDS_RENDERER_HW_TRIANGLES
            /* Immutable adjacent TRI commands have no intervening source
             * state transition. Profile 0/1 has no command callback, so
             * replay the remainder of the run without rebuilding a generic
             * command record or re-entering the full opcode switch. */
#if NDS_RENDERER_PROFILE_LEVEL < 2
            if ((callback == NULL) && (sNdsRendererFastOwnerEnabled != 0u))
#else
            if (sNdsRendererFastOwnerEnabled != 0u)
#endif
            {
                ndsRendererExecuteFastRawCurrentRun(
                    &dl, &i, immutable_command_count,
                    config, stats, state, depth, callback, callback_user);
            }
#if NDS_RENDERER_PROFILE_LEVEL < 2
            else while ((callback == NULL) &&
                   ((i + 1u) < immutable_command_count) &&
                   ((i + 1u) < config->max_list_commands))
            {
                const Gfx *next_dl = dl + 1;
                u32 next_w0 = next_dl->words.w0;
                u32 next_op = next_w0 >> 24;

                if ((next_op != NDS_RENDERER_OP_TRI1) &&
                    (next_op != NDS_RENDERER_OP_TRI2))
                {
                    break;
                }
                if (stats->command_count >= config->max_commands)
                {
                    stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
                    return;
                }
                i++;
                dl = next_dl;
                stats->command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                sNdsRendererProfileTrustedCommandCount++;
                sNdsRendererProfileTriangleRunReuseCount++;
#endif
                ndsRendererExecuteTriangleCommand(
                    stats, config, state, next_op, next_w0,
                    next_dl->words.w1);
            }
#endif
#endif
            break;
        }

        case NDS_RENDERER_OP_ENDDL:
            stats->end_command_count++;
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            return;

        case NDS_RENDERER_OP_DL:
            /* Handled by ndsRendererScanColdCommand above. */
            break;

        case NDS_RENDERER_OP_TEXTURE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordTextureState(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_MTX:
        case NDS_RENDERER_OP_POPMTX:
        case NDS_RENDERER_OP_MOVEWORD:
        case NDS_RENDERER_OP_MOVEMEM:
        case NDS_RENDERER_OP_SPECIAL_1:
        case NDS_RENDERER_OP_SETSCISSOR:
        case NDS_RENDERER_OP_SETCIMG:
        case NDS_RENDERER_OP_SETPRIMDEPTH:
            ndsRendererScanColdStateOpcode(config, stats, state, op, w0, w1);
            break;

        case NDS_RENDERER_OP_GEOMETRYMODE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            stats->geometry_mode = (stats->geometry_mode & w0) | w1;
            stats->geometry_clear_mask = w0;
            stats->geometry_set_mask = w1;
            stats->geometry_command_count++;
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETCOMBINE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetCombine(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTIMG:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetImage(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTILE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetTile(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADTILE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadTile(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADBLOCK:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadBlock(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADTLUT:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadTlut(stats, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTILESIZE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetTileSize(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETFOGCOLOR:
            ndsRendererRecordFogColor(stats, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            stats->color_command_count++;
            break;

        case NDS_RENDERER_OP_SETBLENDCOLOR:
        case NDS_RENDERER_OP_SETENVCOLOR:
        case NDS_RENDERER_OP_SETPRIMCOLOR:
            if (op != NDS_RENDERER_OP_SETBLENDCOLOR)
            {
                NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            }
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            stats->color_command_count++;
            if (op == NDS_RENDERER_OP_SETPRIMCOLOR)
            {
                stats->prim_color = w1;
                stats->prim_min_level = (w0 >> 8) & 0xffu;
                stats->prim_lod_fraction = w0 & 0xffu;
            }
            else if (op == NDS_RENDERER_OP_SETENVCOLOR)
            {
                stats->env_color = w1;
            }
            else
            {
                stats->blend_color = w1;
            }
            break;

        case NDS_RENDERER_OP_RDPPIPESYNC:
        case NDS_RENDERER_OP_RDPLOADSYNC:
        case NDS_RENDERER_OP_RDPTILESYNC:
        case NDS_RENDERER_OP_RDPFULLSYNC:
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            stats->sync_command_count++;
            break;

        case NDS_RENDERER_OP_SETOTHERMODE_H:
        case NDS_RENDERER_OP_SETOTHERMODE_L:
        case NDS_RENDERER_OP_RDPSETOTHERMODE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordOtherMode(stats, op, w0, w1);
            break;

        case NDS_RENDERER_OP_CULLDL:
            ndsRendererRecordCull(stats, w0, w1);
            break;

        default:
            ndsRendererRecordUnsupported(stats, op);
            break;
        }
    }
}

void ndsRendererScanDisplayList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats)
{
    NDSRendererTraversalState state;
    NDSRendererTraversalVertexStorage vertex_storage;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererMatrixSnapshot
        matrix_snapshot_storage[NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY];
    NDSRendererMatrixSnapshot *matrix_snapshots = matrix_snapshot_storage;
#else
    NDSRendererMatrixSnapshot *matrix_snapshots = NULL;
#endif

    if (stats == NULL)
    {
        return;
    }

    if (config == NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }

    ndsRendererInitTraversalState(&state, config, stats, &vertex_storage,
                                  matrix_snapshots, 0u);
    ndsRendererScanList(dl, config, stats, &state, 0, NULL, NULL);
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererHardwareEndBatch();
#endif
    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (stats->unsupported_command_count != 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return;
    }
    if (stats->vertex_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_VERTICES;
        return;
    }
    if (stats->triangle_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_TRIANGLES;
        return;
    }
    if (stats->end_command_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_END;
        return;
    }
}

void ndsRendererExecuteDisplayListWithVertexCache(
    const Gfx *dl,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    NDSRendererTraversalState state;
    NDSRendererTraversalVertexStorage vertex_storage;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererMatrixSnapshot
        local_matrix_snapshots[NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY];
    NDSRendererMatrixSnapshot *matrix_snapshots = local_matrix_snapshots;
#else
    NDSRendererMatrixSnapshot *matrix_snapshots = NULL;
#endif
    u32 matrix_snapshot_count = 0u;

    if (stats == NULL)
    {
        return;
    }

    if (config == NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }

#if NDS_RENDERER_HW_TRIANGLES
    if (vertex_cache != NULL)
    {
        matrix_snapshots = vertex_cache->matrix_snapshots;
        matrix_snapshot_count = vertex_cache->matrix_snapshot_count;
    }
#endif
    ndsRendererInitTraversalState(&state, config, stats, &vertex_storage,
                                  matrix_snapshots, matrix_snapshot_count);
    if (vertex_cache != NULL)
    {
        /* BattleShip submits the selected lists through one persistent RSP
         * stream. Back traversal directly with that stream instead of
         * clearing and copying every 32-slot plane around each list. */
        state.vertices = vertex_cache->transformed_vertices;
        state.vertex_valid_mask = vertex_cache->transformed_valid_mask;
#if NDS_RENDERER_HW_TRIANGLES
        state.input_vertices = vertex_cache->input_vertices;
        state.input_vertex_valid_mask = vertex_cache->input_valid_mask;
        state.raw_vertex_fit_mask = vertex_cache->raw_vertex_fit_mask;
        state.vertex_colors = vertex_cache->vertex_colors;
        state.vertex_color_valid_mask =
            vertex_cache->vertex_color_valid_mask;
        state.vertex_matrix_snapshot = vertex_cache->vertex_matrix_snapshot;
        state.vertex_clip_snapshot = vertex_cache->vertex_clip_snapshot;
#endif
    }
    ndsRendererScanList(dl, config, stats, &state, 0, callback,
                        callback_user);
    if (vertex_cache != NULL)
    {
        vertex_cache->transformed_valid_mask = state.vertex_valid_mask;
#if NDS_RENDERER_HW_TRIANGLES
        vertex_cache->input_valid_mask = state.input_vertex_valid_mask;
        vertex_cache->raw_vertex_fit_mask = state.raw_vertex_fit_mask;
        vertex_cache->vertex_color_valid_mask =
            state.vertex_color_valid_mask;
        vertex_cache->matrix_snapshot_count = state.matrix_snapshot_count;
#endif
    }
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererHardwareEndBatch();
#endif
    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (stats->unsupported_command_count != 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return;
    }
    if (stats->end_command_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_END;
        return;
    }
}

void ndsRendererExecuteDisplayList(const Gfx *dl,
                                   const NDSRendererConfig *config,
                                   NDSRendererCommandCallback callback,
                                   void *callback_user,
                                   NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererExecuteDisplayListWithVertexCache(dl, config, callback,
                                                 callback_user, stats, NULL);
}
