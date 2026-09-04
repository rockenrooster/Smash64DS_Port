#if NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_STAGE_DL_HEADS 4u

static NDSFighterDLDrawState sNdsRendererAdapterStagePersistentState;
static NDSRendererStats sNdsRendererAdapterStagePersistentStats;
static NDSRendererVertexCache sNdsRendererAdapterStageVertexCache;
static sb32 sNdsRendererAdapterStagePersistentActive;
/* Set only while an EFFECT tree submit is on the stack. The stage, the weapons
 * and the effects all reach the hardware through the same SubmitStageDL, and
 * the stage submits hundreds of lists per frame, so publishing the executor's
 * verdict unconditionally would report the last stage list rather than the
 * effect that is being investigated. */
static sb32 sNdsRendererAdapterEffectSubmitActive;
/* Items share the source DObj interpreter with stage/effects, but they are a
 * separate display layer with their own pre-model RDP state.  LinkBomb is the
 * first live client; keeping a distinct flag prevents item ColAnim state from
 * contaminating the effect layer's sticky blend state/diagnostics. */
static sb32 sNdsRendererAdapterItemSubmitActive;
static u32 sNdsRendererAdapterItemSubmitHead;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 sNdsRendererAdapterStageOwnerOccurrence;
static u32 sNdsRendererAdapterStageNextOccurrence;
static u32 sNdsRendererAdapterStageListOrdinal;
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
#define NDS_RENDERER_OWNER_HASH_SEED 2166136261u

typedef struct NDSRendererOwnerStatsSnapshot
{
    u32 vertex_command_count;
    u32 source_vertex_count;
    u32 triangle_command_count;
    u32 triangle_count;
    u32 matrix_command_count;
} NDSRendererOwnerStatsSnapshot;

static u32 ndsRendererOwnerHashBytes(u32 hash, const void *data,
                                     size_t bytes)
{
    const u8 *cursor = data;
    size_t i;

    if (hash == 0u)
    {
        hash = NDS_RENDERER_OWNER_HASH_SEED;
    }
    for (i = 0u; i < bytes; i++)
    {
        hash ^= cursor[i];
        hash *= 16777619u;
    }
    /* Zero is the public "not started" sentinel for the compact ledgers.
     * Keep an intermediate hash from ever aliasing that sentinel. */
    if (hash == 0u)
    {
        hash = 1u;
    }
    return hash;
}

static u32 ndsRendererOwnerHashU32(u32 hash, u32 value)
{
    return ndsRendererOwnerHashBytes(hash, &value, sizeof(value));
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererOwnerRootBranchPath(
    const NDSRelocLoadedFile *loaded, const Gfx *dl, u32 selected_event)
{
    u32 hash = 0u;

    hash = ndsRendererOwnerHashU32(hash, 0x524f4f54u);
    if ((loaded != NULL) && ((uintptr_t)dl >= (uintptr_t)loaded->data) &&
        ((uintptr_t)dl <
         ((uintptr_t)loaded->data + loaded->data_size)))
    {
        hash = ndsRendererOwnerHashU32(hash, 1u);
        hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
        hash = ndsRendererOwnerHashU32(hash, loaded->owner_generation);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl - (uintptr_t)loaded->data));
    }
    else if ((gSYTaskmanGraphicsHeap.start != NULL) &&
             (gSYTaskmanGraphicsHeap.end != NULL) &&
             ((uintptr_t)dl >=
              (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
             ((uintptr_t)dl <
              (uintptr_t)gSYTaskmanGraphicsHeap.end))
    {
        hash = ndsRendererOwnerHashU32(hash, 2u);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl -
                        (uintptr_t)gSYTaskmanGraphicsHeap.start));
    }
    else
    {
        /* Valid roots are reloc- or taskman-backed. Preserve a stable
         * segmented source token for any future resolver-backed root without
         * hashing its process address. */
        hash = ndsRendererOwnerHashU32(hash, 3u);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl & 0x00ffffffu));
    }
    hash = ndsRendererOwnerHashU32(hash, selected_event);
    return hash;
}
#endif

#define NDS_RENDERER_OWNER_POINTER_NULL 0u
#define NDS_RENDERER_OWNER_POINTER_EMPTY_SEGMENT 1u
#define NDS_RENDERER_OWNER_POINTER_RELOC 2u
#define NDS_RENDERER_OWNER_POINTER_TASKMAN 3u
#define NDS_RENDERER_OWNER_POINTER_GRAPHICS_HEAP 4u
#define NDS_RENDERER_OWNER_POINTER_SEGMENTED 5u
#define NDS_RENDERER_OWNER_POINTER_RAW 6u

static u32 ndsRendererOwnerHashStablePointer(u32 hash, uintptr_t value)
{
    const NDSRelocLoadedFile *loaded;
    const u8 *arena = ndsTaskmanArenaStart();
    uintptr_t arena_base = (uintptr_t)arena;
    size_t arena_size = ndsTaskmanArenaSize();
    u32 segment = (u32)(value >> 24);

    hash = ndsRendererOwnerHashU32(hash, 0x50545231u);
    if (value == 0u)
    {
        return ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_NULL);
    }
    if (value == (uintptr_t)sNdsRendererAdapterEmptySegmentEDL)
    {
        return ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_EMPTY_SEGMENT);
    }

    loaded = ndsRelocFindLoadedFileContaining(
        (const void *)value, 1u);
    if (loaded != NULL)
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_RELOC);
        hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
        hash = ndsRendererOwnerHashU32(
            hash, loaded->owner_generation);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value - (uintptr_t)loaded->data));
    }
    if ((arena != NULL) && (value >= arena_base) &&
        ((size_t)(value - arena_base) < arena_size))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_TASKMAN);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value - arena_base));
    }
    if ((gSYTaskmanGraphicsHeap.start != NULL) &&
        (gSYTaskmanGraphicsHeap.end != NULL) &&
        (value >= (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
        (value < (uintptr_t)gSYTaskmanGraphicsHeap.end))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_GRAPHICS_HEAP);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value -
                        (uintptr_t)gSYTaskmanGraphicsHeap.start));
    }
    if ((segment != 0u) && (segment <= 0x0fu))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_SEGMENTED);
        hash = ndsRendererOwnerHashU32(hash, segment);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value & 0x00ffffffu));
    }

    /* Valid renderer operands are reloc-, taskman-, or segment-backed. Keep
     * an explicit raw fallback so an unexpected operand mutation is still
     * visible instead of silently aliasing the null provenance. */
    hash = ndsRendererOwnerHashU32(
        hash, NDS_RENDERER_OWNER_POINTER_RAW);
    return ndsRendererOwnerHashU32(hash, (u32)value);
}

static s32 ndsRendererOwnerCommandUsesPointer(u32 op)
{
    return ((op == NDS_FIGHTER_DL_OP_VTX) ||
            (op == NDS_FIGHTER_DL_OP_MTX) ||
            (op == 0xdcu) || /* F3DEX2 G_MOVEMEM */
            (op == NDS_FIGHTER_DL_OP_DL) ||
            (op == NDS_FIGHTER_DL_OP_SETTIMG) ||
            (op == 0xfeu) || /* G_SETZIMG */
            (op == NDS_FIGHTER_DL_OP_SETCIMG)) ? TRUE : FALSE;
}

static u32 ndsRendererOwnerHashDisplayList(
    u32 hash, const Gfx *dl, const NDSRendererConfig *config,
    u32 depth, u32 *remaining_commands)
{
    u32 i;

    hash = ndsRendererOwnerHashU32(hash, 0x4c495354u);
    hash = ndsRendererOwnerHashStablePointer(
        hash, (uintptr_t)dl);
    hash = ndsRendererOwnerHashU32(hash, depth);
    if ((dl == NULL) || (config == NULL) ||
        (remaining_commands == NULL))
    {
        return ndsRendererOwnerHashU32(hash, 0xffffffffu);
    }
    if (depth > config->max_depth)
    {
        return ndsRendererOwnerHashU32(hash, 0xfffffffeu);
    }

    for (i = 0u; i < config->max_list_commands; i++, dl++)
    {
        u32 w0;
        u32 w1;
        u32 op;

        if (*remaining_commands == 0u)
        {
            return ndsRendererOwnerHashU32(hash, 0xfffffffdu);
        }
        if ((config->validate_range != NULL) &&
            (config->validate_range(dl, sizeof(*dl), config->user) == FALSE))
        {
            hash = ndsRendererOwnerHashStablePointer(
                hash, (uintptr_t)dl);
            return ndsRendererOwnerHashU32(hash, 0xfffffffcu);
        }

        w0 = dl->words.w0;
        w1 = dl->words.w1;
        op = w0 >> 24;
        (*remaining_commands)--;
        hash = ndsRendererOwnerHashU32(hash, 0x434d4431u);
        hash = ndsRendererOwnerHashU32(hash, i);
        hash = ndsRendererOwnerHashU32(hash, w0);
        if (ndsRendererOwnerCommandUsesPointer(op) != FALSE)
        {
            hash = ndsRendererOwnerHashStablePointer(
                hash, (uintptr_t)w1);
        }
        else
        {
            hash = ndsRendererOwnerHashU32(hash, w1);
        }

        if (op == NDS_FIGHTER_DL_OP_DL)
        {
            const Gfx *branch = (const Gfx *)(uintptr_t)w1;
            u32 resolve_kind = NDS_RENDERER_RESOLVE_NONE;
            u32 branch_is_jump =
                ((w0 & (1u << 16)) != 0u) ? TRUE : FALSE;

            if (config->resolve_branch != NULL)
            {
                branch = config->resolve_branch(
                    branch, &resolve_kind, config->user);
            }
            hash = ndsRendererOwnerHashU32(hash, 0x4252414eu);
            hash = ndsRendererOwnerHashU32(hash, resolve_kind);
            hash = ndsRendererOwnerHashU32(hash, branch_is_jump);
            hash = ndsRendererOwnerHashDisplayList(
                hash, branch, config, depth + 1u, remaining_commands);
            if (branch_is_jump != FALSE)
            {
                return hash;
            }
        }
        else if (op == NDS_FIGHTER_DL_OP_ENDDL)
        {
            return ndsRendererOwnerHashU32(hash, 0x454e444cu);
        }
    }
    return ndsRendererOwnerHashU32(hash, 0x4e4f454eu);
}

static u32 ndsRendererOwnerHashTileState(
    u32 hash, const NDSRendererTileState *tile)
{
#define NDS_RENDERER_HASH_TILE_FIELD(field) \
    hash = ndsRendererOwnerHashU32(hash, tile->field)

    NDS_RENDERER_HASH_TILE_FIELD(set_seen);
    NDS_RENDERER_HASH_TILE_FIELD(size_seen);
    NDS_RENDERER_HASH_TILE_FIELD(format);
    NDS_RENDERER_HASH_TILE_FIELD(size);
    NDS_RENDERER_HASH_TILE_FIELD(line);
    NDS_RENDERER_HASH_TILE_FIELD(tmem);
    NDS_RENDERER_HASH_TILE_FIELD(palette);
    NDS_RENDERER_HASH_TILE_FIELD(cms);
    NDS_RENDERER_HASH_TILE_FIELD(cmt);
    NDS_RENDERER_HASH_TILE_FIELD(masks);
    NDS_RENDERER_HASH_TILE_FIELD(maskt);
    NDS_RENDERER_HASH_TILE_FIELD(shifts);
    NDS_RENDERER_HASH_TILE_FIELD(shiftt);
    NDS_RENDERER_HASH_TILE_FIELD(uls);
    NDS_RENDERER_HASH_TILE_FIELD(ult);
    NDS_RENDERER_HASH_TILE_FIELD(lrs);
    NDS_RENDERER_HASH_TILE_FIELD(lrt);
    NDS_RENDERER_HASH_TILE_FIELD(width);
    NDS_RENDERER_HASH_TILE_FIELD(height);
    NDS_RENDERER_HASH_TILE_FIELD(flags);

#undef NDS_RENDERER_HASH_TILE_FIELD
    return hash;
}

static u32 ndsRendererOwnerHashTextureLoadState(
    u32 hash, const NDSRendererTextureLoadState *load)
{
    hash = ndsRendererOwnerHashU32(hash, load->image);
    hash = ndsRendererOwnerHashU32(hash, load->sequence);
    hash = ndsRendererOwnerHashU32(hash, load->image_width);
    hash = ndsRendererOwnerHashU32(hash, load->load_uls);
    hash = ndsRendererOwnerHashU32(hash, load->load_ult);
    hash = ndsRendererOwnerHashU32(hash, load->load_lrs);
    hash = ndsRendererOwnerHashU32(hash, load->load_dxt);
    hash = ndsRendererOwnerHashU32(hash, load->load_texels);
    hash = ndsRendererOwnerHashU32(hash, load->load_tmem);
    hash = ndsRendererOwnerHashU32(hash, load->valid);
    hash = ndsRendererOwnerHashU32(hash, load->image_format);
    hash = ndsRendererOwnerHashU32(hash, load->image_size);
    hash = ndsRendererOwnerHashU32(hash, load->load_kind);
    hash = ndsRendererOwnerHashU32(hash, load->load_tile);
    return hash;
}

static u32 ndsRendererOwnerHashRuntimeState(const NDSRendererStats *stats)
{
    u32 hash = 0u;
    u32 i;

    if (stats == NULL)
    {
        return 0u;
    }

    /* Serialize exactly the persistent renderer contract copied by
     * ndsFighterDLDrawCopyPersistentRendererState().  Do not hash the raw
     * tail: it interleaves proof counters and pointer-bearing diagnostics,
     * and struct padding is not semantic state. */
#define NDS_RENDERER_HASH_STATE_FIELD(field) \
    hash = ndsRendererOwnerHashU32(hash, (u32)stats->field)

    NDS_RENDERER_HASH_STATE_FIELD(othermode_h);
    NDS_RENDERER_HASH_STATE_FIELD(othermode_l);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_mode);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_clear_mask);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_set_mask);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_kind);
    NDS_RENDERER_HASH_STATE_FIELD(texture_scale_s);
    NDS_RENDERER_HASH_STATE_FIELD(texture_scale_t);
    NDS_RENDERER_HASH_STATE_FIELD(texture_level);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_on);
    NDS_RENDERER_HASH_STATE_FIELD(texture_xparam);
    NDS_RENDERER_HASH_STATE_FIELD(texture_state_flags);
    NDS_RENDERER_HASH_STATE_FIELD(texture_image);
    NDS_RENDERER_HASH_STATE_FIELD(texture_format);
    NDS_RENDERER_HASH_STATE_FIELD(texture_size);
    NDS_RENDERER_HASH_STATE_FIELD(texture_image_width);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_image);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_count);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_format);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_size);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_line);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_tmem);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_palette);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_cms);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_cmt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_masks);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_maskt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_shifts);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_shiftt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_flags);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_uls);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_ult);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_lrs);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_dxt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_texels);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_uls);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_ult);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_lrs);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_lrt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_width);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_height);
    for (i = 0u; i < NDS_RENDERER_TILE_COUNT; i++)
    {
        hash = ndsRendererOwnerHashTileState(hash,
                                              &stats->texture_tiles[i]);
    }
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_sequence);
    for (i = 0u; i < NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT; i++)
    {
        hash = ndsRendererOwnerHashTextureLoadState(
            hash, &stats->texture_loads[i]);
    }
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_w0);
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_w1);
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_count);
    NDS_RENDERER_HASH_STATE_FIELD(prim_color);
    NDS_RENDERER_HASH_STATE_FIELD(prim_min_level);
    NDS_RENDERER_HASH_STATE_FIELD(prim_lod_fraction);
    NDS_RENDERER_HASH_STATE_FIELD(env_color);
    NDS_RENDERER_HASH_STATE_FIELD(blend_color);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_1);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_2);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_mask);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_x);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_y);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_z);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_mask);
    NDS_RENDERER_HASH_STATE_FIELD(prim_depth);
    NDS_RENDERER_HASH_STATE_FIELD(prim_depth_delta);
    NDS_RENDERER_HASH_STATE_FIELD(fog_color);
    NDS_RENDERER_HASH_STATE_FIELD(fog_min);
    NDS_RENDERER_HASH_STATE_FIELD(fog_max);
    NDS_RENDERER_HASH_STATE_FIELD(fog_status);
    NDS_RENDERER_HASH_STATE_FIELD(texture_source_hash1);
    NDS_RENDERER_HASH_STATE_FIELD(texture_source_hash2);

#undef NDS_RENDERER_HASH_STATE_FIELD
    return hash;
}

static u32 ndsRendererOwnerHashVertexCache(
    const NDSRendererVertexCache *cache)
{
    u32 hash = 0u;
    u32 i;
    u32 row;
    u32 col;
    u32 snapshot_count;
    u32 input_mask;
    u32 transformed_mask;
    u32 color_mask;

    if (cache == NULL)
    {
        return 0u;
    }
    input_mask = cache->input_valid_mask;
    transformed_mask = cache->transformed_valid_mask & input_mask;
    color_mask = cache->vertex_color_valid_mask & input_mask;
    hash = ndsRendererOwnerHashU32(hash, input_mask);
    hash = ndsRendererOwnerHashU32(
        hash, cache->raw_vertex_fit_mask & input_mask);
    hash = ndsRendererOwnerHashU32(hash, transformed_mask);
    hash = ndsRendererOwnerHashU32(hash, color_mask);
    snapshot_count = cache->matrix_snapshot_count;
    if (snapshot_count > NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY)
    {
        snapshot_count = NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY;
    }
    hash = ndsRendererOwnerHashU32(hash, snapshot_count);
    for (i = 0u; i < NDS_RENDERER_VERTEX_CACHE_SIZE; i++)
    {
        u32 bit = 1u << i;

        if ((input_mask & bit) != 0u)
        {
            const NDSRendererInputVertex *input = &cache->input_vertices[i];

            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->x);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->y);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->z);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->s);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->t);
            hash = ndsRendererOwnerHashU32(hash, input->r);
            hash = ndsRendererOwnerHashU32(hash, input->g);
            hash = ndsRendererOwnerHashU32(hash, input->b);
            hash = ndsRendererOwnerHashU32(hash, input->a);
            hash = ndsRendererOwnerHashU32(
                hash, cache->vertex_matrix_snapshot[i]);
            hash = ndsRendererOwnerHashU32(
                hash, cache->vertex_clip_snapshot[i]);
        }
        if ((transformed_mask & bit) != 0u)
        {
            const NDSRendererClipVertex20p12 *clip =
                &cache->transformed_vertices[i];

            hash = ndsRendererOwnerHashU32(hash, (u32)clip->x);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->y);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->z);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->w);
        }
        if ((color_mask & bit) != 0u)
        {
            hash = ndsRendererOwnerHashU32(hash,
                                           cache->vertex_colors[i]);
        }
    }
    for (i = 0u; i < snapshot_count; i++)
    {
        const NDSRendererMatrixSnapshot *snapshot =
            &cache->matrix_snapshots[i];

        for (row = 0u; row < 4u; row++)
        {
            for (col = 0u; col < 4u; col++)
            {
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)snapshot->matrix.m[row][col]);
            }
        }
        hash = ndsRendererOwnerHashU32(hash, snapshot->generation);
        hash = ndsRendererOwnerHashU32(hash, snapshot->signature);
    }
    return hash;
}

static u32 ndsRendererOwnerHashResolver(
    const NDSFighterDLDrawState *state)
{
    u32 hash = 0u;
    uintptr_t base;
    uintptr_t end;
    size_t bytes;
    size_t i;

    if (state == NULL)
    {
        return 0u;
    }
    if (state->primary_file != NULL)
    {
        hash = ndsRendererOwnerHashU32(hash, 1u);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->asset_id);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->owner_generation);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->data_size);
    }
    else
    {
        hash = ndsRendererOwnerHashU32(hash, 0u);
    }

    base = (uintptr_t)state->segment_e_base;
    end = (uintptr_t)state->segment_e_end;
    if ((base == 0u) || (end <= base))
    {
        return ndsRendererOwnerHashU32(hash, 0u);
    }
    bytes = (size_t)(end - base);
    if (((bytes % sizeof(Gfx)) != 0u) ||
        (ndsFighterDLScanRangeInTaskmanArena(
             state->segment_e_base, bytes) == FALSE))
    {
        hash = ndsRendererOwnerHashU32(hash, 0xffffffffu);
        return ndsRendererOwnerHashU32(hash, (u32)bytes);
    }

    hash = ndsRendererOwnerHashU32(hash, (u32)(bytes / sizeof(Gfx)));
    for (i = 0u; i < (bytes / sizeof(Gfx)); i++)
    {
        const Gfx *command = &state->segment_e_base[i];
        u32 w0 = command->words.w0;
        u32 w1 = command->words.w1;
        u32 op = w0 >> 24;

        hash = ndsRendererOwnerHashU32(
            hash, w0);
        if ((op == NDS_FIGHTER_DL_OP_DL) ||
            (op == NDS_FIGHTER_DL_OP_VTX) ||
            (op == NDS_FIGHTER_DL_OP_MTX) ||
            (op == 0xdcu) || /* F3DEX2 G_MOVEMEM */
            (op == NDS_FIGHTER_DL_OP_SETTIMG))
        {
            const void *pointer = (const void *)(uintptr_t)w1;
            const NDSRelocLoadedFile *loaded = NULL;
            uintptr_t pointer_value = (uintptr_t)pointer;

            if ((pointer_value >= base) && (pointer_value < end))
            {
                hash = ndsRendererOwnerHashU32(hash, 1u);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value - base));
                continue;
            }
            loaded = ndsRelocFindLoadedFileContaining(pointer, 1u);
            if (loaded != NULL)
            {
                hash = ndsRendererOwnerHashU32(hash, 2u);
                hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
                hash = ndsRendererOwnerHashU32(
                    hash, loaded->owner_generation);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value -
                                (uintptr_t)loaded->data));
                continue;
            }
            if ((gSYTaskmanGraphicsHeap.start != NULL) &&
                (gSYTaskmanGraphicsHeap.end != NULL) &&
                (pointer_value >=
                 (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
                (pointer_value <
                 (uintptr_t)gSYTaskmanGraphicsHeap.end))
            {
                hash = ndsRendererOwnerHashU32(hash, 3u);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value -
                                (uintptr_t)gSYTaskmanGraphicsHeap.start));
                continue;
            }
            hash = ndsRendererOwnerHashU32(hash, 4u);
        }
        hash = ndsRendererOwnerHashU32(hash, w1);
    }
    return hash;
}


static void ndsRendererOwnerSnapshotStats(
    const NDSRendererStats *stats, NDSRendererOwnerStatsSnapshot *snapshot)
{
    snapshot->vertex_command_count = stats->vertex_command_count;
    snapshot->source_vertex_count = stats->source_vertex_count;
    snapshot->triangle_command_count = stats->triangle_command_count;
    snapshot->triangle_count = stats->triangle_count;
    snapshot->matrix_command_count = stats->matrix_command_count;
}

static void ndsRendererOwnerAccumulateList(
    NDSRendererProfileOwner owner_id,
    const NDSRelocLoadedFile *loaded,
    const Gfx *dl,
    u32 selected_event,
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    const NDSRendererConfig *config,
    const NDSRendererOwnerStatsSnapshot *before,
    const NDSRendererStats *after)
{
    volatile NDSRendererOwnerProfile *owner;
    u32 dl_offset;
    u32 remaining_commands;

    if ((u32)owner_id >= (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        return;
    }
    owner = &gNdsRendererProfileOwners[(u32)owner_id];
    owner->selected_count++;
    owner->source_command_count += after->command_count;
    owner->vertex_command_count +=
        after->vertex_command_count - before->vertex_command_count;
    owner->source_vertex_count +=
        after->source_vertex_count - before->source_vertex_count;
    owner->triangle_command_count +=
        after->triangle_command_count - before->triangle_command_count;
    owner->triangle_count += after->triangle_count - before->triangle_count;
    /* Every selected list binds its live camera/DObj matrix pair before the
     * source stream runs.  Source MTX commands, when present, are additional
     * changes rather than the whole owner-level matrix census. */
    owner->matrix_change_count += 1u +
        after->matrix_command_count - before->matrix_command_count;

    if ((loaded != NULL) && ((uintptr_t)dl >= (uintptr_t)loaded->data) &&
        ((uintptr_t)dl <
         ((uintptr_t)loaded->data + loaded->data_size)))
    {
        dl_offset = (u32)((uintptr_t)dl - (uintptr_t)loaded->data);
    }
    else if (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) != FALSE)
    {
        dl_offset = (u32)((uintptr_t)dl -
                          (uintptr_t)ndsTaskmanArenaStart());
    }
    else
    {
        dl_offset = (u32)((uintptr_t)dl & 0x00ffffffu);
    }
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, 0x4f574e31u);
    owner->topology_signature = ndsRendererOwnerHashStablePointer(
        owner->topology_signature, (uintptr_t)dl);
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, after->command_count);
    remaining_commands = (config != NULL) ? config->max_commands : 0u;
    owner->topology_signature = ndsRendererOwnerHashDisplayList(
        owner->topology_signature, dl, config, 0u,
        &remaining_commands);
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, remaining_commands);
    owner->selected_event_signature = ndsRendererOwnerHashU32(
        owner->selected_event_signature, selected_event);
    owner->selected_event_signature = ndsRendererOwnerHashU32(
        owner->selected_event_signature, dl_offset);
    if (projection != NULL)
    {
        owner->camera_signature = ndsRendererOwnerHashBytes(
            owner->camera_signature, projection, sizeof(*projection));
    }
    if (modelview != NULL)
    {
        owner->dobj_matrix_signature = ndsRendererOwnerHashBytes(
            owner->dobj_matrix_signature, modelview, sizeof(*modelview));
    }
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->prim_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->env_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->blend_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->texture_combine_w0);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->texture_combine_w1);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, after->light_color_1);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, after->light_color_2);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_x);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_y);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_z);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_image);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_tlut_image);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_scale_s);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_scale_t);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_render_tile);
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererAdapterAccumulateDepth(
    const NDSRendererStats *stats,
    volatile u32 *samples,
    volatile s32 *depth_min,
    volatile s32 *depth_max,
    volatile s32 *w_min,
    volatile s32 *w_max)
{
    if ((stats == NULL) || (samples == NULL) || (depth_min == NULL) ||
        (depth_max == NULL) || (w_min == NULL) || (w_max == NULL) ||
        (stats->hardware_projected_depth_sample_count == 0u))
    {
        return;
    }
    if (*samples == 0u)
    {
        *depth_min = stats->hardware_projected_depth_min;
        *depth_max = stats->hardware_projected_depth_max;
        *w_min = stats->hardware_projected_w_min;
        *w_max = stats->hardware_projected_w_max;
    }
    else
    {
        if (stats->hardware_projected_depth_min < *depth_min)
        {
            *depth_min = stats->hardware_projected_depth_min;
        }
        if (stats->hardware_projected_depth_max > *depth_max)
        {
            *depth_max = stats->hardware_projected_depth_max;
        }
        if (stats->hardware_projected_w_min < *w_min)
        {
            *w_min = stats->hardware_projected_w_min;
        }
        if (stats->hardware_projected_w_max > *w_max)
        {
            *w_max = stats->hardware_projected_w_max;
        }
    }
    *samples += stats->hardware_projected_depth_sample_count;
}
#endif

void ndsRendererAdapterResetDepthDiagnostics(void)
{
    gNdsRendererDepthStageSamples = 0u;
    gNdsRendererDepthFighterP0Samples = 0u;
    gNdsRendererDepthFighterP1Samples = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererAdapterStageOwnerOccurrence = 0u;
    sNdsRendererAdapterStageNextOccurrence = 0u;
    sNdsRendererAdapterStageListOrdinal = 0u;
#endif
}

static sb32 ndsRendererAdapterStatsHasArmedTexture(
    const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            ((stats->texture_image != 0u) ||
             (stats->texture_tlut_image != 0u) ||
             (stats->texture_on != 0u))) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterStatsHasArmedTile(
    const NDSRendererStats *stats)
{
    u32 i;

    if (stats == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_RENDERER_TILE_COUNT; i++)
    {
        const NDSRendererTileState *tile = &stats->texture_tiles[i];

        if ((tile->set_seen != 0u) || (tile->size_seen != 0u) ||
            (tile->line != 0u) || (tile->width != 0u) ||
            (tile->height != 0u))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* THE SOURCE PROC'S OWN prim/env, read back out of the DL head span it wrote.
 *
 * gDPSetPrimColor/gDPSetEnvColor now carry their words (include/PR/gbi.h), and
 * a source effect's proc_display emits exactly those two immediately before it
 * draws its model. Nothing executes the head streams on the DS, so this scans
 * the span the proc appended and hands the values to the effect submit, which
 * seeds them into the renderer's RDP state before the model list runs. Without
 * it every source effect drew in whatever prim/env the previous list left
 * behind -- the stage's, which is what made the shield bubble dark.
 *
 * Bounded and cheap: the shield's span is three commands, and a head whose
 * pointer did not move is skipped outright. */
#define NDS_RENDERER_ADAPTER_DISPLAY_PROC_SCAN_MAX 32u

static const Gfx *sNdsRendererAdapterDisplayProcHeadMark[
    NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterEffectColorMask;
static u32 sNdsRendererAdapterEffectPrimColor;
static u32 sNdsRendererAdapterEffectEnvColor;
/* Colour is per-effect; BLEND STATE IS PER-LAYER AND STICKY. The XLU bracket
 * GObj emits G_RM_AA_ZB_XLU_SURF at display order 0 and the CLD bracket
 * switches to G_RM_CLD_SURF at order 3 (efdisplay.c:15, :5), and every effect
 * drawn in between inherits it -- efManagerShieldProcDisplay emits no render
 * mode of its own at all. So this accumulates across display procs instead of
 * being rebuilt per span, which is what the RDP does. */
static u32 sNdsRendererAdapterEffectOtherModeL;
static u32 sNdsRendererAdapterEffectOtherModeValid;
static u32 sNdsRendererAdapterItemColorMask[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemPrimColor[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemEnvColor[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemOtherModeL[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemOtherModeLValid[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemOtherModeH[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemOtherModeHValid[NDS_RENDERER_STAGE_DL_HEADS];

/* One G_SETOTHERMODE_L packet folded into the running value, using the same
 * shift/length decode as ndsRendererRecordOtherMode (nds_renderer.c:5869) so
 * the two cannot disagree about what a word means. */
static void ndsRendererAdapterFoldDisplayProcOtherModeL(u32 w0, u32 w1)
{
    u32 bits = (w0 & 0xffu) + 1u;
    u32 pos = (w0 >> 8) & 0xffu;
    u32 shift;
    u32 mask;

    if ((bits > 32u) || (pos >= 32u) || ((bits + pos) > 32u))
    {
        return;
    }
    shift = 32u - pos - bits;
    mask = (bits >= 32u) ? 0xffffffffu : (((1u << bits) - 1u) << shift);
    sNdsRendererAdapterEffectOtherModeL =
        (sNdsRendererAdapterEffectOtherModeL & ~mask) | (w1 & mask);
    sNdsRendererAdapterEffectOtherModeValid = 1u;
}

static void ndsRendererAdapterFoldItemOtherMode(u32 *value, u32 *valid,
                                                u32 w0, u32 w1)
{
    u32 bits = (w0 & 0xffu) + 1u;
    u32 pos = (w0 >> 8) & 0xffu;
    u32 shift;
    u32 mask;

    if ((value == NULL) || (valid == NULL) || (bits > 32u) ||
        (pos >= 32u) || ((bits + pos) > 32u))
    {
        return;
    }
    shift = 32u - pos - bits;
    mask = (bits >= 32u) ? 0xffffffffu : (((1u << bits) - 1u) << shift);
    *value = (*value & ~mask) | (w1 & mask);
    *valid = 1u;
}

/* Both ends of a display-proc span must be real main-RAM DL pointers before
 * anything walks between them. On the bounded fast target nothing presents,
 * so `gSYTaskmanDLHeads[]` never receives live DL cursors and holds small
 * non-NULL residue (the P2-3r3 abort read cursor=0x230 end=0x240: the NULL
 * guard passed, `ldr [0x230]` took the MPU data abort, and the nested abort
 * was every "pc=0xfffffffc" corpse the proof harness autopsied). Realtime
 * builds always carry >= 0x02000000 pointers here, so this is behavior-free
 * for every shipping configuration. */
static inline sb32 ndsRendererAdapterDisplayProcSpanValid(const Gfx *cursor,
                                                          const Gfx *end)
{
    return (((uintptr_t)cursor >= 0x02000000u) &&
            ((uintptr_t)end >= 0x02000000u) &&
            (cursor < end)) ? TRUE : FALSE;
}

/* The span one display proc emitted. Folding is idempotent (last writer wins
 * per field), so scanning a span twice -- which the effect path does, once at
 * its own draw and once when the next proc re-marks -- costs nothing. */
static void ndsRendererAdapterScanDisplayProcOtherMode(void)
{
    u32 head;

    for (head = 0u; head < NDS_RENDERER_STAGE_DL_HEADS; head++)
    {
        const Gfx *cursor = sNdsRendererAdapterDisplayProcHeadMark[head];
        const Gfx *end = gSYTaskmanDLHeads[head];
        u32 scanned = 0u;

        if (ndsRendererAdapterDisplayProcSpanValid(cursor, end) == FALSE)
        {
            continue;
        }
        while ((cursor < end) &&
               (scanned < NDS_RENDERER_ADAPTER_DISPLAY_PROC_SCAN_MAX))
        {
            if ((cursor->words.w0 >> 24) ==
                NDS_FIGHTER_DL_OP_SETOTHERMODE_L)
            {
                ndsRendererAdapterFoldDisplayProcOtherModeL(
                    cursor->words.w0, cursor->words.w1);
            }
            cursor++;
            scanned++;
        }
    }
}

void __attribute__((section(".itcm")))
ndsRendererAdapterMarkDisplayProcHeads(void)
{
    u32 i;

    /* Close the previous proc's span before opening this one. The XLU and CLD
     * brackets draw NOTHING, so they never reach the effect submit path and
     * their span would otherwise be overwritten unread -- which is precisely
     * the state the shield depends on. */
    ndsRendererAdapterScanDisplayProcOtherMode();
    for (i = 0u; i < NDS_RENDERER_STAGE_DL_HEADS; i++)
    {
        sNdsRendererAdapterDisplayProcHeadMark[i] = gSYTaskmanDLHeads[i];
    }
}

void ndsRendererAdapterCaptureDisplayProcColors(void)
{
    u32 head;

    sNdsRendererAdapterEffectColorMask = 0u;
    for (head = 0u; head < NDS_RENDERER_STAGE_DL_HEADS; head++)
    {
        const Gfx *cursor = sNdsRendererAdapterDisplayProcHeadMark[head];
        const Gfx *end = gSYTaskmanDLHeads[head];
        u32 scanned = 0u;

        if (ndsRendererAdapterDisplayProcSpanValid(cursor, end) == FALSE)
        {
            continue;
        }
        while ((cursor < end) &&
               (scanned < NDS_RENDERER_ADAPTER_DISPLAY_PROC_SCAN_MAX))
        {
            u32 op = cursor->words.w0 >> 24;

            if (op == NDS_FIGHTER_DL_OP_SETPRIMCOLOR)
            {
                sNdsRendererAdapterEffectPrimColor = cursor->words.w1;
                sNdsRendererAdapterEffectColorMask |= 1u;
            }
            else if (op == NDS_FIGHTER_DL_OP_SETENVCOLOR)
            {
                sNdsRendererAdapterEffectEnvColor = cursor->words.w1;
                sNdsRendererAdapterEffectColorMask |= 2u;
            }
            else if (op == NDS_FIGHTER_DL_OP_SETOTHERMODE_L)
            {
                /* The impact wave sets its own mode at the top of its own
                 * proc (efmanager.c:3286), so the current span matters too. */
                ndsRendererAdapterFoldDisplayProcOtherModeL(
                    cursor->words.w0, cursor->words.w1);
            }
            cursor++;
            scanned++;
        }
    }
    gNdsEffectDLColorMask = sNdsRendererAdapterEffectColorMask;
    gNdsEffectDLPrimColor = sNdsRendererAdapterEffectPrimColor;
    gNdsEffectDLEnvColor = sNdsRendererAdapterEffectEnvColor;
    gNdsEffectDLOtherModeL = sNdsRendererAdapterEffectOtherModeL;
    gNdsEffectDLOtherModeValid = sNdsRendererAdapterEffectOtherModeValid;
}

/* Snapshot only the commands emitted by the current ITEM proc before its DObj
 * draw. BattleShip itDisplayColAnimOPA/XLU writes cycle/render mode + EnvColor
 * immediately before gcDrawDObjTree*, and the DObj hook reaches this function
 * before the proc writes its post-draw restore commands.  The state is kept per
 * DL head because XLU items seed OPA head 0 and XLU head 1 differently. */
void ndsRendererAdapterCaptureItemDisplayProcState(void)
{
    u32 head;

    bzero(sNdsRendererAdapterItemColorMask,
          sizeof(sNdsRendererAdapterItemColorMask));
    bzero(sNdsRendererAdapterItemPrimColor,
          sizeof(sNdsRendererAdapterItemPrimColor));
    bzero(sNdsRendererAdapterItemEnvColor,
          sizeof(sNdsRendererAdapterItemEnvColor));
    bzero(sNdsRendererAdapterItemOtherModeL,
          sizeof(sNdsRendererAdapterItemOtherModeL));
    bzero(sNdsRendererAdapterItemOtherModeLValid,
          sizeof(sNdsRendererAdapterItemOtherModeLValid));
    bzero(sNdsRendererAdapterItemOtherModeH,
          sizeof(sNdsRendererAdapterItemOtherModeH));
    bzero(sNdsRendererAdapterItemOtherModeHValid,
          sizeof(sNdsRendererAdapterItemOtherModeHValid));

    for (head = 0u; head < NDS_RENDERER_STAGE_DL_HEADS; head++)
    {
        const Gfx *cursor = sNdsRendererAdapterDisplayProcHeadMark[head];
        const Gfx *end = gSYTaskmanDLHeads[head];
        u32 scanned = 0u;

        if (ndsRendererAdapterDisplayProcSpanValid(cursor, end) == FALSE)
        {
            continue;
        }
        while ((cursor < end) &&
               (scanned < NDS_RENDERER_ADAPTER_DISPLAY_PROC_SCAN_MAX))
        {
            u32 op = cursor->words.w0 >> 24;

            if (op == NDS_FIGHTER_DL_OP_SETPRIMCOLOR)
            {
                sNdsRendererAdapterItemPrimColor[head] = cursor->words.w1;
                sNdsRendererAdapterItemColorMask[head] |= 1u;
            }
            else if (op == NDS_FIGHTER_DL_OP_SETENVCOLOR)
            {
                sNdsRendererAdapterItemEnvColor[head] = cursor->words.w1;
                sNdsRendererAdapterItemColorMask[head] |= 2u;
            }
            else if (op == NDS_FIGHTER_DL_OP_SETOTHERMODE_L)
            {
                ndsRendererAdapterFoldItemOtherMode(
                    &sNdsRendererAdapterItemOtherModeL[head],
                    &sNdsRendererAdapterItemOtherModeLValid[head],
                    cursor->words.w0, cursor->words.w1);
            }
            else if (op == 0xe3u) /* F3DEX2 G_SETOTHERMODE_H */
            {
                ndsRendererAdapterFoldItemOtherMode(
                    &sNdsRendererAdapterItemOtherModeH[head],
                    &sNdsRendererAdapterItemOtherModeHValid[head],
                    cursor->words.w0, cursor->words.w1);
            }
            cursor++;
            scanned++;
        }
    }
}

void ndsRendererAdapterBeginStageTraversal(void)
{
    bzero(&sNdsRendererAdapterStagePersistentState,
          sizeof(sNdsRendererAdapterStagePersistentState));
    ndsRendererInitStats(&sNdsRendererAdapterStagePersistentStats);
    if ((sNdsFighterDisplayCurrentLightValid != FALSE) &&
        (sNdsFighterDisplayCurrentLightCount != 0u))
    {
        sNdsRendererAdapterStagePersistentStats.light_dir_x =
            sNdsFighterDisplayCurrentLight.l.dir[0];
        sNdsRendererAdapterStagePersistentStats.light_dir_y =
            sNdsFighterDisplayCurrentLight.l.dir[1];
        sNdsRendererAdapterStagePersistentStats.light_dir_z =
            sNdsFighterDisplayCurrentLight.l.dir[2];
        sNdsRendererAdapterStagePersistentStats.light_dir_mask = 1u;
    }
    ndsRendererInitVertexCache(&sNdsRendererAdapterStageVertexCache);
    sNdsRendererAdapterStagePersistentActive = TRUE;
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererAdapterStageOwnerOccurrence =
        sNdsRendererAdapterStageNextOccurrence++;
    sNdsRendererAdapterStageListOrdinal = 0u;
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_state_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_state_hash,
            ndsRendererOwnerHashRuntimeState(
                &sNdsRendererAdapterStagePersistentStats));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_vertex_cache_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_vertex_cache_hash,
            ndsRendererOwnerHashVertexCache(
                &sNdsRendererAdapterStageVertexCache));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_resolver_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_resolver_hash,
            ndsRendererOwnerHashResolver(
                &sNdsRendererAdapterStagePersistentState));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_global_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_global_hash,
            ndsRendererProfileGlobalStateHash());
#endif
}

void ndsRendererAdapterEndStageTraversal(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_state_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_state_hash,
            ndsRendererOwnerHashRuntimeState(
                &sNdsRendererAdapterStagePersistentStats));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_vertex_cache_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_vertex_cache_hash,
            ndsRendererOwnerHashVertexCache(
                &sNdsRendererAdapterStageVertexCache));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_resolver_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_resolver_hash,
            ndsRendererOwnerHashResolver(
                &sNdsRendererAdapterStagePersistentState));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_global_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_global_hash,
            ndsRendererProfileGlobalStateHash());
#endif
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
#endif
    sNdsRendererAdapterStagePersistentActive = FALSE;
}

static s32 ndsRendererAdapterStageValidateRange(const Gfx *dl, size_t bytes,
                                                void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        return FALSE;
    }
    return TRUE;
}

static sb32 ndsRendererAdapterStageDObjDrawable(DObj *dobj, u32 kind)
{
    if (dobj == NULL)
    {
        return FALSE;
    }
    if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0)
    {
        return FALSE;
    }

    switch (kind)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
        return ((dobj->flags & DOBJ_FLAG_NOTEXTURE) == 0) ? TRUE : FALSE;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
        return (dobj->flags == DOBJ_FLAG_NONE) ? TRUE : FALSE;

    default:
        return FALSE;
    }
}

static u32 ndsRendererAdapterMaterialFlags(const MObj *mobj)
{
    u32 flags;

    if (mobj == NULL)
    {
        return MOBJ_FLAG_NONE;
    }
    flags = mobj->sub.flags;
    return (flags == MOBJ_FLAG_NONE) ?
        (MOBJ_FLAG_TEXTURE | 0x20u | MOBJ_FLAG_ALPHA) : flags;
}

static u32 ndsRendererAdapterMaterialPositiveOrOne(s32 value)
{
    return (value <= 0) ? 1u : (u32)value;
}

static void ndsRendererAdapterMaterialLoadBlock(const MObj *mobj,
                                                u32 *texels,
                                                u32 *dxt)
{
    s32 load_texels = 0;
    u32 divisor = 1u;

    if ((mobj == NULL) || (texels == NULL) || (dxt == NULL))
    {
        return;
    }

    switch (mobj->sub.block_siz)
    {
    case G_IM_SIZ_4b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 3) >> 2) -
            1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 16);
        break;
    case G_IM_SIZ_8b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 1) >> 1) -
            1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 8);
        break;
    case G_IM_SIZ_16b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 2) / 8);
        break;
    case G_IM_SIZ_32b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 4) / 8);
        break;
    default:
        break;
    }

    *texels = (load_texels > 0) ? (u32)load_texels : 0u;
    *dxt = (divisor + 0x7ffu) / divisor;
}

static u32 ndsRendererAdapterMaterialCommandCount(const MObj *mobj, u32 flags)
{
    u32 count = 1u;

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj != NULL) &&
        (mobj->sub.palettes != NULL))
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0)
    {
        count++;
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
        {
            count += 5u;
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0)
    {
        count += 2u;
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0)
    {
        count += 2u;
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
    {
        count++;
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
    {
        count++;
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            count += 3u;
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
    {
        count++;
    }
    if ((flags & 0x20u) != 0)
    {
        count++;
    }
    if ((flags & 0x40u) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        count++;
    }
    return count;
}

static sb32 ndsRendererAdapterCountMaterialCommands(DObj *dobj,
                                                    u32 *mobj_count,
                                                    u32 *branch_commands)
{
    MObj *mobj;
    u32 count = 0u;
    u32 commands = 0u;

    if ((dobj == NULL) || (mobj_count == NULL) ||
        (branch_commands == NULL))
    {
        return FALSE;
    }
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        count++;
        if (count > NDS_RENDERER_ADAPTER_MATERIAL_MOBJ_MAX)
        {
            return FALSE;
        }
        commands += ndsRendererAdapterMaterialCommandCount(
            mobj, ndsRendererAdapterMaterialFlags(mobj));
    }
    *mobj_count = count;
    *branch_commands = commands;
    return TRUE;
}

static void ndsRendererAdapterMaterialTextureState(
    const MObj *mobj,
    u32 flags,
    f32 *scau,
    f32 *scav,
    f32 *trau,
    f32 *trav,
    f32 *scrollu,
    f32 *scrollv)
{
    if ((mobj == NULL) || (scau == NULL) || (scav == NULL) ||
        (trau == NULL) || (trav == NULL) || (scrollu == NULL) ||
        (scrollv == NULL) ||
        ((flags & (MOBJ_FLAG_TEXTURE | 0x40u | 0x20u)) == 0))
    {
        return;
    }

    *scau = mobj->sub.scau;
    *scav = mobj->sub.scav;
    *trau = mobj->sub.trau;
    *trav = mobj->sub.trav;
    *scrollu = mobj->sub.scrollu;
    *scrollv = mobj->sub.scrollv;

    if (mobj->sub.unk10 == 1)
    {
        *scau *= 0.5F;
        *trau =
            ((*trau - mobj->sub.unk24) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
        *scrollu =
            ((*scrollu - mobj->sub.unk44) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
    }
}

static u32 ndsRendererAdapterClampU8S32(s32 value)
{
    if (value < 0)
    {
        return 0u;
    }
    return (value > 0xff) ? 0xffu : (u32)value;
}

static u32 ndsRendererAdapterClampU8F32(f32 value)
{
    return ndsRendererAdapterClampU8S32((s32)value);
}

static u32 ndsRendererAdapterPackColor(const SYColorPack *color)
{
    if (color == NULL)
    {
        return 0u;
    }
    return ((u32)color->s.r << 24) |
           ((u32)color->s.g << 16) |
           ((u32)color->s.b << 8) |
           (u32)color->s.a;
}

static const void *ndsRendererAdapterReadPointerEntry(void **items,
                                                      s32 index)
{
    if ((items == NULL) || (index < 0))
    {
        return NULL;
    }
    return items[index];
}

static void ndsRendererAdapterEmitBranchTableCommand(Gfx *cmd,
                                                     const Gfx *branch)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = (NDS_FIGHTER_DL_OP_DL << 24) | (1u << 16);
    cmd->words.w1 = (u32)(uintptr_t)branch;
}

static void ndsRendererAdapterEmitEndDL(Gfx *cmd)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = NDS_FIGHTER_DL_OP_ENDDL << 24;
    cmd->words.w1 = 0u;
}

static void ndsRendererAdapterEmitSync(Gfx *cmd, u32 op)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = op << 24;
    cmd->words.w1 = 0u;
}

static void ndsRendererAdapterEmitTextureImage(Gfx *cmd,
                                               u32 fmt,
                                               u32 siz,
                                               u32 width,
                                               const void *image)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTIMG << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        (((width != 0u) ? (width - 1u) : 0u) & 0x0fffu);
    cmd->words.w1 = (u32)(uintptr_t)image;
}

static void ndsRendererAdapterEmitSetTile(Gfx *cmd,
                                          u32 fmt,
                                          u32 siz,
                                          u32 line,
                                          u32 tmem,
                                          u32 tile,
                                          u32 palette,
                                          u32 cmt,
                                          u32 maskt,
                                          u32 shiftt,
                                          u32 cms,
                                          u32 masks,
                                          u32 shifts)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTILE << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        ((line & 0x01ffu) << 9) |
        (tmem & 0x01ffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((palette & 0x0fu) << 20) |
        ((cmt & 0x3u) << 18) |
        ((maskt & 0x0fu) << 14) |
        ((shiftt & 0x0fu) << 10) |
        ((cms & 0x3u) << 8) |
        ((masks & 0x0fu) << 4) |
        (shifts & 0x0fu);
}

static void ndsRendererAdapterEmitLoadTlut(Gfx *cmd, u32 tile, u32 count)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = NDS_FIGHTER_DL_OP_LOADTLUT << 24;
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((count & 0x03ffu) << 14);
}

static void ndsRendererAdapterEmitMoveWord(Gfx *cmd,
                                           u32 index,
                                           u32 offset,
                                           u32 data)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_MOVEWORD << 24) |
        ((index & 0xffu) << 16) |
        (offset & 0xffffu);
    cmd->words.w1 = data;
}

static Gfx *ndsRendererAdapterEmitLightColor(Gfx *branch_dl,
                                             u32 light,
                                             u32 color)
{
    u32 offset_a = NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_1;
    u32 offset_b = NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_1;

    if (branch_dl == NULL)
    {
        return branch_dl;
    }
    if (light == 2u)
    {
        offset_a = NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_2;
        offset_b = NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_2;
    }
    ndsRendererAdapterEmitMoveWord(branch_dl++,
                                   NDS_RENDERER_ADAPTER_G_MW_LIGHTCOL,
                                   offset_a,
                                   color);
    ndsRendererAdapterEmitMoveWord(branch_dl++,
                                   NDS_RENDERER_ADAPTER_G_MW_LIGHTCOL,
                                   offset_b,
                                   color);
    return branch_dl;
}

static void ndsRendererAdapterEmitPrimColor(Gfx *cmd,
                                            u32 m,
                                            u32 l,
                                            u32 r,
                                            u32 g,
                                            u32 b,
                                            u32 a)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETPRIMCOLOR << 24) |
        ((m & 0xffu) << 8) |
        (l & 0xffu);
    cmd->words.w1 =
        ((r & 0xffu) << 24) |
        ((g & 0xffu) << 16) |
        ((b & 0xffu) << 8) |
        (a & 0xffu);
}

static void ndsRendererAdapterEmitColor(Gfx *cmd,
                                        u32 op,
                                        u32 r,
                                        u32 g,
                                        u32 b,
                                        u32 a)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = op << 24;
    cmd->words.w1 =
        ((r & 0xffu) << 24) |
        ((g & 0xffu) << 16) |
        ((b & 0xffu) << 8) |
        (a & 0xffu);
}

static void ndsRendererAdapterEmitLoadBlock(Gfx *cmd,
                                            u32 tile,
                                            u32 uls,
                                            u32 ult,
                                            u32 lrs,
                                            u32 dxt)
{
    if (cmd == NULL)
    {
        return;
    }
    if (lrs > NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL)
    {
        lrs = NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_LOADBLOCK << 24) |
        ((uls & 0x0fffu) << 12) |
        (ult & 0x0fffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((lrs & 0x0fffu) << 12) |
        (dxt & 0x0fffu);
}

static void ndsRendererAdapterEmitTileSize(Gfx *cmd,
                                           u32 tile,
                                           s32 uls,
                                           s32 ult,
                                           s32 lrs,
                                           s32 lrt)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTILESIZE << 24) |
        (((u32)uls & 0x0fffu) << 12) |
        ((u32)ult & 0x0fffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        (((u32)lrs & 0x0fffu) << 12) |
        ((u32)lrt & 0x0fffu);
}

static void ndsRendererAdapterEmitTexture(Gfx *cmd,
                                          u32 s,
                                          u32 t,
                                          u32 level,
                                          u32 tile,
                                          u32 on)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_TEXTURE << 24) |
        ((level & 0x7u) << 11) |
        ((tile & 0x7u) << 8) |
        ((on & 0x7fu) << 1);
    cmd->words.w1 =
        ((s & 0xffffu) << 16) |
        (t & 0xffffu);
}

static void ndsRendererAdapterNativeMaterialImage(
    u32 fmt, u32 siz, u32 width, const void *image,
    u32 *out_w0, u32 *out_image)
{
    if ((out_w0 == NULL) || (out_image == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTIMG << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        (((width != 0u) ? (width - 1u) : 0u) & 0x0fffu);
    *out_image = (u32)(uintptr_t)image;
}

static void ndsRendererAdapterNativeMaterialTile(
    u32 fmt, u32 siz, u32 line, u32 tmem, u32 tile, u32 palette,
    u32 cmt, u32 maskt, u32 shiftt, u32 cms, u32 masks, u32 shifts,
    u32 *out_w0, u32 *out_w1)
{
    if ((out_w0 == NULL) || (out_w1 == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTILE << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        ((line & 0x01ffu) << 9) |
        (tmem & 0x01ffu);
    *out_w1 =
        ((tile & 0x7u) << 24) |
        ((palette & 0x0fu) << 20) |
        ((cmt & 0x3u) << 18) |
        ((maskt & 0x0fu) << 14) |
        ((shiftt & 0x0fu) << 10) |
        ((cms & 0x3u) << 8) |
        ((masks & 0x0fu) << 4) |
        (shifts & 0x0fu);
}

static void ndsRendererAdapterNativeMaterialTileSize(
    u32 tile, s32 uls, s32 ult, s32 lrs, s32 lrt,
    u32 *out_w0, u32 *out_w1)
{
    if ((out_w0 == NULL) || (out_w1 == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTILESIZE << 24) |
        (((u32)uls & 0x0fffu) << 12) |
        ((u32)ult & 0x0fffu);
    *out_w1 =
        ((tile & 0x7u) << 24) |
        (((u32)lrs & 0x0fffu) << 12) |
        ((u32)lrt & 0x0fffu);
}

static sb32 ndsRendererAdapterBuildNativeMaterialSnapshot(
    MObj *mobj, NDSRendererNativeMaterial *out, sb32 advance_texture_ids,
    s32 *out_curr, s32 *out_next)
{
    u32 flags;
    f32 scau = 0.0F;
    f32 scav = 0.0F;
    f32 trau = 0.0F;
    f32 trav = 0.0F;
    f32 scrollu = 0.0F;
    f32 scrollv = 0.0F;
    s32 uls;
    s32 ult;
    s32 s;
    s32 t;
    s32 texture_id_curr;
    s32 texture_id_next;

    if ((mobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    texture_id_curr = mobj->texture_id_curr;
    texture_id_next = mobj->texture_id_next;
    bzero(out, sizeof(*out));
    flags = ndsRendererAdapterMaterialFlags(mobj);
    out->command_count = 1u; /* ENDDL */
    ndsRendererAdapterMaterialTextureState(
        mobj, flags, &scau, &scav, &trau, &trav, &scrollu, &scrollv);

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj->sub.palettes != NULL))
    {
        const void *palette = ndsRendererAdapterReadPointerEntry(
            mobj->sub.palettes, (s32)mobj->palette_id);

        if (palette != NULL)
        {
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE;
            ndsRendererAdapterNativeMaterialImage(
                G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u, palette,
                &out->palette_image_w0, &out->palette_image);
            out->command_count++;
        }
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.palettes, (s32)mobj->palette_id),
            &out->palette_image_w0, &out->palette_image);
        out->command_count++;
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0u)
        {
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT;
            ndsRendererAdapterNativeMaterialTile(
                G_IM_FMT_RGBA, G_IM_SIZ_4b, 0u, 0x0100u, 5u, 0u,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                &out->palette_tile_w0, &out->palette_tile_w1);
            out->palette_tlut_w1 =
                (5u << 24) |
                (((mobj->sub.siz == G_IM_SIZ_8b) ? 0xffu : 0x0fu) << 14);
            out->sync_count += 3u;
            out->command_count += 5u;
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LIGHT1;
        out->light1 = ndsRendererAdapterPackColor(&mobj->sub.light1color);
        out->command_count += 2u;
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LIGHT2;
        out->light2 = ndsRendererAdapterPackColor(&mobj->sub.light2color);
        out->command_count += 2u;
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0u)
    {
        u32 level;

        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PRIM;
        if ((flags & MOBJ_FLAG_FRAC) != 0u)
        {
            s32 trunc = (s32)mobj->lfrac;

            level = ndsRendererAdapterClampU8F32(
                (mobj->lfrac - (f32)trunc) * 256.0F);
            texture_id_curr = trunc;
            texture_id_next = trunc + 1;
        }
        else
        {
            level = ndsRendererAdapterClampU8F32(mobj->lfrac * 255.0F);
        }
        out->prim_w0 =
            (NDS_FIGHTER_DL_OP_SETPRIMCOLOR << 24) |
            (((u32)mobj->sub.prim_m & 0xffu) << 8) |
            (level & 0xffu);
        out->prim_w1 = ndsRendererAdapterPackColor(&mobj->sub.primcolor);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_ENV;
        out->env_color = ndsRendererAdapterPackColor(&mobj->sub.envcolor);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_BLEND;
        out->blend_color =
            ndsRendererAdapterPackColor(&mobj->sub.blendcolor);
        out->command_count++;
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0u)
    {
        u32 block_siz = (mobj->sub.block_siz == G_IM_SIZ_32b) ?
            G_IM_SIZ_32b : G_IM_SIZ_16b;

        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            mobj->sub.block_fmt, block_siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, texture_id_next),
            &out->block_image_w0, &out->block_image);
        out->command_count++;
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0u)
        {
            u32 texels = 0u;
            u32 dxt = 0u;

            ndsRendererAdapterMaterialLoadBlock(mobj, &texels, &dxt);
            if (texels > NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL)
            {
                texels = NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL;
            }
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LOAD_BLOCK;
            out->load_block_w0 = NDS_FIGHTER_DL_OP_LOADBLOCK << 24;
            out->load_block_w1 =
                (6u << 24) | ((texels & 0x0fffu) << 12) |
                (dxt & 0x0fffu);
            out->sync_count += 2u;
            out->command_count += 3u;
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            mobj->sub.fmt, mobj->sub.siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, texture_id_curr),
            &out->current_image_w0, &out->current_image);
        out->command_count++;
    }
    if ((flags & 0x20u) != 0u)
    {
        if (mobj->sub.unk10 == 2)
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0C * trau) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0E * trav) / scav) * 4.0F) : 0;
            if (uls < 0) { uls = 0; }
            if (ult < 0) { ult = 0; }
        }
        else
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((((f32)mobj->sub.unk0C * trau) +
                         (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((((1.0F - scav) - trav) *
                          (f32)mobj->sub.unk0E) +
                         (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        }
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE;
        ndsRendererAdapterNativeMaterialTileSize(
            NDS_RENDERER_ADAPTER_G_TX_RENDERTILE, uls, ult,
            (((s32)mobj->sub.unk0C - 1) << 2) + uls,
            (((s32)mobj->sub.unk0E - 1) << 2) + ult,
            &out->render_tile_size_w0, &out->render_tile_size_w1);
        out->command_count++;
    }
    if ((flags & 0x40u) != 0u)
    {
        uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
            (s32)(((((f32)mobj->sub.unk38 * scrollu) +
                     (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
        ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
            (s32)((((((1.0F - scav) - scrollv) *
                      (f32)mobj->sub.unk3A) +
                     (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_SCROLL_TILE_SIZE;
        ndsRendererAdapterNativeMaterialTileSize(
            1u, uls, ult,
            (((s32)mobj->sub.unk38 - 1) << 2) + uls,
            (((s32)mobj->sub.unk3A - 1) << 2) + ult,
            &out->scroll_tile_size_w0, &out->scroll_tile_size_w1);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0u)
    {
        if (mobj->sub.unk10 == 2)
        {
            s = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0C * 64.0F) / scau) : 0;
            t = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0E * 64.0F) / scav) : 0;
        }
        else
        {
            s = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scau) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scau) : 0;
            t = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scav) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scav) : 0;
        }
        if (s > 0xffff) { s = 0xffff; }
        if (t > 0xffff) { t = 0xffff; }
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_TEXTURE;
        out->texture_w0 =
            (NDS_FIGHTER_DL_OP_TEXTURE << 24) |
            (NDS_RENDERER_ADAPTER_G_TX_RENDERTILE << 8) |
            (NDS_RENDERER_ADAPTER_G_ON << 1);
        out->texture_w1 =
            (((u32)s & 0xffffu) << 16) | ((u32)t & 0xffffu);
        out->command_count++;
    }
    if (out_curr != NULL)
    {
        *out_curr = texture_id_curr;
    }
    if (out_next != NULL)
    {
        *out_next = texture_id_next;
    }
    if (advance_texture_ids != FALSE)
    {
        mobj->texture_id_curr = texture_id_curr;
        mobj->texture_id_next = texture_id_next;
    }
    return TRUE;
}

#if NDS_TICK_HUD
/* CYCLE 98 -- is the fighter material snapshot re-deriving a constant?
 *
 * BuildNativeMaterialSnapshot has no cache, so unlike the owner validate there
 * is no hit/miss to count. The equivalent question is content invariance: hash
 * the snapshot it just produced and compare it with the last snapshot produced
 * for that same MObj. `Same` is then the number of calls that recomputed a
 * value they had already computed, which is exactly the size of the deletion.
 *
 * Direct-mapped and O(1) on purpose. A linear table would search up to 128
 * entries per call at tens of calls per fighter per frame -- instrument cost
 * inside the very span being priced. Collisions are not hidden: a slot holding
 * a different key is counted as Evict, so a thrashing table reads as thrash
 * rather than as "every material varies".
 *
 * Charter 3.12: keyed on MObj pointers, which are arena addresses valid only
 * within a scene. The census only has to survive the match it measures, and P1
 * boots straight into one. */
#define NDS_FTR_PRE_MAT_TABLE_SIZE 256u

static const MObj *sNdsFtrPreMatKey[NDS_FTR_PRE_MAT_TABLE_SIZE];
static u32 sNdsFtrPreMatHash[NDS_FTR_PRE_MAT_TABLE_SIZE];

static void ndsFtrPreMaterialCensus(const MObj *mobj,
                                    const NDSRendererNativeMaterial *out)
{
    const u32 *words = (const u32 *)(const void *)out;
    u32 hash = 2166136261u;
    u32 i;
    u32 index;

    /* The whole struct is bzero'd at the top of the snapshot builder, so any
     * padding is deterministic and a word-wise hash is stable. */
    for (i = 0u; i < (u32)(sizeof(*out) / sizeof(u32)); i++)
    {
        hash ^= words[i];
        hash *= 16777619u;
    }
    /* Multiplicative rather than a low-bit mask: MObjs come out of the taskman
     * arena at a fixed stride, so their low address bits are the least
     * distinguishing ones they have. */
    index = ((u32)(uintptr_t)mobj * 2654435761u) >> 24;
    gNdsFtrPreMatCalls++;
    if (sNdsFtrPreMatKey[index] == mobj)
    {
        if (sNdsFtrPreMatHash[index] == hash)
        {
            gNdsFtrPreMatSame++;
        }
        else
        {
            gNdsFtrPreMatVariant++;
            sNdsFtrPreMatHash[index] = hash;
        }
        return;
    }
    if (sNdsFtrPreMatKey[index] != NULL)
    {
        gNdsFtrPreMatEvict++;
    }
    else
    {
        gNdsFtrPreMatNew++;
    }
    sNdsFtrPreMatKey[index] = mobj;
    sNdsFtrPreMatHash[index] = hash;
}
#endif

static sb32 ndsRendererAdapterBuildNativeMaterial(
    MObj *mobj, NDSRendererNativeMaterial *out)
{
#if NDS_TICK_HUD
    sb32 built = ndsRendererAdapterBuildNativeMaterialSnapshot(
        mobj, out, TRUE, NULL, NULL);

    if (built != FALSE)
    {
        ndsFtrPreMaterialCensus(mobj, out);
    }
    return built;
#else
    return ndsRendererAdapterBuildNativeMaterialSnapshot(
        mobj, out, TRUE, NULL, NULL);
#endif
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
/* P2-4n1 step 6: every per-segment fact below reads the active stage's
 * capture row (renderer_adapter_matrix.c). Dream Land's rows are the switch,
 * arrays and ternary that used to live here, value for value. */
static GObj *ndsRendererAdapterNativeStageSegmentGObj(u32 segment_index)
{
    const NDSRendererAdapterNativeStageCaptureSegment *row =
        ndsRendererAdapterNativeStageCaptureRow(segment_index);

    if (row == NULL)
    {
        return NULL;
    }
    switch (row->source)
    {
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER:
        return (row->index < 4u) ? gGRCommonLayerGObjs[row->index] : NULL;
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_PUPUPU_MAP:
        return (row->index < 4u) ?
            gGRCommonStruct.pupupu.map_gobj[row->index] : NULL;
    default:
        return NULL;
    }
}

static u32 ndsRendererAdapterNativeStageSegmentLink(u32 segment_index)
{
    const NDSRendererAdapterNativeStageCaptureSegment *row =
        ndsRendererAdapterNativeStageCaptureRow(segment_index);

    return (row != NULL) ? row->link : 0xffu;
}

static sb32 ndsRendererAdapterNativeStageProcMatches(
    u32 segment_index, GObj *gobj)
{
    if ((gobj == NULL) || (gobj->proc_display == NULL))
    {
        return FALSE;
    }
    {
        static void (*const layer_procs[4])(GObj *) = {
            grDisplayLayer0PriProcDisplay, grDisplayLayer1PriProcDisplay,
            grDisplayLayer2PriProcDisplay, grDisplayLayer3PriProcDisplay
        };
        const NDSRendererAdapterNativeStageCaptureSegment *row =
            ndsRendererAdapterNativeStageCaptureRow(segment_index);

        if ((row == NULL) || (row->layer >= 4u))
        {
            return FALSE;
        }
        return (gobj->proc_display == layer_procs[row->layer]) ? TRUE : FALSE;
    }
}

static sb32 ndsRendererAdapterNativeStageGObjLinked(GObj *target, u32 link)
{
    GObj *gobj;
    u32 guard = 0u;

    if ((target == NULL) || (link >= GC_COMMON_MAX_DLLINKS))
    {
        return FALSE;
    }
    for (gobj = gGCCommonDLLinks[link];
         (gobj != NULL) && (guard < 256u);
         gobj = gobj->dl_link_next, guard++)
    {
        if (gobj == target)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsRendererAdapterNativeStageLayer0OrderMatches(
    GObj *const *segments)
{
    GObj *gobj;
    u32 next = 0u;
    u32 guard = 0u;
    const u32 layer0 = ndsRendererAdapterNativeStageLayer0Count();

    if ((segments == NULL) || (layer0 == 0u))
    {
        return FALSE;
    }
    for (gobj = gGCCommonDLLinks[4];
         (gobj != NULL) && (guard < 256u) && (next < layer0);
         gobj = gobj->dl_link_next, guard++)
    {
        if (gobj == segments[next])
        {
            next++;
        }
    }
    return (next == layer0) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterNativeStageTransformFlags(
    const DObj *dobj, u16 *out_flags)
{
    if ((dobj == NULL) || (out_flags == NULL))
    {
        return FALSE;
    }
    if ((dobj->xobjs_num == 1u) && (dobj->xobjs[0] != NULL) &&
        (dobj->xobjs[0]->kind == nGCMatrixKindTraRotRpyRSca))
    {
        *out_flags = 0u;
        return TRUE;
    }
    if ((dobj->xobjs_num == 2u) && (dobj->xobjs[0] != NULL) &&
        (dobj->xobjs[1] != NULL) &&
        (dobj->xobjs[0]->kind == nGCMatrixKindTra))
    {
        if (dobj->xobjs[1]->kind == nGCMatrixKind48)
        {
            *out_flags = 2u;
            return TRUE;
        }
        if (dobj->xobjs[1]->kind == nGCMatrixKind46)
        {
            *out_flags = 4u;
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsRendererAdapterCollectNativeStageDObjs(
    DObj *dobj, u32 owner, u16 parent_index, u8 depth,
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    for (; dobj != NULL; dobj = dobj->sib_next)
    {
        NDSRendererNativeStageDObj *live;
        u32 index;
        u16 transform_flags;

        if ((workspace->dobj_count >= NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT) ||
            (depth > 31u) || ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0u) ||
            (ndsRendererAdapterNativeStageTransformFlags(
                 dobj, &transform_flags) == FALSE))
        {
            return FALSE;
        }
        index = workspace->dobj_count++;
        workspace->dobjs[index] = dobj;
        live = &workspace->live_dobjs[index];
        live->identity = dobj;
        live->parent_index = parent_index;
        live->transform_flags = transform_flags;
        live->owner = (u8)owner;
        live->depth = depth;
        if (dobj->dv != NULL)
        {
            u32 binding = workspace->binding_count++;
            if ((binding >= NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT) ||
                ((dobj->flags & DOBJ_FLAG_NOTEXTURE) != 0u))
            {
                return FALSE;
            }
            live->binding_index = (u16)binding;
            workspace->binding_dobjs[binding] = dobj;
            workspace->binding_display_lists[binding] = dobj->dv;
        }
        else
        {
            live->binding_index = 0xffffu;
        }
        if ((dobj->child != NULL) &&
            (ndsRendererAdapterCollectNativeStageDObjs(
                 dobj->child, owner, (u16)index, (u8)(depth + 1u),
                 workspace) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static u32 ndsRendererAdapterNativeStageStampValue(u32 stamp, uintptr_t value)
{
    stamp ^= (u32)value;
    stamp *= 16777619u;
    stamp ^= stamp >> 16;
    return stamp;
}

static sb32 ndsRendererAdapterBuildNativeStageTopologyStamp(
    NDSRendererAdapterNativeStageWorkspace *workspace,
    u32 generation,
    u32 *out_stamp)
{
    u32 stamp = 2166136261u;
    u32 i;

    if ((workspace == NULL) || (out_stamp == NULL) || (generation == 0u) ||
        (workspace->dobj_count !=
         ndsRendererAdapterNativeStageActiveDObjCount()) ||
        (workspace->binding_count !=
         ndsRendererAdapterNativeStageActiveBindingCount()))
    {
        return FALSE;
    }
    stamp = ndsRendererAdapterNativeStageStampValue(stamp, generation);
    for (i = 0u; i < ndsRendererAdapterNativeStageActiveAssetCount(); i++)
    {
        NDSRelocLoadedFile *loaded = workspace->loaded[i];

        if ((loaded == NULL) || (loaded->data == NULL) ||
            (loaded->owner_generation != generation))
        {
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)loaded);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)loaded->data);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->asset_id);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->data_size);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->owner_generation);
    }
    for (i = 0u; i < ndsRendererAdapterNativeStageActiveSegmentCount(); i++)
    {
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        if ((gobj == NULL) || (gobj != workspace->segments[i]) ||
            (DObjGetStruct(gobj) == NULL) ||
            ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id != ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE) ||
            (ndsRendererAdapterNativeStageGObjLinked(
                 gobj, gobj->dl_link_id) == FALSE))
        {
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)gobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)DObjGetStruct(gobj));
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, gobj->flags);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, gobj->dl_link_id);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)gobj->proc_display);
    }
    if (ndsRendererAdapterNativeStageLayer0OrderMatches(
            workspace->segments) == FALSE)
    {
        return FALSE;
    }
    for (i = 0u; i < workspace->dobj_count; i++)
    {
        DObj *dobj = workspace->dobjs[i];
        NDSRendererNativeStageDObj *live = &workspace->live_dobjs[i];
        u16 transform_flags;
        u32 xobj_index;

        if ((dobj == NULL) || (live->identity != dobj) ||
            (ndsRendererAdapterNativeStageTransformFlags(
                 dobj, &transform_flags) == FALSE) ||
            (transform_flags != live->transform_flags))
        {
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->parent_gobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->parent);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->child);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->sib_next);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->sib_prev);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->dv);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->mobj);
        stamp = ndsRendererAdapterNativeStageStampValue(stamp, dobj->flags);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, dobj->xobjs_num);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->parent_index);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->binding_index);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->owner);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->depth);
        for (xobj_index = 0u; xobj_index < dobj->xobjs_num; xobj_index++)
        {
            XObj *xobj = dobj->xobjs[xobj_index];

            if (xobj == NULL)
            {
                return FALSE;
            }
            stamp = ndsRendererAdapterNativeStageStampValue(
                stamp, (uintptr_t)xobj);
            stamp = ndsRendererAdapterNativeStageStampValue(
                stamp, xobj->kind);
        }
    }
    for (i = 0u; i < workspace->binding_count; i++)
    {
        if ((workspace->binding_dobjs[i] == NULL) ||
            (workspace->binding_display_lists[i] == NULL) ||
            (workspace->binding_dobjs[i]->dv !=
             workspace->binding_display_lists[i]))
        {
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)workspace->binding_dobjs[i]);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)workspace->binding_display_lists[i]);
    }
    *out_stamp = (stamp != 0u) ? stamp : 1u;
    return TRUE;
}

#if NDS_TASK44_STAGE_STEADY
/* Task 44 item 3: the cheap half of stage admission.
 *
 * The asset-mutation generation proves the four reloc payloads have not been
 * replaced or unloaded; it says nothing about the scene graph that hangs off
 * them. These eight checks cover the graph mutations the stage owner must fail
 * closed on — a segment GObj swapped, hidden, relinked, or given a different
 * display proc — and every one of them is a direct global load. What steady
 * state no longer pays for is the O(n) work: the four loaded-file table scans,
 * the eight DL-link list walks, the two layer-0 order walks, and the 57-DObj /
 * 42-binding stamp rebuild with its per-DObj transform-flag derivation. Any
 * failure here drops through to exactly that full validation. */
static sb32 ndsRendererAdapterNativeStageSegmentsUnchanged(
    const NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 i;

    for (i = 0u; i < ndsRendererAdapterNativeStageActiveSegmentCount(); i++)
    {
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        if ((gobj == NULL) || (gobj != workspace->segments[i]) ||
            ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id !=
             ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (DObjGetStruct(gobj) != workspace->task44_segment_roots[i]) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
}
#endif

static sb32 ndsRendererAdapterCollectNativeStageTopology(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    const u32 segment_count = ndsRendererAdapterNativeStageActiveSegmentCount();
    u32 i;

    if (segment_count == 0u)
    {
        return FALSE;
    }
    /* Rows past the active count must read as absent to every later walk. */
    for (i = segment_count; i < NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT; i++)
    {
        workspace->segments[i] = NULL;
#if NDS_TASK44_STAGE_STEADY
        workspace->task44_segment_roots[i] = NULL;
#endif
    }
    for (i = 0u; i < segment_count; i++)
    {
        const NDSRendererAdapterNativeStageCaptureSegment *row =
            ndsRendererAdapterNativeStageCaptureRow(i);
        u32 first_dobj = workspace->dobj_count;
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        workspace->segments[i] = gobj;
        if ((row == NULL) || (gobj == NULL) ||
            ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id != ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE) ||
            (ndsRendererAdapterNativeStageGObjLinked(
                 gobj, gobj->dl_link_id) == FALSE) ||
            (ndsRendererAdapterCollectNativeStageDObjs(
                 DObjGetStruct(gobj), row->owner,
                 0xffffu, 0u, workspace) == FALSE) ||
            ((workspace->dobj_count - first_dobj) != row->dobj_count))
        {
            return FALSE;
        }
#if NDS_TASK44_STAGE_STEADY
        workspace->task44_segment_roots[i] = DObjGetStruct(gobj);
#endif
    }
    return ((workspace->dobj_count ==
             ndsRendererAdapterNativeStageActiveDObjCount()) &&
            (workspace->binding_count ==
             ndsRendererAdapterNativeStageActiveBindingCount()) &&
            (ndsRendererAdapterNativeStageLayer0OrderMatches(
                 workspace->segments) != FALSE)) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterPrepareNativeStageBindingMatrix(
    CObj *cobj, NDSRendererAdapterNativeStageWorkspace *workspace,
    u32 binding_index)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    const NDSRendererMatrix20p12 *projection_ptr;
    const NDSRendererMatrix20p12 *modelview_ptr;

    ndsRendererAdapterPrepareInitialMatrices(
        workspace->binding_dobjs[binding_index], cobj, TRUE,
        &projection, &projection_ptr, &modelview, &modelview_ptr);
    /* MVP recalc returns the completed product in modelview with no separate
     * projection. ComposeNativeRootMatrix explicitly supports that shape. */
    if ((projection_ptr == NULL) && (modelview_ptr == NULL))
    {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        gNdsRendererTask36AdapterRejectReason = 52u;
#endif
        return FALSE;
    }
#if !NDS_TASK36_HW_COMPOSE
    if ((binding_index != 0u) && (projection_ptr != NULL) &&
        (memcmp(&workspace->projection, projection_ptr,
                sizeof(workspace->projection)) != 0))
    {
        return FALSE;
    }
#endif
    if (ndsRendererAdapterComposeNativeRootMatrix(
            modelview_ptr, projection_ptr,
            &workspace->binding_composed[binding_index]) == FALSE)
    {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        gNdsRendererTask36AdapterRejectReason = 54u;
#endif
        return FALSE;
    }
    if ((binding_index == 0u) && (projection_ptr != NULL))
    {
        ndsRendererMatrixCopy20p12(&workspace->projection, projection_ptr);
    }
    return TRUE;
}

static sb32 ndsRendererAdapterPrepareNativeStageMatrices(
    CObj *cobj, NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;

#if NDS_TASK36_HW_COMPOSE
    {
        if (ndsRendererAdapterBuildTask36StageCameraMatrices(
                cobj, &workspace->projection,
                &workspace->camera_modelview) == FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererTask36AdapterRejectReason = 51u;
#endif
            return FALSE;
        }
    }
#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererTask36ObservedDynamicMaskLo = 0u;
    gNdsRendererTask36ObservedDynamicMaskHi = 0u;
    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        NDSRendererMatrix20p12 current_world;

        if ((ndsRendererAdapterBuildDObjWorldMatrixUncached(
                 workspace->binding_dobjs[binding_index],
                 &current_world) == FALSE) ||
            (memcmp(&current_world, &workspace->binding_world[binding_index],
                    sizeof(current_world)) != 0))
        {
            if (binding_index < 32u)
            {
                gNdsRendererTask36ObservedDynamicMaskLo |= 1u << binding_index;
            }
            else
            {
                gNdsRendererTask36ObservedDynamicMaskHi |=
                    1u << (binding_index - 32u);
            }
            if ((workspace->task36_runtime_rigid_mask &
                 ((u64)1u << binding_index)) != 0u)
            {
                gNdsRendererTask36RigidConstancyMismatchCount++;
                return FALSE;
            }
        }
    }
#endif
#endif

#if NDS_TASK36_HW_COMPOSE && NDS_TASK44_STAGE_STEADY
    /* Task 44 item 4: steady state composes exactly the 16 dynamic bindings.
     * The dense list is trusted only while the runtime rigid mask still equals
     * the captured one — a rigid-constancy fallback drops the mask to 0, which
     * makes every binding dynamic and must take the full scan. */
    if ((workspace->task44_binding_lists_valid != FALSE) &&
        (workspace->task36_runtime_rigid_mask ==
         ndsRendererNativeStageRigidBindingMask()))
    {
        u32 dynamic_slot;
#if NDS_R2_STAGE_VIEWPROJ
        /* R2-02 E7. Composing the 16 dynamic bindings the long way cost 54,901
         * ticks/frame, 44.6% of the stage preflight, and almost none of it was
         * arithmetic. Per binding the old path ran the camera cache lookup and
         * three 64-byte matrix copies -- and MTXCOPY is a `bl memcpy` here, see
         * the Task 86 note on NDSRendererMatrix20p12 -- to rebuild operands
         * that are identical for all 16.
         *
         * This is exact, not an approximation. For the battle camera
         * ndsRendererAdapterBuildCameraMatrices leaves modelview_valid FALSE
         * and returns projection = MtxMul(lookat, persp), so the old compose
         * was world x (lookat x persp) with the modelview a plain copy of the
         * world -- one multiply, never two.
         * ndsRendererAdapterBuildTask36StageCameraMatrices derives
         * camera_modelview and projection from the same syMatrixLookAtReflect
         * and syMatrixPerspFast calls on the same CObj, so view_projection
         * reproduces that product bit-for-bit rather than reassociating it.
         * Verified against the pre-E7 arm: binding_composed[] identical across
         * all 42 bindings at frames 260/420/500/700/1100/1700, spanning the
         * camera's full range of motion. No fidelity budget is spent. */
        NDSRendererMatrix20p12 view_projection;

        ndsRendererMtxMul20p12(&workspace->camera_modelview,
                               &workspace->projection, &view_projection);
        for (dynamic_slot = 0u;
             dynamic_slot < workspace->task44_dynamic_binding_count;
             dynamic_slot++)
        {
            u32 vp_binding = workspace->task44_dynamic_bindings[dynamic_slot];
            DObj *vp_dobj = workspace->binding_dobjs[vp_binding];
            NDSRendererMatrix20p12 vp_world;

            /* An mvp-recalc (0x47 or the kind-44 billboard) rewrites the pair
             * after composition, so those bindings keep the exact original
             * path. */
            if ((vp_dobj == NULL) ||
                (ndsRendererAdapterDirectMvpRecalcKind(vp_dobj) != 0u) ||
                /* BUGS.md row 1, 2026-08-13. These are the stage's DYNAMIC
                 * bindings, and several of them are animated (Whispy eyes and
                 * mouth plus the flower actors). Slice 44 used to pass
                 * allow_stale=TRUE on seven of eight frames, before the source
                 * key was even checked. The source DObj therefore advanced at
                 * 30 Hz while its cached world matrix could remain frozen for
                 * eight presented frames. FALSE does not force a rebuild: the
                 * persistent cache still reuses an unchanged source key and
                 * parent generation. It only forbids blind stale reuse. Keep
                 * the stride on the separately-proven rigid-binding guard, not
                 * on dynamic world transforms. */
                (ndsRendererAdapterBuildPersistentStageWorldMatrix(
                     vp_dobj, &vp_world, FALSE) == FALSE))
            {
                if (ndsRendererAdapterPrepareNativeStageBindingMatrix(
                        cobj, workspace, vp_binding) == FALSE)
                {
                    return FALSE;
                }
                continue;
            }
            ndsRendererMtxMul20p12(&vp_world, &view_projection,
                                   &workspace->binding_composed[vp_binding]);
        }
        return TRUE;
#else

        for (dynamic_slot = 0u;
             dynamic_slot < workspace->task44_dynamic_binding_count;
             dynamic_slot++)
        {
            if (ndsRendererAdapterPrepareNativeStageBindingMatrix(
                    cobj, workspace,
                    workspace->task44_dynamic_bindings[dynamic_slot]) == FALSE)
            {
                return FALSE;
            }
        }
        return TRUE;
#endif
    }
#endif
    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
#if NDS_TASK36_HW_COMPOSE
        if ((workspace->task36_runtime_rigid_mask &
             ((u64)1u << binding_index)) != 0u)
        {
            continue;
        }
#endif
        if (ndsRendererAdapterPrepareNativeStageBindingMatrix(
                cobj, workspace, binding_index) == FALSE)
        {
            return FALSE;
        }
    }
    return TRUE;
}

#if NDS_TASK36_HW_COMPOSE
static sb32 ndsRendererAdapterCaptureTask36StageWorld(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;
    const u64 rigid_mask = ndsRendererNativeStageRigidBindingMask();

    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        if (ndsRendererAdapterBuildDObjWorldMatrixUncached(
                workspace->binding_dobjs[binding_index],
                &workspace->binding_world[binding_index]) == FALSE)
        {
            return FALSE;
        }
        if (((rigid_mask &
              ((u64)1u << binding_index)) != 0u) &&
            (ndsRendererAdapterCaptureStageWorldSourceKey(
                 workspace->binding_dobjs[binding_index],
                 &workspace->task36_rigid_source_keys[binding_index]) ==
             FALSE))
        {
            return FALSE;
        }
    }
    workspace->task36_runtime_rigid_mask =
        rigid_mask;
#if NDS_TASK44_STAGE_STEADY
    workspace->task44_rigid_binding_count = 0u;
    workspace->task44_dynamic_binding_count = 0u;
    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        if ((rigid_mask &
             ((u64)1u << binding_index)) != 0u)
        {
            workspace->task44_rigid_bindings[
                workspace->task44_rigid_binding_count++] = (u8)binding_index;
        }
        else
        {
            workspace->task44_dynamic_bindings[
                workspace->task44_dynamic_binding_count++] = (u8)binding_index;
        }
    }
    workspace->task44_binding_lists_valid = TRUE;
#endif
    return TRUE;
}

static void ndsRendererAdapterValidateTask36StageWorld(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;
    const u64 rigid_mask = ndsRendererNativeStageRigidBindingMask();
#if NDS_TASK44_STAGE_STEADY
    u32 rigid_slot;
#endif

#if NDS_R2_STAGE_VALIDATE_STRIDE
    /* Slice 44. Demotion is one-way within a topology: a binding that stopped
     * being rigid does not become rigid again until capture re-arms the mask at
     * ndsRendererAdapterCaptureTask36StageWorld. Before the stride the mask was
     * rebuilt from the constant here every frame and cleared again by the sweep,
     * which was equivalent only because the sweep was complete. With a partial
     * sweep, re-arming would resurrect a binding this frame's slice did not
     * look at. */
    if (workspace->task36_runtime_rigid_mask == 0u)
    {
        return;
    }
#endif
    workspace->task36_runtime_rigid_mask =
        rigid_mask;
#if NDS_TASK44_STAGE_STEADY
    /* Task 44 item 4: walk the 26 rigid bindings directly. The list is only
     * consulted when capture built it for this topology; otherwise fall back
     * to the mask scan below so a torn workspace can never skip validation. */
    if (workspace->task44_binding_lists_valid != FALSE)
    {
        for (rigid_slot = 0u;
             rigid_slot < workspace->task44_rigid_binding_count;
             rigid_slot++)
        {
#if NDS_R2_STAGE_VALIDATE_STRIDE
            if ((rigid_slot % NDS_R2_STAGE_VALIDATE_STRIDE) !=
                workspace->slice44_validate_cursor)
            {
                gNdsR2Slice44RigidSkips++;
                continue;
            }
            gNdsR2Slice44RigidChecks++;
#endif
            binding_index = workspace->task44_rigid_bindings[rigid_slot];
            if (ndsRendererAdapterStageWorldSourceKeyMatches(
                    workspace->binding_dobjs[binding_index],
                    &workspace->task36_rigid_source_keys[binding_index]) ==
                FALSE)
            {
                workspace->task36_runtime_rigid_mask = 0u;
#if NDS_RENDERER_PROFILE_LEVEL == 1
                gNdsRendererTask36RigidConstancyMismatchCount++;
#endif
                return;
            }
        }
        return;
    }
#endif
    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        if ((rigid_mask &
             ((u64)1u << binding_index)) == 0u)
        {
            continue;
        }
        if (ndsRendererAdapterStageWorldSourceKeyMatches(
                workspace->binding_dobjs[binding_index],
                &workspace->task36_rigid_source_keys[binding_index]) ==
            FALSE)
        {
            workspace->task36_runtime_rigid_mask = 0u;
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererTask36RigidConstancyMismatchCount++;
#endif
            return;
        }
    }
}
#endif

static sb32 ndsRendererAdapterPrepareNativeStageMaterials(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    const u32 material_count = ndsRendererAdapterNativeStageActiveMaterialCount();
    u32 i;

    /* Each slot's binding and expected MObj flags come from the packet's
     * material-event table; Dream Land's are {20,22,31,32}/{1,1,0x6b,0x6b}. */
    for (i = 0u; i < material_count; i++)
    {
        u32 binding_index;
        u32 flags;
        MObj *mobj;

        if ((ndsRendererNativeStageMaterialBinding(i, &binding_index,
                                                   &flags) == FALSE) ||
            (binding_index >= workspace->binding_count) ||
            (workspace->binding_dobjs[binding_index] == NULL))
        {
            return FALSE;
        }
        mobj = workspace->binding_dobjs[binding_index]->mobj;
        if ((mobj == NULL) ||
            (ndsRendererAdapterMaterialFlags(mobj) != (u16)flags) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &workspace->materials[i], FALSE,
                 &workspace->material_curr[i],
                 &workspace->material_next[i]) == FALSE))
        {
#if NDS_R2_SECOND_ENTRY_DIAG
            /* Latch the first mismatch against the selected packet before a
             * later scene can overwrite its binding and material identity. */
            gNdsR2StageMaterialRejectCount++;
            if (gNdsR2StageMaterialRejectIndex == 0xFFFFFFFFu)
            {
                gNdsR2StageMaterialRejectIndex = i;
                gNdsR2StageMaterialRejectBinding = binding_index;
                gNdsR2StageMaterialRejectDObj =
                    (u32)(uintptr_t)workspace->binding_dobjs[binding_index];
                gNdsR2StageMaterialRejectMObj = (u32)(uintptr_t)mobj;
                gNdsR2StageMaterialRejectFlagsWant = flags;
                gNdsR2StageMaterialRejectFlagsGot =
                    ndsRendererAdapterMaterialFlags(mobj);
                gNdsR2StageMaterialRejectHeapGen = gNdsTaskmanHeapGeneration;
            }
#endif
            return FALSE;
        }
        workspace->material_mobjs[i] = mobj;
    }
    return TRUE;
}

static void ndsRendererAdapterCommitNativeStageMaterials(
    NDSRendererAdapterNativeStageWorkspace *workspace, u32 segment_index)
{
    u32 mask = ndsRendererNativeStageMaterialMask(segment_index);
    u32 i;

    /* Commit only the material slots owned by this packet's segment. Stages
     * with no material animation have no MObjs in this workspace. */
    for (i = 0u; mask != 0u; i++, mask >>= 1u)
    {
        if ((mask & 1u) == 0u)
        {
            continue;
        }
        workspace->material_mobjs[i]->texture_id_curr =
            workspace->material_curr[i];
        workspace->material_mobjs[i]->texture_id_next =
            workspace->material_next[i];
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3MaterialCommitCount++;
#endif
    }
}

#if NDS_TASK103_STAGE_RUN_PHASE
/* Task 103 E2/E4. Defined here rather than in nds_renderer.c because the spans
 * they measure live in this file, and ahead of both users because C needs the
 * declaration first.
 *
 * E2 wraps the segment commit and the material setup ahead of it. E3 then found
 * that path is only 40% of the stage bucket: **236,039 ticks/frame -- 60% of
 * STG and 18% of all frame work -- are inside
 * ndsRendererAdapterPrepareNativeStageOwner, at one call per frame**, which no
 * task had ever profiled. E4's six spans split that function's steady-state
 * body into the steps it actually runs, so the number stops being a function
 * name and becomes a lever. Lab only, default off. */
volatile u32 gNdsTask103CommitTicks;
volatile u32 gNdsTask103CommitCount;
volatile u32 gNdsTask103MaterialTicks;
volatile u32 gNdsTask103PrepAdmitTicks;
volatile u32 gNdsTask103PrepValidateTicks;
volatile u32 gNdsTask103PrepMatrixTicks;
volatile u32 gNdsTask103PrepMaterialTicks;
volatile u32 gNdsTask103PrepConfigTicks;
volatile u32 gNdsTask103PrepOwnerTicks;
volatile u32 gNdsTask103PrepCalls;
#endif

s32 ndsRendererAdapterPrepareNativeStageOwner(void *camera_gobj_ptr)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;
    NDSRelocLoadedFile *loaded[NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT];
    const u32 asset_count = ndsRendererAdapterNativeStageActiveAssetCount();
    GObj *camera_gobj = camera_gobj_ptr;
    CObj *cobj = (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) : NULL;
    u32 topology_generation = 0u;
    u32 topology_stamp = 0u;
    sb32 topology_cached = FALSE;
    u32 i;
#if NDS_TASK44_STAGE_STEADY
    sb32 steady_admitted = FALSE;
#endif
    /* UNCONDITIONAL. The reject label this feeds calls
     * ndsRendererHardwareAbortBattleStaticTextures, which discards the whole
     * hardware texture cache and leaves it unable to re-arm for the rest of the
     * scene. Which branch got there therefore cannot be a profile-only fact --
     * it is the difference between a next cycle that reads one counter and a
     * next cycle that re-derives six branches. See nds_renderer.c's row 6 note. */
    u32 task36_reject_reason = 1u;
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
    gNdsRendererTask36AdapterRejectReason = 0u;
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
    u32 task103_prep_entry = cpuGetTiming();
    u32 task103_prep_mark;
#endif

    workspace->active = FALSE;
    if (gNdsRendererFastRunMode !=
        NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE)
    {
        return FALSE;
    }
    if ((asset_count == 0u) || (asset_count > NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT))
    {
        return FALSE;
    }
    if (cobj == NULL)
    {
        task36_reject_reason = 2u;
        goto reject;
    }
#if NDS_TASK44_STAGE_STEADY
    /* Task 44 item 3: steady-state admission. One generation compare plus the
     * cheap segment guard replaces the four asset lookups and the whole
     * topology stamp rebuild. Everything the fast path would have recomputed
     * (loaded[], asset_bases[], topology_generation, topology_stamp) is
     * already recorded in the workspace and provably unchanged, because every
     * seam that can change it bumps sNdsRelocStageAssetMutation. */
    if ((workspace->topology_valid != FALSE) &&
        (workspace->task44_admission_generation != 0u) &&
        (workspace->task44_admission_generation ==
         sNdsRelocStageAssetMutation) &&
        (ndsRendererAdapterNativeStageSegmentsUnchanged(workspace) != FALSE))
    {
        steady_admitted = TRUE;
        topology_generation = workspace->topology_generation;
        topology_stamp = workspace->topology_stamp;
        topology_cached = TRUE;
#if NDS_R2_SECOND_ENTRY_DIAG
        /* The fast path that reuses binding_dobjs[] wholesale. It consults
         * neither owner_generation nor the heap generation -- only
         * sNdsRelocStageAssetMutation -- so if that fails to move on a second
         * scene entry, last match's DObj pointers are re-admitted intact. The
         * existing counters here are PROFILE_LEVEL==1 only, which the tick-HUD
         * build is not, so this bug class was invisible to every run. */
        gNdsR2StageSteadyAdmitCount++;
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask44SteadyAdmitCount++;
#endif
    }
    if (steady_admitted == FALSE)
#endif
    {
#if NDS_TASK44_STAGE_STEADY && (NDS_RENDERER_PROFILE_LEVEL == 1)
    gNdsRendererTask44RevalidateCount++;
#endif
    for (i = 0u; i < asset_count; i++)
    {
        loaded[i] = ndsRelocFindLoadedFileByAsset(
            ndsRendererAdapterNativeStageAssetId(i));
        if ((loaded[i] == NULL) || (loaded[i]->data == NULL) ||
            (loaded[i]->data_size !=
             ndsRendererAdapterNativeStageAssetSize(i)) ||
            (loaded[i]->owner_generation == 0u) ||
            ((i != 0u) &&
             (loaded[i]->owner_generation != topology_generation)))
        {
            task36_reject_reason = 3u;
            goto reject;
        }
        topology_generation = loaded[i]->owner_generation;
    }
    if ((workspace->topology_valid != FALSE) &&
        (workspace->topology_generation == topology_generation))
    {
        topology_cached = TRUE;
        for (i = 0u; i < asset_count; i++)
        {
            if (workspace->loaded[i] != loaded[i])
            {
                topology_cached = FALSE;
                break;
            }
        }
        if ((topology_cached != FALSE) &&
            ((ndsRendererAdapterBuildNativeStageTopologyStamp(
                  workspace, topology_generation, &topology_stamp) == FALSE) ||
             (topology_stamp != workspace->topology_stamp)))
        {
            topology_cached = FALSE;
        }
    }
    if (topology_cached == FALSE)
    {
#if NDS_R2_SECOND_ENTRY_DIAG
        /* A full rebuild: binding_dobjs[] is re-collected from the live tree.
         * This is what MUST happen on a second scene entry. */
        gNdsR2StageTopologyRebuildCount++;
#endif
        bzero(workspace, sizeof(*workspace));
#if NDS_R2_STAGE_VALIDATE_STRIDE
        /* Slice 44. binding_dobjs[] is about to be re-collected from the live
         * tree, so every stage world entry keyed on an old DObj address is now
         * meaningless -- and a recycled heap address makes one of them *match*.
         * That was harmless while `validated_frame == frame` forced a rebuild
         * every frame; under the stride a collision would hand back the
         * previous topology's matrix. Free at runtime: this branch is the
         * once-per-scene rebuild that already bzeroes the whole workspace. */
        sNdsRendererAdapterStageWorldCacheCount = 0u;
        memset(sNdsRendererAdapterStageWorldIndex, 0,
               sizeof(sNdsRendererAdapterStageWorldIndex));
#endif
        for (i = 0u; i < asset_count; i++)
        {
            workspace->loaded[i] = loaded[i];
            workspace->frame.asset_bases[i] = loaded[i]->data;
        }
        if ((ndsRendererAdapterCollectNativeStageTopology(workspace) == FALSE) ||
#if NDS_TASK36_HW_COMPOSE
            (ndsRendererAdapterCaptureTask36StageWorld(workspace) == FALSE) ||
#endif
            (ndsRendererAdapterBuildNativeStageTopologyStamp(
                 workspace, topology_generation, &topology_stamp) == FALSE))
        {
            task36_reject_reason = 4u;
            goto reject;
        }
        workspace->topology_generation = topology_generation;
        workspace->topology_stamp = topology_stamp;
        workspace->topology_valid = TRUE;
    }
    else
    {
        for (i = 0u; i < asset_count; i++)
        {
            workspace->frame.asset_bases[i] = loaded[i]->data;
        }
    }
    /* Static DS payloads are source-independent once converted, but their
     * cache keys contain the live O2R source pointers. A stage asset may be
     * replaced at a new taskman-heap address after pre-GO texture preparation;
     * Task 44 already routes every such mutation through this full-validation
     * arm. Refresh only those pointer-derived key words here. This performs no
     * file I/O, conversion, allocation, or VRAM upload. */
    if (ndsRendererHardwareRefreshBattleStaticTexturePointers() == FALSE)
    {
        task36_reject_reason = 3u;
        goto reject;
    }
#if NDS_TASK44_STAGE_STEADY
    /* Only a completed full validation may arm the fast path. */
    workspace->task44_admission_generation = sNdsRelocStageAssetMutation;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask44AdmissionGeneration = sNdsRelocStageAssetMutation;
#endif
#endif
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    task103_prep_mark = cpuGetTiming();
    gNdsTask103PrepAdmitTicks += task103_prep_mark - task103_prep_entry;
#endif
#if NDS_TASK36_HW_COMPOSE
#if NDS_R2_STAGE_VALIDATE_STRIDE
    /* Slice 44: one advance per frame, here, because this is the only site the
     * c120 profile shows running exactly once -- 26 rigid checks a frame against
     * a 26-entry list. Both the rigid sweep below and the dynamic chain walk in
     * ndsRendererAdapterPrepareNativeStageMatrices read the cursor, so they stay
     * in phase and every frame does the same amount of work. */
    workspace->slice44_validate_cursor =
        (u8)((workspace->slice44_validate_cursor + 1u) %
             NDS_R2_STAGE_VALIDATE_STRIDE);
#endif
    ndsRendererAdapterValidateTask36StageWorld(workspace);
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103PrepValidateTicks += cpuGetTiming() - task103_prep_mark;
    task103_prep_mark = cpuGetTiming();
#endif
    if (ndsRendererAdapterPrepareNativeStageMatrices(cobj, workspace) == FALSE)
    {
        task36_reject_reason = 5u;
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        /* The matrix path publishes a finer sub-reason, but only when
         * profiling is compiled in; 5u is the honest answer without it. */
        if (gNdsRendererTask36AdapterRejectReason != 0u)
        {
            task36_reject_reason = gNdsRendererTask36AdapterRejectReason;
        }
#endif
        goto reject;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103PrepMatrixTicks += cpuGetTiming() - task103_prep_mark;
    task103_prep_mark = cpuGetTiming();
#endif
    if (ndsRendererAdapterPrepareNativeStageMaterials(workspace) == FALSE)
    {
        task36_reject_reason = 7u;
        goto reject;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103PrepMaterialTicks += cpuGetTiming() - task103_prep_mark;
    task103_prep_mark = cpuGetTiming();
#endif

    bzero(&workspace->resolver, sizeof(workspace->resolver));
    workspace->resolver.primary_file = workspace->loaded[0];
    workspace->config = (NDSRendererConfig){0};
    workspace->config.max_depth = 8u;
    workspace->config.max_commands = 2048u;
    workspace->config.max_list_commands = 512u;
    workspace->config.texture_data_layout =
        NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    workspace->config.validate_range = ndsRendererAdapterStageValidateRange;
    workspace->config.immutable_command_span =
        ndsRendererAdapterImmutableCommandSpan;
    workspace->config.resolve_branch = ndsFighterDLDrawResolveBranch;
    workspace->config.resolve_data = ndsFighterDLDrawResolveRendererData;
    workspace->config.user = &workspace->resolver;
    workspace->frame.dobjs = workspace->live_dobjs;
    workspace->frame.binding_display_lists =
        workspace->binding_display_lists;
    workspace->frame.projection = &workspace->projection;
#if NDS_TASK36_HW_COMPOSE
    workspace->frame.camera_modelview = &workspace->camera_modelview;
    workspace->frame.binding_world = workspace->binding_world;
    workspace->frame.rigid_binding_mask =
        workspace->task36_runtime_rigid_mask;
#else
    workspace->frame.camera_modelview = NULL;
    workspace->frame.binding_world = NULL;
    workspace->frame.rigid_binding_mask = 0u;
#endif
    workspace->frame.binding_composed = workspace->binding_composed;
    workspace->frame.materials = workspace->materials;
    workspace->frame.config = &workspace->config;
    workspace->frame.topology_generation = workspace->topology_generation;
    workspace->frame.topology_stamp = workspace->topology_stamp;
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103PrepConfigTicks += cpuGetTiming() - task103_prep_mark;
    task103_prep_mark = cpuGetTiming();
#endif
    if (ndsRendererPrepareNativeStageOwner(
            &workspace->frame, &workspace->stats) == FALSE)
    {
        workspace->topology_valid = FALSE;
        task36_reject_reason = 6u;
        goto reject;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103PrepOwnerTicks += cpuGetTiming() - task103_prep_mark;
    gNdsTask103PrepCalls++;
#endif
    workspace->next_segment = 0u;
    workspace->active = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3DObjCount = workspace->dobj_count;
    gNdsRendererM3BindingCount = workspace->binding_count;
    gNdsRendererM3MaterialShadowCount =
        ndsRendererAdapterNativeStageActiveMaterialCount();
#endif
    return TRUE;

reject:
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
    gNdsRendererTask36AdapterRejectReason = task36_reject_reason;
#endif
    /* BUGS row 6. Published unconditionally, and the FIRST reason is latched
     * rather than only the last: the abort below is irreversible for the scene,
     * so the branch that caused it is the one worth keeping. A later reject is
     * a consequence of the first one, not independent evidence. */
    gNdsRendererStageOwnerRejectCount++;
    gNdsRendererStageOwnerLastRejectReason = task36_reject_reason;
    if (gNdsRendererStageOwnerFirstRejectReason == 0u)
    {
        gNdsRendererStageOwnerFirstRejectReason = task36_reject_reason;
    }
    workspace->active = FALSE;
    /* A FAILED DISPLAY-GRAPH PREPARE SAYS NOTHING ABOUT TEXTURE RESIDENCY, AND
     * THIS IS WHERE IT USED TO CLAIM OTHERWISE.
     *
     * This block was:
     *
     *     if (gNdsRendererBattleStaticTextureArmCount != 0u)
     *         ndsRendererHardwareAbortBattleStaticTextures();
     *
     * which discards the entire hardware texture cache and clears
     * sNdsRendererBattleStaticTexturePrepared. Both Arm call sites
     * (taskman_seam.c:5251 and :7975) fire only on the Wait->Go TRANSITION, so
     * they never run again inside a match -- and Arm refuses to re-arm without
     * Prepared anyway. One transient frame therefore dropped the 24 pinned
     * statics for the remainder of the match with no path back.
     *
     * Measured on 2026-08-04 (build-row6-v1, two-death match): at frame 427,
     * right after a star KO, abort went 0->1, preparednow 1->0, and
     * staticpin froze at 7144 and never moved again through frame 2174. The
     * scene stayed correctly textured -- the dynamic cache absorbed it -- but
     * frame rate fell from 27.9 to 20.0 for the rest of the match, and the
     * pinned corpus was gone for good.
     *
     * Nothing here justified that. The pinned corpus is uploaded at scene
     * prepare and is untouched by whatever made this frame's display graph
     * unusable; workspace->active = FALSE above is already the whole fallback
     * contract, and the generic path does not read the workspace. So the
     * correct behaviour on a post-arm reject is to degrade for THIS FRAME and
     * leave every texture exactly where it is, which also leaves the next
     * frame free to succeed normally.
     *
     * The event stays loud rather than becoming silent: the reject counters
     * above fire unconditionally, and this one isolates the post-arm case that
     * used to be destructive, so a regression here is still one counter away.
     */
    if (gNdsRendererBattleStaticTextureArmCount != 0u)
    {
        gNdsRendererStageOwnerPostArmRejectCount++;
    }
    return FALSE;
}

s32 __attribute__((section(".itcm")))
ndsRendererAdapterCommitNativeStageDisplay(
    void *display_gobj_ptr, s32 link_id)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;
    GObj *display_gobj = display_gobj_ptr;
    u32 i;

    if (workspace->active == FALSE)
    {
        return FALSE;
    }
    for (i = 0u; i < ndsRendererAdapterNativeStageActiveSegmentCount(); i++)
    {
        if (display_gobj == workspace->segments[i])
        {
            if ((i != workspace->next_segment) ||
                ((u32)link_id != ndsRendererAdapterNativeStageSegmentLink(i)))
            {
                (void)ndsRendererCommitNativeStageSegment(0xffffffffu);
                return TRUE;
            }
#if NDS_TASK29_GX_CENSUS
            ndsRendererTask29GXSetOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
            /* Task 103 E2. The run loop accounts for only 136,519 of the
             * ~370,000 stage bucket, so these two spans say whether the
             * remainder is per-segment scaffolding inside the commit or
             * material setup ahead of it. Wrapped at the call site because
             * ndsRendererCommitNativeStageSegment has several early returns
             * and an in-function span would miss them. */
            {
                u32 task103_mat_start = cpuGetTiming();
                u32 task103_commit_start;
                s32 task103_committed;

                ndsRendererAdapterCommitNativeStageMaterials(workspace, i);
                task103_commit_start = cpuGetTiming();
                task103_committed = ndsRendererCommitNativeStageSegment(i);
                gNdsTask103MaterialTicks +=
                    task103_commit_start - task103_mat_start;
                gNdsTask103CommitTicks +=
                    cpuGetTiming() - task103_commit_start;
                gNdsTask103CommitCount++;
                if (task103_committed == FALSE)
                {
                    ndsRendererFinishNativeStageOwner();
                    workspace->active = FALSE;
                    return FALSE;
                }
            }
#else
            ndsRendererAdapterCommitNativeStageMaterials(workspace, i);
            if (ndsRendererCommitNativeStageSegment(i) == FALSE)
            {
                ndsRendererFinishNativeStageOwner();
                workspace->active = FALSE;
                return FALSE;
            }
#endif
            workspace->next_segment++;
            return TRUE;
        }
    }
    return FALSE;
}

void ndsRendererAdapterFinishNativeStageOwner(void)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;

    if (workspace->active != FALSE)
    {
        ndsRendererFinishNativeStageOwner();
    }
#if NDS_TASK29_GX_CENSUS
    ndsRendererTask29GXSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
#endif
    workspace->active = FALSE;
    workspace->next_segment = 0u;
}
#else
s32 ndsRendererAdapterPrepareNativeStageOwner(void *camera_gobj)
{
    (void)camera_gobj;
    return FALSE;
}

s32 __attribute__((section(".itcm")))
ndsRendererAdapterCommitNativeStageDisplay(
    void *display_gobj, s32 link_id)
{
    (void)display_gobj;
    (void)link_id;
    return FALSE;
}

void ndsRendererAdapterFinishNativeStageOwner(void)
{
}
#endif

/* Always compiled, unlike the validator below: this counts how often the
 * material write walk's capacity guard actually fires, and a guard whose hit
 * count nobody can read is indistinguishable from one that never runs. */
volatile u32 gNdsR2MaterialWalkBoundHits;
/* Runtime toggle so ONE binary can run both arms. Clearing this from a debugger
 * restores the pre-guard unbounded walk with identical code layout, identical
 * inlining and an identical ROM -- which is the only way to attribute the
 * Sudden Death freeze to the guard rather than to the placement change adding
 * the guard caused. Two different builds cannot answer that question; this
 * campaign has repeatedly measured layout moving results on its own (E11
 * removed real work and P95 still rose 15,744). Defaults to 1: the guard is
 * live in every build, and only a deliberate debugger write disables it. */
volatile u32 gNdsR2MaterialWalkBoundEnabled = 1u;

#if NDS_R2_SECOND_ENTRY_DIAG
/* Second-entry chain validator. Default OFF (Makefile NDS_R2_SECOND_ENTRY_DIAG).
 *
 * The material write walk was running off the end of a four-entry array, which
 * means the MObj chain it walks stops being what pass one measured. Bounding
 * the walk contained the damage; it did not say WHEN the list goes bad. This
 * answers that directly instead of inferring it from the overflow site: the
 * chain is recorded twice per DObj -- once before the counting pass, once
 * immediately before the writing pass -- so a list that is sound at the first
 * probe and broken at the second localises the corruption to the counting pass
 * itself, and one broken at both puts it upstream of this function entirely. */
#define NDS_R2_CHAIN_PROBE_MAX 64u

enum {
    NDS_R2_CHAIN_OK = 0u,
    NDS_R2_CHAIN_OVERLONG = 1u,   /* more nodes than any real DObj has */
    NDS_R2_CHAIN_CYCLE = 2u,      /* next pointer revisits a seen node */
    NDS_R2_CHAIN_OUT_OF_ARENA = 3u/* node or next outside the taskman arena */
};

typedef struct NDSR2ChainProbe {
    u32 status;
    u32 nodes;        /* nodes walked before terminating or failing */
    u32 first_bad;    /* address of the offending node, 0 when clean */
    u32 dobj;         /* which DObj owned the chain */
    u32 generation;   /* taskman-heap generation at probe time */
} NDSR2ChainProbe;

volatile NDSR2ChainProbe gNdsR2ChainProbePass1;
volatile NDSR2ChainProbe gNdsR2ChainProbePass2;
/* Latched at the FIRST failure of the run and never overwritten, so a later
 * clean frame cannot erase the evidence. */
volatile NDSR2ChainProbe gNdsR2ChainProbeFirstBad;
volatile u32 gNdsR2ChainProbeFirstBadPass;
volatile u32 gNdsR2ChainProbeInvalidCount;
volatile u32 gNdsR2ChainProbeCount;

static void ndsR2ChainProbe(DObj *dobj, volatile NDSR2ChainProbe *out, u32 pass)
{
    const MObj *seen[NDS_R2_CHAIN_PROBE_MAX];
    const MObj *mobj;
    u32 count = 0u;
    u32 status = NDS_R2_CHAIN_OK;
    u32 first_bad = 0u;

    gNdsR2ChainProbeCount++;
    for (mobj = (dobj != NULL) ? dobj->mobj : NULL; mobj != NULL;
         mobj = mobj->next)
    {
        u32 i;

        if (ndsFighterDLScanRangeInTaskmanArena(mobj, sizeof(*mobj)) == FALSE)
        {
            status = NDS_R2_CHAIN_OUT_OF_ARENA;
            first_bad = (u32)(uintptr_t)mobj;
            break;
        }
        /* O(n^2) against a 64 bound is 4,096 compares worst case, and this is a
         * default-off diagnostic -- a hash would be more code for no answer. */
        for (i = 0u; i < count; i++)
        {
            if (seen[i] == mobj)
            {
                status = NDS_R2_CHAIN_CYCLE;
                first_bad = (u32)(uintptr_t)mobj;
                break;
            }
        }
        if (status != NDS_R2_CHAIN_OK)
        {
            break;
        }
        if (count >= NDS_R2_CHAIN_PROBE_MAX)
        {
            status = NDS_R2_CHAIN_OVERLONG;
            first_bad = (u32)(uintptr_t)mobj;
            break;
        }
        seen[count++] = mobj;
    }
    out->status = status;
    out->nodes = count;
    out->first_bad = first_bad;
    out->dobj = (u32)(uintptr_t)dobj;
    out->generation = gNdsTaskmanHeapGeneration;

    if (status != NDS_R2_CHAIN_OK)
    {
        gNdsR2ChainProbeInvalidCount++;
        if (gNdsR2ChainProbeFirstBadPass == 0u)
        {
            gNdsR2ChainProbeFirstBadPass = pass;
            gNdsR2ChainProbeFirstBad = *out;
        }
    }
}
#endif

/* Cycle 110. The fighter material snapshot is a pure function of `mobj->sub`
 * plus texture_id_curr/next, lfrac and palette_id -- and it never varies. The
 * NDS_TICK_HUD census that has sat in this file since cycle 98 answered it on a
 * 60-second match: 20,100 builds, 20,069 of them byte-identical to the previous
 * snapshot for the same MObj, 31 first-sights, and **zero** variants. So ~761
 * cycles a build, about twelve times a frame, reconstruct a constant --
 * 13,176 ticks/frame of it, 2,124 of which is the single `mobj->sub.flags`
 * load missing cache at 139 cycles an execution.
 *
 * The key hashes the COMPLETE input set rather than the fields I believe can
 * animate. That is the difference between a skip that is correct by
 * construction and one that is correct until someone adds a texture-scroll
 * track: the builder reads nothing outside that set, so equal inputs means
 * equal output with only a 2^-32 collision to argue about. The heap generation
 * is in the key because MObj pointers are taskman-arena addresses and a scene
 * rewind reuses them.
 *
 * The build's side effect -- writing texture_id_curr/next back into the MObj --
 * is safe to skip for the same reason: the stored hash is taken AFTER the
 * write-back, so a match means the write would store what is already there.
 *
 * Only the production path passes keys. The hierarchy path's materials array is
 * a caller local that does not survive the frame, so it passes NULL and always
 * builds. After this the census counts REBUILDS, which makes gNdsFtrPreMatCalls
 * engagement proof: it should read tens, not tens of thousands. */
#if NDS_TICK_HUD
/* Engagement proof. gNdsFtrPreMatCalls cannot answer this: it counts calls that
 * reach the census inside the build wrapper, and the hierarchy call site passes
 * no keys, so a skipping production path and a never-skipping one can produce
 * the same census. These two count the decision itself. */
volatile u32 gNdsR2MatKeySkip;
volatile u32 gNdsR2MatKeyBuild;
/* And WHY a build happened, because the two answers point at different fixes.
 * Identity: the row holds a different MObj than last frame, so the block for
 * this one is sitting in some other row -- fix is a stable row assignment.
 * Inputs: same MObj, hash moved -- fix would be a narrower key, which is
 * already refuted. Guessing between them once cost a build. */
volatile u32 gNdsR2MatKeyMissIdentity;
volatile u32 gNdsR2MatKeyMissInputs;
#endif

/* Hashes all 30 words of MObjSub, including the six the builder never reads
 * (sub.unk48, sub.unk4C, sub.unk68..unk74). Narrowing it to the builder's exact
 * read set was tried and is REFUTED: the engagement counters came back
 * bit-identical -- 28,786 skips and 30,606 builds either way -- and FTR rose
 * 1,155. The rebuilds are not caused by those words at all. They are
 * `keys[count].mobj != mobj`: the materials array is indexed by (selected-root
 * slot, chain position), and which DObj lands in slot i rotates between frames,
 * so about half the lookups find the right block under the wrong index.
 *
 * Recovering that half needs a per-MObj store -- 33 live MObjs x 100 bytes plus
 * keys, about 7 KB of bss against an arena whose low-water is already under the
 * GObj-cap threshold -- or a stable slot assignment. Neither is this slice.
 * Since the narrow hash bought nothing, keep the one that needs no field audit
 * to stay correct. */
static u32 ndsRendererAdapterMaterialRow(DObj *dobj, u32 fallback_row)
{
    u32 base;
    u32 probe;

    if (dobj == NULL)
    {
        sNdsRendererAdapterMaterialRowClaimMask |= 1u << fallback_row;
        return fallback_row;
    }
    if (sNdsRendererAdapterMaterialRowGeneration != gNdsTaskmanHeapGeneration)
    {
        u32 j;

        for (j = 0u; j < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; j++)
        {
            sNdsRendererAdapterMaterialRowOwner[j] = NULL;
        }
        sNdsRendererAdapterMaterialRowGeneration = gNdsTaskmanHeapGeneration;
        sNdsRendererAdapterMaterialRowClaimMask = 0u;
    }
    /* Multiplicative, not `>> 4`: DObjs are allocated contiguously and a shift
     * hash strided them onto a handful of rows, so the probe loop ran several
     * iterations and the whole lookup measured 106 cycles a call (2,024
     * ticks/frame) on a 128-byte table that is always in cache. */
    base = ((u32)(uintptr_t)dobj * 2654435761u) >>
        (32u - NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED_LOG2);
    for (probe = 0u; probe < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; probe++)
    {
        u32 row = (base + probe) & (NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED - 1u);

        if (sNdsRendererAdapterMaterialRowOwner[row] == dobj)
        {
            /* The display contract can reference the same material DObj more
             * than once. Sharing in that case is source-equivalent and safe. */
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
        if ((sNdsRendererAdapterMaterialRowOwner[row] == NULL) &&
            ((sNdsRendererAdapterMaterialRowClaimMask & (1u << row)) == 0u))
        {
            sNdsRendererAdapterMaterialRowOwner[row] = dobj;
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
    }
    /* Every row is owned by some persistent DObj. Reclaim one that this CURRENT
     * owner has not claimed. The input key catches the changed identity and
     * rebuilds that row. selected_count cannot exceed the row count, so unless
     * duplicate DObjs reduced the number of claims, an unclaimed row is always
     * available here. */
    for (probe = 0u; probe < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; probe++)
    {
        u32 row = (base + probe) & (NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED - 1u);

        if ((sNdsRendererAdapterMaterialRowClaimMask & (1u << row)) == 0u)
        {
            sNdsRendererAdapterMaterialRowOwner[row] = dobj;
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
    }
    /* Defensive only: the collection bound above makes this unreachable. */
    return fallback_row;
}

/* The nine words that actually move during a match, in the two contiguous runs
 * they occupy. `primcolor` through `light2color` is `MObjSub` 0x50..0x67 -- the
 * five colour tracks gcPlayMObjMatAnim writes plus the prim level/min byte pair
 * that shares their run -- and texture_id_curr through palette_id is the twelve
 * bytes immediately after `sub`. Two cache lines instead of five, nine
 * multiply-accumulates instead of thirty-four. */
static u32 __attribute__((section(".itcm")))
ndsRendererAdapterMaterialAnimHash(const MObj *mobj)
{
    const u32 *colors = (const u32 *)(const void *)&mobj->sub.primcolor;
    u32 hash = 2166136261u;
    u32 i;

    for (i = 0u; i < 6u; i++)
    {
        hash = (hash ^ colors[i]) * 16777619u;
    }
    hash = (hash ^ (((u32)mobj->texture_id_curr << 16) |
                    (u32)mobj->texture_id_next)) * 16777619u;
    hash = (hash ^ *(const u32 *)(const void *)&mobj->lfrac) * 16777619u;
    hash = (hash ^ *(const u32 *)(const void *)&mobj->palette_id) * 16777619u;
    return hash;
}

/* `light2color` is the last field of the colour run, so the six words starting
 * at `primcolor` must land exactly on it. If MObjSub is ever reordered this
 * stops compiling rather than silently hashing the wrong bytes. */
_Static_assert(offsetof(MObjSub, light2color) ==
                   offsetof(MObjSub, primcolor) + 20u,
               "material anim hash assumes primcolor..light2color are six "
               "contiguous words");

/* P2-2 fighter packet: the identity of everything the current fighter's
 * material rows were built from -- every prepared MObj's animation hash and
 * pointer plus the colour modulate -- folded into one word per draw and handed
 * to the production owner as its packet key. The hashes already exist (the
 * material memo computes them per MObj per frame), so this is a few XORs. */
static u32 sNdsFighterPacketMaterialIdentity;

/* The animation-state hash and identity of every material the selected roots
 * carry, from the live MObj chains alone -- no rows, no snapshots -- so the
 * adapter can ask whether the packet will replay before it spends the
 * preparation on a frame whose replay reads none of it. Taken before any
 * build on the record path too, so the key means one thing on both paths. */
static u32 ndsRendererAdapterMaterialIdentity(
    DObj *const *material_dobjs, u32 count)
{
    u32 identity = 2166136261u;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        const DObj *dobj = material_dobjs[i];
        const MObj *mobj;

        for (mobj = (dobj != NULL) ? dobj->mobj : NULL;
             mobj != NULL;
             mobj = mobj->next)
        {
            identity = (identity ^ ndsRendererAdapterMaterialAnimHash(mobj)) *
                       16777619u;
            identity ^= (u32)(uintptr_t)mobj;
        }
    }
    return identity;
}

static sb32 ndsRendererAdapterPrepareNativeMaterials(
    DObj *dobj, NDSRendererNativeMaterial *materials,
    u32 capacity, u32 *out_count,
    NDSRendererAdapterMaterialKey *keys,
    s32 *save_curr, s32 *save_next)
{
    MObj *mobj;
    u32 count = 0u;

    if ((materials == NULL) || (out_count == NULL))
    {
        return FALSE;
    }
    *out_count = 0u;
    if ((dobj == NULL) || (dobj->mobj == NULL))
    {
        return TRUE;
    }
#if NDS_R2_SECOND_ENTRY_DIAG
    ndsR2ChainProbe(dobj, &gNdsR2ChainProbePass1, 1u);
    ndsR2ChainProbe(dobj, &gNdsR2ChainProbePass2, 2u);
#endif
    /* There is no counting pre-pass any more. It walked the whole MObj chain a
     * second time -- a dependent pointer chase, 37 chains a frame, 1,215
     * ticks/frame on its `mobj = mobj->next` alone in the c115 per-PC census --
     * purely so an over-capacity chain could be rejected before anything was
     * written. The write walk below already carries that bound of its own, and
     * since the rollback slice it also reports `*out_count` on rejection, so the
     * caller undoes exactly the entries this walk touched. Two passes proved one
     * fact; one pass proves it at the point of use.
     *
     * The chain validator measured 13,938 chains of ONE node against a capacity
     * of four, so the difference between rejecting before and rejecting during
     * is a path that has never been taken. */
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        /* Bound the WRITE walk too, not just the counting one above. The count
         * pass only constrains this pass if the list is identical across both,
         * and it is not: ndsRendererAdapterBuildNativeMaterial is the
         * advance_texture_ids=TRUE wrapper, so this loop writes
         * mobj->texture_id_curr/next into every node as it walks. A list that
         * turns cyclic or is corrupted mid-walk ran `materials[count]` off the
         * end of a `capacity`-entry array -- capacity is 4 -- with no check at
         * all. Returning FALSE hands the caller its existing generic fallback
         * instead of corrupting whatever follows the array.
         *
         * Found while reproducing the Sudden Death freeze (docs/BUGS.md): the
         * scene presents two frames and then none, with no overflow assert
         * firing anywhere, and an interrupt landing inside this loop's inlined
         * material build. */
        if ((gNdsR2MaterialWalkBoundEnabled != 0u) && (count >= capacity))
        {
            /* Engagement proof, and it is NOT decoration. The Sudden Death
             * freeze stopped when this guard went in, but the default-off chain
             * validator then found 13,938 clean chains of ONE node against a
             * capacity of four -- so on that evidence this branch can never be
             * reached, and "the guard fixed the freeze" and "the chain is fine"
             * cannot both be true. This counter is what tells them apart: a run
             * that is freeze-free with this still at zero proves the guard was
             * not the cure and the real cause is still live. */
            gNdsR2MaterialWalkBoundHits++;
            /* Report what the snapshot holds so the caller rolls back exactly
             * the entries this walk mutated, no more and no fewer. */
            *out_count = count;
            return FALSE;
        }
        if (save_curr != NULL)
        {
            /* The rollback snapshot, taken here rather than in a walk of its
             * own. ndsRendererAdapterSaveNativeMaterialTextureIds was a second
             * pass over the same chain reading the same two fields -- 3,429
             * ticks/frame in the c110 profile -- and the hash below loads them
             * anyway, so this costs two stores into a line the caller owns. */
            save_curr[count] = mobj->texture_id_curr;
            save_next[count] = mobj->texture_id_next;
        }
        if (keys != NULL)
        {
            u32 hash = ndsRendererAdapterMaterialAnimHash(mobj);

            if ((keys[count].mobj == mobj) &&
                (keys[count].heap_generation == gNdsTaskmanHeapGeneration) &&
                (keys[count].hash == hash))
            {
#if NDS_TICK_HUD
                gNdsR2MatKeySkip++;
#endif
                count++;
                continue;
            }
#if NDS_TICK_HUD
            gNdsR2MatKeyBuild++;
            if (keys[count].mobj != mobj)
            {
                gNdsR2MatKeyMissIdentity++;
            }
            else
            {
                gNdsR2MatKeyMissInputs++;
            }
#endif
        }
        if (ndsRendererAdapterBuildNativeMaterial(
                mobj, &materials[count]) == FALSE)
        {
            if (keys != NULL)
            {
                keys[count].mobj = NULL;
            }
            /* count + 1: entry `count` was snapshotted above and the builder is
             * the advance_texture_ids=TRUE wrapper, so it may have written this
             * MObj's ids before failing. Restoring an untouched entry writes
             * back what is already there, so over-reporting by one is safe and
             * under-reporting is not. */
            *out_count = count + 1u;
            return FALSE;
        }
        if (keys != NULL)
        {
            /* After the build, so the stored hashes describe the MObj the build
             * left behind -- it writes texture_id_curr/next back. Both are
             * stored on every build: the full one is only CHECKED periodically,
             * but it has to be current whenever that check lands. */
            keys[count].mobj = mobj;
            keys[count].heap_generation = gNdsTaskmanHeapGeneration;
            keys[count].hash = ndsRendererAdapterMaterialAnimHash(mobj);
        }
        count++;
    }
    *out_count = count;
    return TRUE;
}

static void ndsRendererAdapterRestoreNativeMaterialTextureIds(
    DObj *dobj,
    const s32 *curr,
    const s32 *next,
    u32 count)
{
    MObj *mobj;
    u32 i = 0u;

    if ((dobj == NULL) || (curr == NULL) || (next == NULL))
    {
        return;
    }
    for (mobj = dobj->mobj;
         (mobj != NULL) && (i < count);
         mobj = mobj->next, i++)
    {
        mobj->texture_id_curr = curr[i];
        mobj->texture_id_next = next[i];
    }
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static sb32 ndsRendererAdapterValidateNativeOwnerMaterials(
    const NDSRendererNativeMaterial *materials,
    u32 material_count)
{
    u32 i;

    if ((material_count != 0u) && (materials == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < material_count; i++)
    {
        const NDSRendererNativeMaterial *material = &materials[i];
        u32 effects = material->effects;

        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->palette_image,
                 1u) == NULL))
        {
            return FALSE;
        }
        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->block_image,
                 1u) == NULL))
        {
            return FALSE;
        }
        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->current_image,
                 1u) == NULL))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
    DObj *const *material_dobjs,
    u32 root_count)
{
    if (material_dobjs == NULL)
    {
        return;
    }
    /* A contract may select one material DObj more than once. Roll back in
     * reverse event order so each saved pre-event state is restored and the
     * earliest snapshot remains live for the ordinary renderer fallback. */
    while (root_count != 0u)
    {
        u32 root_index = --root_count;

        ndsRendererAdapterRestoreNativeMaterialTextureIds(
            material_dobjs[root_index],
            sNdsRendererAdapterNativeOwnerTextureCurr[root_index],
            sNdsRendererAdapterNativeOwnerTextureNext[root_index],
            sNdsRendererAdapterNativeOwnerTextureCounts[root_index]);
    }
}
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
/* A plan hit skips this outright (see "THE DELETION" at the plan-hit branch), and
 * the plan hits every frame -- the c112 cold map found its body inside the
 * driver's third-largest never-executed run. */
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterValidateNativeOwnerCached(
    u32 slot,
    u32 use_low_detail,
    const NDSRelocLoadedFile *owner_file,
    u32 root_count,
    const u32 *root_offsets,
    const u32 *material_counts)
{
    NDSRendererAdapterNativeOwnerValidationCache *cache;
    u32 i;
    sb32 identity_matches;

    if ((slot >= NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT) ||
        (owner_file == NULL) ||
        (root_offsets == NULL) || (material_counts == NULL) ||
        (root_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
#if NDS_TICK_HUD
        gNdsFtrPreValidateReject++;
#endif
        return FALSE;
    }
    cache = &sNdsRendererAdapterNativeOwnerValidationCache[slot];
    identity_matches =
        ((cache->valid != 0u) &&
         (cache->data == owner_file->data) &&
         (cache->asset_id == owner_file->asset_id) &&
         (cache->owner_generation == owner_file->owner_generation) &&
         (cache->data_size == owner_file->data_size) &&
         (cache->root_count == root_count) &&
         (cache->use_low_detail == use_low_detail)) ? TRUE : FALSE;
    if (identity_matches != FALSE)
    {
        for (i = 0u; i < root_count; i++)
        {
            if ((cache->root_offsets[i] != root_offsets[i]) ||
                (cache->material_counts[i] != material_counts[i]))
            {
                identity_matches = FALSE;
                break;
            }
        }
    }
    if (identity_matches != FALSE)
    {
#if NDS_TICK_HUD
        /* Cycle 98. THE counter this row existed to add: without it a cache hit
         * and a cache miss are indistinguishable here, and deleting work whose
         * cache already hits is the mistake cycle 93 avoided on the stage. */
        gNdsFtrPreValidateReuse++;
#endif
        return TRUE;
    }

#if NDS_TICK_HUD
    gNdsFtrPreValidateBuild++;
#endif
    cache->valid = FALSE;
    if (ndsRendererValidateNativeFighterOwner(
            slot, use_low_detail, owner_file->data_size, root_count,
            root_offsets, material_counts) == FALSE)
    {
        return FALSE;
    }
    cache->data = owner_file->data;
    cache->asset_id = owner_file->asset_id;
    cache->owner_generation = owner_file->owner_generation;
    cache->data_size = owner_file->data_size;
    cache->root_count = root_count;
    cache->use_low_detail = use_low_detail;
    for (i = 0u; i < root_count; i++)
    {
        cache->root_offsets[i] = root_offsets[i];
        cache->material_counts[i] = material_counts[i];
    }
    cache->valid = TRUE;
    return TRUE;
}
#endif

static Gfx *ndsRendererAdapterEmitMaterialCommands(Gfx *branch_dl, MObj *mobj)
{
    u32 flags = ndsRendererAdapterMaterialFlags(mobj);
    f32 scau = 0.0F;
    f32 scav = 0.0F;
    f32 trau = 0.0F;
    f32 trav = 0.0F;
    f32 scrollu = 0.0F;
    f32 scrollv = 0.0F;
    s32 uls;
    s32 ult;
    s32 s;
    s32 t;

    if ((branch_dl == NULL) || (mobj == NULL))
    {
        return branch_dl;
    }

    ndsRendererAdapterMaterialTextureState(
        mobj, flags, &scau, &scav, &trau, &trav, &scrollu, &scrollv);

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj->sub.palettes != NULL))
    {
        const void *palette = ndsRendererAdapterReadPointerEntry(
            mobj->sub.palettes, (s32)mobj->palette_id);

        if (palette != NULL)
        {
            ndsRendererAdapterEmitTextureImage(
                branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u, palette);
        }
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0)
    {
        ndsRendererAdapterEmitTextureImage(
            branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.palettes, (s32)mobj->palette_id));
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
        {
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPTILESYNC);
            ndsRendererAdapterEmitSetTile(
                branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0u, 0x0100u, 5u,
                0u, NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
            ndsRendererAdapterEmitLoadTlut(
                branch_dl++, 5u,
                (mobj->sub.siz == G_IM_SIZ_8b) ? 0xffu : 0x0fu);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPPIPESYNC);
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0)
    {
        branch_dl = ndsRendererAdapterEmitLightColor(
            branch_dl, 1u,
            ndsRendererAdapterPackColor(&mobj->sub.light1color));
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0)
    {
        branch_dl = ndsRendererAdapterEmitLightColor(
            branch_dl, 2u,
            ndsRendererAdapterPackColor(&mobj->sub.light2color));
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0)
    {
        if ((flags & MOBJ_FLAG_FRAC) != 0)
        {
            s32 trunc = (s32)mobj->lfrac;

            ndsRendererAdapterEmitPrimColor(
                branch_dl++, mobj->sub.prim_m,
                ndsRendererAdapterClampU8F32(
                    (mobj->lfrac - (f32)trunc) * 256.0F),
                mobj->sub.primcolor.s.r,
                mobj->sub.primcolor.s.g,
                mobj->sub.primcolor.s.b,
                mobj->sub.primcolor.s.a);
            mobj->texture_id_curr = trunc;
            mobj->texture_id_next = trunc + 1;
        }
        else
        {
            ndsRendererAdapterEmitPrimColor(
                branch_dl++, mobj->sub.prim_m,
                ndsRendererAdapterClampU8F32(mobj->lfrac * 255.0F),
                mobj->sub.primcolor.s.r,
                mobj->sub.primcolor.s.g,
                mobj->sub.primcolor.s.b,
                mobj->sub.primcolor.s.a);
        }
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
    {
        ndsRendererAdapterEmitColor(
            branch_dl++, NDS_FIGHTER_DL_OP_SETENVCOLOR,
            mobj->sub.envcolor.s.r, mobj->sub.envcolor.s.g,
            mobj->sub.envcolor.s.b, mobj->sub.envcolor.s.a);
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
    {
        ndsRendererAdapterEmitColor(
            branch_dl++, NDS_FIGHTER_DL_OP_SETBLENDCOLOR,
            mobj->sub.blendcolor.s.r, mobj->sub.blendcolor.s.g,
            mobj->sub.blendcolor.s.b, mobj->sub.blendcolor.s.a);
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
    {
        s32 block_siz = (mobj->sub.block_siz == G_IM_SIZ_32b) ?
            G_IM_SIZ_32b : G_IM_SIZ_16b;

        ndsRendererAdapterEmitTextureImage(
            branch_dl++, mobj->sub.block_fmt, (u32)block_siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_next));
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            u32 texels = 0u;
            u32 dxt = 0u;

            ndsRendererAdapterMaterialLoadBlock(mobj, &texels, &dxt);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
            ndsRendererAdapterEmitLoadBlock(branch_dl++, 6u, 0u, 0u,
                                            texels, dxt);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
    {
        ndsRendererAdapterEmitTextureImage(
            branch_dl++, mobj->sub.fmt, mobj->sub.siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_curr));
    }
    if ((flags & 0x20u) != 0)
    {
        if (mobj->sub.unk10 == 2)
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0C * trau) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0E * trav) / scav) * 4.0F) : 0;
            if (uls < 0)
            {
                uls = 0;
            }
            if (ult < 0)
            {
                ult = 0;
            }
        }
        else
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((((f32)mobj->sub.unk0C * trau) +
                         (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((((1.0F - scav) - trav) *
                          (f32)mobj->sub.unk0E) +
                         (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        }
        ndsRendererAdapterEmitTileSize(
            branch_dl++, NDS_RENDERER_ADAPTER_G_TX_RENDERTILE, uls, ult,
            (((s32)mobj->sub.unk0C - 1) << 2) + uls,
            (((s32)mobj->sub.unk0E - 1) << 2) + ult);
    }
    if ((flags & 0x40u) != 0)
    {
        uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
            (s32)(((((f32)mobj->sub.unk38 * scrollu) +
                     (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
        ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
            (s32)((((((1.0F - scav) - scrollv) *
                      (f32)mobj->sub.unk3A) +
                     (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        ndsRendererAdapterEmitTileSize(
            branch_dl++, 1u, uls, ult,
            (((s32)mobj->sub.unk38 - 1) << 2) + uls,
            (((s32)mobj->sub.unk3A - 1) << 2) + ult);
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        if (mobj->sub.unk10 == 2)
        {
            s = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0C * 64.0F) / scau) : 0;
            t = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0E * 64.0F) / scav) : 0;
        }
        else
        {
            s = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scau) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scau) : 0;
            t = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scav) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scav) : 0;
        }
        if (s > 0xffff)
        {
            s = 0xffff;
        }
        if (t > 0xffff)
        {
            t = 0xffff;
        }
        ndsRendererAdapterEmitTexture(
            branch_dl++, (u32)s, (u32)t, 0u,
            NDS_RENDERER_ADAPTER_G_TX_RENDERTILE,
            NDS_RENDERER_ADAPTER_G_ON);
    }

    ndsRendererAdapterEmitEndDL(branch_dl++);
    return branch_dl;
}

/* Defined beside the other graphics-heap counters in src/port/diagnostics.c.
 * Declared here rather than in a header for the same reason the decomp bodies
 * declare gSYTaskmanGraphicsHeap locally: this file has no startup header. */
extern volatile u32 gNdsTaskmanGraphicsHeapNoRoomCount;

static sb32 ndsRendererAdapterPrepareMaterialSegment(
    DObj *dobj, NDSFighterDLDrawState *state)
{
    MObj *mobj;
    Gfx *table;
    Gfx *branch_dl;
    uintptr_t heap_start;
    uintptr_t heap_end;
    uintptr_t heap_ptr;
    u32 mobj_count = 0u;
    u32 branch_commands = 0u;
    size_t heap_bytes;
    u32 i = 0u;

    if ((dobj == NULL) || (state == NULL) || (dobj->mobj == NULL))
    {
        return FALSE;
    }
    if (ndsRendererAdapterCountMaterialCommands(
            dobj, &mobj_count, &branch_commands) == FALSE)
    {
        return FALSE;
    }
    if ((mobj_count == 0u) ||
        (gSYTaskmanGraphicsHeap.ptr == NULL) ||
        (gSYTaskmanGraphicsHeap.start == NULL) ||
        (gSYTaskmanGraphicsHeap.end == NULL))
    {
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileRecordMaterialOperations(mobj_count);
#endif

    heap_start = (uintptr_t)gSYTaskmanGraphicsHeap.start;
    heap_end = (uintptr_t)gSYTaskmanGraphicsHeap.end;
    heap_ptr = (uintptr_t)gSYTaskmanGraphicsHeap.ptr;
    heap_bytes = (size_t)(mobj_count + branch_commands) * sizeof(Gfx);
    if ((heap_ptr < heap_start) || (heap_ptr > heap_end))
    {
        return FALSE;
    }
    if (heap_bytes > (size_t)(heap_end - heap_ptr))
    {
        /* P2-3f9. THE ONE UNBOUNDED GRAPHICS-HEAP WRITER, MADE COUNTABLE.
         * Every other writer on this port has a source bound (see the note on
         * NDS_R2_VSBATTLE_GRAPHICS_ARENA_BYTES in battleship_scvsbattle.c);
         * this table is sized by the DObj's own material chain and so cannot
         * be bounded from a header. It has always refused rather than
         * overrun -- which is why an undersized heap shows up here as a
         * MISSING material branch and not as corruption -- but a refusal that
         * nothing counts is indistinguishable from a DObj that had no
         * materials. gNdsTaskmanGraphicsHeapOverflowCount cannot see it: the
         * pointer never passes `end`, so the sampler has nothing to report.
         * The four-CPU stress harness asserts this at 0. */
        gNdsTaskmanGraphicsHeapNoRoomCount++;
        return FALSE;
    }

    table = (Gfx *)gSYTaskmanGraphicsHeap.ptr;
    branch_dl = table + mobj_count;
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next, i++)
    {
        ndsRendererAdapterEmitBranchTableCommand(&table[i], branch_dl);
        branch_dl = ndsRendererAdapterEmitMaterialCommands(branch_dl, mobj);
    }

    gSYTaskmanGraphicsHeap.ptr = branch_dl;
    state->segment_e_base = table;
    state->segment_e_end = branch_dl;
    return TRUE;
}

#if NDS_TICK_HUD
/* G3 STEP 0 -- THE UNIQUE-TEMPLATE CENSUS, and it is the number a packet arena
 * is sized by. Every G3 figure banked so far counts list INSTANCES
 * (gNdsEffectDLSubmitCount: 1,360 Boundary, 527-563 gate arm); an arena sized
 * from an instance count is wrong by whatever the reuse factor is, and that
 * factor has never been measured.
 *
 * The key is the display-list pointer. That is sound WITHIN a match -- dl points
 * into a loaded-file buffer resident for the scene, the same property G1's
 * texture-site memo relies on -- and it is NOT sound across one: charter 3.12,
 * the taskman arena rewinds and hands the next scene the same addresses. A
 * builder sized by this census must re-derive at scene entry. The census only
 * has to survive the window it measures, and P1 boots straight into one match.
 *
 * StateVariants/CommandVariants are the feasibility guards: if one dl is
 * submitted under two different entry blend modes or yields two different
 * command counts, then "one packet per unique dl" is not a complete key and the
 * arena needs more entries than Unique. They must be read before Unique is
 * trusted as the sizing input.
 *
 * Called from the epilogue, OUTSIDE the Exec bracket that closes above it, so
 * ticks/list stays the interpreter's own cost rather than the census's. */
#define NDS_EFFECT_DL_CENSUS_CAPACITY 256u

static const Gfx *sNdsEffectDLCensusKey[NDS_EFFECT_DL_CENSUS_CAPACITY];
static u32 sNdsEffectDLCensusOtherMode[NDS_EFFECT_DL_CENSUS_CAPACITY];
static u32 sNdsEffectDLCensusCommands[NDS_EFFECT_DL_CENSUS_CAPACITY];
/* PER-TEMPLATE GEOMETRY, AND IT MUST BE THE MAX RATHER THAN THE FIRST SIGHTING.
 * hardware_triangle_count is a POST-CULL count, so the same template submits
 * different geometry on different frames as it moves through the frustum. A
 * packet has to encode the template's whole content, so the sizing input is the
 * largest submission ever seen, not a sample of one. GeomVariants says whether
 * culling moves it at all: 0 means the geometry is frame-invariant and the max
 * is exact; non-zero means the max is the honest lower bound on static content
 * and the arena wants margin over it. */
static u32 sNdsEffectDLCensusTrisMax[NDS_EFFECT_DL_CENSUS_CAPACITY];
static u32 sNdsEffectDLCensusVertsMax[NDS_EFFECT_DL_CENSUS_CAPACITY];

static void ndsEffectDLCensusRecord(const Gfx *dl, u32 commands,
                                    u32 othermode_in, u32 tris, u32 verts)
{
    u32 count = gNdsEffectDLCensusUnique;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        if (sNdsEffectDLCensusKey[i] == dl)
        {
            if (sNdsEffectDLCensusOtherMode[i] != othermode_in)
            {
                gNdsEffectDLCensusStateVariants++;
            }
            if (sNdsEffectDLCensusCommands[i] != commands)
            {
                gNdsEffectDLCensusCommandVariants++;
            }
            if (tris != sNdsEffectDLCensusTrisMax[i])
            {
                gNdsEffectDLCensusGeomVariants++;
            }
            /* Totals are maintained incrementally so the report never needs a
             * second pass over the table. */
            if (tris > sNdsEffectDLCensusTrisMax[i])
            {
                gNdsEffectDLCensusTrisMaxTotal +=
                    tris - sNdsEffectDLCensusTrisMax[i];
                sNdsEffectDLCensusTrisMax[i] = tris;
            }
            if (verts > sNdsEffectDLCensusVertsMax[i])
            {
                gNdsEffectDLCensusVertsMaxTotal +=
                    verts - sNdsEffectDLCensusVertsMax[i];
                sNdsEffectDLCensusVertsMax[i] = verts;
            }
            return;
        }
    }
    if (count >= NDS_EFFECT_DL_CENSUS_CAPACITY)
    {
        /* Overflow is reported, never silently truncated: a capped unique count
         * reads exactly like a small one and would size the arena short. */
        gNdsEffectDLCensusOverflow++;
        return;
    }
    sNdsEffectDLCensusKey[count] = dl;
    sNdsEffectDLCensusOtherMode[count] = othermode_in;
    sNdsEffectDLCensusCommands[count] = commands;
    sNdsEffectDLCensusTrisMax[count] = tris;
    sNdsEffectDLCensusVertsMax[count] = verts;
    gNdsEffectDLCensusUniqueCommandTotal += commands;
    gNdsEffectDLCensusTrisMaxTotal += tris;
    gNdsEffectDLCensusVertsMaxTotal += verts;
    if (commands > gNdsEffectDLCensusCommandMax)
    {
        gNdsEffectDLCensusCommandMax = commands;
    }
    gNdsEffectDLCensusUnique = count + 1u;
}

/* G3 STEP 1 -- the per-template verdict on the captured GX stream. The capture
 * itself is in nds_renderer.c, hooked into the GX record funnel; this is the
 * comparison, and it deliberately reuses the census's own key so the two answer
 * for exactly the same template population.
 *
 * 32 entries against a measured 8 uniques is 4x margin, and the overflow is
 * counted rather than wrapped: a table that silently dropped a template would
 * report perfect agreement for the ones it kept. */
#define NDS_EFFECT_PACKET_TEMPLATE_CAPACITY 32u

static const Gfx *sNdsEffectPacketKey[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketGeomHashSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketColorHashSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketMatrixHashSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketGeomWordsSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];

static void ndsEffectPacketVerdictRecord(const Gfx *dl)
{
    u32 count = gNdsEffectPacketTemplates;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        if (sNdsEffectPacketKey[i] != dl)
        {
            continue;
        }
        if (sNdsEffectPacketGeomHashSeen[i] == gNdsEffectPacketGeomHash)
        {
            gNdsEffectPacketGeomMatchCount++;
        }
        else
        {
            gNdsEffectPacketGeomVariantCount++;
        }
        if (sNdsEffectPacketGeomWordsSeen[i] != gNdsEffectPacketGeomWords)
        {
            /* Separate from the hash verdict on purpose: a stream that changed
             * LENGTH is a different failure from one that changed VALUES, and
             * only the second is a candidate for a patch table. */
            gNdsEffectPacketGeomWordVariantCount++;
        }
        if (sNdsEffectPacketColorHashSeen[i] == gNdsEffectPacketColorHash)
        {
            gNdsEffectPacketColorMatchCount++;
        }
        else
        {
            gNdsEffectPacketColorVariantCount++;
        }
        if (sNdsEffectPacketMatrixHashSeen[i] == gNdsEffectPacketMatrixHash)
        {
            gNdsEffectPacketMatrixMatchCount++;
        }
        else
        {
            gNdsEffectPacketMatrixVariantCount++;
        }
        return;
    }
    if (count >= NDS_EFFECT_PACKET_TEMPLATE_CAPACITY)
    {
        gNdsEffectPacketTableOverflow++;
        return;
    }
    sNdsEffectPacketKey[count] = dl;
    sNdsEffectPacketGeomHashSeen[count] = gNdsEffectPacketGeomHash;
    sNdsEffectPacketColorHashSeen[count] = gNdsEffectPacketColorHash;
    sNdsEffectPacketMatrixHashSeen[count] = gNdsEffectPacketMatrixHash;
    sNdsEffectPacketGeomWordsSeen[count] = gNdsEffectPacketGeomWords;
    gNdsEffectPacketTemplates = count + 1u;
}
#endif

#if NDS_ENTRY_EFFECT_DIAG
/* P2-3r6: WHERE THE PIPE BODY'S Y COMES FROM.
 *
 * The renderer already records the modelview each entry-effect root is
 * submitted under, and it says the body (root 0x04c0) sits ~300 units above the
 * rim (0x03c0, a constant -21) for the whole visible life of the effect. That
 * is either what the source animation wrote into the DObj, or something this
 * adapter's matrix build introduced. Recording the DObj's own transform here,
 * beside the matrix build, separates the two without another guess.
 *
 * Floats are stored as their bit patterns: the gdb stub reads globals reliably
 * and the host can decode, whereas printing a float through the stub has
 * already produced one misleading 0.000000. */
volatile u32 gNdsEntryEffectDObjTranslate[2][3];
volatile u32 gNdsEntryEffectDObjScale[2][3];
volatile u32 gNdsEntryEffectDObjRotate[2][3];
volatile u32 gNdsEntryEffectDObjParent[2];
volatile u32 gNdsEntryEffectDObjParentTranslate[2][3];
/* The animation clock beside the value it produced: if the body's translate
 * track is right but its PLAYBACK is twice the source rate, the body leaves in
 * half the frames and the pipe reads as "body missing". anim_speed is what
 * `aobj->length += dobj->anim_speed` advances by, so it is the rate itself. */
volatile u32 gNdsEntryEffectDObjAnim[2][3];
volatile u32 gNdsEntryEffectGObjAnimFrame[2];
/* The DObj address itself, so the AObj track list can be walked from the
 * HOST. Walking it in guest code inside the draw is what this file tried
 * first; one word here and gdb does the rest. */
volatile u32 gNdsEntryEffectDObjPtr[2];

static void ndsEntryEffectDiagRecordDObj(u32 root_offset, DObj *dobj)
{
    u32 slot;
    DObj *parent;

    if (root_offset == 0x03c0u)
    {
        slot = 0u;
    }
    else if (root_offset == 0x04c0u)
    {
        slot = 1u;
    }
    else
    {
        return;
    }
    if (dobj == NULL)
    {
        return;
    }
    gNdsEntryEffectDObjTranslate[slot][0] = *(const u32 *)&dobj->translate.vec.f.x;
    gNdsEntryEffectDObjTranslate[slot][1] = *(const u32 *)&dobj->translate.vec.f.y;
    gNdsEntryEffectDObjTranslate[slot][2] = *(const u32 *)&dobj->translate.vec.f.z;
    gNdsEntryEffectDObjScale[slot][0] = *(const u32 *)&dobj->scale.vec.f.x;
    gNdsEntryEffectDObjScale[slot][1] = *(const u32 *)&dobj->scale.vec.f.y;
    gNdsEntryEffectDObjScale[slot][2] = *(const u32 *)&dobj->scale.vec.f.z;
    gNdsEntryEffectDObjRotate[slot][0] = *(const u32 *)&dobj->rotate.vec.f.x;
    gNdsEntryEffectDObjRotate[slot][1] = *(const u32 *)&dobj->rotate.vec.f.y;
    gNdsEntryEffectDObjRotate[slot][2] = *(const u32 *)&dobj->rotate.vec.f.z;
    gNdsEntryEffectDObjPtr[slot] = (u32)(uintptr_t)dobj;
    gNdsEntryEffectDObjAnim[slot][0] = *(const u32 *)&dobj->anim_speed;
    gNdsEntryEffectDObjAnim[slot][1] = *(const u32 *)&dobj->anim_wait;
    gNdsEntryEffectDObjAnim[slot][2] = *(const u32 *)&dobj->anim_frame;
    if (dobj->parent_gobj != NULL)
    {
        gNdsEntryEffectGObjAnimFrame[slot] =
            *(const u32 *)&dobj->parent_gobj->anim_frame;
    }
    parent = dobj->parent;
    gNdsEntryEffectDObjParent[slot] = (u32)(uintptr_t)parent;
    if (parent != NULL)
    {
        gNdsEntryEffectDObjParentTranslate[slot][0] =
            *(const u32 *)&parent->translate.vec.f.x;
        gNdsEntryEffectDObjParentTranslate[slot][1] =
            *(const u32 *)&parent->translate.vec.f.y;
        gNdsEntryEffectDObjParentTranslate[slot][2] =
            *(const u32 *)&parent->translate.vec.f.z;
    }
}
#endif

static sb32 ndsRendererAdapterTryNativeEntryEffect(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode)
{
#if NDS_RENDERER_HW_TRIANGLES
    const u8 *base = NULL;
    u32 owner_asset_id = 0u;
    u32 root_offset = 0u;
    sb32 candidate = FALSE;
    NDSRendererConfig config = {0};
    NDSRendererStats stats;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    const NDSRendererMatrix20p12 *projection_ptr;
    const NDSRendererMatrix20p12 *modelview_ptr;
#if NDS_P2_LINK
    NDSRendererNativeMaterial link_special2_material;
    NDSRendererNativeMaterial link_spin_materials[9];
#endif
    /* Every owner passes these to the native prepare; only Link's two
     * material-snapshot arms fill them, so the pair lives outside his flag. */
    const NDSRendererNativeMaterial *native_materials = NULL;
    u32 native_material_count = 0u;

    if ((dobj == NULL) || (dl == NULL))
    {
        return FALSE;
    }

    /* Exact source asset + exact generated root is the whole admission test.
     * Do not classify arbitrary effect lists by shape: this path intentionally
     * owns only entry props that the offline source bake emitted. */
    if ((gFTMarioFileSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTMarioFileSpecial2))
    {
        base = (const u8 *)gFTMarioFileSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x03c0u) || (root_offset == 0x04c0u))
        {
            owner_asset_id = 356u;
            candidate = TRUE;
        }
    }
#if NDS_P2_LUIGI
    /* dFTLuigiData points its Special2 slot at llMarioSpecial2FileID exactly
     * like Mario, but ftManager owns a separate destination pointer for each
     * fighter kind.  In a Luigi-vs-Fox match gFTMarioFileSpecial2 is therefore
     * legitimately NULL while gFTDataLuigiSpecial2 contains the same source
     * asset.  Admit that second live base explicitly; otherwise Luigi's pipe
     * falls back to the N64 interpreter even though the AOT packet is already
     * an exact bake of file 356. */
    if ((candidate == FALSE) && (gFTDataLuigiSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataLuigiSpecial2))
    {
        base = (const u8 *)gFTDataLuigiSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x03c0u) || (root_offset == 0x04c0u))
        {
            owner_asset_id = 356u;
            candidate = TRUE;
        }
    }
#endif
    if ((candidate == FALSE) && (gFTDataFoxSpecial3 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataFoxSpecial3))
    {
        base = (const u8 *)gFTDataFoxSpecial3;
        root_offset = (u32)((const u8 *)dl - base);
        switch (root_offset)
        {
        case 0x1fa0u:
        case 0x2920u:
        case 0x29d0u:
        case 0x29f0u:
        case 0x2a20u:
        case 0x2868u:
        case 0x2a50u:
        case 0x2b00u:
            owner_asset_id = 161u;
            candidate = TRUE;
            break;
        default:
            break;
        }
    }
#if NDS_P2_DONKEY
    /* BattleShip dEFManagerDonkeyEntryTaruEffectDesc uses DonkeySpecial2 and
     * gcDrawDObjTreeForGObj. Its DObjDesc at 0x07c8 contains one drawable child
     * whose exact source display-list root is 0x0620. Keep the source tree and
     * animation live; replace only that immutable N64 DL/texture work. */
    if ((candidate == FALSE) && (gFTDataDonkeySpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataDonkeySpecial2))
    {
        base = (const u8 *)gFTDataDonkeySpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if (root_offset == 0x0620u)
        {
            owner_asset_id = 355u;
            candidate = TRUE;
        }
    }
#endif
#if NDS_P2_SAMUS
    /* BattleShip dEFManagerSamusEntryPointEffectDesc owns one animated child
     * whose DObjDLLink submits two immutable source lists (links 0 and 1).
     * Keep the source DObj tree and EntryPoint AnimJoint live -- notably its
     * near-zero -> full -> near-zero Y scale that opens/closes the point -- and
     * replace only those two N64 Gfx streams with their generated DS packets. */
    if ((candidate == FALSE) && (gFTDataSamusSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataSamusSpecial2))
    {
        base = (const u8 *)gFTDataSamusSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x0930u) || (root_offset == 0x0ad0u))
        {
            owner_asset_id = 349u;
            candidate = TRUE;
        }
    }
#endif
#if NDS_P2_LINK
    /* BattleShip's Link entry wave/beam and attached grounded Spin EFFECT each
     * own a live animated DObj in LinkSpecial2. The collision weapon is a
     * different owner (LinkModel below); do not conflate its MatAnim payload
     * with weapon geometry again. */
    if ((candidate == FALSE) && (gFTDataLinkSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataLinkSpecial2))
    {
        base = (const u8 *)gFTDataLinkSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x02d8u) || (root_offset == 0x0698u) ||
            (root_offset == 0x1100u))
        {
            owner_asset_id = 353u;
            candidate = TRUE;
        }
    }
    /* Grounded Spin's source WPAttributes live in LinkMain but resolve their
     * DObj/MObj/Anim/MatAnim pointers into LinkModel. The drawable child's
     * DObjDLLink at +0x118f8 submits exactly LinkModel+0x11680. Its segment-E
     * calls select the nine live MObjs built by gcDrawMObjForDObj. */
    if ((candidate == FALSE) && (gFTDataLinkModel != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataLinkModel))
    {
        base = (const u8 *)gFTDataLinkModel;
        root_offset = (u32)((const u8 *)dl - base);
        if (root_offset == 0x11680u)
        {
            owner_asset_id = 324u;
            candidate = TRUE;
        }
    }
    /* Boomerang's source DObj tree and six-tick rotation loop stay live. Its
     * two drawable children submit these exact LinkSpecial3 wrapper roots. */
    if ((candidate == FALSE) && (gFTDataLinkSpecial3 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataLinkSpecial3))
    {
        base = (const u8 *)gFTDataLinkSpecial3;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x0458u) || (root_offset == 0x0580u))
        {
            owner_asset_id = 325u;
            candidate = TRUE;
        }
    }
#endif
#if NDS_P2_CAPTAIN
    /* BattleShip dEFManagerCaptainEntryCarEffectDesc owns a live 13-node DObj
     * tree. Its 0x6200 main AnimJoint plus 0x6518/0x6598 child animations remain
     * source-owned; only the ten immutable CaptainSpecial2 Gfx roots below are
     * replaced by the exact generated DS packets. */
    if ((candidate == FALSE) && (gFTDataCaptainSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataCaptainSpecial2))
    {
        base = (const u8 *)gFTDataCaptainSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        switch (root_offset)
        {
        case 0x5690u:
        case 0x5c60u:
        case 0x5d20u:
        case 0x5d50u:
        case 0x5d80u:
        case 0x5db0u:
        case 0x5de0u:
        case 0x5e10u:
        case 0x5e40u:
        case 0x5e70u:
            owner_asset_id = 350u;
            candidate = TRUE;
            break;
        default:
            break;
        }
    }
#endif
    if (candidate == FALSE)
    {
        return FALSE;
    }

#if NDS_P2_LINK
    if (owner_asset_id == 353u)
    {
        MObj *mobj = dobj->mobj;

        /* LinkSpecial2's entry Wave, entry Beam, and attached grounded Spin
         * effect each select segment 0xE slot 0 from exactly one live MObj.
         * Keep the source MatAnim live and translate that one typed material
         * state instead of freezing its animated PRIM/light values in AOT. */
        if ((mobj == NULL) || (mobj->next != NULL) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &link_special2_material, FALSE, NULL, NULL) == FALSE))
        {
            gNdsEntryEffectNativeFallbackCount++;
            return FALSE;
        }
        native_materials = &link_special2_material;
        native_material_count = 1u;
    }
    else if (owner_asset_id == 324u)
    {
        MObj *mobj = dobj->mobj;
        u32 i;

        /* Source LinkModel has exactly nine MObjs for this DObj. Snapshot the
         * live values without advancing texture ids: every source MObj is
         * PRIM-only, and the native owner validates that invariant before GX. */
        for (i = 0u; i < 9u; i++)
        {
            if ((mobj == NULL) ||
                (ndsRendererAdapterBuildNativeMaterialSnapshot(
                     mobj, &link_spin_materials[i], FALSE, NULL, NULL) == FALSE))
            {
                gNdsEntryEffectNativeFallbackCount++;
                return FALSE;
            }
            mobj = mobj->next;
        }
        if (mobj != NULL)
        {
            gNdsEntryEffectNativeFallbackCount++;
            return FALSE;
        }
        native_materials = link_spin_materials;
        native_material_count = 9u;
    }
#endif

#if NDS_ENTRY_EFFECT_DIAG
    ndsEntryEffectDiagRecordDObj(root_offset, dobj);
#endif
    ndsRendererAdapterPrepareInitialMatrices(
        dobj,
        (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
            ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL),
        FALSE, &projection, &projection_ptr, &modelview, &modelview_ptr);
    /* The default battle camera has one legitimate split shape where the
     * complete camera transform lives on only one side of the DS pair (the
     * world-quad bridge handles the same contract above).  The generic DL
     * interpreter tolerates that because its matrix stream starts from
     * identity; this fixed owner has no stream to do the implicit fill for it.
     * Make that identity explicit so Fox's six small Arwing glow lists stay on
     * the native path instead of falling back solely because their capture pass
     * supplies one camera half. */
    if ((projection_ptr == NULL) && (modelview_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&projection);
        projection_ptr = &projection;
    }
    if ((modelview_ptr == NULL) && (projection_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&modelview);
        modelview_ptr = &modelview;
    }
    if ((projection_ptr == NULL) || (modelview_ptr == NULL))
    {
        gNdsEntryEffectNativeFallbackCount++;
        return FALSE;
    }

    ndsRendererInitStats(&stats);
    /* Light state. Both lists inherit G_LIGHTING from the battle display and
     * carry their own gSPLightColor words in the packet; what they do not
     * carry is seeded from what the display left in the RSP: the direction
     * scVSBattleFuncLights aimed, re-aimed by the fighter's own
     * ftDisplayLightsDrawReflect when it uses a light (same seed as
     * ndsRendererAdapterBeginStageTraversal), and for a group before the
     * first colour word the last colours written: the effect's own MObj
     * colours when it carries MOBJ_FLAG_LIGHT1/2, otherwise the fighter
     * material drawn before it. */
    if ((sNdsFighterDisplayCurrentLightValid != FALSE) &&
        (sNdsFighterDisplayCurrentLightCount != 0u))
    {
        stats.light_dir_x = sNdsFighterDisplayCurrentLight.l.dir[0];
        stats.light_dir_y = sNdsFighterDisplayCurrentLight.l.dir[1];
        stats.light_dir_z = sNdsFighterDisplayCurrentLight.l.dir[2];
        stats.light_dir_mask = 1u;
    }
    {
        MObj *mobj;

        for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
        {
            u32 flags = mobj->sub.flags;

            if ((flags & MOBJ_FLAG_LIGHT1) != 0u)
            {
                stats.light_color_1 =
                    ndsRendererAdapterPackColor(&mobj->sub.light1color);
                stats.light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK;
            }
            if ((flags & MOBJ_FLAG_LIGHT2) != 0u)
            {
                stats.light_color_2 =
                    ndsRendererAdapterPackColor(&mobj->sub.light2color);
                stats.light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK;
            }
        }
        if (gNdsFighterDisplayContractMaterialLightSeedCount != 0u)
        {
            if ((stats.light_color_mask &
                 NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK) == 0u)
            {
                stats.light_color_1 =
                    gNdsFighterDisplayContractMaterialLight1;
                stats.light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK;
            }
            if ((stats.light_color_mask &
                 NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK) == 0u)
            {
                stats.light_color_2 =
                    gNdsFighterDisplayContractMaterialLight2;
                stats.light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK;
            }
        }
    }
    config.max_depth = 4u;
    config.max_commands = 1u;
    config.max_list_commands = 1u;
    config.initial_projection = projection_ptr;
    config.initial_modelview = modelview_ptr;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;

    if (ndsRendererSubmitNativeEntryEffect(
            owner_asset_id, root_offset, native_materials,
            native_material_count, &config, &stats) == FALSE)
    {
        gNdsEntryEffectNativeFallbackCount++;
        return FALSE;
    }

    gNdsStageGCDrawAllLoopHardwareTriangleCount += stats.hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        stats.hardware_zbuffer_triangle_count;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
        stats.hardware_projected_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
        stats.hardware_decal_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
        stats.hardware_texture_bind_count;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
        stats.hardware_texture_upload_count;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
        stats.hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
        stats.hardware_texture_reject_count;
    return TRUE;
#else
    (void)dobj;
    (void)dl;
    (void)camera_gobj;
    (void)initial_geometry_mode;
    return FALSE;
#endif
}

static void ndsRendererAdapterSubmitStageDL(DObj *dobj, const Gfx *dl,
                                             GObj *camera_gobj,
                                             u32 initial_geometry_mode)
{
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config = {0};
    NDSRendererStats stats;
    NDSRendererStats *render_stats;
    NDSFighterDLDrawState state;
    NDSRendererCommandCallback callback;
    void *callback_user;
    NDSRendererMatrix20p12 initial_projection;
    NDSRendererMatrix20p12 initial_modelview;
    const NDSRendererMatrix20p12 *initial_projection_ptr;
    const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_R2_IMPACT_WAVE_NATIVE
    NDSRendererNativeMaterial impact_wave_material;
    sb32 impact_wave_native_candidate = FALSE;
    sb32 impact_wave_native_handled = FALSE;
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    u32 rebirth_halo_root_offset = 0u;
    sb32 rebirth_halo_native_candidate = FALSE;
    sb32 rebirth_halo_native_handled = FALSE;
#endif
    u32 effect_seed_before = 0u;
    u32 effect_matrix_cmd_before = 0u;
    u32 effect_xform_before = 0u;
    u32 effect_hw_vertex_before = 0u;
    u32 effect_hw_triangle_before = 0u;
#if NDS_RENDERER_HW_TRIANGLES
    void *saved_graphics_heap_ptr;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sb32 detailed_output;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 step_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 adapter_start;
    u32 adapter_ticks;
    NDSRendererOwnerStatsSnapshot owner_stats_before;
#endif
    sb32 inherited_texture = FALSE;
    sb32 inherited_tile = FALSE;
    sb32 inherited_segment = FALSE;
#endif
#if NDS_TICK_HUD
    /* R2-08 phase split. Latched once at entry rather than re-read per phase:
     * the flag is cleared by the tree submit's own epilogue, and a phase that
     * started inside the effect layer must be charged to it whatever the flag
     * says by the time the phase ends. */
    sb32 phase_effect = FALSE;
    u32 phase_dl_mark = 0u;
    u32 phase_mark = 0u;
#endif

    if ((dobj == NULL) || (dl == NULL))
    {
        return;
    }

    /* Entry models are closed generated owners. This sits before the generic
     * loaded-file scan and N64 interpreter setup on purpose: an accepted pipe or
     * Arwing leaf executes only live DObj matrix composition plus the DS-native
     * packet. Compatibility fallback remains available if scene texture prepare
     * failed, and is counted so production verification can require zero. */
    if (ndsRendererAdapterTryNativeEntryEffect(
            dobj, dl, camera_gobj, initial_geometry_mode) != FALSE)
    {
        return;
    }

#if NDS_R2_REBIRTH_HALO_NATIVE && NDS_R2_REBIRTH_HALO_FAST_ADAPTER
    /* RebirthHalo is already identified by the effect-tree owner before any
     * child list reaches here. Its six generated groups contain every source
     * state/texture/vertex dependency, so do not pay the generic adapter's
     * loaded-file scan, segment-E material preparation, callback context and
     * command-interpreter setup merely to arrive at the native submitter.
     *
     * Keep this per-DObj for the experiment: it preserves the exact world
     * matrix of the child and rotating grandchild while isolating the cost of
     * generic adapter ceremony. A later all-tree owner can merge the duplicate
     * child matrix/load once this gate has a visual/tick verdict. */
    if ((sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        uintptr_t rebirth_offset = (uintptr_t)((const u8 *)dl -
                                               (const u8 *)gEFManagerFiles[2]);

        if ((rebirth_offset == 0x2378u) || (rebirth_offset == 0x2a88u) ||
            (rebirth_offset == 0x27e8u))
        {
            NDSRendererConfig rebirth_config = {0};
            NDSRendererStats rebirth_stats;
            NDSRendererStats *rebirth_render_stats;
            NDSRendererMatrix20p12 rebirth_projection;
            NDSRendererMatrix20p12 rebirth_modelview;
            const NDSRendererMatrix20p12 *rebirth_projection_ptr;
            const NDSRendererMatrix20p12 *rebirth_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
            void *rebirth_saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#endif

            /* 0x2378 and 0x2a88 are the two DL links on the SAME child DObj.
             * Once the first one has emitted both native roots with one matrix
             * setup, the tree walker will immediately offer 0x2a88 again. */
            if ((rebirth_offset == 0x2a88u) &&
                (sNdsRendererAdapterRebirthHaloSkipSecondChildList != FALSE))
            {
                sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
                return;
            }

            ndsRendererAdapterPrepareInitialMatrices(
                dobj,
                (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
                    ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL),
                TRUE,
                &rebirth_projection,
                &rebirth_projection_ptr,
                &rebirth_modelview,
                &rebirth_modelview_ptr);

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
            if (sNdsRendererAdapterStagePersistentActive != FALSE)
            {
                rebirth_render_stats = &sNdsRendererAdapterStagePersistentStats;
                ndsFighterDLDrawResetRuntimeRendererStats(rebirth_render_stats);
            }
            else
#endif
            {
                rebirth_render_stats = &rebirth_stats;
                ndsRendererInitStats(rebirth_render_stats);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
                if (sNdsRendererAdapterStagePersistentActive != FALSE)
                {
                    ndsFighterDLDrawCopyPersistentRendererState(
                        rebirth_render_stats, &sNdsRendererAdapterStagePersistentStats);
                }
#endif
            }
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                rebirth_render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                rebirth_render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                rebirth_render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }

            rebirth_config.max_depth = 8u;
            rebirth_config.max_commands = 8192u;
            rebirth_config.max_list_commands = 512u;
            rebirth_config.initial_projection = rebirth_projection_ptr;
            rebirth_config.initial_modelview = rebirth_modelview_ptr;
            rebirth_config.initial_geometry_mode = initial_geometry_mode;
            rebirth_config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;

            if (ndsRendererSubmitNativeRebirthHalo(
                    (u32)rebirth_offset, &rebirth_config,
                    rebirth_render_stats) != FALSE)
            {
                gNdsRebirthHaloNativeDrawCount++;
                if (rebirth_offset == 0x2378u)
                {
                    /* Same DObj, same source matrix, adjacent source order.
                     * Keep the live renderer state produced by 0x2378 and emit
                     * its second linked list without rebuilding the adapter. */
                    if (ndsRendererSubmitNativeRebirthHalo(
                            0x2a88u, &rebirth_config,
                            rebirth_render_stats) != FALSE)
                    {
                        gNdsRebirthHaloNativeDrawCount++;
                        sNdsRendererAdapterRebirthHaloSkipSecondChildList = TRUE;
                    }
                    else
                    {
                        gNdsRebirthHaloNativeFallbackCount++;
                    }
                }
                gNdsStageGCDrawAllLoopHardwareTriangleCount +=
                    rebirth_render_stats->hardware_triangle_count;
                gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
                    rebirth_render_stats->hardware_zbuffer_triangle_count;
                gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
                    rebirth_render_stats->hardware_projected_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
                    rebirth_render_stats->hardware_decal_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
                    rebirth_render_stats->hardware_texture_bind_count;
                gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
                    rebirth_render_stats->hardware_texture_upload_count;
                gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
                    rebirth_render_stats->hardware_texture_ready_count;
                gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
                    rebirth_render_stats->hardware_texture_reject_count;
#if NDS_RENDERER_HW_TRIANGLES
                ndsTaskmanSampleGraphicsHeap();
                gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
#endif
                return;
            }
            gNdsRebirthHaloNativeFallbackCount++;
#if NDS_RENDERER_HW_TRIANGLES
            ndsTaskmanSampleGraphicsHeap();
            gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
#endif
        }
    }
#endif

#if NDS_TICK_HUD
    phase_effect =
        (sNdsRendererAdapterEffectSubmitActive != FALSE) ? TRUE : FALSE;
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseDLCount++;
        phase_dl_mark = cpuGetTiming();
        phase_mark = phase_dl_mark;
    }
#endif
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
#if NDS_TICK_HUD
        /* The REJECT exit still costs a full loaded-file scan plus an arena
         * scan, so it is charged rather than dropped -- an unmeasured early
         * return is exactly how a phase split acquires a residual. */
        if (phase_effect != FALSE)
        {
            gNdsEffectPhaseFindTicks += cpuGetTiming() - phase_mark;
            gNdsEffectPhaseDLTicks += cpuGetTiming() - phase_dl_mark;
        }
#endif
        return;
    }
#if NDS_R2_REBIRTH_HALO_NATIVE
    if ((sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        uintptr_t offset = (uintptr_t)((const u8 *)dl -
                                       (const u8 *)gEFManagerFiles[2]);

        if ((offset == 0x2378u) || (offset == 0x2a88u) ||
            (offset == 0x27e8u))
        {
            rebirth_halo_root_offset = (u32)offset;
            rebirth_halo_native_candidate = TRUE;
        }
    }
#endif
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseFindTicks += cpuGetTiming() - phase_mark;
    }
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    detailed_output = (ndsRendererHardwareNoOracleEnabled() == FALSE) ?
        TRUE : FALSE;
    if (detailed_output != FALSE)
    {
        bzero(&state, sizeof(state));
    }
    else
    {
        /* Profile 0/1 submit with a null command callback. Only the compact
         * branch/data resolver context is live; software-preview vertices
         * are already retained by the renderer's persistent vertex cache. */
        state.segment_e_base = NULL;
        state.segment_e_end = NULL;
    }
#else
    bzero(&state, sizeof(state));
#endif
    state.primary_file = loaded;
    state.slot = 0u;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        inherited_texture = ndsRendererAdapterStatsHasArmedTexture(
            &sNdsRendererAdapterStagePersistentStats);
        inherited_tile = ndsRendererAdapterStatsHasArmedTile(
            &sNdsRendererAdapterStagePersistentStats);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawSeedPersistentState(
                &state, &sNdsRendererAdapterStagePersistentState);
        }
        else
        {
            state.segment_e_base =
                sNdsRendererAdapterStagePersistentState.segment_e_base;
            state.segment_e_end =
                sNdsRendererAdapterStagePersistentState.segment_e_end;
        }
#else
        ndsFighterDLDrawSeedPersistentState(
            &state, &sNdsRendererAdapterStagePersistentState);
#endif
        inherited_segment = (state.segment_e_base != NULL) ? TRUE : FALSE;
        gNdsStageGCDrawAllLoopHardwareCarrySeedCount++;
        if (inherited_texture != FALSE)
        {
            gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount++;
        }
        if (inherited_tile != FALSE)
        {
            gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount++;
        }
        if (inherited_segment != FALSE)
        {
            gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount++;
        }
    }
    saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    adapter_start = cpuGetTiming();
    step_start = adapter_start;
#elif NDS_RENDERER_PROFILE_LEVEL >= 1
    step_start = cpuGetTiming();
#endif
#endif
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        phase_mark = cpuGetTiming();
    }
#endif
#if NDS_R2_IMPACT_WAVE_NATIVE
    if ((sNdsRendererAdapterImpactWaveNativeActive != FALSE) &&
        (dobj->mobj != NULL) &&
        (ndsRendererAdapterBuildNativeMaterial(
             dobj->mobj, &impact_wave_material) != FALSE))
    {
        impact_wave_native_candidate = TRUE;
    }
    else
#endif
    {
        ndsRendererAdapterPrepareMaterialSegment(dobj, &state);
    }
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseMaterialTicks += cpuGetTiming() - phase_mark;
        phase_mark = cpuGetTiming();
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 1)
    gNdsRendererProfileMaterialTicks += cpuGetTiming() - step_start;
    step_start = cpuGetTiming();
#endif
    ndsRendererAdapterPrepareInitialMatrices(dobj,
                                             (camera_gobj != NULL) ?
                                                 CObjGetStruct(camera_gobj) :
                                                 ((gGCCurrentCamera != NULL) ?
                                                      CObjGetStruct(
                                                          gGCCurrentCamera) :
                                                      NULL),
                                             TRUE,
                                             &initial_projection,
                                             &initial_projection_ptr,
                                             &initial_modelview,
                                             &initial_modelview_ptr);
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseMatrixTicks += cpuGetTiming() - phase_mark;
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 1)
    gNdsRendererProfileMatrixTicks += cpuGetTiming() - step_start;
#endif

    config.max_depth = 8u;
    config.max_commands = 8192u;
    config.max_list_commands = 512u;
    config.initial_projection = initial_projection_ptr;
    config.initial_modelview = initial_modelview_ptr;
    config.initial_geometry_mode = 0u;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsRendererAdapterStageValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLDrawResolveBranch;
    config.resolve_data = ndsFighterDLDrawResolveRendererData;
    config.user = &state;
    callback = (ndsRendererHardwareNoOracleEnabled() != FALSE) ?
        NULL : ndsFighterMarioFoxVisitDLDrawCommand;
    callback_user = &state;

    render_stats = &stats;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        render_stats = &sNdsRendererAdapterStagePersistentStats;
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawResetTransientRendererStats(render_stats);
        }
        else
        {
            ndsFighterDLDrawResetRuntimeRendererStats(render_stats);
        }
    }
    else
    {
        ndsRendererInitStats(render_stats);
    }
#else
    ndsRendererInitStats(render_stats);
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        ndsFighterDLDrawCopyPersistentRendererState(
            render_stats, &sNdsRendererAdapterStagePersistentStats);
    }
#endif
#endif
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererOwnerSnapshotStats(render_stats, &owner_stats_before);
    step_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileSetSourceProvenance(
        sNdsRendererAdapterStageOwnerOccurrence,
        sNdsRendererAdapterStageListOrdinal,
        ndsRendererOwnerRootBranchPath(
            loaded, dl, sNdsRendererAdapterStageListOrdinal));
    sNdsRendererAdapterStageListOrdinal++;
#endif
#endif
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        /* Source order: the proc emits prim/env into the head stream and THEN
         * the model list, so these must land before the list executes and must
         * be allowed to be overridden by the list's own colour commands. */
        if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
        {
            render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
        }
        if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
        {
            render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
        }
        /* THE ONLY WRITER OF THE CAPTURED BLEND STATE, and it is inside the
         * effect-submit guard. Without this the effect layer inherits whatever
         * othermode_l the previous list left, which is an opaque stage mode,
         * and ndsRendererHardwareAlpha takes its alpha-31 early return for
         * every effect polygon. */
        if (sNdsRendererAdapterEffectOtherModeValid != 0u)
        {
            render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
        }
        /* Latched HERE, not at the display-proc marker, because the marker
         * publishes the layer's sticky value and a probe stopping at one effect
         * would read another effect's mode. This is the mode THIS list starts
         * with; the Out latch below is what it finishes with. */
        gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        effect_seed_before = render_stats->hardware_matrix_seed_count;
        effect_matrix_cmd_before = render_stats->matrix_command_count;
        effect_xform_before = render_stats->transformed_vertex_count;
        effect_hw_vertex_before = render_stats->hardware_vertex_count;
        effect_hw_triangle_before = render_stats->hardware_triangle_count;
    }
    if (sNdsRendererAdapterItemSubmitActive != FALSE)
    {
        u32 head = (sNdsRendererAdapterItemSubmitHead <
                    NDS_RENDERER_STAGE_DL_HEADS) ?
            sNdsRendererAdapterItemSubmitHead : 0u;

        gNdsItemRendererLastHead = head;
        gNdsItemRendererLastColorMask =
            sNdsRendererAdapterItemColorMask[head];
        gNdsItemRendererLastEnvColor =
            sNdsRendererAdapterItemEnvColor[head];
        gNdsItemRendererLastOtherModeL =
            sNdsRendererAdapterItemOtherModeL[head];
        gNdsItemRendererLastOtherModeH =
            sNdsRendererAdapterItemOtherModeH[head];

        if ((sNdsRendererAdapterItemColorMask[head] & 1u) != 0u)
        {
            render_stats->prim_color = sNdsRendererAdapterItemPrimColor[head];
        }
        if ((sNdsRendererAdapterItemColorMask[head] & 2u) != 0u)
        {
            render_stats->env_color = sNdsRendererAdapterItemEnvColor[head];
        }
        if (sNdsRendererAdapterItemOtherModeLValid[head] != 0u)
        {
            render_stats->othermode_l =
                sNdsRendererAdapterItemOtherModeL[head];
        }
        if (sNdsRendererAdapterItemOtherModeHValid[head] != 0u)
        {
            render_stats->othermode_h =
                sNdsRendererAdapterItemOtherModeH[head];
        }
    }
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        /* Armed OUTSIDE the Exec tick bracket so the capture's own setup is not
         * charged to Exec. The per-word recording inside it necessarily is,
         * which is why this build's Exec ticks are not a performance reading --
         * this run is about the stream's SHAPE, not its cost. */
        ndsEffectPacketCaptureBegin();
        phase_mark = cpuGetTiming();
    }
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    if (rebirth_halo_native_candidate != FALSE)
    {
        rebirth_halo_native_handled = ndsRendererSubmitNativeRebirthHalo(
            rebirth_halo_root_offset, &config, render_stats);
        if (rebirth_halo_native_handled != FALSE)
        {
            gNdsRebirthHaloNativeDrawCount++;
        }
        else
        {
            gNdsRebirthHaloNativeFallbackCount++;
        }
    }
#endif
#if NDS_R2_IMPACT_WAVE_NATIVE
    if (
#if NDS_R2_REBIRTH_HALO_NATIVE
        (rebirth_halo_native_handled == FALSE) &&
#endif
        (impact_wave_native_candidate != FALSE))
    {
        impact_wave_native_handled = ndsRendererSubmitNativeImpactWave(
            sNdsImpactWaveVertices,
            (u32)(sizeof(sNdsImpactWaveVertices) /
                  sizeof(sNdsImpactWaveVertices[0])),
            sNdsImpactWaveTriangles,
            (u32)(sizeof(sNdsImpactWaveTriangles) / 3u),
            dl,
            &impact_wave_material,
            sNdsRendererAdapterImpactWaveVariant,
            &config,
            render_stats);
    }
    if (
#if NDS_R2_REBIRTH_HALO_NATIVE
        (rebirth_halo_native_handled == FALSE) &&
#endif
        (impact_wave_native_handled == FALSE))
    {
        if (impact_wave_native_candidate != FALSE)
        {
            /* Native rejection occurs before any mesh GX emit (near-plane or
             * malformed-contract fallback). Rebuild the source segment-E table
             * only on that rare path, then execute the untouched interpreter. */
            ndsRendererAdapterPrepareMaterialSegment(dobj, &state);
            gNdsImpactWaveNativeFallbackCount++;
        }
        ndsRendererExecuteDisplayListWithVertexCache(
            dl,
            &config,
            callback,
            callback_user,
            render_stats,
            (sNdsRendererAdapterStagePersistentActive != FALSE) ?
                &sNdsRendererAdapterStageVertexCache : NULL);
    }
    else
    {
        gNdsImpactWaveNativeDrawCount++;
    }
#else
#if NDS_R2_REBIRTH_HALO_NATIVE
    if (rebirth_halo_native_handled == FALSE)
#endif
    {
        ndsRendererExecuteDisplayListWithVertexCache(
            dl,
            &config,
            callback,
            callback_user,
            render_stats,
            (sNdsRendererAdapterStagePersistentActive != FALSE) ?
                &sNdsRendererAdapterStageVertexCache : NULL);
    }
#endif
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseExecTicks += cpuGetTiming() - phase_mark;
        ndsEffectPacketCaptureEnd();
    }
#endif
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        /* The config's matrices read from OUR locals, and the executor's own
         * verdict on them read as a delta. Cycles 53-55 tried to read the same
         * two pointers out of the callee's argument register and got three
         * different answers; nds_effects.h records why that read can never
         * settle it. */
        /* Out - In names an asset list that carries its own render mode: the
         * rebirth halo's DObj entry[2] list-0 leaves set TEX_EDGE and restore
         * OPA_SURF (85.vpk0.bin 0x23a8/0x2490), while its list-1 beam
         * (0x2890) emits no G_SETOTHERMODE_L at all and leaves Out == In. */
        gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
        gNdsEffectDLSubmitCount++;
        gNdsEffectDLCfgMask =
            ((config.initial_projection != NULL) ? 1u : 0u) |
            ((config.initial_modelview != NULL) ? 2u : 0u);
        if (config.initial_modelview != NULL)
        {
            gNdsEffectDLCfgMvT[0] = config.initial_modelview->m[3][0];
            gNdsEffectDLCfgMvT[1] = config.initial_modelview->m[3][1];
            gNdsEffectDLCfgMvT[2] = config.initial_modelview->m[3][2];
        }
        gNdsEffectDLMatrixSeed =
            render_stats->hardware_matrix_seed_count - effect_seed_before;
        gNdsEffectDLMatrixCmd =
            render_stats->matrix_command_count - effect_matrix_cmd_before;
        gNdsEffectDLXformVertexCount =
            render_stats->transformed_vertex_count - effect_xform_before;
        gNdsEffectDLHwVertexCount =
            render_stats->hardware_vertex_count - effect_hw_vertex_before;
        gNdsEffectDLHwTriangleCount =
            render_stats->hardware_triangle_count - effect_hw_triangle_before;
#if NDS_RENDERER_HW_TRIANGLES
        if (sNdsRendererAdapterStagePersistentActive != FALSE)
        {
            gNdsEffectDLVtx0[0] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].x;
            gNdsEffectDLVtx0[1] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].y;
            gNdsEffectDLVtx0[2] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].z;
            gNdsEffectDLVtx0[3] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].w;
        }
#endif
        gNdsEffectDLBlocker = render_stats->blocker;
        gNdsEffectDLCommandCount = render_stats->command_count;
#if NDS_TICK_HUD
        /* R2-08 CAP-VERSUS-END. The two lines above are LAST-VALUE-WINS, so a
         * stop reads one list; these are cumulative, so a stop reads the whole
         * window. The question they settle is whether the interpreter stops at
         * the list's end or runs to config->max_commands (8192): the executor
         * sets blocker = BUDGET on exactly that path (nds_renderer.c:28303),
         * and BLOCKER_NONE means it reached G_ENDDL under its own steam.
         *
         * This exists because an arithmetic COINCIDENCE nearly bought a
         * deferral: 8192 x 12.54 = 102,727 against a measured 102,730 per list
         * looks like proof the loop runs to its cap, but the 12.54 was obtained
         * by dividing 102,730 BY 8192, so the agreement is a tautology and
         * carries no information. Mean commands per list is the honest form of
         * the same question and it is two adds. */
        gNdsEffectDLCommandTotal += render_stats->command_count;
        if (render_stats->blocker == NDS_RENDERER_BLOCKER_BUDGET)
        {
            gNdsEffectDLTermCapCount++;
        }
        else if (render_stats->blocker == NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsEffectDLTermEndCount++;
        }
        else
        {
            gNdsEffectDLTermOtherCount++;
            gNdsEffectDLTermOtherMask |= 1u << (render_stats->blocker & 31u);
        }
#endif
        gNdsEffectDLFirstOpcode = render_stats->first_opcode;
        gNdsEffectDLUnsupportedOpcode = render_stats->unsupported_opcode;
        /* vertex_count/triangle_count, NOT the *_command_count pair: every
         * site that increments those is wrapped in
         * NDS_RENDERER_RECORD_PROOF_ONLY, which is ((void)0) whenever
         * NDS_RENDERER_HW_TRIANGLES is set -- i.e. dead in every build that
         * can draw. Reading them cost this investigation one wrong conclusion. */
        gNdsEffectDLVertexCount = render_stats->vertex_count;
        gNdsEffectDLTriangleCount = render_stats->triangle_count;
        gNdsEffectDLPublishCount++;
#if NDS_TICK_HUD
        /* Cumulative twins of the two last-value-wins deltas above: a stop reads
         * one list from those, and the census needs the whole window. */
        {
            u32 census_tris = render_stats->hardware_triangle_count -
                effect_hw_triangle_before;
            u32 census_verts = render_stats->hardware_vertex_count -
                effect_hw_vertex_before;

            gNdsEffectDLTriangleTotal += census_tris;
            gNdsEffectDLVertexTotal += census_verts;
            /* OtherModeIn was latched for THIS list before the executor ran, so
             * it is the entry state; render_stats->othermode_l is now exit. */
            ndsEffectDLCensusRecord(dl, render_stats->command_count,
                                    (u32)gNdsEffectDLSubmitOtherModeIn,
                                    census_tris, census_verts);
            /* Same key, same instant, same population as the census above. If
             * the capture's own arming condition ever disagreed with this one,
             * gNdsEffectPacketCaptureCount would diverge from
             * gNdsEffectDLSubmitCount -- both are published, so the
             * disagreement would be visible rather than silent. */
            ndsEffectPacketVerdictRecord(dl);
        }
#endif
    }
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererAdapterAccumulateDepth(
        render_stats,
        &gNdsRendererDepthStageSamples,
        &gNdsRendererDepthStageMin,
        &gNdsRendererDepthStageMax,
        &gNdsRendererDepthStageWMin,
        &gNdsRendererDepthStageWMax);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileDLTicks += cpuGetTiming() - step_start;
    adapter_ticks = cpuGetTiming() - adapter_start;
    gNdsRendererProfileStageAdapterTicks += adapter_ticks;
    ndsRendererOwnerAccumulateList(
        NDS_RENDERER_PROFILE_OWNER_STAGE, loaded, dl,
        gNdsRendererProfileOwners[
            NDS_RENDERER_PROFILE_OWNER_STAGE].selected_count,
        initial_projection_ptr, initial_modelview_ptr,
        &config,
        &owner_stats_before, render_stats);
#endif
    /* P2-3r13: the fighter's own graphics-heap peak, before it is rolled back
     * and becomes invisible to the end-of-frame sample. */
    ndsTaskmanSampleGraphicsHeap();
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawCapturePersistentState(
                &sNdsRendererAdapterStagePersistentState, &state);
        }
        else
        {
            sNdsRendererAdapterStagePersistentState.segment_e_base =
                state.segment_e_base;
            sNdsRendererAdapterStagePersistentState.segment_e_end =
                state.segment_e_end;
        }
#else
        ndsFighterDLDrawCapturePersistentState(
            &sNdsRendererAdapterStagePersistentState, &state);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsFighterDLDrawCopyPersistentRendererState(
            &sNdsRendererAdapterStagePersistentStats, render_stats);
#endif
        gNdsStageGCDrawAllLoopHardwareCarryCaptureCount++;
        if (render_stats->command_count <= 5u)
        {
            if (inherited_texture != FALSE)
            {
                gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount++;
            }
            if (inherited_tile != FALSE)
            {
                gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount++;
            }
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if ((gNdsRendererProfileHardwareTriangles > 2048u) ||
        (gNdsRendererProfileHardwareVertices > 6144u))
    {
        gNdsRendererProfileHardwareOverLimit = 1u;
    }
#endif
#endif
    gNdsStageGCDrawAllLoopHardwareTriangleCount +=
        render_stats->hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        render_stats->hardware_zbuffer_triangle_count;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
        render_stats->hardware_projected_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
        render_stats->hardware_decal_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
        render_stats->hardware_texture_bind_count;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
        render_stats->hardware_texture_upload_count;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
        render_stats->hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
        render_stats->hardware_texture_reject_count;
    if (render_stats->hardware_texture_ready_count != 0u)
    {
        if (render_stats->hardware_texture_format < 32u)
        {
            gNdsStageGCDrawAllLoopHardwareTextureFormatMask |=
                1u << render_stats->hardware_texture_format;
        }
        if (render_stats->hardware_texture_width >
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth =
                render_stats->hardware_texture_width;
        }
        if (render_stats->hardware_texture_height >
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight =
                render_stats->hardware_texture_height;
        }
    }
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseDLTicks += cpuGetTiming() - phase_dl_mark;
    }
#endif
}

/* ONE NODE IS NOT A TREE, AND THAT IS WHY THE SHIELD WAS A QUARTER CIRCLE.
 *
 * This submitted the DObj it was handed and stopped. The source does not:
 * gcDrawDObjTree (objdisplay.c) recurses into `child` and, from the first
 * sibling, walks the whole `sib_next` chain. Every multi-node effect model
 * therefore drew only its ROOT node here -- the owner's own words on the first
 * build that routed the real asset were "it is using the correct asset but its
 * only like 1/2 or 1/4 of it, like a 1/4 slice of the complete circle".
 *
 * That single omission is what four BUGS.md rows have in common. The shield,
 * Fox's reflector, the rebirth halo and the impact wave are all EFDescs whose
 * geometry is a DObj tree, so all four were being drawn one node deep, and no
 * amount of atlas resolution or palette work could ever have shown the rest of
 * them.
 *
 * The sibling rule is the source's, kept exactly: only a node with no
 * `sib_prev` walks the chain, so a tree is traversed once rather than once per
 * sibling. Recursion depth is the model's own node depth, which these effect
 * descs keep shallow.
 *
 * NOT copied from the source: gcPrepDObjMatrix and the matching gSPPopMatrix.
 * The DS path composes its transform inside ndsRendererAdapterSubmitStageDL
 * per display list rather than pushing an N64 matrix stack here, so adding a
 * push/pop pair around the recursion would double-transform every child. */
static void ndsRendererAdapterSubmitStageDObjNode(DObj *dobj, u32 kind,
                                                  GObj *camera_gobj,
                                                  u32 initial_geometry_mode);

/* BOUNDED, because this walks a tree the PORT builds from resolved offsets and
 * not one the N64 shipped. Those offsets have held raw symbol addresses before
 * -- the note at Makefile:1393 records gcSetupCustomDObjs walking garbage and
 * allocating a DObj per bogus node until the allocator gave up -- so a cycle or
 * a wild pointer here is a real possibility, and unbounded recursion over one
 * is a hung handheld rather than a wrong picture. The first build of this walk
 * timed out a 300-second probe, which is exactly that failure.
 *
 * The limits are far above any real effect model (these descs are a root plus a
 * handful of parts) and both overruns are counted, so hitting one is a
 * diagnosable defect instead of a freeze. */
#define NDS_RENDERER_STAGE_DOBJ_MAX_DEPTH 16u
#define NDS_RENDERER_STAGE_DOBJ_MAX_SIBLINGS 64u

/* The three counters live in diagnostics.c, not here. Defining them inside this
 * `#if NDS_RENDERER_HW_TRIANGLES` block would make them exist only in the
 * configurations that increment them, and a probe naming an absent symbol loses
 * its whole gdb run. nds_effects.h declares them; cliff_ledge.c resets them,
 * which is also what keeps --gc-sections from collecting them. */

static void ndsRendererAdapterSubmitStageDObjTreeDepth(
    DObj *dobj, u32 kind, GObj *camera_gobj, u32 initial_geometry_mode,
    u32 depth)
{
    DObj *sibling;
    u32 seen;

    if (dobj == NULL)
    {
        return;
    }
    if (depth >= NDS_RENDERER_STAGE_DOBJ_MAX_DEPTH)
    {
        gNdsRendererStageDObjDepthOverrunCount++;
        return;
    }
    gNdsRendererStageDObjNodeCount++;
#if NDS_TICK_HUD
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        gNdsEffectPhaseNodeCount++;
    }
#endif
    /* BattleShip gcDrawDObjTree makes HIDDEN a subtree visibility flag: the
     * node's own draw AND its child walk live inside the same !HIDDEN block.
     * The sibling walk is outside that block, so a hidden node must not hide
     * its siblings.  The port used to submit the node through the drawable
     * gate but recurse into its child unconditionally; Mario's pipe exposes
     * that at source frame 100, when the rim becomes HIDDEN for the final 20
     * frames but its barrel child was still emitted as a flat gray/white slab.
     * Keep NOTEXTURE behavior unchanged: it suppresses only this node's DL,
     * not descendants. */
    if ((dobj->flags & DOBJ_FLAG_HIDDEN) == 0u)
    {
        ndsRendererAdapterSubmitStageDObjNode(dobj, kind, camera_gobj,
                                              initial_geometry_mode);
        if (dobj->child != NULL)
        {
            ndsRendererAdapterSubmitStageDObjTreeDepth(
                dobj->child, kind, camera_gobj, initial_geometry_mode,
                depth + 1u);
        }
    }
    if (dobj->sib_prev == NULL)
    {
        seen = 0u;
        for (sibling = dobj->sib_next; sibling != NULL;
             sibling = sibling->sib_next)
        {
            if (++seen > NDS_RENDERER_STAGE_DOBJ_MAX_SIBLINGS)
            {
                gNdsRendererStageDObjSiblingOverrunCount++;
                break;
            }
            ndsRendererAdapterSubmitStageDObjTreeDepth(
                sibling, kind, camera_gobj, initial_geometry_mode,
                depth + 1u);
        }
    }
}

/* EXPLICIT TREE OWNERS ONLY, AND THE MEASUREMENT IS WHY.
 *
 * The first version of this recursed inside ndsRendererAdapterSubmitStageDObj,
 * which is the STAGE entry point -- the stage, the weapons and the effects all
 * reach the hardware through it. A synchronized tick-HUD A/B on identical
 * frames priced that at one whole VBlank:
 *
 *     frame   control      candidate
 *       441   1,119,936    1,120,000
 *       443   1,119,872    1,120,000
 *       447   1,119,488    1,680,384   <- 2 VBlanks -> 3
 *       449   1,119,872    1,680,256   <- 2 VBlanks -> 3
 *
 * +560,896 ticks is 560,190-per-VBlank almost exactly, on frames that were
 * inside the 1.12M gate. The stage carries 57 DObjs (M3_NATIVE_STAGE_CHECK
 * dobjs=57) and the native stage path already handles their geometry, so
 * re-walking them bought nothing and cost a frame.
 *
 * The source models that actually use tree display callbacks still need their
 * descendants. So recursion lives on the explicit effect/item/weapon call
 * sites, while the shared stage entry stays a single-node submit. This keeps
 * the measured stage regression out of normal frames without flattening a
 * source DObj tree such as Link's Boomerang into its transform-only root. */
void ndsRendererAdapterSubmitEffectDObjTree(void *dobj_ptr, u32 kind,
                                            void *camera_gobj_ptr,
                                            u32 initial_geometry_mode)
{
#if NDS_R2_IMPACT_WAVE_NATIVE || NDS_R2_REBIRTH_HALO_NATIVE
    DObj *root = (DObj *)dobj_ptr;
#endif
#if NDS_R2_IMPACT_WAVE_NATIVE

    sNdsRendererAdapterImpactWaveVariant = 0u;
    sNdsRendererAdapterImpactWaveNativeActive =
        ((root != NULL) && (root->parent_gobj != NULL) &&
         (ndsEFManagerImpactWaveVariant(
              root->parent_gobj,
              &sNdsRendererAdapterImpactWaveVariant) != FALSE)) ?
            TRUE : FALSE;
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    sNdsRendererAdapterRebirthHaloNativeActive =
        ((root != NULL) && (root->xobjs_num != 0) &&
         (root->xobjs[0] != NULL) &&
         (root->xobjs[0]->kind == NDS_RENDERER_ADAPTER_JOINT_ATTACH_TRA_MTX_KIND) &&
         (root->child != NULL) && (gEFManagerFiles[2] != NULL) &&
         (root->child->dl_link ==
              (DObjDLLink *)((u8 *)gEFManagerFiles[2] + 0x2a98u))) ?
            TRUE : FALSE;
#if NDS_R2_REBIRTH_HALO_FAST_ADAPTER
    sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
#endif
#endif
    sNdsRendererAdapterEffectSubmitActive = TRUE;
#if NDS_TICK_HUD
    gNdsEffectPhaseActive = 1u;
#endif
    ndsRendererAdapterSubmitStageDObjTreeDepth(dobj_ptr, kind, camera_gobj_ptr,
                                               initial_geometry_mode, 0u);
#if NDS_TICK_HUD
    gNdsEffectPhaseActive = 0u;
#endif
    sNdsRendererAdapterEffectSubmitActive = FALSE;
#if NDS_R2_IMPACT_WAVE_NATIVE
    sNdsRendererAdapterImpactWaveNativeActive = FALSE;
    sNdsRendererAdapterImpactWaveVariant = 0u;
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    sNdsRendererAdapterRebirthHaloNativeActive = FALSE;
#if NDS_R2_REBIRTH_HALO_FAST_ADAPTER
    sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
#endif
#endif
}

void ndsRendererAdapterSubmitItemDObjTree(void *dobj_ptr, u32 kind,
                                          void *camera_gobj_ptr,
                                          u32 initial_geometry_mode)
{
    sNdsRendererAdapterItemSubmitActive = TRUE;
    sNdsRendererAdapterItemSubmitHead = 0u;
    ndsRendererAdapterSubmitStageDObjTreeDepth(dobj_ptr, kind, camera_gobj_ptr,
                                               initial_geometry_mode, 0u);
    sNdsRendererAdapterItemSubmitActive = FALSE;
    sNdsRendererAdapterItemSubmitHead = 0u;
}

void ndsRendererAdapterSubmitWeaponDObjTree(void *dobj_ptr, u32 kind,
                                            void *camera_gobj_ptr,
                                            u32 initial_geometry_mode)
{
    ndsRendererAdapterSubmitStageDObjTreeDepth(dobj_ptr, kind, camera_gobj_ptr,
                                               initial_geometry_mode, 0u);
}

void ndsRendererAdapterSubmitStageDObj(void *dobj_ptr, u32 kind,
                                       void *camera_gobj_ptr,
                                       u32 initial_geometry_mode)
{
    ndsRendererAdapterSubmitStageDObjNode(dobj_ptr, kind, camera_gobj_ptr,
                                          initial_geometry_mode);
}

static void ndsRendererAdapterSubmitStageDObjNode(DObj *dobj, u32 kind,
                                                  GObj *camera_gobj,
                                                  u32 initial_geometry_mode)
{
    DObjDLLink *dl_link;
    u32 i;

    if (ndsRendererAdapterStageDObjDrawable(dobj, kind) == FALSE)
    {
        return;
    }

    switch (kind)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
        if (dobj->dv != NULL)
        {
            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                sNdsRendererAdapterItemSubmitHead =
                    (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1) ?
                        1u : 0u;
            }
            ndsRendererAdapterSubmitStageDL(dobj, dobj->dl, camera_gobj,
                                            initial_geometry_mode);
        }
        break;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
        dl_link = dobj->dl_link;
        if (dl_link == NULL)
        {
            return;
        }
        for (i = 0u; i < GC_COMMON_MAX_DLLINKS; i++, dl_link++)
        {
            if (dl_link->list_id == (s32)NDS_RENDERER_STAGE_DL_HEADS)
            {
                break;
            }
            if ((dl_link->list_id >= 0) &&
                ((u32)dl_link->list_id < NDS_RENDERER_STAGE_DL_HEADS) &&
                (dl_link->dl != NULL))
            {
                if (sNdsRendererAdapterItemSubmitActive != FALSE)
                {
                    sNdsRendererAdapterItemSubmitHead =
                        (u32)dl_link->list_id;
                }
                ndsRendererAdapterSubmitStageDL(dobj, dl_link->dl,
                                                camera_gobj,
                                                initial_geometry_mode);
            }
        }
        break;

    default:
        break;
    }
}

#else
void ndsRendererAdapterBeginStageTraversal(void)
{
}

void ndsRendererAdapterEndStageTraversal(void)
{
}

void ndsRendererAdapterSubmitStageDObj(void *dobj, u32 kind,
                                       void *camera_gobj,
                                       u32 initial_geometry_mode)
{
    (void)dobj;
    (void)kind;
    (void)camera_gobj;
    (void)initial_geometry_mode;
}

void ndsRendererAdapterSubmitEffectDObjTree(void *dobj, u32 kind,
                                            void *camera_gobj,
                                            u32 initial_geometry_mode)
{
    (void)dobj;
    (void)kind;
    (void)camera_gobj;
    (void)initial_geometry_mode;
}

void ndsRendererAdapterSubmitWeaponDObjTree(void *dobj, u32 kind,
                                            void *camera_gobj,
                                            u32 initial_geometry_mode)
{
    (void)dobj;
    (void)kind;
    (void)camera_gobj;
    (void)initial_geometry_mode;
}

void ndsRendererAdapterSubmitItemDObjTree(void *dobj, u32 kind,
                                          void *camera_gobj,
                                          u32 initial_geometry_mode)
{
    (void)dobj;
    (void)kind;
    (void)camera_gobj;
    (void)initial_geometry_mode;
}

void ndsRendererAdapterMarkDisplayProcHeads(void)
{
}

void ndsRendererAdapterCaptureDisplayProcColors(void)
{
}

void ndsRendererAdapterCaptureItemDisplayProcState(void)
{
}

#endif
