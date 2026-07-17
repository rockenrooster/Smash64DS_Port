#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nds/arm9/sprite.h>
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <nds/timers.h>
#include <PR/sp.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_renderer.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>

s32 ndsRendererHardwarePrepareIFCommonA3I5Atlas(
    u32 width, u32 height, const u16 palette[32],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name);

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#define NDS_IFCOMMON_GAME_STATUS_SIZE 0x252d4u
#define NDS_IFCOMMON_OBJ_GFX_ALIGNMENT 128u
#define NDS_IFCOMMON_OBJ_PALETTE_ENTRIES 256u
#define NDS_IFCOMMON_OBJ_VRAM_BYTES (64u * 1024u)
#define NDS_IFCOMMON_ASSET_COUNT 16u
#define NDS_IFCOMMON_MAX_TILES 9u
#define NDS_IFCOMMON_SCREEN_SCALE_Q16 52429u
#define NDS_IFCOMMON_TRAFFIC_SPEC_COUNT 7u
#define NDS_IFCOMMON_TRAFFIC_ATLAS_WIDTH 128u
#define NDS_IFCOMMON_TRAFFIC_ATLAS_HEIGHT 64u
#define NDS_IFCOMMON_TRAFFIC_ATLAS_BYTES \
    (NDS_IFCOMMON_TRAFFIC_ATLAS_WIDTH * NDS_IFCOMMON_TRAFFIC_ATLAS_HEIGHT)
#define NDS_IFCOMMON_CLOUD_FIRST nNDSIFCommonAssetRedContour
#define NDS_IFCOMMON_CLOUD_ATLAS_COUNT 2u
#define NDS_IFCOMMON_CLOUD_ATLAS0_WIDTH 256u
#define NDS_IFCOMMON_CLOUD_ATLAS1_WIDTH 128u
#define NDS_IFCOMMON_CLOUD_ATLAS_HEIGHT 128u
#define NDS_IFCOMMON_CLOUD_SPEC_COUNT 6u
#define NDS_IFCOMMON_CLOUD_ALPHA_THRESHOLD 8u
#define NDS_IFCOMMON_GO_ALPHA_THRESHOLD 112u
#define NDS_IFCOMMON_TRAFFIC_ALPHA_THRESHOLD 8u
#define NDS_IFCOMMON_LIGHT_CORE_THRESHOLD 112u
#define NDS_IFCOMMON_TRAFFIC_PROOF_OFFSET 9u
#define NDS_IFCOMMON_OVERLAY_SPEC_COUNT 16u
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

typedef struct NDSIFCommonTrafficSpec
{
    u8 asset_index;
    u8 atlas_x;
    u8 atlas_y;
    u8 width;
    u8 height;
} NDSIFCommonTrafficSpec;

typedef struct NDSIFCommonCloudSpec
{
    u8 asset_index;
    u8 kind;
    u8 atlas_index;
    u8 atlas_x;
    u8 atlas_y;
    u8 width;
    u8 height;
    u8 source_x;
    u8 source_y;
    u8 palette_index;
} NDSIFCommonCloudSpec;

enum NDSIFCommonCloudKind
{
    nNDSIFCommonCloudLight,
    nNDSIFCommonCloudContour
};

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
    u8 color_format;
    NDSIFCommonTileSpec spec;
} NDSIFCommonNativeTile;

typedef struct NDSIFCommonNativeAsset
{
    const Sprite *sprite;
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
 * used by reloc_backend_assets.c. GO is deinterleaved and prefiltered once
 * into direct RGB555+A1 bitmap OBJ cells; no palette or draw-time filtering
 * can reintroduce the source TEXSHUF comb pattern. */
static const NDSIFCommonAssetSpec sNdsIFCommonAssetSpecs[
    NDS_IFCOMMON_ASSET_COUNT] = {
    { 0x4d78u, 255, 255, 255, 0, 0, 0, 1,
      { TILE(0, 0, 50, 58, 64, 64, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0xa730u, 255, 255, 255, 0, 0, 0, 1,
      { TILE(0, 0, 56, 59, 64, 64, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0xc370u, 255, 255, 255, 0, 0, 0, 1,
      { TILE(0, 0, 19, 58, 32, 64, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
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
      { TILE(0, 0, 26, 32, 32, 32, 0, 0),
        TILE(0, 32, 26, 1, 32, 8, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x22588u, 0xff, 0xff, 0xff, 0, 0, 0, 1,
      { TILE(0, 0, 24, 26, 32, 32, 0, 0), NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x22f18u, 0xff, 0xff, 0xff, 0, 0, 0, 4,
      { TILE(0, 0, 32, 32, 32, 32, 0, 0),
        TILE(32, 0, 5, 32, 8, 32, 0, 0),
        TILE(0, 32, 32, 7, 32, 8, 0, 0),
        TILE(32, 32, 5, 7, 8, 8, 0, 0), NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x23a28u, 0xff, 0x38, 0x38, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE } },
    { 0x24620u, 0xff, 0xa2, 0x00, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE } },
    { 0x25290u, 0x22, 0x66, 0xfe, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE } }
};

#undef TILE
#undef NO_TILE

/* Final 0.8x DS footprints, prefiltered once from the source IA8/I4 pixels. */
static const NDSIFCommonTrafficSpec sNdsIFCommonTrafficSpecs[
    NDS_IFCOMMON_TRAFFIC_SPEC_COUNT] = {
    { nNDSIFCommonAssetRod, 0, 0, 6, 42 },
    { nNDSIFCommonAssetFrame, 6, 0, 78, 26 },
    { nNDSIFCommonAssetShadowInitial, 84, 0, 12, 9 },
    { nNDSIFCommonAssetShadowGo, 96, 0, 12, 9 },
    { nNDSIFCommonAssetRedDim, 0, 42, 12, 12 },
    { nNDSIFCommonAssetYellowDim, 12, 42, 9, 9 },
    { nNDSIFCommonAssetBlueDim, 21, 42, 15, 15 }
};

/* Two A5I3 atlases retain the source I8 alpha ramps: Light at the final DS
 * grid and Contour at 2x. Atlas 1 is only 123x106 occupied, so its 128x128
 * allocation preserves every texel while avoiding 16 KiB of empty VRAM. The
 * opaque traffic base adds one 128x64 A3I5 atlas: 57344 bytes total. */
static const NDSIFCommonCloudSpec sNdsIFCommonCloudSpecs[
    NDS_IFCOMMON_CLOUD_SPEC_COUNT] = {
    { nNDSIFCommonAssetRedLight, nNDSIFCommonCloudLight,
      0, 197, 39, 26, 33, 0, 0, 4 },
    { nNDSIFCommonAssetYellowLight, nNDSIFCommonCloudLight,
      1, 99, 0, 24, 26, 0, 0, 4 },
    { nNDSIFCommonAssetBlueLight, nNDSIFCommonCloudLight,
      0, 197, 0, 37, 39, 0, 0, 4 },
    { nNDSIFCommonAssetRedContour, nNDSIFCommonCloudContour,
      0, 103, 0, 94, 114, 2, 0, 1 },
    { nNDSIFCommonAssetYellowContour, nNDSIFCommonCloudContour,
      1, 0, 0, 99, 106, 6, 0, 2 },
    { nNDSIFCommonAssetBlueContour, nNDSIFCommonCloudContour,
      0, 0, 0, 103, 108, 4, 2, 3 }
};
static const u16 sNdsIFCommonCloudAtlasWidths[
    NDS_IFCOMMON_CLOUD_ATLAS_COUNT] = {
    NDS_IFCOMMON_CLOUD_ATLAS0_WIDTH,
    NDS_IFCOMMON_CLOUD_ATLAS1_WIDTH
};

static NDSIFCommonNativeAsset sNdsIFCommonAssets[
    NDS_IFCOMMON_ASSET_COUNT];
static const void *sNdsIFCommonPreparedFile;
static size_t sNdsIFCommonPreparedFileSize;
static u32 sNdsIFCommonPrepared;
static s32 sNdsIFCommonNextOamID = 127;
static s32 sNdsIFCommonPreviousLowestOamID = 128;
static u32 sNdsIFCommonMatrixCount;
static u16 sNdsIFCommonMatrixInverse[32];
static u32 sNdsIFCommonFrameNeedsCommit;
static u32 sNdsIFCommonCloudTextureNames[NDS_IFCOMMON_CLOUD_ATLAS_COUNT];
static u32 sNdsIFCommonTrafficTextureName;

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
volatile u32 gNdsIFCommonNativeOamPrepareCloudTextureBytes;
volatile u32 gNdsIFCommonNativeOamPrepareCloudTextureCount;
volatile u32 gNdsIFCommonNativeOamPrepareCloudFailureStage;
volatile u32 gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[
    NDS_IFCOMMON_OVERLAY_SPEC_COUNT];
volatile u32 gNdsIFCommonNativeOamHotConvertCount;
volatile u32 gNdsIFCommonNativeOamRuntimeUploadBytes;
volatile u32 gNdsIFCommonNativeOamFrameBeginTicks;
volatile u32 gNdsIFCommonNativeOamFrameTicks;
volatile u32 gNdsIFCommonNativeOamFrameCommitTicks;
volatile u32 gNdsIFCommonNativeOamFrameCommitCalls;
volatile u32 gNdsIFCommonNativeOamFrameClearedObjects;
volatile u32 gNdsIFCommonNativeOamFrameIdle;
volatile u32 gNdsIFCommonNativeOamIdleFrameCount;
volatile u32 gNdsIFCommonNativeOamFrameRecognizedCalls;
volatile u32 gNdsIFCommonNativeOamFrameDrawCalls;
volatile u32 gNdsIFCommonNativeOamFrameFallbackCalls;
volatile u32 gNdsIFCommonNativeOamFrameSObjCount;
volatile u32 gNdsIFCommonNativeOamFrameObjectCount;
volatile u32 gNdsIFCommonNativeOamFrameCloudDrawCount;
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
    if ((rgba & 0xffu) < 0x80u)
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

static s32 ndsIFCommonReadRgba32(const Sprite *sprite,
                                  const void *file_data, size_t file_size,
                                  u32 source_x, u32 source_y, u32 *rgba)
{
    const Bitmap *bitmap = sprite->bitmap;
    u32 out_y = 0u;
    u32 bitmap_index;

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
        u32 shuffled_x;
        const u32 *pixels;

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
        /* RGBA32 TEXSHUF swaps the two 64-bit halves of each 128-bit
         * odd-row block, so the inverse address map is two pixels. */
        shuffled_x = source_x ^ ((local_y & 1u) != 0u ? 2u : 0u);
        pixels = (const u32 *)current->buf;
        if (ndsIFCommonRangeValid(
                file_data, file_size, pixels,
                (size_t)width_img * height * sizeof(*pixels)) == FALSE)
        {
            return FALSE;
        }
        memcpy(rgba, &pixels[(local_y * width_img) + shuffled_x],
               sizeof(*rgba));
        return TRUE;
    }
    return FALSE;
}

static u8 ndsIFCommonBilerpChannel(u32 c00, u32 c10,
                                   u32 c01, u32 c11,
                                   u32 fraction_x, u32 fraction_y)
{
    u32 inverse_x = 256u - fraction_x;
    u32 inverse_y = 256u - fraction_y;
    u32 top = (c00 * inverse_x) + (c10 * fraction_x);
    u32 bottom = (c01 * inverse_x) + (c11 * fraction_x);

    return (u8)(((top * inverse_y) + (bottom * fraction_y) +
                 0x8000u) >> 16);
}

static void ndsIFCommonBilerpPremultipliedRgba(
    const u32 taps[4], u32 fraction_x, u32 fraction_y, u8 rgba[4])
{
    u32 weights[4];
    u64 weighted_alpha = 0u;
    u32 tap_index;

    weights[0] = (256u - fraction_x) * (256u - fraction_y);
    weights[1] = fraction_x * (256u - fraction_y);
    weights[2] = (256u - fraction_x) * fraction_y;
    weights[3] = fraction_x * fraction_y;
    for (tap_index = 0u; tap_index < 4u; tap_index++)
    {
        weighted_alpha +=
            (u64)(taps[tap_index] & 0xffu) * weights[tap_index];
    }
    rgba[3] = (u8)((weighted_alpha + 0x8000u) >> 16);
    if (weighted_alpha == 0u)
    {
        rgba[0] = rgba[1] = rgba[2] = 0u;
    }
    else
    {
        static const u8 shifts[3] = { 24u, 16u, 8u };
        u32 channel;

        for (channel = 0u; channel < 3u; channel++)
        {
            u64 weighted_premultiplied = 0u;

            for (tap_index = 0u; tap_index < 4u; tap_index++)
            {
                u32 alpha = taps[tap_index] & 0xffu;
                u32 color = (taps[tap_index] >> shifts[channel]) & 0xffu;

                weighted_premultiplied +=
                    (u64)color * alpha * weights[tap_index];
            }
            rgba[channel] = (u8)((weighted_premultiplied +
                                  (weighted_alpha / 2u)) /
                                 weighted_alpha);
        }
    }
}

static s32 ndsIFCommonSamplePrefilteredGoPixel(
    const Sprite *sprite, const void *file_data, size_t file_size,
    u32 destination_x, u32 destination_y, u8 rgba[4])
{
    u32 source_x_q8 = (destination_x * 320u) + 32u;
    u32 source_y_q8 = (destination_y * 320u) + 32u;
    u32 source_x = source_x_q8 >> 8;
    u32 source_y = source_y_q8 >> 8;
    u32 next_x = source_x + 1u;
    u32 next_y = source_y + 1u;
    u32 rgba00;
    u32 rgba10;
    u32 rgba01;
    u32 rgba11;
    u32 taps[4];
    if ((sprite == NULL) || (rgba == NULL) ||
        (next_x >= (u32)(u16)sprite->width))
    {
        next_x = source_x;
    }
    if (next_y >= (u32)(u16)sprite->height)
    {
        next_y = source_y;
    }
    if ((ndsIFCommonReadRgba32(sprite, file_data, file_size,
                               source_x, source_y, &rgba00) == FALSE) ||
        (ndsIFCommonReadRgba32(sprite, file_data, file_size,
                               next_x, source_y, &rgba10) == FALSE) ||
        (ndsIFCommonReadRgba32(sprite, file_data, file_size,
                               source_x, next_y, &rgba01) == FALSE) ||
        (ndsIFCommonReadRgba32(sprite, file_data, file_size,
                               next_x, next_y, &rgba11) == FALSE))
    {
        return FALSE;
    }

    taps[0] = rgba00;
    taps[1] = rgba10;
    taps[2] = rgba01;
    taps[3] = rgba11;
    ndsIFCommonBilerpPremultipliedRgba(
        taps, source_x_q8 & 0xffu, source_y_q8 & 0xffu, rgba);
    return TRUE;
}

static u16 ndsIFCommonDecodePrefilteredGoPixel(
    const Sprite *sprite, const void *file_data, size_t file_size,
    u32 destination_x, u32 destination_y)
{
    u8 rgba[4];

    if ((ndsIFCommonSamplePrefilteredGoPixel(
             sprite, file_data, file_size,
             destination_x, destination_y, rgba) == FALSE) ||
        (rgba[3] < NDS_IFCOMMON_GO_ALPHA_THRESHOLD))
    {
        return 0u;
    }
    return ndsIFCommonPackRgb15(rgba[0], rgba[1], rgba[2]);
}

static s32 ndsIFCommonReadI8(const Sprite *sprite,
                             const void *file_data, size_t file_size,
                             u32 source_x, u32 source_y, u8 *intensity)
{
    const Bitmap *bitmap = sprite->bitmap;
    u32 out_y = 0u;
    u32 bitmap_index;

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
        u32 shuffled_x;
        const u8 *pixels;

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
        shuffled_x = source_x ^ ((local_y & 1u) != 0u ? 4u : 0u);
        pixels = (const u8 *)current->buf;
        if (ndsIFCommonRangeValid(file_data, file_size, pixels,
                                  (size_t)width_img * height) == FALSE)
        {
            return FALSE;
        }
        *intensity = pixels[((size_t)local_y * width_img + shuffled_x) ^
                            3u];
        return TRUE;
    }
    return FALSE;
}

static s32 ndsIFCommonReadI4(const Sprite *sprite,
                             const void *file_data, size_t file_size,
                             u32 source_x, u32 source_y, u8 *intensity)
{
    const Bitmap *bitmap = sprite->bitmap;
    u32 out_y = 0u;
    u32 bitmap_index;

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
        u32 shuffled_x;
        u32 row_bytes;
        const u8 *pixels;
        u8 packed;

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
        shuffled_x = source_x ^ ((local_y & 1u) != 0u ? 8u : 0u);
        row_bytes = (width_img + 1u) / 2u;
        pixels = (const u8 *)current->buf;
        if (ndsIFCommonRangeValid(file_data, file_size, pixels,
                                  (size_t)row_bytes * height) == FALSE)
        {
            return FALSE;
        }
        packed = pixels[(((size_t)local_y * row_bytes) +
                         (shuffled_x >> 1)) ^ 3u];
        *intensity = ((shuffled_x & 1u) == 0u) ?
            (u8)(packed >> 4) : (u8)(packed & 0x0fu);
        return TRUE;
    }
    return FALSE;
}

static s32 ndsIFCommonReadTrafficRgba(
    const Sprite *sprite, const NDSIFCommonAssetSpec *spec,
    const void *file_data, size_t file_size,
    u32 source_x, u32 source_y, u32 *rgba)
{
    u8 value;
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;

    if ((sprite->bmfmt == G_IM_FMT_IA) &&
        (sprite->bmsiz == G_IM_SIZ_8b))
    {
        u32 intensity;
        u32 inverse;

        if (ndsIFCommonReadI8(sprite, file_data, file_size,
                              source_x, source_y, &value) == FALSE)
        {
            return FALSE;
        }
        intensity = (u32)(value >> 4) * 17u;
        inverse = 255u - intensity;
        red = (u8)(((u32)spec->red * intensity +
                    (u32)spec->env_red * inverse + 127u) / 255u);
        green = (u8)(((u32)spec->green * intensity +
                      (u32)spec->env_green * inverse + 127u) / 255u);
        blue = (u8)(((u32)spec->blue * intensity +
                     (u32)spec->env_blue * inverse + 127u) / 255u);
        alpha = (u8)((value & 0x0fu) * 17u);
    }
    else if ((sprite->bmfmt == G_IM_FMT_I) &&
             (sprite->bmsiz == G_IM_SIZ_4b))
    {
        if (ndsIFCommonReadI4(sprite, file_data, file_size,
                              source_x, source_y, &value) == FALSE)
        {
            return FALSE;
        }
        red = spec->red;
        green = spec->green;
        blue = spec->blue;
        alpha = (u8)(value * 17u);
    }
    else
    {
        return FALSE;
    }
    *rgba = ((u32)red << 24) | ((u32)green << 16) |
            ((u32)blue << 8) | alpha;
    return TRUE;
}

static s32 ndsIFCommonSamplePrefilteredTrafficPixel(
    const Sprite *sprite, const NDSIFCommonAssetSpec *spec,
    const void *file_data, size_t file_size,
    u32 destination_x, u32 destination_y, u8 rgba[4])
{
    u32 source_x_q8 = (destination_x * 320u) + 32u;
    u32 source_y_q8 = (destination_y * 320u) + 32u;
    u32 source_x = source_x_q8 >> 8;
    u32 source_y = source_y_q8 >> 8;
    u32 next_x = source_x + 1u;
    u32 next_y = source_y + 1u;
    u32 taps[4];

    if (next_x >= (u32)(u16)sprite->width)
    {
        next_x = source_x;
    }
    if (next_y >= (u32)(u16)sprite->height)
    {
        next_y = source_y;
    }
    if ((ndsIFCommonReadTrafficRgba(
             sprite, spec, file_data, file_size,
             source_x, source_y, &taps[0]) == FALSE) ||
        (ndsIFCommonReadTrafficRgba(
             sprite, spec, file_data, file_size,
             next_x, source_y, &taps[1]) == FALSE) ||
        (ndsIFCommonReadTrafficRgba(
             sprite, spec, file_data, file_size,
             source_x, next_y, &taps[2]) == FALSE) ||
        (ndsIFCommonReadTrafficRgba(
             sprite, spec, file_data, file_size,
             next_x, next_y, &taps[3]) == FALSE))
    {
        return FALSE;
    }
    ndsIFCommonBilerpPremultipliedRgba(
        taps, source_x_q8 & 0xffu, source_y_q8 & 0xffu, rgba);
    return TRUE;
}

static s32 ndsIFCommonSampleI8Q8(
    const Sprite *sprite, const void *file_data, size_t file_size,
    s32 source_x_q8, s32 source_y_q8, u8 *out_intensity)
{
    u32 source_x;
    u32 source_y;
    u32 next_x;
    u32 next_y;
    u32 fraction_x;
    u32 fraction_y;
    u8 intensity00;
    u8 intensity10;
    u8 intensity01;
    u8 intensity11;

    if ((sprite == NULL) || (out_intensity == NULL) ||
        (sprite->width <= 0) || (sprite->height <= 0))
    {
        return FALSE;
    }
    if (source_x_q8 <= 0)
    {
        source_x = next_x = fraction_x = 0u;
    }
    else
    {
        source_x = (u32)source_x_q8 >> 8;
        fraction_x = (u32)source_x_q8 & 0xffu;
        if (source_x + 1u >= (u32)(u16)sprite->width)
        {
            source_x = (u32)(u16)sprite->width - 1u;
            next_x = source_x;
            fraction_x = 0u;
        }
        else
        {
            next_x = source_x + 1u;
        }
    }
    if (source_y_q8 <= 0)
    {
        source_y = next_y = fraction_y = 0u;
    }
    else
    {
        source_y = (u32)source_y_q8 >> 8;
        fraction_y = (u32)source_y_q8 & 0xffu;
        if (source_y + 1u >= (u32)(u16)sprite->height)
        {
            source_y = (u32)(u16)sprite->height - 1u;
            next_y = source_y;
            fraction_y = 0u;
        }
        else
        {
            next_y = source_y + 1u;
        }
    }
    if ((ndsIFCommonReadI8(sprite, file_data, file_size,
                           source_x, source_y, &intensity00) == FALSE) ||
        (ndsIFCommonReadI8(sprite, file_data, file_size,
                           next_x, source_y, &intensity10) == FALSE) ||
        (ndsIFCommonReadI8(sprite, file_data, file_size,
                           source_x, next_y, &intensity01) == FALSE) ||
        (ndsIFCommonReadI8(sprite, file_data, file_size,
                           next_x, next_y, &intensity11) == FALSE))
    {
        return FALSE;
    }
    *out_intensity = ndsIFCommonBilerpChannel(
        intensity00, intensity10, intensity01, intensity11,
        fraction_x, fraction_y);
    return TRUE;
}

static u16 ndsIFCommonPremultipliedIntensity(
    u8 red, u8 green, u8 blue, u8 intensity)
{
    u32 level = ((u32)intensity * 15u + 127u) / 255u;

    if (level == 0u)
    {
        return 0u;
    }
    return ndsIFCommonPackRgb15(
        (u8)(((u32)red * level + 7u) / 15u),
        (u8)(((u32)green * level + 7u) / 15u),
        (u8)(((u32)blue * level + 7u) / 15u));
}

static s32 ndsIFCommonPrefilterCloudIntensity(
    const Sprite *sprite, const void *file_data, size_t file_size,
    u32 destination_x, u32 destination_y, u8 *out_intensity)
{
    return ndsIFCommonSampleI8Q8(
        sprite, file_data, file_size,
        (s32)(destination_x * 128u) - 64,
        (s32)(destination_y * 128u) - 64, out_intensity);
}

static s32 ndsIFCommonPrefilterLightIntensity(
    const Sprite *sprite, const void *file_data, size_t file_size,
    u32 destination_x, u32 destination_y, u8 *out_intensity)
{
    return ndsIFCommonSampleI8Q8(
        sprite, file_data, file_size,
        (s32)(destination_x * 320u) + 32,
        (s32)(destination_y * 320u) + 32, out_intensity);
}

static u16 ndsIFCommonDecodePrefilteredLightPixel(
    const Sprite *sprite, const void *file_data, size_t file_size,
    u32 destination_x, u32 destination_y)
{
    u8 intensity;

    if (ndsIFCommonPrefilterLightIntensity(
            sprite, file_data, file_size,
            destination_x, destination_y, &intensity) == FALSE)
    {
        return 0u;
    }
    return (intensity >= NDS_IFCOMMON_LIGHT_CORE_THRESHOLD) ?
        ndsIFCommonPackRgb15(255u, 255u, 255u) : 0u;
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
                texshuf_x ^= 2u;
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
            color = ((ia & 0x0fu) >= 8u) ?
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
            color = ndsIFCommonPremultipliedIntensity(
                spec->red, spec->green, spec->blue, intensity);
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
            color = ndsIFCommonPremultipliedIntensity(
                spec->red, spec->green, spec->blue,
                (u8)(intensity * 17u));
        }
        out_y += advance;
    }
    return color;
}

static u16 ndsIFCommonDecodeAssetPixel(
    u32 asset_index, const Sprite *sprite,
    const NDSIFCommonAssetSpec *asset_spec,
    const void *file_data, size_t file_size,
    u32 destination_x, u32 destination_y)
{
    if (asset_index <= nNDSIFCommonAssetGoExclaim)
    {
        return ndsIFCommonDecodePrefilteredGoPixel(
            sprite, file_data, file_size, destination_x, destination_y);
    }
    if ((asset_index >= nNDSIFCommonAssetRedLight) &&
        (asset_index <= nNDSIFCommonAssetBlueLight))
    {
        return ndsIFCommonDecodePrefilteredLightPixel(
            sprite, file_data, file_size, destination_x, destination_y);
    }
    return ndsIFCommonDecodePixel(sprite, asset_spec, file_data, file_size,
                                  destination_x, destination_y);
}

static void ndsIFCommonReleaseCloudAtlases(void)
{
    u32 atlas_index;

    for (atlas_index = 0u;
         atlas_index < NDS_IFCOMMON_CLOUD_ATLAS_COUNT; atlas_index++)
    {
        if (sNdsIFCommonCloudTextureNames[atlas_index] != 0u)
        {
            ndsRendererHardwareReleaseIFCommonCloudAtlas(
                &sNdsIFCommonCloudTextureNames[atlas_index]);
        }
    }
}

typedef struct NDSIFCommonCloudFillContext
{
    const void *file_data;
    size_t file_size;
    u32 atlas_index;
} NDSIFCommonCloudFillContext;

static s32 ndsIFCommonFillCloudAtlas(u8 *pixels, u32 bytes,
                                      void *user_data)
{
    const NDSIFCommonCloudFillContext *context =
        (const NDSIFCommonCloudFillContext *)user_data;
    u32 atlas_width;
    u32 cloud_index;

    if ((pixels == NULL) || (context == NULL) ||
        (context->atlas_index >= NDS_IFCOMMON_CLOUD_ATLAS_COUNT))
    {
        return FALSE;
    }
    atlas_width = sNdsIFCommonCloudAtlasWidths[context->atlas_index];
    if (bytes != atlas_width * NDS_IFCOMMON_CLOUD_ATLAS_HEIGHT)
    {
        return FALSE;
    }
    memset(pixels, 0, bytes);
    for (cloud_index = 0u;
         cloud_index < NDS_IFCOMMON_CLOUD_SPEC_COUNT; cloud_index++)
    {
        const NDSIFCommonCloudSpec *cloud =
            &sNdsIFCommonCloudSpecs[cloud_index];
        const NDSIFCommonNativeAsset *asset;
        u32 y;

        if (cloud->atlas_index != context->atlas_index)
        {
            continue;
        }
        if ((cloud->asset_index >= NDS_IFCOMMON_ASSET_COUNT) ||
            (cloud->kind > nNDSIFCommonCloudContour) ||
            ((u32)cloud->atlas_x + cloud->width >
             atlas_width) ||
            ((u32)cloud->atlas_y + cloud->height >
             NDS_IFCOMMON_CLOUD_ATLAS_HEIGHT))
        {
            gNdsIFCommonNativeOamPrepareCloudFailureStage = 1u;
            return FALSE;
        }
        asset = &sNdsIFCommonAssets[cloud->asset_index];
        for (y = 0u; y < cloud->height; y++)
        {
            u32 x;

            for (x = 0u; x < cloud->width; x++)
            {
                u8 intensity;
                u8 alpha;
                s32 sampled =
                    (cloud->kind == nNDSIFCommonCloudLight) ?
                    ndsIFCommonPrefilterLightIntensity(
                        asset->sprite, context->file_data,
                        context->file_size,
                        (u32)cloud->source_x + x,
                        (u32)cloud->source_y + y, &intensity) :
                    ndsIFCommonPrefilterCloudIntensity(
                        asset->sprite, context->file_data,
                        context->file_size,
                        (u32)cloud->source_x + x,
                        (u32)cloud->source_y + y, &intensity);

                if (sampled == FALSE)
                {
                    gNdsIFCommonNativeOamPrepareCloudFailureStage = 2u;
                    return FALSE;
                }
                if (intensity < NDS_IFCOMMON_CLOUD_ALPHA_THRESHOLD)
                {
                    continue;
                }
                alpha = (u8)(((u32)intensity * 31u + 127u) / 255u);
                gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[
                    cloud_index]++;
                pixels[((u32)cloud->atlas_y + y) *
                           atlas_width +
                       (u32)cloud->atlas_x + x] =
                    (u8)((alpha << 3) | cloud->palette_index);
            }
        }
    }
    return TRUE;
}

static s32 ndsIFCommonPrepareCloudAtlases(
    const void *file_data, size_t file_size)
{
    static const u16 palette[8] = {
        0,
        RGB15(31, 7, 7),
        RGB15(31, 20, 0),
        RGB15(4, 12, 31),
        RGB15(31, 31, 31),
        0, 0, 0
    };
    u32 atlas_index;

    for (atlas_index = 0u;
         atlas_index < NDS_IFCOMMON_CLOUD_ATLAS_COUNT; atlas_index++)
    {
        NDSIFCommonCloudFillContext context = {
            file_data, file_size, atlas_index
        };

        if (ndsRendererHardwarePrepareIFCommonCloudAtlas(
                sNdsIFCommonCloudAtlasWidths[atlas_index],
                NDS_IFCOMMON_CLOUD_ATLAS_HEIGHT, palette,
                ndsIFCommonFillCloudAtlas, &context,
                &sNdsIFCommonCloudTextureNames[atlas_index]) == FALSE)
        {
            if (gNdsIFCommonNativeOamPrepareCloudFailureStage == 0u)
            {
                gNdsIFCommonNativeOamPrepareCloudFailureStage = 3u;
            }
            ndsIFCommonReleaseCloudAtlases();
            return FALSE;
        }
        gNdsIFCommonNativeOamPrepareCloudTextureBytes +=
            (u32)sNdsIFCommonCloudAtlasWidths[atlas_index] *
                NDS_IFCOMMON_CLOUD_ATLAS_HEIGHT;
        gNdsIFCommonNativeOamPrepareCloudTextureCount++;
    }
    gNdsIFCommonNativeOamPreparePaletteBytes +=
        sizeof(palette) * NDS_IFCOMMON_CLOUD_ATLAS_COUNT;
    return TRUE;
}

static u16 ndsIFCommonRgb15(u8 red, u8 green, u8 blue)
{
    return (u16)((red >> 3) | ((green >> 3) << 5) | ((blue >> 3) << 10));
}

static u32 ndsIFCommonColorDistance(u16 left, u16 right)
{
    s32 delta_red = (s32)(left & 31u) - (s32)(right & 31u);
    s32 delta_green = (s32)((left >> 5) & 31u) -
                      (s32)((right >> 5) & 31u);
    s32 delta_blue = (s32)((left >> 10) & 31u) -
                     (s32)((right >> 10) & 31u);

    return (u32)((delta_red * delta_red) +
                 (delta_green * delta_green) +
                 (delta_blue * delta_blue));
}

static u32 ndsIFCommonPaletteIndex(
    const u16 palette[32], u8 red, u8 green, u8 blue)
{
    u16 source = ndsIFCommonRgb15(red, green, blue);
    u32 best_index = 1u;
    u32 best_distance = UINT32_MAX;
    u32 index;

    for (index = 1u; index < 32u; index++)
    {
        u32 distance = ndsIFCommonColorDistance(source, palette[index]);

        if (distance < best_distance)
        {
            best_distance = distance;
            best_index = index;
        }
    }
    return best_index;
}

/* Frequency-ranked from the exact prefiltered source traffic pixels. Index 0
 * is transparent; the second zero is the visible black used by the shadow. */
static const u16 sNdsIFCommonTrafficPalette[32] = {
    0x0000, 0x1084, 0x0000, 0x0842, 0x18c6, 0x14a5, 0x38a4, 0x0c63,
    0x1ce7, 0x0421, 0x2108, 0x000e, 0x3def, 0x2529, 0x1863, 0x012e,
    0x5ef7, 0x294a, 0x35ad, 0x24a4, 0x2d6b, 0x318c, 0x4e73, 0x39ce,
    0x56b5, 0x000b, 0x4a52, 0x6739, 0x4631, 0x77bd, 0x00a8, 0x0006
};

static void ndsIFCommonReleaseTrafficAtlas(void)
{
    if (sNdsIFCommonTrafficTextureName != 0u)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(
            &sNdsIFCommonTrafficTextureName);
    }
}

typedef struct NDSIFCommonTrafficFillContext
{
    const void *file_data;
    size_t file_size;
} NDSIFCommonTrafficFillContext;

static s32 ndsIFCommonFillTrafficAtlas(
    u8 *pixels, u32 bytes, void *user_data)
{
    const NDSIFCommonTrafficFillContext *context =
        (const NDSIFCommonTrafficFillContext *)user_data;
    u32 traffic_index;

    if ((pixels == NULL) || (context == NULL) ||
        (bytes != NDS_IFCOMMON_TRAFFIC_ATLAS_BYTES))
    {
        return FALSE;
    }
    memset(pixels, 0, bytes);
    for (traffic_index = 0u;
         traffic_index < NDS_IFCOMMON_TRAFFIC_SPEC_COUNT; traffic_index++)
    {
        const NDSIFCommonTrafficSpec *traffic =
            &sNdsIFCommonTrafficSpecs[traffic_index];
        const NDSIFCommonNativeAsset *asset;
        const NDSIFCommonAssetSpec *asset_spec;
        u32 y;

        if ((traffic->asset_index < nNDSIFCommonAssetRod) ||
            (traffic->asset_index > nNDSIFCommonAssetBlueDim) ||
            ((u32)traffic->atlas_x + traffic->width >
             NDS_IFCOMMON_TRAFFIC_ATLAS_WIDTH) ||
            ((u32)traffic->atlas_y + traffic->height >
             NDS_IFCOMMON_TRAFFIC_ATLAS_HEIGHT))
        {
            gNdsIFCommonNativeOamPrepareCloudFailureStage = 1u;
            return FALSE;
        }
        asset = &sNdsIFCommonAssets[traffic->asset_index];
        asset_spec = &sNdsIFCommonAssetSpecs[traffic->asset_index];
        for (y = 0u; y < traffic->height; y++)
        {
            u32 x;

            for (x = 0u; x < traffic->width; x++)
            {
                u8 rgba[4];
                u8 red;
                u8 green;
                u8 blue;
                u32 palette_index;

                if (ndsIFCommonSamplePrefilteredTrafficPixel(
                        asset->sprite, asset_spec,
                        context->file_data, context->file_size,
                        x, y, rgba) == FALSE)
                {
                    gNdsIFCommonNativeOamPrepareCloudFailureStage = 2u;
                    return FALSE;
                }
                if (rgba[3] < NDS_IFCOMMON_TRAFFIC_ALPHA_THRESHOLD)
                {
                    continue;
                }
                /* The housing and dim lamps are opaque source art. Bake the
                 * filtered coverage into RGB so their shading survives, then
                 * use A3 only as a hard cutout mask. */
                red = (u8)(((u32)rgba[0] * rgba[3] + 127u) / 255u);
                green = (u8)(((u32)rgba[1] * rgba[3] + 127u) / 255u);
                blue = (u8)(((u32)rgba[2] * rgba[3] + 127u) / 255u);
                palette_index = ndsIFCommonPaletteIndex(
                    sNdsIFCommonTrafficPalette,
                    red, green, blue);
                gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[
                    NDS_IFCOMMON_TRAFFIC_PROOF_OFFSET + traffic_index]++;
                pixels[((u32)traffic->atlas_y + y) *
                           NDS_IFCOMMON_TRAFFIC_ATLAS_WIDTH +
                       (u32)traffic->atlas_x + x] =
                    (u8)((7u << 5) | palette_index);
            }
        }
    }
    return TRUE;
}

static s32 ndsIFCommonPrepareTrafficAtlas(
    const void *file_data, size_t file_size)
{
    NDSIFCommonTrafficFillContext context = { file_data, file_size };

    if (ndsRendererHardwarePrepareIFCommonA3I5Atlas(
            NDS_IFCOMMON_TRAFFIC_ATLAS_WIDTH,
            NDS_IFCOMMON_TRAFFIC_ATLAS_HEIGHT,
            sNdsIFCommonTrafficPalette,
            ndsIFCommonFillTrafficAtlas, &context,
            &sNdsIFCommonTrafficTextureName) == FALSE)
    {
        if (gNdsIFCommonNativeOamPrepareCloudFailureStage == 0u)
        {
            gNdsIFCommonNativeOamPrepareCloudFailureStage = 3u;
        }
        ndsIFCommonReleaseTrafficAtlas();
        return FALSE;
    }
    gNdsIFCommonNativeOamPrepareCloudTextureBytes +=
        NDS_IFCOMMON_TRAFFIC_ATLAS_BYTES;
    gNdsIFCommonNativeOamPrepareCloudTextureCount++;
    gNdsIFCommonNativeOamPreparePaletteBytes +=
        sizeof(sNdsIFCommonTrafficPalette);
    return TRUE;
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

static s32 ndsIFCommonHybridPaletteIndex(const u16 *palette,
                                          u32 palette_entries, u16 color)
{
    u32 palette_index;

    if (color == 0u)
    {
        return 0;
    }
    for (palette_index = 1u; palette_index < palette_entries;
         palette_index++)
    {
        if (palette[palette_index] == color)
        {
            return (s32)palette_index;
        }
    }
    return -1;
}

static s32 ndsIFCommonBuildHybridPalette(const void *file_data,
                                          size_t file_size, u16 *palette,
                                          u32 *palette_entries)
{
    u32 asset_index;
    u32 entry_count = 1u;

    memset(palette, 0,
           sizeof(*palette) * NDS_IFCOMMON_OBJ_PALETTE_ENTRIES);
    for (asset_index = nNDSIFCommonAssetRod;
         asset_index < NDS_IFCOMMON_ASSET_COUNT; asset_index++)
    {
        const NDSIFCommonAssetSpec *spec =
            &sNdsIFCommonAssetSpecs[asset_index];
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
        for (tile_index = 0u; tile_index < spec->tile_count; tile_index++)
        {
            const NDSIFCommonTileSpec *tile = &spec->tiles[tile_index];
            u32 y;

            for (y = 0u; y < tile->content_height; y++)
            {
                u32 x;

                for (x = 0u; x < tile->content_width; x++)
                {
                    u16 color = ndsIFCommonDecodeAssetPixel(
                        asset_index, sprite, spec,
                        file_data, file_size,
                        (u32)tile->source_x + x,
                        (u32)tile->source_y + y);

                    if (ndsIFCommonHybridPaletteIndex(
                            palette, entry_count, color) < 0)
                    {
                        if (entry_count >=
                            NDS_IFCOMMON_OBJ_PALETTE_ENTRIES)
                        {
                            return FALSE;
                        }
                        palette[entry_count++] = color;
                    }
                }
            }
        }
    }
    *palette_entries = entry_count;
    return TRUE;
}

static s32 ndsIFCommonPrepareAsset(u32 asset_index, const void *file_data,
                                   size_t file_size, u32 *vram_cursor,
                                   const u16 *palette,
                                   u32 palette_entries)
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
    asset->sprite = sprite;
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
        s32 indexed = (asset_index >= nNDSIFCommonAssetRod) ? TRUE : FALSE;
        u32 bytes;

        *vram_cursor = (*vram_cursor +
                        (NDS_IFCOMMON_OBJ_GFX_ALIGNMENT - 1u)) &
                       ~(NDS_IFCOMMON_OBJ_GFX_ALIGNMENT - 1u);
        bytes = (u32)tile_spec->cell_width *
                (u32)tile_spec->cell_height *
                (indexed ? sizeof(u8) : sizeof(u16));
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
        tile->color_format = indexed ? SpriteColorFormat_256Color :
                                       SpriteColorFormat_Bmp;
        tile->spec = *tile_spec;
        if ((u32)tile->size == 0u)
        {
            return FALSE;
        }

        dmaFillHalfWords(0u, tile->gfx, bytes);
        for (y = 0u; y < tile_spec->content_height; y++)
        {
            u32 x;

            for (x = 0u; x < tile_spec->content_width; x++)
            {
                u16 color = ndsIFCommonDecodeAssetPixel(
                    asset_index, sprite, spec,
                    file_data, file_size,
                    (u32)tile_spec->source_x + x,
                    (u32)tile_spec->source_y + y);

                if (indexed != FALSE)
                {
                    u32 destination_x = (u32)tile_spec->pad_x + x;
                    u32 destination_y = (u32)tile_spec->pad_y + y;
                    u32 tile_offset =
                        (((destination_y >> 3) *
                              ((u32)tile_spec->cell_width >> 3) +
                          (destination_x >> 3)) << 6) +
                        ((destination_y & 7u) << 3) +
                        (destination_x & 7u);
                    s32 palette_index = ndsIFCommonHybridPaletteIndex(
                        palette, palette_entries, color);

                    if (palette_index < 0)
                    {
                        return FALSE;
                    }
                    ((u8 *)tile->gfx)[tile_offset] = (u8)palette_index;
                }
                else
                {
                    tile->gfx[
                        (((u32)tile_spec->pad_y + y) *
                             tile_spec->cell_width) +
                        tile_spec->pad_x + x] = color;
                }
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
    /* Traffic conversion and all four IFCommon atlases happen before
     * gameplay; the draw path only emits retained hardware handles. */
    gNdsIFCommonNativeOamPreparePaletteBytes = 0u;
    gNdsIFCommonNativeOamPrepareCloudTextureBytes = 0u;
    gNdsIFCommonNativeOamPrepareCloudTextureCount = 0u;
    gNdsIFCommonNativeOamPrepareCloudFailureStage = 0u;
    memset((void *)gNdsIFCommonNativeOamPrepareCloudNonzeroTexels, 0,
           sizeof(gNdsIFCommonNativeOamPrepareCloudNonzeroTexels));
    gNdsIFCommonNativeOamHotConvertCount = 0u;
    gNdsIFCommonNativeOamRuntimeUploadBytes = 0u;
    memset(sNdsIFCommonCloudTextureNames, 0,
           sizeof(sNdsIFCommonCloudTextureNames));
    sNdsIFCommonTrafficTextureName = 0u;
    sNdsIFCommonPreparedFileSize = 0u;
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
    u16 palette_storage[NDS_IFCOMMON_OBJ_PALETTE_ENTRIES];
    const u16 *palette = palette_storage;
    u32 palette_entries = 0u;

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
    gNdsIFCommonNativeOamPrepareBytes = 0u;
    gNdsIFCommonNativeOamPreparePaletteBytes = 0u;
    gNdsIFCommonNativeOamPrepareCloudTextureBytes = 0u;
    gNdsIFCommonNativeOamPrepareCloudTextureCount = 0u;
    gNdsIFCommonNativeOamPrepareCloudFailureStage = 0u;
    memset((void *)gNdsIFCommonNativeOamPrepareCloudNonzeroTexels, 0,
           sizeof(gNdsIFCommonNativeOamPrepareCloudNonzeroTexels));
    gNdsIFCommonNativeOamPrepareProfileFrame =
        gNdsRendererProfileFrameCount;
    ndsIFCommonReleaseCloudAtlases();
    ndsIFCommonReleaseTrafficAtlas();
    memset(sNdsIFCommonAssets, 0, sizeof(sNdsIFCommonAssets));
    sNdsIFCommonPrepared = FALSE;
    sNdsIFCommonPreparedFile = NULL;
    sNdsIFCommonPreparedFileSize = 0u;

    if (ndsIFCommonBuildHybridPalette(file_data, file_size,
                                      palette_storage,
                                      &palette_entries) == FALSE)
    {
        gNdsIFCommonNativeOamPrepareFailCount++;
        gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackBadAsset;
        return FALSE;
    }

    for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
         asset_index++)
    {
        if (ndsIFCommonPrepareAsset(asset_index, file_data, file_size,
                                    &vram_cursor, palette,
                                    palette_entries) == FALSE)
        {
            gNdsIFCommonNativeOamPrepareFailCount++;
            gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            return FALSE;
        }
    }

    memcpy(SPRITE_PALETTE, palette_storage, sizeof(palette_storage));
    gNdsIFCommonNativeOamPreparePaletteBytes += sizeof(palette_storage);
    sNdsIFCommonPrepared = TRUE;
    sNdsIFCommonPreparedFile = file_data;
    sNdsIFCommonPreparedFileSize = file_size;
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

s32 ndsIFCommonNativeOamPrepareClouds(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 start;

    if ((sNdsIFCommonPrepared == FALSE) ||
        (sNdsIFCommonPreparedFile == NULL) ||
        (sNdsIFCommonPreparedFileSize < NDS_IFCOMMON_GAME_STATUS_SIZE))
    {
        gNdsIFCommonNativeOamPrepareCloudFailureStage = 1u;
        gNdsIFCommonNativeOamPrepareFailCount++;
        return FALSE;
    }
    if ((sNdsIFCommonCloudTextureNames[0] != 0u) &&
        (sNdsIFCommonCloudTextureNames[1] != 0u) &&
        (sNdsIFCommonTrafficTextureName != 0u))
    {
        return TRUE;
    }

    start = cpuGetTiming();
    ndsIFCommonReleaseCloudAtlases();
    ndsIFCommonReleaseTrafficAtlas();
    gNdsIFCommonNativeOamPrepareCloudTextureBytes = 0u;
    gNdsIFCommonNativeOamPrepareCloudTextureCount = 0u;
    gNdsIFCommonNativeOamPrepareCloudFailureStage = 0u;
    memset((void *)gNdsIFCommonNativeOamPrepareCloudNonzeroTexels, 0,
           sizeof(gNdsIFCommonNativeOamPrepareCloudNonzeroTexels));
    if ((ndsIFCommonPrepareCloudAtlases(
             sNdsIFCommonPreparedFile,
             sNdsIFCommonPreparedFileSize) == FALSE) ||
        (ndsIFCommonPrepareTrafficAtlas(
            sNdsIFCommonPreparedFile,
            sNdsIFCommonPreparedFileSize) == FALSE))
    {
        ndsIFCommonReleaseCloudAtlases();
        ndsIFCommonReleaseTrafficAtlas();
        gNdsIFCommonNativeOamPrepareFailCount++;
        gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackBadAsset;
        return FALSE;
    }
    gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
    return TRUE;
#else
    return FALSE;
#endif
}

void ndsIFCommonNativeOamBeginFrame(void)
{
    gNdsIFCommonNativeOamFrameBeginTicks = 0u;
    gNdsIFCommonNativeOamFrameCommitTicks = 0u;
    gNdsIFCommonNativeOamFrameCommitCalls = 0u;
    gNdsIFCommonNativeOamFrameClearedObjects = 0u;
    gNdsIFCommonNativeOamFrameIdle = 0u;
    sNdsIFCommonFrameNeedsCommit = FALSE;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsIFCommonPreviousLowestOamID < 128)
    {
        u32 start = cpuGetTiming();

        gNdsIFCommonNativeOamFrameClearedObjects =
            (u32)(128 - sNdsIFCommonPreviousLowestOamID);
        oamClear(&oamMain, sNdsIFCommonPreviousLowestOamID,
                 (int)gNdsIFCommonNativeOamFrameClearedObjects);
        gNdsIFCommonNativeOamFrameBeginTicks = cpuGetTiming() - start;
        sNdsIFCommonFrameNeedsCommit = TRUE;
    }
#endif
    sNdsIFCommonNextOamID = 127;
    sNdsIFCommonMatrixCount = 0u;
    gNdsIFCommonNativeOamFrameTicks = 0u;
    gNdsIFCommonNativeOamFrameRecognizedCalls = 0u;
    gNdsIFCommonNativeOamFrameDrawCalls = 0u;
    gNdsIFCommonNativeOamFrameFallbackCalls = 0u;
    gNdsIFCommonNativeOamFrameSObjCount = 0u;
    gNdsIFCommonNativeOamFrameObjectCount = 0u;
    gNdsIFCommonNativeOamFrameCloudDrawCount = 0u;
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

static u32 ndsIFCommonBitmapAlpha(u8 source_alpha)
{
    if (source_alpha == 0u)
    {
        return 0u;
    }
    return (((u32)source_alpha * 15u) + 127u) / 255u;
}

static s32 ndsIFCommonRoundFloatHalfUp(f32 value)
{
    return (value >= 0.0F) ? (s32)(value + 0.5F) :
                             (s32)(value - 0.5F);
}

static s32 ndsIFCommonRoundQ16HalfUp(s32 value)
{
    return (value >= 0) ? ((value + 0x8000) >> 16) :
                          -(((-value) + 0x8000) >> 16);
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
    s32 origin_x;
    s32 origin_y;
    u32 tile_index;
    s32 size_double;
    u32 prefiltered;

    for (tile_index = 0u; tile_index < asset->tile_count; tile_index++)
    {
        if ((asset->tiles[tile_index].color_format !=
             SpriteColorFormat_Bmp) && (sobj->sprite.alpha != 255u))
        {
            return FALSE;
        }
    }
    prefiltered = ((asset_index <= nNDSIFCommonAssetGoExclaim) ||
                   ((asset_index >= nNDSIFCommonAssetRedLight) &&
                    (asset_index <= nNDSIFCommonAssetBlueLight))) ?
                      TRUE : FALSE;

    scale_x_q16 = (u32)((sobj->sprite.scalex *
        (f32)(prefiltered ? (1u << 16) :
                            NDS_IFCOMMON_SCREEN_SCALE_Q16)) + 0.5F);
    scale_y_q16 = (u32)((sobj->sprite.scaley *
        (f32)(prefiltered ? (1u << 16) :
                            NDS_IFCOMMON_SCREEN_SCALE_Q16)) + 0.5F);
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
    origin_x = ndsIFCommonRoundQ16HalfUp(ndsIFCommonRoundFloatHalfUp(
        sobj->pos.x * (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16));
    origin_y = ndsIFCommonRoundQ16HalfUp(ndsIFCommonRoundFloatHalfUp(
        sobj->pos.y * (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16));
    size_double = (scale_x_q16 > (1u << 16)) ? TRUE : FALSE;

    for (tile_index = 0u; tile_index < asset->tile_count; tile_index++)
    {
        const NDSIFCommonNativeTile *tile = &asset->tiles[tile_index];
        const NDSIFCommonTileSpec *spec = &tile->spec;
        s32 local_center_x = (s32)spec->source_x +
            ((s32)spec->cell_width / 2) - (s32)spec->pad_x;
        s32 local_center_y = (s32)spec->source_y +
            ((s32)spec->cell_height / 2) - (s32)spec->pad_y;
        s32 center_x = origin_x + ndsIFCommonRoundQ16HalfUp(
            (s32)((s64)local_center_x * (s64)scale_x_q16));
        s32 center_y = origin_y + ndsIFCommonRoundQ16HalfUp(
            (s32)((s64)local_center_y * (s64)scale_y_q16));
        s32 half_bounds_x = size_double ? spec->cell_width :
                                              (spec->cell_width / 2);
        s32 half_bounds_y = size_double ? spec->cell_height :
                                              (spec->cell_height / 2);
        s32 x = center_x - half_bounds_x;
        s32 y = center_y - half_bounds_y;
        s32 oam_id = sNdsIFCommonNextOamID;
        u32 bitmap_alpha = ndsIFCommonBitmapAlpha(sobj->sprite.alpha);

        oamSet(&oamMain, oam_id, x, y, 0,
               (tile->color_format == SpriteColorFormat_Bmp) ?
                   (int)bitmap_alpha : 0,
               tile->size, (SpriteColorFormat)tile->color_format, tile->gfx,
               matrix_index, size_double, false, false, false, false);
        sNdsIFCommonNextOamID--;
        gNdsIFCommonNativeOamFrameObjectCount++;
    }
    return TRUE;
}

static const NDSIFCommonTrafficSpec *ndsIFCommonTrafficSpecForAsset(
    u32 asset_index)
{
    u32 traffic_index;

    for (traffic_index = 0u;
         traffic_index < NDS_IFCOMMON_TRAFFIC_SPEC_COUNT; traffic_index++)
    {
        if ((u32)sNdsIFCommonTrafficSpecs[traffic_index].asset_index ==
            asset_index)
        {
            return &sNdsIFCommonTrafficSpecs[traffic_index];
        }
    }
    return NULL;
}

static s32 ndsIFCommonTrafficSObjValid(
    const SObj *sobj, const NDSIFCommonTrafficSpec *traffic)
{
    u32 scale_x_q16;
    u32 scale_y_q16;

    if ((sobj == NULL) || (traffic == NULL) ||
        (sNdsIFCommonTrafficTextureName == 0u) ||
        (sobj->sprite.alpha != 255u))
    {
        return FALSE;
    }
    scale_x_q16 = (u32)((sobj->sprite.scalex *
        (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16) + 0.5F);
    scale_y_q16 = (u32)((sobj->sprite.scaley *
        (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16) + 0.5F);
    return ((scale_x_q16 == NDS_IFCOMMON_SCREEN_SCALE_Q16) &&
            (scale_y_q16 == NDS_IFCOMMON_SCREEN_SCALE_Q16)) ? TRUE : FALSE;
}

static s32 ndsIFCommonEmitTrafficSObj(
    const SObj *sobj, const NDSIFCommonTrafficSpec *traffic)
{
    s32 origin_x = ndsIFCommonRoundQ16HalfUp(
        ndsIFCommonRoundFloatHalfUp(
            sobj->pos.x * (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16));
    s32 origin_y = ndsIFCommonRoundQ16HalfUp(
        ndsIFCommonRoundFloatHalfUp(
            sobj->pos.y * (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16));

    if (ndsRendererHardwareDrawIFCommonCloudAtlas(
            sNdsIFCommonTrafficTextureName,
            origin_x * (s32)(1u << 16),
            origin_y * (s32)(1u << 16),
            (s32)((u32)traffic->width << 16),
            (s32)((u32)traffic->height << 16),
            traffic->atlas_x, traffic->atlas_y,
            traffic->width, traffic->height,
            48u + traffic->asset_index) == FALSE)
    {
        return FALSE;
    }
    gNdsIFCommonNativeOamFrameCloudDrawCount++;
    return TRUE;
}

static const NDSIFCommonCloudSpec *ndsIFCommonCloudSpecForAsset(
    u32 asset_index)
{
    u32 cloud_index;

    for (cloud_index = 0u;
         cloud_index < NDS_IFCOMMON_CLOUD_SPEC_COUNT; cloud_index++)
    {
        if ((u32)sNdsIFCommonCloudSpecs[cloud_index].asset_index ==
            asset_index)
        {
            return &sNdsIFCommonCloudSpecs[cloud_index];
        }
    }
    return NULL;
}

static s32 ndsIFCommonCloudSObjValid(
    const SObj *sobj, const NDSIFCommonCloudSpec *cloud)
{
    u32 scale_x_q16;
    u32 scale_y_q16;

    if ((sobj == NULL) || (cloud == NULL))
    {
        return FALSE;
    }
    if ((cloud->atlas_index >= NDS_IFCOMMON_CLOUD_ATLAS_COUNT) ||
        (sNdsIFCommonCloudTextureNames[cloud->atlas_index] == 0u) ||
        (sobj->sprite.alpha != 255u))
    {
        return FALSE;
    }
    scale_x_q16 = (u32)((sobj->sprite.scalex *
        (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16) + 0.5F);
    scale_y_q16 = (u32)((sobj->sprite.scaley *
        (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16) + 0.5F);
    if ((scale_x_q16 == 0u) || (scale_x_q16 != scale_y_q16))
    {
        return FALSE;
    }
    return ((cloud->kind != nNDSIFCommonCloudLight) ||
            (scale_x_q16 == NDS_IFCOMMON_SCREEN_SCALE_Q16)) ? TRUE : FALSE;
}

static s32 ndsIFCommonEmitCloudSObj(
    const SObj *sobj, const NDSIFCommonCloudSpec *cloud)
{
    s32 origin_x_q16 = ndsIFCommonRoundFloatHalfUp(
        sobj->pos.x * (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16);
    s32 origin_y_q16 = ndsIFCommonRoundFloatHalfUp(
        sobj->pos.y * (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16);
    u32 source_scale_q16 = (u32)((sobj->sprite.scalex *
        (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16) + 0.5F);
    u32 atlas_scale_q16 = (cloud->kind == nNDSIFCommonCloudLight) ?
        (1u << 16) : (source_scale_q16 + 1u) / 2u;
    s32 x_q16 = origin_x_q16 +
        (s32)((u32)cloud->source_x * atlas_scale_q16);
    s32 y_q16 = origin_y_q16 +
        (s32)((u32)cloud->source_y * atlas_scale_q16);
    s32 width_q16 = (s32)((u32)cloud->width * atlas_scale_q16);
    s32 height_q16 = (s32)((u32)cloud->height * atlas_scale_q16);
    u32 poly_id = (cloud->kind == nNDSIFCommonCloudLight) ? 63u : 62u;

    if (ndsRendererHardwareDrawIFCommonCloudAtlas(
            sNdsIFCommonCloudTextureNames[cloud->atlas_index],
            x_q16, y_q16, width_q16, height_q16,
            cloud->atlas_x, cloud->atlas_y,
            cloud->width, cloud->height, poly_id) == FALSE)
    {
        return FALSE;
    }
    gNdsIFCommonNativeOamFrameCloudDrawCount++;
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
        const NDSIFCommonTrafficSpec *traffic;
        const NDSIFCommonCloudSpec *cloud;

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
        traffic = ndsIFCommonTrafficSpecForAsset((u32)asset_index);
        cloud = ndsIFCommonCloudSpecForAsset((u32)asset_index);
        if (((traffic != NULL) &&
             (ndsIFCommonTrafficSObjValid(scan, traffic) == FALSE)) ||
            ((cloud != NULL) &&
             (ndsIFCommonCloudSObjValid(scan, cloud) == FALSE)))
        {
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            gNdsIFCommonNativeOamFrameTicks += cpuGetTiming() - start;
            return FALSE;
        }
        if ((traffic == NULL) && (cloud == NULL))
        {
            required_objects += sNdsIFCommonAssets[asset_index].tile_count;
        }
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
        if ((asset_index >= 0) &&
            ((ndsIFCommonTrafficSpecForAsset((u32)asset_index) != NULL) ||
             (ndsIFCommonCloudSpecForAsset((u32)asset_index) != NULL)))
        {
            continue;
        }
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
    for (scan = sobj; scan != NULL; scan = scan->next)
    {
        s32 asset_index;
        const NDSIFCommonTrafficSpec *traffic;
        const NDSIFCommonCloudSpec *cloud;

        if ((scan->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        asset_index = ndsIFCommonAssetForSObj(scan);
        traffic = (asset_index >= 0) ?
            ndsIFCommonTrafficSpecForAsset((u32)asset_index) : NULL;
        cloud = (asset_index >= 0) ?
            ndsIFCommonCloudSpecForAsset((u32)asset_index) : NULL;
        if (((traffic != NULL) &&
             (ndsIFCommonEmitTrafficSObj(scan, traffic) == FALSE)) ||
            ((cloud != NULL) &&
             (ndsIFCommonEmitCloudSObj(scan, cloud) == FALSE)))
        {
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            gNdsIFCommonNativeOamFrameTicks += cpuGetTiming() - start;
            return FALSE;
        }
    }
    gNdsIFCommonNativeOamFrameDrawCalls++;
    if (gNdsIFCommonNativeOamFrameObjectCount != object_count_before)
    {
        sNdsIFCommonFrameNeedsCommit = TRUE;
    }
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
    if (sNdsIFCommonFrameNeedsCommit != FALSE)
    {
        u32 start = cpuGetTiming();

        oamUpdate(&oamMain);
        gNdsIFCommonNativeOamFrameCommitTicks = cpuGetTiming() - start;
        gNdsIFCommonNativeOamFrameCommitCalls = 1u;
        gNdsIFCommonNativeOamCommitCount++;
    }
    if (gNdsIFCommonNativeOamFrameObjectCount != 0u)
    {
        sNdsIFCommonPreviousLowestOamID = sNdsIFCommonNextOamID + 1;
    }
    else
    {
        sNdsIFCommonPreviousLowestOamID = 128;
    }
    if (sNdsIFCommonFrameNeedsCommit == FALSE)
    {
        gNdsIFCommonNativeOamFrameIdle = 1u;
        gNdsIFCommonNativeOamIdleFrameCount++;
    }
#endif
}
