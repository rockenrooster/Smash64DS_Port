#ifndef SSB64_NDS_EF_EFFECT_H
#define SSB64_NDS_EF_EFFECT_H

#include <PR/ultratypes.h>
#include <ssb_types.h>
#include <gr/ground.h>

typedef struct LBParticle LBParticle;
typedef struct LBGenerator LBGenerator;

#define LBPARTICLE_MASK_GENLINK(link) ((link) << 16)

enum {
    nLBTransformStatusReady = 0
};

enum {
    nEFKindDustExpandLarge = 17,
    nEFKindImpactWave = 22,
    nEFKindQuakeMag0 = 32
};

s32 efParticleGetLoadBankID(void *script_lo, void *script_hi,
                            void *texture_lo, void *texture_hi);
LBParticle *lbParticleMakeScriptID(s32 bank_id, s32 script_id);
LBTransform *lbParticleAddTransformForStruct(LBParticle *pc, u8 status);
void LBParticleProcessStruct(LBParticle *pc);
void lbParticleEjectStruct(LBParticle *pc);
void lbParticleEjectStructID(s32 generator_id, s32 index);
LBParticle *efManagerFlashMiddleMakeEffect(Vec3f *pos);
LBParticle *efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale);
void efManagerQuakeMakeEffect(s32 id);
void efGroundMakeAppearActor(void);

#endif
