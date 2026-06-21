#ifndef SSB64_NDS_PLATFORM_H
#define SSB64_NDS_PLATFORM_H

#include <PR/ultratypes.h>

enum NDSInput {
    NDS_INPUT_LEFT  = 1u << 0,
    NDS_INPUT_RIGHT = 1u << 1,
    NDS_INPUT_UP    = 1u << 2,
    NDS_INPUT_DOWN  = 1u << 3,
    NDS_INPUT_A     = 1u << 4,
    NDS_INPUT_START = 1u << 5
};

void ndsPlatformInit(void);
u32 ndsPlatformReadInput(void);
void ndsPlatformBeginFrame(void);
void ndsPlatformDrawRect(s32 x, s32 y, s32 width, s32 height, u16 color);
u16 *ndsPlatformBeginOriginalSpritePreview(u32 width, u32 height,
                                           s32 n64_x, s32 n64_y,
                                           u32 *out_pitch);
void ndsPlatformCommitOriginalSpritePreview(void);
void ndsPlatformClearOriginalSpritePreview(void);
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
void ndsPlatformRenderDebugHud(void);
void ndsPlatformEndFrame(void);
u32 ndsPlatformTicks(void);
u32 ndsPlatformHeldKeys(void);

#endif
