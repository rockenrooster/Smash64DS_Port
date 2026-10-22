/* Native renderer, the texture resolver: an RDP texture state to a cached GL
 * name (key, lookup, convert, upload, eviction). A slice of nds_renderer.c. */

/* The stage select's preview (battleship_mnmaps.c) points this at its
 * low-pass around its prepares, whose uploads are all its own; NULL
 * everywhere else. */
void (*gNdsRendererTexturePrefilter)(u16 *texels, u32 width, u32 height);

static s32 ndsRendererHardwareResolveOrBindTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    NDSRendererHardwareResolvedTexture *resolved,
    s32 allow_stage_source_frame)
{
#if NDS_TICK_HUD
    /* OUTSIDE the PROFILE_LEVEL guard on purpose. Its two uses are guarded on
     * NDS_TICK_HUD alone, and the tick-HUD build is PROFILE_LEVEL 0 -- nesting
     * this declaration inside the level-1 block compiled it out from under
     * them. */
    u32 tickhud_upload_mark = 0u;
#endif
    NDSRendererHardwareTextureKey key;
    NDSRendererHardwareTextureCacheEntry *entry;
    NDSRendererHardwareTextureCacheEntry *fraction_entry;
    NDSRendererHardwareTexel1Source texel1_source;
    const NDSRendererTextureLoadState *primary_load;
    const u8 *texels_src;
    const u16 *tlut_src;
    u32 width;
    u32 height;
    u32 format;
    u32 size;
    u32 upload_width;
    u32 upload_height;
    u32 upload_bytes;
    u32 resident_upload_bytes;
    u32 staged_bytes;
    u32 staged_row_bytes = 0u;
    u32 staged_row_count = 0u;
    u8 staged_row_map[NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
    u32 texels;
    u32 bytes;
    u32 loaded_bytes;
    u32 source_extent_width;
    u32 source_extent_height;
    u32 source_width;
    u32 source_read_width;
    u32 source_read_height;
    u32 source_origin_s;
    u32 source_origin_t;
    u32 source_last_index;
    u32 source_texels;
    u32 source_bytes;
    u32 source_physical_bytes;
    u32 palette_base;
    u32 tlut_physical_bytes;
    u32 tlut_count;
    u32 params;
    u32 key_hash;
    u32 render_tile_index;
    u32 render_tile_flags;
    u32 upload_attempts;
    sb32 holes_open;
    u32 primary_image;
    u32 primary_image_format;
    u32 primary_image_size;
    u32 primary_image_width;
    u32 primary_load_kind;
    u32 primary_load_tile;
    u32 primary_load_uls;
    u32 primary_load_ult;
    u32 primary_load_lrs;
    u32 primary_load_dxt;
    u32 primary_load_texels;
    u32 prim_env_blend_mode;
    s32 materialize_s;
    s32 materialize_t;
    s32 wants_texel1;
    s32 texel1_alpha_from_texel1 = FALSE;
    s32 use_texel1 = FALSE;
    s32 alpha_ignores_texels = FALSE;
    s32 graded_coverage = FALSE;
    s32 intensity_coverage = FALSE;
    s32 blendpe_coverage = FALSE;
    s32 ia_graded_alpha = FALSE;
    s32 use_texel1_ci4_lut = FALSE;
    s32 use_texel1_ci4_direct = FALSE;
    s32 queue_texture_refresh = FALSE;
    s32 compact_row_output = FALSE;
    s32 texel1_origin_delta_s = 0;
    s32 texel1_origin_delta_t = 0;
    const NDSRendererTileState *render_tile;
    u32 green_texels = 0u;
    u32 nonwhite_texels = 0u;
    u16 resident_palette[16];
    u32 resident_palette_entries = 0u;
    u32 resident_color0_transparent = FALSE;
    GL_TEXTURE_TYPE_ENUM resident_texture_type = GL_RGBA;
    int size_x;
    int size_y;
    u32 x;
    u32 y;
    u16 *upload_buffer = sNdsRendererHardwareTextureScratch;

    if (stats == NULL)
    {
        return FALSE;
    }
    /* Hierarchy preflight must remain a pure resident lookup.  The stage-site
     * shortcut performs the live bind/apply side effects, so only the normal
     * execution path may take it. */
    if ((resolved == NULL) &&
        (ndsRendererStageTextureSiteTryBind(stats, state, config) != FALSE))
    {
        return TRUE;
    }
    ndsRendererSyncTextureTile(stats);
    render_tile_index = ndsRendererActiveTextureTile(stats);
    render_tile = &stats->texture_tiles[render_tile_index];
    wants_texel1 = ndsRendererHardwareUsesTexel01Lerp(stats);
    if ((stats->texture_combine_w0 == 0xfc272c04u) &&
        (stats->texture_combine_w1 == 0x1f1093ffu))
    {
        gNdsRendererZebesAcidBindCount++;
        if (wants_texel1 != FALSE)
        {
            gNdsRendererZebesAcidWantsTexel1TrueCount++;
        }
        else
        {
            gNdsRendererZebesAcidWantsTexel1FalseCount++;
        }
    }
    texel1_alpha_from_texel1 =
        ((wants_texel1 != FALSE) &&
         (((stats->texture_combine_w1 >> 18) & 0x07u) ==
          NDS_RENDERER_ACMUX_SHADE)) ? TRUE : FALSE;
    prim_env_blend_mode =
        ndsRendererHardwarePrimEnvTexel0BlendMode(stats);
    primary_load = NULL;
    primary_image = stats->texture_image;
    primary_image_format = stats->texture_format;
    primary_image_size = stats->texture_size;
    primary_image_width = stats->texture_image_width;
    primary_load_kind = stats->texture_load_kind;
    primary_load_tile = stats->texture_load_tile;
    primary_load_uls = stats->texture_load_block_uls;
    primary_load_ult = stats->texture_load_block_ult;
    primary_load_lrs = stats->texture_load_block_lrs;
    primary_load_dxt = stats->texture_load_block_dxt;
    primary_load_texels = stats->texture_load_texels;
    /* The render tile samples whatever LOADBLOCK/LOADTILE last put at its
     * TMEM address, not the last SETTIMG: TMEM persists across commands, so a
     * palette swap (SETTIMG palette + LOADTLUT, no texel reload -- Planet
     * Zebes' tower and rock layers, 2026-09-07) keeps drawing the texels
     * loaded earlier. Reading the global image there converted the palette
     * bytes as texels. The per-TMEM load record is the truth whenever one
     * exists; the global words remain the fallback for a load the bounded
     * record could not hold. */
    primary_load = ndsRendererHardwareFindTextureLoadForTmem(
        stats, render_tile->tmem);
    if ((primary_load != NULL) &&
        ((primary_load->image == 0u) || (primary_load->load_texels == 0u)))
    {
        primary_load = NULL;
    }
    if ((wants_texel1 != FALSE) && (primary_load == NULL))
    {
        /* A two-texture combiner consumes the images resident at each render
         * tile's TMEM address, so it cannot fall back to the global image. */
        ndsRendererProfileRecordTexel1Reject();
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_LOAD_STATE);
        ndsRendererHardwareRejectTexture(
            stats, stats->texture_format, stats->texture_size,
            NDS_RENDERER_HW_TEXREJECT_MISSING_STATE);
        return FALSE;
    }
    if (primary_load != NULL)
    {
        primary_image = primary_load->image;
        primary_image_format = primary_load->image_format;
        primary_image_size = primary_load->image_size;
        primary_image_width = primary_load->image_width;
        primary_load_kind = primary_load->load_kind;
        primary_load_tile = primary_load->load_tile;
        primary_load_uls = primary_load->load_uls;
        primary_load_ult = primary_load->load_ult;
        primary_load_lrs = primary_load->load_lrs;
        primary_load_dxt = primary_load->load_dxt;
        primary_load_texels = primary_load->load_texels;
    }
    render_tile_flags = 0u;
    if (render_tile->set_seen != 0u)
    {
        render_tile_flags |= NDS_RENDERER_TILE_RENDER_SEEN |
            render_tile->flags;
    }
    if (stats->texture_tiles[NDS_RENDERER_LOAD_TILE].set_seen != 0u)
    {
        render_tile_flags |= NDS_RENDERER_TILE_LOAD_SEEN;
    }
    if ((((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_ON) == 0u) &&
         (ndsRendererHardwareTextureImplicitStateOn(stats) == FALSE)) ||
        (primary_image == 0u) ||
        (render_tile->line == 0u) ||
        (primary_load_texels == 0u))
    {
        ndsRendererHardwareRejectTexture(
            stats, stats->texture_format, stats->texture_size,
            NDS_RENDERER_HW_TEXREJECT_MISSING_STATE);
        return FALSE;
    }

    if (render_tile->set_seen != 0u)
    {
        format = render_tile->format;
        size = render_tile->size;
    }
    else
    {
        format = stats->texture_format;
        size = stats->texture_size;
    }

    if ((format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        ((size != NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
         (size != NDS_RENDERER_HW_TEXTURE_SIZ_8B)))
    {
        if (stats->texture_tlut_count <= 16u)
        {
            size = NDS_RENDERER_HW_TEXTURE_SIZ_4B;
        }
        else if (stats->texture_tlut_count <= 256u)
        {
            size = NDS_RENDERER_HW_TEXTURE_SIZ_8B;
        }
        else
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_CI_SIZE);
            return FALSE;
        }
    }
    if ((format != NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_RGBA16) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_IA) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_I16))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_UNSUPPORTED_FORMAT);
        return FALSE;
    }

    loaded_bytes = (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
        primary_load_texels * sizeof(u32) :
        primary_load_texels * sizeof(u16);
    width = render_tile->width;
    height = render_tile->height;
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        (ndsRendererHardwareTextureSourceBytes(format, size, width * height) >
         loaded_bytes))
    {
        width = ndsRendererHardwareTextureLinePixels(
            size, render_tile->line);
        texels = primary_load_texels * sizeof(u16);
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            texels *= 2u;
        }
        else if ((size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ||
                 (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B))
        {
            texels /= 2u;
        }
        height = (width != 0u) ? texels / width : 0u;
    }
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_DIMENSIONS);
        return FALSE;
    }
    source_extent_width = width;
    source_extent_height = height;
    materialize_s = ndsRendererHardwareTextureMaterializesMaskedClamp(
        render_tile->cms, render_tile->masks, source_extent_width,
        render_tile->width);
    materialize_t = ndsRendererHardwareTextureMaterializesMaskedClamp(
        render_tile->cmt, render_tile->maskt, source_extent_height,
        render_tile->height);
    if (materialize_s != FALSE)
    {
        width = render_tile->width;
    }
    if (materialize_t != FALSE)
    {
        height = render_tile->height;
    }
    /* A CLAMPED WINDOW TOO WIDE TO MATERIALISE STILL REPEATS INSIDE ITSELF.
     *
     * The RDP clamps a coordinate to the tile window first and masks it second,
     * so inside the window a clamped tile repeats every 1 << mask texels.
     * Materialising writes that repetition into a window-sized upload, which
     * only works up to the largest upload; past it the tile kept its load
     * stride with a clamping sampler, and every coordinate beyond the first
     * stride drew one edge texel. Mushroom Kingdom's upper-left ledge is a
     * 144x16 window over an 8-texel period (SETTILE masks 3, G_TX_CLAMP): it
     * drew as a flat tan slab (BUGS.md, stage row).
     *
     * Upload exactly one period instead. ndsRendererHardwareTextureParams
     * already turns "upload == period, window > period" into a repeating
     * sampler, so the only thing given up is the clamp OUTSIDE the window,
     * which geometry mapped inside its own window never samples. Eight is the
     * smallest DS texture edge, so a shorter period stays on the old path. */
    if ((materialize_s == FALSE) &&
        ((render_tile->cms & NDS_RENDERER_TX_CLAMP) != 0u) &&
        (render_tile->masks >= 3u) && (render_tile->masks < 31u) &&
        (render_tile->width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) &&
        ((1u << render_tile->masks) <= source_extent_width))
    {
        width = 1u << render_tile->masks;
        NDS_DIAG(gNdsRendererClampedWindowPeriodUploadCount++);
    }
    if ((materialize_t == FALSE) &&
        ((render_tile->cmt & NDS_RENDERER_TX_CLAMP) != 0u) &&
        (render_tile->maskt >= 3u) && (render_tile->maskt < 31u) &&
        (render_tile->height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) &&
        ((1u << render_tile->maskt) <= source_extent_height))
    {
        height = 1u << render_tile->maskt;
        NDS_DIAG(gNdsRendererClampedWindowPeriodUploadCount++);
    }
    {
        u32 wrap_width = ndsRendererHardwareTextureWrapPeriodExtent(
            render_tile->cms, render_tile->masks, width);
        u32 wrap_height = ndsRendererHardwareTextureWrapPeriodExtent(
            render_tile->cmt, render_tile->maskt, height);

        if ((wrap_width != width) || (wrap_height != height))
        {
            NDS_DIAG(gNdsRendererWrapPeriodUploadCount++);
        }
        width = wrap_width;
        height = wrap_height;
    }

    upload_width = ndsRendererHardwareTextureNextPow2(width);
    upload_height = ndsRendererHardwareTextureNextPow2(height);
    if ((upload_width < width) || (upload_height < height) ||
        (ndsRendererHardwareTextureSizeEnum(upload_width, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(upload_height, &size_y) == FALSE))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_UPLOAD_SIZE);
        return FALSE;
    }

    if (primary_load_kind == NDS_RENDERER_TEXTURE_LOADTILE)
    {
        source_origin_s = primary_load_uls >> 2;
        source_origin_t = primary_load_ult >> 2;
        source_width = ndsRendererHardwareTextureSourceWidthPixels(
            size, primary_image_size, primary_image_width);
    }
    else
    {
        u32 dxt = primary_load_dxt;

        source_origin_s = 0u;
        source_origin_t = 0u;
        source_width = source_extent_width;
        if (dxt != 0u)
        {
            u32 qwords = (NDS_RENDERER_G_TX_DXT_ONE + dxt - 1u) / dxt;

            /* BattleShip gbi.h:3291,3309-3317 encodes DXT as the rounded
             * 1.11 reciprocal of 64-bit source words per row. LOADBLOCK's
             * SETTIMG width is one, so DXT owns the DRAM row stride. */
            source_width = ndsRendererHardwareTextureLinePixels(size, qwords);
        }
    }
    source_read_width = (materialize_s != FALSE) ?
        (1u << render_tile->masks) : width;
    source_read_height = (materialize_t != FALSE) ?
        (1u << render_tile->maskt) : height;
    if ((source_width == 0u) ||
        (source_origin_s >= source_width) ||
        (source_read_width > (source_width - source_origin_s)))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_RANGE);
        return FALSE;
    }
    /* Steady state for the rebirth-halo beam and the Yoster cloud. The source
     * stride and masked sampling contract are known only after LOADBLOCK/LOADTILE
     * has been decoded, so residency is checked here rather than from the tile
     * extent alone. Yoster is a 32x32 I4 source presented through a 64x64
     * clamp+mirror tile; treating those two extents as one was the scrambled
     * cloud bug.
     *
     * LIVE BINDS ONLY. The hierarchy preflight hands back an entry pointer its
     * caller revalidates and re-resolves when NULL, and this surface has no
     * entry to give. */
    if ((resolved == NULL) &&
        ((prim_env_blend_mode ==
              NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA) ||
         (prim_env_blend_mode == NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA)) &&
        (format == NDS_RENDERER_HW_TEXTURE_FMT_I16) &&
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (ndsRendererHardwarePrimRgbTexel0AlphaResident(
             stats, primary_image, source_width, source_origin_s,
             source_origin_t, render_tile, materialize_s, materialize_t,
             upload_width, upload_height) != FALSE))
    {
        ndsRendererHardwareBindPrimRgbTexel0AlphaTexture(
            stats,
            ndsRendererHardwareTextureParams(stats, render_tile, upload_width,
                                             upload_height),
            format, width, height);
        return TRUE;
    }
    source_last_index =
        ((source_origin_t + source_read_height - 1u) * source_width) +
        source_origin_s + source_read_width - 1u;
    source_texels = source_last_index + 1u;
    source_bytes = ndsRendererHardwareTextureSourceBytes(format, size,
                                                        source_texels);
    if (source_bytes == 0u)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }
    source_physical_bytes = ndsRendererTexturePhysicalByteSpan(
        ndsRendererTextureDataLayout(config), source_bytes);
    if (source_physical_bytes < source_bytes)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }

    if (wants_texel1 != FALSE)
    {
        if (ndsRendererHardwarePrepareTexel1Source(
                stats, config, format, size, width, height,
                &texel1_source) != FALSE)
        {
            use_texel1 = TRUE;
            texel1_origin_delta_s = ndsRendererHardwareQuarterToTexel(
                ndsRendererHardwareTileOriginDelta(
                    render_tile->uls, texel1_source.render_tile->uls));
            texel1_origin_delta_t = ndsRendererHardwareQuarterToTexel(
                ndsRendererHardwareTileOriginDelta(
                    render_tile->ult, texel1_source.render_tile->ult));
            ndsRendererProfileRecordTexel1Composite();
        }
        else
        {
            ndsRendererProfileRecordTexel1Reject();
        }
    }
    /* N64 RGBA/CI/IA can carry useful colour behind alpha zero.  Preserve it
     * when the effective C/D alpha result does not read either texture and the
     * N64 alpha compare is disabled.  MODULATEIA puts TEXEL0 in alpha slot A,
     * so a full combiner-dependency test would incorrectly erase that hidden
     * colour again on opaque surfaces.  Conversely, forcing DS texel alpha
     * opaque while alpha compare is active fills source cutouts.  Keep the
     * optimized TEXEL0/TEXEL1 composite path unchanged; its output alpha is
     * part of the composite contract and is already keyed by the combine
     * words.
     *
     * Under a render mode whose blender reads alpha, the texels' alpha
     * stays whenever the output alpha reads it through any slot: the
     * two-cycle family-0 combine takes TEXEL0a in cycle 0 and COMBINEDa *
     * ENVa in cycle 1's A and C slots, and the C/D test alone made Wario's
     * mustache and Meta Knight's wings (CI4 decals, render mode 0xC4113878:
     * ZMODE_XLU, CVG_X_ALPHA, ALPHA_CVG_SEL) opaque rectangles (owner,
     * 2026-10-11). */
    alpha_ignores_texels =
        ((use_texel1 == FALSE) &&
         (stats != NULL) &&
         ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) == 0u) &&
         (ndsRendererHardwareOutputUsesAlpha(
              stats, NDS_RENDERER_ACMUX_TEXEL0) == FALSE) &&
         (ndsRendererHardwareOutputUsesAlpha(
              stats, NDS_RENDERER_ACMUX_TEXEL1) == FALSE) &&
         ((ndsRendererHardwareBlenderReadsAlpha(stats) == FALSE) ||
          ((ndsRendererHardwareOutputReadsAlphaFull(
                stats, NDS_RENDERER_ACMUX_TEXEL0) == FALSE) &&
           (ndsRendererHardwareOutputReadsAlphaFull(
                stats, NDS_RENDERER_ACMUX_TEXEL1) == FALSE)))) ? TRUE : FALSE;
    /* An owner's intensity-coverage request (sNdsRendererHardwareIntensity
     * Coverage): only an I4/I8 tile none of the bakes above already owns. */
    intensity_coverage =
        ((sNdsRendererHardwareIntensityCoverage != 0u) &&
         (use_texel1 == FALSE) &&
         (alpha_ignores_texels == FALSE) &&
         (prim_env_blend_mode == NDS_RENDERER_PRIM_ENV_BLEND_NONE) &&
         (format == NDS_RENDERER_HW_TEXTURE_FMT_I16) &&
         ((size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ||
          (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B))) ? TRUE : FALSE;
    /* The same request over the BLENDPE lerp whose alpha is TEXEL0 * PRIM on
     * an I8 tile (Board the Platforms' lights; the I4 form already has its
     * dedicated A5I3 bake above): the lerp's palette, the intensity as
     * graded coverage, PRIM alpha left on the polygon. Unrequested, the I
     * tile converts with one-bit alpha and the lights drew as solid discs. */
    blendpe_coverage =
        ((sNdsRendererHardwareIntensityCoverage != 0u) &&
         (use_texel1 == FALSE) &&
         (alpha_ignores_texels == FALSE) &&
         (prim_env_blend_mode == NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA) &&
         (format == NDS_RENDERER_HW_TEXTURE_FMT_I16) &&
         (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)) ? TRUE : FALSE;
    /* A fighter owner's graded-alpha request (sNdsRendererHardwareTexel
     * AlphaGraded): an IA8 tile whose alpha the blender takes as is. */
    ia_graded_alpha =
        ((sNdsRendererHardwareTexelAlphaGraded != 0u) &&
         (use_texel1 == FALSE) &&
         (alpha_ignores_texels == FALSE) &&
         (prim_env_blend_mode == NDS_RENDERER_PRIM_ENV_BLEND_NONE) &&
         ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) == 0u) &&
         (ndsRendererHardwareBlenderReadsAlpha(stats) != FALSE) &&
         (format == NDS_RENDERER_HW_TEXTURE_FMT_IA) &&
         (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)) ? TRUE : FALSE;

    memset(&key, 0, sizeof(key));
    key.image = primary_image;
    key.image_format = primary_image_format;
    key.image_size = primary_image_size;
    key.image_width = primary_image_width;
    key.tlut_image = stats->texture_tlut_image;
    key.tlut_count = stats->texture_tlut_count;
    key.data_layout = (config != NULL) ?
        (u32)config->texture_data_layout :
        (u32)NDS_RENDERER_TEXTURE_DATA_NATIVE;
    key.format = format;
    key.size = size;
    key.width = width;
    key.height = height;
    key.render_tile = render_tile_index;
    key.render_tmem = render_tile->tmem;
    key.render_palette = render_tile->palette;
    key.render_tile_cms = render_tile->cms;
    key.render_tile_cmt = render_tile->cmt;
    key.render_tile_masks = render_tile->masks;
    key.render_tile_maskt = render_tile->maskt;
    key.render_tile_shifts = render_tile->shifts;
    key.render_tile_shiftt = render_tile->shiftt;
    key.load_tile = primary_load_tile;
    key.load_uls = primary_load_uls;
    key.load_ult = primary_load_ult;
    key.load_lrs = primary_load_lrs;
    key.load_dxt = primary_load_dxt;
    key.load_texels = primary_load_texels;
    key.tile_uls = render_tile->uls;
    key.tile_ult = render_tile->ult;
    key.tile_lrs = render_tile->lrs;
    key.tile_lrt = render_tile->lrt;
    /* A tile that wraps on both axes samples the whole loaded image through
     * its masks; its origin only offsets the texture coordinates, which the
     * vertex path already subtracts (texture_prepare_origin_s/t). Keying on
     * that origin made every G_SETTILESIZE scroll step a fresh upload: Planet
     * Zebes' acid re-uploaded its 32x32 CI4 surface each frame and took 40 of
     * the 79 dynamic slots in 40 frames (2026-09-07). Keep the extent, drop
     * the origin. A clamped tile keeps its rectangle -- the DS clamps at the
     * upload edge, so its texels depend on it. "Wraps" is the sampler's own
     * decision (ndsRendererHardwareTextureParams): a clamped tile whose window
     * exceeds the mask period wraps too -- the acid is a 384x384 window over
     * a 32x32 image. Every Dream Land pinned key with a nonzero origin is a
     * clamped tile inside its period, so the static corpus still matches word
     * for word. */
    if ((render_tile->masks != 0u) && (render_tile->maskt != 0u) &&
        (((render_tile->cms & NDS_RENDERER_TX_CLAMP) == 0u) ||
         (ndsRendererHardwareTextureMaskedClampNeedsWrap(
              render_tile->cms, render_tile->masks, upload_width,
              render_tile->width) != FALSE)) &&
        (((render_tile->cmt & NDS_RENDERER_TX_CLAMP) == 0u) ||
         (ndsRendererHardwareTextureMaskedClampNeedsWrap(
              render_tile->cmt, render_tile->maskt, upload_height,
              render_tile->height) != FALSE)) &&
        (render_tile->lrs >= render_tile->uls) &&
        (render_tile->lrt >= render_tile->ult))
    {
        key.tile_uls = 0u;
        key.tile_ult = 0u;
        key.tile_lrs = render_tile->lrs - render_tile->uls;
        key.tile_lrt = render_tile->lrt - render_tile->ult;
    }
    key.line = render_tile->line;
    key.flags = render_tile_flags | (primary_load_kind << 8);
    if (alpha_ignores_texels != FALSE)
    {
        key.flags |= NDS_RENDERER_HW_TEXTURE_KEY_ALPHA_IGNORES_TEXELS;
    }
    if ((intensity_coverage != FALSE) || (blendpe_coverage != FALSE))
    {
        key.flags |= NDS_RENDERER_HW_TEXTURE_KEY_I_TEXEL_ALPHA;
    }
    if (ia_graded_alpha != FALSE)
    {
        key.flags |= NDS_RENDERER_HW_TEXTURE_KEY_IA_GRADED_ALPHA;
    }
    if (use_texel1 != FALSE)
    {
        const NDSRendererTextureLoadState *load = texel1_source.load;
        const NDSRendererTileState *tile = texel1_source.render_tile;

        key.texel1_image = load->image;
        key.texel1_image_format = load->image_format;
        key.texel1_image_size = load->image_size;
        key.texel1_image_width = load->image_width;
        key.texel1_load_kind = load->load_kind;
        key.texel1_render_tmem = tile->tmem;
        key.texel1_render_line = tile->line;
        key.texel1_render_palette = tile->palette;
        key.texel1_render_tile_cms = tile->cms;
        key.texel1_render_tile_cmt = tile->cmt;
        key.texel1_render_tile_masks = tile->masks;
        key.texel1_render_tile_maskt = tile->maskt;
        key.texel1_render_tile_shifts = tile->shifts;
        key.texel1_render_tile_shiftt = tile->shiftt;
        key.texel1_load_tile = load->load_tile;
        key.texel1_load_uls = load->load_uls;
        key.texel1_load_ult = load->load_ult;
        key.texel1_load_lrs = load->load_lrs;
        key.texel1_load_dxt = load->load_dxt;
        key.texel1_load_texels = load->load_texels;
        key.texel1_tile_uls = tile->uls;
        key.texel1_tile_ult = tile->ult;
        key.texel1_tile_lrs = tile->lrs;
        key.texel1_tile_lrt = tile->lrt;
        key.prim_lod_fraction = stats->prim_lod_fraction;
        key.combine_w0 = stats->texture_combine_w0;
        key.combine_w1 = stats->texture_combine_w1;
    }
    else if (prim_env_blend_mode ==
             NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA)
    {
        /* ITS OWN BIT, BECAUSE THE EXISTING ONE DOES NOT SEPARATE THE BAKES.
         * Modes 1 and 2 both bake through ndsRendererHardwareBlendPrimEnvTexel0,
         * so one flag was enough to tell "baked" from "raw". This mode bakes
         * differently, so sharing the flag would let a beam and a BLENDPE
         * surface with equal prim/env collide on one cache entry and serve
         * whichever was converted first.
         *
         * PRIM RGB is already keyed by the assignment below and must stay
         * keyed: it is now the texel colour itself, not a blend endpoint. ENV
         * is dropped rather than copied -- this combine never reads it, and
         * keying on it would force a re-bake every time an unrelated list
         * moved the env colour. */
        key.flags |= NDS_RENDERER_HW_TEXTURE_KEY_PRIM_RGB_TEXEL0_ALPHA;
        key.combine_w0 = stats->prim_color & 0xffffff00u;
        key.combine_w1 = 0u;
    }
    else if (prim_env_blend_mode != NDS_RENDERER_PRIM_ENV_BLEND_NONE)
    {
        /* TEXEL1 and this bake are mutually exclusive. Reuse the key's
         * variant tail without growing its 236-byte DS cache footprint. Alpha
         * is excluded because it remains live polygon state, not texel RGB. */
        key.flags |= NDS_RENDERER_HW_TEXTURE_KEY_PRIM_ENV_BLEND;
        key.combine_w0 = stats->prim_color & 0xffffff00u;
        key.combine_w1 = stats->env_color & 0xffffff00u;
    }

    key_hash = ndsRendererHardwareTextureKeyHash(&key);
#if NDS_VRAM_CENSUS_LIVE
    if (sNdsFtrAdmitKeyProbe != NULL)
    {
        *sNdsFtrAdmitKeyProbe = key;
        return FALSE;
    }
#endif

    fraction_entry = NULL;
    entry = ndsRendererHardwareFindTexture(&key, key_hash);
    params = ndsRendererHardwareTextureParams(stats, render_tile,
                                               upload_width, upload_height);
    if ((entry == NULL) && (allow_stage_source_frame != FALSE) &&
        (sNdsRendererBattleStaticTextureArmed != 0u))
    {
        /* Dynamic Pupupu materials keep their source animation and geometry,
         * but reuse the first resident source image when a later image was not
         * prepared before GO. Every other renderer-key word must still match. */
        entry = ndsRendererHardwareFindStageSourceFrameTexture(&key);
    }
    if (entry != NULL)
    {
        if (resolved != NULL)
        {
            resolved->entry = entry;
            resolved->name = (u32)entry->name;
            resolved->params = (entry->params &
                ~NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK) |
                (params & NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK);
            resolved->format = format;
            resolved->width = width;
            resolved->height = height;
            return TRUE;
        }
        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
        if (entry->pinned != 0u)
        {
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
            /* The offline key deliberately excludes DS sampler bits. Resolve
             * them from the first live BattleShip state, then let the existing
             * site cache reuse the exact entry. */
            entry->params = ndsRendererHardwareMergeTextureParams(params);
            ndsRendererHardwareApplyTextureParams(entry->params);
            sNdsRendererHardwareActiveTextureEntry = entry;
            ndsRendererHardwareRecordBattleStaticTextureHit(entry);
        }
        else if (sNdsRendererHardwareActiveTextureEntry != entry)
        {
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
            ndsRendererHardwareApplyTextureParams(entry->params);
            sNdsRendererHardwareActiveTextureEntry = entry;
        }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = format;
        stats->hardware_texture_width = width;
        stats->hardware_texture_height = height;
        ndsRendererStageTextureSiteRemember(
            state, stats, entry, format, size, width, height);
        return TRUE;
    }
    if (resolved != NULL)
    {
        return FALSE;
    }
    if ((sNdsRendererBattleStaticTextureArmed != 0u) &&
        (gNdsRendererBattleTextureFallbackFirstFrame == 0u))
    {
        gNdsRendererBattleTextureFallbackFirstFrame =
            gNdsRendererProfileFrameCount;
        gNdsRendererBattleTextureFallbackFirstKeyHash = key_hash;
        (void)ndsRelocGetLoadedPointerProvenance(
            (const void *)(uintptr_t)key.image,
            (u32 *)&gNdsRendererBattleTextureFallbackFirstImageAsset,
            (u32 *)&gNdsRendererBattleTextureFallbackFirstImageOffset);
        (void)ndsRelocGetLoadedPointerProvenance(
            (const void *)(uintptr_t)key.tlut_image,
            (u32 *)&gNdsRendererBattleTextureFallbackFirstTlutAsset,
            (u32 *)&gNdsRendererBattleTextureFallbackFirstTlutOffset);
        gNdsRendererBattleTextureFallbackFirstFormat = format;
        gNdsRendererBattleTextureFallbackFirstSize = size;
        gNdsRendererBattleTextureFallbackFirstWidth = width;
        gNdsRendererBattleTextureFallbackFirstHeight = height;
        gNdsRendererBattleTextureFallbackFirstFlags = key.flags;
        gNdsRendererBattleTextureFallbackFirstCombineW0 = key.combine_w0;
        gNdsRendererBattleTextureFallbackFirstCombineW1 = key.combine_w1;
    }
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_MANIFEST_FALLBACK);
#if defined(NDS_LAB_TEXMISS) && NDS_LAB_TEXMISS
    /* LAB ONLY (Makefile NDS_LAB_TEXMISS): every converted miss, for the
     * over-gate texture census. The count keeps running past the end. */
    {
        u32 n = gNdsLabTexMissCount;

        if (n < NDS_LAB_TEXMISS_MAX)
        {
            u32 *rec = gNdsLabTexMiss[n];

            rec[0] = gNdsRendererProfileFrameCount;
            rec[1] = (u32)key.image;
            rec[2] = (u32)key.tlut_image;
            rec[3] = (format << 24) | (size << 16) | (u32)key.flags;
            rec[4] = (width << 16) | height;
            rec[5] = key.combine_w0;
            rec[6] = key.combine_w1;
            rec[7] = key_hash;
            rec[8] = (use_texel1 != FALSE) ? (0x80000000u |
                                              key.prim_lod_fraction) : 0u;
            /* The image's provenance: its asset id (high half) and offset
             * (low half, in 16-byte units), 0xffffffff when unknown. */
            {
                u32 asset = 0xffffffffu;
                u32 offset = 0xffffffffu;

                rec[9] = 0xffffffffu;
                if ((ndsRelocGetLoadedPointerProvenance(
                         (const void *)(uintptr_t)key.image, &asset,
                         &offset) != FALSE) &&
                    (asset <= 0xffffu))
                {
                    rec[9] = (asset << 16) | ((offset >> 4) & 0xffffu);
                }
            }
            /* The debugger reads memory, not the data cache. */
            DC_FlushRange(rec, sizeof(gNdsLabTexMiss[0]));
        }
        gNdsLabTexMissCount = n + 1u;
        DC_FlushRange((const void *)&gNdsLabTexMissCount,
                      sizeof(gNdsLabTexMissCount));
    }
#endif
    if (use_texel1 != FALSE)
    {
        fraction_entry =
            ndsRendererHardwareFindTexel1RefreshTexture(&key);
    }


    texels = width * height;
    bytes = ndsRendererHardwareTextureSourceBytes(
        format, size, source_read_width * source_read_height);
    if ((bytes == 0u) || (bytes > loaded_bytes))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }
    texels_src = ndsRendererResolveTextureDataPointer(
        config, (const void *)(uintptr_t)primary_image,
        source_physical_bytes);
    if (texels_src == NULL)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_PTR);
        return FALSE;
    }

    /* First sight of the beam or the Yoster cloud this scene: build the
     * dedicated A5I3 texture from the source in hand. Deliberately here
     * rather than at scene load -- the
     * discriminator is semantic (this combine over an I4 tile) and the extent
     * comes from the tile, so nothing hardcodes an asset address or depends on
     * load order. On failure the generic path continues and produces the
     * previous one-bit result rather than dropping the draw. */
    if ((resolved == NULL) && (use_texel1 == FALSE) &&
        ((prim_env_blend_mode ==
              NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA) ||
         (prim_env_blend_mode == NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA)) &&
        (format == NDS_RENDERER_HW_TEXTURE_FMT_I16) &&
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (ndsRendererHardwarePreparePrimRgbTexel0AlphaTexture(
             stats, config, texels_src, primary_image, source_width,
             source_origin_s, source_origin_t, render_tile, materialize_s,
             materialize_t, width, height, upload_width,
             upload_height) != FALSE))
    {
        ndsRendererHardwareBindPrimRgbTexel0AlphaTexture(
            stats, params, format, width, height);
        return TRUE;
    }
    tlut_src = NULL;
    palette_base = 0u;
    tlut_count = stats->texture_tlut_count;
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        u32 palette_entries;

        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            palette_base = render_tile->palette * 16u;
        }
        /* LOADTLUT is allowed to load fewer than the format's full 16/256
         * entries. Mario's source Fireball list deliberately loads 13 CI4
         * entries because its 16x16 image uses only indices 0..12. Validate
         * the exact indices reachable by this tile instead of rejecting that
         * legal BattleShip state or reading beyond the loaded palette. Keep
         * the common full-TLUT path scan-free; only partial source TLUTs pay
         * the bounded index census. */
        palette_entries = palette_base +
            ((size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ? 16u : 256u);
        if ((use_texel1 != FALSE) &&
            (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI))
        {
            u32 texel1_palette_entries = texel1_source.palette_base +
                ((texel1_source.size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
                    16u : 256u);

            if (texel1_palette_entries > palette_entries)
            {
                palette_entries = texel1_palette_entries;
            }
        }
        if (stats->texture_tlut_count < palette_entries)
        {
            ndsRendererHardwareRecordBattleTextureFence(
                NDS_RENDERER_BATTLE_TEXTURE_FENCE_PALETTE_DECODE);
            palette_entries = ndsRendererHardwareCiPaletteEntriesUsed(
                config, texels_src, size, source_width,
                source_origin_s, source_origin_t,
                source_read_width, source_read_height, palette_base);
            if ((use_texel1 != FALSE) &&
                (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI))
            {
                u32 texel1_read_width =
                    (texel1_source.materialize_s != FALSE) ?
                        (1u << texel1_source.render_tile->masks) :
                        texel1_source.width;
                u32 texel1_read_height =
                    (texel1_source.materialize_t != FALSE) ?
                        (1u << texel1_source.render_tile->maskt) :
                        texel1_source.height;
                u32 texel1_palette_entries;

                ndsRendererHardwareRecordBattleTextureFence(
                    NDS_RENDERER_BATTLE_TEXTURE_FENCE_PALETTE_DECODE);
                texel1_palette_entries =
                    ndsRendererHardwareCiPaletteEntriesUsed(
                        config, texel1_source.texels, texel1_source.size,
                        texel1_source.source_width,
                        texel1_source.source_origin_s,
                        texel1_source.source_origin_t,
                        texel1_read_width, texel1_read_height,
                        texel1_source.palette_base);

                if (texel1_palette_entries > palette_entries)
                {
                    palette_entries = texel1_palette_entries;
                }
            }
        }
        /* P2-6 (2026-10-01): A STAGE PALETTE SHORTER THAN ITS INDICES READS
         * THE WORDS THAT FOLLOW IT. Fifteen of the 24 bonus boards' first
         * texture is a CI4 brick that uses index 3 under a three-colour
         * LOADTLUT (the other nine load four colours of the same image and
         * draw); the RDP reads whatever TMEM held there, and refusing the
         * texture refused the whole map -- only targets and platforms drew
         * (owner, Kirby's boards). A stage's palette sits in its own file,
         * so the entries past the loaded count are the source's next palette
         * words, which is what the four-colour boards load. Fighters and
         * effects keep the refusal. */
        tlut_count = stats->texture_tlut_count;
        if ((allow_stage_source_frame != FALSE) &&
            (stats->texture_tlut_image != 0u) && (tlut_count != 0u) &&
            (tlut_count < palette_entries))
        {
            tlut_count = palette_entries;
            NDS_DIAG(gNdsStageShortTlutExtendCount++);
        }
        if ((stats->texture_tlut_image == 0u) ||
            (tlut_count < palette_entries))
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_TLUT);
            return FALSE;
        }
        tlut_physical_bytes = ndsRendererTexturePhysicalByteSpan(
            ndsRendererTextureDataLayout(config),
            palette_entries * sizeof(u16));
        tlut_src = ndsRendererResolveTextureDataPointer(
            config, (const void *)(uintptr_t)stats->texture_tlut_image,
            tlut_physical_bytes);
        if (tlut_src == NULL)
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_TLUT_PTR);
            return FALSE;
        }
    }

    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_CONVERT);
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_PALETTE_DECODE);
    upload_bytes = upload_width * upload_height * sizeof(u16);
    NDS_RENDERER_CAPACITY_HIGH_WATER(
        gNdsRendererTextureScratchDynamicUploadBytesHighWater, upload_bytes);
    resident_upload_bytes = upload_bytes;
    staged_bytes = upload_bytes;
    if (fraction_entry != NULL)
    {
        if ((upload_bytes <=
             sizeof(sNdsRendererHardwareTextureRefreshSmall)) &&
            (ndsRendererHardwareTextureRefreshUses(
                 sNdsRendererHardwareTextureRefreshSmall) == FALSE))
        {
            upload_buffer =
                sNdsRendererHardwareTextureRefreshSmall;
            queue_texture_refresh = TRUE;
        }
        else if ((upload_bytes >
                  sizeof(sNdsRendererHardwareTextureRefreshSmall)) &&
                 (ndsRendererHardwareTextureRefreshUses(
                      sNdsRendererHardwareTextureRefreshLarge) == FALSE))
        {
            upload_buffer =
                sNdsRendererHardwareTextureRefreshLarge;
            queue_texture_refresh = TRUE;
        }
    }
    if ((use_texel1 != FALSE) &&
        (format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (texel1_source.size == NDS_RENDERER_HW_TEXTURE_SIZ_4B))
    {
        /* The source pond's two CI4 inputs can produce only 16x16 palette
         * pairs for a given primitive LOD fraction. Precompute exact RGB and
         * the 16-bit ordered-coverage mask once per pair so the animated pixel
         * loop avoids per-pixel color blending. */
        ndsRendererHardwareBuildTexel01Ci4Lut(
            config, tlut_src, tlut_count, palette_base,
            texel1_source.palette_base, stats->prim_lod_fraction);
        use_texel1_ci4_lut = TRUE;
        use_texel1_ci4_direct = TRUE;
    }

    /* A BLENDPE SURFACE OVER AN I TILE KEEPS ITS COVERAGE LEVELS.
     *
     * (PRIM - ENV) * TEXEL0 + ENV with alpha (0,0,0,TEXEL0) over an I tile is a
     * coverage image: colour is the endpoint lerp AT that coverage and alpha IS
     * that coverage. ndsRendererHardwareConvertI sets the alpha bit on every
     * texel, and the RGB5A1 bake below kept it, so such a surface drew as its
     * whole rectangle at the ENV colour: Yoshi's Island's two rotating cards
     * were opaque white cards around the cloud shape, and the heart's sparkles
     * were solid spikes on their ENV blue (BUGS.md, stage row).
     *
     * One byte a texel as GL_RGB8_A5 holds five alpha bits and three palette
     * bits, exactly as the dedicated rebirth-beam name does, but as an ordinary
     * cache entry: the entry already carries a resident type per upload (PAL16
     * below), so a prepared stage run binds it like any other name. The loop
     * leaves the raw 5-bit coverage in the scratch word so the clamp padding
     * replicates it unchanged, and the pack after it folds the words to bytes
     * in place. PRIM and ENV are in the key already (modes 1 and 2 bake them),
     * so two cards over one image keep one entry each. */
    graded_coverage =
        ((use_texel1 == FALSE) && (fraction_entry == NULL) &&
         (upload_buffer == sNdsRendererHardwareTextureScratch) &&
         (alpha_ignores_texels == FALSE) &&
         (((format == NDS_RENDERER_HW_TEXTURE_FMT_I16) &&
           ((size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ||
            (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)) &&
           ((prim_env_blend_mode ==
             NDS_RENDERER_PRIM_ENV_BLEND_SOURCE_ALPHA) ||
            (intensity_coverage != FALSE) || (blendpe_coverage != FALSE))) ||
          (ia_graded_alpha != FALSE))) ? TRUE : FALSE;

    /* Only the power-of-two rectangle handed to libnds is observable.  The
     * shared scratch arena is sized for the worst 128x128 texture, but smaller
     * animated uploads must not pay to clear the unused tail every frame. */
    if ((width != upload_width) || (height != upload_height))
    {
        memset(sNdsRendererHardwareTextureScratch, 0, upload_bytes);
    }
    if (use_texel1_ci4_direct != FALSE)
    {
        compact_row_output = ndsRendererHardwareConvertTexel01Ci4Direct(
            config, texels_src, source_texels, source_width, source_origin_s,
            source_origin_t, render_tile, materialize_s, materialize_t,
            &texel1_source, texel1_origin_delta_s, texel1_origin_delta_t,
            texel1_alpha_from_texel1, width, height, upload_width,
            ((upload_buffer == sNdsRendererHardwareTextureRefreshLarge) &&
             (queue_texture_refresh != FALSE) &&
             (width == upload_width) && (height == upload_height)) ?
                staged_row_map : NULL,
            &staged_bytes, &green_texels, &nonwhite_texels);
        ndsRendererProfileRecordTextureCi4Direct(width * height);
    }
    else
    {
        /* Per-value colours (P2-2p8, 2026-10-07). For a 4- or 8-bit CI, IA
         * or I texel without TEXEL1 or graded coverage, the loop's whole
         * chain -- the palette or intensity decode, the PRIM/ENV bake and
         * the opaque rule -- is a function of the stored value alone, so it
         * runs once a value and the loop reads the value and looks it up.
         * The same words reach the scratch image; a first-use 64x64 paid the
         * chain 4,096 times (~60 cycles each, the bulk of a 300-500K-cycle
         * Saffron first sight). An 8-bit table pays 256 values, so it waits
         * for images with more texels than that. */
        u32 value_lut_count = 0u;

        if ((use_texel1_ci4_lut == FALSE) && (use_texel1 == FALSE) &&
            (graded_coverage == FALSE) &&
            ((format == NDS_RENDERER_HW_TEXTURE_FMT_CI) ||
             (format == NDS_RENDERER_HW_TEXTURE_FMT_IA) ||
             (format == NDS_RENDERER_HW_TEXTURE_FMT_I16)) &&
            ((size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ||
             ((size == NDS_RENDERER_HW_TEXTURE_SIZ_8B) &&
              ((width * height) > 1024u))))
        {
            u32 v;

            value_lut_count =
                (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ? 16u : 256u;
            for (v = 0u; v < value_lut_count; v++)
            {
                u16 color = ndsRendererHardwareTextureValueColor(
                    config, format, size, tlut_src, tlut_count,
                    palette_base, v,
                    (alpha_ignores_texels != FALSE) ? TRUE : FALSE);

                if (prim_env_blend_mode ==
                    NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA)
                {
                    color = ndsRendererHardwarePrimRgbTexel0Alpha(
                        color, stats->prim_color);
                }
                else if (prim_env_blend_mode !=
                         NDS_RENDERER_PRIM_ENV_BLEND_NONE)
                {
                    color = ndsRendererHardwareBlendPrimEnvTexel0(
                        color, stats->prim_color, stats->env_color);
                }
                if (alpha_ignores_texels != FALSE)
                {
                    color |= 0x8000u;
                }
                sNdsRendererHardwareTextureValueLut[v] = color;
            }
        }
        for (y = 0u; y < height; y++)
        {
            u32 source_y = (materialize_t != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    y, render_tile->cmt, render_tile->maskt) : y;

            for (x = 0u; x < width; x++)
            {
                u32 source_x = (materialize_s != FALSE) ?
                    ndsRendererHardwareTextureMaskedAddress(
                        x, render_tile->cms, render_tile->masks) : x;
                u32 src_index =
                    ((source_origin_t + source_y) * source_width) +
                    source_origin_s + source_x;
                u32 dst_index = (y * upload_width) + x;
                u16 color;

                if (value_lut_count != 0u)
                {
                    color = sNdsRendererHardwareTextureValueLut[
                        (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
                            ndsRendererReadTexturePackedNibble(
                                config, texels_src, src_index, format, size) :
                            ndsRendererReadTextureByte(
                                config, texels_src, src_index, format,
                                size)];
                }
                else if (use_texel1_ci4_lut != FALSE)
                {
                    u32 index0 = ndsRendererReadTexturePackedNibble(
                        config, texels_src, src_index, format, size);
                    u32 source1_index = ndsRendererHardwareTexel1SourceIndex(
                        &texel1_source, texel1_origin_delta_s,
                        texel1_origin_delta_t, x, y);
                    u32 index1 = ndsRendererReadTexturePackedNibble(
                        config, texel1_source.texels, source1_index,
                        texel1_source.format, texel1_source.size);

                    color = ndsRendererHardwareResolveTexel01Ci4Lut(
                        index0, index1, x, y, texel1_alpha_from_texel1);
                }
                else
                {
                    color = ndsRendererHardwareTextureColor(
                        config, format, size, texels_src, tlut_src,
                        tlut_count, palette_base, src_index,
                        ((use_texel1 != FALSE) ||
                         (alpha_ignores_texels != FALSE)) ? TRUE : FALSE);
                    if (use_texel1 != FALSE)
                    {
                        u16 color1 = ndsRendererHardwareTexel1Color(
                            &texel1_source, config, tlut_src,
                            tlut_count,
                            texel1_origin_delta_s, texel1_origin_delta_t,
                            x, y);

                        color = ndsRendererHardwareBlendTexel01(
                            color, color1, stats->prim_lod_fraction, x, y);
                        if (texel1_alpha_from_texel1 != FALSE)
                        {
                            color = (u16)(
                                (color & NDS_RENDERER_HW_TEXEL01_RGB_MASK) |
                                (color1 & 0x8000u));
                        }
                    }
                    else if (ia_graded_alpha != FALSE)
                    {
                        /* The packed byte itself: the alpha nibble widened
                         * to five bits over the intensity's top three. */
                        u32 value = ndsRendererReadTextureByte(
                            config, texels_src, src_index, format, size);
                        u32 alpha = value & 0xfu;

                        color = (u16)((((alpha << 1) | (alpha >> 3)) << 3) |
                                      (value >> 5));
                    }
                    else if (graded_coverage != FALSE)
                    {
                        /* The I conversion wrote intensity >> 3 into every
                         * lane; one lane is the 5-bit coverage. */
                        color = (u16)(color & 0x1fu);
                    }
                    else if (prim_env_blend_mode ==
                             NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA)
                    {
                        color = ndsRendererHardwarePrimRgbTexel0Alpha(
                            color, stats->prim_color);
                    }
                    else if (prim_env_blend_mode !=
                             NDS_RENDERER_PRIM_ENV_BLEND_NONE)
                    {
                        color = ndsRendererHardwareBlendPrimEnvTexel0(
                            color, stats->prim_color, stats->env_color);
                    }
                }
                if (alpha_ignores_texels != FALSE)
                {
                    /* DS texture alpha participates in modulation even when
                     * the N64 combiner ignores TEXEL alpha.  Preserve the
                     * resolved source colour and make the DS texel opaque so
                     * polygon/vertex alpha remains the only live alpha. */
                    color |= 0x8000u;
                }
                sNdsRendererHardwareTextureScratch[dst_index] = color;
            }
        }
    }
    /* Source tile smaller than its upload: see the helper for why the padding
     * of a clamped axis must carry the edge texel. The CI4 direct path writes
     * its own rows and is left alone. */
    if ((use_texel1_ci4_direct == FALSE) &&
        ((width != upload_width) || (height != upload_height)))
    {
        ndsRendererHardwareReplicateClampPadding(
            sNdsRendererHardwareTextureScratch, width, height,
            upload_width, upload_height, render_tile->cms, render_tile->cmt);
    }
    /* The stage select's preview low-passes its uploads (see
     * gNdsRendererTexturePrefilter): direct-colour words only, so not the
     * coverage encodings, TEXEL1's images or the CI4 direct rows. */
    if ((gNdsRendererTexturePrefilter != NULL) &&
        (graded_coverage == FALSE) && (intensity_coverage == FALSE) &&
        (blendpe_coverage == FALSE) && (use_texel1 == FALSE) &&
        (use_texel1_ci4_direct == FALSE) && (fraction_entry == NULL) &&
        (upload_buffer == sNdsRendererHardwareTextureScratch))
    {
        gNdsRendererTexturePrefilter(sNdsRendererHardwareTextureScratch,
                                     upload_width, upload_height);
    }
    /* Preserve the canonical lane-observation totals while paying one volatile
     * update per converted texture instead of one per texel. */

    /* TEXEL1 is the one path that may refresh an existing direct-colour
     * allocation in place when only the blend fraction changes. Keep that
     * refresh representation unchanged. Every other miss is immutable for its
     * exact 236-byte key, so compact the already-resolved final image whenever
     * it fits losslessly in PAL16. This is especially important in 3+/4-player
     * battles: BattleShip correctly selects Low fighter detail there, and the
     * fourth owner's late material must not fail merely because earlier dynamic
     * images consumed texture VRAM as 16bpp direct colour. */
    if (graded_coverage != FALSE)
    {
        u8 *packed = (u8 *)sNdsRendererHardwareTextureScratch;
        u32 texel_count = upload_width * upload_height;
        u32 i;

        /* alpha5 in bits 3-7, palette index in bits 0-2. Byte i lands at or
         * below word i's own bytes, so the forward pass never overwrites a
         * word it has not read. */
        for (i = 0u; i < texel_count; i++)
        {
            u32 coverage = sNdsRendererHardwareTextureScratch[i] & 0x1fu;

            packed[i] = (ia_graded_alpha != FALSE) ?
                (u8)sNdsRendererHardwareTextureScratch[i] :
                (u8)((coverage << 3) | (coverage >> 2));
        }
        /* Each palette step is the endpoint lerp at the midpoint of the
         * coverage band it serves, through the same helper the RGB5A1 bake
         * uses, so the colours agree with every other BLENDPE surface. */
        for (i = 0u; i < 8u; i++)
        {
            u32 v = (i << 2) + 2u;
            u16 texel0 = (u16)(0x8000u | v | (v << 5) | (v << 10));

            /* An IA8 tile's step i is intensity nibble (i << 1) | (i >> 2),
             * so 0 and 7 keep black and white. */
            if (ia_graded_alpha != FALSE)
            {
                u32 g = ((((i << 1) | (i >> 2)) * 0x11u) >> 3);

                texel0 = (u16)(0x8000u | g | (g << 5) | (g << 10));
            }
            /* An intensity-coverage tile keeps TEXEL0's own grey: the
             * vertex/PRIM modulate is the hardware's, not the bake's. */
            resident_palette[i] = ((intensity_coverage != FALSE) ||
                                   (ia_graded_alpha != FALSE)) ?
                (u16)(texel0 & 0x7fffu) :
                (u16)(ndsRendererHardwareBlendPrimEnvTexel0(
                          texel0, stats->prim_color,
                          stats->env_color) & 0x7fffu);
        }
        resident_palette_entries = 8u;
        resident_texture_type = GL_RGB8_A5;
        resident_upload_bytes = texel_count;
        NDS_DIAG(gNdsRendererGradedCoverageUploadCount++);
    }
    else if ((use_texel1 == FALSE) &&
        (upload_buffer == sNdsRendererHardwareTextureScratch))
    {
        resident_palette_entries = ndsRendererHardwarePackResolvedPal16(
            sNdsRendererHardwareTextureScratch,
            upload_width * upload_height,
            resident_palette, &resident_color0_transparent);
        if (resident_palette_entries != 0u)
        {
            resident_texture_type = GL_RGB16;
            resident_upload_bytes = (upload_width * upload_height + 1u) >> 1;
            if (resident_color0_transparent != FALSE)
            {
                params |= (u32)GL_TEXTURE_COLOR0_TRANSPARENT;
            }
        }
    }
#if NDS_TICK_HUD
    /* R2-07: the same span, re-marked, because upload_start above only exists
     * at NDS_RENDERER_PROFILE_LEVEL >= 1 and the tick-HUD build is level 0. */
    tickhud_upload_mark = cpuGetTiming();
#endif
    if (upload_buffer == sNdsRendererHardwareTextureRefreshLarge)
    {
        staged_row_bytes = upload_width * sizeof(u16);
        staged_row_count = upload_height;
        if ((compact_row_output == FALSE) &&
            (ndsRendererHardwareStageUniqueTextureRows(
                sNdsRendererHardwareTextureScratch, upload_buffer,
                sizeof(sNdsRendererHardwareTextureRefreshLarge),
                staged_row_map, staged_row_bytes, staged_row_count,
                &staged_bytes) == FALSE))
        {
            upload_buffer = sNdsRendererHardwareTextureScratch;
            queue_texture_refresh = FALSE;
            staged_bytes = upload_bytes;
            staged_row_bytes = 0u;
            staged_row_count = 0u;
        }
    }
    else if (upload_buffer != sNdsRendererHardwareTextureScratch)
    {
        memcpy(upload_buffer, sNdsRendererHardwareTextureScratch,
               staged_bytes);
    }
#if NDS_TICK_HUD
    if ((queue_texture_refresh != FALSE) &&
        (upload_buffer == sNdsRendererHardwareTextureRefreshSmall))
    {
        NDS_RENDERER_CAPACITY_HIGH_WATER(
            gNdsRendererTextureRefreshSmallBytesHighWater, staged_bytes);
    }
    else if ((queue_texture_refresh != FALSE) &&
             (upload_buffer == sNdsRendererHardwareTextureRefreshLarge))
    {
        NDS_RENDERER_CAPACITY_HIGH_WATER(
            gNdsRendererTextureRefreshLargeBytesHighWater, staged_bytes);
    }
#endif
    entry = fraction_entry;
    if (entry != NULL)
    {
        s32 refresh_ready = FALSE;

        if (queue_texture_refresh != FALSE)
        {
            refresh_ready = ndsRendererHardwareQueueTextureRefresh(
                entry, upload_buffer, staged_bytes, upload_bytes,
                (staged_row_count != 0u) ? staged_row_map : NULL,
                staged_row_bytes, staged_row_count);
        }
        if ((refresh_ready == FALSE) &&
            (ndsRendererHardwareReplaceTextureData(
                 entry, upload_buffer, staged_bytes, upload_bytes, NULL, 0u,
                 0u) != FALSE))
        {
            refresh_ready = TRUE;
        }
        if (refresh_ready == FALSE)
        {
            entry = ndsRendererHardwareReleaseTexture(entry);
            entry = NULL;
        }
        else
        {
            ndsRendererProfileRecordTexel1Refresh();
        }
    }
    if (entry == NULL)
    {
        entry = ndsRendererHardwareAllocTexture();
        if (entry == NULL)
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size, NDS_RENDERER_HW_TEXREJECT_ALLOC);
            return FALSE;
        }
        /* Slice 2c: the admission carves -- its own VRAM, no libnds name or
         * record, no libc -- and names the entry NDS_FTR_CARVE_NAME_BASE |
         * slot. The bind below then goes through the carved words. */
        if ((sNdsFtrCarveActive != 0u) && (entry->name == 0))
        {
            ndsRendererHardwareEndBatch();
            if (ndsFtrCarveUpload(entry, (u32)resident_texture_type,
                                  (u32)size_x, (u32)size_y, params,
                                  sNdsRendererHardwareTextureScratch,
                                  resident_palette_entries,
                                  resident_palette) == FALSE)
            {
                ndsRendererHardwareRejectTexture(
                    stats, format, size, NDS_RENDERER_HW_TEXREJECT_TEXIMAGE);
                return FALSE;
            }
        }
        else
        {
        if (entry->name == 0)
        {
            if (ndsRendererHardwareFencedGlGenTextures(
                    1, &entry->name) == 0)
            {
                ndsRendererHardwareRejectTexture(
                    stats, format, size, NDS_RENDERER_HW_TEXREJECT_GENTEX);
                return FALSE;
            }
        }

        ndsRendererHardwareEndBatch();
        ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        upload_attempts = 0u;
        holes_open = FALSE;
        while (ndsRendererHardwareFencedGlTexImage2D(
                   GL_TEXTURE_2D, 0, resident_texture_type, size_x, size_y, 0,
                   params, sNdsRendererHardwareTextureScratch) == 0)
        {
            (void)ndsRendererHardwareReleaseTexture(entry);
            upload_attempts++;
            if ((upload_attempts >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT) ||
                (ndsRendererHardwareEvictTexture(entry) == FALSE))
            {
                /* Nothing in the cache may go: once more with A+B's holes
                 * (ndsFtrLeanAdmitOpenHoles), then refuse. */
                if ((holes_open != FALSE) ||
                    (ndsFtrLeanAdmitOpenHoles() == FALSE))
                {
                    if (holes_open != FALSE)
                    {
                        ndsFtrLeanAdmitCloseHoles();
                    }
                    ndsRendererHardwareRejectTexture(
                        stats, format, size,
                        NDS_RENDERER_HW_TEXREJECT_TEXIMAGE);
                    return FALSE;
                }
                holes_open = TRUE;
            }
            if (ndsRendererHardwareFencedGlGenTextures(
                    1, &entry->name) == 0)
            {
                if (holes_open != FALSE)
                {
                    ndsFtrLeanAdmitCloseHoles();
                }
                ndsRendererHardwareRejectTexture(
                    stats, format, size,
                    NDS_RENDERER_HW_TEXREJECT_GENTEX);
                return FALSE;
            }
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        }
        if (holes_open != FALSE)
        {
            ndsFtrLeanAdmitCloseHoles();
        }
        if (resident_palette_entries != 0u)
        {
            glColorTableEXT(GL_TEXTURE_2D, 0, (int)resident_palette_entries,
                            0, 0, resident_palette);
        }
        }
    }
    ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
#if NDS_TICK_HUD
    gNdsMiscTexUploadTicks += cpuGetTiming() - tickhud_upload_mark;
#endif
    /* P2-2p8 Phase 1 slice 1: fighter texture uploads (after GO). */
    ndsFtrLeanNoteTextureUpload(resident_upload_bytes);

    ndsRendererHardwareTextureLookupRemove(entry);
    ndsRendererHardwareEntrySetKey(entry, &key);
    sNdsRendererHardwareTextureKeyGeneration++;
    if (sNdsRendererHardwareTextureKeyGeneration == 0u)
    {
        sNdsRendererHardwareTextureKeyGeneration++;
    }
    entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
    entry->key_hash = key_hash;
    entry->params = ndsRendererHardwareMergeTextureParams(params);
#if NDS_VRAM_CENSUS_LIVE
    /* Slice 2a (lab): the distinct fighter textures each slot uploads. */
    ndsVramCensusNoteCacheKey(entry, &key, resident_palette_entries);
#endif
    (void)green_texels;
    (void)nonwhite_texels;
    entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    entry->ready = TRUE;
    ndsRendererHardwareTextureLookupInsert(entry);
    sNdsRendererHardwareActiveTextureEntry = entry;
    stats->hardware_texture_upload_count++;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = format;
    stats->hardware_texture_width = width;
    stats->hardware_texture_height = height;
    ndsRendererHardwareApplyTextureParams(entry->params);
    ndsRendererProfileRecordTextureUpload(resident_upload_bytes);
    ndsRendererStageTextureSiteRemember(
        state, stats, entry, format, size, width, height);
    return TRUE;
}
