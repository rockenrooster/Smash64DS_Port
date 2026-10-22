#include <nds.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <nds/nds_boot.h>
#include <nds/nds_audio_bgm.h>
#include <nds/nds_battle_hud.h>
#include <nds/nds_controller.h>
#include <nds/nds_effects.h>
#include <nds/nds_freeze_diagnostics.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_menu_shell.h>
#include <nds/nds_ui_kit.h>
#include <nds/nds_platform.h>
#include <nds/nds_gx_dma.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/nds_results_oam.h>
#include <nds/nds_source2d.h>
#include <nds/nds_task37_itcm.h>
#include <nds/nds_scene.h>
#include <nds/nds_startup.h>
#include <nds/nds_video.h>
#include <lb/lbfade_ds.h>
#include <sys/controller.h>

/* Per-commit revision telemetry. Lives in its own generated header
 * (nds_build_revision.h) so a new commit rebuilds only its owners instead of
 * every TU force-included with nds_build_config.h. The fallback covers host
 * builds without a generated header; a DS build always has one via the
 * nds_platform.o Makefile edge, so it can never go stale. */
#if defined(__has_include)
#if __has_include("nds_build_revision.h")
#include "nds_build_revision.h"
#endif
#endif
#ifndef NDS_TASK10_GIT_SHORT
#define NDS_TASK10_GIT_SHORT "unknown"
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
#define NDS_MENU_FPS_HUD_SAMPLE_VBLANKS 30u
#define NDS_BATTLE_SOURCE_TICKS_PER_SECOND 60u
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
    (NDS_TICK_HUD && NDS_TICK_HUD_DRAW)
/* P2-2p8 (2026-10-04): the developer FPS console's periodic text (the FPS/UP,
 * SLIP, VBI and GIT rows: about five iprintf lines through the libnds console
 * every half second, +25-40K ticks on one presented frame in fifteen) is A9
 * instrument cost in a scripted measurement: the GDB sampler reads the pacing
 * counters and the tick ring, never the console. NDS_BATTLE_FPS_HUD_DRAW=0
 * (the four-CPU gate target) keeps the sampling, the published FPS group and
 * the one clear at match start, and skips the periodic console writes. */
#if NDS_BATTLE_FPS_HUD_DRAW
#define NDS_BATTLE_FPS_HUD_PRINT(...) ndsPlatformPrintDebugLine(__VA_ARGS__)
#else
#define NDS_BATTLE_FPS_HUD_PRINT(...) ((void)0)
#endif
static u32 sTicks;
static u32 sHeldKeys;
static volatile u32 sVBlankCount;
static u32 sEarliestPresentVBlank;
/* Scene transition hold (ndsPlatformTransitionHoldBegin, below). EXIT: the
 * old scene is tearing down -- its display teardown is recorded, not done.
 * LOAD: the next scene is starting -- its first display write ends the hold.
 * The state is up here because the layer owners above the hold's own code
 * consult it. */
#define NDS_TRANSITION_HOLD_NONE 0u
#define NDS_TRANSITION_HOLD_EXIT 1u
#define NDS_TRANSITION_HOLD_LOAD 2u
static u32 sNdsTransitionHold;
#define NDS_TRANSITION_HOLD_PENDING_TITLE_FIRE_OFF 1u
#define NDS_TRANSITION_HOLD_PENDING_LAYER_MASK 2u
#define NDS_TRANSITION_HOLD_PENDING_BACKDROP 4u
#define NDS_TRANSITION_HOLD_PENDING_OAM_MAIN 8u
#define NDS_TRANSITION_HOLD_PENDING_OAM_SUB 16u
static u32 sNdsTransitionHoldPending;
static u32 sNdsTransitionHoldLayerMask;
static u16 sNdsTransitionHoldBackdrop;
static s32 sNdsTransitionHoldWants3D;
/* Service presents seen during the hold; the cap is its failsafe (600 is ten
 * seconds; SSS -> battle, the longest load, measured 170). */
static u32 sNdsTransitionHoldPresents;
#define NDS_TRANSITION_HOLD_MAX_PRESENTS 600u
/* The transition snapshot (ndsPlatformTransitionSnapshotBegin, below): 0, or
 * 1 + the bank (0 = A, 1 = B: captured into; 2 = C: moved into, see
 * ndsPlatformTransitionSnapshotMoveToC; 3 = D: a battle exit's, see
 * ndsPlatformTransitionSnapshotOverWallpaper) the main screen now shows; the
 * bank's VRAMCNT value to restore. Bg2Touched: the next scene was handed
 * the BG2 bitmap since the capture, so bank C is no longer free to hold it. */
static u32 sNdsTransitionSnapshotBank;
static u8 sNdsTransitionSnapshotBankCr;
static u32 sNdsTransitionSnapshotBg2Touched;
#define NDS_TRANSITION_SNAPSHOT_BANK_C 3u
#define NDS_TRANSITION_SNAPSHOT_BANK_D 4u
static s32 ndsPlatformTransitionSnapshotMoveToC(void);

/* The next scene is about to write the BG2 bitmap: a held frame in bank C
 * gives the bank back first (under the black cover, cleared). */
static void ndsPlatformTransitionSnapshotLeaveBg2(void)
{
    if (sNdsTransitionSnapshotBank == NDS_TRANSITION_SNAPSHOT_BANK_C)
    {
        ndsPlatformTransitionSnapshotAbort();
    }
    sNdsTransitionSnapshotBg2Touched = 1u;
}

/* The next scene is about to write the BG3 bitmap, whose bank D a battle
 * exit's held frame occupies: it ends there, under the black cover. A clear
 * alone waits (nothing of the scene's is lost: BG3 stays hidden and D's
 * return pending until the release). */
static void ndsPlatformTransitionSnapshotLeaveBg3(void)
{
    if (sNdsTransitionSnapshotBank == NDS_TRANSITION_SNAPSHOT_BANK_D)
    {
        ndsPlatformTransitionSnapshotAbort();
    }
}
/* BG2's affine as last written (PA, PD, X, Y; PB = PC = 0 at every write
 * here): a battle exit's held frame redraws the wallpaper through it. */
static s32 sNdsBg2AffineShown[4] = { 1 << 8, 1 << 8, 0, 0 };
/* Runtime A/B word (the control arm is the same binary); counters read by
 * the transition captures. Decline: 1 brightness, 2 display mode, 3 capture
 * busy, 4 no free texture bank, 5 capture timeout, 6 a battle exit's
 * capture timed out. */
volatile u32 gNdsTransitionSnapshotCount;
volatile u32 gNdsTransitionSnapshotReleaseCount;
volatile u32 gNdsTransitionSnapshotAbortCount;
volatile u32 gNdsTransitionSnapshotMoveCount;
volatile u32 gNdsTransitionSnapshotPhotoCount;
volatile u32 gNdsTransitionSnapshotExitCount;
volatile u32 gNdsTransitionSnapshotDecline;
volatile u32 gNdsTransitionSnapshotDeclineCount;
volatile u32 gNdsTransitionHoldCount;
volatile u32 gNdsTransitionThawCount;
/* What ended the last hold (a return address) and when (VBlank counts):
 * read by the transition captures. */
volatile u32 gNdsTransitionThawCaller;
volatile u32 gNdsTransitionHoldVBlank;
volatile u32 gNdsTransitionThawVBlank;
volatile u32 gNdsPlatformHeldKeys;
/* Defined in src/import/battleship_ifcommon.c, beside the HUD mirrors it
 * guards. Declared here rather than in a header because this file must stay
 * out of the BattleShip scene include graph (nds_menu_shell.h's rule). */
extern s32 ndsIFCommonBattleHudInterfaceVisible(void);
static u32 sBattleFpsHudSampleReady;
static u32 sBattleFpsHudLastTick;
static u32 sBattleFpsHudLastPresentedFrames;
static u32 sBattleFpsHudLastLogicFrames;
static u32 sBattleFpsHudPrintedFpsX10 = 0xffffffffu;
static u32 sBattleFpsHudPrintedUpdatesX10 = 0xffffffffu;
static u32 sBattleFpsHudPrintedGx3dFaults = 0xffffffffu;
/* Frames the 3D engine reported a fault on (DISP3DCNT acknowledge bits, read
 * at each flush): its line buffer underflowed -- too much on some scanlines
 * to render in time, which real hardware shows as wrong or blank lines and no
 * emulator models (melonDS answers RDLINES_COUNT with a constant) -- or the
 * polygon/vertex RAM overflowed and dropped geometry. Shown on the battle
 * FPS row when nonzero, so a hardware photo can tell an overload from a
 * drawing bug. */
volatile u32 gNdsGx3dUnderflowFrames;
volatile u32 gNdsGx3dRamOverflowFrames;
volatile u32 gNdsGx3dRdlinesMin = 46u;
#if NDS_P2_MENU_SHELL
static u32 sMenuFpsHudScreen = 0xffffffffu;
static u32 sMenuFpsHudEnterCount = 0xffffffffu;
static u32 sMenuFpsHudLastVBlank;
static u32 sMenuFpsHudLastFrames;
static u32 sMenuFpsHudPrintedFpsX10 = 0xffffffffu;
static u32 sMenuFpsHudPrintedTwoVBlankPct = 0xffffffffu;
static u32 sMenuFpsHudPrintedThreePlus = 0xffffffffu;
static u32 sMenuFpsHudPrintedMax = 0xffffffffu;
static const char *const sMenuFpsHudNames[NDS_MENU_SHELL_SCREEN_COUNT] = {
    "TITLE", "MODE", "VS", "CSS", "SSS", "VSOPT",
    "ITEM", "OPTION", "BACKUP", "DATA", "SOUND", "VSREC"
};
#endif
static u32 sBattleTextHudReady;
static u32 sBattleTextHudFingerprint = 0xffffffffu;
/* The native shell does not rasterize source SObjs. Keep this 150 KiB scratch
 * in the scene that actually uses it, rather than permanently shrinking every
 * scene arena. Hardware commits copy its pixels into BG VRAM immediately. */
static u16 *sOriginalSpritePreview;
static u32 sOriginalSpritePreviewGeneration;
extern volatile u32 gNdsTaskmanHeapGeneration;
extern void *syTaskmanMalloc(size_t size, u32 align);
static u32 sOriginalSpritePreviewWidth;
static u32 sOriginalSpritePreviewHeight;
static s32 sOriginalSpritePreviewX;
static s32 sOriginalSpritePreviewY;
static u32 sOriginalSpritePreviewReady;
static int sOriginalSpriteOverlayBg = -1;
static int sOriginalSpriteOverlayForegroundBg = -1;

/* Every BG2 affine write (PB = PC = 0), recorded in sNdsBg2AffineShown. */
static void ndsPlatformSetBg2Affine(s32 pa, s32 pd, s32 dx, s32 dy)
{
    bgSetAffineMatrixScroll(sOriginalSpriteOverlayBg, pa, 0, 0, pd, dx, dy);
    sNdsBg2AffineShown[0] = pa;
    sNdsBg2AffineShown[1] = pd;
    sNdsBg2AffineShown[2] = dx;
    sNdsBg2AffineShown[3] = dy;
}
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
/* Layers hidden while a picture is written into them (bit 0 BG2, bit 1 BG3);
 * shown again at the next EndFrame commit. */
static u32 sOriginalSpriteOverlayRevealPending;
/* P2-2p8 Phase 1 slice 2b: while a battle that left BG3 empty lends bank D to
 * 3D textures (ndsPlatformVramTakeBankD), the foreground layer is withheld. A
 * scene that asks for it is remembered and gets it back at
 * ndsPlatformVramReturnBankD; any attempt to draw into it meanwhile is refused
 * and counted (gNdsVramBg3RefusedWrites -- the lossless gate: 0). */
static u32 sOriginalSpriteOverlayBg3Lent;
static u32 sOriginalSpriteOverlayBg3Wanted;
/* Battle exit asked for D back; it is remapped once the next scene's first 3D
 * frame is displayed, or at its first BG3 write or clear, whichever comes
 * first (slice 7: a layer-mask request alone no longer returns it). */
static u32 sOriginalSpriteOverlayBg3ReturnPending;
static void ndsPlatformVramReturnBankDNow(void);
/* A non-battle frame while D is still lent means the battle's exit hook was
 * missed; ndsPlatformEndFrame closes the texture side itself (counted). Weak:
 * targets without the scene manager or the renderer's admission link. */
__attribute__((used)) volatile u32 gNdsVramBankDMissedExits;
extern volatile u32 gNdsSceneManagerCurrIsBattle __attribute__((weak));
void ndsFtrLeanAdmitBattleExit(void) __attribute__((weak));
__attribute__((used)) volatile u32 gNdsVramBankDLent;
__attribute__((used)) volatile u32 gNdsVramBg3RefusedWrites;
__attribute__((used)) volatile u32 gNdsVramBankDTakes;
__attribute__((used)) volatile u32 gNdsVramBankDReturns;
/* See ndsPlatformSet3DLayerEnabled: an enable is ARMED here and committed by
 * ndsPlatformEndFrame only after a real GX frame was submitted, so BG0 can
 * never composite the previous 3D owner's retained frame. */
static s32 s3dLayerEnableOnNextPresent;
static u32 sOriginalSpriteOverlayEpoch[2] = { 1u, 1u };
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
volatile u32 gNdsBattlePlayableHudFpsX10;
volatile u32 gNdsBattlePlayableHudFpsSampleCount;
volatile u32 gNdsBattlePlayableHudFpsFrameWindow;
volatile u32 gNdsBattlePlayableHudFpsTickWindow;
#if NDS_P2_MENU_SHELL
volatile u32 gNdsMenuFpsHudRefreshCount __attribute__((used));
#endif
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
static void ndsPlatformVBlankInterrupt(void)
{
    sVBlankCount++;
}

/* The present lead's wake line (ndsPlatformPresentLeadEnable): a handler of
 * its own so the dispatcher acknowledges it; the waiting thread is the
 * point. */
static void ndsPlatformVCountInterrupt(void)
{
}

/* An ARM9 self-restart lived here briefly, as the cheap way past the rematch's
 * second-entry geometry corruption. It is deleted rather than left default-off
 * because it does not work and a dead selector is worse than none. The full
 * measurement and the three toolchain reasons are recorded at the one place a
 * reader would look for them, `ndsMNVSResultsSetLoadScene` in
 * `src/import/battleship_mnvsresults.c`. Short version: calico links no
 * `svcSoftReset`, bare BIOS `swi #0` is measurably a no-op here, and
 * `crt0Startup` needs three loader-supplied arguments that no longer exist. */

/* cpuGetTiming without its call chain (A9: the instrument stays out of the
 * gate). The libnds version is Thumb in main RAM and calls Calico's
 * tickGetCount, which masks IME around five I/O accesses: ~65 ticks a call,
 * ~500 calls in a tick-HUD frame, all inside WORK-H. This returns the same
 * value from the same state -- Calico's TIMER2 count, its overflow count and
 * the pending-overflow rule, minus the same tickRef -- but retries on a
 * changed overflow count instead of masking IRQs, from ARM code in ITCM.
 *
 * Calico keeps the overflow count and tickRef static, so their addresses come
 * from the literal pools of tickGetCount and the real cpuGetTiming, accepted
 * only when the surrounding instructions are the ones decoded below. Anything
 * else leaves the fast path off and every call goes to the real function. */
extern u32 __real_cpuGetTiming(void);
extern u64 tickGetCount(void);
static volatile const u32 *sNdsFastTimingHigh;
static volatile const u32 *sNdsFastTimingRef;
__attribute__((used)) volatile u32 gNdsFastTimingEnabled;
__attribute__((used)) volatile u32 gNdsFastTimingMismatch;

u32 __attribute__((section(".itcm"), target("arm"), noinline))
__wrap_cpuGetTiming(void)
{
    volatile const u32 *high = sNdsFastTimingHigh;
    u32 hi, count, flags;

    if (high == NULL)
    {
        return __real_cpuGetTiming();
    }
    do
    {
        hi = *high;
        count = TIMER_DATA(2);
        flags = REG_IF;
    } while (hi != *high);
    /* An overflow not yet serviced counts once, and only if the count read
     * already wrapped (Calico's rule). */
    hi += ((flags >> 5) & 1u) & ((count ^ 0x8000u) >> 15);
    return (((hi << 16) | count) - *sNdsFastTimingRef) << 6;
}

static void ndsPlatformFastTimingInit(void)
{
    const u32 *tick = (const u32 *)(uintptr_t)tickGetCount;
    const u16 *get = (const u16 *)((uintptr_t)__real_cpuGetTiming & ~1u);
    u32 i;

    /* tickGetCount (ARM): ldr r2,=high; ldr r0,=TIMER0_DATA; ldrh r0,[r0,#8];
     * ldrd r4,[r2]. cpuGetTiming (Thumb): ldr r4,=tickRef; ldr r2,[r4];
     * subs r0,r0,r2; ... lsls r0,r0,#6. */
    if (((uintptr_t)tickGetCount & 3u) != 0u ||
        tick[5] != 0xe59f2038u || tick[6] != 0xe59f0038u ||
        tick[7] != 0xe1d000b8u || tick[8] != 0xe1c240d0u ||
        tick[22] != 0x04000100u ||
        ((uintptr_t)__real_cpuGetTiming & 1u) == 0u ||
        get[4] != 0x4c02u || get[5] != 0x6822u || get[6] != 0x1a80u ||
        get[8] != 0x0180u)
    {
        return;
    }
    /* Thumb ldr literal: Align(PC, 4) + 8, with PC = that instruction + 4. */
    sNdsFastTimingRef = *(volatile const u32 *const *)(
        (((uintptr_t)&get[4] + 4u) & ~(uintptr_t)3u) + 8u);
    sNdsFastTimingHigh = (volatile const u32 *)tick[21];
    /* Bracket the fast value with the real one; any disagreement turns the
     * fast path off for good. */
    for (i = 0u; i < 64u; i++)
    {
        u32 before = __real_cpuGetTiming();
        u32 fast = __wrap_cpuGetTiming();
        u32 after = __real_cpuGetTiming();

        if ((u32)(fast - before) > (u32)(after - before))
        {
            gNdsFastTimingMismatch++;
        }
    }
    if (gNdsFastTimingMismatch != 0u)
    {
        sNdsFastTimingHigh = NULL;
        return;
    }
    gNdsFastTimingEnabled = 1u;
}

/* P2-2p8 S3 (2026-09-28). libnds keeps texture and palette names in
 * DynamicArrays that grow by realloc when a new name reaches their size, and
 * glColorTableEXT stores the new palette index in the texture BEFORE it grows
 * the palette array, ignoring a failed growth. In the published configuration
 * with Captain/Link/Pikachu/Kirby the 257th palette arrived at frame 418 of a
 * VS battle with 953 B left in libc's top chunk: the 2 KB realloc failed, the
 * texture kept palette index 257 with no entry behind it, and the next
 * glBindTexture read the NULL entry. glResetTextures frees the entries but
 * keeps each array's capacity, so growing the arrays once, here, while libc
 * has room, takes the realloc out of every later scene. */
#define NDS_PLATFORM_GL_NAME_TABLE_ENTRIES 512u
__attribute__((used)) volatile u32 gNdsPlatformGlNameTablesReserved;

static void ndsPlatformReserveGlNameTables(void)
{
    const u32 last = NDS_PLATFORM_GL_NAME_TABLE_ENTRIES - 1u;
    u32 ok = 0u;

    ok += (DynamicArraySet(&glGlob->texturePtrs, last, NULL) != false) ? 1u : 0u;
    ok += (DynamicArraySet(&glGlob->palettePtrs, last, NULL) != false) ? 1u : 0u;
    ok += (DynamicArraySet(&glGlob->deallocTex, last, NULL) != false) ? 1u : 0u;
    ok += (DynamicArraySet(&glGlob->deallocPal, last, NULL) != false) ? 1u : 0u;
    gNdsPlatformGlNameTablesReserved = ok;
}

void ndsPlatformInit(void)
{
    /* Calico's system tick owns timers 2/3. Initialize it before original
     * code can sample libultra time/count; BGM seam timing uses free timer 0. */
    cpuStartTiming(0);
    ndsPlatformFastTimingInit();
#if NDS_TICK_HUD
    {
        extern void ndsFastMemSelfTest(void);

        ndsFastMemSelfTest();
    }
#endif
    sVBlankCount = 0u;
    sEarliestPresentVBlank = 0u;
    irqSet(IRQ_VBLANK, ndsPlatformVBlankInterrupt);
    irqEnable(IRQ_VBLANK);

    gNdsIFCommonHUDLowerTextMode = 1u;

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
    ndsPlatformReserveGlNameTables();
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
    ndsPlatformSetBg2Affine(1 << 8, 1 << 8, 0, 0);
    bgWrapOff(sOriginalSpriteOverlayBg);
    REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG0 | BLEND_DST_BG2;
    REG_BLDALPHA = 16u | (16u << 8);
    dmaFillHalfWords(0, bgGetGfxPtr(sOriginalSpriteOverlayBg),
                     256u * 256u * sizeof(u16));
    dmaFillHalfWords(0, bgGetGfxPtr(sOriginalSpriteOverlayForegroundBg),
                     256u * 256u * sizeof(u16));

    videoSetModeSub(MODE_0_2D);
    vramSetBankH(VRAM_H_SUB_BG);
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 15, 0, false, true);
    iprintf("\x1b[?25l");
    NDS_FREEZE_DIAGNOSTICS_INIT();


}

/* libnds's text console is the image's only siscanf caller: its escape parser
 * reads "[%dA" through "[%d;%df" (console.c). newlib's siscanf links the whole
 * scanf engine and the wide-character class tables, about 25 KB of .main;
 * this one reads what the console asks for -- literal characters and %d --
 * and, like siscanf, stops at the first mismatch and returns the conversions
 * it assigned. */
int siscanf(const char *str, const char *format, ...)
{
    va_list args;
    int assigned = 0;

    va_start(args, format);
    while (*format != '\0')
    {
        if ((format[0] == '%') && (format[1] == 'd'))
        {
            const char *digits;
            int negative = 0;
            int value = 0;

            while ((*str == ' ') || (*str == '\t') || (*str == '\n'))
            {
                str++;
            }
            if ((*str == '-') || (*str == '+'))
            {
                negative = (*str == '-');
                str++;
            }
            digits = str;
            while ((*str >= '0') && (*str <= '9'))
            {
                value = (value * 10) + (*str - '0');
                str++;
            }
            if (str == digits)
            {
                break;
            }
            *va_arg(args, int *) = negative ? -value : value;
            assigned++;
            format += 2;
            continue;
        }
        if (*str != *format)
        {
            break;
        }
        str++;
        format++;
    }
    va_end(args);
    return assigned;
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
#if NDS_NET_LAB_LOBBY
    {
        /* P3 lab: the two-console autopilot (src/nds/net/nds_net_autopilot.c). */
        extern u32 ndsNetLabAutopilotKeys(void);

        held |= ndsNetLabAutopilotKeys();
    }
#endif
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
    if (held & KEY_SELECT) input |= NDS_INPUT_SELECT;

    return input;
}

void ndsPlatformBeginFrame(void)
{
    return;
}

s32 ndsPlatformReserveOriginalSpritePreview(void)
{
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
    return 256u * 256u;
}

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

    {
        u32 layer = (is_foreground != FALSE) ? 1u : 0u;
        int bg = (layer != 0u) ?
            sOriginalSpriteOverlayForegroundBg : sOriginalSpriteOverlayBg;

        if (layer != 0u)
        {
            ndsPlatformTransitionSnapshotLeaveBg3();
        }
        if ((layer != 0u) && (sOriginalSpriteOverlayBg3Lent != 0u))
        {
            if (sOriginalSpriteOverlayBg3ReturnPending == 0u)
            {
                gNdsVramBg3RefusedWrites++;
                return NULL;
            }
            ndsPlatformVramReturnBankDNow();
        }
        if (((sOriginalSpriteOverlayLayerMask & (1u << layer)) == 0u) ||
            (bg < 0))
        {
            return NULL;
        }
        if (layer == 0u)
        {
            ndsPlatformTransitionSnapshotLeaveBg2();
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
}

u32 ndsPlatformCommitOriginalSpriteFinalLayer(s32 is_foreground,
                                               u32 pixel_write_count)
{
    u32 layer = (is_foreground != FALSE) ? 1u : 0u;
    int bg = (layer != 0u) ?
        sOriginalSpriteOverlayForegroundBg : sOriginalSpriteOverlayBg;
    u32 bytes;

    if (layer != 0u)
    {
        ndsPlatformTransitionSnapshotLeaveBg3();
    }
    if ((layer != 0u) && (sOriginalSpriteOverlayBg3Lent != 0u))
    {
        if (sOriginalSpriteOverlayBg3ReturnPending == 0u)
        {
            gNdsVramBg3RefusedWrites++;
            return 0u;
        }
        ndsPlatformVramReturnBankDNow();
    }
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
}


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
    const u16 *display_preview = sOriginalSpritePreview;
    if ((display_preview == NULL) ||
        (sOriginalSpritePreviewGeneration != gNdsTaskmanHeapGeneration))
    {
        return;
    }

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
    sOriginalSpriteDisplayPreviewWidth = (u32)dst_w;
    sOriginalSpriteDisplayPreviewHeight = (u32)dst_h;
    sOriginalSpritePreviewReady = 1;
    gNdsOriginalSpritePreviewReady = 1;
    gNdsOriginalSpritePreviewDisplayWidth = (u32)dst_w;
    gNdsOriginalSpritePreviewDisplayHeight = (u32)dst_h;
    gNdsOriginalSpritePreviewCommitCount++;

    {
        u32 layer = (is_foreground != FALSE) ? 1u : 0u;

        /* A visible overlay write: the held frame ends here. */
        ndsPlatformTransitionThaw();
        if (layer == 0u)
        {
            ndsPlatformTransitionSnapshotLeaveBg2();
        }
        else
        {
            ndsPlatformTransitionSnapshotLeaveBg3();
        }
        if ((layer != 0u) && (sOriginalSpriteOverlayBg3Lent != 0u) &&
            (sOriginalSpriteOverlayBg3ReturnPending != 0u))
        {
            ndsPlatformVramReturnBankDNow();
        }
        if ((layer != 0u) && (sOriginalSpriteOverlayBg3Lent != 0u))
        {
            gNdsVramBg3RefusedWrites++;
        }
        else if ((sOriginalSpriteOverlayLayerMask & (1u << layer)) != 0u)
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
                gNdsOriginalSpriteBg2CopyBytes +=
                    (u32)dst_w * (u32)dst_h * sizeof(u16);
                ndsPlatformAdvanceOriginalSpriteOverlayEpoch(0u);
            }
        }
    }
}

void ndsPlatformCommitOriginalSpritePreview(void)
{
    ndsPlatformCommitOriginalSpritePreviewLayer(FALSE);
}

void ndsPlatformClearOriginalSpriteOverlayLayer(s32 is_foreground)
{
    int bg = (is_foreground != FALSE) ?
        sOriginalSpriteOverlayForegroundBg : sOriginalSpriteOverlayBg;

    /* An exit's clear waits for the Thaw's own; the next scene's ends the
     * hold. */
    if (sNdsTransitionHold == NDS_TRANSITION_HOLD_EXIT)
    {
        return;
    }
    ndsPlatformTransitionThaw();
    if ((is_foreground != FALSE) && (sOriginalSpriteOverlayBg3Lent != 0u))
    {
        if (sOriginalSpriteOverlayBg3ReturnPending == 0u)
        {
            return;
        }
        ndsPlatformVramReturnBankDNow();
    }
    if (bg >= 0)
    {
        u32 clear_bytes =
            ndsPlatformOriginalSpriteOverlayClearPixels() * sizeof(u16);

        /* Bank C holds the snapshot: its release clears the whole bank. */
        if ((is_foreground == FALSE) &&
            (sNdsTransitionSnapshotBank == NDS_TRANSITION_SNAPSHOT_BANK_C))
        {
            clear_bytes = 0u;
        }
        else
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
            /* Clearing BG2 transfers ownership; do not let its previous
             * wallpaper transform affect the next menu's converted surface. */
            (void)ndsPlatformQueueNativeWallpaperAffine(256, 256, 0, 0);
        }
    }
}

/* A converted wallpaper is written straight into the visible BG2 bitmap, a
 * row at a time from NitroFS, while the owner's tint over it (Results Tint2,
 * an OBJ plane) reaches the screen only at the frame's OAM commit. So the rows
 * showed as they landed: at Results tic 80 the wallpaper stood full-bright for
 * two retraces before the black tint the source makes in the same tic
 * (mnvsresults.c:3235-3242) covered it, the r62 transition captures'
 * battle-results 0225/0226 (also in the r58 baseline, 0328-0333). The layer
 * is hidden while it is written; the EndFrame that commits the frame's OBJ
 * planes and brightness shows it again. */
void ndsPlatformHideOriginalSpriteOverlayUntilCommit(s32 is_foreground)
{
    u32 layer = (is_foreground != FALSE) ? 1u : 0u;
    int bg = (layer != 0u) ?
        sOriginalSpriteOverlayForegroundBg : sOriginalSpriteOverlayBg;

    if ((bg < 0) || ((sOriginalSpriteOverlayLayerMask & (1u << layer)) == 0u))
    {
        return;
    }
    ndsPlatformTransitionThaw();
    bgHide(bg);
    sOriginalSpriteOverlayRevealPending |= 1u << layer;
}

static void ndsPlatformRevealOriginalSpriteOverlay(void)
{
    u32 pending = sOriginalSpriteOverlayRevealPending;

    sOriginalSpriteOverlayRevealPending = 0u;
    /* A layer the scene gave up meanwhile stays as the mask left it. */
    pending &= sOriginalSpriteOverlayLayerMask;
    if (((pending & 1u) != 0u) && (sOriginalSpriteOverlayBg >= 0))
    {
        bgShow(sOriginalSpriteOverlayBg);
    }
    if (((pending & 2u) != 0u) && (sOriginalSpriteOverlayForegroundBg >= 0))
    {
        bgShow(sOriginalSpriteOverlayForegroundBg);
    }
}

void ndsPlatformSetOriginalSpriteOverlayLayerMask(u32 layer_mask)
{
    u32 previous_mask;

    /* An exit's mask (the source menus turn the overlay off) is applied by
     * the Thaw, after its cover; the next scene's ends the hold. */
    if (sNdsTransitionHold == NDS_TRANSITION_HOLD_EXIT)
    {
        sNdsTransitionHoldPending |= NDS_TRANSITION_HOLD_PENDING_LAYER_MASK;
        sNdsTransitionHoldLayerMask = layer_mask;
        return;
    }
    ndsPlatformTransitionThaw();
    previous_mask = sOriginalSpriteOverlayLayerMask;
    layer_mask &= NDS_ORIGINAL_SPRITE_OVERLAY_ALL;
    /* P2-2p8 Phase 1 slice 7 (owner playtest, 2026-09-24): a scene that only
     * ASKS for BG3 while a battle's bank D return is pending no longer takes
     * D back here. VS Results asks at its first update, while the final
     * battle frame -- whose mid-match textures and fighter tint tiles live in
     * D -- is still the displayed picture for the whole Results load, and the
     * remap's clear turned every tinted part of it grey. Wanting BG3 is
     * recorded exactly as while D is lent; D comes back after the scene's
     * first displayed 3D frame (ndsPlatformEndFrame), or at its first BG3
     * write or clear, which return it before they touch the layer. */
    if (sOriginalSpriteOverlayBg3Lent != 0u)
    {
        sOriginalSpriteOverlayBg3Wanted =
            ((layer_mask & NDS_ORIGINAL_SPRITE_OVERLAY_FOREGROUND) != 0u) ?
                1u : 0u;
        layer_mask &= ~NDS_ORIGINAL_SPRITE_OVERLAY_FOREGROUND;
    }
    if (((previous_mask ^ layer_mask) &
         NDS_ORIGINAL_SPRITE_OVERLAY_BACKGROUND) != 0u)
    {
        (void)ndsPlatformQueueNativeWallpaperAffine(256, 256, 0, 0);
    }
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
}

void ndsPlatformSetOriginalSpriteOverlayEnabled(s32 is_enabled)
{
    ndsPlatformSetOriginalSpriteOverlayLayerMask(
        (is_enabled != FALSE) ? NDS_ORIGINAL_SPRITE_OVERLAY_ALL : 0u);
}

/* P2-2p8 Phase 1 slice 2b: the property that makes lending bank D safe --
 * BG3 shows nothing: no opaque (bit 15) pixel in its visible 256x192 window.
 * Hiding an empty layer changes no pixel; later writes are refused and
 * counted (gNdsVramBg3RefusedWrites). Reads only. */
u32 ndsPlatformVramBg3Empty(void)
{
    const volatile u16 *pixels;
    u32 i;

    if (sOriginalSpriteOverlayBg3Lent != 0u)
    {
        return TRUE;
    }
    if (sOriginalSpriteOverlayForegroundBg < 0)
    {
        return TRUE;
    }
    pixels = (const volatile u16 *)bgGetGfxPtr(
        sOriginalSpriteOverlayForegroundBg);
    for (i = 0u; i < 256u * 192u; i++)
    {
        if ((pixels[i] & 0x8000u) != 0u)
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* Lend bank D to the 3D engine as texture slot 3 (libnds computes a texture's
 * slot from its LCD address, so D must be slot 3). BG3 is hidden and
 * withheld until ndsPlatformVramReturnBankD. */
s32 ndsPlatformVramTakeBankD(void)
{
    /* A battle exit's held frame in D (a battle straight after it) moves to
     * C first, or ends. */
    if ((sNdsTransitionSnapshotBank == NDS_TRANSITION_SNAPSHOT_BANK_D) &&
        (ndsPlatformTransitionSnapshotMoveToC() == FALSE))
    {
        ndsPlatformTransitionSnapshotAbort();
    }
    if (sOriginalSpriteOverlayBg3Lent != 0u)
    {
        /* A battle straight after a battle: D never left the texture slot. */
        sOriginalSpriteOverlayBg3ReturnPending = 0u;
        return TRUE;
    }
    /* Hiding BG3 and remapping D changes no pixel of a held frame whose BG3
     * shows nothing (the take's own premise, ndsPlatformVramBg3Empty); one
     * that does show it ends the hold first. */
    if ((sNdsTransitionHold != NDS_TRANSITION_HOLD_NONE) &&
        ((REG_DISPCNT & DISPLAY_BG3_ACTIVE) != 0u) &&
        (ndsPlatformVramBg3Empty() == FALSE))
    {
        ndsPlatformTransitionThaw();
    }
    sOriginalSpriteOverlayBg3Wanted =
        ((sOriginalSpriteOverlayLayerMask &
          NDS_ORIGINAL_SPRITE_OVERLAY_FOREGROUND) != 0u) ? 1u : 0u;
    sOriginalSpriteOverlayLayerMask &= ~NDS_ORIGINAL_SPRITE_OVERLAY_FOREGROUND;
    if (sOriginalSpriteOverlayForegroundBg >= 0)
    {
        bgHide(sOriginalSpriteOverlayForegroundBg);
    }
    vramSetBankD(VRAM_D_TEXTURE);
    sOriginalSpriteOverlayBg3Lent = 1u;
    gNdsVramBankDLent = 1u;
    gNdsVramBankDTakes++;
    return TRUE;
}

/* Battle exit: the texture side has released everything in D and locked it;
 * the final battle frame may still be on screen and reading D, so the remap
 * waits for the next scene's first displayed 3D frame (ndsPlatformEndFrame)
 * or its first BG3 write or clear. */
void ndsPlatformVramReturnBankD(void)
{
    if (sOriginalSpriteOverlayBg3Lent != 0u)
    {
        sOriginalSpriteOverlayBg3ReturnPending = 1u;
    }
}

/* Give bank D back to BG3: remap, clear it whole (the texels it held are not
 * an image), and show the foreground again if the scene still wants it. */
static void ndsPlatformVramReturnBankDNow(void)
{
    /* A battle exit's held frame is in D: the return waits for its release
     * (BG3 stays hidden meanwhile; a BG3 write ends the frame first). */
    if (sNdsTransitionSnapshotBank == NDS_TRANSITION_SNAPSHOT_BANK_D)
    {
        return;
    }
    sOriginalSpriteOverlayBg3ReturnPending = 0u;
    if (sOriginalSpriteOverlayBg3Lent == 0u)
    {
        return;
    }
    vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
    if (sOriginalSpriteOverlayForegroundBg >= 0)
    {
        dmaFillHalfWords(0, bgGetGfxPtr(sOriginalSpriteOverlayForegroundBg),
                         256u * 256u * sizeof(u16));
    }
    sOriginalSpriteOverlayBg3Lent = 0u;
    gNdsVramBankDLent = 0u;
    gNdsVramBankDReturns++;
#if NDS_TICK_HUD
    /* Slice 2c lab: read from the next scene by gdb, which sees memory. */
    DC_FlushAll();
#endif
    if (sOriginalSpriteOverlayBg3Wanted != 0u)
    {
        sOriginalSpriteOverlayLayerMask |= NDS_ORIGINAL_SPRITE_OVERLAY_FOREGROUND;
        if (sOriginalSpriteOverlayForegroundBg >= 0)
        {
            bgShow(sOriginalSpriteOverlayForegroundBg);
        }
    }
    sOriginalSpriteOverlayBg3Wanted = 0u;
}

#if defined(NDS_TICK_HUD) && NDS_TICK_HUD
/* P2-2p8 Phase 1 slice 2a (lab only, reads only): the opaque (bit 15) pixels
 * of one overlay layer's visible 256x192 window -- layer 0 = BG2 (bank C,
 * the battle wallpaper), 1 = BG3 (bank D, the foreground overlay) -- and the
 * layer mask the scene left set, for the VRAM census. */
volatile u32 gNdsVramCensusOverlayMask __attribute__((used));

u32 ndsPlatformVramCensusOverlayOpaque(u32 layer)
{
    int bg = (layer != 0u) ? sOriginalSpriteOverlayForegroundBg :
        sOriginalSpriteOverlayBg;
    const volatile u16 *pixels;
    u32 opaque = 0u;
    u32 i;

    gNdsVramCensusOverlayMask = sOriginalSpriteOverlayLayerMask;
    if (bg < 0)
    {
        return 0xffffffffu;
    }
    pixels = (const volatile u16 *)bgGetGfxPtr(bg);
    for (i = 0u; i < 256u * 192u; i++)
    {
        if ((pixels[i] & 0x8000u) != 0u)
        {
            opaque++;
        }
    }
    return opaque;
}
#endif

void ndsPlatformSet3DLayerEnabled(s32 is_enabled)
{
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
    /* While a frame is held, BG0 is that frame's: the request is recorded and
     * the Thaw, which hides the old list, applies it. */
    if (sNdsTransitionHold != NDS_TRANSITION_HOLD_NONE)
    {
        sNdsTransitionHoldWants3D = (is_enabled != FALSE) ? TRUE : FALSE;
        return;
    }
    if (is_enabled != FALSE)
    {
        if ((REG_DISPCNT & DISPLAY_BG0_ACTIVE) == 0u)
        {
            s3dLayerEnableOnNextPresent = TRUE;
        }
        return;
    }
    s3dLayerEnableOnNextPresent = FALSE;
    REG_DISPCNT &= ~DISPLAY_BG0_ACTIVE;
}

/* The loading cover (r63): black on both screens at a VBlank, then the old
 * scene's BG0, overlays, lower HUD and Results OBJ tenant retired under it.
 * The next scene's first complete present releases it. */
static void ndsPlatformTransitionCover(s32 retire_list)
{
    ndsVideoSetTransitionBlackout(TRUE);
    /* Drain asynchronous submission before retiring the retained GX list. */
    ndsRendererFighterPacketDmaWait();
    if (retire_list != FALSE)
    {
        /* At the scene exit the last frame has already swapped: the build
         * list is empty. (A hold's Thaw may come mid-way through the next
         * scene's first frame, whose build list it must not swap in half
         * built -- hiding BG0 alone retires the old list there.) */
        glFlush(GL_TRANS_MANUALSORT);
    }
    /* A scene-boundary wait, with no extra logic tick or gameplay present. */
    swiWaitForVBlank();
    ndsVideoBlackoutCommit();
    ndsPlatformSet3DLayerEnabled(FALSE);
    ndsPlatformClearOriginalSpriteOverlayLayer(FALSE);
    /* Keep the existing bank-D loan guard; a lent bank is not an overlay. */
    ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
    ndsPlatformClearBattleTextHud();
    /* Results' OBJ tenant retires here, under the cover, rather than at its
     * START press: that exit cleared OAM mid-frame, so the text vanished one
     * scanout before the black (r62 results-css 0029). Idempotent. */
    ndsResultsOamExit();
    ndsSource2DExit();
}

/* Whether libnds's texture allocator holds any live block inside bank
 * `bank` (0 = A, 1 = B; LCD addresses). */
static u32 ndsPlatformTextureBankInUse(u32 bank)
{
    const s_vramBlock *mb = glGlob->vramBlocks[0];
    const s_SingleBlock *b;
    u32 lo = 0x06800000u + (bank << 17);
    u32 hi = lo + 0x20000u;
    u32 guard = 0u;

    if (mb == NULL)
    {
        return TRUE;
    }
    for (b = mb->firstBlock; (b != NULL) && (guard < 4096u);
         b = b->node[1], guard++)
    {
        u32 block_lo = (u32)(uintptr_t)b->AddrSet;
        u32 block_hi = block_lo + b->blockSize;

        if ((b->indexOut != 0u) && (block_lo < hi) && (block_hi > lo))
        {
            return TRUE;
        }
    }
    return (guard >= 4096u) ? TRUE : FALSE;
}

/* Back to the live layers: display mode 1, the bank to its texture slot (A,
 * B) or, emptied first while still LCD-mapped, to BG2 (C): the held pixels
 * are not the next scene's overlay, and its BG2 writes waited for this. */
static void ndsPlatformTransitionSnapshotRestore(void)
{
    vu8 *cr = (vu8 *)0x04000240u;
    u32 bank = sNdsTransitionSnapshotBank - 1u;

    REG_DISPCNT = (REG_DISPCNT & ~0x000F0000u) | 0x00010000u;
    if (sNdsTransitionSnapshotBank == NDS_TRANSITION_SNAPSHOT_BANK_C)
    {
        dmaFillWords(0u, (void *)0x06840000u, 0x20000u);
    }
    cr[bank] = sNdsTransitionSnapshotBankCr;
    sNdsTransitionSnapshotBank = 0u;
    ndsVideoSetTransitionMainHeld(FALSE);
}

/* A battle's load needs the texture bank the snapshot sits in (its texture
 * prepare, ~112 VBlanks before its first frame on the 1P intro -> stage
 * hop). Bank C is BG2's bitmap -- the battle wallpaper's and the menus'
 * overlay, never a texture slot -- and until the next scene is handed that
 * bitmap (sNdsTransitionSnapshotBg2Touched) nothing in it is that scene's:
 * the held frame moves there, the main screen shows C instead (still display
 * mode 2, identical pixels), and A/B go back to their texture slots before
 * the upload plans around them. The hold then lasts to the scene's first BG2
 * write (ndsPlatformTransitionSnapshotLeaveBg2) or first complete frame. */
static s32 ndsPlatformTransitionSnapshotMoveToC(void)
{
    vu8 *cr = (vu8 *)0x04000240u;
    u32 bank = sNdsTransitionSnapshotBank - 1u;

    /* From A, B or (a battle exit's) D. */
    if ((bank == 2u) || (bank > 3u) ||
        (sNdsTransitionSnapshotBg2Touched != 0u) ||
        (cr[2] != (VRAM_ENABLE | VRAM_C_MAIN_BG_0x06000000)))
    {
        return FALSE;
    }
    cr[2] = VRAM_ENABLE;
    dmaCopyWords(3, (const void *)(0x06800000u + (bank << 17)),
                 (void *)0x06840000u, 256u * 192u * sizeof(u16));
    REG_DISPCNT = (REG_DISPCNT & ~0x000C0000u) | (2u << 18);
    cr[bank] = sNdsTransitionSnapshotBankCr;
    sNdsTransitionSnapshotBankCr = VRAM_ENABLE | VRAM_C_MAIN_BG_0x06000000;
    sNdsTransitionSnapshotBank = NDS_TRANSITION_SNAPSHOT_BANK_C;
    NDS_DIAG(gNdsTransitionSnapshotMoveCount++);
    return TRUE;
}

/* A 1P battle's exit (2026-10-03, owner BUGS.md: "Transition from match end
 * to score has black screen, should hold last match end frame until score is
 * ready"). The last battle frame fills both texture banks (A and B: 256 KB of
 * live blocks at the Hyrule exit) and reads bank D's released fighter texels
 * and BG2's wallpaper in C, so no bank is free for the capture below
 * (decline 4) and the tally's whole load was black. The frame is rebuilt
 * instead: the wallpaper as BG2 shows it is copied out of C into the packet
 * arena (the battle's packets are done; the arena is re-cleared after), the
 * backdrop takes the wallpaper's average colour, and C is lent to the
 * capture unit for one composited frame -- 3D, OBJ and the other layers over
 * that backdrop, BG2 having no bank. Every captured pixel of exactly the
 * backdrop colour is where the wallpaper showed, and the wallpaper, through
 * BG2's own affine, fills it (a 3D or OBJ pixel of exactly that colour takes
 * the wallpaper's pixel there, a near match). For that frame and the
 * composite the main screen shows the rest over the average colour. The
 * frame then goes to D, which no displayed layer reads once it shows, and C
 * back to BG2, empty: the next scene's BG2 writes and texture uploads find
 * their banks as before. 3D translucent over the wallpaper is kept
 * unblended. Taken only at a 1P battle's or bonus stage's exit, whose next
 * scene reads the frame as its photo too (the stage clear,
 * ndsPlatformTransitionSnapshotToN64Frame).
 *
 * D may still hold live texture blocks (owner r74: the tally was black after
 * Mario Bros and the Polygon Team: 11 KB and 1 KB of the battle's textures
 * past 0x06860000 refused the capture). D is the battle's loan (lent and
 * asked back), so every block in it is the leaving battle's: nothing draws
 * them once the held frame replaces the 3D layer, and the next scene's entry
 * drops their names with every other texture (glResetTextures). */
extern volatile u32 gNdsSceneManagerCurrKind __attribute__((weak));
extern u16 gSYFramebufferSets[1][231][320];

static s32 ndsPlatformTransitionSnapshotOverWallpaper(void)
{
    vu8 *cr = (vu8 *)0x04000240u;
    const u16 backdrop = BG_PALETTE[0];
    u16 key = backdrop & 0x7fffu;
    const s32 pa = sNdsBg2AffineShown[0];
    const s32 pd = sNdsBg2AffineShown[1];
    u16 *wall;
    u16 *frame = (u16 *)0x06840000u;
    u32 wallpaper;
    u32 sum[3] = { 0u, 0u, 0u };
    u32 samples = 0u;
    u8 d_cr;
    u32 i;
    s32 y;

    if ((&gNdsSceneManagerCurrKind == NULL) ||
        ((gNdsSceneManagerCurrKind != NDS_PLATFORM_SCENE_KIND_1P_GAME) &&
         (gNdsSceneManagerCurrKind !=
          NDS_PLATFORM_SCENE_KIND_1P_BONUS_STAGE)) ||
        (sizeof(gSYFramebufferSets) < 0x20000u) ||
        (sOriginalSpriteOverlayBg < 0) ||
        (sOriginalSpriteOverlayBg3Lent == 0u) ||
        (sOriginalSpriteOverlayBg3ReturnPending == 0u) ||
        (cr[2] != (VRAM_ENABLE | VRAM_C_MAIN_BG_0x06000000)) ||
        ((cr[3] & 0x87u) != 0x83u) ||
        ((REG_DISPCNT & DISPLAY_BG0_ACTIVE) == 0u))
    {
        return FALSE;
    }
    /* The packets' DMA reads the arena this borrows. */
    ndsRendererFighterPacketDmaWait();
    wall = &gSYFramebufferSets[0][0][0];
    wallpaper = (((REG_DISPCNT & DISPLAY_BG2_ACTIVE) != 0u) &&
                 ((sOriginalSpriteOverlayLayerMask &
                   NDS_ORIGINAL_SPRITE_OVERLAY_BACKGROUND) != 0u)) ? 1u : 0u;
    if (wallpaper != 0u)
    {
        memcpy(wall, bgGetGfxPtr(sOriginalSpriteOverlayBg), 0x20000u);
        /* Its average over a 16 x 12 grid: the backdrop while C is away,
         * and the key. */
        for (y = 8; y < 192; y += 16)
        {
            const s32 ty = (sNdsBg2AffineShown[3] + (pd * y)) >> 8;
            s32 x;

            for (x = 8; (x < 256) && (ty >= 0) && (ty < 256); x += 16)
            {
                const s32 tx = (sNdsBg2AffineShown[2] + (pa * x)) >> 8;
                u32 c;

                if ((tx < 0) || (tx >= 256))
                {
                    continue;
                }
                c = wall[(ty << 8) + tx];
                if ((c & 0x8000u) == 0u)
                {
                    continue;
                }
                sum[0] += c & 0x1fu;
                sum[1] += (c >> 5) & 0x1fu;
                sum[2] += (c >> 10) & 0x1fu;
                samples++;
            }
        }
        if (samples != 0u)
        {
            key = (u16)((sum[0] / samples) | ((sum[1] / samples) << 5) |
                        ((sum[2] / samples) << 10));
            BG_PALETTE[0] = key;
        }
    }
    cr[2] = VRAM_ENABLE;
    REG_DISPCAPCNT = DCAP_ENABLE | DCAP_MODE(DCAP_MODE_A) |
                     DCAP_SRC_A(DCAP_SRC_A_COMPOSITED) |
                     DCAP_SIZE(DCAP_SIZE_256x192) | DCAP_OFFSET(0) |
                     DCAP_BANK(2);
    for (i = 0u; (i < 3u) && ((REG_DISPCAPCNT & DCAP_ENABLE) != 0u); i++)
    {
        swiWaitForVBlank();
    }
    if ((REG_DISPCAPCNT & DCAP_ENABLE) != 0u)
    {
        REG_DISPCAPCNT = 0u;
        memcpy(frame, wall, 0x20000u);
        cr[2] = VRAM_ENABLE | VRAM_C_MAIN_BG_0x06000000;
        BG_PALETTE[0] = backdrop;
        ndsRendererFighterPacketRelease();
        gNdsTransitionSnapshotDecline = 6u;
        return FALSE;
    }
    /* The wallpaper where the capture shows the key; its own transparent
     * pixels and its outside are the real backdrop. */
    for (y = 0; y < 192; y++)
    {
        const s32 ty = (sNdsBg2AffineShown[3] + (pd * y)) >> 8;
        const u16 *src = ((wallpaper != 0u) && (ty >= 0) && (ty < 256)) ?
            &wall[ty << 8] : NULL;
        u16 *dst = &frame[(u32)y << 8];
        s32 tx_q8 = sNdsBg2AffineShown[2];
        s32 x;

        for (x = 0; x < 256; x++, tx_q8 += pa)
        {
            u32 w = 0u;

            if ((dst[x] & 0x7fffu) != key)
            {
                continue;
            }
            if ((src != NULL) && ((u32)(tx_q8 >> 8) < 256u))
            {
                w = src[tx_q8 >> 8];
            }
            dst[x] = ((w & 0x8000u) != 0u) ? (u16)w :
                                               (u16)(backdrop | 0x8000u);
        }
    }
    d_cr = cr[3];
    cr[3] = VRAM_ENABLE;
    dmaCopyWords(3, frame, (void *)0x06860000u, 256u * 192u * sizeof(u16));
    swiWaitForVBlank();
    REG_DISPCNT = (REG_DISPCNT & ~0x000F0000u) | 0x00020000u | (3u << 18);
    BG_PALETTE[0] = backdrop;
    /* C back to BG2, empty: the battle's wallpaper is not the next scene's. */
    dmaFillWords(0u, frame, 0x20000u);
    cr[2] = VRAM_ENABLE | VRAM_C_MAIN_BG_0x06000000;
    sNdsTransitionSnapshotBankCr = d_cr;
    sNdsTransitionSnapshotBank = NDS_TRANSITION_SNAPSHOT_BANK_D;
    sNdsTransitionSnapshotBg2Touched = 0u;
    ndsVideoSetTransitionMainHeld(TRUE);
    NDS_DIAG(gNdsTransitionSnapshotCount++);
    NDS_DIAG(gNdsTransitionSnapshotExitCount++);
    /* The arena held the wallpaper: the packets' words are gone. */
    ndsRendererFighterPacketRelease();
    return TRUE;
}

/* THE TRANSITION SNAPSHOT (owner, r68, 2026-10-02: "Do we have to have the
 * Black transition screen everywhere instead of holding the last frame?").
 *
 * The hold below keeps the leaving frame only until the next scene's first
 * display write, because nothing here is a framebuffer: BG0 re-renders from
 * texture VRAM and the 2D layers scan theirs live. The display capture unit
 * makes one. At the hold's start the frame on screen is captured, whole and
 * composited (BG, OBJ and 3D, before master brightness), into a texture bank
 * that holds no texture -- so neither the captured frame's 3D nor any upload
 * the next scene makes (libnds allocates only in banks mapped as texture)
 * reads or writes it -- and the main screen then shows that bank directly
 * (display mode 2). Every layer behind it is free for the next scene; the
 * loading cover darkens only the lower screen; the next scene's first
 * complete frame switches back (ndsPlatformEndFrame). A texture upload
 * before that ends it early under the old black cover, because a 3D scene's
 * VRAM layout (the battle's statics run contiguously from bank A) must not
 * be planned around a borrowed bank. A frame with no texture-free bank (a
 * battle's), or one under a fade, keeps the hold alone. */
static void ndsPlatformTransitionSnapshotBegin(void)
{
    vu8 *cr = (vu8 *)0x04000240u;
    u32 bank;
    u32 i;

    if (sNdsTransitionSnapshotBank != 0u)
    {
        return;
    }
    gNdsTransitionSnapshotDecline = 0u;
    if ((REG_MASTER_BRIGHT != 0u) || (REG_MASTER_BRIGHT_SUB != 0u))
    {
        /* The capture is taken before brightness: under a fade it would
         * show the frame brighter than it was. */
        gNdsTransitionSnapshotDecline = 1u;
    }
    else if ((REG_DISPCNT & 0x00030000u) != 0x00010000u)
    {
        gNdsTransitionSnapshotDecline = 2u;
    }
    else if ((REG_DISPCAPCNT & DCAP_ENABLE) != 0u)
    {
        gNdsTransitionSnapshotDecline = 3u;
    }
    if (gNdsTransitionSnapshotDecline != 0u)
    {
        NDS_DIAG(gNdsTransitionSnapshotDeclineCount++);
        return;
    }
    /* B first: slot 1 fills only after slot 0. */
    for (bank = 2u; bank-- > 0u;)
    {
        if (((cr[bank] & 0x87u) == 0x83u) &&
            (ndsPlatformTextureBankInUse(bank) == FALSE))
        {
            break;
        }
    }
    if (bank > 1u)
    {
        if (ndsPlatformTransitionSnapshotOverWallpaper() != FALSE)
        {
            return;
        }
        if (gNdsTransitionSnapshotDecline == 0u)
        {
            gNdsTransitionSnapshotDecline = 4u;
        }
        NDS_DIAG(gNdsTransitionSnapshotDeclineCount++);
        return;
    }
    sNdsTransitionSnapshotBankCr = cr[bank];
    cr[bank] = VRAM_ENABLE;
    REG_DISPCAPCNT = DCAP_ENABLE | DCAP_MODE(DCAP_MODE_A) |
                     DCAP_SRC_A(DCAP_SRC_A_COMPOSITED) |
                     DCAP_SIZE(DCAP_SIZE_256x192) | DCAP_OFFSET(0) |
                     DCAP_BANK(bank);
    /* The capture runs through the next whole frame and clears its enable
     * bit at that frame's end. */
    for (i = 0u; (i < 3u) && ((REG_DISPCAPCNT & DCAP_ENABLE) != 0u); i++)
    {
        swiWaitForVBlank();
    }
    if ((REG_DISPCAPCNT & DCAP_ENABLE) != 0u)
    {
        REG_DISPCAPCNT = 0u;
        cr[bank] = sNdsTransitionSnapshotBankCr;
        gNdsTransitionSnapshotDecline = 5u;
        NDS_DIAG(gNdsTransitionSnapshotDeclineCount++);
        return;
    }
    /* Inside the VBlank the wait returned in: the next scanout is the
     * capture. */
    REG_DISPCNT = (REG_DISPCNT & ~0x000F0000u) | 0x00020000u | (bank << 18);
    sNdsTransitionSnapshotBank = bank + 1u;
    sNdsTransitionSnapshotBg2Touched = 0u;
    ndsVideoSetTransitionMainHeld(TRUE);
    NDS_DIAG(gNdsTransitionSnapshotCount++);
}

s32 ndsPlatformTransitionSnapshotHoldsBg2(void)
{
    return (sNdsTransitionSnapshotBank == NDS_TRANSITION_SNAPSHOT_BANK_C) ?
        TRUE : FALSE;
}

/* The held frame as the N64 colour framebuffer a source photo reader copies
 * (sc1PStageClearCopyFramebufToWallpaper: the 300x220 area at (10,10) of a
 * 320-wide RGBA5551 frame, a word -- two pixels -- at a time). The screen
 * shows that area at 256x192, so each N64 pixel takes the held pixel its
 * centre falls in. The port's runtime files keep every 32-bit word
 * byte-swapped (16-bit texel x at index x ^ 1) and the copy moves words, so
 * the frame is written in that order. FALSE, frame untouched, without a
 * held frame. */
s32 ndsPlatformTransitionSnapshotToN64Frame(u16 *frame, u32 width,
                                            u32 height)
{
    const u16 *held;
    u8 column[300];
    u32 x;
    u32 y;

    if ((sNdsTransitionSnapshotBank == 0u) || (frame == NULL) ||
        (width < 320u) || (height < 230u))
    {
        return FALSE;
    }
    held = (const u16 *)(uintptr_t)(0x06800000u +
                                    ((sNdsTransitionSnapshotBank - 1u) << 17));
    for (x = 0u; x < 300u; x++)
    {
        column[x] = (u8)((((x << 1) + 1u) * 256u) / 600u);
    }
    for (y = 0u; y < 220u; y++)
    {
        const u16 *src = &held[((((y << 1) + 1u) * 192u) / 440u) * 256u];
        u16 *dst = &frame[(y + 10u) * width];

        for (x = 0u; x < 300u; x++)
        {
            u32 c = src[column[x]];

            dst[(x + 10u) ^ 1u] = (u16)(((c & 0x1fu) << 11) |
                                        (((c >> 5) & 0x1fu) << 6) |
                                        (((c >> 10) & 0x1fu) << 1) | 1u);
        }
    }
    NDS_DIAG(gNdsTransitionSnapshotPhotoCount++);
    return TRUE;
}

/* A texture upload, or the battle's texture VRAM reset, while the snapshot
 * shows: the black cover takes the main screen at a VBlank, then the bank
 * goes back to its texture slot before the upload plans around it. */
void ndsPlatformTransitionSnapshotAbort(void)
{
    if (sNdsTransitionSnapshotBank == 0u)
    {
        return;
    }
    NDS_DIAG(gNdsTransitionSnapshotAbortCount++);
    ndsVideoSetTransitionBlackout(TRUE);
    ndsVideoSetTransitionMainHeld(FALSE);
    swiWaitForVBlank();
    ndsVideoBlackoutCommit();
    ndsPlatformTransitionSnapshotRestore();
}

/* SCENE TRANSITION HOLD (owner, r64, 2026-09-30: "keep what's currently on
 * the screen until the next screen is ready, then switch").
 *
 * That is what the N64 shows: its VI scans the last framebuffer until the
 * next scene's first composed frame replaces it (artifacts/bugs/2026-09-30_
 * scene-transitions/REPORT.md section 3). Nothing here is a framebuffer --
 * BG0 re-renders the retained GX list from texture VRAM every frame and the
 * 2D layers scan their VRAM live -- so the old frame stays on screen exactly
 * as long as nothing writes those. Hence:
 *  - the hold starts after the leaving frame's present (menu shell), at a
 *    source menu's leaving update, which it never draws (taskman.c:994), or
 *    at the scene exit (battle): where r63 committed black;
 *  - EXIT, while the old scene tears down: the layer owners record its
 *    display teardown (BG0 off, overlay mask, title fire, backdrop, OBJ
 *    shadows) instead of performing it;
 *  - LOAD, from ndsPlatformBeginSceneTransition: the next scene's software
 *    runs; its first display write -- a 2D layer, an OBJ tenant, bank D under
 *    a visible BG3, a texture upload under a visible BG0 -- calls the Thaw;
 *  - the Thaw is the r63 cover plus the recorded teardown, so the black now
 *    lasts only while the next scene builds its first frame; every scene's
 *    first complete present releases it, as before.
 * EndFrame presents nothing during a hold: a loading service frame must not
 * commit the next scene's half-built OBJ shadows, fades or wallpaper.
 * Measured on r64 (trace, VBlanks): SSS -> battle held 147 of 170. */
void ndsPlatformTransitionHoldBegin(void)
{
    if (sNdsTransitionHold != NDS_TRANSITION_HOLD_NONE)
    {
        return;
    }
    sNdsTransitionHold = NDS_TRANSITION_HOLD_EXIT;
    sNdsTransitionHoldPending = 0u;
    sNdsTransitionHoldWants3D = FALSE;
    sNdsTransitionHoldPresents = 0u;
    NDS_DIAG(gNdsTransitionHoldCount++);
    gNdsTransitionHoldVBlank = sVBlankCount;
    ndsPlatformTransitionSnapshotBegin();
}

u32 ndsPlatformTransitionHolding(void)
{
    return sNdsTransitionHold;
}

u32 ndsPlatformSceneLoading(void)
{
    if (sNdsTransitionHold == NDS_TRANSITION_HOLD_LOAD)
    {
        return TRUE;
    }
    return (ndsVideoGetTransitionBlackout() != FALSE) ? TRUE : FALSE;
}

void ndsPlatformTransitionThaw(void)
{
    u32 pending;

    if (sNdsTransitionHold == NDS_TRANSITION_HOLD_NONE)
    {
        return;
    }
    gNdsTransitionThawCaller = (u32)(uintptr_t)__builtin_return_address(0);
    gNdsTransitionThawVBlank = sVBlankCount;
    NDS_DIAG(gNdsTransitionThawCount++);
    pending = sNdsTransitionHoldPending;
    sNdsTransitionHold = NDS_TRANSITION_HOLD_NONE;
    sNdsTransitionHoldPending = 0u;
    ndsPlatformTransitionCover(FALSE);
    /* The old scene's exit, in its order, now under the cover. */
    if ((pending & NDS_TRANSITION_HOLD_PENDING_TITLE_FIRE_OFF) != 0u)
    {
        ndsPlatformSetTitleFireEnabled(FALSE, 0, 0);
    }
    if ((pending & NDS_TRANSITION_HOLD_PENDING_LAYER_MASK) != 0u)
    {
        ndsPlatformSetOriginalSpriteOverlayLayerMask(
            sNdsTransitionHoldLayerMask);
    }
    if ((pending & NDS_TRANSITION_HOLD_PENDING_BACKDROP) != 0u)
    {
        BG_PALETTE[0] = sNdsTransitionHoldBackdrop;
    }
    if ((pending & NDS_TRANSITION_HOLD_PENDING_OAM_MAIN) != 0u)
    {
        oamUpdate(&oamMain);
    }
    if ((pending & NDS_TRANSITION_HOLD_PENDING_OAM_SUB) != 0u)
    {
        oamUpdate(&oamSub);
    }
    /* The next scene's own BG0 request, made while BG0 was the old frame's:
     * enabled at its first presented frame (ndsPlatformEndFrame). */
    if (sNdsTransitionHoldWants3D != FALSE)
    {
        ndsPlatformSet3DLayerEnabled(TRUE);
    }
    /* Results' OBJ tenant, entered at its scene start under the held battle
     * frame (nds_results_oam.c). */
    ndsResultsOamEnterPending();
    ndsSource2DEnterPending();
}

void ndsPlatformTransitionThawIf3DShown(void)
{
    /* No texture lives in C, and none is placed in D while a battle exit's
     * frame keeps it LCD-mapped (libnds allocates only in banks mapped as
     * texture; a battle's take of D moves or ends it first,
     * ndsPlatformVramTakeBankD): a snapshot in either outlasts every upload
     * and the scene's texture VRAM reset. */
    if ((sNdsTransitionSnapshotBank != 0u) &&
        (sNdsTransitionSnapshotBank != NDS_TRANSITION_SNAPSHOT_BANK_C) &&
        (sNdsTransitionSnapshotBank != NDS_TRANSITION_SNAPSHOT_BANK_D) &&
        (ndsPlatformTransitionSnapshotMoveToC() == FALSE))
    {
        ndsPlatformTransitionSnapshotAbort();
    }
    if ((sNdsTransitionHold != NDS_TRANSITION_HOLD_NONE) &&
        ((REG_DISPCNT & DISPLAY_BG0_ACTIVE) != 0u))
    {
        ndsPlatformTransitionThaw();
    }
}

void ndsPlatformTransitionHoldPendOamUpdate(u32 engine)
{
    sNdsTransitionHoldPending |= (engine != 0u) ?
        NDS_TRANSITION_HOLD_PENDING_OAM_SUB :
        NDS_TRANSITION_HOLD_PENDING_OAM_MAIN;
}

void ndsPlatformSetBackdropColor(u16 color)
{
    if (sNdsTransitionHold == NDS_TRANSITION_HOLD_EXIT)
    {
        sNdsTransitionHoldPending |= NDS_TRANSITION_HOLD_PENDING_BACKDROP;
        sNdsTransitionHoldBackdrop = color;
        return;
    }
    ndsPlatformTransitionThaw();
    BG_PALETTE[0] = color;
}

void ndsPlatformBeginSceneTransition(void)
{
    ndsRendererFighterPacketDmaWait();
    /* A battle's exit has no leaving-frame call: its hold starts here. */
    ndsPlatformTransitionHoldBegin();
    /* The old scene has exited; what runs from now is the next one. */
    sNdsTransitionHold = NDS_TRANSITION_HOLD_LOAD;
}

/* Texture writes the next scene makes while a frame with 3D is held would
 * overwrite that frame's texels under it: the link wraps libnds's uploads
 * (-Wl,--wrap, Makefile) to end the hold first. */
int __real_glTexImage2D(int target, int empty1, GL_TEXTURE_TYPE_ENUM type,
                        int sizeX, int sizeY, int empty2, int param,
                        const void *texture);
void __real_glColorTableEXT(int target, int empty1, u16 width, int empty2,
                            int empty3, const u16 *table);
void __real_glColorSubTableEXT(int target, int start, int count, int empty1,
                               int empty2, const u16 *data);

/* libnds splits a VRAM block with a malloc it never checks (videoGL.c,
 * vramBlock__allocateBlock), one ~28-byte node per texture and per palette.
 * The campaign walk faulted there in the Giant DK battle (2026-10-02): the
 * palette manager held 303 blocks and libc's top chunk was down to 624 B.
 * An upload refused here is glTexImage2D's 0, the answer the texture cache
 * already takes when VRAM is full: it evicts its oldest texture, whose blocks
 * go back to libc, and retries. The margin keeps libc's other users (stdio
 * buffers, storage calls) clear of the edge. A palette refused leaves the
 * texture's previous palette, not a fault. */
#define NDS_PLATFORM_GL_LIBC_ROOM 4096u
__attribute__((used)) volatile u32 gNdsPlatformGlLibcRefusals;

/* libnds's texture upload copies on DMA 0 (glTexImage2D programs DMA0SAD..
 * DMA0CNT), the channel the geometry FIFO's transfers run on, and the
 * renderer leaves a FIFO transfer running for the next writer to wait on.
 * Started meanwhile, the copy rewrote that transfer mid-list: the engine
 * waited on a command's missing parameters and the next CPU write to the
 * full FIFO stalled the ARM9 on the bus, the freeze watchdog with it
 * (2026-10-18, owner hardware: a silent host freeze, music still playing).
 * Every upload and palette load waits for the transfer first (bounded:
 * NDS_GX_DMA0_WAIT clears a stall that outlasts four frames). */
#define NDS_PLATFORM_GL_DMA0_SITE 9u

int __wrap_glTexImage2D(int target, int empty1, GL_TEXTURE_TYPE_ENUM type,
                        int sizeX, int sizeY, int empty2, int param,
                        const void *texture)
{
    ndsPlatformTransitionThawIf3DShown();
    if (ndsTaskmanLibcHasRoom(NDS_PLATFORM_GL_LIBC_ROOM) == FALSE)
    {
        NDS_DIAG(gNdsPlatformGlLibcRefusals++);
        return 0;
    }
    NDS_GX_DMA0_WAIT(NDS_PLATFORM_GL_DMA0_SITE);
    return __real_glTexImage2D(target, empty1, type, sizeX, sizeY, empty2,
                               param, texture);
}

void __wrap_glColorTableEXT(int target, int empty1, u16 width, int empty2,
                            int empty3, const u16 *table)
{
    ndsPlatformTransitionThawIf3DShown();
    /* A zero width (with no table) frees the palette: never refused. */
    if ((width != 0u) && (table != NULL) &&
        (ndsTaskmanLibcHasRoom(NDS_PLATFORM_GL_LIBC_ROOM) == FALSE))
    {
        NDS_DIAG(gNdsPlatformGlLibcRefusals++);
        return;
    }
    NDS_GX_DMA0_WAIT(NDS_PLATFORM_GL_DMA0_SITE);
    __real_glColorTableEXT(target, empty1, width, empty2, empty3, table);
}

void __wrap_glColorSubTableEXT(int target, int start, int count, int empty1,
                               int empty2, const u16 *data)
{
    ndsPlatformTransitionThawIf3DShown();
    NDS_GX_DMA0_WAIT(NDS_PLATFORM_GL_DMA0_SITE);
    __real_glColorSubTableEXT(target, start, count, empty1, empty2, data);
}

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
}

void ndsPlatformReset3DViewport(void)
{
    ndsRendererFighterPacketDmaWait();
    glViewport(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
}

/* P2-1h. Apply BG2's queued affine NOW instead of at the next present.
 *
 * The fast wallpaper leaves BG2 under the battle's own 4/5 transform, and
 * clearing the layer only QUEUES the identity reset -- ndsPlatformEndFrame
 * commits it. A menu that draws backdrop art between the clear and the first
 * present would therefore show one frame of that art scaled by whatever the
 * last battle left behind. This is the same commit, called early; it is
 * idempotent, because the commit clears its own pending flag. */
static s32 sNativeWallpaperAffine[4];
static u32 sNativeWallpaperAffineEpoch;
static u32 sNativeWallpaperAffinePending;

u32 ndsPlatformQueueNativeWallpaperAffine(s32 pa, s32 pd, s32 dx, s32 dy)
{
    if ((sOriginalSpriteOverlayBg < 0) || (pa <= 0) || (pa > 32767) ||
        (pd <= 0) || (pd > 32767) ||
        (dx < -134217728) || (dx > 134217727) ||
        (dy < -134217728) || (dy > 134217727))
    {
        return FALSE;
    }
    sNativeWallpaperAffine[0] = pa;
    sNativeWallpaperAffine[1] = pd;
    sNativeWallpaperAffine[2] = dx;
    sNativeWallpaperAffine[3] = dy;
    sNativeWallpaperAffineEpoch = sOriginalSpriteOverlayEpoch[0];
    sNativeWallpaperAffinePending = TRUE;
    return TRUE;
}

static void ndsPlatformCommitNativeWallpaperAffine(void)
{
    if ((sNativeWallpaperAffinePending != FALSE) &&
        (sNativeWallpaperAffineEpoch == sOriginalSpriteOverlayEpoch[0]) &&
        (sOriginalSpriteOverlayBg >= 0))
    {
        ndsPlatformSetBg2Affine(sNativeWallpaperAffine[0],
                                sNativeWallpaperAffine[1],
                                sNativeWallpaperAffine[2],
                                sNativeWallpaperAffine[3]);
    }
    sNativeWallpaperAffinePending = FALSE;
}

void ndsPlatformCommitOriginalSpriteOverlayTransform(void)
{
    ndsPlatformTransitionThaw();
    ndsPlatformCommitNativeWallpaperAffine();
}

/* P2-1i -- BG3 as the title's fire layer. See the header for why this is an
 * affine and not a blit.
 *
 * PA/PD are the SOURCE step per screen pixel in 8.8, so a cell 51 wide across
 * a 256 px screen is PA = 51 and a cell 42 tall across 192 rows is PD = 56.
 * PB/PC are zero: the source's fire is axis-aligned. */
void ndsPlatformSetTitleFireEnabled(s32 is_enabled, s32 pa, s32 pd)
{
    if (sOriginalSpriteOverlayForegroundBg < 0)
    {
        return;
    }
    /* The title's exit hands BG3 back after the Thaw's cover. */
    if ((is_enabled == FALSE) &&
        (sNdsTransitionHold == NDS_TRANSITION_HOLD_EXIT))
    {
        sNdsTransitionHoldPending |= NDS_TRANSITION_HOLD_PENDING_TITLE_FIRE_OFF;
        return;
    }
    ndsPlatformTransitionThaw();
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
}

void ndsPlatformSetTitleFireFrame(s32 atlas_x, s32 atlas_y)
{
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
}


u32 ndsPlatformSceneWallpaperQueueTransform(s32 origin_x, s32 origin_y,
                                             u32 scale_x_q16,
                                             u32 scale_y_q16)
{
    (void)origin_x;
    (void)origin_y;
    (void)scale_x_q16;
    (void)scale_y_q16;
    return FALSE;
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
    sOriginalSpriteDecodeCacheEpoch++;
    if (sOriginalSpriteDecodeCacheEpoch == 0u)
    {
        sOriginalSpriteDecodeCacheEpoch = 1u;
    }
}

u16 *ndsPlatformBeginOriginalDLPreview(u32 width, u32 height, u32 *out_pitch)
{
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
}


void ndsPlatformCommitOriginalDLPreview(void)
{
}


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

#if NDS_TICK_HUD
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
/* NDS_TICK_HUD_WINDOW (Makefile): 128, or 32 on the four-CPU gate ROM, whose
 * ring is battle heap the published ROM has (scripts/sample-tick-hud-buckets.ps1
 * reads the window from the ELF). */
#ifndef NDS_TICK_HUD_WINDOW
#define NDS_TICK_HUD_WINDOW 128u
#endif
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
    "SINT", "SPHD", "SPHC",
    /* P2-2p8 Phase 0 MISC split and GX list usage. */
    "MWPN", "MITM", "MEFX", "MACT", "MPRT", "MTEX", "MCAP", "MPRO",
    "MFLS", "GPOL", "GVTX",
    /* P2-2p8 Phase 0 replay digest (not ticks). */
    "DGSA", "DGSB"
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
 * likewise under NDS_TICK_HUD. */
#if NDS_TICK_HUD
void ndsPlatformTickHudReset(void)
{
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
}

void ndsPlatformTickHudSample(void)
{
    u32 bucket;
    u32 head = sBattleTickHudRingHead;

    for (bucket = 0u; bucket < nNDSTickHudBucketCount; bucket++)
    {
        sBattleTickHudRing[bucket][head] = gNdsTickHudBuckets[bucket];
    }
    sBattleTickHudRingHead = (head + 1u) % NDS_TICK_HUD_WINDOW;
    if (sBattleTickHudRingCount < NDS_TICK_HUD_WINDOW)
    {
        sBattleTickHudRingCount++;
    }
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

#if NDS_P2_MENU_SHELL
/* The menu shell already owns the cadence measurement. This HUD only turns its
 * published counters into two readable lower-screen rows: no per-frame ring,
 * sorting, or second presentation recorder. FPS is the recent presented-frame
 * delta over the same platform VBlank clock, while 2VB/3+/MAX come directly
 * from the cumulative per-screen histogram used by probe-p2-shell.ps1. */
static void ndsPlatformRenderMenuFpsHud(void)
{
    u32 screen = gNdsMenuShellScreen;
    u32 enter_count;
    u32 now_vblank;
    u32 frames;
    u32 elapsed_vblanks;
    u32 elapsed_frames;
    u32 hist_total;
    u32 two_vblank;
    u32 two_vblank_pct;
    u32 three_plus;
    u32 max_interval;
    u32 fps_x10;

    if (screen >= NDS_MENU_SHELL_SCREEN_COUNT)
    {
        return;
    }

    enter_count = gNdsMenuShellEnterCount[screen];
    now_vblank = ndsPlatformVBlankCount();
    frames = gNdsMenuShellFrames[screen];

    if ((screen != sMenuFpsHudScreen) ||
        (enter_count != sMenuFpsHudEnterCount))
    {
        u32 h0 = gNdsMenuShellVBlankHist[screen][0u];
        u32 h1 = gNdsMenuShellVBlankHist[screen][1u];
        u32 h2 = gNdsMenuShellVBlankHist[screen][2u];
        u32 h3 = gNdsMenuShellVBlankHist[screen][3u];

        hist_total = h0 + h1 + h2 + h3;
        two_vblank_pct = (hist_total == 0u) ? 0u :
            ((h1 * 100u) + (hist_total / 2u)) / hist_total;
        three_plus = h2 + h3;
        max_interval = gNdsMenuShellVBlankMax[screen];

        sMenuFpsHudScreen = screen;
        sMenuFpsHudEnterCount = enter_count;
        sMenuFpsHudLastVBlank = now_vblank;
        sMenuFpsHudLastFrames = frames;
        sMenuFpsHudPrintedFpsX10 = 0xffffffffu;
        sMenuFpsHudPrintedTwoVBlankPct = two_vblank_pct;
        sMenuFpsHudPrintedThreePlus = three_plus;
        sMenuFpsHudPrintedMax = max_interval;
        gNdsMenuFpsHudRefreshCount++;
        DC_FlushRange((const void *)&gNdsMenuFpsHudRefreshCount,
                      sizeof(gNdsMenuFpsHudRefreshCount));

        ndsPlatformPrintDebugLine(0u, "%s FPS --.-",
                                  sMenuFpsHudNames[screen]);
        ndsPlatformPrintDebugLine(1u, "2VB %lu%% 3+ %lu MAX %lu",
                                  (unsigned long)two_vblank_pct,
                                  (unsigned long)three_plus,
                                  (unsigned long)max_interval);
        return;
    }

    elapsed_vblanks = now_vblank - sMenuFpsHudLastVBlank;
    if (elapsed_vblanks < NDS_MENU_FPS_HUD_SAMPLE_VBLANKS)
    {
        return;
    }

    elapsed_frames = frames - sMenuFpsHudLastFrames;
    fps_x10 = ((elapsed_frames *
                (NDS_BATTLE_SOURCE_TICKS_PER_SECOND * 10u)) +
               (elapsed_vblanks / 2u)) / elapsed_vblanks;
    sMenuFpsHudLastVBlank = now_vblank;
    sMenuFpsHudLastFrames = frames;

    {
        u32 h0 = gNdsMenuShellVBlankHist[screen][0u];
        u32 h1 = gNdsMenuShellVBlankHist[screen][1u];
        u32 h2 = gNdsMenuShellVBlankHist[screen][2u];
        u32 h3 = gNdsMenuShellVBlankHist[screen][3u];

        hist_total = h0 + h1 + h2 + h3;
        two_vblank = h1;
        two_vblank_pct = (hist_total == 0u) ? 0u :
            ((two_vblank * 100u) + (hist_total / 2u)) / hist_total;
        three_plus = h2 + h3;
    }
    max_interval = gNdsMenuShellVBlankMax[screen];
    gNdsMenuFpsHudRefreshCount++;
    DC_FlushRange((const void *)&gNdsMenuFpsHudRefreshCount,
                  sizeof(gNdsMenuFpsHudRefreshCount));

    if (fps_x10 != sMenuFpsHudPrintedFpsX10)
    {
        sMenuFpsHudPrintedFpsX10 = fps_x10;
        ndsPlatformPrintDebugLine(0u, "%s FPS %lu.%lu",
                                  sMenuFpsHudNames[screen],
                                  (unsigned long)(fps_x10 / 10u),
                                  (unsigned long)(fps_x10 % 10u));
    }
    if ((two_vblank_pct != sMenuFpsHudPrintedTwoVBlankPct) ||
        (three_plus != sMenuFpsHudPrintedThreePlus) ||
        (max_interval != sMenuFpsHudPrintedMax))
    {
        sMenuFpsHudPrintedTwoVBlankPct = two_vblank_pct;
        sMenuFpsHudPrintedThreePlus = three_plus;
        sMenuFpsHudPrintedMax = max_interval;
        ndsPlatformPrintDebugLine(1u, "2VB %lu%% 3+ %lu MAX %lu",
                                  (unsigned long)two_vblank_pct,
                                  (unsigned long)three_plus,
                                  (unsigned long)max_interval);
    }
}
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
        ndsPlatformPublishBattleFpsHudGroup();
        consoleClear();
        NDS_BATTLE_FPS_HUD_PRINT(0u, "FPS --.-  UP --.-");
#if NDS_TICK_HUD
        /* Ring lifetime is owned by ndsBattlePlayablePacingStart(), before the
         * first sample can be written.  Do not clear it here: this HUD path is
         * lazy and may first run several presented frames into the match. */
        {
            u32 bucket;

            for (bucket = 0u; bucket < nNDSTickHudBucketDisplayCount; bucket++)
            {
                NDS_BATTLE_FPS_HUD_PRINT(11u + bucket, "%s      --       --",
                                          sBattleTickHudNames[bucket]);
            }
        }
        /* Task 66 took the row that used to hold the column legend. WORK is the
         * number the milestone is judged on (PROJECT_GOAL.md: P95 <= 1.12M) and
         * it earns the last row the console has; the two columns are still P50
         * then P95, as every row above it. n is the live window depth, so a
         * reading taken before it reaches 128 is visibly partial. */
        NDS_BATTLE_FPS_HUD_PRINT(20u, "WORK      --       -- n:0");
        NDS_BATTLE_FPS_HUD_PRINT(21u, "VBI  --  --  --");
        NDS_BATTLE_FPS_HUD_PRINT(22u, "5+  --  max --");
        NDS_BATTLE_FPS_HUD_PRINT(23u, "GIT %s TICKHUD", NDS_TASK10_GIT_SHORT);
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
    sBattleFpsHudLastTick = now_tick;
    sBattleFpsHudLastPresentedFrames = presented_frames;
    sBattleFpsHudLastLogicFrames = logic_frames;

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
                    NDS_BATTLE_FPS_HUD_PRINT(
                        11u + bucket, "%s%8lu %8lu",
                        sBattleTickHudNames[bucket],
                        (unsigned long)sBattleTickHudP50[bucket],
                        (unsigned long)sBattleTickHudP95[bucket]);
                }
            }
            NDS_BATTLE_FPS_HUD_PRINT(
                20u, "WORK%8lu %8lu n:%lu",
                (unsigned long)sBattleTickHudP50[nNDSTickHudBucketWork],
                (unsigned long)sBattleTickHudP95[nNDSTickHudBucketWork],
                (unsigned long)count);
            /* Presentation-interval histogram, cumulative since HUD reset.
             * Device A/B reports read this, never min FPS, because one frame
             * crossing the 4->5 VBlank boundary reads as 12 FPS while the
             * histogram stays continuous. */
            NDS_BATTLE_FPS_HUD_PRINT(
                21u, "VBI 2:%-5lu 3:%-5lu 4:%-5lu",
                (unsigned long)gNdsBattlePlayablePacingPresentIntervalBucket[2u],
                (unsigned long)gNdsBattlePlayablePacingPresentIntervalBucket[3u],
                (unsigned long)gNdsBattlePlayablePacingPresentIntervalBucket[4u]);
            NDS_BATTLE_FPS_HUD_PRINT(
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
            NDS_BATTLE_FPS_HUD_PRINT(
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
        (updates_x10 != sBattleFpsHudPrintedUpdatesX10) ||
        ((gNdsGx3dUnderflowFrames + gNdsGx3dRamOverflowFrames) !=
         sBattleFpsHudPrintedGx3dFaults))
    {
        sBattleFpsHudPrintedFpsX10 = fps_x10;
        sBattleFpsHudPrintedUpdatesX10 = updates_x10;
        sBattleFpsHudPrintedGx3dFaults =
            gNdsGx3dUnderflowFrames + gNdsGx3dRamOverflowFrames;
        if (sBattleFpsHudPrintedGx3dFaults == 0u)
        {
            NDS_BATTLE_FPS_HUD_PRINT(0u, "FPS %lu.%lu  UP %lu.%lu",
                                      (unsigned long)(fps_x10 / 10u),
                                      (unsigned long)(fps_x10 % 10u),
                                      (unsigned long)(updates_x10 / 10u),
                                      (unsigned long)(updates_x10 % 10u));
        }
        else
        {
            NDS_BATTLE_FPS_HUD_PRINT(0u, "FPS %lu.%lu  UP %lu.%lu  L%lu R%lu",
                                      (unsigned long)(fps_x10 / 10u),
                                      (unsigned long)(fps_x10 % 10u),
                                      (unsigned long)(updates_x10 / 10u),
                                      (unsigned long)(updates_x10 % 10u),
                                      (unsigned long)gNdsGx3dUnderflowFrames,
                                      (unsigned long)gNdsGx3dRamOverflowFrames);
        }
    }
}

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
    /* P2-3f53 natural-path proof reads these three values at this exact stop.
     * Publish them as one declared group so the debugger observes the same completed
     * frame generation instead of whatever ARM9 D-cache state happens to have
     * reached main RAM. */
    NDS_PUBLISH_DEBUGGER_GROUP(NDS_EFDESC_NATIVE_GROUP);
}

static u32 ndsPlatformMixDebugValue(u32 hash, u32 value)
{
    hash ^= value;
    return hash * 16777619u;
}

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

void ndsPlatformClearBattleTextHud(void)
{
    /* The lower screen is part of the held picture: an exit's clear waits
     * for the Thaw's, the next scene's ends the hold. */
    if (sNdsTransitionHold == NDS_TRANSITION_HOLD_EXIT)
    {
        return;
    }
    ndsPlatformTransitionThaw();
    gNdsBattleTextHudClearCount++;
    ndsBattleHudClear();
    if ((sBattleFpsHudSampleReady != 0u) ||
        (sBattleTextHudReady != FALSE))
    {
        consoleClear();
    }
    sBattleFpsHudSampleReady = 0u;
    sBattleFpsHudPrintedFpsX10 = 0xffffffffu;
    sBattleFpsHudPrintedGx3dFaults = 0xffffffffu;
    gNdsGx3dUnderflowFrames = 0u;
    gNdsGx3dRamOverflowFrames = 0u;
    gNdsGx3dRdlinesMin = 46u;
    sBattleTextHudReady = FALSE;
    sBattleTextHudFingerprint = 0xffffffffu;
}


void ndsPlatformRenderDebugHud(void)
{
#if NDS_P2_MENU_SHELL
    if (gNdsMenuShellScreen < NDS_MENU_SHELL_SCREEN_COUNT)
    {
        ndsPlatformRenderMenuFpsHud();
    }
    else
#endif
    if (gNdsBattlePlayablePacingDrawCalls != 0u)
    {
        ndsPlatformRenderBattleFpsHud();
        ndsPlatformRenderBattleTextHud();
    }
}

u32 ndsPlatformVBlankCount(void)
{
    return sVBlankCount;
}

void ndsPlatformSchedulePresentAtVBlank(u32 vblank)
{
    sEarliestPresentVBlank = vblank;
}

/* THE FRAME'S IDLE TIME (P2-2p8, 2026-09-30). Between a frame's last work and
 * the VBlank it is presented at, the ARM9 sleeps. The fighter motion prefetch
 * (reloc_backend_assets.c) issues and lands its ARM7 reads there, checking
 * ndsPlatformTicksToPresentVBlank before each bounded step so the present
 * never moves. Weak so a target without the prefetch links. */
__attribute__((weak)) void ndsR2AnimPrefetchIdle(void)
{
}

static u32 sIdlePresentVBlank;
/* A led frame's idle ends its lead before the retrace (ndsPlatformPresent-
 * LeadWait): the scanlines the deadline comes early by. */
static u32 sIdleLeadLines;

/* cpuGetTiming ticks (2,130 a scanline) until the VBlank this frame will be
 * presented at: the next one, or `earliest` when the pacing scheduled a later
 * one. */
u32 ndsPlatformTicksToPresentVBlank(void)
{
    const u32 line = REG_VCOUNT;
    u32 lines = (line < 192u) ? (192u - line) : ((263u - line) + 192u);
    const s32 ahead = (s32)(sIdlePresentVBlank - sVBlankCount);

    if ((sIdlePresentVBlank != 0u) && (ahead > 1))
    {
        lines += 263u * (u32)(ahead - 1);
    }
    lines = (lines > sIdleLeadLines) ? (lines - sIdleLeadLines) : 0u;
    return lines * 2130u;
}

static void ndsPlatformWaitForScheduledVBlank(void)
{
    u32 earliest = sEarliestPresentVBlank;

    sEarliestPresentVBlank = 0u;
    NDS_FREEZE_DIAGNOSTICS_VBLANK_WAIT();
    sIdlePresentVBlank = earliest;
    ndsR2AnimPrefetchIdle();
    swiWaitForVBlank();
    /* The first retrace after the flush swapped the frame in: the palette
     * rewrites its draws queued land now, before line 214 renders it. */
    ndsRendererCommitDeferredPalettes();
    while ((earliest != 0u) &&
           ((s32)(sVBlankCount - earliest) < 0))
    {
        swiWaitForVBlank();
    }
}

/* What the swap retrace commits for the frame it shows: everything after the
 * wait in EndFrame, run there or, for a led frame, on the present thread. */
static void ndsPlatformPostSwapCommit(void)
{
    /* Single final fade application, after all draws: the published lbFade
     * frame (if any) becomes the MASTER_BRIGHT level for the commit below.
     * Covers 3D, both staging layers, and fade-only frames with no staging
     * commit; blackout precedence resolves inside the sole register owner. */
    ndsLBFadePushHardwareFrame();
    ndsPlatformCommitNativeWallpaperAffine();
    ndsRendererHardwareCommitPendingTextureRefreshes();
    ndsIFCommonNativeOamCommit();
    ndsResultsOamCommit();
    ndsSource2DCommit();
#if NDS_P2_UI_KIT
    /* P2-1c. After the battle's OBJ tenant, because the two share one shadow
     * OAM and the later publisher wins; they are never live in the same scene,
     * so this ordering only decides which one pays for the oamUpdate. */
    ndsUiKitCommit();
#endif
    /* A layer written this frame shows with the OBJ planes committed above. */
    ndsPlatformRevealOriginalSpriteOverlay();
    /* The next scene's first complete frame replaces the snapshot. */
    if ((sNdsTransitionSnapshotBank != 0u) &&
        (sNdsTransitionHold == NDS_TRANSITION_HOLD_NONE) &&
        (ndsVideoGetTransitionBlackout() == FALSE))
    {
        ndsPlatformTransitionSnapshotRestore();
        NDS_DIAG(gNdsTransitionSnapshotReleaseCount++);
    }
    /* Release brightness only after this frame's BG/OAM owners committed. */
    ndsVideoBlackoutCommit();
    sTicks++;
}

/* BOUNDED PIPELINING (owner 2026-10-19). A battle frame used to flush, sleep
 * to its swap retrace and only then run the next frame's game logic, so a
 * heavy logic frame (a status change's clip load and parse, a list
 * materialized for its new pose) could not use the spare time the frame
 * before it left: 6-15% of the cast's frames took three VBlanks on eight
 * stages. Now, while the battle host enables it, EndFrame returns once the
 * swap retrace is at most gNdsPresentLeadLines scanlines away (half a VBlank
 * by default, the owner's bound on the added input latency, ~8 ms on
 * average), and the next frame's logic runs from there. Game order, the
 * replay digest and the image are unchanged: the logic still runs after the
 * draw that reads it.
 *
 * What the retrace itself must commit for the frame it shows -- the deferred
 * palettes, the fade, the wallpaper's affine, the texture refreshes, the OAM
 * planes -- cannot wait for the logic: the rendering engine reads the new
 * frame's VRAM from line 214. A thread above the main thread's priority
 * sleeps on VBlank and runs that commit at the swap retrace, preempting the
 * logic; the logic touches none of it (no DMA, VRAM, OAM or GX of its own),
 * and the next draw starts only after the commit (Retire). The flush itself
 * is held until the retrace before the scheduled one, so a frame whose draw
 * ended early still swaps at its scheduled retrace.
 *
 * A frame that needs the old path's extras -- a scene's first 3D frame
 * (BG0's reveal waits a second retrace), bank D's return, a held snapshot --
 * presents the old way. gNdsPresentLeadLines 0 is the old loop throughout
 * (one ROM A/Bs both through a boot poke). */
#ifndef NDS_PRESENT_LEAD
#define NDS_PRESENT_LEAD 0
#endif
#define NDS_PRESENT_LEAD_LIVE (NDS_RENDERER_HW_TRIANGLES && (NDS_PRESENT_LEAD != 0))
__attribute__((used)) volatile u32 gNdsPresentLeadLines = NDS_PRESENT_LEAD;
/* Frames that led, and led frames whose next logic still ran past their
 * retrace (the next draw started late). */
__attribute__((used)) volatile u32 gNdsPresentLeadFrames;
__attribute__((used)) volatile u32 gNdsPresentLeadLate;
#if NDS_PRESENT_LEAD_LIVE
#include <calico/system/mailbox.h>
static Thread sNdsPresentThread;
static u8 sNdsPresentThreadStack[2048] __attribute__((aligned(8)));
static u32 sNdsPresentMailSlots[2];
static Mailbox sNdsPresentMail;
static u32 sNdsPresentThreadStarted;
static u32 sNdsPresentLeadOn;
/* Main thread: a led frame waits for Retire. */
static u32 sNdsPresentLeadPending;
/* Present thread: the retrace to commit at, and the frame waiting for it. */
static volatile u32 sNdsPresentLeadSwap;
static volatile u32 sNdsPresentLeadArmed;

static int ndsPlatformPresentThreadMain(void *arg)
{
    (void)arg;
    for (;;)
    {
        threadWaitForVBlank();
        if ((sNdsPresentLeadArmed != 0u) &&
            ((s32)(sVBlankCount - sNdsPresentLeadSwap) >= 0))
        {
            const u32 at = sVBlankCount;

            /* The first retrace after the flush swapped the frame in. */
            ndsRendererCommitDeferredPalettes();
            ndsPlatformPostSwapCommit();
            sNdsPresentLeadArmed = 0u;
            (void)mailboxTrySend(&sNdsPresentMail, at);
        }
    }
    return 0;
}

/* The flush waits for the retrace before the scheduled one, so the swap
 * lands on it rather than a retrace early. */
static void ndsPlatformPresentLeadHoldFlush(u32 earliest)
{
    while ((earliest != 0u) && ((s32)((sVBlankCount + 1u) - earliest) < 0))
    {
        NDS_FREEZE_DIAGNOSTICS_VBLANK_WAIT();
        threadWaitForVBlank();
    }
}

/* After the flush: arm the retrace's commit, then sleep until it is at most
 * gNdsPresentLeadLines away (or past). */
static void ndsPlatformPresentLeadWait(u32 swap)
{
    const u32 lead = gNdsPresentLeadLines;

    sNdsPresentLeadSwap = swap;
    sNdsPresentLeadArmed = 1u;
    sNdsPresentLeadPending = 1u;
    gNdsPresentLeadFrames++;
    if ((s32)(sVBlankCount - swap) >= 0)
    {
        /* The retrace passed between the flush and the arming: the present
         * thread woke before there was anything to commit. */
        const IrqState st = irqLock();
        const u32 armed = sNdsPresentLeadArmed;

        sNdsPresentLeadArmed = 0u;
        irqUnlock(st);
        if (armed != 0u)
        {
            ndsRendererCommitDeferredPalettes();
            ndsPlatformPostSwapCommit();
            (void)mailboxTrySend(&sNdsPresentMail, sVBlankCount);
        }
        return;
    }
    sIdlePresentVBlank = swap;
    sIdleLeadLines = lead;
    ndsR2AnimPrefetchIdle();
    for (;;)
    {
        const u32 count = sVBlankCount;
        const u32 line = REG_VCOUNT;

        if ((s32)(count - swap) >= 0)
        {
            break;
        }
        if (((count + 1u) == swap) && (line < 192u) &&
            ((192u - line) <= lead))
        {
            break;
        }
        NDS_FREEZE_DIAGNOSTICS_VBLANK_WAIT();
        /* The wake line, or the retrace itself should the line pass
         * between the test and the wait. */
        (void)threadIrqWait(true, IRQ_VCOUNT | IRQ_VBLANK);
    }
    sIdleLeadLines = 0u;
}

/* A battle frame with a scheduled retrace and none of the old path's
 * extras leads. */
static u32 ndsPlatformPresentLeadThisFrame(u32 submitted)
{
    if ((sNdsPresentLeadOn == 0u) || (submitted == 0u) ||
        (sEarliestPresentVBlank == 0u) ||
        (s3dLayerEnableOnNextPresent != FALSE) ||
        (sOriginalSpriteOverlayBg3ReturnPending != 0u))
    {
        return FALSE;
    }
    if ((sNdsTransitionSnapshotBank != 0u) ||
        (sNdsTransitionHold != NDS_TRANSITION_HOLD_NONE))
    {
        return FALSE;
    }
    return TRUE;
}
#endif

void ndsPlatformPresentLeadEnable(u32 enable)
{
#if NDS_PRESENT_LEAD_LIVE
    const u32 lead = gNdsPresentLeadLines;

    if ((enable == FALSE) || (lead == 0u) || (lead >= 192u))
    {
        (void)ndsPlatformPresentLeadRetire();
        if (sNdsPresentLeadOn != 0u)
        {
            irqDisable(IRQ_VCOUNT);
            REG_DISPSTAT &= ~DISPSTAT_IE_VCOUNT;
        }
        sNdsPresentLeadOn = 0u;
        return;
    }
    if (sNdsPresentThreadStarted == 0u)
    {
        mailboxPrepare(&sNdsPresentMail, sNdsPresentMailSlots,
                       sizeof(sNdsPresentMailSlots) /
                           sizeof(sNdsPresentMailSlots[0]));
        threadPrepare(&sNdsPresentThread, ndsPlatformPresentThreadMain, NULL,
                      &sNdsPresentThreadStack[sizeof(sNdsPresentThreadStack)],
                      MAIN_THREAD_PRIO - 1u);
        threadStart(&sNdsPresentThread);
        irqSet(IRQ_VCOUNT, ndsPlatformVCountInterrupt);
        sNdsPresentThreadStarted = 1u;
    }
    SetYtrigger((int)(192u - lead));
    REG_DISPSTAT |= DISPSTAT_IE_VCOUNT;
    irqEnable(IRQ_VCOUNT);
    sNdsPresentLeadOn = 1u;
#else
    (void)enable;
#endif
}

u32 ndsPlatformPresentLeadPending(void)
{
#if NDS_PRESENT_LEAD_LIVE
    return sNdsPresentLeadPending;
#else
    return 0u;
#endif
}

/* Before the next draw: the led frame's retrace has committed (waiting for
 * it if the logic finished early). -> the VBlank it was swapped in at. */
u32 ndsPlatformPresentLeadRetire(void)
{
#if NDS_PRESENT_LEAD_LIVE
    u32 at;

    if (sNdsPresentLeadPending == 0u)
    {
        return 0u;
    }
    sNdsPresentLeadPending = 0u;
    if (sNdsPresentLeadArmed == 0u)
    {
        gNdsPresentLeadLate++;
    }
    else
    {
        sIdlePresentVBlank = sNdsPresentLeadSwap;
        ndsR2AnimPrefetchIdle();
    }
    NDS_FREEZE_DIAGNOSTICS_VBLANK_WAIT();
    at = mailboxRecv(&sNdsPresentMail);
    return at;
#else
    return 0u;
#endif
}

void ndsPlatformEndFrame(void)
{
#if NDS_TICK_HUD
    u32 tickhud_flush_start;
    u32 tickhud_wait_start;
#endif
    u32 submitted;
#if NDS_PRESENT_LEAD_LIVE
    u32 lead = FALSE;
    u32 lead_flush_vblank = 0u;
#endif

    /* A present while a frame is held is a loading service present -- every
     * scene's own frame Thaws before its EndFrame. It keeps the pacing and
     * commits nothing: no frame consume (its cloud emit and tint-tile
     * uploads), flush, OBJ shadow, fade, wallpaper or bank D return of the
     * scene being built. Ten seconds of them end the hold regardless, so a
     * present path that never Thaws degrades to the r63 cover, not to an
     * invisible scene. */
    if (sNdsTransitionHold != NDS_TRANSITION_HOLD_NONE)
    {
        if (++sNdsTransitionHoldPresents < NDS_TRANSITION_HOLD_MAX_PRESENTS)
        {
            ndsPlatformWaitForScheduledVBlank();
            sTicks++;
            return;
        }
        ndsPlatformTransitionThaw();
    }
    /* A present path that releases its cover without the explicit call
     * still starts the music its scene asked for during the load. */
    if (ndsPlatformSceneLoading() == FALSE)
    {
        ndsAudioReleaseDeferredBGM();
    }
    submitted = ndsRendererHardwareConsumeSubmittedFrame();
#if NDS_PRESENT_LEAD_LIVE
    lead = ndsPlatformPresentLeadThisFrame(submitted);
#endif
    if ((submitted != 0u) || (sOriginalSpriteOverlayNeedsFlush != FALSE))
    {
        if (submitted != 0u)
        {
            NDS_DIAG(gNdsHardwareRendererSubmittedFrameCount++);
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
#if NDS_SHIP_TELEMETRY || NDS_TICK_HUD
        gNdsHardwareRendererPolyRamCount = GFX_POLYGON_RAM_USAGE;
        gNdsHardwareRendererVertexRamCount = GFX_VERTEX_RAM_USAGE;
        gNdsHardwareRendererStatus = GFX_STATUS;
        gNdsHardwareRendererControl = GFX_CONTROL;
#endif
        NDS_FREEZE_DIAGNOSTICS_FLUSH();
        /* The previous frame's 3D faults (gNdsGx3dUnderflowFrames above):
         * DISP3DCNT bits 12 and 13 latch until written back as 1. */
        {
            const u32 control = GFX_CONTROL;
            const u32 rdlines = *(vu8 *)0x04000320u;

            if (rdlines < gNdsGx3dRdlinesMin)
            {
                gNdsGx3dRdlinesMin = rdlines;
            }
            if ((control & ((1u << 12) | (1u << 13))) != 0u)
            {
                if ((control & (1u << 12)) != 0u)
                {
                    gNdsGx3dUnderflowFrames++;
                }
                if ((control & (1u << 13)) != 0u)
                {
                    gNdsGx3dRamOverflowFrames++;
                }
                GFX_CONTROL = control;
            }
        }
#if NDS_TICK_HUD
        tickhud_flush_start = cpuGetTiming();
#endif
        /* P2-2p3: a fighter packet DMA may still be draining into the FIFO;
         * the flush is a FIFO command. Above the Task 29 record on purpose --
         * check-gbi-decode-fixtures pins the record adjacent to the flush. */
        ndsRendererFighterPacketDmaWait();
#if NDS_PRESENT_LEAD_LIVE
        if (lead != FALSE)
        {
            /* Idle until the retrace before the scheduled one: WAIT, not the
             * flush's MISC. */
#if NDS_TICK_HUD
            const u32 hold_start = cpuGetTiming();
#endif
            ndsPlatformPresentLeadHoldFlush(sEarliestPresentVBlank);
#if NDS_TICK_HUD
            {
                const u32 held = cpuGetTiming() - hold_start;

                gNdsTickHudVBlankWaitTicks += held;
                tickhud_flush_start += held;
            }
#endif
        }
#endif
        glFlush(GL_TRANS_MANUALSORT);
#if NDS_PRESENT_LEAD_LIVE
        lead_flush_vblank = sVBlankCount;
#endif
#if NDS_TICK_HUD
        gNdsTickHudFlushTicks += cpuGetTiming() - tickhud_flush_start;
#endif
        if (submitted != 0u)
        {
            NDS_DIAG(gNdsHardwareRendererFlushCount++);
        }
        else
        {
            /* Overlay-transform-only flush: no 3D frame was submitted, so
             * this is not a hardware frame flush (see the counter's own
             * comment at its definition). */
            NDS_DIAG(gNdsHardwareRendererOverlayOnlyFlushCount++);
        }
        sOriginalSpriteOverlayNeedsFlush = FALSE;
    }
#if NDS_TICK_HUD
    tickhud_wait_start = cpuGetTiming();
#endif
#if NDS_PRESENT_LEAD_LIVE
    if (lead != FALSE)
    {
        /* The swap is the retrace after the flush; its commit is the
         * present thread's, and the next logic starts at the lead. */
        sEarliestPresentVBlank = 0u;
        ndsPlatformPresentLeadWait(lead_flush_vblank + 1u);
#if NDS_TICK_HUD
        gNdsTickHudVBlankWaitTicks += cpuGetTiming() - tickhud_wait_start;
#endif
        return;
    }
#endif
    ndsPlatformWaitForScheduledVBlank();
#if NDS_TICK_HUD
    gNdsTickHudVBlankWaitTicks += cpuGetTiming() - tickhud_wait_start;
#endif
    if ((submitted != 0u) && (s3dLayerEnableOnNextPresent != FALSE))
    {
        /* The swap retrace starts rasterizing the new GX list. Keep BG0
         * hidden until the following retrace can display that completed list.
         * Only a hidden -> visible ownership change pays this wait. */
        swiWaitForVBlank();
        REG_DISPCNT |= DISPLAY_BG0_ACTIVE;
        s3dLayerEnableOnNextPresent = FALSE;
    }
    /* P2-2p8 Phase 1 slice 2b: a battle's pending bank D return lands here,
     * after the VBlank that made the next scene's first 3D frame the displayed
     * one -- from now on no displayed geometry reads D. */
    if ((submitted != 0u) && (sOriginalSpriteOverlayBg3ReturnPending != 0u))
    {
        ndsPlatformVramReturnBankDNow();
    }
    else if ((sOriginalSpriteOverlayBg3Lent != 0u) &&
             (sOriginalSpriteOverlayBg3ReturnPending == 0u) &&
             (&gNdsSceneManagerCurrIsBattle != NULL) &&
             (gNdsSceneManagerCurrIsBattle == 0u) &&
             (ndsFtrLeanAdmitBattleExit != NULL))
    {
        /* Belt and braces: the texture side lets go now, and D comes back
         * after the next displayed 3D frame. */
        gNdsVramBankDMissedExits++;
        ndsFtrLeanAdmitBattleExit();
#if NDS_TICK_HUD
        DC_FlushAll();
#endif
    }
#if defined(NDS_P2_1P_GAME) && NDS_P2_1P_GAME && \
    defined(NDS_P2_MENU_WALK) && NDS_P2_MENU_WALK && !NDS_TICK_HUD
    /* P2-2p8 Phase 1 slice 3 lab (the 1P campaign walk ROM, no tick HUD):
     * the campaign probe reads the bank D words through GDB, which sees
     * memory, not the D-cache -- clean them every frame so no read is a
     * stale line. Slice 7: keyed on that ROM's own contract
     * (scripts/menus/probe-p2-campaign.ps1 requires NDS_P2_1P_GAME=1 and the
     * walk), since the lab-only NDS_FTR_LEAN_ADMIT_LAB is gone -- admission 2
     * is the default. The VS walk ROMs (shell, shell loop) stay without it. */
    DC_FlushRange((const void *)&gNdsVramBankDTakes, sizeof(u32));
    DC_FlushRange((const void *)&gNdsVramBankDReturns, sizeof(u32));
    DC_FlushRange((const void *)&gNdsVramBankDMissedExits, sizeof(u32));
    DC_FlushRange((const void *)&gNdsVramBankDLent, sizeof(u32));
    DC_FlushRange((const void *)&gNdsVramBg3RefusedWrites, sizeof(u32));
#endif
    ndsPlatformPostSwapCommit();
}

u32 ndsPlatformTicks(void)
{
    return sTicks;
}

u32 ndsPlatformHeldKeys(void)
{
    return sHeldKeys;
}
