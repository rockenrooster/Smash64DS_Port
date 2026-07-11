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
void ndsPlatformCommitOriginalSpritePreview(void);
void ndsPlatformCommitOriginalSpritePreviewLayer(s32 is_foreground);
void ndsPlatformClearOriginalSpriteOverlayLayer(s32 is_foreground);
void ndsPlatformClearOriginalSpritePreview(void);
void ndsPlatformSetOriginalSpriteOverlayEnabled(s32 is_enabled);
u16 *ndsPlatformBeginOriginalDLPreview(u32 width, u32 height,
                                       u32 *out_pitch);
void ndsPlatformCommitOriginalDLPreview(void);
void ndsPlatformClearOriginalDLPreview(void);
extern volatile u32 gNdsOriginalSpritePreviewReady;
extern volatile u32 gNdsOriginalSpritePreviewCommitCount;
extern volatile u32 gNdsOriginalSpritePreviewDrawCount;
extern volatile u32 gNdsOriginalSpritePreviewDisplayWidth;
extern volatile u32 gNdsOriginalSpritePreviewDisplayHeight;
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
extern volatile u32 gNdsHardwareRendererSubmittedFrameCount;
extern volatile u32 gNdsHardwareRendererFlushCount;
extern volatile u32 gNdsHardwareRendererPolyRamCount;
extern volatile u32 gNdsHardwareRendererVertexRamCount;
extern volatile u32 gNdsHardwareRendererStatus;
extern volatile u32 gNdsHardwareRendererControl;
void ndsPlatformRenderDebugHud(void);
void ndsPlatformEndFrame(void);
u32 ndsPlatformTicks(void);
u32 ndsPlatformHeldKeys(void);

#endif
