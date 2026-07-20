#include <nds.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <nds/nds_boot.h>
#include <nds/nds_audio_bgm.h>
#include <nds/nds_controller.h>
#include <nds/nds_freeze_diagnostics.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_platform.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/nds_scene.h>
#include <nds/nds_startup.h>
#include <nds/nds_video.h>
#include <sys/controller.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#ifndef NDS_DEBUG_HUD
#define NDS_DEBUG_HUD 1
#endif

#ifndef NDS_SCENE_MIP_CACHE_LAB
#define NDS_SCENE_MIP_CACHE_LAB 0
#endif

#ifndef NDS_FAST_WALLPAPER_AFFINE
#define NDS_FAST_WALLPAPER_AFFINE 0
#endif

extern volatile u32 gNdsBootSelfTestResult;
extern volatile u32 gNdsFrameCounter;

#define NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH 320u
#define NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT 240u
#define NDS_ORIGINAL_DL_PREVIEW_MAX_WIDTH 96u
#define NDS_ORIGINAL_DL_PREVIEW_MAX_HEIGHT 72u
#define NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH 72u
#define NDS_ORIGINAL_DL_PREVIEW_DISPLAY_HEIGHT 54u
#define NDS_ORIGINAL_DL_PREVIEW_DISPLAY_X 0
#define NDS_ORIGINAL_DL_PREVIEW_DISPLAY_Y 128
#define NDS_ORIGINAL_DL_PREVIEW_BORDER_COLOR RGB15(26, 18, 0)
#define NDS_ORIGINAL_DL_PREVIEW_BG_COLOR RGB15(2, 2, 2)
#define NDS_N64_LOGICAL_WIDTH 320
#define NDS_N64_LOGICAL_HEIGHT 240
#define NDS_TOP_BACKGROUND_COLOR (RGB15(2, 3, 6) | BIT(15))
#define NDS_PERF_SAMPLE_TICKS 60u
#define NDS_BATTLE_FPS_HUD_SAMPLE_TICKS (BUS_CLOCK / 2u)
#define NDS_BATTLE_SOURCE_TICKS_PER_SECOND 60u
#define NDS_BATTLE_FPS_HUD_ENABLED \
    ((NDS_HARNESS_FAST_LOGIC == 0) && \
     (NDS_RENDERER_HW_TRIANGLES != 0) && \
     (NDS_DEV_LIVE_INPUT_PREVIEW != 0) && \
     (NDS_DEBUG_HUD == 0))
#define NDS_BATTLE_PHASE_HUD_ENABLED \
    (NDS_BATTLE_FPS_HUD_ENABLED && (NDS_RENDERER_PROFILE_LEVEL >= 1))
#if !NDS_RENDERER_HW_TRIANGLES
static u16 *sFramebuffer;
static u16 *sFramebuffers[2];
static u32 sDrawFramebufferIndex;
#endif
static u32 sTicks;
static u32 sHeldKeys;
static volatile u32 sVBlankCount;
static u32 sEarliestPresentVBlank;
volatile u32 gNdsPlatformHeldKeys;
static u32 sPerfSampleReady;
static u32 sPerfLastTick;
static u32 sPerfLastFrameCounter;
static u32 sPerfLastLogicTickCount;
static u32 sPerfLastDLPreviewDrawCount;
static u32 sPerfLastPreviewCommitCount;
#if NDS_BATTLE_FPS_HUD_ENABLED
static u32 sBattleFpsHudSampleReady;
static u32 sBattleFpsHudLastTick;
static u32 sBattleFpsHudLastPresentedFrames;
static u32 sBattleFpsHudLastLogicFrames;
static u32 sBattleFpsHudPrintedFpsX10 = 0xffffffffu;
static u32 sBattleFpsHudPrintedUpdatesX10 = 0xffffffffu;
static u32 sBattleTextHudReady;
static u32 sBattleTextHudFingerprint = 0xffffffffu;
#if NDS_BATTLE_PHASE_HUD_ENABLED
static u32 sBattlePhaseHudLastSlipCount;
#endif
#endif
static u16 sOriginalSpritePreview[
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH *
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT];
static u32 sOriginalSpritePreviewWidth;
static u32 sOriginalSpritePreviewHeight;
static s32 sOriginalSpritePreviewX;
static s32 sOriginalSpritePreviewY;
static u32 sOriginalSpritePreviewReady;
#if NDS_RENDERER_HW_TRIANGLES
static int sOriginalSpriteOverlayBg = -1;
static int sOriginalSpriteOverlayForegroundBg = -1;
static u32 sOriginalSpriteOverlayLayerMask;
static s32 sOriginalSpriteOverlayNeedsFlush;
static u32 sOriginalSpriteOverlayEpoch[2] = { 1u, 1u };
#if NDS_FAST_WALLPAPER_AFFINE
typedef struct NDSFastWallpaperTransform
{
    s32 origin_x;
    s32 origin_y;
    u32 scale_x_q16;
    u32 scale_y_q16;
} NDSFastWallpaperTransform;

typedef struct NDSFastWallpaperAffine
{
    s32 hdx;
    s32 vdy;
    s32 dx;
    s32 dy;
} NDSFastWallpaperAffine;

typedef struct NDSFastWallpaperOwner
{
    NDSFastWallpaperState state;
    NDSFastWallpaperTransform seed;
    NDSFastWallpaperTransform latest;
    NDSFastWallpaperAffine pending;
    NDSFastWallpaperAffine committed;
    u32 latest_valid;
    u32 pending_valid;
    u32 committed_valid;
    u32 overlay_generation;
    u32 asset_identity;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 seed_start_ticks;
#endif
} NDSFastWallpaperOwner;

static NDSFastWallpaperOwner sFastWallpaper = {
    .state = NDS_FAST_WALLPAPER_UNSEEDED,
    .committed = { 1 << 8, 1 << 8, 0, 0 },
    .committed_valid = TRUE
};

static void ndsPlatformFastWallpaperResetInternal(void);
static void ndsPlatformFastWallpaperCommitAffine(void);
#endif
#if NDS_SCENE_MIP_CACHE_LAB
typedef struct NDSSceneWallpaperTransform
{
    s32 origin_x;
    s32 origin_y;
    u32 scale_x_q16;
    u32 scale_y_q16;
} NDSSceneWallpaperTransform;

static u32 sSceneMipCapturePending;
static u32 sSceneMipCaptureCompleted;
static u32 sSceneMipCacheReady;
static u32 sSceneMipCacheFailed;
static NDSSceneWallpaperTransform sSceneWallpaperSeedTransform;
static NDSSceneWallpaperTransform sSceneWallpaperLatestTransform;
static u32 sSceneWallpaperLatestTransformValid;
static u32 sSceneWallpaperSeedRasterCommitted;
static s32 sSceneWallpaperPendingHdx;
static s32 sSceneWallpaperPendingVdy;
static s32 sSceneWallpaperPendingDx;
static s32 sSceneWallpaperPendingDy;
static u32 sSceneWallpaperAffinePending;
static u32 sSceneWallpaperAffineResetPending;
#endif
#endif
static u16 sOriginalSpriteDisplayPreview[
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH *
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT];
static u32 sOriginalSpriteDecodeCacheEpoch = 1u;
static u32 sOriginalSpriteDisplayPreviewWidth;
static u32 sOriginalSpriteDisplayPreviewHeight;
static u16 sOriginalDLPreview[
    NDS_ORIGINAL_DL_PREVIEW_MAX_WIDTH *
    NDS_ORIGINAL_DL_PREVIEW_MAX_HEIGHT];
static u16 sOriginalDLDisplayPreview[
    NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH *
    NDS_ORIGINAL_DL_PREVIEW_DISPLAY_HEIGHT];
static u32 sOriginalDLPreviewWidth;
static u32 sOriginalDLPreviewHeight;
static u32 sOriginalDLDisplayPreviewWidth;
static u32 sOriginalDLDisplayPreviewHeight;
static u32 sOriginalDLPreviewReady;
static u32 sDebugTextFingerprint = 0xffffffffu;
static u32 sDebugTextReady;

volatile u32 gNdsOriginalSpritePreviewReady;
volatile u32 gNdsOriginalSpritePreviewCommitCount;
volatile u32 gNdsOriginalSpritePreviewDrawCount;
/* These retain the last committed frame while a new scratch layer is built. */
volatile u32 gNdsOriginalSpritePreviewDisplayWidth;
volatile u32 gNdsOriginalSpritePreviewDisplayHeight;
volatile u32 gNdsOriginalSpriteBg2ClearBytes;
volatile u32 gNdsOriginalSpriteBg2CopyBytes;
volatile u32 gNdsOriginalSpriteBg2FinalWriteBytes;
volatile u32 gNdsOriginalSpriteBg3ClearBytes;
volatile u32 gNdsOriginalSpriteBg3CopyBytes;
volatile u32 gNdsOriginalSpriteBg3FinalWriteBytes;
volatile u32 gNdsOriginalDLPreviewReady;
volatile u32 gNdsOriginalDLPreviewWidth;
volatile u32 gNdsOriginalDLPreviewHeight;
volatile u32 gNdsOriginalDLPreviewCommitCount;
volatile u32 gNdsOriginalDLPreviewDrawCount;
volatile u32 gNdsPerfPresentFps;
volatile u32 gNdsPerfLogicFps;
volatile u32 gNdsPerfDLDrawFps;
volatile u32 gNdsPerfPreviewCommitFps;
volatile u32 gNdsPerfPreviewCommitCount;
volatile u32 gNdsPerfSampleCount;
volatile u32 gNdsPerfSampleWindowTicks;
volatile u32 gNdsBattlePlayableHudFpsX10;
volatile u32 gNdsBattlePlayableHudFpsSampleCount;
volatile u32 gNdsBattlePlayableHudFpsFrameWindow;
volatile u32 gNdsBattlePlayableHudFpsTickWindow;
volatile u32 gNdsBattleTextHudRenderCount;
volatile u32 gNdsBattleTextHudChangeCount;
volatile u32 gNdsBattleTextHudFingerprint;
volatile u32 gNdsBattleTextHudTimeSeconds;
volatile u32 gNdsBattleTextHudP0Damage;
volatile u32 gNdsBattleTextHudP1Damage;
volatile u32 gNdsBattleTextHudP0Stock;
volatile u32 gNdsBattleTextHudP1Stock;
volatile u32 gNdsBattleTextHudActiveMask;
volatile u32 gNdsBattleTextHudShowDamageMask;
volatile u32 gNdsBattleTextHudClearCount;
volatile u32 gNdsHardwareRendererSubmittedFrameCount;
volatile u32 gNdsHardwareRendererFlushCount;
volatile u32 gNdsHardwareRendererPolyRamCount;
volatile u32 gNdsHardwareRendererVertexRamCount;
volatile u32 gNdsHardwareRendererStatus;
volatile u32 gNdsHardwareRendererControl;
volatile u32 gNdsSceneMipCacheState;
volatile u32 gNdsSceneMipCacheCaptureCount;
volatile u32 gNdsSceneMipCacheUploadCount;
volatile u32 gNdsSceneMipCacheFailureCount;
volatile u32 gNdsSceneMipCacheLastHash;
volatile u32 gNdsSceneMipCacheLastNonzeroPixels;
volatile u32 gNdsSceneWallpaperAffineQueueCount;
volatile u32 gNdsSceneWallpaperAffineApplyCount;
volatile u32 gNdsSceneWallpaperAffineCoverageFailureCount;
volatile u32 gNdsSceneWallpaperAffineLastTicks;
volatile s32 gNdsSceneWallpaperAffineHdx;
volatile s32 gNdsSceneWallpaperAffineVdy;
volatile s32 gNdsSceneWallpaperAffineDx;
volatile s32 gNdsSceneWallpaperAffineDy;
volatile u32 gNdsFastWallpaperState;
volatile u32 gNdsFastWallpaperSeedAttemptCount;
volatile u32 gNdsFastWallpaperSeedSuccessCount;
volatile u32 gNdsFastWallpaperSeedFailureCount;
volatile u32 gNdsFastWallpaperStaticDegradedCount;
volatile u32 gNdsFastWallpaperSeedTicks;
volatile u32 gNdsFastWallpaperQueueCount;
volatile u32 gNdsFastWallpaperApplyCount;
volatile u32 gNdsFastWallpaperUnchangedSkipCount;
volatile u32 gNdsFastWallpaperClampXCount;
volatile u32 gNdsFastWallpaperClampYCount;
volatile u32 gNdsFastWallpaperClampScaleCount;
volatile u32 gNdsFastWallpaperInvalidTransformCount;
volatile u32 gNdsFastWallpaperReusePreviousCount;
volatile u32 gNdsFastWallpaperAffineLastTicks;
volatile u32 gNdsFastWallpaperPostReadySoftwareDrawCount;
volatile u32 gNdsFastWallpaperPostReadyPixelWriteCount;
volatile u32 gNdsFastWallpaperSeedHash;
volatile u32 gNdsFastWallpaperSeedOpaquePixelCount;
volatile u32 gNdsFastWallpaperSeedRestoreMismatchCount;

static void ndsPlatformVBlankInterrupt(void)
{
    sVBlankCount++;
}

void ndsPlatformInit(void)
{
    /* libultra time/count are sub-frame hardware counters. Reserve timers 0/1
     * for libnds' 32-bit CPU timing source before original code can sample it. */
    cpuStartTiming(0);
    sVBlankCount = 0u;
    sEarliestPresentVBlank = 0u;
    irqSet(IRQ_VBLANK, ndsPlatformVBlankInterrupt);
    irqEnable(IRQ_VBLANK);

#if NDS_BATTLE_FPS_HUD_ENABLED
    gNdsIFCommonHUDLowerTextMode = 1u;
#else
    gNdsIFCommonHUDLowerTextMode = 0u;
#endif

#if NDS_RENDERER_HW_TRIANGLES
    videoSetMode(MODE_5_3D | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE);
    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_TEXTURE);
    /* IFCommon's opaque OAM assets fit in E. Its source-alpha flare uses
     * A5I3 hardware textures, so F/G remain texture-palette banks. */
    vramSetBankE(VRAM_E_MAIN_SPRITE);
    vramSetBankF(VRAM_F_TEX_PALETTE_SLOT0);
    vramSetBankG(VRAM_G_TEX_PALETTE_SLOT1);
    ndsIFCommonNativeOamInit();
    glInit();
    glClearColor(2, 3, 6, 31);
    glClearDepth(GL_MAX_DEPTH);
    glEnable(GL_ANTIALIAS);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glViewport(0, 0, 255, 191);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    vramSetBankC(VRAM_C_MAIN_BG_0x06000000);
    vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
    sOriginalSpriteOverlayBg =
        bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    sOriginalSpriteOverlayForegroundBg =
        bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 8, 0);
    bgSetPriority(sOriginalSpriteOverlayForegroundBg, 0);
    bgSetPriority(0, 1);
    bgSetPriority(sOriginalSpriteOverlayBg, 2);
    bgSetAffineMatrixScroll(sOriginalSpriteOverlayBg,
                            1 << 8, 0, 0, 1 << 8, 0, 0);
    bgWrapOff(sOriginalSpriteOverlayBg);
    REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG0 | BLEND_DST_BG2;
    REG_BLDALPHA = 16u | (16u << 8);
    dmaFillHalfWords(0, bgGetGfxPtr(sOriginalSpriteOverlayBg),
                     256u * 256u * sizeof(u16));
    dmaFillHalfWords(0, bgGetGfxPtr(sOriginalSpriteOverlayForegroundBg),
                     256u * 256u * sizeof(u16));
#else
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    sFramebuffers[0] = VRAM_A;
    sFramebuffers[1] = VRAM_B;
    sDrawFramebufferIndex = 1;
    sFramebuffer = sFramebuffers[sDrawFramebufferIndex];
    dmaFillHalfWords(NDS_TOP_BACKGROUND_COLOR, sFramebuffers[0],
                     SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
    dmaFillHalfWords(NDS_TOP_BACKGROUND_COLOR, sFramebuffers[1],
                     SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
#endif

    videoSetModeSub(MODE_0_2D);
    vramSetBankH(VRAM_H_SUB_BG);
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 15, 0, false, true);
    iprintf("\x1b[?25l");
    NDS_FREEZE_DIAGNOSTICS_INIT();

#if NDS_DEBUG_HUD
    iprintf("Smash 64 DS Port\n");
    iprintf("================\n");
    iprintf("melonDS visual debug active\n");
#endif
}

u32 ndsPlatformReadInput(void)
{
    u32 input = 0;
    u32 held;

    scanKeys();
    held = keysHeld();
    sHeldKeys = held;
    gNdsPlatformHeldKeys = held;

    if (held & KEY_LEFT) input |= NDS_INPUT_LEFT;
    if (held & KEY_RIGHT) input |= NDS_INPUT_RIGHT;
    if (held & KEY_UP) input |= NDS_INPUT_UP;
    if (held & KEY_DOWN) input |= NDS_INPUT_DOWN;
    if (held & KEY_A) input |= NDS_INPUT_A;
    if (held & KEY_START) input |= NDS_INPUT_START;
    if (held & KEY_B) input |= NDS_INPUT_B;
    if (held & KEY_X) input |= NDS_INPUT_X;
    if (held & KEY_Y) input |= NDS_INPUT_Y;
    if (held & KEY_L) input |= NDS_INPUT_L;
    if (held & KEY_R) input |= NDS_INPUT_R;

    return input;
}

void ndsPlatformBeginFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    return;
#else
    dmaFillHalfWords(NDS_TOP_BACKGROUND_COLOR, sFramebuffer,
                     SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
#endif
}

void ndsPlatformDrawRect(s32 x, s32 y, s32 width, s32 height, u16 color)
{
#if NDS_RENDERER_HW_TRIANGLES
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
#else
    s32 row;
    s32 column;

    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > SCREEN_WIDTH) width = SCREEN_WIDTH - x;
    if (y + height > SCREEN_HEIGHT) height = SCREEN_HEIGHT - y;
    if (width <= 0 || height <= 0) return;

    color |= BIT(15);
    for (row = 0; row < height; row++)
    {
        u16 *dst = sFramebuffer + ((y + row) * SCREEN_WIDTH) + x;
        for (column = 0; column < width; column++)
        {
            dst[column] = color;
        }
    }
#endif
}

u16 *ndsPlatformBeginOriginalSpritePreview(u32 width, u32 height,
                                           s32 n64_x, s32 n64_y,
                                           u32 *out_pitch)
{
    u32 row;

    if ((width == 0) || (height == 0) ||
        (width > NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH) ||
        (height > NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT))
    {
        return NULL;
    }

    sOriginalSpritePreviewWidth = width;
    sOriginalSpritePreviewHeight = height;
    sOriginalSpritePreviewX = n64_x;
    sOriginalSpritePreviewY = n64_y;
    sOriginalSpritePreviewReady = 0;
    sOriginalSpriteDisplayPreviewWidth = 0;
    sOriginalSpriteDisplayPreviewHeight = 0;
    gNdsOriginalSpritePreviewReady = 0;
    for (row = 0; row < height; row++)
    {
        memset(&sOriginalSpritePreview[
                   row * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH],
               0, width * sizeof(sOriginalSpritePreview[0]));
    }

    if (out_pitch != NULL)
    {
        *out_pitch = NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH;
    }
    return sOriginalSpritePreview;
}

u16 *ndsPlatformGetOriginalSpriteDecodeCache(u32 *out_pitch,
                                              u32 *out_height,
                                              u32 *out_epoch)
{
#if NDS_RENDERER_HW_TRIANGLES
    if (out_pitch != NULL)
    {
        *out_pitch = NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH;
    }
    if (out_height != NULL)
    {
        *out_height = NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT;
    }
    if (out_epoch != NULL)
    {
        *out_epoch = sOriginalSpriteDecodeCacheEpoch;
    }
    return sOriginalSpriteDisplayPreview;
#else
    if (out_pitch != NULL) { *out_pitch = 0u; }
    if (out_height != NULL) { *out_height = 0u; }
    if (out_epoch != NULL) { *out_epoch = sOriginalSpriteDecodeCacheEpoch; }
    return NULL;
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static u32 ndsPlatformAdvanceOriginalSpriteOverlayEpoch(u32 layer)
{
    sOriginalSpriteOverlayEpoch[layer]++;
    if (sOriginalSpriteOverlayEpoch[layer] == 0u)
    {
        sOriginalSpriteOverlayEpoch[layer] = 1u;
    }
    return sOriginalSpriteOverlayEpoch[layer];
}

static u32 ndsPlatformOriginalSpriteOverlayClearPixels(void)
{
#if NDS_SCENE_MIP_CACHE_LAB
    if ((sSceneMipCapturePending != 0u) ||
        ((sSceneMipCaptureCompleted != 0u) &&
         (sSceneMipCacheReady == FALSE) &&
         (sSceneMipCacheFailed == FALSE)))
    {
        /* Rows 192..255 stage already captured 128x128 scene textures. */
        return SCREEN_WIDTH * SCREEN_HEIGHT;
    }
#endif
    return 256u * 256u;
}
#endif

u16 *ndsPlatformGetOriginalSpriteOverlayLayer(s32 is_foreground,
                                               u32 *out_pitch,
                                               u32 *out_width,
                                               u32 *out_height,
                                               u32 *out_epoch)
{
    if (out_pitch != NULL) { *out_pitch = 0u; }
    if (out_width != NULL) { *out_width = 0u; }
    if (out_height != NULL) { *out_height = 0u; }
    if (out_epoch != NULL) { *out_epoch = 0u; }

#if NDS_RENDERER_HW_TRIANGLES
    {
        u32 layer = (is_foreground != FALSE) ? 1u : 0u;
        int bg = (layer != 0u) ?
            sOriginalSpriteOverlayForegroundBg : sOriginalSpriteOverlayBg;

        if (((sOriginalSpriteOverlayLayerMask & (1u << layer)) == 0u) ||
            (bg < 0))
        {
            return NULL;
        }
        if (out_pitch != NULL) { *out_pitch = 256u; }
        if (out_width != NULL) { *out_width = SCREEN_WIDTH; }
        if (out_height != NULL) { *out_height = SCREEN_HEIGHT; }
        if (out_epoch != NULL)
        {
            *out_epoch = sOriginalSpriteOverlayEpoch[layer];
        }
        return (u16 *)bgGetGfxPtr(bg);
    }
#else
    (void)is_foreground;
    return NULL;
#endif
}

u32 ndsPlatformCommitOriginalSpriteFinalLayer(s32 is_foreground,
                                               u32 pixel_write_count)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 layer = (is_foreground != FALSE) ? 1u : 0u;
    int bg = (layer != 0u) ?
        sOriginalSpriteOverlayForegroundBg : sOriginalSpriteOverlayBg;
    u32 bytes;

    if (((sOriginalSpriteOverlayLayerMask & (1u << layer)) == 0u) ||
        (bg < 0) ||
        (pixel_write_count > (SCREEN_WIDTH * SCREEN_HEIGHT)))
    {
        return 0u;
    }
    bytes = pixel_write_count * sizeof(u16);
    if (layer != 0u)
    {
        gNdsOriginalSpriteBg3FinalWriteBytes += bytes;
    }
    else
    {
#if NDS_FAST_WALLPAPER_AFFINE
        if ((sFastWallpaper.state == NDS_FAST_WALLPAPER_READY) ||
            (sFastWallpaper.state == NDS_FAST_WALLPAPER_STATIC_DEGRADED))
        {
            gNdsFastWallpaperPostReadyPixelWriteCount += pixel_write_count;
        }
#endif
        gNdsOriginalSpriteBg2FinalWriteBytes += bytes;
    }
    sOriginalSpriteDisplayPreviewWidth = SCREEN_WIDTH;
    sOriginalSpriteDisplayPreviewHeight = SCREEN_HEIGHT;
    sOriginalSpritePreviewReady = 1u;
    gNdsOriginalSpritePreviewReady = 1u;
    gNdsOriginalSpritePreviewDisplayWidth = SCREEN_WIDTH;
    gNdsOriginalSpritePreviewDisplayHeight = SCREEN_HEIGHT;
    gNdsOriginalSpritePreviewCommitCount++;
    return ndsPlatformAdvanceOriginalSpriteOverlayEpoch(layer);
#else
    (void)is_foreground;
    (void)pixel_write_count;
    return 0u;
#endif
}

#if !NDS_RENDERER_HW_TRIANGLES
static void ndsPlatformCopyOriginalSpritePreviewNative(
    u16 *destination, u32 destination_pitch, s32 dst_w, s32 dst_h)
{
    s32 y;

    if ((destination == NULL) || (destination_pitch < (u32)dst_w))
    {
        return;
    }
    for (y = 0; y < dst_h; y++)
    {
        memcpy(&destination[y * destination_pitch],
               &sOriginalSpritePreview[
                   y * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH],
               (size_t)dst_w * sizeof(destination[0]));
    }
}
#endif

static void ndsPlatformScaleOriginalSpritePreviewNearest(
    u16 *destination, u32 destination_pitch, s32 dst_w, s32 dst_h)
{
    u32 step_x;
    u32 step_y;
    u32 src_y_q16;
    s32 y;

    if ((destination == NULL) || (dst_w <= 0) || (dst_h <= 0) ||
        (destination_pitch < (u32)dst_w))
    {
        return;
    }

    step_x = (sOriginalSpritePreviewWidth << 16) / (u32)dst_w;
    step_y = (sOriginalSpritePreviewHeight << 16) / (u32)dst_h;
    src_y_q16 = step_y >> 1;

    for (y = 0; y < dst_h; y++)
    {
        u32 src_y = src_y_q16 >> 16;
        u32 src_x_q16 = step_x >> 1;
        u16 *dst = &destination[y * destination_pitch];
        const u16 *src;
        s32 x;

        if (src_y >= sOriginalSpritePreviewHeight)
        {
            src_y = sOriginalSpritePreviewHeight - 1u;
        }
        src = &sOriginalSpritePreview[
            src_y * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH];

        for (x = 0; x < dst_w; x++)
        {
            u32 src_x = src_x_q16 >> 16;

            if (src_x >= sOriginalSpritePreviewWidth)
            {
                src_x = sOriginalSpritePreviewWidth - 1u;
            }
            dst[x] = src[src_x];
            src_x_q16 += step_x;
        }
        src_y_q16 += step_y;
    }
}

void ndsPlatformCommitOriginalSpritePreviewLayer(s32 is_foreground)
{
    s32 dst_w;
    s32 dst_h;
#if NDS_RENDERER_HW_TRIANGLES
    const u16 *display_preview = sOriginalSpritePreview;
#endif

    if ((sOriginalSpritePreviewWidth == 0) ||
        (sOriginalSpritePreviewHeight == 0))
    {
        return;
    }

    /* SW keeps its retained visual diagnostic at native asset resolution. HW
     * consumes staging directly into BG VRAM and owns no retained frame copy. */
    dst_w = (s32)sOriginalSpritePreviewWidth;
    dst_h = (s32)sOriginalSpritePreviewHeight;
    if (dst_w <= 0) dst_w = 1;
    if (dst_h <= 0) dst_h = 1;
    if (dst_w > SCREEN_WIDTH)
    {
        dst_w = SCREEN_WIDTH;
    }
    if (dst_h > SCREEN_HEIGHT)
    {
        dst_h = SCREEN_HEIGHT;
    }
    if (dst_w > (s32)NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH)
    {
        dst_w = NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH;
    }
    if (dst_h > (s32)NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT)
    {
        dst_h = NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT;
    }

#if NDS_RENDERER_HW_TRIANGLES
    /* Hardware layers consume staging immediately. Downscaling in place is
     * safe because both source coordinates advance at least as quickly as the
     * destination; it leaves the retained display buffer free for immutable
     * decoded sprite data without adding a second pixel buffer. */
    if ((dst_w != (s32)sOriginalSpritePreviewWidth) ||
        (dst_h != (s32)sOriginalSpritePreviewHeight))
    {
        ndsPlatformScaleOriginalSpritePreviewNearest(
            sOriginalSpritePreview,
            NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH, dst_w, dst_h);
        /* Staging now contains the compacted image. Publish its new extent so
         * a repeated commit is idempotent until the next Begin call. */
        sOriginalSpritePreviewWidth = (u32)dst_w;
        sOriginalSpritePreviewHeight = (u32)dst_h;
    }
#else
    if ((dst_w == (s32)sOriginalSpritePreviewWidth) &&
        (dst_h == (s32)sOriginalSpritePreviewHeight))
    {
        ndsPlatformCopyOriginalSpritePreviewNative(
            sOriginalSpriteDisplayPreview,
            NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH, dst_w, dst_h);
    }
    else
    {
        ndsPlatformScaleOriginalSpritePreviewNearest(
            sOriginalSpriteDisplayPreview,
            NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH, dst_w, dst_h);
    }
#endif
    sOriginalSpriteDisplayPreviewWidth = (u32)dst_w;
    sOriginalSpriteDisplayPreviewHeight = (u32)dst_h;
    sOriginalSpritePreviewReady = 1;
    gNdsOriginalSpritePreviewReady = 1;
    gNdsOriginalSpritePreviewDisplayWidth = (u32)dst_w;
    gNdsOriginalSpritePreviewDisplayHeight = (u32)dst_h;
    gNdsOriginalSpritePreviewCommitCount++;

#if NDS_RENDERER_HW_TRIANGLES
    {
        u32 layer = (is_foreground != FALSE) ? 1u : 0u;

        if ((sOriginalSpriteOverlayLayerMask & (1u << layer)) != 0u)
        {
            int bg = (is_foreground != FALSE) ?
                sOriginalSpriteOverlayForegroundBg : sOriginalSpriteOverlayBg;
            u16 *overlay;
            s32 y;

            if (bg < 0)
            {
                return;
            }
            overlay = (u16 *)bgGetGfxPtr(bg);
            /* A full-screen staging image already contains transparent zeroes.
             * Clearing visible VRAM before the row copy exposes black bands when
             * scanout catches the single-buffered overlay mid-commit. */
            if ((dst_w < SCREEN_WIDTH) || (dst_h < SCREEN_HEIGHT))
            {
                u32 clear_bytes =
                    ndsPlatformOriginalSpriteOverlayClearPixels() * sizeof(u16);

                dmaFillHalfWords(0, overlay, clear_bytes);
                if (is_foreground != FALSE)
                {
                    gNdsOriginalSpriteBg3ClearBytes += clear_bytes;
                }
                else
                {
                    gNdsOriginalSpriteBg2ClearBytes += clear_bytes;
                }
            }
            for (y = 0; y < dst_h; y++)
            {
                memcpy(&overlay[y * 256],
                       &display_preview[
                           y * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH],
                       (size_t)dst_w * sizeof(u16));
            }
            if (is_foreground != FALSE)
            {
                gNdsOriginalSpriteBg3CopyBytes +=
                    (u32)dst_w * (u32)dst_h * sizeof(u16);
                ndsPlatformAdvanceOriginalSpriteOverlayEpoch(1u);
            }
            else
            {
#if NDS_FAST_WALLPAPER_AFFINE
                if ((sFastWallpaper.state == NDS_FAST_WALLPAPER_READY) ||
                    (sFastWallpaper.state ==
                        NDS_FAST_WALLPAPER_STATIC_DEGRADED))
                {
                    gNdsFastWallpaperPostReadyPixelWriteCount +=
                        (u32)dst_w * (u32)dst_h;
                }
#endif
                gNdsOriginalSpriteBg2CopyBytes +=
                    (u32)dst_w * (u32)dst_h * sizeof(u16);
                ndsPlatformAdvanceOriginalSpriteOverlayEpoch(0u);
            }
        }
    }
#endif
}

void ndsPlatformCommitOriginalSpritePreview(void)
{
    ndsPlatformCommitOriginalSpritePreviewLayer(FALSE);
}

void ndsPlatformClearOriginalSpriteOverlayLayer(s32 is_foreground)
{
#if NDS_RENDERER_HW_TRIANGLES
    int bg = (is_foreground != FALSE) ?
        sOriginalSpriteOverlayForegroundBg : sOriginalSpriteOverlayBg;

    if (bg >= 0)
    {
#if NDS_FAST_WALLPAPER_AFFINE
        if (is_foreground == FALSE)
        {
            ndsPlatformFastWallpaperResetInternal();
        }
#endif
        u32 clear_bytes =
            ndsPlatformOriginalSpriteOverlayClearPixels() * sizeof(u16);

        dmaFillHalfWords(0, bgGetGfxPtr(bg), clear_bytes);
        if (is_foreground != FALSE)
        {
            gNdsOriginalSpriteBg3ClearBytes += clear_bytes;
            ndsPlatformAdvanceOriginalSpriteOverlayEpoch(1u);
        }
        else
        {
            gNdsOriginalSpriteBg2ClearBytes += clear_bytes;
            ndsPlatformAdvanceOriginalSpriteOverlayEpoch(0u);
        }
    }
#else
    (void)is_foreground;
#endif
}

void ndsPlatformSetOriginalSpriteOverlayLayerMask(u32 layer_mask)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 previous_mask = sOriginalSpriteOverlayLayerMask;

    layer_mask &= NDS_ORIGINAL_SPRITE_OVERLAY_ALL;
#if NDS_FAST_WALLPAPER_AFFINE
    if (((previous_mask ^ layer_mask) &
         NDS_ORIGINAL_SPRITE_OVERLAY_BACKGROUND) != 0u)
    {
        ndsPlatformFastWallpaperResetInternal();
    }
#endif
    if ((layer_mask != 0u) && (layer_mask != previous_mask))
    {
        sOriginalSpriteOverlayNeedsFlush = TRUE;
        /* Start a fresh traffic window for the scene that owns the overlay.
         * Initialization/previous-scene clears must not masquerade as live
         * compositor work in the canonical battle profile. */
        gNdsOriginalSpriteBg2ClearBytes = 0u;
        gNdsOriginalSpriteBg2CopyBytes = 0u;
        gNdsOriginalSpriteBg2FinalWriteBytes = 0u;
        gNdsOriginalSpriteBg3ClearBytes = 0u;
        gNdsOriginalSpriteBg3CopyBytes = 0u;
        gNdsOriginalSpriteBg3FinalWriteBytes = 0u;
    }
    sOriginalSpriteOverlayLayerMask = layer_mask;
    glClearColor(2, 3, 6, (layer_mask != 0u) ? 0 : 31);

    if (sOriginalSpriteOverlayBg >= 0)
    {
        if ((layer_mask & NDS_ORIGINAL_SPRITE_OVERLAY_BACKGROUND) != 0u)
        {
            bgShow(sOriginalSpriteOverlayBg);
        }
        else
        {
            if ((previous_mask & NDS_ORIGINAL_SPRITE_OVERLAY_BACKGROUND) != 0u)
            {
                ndsPlatformClearOriginalSpriteOverlayLayer(FALSE);
            }
            bgHide(sOriginalSpriteOverlayBg);
        }
    }
    if (sOriginalSpriteOverlayForegroundBg >= 0)
    {
        if ((layer_mask & NDS_ORIGINAL_SPRITE_OVERLAY_FOREGROUND) != 0u)
        {
            bgShow(sOriginalSpriteOverlayForegroundBg);
        }
        else
        {
            if ((previous_mask & NDS_ORIGINAL_SPRITE_OVERLAY_FOREGROUND) != 0u)
            {
                ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
            }
            bgHide(sOriginalSpriteOverlayForegroundBg);
        }
    }
#else
    (void)layer_mask;
#endif
}

void ndsPlatformSetOriginalSpriteOverlayEnabled(s32 is_enabled)
{
    ndsPlatformSetOriginalSpriteOverlayLayerMask(
        (is_enabled != FALSE) ? NDS_ORIGINAL_SPRITE_OVERLAY_ALL : 0u);
}

#if NDS_RENDERER_HW_TRIANGLES && NDS_FAST_WALLPAPER_AFFINE
#define NDS_FAST_WALLPAPER_PREFILL_COLOR (RGB15(8, 20, 27) | BIT(15))
#define NDS_FAST_WALLPAPER_SCROLL_QUANTUM_Q8 0x40

#if NDS_RENDERER_PROFILE_LEVEL >= 1
#define NDS_FAST_WALLPAPER_PROFILE_INC(value) ((value)++)
#else
#define NDS_FAST_WALLPAPER_PROFILE_INC(value) ((void)0)
#endif

static u32 ndsPlatformFastWallpaperAffineEqual(
    const NDSFastWallpaperAffine *a,
    const NDSFastWallpaperAffine *b)
{
    return ((a->hdx == b->hdx) && (a->vdy == b->vdy) &&
            (a->dx == b->dx) && (a->dy == b->dy)) ? TRUE : FALSE;
}

static u32 ndsPlatformFastWallpaperIsAdmitted(void)
{
    return ((sFastWallpaper.state == NDS_FAST_WALLPAPER_READY) ||
            (sFastWallpaper.state ==
                NDS_FAST_WALLPAPER_STATIC_DEGRADED)) ? TRUE : FALSE;
}

static void ndsPlatformFastWallpaperQueueAffine(
    const NDSFastWallpaperAffine *affine, u32 count_queue)
{
    if ((sFastWallpaper.pending_valid != FALSE) &&
        (ndsPlatformFastWallpaperAffineEqual(
            &sFastWallpaper.pending, affine) != FALSE))
    {
        if (count_queue != FALSE)
        {
            NDS_FAST_WALLPAPER_PROFILE_INC(
                gNdsFastWallpaperUnchangedSkipCount);
        }
        return;
    }
    if ((sFastWallpaper.pending_valid == FALSE) &&
        (sFastWallpaper.committed_valid != FALSE) &&
        (ndsPlatformFastWallpaperAffineEqual(
            &sFastWallpaper.committed, affine) != FALSE))
    {
        if (count_queue != FALSE)
        {
            NDS_FAST_WALLPAPER_PROFILE_INC(
                gNdsFastWallpaperUnchangedSkipCount);
        }
        return;
    }
    sFastWallpaper.pending = *affine;
    sFastWallpaper.pending_valid = TRUE;
    gNdsSceneWallpaperAffineHdx = affine->hdx;
    gNdsSceneWallpaperAffineVdy = affine->vdy;
    gNdsSceneWallpaperAffineDx = affine->dx;
    gNdsSceneWallpaperAffineDy = affine->dy;
    if (count_queue != FALSE)
    {
        NDS_FAST_WALLPAPER_PROFILE_INC(gNdsFastWallpaperQueueCount);
    }
}

static void ndsPlatformFastWallpaperQueueIdentity(void)
{
    const NDSFastWallpaperAffine identity = {
        1 << 8, 1 << 8, 0, 0
    };

    ndsPlatformFastWallpaperQueueAffine(&identity, FALSE);
}

static void ndsPlatformFastWallpaperResetInternal(void)
{
    if ((sFastWallpaper.state == NDS_FAST_WALLPAPER_UNSEEDED) &&
        (sFastWallpaper.pending_valid == FALSE) &&
        (sFastWallpaper.committed_valid != FALSE) &&
        (sFastWallpaper.committed.hdx == (1 << 8)) &&
        (sFastWallpaper.committed.vdy == (1 << 8)) &&
        (sFastWallpaper.committed.dx == 0) &&
        (sFastWallpaper.committed.dy == 0))
    {
        return;
    }
    sFastWallpaper.state = NDS_FAST_WALLPAPER_UNSEEDED;
    memset(&sFastWallpaper.seed, 0, sizeof(sFastWallpaper.seed));
    memset(&sFastWallpaper.latest, 0, sizeof(sFastWallpaper.latest));
    sFastWallpaper.latest_valid = FALSE;
    sFastWallpaper.overlay_generation = 0u;
    sFastWallpaper.asset_identity = 0u;
    gNdsFastWallpaperState = NDS_FAST_WALLPAPER_UNSEEDED;
    ndsPlatformFastWallpaperQueueIdentity();
}

u32 ndsPlatformFastWallpaperCanSeed(void)
{
    if ((ndsPlatformFastWallpaperIsAdmitted() != FALSE) &&
        (sFastWallpaper.overlay_generation !=
            sOriginalSpriteOverlayEpoch[0]))
    {
        ndsPlatformFastWallpaperResetInternal();
    }
    return ((sFastWallpaper.state == NDS_FAST_WALLPAPER_UNSEEDED) &&
            (sOriginalSpriteOverlayBg >= 0) &&
            ((sOriginalSpriteOverlayLayerMask &
                NDS_ORIGINAL_SPRITE_OVERLAY_BACKGROUND) != 0u) &&
            (bgGetGfxPtr(sOriginalSpriteOverlayBg) != NULL)) ? TRUE : FALSE;
}

u32 ndsPlatformFastWallpaperBeginSeed(s32 origin_x, s32 origin_y,
                                       u32 scale_x_q16,
                                       u32 scale_y_q16,
                                       u32 asset_identity)
{
    u16 *wallpaper;

    if (ndsPlatformFastWallpaperCanSeed() == FALSE)
    {
        return FALSE;
    }
    wallpaper = (u16 *)bgGetGfxPtr(sOriginalSpriteOverlayBg);
    if (wallpaper == NULL)
    {
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sFastWallpaper.seed_start_ticks = cpuGetTiming();
#endif
    sFastWallpaper.state = NDS_FAST_WALLPAPER_CAPTURING;
    sFastWallpaper.seed.origin_x = origin_x;
    sFastWallpaper.seed.origin_y = origin_y;
    sFastWallpaper.seed.scale_x_q16 = scale_x_q16;
    sFastWallpaper.seed.scale_y_q16 = scale_y_q16;
    sFastWallpaper.latest_valid = FALSE;
    sFastWallpaper.asset_identity = asset_identity;
    gNdsFastWallpaperState = NDS_FAST_WALLPAPER_CAPTURING;
    gNdsFastWallpaperSeedAttemptCount++;

    dmaFillHalfWords(NDS_FAST_WALLPAPER_PREFILL_COLOR, wallpaper,
                     SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
    sFastWallpaper.overlay_generation =
        ndsPlatformAdvanceOriginalSpriteOverlayEpoch(0u);
    return TRUE;
}

static u32 ndsPlatformFastWallpaperHashVisible(const u16 *pixels)
{
    u32 hash = 2166136261u;
    u32 y;

    for (y = 0u; y < SCREEN_HEIGHT; y++)
    {
        u32 x;

        for (x = 0u; x < SCREEN_WIDTH; x++)
        {
            u16 pixel = pixels[(y * 256u) + x];

            hash ^= pixel & 0xffu;
            hash *= 16777619u;
            hash ^= pixel >> 8;
            hash *= 16777619u;
        }
    }
    return hash;
}

u32 ndsPlatformFastWallpaperFinishSeed(u32 software_draw_succeeded)
{
    u16 *wallpaper;
    u32 source_opaque = 0u;
    u32 filled = 0u;
    u32 final_layer_committed;
    u32 valid;
    u32 y;

    if (sFastWallpaper.state != NDS_FAST_WALLPAPER_CAPTURING)
    {
        return FALSE;
    }
    wallpaper = (sOriginalSpriteOverlayBg >= 0) ?
        (u16 *)bgGetGfxPtr(sOriginalSpriteOverlayBg) : NULL;
    final_layer_committed =
        (sFastWallpaper.overlay_generation !=
            sOriginalSpriteOverlayEpoch[0]) ? TRUE : FALSE;
    if (wallpaper != NULL)
    {
        for (y = 0u; y < SCREEN_HEIGHT; y++)
        {
            u32 x;

            for (x = 0u; x < SCREEN_WIDTH; x++)
            {
                u16 *pixel = &wallpaper[(y * 256u) + x];

                if ((*pixel & BIT(15)) != 0u)
                {
                    source_opaque++;
                }
                else
                {
                    *pixel = NDS_FAST_WALLPAPER_PREFILL_COLOR;
                    filled++;
                }
            }
        }
        if (filled != 0u)
        {
            ndsPlatformAdvanceOriginalSpriteOverlayEpoch(0u);
        }
        gNdsFastWallpaperSeedHash =
            ndsPlatformFastWallpaperHashVisible(wallpaper);
    }
    gNdsFastWallpaperSeedOpaquePixelCount = source_opaque;
    sFastWallpaper.overlay_generation = sOriginalSpriteOverlayEpoch[0];
    valid = ((software_draw_succeeded != FALSE) &&
             (final_layer_committed != FALSE) &&
             (wallpaper != NULL) &&
             (source_opaque >=
                ((SCREEN_WIDTH * SCREEN_HEIGHT * 3u) / 4u)) &&
             (sFastWallpaper.seed.scale_x_q16 != 0u) &&
             (sFastWallpaper.seed.scale_y_q16 != 0u) &&
             (sFastWallpaper.asset_identity != 0u)) ?
        TRUE : FALSE;
    if (valid != FALSE)
    {
        sFastWallpaper.state = NDS_FAST_WALLPAPER_READY;
        gNdsFastWallpaperSeedSuccessCount++;
    }
    else
    {
        sFastWallpaper.state = NDS_FAST_WALLPAPER_STATIC_DEGRADED;
        gNdsFastWallpaperSeedFailureCount++;
        gNdsFastWallpaperStaticDegradedCount++;
    }
    gNdsFastWallpaperState = (u32)sFastWallpaper.state;
    ndsPlatformFastWallpaperQueueIdentity();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsFastWallpaperSeedTicks =
        cpuGetTiming() - sFastWallpaper.seed_start_ticks;
#endif
    return TRUE;
}

static s64 ndsPlatformFastWallpaperQ16ToQ8(s64 value_q16)
{
    if (value_q16 < 0)
    {
        return -(((-value_q16) + 0x80) >> 8);
    }
    return (value_q16 + 0x80) >> 8;
}

static s64 ndsPlatformFastWallpaperClampS64(s64 value,
                                             s64 minimum,
                                             s64 maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static void ndsPlatformFastWallpaperBuildAffine(
    const NDSFastWallpaperTransform *live,
    NDSFastWallpaperAffine *affine)
{
    const s64 preview_pixel_center_q16 = 0xa000;
    const s32 max_hdx =
        (((SCREEN_WIDTH << 8) - 1) / (SCREEN_WIDTH - 1));
    const s32 max_vdy =
        (((SCREEN_HEIGHT << 8) - 1) / (SCREEN_HEIGHT - 1));
    u64 ratio_x_q16;
    u64 ratio_y_q16;
    u64 raw_hdx;
    u64 raw_vdy;
    s64 offset_x_q16;
    s64 offset_y_q16;
    s64 raw_dx;
    s64 raw_dy;
    s64 dx;
    s64 dy;
    s64 max_dx;
    s64 max_dy;
    u32 scale_clamped = FALSE;

    ratio_x_q16 = ((((u64)sFastWallpaper.seed.scale_x_q16 << 16) +
                    (live->scale_x_q16 >> 1)) /
                   live->scale_x_q16);
    ratio_y_q16 = ((((u64)sFastWallpaper.seed.scale_y_q16 << 16) +
                    (live->scale_y_q16 >> 1)) /
                   live->scale_y_q16);
    raw_hdx = (ratio_x_q16 + 0x80u) >> 8;
    raw_vdy = (ratio_y_q16 + 0x80u) >> 8;
    affine->hdx = (s32)ndsPlatformFastWallpaperClampS64(
        (s64)raw_hdx, 1, max_hdx);
    affine->vdy = (s32)ndsPlatformFastWallpaperClampS64(
        (s64)raw_vdy, 1, max_vdy);
    if ((raw_hdx != (u64)affine->hdx) ||
        (raw_vdy != (u64)affine->vdy))
    {
        scale_clamped = TRUE;
    }
    if ((affine->hdx - affine->vdy <= 1) &&
        (affine->vdy - affine->hdx <= 1))
    {
        s32 uniform = (affine->hdx + affine->vdy + 1) >> 1;

        affine->hdx = uniform;
        affine->vdy = uniform;
    }
    if (scale_clamped != FALSE)
    {
        NDS_FAST_WALLPAPER_PROFILE_INC(
            gNdsFastWallpaperClampScaleCount);
    }

    ratio_x_q16 = (u64)affine->hdx << 8;
    ratio_y_q16 = (u64)affine->vdy << 8;
    offset_x_q16 = (((s64)ratio_x_q16 *
        (preview_pixel_center_q16 -
         ((s64)live->origin_x * 65536))) / 65536) +
        ((s64)sFastWallpaper.seed.origin_x * 65536) -
        preview_pixel_center_q16;
    offset_y_q16 = (((s64)ratio_y_q16 *
        (preview_pixel_center_q16 -
         ((s64)live->origin_y * 65536))) / 65536) +
        ((s64)sFastWallpaper.seed.origin_y * 65536) -
        preview_pixel_center_q16;
    offset_x_q16 = (offset_x_q16 * 4) / 5;
    offset_y_q16 = (offset_y_q16 * 4) / 5;
    raw_dx = ndsPlatformFastWallpaperQ16ToQ8(offset_x_q16);
    raw_dy = ndsPlatformFastWallpaperQ16ToQ8(offset_y_q16);
    max_dx = (((s64)SCREEN_WIDTH << 8) - 1) -
        ((s64)affine->hdx * (SCREEN_WIDTH - 1));
    max_dy = (((s64)SCREEN_HEIGHT << 8) - 1) -
        ((s64)affine->vdy * (SCREEN_HEIGHT - 1));
    dx = ndsPlatformFastWallpaperClampS64(raw_dx, 0, max_dx);
    dy = ndsPlatformFastWallpaperClampS64(raw_dy, 0, max_dy);
    if (dx != raw_dx)
    {
        NDS_FAST_WALLPAPER_PROFILE_INC(gNdsFastWallpaperClampXCount);
    }
    if (dy != raw_dy)
    {
        NDS_FAST_WALLPAPER_PROFILE_INC(gNdsFastWallpaperClampYCount);
    }
    dx = ((dx + (NDS_FAST_WALLPAPER_SCROLL_QUANTUM_Q8 / 2)) /
          NDS_FAST_WALLPAPER_SCROLL_QUANTUM_Q8) *
        NDS_FAST_WALLPAPER_SCROLL_QUANTUM_Q8;
    dy = ((dy + (NDS_FAST_WALLPAPER_SCROLL_QUANTUM_Q8 / 2)) /
          NDS_FAST_WALLPAPER_SCROLL_QUANTUM_Q8) *
        NDS_FAST_WALLPAPER_SCROLL_QUANTUM_Q8;
    affine->dx = (s32)ndsPlatformFastWallpaperClampS64(dx, 0, max_dx);
    affine->dy = (s32)ndsPlatformFastWallpaperClampS64(dy, 0, max_dy);
}

u32 ndsPlatformFastWallpaperQueueTransform(s32 origin_x, s32 origin_y,
                                            u32 scale_x_q16,
                                            u32 scale_y_q16,
                                            u32 asset_identity)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 profile_start = cpuGetTiming();
#endif
    NDSFastWallpaperTransform live;
    NDSFastWallpaperAffine affine;

    if (ndsPlatformFastWallpaperIsAdmitted() == FALSE)
    {
        return FALSE;
    }
    if ((sFastWallpaper.overlay_generation !=
            sOriginalSpriteOverlayEpoch[0]) ||
        (sFastWallpaper.asset_identity != asset_identity))
    {
        ndsPlatformFastWallpaperResetInternal();
        return FALSE;
    }
    if (sFastWallpaper.state == NDS_FAST_WALLPAPER_STATIC_DEGRADED)
    {
        NDS_FAST_WALLPAPER_PROFILE_INC(
            gNdsFastWallpaperUnchangedSkipCount);
        return TRUE;
    }
    if ((scale_x_q16 == 0u) || (scale_y_q16 == 0u))
    {
        NDS_FAST_WALLPAPER_PROFILE_INC(
            gNdsFastWallpaperInvalidTransformCount);
        NDS_FAST_WALLPAPER_PROFILE_INC(
            gNdsFastWallpaperReusePreviousCount);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsFastWallpaperAffineLastTicks =
            cpuGetTiming() - profile_start;
#endif
        return TRUE;
    }
    live.origin_x = origin_x;
    live.origin_y = origin_y;
    live.scale_x_q16 = scale_x_q16;
    live.scale_y_q16 = scale_y_q16;
    sFastWallpaper.latest = live;
    sFastWallpaper.latest_valid = TRUE;
    ndsPlatformFastWallpaperBuildAffine(&live, &affine);
    ndsPlatformFastWallpaperQueueAffine(&affine, TRUE);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsFastWallpaperAffineLastTicks =
        cpuGetTiming() - profile_start;
#endif
    return TRUE;
}

void ndsPlatformFastWallpaperRecordSoftwareDraw(void)
{
    if (ndsPlatformFastWallpaperIsAdmitted() != FALSE)
    {
        gNdsFastWallpaperPostReadySoftwareDrawCount++;
    }
}

void ndsPlatformFastWallpaperReset(void)
{
    ndsPlatformFastWallpaperResetInternal();
}

static void ndsPlatformFastWallpaperCommitAffine(void)
{
    if ((sOriginalSpriteOverlayBg < 0) ||
        (sFastWallpaper.pending_valid == FALSE))
    {
        return;
    }
    if ((sFastWallpaper.committed_valid != FALSE) &&
        (ndsPlatformFastWallpaperAffineEqual(
            &sFastWallpaper.pending,
            &sFastWallpaper.committed) != FALSE))
    {
        sFastWallpaper.pending_valid = FALSE;
        return;
    }
    bgSetAffineMatrixScroll(sOriginalSpriteOverlayBg,
                            sFastWallpaper.pending.hdx, 0, 0,
                            sFastWallpaper.pending.vdy,
                            sFastWallpaper.pending.dx,
                            sFastWallpaper.pending.dy);
    sFastWallpaper.committed = sFastWallpaper.pending;
    sFastWallpaper.committed_valid = TRUE;
    sFastWallpaper.pending_valid = FALSE;
    NDS_FAST_WALLPAPER_PROFILE_INC(gNdsFastWallpaperApplyCount);
}

#undef NDS_FAST_WALLPAPER_PROFILE_INC
#endif

#if !(NDS_RENDERER_HW_TRIANGLES && NDS_FAST_WALLPAPER_AFFINE)
u32 ndsPlatformFastWallpaperCanSeed(void)
{
    return FALSE;
}

u32 ndsPlatformFastWallpaperBeginSeed(s32 origin_x, s32 origin_y,
                                       u32 scale_x_q16,
                                       u32 scale_y_q16,
                                       u32 asset_identity)
{
    (void)origin_x;
    (void)origin_y;
    (void)scale_x_q16;
    (void)scale_y_q16;
    (void)asset_identity;
    return FALSE;
}

u32 ndsPlatformFastWallpaperFinishSeed(u32 software_draw_succeeded)
{
    (void)software_draw_succeeded;
    return FALSE;
}

u32 ndsPlatformFastWallpaperQueueTransform(s32 origin_x, s32 origin_y,
                                            u32 scale_x_q16,
                                            u32 scale_y_q16,
                                            u32 asset_identity)
{
    (void)origin_x;
    (void)origin_y;
    (void)scale_x_q16;
    (void)scale_y_q16;
    (void)asset_identity;
    return FALSE;
}

void ndsPlatformFastWallpaperRecordSoftwareDraw(void)
{
}

void ndsPlatformFastWallpaperReset(void)
{
}
#endif

#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
static s32 ndsPlatformSceneWallpaperQ16ToQ8(s64 value_q16)
{
    if (value_q16 < 0)
    {
        return -(s32)(((-value_q16) + 0x80) >> 8);
    }
    return (s32)((value_q16 + 0x80) >> 8);
}

static void ndsPlatformSceneWallpaperQueueIdentity(void)
{
    sSceneWallpaperPendingHdx = 1 << 8;
    sSceneWallpaperPendingVdy = 1 << 8;
    sSceneWallpaperPendingDx = 0;
    sSceneWallpaperPendingDy = 0;
    sSceneWallpaperAffinePending = TRUE;
}

static u32 ndsPlatformSceneWallpaperBuildAffine(
    const NDSSceneWallpaperTransform *live)
{
    const s64 preview_pixel_center_q16 = 0xa000;
    u32 ratio_x_q16;
    u32 ratio_y_q16;
    s64 offset_x_q16;
    s64 offset_y_q16;
    s64 source_x_end_q8;
    s64 source_y_end_q8;

    if ((live == NULL) ||
        (live->scale_x_q16 == 0u) || (live->scale_y_q16 == 0u) ||
        (sSceneWallpaperSeedTransform.scale_x_q16 == 0u) ||
        (sSceneWallpaperSeedTransform.scale_y_q16 == 0u))
    {
        return FALSE;
    }
    ratio_x_q16 = (u32)((
        ((u64)sSceneWallpaperSeedTransform.scale_x_q16 << 16) +
        (live->scale_x_q16 >> 1)) / live->scale_x_q16);
    ratio_y_q16 = (u32)((
        ((u64)sSceneWallpaperSeedTransform.scale_y_q16 << 16) +
        (live->scale_y_q16 >> 1)) / live->scale_y_q16);
    sSceneWallpaperPendingHdx = (s32)((ratio_x_q16 + 0x80u) >> 8);
    sSceneWallpaperPendingVdy = (s32)((ratio_y_q16 + 0x80u) >> 8);
    if ((sSceneWallpaperPendingHdx <= 0) ||
        (sSceneWallpaperPendingVdy <= 0) ||
        (sSceneWallpaperPendingHdx > 0x7fff) ||
        (sSceneWallpaperPendingVdy > 0x7fff))
    {
        return FALSE;
    }

    /* The software compositor samples the 320x240 preview at pixel centers
     * while reducing it to BG2's 256x192 window. Solve the same mapping for
     * the retained seed image, including that 0.625-preview-pixel center. */
    offset_x_q16 = (((s64)ratio_x_q16 *
        (preview_pixel_center_q16 -
         ((s64)live->origin_x << 16))) / 65536) +
        ((s64)sSceneWallpaperSeedTransform.origin_x << 16) -
        preview_pixel_center_q16;
    offset_y_q16 = (((s64)ratio_y_q16 *
        (preview_pixel_center_q16 -
         ((s64)live->origin_y << 16))) / 65536) +
        ((s64)sSceneWallpaperSeedTransform.origin_y << 16) -
        preview_pixel_center_q16;
    offset_x_q16 = (offset_x_q16 * 4) / 5;
    offset_y_q16 = (offset_y_q16 * 4) / 5;
    sSceneWallpaperPendingDx =
        ndsPlatformSceneWallpaperQ16ToQ8(offset_x_q16);
    sSceneWallpaperPendingDy =
        ndsPlatformSceneWallpaperQ16ToQ8(offset_y_q16);

    source_x_end_q8 = (s64)sSceneWallpaperPendingDx +
        ((s64)sSceneWallpaperPendingHdx * (SCREEN_WIDTH - 1));
    source_y_end_q8 = (s64)sSceneWallpaperPendingDy +
        ((s64)sSceneWallpaperPendingVdy * (SCREEN_HEIGHT - 1));
    if ((sSceneWallpaperPendingDx < 0) ||
        (sSceneWallpaperPendingDy < 0) ||
        (source_x_end_q8 >= ((s64)SCREEN_WIDTH << 8)) ||
        (source_y_end_q8 >= ((s64)SCREEN_HEIGHT << 8)))
    {
        return FALSE;
    }
    sSceneWallpaperAffinePending = TRUE;
    gNdsSceneWallpaperAffineHdx = sSceneWallpaperPendingHdx;
    gNdsSceneWallpaperAffineVdy = sSceneWallpaperPendingVdy;
    gNdsSceneWallpaperAffineDx = sSceneWallpaperPendingDx;
    gNdsSceneWallpaperAffineDy = sSceneWallpaperPendingDy;
    return TRUE;
}

static void ndsPlatformSceneWallpaperCommitAffine(void)
{
    if (sOriginalSpriteOverlayBg < 0)
    {
        return;
    }
    if (sSceneWallpaperAffineResetPending != FALSE)
    {
        bgSetAffineMatrixScroll(sOriginalSpriteOverlayBg,
                                1 << 8, 0, 0, 1 << 8, 0, 0);
        sSceneWallpaperAffineResetPending = FALSE;
        sSceneWallpaperAffinePending = FALSE;
        return;
    }
    if (sSceneWallpaperAffinePending == FALSE)
    {
        return;
    }
    bgSetAffineMatrixScroll(sOriginalSpriteOverlayBg,
                            sSceneWallpaperPendingHdx, 0, 0,
                            sSceneWallpaperPendingVdy,
                            sSceneWallpaperPendingDx,
                            sSceneWallpaperPendingDy);
    sSceneWallpaperAffinePending = FALSE;
    gNdsSceneWallpaperAffineApplyCount++;
}
#endif

u32 ndsPlatformSceneWallpaperQueueTransform(s32 origin_x, s32 origin_y,
                                             u32 scale_x_q16,
                                             u32 scale_y_q16)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
    u32 profile_start = cpuGetTiming();

    if (sSceneMipCacheFailed != FALSE)
    {
        gNdsSceneWallpaperAffineLastTicks =
            cpuGetTiming() - profile_start;
        return FALSE;
    }
    sSceneWallpaperLatestTransform.origin_x = origin_x;
    sSceneWallpaperLatestTransform.origin_y = origin_y;
    sSceneWallpaperLatestTransform.scale_x_q16 = scale_x_q16;
    sSceneWallpaperLatestTransform.scale_y_q16 = scale_y_q16;
    sSceneWallpaperLatestTransformValid =
        ((scale_x_q16 != 0u) && (scale_y_q16 != 0u)) ? TRUE : FALSE;
    if (sSceneMipCacheReady == FALSE)
    {
        gNdsSceneWallpaperAffineLastTicks =
            cpuGetTiming() - profile_start;
        return FALSE;
    }
    if (ndsPlatformSceneWallpaperBuildAffine(
            &sSceneWallpaperLatestTransform) == FALSE)
    {
        gNdsSceneWallpaperAffineCoverageFailureCount++;
        ndsPlatformSceneMipCacheAbort();
        gNdsSceneWallpaperAffineLastTicks =
            cpuGetTiming() - profile_start;
        return FALSE;
    }
    gNdsSceneWallpaperAffineQueueCount++;
    gNdsSceneWallpaperAffineLastTicks = cpuGetTiming() - profile_start;
    return TRUE;
#else
    (void)origin_x;
    (void)origin_y;
    (void)scale_x_q16;
    (void)scale_y_q16;
    return FALSE;
#endif
}

void ndsPlatformSceneWallpaperConfirmRaster(void)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
    if ((sSceneMipCapturePending != 0u) &&
        (sSceneMipCacheReady == FALSE) &&
        (sSceneMipCacheFailed == FALSE))
    {
        sSceneWallpaperSeedRasterCommitted = TRUE;
    }
#endif
}

#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
static void ndsPlatformSceneMipPublishWallpaperHash(const u16 *pixels)
{
    u32 hash = 2166136261u;
    u32 nonzero = 0u;
    u32 y;

    for (y = 0u; y < SCREEN_HEIGHT; y++)
    {
        u32 x;

        for (x = 0u; x < SCREEN_WIDTH; x++)
        {
            u16 pixel = pixels[(y * 256u) + x];

            hash ^= pixel & 0xffu;
            hash *= 16777619u;
            hash ^= pixel >> 8;
            hash *= 16777619u;
            if ((pixel & BIT(15)) != 0u)
            {
                nonzero++;
            }
        }
    }
    gNdsSceneMipCacheLastHash = hash;
    gNdsSceneMipCacheLastNonzeroPixels = nonzero;
}

static void ndsPlatformSceneMipFinishCapture(void)
{
    const u16 *wallpaper = (sOriginalSpriteOverlayBg >= 0) ?
        (const u16 *)bgGetGfxPtr(sOriginalSpriteOverlayBg) : NULL;

    if ((sSceneMipCapturePending != 1u) || (wallpaper == NULL) ||
        (sSceneWallpaperLatestTransformValid == FALSE) ||
        (sSceneWallpaperSeedRasterCommitted == FALSE))
    {
        sSceneMipCacheFailed = TRUE;
        gNdsSceneMipCacheFailureCount++;
        gNdsSceneMipCacheState = 3u;
        sSceneMipCapturePending = 0u;
        return;
    }
    /* Cut G freezes only BattleShip's already-composited BG2 wallpaper. The
     * source stage and fighters continue through the live GX display graph,
     * preserving the depth and parallax lost by a flattened scene texture. */
    ndsPlatformSceneMipPublishWallpaperHash(wallpaper);
    if (gNdsSceneMipCacheLastNonzeroPixels <
        ((SCREEN_WIDTH * SCREEN_HEIGHT * 3u) / 4u))
    {
        sSceneMipCacheFailed = TRUE;
        gNdsSceneMipCacheFailureCount++;
        gNdsSceneMipCacheState = 3u;
        sSceneMipCapturePending = 0u;
        return;
    }
    sSceneWallpaperSeedTransform = sSceneWallpaperLatestTransform;
    sSceneMipCaptureCompleted = 1u;
    sSceneMipCapturePending = 0u;
    gNdsSceneMipCacheCaptureCount = 1u;
    gNdsSceneMipCacheUploadCount = 0u;
    sSceneMipCacheReady = TRUE;
    gNdsSceneMipCacheState = 2u;
    ndsPlatformSceneWallpaperQueueIdentity();
    ndsPlatformSetOriginalSpriteOverlayLayerMask(
        NDS_ORIGINAL_SPRITE_OVERLAY_ALL);
}
#endif

u32 ndsPlatformSceneMipCaptureRequest(u32 mip_index)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
    if ((sSceneMipCacheReady != FALSE) ||
        (sSceneMipCacheFailed != FALSE))
    {
        return FALSE;
    }
    if ((sSceneMipCapturePending != 0u) ||
        (mip_index != 0u) ||
        (mip_index != sSceneMipCaptureCompleted))
    {
        sSceneMipCacheFailed = TRUE;
        gNdsSceneMipCacheFailureCount++;
        gNdsSceneMipCacheState = 3u;
        return FALSE;
    }
    sSceneMipCapturePending = mip_index + 1u;
    sSceneWallpaperLatestTransformValid = FALSE;
    sSceneWallpaperSeedRasterCommitted = FALSE;
    gNdsSceneMipCacheState = 1u;
    return TRUE;
#else
    (void)mip_index;
    return FALSE;
#endif
}

void ndsPlatformSceneMipCacheAbort(void)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
    if (sSceneMipCacheFailed == FALSE)
    {
        sSceneMipCacheFailed = TRUE;
        gNdsSceneMipCacheFailureCount++;
        gNdsSceneMipCacheState = 3u;
    }
    sSceneMipCacheReady = FALSE;
    sSceneMipCapturePending = 0u;
    sSceneWallpaperAffinePending = FALSE;
    sSceneWallpaperAffineResetPending = TRUE;
#endif
}

u32 ndsPlatformSceneMipCaptureCompletedCount(void)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
    return sSceneMipCaptureCompleted;
#else
    return 0u;
#endif
}

u32 ndsPlatformSceneMipCacheReady(void)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
    return sSceneMipCacheReady;
#else
    return FALSE;
#endif
}

u32 ndsPlatformSceneMipCacheFailed(void)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
    return sSceneMipCacheFailed;
#else
    return FALSE;
#endif
}

void ndsPlatformClearOriginalSpritePreview(void)
{
    sOriginalSpritePreviewWidth = 0;
    sOriginalSpritePreviewHeight = 0;
    sOriginalSpritePreviewX = 0;
    sOriginalSpritePreviewY = 0;
    sOriginalSpritePreviewReady = 0;
    sOriginalSpriteDisplayPreviewWidth = 0;
    sOriginalSpriteDisplayPreviewHeight = 0;
    gNdsOriginalSpritePreviewReady = 0;
    memset(sOriginalSpritePreview, 0, sizeof(sOriginalSpritePreview));
    memset(sOriginalSpriteDisplayPreview, 0,
           sizeof(sOriginalSpriteDisplayPreview));
    sOriginalSpriteDecodeCacheEpoch++;
    if (sOriginalSpriteDecodeCacheEpoch == 0u)
    {
        sOriginalSpriteDecodeCacheEpoch = 1u;
    }
}

u16 *ndsPlatformBeginOriginalDLPreview(u32 width, u32 height, u32 *out_pitch)
{
    if ((width == 0) || (height == 0) ||
        (width > NDS_ORIGINAL_DL_PREVIEW_MAX_WIDTH) ||
        (height > NDS_ORIGINAL_DL_PREVIEW_MAX_HEIGHT))
    {
        return NULL;
    }

    sOriginalDLPreviewWidth = width;
    sOriginalDLPreviewHeight = height;
    sOriginalDLDisplayPreviewWidth = 0;
    sOriginalDLDisplayPreviewHeight = 0;
    sOriginalDLPreviewReady = 0;
    gNdsOriginalDLPreviewReady = 0;
    gNdsOriginalDLPreviewWidth = width;
    gNdsOriginalDLPreviewHeight = height;
    memset(sOriginalDLPreview, 0, sizeof(sOriginalDLPreview));

    if (out_pitch != NULL)
    {
        *out_pitch = NDS_ORIGINAL_DL_PREVIEW_MAX_WIDTH;
    }
    return sOriginalDLPreview;
}

static void ndsPlatformBuildOriginalDLDisplayPreview(void)
{
    u32 dst_w = sOriginalDLPreviewWidth;
    u32 dst_h = sOriginalDLPreviewHeight;
    u32 y;

    if (dst_w > NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH)
    {
        dst_w = NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH;
    }
    if (dst_h > NDS_ORIGINAL_DL_PREVIEW_DISPLAY_HEIGHT)
    {
        dst_h = NDS_ORIGINAL_DL_PREVIEW_DISPLAY_HEIGHT;
    }

    for (y = 0; y < dst_h; y++)
    {
        u32 src_y = (y * sOriginalDLPreviewHeight) / dst_h;
        u16 *dst = &sOriginalDLDisplayPreview[
            y * NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH];
        u32 x;

        for (x = 0; x < dst_w; x++)
        {
            u32 src_x = (x * sOriginalDLPreviewWidth) / dst_w;
            u16 color = sOriginalDLPreview[
                (src_y * NDS_ORIGINAL_DL_PREVIEW_MAX_WIDTH) + src_x];

            dst[x] = (color != 0) ?
                color :
                (u16)(NDS_ORIGINAL_DL_PREVIEW_BG_COLOR | BIT(15));
        }
    }

    sOriginalDLDisplayPreviewWidth = dst_w;
    sOriginalDLDisplayPreviewHeight = dst_h;
}

void ndsPlatformCommitOriginalDLPreview(void)
{
    if ((sOriginalDLPreviewWidth != 0) && (sOriginalDLPreviewHeight != 0))
    {
        ndsPlatformBuildOriginalDLDisplayPreview();
        sOriginalDLPreviewReady = 1;
        gNdsOriginalDLPreviewReady = 1;
        gNdsOriginalDLPreviewCommitCount++;
    }
}

void ndsPlatformClearOriginalDLPreview(void)
{
    sOriginalDLPreviewWidth = 0;
    sOriginalDLPreviewHeight = 0;
    sOriginalDLDisplayPreviewWidth = 0;
    sOriginalDLDisplayPreviewHeight = 0;
    sOriginalDLPreviewReady = 0;
    gNdsOriginalDLPreviewReady = 0;
    gNdsOriginalDLPreviewWidth = 0;
    gNdsOriginalDLPreviewHeight = 0;
    memset(sOriginalDLPreview, 0, sizeof(sOriginalDLPreview));
}

#if !NDS_RENDERER_HW_TRIANGLES
static void ndsPlatformDrawOriginalSpritePreview(void)
{
    s32 dst_x;
    s32 dst_y;
    s32 dst_w;
    s32 dst_h;
    s32 y;

    if ((sOriginalSpritePreviewReady == 0) ||
        (sOriginalSpritePreviewWidth == 0) ||
        (sOriginalSpritePreviewHeight == 0))
    {
        return;
    }

    dst_x = (sOriginalSpritePreviewX * SCREEN_WIDTH) / NDS_N64_LOGICAL_WIDTH;
    dst_y = (sOriginalSpritePreviewY * SCREEN_HEIGHT) / NDS_N64_LOGICAL_HEIGHT;
    dst_w = (s32)sOriginalSpriteDisplayPreviewWidth;
    dst_h = (s32)sOriginalSpriteDisplayPreviewHeight;

    if (dst_w <= 0) dst_w = 1;
    if (dst_h <= 0) dst_h = 1;
    if ((dst_x + dst_w) > SCREEN_WIDTH)
    {
        dst_x = SCREEN_WIDTH - dst_w;
    }
    if ((dst_y + dst_h) > SCREEN_HEIGHT)
    {
        dst_y = SCREEN_HEIGHT - dst_h;
    }
    if (dst_x < 0) dst_x = 0;
    if (dst_y < 0) dst_y = 0;

    gNdsOriginalSpritePreviewDrawCount++;
    for (y = 0; y < dst_h; y++)
    {
        s32 screen_y = dst_y + y;
        s32 x;

        if ((screen_y < 0) || (screen_y >= SCREEN_HEIGHT))
        {
            continue;
        }

        for (x = 0; x < dst_w; x++)
        {
            s32 screen_x = dst_x + x;
            u16 color;

            if ((screen_x < 0) || (screen_x >= SCREEN_WIDTH))
            {
                continue;
            }

            color = sOriginalSpriteDisplayPreview[
                (y * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH) + x];
            if (color != 0)
            {
                sFramebuffer[(screen_y * SCREEN_WIDTH) + screen_x] = color;
            }
        }
    }
}

static void ndsPlatformDrawOriginalDLPreview(void)
{
    const s32 dst_x = NDS_ORIGINAL_DL_PREVIEW_DISPLAY_X;
    const s32 dst_y = NDS_ORIGINAL_DL_PREVIEW_DISPLAY_Y;
    u32 dst_w;
    u32 dst_h;
    s32 y;

    if ((gNdsOriginalDLPreviewReady == 0) ||
        (gNdsOriginalDLPreviewWidth == 0) ||
        (gNdsOriginalDLPreviewHeight == 0) ||
        (sOriginalDLDisplayPreviewWidth == 0) ||
        (sOriginalDLDisplayPreviewHeight == 0))
    {
        return;
    }

    dst_w = sOriginalDLDisplayPreviewWidth;
    dst_h = sOriginalDLDisplayPreviewHeight;

    gNdsOriginalDLPreviewDrawCount++;
    ndsPlatformDrawRect(dst_x - 2, dst_y - 2,
                        (s32)dst_w + 4,
                        (s32)dst_h + 4,
                        NDS_ORIGINAL_DL_PREVIEW_BORDER_COLOR);
    ndsPlatformDrawRect(dst_x - 1, dst_y - 1,
                        (s32)dst_w + 2,
                        (s32)dst_h + 2,
                        NDS_ORIGINAL_DL_PREVIEW_BG_COLOR);

    for (y = 0; y < (s32)dst_h; y++)
    {
        memcpy(&sFramebuffer[((dst_y + y) * SCREEN_WIDTH) + dst_x],
               &sOriginalDLDisplayPreview[
                   (u32)y * NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH],
               dst_w * sizeof(sOriginalDLDisplayPreview[0]));
    }
}
#endif

static void ndsPlatformPrintDebugLine(u32 row, const char *format, ...)
{
    char line[32];
    va_list args;

    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    line[sizeof(line) - 1] = '\0';
    iprintf("\x1b[%lu;0H%-31s", (unsigned long)row, line);
}

#if NDS_BATTLE_FPS_HUD_ENABLED
#if NDS_BATTLE_PHASE_HUD_ENABLED
static u64 sBattlePhaseHudUpdTickSum;
static u64 sBattlePhaseHudDrawTickSum;
static u64 sBattlePhaseHudActiveTickSum;
static u64 sBattlePhaseHudLoopTickSum;
static u32 sBattlePhaseHudAvgSampleCount;
#endif

static void ndsPlatformRenderBattleFpsHud(void)
{
    u32 now_tick = cpuGetTiming();
    u32 presented_frames = gNdsBattlePlayablePacingPresentedFrames;
    u32 logic_frames = gNdsBattlePlayablePacingLogicFrames;
    u32 elapsed_ticks;
    u32 elapsed_frames;
    u32 elapsed_logic_frames;
    u32 fps_x10;
    u32 updates_x10;

    if ((sBattleFpsHudSampleReady == 0u) ||
        (presented_frames < sBattleFpsHudLastPresentedFrames))
    {
        sBattleFpsHudSampleReady = 1u;
        sBattleFpsHudLastTick = now_tick;
        sBattleFpsHudLastPresentedFrames = presented_frames;
        sBattleFpsHudLastLogicFrames = logic_frames;
        sBattleFpsHudPrintedFpsX10 = 0xffffffffu;
        sBattleFpsHudPrintedUpdatesX10 = 0xffffffffu;
        sBattleTextHudReady = FALSE;
        sBattleTextHudFingerprint = 0xffffffffu;
        gNdsBattlePlayableHudFpsX10 = 0u;
        gNdsBattlePlayableHudFpsSampleCount = 0u;
        gNdsBattlePlayableHudFpsFrameWindow = 0u;
        gNdsBattlePlayableHudFpsTickWindow = 0u;
        consoleClear();
        ndsPlatformPrintDebugLine(0u, "FPS --.-  UP --.-");
#if NDS_BATTLE_PHASE_HUD_ENABLED
        sBattlePhaseHudLastSlipCount =
            gNdsBattlePlayablePacingCadenceViolationCount;
        sBattlePhaseHudUpdTickSum = 0u;
        sBattlePhaseHudDrawTickSum = 0u;
        sBattlePhaseHudActiveTickSum = 0u;
        sBattlePhaseHudLoopTickSum = 0u;
        sBattlePhaseHudAvgSampleCount = 0u;
        ndsPlatformPrintDebugLine(12u, "UPD        --");
        ndsPlatformPrintDebugLine(13u, "DRW        --");
        ndsPlatformPrintDebugLine(14u, "ACT        --");
        ndsPlatformPrintDebugLine(15u, "LOOP       --");
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && NDS_RENDERER_M3_PHASE0_PROFILE
        ndsPlatformPrintDebugLine(16u, "PRE        --");
        ndsPlatformPrintDebugLine(17u, "PRP        --");
        ndsPlatformPrintDebugLine(18u, "CMT        --");
#endif
        ndsPlatformPrintDebugLine(19u, "SLIP        0");
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        ndsPlatformPrintDebugLine(20u, "BGM slices       0");
#endif
        ndsPlatformPrintDebugLine(21u, "VBI  --  --  --");
        ndsPlatformPrintDebugLine(22u, "5+  --  max --  BGM --/--");
        ndsPlatformPrintDebugLine(23u, "GIT %s", NDS_TASK10_GIT_SHORT);
#endif
        return;
    }

    elapsed_ticks = now_tick - sBattleFpsHudLastTick;
    if (elapsed_ticks < NDS_BATTLE_FPS_HUD_SAMPLE_TICKS)
    {
        return;
    }

    elapsed_frames =
        presented_frames - sBattleFpsHudLastPresentedFrames;
    elapsed_logic_frames = logic_frames - sBattleFpsHudLastLogicFrames;
    fps_x10 = (elapsed_frames == 0u) ? 0u :
        (u32)((((u64)elapsed_frames * BUS_CLOCK * 10u) +
               (elapsed_ticks / 2u)) / elapsed_ticks);
    updates_x10 = (elapsed_logic_frames == 0u) ? 0u :
        (u32)((((u64)elapsed_logic_frames * BUS_CLOCK * 10u) +
               (elapsed_ticks / 2u)) / elapsed_ticks);
    gNdsBattlePlayableHudFpsX10 = fps_x10;
    gNdsBattlePlayableHudFpsSampleCount++;
    gNdsBattlePlayableHudFpsFrameWindow = elapsed_frames;
    gNdsBattlePlayableHudFpsTickWindow = elapsed_ticks;
    sBattleFpsHudLastTick = now_tick;
    sBattleFpsHudLastPresentedFrames = presented_frames;
    sBattleFpsHudLastLogicFrames = logic_frames;

#if NDS_BATTLE_PHASE_HUD_ENABLED
    /* Left column = latest snapshot; right column = running mean of these
     * once-per-sample-window snapshots since HUD reset (not per-frame). */
    {
        u32 upd_ticks = gNdsRendererProfileSourceUpdateTicks;
        u32 drw_ticks = gNdsRendererProfileDrawTicks;
        u32 act_ticks = gNdsRendererProfilePresentActiveTicks;
        u32 loop_ticks = gNdsRendererProfileLoopWallTicks;
        u32 avg_count;

        sBattlePhaseHudUpdTickSum += upd_ticks;
        sBattlePhaseHudDrawTickSum += drw_ticks;
        sBattlePhaseHudActiveTickSum += act_ticks;
        sBattlePhaseHudLoopTickSum += loop_ticks;
        sBattlePhaseHudAvgSampleCount++;
        avg_count = sBattlePhaseHudAvgSampleCount;

        ndsPlatformPrintDebugLine(
            12u, "UPD %8lu %8lu", (unsigned long)upd_ticks,
            (unsigned long)(sBattlePhaseHudUpdTickSum / avg_count));
        ndsPlatformPrintDebugLine(
            13u, "DRW %8lu %8lu", (unsigned long)drw_ticks,
            (unsigned long)(sBattlePhaseHudDrawTickSum / avg_count));
        ndsPlatformPrintDebugLine(
            14u, "ACT %8lu %8lu", (unsigned long)act_ticks,
            (unsigned long)(sBattlePhaseHudActiveTickSum / avg_count));
        ndsPlatformPrintDebugLine(
            15u, "LOOP%8lu %8lu", (unsigned long)loop_ticks,
            (unsigned long)(sBattlePhaseHudLoopTickSum / avg_count));
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        ndsPlatformPrintDebugLine(
            20u, "BGM slices %7lu",
            (unsigned long)gNdsAudioBgmSliceCount);
#elif NDS_FAST_WALLPAPER_AFFINE
        /* Engagement proof for the affine wallpaper: applies must climb and
         * post-ready pixel writes must stay near zero, ON DEVICE, or the
         * feature is not actually running there. */
        {
            ndsPlatformPrintDebugLine(
                20u, "WLP %8lu %8lu",
                (unsigned long)gNdsFastWallpaperApplyCount,
                (unsigned long)gNdsFastWallpaperPostReadyPixelWriteCount);
        }
#endif
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && NDS_RENDERER_M3_PHASE0_PROFILE
    ndsPlatformPrintDebugLine(
        16u, "PRE %10lu",
        (unsigned long)gNdsRendererM3Phase0PreflightTicks);
    ndsPlatformPrintDebugLine(
        17u, "PRP %10lu",
        (unsigned long)gNdsRendererM3Phase0PrepareRunTicks);
    ndsPlatformPrintDebugLine(
        18u, "CMT %10lu",
        (unsigned long)gNdsRendererM3Phase0CommitTicks);
#endif
    ndsPlatformPrintDebugLine(
        19u, "SLIP%10lu",
        (unsigned long)(gNdsBattlePlayablePacingCadenceViolationCount -
                        sBattlePhaseHudLastSlipCount));
    sBattlePhaseHudLastSlipCount =
        gNdsBattlePlayablePacingCadenceViolationCount;
    /* Presentation-interval histogram, cumulative since HUD reset. Device A/B
     * reports read this, never min FPS, because one frame crossing the 4->5
     * VBlank boundary reads as 12 FPS while the histogram stays continuous. */
    ndsPlatformPrintDebugLine(
        21u, "VBI 2:%-5lu 3:%-5lu 4:%-5lu",
        (unsigned long)gNdsBattlePlayablePacingPresentIntervalBucket[2u],
        (unsigned long)gNdsBattlePlayablePacingPresentIntervalBucket[3u],
        (unsigned long)gNdsBattlePlayablePacingPresentIntervalBucket[4u]);
    ndsPlatformPrintDebugLine(
        22u, "5+:%-5lu max:%lu BGM %lu/%lu%s",
        (unsigned long)gNdsBattlePlayablePacingPresentIntervalBucket[
            NDS_BATTLE_PLAYABLE_PACING_INTERVAL_BUCKET_5PLUS],
        (unsigned long)gNdsBattlePlayablePacingPresentIntervalMax,
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        (unsigned long)gNdsAudioBgmRefillTicksLast,
        (unsigned long)gNdsAudioBgmRefillTicksMax,
#else
        0ul,
        0ul,
#endif
        (gNdsAudioBgmFalsifierOff != 0u) ? " [OFF]" : "");
#endif

    /* sm64-nds also dedicates the lower console to FPS. Keep this port's
     * update change-driven so the counter does not clear or pulse the screen. */
    if ((fps_x10 != sBattleFpsHudPrintedFpsX10) ||
        (updates_x10 != sBattleFpsHudPrintedUpdatesX10))
    {
        sBattleFpsHudPrintedFpsX10 = fps_x10;
        sBattleFpsHudPrintedUpdatesX10 = updates_x10;
        ndsPlatformPrintDebugLine(0u, "FPS %lu.%lu  UP %lu.%lu",
                                  (unsigned long)(fps_x10 / 10u),
                                   (unsigned long)(fps_x10 % 10u),
                                   (unsigned long)(updates_x10 / 10u),
                                   (unsigned long)(updates_x10 % 10u));
    }
}
#endif

static u32 ndsPlatformMixDebugValue(u32 hash, u32 value)
{
    hash ^= value;
    return hash * 16777619u;
}

#if NDS_BATTLE_FPS_HUD_ENABLED
static const char *ndsPlatformBattleHudFighterName(u32 fighter_kind)
{
    static const char *const names[] = {
        "MARIO", "FOX", "DK", "SAMUS", "LUIGI", "LINK",
        "YOSHI", "CAPTAIN", "KIRBY", "PIKACHU", "JIGGLYPUFF",
        "NESS"
    };

    return (fighter_kind < (sizeof(names) / sizeof(names[0]))) ?
        names[fighter_kind] : "FIGHTER";
}

static u32 ndsPlatformBattleHudDisplayDamage(u32 damage)
{
    return (damage > 999u) ? 999u : damage;
}

static u32 ndsPlatformBattleHudDisplaySeconds(void)
{
    u32 remain = gNdsIFCommonHUDTimeRemain;
    u32 display_ticks;

    if ((gNdsIFCommonHUDRecordCount == 0u) || (remain == 0u))
    {
        return 0u;
    }
    display_ticks = remain;
    if ((remain != gNdsIFCommonHUDTimerLimit) &&
        (remain <= (0xffffffffu -
                    (NDS_BATTLE_SOURCE_TICKS_PER_SECOND - 1u))))
    {
        /* Match ifCommonTimerProcDisplay's source ceil-to-second rule. */
        display_ticks += NDS_BATTLE_SOURCE_TICKS_PER_SECOND - 1u;
    }
    return display_ticks / NDS_BATTLE_SOURCE_TICKS_PER_SECOND;
}

static u32 ndsPlatformBattleTextHudStateFingerprint(void)
{
    u32 hash = 2166136261u;

    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDRecordCount != 0u);
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDActivePlayerMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDShowDamageMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDCPUPlayerMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDP0FighterKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDP1FighterKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDP0Level);
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDP1Level);
    hash = ndsPlatformMixDebugValue(
        hash, ndsPlatformBattleHudDisplayDamage(
                  gNdsIFCommonHUDP0DamageCurrent));
    hash = ndsPlatformMixDebugValue(
        hash, ndsPlatformBattleHudDisplayDamage(
                  gNdsIFCommonHUDP1DamageCurrent));
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDP0LowerStock);
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDP1LowerStock);
    hash = ndsPlatformMixDebugValue(hash,
                                    ndsPlatformBattleHudDisplaySeconds());
    return hash;
}

static void ndsPlatformRenderBattlePlayerText(u32 player, u32 row,
                                               u32 fighter_kind,
                                               u32 damage, u32 stock)
{
    u32 player_bit = 1u << player;

    if ((gNdsIFCommonHUDActivePlayerMask & player_bit) == 0u)
    {
        ndsPlatformPrintDebugLine(row, "");
        ndsPlatformPrintDebugLine(row + 1u, "");
        return;
    }

    if ((gNdsIFCommonHUDCPUPlayerMask & player_bit) != 0u)
    {
        u32 level = (player == 0u) ? gNdsIFCommonHUDP0Level :
                                     gNdsIFCommonHUDP1Level;

        ndsPlatformPrintDebugLine(row, "CPU L%lu [%s]",
                                  (unsigned long)level,
                                  ndsPlatformBattleHudFighterName(
                                      fighter_kind));
    }
    else
    {
        ndsPlatformPrintDebugLine(row, "P%lu [%s]",
                                  (unsigned long)(player + 1u),
                                  ndsPlatformBattleHudFighterName(
                                      fighter_kind));
    }
    if ((gNdsIFCommonHUDShowDamageMask & player_bit) == 0u)
    {
        if (stock == 0x7fu)
        {
            ndsPlatformPrintDebugLine(row + 1u,
                                      "DMG --    STOCK --");
        }
        else
        {
            ndsPlatformPrintDebugLine(row + 1u,
                                      "DMG --    STOCK x%lu",
                                      (unsigned long)stock);
        }
    }
    else if (stock == 0x7fu)
    {
        ndsPlatformPrintDebugLine(row + 1u,
                                  "DMG %lu%%   STOCK --",
                                  (unsigned long)damage);
    }
    else
    {
        ndsPlatformPrintDebugLine(row + 1u,
                                  "DMG %lu%%   STOCK x%lu",
                                  (unsigned long)damage,
                                  (unsigned long)stock);
    }
}

static void ndsPlatformRenderBattleTextHud(void)
{
    u32 fingerprint = ndsPlatformBattleTextHudStateFingerprint();
    u32 seconds = ndsPlatformBattleHudDisplaySeconds();

    gNdsBattleTextHudRenderCount++;
    if ((sBattleTextHudReady != FALSE) &&
        (fingerprint == sBattleTextHudFingerprint))
    {
        return;
    }
    sBattleTextHudReady = TRUE;
    sBattleTextHudFingerprint = fingerprint;
    gNdsBattleTextHudChangeCount++;
    gNdsBattleTextHudFingerprint = fingerprint;
    gNdsBattleTextHudTimeSeconds = seconds;
    gNdsBattleTextHudP0Damage = ndsPlatformBattleHudDisplayDamage(
        gNdsIFCommonHUDP0DamageCurrent);
    gNdsBattleTextHudP1Damage = ndsPlatformBattleHudDisplayDamage(
        gNdsIFCommonHUDP1DamageCurrent);
    gNdsBattleTextHudP0Stock = gNdsIFCommonHUDP0LowerStock;
    gNdsBattleTextHudP1Stock = gNdsIFCommonHUDP1LowerStock;
    gNdsBattleTextHudActiveMask = gNdsIFCommonHUDActivePlayerMask;
    gNdsBattleTextHudShowDamageMask = gNdsIFCommonHUDShowDamageMask;

    if (gNdsIFCommonHUDRecordCount == 0u)
    {
        ndsPlatformPrintDebugLine(2u, "TIME  --:--");
    }
    else
    {
        ndsPlatformPrintDebugLine(2u, "TIME  %02lu:%02lu",
                                  (unsigned long)(seconds / 60u),
                                  (unsigned long)(seconds % 60u));
    }
    ndsPlatformRenderBattlePlayerText(
        0u, 5u, gNdsIFCommonHUDP0FighterKind,
        ndsPlatformBattleHudDisplayDamage(
            gNdsIFCommonHUDP0DamageCurrent),
        gNdsIFCommonHUDP0LowerStock);
    ndsPlatformRenderBattlePlayerText(
        1u, 9u, gNdsIFCommonHUDP1FighterKind,
        ndsPlatformBattleHudDisplayDamage(
            gNdsIFCommonHUDP1DamageCurrent),
        gNdsIFCommonHUDP1LowerStock);
}
#endif

void ndsPlatformClearBattleTextHud(void)
{
    gNdsBattleTextHudClearCount++;
#if NDS_BATTLE_FPS_HUD_ENABLED
    if ((sBattleFpsHudSampleReady != 0u) ||
        (sBattleTextHudReady != FALSE))
    {
        consoleClear();
    }
    sBattleFpsHudSampleReady = 0u;
    sBattleFpsHudPrintedFpsX10 = 0xffffffffu;
    sBattleTextHudReady = FALSE;
    sBattleTextHudFingerprint = 0xffffffffu;
#endif
}

static u32 ndsPlatformScaleToFps(u32 delta, u32 elapsed_ticks)
{
    if (elapsed_ticks == 0)
    {
        return 0;
    }
    return ((delta * 60u) + (elapsed_ticks / 2u)) / elapsed_ticks;
}

static u32 ndsPlatformOpeningMovieLogicTickCount(void)
{
    return gNdsOpeningRoomTickCount +
           gNdsOpeningPortraitsTickCount +
           gNdsOpeningMarioTickCount +
           gNdsOpeningMovieActionPreviewFrameCount;
}

static u32 ndsPlatformPreviewCommitCount(void)
{
    return gNdsOriginalSpritePreviewCommitCount +
           gNdsOriginalDLPreviewCommitCount;
}

static void ndsPlatformUpdatePerfCounters(void)
{
    u32 now_tick = sTicks;
    u32 logic_tick_count = ndsPlatformOpeningMovieLogicTickCount();
    u32 preview_commit_count = ndsPlatformPreviewCommitCount();
    u32 elapsed_ticks;

    if (sPerfSampleReady == 0)
    {
        sPerfSampleReady = 1;
        sPerfLastTick = now_tick;
        sPerfLastFrameCounter = gNdsFrameCounter;
        sPerfLastLogicTickCount = logic_tick_count;
        sPerfLastDLPreviewDrawCount = gNdsOriginalDLPreviewDrawCount;
        sPerfLastPreviewCommitCount = preview_commit_count;
        return;
    }

    elapsed_ticks = now_tick - sPerfLastTick;
    if (elapsed_ticks < NDS_PERF_SAMPLE_TICKS)
    {
        return;
    }

    gNdsPerfPresentFps = ndsPlatformScaleToFps(
        gNdsFrameCounter - sPerfLastFrameCounter,
        elapsed_ticks);
    gNdsPerfLogicFps = ndsPlatformScaleToFps(
        logic_tick_count - sPerfLastLogicTickCount,
        elapsed_ticks);
    gNdsPerfDLDrawFps = ndsPlatformScaleToFps(
        gNdsOriginalDLPreviewDrawCount - sPerfLastDLPreviewDrawCount,
        elapsed_ticks);
    gNdsPerfPreviewCommitFps = ndsPlatformScaleToFps(
        preview_commit_count - sPerfLastPreviewCommitCount,
        elapsed_ticks);
    gNdsPerfPreviewCommitCount = preview_commit_count;
    gNdsPerfSampleWindowTicks = elapsed_ticks;
    gNdsPerfSampleCount++;

    sPerfLastTick = now_tick;
    sPerfLastFrameCounter = gNdsFrameCounter;
    sPerfLastLogicTickCount = logic_tick_count;
    sPerfLastDLPreviewDrawCount = gNdsOriginalDLPreviewDrawCount;
    sPerfLastPreviewCommitCount = preview_commit_count;
}

static u32 ndsPlatformOpeningHudTickMilestone(void)
{
    u32 tick = gNdsOpeningRoomTickCount;

    if ((gNdsOpeningRoomDrawResult == NDS_OPENING_ROOM_DRAW_PASS) ||
        (tick >= 560u))
    {
        return 560u;
    }
    if ((gNdsOpeningRoomTick500RunResult ==
         NDS_OPENING_ROOM_TICK500_RUN_PASS) ||
        (tick >= 500u))
    {
        return 500u;
    }
    if ((gNdsOpeningRoomTick450RunResult ==
         NDS_OPENING_ROOM_TICK450_RUN_PASS) ||
        (tick >= 450u))
    {
        return 450u;
    }
    if ((gNdsOpeningRoomTick380DeferredResult ==
         NDS_OPENING_ROOM_TICK380_DEFER_PASS) ||
        (tick >= 380u))
    {
        return 380u;
    }
    if ((gNdsOpeningRoomFirstEventRunResult ==
         NDS_OPENING_ROOM_FIRST_EVENT_RUN_PASS) ||
        (tick >= 280u))
    {
        return 280u;
    }
    if (gNdsOpeningRoomFuncStartResult == NDS_OPENING_ROOM_FUNC_START_PASS)
    {
        return 1u;
    }
    return 0u;
}

static u32 ndsPlatformDebugTextFingerprint(void)
{
    u32 hash = 2166136261u;

    hash = ndsPlatformMixDebugValue(hash, gNdsOriginalBootStage);
    hash = ndsPlatformMixDebugValue(hash, gNdsBootSelfTestResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsSceneBoundaryResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsSceneBoundaryKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsTaskmanReturnCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsTaskmanBridgeResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsTaskmanCleanupResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsRelocAssetInitResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocFileMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocHeaderMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocPayloadMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocWordSwapMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocWordSwapCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocPointerFixupMask);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningRoomRelocPointerFixupCount);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningRoomFirstEventDeferredMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomTick380DeferredMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomTick450DeferredMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomTick500DeferredMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomTick560DeferredMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomSpotlightCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomScene1CameraCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomScene2CameraCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomLogoCameraCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDeskCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomHazeCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomOutsideCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomSunlightCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomPencilsCreateResult);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningRoomFighterDeferredKind & 0xffu);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDLPreviewResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDLPreviewTriangleCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDLPreviewPixelCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDLPreviewTextureMask);
    hash = ndsPlatformMixDebugValue(hash, ndsPlatformOpeningHudTickMilestone());
    hash = ndsPlatformMixDebugValue(hash, sHeldKeys & 0xfffu);
    hash = ndsPlatformMixDebugValue(hash, gNdsControllerLivePad0Button);
    hash = ndsPlatformMixDebugValue(hash, (u32)gNdsControllerLivePad0StickX);
    hash = ndsPlatformMixDebugValue(hash, (u32)gNdsControllerLivePad0StickY);
    hash = ndsPlatformMixDebugValue(hash, gSYControllerDevices[0].button_hold);
    hash = ndsPlatformMixDebugValue(hash, gSYControllerDevices[0].button_tap);
    hash = ndsPlatformMixDebugValue(hash,
                                    (u32)gSYControllerDevices[0].stick_range.x);
    hash = ndsPlatformMixDebugValue(hash,
                                    (u32)gSYControllerDevices[0].stick_range.y);
    hash = ndsPlatformMixDebugValue(hash,
                                    (u32)gNdsFighterBattlePlayableFinalXMilli);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfSampleCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfPresentFps);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfLogicFps);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfDLDrawFps);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfPreviewCommitFps);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfPreviewCommitCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomSkipToTitleCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDrawResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDrawBlocker);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningRoomDrawCameraCallbackCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDrawDObjCallbackCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieRoomHandoffResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieRoomHandoffTick);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieRoomHandoffScene);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsStartResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsFuncStartResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsUpdateResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsRelocResult);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningPortraitsSpriteNormalizeCount);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningPortraitsSpriteNormalizeFailCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsTickCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsDrawResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsDrawBlocker);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningPortraitsDrawVisibleSObjCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsDrawWidth);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsDrawHeight);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsDrawPixels);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsNextSceneResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsNextSceneKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMarioTickCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMarioDrawResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMarioDrawVisibleSObjCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMarioDrawPixels);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningNameSceneDispatchMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningNameSceneDrawMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningNameSceneDispatchCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningNameSceneLastKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningNameSceneLastNextKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieBridgeResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieBridgeMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieBridgeCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieActionPreviewResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieActionPreviewMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieActionPreviewCount);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningMovieActionPreviewFrameCount);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningMovieActionPreviewLastKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieTitleResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleRelocResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitlePreviewResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleDrawResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleSpriteNormalizeCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleSpriteNormalizeFailCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleDrawVisibleSObjCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleDrawRenderableSObjCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleDrawPixels);

    return hash;
}

void ndsPlatformRenderDebugHud(void)
{
    u32 debug_text_fingerprint;

#if NDS_BATTLE_FPS_HUD_ENABLED && !NDS_DEBUG_HUD
    if (gNdsBattlePlayablePacingDrawCalls != 0u)
    {
        ndsPlatformRenderBattleFpsHud();
        ndsPlatformRenderBattleTextHud();
    }
#endif
#if !NDS_DEBUG_HUD
    return;
#endif
#if !NDS_RENDERER_HW_TRIANGLES
    ndsPlatformDrawOriginalDLPreview();
    ndsPlatformDrawOriginalSpritePreview();
#endif
    ndsPlatformUpdatePerfCounters();

    debug_text_fingerprint = ndsPlatformDebugTextFingerprint();
    if ((sDebugTextReady != 0) &&
        (debug_text_fingerprint == sDebugTextFingerprint))
    {
        return;
    }
    sDebugTextReady = 1;
    sDebugTextFingerprint = debug_text_fingerprint;

    ndsPlatformPrintDebugLine(0, "Smash64DS movie debug");
    ndsPlatformPrintDebugLine(1, "text=change %08lx",
                              (unsigned long)debug_text_fingerprint);
    ndsPlatformPrintDebugLine(2, "boot=%08lx self=%08lx",
                              (unsigned long)gNdsOriginalBootStage,
                              (unsigned long)gNdsBootSelfTestResult);
    ndsPlatformPrintDebugLine(3, "scene=%08lx k=%lu ret=%lu",
                              (unsigned long)gNdsSceneBoundaryResult,
                              (unsigned long)gNdsSceneBoundaryKind,
                              (unsigned long)gNdsTaskmanReturnCount);
    ndsPlatformPrintDebugLine(4, "task=%08lx clean=%08lx",
                              (unsigned long)gNdsTaskmanBridgeResult,
                              (unsigned long)gNdsTaskmanCleanupResult);
    ndsPlatformPrintDebugLine(5, "rel=%08lx nfs=%08lx",
                              (unsigned long)gNdsOpeningRoomRelocResult,
                              (unsigned long)gNdsRelocAssetInitResult);
    ndsPlatformPrintDebugLine(6, "file=%02lx hdr=%02lx pay=%02lx",
                              (unsigned long)gNdsOpeningRoomRelocFileMask,
                              (unsigned long)gNdsOpeningRoomRelocHeaderMask,
                              (unsigned long)gNdsOpeningRoomRelocPayloadMask);
    ndsPlatformPrintDebugLine(7, "fix sw=%02lx/%lu pt=%02lx/%lu",
                              (unsigned long)gNdsOpeningRoomRelocWordSwapMask,
                              (unsigned long)gNdsOpeningRoomRelocWordSwapCount,
                              (unsigned long)gNdsOpeningRoomRelocPointerFixupMask,
                              (unsigned long)gNdsOpeningRoomRelocPointerFixupCount);
    ndsPlatformPrintDebugLine(8, "evt28=%02lx 38=%02lx 45=%02lx",
                              (unsigned long)gNdsOpeningRoomFirstEventDeferredMask,
                              (unsigned long)gNdsOpeningRoomTick380DeferredMask,
                              (unsigned long)gNdsOpeningRoomTick450DeferredMask);
    ndsPlatformPrintDebugLine(9, "evt50=%02lx 56=%02lx sp=%02lx",
                              (unsigned long)gNdsOpeningRoomTick500DeferredMask,
                              (unsigned long)gNdsOpeningRoomTick560DeferredMask,
                              (unsigned long)gNdsOpeningRoomSpotlightCreateMask);
    ndsPlatformPrintDebugLine(10, "cam s1=%03lx s2=%03lx l=%02lx",
                              (unsigned long)gNdsOpeningRoomScene1CameraCreateMask,
                              (unsigned long)gNdsOpeningRoomScene2CameraCreateMask,
                              (unsigned long)gNdsOpeningRoomLogoCameraCreateMask);
    ndsPlatformPrintDebugLine(11, "obj d=%02lx h=%02lx o=%02lx s=%02lx",
                              (unsigned long)gNdsOpeningRoomDeskCreateMask,
                              (unsigned long)gNdsOpeningRoomHazeCreateMask,
                              (unsigned long)gNdsOpeningRoomOutsideCreateMask,
                              (unsigned long)gNdsOpeningRoomSunlightCreateMask);
    ndsPlatformPrintDebugLine(12, "pcl=%08lx fk=%02lx",
                              (unsigned long)gNdsOpeningRoomPencilsCreateResult,
                              (unsigned long)(gNdsOpeningRoomFighterDeferredKind & 0xffu));
    ndsPlatformPrintDebugLine(13, "dlp=%08lx t=%lu p=%lu x=%02lx",
                              (unsigned long)gNdsOpeningRoomDLPreviewResult,
                              (unsigned long)gNdsOpeningRoomDLPreviewTriangleCount,
                              (unsigned long)gNdsOpeningRoomDLPreviewPixelCount,
                              (unsigned long)gNdsOpeningRoomDLPreviewTextureMask);
    ndsPlatformPrintDebugLine(14, "tick~%lu key=%03lx skip=%lu",
                              (unsigned long)ndsPlatformOpeningHudTickMilestone(),
                              (unsigned long)(sHeldKeys & 0xfffu),
                              (unsigned long)gNdsOpeningRoomSkipToTitleCount);
    ndsPlatformPrintDebugLine(15, "draw=%08lx b=%lu c=%lu d=%lu",
                              (unsigned long)gNdsOpeningRoomDrawResult,
                              (unsigned long)gNdsOpeningRoomDrawBlocker,
                              (unsigned long)gNdsOpeningRoomDrawCameraCallbackCount,
                              (unsigned long)gNdsOpeningRoomDrawDObjCallbackCount);
    ndsPlatformPrintDebugLine(16, "mv h=%08lx p=%08lx",
                              (unsigned long)gNdsOpeningMovieRoomHandoffResult,
                              (unsigned long)gNdsOpeningPortraitsStartResult);
    ndsPlatformPrintDebugLine(17, "por t=%lu d=%08lx v=%lu",
                              (unsigned long)gNdsOpeningPortraitsTickCount,
                              (unsigned long)gNdsOpeningPortraitsDrawResult,
                              (unsigned long)gNdsOpeningPortraitsDrawVisibleSObjCount);
    ndsPlatformPrintDebugLine(18, "mario t=%lu v=%lu px=%lu",
                              (unsigned long)gNdsOpeningMarioTickCount,
                              (unsigned long)gNdsOpeningMarioDrawVisibleSObjCount,
                              (unsigned long)gNdsOpeningMarioDrawPixels);
    ndsPlatformPrintDebugLine(19, "name m=%02lx d=%02lx c=%lu",
                              (unsigned long)gNdsOpeningNameSceneDispatchMask,
                              (unsigned long)gNdsOpeningNameSceneDrawMask,
                              (unsigned long)gNdsOpeningNameSceneDispatchCount);
    ndsPlatformPrintDebugLine(20, "fps=%02lu up=%02lu dl=%02lu cv=%02lu",
                              (unsigned long)gNdsPerfPresentFps,
                              (unsigned long)gNdsPerfLogicFps,
                              (unsigned long)gNdsPerfDLDrawFps,
                              (unsigned long)gNdsPerfPreviewCommitFps);
    ndsPlatformPrintDebugLine(21, "ch=%03lx pf=%03lx smp=%02lu win=%02lu",
                              (unsigned long)(gNdsPerfPreviewCommitCount &
                                              0xfffu),
                              (unsigned long)(gNdsOpeningMoviePresentFrameCount &
                                              0xfffu),
                              (unsigned long)(gNdsPerfSampleCount & 0xffu),
                              (unsigned long)(gNdsPerfSampleWindowTicks & 0xffu));
    ndsPlatformPrintDebugLine(22, "inp k=%03lx p=%04lx %ld,%ld",
                              (unsigned long)(sHeldKeys & 0xfffu),
                              (unsigned long)gNdsControllerLivePad0Button,
                              (long)gNdsControllerLivePad0StickX,
                              (long)gNdsControllerLivePad0StickY);
    ndsPlatformPrintDebugLine(23, "sy h=%04x t=%04x %d,%d x=%ld",
                              (unsigned int)gSYControllerDevices[0].button_hold,
                              (unsigned int)gSYControllerDevices[0].button_tap,
                              (int)gSYControllerDevices[0].stick_range.x,
                              (int)gSYControllerDevices[0].stick_range.y,
                              (long)gNdsFighterBattlePlayableFinalXMilli);
}

u32 ndsPlatformVBlankCount(void)
{
    return sVBlankCount;
}

void ndsPlatformSchedulePresentAtVBlank(u32 vblank)
{
    sEarliestPresentVBlank = vblank;
}

static void ndsPlatformWaitForScheduledVBlank(void)
{
    u32 earliest = sEarliestPresentVBlank;

    sEarliestPresentVBlank = 0u;
    NDS_FREEZE_DIAGNOSTICS_VBLANK_WAIT();
    do
    {
        swiWaitForVBlank();
    }
    while ((earliest != 0u) &&
           ((s32)(sVBlankCount - earliest) < 0));
}

void ndsPlatformEndFrame(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 profile_start;

    gNdsRendererProfileFlushTicks = 0u;
    gNdsRendererProfileVBlankWaitTicks = 0u;
    gNdsRendererProfileGXStatusBeforeFlush = GFX_STATUS;
    gNdsRendererProfileGXStatusAfterFlush = GFX_STATUS;
    gNdsRendererProfileGXControlBeforeFlush = GFX_CONTROL;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 phase05_flush_prep_done = FALSE;
    u32 phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif
#if NDS_RENDERER_HW_TRIANGLES
    u32 submitted = ndsRendererHardwareConsumeSubmittedFrame();

    if ((submitted != 0u) || (sOriginalSpriteOverlayNeedsFlush != FALSE))
    {
        if (submitted != 0u)
        {
            gNdsHardwareRendererSubmittedFrameCount++;
        }
        gNdsHardwareRendererPolyRamCount = GFX_POLYGON_RAM_USAGE;
        gNdsHardwareRendererVertexRamCount = GFX_VERTEX_RAM_USAGE;
        gNdsHardwareRendererStatus = GFX_STATUS;
        gNdsHardwareRendererControl = GFX_CONTROL;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileGXStatusBeforeFlush = GFX_STATUS;
        gNdsRendererProfileGXControlBeforeFlush = GFX_CONTROL;
#if NDS_RENDERER_M3_PHASE0_PROFILE
        NDS_RENDERER_PHASE05_FINISH(
            gNdsRendererPhase05FlushPrepTicks, phase05_start);
        phase05_flush_prep_done = TRUE;
#endif
        profile_start = cpuGetTiming();
#endif
        NDS_FREEZE_DIAGNOSTICS_FLUSH();
#if NDS_TASK29_GX_CENSUS
        ndsRendererTask29GXRecordFlush(GL_TRANS_MANUALSORT);
#endif
        glFlush(GL_TRANS_MANUALSORT);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileFlushTicks = cpuGetTiming() - profile_start;
        gNdsRendererProfileGXStatusAfterFlush = GFX_STATUS;
#endif
        gNdsHardwareRendererFlushCount++;
        sOriginalSpriteOverlayNeedsFlush = FALSE;
    }
#if NDS_TASK29_GX_CENSUS
    ndsRendererTask29GXPublishFrame();
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    if (phase05_flush_prep_done == FALSE)
    {
        NDS_RENDERER_PHASE05_FINISH(
            gNdsRendererPhase05FlushPrepTicks, phase05_start);
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    profile_start = cpuGetTiming();
#endif
    ndsPlatformWaitForScheduledVBlank();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileVBlankWaitTicks = cpuGetTiming() - profile_start;
    profile_start = cpuGetTiming();
#endif
#if NDS_SCENE_MIP_CACHE_LAB
    ndsPlatformSceneWallpaperCommitAffine();
#endif
#if NDS_FAST_WALLPAPER_AFFINE
    ndsPlatformFastWallpaperCommitAffine();
#endif
    ndsRendererHardwareCommitPendingTextureRefreshes();
    ndsIFCommonNativeOamCommit();
#if NDS_SCENE_MIP_CACHE_LAB
    if (sSceneMipCapturePending != 0u)
    {
        /* EndFrame runs after the SObj compositor committed BG2. Mark that
         * exact source wallpaper as retained without copying it through GX
         * texture VRAM or consuming a second VBlank. */
        ndsPlatformSceneMipFinishCapture();
        ndsPlatformSceneWallpaperCommitAffine();
    }
#endif
    sTicks++;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfilePostVBlankTicks +=
        cpuGetTiming() - profile_start;
#endif
#else
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    profile_start = cpuGetTiming();
#endif
    ndsPlatformWaitForScheduledVBlank();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileVBlankWaitTicks = cpuGetTiming() - profile_start;
    profile_start = cpuGetTiming();
#endif
    videoSetMode((sDrawFramebufferIndex == 0) ? MODE_FB0 : MODE_FB1);
    sDrawFramebufferIndex ^= 1u;
    sFramebuffer = sFramebuffers[sDrawFramebufferIndex];
    sTicks++;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfilePostVBlankTicks +=
        cpuGetTiming() - profile_start;
#endif
#endif
}

void ndsPlatformProfileSampleFrameBoundaryGXState(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    ndsRendererProfileRecordFrameBoundaryGXState(GFX_STATUS, GFX_CONTROL);
#endif
}

u32 ndsPlatformTicks(void)
{
    return sTicks;
}

u32 ndsPlatformHeldKeys(void)
{
    return sHeldKeys;
}
