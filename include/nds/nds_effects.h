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
    /* Shield, Reflector and Rebirth are GONE (2026-08-04). Those three kinds
     * were the procedural stand-ins for dEFManagerShieldEffectDesc,
     * dEFManagerFoxReflectorEffectDesc and dEFManagerRebirthHaloEffectDesc,
     * which all draw as source models now. The kinds that remain have no source
     * EFDesc route and are still the DS presentation for their effect ids. */
    nNDSVisualEffectDeath,
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
/* Named for what it does: it stops EVERY effect attached to the fighter, source
 * models included, exactly as source's ftParamProcStopEffect does. The old name
 * said "VisualEffects" and the body matched the name rather than the contract,
 * which is how source effects went a whole campaign with no owner ejecting
 * them. */
void ndsEFManagerStopAttachedEffects(GObj *fighter_gobj);
/* Source-kind effects ejected by that walk. The engagement control for the fix:
 * it must be non-zero on a flag-1 run in which anyone guards, and it stays 0 on
 * a flag-0 run because no source effect is attached there. */
extern volatile u32 gNdsEFManagerSourceEffectStopCount;

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
/* Descs whose file_head ndsEFManagerFileSpan does not recognise, so their
 * offsets were never bounds-checked at all. This branch used to return in
 * silence, and it swallowed the shield and Fox's reflector for three cycles --
 * "unvalidated" read exactly like "validated and fine". Must be 0 for the P1
 * desc set now that the span table covers gFTManagerCommonFile and
 * gFTDataFoxSpecial2 as well as the three EF-common files. */
extern volatile u32 gNdsEFDescUnknownFileCount;
/* WHICH desc, not just how many. The first run with the two counters above read
 * disabled=1 unknownfile=1 and could not say whether that was one desc failing
 * both tests or two different ones -- and the answer decides the next move
 * entirely (an out-of-span offset is a data bug; a NULL file slot is a missing
 * asset). These hold the EFDesc address, which gdb symbolises, so one printf
 * names it. Read them only when the counts above are non-zero. */
extern volatile u32 gNdsEFDescDisabledLast;
extern volatile u32 gNdsEFDescUnknownFileLast;
/* Descs deferred because their file was not resident at effect-init time, and
 * recovered once it loaded. recover>0 with disabled back to 0 is the proof the
 * reflector's file simply arrived late rather than being unbacked. */
extern volatile u32 gNdsEFDescDeferRecoverCount;
extern volatile u32 gNdsEFDescDeferOverflowCount;
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
/* Link-15 source effects (shield, Fox reflector) that actually DREW -- the
 * arming signal a shield capture needs. The wave and rebirth halo are link 10,
 * so this separates them; see reloc_backend_movement.c for why neither
 * existing counter could do it. */
extern volatile u32 gNdsEffectRendererLink15DrawCount;
/* Effect GObjs admitted to the hardware path because they carry a SOURCE model
 * rather than one of the procedural templates -- links 10 and 15 as well as 18.
 * NON-ZERO at the tracked default since 2026-08-04; zero means the source
 * models stopped being admitted and is a regression, not a configuration. */
extern volatile u32 gNdsEffectRendererSourceModelAdmitCount;

/* THE EXECUTOR'S OWN VERDICT ON AN EFFECT DISPLAY LIST, because "tris=0" cannot
 * distinguish "the list ran to its end and drew nothing" from "the walk stopped
 * at command k". The impact wave is the case that needs it: it is proven to
 * reach ndsRendererAdapterSubmitStageDL with a dl the loaded-file test accepts,
 * and its geometry sits at command indices 19-23 of the list at
 * EFCommonEffects1+0x7C28, INLINE AFTER the G_DL to segment 0x0E at index 14 --
 * so an unresolved segment E (which returns an empty list rather than aborting)
 * cannot be the explanation and a command count settles it in one read.
 * Published only while an effect tree submit is on the stack, so stage traffic
 * does not overwrite them. Last-value, not accumulating: one wave per read. */
extern volatile u32 gNdsEffectDLBlocker;
extern volatile u32 gNdsEffectDLCommandCount;
extern volatile u32 gNdsEffectDLFirstOpcode;
extern volatile u32 gNdsEffectDLUnsupportedOpcode;
extern volatile u32 gNdsEffectDLVertexCount;
extern volatile u32 gNdsEffectDLTriangleCount;
extern volatile u32 gNdsEffectDLPublishCount;

/* WHICH MATRIX THE EFFECT DISPLAY LIST ACTUALLY EXECUTED WITH, published by the
 * submitter from its OWN locals. The config is a stack object and reading it
 * through the argument register at ndsRendererExecuteDisplayListWithVertexCache
 * has now produced three mutually contradictory answers on three ROMs -- cycle
 * 53 read 8/8192/512 with both matrix pointers NULL, cycles 54 and 55 read five
 * zero words on a pointer whose config->user matched callback_user exactly. A
 * stack read is not evidence on this remote; a global written by the code that
 * owns the value is. CfgMask is bit 0 initial_projection, bit 1
 * initial_modelview. MatrixSeed is the DELTA of hardware_matrix_seed_count
 * across the call, which ndsRendererInitTraversalState increments only when the
 * config's matrices composed into a valid traversal matrix -- so
 * CfgMask 3 with MatrixSeed 1 closes the gap between the prep verdict
 * (gNdsRendererAdapterEffectPrepMask) and the executor with no register read at
 * all. MatrixCmd is the delta of the list's own matrix commands: non-zero means
 * the display list overrode what the config seeded. */
extern volatile u32 gNdsEffectDLCfgMask;
extern volatile s32 gNdsEffectDLCfgMvT[3];
extern volatile u32 gNdsEffectDLMatrixSeed;
extern volatile u32 gNdsEffectDLMatrixCmd;
extern volatile u32 gNdsEffectDLXformVertexCount;
extern volatile u32 gNdsEffectDLHwVertexCount;
extern volatile u32 gNdsEffectDLHwTriangleCount;
extern volatile s32 gNdsEffectDLVtx0[4];
/* The prim/env this effect's own proc_display emitted, recovered from the DL
 * head span it wrote. Mask bit 0 prim, bit 1 env; zero means the proc emitted
 * neither and the effect keeps the previous list's RDP colour. */
extern volatile u32 gNdsEffectDLColorMask;
extern volatile u32 gNdsEffectDLPrimColor;
extern volatile u32 gNdsEffectDLEnvColor;

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
 * DLHEAD0 drawable rule (flags == DOBJ_FLAG_NONE) rejects it. Measured 0x0. */
extern volatile u32 gNdsEffectRendererDObjFlagsMask;
/* Which DObj field carries the geometry: bit0 dl, bit1 dl_link, bit2 dv.
 * DLHEAD0 submits `dl`; only the *_DLLINKS kinds read `dl_link`. */
extern volatile u32 gNdsEffectRendererDObjFieldMask;

#endif
