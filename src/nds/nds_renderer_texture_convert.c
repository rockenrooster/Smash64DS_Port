/* Native renderer, texture conversion: N64 texel formats to the DS's, the PAL16
 * repack, texel reads and colours, TEXEL1 sources, CI4 maps and the PRIM rgb
 * TEXEL0 alpha tiles. A slice of nds_renderer.c. */

static u16 ndsRendererHardwareConvertRgba16(u16 n64_color,
                                            s32 preserve_transparent_rgb)
{
    u16 red;
    u16 green;
    u16 blue;
    u16 alpha;

    if (((n64_color & 1u) == 0u) &&
        (preserve_transparent_rgb == FALSE))
    {
        return 0u;
    }

    red = (u16)((n64_color >> 11) & 0x1fu);
    green = (u16)((n64_color >> 6) & 0x1fu);
    blue = (u16)((n64_color >> 1) & 0x1fu);
    alpha = (n64_color & 1u) ? (1u << 15) : 0u;
    return (u16)(alpha | red | (green << 5) | (blue << 10));
}

static u16 ndsRendererHardwareConvertRgba32(u32 rgba,
                                            s32 preserve_transparent_rgb)
{
    u8 red = (u8)(rgba >> 24);
    u8 green = (u8)(rgba >> 16);
    u8 blue = (u8)(rgba >> 8);
    u8 alpha = (u8)rgba;

    if ((alpha == 0u) && (preserve_transparent_rgb == FALSE))
    {
        return 0u;
    }
    return (u16)(((alpha != 0u) ? (1u << 15) : 0u) |
                 ((u16)(red >> 3)) |
                 ((u16)(green >> 3) << 5) |
                 ((u16)(blue >> 3) << 10));
}

static u16 ndsRendererHardwareConvertI(u8 intensity)
{
    u16 v;

    v = (u16)(intensity >> 3);
    return (u16)((1u << 15) | v | (v << 5) | (v << 10));
}

static u16 ndsRendererHardwareConvertI16(u16 value)
{
    u8 intensity = (u8)(value >> 8);

    if (intensity == 0u)
    {
        intensity = (u8)value;
    }
    return ndsRendererHardwareConvertI(intensity);
}

static u16 ndsRendererHardwareConvertIA(u8 intensity, u8 alpha,
                                        s32 preserve_transparent_rgb)
{
    u16 v;

    if ((alpha == 0u) && (preserve_transparent_rgb == FALSE))
    {
        return 0u;
    }
    v = (u16)(intensity >> 3);
    return (u16)(((alpha != 0u) ? (1u << 15) : 0u) |
                 v | (v << 5) | (v << 10));
}

/* Losslessly repack a resolved RGB5A1 image into the DS's native 16-colour
 * format when the final image actually uses at most sixteen visible colours.
 *
 * This is the runtime twin of generate_battle_playable_static_textures.py's
 * repack_paletted(): the renderer has already resolved every N64 texture,
 * palette, material and combiner contribution into canonical RGB5A1 pixels, so
 * this changes representation only. Alpha-zero RGB is intentionally collapsed
 * to palette index 0 because those colour bits are not visible; every opaque
 * halfword remains exact. GL_RGB16 then stores two texels per byte instead of
 * one 16-bit halfword per texel, returning 75% of the texture-VRAM footprint.
 *
 * `pixels` is packed in place. The packed write cursor is always behind the
 * unread u16 cursor, so a separate scratch allocation is unnecessary. Returns
 * the palette entry count, or zero when the image needs direct colour. */
static u32 ndsRendererHardwarePackResolvedPal16(
    u16 *pixels, u32 pixel_count, u16 palette[16], u32 *color0_transparent)
{
    /* Colour -> palette index through a 32-slot open-addressed table and a
     * last-colour shortcut (P2-2p8, 2026-10-06): the per-pixel linear search
     * of up to 16 entries, twice, was 614K of the ~1.0M cycles one 64x64
     * first-use conversion cost (artifacts/performance/2026-10-06_texmiss).
     * Same palette, same order, same packing. Slot 0xff is empty; the opaque
     * bit is folded in so 0x0000 and 0x8000 cannot collide by construction. */
    u8 *packed = (u8 *)pixels;
    u16 keys[32];
    u8 slots[32];
    u32 palette_count = 0u;
    u32 last = 0x10000u;
    u32 last_index = 0u;
    u32 i;

    if ((pixels == NULL) || (palette == NULL) ||
        (color0_transparent == NULL) || (pixel_count == 0u))
    {
        return 0u;
    }
    memset(slots, 0xff, sizeof(slots));

    for (i = 0u; i < pixel_count; i++)
    {
        const u32 color = ((pixels[i] & 0x8000u) != 0u) ? pixels[i] : 0u;
        u32 h;

        if (color == last)
        {
            continue;
        }
        last = color;
        h = (color ^ (color >> 5) ^ (color >> 10)) & 31u;
        while ((slots[h] != 0xffu) && (keys[h] != (u16)color))
        {
            h = (h + 1u) & 31u;
        }
        if (slots[h] == 0xffu)
        {
            if (palette_count >= 16u)
            {
                return 0u;
            }
            keys[h] = (u16)color;
            slots[h] = (u8)palette_count;
            palette[palette_count++] = (u16)color;
        }
    }

    /* COLOR0_TRANSPARENT applies to index zero only. Keep the generator's
     * invariant that a transparent image has canonical 0 in that slot. */
    *color0_transparent = FALSE;
    for (i = 0u; i < palette_count; i++)
    {
        if (palette[i] == 0u)
        {
            u16 first = palette[0];
            u32 h;

            palette[0] = 0u;
            palette[i] = first;
            *color0_transparent = TRUE;
            if (i != 0u)
            {
                for (h = 0u; h < 32u; h++)
                {
                    if (slots[h] == 0u)
                    {
                        slots[h] = (u8)i;
                    }
                    else if (slots[h] == (u8)i)
                    {
                        slots[h] = 0u;
                    }
                }
            }
            break;
        }
    }

    last = 0x10000u;
    for (i = 0u; i < pixel_count; i++)
    {
        const u32 color = ((pixels[i] & 0x8000u) != 0u) ? pixels[i] : 0u;

        if (color != last)
        {
            u32 h = (color ^ (color >> 5) ^ (color >> 10)) & 31u;

            while (keys[h] != (u16)color)
            {
                h = (h + 1u) & 31u;
            }
            last = color;
            last_index = slots[h];
        }
        if ((i & 1u) != 0u)
        {
            packed[i >> 1] |= (u8)(last_index << 4);
        }
        else
        {
            packed[i >> 1] = (u8)last_index;
        }
    }
    return palette_count;
}

static u32 ndsRendererHardwareTextureLinePixels(u32 size, u32 line)
{
    switch (size)
    {
    case NDS_RENDERER_HW_TEXTURE_SIZ_4B:
        return line * 16u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_8B:
        return line * 8u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_16B:
        return line * 4u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_32B:
        return line * 2u;
    default:
        return 0u;
    }
}

/* Task 37: 132 bytes at 8.54 cycles per instruction, called from every texture
 * bind and resolve. Small, hot, and reached from many different call sites --
 * the shape that never stays resident in a shared instruction cache. */
static u32 NDS_R2_ITCM_PACK2_EVICTED_PLAIN_CODE ndsRendererHardwareTextureSourceBytes(
    u32 format, u32 size, u32 texels)
{
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_RGBA16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            return texels * sizeof(u16);
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
            texels * sizeof(u32) : 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_I16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ?
            texels * sizeof(u16) : 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_IA)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ?
            texels * sizeof(u16) : 0u;
    }
    return 0u;
}

static u32 ndsRendererHardwareTextureSourceWidthPixels(u32 render_size,
                                                       u32 image_size,
                                                       u32 image_width)
{
    if ((render_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (image_size == NDS_RENDERER_HW_TEXTURE_SIZ_8B))
    {
        return image_width * 2u;
    }
    return image_width;
}


static NDSRendererTextureDataLayout ndsRendererTextureDataLayout(
    const NDSRendererConfig *config)
{
    if (config == NULL)
    {
        return NDS_RENDERER_TEXTURE_DATA_NATIVE;
    }
    if (config->texture_data_layout ==
        NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED)
    {
        return NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    }
    return NDS_RENDERER_TEXTURE_DATA_NATIVE;
}



static u32 ndsRendererTextureLogicalByteIndex(
    NDSRendererTextureDataLayout layout, u32 logical_index)
{
    return (layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ?
        (logical_index ^ 3u) : logical_index;
}

static u32 ndsRendererTextureLogicalHalfwordIndex(
    NDSRendererTextureDataLayout layout, u32 logical_index)
{
    return (layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ?
        (logical_index ^ 1u) : logical_index;
}

static u32 ndsRendererTexturePhysicalByteSpan(
    NDSRendererTextureDataLayout layout, u32 logical_bytes)
{
    if ((layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) &&
        (logical_bytes != 0u))
    {
        return (logical_bytes + 3u) & ~3u;
    }
    return logical_bytes;
}

static u8 ndsRendererReadTextureByte(
    const NDSRendererConfig *config, const u8 *texels, u32 logical_index,
    u32 format, u32 size)
{
    NDSRendererTextureDataLayout layout = ndsRendererTextureDataLayout(config);

    /* The conversion loop aggregates this access after rasterization. Do not
     * update volatile diagnostics for every converted texel: animated
     * TEXEL0/TEXEL1 water reads this path tens of thousands of times per
     * frame, while the lane contract is invariant for the conversion. */
    (void)format;
    (void)size;
    return texels[ndsRendererTextureLogicalByteIndex(layout, logical_index)];
}

static u8 ndsRendererReadTexturePackedNibble(
    const NDSRendererConfig *config, const u8 *texels, u32 logical_texel_index,
    u32 format, u32 size)
{
    u8 packed = ndsRendererReadTextureByte(
        config, texels, logical_texel_index >> 1, format, size);

    return ((logical_texel_index & 1u) == 0u) ?
        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
}

NDS_RENDERER_COLD static u32 ndsRendererHardwareCiPaletteEntriesUsed(
    const NDSRendererConfig *config,
    const u8 *texels,
    u32 size,
    u32 source_width,
    u32 source_origin_s,
    u32 source_origin_t,
    u32 source_read_width,
    u32 source_read_height,
    u32 palette_base)
{
    u32 max_index = 0u;
    u32 x;
    u32 y;

    if ((texels == NULL) || (source_width == 0u) ||
        (source_read_width == 0u) || (source_read_height == 0u))
    {
        return palette_base;
    }
    for (y = 0u; y < source_read_height; y++)
    {
        u32 row = (source_origin_t + y) * source_width;

        for (x = 0u; x < source_read_width; x++)
        {
            u32 source_index = row + source_origin_s + x;
            u32 palette_index =
                (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
                    ndsRendererReadTexturePackedNibble(
                        config, texels, source_index,
                        NDS_RENDERER_HW_TEXTURE_FMT_CI, size) :
                    ndsRendererReadTextureByte(
                        config, texels, source_index,
                        NDS_RENDERER_HW_TEXTURE_FMT_CI, size);

            if (palette_index > max_index)
            {
                max_index = palette_index;
            }
        }
    }
    return palette_base + max_index + 1u;
}

static u16 ndsRendererReadTextureHalfword(
    const NDSRendererConfig *config, const u16 *data, u32 logical_index,
    u32 format, u32 size)
{
    NDSRendererTextureDataLayout layout = ndsRendererTextureDataLayout(config);

    (void)format;
    (void)size;
    return data[ndsRendererTextureLogicalHalfwordIndex(layout, logical_index)];
}


static u16 ndsRendererHardwarePaletteColor(
    const NDSRendererConfig *config, const u16 *palette, u32 index, u32 count,
    s32 preserve_transparent_rgb)
{
    if ((palette == NULL) || (index >= count))
    {
        return 0u;
    }
    return ndsRendererHardwareConvertRgba16(
        ndsRendererReadTextureHalfword(
            config, palette, index, NDS_RENDERER_HW_TEXTURE_FMT_CI,
            NDS_RENDERER_HW_TEXTURE_SIZ_16B),
        preserve_transparent_rgb);
}

static u16 ndsRendererHardwareTextureColor(
    const NDSRendererConfig *config,
    u32 format,
    u32 size,
    const u8 *texels,
    const u16 *palette,
    u32 palette_count,
    u32 palette_base,
    u32 index,
    s32 preserve_transparent_rgb)
{
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        u32 palette_index;

        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            palette_index = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);
        }
        else
        {
            palette_index = ndsRendererReadTextureByte(
                config, texels, index, format, size);
        }
        palette_index += palette_base;
        return ndsRendererHardwarePaletteColor(config, palette, palette_index,
                                               palette_count,
                                               preserve_transparent_rgb);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_RGBA16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B)
        {
            u32 rgba;

            memcpy(&rgba, &texels[index * sizeof(rgba)], sizeof(rgba));
            return ndsRendererHardwareConvertRgba32(
                rgba, preserve_transparent_rgb);
        }
        return ndsRendererHardwareConvertRgba16(
            ndsRendererReadTextureHalfword(
                config, (const u16 *)texels, index, format, size),
            preserve_transparent_rgb);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_IA)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            u8 value = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);
            u8 intensity = (u8)(((value >> 1) & 0x07u) * 0x24u);
            u8 alpha = (value & 1u) ? 0xffu : 0u;

            return ndsRendererHardwareConvertIA(
                intensity, alpha, preserve_transparent_rgb);
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            u8 value = ndsRendererReadTextureByte(
                config, texels, index, format, size);
            u8 intensity = (u8)((value >> 4) * 0x11u);
            u8 alpha = (u8)((value & 0x0fu) * 0x11u);

            return ndsRendererHardwareConvertIA(
                intensity, alpha, preserve_transparent_rgb);
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            u16 value = ndsRendererReadTextureHalfword(
                config, (const u16 *)texels, index, format, size);

            return ndsRendererHardwareConvertIA(
                (u8)(value >> 8), (u8)value, preserve_transparent_rgb);
        }
        return 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_I16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            u8 intensity4 = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);

            return ndsRendererHardwareConvertI((u8)(intensity4 * 0x11u));
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return ndsRendererHardwareConvertI(
                ndsRendererReadTextureByte(config, texels, index,
                                           format, size));
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            return ndsRendererHardwareConvertI16(
                ndsRendererReadTextureHalfword(
                    config, (const u16 *)texels, index, format, size));
        }
    }
    return 0u;
}

/* ndsRendererHardwareTextureColor for one stored value of a 4- or 8-bit CI,
 * IA or I texel: the same branches with the texel read already done. The
 * conversion loop's per-value table below calls it once per value. */
/* The conversion loop's per-value colours (see the resolver). */
static u16 sNdsRendererHardwareTextureValueLut[256];

static u16 ndsRendererHardwareTextureValueColor(
    const NDSRendererConfig *config,
    u32 format,
    u32 size,
    const u16 *palette,
    u32 palette_count,
    u32 palette_base,
    u32 value,
    s32 preserve_transparent_rgb)
{
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        return ndsRendererHardwarePaletteColor(config, palette,
                                               value + palette_base,
                                               palette_count,
                                               preserve_transparent_rgb);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_IA)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            u8 intensity = (u8)(((value >> 1) & 0x07u) * 0x24u);
            u8 alpha = (value & 1u) ? 0xffu : 0u;

            return ndsRendererHardwareConvertIA(
                intensity, alpha, preserve_transparent_rgb);
        }
        {
            u8 intensity = (u8)((value >> 4) * 0x11u);
            u8 alpha = (u8)((value & 0x0fu) * 0x11u);

            return ndsRendererHardwareConvertIA(
                intensity, alpha, preserve_transparent_rgb);
        }
    }
    if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
    {
        return ndsRendererHardwareConvertI((u8)(value * 0x11u));
    }
    return ndsRendererHardwareConvertI((u8)value);
}


/* Stage textures whose palette read ran past a short LOADTLUT (see the
 * palette block in ndsRendererHardwareResolveOrBindTexture). */
volatile u32 gNdsStageShortTlutExtendCount;

static void ndsRendererHardwareRejectTexture(NDSRendererStats *stats,
                                             u32 format, u32 size,
                                             u32 reason)
{
    /* P2-2p8 Phase 1 slice 1: the fighter texture-admission census's reject
     * witness (first fighter-owned reject, reason + cache census). */
    ndsFtrLeanNoteTextureReject(reason, format, size);
#if NDS_VRAM_CENSUS_LIVE
    gNdsFtrAdmitLastReject = reason;
#endif
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_MANIFEST_FALLBACK);
    if (stats != NULL)
    {
        stats->hardware_texture_reject_count++;
    }
    (void)format;
    (void)size;
    (void)reason;
}

static s32 ndsRendererHardwarePrepareTexel1Source(
    const NDSRendererStats *stats,
    const NDSRendererConfig *config,
    u32 primary_format,
    u32 primary_size,
    u32 primary_width,
    u32 primary_height,
    NDSRendererHardwareTexel1Source *out)
{
    const NDSRendererTileState *tile;
    const NDSRendererTextureLoadState *load;
    u32 loaded_bytes;
    u32 width;
    u32 height;
    u32 texels;
    u32 source_read_width;
    u32 source_read_height;
    u32 source_last_index;
    u32 source_bytes;
    u32 source_physical_bytes;
    u32 source_width;
    u32 source_origin_s;
    u32 source_origin_t;
    s32 materialize_s;
    s32 materialize_t;

    if (out == NULL)
    {
        return FALSE;
    }
    memset(out, 0, sizeof(*out));
    if ((stats == NULL) ||
        (ndsRendererHardwareUsesTexel01Lerp(stats) == FALSE) ||
        (ndsRendererActiveTextureTile(stats) != NDS_RENDERER_RENDER_TILE))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_ACTIVE_TILE);
        return FALSE;
    }

    tile = &stats->texture_tiles[NDS_RENDERER_RENDER_TILE_1];
    if ((primary_format != NDS_RENDERER_HW_TEXTURE_FMT_CI) ||
        (primary_size != NDS_RENDERER_HW_TEXTURE_SIZ_4B) ||
        (tile->set_seen == 0u) || (tile->size_seen == 0u) ||
        (tile->line == 0u) || (tile->shifts != 0u) ||
        (tile->shiftt != 0u) ||
        (tile->format != primary_format) || (tile->size != primary_size))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_TILE_STATE);
        return FALSE;
    }
    load = ndsRendererHardwareFindTextureLoadForTmem(stats, tile->tmem);
    if ((load == NULL) || (load->image == 0u) ||
        (load->load_texels == 0u))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_LOAD_STATE);
        return FALSE;
    }

    loaded_bytes = (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
        load->load_texels * sizeof(u32) :
        load->load_texels * sizeof(u16);
    width = tile->width;
    height = tile->height;
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        (ndsRendererHardwareTextureSourceBytes(
             primary_format, primary_size, width * height) > loaded_bytes))
    {
        width = ndsRendererHardwareTextureLinePixels(primary_size,
                                                     tile->line);
        texels = load->load_texels * sizeof(u16);
        if (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            texels *= 2u;
        }
        else if ((primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ||
                 (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_32B))
        {
            texels /= 2u;
        }
        height = (width != 0u) ? texels / width : 0u;
    }
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_DIMENSIONS);
        return FALSE;
    }

    out->source_extent_width = width;
    out->source_extent_height = height;
    materialize_s = ndsRendererHardwareTextureMaterializesMaskedClamp(
        tile->cms, tile->masks, width, tile->width);
    materialize_t = ndsRendererHardwareTextureMaterializesMaskedClamp(
        tile->cmt, tile->maskt, height, tile->height);
    if (materialize_s != FALSE)
    {
        width = tile->width;
    }
    if (materialize_t != FALSE)
    {
        height = tile->height;
    }
    width = ndsRendererHardwareTextureWrapPeriodExtent(
        tile->cms, tile->masks, width);
    height = ndsRendererHardwareTextureWrapPeriodExtent(
        tile->cmt, tile->maskt, height);
    if ((width != primary_width) || (height != primary_height))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_PAIR_SIZE);
        return FALSE;
    }

    if (load->load_kind == NDS_RENDERER_TEXTURE_LOADTILE)
    {
        source_origin_s = load->load_uls >> 2;
        source_origin_t = load->load_ult >> 2;
        source_width = ndsRendererHardwareTextureSourceWidthPixels(
            primary_size, load->image_size, load->image_width);
    }
    else
    {
        u32 dxt = load->load_dxt;

        source_origin_s = 0u;
        source_origin_t = 0u;
        source_width = out->source_extent_width;
        if (dxt != 0u)
        {
            u32 qwords =
                (NDS_RENDERER_G_TX_DXT_ONE + dxt - 1u) / dxt;

            source_width = ndsRendererHardwareTextureLinePixels(
                primary_size, qwords);
        }
    }
    source_read_width = (materialize_s != FALSE) ?
        (1u << tile->masks) : width;
    source_read_height = (materialize_t != FALSE) ?
        (1u << tile->maskt) : height;
    if ((source_width == 0u) ||
        (source_origin_s >= source_width) ||
        (source_read_width > (source_width - source_origin_s)) ||
        (source_read_width > out->source_extent_width) ||
        (source_read_height > out->source_extent_height))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_RANGE);
        return FALSE;
    }

    source_last_index =
        ((source_origin_t + source_read_height - 1u) * source_width) +
        source_origin_s + source_read_width - 1u;
    source_bytes = ndsRendererHardwareTextureSourceBytes(
        primary_format, primary_size, source_last_index + 1u);
    if (source_bytes == 0u)
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_BYTES);
        return FALSE;
    }
    source_physical_bytes = ndsRendererTexturePhysicalByteSpan(
        ndsRendererTextureDataLayout(config), source_bytes);
    out->texels = ndsRendererResolveTextureDataPointer(
        config, (const void *)(uintptr_t)load->image,
        source_physical_bytes);
    if (out->texels == NULL)
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_PTR);
        return FALSE;
    }

    out->load = load;
    out->render_tile = tile;
    out->format = primary_format;
    out->size = primary_size;
    out->width = width;
    out->height = height;
    out->source_width = source_width;
    out->source_texels = source_last_index + 1u;
    out->source_origin_s = source_origin_s;
    out->source_origin_t = source_origin_t;
    out->palette_base = (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
        tile->palette * 16u : 0u;
    out->materialize_s = materialize_s;
    out->materialize_t = materialize_t;
    return TRUE;
}

static s32 ndsRendererHardwareTileOriginDelta(u32 primary, u32 secondary)
{
    s32 delta = (s32)((primary - secondary) & 0x0fffu);

    if ((delta & 0x0800) != 0)
    {
        delta -= 0x1000;
    }
    return delta;
}

static s32 ndsRendererHardwareQuarterToTexel(s32 coord)
{
    if (coord < 0)
    {
        return -(((-coord) + 2) >> 2);
    }
    return (coord + 2) >> 2;
}

static u32 ndsRendererHardwareTextureAddressCoord(
    s32 coord, u32 logical_extent, u32 source_extent, u32 mode, u32 mask)
{
    s32 period;
    s32 local;
    u32 mask_extent;

    if (source_extent == 0u)
    {
        return 0u;
    }
    /* The common interior case is already in the first source/mask period.
     * Return it directly instead of paying signed divide/modulo for every
     * TEXEL1 pixel; edge, wrap and mirror coordinates retain the full path. */
    if ((coord >= 0) && ((u32)coord < source_extent) &&
        ((logical_extent == 0u) || ((u32)coord < logical_extent)) &&
        ((mask == 0u) || (mask >= 31u) ||
         ((u32)coord < (1u << mask))))
    {
        return (u32)coord;
    }
    if ((mode & NDS_RENDERER_TX_CLAMP) != 0u)
    {
        if (coord < 0)
        {
            coord = 0;
        }
        else if ((logical_extent != 0u) &&
                 ((u32)coord >= logical_extent))
        {
            coord = (s32)logical_extent - 1;
        }
    }
    if ((mask != 0u) && (mask < 31u))
    {
        mask_extent = 1u << mask;
        period = coord / (s32)mask_extent;
        local = coord % (s32)mask_extent;
        if (local < 0)
        {
            local += (s32)mask_extent;
            period--;
        }
        if (((mode & NDS_RENDERER_TX_MIRROR) != 0u) &&
            ((period & 1) != 0))
        {
            local = (s32)mask_extent - 1 - local;
        }
        return ((u32)local < source_extent) ? (u32)local :
            (source_extent - 1u);
    }
    local = coord % (s32)source_extent;
    if (local < 0)
    {
        local += (s32)source_extent;
    }
    if ((mode & NDS_RENDERER_TX_CLAMP) != 0u)
    {
        return ((u32)coord < source_extent) ? (u32)coord :
            (source_extent - 1u);
    }
    return (u32)local;
}

static u32 ndsRendererHardwareTexel1SourceIndex(
    const NDSRendererHardwareTexel1Source *source,
    s32 origin_delta_s,
    s32 origin_delta_t,
    u32 x,
    u32 y)
{
    s32 source_x;
    s32 source_y;
    u32 addressed_x;
    u32 addressed_y;
    u32 index;

    source_x = (s32)x + origin_delta_s;
    source_y = (s32)y + origin_delta_t;
    addressed_x = ndsRendererHardwareTextureAddressCoord(
        source_x, source->render_tile->width,
        source->source_extent_width, source->render_tile->cms,
        source->render_tile->masks);
    addressed_y = ndsRendererHardwareTextureAddressCoord(
        source_y, source->render_tile->height,
        source->source_extent_height, source->render_tile->cmt,
        source->render_tile->maskt);
    index = ((source->source_origin_t + addressed_y) *
             source->source_width) + source->source_origin_s + addressed_x;
    return index;
}

static u16 ndsRendererHardwareTexel1Color(
    const NDSRendererHardwareTexel1Source *source,
    const NDSRendererConfig *config,
    const u16 *palette,
    u32 palette_count,
    s32 origin_delta_s,
    s32 origin_delta_t,
    u32 x,
    u32 y)
{
    u32 index;

    if ((source == NULL) || (source->render_tile == NULL) ||
        (source->texels == NULL))
    {
        return 0u;
    }
    index = ndsRendererHardwareTexel1SourceIndex(
        source, origin_delta_s, origin_delta_t, x, y);
    return ndsRendererHardwareTextureColor(
        config, source->format, source->size, source->texels, palette,
        palette_count, source->palette_base, index, TRUE);
}

static u32 ndsRendererHardwareAlphaCoverageThreshold(u32 x, u32 y)
{
    static const u8 bayer4x4[16] = {
        0u, 8u, 2u, 10u,
        12u, 4u, 14u, 6u,
        3u, 11u, 1u, 9u,
        15u, 7u, 13u, 5u
    };

    return ((u32)bayer4x4[((y & 3u) << 2) | (x & 3u)] << 4) + 8u;
}

static u32 ndsRendererHardwareExpand5To8(u32 value)
{
    value &= 0x1fu;
    return (value << 3) | (value >> 2);
}

static u32 ndsRendererHardwareBlendTexel01Value(u16 texel0, u16 texel1,
                                                u32 fraction)
{
    u32 inverse;
    u32 red;
    u32 green;
    u32 blue;
    u32 alpha_coverage;
    u32 texel0_red;
    u32 texel0_green;
    u32 texel0_blue;
    u32 texel1_red;
    u32 texel1_green;
    u32 texel1_blue;

    if (fraction > 0xffu)
    {
        fraction = 0xffu;
    }
    inverse = 0x100u - fraction;
    texel0_red = ndsRendererHardwareExpand5To8(texel0 >> 0);
    texel0_green = ndsRendererHardwareExpand5To8(texel0 >> 5);
    texel0_blue = ndsRendererHardwareExpand5To8(texel0 >> 10);
    texel1_red = ndsRendererHardwareExpand5To8(texel1 >> 0);
    texel1_green = ndsRendererHardwareExpand5To8(texel1 >> 5);
    texel1_blue = ndsRendererHardwareExpand5To8(texel1 >> 10);
    red = (((texel0_red * inverse) + (texel1_red * fraction)) >> 8) >> 3;
    green = (((texel0_green * inverse) +
              (texel1_green * fraction)) >> 8) >> 3;
    blue = (((texel0_blue * inverse) +
             (texel1_blue * fraction)) >> 8) >> 3;
    /* G_RM_AA_TEX_EDGE2 converts the fractional alpha lerp to coverage. DS
     * direct-color textures expose A1 only, so retain the same mean coverage
     * with an ordered 4x4 decision instead of unioning both silhouettes. */
    alpha_coverage = ((((texel0 >> 15) & 1u) * 0x100u * inverse) +
                      (((texel1 >> 15) & 1u) * 0x100u * fraction)) >> 8;
    return red | (green << 5) | (blue << 10) |
        (alpha_coverage << NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT);
}

static u16 ndsRendererHardwareResolveTexel01Value(u32 value, u32 x, u32 y)
{
    u32 alpha_coverage =
        value >> NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT;
    u32 alpha = (alpha_coverage >
                 ndsRendererHardwareAlphaCoverageThreshold(x, y)) ? 1u : 0u;

    return (u16)((value & NDS_RENDERER_HW_TEXEL01_RGB_MASK) |
                 (alpha << 15));
}

static u16 ndsRendererHardwareBlendTexel01(u16 texel0, u16 texel1,
                                           u32 fraction, u32 x, u32 y)
{
    return ndsRendererHardwareResolveTexel01Value(
        ndsRendererHardwareBlendTexel01Value(texel0, texel1, fraction),
        x, y);
}

static u16 ndsRendererHardwareBlendPrimEnvTexel0(u16 texel0,
                                                 u32 primitive,
                                                 u32 environment)
{
    u32 red_weight = (texel0 >> 0) & 0x1fu;
    u32 green_weight = (texel0 >> 5) & 0x1fu;
    u32 blue_weight = (texel0 >> 10) & 0x1fu;
    u32 primitive_red = (primitive >> 27) & 0x1fu;
    u32 primitive_green = (primitive >> 19) & 0x1fu;
    u32 primitive_blue = (primitive >> 11) & 0x1fu;
    u32 environment_red = (environment >> 27) & 0x1fu;
    u32 environment_green = (environment >> 19) & 0x1fu;
    u32 environment_blue = (environment >> 11) & 0x1fu;
    u32 red;
    u32 green;
    u32 blue;

    /* DS direct-color textures expose RGB5+A1. Interpolate the captured N64
     * endpoint colours at that exact RGB5 ceiling and retain source coverage;
     * polygon alpha supplies PRIMITIVE alpha for the modulated raw form. */
    red = ((environment_red * (31u - red_weight)) +
           (primitive_red * red_weight) + 15u) / 31u;
    green = ((environment_green * (31u - green_weight)) +
             (primitive_green * green_weight) + 15u) / 31u;
    blue = ((environment_blue * (31u - blue_weight)) +
            (primitive_blue * blue_weight) + 15u) / 31u;
    return (u16)((texel0 & 0x8000u) | red | (green << 5) | (blue << 10));
}

/* rgb = PRIMITIVE, alpha = TEXEL0 (NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_
 * TEXEL0_ALPHA). The colour is flat, so the whole job is recovering the source
 * coverage that the I-format arm has already folded away.
 *
 * ndsRendererHardwareConvertI writes v = intensity >> 3 into all three RGB5
 * lanes and sets the alpha bit unconditionally, so the coverage this combine
 * wants is still readable from any one lane -- and reading it back here is why
 * that shared converter is left alone. It serves every I-format texture in the
 * game; the defect is one caller's combine being unrecognised, not the
 * converter being wrong.
 *
 * The destination is RGB555+A1, so coverage quantises to one bit. Round to
 * nearest (>= 16 of 31) rather than "any non-zero": the beam's identity is its
 * bright core, and a threshold of 1 would promote the entire faint outer ramp
 * to fully opaque and draw a blob wider than the source. The lost fade is the
 * hard-edge tradeoff the owner is being asked to judge; GL_RGB8_A5 is what
 * would recover it. */
static u16 ndsRendererHardwarePrimRgbTexel0Alpha(u16 texel0, u32 primitive)
{
    u32 coverage = texel0 & 0x1fu;
    u32 red = (primitive >> 27) & 0x1fu;
    u32 green = (primitive >> 19) & 0x1fu;
    u32 blue = (primitive >> 11) & 0x1fu;
    u16 opaque = (coverage >= 16u) ? (u16)0x8000u : (u16)0u;

    return (u16)(opaque | red | (green << 5) | (blue << 10));
}

NDS_RENDERER_COLD static void __attribute__((noinline))
NDS_RENDERER_COLD ndsRendererHardwareBuildTexel01Ci4Lut(
    const NDSRendererConfig *config,
    const u16 *palette,
    u32 palette_count,
    u32 palette0_base,
    u32 palette1_base,
    u32 fraction)
{
    static const u16 alpha_phase_prefix[17] = {
        0x0000u, 0x0001u, 0x0401u, 0x0405u, 0x0505u, 0x0525u,
        0x8525u, 0x85a5u, 0xa5a5u, 0xa5a7u, 0xada7u, 0xadafu,
        0xafafu, 0xafbfu, 0xefbfu, 0xefffu, 0xffffu
    };
    u16 palette0[16];
    u16 palette1[16];
    u32 index0;
    u32 index1;

    for (index0 = 0u; index0 < 16u; index0++)
    {
        palette0[index0] = ndsRendererHardwarePaletteColor(
            config, palette, palette0_base + index0, palette_count, TRUE);
        palette1[index0] = ndsRendererHardwarePaletteColor(
            config, palette, palette1_base + index0, palette_count, TRUE);
    }
    if ((sNdsRendererHardwareTexel01Ci4LutKeyValid != 0u) &&
        (sNdsRendererHardwareTexel01Ci4LutFraction == fraction) &&
        (memcmp(sNdsRendererHardwareTexel01Ci4LutPalette0,
                palette0, sizeof(palette0)) == 0) &&
        (memcmp(sNdsRendererHardwareTexel01Ci4LutPalette1,
                palette1, sizeof(palette1)) == 0))
    {
        return;
    }
    memcpy(sNdsRendererHardwareTexel01Ci4LutPalette0,
           palette0, sizeof(palette0));
    memcpy(sNdsRendererHardwareTexel01Ci4LutPalette1,
           palette1, sizeof(palette1));
    sNdsRendererHardwareTexel01Ci4LutFraction = fraction;
    sNdsRendererHardwareTexel01Ci4LutKeyValid = TRUE;
    for (index0 = 0u; index0 < 16u; index0++)
    {
        for (index1 = 0u; index1 < 16u; index1++)
        {
            u32 value = ndsRendererHardwareBlendTexel01Value(
                palette0[index0], palette1[index1], fraction);
            u32 alpha_coverage =
                value >> NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT;
            u32 alpha_prefix_count = (alpha_coverage + 7u) >> 4;
            u32 lut_index = (index0 << 4) | index1;
            u16 rgb;
            u16 alpha_phase_mask;

            if (alpha_prefix_count > 16u)
            {
                alpha_prefix_count = 16u;
            }
            rgb = (u16)(value & NDS_RENDERER_HW_TEXEL01_RGB_MASK);
            alpha_phase_mask = alpha_phase_prefix[alpha_prefix_count];
            sNdsRendererHardwareTexel01Ci4PairLut[lut_index] =
                (u32)rgb | ((u32)alpha_phase_mask << 16);
        }
    }
}

static inline u16 ndsRendererHardwareResolveTexel01Ci4Lut(
    u32 index0, u32 index1, u32 x, u32 y, s32 texel1_alpha_from_texel1)
{
    u32 phase = ((y & 3u) << 2) | (x & 3u);
    u32 lut_index = (index0 << 4) | index1;
    u32 pair = sNdsRendererHardwareTexel01Ci4PairLut[lut_index];
    u16 alpha;

    if (texel1_alpha_from_texel1 != FALSE)
    {
        /* TEMPLERP's first-cycle alpha equation resolves to TEXEL1 exactly.
         * Zebes then multiplies that alpha by SHADE, which remains live as
         * DS vertex/polygon alpha. Keep Pupupu's existing ordered-coverage
         * result byte-identical when its second cycle uses PRIMITIVE alpha. */
        alpha = sNdsRendererHardwareTexel01Ci4LutPalette1[index1] & 0x8000u;
    }
    else
    {
        alpha = (u16)(((pair >> (16u + phase)) & 1u) << 15);
    }

    return (u16)((pair & NDS_RENDERER_HW_TEXEL01_RGB_MASK) | alpha);
}

static inline u8 ndsRendererHardwareReadCi4Direct(
    const u8 *texels, u32 logical_texel_index, u32 byte_lane_xor)
{
    u8 packed = texels[(logical_texel_index >> 1) ^ byte_lane_xor];

    return ((logical_texel_index & 1u) == 0u) ?
        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
}

static const u8 *ndsRendererHardwareGetCi4Indices(
    const u8 *source, u32 source_texels, u32 byte_lane_xor,
    const u8 *protected_indices)
{
    NDSRendererHardwareCi4IndexCacheEntry *entry;
    u32 replace_index;
    u32 i;

    if ((source == NULL) || (source_texels == 0u) ||
        (source_texels > NDS_RENDERER_HW_CI4_INDEX_CACHE_TEXELS))
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT; i++)
    {
        entry = &sNdsRendererHardwareCi4IndexCache[i];
        if ((entry->valid != 0u) &&
            (entry->source == source) &&
            (entry->source_texels == source_texels) &&
            (entry->byte_lane_xor == byte_lane_xor))
        {
            ndsRendererProfileRecordCi4IndexCacheReuse();
            return entry->indices;
        }
    }

    replace_index = sNdsRendererHardwareCi4IndexCacheNext;
    if (sNdsRendererHardwareCi4IndexCache[replace_index].indices ==
        protected_indices)
    {
        replace_index = (replace_index + 1u) &
            (NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT - 1u);
    }
    entry = &sNdsRendererHardwareCi4IndexCache[replace_index];
    sNdsRendererHardwareCi4IndexCacheNext =
        (replace_index + 1u) &
        (NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT - 1u);
    entry->valid = FALSE;
    entry->source = source;
    entry->source_texels = source_texels;
    entry->byte_lane_xor = byte_lane_xor;
    for (i = 0u; i < source_texels; i++)
    {
        entry->indices[i] = ndsRendererHardwareReadCi4Direct(
            source, i, byte_lane_xor);
    }
    entry->valid = TRUE;
    ndsRendererProfileRecordCi4IndexCacheBuild();
    return entry->indices;
}

NDS_RENDERER_COLD static u32 ndsRendererHardwareBuildCi4RepresentativeMap(
    const u8 *source0, const u8 *source1, u8 *representative, u32 count)
{
    u32 i;
    u32 unique = 0u;

    /* The exact 18-bit class key plus one reserves zero as empty; the upper
     * field stores the first coordinate. At most 128 coordinates enter the
     * 256-slot table, so linear probing always reaches an empty terminator. */
    memset(sNdsRendererHardwareTexel01Ci4ClassTable, 0,
           sizeof(sNdsRendererHardwareTexel01Ci4ClassTable));
    for (i = 0u; i < count; i++)
    {
        u32 key = ((i & 3u) << 16) |
            ((u32)source1[i] << 8) | source0[i];
        u32 stored_key = key + 1u;
        u32 slot = (key * 0x9e3779b1u) >> 24;

        while (sNdsRendererHardwareTexel01Ci4ClassTable[slot] != 0u)
        {
            u32 entry = sNdsRendererHardwareTexel01Ci4ClassTable[slot];

            if ((entry & NDS_RENDERER_HW_CI4_CLASS_KEY_MASK) == stored_key)
            {
                representative[i] =
                    (u8)(entry >> NDS_RENDERER_HW_CI4_CLASS_INDEX_SHIFT);
                break;
            }
            slot = (slot + 1u) &
                (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT - 1u);
        }
        if (sNdsRendererHardwareTexel01Ci4ClassTable[slot] == 0u)
        {
            sNdsRendererHardwareTexel01Ci4ClassTable[slot] =
                (i << NDS_RENDERER_HW_CI4_CLASS_INDEX_SHIFT) | stored_key;
            representative[i] = (u8)i;
            unique++;
        }
    }
    return unique;
}

static s32 ndsRendererHardwareStageUniqueTextureRows(
    const u16 *source,
    u16 *staging,
    u32 staging_bytes,
    u8 *row_map,
    u32 row_bytes,
    u32 row_count,
    u32 *staged_bytes)
{
    u32 staging_rows;
    u32 unique_rows = 0u;
    u32 y;

    if ((source == NULL) || (staging == NULL) || (row_map == NULL) ||
        (staged_bytes == NULL) || (row_bytes == 0u) || (row_count == 0u) ||
        (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        ((row_bytes & (sizeof(u16) - 1u)) != 0u))
    {
        return FALSE;
    }
    staging_rows = staging_bytes / row_bytes;
    if ((staging_rows == 0u) ||
        (staging_rows > NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS))
    {
        return FALSE;
    }

    if (sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid == 0u)
    {
        return FALSE;
    }
    for (y = 0u; y < row_count; y++)
    {
        u32 representative =
            sNdsRendererHardwareTexel01Ci4RepresentativeT[y];

        if ((representative >= row_count) || (representative > y))
        {
            return FALSE;
        }
        if (representative == y)
        {
            if (unique_rows >= staging_rows)
            {
                return FALSE;
            }
            memcpy((u8 *)staging + (unique_rows * row_bytes),
                   (const u8 *)source + (y * row_bytes), row_bytes);
            row_map[y] = (u8)unique_rows;
            unique_rows++;
        }
        else
        {
            row_map[y] = row_map[representative];
        }
    }
    *staged_bytes = unique_rows * row_bytes;
    return TRUE;
}

static s32 NDS_RENDERER_EVICTED_HOT_CODE
ndsRendererHardwareConvertTexel01Ci4Direct(
    const NDSRendererConfig *config,
    const u8 *texels0,
    u32 source0_texels,
    u32 source0_width,
    u32 source0_origin_s,
    u32 source0_origin_t,
    const NDSRendererTileState *render_tile0,
    s32 materialize0_s,
    s32 materialize0_t,
    const NDSRendererHardwareTexel1Source *source1,
    s32 origin1_delta_s,
    s32 origin1_delta_t,
    s32 texel1_alpha_from_texel1,
    u32 width,
    u32 height,
    u32 upload_width,
    u8 *compact_row_map,
    u32 *compact_staged_bytes,
    u32 *green_texels,
    u32 *nonwhite_texels)
{
    u32 byte_lane_xor =
        (ndsRendererTextureDataLayout(config) ==
         NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ? 3u : 0u;
    const NDSRendererTileState *render_tile1 = source1->render_tile;
    const u8 *indices0 = ndsRendererHardwareGetCi4Indices(
        texels0, source0_texels, byte_lane_xor, NULL);
    const u8 *indices1 = ndsRendererHardwareGetCi4Indices(
        source1->texels, source1->source_texels, byte_lane_xor, indices0);
    u32 x;
    u32 y;

    sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid = FALSE;
    (void)green_texels;
    (void)nonwhite_texels;
    /* Animated tile origins can wrap or mirror TEXEL1. Resolve those exact
     * addressing rules once per S coordinate, then reuse them for every row. */
    for (x = 0u; x < width; x++)
    {
        sNdsRendererHardwareTexel01Ci4Source0S[x] = (u8)(
            (materialize0_s != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    x, render_tile0->cms, render_tile0->masks) : x);
        sNdsRendererHardwareTexel01Ci4Source1S[x] = (u8)
            ndsRendererHardwareTextureAddressCoord(
                (s32)x + origin1_delta_s, render_tile1->width,
                source1->source_extent_width, render_tile1->cms,
                render_tile1->masks);
    }
    for (y = 0u; y < height; y++)
    {
        sNdsRendererHardwareTexel01Ci4Source0T[y] = (u8)(
            (materialize0_t != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    y, render_tile0->cmt, render_tile0->maskt) : y);
        sNdsRendererHardwareTexel01Ci4Source1T[y] = (u8)
            ndsRendererHardwareTextureAddressCoord(
                (s32)y + origin1_delta_t, render_tile1->height,
                source1->source_extent_height, render_tile1->cmt,
                render_tile1->maskt);
    }

    if ((indices0 != NULL) && (indices1 != NULL))
    {
        u32 texels = width * height;

        /* Large clamped/masked animated tiles revisit the same pair of source
         * coordinates and ordered-coverage phase many times. Index the first
         * exact representative of each separable S/T class; forward X expansion
         * reads only earlier representatives. Cold output copies repeated rows
         * in reverse Y order, while warm staging records their exact row map. */
        if (texels >= 4096u)
        {
            u32 unique_s = 0u;
            u32 unique_t = 0u;
            u32 unique_texels;

            unique_s = ndsRendererHardwareBuildCi4RepresentativeMap(
                sNdsRendererHardwareTexel01Ci4Source0S,
                sNdsRendererHardwareTexel01Ci4Source1S,
                sNdsRendererHardwareTexel01Ci4RepresentativeS, width);
            unique_t = ndsRendererHardwareBuildCi4RepresentativeMap(
                sNdsRendererHardwareTexel01Ci4Source0T,
                sNdsRendererHardwareTexel01Ci4Source1T,
                sNdsRendererHardwareTexel01Ci4RepresentativeT, height);

            unique_texels = unique_s * unique_t;
            if ((unique_texels * 2u) <= texels)
            {
                NDS_RENDERER_CAPACITY_HIGH_WATER(
                    gNdsRendererTextureRefreshCompactRequestBytesHighWater,
                    unique_t * upload_width * sizeof(u16));
                s32 compact_output =
                    (compact_row_map != NULL) &&
                    (compact_staged_bytes != NULL) &&
                    (width == upload_width) &&
                    (height <= NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) &&
                    (unique_t <=
                     NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS) &&
                    ((unique_t * upload_width * sizeof(u16)) <=
                     sizeof(sNdsRendererHardwareTextureRefreshLarge));
                u16 *destination = (compact_output != FALSE) ?
                    sNdsRendererHardwareTextureRefreshLarge :
                    sNdsRendererHardwareTextureScratch;
                u32 unique_row = 0u;

                /* Warm animated-water refreshes already upload a compact set
                 * of unique rows during VBlank.  Produce that exact compact
                 * representation here instead of expanding repeated rows into
                 * the 32 KiB scratch arena and immediately copying the unique
                 * rows back into the 16 KiB refresh staging buffer.  Cold
                 * uploads use the same loop with the full scratch destination. */
                for (y = 0u; y < height; y++)
                {
                    u32 representative_y =
                        sNdsRendererHardwareTexel01Ci4RepresentativeT[y];
                    u32 source0_row;
                    u32 source1_row;
                    u32 dst_index;

                    if (representative_y != y)
                    {
                        if (compact_output != FALSE)
                        {
                            compact_row_map[y] =
                                compact_row_map[representative_y];
                        }
                        continue;
                    }
                    if (compact_output != FALSE)
                    {
                        compact_row_map[y] = (u8)unique_row;
                    }
                    source0_row = ((source0_origin_t +
                        sNdsRendererHardwareTexel01Ci4Source0T[y]) *
                        source0_width) + source0_origin_s;
                    source1_row = ((source1->source_origin_t +
                        sNdsRendererHardwareTexel01Ci4Source1T[y]) *
                        source1->source_width) + source1->source_origin_s;
                    dst_index = ((compact_output != FALSE) ?
                        unique_row : y) * upload_width;

                    for (x = 0u; x < width; x++)
                    {
                        u32 index0;
                        u32 index1;
                        u32 representative_x =
                            sNdsRendererHardwareTexel01Ci4RepresentativeS[x];

                        if (representative_x != x)
                        {
                            destination[dst_index + x] =
                                destination[dst_index + representative_x];
                            continue;
                        }
                        index0 = indices0[source0_row +
                            sNdsRendererHardwareTexel01Ci4Source0S[x]];
                        index1 = indices1[source1_row +
                            sNdsRendererHardwareTexel01Ci4Source1S[x]];
                        destination[dst_index + x] =
                            ndsRendererHardwareResolveTexel01Ci4Lut(
                                index0, index1, x, y,
                                texel1_alpha_from_texel1);
                    }
                    unique_row++;
                }
                if (compact_output != FALSE)
                {
                    *compact_staged_bytes =
                        unique_row * upload_width * sizeof(u16);
                }
                else
                {
                    for (y = height; y != 0u; )
                    {
                        u32 representative_y;

                        y--;
                        representative_y =
                            sNdsRendererHardwareTexel01Ci4RepresentativeT[y];
                        if (representative_y != y)
                        {
                            memcpy(&sNdsRendererHardwareTextureScratch[
                                       y * upload_width],
                                   &sNdsRendererHardwareTextureScratch[
                                       representative_y * upload_width],
                                   width * sizeof(u16));
                        }
                    }
                }
                ndsRendererProfileRecordCi4RepresentativeReuse(
                    unique_texels, texels - unique_texels);
                sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid = TRUE;
                return compact_output;
            }
        }

        for (y = 0u; y < height; y++)
        {
            u32 source0_y = (materialize0_t != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    y, render_tile0->cmt, render_tile0->maskt) : y;
            u32 source1_y = ndsRendererHardwareTextureAddressCoord(
                (s32)y + origin1_delta_t, render_tile1->height,
                source1->source_extent_height, render_tile1->cmt,
                render_tile1->maskt);
            u32 source0_row =
                ((source0_origin_t + source0_y) * source0_width) +
                source0_origin_s;
            u32 source1_row =
                ((source1->source_origin_t + source1_y) *
                 source1->source_width) + source1->source_origin_s;
            u32 dst_index = y * upload_width;

            for (x = 0u; x < width; x++)
            {
                u32 index0 = indices0[source0_row +
                    sNdsRendererHardwareTexel01Ci4Source0S[x]];
                u32 index1 = indices1[source1_row +
                    sNdsRendererHardwareTexel01Ci4Source1S[x]];

                sNdsRendererHardwareTextureScratch[dst_index + x] =
                    ndsRendererHardwareResolveTexel01Ci4Lut(
                        index0, index1, x, y,
                        texel1_alpha_from_texel1);
            }
        }
        return FALSE;
    }
    for (y = 0u; y < height; y++)
    {
        u32 source0_y = (materialize0_t != FALSE) ?
            ndsRendererHardwareTextureMaskedAddress(
                y, render_tile0->cmt, render_tile0->maskt) : y;
        u32 source1_y = ndsRendererHardwareTextureAddressCoord(
            (s32)y + origin1_delta_t, render_tile1->height,
            source1->source_extent_height, render_tile1->cmt,
            render_tile1->maskt);
        u32 source0_row =
            ((source0_origin_t + source0_y) * source0_width) +
            source0_origin_s;
        u32 source1_row =
            ((source1->source_origin_t + source1_y) *
             source1->source_width) + source1->source_origin_s;
        u32 dst_index = y * upload_width;

        for (x = 0u; x < width; x++)
        {
            u32 source0_index = source0_row +
                sNdsRendererHardwareTexel01Ci4Source0S[x];
            u32 source1_index = source1_row +
                sNdsRendererHardwareTexel01Ci4Source1S[x];
            u32 index0 = ndsRendererHardwareReadCi4Direct(
                texels0, source0_index, byte_lane_xor);
            u32 index1 = ndsRendererHardwareReadCi4Direct(
                source1->texels, source1_index, byte_lane_xor);
            u16 color = ndsRendererHardwareResolveTexel01Ci4Lut(
                index0, index1, x, y, texel1_alpha_from_texel1);

            sNdsRendererHardwareTextureScratch[dst_index + x] = color;
        }
    }
    return FALSE;
}

/* THE REBIRTH-HALO BEAM'S EDGE, AND WHY IT GETS A TEXTURE OF ITS OWN.
 *
 * Its list (85.vpk0.bin 0x28a0) sets G_SETCOMBINE FCFFFFFF FFFDF2F9 -- rgb is
 * constant PRIMITIVE, alpha is pure TEXEL0 -- over the I4 tile declared at
 * 0x28b8 (fmt=I, siz=4b, 8x16, clamp on both axes) with PRIM white and opaque
 * at 0x28a8. So the source image carries COVERAGE and nothing else, in sixteen
 * levels, and the generic cache uploads every entry as GL_RGBA: RGB555 plus a
 * SINGLE alpha bit. ndsRendererHardwarePrimRgbTexel0Alpha therefore had to
 * threshold sixteen levels down to two, and the beam drew hard-edged where the
 * source fades.
 *
 * GL_RGB8_A5 is one byte a texel -- three index bits and FIVE alpha bits. The
 * sixteen source levels land in thirty-two slots injectively, so this recovers
 * the ramp exactly rather than approximating it, and the alpha stored here is
 * bit-identical to the 5-bit coverage the RGB555 bake already computed before
 * discarding it: (n * 0x11) >> 3 equals round(n * 31 / 15) at all sixteen
 * inputs. The combine can show one colour, so one palette holds PRIM.
 *
 * A DEDICATED NAME, NOT A PER-ENTRY CACHE FORMAT. The texture cache uploads one
 * format for all of its entries; teaching it per-entry formats to serve a single
 * 128-texel surface would put four just-closed rows at risk for a cosmetic one.
 * This costs one name and consumes no cache entry -- it RETURNS one to a pool
 * the post-KO scene already over-subscribes, and halves this surface's VRAM.
 *
 * The palette rides the texture NAME, not the bind. libnds documents
 * glColorTableEXT as setting the palette on the currently bound texture and
 * glDeleteTextures as deleting "associated palettes", and glAssignColorTable
 * exists precisely to share one between names. So it is attached once at
 * prepare, exactly as the cloud atlases and particle sheets already do, and the
 * bind path needs nothing. */
typedef struct NDSRendererPrimRgbTexel0AlphaFill
{
    const NDSRendererConfig *config;
    const u8 *texels;
    const NDSRendererTileState *render_tile;
    u32 source_width;
    u32 source_origin_s;
    u32 source_origin_t;
    u32 width;
    u32 height;
    u32 upload_width;
    u32 upload_height;
    u32 prim_env_blend_mode;
    s32 materialize_s;
    s32 materialize_t;
} NDSRendererPrimRgbTexel0AlphaFill;

NDS_RENDERER_COLD static s32 ndsRendererHardwarePrimRgbTexel0AlphaFill(
    u8 *pixels, u32 bytes, void *user_data)
{
    const NDSRendererPrimRgbTexel0AlphaFill *fill =
        (const NDSRendererPrimRgbTexel0AlphaFill *)user_data;
    u32 x;
    u32 y;

    if ((pixels == NULL) || (fill == NULL) || (fill->texels == NULL) ||
        (fill->upload_width == 0u) || (fill->upload_height == 0u) ||
        (fill->width > fill->upload_width) ||
        (fill->height > fill->upload_height) ||
        (bytes < (fill->upload_width * fill->upload_height)))
    {
        return FALSE;
    }
    /* The power-of-two tail lies outside the tile and must read as absent.
     * Left uncleared it would be palette entry 0 at full coverage -- an opaque
     * block of PRIM exactly where the old one-bit threshold used to put one. */
    memset(pixels, 0, bytes);
    for (y = 0u; y < fill->height; y++)
    {
        u32 source_y = (fill->materialize_t != FALSE) ?
            ndsRendererHardwareTextureMaskedAddress(
                y, fill->render_tile->cmt, fill->render_tile->maskt) : y;

        for (x = 0u; x < fill->width; x++)
        {
            u32 source_x = (fill->materialize_s != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    x, fill->render_tile->cms, fill->render_tile->masks) : x;
            u32 index =
                ((fill->source_origin_t + source_y) * fill->source_width) +
                fill->source_origin_s + source_x;
            u32 intensity = ndsRendererReadTexturePackedNibble(
                fill->config, fill->texels, index,
                NDS_RENDERER_HW_TEXTURE_FMT_I16,
                NDS_RENDERER_HW_TEXTURE_SIZ_4B);

            /* alpha5 in bits 3-7, palette index in bits 0-2. The beam is
             * flat PRIM, so every texel takes index 0. The cloud's colour is
             * the prim/env lerp at this coverage, quantised to eight palette
             * steps while alpha stays exact; PRIM alpha is not baked here --
             * it remains live polygon state. */
            if (fill->prim_env_blend_mode ==
                NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA)
            {
                pixels[(y * fill->upload_width) + x] =
                    (u8)((((intensity * 0x11u) >> 3) << 3) |
                         (intensity >> 1));
            }
            else
            {
                pixels[(y * fill->upload_width) + x] =
                    (u8)(((intensity * 0x11u) >> 3) << 3);
            }
        }
    }
    return TRUE;
}

static u32 ndsRendererHardwarePrimRgbTexel0AlphaExtentOf(u32 upload_width,
                                                         u32 upload_height)
{
    return (upload_width << 16) | (upload_height & 0xffffu);
}

static u32 ndsRendererHardwarePrimRgbTexel0AlphaSamplerOf(
    const NDSRendererTileState *render_tile, s32 materialize_s,
    s32 materialize_t)
{
    return (render_tile->cms & 0x3u) |
        ((render_tile->cmt & 0x3u) << 2) |
        ((render_tile->masks & 0xfu) << 4) |
        ((render_tile->maskt & 0xfu) << 8) |
        ((materialize_s != FALSE) ? (1u << 12) : 0u) |
        ((materialize_t != FALSE) ? (1u << 13) : 0u);
}

/* The Yoster cloud shares the beam's dedicated A5I3 name: one source image,
 * one upload extent, one primitive RGB, plus the combine mode and (for the
 * cloud, whose colour lerps prim/env) the environment RGB. All re-prepare on
 * any move. A5I3 is one byte a texel, so the upload extent already sizes VRAM;
 * no per-entry cache key exists for this name. The mode and env words live
 * beside the beam's four in nds_renderer_preamble.c, ahead of the discard
 * path that clears them. */

static s32 ndsRendererHardwarePrimRgbTexel0AlphaResident(
    const NDSRendererStats *stats, u32 primary_image,
    u32 source_width, u32 source_origin_s, u32 source_origin_t,
    const NDSRendererTileState *render_tile, s32 materialize_s,
    s32 materialize_t, u32 upload_width, u32 upload_height)
{
    u32 mode = ndsRendererHardwarePrimEnvTexel0BlendMode(stats);

    if ((mode != NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA) &&
        (mode != NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA))
    {
        return FALSE;
    }
    if ((sNdsRendererHardwarePrimRgbTexel0AlphaName == 0u) ||
        (sNdsRendererHardwarePrimRgbTexel0AlphaImage != primary_image) ||
        (sNdsRendererHardwarePrimRgbTexel0AlphaExtent !=
             ndsRendererHardwarePrimRgbTexel0AlphaExtentOf(
                 upload_width, upload_height)) ||
        (sNdsRendererHardwarePrimRgbTexel0AlphaPrim !=
             (stats->prim_color & 0xffffff00u)) ||
        (sNdsRendererHardwarePrimAlphaTexel0Mode != mode) ||
        (sNdsRendererPrimRgbTexel0AlphaSourceWidth != source_width) ||
        (sNdsRendererPrimRgbTexel0AlphaSourceOrigin !=
             ((source_origin_s << 16) | (source_origin_t & 0xffffu))) ||
        (sNdsRendererPrimRgbTexel0AlphaSampler !=
             ndsRendererHardwarePrimRgbTexel0AlphaSamplerOf(
                 render_tile, materialize_s, materialize_t)))
    {
        return FALSE;
    }
    /* The beam never reads ENV, so only the cloud keys on it. */
    if ((mode == NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA) &&
        (sNdsRendererHardwarePrimAlphaTexel0Env !=
             (stats->env_color & 0xffffff00u)))
    {
        return FALSE;
    }
    return TRUE;
}

static s32 ndsRendererHardwarePreparePrimRgbTexel0AlphaTexture(
    const NDSRendererStats *stats, const NDSRendererConfig *config,
    const u8 *texels_src, u32 primary_image, u32 source_width,
    u32 source_origin_s, u32 source_origin_t,
    const NDSRendererTileState *render_tile, s32 materialize_s,
    s32 materialize_t, u32 width, u32 height, u32 upload_width,
    u32 upload_height)
{
    NDSRendererPrimRgbTexel0AlphaFill fill;
    u32 prim = stats->prim_color & 0xffffff00u;
    u32 mode = ndsRendererHardwarePrimEnvTexel0BlendMode(stats);

    sNdsRendererPrimRgbTexel0AlphaGen++;
    u16 color = (u16)(((prim >> 27) & 0x1fu) |
                      (((prim >> 19) & 0x1fu) << 5) |
                      (((prim >> 11) & 0x1fu) << 10));
    u16 palette[8];
    u32 i;

    fill.config = config;
    fill.texels = texels_src;
    fill.render_tile = render_tile;
    fill.source_width = source_width;
    fill.source_origin_s = source_origin_s;
    fill.source_origin_t = source_origin_t;
    fill.width = width;
    fill.height = height;
    fill.upload_width = upload_width;
    fill.upload_height = upload_height;
    fill.prim_env_blend_mode = mode;
    fill.materialize_s = materialize_s;
    fill.materialize_t = materialize_t;
    if (mode == NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA)
    {
        /* The cloud's colour is (PRIM-ENV)*TEXEL0+ENV, so each palette step
         * holds that lerp at the midpoint intensity of its texel pair,
         * computed by the same helper the generic bake uses. Alpha stays
         * exact per texel; PRIM alpha stays out of the palette entirely. */
        for (i = 0u; i < 8u; i++)
        {
            u32 n = (i << 1) + 1u;
            u32 v = (n * 0x11u) >> 3;
            u16 texel0 = (u16)(0x8000u | v | (v << 5) | (v << 10));

            palette[i] =
                (u16)(ndsRendererHardwareBlendPrimEnvTexel0(
                          texel0, stats->prim_color,
                          stats->env_color) & 0x7fffu);
        }
    }
    else
    {
        /* Every index resolves to PRIM. The combine has no second colour, and the
         * fill only ever writes index 0, so the remaining seven exist to make a
         * stray index harmless rather than black. */
        for (i = 0u; i < 8u; i++)
        {
            palette[i] = color;
        }
    }
    if (ndsRendererHardwarePrepareIFCommonCloudAtlas(
            upload_width, upload_height, palette,
            ndsRendererHardwarePrimRgbTexel0AlphaFill, &fill,
            &sNdsRendererHardwarePrimRgbTexel0AlphaName) == FALSE)
    {
        sNdsRendererHardwarePrimRgbTexel0AlphaImage = 0u;
        sNdsRendererHardwarePrimRgbTexel0AlphaExtent = 0u;
        sNdsRendererHardwarePrimRgbTexel0AlphaPrim = 0u;
        sNdsRendererHardwarePrimAlphaTexel0Mode = 0u;
        sNdsRendererHardwarePrimAlphaTexel0Env = 0u;
        sNdsRendererPrimRgbTexel0AlphaSourceWidth = 0u;
        sNdsRendererPrimRgbTexel0AlphaSourceOrigin = 0u;
        sNdsRendererPrimRgbTexel0AlphaSampler = 0u;
        return FALSE;
    }
    sNdsRendererHardwarePrimRgbTexel0AlphaImage = primary_image;
    sNdsRendererHardwarePrimRgbTexel0AlphaExtent =
        ndsRendererHardwarePrimRgbTexel0AlphaExtentOf(upload_width,
                                                      upload_height);
    sNdsRendererHardwarePrimRgbTexel0AlphaPrim = prim;
    sNdsRendererHardwarePrimAlphaTexel0Mode = mode;
    sNdsRendererHardwarePrimAlphaTexel0Env = stats->env_color & 0xffffff00u;
    sNdsRendererPrimRgbTexel0AlphaSourceWidth = source_width;
    sNdsRendererPrimRgbTexel0AlphaSourceOrigin =
        (source_origin_s << 16) | (source_origin_t & 0xffffu);
    sNdsRendererPrimRgbTexel0AlphaSampler =
        ndsRendererHardwarePrimRgbTexel0AlphaSamplerOf(
            render_tile, materialize_s, materialize_t);
    if ((mode == NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA) &&
        ((materialize_s != FALSE) || (materialize_t != FALSE)))
    {
        gNdsRendererPrimEnvMaskedBakeCount++;
    }
    gNdsRendererPrimRgbTexel0AlphaPrepareCount++;
    return TRUE;
}

static void ndsRendererHardwareBindPrimRgbTexel0AlphaTexture(
    NDSRendererStats *stats, u32 params, u32 format, u32 width, u32 height)
{
    const u32 merged = ndsRendererHardwareMergeTextureParams(params);

    ndsRendererHardwareBindTextureName(
        stats, sNdsRendererHardwarePrimRgbTexel0AlphaName);
    ndsRendererHardwareApplyTextureParams(merged);
    sNdsRendererPrimRgbTexel0AlphaLastParams = merged;
    sNdsRendererPrimRgbTexel0AlphaLastFormat = format;
    sNdsRendererPrimRgbTexel0AlphaLastWidth = width;
    sNdsRendererPrimRgbTexel0AlphaLastHeight = height;
    /* Not a cache entry, so nothing may be left pointing into the cache. */
    sNdsRendererHardwareActiveTextureEntry = NULL;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = format;
    stats->hardware_texture_width = width;
    stats->hardware_texture_height = height;
    gNdsRendererPrimRgbTexel0AlphaBindCount++;
}

/* P2-2p8 Phase 1 slice 2c: while the fighter admission runs, a new cache entry
 * is carved (ndsFtrCarveUpload, with the admission below) instead of taking a
 * libnds name and glTexImage2D. */
static u32 sNdsFtrCarveActive;
#if defined(NDS_LAB_TEXMISS) && NDS_LAB_TEXMISS
#include <nds/arm9/cache.h>
/* LAB ONLY: the over-gate texture census ring (frame, image, tlut,
 * format|size|flags, width|height, combine words, key hash, texel1 fraction,
 * spare) -- see the battle fallback in ResolveOrBindTexture. */
#define NDS_LAB_TEXMISS_MAX 1024u
__attribute__((used)) u32 gNdsLabTexMiss[NDS_LAB_TEXMISS_MAX][10];
__attribute__((used)) volatile u32 gNdsLabTexMissCount;
#endif
static s32 ndsFtrCarveUpload(NDSRendererHardwareTextureCacheEntry *entry,
                             u32 type, u32 size_x, u32 size_y, u32 params,
                             const void *texels, u32 palette_entries,
                             const u16 *palette);
#if NDS_VRAM_CENSUS_LIVE
/* Lab: the admission census's key probe -- the resolve path copies the key
 * it built here and returns before the lookup, conversion and upload. */
static NDSRendererHardwareTextureKey *sNdsFtrAdmitKeyProbe;
#endif
