#ifndef SSB64_NDS_LB_LBFADE_DS_H
#define SSB64_NDS_LB_LBFADE_DS_H

#include <ssb_types.h>

/* DS seam for BattleShip lb/lbfade.c (decomp lb/lbfade.c:34-90).
 *
 * Lifecycle (update timing, callback/eject, alpha math) is owned by
 * src/import/battleship_lbfade.c with source-exact semantics. This header is
 * the narrow contract the platform compositor consumes.
 *
 * FINAL HARDWARE PATH (single application, after draw):
 * - Source draws the fade with G_RM_CLD_SURF (lbfade.c:71): src-over of the
 *   primitive color over the WHOLE framebuffer. The DS staging buffers cannot
 *   express that (foreground/background staging pixels use 0 as transparent
 *   3D show-through, with no per-pixel alpha), and blending into staging
 *   before the platform commit both misses pixels committed before the fade
 *   display proc runs and never reaches 3D or fade-only frames (no staging
 *   draws means no commit gate). A staging blend PLUS a hardware pass would
 *   double-fade. So there is exactly one application: ndsPlatformEndFrame
 *   calls ndsLBFadePushHardwareFrame() after all draws, which peeks the
 *   frame published by lbFadeProcDisplay and latches it into the
 *   master-brightness owner (src/port/video_blackout.c); the existing
 *   post-VBlank ndsVideoBlackoutCommit() writes the registers. Per-frame
 *   lifetime holds because each push consumes the published frame. Native
 *   screens with no SObj draw therefore recover on their next present too.
 * - Black-only scope: every actual lbFadeMakeActor caller in source passes
 *   {0,0,0,...} (proven by scripts/menus/test_source_fade.py, which pins
 *   each d*FadeColor). The fade is therefore a fade-down-to-black, exactly
 *   what DS MASTER_BRIGHT expresses with no new layer and no VRAM touched
 *   (BG0=3D, BG2/BG3=staging stay as ndsPlatformInit left them; BG1 is
 *   untouched because nothing needs it).
 * - Quantization: source alpha 0..255 maps to native brightness steps 0..16
 *   via (alpha*16+127)/255 (round-to-nearest; endpoints exact, worst error
 *   half a step = ~8/255). Adjacent source ticks can share one visible step;
 *   that is the hardware grid, not a lost update.
 * - Rect delta: source fills [10,10,310,230] (GS_SCREEN_* - 10 inset), leaving
 *   the 10 px overscan border unfaded. MASTER_BRIGHT has no window, so the DS
 *   fades the full screen instead (DS-mapped rect would be [8,8,248,184] at
 *   the 4/5 viewport scale). This outer-ring delta needs visual acceptance
 *   in scenes whose DS presentation extends to the screen edge.
 * - Blackout precedence: SYVIDEO_FLAG_BLACKOUT (full black, level 16) wins
 *   over any fade level; the resolve is max() in the sole MASTER_BRIGHT
 *   owner, committed once per frame.
 * - Out of scope: sc1PGameBossProcDisplayFadeColor/Alpha are a separate boss
 *   wallpaper proc pair, not lbFade callers, and are not covered here. A
 *   non-black lbFade color (no caller today) publishes no hardware level
 *   rather than a wrong hue; no generic arbitrary-RGB backend is invented.
 *
 * Staging blends were deleted with this path: no software fade code remains. */

extern volatile u32 gNdsLBFadeCreateCount;
extern volatile u32 gNdsLBFadeDisplayCount;

/* Source rect (lbfade.c:72): GS_SCREEN_* - 10 inset, 320x240 framebuffer
 * (decomp config.h). Inclusive bounds. Documented for the rect delta above;
 * the hardware path does not clip to it. No per-scene rects. */
#define NDS_LBFADE_RECT_ULX 10
#define NDS_LBFADE_RECT_ULY 10
#define NDS_LBFADE_RECT_LRX 310
#define NDS_LBFADE_RECT_LRY 230

/* Native MASTER_BRIGHT fade-down steps (level 16 = full black). */
#define NDS_LBFADE_HW_LEVEL_MAX 16u

/* Source-exact display alpha (lbfade.c:61-66): linear current/max ramp scaled
 * to 0..255, inverted when the base color carries a==0 (fade-from). The
 * max<=0 arm is a host-safe guard (AVOID_UB): no caller passes 0 today (min
 * 10), and a zero-length fade is already at its end state. */
s32 ndsLBFadeComputeAlpha(s32 current, s32 max, u8 base_alpha);

/* Pure source-alpha (0..255) to hardware level (0..16) mapping:
 * (alpha*16+127)/255. Endpoints exact; worst error half a step. */
u32 ndsLBFadeAlphaToHardwareLevel(u8 alpha);

/* Non-destructive read of the published frame as a hardware level. Returns
 * TRUE with 0..16 when lbFadeProcDisplay ran since the last discard AND the
 * published color is black (r==g==b==0); FALSE (level 0) means no fade this
 * frame, including any non-black color, which this path does not express.
 * The latch is kept: exactly one consumer (the EndFrame push) reads per
 * frame; the push consumes it, and SObj BeginFrame also drops stale state. */
u32 ndsLBFadePeekHardwareLevel(u32 *out_level);

/* Non-destructive read of the published frame (same out contract, latch
 * kept). Kept for diagnostics/host tests; the platform hook uses the level
 * form above. */
u32 ndsLBFadePeekFrame(u8 *out_r, u8 *out_g, u8 *out_b, u8 *out_alpha);

/* Drops the published frame without consuming. Called from
 * ndsSObjPreviewBeginFrame only. */
void ndsLBFadeDiscardFrame(void);

/* Final per-frame application, called once from ndsPlatformEndFrame after
 * all draws and before ndsVideoBlackoutCommit: peeks the hardware level
 * (0 when inactive/non-black) and latches it into the MASTER_BRIGHT owner.
 * Fade-only frames fade because this needs no staging commit. */
void ndsLBFadePushHardwareFrame(void);

#endif
