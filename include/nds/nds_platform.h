#ifndef SSB64_NDS_PLATFORM_H
#define SSB64_NDS_PLATFORM_H

#include <PR/ultratypes.h>

enum NDSInput {
    NDS_INPUT_LEFT  = 1u << 0,
    NDS_INPUT_RIGHT = 1u << 1,
    NDS_INPUT_UP    = 1u << 2,
    NDS_INPUT_DOWN  = 1u << 3,
    NDS_INPUT_A     = 1u << 4,
    NDS_INPUT_START = 1u << 5,
    NDS_INPUT_B     = 1u << 6,
    NDS_INPUT_X     = 1u << 7,
    NDS_INPUT_Y     = 1u << 8,
    NDS_INPUT_L     = 1u << 9,
    NDS_INPUT_R     = 1u << 10
};

enum NDSOriginalSpriteOverlayLayer {
    NDS_ORIGINAL_SPRITE_OVERLAY_BACKGROUND = 1u << 0,
    NDS_ORIGINAL_SPRITE_OVERLAY_FOREGROUND = 1u << 1,
    NDS_ORIGINAL_SPRITE_OVERLAY_ALL =
        NDS_ORIGINAL_SPRITE_OVERLAY_BACKGROUND |
        NDS_ORIGINAL_SPRITE_OVERLAY_FOREGROUND
};

typedef enum NDSFastWallpaperState {
    NDS_FAST_WALLPAPER_UNSEEDED = 0,
    NDS_FAST_WALLPAPER_CAPTURING,
    NDS_FAST_WALLPAPER_READY,
    NDS_FAST_WALLPAPER_STATIC_DEGRADED
} NDSFastWallpaperState;

/* Hardware-triangle builds use the retained sprite-display buffer only as the
 * immutable 300x220 wallpaper decode plus renderer scratch. Five 300-pixel
 * rows leave 1,500 u16 scratch pixels; the renderer statically proves its map
 * requirement fits this capacity. Software builds keep their legacy 320x240
 * retained display surface and do not use this decode-cache API. */
#define NDS_ORIGINAL_SPRITE_DECODE_CACHE_WIDTH 300u
#define NDS_ORIGINAL_SPRITE_DECODE_CACHE_CONTENT_HEIGHT 220u
#define NDS_ORIGINAL_SPRITE_DECODE_CACHE_HEIGHT 225u
#define NDS_ORIGINAL_SPRITE_DECODE_CACHE_SCRATCH_PIXELS \
    ((NDS_ORIGINAL_SPRITE_DECODE_CACHE_HEIGHT - \
      NDS_ORIGINAL_SPRITE_DECODE_CACHE_CONTENT_HEIGHT) * \
     NDS_ORIGINAL_SPRITE_DECODE_CACHE_WIDTH)

void ndsPlatformInit(void);
u32 ndsPlatformReadInput(void);
void ndsPlatformBeginFrame(void);
void ndsPlatformDrawRect(s32 x, s32 y, s32 width, s32 height, u16 color);
u16 *ndsPlatformBeginOriginalSpritePreview(u32 width, u32 height,
                                           s32 n64_x, s32 n64_y,
                                           u32 *out_pitch);
u16 *ndsPlatformGetOriginalSpriteDecodeCache(u32 *out_pitch,
                                              u32 *out_height,
                                              u32 *out_epoch);
u16 *ndsPlatformGetOriginalSpriteOverlayLayer(s32 is_foreground,
                                               u32 *out_pitch,
                                               u32 *out_width,
                                               u32 *out_height,
                                               u32 *out_epoch);
u32 ndsPlatformCommitOriginalSpriteFinalLayer(s32 is_foreground,
                                               u32 pixel_write_count);
void ndsPlatformCommitOriginalSpritePreview(void);
void ndsPlatformCommitOriginalSpritePreviewLayer(s32 is_foreground);
void ndsPlatformClearOriginalSpriteOverlayLayer(s32 is_foreground);
void ndsPlatformClearOriginalSpritePreview(void);
void ndsPlatformSetOriginalSpriteOverlayLayerMask(u32 layer_mask);
void ndsPlatformSetOriginalSpriteOverlayEnabled(s32 is_enabled);
/* Applies BG2's queued affine transform immediately rather than at the next
 * present. A caller that draws into the overlay bitmap between a clear and
 * the first present of a scene needs this, or one frame renders under the
 * previous scene's transform. */
void ndsPlatformCommitOriginalSpriteOverlayTransform(void);
u32 ndsPlatformFastWallpaperCanSeed(void);
u32 ndsPlatformFastWallpaperBeginSeed(s32 origin_x, s32 origin_y,
                                       u32 scale_x_q16,
                                       u32 scale_y_q16,
                                       u32 asset_identity);
u32 ndsPlatformFastWallpaperFinishSeed(u32 software_draw_succeeded);
u32 ndsPlatformFastWallpaperQueueTransform(s32 origin_x, s32 origin_y,
                                            u32 scale_x_q16,
                                            u32 scale_y_q16,
                                            u32 asset_identity);
void ndsPlatformFastWallpaperRecordSoftwareDraw(void);
void ndsPlatformFastWallpaperReset(void);
u32 ndsPlatformSceneWallpaperQueueTransform(s32 origin_x, s32 origin_y,
                                             u32 scale_x_q16,
                                             u32 scale_y_q16);
void ndsPlatformSceneWallpaperConfirmRaster(void);
u32 ndsPlatformSceneMipCaptureRequest(u32 mip_index);
void ndsPlatformSceneMipCacheAbort(void);
u32 ndsPlatformSceneMipCaptureCompletedCount(void);
u32 ndsPlatformSceneMipCacheReady(void);
u32 ndsPlatformSceneMipCacheFailed(void);
u16 *ndsPlatformBeginOriginalDLPreview(u32 width, u32 height,
                                       u32 *out_pitch);
void ndsPlatformCommitOriginalDLPreview(void);
void ndsPlatformClearOriginalDLPreview(void);
extern volatile u32 gNdsOriginalSpritePreviewReady;
extern volatile u32 gNdsOriginalSpritePreviewCommitCount;
extern volatile u32 gNdsOriginalSpritePreviewDrawCount;
extern volatile u32 gNdsOriginalSpritePreviewDisplayWidth;
extern volatile u32 gNdsOriginalSpritePreviewDisplayHeight;
extern volatile u32 gNdsOriginalSpriteBg2ClearBytes;
extern volatile u32 gNdsOriginalSpriteBg2CopyBytes;
extern volatile u32 gNdsOriginalSpriteBg2FinalWriteBytes;
extern volatile u32 gNdsOriginalSpriteBg3ClearBytes;
extern volatile u32 gNdsOriginalSpriteBg3CopyBytes;
extern volatile u32 gNdsOriginalSpriteBg3FinalWriteBytes;
extern volatile u32 gNdsOriginalDLPreviewReady;
extern volatile u32 gNdsOriginalDLPreviewWidth;
extern volatile u32 gNdsOriginalDLPreviewHeight;
extern volatile u32 gNdsOriginalDLPreviewCommitCount;
extern volatile u32 gNdsOriginalDLPreviewDrawCount;
extern volatile u32 gNdsPerfPresentFps;
extern volatile u32 gNdsPerfLogicFps;
extern volatile u32 gNdsPerfDLDrawFps;
extern volatile u32 gNdsPerfPreviewCommitFps;
extern volatile u32 gNdsPerfPreviewCommitCount;
extern volatile u32 gNdsPerfSampleCount;
extern volatile u32 gNdsPerfSampleWindowTicks;
extern volatile u32 gNdsBattlePlayableHudFpsX10;
extern volatile u32 gNdsBattlePlayableHudFpsSampleCount;
extern volatile u32 gNdsBattlePlayableHudFpsFrameWindow;
extern volatile u32 gNdsBattlePlayableHudFpsTickWindow;
extern volatile u32 gNdsBattleTextHudRenderCount;
extern volatile u32 gNdsBattleTextHudChangeCount;
extern volatile u32 gNdsBattleTextHudFingerprint;
extern volatile u32 gNdsBattleTextHudTimeSeconds;
extern volatile u32 gNdsBattleTextHudP0Damage;
extern volatile u32 gNdsBattleTextHudP1Damage;
extern volatile u32 gNdsBattleTextHudP0Stock;
extern volatile u32 gNdsBattleTextHudP1Stock;
extern volatile u32 gNdsBattleTextHudActiveMask;
extern volatile u32 gNdsBattleTextHudShowDamageMask;
extern volatile u32 gNdsBattleTextHudClearCount;
extern volatile u32 gNdsHardwareRendererSubmittedFrameCount;
extern volatile u32 gNdsHardwareRendererFlushCount;
extern volatile u32 gNdsHardwareRendererPolyRamCount;
extern volatile u32 gNdsHardwareRendererVertexRamCount;
extern volatile u32 gNdsHardwareRendererStatus;
extern volatile u32 gNdsHardwareRendererControl;
extern volatile u32 gNdsFastWallpaperState;
extern volatile u32 gNdsFastWallpaperSeedAttemptCount;
extern volatile u32 gNdsFastWallpaperSeedSuccessCount;
extern volatile u32 gNdsFastWallpaperSeedFailureCount;
extern volatile u32 gNdsFastWallpaperStaticDegradedCount;
extern volatile u32 gNdsFastWallpaperSeedTicks;
extern volatile u32 gNdsFastWallpaperQueueCount;
extern volatile u32 gNdsFastWallpaperApplyCount;
extern volatile u32 gNdsFastWallpaperUnchangedSkipCount;
extern volatile u32 gNdsFastWallpaperClampXCount;
extern volatile u32 gNdsFastWallpaperClampYCount;
extern volatile u32 gNdsFastWallpaperClampScaleCount;
extern volatile u32 gNdsFastWallpaperInvalidTransformCount;
extern volatile u32 gNdsFastWallpaperReusePreviousCount;
extern volatile u32 gNdsFastWallpaperAffineLastTicks;
extern volatile u32 gNdsFastWallpaperPostReadySoftwareDrawCount;
extern volatile u32 gNdsFastWallpaperPostReadyPixelWriteCount;
extern volatile u32 gNdsFastWallpaperSeedHash;
extern volatile u32 gNdsFastWallpaperSeedOpaquePixelCount;
extern volatile u32 gNdsFastWallpaperSeedRestoreMismatchCount;
void ndsPlatformRenderDebugHud(void);
void ndsPlatformClearBattleTextHud(void);
/* Push one presented-iteration snapshot of gNdsTickHudBuckets into the tick-HUD
 * percentile window. Must be called from the per-frame path: the HUD renderer
 * itself only runs about twice a second, so sampling there would build the
 * distribution out of half-second-spaced single frames. No-op unless the tick
 * HUD is compiled in. */
void ndsPlatformTickHudSample(void);
u32 ndsPlatformVBlankCount(void);
void ndsPlatformSchedulePresentAtVBlank(u32 vblank);
void ndsPlatformEndFrame(void);
void ndsPlatformProfileSampleFrameBoundaryGXState(void);
u32 ndsPlatformTicks(void);
u32 ndsPlatformHeldKeys(void);

/* PUBLISHING A DEBUGGER-VISIBLE COUNTER GROUP -- the one form, in one place.
 *
 * A "group" here is a set of globals the ROM writes and only GDB reads, whose
 * members a harness cross-checks against each other at a stop. melonDS reads
 * them through `ARMv5::ReadMem` (`src/ARM.cpp:1545`), which special-cases ITCM
 * and DTCM and otherwise falls through to `BusRead32` -- there is no DCache
 * lookup anywhere on that path. A word still sitting dirty in the ARM9 data
 * cache therefore reads STALE over GDB, and a group only PARTLY resident in
 * main RAM is observed TORN: one member current, its neighbour one behind.
 * ARM946E-S does not write-allocate, which is what makes the tearing possible
 * at all -- a store to a non-resident line reaches RAM, while the next store
 * to a line a load has since filled only marks it dirty and aborts the bus
 * write.
 *
 * This has now been diagnosed three times as three separate defects:
 *   * R2-04 E2, the FPS-HUD group, measured frame-by-frame in
 *     `artifacts/verification/2026-08-15_fpshud-publication.txt`;
 *   * `KNOWN_ISSUES.md`'s `phaseLag=-1` under NDS_R2_CAMERA_MATRIX_LEAN=3
 *     (2026-08-09), which held that lever off by default for six days;
 *   * the resident-battlepack arm's `drawLead=-1` (2026-08-15).
 * In every one the counter written FIRST is the one that reads low, and in
 * every one the trigger was a change that moved heap layout -- which is what
 * decides which lines are resident when the debugger halts.
 *
 * So the group is declared ONCE as an X-macro list beside its externs, and the
 * publish is GENERATED from that list. A member cannot be added to a group and
 * silently left unpublished, because there is only one list and it is both the
 * group and the flush.
 *
 * Per member rather than as one span: the flush must not depend on the linker
 * keeping separate objects adjacent or in declaration order. Each call cleans
 * the containing line(s), so the cost is one CP15 clean per line however they
 * are laid out.
 *
 * Publish where the group is CONSISTENT and BEFORE the stop can observe it.
 * GDB breaks on a function's entry, so publishing inside the marker the
 * harness breaks on is too late -- it must precede the call. */
#define NDS_PUBLISH_DEBUGGER_GROUP_MEMBER(sym) \
    DC_FlushRange((const void *)&(sym), sizeof(sym));

#define NDS_PUBLISH_DEBUGGER_GROUP(members) \
    do { members(NDS_PUBLISH_DEBUGGER_GROUP_MEMBER) } while (0)

/* The FPS_HUD marker's group, in marker field order. */
#define NDS_BATTLE_FPS_HUD_GROUP(X) \
    X(gNdsBattlePlayableHudFpsX10) \
    X(gNdsBattlePlayableHudFpsSampleCount) \
    X(gNdsBattlePlayableHudFpsFrameWindow) \
    X(gNdsBattlePlayableHudFpsTickWindow)

/* Clean every group the frame-complete stop reads as a self-consistent set out
 * of the D-cache. Called immediately before ndsBattlePlayableFrameCompleteMarker(),
 * which is the function the realtime harness breaks on; the group lists live in
 * nds_startup.h beside the externs.
 *
 * It publishes BPLAY_PACE and GCRUNALL_TASKMAN together because the harness
 * cross-checks ACROSS them (`taskmanPresentLead`), and publishing one side of a
 * subtraction is not a fix -- it just moves which counter is free to read
 * stale. A group joins this seam when a harness compares one of its members to
 * another live counter, not merely when it is printed. */
void ndsPlatformPublishBattleFrameCompleteGroups(void);

#endif
