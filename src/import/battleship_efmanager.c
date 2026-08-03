#include <ef/effect.h>
#include <ft/ftdata_file_slots.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <it/item.h>
#include <sc/scene.h>
#include <wp/weapon.h>
#include <reloc_data.h>
#include <reloc_data_ftdata_symbols.h>
#include <nds/generated/nds_particle_banks.generated.h>
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

/* Shrink the effect-instance pool AT ITS ALLOCATION rather than truncating the
 * free list afterwards. efManagerInitEffects does one
 * syTaskmanMalloc(sizeof(EFStruct) * EFFECT_ALLOC_NUM) out of
 * gSYTaskmanGeneralHeap, and that heap is bump-allocated -- so a truncated list
 * bounds concurrency but hands nothing back, and the 26 unreachable entries go
 * on costing the match their full size. Redefining the count here makes the
 * source allocate exactly what the port will use, which is general heap
 * returned to the margin that ifCommonSetMaxNumGObj watches at 25,600.
 *
 * The list truncation in ndsEFManagerBoundEffectPool stays as the belt: it
 * no-ops when this define has already taken effect, and still bounds the pool
 * if a future include order stops it from applying. */
#undef EFFECT_ALLOC_NUM
#define EFFECT_ALLOC_NUM NDS_R2_EFFECT_POOL

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
#define NDS_VISUAL_TEMPLATE_COUNT 14
#define NDS_TASK39_SHIELD_MESH_SCALE (1.0F / 6.0F)
#else
#define NDS_VISUAL_TEMPLATE_COUNT 10
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
    nNDSVisualTemplateReflector,
    nNDSVisualTemplateRebirth,
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

/* `glint_dx` slides the highlight patch along x. The patch is centred at
 * (-39.5, 111), i.e. UPPER LEFT, which is where it has always been and where
 * nothing in the source puts it. The owner's N64 capture and the extracted
 * shield asset agree that the shield's highlight sits TOP MIDDLE -- "the glint
 * is at the top middle where mario's ear is" -- so the shield passes +40 to
 * centre it and the reflector and respawn pad pass 0 to stay byte-identical.
 * They get their own reading when someone has a reference for them; guessing
 * for all three off one screenshot is how the upper-left one got here. */
static void ndsEFManagerBuildDisc(NDSVisualTemplate *template,
                                  u32 center_rgba, u32 outer_rgba,
                                  s16 glint_dx)
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
    ndsEFManagerSetVertex(template, 9u, (s16)(-92 + glint_dx), 76, 0,
                          0xffffffb0u);
    ndsEFManagerSetVertex(template, 10u, (s16)(-58 + glint_dx), 126, 0,
                          0xffffffb0u);
    ndsEFManagerSetVertex(template, 11u, (s16)(20 + glint_dx), 143, 0,
                          0xffffffb0u);
    ndsEFManagerSetVertex(template, 12u, (s16)(-28 + glint_dx), 100, 0,
                          0xffffffb0u);
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
    /* BUGS.md "Shield VFX is not correct". The RGB here was already exact --
     * every pair matches dEFManagerShieldColors (efmanager.c:450) -- but the
     * ALPHA was not, and alpha is the whole look of a shield bubble. The source
     * sets 0xC0 on BOTH prim and env for all five entries
     * (efManagerShieldProcDisplay, efmanager.c:4112); this shipped 0x60 centre
     * and 0x50 rim, so the bubble drew at half opacity in the middle and less
     * than half at the edge where it reads as an outline. These are N64 Gfx
     * templates, so the vertex alpha IS the transparency -- 0xe200001c is
     * G_SETOTHERMODE_L carrying the XLU blend state, not a level.
     * NOTE: the shield draws through gcDrawDObjTreeForGObj, so the 2026-08-02
     * particle-camera fix does NOT touch it. This row was its own defect. */
    ndsEFManagerBuildDisc(&sNdsVisualTemplates[nNDSVisualTemplateShield],
                          0xffffffc0u, 0xff0000c0u, 40);
    ndsEFManagerBuildDisc(&sNdsVisualTemplates[nNDSVisualTemplateShieldP2],
                          0xffffffc0u, 0x00ff00c0u, 40);
    ndsEFManagerBuildDisc(&sNdsVisualTemplates[nNDSVisualTemplateShieldP3],
                          0xffffffc0u, 0x0000ffc0u, 40);
    ndsEFManagerBuildDisc(&sNdsVisualTemplates[nNDSVisualTemplateShieldP4],
                          0xffffffc0u, 0x000000c0u, 40);
    ndsEFManagerBuildDisc(
        &sNdsVisualTemplates[nNDSVisualTemplateShieldDamage],
        0xffffffc0u, 0xc0c0c0c0u, 40);
#else
    ndsEFManagerBuildRing(&sNdsVisualTemplates[nNDSVisualTemplateShield],
                          0x40b8ffffu, 0xe0ffffffu);
#endif
    ndsEFManagerBuildRing(&sNdsVisualTemplates[nNDSVisualTemplateDeath],
                          0xff4060ffu, 0xffffffffu);
    /* BUGS.md #5 and #9: Reflector shared the Shield slot and Rebirth shared
     * the Death slot, so Fox's down B drew Mario's P1 shield disc and a
     * respawn drew the KO ring. Both get their own slot. The hues reuse pairs
     * already chosen in this file -- the pre-Task39 shield ring's blue for the
     * reflector barrier, the sparkle pair for the respawn flash -- so this
     * stays a source-derived approximation rather than a new palette.
     * Reproducing the original textured bursts needs the particle banks and
     * is P2 (KNOWN_ISSUES.md). */
    ndsEFManagerBuildDisc(&sNdsVisualTemplates[nNDSVisualTemplateReflector],
                          0xe0ffff60u, 0x40b8ff50u, 0);
    /* A DISC, not a ring. BUGS row 3 stayed open through a lifetime fix (8 ->
     * 390 frames) and a growth fix, and the owner still reported "I don't see
     * the floating platform" -- because everything about it was alive and
     * correctly sized and it was drawing an OUTLINE. The chain checks out:
     * created, alive 390 frames, scale clamped to at least 0.2, template
     * geometry present. What was left is that a thin ring is nearly invisible
     * at this scale, and the source's respawn platform is not a ring: it is a
     * solid translucent disc the fighter stands on. BuildDisc is the same
     * source-derived approximation the reflector already uses, so this stays a
     * cheap approximation rather than a new asset. */
    ndsEFManagerBuildDisc(&sNdsVisualTemplates[nNDSVisualTemplateRebirth],
                          0xffffffffu, 0x90e8ffffu, 0);
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
        template_kind = nNDSVisualTemplateShield;
        break;
    case nNDSVisualEffectReflector:
        template_kind = nNDSVisualTemplateReflector;
        break;
    case nNDSVisualEffectDeath:
        template_kind = nNDSVisualTemplateDeath;
        break;
    case nNDSVisualEffectRebirth:
        template_kind = nNDSVisualTemplateRebirth;
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
    /* BUGS row 3, "Respawn floating platform isn't visible when respawning."
     * It was visible -- for EIGHT FRAMES. Every kind in this table is a hit
     * flash that belongs on screen for an eighth of a second, and Rebirth was
     * falling through to the same default while the source keeps its halo for
     * FTCOMMON_REBIRTH_HALO_DESPAWN_WAIT (ftcommon.h:14) and spends the first
     * FTCOMMON_REBIRTH_HALO_LOWER_WAIT of it descending. 390 is that constant.
     * Measured before the change: gNdsVisualEffectActiveCount was already 0
     * twenty-four frames after the maker ran, with the Rebirth bit set in
     * gNdsVisualEffectKindMask -- created, then gone before anyone could see
     * it. */
    case nNDSVisualEffectRebirth:
        return 390;
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
    /* Zero, and it is a precondition of the lifetime above rather than a taste
     * call. Growth is per frame, so the default 0.04 over a hit flash's eight
     * frames is a 32% swell and over Rebirth's 390 it is a scale of 16 -- the
     * platform would fill the stage before it despawned. The source halo holds
     * its size and descends; it never grows. */
    case nNDSVisualEffectRebirth:
        return 0.0F;
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

/* OUTSIDE #if NDS_TASK39_FX_SHIELD, and that is not incidental. The respawn pad
 * has nothing to do with the shield flag, and this block sat inside it for one
 * build: the flag is 0 in the DEFAULT configuration -- the published
 * smash64ds.nds -- so the proc vanished while its use site did not, and the
 * compile failed on exactly the shape of defect fixed hours earlier for
 * ndsRendererSetParticleCamera. A symbol's guard must be the guard of the thing
 * it belongs to, not of whatever it happens to be typed next to.
 *
 * ndsEFManagerBuildDisc's `outer` table reaches 180 vertex units on both axes,
 * so every disc template -- shield and rebirth alike -- has a local radius of
 * 180, and the quad has to match it or swapping between the sheet path and its
 * tree-draw fallback would visibly resize the effect. (The builder's third
 * argument is glint_dx, not a radius; reading it as one is how this constant
 * was first written as 60.)
 *
 * The glow is I4 -- greyscale with no colour of its own -- so the tint is the
 * port's choice; white keeps the source's additive-glow read and lets the
 * texture's own intensity ramp do the shaping. Alpha 0xC0 is the source's own
 * value for a translucent effect overlay (efmanager.c:4112). */
#define NDS_R2_REBIRTH_QUAD_SIZE 180.0F
/* BGR555 like the shield's -- see ndsEFManagerShieldQuadColor. 0xffffff happened
 * to survive the wrong packing intact (all fifteen low bits set), which is why
 * only the shield read as black and this one merely read as invisible; its
 * fault was the texture encoding, not the colour. */
#define NDS_R2_REBIRTH_QUAD_COLOR 0x7fffu
#define NDS_R2_REBIRTH_QUAD_ALPHA 0xC0u

/* THE RESPAWN PAD'S SOURCE ASSET IS THE MBallRays GLOW, NOT A DISC.
 * dEFCommonEffects3_RebirthHalo (relocData/85_EFCommonEffects3.c:856) is a
 * four-node chain: node[1] carries the MBallRays lists at translate
 * (0, -60, 0), node[2] a second set at the origin, and its AnimJoint spins
 * node[2] rotY 0 -> 2*pi over 30 frames on a self-looping script. Both nodes
 * draw the same I4 32x16 glow texture. The -60 is already handled in
 * ndsEFManagerVisualProcUpdate; this is the other half the owner asked for --
 * *"not using correct asset for the revival platform"* -- the art itself.
 *
 * The spin is deliberately NOT reproduced. A camera-facing quad has no
 * meaningful rotY, and the source's rotation is what makes a flat pair of ray
 * fans read as a disc from any angle; a billboard already reads that way. If
 * the owner wants the sweep back, the honest route is node[2] as a second quad
 * with a rotating UV, not a spin on a quad that always faces you. */
static void ndsEFManagerRebirthProcDisplay(GObj *effect_gobj)
{
    DObj *dobj = DObjGetStruct(effect_gobj);

    if (dobj == NULL)
    {
        return;
    }
    if (ndsParticleDrawSourceAssetQuad(
            NDS_PARTICLE_QUAD_REBIRTH_TEXTURE, &dobj->translate.vec.f,
            dobj->scale.vec.f.x * NDS_R2_REBIRTH_QUAD_SIZE,
            NDS_R2_REBIRTH_QUAD_COLOR,
            NDS_R2_REBIRTH_QUAD_ALPHA) == FALSE)
    {
        gcDrawDObjTreeForGObj(effect_gobj);
    }
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

/* Local radius 180, same table, same reason as the rebirth constant above. */
#define NDS_R2_SHIELD_QUAD_SIZE 180.0F
/* 0xC0 on both prim and env for all five entries, efmanager.c:4112. The same
 * value the disc templates carry; see the comment on dEFManagerShieldColors
 * below for why alpha is the whole look of a shield. */
#define NDS_R2_SHIELD_QUAD_ALPHA 0xC0u

/* BGR555, NOT 0xRRGGBB. ndsRendererSubmitParticleQuad takes the DS's own vertex
 * colour packing -- red in bits 0-4, green 5-9, blue 10-14 -- which
 * lbParticleDrawTextures builds as `(r>>3) | (g>>3)<<5 | (b>>3)<<10`. This
 * function first returned 0xRRGGBB constants, and 0xff0000 read through that
 * packing is r=0 g=0 b=0: the owner reported the shield as *"Looks Black in
 * color"*, which is exactly what it was. A wrong-unit colour does not look
 * wrong, it looks like a different bug.
 *
 * Colour only -- DS vertex colour has no alpha channel, so the transparency
 * travels as the separate POLYGON_ATTR alpha above. The values are the rim
 * halves of the disc templates, which match dEFManagerShieldColors
 * (efmanager.c:450). */
#define NDS_R2_BGR555(r, g, b) \
    ((((u32)(r) >> 3) & 31u) | ((((u32)(g) >> 3) & 31u) << 5) | \
     ((((u32)(b) >> 3) & 31u) << 10))

static u32 ndsEFManagerShieldQuadColor(s32 player)
{
    static const u32 rim[] = {
        NDS_R2_BGR555(0xff, 0x00, 0x00),
        NDS_R2_BGR555(0x00, 0xff, 0x00),
        NDS_R2_BGR555(0x00, 0x00, 0xff),
        NDS_R2_BGR555(0x40, 0x40, 0x40)
    };

    return ((u32)player < ARRAY_COUNT(rim)) ?
        rim[player] : NDS_R2_BGR555(0xc0, 0xc0, 0xc0);
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
    /* THE SOURCE SHIELD IS A TEXTURED QUAD, NOT A DISC. Its whole asset is
     * dFTManagerCommon_Shield (relocData/163_FTManagerCommon.c:39): a three-node
     * DObjDesc whose one drawing node carries a 21-command DL over exactly FOUR
     * vertices and an IA8 16x32 texture. Four vertices is a quad, and a quad
     * that always faces the camera is what the particle sheet draws -- so the
     * shield now draws its own art instead of the procedural ring-and-disc that
     * stood in for it, which is what the owner filed.
     *
     * The template is still built and still assigned, because it carries the
     * per-player colour the source tints the bubble with and it is the fallback
     * the tree draw uses when the sheet is unavailable (no atlas, texture not
     * seated, degenerate camera). A shield that silently stops drawing is worse
     * than one drawn plainly. */
    dobj->dl = ndsEFManagerShieldTemplate(
                   ep->effect_vars.shield.player,
                   ep->effect_vars.shield.is_damage_shield)
                   ->display_list;
    if (ndsParticleDrawSourceAssetQuad(
            NDS_PARTICLE_QUAD_SHIELD_TEXTURE, &dobj->translate.vec.f,
            dobj->scale.vec.f.x * NDS_R2_SHIELD_QUAD_SIZE,
            ndsEFManagerShieldQuadColor(ep->effect_vars.shield.player),
            NDS_R2_SHIELD_QUAD_ALPHA) == FALSE)
    {
        gcDrawDObjTreeForGObj(effect_gobj);
    }
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
            /* THE RESPAWN PAD SITS UNDER THE FIGHTER, NOT ON THEM. Read off the
             * source asset rather than guessed: dEFManagerRebirthHaloEffectDesc
             * points at llEFCommonEffects3RebirthHaloDObjDesc, reloc file 0x55
             * offset 0x2AC0, and that is a CHAIN -- node[1] carries a display
             * list at translate (0, -60, 0) and node[2] a second one at the
             * origin. The port draws a single template and pinned it to the
             * joint's own world position, so the pad rendered centred ON the
             * fighter instead of sixty units beneath their feet. That is the
             * owner's "I don't see the CORRECT floating platform": it is there
             * and it is inside them.
             * Only the pad moves; every other visual keeps the joint position. */
            if (ep->effect_vars.common.size == nNDSVisualEffectRebirth)
            {
                pos.y -= 60.0F;
            }
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
        (kind == nNDSVisualEffectRebirth) ? ndsEFManagerRebirthProcDisplay :
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

/* ------------------------------------------------------------------------
 * EFDesc FILE-OFFSET RESOLUTION
 *
 * The root cause of the owner's 2026-08-01 "the KO burst freezes the game",
 * and -- because it is systemic -- of the heap exhaustion that has been read
 * as "source effects are too expensive" for this whole campaign.
 *
 * On N64, llEFCommonEffects2DeadExplodeDefaultDObjDesc and its ~180 siblings
 * are ABSOLUTE linker symbols: the symbol's ADDRESS *is* the byte offset into
 * the effect file, so `&llFoo` evaluates to something like 0x4F08 and
 * efManagerMakeEffect's raw `addr + effect_desc->o_dobjsetup` arithmetic is
 * correct. This port supplies them from a generated header as
 * `static uintptr_t llFoo = 0x4F08u`, so `&llFoo` is the VARIABLE'S ADDRESS in
 * main RAM, not 0x4F08 -- and every EFDesc initialiser in the decomp is written
 * `&llFoo`. All 182 references in efmanager.c are &-prefixed; none reads the
 * value, so nothing else caught it.
 *
 * The port's reloc design normally absorbs exactly this: a symbol's address is
 * a lookup TOKEN and ndsRelocGetFileData translates it. efManagerMakeEffect
 * does not go through that translation -- it adds the field to the file base
 * directly -- so this one seam kept the N64 assumption.
 *
 * The failure is not a wrong picture, it is a hang. `addr + &llFoo` is roughly
 * 0x023xxxxx + 0x021xxxxx = 0x044xxxxx, far outside the DS's 4 MB, and
 * gcSetupCustomDObjs then walks that as a DObjDesc tree and allocates a
 * 136-byte DObj per bogus node until syMallocSet gives up in its `for (;;)`.
 * Captured verbatim: MALLOCOVF=1 req=136 head=112, gcSetupCustomDObjs with
 * dobjdesc=0x446da28, under efManagerMakeEffect(dEFManagerDeadExplodeEffectDesc)
 * from ftCommonDeadLeftSetStatus. Note what this means for the earlier
 * measurement that priced a source effect at "~5 DObjs": that was never the
 * cost of the effect, it was a runaway walk over garbage.
 *
 * Recovering the offset is one dereference, because the generated symbol still
 * HOLDS it. The guard makes that idempotent, which matters: efManagerInitEffects
 * runs once per scene, and two of these descs have their offsets reassigned at
 * runtime from address-valued tables. */
static intptr_t ndsEFManagerResolveOffset(intptr_t value)
{
    /* File offsets are small (the largest in the generated header is ~0xD000);
     * anything at or above 0x01000000 is a RAM address, i.e. still unresolved.
     * Zero is meaningful -- efManagerMakeEffect tests it -- and passes through. */
    if ((uintptr_t)value >= 0x01000000u)
    {
        gNdsEFDescResolveCount++;
        return (intptr_t) * (uintptr_t *)value;
    }
    return value;
}

/* Byte span of the LOADED file a desc offsets into, or 0 when this port has no
 * way to know. Only the three EF common files are knowable from the desc alone
 * -- file_head is a pointer to the slot, not a file id -- and they are the ones
 * that matter here. */
static size_t ndsEFManagerFileSpan(void **file_head)
{
    if (file_head == &gEFManagerFiles[0])
    {
        return ndsRelocGetLoadedFileSize(&llEFCommonEffects1FileID);
    }
    if (file_head == &gEFManagerFiles[1])
    {
        return ndsRelocGetLoadedFileSize(&llEFCommonEffects2FileID);
    }
    if (file_head == &gEFManagerFiles[2])
    {
        return ndsRelocGetLoadedFileSize(&llEFCommonEffects3FileID);
    }
    return 0u;
}

/* Resolving the offsets is necessary but not sufficient: a correct offset into
 * a file this port never shipped is still a walk over unrelated heap.
 * lbRelocGetExternHeapFile returns the raw uninitialised malloc when an asset
 * is absent, and lbRelocGetFileSize then answers sizeof(Sprite), so the base
 * pointer looks perfectly valid and `base + 0x4F08` lands twenty kilobytes past
 * it -- which is the CPU abort that replaced the malloc spin once the offsets
 * were fixed.
 *
 * Neutralising the desc uses a door the source already opens: efManagerMakeEffect
 * returns the bare GObj as soon as proc_display is NULL, before it touches
 * file_head at all. So a desc whose file cannot back it produces an effect GObj
 * with no DObj tree -- nothing drawn, nothing walked -- and the guarded
 * accessors downstream see a NULL DObjGetStruct and return cleanly. */
static void ndsEFManagerResolveDescOffsets(EFDesc *desc)
{
    size_t span;

    desc->o_dobjsetup     = ndsEFManagerResolveOffset(desc->o_dobjsetup);
    desc->o_mobjsub       = ndsEFManagerResolveOffset(desc->o_mobjsub);
    desc->o_anim_joint    = ndsEFManagerResolveOffset(desc->o_anim_joint);
    desc->o_matanim_joint = ndsEFManagerResolveOffset(desc->o_matanim_joint);

    if (desc->proc_display == NULL) { return; }

    span = ndsEFManagerFileSpan(desc->file_head);
    if (span == 0u) { return; }          /* not knowable: leave it alone */

    if ((*desc->file_head == NULL) ||
        ((size_t)desc->o_dobjsetup >= span) ||
        ((size_t)desc->o_mobjsub >= span) ||
        ((size_t)desc->o_anim_joint >= span) ||
        ((size_t)desc->o_matanim_joint >= span))
    {
        desc->proc_display = NULL;
        gNdsEFDescDisabledCount++;
    }
}

/* The EFDescs reachable in the P1 milestone -- Mario, Fox and the shared
 * combat effects. Listed rather than swept, because they are separate globals
 * with no guaranteed layout, and because a desc left out is a heap-exhaustion
 * hang rather than a missing effect: too severe to leave to a pattern match.
 *
 * Deliberately NOT the full set of fifty, and split by NDS_R2_SOURCE_EFFECTS_FULL.
 * Naming a desc here KEEPS IT LINKED, and this list is charged in boot arena,
 * measured twice on 2026-08-01:
 *
 *   all 50 descs   arena 1,265,664  battle heap low-water 21,784  (cap latched at 47)
 *   16 descs       arena 1,269,760  battle heap low-water 23,720  (still under)
 *   2 descs        arena 1,273,856  battle heap low-water 26,876  (clear)
 *
 * The latch is ifCommonSetMaxNumGObj at 25,600 free, so the first two both
 * capped the GObj pool for the whole match -- a correctness fix that shrinks
 * the arena is a regression with extra steps. DeadExplode is the only source
 * DObj-tree maker that can spawn with the flag off, so it is the only
 * unconditional desc. Rebirth uses the bounded DS visual seam: source effect
 * DL links are not submitted by the battle hardware path, which is why the
 * respawn platform was invisible.
 *
 * dEFManagerMBallThrown/CaptureKirbyStar/LoseKirbyStar are excluded for a
 * second reason: their file_head is &gITManagerCommonData, which this ROM does
 * not link, so naming them is a link error rather than a fix. */
#if NDS_R2_SOURCE_EFFECTS_FULL
#define NDS_EF_MANAGER_DESCS_FULL(X) \
    X(dEFManagerDamageSlashEffectDesc) \
    X(dEFManagerShockSmallEffectDesc) \
    X(dEFManagerDamageFlyOrbsEffectDesc) \
    X(dEFManagerDamageSpawnOrbsEffectDesc) \
    X(dEFManagerImpactWaveEffectDesc) \
    X(dEFManagerDamageFlySparksEffectDesc) \
    X(dEFManagerDamageSpawnSparksEffectDesc) \
    X(dEFManagerDamageFlyMDustEffectDesc) \
    X(dEFManagerDamageSpawnMDustEffectDesc) \
    X(dEFManagerFireSparkEffectDesc) \
    X(dEFManagerShieldEffectDesc) \
    X(dEFManagerCatchSwirlEffectDesc) \
    X(dEFManagerReflectBreakEffectDesc)
#else
#define NDS_EF_MANAGER_DESCS_FULL(X)
#endif

#define NDS_EF_MANAGER_DESCS(X) \
    X(dEFManagerDeadExplodeEffectDesc) \
    NDS_EF_MANAGER_DESCS_FULL(X)

static void ndsEFManagerResolveAllDescOffsets(void)
{
    /* Recorded so a soak can tell "the effect is disabled because its file
     * cannot back it" from "the resolver is broken". These are the LOADED
     * spans: 0 means not resident. Do not switch this to lbRelocGetFileSize --
     * it answers sizeof(Sprite) for a resident file, and reading its 68 as
     * "the effect assets are missing" cost a wrong conclusion on 2026-08-01
     * when all three were loaded and EFCommonEffects2 was 28,432 bytes. */
    gNdsEFDescEffectsSpan[0] = (u32)ndsRelocGetLoadedFileSize(&llEFCommonEffects1FileID);
    gNdsEFDescEffectsSpan[1] = (u32)ndsRelocGetLoadedFileSize(&llEFCommonEffects2FileID);
    gNdsEFDescEffectsSpan[2] = (u32)ndsRelocGetLoadedFileSize(&llEFCommonEffects3FileID);

#define NDS_EF_RESOLVE_ONE(name) ndsEFManagerResolveDescOffsets(&name);
    NDS_EF_MANAGER_DESCS(NDS_EF_RESOLVE_ONE)
#undef NDS_EF_RESOLVE_ONE
}

/* Install NDS_R2_EFFECT_POOL as the effect-instance pool depth by truncating
 * the free list the source just built. See include/nds/nds_startup.h for why
 * bounding this bounds the DObj peak, and why refusal is source behaviour
 * rather than a dropped effect.
 *
 * Truncation, not a smaller syTaskmanMalloc: the block is one allocation of
 * EFFECT_ALLOC_NUM entries from a bump allocator, so shortening it would not
 * return anything, and the entries past the cut are simply never reachable.
 * Nothing outside efmanager.c indexes the array -- every user goes through the
 * head push/pop pair -- so the unreachable tail is inert. */
static void ndsEFManagerBoundEffectPool(void)
{
    EFStruct *ep = sEFManagerStructsAllocFree;
    s32 depth = 1;

    if ((ep == NULL) || (NDS_R2_EFFECT_POOL >= EFFECT_ALLOC_NUM))
    {
        gNdsEffectPoolDepth = (u32)sEFManagerStructsFreeNum;
        return;
    }
    while ((depth < NDS_R2_EFFECT_POOL) && (ep->next != NULL))
    {
        ep = ep->next;
        depth++;
    }
    ep->next = NULL;
    sEFManagerStructsFreeNum = depth;
    gNdsEffectPoolDepth = (u32)depth;
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
    ndsEFManagerResolveAllDescOffsets();
    ndsEFManagerBoundEffectPool();
    ndsEFManagerInitVisualTemplates();
}

GObj *efManagerFoxReflectorMakeEffect(GObj *fighter_gobj)
{
    return ndsEFManagerMakeVisualEffect(nNDSVisualEffectReflector, NULL,
                                        1.6F, 1, fighter_gobj);
}

/* ------------------------------------------------------------------------
 * SOURCE EFFECTS, not stand-ins.
 *
 * Every function below is a one-line forward to the decomp implementation that
 * this file already compiles as ndsBaseEFManager*. They exist because the port
 * shipped WEAK substitutes for all of them (reloc_backend_compat_shims.c,
 * battle_playable_compat_stubs.c) that build a 16-vertex untextured primitive
 * -- dust, star, ring or disc -- recoloured per effect. Thirteen effect kinds
 * shared four shapes, so a Coin looked like a Sparkle and a KO burst was a red
 * ring.
 *
 * Those substitutes existed for one reason: lbParticleMakeScriptID was a stub
 * and the particle bank was not resident, so the real scripts could not draw.
 * Both of those are false as of 2026-08-01 -- the imported interpreter owns the
 * constructor, the common and Dream Land banks are packed, and a five-minute
 * both-CPU soak drew 347,100 textured quads with zero atlas misses. The
 * stand-ins are now the only thing standing between the source scripts and the
 * screen, which is exactly what OPTIMIZATION_IDEAS.md means by "they are
 * stand-ins that should be deleted when the real particle scripts become
 * visible".
 *
 * A strong definition here overrides the weak one at link time, so this is a
 * deletion of the substitute rather than a second effect architecture.
 *
 * THE RISK IS COVERAGE, AND IT IS INSTRUMENTED. A particle whose texture is not
 * in the 8,192-byte atlas draws NOTHING rather than something wrong, so routing
 * an effect whose texture was never admitted trades a wrong-coloured shape for
 * an absent one. gNdsParticleQuadMissMask names the source ids that missed and
 * gNdsParticleQuadMissFrameMask the frames, so the admitted set is regraded
 * from a measured run rather than guessed. Read those two after any change
 * here. */

/*
 * MEASURED 2026-08-01: ROUTING ALL TWENTY AT ONCE DOES NOT FIT.
 * The first attempt forwarded every seam below and the ROM froze MID-MATCH on a
 * KO: `MALLOCOVF=1, id=65536, req=136, head=24`. The stack names the mechanism
 * exactly -- gcGetDObjSetNextAlloc (objman.c:692) <- gcAddDObjForGObj <-
 * gcSetupCustomDObjs <- efManagerMakeEffect(dEFManagerDeadExplodeEffectDesc) <-
 * ftCommonDeadDownSetStatus. The failing allocation is a 136-byte DObj, so the
 * cost is source effects building REAL DObj TREES out of gSYTaskmanGeneralHeap
 * while the match runs, where the stand-ins reused a handful of fixed
 * templates. The seventeen drained the heap and the KO burst's own DObj was the
 * straw. gSYTaskmanGeneralHeap had 26,876 bytes of battle-time headroom before
 * and 24 at the halt.
 *
 * (An earlier reading of this stack said "battle setup", from frames #25-#26
 * being syTaskmanLoadScene/syTaskmanStartTask. Those are the task that was
 * started, not a load in progress; #24 syTaskmanRunTask is the running task.
 * Read the middle of a stack before naming its phase.)
 *
 * So the set is graduated in measured groups rather than wholesale, smallest
 * blast radius first, with gNdsTaskmanGeneralHeapFreeMin read after each. The
 * KO group is first because it is what the owner named and because the
 * substitute for it was the furthest from source -- it discarded the `player`
 * argument entirely. Move a group above the #if only with a soak that keeps the
 * low-water above the 25,600 ifCommonSetMaxNumGObj latch.
 *
 */


/* The star KO. Source spawns efcommon script 0x5C directly. */
LBParticle *efManagerSparkleWhiteDeadMakeEffect(Vec3f *pos, f32 scale)
{
    return ndsBaseEFManagerSparkleWhiteDeadMakeEffect(pos, scale);
}

/* The KO burst. Source selects a player- and type-specific DeathExplode
 * generator with its own orientation, DObj/material animation and player
 * colours; the substitute discarded `player` entirely and scaled one red ring
 * by `type`. */
/* The KO burst, reimplemented here rather than forwarded, because the source
 * body walks its own DObj tree with no NULL guard anywhere:
 *
 *     dobj        = DObjGetStruct(effect_gobj);
 *     dobj->translate.vec.f = *pos;
 *     child_dobj  = dobj->child;
 *     sibling_dobj = dobj->child->sib_next->sib_next;
 *     sibling_dobj->mobj->sub.envcolor... ;
 *     child_dobj->mobj->sub.envcolor... ;
 *
 * That is five unchecked dereferences behind a tree the caller does not build
 * and cannot inspect, and efManagerMakeEffect has three separate ways to hand
 * back something shorter than it assumes: it returns the bare GObj early when
 * proc_display is NULL (no DObj at all, so DObjGetStruct is NULL and the very
 * next line faults), it builds the tree from *effect_desc->file_head so a
 * bank that did not load yields an empty or garbage tree, and every
 * gcAddChildForDObj/gcAddDObjForGObj inside it draws from the same
 * gcGetDObjSetNextAlloc pool that runs out under heap pressure. On N64 that
 * pool never ran out during a KO, so the missing guards never showed; here the
 * KO burst is a forced allocation -- efManagerMakeEffectForce bypasses the
 * five-entry reserve precisely so a KO always spawns -- which means it is the
 * one effect guaranteed to be built at the worst moment. Owner report,
 * 2026-08-01: "the KO burst freezes the game".
 *
 * Behaviour is identical to the source whenever the tree is complete, which is
 * the normal case. When it is not, the burst is skipped and
 * gNdsKOBurstDropMask names the link that was missing, so an incomplete tree
 * is a diagnosable missing cosmetic instead of a hang. */
GObj *efManagerDeadExplodeMakeEffect(Vec3f *pos, s32 player, u32 type)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    DObj *dobj;
    DObj *child_dobj;
    DObj *sibling_dobj;
    u8 index = (u8)(((type % 2) * GMCOMMON_PLAYERS_MAX) + player);

    u32 drop = 0u;

    gNdsKOBurstAttemptCount++;
    gNdsKOBurstStage = NDS_KO_BURST_STAGE_ENTER;

    gNdsKOBurstStage = NDS_KO_BURST_STAGE_PARTICLE;
    pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(1),
                                dEFManagerDeadExplodeGenID[index]);
    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                gNdsKOBurstDropMask |= NDS_KO_BURST_DROP_XF_UNUSED;
                return NULL;
            }
            xf->translate = *pos;
            xf->rotate.z = F_CLC_DTOR32(dEFManagerDeadExplodeRotateD[type]);
        }
        else
        {
            drop |= NDS_KO_BURST_DROP_XF;
            lbParticleEjectStruct(pc);
        }
    }
    else { drop |= NDS_KO_BURST_DROP_PARTICLE; }

    gNdsKOBurstStage = NDS_KO_BURST_STAGE_MATANIM;
    /* Reassigned per KO from an address-valued table, so it needs resolving
     * every time -- the once-per-scene sweep in efManagerInitEffects cannot
     * hold this field. */
    dEFManagerDeadExplodeEffectDesc.o_matanim_joint = ndsEFManagerResolveOffset(
        dEFManagerDeadExplodeMatAnimJoints[player]);

    gNdsKOBurstStage = NDS_KO_BURST_STAGE_MAKEFORCE;
    effect_gobj = efManagerMakeEffectForce(&dEFManagerDeadExplodeEffectDesc);

    if (effect_gobj == NULL)
    {
        gNdsKOBurstDropMask |= (drop | NDS_KO_BURST_DROP_GOBJ);
        return NULL;
    }
    gNdsKOBurstStage = NDS_KO_BURST_STAGE_TREE;
    /* Every link the source assumes, checked once. A short tree leaves the GObj
     * alive and harmless -- it simply has nothing to colour. */
    dobj = DObjGetStruct(effect_gobj);
    if (dobj == NULL)
    {
        gNdsKOBurstDropMask |= (drop | NDS_KO_BURST_DROP_ROOT_DOBJ);
        return effect_gobj;
    }
    dobj->translate.vec.f = *pos;
    dobj->rotate.vec.f.z = F_CLC_DTOR32(dEFManagerDeadExplodeRotateD[type]);

    child_dobj = dobj->child;
    if (child_dobj == NULL)
    {
        gNdsKOBurstDropMask |= (drop | NDS_KO_BURST_DROP_CHILD);
        return effect_gobj;
    }
    sibling_dobj = child_dobj->sib_next;
    sibling_dobj = (sibling_dobj != NULL) ? sibling_dobj->sib_next : NULL;
    if (sibling_dobj == NULL)
    {
        drop |= NDS_KO_BURST_DROP_SIBLING;
    }
    else if (sibling_dobj->mobj == NULL)
    {
        drop |= NDS_KO_BURST_DROP_SIBLING_MOBJ;
    }
    else
    {
        sibling_dobj->mobj->sub.envcolor.s.r = dEFManagerDeadExplodeEnvColorSiblingR[player];
        sibling_dobj->mobj->sub.envcolor.s.g = dEFManagerDeadExplodeEnvColorSiblingG[player];
        sibling_dobj->mobj->sub.envcolor.s.b = dEFManagerDeadExplodeEnvColorSiblingB[player];
        sibling_dobj->mobj->sub.flags |= MOBJ_FLAG_ENVCOLOR;
    }
    if (child_dobj->mobj == NULL)
    {
        drop |= NDS_KO_BURST_DROP_CHILD_MOBJ;
    }
    else
    {
        child_dobj->mobj->sub.envcolor.s.r = dEFManagerDeadExplodeEnvColorChildR[player];
        child_dobj->mobj->sub.envcolor.s.g = dEFManagerDeadExplodeEnvColorChildG[player];
        child_dobj->mobj->sub.envcolor.s.b = dEFManagerDeadExplodeEnvColorChildB[player];
        child_dobj->mobj->sub.flags |= MOBJ_FLAG_ENVCOLOR;
    }
    gNdsKOBurstStage = NDS_KO_BURST_STAGE_DONE;
    if (drop == 0u) { gNdsKOBurstCompleteCount++; }
    gNdsKOBurstDropMask |= drop;
    return effect_gobj;
}

/* THE PARTICLE-ONLY SPLIT WAS RIGHT AFTER ALL, and this is the second
 * correction -- the first one, which collapsed these eleven into
 * NDS_R2_SOURCE_EFFECTS_FULL, was reasoning from a freeze that has since been
 * attributed elsewhere.
 *
 * It argued that "a GObj pulls DObjs", so calling gcMakeGObjSPAfter was as
 * expensive as calling efManagerMakeEffect. Read the signature:
 * gcMakeGObjSPAfter(u32 id, void (*func_run)(GObj*), u8 link, u32 priority)
 * (objman.c:1724). The second argument is a run function. Every one of these
 * eleven passes NULL there and never calls gcAddDObjForGObj, so the tree they
 * build is empty and they draw entirely through lbParticleDrawTextures.
 *
 * The stand-in they displace is the expensive one:
 * ndsEFManagerMakeVisualEffect takes the same EFStruct AND a real
 * gcAddDObjForGObj, so every substituted hit spark costs 136 bytes of general
 * heap that gcGetDObjSetNextAlloc never gives back. Routing these to source
 * LOWERS gNdsGCDrawsActiveMax.
 *
 * What actually froze the ROM when all twenty were routed was
 * dEFManagerDeadExplodeEffectDesc's offsets holding symbol addresses, which
 * sent gcSetupCustomDObjs walking garbage and allocating a DObj per bogus node
 * -- see ndsEFManagerResolveDescOffsets above. The "gSYTaskmanGeneralHeap down
 * to 200 bytes" reading was that walk, not eleven particles. */
#if NDS_R2_SOURCE_EFFECTS_PARTICLE
LBParticle *efManagerDamageNormalLightMakeEffect(Vec3f *pos, s32 player,
                                                 s32 size, sb32 is_static)
{
    return ndsBaseEFManagerDamageNormalLightMakeEffect(pos, player, size,
                                                       is_static);
}
LBParticle *efManagerDamageNormalHeavyMakeEffect(Vec3f *pos, s32 player,
                                                 s32 size)
{
    return ndsBaseEFManagerDamageNormalHeavyMakeEffect(pos, player, size);
}
LBParticle *efManagerDamageFireMakeEffect(Vec3f *pos, s32 size)
{
    return ndsBaseEFManagerDamageFireMakeEffect(pos, size);
}
LBParticle *efManagerDamageElectricMakeEffect(Vec3f *pos, s32 size)
{
    return ndsBaseEFManagerDamageElectricMakeEffect(pos, size);
}
LBParticle *efManagerDamageCoinMakeEffect(Vec3f *pos)
{
    return ndsBaseEFManagerDamageCoinMakeEffect(pos);
}
LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index)
{
    return ndsBaseEFManagerDustExpandSmallMakeEffect(pos, f_index);
}
LBParticle *efManagerFireGrindMakeEffect(Vec3f *pos)
{
    return ndsBaseEFManagerFireGrindMakeEffect(pos);
}
LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos)
{
    return ndsBaseEFManagerSparkleWhiteMakeEffect(pos);
}
LBParticle *efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale)
{
    return ndsBaseEFManagerSparkleWhiteScaleMakeEffect(pos, scale);
}
LBParticle *efManagerFlashMiddleMakeEffect(Vec3f *pos)
{
    return ndsBaseEFManagerFlashMiddleMakeEffect(pos);
}
LBParticle *efManagerSetOffMakeEffect(Vec3f *pos, s32 size)
{
    return ndsBaseEFManagerSetOffMakeEffect(pos, size);
}
#endif /* NDS_R2_SOURCE_EFFECTS_PARTICLE */

/* DObj TREE, therefore priced against the nine-DObj margin. All six take
 * efManagerMakeEffectNoForce, so the EFStruct pool bounds them -- but the bound
 * is (pool depth x DObjs per tree), which is what exhausted the heap when all
 * twenty were routed at once. Graduate these as their own measured group.
 *
 * They have a second problem that heap cannot fix: their geometry goes out as
 * source effect DL links, which the battle hardware path does not submit, so
 * routing them today swaps a visible primitive for an invisible one. Same seam
 * as the respawn platform. */
#if NDS_R2_SOURCE_EFFECTS_FULL
GObj *efManagerDamageSlashMakeEffect(Vec3f *pos, s32 size, f32 rotate)
{
    return ndsBaseEFManagerDamageSlashMakeEffect(pos, size, rotate);
}
GObj *efManagerImpactWaveMakeEffect(Vec3f *pos, s32 index, f32 rotate)
{
    return ndsBaseEFManagerImpactWaveMakeEffect(pos, index, rotate);
}
GObj *efManagerCatchSwirlMakeEffect(Vec3f *pos)
{
    return ndsBaseEFManagerCatchSwirlMakeEffect(pos);
}
GObj *efManagerDamageSpawnOrbsRandomMakeEffect(Vec3f *pos)
{
    return ndsBaseEFManagerDamageSpawnOrbsRandomMakeEffect(pos);
}
GObj *efManagerDamageSpawnSparksRandomMakeEffect(Vec3f *pos, s32 lr)
{
    return ndsBaseEFManagerDamageSpawnSparksRandomMakeEffect(pos, lr);
}
GObj *efManagerDamageSpawnMDustRandomMakeEffect(Vec3f *pos, s32 lr)
{
    return ndsBaseEFManagerDamageSpawnMDustRandomMakeEffect(pos, lr);
}
#endif /* NDS_R2_SOURCE_EFFECTS_FULL */
