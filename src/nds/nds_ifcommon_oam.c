/* IFCommon on OAM: the battle HUD's countdown lamps, GO, the end letters,
 * the player tags and the pause and Training menus as DS sprites, with the
 * GX atlases for the lamps and clouds. This file: the asset table, the
 * atlases and the prefiltered samplers; its slices follow at its end:
 * status, sobjs, pause. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <nds/arm9/background.h>
#include <nds/arm9/console.h>
#include <nds/arm9/sprite.h>
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <nds/timers.h>
#include <PR/sp.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_platform.h>
#include <nds/nds_p5.h>

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
#include <nds/nds_fixed_convert.h>
#include <nds/nds_fcmp.h>
#include <nds/nds_hwmath_unit.h>
#include <nds/nds_fx_effect_census.h>
#include <gm/generic.h>
#include <sys/obj.h>
#include <ft/fighter.h>
#include <sc/scene.h>
#include <if/interface.h>
#if NDS_1P_GAME
#include <gr/ground.h>
#endif
#include <nds/nds_reloc_assets.h>
#include <sys/taskman.h>

/* lbcommon.c's generic SObj display callback (the port's lbcommon header is
 * not included here). */
extern void lbCommonDrawSObjAttr(GObj *gobj);

s32 ndsRendererHardwarePrepareIFCommonA3I5Atlas(
    u32 width, u32 height, const u16 palette[32],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name);


#define NDS_IFCOMMON_GAME_STATUS_SIZE 0x252d4u
#define NDS_IFCOMMON_OBJ_GFX_ALIGNMENT 128u
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
#define NDS_IFCOMMON_CLOUD_ATLAS_COUNT 2u
#define NDS_IFCOMMON_CLOUD_ATLAS0_WIDTH 256u
#define NDS_IFCOMMON_CLOUD_ATLAS1_WIDTH 128u
#define NDS_IFCOMMON_CLOUD_ATLAS_HEIGHT 128u
#define NDS_IFCOMMON_CLOUD_SPEC_COUNT 6u
#define NDS_IFCOMMON_CLOUD_ALPHA_THRESHOLD 8u
#define NDS_IFCOMMON_GO_ALPHA_THRESHOLD 112u
#define NDS_IFCOMMON_TRAFFIC_ALPHA_THRESHOLD 8u
#define NDS_IFCOMMON_TRAFFIC_PROOF_OFFSET 9u
#define NDS_IFCOMMON_OVERLAY_SPEC_COUNT 16u
#define NDS_IFCOMMON_HASH_SEED 0x49464f41u

#define NDS_FX_HIT_SPARK_CAPACITY 16u
#define NDS_FX_HIT_SPARK_CELL_BYTES 512u
#define NDS_FX_HIT_SPARK_FRAME_COUNT 44u
#define NDS_FX_HIT_SPARK_LIGHT_FRAMES 8u
#define NDS_FX_HIT_SPARK_LIGHT_LIFETIME 17u
#define NDS_FX_HIT_SPARK_HEAVY_OFFSET 32u
#define NDS_FX_HIT_SPARK_HEAVY_FRAMES 3u
#define NDS_FX_HIT_SPARK_HEAVY_LIFETIME 6u
#define NDS_FX_HIT_SPARK_SCREEN_SCALE 1.6F

#include "generated/fx_hit_sparks.generated.inc"

extern Mtx44f gGMCameraMatrix;
extern GObj *gGMCameraGObj;
extern f32 syUtilsRandFloat(void);
extern f32 __cosf(f32 value);
extern f32 __sinf(f32 value);
extern void func_ovl2_800EB924(CObj *cobj, Mtx44f matrix, Vec3f *pos,
                               f32 *dist_x, f32 *dist_y);
extern sb32 gmCameraCheckTargetInBounds(f32 pos_x, f32 pos_y);
/* renderer_adapter_fighter.c: func_ovl2_800EB924's fixed projection, its two
 * results left at Q22 (the HUD anchors below stay integers end to end). */
extern void ndsProjectToViewportQ22(CObj *cobj, Mtx44f matrix,
                                    const int32_t p_q12[3],
                                    int64_t *dist_x_q22,
                                    int64_t *dist_y_q22);

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
    nNDSIFCommonFallbackDisabled, /* retired 2026-10-06 with its route word */
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

typedef struct NDSFxHitSpark
{
    Vec3f pos;
    Vec3f vel;
    f32 scale;
    s16 size;
    u8 age;
    u8 player;
    u8 is_heavy;
    u8 active;
} NDSFxHitSpark;

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
static NDSFxHitSpark sNdsFxHitSparks[
    NDS_FX_HIT_SPARK_CAPACITY];
static u16 *sNdsFxHitSparkGfx;

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
volatile u32 gNdsFxSpawnTicks;
volatile u32 gNdsFxUpdateTicks;
volatile u32 gNdsFxDrawTicks;
volatile u32 gNdsFxFrameTicks;
volatile u32 gNdsFxMaxFrameTicks;
volatile u32 gNdsFxEngagementMask;
volatile u32 gNdsFxHitSparkSpawnCount;
volatile u32 gNdsFxHitSparkUpdateCount;
volatile u32 gNdsFxHitSparkDrawCount;
volatile u32 gNdsFxHitSparkDropCount;
volatile u32 gNdsFxFlashDrawCount;
volatile u32 gNdsFxArenaRejectCount;
volatile u32 gNdsFxArenaBootSize;
volatile u32 gNdsFxObjVramBytes;
volatile u32 gNdsFxObjVramRemaining;

static void ndsFxHitSparksDraw(void);
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

/* The GO lamp's lettering, both colourings (black before GO, navy at GO), as
 * a hand-set 12x9 pixel font on the source sprite's 0.8x footprint (owner,
 * 2026-10-06: '"GO" unlit and lit is still blurry and hard to read. maybe we
 * should generate new candidates'; picked candidate E of
 * artifacts/visibility/2026-10-06_go-candidates). The source's 15x11 IA8
 * lettering has two-texel strokes, and at 0.8x every resampling of it puts
 * strokes between DS texels: the point sample read "GC" (owner r75) and the
 * area filter left soft grey strokes. The glyph keeps the source's colour --
 * the mean of the colouring's opaque texels (ndsIFCommonGoLampInk) -- at full
 * coverage. scripts/check_ifcommon_hybrid_oam.py pins the same mask. */
#define NDS_IFCOMMON_GO_LAMP_GLYPH_WIDTH 12u
#define NDS_IFCOMMON_GO_LAMP_GLYPH_HEIGHT 9u
static const char sNdsIFCommonGoLampGlyph[NDS_IFCOMMON_GO_LAMP_GLYPH_HEIGHT]
                                         [NDS_IFCOMMON_GO_LAMP_GLYPH_WIDTH +
                                          1u] = {
    ".####..####.",
    "##....##..##",
    "##....##..##",
    "##.##.##..##",
    "##..#.##..##",
    "##..#.##..##",
    "##..#.##..##",
    ".####..####.",
    "............",
};

/* The mean colour of the lettering sprite's texels above half coverage, in
 * this colouring (its prim/env lerp by intensity). */
static s32 ndsIFCommonGoLampInk(
    const Sprite *sprite, const NDSIFCommonAssetSpec *spec,
    const void *file_data, size_t file_size, u8 ink[3])
{
    u32 width = (u32)(u16)sprite->width;
    u32 height = (u32)(u16)sprite->height;
    u32 sum_red = 0u;
    u32 sum_green = 0u;
    u32 sum_blue = 0u;
    u32 count = 0u;
    u32 y;

    for (y = 0u; y < height; y++)
    {
        u32 x;

        for (x = 0u; x < width; x++)
        {
            u32 texel;

            if (ndsIFCommonReadTrafficRgba(sprite, spec, file_data,
                                           file_size, x, y, &texel) == FALSE)
            {
                return FALSE;
            }
            if ((texel & 0xffu) > 128u)
            {
                sum_red += texel >> 24;
                sum_green += (texel >> 16) & 0xffu;
                sum_blue += (texel >> 8) & 0xffu;
                count++;
            }
        }
    }
    if (count == 0u)
    {
        return FALSE;
    }
    ink[0] = (u8)(sum_red / count);
    ink[1] = (u8)(sum_green / count);
    ink[2] = (u8)(sum_blue / count);
    return TRUE;
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
        if ((traffic->asset_index == nNDSIFCommonAssetShadowInitial) ||
            (traffic->asset_index == nNDSIFCommonAssetShadowGo))
        {
            u8 ink[3];
            u32 palette_index;

            if ((traffic->width != NDS_IFCOMMON_GO_LAMP_GLYPH_WIDTH) ||
                (traffic->height != NDS_IFCOMMON_GO_LAMP_GLYPH_HEIGHT) ||
                (ndsIFCommonGoLampInk(asset->sprite, asset_spec,
                                      context->file_data, context->file_size,
                                      ink) == FALSE))
            {
                gNdsIFCommonNativeOamPrepareCloudFailureStage = 2u;
                return FALSE;
            }
            palette_index = ndsIFCommonPaletteIndex(
                sNdsIFCommonTrafficPalette, ink[0], ink[1], ink[2]);
            for (y = 0u; y < NDS_IFCOMMON_GO_LAMP_GLYPH_HEIGHT; y++)
            {
                u32 x;

                for (x = 0u; x < NDS_IFCOMMON_GO_LAMP_GLYPH_WIDTH; x++)
                {
                    if (sNdsIFCommonGoLampGlyph[y][x] != '#')
                    {
                        continue;
                    }
                    gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[
                        NDS_IFCOMMON_TRAFFIC_PROOF_OFFSET + traffic_index]++;
                    pixels[((u32)traffic->atlas_y + y) *
                               NDS_IFCOMMON_TRAFFIC_ATLAS_WIDTH +
                           (u32)traffic->atlas_x + x] =
                        (u8)((7u << 5) | palette_index);
                }
            }
            continue;
        }
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
                if (traffic->asset_index >= nNDSIFCommonAssetRedDim)
                {
                    /* The dim lamps are TRANSLUCENT on the N64: an I4
                     * coverage disc of the lamp's prim colour at about 47%
                     * over the housing, whose sockets carry the bulbs'
                     * shading and highlight. Baked opaque (the rule until
                     * 2026-10-04) they hid that and read as flat discs (owner
                     * r75). They keep their coverage as the texel's A3 alpha
                     * and their own colour. */
                    alpha3 = ((u32)rgba[3] * 7u + 127u) / 255u;
                    if (alpha3 == 0u)
                    {
                        continue;
                    }
                    red = asset_spec->red;
                    green = asset_spec->green;
                    blue = asset_spec->blue;
                }
                else
                {
                    if (rgba[3] < NDS_IFCOMMON_TRAFFIC_ALPHA_THRESHOLD)
                    {
                        continue;
                    }
                    /* The rod and housing are opaque source art. Bake the
                     * filtered coverage into RGB so their shading survives,
                     * then use A3 only as a hard cutout. */
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

#include "nds_ifcommon_oam_status.c"
#include "nds_ifcommon_oam_sobjs.c"
#include "nds_ifcommon_oam_pause.c"
