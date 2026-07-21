#ifndef SSB64_NDS_IFCOMMON_OAM_H
#define SSB64_NDS_IFCOMMON_OAM_H

#include <stddef.h>

#include <PR/ultratypes.h>
#include <sys/vector.h>

#ifndef NDS_IFCOMMON_HYBRID_OAM
#define NDS_IFCOMMON_HYBRID_OAM 0
#endif

struct GObj;

void ndsIFCommonNativeOamInit(void);
s32 ndsIFCommonNativeOamPrepareGameStatus(void *file_data,
                                           size_t file_size);
s32 ndsIFCommonNativeOamPrepareClouds(void);
void ndsIFCommonNativeOamBeginFrame(void);
s32 ndsIFCommonNativeOamDrawGObj(struct GObj *gobj);
void ndsIFCommonNativeOamCommit(void);

#define NDS_TASK39_FX_ENGAGED_SPRITES (1u << 0)
#define NDS_TASK39_FX_ENGAGED_FLASH (1u << 1)
#define NDS_TASK39_FX_ENGAGED_SHIELD (1u << 2)

void ndsTask39HitSparkSpawn(const Vec3f *pos, s32 player, s32 size,
                            sb32 is_static, sb32 is_heavy);
void ndsTask39EffectsUpdate(void);
void ndsTask39EffectsAddDrawTicks(u32 ticks);
void ndsTask39EffectsEngage(u32 mask);

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
extern volatile u32 gNdsIFCommonNativeOamPrepareCloudTextureBytes;
extern volatile u32 gNdsIFCommonNativeOamPrepareCloudTextureCount;
extern volatile u32 gNdsIFCommonNativeOamPrepareCloudFailureStage;
extern volatile u32 gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[16];
extern volatile u32 gNdsIFCommonNativeOamHotConvertCount;
extern volatile u32 gNdsIFCommonNativeOamRuntimeUploadBytes;
extern volatile u32 gNdsIFCommonNativeOamFrameBeginTicks;
extern volatile u32 gNdsIFCommonNativeOamFrameTicks;
extern volatile u32 gNdsIFCommonNativeOamFrameCommitTicks;
extern volatile u32 gNdsIFCommonNativeOamFrameCommitCalls;
extern volatile u32 gNdsIFCommonNativeOamFrameClearedObjects;
extern volatile u32 gNdsIFCommonNativeOamFrameIdle;
extern volatile u32 gNdsIFCommonNativeOamIdleFrameCount;
extern volatile u32 gNdsIFCommonNativeOamFrameRecognizedCalls;
extern volatile u32 gNdsIFCommonNativeOamFrameDrawCalls;
extern volatile u32 gNdsIFCommonNativeOamFrameFallbackCalls;
extern volatile u32 gNdsIFCommonNativeOamFrameSObjCount;
extern volatile u32 gNdsIFCommonNativeOamFrameObjectCount;
extern volatile u32 gNdsIFCommonNativeOamFrameCloudDrawCount;
extern volatile u32 gNdsIFCommonNativeOamFrameSemanticHash;
extern volatile u32 gNdsIFCommonNativeOamLastFallbackReason;
extern volatile u32 gNdsIFCommonNativeOamCommitCount;
extern volatile u32 gNdsTask39FxSpawnTicks;
extern volatile u32 gNdsTask39FxUpdateTicks;
extern volatile u32 gNdsTask39FxDrawTicks;
extern volatile u32 gNdsTask39FxFrameTicks;
extern volatile u32 gNdsTask39FxMaxFrameTicks;
extern volatile u32 gNdsTask39FxEngagementMask;
extern volatile u32 gNdsTask39FxHitSparkSpawnCount;
extern volatile u32 gNdsTask39FxHitSparkUpdateCount;
extern volatile u32 gNdsTask39FxHitSparkDrawCount;
extern volatile u32 gNdsTask39FxHitSparkDropCount;
extern volatile u32 gNdsTask39FxFlashDrawCount;
extern volatile u32 gNdsTask39FxShieldDrawCount;
extern volatile u32 gNdsTask39FxArenaRejectCount;
extern volatile u32 gNdsTask39FxObjVramBytes;
extern volatile u32 gNdsTask39FxObjVramRemaining;

#endif
