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
extern volatile u32 gNdsHardwareRendererSubmittedFrameCount;
extern volatile u32 gNdsHardwareRendererFlushCount;
extern volatile u32 gNdsHardwareRendererPolyRamCount;
extern volatile u32 gNdsHardwareRendererVertexRamCount;
extern volatile u32 gNdsHardwareRendererStatus;
extern volatile u32 gNdsHardwareRendererControl;
void ndsPlatformRenderDebugHud(void);
void ndsPlatformEndFrame(void);
void ndsPlatformProfileSampleFrameBoundaryGXState(void);
u32 ndsPlatformTicks(void);
u32 ndsPlatformHeldKeys(void);

#endif
