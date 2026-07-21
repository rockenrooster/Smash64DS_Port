#include <ef/effect.h>
#include <ft/ftdata_file_slots.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <it/item.h>
#include <sc/scene.h>
#include <wp/weapon.h>
#include <reloc_data.h>
#include <reloc_data_ftdata_symbols.h>
#include <nds/nds_effects.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_task39_effect_census.h>
#include <nds/timers.h>
#include <sys/audio.h>
#include <string.h>

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

#define efManagerInitEffects ndsBaseEFManagerInitEffects
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

#undef efManagerInitEffects
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

#if NDS_TASK39_FX_SHIELD
#define NDS_VISUAL_TEMPLATE_COUNT 12
#define NDS_TASK39_SHIELD_MESH_SCALE (1.0F / 6.0F)
#else
#define NDS_VISUAL_TEMPLATE_COUNT 8
#endif
#define NDS_VISUAL_TEMPLATE_VERTICES 16
#define NDS_VISUAL_TEMPLATE_COMMANDS 12

typedef enum NDSVisualTemplateKind
{
    nNDSVisualTemplateDust,
    nNDSVisualTemplateNormal,
    nNDSVisualTemplateFire,
    nNDSVisualTemplateElectric,
    nNDSVisualTemplateSparkle,
    nNDSVisualTemplateWave,
    nNDSVisualTemplateShield,
    nNDSVisualTemplateDeath,
    nNDSVisualTemplateShieldP2,
    nNDSVisualTemplateShieldP3,
    nNDSVisualTemplateShieldP4,
    nNDSVisualTemplateShieldDamage
} NDSVisualTemplateKind;

typedef struct NDSVisualTemplate
{
    u32 vertices[NDS_VISUAL_TEMPLATE_VERTICES][4];
    Gfx display_list[NDS_VISUAL_TEMPLATE_COMMANDS];
} NDSVisualTemplate;

static NDSVisualTemplate *sNdsVisualTemplates;

static void ndsEFManagerSetCommand(Gfx *command, u32 w0, u32 w1)
{
    command->words.w0 = w0;
    command->words.w1 = w1;
}

static void ndsEFManagerSetVertex(NDSVisualTemplate *template, u32 index,
                                  s16 x, s16 y, s16 z, u32 rgba)
{
    template->vertices[index][0] = ((u32)(u16)x << 16) | (u16)y;
    template->vertices[index][1] = (u32)(u16)z << 16;
    template->vertices[index][2] = 0u;
    template->vertices[index][3] = rgba;
}

static u32 ndsEFManagerPackTriangle(u32 v0, u32 v1, u32 v2)
{
    return ((v0 * 2u) << 16) | ((v1 * 2u) << 8) | (v2 * 2u);
}

static u32 ndsEFManagerBeginTemplate(NDSVisualTemplate *template,
                                     u32 vertex_count)
{
    ndsEFManagerSetCommand(&template->display_list[0], 0xd9000000u,
                           G_SHADE);
    ndsEFManagerSetCommand(&template->display_list[1], 0xd7000000u, 0u);
    ndsEFManagerSetCommand(
        &template->display_list[2],
        0x01000000u | (vertex_count << 12) | (vertex_count << 1),
        (u32)(uintptr_t)template->vertices);
    return 3u;
}

static void ndsEFManagerBuildStar(NDSVisualTemplate *template,
                                  u32 center_rgba, u32 outer_rgba)
{
    static const s16 outer[8][2] = {
        { 0, 180 }, { 42, 42 }, { 180, 0 }, { 42, -42 },
        { 0, -180 }, { -42, -42 }, { -180, 0 }, { -42, 42 }
    };
    u32 command;
    u32 i;

    ndsEFManagerSetVertex(template, 0u, 0, 0, 0, center_rgba);
    for (i = 0u; i < 8u; i++)
    {
        ndsEFManagerSetVertex(template, i + 1u, outer[i][0], outer[i][1],
                              0, outer_rgba);
    }
    command = ndsEFManagerBeginTemplate(template, 9u);
    for (i = 0u; i < 8u; i += 2u)
    {
        u32 next0 = ((i + 1u) & 7u) + 1u;
        u32 next1 = ((i + 2u) & 7u) + 1u;

        ndsEFManagerSetCommand(
            &template->display_list[command++],
            0x06000000u |
                ndsEFManagerPackTriangle(0u, i + 1u, next0),
            ndsEFManagerPackTriangle(0u, i + 2u, next1));
    }
    ndsEFManagerSetCommand(&template->display_list[command],
                           0xdf000000u, 0u);
}

static void ndsEFManagerBuildDust(NDSVisualTemplate *template,
                                  u32 center_rgba, u32 outer_rgba)
{
    static const s16 outer[6][2] = {
        { -170, -25 }, { -90, 55 }, { 0, 80 },
        { 90, 55 }, { 170, -25 }, { 0, -65 }
    };
    u32 command;
    u32 i;

    ndsEFManagerSetVertex(template, 0u, 0, 0, 0, center_rgba);
    for (i = 0u; i < 6u; i++)
    {
        ndsEFManagerSetVertex(template, i + 1u, outer[i][0], outer[i][1],
                              0, outer_rgba);
    }
    command = ndsEFManagerBeginTemplate(template, 7u);
    for (i = 0u; i < 6u; i += 2u)
    {
        u32 next0 = ((i + 1u) % 6u) + 1u;
        u32 next1 = ((i + 2u) % 6u) + 1u;

        ndsEFManagerSetCommand(
            &template->display_list[command++],
            0x06000000u |
                ndsEFManagerPackTriangle(0u, i + 1u, next0),
            ndsEFManagerPackTriangle(0u, i + 2u, next1));
    }
    ndsEFManagerSetCommand(&template->display_list[command],
                           0xdf000000u, 0u);
}

static void ndsEFManagerBuildRing(NDSVisualTemplate *template,
                                  u32 outer_rgba, u32 inner_rgba)
{
    static const s16 outer[8][2] = {
        { 0, 180 }, { 127, 127 }, { 180, 0 }, { 127, -127 },
        { 0, -180 }, { -127, -127 }, { -180, 0 }, { -127, 127 }
    };
    static const s16 inner[8][2] = {
        { 0, 105 }, { 74, 74 }, { 105, 0 }, { 74, -74 },
        { 0, -105 }, { -74, -74 }, { -105, 0 }, { -74, 74 }
    };
    u32 command;
    u32 i;

    for (i = 0u; i < 8u; i++)
    {
        ndsEFManagerSetVertex(template, i * 2u, outer[i][0], outer[i][1],
                              0, outer_rgba);
        ndsEFManagerSetVertex(template, (i * 2u) + 1u, inner[i][0],
                              inner[i][1], 0, inner_rgba);
    }
    command = ndsEFManagerBeginTemplate(template, 16u);
    for (i = 0u; i < 8u; i++)
    {
        u32 next = (i + 1u) & 7u;
        u32 outer0 = i * 2u;
        u32 inner0 = outer0 + 1u;
        u32 outer1 = next * 2u;
        u32 inner1 = outer1 + 1u;

        ndsEFManagerSetCommand(
            &template->display_list[command++],
            0x06000000u |
                ndsEFManagerPackTriangle(outer0, outer1, inner0),
            ndsEFManagerPackTriangle(inner0, outer1, inner1));
    }
    ndsEFManagerSetCommand(&template->display_list[command],
                           0xdf000000u, 0u);
}

static void ndsEFManagerBuildDisc(NDSVisualTemplate *template,
                                  u32 center_rgba, u32 outer_rgba)
{
    static const s16 outer[8][2] = {
        { 0, 180 }, { 127, 127 }, { 180, 0 }, { 127, -127 },
        { 0, -180 }, { -127, -127 }, { -180, 0 }, { -127, 127 }
    };
    u32 command;
    u32 i;

    ndsEFManagerSetVertex(template, 0u, 0, 0, 0, center_rgba);
    for (i = 0u; i < 8u; i++)
    {
        ndsEFManagerSetVertex(template, i + 1u, outer[i][0], outer[i][1],
                              0, outer_rgba);
    }
    ndsEFManagerSetVertex(template, 9u, -92, 76, 0, 0xffffffb0u);
    ndsEFManagerSetVertex(template, 10u, -58, 126, 0, 0xffffffb0u);
    ndsEFManagerSetVertex(template, 11u, 20, 143, 0, 0xffffffb0u);
    ndsEFManagerSetVertex(template, 12u, -28, 100, 0, 0xffffffb0u);
    command = ndsEFManagerBeginTemplate(template, 13u);
    /* Flat translucent shield: use the same proven XLU state as hit sparks. */
    ndsEFManagerSetCommand(&template->display_list[command++],
                           0xe200001cu, 0x00504240u);
    for (i = 0u; i < 8u; i += 2u)
    {
        ndsEFManagerSetCommand(
            &template->display_list[command++],
            0x06000000u |
                ndsEFManagerPackTriangle(0u, i + 1u, i + 2u),
            ndsEFManagerPackTriangle(0u, i + 2u,
                                     ((i + 2u) & 7u) + 1u));
    }
    ndsEFManagerSetCommand(
        &template->display_list[command++],
        0x06000000u | ndsEFManagerPackTriangle(9u, 10u, 11u),
        ndsEFManagerPackTriangle(9u, 11u, 12u));
    ndsEFManagerSetCommand(&template->display_list[command],
                           0xdf000000u, 0u);
}

static void ndsEFManagerInitVisualTemplates(void)
{
    sNdsVisualTemplates = syTaskmanMalloc(sizeof(*sNdsVisualTemplates) *
                                              NDS_VISUAL_TEMPLATE_COUNT,
                                          0x10);
    if (sNdsVisualTemplates == NULL)
    {
        gNdsVisualEffectTemplateBytes = 0u;
        return;
    }
    memset(sNdsVisualTemplates, 0,
           sizeof(*sNdsVisualTemplates) * NDS_VISUAL_TEMPLATE_COUNT);
    ndsEFManagerBuildDust(&sNdsVisualTemplates[nNDSVisualTemplateDust],
                          0xddd0b0ffu, 0x806c54ffu);
    ndsEFManagerBuildStar(&sNdsVisualTemplates[nNDSVisualTemplateNormal],
                          0xffffffffu, 0xffd040ffu);
    ndsEFManagerBuildStar(&sNdsVisualTemplates[nNDSVisualTemplateFire],
                          0xffff90ffu, 0xff4a10ffu);
    ndsEFManagerBuildStar(&sNdsVisualTemplates[nNDSVisualTemplateElectric],
                          0xffffffffu, 0x3090ffffu);
    ndsEFManagerBuildStar(&sNdsVisualTemplates[nNDSVisualTemplateSparkle],
                          0xffffffffu, 0x90e8ffffu);
    ndsEFManagerBuildRing(&sNdsVisualTemplates[nNDSVisualTemplateWave],
                          0x60ff80ffu, 0xffff80ffu);
#if NDS_TASK39_FX_SHIELD
    ndsEFManagerBuildDisc(&sNdsVisualTemplates[nNDSVisualTemplateShield],
                          0xffffff60u, 0xff000050u);
    ndsEFManagerBuildDisc(&sNdsVisualTemplates[nNDSVisualTemplateShieldP2],
                          0xffffff60u, 0x00ff0050u);
    ndsEFManagerBuildDisc(&sNdsVisualTemplates[nNDSVisualTemplateShieldP3],
                          0xffffff60u, 0x0000ff50u);
    ndsEFManagerBuildDisc(&sNdsVisualTemplates[nNDSVisualTemplateShieldP4],
                          0xffffff60u, 0x00000050u);
    ndsEFManagerBuildDisc(
        &sNdsVisualTemplates[nNDSVisualTemplateShieldDamage],
        0xffffff60u, 0xc0c0c050u);
#else
    ndsEFManagerBuildRing(&sNdsVisualTemplates[nNDSVisualTemplateShield],
                          0x40b8ffffu, 0xe0ffffffu);
#endif
    ndsEFManagerBuildRing(&sNdsVisualTemplates[nNDSVisualTemplateDeath],
                          0xff4060ffu, 0xffffffffu);
    gNdsVisualEffectTemplateBytes =
        sizeof(*sNdsVisualTemplates) * NDS_VISUAL_TEMPLATE_COUNT;
}

static NDSVisualTemplate *ndsEFManagerGetVisualTemplate(
    NDSVisualEffectKind kind)
{
    NDSVisualTemplateKind template_kind;

    if (sNdsVisualTemplates == NULL)
    {
        return NULL;
    }
    switch (kind)
    {
    case nNDSVisualEffectDust:
        template_kind = nNDSVisualTemplateDust;
        break;
    case nNDSVisualEffectHitFire:
        template_kind = nNDSVisualTemplateFire;
        break;
    case nNDSVisualEffectHitElectric:
        template_kind = nNDSVisualTemplateElectric;
        break;
    case nNDSVisualEffectCoin:
    case nNDSVisualEffectSparkle:
        template_kind = nNDSVisualTemplateSparkle;
        break;
    case nNDSVisualEffectImpactWave:
    case nNDSVisualEffectCatch:
        template_kind = nNDSVisualTemplateWave;
        break;
    case nNDSVisualEffectShield:
    case nNDSVisualEffectReflector:
        template_kind = nNDSVisualTemplateShield;
        break;
    case nNDSVisualEffectDeath:
    case nNDSVisualEffectRebirth:
        template_kind = nNDSVisualTemplateDeath;
        break;
    case nNDSVisualEffectSlash:
    case nNDSVisualEffectHitNormal:
    default:
        template_kind = nNDSVisualTemplateNormal;
        break;
    }
    return &sNdsVisualTemplates[template_kind];
}

static s32 ndsEFManagerVisualLifetime(NDSVisualEffectKind kind)
{
    switch (kind)
    {
    case nNDSVisualEffectDust:
        return 9;
    case nNDSVisualEffectCoin:
    case nNDSVisualEffectImpactWave:
        return 12;
    case nNDSVisualEffectCatch:
        return 14;
    case nNDSVisualEffectDeath:
        return 18;
    case nNDSVisualEffectHitFire:
    case nNDSVisualEffectSparkle:
        return 10;
    default:
        return 8;
    }
}

static f32 ndsEFManagerVisualGrowth(NDSVisualEffectKind kind)
{
    switch (kind)
    {
    case nNDSVisualEffectDust:
        return 0.06F;
    case nNDSVisualEffectImpactWave:
    case nNDSVisualEffectCatch:
        return 0.09F;
    case nNDSVisualEffectDeath:
        return 0.12F;
    case nNDSVisualEffectSlash:
        return 0.10F;
    default:
        return 0.04F;
    }
}

static f32 ndsEFManagerVisualSpin(NDSVisualEffectKind kind, s32 lr)
{
    f32 spin;

    switch (kind)
    {
    case nNDSVisualEffectCoin:
        spin = 0.26F;
        break;
    case nNDSVisualEffectHitElectric:
    case nNDSVisualEffectSparkle:
        spin = 0.20F;
        break;
    case nNDSVisualEffectHitNormal:
    case nNDSVisualEffectHitFire:
        spin = 0.14F;
        break;
    default:
        spin = 0.0F;
        break;
    }
    return (lr < 0) ? -spin : spin;
}

static void ndsEFManagerDestroyVisualEffect(GObj *effect_gobj)
{
    EFStruct *ep;

    if (ndsEFManagerIsVisualEffectGObj(effect_gobj) == FALSE)
    {
        return;
    }
    ep = efGetStruct(effect_gobj);
    if (ep != NULL)
    {
        efManagerSetPrevStructAlloc(ep);
    }
    if (gNdsVisualEffectActiveCount != 0u)
    {
        gNdsVisualEffectActiveCount--;
    }
    gNdsVisualEffectDestroyCount++;
    gcEjectGObj(effect_gobj);
}

#if NDS_TASK39_FX_SHIELD
static NDSVisualTemplate *ndsEFManagerShieldTemplate(s32 player,
                                                    sb32 is_damage)
{
    static const u8 player_templates[4] = {
        nNDSVisualTemplateShield,
        nNDSVisualTemplateShieldP2,
        nNDSVisualTemplateShieldP3,
        nNDSVisualTemplateShieldP4
    };

    if (is_damage != FALSE)
    {
        return &sNdsVisualTemplates[nNDSVisualTemplateShieldDamage];
    }
    return &sNdsVisualTemplates[player_templates[(u32)player & 3u]];
}

static void ndsEFManagerShieldProcDisplay(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *dobj = DObjGetStruct(effect_gobj);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 start = cpuGetTiming();
#endif

    if ((ep == NULL) || (dobj == NULL))
    {
        return;
    }
    dobj->dl = ndsEFManagerShieldTemplate(
                   ep->effect_vars.shield.player,
                   ep->effect_vars.shield.is_damage_shield)
                   ->display_list;
    gcDrawDObjTreeForGObj(effect_gobj);
    gNdsTask39FxShieldDrawCount++;
    ndsTask39EffectsEngage(NDS_TASK39_FX_ENGAGED_SHIELD);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    ndsTask39EffectsAddDrawTicks(cpuGetTiming() - start);
#endif
    ep->effect_vars.shield.is_damage_shield = FALSE;
}
#endif

static void ndsEFManagerVisualProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *dobj = DObjGetStruct(effect_gobj);

    if ((ep == NULL) || (dobj == NULL) || (ep->is_pause_effect != FALSE))
    {
        return;
    }
    if (ep->fighter_gobj != NULL)
    {
        DObj *joint = dobj->user_data.p;

        if (joint != NULL)
        {
            Vec3f pos = { 0.0F, 0.0F, 0.0F };

            gmCollisionGetFighterPartsWorldPosition(joint, &pos);
            dobj->translate.vec.f = pos;
#if NDS_TASK39_FX_SHIELD
            if (ep->effect_vars.common.size == nNDSVisualEffectShield)
            {
                FTStruct *fp = ftGetStruct(ep->fighter_gobj);

                if ((fp != NULL) && (fp->is_shield != FALSE))
                {
                    dobj->scale.vec.f.x = joint->scale.vec.f.x *
                                          NDS_TASK39_SHIELD_MESH_SCALE;
                    dobj->scale.vec.f.y = joint->scale.vec.f.y *
                                          NDS_TASK39_SHIELD_MESH_SCALE;
                    dobj->scale.vec.f.z = joint->scale.vec.f.z *
                                          NDS_TASK39_SHIELD_MESH_SCALE;
                }
            }
#endif
        }
        return;
    }
    dobj->translate.vec.f.x += ep->effect_vars.common.vel.x;
    dobj->translate.vec.f.y += ep->effect_vars.common.vel.y;
    dobj->translate.vec.f.z += ep->effect_vars.common.vel.z;
    dobj->rotate.vec.f.z += dobj->anim_frame;
    dobj->scale.vec.f.x += dobj->anim_speed;
    dobj->scale.vec.f.y += dobj->anim_speed;
    effect_gobj->anim_frame -= 1.0F;
    if (effect_gobj->anim_frame <= 0.0F)
    {
        ndsEFManagerDestroyVisualEffect(effect_gobj);
    }
}

s32 ndsEFManagerIsVisualEffectGObj(GObj *effect_gobj)
{
    DObj *dobj;
    u32 i;

    if ((effect_gobj == NULL) || (effect_gobj->id != nGCCommonKindEffect) ||
        (sNdsVisualTemplates == NULL))
    {
        return FALSE;
    }
    dobj = DObjGetStruct(effect_gobj);
    if (dobj == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_VISUAL_TEMPLATE_COUNT; i++)
    {
        if (dobj->dl == sNdsVisualTemplates[i].display_list)
        {
            return TRUE;
        }
    }
    return FALSE;
}

GObj *ndsEFManagerMakeVisualEffect(NDSVisualEffectKind kind,
                                    const Vec3f *pos, f32 scale, s32 lr,
                                    GObj *fighter_gobj)
{
    NDSVisualTemplate *template = ndsEFManagerGetVisualTemplate(kind);
    EFStruct *ep;
    GObj *effect_gobj;
    DObj *dobj;

    if ((template == NULL) ||
        ((u32)kind >= (u32)nNDSVisualEffectKindCount))
    {
        gNdsVisualEffectDropCount++;
        return NULL;
    }
    ep = (fighter_gobj != NULL) ? efManagerGetEffectForce() :
                                  efManagerGetEffectNoForce();
    if (ep == NULL)
    {
        gNdsVisualEffectDropCount++;
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL,
                                    nGCCommonLinkIDEffect,
                                    GOBJ_PRIORITY_DEFAULT);
    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);
        gNdsVisualEffectDropCount++;
        return NULL;
    }
    effect_gobj->user_data.p = ep;
    dobj = gcAddDObjForGObj(effect_gobj, template->display_list);
    if ((dobj == NULL) ||
        (gcAddXObjForDObjFixed(dobj, nGCMatrixKindTraRotRpyRSca, 0) ==
         NULL))
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
        gNdsVisualEffectDropCount++;
        return NULL;
    }
    if (scale < 0.2F)
    {
        scale = 0.2F;
    }
    else if (scale > 5.0F)
    {
        scale = 5.0F;
    }
#if NDS_TASK39_FX_SHIELD
    if (kind == nNDSVisualEffectShield)
    {
        scale *= NDS_TASK39_SHIELD_MESH_SCALE;
    }
#endif
    dobj->scale.vec.f.x = scale;
    dobj->scale.vec.f.y = scale;
    dobj->scale.vec.f.z = scale;
    if (kind == nNDSVisualEffectDust)
    {
        dobj->scale.vec.f.y *= 0.65F;
    }
    else if (kind == nNDSVisualEffectSlash)
    {
        dobj->scale.vec.f.x *= 1.5F;
        dobj->scale.vec.f.y *= 0.35F;
    }
    if (pos != NULL)
    {
        dobj->translate.vec.f = *pos;
    }
    dobj->anim_speed = ndsEFManagerVisualGrowth(kind);
    dobj->anim_frame = ndsEFManagerVisualSpin(kind, lr);
    effect_gobj->anim_frame = (f32)ndsEFManagerVisualLifetime(kind);
    ep->proc_update = ndsEFManagerVisualProcUpdate;
    ep->effect_vars.common.vel.x = 0.0F;
    ep->effect_vars.common.vel.y = 0.0F;
    ep->effect_vars.common.vel.z = 0.0F;
    ep->effect_vars.common.size = kind;
    if (kind == nNDSVisualEffectDust)
    {
        ep->effect_vars.common.vel.x = (f32)-lr * 1.5F;
        ep->effect_vars.common.vel.y = 2.0F;
    }
    else if ((kind == nNDSVisualEffectCoin) ||
             (kind == nNDSVisualEffectSparkle))
    {
        ep->effect_vars.common.vel.y = 2.5F;
    }
    ep->fighter_gobj = fighter_gobj;
    if (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        DObj *joint = NULL;

        if (fp != NULL)
        {
            joint = fp->joints[(kind == nNDSVisualEffectShield) ?
                                   nFTPartsJointYRotN :
                                   nFTPartsJointTopN];
            fp->is_effect_attach = TRUE;
#if NDS_TASK39_FX_SHIELD
            if (kind == nNDSVisualEffectShield)
            {
                ep->effect_vars.shield.player = fp->player;
                ep->effect_vars.shield.is_damage_shield = FALSE;
            }
#endif
        }
        dobj->user_data.p = joint;
        if (joint != NULL)
        {
            Vec3f joint_pos = { 0.0F, 0.0F, 0.0F };

            gmCollisionGetFighterPartsWorldPosition(joint, &joint_pos);
            dobj->translate.vec.f = joint_pos;
        }
    }
    gcAddGObjProcess(effect_gobj, ndsEFManagerVisualProcUpdate,
                     nGCProcessKindFunc, 3);
    gcAddGObjDisplay(
        effect_gobj,
#if NDS_TASK39_FX_SHIELD
        (kind == nNDSVisualEffectShield) ? ndsEFManagerShieldProcDisplay :
#endif
                                           gcDrawDObjTreeForGObj,
        18, 2, -1);
    gNdsVisualEffectCreateCount++;
    gNdsVisualEffectActiveCount++;
    if (gNdsVisualEffectActiveCount > gNdsVisualEffectMaxActiveCount)
    {
        gNdsVisualEffectMaxActiveCount = gNdsVisualEffectActiveCount;
    }
    gNdsVisualEffectKindMask |= 1u << (u32)kind;
    return effect_gobj;
}

void ndsEFManagerStopAttachedVisualEffects(GObj *fighter_gobj)
{
    GObj *effect_gobj = gGCCommonLinks[nGCCommonLinkIDEffect];

    if (fighter_gobj == NULL)
    {
        return;
    }
    while (effect_gobj != NULL)
    {
        GObj *next = effect_gobj->link_next;
        EFStruct *ep = efGetStruct(effect_gobj);

        if ((ep != NULL) && (ep->fighter_gobj == fighter_gobj) &&
            (ndsEFManagerIsVisualEffectGObj(effect_gobj) != FALSE))
        {
            ndsEFManagerDestroyVisualEffect(effect_gobj);
        }
        effect_gobj = next;
    }
}

void efManagerInitEffects(void)
{
    ndsTask39EffectCensusReset();
    gNdsVisualEffectCreateCount = 0u;
    gNdsVisualEffectDestroyCount = 0u;
    gNdsVisualEffectDropCount = 0u;
    gNdsVisualEffectActiveCount = 0u;
    gNdsVisualEffectMaxActiveCount = 0u;
    gNdsVisualEffectKindMask = 0u;
    gNdsVisualEffectTemplateBytes = 0u;
    ndsBaseEFManagerInitEffects();
    ndsEFManagerInitVisualTemplates();
}

GObj *efManagerFoxReflectorMakeEffect(GObj *fighter_gobj)
{
    return ndsEFManagerMakeVisualEffect(nNDSVisualEffectReflector, NULL,
                                        1.6F, 1, fighter_gobj);
}
