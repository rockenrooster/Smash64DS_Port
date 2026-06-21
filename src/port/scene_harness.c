#include <nds/nds_scene_harness.h>

#ifndef NDS_DEV_SCENE_HARNESS
#define NDS_DEV_SCENE_HARNESS NDS_DEV_SCENE_HARNESS_NORMAL
#endif

volatile u32 gNdsSceneHarnessMode;
volatile u32 gNdsSceneHarnessResult;
volatile u32 gNdsSceneHarnessSceneCurr;
volatile u32 gNdsSceneHarnessScenePrev;
volatile u32 gNdsSceneHarnessReservedMask;

static void ndsSceneHarnessSetDefaultScene(SCKind scene_curr, SCKind scene_prev)
{
    dSCManagerDefaultSceneData.scene_curr = (u8)scene_curr;
    dSCManagerDefaultSceneData.scene_prev = (u8)scene_prev;

    gNdsSceneHarnessSceneCurr = (u32)scene_curr;
    gNdsSceneHarnessScenePrev = (u32)scene_prev;
}

void ndsDevSceneHarnessApply(void)
{
    gNdsSceneHarnessMode = (u32)NDS_DEV_SCENE_HARNESS;
    gNdsSceneHarnessResult = NDS_SCENE_HARNESS_NONE;
    gNdsSceneHarnessReservedMask = 0;
    gNdsSceneHarnessSceneCurr = dSCManagerDefaultSceneData.scene_curr;
    gNdsSceneHarnessScenePrev = dSCManagerDefaultSceneData.scene_prev;

#if defined(SSB64_TARGET_NDS)
    switch (NDS_DEV_SCENE_HARNESS)
    {
    case NDS_DEV_SCENE_HARNESS_NORMAL:
        return;

    case NDS_DEV_SCENE_HARNESS_TITLE:
        ndsSceneHarnessSetDefaultScene(nSCKindTitle, nSCKindOpeningNewcomers);
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_VS_SETUP:
        ndsSceneHarnessSetDefaultScene(nSCKindVSMode, nSCKindTitle);
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_PASS;
        return;

    case NDS_DEV_SCENE_HARNESS_BATTLE_FD:
        gNdsSceneHarnessReservedMask = 1u;
        ndsSceneHarnessSetDefaultScene(nSCKindTitle, nSCKindOpeningNewcomers);
        gNdsSceneHarnessResult = NDS_SCENE_HARNESS_RESERVED_PASS;
        return;

    default:
        gNdsSceneHarnessReservedMask = 0x80000000u;
        return;
    }
#endif
}
