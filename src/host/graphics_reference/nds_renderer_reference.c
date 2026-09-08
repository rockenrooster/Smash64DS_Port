/* Host reference only; include after the shared renderer units in a host oracle. */
#if defined(ARM9) || defined(ARM7) || defined(__NDS__)
#error Reference graphics implementations cannot be compiled into a Nintendo DS ROM
#endif
#include "nds_renderer_reference.h"

static s32 ndsRendererValidateCommand(const Gfx *dl,
                                       const NDSRendererConfig *config)
{
    uintptr_t addr = (uintptr_t)dl;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileValidatedCommandCount++;
#endif

    if ((dl == NULL) || ((addr & 0x3u) != 0))
    {
        return FALSE;
    }
    if (config->validate_range == NULL)
    {
        return TRUE;
    }
    return config->validate_range(dl, sizeof(*dl), config->user);
}

static void NDS_RENDERER_HOT_CODE
ndsRendererApplyVertexCommand(
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    u32 v0;
    u32 count;
    const u8 *src;
    u32 i;
#if NDS_RENDERER_HW_TRIANGLES
    const NDSRendererHardwareLightDirection *prepared_light_direction = NULL;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    const u32 *prepared_light_shade_lut = NULL;
#endif
    u32 matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
#endif

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->vertex_command_count++);
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    if (ndsGBIDecodeF3DEX2Vtx(w0, NDS_RENDERER_MAX_VTX, &v0,
                              &count) == FALSE)
    {
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
        return;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    stats->source_vertex_count += count;
#endif
    if ((v0 + count) > stats->vertex_count)
    {
        stats->vertex_count = v0 + count;
    }
#if !NDS_RENDERER_HW_TRIANGLES
    if (state->matrix_valid == 0u)
    {
        return;
    }
#endif

    src = ndsRendererResolveDataPointer(config,
                                        (const void *)(uintptr_t)w1,
                                        (size_t)count * 16u);
    if (src == NULL)
    {
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
        return;
    }
#if NDS_RENDERER_HW_TRIANGLES
    if (state->matrix_valid != 0u)
    {
        matrix_snapshot = ndsRendererAcquireCurrentMatrixSnapshot(state);
    }
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        if (state->prepared_light_direction_valid == 0u)
        {
            /* Matrix and MOVEMEM handlers invalidate this exact source-state
             * value; adjacent VTX commands can share its float normalization. */
            ndsRendererHardwarePrepareLitDirection(
                stats,
                (state->modelview_valid != 0u) ? &state->modelview : NULL,
                &state->prepared_light_direction);
            state->prepared_light_direction_valid = TRUE;
        }
        prepared_light_direction = &state->prepared_light_direction;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((stats->light_color_mask &
             (NDS_RENDERER_LIGHT_COLOR_1_MASK |
              NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
            (NDS_RENDERER_LIGHT_COLOR_1_MASK |
             NDS_RENDERER_LIGHT_COLOR_2_MASK))
        {
            prepared_light_shade_lut = ndsRendererHardwareGetLightShadeLut(
                stats->light_color_1, stats->light_color_2);
        }
#endif
    }
#endif

    for (i = 0u; i < count; i++)
    {
        u32 index = v0 + i;
#if NDS_RENDERER_HW_TRIANGLES
        NDSRendererInputVertex *input = &state->input_vertices[index];
        u32 mask = 1u << index;
#else
        NDSRendererInputVertex input_storage;
        NDSRendererInputVertex *input = &input_storage;
        NDSRendererClipVertex20p12 *out = &state->vertices[index];
#endif

        ndsRendererDecodeInputVertex(input, src + (i * 16u));
#if NDS_RENDERER_HW_TRIANGLES
        ndsRendererProfileRecordSourceVertexLoad();
        state->input_vertex_valid_mask |= mask;
        if (ndsRendererHardwareRawVertexFits(input) != FALSE)
        {
            state->raw_vertex_fit_mask |= mask;
        }
        else
        {
            state->raw_vertex_fit_mask &= ~mask;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (prepared_light_shade_lut != NULL)
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorLut(
                    input, prepared_light_direction,
                    prepared_light_shade_lut);
        }
        else
#endif
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
#endif
        if (state->matrix_valid == 0u)
        {
            continue;
        }
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        (void)ndsRendererTransformCachedVertex(
            stats, state, index, &state->matrix, matrix_snapshot);
#else
        /* Profile 0/1 keep source vertices raw for GX. Only an exhausted
         * snapshot table needs the eager clip fallback retained here. */
        if (matrix_snapshot == NDS_RENDERER_MATRIX_SNAPSHOT_INVALID)
        {
            (void)ndsRendererTransformCachedVertex(
                stats, state, index, &state->matrix, matrix_snapshot);
        }
#endif
#else
        ndsRendererTransformVertex20p12(&state->matrix, input, out);
        state->vertex_valid_mask |= 1u << index;
        stats->matrix_transform_count++;
        stats->transformed_vertex_count++;
        if (stats->transformed_vertex_count == 1u)
        {
            stats->first_transformed_x = out->x;
            stats->first_transformed_y = out->y;
            stats->first_transformed_z = out->z;
            stats->first_transformed_w = out->w;
        }
#endif
    }
}

static inline void ndsRendererExecuteTriangleCommand(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 op,
    u32 w0,
    u32 w1)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    u32 triangle_submit_start;
#endif
#if !NDS_RENDERER_HW_TRIANGLES
    (void)config;
#endif

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->triangle_command_count++);
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->render_command_count++);
    if (op == NDS_RENDERER_OP_TRI1)
    {
        u32 packed = ndsGBIDecodeF3DEX2Tri1(w0);

        stats->triangle_count++;
#if NDS_RENDERER_HW_TRIANGLES
        if (sNdsRendererHardwareNoOracle == 0u)
#endif
        ndsRendererRecordTransformedTriangle(stats, state, packed);
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        triangle_submit_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_tri2_half = 0u;
#endif
        NDS_EFFECT_PHASE_TRI(
            ndsRendererSubmitHardwareTriangle(stats, config, state, packed));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererProfileTriangleSubmitTicks +=
            cpuGetTiming() - triangle_submit_start;
#endif
#endif
        return;
    }

    stats->triangle_count += 2u;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererHardwareNoOracle == 0u)
    {
#endif
        ndsRendererRecordTransformedTriangle(
            stats, state, ndsGBIDecodeF3DEX2Tri2First(w0));
        ndsRendererRecordTransformedTriangle(
            stats, state, ndsGBIDecodeF3DEX2Tri2Second(w1));
#if NDS_RENDERER_HW_TRIANGLES
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    triangle_submit_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->semantic_tri2_half = 0u;
#endif
    NDS_EFFECT_PHASE_TRI(ndsRendererSubmitHardwareTriangle(
        stats, config, state, ndsGBIDecodeF3DEX2Tri2First(w0)));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->semantic_tri2_half = 1u;
#endif
    NDS_EFFECT_PHASE_TRI(ndsRendererSubmitHardwareTriangle(
        stats, config, state, ndsGBIDecodeF3DEX2Tri2Second(w1)));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileTriangleSubmitTicks +=
        cpuGetTiming() - triangle_submit_start;
#endif
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
/* Direct immutable TRI-run records are topology only.  Live vertex, matrix,
 * material, texture, and light state remains in the traversal object and is
 * rebound by the existing exact path.  The cache is reset with reloc/source
 * caches at scene boundaries, so it never survives pointer ownership changes. */
#define NDS_RENDERER_DIRECT_RAW_PLAN_COUNT 128u
#define NDS_RENDERER_DIRECT_RAW_ENTRY_COUNT 384u

typedef struct NDSRendererDirectRawEntry
{
    u32 required_mask;
    u8 indices[6];
    u8 triangle_count;
    u8 reserved;
} NDSRendererDirectRawEntry;

typedef struct NDSRendererDirectRawPlan
{
    const Gfx *source;
    u16 entry_offset;
    u16 command_count;
    u16 triangle_count;
    u16 reserved;
    u32 first_w0;
    u32 first_w1;
    u32 last_w0;
    u32 last_w1;
} NDSRendererDirectRawPlan;

static NDSRendererDirectRawPlan
    sNdsRendererDirectRawPlans[NDS_RENDERER_DIRECT_RAW_PLAN_COUNT];
static NDSRendererDirectRawEntry
    sNdsRendererDirectRawEntries[NDS_RENDERER_DIRECT_RAW_ENTRY_COUNT];
static u32 sNdsRendererDirectRawEntryCount;

_Static_assert(
    (sizeof(sNdsRendererDirectRawPlans) +
     sizeof(sNdsRendererDirectRawEntries)) <= (8u * 1024u),
    "direct raw topology cache must remain within 8 KiB");

static u32 ndsRendererDirectRawPlanHash(const Gfx *source)
{
    uintptr_t value = (uintptr_t)source;

    return (u32)(((value >> 3) ^ (value >> 11)) &
                 (NDS_RENDERER_DIRECT_RAW_PLAN_COUNT - 1u));
}

static NDSRendererDirectRawPlan *ndsRendererDirectRawFindPlan(
    const Gfx *source, u32 max_commands)
{
    NDSRendererDirectRawPlan *empty = NULL;
    u32 slot = ndsRendererDirectRawPlanHash(source);
    u32 probe;
    u32 command_count = 0u;
    u32 triangle_count = 0u;
    u32 entry_offset;
    u32 i;

    if ((source == NULL) || (max_commands == 0u))
    {
        return NULL;
    }
    for (probe = 0u; probe < NDS_RENDERER_DIRECT_RAW_PLAN_COUNT; probe++)
    {
        NDSRendererDirectRawPlan *plan =
            &sNdsRendererDirectRawPlans[
                (slot + probe) & (NDS_RENDERER_DIRECT_RAW_PLAN_COUNT - 1u)];

        if (plan->source == source)
        {
            const Gfx *last;

            if ((plan->command_count == 0u) ||
                (plan->command_count > max_commands))
            {
                return NULL;
            }
            last = source + plan->command_count - 1u;
            if ((source->words.w0 != plan->first_w0) ||
                (source->words.w1 != plan->first_w1) ||
                (last->words.w0 != plan->last_w0) ||
                (last->words.w1 != plan->last_w1))
            {
                return NULL;
            }
            return plan;
        }
        if (plan->source == NULL)
        {
            empty = plan;
            break;
        }
    }
    if (empty == NULL)
    {
        return NULL;
    }

    while (command_count < max_commands)
    {
        u32 op = source[command_count].words.w0 >> 24;

        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            break;
        }
        triangle_count += (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        command_count++;
    }
    if ((command_count == 0u) ||
        (command_count > 0xffffu) ||
        (triangle_count > 0xffffu) ||
        ((sNdsRendererDirectRawEntryCount + command_count) >
         NDS_RENDERER_DIRECT_RAW_ENTRY_COUNT))
    {
        return NULL;
    }

    entry_offset = sNdsRendererDirectRawEntryCount;
    for (i = 0u; i < command_count; i++)
    {
        const Gfx *command = source + i;
        NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[entry_offset + i];
        u32 w0 = command->words.w0;
        u32 w1 = command->words.w1;
        u32 op = w0 >> 24;
        u32 packed[2];
        u32 indices[6];
        u32 required_mask = 0u;
        u32 command_triangles =
            (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        u32 index;

        packed[0] = (op == NDS_RENDERER_OP_TRI1) ?
            ndsGBIDecodeF3DEX2Tri1(w0) :
            ndsGBIDecodeF3DEX2Tri2First(w0);
        packed[1] = (command_triangles == 2u) ?
            ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
        if ((ndsRendererFastDecodeTriangle(
                 packed[0], &indices[0], &required_mask) == FALSE) ||
            ((command_triangles == 2u) &&
             (ndsRendererFastDecodeTriangle(
                  packed[1], &indices[3], &required_mask) == FALSE)))
        {
            return NULL;
        }
        entry->required_mask = required_mask;
        entry->triangle_count = (u8)command_triangles;
        entry->reserved = 0u;
        for (index = 0u; index < (command_triangles * 3u); index++)
        {
            entry->indices[index] = (u8)indices[index];
        }
        for (; index < 6u; index++)
        {
            entry->indices[index] = 0u;
        }
    }

    sNdsRendererDirectRawEntryCount += command_count;
    empty->entry_offset = (u16)entry_offset;
    empty->command_count = (u16)command_count;
    empty->triangle_count = (u16)triangle_count;
    empty->reserved = 0u;
    empty->first_w0 = source->words.w0;
    empty->first_w1 = source->words.w1;
    empty->last_w0 = source[command_count - 1u].words.w0;
    empty->last_w1 = source[command_count - 1u].words.w1;
    empty->source = source;
    return empty;
}

static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererFastRawFallbackCommand(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 op,
    u32 w0,
    u32 w1)
{
    ndsRendererExecuteTriangleCommand(stats, config, state, op, w0, w1);
}

static inline void ndsRendererFastEmitDirectRawEntry(
    const NDSRendererTraversalState *state,
    const NDSRendererDirectRawEntry *entry,
    u32 textured)
{
    u32 vertex_count = (u32)entry->triangle_count * 3u;
    u32 vertex_index;

    if (textured != 0u)
    {
        for (vertex_index = 0u; vertex_index < vertex_count; vertex_index++)
        {
            ndsRendererFastEmitRawTexturedVertex(
                state, entry->indices[vertex_index]);
        }
    }
    else
    {
        for (vertex_index = 0u; vertex_index < vertex_count; vertex_index++)
        {
            ndsRendererFastEmitRawUntexturedVertex(
                state, entry->indices[vertex_index]);
        }
    }
}

static s32 NDS_RENDERER_FAST_RUN_CODE ndsRendererExecuteDirectRawRemainder(
    const Gfx **dl_io,
    u32 *list_index_io,
    u32 immutable_command_count,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    NDSRendererCommandCallback callback)
{
    const Gfx *source;
    NDSRendererDirectRawPlan *plan;
    u32 first_index;
    u32 remaining_commands;
    u32 available_mask;
    u32 required_mask = 0u;
    u32 entry_number;
    u32 textured;

    if ((callback != NULL) ||
        (ndsRendererFastRawStateEligible(state) == FALSE))
    {
        return FALSE;
    }
    first_index = *list_index_io + 1u;
    if ((first_index >= immutable_command_count) ||
        (first_index >= config->max_list_commands))
    {
        return FALSE;
    }
    remaining_commands = immutable_command_count - first_index;
    if (remaining_commands > (config->max_list_commands - first_index))
    {
        remaining_commands = config->max_list_commands - first_index;
    }
    source = *dl_io + 1;
    plan = ndsRendererDirectRawFindPlan(source, remaining_commands);
    if ((plan == NULL) ||
        ((stats->command_count + plan->command_count) > config->max_commands))
    {
        return FALSE;
    }

    available_mask = state->input_vertex_valid_mask &
        state->raw_vertex_fit_mask & state->current_transform_vertex_mask;
    for (entry_number = 0u;
         entry_number < plan->command_count;
         entry_number++)
    {
        const NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[
                plan->entry_offset + entry_number];

        if ((available_mask & entry->required_mask) != entry->required_mask)
        {
            return FALSE;
        }
        required_mask |= entry->required_mask;
    }

    textured = state->texture_prepare_enabled;
    ndsRendererFastPrepareRawSlots(stats, state, required_mask, textured);
    for (entry_number = 0u;
         entry_number < plan->command_count;
         entry_number++)
    {
        const Gfx *command = source + entry_number;
        const NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[
                plan->entry_offset + entry_number];

        stats->command_count++;
        NDS_RENDERER_RECORD_PROOF_ONLY(
            stats->triangle_command_count++;
            stats->render_command_count++;
        );
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_command_index = first_index + entry_number;
        state->semantic_tri2_half = 0u;
        sNdsRendererProfileTrustedCommandCount++;
        sNdsRendererProfileTriangleRunReuseCount++;
#endif
        ndsRendererFastEmitDirectRawEntry(state, entry, textured);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        {
            u32 w0 = command->words.w0;
            u32 w1 = command->words.w1;
            u32 packed[2];
            u32 triangle_index;

            packed[0] = (entry->triangle_count == 1u) ?
                ndsGBIDecodeF3DEX2Tri1(w0) :
                ndsGBIDecodeF3DEX2Tri2First(w0);
            packed[1] = (entry->triangle_count == 2u) ?
                ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
            for (triangle_index = 0u;
                 triangle_index < entry->triangle_count;
                 triangle_index++)
            {
                u32 indices[3];
                u32 base = triangle_index * 3u;

                indices[0] = entry->indices[base];
                indices[1] = entry->indices[base + 1u];
                indices[2] = entry->indices[base + 2u];
                if (sNdsRendererHardwareNoOracle == 0u)
                {
                    ndsRendererRecordTransformedTriangle(
                        stats, state, packed[triangle_index]);
                }
                state->semantic_tri2_half = triangle_index;
                ndsRendererFastCommitRawSemanticTriangle(
                    stats, state, packed[triangle_index], indices);
            }
        }
#else
        (void)command;
#endif
    }

    stats->triangle_count += plan->triangle_count;
    ndsRendererFastAccountRawTriangles(
        stats, plan->triangle_count, plan->triangle_count);
    sNdsRendererFastRunCount++;
    sNdsRendererFastTriangleCount += plan->triangle_count;
    if ((u32)sNdsRendererRuntimeOwner <
        (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererFastOwnerTriangleCount[
            (u32)sNdsRendererRuntimeOwner] += plan->triangle_count;
    }
    *dl_io = source + plan->command_count - 1u;
    *list_index_io = first_index + plan->command_count - 1u;
    return TRUE;
}

static void NDS_RENDERER_FAST_RUN_CODE ndsRendererExecuteFastRawCurrentRun(
    const Gfx **dl_io,
    u32 *list_index_io,
    u32 immutable_command_count,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 depth,
    NDSRendererCommandCallback callback,
    void *callback_user)
{
    const Gfx *dl = *dl_io;
    u32 list_index = *list_index_io;
    u32 fast_triangles = 0u;
    u32 fallback_state = 0u;
    u32 fallback_vertex = 0u;
    u32 fallback_command = 0u;
    u32 fast_command_count = 0u;

    if (ndsRendererExecuteDirectRawRemainder(
            dl_io, list_index_io, immutable_command_count,
            config, stats, state, callback) != FALSE)
    {
        return;
    }

    while (((list_index + 1u) < immutable_command_count) &&
           ((list_index + 1u) < config->max_list_commands))
    {
        const Gfx *next_dl = dl + 1;
        u32 w0 = next_dl->words.w0;
        u32 w1 = next_dl->words.w1;
        u32 op = w0 >> 24;
        u32 packed[2];
        u32 indices[6];
        u32 required_mask = 0u;
        u32 triangle_count;
        s32 decode_ok;
        s32 state_ok;
        s32 vertex_ok;
        NDSRendererCommand command;

        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            break;
        }
        if (stats->command_count >= config->max_commands)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
            break;
        }

        list_index++;
        dl = next_dl;
        stats->command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_command_index = list_index;
        state->semantic_tri2_half = 0u;
        sNdsRendererProfileTrustedCommandCount++;
        sNdsRendererProfileTriangleRunReuseCount++;
#endif
        if (callback != NULL)
        {
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
            if (callback(&command, callback_user) == FALSE)
            {
                ndsRendererRecordUnsupported(stats, op);
                stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
                break;
            }
        }
        triangle_count = (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        packed[0] = (op == NDS_RENDERER_OP_TRI1) ?
            ndsGBIDecodeF3DEX2Tri1(w0) :
            ndsGBIDecodeF3DEX2Tri2First(w0);
        packed[1] = (triangle_count == 2u) ?
            ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
        decode_ok = ndsRendererFastDecodeTriangle(
            packed[0], &indices[0], &required_mask);
        if ((decode_ok != FALSE) && (triangle_count == 2u))
        {
            decode_ok = ndsRendererFastDecodeTriangle(
                packed[1], &indices[3], &required_mask);
        }
        state_ok = ndsRendererFastRawStateEligible(state);
        vertex_ok = ((decode_ok != FALSE) &&
                     ((state->input_vertex_valid_mask & required_mask) ==
                      required_mask) &&
                     ((state->raw_vertex_fit_mask & required_mask) ==
                      required_mask) &&
                     ((state->current_transform_vertex_mask & required_mask) ==
                      required_mask)) ? TRUE : FALSE;

        if ((state_ok != FALSE) && (vertex_ok != FALSE))
        {
            u32 textured = state->texture_prepare_enabled;
            u32 triangle_index;

            NDS_RENDERER_RECORD_PROOF_ONLY(
                stats->triangle_command_count++;
                stats->render_command_count++;
            );
            stats->triangle_count += triangle_count;
            ndsRendererFastPrepareRawSlots(
                stats, state, required_mask, textured);
            for (triangle_index = 0u;
                 triangle_index < triangle_count;
                 triangle_index++)
            {
                const u32 *triangle = &indices[triangle_index * 3u];

                if (sNdsRendererHardwareNoOracle == 0u)
                {
                    ndsRendererRecordTransformedTriangle(
                        stats, state, packed[triangle_index]);
                }
                ndsRendererFastEmitRawCommand(
                    state, triangle, 1u, textured);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                state->semantic_tri2_half = triangle_index;
                ndsRendererFastCommitRawSemanticTriangle(
                    stats, state, packed[triangle_index], triangle);
#endif
            }
            fast_triangles += triangle_count;
            fast_command_count++;
        }
        else
        {
            if (decode_ok == FALSE)
            {
                fallback_command++;
            }
            else if (state_ok == FALSE)
            {
                fallback_state++;
            }
            else
            {
                fallback_vertex++;
            }
            ndsRendererFastRawFallbackCommand(
                stats, config, state, op, w0, w1);
        }
    }

    if (fast_triangles != 0u)
    {
        ndsRendererFastAccountRawTriangles(
            stats, fast_triangles, fast_triangles);
        sNdsRendererFastRunCount++;
        sNdsRendererFastTriangleCount += fast_triangles;
        if ((u32)sNdsRendererRuntimeOwner <
            (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
        {
            sNdsRendererFastOwnerTriangleCount[
                (u32)sNdsRendererRuntimeOwner] += fast_triangles;
        }
    }
    (void)fast_command_count;
    sNdsRendererFastFallbackCount[0] += fallback_state;
    sNdsRendererFastFallbackCount[1] += fallback_vertex;
    sNdsRendererFastFallbackCount[2] += fallback_command;
    *dl_io = dl;
    *list_index_io = list_index;
}
#endif

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
#if NDS_RENDERER_HW_TRIANGLES
    /* Host runs do not share derived source-pointer plans across submissions. */
    memset(sNdsRendererDirectRawPlans, 0, sizeof(sNdsRendererDirectRawPlans));
    sNdsRendererDirectRawEntryCount = 0u;
#endif
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
#if NDS_RENDERER_HW_TRIANGLES
    memset(sNdsRendererDirectRawPlans, 0, sizeof(sNdsRendererDirectRawPlans));
    sNdsRendererDirectRawEntryCount = 0u;
#endif
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
