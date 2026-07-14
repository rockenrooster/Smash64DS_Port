#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nds/arm9/sprite.h>
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <nds/timers.h>
#include <PR/sp.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#define NDS_IFCOMMON_GAME_STATUS_SIZE 0x252d4u
#define NDS_IFCOMMON_OBJ_VRAM_BYTES (96u * 1024u)
#define NDS_IFCOMMON_ASSET_COUNT 16u
#define NDS_IFCOMMON_MAX_TILES 9u
#define NDS_IFCOMMON_SCREEN_SCALE_Q16 52429u
#define NDS_IFCOMMON_HASH_SEED 0x49464f41u

enum NDSIFCommonNativeAssetKind
{
    nNDSIFCommonAssetGoG,
    nNDSIFCommonAssetGoO,
    nNDSIFCommonAssetGoExclaim,
    nNDSIFCommonAssetRod,
    nNDSIFCommonAssetFrame,
    nNDSIFCommonAssetShadowInitial,
    nNDSIFCommonAssetShadowGo,
    nNDSIFCommonAssetRedDim,
    nNDSIFCommonAssetYellowDim,
    nNDSIFCommonAssetBlueDim,
    nNDSIFCommonAssetRedLight,
    nNDSIFCommonAssetYellowLight,
    nNDSIFCommonAssetBlueLight,
    nNDSIFCommonAssetRedContour,
    nNDSIFCommonAssetYellowContour,
    nNDSIFCommonAssetBlueContour
};

enum NDSIFCommonNativeFallbackReason
{
    nNDSIFCommonFallbackNone,
    nNDSIFCommonFallbackDisabled,
    nNDSIFCommonFallbackNotPrepared,
    nNDSIFCommonFallbackUnknownSprite,
    nNDSIFCommonFallbackRuntimeColor,
    nNDSIFCommonFallbackObjectLimit,
    nNDSIFCommonFallbackMatrixLimit,
    nNDSIFCommonFallbackBadAsset
};

typedef struct NDSIFCommonTileSpec
{
    u8 source_x;
    u8 source_y;
    u8 content_width;
    u8 content_height;
    u8 cell_width;
    u8 cell_height;
    u8 pad_x;
    u8 pad_y;
} NDSIFCommonTileSpec;

typedef struct NDSIFCommonAssetSpec
{
    u32 offset;
    u8 red;
    u8 green;
    u8 blue;
    u8 env_red;
    u8 env_green;
    u8 env_blue;
    u8 tile_count;
    NDSIFCommonTileSpec tiles[NDS_IFCOMMON_MAX_TILES];
} NDSIFCommonAssetSpec;

typedef struct NDSIFCommonNativeTile
{
    u16 *gfx;
    SpriteSize size;
    NDSIFCommonTileSpec spec;
} NDSIFCommonNativeTile;

typedef struct NDSIFCommonNativeAsset
{
    const Bitmap *bitmap;
    u32 width;
    u32 height;
    u32 tile_count;
    u32 runtime_red;
    u32 runtime_green;
    u32 runtime_blue;
    u32 runtime_env_red;
    u32 runtime_env_green;
    u32 runtime_env_blue;
    NDSIFCommonNativeTile tiles[NDS_IFCOMMON_MAX_TILES];
} NDSIFCommonNativeAsset;

#define TILE(sx, sy, sw, sh, cw, ch, px, py) \
    { (sx), (sy), (sw), (sh), (cw), (ch), (px), (py) }
#define NO_TILE TILE(0, 0, 0, 0, 0, 0, 0, 0)

/* These offsets and dimensions are the exact mixed-width Sprite manifest
 * used by reloc_backend_assets.c.  Traffic colors are ifcommon.c's original
 * dIFCommonTrafficSpriteColors tables; the second shadow is the GO-time
 * primary/environment pair from the same translation unit. */
static const NDSIFCommonAssetSpec sNdsIFCommonAssetSpecs[
    NDS_IFCOMMON_ASSET_COUNT] = {
    { 0x4d78u, 255, 255, 255, 0, 0, 0, 3,
      { TILE(0, 0, 62, 64, 64, 64, 0, 0),
        TILE(0, 64, 32, 9, 32, 16, 0, 0),
        TILE(32, 64, 30, 9, 32, 16, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0xa730u, 255, 255, 255, 0, 0, 0, 6,
      { TILE(0, 0, 64, 64, 64, 64, 0, 0),
        TILE(64, 0, 6, 32, 8, 32, 0, 0),
        TILE(64, 32, 6, 32, 8, 32, 0, 0),
        TILE(0, 64, 32, 10, 32, 16, 0, 0),
        TILE(32, 64, 32, 10, 32, 16, 0, 0),
        TILE(64, 64, 6, 10, 8, 16, 0, 0), NO_TILE, NO_TILE,
        NO_TILE } },
    { 0xc370u, 255, 255, 255, 0, 0, 0, 2,
      { TILE(0, 0, 24, 64, 32, 64, 0, 0),
        TILE(0, 64, 24, 9, 32, 16, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x20990u, 255, 255, 255, 0, 0, 0, 2,
      { TILE(0, 0, 8, 32, 8, 32, 0, 0),
        TILE(0, 32, 8, 21, 8, 32, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x21760u, 255, 255, 255, 0, 0, 0, 8,
      { TILE(0, 0, 32, 32, 32, 32, 0, 0),
        TILE(32, 0, 32, 32, 32, 32, 0, 0),
        TILE(64, 0, 32, 32, 32, 32, 0, 0),
        TILE(96, 0, 1, 32, 8, 32, 0, 0),
        TILE(0, 32, 32, 1, 32, 8, 0, 0),
        TILE(32, 32, 32, 1, 32, 8, 0, 0),
        TILE(64, 32, 32, 1, 32, 8, 0, 0),
        TILE(96, 32, 1, 1, 8, 8, 0, 0), NO_TILE } },
    { 0x21878u, 0, 0, 0, 0, 0, 0, 1,
      { TILE(0, 0, 15, 11, 16, 16, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x21878u, 0x6a, 0x6a, 0x95, 0x12, 0x12, 0x2e, 1,
      { TILE(0, 0, 15, 11, 16, 16, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x21950u, 0xfe, 0x0c, 0x0c, 0, 0, 0, 1,
      { TILE(0, 0, 15, 15, 16, 16, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x21a10u, 0xff, 0xa2, 0x00, 0, 0, 0, 1,
      { TILE(0, 0, 11, 11, 16, 16, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x21ba8u, 0x4b, 0x64, 0xff, 0, 0, 0, 1,
      { TILE(0, 0, 19, 19, 32, 32, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x22128u, 0xff, 0xff, 0xff, 0, 0, 0, 2,
      { TILE(0, 0, 32, 32, 32, 32, 0, 0),
        TILE(0, 32, 32, 9, 32, 16, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x22588u, 0xff, 0xff, 0xff, 0, 0, 0, 1,
      { TILE(0, 0, 30, 32, 32, 32, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x22f18u, 0xff, 0xff, 0xff, 0, 0, 0, 6,
      { TILE(0, 0, 32, 32, 32, 32, 0, 0),
        TILE(32, 0, 14, 32, 16, 32, 0, 0),
        TILE(0, 32, 32, 16, 32, 16, 0, 0),
        TILE(32, 32, 14, 16, 16, 16, 0, 0),
        TILE(0, 48, 32, 1, 32, 8, 0, 0),
        TILE(32, 48, 14, 1, 16, 8, 0, 0), NO_TILE, NO_TILE,
        NO_TILE } },
    { 0x23a28u, 0xff, 0x38, 0x38, 0, 0, 0, 6,
      { TILE(0, 0, 32, 32, 32, 32, 0, 0),
        TILE(32, 0, 16, 32, 16, 32, 0, 0),
        TILE(0, 32, 32, 16, 32, 16, 0, 0),
        TILE(32, 32, 16, 16, 16, 16, 0, 0),
        TILE(0, 48, 32, 9, 32, 16, 0, 0),
        TILE(32, 48, 16, 9, 16, 16, 0, 0), NO_TILE, NO_TILE,
        NO_TILE } },
    { 0x24620u, 0xff, 0xa2, 0x00, 0, 0, 0, 9,
      { TILE(0, 0, 18, 18, 32, 32, 7, 7),
        TILE(18, 0, 18, 18, 32, 32, 7, 7),
        TILE(36, 0, 17, 18, 32, 32, 7, 7),
        TILE(0, 18, 18, 18, 32, 32, 7, 7),
        TILE(18, 18, 18, 18, 32, 32, 7, 7),
        TILE(36, 18, 17, 18, 32, 32, 7, 7),
        TILE(0, 36, 18, 17, 32, 32, 7, 7),
        TILE(18, 36, 18, 17, 32, 32, 7, 7),
        TILE(36, 36, 17, 17, 32, 32, 7, 7) } },
    { 0x25290u, 0x22, 0x66, 0xfe, 0, 0, 0, 9,
      { TILE(0, 0, 19, 19, 32, 32, 6, 6),
        TILE(19, 0, 18, 19, 32, 32, 7, 6),
        TILE(37, 0, 18, 19, 32, 32, 7, 6),
        TILE(0, 19, 19, 18, 32, 32, 6, 7),
        TILE(19, 19, 18, 18, 32, 32, 7, 7),
        TILE(37, 19, 18, 18, 32, 32, 7, 7),
        TILE(0, 37, 19, 18, 32, 32, 6, 7),
        TILE(19, 37, 18, 18, 32, 32, 7, 7),
        TILE(37, 37, 18, 18, 32, 32, 7, 7) } }
};

#undef TILE
#undef NO_TILE

static NDSIFCommonNativeAsset sNdsIFCommonAssets[
    NDS_IFCOMMON_ASSET_COUNT];
static const void *sNdsIFCommonPreparedFile;
static u32 sNdsIFCommonPrepared;
static s32 sNdsIFCommonNextOamID = 127;
static u32 sNdsIFCommonMatrixCount;
static u16 sNdsIFCommonMatrixInverse[32];

volatile u32 gNdsIFCommonNativeOamEnabled = 1u;
volatile u32 gNdsIFCommonNativeOamPrepareCount;
volatile u32 gNdsIFCommonNativeOamPrepareSuccessCount;
volatile u32 gNdsIFCommonNativeOamPrepareFailCount;
volatile u32 gNdsIFCommonNativeOamPrepareTicks;
volatile u32 gNdsIFCommonNativeOamPrepareBytes;
volatile u32 gNdsIFCommonNativeOamPrepareAssets;
volatile u32 gNdsIFCommonNativeOamPrepareTiles;
volatile u32 gNdsIFCommonNativeOamPrepareProfileFrame;
volatile u32 gNdsIFCommonNativeOamPreparePaletteBytes;
volatile u32 gNdsIFCommonNativeOamHotConvertCount;
volatile u32 gNdsIFCommonNativeOamRuntimeUploadBytes;
volatile u32 gNdsIFCommonNativeOamFrameTicks;
volatile u32 gNdsIFCommonNativeOamFrameRecognizedCalls;
volatile u32 gNdsIFCommonNativeOamFrameDrawCalls;
volatile u32 gNdsIFCommonNativeOamFrameFallbackCalls;
volatile u32 gNdsIFCommonNativeOamFrameSObjCount;
volatile u32 gNdsIFCommonNativeOamFrameObjectCount;
volatile u32 gNdsIFCommonNativeOamFrameSemanticHash;
volatile u32 gNdsIFCommonNativeOamLastFallbackReason;
volatile u32 gNdsIFCommonNativeOamCommitCount;

static u32 ndsIFCommonHashMix(u32 hash, u32 value)
{
    hash ^= value + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    return hash;
}

static u32 ndsIFCommonFloatBits(f32 value)
{
    u32 bits;

    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static u16 ndsIFCommonPackRgb15(u8 red, u8 green, u8 blue)
{
    return (u16)((1u << 15) | (red >> 3) | ((green >> 3) << 5) |
                 ((blue >> 3) << 10));
}

static u16 ndsIFCommonLerp(u8 red, u8 green, u8 blue,
                           u8 env_red, u8 env_green, u8 env_blue,
                           u8 intensity)
{
    u32 inverse = 255u - intensity;
    u8 out_red = (u8)(((u32)red * intensity +
                       (u32)env_red * inverse + 127u) / 255u);
    u8 out_green = (u8)(((u32)green * intensity +
                         (u32)env_green * inverse + 127u) / 255u);
    u8 out_blue = (u8)(((u32)blue * intensity +
                        (u32)env_blue * inverse + 127u) / 255u);

    return ndsIFCommonPackRgb15(out_red, out_green, out_blue);
}

static u16 ndsIFCommonConvertRgba32(u32 rgba)
{
    if ((rgba & 0xffu) == 0u)
    {
        return 0u;
    }
    return ndsIFCommonPackRgb15((u8)(rgba >> 24), (u8)(rgba >> 16),
                                (u8)(rgba >> 8));
}

static s32 ndsIFCommonRangeValid(const void *base, size_t size,
                                 const void *ptr, size_t bytes)
{
    uintptr_t first = (uintptr_t)base;
    uintptr_t address = (uintptr_t)ptr;

    return ((address >= first) && ((address - first) <= size) &&
            (bytes <= (size - (address - first)))) ? TRUE : FALSE;
}

static u16 ndsIFCommonDecodePixel(const Sprite *sprite,
                                  const NDSIFCommonAssetSpec *spec,
                                  const void *file_data, size_t file_size,
                                  u32 source_x, u32 source_y)
{
    const Bitmap *bitmap = sprite->bitmap;
    u32 out_y = 0u;
    u32 bitmap_index;
    u16 color = 0u;

    for (bitmap_index = 0u;
         (bitmap_index < (u32)(u16)sprite->nbitmaps) &&
         (out_y < (u32)(u16)sprite->height);
         bitmap_index++)
    {
        const Bitmap *current = &bitmap[bitmap_index];
        u32 width = (u32)(u16)current->width;
        u32 width_img = (u32)(u16)current->width_img;
        u32 height = (u32)(u16)current->actualHeight;
        u32 advance = (u32)(u16)sprite->bmheight;
        u32 local_y;
        u32 texshuf_x;
        size_t row_bytes;
        size_t source_index;

        if (width == 0u)
        {
            break;
        }
        if (width_img == 0u)
        {
            width_img = width;
        }
        if (height == 0u)
        {
            height = advance;
        }
        if (advance == 0u)
        {
            advance = height;
        }
        if ((source_y < out_y) || (source_y >= (out_y + height)) ||
            (source_x >= width))
        {
            out_y += advance;
            continue;
        }

        local_y = source_y - out_y;
        texshuf_x = source_x;
        if ((local_y & 1u) != 0u)
        {
            if (sprite->bmsiz == G_IM_SIZ_4b)
            {
                texshuf_x ^= 8u;
            }
            else if (sprite->bmsiz == G_IM_SIZ_8b)
            {
                texshuf_x ^= 4u;
            }
            else if ((sprite->bmfmt == G_IM_FMT_RGBA) &&
                     (sprite->bmsiz == G_IM_SIZ_32b))
            {
                texshuf_x ^= 1u;
            }
        }

        if ((sprite->bmfmt == G_IM_FMT_RGBA) &&
            (sprite->bmsiz == G_IM_SIZ_32b))
        {
            const u32 *pixels = (const u32 *)current->buf;
            u32 rgba;

            if (ndsIFCommonRangeValid(
                    file_data, file_size, pixels,
                    (size_t)width_img * height * sizeof(*pixels)) == FALSE)
            {
                return 0u;
            }
            memcpy(&rgba, &pixels[(local_y * width_img) + texshuf_x],
                   sizeof(rgba));
            color = ndsIFCommonConvertRgba32(rgba);
        }
        else if ((sprite->bmfmt == G_IM_FMT_IA) &&
                 (sprite->bmsiz == G_IM_SIZ_8b))
        {
            const u8 *pixels = (const u8 *)current->buf;
            u8 ia;

            row_bytes = width_img;
            if (ndsIFCommonRangeValid(file_data, file_size, pixels,
                                      row_bytes * height) == FALSE)
            {
                return 0u;
            }
            source_index = ((size_t)local_y * row_bytes) + texshuf_x;
            ia = pixels[source_index ^ 3u];
            color = ((ia & 0x0fu) != 0u) ?
                ndsIFCommonLerp(spec->red, spec->green, spec->blue,
                                spec->env_red, spec->env_green,
                                spec->env_blue,
                                (u8)(((ia >> 4) * 255u) / 15u)) : 0u;
        }
        else if ((sprite->bmfmt == G_IM_FMT_I) &&
                 (sprite->bmsiz == G_IM_SIZ_8b))
        {
            const u8 *pixels = (const u8 *)current->buf;
            u8 intensity;

            row_bytes = width_img;
            if (ndsIFCommonRangeValid(file_data, file_size, pixels,
                                      row_bytes * height) == FALSE)
            {
                return 0u;
            }
            source_index = ((size_t)local_y * row_bytes) + texshuf_x;
            intensity = pixels[source_index ^ 3u];
            color = (intensity != 0u) ?
                ndsIFCommonPackRgb15(spec->red, spec->green, spec->blue) :
                0u;
        }
        else if ((sprite->bmfmt == G_IM_FMT_I) &&
                 (sprite->bmsiz == G_IM_SIZ_4b))
        {
            const u8 *pixels = (const u8 *)current->buf;
            u8 packed;
            u8 intensity;

            row_bytes = (width_img + 1u) / 2u;
            if (ndsIFCommonRangeValid(file_data, file_size, pixels,
                                      row_bytes * height) == FALSE)
            {
                return 0u;
            }
            source_index = ((size_t)local_y * row_bytes) +
                           (texshuf_x >> 1);
            packed = pixels[source_index ^ 3u];
            intensity = ((texshuf_x & 1u) == 0u) ?
                (u8)(packed >> 4) : (u8)(packed & 0x0fu);
            color = (intensity != 0u) ?
                ndsIFCommonPackRgb15(spec->red, spec->green, spec->blue) :
                0u;
        }
        out_y += advance;
    }
    return color;
}

static SpriteSize ndsIFCommonSpriteSize(u32 width, u32 height)
{
    if ((width == 8u) && (height == 8u)) return SpriteSize_8x8;
    if ((width == 16u) && (height == 16u)) return SpriteSize_16x16;
    if ((width == 32u) && (height == 32u)) return SpriteSize_32x32;
    if ((width == 64u) && (height == 64u)) return SpriteSize_64x64;
    if ((width == 16u) && (height == 8u)) return SpriteSize_16x8;
    if ((width == 32u) && (height == 8u)) return SpriteSize_32x8;
    if ((width == 32u) && (height == 16u)) return SpriteSize_32x16;
    if ((width == 64u) && (height == 32u)) return SpriteSize_64x32;
    if ((width == 8u) && (height == 16u)) return SpriteSize_8x16;
    if ((width == 8u) && (height == 32u)) return SpriteSize_8x32;
    if ((width == 16u) && (height == 32u)) return SpriteSize_16x32;
    if ((width == 32u) && (height == 64u)) return SpriteSize_32x64;
    return (SpriteSize)0;
}

static s32 ndsIFCommonPrepareAsset(u32 asset_index, const void *file_data,
                                   size_t file_size, u32 *vram_cursor)
{
    const NDSIFCommonAssetSpec *spec =
        &sNdsIFCommonAssetSpecs[asset_index];
    NDSIFCommonNativeAsset *asset = &sNdsIFCommonAssets[asset_index];
    const Sprite *sprite = (const Sprite *)((const u8 *)file_data +
                                             spec->offset);
    u32 tile_index;

    if ((spec->offset + sizeof(*sprite) > file_size) ||
        (ndsIFCommonRangeValid(file_data, file_size, sprite->bitmap,
                              (size_t)(u16)sprite->nbitmaps *
                                  sizeof(Bitmap)) == FALSE))
    {
        return FALSE;
    }

    memset(asset, 0, sizeof(*asset));
    asset->bitmap = sprite->bitmap;
    asset->width = (u32)(u16)sprite->width;
    asset->height = (u32)(u16)sprite->height;
    asset->tile_count = spec->tile_count;
    asset->runtime_red = spec->red;
    asset->runtime_green = spec->green;
    asset->runtime_blue = spec->blue;
    asset->runtime_env_red = spec->env_red;
    asset->runtime_env_green = spec->env_green;
    asset->runtime_env_blue = spec->env_blue;

    for (tile_index = 0u; tile_index < spec->tile_count; tile_index++)
    {
        const NDSIFCommonTileSpec *tile_spec = &spec->tiles[tile_index];
        NDSIFCommonNativeTile *tile = &asset->tiles[tile_index];
        u32 bytes = (u32)tile_spec->cell_width *
                    (u32)tile_spec->cell_height * sizeof(u16);
        u32 y;

        if (((*vram_cursor + bytes) > NDS_IFCOMMON_OBJ_VRAM_BYTES) ||
            ((tile_spec->source_x + tile_spec->content_width) >
             asset->width) ||
            ((tile_spec->source_y + tile_spec->content_height) >
             asset->height) ||
            ((tile_spec->pad_x + tile_spec->content_width) >
             tile_spec->cell_width) ||
            ((tile_spec->pad_y + tile_spec->content_height) >
             tile_spec->cell_height))
        {
            return FALSE;
        }
        tile->gfx = (u16 *)((u8 *)SPRITE_GFX + *vram_cursor);
        tile->size = ndsIFCommonSpriteSize(tile_spec->cell_width,
                                           tile_spec->cell_height);
        tile->spec = *tile_spec;
        if ((u32)tile->size == 0u)
        {
            return FALSE;
        }

        dmaFillHalfWords(0u, tile->gfx, bytes);
        for (y = 0u; y < tile_spec->content_height; y++)
        {
            u32 x;
            u16 *destination = &tile->gfx[
                ((u32)tile_spec->pad_y + y) * tile_spec->cell_width +
                tile_spec->pad_x];

            for (x = 0u; x < tile_spec->content_width; x++)
            {
                destination[x] = ndsIFCommonDecodePixel(
                    sprite, spec, file_data, file_size,
                    (u32)tile_spec->source_x + x,
                    (u32)tile_spec->source_y + y);
            }
        }
        *vram_cursor += bytes;
        gNdsIFCommonNativeOamPrepareTiles++;
    }
    gNdsIFCommonNativeOamPrepareAssets++;
    return TRUE;
}

void ndsIFCommonNativeOamInit(void)
{
    /* These remain zero for the lifetime of the prepared asset set. Bitmap
     * texels are converted directly into OBJ VRAM during relocation, and the
     * 16-bit OBJ path has no palette payload or gameplay-time upload path. */
    gNdsIFCommonNativeOamPreparePaletteBytes = 0u;
    gNdsIFCommonNativeOamHotConvertCount = 0u;
    gNdsIFCommonNativeOamRuntimeUploadBytes = 0u;
#if NDS_RENDERER_HW_TRIANGLES
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
    oamClear(&oamMain, 0, 128);
    oamUpdate(&oamMain);
#endif
}

s32 ndsIFCommonNativeOamPrepareGameStatus(void *file_data,
                                           size_t file_size)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 start;
    u32 vram_cursor = 0u;
    u32 asset_index;

    if ((file_data == NULL) ||
        (file_size < NDS_IFCOMMON_GAME_STATUS_SIZE))
    {
        gNdsIFCommonNativeOamPrepareFailCount++;
        return FALSE;
    }
    if ((sNdsIFCommonPrepared != FALSE) &&
        (sNdsIFCommonPreparedFile == file_data))
    {
        return TRUE;
    }

    start = cpuGetTiming();
    gNdsIFCommonNativeOamPrepareCount++;
    gNdsIFCommonNativeOamPrepareAssets = 0u;
    gNdsIFCommonNativeOamPrepareTiles = 0u;
    gNdsIFCommonNativeOamPrepareProfileFrame =
        gNdsRendererProfileFrameCount;
    memset(sNdsIFCommonAssets, 0, sizeof(sNdsIFCommonAssets));
    sNdsIFCommonPrepared = FALSE;
    sNdsIFCommonPreparedFile = NULL;

    for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
         asset_index++)
    {
        if (ndsIFCommonPrepareAsset(asset_index, file_data, file_size,
                                    &vram_cursor) == FALSE)
        {
            gNdsIFCommonNativeOamPrepareFailCount++;
            gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            return FALSE;
        }
    }

    sNdsIFCommonPrepared = TRUE;
    sNdsIFCommonPreparedFile = file_data;
    gNdsIFCommonNativeOamPrepareBytes = vram_cursor;
    gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
    gNdsIFCommonNativeOamPrepareSuccessCount++;
    return TRUE;
#else
    (void)file_data;
    (void)file_size;
    return FALSE;
#endif
}

void ndsIFCommonNativeOamBeginFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    oamClear(&oamMain, 0, 128);
#endif
    sNdsIFCommonNextOamID = 127;
    sNdsIFCommonMatrixCount = 0u;
    gNdsIFCommonNativeOamFrameTicks = 0u;
    gNdsIFCommonNativeOamFrameRecognizedCalls = 0u;
    gNdsIFCommonNativeOamFrameDrawCalls = 0u;
    gNdsIFCommonNativeOamFrameFallbackCalls = 0u;
    gNdsIFCommonNativeOamFrameSObjCount = 0u;
    gNdsIFCommonNativeOamFrameObjectCount = 0u;
    gNdsIFCommonNativeOamFrameSemanticHash = NDS_IFCOMMON_HASH_SEED;
    gNdsIFCommonNativeOamLastFallbackReason =
        nNDSIFCommonFallbackNone;
}

static s32 ndsIFCommonAssetForSObj(const SObj *sobj)
{
    u32 asset_index;

    for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
         asset_index++)
    {
        const NDSIFCommonNativeAsset *asset =
            &sNdsIFCommonAssets[asset_index];

        if ((sobj->sprite.bitmap == asset->bitmap) &&
            ((u32)(u16)sobj->sprite.width == asset->width) &&
            ((u32)(u16)sobj->sprite.height == asset->height))
        {
            if ((asset_index == nNDSIFCommonAssetShadowInitial) ||
                (asset_index == nNDSIFCommonAssetShadowGo))
            {
                if (((u32)sobj->sprite.red != asset->runtime_red) ||
                    ((u32)sobj->sprite.green != asset->runtime_green) ||
                    ((u32)sobj->sprite.blue != asset->runtime_blue) ||
                    ((u32)sobj->envcolor.r != asset->runtime_env_red) ||
                    ((u32)sobj->envcolor.g != asset->runtime_env_green) ||
                    ((u32)sobj->envcolor.b != asset->runtime_env_blue))
                {
                    continue;
                }
            }
            else if ((asset_index >= nNDSIFCommonAssetRedDim) &&
                     (((u32)sobj->sprite.red != asset->runtime_red) ||
                      ((u32)sobj->sprite.green != asset->runtime_green) ||
                      ((u32)sobj->sprite.blue != asset->runtime_blue)))
            {
                return -2;
            }
            return (s32)asset_index;
        }
    }
    return -1;
}

static s32 ndsIFCommonMatrixForScale(u16 inverse)
{
    u32 matrix_index;

    for (matrix_index = 0u; matrix_index < sNdsIFCommonMatrixCount;
         matrix_index++)
    {
        if (sNdsIFCommonMatrixInverse[matrix_index] == inverse)
        {
            return (s32)matrix_index;
        }
    }
    if (sNdsIFCommonMatrixCount >= 32u)
    {
        return -1;
    }
    matrix_index = sNdsIFCommonMatrixCount++;
    sNdsIFCommonMatrixInverse[matrix_index] = inverse;
    oamRotateScale(&oamMain, (int)matrix_index, 0, inverse, inverse);
    return (s32)matrix_index;
}

static void ndsIFCommonRecordSemantic(const SObj *sobj, u32 asset_index)
{
    u32 hash = gNdsIFCommonNativeOamFrameSemanticHash;

    hash = ndsIFCommonHashMix(hash, asset_index);
    hash = ndsIFCommonHashMix(hash, ndsIFCommonFloatBits(sobj->pos.x));
    hash = ndsIFCommonHashMix(hash, ndsIFCommonFloatBits(sobj->pos.y));
    hash = ndsIFCommonHashMix(hash,
                             ndsIFCommonFloatBits(sobj->sprite.scalex));
    hash = ndsIFCommonHashMix(hash,
                             ndsIFCommonFloatBits(sobj->sprite.scaley));
    hash = ndsIFCommonHashMix(
        hash, ((u32)sobj->sprite.red << 24) |
                  ((u32)sobj->sprite.green << 16) |
                  ((u32)sobj->sprite.blue << 8) | sobj->sprite.alpha);
    hash = ndsIFCommonHashMix(
        hash, ((u32)sobj->envcolor.r << 24) |
                  ((u32)sobj->envcolor.g << 16) |
                  ((u32)sobj->envcolor.b << 8) | sobj->envcolor.a);
    hash = ndsIFCommonHashMix(hash, sobj->sprite.attr);
    gNdsIFCommonNativeOamFrameSemanticHash = hash;
    gNdsIFCommonNativeOamFrameSObjCount++;
}

static s32 ndsIFCommonEmitSObj(const SObj *sobj, u32 asset_index)
{
    const NDSIFCommonNativeAsset *asset =
        &sNdsIFCommonAssets[asset_index];
    u32 scale_x_q16;
    u32 scale_y_q16;
    u16 inverse_x;
    u16 inverse_y;
    s32 matrix_index;
    s32 origin_x_q16;
    s32 origin_y_q16;
    u32 tile_index;
    s32 size_double;

    scale_x_q16 = (u32)((sobj->sprite.scalex *
                         (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16) + 0.5F);
    scale_y_q16 = (u32)((sobj->sprite.scaley *
                         (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16) + 0.5F);
    if ((scale_x_q16 == 0u) || (scale_y_q16 == 0u) ||
        (scale_x_q16 != scale_y_q16))
    {
        return FALSE;
    }
    inverse_x = (u16)(((1u << 24) + (scale_x_q16 / 2u)) /
                      scale_x_q16);
    inverse_y = (u16)(((1u << 24) + (scale_y_q16 / 2u)) /
                      scale_y_q16);
    if (inverse_x != inverse_y)
    {
        return FALSE;
    }
    matrix_index = ndsIFCommonMatrixForScale(inverse_x);
    if (matrix_index < 0)
    {
        return FALSE;
    }
    origin_x_q16 = (s32)(sobj->pos.x *
                         (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16);
    origin_y_q16 = (s32)(sobj->pos.y *
                         (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16);
    size_double = (scale_x_q16 > (1u << 16)) ? TRUE : FALSE;

    for (tile_index = 0u; tile_index < asset->tile_count; tile_index++)
    {
        const NDSIFCommonNativeTile *tile = &asset->tiles[tile_index];
        const NDSIFCommonTileSpec *spec = &tile->spec;
        s32 local_center_x = (s32)spec->source_x +
            ((s32)spec->cell_width / 2) - (s32)spec->pad_x;
        s32 local_center_y = (s32)spec->source_y +
            ((s32)spec->cell_height / 2) - (s32)spec->pad_y;
        s32 center_x_q16 = origin_x_q16 +
            (s32)((s64)local_center_x * (s64)scale_x_q16);
        s32 center_y_q16 = origin_y_q16 +
            (s32)((s64)local_center_y * (s64)scale_y_q16);
        s32 half_bounds_x = size_double ? spec->cell_width :
                                              (spec->cell_width / 2);
        s32 half_bounds_y = size_double ? spec->cell_height :
                                              (spec->cell_height / 2);
        s32 x = (center_x_q16 >> 16) - half_bounds_x;
        s32 y = (center_y_q16 >> 16) - half_bounds_y;

        oamSet(&oamMain, sNdsIFCommonNextOamID, x, y, 0, 15,
               tile->size, SpriteColorFormat_Bmp, tile->gfx,
               matrix_index, size_double, false, false, false, false);
        sNdsIFCommonNextOamID--;
        gNdsIFCommonNativeOamFrameObjectCount++;
    }
    return TRUE;
}

s32 ndsIFCommonNativeOamDrawGObj(struct GObj *gobj)
{
#if NDS_RENDERER_HW_TRIANGLES
    SObj *sobj;
    SObj *scan;
    u32 required_objects = 0u;
    u32 start = cpuGetTiming();
    s32 oam_id_before;
    u32 matrix_count_before;
    u32 object_count_before;
    s32 recognized = FALSE;

    if (gobj == NULL)
    {
        return FALSE;
    }
    sobj = SObjGetStruct(gobj);
    if (sobj == NULL)
    {
        return FALSE;
    }

    for (scan = sobj; scan != NULL; scan = scan->next)
    {
        s32 asset_index;

        if ((scan->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        asset_index = ndsIFCommonAssetForSObj(scan);
        if (asset_index == -1)
        {
            if (recognized == FALSE)
            {
                return FALSE;
            }
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackUnknownSprite;
            gNdsIFCommonNativeOamFrameTicks += cpuGetTiming() - start;
            return FALSE;
        }
        if (asset_index == -2)
        {
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackRuntimeColor;
            gNdsIFCommonNativeOamFrameTicks += cpuGetTiming() - start;
            return FALSE;
        }
        recognized = TRUE;
        required_objects += sNdsIFCommonAssets[asset_index].tile_count;
        ndsIFCommonRecordSemantic(scan, (u32)asset_index);
    }
    if (recognized == FALSE)
    {
        return FALSE;
    }
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    if (gNdsIFCommonNativeOamEnabled == 0u)
    {
        gNdsIFCommonNativeOamFrameFallbackCalls++;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackDisabled;
        gNdsIFCommonNativeOamFrameTicks += cpuGetTiming() - start;
        return FALSE;
    }
    if (sNdsIFCommonPrepared == FALSE)
    {
        gNdsIFCommonNativeOamFrameFallbackCalls++;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackNotPrepared;
        gNdsIFCommonNativeOamFrameTicks += cpuGetTiming() - start;
        return FALSE;
    }
    if (required_objects > (u32)(sNdsIFCommonNextOamID + 1))
    {
        gNdsIFCommonNativeOamFrameFallbackCalls++;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackObjectLimit;
        gNdsIFCommonNativeOamFrameTicks += cpuGetTiming() - start;
        return FALSE;
    }

    oam_id_before = sNdsIFCommonNextOamID;
    matrix_count_before = sNdsIFCommonMatrixCount;
    object_count_before = gNdsIFCommonNativeOamFrameObjectCount;
    for (scan = sobj; scan != NULL; scan = scan->next)
    {
        s32 asset_index;

        if ((scan->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        asset_index = ndsIFCommonAssetForSObj(scan);
        if ((asset_index < 0) ||
            (ndsIFCommonEmitSObj(scan, (u32)asset_index) == FALSE))
        {
            if (sNdsIFCommonNextOamID < oam_id_before)
            {
                oamClear(&oamMain, sNdsIFCommonNextOamID + 1,
                         oam_id_before - sNdsIFCommonNextOamID);
            }
            sNdsIFCommonNextOamID = oam_id_before;
            sNdsIFCommonMatrixCount = matrix_count_before;
            gNdsIFCommonNativeOamFrameObjectCount = object_count_before;
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackMatrixLimit;
            gNdsIFCommonNativeOamFrameTicks += cpuGetTiming() - start;
            return FALSE;
        }
    }
    gNdsIFCommonNativeOamFrameDrawCalls++;
    gNdsIFCommonNativeOamFrameTicks += cpuGetTiming() - start;
    return TRUE;
#else
    (void)gobj;
    return FALSE;
#endif
}

void ndsIFCommonNativeOamCommit(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    oamUpdate(&oamMain);
    gNdsIFCommonNativeOamCommitCount++;
#endif
}
