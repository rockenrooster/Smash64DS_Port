void lbCommonClearExternSpriteParams(void)
{
}

void lbCommonSetExternSpriteParams(Sprite *sprite)
{
    (void)sprite;
}

void lbCommonPrepSObjAttr(Gfx **dls, SObj *sobj)
{
    (void)dls;
    (void)sobj;
}

void lbCommonPrepSObjDraw(Gfx **dls, SObj *sobj)
{
    (void)dls;
    (void)sobj;
}

extern void ndsIFCommonRecordHUDState(void);

DObj *lbCommonGetTreeDObjNextFromRoot(DObj *dobj, DObj *root)
{
    if (dobj->child != NULL)
    {
        dobj = dobj->child;
    }
    else if (dobj == root)
    {
        dobj = NULL;
    }
    else if (dobj->sib_next != NULL)
    {
        dobj = dobj->sib_next;
    }
    else while (TRUE)
    {
        if (dobj->parent == root)
        {
            dobj = NULL;
            break;
        }
        else if (dobj->parent->sib_next != NULL)
        {
            dobj = dobj->parent->sib_next;
            break;
        }
        else
        {
            dobj = dobj->parent;
        }
    }
    return dobj;
}

/* Narrow lbcommon startup shim.
 *
 * The full original lb/lbcommon.c translation unit fans out into the fighter
 * part tree, camera look-at helpers, and the N64 sprite display-list pipeline.
 * Startup only needs to create one SObj and retain the display callback
 * identity, so keep this bounded here until the renderer slice is ready. */
SObj *lbCommonMakeSObjForGObj(GObj *gobj, Sprite *sprite)
{
    SObj *sobj = gcAddSObjForGObj(gobj, sprite);

    sobj->envcolor.r = 0;
    sobj->envcolor.g = 0;
    sobj->envcolor.b = 0;
    sobj->envcolor.a = 0;
    sobj->maskt = 0;
    sobj->masks = 0;
    sobj->cmt = 2;
    sobj->cms = 2;
    sobj->pos.x = 0.0F;
    sobj->pos.y = 0.0F;

    return sobj;
}

GObj *lbCommonMakeSpriteGObj(u32 id, void (*func_run)(GObj *), s32 link,
                             u32 link_priority,
                             void (*proc_display)(GObj *), s32 dl_link,
                             u32 dl_link_priority, u32 camera_tag,
                             Sprite *sprite, u8 gobjproc_kind,
                             void (*proc)(GObj *), u32 gobjproc_priority)
{
    GObj *gobj = gcMakeGObjSPAfter(id, func_run, link, link_priority);

    if (gobj == NULL)
    {
        return NULL;
    }
    lbCommonMakeSObjForGObj(gobj, sprite);
    if (proc_display != NULL)
    {
        gcAddGObjDisplay(gobj, proc_display, dl_link, dl_link_priority,
                         camera_tag);
    }
    if (proc != NULL)
    {
        gcAddGObjProcess(gobj, proc, gobjproc_kind, gobjproc_priority);
    }
    return gobj;
}

static u16 ndsStartupLogoConvertRgba16(u16 n64_color)
{
    u16 red;
    u16 green;
    u16 blue;

    if ((n64_color & 1u) == 0)
    {
        return 0;
    }

    red = (u16)((n64_color >> 11) & 0x1fu);
    green = (u16)((n64_color >> 6) & 0x1fu);
    blue = (u16)((n64_color >> 1) & 0x1fu);
    return (u16)((1u << 15) | red | (green << 5) | (blue << 10));
}

static u16 ndsSpritePackRgb15(u8 red, u8 green, u8 blue)
{
    return (u16)((1u << 15) | ((u16)(red >> 3)) |
                 ((u16)(green >> 3) << 5) |
                 ((u16)(blue >> 3) << 10));
}

static u16 ndsSpriteLerpPrimEnv(const SObj *sobj, u8 intensity)
{
    u32 inverse = 255u - intensity;
    u8 red = (u8)(((u32)sobj->sprite.red * intensity +
                   (u32)sobj->envcolor.r * inverse + 127u) / 255u);
    u8 green = (u8)(((u32)sobj->sprite.green * intensity +
                     (u32)sobj->envcolor.g * inverse + 127u) / 255u);
    u8 blue = (u8)(((u32)sobj->sprite.blue * intensity +
                    (u32)sobj->envcolor.b * inverse + 127u) / 255u);

    return ndsSpritePackRgb15(red, green, blue);
}

static u16 ndsSpriteConvertRgba32(u32 rgba)
{
    u8 red = (u8)(rgba >> 24);
    u8 green = (u8)(rgba >> 16);
    u8 blue = (u8)(rgba >> 8);
    u8 alpha = (u8)rgba;

    if (alpha == 0)
    {
        return 0;
    }
    return (u16)((1u << 15) |
                 ((u16)(red >> 3)) |
                 ((u16)(green >> 3) << 5) |
                 ((u16)(blue >> 3) << 10));
}

static u16 ndsStartupLogoReadRgba16Pixel(const u16 *pixels, u32 width,
                                         u32 row, u32 column,
                                         u32 is_texshuf)
{
    u32 index;

    if ((is_texshuf != 0) && ((row & 1u) != 0))
    {
        /*
         * SP_TEXSHUF sprite strips are stored in DRAM with the N64 TMEM
         * odd-row bank-conflict swizzle. Hardware undoes this while sampling;
         * the DS diagnostic preview has to apply the same inverse address map.
         */
        u32 swizzled_column = column ^ 2u;

        if (swizzled_column < width)
        {
            column = swizzled_column;
        }
    }

    index = (row * width) + column;

    /* The O2R loader converts each big-endian 32-bit word to native order.
     * RGBA16 texture halfwords are therefore correct but swapped in pairs. */
    return pixels[index ^ 1u];
}

static void ndsRecordSObjDrawBlocker(u32 record_startup, u32 blocker)
{
    if (record_startup != 0)
    {
        gNdsStartupLogoDrawBlocker = blocker;
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindOpeningPortraits) &&
        ((blocker == NDS_STARTUP_LOGO_BLOCKER_NONE) ||
         (gNdsOpeningPortraitsDrawResult != NDS_OPENING_PORTRAITS_DRAW_PASS)))
    {
        gNdsOpeningPortraitsDrawBlocker = blocker;
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindOpeningMario) &&
        ((blocker == NDS_STARTUP_LOGO_BLOCKER_NONE) ||
         (gNdsOpeningMarioDrawResult != NDS_OPENING_MARIO_DRAW_PASS)))
    {
        gNdsOpeningMarioDrawBlocker = blocker;
    }
    if ((ndsOpeningIsImportedNameScene(gSCManagerSceneData.scene_curr) !=
         FALSE) &&
        ((blocker == NDS_STARTUP_LOGO_BLOCKER_NONE) ||
         (gNdsOpeningNameSceneDrawResult != NDS_OPENING_NAME_DRAW_PASS)))
    {
        gNdsOpeningNameSceneDrawBlocker = blocker;
    }
}

static s32 ndsSObjPreviewBasicSupported(SObj *sobj)
{
    Sprite *sprite;
    u32 width;
    u32 height;
    u32 bitmap_count;

    if (sobj == NULL)
    {
        return FALSE;
    }

    sprite = &sobj->sprite;
    width = (u32)(u16)sprite->width;
    height = (u32)(u16)sprite->height;
    bitmap_count = (u32)(u16)sprite->nbitmaps;

    return ((((sprite->bmfmt == G_IM_FMT_RGBA) &&
              (sprite->bmsiz == G_IM_SIZ_16b)) ||
             ((sprite->bmfmt == G_IM_FMT_RGBA) &&
              (sprite->bmsiz == G_IM_SIZ_32b)) ||
             ((sprite->bmfmt == G_IM_FMT_IA) &&
              (sprite->bmsiz == G_IM_SIZ_8b)) ||
             ((sprite->bmfmt == G_IM_FMT_CI) &&
              (sprite->bmsiz == G_IM_SIZ_8b)) ||
             ((sprite->bmfmt == G_IM_FMT_I) &&
              (sprite->bmsiz == G_IM_SIZ_4b))) &&
            (width != 0) && (height != 0) &&
            (width <= 320u) &&
            (height <= NDS_OPENING_ACTION_PREVIEW_MAX_HEIGHT) &&
            (bitmap_count != 0) && (bitmap_count <= 128u)) ? TRUE : FALSE;
}

typedef struct NDSSObjWallpaperDecodeCache
{
    u32 valid;
    u32 asset_id;
    u32 owner_scene;
    u32 owner_generation;
    const void *loaded_data;
    u32 bitmap_offset;
    u32 platform_epoch;
    u32 layout_fingerprint;
    u32 width;
    u32 height;
    u32 bitmap_count;
    u32 bmheight;
    u32 bmHreal;
    u32 texshuf;
    u32 source_drawn_pixels;
    u32 opaque_pixels;
} NDSSObjWallpaperDecodeCache;

static NDSSObjWallpaperDecodeCache sNdsSObjWallpaperDecodeCache;

volatile u32 gNdsSObjWallpaperCacheBuildCount;
volatile u32 gNdsSObjWallpaperCacheHitCount;
volatile u32 gNdsSObjWallpaperCacheFastDrawCount;
volatile u32 gNdsSObjWallpaperCacheFallbackCount;
volatile u32 gNdsSObjWallpaperCacheWidth;
volatile u32 gNdsSObjWallpaperCacheHeight;
volatile u32 gNdsSObjWallpaperCacheOpaquePixels;
volatile u32 gNdsSObjWallpaperCacheBuildTicks;
volatile u32 gNdsSObjWallpaperCacheDrawTicks;

static u32 ndsSObjWallpaperCacheMix(u32 hash, u32 value)
{
    hash ^= value + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    return hash;
}

static u32 ndsSObjWallpaperLayoutFingerprint(const NDSRelocLoadedFile *loaded,
                                              const Bitmap *bitmap,
                                              u32 bitmap_count)
{
    u32 hash = 0x57414c4cu;
    u32 i;

    if ((loaded == NULL) || (bitmap == NULL))
    {
        return 0u;
    }
    for (i = 0u; i < bitmap_count; i++)
    {
        const Bitmap *current = &bitmap[i];
        uintptr_t buffer = (uintptr_t)current->buf;
        uintptr_t base = (uintptr_t)loaded->data;

        hash = ndsSObjWallpaperCacheMix(
            hash, ((u32)(u16)current->width << 16) |
                      (u32)(u16)current->width_img);
        hash = ndsSObjWallpaperCacheMix(
            hash, ((u32)(u16)current->s << 16) | (u32)(u16)current->t);
        hash = ndsSObjWallpaperCacheMix(
            hash, ((u32)(u16)current->actualHeight << 16) |
                      (u32)(u16)current->LUToffset);
        hash = ndsSObjWallpaperCacheMix(
            hash, (buffer >= base) ? (u32)(buffer - base) : (u32)buffer);
    }
    return hash;
}

static s32 ndsSObjWallpaperCacheKeyMatches(
    const NDSRelocLoadedFile *loaded, const Sprite *sprite,
    u32 platform_epoch, u32 layout_fingerprint)
{
    const NDSSObjWallpaperDecodeCache *cache =
        &sNdsSObjWallpaperDecodeCache;

    return ((cache->valid != 0u) &&
            (loaded != NULL) &&
            (cache->asset_id == loaded->asset_id) &&
            (cache->owner_scene == loaded->owner_scene) &&
            (cache->owner_generation == loaded->owner_generation) &&
            (cache->loaded_data == loaded->data) &&
            (cache->bitmap_offset ==
             (u32)((uintptr_t)sprite->bitmap - (uintptr_t)loaded->data)) &&
            (cache->platform_epoch == platform_epoch) &&
            (cache->layout_fingerprint == layout_fingerprint) &&
            (cache->width == (u32)(u16)sprite->width) &&
            (cache->height == (u32)(u16)sprite->height) &&
            (cache->bitmap_count == (u32)(u16)sprite->nbitmaps) &&
            (cache->bmheight == (u32)(u16)sprite->bmheight) &&
            (cache->bmHreal == (u32)(u16)sprite->bmHreal) &&
            (cache->texshuf ==
             (((sprite->attr & SP_TEXSHUF) != 0u) ? 1u : 0u))) ? TRUE :
                                                                    FALSE;
}

static s32 ndsSObjBuildWallpaperDecodeCache(
    const NDSRelocLoadedFile *loaded, const Sprite *sprite,
    u16 *cache_pixels, u32 cache_pitch, u32 cache_height,
    u32 platform_epoch, u32 layout_fingerprint)
{
    const Bitmap *bitmap = sprite->bitmap;
    u32 width = (u32)(u16)sprite->width;
    u32 height = (u32)(u16)sprite->height;
    u32 bitmap_count = (u32)(u16)sprite->nbitmaps;
    u32 is_texshuf = ((sprite->attr & SP_TEXSHUF) != 0u) ? 1u : 0u;
    u32 out_y = 0u;
    u32 drawn_pixels = 0u;
    u32 opaque_pixels = 0u;
    u32 bitmap_index;
    u32 row;
    u32 build_start = cpuGetTiming();

    if ((loaded == NULL) || (cache_pixels == NULL) ||
        (cache_pitch < width) || (cache_height < height))
    {
        return FALSE;
    }
    sNdsSObjWallpaperDecodeCache.valid = FALSE;
    for (row = 0u; row < height; row++)
    {
        memset(&cache_pixels[row * cache_pitch], 0,
               width * sizeof(cache_pixels[0]));
    }
    for (bitmap_index = 0u;
         (bitmap_index < bitmap_count) && (out_y < height);
         bitmap_index++)
    {
        const Bitmap *current = &bitmap[bitmap_index];
        const u16 *src = current->buf;
        u32 src_width = (u32)(u16)current->width_img;
        u32 src_draw_width = (u32)(u16)current->width;
        u32 src_height = (u32)(u16)current->actualHeight;
        u32 row_advance = (u32)(u16)sprite->bmheight;
        size_t src_bytes;

        if (src_draw_width == 0u)
        {
            break;
        }
        if (src_width == 0u) { src_width = src_draw_width; }
        if (src_height == 0u) { src_height = row_advance; }
        if (row_advance == 0u) { row_advance = src_height; }
        if ((src_width == 0u) || (src_height == 0u))
        {
            continue;
        }
        if (src_draw_width > width) { src_draw_width = width; }
        src_bytes = (size_t)src_width * src_height * sizeof(u16);
        if (ndsRelocPointerRangeInLoadedFile(loaded, src, src_bytes) == FALSE)
        {
            return FALSE;
        }
        for (row = 0u;
             (row < src_height) && ((out_y + row) < height);
             row++)
        {
            u32 x;
            u16 *dst = &cache_pixels[(out_y + row) * cache_pitch];

            for (x = 0u; x < src_draw_width; x++)
            {
                u16 color = ndsStartupLogoConvertRgba16(
                    ndsStartupLogoReadRgba16Pixel(
                        src, src_width, row, x, is_texshuf));

                /* Transparent later strips do not erase earlier overlap rows
                 * in the source sprite pipeline. */
                if (color != 0u)
                {
                    dst[x] = color;
                    drawn_pixels++;
                }
            }
        }
        out_y += row_advance;
    }
    if (drawn_pixels == 0u)
    {
        return FALSE;
    }
    for (row = 0u; row < height; row++)
    {
        u32 x;
        const u16 *src = &cache_pixels[row * cache_pitch];

        for (x = 0u; x < width; x++)
        {
            if (src[x] != 0u) { opaque_pixels++; }
        }
    }

    sNdsSObjWallpaperDecodeCache.asset_id = loaded->asset_id;
    sNdsSObjWallpaperDecodeCache.owner_scene = loaded->owner_scene;
    sNdsSObjWallpaperDecodeCache.owner_generation = loaded->owner_generation;
    sNdsSObjWallpaperDecodeCache.loaded_data = loaded->data;
    sNdsSObjWallpaperDecodeCache.bitmap_offset =
        (u32)((uintptr_t)sprite->bitmap - (uintptr_t)loaded->data);
    sNdsSObjWallpaperDecodeCache.platform_epoch = platform_epoch;
    sNdsSObjWallpaperDecodeCache.layout_fingerprint = layout_fingerprint;
    sNdsSObjWallpaperDecodeCache.width = width;
    sNdsSObjWallpaperDecodeCache.height = height;
    sNdsSObjWallpaperDecodeCache.bitmap_count = bitmap_count;
    sNdsSObjWallpaperDecodeCache.bmheight = (u32)(u16)sprite->bmheight;
    sNdsSObjWallpaperDecodeCache.bmHreal = (u32)(u16)sprite->bmHreal;
    sNdsSObjWallpaperDecodeCache.texshuf = is_texshuf;
    sNdsSObjWallpaperDecodeCache.source_drawn_pixels = drawn_pixels;
    sNdsSObjWallpaperDecodeCache.opaque_pixels = opaque_pixels;
    gNdsSObjWallpaperCacheBuildCount++;
    gNdsSObjWallpaperCacheWidth = width;
    gNdsSObjWallpaperCacheHeight = height;
    gNdsSObjWallpaperCacheOpaquePixels = opaque_pixels;
    gNdsSObjWallpaperCacheBuildTicks += cpuGetTiming() - build_start;
    sNdsSObjWallpaperDecodeCache.valid = TRUE;
    return TRUE;
}

static u32 ndsSObjWallpaperLastSource(u32 relative, u32 scale_q16)
{
    /* The 320x240 clipped viewport keeps this numerator within u32 and avoids
     * an ARM9 software 64-bit divide on every source-map entry. */
    return ((((relative + 1u) << 16) - 1u) / scale_q16);
}

static s32 ndsSObjDrawOpaqueWallpaperCache(
    const u16 *cache_pixels, u32 cache_pitch,
    u16 *source_x_map,
    u32 width, u32 height, u32 scale_x_q16, u32 scale_y_q16,
    u16 *preview, u32 preview_pitch, u32 preview_width, u32 preview_height,
    s32 origin_x, s32 origin_y)
{
    s32 dst_x_start = origin_x;
    s32 dst_y_start = origin_y;
    s32 dst_x_end = origin_x +
        (s32)((((u64)width * scale_x_q16) + 0xffffu) >> 16);
    s32 dst_y_end = origin_y +
        (s32)((((u64)height * scale_y_q16) + 0xffffu) >> 16);
    s32 dst_x;
    s32 dst_y;

    if (dst_x_start < 0) { dst_x_start = 0; }
    if (dst_y_start < 0) { dst_y_start = 0; }
    if (dst_x_end > (s32)preview_width) { dst_x_end = preview_width; }
    if (dst_y_end > (s32)preview_height) { dst_y_end = preview_height; }
    if (dst_y_start >= dst_y_end)
    {
        return FALSE;
    }
    for (dst_x = dst_x_start; dst_x < dst_x_end; dst_x++)
    {
        u32 relative = (u32)(dst_x - origin_x);
        u32 source_x = ndsSObjWallpaperLastSource(relative, scale_x_q16);

        if (source_x >= width) { source_x = width - 1u; }
        source_x_map[dst_x] = (u16)source_x;
    }
    for (dst_y = dst_y_start; dst_y < dst_y_end; dst_y++)
    {
        u32 relative = (u32)(dst_y - origin_y);
        u32 source_y = ndsSObjWallpaperLastSource(relative, scale_y_q16);
        const u16 *src;
        u16 *dst;

        if (source_y >= height) { source_y = height - 1u; }
        src = &cache_pixels[source_y * cache_pitch];
        dst = &preview[(u32)dst_y * preview_pitch];
        for (dst_x = dst_x_start; dst_x < dst_x_end; dst_x++)
        {
            dst[dst_x] = src[source_x_map[dst_x]];
        }
    }
    return TRUE;
}

static u32 ndsSObjDrawCachedWallpaper(
    const NDSRelocLoadedFile *loaded, const Sprite *sprite,
    u16 *preview, u32 preview_pitch, u32 preview_width, u32 preview_height,
    s32 origin_x, s32 origin_y, u32 scale_x_q16, u32 scale_y_q16)
{
    u16 *cache_pixels;
    u32 cache_pitch = 0u;
    u32 cache_height = 0u;
    u32 platform_epoch = 0u;
    u32 layout_fingerprint;
    u32 draw_start;

    cache_pixels = ndsPlatformGetOriginalSpriteDecodeCache(
        &cache_pitch, &cache_height, &platform_epoch);
    if ((cache_pixels == NULL) || (loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_STAGE_DREAM_LAND) ||
        ((u32)(u16)sprite->width != 300u) ||
        ((u32)(u16)sprite->height != 220u) ||
        ((u32)(u16)sprite->nbitmaps != 44u) ||
        ((u32)(u16)sprite->bmheight != 5u) ||
        ((u32)(u16)sprite->bmHreal != 6u) ||
        (sprite->bmfmt != G_IM_FMT_RGBA) ||
        (sprite->bmsiz != G_IM_SIZ_16b))
    {
        return 0u;
    }
    layout_fingerprint = ndsSObjWallpaperLayoutFingerprint(
        loaded, sprite->bitmap, (u32)(u16)sprite->nbitmaps);
    if (ndsSObjWallpaperCacheKeyMatches(
            loaded, sprite, platform_epoch, layout_fingerprint) == FALSE)
    {
        if (ndsSObjBuildWallpaperDecodeCache(
                loaded, sprite, cache_pixels, cache_pitch, cache_height,
                platform_epoch, layout_fingerprint) == FALSE)
        {
            sNdsSObjWallpaperDecodeCache.valid = FALSE;
            return 0u;
        }
    }
    else
    {
        gNdsSObjWallpaperCacheHitCount++;
    }

    /* The destination-driven last-writer mapping is exact only after proving
     * this source is fully opaque. Any layout or future camera-scale change
     * outside that contract returns to the unchanged generic compositor. */
    if ((sNdsSObjWallpaperDecodeCache.opaque_pixels !=
         sNdsSObjWallpaperDecodeCache.width *
             sNdsSObjWallpaperDecodeCache.height) ||
        (scale_x_q16 < (1u << 16)) ||
        (scale_y_q16 < (1u << 16)) ||
        (cache_height <= sNdsSObjWallpaperDecodeCache.height) ||
        (((cache_height - sNdsSObjWallpaperDecodeCache.height) *
          cache_pitch) < preview_width))
    {
        return 0u;
    }

    draw_start = cpuGetTiming();
    {
        u16 *source_x_map = &cache_pixels[
            sNdsSObjWallpaperDecodeCache.height * cache_pitch];

        if (ndsSObjDrawOpaqueWallpaperCache(
                cache_pixels, cache_pitch, source_x_map,
                sNdsSObjWallpaperDecodeCache.width,
                sNdsSObjWallpaperDecodeCache.height,
                scale_x_q16, scale_y_q16, preview, preview_pitch,
                preview_width, preview_height, origin_x, origin_y) == FALSE)
        {
            gNdsSObjWallpaperCacheDrawTicks = cpuGetTiming() - draw_start;
            return 0u;
        }
        gNdsSObjWallpaperCacheFastDrawCount++;
    }
    gNdsSObjWallpaperCacheDrawTicks = cpuGetTiming() - draw_start;
    return sNdsSObjWallpaperDecodeCache.source_drawn_pixels;
}

static s32 ndsDrawSObjIntoPreview(SObj *sobj, u32 record_startup,
                                  u16 *preview, u32 preview_pitch,
                                  u32 preview_width, u32 preview_height,
                                  s32 origin_x, s32 origin_y,
                                  u32 results_wallpaper_combine,
                                  u32 cache_wallpaper)
{
    Sprite *sprite;
    Bitmap *bitmap;
    NDSRelocLoadedFile *loaded;
    u32 width;
    u32 height;
    u32 bitmap_count;
    u32 bitmap_index;
    u32 out_y = 0;
    u32 drawn_pixels = 0;
    u32 is_texshuf;
    u32 is_scaled;
    u32 scale_x_q16;
    u32 scale_y_q16;

    if (sobj == NULL)
    {
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_NO_SOBJ);
        return FALSE;
    }

    sprite = &sobj->sprite;
    if (record_startup != 0)
    {
        gNdsStartupLogoDrawWidth = (u32)(u16)sprite->width;
        gNdsStartupLogoDrawHeight = (u32)(u16)sprite->height;
        gNdsStartupLogoDrawFormat = sprite->bmfmt;
        gNdsStartupLogoDrawSize = sprite->bmsiz;
        gNdsStartupLogoDrawBitmaps = (u32)(u16)sprite->nbitmaps;
    }

    if (!(((sprite->bmfmt == G_IM_FMT_RGBA) &&
           (sprite->bmsiz == G_IM_SIZ_16b)) ||
          ((sprite->bmfmt == G_IM_FMT_RGBA) &&
           (sprite->bmsiz == G_IM_SIZ_32b)) ||
          ((sprite->bmfmt == G_IM_FMT_IA) &&
           (sprite->bmsiz == G_IM_SIZ_8b)) ||
          ((sprite->bmfmt == G_IM_FMT_CI) &&
           (sprite->bmsiz == G_IM_SIZ_8b)) ||
          ((sprite->bmfmt == G_IM_FMT_I) &&
           (sprite->bmsiz == G_IM_SIZ_4b))))
    {
        ndsRecordSObjDrawBlocker(
            record_startup, NDS_STARTUP_LOGO_BLOCKER_UNSUPPORTED_FORMAT);
        return FALSE;
    }

    width = (u32)(u16)sprite->width;
    height = (u32)(u16)sprite->height;
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    is_texshuf = ((sprite->attr & SP_TEXSHUF) != 0) ? 1u : 0u;
    if ((sprite->scalex < 0.0001F) || (sprite->scaley < 0.0001F))
    {
        return FALSE;
    }
    if ((sprite->attr & SP_FASTCOPY) != 0)
    {
        scale_x_q16 = 1u << 16;
        scale_y_q16 = 1u << 16;
    }
    else
    {
        scale_x_q16 = (u32)((sprite->scalex * 65536.0F) + 0.5F);
        scale_y_q16 = (u32)((sprite->scaley * 65536.0F) + 0.5F);
    }
    is_scaled = ((scale_x_q16 != (1u << 16)) ||
                 (scale_y_q16 != (1u << 16))) ? TRUE : FALSE;
    if (record_startup != 0)
    {
        gNdsStartupLogoDrawTexshuf = is_texshuf;
    }

    if ((width == 0) || (height == 0) ||
        (width > 320u) ||
        (height > NDS_OPENING_ACTION_PREVIEW_MAX_HEIGHT) ||
        (bitmap_count == 0) || (bitmap_count > 128u))
    {
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_BAD_DIMENSIONS);
        return FALSE;
    }

    bitmap = sprite->bitmap;
    loaded = ndsRelocFindLoadedFileContaining(
        bitmap,
        sizeof(Bitmap) * bitmap_count);
    if (ndsRelocPointerRangeInLoadedFile(loaded, bitmap,
                                         sizeof(Bitmap) * bitmap_count) == FALSE)
    {
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_BAD_BITMAP_TABLE);
        return FALSE;
    }

    if ((preview == NULL) || (preview_pitch == 0) ||
        (preview_width == 0) || (preview_height == 0))
    {
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_NO_PREVIEW_BUFFER);
        return FALSE;
    }

    if (cache_wallpaper != 0u)
    {
        drawn_pixels = ndsSObjDrawCachedWallpaper(
            loaded, sprite, preview, preview_pitch, preview_width,
            preview_height, origin_x, origin_y, scale_x_q16, scale_y_q16);
        if (drawn_pixels != 0u)
        {
            goto draw_complete;
        }
        gNdsSObjWallpaperCacheFallbackCount++;
    }

    for (bitmap_index = 0;
         (bitmap_index < bitmap_count) && (out_y < height);
         bitmap_index++)
    {
        Bitmap *current = &bitmap[bitmap_index];
        const u16 *src = current->buf;
        u32 src_width = (u32)(u16)current->width_img;
        u32 src_draw_width = (u32)(u16)current->width;
        u32 src_height = (u32)(u16)current->actualHeight;
        u32 row_advance = (u32)(u16)sprite->bmheight;
        u32 draw_y;
        u32 draw_rows;
        u32 row;
        size_t src_bytes;
        size_t src_row_bytes;
        u32 bytes_per_pixel = 1u;
        u32 ci_palette_ready = 0;
        u32 ci_max_index = 0;

        if (src_draw_width == 0)
        {
            break;
        }
        if (src_width == 0)
        {
            src_width = src_draw_width;
        }
        if (src_height == 0)
        {
            src_height = row_advance;
        }
        if (row_advance == 0)
        {
            row_advance = src_height;
        }
        if ((src_width == 0) || (src_height == 0))
        {
            continue;
        }
        if (src_draw_width > width)
        {
            src_draw_width = width;
        }

        if ((sprite->bmfmt == G_IM_FMT_RGBA) &&
            (sprite->bmsiz == G_IM_SIZ_16b))
        {
            bytes_per_pixel = 2u;
        }
        else if ((sprite->bmfmt == G_IM_FMT_RGBA) &&
                 (sprite->bmsiz == G_IM_SIZ_32b))
        {
            bytes_per_pixel = 4u;
        }

        if ((sprite->bmfmt == G_IM_FMT_I) &&
            (sprite->bmsiz == G_IM_SIZ_4b))
        {
            src_row_bytes = ((size_t)src_width + 1u) / 2u;
        }
        else
        {
            src_row_bytes = (size_t)src_width * bytes_per_pixel;
        }
        src_bytes = src_row_bytes * src_height;
        if (ndsRelocPointerRangeInLoadedFile(loaded, src, src_bytes) == FALSE)
        {
            if (record_startup != 0)
            {
                gNdsStartupLogoDrawBlocker =
                    NDS_STARTUP_LOGO_BLOCKER_BAD_BITMAP_BUFFER;
            }
            return FALSE;
        }
        if ((sprite->bmfmt == G_IM_FMT_CI) &&
            (sprite->bmsiz == G_IM_SIZ_8b))
        {
            const u8 *src_ci = (const u8 *)src;
            const u16 *palette = (const u16 *)sprite->LUT;
            size_t i;

            for (i = 0; i < src_bytes; i++)
            {
                if ((u32)src_ci[i] > ci_max_index)
                {
                    ci_max_index = src_ci[i];
                }
            }
            if ((palette != NULL) &&
                (ndsRelocPointerRangeInLoadedFile(
                    loaded, palette,
                    ((size_t)ci_max_index + 1u +
                     (((ci_max_index & 1u) == 0) ? 1u : 0u)) *
                    sizeof(u16)) != FALSE))
            {
                ci_palette_ready = 1u;
            }
            else
            {
                return FALSE;
            }
        }

        /* libultra draws the bitmap's real height, then advances by
         * sprite->bmheight. N64Logo uses 15-pixel strips with a 14-pixel
         * advance and SP_OVERLAP, so dropping the overlap row makes the
         * retained preview look coarse. */
        draw_y = out_y;
        draw_rows = src_height;

        for (row = 0; (row < draw_rows) && ((draw_y + row) < height); row++)
        {
            u32 x;
            u32 dst_x_q16 = 0u;
            u32 source_y = draw_y + row;
            s32 dst_y_start = origin_y +
                (s32)(((u64)source_y * scale_y_q16) >> 16);
            s32 dst_y_end = origin_y +
                (s32)((((u64)(source_y + 1u) * scale_y_q16) +
                       0xffffu) >> 16);

            if ((dst_y_end <= 0) ||
                (dst_y_start >= (s32)preview_height))
            {
                continue;
            }
            for (x = 0; x < src_draw_width; x++)
            {
                s32 dst_x_start = origin_x + (s32)(dst_x_q16 >> 16);
                s32 dst_x_end;
                u16 color;

                dst_x_q16 += scale_x_q16;
                dst_x_end = origin_x +
                    (s32)((dst_x_q16 + 0xffffu) >> 16);

                if ((sprite->bmfmt == G_IM_FMT_RGBA) &&
                    (sprite->bmsiz == G_IM_SIZ_16b))
                {
                    color = ndsStartupLogoConvertRgba16(
                        ndsStartupLogoReadRgba16Pixel(src, src_width, row, x,
                                                      is_texshuf));
                }
                else if ((sprite->bmfmt == G_IM_FMT_RGBA) &&
                         (sprite->bmsiz == G_IM_SIZ_32b))
                {
                    const u32 *src_rgba32 = (const u32 *)src;
                    u32 source_x = x;
                    u32 rgba;

                    if ((is_texshuf != 0) && ((row & 1u) != 0))
                    {
                        source_x ^= 1u;
                    }
                    memcpy(&rgba,
                           &src_rgba32[(row * src_width) + source_x],
                           sizeof(rgba));
                    color = ndsSpriteConvertRgba32(rgba);
                }
                else if ((sprite->bmfmt == G_IM_FMT_IA) &&
                         (sprite->bmsiz == G_IM_SIZ_8b))
                {
                    const u8 *src_ia = (const u8 *)src;
                    u32 source_x = x;
                    size_t source_index;
                    u8 ia;

                    if ((is_texshuf != 0) && ((row & 1u) != 0))
                    {
                        source_x ^= 4u;
                    }
                    source_index = ((size_t)row * src_row_bytes) + source_x;
                    ia = src_ia[source_index ^ 3u];
                    color = (((ia & 0x0fu) != 0u) &&
                             (sprite->alpha != 0u)) ?
                        ndsSpriteLerpPrimEnv(
                            sobj, (u8)(((ia >> 4) * 255u) / 15u)) : 0;
                }
                else if ((sprite->bmfmt == G_IM_FMT_CI) &&
                         (sprite->bmsiz == G_IM_SIZ_8b))
                {
                    const u8 *src_ci = (const u8 *)src;
                    const u16 *palette = (const u16 *)sprite->LUT;
                    u32 source_x = x;
                    size_t source_index;
                    u8 index;

                    if ((is_texshuf != 0) && ((row & 1u) != 0))
                    {
                        source_x ^= 4u;
                    }
                    source_index = ((size_t)row * src_row_bytes) + source_x;
                    index = src_ci[source_index ^ 3u];
                    color = (ci_palette_ready != 0) ?
                        ndsStartupLogoConvertRgba16(
                            palette[((u32)index) ^ 1u]) : 0;
                }
                else if ((sprite->bmfmt == G_IM_FMT_I) &&
                         (sprite->bmsiz == G_IM_SIZ_4b))
                {
                    const u8 *src_i4 = (const u8 *)src;
                    u32 source_x = x;
                    size_t source_index;
                    u8 packed;
                    u8 intensity =
                        0;

                    if ((is_texshuf != 0) && ((row & 1u) != 0))
                    {
                        source_x ^= 8u;
                    }
                    source_index = ((size_t)row * src_row_bytes) +
                                   (source_x >> 1);
                    packed = src_i4[source_index ^ 3u];
                    intensity = ((source_x & 1u) == 0) ?
                        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
                    if (results_wallpaper_combine != 0u)
                    {
                        color = ndsSpriteLerpPrimEnv(
                            sobj, (u8)(((u32)intensity * 255u) / 15u));
                    }
                    else
                    {
                        color = ((intensity != 0u) &&
                                 (sprite->alpha != 0u)) ?
                            ndsSpritePackRgb15(sprite->red, sprite->green,
                                               sprite->blue) : 0;
                    }
                }
                else
                {
                    color = 0;
                }

                if ((is_texshuf != 0) && ((row & 1u) != 0))
                {
                    if (record_startup != 0)
                    {
                        gNdsStartupLogoDrawTexshufSamples++;
                    }
                }
                if (color != 0)
                {
                    if (is_scaled == FALSE)
                    {
                        if ((dst_x_start >= 0) &&
                            (dst_x_start < (s32)preview_width))
                        {
                            preview[((u32)dst_y_start * preview_pitch) +
                                    (u32)dst_x_start] = color;
                        }
                    }
                    else
                    {
                        s32 dst_y;

                        for (dst_y = dst_y_start;
                             dst_y < dst_y_end; dst_y++)
                        {
                            s32 dst_x;

                            if ((dst_y < 0) ||
                                (dst_y >= (s32)preview_height))
                            {
                                continue;
                            }
                            for (dst_x = dst_x_start;
                                 dst_x < dst_x_end; dst_x++)
                            {
                                if ((dst_x >= 0) &&
                                    (dst_x < (s32)preview_width))
                                {
                                    preview[((u32)dst_y * preview_pitch) +
                                            (u32)dst_x] = color;
                                }
                            }
                        }
                    }
                    drawn_pixels++;
                }
            }
        }
        out_y += row_advance;
    }

draw_complete:
    if (drawn_pixels == 0)
    {
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_BAD_BITMAP_BUFFER);
        return FALSE;
    }

    if (record_startup != 0)
    {
        gNdsStartupLogoDrawPixels = drawn_pixels;
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_NONE);
        gNdsStartupLogoDrawResult = NDS_STARTUP_LOGO_DRAW_PASS;
    }
    if (gSCManagerSceneData.scene_curr == nSCKindOpeningPortraits)
    {
        gNdsOpeningPortraitsDrawWidth = width;
        gNdsOpeningPortraitsDrawHeight = height;
        gNdsOpeningPortraitsDrawFormat = sprite->bmfmt;
        gNdsOpeningPortraitsDrawSize = sprite->bmsiz;
        gNdsOpeningPortraitsDrawBitmaps = bitmap_count;
        gNdsOpeningPortraitsDrawResult = NDS_OPENING_PORTRAITS_DRAW_PASS;
        gNdsOpeningPortraitsDrawPixels += drawn_pixels;
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_NONE);
    }
    if (gSCManagerSceneData.scene_curr == nSCKindOpeningMario)
    {
        gNdsOpeningMarioDrawWidth = width;
        gNdsOpeningMarioDrawHeight = height;
        gNdsOpeningMarioDrawFormat = sprite->bmfmt;
        gNdsOpeningMarioDrawSize = sprite->bmsiz;
        gNdsOpeningMarioDrawBitmaps = bitmap_count;
        gNdsOpeningMarioDrawResult = NDS_OPENING_MARIO_DRAW_PASS;
        gNdsOpeningMarioDrawPixels += drawn_pixels;
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_NONE);
    }
    if (ndsOpeningIsImportedNameScene(gSCManagerSceneData.scene_curr) != FALSE)
    {
        gNdsOpeningNameSceneDrawWidth = width;
        gNdsOpeningNameSceneDrawHeight = height;
        gNdsOpeningNameSceneDrawFormat = sprite->bmfmt;
        gNdsOpeningNameSceneDrawSize = sprite->bmsiz;
        gNdsOpeningNameSceneDrawBitmaps = bitmap_count;
        gNdsOpeningNameSceneDrawResult = NDS_OPENING_NAME_DRAW_PASS;
        gNdsOpeningNameSceneDrawPixels += drawn_pixels;
        gNdsOpeningNameSceneDrawMask |=
            ndsOpeningNameSceneMask(gSCManagerSceneData.scene_curr);
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_NONE);
    }
    if (gSCManagerSceneData.scene_curr == nSCKindTitle)
    {
        gNdsTitleDrawLastWidth = width;
        gNdsTitleDrawLastHeight = height;
        gNdsTitleDrawLastFormat = sprite->bmfmt;
        gNdsTitleDrawLastSize = sprite->bmsiz;
        gNdsTitleDrawPixels += drawn_pixels;
        gNdsTitleDrawResult = NDS_TITLE_DRAW_PASS;
    }
    if ((gSCManagerSceneData.scene_curr >= nSCKindOpeningRun) &&
        (gSCManagerSceneData.scene_curr <= nSCKindOpeningNewcomers))
    {
        gNdsOpeningMovieActionPreviewResult =
            NDS_OPENING_MOVIE_ACTION_PREVIEW_PASS;
        gNdsOpeningMovieActionPreviewMask |=
            1u << (gSCManagerSceneData.scene_curr - nSCKindOpeningRun);
        gNdsOpeningMovieActionPreviewPixels += drawn_pixels;
        gNdsOpeningMovieActionPreviewLastKind =
            gSCManagerSceneData.scene_curr;
        gNdsOpeningMovieActionPreviewLastWidth = width;
        gNdsOpeningMovieActionPreviewLastHeight = height;
        gNdsOpeningMovieActionPreviewLastFormat = sprite->bmfmt;
        gNdsOpeningMovieActionPreviewLastSize = sprite->bmsiz;
    }
    return TRUE;
}

static s32 ndsDrawSObjPreview(SObj *sobj, u32 record_startup)
{
    u16 *preview;
    u32 preview_pitch = 0;
    u32 width;
    u32 height;

    if (sobj == NULL)
    {
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_NO_SOBJ);
        return FALSE;
    }

    width = (u32)(u16)sobj->sprite.width;
    height = (u32)(u16)sobj->sprite.height;
    preview = ndsPlatformBeginOriginalSpritePreview(
        width, height, (s32)sobj->pos.x, (s32)sobj->pos.y, &preview_pitch);
    if ((preview == NULL) || (preview_pitch == 0))
    {
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_NO_PREVIEW_BUFFER);
        return FALSE;
    }
    if (ndsDrawSObjIntoPreview(sobj, record_startup, preview, preview_pitch,
                               width, height, 0, 0, 0u, 0u) == FALSE)
    {
        return FALSE;
    }
    ndsPlatformCommitOriginalSpritePreview();
    return TRUE;
}

static u16 *sNdsSObjFramePreview;
static u32 sNdsSObjFramePreviewPitch;
static u32 sNdsSObjFramePreviewDrawCount;
static u32 sNdsSObjFrameForeground;

static void ndsSObjPreviewCommitLayer(void)
{
    if ((sNdsSObjFramePreview != NULL) &&
        (sNdsSObjFramePreviewDrawCount != 0u))
    {
        ndsPlatformCommitOriginalSpritePreviewLayer(
            sNdsSObjFrameForeground != 0u);
    }
}

static void ndsDrawLayeredSObjFrame(GObj *gobj,
                                    u32 wallpaper_combine)
{
    SObj *sobj = (gobj != NULL) ? SObjGetStruct(gobj) : NULL;
    u32 foreground = FALSE;
    u32 cache_wallpaper = FALSE;

    if (gobj != NULL)
    {
        foreground = (gSCManagerSceneData.scene_curr == nSCKindVSResults) ?
            ((gobj->dl_link_id != 26u) ? TRUE : FALSE) :
            ((gobj->id != nGCCommonKindWallpaper) ? TRUE : FALSE);
        cache_wallpaper =
            ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
             (gobj->id == nGCCommonKindWallpaper) &&
             (wallpaper_combine == 0u)) ? TRUE : FALSE;
    }

    if ((foreground != FALSE) && (sNdsSObjFrameForeground == FALSE))
    {
        ndsSObjPreviewCommitLayer();
        sNdsSObjFramePreview = ndsPlatformBeginOriginalSpritePreview(
            320u, 240u, 0, 0, &sNdsSObjFramePreviewPitch);
        sNdsSObjFramePreviewDrawCount = 0u;
        sNdsSObjFrameForeground = TRUE;
    }

    while (sobj != NULL)
    {
        if (((sobj->sprite.attr & SP_HIDDEN) == 0) &&
            (ndsDrawSObjIntoPreview(
                sobj, 0u, sNdsSObjFramePreview,
                sNdsSObjFramePreviewPitch, 320u, 240u,
                (s32)sobj->pos.x, (s32)sobj->pos.y,
                wallpaper_combine, cache_wallpaper) != FALSE))
        {
            sNdsSObjFramePreviewDrawCount++;
        }
        sobj = sobj->next;
    }
}

void ndsSObjPreviewBeginFrame(void)
{
    sNdsSObjFramePreview = ndsPlatformBeginOriginalSpritePreview(
        320u, 240u, 0, 0, &sNdsSObjFramePreviewPitch);
    sNdsSObjFramePreviewDrawCount = 0;
    sNdsSObjFrameForeground = FALSE;
    ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
}

void ndsSObjPreviewEndFrame(void)
{
    ndsSObjPreviewCommitLayer();
    sNdsSObjFramePreview = NULL;
    sNdsSObjFramePreviewPitch = 0;
    sNdsSObjFramePreviewDrawCount = 0;
    sNdsSObjFrameForeground = FALSE;
}

void lbCommonDrawSObjAttr(GObj *gobj)
{
    SObj *sobj = (gobj != NULL) ? SObjGetStruct(gobj) : NULL;
    u32 visible_sobjs = 0;
    u32 record_startup = (gSCManagerSceneData.scene_curr == nSCKindStartup) ? 1u : 0u;

    if (record_startup != 0)
    {
        gNdsStartupLogoDrawCallbackCount++;
        gNdsStartupLogoDrawGObjID = (gobj != NULL) ? gobj->id : 0xffffffffu;
        gNdsStartupLogoDrawGObjObjKind =
            (gobj != NULL) ? gobj->obj_kind : 0xffffffffu;
    }
    if (gSCManagerSceneData.scene_curr == nSCKindOpeningPortraits)
    {
        gNdsOpeningPortraitsDrawCallbackCount++;
    }
    if (gSCManagerSceneData.scene_curr == nSCKindOpeningMario)
    {
        gNdsOpeningMarioDrawCallbackCount++;
    }
    if (ndsOpeningIsImportedNameScene(gSCManagerSceneData.scene_curr) != FALSE)
    {
        gNdsOpeningNameSceneDrawCallbackCount++;
    }
    if (sobj == NULL)
    {
        if (record_startup != 0)
        {
            gNdsStartupLogoDrawBlocker = NDS_STARTUP_LOGO_BLOCKER_NO_SOBJ;
        }
        return;
    }
    if (record_startup != 0)
    {
        gNdsStartupLogoDrawSObjAttr = sobj->sprite.attr;
    }
    if (((gSCManagerSceneData.scene_curr == nSCKindVSResults) ||
         (gSCManagerSceneData.scene_curr == nSCKindVSBattle)) &&
        (sNdsSObjFramePreview != NULL) &&
        (sNdsSObjFramePreviewPitch != 0u))
    {
        ndsDrawLayeredSObjFrame(gobj, 0u);
        return;
    }

    if ((gSCManagerSceneData.scene_curr == nSCKindOpeningPortraits) ||
        (ndsOpeningIsImportedNameScene(gSCManagerSceneData.scene_curr) !=
         FALSE))
    {
        SObj *scan_sobj = sobj;
        u32 renderable_sobjs = 0;
        u16 *preview;
        u32 preview_pitch = 0;
        u32 drew_any = 0;

        while (scan_sobj != NULL)
        {
            if ((scan_sobj->sprite.attr & SP_HIDDEN) == 0)
            {
                visible_sobjs++;
                if (ndsSObjPreviewBasicSupported(scan_sobj) != FALSE)
                {
                    renderable_sobjs++;
                }
            }
            scan_sobj = scan_sobj->next;
        }

        if ((gSCManagerSceneData.scene_curr == nSCKindOpeningPortraits) &&
            (visible_sobjs > gNdsOpeningPortraitsDrawVisibleSObjCount))
        {
            gNdsOpeningPortraitsDrawVisibleSObjCount = visible_sobjs;
        }
        if ((gSCManagerSceneData.scene_curr == nSCKindOpeningMario) &&
            (visible_sobjs > gNdsOpeningMarioDrawVisibleSObjCount))
        {
            gNdsOpeningMarioDrawVisibleSObjCount = visible_sobjs;
        }
        if ((ndsOpeningIsImportedNameScene(gSCManagerSceneData.scene_curr) !=
             FALSE) &&
            (visible_sobjs > gNdsOpeningNameSceneDrawVisibleSObjCount))
        {
            gNdsOpeningNameSceneDrawVisibleSObjCount = visible_sobjs;
        }
        if (renderable_sobjs == 0)
        {
            if (visible_sobjs == 0)
            {
                ndsRecordSObjDrawBlocker(
                    record_startup, NDS_STARTUP_LOGO_BLOCKER_NO_VISIBLE_SOBJ);
            }
            return;
        }

        preview = ndsPlatformBeginOriginalSpritePreview(320u, 240u, 0, 0,
                                                        &preview_pitch);
        if ((preview == NULL) || (preview_pitch == 0))
        {
            ndsRecordSObjDrawBlocker(
                record_startup, NDS_STARTUP_LOGO_BLOCKER_NO_PREVIEW_BUFFER);
            return;
        }

        scan_sobj = sobj;
        while (scan_sobj != NULL)
        {
            if ((scan_sobj->sprite.attr & SP_HIDDEN) == 0)
            {
                if (ndsDrawSObjIntoPreview(
                        scan_sobj, record_startup, preview, preview_pitch,
                        320u, 240u, (s32)scan_sobj->pos.x,
                        (s32)scan_sobj->pos.y, 0u, 0u) != FALSE)
                {
                    drew_any++;
                }
            }
            scan_sobj = scan_sobj->next;
        }

        if (drew_any != 0)
        {
            ndsPlatformCommitOriginalSpritePreview();
        }
        return;
    }

    while (sobj != NULL)
    {
        if ((sobj->sprite.attr & SP_HIDDEN) == 0)
        {
            visible_sobjs++;
            if (record_startup != 0)
            {
                gNdsStartupLogoDrawVisibleSObjCount = visible_sobjs;
            }
            if (gSCManagerSceneData.scene_curr == nSCKindOpeningPortraits)
            {
                gNdsOpeningPortraitsDrawVisibleSObjCount = visible_sobjs;
            }
            if (gSCManagerSceneData.scene_curr == nSCKindOpeningMario)
            {
                gNdsOpeningMarioDrawVisibleSObjCount = visible_sobjs;
            }
            if (ndsOpeningIsImportedNameScene(gSCManagerSceneData.scene_curr) !=
                FALSE)
            {
                gNdsOpeningNameSceneDrawVisibleSObjCount = visible_sobjs;
            }
            if (ndsDrawSObjPreview(sobj, record_startup) != FALSE)
            {
                if (record_startup != 0)
                {
                    return;
                }
            }
        }
        sobj = sobj->next;
    }
    if ((visible_sobjs == 0) &&
        (gNdsStartupLogoDrawResult != NDS_STARTUP_LOGO_DRAW_PASS))
    {
        gNdsStartupLogoDrawBlocker =
            NDS_STARTUP_LOGO_BLOCKER_NO_VISIBLE_SOBJ;
    }
    ndsIFCommonRecordHUDState();
}

void lbCommonDrawSObjNoAttr(GObj *gobj)
{
    if (((gSCManagerSceneData.scene_curr == nSCKindVSResults) ||
         (gSCManagerSceneData.scene_curr == nSCKindVSBattle)) &&
        (sNdsSObjFramePreview != NULL) &&
        (sNdsSObjFramePreviewPitch != 0u))
    {
        ndsDrawLayeredSObjFrame(gobj, 1u);
        return;
    }
    lbCommonDrawSObjAttr(gobj);
}

void lbCommonDrawSprite(GObj *camera_gobj)
{
    CObj *cobj;

    if (camera_gobj == NULL)
    {
        return;
    }

    cobj = CObjGetStruct(camera_gobj);
    if (cobj == NULL)
    {
        return;
    }

    gcCaptureCameraGObj(camera_gobj,
                        (cobj->flags & COBJ_FLAG_IDENTIFIER) ? TRUE : FALSE);
}

void lbFadeMakeActor(u32 id, u8 link, u32 priority, SYColorRGBA *color,
                     s32 duration, ub8 is_reverse, void *callback)
{
    (void)id;
    (void)link;
    (void)priority;
    (void)color;
    (void)duration;
    (void)is_reverse;
    (void)callback;
    sNdsFadeCreateCount++;
}

/* Object-manager dependency stubs.
 *
 * The imported sys/objman.c and sys/objhelper.c call into subsystems that are
 * not yet imported (display-list init, animation/script parsing, the camera
 * capture pipeline). The bounded update reaches only gcParseGObjScript through
 * the default camera GObj, with no active startup scripts; the display stubs
 * remain behind the parked draw path. Each is documented with the original file
 * it stands in for. */

/* sys/objscript.c: GObj script parse. The bounded startup update reaches this
 * through gcDefaultFuncRun, but startup has no active GObj scripts yet. */
sb32 gcParseGObjScript(void (*func)(GObjScript))
{
    (void)func;
    return FALSE;
}

/* sys/rdp.c default viewport contract. Keep this project-owned copy narrow
 * until importing the full RDP reset display-list path is safe. */
void syRdpSetDefaultViewport(Vp *vp)
{
    if (vp == NULL)
    {
        return;
    }

    vp->vp.vscale[0] = (s16)(gSYVideoResWidth * 2);
    vp->vp.vtrans[0] = (s16)(gSYVideoResWidth * 2);
    vp->vp.vscale[1] = (s16)(gSYVideoResHeight * 2);
    vp->vp.vtrans[1] = (s16)(gSYVideoResHeight * 2);
    vp->vp.vscale[2] = (s16)(0x03FF / 2);
    vp->vp.vtrans[2] = (s16)(0x03FF / 2);

    gNdsRdpDefaultViewportSetCount++;
    gNdsRdpDefaultViewportScaleX = vp->vp.vscale[0];
    gNdsRdpDefaultViewportScaleY = vp->vp.vscale[1];
    gNdsRdpDefaultViewportTransX = vp->vp.vtrans[0];
    gNdsRdpDefaultViewportTransY = vp->vp.vtrans[1];
    gNdsRdpDefaultViewportScaleZ = vp->vp.vscale[2];
    gNdsRdpDefaultViewportTransZ = vp->vp.vtrans[2];
}

/* sys/objdisplay.c: display-list and camera capture backend.
 *
 * Keep this as a narrow DS shim: imported gcDrawAll owns camera ordering, and
 * this file only lets the selected camera capture path reach one original
 * display callback before the real display-list translator is available. */
void gcInitDLs(void)
{
}

void gcSetCameraMatrixMode(s32 val)
{
    (void)val;
}

void gcSetMatrixFuncList(syMtxProcess *proc_mtx)
{
    (void)proc_mtx;
}
