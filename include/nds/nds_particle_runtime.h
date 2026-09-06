/* R2-07 particle runtime engagement counters.
 *
 * The interpreter itself is the unmodified BattleShip lb/lbparticle.c compiled
 * in place by src/import/battleship_lbparticle.c. These counters are the only
 * port-side observation surface: they say whether the original scripts were
 * loaded, started and stepped, and how many script ids were rejected instead of
 * being mis-indexed into a different effect. They compile to nothing when the
 * runtime is off.
 */
#ifndef SSB64_NDS_PARTICLE_RUNTIME_H
#define SSB64_NDS_PARTICLE_RUNTIME_H

#include <PR/ultratypes.h>

/* Load-time bank census, written once by the DS bank loader. */
extern volatile u32 gNdsParticleBankLoadResult;   /* 0 unrun, 1 pass, 2 reject */
extern volatile u32 gNdsParticleBankBytes;        /* bank bytes normalized     */
extern volatile u32 gNdsParticleBankScriptsPacked;
extern volatile u32 gNdsParticleBankScriptsUnpacked;
extern volatile u32 gNdsParticleBankScriptsRejected; /* malformed bytecode     */
extern volatile u32 gNdsParticleBankCommands;     /* commands in packed bank   */
extern volatile u32 gNdsParticleBankFloatOperands; /* BE f32 operands swapped  */
extern volatile u32 gNdsParticleBankTextures;
extern volatile u32 gNdsParticleBankEFCommonID;   /* bank id given to efcommon */
extern volatile u32 gNdsParticleBankOtherID;      /* bank id given to any other*/
extern volatile u32 gNdsParticleBankPupupuID;     /* Dream Land's, 0xff if none */
extern volatile u32 gNdsParticlePupupuScriptsPacked;
extern volatile u32 gNdsParticleBankHyruleID;
extern volatile u32 gNdsHyruleScriptsPacked;
extern volatile u32 gNdsHyruleNativeTexturePrepareCount;
extern volatile u32 gNdsHyruleNativeTextureFailCount;
extern volatile u32 gNdsHyruleNativeDrawCount;
extern volatile u32 gNdsHyruleNativeMissCount;
extern volatile u32 gNdsHyruleNativeFrameMask;
/* Hyrule textures are prepared with the particle atlas before the countdown.
 * Name lookup never allocates or reads NitroFS. Exit/reset/failure share the
 * idempotent release; native frame 0 is not substituted for later frames. */
s32 ndsRendererHardwarePrepareHyruleTextures(void);
void ndsRendererHardwareReleaseHyruleTextures(void);
u32 ndsRendererHardwareHyruleTextureName(u32 texture, u32 frame);
/* efParticleInitAll restarts bank numbering, so these two together say whether
 * two banks were handed the same slot. */
extern volatile u32 gNdsParticleInitAllCount;
extern volatile u32 gNdsParticleBankRegisterCount;

/* Runtime engagement. Start/reject are exact: every external entry point is
 * validated against the packed set before the source constructor runs. */
extern volatile u32 gNdsParticleScriptStartCount;
extern volatile u32 gNdsParticleGeneratorStartCount;
extern volatile u32 gNdsParticleRejectCount;
extern volatile u32 gNdsParticleDrawSeamCount;    /* draw path still gated off */

/* WHICH textures a P1 match actually draws, as opposed to which ones the
 * generator's static reachability admits.
 *
 * This is a VRAM budget question with a hard edge. The battle's static texture
 * set already pins 136,192 bytes at the base of VRAM_A, and VRAM_A+B together
 * are 262,144, so a particle set has 119,872 bytes to live in. The generator's
 * reachable set is 31 textures and 137,152 bytes -- 17,280 over, before the
 * draw path exists to be measured. Guessing which to drop would be guessing;
 * the reachable set is a static over-approximation over 87 scripts and the
 * first live run started ten of them.
 *
 * So the draw seam counts. One bit per SOURCE texture id (efcommon has 47) plus
 * a per-id frame high-water, because a ten-frame animation costs ten times a
 * still and the frame count is the first thing worth trimming. Read after a
 * full match; the answer is the upload set. */
#define NDS_PARTICLE_TEXTURE_USE_IDS 47u
extern volatile u32 gNdsParticleTextureUseMask[2];
extern volatile u8 gNdsParticleTextureFrameMax[NDS_PARTICLE_TEXTURE_USE_IDS];
extern volatile u32 gNdsParticleDrawVisibleCount;  /* particles past the clip  */
extern volatile u32 gNdsParticleDrawVisibleMax;    /* worst single frame       */

/* The draw itself. Emit vs visible is the fail-closed margin: a particle whose
 * texture is not in the atlas draws NOTHING rather than a neighbouring cell,
 * and MissCount says how often, so "no effects appeared" and "effects appeared
 * wrong" can never be confused for one another. */
extern volatile u32 gNdsParticleQuadEmitCount;
extern volatile u32 gNdsParticleQuadEmitMax;
extern volatile u32 gNdsParticleQuadMissCount;
/* Source MASKS/MASKT engagement. ST is also included in the S and T totals;
 * these count submitted SOURCE particles, not the 2/4 atlas quads emitted for
 * reconstruction. A ledge-grab FlashMiddle run must engage ST. */
extern volatile u32 gNdsParticleMirrorSSubmitCount;
extern volatile u32 gNdsParticleMirrorTSubmitCount;
extern volatile u32 gNdsParticleMirrorSTSubmitCount;
/* Which SOURCE texture ids missed (pre-stride), which frame indices missed, and
 * how many draws took the Dream Land bank stride. A bare MissCount cannot tell
 * an unadmitted texture from a frame past the packed animation from a stride
 * applied to the wrong bank. */
extern volatile u32 gNdsParticleQuadMissMask[2];
extern volatile u32 gNdsParticleQuadMissFrameMask;
extern volatile u32 gNdsParticleQuadStrideCount;

/* Mirrors of the interpreter's own live/highwater tallies, published so a run
 * can read them without a symbol lookup into the imported translation unit. */
extern volatile u32 gNdsParticleStructsLive;
extern volatile u32 gNdsParticleStructsMax;
extern volatile u32 gNdsParticleGeneratorsLive;
extern volatile u32 gNdsParticleGeneratorsMax;
extern volatile u32 gNdsParticleTransformsLive;
extern volatile u32 gNdsParticleTransformsMax;
extern volatile u32 gNdsParticleRootSpawnCount;   /* monotonic generator ids   */

/* Copies the interpreter tallies into the mirrors above. Cheap; called from the
 * particle draw seam once per presented frame. */
void ndsParticleRuntimePublishTallies(void);

#endif
