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
extern u32 ndsIFCommonRouteGObjToLowerTextHUD(GObj *gobj);

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

u8 lbCommonGetBitmapDecodeNibble(u8 index)
{
    static const u8 nibbles[4] = { 0x00, 0x05, 0x0a, 0x0f };

    return nibbles[index];
}

void lbCommonDecodeBitmapSiz4b(u8 *bitmap_csr, u8 *bitmap_buf,
                               u8 *bitmap_start)
{
    size_t input_size;
    size_t output_size;
    u32 byte_lane_xor;

    if ((bitmap_csr == NULL) || (bitmap_buf == NULL) ||
        (bitmap_start == NULL) || (bitmap_csr < bitmap_start) ||
        (bitmap_buf < bitmap_start))
    {
        return;
    }

    input_size = (size_t)(bitmap_csr - bitmap_start) + 1u;
    output_size = (size_t)(bitmap_buf - bitmap_start) + 1u;
    byte_lane_xor =
        (ndsRelocFindLoadedFileContaining(bitmap_start, output_size) != NULL) ?
        3u : 0u;

    while (input_size != 0u)
    {
        size_t input_index = input_size - 1u;
        size_t output_index = input_index * 2u;
        u8 packed = bitmap_start[input_index ^ byte_lane_xor];
        u8 lower = lbCommonGetBitmapDecodeNibble((packed >> 4) & 3u);
        u8 upper = lbCommonGetBitmapDecodeNibble((packed >> 6) & 3u);

        bitmap_start[output_index ^ byte_lane_xor] =
            (u8)(lower | (upper << 4));
        lower = lbCommonGetBitmapDecodeNibble(packed & 3u);
        upper = lbCommonGetBitmapDecodeNibble((packed >> 2) & 3u);
        bitmap_start[(output_index + 1u) ^ byte_lane_xor] =
            (u8)(lower | (upper << 4));
        input_size--;
    }
}

void lbCommonDecodeSpriteBitmapsSiz4b(Sprite *sprite)
{
    s32 n;
    Bitmap *bitmap;

    for (n = sprite->nbitmaps, bitmap = sprite->bitmap; n > 0; n--)
    {
        s32 res = (bitmap[n - 1].width_img / 2) *
                  bitmap[n - 1].actualHeight;
        u8 *bitmap_start = (u8 *)bitmap[n - 1].buf;

        lbCommonDecodeBitmapSiz4b(bitmap_start + (res / 2) - 1,
                                  bitmap_start + res - 1,
                                  bitmap_start);
    }
    sprite->bmsiz = G_IM_SIZ_4b;
}

/* Narrow lbcommon startup shim.
 *
 * The full original lb/lbcommon.c translation unit fans out into the fighter
 * part tree, camera look-at helpers, and the N64 sprite display-list pipeline.
 * The current scenes need its SObj creation and source 4c expansion behavior,
 * so keep those bounded here until the full renderer slice is ready. */
SObj *lbCommonMakeSObjForGObj(GObj *gobj, Sprite *sprite)
{
    SObj *sobj;

    if (sprite->bmsiz == G_IM_SIZ_4c)
    {
        lbCommonDecodeSpriteBitmapsSiz4b(sprite);
    }
    sobj = gcAddSObjForGObj(gobj, sprite);

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
             ((sprite->bmfmt == G_IM_FMT_CI) &&
              (sprite->bmsiz == G_IM_SIZ_4b)) ||
             ((sprite->bmfmt == G_IM_FMT_I) &&
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

#define NDS_SOBJ_WALLPAPER_FINAL_MAPPING_VERSION 2u
#define NDS_SOBJ_WALLPAPER_FINAL_X_MAP_COUNT 256u
#define NDS_SOBJ_WALLPAPER_FINAL_Y_MAP_COUNT 192u
#define NDS_SOBJ_WALLPAPER_FINAL_MAP_SLOT_COUNT 2u
#define NDS_SOBJ_WALLPAPER_FINAL_MAP_SCRATCH_PIXELS \
    ((NDS_SOBJ_WALLPAPER_FINAL_X_MAP_COUNT * \
      NDS_SOBJ_WALLPAPER_FINAL_MAP_SLOT_COUNT) + \
     (NDS_SOBJ_WALLPAPER_FINAL_Y_MAP_COUNT * \
      NDS_SOBJ_WALLPAPER_FINAL_MAP_SLOT_COUNT) + \
     (NDS_SOBJ_WALLPAPER_FINAL_X_MAP_COUNT * 2u))

typedef struct NDSSObjWallpaperFinalCache
{
    u32 valid;
    u32 asset_id;
    u32 owner_scene;
    u32 owner_generation;
    const void *loaded_data;
    u32 bitmap_offset;
    u32 source_platform_epoch;
    u32 layout_fingerprint;
    u32 overlay_epoch;
    s32 origin_x;
    s32 origin_y;
    u32 scale_x_q16;
    u32 scale_y_q16;
    u32 combine_mode;
    u32 mapping_version;
    u32 map_slot;
} NDSSObjWallpaperFinalCache;

static NDSSObjWallpaperFinalCache sNdsSObjWallpaperFinalCache;

volatile u32 gNdsSObjWallpaperCacheBuildCount;
volatile u32 gNdsSObjWallpaperCacheHitCount;
volatile u32 gNdsSObjWallpaperCacheFastDrawCount;
volatile u32 gNdsSObjWallpaperCacheFallbackCount;
volatile u32 gNdsSObjWallpaperCacheWidth;
volatile u32 gNdsSObjWallpaperCacheHeight;
volatile u32 gNdsSObjWallpaperCacheOpaquePixels;
volatile u32 gNdsSObjWallpaperCacheBuildTicks;
volatile u32 gNdsSObjWallpaperCacheDrawTicks;
volatile u32 gNdsSObjWallpaperFinalDirectCount;
volatile u32 gNdsSObjWallpaperFinalSkipCount;
volatile u32 gNdsSObjWallpaperFinalKeyChangeCount;
volatile u32 gNdsSObjWallpaperFinalPixelWriteCount;
#if NDS_RENDERER_PROFILE_LEVEL == 0
volatile u32 gNdsSObjWallpaperIncrementalMode = 1u;
#else
volatile u32 gNdsSObjWallpaperIncrementalMode;
#endif
volatile u32 gNdsSObjWallpaperMapOracleCheckCount;
volatile u32 gNdsSObjWallpaperMapOracleMismatchCount;
volatile u32 gNdsSObjWallpaperPixelOracleCheckCount;
volatile u32 gNdsSObjWallpaperPixelOracleMismatchCount;
volatile u32 gNdsSObjWallpaperOracleFirstKind;
volatile u32 gNdsSObjWallpaperOracleFirstIndex;
volatile u32 gNdsSObjWallpaperOracleFirstExpected;
volatile u32 gNdsSObjWallpaperOracleFirstActual;
volatile u32 gNdsSObjBackgroundStagingClearBytes;
volatile u32 gNdsSObjForegroundStagingClearBytes;

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

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsSObjWallpaperRecordOracleMismatch(
    u32 kind, u32 index, u32 expected, u32 actual)
{
    if ((gNdsSObjWallpaperMapOracleMismatchCount == 0u) &&
        (gNdsSObjWallpaperPixelOracleMismatchCount == 0u))
    {
        gNdsSObjWallpaperOracleFirstKind = kind;
        gNdsSObjWallpaperOracleFirstIndex = index;
        gNdsSObjWallpaperOracleFirstExpected = expected;
        gNdsSObjWallpaperOracleFirstActual = actual;
    }
}
#endif

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

static s32 ndsSObjGetOpaqueWallpaperCache(
    const NDSRelocLoadedFile *loaded, const Sprite *sprite,
    u32 scale_x_q16, u32 scale_y_q16, u32 scratch_pixels,
    u16 **out_cache_pixels, u32 *out_cache_pitch)
{
    u16 *cache_pixels;
    u32 cache_pitch = 0u;
    u32 cache_height = 0u;
    u32 platform_epoch = 0u;
    u32 layout_fingerprint;

    if (out_cache_pixels != NULL) { *out_cache_pixels = NULL; }
    if (out_cache_pitch != NULL) { *out_cache_pitch = 0u; }
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
        return FALSE;
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
            return FALSE;
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
          cache_pitch) < scratch_pixels))
    {
        return FALSE;
    }
    if (out_cache_pixels != NULL) { *out_cache_pixels = cache_pixels; }
    if (out_cache_pitch != NULL) { *out_cache_pitch = cache_pitch; }
    return TRUE;
}

static void ndsSObjWallpaperPublishDrawTicks(u32 draw_start)
{
    u32 ticks = cpuGetTiming() - draw_start;

    gNdsSObjWallpaperCacheDrawTicks = (ticks != 0u) ? ticks : 1u;
}

static u32 ndsSObjDrawCachedWallpaper(
    const NDSRelocLoadedFile *loaded, const Sprite *sprite,
    u16 *preview, u32 preview_pitch, u32 preview_width, u32 preview_height,
    s32 origin_x, s32 origin_y, u32 scale_x_q16, u32 scale_y_q16)
{
    u16 *cache_pixels;
    u32 cache_pitch;
    u32 draw_start;
    u16 *source_x_map;

    if (ndsSObjGetOpaqueWallpaperCache(
            loaded, sprite, scale_x_q16, scale_y_q16, preview_width,
            &cache_pixels, &cache_pitch) == FALSE)
    {
        return 0u;
    }
    draw_start = cpuGetTiming();
    source_x_map = &cache_pixels[
        sNdsSObjWallpaperDecodeCache.height * cache_pitch];
    if (ndsSObjDrawOpaqueWallpaperCache(
            cache_pixels, cache_pitch, source_x_map,
            sNdsSObjWallpaperDecodeCache.width,
            sNdsSObjWallpaperDecodeCache.height,
            scale_x_q16, scale_y_q16, preview, preview_pitch,
            preview_width, preview_height, origin_x, origin_y) == FALSE)
    {
        ndsSObjWallpaperPublishDrawTicks(draw_start);
        return 0u;
    }
    gNdsSObjWallpaperCacheFastDrawCount++;
    ndsSObjWallpaperPublishDrawTicks(draw_start);
    return sNdsSObjWallpaperDecodeCache.source_drawn_pixels;
}

static s32 ndsSObjWallpaperFinalSourceMatches(
    const NDSRelocLoadedFile *loaded, u32 overlay_epoch, u32 combine_mode)
{
    const NDSSObjWallpaperFinalCache *final_cache =
        &sNdsSObjWallpaperFinalCache;
    const NDSSObjWallpaperDecodeCache *source_cache =
        &sNdsSObjWallpaperDecodeCache;

    return ((final_cache->valid != 0u) &&
            (source_cache->valid != 0u) &&
            (loaded != NULL) &&
            (final_cache->asset_id == source_cache->asset_id) &&
            (final_cache->owner_scene == source_cache->owner_scene) &&
            (final_cache->owner_generation == source_cache->owner_generation) &&
            (final_cache->loaded_data == source_cache->loaded_data) &&
            (final_cache->bitmap_offset == source_cache->bitmap_offset) &&
            (final_cache->source_platform_epoch ==
             source_cache->platform_epoch) &&
            (final_cache->layout_fingerprint ==
             source_cache->layout_fingerprint) &&
            (final_cache->overlay_epoch == overlay_epoch) &&
            (final_cache->combine_mode == combine_mode) &&
            (final_cache->mapping_version ==
             NDS_SOBJ_WALLPAPER_FINAL_MAPPING_VERSION) &&
            (final_cache->map_slot <
             NDS_SOBJ_WALLPAPER_FINAL_MAP_SLOT_COUNT)) ? TRUE : FALSE;
}

static s32 ndsSObjWallpaperFinalKeyMatches(
    const NDSRelocLoadedFile *loaded, u32 overlay_epoch,
    s32 origin_x, s32 origin_y, u32 scale_x_q16, u32 scale_y_q16,
    u32 combine_mode)
{
    const NDSSObjWallpaperFinalCache *final_cache =
        &sNdsSObjWallpaperFinalCache;

    return ((ndsSObjWallpaperFinalSourceMatches(
                loaded, overlay_epoch, combine_mode) != FALSE) &&
            (final_cache->origin_x == origin_x) &&
            (final_cache->origin_y == origin_y) &&
            (final_cache->scale_x_q16 == scale_x_q16) &&
            (final_cache->scale_y_q16 == scale_y_q16)) ? TRUE : FALSE;
}

static void ndsSObjWallpaperStoreFinalKey(
    u32 overlay_epoch, s32 origin_x, s32 origin_y,
    u32 scale_x_q16, u32 scale_y_q16, u32 combine_mode, u32 map_slot)
{
    NDSSObjWallpaperFinalCache *final_cache =
        &sNdsSObjWallpaperFinalCache;
    const NDSSObjWallpaperDecodeCache *source_cache =
        &sNdsSObjWallpaperDecodeCache;

    final_cache->asset_id = source_cache->asset_id;
    final_cache->owner_scene = source_cache->owner_scene;
    final_cache->owner_generation = source_cache->owner_generation;
    final_cache->loaded_data = source_cache->loaded_data;
    final_cache->bitmap_offset = source_cache->bitmap_offset;
    final_cache->source_platform_epoch = source_cache->platform_epoch;
    final_cache->layout_fingerprint = source_cache->layout_fingerprint;
    final_cache->overlay_epoch = overlay_epoch;
    final_cache->origin_x = origin_x;
    final_cache->origin_y = origin_y;
    final_cache->scale_x_q16 = scale_x_q16;
    final_cache->scale_y_q16 = scale_y_q16;
    final_cache->combine_mode = combine_mode;
    final_cache->mapping_version = NDS_SOBJ_WALLPAPER_FINAL_MAPPING_VERSION;
    final_cache->map_slot = map_slot;
    final_cache->valid = TRUE;
}

static s32 __attribute__((hot, optimize("O3")))
ndsSObjDrawOpaqueWallpaperFinal(
    const u16 *cache_pixels, u32 cache_pitch, u16 *map_scratch,
    u32 current_map_slot, u32 incremental_valid, u32 row_dma_enabled,
    u32 width, u32 height, u32 scale_x_q16, u32 scale_y_q16,
    u16 *overlay, u32 overlay_pitch, u32 overlay_width, u32 overlay_height,
    s32 origin_x, s32 origin_y, u32 *out_pixel_write_count)
{
    const u32 preview_width = 320u;
    const u32 preview_height = 240u;
    const u16 no_source = 0xffffu;
    u16 *source_x_map;
    u16 *source_y_map;
    const u16 *previous_source_x_map;
    const u16 *previous_source_y_map;
    u16 *changed_x_indices;
    u16 *expanded_row;
    const u16 *expanded_row_source = NULL;
    u32 expanded_row_valid = FALSE;
    u32 previous_map_slot;
    u32 changed_x_count = 0u;
    u32 pixel_write_count = 0u;
    u32 step_x;
    u32 step_y;
    u32 preview_x_q16;
    u32 preview_y_q16;
    u32 previous_preview_x = 0u;
    u32 previous_preview_y = 0u;
    u32 source_x_unclamped = 0u;
    u32 source_y_unclamped = 0u;
    u32 source_x_remainder = 0u;
    u32 source_y_remainder = 0u;
    u32 source_x_recurrence_valid = FALSE;
    u32 source_y_recurrence_valid = FALSE;
    u32 source_x_map_complete = TRUE;
    u32 packed_rows;
    const u16 *previous_src = NULL;
    u16 *previous_dst = NULL;
    s32 dst_x_end;
    s32 dst_y_end;
    u32 x;
    u32 y;

    if (out_pixel_write_count != NULL) { *out_pixel_write_count = 0u; }
    if ((cache_pixels == NULL) || (map_scratch == NULL) ||
        (overlay == NULL) || (overlay_pitch < overlay_width) ||
        (overlay_width != NDS_SOBJ_WALLPAPER_FINAL_X_MAP_COUNT) ||
        (overlay_height != NDS_SOBJ_WALLPAPER_FINAL_Y_MAP_COUNT) ||
        (current_map_slot >= NDS_SOBJ_WALLPAPER_FINAL_MAP_SLOT_COUNT))
    {
        return FALSE;
    }
    previous_map_slot = current_map_slot ^ 1u;
    /* The immutable 300x220 decode occupies 70,400 of the retained 76,800
     * pixels. Keep both exact screen-to-source maps, the changed-X list, and
     * one expanded DMA row in that existing 6,400-pixel scratch tail. */
    source_x_map = &map_scratch[
        current_map_slot * NDS_SOBJ_WALLPAPER_FINAL_X_MAP_COUNT];
    previous_source_x_map = &map_scratch[
        previous_map_slot * NDS_SOBJ_WALLPAPER_FINAL_X_MAP_COUNT];
    source_y_map = &map_scratch[
        (NDS_SOBJ_WALLPAPER_FINAL_X_MAP_COUNT *
         NDS_SOBJ_WALLPAPER_FINAL_MAP_SLOT_COUNT) +
        (current_map_slot * NDS_SOBJ_WALLPAPER_FINAL_Y_MAP_COUNT)];
    previous_source_y_map = &map_scratch[
        (NDS_SOBJ_WALLPAPER_FINAL_X_MAP_COUNT *
         NDS_SOBJ_WALLPAPER_FINAL_MAP_SLOT_COUNT) +
        (previous_map_slot * NDS_SOBJ_WALLPAPER_FINAL_Y_MAP_COUNT)];
    changed_x_indices = &map_scratch[
        (NDS_SOBJ_WALLPAPER_FINAL_X_MAP_COUNT *
         NDS_SOBJ_WALLPAPER_FINAL_MAP_SLOT_COUNT) +
        (NDS_SOBJ_WALLPAPER_FINAL_Y_MAP_COUNT *
         NDS_SOBJ_WALLPAPER_FINAL_MAP_SLOT_COUNT)];
    expanded_row = changed_x_indices +
        NDS_SOBJ_WALLPAPER_FINAL_X_MAP_COUNT;
    step_x = (preview_width << 16) / overlay_width;
    step_y = (preview_height << 16) / overlay_height;
    preview_x_q16 = step_x >> 1;
    dst_x_end = origin_x +
        (s32)((((u64)width * scale_x_q16) + 0xffffu) >> 16);
    dst_y_end = origin_y +
        (s32)((((u64)height * scale_y_q16) + 0xffffu) >> 16);

    for (x = 0u; x < overlay_width; x++)
    {
        u32 preview_x = preview_x_q16 >> 16;

        source_x_map[x] = no_source;
        if (((s32)preview_x >= origin_x) &&
            ((s32)preview_x < dst_x_end))
        {
            u32 source_x;

            if (source_x_recurrence_valid == FALSE)
            {
                u32 relative = (u32)((s32)preview_x - origin_x);
                u32 numerator = ((relative + 1u) << 16) - 1u;

                source_x_unclamped = numerator / scale_x_q16;
                source_x_remainder = numerator -
                    (source_x_unclamped * scale_x_q16);
                source_x_recurrence_valid = TRUE;
            }
            else
            {
                source_x_remainder +=
                    (preview_x - previous_preview_x) << 16;
                while (source_x_remainder >= scale_x_q16)
                {
                    source_x_remainder -= scale_x_q16;
                    source_x_unclamped++;
                }
            }
            previous_preview_x = preview_x;
            source_x = source_x_unclamped;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
            {
                u32 expected_source_x = ndsSObjWallpaperLastSource(
                    (u32)((s32)preview_x - origin_x), scale_x_q16);

                gNdsSObjWallpaperMapOracleCheckCount++;
                if (source_x != expected_source_x)
                {
                    ndsSObjWallpaperRecordOracleMismatch(
                        1u, x, expected_source_x, source_x);
                    gNdsSObjWallpaperMapOracleMismatchCount++;
                }
            }
#endif

            if (source_x >= width) { source_x = width - 1u; }
            source_x_map[x] = (u16)source_x;
        }
        else
        {
            source_x_map_complete = FALSE;
        }
        if ((incremental_valid != FALSE) &&
            (source_x_map[x] != previous_source_x_map[x]))
        {
            changed_x_indices[changed_x_count++] = (u16)x;
        }
        preview_x_q16 += step_x;
    }

    packed_rows = ((source_x_map_complete != FALSE) &&
                   ((overlay_width & 1u) == 0u) &&
                   ((overlay_pitch & 1u) == 0u) &&
                   (((uintptr_t)source_x_map & 3u) == 0u) &&
                   (((uintptr_t)overlay & 3u) == 0u)) ? TRUE : FALSE;
    preview_y_q16 = step_y >> 1;
    for (y = 0u; y < overlay_height; y++)
    {
        u32 preview_y = preview_y_q16 >> 16;
        u16 source_y_map_value = no_source;
        const u16 *src = NULL;
        u16 *dst = &overlay[y * overlay_pitch];
        u32 full_row;

        if (((s32)preview_y >= origin_y) &&
            ((s32)preview_y < dst_y_end))
        {
            u32 source_y;

            if (source_y_recurrence_valid == FALSE)
            {
                u32 relative = (u32)((s32)preview_y - origin_y);
                u32 numerator = ((relative + 1u) << 16) - 1u;

                source_y_unclamped = numerator / scale_y_q16;
                source_y_remainder = numerator -
                    (source_y_unclamped * scale_y_q16);
                source_y_recurrence_valid = TRUE;
            }
            else
            {
                source_y_remainder +=
                    (preview_y - previous_preview_y) << 16;
                while (source_y_remainder >= scale_y_q16)
                {
                    source_y_remainder -= scale_y_q16;
                    source_y_unclamped++;
                }
            }
            previous_preview_y = preview_y;
            source_y = source_y_unclamped;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
            {
                u32 expected_source_y = ndsSObjWallpaperLastSource(
                    (u32)((s32)preview_y - origin_y), scale_y_q16);

                gNdsSObjWallpaperMapOracleCheckCount++;
                if (source_y != expected_source_y)
                {
                    ndsSObjWallpaperRecordOracleMismatch(
                        2u, y, expected_source_y, source_y);
                    gNdsSObjWallpaperMapOracleMismatchCount++;
                }
            }
#endif

            if (source_y >= height) { source_y = height - 1u; }
            source_y_map_value = (u16)source_y;
            src = &cache_pixels[source_y * cache_pitch];
        }
        source_y_map[y] = source_y_map_value;
        full_row = ((incremental_valid == FALSE) ||
                    (source_y_map_value != previous_source_y_map[y]) ||
                    ((row_dma_enabled != FALSE) &&
                     (changed_x_count >= (overlay_width >> 1)))) ?
            TRUE : FALSE;
        if ((full_row != FALSE) && (row_dma_enabled != FALSE))
        {
            if ((expanded_row_valid == FALSE) ||
                (expanded_row_source != src))
            {
                if ((src != NULL) && (packed_rows != FALSE))
                {
                    u32 *expanded_pairs = (u32 *)expanded_row;
                    const u32 *source_pair = (const u32 *)source_x_map;
                    const u32 *source_pair_end = source_pair +
                        (overlay_width >> 1);

                    while ((source_pair + 1) < source_pair_end)
                    {
                        u32 pair0 = source_pair[0];
                        u32 pair1 = source_pair[1];

                        expanded_pairs[0] =
                            (u32)src[(u16)pair0] |
                            ((u32)src[pair0 >> 16] << 16);
                        expanded_pairs[1] =
                            (u32)src[(u16)pair1] |
                            ((u32)src[pair1 >> 16] << 16);
                        source_pair += 2;
                        expanded_pairs += 2;
                    }
                    if (source_pair < source_pair_end)
                    {
                        u32 pair = *source_pair;

                        *expanded_pairs = (u32)src[(u16)pair] |
                            ((u32)src[pair >> 16] << 16);
                    }
                }
                else
                {
                    for (x = 0u; x < overlay_width; x++)
                    {
                        expanded_row[x] = ((src != NULL) &&
                            (source_x_map[x] != no_source)) ?
                            src[source_x_map[x]] : 0u;
                    }
                }
                DC_FlushRange(
                    expanded_row, overlay_width * sizeof(expanded_row[0]));
                expanded_row_source = src;
                expanded_row_valid = TRUE;
            }
            dmaCopyHalfWords(
                0, expanded_row, dst,
                overlay_width * sizeof(expanded_row[0]));
            pixel_write_count += overlay_width;
        }
        else if ((full_row != FALSE) &&
                 (src != NULL) && (src == previous_src) &&
                 (previous_dst != NULL) && (packed_rows != FALSE))
        {
            memcpy(dst, previous_dst,
                   overlay_width * sizeof(dst[0]));
            pixel_write_count += overlay_width;
        }
        else if ((full_row != FALSE) &&
                 (src != NULL) && (packed_rows != FALSE))
        {
            u32 *dst_pairs = (u32 *)dst;

            /* BG2 rows are word-aligned and the opaque Dream Land wallpaper
             * covers the complete visible X map. Pack two exact RGB5A1
             * samples per VRAM store instead of issuing 49,152 halfword
             * writes every camera update. */
            const u32 *source_pair = (const u32 *)source_x_map;
            const u32 *source_pair_end = source_pair +
                (overlay_width >> 1);

            while ((source_pair + 1) < source_pair_end)
            {
                u32 pair0 = source_pair[0];
                u32 pair1 = source_pair[1];

                dst_pairs[0] =
                    (u32)src[(u16)pair0] |
                    ((u32)src[pair0 >> 16] << 16);
                dst_pairs[1] =
                    (u32)src[(u16)pair1] |
                    ((u32)src[pair1 >> 16] << 16);
                source_pair += 2;
                dst_pairs += 2;
            }
            if (source_pair < source_pair_end)
            {
                u32 pair = *source_pair;

                *dst_pairs = (u32)src[(u16)pair] |
                    ((u32)src[pair >> 16] << 16);
            }
            pixel_write_count += overlay_width;
        }
        else if (full_row != FALSE)
        {
            for (x = 0u; x < overlay_width; x++)
            {
                dst[x] = ((src != NULL) &&
                          (source_x_map[x] != no_source)) ?
                    src[source_x_map[x]] : 0u;
            }
            pixel_write_count += overlay_width;
        }
        else
        {
            for (x = 0u; x < changed_x_count; x++)
            {
                u32 changed_x = changed_x_indices[x];

                dst[changed_x] = ((src != NULL) &&
                    (source_x_map[changed_x] != no_source)) ?
                    src[source_x_map[changed_x]] : 0u;
            }
            pixel_write_count += changed_x_count;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        for (x = 0u; x < overlay_width; x++)
        {
            u16 expected_pixel = ((src != NULL) &&
                                  (source_x_map[x] != no_source)) ?
                src[source_x_map[x]] : 0u;

            gNdsSObjWallpaperPixelOracleCheckCount++;
            if (dst[x] != expected_pixel)
            {
                ndsSObjWallpaperRecordOracleMismatch(
                    3u, (y * overlay_width) + x,
                    expected_pixel, dst[x]);
                gNdsSObjWallpaperPixelOracleMismatchCount++;
            }
        }
#endif
        previous_src = src;
        previous_dst = dst;
        preview_y_q16 += step_y;
    }
    if (out_pixel_write_count != NULL)
    {
        *out_pixel_write_count = pixel_write_count;
    }
    return TRUE;
}

static s32 ndsSObjDrawCachedWallpaperFinal(SObj *sobj, u32 combine_mode)
{
    Sprite *sprite;
    NDSRelocLoadedFile *loaded;
    u16 *overlay;
    u16 *cache_pixels;
    u16 *map_scratch;
    u32 overlay_pitch;
    u32 overlay_width;
    u32 overlay_height;
    u32 overlay_epoch;
    u32 cache_pitch;
    u32 scale_x_q16;
    u32 scale_y_q16;
    u32 draw_start;
    u32 committed_epoch;
    u32 current_map_slot = 0u;
    u32 incremental_valid = FALSE;
    u32 incremental_mode;
    u32 pixel_count;
    s32 origin_x;
    s32 origin_y;

    if ((sobj == NULL) || (combine_mode != 0u))
    {
        return FALSE;
    }
    sprite = &sobj->sprite;
    if ((sprite->scalex < 0.0001F) || (sprite->scaley < 0.0001F))
    {
        return FALSE;
    }
    overlay = ndsPlatformGetOriginalSpriteOverlayLayer(
        FALSE, &overlay_pitch, &overlay_width, &overlay_height,
        &overlay_epoch);
    if ((overlay == NULL) || (overlay_width != 256u) ||
        (overlay_height != 192u) || (overlay_pitch < overlay_width))
    {
        return FALSE;
    }
    loaded = ndsRelocFindLoadedFileContaining(
        sprite->bitmap,
        sizeof(Bitmap) * (u32)(u16)sprite->nbitmaps);
    if (ndsRelocPointerRangeInLoadedFile(
            loaded, sprite->bitmap,
            sizeof(Bitmap) * (u32)(u16)sprite->nbitmaps) == FALSE)
    {
        return FALSE;
    }
    if ((sprite->attr & SP_FASTCOPY) != 0u)
    {
        scale_x_q16 = 1u << 16;
        scale_y_q16 = 1u << 16;
    }
    else
    {
        scale_x_q16 = (u32)((sprite->scalex * 65536.0F) + 0.5F);
        scale_y_q16 = (u32)((sprite->scaley * 65536.0F) + 0.5F);
    }
    if (ndsSObjGetOpaqueWallpaperCache(
            loaded, sprite, scale_x_q16, scale_y_q16,
            NDS_SOBJ_WALLPAPER_FINAL_MAP_SCRATCH_PIXELS,
            &cache_pixels, &cache_pitch) == FALSE)
    {
        return FALSE;
    }

    draw_start = cpuGetTiming();
    origin_x = (s32)sobj->pos.x;
    origin_y = (s32)sobj->pos.y;
    if (ndsSObjWallpaperFinalKeyMatches(
            loaded, overlay_epoch, origin_x, origin_y,
            scale_x_q16, scale_y_q16, combine_mode) != FALSE)
    {
        gNdsSObjWallpaperFinalDirectCount++;
        gNdsSObjWallpaperFinalSkipCount++;
        gNdsSObjWallpaperCacheFastDrawCount++;
        ndsSObjWallpaperPublishDrawTicks(draw_start);
        return TRUE;
    }

#if NDS_RENDERER_PROFILE_LEVEL == 0
    /* Shipping has no A/B telemetry state in the decision path. Profiles
     * 1/2 retain the runtime selector for same-ROM timing and exact oracles. */
    incremental_mode = TRUE;
#else
    incremental_mode =
        (gNdsSObjWallpaperIncrementalMode != 0u) ? TRUE : FALSE;
#endif
    if ((incremental_mode != FALSE) &&
        (ndsSObjWallpaperFinalSourceMatches(
            loaded, overlay_epoch, combine_mode) != FALSE))
    {
        current_map_slot = sNdsSObjWallpaperFinalCache.map_slot ^ 1u;
        incremental_valid = TRUE;
    }
    map_scratch = &cache_pixels[
        sNdsSObjWallpaperDecodeCache.height * cache_pitch];
    if (ndsSObjDrawOpaqueWallpaperFinal(
            cache_pixels, cache_pitch, map_scratch,
            current_map_slot, incremental_valid,
            incremental_mode,
            sNdsSObjWallpaperDecodeCache.width,
            sNdsSObjWallpaperDecodeCache.height,
            scale_x_q16, scale_y_q16, overlay, overlay_pitch,
            overlay_width, overlay_height, origin_x, origin_y,
            &pixel_count) == FALSE)
    {
        ndsSObjWallpaperPublishDrawTicks(draw_start);
        return FALSE;
    }
    committed_epoch = ndsPlatformCommitOriginalSpriteFinalLayer(
        FALSE, pixel_count);
    if (committed_epoch == 0u)
    {
        sNdsSObjWallpaperFinalCache.valid = FALSE;
        ndsSObjWallpaperPublishDrawTicks(draw_start);
        return FALSE;
    }
    ndsSObjWallpaperStoreFinalKey(
        committed_epoch, origin_x, origin_y,
        scale_x_q16, scale_y_q16, combine_mode, current_map_slot);
    gNdsSObjWallpaperFinalDirectCount++;
    gNdsSObjWallpaperFinalKeyChangeCount++;
    gNdsSObjWallpaperFinalPixelWriteCount += pixel_count;
    gNdsSObjWallpaperCacheFastDrawCount++;
    ndsSObjWallpaperPublishDrawTicks(draw_start);
    return TRUE;
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
          ((sprite->bmfmt == G_IM_FMT_CI) &&
           (sprite->bmsiz == G_IM_SIZ_4b)) ||
          ((sprite->bmfmt == G_IM_FMT_I) &&
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

        if (sprite->bmsiz == G_IM_SIZ_4b)
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
        if (sprite->bmfmt == G_IM_FMT_CI)
        {
            const u8 *src_ci = (const u8 *)src;
            const u16 *palette = (const u16 *)sprite->LUT;
            size_t i;

            for (i = 0; i < src_bytes; i++)
            {
                u32 first_index = src_ci[i];
                u32 second_index = first_index;

                if (sprite->bmsiz == G_IM_SIZ_4b)
                {
                    second_index = first_index & 0x0fu;
                    first_index >>= 4;
                }
                if (first_index > ci_max_index)
                {
                    ci_max_index = first_index;
                }
                if (second_index > ci_max_index)
                {
                    ci_max_index = second_index;
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
                else if ((sprite->bmfmt == G_IM_FMT_CI) &&
                         (sprite->bmsiz == G_IM_SIZ_4b))
                {
                    const u8 *src_ci = (const u8 *)src;
                    const u16 *palette = (const u16 *)sprite->LUT;
                    u32 source_x = x;
                    size_t source_index;
                    u8 packed;
                    u8 index;

                    if ((is_texshuf != 0) && ((row & 1u) != 0))
                    {
                        source_x ^= 8u;
                    }
                    source_index = ((size_t)row * src_row_bytes) +
                                   (source_x >> 1);
                    packed = src_ci[source_index ^ 3u];
                    index = ((source_x & 1u) == 0u) ?
                        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
                    color = (ci_palette_ready != 0u) ?
                        ndsStartupLogoConvertRgba16(
                            palette[((u32)index) ^ 1u]) : 0;
                }
                else if ((sprite->bmfmt == G_IM_FMT_I) &&
                         (sprite->bmsiz == G_IM_SIZ_8b))
                {
                    const u8 *src_i8 = (const u8 *)src;
                    u32 source_x = x;
                    size_t source_index;
                    u8 intensity;

                    if ((is_texshuf != 0) && ((row & 1u) != 0))
                    {
                        source_x ^= 4u;
                    }
                    source_index = ((size_t)row * src_row_bytes) + source_x;
                    intensity = src_i8[source_index ^ 3u];
                    color = ((intensity != 0u) &&
                             (sprite->alpha != 0u)) ?
                        ndsSpritePackRgb15(sprite->red, sprite->green,
                                           sprite->blue) : 0;
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
static u32 sNdsSObjFrameActive;
static SObj *sNdsSObjFramePendingWallpaper;
static SObj sNdsSObjFramePendingWallpaperSnapshot;
static u32 sNdsSObjFramePendingWallpaperCombine;
static u32 sNdsSObjFrameForegroundCommitted;
static u32 sNdsSObjOverlayForegroundPopulated;

static void ndsSObjPreviewBeginStagingLayer(void)
{
    if (sNdsSObjFramePreview != NULL)
    {
        return;
    }
    sNdsSObjFramePreview = ndsPlatformBeginOriginalSpritePreview(
        320u, 240u, 0, 0, &sNdsSObjFramePreviewPitch);
    if ((sNdsSObjFramePreview != NULL) &&
        (sNdsSObjFramePreviewPitch != 0u))
    {
        if (sNdsSObjFrameForeground != FALSE)
        {
            gNdsSObjForegroundStagingClearBytes +=
                320u * 240u * sizeof(u16);
        }
        else
        {
            gNdsSObjBackgroundStagingClearBytes +=
                320u * 240u * sizeof(u16);
        }
    }
}

static void ndsSObjPreviewFlushPendingWallpaperToStaging(void)
{
    SObj *wallpaper = sNdsSObjFramePendingWallpaper;
    u32 combine_mode = sNdsSObjFramePendingWallpaperCombine;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 profile_start;
#endif

    sNdsSObjFramePendingWallpaper = NULL;
    sNdsSObjFramePendingWallpaperCombine = 0u;
    if (wallpaper == NULL)
    {
        return;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    profile_start = cpuGetTiming();
#endif
    ndsSObjPreviewBeginStagingLayer();
    if (ndsDrawSObjIntoPreview(
            wallpaper, 0u, sNdsSObjFramePreview,
            sNdsSObjFramePreviewPitch, 320u, 240u,
            (s32)wallpaper->pos.x, (s32)wallpaper->pos.y,
            combine_mode, TRUE) != FALSE)
    {
        sNdsSObjFramePreviewDrawCount++;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileWallpaperTicks += cpuGetTiming() - profile_start;
#endif
}

static void ndsSObjPreviewCommitLayer(void)
{
    if (sNdsSObjFramePendingWallpaper != NULL)
    {
        s32 final_wallpaper = FALSE;

        if (sNdsSObjFrameForeground == FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            u32 wallpaper_start = cpuGetTiming();
#endif
            SObj *wallpaper = sNdsSObjFramePendingWallpaper;
            u32 scale_x_q16 = 0u;
            u32 scale_y_q16 = 0u;
            u32 retained_wallpaper = FALSE;

            if ((wallpaper->sprite.scalex >= 0.0001F) &&
                (wallpaper->sprite.scaley >= 0.0001F))
            {
                if ((wallpaper->sprite.attr & SP_FASTCOPY) != 0u)
                {
                    scale_x_q16 = 1u << 16;
                    scale_y_q16 = 1u << 16;
                }
                else
                {
                    scale_x_q16 = (u32)(
                        (wallpaper->sprite.scalex * 65536.0F) + 0.5F);
                    scale_y_q16 = (u32)(
                        (wallpaper->sprite.scaley * 65536.0F) + 0.5F);
                }
                retained_wallpaper =
                    ndsPlatformSceneWallpaperQueueTransform(
                        (s32)wallpaper->pos.x,
                        (s32)wallpaper->pos.y,
                        scale_x_q16, scale_y_q16);
            }
            if (retained_wallpaper != FALSE)
            {
                final_wallpaper = TRUE;
            }
            else
            {
                final_wallpaper = ndsSObjDrawCachedWallpaperFinal(
                    wallpaper, sNdsSObjFramePendingWallpaperCombine);
                if (final_wallpaper != FALSE)
                {
                    ndsPlatformSceneWallpaperConfirmRaster();
                }
            }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            gNdsRendererProfileWallpaperTicks +=
                cpuGetTiming() - wallpaper_start;
#endif
        }
        if (final_wallpaper != FALSE)
        {
            sNdsSObjFramePendingWallpaper = NULL;
            sNdsSObjFramePendingWallpaperCombine = 0u;
        }
        else
        {
            ndsSObjPreviewFlushPendingWallpaperToStaging();
        }
    }
    if ((sNdsSObjFramePreview != NULL) &&
        (sNdsSObjFramePreviewDrawCount != 0u))
    {
        ndsPlatformCommitOriginalSpritePreviewLayer(
            sNdsSObjFrameForeground != 0u);
        if (sNdsSObjFrameForeground != FALSE)
        {
            sNdsSObjFrameForegroundCommitted = TRUE;
            sNdsSObjOverlayForegroundPopulated = TRUE;
        }
    }
    sNdsSObjFramePreview = NULL;
    sNdsSObjFramePreviewPitch = 0u;
    sNdsSObjFramePreviewDrawCount = 0u;
}

static void ndsDrawLayeredSObjFrame(GObj *gobj,
                                    u32 wallpaper_combine)
{
    SObj *sobj = (gobj != NULL) ? SObjGetStruct(gobj) : NULL;
    u32 foreground = FALSE;
    u32 cache_wallpaper = FALSE;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 foreground_start = 0u;
    u32 profile_foreground = FALSE;
#endif

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
        sNdsSObjFrameForeground = TRUE;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    if ((foreground != FALSE) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle))
    {
        profile_foreground = TRUE;
        foreground_start = cpuGetTiming();
    }
#endif

    if ((foreground != FALSE) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (ndsIFCommonNativeOamDrawGObj(gobj) != FALSE))
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        if (profile_foreground != FALSE)
        {
            gNdsRendererProfileForegroundTicks +=
                cpuGetTiming() - foreground_start;
        }
#endif
        return;
    }

    while (sobj != NULL)
    {
        if ((sobj->sprite.attr & SP_HIDDEN) == 0)
        {
            if ((cache_wallpaper != FALSE) && (foreground == FALSE) &&
                (sNdsSObjFramePendingWallpaper == NULL) &&
                (sNdsSObjFramePreview == NULL) &&
                (sNdsSObjFramePreviewDrawCount == 0u))
            {
                /* Delay the one-source Dream Land background until the layer
                 * boundary. A later background SObj forces the unchanged
                 * staging path before any final BG2 pixels are written. Keep
                 * the source state by value because Cut G restores its seed
                 * camera before the outer frame commits this deferred layer. */
                sNdsSObjFramePendingWallpaperSnapshot = *sobj;
                sNdsSObjFramePendingWallpaperSnapshot.next = NULL;
                sNdsSObjFramePendingWallpaperSnapshot.prev = NULL;
                sNdsSObjFramePendingWallpaper =
                    &sNdsSObjFramePendingWallpaperSnapshot;
                sNdsSObjFramePendingWallpaperCombine = wallpaper_combine;
            }
            else
            {
                if (ndsSObjPreviewBasicSupported(sobj) == FALSE)
                {
                    sobj = sobj->next;
                    continue;
                }
                if (sNdsSObjFramePendingWallpaper != NULL)
                {
                    ndsSObjPreviewFlushPendingWallpaperToStaging();
                }
                ndsSObjPreviewBeginStagingLayer();
                if (ndsDrawSObjIntoPreview(
                        sobj, 0u, sNdsSObjFramePreview,
                        sNdsSObjFramePreviewPitch, 320u, 240u,
                        (s32)sobj->pos.x, (s32)sobj->pos.y,
                        wallpaper_combine, cache_wallpaper) != FALSE)
                {
                    sNdsSObjFramePreviewDrawCount++;
                }
            }
        }
        sobj = sobj->next;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    if (profile_foreground != FALSE)
    {
        gNdsRendererProfileForegroundTicks +=
            cpuGetTiming() - foreground_start;
    }
#endif
}

void ndsSObjPreviewBeginFrame(void)
{
    ndsIFCommonNativeOamBeginFrame();
    sNdsSObjFramePreview = NULL;
    sNdsSObjFramePreviewPitch = 0u;
    sNdsSObjFramePreviewDrawCount = 0u;
    sNdsSObjFrameForeground = FALSE;
    sNdsSObjFrameActive = TRUE;
    sNdsSObjFramePendingWallpaper = NULL;
    sNdsSObjFramePendingWallpaperCombine = 0u;
    sNdsSObjFrameForegroundCommitted = FALSE;
}

void ndsSObjPreviewEndFrame(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 profile_foreground =
        ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
         ((sNdsSObjFrameForeground != FALSE) ||
          (sNdsSObjOverlayForegroundPopulated != FALSE))) ? TRUE : FALSE;
    u32 foreground_start =
        (profile_foreground != FALSE) ? cpuGetTiming() : 0u;
#endif

    ndsSObjPreviewCommitLayer();
    if ((sNdsSObjFrameForegroundCommitted == FALSE) &&
        (sNdsSObjOverlayForegroundPopulated != FALSE))
    {
        /* A full 256x192 foreground commit already carries transparent zeroes
         * for every untouched pixel. Only clear BG3 when a previously
         * populated layer becomes empty; the old unconditional 128 KiB clear
         * duplicated work on every frame. */
        ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
        sNdsSObjOverlayForegroundPopulated = FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    if (profile_foreground != FALSE)
    {
        gNdsRendererProfileForegroundTicks +=
            cpuGetTiming() - foreground_start;
    }
#endif
    sNdsSObjFramePreview = NULL;
    sNdsSObjFramePreviewPitch = 0;
    sNdsSObjFramePreviewDrawCount = 0;
    sNdsSObjFrameForeground = FALSE;
    sNdsSObjFrameActive = FALSE;
    sNdsSObjFramePendingWallpaper = NULL;
    sNdsSObjFramePendingWallpaperCombine = 0u;
    sNdsSObjFrameForegroundCommitted = FALSE;
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
        (sNdsSObjFrameActive != FALSE))
    {
        if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
            (ndsIFCommonRouteGObjToLowerTextHUD(gobj) != FALSE))
        {
            /* BattleShip still runs each source display callback so timer,
             * stock, and damage state advance normally. Only its prepared
             * steady HUD composition is redirected to the DS lower text
             * backend; countdown/GO GObjs keep the original top BG3 path. */
            ndsIFCommonRecordHUDState();
            return;
        }
        if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
            (gobj != NULL) &&
            (gobj->id == nGCCommonKindInterface) &&
            (gobj->proc_display == lbCommonDrawSObjAttr))
        {
            gNdsIFCommonHUDTopGenericPassCount++;
        }
        ndsDrawLayeredSObjFrame(gobj, 0u);
        if (gSCManagerSceneData.scene_curr == nSCKindVSBattle)
        {
            ndsIFCommonRecordHUDState();
        }
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
        (sNdsSObjFrameActive != FALSE))
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
