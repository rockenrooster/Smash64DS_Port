#ifndef SSB64_NDS_VIDEO_BOOTSTRAP_H
#define SSB64_NDS_VIDEO_BOOTSTRAP_H

#include <PR/ultratypes.h>

#define NDS_VIDEO_BOOTSTRAP_PASS 0x56494430u

extern volatile u32 gNdsVideoBootstrapResult;

void ndsVideoBootstrapStart(void);
void ndsVideoBootstrapUpdate(void);
/* Apply queued blackout changes in the platform's VBlank commit window. */
void ndsVideoBlackoutCommit(void);
/* Port loading cover (see include/sys/video.h): set at a scene exit, released
 * only by a scene-owned complete draw. */
void ndsVideoSetTransitionBlackout(s32 black);
s32 ndsVideoGetTransitionBlackout(void);
/* Source-fade latch (BattleShip lbFade, black-only): fade-down level 0..16,
 * pushed once per frame by ndsLBFadePushHardwareFrame() after all draws and
 * resolved against blackout (which wins) in the same commit. Sole register
 * owner stays src/port/video_blackout.c. */
void ndsVideoSetSourceFade(u32 level);
u32 ndsVideoGetSourceFade(void);
/* Pure blackout/fade resolve to a MASTER_BRIGHT value (host-testable):
 * 0 when clear, else fade-down mode (2<<14) ORed with max(blackout?16:0,
 * clamped fade). */
u16 ndsVideoResolveBrightnessValue(u32 blackout, u32 fade_level);
/* Scene fade (P2-6 ending): a scene whose source paints a full-viewport
 * black or white PRIM rectangle over its 3D (mvending.c:261-360) drives the
 * same registers instead -- down = black level 0..16, up = white level 0..16.
 * Black wins over white, and blackout/source fade win over both. Persists
 * until the scene that follows clears it at its first draw. */
void ndsVideoSetSceneFade(u32 down_level, u32 up_level);

#endif
