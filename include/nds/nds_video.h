#ifndef SSB64_NDS_VIDEO_BOOTSTRAP_H
#define SSB64_NDS_VIDEO_BOOTSTRAP_H

#include <PR/ultratypes.h>

#define NDS_VIDEO_BOOTSTRAP_PASS 0x56494430u

extern volatile u32 gNdsVideoBootstrapResult;

void ndsVideoBootstrapStart(void);
void ndsVideoBootstrapUpdate(void);
/* Apply queued blackout changes in the platform's VBlank commit window. */
void ndsVideoBlackoutCommit(void);
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

#endif
