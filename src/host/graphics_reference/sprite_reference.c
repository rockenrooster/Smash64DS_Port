/* Host-only software SObj rasterizer reference.
 *
 * Owner-adopted `docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md` forbids the
 * generic software scene/sprite compositor in every newly built ROM. This file
 * is the host-only home of that implementation: the exact SObj→preview
 * rasterizer (`ndsDrawSObjIntoPreview`), its wallpaper decode/cache cluster,
 * and the small pure pixel helpers they need. Nothing here may enter an
 * ARM9/ARM7/NDS link; `include/nds/nds_renderer.h` poisons
 * `ndsDrawSObjIntoPreview` in ROM translation units to enforce it, and the
 * ROM counterpart (`src/port/sprite_preview_backend.c`, compiled into the
 * scene_backend TU) keeps only native-safe pieces: native OAM dispatch,
 * converted-image BG/affine wallpaper copy through
 * `src/nds/nds_native_wallpaper.c`, shape tests, the prim/env palette bake,
 * and explicit `ndsRendererRecordNativeFailure` reporting.
 *
 * Small pure helpers mirrored here (RGB555 pack/lerp, Dream Land stretch,
 * Results shape test, combine bake, draw blocker) are byte-identical copies
 * of the ROM-side originals they name; the ROM copies stay because live
 * native paths (combine palette, stretch, shape gates, startup diagnostics,
 * `scripts/check_sprite_lerp_exact.py`) still use them. Everything else below
 * (logo converters, texel mapper, wallpaper decode/cache cluster, destination
 * mapper, the rasterizer itself) MOVED here and is deleted from the ROM side.
 * Keep mirrors in sync by copying, never by diverging.
 *
 * Environment (reloc range checks, platform preview, timing, scene census,
 * DMA/cache) is provided by the host harness linking this file; see the
 * focused host test for the stub contract.
 */
#if defined(ARM9) || defined(ARM7) || defined(__NDS__)
#error Software SObj reference rasterization is host-only
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <ssb_types.h>
#include <PR/sp.h>
#include <PR/mbi.h>
#include <nds/nds_startup.h>

/* Minimal host replicas of the decomp object/scene types. <sys/obj.h> and
 * <sc/scene.h> are not host-compilable (PR/os.h errno field, FTData ARM-ABI
 * asserts), so this file replicates exactly the prefix it uses:
 * - SObj: field order/types verbatim from decomp
 *   `src/sys/objtypes.h:467-481` (GCUserData is 4 bytes on ARM; host pointers
 *   widen it -- test-layout only, behavior identical).
 * - SCCommonData: first-use prefix; scene_curr at offset 0 verified in
 *   `include/sc/scene.h:987-989`.
 * - SCBattleState: first-use prefix; game_type/gkind first verified in
 *   `include/sc/scene.h:972`.
 * - SCKind: enumerators verbatim from `include/sc/scene.h:42-110` (REGION_US
 *   guards kept, so values match the ROM build).
 * - nGRKindPupupu = 6: seventh enumerator of the ROM GRKind order (Castle 0,
 *   Sector 1, Jungle 2, Zebes 3, Hyrule 4, Yoster 5, Pupupu 6). */
typedef struct GObj GObj;
typedef union GCUserData
{
    s32 s;
    u32 u;
    void *p;
} GCUserData;
typedef struct SObj
{
    struct SObj *alloc_free;
    GObj *parent_gobj;
    struct SObj *next;
    struct SObj *prev;
    Sprite sprite;
    GCUserData user_data;
    Vec2f pos;
    SYColorRGBA envcolor;
    u8 cmt, cms;
    u8 maskt, masks;
    u16 lrs, lrt;
} SObj;
typedef struct SCCommonData
{
    u8 scene_curr, scene_prev;
} SCCommonData;
typedef struct SCBattleState
{
    u8 game_type, gkind;
} SCBattleState;
typedef enum SCKind {
    nSCKindNoController,
    nSCKindTitle,
    nSCKindDebugMaps,
    nSCKindDebugCube,
    nSCKindDebugBattle,
    nSCKindDebugFalls,
    nSCKindDebugUnknown,
    nSCKindModeSelect,
    nSCKind1PMode,
    nSCKindVSMode,
    nSCKindVSOptions,
    nSCKindVSItemSwitch,
    nSCKindMessage,
    nSCKind1PChallenger,
    nSCKind1PIntro,
    nSCKindScreenAdjust,
    nSCKindPlayersVS,
    nSCKind1PGamePlayers,
    nSCKindPlayers1PTraining,
    nSCKind1PBonus1Players,
    nSCKind1PBonus2Players,
    nSCKindMaps,
    nSCKindVSBattle,
    nSCKindUnknownMario,
    nSCKindVSResults,
    nSCKindVSRecord,
    nSCKindCharacters,
#if defined(REGION_US)
    nSCKindStartup,
#endif
    nSCKindOpeningRoom,
    nSCKindOpeningPortraits,
    nSCKindOpeningMario,
    nSCKindOpeningDonkey,
    nSCKindOpeningSamus,
    nSCKindOpeningFox,
    nSCKindOpeningLink,
    nSCKindOpeningYoshi,
    nSCKindOpeningPikachu,
    nSCKindOpeningKirby,
    nSCKindOpeningRun,
    nSCKindOpeningYoster,
    nSCKindOpeningCliff,
    nSCKindOpeningStandoff,
    nSCKindOpeningYamabuki,
    nSCKindOpeningClash,
    nSCKindOpeningSector,
    nSCKindOpeningJungle,
    nSCKindOpeningNewcomers,
    nSCKindBackupClear,
    nSCKindEnding,
    nSCKind1PContinue,
    nSCKind1PScoreUnk,
    nSCKind1PStageClear,
    nSCKind1PGame,
    nSCKind1PBonusStage,
    nSCKind1PTrainingMode,
#if defined(REGION_US)
    nSCKindCongra,
#endif
    nSCKindStaffroll,
    nSCKindOption,
    nSCKindData,
    nSCKindSoundTest,
    nSCKindExplain,
    nSCKindAutoDemo
} SCKind;
enum {
    nGRKindPupupu = 6
};

/* NDSRelocLoadedFile lives in `src/port/reloc_backend_assets.c:892-913`
 * (same scene_backend TU as the ROM counterpart, not in a header).
 * Replicated field-for-field so the moved code below keeps working on the
 * same layout; keep in sync by copying, never by diverging. Only asset_id,
 * data, owner_scene, and owner_generation are read here. */
typedef struct NDSRelocLoadedFile {
    u32 asset_id;
    u32 bit;
    void *data;
    u32 data_size;
    u32 owner_scene;
    u32 owner_generation;
    u16 reloc_intern_offset;
    u16 reloc_extern_offset;
    u32 extern_count;
    u16 *extern_file_ids;
    u32 external_fixup_count;
    u32 external_fixup_fail_count;
    u32 internal_fixup_count;
    u8 internal_fixups_applied;
    u8 external_fixups_applied;
    u8 format_fixups_applied;
    u8 fixups_applying;
    u8 offsets_are_relative;
    u8 reserved[3];
} NDSRelocLoadedFile;

/* Build-config knobs the moved code reads. The host harness passes the same
 * values the ROM build uses; shipping ROMs build at PROFILE_LEVEL 0 with the
 * Results affine path enabled. */
#ifndef NDS_RENDERER_PROFILE_LEVEL
#define NDS_RENDERER_PROFILE_LEVEL 0
#endif
#ifndef NDS_TICK_HUD
#define NDS_TICK_HUD 0
#endif
#ifndef NDS_R2_RESULTS_AFFINE
#define NDS_R2_RESULTS_AFFINE 1
#endif
#ifndef NDS_RENDERER_M3_PHASE0_PROFILE
#define NDS_RENDERER_M3_PHASE0_PROFILE 0
#endif
#ifndef NDS_OPENING_ACTION_PREVIEW_MAX_HEIGHT
#define NDS_OPENING_ACTION_PREVIEW_MAX_HEIGHT 264u
#endif

/* ---- Host-provided environment (stubs in the focused host test) ---- */
extern u32 cpuGetTiming(void);
extern NDSRelocLoadedFile *ndsRelocFindLoadedFileContaining(const void *ptr,
                                                            size_t size);
extern s32 ndsRelocPointerRangeInLoadedFile(const NDSRelocLoadedFile *loaded,
                                            const void *ptr, size_t size);
extern u16 *ndsPlatformBeginOriginalSpritePreview(u32 width, u32 height,
                                                  s32 n64_x, s32 n64_y,
                                                  u32 *out_pitch);
extern void ndsPlatformCommitOriginalSpritePreview(void);
extern u32 ndsPlatformGetOriginalSpritePreviewEpoch(void);
extern s32 ndsOpeningIsImportedNameScene(u32 scene);
extern u32 ndsOpeningNameSceneMask(u32 scene);
extern void DC_FlushRange(const void *base, u32 size);
extern void dmaCopyHalfWords(int channel, const void *src, void *dst,
                             u32 size);

/* Scene/diagnostic globals owned by the ROM in production and by the host
 * harness here. */
extern SCCommonData gSCManagerSceneData;
extern SCBattleState *gSCManagerBattleState;
extern volatile u32 gNdsSceneManagerCurrIsBattle;
extern volatile u32 gNdsSObjWallpaperStretchAllStages;
extern volatile u32 gNdsStartupLogoDrawBlocker;
extern volatile u32 gNdsStartupLogoDrawWidth;
extern volatile u32 gNdsStartupLogoDrawHeight;
extern volatile u32 gNdsStartupLogoDrawFormat;
extern volatile u32 gNdsStartupLogoDrawSize;
extern volatile u32 gNdsStartupLogoDrawBitmaps;
extern volatile u32 gNdsStartupLogoDrawTexshuf;
extern volatile u32 gNdsStartupLogoDrawTexshufSamples;
extern volatile u32 gNdsStartupLogoDrawPixels;
extern volatile u32 gNdsStartupLogoDrawResult;
extern volatile u32 gNdsOpeningPortraitsDrawVisibleSObjCount;
extern volatile u32 gNdsOpeningPortraitsDrawWidth;
extern volatile u32 gNdsOpeningPortraitsDrawHeight;
extern volatile u32 gNdsOpeningPortraitsDrawFormat;
extern volatile u32 gNdsOpeningPortraitsDrawSize;
extern volatile u32 gNdsOpeningPortraitsDrawBitmaps;
extern volatile u32 gNdsOpeningPortraitsDrawResult;
extern volatile u32 gNdsOpeningPortraitsDrawPixels;
extern volatile u32 gNdsOpeningPortraitsDrawBlocker;
extern volatile u32 gNdsOpeningMarioDrawVisibleSObjCount;
extern volatile u32 gNdsOpeningMarioDrawWidth;
extern volatile u32 gNdsOpeningMarioDrawHeight;
extern volatile u32 gNdsOpeningMarioDrawFormat;
extern volatile u32 gNdsOpeningMarioDrawSize;
extern volatile u32 gNdsOpeningMarioDrawBitmaps;
extern volatile u32 gNdsOpeningMarioDrawResult;
extern volatile u32 gNdsOpeningMarioDrawPixels;
extern volatile u32 gNdsOpeningMarioDrawBlocker;
extern volatile u32 gNdsOpeningNameSceneDrawVisibleSObjCount;
extern volatile u32 gNdsOpeningNameSceneDrawWidth;
extern volatile u32 gNdsOpeningNameSceneDrawHeight;
extern volatile u32 gNdsOpeningNameSceneDrawFormat;
extern volatile u32 gNdsOpeningNameSceneDrawSize;
extern volatile u32 gNdsOpeningNameSceneDrawBitmaps;
extern volatile u32 gNdsOpeningNameSceneDrawResult;
extern volatile u32 gNdsOpeningNameSceneDrawPixels;
extern volatile u32 gNdsOpeningNameSceneDrawBlocker;
extern volatile u32 gNdsOpeningNameSceneDrawMask;
extern volatile u32 gNdsTitleDrawLastWidth;
extern volatile u32 gNdsTitleDrawLastHeight;
extern volatile u32 gNdsTitleDrawLastFormat;
extern volatile u32 gNdsTitleDrawLastSize;
extern volatile u32 gNdsTitleDrawPixels;
extern volatile u32 gNdsTitleDrawResult;
extern volatile u32 gNdsOpeningMovieActionPreviewResult;
extern volatile u32 gNdsOpeningMovieActionPreviewMask;
extern volatile u32 gNdsOpeningMovieActionPreviewPixels;
extern volatile u32 gNdsOpeningMovieActionPreviewLastKind;
extern volatile u32 gNdsOpeningMovieActionPreviewLastWidth;
extern volatile u32 gNdsOpeningMovieActionPreviewLastHeight;
extern volatile u32 gNdsOpeningMovieActionPreviewLastFormat;
extern volatile u32 gNdsOpeningMovieActionPreviewLastSize;
extern volatile u32 gNdsSObjWallpaperCacheBuildCount;
extern volatile u32 gNdsSObjWallpaperCacheHitCount;
extern volatile u32 gNdsSObjWallpaperCacheFastDrawCount;
extern volatile u32 gNdsSObjWallpaperCacheFallbackCount;
extern volatile u32 gNdsSObjWallpaperCacheWidth;
extern volatile u32 gNdsSObjWallpaperCacheHeight;
extern volatile u32 gNdsSObjWallpaperCacheOpaquePixels;
extern volatile u32 gNdsSObjWallpaperCacheBuildTicks;
extern volatile u32 gNdsSObjWallpaperCacheDrawTicks;
extern volatile u32 gNdsSObjWallpaperFinalDirectCount;
extern volatile u32 gNdsSObjWallpaperFinalSkipCount;
extern volatile u32 gNdsSObjWallpaperFinalKeyChangeCount;
extern volatile u32 gNdsSObjWallpaperFinalPixelWriteCount;
extern volatile u32 gNdsSObjWallpaperMapOracleCheckCount;
extern volatile u32 gNdsSObjWallpaperMapOracleMismatchCount;
extern volatile u32 gNdsSObjWallpaperPixelOracleCheckCount;
extern volatile u32 gNdsSObjWallpaperPixelOracleMismatchCount;
extern volatile u32 gNdsSObjWallpaperOracleFirstKind;
extern volatile u32 gNdsSObjWallpaperOracleFirstIndex;
extern volatile u32 gNdsSObjWallpaperOracleFirstExpected;
extern volatile u32 gNdsSObjWallpaperOracleFirstActual;

/* ---- Moved implementation (verbatim from src/port/sprite_preview_backend.c)
 * ----
 *
 * The software rasterizer cluster below moved here from
 * `src/port/sprite_preview_backend.c` for the native-only rule. The ROM keeps
 * only the native-safe pieces listed in this file's banner. Order and
 * comments are preserved.
 */

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

/* Mirror of the ROM-side original: the live native combine bake still uses
 * it there (see this file's banner). */
static u16 ndsSpritePackRgb15(u8 red, u8 green, u8 blue)
{
    return (u16)((1u << 15) | ((u16)(red >> 3)) |
                 ((u16)(green >> 3) << 5) |
                 ((u16)(blue >> 3) << 10));
}

/* Mirror of the ROM-side original with its exactness proof. Verified
 * exhaustively over [0, 65152] and over the full (colour, envcolour,
 * intensity) input space by `scripts/check_sprite_lerp_exact.py`. Do not
 * widen `intensity` beyond u8 or make `inverse` independent of it. */
#define NDS_SPRITE_DIV255(x) (((x) * 257u + 257u) >> 16)

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

/* Mirror of the ROM-side original: startup/opening diagnostics still record
 * through it there. */
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

#define NDS_DREAMLAND_WALLPAPER_STRETCH_SHIFT 3u

/* Mirror of the ROM-side original: the live native wallpaper transform still
 * applies it there. K = 9/8 = 1.125, shifts/adds only. */
static inline s32 ndsSObjWallpaperMul9Div8Signed(s32 value)
{
    u32 magnitude = (value < 0) ? (u32)(-value) : (u32)value;
    s32 scaled = (s32)(((magnitude << NDS_DREAMLAND_WALLPAPER_STRETCH_SHIFT) +
                        magnitude) >> NDS_DREAMLAND_WALLPAPER_STRETCH_SHIFT);

    return (value < 0) ? -scaled : scaled;
}

/* Mirror of the ROM-side original: the live native wallpaper transform still
 * applies it there. Pure port-side presentation correction; the source
 * SObj/camera state is not changed, and results/menus keep their exact
 * source transform. */
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

/* Mirror of the ROM-side original: the live native wallpaper path still bakes
 * the Results combine through it there. */
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

extern volatile u32 gNdsSObjWallpaperShapeRejectCount;
extern volatile u32 gNdsSObjWallpaperShapeRejectAsset;
extern volatile u32 gNdsSObjWallpaperShapeRejectBitmaps;

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

/* Forward declarations: the ROM orders GetOpaqueWallpaperCache (962) and
 * PublishDrawTicks (1077) before DrawCachedWallpaper (1084); this file keeps
 * that order below, so these announce the later definitions for the earlier
 * caller. */
static s32 ndsSObjGetOpaqueWallpaperCache(
    const NDSRelocLoadedFile *loaded, const Sprite *sprite,
    u32 scale_x_q16, u32 scale_y_q16, u32 scratch_pixels,
    const u16 *combine_palette);
static void ndsSObjWallpaperPublishDrawTicks(u32 draw_start);

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
                    gNdsSObjWallpaperPixelOracleMismatchCount++;
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
        ndsRecordSObjDrawBlocker(record_startup,
                                 NDS_STARTUP_LOGO_BLOCKER_BAD_DIMENSIONS);
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
