#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nds/arm9/sprite.h>
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <PR/sp.h>
#include <sys/vector.h>

#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/nds_results_oam.h>
#include <sc/scene.h>
#include <sys/obj.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#define NDS_RESULTS_SRC_ORIGIN_X 10
#define NDS_RESULTS_SRC_ORIGIN_Y 10
#define NDS_RESULTS_SCALE_X_Q16 55924u
#define NDS_RESULTS_SCALE_Y_Q16 57195u
#define NDS_RESULTS_OBJ_GFX_ALIGNMENT 128u
#define NDS_RESULTS_OBJ_VRAM_BYTES (64u * 1024u)
#define NDS_RESULTS_CELL_SLOTS 48u
#define NDS_RESULTS_PALETTE_BANKS 16u
#define NDS_RESULTS_MAX_TILES 4u
#define NDS_RESULTS_FILL_CELL_WIDTH 32u
#define NDS_RESULTS_FILL_CELL_HEIGHT 8u
#define NDS_RESULTS_TINT_CELL_WIDTH 64u
#define NDS_RESULTS_TINT_CELL_HEIGHT 64u

_Static_assert(NDS_RESULTS_OBJ_GFX_ALIGNMENT == 128u,
               "Results OBJ gfx alignment drifted");

typedef struct NDSResultsOamShape
{
    u8 width;
    u8 height;
    SpriteSize size;
} NDSResultsOamShape;

typedef struct NDSResultsOamTilePlan
{
    u8 count;
    NDSResultsOamShape shape[NDS_RESULTS_MAX_TILES];
    u16 x[NDS_RESULTS_MAX_TILES];
} NDSResultsOamTilePlan;

typedef struct NDSResultsOamSource
{
    const Sprite *sprite;
    const void *file_data;
    size_t file_size;
    const void *lut_data;
    size_t lut_size;
    u32 asset_id;
    u32 bitmap_offset;
    u32 lut_asset_id;
    u32 lut_offset;
} NDSResultsOamSource;

typedef struct NDSResultsOamCell
{
    u32 asset_id;
    u32 bitmap_offset;
    u32 lut_asset_id;
    u32 lut_offset;
    u16 source_width;
    u16 source_height;
    u16 final_width;
    u16 final_height;
    u16 tile_x;
    u8 cell_width;
    u8 cell_height;
    u8 bmfmt;
    u8 bmsiz;
    u8 color_format;
    SpriteSize size;
    u16 *gfx;
} NDSResultsOamCell;

typedef struct NDSResultsOamPalette
{
    u8 prim_r;
    u8 prim_g;
    u8 prim_b;
    u8 env_r;
    u8 env_g;
    u8 env_b;
} NDSResultsOamPalette;

static const NDSResultsOamShape sNdsResultsShapes[] =
{
    /* Wide-first order is the tie-break after fewest cells and least area. */
    { 64u, 32u, SpriteSize_64x32 },
    { 64u, 64u, SpriteSize_64x64 },
    { 32u, 8u, SpriteSize_32x8 },
    { 32u, 16u, SpriteSize_32x16 },
    { 32u, 32u, SpriteSize_32x32 },
    { 32u, 64u, SpriteSize_32x64 },
    { 16u, 8u, SpriteSize_16x8 },
    { 16u, 16u, SpriteSize_16x16 },
    { 16u, 32u, SpriteSize_16x32 },
    { 8u, 8u, SpriteSize_8x8 },
    { 8u, 16u, SpriteSize_8x16 },
    { 8u, 32u, SpriteSize_8x32 }
};

static NDSResultsOamCell sNdsResultsCells[NDS_RESULTS_CELL_SLOTS];
static NDSResultsOamPalette sNdsResultsPalettes[NDS_RESULTS_PALETTE_BANKS];
static u32 sNdsResultsIndexedScratch[(64u * 64u / 2u) / sizeof(u32)];
static u16 *sNdsResultsTintGfx;
static u16 *sNdsResultsFillGfx[NDS_RESULTS_FILL_CELL_HEIGHT];
static u32 sNdsResultsCellCount;
static u32 sNdsResultsPaletteCount;
static u32 sNdsResultsVramCursor;
static s32 sNdsResultsNextOamId = 127;
static s32 sNdsResultsPreviousNextOamId = 127;
static u32 sNdsResultsFrameNeedsCommit;
static u32 sNdsResultsActive;
static u16 sNdsResultsSavedBldCnt;
static u8 sNdsResultsFillPaletteBank;

volatile u32 gNdsResultsOamEnterCount;
volatile u32 gNdsResultsOamExitCount;
volatile u32 gNdsResultsOamBeginFrameCount;
volatile u32 gNdsResultsOamDrawGObjCount;
volatile u32 gNdsResultsOamDrawSObjCount;
volatile u32 gNdsResultsOamBakeCellCount;
volatile u32 gNdsResultsOamPaletteCount;
volatile u32 gNdsResultsOamEmitCount;
volatile u32 gNdsResultsOamCommitCount;
volatile u32 gNdsResultsOamRollbackCount;
volatile u32 gNdsResultsOamVramBytes;

volatile u32 gNdsResultsOamFailureInactive;
volatile u32 gNdsResultsOamFailureUnsupportedFormat;
volatile u32 gNdsResultsOamFailureBadProvenance;
volatile u32 gNdsResultsOamFailureTileOverflow;
volatile u32 gNdsResultsOamFailureCellSlotsFull;
volatile u32 gNdsResultsOamFailureVramFull;
volatile u32 gNdsResultsOamFailurePaletteFull;
volatile u32 gNdsResultsOamFailureOamFull;

static s32 ndsResultsRangeValid(const void *base, size_t size,
                                const void *ptr, size_t bytes)
{
    uintptr_t start;
    uintptr_t end;
    uintptr_t range_start;
    uintptr_t range_end;

    if ((base == NULL) || (ptr == NULL))
    {
        return 0;
    }
    start = (uintptr_t)base;
    end = start + size;
    range_start = (uintptr_t)ptr;
    range_end = range_start + bytes;
    if ((end < start) || (range_end < range_start))
    {
        return 0;
    }
    return ((range_start >= start) && (range_end <= end)) ? 1 : 0;
}

static u32 ndsResultsAlignVram(u32 cursor)
{
    return (cursor + (NDS_RESULTS_OBJ_GFX_ALIGNMENT - 1u)) &
           ~(NDS_RESULTS_OBJ_GFX_ALIGNMENT - 1u);
}

static u16 *ndsResultsAllocObjBytes(u32 bytes)
{
    u32 cursor = ndsResultsAlignVram(sNdsResultsVramCursor);
    u16 *gfx;

    if ((cursor & (NDS_RESULTS_OBJ_GFX_ALIGNMENT - 1u)) != 0u)
    {
        gNdsResultsOamFailureVramFull++;
        return NULL;
    }
    if ((bytes > NDS_RESULTS_OBJ_VRAM_BYTES) ||
        (cursor > (NDS_RESULTS_OBJ_VRAM_BYTES - bytes)))
    {
        gNdsResultsOamFailureVramFull++;
        return NULL;
    }
    gfx = (u16 *)((u8 *)SPRITE_GFX + cursor);
    sNdsResultsVramCursor = cursor + bytes;
    gNdsResultsOamVramBytes = sNdsResultsVramCursor;
    return gfx;
}

static u16 ndsResultsRgb15(u8 red, u8 green, u8 blue)
{
    return (u16)(0x8000u | (u16)(red >> 3) |
                 ((u16)(green >> 3) << 5) |
                 ((u16)(blue >> 3) << 10));
}

static s32 ndsResultsFindPalette(u8 prim_r, u8 prim_g, u8 prim_b,
                                 u8 env_r, u8 env_g, u8 env_b)
{
    u32 bank;

    for (bank = 0u; bank < sNdsResultsPaletteCount; bank++)
    {
        const NDSResultsOamPalette *palette = &sNdsResultsPalettes[bank];

        if ((palette->prim_r == prim_r) && (palette->prim_g == prim_g) &&
            (palette->prim_b == prim_b) && (palette->env_r == env_r) &&
            (palette->env_g == env_g) && (palette->env_b == env_b))
        {
            return (s32)bank;
        }
    }
    return -1;
}

static s32 ndsResultsAllocPalette(u8 prim_r, u8 prim_g, u8 prim_b,
                                  u8 env_r, u8 env_g, u8 env_b)
{
    s32 existing = ndsResultsFindPalette(prim_r, prim_g, prim_b,
                                         env_r, env_g, env_b);
    u32 bank;
    u32 index;

    if (existing >= 0)
    {
        return existing;
    }
    if (sNdsResultsPaletteCount >= NDS_RESULTS_PALETTE_BANKS)
    {
        gNdsResultsOamFailurePaletteFull++;
        return -1;
    }
    bank = sNdsResultsPaletteCount++;
    sNdsResultsPalettes[bank].prim_r = prim_r;
    sNdsResultsPalettes[bank].prim_g = prim_g;
    sNdsResultsPalettes[bank].prim_b = prim_b;
    sNdsResultsPalettes[bank].env_r = env_r;
    sNdsResultsPalettes[bank].env_g = env_g;
    sNdsResultsPalettes[bank].env_b = env_b;
    SPRITE_PALETTE[bank * 16u] = 0u;
    for (index = 1u; index < 16u; index++)
    {
        u32 step = index - 1u;
        u32 inv = 14u - step;
        u8 red = (u8)(((u32)env_r * inv + (u32)prim_r * step + 7u) / 14u);
        u8 green = (u8)(((u32)env_g * inv + (u32)prim_g * step + 7u) / 14u);
        u8 blue = (u8)(((u32)env_b * inv + (u32)prim_b * step + 7u) / 14u);

        SPRITE_PALETTE[(bank * 16u) + index] =
            ndsResultsRgb15(red, green, blue);
    }
    gNdsResultsOamPaletteCount = sNdsResultsPaletteCount;
    return (s32)bank;
}

static u32 ndsResultsMulQ16(u32 lhs_q16, u32 rhs_q16)
{
    u32 whole = lhs_q16 >> 16;
    u32 frac = lhs_q16 & 0xffffu;

    return (whole * rhs_q16) + (((frac * rhs_q16) + 0x8000u) >> 16);
}

static u32 ndsResultsFloatScaleQ16(f32 scale)
{
    if ((scale < 0.0001F) || (scale > 16.0F))
    {
        return 0u;
    }
    return (u32)((scale * 65536.0F) + 0.5F);
}

static s32 ndsResultsFinalDimensions(const Sprite *sprite,
                                     u32 *out_width, u32 *out_height)
{
    u32 scale_x_q16;
    u32 scale_y_q16;
    u32 combined_x_q16;
    u32 combined_y_q16;
    u32 width;
    u32 height;

    if ((sprite == NULL) || (out_width == NULL) || (out_height == NULL))
    {
        return 0;
    }
    width = (u32)(u16)sprite->width;
    height = (u32)(u16)sprite->height;
    if ((width == 0u) || (height == 0u))
    {
        return 0;
    }
    if ((sprite->attr & SP_FASTCOPY) != 0u)
    {
        scale_x_q16 = 1u << 16;
        scale_y_q16 = 1u << 16;
    }
    else
    {
        scale_x_q16 = ndsResultsFloatScaleQ16(sprite->scalex);
        scale_y_q16 = ndsResultsFloatScaleQ16(sprite->scaley);
    }
    if ((scale_x_q16 == 0u) || (scale_y_q16 == 0u))
    {
        return 0;
    }
    combined_x_q16 = ndsResultsMulQ16(scale_x_q16,
                                      NDS_RESULTS_SCALE_X_Q16);
    combined_y_q16 = ndsResultsMulQ16(scale_y_q16,
                                      NDS_RESULTS_SCALE_Y_Q16);
    *out_width = ((width * combined_x_q16) + 0x8000u) >> 16;
    *out_height = ((height * combined_y_q16) + 0x8000u) >> 16;
    if (*out_width == 0u) *out_width = 1u;
    if (*out_height == 0u) *out_height = 1u;
    return 1;
}

static void ndsResultsSearchTilePlan(u32 width, u32 height, u32 target_count,
                                     u32 depth, u32 covered, u32 area,
                                     NDSResultsOamShape chosen[],
                                     NDSResultsOamTilePlan *best,
                                     u32 *best_area)
{
    u32 i;

    if (depth == target_count)
    {
        if ((covered >= width) && (area < *best_area))
        {
            u32 x = 0u;

            best->count = (u8)target_count;
            for (i = 0u; i < target_count; i++)
            {
                best->shape[i] = chosen[i];
                best->x[i] = (u16)x;
                x += chosen[i].width;
            }
            *best_area = area;
        }
        return;
    }
    for (i = 0u; i < (u32)(sizeof(sNdsResultsShapes) /
                           sizeof(sNdsResultsShapes[0])); i++)
    {
        const NDSResultsOamShape *shape = &sNdsResultsShapes[i];

        if (shape->height < height)
        {
            continue;
        }
        chosen[depth] = *shape;
        ndsResultsSearchTilePlan(width, height, target_count, depth + 1u,
                                 covered + shape->width,
                                 area + ((u32)shape->width * shape->height),
                                 chosen, best, best_area);
    }
}

static s32 ndsResultsChooseTilePlan(u32 width, u32 height,
                                    NDSResultsOamTilePlan *plan)
{
    NDSResultsOamShape chosen[NDS_RESULTS_MAX_TILES];
    u32 count;

    if ((plan == NULL) || (width == 0u) || (height == 0u) || (height > 64u))
    {
        return 0;
    }
    memset(plan, 0, sizeof(*plan));
    for (count = 1u; count <= NDS_RESULTS_MAX_TILES; count++)
    {
        u32 best_area = 0xffffffffu;

        ndsResultsSearchTilePlan(width, height, count, 0u, 0u, 0u,
                                 chosen, plan, &best_area);
        if (plan->count != 0u)
        {
            return 1;
        }
    }
    return 0;
}

static s32 ndsResultsResolveSource(const Sprite *sprite,
                                   NDSResultsOamSource *source)
{
    const void *file_data;
    u32 file_size;
    u32 asset_id;
    u32 bitmap_offset;

    if ((sprite == NULL) || (source == NULL) || (sprite->bitmap == NULL) ||
        (sprite->nbitmaps <= 0))
    {
        gNdsResultsOamFailureBadProvenance++;
        return 0;
    }
    if (ndsRelocGetLoadedPointerProvenance(sprite->bitmap, &asset_id,
                                           &bitmap_offset) == 0)
    {
        gNdsResultsOamFailureBadProvenance++;
        return 0;
    }
    if (ndsRelocGetLoadedAssetView(asset_id, &file_data, &file_size) == 0)
    {
        gNdsResultsOamFailureBadProvenance++;
        return 0;
    }
    if (ndsResultsRangeValid(file_data, file_size, sprite->bitmap,
                             (size_t)(u16)sprite->nbitmaps * sizeof(Bitmap)) == 0)
    {
        gNdsResultsOamFailureBadProvenance++;
        return 0;
    }
    memset(source, 0, sizeof(*source));
    source->sprite = sprite;
    source->file_data = file_data;
    source->file_size = (size_t)file_size;
    source->asset_id = asset_id;
    source->bitmap_offset = bitmap_offset;
    source->lut_asset_id = 0xffffffffu;
    source->lut_offset = 0xffffffffu;
    if ((sprite->bmfmt == G_IM_FMT_CI) && (sprite->bmsiz == G_IM_SIZ_4b))
    {
        u32 lut_asset_id;
        u32 lut_offset;
        const void *lut_data;
        u32 lut_size;

        if ((sprite->LUT == NULL) ||
            (ndsRelocGetLoadedPointerProvenance(sprite->LUT, &lut_asset_id,
                                                &lut_offset) == 0) ||
            (ndsRelocGetLoadedAssetView(lut_asset_id, &lut_data, &lut_size) == 0) ||
            (ndsResultsRangeValid(lut_data, lut_size, sprite->LUT,
                                  16u * sizeof(u16)) == 0))
        {
            gNdsResultsOamFailureBadProvenance++;
            return 0;
        }
        source->lut_data = lut_data;
        source->lut_size = (size_t)lut_size;
        source->lut_asset_id = lut_asset_id;
        source->lut_offset = lut_offset;
    }
    return 1;
}

static s32 ndsResultsFindBitmap(const NDSResultsOamSource *source,
                                u32 source_x, u32 source_y,
                                const Bitmap **out_bitmap, u32 *out_local_y,
                                u32 *out_width_img, u32 *out_height)
{
    const Sprite *sprite = source->sprite;
    u32 out_y = 0u;
    u32 bitmap_index;

    for (bitmap_index = 0u;
         (bitmap_index < (u32)(u16)sprite->nbitmaps) &&
         (out_y < (u32)(u16)sprite->height);
         bitmap_index++)
    {
        const Bitmap *current = &sprite->bitmap[bitmap_index];
        u32 width = (u32)(u16)current->width;
        u32 width_img = (u32)(u16)current->width_img;
        u32 height = (u32)(u16)current->actualHeight;
        u32 advance = (u32)(u16)sprite->bmheight;

        if (width == 0u)
        {
            break;
        }
        if (width_img == 0u) width_img = width;
        if (height == 0u) height = advance;
        if (advance == 0u) advance = height;
        if ((source_y >= out_y) && (source_y < (out_y + height)) &&
            (source_x < width))
        {
            *out_bitmap = current;
            *out_local_y = source_y - out_y;
            *out_width_img = width_img;
            *out_height = height;
            return 1;
        }
        out_y += advance;
    }
    return 0;
}

static s32 ndsResultsReadIA8(const NDSResultsOamSource *source,
                             u32 source_x, u32 source_y, u8 *out_value)
{
    const Bitmap *bitmap;
    u32 local_y;
    u32 width_img;
    u32 height;
    u32 x;
    const u8 *pixels;

    if (ndsResultsFindBitmap(source, source_x, source_y, &bitmap, &local_y,
                             &width_img, &height) == 0)
    {
        return 0;
    }
    x = source_x;
    if (((source->sprite->attr & SP_TEXSHUF) != 0u) && ((local_y & 1u) != 0u))
    {
        u32 shuffled = x ^ 4u;
        if (shuffled < width_img) x = shuffled;
    }
    pixels = (const u8 *)bitmap->buf;
    if (ndsResultsRangeValid(source->file_data, source->file_size, pixels,
                             (size_t)width_img * height) == 0)
    {
        return 0;
    }
    *out_value = pixels[(((size_t)local_y * width_img) + x) ^ 3u];
    return 1;
}

static s32 ndsResultsReadPacked4(const NDSResultsOamSource *source,
                                 u32 source_x, u32 source_y, u8 *out_value)
{
    const Bitmap *bitmap;
    u32 local_y;
    u32 width_img;
    u32 height;
    u32 x;
    u32 row_bytes;
    const u8 *pixels;
    u8 packed;

    if (ndsResultsFindBitmap(source, source_x, source_y, &bitmap, &local_y,
                             &width_img, &height) == 0)
    {
        return 0;
    }
    x = source_x;
    if (((source->sprite->attr & SP_TEXSHUF) != 0u) && ((local_y & 1u) != 0u))
    {
        u32 shuffled = x ^ 8u;
        if (shuffled < width_img) x = shuffled;
    }
    row_bytes = (width_img + 1u) / 2u;
    pixels = (const u8 *)bitmap->buf;
    if (ndsResultsRangeValid(source->file_data, source->file_size, pixels,
                             (size_t)row_bytes * height) == 0)
    {
        return 0;
    }
    packed = pixels[(((size_t)local_y * row_bytes) + (x >> 1)) ^ 3u];
    *out_value = ((x & 1u) == 0u) ? (u8)(packed >> 4) :
                                      (u8)(packed & 0x0fu);
    return 1;
}

static s32 ndsResultsReadRGBA16(const NDSResultsOamSource *source,
                                u32 source_x, u32 source_y, u16 *out_value)
{
    const Bitmap *bitmap;
    u32 local_y;
    u32 width_img;
    u32 height;
    u32 x;
    const u16 *pixels;

    if (ndsResultsFindBitmap(source, source_x, source_y, &bitmap, &local_y,
                             &width_img, &height) == 0)
    {
        return 0;
    }
    x = source_x;
    if (((source->sprite->attr & SP_TEXSHUF) != 0u) && ((local_y & 1u) != 0u))
    {
        u32 shuffled = x ^ 2u;
        if (shuffled < width_img) x = shuffled;
    }
    pixels = (const u16 *)bitmap->buf;
    if (ndsResultsRangeValid(source->file_data, source->file_size, pixels,
                             (size_t)width_img * height * sizeof(u16)) == 0)
    {
        return 0;
    }
    *out_value = pixels[(((size_t)local_y * width_img) + x) ^ 1u];
    return 1;
}

static u32 ndsResultsRGBA16To8888(u16 color)
{
    u32 red5 = (color >> 11) & 31u;
    u32 green5 = (color >> 6) & 31u;
    u32 blue5 = (color >> 1) & 31u;
    u32 red = (red5 << 3) | (red5 >> 2);
    u32 green = (green5 << 3) | (green5 >> 2);
    u32 blue = (blue5 << 3) | (blue5 >> 2);
    u32 alpha = ((color & 1u) != 0u) ? 255u : 0u;

    return (red << 24) | (green << 16) | (blue << 8) | alpha;
}

static s32 ndsResultsReadRGBA8888(const NDSResultsOamSource *source,
                                  u32 source_x, u32 source_y, u32 *out_rgba)
{
    const Sprite *sprite = source->sprite;

    if ((sprite->bmfmt == G_IM_FMT_IA) && (sprite->bmsiz == G_IM_SIZ_8b))
    {
        u8 ia;
        u32 intensity;
        u32 alpha;

        if (ndsResultsReadIA8(source, source_x, source_y, &ia) == 0) return 0;
        intensity = (u32)(ia >> 4) * 17u;
        alpha = (u32)(ia & 0x0fu) * 17u;
        *out_rgba = (intensity << 24) | (intensity << 16) |
                    (intensity << 8) | alpha;
        return 1;
    }
    if ((sprite->bmfmt == G_IM_FMT_I) && (sprite->bmsiz == G_IM_SIZ_4b))
    {
        u8 intensity;
        u32 alpha;

        if (ndsResultsReadPacked4(source, source_x, source_y, &intensity) == 0)
        {
            return 0;
        }
        alpha = (u32)intensity * 17u;
        *out_rgba = 0xffffff00u | alpha;
        return 1;
    }
    if ((sprite->bmfmt == G_IM_FMT_CI) && (sprite->bmsiz == G_IM_SIZ_4b))
    {
        const u16 *palette = (const u16 *)sprite->LUT;
        u8 index;

        if (ndsResultsReadPacked4(source, source_x, source_y, &index) == 0)
        {
            return 0;
        }
        if (ndsResultsRangeValid(source->lut_data, source->lut_size, palette,
                                 16u * sizeof(u16)) == 0)
        {
            return 0;
        }
        *out_rgba = ndsResultsRGBA16To8888(palette[((u32)index) ^ 1u]);
        return 1;
    }
    if ((sprite->bmfmt == G_IM_FMT_RGBA) &&
        (sprite->bmsiz == G_IM_SIZ_16b))
    {
        u16 color;

        if (ndsResultsReadRGBA16(source, source_x, source_y, &color) == 0)
        {
            return 0;
        }
        *out_rgba = ndsResultsRGBA16To8888(color);
        return 1;
    }
    return 0;
}

static void ndsResultsBilerpPremultiplied(const u32 taps[4], u32 fx, u32 fy,
                                          u8 rgba[4])
{
    u32 weights[4];
    u32 weighted_alpha = 0u;
    u32 tap;
    u32 channel;

    weights[0] = (256u - fx) * (256u - fy);
    weights[1] = fx * (256u - fy);
    weights[2] = (256u - fx) * fy;
    weights[3] = fx * fy;
    for (tap = 0u; tap < 4u; tap++)
    {
        weighted_alpha += (taps[tap] & 0xffu) * weights[tap];
    }
    rgba[3] = (u8)((weighted_alpha + 0x8000u) >> 16);
    if (weighted_alpha == 0u)
    {
        rgba[0] = 0u;
        rgba[1] = 0u;
        rgba[2] = 0u;
        return;
    }
    for (channel = 0u; channel < 3u; channel++)
    {
        u32 shift = 24u - (channel * 8u);
        u32 weighted_premultiplied = 0u;

        for (tap = 0u; tap < 4u; tap++)
        {
            u32 color = (taps[tap] >> shift) & 0xffu;
            u32 alpha = taps[tap] & 0xffu;

            weighted_premultiplied += color * alpha * weights[tap];
        }
        rgba[channel] = (u8)((weighted_premultiplied +
                              (weighted_alpha >> 1)) / weighted_alpha);
    }
}

static s32 ndsResultsSamplePrefiltered(const NDSResultsOamSource *source,
                                       u32 final_width, u32 final_height,
                                       u32 destination_x, u32 destination_y,
                                       u8 rgba[4])
{
    u32 source_width = (u32)(u16)source->sprite->width;
    u32 source_height = (u32)(u16)source->sprite->height;
    u32 step_x_q16 = (source_width << 16) / final_width;
    u32 step_y_q16 = (source_height << 16) / final_height;
    s32 source_x_q16 = (s32)(step_x_q16 >> 1) - 0x8000 +
                       (s32)(destination_x * step_x_q16);
    s32 source_y_q16 = (s32)(step_y_q16 >> 1) - 0x8000 +
                       (s32)(destination_y * step_y_q16);
    u32 source_x_q8;
    u32 source_y_q8;
    u32 x0;
    u32 y0;
    u32 x1;
    u32 y1;
    u32 taps[4];

    if (source_x_q16 < 0) source_x_q16 = 0;
    if (source_y_q16 < 0) source_y_q16 = 0;
    source_x_q8 = (u32)source_x_q16 >> 8;
    source_y_q8 = (u32)source_y_q16 >> 8;
    x0 = source_x_q8 >> 8;
    y0 = source_y_q8 >> 8;
    if (x0 >= source_width) x0 = source_width - 1u;
    if (y0 >= source_height) y0 = source_height - 1u;
    x1 = (x0 + 1u < source_width) ? x0 + 1u : x0;
    y1 = (y0 + 1u < source_height) ? y0 + 1u : y0;
    if ((ndsResultsReadRGBA8888(source, x0, y0, &taps[0]) == 0) ||
        (ndsResultsReadRGBA8888(source, x1, y0, &taps[1]) == 0) ||
        (ndsResultsReadRGBA8888(source, x0, y1, &taps[2]) == 0) ||
        (ndsResultsReadRGBA8888(source, x1, y1, &taps[3]) == 0))
    {
        return 0;
    }
    ndsResultsBilerpPremultiplied(taps, source_x_q8 & 0xffu,
                                  source_y_q8 & 0xffu, rgba);
    return 1;
}

static s32 ndsResultsCellMatches(const NDSResultsOamCell *cell,
                                 const NDSResultsOamSource *source,
                                 u32 final_width, u32 final_height,
                                 u32 tile_x, const NDSResultsOamShape *shape)
{
    return ((cell->asset_id == source->asset_id) &&
            (cell->bitmap_offset == source->bitmap_offset) &&
            (cell->lut_asset_id == source->lut_asset_id) &&
            (cell->lut_offset == source->lut_offset) &&
            (cell->source_width == (u16)source->sprite->width) &&
            (cell->source_height == (u16)source->sprite->height) &&
            (cell->final_width == final_width) &&
            (cell->final_height == final_height) &&
            (cell->tile_x == tile_x) &&
            (cell->cell_width == shape->width) &&
            (cell->cell_height == shape->height) &&
            (cell->bmfmt == source->sprite->bmfmt) &&
            (cell->bmsiz == source->sprite->bmsiz)) ? 1 : 0;
}

static NDSResultsOamCell *ndsResultsFindCell(const NDSResultsOamSource *source,
                                             u32 final_width,
                                             u32 final_height, u32 tile_x,
                                             const NDSResultsOamShape *shape)
{
    u32 i;

    for (i = 0u; i < sNdsResultsCellCount; i++)
    {
        if (ndsResultsCellMatches(&sNdsResultsCells[i], source, final_width,
                                  final_height, tile_x, shape) != 0)
        {
            return &sNdsResultsCells[i];
        }
    }
    return NULL;
}

static s32 ndsResultsWriteIndexedCell(NDSResultsOamCell *cell,
                                      const NDSResultsOamSource *source,
                                      u32 final_width, u32 final_height)
{
    u32 bytes = ((u32)cell->cell_width * cell->cell_height) / 2u;
    u8 *scratch = (u8 *)sNdsResultsIndexedScratch;
    u32 y;
    u32 word;

    memset(scratch, 0, bytes);
    for (y = 0u; y < cell->cell_height; y++)
    {
        u32 x;

        for (x = 0u; x < cell->cell_width; x++)
        {
            u32 destination_x = (u32)cell->tile_x + x;
            u32 destination_y = y;
            u8 rgba[4];
            u8 index;
            u32 tile;
            u32 offset;

            if ((destination_x >= final_width) ||
                (destination_y >= final_height))
            {
                continue;
            }
            if (ndsResultsSamplePrefiltered(source, final_width, final_height,
                                            destination_x, destination_y,
                                            rgba) == 0)
            {
                return 0;
            }
            if (source->sprite->bmfmt == G_IM_FMT_IA)
            {
                if (rgba[3] < 128u)
                {
                    continue;
                }
                index = (u8)(((u32)rgba[0] + 8u) / 17u);
                if (index == 0u) index = 1u;
                if (index > 15u) index = 15u;
            }
            else
            {
                index = (u8)(((u32)rgba[3] + 8u) / 17u);
                if (index > 15u) index = 15u;
                if (index == 0u) continue;
            }
            tile = ((y >> 3) * ((u32)cell->cell_width >> 3)) + (x >> 3);
            offset = (tile * 32u) + ((y & 7u) * 4u) + ((x & 7u) >> 1);
            if ((x & 1u) == 0u)
            {
                scratch[offset] = (u8)((scratch[offset] & 0xf0u) | index);
            }
            else
            {
                scratch[offset] = (u8)((scratch[offset] & 0x0fu) |
                                       (u8)(index << 4));
            }
        }
    }
    for (word = 0u; word < bytes / sizeof(u32); word++)
    {
        ((u32 *)(void *)cell->gfx)[word] = sNdsResultsIndexedScratch[word];
    }
    return 1;
}

static s32 ndsResultsWriteBitmapCell(NDSResultsOamCell *cell,
                                     const NDSResultsOamSource *source,
                                     u32 final_width, u32 final_height)
{
    u32 bytes = (u32)cell->cell_width * cell->cell_height * sizeof(u16);
    u32 y;

    dmaFillHalfWords(0u, cell->gfx, bytes);
    for (y = 0u; y < cell->cell_height; y++)
    {
        u32 x;

        for (x = 0u; x < cell->cell_width; x++)
        {
            u32 destination_x = (u32)cell->tile_x + x;
            u32 destination_y = y;
            u8 rgba[4];

            if ((destination_x >= final_width) ||
                (destination_y >= final_height))
            {
                continue;
            }
            if (ndsResultsSamplePrefiltered(source, final_width, final_height,
                                            destination_x, destination_y,
                                            rgba) == 0)
            {
                return 0;
            }
            if (rgba[3] >= 128u)
            {
                cell->gfx[(y * cell->cell_width) + x] =
                    ndsResultsRgb15(rgba[0], rgba[1], rgba[2]);
            }
        }
    }
    return 1;
}

static NDSResultsOamCell *ndsResultsBakeCell(const NDSResultsOamSource *source,
                                             u32 final_width,
                                             u32 final_height, u32 tile_x,
                                             const NDSResultsOamShape *shape)
{
    NDSResultsOamCell *cell;
    u32 bytes;
    u32 direct_color;

    cell = ndsResultsFindCell(source, final_width, final_height, tile_x, shape);
    if (cell != NULL)
    {
        return cell;
    }
    if (sNdsResultsCellCount >= NDS_RESULTS_CELL_SLOTS)
    {
        gNdsResultsOamFailureCellSlotsFull++;
        return NULL;
    }
    direct_color = ((source->sprite->bmfmt == G_IM_FMT_CI) ||
                    (source->sprite->bmfmt == G_IM_FMT_RGBA)) ? 1u : 0u;
    bytes = (u32)shape->width * shape->height * (direct_color ? 2u : 1u) /
            (direct_color ? 1u : 2u);
    cell = &sNdsResultsCells[sNdsResultsCellCount];
    memset(cell, 0, sizeof(*cell));
    cell->gfx = ndsResultsAllocObjBytes(bytes);
    if (cell->gfx == NULL)
    {
        return NULL;
    }
    cell->asset_id = source->asset_id;
    cell->bitmap_offset = source->bitmap_offset;
    cell->lut_asset_id = source->lut_asset_id;
    cell->lut_offset = source->lut_offset;
    cell->source_width = (u16)source->sprite->width;
    cell->source_height = (u16)source->sprite->height;
    cell->final_width = (u16)final_width;
    cell->final_height = (u16)final_height;
    cell->tile_x = (u16)tile_x;
    cell->cell_width = shape->width;
    cell->cell_height = shape->height;
    cell->bmfmt = source->sprite->bmfmt;
    cell->bmsiz = source->sprite->bmsiz;
    cell->size = shape->size;
    cell->color_format = direct_color ? SpriteColorFormat_Bmp :
                                        SpriteColorFormat_16Color;
    if ((direct_color != 0u) ?
        (ndsResultsWriteBitmapCell(cell, source, final_width, final_height) == 0) :
        (ndsResultsWriteIndexedCell(cell, source, final_width, final_height) == 0))
    {
        gNdsResultsOamFailureBadProvenance++;
        return NULL;
    }
    sNdsResultsCellCount++;
    gNdsResultsOamBakeCellCount++;
    return cell;
}

static s32 ndsResultsSupportedFormat(const Sprite *sprite)
{
    return (((sprite->bmfmt == G_IM_FMT_IA) &&
             (sprite->bmsiz == G_IM_SIZ_8b)) ||
            ((sprite->bmfmt == G_IM_FMT_I) &&
             (sprite->bmsiz == G_IM_SIZ_4b)) ||
            ((sprite->bmfmt == G_IM_FMT_CI) &&
             (sprite->bmsiz == G_IM_SIZ_4b)) ||
            ((sprite->bmfmt == G_IM_FMT_RGBA) &&
             (sprite->bmsiz == G_IM_SIZ_16b))) ? 1 : 0;
}

static s32 ndsResultsPaletteForSObj(const SObj *sobj)
{
    const Sprite *sprite = &sobj->sprite;

    if ((sprite->bmfmt == G_IM_FMT_CI) || (sprite->bmfmt == G_IM_FMT_RGBA))
    {
        return 0;
    }
    if (sprite->bmfmt == G_IM_FMT_I)
    {
        return ndsResultsAllocPalette(sprite->red, sprite->green, sprite->blue,
                                      0u, 0u, 0u);
    }
    return ndsResultsAllocPalette(sprite->red, sprite->green, sprite->blue,
                                  sobj->envcolor.r, sobj->envcolor.g,
                                  sobj->envcolor.b);
}

static s32 ndsResultsPrepareSObj(const SObj *sobj, u32 *out_objects)
{
    NDSResultsOamSource source;
    NDSResultsOamTilePlan plan;
    u32 final_width;
    u32 final_height;
    u32 cell_count_before = sNdsResultsCellCount;
    u32 palette_count_before = sNdsResultsPaletteCount;
    u32 vram_before = sNdsResultsVramCursor;
    u32 tile;

    if (ndsResultsSupportedFormat(&sobj->sprite) == 0)
    {
        gNdsResultsOamFailureUnsupportedFormat++;
        return 0;
    }
    if (ndsResultsFinalDimensions(&sobj->sprite, &final_width, &final_height) == 0)
    {
        gNdsResultsOamFailureUnsupportedFormat++;
        return 0;
    }
    if ((final_width > 256u) || (ndsResultsChooseTilePlan(final_width,
                                                          final_height,
                                                          &plan) == 0))
    {
        gNdsResultsOamFailureTileOverflow++;
        return 0;
    }
    if (ndsResultsResolveSource(&sobj->sprite, &source) == 0)
    {
        return 0;
    }
    if (ndsResultsPaletteForSObj(sobj) < 0)
    {
        return 0;
    }
    for (tile = 0u; tile < plan.count; tile++)
    {
        if (ndsResultsBakeCell(&source, final_width, final_height,
                               plan.x[tile], &plan.shape[tile]) == NULL)
        {
            sNdsResultsCellCount = cell_count_before;
            sNdsResultsPaletteCount = palette_count_before;
            sNdsResultsVramCursor = vram_before;
            gNdsResultsOamPaletteCount = sNdsResultsPaletteCount;
            gNdsResultsOamVramBytes = sNdsResultsVramCursor;
            return 0;
        }
    }
    *out_objects += plan.count;
    return 1;
}

static s32 ndsResultsMapSourceCoord(s32 source, s32 origin, u32 scale_q16)
{
    s32 delta = source - origin;
    s32 product = delta * (s32)scale_q16;

    if (product >= 0)
    {
        return (product + 0x8000) >> 16;
    }
    return -(((-product) + 0x8000) >> 16);
}

static s32 ndsResultsRoundFloat(f32 value)
{
    return (value >= 0.0F) ? (s32)(value + 0.5F) : (s32)(value - 0.5F);
}

static void ndsResultsRollbackOam(s32 cursor_before, u32 emit_before)
{
    while (sNdsResultsNextOamId < cursor_before)
    {
        sNdsResultsNextOamId++;
        oamClear(&oamMain, sNdsResultsNextOamId, 1);
    }
    gNdsResultsOamEmitCount = emit_before;
    sNdsResultsFrameNeedsCommit = 1u;
    gNdsResultsOamRollbackCount++;
}

static s32 ndsResultsEmitSObj(const SObj *sobj)
{
    NDSResultsOamSource source;
    NDSResultsOamTilePlan plan;
    u32 final_width;
    u32 final_height;
    s32 palette_bank;
    s32 screen_x;
    s32 screen_y;
    u32 tile;

    if ((ndsResultsFinalDimensions(&sobj->sprite, &final_width, &final_height) == 0) ||
        (ndsResultsChooseTilePlan(final_width, final_height, &plan) == 0) ||
        (ndsResultsResolveSource(&sobj->sprite, &source) == 0))
    {
        return 0;
    }
    palette_bank = ndsResultsPaletteForSObj(sobj);
    if (palette_bank < 0)
    {
        return 0;
    }
    screen_x = ndsResultsMapSourceCoord(ndsResultsRoundFloat(sobj->pos.x),
                                        NDS_RESULTS_SRC_ORIGIN_X,
                                        NDS_RESULTS_SCALE_X_Q16);
    screen_y = ndsResultsMapSourceCoord(ndsResultsRoundFloat(sobj->pos.y),
                                        NDS_RESULTS_SRC_ORIGIN_Y,
                                        NDS_RESULTS_SCALE_Y_Q16);
    for (tile = 0u; tile < plan.count; tile++)
    {
        NDSResultsOamCell *cell = ndsResultsFindCell(&source, final_width,
                                                     final_height, plan.x[tile],
                                                     &plan.shape[tile]);

        if ((cell == NULL) || (sNdsResultsNextOamId < 0))
        {
            return 0;
        }
        oamSet(&oamMain, sNdsResultsNextOamId, screen_x + plan.x[tile],
               screen_y, 0, palette_bank, cell->size,
               (SpriteColorFormat)cell->color_format, cell->gfx, -1,
               false, false, false, false, false);
        sNdsResultsNextOamId--;
        gNdsResultsOamEmitCount++;
    }
    return 1;
}

static void ndsResultsRecordFillFailure(void)
{
    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_SPRITE,
        (u32)gSCManagerSceneData.scene_curr, 0xffffffffu, 0u, 0u, 0u,
        NDS_NATIVE_FAILURE_REJECTED_PROGRAM);
}

static u32 ndsResultsBitmapAlpha(u32 alpha)
{
    if (alpha == 0u)
    {
        return 0u;
    }
    if (alpha > 255u) alpha = 255u;
    return ((alpha * 15u) + 127u) / 255u;
}

static s32 ndsResultsBakeFillCell(u32 content_height, u16 **out_gfx)
{
    u16 *gfx;
    u32 bytes = (NDS_RESULTS_FILL_CELL_WIDTH * NDS_RESULTS_FILL_CELL_HEIGHT) / 2u;
    u32 word;
    u8 *scratch = (u8 *)sNdsResultsIndexedScratch;
    u32 y;

    gfx = ndsResultsAllocObjBytes(bytes);
    if (gfx == NULL)
    {
        return 0;
    }
    memset(scratch, 0, bytes);
    for (y = 0u; y < content_height; y++)
    {
        u32 x;

        for (x = 0u; x < NDS_RESULTS_FILL_CELL_WIDTH; x++)
        {
            u32 tile = ((y >> 3) * (NDS_RESULTS_FILL_CELL_WIDTH >> 3)) +
                       (x >> 3);
            u32 offset = (tile * 32u) + ((y & 7u) * 4u) + ((x & 7u) >> 1);

            if ((x & 1u) == 0u)
            {
                scratch[offset] = (u8)((scratch[offset] & 0xf0u) | 0x0fu);
            }
            else
            {
                scratch[offset] = (u8)((scratch[offset] & 0x0fu) | 0xf0u);
            }
        }
    }
    for (word = 0u; word < bytes / sizeof(u32); word++)
    {
        ((u32 *)(void *)gfx)[word] = sNdsResultsIndexedScratch[word];
    }
    *out_gfx = gfx;
    return 1;
}

void ndsResultsOamEnter(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 tint_bytes = NDS_RESULTS_TINT_CELL_WIDTH *
                     NDS_RESULTS_TINT_CELL_HEIGHT * sizeof(u16);
    u32 i;

    if (sNdsResultsActive != 0u)
    {
        ndsResultsOamExit();
    }
    memset(sNdsResultsCells, 0, sizeof(sNdsResultsCells));
    memset(sNdsResultsPalettes, 0, sizeof(sNdsResultsPalettes));
    sNdsResultsCellCount = 0u;
    sNdsResultsPaletteCount = 0u;
    sNdsResultsVramCursor = 0u;
    sNdsResultsNextOamId = 127;
    sNdsResultsPreviousNextOamId = 127;
    sNdsResultsFrameNeedsCommit = 0u;
    sNdsResultsTintGfx = NULL;
    memset(sNdsResultsFillGfx, 0, sizeof(sNdsResultsFillGfx));
    sNdsResultsSavedBldCnt = REG_BLDCNT;
    REG_BLDCNT |= BLEND_DST_BG0 | BLEND_DST_BACKDROP;
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
    oamClear(&oamMain, 0, 128);
    oamUpdate(&oamMain);
    sNdsResultsTintGfx = ndsResultsAllocObjBytes(tint_bytes);
    if (sNdsResultsTintGfx != NULL)
    {
        for (i = 0u; i < tint_bytes / sizeof(u16); i++)
        {
            sNdsResultsTintGfx[i] = 0x8000u;
        }
    }
    if (sNdsResultsTintGfx == NULL)
    {
        REG_BLDCNT = sNdsResultsSavedBldCnt;
        sNdsResultsActive = 0u;
        return;
    }
    for (i = 0u; i < NDS_RESULTS_FILL_CELL_HEIGHT; i++)
    {
        if (ndsResultsBakeFillCell(i + 1u, &sNdsResultsFillGfx[i]) == 0)
        {
            REG_BLDCNT = sNdsResultsSavedBldCnt;
            sNdsResultsActive = 0u;
            return;
        }
    }
    i = (u32)ndsResultsAllocPalette(0xffu, 0xffu, 0xffu,
                                    0xffu, 0xffu, 0xffu);
    if (i >= NDS_RESULTS_PALETTE_BANKS)
    {
        REG_BLDCNT = sNdsResultsSavedBldCnt;
        sNdsResultsActive = 0u;
        return;
    }
    sNdsResultsFillPaletteBank = (u8)i;
    oamRotateScale(&oamMain, 0, 0, 128, 128);
    sNdsResultsActive = 1u;
    gNdsResultsOamEnterCount++;
#else
    gNdsResultsOamFailureInactive++;
#endif
}

void ndsResultsOamExit(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsResultsActive == 0u)
    {
        return;
    }
    oamClear(&oamMain, 0, 128);
    oamUpdate(&oamMain);
    REG_BLDCNT = sNdsResultsSavedBldCnt;
    memset(sNdsResultsCells, 0, sizeof(sNdsResultsCells));
    sNdsResultsCellCount = 0u;
    sNdsResultsPaletteCount = 0u;
    sNdsResultsVramCursor = 0u;
    sNdsResultsNextOamId = 127;
    sNdsResultsPreviousNextOamId = 127;
    sNdsResultsFrameNeedsCommit = 0u;
    sNdsResultsTintGfx = NULL;
    memset(sNdsResultsFillGfx, 0, sizeof(sNdsResultsFillGfx));
    sNdsResultsActive = 0u;
    gNdsResultsOamVramBytes = 0u;
    gNdsResultsOamPaletteCount = 0u;
    gNdsResultsOamExitCount++;
#endif
}

s32 ndsResultsOamIsActive(void)
{
    return (sNdsResultsActive != 0u) ? 1 : 0;
}

void ndsResultsOamBeginFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    s32 first_previous;

    if (sNdsResultsActive == 0u)
    {
        return;
    }
    first_previous = sNdsResultsPreviousNextOamId + 1;
    if (first_previous < 128)
    {
        oamClear(&oamMain, first_previous, 128 - first_previous);
        sNdsResultsFrameNeedsCommit = 1u;
    }
    sNdsResultsNextOamId = 127;
    sNdsResultsPreviousNextOamId = 127;
    gNdsResultsOamBeginFrameCount++;
#endif
}

u32 ndsResultsOamBakeGObj(struct GObj *gobj)
{
#if NDS_RENDERER_HW_TRIANGLES
    SObj *sobj;
    u32 objects = 0u;

    if (sNdsResultsActive == 0u)
    {
        gNdsResultsOamFailureInactive++;
        return 0u;
    }
    if (gobj == NULL)
    {
        return 0u;
    }
    sobj = SObjGetStruct(gobj);
    if (sobj == NULL)
    {
        return 0u;
    }
    for (; sobj != NULL; sobj = sobj->next)
    {
        if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        if (ndsResultsPrepareSObj(sobj, &objects) == 0)
        {
            return 0u;
        }
    }
    return 1u;
#else
    (void)gobj;
    return 0u;
#endif
}

s32 ndsResultsOamDrawGObj(struct GObj *gobj)
{
#if NDS_RENDERER_HW_TRIANGLES
    SObj *sobj;
    SObj *scan;
    u32 required_objects = 0u;
    s32 cursor_before;
    u32 emit_before;

    if (sNdsResultsActive == 0u)
    {
        gNdsResultsOamFailureInactive++;
        return 0;
    }
    if (gobj == NULL)
    {
        return 0;
    }
    sobj = SObjGetStruct(gobj);
    if (sobj == NULL)
    {
        return 0;
    }
    for (scan = sobj; scan != NULL; scan = scan->next)
    {
        if ((scan->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        if (ndsResultsPrepareSObj(scan, &required_objects) == 0)
        {
            return 0;
        }
    }
    if (required_objects > (u32)(sNdsResultsNextOamId + 1))
    {
        gNdsResultsOamFailureOamFull++;
        return 0;
    }
    cursor_before = sNdsResultsNextOamId;
    emit_before = gNdsResultsOamEmitCount;
    for (scan = sobj; scan != NULL; scan = scan->next)
    {
        if ((scan->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        if (ndsResultsEmitSObj(scan) == 0)
        {
            ndsResultsRollbackOam(cursor_before, emit_before);
            return 0;
        }
        gNdsResultsOamDrawSObjCount++;
    }
    sNdsResultsFrameNeedsCommit = 1u;
    gNdsResultsOamDrawGObjCount++;
    return 1;
#else
    (void)gobj;
    return 0;
#endif
}

s32 ndsResultsOamEmitTintPlane(u32 source_alpha)
{
#if NDS_RENDERER_HW_TRIANGLES
    static const s16 positions[4][2] =
    {
        { 0, 0 }, { 128, 0 }, { 0, 128 }, { 128, 128 }
    };
    u32 alpha;
    u32 i;

    if (sNdsResultsActive == 0u)
    {
        gNdsResultsOamFailureInactive++;
        ndsResultsRecordFillFailure();
        return 0;
    }
    if ((sNdsResultsTintGfx == NULL) || (sNdsResultsNextOamId < 3))
    {
        gNdsResultsOamFailureOamFull++;
        ndsResultsRecordFillFailure();
        return 0;
    }
    alpha = ndsResultsBitmapAlpha(source_alpha);
    for (i = 0u; i < 4u; i++)
    {
        oamSet(&oamMain, sNdsResultsNextOamId, positions[i][0], positions[i][1],
               0, (int)alpha, SpriteSize_64x64, SpriteColorFormat_Bmp,
               sNdsResultsTintGfx, 0, true, false, false, false, false);
        sNdsResultsNextOamId--;
        gNdsResultsOamEmitCount++;
    }
    sNdsResultsFrameNeedsCommit = 1u;
    return 1;
#else
    (void)source_alpha;
    return 0;
#endif
}

s32 ndsResultsOamEmitFillRect(s32 sx0, s32 sy0, s32 sx1, s32 sy1)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 source_width;
    u32 source_height;
    u32 mapped_width;
    u32 mapped_height;
    u32 remaining;
    u32 required = 0u;
    s32 screen_x;
    s32 screen_y;
    u16 *gfx;

    if (sNdsResultsActive == 0u)
    {
        gNdsResultsOamFailureInactive++;
        ndsResultsRecordFillFailure();
        return 0;
    }
    if ((sx1 < sx0) || (sy1 < sy0))
    {
        gNdsResultsOamFailureUnsupportedFormat++;
        ndsResultsRecordFillFailure();
        return 0;
    }
    source_width = (u32)(sx1 - sx0 + 1);
    source_height = (u32)(sy1 - sy0 + 1);
    mapped_width = ((source_width * NDS_RESULTS_SCALE_X_Q16) + 0x8000u) >> 16;
    mapped_height = ((source_height * NDS_RESULTS_SCALE_Y_Q16) + 0x8000u) >> 16;
    mapped_width &= ~7u;
    if (mapped_width == 0u) mapped_width = 8u;
    if (mapped_height == 0u) mapped_height = 1u;
    if (mapped_height > NDS_RESULTS_FILL_CELL_HEIGHT)
    {
        gNdsResultsOamFailureUnsupportedFormat++;
        ndsResultsRecordFillFailure();
        return 0;
    }
    gfx = sNdsResultsFillGfx[mapped_height - 1u];
    if (gfx == NULL)
    {
        gNdsResultsOamFailureVramFull++;
        ndsResultsRecordFillFailure();
        return 0;
    }
    remaining = mapped_width;
    while (remaining != 0u)
    {
        u32 step = (remaining >= 32u) ? 32u :
                   ((remaining >= 16u) ? 16u : 8u);
        required++;
        remaining -= step;
    }
    if (required > (u32)(sNdsResultsNextOamId + 1))
    {
        gNdsResultsOamFailureOamFull++;
        ndsResultsRecordFillFailure();
        return 0;
    }
    screen_x = ndsResultsMapSourceCoord(sx0, NDS_RESULTS_SRC_ORIGIN_X,
                                        NDS_RESULTS_SCALE_X_Q16);
    screen_y = ndsResultsMapSourceCoord(sy0, NDS_RESULTS_SRC_ORIGIN_Y,
                                        NDS_RESULTS_SCALE_Y_Q16);
    remaining = mapped_width;
    while (remaining != 0u)
    {
        u32 step = (remaining >= 32u) ? 32u :
                   ((remaining >= 16u) ? 16u : 8u);
        SpriteSize size = (step == 32u) ? SpriteSize_32x8 :
                          ((step == 16u) ? SpriteSize_16x8 : SpriteSize_8x8);

        oamSet(&oamMain, sNdsResultsNextOamId, screen_x, screen_y, 0,
               sNdsResultsFillPaletteBank, size, SpriteColorFormat_16Color,
               gfx, -1, false, false, false, false, false);
        sNdsResultsNextOamId--;
        gNdsResultsOamEmitCount++;
        screen_x += (s32)step;
        remaining -= step;
    }
    sNdsResultsFrameNeedsCommit = 1u;
    return 1;
#else
    (void)sx0;
    (void)sy0;
    (void)sx1;
    (void)sy1;
    return 0;
#endif
}

void ndsResultsOamCommit(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    if ((sNdsResultsActive == 0u) || (sNdsResultsFrameNeedsCommit == 0u))
    {
        return;
    }
    oamUpdate(&oamMain);
    sNdsResultsPreviousNextOamId = sNdsResultsNextOamId;
    sNdsResultsFrameNeedsCommit = 0u;
    gNdsResultsOamCommitCount++;
#endif
}
