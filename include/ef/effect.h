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
LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
LBParticle *efManagerFireGrindMakeEffect(Vec3f *pos);
LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos);
LBParticle *efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale);
LBParticle *efManagerSetOffMakeEffect(Vec3f *pos, s32 size);
LBParticle *efManagerDamageNormalLightMakeEffect(Vec3f *pos, s32 player,
                                                 s32 size, sb32 is_static);
LBParticle *efManagerDamageNormalHeavyMakeEffect(Vec3f *pos, s32 player,
                                                 s32 size);
LBParticle *efManagerDamageFireMakeEffect(Vec3f *pos, s32 size);
LBParticle *efManagerDamageElectricMakeEffect(Vec3f *pos, s32 size);
LBParticle *efManagerDamageCoinMakeEffect(Vec3f *pos);
GObj *efManagerDamageSlashMakeEffect(Vec3f *pos, s32 size, f32 rotate);
GObj *efManagerDamageSpawnOrbsRandomMakeEffect(Vec3f *pos);
GObj *efManagerDamageSpawnSparksRandomMakeEffect(Vec3f *pos, s32 lr);
GObj *efManagerDamageSpawnMDustRandomMakeEffect(Vec3f *pos, s32 lr);
void efManagerQuakeMakeEffect(s32 id);
void efGroundMakeAppearActor(void);

#endif
