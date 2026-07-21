#ifndef SSB64_NDS_EF_EFFECT_H
#define SSB64_NDS_EF_EFFECT_H

#include <PR/ultratypes.h>
#include <ssb_types.h>
#include <gr/ground.h>

typedef struct LBParticle LBParticle;
typedef struct LBGenerator LBGenerator;

#include <ef/efdef.h>
#include <ef/efvars.h>

struct EFDesc
{
    u8 flags;
    u8 dl_link;
    void **file_head;
    DObjTransformTypes transform_types1;
    DObjTransformTypes transform_types2;
    void (*proc_update)(GObj*);
    void (*proc_display)(GObj*);
    intptr_t o_dobjsetup;
    intptr_t o_mobjsub;
    intptr_t o_anim_joint;
    intptr_t o_matanim_joint;
};

typedef struct EFGroundParam
{
    u16 effect_id;
    u16 make_queue;
    s32 lr;
    u8 effect_weight;
} EFGroundParam;

typedef struct EFGroundDesc
{
    f32 alt_high;
    f32 alt_low;
    f32 pos_z;
    f32 scale;
    u16 effect_status;
    void (*proc_groundeffect)(GObj*);
    EFDesc effect_desc;
} EFGroundDesc;

typedef struct EFGroundData
{
    u8 params_num;
    EFGroundParam *effect_params;
    intptr_t o_data;
    EFGroundDesc *effect_descs;
} EFGroundData;

typedef struct EFGroundActor
{
    s32 make_wait;
    u16 effect_id;
    u16 make_queue;
    u8 effect_count;
    u8 *effect_ids;
    s32 lr;
    void *file_head;
    EFGroundData *effect_data;
} EFGroundActor;

struct EFStruct
{
    EFStruct *next;
    GObj *fighter_gobj;
    u16 bank_id;
    LBTransform *xf;
    ub32 is_pause_effect : 1;
    void (*proc_update)(GObj*);

    union efEffectVars
    {
        EFCommonEffectVarsCommon common;
        EFCommonEffectVarsContainer container;
        EFCommonEffectVarsDamageNormalHeavy damage_normal_heavy;
        EFCommonEffectVarsDustLight dust_light;
        EFCommonEffectVarsDustHeavy dust_heavy;
        EFCommonEffectVarsDamageFlyOrbs damage_fly_orbs;
        EFCommonEffectVarsDamageSpawnOrbs damage_spawn_orbs;
        EFCommonEffectVarsImpactWave impact_wave;
        EFCommonEffectVarsStarRodSpark star_rod_spark;
        EFCommonEffectVarsDamageFlySpark damage_fly_sparks;
        EFCommonEffectVarsDamageSpawnSpark damage_spawn_sparks;
        EFCommonEffectVarsDamageFlyMDust damage_fly_mdust;
        EFCommonEffectVarsDamageSpawnMDust damage_spawn_mdust;
        EFCommonEffectVarsQuake quake;
        EFCommonEffectVarsReflector reflector;
        EFCommonEffectVarsShield shield;
        EFCommonEffectVarsUnknown1 unknown1;
        EFCommonEffectVarsThunderTrail thunder_trail;
        EFCommonEffectVarsVulcanJab vulcan_jab;
        EFCommonEffectVarsPKThunder pkthunder;
        EFCommonEffectVarsYoshiEggLay yoshi_egg_lay;
        EFCommonEffectVarsCaptureKirbyStar capture_kirby_star;
        EFCommonEffectVarsLoseKirbyStar lose_kirby_star;
        EFGroundEffectVarsCommon ground_effect;
    } effect_vars;
};

#include <ef/efdisplay.h>
#include <ef/efmanager.h>
#include <ef/efground.h>

#define LBPARTICLE_MASK_GENLINK(link) ((link) << 16)

#define efGetStruct(effect_gobj) ((EFStruct *)(effect_gobj)->user_data.p)

extern void *gEFManagerFiles[3];
extern s32 gEFManagerParticleBankID;

LBParticle *lbParticleMakeScriptID(s32 bank_id, s32 script_id);
LBParticle *lbParticleMakeCommon(s32 bank_id, s32 script_id);
LBParticle *lbParticleMakePosVel(s32 bank_id, s32 script_id, f32 pos_x,
                                 f32 pos_y, f32 pos_z, f32 vel_x,
                                 f32 vel_y, f32 vel_z);
LBGenerator *lbParticleMakeGenerator(s32 bank_id, s32 generator_id);
LBTransform *lbParticleAddTransformForStruct(LBParticle *pc, u8 status);
void LBParticleProcessStruct(LBParticle *pc);
void lbParticleEjectStruct(LBParticle *pc);
void lbParticleEjectStructID(u16 generator_id, s32 link_id);
void lbParticleDrawTextures(GObj *gobj);

LBParticle *efManagerDamageNormalHeavyMakeEffect(Vec3f *pos, s32 player,
                                                 s32 size);
GObj *efManagerShieldMakeEffect(GObj *fighter_gobj);

#endif
