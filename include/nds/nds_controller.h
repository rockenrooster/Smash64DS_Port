#ifndef SSB64_NDS_CONTROLLER_H
#define SSB64_NDS_CONTROLLER_H

#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ssb_types.h>

typedef struct NDSControllerPlaybackPad
{
    u16 button;
    s8 stick_x;
    s8 stick_y;
    u8 errno_value;
} NDSControllerPlaybackPad;

extern volatile u32 gNdsControllerPollCount;
extern volatile u32 gNdsControllerPlaybackEnabled;
extern volatile u32 gNdsControllerPlaybackConnectedMask;
extern volatile u32 gNdsControllerPlaybackFrameCount;
extern volatile u32 gNdsControllerPlaybackReadCount;
extern volatile u32 gNdsControllerLiveReadCount;
extern volatile u32 gNdsControllerLiveConnectedMask;
extern volatile u32 gNdsControllerLivePad0Button;
extern volatile s32 gNdsControllerLivePad0StickX;
extern volatile s32 gNdsControllerLivePad0StickY;
extern volatile u32 gNdsControllerLivePad1Button;
extern volatile s32 gNdsControllerLivePad1StickX;
extern volatile s32 gNdsControllerLivePad1StickY;
extern volatile u32 gNdsControllerLiveMapCount;
extern volatile u32 gNdsControllerPlaybackPad0Button;
extern volatile u32 gNdsControllerPlaybackPad1Button;
extern volatile s32 gNdsControllerPlaybackPad0StickX;
extern volatile s32 gNdsControllerPlaybackPad1StickX;
extern volatile s32 gNdsControllerPlaybackPad0StickY;
extern volatile s32 gNdsControllerPlaybackPad1StickY;

/* Edge-publish interlock (src/import/battleship_sys_controller.c). Suppressed
 * counts every second `syControllerUpdateGlobalData` of one read -- each of
 * those used to overwrite a live `button_tap` with zero. EdgeSeenMask is the
 * sticky OR of the computed rising edge, so a nonzero mask with a zero
 * published tap localises the loss to the publish, not the sample. */
extern volatile u32 gNdsControllerPublishCount;
extern volatile u32 gNdsControllerPublishSuppressedCount;
extern volatile u32 gNdsControllerReadCount;
extern volatile u32 gNdsControllerEdgeSeenMask;
extern volatile u32 gNdsControllerPublishedTapMask;
extern volatile u32 gNdsControllerReadEdgeCount;
extern volatile u32 gNdsControllerPublishTapNonzeroCount;
void ndsControllerEdgeTelemetryReset(void);

int ndsControllerBackendSelfTest(void);
void ndsControllerPlaybackReset(void);
void ndsControllerPlaybackSetEnabled(sb32 is_enabled);
void ndsControllerPlaybackSetConnectedMask(u32 mask);
void ndsControllerPlaybackSetPad(u32 slot, u16 button, s8 stick_x,
                                 s8 stick_y);
void ndsControllerPlaybackCommitFrame(void);

#endif
