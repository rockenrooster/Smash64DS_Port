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
#include <nds/nds_firegrind.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_renderer.h>
#include <nds/nds_startup.h>
#include <nds/nds_task39_effect_census.h>
#include <nds/timers.h>
#include <sys/audio.h>
#include <string.h>
/* sqrtf, for the shield quad's guard scale. float only -- a double here would
 * be a defect on ARM9. */
#include <math.h>

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

/* P2-3 LinkBomb keeps LBParticle opaque outside the effect owner. BattleShip's
 * item source scales the explosion particle's transform directly; expose that
 * exact three-lane operation here rather than duplicating the private particle
 * layout in the item TU. */
void ndsEFManagerScaleParticle(LBParticle *pc, f32 scale)
{
    if ((pc != NULL) && (pc->xf != NULL))
    {
        pc->xf->scale.x = scale;
        pc->xf->scale.y = scale;
        pc->xf->scale.z = scale;
    }
}

void lbCommonDObjScaleXProcDisplay(GObj *gobj);
void gcDrawDObjDLHead1(GObj *gobj);
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
extern ftCommonYoshiEggDesc dFTCommonYoshiEggDamageCollDescs[];
void ftParamProcPauseEffect(GObj *effect_gobj);
void ftParamProcResumeEffect(GObj *fighter_gobj);
void gmCameraSetVelAt(Vec3f *move);

/* `nFTCaptainStatusSpecialAirLw` used to be stood up here as
 * `nFTCommonStatusSpecialStart + 13`, because `include/ft/fighter.h` carried
 * only a four-name Captain placeholder. It now carries the source's complete
 * nineteen-status window (P2-3f3), and `#ifndef` cannot see an ENUMERATOR, so
 * leaving this would quietly re-shadow the real enum with a macro that merely
 * happens to agree today. */

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
#if NDS_R2_FOX_BLASTER_GLOW_AOT
#define efManagerFoxBlasterGlowMakeEffect \
    ndsBaseEFManagerFoxBlasterGlowMakeEffect
#endif

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
#if NDS_R2_FOX_BLASTER_GLOW_AOT
#undef efManagerFoxBlasterGlowMakeEffect
#endif

#if NDS_R2_FOX_BLASTER_GLOW_AOT
/* EFCommon script 0x62 is a closed ten-tick flash. Its only four source
 * callers are wpfoxblaster.c's spawn/map/hit/hop sites, and all four discard
 * the return value. That makes this the owning seam for replacing the
 * LBParticle allocation + bytecode interpreter with the fixed native pool.
 * Any admission failure calls the renamed source body, preserving a real
 * LBParticle and the original renderer as the correctness fallback. */
LBParticle *efManagerFoxBlasterGlowMakeEffect(Vec3f *pos)
{
    if (ndsParticleSpawnFoxBlasterGlowAOT(pos) != FALSE)
    {
        return NULL;
    }
    return ndsBaseEFManagerFoxBlasterGlowMakeEffect(pos);
}
#endif

/* ROUTE THE THREE MODEL EFFECTS AT THEIR SOURCE MAKERS, WHICH IS THE HALF THE
 * FLAG NEVER DID.
 *
 * BUGS.md has carried "the Halo is not the correct asset", "still not using the
 * correct asset for Fox's down B" and a shield that "looks cut in half" for
 * several cycles, and every attempt at them so far has been an atlas tweak.
 * They are not sprites: each is an EFDesc whose geometry is a DObj tree with
 * joint animation (dEFManagerShieldEffectDesc -> llFTManagerCommonShieldDObjDesc,
 * dEFManagerRebirthHaloEffectDesc -> llEFCommonEffects3RebirthHaloDObjDesc,
 * dEFManagerFoxReflectorEffectDesc -> llFoxSpecial2ReflectorDObjDesc).
 *
 * The source makers for all three are compiled and sitting right here under
 * their ndsBase* names. What reaches them was a WEAK shim per effect
 * (reloc_backend_compat_shims.c:1728 for the shield,
 * battle_playable_compat_stubs.c:141 for the halo) that calls
 * ndsEFManagerMakeVisualEffect instead -- a procedurally built disc, recorded
 * in its own census as NDS_TASK39_EFFECT_SUBSTITUTE. So the model was never
 * made, let alone drawn, and no cell size or palette could have changed that.
 *
 * Routing alone was not enough, which is why this took cycles 50-59: the
 * renderer refused the models at the admission gate, at the submit guard and at
 * the accepted-kind list, then drew them at the origin, in the ground plane, at
 * 1/9th size and untextured. Those are all fixed and the models draw. The
 * owner priced the result at P95 +36,032 and took it on 2026-08-04, so this is
 * the only route now -- the weak stand-in shims these override are DELETED and
 * there is no second definition left for a strong symbol to beat. Kept as
 * one-line forwards because the decomp bodies compile under their ndsBase*
 * names (the #define/#undef pair above). */
GObj *efManagerShieldMakeEffect(GObj *fighter_gobj)
{
    return ndsBaseEFManagerShieldMakeEffect(fighter_gobj);
}

#if NDS_R2_REBIRTH_HALO_NATIVE && NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
/* EFCommonEffects3:0x2B7C is only a looping RotY track.  A live source trace
 * proves the visible states are exactly 0, k*(2pi/30) for k=1..29, then 0;
 * the script's setup gives the loop 31 update calls, not 30 visible values.
 * Store the IEEE-754 bits so ARM9 does no animation-script decode, float divide,
 * interpolation, or trig at runtime and still lands bit-for-bit on the source. */
static const u32 sNdsRebirthHaloRotYBits[31] = {
    0x00000000u,
    0x3e567750u, 0x3ed67750u, 0x3f20d97cu, 0x3f567750u,
    0x3f860a92u, 0x3fa0d97cu, 0x3fbba866u, 0x3fd67750u,
    0x3ff1463au, 0x40060a92u, 0x40137207u, 0x4020d97cu,
    0x402e40f1u, 0x403ba866u, 0x40490fdbu, 0x40567750u,
    0x4063dec5u, 0x4071463au, 0x407eadafu, 0x40860a92u,
    0x408cbe4cu, 0x40937207u, 0x409a25c2u, 0x40a0d97cu,
    0x40a78d36u, 0x40ae40f1u, 0x40b4f4acu, 0x40bba866u,
    0x40c25c20u,
    0x00000000u,
};

volatile u32 gNdsRebirthHaloAotUpdateCount;

static void ndsEFManagerRebirthHaloProcUpdateAOT(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *root = DObjGetStruct(effect_gobj);
    DObj *rotating;
    s32 phase;
    union
    {
        u32 bits;
        f32 value;
    } rot_y;

    if ((ep == NULL) || (root == NULL) || (root->child == NULL) ||
        (root->child->child == NULL))
    {
        return;
    }
    rotating = root->child->child;
    phase = ep->effect_vars.common.size;
    if ((u32)phase >= (sizeof(sNdsRebirthHaloRotYBits) /
                       sizeof(sNdsRebirthHaloRotYBits[0])))
    {
        phase = 0;
    }
    rot_y.bits = sNdsRebirthHaloRotYBits[phase];
    rotating->rotate.vec.f.y = rot_y.value;
    phase++;
    if ((u32)phase >= (sizeof(sNdsRebirthHaloRotYBits) /
                       sizeof(sNdsRebirthHaloRotYBits[0])))
    {
        phase = 0;
    }
    ep->effect_vars.common.size = phase;
    gNdsRebirthHaloAotUpdateCount++;
}
#endif

GObj *efManagerRebirthHaloMakeEffect(GObj *fighter_gobj, f32 size)
{
#if NDS_R2_REBIRTH_HALO_NATIVE && NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
    void (*source_update)(GObj *) = dEFManagerRebirthHaloEffectDesc.proc_update;
    intptr_t source_anim_joint = dEFManagerRebirthHaloEffectDesc.o_anim_joint;
    GObj *effect_gobj;
    EFStruct *ep;

    /* efManagerMakeEffect installs the descriptor's update function into the
     * GObj process while constructing it.  Temporarily point the descriptor at
     * the closed AOT updater and suppress creation of AObj interpreter state;
     * restore the source descriptor immediately so every non-native route stays
     * source-exact.  The scheduler is single-threaded here. */
    dEFManagerRebirthHaloEffectDesc.proc_update =
        ndsEFManagerRebirthHaloProcUpdateAOT;
    dEFManagerRebirthHaloEffectDesc.o_anim_joint = 0;
    effect_gobj = ndsBaseEFManagerRebirthHaloMakeEffect(fighter_gobj, size);
    dEFManagerRebirthHaloEffectDesc.proc_update = source_update;
    dEFManagerRebirthHaloEffectDesc.o_anim_joint = source_anim_joint;

    ep = (effect_gobj != NULL) ? efGetStruct(effect_gobj) : NULL;
    if (ep != NULL)
    {
        ep->effect_vars.common.size = 0;
    }
    return effect_gobj;
#else
    return ndsBaseEFManagerRebirthHaloMakeEffect(fighter_gobj, size);
#endif
}

#if NDS_R2_IMPACT_WAVE_NATIVE
volatile u32 gNdsImpactWaveNativeDrawCount;
volatile u32 gNdsImpactWaveNativeFallbackCount;
volatile u32 gNdsImpactWaveNativeTexturePrepareCount;
volatile u32 gNdsImpactWaveNativeTextureBindCount;

s32 ndsEFManagerImpactWaveVariant(GObj *effect_gobj, u32 *variant_out)
{
    EFStruct *ep;

    if ((effect_gobj == NULL) || (variant_out == NULL))
    {
        return FALSE;
    }
    ep = efGetStruct(effect_gobj);
    if ((ep == NULL) || (ep->proc_update != efManagerImpactWaveProcUpdate) ||
        ((u32)ep->effect_vars.impact_wave.index >= 5u))
    {
        return FALSE;
    }
    *variant_out = (u32)ep->effect_vars.impact_wave.index;
    return TRUE;
}

s32 ndsEFManagerIsImpactWaveGObj(GObj *effect_gobj)
{
    u32 variant;

    return ndsEFManagerImpactWaveVariant(effect_gobj, &variant);
}
#endif

#if NDS_R2_REBIRTH_HALO_NATIVE
volatile u32 gNdsRebirthHaloNativeDrawCount;
volatile u32 gNdsRebirthHaloNativeFallbackCount;
volatile u32 gNdsRebirthHaloNativeTexturePrepareCount;
volatile u32 gNdsRebirthHaloNativeTextureBindCount;
#endif

/* Seven, down from fourteen. The shield's five disc templates, the reflector's
 * and the respawn pad's went with the procedural stand-ins on 2026-08-04; what
 * remains serves the effect kinds the source path does NOT replace -- hit
 * sparks, flames, electric, sparkles/coins, the ripple/flash wave and the
 * generic death ring. */
#define NDS_VISUAL_TEMPLATE_COUNT 7
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
    nNDSVisualTemplateDeath
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

/* ndsEFManagerBuildDisc IS GONE (2026-08-04). It built the shield bubble, Fox's
 * reflector barrier and the respawn pad as a procedural glinted disc, and all
 * three now draw their source EFDesc models. Nothing else ever used it. */

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
    case nNDSVisualEffectDeath:
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

/* THE PROCEDURAL SHIELD AND RESPAWN-PAD DRAW PATHS ARE GONE (2026-08-04).
 * ndsEFManagerShieldProcDisplay, ndsEFManagerShieldTemplate,
 * ndsEFManagerShieldQuadColor, ndsEFManagerRebirthProcDisplay and the
 * NDS_TASK39_FX_SHIELD flag that gated the first three all existed to draw a
 * stand-in where a source EFDesc model belonged. Both models draw now --
 * dEFManagerShieldEffectDesc on DL link 15 and dEFManagerRebirthHaloEffectDesc
 * on link 10 -- so the stand-ins are deleted rather than left selectable.
 * Their atlas cells (NDS_PARTICLE_QUAD_SHIELD_TEXTURE,
 * NDS_PARTICLE_QUAD_REBIRTH_TEXTURE) are deliberately still packed: dropping
 * them would re-run the quad packer's admission and could admit a different
 * set of textures, which is a picture change nobody asked for. */

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
            joint = fp->joints[nFTPartsJointTopN];
            fp->is_effect_attach = TRUE;
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
    gcAddGObjDisplay(effect_gobj, gcDrawDObjTreeForGObj, 18, 2, -1);
    gNdsVisualEffectCreateCount++;
    gNdsVisualEffectActiveCount++;
    if (gNdsVisualEffectActiveCount > gNdsVisualEffectMaxActiveCount)
    {
        gNdsVisualEffectMaxActiveCount = gNdsVisualEffectActiveCount;
    }
    gNdsVisualEffectKindMask |= 1u << (u32)kind;
    return effect_gobj;
}

/* THE MATCH IS ep->fighter_gobj AND NOTHING ELSE, because that is what source
 * does. ftMainSetStatus calls ftParamProcStopEffect whenever the caller omits
 * FTSTATUS_PRESERVE_EFFECT (ftmain.c:4449); that runs ftParamRunProcEffect over
 * gGCCommonLinks[nGCCommonLinkIDEffect] and ejects EVERY effect whose
 * ep->fighter_gobj matches, never asking what kind of effect it is.
 *
 * This walk used to carry `&& ndsEFManagerIsVisualEffectGObj(...)` in the match,
 * and that predicate is true only when dobj->dl is one of the
 * sNdsVisualTemplates[] display lists -- only the PROCEDURAL stand-ins. A source
 * EFDesc effect carries the source model's display list, so it failed the test
 * and NOTHING IN THE BUILD EVER EJECTED IT. The filter was harmless while the
 * source path drew nothing; it became a leak the moment
 * NDS_R2_SOURCE_EFFECTS_FULL made those effects real. Measured before the fix:
 * three guards in one match left three shields drawing, link-15 draw count
 * 1 -> 2 -> 3 and never down, still +3/frame 494 tics after the first spawn.
 *
 * So the kind test demotes from a MATCH filter to a TEARDOWN discriminator: the
 * two kinds are allocated differently and must be released differently. Source
 * effects mirror ftParamStopEffect (ftparam.c) exactly. */
void ndsEFManagerStopAttachedEffects(GObj *fighter_gobj)
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

        if ((ep != NULL) && (ep->fighter_gobj == fighter_gobj))
        {
            if (ndsEFManagerIsVisualEffectGObj(effect_gobj) != FALSE)
            {
                ndsEFManagerDestroyVisualEffect(effect_gobj);
            }
            else
            {
                if (ep->xf != NULL)
                {
                    lbParticleEjectStructID(ep->xf->generator_id,
                                            ep->bank_id >> 3);
                }
                efManagerSetPrevStructAlloc(ep);
                gcEjectGObj(effect_gobj);
                gNdsEFManagerSourceEffectStopCount++;
            }
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
 * way to know. file_head is a pointer to a slot, not a file id, so every entry
 * here is a slot-address comparison and the table has to be written out.
 *
 * The three EF common files were the whole table until 2026-08-03, and the two
 * fighter files below are why that mattered: a desc whose file_head is not
 * listed gets span 0, and span 0 makes ndsEFManagerResolveDescOffsets return
 * before it validates anything at all. The shield (&gFTManagerCommonFile) and
 * Fox's reflector (&gFTDataFoxSpecial2) were both in that hole. Returning 0 has
 * to mean "no such file", not "a file I decline to check", or the fail-closed
 * path below is decoration. */
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
    if (file_head == &gFTManagerCommonFile)
    {
        return ndsRelocGetLoadedFileSize(&llFTManagerCommonFileID);
    }
    if (file_head == &gFTDataFoxSpecial2)
    {
        return ndsRelocGetLoadedFileSize(&llFoxSpecial2FileID);
    }
    if (file_head == &gFTMarioFileSpecial2)
    {
        return ndsRelocGetLoadedFileSize(&llMarioSpecial2FileID);
    }
    if (file_head == &gFTDataFoxSpecial3)
    {
        return ndsRelocGetLoadedFileSize(&llFoxSpecial3FileID);
    }
#if NDS_P2_DONKEY
    if (file_head == &gFTDataDonkeySpecial2)
    {
        /* BattleShip dEFManagerDonkeyEntryTaruEffectDesc owns DonkeySpecial2.
         * Keep the same deferred-file validation used by Mario's pipe and
         * Fox's Arwing/reflector; otherwise the source descriptor's linker
         * symbol addresses remain unresolved DS RAM tokens. */
        return ndsRelocGetLoadedFileSize(&llDonkeySpecial2FileID);
    }
#endif
#if NDS_P2_SAMUS
    if (file_head == &gFTDataSamusSpecial2)
    {
        /* BattleShip dEFManagerSamusEntryPointEffectDesc owns SamusSpecial2.
         * Like the already-landed entry descriptors, these offsets are
         * linker-symbol addresses until this resolver sees the real file
         * span, and the file is not resident when efManagerInitEffects first
         * walks the table. Charge Shot itself is a WPDesc backed by
         * SamusSpecial1, not an EFDesc. */
        return ndsRelocGetLoadedFileSize(&llSamusSpecial2FileID);
    }
#endif
#if NDS_P2_LINK
    if (file_head == &gFTDataLinkSpecial2)
    {
        /* BattleShip's Link entry wave, entry beam, and Spin Attack effect all
         * own LinkSpecial2. The file is intentionally loaded after the effect
         * table is initialized, so keep these descriptors on the same deferred
         * residency/recovery path as the other landed fighter effects. */
        return ndsRelocGetLoadedFileSize(&llLinkSpecial2FileID);
    }
#endif
#if NDS_P2_PIKACHU
    if (file_head == &gFTDataPikachuSpecial3)
    {
        /* dEFManagerThunderJoltEffectDesc owns PikachuSpecial3 (the same
         * file the grounded Thunder Jolt weapon draws from). Same deferred
         * residency/recovery path as the other landed fighter effects. */
        return ndsRelocGetLoadedFileSize(&llPikachuSpecial3FileID);
    }
    if (file_head == &gFTDataPikachuSpecial2)
    {
        /* dEFManagerPikachuThunderShockEffectDesc owns PikachuSpecial2; it is
         * the shock burst ftcommonattacks4.c spawns from his up smash. */
        return ndsRelocGetLoadedFileSize(&llPikachuSpecial2FileID);
    }
    if (file_head == &gFTDataPikachuModel)
    {
        /* dEFManagerPikachuThunderTrailEffectDesc draws Thunder's bolt segments
         * straight out of PikachuModel (llPikachuModelThunderTrail*). */
        return ndsRelocGetLoadedFileSize(&llPikachuModelFileID);
    }
#endif
#if NDS_P2_YOSHI
    if (file_head == &gFTDataYoshiSpecial2)
    {
        /* dEFManagerYoshiEntryEggEffectDesc: the entry egg his Appear cracks
         * out of (efmanager.c:1312). */
        return ndsRelocGetLoadedFileSize(&llYoshiSpecial2FileID);
    }
    if (file_head == &gFTDataYoshiSpecial3)
    {
        /* dEFManagerYoshiEggLayEffectDesc: the egg the victim of Egg Lay
         * wears, with its wait/break/throw anim joints (efmanager.c:1345). */
        return ndsRelocGetLoadedFileSize(&llYoshiSpecial3FileID);
    }
    if (file_head == &gFTDataYoshiModel)
    {
        /* dEFManagerYoshiEggEscapeEffectDesc draws the escape burst straight
         * out of YoshiModel (efmanager.c:1375). */
        return ndsRelocGetLoadedFileSize(&llYoshiModelFileID);
    }
#endif
#if NDS_P2_PURIN
    if (file_head == &gFTDataPurinSpecial2)
    {
        /* dEFManagerPurinSingEffectDesc: Sing's note ring (efmanager.c:824). */
        return ndsRelocGetLoadedFileSize(&llPurinSpecial2FileID);
    }
#endif
#if NDS_P2_KIRBY
    if (file_head == &gFTDataKirbySpecial2)
    {
        /* Vulcan Jab, the four Final Cutter descs and the entry star (efmanager.c:704,896..986,1226). */
        return ndsRelocGetLoadedFileSize(&llKirbySpecial2FileID);
    }
#endif
#if NDS_P2_CAPTAIN
    if (file_head == &gFTDataCaptainSpecial2)
    {
        /* dEFManagerCaptainEntryCarEffectDesc (efmanager.c:1495) and
         * dEFManagerCaptainFalconKickEffectDesc (:760) both own
         * CaptainSpecial2 -- the Falcon Flyer AND the Falcon Kick effect are
         * in that one file. */
        return ndsRelocGetLoadedFileSize(&llCaptainSpecial2FileID);
    }
    if (file_head == &gFTDataCaptainSpecial3)
    {
        /* dEFManagerCaptainFalconPunchEffectDesc (efmanager.c:790). */
        return ndsRelocGetLoadedFileSize(&llCaptainSpecial3FileID);
    }
#endif
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
/* Descs whose file was not resident at efManagerInitEffects time.
 *
 * THE TABLE MUST HOLD EVERY DESC THE RESOLVER CAN HAND IT. A desc it cannot
 * remember is a desc it can never re-enable, and the paragraph above is then
 * false in the one way that matters: `efManagerMakeEffect` answers a
 * permanently-neutralised desc with a GObj carrying NO DObj tree, and the
 * source makers do NOT all guard that. `efManagerCaptainEntryCarMakeEffect`
 * walks `DObjGetStruct(effect_gobj)->child->child->child` (efmanager.c:5639)
 * on the very next line, so a lost slot is a data abort, not a missing effect.
 *
 * Four slots covered the closed Mario/Fox set with one spare. With Donkey and
 * Captain landed the resolver visits SEVEN deferrable descs -- Mario's pipe,
 * Fox's Arwing and reflector, DK's barrel, and Falcon's Flyer/Kick/Punch --
 * so the last three overflowed and stayed disabled for the whole match. Board
 * row P2-3f10 measured exactly that at Falcon's entry: `EFDESC disabled=7
 * unknownfile=7 recover=2 overflow=3`, `CARDESC proc=(nil)`, then an
 * ABORT-mode (`cpsr` 0x…97) data abort on `dobj->child`. The size is asserted
 * against the two desc lists below, so fighter #6 cannot re-open this by
 * adding a desc and forgetting a number. Overflow stays counted rather than
 * silently dropped, because "the retry table was full" and "the file never
 * loaded" are different failures and must not read alike. */
/* 24 covered the roster through Falcon; Link's three descs, Pikachu's four
 * and Yoshi's three move it to 34. The static assert beside
 * NDS_EF_ROSTER_DESCS is the guard. */
#define NDS_EF_DEFERRED_MAX 41u
static EFDesc *sNdsEFDeferredDescs[NDS_EF_DEFERRED_MAX];
static void (*sNdsEFDeferredProcs[NDS_EF_DEFERRED_MAX])(GObj *);
static u32 sNdsEFDeferredCount;

static void ndsEFManagerResetDeferredDescs(void)
{
    u32 i;

    /* EFDesc globals outlive the taskman arena. If a previous match never
     * loaded the fighter file for one deferred desc, that desc still has its
     * display callback cleared here. Restore the source definition before the
     * new match sweeps residency again; otherwise "Mario-only match, then Fox"
     * permanently disables Fox's entry effect. Recovered entries are already
     * restored and their table slot is NULL, so this is idempotent. */
    for (i = 0u; i < sNdsEFDeferredCount; i++)
    {
        if ((sNdsEFDeferredDescs[i] != NULL) &&
            (sNdsEFDeferredProcs[i] != NULL))
        {
            sNdsEFDeferredDescs[i]->proc_display = sNdsEFDeferredProcs[i];
        }
        sNdsEFDeferredDescs[i] = NULL;
        sNdsEFDeferredProcs[i] = NULL;
    }
    sNdsEFDeferredCount = 0u;
}

static void ndsEFManagerDeferDesc(EFDesc *desc)
{
    u32 i;

    for (i = 0u; i < sNdsEFDeferredCount; i++)
    {
        if (sNdsEFDeferredDescs[i] == desc)
        {
            return;
        }
    }
    if (sNdsEFDeferredCount >= NDS_EF_DEFERRED_MAX)
    {
        gNdsEFDescDeferOverflowCount++;
        return;
    }
    sNdsEFDeferredDescs[sNdsEFDeferredCount] = desc;
    sNdsEFDeferredProcs[sNdsEFDeferredCount] = desc->proc_display;
    sNdsEFDeferredCount++;
}

/* Call before making an effect whose file loads after startup. Restores
 * proc_display once the span is real and the slot is populated, and applies the
 * same bounds check the startup path would have. Idempotent and cheap: it walks
 * at most four entries and clears each one the first time it succeeds, so it is
 * not a per-effect cost after the file arrives. */
void ndsEFManagerRetryDeferredDescs(void)
{
    u32 i;

    for (i = 0u; i < sNdsEFDeferredCount; i++)
    {
        EFDesc *desc = sNdsEFDeferredDescs[i];
        size_t span;

        if (desc == NULL)
        {
            continue;
        }
        span = ndsEFManagerFileSpan(desc->file_head);
        if ((span == 0u) || (*desc->file_head == NULL))
        {
            continue;
        }
        sNdsEFDeferredDescs[i] = NULL;
        if (((size_t)desc->o_dobjsetup >= span) ||
            ((size_t)desc->o_mobjsub >= span) ||
            ((size_t)desc->o_anim_joint >= span) ||
            ((size_t)desc->o_matanim_joint >= span))
        {
            continue;
        }
        desc->proc_display = sNdsEFDeferredProcs[i];
        gNdsEFDescDeferRecoverCount++;
    }
}

static void ndsEFManagerResolveDescOffsets(EFDesc *desc)
{
    size_t span;

    desc->o_dobjsetup     = ndsEFManagerResolveOffset(desc->o_dobjsetup);
    desc->o_mobjsub       = ndsEFManagerResolveOffset(desc->o_mobjsub);
    desc->o_anim_joint    = ndsEFManagerResolveOffset(desc->o_anim_joint);
    desc->o_matanim_joint = ndsEFManagerResolveOffset(desc->o_matanim_joint);

    if (desc->proc_display == NULL) { return; }

    span = ndsEFManagerFileSpan(desc->file_head);
    if (span == 0u)
    {
        /* No span means the table above does not know this desc's file, so the
         * offset checks below would be theatre. The NULL test is NOT theatre:
         * an empty file slot makes efManagerMakeEffect compute 0 + offset and
         * hand gcSetupCustomDObjs a DObjDesc in low memory, which is fatal
         * whatever the span is, and this early return used to skip it.
         *
         * Counted rather than silent. Before 2026-08-03 this branch swallowed
         * the shield and Fox's reflector without a trace, and "unvalidated"
         * read exactly like "validated and fine". It must read 0 for the P1
         * desc set now that the table covers both fighter files. */
        gNdsEFDescUnknownFileCount++;
        gNdsEFDescUnknownFileLast = (u32)(uintptr_t)desc;
        if (*desc->file_head == NULL)
        {
            /* DEFERRED, NOT DEAD. efManagerInitEffects resolves every desc once
             * at startup, but gFTDataFoxSpecial2 is a Fox special-move file that
             * is not resident yet at that point -- so the reflector was being
             * disabled for a file that simply had not loaded, and stayed
             * disabled forever because this function returns early once
             * proc_display is NULL and can never re-enable it. Measured
             * 2026-08-03: EFDESC disabled=1 unknownfile=1 naming
             * dEFManagerFoxReflectorEffectDesc.
             *
             * The OFFSETS above are already correct -- resolving them does not
             * depend on residency -- so all that is owed is the span check,
             * retried once the file arrives. Remember the proc so the retry has
             * something to restore. */
            ndsEFManagerDeferDesc(desc);
            desc->proc_display = NULL;
            gNdsEFDescDisabledCount++;
            gNdsEFDescDisabledLast = (u32)(uintptr_t)desc;
        }
        return;
    }

    if ((*desc->file_head == NULL) ||
        ((size_t)desc->o_dobjsetup >= span) ||
        ((size_t)desc->o_mobjsub >= span) ||
        ((size_t)desc->o_anim_joint >= span) ||
        ((size_t)desc->o_matanim_joint >= span))
    {
        desc->proc_display = NULL;
        gNdsEFDescDisabledCount++;
        gNdsEFDescDisabledLast = (u32)(uintptr_t)desc;
    }
}

/* The EFDescs reachable in the P1 milestone -- Mario, Fox and the shared
 * combat effects. Listed rather than swept, because they are separate globals
 * with no guaranteed layout, and because a desc left out is a heap-exhaustion
 * hang rather than a missing effect: too severe to leave to a pattern match.
 *
 * Deliberately NOT the full set of fifty.
 * Naming a desc here KEEPS IT LINKED, and this list is charged in boot arena,
 * measured twice on 2026-08-01:
 *
 *   all 50 descs   arena 1,265,664  battle heap low-water 21,784  (cap latched at 47)
 *   16 descs       arena 1,269,760  battle heap low-water 23,720  (still under)
 *   2 descs        arena 1,273,856  battle heap low-water 26,876  (clear)
 *
 * The latch is ifCommonSetMaxNumGObj at 25,600 free, so the first two both
 * capped the GObj pool for the whole match -- a correctness fix that shrinks
 * the arena is a regression with extra steps. Sixteen descs is what the
 * shipping default now carries, since the gate-6 flip made every entry below
 * unconditional; the 2026-08-04 publish soak is the reading that covers it.
 *
 * dEFManagerMBallThrown/CaptureKirbyStar/LoseKirbyStar are excluded for a
 * second reason: their file_head is &gITManagerCommonData, which this ROM does
 * not link, so naming them is a link error rather than a fix. */
/* THE RESPAWN PLATFORM AND FOX'S REFLECTOR are the last two entries below, and
 * their absence from this list until 2026-08-03 IS those two rows.
 *
 * Measured on the flag-on ROM with no rebuild:
 * dEFManagerRebirthHaloEffectDesc.o_dobjsetup read 0x20E8610 -- still the
 * generated symbol's ADDRESS, never dereferenced to the offset it holds --
 * while dEFManagerImpactWaveEffectDesc, which IS listed, read 0x7C28 and the
 * shield read 0x300. Only a desc named here is ever passed to
 * ndsEFManagerResolveDescOffsets, so an unlisted one keeps a RAM address in a
 * field efManagerMakeEffect uses as a byte offset.
 *
 * It then computes *file_head + 0x20E8610 and hands gcSetupCustomDObjs a
 * DObjDesc thirty-four megabytes past the file, and every symptom in the
 * BUGS.md banner falls out of that one value: the tree is one node (measured
 * child=NULL sib=NULL, which is the "nodes=6 over 6 frames" the banner records
 * as a separate defect -- it is not separate), dobj->dl is whatever word was
 * there (measured 0x14006 and 0x3F800000, the second being 1.0f, a Vec3f
 * component, because DObjDesc is {id, dl, translate, rotate, scale}), and
 * ndsRendererAdapterSubmitStageDL returns at renderer_dl.c:8571 because that
 * value is inside no loaded file and no arena. That silent return is the only
 * exit leaving tris, texready and texreject all 0 with no stat of any kind,
 * which is exactly what three cycles chased through the atlas, the camera and
 * the tree walk. */
#define NDS_EF_MANAGER_DESCS(X) \
    X(dEFManagerDeadExplodeEffectDesc) \
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
    X(dEFManagerStarRodSparkEffectDesc) \
    X(dEFManagerShieldEffectDesc) \
    X(dEFManagerCatchSwirlEffectDesc) \
    /* itMainSetFighterHold always spawns BattleShip's item-pickup swirl. Its
     * EFCommonEffects3 descriptor carries four &ll... linker symbols in fields
     * efManagerMakeEffect treats as byte offsets. Leaving it out of this resolver
     * therefore makes the LinkBomb hold path walk DS RAM as a DObjDesc tree. */ \
    X(dEFManagerItemGetSwirlEffectDesc) \
    X(dEFManagerReflectBreakEffectDesc) \
    X(dEFManagerMarioEntryDokanEffectDesc) \
    X(dEFManagerFoxEntryArwingEffectDesc) \
    X(dEFManagerRebirthHaloEffectDesc) \
    X(dEFManagerFoxReflectorEffectDesc)

/* THE PER-ROSTER DESCS, as a list rather than a run of hand-written calls, so
 * the deferral table's size assertion below counts them. A fighter landing an
 * effect desc adds one line here and nothing else -- which is what P2-3f10
 * needed and did not have: Falcon's three descs were resolved by name, so
 * nothing tied them to NDS_EF_DEFERRED_MAX and the table silently overflowed. */
#if NDS_P2_DONKEY
/* P2-3 admits DK's source barrel entry effect at the same seam as the
 * already-qualified Mario/Fox entry descriptors.  The decomp initializer
 * stores &llDonkeySpecial2* linker symbols in offset fields, so this must
 * run before efManagerMakeEffect performs its source `base + offset` math. */
#define NDS_EF_ROSTER_DESCS_DONKEY(X) \
    X(dEFManagerDonkeyEntryTaruEffectDesc)
#else
#define NDS_EF_ROSTER_DESCS_DONKEY(X)
#endif
#if NDS_P2_SAMUS
/* SamusSpecial2 backs her source entry point and grapple beam. Keep both in
 * the same resolve/defer contract as the already-landed Mario/Fox/DK/Falcon
 * fighter-file effects. Charge Shot is a weapon owner, so it does not belong
 * in this EFDesc list. */
#define NDS_EF_ROSTER_DESCS_SAMUS(X) \
    X(dEFManagerSamusEntryPointEffectDesc) \
    X(dEFManagerSamusGrappleBeamEffectDesc)
#else
#define NDS_EF_ROSTER_DESCS_SAMUS(X)
#endif
#if NDS_P2_CAPTAIN
/* Falcon's three source descriptors carry &llCaptainSpecial2/3* linker symbols
 * in their offset fields, the same as DK's barrel above, so they need the same
 * resolve before efManagerMakeEffect does `base + offset`. The Flyer is the
 * entry effect; the Kick and Punch descs are reached from
 * ftcaptainspeciallw.c / ftcaptainspecialn.c. */
#define NDS_EF_ROSTER_DESCS_CAPTAIN(X) \
    X(dEFManagerCaptainEntryCarEffectDesc) \
    X(dEFManagerCaptainFalconKickEffectDesc) \
    X(dEFManagerCaptainFalconPunchEffectDesc)
#else
#define NDS_EF_ROSTER_DESCS_CAPTAIN(X)
#endif
#if NDS_P2_LINK
#define NDS_EF_ROSTER_DESCS_LINK(X) \
    X(dEFManagerLinkEntryWaveEffectDesc) \
    X(dEFManagerLinkEntryBeamEffectDesc) \
    X(dEFManagerLinkSpinAttackEffectDesc)
#else
#define NDS_EF_ROSTER_DESCS_LINK(X)
#endif
#if NDS_P2_PIKACHU
/* Pikachu's three fighter-file descs carry &llPikachu* linker symbols in their
 * offset fields exactly like DK's barrel, so they take the same resolve. The
 * Master Ball rays his entry spawns on flag1 live in EFCommonEffects3 and were
 * never listed because no landed fighter reached them; the ball itself
 * (dEFManagerMBallThrownEffectDesc) owns &gITManagerCommonData, which this
 * ROM does not link -- see the entry seam in battleship_ftcommon_entry.c. */
#define NDS_EF_ROSTER_DESCS_PIKACHU(X) \
    X(dEFManagerThunderJoltEffectDesc) \
    X(dEFManagerPikachuThunderTrailEffectDesc) \
    X(dEFManagerPikachuThunderShockEffectDesc) \
    X(dEFManagerMBallRaysEffectDesc)
#else
#define NDS_EF_ROSTER_DESCS_PIKACHU(X)
#endif
#if NDS_P2_YOSHI
/* The source shield wrapper also creates an EFDesc tree. Resolve its model
 * offset alongside Yoshi's entry and Egg Lay effects. */
#define NDS_EF_ROSTER_DESCS_YOSHI(X) \
    X(dEFManagerYoshiShieldEffectDesc) \
    X(dEFManagerYoshiEntryEggEffectDesc) \
    X(dEFManagerYoshiEggLayEffectDesc) \
    X(dEFManagerYoshiEggEscapeEffectDesc)
#else
#define NDS_EF_ROSTER_DESCS_YOSHI(X)
#endif
#if NDS_P2_PURIN
/* Purin's fighter-file descs carry &llPurin* linker symbols (admit_fighter.py). */
#define NDS_EF_ROSTER_DESCS_PURIN(X) \
    X(dEFManagerPurinSingEffectDesc)
#else
#define NDS_EF_ROSTER_DESCS_PURIN(X)
#endif
#if NDS_P2_NESS
/* Ness's five fighter-file descs carry &llNess* linker symbols, the same shape
 * as Yoshi's and Pikachu's above (decomp efmanager.c:1012-1160). He was the
 * only landed-or-pending kind with no block here at all, so his specials
 * resolved nothing: PSI Magnet, PK Thunder's trail and wave, the reflected
 * trail and PK Flash all reach efManagerMakeEffect* in the source (:4958,
 * :5035, :5085, :5111, :5138) and had no desc for the resolver to visit.
 * Found by the P2-3 readiness sweep, 2026-09-04.
 *
 * NDS_EF_DEFERRED_MAX (41) is asserted against the total desc count below.
 * These five are gated off while NDS_P2_NESS is 0; if enabling him trips that
 * assert, RAISE THE CAP rather than trimming the list -- every desc the
 * resolver can visit has to fit, which is what the assert exists to say. */
#define NDS_EF_ROSTER_DESCS_NESS(X) \
    X(dEFManagerNessPsychicMagnetEffectDesc) \
    X(dEFManagerNessPKThunderTrailEffectDesc) \
    X(dEFManagerNessPKReflectTrailEffectDesc) \
    X(dEFManagerNessPKThunderWaveEffectDesc) \
    X(dEFManagerNessPKFlashEffectDesc)
#else
#define NDS_EF_ROSTER_DESCS_NESS(X)
#endif
#if NDS_P2_KIRBY
/* Kirby's fighter-file descs carry &llKirby* linker symbols (admit_fighter.py). */
#define NDS_EF_ROSTER_DESCS_KIRBY(X) \
    X(dEFManagerVulcanJabEffectDesc) \
    X(dEFManagerKirbyCutterUpEffectDesc) \
    X(dEFManagerKirbyCutterDownEffectDesc) \
    X(dEFManagerKirbyCutterDrawEffectDesc) \
    X(dEFManagerKirbyCutterTrailEffectDesc) \
    X(dEFManagerKirbyEntryStarEffectDesc)
#else
#define NDS_EF_ROSTER_DESCS_KIRBY(X)
#endif
#define NDS_EF_ROSTER_DESCS(X) \
    NDS_EF_ROSTER_DESCS_DONKEY(X) \
    NDS_EF_ROSTER_DESCS_SAMUS(X) \
    NDS_EF_ROSTER_DESCS_CAPTAIN(X) \
    NDS_EF_ROSTER_DESCS_LINK(X) \
    NDS_EF_ROSTER_DESCS_PIKACHU(X) \
    NDS_EF_ROSTER_DESCS_YOSHI(X) \
    NDS_EF_ROSTER_DESCS_PURIN(X) \
    NDS_EF_ROSTER_DESCS_NESS(X) \
    NDS_EF_ROSTER_DESCS_KIRBY(X)

/* Every desc the resolver visits can reach ndsEFManagerDeferDesc, so the table
 * has to be at least this big. Asserted here rather than derived at the table,
 * because NDS_EF_DEFERRED_MAX sizes an array declared above these lists. */
#define NDS_EF_DESC_COUNT_ONE(name) + 1u
_Static_assert((0u NDS_EF_MANAGER_DESCS(NDS_EF_DESC_COUNT_ONE)
                   NDS_EF_ROSTER_DESCS(NDS_EF_DESC_COUNT_ONE)) <=
                   NDS_EF_DEFERRED_MAX,
               "NDS_EF_DEFERRED_MAX must cover every desc "
               "ndsEFManagerResolveAllDescOffsets visits");

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

#if NDS_P2_CAPTAIN
    /* Falcon Punch is the one source effect still pointing at the shared
     * lbCommonDObjScaleXProcDisplay compatibility stub, which is intentionally
     * a no-op because weapon users of that symbol have their own DS owner.
     *
     * BattleShip's punch descriptor is a SINGLE DObj (no 0x4 tree flag), and
     * lbCommonDObjScaleXProcDisplay therefore reduces to: reset gGCScaleX,
     * prepare this DObj's 0x50 joint-translation + RotRpyR matrices, then submit
     * its MObj/DL through display-list head 1. The DS renderer already implements
     * the exact 0x50 attachment transform, so route just this descriptor through
     * the existing DLHEAD1 capture seam rather than making the global bridge
     * draw every unrelated caller. Set it before deferred-desc resolution so a
     * late CaptainSpecial3 load remembers/restores this DS-equivalent callback. */
    dEFManagerCaptainFalconPunchEffectDesc.proc_display = gcDrawDObjDLHead1;
#endif

#define NDS_EF_RESOLVE_ONE(name) ndsEFManagerResolveDescOffsets(&name);
    NDS_EF_MANAGER_DESCS(NDS_EF_RESOLVE_ONE)
    NDS_EF_ROSTER_DESCS(NDS_EF_RESOLVE_ONE)
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

#if NDS_R2_SHIELD_QUAD
/* THE SHIELD AS A CAMERA-FACING QUAD. An experiment, DEFAULT OFF.
 *
 * Owner, 2026-08-06: "the shield is always camera facing too, so it behaves
 * like a billboard". The source asset agrees and says it more precisely --
 * dFTManagerCommon_Shield's one drawing node is a 21-command DL over exactly
 * FOUR vertices carrying an IA8 16x32 texture. Four vertices is a quad, so the
 * PICTURE is a textured quad on either route. What differs is the SUBMIT:
 * interpreting 21 display-list commands at ~626 ticks each, against one direct
 * camera-facing quad the particle pass already knows how to emit.
 *
 * THIS IS NOT A REVERT OF 2026-08-04. That change routed the shield to its
 * source EFDesc model and the owner priced it deliberately -- "36k p95 is worth
 * it for correctness". The model remains the default and this flag is off
 * unless a build asks for it. Two things make the trade worth re-TESTING rather
 * than re-litigating: the owner's observation above, and the fact that the 36k
 * was measured on a 128-frame window that the whole-match instrument later
 * showed reads the cheapest ~6% of a match, so the real price is unknown.
 *
 * The tree draw stays as the fallback on any quad refusal -- no atlas, texture
 * not seated, degenerate camera. That sentence is inherited from the path this
 * revives and it still holds: a shield that silently stops drawing is worse
 * than one drawn plainly. */
/* WORLD HALF-EXTENT PER UNIT OF GUARD SCALE, AND IT IS THE SOURCE'S OWN
 * NUMBER -- not a tuned one. dFTManagerCommon_gap_0x0000_sub_0x208, the
 * shield's four vertices, are at x/y = +-30 with z 0 (decoded 2026-08-06), and
 * the child DObj's matrix kind 0x2C is the billboard that multiplies them by
 * gGCScaleX, the guard scale. So the bubble is 30 * guard_scale, which for Fox
 * at full health is 30 * 9.3333 = 280 -- the same 280 as attr->shield_size,
 * which is the arithmetic checking itself.
 *
 * The 180 that stood here was multiplied by the ROOT DObj's scale, which is
 * always 1, so it asked for a flat 180 and sized the bubble that sat at the
 * world origin. Damage shrink now comes along for free: the source scales this
 * joint by ((0.65 * shield_health/55) + 0.35), which a constant could not
 * express at all. */
#define NDS_R2_SHIELD_QUAD_SIZE 30.0F
/* THE SOURCE'S PRIM ALPHA, because the combiner's alpha term is exactly
 * `TEXEL0_alpha * PRIMITIVE_alpha` and nothing else (SETCOMBINE FC309661
 * 552EFF7F, decoded 2026-08-06). The texel's alpha nibble is a hard 15 across
 * the whole disc with a two-texel margin, so the source bubble is a UNIFORM
 * 0xC0 -- 75% -- and the generator now hands over that same flat coverage
 * instead of a fold.
 *
 * This briefly sat at 0xFF to fight the fold's attenuation. That was treating a
 * symptom: with coverage no longer multiplied by intensity there is nothing to
 * compensate, and 0xFF would simply be more opaque than the N64. */
#define NDS_R2_SHIELD_QUAD_ALPHA 0xC0u
/* WORLD UNITS TOWARD THE CAMERA. The bubble sits ON the fighter's YRotN joint,
 * so its depth and the fighter's body interleave and the opaque body wins
 * wherever it is nearer -- the shield reads as being inside Mario rather than
 * around him. There is no draw-on-top flag to reach for (see the depth_bias
 * note on ndsParticleDrawSourceAssetQuad), so it is pulled forward instead.
 *
 * 150 is chosen against the thing it has to clear, not tuned: the fighter's
 * body is a few tens of units deep either side of that joint, and the bubble's
 * own full-health radius is 280, so 150 clears the torso while staying well
 * inside the bubble's own extent -- it cannot detach and float. The owner
 * judges it; this is presentation, not mechanics. */
#define NDS_R2_SHIELD_QUAD_DEPTH_BIAS 150.0F
/* BGR555, NOT 0xRRGGBB. ndsRendererSubmitParticleQuad takes the DS's own vertex
 * colour packing -- red bits 0-4, green 5-9, blue 10-14. This function first
 * used 0xRRGGBB constants, and 0xff0000 read through that packing is r=0 g=0
 * b=0: the owner reported the shield as "Looks Black in color", which is
 * exactly what it was. A wrong-unit colour does not look wrong, it looks like a
 * different bug. */
#define NDS_R2_BGR555(r, g, b) \
    ((((u32)(r) >> 3) & 31u) | ((((u32)(g) >> 3) & 31u) << 5) | \
     ((((u32)(b) >> 3) & 31u) << 10))

/* Engagement pair. Draw counting up with Fallback at 0 is the quad route
 * working; Fallback climbing means the quad is being refused every frame and
 * the tree is carrying the picture, which looks identical and costs MORE than
 * the default -- so a measurement taken without reading these would be
 * meaningless. */
volatile u32 gNdsShieldQuadDrawCount;
volatile u32 gNdsShieldQuadFallbackCount;

/* ONE GL_RGB8_A5 TEXTURE PER SHIELDING PLAYER, built on first use and kept for
 * the match. The texels are player-independent (generated, see
 * NDS_SHIELD_A5I3_*); only the eight-entry palette is not, which is the whole
 * reason the shield left the shared quad sheet -- that sheet's cells are white
 * and take their colour from one vertex colour, and this asset needs a colour
 * that varies per texel.
 *
 * Released in efManagerInitEffects rather than leaked: START at the results
 * screen restarts the match, so a name allocated per match and never given back
 * would accumulate across restarts. */
static u32 sNdsShieldTextureName[5];

static s32 ndsEFManagerShieldTexelFill(u8 *pixels, u32 bytes, void *user_data)
{
    (void)user_data;
    if ((pixels == NULL) || (bytes < NDS_SHIELD_TEX_BYTES))
    {
        return FALSE;
    }
    memcpy(pixels, gNdsShieldTexels, NDS_SHIELD_TEX_BYTES);
    return TRUE;
}

static u32 ndsEFManagerShieldTexture(s32 player)
{
    /* NOTHING IS COMPUTED HERE. gNdsShieldPalettes is the combiner's own ramp
     * per dEFManagerShieldColors entry, evaluated by the generator out of that
     * array in source, and gNdsShieldTexels is already in the hardware's A3I5
     * layout -- so preparing the shield is an upload and a palette pointer,
     * with no per-texel or per-entry work on the ARM9 at all. */
    const u16 *palette;
    u32 slot = ((u32)player < NDS_SHIELD_PALETTE_COUNT)
        ? (u32)player : (NDS_SHIELD_PALETTE_COUNT - 1u);

    if (sNdsShieldTextureName[slot] != 0u)
    {
        return sNdsShieldTextureName[slot];
    }
    palette = gNdsShieldPalettes[slot];
    /* Same upload either way; only the format's palette width differs, and the
     * generator emits the entry count that matches the texels it packed. */
#if NDS_SHIELD_TEX_A3I5
    if (ndsRendererHardwarePrepareIFCommonA3I5Atlas(
            NDS_SHIELD_TEX_WIDTH, NDS_SHIELD_TEX_HEIGHT, palette,
            ndsEFManagerShieldTexelFill, NULL,
            &sNdsShieldTextureName[slot]) == FALSE)
#else
    if (ndsRendererHardwarePrepareIFCommonCloudAtlas(
            NDS_SHIELD_TEX_WIDTH, NDS_SHIELD_TEX_HEIGHT, palette,
            ndsEFManagerShieldTexelFill, NULL,
            &sNdsShieldTextureName[slot]) == FALSE)
#endif
    {
        sNdsShieldTextureName[slot] = 0u;
    }
    return sNdsShieldTextureName[slot];
}

/* THE ROOT DObj CARRIES NEITHER THE POSITION NOR THE SIZE, and reading them
 * from it is why the owner saw the bubble at the world origin on the first
 * candidate ROM. dEFManagerShieldEffectDesc's root uses matrix kind 0x4F
 * (efmanager.c:462), whose callback func_ovl0_800C994C ignores the DObj's own
 * translate/rotate/scale entirely and loads the joint stored in user_data.p --
 * efManagerShieldMakeEffect puts fp->joints[nFTPartsJointYRotN] there
 * (efmanager.c:4139). The root's own vectors are therefore never written:
 * measured 2026-08-04, translate/rotate 0 and scale 1. Taking the tree draw
 * away takes that matrix builder away with it, so this proc has to do what
 * ndsRendererAdapterBuildJointAttachMtx (reloc_backend_renderer_dl.c:1242)
 * does, and it is deliberately the same four lines:
 *
 *   - func_ovl2_800EDBA4 FIRST, not as a formality. mtx_translate is a
 *     per-frame cache that ndsFTParamsInvalidateFighterParts clears, and the
 *     fighter's own draw does not fill it, so reading it without the rebuild
 *     returns all zeros -- the origin again, by a second route.
 *   - Position is row 3. nds_r2_collision_mtx.h:639 pins the convention for
 *     this engine: world[c] = sum_k local[k]*M[k][c] + M[3][c].
 *   - Size is the LENGTH OF ROW 0, which for this joint is the shield size
 *     itself: ftCommonGuardUpdateShieldCollision writes
 *     ((0.65 * shield_health/55) + 0.35) * attr->shield_size / 30 into that
 *     joint's scale (ftcommonguard1.c:125). The local scale field holds the
 *     same number today, but the world row is what the source billboard
 *     actually multiplies by, so take it from the matrix and stay correct if a
 *     parent joint is ever scaled.
 *
 * Guard set is that builder's, for that builder's reason: func_ovl2_800EDBA4
 * dereferences ftGetStruct and ftGetParts without checking either. */
static void ndsEFManagerShieldQuadProcDisplay(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *dobj = DObjGetStruct(effect_gobj);
    DObj *attach;
    FTParts *parts;

    if ((ep == NULL) || (dobj == NULL))
    {
        return;
    }
    attach = (DObj *)dobj->user_data.p;
    if ((attach != NULL) && (attach != DOBJ_PARENT_NULL) &&
        (attach->parent_gobj != NULL) &&
        ((parts = ftGetParts(attach)) != NULL))
    {
        Vec3f world_pos;
        f32 guard_scale;

        func_ovl2_800EDBA4(attach);
        world_pos.x = parts->mtx_translate[3][0];
        world_pos.y = parts->mtx_translate[3][1];
        world_pos.z = parts->mtx_translate[3][2];
        guard_scale =
            sqrtf((parts->mtx_translate[0][0] * parts->mtx_translate[0][0]) +
                  (parts->mtx_translate[0][1] * parts->mtx_translate[0][1]) +
                  (parts->mtx_translate[0][2] * parts->mtx_translate[0][2]));
        /* WHITE vertex colour on purpose: the palette carries the player's
         * colour now, and the polygon stays in modulation mode, so anything
         * other than white would tint the ramp a second time. */
        u32 texture_name =
            ndsEFManagerShieldTexture(ep->effect_vars.shield.player);

        if ((texture_name != 0u) &&
            (ndsParticleDrawOwnTextureQuad(
                 texture_name, NDS_SHIELD_TEX_WIDTH, NDS_SHIELD_TEX_HEIGHT,
                 &world_pos, guard_scale * NDS_R2_SHIELD_QUAD_SIZE,
                 NDS_R2_BGR555(0xff, 0xff, 0xff),
                 NDS_R2_SHIELD_QUAD_ALPHA,
                 NDS_R2_SHIELD_QUAD_DEPTH_BIAS,
                 0.0F, FALSE) != FALSE))
        {
            gNdsShieldQuadDrawCount++;
            return;
        }
    }
    gNdsShieldQuadFallbackCount++;
    gcDrawDObjTreeForGObj(effect_gobj);
}
#endif /* NDS_R2_SHIELD_QUAD */

void efManagerInitEffects(void)
{
    ndsEFManagerResetDeferredDescs();
    ndsTask39EffectCensusReset();
    gNdsVisualEffectCreateCount = 0u;
    gNdsVisualEffectDestroyCount = 0u;
    gNdsEFManagerSourceEffectStopCount = 0u;
    gNdsVisualEffectDropCount = 0u;
    gNdsVisualEffectActiveCount = 0u;
    gNdsVisualEffectMaxActiveCount = 0u;
    gNdsVisualEffectKindMask = 0u;
    gNdsVisualEffectTemplateBytes = 0u;
    ndsBaseEFManagerInitEffects();
#if NDS_R2_IMPACT_WAVE_NATIVE
    gNdsImpactWaveNativeDrawCount = 0u;
    gNdsImpactWaveNativeFallbackCount = 0u;
    gNdsImpactWaveNativeTexturePrepareCount = 0u;
    gNdsImpactWaveNativeTextureBindCount = 0u;
#endif
    ndsEFManagerResolveAllDescOffsets();
#if NDS_R2_SHIELD_QUAD
    /* AFTER the resolver and BEFORE any shield exists. The source maker copies
     * proc_display off the desc when it builds the effect, so overriding the
     * desc here reaches every shield made this match without touching the maker
     * or decomp. Descs are already patched in place by the resolver above, so
     * writing one more field is the same kind of write. */
    {
        /* Give last match's names back before this one allocates. START at the
         * results screen restarts the match, so holding them would grow the
         * texture cache by one name per player per restart. */
        u32 slot;

        for (slot = 0u; slot < ARRAY_COUNT(sNdsShieldTextureName); slot++)
        {
            if (sNdsShieldTextureName[slot] != 0u)
            {
                ndsRendererHardwareReleaseIFCommonCloudAtlas(
                    &sNdsShieldTextureName[slot]);
                sNdsShieldTextureName[slot] = 0u;
            }
        }
    }
    gNdsShieldQuadDrawCount = 0u;
    gNdsShieldQuadFallbackCount = 0u;
    dEFManagerShieldEffectDesc.proc_display = ndsEFManagerShieldQuadProcDisplay;
#endif
#if NDS_R2_FIREBALL_QUAD && NDS_RENDERER_HW_TRIANGLES
    /* The fireball's baked texture has the same match lifetime as the shield's
     * and is released here for the same reason: START at the results screen
     * restarts the match. It lives in the port file that owns the weapon
     * submit, so this is the call rather than another release loop. The
     * NDS_RENDERER_HW_TRIANGLES term matches the definition's own home: the
     * quad hook sits inside the hardware-submit block in
     * reloc_backend_movement.c (12778-13413), so a software build that links
     * this call against nothing would fail exactly the way this one just did. */
    ndsWeaponReleaseBakedTextures();
#endif
    ndsEFManagerBoundEffectPool();
    ndsEFManagerInitVisualTemplates();
}

/* The source maker builds llFoxSpecial2ReflectorDObjDesc with its start/loop/
 * hit/end anim joints. The flat-disc stand-in that used to sit here is DELETED
 * (2026-08-04) -- it was what "still not using the correct asset" meant every
 * cycle, and resurrecting it would only re-hide the one seam left.
 *
 * THE RETRY IS THE LIVE PART, and it has never been observed to fire.
 * dEFManagerFoxReflectorEffectDesc resolves with EFDescDisabledCount=1 -- its
 * file is not resident when efManagerInitEffects sweeps -- so the desc is
 * disabled and this call is the only thing that can recover it.
 * gNdsEFDescDeferRecoverCount has read 0 in every automated run because level-3
 * Fox never down-Bs unattended (zero spawns in 150 s). The owner's own manual
 * down-B is what tests it live; keep this call and that counter together. */
GObj *efManagerFoxReflectorMakeEffect(GObj *fighter_gobj)
{
    ndsEFManagerRetryDeferredDescs();
    return ndsBaseEFManagerFoxReflectorMakeEffect(fighter_gobj);
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


/* The star KO. Source spawns efcommon script 0x5C directly.
 *
 * INSTRUMENTED because this row has now survived two fixes and the owner still
 * reports *"not playing at location of the fighter"*. The source spawns it at
 * fp->joints[nFTPartsJointTopN]->translate (ftcommondead.c:357) -- the
 * fighter's own root joint -- and the port imports that function, so the
 * position it receives SHOULD already be the fighter. Twice now this row has
 * been answered with a theory (the v16 rail, then the spawn position) instead
 * of a reading. These record what actually arrives, in whole units so the soak
 * dump is legible: if x/y land near the fighter the position is fine and the
 * complaint is resolution, and if they do not the caller is wrong. */
volatile s32 gNdsStarKOSparkleLastX;
volatile s32 gNdsStarKOSparkleLastY;
volatile s32 gNdsStarKOSparkleLastZ;
volatile u32 gNdsStarKOSparkleCount;

LBParticle *efManagerSparkleWhiteDeadMakeEffect(Vec3f *pos, f32 scale)
{
    if (pos != NULL)
    {
        gNdsStarKOSparkleLastX = (s32)pos->x;
        gNdsStarKOSparkleLastY = (s32)pos->y;
        gNdsStarKOSparkleLastZ = (s32)pos->z;
        gNdsStarKOSparkleCount++;
    }
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

/* Mew's water ripple and heal sparkles (decomp ef/efmanager.c:4231-4242
 * and :5316-5341) need no port work at all: this TU includes the whole of
 * decomp ef/efmanager.c at :206, so both are already compiled in. They were
 * only ever missing a DECLARATION, which include/ef/effect.h:142-143 now
 * carries -- without it Mew's calls compiled as implicit int-returning
 * functions. Defining them here as well was a redefinition error, and the
 * lesson is to check the included source before porting anything into this
 * file. */

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
/* DAMAGE-SPARK SIZE IS A DS DIVERGENCE, AND IT LIVES HERE RATHER THAN IN THE
 * SOURCE MAKER ON PURPOSE. `decomp/` is the behavioral specification: a reader
 * who opens efmanager.c to learn what SSB64 does must find SSB64, not a port
 * preference. So the source ramp is left exactly as it is and adjusted on the
 * way out.
 *
 * What source does (efmanager.c, efManagerDamageNormalLight/HeavyMakeEffect):
 * LIGHT ramps xf->scale with damage -- 0.5x below 10, (size-10)*0.13+1.0 above,
 * unclamped, so 4.9x at the 40-damage ceiling. HEAVY never touches xf->scale at
 * all, so it is a flat 1.0 and a big light spark out-sizes the heavy flash it
 * decays into (efManagerDamageNormalHeavyProcDead respawns a light one).
 *
 * Owner, 2026-08-06, reviewing against N64 capture and then playing the build:
 * halve both and give HEAVY the same ramp. Approved by eye -- this is a
 * presentation choice with the owner as oracle, not a defect being corrected.
 * 4.9x is right at 640x480 and reads as an orange ball at 256x192.
 *
 * Writing scale after the maker returns is the same position source writes it:
 * both land after LBParticleProcessStruct, and ndsParticleTransformForDraw
 * builds the affine lazily from transform_status, which is still Default here. */
#ifndef NDS_DAMAGE_SPARK_SCALE
#define NDS_DAMAGE_SPARK_SCALE 0.5F
#endif

/* Engagement proof for the route above: non-zero means the port-side adjust ran
 * on a real transform. Zero after a match with hits means either the maker was
 * never reached or pc->xf was NULL, and those need different fixes. */
volatile u32 gNdsDamageSparkScaleCount;

static void ndsDamageSparkScale(LBParticle *pc, s32 size)
{
    f32 ramp;

    if ((pc == NULL) || (pc->xf == NULL))
    {
        return;
    }
    if (size < 0)
    {
        size = 0;
    }
    else if (size > 40)
    {
        size = 40;
    }
    ramp = (size < 10) ? (((10 - size) * -0.05F) + 1.0F)
                       : (((size - 10) * 0.13F) + 1.0F);
    pc->xf->scale.x = pc->xf->scale.y = pc->xf->scale.z =
        ramp * NDS_DAMAGE_SPARK_SCALE;
    gNdsDamageSparkScaleCount++;
}

LBParticle *efManagerDamageNormalLightMakeEffect(Vec3f *pos, s32 player,
                                                 s32 size, sb32 is_static)
{
    LBParticle *pc = ndsBaseEFManagerDamageNormalLightMakeEffect(
        pos, player, size, is_static);

    ndsDamageSparkScale(pc, size);
    return pc;
}
LBParticle *efManagerDamageNormalHeavyMakeEffect(Vec3f *pos, s32 player,
                                                 s32 size)
{
    LBParticle *pc = ndsBaseEFManagerDamageNormalHeavyMakeEffect(pos, player,
                                                                 size);

    ndsDamageSparkScale(pc, size);
    return pc;
}
LBParticle *efManagerDamageFireMakeEffect(Vec3f *pos, s32 size)
{
    /* BUGS.md fire-burn row: the only live fire request in P1. Called only
     * from the nGMHitElementFire arms of ftmain.c (2713/2771/2808) and the
     * item equivalents, which are off, so this counts fire hits that
     * dispatched. */
    gNdsFighterDamageFireCallCount++;
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
#if NDS_R2_FIREGRIND_NATIVE
    /* DS-native FireGrind: three fixed-pool quads at the rebound point instead
     * of the source root particle + three generators + six sparks. The Mario
     * fireball caller ignores the returned LBParticle*, so NULL is safe. See
     * include/nds/nds_firegrind.h. */
    ndsFireGrindSpawn(pos);
    return NULL;
#else
    return ndsBaseEFManagerFireGrindMakeEffect(pos);
#endif
}
LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos)
{
    return ndsBaseEFManagerSparkleWhiteMakeEffect(pos);
}
LBParticle *efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale)
{
    return ndsBaseEFManagerSparkleWhiteScaleMakeEffect(pos, scale);
}

/* BattleShip ftparam.c:2059-2066 owns this tiny specialization at the fighter
 * layer, but LBParticle is deliberately opaque outside the effect owner on DS.
 * Keep the exact source result (scale 0.7, prim alpha 0xC0) here rather than
 * leaking the particle layout into reloc_backend_compat_shims.c. */
LBParticle *ndsEFManagerChargeSparkleMakeEffect(Vec3f *pos)
{
    LBParticle *pc = efManagerSparkleWhiteScaleMakeEffect(pos, 0.7F);

    if (pc != NULL)
    {
        pc->primcolor.a = 0xC0;
    }
    return pc;
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
 * twenty were routed at once.
 *
 * GRADUATED 2026-08-04. The second problem -- "their geometry goes out as
 * source effect DL links, which the battle hardware path does not submit" --
 * was a link-coverage gap in ndsStageGCDrawAllLoopIsEffectDisplay, closed in
 * cycle 50; the impact wave is the one of the six that spawns in P1 and it
 * draws. The weak stand-in shims these used to override are deleted. */
GObj *efManagerDamageSlashMakeEffect(Vec3f *pos, s32 size, f32 rotate)
{
    return ndsBaseEFManagerDamageSlashMakeEffect(pos, size, rotate);
}
GObj *efManagerImpactWaveMakeEffect(Vec3f *pos, s32 index, f32 rotate)
{
    /* Row 4's arming counter. The wave has never been captured, and without
     * this a probe cannot tell "spawned and drew grey" from "never spawned":
     * efManagerMakeEffectNoForce returns NULL on a full EFStruct pool and the
     * source maker swallows it (efmanager.c:3335). Index is recorded because
     * it IS the trigger identity -- the hard landing passes 4 and the owner
     * reports neither white nor green. */
    GObj *effect_gobj =
        ndsBaseEFManagerImpactWaveMakeEffect(pos, index, rotate);

    gNdsEffectImpactWaveLastIndex = index;
    if (effect_gobj != NULL)
    {
        gNdsEffectImpactWaveMakeCount++;
        gNdsEffectImpactWaveLastGObjID = (u32)effect_gobj->id;
    }
    else
    {
        gNdsEffectImpactWaveMakeNullCount++;
    }
    return effect_gobj;
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

/* Yoshi's egg-shaped guard bubble: dEFManagerYoshiShieldEffectDesc draws the
 * Yoshi-model shield DObj (llYoshiModelShieldDObjDesc) with
 * efManagerShieldProcUpdate / efManagerYoshiShieldProcDisplay. Callers are
 * ftcommonguard1.c:389 and ftcommonguard2.c:21, which store the result in
 * fp->status_vars.common.guard.effect_gobj. It was missing because this TU
 * compiled the body only as ndsBaseEFManagerYoshiShieldMakeEffect while a
 * weak NULL-returning stub in reloc_backend_compat_shims.c:3532 won the link,
 * so guarding as Yoshi never produced the bubble. A DObj-tree model effect --
 * no particle-bank script, so the bank reachable set does not gate it. */
GObj *efManagerYoshiShieldMakeEffect(GObj *fighter_gobj)
{
    return ndsBaseEFManagerYoshiShieldMakeEffect(fighter_gobj);
}

/* Kirby's Vulcan Jab hit effect: dEFManagerVulcanJabEffectDesc
 * (llKirbySpecial2VulcanJabDObjDesc, file gFTDataKirbySpecial2) with
 * efManagerKirbyVulcanJabProcUpdate. Caller is ftcommonattack100.c:99, which
 * passes the per-hit rotate/vel/add. It was missing for the same reason: the
 * ndsBase body sat uncalled while the weak stub in
 * reloc_backend_compat_shims.c:4500 answered NULL. DObj-tree model effect, no
 * particle-bank script, so the reachable set does not gate it. */
GObj *efManagerKirbyVulcanJabMakeEffect(Vec3f *pos, s32 lr, f32 rotate, f32 vel, f32 add)
{
    return ndsBaseEFManagerKirbyVulcanJabMakeEffect(pos, lr, rotate, vel, add);
}

/* Samus's grapple-beam glow while holding a caught fighter:
 * dEFManagerSamusGrappleBeamEffectDesc (llSamusSpecial2GrappleBeamDObjDesc,
 * file gFTDataSamusSpecial2), attached to joint 23. Callers are
 * ftcommoncatch1.c:98 (gated on nFTKindSamus / nFTKindNSamus) and
 * ftcommonthrow.c:88. Missing because the weak stub in
 * reloc_backend_compat_shims.c:4512 won the link over the uncalled ndsBase
 * body. DObj-tree model effect, no particle-bank script, so the reachable
 * set does not gate it. */
GObj *efManagerSamusGrappleBeamGlowMakeEffect(GObj *fighter_gobj)
{
    return ndsBaseEFManagerSamusGrappleBeamGlowMakeEffect(fighter_gobj);
}
