#ifndef SSB64_NDS_IFCOMMON_OAM_H
#define SSB64_NDS_IFCOMMON_OAM_H

#include <stddef.h>

#include <PR/ultratypes.h>

struct GObj;

void ndsIFCommonNativeOamInit(void);
s32 ndsIFCommonNativeOamPrepareGameStatus(void *file_data,
                                           size_t file_size);
void ndsIFCommonNativeOamBeginFrame(void);
s32 ndsIFCommonNativeOamDrawGObj(struct GObj *gobj);
void ndsIFCommonNativeOamCommit(void);

extern volatile u32 gNdsIFCommonNativeOamEnabled;
extern volatile u32 gNdsIFCommonNativeOamPrepareCount;
extern volatile u32 gNdsIFCommonNativeOamPrepareSuccessCount;
extern volatile u32 gNdsIFCommonNativeOamPrepareFailCount;
extern volatile u32 gNdsIFCommonNativeOamPrepareTicks;
extern volatile u32 gNdsIFCommonNativeOamPrepareBytes;
extern volatile u32 gNdsIFCommonNativeOamPrepareAssets;
extern volatile u32 gNdsIFCommonNativeOamPrepareTiles;
extern volatile u32 gNdsIFCommonNativeOamPrepareProfileFrame;
extern volatile u32 gNdsIFCommonNativeOamPreparePaletteBytes;
extern volatile u32 gNdsIFCommonNativeOamHotConvertCount;
extern volatile u32 gNdsIFCommonNativeOamRuntimeUploadBytes;
extern volatile u32 gNdsIFCommonNativeOamFrameTicks;
extern volatile u32 gNdsIFCommonNativeOamFrameRecognizedCalls;
extern volatile u32 gNdsIFCommonNativeOamFrameDrawCalls;
extern volatile u32 gNdsIFCommonNativeOamFrameFallbackCalls;
extern volatile u32 gNdsIFCommonNativeOamFrameSObjCount;
extern volatile u32 gNdsIFCommonNativeOamFrameObjectCount;
extern volatile u32 gNdsIFCommonNativeOamFrameSemanticHash;
extern volatile u32 gNdsIFCommonNativeOamLastFallbackReason;
extern volatile u32 gNdsIFCommonNativeOamCommitCount;

#endif
