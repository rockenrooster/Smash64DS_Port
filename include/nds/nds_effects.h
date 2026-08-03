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

/* One camera-facing textured quad from the particle sheet, for a GObj effect
 * that is not a particle. Defined in src/import/battleship_lbparticle.c beside
 * the pass whose atlas and camera it borrows; texture_id is one of the
 * NDS_PARTICLE_QUAD_*_TEXTURE keys the bank generator emits. Fails closed. */
sb32 ndsParticleDrawSourceAssetQuad(u32 texture_id, const Vec3f *pos, f32 size,
                                    u32 color, u8 alpha);
extern volatile u32 gNdsSourceAssetQuadAttempts;
extern volatile u32 gNdsSourceAssetQuadDrawn;
extern volatile u32 gNdsSourceAssetQuadMissMask;

GObj *ndsEFManagerMakeVisualEffect(NDSVisualEffectKind kind,
                                    const Vec3f *pos, f32 scale, s32 lr,
                                    GObj *fighter_gobj);
s32 ndsEFManagerIsVisualEffectGObj(GObj *effect_gobj);
void ndsEFManagerStopAttachedVisualEffects(GObj *fighter_gobj);

/* DS effect-instance pool depth, replacing the source's EFFECT_ALLOC_NUM 38.
 *
 * Every source effect that allocates takes an EFStruct from
 * efManagerGetEffectNoForce FIRST and a GObj from gcMakeGObjSPAfter second;
 * particles and transforms come from lbParticle's own fixed pools
 * (sLBParticleTransformsAllocFree, sLBParticleStructsAllocLinks), so the GObj
 * is the only unbounded allocation in an effect. gcGetDObjSetNextAlloc
 * (objman.c:692) grows the DObj pool out of gSYTaskmanGeneralHeap 136 bytes at
 * a time with no ceiling and never shrinks it, so peak concurrency -- not
 * lifetime churn -- is what drives the heap under the 25,600-byte
 * ifCommonSetMaxNumGObj latch. Bounding the EFStruct free list therefore
 * bounds the DObj peak, with the source's real scripts, textures, transforms
 * and DObj trees intact: no substitute templates, no second renderer.
 *
 * The source's own reserve is preserved and is the reason this works:
 * efManagerGetNextStructAlloc refuses a non-forced request below 5 free, so a
 * depth of N yields (N - 4) concurrent cosmetic effects and keeps 4 slots for
 * the forced makers -- the KO burst takes efManagerGetEffectForce precisely so
 * that it always spawns. Refusal is a path the source already takes when it is
 * busy, and every caller tolerates the NULL return, so an over-subscribed
 * frame drops the newest cosmetic effect exactly as the N64 does rather than
 * growing the heap. */
#ifndef NDS_R2_EFFECT_POOL
#define NDS_R2_EFFECT_POOL 12
#endif
/* Depth actually installed, and the battle-time low-water of
 * sEFManagerStructsFreeNum. A low-water pinned at 4 means the pool is
 * saturating and cosmetic effects are being refused; 0 means the forced
 * reserve is being consumed too. */
extern volatile u32 gNdsEffectPoolDepth;
extern volatile u32 gNdsEffectPoolFreeMin;

/* KO burst (efManagerDeadExplodeMakeEffect). The source walks its DObj tree
 * with no NULL guard; the port implementation in
 * src/import/battleship_efmanager.c checks every link and records the missing
 * one here instead of faulting. Attempt == Complete means the tree is whole. */
/* EFDesc file-offset fields translated from symbol-address to offset. Non-zero
 * proves the resolver ran; see the block comment in
 * src/import/battleship_efmanager.c for why an unresolved field is a heap
 * exhaustion hang rather than a wrong picture. */
extern volatile u32 gNdsEFDescResolveCount;
/* Descs neutralised because their file cannot back their offsets, and the
 * measured byte span of EFCommonEffects1/2/3. A span equal to sizeof(Sprite)
 * means that asset is absent and lbRelocGetFileSize fell back to its stub. */
extern volatile u32 gNdsEFDescDisabledCount;
extern volatile u32 gNdsEFDescEffectsSpan[3];
/* Last checkpoint the KO burst reached. Latched, not cleared, so a frozen
 * capture names the step that faulted even with no usable backtrace -- the
 * burst runs with the game thread already tearing down a fighter, and the
 * 2026-08-01 captures came back as a bare armWaitForIrq idle loop with every
 * register zeroed. */
extern volatile u32 gNdsKOBurstStage;
#define NDS_KO_BURST_STAGE_ENTER      1u
#define NDS_KO_BURST_STAGE_PARTICLE   2u
#define NDS_KO_BURST_STAGE_MATANIM    3u
#define NDS_KO_BURST_STAGE_MAKEFORCE  4u /* inside efManagerMakeEffectForce */
#define NDS_KO_BURST_STAGE_TREE       5u /* force returned; walking the tree */
#define NDS_KO_BURST_STAGE_DONE       6u
extern volatile u32 gNdsKOBurstAttemptCount;
extern volatile u32 gNdsKOBurstCompleteCount;
extern volatile u32 gNdsKOBurstDropMask;
#define NDS_KO_BURST_DROP_PARTICLE     (1u << 0) /* lbParticleMakeScriptID */
#define NDS_KO_BURST_DROP_XF           (1u << 1) /* no transform for particle */
#define NDS_KO_BURST_DROP_XF_UNUSED    (1u << 2) /* xf->users_num == 0 */
#define NDS_KO_BURST_DROP_GOBJ         (1u << 3) /* efManagerMakeEffectForce */
#define NDS_KO_BURST_DROP_ROOT_DOBJ    (1u << 4) /* DObjGetStruct == NULL */
#define NDS_KO_BURST_DROP_CHILD        (1u << 5) /* dobj->child == NULL */
#define NDS_KO_BURST_DROP_SIBLING      (1u << 6) /* child->sib_next->sib_next */
#define NDS_KO_BURST_DROP_SIBLING_MOBJ (1u << 7)
#define NDS_KO_BURST_DROP_CHILD_MOBJ   (1u << 8)
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
/* Effect GObjs admitted to the hardware path because they carry a SOURCE model
 * rather than one of the procedural templates -- links 10 and 15 as well as 18.
 * Zero at the tracked default; non-zero is the engagement proof for
 * NDS_R2_SOURCE_EFFECTS_FULL. */
extern volatile u32 gNdsEffectRendererSourceModelAdmitCount;

/* The effect DObj tree walk (reloc_backend_renderer_dl.c). Declared and reset
 * here so they exist in EVERY build, not only the one that increments them:
 * -fdata-sections plus --gc-sections deletes a volatile counter with no reader,
 * and a probe that names an absent symbol loses its whole gdb run. NodeCount is
 * what separates "the walk emitted nothing" from "the walk never ran". */
extern volatile u32 gNdsRendererStageDObjNodeCount;
extern volatile u32 gNdsRendererStageDObjDepthOverrunCount;
extern volatile u32 gNdsRendererStageDObjSiblingOverrunCount;

/* Which NDS_OPENING_ROOM_DRAW_CALLBACK_* kinds reach the effect submit, and
 * which of them it refuses. Masks rather than last-values: several kinds arrive
 * per frame and the last one is not necessarily the refused one. */
extern volatile u32 gNdsEffectRendererCallbackKindMask;
extern volatile u32 gNdsEffectRendererRejectedKindMask;
/* OR of the effect root DObj->flags seen at the submit. Non-zero means the
 * DLHEAD0 drawable rule (flags == DOBJ_FLAG_NONE) rejects it. */
extern volatile u32 gNdsEffectRendererDObjFlagsMask;

#endif
