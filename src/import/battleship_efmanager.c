#include <ef/effect.h>
#include <ft/ftdata_file_slots.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <it/item.h>
#include <sc/scene.h>
#include <wp/weapon.h>
#include <reloc_data.h>
#include <reloc_data_ftdata_symbols.h>
#include <sys/audio.h>

#include "battleship_efmanager_symbols.h"

struct LBGenerator
{
    LBGenerator *next;
    u16 generator_id;
    u16 flags;
    u8 kind;
    u8 bank_id;
    u16 texture_id;
    u16 particle_lifetime;
    u16 generator_lifetime;
    u8 *bytecode;
    Vec3f pos;
    Vec3f vel;
    f32 gravity;
    f32 friction;
    f32 size;
    f32 unk_gn_0x38;
    f32 unk_gn_0x3C;
    f32 update_rate;
    f32 frame;
    DObj *dobj;
    LBTransform *xf;

    union
    {
        struct { f32 base, target; } rotate;
        Vec3f move;
        struct { f32 f; u16 lifetime; } vortex;
    } generator_vars;
};

struct LBParticle
{
    LBParticle *next;
    u16 generator_id;
    u16 flags;
    u8 bank_id;
    u8 loop_count;
    u8 texture_id;
    u8 frame_id;
    ub16 bytecode_timer;
    u16 size_target_length;
    u16 primcolor_target_length;
    u16 envcolor_target_length;
    u8 *bytecode;
    u16 bytecode_csr;
    u16 return_ptr;
    u16 loop_ptr;
    u16 lifetime;
    Vec3f pos;
    Vec3f vel;
    f32 gravity;
    f32 friction;
    f32 size;
    f32 size_target;
    SYColorRGBA primcolor;
    SYColorRGBA target_primcolor;
    SYColorRGBA envcolor;
    SYColorRGBA target_envcolor;
    LBGenerator *gn;
    LBTransform *xf;
};

void lbCommonDObjScaleXProcDisplay(GObj *gobj);
DObj *lbCommonGetTreeDObjNextFromRoot(DObj *a, DObj *b);
void lbCommonAddDObjAnimJointAll(DObj *root_dobj,
                                 AObjEvent32 **anim_joints,
                                 f32 anim_frame);
void lbCommonAddTreeDObjsAnimAll(DObj *root_dobj,
                                 AObjEvent32 **anim_joints,
                                 AObjEvent32 ***p_matanim_joints,
                                 f32 anim_frame);
void lbCommonSetupTreeDObjs(DObj *root_dobj, DObjDesc *dobjdesc,
                            DObj **dobjs, u8 tk1, u8 tk2, u8 tk3);
void lbCommonAddMObjForTreeDObjs(DObj *root_dobj, MObjSub ***p_mobjsubs);
void lbCommonSetDObjTransformsForTreeDObjs(DObj *root_dobj,
                                           DObjDesc *dobjdesc);
f32 lbCommonSin(f32 x);
f32 lbCommonCos(f32 x);
extern GObj *gGMCameraGObj;
extern void *gFTManagerCommonFile;
extern void *gITManagerCommonData;
typedef struct ftCommonYoshiEggDesc
{
    f32 effect_size;
    Vec3f offset;
    Vec3f size;
} ftCommonYoshiEggDesc;
extern ftCommonYoshiEggDesc dFTCommonYoshiEggDamageCollDescs[];
void ftParamProcPauseEffect(GObj *effect_gobj);
void ftParamProcResumeEffect(GObj *fighter_gobj);
void gmCameraSetVelAt(Vec3f *move);

#ifndef nFTCaptainStatusSpecialAirLw
#define nFTCaptainStatusSpecialAirLw (nFTCommonStatusSpecialStart + 13)
#endif

#ifndef nWPNessPKThunderStatusActive
#define nWPNessPKThunderStatusActive 0
#define nWPNessPKThunderStatusDestroy 1
#define nWPNessPKThunderStatusCollide 2
#endif

uintptr_t lEFCommonParticleScriptBankLo;
uintptr_t lEFCommonParticleScriptBankHi;
uintptr_t lEFCommonParticleTextureBankLo;
uintptr_t lEFCommonParticleTextureBankHi;

#define efParticleGetLoadBankID(script_lo, script_hi, texture_lo, texture_hi) \
    efParticleGetLoadBankID((uintptr_t)(script_lo), (uintptr_t)(script_hi), \
                            (uintptr_t)(texture_lo), (uintptr_t)(texture_hi))

#include "../../decomp/BattleShip-main/decomp/src/ef/efdisplay.c"

#undef efParticleGetLoadBankID

#define efManagerDamageNormalLightMakeEffect ndsBaseEFManagerDamageNormalLightMakeEffect
#define efManagerDamageNormalHeavyMakeEffect ndsBaseEFManagerDamageNormalHeavyMakeEffect
#define efManagerDamageFireMakeEffect ndsBaseEFManagerDamageFireMakeEffect
#define efManagerDamageElectricMakeEffect ndsBaseEFManagerDamageElectricMakeEffect
#define efManagerDamageCoinMakeEffect ndsBaseEFManagerDamageCoinMakeEffect
#define efManagerDamageSlashMakeEffect ndsBaseEFManagerDamageSlashMakeEffect
#define efManagerDustExpandSmallMakeEffect ndsBaseEFManagerDustExpandSmallMakeEffect
#define efManagerFireGrindMakeEffect ndsBaseEFManagerFireGrindMakeEffect
#define efManagerSparkleWhiteMakeEffect ndsBaseEFManagerSparkleWhiteMakeEffect
#define efManagerSparkleWhiteScaleMakeEffect ndsBaseEFManagerSparkleWhiteScaleMakeEffect
#define efManagerDamageSpawnOrbsRandomMakeEffect ndsBaseEFManagerDamageSpawnOrbsRandomMakeEffect
#define efManagerDamageSpawnSparksRandomMakeEffect ndsBaseEFManagerDamageSpawnSparksRandomMakeEffect
#define efManagerDamageSpawnMDustRandomMakeEffect ndsBaseEFManagerDamageSpawnMDustRandomMakeEffect
#define efManagerImpactWaveMakeEffect ndsBaseEFManagerImpactWaveMakeEffect
#define efManagerQuakeMakeEffect ndsBaseEFManagerQuakeMakeEffect
#define efManagerSetOffMakeEffect ndsBaseEFManagerSetOffMakeEffect
#define efManagerShieldMakeEffect ndsBaseEFManagerShieldMakeEffect
#define efManagerYoshiShieldMakeEffect ndsBaseEFManagerYoshiShieldMakeEffect
#define efManagerCatchSwirlMakeEffect ndsBaseEFManagerCatchSwirlMakeEffect
#define efManagerFlashMiddleMakeEffect ndsBaseEFManagerFlashMiddleMakeEffect
#define efManagerKirbyVulcanJabMakeEffect ndsBaseEFManagerKirbyVulcanJabMakeEffect
#define efManagerSamusGrappleBeamGlowMakeEffect ndsBaseEFManagerSamusGrappleBeamGlowMakeEffect
#define efManagerDeadExplodeMakeEffect ndsBaseEFManagerDeadExplodeMakeEffect
#define efManagerSparkleWhiteDeadMakeEffect ndsBaseEFManagerSparkleWhiteDeadMakeEffect
#define efManagerRebirthHaloMakeEffect ndsBaseEFManagerRebirthHaloMakeEffect
#define efManagerStockSnapMakeEffect ndsBaseEFManagerStockSnapMakeEffect
#define efManagerStockStealStartMakeEffect ndsBaseEFManagerStockStealStartMakeEffect
#define efManagerStockStealEndMakeEffect ndsBaseEFManagerStockStealEndMakeEffect
#define efManagerBattleScoreMakeEffect ndsBaseEFManagerBattleScoreMakeEffect
#define efManagerEggBreakMakeEffect ndsBaseEFManagerEggBreakMakeEffect
#define efManagerFoxReflectorMakeEffect ndsBaseEFManagerFoxReflectorMakeEffect

#include "../../decomp/BattleShip-main/decomp/src/ef/efmanager.c"

#undef efManagerDamageNormalLightMakeEffect
#undef efManagerDamageNormalHeavyMakeEffect
#undef efManagerDamageFireMakeEffect
#undef efManagerDamageElectricMakeEffect
#undef efManagerDamageCoinMakeEffect
#undef efManagerDamageSlashMakeEffect
#undef efManagerDustExpandSmallMakeEffect
#undef efManagerFireGrindMakeEffect
#undef efManagerSparkleWhiteMakeEffect
#undef efManagerSparkleWhiteScaleMakeEffect
#undef efManagerDamageSpawnOrbsRandomMakeEffect
#undef efManagerDamageSpawnSparksRandomMakeEffect
#undef efManagerDamageSpawnMDustRandomMakeEffect
#undef efManagerImpactWaveMakeEffect
#undef efManagerQuakeMakeEffect
#undef efManagerSetOffMakeEffect
#undef efManagerShieldMakeEffect
#undef efManagerYoshiShieldMakeEffect
#undef efManagerCatchSwirlMakeEffect
#undef efManagerFlashMiddleMakeEffect
#undef efManagerKirbyVulcanJabMakeEffect
#undef efManagerSamusGrappleBeamGlowMakeEffect
#undef efManagerDeadExplodeMakeEffect
#undef efManagerSparkleWhiteDeadMakeEffect
#undef efManagerRebirthHaloMakeEffect
#undef efManagerStockSnapMakeEffect
#undef efManagerStockStealStartMakeEffect
#undef efManagerStockStealEndMakeEffect
#undef efManagerBattleScoreMakeEffect
#undef efManagerEggBreakMakeEffect
#undef efManagerFoxReflectorMakeEffect

GObj *efManagerFoxReflectorMakeEffect(GObj *fighter_gobj)
{
    dEFManagerFoxReflectorEffectDesc.o_dobjsetup =
        (intptr_t)llFoxSpecial2ReflectorDObjDesc;
    dEFManagerFoxReflectorEffectDesc.o_anim_joint =
        (intptr_t)llFoxSpecial2ReflectorStartAnimJoint;
    return ndsBaseEFManagerFoxReflectorMakeEffect(fighter_gobj);
}
