#ifndef NDS_SCENE_HARNESS_H
#define NDS_SCENE_HARNESS_H

#include <PR/ultratypes.h>
#include <sc/scene.h>

#define NDS_DEV_SCENE_HARNESS_NORMAL 0u
#define NDS_DEV_SCENE_HARNESS_TITLE 1u
#define NDS_DEV_SCENE_HARNESS_VS_SETUP 2u
#define NDS_DEV_SCENE_HARNESS_BATTLE_FD 3u

#define NDS_SCENE_HARNESS_NONE 0u
#define NDS_SCENE_HARNESS_PASS 0x4841524Eu
#define NDS_SCENE_HARNESS_RESERVED_PASS 0x48525356u

extern volatile u32 gNdsSceneHarnessMode;
extern volatile u32 gNdsSceneHarnessResult;
extern volatile u32 gNdsSceneHarnessSceneCurr;
extern volatile u32 gNdsSceneHarnessScenePrev;
extern volatile u32 gNdsSceneHarnessReservedMask;

void ndsDevSceneHarnessApply(void);

#endif
