#ifndef SSB64_NDS_EFFECTS_H
#define SSB64_NDS_EFFECTS_H

#include <PR/ultratypes.h>
#include <ssb_types.h>
#include <sys/objdef.h>

/* Bounded, untextured DS presentations for the Mario/Fox P1 effect seams. */
typedef enum NDSVisualEffectKind
{
    nNDSVisualEffectDust,
    nNDSVisualEffectHitNormal,
    nNDSVisualEffectHitFire,
    nNDSVisualEffectHitElectric,
    nNDSVisualEffectCoin,
    nNDSVisualEffectSlash,
    nNDSVisualEffectSparkle,
    nNDSVisualEffectImpactWave,
    nNDSVisualEffectShield,
    nNDSVisualEffectReflector,
    nNDSVisualEffectDeath,
    nNDSVisualEffectRebirth,
    nNDSVisualEffectCatch,
    nNDSVisualEffectKindCount
} NDSVisualEffectKind;

GObj *ndsEFManagerMakeVisualEffect(NDSVisualEffectKind kind,
                                    const Vec3f *pos, f32 scale, s32 lr,
                                    GObj *fighter_gobj);
s32 ndsEFManagerIsVisualEffectGObj(GObj *effect_gobj);
void ndsEFManagerStopAttachedVisualEffects(GObj *fighter_gobj);

extern volatile u32 gNdsVisualEffectCreateCount;
extern volatile u32 gNdsVisualEffectDestroyCount;
extern volatile u32 gNdsVisualEffectDropCount;
extern volatile u32 gNdsVisualEffectActiveCount;
extern volatile u32 gNdsVisualEffectMaxActiveCount;
extern volatile u32 gNdsVisualEffectKindMask;
extern volatile u32 gNdsVisualEffectTemplateBytes;
extern volatile u32 gNdsEffectRendererCaptureCount;
extern volatile u32 gNdsEffectRendererDObjDrawCount;
extern volatile u32 gNdsEffectRendererSubmitCount;
extern volatile u32 gNdsEffectRendererTriangleCount;
extern volatile u32 gNdsEffectRendererTextureReadyCount;
extern volatile u32 gNdsEffectRendererTextureRejectCount;
extern volatile u32 gNdsEffectRendererRejectedDrawCount;

#endif
