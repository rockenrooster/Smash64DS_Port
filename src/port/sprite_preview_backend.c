#include <lb/lbfade_ds.h>
#include <nds/nds_native_wallpaper.h>
#include <nds/nds_renderer.h>
#include <nds/nds_results_oam.h>

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

/* Logo RGBA16/TEXSHUF converters moved host-only with the rasterizer
 * (src/host/graphics_reference/sprite_reference.c). */

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

/* Native-only explicit failure for required sprites with no native program.
 * The first record sticks across scenes
 * (`src/nds/nds_renderer_dispatch_profile.c`); the count still itemizes every
 * site. Intentional hidden/offscreen SObjs never reach here (callers skip
 * them silently); everything reaching here is a loud failure, never a
 * successful empty draw. */
static void ndsSObjRecordSpriteFailure(const GObj *gobj, const SObj *sobj,
                                       u32 reason)
{
    u32 scene = (u32)gSCManagerSceneData.scene_curr;
    u32 identity = (gobj != NULL) ?
        (((u32)gobj->id << 16) | ((u32)gobj->dl_link_id & 0xffffu)) :
        0xffffffffu;
    u32 status = 0u;
    u32 root = 0u;
    u32 material = 0u;

    if (sobj != NULL)
    {
        status = (((u32)sobj->sprite.bmfmt << 16) |
                  (u32)sobj->sprite.bmsiz);
        root = (u32)(uintptr_t)sobj->sprite.bitmap;
        material = (u32)(uintptr_t)sobj->sprite.LUT;
    }
    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_SPRITE, scene, identity,
                                   status, root, material, reason);
}

static u32 ndsSObjNativeFailureReasonFor(SObj *sobj)
{
    return (ndsSObjPreviewBasicSupported(sobj) != FALSE) ?
        NDS_NATIVE_FAILURE_NO_PROGRAM : NDS_NATIVE_FAILURE_BAD_ASSET;
}

/* Intentional offscreen stays silent: a fully clipped SObj draws nothing by
 * source semantics too, so there is no missing-renderer failure to record.
 * Only FASTCOPY has an unscaled extent. Other transforms stay visible to
 * admission so a stretched or flipped sprite cannot be silently discarded. */
static u32 ndsSObjFastWallpaperFloatFinite(f32 value);

static s32 ndsSObjIsFullyOffscreen320x240(const SObj *sobj)
{
    s32 width;
    s32 height;

    if (sobj == NULL)
    {
        return TRUE;
    }
    if (((sobj->sprite.attr & SP_FASTCOPY) == 0u) ||
        (ndsSObjFastWallpaperFloatFinite(sobj->pos.x) == FALSE) ||
        (ndsSObjFastWallpaperFloatFinite(sobj->pos.y) == FALSE))
    {
        return FALSE;
    }
    width = (s32)(u16)sobj->sprite.width;
    height = (s32)(u16)sobj->sprite.height;
    return (((sobj->pos.x + width) <= 0.0F) || (sobj->pos.x >= 320.0F) ||
            ((sobj->pos.y + height) <= 0.0F) || (sobj->pos.y >= 240.0F)) ?
        TRUE : FALSE;
}

/* Software-rasterizer wallpaper state moved host-only with the rasterizer
 * (src/host/graphics_reference/sprite_reference.c: decode cache, final-map
 * scratch, source row, final cache, mapping-version macros). The ROM keeps
 * only the debugger-visible counters below; the live wallpaper path is the
 * converted native asset copy in src/nds/nds_native_wallpaper.c. */

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

/* Wallpaper strip fingerprinting moved host-only with the rasterizer
 * (src/host/graphics_reference/sprite_reference.c). */

/* `combine_palette` is NULL for the RGBA/16b battle wallpaper and sixteen baked
 * entries for the I/4b Results wallpaper. Rather than expanding either asset to
 * a retained 300x220 RGB555 duplicate, validate every source strip once and run
 * the old decode over one scratch row at a time to prove final opacity. */

/* Destination-driven wallpaper mapping moved host-only with the rasterizer
 * (src/host/graphics_reference/sprite_reference.c). */

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

/* Every stage's source wallpaper deliberately leaves the outer ~10 preview
 * pixels uncovered at the 1.004 camera-scale floor of the shared pan/zoom law
 * (grwallpaper.c:76-107, grWallpaperMakeCommon); that was hidden by N64
 * overscan but the DS presents the full 256x192 image. Grow the presentation
 * transform by 9/8 (1.125) about the 320x240 preview centre. Keep this a pure
 * port-side presentation correction: the source SObj/camera state is not
 * changed, and results/menus keep their exact source transform. It was
 * Dream Land-only until the owner saw the same edges on Castle, Congo,
 * Hyrule and Zebes (docs/BUGS.md, 2026-09-07); stages with wider camera
 * bounds than Dream Land pan further, so they expose the floor sooner.
 *
 * This helper uses only fixed integer arithmetic. The source scale is
 * clamped to [1.004, 2.0], and the origin input is already constrained to the
 * signed 16-bit range by the affine caller. */
/* Bisect bit (gdb): 0 restores the Dream Land-only stretch. */
volatile u32 gNdsSObjWallpaperStretchAllStages
    __attribute__((section(".data"), aligned(32))) = 1u;

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
        ((gNdsSObjWallpaperStretchAllStages == 0u) &&
         (gSCManagerBattleState->gkind != nGRKindPupupu)))
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

/* Opaque-cache admission and staging draw moved host-only with the
 * rasterizer (src/host/graphics_reference/sprite_reference.c). The live
 * wallpaper path below calls the converted native asset copy instead. */

/* Final-map keying moved host-only with the rasterizer
 * (src/host/graphics_reference/sprite_reference.c). */

/* Destination-driven mapper moved host-only with the rasterizer
 * (src/host/graphics_reference/sprite_reference.c). */

static u32 ndsSObjFastWallpaperGetTransform(
    const SObj *wallpaper, s32 *origin_x, s32 *origin_y,
    u32 *scale_x_q16, u32 *scale_y_q16);

static s32 ndsSObjDrawCachedWallpaperFinal(SObj *sobj, u32 combine_mode)
{
    Sprite *sprite;
    u32 asset_id;
    u32 bitmap_offset;
    u16 combine_palette[16];
    const u16 *palette = NULL;
    s32 origin_x;
    s32 origin_y;
    u32 scale_x_q16;
    u32 scale_y_q16;
    u32 draw_start = cpuGetTiming();

    /* Native-only wallpaper. The source-strip decode cache this function used
     * to fill is host-only now
     * (src/host/graphics_reference/sprite_reference.c). The live wallpaper
     * reaches the screen only as a converted native image:
     * `ndsNativeWallpaperDraw` copies the provider asset into the overlay BG
     * (via the existing platform layer APIs) and the commit layer owns the
     * affine transform. Anything else records an explicit sprite failure,
     * never a silent empty draw. */

    if (sobj == NULL)
    {
        return FALSE;
    }

    /* A combining wallpaper is admitted only when the combine bakes into the
     * sixteen palette entries (Results I/4b). Every other combining shape is
     * refused loudly. */
    if (combine_mode != 0u)
    {
        if (ndsSObjWallpaperCombinePaletteFor(sobj, &sobj->sprite,
                                              combine_palette) == FALSE)
        {
            ndsSObjRecordSpriteFailure(NULL, sobj,
                                       NDS_NATIVE_FAILURE_REJECTED_PROGRAM);
            return FALSE;
        }
        palette = combine_palette;
    }

    sprite = &sobj->sprite;
    if ((ndsSObjFastWallpaperGetTransform(sobj, &origin_x, &origin_y,
                                         &scale_x_q16, &scale_y_q16) == FALSE) ||
        (ndsRelocGetLoadedPointerProvenance(sprite->bitmap, &asset_id,
                                           &bitmap_offset) == FALSE))
    {
        ndsSObjRecordSpriteFailure(NULL, sobj, NDS_NATIVE_FAILURE_BAD_ASSET);
        return FALSE;
    }
    if (palette == NULL)
    {
        /* Battle wallpapers keep the presentation stretch; the Results
         * wallpaper keeps its exact source transform (the provider
         * full-bleeds it onto the overlay itself). */
        ndsSObjApplyDreamLandWallpaperStretch(
            &origin_x, &origin_y, &scale_x_q16, &scale_y_q16, TRUE);
    }
    if (ndsNativeWallpaperDraw(asset_id, bitmap_offset, origin_x, origin_y,
                               scale_x_q16, scale_y_q16,
                               palette) == FALSE)
    {
        /* No converted asset registered for this wallpaper yet (Main wires
         * the generator output into the provider). Loud first-failure, not
         * an empty draw. */
        ndsSObjRecordSpriteFailure(NULL, sobj,
                                   NDS_NATIVE_FAILURE_NO_PROGRAM);
        return FALSE;
    }
    gNdsSObjWallpaperCacheFastDrawCount++;
    gNdsSObjWallpaperCacheDrawTicks = cpuGetTiming() - draw_start;
    if (gNdsSObjWallpaperCacheDrawTicks == 0u)
    {
        gNdsSObjWallpaperCacheDrawTicks = 1u;
    }
    return TRUE;
}

/* Software SObj rasterizer moved host-only
 * (src/host/graphics_reference/sprite_reference.c: `ndsDrawSObjIntoPreview`
 * plus its wallpaper decode/cache cluster). ROM entry points below either
 * dispatch to native OAM/converted-image paths or record an explicit
 * `NDS_NATIVE_FAILURE_SPRITE` failure. */
static s32 ndsDrawSObjPreview(SObj *sobj, u32 record_startup)
{
    if (sobj == NULL)
    {
        ndsRecordSObjDrawBlocker(record_startup,
                                NDS_STARTUP_LOGO_BLOCKER_NO_SOBJ);
        return FALSE;
    }
    if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
    {
        return TRUE;
    }
    ndsSObjRecordSpriteFailure(NULL, sobj, ndsSObjNativeFailureReasonFor(sobj));
    return FALSE;
}

static u32 sNdsSObjFrameForeground;
static u32 sNdsSObjFrameActive;
static SObj *sNdsSObjFramePendingWallpaper;
static SObj sNdsSObjFramePendingWallpaperSnapshot;
static u32 sNdsSObjFramePendingWallpaperCombine;

/* The old layer memo replayed source sprites into a software framebuffer.
 * Native BG residency is owned by the converted wallpaper provider. */
void ndsSObjLayerMemoInvalidate(void)
{
    ndsNativeWallpaperInvalidate();
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
    *origin_x = (s32)wallpaper->pos.x;
    *origin_y = (s32)wallpaper->pos.y;
    *scale_x_q16 = (u32)((scale_x * 65536.0F) + 0.5F);
    *scale_y_q16 = (u32)((scale_y * 65536.0F) + 0.5F);
    return ((*scale_x_q16 != 0u) && (*scale_y_q16 != 0u)) ? TRUE : FALSE;
}

static void ndsSObjPreviewCommitLayer(void)
{
    if (sNdsSObjFramePendingWallpaper != NULL)
    {
        (void)ndsSObjDrawCachedWallpaperFinal(sNdsSObjFramePendingWallpaper,
                                             sNdsSObjFramePendingWallpaperCombine);
        sNdsSObjFramePendingWallpaper = NULL;
        sNdsSObjFramePendingWallpaperCombine = 0u;
    }
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
         * and an unsupported shape records native failure. */
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
    if ((foreground != FALSE) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSResults) &&
        (ndsResultsOamDrawGObj(gobj) != FALSE))
    {
        return;
    }

    while (sobj != NULL)
    {
        if ((sobj->sprite.attr & SP_HIDDEN) == 0)
        {
            if ((cache_wallpaper != FALSE) && (foreground == FALSE) &&
                (sNdsSObjFramePendingWallpaper == NULL))
            {
                /* Snapshot the live source transform until the background
                 * layer commits. Another visible background object needs its
                 * own native owner and is reported below. */
                sNdsSObjFramePendingWallpaperSnapshot = *sobj;
                sNdsSObjFramePendingWallpaperSnapshot.next = NULL;
                sNdsSObjFramePendingWallpaperSnapshot.prev = NULL;
                sNdsSObjFramePendingWallpaper =
                    &sNdsSObjFramePendingWallpaperSnapshot;
                sNdsSObjFramePendingWallpaperCombine = wallpaper_combine;
            }
            else if (ndsSObjIsFullyOffscreen320x240(sobj) == FALSE)
            {
                ndsSObjRecordSpriteFailure(gobj, sobj,
                                           ndsSObjNativeFailureReasonFor(sobj));
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

/* Sink state (body at EOF). The drain mark must precede
 * ndsSObjPreviewBeginFrame, which resets it. */
static Gfx *sNdsMenuFillDrainMark = NULL;

volatile u32 gNdsMenuFillRectCount;
volatile u32 gNdsMenuFillPixelCount;

void ndsSObjPreviewBeginFrame(void)
{
    static u32 sLastSceneCurr = 0xffffffffu;

    if (sLastSceneCurr != (u32)gSCManagerSceneData.scene_curr)
    {
        sLastSceneCurr = (u32)gSCManagerSceneData.scene_curr;
        ndsSObjLayerMemoInvalidate();
    }
    ndsIFCommonNativeOamBeginFrame();
    ndsResultsOamBeginFrame();
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
    sNdsSObjFrameForeground = FALSE;
    sNdsSObjFrameActive = TRUE;
    sNdsSObjFramePendingWallpaper = NULL;
    sNdsSObjFramePendingWallpaperCombine = 0u;
    /* Drop last frame's published fade: the fade GObj ejects after
     * fade_length+2 ticks and its display proc stops publishing, so without
     * this a stale frame would repaint forever. Display procs run after this
     * point, so the fresh frame (if any) is published after the discard. */
    ndsLBFadeDiscardFrame();
    /* Fresh DL baseline every frame; the drain re-marks on invalid spans. */
    sNdsMenuFillDrainMark = gSYTaskmanDLHeads[0];
}

/* Native-only menu admission; the old fill compositor lives host-only. */
static u32 ndsMenuFillSinkSceneGated(void);
static void ndsMenuFillSinkEndFrame(void);
static u32 ndsMenuFillSinkDrawSObj(SObj *sobj);

void ndsSObjPreviewEndFrame(void)
{
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    u32 profile_foreground =
        ((gNdsSceneManagerCurrIsBattle != 0u) &&
         (sNdsSObjFrameForeground != FALSE)) ? TRUE : FALSE;
    u32 foreground_start =
        (profile_foreground != FALSE) ? cpuGetTiming() : 0u;
#endif

    /* Diagnose any remaining source-only menu graphics before commit. */
    ndsMenuFillSinkEndFrame();
    ndsSObjPreviewCommitLayer();
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
    sNdsSObjFrameForeground = FALSE;
    sNdsSObjFrameActive = FALSE;
    sNdsSObjFramePendingWallpaper = NULL;
    sNdsSObjFramePendingWallpaperCombine = 0u;
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
            /* MENU scenes share one staging so FILLRECTs and SObjs keep
             * source display order; anywhere else keeps its own preview. */
            if ((record_startup == 0u) &&
                (ndsMenuFillSinkDrawSObj(sobj) != FALSE))
            {
                sobj = sobj->next;
                continue;
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

/* lbFade ownership lives in src/import/battleship_lbfade.c (source-exact
 * lifecycle: update timing, proceed/eject, display alpha; no RDP words, so
 * this sink never folds a fade word). The single final application is
 * hardware: ndsPlatformEndFrame pushes the published frame to MASTER_BRIGHT
 * via src/port/video_blackout.c, after all draws. The create count is
 * gNdsLBFadeCreateCount (<lb/lbfade_ds.h>); BeginFrame discards the stale
 * latch so an ejected fade cannot repaint. */

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

/* Campaign remains paused. Its unfinished software glyph path is preserved
 * host-only; no new ROM may carry that compositor while native work is parked. */
void ndsStaffrollGlyphCacheInvalidate(void)
{
    /* No native glyph residency has been admitted yet. */
}

s32 ndsStaffrollGlyphEnsure(const void *image, u32 width, u32 height,
                            u32 *out_slot)
{
    if (out_slot != NULL) { *out_slot = 0xffffffffu; }
    ndsRendererRecordNativeFailure(
        NDS_NATIVE_FAILURE_SPRITE, (u32)gSCManagerSceneData.scene_curr,
        0xffffffffu, (width << 16) | (height & 0xffffu),
        (u32)(uintptr_t)image, 0u, NDS_NATIVE_FAILURE_NO_PROGRAM);
    return FALSE;
}

void ndsStaffrollGlyphBlit(u32 slot, s32 org_x, s32 org_y,
                           u8 prim_r, u8 prim_g, u8 prim_b,
                           u16 *preview, u32 preview_pitch,
                           u32 preview_width, u32 preview_height)
{
    (void)org_x; (void)org_y;
    (void)prim_r; (void)prim_g; (void)prim_b;
    (void)preview_pitch; (void)preview_width; (void)preview_height;
    ndsRendererRecordNativeFailure(
        NDS_NATIVE_FAILURE_SPRITE, (u32)gSCManagerSceneData.scene_curr,
        slot, 0u, (u32)(uintptr_t)preview, 0u,
        NDS_NATIVE_FAILURE_CPU_FRAMEBUFFER);
}

s32 ndsStaffrollFrameBegin(u16 **out_preview, u32 *out_pitch)
{
    if (out_preview != NULL) { *out_preview = NULL; }
    if (out_pitch != NULL) { *out_pitch = 0u; }
    ndsRendererRecordNativeFailure(
        NDS_NATIVE_FAILURE_SPRITE, (u32)gSCManagerSceneData.scene_curr,
        0xffffffffu, 0u, 0u, 0u, NDS_NATIVE_FAILURE_NO_PROGRAM);
    return FALSE;
}

void ndsStaffrollFrameCommit(void)
{
    ndsRendererRecordNativeFailure(
        NDS_NATIVE_FAILURE_SPRITE, (u32)gSCManagerSceneData.scene_curr,
        0xffffffffu, 0u, 0u, 0u, NDS_NATIVE_FAILURE_CPU_FRAMEBUFFER);
}

/* The sink body. Runs only under ndsMenuFillSinkSceneGated; every other
 * scene never reaches past the gate, so battle/results/startup behaviour is
 * unchanged by construction. */

static u32 ndsMenuFillSinkSceneGated(void)
{
    const NdsSceneDesc *desc;

    if (sNdsSObjFrameActive == FALSE)
    {
        return FALSE;
    }
    if (gNdsSceneManagerCurrIsBattle != 0u)
    {
        return FALSE;
    }
    desc = ndsSceneManagerFind((u32)gSCManagerSceneData.scene_curr);
    if (desc == NULL)
    {
        return FALSE;
    }
    return ((desc->flags & NDS_SCENE_FLAG_MENU) != 0u) ? TRUE : FALSE;
}

/* A live menu that still emits source graphics needs a native owner.
 * Do not interpret the words or report a successful empty composition. */
static void ndsMenuFillSinkEndFrame(void)
{
    if ((ndsMenuFillSinkSceneGated() != FALSE) &&
        (sNdsMenuFillDrainMark != gSYTaskmanDLHeads[0]))
    {
        ndsRendererRecordNativeFailure(
            NDS_NATIVE_FAILURE_SPRITE, (u32)gSCManagerSceneData.scene_curr,
            0xffffffffu, 0u, (u32)(uintptr_t)sNdsMenuFillDrainMark, 0u,
            NDS_NATIVE_FAILURE_NO_PROGRAM);
    }
}

/* Gated menu SObj draw: drain earlier procs' fills first, then share the
 * frame staging at source position so fills and sprites keep display order
 * on one layer. TRUE means handled (caller skips its own preview).
 * 640x480 scenes draw through a value snapshot with pos and scale mapped;
 * the live SObj is never mutated. */
static u32 ndsMenuFillSinkDrawSObj(SObj *sobj)
{
    if ((ndsMenuFillSinkSceneGated() == FALSE) || (sobj == NULL))
    {
        return FALSE;
    }
    if ((sobj->sprite.attr & SP_HIDDEN) == 0u)
    {
        ndsSObjRecordSpriteFailure(NULL, sobj,
                                   ndsSObjNativeFailureReasonFor(sobj));
    }
    return TRUE;
}
