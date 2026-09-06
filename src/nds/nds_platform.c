#include <nds.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <nds/nds_boot.h>
#include <nds/nds_audio_bgm.h>
#include <nds/nds_battle_hud.h>
#include <nds/nds_controller.h>
#include <nds/nds_freeze_diagnostics.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_menu_shell.h>
#include <nds/nds_ui_kit.h>
#if NDS_R2_HWMATH_BENCH
#include <nds/nds_r2_hwmath_bench.h>
#endif
#include <nds/nds_platform.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/nds_task37_itcm.h>
#include <nds/nds_scene.h>
#include <nds/nds_startup.h>
#include <nds/nds_video.h>
#include <sys/controller.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#ifndef NDS_DEBUG_HUD
#define NDS_DEBUG_HUD 0
#endif

#if NDS_DEBUG_HUD
#error "NDS_DEBUG_HUD legacy debug wall is retired"
#endif

#ifndef NDS_SCENE_MIP_CACHE_LAB
#define NDS_SCENE_MIP_CACHE_LAB 0
#endif

#ifndef NDS_FAST_WALLPAPER_AFFINE
#define NDS_FAST_WALLPAPER_AFFINE 0
#endif

#ifndef NDS_R2_CAMERA_FIXED_TOGGLE
#define NDS_R2_CAMERA_FIXED_TOGGLE 0
#endif

#if NDS_R2_CAMERA_FIXED_TOGGLE
/* Lab-only live A/B of the Q20.12 camera chain. gNdsR2CameraFixedEnabled is
 * already a `.data` word read at each call site rather than a compile-time
 * gate, so flipping it between frames is exactly the same switch -SetGlobals
 * performs -- the only new thing here is who pushes it.
 *
 * SAFE MID-MATCH, and the reason is structural rather than empirical: both arms
 * of both producers are pure functions of the CObj passed to them that frame,
 * and every value either arm publishes -- gGMCameraMatrix, sGCMatrixProjectL,
 * gGCMatrixPerspF, the caller's Mtx, the renderer's NDSRendererMatrix20p12 --
 * is rewritten from scratch on the next entry. Nothing is carried across a
 * frame, so no arm can read a value the other arm left behind. The one field
 * BOTH arms write into live state, gGMCameraStruct.look_at, has exactly two
 * referrers in this tree and they are those two writes; its only consumers in
 * the source are gSPLookAtX/gSPLookAtY, which are RSP display-list commands
 * this port does not emit. No simulation reader exists, so the flip cannot
 * desync the fight. See artifacts/performance/2026-08-16_camera-fixedpoint/. */
#include <nds/nds_r2_camera_fixed.h>
#endif
#if NDS_R2_SIM_MAC_SHADOW
#include <nds/nds_r2_sim_mac_fixed.h>
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
#define NDS_BATTLE_TICK_HUD_ENABLED \
    (NDS_BATTLE_FPS_HUD_ENABLED && NDS_TICK_HUD)
/* R2-03 E30. The tick HUD's on-screen block re-sorts eleven 128-entry rings and
 * pushes thirteen vsnprintf/iprintf lines through the libnds text console about
 * twice a second, and it measured 345,024 ticks each time against a 960-tick
 * median -- 30% of the frame budget, landing on exactly the frames the P95 gate
 * is decided on. None of it exists in the published ROM, and the GDB sampler
 * reads sBattleTickHudRing directly and never touches sBattleTickHudP50/P95, so
 * for a scripted measurement the whole block is instrument cost with no reader.
 * Set NDS_TICK_HUD_DRAW=0 for a measurement run; leave it 1 to read the HUD on
 * a device or in a screenshot. */
#define NDS_BATTLE_TICK_HUD_DRAW_ENABLED \
    (NDS_BATTLE_TICK_HUD_ENABLED && NDS_TICK_HUD_DRAW)
#if NDS_R2_CAMERA_FIXED_TOGGLE && !NDS_BATTLE_FPS_HUD_ENABLED
/* The indicator draws into the battle FPS HUD's console. Without it the owner
 * could flip the camera arm and have no way to tell which one is on screen,
 * which is the one failure this build exists to prevent -- so it is a build
 * error, not a silently degraded ROM. */
#error "NDS_R2_CAMERA_FIXED_TOGGLE needs NDS_BATTLE_FPS_HUD_ENABLED for its arm indicator"
#endif
#if NDS_LAB_NO_CULL && !NDS_BATTLE_FPS_HUD_ENABLED
/* Same rule for the seam probe: an arm nobody can read off the screenshot is
 * not evidence. */
#error "NDS_LAB_NO_CULL needs NDS_BATTLE_FPS_HUD_ENABLED for its arm indicator"
#endif
#if NDS_LAB_NO_CULL && NDS_R2_CAMERA_FIXED_TOGGLE
/* Both bind SELECT and both print row 3. One SELECT meaning per build. */
#error "NDS_LAB_NO_CULL and NDS_R2_CAMERA_FIXED_TOGGLE both own SELECT"
#endif
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
#if NDS_R204_FPSHUD_SHADOW
volatile u32 gNdsR204FpsHudShadowX10;
volatile u32 gNdsR204FpsHudShadowFrames;
volatile u32 gNdsR204FpsHudShadowTicks;
volatile u32 gNdsR204FpsHudShadowBusClock;
#endif
static u32 sBattleFpsHudSampleReady;
static u32 sBattleFpsHudLastTick;
static u32 sBattleFpsHudLastPresentedFrames;
static u32 sBattleFpsHudLastLogicFrames;
static u32 sBattleFpsHudPrintedFpsX10 = 0xffffffffu;
static u32 sBattleFpsHudPrintedUpdatesX10 = 0xffffffffu;
#if NDS_R2_CAMERA_FIXED_TOGGLE
/* Its own repaint gate rather than a field of the text HUD's fingerprint: that
 * fingerprint is a function of match state only, so an arm flip would not
 * repaint until the clock or a damage value happened to change and the owner
 * would press SELECT and watch nothing happen for up to a second. */
static u32 sBattleCameraArmPrinted = 0xffffffffu;
#endif
#if NDS_LAB_NO_CULL
/* Same gate, same reason, for the seam probe's arm line. */
static u32 sBattleSeamArmPrinted = 0xffffffffu;
#endif
static u32 sBattleTextHudReady;
static u32 sBattleTextHudFingerprint = 0xffffffffu;
/* Defined in src/import/battleship_ifcommon.c, beside the HUD mirrors it
 * guards. Declared here rather than in a header because this file must stay
 * out of the BattleShip scene include graph (nds_menu_shell.h's rule). */
extern s32 ndsIFCommonBattleHudInterfaceVisible(void);
#if NDS_BATTLE_PHASE_HUD_ENABLED
static u32 sBattlePhaseHudLastSlipCount;
#endif
#endif
#if NDS_RENDERER_HW_TRIANGLES
/* The native shell does not rasterize source SObjs. Keep this 150 KiB scratch
 * in the scene that actually uses it, rather than permanently shrinking every
 * scene arena. Hardware commits copy its pixels into BG VRAM immediately. */
static u16 *sOriginalSpritePreview;
static u32 sOriginalSpritePreviewGeneration;
extern volatile u32 gNdsTaskmanHeapGeneration;
extern void *syTaskmanMalloc(size_t size, u32 align);
#else
static u16 sOriginalSpritePreview[
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH *
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT];
#endif
static u32 sOriginalSpritePreviewWidth;
static u32 sOriginalSpritePreviewHeight;
static s32 sOriginalSpritePreviewX;
static s32 sOriginalSpritePreviewY;
static u32 sOriginalSpritePreviewReady;
#if NDS_RENDERER_HW_TRIANGLES
static int sOriginalSpriteOverlayBg = -1;
static int sOriginalSpriteOverlayForegroundBg = -1;
/* P2-1i. The fire's affine steps, held so a frame change can rewrite the
 * whole matrix+scroll block in one call without re-deriving them. */
static s32 sTitleFirePa = 1 << 8;
static s32 sTitleFirePd = 1 << 8;
/* Published so "the animation ran" is a counter and not a screenshot: enable
 * and disable must balance across a loop, and Frame must climb once per
 * presented title frame. */
__attribute__((used)) volatile u32 gNdsTitleFireEnableCount;
__attribute__((used)) volatile u32 gNdsTitleFireDisableCount;
__attribute__((used)) volatile u32 gNdsTitleFireFrameCount;
static u32 sOriginalSpriteOverlayLayerMask;
static s32 sOriginalSpriteOverlayNeedsFlush;
/* See ndsPlatformSet3DLayerEnabled: an enable is ARMED here and committed by
 * ndsPlatformEndFrame only after a real GX frame was submitted, so BG0 can
 * never composite the previous 3D owner's retained frame. */
static s32 s3dLayerEnableOnNextPresent;
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
#if !NDS_RENDERER_HW_TRIANGLES
/* Software builds retain the complete 320x240 diagnostic image. Hardware
 * builds commit the staging layer directly to BG VRAM and no longer carry a
 * second full RGB555 wallpaper decode; that DS-only source view belongs to the
 * SObj backend that consumes it. */
static u16 sOriginalSpriteDisplayPreview[
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH *
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT];
#endif
static u32 sOriginalSpriteDecodeCacheEpoch = 1u;
static u32 sOriginalSpriteDisplayPreviewWidth;
static u32 sOriginalSpriteDisplayPreviewHeight;
/* The DL preview's only consumer is ndsPlatformDrawOriginalSpritePreview's
 * companion blit below, and that whole block is `#if !NDS_RENDERER_HW_TRIANGLES`.
 * On a hardware-triangle build -- which is what P1 ships -- nothing ever reads
 * either array, so the producer was filling 21,600 bytes of main RAM that no
 * path displays. Storage follows its reader's condition exactly; that is what
 * makes this safe rather than a guess about who might use it.
 *
 * Proven on the linked ROM, not by grepping source: of the seven call sites the
 * source has for ndsPlatformBeginOriginalDLPreview, exactly one survives into
 * the shipped battle ELF (ndsOpeningRoomRenderDLPreview), and the only functions
 * holding an address inside either array are the three API entry points here. */
#if !NDS_RENDERER_HW_TRIANGLES
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
#endif
#if NDS_DEBUG_HUD
static u32 sDebugTextFingerprint = 0xffffffffu;
static u32 sDebugTextReady;
#endif

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
/* KEEP these five even when the DL preview compiles out. On a hardware-triangle
 * build nothing writes them any more, so `--gc-sections` drops them and every
 * harness that reads them fails to resolve the symbol -- which is how the
 * Boundary verifier went RED on "Missing ELF symbol gNdsOriginalDLPreviewReady"
 * rather than on any behaviour change. Five readers depend on them:
 * verify-runtime.ps1 (OPENING_ROOM_DL_PREVIEW_PRESENT, PERF_CONTENT),
 * verify-battle-mariofox-gcrunall-loop-harness.ps1 (PLATFORM_DL_PREVIEW) and
 * sample-runtime-speed.ps1.
 *
 * Reading a permanent 0 here is the correct measurement, not a stub: it is the
 * verifier's existing assertion that the preview never engages, and on hwtri
 * that is now guaranteed structurally because the code which could set it does
 * not exist. Non-hwtri builds are unaffected and still drive these normally.
 * 20 bytes total. */
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
volatile u32 gNdsBattleTextHudP2Damage;
volatile u32 gNdsBattleTextHudP3Damage;
volatile u32 gNdsBattleTextHudP0Stock;
volatile u32 gNdsBattleTextHudP1Stock;
volatile u32 gNdsBattleTextHudP2Stock;
volatile u32 gNdsBattleTextHudP3Stock;
volatile u32 gNdsBattleTextHudActiveMask;
volatile u32 gNdsBattleTextHudShowDamageMask;
volatile u32 gNdsBattleTextHudClearCount;
volatile u32 gNdsHardwareRendererSubmittedFrameCount;
volatile u32 gNdsHardwareRendererFlushCount;
/* P2-1M gate catch (2026-08-19): a flush fired for the menu overlay's queued
 * transform commit (sOriginalSpriteOverlayNeedsFlush) with no submitted 3D
 * frame is not a hardware FRAME flush -- counting it in FlushCount made
 * flushed read submitted+1 forever on the shell path (the gcrunall gate's
 * "submitted -eq flushed stays exact" invariant), while direct-boot never
 * queued one. It gets its own counter so nothing is hidden. */
__attribute__((used)) volatile u32 gNdsHardwareRendererOverlayOnlyFlushCount;
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

/* An ARM9 self-restart lived here briefly, as the cheap way past the rematch's
 * second-entry geometry corruption. It is deleted rather than left default-off
 * because it does not work and a dead selector is worse than none. The full
 * measurement and the three toolchain reasons are recorded at the one place a
 * reader would look for them, `ndsMNVSResultsSetLoadScene` in
 * `src/import/battleship_mnvsresults.c`. Short version: calico links no
 * `svcSoftReset`, bare BIOS `swi #0` is measurably a no-op here, and
 * `crt0Startup` needs three loader-supplied arguments that no longer exist. */

void ndsPlatformInit(void)
{
    /* Calico's system tick owns timers 2/3. Initialize it before original
     * code can sample libultra time/count; BGM seam timing uses free timer 0. */
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

#if NDS_R2_HWMATH_BENCH
    /* Once, at boot, before any scene exists. It is loading-time work by
     * construction -- the thing PROJECT_GOAL.md says is cheap -- and it cannot
     * perturb a gameplay frame because it has finished before the first one.
     * The counters it publishes are read at the end of the run by
     * sample-tick-hud-buckets.ps1 -ExtraGlobals. */
    ndsR2HwMathBenchRun();
#endif

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
#if (NDS_P2_MENU_SHELL && NDS_P2_MENU_WALK)
    /* P2-1g. The scripted walk's ONE non-shell button: START on the Results
     * screen, which is the imported scene's own exit and the only input in the
     * loop that no shell handler can synthesise. It is ORed in BEFORE the
     * latch, so it reaches `osContGetReadData` (which reads exactly this
     * latched value) and from there the source's controller pipeline and
     * `mnVSResultsCheckExit`. Compiled out of every published and Boundary
     * configuration with the rest of the walk. */
    if (ndsMenuShellWalkWantsResultsStart() != 0u)
    {
        held |= KEY_START;
    }
#endif
    sHeldKeys = held;
    gNdsPlatformHeldKeys = held;

#if NDS_R2_CAMERA_FIXED_TOGGLE
    /* SELECT is the only key the battle leaves unbound, so binding it here
     * cannot shadow a real input. keysDown() is READ, never re-scanned: the
     * single scanKeys() above is the frame's only scan and a second one would
     * eat the edge ndsControllerLiveButtons depends on. */
    if ((keysDown() & KEY_SELECT) != 0)
    {
        gNdsR2CameraFixedEnabled = (gNdsR2CameraFixedEnabled != 0u) ? 0u : 1u;
    }
#endif
#if NDS_LAB_NO_CULL
    /* BUGS.md #10 / P2-3r17 seam probe. Same key, same read-not-rescan rule as
     * the camera toggle above; the two are mutually exclusive by the #error in
     * the HUD block, so SELECT still has exactly one meaning per build. */
    if ((keysDown() & KEY_SELECT) != 0)
    {
        (void)ndsRendererLabSeamAdvanceArm();
    }
#endif

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

s32 ndsPlatformReserveOriginalSpritePreview(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    if ((sOriginalSpritePreview == NULL) ||
        (sOriginalSpritePreviewGeneration != gNdsTaskmanHeapGeneration))
    {
        sOriginalSpritePreviewWidth = 0u;
        sOriginalSpritePreviewHeight = 0u;
        sOriginalSpritePreviewReady = 0u;
        gNdsOriginalSpritePreviewReady = 0u;
        gNdsOriginalSpritePreviewDisplayWidth = 0u;
        gNdsOriginalSpritePreviewDisplayHeight = 0u;
        sOriginalSpritePreview = NULL;
        if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH *
                NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT * sizeof(u16), 4u) == FALSE)
        {
            return FALSE;
        }
        sOriginalSpritePreview = syTaskmanMalloc(
            NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH *
            NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT * sizeof(u16), 4u);
        if (sOriginalSpritePreview == NULL)
        {
            return FALSE;
        }
        sOriginalSpritePreviewGeneration = gNdsTaskmanHeapGeneration;
    }
#endif
    return TRUE;
}

u16 *ndsPlatformBeginOriginalSpritePreview(u32 width, u32 height,
                                           s32 n64_x, s32 n64_y,
                                           u32 *out_pitch)
{
    u32 row;

    if ((width == 0) || (height == 0) ||
        (width > NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH) ||
        (height > NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT) ||
        (ndsPlatformReserveOriginalSpritePreview() == FALSE))
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

u32 ndsPlatformGetOriginalSpritePreviewEpoch(void)
{
    return sOriginalSpriteDecodeCacheEpoch;
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
    if ((display_preview == NULL) ||
        (sOriginalSpritePreviewGeneration != gNdsTaskmanHeapGeneration))
    {
        return;
    }
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

void ndsPlatformSet3DLayerEnabled(s32 is_enabled)
{
#if NDS_RENDERER_HW_TRIANGLES
    /* MODE_5_3D presents the geometry engine through main BG0. Unlike a
     * software framebuffer, that layer retains the last completed 3D frame
     * when a scene submits no new geometry. Hide the display owner rather than
     * manufacturing a dummy GX frame just to erase stale pixels.
     *
     * ENABLING IS DEFERRED TO THE NEXT PRESENTED FRAME (2026-08-21). Showing
     * BG0 the moment a scene asks exposes whatever the PREVIOUS 3D owner last
     * rendered: VSBattle reclaimed the layer at scene entry, so the whole
     * asset-load window composited the character select's retained fighter
     * previews -- the owner's "fighters appear the instant a stage is chosen".
     * Arming here and committing in ndsPlatformEndFrame, only after a real GX
     * frame was submitted and flushed, means every enable shows that scene's
     * own first frame or nothing at all. A scene that never presents keeps the
     * layer hidden, which is also the safer failure. */
    if (is_enabled != FALSE)
    {
        s3dLayerEnableOnNextPresent = TRUE;
        return;
    }
    s3dLayerEnableOnNextPresent = FALSE;
    REG_DISPCNT &= ~DISPLAY_BG0_ACTIVE;
#else
    (void)is_enabled;
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static s32 ndsPlatformScaleSourceViewportEdge(s32 value, s32 limit)
{
    s32 scaled = (value * 4 + 2) / 5;

    if (scaled < 0)
    {
        scaled = 0;
    }
    if (scaled > limit)
    {
        scaled = limit;
    }
    return scaled;
}
#endif

/* P2-3r7. GFX_VIEWPORT IS A GX COMMAND, SO THESE TWO ARE FIFO WRITERS.
 *
 * P2-2p3 made the fighter packet replay start its DMA into GFX_FIFO and return
 * without waiting, on the contract that "whoever writes the FIFO next waits
 * first" -- every renderer seam calls ndsRendererFighterPacketDmaWait. These
 * two helpers were added later, for the character select's source viewport, and
 * they bypassed that contract: ndsMNPlayersVSPreviewFrame calls
 * ndsPlatformReset3DViewport IMMEDIATELY after gcDrawAll, so the glViewport
 * word landed in the FIFO in the middle of the last preview fighter's still-
 * draining packet. The geometry engine then read it as a parameter of whatever
 * command was open and every following word shifted, which is why the preview
 * drew with its head, cap, gloves and legs at other joints' positions -- and
 * why only the character select was affected: it is the only caller.
 *
 * Measured on build-p2-shell, same ROM, one poked bit: holding
 * sNdsFighterPackets[].valid at 0 (every draw records, nothing replays) drew
 * the preview assembled, while the replay drew it in pieces; two record frames
 * one draw apart differed in 105 words, ALL of them the replay's own patch
 * sites, so the recorded stream was never the problem -- its delivery was. */
void ndsPlatformSet3DViewportSource(s32 ulx, s32 uly, s32 lrx, s32 lry)
{
#if NDS_RENDERER_HW_TRIANGLES
    s32 x0 = ndsPlatformScaleSourceViewportEdge(ulx, SCREEN_WIDTH);
    s32 y0 = ndsPlatformScaleSourceViewportEdge(uly, SCREEN_HEIGHT);
    s32 x1 = ndsPlatformScaleSourceViewportEdge(lrx, SCREEN_WIDTH) - 1;
    s32 y1 = ndsPlatformScaleSourceViewportEdge(lry, SCREEN_HEIGHT) - 1;

    if (x1 < x0)
    {
        x1 = x0;
    }
    if (y1 < y0)
    {
        y1 = y0;
    }
    if (x1 >= SCREEN_WIDTH)
    {
        x1 = SCREEN_WIDTH - 1;
    }
    if (y1 >= SCREEN_HEIGHT)
    {
        y1 = SCREEN_HEIGHT - 1;
    }
    ndsRendererFighterPacketDmaWait();
    glViewport(x0, y0, x1, y1);
#else
    (void)ulx;
    (void)uly;
    (void)lrx;
    (void)lry;
#endif
}

void ndsPlatformReset3DViewport(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererFighterPacketDmaWait();
    glViewport(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
#endif
}

/* P2-1h. Apply BG2's queued affine NOW instead of at the next present.
 *
 * The fast wallpaper leaves BG2 under the battle's own 4/5 transform, and
 * clearing the layer only QUEUES the identity reset -- ndsPlatformEndFrame
 * commits it. A menu that draws backdrop art between the clear and the first
 * present would therefore show one frame of that art scaled by whatever the
 * last battle left behind. This is the same commit, called early; it is
 * idempotent, because the commit clears its own pending flag. */
void ndsPlatformCommitOriginalSpriteOverlayTransform(void)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_FAST_WALLPAPER_AFFINE
    ndsPlatformFastWallpaperCommitAffine();
#endif
}

/* P2-1i -- BG3 as the title's fire layer. See the header for why this is an
 * affine and not a blit.
 *
 * PA/PD are the SOURCE step per screen pixel in 8.8, so a cell 51 wide across
 * a 256 px screen is PA = 51 and a cell 42 tall across 192 rows is PD = 56.
 * PB/PC are zero: the source's fire is axis-aligned. */
void ndsPlatformSetTitleFireEnabled(s32 is_enabled, s32 pa, s32 pd)
{
#if NDS_RENDERER_HW_TRIANGLES
    if (sOriginalSpriteOverlayForegroundBg < 0)
    {
        return;
    }
    if (is_enabled != FALSE)
    {
        /* BEHIND BG2, which is where the fire belongs: the title's wordmark
         * and copyright line are BG2 surfaces. The translucent red emblem is
         * a priority-3 bitmap OBJ between those words and this BG3 fire, which
         * mirrors the source's link-0 emblem / link-1 words ordering. */
        bgSetPriority(sOriginalSpriteOverlayForegroundBg, 3);
        /* Bitmap-OBJ alpha only blends against an admitted second target.
         * The standing battle blend admits BG2 for translucent BG0; the title
         * emblem sits over BG3 instead. Without this, OAM still contains alpha
         * 5 but the red logo composites opaquely over the fire. */
        REG_BLDCNT |= BLEND_DST_BG3;
        /* WRAP IS DELIBERATELY NOT TOUCHED. It would be a reasonable safety
         * net, but there is no bgGetWrap to restore it with, and the affine
         * provably cannot sample outside the sheet: the largest source
         * coordinate is ((255 * PA) >> 8, (191 * PD) >> 8) = (50, 41) plus a
         * cell origin of at most (204, 210), i.e. (254, 251), inside the
         * 255x252 atlas. Leaving it alone is what makes "BG3 is handed back
         * exactly as it was found" a property of this function rather than a
         * claim about the battle's tolerance. */
        sTitleFirePa = pa;
        sTitleFirePd = pd;
        bgSetAffineMatrixScroll(sOriginalSpriteOverlayForegroundBg,
                                pa, 0, 0, pd, 0, 0);
        gNdsTitleFireEnableCount++;
    }
    else
    {
        sTitleFirePa = 1 << 8;
        sTitleFirePd = 1 << 8;
        bgSetAffineMatrixScroll(sOriginalSpriteOverlayForegroundBg,
                                1 << 8, 0, 0, 1 << 8, 0, 0);
        bgSetPriority(sOriginalSpriteOverlayForegroundBg, 0);
        /* BG3 is a title-only blend destination; restore the battle contract. */
        REG_BLDCNT &= (u16)~BLEND_DST_BG3;
        gNdsTitleFireDisableCount++;
    }
#else
    (void)is_enabled;
    (void)pa;
    (void)pd;
#endif
}

void ndsPlatformSetTitleFireFrame(s32 atlas_x, s32 atlas_y)
{
#if NDS_RENDERER_HW_TRIANGLES
    if (sOriginalSpriteOverlayForegroundBg < 0)
    {
        return;
    }
    /* The reference point is 20.8, so a whole-texel cell origin is << 8.
     * `bgSetAffineMatrixScroll` and not `bgSetScrollf`: the latter only marks
     * the shadow dirty, and the `bgUpdate` that would flush it REBUILDS the
     * matrix from libnds' own angle/scale state -- which would throw the
     * upscale away on the first animation frame. This writes the six
     * registers and nothing else, so an animation frame costs no pixels. */
    bgSetAffineMatrixScroll(sOriginalSpriteOverlayForegroundBg,
                            sTitleFirePa, 0, 0, sTitleFirePd,
                            atlas_x << 8, atlas_y << 8);
    gNdsTitleFireFrameCount++;
#else
    (void)atlas_x;
    (void)atlas_y;
#endif
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
#if !NDS_RENDERER_HW_TRIANGLES
    memset(sOriginalSpritePreview, 0, sizeof(sOriginalSpritePreview));
    memset(sOriginalSpriteDisplayPreview, 0,
           sizeof(sOriginalSpriteDisplayPreview));
#endif
    sOriginalSpriteDecodeCacheEpoch++;
    if (sOriginalSpriteDecodeCacheEpoch == 0u)
    {
        sOriginalSpriteDecodeCacheEpoch = 1u;
    }
}

u16 *ndsPlatformBeginOriginalDLPreview(u32 width, u32 height, u32 *out_pitch)
{
#if NDS_RENDERER_HW_TRIANGLES
    /* No consumer exists in this configuration, so there is no buffer to hand
     * back. NULL is the caller's already-supported answer, not a new failure
     * mode: ndsOpeningRoomRenderDLPreview sets
     * NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_NO_PIXELS and returns. Refusing here
     * also drops the opening room's rasterisation into a buffer nobody reads. */
    (void)width;
    (void)height;
    (void)out_pitch;
    /* These five stores exist to keep the diagnostic globals LINKED, and they
     * are the reason this is a store rather than an attribute. `used` only
     * stops the compiler discarding them -- the object file still emitted each
     * one into its own `.bss.gNds...` section -- and `retain` was accepted
     * silently without setting SHF_GNU_RETAIN, so `--gc-sections` dropped all
     * five either way. A real reference from a still-linked function is what
     * the linker actually honours.
     *
     * Five harnesses resolve these by name and fail hard when they are absent:
     * that is how Boundary went RED on "Missing ELF symbol
     * gNdsOriginalDLPreviewReady" with nothing behaviourally wrong. See
     * verify-runtime.ps1 (OPENING_ROOM_DL_PREVIEW_PRESENT, PERF_CONTENT),
     * verify-battle-mariofox-gcrunall-loop-harness.ps1 (PLATFORM_DL_PREVIEW)
     * and sample-runtime-speed.ps1.
     *
     * Zero runtime cost: nothing calls this on a hwtri build -- the opening
     * room is the only caller and it never runs, which is exactly what
     * gNdsOriginalDLPreviewCommitCount == 0 measured before the change. The
     * harnesses therefore read the .bss zeroes, which is the correct answer and
     * now guaranteed structurally rather than by luck. */
    gNdsOriginalDLPreviewReady = 0u;
    gNdsOriginalDLPreviewWidth = 0u;
    gNdsOriginalDLPreviewHeight = 0u;
    gNdsOriginalDLPreviewCommitCount = 0u;
    gNdsOriginalDLPreviewDrawCount = 0u;
    return NULL;
#else
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
#endif
}

#if !NDS_RENDERER_HW_TRIANGLES
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
#endif

void ndsPlatformCommitOriginalDLPreview(void)
{
#if !NDS_RENDERER_HW_TRIANGLES
    if ((sOriginalDLPreviewWidth != 0) && (sOriginalDLPreviewHeight != 0))
    {
        ndsPlatformBuildOriginalDLDisplayPreview();
        sOriginalDLPreviewReady = 1;
        gNdsOriginalDLPreviewReady = 1;
        gNdsOriginalDLPreviewCommitCount++;
    }
#endif
}

void ndsPlatformClearOriginalDLPreview(void)
{
#if !NDS_RENDERER_HW_TRIANGLES
    sOriginalDLPreviewWidth = 0;
    sOriginalDLPreviewHeight = 0;
    sOriginalDLDisplayPreviewWidth = 0;
    sOriginalDLDisplayPreviewHeight = 0;
    sOriginalDLPreviewReady = 0;
    gNdsOriginalDLPreviewReady = 0;
    gNdsOriginalDLPreviewWidth = 0;
    gNdsOriginalDLPreviewHeight = 0;
    memset(sOriginalDLPreview, 0, sizeof(sOriginalDLPreview));
#endif
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

/* vsniprintf, NOT vsnprintf. All 73 call sites format integers and strings --
 * the HUD's "FPS 24.1" is already assembled from integer tenths, and there is
 * not one %f/%g/%e among them -- but newlib picks the formatter from the symbol,
 * not from the format string, so the float one was being linked for nothing.
 * See the comment in nds_reloc_assets.c: it costs 31,555 bytes of image, which
 * is seven taskman arena steps on a target where eight decide whether the
 * battle boots at all. */
static void ndsPlatformPrintDebugLine(u32 row, const char *format, ...)
{
    char line[32];
    va_list args;

    va_start(args, format);
    vsniprintf(line, sizeof(line), format, args);
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
#if NDS_BATTLE_TICK_HUD_ENABLED
/* The tick HUD reports P50 and P95, not latest-and-mean. Per-frame buckets here
 * are heavily skewed: audio refill and HUD text redraw fire on a minority of
 * frames, so a mean measures how many bursts a window happened to contain
 * rather than what a frame costs. Measured 2026-07-22, HUD p50 was 1,024 ticks
 * against a max of 200,256, and fighter cost read 9.6% below retail as a mean
 * but 4.4% below as a P50. docs/VERIFYING.md already made P50/P95 the decision
 * basis; this makes the device HUD report the same statistic.
 *
 * A window of raw samples is kept and the order statistics are recomputed once
 * per refresh interval instead of every frame. That is strictly cheaper than
 * what it replaces: nine iprintf calls per presented frame become nine per
 * refresh, so the HUD now perturbs the loop it measures far less. */
#define NDS_TICK_HUD_WINDOW 128u
/* volatile: the GDB sampler reads this ring out of the ELF, and with
 * NDS_TICK_HUD_DRAW=0 nothing in the ROM reads it, so dead-store elimination
 * plus --gc-sections deleted the array outright and the sampler failed with
 * "Attempt to take address of value not located in memory". A measurement
 * buffer whose only consumer is a debugger has to say so. */
static volatile u32
    sBattleTickHudRing[nNDSTickHudBucketCount][NDS_TICK_HUD_WINDOW];
static u32 sBattleTickHudScratch[NDS_TICK_HUD_WINDOW];
static u32 sBattleTickHudRingHead;
static u32 sBattleTickHudRingCount;
#if NDS_TASK68_FALLBACK_CENSUS || NDS_TASK75_LOAD_CENSUS
/* Task 70. The native-owner counters are cumulative, and a run-level total
 * cannot say whether the frames that fell back are the frames that cost the
 * P95 or merely as numerous as them. Ringing the per-frame delta alongside the
 * buckets puts both on the same index, so the two medians can be compared
 * directly instead of correlated across separate runs.
 *
 * volatile because nothing in the ROM ever reads it: the bucket ring survives
 * because the HUD computes percentiles from it, but a static array that is only
 * ever stored to is a dead store and GCC deletes it outright -- the first build
 * linked with no such symbol at all.
 *
 * Task 75 E0 rides the same ring rather than adding a second one. The two
 * censuses answer the same shape of question about different counters and are
 * never built together, so one ring and one selected source keeps
 * scripts/sample-tick-hud-buckets.ps1 unchanged -- its two-stop baseline path
 * is proven and editing a .ps1 has corrupted these files before. Which counter
 * a dump holds is a property of the build, and each task's document records it. */
#if NDS_TASK75_LOAD_CENSUS
#define NDS_TICK_HUD_CENSUS_RING_SOURCE gNdsTask75AssetLoadCount
#else
#define NDS_TICK_HUD_CENSUS_RING_SOURCE gNdsTickHudNativeOwnerFallbackCount
#endif
static volatile u32 sBattleTickHudFallbackRing[NDS_TICK_HUD_WINDOW];
static u32 sBattleTickHudFallbackPrev;
#endif
/* The values last printed. Retained so the HUD can be asserted over GDB
 * instead of only photographed - scripts/sample-tick-hud-buckets.ps1 computes
 * the same order statistics from the raw buckets and the two must agree. */
static u32 sBattleTickHudP50[nNDSTickHudBucketCount];
static u32 sBattleTickHudP95[nNDSTickHudBucketCount];
static const char *const sBattleTickHudNames[nNDSTickHudBucketCount] = {
    "ALL ", "FTR ", "STG ", "BG  ", "AUD ", "HUD ", "SRC ",
    "MISC", "OTHR", "WAIT", "WORK", "SHDT", "SWRM",
    "GCRA", "SCPU", "SCAT", "SPRM",
    /* Cycle 92 SGCO split. This array is sized by nNDSTickHudBucketCount, so a
     * bucket added without a name here leaves a NULL the HUD would dereference;
     * it must move with the enum. */
    "SINT", "SPHD", "SPHC"
};

/* Shell sort, Knuth gaps: no recursion, no allocation, and no worst case that
 * can stall a frame the way a naive quicksort pivot can. It runs once per
 * refresh window over 128 u32, which is far below the cost of the console
 * writes it shares that frame with. */
static void ndsPlatformTickHudSort(u32 *values, u32 count)
{
    u32 gap = 1u;

    while (gap < (count / 3u))
    {
        gap = (gap * 3u) + 1u;
    }
    while (gap != 0u)
    {
        u32 i;

        for (i = gap; i < count; i++)
        {
            u32 value = values[i];
            u32 j = i;

            while ((j >= gap) && (values[j - gap] > value))
            {
                values[j] = values[j - gap];
                j -= gap;
            }
            values[j] = value;
        }
        gap = (gap - 1u) / 3u;
    }
}

/* floor((count - 1) * percent / 100), byte-for-byte the index used by
 * scripts/sample-tick-hud-buckets.ps1, so the device HUD and the GDB sampler
 * report the same number for the same window instead of two near-misses. */
static u32 ndsPlatformTickHudPercentile(const u32 *sorted, u32 count,
                                        u32 percent)
{
    return sorted[((count - 1u) * percent) / 100u];
}
#endif

/* Emitted only when the tick HUD is compiled in. An empty external-linkage
 * function still occupies text and shifts layout, which changed the published
 * lean ROM's SHA-256 for a diagnostic it does not contain; the only caller is
 * likewise under NDS_TICK_HUD. The inner guard keeps the symbol resolvable when
 * NDS_TICK_HUD is set but the FPS HUD it draws into is not enabled. */
#if NDS_TICK_HUD
void ndsPlatformTickHudReset(void)
{
#if NDS_BATTLE_TICK_HUD_ENABLED
    /* The sampling epoch starts in ndsBattlePlayablePacingStart(), before the
     * first presented iteration.  This used to be cleared lazily by the first
     * FPS-HUD refresh instead, after ndsPlatformTickHudSample() had already
     * written several real battle frames.  The repeated-ring verifier then
     * asked for frame 1 but its oldest surviving row began around frame 12.
     * Reset at the pacing boundary so ring identity and the guest's published
     * PresentedFrames counter have the same lifetime. */
    sBattleTickHudRingHead = 0u;
    sBattleTickHudRingCount = 0u;
    memset(sBattleTickHudP50, 0, sizeof(sBattleTickHudP50));
    memset(sBattleTickHudP95, 0, sizeof(sBattleTickHudP95));
#if NDS_TASK68_FALLBACK_CENSUS || NDS_TASK75_LOAD_CENSUS
    sBattleTickHudFallbackPrev = NDS_TICK_HUD_CENSUS_RING_SOURCE;
#endif
#endif
}

void ndsPlatformTickHudSample(void)
{
#if NDS_BATTLE_TICK_HUD_ENABLED
    u32 bucket;
    u32 head = sBattleTickHudRingHead;

    for (bucket = 0u; bucket < nNDSTickHudBucketCount; bucket++)
    {
        sBattleTickHudRing[bucket][head] = gNdsTickHudBuckets[bucket];
    }
#if NDS_TASK68_FALLBACK_CENSUS || NDS_TASK75_LOAD_CENSUS
    {
        u32 total = NDS_TICK_HUD_CENSUS_RING_SOURCE;

        sBattleTickHudFallbackRing[head] = total - sBattleTickHudFallbackPrev;
        sBattleTickHudFallbackPrev = total;
    }
#endif
    sBattleTickHudRingHead = (head + 1u) % NDS_TICK_HUD_WINDOW;
    if (sBattleTickHudRingCount < NDS_TICK_HUD_WINDOW)
    {
        sBattleTickHudRingCount++;
    }
#endif
}
#else
void ndsPlatformTickHudReset(void)
{
}
#endif

/* THE FPS-HUD GROUP HAS EXACTLY ONE CONSUMER AND IT CANNOT SEE THE D-CACHE.
 *
 * These four globals are written by the ROM and read only by GDB. melonDS's
 * stub reads them through `ARMv5::ReadMem` (`src/ARM.cpp:1545`), which special-
 * cases ITCM and DTCM and otherwise falls through to `ARM::ReadMem` ->
 * `BusRead32` -- there is no DCache lookup anywhere on that path. So a word
 * still sitting dirty in the ARM9 data cache reads STALE over GDB, and a
 * publication that leaves only PART of the group in main RAM is observed torn.
 *
 * That is exactly what R2-04 E2 has been reporting since 2026-08-01. Measured
 * 2026-08-15 (`artifacts/verification/2026-08-15_fpshud-publication.txt`, 420
 * consecutive presented frames on the flag-1 arm): X10 changes on one presented
 * frame and SampleCount/FrameWindow/TickWindow on the NEXT one, three times in
 * 29 publications. The compiled sequence says why -- one store precedes the
 * SampleCount read-modify-write:
 *
 *   2008028:  str r4,[r2]   X10          <- write MISS (ARM946E-S does not
 *                                           write-allocate) -> reaches RAM
 *   200802a:  ldr r2,[r1]   SampleCount  <- LINEFILL of that same 32-byte line
 *   200802e:  str r2,[r1]   SampleCount  <- write HIT: marks the line dirty and
 *   2008034:  str r1,[r2]   FrameWindow     ABORTS the bus write, so main RAM
 *   200803a:  str r1,[r2]   TickWindow      keeps the PREVIOUS sample's words
 *
 * All four live in one 32-byte line (0x02104f40..0x02104f4c), so a straddle was
 * never the mechanism and neither was a second writer: the write is coherent,
 * the READ is not. Cleaning the line publishes the whole group.
 *
 * The group list and the rule are now in nds_platform.h
 * (NDS_BATTLE_FPS_HUD_GROUP / NDS_PUBLISH_DEBUGGER_GROUP), because this is the
 * third group to need it. The measurement above stays here: it is this group's
 * evidence, not doctrine. */
static void ndsPlatformPublishBattleFpsHudGroup(void)
{
    NDS_PUBLISH_DEBUGGER_GROUP(NDS_BATTLE_FPS_HUD_GROUP);
}

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
#if NDS_R2_CAMERA_FIXED_TOGGLE
        /* consoleClear() below wipes row 3 with the rest; the block at the end
         * of this same call repaints it. */
        sBattleCameraArmPrinted = 0xffffffffu;
#endif
#if NDS_LAB_NO_CULL
        sBattleSeamArmPrinted = 0xffffffffu;
#endif
        gNdsBattlePlayableHudFpsX10 = 0u;
        gNdsBattlePlayableHudFpsSampleCount = 0u;
        gNdsBattlePlayableHudFpsFrameWindow = 0u;
        gNdsBattlePlayableHudFpsTickWindow = 0u;
        ndsPlatformPublishBattleFpsHudGroup();
        consoleClear();
        ndsPlatformPrintDebugLine(0u, "FPS --.-  UP --.-");
#if NDS_BATTLE_PHASE_HUD_ENABLED
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        ndsPlatformPrintDebugLine(11u, "FX 0/0/0 E0 A0");
#endif
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
        ndsPlatformPrintDebugLine(21u, "VBI  --  --  --");
        ndsPlatformPrintDebugLine(22u, "5+  --  max --  BGM --/--");
        ndsPlatformPrintDebugLine(
#if NDS_TASK36_HW_COMPOSE == 2
            23u, "GIT %s H%lu P%lu F%lu A%lx S%u", NDS_TASK10_GIT_SHORT,
            (unsigned long)gNdsRendererTask36HardwareComposedDObjCount,
            (unsigned long)gNdsRendererTask36ReplaySegmentCount,
            (unsigned long)gNdsRendererTask36ReplayFallbackCount,
            (unsigned long)(gNdsTaskmanArenaChosenSize >> 12),
            (unsigned int)NDS_TASK44_STAGE_STEADY);
#elif NDS_TASK36_HW_COMPOSE
            23u, "GIT %s HC%lu R%lu F%lu", NDS_TASK10_GIT_SHORT,
            (unsigned long)gNdsRendererTask36HardwareComposedDObjCount,
            (unsigned long)(gNdsRendererTask36AdapterRejectReason |
                            gNdsRendererTask36RendererRejectReason |
                            gNdsRendererTask36PrepareRunRejectReason),
            (unsigned long)gNdsRendererM3PostArmFailureCount);
#else
            23u, "GIT %s DHT %u", NDS_TASK10_GIT_SHORT,
            (unsigned int)NDS_TASK32_DRAW_HOT_TEXT);
#endif
#endif
#if NDS_BATTLE_TICK_HUD_ENABLED
        /* Ring lifetime is owned by ndsBattlePlayablePacingStart(), before the
         * first sample can be written.  Do not clear it here: this HUD path is
         * lazy and may first run several presented frames into the match. */
        {
            u32 bucket;

            for (bucket = 0u; bucket < nNDSTickHudBucketDisplayCount; bucket++)
            {
                ndsPlatformPrintDebugLine(11u + bucket, "%s      --       --",
                                          sBattleTickHudNames[bucket]);
            }
        }
        /* Task 66 took the row that used to hold the column legend. WORK is the
         * number the milestone is judged on (PROJECT_GOAL.md: P95 <= 1.12M) and
         * it earns the last row the console has; the two columns are still P50
         * then P95, as every row above it. n is the live window depth, so a
         * reading taken before it reaches 128 is visibly partial. */
        ndsPlatformPrintDebugLine(20u, "WORK      --       -- n:0");
        ndsPlatformPrintDebugLine(21u, "VBI  --  --  --");
        ndsPlatformPrintDebugLine(22u, "5+  --  max --");
        ndsPlatformPrintDebugLine(23u, "GIT %s TICKHUD", NDS_TASK10_GIT_SHORT);
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
    /* PUBLISHED UNDER A CRITICAL SECTION, and the reason is a two-year-old
     * verifier failure rather than caution.
     *
     * `battle_playable lower-screen rolling FPS counter did not sample actual
     * presentation cadence` asserts that fps_x10 equals a recompute from the
     * two windows published beside it. R2-04 E2 refuted the two obvious
     * explanations -- a stale harness BUS_CLOCK (the shadow proves the constant
     * is 33,513,982 on both sides) and a non-stationary frame rate (these four
     * values come from the same locals in the same breath, so a rolling-versus-
     * spot mismatch cannot arise) -- and left it recorded as intermittent and
     * unexplained.
     *
     * RETRACTED 2026-08-15: the IRQ-tearing explanation this comment used to
     * carry was wrong, and so was the direction it inferred. An IRQ cannot land
     * inside this block, and measurement says X10 LEADS the other three by one
     * publication rather than lagging them. The mechanism is the D-cache, not
     * the write -- see ndsPlatformPublishBattleFpsHudGroup() above, which is
     * what actually repairs it. The critical section stays because a group
     * meant to be read as a group should still be written as one.
     *
     * Two register writes once per ~0.5 s window. */
    {
        unsigned int ime = REG_IME;

        REG_IME = 0u;
        gNdsBattlePlayableHudFpsX10 = fps_x10;
        gNdsBattlePlayableHudFpsSampleCount++;
        gNdsBattlePlayableHudFpsFrameWindow = elapsed_frames;
        gNdsBattlePlayableHudFpsTickWindow = elapsed_ticks;
        REG_IME = ime;
    }
    ndsPlatformPublishBattleFpsHudGroup();
#if NDS_R204_FPSHUD_SHADOW
    /* R2-04 E2. The Boundary assert recomputes fps from the frame/tick window
     * published beside it and found 290 against 15/17,485,504, which is 288.
     * These four stores are adjacent and the values are locals, so the group
     * cannot be internally inconsistent at this instant -- yet the harness reads
     * one that is. This shadow is written in the same breath from the same
     * locals. If the shadow stays self-consistent while the primary does not,
     * something rewrites the primary after this point; if both disagree with the
     * harness, its BUS_CLOCK constant is the wrong one. */
    gNdsR204FpsHudShadowX10 = fps_x10;
    gNdsR204FpsHudShadowFrames = elapsed_frames;
    gNdsR204FpsHudShadowTicks = elapsed_ticks;
    gNdsR204FpsHudShadowBusClock = (u32)BUS_CLOCK;
#endif
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
            11u, "FX %lu/%lu/%lu E%lx A%lu",
            (unsigned long)gNdsTask39FxSpawnTicks,
            (unsigned long)gNdsTask39FxUpdateTicks,
            (unsigned long)gNdsTask39FxDrawTicks,
            (unsigned long)gNdsTask39FxEngagementMask,
            (unsigned long)gNdsTask39FxArenaRejectCount);
#endif
#if NDS_FAST_WALLPAPER_AFFINE
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
#if NDS_TASK36_HW_COMPOSE == 2
    ndsPlatformPrintDebugLine(
        23u, "GIT %s H%lu P%lu F%lu A%lx S%u", NDS_TASK10_GIT_SHORT,
        (unsigned long)gNdsRendererTask36HardwareComposedDObjCount,
        (unsigned long)gNdsRendererTask36ReplaySegmentCount,
        (unsigned long)gNdsRendererTask36ReplayFallbackCount,
        (unsigned long)(gNdsTaskmanArenaChosenSize >> 12),
        (unsigned int)NDS_TASK44_STAGE_STEADY);
#elif NDS_TASK36_HW_COMPOSE
    ndsPlatformPrintDebugLine(
        23u, "GIT %s HC%lu R%lu F%lu", NDS_TASK10_GIT_SHORT,
        (unsigned long)gNdsRendererTask36HardwareComposedDObjCount,
        (unsigned long)(gNdsRendererTask36AdapterRejectReason |
                        gNdsRendererTask36RendererRejectReason |
                        gNdsRendererTask36PrepareRunRejectReason),
        (unsigned long)gNdsRendererM3PostArmFailureCount);
#endif
#endif
#if NDS_BATTLE_TICK_HUD_DRAW_ENABLED
    {
        u32 bucket;
        u32 count = sBattleTickHudRingCount;

        /* Samples arrive per presented iteration via ndsPlatformTickHudSample.
         * This renderer runs about twice a second, which is both a readable
         * update rate and cheap enough to re-derive all nine order statistics
         * every time rather than hold stale values. Nothing is printed until a
         * sample exists; the percentile index would underflow at count 0. */
        if (count != 0u)
        {
            for (bucket = 0u; bucket < nNDSTickHudBucketCount; bucket++)
            {
                memcpy(sBattleTickHudScratch, sBattleTickHudRing[bucket],
                       count * sizeof(sBattleTickHudScratch[0]));
                ndsPlatformTickHudSort(sBattleTickHudScratch, count);
                sBattleTickHudP50[bucket] = ndsPlatformTickHudPercentile(
                    sBattleTickHudScratch, count, 50u);
                sBattleTickHudP95[bucket] = ndsPlatformTickHudPercentile(
                    sBattleTickHudScratch, count, 95u);
                /* Every bucket is percentiled -- the GDB sampler asserts
                 * against all of them -- but only the ones with a row get
                 * drawn. WAIT is the one that does not fit; it is ALL minus
                 * WORK, so nothing is actually hidden. */
                if (bucket < (u32)nNDSTickHudBucketDisplayCount)
                {
                    ndsPlatformPrintDebugLine(
                        11u + bucket, "%s%8lu %8lu",
                        sBattleTickHudNames[bucket],
                        (unsigned long)sBattleTickHudP50[bucket],
                        (unsigned long)sBattleTickHudP95[bucket]);
                }
            }
            ndsPlatformPrintDebugLine(
                20u, "WORK%8lu %8lu n:%lu",
                (unsigned long)sBattleTickHudP50[nNDSTickHudBucketWork],
                (unsigned long)sBattleTickHudP95[nNDSTickHudBucketWork],
                (unsigned long)count);
            /* Presentation-interval histogram, cumulative since HUD reset.
             * Device A/B reports read this, never min FPS, because one frame
             * crossing the 4->5 VBlank boundary reads as 12 FPS while the
             * histogram stays continuous. */
            ndsPlatformPrintDebugLine(
                21u, "VBI 2:%-5lu 3:%-5lu 4:%-5lu",
                (unsigned long)gNdsBattlePlayablePacingPresentIntervalBucket[2u],
                (unsigned long)gNdsBattlePlayablePacingPresentIntervalBucket[3u],
                (unsigned long)gNdsBattlePlayablePacingPresentIntervalBucket[4u]);
            ndsPlatformPrintDebugLine(
                22u, "5+:%-5lu max:%lu",
                (unsigned long)gNdsBattlePlayablePacingPresentIntervalBucket[
                    NDS_BATTLE_PLAYABLE_PACING_INTERVAL_BUCKET_5PLUS],
                (unsigned long)gNdsBattlePlayablePacingPresentIntervalMax);
            /* Build/engagement identity. This used to be published only when
             * the profile-1 phase HUD was compiled in, so the profile-0 tick
             * HUD had no way to confirm which ROM was running - which made the
             * S-digit check in a device A/B packet impossible to perform.
             *
             * The phase HUD's H/P/F replay counters cannot appear here: they
             * are declared under NDS_RENDERER_PROFILE_LEVEL == 1 and do not
             * exist in a profile-0 build. Everything printed below is either a
             * compile-time constant or an unguarded global, so this row costs
             * the lean build nothing and cannot go stale against the flags. */
            ndsPlatformPrintDebugLine(
                23u, "GIT %s A%lx S%u C%u L%u", NDS_TASK10_GIT_SHORT,
                (unsigned long)(gNdsTaskmanArenaChosenSize >> 12),
                (unsigned int)NDS_TASK44_STAGE_STEADY,
                (unsigned int)NDS_TASK36_HW_COMPOSE,
                (unsigned int)NDS_TASK37_ITCM_LEAVES);
        }
    }
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
#if NDS_R2_CAMERA_FIXED_TOGGLE
    /* Row 3 is free in this configuration: row 0 is FPS, row 2 TIME, rows 5/6
     * and 9/10 the two players, and rows 11+ belong to the tick HUD, which is
     * compiled out here (NDS_TICK_HUD is 0 on the proof target). */
    {
        u32 arm = (gNdsR2CameraFixedEnabled != 0u) ? 1u : 0u;

        if (arm != sBattleCameraArmPrinted)
        {
            sBattleCameraArmPrinted = arm;
            ndsPlatformPrintDebugLine(
                3u, (arm != 0u) ? "CAM  FIXED Q20.12  [SELECT]"
                                : "CAM  FLOAT shipping[SELECT]");
        }
    }
#endif
#if NDS_LAB_NO_CULL
    /* Row 3, same reasoning as the camera toggle's indicator above. */
    {
    #if NDS_R2_STRIP_ROUTE && (NDS_TASK56_FIGHTER_PRIMITIVES >= 1) && \
        (NDS_RENDERER_PROFILE_LEVEL < 2) && NDS_RENDERER_HW_TRIANGLES
        static const char *const seam_arm_names[5] = {
            "SEAM 0 shipped     [SELECT]",
            "SEAM 1 cull NONE   [SELECT]",
            "SEAM 2 cull FRONT  [SELECT]",
            "SEAM 3 strips off  [SELECT]",
            "SEAM 4 source world[SELECT]"
        };
        const u32 seam_arm_count = 5u;
    #else
        static const char *const seam_arm_names[4] = {
            "SEAM 0 shipped     [SELECT]",
            "SEAM 1 cull NONE   [SELECT]",
            "SEAM 2 cull FRONT  [SELECT]",
            "SEAM 3 source world[SELECT]"
        };
        const u32 seam_arm_count = 4u;
    #endif
        u32 arm = ndsRendererLabSeamArm();

        if (arm != sBattleSeamArmPrinted)
        {
            sBattleSeamArmPrinted = arm;
            ndsPlatformPrintDebugLine(
                3u, "%s", seam_arm_names[arm % seam_arm_count]);
        }
    }
#endif
}
#endif

/* The frame-complete stop's groups, published for the debugger exactly as the
 * FPS-HUD group above is. Unconditional: the realtime harness reads these
 * markers on every configuration, including the published ROM with no HUD
 * compiled in, and every quantity it gates on is a DIFFERENCE between two
 * members -- logicLag, drawLead and phaseLag inside BPLAY_PACE, and
 * taskmanPresentLead across the two groups. taskman_seam.c calls this
 * immediately before ndsBattlePlayableFrameCompleteMarker(), which is where the
 * debugger stops and where every one of those differences is at its resting
 * value. */
void ndsPlatformPublishBattleFrameCompleteGroups(void)
{
    NDS_PUBLISH_DEBUGGER_GROUP(NDS_BATTLE_PLAYABLE_PACING_GROUP);
    NDS_PUBLISH_DEBUGGER_GROUP(NDS_BATTLE_PLAYABLE_PACING_HISTOGRAM_GROUP);
    NDS_PUBLISH_DEBUGGER_GROUP(NDS_GCRUNALL_TASKMAN_GROUP);
#if NDS_TASK68_FALLBACK_CENSUS
    /* Task 68's counters are sampled beside the frame-complete pacing group.
     * Publish them at that same coherent stop so a route proof can compare the
     * frame number, native plan engagement and fallback total without mixing
     * ARM9 D-cache generations.  Lab flag only; shipping builds pay nothing. */
    DC_FlushRange((const void *)&gNdsTickHudNativeOwnerFallbackCount,
                  sizeof(gNdsTickHudNativeOwnerFallbackCount));
    DC_FlushRange((const void *)&gNdsTickHudNativeOwnerFallbackByReason,
                  sizeof(gNdsTickHudNativeOwnerFallbackByReason));
    DC_FlushRange((const void *)&gNdsFtrPlanHit, sizeof(gNdsFtrPlanHit));
#if NDS_R2_FOX_GUN_OVERLAY
    DC_FlushRange((const void *)&gNdsRendererFoxGunDrawCount,
                  sizeof(gNdsRendererFoxGunDrawCount));
    DC_FlushRange((const void *)&gNdsRendererFoxGunTriangleCount,
                  sizeof(gNdsRendererFoxGunTriangleCount));
    DC_FlushRange((const void *)&gNdsRendererFoxGunFailCount,
                  sizeof(gNdsRendererFoxGunFailCount));
    DC_FlushRange((const void *)&gNdsRendererFoxGunBytes,
                  sizeof(gNdsRendererFoxGunBytes));
#endif
#endif
#if NDS_R2_STAGE_ROUTE_PROBE
    ndsRendererPublishStageRouteProbeDiagnostics();
#endif
#if NDS_R2_SIM_MAC_SHADOW
    /* The warm-MAC instrument's counters. A max-deviation counter is written
     * only when a new maximum occurs, which is exactly the access pattern that
     * is still dirty in the D-cache at the run's final stop and reads STALE. */
    NDS_PUBLISH_DEBUGGER_GROUP(NDS_R2_SIM_MAC_GROUP);
#endif
}

static u32 ndsPlatformMixDebugValue(u32 hash, u32 value)
{
    hash ^= value;
    return hash * 16777619u;
}

#if NDS_BATTLE_FPS_HUD_ENABLED
static u32 ndsPlatformBattleHudDisplayDamage(u32 damage)
{
    return (damage > 999u) ? 999u : damage;
}

/* P2-2. The BattleShip HUD state is four-player; the DS lower-screen text HUD
 * is only a presentation sink. Keep these accessors dumb so player identity
 * stays the source slot (0..3), not the Mario/Fox native-render owner (0..1). */
static u32 ndsPlatformBattleHudDamage(u32 player)
{
    switch (player)
    {
    case 0u: return gNdsIFCommonHUDP0DamageCurrent;
    case 1u: return gNdsIFCommonHUDP1DamageCurrent;
    case 2u: return gNdsIFCommonHUDP2DamageCurrent;
    default: return gNdsIFCommonHUDP3DamageCurrent;
    }
}

static u32 ndsPlatformBattleHudStock(u32 player)
{
    switch (player)
    {
    case 0u: return gNdsIFCommonHUDP0LowerStock;
    case 1u: return gNdsIFCommonHUDP1LowerStock;
    case 2u: return gNdsIFCommonHUDP2LowerStock;
    default: return gNdsIFCommonHUDP3LowerStock;
    }
}

static u32 ndsPlatformBattleHudFighterKind(u32 player)
{
    switch (player)
    {
    case 0u: return gNdsIFCommonHUDP0FighterKind;
    case 1u: return gNdsIFCommonHUDP1FighterKind;
    case 2u: return gNdsIFCommonHUDP2FighterKind;
    default: return gNdsIFCommonHUDP3FighterKind;
    }
}

static u32 ndsPlatformBattleHudLevel(u32 player)
{
    switch (player)
    {
    case 0u: return gNdsIFCommonHUDP0Level;
    case 1u: return gNdsIFCommonHUDP1Level;
    case 2u: return gNdsIFCommonHUDP2Level;
    default: return gNdsIFCommonHUDP3Level;
    }
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
    u32 player;

    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDRecordCount != 0u);
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDActivePlayerMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDShowDamageMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsIFCommonHUDCPUPlayerMask);
    for (player = 0u; player < 4u; player++)
    {
        hash = ndsPlatformMixDebugValue(
            hash, ndsPlatformBattleHudFighterKind(player));
        hash = ndsPlatformMixDebugValue(hash, ndsPlatformBattleHudLevel(player));
        hash = ndsPlatformMixDebugValue(
            hash, ndsPlatformBattleHudDisplayDamage(
                      ndsPlatformBattleHudDamage(player)));
        hash = ndsPlatformMixDebugValue(hash, ndsPlatformBattleHudStock(player));
    }
    hash = ndsPlatformMixDebugValue(hash,
                                    ndsPlatformBattleHudDisplaySeconds());
    return hash;
}

static void ndsPlatformRenderBattleTextHud(void)
{
#if NDS_SHIP_TELEMETRY
    u32 fingerprint = ndsPlatformBattleTextHudStateFingerprint();
#endif
    u32 seconds = ndsPlatformBattleHudDisplaySeconds();
    u32 damage[4];
    u32 stock[4];
    u32 player;

    for (player = 0u; player < 4u; player++)
    {
        damage[player] = ndsPlatformBattleHudDisplayDamage(
            ndsPlatformBattleHudDamage(player));
        stock[player] = ndsPlatformBattleHudStock(player);
    }

    /* P2-2: IFCommon still owns every timer/damage/stock update, but the steady
     * presentation now belongs to the sub 2D engine.  Run that sink before the
     * legacy text-telemetry early-outs: costume/flash state is intentionally
     * richer than the old console fingerprint and must not be starved by it.
     *
     * The source hides the whole interface link once the match ends
     * (ifCommonBattleInterfaceProcSet -> ifCommonInterfaceSetGObjFlagsAll(
     * GOBJ_FLAG_HIDDEN), ifcommon.c:2974, and the same call from pause at
     * :2884).  Those hidden gobjs stop running their display callbacks, which
     * is what feeds ndsIFCommonRecordHUDState -- so from game end onward the
     * mirror masks freeze nonzero, and an unconditional render here would
     * first keep the last battle frame through the victory window and then,
     * after the Results seam's clear un-prepared the HUD, re-prepare and
     * redraw it over the Results screen.  Render only while the source
     * interface is visible, and clear on every other frame so the last drawn
     * frame cannot survive.  The predicate lives in the ifCommon bridge; this
     * file stays out of the BattleShip scene include graph
     * (nds_menu_shell.h's header rule). */
    if (ndsIFCommonBattleHudInterfaceVisible() != FALSE)
    {
        ndsBattleHudRender();
    }
    else
    {
        ndsBattleHudClear();
    }

#if NDS_SHIP_TELEMETRY
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
#else
    if ((sBattleTextHudReady != FALSE) &&
        (gNdsBattleTextHudTimeSeconds == seconds) &&
        (gNdsBattleTextHudP0Damage == damage[0]) &&
        (gNdsBattleTextHudP1Damage == damage[1]) &&
        (gNdsBattleTextHudP2Damage == damage[2]) &&
        (gNdsBattleTextHudP3Damage == damage[3]) &&
        (gNdsBattleTextHudP0Stock == stock[0]) &&
        (gNdsBattleTextHudP1Stock == stock[1]) &&
        (gNdsBattleTextHudP2Stock == stock[2]) &&
        (gNdsBattleTextHudP3Stock == stock[3]) &&
        (gNdsBattleTextHudActiveMask == gNdsIFCommonHUDActivePlayerMask) &&
        (gNdsBattleTextHudShowDamageMask ==
            gNdsIFCommonHUDShowDamageMask))
    {
        return;
    }
    sBattleTextHudReady = TRUE;
    #endif
    gNdsBattleTextHudTimeSeconds = seconds;
    gNdsBattleTextHudP0Damage = damage[0];
    gNdsBattleTextHudP1Damage = damage[1];
    gNdsBattleTextHudP2Damage = damage[2];
    gNdsBattleTextHudP3Damage = damage[3];
    gNdsBattleTextHudP0Stock = stock[0];
    gNdsBattleTextHudP1Stock = stock[1];
    gNdsBattleTextHudP2Stock = stock[2];
    gNdsBattleTextHudP3Stock = stock[3];
    gNdsBattleTextHudActiveMask = gNdsIFCommonHUDActivePlayerMask;
    gNdsBattleTextHudShowDamageMask = gNdsIFCommonHUDShowDamageMask;
}
#endif

void ndsPlatformClearBattleTextHud(void)
{
    gNdsBattleTextHudClearCount++;
    ndsBattleHudClear();
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
#if NDS_R2_CAMERA_FIXED_TOGGLE
    sBattleCameraArmPrinted = 0xffffffffu;
#endif
#if NDS_LAB_NO_CULL
    sBattleSeamArmPrinted = 0xffffffffu;
#endif
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

#if NDS_DEBUG_HUD
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
#endif

void ndsPlatformRenderDebugHud(void)
{
#if !NDS_RENDERER_HW_TRIANGLES
    ndsPlatformDrawOriginalDLPreview();
    ndsPlatformDrawOriginalSpritePreview();
    ndsPlatformUpdatePerfCounters();
#endif
#if NDS_BATTLE_FPS_HUD_ENABLED && !NDS_DEBUG_HUD
    if (gNdsBattlePlayablePacingDrawCalls != 0u)
    {
        ndsPlatformRenderBattleFpsHud();
        ndsPlatformRenderBattleTextHud();
    }
#endif
#if NDS_DEBUG_HUD
    u32 debug_text_fingerprint;

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
#endif
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
#if NDS_TICK_HUD
    u32 tickhud_flush_start;
    u32 tickhud_wait_start;
#endif
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
/* NDS_TICK_HUD joined this condition for the slice 43 blink (2026-08-11). The
 * fighters kept submitting a CONSTANT 320/306 triangles a frame while they
 * visibly vanished for one frame, so submission counters could not see the bug
 * at all -- what changes on a blink frame is how much of that survives to
 * POLYGON RAM. These four registers are the difference between "submitted" and
 * "accepted", and every measurement in this campaign runs on the tick-HUD ROM,
 * which was the one build that could not read them.
 *
 * GFX_STATUS is GXSTAT, so this also carries the position/vector matrix stack
 * LEVEL (bits 8..12) and its sticky over/underflow bit (15), and that is the
 * standing lead on the OPEN blink row. Measured over 128 presented frames on a
 * ROM that still blinked: the error bit was set on EVERY frame, the level
 * advanced +3 per frame wrapping mod 32, and every frame where it wrapped to 0
 * was a low-polygon frame -- 449/481/482/513/545 at 145/165/165/106/306 against
 * a 378 median, no exceptions. That is the stack leaking ~3 unbalanced pushes a
 * frame, and its 32-frame wrap is the blink's period.
 *
 * A leaking stack pointer is harmless while nothing keeps live data in the
 * stack, which is why the CPU joint compose is clean and why GX joint compose --
 * which parks parent worlds in absolute MATRIX_STORE levels the pointer walks
 * over -- is not. So the leak most likely PREDATES slice 43, which only made it
 * visible; confirming that needs one instrumented build at GX_COMPOSE=0. Note a
 * glPushMatrix grep cannot balance the books here: the Whispy native path emits
 * raw MATRIX_PUSH/MATRIX_POP words straight into the FIFO, and
 * ndsRendererEndParticleQuads' pop is conditional on two separate flags.
 *
 * Four volatile register reads on the frame the renderer already flushes. It is
 * NOT in the published block on purpose: the cost is negligible but not zero,
 * and the instrument may carry a diagnostic the shipped ROM does not. */
#if NDS_SHIP_TELEMETRY || (NDS_RENDERER_PROFILE_LEVEL >= 1) || NDS_TICK_HUD
        gNdsHardwareRendererPolyRamCount = GFX_POLYGON_RAM_USAGE;
        gNdsHardwareRendererVertexRamCount = GFX_VERTEX_RAM_USAGE;
        gNdsHardwareRendererStatus = GFX_STATUS;
        gNdsHardwareRendererControl = GFX_CONTROL;
#endif
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
#if NDS_TICK_HUD
        tickhud_flush_start = cpuGetTiming();
#endif
        /* P2-2p3: a fighter packet DMA may still be draining into the FIFO;
         * the flush is a FIFO command. Above the Task 29 record on purpose --
         * check-gbi-decode-fixtures pins the record adjacent to the flush. */
        ndsRendererFighterPacketDmaWait();
#if NDS_TASK29_GX_CENSUS
        ndsRendererTask29GXRecordFlush(GL_TRANS_MANUALSORT);
#endif
        glFlush(GL_TRANS_MANUALSORT);
#if NDS_TICK_HUD
        gNdsTickHudFlushTicks += cpuGetTiming() - tickhud_flush_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileFlushTicks = cpuGetTiming() - profile_start;
        gNdsRendererProfileGXStatusAfterFlush = GFX_STATUS;
#endif
        if (submitted != 0u)
        {
            gNdsHardwareRendererFlushCount++;
            /* Commit a deferred 3D-layer enable only now, with this scene's
             * own frame completed: the retained-image hazard this guards is
             * documented at s3dLayerEnableOnNextPresent. */
            if (s3dLayerEnableOnNextPresent != FALSE)
            {
                REG_DISPCNT |= DISPLAY_BG0_ACTIVE;
                s3dLayerEnableOnNextPresent = FALSE;
            }
        }
        else
        {
            /* Overlay-transform-only flush: no 3D frame was submitted, so
             * this is not a hardware frame flush (see the counter's own
             * comment at its definition). */
            gNdsHardwareRendererOverlayOnlyFlushCount++;
        }
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
#if NDS_TICK_HUD
    tickhud_wait_start = cpuGetTiming();
#endif
    ndsPlatformWaitForScheduledVBlank();
#if NDS_TICK_HUD
    gNdsTickHudVBlankWaitTicks += cpuGetTiming() - tickhud_wait_start;
#endif
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
#if NDS_P2_UI_KIT
    /* P2-1c. After the battle's OBJ tenant, because the two share one shadow
     * OAM and the later publisher wins; they are never live in the same scene,
     * so this ordering only decides which one pays for the oamUpdate. */
    ndsUiKitCommit();
#endif
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
#if NDS_TICK_HUD
    tickhud_wait_start = cpuGetTiming();
#endif
    ndsPlatformWaitForScheduledVBlank();
#if NDS_TICK_HUD
    gNdsTickHudVBlankWaitTicks += cpuGetTiming() - tickhud_wait_start;
#endif
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
