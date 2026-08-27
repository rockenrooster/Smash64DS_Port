
static u32 ndsFighterDLExecReadU32(const void *ptr)
{
    const u8 *bytes = ptr;

    return (u32)bytes[0] |
           ((u32)bytes[1] << 8) |
           ((u32)bytes[2] << 16) |
           ((u32)bytes[3] << 24);
}











static const Gfx *ndsFighterDLDrawResolveBranch(const Gfx *dl,
                                                 u32 *resolve_kind,
                                                 void *user)
{
    NDSFighterDLDrawState *state = user;
    uintptr_t raw = (uintptr_t)dl;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if (resolve_kind != NULL)
    {
        *resolve_kind = NDS_RENDERER_RESOLVE_NONE;
    }
    if ((ndsRelocFindLoadedFileContaining(dl, sizeof(Gfx)) != NULL) ||
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(Gfx)) != FALSE))
    {
        return dl;
    }
    if ((state != NULL) && (state->segment_e_base != NULL) &&
        ((raw >> 24) == 0x0eu))
    {
        uintptr_t base = (uintptr_t)state->segment_e_base;
        uintptr_t end = (uintptr_t)state->segment_e_end;

        if ((end > base) && (offset <= (end - base)) &&
            (sizeof(Gfx) <= (size_t)(end - base - offset)))
        {
            if (resolve_kind != NULL)
            {
                *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
            }
            return (const Gfx *)(base + offset);
        }
    }
    if ((raw >> 24) == 0x0eu)
    {
        if (resolve_kind != NULL)
        {
            *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
        }
        return sNdsRendererAdapterEmptySegmentEDL;
    }
    if ((state != NULL) &&
        (state->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(state->primary_file,
                                   offset,
                                   sizeof(Gfx)) != FALSE))
    {
        if (resolve_kind != NULL)
        {
            *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
        }
        return (const Gfx *)((const u8 *)state->primary_file->data + offset);
    }
    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      sizeof(Gfx)) != FALSE)
        {
            if (resolve_kind != NULL)
            {
                *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
            }
            return (const Gfx *)((const u8 *)sNdsRelocLoadedFiles[i].data +
                                 offset);
        }
    }
    return dl;
}

static const void *ndsFighterDLDrawResolveDataPointer(uintptr_t raw,
                                                      size_t bytes,
                                                      NDSFighterDLDrawState *state)
{
    const void *ptr = (const void *)raw;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if ((ndsRelocFindLoadedFileContaining(ptr, bytes) != NULL) ||
        (ndsFighterDLScanRangeInTaskmanArena(ptr, bytes) != FALSE))
    {
        return ptr;
    }
    if ((state != NULL) && (state->segment_e_base != NULL) &&
        ((raw >> 24) == 0x0eu))
    {
        uintptr_t base = (uintptr_t)state->segment_e_base;
        uintptr_t end = (uintptr_t)state->segment_e_end;

        if ((end > base) && (offset <= (end - base)) &&
            (bytes <= (size_t)(end - base - offset)))
        {
            return (const void *)(base + offset);
        }
    }
    if ((raw >> 24) == 0x0eu)
    {
        return NULL;
    }
    if ((state != NULL) &&
        (state->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(state->primary_file,
                                   offset,
                                   bytes) != FALSE))
    {
        return (const u8 *)state->primary_file->data + offset;
    }
    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      bytes) != FALSE)
        {
            return (const u8 *)sNdsRelocLoadedFiles[i].data + offset;
        }
    }
    return NULL;
}

static const void *ndsFighterDLDrawResolveRendererData(const void *ptr,
                                                       size_t bytes,
                                                       void *user)
{
    return ndsFighterDLDrawResolveDataPointer((uintptr_t)ptr, bytes, user);
}


static void ndsFighterDLDrawDecodeVtx(NDSFighterDLDrawState *state,
                                      u32 index, const u8 *src)
{
    NDSFighterDLDrawVtx *dst;
    u32 xy;
    u32 zf;
    u32 st;
    u32 rgba;

    if ((state == NULL) || (src == NULL) ||
        (index >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
    {
        return;
    }

    dst = &state->vertices[index];
    xy = ndsFighterDLExecReadU32(src + 0);
    zf = ndsFighterDLExecReadU32(src + 4);
    st = ndsFighterDLExecReadU32(src + 8);
    rgba = ndsFighterDLExecReadU32(src + 12);

    dst->x = (s16)(xy >> 16);
    dst->y = (s16)(xy & 0xffffu);
    dst->z = (s16)(zf >> 16);
    dst->s = (s16)(st >> 16);
    dst->t = (s16)(st & 0xffffu);
    dst->r = rgba >> 24;
    dst->g = rgba >> 16;
    dst->b = rgba >> 8;
    dst->a = rgba;
    if (dst->a == 0)
    {
        dst->a = 0xffu;
    }
    dst->valid = TRUE;

    state->vertex_valid_mask |= 1u << index;
    state->vertex_decoded_count++;
    state->color_checksum =
        (state->color_checksum * 33u) ^
        (u32)((u16)dst->x + ((u16)dst->y << 1) + ((u16)dst->z << 2)) ^
        rgba;
}

static u32 ndsFighterDLDrawCountValidVertices(u32 mask)
{
    u32 count = 0u;

    while (mask != 0u)
    {
        count += mask & 1u;
        mask >>= 1;
    }
    return count;
}

static void ndsFighterDLDrawSeedPersistentState(
    NDSFighterDLDrawState *state, const NDSFighterDLDrawState *persistent)
{
    if ((state == NULL) || (persistent == NULL))
    {
        return;
    }

    state->segment_e_base = persistent->segment_e_base;
    state->segment_e_end = persistent->segment_e_end;
    memcpy(state->vertices, persistent->vertices, sizeof(state->vertices));
    state->vertex_valid_mask = persistent->vertex_valid_mask;
    state->vertex_decoded_count =
        ndsFighterDLDrawCountValidVertices(state->vertex_valid_mask);
}

static void ndsFighterDLDrawCapturePersistentState(
    NDSFighterDLDrawState *persistent, const NDSFighterDLDrawState *state)
{
    if ((persistent == NULL) || (state == NULL))
    {
        return;
    }

    persistent->segment_e_base = state->segment_e_base;
    persistent->segment_e_end = state->segment_e_end;
    memcpy(persistent->vertices, state->vertices,
           sizeof(persistent->vertices));
    persistent->vertex_valid_mask = state->vertex_valid_mask;
}

static void ndsFighterDLDrawCopyPersistentRendererState(
    NDSRendererStats *dst, const NDSRendererStats *src)
{
#define NDS_RENDERER_COPY_STATE(field) dst->field = src->field

    if ((dst == NULL) || (src == NULL))
    {
        return;
    }

    NDS_RENDERER_COPY_STATE(othermode_h);
    NDS_RENDERER_COPY_STATE(othermode_l);
    NDS_RENDERER_COPY_STATE(geometry_mode);
    NDS_RENDERER_COPY_STATE(geometry_clear_mask);
    NDS_RENDERER_COPY_STATE(geometry_set_mask);
    NDS_RENDERER_COPY_STATE(texture_load_kind);
    NDS_RENDERER_COPY_STATE(texture_scale_s);
    NDS_RENDERER_COPY_STATE(texture_scale_t);
    NDS_RENDERER_COPY_STATE(texture_level);
    NDS_RENDERER_COPY_STATE(texture_tile);
    NDS_RENDERER_COPY_STATE(texture_on);
    NDS_RENDERER_COPY_STATE(texture_xparam);
    NDS_RENDERER_COPY_STATE(texture_state_flags);
    NDS_RENDERER_COPY_STATE(texture_image);
    NDS_RENDERER_COPY_STATE(texture_format);
    NDS_RENDERER_COPY_STATE(texture_size);
    NDS_RENDERER_COPY_STATE(texture_image_width);
    NDS_RENDERER_COPY_STATE(texture_tlut_image);
    NDS_RENDERER_COPY_STATE(texture_tlut_count);
    NDS_RENDERER_COPY_STATE(texture_tlut_tile);
    NDS_RENDERER_COPY_STATE(texture_render_tile);
    NDS_RENDERER_COPY_STATE(texture_render_tile_format);
    NDS_RENDERER_COPY_STATE(texture_render_tile_size);
    NDS_RENDERER_COPY_STATE(texture_render_tile_line);
    NDS_RENDERER_COPY_STATE(texture_render_tile_tmem);
    NDS_RENDERER_COPY_STATE(texture_render_tile_palette);
    NDS_RENDERER_COPY_STATE(texture_render_tile_cms);
    NDS_RENDERER_COPY_STATE(texture_render_tile_cmt);
    NDS_RENDERER_COPY_STATE(texture_render_tile_masks);
    NDS_RENDERER_COPY_STATE(texture_render_tile_maskt);
    NDS_RENDERER_COPY_STATE(texture_render_tile_shifts);
    NDS_RENDERER_COPY_STATE(texture_render_tile_shiftt);
    NDS_RENDERER_COPY_STATE(texture_render_tile_flags);
    NDS_RENDERER_COPY_STATE(texture_load_tile);
    NDS_RENDERER_COPY_STATE(texture_load_block_uls);
    NDS_RENDERER_COPY_STATE(texture_load_block_ult);
    NDS_RENDERER_COPY_STATE(texture_load_block_lrs);
    NDS_RENDERER_COPY_STATE(texture_load_block_dxt);
    NDS_RENDERER_COPY_STATE(texture_load_texels);
    NDS_RENDERER_COPY_STATE(texture_tile_size_tile);
    NDS_RENDERER_COPY_STATE(texture_tile_size_uls);
    NDS_RENDERER_COPY_STATE(texture_tile_size_ult);
    NDS_RENDERER_COPY_STATE(texture_tile_size_lrs);
    NDS_RENDERER_COPY_STATE(texture_tile_size_lrt);
    NDS_RENDERER_COPY_STATE(texture_tile_width);
    NDS_RENDERER_COPY_STATE(texture_tile_height);
    /* The tile-sync memo's two words travel with the state they describe. This
     * copy carries texture_tiles[] and every republished texture_render_tile_*
     * field, so carrying the serials keeps dst's memo exact; dropping either
     * one would let dst skip a republish it still owes. */
    NDS_RENDERER_COPY_STATE(texture_tile_write_serial);
    NDS_RENDERER_COPY_STATE(texture_tile_sync_serial);
    memcpy(dst->texture_tiles, src->texture_tiles, sizeof(dst->texture_tiles));
    NDS_RENDERER_COPY_STATE(texture_load_sequence);
    memcpy(dst->texture_loads, src->texture_loads,
           sizeof(dst->texture_loads));
    NDS_RENDERER_COPY_STATE(texture_combine_w0);
    NDS_RENDERER_COPY_STATE(texture_combine_w1);
    NDS_RENDERER_COPY_STATE(texture_combine_count);
    NDS_RENDERER_COPY_STATE(prim_color);
    NDS_RENDERER_COPY_STATE(prim_min_level);
    NDS_RENDERER_COPY_STATE(prim_lod_fraction);
    NDS_RENDERER_COPY_STATE(env_color);
    NDS_RENDERER_COPY_STATE(blend_color);
    NDS_RENDERER_COPY_STATE(light_color_1);
    NDS_RENDERER_COPY_STATE(light_color_2);
    NDS_RENDERER_COPY_STATE(light_color_mask);
    NDS_RENDERER_COPY_STATE(light_dir_x);
    NDS_RENDERER_COPY_STATE(light_dir_y);
    NDS_RENDERER_COPY_STATE(light_dir_z);
    NDS_RENDERER_COPY_STATE(light_dir_mask);
    NDS_RENDERER_COPY_STATE(prim_depth);
    NDS_RENDERER_COPY_STATE(prim_depth_delta);
    NDS_RENDERER_COPY_STATE(fog_color);
    NDS_RENDERER_COPY_STATE(fog_min);
    NDS_RENDERER_COPY_STATE(fog_max);
    NDS_RENDERER_COPY_STATE(fog_status);
    NDS_RENDERER_COPY_STATE(texture_source_hash1);
    NDS_RENDERER_COPY_STATE(texture_source_hash2);

#undef NDS_RENDERER_COPY_STATE
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static void ndsFighterDLDrawResetTransientRendererStats(
    NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return;
    }
#if NDS_TICK_HUD
    /* Cycle 98. Both call sites of this function sit in the `detailed_output`
     * arm of an if/else whose other arm calls the Runtime reset below, and the
     * native owner production path is itself gated on detailed_output == FALSE
     * (:13862). So this is predicted to read 0 on the gate arm, i.e. the seam
     * the board called "deletion-shaped" is predicted already dead. Runtime is
     * the control that makes that zero readable. */
    gNdsFtrPreResetTransient++;
#endif

    /* The prefix is MOSTLY per-list proof/counter output. The renderer state
     * begins at othermode_h and remains live across BattleShip's stage heads
     * and fighter-part lists. A few diagnostics are interleaved with that state
     * for the profile-2 ABI and are reset explicitly.
     *
     * CORRECTED 2026-07-30 (R2-07 R4f): this comment used to say "exclusively",
     * and that is wrong in a way that invites deleting the bzero. Three fields
     * before othermode_h are read for DECISIONS, not just published --
     * hardware_texture_format, hardware_texture_width, hardware_texture_height.
     * The R2-03 E12 texture memo compares `memo->format !=
     * stats->hardware_texture_format` (nds_renderer.c:18545), and two sites
     * build masks with `1u << stats->hardware_texture_format` (:8597, :11825).
     * Dropping this clear would leak the previous part list's texture format
     * and size into those decisions. Priced before anyone tries: the prefix is
     * ~75 u32, and the clear is ~3.1% of the Results frame (~52,000 ticks/tic),
     * so it is not worth the risk even if it were safe. */
    bzero(stats, offsetof(NDSRendererStats, othermode_h));
    stats->first_cull_w0 = 0u;
    stats->first_cull_w1 = 0u;
    stats->first_branch_dl = NULL;
    stats->first_resolved_branch_dl = NULL;
    stats->geometry_command_count = 0u;
    stats->texture_mask = 0u;
    stats->texture_command_count = 0u;
    stats->texture_set_tile_count = 0u;
    stats->prim_depth_command_count = 0u;
}

static void ndsFighterDLDrawResetRuntimeRendererStats(
    NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return;
    }
#if NDS_TICK_HUD
    gNdsFtrPreResetRuntime++;
#endif

    /* Profiles 0/1 keep the ordered RDP state below intact. The null-callback
     * path only needs traversal guards and the owner-level hardware totals;
     * historical command/matrix/color ledgers remain profile-2 output. */
    stats->blocker = NDS_RENDERER_BLOCKER_NONE;
    stats->command_count = 0u;
    stats->unsupported_command_count = 0u;
    stats->end_command_count = 0u;
    stats->hardware_triangle_count = 0u;
    stats->hardware_zbuffer_triangle_count = 0u;
    stats->hardware_projected_depth_triangle_count = 0u;
    stats->hardware_decal_depth_triangle_count = 0u;
    stats->hardware_texture_bind_count = 0u;
    stats->hardware_texture_upload_count = 0u;
    stats->hardware_texture_ready_count = 0u;
    stats->hardware_texture_reject_count = 0u;
}
#endif

static sb32 ndsFighterDLDrawAppendTriangle(NDSFighterDLDrawState *state,
                                           u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;
    u32 mask;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);

    if ((state == NULL) ||
        (state->triangle_count >= NDS_FIGHTER_DL_DRAW_MAX_TRIS))
    {
        return FALSE;
    }

    state->tris[state->triangle_count].v0 = i0;
    state->tris[state->triangle_count].v1 = i1;
    state->tris[state->triangle_count].v2 = i2;
    state->triangle_count++;

    if ((i0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
        (i1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
        (i2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
    {
        return FALSE;
    }
    mask = (1u << i0) | (1u << i1) | (1u << i2);
    if ((state->vertex_valid_mask & mask) != mask)
    {
        return FALSE;
    }
    state->triangle_valid_count++;
    return TRUE;
}

static s32 ndsFighterMarioFoxVisitDLDrawCommand(
    const NDSRendererCommand *command, void *user)
{
    NDSFighterDLDrawState *state = user;
    u32 op;

    if ((command == NULL) || (state == NULL))
    {
        return FALSE;
    }

    op = command->op;
    switch (op)
    {
    case NDS_FIGHTER_DL_OP_NOOP:
        return TRUE;

    case NDS_FIGHTER_DL_OP_MODIFYVTX:
        return TRUE;

    case NDS_FIGHTER_DL_OP_VTX:
    {
        u32 v0;
        u32 count;
        size_t bytes;
        const u8 *src;
        u32 i;

        if (ndsGBIDecodeF3DEX2Vtx(command->w0, NDS_FIGHTER_DL_DRAW_MAX_VTX,
                                  &v0, &count) == FALSE)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        bytes = (size_t)count * 16u;
        src = ndsFighterDLDrawResolveDataPointer((uintptr_t)command->w1,
                                                 bytes,
                                                 state);
        if (src == NULL)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        for (i = 0; i < count; i++)
        {
            ndsFighterDLDrawDecodeVtx(state, v0 + i, src + (i * 16u));
        }
        return TRUE;
    }

    case NDS_FIGHTER_DL_OP_TRI1:
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri1(command->w0));
        return TRUE;

    case NDS_FIGHTER_DL_OP_TRI2:
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri2First(
                                           command->w0));
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri2Second(
                                           command->w1));
        return TRUE;

    case NDS_FIGHTER_DL_OP_CULLDL:
    case NDS_FIGHTER_DL_OP_TEXTURE:
    case NDS_FIGHTER_DL_OP_POPMTX:
    case NDS_FIGHTER_DL_OP_MTX:
    case NDS_FIGHTER_DL_OP_GEOMETRYMODE:
    case NDS_FIGHTER_DL_OP_MOVEWORD:
    case NDS_FIGHTER_DL_OP_SPECIAL_1:
    case NDS_FIGHTER_DL_OP_DL:
    case NDS_FIGHTER_DL_OP_ENDDL:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_H:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_L:
    case NDS_FIGHTER_DL_OP_SETSCISSOR:
    case NDS_FIGHTER_DL_OP_SETCOMBINE:
    case NDS_FIGHTER_DL_OP_SETCIMG:
    case NDS_FIGHTER_DL_OP_SETFOGCOLOR:
    case NDS_FIGHTER_DL_OP_SETBLENDCOLOR:
    case NDS_FIGHTER_DL_OP_SETENVCOLOR:
    case NDS_FIGHTER_DL_OP_SETPRIMCOLOR:
    case NDS_FIGHTER_DL_OP_SETTIMG:
    case NDS_FIGHTER_DL_OP_SETTILE:
    case NDS_FIGHTER_DL_OP_LOADBLOCK:
    case NDS_FIGHTER_DL_OP_LOADTLUT:
    case NDS_FIGHTER_DL_OP_SETTILESIZE:
    case NDS_FIGHTER_DL_OP_RDPSETOTHERMODE:
    case NDS_FIGHTER_DL_OP_RDPPIPESYNC:
    case NDS_FIGHTER_DL_OP_RDPLOADSYNC:
    case NDS_FIGHTER_DL_OP_RDPTILESYNC:
    case NDS_FIGHTER_DL_OP_RDPFULLSYNC:
        return TRUE;

    default:
        if (state->unsupported_opcode == 0u)
        {
            state->unsupported_opcode = op;
        }
        state->unsupported_command_count++;
        return FALSE;
    }
}

/* scVSBattleFuncLights computes this once from gMPCollisionLightAngleX/Y and
 * reaches ndsFighterDisplayContractSetLight through the imported gSPLight. */
static Light sNdsFighterDisplayCurrentLight;
static u32 sNdsFighterDisplayCurrentLightCount;
static u32 sNdsFighterDisplayCurrentLightValid;
/* NDSRendererStats.light_color_mask bits (nds_renderer.c's private
 * NDS_RENDERER_LIGHT_COLOR_*_MASK). Declared up here because the entry-effect
 * owner seeds them long before the fighter display contract is defined. */
#define NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK (1u << 0)
#define NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK (1u << 1)

