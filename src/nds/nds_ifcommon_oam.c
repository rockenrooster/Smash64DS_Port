#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <nds/arm9/sprite.h>
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <nds/timers.h>
#include <PR/sp.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_platform.h>

#if NDS_SHIP_TELEMETRY
#define NDS_IFCOMMON_TELEMETRY_TICK() cpuGetTiming()
#define NDS_IFCOMMON_TELEMETRY_ADD(dst, start) \
    ((dst) += cpuGetTiming() - (start))
#else
#define NDS_IFCOMMON_TELEMETRY_TICK() 0u
#define NDS_IFCOMMON_TELEMETRY_ADD(dst, start) ((void)(start))
#endif
#include <nds/nds_renderer.h>
#include <nds/nds_startup.h>
#include <nds/nds_task39_effect_census.h>
#include <gm/generic.h>
#include <sys/obj.h>
#include <ft/fighter.h>
#include <sc/scene.h>
#include <if/interface.h>
#if NDS_P2_1P_GAME
#include <gr/ground.h>
#endif
#include <nds/nds_reloc_assets.h>

/* lbcommon.c's generic SObj display callback (the port's lbcommon header is
 * not included here). */
extern void lbCommonDrawSObjAttr(GObj *gobj);

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
#define NDS_IFCOMMON_ASSET_COUNT 25u
#define NDS_IFCOMMON_MAX_TILES 9u
/* Fixed OBJ bank plan: GO stays resident (GO can still be live when the
 * end message arrives), the end bank is shared TIME UP vs GAME SET only
 * (source-exclusive per round), then sparks, tags, and the common item arrow.
 * GO 17408 + END 20736 + SPARK 22528 + TAG 3072 + ITEM 64 = 63808 < 65536. */
#define NDS_IFCOMMON_GO_BANK_BASE 0u
#define NDS_IFCOMMON_GO_BANK_BYTES 17408u
#define NDS_IFCOMMON_END_BANK_BASE 17408u
#define NDS_IFCOMMON_END_BANK_BYTES 20736u
#define NDS_IFCOMMON_SPARK_BANK_BASE 38144u
#define NDS_IFCOMMON_SPARK_BANK_BYTES 22528u
#define NDS_IFCOMMON_TAG_BANK_BASE 60672u
#define NDS_IFCOMMON_TAG_BANK_BYTES 3072u
#define NDS_IFCOMMON_ITEM_BANK_BASE 63744u
#define NDS_IFCOMMON_ITEM_BANK_BYTES 64u
/* The 1P bonus boards' task icon (one target or platform, 16x16 4bpp), on
 * the next 128-byte OBJ tile boundary. */
#define NDS_IFCOMMON_TASK_BANK_BASE 63872u
#define NDS_IFCOMMON_TASK_BANK_BYTES 128u
#define NDS_IFCOMMON_USED_BYTES 64000u
#define NDS_IFCOMMON_ANNOUNCE_TIME_UP 0u
#define NDS_IFCOMMON_ANNOUNCE_GAME_SET 1u
#define NDS_IFCOMMON_END_FIRST 16u
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

#define NDS_TASK39_HIT_SPARK_CAPACITY 16u
#define NDS_TASK39_HIT_SPARK_CELL_BYTES 512u
#define NDS_TASK39_HIT_SPARK_FRAME_COUNT 44u
#define NDS_TASK39_HIT_SPARK_LIGHT_FRAMES 8u
#define NDS_TASK39_HIT_SPARK_LIGHT_LIFETIME 17u
#define NDS_TASK39_HIT_SPARK_HEAVY_OFFSET 32u
#define NDS_TASK39_HIT_SPARK_HEAVY_FRAMES 3u
#define NDS_TASK39_HIT_SPARK_HEAVY_LIFETIME 6u
#define NDS_TASK39_HIT_SPARK_SCREEN_SCALE 1.6F

#if NDS_TASK39_FX_SPRITES
#include "generated/task39_hit_sparks.generated.inc"
#endif

extern Mtx44f gGMCameraMatrix;
extern GObj *gGMCameraGObj;
extern f32 syUtilsRandFloat(void);
extern f32 __cosf(f32 value);
extern f32 __sinf(f32 value);
extern void func_ovl2_800EB924(CObj *cobj, Mtx44f matrix, Vec3f *pos,
                               f32 *dist_x, f32 *dist_y);
extern sb32 gmCameraCheckTargetInBounds(f32 pos_x, f32 pos_y);

#ifndef CObjGetStruct
#define CObjGetStruct(gobj) ((CObj *)((gobj)->obj))
#endif

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
    nNDSIFCommonAssetBlueContour,
    nNDSIFCommonAssetEndT,
    nNDSIFCommonAssetEndI,
    nNDSIFCommonAssetEndM,
    nNDSIFCommonAssetEndE,
    nNDSIFCommonAssetEndU,
    nNDSIFCommonAssetEndP,
    nNDSIFCommonAssetEndS,
    nNDSIFCommonAssetEndA,
    nNDSIFCommonAssetEndG
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

typedef struct NDSTask39HitSpark
{
    Vec3f pos;
    Vec3f vel;
    f32 scale;
    s16 size;
    u8 age;
    u8 player;
    u8 is_heavy;
    u8 active;
} NDSTask39HitSpark;

#define TILE(sx, sy, sw, sh, cw, ch, px, py) \
    { (sx), (sy), (sw), (sh), (cw), (ch), (px), (py) }
#define NO_TILE TILE(0, 0, 0, 0, 0, 0, 0, 0)

/* These offsets are the exact Sprite manifest used by
 * reloc_backend_assets.c. GO keeps its existing prefiltered 0.8 pixels,
 * re-tiled into tight legal DS cells (G 56x64, O 56x64, ! 24x64 = 17408
 * bytes). Indexed entries keep offset/color metadata for traffic/cloud
 * matching but own no OBJ bytes; every one of them renders through the
 * retained GX atlases. End letters are prefiltered once at the final 0.8
 * DS grid into exact RGB555 direct cells (group-8 sizes, no palette). */
static const NDSIFCommonAssetSpec sNdsIFCommonAssetSpecs[
    NDS_IFCOMMON_ASSET_COUNT] = {
    { 0x4d78u, 255, 255, 255, 0, 0, 0, 5,
      { TILE(0, 0, 32, 58, 32, 64, 0, 0),
        TILE(32, 0, 16, 29, 16, 32, 0, 0),
        TILE(32, 29, 16, 29, 16, 32, 0, 0),
        TILE(48, 0, 2, 29, 8, 32, 0, 0),
        TILE(48, 29, 2, 29, 8, 32, 0, 0),
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0xa730u, 255, 255, 255, 0, 0, 0, 5,
      { TILE(0, 0, 32, 59, 32, 64, 0, 0),
        TILE(32, 0, 16, 30, 16, 32, 0, 0),
        TILE(32, 30, 16, 29, 16, 32, 0, 0),
        TILE(48, 0, 8, 30, 8, 32, 0, 0),
        TILE(48, 30, 8, 29, 8, 32, 0, 0),
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0xc370u, 255, 255, 255, 0, 0, 0, 4,
      { TILE(0, 0, 16, 29, 16, 32, 0, 0),
        TILE(0, 29, 16, 29, 16, 32, 0, 0),
        TILE(16, 0, 3, 29, 8, 32, 0, 0),
        TILE(16, 29, 3, 29, 8, 32, 0, 0), NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x20990u, 255, 255, 255, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x21760u, 255, 255, 255, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x21878u, 0, 0, 0, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x21878u, 0x6a, 0x6a, 0x95, 0x12, 0x12, 0x2e, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x21950u, 0xfe, 0x0c, 0x0c, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x21a10u, 0xff, 0xa2, 0x00, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x21ba8u, 0x4b, 0x64, 0xff, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x22128u, 0xff, 0xff, 0xff, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x22588u, 0xff, 0xff, 0xff, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x22f18u, 0xff, 0xff, 0xff, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x23a28u, 0xff, 0x38, 0x38, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x24620u, 0xff, 0xa2, 0x00, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x25290u, 0x22, 0x66, 0xfe, 0, 0, 0, 0,
      { NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0xe4a8u, 255, 255, 255, 0, 0, 0, 2,
      { TILE(0, 0, 29, 32, 32, 32, 0, 0),
        TILE(0, 32, 29, 13, 32, 16, 0, 0), NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0xf740u, 255, 255, 255, 0, 0, 0, 2,
      { TILE(0, 0, 14, 32, 16, 32, 0, 0),
        TILE(0, 32, 14, 14, 16, 16, 0, 0), NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x127e0u, 255, 255, 255, 0, 0, 0, 4,
      { TILE(0, 0, 32, 32, 32, 32, 0, 0),
        TILE(32, 0, 8, 32, 8, 32, 0, 0),
        TILE(0, 32, 32, 13, 32, 16, 0, 0),
        TILE(32, 32, 8, 13, 8, 16, 0, 0), NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x144e0u, 255, 255, 255, 0, 0, 0, 2,
      { TILE(0, 0, 26, 32, 32, 32, 0, 0),
        TILE(0, 32, 26, 13, 32, 16, 0, 0), NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x16eb8u, 255, 255, 255, 0, 0, 0, 4,
      { TILE(0, 0, 32, 32, 32, 32, 0, 0),
        TILE(32, 0, 1, 32, 8, 32, 0, 0),
        TILE(0, 32, 32, 14, 32, 16, 0, 0),
        TILE(32, 32, 1, 14, 8, 16, 0, 0), NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x18fe8u, 255, 255, 255, 0, 0, 0, 2,
      { TILE(0, 0, 29, 32, 32, 32, 0, 0),
        TILE(0, 32, 29, 13, 32, 16, 0, 0), NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x1b5f8u, 255, 255, 255, 0, 0, 0, 2,
      { TILE(0, 0, 31, 32, 32, 32, 0, 0),
        TILE(0, 32, 31, 14, 32, 16, 0, 0), NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x1de68u, 255, 255, 255, 0, 0, 0, 4,
      { TILE(0, 0, 32, 32, 32, 32, 0, 0),
        TILE(32, 0, 2, 32, 8, 32, 0, 0),
        TILE(0, 32, 32, 13, 32, 16, 0, 0),
        TILE(32, 32, 2, 13, 8, 16, 0, 0), NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } },
    { 0x20788u, 255, 255, 255, 0, 0, 0, 4,
      { TILE(0, 0, 32, 32, 32, 32, 0, 0),
        TILE(32, 0, 1, 32, 8, 32, 0, 0),
        TILE(0, 32, 32, 14, 32, 16, 0, 0),
        TILE(32, 32, 1, 14, 8, 16, 0, 0), NO_TILE,
        NO_TILE, NO_TILE, NO_TILE, NO_TILE } }
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

/* Source RGBA32 extents (reloc_backend_assets.c) and final 0.8 DS grid
 * extents for the nine end letters. TIME UP = T/I/M/E/U/P,
 * GAME SET = G/A/M/E/S/T (E listed once per subset below). */
typedef struct NDSIFCommonEndLetter
{
    u32 asset_index;
    u32 offset;
    u8 source_width;
    u8 source_height;
    u8 ds_width;
    u8 ds_height;
} NDSIFCommonEndLetter;

static const NDSIFCommonEndLetter sNdsIFCommonEndLetters[9] = {
    { nNDSIFCommonAssetEndT, 0xe4a8u, 36, 56, 29, 45 },
    { nNDSIFCommonAssetEndI, 0xf740u, 17, 57, 14, 46 },
    { nNDSIFCommonAssetEndM, 0x127e0u, 50, 56, 40, 45 },
    { nNDSIFCommonAssetEndE, 0x144e0u, 32, 56, 26, 45 },
    { nNDSIFCommonAssetEndU, 0x16eb8u, 41, 58, 33, 46 },
    { nNDSIFCommonAssetEndP, 0x18fe8u, 36, 56, 29, 45 },
    { nNDSIFCommonAssetEndS, 0x1b5f8u, 39, 58, 31, 46 },
    { nNDSIFCommonAssetEndA, 0x1de68u, 43, 56, 34, 45 },
    { nNDSIFCommonAssetEndG, 0x20788u, 41, 57, 33, 46 }
};

/* Fixed end-bank slots, relative to NDS_IFCOMMON_END_BANK_BASE. Only one
 * message is active per round, so TIME UP and GAME SET slots overlap. */
typedef struct NDSIFCommonEndSlot
{
    u32 asset_index;
    u32 bank_offset;
} NDSIFCommonEndSlot;

static const NDSIFCommonEndSlot sNdsIFCommonTimeUpSlots[6] = {
    { nNDSIFCommonAssetEndT, 0u },
    { nNDSIFCommonAssetEndI, 3072u },
    { nNDSIFCommonAssetEndM, 4608u },
    { nNDSIFCommonAssetEndE, 8448u },
    { nNDSIFCommonAssetEndU, 11520u },
    { nNDSIFCommonAssetEndP, 15360u }
};

static const NDSIFCommonEndSlot sNdsIFCommonGameSetSlots[6] = {
    { nNDSIFCommonAssetEndG, 0u },
    { nNDSIFCommonAssetEndA, 3840u },
    { nNDSIFCommonAssetEndM, 7680u },
    { nNDSIFCommonAssetEndE, 11520u },
    { nNDSIFCommonAssetEndS, 14592u },
    { nNDSIFCommonAssetEndT, 17664u }
};

static NDSIFCommonNativeAsset sNdsIFCommonAssets[
    NDS_IFCOMMON_ASSET_COUNT];
static const void *sNdsIFCommonPreparedFile;
static size_t sNdsIFCommonPreparedFileSize;
static u32 sNdsIFCommonPrepared;
static u32 sNdsIFCommonAnnounceActive;
static u32 sNdsIFCommonAnnounceGameSet;
static s32 sNdsIFCommonNextOamID = 127;
static s32 sNdsIFCommonPreviousLowestOamID = 128;
static u32 sNdsIFCommonMatrixCount;
static u16 sNdsIFCommonMatrixInverse[32];
static u32 sNdsIFCommonFrameNeedsCommit;
static u32 sNdsIFCommonCloudTextureNames[NDS_IFCOMMON_CLOUD_ATLAS_COUNT];
static u32 sNdsIFCommonTrafficTextureName;
static NDSTask39HitSpark sNdsTask39HitSparks[
    NDS_TASK39_HIT_SPARK_CAPACITY];
static u16 *sNdsTask39HitSparkGfx;

#if defined(NDS_IF_GAMESTATUS_COMPACT) && NDS_IF_GAMESTATUS_COMPACT
/* P2-2p8 Phase 3 (A7): the file this state points at is the compact image
 * (src/port/reloc_backend_assets.c) -- the GO and end letters' pixels are not
 * in it, so the end messages come from these pre-baked streams instead. Each
 * stream is the whole end bank (NDS_IFCOMMON_END_BANK_BYTES) as the VRAM bake
 * would leave it, run-length coded in halfwords: a control word 0x8000 | n
 * copies the next n words, n (1..0x7fff) writes n zero words. */
static u32 sNdsIFCommonCompact;
static const u16 *sNdsIFCommonEndBaked[2];
static u32 sNdsIFCommonEndBakedWords[2];
volatile u32 gNdsIFCommonEndBakedBytes;
volatile u32 gNdsIFCommonEndBakedFailCount;
volatile u32 gNdsIFCommonEndBakedDecodeCount;
#endif

#define NDS_IFCOMMON_TAG_COUNT 6u
#define NDS_IFCOMMON_TAG_CELL 32u
#define NDS_IFCOMMON_TAG_CELL_BYTES 512u
#define NDS_IFCOMMON_TAG_PALETTE_BASE 11u
#define NDS_IFCOMMON_TAG_PRIM_COUNT 5u
#define NDS_IFCOMMON_ITEM_PALETTE 10u
#define NDS_IFCOMMON_ITEM_WIDTH 9u
#define NDS_IFCOMMON_ITEM_HEIGHT 7u
#define NDS_IFCOMMON_ITEM_CELL_WIDTH 16u
#define NDS_IFCOMMON_ITEM_CELL_HEIGHT 8u

typedef struct NDSIFCommonPlayerTag
{
    const Bitmap *bitmap;
    u32 width;
    u32 height;
    u16 *gfx;
    u32 baked;
} NDSIFCommonPlayerTag;

typedef struct NDSIFCommonItemArrow
{
    const Bitmap *bitmap;
    u32 width;
    u32 height;
    u16 *gfx;
    u32 runtime_red;
    u32 runtime_green;
    u32 runtime_blue;
    u32 baked;
} NDSIFCommonItemArrow;

static NDSIFCommonPlayerTag sNdsIFCommonPlayerTags[
    NDS_IFCOMMON_TAG_COUNT];
static u32 sNdsIFCommonPlayerTagCursor;
static u32 sNdsIFCommonPlayerTagCursorValid;
static NDSIFCommonItemArrow sNdsIFCommonItemArrow;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
static u32 sNdsTask39FxSpawnTickAccum;
static u32 sNdsTask39FxUpdateTickAccum;
#endif

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
volatile u32 gNdsIFCommonNativeOamTextureDiscardCount;
volatile u32 gNdsIFCommonNativeOamCloudReleaseCount;
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
volatile u32 gNdsTask39FxSpawnTicks;
volatile u32 gNdsTask39FxUpdateTicks;
volatile u32 gNdsTask39FxDrawTicks;
volatile u32 gNdsTask39FxFrameTicks;
volatile u32 gNdsTask39FxMaxFrameTicks;
volatile u32 gNdsTask39FxEngagementMask;
volatile u32 gNdsTask39FxHitSparkSpawnCount;
volatile u32 gNdsTask39FxHitSparkUpdateCount;
volatile u32 gNdsTask39FxHitSparkDrawCount;
volatile u32 gNdsTask39FxHitSparkDropCount;
volatile u32 gNdsTask39FxFlashDrawCount;
volatile u32 gNdsTask39FxArenaRejectCount;
volatile u32 gNdsTask39FxArenaBootSize;
volatile u32 gNdsTask39FxObjVramBytes;
volatile u32 gNdsTask39FxObjVramRemaining;

static void ndsTask39HitSparksDraw(void);
static s32 ndsIFCommonRoundFloatHalfUp(f32 value);
static f32 ndsIFCommonBattleScreenX(f32 projected_x);
static f32 ndsIFCommonBattleScreenY(f32 projected_y);
static void ndsIFCommonResetPlayerTags(void);
static void ndsIFCommonResetItemArrow(void);
static void ndsIFCommonResetCommonLetters(void);

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
    u32 weighted_alpha = 0u;
    u32 tap_index;

    weights[0] = (256u - fraction_x) * (256u - fraction_y);
    weights[1] = fraction_x * (256u - fraction_y);
    weights[2] = (256u - fraction_x) * fraction_y;
    weights[3] = fraction_x * fraction_y;
    for (tap_index = 0u; tap_index < 4u; tap_index++)
    {
        weighted_alpha +=
            (taps[tap_index] & 0xffu) * weights[tap_index];
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
            /* The four bilinear weights sum to exactly 65536. Therefore:
             *   weighted_alpha <= 255 * 65536 = 16,711,680
             *   weighted_premultiplied <= 255 * 255 * 65536
             *                            = 4,261,478,400
             * and adding weighted_alpha / 2 still stays below UINT32_MAX.
             * Keep this DS hot/setup loop in native 32-bit arithmetic instead
             * of paying libgcc's software 64-bit multiply/divide helpers. */
            u32 weighted_premultiplied = 0u;

            for (tap_index = 0u; tap_index < 4u; tap_index++)
            {
                u32 alpha = taps[tap_index] & 0xffu;
                u32 color = (taps[tap_index] >> shifts[channel]) & 0xffu;

                weighted_premultiplied +=
                    color * alpha * weights[tap_index];
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

/* The GO lettering's 0.8x footprint: each DS texel is the exact 1.25x1.25
 * source box under it, coverage-weighted, so both strokes of every letter
 * keep their weight. A point sample (the rule until 2026-10-04) dropped every
 * fifth source column and row, which broke the O's right stroke to one pixel
 * and read as "GC" (owner r75). Q8 source units; the box is 320 wide. */
static s32 ndsIFCommonSampleAreaTrafficPixel(
    const Sprite *sprite, const NDSIFCommonAssetSpec *spec,
    const void *file_data, size_t file_size,
    u32 destination_x, u32 destination_y, u8 rgba[4])
{
    u32 x0 = destination_x * 320u;
    u32 x1 = x0 + 320u;
    u32 y0 = destination_y * 320u;
    u32 y1 = y0 + 320u;
    u32 width = (u32)(u16)sprite->width;
    u32 height = (u32)(u16)sprite->height;
    u64 sum_red = 0u;
    u64 sum_green = 0u;
    u64 sum_blue = 0u;
    u64 sum_alpha = 0u;
    u32 source_y;

    for (source_y = y0 >> 8; (source_y < height) && ((source_y << 8) < y1);
         source_y++)
    {
        u32 top = ((source_y << 8) > y0) ? (source_y << 8) : y0;
        u32 bottom = (((source_y + 1u) << 8) < y1) ?
            ((source_y + 1u) << 8) : y1;
        u32 source_x;

        for (source_x = x0 >> 8;
             (source_x < width) && ((source_x << 8) < x1); source_x++)
        {
            u32 left = ((source_x << 8) > x0) ? (source_x << 8) : x0;
            u32 right = (((source_x + 1u) << 8) < x1) ?
                ((source_x + 1u) << 8) : x1;
            u64 weight = (u64)((right - left) * (bottom - top));
            u32 texel;
            u64 alpha;

            if (ndsIFCommonReadTrafficRgba(
                    sprite, spec, file_data, file_size,
                    source_x, source_y, &texel) == FALSE)
            {
                return FALSE;
            }
            alpha = (u64)(texel & 0xffu) * weight;
            sum_red += (u64)(texel >> 24) * alpha;
            sum_green += (u64)((texel >> 16) & 0xffu) * alpha;
            sum_blue += (u64)((texel >> 8) & 0xffu) * alpha;
            sum_alpha += alpha;
        }
    }
    if (sum_alpha == 0u)
    {
        rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0u;
        return TRUE;
    }
    rgba[0] = (u8)(sum_red / sum_alpha);
    rgba[1] = (u8)(sum_green / sum_alpha);
    rgba[2] = (u8)(sum_blue / sum_alpha);
    rgba[3] = (u8)((sum_alpha + (320u * 320u / 2u)) / (320u * 320u));
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

    if (spec == &sNdsIFCommonAssetSpecs[nNDSIFCommonAssetShadowGo])
    {
        return ndsIFCommonSampleAreaTrafficPixel(
            sprite, spec, file_data, file_size,
            destination_x, destination_y, rgba);
    }
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

/* Source-derived traffic palette (scripts/check_ifcommon_hybrid_oam.py
 * rebuilds and pins it), fixed so no one asset's sample can perturb the
 * others. Index 0 is transparent; the second zero is the visible black used
 * by the shadow. 2026-10-04: the dim lamps' own prim colours (0x043f red,
 * 0x029f orange, 0x7d89 blue) replace their old premultiplied dark shades. */
static const u16 sNdsIFCommonTrafficPalette[32] = {
    0x0000, 0x1084, 0x0000, 0x0842, 0x18c6, 0x7d89, 0x14a5, 0x0c63,
    0x043f, 0x1ce7, 0x2108, 0x0421, 0x2529, 0x029f, 0x3def, 0x5ef7,
    0x294a, 0x35ad, 0x2084, 0x2d6b, 0x318c, 0x4e73, 0x24a5, 0x39ce,
    0x28c6, 0x56b5, 0x1c63, 0x4a52, 0x4210, 0x6739, 0x4631, 0x77bd
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
                u32 alpha3;
                u32 palette_index;

                if (ndsIFCommonSamplePrefilteredTrafficPixel(
                        asset->sprite, asset_spec,
                        context->file_data, context->file_size,
                        x, y, rgba) == FALSE)
                {
                    gNdsIFCommonNativeOamPrepareCloudFailureStage = 2u;
                    return FALSE;
                }
                if (traffic->asset_index >= nNDSIFCommonAssetShadowGo)
                {
                    /* The dim lamps are TRANSLUCENT on the N64: an I4
                     * coverage disc of the lamp's prim colour at about 47%
                     * over the housing, whose sockets carry the bulbs'
                     * shading and highlight. Baked opaque (the rule until
                     * 2026-10-04) they hid that and read as flat discs (owner
                     * r75). They and the GO lettering keep their coverage as
                     * the texel's A3 alpha and their own colour. */
                    alpha3 = ((u32)rgba[3] * 7u + 127u) / 255u;
                    if (alpha3 == 0u)
                    {
                        continue;
                    }
                    if (traffic->asset_index == nNDSIFCommonAssetShadowGo)
                    {
                        red = rgba[0];
                        green = rgba[1];
                        blue = rgba[2];
                    }
                    else
                    {
                        red = asset_spec->red;
                        green = asset_spec->green;
                        blue = asset_spec->blue;
                    }
                }
                else
                {
                    if (rgba[3] < NDS_IFCOMMON_TRAFFIC_ALPHA_THRESHOLD)
                    {
                        continue;
                    }
                    /* The rod, housing and initial shadow are opaque source
                     * art. Bake the filtered coverage into RGB so their
                     * shading survives, then use A3 only as a hard cutout. */
                    alpha3 = 7u;
                    red = (u8)(((u32)rgba[0] * rgba[3] + 127u) / 255u);
                    green = (u8)(((u32)rgba[1] * rgba[3] + 127u) / 255u);
                    blue = (u8)(((u32)rgba[2] * rgba[3] + 127u) / 255u);
                }
                palette_index = ndsIFCommonPaletteIndex(
                    sNdsIFCommonTrafficPalette,
                    red, green, blue);
                gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[
                    NDS_IFCOMMON_TRAFFIC_PROOF_OFFSET + traffic_index]++;
                pixels[((u32)traffic->atlas_y + y) *
                           NDS_IFCOMMON_TRAFFIC_ATLAS_WIDTH +
                       (u32)traffic->atlas_x + x] =
                    (u8)((alpha3 << 5) | palette_index);
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

/* Indexed entries own no OBJ bytes: every one of them renders through the
 * retained traffic/cloud GX atlases. This binds sprite/bitmap/size metadata
 * for all 25 assets so traffic/cloud/GO/end matching keeps working, without
 * allocating or baking indexed OBJ cells. End letters additionally check
 * the exact source RGBA32 extents. */
static s32 ndsIFCommonBindGameStatusMetadata(const void *file_data,
                                             size_t file_size)
{
    u32 asset_index;
    u32 letter;

    memset(sNdsIFCommonAssets, 0, sizeof(sNdsIFCommonAssets));
    for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
         asset_index++)
    {
        const NDSIFCommonAssetSpec *spec =
            &sNdsIFCommonAssetSpecs[asset_index];
        NDSIFCommonNativeAsset *asset = &sNdsIFCommonAssets[asset_index];
        const Sprite *sprite = (const Sprite *)((const u8 *)file_data +
                                                spec->offset);

        if ((spec->offset + sizeof(*sprite) > file_size) ||
            (ndsIFCommonRangeValid(file_data, file_size, sprite->bitmap,
                                   (size_t)(u16)sprite->nbitmaps *
                                       sizeof(Bitmap)) == FALSE))
        {
            return FALSE;
        }
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
    }
    for (letter = 0u; letter < 9u; letter++)
    {
        const NDSIFCommonNativeAsset *asset =
            &sNdsIFCommonAssets[NDS_IFCOMMON_END_FIRST + letter];

        if ((asset->width != sNdsIFCommonEndLetters[letter].source_width) ||
            (asset->height != sNdsIFCommonEndLetters[letter].source_height) ||
            (asset->sprite->bmfmt != G_IM_FMT_RGBA) ||
            (asset->sprite->bmsiz != G_IM_SIZ_32b))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* Direct-only tile bake into a fixed VRAM base. GO keeps its existing
 * prefiltered pixels; end letters use the same 0.8 RGBA prefilter at the
 * final DS grid. No palette, no indexed path.
 *
 * `dest` is where the pixels go: OBJ VRAM itself, or (P2-2p8 A7) a RAM image
 * of an OBJ bank whose first halfword stands for VRAM offset `origin`, laid
 * out byte-for-byte as the VRAM bake would lay it out. `set_tiles` records
 * the tile layout the draw path reads; `fill` writes the pixels. The VRAM
 * bake is (SPRITE_GFX, 0, TRUE, TRUE) and stays exactly what it was. */
static s32 ndsIFCommonBakeDirectAssetTo(u32 asset_index,
                                        const void *file_data,
                                        size_t file_size, u32 vram_base,
                                        u16 *dest, u32 origin,
                                        u32 set_tiles, u32 fill)
{
    const NDSIFCommonAssetSpec *spec =
        &sNdsIFCommonAssetSpecs[asset_index];
    NDSIFCommonNativeAsset *asset = &sNdsIFCommonAssets[asset_index];
    const Sprite *sprite = asset->sprite;
    u32 to_vram = (dest == (u16 *)SPRITE_GFX) ? TRUE : FALSE;
    u32 tile_index;
    u32 cursor = vram_base;

    if ((sprite == NULL) || (spec->tile_count > NDS_IFCOMMON_MAX_TILES) ||
        (vram_base < origin))
    {
        return FALSE;
    }
    for (tile_index = 0u; tile_index < spec->tile_count; tile_index++)
    {
        const NDSIFCommonTileSpec *tile_spec = &spec->tiles[tile_index];
        NDSIFCommonNativeTile *tile = &asset->tiles[tile_index];
        u32 bytes = (u32)tile_spec->cell_width *
                    (u32)tile_spec->cell_height * sizeof(u16);
        u32 y;

        if (((tile_spec->source_x + tile_spec->content_width) >
             asset->width) ||
            ((tile_spec->source_y + tile_spec->content_height) >
             asset->height) ||
            ((tile_spec->pad_x + tile_spec->content_width) >
             tile_spec->cell_width) ||
            ((tile_spec->pad_y + tile_spec->content_height) >
             tile_spec->cell_height) ||
            ((cursor + bytes) > NDS_IFCOMMON_OBJ_VRAM_BYTES))
        {
            return FALSE;
        }
        if ((u32)ndsIFCommonSpriteSize(tile_spec->cell_width,
                                       tile_spec->cell_height) == 0u)
        {
            return FALSE;
        }
        if (set_tiles != FALSE)
        {
            tile->gfx = (u16 *)((u8 *)SPRITE_GFX + cursor);
            tile->size = ndsIFCommonSpriteSize(tile_spec->cell_width,
                                               tile_spec->cell_height);
            tile->color_format = SpriteColorFormat_Bmp;
            tile->spec = *tile_spec;
        }
        if (fill != FALSE)
        {
            u16 *pixels = (u16 *)((u8 *)dest + (cursor - origin));

            if (to_vram != FALSE)
            {
                dmaFillHalfWords(0u, pixels, bytes);
            }
            else
            {
                memset(pixels, 0, bytes);
            }
            for (y = 0u; y < tile_spec->content_height; y++)
            {
                u32 x;

                for (x = 0u; x < tile_spec->content_width; x++)
                {
                    u16 color = ndsIFCommonDecodePrefilteredGoPixel(
                            sprite, file_data, file_size,
                            (u32)tile_spec->source_x + x,
                            (u32)tile_spec->source_y + y);

                    pixels[(((u32)tile_spec->pad_y + y) *
                                tile_spec->cell_width) +
                           tile_spec->pad_x + x] = color;
                }
            }
        }
        cursor += bytes;
        gNdsIFCommonNativeOamPrepareTiles++;
    }
    gNdsIFCommonNativeOamPrepareAssets++;
    return TRUE;
}

static s32 ndsIFCommonBakeDirectAsset(u32 asset_index, const void *file_data,
                                      size_t file_size, u32 vram_base)
{
    return ndsIFCommonBakeDirectAssetTo(asset_index, file_data, file_size,
                                        vram_base, (u16 *)SPRITE_GFX, 0u,
                                        TRUE, TRUE);
}

static s32 ndsIFCommonEndAssetActive(u32 asset_index)
{
    const NDSIFCommonEndSlot *slots;
    u32 slot;

    if ((sNdsIFCommonPrepared == FALSE) ||
        (sNdsIFCommonAnnounceActive == FALSE) ||
        (asset_index < NDS_IFCOMMON_END_FIRST) ||
        (asset_index >= NDS_IFCOMMON_ASSET_COUNT))
    {
        return FALSE;
    }
    slots = (sNdsIFCommonAnnounceGameSet != FALSE) ?
        sNdsIFCommonGameSetSlots : sNdsIFCommonTimeUpSlots;
    for (slot = 0u; slot < 6u; slot++)
    {
        if (slots[slot].asset_index == asset_index)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void ndsTask39EffectsEngage(u32 mask)
{
    gNdsTask39FxEngagementMask |= mask;
}

void ndsTask39EffectsAddDrawTicks(u32 ticks)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsTask39FxDrawTicks += ticks;
    gNdsTask39FxFrameTicks = gNdsTask39FxSpawnTicks +
                             gNdsTask39FxUpdateTicks +
                             gNdsTask39FxDrawTicks;
    if (gNdsTask39FxFrameTicks > gNdsTask39FxMaxFrameTicks)
    {
        gNdsTask39FxMaxFrameTicks = gNdsTask39FxFrameTicks;
    }
#else
    (void)ticks;
#endif
}

static s32 ndsTask39EffectsArenaValid(void)
{
    if ((gNdsTask39FxArenaBootSize == 0u) &&
        (gNdsTaskmanArenaChosenSize != 0u))
    {
        gNdsTask39FxArenaBootSize = gNdsTaskmanArenaChosenSize;
    }
    if ((gNdsTask39FxArenaBootSize == 0u) ||
        (gNdsTaskmanArenaChosenSize != gNdsTask39FxArenaBootSize))
    {
        gNdsTask39FxArenaRejectCount = 1u;
        return FALSE;
    }
    return TRUE;
}

/* THIS WHOLE SPRITE HIT-SPARK PATH IS UNREACHABLE IN THE SHIPPING ROM, and the
 * clamp below is what proved it the expensive way.
 *
 * Its only external entry is the weak `efManagerDamageNormalLightMakeEffect` /
 * `...HeavyMakeEffect` in reloc_backend_compat_shims.c, and
 * `battleship_efmanager.c` provides STRONG definitions of both names under
 * NDS_R2_SOURCE_EFFECTS_PARTICLE, which defaults to 1 and is not overridden by
 * any target. The linker takes the strong ones. Disassembly of the published
 * ROM, 2026-08-03:
 *
 *   020941c8 <efManagerDamageNormalLightMakeEffect>:
 *     bl 209224c <ndsBaseEFManagerDamageNormalLightMakeEffect>
 *
 * -- straight to the source implementation, never here. `ndsTask39HitSparkSpawn`
 * is reached only from this file's own heavy-spark death, which nothing starts,
 * so gNdsTask39FxHitSparkSpawnCount is structurally 0.
 *
 * BUGS.md's *"orange ball ... looks too big"* row was answered here with the 2.2
 * ceiling below and the owner re-filed it unchanged, because the effect the
 * owner sees is drawn by the source particle path. The scale there is
 * source-exact and was re-derived twice: efmanager.c:2175 ramps light to 4.9x
 * at 40 damage and the heavy maker (efmanager.c:2197) sets no scale at all --
 * both exactly what the port does. What was wrong is the IMAGE: a 32x32 spark
 * box-averaged into a 16x16 atlas cell reads as a soft round blob, and
 * magnified 4.9x that is an orange ball. The cell is source resolution now.
 *
 * Left in place rather than deleted because NDS_TASK39_FX_SPRITES gates more
 * than this one path and a deletion is a separate, wider change. Do not tune
 * this constant again -- nothing reads it. */
#define NDS_TASK39_HIT_SPARK_SCALE_MAX 2.2F

volatile u32 gNdsTask39FxHitSparkScaleClampCount;

void ndsTask39HitSparkSpawn(const Vec3f *pos, s32 player, s32 size,
                            sb32 is_static, sb32 is_heavy)
{
#if NDS_TASK39_FX_SPRITES
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 start = cpuGetTiming();
#endif
    u32 index;
    NDSTask39HitSpark *spark = NULL;

    if ((pos == NULL) || (sNdsTask39HitSparkGfx == NULL) ||
        (ndsTask39EffectsArenaValid() == FALSE))
    {
        gNdsTask39FxHitSparkDropCount++;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        sNdsTask39FxSpawnTickAccum += cpuGetTiming() - start;
#endif
        return;
    }
    for (index = 0u; index < NDS_TASK39_HIT_SPARK_CAPACITY; index++)
    {
        if (sNdsTask39HitSparks[index].active == FALSE)
        {
            spark = &sNdsTask39HitSparks[index];
            break;
        }
    }
    if (spark == NULL)
    {
        gNdsTask39FxHitSparkDropCount++;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        sNdsTask39FxSpawnTickAccum += cpuGetTiming() - start;
#endif
        return;
    }

    *spark = (NDSTask39HitSpark){0};
    spark->pos = *pos;
    spark->player = (u8)((u32)player & 3u);
    spark->size = (s16)size;
    spark->is_heavy = (is_heavy != FALSE) ? TRUE : FALSE;
    spark->active = TRUE;
    if (spark->is_heavy != FALSE)
    {
        spark->scale = 1.0F;
    }
    else
    {
        f32 angle;
        f32 velocity = (is_static != FALSE) ? 0.0F :
            ((syUtilsRandFloat() * 38.0F) + 12.0F);

        angle = syUtilsRandFloat() * 6.28318530718F;
        spark->vel.x = __cosf(angle) * velocity;
        spark->vel.y = __sinf(angle) * velocity;
        spark->scale = (size < 10) ?
            (((10 - size) * -0.05F) + 1.0F) :
            (((size - 10) * 0.13F) + 1.0F);
        /* THE ORANGE BALL. `size` is damage and clamps at 40 upstream, so this
         * ramp reaches (40-10)*0.13 + 1.0 = 4.9x -- nearly five times the
         * sprite -- on a 256x192 screen. That is the owner's *"orange ball
         * visual effect that looks too big"* on side-A hits: a forward tilt is
         * normal-element and light, so it lands here, and HEAVY_ENV tints
         * player 1's spark RED (generate_task39_hit_sparks.py:30), which is the
         * orange.
         *
         * Note the heavy branch above is a flat 1.0, so before this the LIGHT
         * spark could draw five times the size of the HEAVY one -- the ramp was
         * never bounded, only the damage feeding it was.
         *
         * There is no source number to derive a ceiling from: the N64 draws
         * particles here, not sprites, so the sprite's size is a port choice
         * and the owner is its oracle. 2.2 keeps a big hit clearly bigger than
         * a small one while staying near the heavy spark's own scale. */
        if (spark->scale > NDS_TASK39_HIT_SPARK_SCALE_MAX)
        {
            spark->scale = NDS_TASK39_HIT_SPARK_SCALE_MAX;
            gNdsTask39FxHitSparkScaleClampCount++;
        }
    }
    gNdsTask39FxHitSparkSpawnCount++;
    ndsTask39EffectsEngage(NDS_TASK39_FX_ENGAGED_SPRITES);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsTask39FxSpawnTickAccum += cpuGetTiming() - start;
#endif
#else
    (void)pos;
    (void)player;
    (void)size;
    (void)is_static;
    (void)is_heavy;
#endif
}

void ndsTask39EffectsUpdate(void)
{
#if NDS_TASK39_FX_SPRITES
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 start = cpuGetTiming();
#endif
    u32 index;

    if (ndsTask39EffectsArenaValid() == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        sNdsTask39FxUpdateTickAccum += cpuGetTiming() - start;
#endif
        return;
    }
    for (index = 0u; index < NDS_TASK39_HIT_SPARK_CAPACITY; index++)
    {
        NDSTask39HitSpark *spark = &sNdsTask39HitSparks[index];
        u32 lifetime;

        if (spark->active == FALSE)
        {
            continue;
        }
        spark->pos.x += spark->vel.x;
        spark->pos.y += spark->vel.y;
        spark->age++;
        gNdsTask39FxHitSparkUpdateCount++;
        lifetime = (spark->is_heavy != FALSE) ?
            NDS_TASK39_HIT_SPARK_HEAVY_LIFETIME :
            NDS_TASK39_HIT_SPARK_LIGHT_LIFETIME;
        if (spark->age >= lifetime)
        {
            if (spark->is_heavy != FALSE)
            {
                Vec3f pos = spark->pos;
                s32 player = spark->player;
                s32 size = spark->size;

                spark->active = FALSE;
                ndsTask39EffectCensusRecord(
                    NDS_TASK39_EFFECT_EF_MANAGER_DAMAGE_NORMAL_LIGHT_MAKE_EFFECT,
                    NDS_TASK39_EFFECT_SUBSTITUTE);
                ndsTask39HitSparkSpawn(&pos, player, size, FALSE, FALSE);
            }
            else
            {
                spark->active = FALSE;
            }
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsTask39FxUpdateTickAccum += cpuGetTiming() - start;
#endif
#endif
}

/* Stream the hit-spark sheet from NitroFS straight into OBJ VRAM.
 *
 * It used to be a linked 22,528-byte array DMA'd in one shot. That array was
 * five whole steps of the boot-time taskman arena search (4,096 each), spent on
 * bytes that are read exactly once and then live in VRAM for the rest of the
 * run -- and it is what left NDS_R2_PARTICLE_RUNTIME=1 with no arena to boot
 * into. The generator was already writing the identical payload to
 * assets/effects/, so this reads that instead.
 *
 * One frame cell at a time, through a stack buffer, with explicit 32-bit
 * stores. Two DS-specific reasons, both of which have bitten this project:
 * DMA cannot source from DTCM, where libnds puts the stack, so dmaCopyWords
 * from a local is silently wrong; and VRAM rejects 8-bit writes, so a memcpy
 * whose tail degrades to byte stores would corrupt the sheet rather than fail.
 * A u32 loop is immune to both. 44 cells at load time costs nothing. */
void ndsFsLock(void);
void ndsFsUnlock(void);

static s32 ndsTask39PrepareHitSparksUnlocked(u32 *vram_cursor)
{
#if NDS_TASK39_FX_SPRITES
    u32 bytes = NDS_TASK39_HIT_SPARK_ASSET_BYTES;
    u32 cell[NDS_TASK39_HIT_SPARK_CELL_BYTES / sizeof(u32)];
    FILE *file;
    u32 offset;

    _Static_assert(NDS_TASK39_HIT_SPARK_ASSET_BYTES ==
                       NDS_TASK39_HIT_SPARK_CELL_BYTES *
                           NDS_TASK39_HIT_SPARK_FRAME_COUNT,
                   "Task 39 hit-spark payload size drifted");
    *vram_cursor = (*vram_cursor +
                    (NDS_IFCOMMON_OBJ_GFX_ALIGNMENT - 1u)) &
                   ~(NDS_IFCOMMON_OBJ_GFX_ALIGNMENT - 1u);
    if ((*vram_cursor + bytes) > NDS_IFCOMMON_OBJ_VRAM_BYTES)
    {
        return FALSE;
    }
    file = fopen(NDS_TASK39_HIT_SPARK_ASSET_PATH, "rb");
    if (file == NULL)
    {
        gNdsTask39FxObjVramBytes = 0u;
        return FALSE;
    }
    sNdsTask39HitSparkGfx =
        (u16 *)((u8 *)SPRITE_GFX + *vram_cursor);
    for (offset = 0u; offset < bytes;
         offset += NDS_TASK39_HIT_SPARK_CELL_BYTES)
    {
        u32 *dst = (u32 *)(void *)((u8 *)sNdsTask39HitSparkGfx + offset);
        u32 word;

        if (fread(cell, 1u, sizeof(cell), file) != sizeof(cell))
        {
            fclose(file);
            sNdsTask39HitSparkGfx = NULL;
            gNdsTask39FxObjVramBytes = 0u;
            return FALSE;
        }
        for (word = 0u; word < (u32)(sizeof(cell) / sizeof(cell[0])); word++)
        {
            dst[word] = cell[word];
        }
    }
    fclose(file);
    *vram_cursor += bytes;
    gNdsTask39FxObjVramBytes = bytes;
    gNdsTask39FxObjVramRemaining =
        NDS_IFCOMMON_OBJ_VRAM_BYTES - *vram_cursor;
#else
    (void)vram_cursor;
#endif
    return TRUE;
}

static s32 ndsTask39PrepareHitSparks(u32 *vram_cursor)
{
    s32 result;

    ndsFsLock();
    result = ndsTask39PrepareHitSparksUnlocked(vram_cursor);
    ndsFsUnlock();
    return result;
}

/* Drop the retained texture NAMES when the VRAM behind them is dropped.
 *
 * These names were cleared in exactly one place -- ndsIFCommonNativeOamInit,
 * which runs once at boot (nds_platform.c:328) -- while the allocations they
 * refer to are released on every scene change, by the
 * ndsRendererHardwareDiscardBattleStaticTextures call inside the scene-cache
 * eviction (reloc_backend_assets.c). A boot-scoped cache guarding a
 * scene-scoped resource.
 *
 * The consequence is silent and specific: ndsIFCommonNativeOamPrepareClouds
 * early-returns TRUE as soon as both cloud names and the traffic name are
 * non-zero, so after a scene reload it re-uploads NOTHING and the OAM path
 * keeps emitting handles into VRAM that has since been reused. Sprites drawn
 * from freed texture names is what the owner's rematch screenshot shows.
 *
 * This is the same shape as sNdsRendererBattleStaticTexturePrepared, which is
 * already cleared by ndsRendererHardwareDiscardTextureCache -- that latch
 * survived the bug because someone wired its invalidation; this one was
 * missed. */
s32 ndsIFCommonNativeOamIsPrepared(void)
{
    return (sNdsIFCommonPrepared != FALSE) ? TRUE : FALSE;
}

void ndsIFCommonNativeOamDiscardTextures(void)
{
    memset(sNdsIFCommonCloudTextureNames, 0,
           sizeof(sNdsIFCommonCloudTextureNames));
    sNdsIFCommonTrafficTextureName = 0u;

    /* The prepare latch, for the same reason and one level up. Its guard reads
     * `sNdsIFCommonPrepared && sNdsIFCommonPreparedFile == file_data`, which
     * LOOKS self-invalidating -- it compares the asset pointer, so a different
     * file re-prepares -- and that is exactly why it survived review. Pointer
     * identity is not identity across a rewound allocator: the reloc heap is
     * rewound between scenes (docs/BUGS.md records AdapterCount 2 as the proof),
     * so the second entry loads the same asset into the same slot, gets the same
     * address, matches the guard and returns TRUE without re-preparing -- after
     * ndsRendererHardwareDiscardBattleStaticTextures has already released the
     * cloud and traffic atlases this latch claims are resident.
     *
     * Clearing it here makes the cache scene-scoped, which is the scope it
     * always needed; within a scene the pointer compare is still valid because
     * addresses are stable there. */
    sNdsIFCommonPrepared = FALSE;
    sNdsIFCommonPreparedFile = NULL;
    sNdsIFCommonPreparedFileSize = 0u;
    /* Scene reset returns GO ownership and drops the end phase with it; the
     * next scene re-bakes GO and the source-selected ending from scratch. */
    sNdsIFCommonAnnounceActive = FALSE;
    sNdsIFCommonAnnounceGameSet = NDS_IFCOMMON_ANNOUNCE_TIME_UP;

    /* And the Task 39 hit-spark sheet, which is not a texture name at all but a
     * raw VRAM address -- (u8 *)SPRITE_GFX + vram_cursor, handed out by the same
     * OBJ VRAM packing the prepare above redoes from scratch. Both draw sites
     * guard on `!= NULL` only, so a stale pointer is indistinguishable from a
     * live one and the sparks keep sampling whatever now occupies that offset. */
    memset(sNdsTask39HitSparks, 0, sizeof(sNdsTask39HitSparks));
    sNdsTask39HitSparkGfx = NULL;
    ndsIFCommonResetPlayerTags();
    ndsIFCommonResetItemArrow();

    gNdsIFCommonNativeOamTextureDiscardCount++;
}

/* R2-07 E2. The cloud and traffic atlases ONLY -- not the prepare latch, and
 * not the hit-spark OBJ VRAM. ndsIFCommonNativeOamDiscardTextures above is the
 * scene-teardown form and clears everything; this is the re-entry form, and the
 * difference matters because ndsIFCommonNativeOamPrepareClouds needs
 * sNdsIFCommonPrepared and sNdsIFCommonPreparedFile to still hold in order to
 * rebuild the atlases from the asset it already has.
 *
 * Why a second entry needs it: texture VRAM is allocated by libnds inside
 * glTexImage2D, and these three names are the only textures that survive
 * ndsRendererHardwareDiscardTextureCache -- PrepareClouds early-returns while
 * all three are non-zero. So entry one allocates statics first and atlases
 * after, while entry two frees and re-uploads the 24 statics AROUND atlas
 * blocks already sitting in the middle of the pool. Same bytes, different
 * layout, and the dynamic stage textures that fit on entry one no longer fit:
 * measured as TEXREJECT bit 12 (TEXIMAGE) on the second entry against a mask of
 * 0 on the first, which fails PrepareRun for run 42 and rejects the whole
 * native stage owner.
 *
 * Releasing them here restores entry one's allocation ORDER, which is what the
 * allocator actually depends on. */
void ndsIFCommonNativeOamReleaseCloudTextures(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    ndsIFCommonReleaseCloudAtlases();
    ndsIFCommonReleaseTrafficAtlas();
    gNdsIFCommonNativeOamCloudReleaseCount++;
#endif
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
    sNdsIFCommonPreparedFile = NULL;
    sNdsIFCommonPreparedFileSize = 0u;
    sNdsIFCommonAnnounceActive = FALSE;
    sNdsIFCommonAnnounceGameSet = NDS_IFCOMMON_ANNOUNCE_TIME_UP;
    memset(sNdsTask39HitSparks, 0, sizeof(sNdsTask39HitSparks));
    sNdsTask39HitSparkGfx = NULL;
    ndsIFCommonResetPlayerTags();
    ndsIFCommonResetCommonLetters();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsTask39FxSpawnTickAccum = 0u;
    sNdsTask39FxUpdateTickAccum = 0u;
#endif
    gNdsTask39FxSpawnTicks = 0u;
    gNdsTask39FxUpdateTicks = 0u;
    gNdsTask39FxDrawTicks = 0u;
    gNdsTask39FxFrameTicks = 0u;
    gNdsTask39FxMaxFrameTicks = 0u;
    gNdsTask39FxEngagementMask = 0u;
    gNdsTask39FxHitSparkSpawnCount = 0u;
    gNdsTask39FxHitSparkUpdateCount = 0u;
    gNdsTask39FxHitSparkDrawCount = 0u;
    gNdsTask39FxHitSparkDropCount = 0u;
    gNdsTask39FxFlashDrawCount = 0u;
    gNdsTask39FxArenaRejectCount = 0u;
    gNdsTask39FxArenaBootSize = gNdsTaskmanArenaChosenSize;
    gNdsTask39FxObjVramBytes = 0u;
    gNdsTask39FxObjVramRemaining = NDS_IFCOMMON_OBJ_VRAM_BYTES;
#if NDS_RENDERER_HW_TRIANGLES
    /* The battle's first OBJ write: a held frame's OBJs (the 1P intro's,
     * nds_source2d.c) still read bank E and the OAM, so the hold ends here
     * (ndsPlatformTransitionThaw), before the init and the atlas bakes. */
    ndsPlatformTransitionThaw();
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
    oamClear(&oamMain, 0, 128);
    oamUpdate(&oamMain);
#endif
}

/* Fixed GO slot bases inside the GO bank. G 7168 + O 7168 + ! 3072. */
static const u32 sNdsIFCommonGoSlotBases[3] = { 0u, 7168u, 14336u };

s32 ndsIFCommonNativeOamPrepareGameStatus(void *file_data,
                                           size_t file_size)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 start;
    u32 vram_cursor;
    u32 go_index;

    _Static_assert(NDS_IFCOMMON_GO_BANK_BYTES == 17408u,
                   "GO bank size drifted");
    _Static_assert(NDS_IFCOMMON_END_BANK_BYTES == 20736u,
                   "end bank size drifted");
    _Static_assert(NDS_IFCOMMON_USED_BYTES <= NDS_IFCOMMON_OBJ_VRAM_BYTES,
                   "fixed banks exceed OBJ VRAM");

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
    /* This bake runs while the battle's files load -- before the OAM init
     * below it, which used to be the hold's only end on this path. The held
     * frame (the 1P intro's banners, the SSS) still reads bank E, so baking
     * the GO, spark and end tiles into it showed them through the held
     * sprites (owner 2026-10-01: intro banners full of blue blocks). End the
     * hold first; the cover stands until the battle presents. */
    ndsPlatformTransitionThaw();

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
    ndsIFCommonResetPlayerTags();
    ndsIFCommonResetItemArrow();
    ndsIFCommonResetCommonLetters();
    sNdsIFCommonPrepared = FALSE;
    sNdsIFCommonPreparedFile = NULL;
    sNdsIFCommonPreparedFileSize = 0u;
    sNdsIFCommonAnnounceActive = FALSE;
    sNdsIFCommonAnnounceGameSet = NDS_IFCOMMON_ANNOUNCE_TIME_UP;
#if defined(NDS_IF_GAMESTATUS_COMPACT) && NDS_IF_GAMESTATUS_COMPACT
    /* A new load is the full file again; the previous scene's compact image
     * and baked streams died with its heap. */
    sNdsIFCommonCompact = FALSE;
    sNdsIFCommonEndBaked[0] = NULL;
    sNdsIFCommonEndBaked[1] = NULL;
    sNdsIFCommonEndBakedWords[0] = 0u;
    sNdsIFCommonEndBakedWords[1] = 0u;
#endif

    if (ndsIFCommonBindGameStatusMetadata(file_data, file_size) == FALSE)
    {
        gNdsIFCommonNativeOamPrepareFailCount++;
        gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackBadAsset;
        return FALSE;
    }
    for (go_index = 0u; go_index < 3u; go_index++)
    {
        if (ndsIFCommonBakeDirectAsset(go_index, file_data, file_size,
                                       NDS_IFCOMMON_GO_BANK_BASE +
                                           sNdsIFCommonGoSlotBases[
                                               go_index]) == FALSE)
        {
            gNdsIFCommonNativeOamPrepareFailCount++;
            gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            return FALSE;
        }
    }
    vram_cursor = NDS_IFCOMMON_SPARK_BANK_BASE;
    if (ndsTask39PrepareHitSparks(&vram_cursor) == FALSE)
    {
        gNdsIFCommonNativeOamPrepareFailCount++;
        gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackBadAsset;
        return FALSE;
    }

    sNdsIFCommonPrepared = TRUE;
    sNdsIFCommonPreparedFile = file_data;
    sNdsIFCommonPreparedFileSize = file_size;
    gNdsIFCommonNativeOamPrepareBytes = NDS_IFCOMMON_USED_BYTES;
    gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
    gNdsIFCommonNativeOamPrepareSuccessCount++;
    return TRUE;
#else
    (void)file_data;
    (void)file_size;
    return FALSE;
#endif
}

/* Bake the source-selected ending into the end bank. The source SObj
 * creation seam calls this before adding the first end-message glyph, so
 * GO can still be live: GO bytes are never touched here. TIME UP (0) and
 * GAME SET (1) are source-exclusive per round and share the bank. Repeat
 * calls with the active phase upload nothing. A partial failure leaves the
 * phase inactive so a retry re-bakes cleanly without falsely reading ready. */
s32 ndsIFCommonNativeOamPrepareAnnouncement(u32 game_set)
{
#if NDS_RENDERER_HW_TRIANGLES
    const NDSIFCommonEndSlot *slots;
    u32 start;
    u32 slot;

    if (game_set > NDS_IFCOMMON_ANNOUNCE_GAME_SET)
    {
        gNdsIFCommonNativeOamPrepareFailCount++;
        return FALSE;
    }
    if ((sNdsIFCommonPrepared == FALSE) ||
        (sNdsIFCommonPreparedFile == NULL) ||
#if defined(NDS_IF_GAMESTATUS_COMPACT) && NDS_IF_GAMESTATUS_COMPACT
        ((sNdsIFCommonCompact == FALSE) &&
         (sNdsIFCommonPreparedFileSize < NDS_IFCOMMON_GAME_STATUS_SIZE)))
#else
        (sNdsIFCommonPreparedFileSize < NDS_IFCOMMON_GAME_STATUS_SIZE))
#endif
    {
        gNdsIFCommonNativeOamPrepareFailCount++;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackNotPrepared;
        return FALSE;
    }
    if ((sNdsIFCommonAnnounceActive != FALSE) &&
        (sNdsIFCommonAnnounceGameSet == game_set))
    {
        return TRUE;
    }
    start = cpuGetTiming();
    /* TIME UP / GAME SET take the end bank the common letters live in. */
    ndsIFCommonResetCommonLetters();
    slots = (game_set != FALSE) ? sNdsIFCommonGameSetSlots :
                                  sNdsIFCommonTimeUpSlots;
#if defined(NDS_IF_GAMESTATUS_COMPACT) && NDS_IF_GAMESTATUS_COMPACT
    if (sNdsIFCommonCompact != FALSE)
    {
        /* The letters' pixels are not resident: lay the tiles out exactly as
         * the bake below would, then expand the pre-baked bank image. */
        const u16 *stream = sNdsIFCommonEndBaked[game_set];
        u32 words = sNdsIFCommonEndBakedWords[game_set];
        u16 *out = (u16 *)((u8 *)SPRITE_GFX + NDS_IFCOMMON_END_BANK_BASE);
        u32 out_words = NDS_IFCOMMON_END_BANK_BYTES / sizeof(u16);
        u32 at = 0u;
        u32 i = 0u;

        for (slot = 0u; slot < 6u; slot++)
        {
            if ((stream == NULL) ||
                (ndsIFCommonBakeDirectAssetTo(
                     slots[slot].asset_index, sNdsIFCommonPreparedFile,
                     sNdsIFCommonPreparedFileSize,
                     NDS_IFCOMMON_END_BANK_BASE + slots[slot].bank_offset,
                     (u16 *)SPRITE_GFX, 0u, TRUE, FALSE) == FALSE))
            {
                stream = NULL;
                break;
            }
        }
        while ((stream != NULL) && (i < words) && (at < out_words))
        {
            u32 control = stream[i++];
            u32 count = control & 0x7fffu;

            if ((count == 0u) || ((at + count) > out_words))
            {
                stream = NULL;
                break;
            }
            if ((control & 0x8000u) != 0u)
            {
                if ((i + count) > words)
                {
                    stream = NULL;
                    break;
                }
                while (count-- != 0u)
                {
                    out[at++] = stream[i++];
                }
            }
            else
            {
                while (count-- != 0u)
                {
                    out[at++] = 0u;
                }
            }
        }
        if ((stream == NULL) || (at != out_words) || (i != words))
        {
            for (slot = 0u; slot < 6u; slot++)
            {
                NDSIFCommonNativeAsset *dead =
                    &sNdsIFCommonAssets[slots[slot].asset_index];
                u32 dead_tile;

                for (dead_tile = 0u; dead_tile < NDS_IFCOMMON_MAX_TILES;
                     dead_tile++)
                {
                    dead->tiles[dead_tile].gfx = NULL;
                }
            }
            sNdsIFCommonAnnounceActive = FALSE;
            gNdsIFCommonEndBakedFailCount++;
            gNdsIFCommonNativeOamPrepareFailCount++;
            gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            return FALSE;
        }
        gNdsIFCommonEndBakedDecodeCount++;
        sNdsIFCommonAnnounceActive = TRUE;
        sNdsIFCommonAnnounceGameSet = game_set;
        gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
        gNdsIFCommonNativeOamPrepareSuccessCount++;
        return TRUE;
    }
#endif
    for (slot = 0u; slot < 6u; slot++)
    {
        u32 asset_index = slots[slot].asset_index;
        NDSIFCommonNativeAsset *asset = &sNdsIFCommonAssets[asset_index];
        u32 tile;

        for (tile = 0u; tile < NDS_IFCOMMON_MAX_TILES; tile++)
        {
            asset->tiles[tile].gfx = NULL;
        }
        if (ndsIFCommonBakeDirectAsset(
                asset_index, sNdsIFCommonPreparedFile,
                sNdsIFCommonPreparedFileSize,
                NDS_IFCOMMON_END_BANK_BASE + slots[slot].bank_offset) ==
            FALSE)
        {
            u32 clear;

            for (clear = 0u; clear <= slot; clear++)
            {
                NDSIFCommonNativeAsset *dead =
                    &sNdsIFCommonAssets[slots[clear].asset_index];
                u32 dead_tile;

                for (dead_tile = 0u; dead_tile < NDS_IFCOMMON_MAX_TILES;
                     dead_tile++)
                {
                    dead->tiles[dead_tile].gfx = NULL;
                }
            }
            sNdsIFCommonAnnounceActive = FALSE;
            gNdsIFCommonNativeOamPrepareFailCount++;
            gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            return FALSE;
        }
    }
    sNdsIFCommonAnnounceActive = TRUE;
    sNdsIFCommonAnnounceGameSet = game_set;
    gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
    gNdsIFCommonNativeOamPrepareSuccessCount++;
    return TRUE;
#else
    (void)game_set;
    return FALSE;
#endif
}

s32 ndsIFCommonNativeOamPrepareClouds(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 start;

    if ((sNdsIFCommonPrepared == FALSE) ||
        (sNdsIFCommonPreparedFile == NULL) ||
#if defined(NDS_IF_GAMESTATUS_COMPACT) && NDS_IF_GAMESTATUS_COMPACT
        ((sNdsIFCommonCompact == FALSE) &&
         (sNdsIFCommonPreparedFileSize < NDS_IFCOMMON_GAME_STATUS_SIZE)))
#else
        (sNdsIFCommonPreparedFileSize < NDS_IFCOMMON_GAME_STATUS_SIZE))
#endif
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

#if defined(NDS_IF_GAMESTATUS_COMPACT) && NDS_IF_GAMESTATUS_COMPACT
extern void *syTaskmanMalloc(size_t size, u32 align);

static u32 ndsIFCommonPayloadBytes(const Sprite *sprite, const Bitmap *bitmap)
{
    u32 texels = (u32)(u16)bitmap->width_img * (u32)(u16)bitmap->actualHeight;

    switch (sprite->bmsiz)
    {
    case G_IM_SIZ_4b: return (texels + 1u) / 2u;
    case G_IM_SIZ_8b: return texels;
    case G_IM_SIZ_16b: return texels * 2u;
    case G_IM_SIZ_32b: return texels * 4u;
    default: return 0u;
    }
}

/* Ranges (offset from the prepared file, bytes) the compact image may drop:
 * every bitmap payload of an asset with OBJ tiles -- the GO and end letters,
 * whose pixels are baked here and never read again. Returns 0 (keep the
 * whole file) if any range is out of the file, exceeds `max`, or shares a
 * byte with a payload whose asset keeps its pixels (lamps, rod, frame: the
 * cloud and traffic atlases read them after this point). */
u32 ndsIFCommonNativeOamLetterPayloads(u32 *offsets, u32 *bytes, u32 max)
{
    const u8 *base = (const u8 *)sNdsIFCommonPreparedFile;
    size_t size = sNdsIFCommonPreparedFileSize;
    u32 count = 0u;
    u32 pass;

    if ((sNdsIFCommonPrepared == FALSE) || (base == NULL) ||
        (sNdsIFCommonCompact != FALSE))
    {
        return 0u;
    }
    for (pass = 0u; pass < 2u; pass++)
    {
        u32 asset_index;

        for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
             asset_index++)
        {
            const Sprite *sprite = sNdsIFCommonAssets[asset_index].sprite;
            u32 letter =
                (sNdsIFCommonAssetSpecs[asset_index].tile_count != 0u) ?
                    TRUE : FALSE;
            u32 b;

            if (sprite == NULL)
            {
                return 0u;
            }
            if ((pass == 0u) != (letter != FALSE))
            {
                continue;
            }
            for (b = 0u; b < (u32)(u16)sprite->nbitmaps; b++)
            {
                const Bitmap *bitmap = &sprite->bitmap[b];
                u32 payload = ndsIFCommonPayloadBytes(sprite, bitmap);
                uintptr_t at = (uintptr_t)bitmap->buf - (uintptr_t)base;
                u32 i;

                if ((bitmap->buf == NULL) || (payload == 0u))
                {
                    continue;
                }
                if ((at >= size) || (payload > (size - at)))
                {
                    return 0u;
                }
                if (pass == 0u)
                {
                    for (i = 0u; i < count; i++)
                    {
                        if ((offsets[i] == (u32)at) && (bytes[i] == payload))
                        {
                            break;
                        }
                    }
                    if (i < count)
                    {
                        continue;
                    }
                    if (count >= max)
                    {
                        return 0u;
                    }
                    offsets[count] = (u32)at;
                    bytes[count] = payload;
                    count++;
                    continue;
                }
                for (i = 0u; i < count; i++)
                {
                    if (((u32)at < (offsets[i] + bytes[i])) &&
                        (offsets[i] < ((u32)at + payload)))
                    {
                        return 0u;
                    }
                }
            }
        }
    }
    return count;
}

static u32 ndsIFCommonRleEncode(const u16 *in, u32 n, u16 *out, u32 cap)
{
    u32 i = 0u;
    u32 o = 0u;

    while (i < n)
    {
        if (in[i] == 0u)
        {
            u32 run = 0u;

            while (((i + run) < n) && (in[i + run] == 0u) && (run < 0x7fffu))
            {
                run++;
            }
            if (o >= cap)
            {
                return 0u;
            }
            out[o++] = (u16)run;
            i += run;
        }
        else
        {
            u32 start = i;
            u32 literal = 0u;

            /* A lone zero between texels stays literal: a zero run costs a
             * control word either way. */
            while ((i < n) && (literal < 0x7fffu))
            {
                if ((in[i] == 0u) && (((i + 1u) >= n) || (in[i + 1u] == 0u)))
                {
                    break;
                }
                i++;
                literal++;
            }
            if ((o + 1u + literal) > cap)
            {
                return 0u;
            }
            out[o++] = (u16)(0x8000u | literal);
            memcpy(&out[o], &in[start], literal * sizeof(u16));
            o += literal;
        }
    }
    return o;
}

/* Bake TIME UP (0) and GAME SET (1) into RAM images of the end bank, exactly
 * as ndsIFCommonNativeOamPrepareAnnouncement would bake them into OBJ VRAM,
 * and keep them run-length coded on the scene heap. `scratch` holds one bank
 * image; `code` holds the worst-case stream. Must run while the full file is
 * still the prepared file. */
s32 ndsIFCommonNativeOamBakeEndVariants(u16 *scratch, u16 *code,
                                        u32 code_capacity_words)
{
    u32 game_set;

    if ((sNdsIFCommonPrepared == FALSE) || (sNdsIFCommonCompact != FALSE) ||
        (scratch == NULL) || (code == NULL))
    {
        return FALSE;
    }
    gNdsIFCommonEndBakedBytes = 0u;
    for (game_set = 0u; game_set < 2u; game_set++)
    {
        const NDSIFCommonEndSlot *slots =
            (game_set != 0u) ? sNdsIFCommonGameSetSlots :
                               sNdsIFCommonTimeUpSlots;
        u32 slot;
        u32 words;
        u16 *resident;

        memset(scratch, 0, NDS_IFCOMMON_END_BANK_BYTES);
        for (slot = 0u; slot < 6u; slot++)
        {
            if (ndsIFCommonBakeDirectAssetTo(
                    slots[slot].asset_index, sNdsIFCommonPreparedFile,
                    sNdsIFCommonPreparedFileSize,
                    NDS_IFCOMMON_END_BANK_BASE + slots[slot].bank_offset,
                    scratch, NDS_IFCOMMON_END_BANK_BASE, FALSE, TRUE) ==
                FALSE)
            {
                gNdsIFCommonEndBakedFailCount++;
                return FALSE;
            }
        }
        words = ndsIFCommonRleEncode(
            scratch, NDS_IFCOMMON_END_BANK_BYTES / sizeof(u16), code,
            code_capacity_words);
        resident = (words != 0u) ?
            (u16 *)syTaskmanMalloc(words * sizeof(u16), 4u) : NULL;
        if (resident == NULL)
        {
            gNdsIFCommonEndBakedFailCount++;
            return FALSE;
        }
        memcpy(resident, code, words * sizeof(u16));
        sNdsIFCommonEndBaked[game_set] = resident;
        sNdsIFCommonEndBakedWords[game_set] = words;
        gNdsIFCommonEndBakedBytes += words * sizeof(u16);
    }
    return TRUE;
}

/* The loader copied the file into its compact image: move every retained
 * pointer into it. `map` turns a source offset (and a byte count that must
 * stay contiguous) into a compact offset. All-or-nothing. */
s32 ndsIFCommonNativeOamIsPreparedFile(const void *file_data)
{
    return ((sNdsIFCommonPrepared != FALSE) &&
            (sNdsIFCommonPreparedFile == file_data)) ? TRUE : FALSE;
}

s32 ndsIFCommonNativeOamRebaseGameStatus(
    const void *old_base, size_t old_size, void *new_base, size_t new_size,
    s32 (*map)(u32 source_offset, u32 bytes, u32 *out_offset), u32 compact)
{
    const Sprite *sprites[NDS_IFCOMMON_ASSET_COUNT];
    u32 asset_index;

    if ((sNdsIFCommonPrepared == FALSE) ||
        (sNdsIFCommonPreparedFile != old_base) || (map == NULL) ||
        (new_base == NULL))
    {
        return FALSE;
    }
    for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
         asset_index++)
    {
        const Sprite *sprite = sNdsIFCommonAssets[asset_index].sprite;
        uintptr_t at = (uintptr_t)sprite - (uintptr_t)old_base;
        u32 mapped;

        if ((sprite == NULL) || (at >= old_size) ||
            (map((u32)at, sizeof(Sprite), &mapped) == FALSE) ||
            ((mapped + sizeof(Sprite)) > new_size))
        {
            return FALSE;
        }
        sprites[asset_index] =
            (const Sprite *)((const u8 *)new_base + mapped);
    }
    for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
         asset_index++)
    {
        NDSIFCommonNativeAsset *asset = &sNdsIFCommonAssets[asset_index];

        asset->sprite = sprites[asset_index];
        /* The loader already re-seated the Sprite's own bitmap pointer. */
        asset->bitmap = sprites[asset_index]->bitmap;
    }
    sNdsIFCommonPreparedFile = new_base;
    sNdsIFCommonPreparedFileSize = new_size;
    sNdsIFCommonCompact = (compact != FALSE) ? TRUE : FALSE;
    return TRUE;
}

_Static_assert(NDS_IFCOMMON_END_BANK_IMAGE_BYTES ==
                   NDS_IFCOMMON_END_BANK_BYTES,
               "the loader's end-bank scratch must match the bank");
#endif

void ndsIFCommonNativeOamBeginFrame(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsTask39FxSpawnTicks = sNdsTask39FxSpawnTickAccum;
    gNdsTask39FxUpdateTicks = sNdsTask39FxUpdateTickAccum;
    gNdsTask39FxDrawTicks = 0u;
    gNdsTask39FxFrameTicks = gNdsTask39FxSpawnTicks +
                             gNdsTask39FxUpdateTicks;
    sNdsTask39FxSpawnTickAccum = 0u;
    sNdsTask39FxUpdateTickAccum = 0u;
#endif
#if NDS_SHIP_TELEMETRY
    gNdsIFCommonNativeOamFrameBeginTicks = 0u;
    gNdsIFCommonNativeOamFrameCommitTicks = 0u;
#endif
    gNdsIFCommonNativeOamFrameCommitCalls = 0u;
    gNdsIFCommonNativeOamFrameClearedObjects = 0u;
    gNdsIFCommonNativeOamFrameIdle = 0u;
    sNdsIFCommonFrameNeedsCommit = FALSE;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsIFCommonPreviousLowestOamID < 128)
    {
        u32 start = NDS_IFCOMMON_TELEMETRY_TICK();

        gNdsIFCommonNativeOamFrameClearedObjects =
            (u32)(128 - sNdsIFCommonPreviousLowestOamID);
        oamClear(&oamMain, sNdsIFCommonPreviousLowestOamID,
                 (int)gNdsIFCommonNativeOamFrameClearedObjects);
        NDS_IFCOMMON_TELEMETRY_ADD(
            gNdsIFCommonNativeOamFrameBeginTicks, start);
        sNdsIFCommonFrameNeedsCommit = TRUE;
    }
#endif
    sNdsIFCommonNextOamID = 127;
    sNdsIFCommonMatrixCount = 0u;
#if NDS_SHIP_TELEMETRY
    gNdsIFCommonNativeOamFrameTicks = 0u;
#endif
    gNdsIFCommonNativeOamFrameRecognizedCalls = 0u;
    gNdsIFCommonNativeOamFrameDrawCalls = 0u;
    gNdsIFCommonNativeOamFrameFallbackCalls = 0u;
#if NDS_SHIP_TELEMETRY
    gNdsIFCommonNativeOamFrameSObjCount = 0u;
    gNdsIFCommonNativeOamFrameSemanticHash = NDS_IFCOMMON_HASH_SEED;
#endif
    gNdsIFCommonNativeOamFrameObjectCount = 0u;
    gNdsIFCommonNativeOamFrameCloudDrawCount = 0u;
    gNdsIFCommonNativeOamLastFallbackReason =
        nNDSIFCommonFallbackNone;
    ndsTask39HitSparksDraw();
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

static void ndsTask39HitSparksDraw(void)
{
#if NDS_TASK39_FX_SPRITES
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 start = cpuGetTiming();
#endif
    u32 index;

    if ((sNdsTask39HitSparkGfx == NULL) || (gGMCameraGObj == NULL) ||
        (CObjGetStruct(gGMCameraGObj) == NULL) ||
        (ndsTask39EffectsArenaValid() == FALSE))
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        ndsTask39EffectsAddDrawTicks(cpuGetTiming() - start);
#endif
        return;
    }
    for (index = 0u; index < NDS_TASK39_HIT_SPARK_CAPACITY; index++)
    {
        NDSTask39HitSpark *spark = &sNdsTask39HitSparks[index];
        Vec3f pos;
        f32 projected_x;
        f32 projected_y;
        f32 scale;
        u32 scale_q16;
        u32 frame;
        u16 inverse;
        s32 matrix_index;
        s32 center_x;
        s32 center_y;
        s32 size_double;

        if (spark->active == FALSE)
        {
            continue;
        }
        if (sNdsIFCommonNextOamID < 0)
        {
            gNdsTask39FxHitSparkDropCount++;
            break;
        }
        pos = spark->pos;
        func_ovl2_800EB924(CObjGetStruct(gGMCameraGObj),
                           gGMCameraMatrix, &pos,
                           &projected_x, &projected_y);
        center_x = ndsIFCommonRoundFloatHalfUp(
            ndsIFCommonBattleScreenX(projected_x));
        center_y = ndsIFCommonRoundFloatHalfUp(
            ndsIFCommonBattleScreenY(projected_y));
        if ((center_x < -16) || (center_x > 272) ||
            (center_y < -16) || (center_y > 208))
        {
            continue;
        }

        scale = spark->scale * NDS_TASK39_HIT_SPARK_SCREEN_SCALE;
        scale_q16 = (u32)((scale * 65536.0F) + 0.5F);
        if (scale_q16 == 0u)
        {
            continue;
        }
        inverse = (u16)(((1u << 24) + (scale_q16 / 2u)) / scale_q16);
        matrix_index = ndsIFCommonMatrixForScale(inverse);
        if (matrix_index < 0)
        {
            gNdsTask39FxHitSparkDropCount++;
            continue;
        }
        if (spark->is_heavy != FALSE)
        {
            frame = NDS_TASK39_HIT_SPARK_HEAVY_OFFSET +
                ((u32)spark->player * NDS_TASK39_HIT_SPARK_HEAVY_FRAMES) +
                (((u32)spark->age * NDS_TASK39_HIT_SPARK_HEAVY_FRAMES) /
                 NDS_TASK39_HIT_SPARK_HEAVY_LIFETIME);
        }
        else
        {
            frame = ((u32)spark->player *
                     NDS_TASK39_HIT_SPARK_LIGHT_FRAMES) +
                (((u32)spark->age * NDS_TASK39_HIT_SPARK_LIGHT_FRAMES) /
                 NDS_TASK39_HIT_SPARK_LIGHT_LIFETIME);
        }
        size_double = (scale_q16 > (1u << 16)) ? TRUE : FALSE;
        oamSet(&oamMain, sNdsIFCommonNextOamID,
               center_x - (size_double ? 16 : 8),
               center_y - (size_double ? 16 : 8),
               0, 15, SpriteSize_16x16, SpriteColorFormat_Bmp,
               sNdsTask39HitSparkGfx +
                   (frame * (NDS_TASK39_HIT_SPARK_CELL_BYTES / 2u)),
               matrix_index, size_double, false, false, false, false);
        sNdsIFCommonNextOamID--;
        sNdsIFCommonFrameNeedsCommit = TRUE;
        gNdsIFCommonNativeOamFrameObjectCount++;
        gNdsTask39FxHitSparkDrawCount++;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    ndsTask39EffectsAddDrawTicks(cpuGetTiming() - start);
#endif
#endif
}

#if NDS_SHIP_TELEMETRY
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
#endif

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

/* A battle point the source projects (func_ovl2_800EB924: x, y about the
 * viewport centre, in source pixels) on the DS screen. The battle's 3D fills
 * the screen, so the camera's 300 x 220 viewport maps onto 256 x 192: x
 * scales by 128/150 and y by 96/110, not the 2D layer's uniform 0.8, which is
 * right only for screen-fixed HUD. Anchoring a fighter's tag, an item arrow
 * or a hit spark at 0.8 pulled it toward the centre by up to ~8 px at the
 * screen's edges (owner 10-01: tags "offset around edges of screen"). */
static f32 ndsIFCommonBattleScreenX(f32 projected_x)
{
    f32 half = (f32)gGMCameraStruct.viewport_width * 0.5F;

    return (half > 0.0F) ? (128.0F + (projected_x * (128.0F / half))) :
                           (128.0F + (projected_x * 0.8F));
}

static f32 ndsIFCommonBattleScreenY(f32 projected_y)
{
    f32 half = (f32)gGMCameraStruct.viewport_height * 0.5F;

    return (half > 0.0F) ? (96.0F - (projected_y * (96.0F / half))) :
                           (96.0F - (projected_y * 0.8F));
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

    if (asset_index >= NDS_IFCOMMON_END_FIRST)
    {
        if (ndsIFCommonEndAssetActive(asset_index) == FALSE)
        {
            return FALSE;
        }
    }
    for (tile_index = 0u; tile_index < asset->tile_count; tile_index++)
    {
        if ((asset->tiles[tile_index].color_format !=
             SpriteColorFormat_Bmp) && (sobj->sprite.alpha != 255u))
        {
            return FALSE;
        }
        if ((asset_index >= NDS_IFCOMMON_END_FIRST) &&
            (asset->tiles[tile_index].gfx == NULL))
        {
            return FALSE;
        }
    }
    /* GO and end bitmaps are already final-DS-resolution: source scalex
     * applies 1:1 locally while the source position still scales by 0.8. */
    prefiltered = ((asset_index <= nNDSIFCommonAssetGoExclaim) ||
                   (asset_index >= NDS_IFCOMMON_END_FIRST)) ?
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

/* Player tags (1P/2P/3P/4P/CP and the 1P allies' heart, source
 * ifCommonPlayerTagProcDisplay).
 *
 * The six glyphs are IA8 bitmaps 15-21 wide by 24 high living in the
 * IFCommonPlayerTags reloc file, not the GameStatus file this unit prepares,
 * so they cannot ride the hybrid OBJ bake. Each distinct live bitmap is
 * decoded once into its own 4bpp 32x32 OBJ cell; the draw arm then replays
 * the source placement exactly and tints through a 16-entry intensity
 * palette built from that player's source prim colour over black env.
 *
 * Palettes 11-15 hold the five prim ramps. No emitted direct-colour tile
 * reads the main OBJ palettes: GO and the end letters are direct-colour
 * Bmp, and every indexed asset routes to the cloud/traffic texture atlases
 * instead of OAM. Tags live in the fixed tag bank after the spark sheet.
 * Lower HUD (timer/stock/damage) lives on oamSub and is untouched here. */
static void ndsIFCommonResetPlayerTags(void)
{
    memset(sNdsIFCommonPlayerTags, 0, sizeof(sNdsIFCommonPlayerTags));
    sNdsIFCommonPlayerTagCursor = 0u;
    sNdsIFCommonPlayerTagCursorValid = FALSE;
}

static const Bitmap *sNdsIFCommonTaskBitmap;

static void ndsIFCommonResetItemArrow(void)
{
    memset(&sNdsIFCommonItemArrow, 0, sizeof(sNdsIFCommonItemArrow));
    sNdsIFCommonTaskBitmap = NULL;
}

/* IFCommonItem's pickup arrow is source I4 + SP_TRANSPARENT. The retained
 * generic reference treats ordinary I4 as binary coverage: nibble zero is
 * transparent and every nonzero nibble emits the Sprite prim colour. Keep
 * that exact contract in one 16x8 4bpp OBJ cell; the source itself is 9x7. */
static s32 ndsIFCommonReadItemArrowI4(const Sprite *sprite, u32 source_x,
                                     u32 source_y, u8 *intensity)
{
    const Bitmap *bitmap;
    const u8 *pixels;
    u32 width_img;
    u32 height;
    u32 shuffled_x;
    u32 row_bytes;
    u8 packed;

    if ((sprite == NULL) || (intensity == NULL) ||
        (sprite->bitmap == NULL) || (sprite->nbitmaps != 1))
    {
        return FALSE;
    }
    bitmap = sprite->bitmap;
    width_img = (u32)(u16)bitmap->width_img;
    height = (u32)(u16)bitmap->actualHeight;
    if (width_img == 0u)
    {
        width_img = (u32)(u16)bitmap->width;
    }
    if (height == 0u)
    {
        height = (u32)(u16)sprite->height;
    }
    if ((source_x >= (u32)(u16)sprite->width) || (source_y >= height))
    {
        return FALSE;
    }
    shuffled_x = source_x ^ ((source_y & 1u) != 0u ? 8u : 0u);
    if (shuffled_x >= width_img)
    {
        return FALSE;
    }
    row_bytes = (width_img + 1u) / 2u;
    pixels = (const u8 *)bitmap->buf;
    if (pixels == NULL)
    {
        return FALSE;
    }
    packed = pixels[(((size_t)source_y * row_bytes) +
                     (shuffled_x >> 1)) ^ 3u];
    *intensity = ((shuffled_x & 1u) == 0u) ?
        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
    return TRUE;
}

s32 ndsIFCommonNativeOamBakeItemArrow(const Sprite *sprite)
{
    u32 cell[NDS_IFCOMMON_ITEM_BANK_BYTES / sizeof(u32)];
    u8 *cell_bytes = (u8 *)cell;
    u16 *gfx;
    u32 *dst;
    u16 *palette;
    u32 word;
    u32 y;

    if ((sprite == NULL) || (sprite->bitmap == NULL) ||
        (sprite->bmfmt != G_IM_FMT_I) ||
        (sprite->bmsiz != G_IM_SIZ_4b) || (sprite->nbitmaps != 1) ||
        ((u32)(u16)sprite->width != NDS_IFCOMMON_ITEM_WIDTH) ||
        ((u32)(u16)sprite->height != NDS_IFCOMMON_ITEM_HEIGHT))
    {
        return FALSE;
    }
    if ((sNdsIFCommonItemArrow.baked != FALSE) &&
        (sNdsIFCommonItemArrow.bitmap == sprite->bitmap) &&
        (sNdsIFCommonItemArrow.runtime_red == (u32)sprite->red) &&
        (sNdsIFCommonItemArrow.runtime_green == (u32)sprite->green) &&
        (sNdsIFCommonItemArrow.runtime_blue == (u32)sprite->blue))
    {
        return TRUE;
    }
    if (sNdsIFCommonPrepared == FALSE)
    {
        return FALSE;
    }

    memset(cell, 0, sizeof(cell));
    for (y = 0u; y < NDS_IFCOMMON_ITEM_HEIGHT; y++)
    {
        u32 x;

        for (x = 0u; x < NDS_IFCOMMON_ITEM_WIDTH; x++)
        {
            u8 intensity;
            u32 tile;
            u32 offset;

            if (ndsIFCommonReadItemArrowI4(sprite, x, y, &intensity) == FALSE)
            {
                return FALSE;
            }
            if (intensity == 0u)
            {
                continue;
            }
            tile = (x >> 3);
            offset = (tile * 32u) + ((y & 7u) * 4u) +
                     ((x & 7u) >> 1);
            if ((x & 1u) == 0u)
            {
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0xf0u) | 1u);
            }
            else
            {
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0x0fu) | 0x10u);
            }
        }
    }

    gfx = (u16 *)((u8 *)SPRITE_GFX + NDS_IFCOMMON_ITEM_BANK_BASE);
    dst = (u32 *)(void *)gfx;
    for (word = 0u; word < (u32)(sizeof(cell) / sizeof(cell[0])); word++)
    {
        dst[word] = cell[word];
    }
    palette = &SPRITE_PALETTE[NDS_IFCOMMON_ITEM_PALETTE * 16u];
    palette[0] = 0u;
    for (word = 1u; word < 16u; word++)
    {
        palette[word] = ndsIFCommonPackRgb15(
            sprite->red, sprite->green, sprite->blue);
    }
    sNdsIFCommonItemArrow.bitmap = sprite->bitmap;
    sNdsIFCommonItemArrow.width = NDS_IFCOMMON_ITEM_WIDTH;
    sNdsIFCommonItemArrow.height = NDS_IFCOMMON_ITEM_HEIGHT;
    sNdsIFCommonItemArrow.gfx = gfx;
    sNdsIFCommonItemArrow.runtime_red = sprite->red;
    sNdsIFCommonItemArrow.runtime_green = sprite->green;
    sNdsIFCommonItemArrow.runtime_blue = sprite->blue;
    sNdsIFCommonItemArrow.baked = TRUE;
    return TRUE;
}

/* ndsIFCommonReadI8 without a file range to check against: the bitmap comes
 * from a live SObj whose shape the loader already validated, so only the
 * pointer and the TEXSHUF walk remain. */
static s32 ndsIFCommonReadTagI8(const Sprite *sprite, u32 source_x,
                                u32 source_y, u8 *intensity)
{
    const Bitmap *bitmap;
    u32 out_y = 0u;
    u32 bitmap_index;

    if ((sprite == NULL) || (intensity == NULL) ||
        (sprite->bitmap == NULL))
    {
        return FALSE;
    }
    bitmap = sprite->bitmap;
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
        if (pixels == NULL)
        {
            return FALSE;
        }
        *intensity = pixels[((size_t)local_y * width_img + shuffled_x) ^
                            3u];
        return TRUE;
    }
    return FALSE;
}

s32 ndsIFCommonNativeOamBakePlayerTag(const Sprite *sprite)
{
    u32 slot;
    u32 width;
    u32 height;
    u32 cell[NDS_IFCOMMON_TAG_CELL_BYTES / sizeof(u32)];
    u8 *cell_bytes = (u8 *)cell;
    u16 *gfx;
    u32 *dst;
    u32 word;
    u32 y;

    if ((sprite == NULL) || (sprite->bitmap == NULL) ||
        (sprite->bmfmt != G_IM_FMT_IA) ||
        (sprite->bmsiz != G_IM_SIZ_8b) || (sprite->nbitmaps < 1))
    {
        return FALSE;
    }
    width = (u32)(u16)sprite->width;
    height = (u32)(u16)sprite->height;
    if ((width == 0u) || (width > NDS_IFCOMMON_TAG_CELL) ||
        (height == 0u) || (height > NDS_IFCOMMON_TAG_CELL))
    {
        return FALSE;
    }
    for (slot = 0u; slot < NDS_IFCOMMON_TAG_COUNT; slot++)
    {
        if ((sNdsIFCommonPlayerTags[slot].baked != FALSE) &&
            (sNdsIFCommonPlayerTags[slot].bitmap == sprite->bitmap) &&
            (sNdsIFCommonPlayerTags[slot].width == width) &&
            (sNdsIFCommonPlayerTags[slot].height == height))
        {
            return TRUE;
        }
    }
    if (sNdsIFCommonPrepared == FALSE)
    {
        return FALSE;
    }
    for (slot = 0u; slot < NDS_IFCOMMON_TAG_COUNT; slot++)
    {
        if (sNdsIFCommonPlayerTags[slot].baked == FALSE)
        {
            break;
        }
    }
    if (slot >= NDS_IFCOMMON_TAG_COUNT)
    {
        return FALSE;
    }
    if (sNdsIFCommonPlayerTagCursorValid == FALSE)
    {
        sNdsIFCommonPlayerTagCursor = NDS_IFCOMMON_TAG_BANK_BASE;
        sNdsIFCommonPlayerTagCursorValid = TRUE;
    }
    if ((sNdsIFCommonPlayerTagCursor + NDS_IFCOMMON_TAG_CELL_BYTES) >
        NDS_IFCOMMON_OBJ_VRAM_BYTES)
    {
        return FALSE;
    }
    memset(cell, 0, sizeof(cell));
    for (y = 0u; y < height; y++)
    {
        u32 x;

        for (x = 0u; x < width; x++)
        {
            u8 ia;
            u8 index;
            u32 tile = ((y >> 3) * (NDS_IFCOMMON_TAG_CELL >> 3)) +
                       (x >> 3);
            u32 offset = (tile * 32u) + ((y & 7u) * 4u) +
                         ((x & 7u) >> 1);

            if (ndsIFCommonReadTagI8(sprite, x, y, &ia) == FALSE)
            {
                return FALSE;
            }
            /* Source IA combine keeps texels with alpha nibble >= 8 and
             * lerps prim over env by the intensity nibble; index 0 stays
             * transparent. Intensity 0 with kept alpha is opaque black,
             * which shares ramp entry 1 (black) with intensity 1. */
            if ((ia & 0x0fu) < 8u)
            {
                continue;
            }
            index = (u8)(ia >> 4);
            if (index == 0u)
            {
                index = 1u;
            }
            if ((x & 1u) == 0u)
            {
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0xf0u) |
                                          (index & 0x0fu));
            }
            else
            {
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0x0fu) |
                                          (u8)((index & 0x0fu) << 4));
            }
        }
    }
    /* Stack staging to VRAM with explicit 32-bit stores: DMA cannot source
     * from DTCM stack and VRAM rejects 8-bit writes, same as the hit-spark
     * sheet loader above. */
    gfx = (u16 *)((u8 *)SPRITE_GFX + sNdsIFCommonPlayerTagCursor);
    dst = (u32 *)(void *)gfx;
    for (word = 0u; word < (u32)(sizeof(cell) / sizeof(cell[0])); word++)
    {
        dst[word] = cell[word];
    }
    sNdsIFCommonPlayerTags[slot].bitmap = sprite->bitmap;
    sNdsIFCommonPlayerTags[slot].width = width;
    sNdsIFCommonPlayerTags[slot].height = height;
    sNdsIFCommonPlayerTags[slot].gfx = gfx;
    sNdsIFCommonPlayerTags[slot].baked = TRUE;
    sNdsIFCommonPlayerTagCursor += NDS_IFCOMMON_TAG_CELL_BYTES;
    return TRUE;
}

/* x / 15 for x < 2^16 (checked exhaustively): the palette ramps below divide
 * by 15 per channel per entry, and Thumb code has no UMULL, so GCC called
 * __udivsi3 for each (~55 calls a frame between this file and the HUD). */
static inline u32 ndsIFCommonDiv15(u32 x)
{
    return (x * 34953u) >> 19;
}

static void ndsIFCommonPlayerTagPalette(u32 color_id)
{
    u32 red = (u32)dIFCommonPlayerTagPrimColorsR[color_id];
    u32 green = (u32)dIFCommonPlayerTagPrimColorsG[color_id];
    u32 blue = (u32)dIFCommonPlayerTagPrimColorsB[color_id];
    u16 *palette = &SPRITE_PALETTE[
        (NDS_IFCOMMON_TAG_PALETTE_BASE + color_id) * 16u];
    u32 i;

    palette[0] = 0u;
    for (i = 1u; i < 16u; i++)
    {
        palette[i] = (u16)((1u << 15) |
                           ((ndsIFCommonDiv15(red * i + 7u) >> 3) & 31u) |
                           (((ndsIFCommonDiv15(green * i + 7u) >> 3) & 31u)
                            << 5) |
                           (((ndsIFCommonDiv15(blue * i + 7u) >> 3) & 31u)
                            << 10));
    }
    /* Intensity 0 with kept alpha maps to entry 1 in the bake; the source
     * lerps prim over an all-zero env there, so entry 1 is black, not
     * prim/15 (review, 2026-09-06). Intensity-1 texels share it, one 5-bit
     * step below their true value. */
    palette[1] = (u16)(1u << 15);
}

static s32 ndsIFCommonPlayerTagMiss(u32 reason)
{
    gNdsIFCommonNativeOamFrameFallbackCalls++;
    gNdsIFCommonNativeOamLastFallbackReason = reason;
    return FALSE;
}

static s32 ndsIFCommonPlayerTagRecognized(void)
{
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    return TRUE;
}

/* Source ifCommonPlayerTagProcDisplay:1821-1850 replayed for OAM: TopN joint
 * plus camera_zoom_base, the source projection helper, in-bounds clipping,
 * the wait==1 / eye.z>6000 gate, the hide and bossend flags, SObj centering
 * by sprite width/height. TRUE means handled (drawn, or the source draws
 * nothing this frame); FALSE records a native-render failure. */
static s32 ndsIFCommonEmitPlayerTag(struct GObj *gobj)
{
    SObj *sobj = SObjGetStruct(gobj);
    s32 player;
    SCPlayerData *battle_player;
    FTStruct *fp;
    DObj *topn;
    CObj *cobj;
    Vec3f pos;
    f32 projected_x;
    f32 projected_y;
    s32 screen_x;
    s32 screen_y;
    u32 tag;
    u32 color_id;
    u32 slot;

    if ((sobj == NULL) || (sobj->next != NULL) ||
        (sobj->sprite.bitmap == NULL))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
    {
        return ndsIFCommonPlayerTagRecognized();
    }
    /* SP_TRANSPARENT is the source tag mode (SP_TEXSHUF | SP_TRANSPARENT);
     * anything else, a non-opaque alpha, or a scaled sprite keeps the exact
     * generic path, which owns XLU blending and scaling. */
    if (((sobj->sprite.attr & SP_TRANSPARENT) == 0u) ||
        (sobj->sprite.alpha != 255u) ||
        (sobj->sprite.scalex != 1.0F) ||
        (sobj->sprite.scaley != 1.0F))
    {
        return ndsIFCommonPlayerTagMiss(
            nNDSIFCommonFallbackRuntimeColor);
    }
    if ((sobj->sprite.bmfmt != G_IM_FMT_IA) ||
        (sobj->sprite.bmsiz != G_IM_SIZ_8b) || (sobj->sprite.nbitmaps < 1) ||
        ((u32)(u16)sobj->sprite.width == 0u) ||
        ((u32)(u16)sobj->sprite.width > NDS_IFCOMMON_TAG_CELL) ||
        ((u32)(u16)sobj->sprite.height == 0u) ||
        ((u32)(u16)sobj->sprite.height > NDS_IFCOMMON_TAG_CELL))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if (gNdsIFCommonNativeOamEnabled == 0u)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackDisabled);
    }
    player = ifGetPlayer(gobj);
    if ((player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if (gSCManagerBattleState == NULL)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackNotPrepared);
    }
    battle_player = &gSCManagerBattleState->players[player];
    if ((battle_player->pkind == nFTPlayerKindNot) ||
        (battle_player->fighter_gobj == NULL))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    tag = (u32)battle_player->tag;
    if (tag > (u32)nIFPlayerTagKindHeart)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackUnknownSprite);
    }
    color_id = (u32)battle_player->color;
    if (color_id >= NDS_IFCOMMON_TAG_PRIM_COUNT)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if (((u32)sobj->sprite.red !=
         (u32)dIFCommonPlayerTagPrimColorsR[color_id]) ||
        ((u32)sobj->sprite.green !=
         (u32)dIFCommonPlayerTagPrimColorsG[color_id]) ||
        ((u32)sobj->sprite.blue !=
         (u32)dIFCommonPlayerTagPrimColorsB[color_id]) ||
        (sobj->envcolor.r != 0u) || (sobj->envcolor.g != 0u) ||
        (sobj->envcolor.b != 0u))
    {
        return ndsIFCommonPlayerTagMiss(
            nNDSIFCommonFallbackRuntimeColor);
    }
    fp = ftGetStruct(battle_player->fighter_gobj);
    if (fp == NULL)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if ((fp->is_playertag_bossend != FALSE) ||
        (fp->is_playertag_hide != FALSE))
    {
        return ndsIFCommonPlayerTagRecognized();
    }
    if (gGMCameraGObj == NULL)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackNotPrepared);
    }
    cobj = CObjGetStruct(gGMCameraGObj);
    if (cobj == NULL)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackNotPrepared);
    }
    if ((fp->playertag_wait != 1) && (cobj->vec.eye.z <= 6000.0F))
    {
        return ndsIFCommonPlayerTagRecognized();
    }
    if ((fp->attr == NULL) ||
        (fp->joints[nFTPartsJointTopN] == NULL))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    topn = fp->joints[nFTPartsJointTopN];
    pos = topn->translate.vec.f;
    pos.y += fp->attr->camera_zoom_base;
    func_ovl2_800EB924(cobj, gGMCameraMatrix, &pos,
                       &projected_x, &projected_y);
    if (gmCameraCheckTargetInBounds(projected_x, projected_y) == FALSE)
    {
        return ndsIFCommonPlayerTagRecognized();
    }
    /* The source's SObj position: the tag's bottom centre on the projected
     * point. The OAM tag is the sprite's own size, so it is centred on the
     * point's DS position rather than scaled with it. */
    screen_x = ndsIFCommonRoundFloatHalfUp(
        ndsIFCommonBattleScreenX(projected_x) -
        ((f32)sobj->sprite.width * 0.5F));
    screen_y = ndsIFCommonRoundFloatHalfUp(
        ndsIFCommonBattleScreenY(projected_y) - (f32)sobj->sprite.height);
    if ((screen_x <= -32) || (screen_x >= 256) ||
        (screen_y <= -32) || (screen_y >= 192))
    {
        return ndsIFCommonPlayerTagRecognized();
    }
    if (sNdsIFCommonPrepared == FALSE)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackNotPrepared);
    }
    if ((sobj->sprite.bitmap == NULL) ||
        (sobj->sprite.bmfmt != G_IM_FMT_IA) ||
        (sobj->sprite.bmsiz != G_IM_SIZ_8b) || (sobj->sprite.nbitmaps < 1) ||
        ((u32)(u16)sobj->sprite.width > NDS_IFCOMMON_TAG_CELL) ||
        ((u32)(u16)sobj->sprite.height > NDS_IFCOMMON_TAG_CELL))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if (ndsIFCommonNativeOamBakePlayerTag(&sobj->sprite) == FALSE)
    {
        /* Format was checked above, so a bake miss is slot or VRAM
         * exhaustion. */
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackObjectLimit);
    }
    for (slot = 0u; slot < NDS_IFCOMMON_TAG_COUNT; slot++)
    {
        if ((sNdsIFCommonPlayerTags[slot].baked != FALSE) &&
            (sNdsIFCommonPlayerTags[slot].bitmap == sobj->sprite.bitmap))
        {
            break;
        }
    }
    if (slot >= NDS_IFCOMMON_TAG_COUNT)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if (sNdsIFCommonNextOamID < 0)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackObjectLimit);
    }
    ndsIFCommonPlayerTagPalette(color_id);
    oamSet(&oamMain, sNdsIFCommonNextOamID, screen_x, screen_y, 0,
           (int)(NDS_IFCOMMON_TAG_PALETTE_BASE + color_id),
           SpriteSize_32x32, SpriteColorFormat_16Color,
           sNdsIFCommonPlayerTags[slot].gfx, -1, false, false, false,
           false, false);
    sNdsIFCommonNextOamID--;
    sNdsIFCommonFrameNeedsCommit = TRUE;
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    gNdsIFCommonNativeOamFrameDrawCalls++;
    gNdsIFCommonNativeOamFrameObjectCount++;
    return TRUE;
}

static s32 ndsIFCommonItemArrowMiss(u32 reason)
{
    gNdsIFCommonNativeOamFrameFallbackCalls++;
    gNdsIFCommonNativeOamLastFallbackReason = reason;
    return FALSE;
}

static s32 ndsIFCommonItemArrowRecognized(void)
{
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    return TRUE;
}

/* ifCommonItemArrowProcDisplay already owns the source visibility test,
 * projection, and SObj position. Reproduce only lbCommonDrawSObjAttr here:
 * one red I4 source sprite at the battle screen's 0.8 presentation scale. */
static s32 ndsIFCommonEmitItemArrow(struct GObj *gobj)
{
    SObj *sobj = SObjGetStruct(gobj);
    u32 scale_q16 = NDS_IFCOMMON_SCREEN_SCALE_Q16;
    u16 inverse;
    s32 matrix_index;
    s32 origin_x;
    s32 origin_y;
    s32 center_x;
    s32 center_y;
    s32 x;
    s32 y;

    if ((sobj == NULL) || (sobj->next != NULL) ||
        (sobj->sprite.bitmap == NULL))
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackBadAsset);
    }
    if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
    {
        return ndsIFCommonItemArrowRecognized();
    }
    if (((sobj->sprite.attr & SP_TRANSPARENT) == 0u) ||
        (sobj->sprite.scalex != 1.0F) || (sobj->sprite.scaley != 1.0F) ||
        (sobj->sprite.bmfmt != G_IM_FMT_I) ||
        (sobj->sprite.bmsiz != G_IM_SIZ_4b) ||
        (sobj->sprite.nbitmaps != 1) ||
        ((u32)(u16)sobj->sprite.width != NDS_IFCOMMON_ITEM_WIDTH) ||
        ((u32)(u16)sobj->sprite.height != NDS_IFCOMMON_ITEM_HEIGHT))
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackBadAsset);
    }
    if (sobj->sprite.alpha == 0u)
    {
        return ndsIFCommonItemArrowRecognized();
    }
    if (gNdsIFCommonNativeOamEnabled == 0u)
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackDisabled);
    }
    if (ndsIFCommonNativeOamBakeItemArrow(&sobj->sprite) == FALSE)
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackNotPrepared);
    }
    if (sNdsIFCommonNextOamID < 0)
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackObjectLimit);
    }

    inverse = (u16)(((1u << 24) + (scale_q16 / 2u)) / scale_q16);
    matrix_index = ndsIFCommonMatrixForScale(inverse);
    if (matrix_index < 0)
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackMatrixLimit);
    }
    /* The source put the arrow's bottom centre on the item's projected point
     * (pos = centre + projected - (width / 2, height)); that point takes the
     * battle 3D's mapping (ndsIFCommonBattleScreenX/Y) and the 0.8-scaled
     * arrow hangs from it. */
    origin_x = ndsIFCommonRoundFloatHalfUp(
        ndsIFCommonBattleScreenX(
            (sobj->pos.x + ((f32)NDS_IFCOMMON_ITEM_WIDTH * 0.5F)) -
            (f32)gGMCameraStruct.viewport_center_x) -
        ((f32)NDS_IFCOMMON_ITEM_WIDTH * 0.4F));
    origin_y = ndsIFCommonRoundFloatHalfUp(
        ndsIFCommonBattleScreenY(
            (f32)gGMCameraStruct.viewport_center_y -
            (sobj->pos.y + (f32)NDS_IFCOMMON_ITEM_HEIGHT)) -
        ((f32)NDS_IFCOMMON_ITEM_HEIGHT * 0.8F));
    center_x = origin_x + ndsIFCommonRoundQ16HalfUp(
        (s32)(NDS_IFCOMMON_ITEM_CELL_WIDTH / 2u) * (s32)scale_q16);
    center_y = origin_y + ndsIFCommonRoundQ16HalfUp(
        (s32)(NDS_IFCOMMON_ITEM_CELL_HEIGHT / 2u) * (s32)scale_q16);
    x = center_x - (s32)(NDS_IFCOMMON_ITEM_CELL_WIDTH / 2u);
    y = center_y - (s32)(NDS_IFCOMMON_ITEM_CELL_HEIGHT / 2u);

    oamSet(&oamMain, sNdsIFCommonNextOamID, x, y, 0,
           NDS_IFCOMMON_ITEM_PALETTE,
           SpriteSize_16x8, SpriteColorFormat_16Color,
           sNdsIFCommonItemArrow.gfx, matrix_index,
           false, false, false, false, false);
    sNdsIFCommonNextOamID--;
    sNdsIFCommonFrameNeedsCommit = TRUE;
    gNdsIFCommonNativeOamFrameObjectCount++;
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    gNdsIFCommonNativeOamFrameDrawCalls++;
#if NDS_SHIP_TELEMETRY
    ndsIFCommonRecordSemantic(sobj, NDS_IFCOMMON_ASSET_COUNT);
#endif
    return TRUE;
}

#if NDS_P2_1P_GAME
/* The 1P bonus boards' task row (sc1PBonusStageMakeTargetSprites /
 * MakePlatformSprites, sc1pbonusstage.c): one SObj per remaining target or
 * platform, all the same 16x16 CI4 SC1PStageClear3 sprite, SP_TEXSHUF |
 * SP_TRANSPARENT, at the top left of the game screen. The one live bitmap is
 * decoded once into a 16x16 4bpp cell with its own LUT as palette 9
 * (transparent source entries become index 0); each SObj is then placed like
 * the item arrow, through the screen's 0.8 matrix. */
#define NDS_IFCOMMON_TASK_PALETTE 9u
#define NDS_IFCOMMON_TASK_SIZE 16u

static s32 ndsIFCommonBakeTaskIcon(const Sprite *sprite)
{
    u32 cell[NDS_IFCOMMON_TASK_BANK_BYTES / sizeof(u32)];
    u8 *cell_bytes = (u8 *)cell;
    u8 remap[16];
    u16 colors[16];
    const Bitmap *bitmap;
    const u8 *pixels;
    const u16 *lut;
    u32 width_img;
    u32 row_bytes;
    u32 used = 1u;
    u32 *dst;
    u32 i;
    u32 y;

    if ((sprite == NULL) || (sprite->bitmap == NULL) || (sprite->LUT == NULL) ||
        (sprite->bmfmt != G_IM_FMT_CI) || (sprite->bmsiz != G_IM_SIZ_4b) ||
        (sprite->nbitmaps != 1) ||
        ((u32)(u16)sprite->width > NDS_IFCOMMON_TASK_SIZE) ||
        ((u32)(u16)sprite->height > NDS_IFCOMMON_TASK_SIZE))
    {
        return FALSE;
    }
    if (sNdsIFCommonTaskBitmap == sprite->bitmap)
    {
        return TRUE;
    }
    bitmap = sprite->bitmap;
    pixels = (const u8 *)bitmap->buf;
    lut = (const u16 *)sprite->LUT;
    width_img = (u32)(u16)bitmap->width_img;
    if (width_img == 0u)
    {
        width_img = (u32)(u16)bitmap->width;
    }
    if ((pixels == NULL) || (width_img < (u32)(u16)sprite->width))
    {
        return FALSE;
    }
    /* RGBA5551 LUT, halfword-swapped within its words like every converted
     * LUT (entry i at [i ^ 1]); alpha 0 is the cutout. */
    memset(remap, 0, sizeof(remap));
    memset(colors, 0, sizeof(colors));
    for (i = 0u; i < 16u; i++)
    {
        u16 rgba = lut[i ^ 1u];

        if ((rgba & 1u) == 0u)
        {
            continue;
        }
        if (used >= 16u)
        {
            return FALSE;
        }
        remap[i] = (u8)used;
        colors[used] = (u16)(((rgba >> 11) & 0x1fu) |
                             (((rgba >> 6) & 0x1fu) << 5) |
                             (((rgba >> 1) & 0x1fu) << 10));
        used++;
    }
    if (used > 16u)
    {
        return FALSE;
    }
    memset(cell, 0, sizeof(cell));
    row_bytes = (width_img + 1u) / 2u;
    for (y = 0u; y < (u32)(u16)sprite->height; y++)
    {
        u32 x;

        for (x = 0u; x < (u32)(u16)sprite->width; x++)
        {
            /* SP_TEXSHUF: odd rows swap 8-texel halves (the item arrow's
             * reader, ndsIFCommonReadItemArrowI4). */
            u32 shuffled_x = x ^ (((y & 1u) != 0u) ? 8u : 0u);
            u8 packed;
            u8 index;
            u32 offset;

            if (shuffled_x >= width_img)
            {
                return FALSE;
            }
            packed = pixels[((y * row_bytes) + (shuffled_x >> 1)) ^ 3u];
            index = remap[((shuffled_x & 1u) == 0u) ?
                          (packed >> 4) : (packed & 0x0fu)];
            if (index == 0u)
            {
                continue;
            }
            offset = ((y >> 3) * 2u + (x >> 3)) * 32u +
                     ((y & 7u) * 4u) + ((x & 7u) >> 1);
            if ((x & 1u) == 0u)
            {
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0xf0u) | index);
            }
            else
            {
                cell_bytes[offset] =
                    (u8)((cell_bytes[offset] & 0x0fu) | (index << 4));
            }
        }
    }
    dst = (u32 *)(void *)((u8 *)SPRITE_GFX + NDS_IFCOMMON_TASK_BANK_BASE);
    for (i = 0u; i < (u32)(sizeof(cell) / sizeof(cell[0])); i++)
    {
        dst[i] = cell[i];
    }
    for (i = 0u; i < 16u; i++)
    {
        SPRITE_PALETTE[NDS_IFCOMMON_TASK_PALETTE * 16u + i] = colors[i];
    }
    sNdsIFCommonTaskBitmap = sprite->bitmap;
    return TRUE;
}

static s32 ndsIFCommonIsBonusTaskGObj(struct GObj *gobj)
{
    if ((gSCManagerSceneData.scene_curr != nSCKind1PBonusStage) ||
        (gSCManagerBattleState == NULL) || (gobj == NULL))
    {
        return FALSE;
    }
    return (gobj == ((gSCManagerBattleState->gkind >= nGRKindBonus2Start) ?
                     gGRCommonStruct.bonus2.interface_gobj :
                     gGRCommonStruct.bonus1.interface_gobj)) ? TRUE : FALSE;
}

static s32 ndsIFCommonEmitBonusTasks(struct GObj *gobj)
{
    u32 scale_q16 = NDS_IFCOMMON_SCREEN_SCALE_Q16;
    s32 matrix_index = -1;
    SObj *sobj;

    if (gNdsIFCommonNativeOamEnabled == 0u)
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackDisabled);
    }
    for (sobj = SObjGetStruct(gobj); sobj != NULL; sobj = sobj->next)
    {
        s32 origin_x;
        s32 origin_y;
        s32 x;
        s32 y;

        if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        if ((sobj->sprite.scalex != 1.0F) || (sobj->sprite.scaley != 1.0F) ||
            (ndsIFCommonBakeTaskIcon(&sobj->sprite) == FALSE))
        {
            return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackBadAsset);
        }
        if (sNdsIFCommonNextOamID < 0)
        {
            return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackObjectLimit);
        }
        if (matrix_index < 0)
        {
            matrix_index = ndsIFCommonMatrixForScale((u16)(
                ((1u << 24) + (scale_q16 / 2u)) / scale_q16));
            if (matrix_index < 0)
            {
                return ndsIFCommonItemArrowMiss(
                    nNDSIFCommonFallbackMatrixLimit);
            }
        }
        origin_x = ndsIFCommonRoundQ16HalfUp(ndsIFCommonRoundFloatHalfUp(
            sobj->pos.x * (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16));
        origin_y = ndsIFCommonRoundQ16HalfUp(ndsIFCommonRoundFloatHalfUp(
            sobj->pos.y * (f32)NDS_IFCOMMON_SCREEN_SCALE_Q16));
        x = origin_x + ndsIFCommonRoundQ16HalfUp(
            (s32)(NDS_IFCOMMON_TASK_SIZE / 2u) * (s32)scale_q16) -
            (s32)(NDS_IFCOMMON_TASK_SIZE / 2u);
        y = origin_y + ndsIFCommonRoundQ16HalfUp(
            (s32)(NDS_IFCOMMON_TASK_SIZE / 2u) * (s32)scale_q16) -
            (s32)(NDS_IFCOMMON_TASK_SIZE / 2u);
        oamSet(&oamMain, sNdsIFCommonNextOamID, x, y, 0,
               NDS_IFCOMMON_TASK_PALETTE,
               SpriteSize_16x16, SpriteColorFormat_16Color,
               (u16 *)((u8 *)SPRITE_GFX + NDS_IFCOMMON_TASK_BANK_BASE),
               matrix_index, false, false, false, false, false);
        sNdsIFCommonNextOamID--;
        sNdsIFCommonFrameNeedsCommit = TRUE;
        gNdsIFCommonNativeOamFrameObjectCount++;
    }
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    gNdsIFCommonNativeOamFrameDrawCalls++;
    return TRUE;
}
#endif

/* The IFCommonAnnounceCommon messages (ifcommon.c dIFCommonAnnounce*
 * SpriteData: SUDDEN DEATH!, FAILURE, COMPLETE!): IA8 letters with SP_TEXSHUF
 * | SP_TRANSPARENT, prim and env set per message by ifCommonAnnounceSetColors.
 * Each distinct letter is decoded once at the screen's 0.8 grid into a 32x32
 * 4bpp cell in the end bank -- these messages never share the screen with
 * TIME UP or GAME SET, and whichever comes next re-lays the bank -- and the
 * message's ramp (env at intensity 0 to prim at 15, the IA combine) is
 * palette 8. */
#define NDS_IFCOMMON_COMMON_LETTER_MAX 12u
#define NDS_IFCOMMON_COMMON_LETTER_CELL 32u
#define NDS_IFCOMMON_COMMON_LETTER_BYTES 512u
#define NDS_IFCOMMON_COMMON_PALETTE 8u
#define NDS_IFCOMMON_ASSET_ANNOUNCE_COMMON 37u

typedef struct NDSIFCommonCommonLetter
{
    const Bitmap *bitmap;
    u16 *gfx;
} NDSIFCommonCommonLetter;

static NDSIFCommonCommonLetter
    sNdsIFCommonCommonLetters[NDS_IFCOMMON_COMMON_LETTER_MAX];
static u32 sNdsIFCommonCommonLetterCount;
/* The ramp palette 8 holds: the message's prim and env as 0xRRGGBB. */
static u32 sNdsIFCommonCommonPalettePrim;
static u32 sNdsIFCommonCommonPaletteEnv;
static u32 sNdsIFCommonCommonPaletteValid;

/* Owner r71: "Failure" and "Complete" did not draw correctly. The letter
 * cells and palette 8 were cached for the life of the ROM, but both are
 * shared hardware: TIME UP / GAME SET re-lay the end bank under the cached
 * cells, and other scenes' OBJ presenters (source 2D, results) rewrite OBJ
 * VRAM and the palette, so a later message drew from whatever was there --
 * and COMPLETE! plus FAILURE need 13 distinct letters against the 12-cell
 * cache. The cache now lives for one battle scene and one end-bank layout. */
static void ndsIFCommonResetCommonLetters(void)
{
    memset(sNdsIFCommonCommonLetters, 0, sizeof(sNdsIFCommonCommonLetters));
    sNdsIFCommonCommonLetterCount = 0u;
    sNdsIFCommonCommonPaletteValid = FALSE;
}

static s32 ndsIFCommonIsCommonLetter(const SObj *sobj)
{
    u32 asset_id;
    u32 offset;

    return ((sobj->sprite.bitmap != NULL) &&
            (sobj->sprite.bmfmt == G_IM_FMT_IA) &&
            (sobj->sprite.bmsiz == G_IM_SIZ_8b) &&
            (ndsRelocGetLoadedPointerProvenance(sobj->sprite.bitmap,
                                                &asset_id, &offset) != 0) &&
            (asset_id == NDS_IFCOMMON_ASSET_ANNOUNCE_COMMON)) ? TRUE : FALSE;
}

static u16 *ndsIFCommonBakeCommonLetter(const Sprite *sprite)
{
    u32 cell[NDS_IFCOMMON_COMMON_LETTER_BYTES / sizeof(u32)];
    u8 *cell_bytes = (u8 *)cell;
    u32 width = (u32)(u16)sprite->width;
    u32 height = (u32)(u16)sprite->height;
    u32 ds_width;
    u32 ds_height;
    u16 *gfx;
    u32 *dst;
    u32 slot;
    u32 y;

    for (slot = 0u; slot < sNdsIFCommonCommonLetterCount; slot++)
    {
        if (sNdsIFCommonCommonLetters[slot].bitmap == sprite->bitmap)
        {
            return sNdsIFCommonCommonLetters[slot].gfx;
        }
    }
    if (sNdsIFCommonCommonLetterCount >= NDS_IFCOMMON_COMMON_LETTER_MAX)
    {
        return NULL;
    }
    ds_width = (width * 4u + 2u) / 5u;
    ds_height = (height * 4u + 2u) / 5u;
    if ((width == 0u) || (height == 0u) ||
        (ds_width > NDS_IFCOMMON_COMMON_LETTER_CELL) ||
        (ds_height > NDS_IFCOMMON_COMMON_LETTER_CELL))
    {
        return NULL;
    }
    if (sNdsIFCommonCommonLetterCount == 0u)
    {
        /* The end bank is about to hold these letters: TIME UP / GAME SET
         * must be laid out again before either shows. */
        sNdsIFCommonAnnounceActive = FALSE;
    }
    memset(cell, 0, sizeof(cell));
    for (y = 0u; y < ds_height; y++)
    {
        u32 source_y = ((2u * y + 1u) * height) / (2u * ds_height);
        u32 x;

        for (x = 0u; x < ds_width; x++)
        {
            u32 source_x = ((2u * x + 1u) * width) / (2u * ds_width);
            u32 offset = (((y >> 3) * (NDS_IFCOMMON_COMMON_LETTER_CELL >> 3)) +
                          (x >> 3)) * 32u + ((y & 7u) * 4u) + ((x & 7u) >> 1);
            u8 ia;
            u8 index;

            if (ndsIFCommonReadTagI8(sprite, source_x, source_y, &ia) == FALSE)
            {
                return NULL;
            }
            /* The player tags' IA rule: alpha nibble >= 8 is kept, the
             * intensity nibble picks the ramp entry, 0 stays transparent. */
            if ((ia & 0x0fu) < 8u)
            {
                continue;
            }
            index = (u8)(ia >> 4);
            if (index == 0u)
            {
                index = 1u;
            }
            if ((x & 1u) == 0u)
            {
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0xf0u) | index);
            }
            else
            {
                cell_bytes[offset] =
                    (u8)((cell_bytes[offset] & 0x0fu) | (u8)(index << 4));
            }
        }
    }
    gfx = (u16 *)((u8 *)SPRITE_GFX + NDS_IFCOMMON_END_BANK_BASE +
                  sNdsIFCommonCommonLetterCount *
                      NDS_IFCOMMON_COMMON_LETTER_BYTES);
    dst = (u32 *)(void *)gfx;
    for (y = 0u; y < (u32)(sizeof(cell) / sizeof(cell[0])); y++)
    {
        dst[y] = cell[y];
    }
    sNdsIFCommonCommonLetters[sNdsIFCommonCommonLetterCount].bitmap =
        sprite->bitmap;
    sNdsIFCommonCommonLetters[sNdsIFCommonCommonLetterCount].gfx = gfx;
    sNdsIFCommonCommonLetterCount++;
    return gfx;
}

static void ndsIFCommonCommonPalette(const SObj *sobj)
{
    /* Prim and env compared whole: the old single-word key folded them with
     * a mis-parenthesised XOR, and FAILURE's (white over blue) came out as
     * 0xffffffff -- the never-written sentinel -- so its ramp was never
     * written at all. */
    u32 prim = ((u32)sobj->sprite.red << 16) |
               ((u32)sobj->sprite.green << 8) | (u32)sobj->sprite.blue;
    u32 env = ((u32)sobj->envcolor.r << 16) |
              ((u32)sobj->envcolor.g << 8) | (u32)sobj->envcolor.b;
    u16 *palette = &SPRITE_PALETTE[NDS_IFCOMMON_COMMON_PALETTE * 16u];
    u32 i;

    if ((sNdsIFCommonCommonPaletteValid != FALSE) &&
        (prim == sNdsIFCommonCommonPalettePrim) &&
        (env == sNdsIFCommonCommonPaletteEnv))
    {
        return;
    }
    sNdsIFCommonCommonPalettePrim = prim;
    sNdsIFCommonCommonPaletteEnv = env;
    sNdsIFCommonCommonPaletteValid = TRUE;
    palette[0] = 0u;
    for (i = 1u; i < 16u; i++)
    {
        /* Entry i is intensity nibble i (entry 1 also holds intensity 0). */
        u32 t = (i == 1u) ? 0u : i;
        u32 red = sobj->envcolor.r +
            ndsIFCommonDiv15((u32)((s32)sobj->sprite.red -
                                   (s32)sobj->envcolor.r + 255) * t) -
            ndsIFCommonDiv15(255u * t);
        u32 green = sobj->envcolor.g +
            ndsIFCommonDiv15((u32)((s32)sobj->sprite.green -
                                   (s32)sobj->envcolor.g + 255) * t) -
            ndsIFCommonDiv15(255u * t);
        u32 blue = sobj->envcolor.b +
            ndsIFCommonDiv15((u32)((s32)sobj->sprite.blue -
                                   (s32)sobj->envcolor.b + 255) * t) -
            ndsIFCommonDiv15(255u * t);

        palette[i] = ndsIFCommonPackRgb15((u8)red, (u8)green, (u8)blue);
    }
}

static s32 ndsIFCommonEmitCommonLetters(struct GObj *gobj)
{
    SObj *sobj;

    if (gNdsIFCommonNativeOamEnabled == 0u)
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackDisabled);
    }
    for (sobj = SObjGetStruct(gobj); sobj != NULL; sobj = sobj->next)
    {
        u16 *gfx;

        if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        if ((ndsIFCommonIsCommonLetter(sobj) == FALSE) ||
            (sobj->sprite.scalex != 1.0F) || (sobj->sprite.scaley != 1.0F))
        {
            return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackBadAsset);
        }
        gfx = ndsIFCommonBakeCommonLetter(&sobj->sprite);
        if (gfx == NULL)
        {
            return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackObjectLimit);
        }
        if (sNdsIFCommonNextOamID < 0)
        {
            return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackObjectLimit);
        }
        ndsIFCommonCommonPalette(sobj);
        oamSet(&oamMain, sNdsIFCommonNextOamID,
               ndsIFCommonRoundFloatHalfUp(sobj->pos.x * 0.8F),
               ndsIFCommonRoundFloatHalfUp(sobj->pos.y * 0.8F), 0,
               NDS_IFCOMMON_COMMON_PALETTE,
               SpriteSize_32x32, SpriteColorFormat_16Color, gfx, -1,
               false, false, false, false, false);
        sNdsIFCommonNextOamID--;
        sNdsIFCommonFrameNeedsCommit = TRUE;
        gNdsIFCommonNativeOamFrameObjectCount++;
    }
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    gNdsIFCommonNativeOamFrameDrawCalls++;
    return TRUE;
}

s32 ndsIFCommonNativeOamDrawGObj(struct GObj *gobj)
{
#if NDS_RENDERER_HW_TRIANGLES
    SObj *sobj;
    SObj *scan;
    u32 required_objects = 0u;
    u32 start = NDS_IFCOMMON_TELEMETRY_TICK();
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
    if (gobj->proc_display == ifCommonPlayerTagProcDisplay)
    {
        return ndsIFCommonEmitPlayerTag(gobj);
    }
    if (gobj->proc_display == ifCommonItemArrowProcDisplay)
    {
        return ndsIFCommonEmitItemArrow(gobj);
    }
#if NDS_P2_1P_GAME
    if (ndsIFCommonIsBonusTaskGObj(gobj) != FALSE)
    {
        return ndsIFCommonEmitBonusTasks(gobj);
    }
#endif
    if ((gobj->proc_display == lbCommonDrawSObjAttr) &&
        (ndsIFCommonIsCommonLetter(sobj) != FALSE))
    {
        return ndsIFCommonEmitCommonLetters(gobj);
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
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
            return FALSE;
        }
        if (asset_index == -2)
        {
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackRuntimeColor;
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
            return FALSE;
        }
        recognized = TRUE;
        traffic = ndsIFCommonTrafficSpecForAsset((u32)asset_index);
        cloud = ndsIFCommonCloudSpecForAsset((u32)asset_index);
        if (((u32)asset_index >= NDS_IFCOMMON_END_FIRST) &&
            (ndsIFCommonEndAssetActive((u32)asset_index) == FALSE))
        {
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackNotPrepared;
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
            return FALSE;
        }
        if (((traffic != NULL) &&
             (ndsIFCommonTrafficSObjValid(scan, traffic) == FALSE)) ||
            ((cloud != NULL) &&
             (ndsIFCommonCloudSObjValid(scan, cloud) == FALSE)))
        {
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
            return FALSE;
        }
        if ((traffic == NULL) && (cloud == NULL))
        {
            required_objects += sNdsIFCommonAssets[asset_index].tile_count;
        }
#if NDS_SHIP_TELEMETRY
        ndsIFCommonRecordSemantic(scan, (u32)asset_index);
#endif
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
        NDS_IFCOMMON_TELEMETRY_ADD(
            gNdsIFCommonNativeOamFrameTicks, start);
        return FALSE;
    }
    if (sNdsIFCommonPrepared == FALSE)
    {
        gNdsIFCommonNativeOamFrameFallbackCalls++;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackNotPrepared;
        NDS_IFCOMMON_TELEMETRY_ADD(
            gNdsIFCommonNativeOamFrameTicks, start);
        return FALSE;
    }
    if (required_objects > (u32)(sNdsIFCommonNextOamID + 1))
    {
        gNdsIFCommonNativeOamFrameFallbackCalls++;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackObjectLimit;
        NDS_IFCOMMON_TELEMETRY_ADD(
            gNdsIFCommonNativeOamFrameTicks, start);
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
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
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
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
            return FALSE;
        }
    }
    gNdsIFCommonNativeOamFrameDrawCalls++;
    if (gNdsIFCommonNativeOamFrameObjectCount != object_count_before)
    {
        sNdsIFCommonFrameNeedsCommit = TRUE;
    }
    NDS_IFCOMMON_TELEMETRY_ADD(gNdsIFCommonNativeOamFrameTicks, start);
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
        u32 start = NDS_IFCOMMON_TELEMETRY_TICK();

        oamUpdate(&oamMain);
#if NDS_SHIP_TELEMETRY
        gNdsIFCommonNativeOamFrameCommitTicks = cpuGetTiming() - start;
#endif
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
