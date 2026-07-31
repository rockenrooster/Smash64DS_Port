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

/* Runtime engagement. Start/reject are exact: every external entry point is
 * validated against the packed set before the source constructor runs. */
extern volatile u32 gNdsParticleScriptStartCount;
extern volatile u32 gNdsParticleGeneratorStartCount;
extern volatile u32 gNdsParticleRejectCount;
extern volatile u32 gNdsParticleDrawSeamCount;    /* draw path still gated off */

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
