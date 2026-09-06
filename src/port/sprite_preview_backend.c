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

/* R2-07 R0c. This is the hottest function in the Results scene: it runs once per
 * blitted pixel, and the I4 wallpaper alone is 300x220 = 66,000 pixels per frame.
 *
 * The three divisions here were `/ 255u`, and at -Os GCC does NOT turn a
 * compile-time constant divisor into a reciprocal multiply -- it emits
 * `blx __udivsi3`, because the call is smaller than the multiply-shift sequence.
 * That trade is catastrophic at this trip count: measured on the ELF, this
 * function carried THREE `__udivsi3` calls, so the wallpaper paid ~198,000 library
 * divisions per frame on a core with no divide instruction. (The caller's
 * `(nibble * 255u) / 15u` was NOT one of them -- GCC had already strength-reduced
 * that, and it is now `* 17u` for clarity rather than for speed.)
 *
 * `(x * 257 + 257) >> 16` equals `x / 255` exactly for every x this function can
 * produce. The bound is what makes it exact and it is not obvious: `intensity`
 * and `inverse` are COMPLEMENTARY (they sum to 255), so the numerator cannot
 * reach 2*255*255 -- its true maximum is 255*255 + 127 = 65,152, inside the range
 * where the identity holds. Verified exhaustively over [0, 65152] and over the
 * full (colour, envcolour, intensity) input space by
 * `scripts/check_sprite_lerp_exact.py`, which fails the check if either bound
 * moves. Do not widen `intensity` beyond u8 or make `inverse` independent of it
 * without re-running that checker: both would break the bound, silently. */
#define NDS_SPRITE_DIV255(x) (((x) * 257u + 257u) >> 16)

/* R2-07 R0d. `always_inline` because -Os would not: R0c's ELF still showed two real
 * `bl` sites into a `.isra.0` clone, and the callee pushed and popped EIGHT registers
 * (r4-r7 plus r8/r9/sl/lr) around what is now three multiply-shift channels. At 66,000
 * calls per frame the prologue and epilogue cost more than the arithmetic they guard.
 * Only two call sites exist, so the size cost is bounded at roughly one extra copy.
 *
 * The opposite of E65's choice on `ndsR2CubicValueFixed`, and for a reason worth
 * keeping straight: that one is `noinline` to hold ONE copy of six inlined conversions
 * inside `.text.hot`'s curated 8 KiB. This blitter is not in `.text.hot`, so that
 * constraint does not apply here. */
#if defined(__GNUC__)
#define NDS_SPRITE_LERP_ATTR static inline __attribute__((always_inline))
#else
#define NDS_SPRITE_LERP_ATTR static inline
#endif

NDS_SPRITE_LERP_ATTR u16 ndsSpriteLerpPrimEnv(const SObj *sobj, u8 intensity)
{
    u32 inverse = 255u - intensity;
    u8 red = (u8)NDS_SPRITE_DIV255((u32)sobj->sprite.red * intensity +
                                   (u32)sobj->envcolor.r * inverse + 127u);
    u8 green = (u8)NDS_SPRITE_DIV255((u32)sobj->sprite.green * intensity +
                                     (u32)sobj->envcolor.g * inverse + 127u);
    u8 blue = (u8)NDS_SPRITE_DIV255((u32)sobj->sprite.blue * intensity +
                                    (u32)sobj->envcolor.b * inverse + 127u);

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
             ((sprite->bmfmt == G_IM_FMT_IA) &&
              (sprite->bmsiz == G_IM_SIZ_4b)) ||
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
    /* R2-07 R2b. Non-zero when the decode already applied the prim/env combine,
     * so every consumer downstream may treat this cache as combine-free. The
     * Dream Land battle wallpaper is RGBA/16b with no combine and leaves this
     * zero; the VS Results wallpaper is I/4b under a combine whose output is a
     * pure function of the 4-bit intensity (R0e), so sixteen palette entries
     * bake it exactly. Threading one flag is what lets the Results wallpaper
     * reach the affine BG path without teaching that path about combines --
     * and keeping it a FLAG rather than an assumption is what stops a cache
     * MISS from silently drawing the wallpaper uncombined. */
    u32 combine_baked;
    /* Retain the sixteen exact Results colours instead of 300x220 expanded
     * RGB555 pixels. Dream Land leaves combine_baked zero and never reads it. */
    u16 combine_palette[16];
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
#define NDS_SOBJ_WALLPAPER_SOURCE_ROW_PIXELS 300u

/* P2-2 RAM reclaim. The old hardware path retained a 300x220 RGB555 decode
 * (132 KiB) only so the destination mapper could sample one source row at a
 * time. Keep exactly what the mapper actually needs: its two map generations,
 * changed-X / expanded DMA row scratch, and one decoded 300-pixel source row.
 * Source pixels remain in the already-loaded BattleShip asset and are decoded
 * from the validated strip layout on demand. */
static u16 sNdsSObjWallpaperMapScratch[
    NDS_SOBJ_WALLPAPER_FINAL_MAP_SCRATCH_PIXELS];
static u16 sNdsSObjWallpaperSourceRow[
    NDS_SOBJ_WALLPAPER_SOURCE_ROW_PIXELS];

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

/* Decode one source row directly from the source sprite's bitmap strips.
 * This is the exact old full-cache pixel loop, just scoped to one Y. Later
 * strips overwrite only with opaque RGBA pixels, preserving BattleShip's SObj
 * overlap semantics; Results I4 is fully opaque after the exact palette bake. */
static u32 ndsSObjDecodeWallpaperSourceRow(
    const Sprite *sprite, u32 source_y, const u16 *combine_palette,
    u16 *dst, u32 dst_width)
{
    const Bitmap *bitmap;
    u32 width;
    u32 height;
    u32 bitmap_count;
    u32 is_texshuf;
    u32 out_y = 0u;
    u32 drawn_pixels = 0u;
    u32 bitmap_index;

    if ((sprite == NULL) || (dst == NULL))
    {
        return 0u;
    }
    bitmap = sprite->bitmap;
    width = (u32)(u16)sprite->width;
    height = (u32)(u16)sprite->height;
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    is_texshuf = ((sprite->attr & SP_TEXSHUF) != 0u) ? 1u : 0u;
    if ((source_y >= height) || (dst_width < width)) { return 0u; }
    memset(dst, 0, width * sizeof(dst[0]));
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
        size_t src_row_bytes;

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
        src_row_bytes = (combine_palette != NULL) ?
            (((size_t)src_width + 1u) / 2u) :
            ((size_t)src_width * sizeof(u16));
        if ((source_y >= out_y) && (source_y < (out_y + src_height)))
        {
            u32 row = source_y - out_y;
            u32 x;

            if (combine_palette != NULL)
            {
                /* I/4b under a baked combine. Same index algebra as R0e's
                 * specialized row -- one byte per PAIR of columns, `^ 4` on the
                 * byte index for SP_TEXSHUF's odd rows, `^ 3` word swizzle --
                 * and proven by `check_sprite_lerp_exact.py`. Every palette
                 * entry has bit 15 set, so every pixel is opaque and the census
                 * below reaches width*height, which is exactly the condition
                 * `ndsSObjGetOpaqueWallpaperCache` requires before it will use a
                 * destination-driven last-writer mapping. Runs ONCE per scene. */
                const u8 *src_i4 = (const u8 *)src;
                size_t row_base = (size_t)row * src_row_bytes;
                size_t byte_xor = ((is_texshuf != 0u) && ((row & 1u) != 0u)) ?
                    4u : 0u;
                u32 pairs = src_draw_width >> 1;
                u32 pair;

                for (pair = 0u; pair < pairs; pair++)
                {
                    u8 packed = src_i4[(row_base + (pair ^ byte_xor)) ^ 3u];

                    dst[pair * 2u] = combine_palette[packed >> 4];
                    dst[(pair * 2u) + 1u] = combine_palette[packed & 0x0fu];
                }
                if ((src_draw_width & 1u) != 0u)
                {
                    u8 packed = src_i4[(row_base + (pairs ^ byte_xor)) ^ 3u];

                    dst[src_draw_width - 1u] = combine_palette[packed >> 4];
                }
                drawn_pixels += src_draw_width;
            }
            else
            {
                for (x = 0u; x < src_draw_width; x++)
                {
                    u16 color = ndsStartupLogoConvertRgba16(
                        ndsStartupLogoReadRgba16Pixel(
                            src, src_width, row, x, is_texshuf));

                    /* Transparent later strips do not erase earlier overlap
                     * rows in the source sprite pipeline. */
                    if (color != 0u)
                    {
                        dst[x] = color;
                        drawn_pixels++;
                    }
                }
            }
        }
        out_y += row_advance;
    }
    return drawn_pixels;
}

/* `combine_palette` is NULL for the RGBA/16b battle wallpaper and sixteen baked
 * entries for the I/4b Results wallpaper. Rather than expanding either asset to
 * a retained 300x220 RGB555 duplicate, validate every source strip once and run
 * the old decode over one scratch row at a time to prove final opacity. */
static s32 ndsSObjBuildWallpaperDecodeCache(
    const NDSRelocLoadedFile *loaded, const Sprite *sprite,
    u32 platform_epoch, u32 layout_fingerprint,
    const u16 *combine_palette)
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

    if ((loaded == NULL) || (sprite == NULL) ||
        (width > NDS_SOBJ_WALLPAPER_SOURCE_ROW_PIXELS))
    {
        return FALSE;
    }
    sNdsSObjWallpaperDecodeCache.valid = FALSE;
    /* Preserve the old fail-closed pointer/range proof. The row decoder can
     * then sample without repeating relocation range checks for every screen
     * row on every affine-key change. */
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
        size_t src_row_bytes;
        size_t src_bytes;

        if (src_draw_width == 0u) { break; }
        if (src_width == 0u) { src_width = src_draw_width; }
        if (src_height == 0u) { src_height = row_advance; }
        if (row_advance == 0u) { row_advance = src_height; }
        if ((src_width == 0u) || (src_height == 0u))
        {
            continue;
        }
        src_row_bytes = (combine_palette != NULL) ?
            (((size_t)src_width + 1u) / 2u) :
            ((size_t)src_width * sizeof(u16));
        src_bytes = src_row_bytes * src_height;
        if (ndsRelocPointerRangeInLoadedFile(loaded, src, src_bytes) == FALSE)
        {
            return FALSE;
        }
        out_y += row_advance;
    }
    for (row = 0u; row < height; row++)
    {
        u32 x;

        drawn_pixels += ndsSObjDecodeWallpaperSourceRow(
            sprite, row, combine_palette, sNdsSObjWallpaperSourceRow,
            NDS_SOBJ_WALLPAPER_SOURCE_ROW_PIXELS);
        for (x = 0u; x < width; x++)
        {
            if (sNdsSObjWallpaperSourceRow[x] != 0u) { opaque_pixels++; }
        }
    }
    if (drawn_pixels == 0u)
    {
        return FALSE;
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
    sNdsSObjWallpaperDecodeCache.combine_baked =
        (combine_palette != NULL) ? 1u : 0u;
    if (combine_palette != NULL)
    {
        memcpy(sNdsSObjWallpaperDecodeCache.combine_palette, combine_palette,
               sizeof(sNdsSObjWallpaperDecodeCache.combine_palette));
    }
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
    const Sprite *sprite, const u16 *combine_palette, u16 *source_x_map,
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
        const u16 *src = sNdsSObjWallpaperSourceRow;
        u16 *dst;

        if (source_y >= height) { source_y = height - 1u; }
        (void)ndsSObjDecodeWallpaperSourceRow(
            sprite, source_y, combine_palette, sNdsSObjWallpaperSourceRow,
            NDS_SOBJ_WALLPAPER_SOURCE_ROW_PIXELS);
        dst = &preview[(u32)dst_y * preview_pitch];
        for (dst_x = dst_x_start; dst_x < dst_x_end; dst_x++)
        {
            dst[dst_x] = src[source_x_map[dst_x]];
        }
    }
    return TRUE;
}

/* Two wallpapers reach this cache, and they are told apart by format rather than
 * by scene, so nothing here has to know which scene is running.
 *
 *   Dream Land battle: RGBA/16b, 44 bitmaps, bmheight 5, bmHreal 6, no combine.
 *   VS Results:        I/4b, 9 bitmaps, under the prim/env combine, which the
 *                      decode bakes into sixteen palette entries (R0e).
 *
 * Both are 300x220. Returning a palette pointer through `out_combine_palette` is
 * how the caller learns which one it got; the storage is the caller's, so this
 * function stays free of state. */
static s32 ndsSObjWallpaperIsResultsShape(const Sprite *sprite)
{
#if NDS_R2_RESULTS_AFFINE
    return ((sprite != NULL) && (sprite->bmfmt == G_IM_FMT_I) &&
            (sprite->bmsiz == G_IM_SIZ_4b) &&
            ((u32)(u16)sprite->nbitmaps == 9u) &&
            ((u32)(u16)sprite->width == 300u) &&
            ((u32)(u16)sprite->height == 220u)) ? TRUE : FALSE;
#else
    (void)sprite;
    return FALSE;
#endif
}

#define NDS_DREAMLAND_WALLPAPER_STRETCH_SHIFT 3u

/* K = 9/8 = 1.125. The design minimum is 1.090909 and the originally derived
 * K=1.12 is visually indistinguishable at DS resolution, but /25 generated
 * ARM software-divide helpers and the exact reciprocal form generated 64-bit
 * multiply helpers. 9/8 is the DS-native form: shifts/adds only, with 0.45%
 * more overdraw than 1.12. */
static inline s32 ndsSObjWallpaperMul9Div8Signed(s32 value)
{
    u32 magnitude = (value < 0) ? (u32)(-value) : (u32)value;
    s32 scaled = (s32)(((magnitude << NDS_DREAMLAND_WALLPAPER_STRETCH_SHIFT) +
                        magnitude) >> NDS_DREAMLAND_WALLPAPER_STRETCH_SHIFT);

    return (value < 0) ? -scaled : scaled;
}

/* Dream Land's source wallpaper deliberately leaves the outer ~10 preview
 * pixels uncovered at its 1.004 camera-scale floor; that was hidden by N64
 * overscan but the DS presents the full 256x192 image. Grow the presentation
 * transform by 9/8 (1.125) about the 320x240 preview centre. Keep this a pure
 * port-side presentation correction: the source SObj/camera state is not
 * changed, and other stages/results keep their exact source transform.
 *
 * This helper uses only fixed integer arithmetic. Dream Land's source scale is
 * clamped to [1.004, 2.0], and the origin input is already constrained to the
 * signed 16-bit range by the affine caller. */
static void ndsSObjApplyDreamLandWallpaperStretch(
    s32 *origin_x, s32 *origin_y, u32 *scale_x_q16, u32 *scale_y_q16,
    u32 stretch_scale)
{
    s32 dx;
    s32 dy;

    /* Every fight gate in this file reads the scene table's BATTLE flag
     * (gNdsSceneManagerCurrIsBattle), not the VS kind: the 1P ladder, the
     * bonus boards, Training, Explain and the attract demo present the same
     * HUD, wallpaper and OAM foreground as a VS match (2026-09-05). */
    if ((origin_x == NULL) || (origin_y == NULL) ||
        (scale_x_q16 == NULL) || (scale_y_q16 == NULL) ||
        (gNdsSceneManagerCurrIsBattle == 0u) ||
        (gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->gkind != nGRKindPupupu))
    {
        return;
    }
    /* The source contract is 1.004..2.0. Refuse an unexpected transform rather
     * than multiplying an unrelated/corrupt value under a Dream Land scene. */
    if ((*scale_x_q16 < (1u << 16)) ||
        (*scale_y_q16 < (1u << 16)) ||
        (*scale_x_q16 > (2u << 16)) ||
        (*scale_y_q16 > (2u << 16)))
    {
        return;
    }

    dx = *origin_x - 160;
    dy = *origin_y - 120;
    *origin_x = 160 + ndsSObjWallpaperMul9Div8Signed(dx);
    *origin_y = 120 + ndsSObjWallpaperMul9Div8Signed(dy);
    if (stretch_scale != FALSE)
    {
        *scale_x_q16 += (*scale_x_q16 + 4u) >>
            NDS_DREAMLAND_WALLPAPER_STRETCH_SHIFT;
        *scale_y_q16 += (*scale_y_q16 + 4u) >>
            NDS_DREAMLAND_WALLPAPER_STRETCH_SHIFT;
    }
}

static s32 ndsSObjWallpaperCombinePaletteFor(
    const SObj *sobj, const Sprite *sprite, u16 *storage)
{
    if ((sobj != NULL) && (ndsSObjWallpaperIsResultsShape(sprite) != FALSE))
    {
        u32 nibble;

        for (nibble = 0u; nibble < 16u; nibble++)
        {
            storage[nibble] = ndsSpriteLerpPrimEnv(sobj, (u8)(nibble * 17u));
        }
        return TRUE;
    }
    (void)storage;
    return FALSE;
}

/* Why a battle wallpaper was refused by the shape test below. Zero on every
 * healthy run; a non-zero count with a stage showing no background names this
 * seam instead of leaving it to be re-derived. */
__attribute__((used)) volatile u32 gNdsSObjWallpaperShapeRejectCount;
__attribute__((used)) volatile u32 gNdsSObjWallpaperShapeRejectAsset;
__attribute__((used)) volatile u32 gNdsSObjWallpaperShapeRejectBitmaps;

static s32 ndsSObjGetOpaqueWallpaperCache(
    const NDSRelocLoadedFile *loaded, const Sprite *sprite,
    u32 scale_x_q16, u32 scale_y_q16, u32 scratch_pixels,
    const u16 *combine_palette)
{
    u32 platform_epoch;
    u32 layout_fingerprint;
    u32 shape_ok;
    u32 palette_changed = FALSE;

    platform_epoch = ndsPlatformGetOriginalSpritePreviewEpoch();
    if ((loaded == NULL) || (sprite == NULL) ||
        ((u32)(u16)sprite->width != 300u) ||
        ((u32)(u16)sprite->height != 220u) ||
        (scratch_pixels > NDS_SOBJ_WALLPAPER_FINAL_MAP_SCRATCH_PIXELS))
    {
        return FALSE;
    }
    if (combine_palette != NULL)
    {
        /* The Results wallpaper. Its asset is whatever mnVSResultsMakeWallpaper
         * loaded, so the shape -- not an asset id -- is the contract. */
        shape_ok = (((u32)(u16)sprite->nbitmaps == 9u) &&
                    (sprite->bmfmt == G_IM_FMT_I) &&
                    (sprite->bmsiz == G_IM_SIZ_4b)) ? 1u : 0u;
    }
    else
    {
        /* THE BATTLE WALLPAPER, FOR WHICHEVER STAGE LOADED IT.
         *
         * This used to lead with
         * `loaded->asset_id == NDS_RELOC_ASSET_STAGE_DREAM_LAND`, which was
         * true while Dream Land was the only stage that reached a battle. It
         * is a contradiction of the arm directly above, whose comment states
         * the rule: the asset is whatever the scene loaded, so the SHAPE is
         * the contract, not an id. With eight opt-in stages shipped, every
         * one of their wallpapers was decoded into an SObj and then refused
         * here, which is why the owner's 2026-09-04 playtest reports all
         * eight missing their background while Dream Land keeps its own
         * (docs/BUGS.md).
         *
         * The shape terms stay exactly as they were -- 300x220 is already
         * checked above, and these pin the tiling and format that the decode
         * below assumes. Dropping only the id widens this to every stage that
         * presents the same wallpaper shape and admits nothing else. The
         * cache cannot be confused between stages either: its key is built
         * from `loaded` (ndsSObjWallpaperCacheKeyMatches), so a different
         * asset rebuilds rather than reuses.
         *
         * A stage whose wallpaper does NOT match this shape is still refused,
         * and that used to be silent. The counter below makes it attributable
         * -- a non-zero value with a missing background names this seam. */
        shape_ok = (((u32)(u16)sprite->nbitmaps == 44u) &&
                    ((u32)(u16)sprite->bmheight == 5u) &&
                    ((u32)(u16)sprite->bmHreal == 6u) &&
                    (sprite->bmfmt == G_IM_FMT_RGBA) &&
                    (sprite->bmsiz == G_IM_SIZ_16b)) ? 1u : 0u;
        if (shape_ok == 0u)
        {
            gNdsSObjWallpaperShapeRejectCount++;
            gNdsSObjWallpaperShapeRejectAsset = loaded->asset_id;
            gNdsSObjWallpaperShapeRejectBitmaps =
                (u32)(u16)sprite->nbitmaps;
        }
    }
    if (shape_ok == 0u)
    {
        return FALSE;
    }
    layout_fingerprint = ndsSObjWallpaperLayoutFingerprint(
        loaded, sprite->bitmap, (u32)(u16)sprite->nbitmaps);
    if ((combine_palette != NULL) &&
        (sNdsSObjWallpaperDecodeCache.combine_baked != 0u) &&
        (memcmp(sNdsSObjWallpaperDecodeCache.combine_palette,
                combine_palette,
                sizeof(sNdsSObjWallpaperDecodeCache.combine_palette)) != 0))
    {
        palette_changed = TRUE;
    }
    /* A cache built for one of the two wallpapers must not be reused for the
     * other. Results also keys the sixteen baked colours now that the expanded
     * RGB image is no longer retained. */
    if ((ndsSObjWallpaperCacheKeyMatches(
             loaded, sprite, platform_epoch, layout_fingerprint) == FALSE) ||
        (sNdsSObjWallpaperDecodeCache.combine_baked !=
         ((combine_palette != NULL) ? 1u : 0u)) ||
        (palette_changed != FALSE))
    {
        if (ndsSObjBuildWallpaperDecodeCache(
                loaded, sprite, platform_epoch, layout_fingerprint,
                combine_palette) == FALSE)
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
        (scale_y_q16 < (1u << 16)))
    {
        return FALSE;
    }
    return TRUE;
}

static void ndsSObjWallpaperPublishDrawTicks(u32 draw_start)
{
    u32 ticks = cpuGetTiming() - draw_start;

    gNdsSObjWallpaperCacheDrawTicks = (ticks != 0u) ? ticks : 1u;
}

static u32 ndsSObjDrawCachedWallpaper(
    const SObj *sobj,
    const NDSRelocLoadedFile *loaded, const Sprite *sprite,
    u16 *preview, u32 preview_pitch, u32 preview_width, u32 preview_height,
    s32 origin_x, s32 origin_y, u32 scale_x_q16, u32 scale_y_q16)
{
    u32 draw_start;
    u16 *source_x_map = sNdsSObjWallpaperMapScratch;
    u16 combine_palette[16];
    const u16 *palette =
        (ndsSObjWallpaperCombinePaletteFor(sobj, sprite, combine_palette) !=
         FALSE) ? combine_palette : NULL;

    if (ndsSObjGetOpaqueWallpaperCache(
            loaded, sprite, scale_x_q16, scale_y_q16, preview_width,
            palette) == FALSE)
    {
        return 0u;
    }
    draw_start = cpuGetTiming();
    if (ndsSObjDrawOpaqueWallpaperCache(
            sprite,
            (sNdsSObjWallpaperDecodeCache.combine_baked != 0u) ?
                sNdsSObjWallpaperDecodeCache.combine_palette : NULL,
            source_x_map,
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
    const Sprite *sprite, const u16 *combine_palette, u16 *map_scratch,
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
    u16 expanded_row_source_y = no_source;
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
    u16 previous_source_y = no_source;
    u16 *previous_dst = NULL;
    s32 dst_x_end;
    s32 dst_y_end;
    u32 x;
    u32 y;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 phase05_end;
    u32 phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif

    if (out_pixel_write_count != NULL) { *out_pixel_write_count = 0u; }
    if ((sprite == NULL) || (map_scratch == NULL) ||
        (overlay == NULL) || (overlay_pitch < overlay_width) ||
        (overlay_width != NDS_SOBJ_WALLPAPER_FINAL_X_MAP_COUNT) ||
        (overlay_height != NDS_SOBJ_WALLPAPER_FINAL_Y_MAP_COUNT) ||
        (current_map_slot >= NDS_SOBJ_WALLPAPER_FINAL_MAP_SLOT_COUNT))
    {
#if NDS_RENDERER_M3_PHASE0_PROFILE
        NDS_RENDERER_PHASE05_FINISH(
            gNdsRendererPhase05WallpaperSetupTicks, phase05_start);
#endif
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

#if NDS_RENDERER_M3_PHASE0_PROFILE
    phase05_end = NDS_RENDERER_PHASE05_TICK();
    gNdsRendererPhase05WallpaperSetupTicks += phase05_end - phase05_start;
    gNdsRendererPhase05TimerSpanCount++;
    phase05_start = phase05_end;
#endif
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

#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererPhase05WallpaperChangedXCount = changed_x_count;
    if (changed_x_count != 0u)
    {
        u32 run_start = 0u;

        while (run_start < changed_x_count)
        {
            u32 run_end = run_start + 1u;
            u32 run_length;

            while ((run_end < changed_x_count) &&
                   (changed_x_indices[run_end] ==
                    (u16)(changed_x_indices[run_end - 1u] + 1u)))
            {
                run_end++;
            }
            run_length = run_end - run_start;
            gNdsRendererPhase05WallpaperChangedRunCount++;
            if (run_length >
                gNdsRendererPhase05WallpaperLongestChangedRun)
            {
                gNdsRendererPhase05WallpaperLongestChangedRun = run_length;
            }
            if (run_length >= 2u)
            {
                gNdsRendererPhase05WallpaperRunGE2Count++;
                gNdsRendererPhase05WallpaperRunGE2Pixels += run_length;
            }
            if (run_length >= 4u)
            {
                gNdsRendererPhase05WallpaperRunGE4Count++;
                gNdsRendererPhase05WallpaperRunGE4Pixels += run_length;
            }
            if (run_length >= 8u)
            {
                gNdsRendererPhase05WallpaperRunGE8Count++;
                gNdsRendererPhase05WallpaperRunGE8Pixels += run_length;
            }
            run_start = run_end;
        }
    }
    phase05_end = NDS_RENDERER_PHASE05_TICK();
    gNdsRendererPhase05WallpaperXMapTicks += phase05_end - phase05_start;
    gNdsRendererPhase05TimerSpanCount++;
    phase05_start = phase05_end;
#endif
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
            if (source_y_map_value != previous_source_y)
            {
                (void)ndsSObjDecodeWallpaperSourceRow(
                    sprite, source_y, combine_palette,
                    sNdsSObjWallpaperSourceRow,
                    NDS_SOBJ_WALLPAPER_SOURCE_ROW_PIXELS);
            }
            src = sNdsSObjWallpaperSourceRow;
        }
        source_y_map[y] = source_y_map_value;
        full_row = ((incremental_valid == FALSE) ||
                    (source_y_map_value != previous_source_y_map[y]) ||
                    ((row_dma_enabled != FALSE) &&
                     (changed_x_count >= (overlay_width >> 1)))) ?
            TRUE : FALSE;
#if NDS_RENDERER_M3_PHASE0_PROFILE
        if (full_row != FALSE)
        {
            gNdsRendererPhase05WallpaperFullRowCount++;
        }
        else
        {
            gNdsRendererPhase05WallpaperIncrementalRowCount++;
        }
        phase05_end = NDS_RENDERER_PHASE05_TICK();
        gNdsRendererPhase05WallpaperYMapTicks +=
            phase05_end - phase05_start;
        gNdsRendererPhase05TimerSpanCount++;
        phase05_start = phase05_end;
#endif
        if ((full_row != FALSE) && (row_dma_enabled != FALSE))
        {
            if ((expanded_row_valid == FALSE) ||
                (expanded_row_source_y != source_y_map_value))
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
                expanded_row_source_y = source_y_map_value;
                expanded_row_valid = TRUE;
            }
            dmaCopyHalfWords(
                0, expanded_row, dst,
                overlay_width * sizeof(expanded_row[0]));
            pixel_write_count += overlay_width;
#if NDS_RENDERER_M3_PHASE0_PROFILE
            gNdsRendererPhase05WallpaperDmaPixelCount += overlay_width;
#endif
        }
        else if ((full_row != FALSE) &&
                 (src != NULL) &&
                 (source_y_map_value == previous_source_y) &&
                 (previous_dst != NULL) && (packed_rows != FALSE))
        {
            memcpy(dst, previous_dst,
                   overlay_width * sizeof(dst[0]));
            pixel_write_count += overlay_width;
#if NDS_RENDERER_M3_PHASE0_PROFILE
            gNdsRendererPhase05WallpaperCopyPixelCount += overlay_width;
#endif
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
#if NDS_RENDERER_M3_PHASE0_PROFILE
            gNdsRendererPhase05WallpaperPackedStoreCount +=
                overlay_width >> 1;
#endif
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
#if NDS_RENDERER_M3_PHASE0_PROFILE
            gNdsRendererPhase05WallpaperScalarStoreCount += overlay_width;
#endif
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
#if NDS_RENDERER_M3_PHASE0_PROFILE
            gNdsRendererPhase05WallpaperScalarStoreCount += changed_x_count;
#endif
        }
#if NDS_RENDERER_M3_PHASE0_PROFILE
        phase05_end = NDS_RENDERER_PHASE05_TICK();
        gNdsRendererPhase05WallpaperWriteTicks +=
            phase05_end - phase05_start;
        gNdsRendererPhase05TimerSpanCount++;
        phase05_start = phase05_end;
        gNdsRendererPhase05WallpaperRowCount++;
#endif
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
        previous_source_y = source_y_map_value;
        previous_dst = dst;
        preview_y_q16 += step_y;
    }
    if (out_pixel_write_count != NULL)
    {
        *out_pixel_write_count = pixel_write_count;
    }
#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererPhase05WallpaperPixelWriteCount += pixel_write_count;
#endif
    return TRUE;
}

static s32 ndsSObjDrawCachedWallpaperFinal(SObj *sobj, u32 combine_mode)
{
    Sprite *sprite;
    NDSRelocLoadedFile *loaded;
    u16 *overlay;
    u16 *map_scratch = sNdsSObjWallpaperMapScratch;
    u32 overlay_pitch;
    u32 overlay_width;
    u32 overlay_height;
    u32 overlay_epoch;
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
    u16 combine_palette[16];
    const u16 *palette = NULL;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif

    ndsPlatformFastWallpaperRecordSoftwareDraw();

    /* A combining wallpaper used to be refused outright, because the prim/env
     * lerp is per-pixel work that the opaque cache has no way to represent.
     * It does now: the lerp only ever reads the source intensity, so for I/4b
     * the whole combine collapses to sixteen colours that can be baked into
     * the cache once. Admit `combine_mode != 0` exactly when that bake is
     * available, and keep refusing every other combining shape. */
    if ((sobj != NULL) && (combine_mode != 0u) &&
        (ndsSObjWallpaperCombinePaletteFor(sobj, &sobj->sprite,
                                           combine_palette) != FALSE))
    {
        palette = combine_palette;
        combine_mode = 0u;
    }

    if ((sobj == NULL) || (combine_mode != 0u))
    {
#if NDS_RENDERER_M3_PHASE0_PROFILE
        NDS_RENDERER_PHASE05_FINISH(
            gNdsRendererPhase05WallpaperSetupTicks, phase05_start);
#endif
        return FALSE;
    }
    sprite = &sobj->sprite;
    if ((sprite->scalex < 0.0001F) || (sprite->scaley < 0.0001F))
    {
#if NDS_RENDERER_M3_PHASE0_PROFILE
        NDS_RENDERER_PHASE05_FINISH(
            gNdsRendererPhase05WallpaperSetupTicks, phase05_start);
#endif
        return FALSE;
    }
    overlay = ndsPlatformGetOriginalSpriteOverlayLayer(
        FALSE, &overlay_pitch, &overlay_width, &overlay_height,
        &overlay_epoch);
    if ((overlay == NULL) || (overlay_width != 256u) ||
        (overlay_height != 192u) || (overlay_pitch < overlay_width))
    {
#if NDS_RENDERER_M3_PHASE0_PROFILE
        NDS_RENDERER_PHASE05_FINISH(
            gNdsRendererPhase05WallpaperSetupTicks, phase05_start);
#endif
        return FALSE;
    }
    loaded = ndsRelocFindLoadedFileContaining(
        sprite->bitmap,
        sizeof(Bitmap) * (u32)(u16)sprite->nbitmaps);
    if (ndsRelocPointerRangeInLoadedFile(
            loaded, sprite->bitmap,
            sizeof(Bitmap) * (u32)(u16)sprite->nbitmaps) == FALSE)
    {
#if NDS_RENDERER_M3_PHASE0_PROFILE
        NDS_RENDERER_PHASE05_FINISH(
            gNdsRendererPhase05WallpaperSetupTicks, phase05_start);
#endif
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
    origin_x = (s32)sobj->pos.x;
    origin_y = (s32)sobj->pos.y;
    ndsSObjApplyDreamLandWallpaperStretch(
        &origin_x, &origin_y, &scale_x_q16, &scale_y_q16, TRUE);
    if (ndsSObjGetOpaqueWallpaperCache(
            loaded, sprite, scale_x_q16, scale_y_q16,
            NDS_SOBJ_WALLPAPER_FINAL_MAP_SCRATCH_PIXELS,
            palette) == FALSE)
    {
#if NDS_RENDERER_M3_PHASE0_PROFILE
        NDS_RENDERER_PHASE05_FINISH(
            gNdsRendererPhase05WallpaperSetupTicks, phase05_start);
#endif
        return FALSE;
    }

    draw_start = cpuGetTiming();
    if (palette != NULL)
    {
        /* Full-bleed the Results wallpaper. `ndsSObjDrawOpaqueWallpaperFinal`
         * maps each of the 256 overlay columns into the 320-wide preview space
         * (`preview_x = 1.25x`) and drops any column falling outside
         * [origin_x, origin_x + width*scale). Dream Land sits at (0,0) so it
         * covers every column; the Results wallpaper sits at (10,10), which
         * leaves preview_x < 10 and >= 310 unmapped -- measured as an 8-pixel
         * backdrop frame on all four sides, since 10/1.25 = 8.
         *
         * That letter-box is what the mapper is specified to do, and it is not
         * what the software compositor shows: it consumes the offset by
         * cropping to the content, so the source covers the screen. Match that
         * by mapping the whole preview onto the whole source -- origin 0, and a
         * scale of preview/source per axis. Both land above 1<<16, which the
         * cache's own last-writer precondition requires. */
        origin_x = 0;
        origin_y = 0;
        scale_x_q16 = (320u << 16) / sNdsSObjWallpaperDecodeCache.width;
        scale_y_q16 = (240u << 16) / sNdsSObjWallpaperDecodeCache.height;
    }
    if (ndsSObjWallpaperFinalKeyMatches(
            loaded, overlay_epoch, origin_x, origin_y,
            scale_x_q16, scale_y_q16, combine_mode) != FALSE)
    {
        gNdsSObjWallpaperFinalDirectCount++;
        gNdsSObjWallpaperFinalSkipCount++;
        gNdsSObjWallpaperCacheFastDrawCount++;
        ndsSObjWallpaperPublishDrawTicks(draw_start);
#if NDS_RENDERER_M3_PHASE0_PROFILE
        NDS_RENDERER_PHASE05_FINISH(
            gNdsRendererPhase05WallpaperSetupTicks, phase05_start);
#endif
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
#if NDS_RENDERER_M3_PHASE0_PROFILE
    NDS_RENDERER_PHASE05_FINISH(
        gNdsRendererPhase05WallpaperSetupTicks, phase05_start);
#endif
    if (ndsSObjDrawOpaqueWallpaperFinal(
            sprite,
            (sNdsSObjWallpaperDecodeCache.combine_baked != 0u) ?
                sNdsSObjWallpaperDecodeCache.combine_palette : NULL,
            map_scratch,
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
#if NDS_RENDERER_M3_PHASE0_PROFILE
    phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif
    committed_epoch = ndsPlatformCommitOriginalSpriteFinalLayer(
        FALSE, pixel_count);
    if (committed_epoch == 0u)
    {
        sNdsSObjWallpaperFinalCache.valid = FALSE;
        ndsSObjWallpaperPublishDrawTicks(draw_start);
#if NDS_RENDERER_M3_PHASE0_PROFILE
        NDS_RENDERER_PHASE05_FINISH(
            gNdsRendererPhase05WallpaperCommitTicks, phase05_start);
#endif
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
#if NDS_RENDERER_M3_PHASE0_PROFILE
    NDS_RENDERER_PHASE05_FINISH(
        gNdsRendererPhase05WallpaperCommitTicks, phase05_start);
#endif
    return TRUE;
}

/* N64 tile masks repeat in powers of two; the following period mirrors when
 * G_TX_MIRROR is set. Callers validate the positive physical extent first. */
static u32 ndsSObjMapTexel(u32 coordinate, u32 mode, u32 mask, u32 extent)
{
    u32 period;
    u32 texel;

    if ((mode & 2u) != 0u)
    {
        return (coordinate < extent) ? coordinate : extent - 1u;
    }
    if ((mask == 0u) || (mask > 15u))
    {
        return coordinate;
    }
    period = 1u << mask;
    texel = coordinate & (period - 1u);
    return (((mode & 1u) != 0u) && ((coordinate & period) != 0u)) ?
        period - 1u - texel : texel;
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
    u32 draw_width;
    u32 draw_height;
    u32 bitmap_count;
    u32 bitmap_index;
    u32 out_y = 0;
    u32 drawn_pixels = 0;
    u32 is_texshuf;
    u32 is_scaled;
    u32 scale_x_q16;
    u32 scale_y_q16;
    /* R2-07 R0e/R2a. `ndsSpriteLerpPrimEnv` is always called with a 4-bit
     * nibble scaled by 17, from two arms of the pixel loop: the I/4b wallpaper
     * under the prim/env combine, and every IA/8b sprite. Its output therefore
     * has sixteen possible values per sobj, and the sobj's prim/env colours are
     * fixed for the whole call -- so one table built once replaces ~45 Thumb
     * instructions per pixel. Proven in check_sprite_lerp_exact.py.
     *
     * NULL means neither arm can use it, which is every other caller of this
     * blitter. The table deliberately does NOT fold in `sprite->alpha`: the IA
     * arm tests it and the I4 combine arm does not, so folding it would change
     * the wallpaper when alpha is zero. */
    u16 fast_lerp_palette[16];
    const u16 *fast_lerp = NULL;
    /* The I/4b paired row is legal on top of the table, under more conditions. */
    u32 fast_i4_specialized = 0u;

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
          ((sprite->bmfmt == G_IM_FMT_IA) &&
           (sprite->bmsiz == G_IM_SIZ_4b)) ||
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
    if (cache_wallpaper != 0u)
    {
        ndsSObjApplyDreamLandWallpaperStretch(
            &origin_x, &origin_y, &scale_x_q16, &scale_y_q16, TRUE);
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

    /* lbCommonPrepSObjDraw uses lrs/lrt for non-clamped rectangles. Their
     * geometry can span several repeats of one physical bitmap (menu tabs).
     * Keep physical dimensions for buffer validation and map texels separately. */
    draw_width = ((sobj->cms != 2u) && (sobj->lrs > 0)) ?
        (u32)sobj->lrs : width;
    draw_height = ((sobj->cmt != 2u) && (sobj->lrt > 0) &&
                   (bitmap_count == 1u)) ? (u32)sobj->lrt : height;

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
            sobj, loaded, sprite, preview, preview_pitch, preview_width,
            preview_height, origin_x, origin_y, scale_x_q16, scale_y_q16);
        if (drawn_pixels != 0u)
        {
            goto draw_complete;
        }
        gNdsSObjWallpaperCacheFallbackCount++;
    }

    /* Build the sixteen-entry table once per call, for either arm that lerps.
     *
     * The I/4b paired row needs more than the table: `is_scaled` false, because
     * the scaled arm writes a rectangle per source pixel rather than one pixel,
     * and a non-negative origin. The IA/8b sprites in this scene ARE scaled --
     * `mnVSResultsMakeWallpaper`'s text helper sets `scalex` and clears
     * `SP_FASTCOPY` outright (BattleShip `mnvsresults.c:1204`) -- so they keep
     * the generic loop and only swap the lerp for a lookup. That is why the row
     * flag carries its own format test rather than reusing `fast_lerp != NULL`.
     *
     * The table's condition is exactly "one of the two lerping arms can run", so
     * both may index it without a null test. `record_startup` is deliberately NOT
     * part of it -- building sixteen entries costs a startup-logo call nothing,
     * and keeping it out is what makes the table unconditionally available. It
     * gates the specialized ROW instead, which skips the per-pixel texshuf
     * sample counter the startup-logo diagnostic needs. */
    if (((sprite->bmfmt == G_IM_FMT_I) && (sprite->bmsiz == G_IM_SIZ_4b) &&
         (results_wallpaper_combine != 0u)) ||
        (sprite->bmfmt == G_IM_FMT_IA))
    {
        u32 nibble;

        for (nibble = 0u; nibble < 16u; nibble++)
        {
            u32 intensity = nibble * 17u;

            if ((sprite->bmfmt == G_IM_FMT_IA) &&
                (sprite->bmsiz == G_IM_SIZ_4b))
            {
                /* IA4 stores three intensity bits and one alpha bit. */
                u32 i3 = nibble >> 1;
                intensity = (i3 << 5) | (i3 << 2) | (i3 >> 1);
            }
            fast_lerp_palette[nibble] =
                ndsSpriteLerpPrimEnv(sobj, (u8)intensity);
        }
        fast_lerp = fast_lerp_palette;
        fast_i4_specialized =
            ((sprite->bmfmt == G_IM_FMT_I) && (record_startup == 0u) &&
             (is_scaled == FALSE) && (origin_x >= 0) &&
             (sobj->cms == 2u) && (sobj->cmt == 2u)) ? 1u : 0u;
    }

    for (bitmap_index = 0;
         (bitmap_index < bitmap_count) && (out_y < draw_height);
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
        u32 draw_columns;
        u32 row;
        size_t src_bytes;
        size_t src_row_bytes;
        u32 bytes_per_pixel = 1u;
        u32 ci_palette_ready = 0;
        u32 ci_max_index = 0;
        u32 fast_i4_row = 0u;

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
        draw_columns = ((sobj->cms != 2u) && (sobj->lrs > 0)) ?
            draw_width : src_draw_width;
        /* The generic loop skips any pixel whose destination column falls
         * outside the preview. Requiring the whole strip to land inside it
         * removes that per-pixel test without changing which pixels are
         * written; a strip that does not fit takes the generic loop. */
        fast_i4_row = ((fast_i4_specialized != 0u) &&
                       (((u32)origin_x + src_draw_width) <= preview_width)) ?
            1u : 0u;

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
        draw_rows = (bitmap_count == 1u) ? draw_height : src_height;

        for (row = 0; (row < draw_rows) && ((draw_y + row) < draw_height); row++)
        {
            u32 x;
            u32 dst_x_q16 = 0u;
            u32 source_y = draw_y + row;
            u32 sample_row = ndsSObjMapTexel(row, sobj->cmt, sobj->maskt, src_height);
            s32 dst_y_start = origin_y +
                (s32)(((u64)source_y * scale_y_q16) >> 16);
            s32 dst_y_end = origin_y +
                (s32)((((u64)(source_y + 1u) * scale_y_q16) +
                       0xffffu) >> 16);

            if ((sample_row >= src_height) || (dst_y_end <= 0) ||
                (dst_y_start >= (s32)preview_height))
            {
                continue;
            }
            if ((fast_i4_row != 0u) && (dst_y_start >= 0))
            {
                /* One destination row of the Results wallpaper. The generic
                 * loop spends about 112 Thumb instructions per source pixel
                 * here: sixteen of them walk the seven-way format chain, and
                 * the prim/env lerp is another forty-five. This emits eighteen
                 * per PAIR of pixels -- measured in the ELF, not estimated --
                 * because a 4-bit row stores both nibbles of a pair in one byte
                 * and the low nibble is always the odd column.
                 *
                 * That pairing survives SP_TEXSHUF: the odd-row swizzle is
                 * `source_x ^= 8`, which cannot touch bit 0, so columns 2k and
                 * 2k+1 still share a byte and still land hi-then-lo. It reduces
                 * to `^ 4` on the byte index. The trailing `^ 3` is the same
                 * word-order swizzle the generic arm applies. */
                const u8 *src_i4 = (const u8 *)src;
                size_t row_base = (size_t)row * src_row_bytes;
                size_t byte_xor =
                    ((is_texshuf != 0) && ((row & 1u) != 0)) ? 4u : 0u;
                u16 *dst = &preview[((u32)dst_y_start * preview_pitch) +
                                    (u32)origin_x];
                u32 pairs = src_draw_width >> 1;
                u32 pair;

                /* Two `strh`, deliberately, NOT one `str`. R0g folded the pair
                 * into a single word store -- provably identical bytes, the base
                 * is always 4-byte aligned here -- and measured **-0.06%**:
                 * 3.9974 against 4.0000 VBlanks per wallpaper call. Halving the
                 * count of main-RAM halfword stores changes nothing, so the
                 * ~1.6M ticks this call costs beyond its instruction count are
                 * NOT the store. Reverted because it was one more instruction per
                 * pair and needed a runtime alignment gate to be safe. */
                for (pair = 0u; pair < pairs; pair++)
                {
                    u8 packed = src_i4[(row_base + (pair ^ byte_xor)) ^ 3u];

                    dst[0] = fast_lerp[packed >> 4];
                    dst[1] = fast_lerp[packed & 0x0fu];
                    dst += 2;
                }
                if ((src_draw_width & 1u) != 0u)
                {
                    /* An odd width leaves one high nibble: the last column is
                     * even, so `source_x & 1` is zero and `source_x >> 1` is
                     * `pairs`. */
                    u8 packed = src_i4[(row_base + (pairs ^ byte_xor)) ^ 3u];

                    dst[0] = fast_lerp[packed >> 4];
                }
                /* Every entry of the palette has bit 15 set, so the generic
                 * loop's `color != 0` test never skips a pixel of this sprite
                 * and every column counts. */
                drawn_pixels += src_draw_width;
                continue;
            }
            for (x = 0; x < draw_columns; x++)
            {
                s32 dst_x_start = origin_x + (s32)(dst_x_q16 >> 16);
                s32 dst_x_end;
                u16 color;
                u32 sample_x = ndsSObjMapTexel(x, sobj->cms, sobj->masks, src_draw_width);

                dst_x_q16 += scale_x_q16;
                dst_x_end = origin_x +
                    (s32)((dst_x_q16 + 0xffffu) >> 16);

                if (sample_x >= src_width)
                {
                    continue;
                }
                if ((sprite->bmfmt == G_IM_FMT_RGBA) &&
                    (sprite->bmsiz == G_IM_SIZ_16b))
                {
                    color = ndsStartupLogoConvertRgba16(
                        ndsStartupLogoReadRgba16Pixel(src, src_width, sample_row, sample_x,
                                                      is_texshuf));
                }
                else if ((sprite->bmfmt == G_IM_FMT_RGBA) &&
                         (sprite->bmsiz == G_IM_SIZ_32b))
                {
                    const u32 *src_rgba32 = (const u32 *)src;
                    u32 source_x = sample_x;
                    u32 rgba;

                    if ((is_texshuf != 0) && ((sample_row & 1u) != 0))
                    {
                        source_x ^= 2u;
                    }
                    memcpy(&rgba,
                           &src_rgba32[(sample_row * src_width) + source_x],
                           sizeof(rgba));
                    color = ndsSpriteConvertRgba32(rgba);
                }
                else if ((sprite->bmfmt == G_IM_FMT_IA) &&
                         (sprite->bmsiz == G_IM_SIZ_8b))
                {
                    const u8 *src_ia = (const u8 *)src;
                    u32 source_x = sample_x;
                    size_t source_index;
                    u8 ia;

                    if ((is_texshuf != 0) && ((sample_row & 1u) != 0))
                    {
                        source_x ^= 4u;
                    }
                    source_index = ((size_t)sample_row * src_row_bytes) + source_x;
                    ia = src_ia[source_index ^ 3u];
                    /* R2a. The table holds `lerp(sobj, n * 17)` for all sixteen
                     * nibbles, so this is the same value the call produced --
                     * 255/15 == 17 exactly, no library division either way. The
                     * alpha test stays HERE rather than folded into the table,
                     * because the I4 combine arm below does not apply it.
                     * Measured owner: these glyphs were 78.2 ticks/pixel against
                     * the specialized wallpaper row's 8.6. */
                    color = (((ia & 0x0fu) != 0u) &&
                             (sprite->alpha != 0u)) ?
                        fast_lerp[ia >> 4] : 0;
                }
                else if ((sprite->bmfmt == G_IM_FMT_CI) &&
                         (sprite->bmsiz == G_IM_SIZ_8b))
                {
                    const u8 *src_ci = (const u8 *)src;
                    const u16 *palette = (const u16 *)sprite->LUT;
                    u32 source_x = sample_x;
                    size_t source_index;
                    u8 index;

                    if ((is_texshuf != 0) && ((sample_row & 1u) != 0))
                    {
                        source_x ^= 4u;
                    }
                    source_index = ((size_t)sample_row * src_row_bytes) + source_x;
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
                    u32 source_x = sample_x;
                    size_t source_index;
                    u8 packed;
                    u8 index;

                    if ((is_texshuf != 0) && ((sample_row & 1u) != 0))
                    {
                        source_x ^= 8u;
                    }
                    source_index = ((size_t)sample_row * src_row_bytes) +
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
                    u32 source_x = sample_x;
                    size_t source_index;
                    u8 intensity;

                    if ((is_texshuf != 0) && ((sample_row & 1u) != 0))
                    {
                        source_x ^= 4u;
                    }
                    source_index = ((size_t)sample_row * src_row_bytes) + source_x;
                    intensity = src_i8[source_index ^ 3u];
                    color = ((intensity != 0u) &&
                             (sprite->alpha != 0u)) ?
                        ndsSpritePackRgb15(sprite->red, sprite->green,
                                           sprite->blue) : 0;
                }
                else if (((sprite->bmfmt == G_IM_FMT_I) ||
                          (sprite->bmfmt == G_IM_FMT_IA)) &&
                         (sprite->bmsiz == G_IM_SIZ_4b))
                {
                    const u8 *src_i4 = (const u8 *)src;
                    u32 source_x = sample_x;
                    size_t source_index;
                    u8 packed;
                    u8 intensity =
                        0;

                    if ((is_texshuf != 0) && ((sample_row & 1u) != 0))
                    {
                        source_x ^= 8u;
                    }
                    source_index = ((size_t)sample_row * src_row_bytes) +
                                   (source_x >> 1);
                    packed = src_i4[source_index ^ 3u];
                    intensity = ((source_x & 1u) == 0) ?
                        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
                    if (sprite->bmfmt == G_IM_FMT_IA)
                    {
                        color = ((intensity & 1u) && sprite->alpha) ?
                            fast_lerp[intensity] : 0;
                    }
                    else if (results_wallpaper_combine != 0u)
                    {
                        /* Same table as the IA arm, and no alpha test here --
                         * this arm never had one. Reached only when the paired
                         * row above declined the strip (scaled, hanging off the
                         * preview, or a startup-logo call); the table's build
                         * condition is this branch's condition, so no null test. */
                        color = fast_lerp[intensity & 0x0fu];
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

                if ((is_texshuf != 0) && ((sample_row & 1u) != 0))
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
static SObj sNdsFastWallpaperSeedSnapshot;
static u32 sNdsFastWallpaperSeedSnapshotValid;

#if NDS_R2_RESULTS_LAYER_MEMO
/* R2-07 R4b. Skip the whole foreground layer -- staging clear, every blit, the
 * 320x240 -> 256x192 downscale and the 98,304-byte VRAM copy -- on any frame
 * whose foreground draw set is byte-identical to the one already sitting in BG
 * VRAM. Two censuses put those four stages at 41.03% (full-match ROM) and
 * 44.38% (results-lab ROM) of the Results frame, and the overlay is
 * single-buffered, so its contents persist by design -- `nds_platform.c:781-783`
 * already documents relying on that.
 *
 * WHY THE DRAWS ARE DEFERRED RATHER THAN GATED IN PLACE. Draws stream one GObj
 * at a time, and the very first one begins (and therefore clears) the staging
 * layer. A test placed at the draw site could only ever save the blits, never
 * the clear/downscale/copy that are three quarters of the cost. Buffering the
 * layer's draws and deciding once at commit is what makes all four stages
 * skippable. The list is consumed inside the same `gcDrawAll` pass that filled
 * it, so no SObj can change between record and replay.
 *
 * WHY THE FINGERPRINT IS A RAW BYTE HASH AND NOT A FIELD LIST. Enumerating the
 * blitter's inputs by hand -- attr, bmfmt, bmsiz, alpha, width/height, bmheight,
 * scalex/scaley, red/green/blue, nbitmaps, bitmap, plus pos, envcolor and the
 * cmt/cms/maskt/masks/lrs/lrt wrap state -- is exactly the kind of list that
 * goes stale the first time a field is added and then fails as a stale-pixel
 * bug rather than a build error. `SObj` places all of it contiguously from
 * `sprite` to the end of the struct; the only members before it are the alloc
 * and linked-list pointers, which cannot affect a pixel. So hash that whole
 * span. It over-covers (`user_data` is in it) and over-covering costs a
 * needless redraw, never a stale frame.
 *
 * WHAT IT DELIBERATELY DOES NOT COVER: the pixels behind `sprite.bitmap`. A
 * mutated bitmap under an unchanged pointer would be missed. Sprite bitmaps are
 * immutable loaded assets in this engine, and the affine wallpaper cache
 * already rests on the same assumption. If that ever stops being true, this
 * memo has to key on content, not on the pointer. */
#define NDS_SOBJ_LAYER_MEMO_MAX 48u

typedef struct NDSSObjLayerMemoDraw {
    SObj *sobj;
    s32 origin_x;
    s32 origin_y;
    u32 combine;
    u32 cache_wallpaper;
} NDSSObjLayerMemoDraw;

static NDSSObjLayerMemoDraw sNdsSObjLayerMemoDraws[NDS_SOBJ_LAYER_MEMO_MAX];
static u32 sNdsSObjLayerMemoCount;
static u32 sNdsSObjLayerMemoOverflowed;
static u32 sNdsSObjLayerMemoFingerprint;
static u32 sNdsSObjLayerMemoResidentFingerprint;
static u32 sNdsSObjLayerMemoResidentValid;

volatile u32 gNdsSObjLayerMemoSkipCount;
volatile u32 gNdsSObjLayerMemoRedrawCount;
volatile u32 gNdsSObjLayerMemoOverflowCount;

static u32 ndsSObjLayerMemoMix(u32 hash, u32 value)
{
    hash ^= value;
    hash *= 16777619u;
    return hash;
}

static u32 ndsSObjLayerMemoHashDraw(u32 hash, const NDSSObjLayerMemoDraw *draw)
{
    const u8 *bytes = (const u8 *)draw->sobj;
    u32 offset = (u32)offsetof(SObj, sprite);
    u32 size = (u32)sizeof(SObj);

    hash = ndsSObjLayerMemoMix(hash, (u32)draw->origin_x);
    hash = ndsSObjLayerMemoMix(hash, (u32)draw->origin_y);
    hash = ndsSObjLayerMemoMix(hash, draw->combine);
    hash = ndsSObjLayerMemoMix(hash, draw->cache_wallpaper);
    for (; offset < size; offset++)
    {
        hash = ndsSObjLayerMemoMix(hash, (u32)bytes[offset]);
    }
    return hash;
}

/* The resident image is only trustworthy while nothing else owns BG VRAM.
 * Called on scene change and whenever the layer is torn down. */
void ndsSObjLayerMemoInvalidate(void)
{
    sNdsSObjLayerMemoResidentValid = FALSE;
    sNdsSObjLayerMemoResidentFingerprint = 0u;
}
#endif

void ndsSObjFastWallpaperOfferSeed(const SObj *seed)
{
#if NDS_FAST_WALLPAPER_AFFINE
    if ((seed != NULL) &&
        (ndsPlatformFastWallpaperCanSeed() != FALSE))
    {
        sNdsFastWallpaperSeedSnapshot = *seed;
        sNdsFastWallpaperSeedSnapshot.next = NULL;
        sNdsFastWallpaperSeedSnapshot.prev = NULL;
        sNdsFastWallpaperSeedSnapshotValid = TRUE;
    }
#else
    (void)seed;
#endif
}

static u32 ndsSObjFastWallpaperFloatFinite(f32 value)
{
    u32 bits;

    memcpy(&bits, &value, sizeof(bits));
    return ((bits & 0x7f800000u) != 0x7f800000u) ? TRUE : FALSE;
}

static u32 ndsSObjFastWallpaperGetTransform(
    const SObj *wallpaper, s32 *origin_x, s32 *origin_y,
    u32 *scale_x_q16, u32 *scale_y_q16)
{
    f32 scale_x;
    f32 scale_y;

    if ((wallpaper == NULL) || (origin_x == NULL) || (origin_y == NULL) ||
        (scale_x_q16 == NULL) || (scale_y_q16 == NULL))
    {
        return FALSE;
    }
    if (((wallpaper->sprite.attr & SP_FASTCOPY) == 0u) &&
        ((ndsSObjFastWallpaperFloatFinite(
            wallpaper->sprite.scalex) == FALSE) ||
         (ndsSObjFastWallpaperFloatFinite(
            wallpaper->sprite.scaley) == FALSE)))
    {
        return FALSE;
    }
    if ((ndsSObjFastWallpaperFloatFinite(wallpaper->pos.x) == FALSE) ||
        (ndsSObjFastWallpaperFloatFinite(wallpaper->pos.y) == FALSE) ||
        (wallpaper->pos.x < -32768.0F) ||
        (wallpaper->pos.x > 32767.0F) ||
        (wallpaper->pos.y < -32768.0F) ||
        (wallpaper->pos.y > 32767.0F))
    {
        return FALSE;
    }
    if ((wallpaper->sprite.attr & SP_FASTCOPY) != 0u)
    {
        scale_x = 1.0F;
        scale_y = 1.0F;
    }
    else
    {
        scale_x = wallpaper->sprite.scalex;
        scale_y = wallpaper->sprite.scaley;
    }
    if ((scale_x < 0.0001F) || (scale_y < 0.0001F) ||
        (scale_x > 32767.0F) || (scale_y > 32767.0F))
    {
        return FALSE;
    }
    if (ndsSObjWallpaperIsResultsShape(&wallpaper->sprite) != FALSE)
    {
        /* The Results seed is already fully mapped when it is drawn. Its 300x220
         * source reaches the screen through `ndsSObjDrawCachedWallpaperFinal`'s
         * destination-driven map, which walks the 256x192 destination and pulls
         * the nearest source pixel -- so pos (10,10) and the 320x240 staging
         * layer's 0.8 downscale are both consumed while producing the pixels.
         * Handing the hardware the source transform on top of that applies it
         * twice: measured as an 8-pixel backdrop frame on all four sides,
         * 10 * 0.8, with the picture otherwise correct. The seed pixels ARE
         * screen space, so the layer transform is identity.
         *
         * This has to be here rather than at the two call sites, because the
         * per-frame `QueueTransform` retention test compares against the seed's
         * transform. If they disagreed the layer would read as moved every
         * frame and re-seed, which costs more than the software path it
         * replaces -- the failure would look like "no win" rather than a
         * visual bug. */
        *origin_x = 0;
        *origin_y = 0;
        *scale_x_q16 = 1u << 16;
        *scale_y_q16 = 1u << 16;
        return TRUE;
    }
    *origin_x = (s32)wallpaper->pos.x;
    *origin_y = (s32)wallpaper->pos.y;
    *scale_x_q16 = (u32)((scale_x * 65536.0F) + 0.5F);
    *scale_y_q16 = (u32)((scale_y * 65536.0F) + 0.5F);
    /* The seed RASTER was stretched, but the hardware affine needs only the
     * seed/live scale RATIO. K cancels analytically. Preserve the original
     * source q16 scales here so the ratio and therefore hdx/vdy are bit-exact
     * to the pre-fix path instead of suffering a second K-rounding step. The
     * centered origin still changes because that is the intended coverage fix. */
    ndsSObjApplyDreamLandWallpaperStretch(
        origin_x, origin_y, scale_x_q16, scale_y_q16, FALSE);
    return ((*scale_x_q16 != 0u) && (*scale_y_q16 != 0u)) ? TRUE : FALSE;
}

static u32 ndsSObjFastWallpaperCaptureSeed(u32 combine_mode)
{
#if NDS_FAST_WALLPAPER_AFFINE
    s32 origin_x;
    s32 origin_y;
    u32 scale_x_q16;
    u32 scale_y_q16;
    u32 asset_identity;
    u32 draw_succeeded;

    if (sNdsFastWallpaperSeedSnapshotValid == FALSE)
    {
        return FALSE;
    }
    /* A combining wallpaper is admitted only when the combine can be baked
     * into the cache's palette; `ndsSObjDrawCachedWallpaperFinal` decides that
     * from the same snapshot and refuses everything else, so ask it here
     * rather than duplicating the shape test. Refusing before `BeginSeed`
     * keeps every other combining shape on exactly its old path. */
    if (combine_mode != 0u)
    {
        u16 probe_palette[16];

        if (ndsSObjWallpaperCombinePaletteFor(
                &sNdsFastWallpaperSeedSnapshot,
                &sNdsFastWallpaperSeedSnapshot.sprite,
                probe_palette) == FALSE)
        {
            return FALSE;
        }
    }
    asset_identity = (u32)(uintptr_t)
        sNdsFastWallpaperSeedSnapshot.sprite.bitmap;
    if (ndsSObjFastWallpaperGetTransform(
            &sNdsFastWallpaperSeedSnapshot,
            &origin_x, &origin_y,
            &scale_x_q16, &scale_y_q16) == FALSE)
    {
        if (ndsPlatformFastWallpaperBeginSeed(
                0, 0, 1u << 16, 1u << 16,
                asset_identity) == FALSE)
        {
            return FALSE;
        }
        sNdsFastWallpaperSeedSnapshotValid = FALSE;
        return ndsPlatformFastWallpaperFinishSeed(FALSE);
    }
    if (ndsPlatformFastWallpaperBeginSeed(
            origin_x, origin_y, scale_x_q16, scale_y_q16,
            asset_identity) == FALSE)
    {
        return FALSE;
    }
    draw_succeeded = ndsSObjDrawCachedWallpaperFinal(
        &sNdsFastWallpaperSeedSnapshot, combine_mode);
    sNdsFastWallpaperSeedSnapshotValid = FALSE;
    return ndsPlatformFastWallpaperFinishSeed(draw_succeeded);
#else
    (void)combine_mode;
    return FALSE;
#endif
}

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
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    u32 profile_start;
#endif

    sNdsSObjFramePendingWallpaper = NULL;
    sNdsSObjFramePendingWallpaperCombine = 0u;
    if (wallpaper == NULL)
    {
        return;
    }
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
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
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    {
        u32 ticks = cpuGetTiming() - profile_start;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileWallpaperTicks += ticks;
#endif
#if NDS_TICK_HUD
        gNdsTickHudBackgroundTicks += ticks;
#endif
    }
#endif
}

#if NDS_R2_RESULTS_LAYER_MEMO
/* Replay the buffered foreground draws into a freshly begun staging layer.
 * Shared by the overflow escape hatch and the ordinary redraw, so the two can
 * never disagree about ordering. */
static void ndsSObjLayerMemoFlushBufferedDraws(void)
{
    u32 i;

    if (sNdsSObjLayerMemoCount == 0u)
    {
        return;
    }
    if (sNdsSObjFramePendingWallpaper != NULL)
    {
        ndsSObjPreviewFlushPendingWallpaperToStaging();
    }
    ndsSObjPreviewBeginStagingLayer();
    for (i = 0; i < sNdsSObjLayerMemoCount; i++)
    {
        const NDSSObjLayerMemoDraw *draw = &sNdsSObjLayerMemoDraws[i];

        if (ndsDrawSObjIntoPreview(
                draw->sobj, 0u, sNdsSObjFramePreview,
                sNdsSObjFramePreviewPitch, 320u, 240u,
                draw->origin_x, draw->origin_y,
                draw->combine, draw->cache_wallpaper) != FALSE)
        {
            sNdsSObjFramePreviewDrawCount++;
        }
    }
    sNdsSObjLayerMemoCount = 0u;
}

/* Decide the buffered foreground layer: skip it when the resident BG VRAM image
 * already came from a byte-identical draw set, otherwise replay and re-commit.
 * Returns TRUE when the layer was skipped entirely. */
static u32 ndsSObjLayerMemoResolve(void)
{
    if (sNdsSObjLayerMemoCount == 0u)
    {
        return FALSE;
    }
    if ((sNdsSObjLayerMemoResidentValid != FALSE) &&
        (sNdsSObjLayerMemoResidentFingerprint == sNdsSObjLayerMemoFingerprint))
    {
        /* The pending wallpaper still has to be resolved even though nothing
         * is drawn: the affine path consumes it, and leaving it set would make
         * the next frame believe a background draw is outstanding. */
        sNdsSObjLayerMemoCount = 0u;
        gNdsSObjLayerMemoSkipCount++;
        return TRUE;
    }
    ndsSObjLayerMemoFlushBufferedDraws();
    sNdsSObjLayerMemoResidentFingerprint = sNdsSObjLayerMemoFingerprint;
    sNdsSObjLayerMemoResidentValid = TRUE;
    gNdsSObjLayerMemoRedrawCount++;
    return FALSE;
}
#endif

static void ndsSObjPreviewCommitLayer(void)
{
#if NDS_R2_RESULTS_LAYER_MEMO
    if (ndsSObjLayerMemoResolve() != FALSE)
    {
        /* Skipped. Drop the frame's layer state without clearing, blitting,
         * downscaling or copying; BG VRAM already holds this exact image. */
        sNdsSObjFramePendingWallpaper = NULL;
        sNdsSObjFramePendingWallpaperCombine = 0u;
        sNdsSObjFramePreview = NULL;
        sNdsSObjFramePreviewPitch = 0u;
        sNdsSObjFramePreviewDrawCount = 0u;
        if (sNdsSObjFrameForeground != FALSE)
        {
            /* The overlay is still populated -- by last frame's identical
             * commit -- so `ndsSObjPreviewEndFrame` must not clear BG3. */
            sNdsSObjFrameForegroundCommitted = TRUE;
            sNdsSObjOverlayForegroundPopulated = TRUE;
        }
        return;
    }
#endif
    if (sNdsSObjFramePendingWallpaper != NULL)
    {
        s32 final_wallpaper = FALSE;

        if (sNdsSObjFrameForeground == FALSE)
        {
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
            u32 wallpaper_start = cpuGetTiming();
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
            u32 phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif
            SObj *wallpaper = sNdsSObjFramePendingWallpaper;
            s32 origin_x = 0;
            s32 origin_y = 0;
            u32 scale_x_q16 = 0u;
            u32 scale_y_q16 = 0u;
            u32 asset_identity = (u32)(uintptr_t)
                wallpaper->sprite.bitmap;
            u32 retained_wallpaper = FALSE;

            (void)ndsSObjFastWallpaperGetTransform(
                wallpaper, &origin_x, &origin_y,
                &scale_x_q16, &scale_y_q16);
            retained_wallpaper =
                ndsPlatformFastWallpaperQueueTransform(
                    origin_x, origin_y, scale_x_q16, scale_y_q16,
                    asset_identity);
            if ((retained_wallpaper == FALSE) &&
                (sNdsFastWallpaperSeedSnapshotValid == FALSE) &&
                (ndsPlatformFastWallpaperCanSeed() != FALSE))
            {
                /* An identity/generation change can invalidate the owner after
                 * neutral seed preparation. Admit this live SObj as the one
                 * conservative seed instead of reopening frame-by-frame work. */
                ndsSObjFastWallpaperOfferSeed(wallpaper);
            }
            if ((retained_wallpaper == FALSE) &&
                (ndsSObjFastWallpaperCaptureSeed(
                    sNdsSObjFramePendingWallpaperCombine) != FALSE))
            {
                retained_wallpaper =
                    ndsPlatformFastWallpaperQueueTransform(
                        origin_x, origin_y,
                        scale_x_q16, scale_y_q16,
                        asset_identity);
            }
            if ((retained_wallpaper == FALSE) &&
                (scale_x_q16 != 0u) && (scale_y_q16 != 0u))
            {
                retained_wallpaper =
                    ndsPlatformSceneWallpaperQueueTransform(
                        origin_x, origin_y,
                        scale_x_q16, scale_y_q16);
            }
            if (retained_wallpaper != FALSE)
            {
                final_wallpaper = TRUE;
#if NDS_RENDERER_M3_PHASE0_PROFILE
                NDS_RENDERER_PHASE05_FINISH(
                    gNdsRendererPhase05WallpaperSetupTicks, phase05_start);
#endif
            }
            else
            {
#if NDS_RENDERER_M3_PHASE0_PROFILE
                NDS_RENDERER_PHASE05_FINISH(
                    gNdsRendererPhase05WallpaperSetupTicks, phase05_start);
#endif
                final_wallpaper = ndsSObjDrawCachedWallpaperFinal(
                    wallpaper, sNdsSObjFramePendingWallpaperCombine);
#if NDS_RENDERER_M3_PHASE0_PROFILE
                phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif
                if (final_wallpaper != FALSE)
                {
                    ndsPlatformSceneWallpaperConfirmRaster();
                }
#if NDS_RENDERER_M3_PHASE0_PROFILE
                NDS_RENDERER_PHASE05_FINISH(
                    gNdsRendererPhase05WallpaperSetupTicks, phase05_start);
#endif
            }
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
            {
                u32 ticks = cpuGetTiming() - wallpaper_start;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                gNdsRendererProfileWallpaperTicks += ticks;
#endif
#if NDS_TICK_HUD
                gNdsTickHudBackgroundTicks += ticks;
#endif
            }
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
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    u32 foreground_start = 0u;
    u32 profile_foreground = FALSE;
#endif

    if (gobj != NULL)
    {
        foreground = (gSCManagerSceneData.scene_curr == nSCKindVSResults) ?
            ((gobj->dl_link_id != 26u) ? TRUE : FALSE) :
            ((gobj->id != nGCCommonKindWallpaper) ? TRUE : FALSE);
        cache_wallpaper =
            ((gNdsSceneManagerCurrIsBattle != 0u) &&
             (gobj->id == nGCCommonKindWallpaper) &&
             (wallpaper_combine == 0u)) ? TRUE : FALSE;
#if NDS_R2_RESULTS_AFFINE
        /* Results identifies its background by display link, not by `id`: the
         * scene builds it through `mnVSResultsMakeWallpaper`, which never sets
         * `nGCCommonKindWallpaper`. It also combines, so it is admitted here on
         * the strength of the palette bake rather than `wallpaper_combine == 0`
         * — the cache itself refuses any combining shape it cannot bake, and
         * that refusal falls back to the generic blitter. */
        if ((gSCManagerSceneData.scene_curr == nSCKindVSResults) &&
            (gobj->dl_link_id == 26u))
        {
            cache_wallpaper = TRUE;
        }
#endif
    }

    if ((foreground != FALSE) && (sNdsSObjFrameForeground == FALSE))
    {
        ndsSObjPreviewCommitLayer();
        sNdsSObjFrameForeground = TRUE;
    }
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    if ((foreground != FALSE) &&
        (gNdsSceneManagerCurrIsBattle != 0u))
    {
        profile_foreground = TRUE;
        foreground_start = cpuGetTiming();
    }
#endif

    if ((foreground != FALSE) &&
        (gNdsSceneManagerCurrIsBattle != 0u) &&
        (ndsIFCommonNativeOamDrawGObj(gobj) != FALSE))
    {
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
        if (profile_foreground != FALSE)
        {
            u32 ticks = cpuGetTiming() - foreground_start;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            gNdsRendererProfileForegroundTicks += ticks;
#endif
#if NDS_TICK_HUD
            gNdsTickHudForegroundTicks += ticks;
#endif
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
#if NDS_R2_RESULTS_LAYER_MEMO
                /* Record instead of draw while the foreground layer is being
                 * built. Only the foreground defers: the background is either
                 * the affine wallpaper (which never stages at all) or a
                 * fallback that must reach the buffer immediately, and buffering
                 * it would reorder it behind the foreground it sits under. */
                if ((foreground != FALSE) &&
                    (sNdsSObjLayerMemoOverflowed == FALSE))
                {
                    if (sNdsSObjLayerMemoCount < NDS_SOBJ_LAYER_MEMO_MAX)
                    {
                        NDSSObjLayerMemoDraw *draw =
                            &sNdsSObjLayerMemoDraws[sNdsSObjLayerMemoCount];

                        draw->sobj = sobj;
                        draw->origin_x = (s32)sobj->pos.x;
                        draw->origin_y = (s32)sobj->pos.y;
                        draw->combine = wallpaper_combine;
                        draw->cache_wallpaper = cache_wallpaper;
                        sNdsSObjLayerMemoFingerprint =
                            ndsSObjLayerMemoHashDraw(
                                sNdsSObjLayerMemoFingerprint, draw);
                        sNdsSObjLayerMemoCount++;
                        sobj = sobj->next;
                        continue;
                    }
                    /* More foreground SObjs than the buffer holds. Fall through
                     * to the immediate path and disable the memo for this
                     * frame rather than dropping a draw -- but the already
                     * buffered ones have to be flushed first or they would be
                     * painted after the ones that overflowed. */
                    sNdsSObjLayerMemoOverflowed = TRUE;
                    gNdsSObjLayerMemoOverflowCount++;
                    ndsSObjLayerMemoFlushBufferedDraws();
                }
#endif
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
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    if (profile_foreground != FALSE)
    {
        u32 ticks = cpuGetTiming() - foreground_start;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileForegroundTicks += ticks;
#endif
#if NDS_TICK_HUD
        gNdsTickHudForegroundTicks += ticks;
#endif
    }
#endif
}

void ndsSObjPreviewBeginFrame(void)
{
#if NDS_R2_RESULTS_LAYER_MEMO
    static u32 sLastSceneCurr = 0xFFFFFFFFu;

    /* A scene change hands BG VRAM to someone else, so the resident image stops
     * describing what is on screen. Invalidate before anything can be skipped
     * against it -- this is the same class of miss as the boot-scoped OAM
     * texture-name cache that guarded scene-scoped VRAM. */
    if (sLastSceneCurr != (u32)gSCManagerSceneData.scene_curr)
    {
        sLastSceneCurr = (u32)gSCManagerSceneData.scene_curr;
        ndsSObjLayerMemoInvalidate();
    }
    sNdsSObjLayerMemoCount = 0u;
    sNdsSObjLayerMemoOverflowed = FALSE;
    sNdsSObjLayerMemoFingerprint = 2166136261u;
#endif
    ndsIFCommonNativeOamBeginFrame();
    if ((gNdsSceneManagerCurrIsBattle == 0u)
#if NDS_R2_RESULTS_AFFINE
        && (gSCManagerSceneData.scene_curr != nSCKindVSResults)
#endif
        )
    {
        /* Resetting per frame would re-seed the affine layer every frame and
         * lose the whole point of owning it, so the two scenes that hold a
         * retained wallpaper are exempt. Every other scene still starts from a
         * clean layer, because it has no wallpaper to retain. */
        ndsPlatformFastWallpaperReset();
    }
    sNdsFastWallpaperSeedSnapshotValid = FALSE;
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
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    u32 profile_foreground =
        ((gNdsSceneManagerCurrIsBattle != 0u) &&
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
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    if (profile_foreground != FALSE)
    {
        u32 ticks = cpuGetTiming() - foreground_start;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileForegroundTicks += ticks;
#endif
#if NDS_TICK_HUD
        gNdsTickHudForegroundTicks += ticks;
#endif
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
         (gNdsSceneManagerCurrIsBattle != 0u)) &&
        (sNdsSObjFrameActive != FALSE))
    {
        if ((gNdsSceneManagerCurrIsBattle != 0u) &&
            (ndsIFCommonRouteGObjToLowerTextHUD(gobj) != FALSE))
        {
            /* BattleShip still runs each source display callback so timer,
             * stock, and damage state advance normally. Only its prepared
             * steady HUD composition is redirected to the DS lower text
             * backend; countdown/GO GObjs keep the original top BG3 path. */
            ndsIFCommonRecordHUDState();
            return;
        }
        if ((gNdsSceneManagerCurrIsBattle != 0u) &&
            (gobj != NULL) &&
            (gobj->id == nGCCommonKindInterface) &&
            (gobj->proc_display == lbCommonDrawSObjAttr))
        {
            gNdsIFCommonHUDTopGenericPassCount++;
        }
        ndsDrawLayeredSObjFrame(gobj, 0u);
        if (gNdsSceneManagerCurrIsBattle != 0u)
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
         (gNdsSceneManagerCurrIsBattle != 0u)) &&
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

/* P2-6 step 8 tail. SCStaffroll name/job glyph seam.
 *
 * Source (decomp sc/sccommon/scstaffroll.c) draws every staff name and job
 * glyph from raw 4-bit intensity Image blocks, not Sprite records:
 * scStaffrollInitNameAndJobDisplayLists (:2053-2102) builds one Gfx DL per
 * glyph with gDPLoadTextureBlock_4b from the Image offset (:2085, G_IM_FMT_I,
 * width padded to 16), one textured quad via gSPVertex + gSP2Triangles,
 * attached as DObj children (:1570, :1758) and drawn by
 * scStaffrollJobProcDisplay (:1499) / scStaffrollNameProcDisplay (:1513),
 * which set a PRIMITIVE-tinted XLU combine before gcDrawDObjTreeForGObj.
 * The port SObj path only accepts Sprite records
 * (lbCommonMakeSObjForGObj above, preview shape tests, wallpaper/decode
 * caches), and gcDrawDObjTreeForGObj is a draw recorder on DS, so these
 * quads have no DS expression today.
 *
 * Seam: decode each glyph block ONCE on first draw into a per-scene cache
 * keyed by Image pointer (the source decodes per DL at init, so a per-scene
 * cache matches its cost model), then blit DS halfword pixels into the
 * original-sprite staging preview exactly where the source quad lands,
 * tinted by the display proc's PRIMITIVE colour. No allocation happens on the
 * per-frame path: the pool bump-allocates only while filling, the DObj walk
 * lives in the import TU and is stack-only, and the frame coalescing below
 * reuses one staging Begin per frame. VRAM cost is zero (main-RAM staging,
 * like every other preview arm).
 *
 * Geometry preserved: source verts span [-width,+width] x [-height,+height]
 * around the DObj origin (:2065-2066) with the width x height texels stretched
 * across (:2071-2072), and sibling advance is 2*width (:1643-:1646), so the
 * blit plots a 2w x 2h rect with exact 2x nearest sampling. Tint preserved:
 * PRIM * intensity / 255 per channel (env contributes nothing, same as an
 * SObj with envcolor zero through ndsSpriteLerpPrimEnv). XLU preserved as the
 * preview compositor allows: intensity 0 texels are skipped (transparent),
 * all others land opaque -- the same coverage rule as the other I4 arms.
 * Text, order, kerning, scroll timing, and the recorder call are untouched;
 * those still run in the source procs.
 *
 * I4 byte layout mirrors the generic I4 arm of ndsDrawSObjIntoPreview: one
 * byte per texel pair, `^ 3` word-order swizzle from the O2R loader, high
 * nibble on even columns, rows of padded-width/2 bytes. The glyph loads use
 * NOMIRROR/CLAMP with no TEXSHUF, so no odd-row xor. */

#define NDS_STAFFROLL_GLYPH_SLOTS 64u
/* Sum of width*height over the 56 staged NameAndJob rows is 17,593 bytes
 * (uppers ~9,702 + lowers ~7,314 + punctuation/4 ~577, from
 * dSCStaffrollNameAndJobSpriteInfo); 17,920 leaves 327 bytes of margin. The
 * raw blocks themselves stay resident in the reloc file (offsets
 * 0x0008..0x3258, span 0x3250 = 12,880 bytes); this pool holds the decoded
 * one-byte-per-texel intensities only. */
#define NDS_STAFFROLL_GLYPH_POOL_BYTES 17920u

typedef struct NDSStaffrollGlyphEntry
{
    u32 valid;
    const void *image;
    u32 probe;
    u8 width;
    u8 height;
    u32 pool_offset;
} NDSStaffrollGlyphEntry;

static NDSStaffrollGlyphEntry sNdsStaffrollGlyphs[NDS_STAFFROLL_GLYPH_SLOTS];
/* The pool is carved from the scene's general heap on first use, not from
 * .bss: a static 17,920-byte buffer would be resident in every scene of the
 * game for a screen that runs once per campaign, and RAM is the binding P2
 * constraint. The arena resets with the scene (ndsSceneManagerEnter), so the
 * pointer is dropped by ndsStaffrollGlyphCacheInvalidate, which the import TU
 * calls on a file change, and never freed by hand. */
static u8 *sNdsStaffrollGlyphPool = NULL;
static u32 sNdsStaffrollGlyphPoolUsed;
static u32 sNdsStaffrollPreviewFrame = 0xffffffffu;
static u16 *sNdsStaffrollPreview = NULL;
static u32 sNdsStaffrollPreviewPitch = 0u;

volatile u32 gNdsStaffrollGlyphCacheBuildCount;
volatile u32 gNdsStaffrollGlyphDrawPixelCount;

void ndsStaffrollGlyphCacheInvalidate(void)
{
    u32 i;

    for (i = 0u; i < NDS_STAFFROLL_GLYPH_SLOTS; i++)
    {
        sNdsStaffrollGlyphs[i].valid = 0u;
    }
    sNdsStaffrollGlyphPoolUsed = 0u;
    sNdsStaffrollGlyphPool = NULL; /* arena memory; the scene reset reclaims it */
    sNdsStaffrollPreviewFrame = 0xffffffffu;
    sNdsStaffrollPreview = NULL;
    sNdsStaffrollPreviewPitch = 0u;
}

/* Decode-once keyed by Image pointer. A hit needs pointer equality, matching
 * dimensions, and a 4-byte content probe (a same-address reload with different
 * bytes must remiss). The image pointer is only dereferenced after the caller
 * derived it from the resident file, and the reloc range proof runs before
 * any byte is read, mirroring ndsSObjBuildWallpaperDecodeCache. */
s32 ndsStaffrollGlyphEnsure(const void *image, u32 width, u32 height,
                            u32 *out_slot)
{
    u32 stride;
    u32 row_bytes;
    u32 need;
    u32 i;
    u32 x;
    u32 y;
    const u8 *src;
    u8 *dst;
    u32 probe;
    NDSRelocLoadedFile *loaded;

    if ((image == NULL) || (out_slot == NULL) ||
        (width == 0u) || (height == 0u) ||
        (width > 32u) || (height > 32u))
    {
        return FALSE;
    }
    memcpy(&probe, image, sizeof(probe));
    for (i = 0u; i < NDS_STAFFROLL_GLYPH_SLOTS; i++)
    {
        NDSStaffrollGlyphEntry *entry = &sNdsStaffrollGlyphs[i];

        if ((entry->valid != 0u) && (entry->image == image) &&
            (entry->probe == probe) &&
            ((u32)entry->width == width) &&
            ((u32)entry->height == height))
        {
            *out_slot = i;
            return TRUE;
        }
    }
    stride = ((width + 15u) / 16u) * 16u;
    row_bytes = stride / 2u;
    need = width * height;
    loaded = ndsRelocFindLoadedFileContaining(
        image, (size_t)row_bytes * (size_t)height);
    if (ndsRelocPointerRangeInLoadedFile(
            loaded, image,
            (size_t)row_bytes * (size_t)height) == FALSE)
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_STAFFROLL_GLYPH_SLOTS; i++)
    {
        if (sNdsStaffrollGlyphs[i].valid == 0u)
        {
            break;
        }
    }
    if ((i >= NDS_STAFFROLL_GLYPH_SLOTS) ||
        ((sNdsStaffrollGlyphPoolUsed + need) >
         NDS_STAFFROLL_GLYPH_POOL_BYTES))
    {
        return FALSE;
    }
    if (sNdsStaffrollGlyphPool == NULL)
    {
        /* First glyph of the scene: one arena carve, reclaimed by the scene
         * reset. A NULL here means the credits arena is already exhausted,
         * and the glyph is refused rather than written through NULL. */
        sNdsStaffrollGlyphPool =
            (u8 *)syTaskmanMalloc(NDS_STAFFROLL_GLYPH_POOL_BYTES, 16u);
        if (sNdsStaffrollGlyphPool == NULL)
        {
            return FALSE;
        }
        sNdsStaffrollGlyphPoolUsed = 0u;
    }
    src = (const u8 *)image;
    dst = &sNdsStaffrollGlyphPool[sNdsStaffrollGlyphPoolUsed];
    for (y = 0u; y < height; y++)
    {
        for (x = 0u; x < width; x++)
        {
            size_t byte_index = ((size_t)y * (size_t)row_bytes) + (x >> 1);
            u8 packed = src[byte_index ^ 3u];
            u8 nibble = (((x & 1u) == 0u) != FALSE) ?
                (u8)(packed >> 4) : (u8)(packed & 0x0fu);

            dst[((size_t)y * (size_t)width) + x] = (u8)(nibble * 17u);
        }
    }
    sNdsStaffrollGlyphs[i].valid = TRUE;
    sNdsStaffrollGlyphs[i].image = image;
    sNdsStaffrollGlyphs[i].probe = probe;
    sNdsStaffrollGlyphs[i].width = (u8)width;
    sNdsStaffrollGlyphs[i].height = (u8)height;
    sNdsStaffrollGlyphs[i].pool_offset = sNdsStaffrollGlyphPoolUsed;
    sNdsStaffrollGlyphPoolUsed += need;
    gNdsStaffrollGlyphCacheBuildCount++;
    *out_slot = i;
    return TRUE;
}

/* One glyph quad at DObj origin (org_x, org_y): rect
 * [org_x-w, org_x+w) x [org_y-h, org_y+h), 2x nearest sampling, halfword
 * stores with preview clipping. Intensity 0 skips (XLU); otherwise
 * PRIM*intensity/255 via NDS_SPRITE_DIV255 (bound 255*255+127 = 65,152 holds,
 * same proof as ndsSpriteLerpPrimEnv) packed by ndsSpritePackRgb15. */
void ndsStaffrollGlyphBlit(u32 slot, s32 org_x, s32 org_y,
                           u8 prim_r, u8 prim_g, u8 prim_b,
                           u16 *preview, u32 preview_pitch,
                           u32 preview_width, u32 preview_height)
{
    u32 width;
    u32 height;
    const u8 *texels;
    s32 x0;
    s32 x1;
    s32 y0;
    s32 y1;
    s32 dst_x;
    s32 dst_y;

    if ((slot >= NDS_STAFFROLL_GLYPH_SLOTS) ||
        (sNdsStaffrollGlyphs[slot].valid == 0u) ||
        (preview == NULL) || (preview_pitch == 0u))
    {
        return;
    }
    width = (u32)sNdsStaffrollGlyphs[slot].width;
    height = (u32)sNdsStaffrollGlyphs[slot].height;
    texels = &sNdsStaffrollGlyphPool[sNdsStaffrollGlyphs[slot].pool_offset];
    x0 = org_x - (s32)width;
    x1 = org_x + (s32)width;
    y0 = org_y - (s32)height;
    y1 = org_y + (s32)height;
    for (dst_y = y0; dst_y < y1; dst_y++)
    {
        u32 src_y;

        if ((dst_y < 0) || (dst_y >= (s32)preview_height))
        {
            continue;
        }
        src_y = (u32)(dst_y - y0) >> 1;
        if (src_y >= height)
        {
            src_y = height - 1u;
        }
        for (dst_x = x0; dst_x < x1; dst_x++)
        {
            u32 src_x;
            u8 intensity;
            u8 red;
            u8 green;
            u8 blue;

            if ((dst_x < 0) || (dst_x >= (s32)preview_width))
            {
                continue;
            }
            src_x = (u32)(dst_x - x0) >> 1;
            if (src_x >= width)
            {
                src_x = width - 1u;
            }
            intensity = texels[(src_y * width) + src_x];
            if (intensity == 0u)
            {
                continue;
            }
            red = (u8)NDS_SPRITE_DIV255(
                ((u32)prim_r * (u32)intensity) + 127u);
            green = (u8)NDS_SPRITE_DIV255(
                ((u32)prim_g * (u32)intensity) + 127u);
            blue = (u8)NDS_SPRITE_DIV255(
                ((u32)prim_b * (u32)intensity) + 127u);
            preview[((u32)dst_y * preview_pitch) + (u32)dst_x] =
                ndsSpritePackRgb15(red, green, blue);
            gNdsStaffrollGlyphDrawPixelCount++;
        }
    }
}

/* One staging Begin per frame shared by the job and name display procs (Begin
 * clears, so each proc beginning separately would wipe the other's glyphs).
 * Every call blits; every call commits; only the first call per
 * gNdsFrameCounter begins. Relies on the counter ticking per displayed frame;
 * a stalled counter would smear scrolling glyphs instead of clearing. */
s32 ndsStaffrollFrameBegin(u16 **out_preview, u32 *out_pitch)
{
    if ((out_preview == NULL) || (out_pitch == NULL))
    {
        return FALSE;
    }
    if ((sNdsStaffrollPreviewFrame != gNdsFrameCounter) ||
        (sNdsStaffrollPreview == NULL))
    {
        sNdsStaffrollPreview = ndsPlatformBeginOriginalSpritePreview(
            320u, 240u, 0, 0, &sNdsStaffrollPreviewPitch);
        if ((sNdsStaffrollPreview == NULL) ||
            (sNdsStaffrollPreviewPitch == 0u))
        {
            sNdsStaffrollPreview = NULL;
            sNdsStaffrollPreviewPitch = 0u;
            return FALSE;
        }
        sNdsStaffrollPreviewFrame = gNdsFrameCounter;
    }
    *out_preview = sNdsStaffrollPreview;
    *out_pitch = sNdsStaffrollPreviewPitch;
    return TRUE;
}

void ndsStaffrollFrameCommit(void)
{
    if (sNdsStaffrollPreview != NULL)
    {
        ndsPlatformCommitOriginalSpritePreview();
    }
}
