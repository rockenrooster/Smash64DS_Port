/*
 * P4 runtime: Smash Remix fighters as setup parent + resolved donor content.
 * See include/nds/nds_p4.h for the identity model and docs/P4/P4_STATUS.md for
 * the source adapter that generates each content's data from the user's ROM.
 */
#include <nds/nds_p4.h>
#if NDS_P5
#include "nds_p5_native_stages.generated.h"
#endif
#include "nds_p4_onep_art.generated.h"


#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ef/effect.h>
#include <it/item.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_frontend_overlay.h>
#include <nds/nds_kirby_hat_residency.h>
#include <nds/nds_renderer.h>
#include <nds/nds_scene_manager.h>
#include <nds/nds_startup.h>
#include <macros.h>
#include <sc/scene.h>
#include <sys/objman.h>
#include <sys/audio.h>
#include <sys/taskman.h>
#include <wp/weapon.h>
#include <nds/nds_p7.h>
#include <reloc_data.h>
#include "kirby/nds_p4_kirby.generated.h"

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

DObj *gcGetTreeDObjNext(DObj *dobj);
void func_ovl2_800EDBA4(DObj *main_dobj);
void ftComputerSetCommandWaitShort(FTStruct *fp, s32 index);
void ftComputerSetCommandImmediate(FTStruct *fp, s32 index);
void ftComputerSetControlPKThunder(FTStruct *fp);
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);

u8 gNdsP4PlayerContent[GMCOMMON_PLAYERS_MAX];
u8 gNdsP4TrainingContent[2];
u8 gNdsP4OnePlayerContent;
u8 gNdsP7BonusPracticeContent;
f32 gNdsP4TranslationMultiplier[GMCOMMON_PLAYERS_MAX] = { 1.0F, 1.0F, 1.0F, 1.0F };
u32 gNdsP4EnvColor[GMCOMMON_PLAYERS_MAX];
/* Remix custom commands no P4 content has needed yet: counted, never silent. */
__attribute__((used)) volatile u32 gNdsP4UnportedMotionEvents;
__attribute__((used)) volatile u32 gNdsP4UnportedMotionEventLast;

/* Each content's generated TU (scripts/p4/generate_p4_fighter.py) defines
 * the same family of symbols under its title; one row per content. */
#define NDS_P4_DECLARE(T) \
    extern FTData gNdsP4##T##Data; \
    extern FTStatusDesc gNdsP4##T##SpecialStatusDescs[]; \
    extern const u32 gNdsP4##T##SpecialStatusCount; \
    extern const u32 gNdsP4##T##LabFallbackCount; \
    extern const u16 gNdsP4##T##RelocAssets[]; \
    extern const u32 gNdsP4##T##RelocAssetCount; \
    extern const FTFileSize gNdsP4##T##FileSize; \
    extern const u16 gNdsP4##T##Anims[]; \
    extern const u32 gNdsP4##T##AnimCount; \
    extern const u8 gNdsP4##T##StockGfx[]; \
    extern const u16 gNdsP4##T##StockPalettes[][16]; \
    extern const u32 gNdsP4##T##StockPaletteCount; \
    extern const NDSP4SpriteDesc gNdsP4##T##Sprites[]; \
    extern const u32 gNdsP4##T##SpriteCount; \
    extern const NDSP4Present gNdsP4##T##Present; \
    extern const NDSP4SwordTrail gNdsP4##T##SwordTrails[]; \
    extern const u32 gNdsP4##T##SwordTrailCount; \
    extern const NDSP4Entry gNdsP4##T##Entry; \
    extern const NDSP4SpecialStart gNdsP4##T##SpecialStarts[]; \
    extern const NDSP4Jab gNdsP4##T##Jab; \
    extern const f32 gNdsP4##T##YoshiEgg[]; \
    extern const u16 gNdsP4##T##LabSkipFiles[]; \
    extern const u32 gNdsP4##T##LabSkipFileCount; \
    extern const u8 gNdsP4##T##OpenSpecialMask; \
    extern const NDSP4KindCases gNdsP4##T##KindCases; \
    extern const NDSP4KirbyInhale gNdsP4##T##KirbyInhale; \
    extern const u8 gNdsP4##T##ComputerLongRange;     extern const u16 gNdsP4##T##Event32Commands;     extern const u16 gNdsP4##T##Event32MatAnims[2];     extern const u16 gNdsP4##T##Event32Allowance; \
    extern const u8 gNdsP4##T##ComputerLongRangeMove; \
    extern const u8 gNdsP4##T##ComputerReflect; \
    extern const NDSP4BakedRef gNdsP4##T##BakedRefs[]; \
    extern const u32 gNdsP4##T##BakedRefCount; \
    extern const NDSP4ShieldPose gNdsP4##T##ShieldPose; \
    extern const f32 gNdsP4##T##AnimRate; \
    extern const u16 gNdsP4##T##AnimRateStatuses[]; \
    extern const u16 gNdsP4##T##AnimRateStatusCount; \
    extern const f32 gNdsP4##T##DamageScale; \
    extern const u8 gNdsP4##T##DamageScaleSkipsSmash;
#define NDS_P4_ROW(T, title, parent, on_status_hook, computer_rows, override_rows) \
    { \
        .name = (title), .parent_kind = (parent), \
        .data = &gNdsP4##T##Data, \
        .special_statuses = gNdsP4##T##SpecialStatusDescs, \
        .special_status_count = &gNdsP4##T##SpecialStatusCount, \
        .lab_fallback_count = &gNdsP4##T##LabFallbackCount, \
        .assets = gNdsP4##T##RelocAssets, \
        .asset_count = &gNdsP4##T##RelocAssetCount, \
        .file_size = &gNdsP4##T##FileSize, \
        .anims = gNdsP4##T##Anims, .anim_count = &gNdsP4##T##AnimCount, \
        .on_status = (on_status_hook), \
        .computer = (computer_rows), \
        .stock_gfx = gNdsP4##T##StockGfx, \
        .stock_palettes = gNdsP4##T##StockPalettes, \
        .stock_palette_count = &gNdsP4##T##StockPaletteCount, \
        .sprites = gNdsP4##T##Sprites, \
        .sprite_count = &gNdsP4##T##SpriteCount, \
        .present = &gNdsP4##T##Present, \
        .sword_trails = gNdsP4##T##SwordTrails, \
        .sword_trail_count = &gNdsP4##T##SwordTrailCount, \
        .entry = &gNdsP4##T##Entry, \
        .special_starts = gNdsP4##T##SpecialStarts, \
        .jab = &gNdsP4##T##Jab, \
        .yoshi_egg = gNdsP4##T##YoshiEgg, \
        .lab_skip_files = gNdsP4##T##LabSkipFiles, \
        .lab_skip_file_count = &gNdsP4##T##LabSkipFileCount, \
        .open_special_mask = &gNdsP4##T##OpenSpecialMask, \
        .kind_cases = &gNdsP4##T##KindCases, \
        .kirby_inhale = &gNdsP4##T##KirbyInhale, \
        .computer_long_range = &gNdsP4##T##ComputerLongRange, \
        .computer_long_range_move = &gNdsP4##T##ComputerLongRangeMove,         .event32_commands = &gNdsP4##T##Event32Commands,         .event32_matanims = gNdsP4##T##Event32MatAnims,         .event32_allowance = &gNdsP4##T##Event32Allowance, \
        .computer_reflect = &gNdsP4##T##ComputerReflect, \
        .baked_refs = gNdsP4##T##BakedRefs, \
        .baked_ref_count = &gNdsP4##T##BakedRefCount, \
        .shield_pose = &gNdsP4##T##ShieldPose, \
        .anim_rate = &gNdsP4##T##AnimRate, \
        .anim_rate_statuses = gNdsP4##T##AnimRateStatuses, \
        .anim_rate_status_count = &gNdsP4##T##AnimRateStatusCount, \
        .damage_scale = &gNdsP4##T##DamageScale, \
        .damage_scale_skips_smash = &gNdsP4##T##DamageScaleSkipsSmash, \
        .overrides = (override_rows), \
    }

/* Every compiled content (nds_p4_contents.h). Its native hooks live in
 * src/port/nds_p4_<name>.c when it has any: ndsP4<Title>OnStatus (donor init
 * hooks after the status callbacks), gNdsP4<Title>Computer (CPU routines)
 * and gNdsP4<Title>Overrides (the parent's routines it replaces). All
 * are weak, so a content without them gets NULL. A content takes the hooks
 * of the content whose code it runs (NDS_P4_CODE_ROWS): its own, or a P6
 * authored content's base's until it has a source file of its own. */
#define NDS_P4_DECLARE_ROW(id_, T_, N_, n_, parent_, model_, main_) \
    NDS_P4_DECLARE(T_)
NDS_P4_CONTENT_ROWS(NDS_P4_DECLARE_ROW)
#undef NDS_P4_DECLARE_ROW
#define NDS_P4_DECLARE_HOOKS(id_, T_, code_id_, C_) \
    void ndsP4##C_##OnStatus(GObj *fighter_gobj, s32 status_id) \
        __attribute__((weak)); \
    extern const NDSP4Computer gNdsP4##C_##Computer __attribute__((weak)); \
    extern const NDSP4Overrides gNdsP4##C_##Overrides __attribute__((weak));
NDS_P4_CODE_ROWS(NDS_P4_DECLARE_HOOKS)
#undef NDS_P4_DECLARE_HOOKS
#define NDS_P4_HOOK_(C_, prefix_, what_) prefix_##C_##what_
#define NDS_P4_HOOK(C_, prefix_, what_) NDS_P4_HOOK_(C_, prefix_, what_)

static const NDSP4Fighter sNdsP4Fighters[NDS_P4_CONTENT_LIMIT] = {
#define NDS_P4_FIGHTER_ROW(id_, T_, N_, n_, parent_, model_, main_) \
    [(id_)] = NDS_P4_ROW(T_, #T_, parent_, \
                         NDS_P4_HOOK(NDS_P4_CODE_TITLE_##T_, ndsP4, OnStatus), \
                         &NDS_P4_HOOK(NDS_P4_CODE_TITLE_##T_, gNdsP4, Computer), \
                         &NDS_P4_HOOK(NDS_P4_CODE_TITLE_##T_, gNdsP4, Overrides)),
    NDS_P4_CONTENT_ROWS(NDS_P4_FIGHTER_ROW)
#undef NDS_P4_FIGHTER_ROW
};
#undef NDS_P4_HOOK
#undef NDS_P4_HOOK_

/* The content whose code each content runs (0: its own). */
static const u8 sNdsP4CodeContent[NDS_P4_CONTENT_LIMIT] = {
#define NDS_P4_CODE_CONTENT_ROW(id_, T_, code_id_, C_) [(id_)] = (code_id_),
    NDS_P4_CODE_ROWS(NDS_P4_CODE_CONTENT_ROW)
#undef NDS_P4_CODE_CONTENT_ROW
};

u32 ndsP4CodeContent(u32 content)
{
    return ((content < NDS_P4_CONTENT_LIMIT) && (sNdsP4CodeContent[content] != 0u)) ?
        (u32)sNdsP4CodeContent[content] : content;
}

/* Menu-motion scripts are absolute pointers in the source (opening statuses
 * take motion_desc->offset as the script itself); the generator emits them as
 * offsets into the content's motion file, rebased here per load. */
static void *sNdsP4MenuScriptBase[NDS_P4_CONTENT_LIMIT];

/* Each content's match tables (ndsP4LoadTables) and the heap generation
 * they were read in: a later scene's heap is a new one. */
static const NDSP4TablesHeader *sNdsP4Tables[NDS_P4_CONTENT_LIMIT];
static u32 sNdsP4TablesGeneration[NDS_P4_CONTENT_LIMIT];
static const char *const sNdsP4TableNames[NDS_P4_CONTENT_LIMIT] = {
#define NDS_P4_TABLE_NAME_ROW(id_, T_, N_, n_, parent_, model_, main_) [(id_)] = #n_,
    NDS_P4_CONTENT_ROWS(NDS_P4_TABLE_NAME_ROW)
#undef NDS_P4_TABLE_NAME_ROW
};
/* The content whose tables failed to load (a halt follows), for probes. */
__attribute__((used)) volatile u32 gNdsP4TablesFailure;
static const NDSP4TablesHeader *ndsP4FighterTables(const FTStruct *fp);

/* Whether a content's animation rate applies to `status_id`: its list
 * (ascending) holds it, or it lists none. */
static sb32 ndsP4AnimRateStatus(const NDSP4Fighter *f, s32 status_id)
{
    u32 lo = 0u;
    u32 hi = (f->anim_rate_status_count != NULL) ? *f->anim_rate_status_count : 0u;

    if (hi == 0u)
    {
        return TRUE;
    }
    while (lo < hi)
    {
        u32 mid = (lo + hi) >> 1;
        s32 s = f->anim_rate_statuses[mid];

        if (s == status_id)
        {
            return TRUE;
        }
        if (s < status_id)
        {
            lo = mid + 1u;
        }
        else hi = mid;
    }
    return FALSE;
}

f32 ndsP4ScaleAnimSpeed(GObj *fighter_gobj, s32 status_id, f32 speed)
{
    const NDSP4Fighter *f = ndsP4Fighter(ftGetStruct(fighter_gobj)->nds_p4_content);

    if ((f == NULL) || (f->anim_rate == NULL) || (*f->anim_rate == 1.0F) ||
        !ndsP4AnimRateStatus(f, status_id))
    {
        return speed;
    }
    return speed * *f->anim_rate;
}

void ndsP4AttackCollPlanarSlow(const FTStruct *fp, Vec3f *pos)
{
    const NDSP4Overrides *o = ndsP4Overrides(fp);

    if ((o != NULL) && (o->planar_hits != 0u))
    {
        pos->z = fp->joints[nFTPartsJointTopN]->translate.vec.f.z;
    }
}

s32 ndsP4StaledDamage(const FTStruct *fp, s32 player, s32 damage, s32 attack_id,
                      u16 motion_count)
{
    s32 staled = ftParamGetStaledDamage(player, damage, attack_id, motion_count);
    const NDSP4Fighter *f = ndsP4Fighter(fp->nds_p4_content);

    if ((f == NULL) || (f->damage_scale == NULL) || (*f->damage_scale == 1.0F))
    {
        return staled;
    }
    /* Owner p7h (2026-10-19): Bowser's +10% is on his non-smash attacks. */
    if ((f->damage_scale_skips_smash != NULL) &&
        (*f->damage_scale_skips_smash != 0u) &&
        (fp->status_id >= nFTCommonStatusAttackS4Hi) &&
        (fp->status_id <= nFTCommonStatusAttackLw4))
    {
        return staled;
    }
    return (s32)(((f32)staled * *f->damage_scale) + 0.5F);
}

const NDSP4Fighter *ndsP4Fighter(u32 content)
{
    if ((content == 0u) || (content >= NDS_P4_CONTENT_LIMIT) ||
        (sNdsP4Fighters[content].data == NULL))
    {
        return NULL;
    }
    return &sNdsP4Fighters[content];
}

u32 ndsP4PlayerContent(u32 player)
{
    GObj *gobj;

    if ((player >= GMCOMMON_PLAYERS_MAX) || (gSCManagerBattleState == NULL))
    {
        return 0u;
    }
    /* An empty seat's fighter_gobj is whatever the battle state last held:
     * the 1P game keeps its state across stages, so after a team stage the
     * seats a one-enemy stage leaves empty point at freed GObjs (the battle
     * HUD hashes every seat each frame; a content player's campaign aborted
     * reading one at stage 3, walk-tr4, 2026-10-19). */
    if (gSCManagerBattleState->players[player].pkind == nFTPlayerKindNot)
    {
        return 0u;
    }
    gobj = gSCManagerBattleState->players[player].fighter_gobj;
    if ((gobj == NULL) || (gobj->user_data.p == NULL))
    {
        return 0u;
    }
    return ((FTStruct *)gobj->user_data.p)->nds_p4_content;
}

static u8 sNdsP4PreviewContent[GMCOMMON_PLAYERS_MAX];

void ndsP4SetPreviewContent(u32 slot, u32 content)
{
    if (slot < GMCOMMON_PLAYERS_MAX)
    {
        sNdsP4PreviewContent[slot] = (u8)content;
    }
}

u32 ndsP4MatchContent(s32 player)
{
    if ((player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
    {
        return 0u;
    }
    switch (gSCManagerSceneData.scene_curr)
    {
    case nSCKindVSBattle:
    case nSCKindVSResults:
        return gNdsP4PlayerContent[player];
#if NDS_1P_GAME
    case nSCKind1PBonusStage:
        /* The campaign's boards are entered from nSCKind1PGame
         * (sc1PBonusStageInitVars) and seat the campaign player; practice
         * boards take the bonus select's fighter (C2). */
        if (gSCManagerSceneData.scene_prev != nSCKind1PGame)
        {
            /* P7 C2: a practice's player, while its kind is the content's
             * parent (sc1PBonusStageInitVars seats bonus_fkind). */
            if ((player == (s32)gSCManagerSceneData.player) &&
                ((gSCManagerSceneData.scene_prev == nSCKind1PBonus1Players) ||
                 (gSCManagerSceneData.scene_prev == nSCKind1PBonus2Players)))
            {
                const NDSP4Fighter *f = ndsP4Fighter(gNdsP7BonusPracticeContent);

                return ((f != NULL) &&
                        (f->parent_kind == (s32)gSCManagerSceneData.bonus_fkind)) ?
                    (u32)gNdsP7BonusPracticeContent : 0u;
            }
            return 0u;
        }
        /* fall through */
    case nSCKind1PGame:
        /* P7 C1: the campaign player, while its kind is the content's
         * parent (the select wrote both). */
        if (player == (s32)gSCManagerSceneData.player)
        {
            const NDSP4Fighter *f = ndsP4Fighter(gNdsP4OnePlayerContent);

            return ((f != NULL) &&
                    (f->parent_kind == (s32)gSCManagerSceneData.fkind)) ?
                (u32)gNdsP4OnePlayerContent : 0u;
        }
        /* P7 U5: a content challenger's fight (battleship_sc1pmanager.c). */
        return ndsP7ChallengerContent(player);
#if NDS_1P_INTRO_BAKE
    case nSCKind1PIntro:
    {
        /* LAB: the still bake's live intro makes the player's card as the
         * campaign player's content (battleship_sc1pintro.c
         * gNdsIntroBakePlayerCard); the shipping intro makes no fighter. */
        extern volatile u32 gNdsIntroBakePlayerCard;

        return (gNdsIntroBakePlayerCard != 0u) ? (u32)gNdsP4OnePlayerContent : 0u;
    }
#endif
    case nSCKind1PChallenger:
        /* P7 U5: a content challenger's screen; 0 in every other 1P scene. */
        return ndsP7ChallengerContent(player);
    case nSCKind1PContinue:
    case nSCKindEnding:
        /* P7 C1: the campaign player's figure on the continue screen and in
         * the ending (mnPlayers1PGameContinueMakeFighter,
         * mvEndingMakeFighter: seat 0, the select's kind). */
        if (player == 0)
        {
            const NDSP4Fighter *f = ndsP4Fighter(gNdsP4OnePlayerContent);

            return ((f != NULL) &&
                    (f->parent_kind == (s32)gSCManagerSceneData.fkind)) ?
                (u32)gNdsP4OnePlayerContent : 0u;
        }
        return 0u;
    case nSCKind1PTrainingMode:
        /* P7 C4: the man and the COM by the scene's own seats. */
        if (player == (s32)gSCManagerSceneData.player)
        {
            return gNdsP4TrainingContent[0];
        }
        if (player == ((gSCManagerSceneData.player == 0) ? 1 : 0))
        {
            return gNdsP4TrainingContent[1];
        }
        return 0u;
#endif
    default:
        return 0u;
    }
}

s32 ndsP4GroundedKind(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return ((f != NULL) && (f->kind_cases != NULL)) ?
        (s32)f->kind_cases->grounded : (s32)fp->fkind;
}

s32 ndsP4PipeTurnKind(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return ((f != NULL) && (f->kind_cases != NULL) && (f->kind_cases->pipe_turn != 0u)) ?
        (s32)fp->fkind : NDS_P4_FOREIGN_FKIND;
}

/* S15 (owner 2026-10-08): a content in a battle of three or four fighters
 * keeps its low detail. The campaign's team stages are such battles too (P7
 * C1: Palutena's high image and pack ran Giant DK's and the Polygon Team's
 * setups out of memory). */
sb32 ndsP4LowDetailBattle(void)
{
    const u32 scene = (u32)gSCManagerSceneData.scene_curr;

    if ((scene != (u32)nSCKindVSBattle)
#if NDS_1P_GAME
        && (scene != (u32)nSCKind1PGame) &&
        (scene != (u32)nSCKind1PBonusStage) &&
        (scene != (u32)nSCKind1PTrainingMode)
#endif
       )
    {
        return FALSE;
    }
    if (gSCManagerBattleState == NULL)
    {
        return FALSE;
    }
    return ((gSCManagerBattleState->pl_count + gSCManagerBattleState->cp_count) >= 3) ?
        TRUE : FALSE;
}

sb32 ndsP4DuelHighDetail(s32 player, s32 fkind)
{
    u32 content;
    const NDSP4Fighter *f;
    sb32 duel = FALSE;

    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) && (gSCManagerBattleState != NULL) &&
        ((gSCManagerBattleState->pl_count + gSCManagerBattleState->cp_count) <= 2))
    {
        duel = TRUE;
    }
#if NDS_1P_GAME
    /* P7 U5: a challenger fight is one on one. */
    if ((gSCManagerSceneData.scene_curr == nSCKind1PGame) && (gNdsP7Challenger.active != 0u))
    {
        duel = TRUE;
    }
#endif
    if (duel == FALSE)
    {
        return FALSE;
    }
    content = ndsP4MakeContent(player, fkind);
    f = (content != 0u) ? ndsP4Fighter(content) : NULL;
    return ((f != NULL) && (f->overrides != NULL) && (f->overrides->duel_high_detail != 0u)) ? TRUE : FALSE;
}

/* The native character select: the VS select, and the training and (P7 C1)
 * 1P selects it also runs as -- its previews take their contents from the
 * screen (sNdsP4PreviewContent), not from a match's seats. */
static sb32 ndsP4SceneIsSelect(void)
{
    const u32 scene = (u32)gSCManagerSceneData.scene_curr;

    return ((scene == (u32)nSCKindPlayersVS)
#if NDS_1P_GAME && NDS_MENU_SHELL
            || (scene == (u32)nSCKindPlayers1PTraining) ||
            (scene == (u32)nSCKind1PGamePlayers) ||
            /* P7 C2: the bonus practice selects too. */
            (scene == (u32)nSCKind1PBonus1Players) ||
            (scene == (u32)nSCKind1PBonus2Players)
#endif
           ) ? TRUE : FALSE;
}

sb32 ndsP4LowDetailSelect(void)
{
    return ndsP4SceneIsSelect();
}

void ftNessSpecialLwProcAbsorb(GObj *fighter_gobj);
void ftParamsUpdateFighterPartsTransformAll(DObj *dobj);
void ftParamSetAnimLocks(FTStruct *fp);
void ftParamClearAnimLocks(FTStruct *fp);

void ndsP4ProcAbsorb(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o = ndsP4Overrides(fp);
    DObj *joint;

    if ((o == NULL) || (o->absorb_hit == NULL))
    {
        ftNessSpecialLwProcAbsorb(fighter_gobj);
        return;
    }
    /* ftNessSpecialLwProcAbsorb up to the status it sets. */
    joint = fp->joints[nFTPartsJointTopN];
    fp->lr = fp->absorb_lr;
    joint->rotate.vec.f.y += F_CST_DTOR32(-180.0F);
    ftParamsUpdateFighterPartsTransformAll(fp->joints[nFTPartsJointTopN]);
    o->absorb_hit(fighter_gobj);
}

void ftParamStopVoiceRunProcDamage(GObj *fighter_gobj);
void ftSetupDropItem(FTStruct *fp);
void ftCommonThrownSetStatusDamageRelease(GObj *fighter_gobj);
void ftCommonThrownDecideFighterLoseGrip(GObj *fighter_gobj, GObj *interact_gobj);
void ftCommonThrownProcPhysics(GObj *fighter_gobj);
void ftCommonCapturePulledProcMap(GObj *fighter_gobj);

/* ftCommonCapturePulledProcCapture with the grabber's capture status, and
 * the thrown statuses' physics for the first frame
 * (CustomGrabAction.asm capture_action_override_, capture_position_fix_). */
sb32 ndsP4CustomCapture(GObj *fighter_gobj, GObj *capture_gobj)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
    FTStruct *capture_fp = ftGetStruct(capture_gobj);
    const NDSP4Overrides *o = ndsP4Overrides(capture_fp);

    if ((o == NULL) || (o->capture_status == 0))
    {
        return FALSE;
    }
    ftParamStopVoiceRunProcDamage(fighter_gobj);

    if ((this_fp->item_gobj != NULL) && (itGetStruct(this_fp->item_gobj)->weight == nITWeightHeavy))
    {
        ftSetupDropItem(this_fp);
    }
    if (this_fp->catch_gobj != NULL)
    {
        ftCommonThrownSetStatusDamageRelease(this_fp->catch_gobj);

        this_fp->catch_gobj = NULL;
    }
    else if (this_fp->capture_gobj != NULL)
    {
        ftCommonThrownDecideFighterLoseGrip(this_fp->capture_gobj, fighter_gobj);
    }
    this_fp->is_catch_or_capture = FALSE;
    this_fp->capture_gobj = capture_gobj;
    this_fp->lr = -capture_fp->lr;

    ftMainSetStatus(fighter_gobj, o->capture_status, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);

    this_fp->status_vars.common.capture.is_goto_pulled_wait = FALSE;

    ftParamSetCaptureImmuneMask(this_fp, FTCATCHKIND_MASK_ALL);
    ftParamMakeRumble(this_fp, 9, 0);
    ftPhysicsStopVelAll(fighter_gobj);
    ftCommonThrownProcPhysics(fighter_gobj);
    ftCommonCapturePulledProcMap(fighter_gobj);

    return TRUE;
}

sb32 ndsP4ShoulderedInterrupt(GObj *victim_gobj)
{
    GObj *capture_gobj = ftGetStruct(victim_gobj)->capture_gobj;
    const NDSP4Overrides *o = (capture_gobj != NULL) ?
        ndsP4Overrides(ftGetStruct(capture_gobj)) : NULL;

    return ((o != NULL) && (o->shouldered_interrupt != NULL)) ?
        o->shouldered_interrupt(victim_gobj) : FALSE;
}

s32 ndsP4CommandCaptureStatus(GObj *fighter_gobj, s32 status_id)
{
    GObj *capture_gobj = ftGetStruct(fighter_gobj)->capture_gobj;
    const NDSP4Overrides *o;

    if ((status_id != nFTCommonStatusCaptureYoshi) || (capture_gobj == NULL))
    {
        return status_id;
    }
    o = ndsP4Overrides(ftGetStruct(capture_gobj));

    return ((o != NULL) && (o->command_capture_status != 0)) ?
        o->command_capture_status : status_id;
}

GObj *(*ndsP4Fireball(const FTStruct *fp))(GObj *fighter_gobj, Vec3f *pos)
{
    const NDSP4Overrides *o = ndsP4Overrides(fp);

    if (o != NULL)
    {
        return o->fireball;
    }
    if ((fp->nds_p4_content == 0u) && (fp->nds_p4_kirby_copy != 0u))
    {
        const NDSP4Fighter *f = ndsP4Fighter(fp->nds_p4_kirby_copy);

        return ((f != NULL) && (f->overrides != NULL)) ? f->overrides->kirby_fireball : NULL;
    }
    return NULL;
}

sb32 ndsP4ThrownHidden(const FTStruct *fp)
{
    const NDSP4Overrides *o = ndsP4Overrides(fp);

    return ((o != NULL) && (o->thrown_hidden != FALSE)) ? TRUE : FALSE;
}

sb32 ndsP4GuardPosesNeeded(void)
{
    return (gSCManagerSceneData.scene_curr != nSCKindVSResults) ? TRUE : FALSE;
}

sb32 ndsP4LowDetailResults(void)
{
    if (gSCManagerSceneData.scene_curr != nSCKindVSResults)
    {
        return FALSE;
    }
    return ((gSCManagerTransferBattleState.pl_count +
             gSCManagerTransferBattleState.cp_count) >= 3) ? TRUE : FALSE;
}

sb32 ndsP4ContentLowDetailOnly(s32 player)
{
    if (ndsP4MatchContent(player) == 0u)
    {
        return FALSE;
    }
    return ndsP4LowDetailBattle();
}

u32 ndsP4MakeContent(s32 player, s32 fkind)
{
    const NDSP4Fighter *f;
    u32 content;

    if ((player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
    {
        return 0u;
    }
    content = (ndsP4SceneIsSelect() != FALSE) ?
        sNdsP4PreviewContent[player] : ndsP4MatchContent(player);
    f = ndsP4Fighter(content);
    return ((f != NULL) && (f->parent_kind == fkind)) ? content : 0u;
}

sb32 ndsP4ParentFilesNeeded(s32 fkind)
{
    const SCBattleState *state;
    s32 player;
    sb32 child = FALSE;

    /* The two scenes whose fighters are exactly the match's players: the
     * battle, and the Results podium built from the transfer state. */
    if (gSCManagerSceneData.scene_curr == nSCKindVSBattle)
    {
        state = gSCManagerBattleState;
    }
    else if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
    {
        state = &gSCManagerTransferBattleState;
    }
#if NDS_1P_GAME
    else if ((gSCManagerSceneData.scene_curr == nSCKind1PGame) &&
             (gNdsP7Challenger.active != 0u))
    {
        /* P7 U5: a content challenger's fight loads no parent files unless
         * the player is the parent. */
        state = gSCManagerBattleState;
    }
#endif
    else
    {
        return TRUE;
    }
    if (state == NULL)
    {
        return TRUE;
    }
    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        if ((state->players[player].pkind == nFTPlayerKindNot) ||
            (state->players[player].fkind != fkind))
        {
            continue;
        }
        if (ndsP4Fighter(ndsP4MatchContent(player)) == NULL)
        {
            return TRUE;
        }
        child = TRUE;
    }
    return (child != FALSE) ? FALSE : TRUE;
}

void ndsP4SetupFileSizes(u32 data_flags)
{
    u32 c;

    for (c = 1u; c < NDS_P4_CONTENT_LIMIT; c++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(c);
        u32 largest = 0u;

        if (f == NULL)
        {
            continue;
        }
        *f->data->p_file_main = NULL;
        /* A new scene: its tables are read again at first use. */
        f->data->mainmotion = NULL;
        sNdsP4Tables[c] = NULL;
        f->data->file_main_size = f->file_size->main;
        if ((data_flags & FTDATA_FLAG_MAINMOTION) &&
            (f->file_size->mainmotion_largest_anim > largest))
        {
            largest = f->file_size->mainmotion_largest_anim;
        }
        if ((data_flags & FTDATA_FLAG_SUBMOTION) &&
            (f->file_size->submotion_largest_anim > largest))
        {
            largest = f->file_size->submotion_largest_anim;
        }
        f->data->file_anim_size = largest;
        if (gFTManagerFigatreeHeapSize < largest)
        {
            gFTManagerFigatreeHeapSize = largest;
        }
    }
    /* The battle scene sizes each player's figatree heap from the kind it
     * passes, which for a P4 player is the parent (ftManagerAllocFigatreeHeapKind
     * has no player argument). Raise the parent's row to its largest selected
     * child for this battle only; ftManagerAllocFighter rewrote every legacy row
     * from the source census just before this runs. */
    if (gSCManagerBattleState != NULL)
    {
        s32 player;

        for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
        {
            const NDSP4Fighter *f = ndsP4Fighter(ndsP4MatchContent(player));
            FTData *parent;

            if ((f == NULL) ||
                (gSCManagerBattleState->players[player].pkind == nFTPlayerKindNot))
            {
                continue;
            }
            parent = dFTManagerDataFiles[f->parent_kind];
            if (parent->file_anim_size < f->data->file_anim_size)
            {
                parent->file_anim_size = f->data->file_anim_size;
            }
        }
    }
}

const NDSP4Computer *ndsP4Computer(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? f->computer : NULL;
}

void ndsP4ComputerRecover(FTStruct *fp)
{
    const NDSP4Computer *c = ndsP4Computer(fp);

    if ((c != NULL) && (c->recover != NULL))
    {
        c->recover(fp);
    }
}

/* Remix's added input routines (AI.asm add_cpu_input_routine), as
 * assembled: each content's attack list names its own (generated,
 * ComputerScriptBytes). These are the ones hand-ported code issues outside
 * a list: Falco's recovery steers with NSP_TOWARDS, NULL drops a command,
 * and Bowser's Whirling Fortress steers with POINT_STICK_TO_TARGET (custom
 * command 3). Their macros spell some steps oddly -- UNPRESS_A() is a Z
 * release and UNPRESS_Z() a move-toward-target step before one -- and the
 * bytes are what Remix's CPUs run. */
static const struct
{
    u8 nsp_towards[13];
    u8 null_routine[1];
    u8 point_stick_to_target[3];
} sNdsP4ComputerScripts = {
    { 0xC0, 0x50, 0x50, 0x30, 0xB0, 0x00, 0xA0, 0x7F, 0x21, 0xA0, 0x00, 0x30,
      0xFF },
    { 0xFF },
    { 0xFE, 0x03, 0xFF },
};

/* The content's generated routine for a Remix input id, or NULL. */
static const u8 *ndsP4ComputerOwnScript(const FTStruct *fp, s32 index)
{
    const NDSP4TablesHeader *t = ndsP4FighterTables(fp);
    const NDSP4ComputerScript *scripts;
    const u8 *bytes;
    u32 i;

    if (t == NULL)
    {
        return NULL;
    }
    scripts = (const NDSP4ComputerScript *)(const void *)((const u8 *)t + t->script_off);
    bytes = (const u8 *)t + t->script_bytes_off;
    for (i = 0u; i < t->script_count; i++)
    {
        if (scripts[i].input == (u32)index)
        {
            return &bytes[scripts[i].offset];
        }
    }
    return NULL;
}

static const u8 *ndsP4ComputerScript(const FTStruct *fp, s32 index)
{
    const u8 *own = ndsP4ComputerOwnScript(fp, index);

    if (own != NULL)
    {
        return own;
    }
    switch (index)
    {
    case nNDSP4ComputerInputNSPTowards:
        return sNdsP4ComputerScripts.nsp_towards;
    case nNDSP4ComputerInputPointStickToTarget:
        return sNdsP4ComputerScripts.point_stick_to_target;
    default:
        return sNdsP4ComputerScripts.null_routine;
    }
}

const FTComputerAttack *ndsP4ComputerAttacks(const FTStruct *fp)
{
    const NDSP4TablesHeader *t = ndsP4FighterTables(fp);

    return ((t != NULL) && (t->attack_count != 0u)) ?
        (const FTComputerAttack *)(const void *)((const u8 *)t + t->attack_off) : NULL;
}

u32 ndsP4ComputerLongRange(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? *f->computer_long_range :
                         NDS_P4_COMPUTER_LONG_RANGE_PARENT;
}

u32 ndsP4ComputerLongRangeMove(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? *f->computer_long_range_move : 0u;
}

u32 ndsP4Event32Commands(u32 content)
{
    const NDSP4Fighter *f = ndsP4Fighter(content);

    return (f != NULL) ? *f->event32_commands : 0u;
}

u32 ndsP4Event32MatAnims(u32 content, sb32 low_only)
{
    const NDSP4Fighter *f = ndsP4Fighter(content);

    return (f != NULL) ? f->event32_matanims[(low_only != FALSE) ? 0 : 1] : 0u;
}

u32 ndsP4Event32Allowance(u32 content)
{
    const NDSP4Fighter *f = ndsP4Fighter(content);

    return (f != NULL) ? *f->event32_allowance : 0u;
}

u32 ndsP4ComputerReflect(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? *f->computer_reflect : 0u;
}

s32 ndsP4ComputerReflectKind(const FTStruct *fp)
{
    if (ndsP4Content(fp) == 0u)
    {
        return fp->fkind;
    }
    if ((ndsP4ComputerReflect(fp) & NDS_P4_COMPUTER_REFLECT_NESS) != 0u)
    {
        return nFTKindNess;
    }
    return ((ndsP4ComputerReflect(fp) & NDS_P4_COMPUTER_REFLECT_FOX) != 0u) ?
        nFTKindFox : NDS_P4_FOREIGN_FKIND;
}

sb32 ndsP4ComputerAbsorbing(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));
    const NDSP4Computer *c = (f != NULL) ? f->computer : NULL;

    return ((c != NULL) && (fp->status_id >= c->absorb_first) &&
            (fp->status_id < c->absorb_end)) ? TRUE : FALSE;
}

sb32 ndsP4ComputerTargetReflects(const FTStruct *fp)
{
    if (ndsP4Content(fp) != 0u)
    {
        return ((ndsP4ComputerReflect(fp) & NDS_P4_COMPUTER_REFLECT_TABLE) != 0u) ?
            TRUE : FALSE;
    }
    return ((fp->fkind == nFTKindNess) || (fp->fkind == nFTKindFox)) ? TRUE : FALSE;
}

/* Remix extends the command table past the vanilla 0x31 (AI.asm
 * extend_commands_1/3); the vanilla setter keeps its wait and its random
 * draws, and the routine replaces the vanilla script it set. */
void ndsP4ComputerSetCommandWaitShort(FTStruct *fp, s32 index)
{
    if (index < NDS_P4_COMPUTER_INPUT_BASE)
    {
        ftComputerSetCommandWaitShort(fp, index);
        return;
    }
    ftComputerSetCommandWaitShort(fp, nFTComputerInputStickN);
    fp->computer.p_command = (u8 *)ndsP4ComputerScript(fp, index);
}

void ndsP4ComputerSetCommandImmediate(FTStruct *fp, s32 index)
{
    if (index < NDS_P4_COMPUTER_INPUT_BASE)
    {
        ftComputerSetCommandImmediate(fp, index);
        return;
    }
    ftComputerSetCommandImmediate(fp, nFTComputerInputStickN);
    fp->computer.p_command = (u8 *)ndsP4ComputerScript(fp, index);
}

/* Remix's CPU custom commands (AI.asm CUSTOM_COMMANDS): the opcode, then the
 * routine's index. */
#define NDS_P4_COMPUTER_COMMAND_CUSTOM 0xFE
enum
{
    NDS_P4_COMPUTER_CUSTOM_JUMPSQUAT_WAIT,
    NDS_P4_COMPUTER_CUSTOM_PRESS_C,
    NDS_P4_COMPUTER_CUSTOM_UNPRESS_C,
    NDS_P4_COMPUTER_CUSTOM_STICK_TO_TARGET,
    NDS_P4_COMPUTER_CUSTOM_TURNAROUND_WAIT
};
_Static_assert((nFTCommonStatusTurn == 0x12) && (nFTCommonStatusTurnRun == 0x13) &&
               (nFTCommonStatusKneeBend == 0x14), "Remix Action.Turn/TurnRun/JumpSquat");
_Static_assert((offsetof(FTStruct, input.cp.button_inputs) == 0x1C6) &&
               (offsetof(FTStruct, input.cp.stick_range) == 0x1C8),
               "Remix CPU button and stick offsets");

/* ftcomputer.c:3410 ftComputerUpdateInputs as a content's CPU runs it on the
 * N64: with Remix's two extensions, the stick-X values 0x81-0x84 (AI.asm
 * extend_stick_x_commands: away from the target at 80/40, forward/back from
 * the facing at 80) and the 0xFE custom commands (CUSTOM_COMMANDS: wait out
 * the jump squat or a turn, press or release C, point the stick at the
 * target), and with the Fox up-special test taking the content only where
 * Remix's usp_check_ does. The source's scripts carry neither extension, so
 * they run here exactly as in the source's interpreter. */
static void ndsP4ComputerRunScript(FTStruct *this_fp)
{
    FTComputer *com = &this_fp->computer;
    const NDSP4Computer *rows = ndsP4Computer(this_fp);
    sb32 is_fox_usp = (rows != NULL) && (rows->fox_usp_check != FALSE);
    u8 *p_command;
    u8 command;
    s8 var_t1 = 0;
    s16 stick_range_y;
    s16 stick_range_x;
    f32 dist_x;
    f32 dist_y;

    if (com->input_wait == 0)
    {
        return;
    }
    com->input_wait--;

    if (com->input_wait == 0)
    {
        p_command = com->p_command;

        while (com->input_wait == 0)
        {
            command = *p_command++;

            if (command < FTCOMPUTER_COMMAND_DEFAULT_MAX)
            {
                com->input_wait = command & FTCOMPUTER_COMMAND_TIMER_MASK;

                switch (command & FTCOMPUTER_COMMAND_OPCODE_MASK)
                {
                case FTCOMPUTER_COMMAND_BUTTON_A_PRESS:
                    this_fp->input.cp.button_inputs |= A_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_A_RELEASE:
                    this_fp->input.cp.button_inputs &= ~A_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_B_PRESS:
                    this_fp->input.cp.button_inputs |= B_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_B_RELEASE:
                    this_fp->input.cp.button_inputs &= ~B_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_Z_PRESS:
                    this_fp->input.cp.button_inputs |= Z_TRIG;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_Z_RELEASE:
                    this_fp->input.cp.button_inputs &= ~Z_TRIG;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_L_PRESS:
                    this_fp->input.cp.button_inputs |= L_TRIG;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_L_RELEASE:
                    this_fp->input.cp.button_inputs &= ~L_TRIG;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_START_PRESS:
                    this_fp->input.cp.button_inputs |= START_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_START_RELEASE:
                    this_fp->input.cp.button_inputs &= ~START_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_STICK_X_TILT:
                    switch (*p_command)
                    {
                    default:
                        this_fp->input.cp.stick_range.x = *p_command++;
                        break;

                    case FTCOMPUTER_STICK_AUTOFULL:
                        this_fp->input.cp.stick_range.x = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x < com->target_pos.x) ? (I_CONTROLLER_RANGE_MAX) : -(I_CONTROLLER_RANGE_MAX);
                        p_command++;
                        break;

                    case FTCOMPUTER_STICK_AUTOHALF:
                        this_fp->input.cp.stick_range.x = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x < com->target_pos.x) ? (I_CONTROLLER_RANGE_MAX / 2) : -(I_CONTROLLER_RANGE_MAX / 2);
                        p_command++;
                        break;

                    /* extend_stick_x_commands. */
                    case 0x81:
                        this_fp->input.cp.stick_range.x = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x < com->target_pos.x) ? -(I_CONTROLLER_RANGE_MAX) : (I_CONTROLLER_RANGE_MAX);
                        p_command++;
                        break;

                    case 0x82:
                        this_fp->input.cp.stick_range.x = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x < com->target_pos.x) ? -(I_CONTROLLER_RANGE_MAX / 2) : (I_CONTROLLER_RANGE_MAX / 2);
                        p_command++;
                        break;

                    case 0x83:
                        this_fp->input.cp.stick_range.x = (this_fp->lr >= 0) ? (I_CONTROLLER_RANGE_MAX) : -(I_CONTROLLER_RANGE_MAX);
                        p_command++;
                        break;

                    case 0x84:
                        this_fp->input.cp.stick_range.x = (this_fp->lr >= 0) ? -(I_CONTROLLER_RANGE_MAX) : (I_CONTROLLER_RANGE_MAX);
                        p_command++;
                        break;
                    }
                    break;

                case FTCOMPUTER_COMMAND_STICK_Y_TILT:
                    switch (*p_command)
                    {
                    default:
                        this_fp->input.cp.stick_range.y = *p_command++;
                        break;

                    case FTCOMPUTER_STICK_AUTOFULL:
                        this_fp->input.cp.stick_range.y = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y < com->target_pos.y) ? (I_CONTROLLER_RANGE_MAX) : -(I_CONTROLLER_RANGE_MAX);
                        p_command++;
                        break;

                    case FTCOMPUTER_STICK_AUTOHALF:
                        this_fp->input.cp.stick_range.y = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y < com->target_pos.y) ? (I_CONTROLLER_RANGE_MAX / 2) : -(I_CONTROLLER_RANGE_MAX / 2);
                        p_command++;
                        break;
                    }
                    break;

                case FTCOMPUTER_COMMAND_MOVEAUTO:
                    dist_x = com->target_pos.x - this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x;
                    dist_y = com->target_pos.y - this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y;

                    if ((this_fp->ga == nMPKineticsGround) && (this_fp->level < 5))
                    {
                        stick_range_x = (ABSF(dist_x) > 100.0F) ? (I_CONTROLLER_RANGE_MAX / 2) : 0;
                    }
                    else if (this_fp->ga == nMPKineticsGround)
                    {
                        if ((com->dash_predict * 1.5F) < ABSF(dist_x))
                        {
                            stick_range_x = (I_CONTROLLER_RANGE_MAX);
                        }
                        else
                        {
                            if (com->dash_predict < ABSF(dist_x))
                            {
                                stick_range_x = ((2.0F * ((ABSF(dist_x) - com->dash_predict) / com->dash_predict) * (F_CONTROLLER_RANGE_MAX / 2)) + (F_CONTROLLER_RANGE_MAX / 2));
                            }
                            else
                            {
                                stick_range_x = (ABSF(dist_x) > 100.0F) ? (I_CONTROLLER_RANGE_MAX / 2) : 0;
                            }
                        }
                    }
                    else
                    {
                        stick_range_x = ((ABSF(dist_x) > 100.0F) || ((this_fp->lr * dist_x) < 0.0F)) ? (I_CONTROLLER_RANGE_MAX) : (I_CONTROLLER_RANGE_MAX / 4);
                    }
                    stick_range_y = I_CONTROLLER_RANGE_MAX;

                    if (this_fp->ga == nMPKineticsGround)
                    {
                        if (this_fp->status_id != nFTCommonStatusKneeBend)
                        {
                            if (com->target_line_id == this_fp->coll_data.floor_line_id)
                            {
                                stick_range_y = dist_y = 0.0F;
                            }
                            if
                            (
                                (com->ftcom_flags_0x4A_b1) &&
                                (
                                    (ftGetComTargetFighter(com)->status_id == nFTCommonStatusCliffCatch) ||
                                    (ftGetComTargetFighter(com)->status_id == nFTCommonStatusCliffWait)
                                )
                            )
                            {
                                stick_range_y = dist_y = 0.0F;
                            }
                        }
                    }
                    else
                    {
                        /* The source's Fox test, as usp_check_ makes it. */
                        if
                        (
                            (
                                (is_fox_usp == FALSE) ||
                                (
                                    (this_fp->status_id != nFTFoxStatusSpecialHiStart) &&
                                    (this_fp->status_id != nFTFoxStatusSpecialAirHiStart) &&
                                    (this_fp->status_id != nFTFoxStatusSpecialHiHold) &&
                                    (this_fp->status_id != nFTFoxStatusSpecialAirHiHold)
                                )
                            )
                            &&
                            (dist_y < 0)
                        )
                        {
                            stick_range_y = dist_y = 0.0F;
                        }
                        switch (com->behavior)
                        {
                        case nFTComputerBehaviorYoshiTeam:
                            if (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y < 0)
                            {
                                stick_range_y = dist_y = 0.0F;
                            }
                            break;

                        case nFTComputerBehaviorKirbyTeam:
                        case nFTComputerBehaviorPolyTeam:
                            if (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y < -300.0F)
                            {
                                stick_range_y = dist_y = 0.0F;
                            }
                            break;
                        }
                    }
                    if ((dist_x != 0.0F) && (dist_y != 0.0F))
                    {
                        if (ABSF(dist_y) < ABSF(dist_x))
                        {
                            this_fp->input.cp.stick_range.x = (dist_x > 0.0F) ? stick_range_x : -stick_range_x;

                            this_fp->input.cp.stick_range.y = (ABSF((dist_y / dist_x)) * ((dist_y > 0.0F) ? stick_range_y : -stick_range_y));
                        }
                        else
                        {
                            this_fp->input.cp.stick_range.x = (ABSF((dist_x / dist_y)) * ((dist_x > 0.0F) ? stick_range_x : -stick_range_x));

                            this_fp->input.cp.stick_range.y = (dist_y > 0.0F) ? stick_range_y : -stick_range_y;
                        }
                    }
                    else if (dist_x != 0.0F)
                    {
                        this_fp->input.cp.stick_range.x = (dist_x > 0.0F) ? stick_range_x : -stick_range_x;

                        this_fp->input.cp.stick_range.y = (ABSF((dist_y / dist_x)) * ((dist_y > 0.0F) ? stick_range_y : -stick_range_y));
                    }
                    else if (dist_y != 0.0F)
                    {
                        this_fp->input.cp.stick_range.x = (ABSF((dist_x / dist_y)) * ((dist_x > 0.0F) ? stick_range_x : -stick_range_x));

                        this_fp->input.cp.stick_range.y = (dist_y > 0.0F) ? stick_range_y : -stick_range_y;
                    }
                    else
                    {
                        this_fp->input.cp.stick_range.x = this_fp->input.cp.stick_range.y = 0;
                    }
                    break;

                case FTCOMPUTER_COMMAND_STICK_X_VAR:
                    this_fp->input.cp.stick_range.x = var_t1;
                    break;

                case FTCOMPUTER_COMMAND_STICK_Y_VAR:
                    this_fp->input.cp.stick_range.y = var_t1;
                    break;
                }
            }
            else switch (command)
            {
            case FTCOMPUTER_COMMAND_DEFAULT_MAX + 0:
                com->input_wait = *p_command++;
                break;

            case FTCOMPUTER_COMMAND_DEFAULT_MAX + 1:
                var_t1 = 1;
                break;

            case FTCOMPUTER_COMMAND_DEFAULT_MAX + 2:
                com->input_wait = var_t1;
                break;

            case FTCOMPUTER_COMMAND_DEFAULT_MAX + 3:
                ftComputerSetControlPKThunder(this_fp);
                break;

            case NDS_P4_COMPUTER_COMMAND_CUSTOM:
                switch (*p_command++)
                {
                /* Hold this command until the jump squat (a turn) ends:
                 * one tick, then read it again. */
                case NDS_P4_COMPUTER_CUSTOM_JUMPSQUAT_WAIT:
                    if (this_fp->status_id == nFTCommonStatusKneeBend)
                    {
                        com->input_wait = 1;
                        p_command -= 2;
                    }
                    break;

                case NDS_P4_COMPUTER_CUSTOM_TURNAROUND_WAIT:
                    if ((this_fp->status_id == nFTCommonStatusTurn) ||
                        (this_fp->status_id == nFTCommonStatusTurnRun))
                    {
                        com->input_wait = 1;
                        p_command -= 2;
                    }
                    break;

                /* R_CBUTTONS; the release clears all four C buttons. */
                case NDS_P4_COMPUTER_CUSTOM_PRESS_C:
                    this_fp->input.cp.button_inputs |= 0x0001;
                    break;

                case NDS_P4_COMPUTER_CUSTOM_UNPRESS_C:
                    this_fp->input.cp.button_inputs &= 0xFFF0;
                    break;

                /* The stick at full range toward the target, rounded to
                 * nearest as the routine's cvt.w.s does. */
                case NDS_P4_COMPUTER_CUSTOM_STICK_TO_TARGET:
                {
                    const Vec3f *pos = this_fp->coll_data.p_translate;
                    f32 dx = com->target_pos.x - pos->x;
                    f32 dy = com->target_pos.y - pos->y;
                    f32 d2 = (dx * dx) + (dy * dy);
                    f32 magnitude = (d2 != 0.0F) ? sqrtf(d2) : 0.0F;

                    if (magnitude == 0.0F)
                    {
                        this_fp->input.cp.stick_range.x = 0;
                        this_fp->input.cp.stick_range.y = 0;
                    }
                    else
                    {
                        f32 scale = F_CONTROLLER_RANGE_MAX / magnitude;

                        this_fp->input.cp.stick_range.x = (s32)rintf(dx * scale);
                        this_fp->input.cp.stick_range.y = (s32)rintf(dy * scale);
                    }
                    break;
                }
                }
                break;

            case FTCOMPUTER_COMMAND_END:
                com->input_wait = 0;
                com->p_command = NULL;
                return;
            }
        }
        com->p_command = p_command;
    }
}

sb32 ndsP4ComputerRunInputs(FTStruct *this_fp)
{
    if (this_fp->nds_p4_content == 0u)
    {
        return FALSE;
    }
    ndsP4ComputerRunScript(this_fp);

    return TRUE;
}

/* AI.asm cpu_post_process (0x8013A884): its checks for every CPU, then the
 * character's row. Of the shared checks, only "a dash attack that cannot
 * start here is dropped" applies below level 10 without Remix's improved-AI
 * toggle, and only Remix routines issue a dash attack. */
void ndsP4ComputerPostProcess(FTStruct *fp)
{
    const NDSP4Computer *c = ndsP4Computer(fp);

    if (c == NULL)
    {
        return;
    }
    if ((fp->computer.p_command != NULL) &&
        (fp->computer.p_command ==
         ndsP4ComputerOwnScript(fp, nNDSP4ComputerInputDashAttack)) &&
        (fp->status_id != nFTCommonStatusWait) &&
        (fp->status_id != nFTCommonStatusRun))
    {
        ndsP4ComputerSetCommandImmediate(fp, nNDSP4ComputerInputNull);
    }
    if (c->post_process != NULL)
    {
        c->post_process(fp);
    }
}

extern const NDSP4SwordTrail gNdsP4KirbySwordTrails[];
extern const u32 gNdsP4KirbySwordTrailCount;

const NDSP4SwordTrail *ndsP4SwordTrail(const FTStruct *fp, u32 id)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));
    const NDSP4SwordTrail *rows;
    u32 count;
    u32 i;

    if (f != NULL)
    {
        rows = f->sword_trails;
        count = *f->sword_trail_count;
    }
    /* S8: the rows Kirby's power scripts select. */
    else if (fp->fkind == nFTKindKirby)
    {
        rows = gNdsP4KirbySwordTrails;
        count = gNdsP4KirbySwordTrailCount;
    }
    else
    {
        return NULL;
    }
    for (i = 0u; i < count; i++)
    {
        if (rows[i].id == id)
        {
            return &rows[i];
        }
    }
    return NULL;
}

void ndsP4UpdateSwordTrail(FTStruct *fp)
{
    const NDSP4SwordTrail *row =
        ndsP4SwordTrail(fp, (u32)fp->afterimage.is_itemswing);
    FTAfterImage *desc;
    FTParts *parts;
    DObj *joint;

    if (row == NULL)
    {
        return;
    }
    /* initial_setup_: the row's model part instead of Link's joint 11, then
     * the source's Link-sword step (ftmain.c, ftMainProcParams' afterimage
     * switch); axis_setup_: the row's matrix row instead of Z, or (axis 3)
     * the row's local direction through the joint's matrix. */
    joint = fp->joints[row->model_part + nFTPartsJointCommonStart];
    if ((joint == NULL) || (row->axis > 3u))
    {
        return;
    }
    parts = joint->user_data.p;
    func_ovl2_800EDBA4(joint);
    desc = &fp->afterimage.desc[fp->afterimage.desc_id];
    desc->translate_x = parts->mtx_translate[3][0];
    desc->translate_y = parts->mtx_translate[3][1];
    desc->translate_z = parts->mtx_translate[3][2];
    if (row->axis == 3u)
    {
        desc->vec.x = (row->local[0] * parts->mtx_translate[0][0]) +
                      (row->local[1] * parts->mtx_translate[1][0]) +
                      (row->local[2] * parts->mtx_translate[2][0]);
        desc->vec.y = (row->local[0] * parts->mtx_translate[0][1]) +
                      (row->local[1] * parts->mtx_translate[1][1]) +
                      (row->local[2] * parts->mtx_translate[2][1]);
        desc->vec.z = (row->local[0] * parts->mtx_translate[0][2]) +
                      (row->local[1] * parts->mtx_translate[1][2]) +
                      (row->local[2] * parts->mtx_translate[2][2]);
    }
    else
    {
        desc->vec.x = parts->mtx_translate[row->axis][0];
        desc->vec.y = parts->mtx_translate[row->axis][1];
        desc->vec.z = parts->mtx_translate[row->axis][2];
    }

    if (fp->afterimage.desc_id == 2)
    {
        fp->afterimage.desc_id = 0;
    }
    else fp->afterimage.desc_id++;

    if (fp->afterimage.drawstatus <= 2)
    {
        fp->afterimage.drawstatus++;
    }
}

void ndsP4OnSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    /* Remix Command.asm change_action_: every action change restores the
     * top-joint translation multiplier to 1.0 for that port and clears its
     * env colour override. */
    gNdsP4TranslationMultiplier[fp->player & 3u] = 1.0F;
    gNdsP4EnvColor[fp->player & 3u] = 0u;
}

static u8 sNdsP4SpecialStatusResolved[NDS_P4_CONTENT_LIMIT];
static FTStatusDesc *ndsP4KirbySpecialStatusDescs(const FTStruct *fp,
                                                  const FTStatusDesc *parent);

FTStatusDesc *ndsP4SpecialStatusDescs(const FTStruct *fp,
                                      const FTStatusDesc *parent)
{
    u32 content = ndsP4Content(fp);
    const NDSP4Fighter *f = ndsP4Fighter(content);
    u32 i;

    if (f == NULL)
    {
        return ndsP4KirbySpecialStatusDescs(fp, parent);
    }
    if (sNdsP4SpecialStatusResolved[content] == 0u)
    {
        /* Only statuses the parent has carry the marker: an appended status
         * names every routine. */
        for (i = 0u; i < *f->special_status_count; i++)
        {
            FTStatusDesc *row = &f->special_statuses[i];

#define NDS_P4_RESOLVE_PROC(proc_) \
            if (row->proc_ == NDS_P4_PROC_INHERIT) row->proc_ = parent[i].proc_
            NDS_P4_RESOLVE_PROC(proc_update);
            NDS_P4_RESOLVE_PROC(proc_interrupt);
            NDS_P4_RESOLVE_PROC(proc_physics);
            NDS_P4_RESOLVE_PROC(proc_map);
#undef NDS_P4_RESOLVE_PROC
        }
        sNdsP4SpecialStatusResolved[content] = 1u;
    }
    return f->special_statuses;
}

const NDSP4Entry *ndsP4Entry(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? f->entry : NULL;
}

const NDSP4Overrides *ndsP4Overrides(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? f->overrides : NULL;
}

sb32 ndsP4ColAnimEnd(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *ov = ndsP4Overrides(fp);

    if ((fp->nds_p4_content == 0u) && (fp->nds_p4_kirby_copy != 0u))
    {
        const NDSP4Fighter *f = ndsP4Fighter(fp->nds_p4_kirby_copy);

        if ((f == NULL) || (f->overrides == NULL) ||
            (f->overrides->kirby_colanim_end == NULL))
        {
            return FALSE;
        }
        f->overrides->kirby_colanim_end(fighter_gobj);
        return TRUE;
    }
    if ((ov == NULL) || (ov->colanim_end == NULL))
    {
        return FALSE;
    }
    ov->colanim_end(fighter_gobj);
    return TRUE;
}

void ndsP4ItemProc(GObj *item_gobj, s32 which)
{
    s32 player;

    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(ndsP4MatchContent(player));

        if ((f != NULL) && (f->overrides != NULL) && (f->overrides->item != NULL) &&
            (f->overrides->item(item_gobj, which) != FALSE))
        {
            return;
        }
    }
}

__attribute__((used)) volatile u32 gNdsP4EntryEffectMisses;

GObj *ndsP4EntryMakeEffect(struct EFDesc *desc, const Vec3f *pos)
{
    GObj *effect_gobj;

    if ((desc->file_head == NULL) || (*desc->file_head == NULL))
    {
        gNdsP4EntryEffectMisses++;
        return NULL;
    }
    effect_gobj = efManagerMakeEffectNoForce(desc);
    if (effect_gobj == NULL)
    {
        gNdsP4EntryEffectMisses++;
        return NULL;
    }
    DObjGetStruct(effect_gobj)->translate.vec.f = *pos;
    return effect_gobj;
}

static void ndsP4TablesHalt(u32 content)
{
    gNdsP4TablesFailure = content;
    for (;;)
    {
    }
}

const NDSP4TablesHeader *ndsP4LoadTables(u32 content)
{
    const NDSP4Fighter *f = ndsP4Fighter(content);
    NDSP4TablesHeader *t;
    char path[48];
    FILE *file;
    long bytes;

    if (f == NULL)
    {
        return NULL;
    }
    if ((sNdsP4Tables[content] != NULL) &&
        (sNdsP4TablesGeneration[content] == gNdsTaskmanHeapGeneration))
    {
        return sNdsP4Tables[content];
    }
    /* sniprintf: snprintf links newlib's floating-point formatter (about
     * 18 KB of resident code) for this one string. */
    sniprintf(path, sizeof(path), "nitro:/p4/%s.tab", sNdsP4TableNames[content]);
    ndsFsLock();
    file = fopen(path, "rb");
    if ((file == NULL) || (fseek(file, 0, SEEK_END) != 0) ||
        ((bytes = ftell(file)) < (long)sizeof(NDSP4TablesHeader)) ||
        (fseek(file, 0, SEEK_SET) != 0))
    {
        if (file != NULL)
        {
            fclose(file);
        }
        ndsFsUnlock();
        ndsP4TablesHalt(content);
    }
    /* Battle-lifetime: a VS battle's idle overlay tail first. */
    t = ndsVSBattleOverlayTryAlloc((size_t)bytes, 4u);
    if (t == NULL)
    {
        t = syTaskmanMalloc((size_t)bytes, 4);
    }
    if (fread(t, 1u, (size_t)bytes, file) != (size_t)bytes)
    {
        fclose(file);
        ndsFsUnlock();
        ndsP4TablesHalt(content);
    }
    fclose(file);
    ndsFsUnlock();
    if ((t->magic != NDS_P4_TABLES_MAGIC) || (t->version != NDS_P4_TABLES_VERSION) ||
        (t->bytes != (u32)bytes) ||
        (t->motion_off + (t->motion_count * sizeof(FTMotionDesc)) > t->bytes) ||
        ((s32)t->motion_count != f->data->mainmotion_array_count) ||
        (t->attack_off + (t->attack_count * sizeof(FTComputerAttack)) > t->bytes) ||
        (t->script_off + (t->script_count * sizeof(NDSP4ComputerScript)) > t->bytes) ||
        (t->script_bytes_off > t->bytes))
    {
        ndsP4TablesHalt(content);
    }
    sNdsP4Tables[content] = t;
    sNdsP4TablesGeneration[content] = gNdsTaskmanHeapGeneration;
    f->data->mainmotion = (FTMotionDescArray *)(void *)((u8 *)t + t->motion_off);

    return t;
}

/* The loaded tables of a fighter's content, NULL before its first use. */
static const NDSP4TablesHeader *ndsP4FighterTables(const FTStruct *fp)
{
    u32 content = ndsP4Content(fp);

    if ((content == 0u) || (content >= NDS_P4_CONTENT_LIMIT) ||
        (sNdsP4Tables[content] == NULL) ||
        (sNdsP4TablesGeneration[content] != gNdsTaskmanHeapGeneration))
    {
        return NULL;
    }
    return sNdsP4Tables[content];
}

const f32 *ndsP4YoshiEggRow(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? f->yoshi_egg : NULL;
}

const NDSP4Jab *ndsP4JabRows(const FTStruct *fp)
{
    static const NDSP4Jab none;
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return ((f != NULL) && (f->jab != NULL)) ? f->jab : &none;
}

#if NDS_MMARIO
static void ndsKirbyMetalAfterProcMap(FTStruct *fp);
#endif

volatile u32 gNdsP4EntryDeferLoads;
volatile u32 gNdsP4EntryDeferDeclines;

/* The FTData special-file id and global for slot 1-4. */
static void **ndsP4SpecialFileSlot(const NDSP4Fighter *f, u32 special, u32 *file_id)
{
    switch (special)
    {
    case 1u: *file_id = f->data->file_special1_id; return f->data->p_file_special1;
    case 2u: *file_id = f->data->file_special2_id; return f->data->p_file_special2;
    case 3u: *file_id = f->data->file_special3_id; return f->data->p_file_special3;
    case 4u: *file_id = f->data->file_special4_id; return f->data->p_file_special4;
    default: *file_id = 0u; return NULL;
    }
}

sb32 ndsP4DefersEntryDependency(u32 content, u32 dep_asset)
{
    const NDSP4Fighter *f = ndsP4Fighter(content);
    u32 file_id;

    if ((f == NULL) || (f->overrides == NULL) ||
        (f->overrides->entry_deferred_special == 0u) ||
        (ndsP4SpecialFileSlot(f, f->overrides->entry_deferred_special, &file_id) == NULL))
    {
        return FALSE;
    }
    return ((file_id != 0u) && (file_id == dep_asset)) ? TRUE : FALSE;
}

/* Weak: nds_renderer_native_common.c, in a build whose contents have
 * entry-only corners. */
void ndsRendererEntryP4RetireEntries(void **storage) __attribute__((weak));

void ndsP4EntryFileEnsure(FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(fp->nds_p4_content);
    void **slot;
    void *data;
    u32 file_id;
    u32 cache;

    if ((f == NULL) || (f->overrides == NULL) ||
        (f->overrides->entry_deferred_special == 0u))
    {
        return;
    }
    slot = ndsP4SpecialFileSlot(f, f->overrides->entry_deferred_special, &file_id);
    if ((slot == NULL) || (*slot != NULL))
    {
        return;
    }
    cache = ndsR2AnimCacheElasticBytes();
    data = ndsRelocLoadDeferredEntryFile(fp->nds_p4_content,
                                         NDS_P4_ENTRY_DEFER_KEEP_FREE +
                                         ((cache < NDS_P4_ENTRY_DEFER_CACHE_FLOOR) ?
                                              cache : NDS_P4_ENTRY_DEFER_CACHE_FLOOR));
    if (data == NULL)
    {
        gNdsP4EntryDeferDeclines++;
        /* The file's roots cannot draw this battle: when they are all of the
         * content's entry roots (Sonic's Tails plane), their corners go back
         * to the battle now. */
        if (ndsRendererEntryP4RetireEntries != NULL)
        {
            ndsRendererEntryP4RetireEntries(slot);
        }
        return;
    }
    *slot = data;
    gNdsP4EntryDeferLoads++;
    /* Sonic's Tails plane draws from the entry-effect packet: its lists
     * and vertices go back to the battle the moment the file is in. */
    ndsP4LendArticleDeadSpans();
}

void ndsP4AfterProcMapSlow(GObj *fighter_gobj)
{
    const FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o = ndsP4Overrides(fp);

    if ((o != NULL) && (o->after_proc_map != NULL))
    {
        o->after_proc_map(fighter_gobj);
    }
    else if ((fp->nds_p4_content == 0u) && (fp->nds_p4_kirby_copy != 0u))
    {
        const NDSP4Fighter *f = ndsP4Fighter(fp->nds_p4_kirby_copy);

        if ((f != NULL) && (f->overrides != NULL) &&
            (f->overrides->kirby_after_proc_map != NULL))
        {
            f->overrides->kirby_after_proc_map(fighter_gobj);
        }
    }
#if NDS_MMARIO
    if (fp->nds_kirby_metal != 0u)
    {
        ndsKirbyMetalAfterProcMap(ftGetStruct(fighter_gobj));
    }
#endif
}

void ndsP4OnDead(GObj *fighter_gobj)
{
    const NDSP4Overrides *o = ndsP4Overrides(ftGetStruct(fighter_gobj));

    if ((o != NULL) && (o->on_dead != NULL))
    {
        o->on_dead(fighter_gobj);
    }
}

void ndsP4OnHitInteractSlow(FTStruct *fp, s32 attack_type)
{
    const NDSP4Overrides *o = ndsP4Overrides(fp);

    if ((o != NULL) && (o->on_hit_interact != NULL))
    {
        o->on_hit_interact(fp, attack_type);
    }
    else if ((fp->nds_p4_content == 0u) && (fp->nds_p4_kirby_copy != 0u))
    {
        const NDSP4Fighter *f = ndsP4Fighter(fp->nds_p4_kirby_copy);

        if ((f != NULL) && (f->overrides != NULL) &&
            (f->overrides->kirby_on_hit_interact != NULL))
        {
            f->overrides->kirby_on_hit_interact(fp, attack_type);
        }
    }
}

sb32 ndsP4GravitySlow(FTStruct *fp)
{
    const NDSP4Overrides *o = ndsP4Overrides(fp);

    return ((o != NULL) && (o->gravity != NULL)) ? o->gravity(fp) : FALSE;
}

sb32 ndsP4AirJumpCheck(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return FALSE;
    }
    o = ndsP4Overrides(fp);
    return ((o != NULL) && (o->air_jump_check != NULL)) ? o->air_jump_check(fighter_gobj)
                                                        : FALSE;
}

void ndsP4FallSetStatus(GObj *fighter_gobj, s32 status_id, f32 frame_begin,
                        f32 anim_speed, u32 flags)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->nds_p4_content != 0u)
    {
        const NDSP4Overrides *o = ndsP4Overrides(fp);

        if ((o != NULL) && (o->fall_status != NULL) && (o->fall_status(fighter_gobj) != FALSE))
        {
            return;
        }
    }
    ftMainSetStatus(fighter_gobj, status_id, frame_begin, anim_speed, flags);
}

sb32 ndsP4AirLightThrowCheck(FTStruct *fp)
{
    if (fp->nds_p4_content != 0u)
    {
        const NDSP4Overrides *o = ndsP4Overrides(fp);

        if ((o != NULL) && (o->air_item_throw_block != NULL) &&
            (o->air_item_throw_block(fp) != FALSE))
        {
            return FALSE;
        }
    }
    return ftCommonLightThrowCheckItemTypeThrow(fp);
}

void ndsP4OnDamage(FTStruct *fp)
{
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->on_damage != NULL))
    {
        o->on_damage(fp);
    }
}

void ndsP4OnLanding(FTStruct *fp, sb32 cliff)
{
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        /* S8: Kirby's grounded_script row, run for the power he holds. */
        if (fp->nds_p4_kirby_copy != 0u)
        {
            const NDSP4Fighter *f = ndsP4Fighter(fp->nds_p4_kirby_copy);

            if ((f != NULL) && (f->overrides != NULL) &&
                (f->overrides->kirby_on_landing != NULL))
            {
                f->overrides->kirby_on_landing(fp);
            }
        }
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->on_landing != NULL))
    {
        o->on_landing(fp, cliff);
    }
}

void ndsP4BeforePhysicsMapSlow(GObj *fighter_gobj)
{
    const NDSP4Overrides *o = ndsP4Overrides(ftGetStruct(fighter_gobj));

    if ((o != NULL) && (o->before_physics_map != NULL))
    {
        o->before_physics_map(fighter_gobj);
    }
}

sb32 ndsP4CatchDamageCollide(FTStruct *victim_fp, FTAttackColl *attack_coll,
                             FTDamageColl *damage_coll)
{
    if (victim_fp->nds_p4_content != 0u)
    {
        const NDSP4Overrides *o = ndsP4Overrides(victim_fp);

        if ((o != NULL) && (o->grabbable != NULL) &&
            (o->grabbable(victim_fp, (s32)(damage_coll - victim_fp->damage_colls)) == FALSE))
        {
            return FALSE;
        }
    }
    return gmCollisionCheckFighterAttackDamageCollide(attack_coll, damage_coll);
}

void ndsP4AttackAirStart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if ((fp->nds_p4_content == 0u) || (fp->status_id != nFTCommonStatusAttackAirLw))
    {
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->attack_air_lw_hit != NULL))
    {
        fp->proc_hit = o->attack_air_lw_hit;
    }
}

s32 ndsP4DamageStatus(GObj *fighter_gobj, s32 status_id, s32 *status_id_after)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return status_id;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->damage_status != NULL))
    {
        return o->damage_status(fp, status_id, status_id_after);
    }
    return status_id;
}

sb32 ndsP4AirSpecialBlocked(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return FALSE;
    }
    o = ndsP4Overrides(fp);
    return ((o != NULL) && (o->air_special_block != NULL)) ? o->air_special_block(fp) : FALSE;
}

const f32 *ndsP4MultiJumpVelocities(const FTStruct *fp)
{
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return NULL;
    }
    o = ndsP4Overrides(fp);
    return (o != NULL) ? o->multi_jump_vel : NULL;
}

void ndsP4OnInitFighter(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    /* S8: a new stock starts with the source's copy_kind (none), and no
     * content power with it. */
    fp->nds_p4_kirby_copy = 0u;
    fp->nds_p4_kirby_hat = 0u;
    if (fp->nds_p4_content == 0u)
    {
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->on_init != NULL))
    {
        o->on_init(fp);
    }
}

s32 ndsP4KirbyInhaleStatus(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        /* S8: a Kirby holding Dedede's power inhales on his DEDEDE_NSP
         * statuses. */
        if (fp->nds_p4_kirby_copy != 0u)
        {
            const NDSP4Fighter *f = ndsP4Fighter(fp->nds_p4_kirby_copy);

            if ((f != NULL) && (f->overrides != NULL) &&
                (f->overrides->kirby_copy_inhale_status != NULL))
            {
                return f->overrides->kirby_copy_inhale_status(status_id);
            }
        }
        return status_id;
    }
    o = ndsP4Overrides(fp);
    return ((o != NULL) && (o->kirby_inhale_status != NULL)) ? o->kirby_inhale_status(status_id)
                                                             : status_id;
}

/* relocData/228_KirbyMainMotion.c's FTKirbyCopy[27], the table Kirby's
 * inhale reads from his main motion file; Remix's kirby_inhale_struct holds
 * the same rows for the original fighters. The inhale a content runs reads
 * it here, Kirby's file or not in the match (a pointer no loaded file owns,
 * so lbRelocGetFileData answers it as is). */
static const FTKirbyCopy sNdsP4KirbyCopyTable[27] = {
    { nFTKindMario, 12, 1.5F, 17 },
    { nFTKindFox, 7, 1.5F, 17 },
    { nFTKindDonkey, 4, 2.0F, 30 },
    { nFTKindSamus, 8, 1.6F, 17 },
    { nFTKindLuigi, 11, 1.6F, 17 },
    { nFTKindLink, 10, 1.5F, 17 },
    { nFTKindYoshi, 5, 1.7F, 25 },
    { nFTKindCaptain, 9, 1.7F, 17 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindPikachu, 6, 1.5F, 17 },
    { nFTKindPurin, 3, 1.6F, 17 },
    { nFTKindNess, 13, 1.6F, 17 },
    { nFTKindKirby, 0, 1.0F, 17 },
    { nFTKindKirby, 0, 1.5F, 17 },
    { nFTKindKirby, 0, 1.5F, 17 },
    { nFTKindKirby, 0, 1.5F, 17 },
    { nFTKindKirby, 0, 2.0F, 30 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindKirby, 0, 1.5F, 17 },
    { nFTKindKirby, 0, 1.7F, 17 },
    { nFTKindKirby, 0, 1.7F, 17 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindKirby, 0, 1.5F, 17 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindDonkey, 4, 2.0F, 50 },
};

void *ndsP4KirbyCopyFile(void)
{
    return (void *)sNdsP4KirbyCopyTable;
}

/* ---- S8: Kirby's copies of the contents (nds_p4.h NDSP4KirbyPower) ---- */

extern FTStatusDesc gNdsP4KirbySpecialStatusDescs[];
extern const u32 gNdsP4KirbySpecialStatusCount;
extern const NDSP4KirbyClone gNdsP4KirbyClones[];
extern const NDSP4KirbyPower gNdsP4KirbyPowers[NDS_P4_CONTENT_LIMIT];
extern const u16 gNdsP4KirbyRelocAssets[];
extern const u32 gNdsP4KirbyRelocAssetCount;
extern const u16 gNdsP4KirbyAnims[];
extern const u32 gNdsP4KirbyAnimCount;
extern FTData dFTKirbyData;
extern FTMotionDesc dFTKirbyMotionDescs[];

#define NDS_P4_KIRBY_SOURCE_SPECIALS 83u /* 0xDC..0x12E */
#define NDS_P4_KIRBY_FIRST_CUSTOM_HAT 0xF  /* KirbyHats.asm: ids past the source's */
#define NDS_P4_KIRBY_HAT_JOINT 6           /* FTKIRBY_COPY_MODELPARTS_JOINT */

_Static_assert(NDS_P4_KIRBY_STATUS_FIRST ==
               nFTCommonStatusSpecialStart + NDS_P4_KIRBY_SOURCE_SPECIALS,
               "Kirby's power statuses follow his source specials");

/* The match's Kirby FTData: the source's, with the power motion rows after
 * his own and the script file standing as his submotion file (Kirby has
 * none; the power rows carry the submotion-script flag, so ftMainSetStatus
 * reads their scripts there). One per heap generation, shared by every
 * Kirby of the match as they share the source's. */
static FTData sNdsP4KirbyData;
static void *sNdsP4KirbyScriptFile;
static FTMotionDesc *sNdsP4KirbyMotions;
static FTStatusDesc *sNdsP4KirbyStatusDescs;
static u32 sNdsP4KirbyGeneration = 0xFFFFFFFFu;
static u32 sNdsP4KirbyStatusGeneration = 0xFFFFFFFFu;
/* The power the last catch took, by the catching player. */
static u8 sNdsP4KirbyPending[GMCOMMON_PLAYERS_MAX];

/* The content's own files (its closure) include the file. */
static sb32 ndsP4ContentHasAsset(u32 content, u32 file_id)
{
    const NDSP4Fighter *f = ndsP4Fighter(content);
    u32 i;

    for (i = 0u; (f != NULL) && (file_id != 0u) && (i < *f->asset_count); i++)
    {
        if (f->assets[i] == file_id)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static const NDSP4KirbyPower *ndsP4KirbyPower(u32 content)
{
    if ((content == 0u) || (content >= NDS_P4_CONTENT_LIMIT) ||
        (ndsP4Fighter(content) == NULL) || (gNdsP4KirbyPowers[content].hat_id == 0u))
    {
        return NULL;
    }
    return &gNdsP4KirbyPowers[content];
}

static sb32 ndsP4KirbyMatchLoaded(void)
{
    return (sNdsP4KirbyGeneration == gNdsTaskmanHeapGeneration) ? TRUE : FALSE;
}

/* nitro:/p4/kirby.tab's rows after Kirby's own, in one heap block. */
static FTMotionDesc *ndsP4KirbyLoadMotions(void)
{
    NDSP4TablesHeader header;
    FTMotionDesc *motions;
    FILE *file;
    size_t own = (size_t)NDS_P4_KIRBY_MOTION_FIRST * sizeof(FTMotionDesc);
    size_t rows = (size_t)NDS_P4_KIRBY_MOTION_COUNT * sizeof(FTMotionDesc);

    ndsFsLock();
    file = fopen("nitro:/p4/kirby.tab", "rb");
    if ((file == NULL) || (fread(&header, 1u, sizeof(header), file) != sizeof(header)) ||
        (header.magic != NDS_P4_TABLES_MAGIC) || (header.version != NDS_P4_TABLES_VERSION) ||
        (header.motion_count != NDS_P4_KIRBY_MOTION_COUNT) ||
        (header.motion_off + rows > header.bytes) ||
        (fseek(file, (long)header.motion_off, SEEK_SET) != 0))
    {
        if (file != NULL)
        {
            fclose(file);
        }
        ndsFsUnlock();
        ndsP4TablesHalt(NDS_P4_CONTENT_LIMIT);
    }
    motions = syTaskmanMalloc(own + rows, 4);
    memcpy(motions, dFTKirbyMotionDescs, own);
    if (fread(motions + NDS_P4_KIRBY_MOTION_FIRST, 1u, rows, file) != rows)
    {
        fclose(file);
        ndsFsUnlock();
        ndsP4TablesHalt(NDS_P4_CONTENT_LIMIT);
    }
    fclose(file);
    ndsFsUnlock();
    return motions;
}

FTData *ndsP4KirbyMatchData(void)
{
    const void *id = (const void *)(uintptr_t)NDS_P4_KIRBY_SCRIPT_FILE;
    sb32 needed = FALSE;
    s32 player;

    if (gSCManagerSceneData.scene_curr != nSCKindVSBattle)
    {
        return NULL;
    }
    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        if (ndsP4KirbyPower(ndsP4MatchContent(player)) != NULL)
        {
            needed = TRUE;
        }
    }
    if (needed == FALSE)
    {
        return NULL;
    }
    if (ndsP4KirbyMatchLoaded() == FALSE)
    {
        sNdsP4KirbyMotions = ndsP4KirbyLoadMotions();
        sNdsP4KirbyScriptFile = lbRelocGetExternHeapFile(
            id, syTaskmanMalloc(lbRelocGetFileSize(id), 0x10));
        /* The powers' Remix hat textures (their images read them as foreign
         * images, p4_kirby_hats.py) and the weapons they hold on joint 17
         * (sidecar roots; Wolf's is his own gun, loaded with his files). One
         * load per file. */
        {
            u16 loaded[2 * GMCOMMON_PLAYERS_MAX] = { 0u };
            u32 n = 0u;

            for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
            {
                u32 content = ndsP4MatchContent(player);
                const NDSP4KirbyPower *p = ndsP4KirbyPower(content);
                u32 files[2];
                u32 k;

                if (p == NULL)
                {
                    continue;
                }
                files[0] = p->hat_texture_file;
                files[1] = (ndsP4ContentHasAsset(content, p->weapon_file) == FALSE) ?
                    p->weapon_file : 0u;
                for (k = 0u; k < 2u; k++)
                {
                    u32 i;

                    for (i = 0u; (i < n) && (loaded[i] != files[k]); i++)
                    {
                    }
                    if ((files[k] != 0u) && (i == n))
                    {
                        const void *file = (const void *)(uintptr_t)files[k];

                        loaded[n++] = (u16)files[k];
                        (void)lbRelocGetExternHeapFile(
                            file, syTaskmanMalloc(lbRelocGetFileSize(file), 0x10));
                    }
                }
            }
        }
        sNdsP4KirbyGeneration = gNdsTaskmanHeapGeneration;
    }
    sNdsP4KirbyData = dFTKirbyData;
    sNdsP4KirbyData.mainmotion = (FTMotionDescArray *)(void *)sNdsP4KirbyMotions;
    sNdsP4KirbyData.mainmotion_array_count =
        NDS_P4_KIRBY_MOTION_FIRST + NDS_P4_KIRBY_MOTION_COUNT;
    sNdsP4KirbyData.p_file_submotion = &sNdsP4KirbyScriptFile;
    return &sNdsP4KirbyData;
}

/* The source table's rows, then the powers'. Built at the first status a
 * match's Kirby takes (his entry, at construction). */
static FTStatusDesc *ndsP4KirbySpecialStatusDescs(const FTStruct *fp,
                                                  const FTStatusDesc *parent)
{
    if ((fp->data != &sNdsP4KirbyData) || (ndsP4KirbyMatchLoaded() == FALSE))
    {
        return NULL;
    }
    if (sNdsP4KirbyStatusGeneration != gNdsTaskmanHeapGeneration)
    {
        size_t own = NDS_P4_KIRBY_SOURCE_SPECIALS * sizeof(FTStatusDesc);
        size_t rows = gNdsP4KirbySpecialStatusCount * sizeof(FTStatusDesc);

        sNdsP4KirbyStatusDescs = syTaskmanMalloc(own + rows, 4);
        memcpy(sNdsP4KirbyStatusDescs, parent, own);
        memcpy(sNdsP4KirbyStatusDescs + NDS_P4_KIRBY_SOURCE_SPECIALS,
               gNdsP4KirbySpecialStatusDescs, rows);
        sNdsP4KirbyStatusGeneration = gNdsTaskmanHeapGeneration;
    }
    return sNdsP4KirbyStatusDescs;
}

s32 ndsP4KirbyCloneStatus(FTStruct *fp, s32 status_id)
{
    const NDSP4KirbyPower *p = ndsP4KirbyPower(fp->nds_p4_kirby_copy);
    u32 i;

    if ((p == NULL) || (fp->fkind != nFTKindKirby))
    {
        return status_id;
    }
    for (i = 0u; i < p->clone_count; i++)
    {
        const NDSP4KirbyClone *c = &gNdsP4KirbyClones[p->clone_first + i];

        if (c->status == status_id)
        {
            return c->clone;
        }
    }
    return status_id;
}

void ndsP4KirbyAfterSetStatus(GObj *fighter_gobj, s32 status_id)
{
    const NDSP4Fighter *f = ndsP4Fighter(ftGetStruct(fighter_gobj)->nds_p4_kirby_copy);

    if ((f != NULL) && (f->overrides != NULL) && (f->overrides->kirby_on_status != NULL))
    {
        f->overrides->kirby_on_status(fighter_gobj, status_id);
    }
}

/* A content its source will not let Kirby copy: EXTRA's define_character
 * takes bool_inhale_copy from the config's kirby_hat (FALSE unless set),
 * and the inhale row then holds Kirby's own id and no hat. These are the
 * contents with no power (p4_kirby.py checks their exports). */
static sb32 ndsP4KirbyNoCopy(u32 content)
{
    return ((content != 0u) && (ndsP4Fighter(content) != NULL) &&
            (ndsP4KirbyPower(content) == NULL)) ? TRUE : FALSE;
}

sb32 ndsP4KirbyPlayerNoCopy(s32 player)
{
    return ndsP4KirbyNoCopy(ndsP4MatchContent(player));
}

#if NDS_MMARIO
/* Owner 2026-10-18 ("Metal Mario Kirby hat (metal skin overlay similar to
 * mrgaw hat and gravity like metal Mario)"): a Kirby who swallows Metal Mario
 * takes Mario's power, as the source's copy row gives him, and Metal Mario's
 * metal with it until he loses the copy -- his skin a metal grey through the
 * colour animation's blend (MRGAW's black figure is the same field at full
 * strength) and Metal Mario's gravity and fall speeds. A Kirby who swallows
 * such a Kirby takes the metal on with the copy, as he takes the copy_id.
 *
 * The gravity is a copy of the Kirby's attributes with Metal Mario's three
 * fall values in it (every reader of attr->gravity, tvel_base and tvel_fast
 * sees them: the source's physics, his specials, the CPU's jump arcs), taken
 * from the battle's asset storage once a match per player; when no copy fits
 * the metal skin still holds and the fall stays Kirby's. */
/* The colour animation's blend replaces his material's diffuse and ambient
 * colours (full strength), the hardware still lights them by his normals, and
 * the DS multiplies his texture by the lit colour (no N64 fog blend): a dim
 * cyan-grey cancels his pink, ~70,70,81 in shadow to ~139,141,162 in full
 * light, a blue-steel grey with the lighting's contrast. MRGAW's black figure
 * is the same field with black. */
#define NDS_KIRBY_METAL_R 0x4Au
#define NDS_KIRBY_METAL_G 0x70u
#define NDS_KIRBY_METAL_B 0x78u
#define NDS_KIRBY_METAL_A 0xFFu
/* The heap the copy may leave: the battle's low-water floor. */
#define NDS_KIRBY_METAL_KEEP_FREE 25600u

static u8 sNdsKirbyMetalPending[4];
static f32 sNdsKirbyMetalFall[4][3];
static FTAttributes *sNdsKirbyMetalAttr[4];
static FTAttributes *sNdsKirbyMetalOwnAttr[4];

void ndsKirbyMetalResetMatch(void)
{
    u32 i;

    for (i = 0u; i < 4u; i++)
    {
        sNdsKirbyMetalPending[i] = 0u;
        sNdsKirbyMetalAttr[i] = NULL;
        sNdsKirbyMetalOwnAttr[i] = NULL;
    }
}

static void ndsKirbyMetalCatch(FTStruct *kirby_fp, FTStruct *victim_fp)
{
    u32 player = kirby_fp->player & 3u;
    sb32 metal = FALSE;

    if ((kirby_fp->fkind == nFTKindKirby) && (victim_fp->attr != NULL))
    {
        if (victim_fp->fkind == nFTKindMMario)
        {
            metal = TRUE;
        }
        else if (((victim_fp->fkind == nFTKindKirby) ||
                  (victim_fp->fkind == nFTKindNKirby)) &&
                 (victim_fp->nds_kirby_metal != 0u))
        {
            metal = TRUE;
        }
    }
    sNdsKirbyMetalPending[player] = (u8)metal;
    if (metal != FALSE)
    {
        /* The source's copy row gives Metal Mario none (Kirby's own id):
         * he takes Mario's power and cap, in metal (owner: "Metal Mario
         * Kirby hat"). */
        if (victim_fp->fkind == nFTKindMMario)
        {
            kirby_fp->status_vars.kirby.specialn.copy_id = nFTKindMario;
        }
        sNdsKirbyMetalFall[player][0] = victim_fp->attr->gravity;
        sNdsKirbyMetalFall[player][1] = victim_fp->attr->tvel_base;
        sNdsKirbyMetalFall[player][2] = victim_fp->attr->tvel_fast;
    }
}

static sb32 ndsKirbyMetalIsSkin(const GMColAnim *colanim)
{
    return ((colanim->is_use_color1 != FALSE) && (colanim->cs[0].p_script == NULL) &&
            (colanim->color1.r == NDS_KIRBY_METAL_R) &&
            (colanim->color1.g == NDS_KIRBY_METAL_G) &&
            (colanim->color1.b == NDS_KIRBY_METAL_B) &&
            (colanim->color1.a == NDS_KIRBY_METAL_A)) ? TRUE : FALSE;
}

static void ndsKirbyMetalClear(FTStruct *fp)
{
    u32 player = fp->player & 3u;

    if (fp->nds_kirby_metal == 0u)
    {
        return;
    }
    fp->nds_kirby_metal = 0u;
    if ((sNdsKirbyMetalAttr[player] != NULL) &&
        (fp->attr == sNdsKirbyMetalAttr[player]) &&
        (sNdsKirbyMetalOwnAttr[player] != NULL))
    {
        fp->attr = sNdsKirbyMetalOwnAttr[player];
    }
    if (ndsKirbyMetalIsSkin(&fp->colanim) != FALSE)
    {
        ftParamResetColAnim(&fp->colanim);
    }
}

/* ftKirbySpecialNCopyInitCopyVars with a copy taken (battleship_kirby.c):
 * the metal is what the catch saw, whether or not the copy_id changed (a
 * Kirby with Mario's power who swallows Metal Mario turns metal; one with
 * the metal who swallows Mario loses it). */
void ndsKirbyMetalTake(FTStruct *fp)
{
    u32 player = fp->player & 3u;
    FTAttributes *metal;

    if (sNdsKirbyMetalPending[player] == 0u)
    {
        ndsKirbyMetalClear(fp);
        return;
    }
    sNdsKirbyMetalPending[player] = 0u;
    metal = sNdsKirbyMetalAttr[player];
    if ((metal == NULL) && (fp->attr != NULL))
    {
        metal = (FTAttributes *)ndsSceneAssetTryAlloc(sizeof(FTAttributes), 8u,
                                                      NDS_KIRBY_METAL_KEEP_FREE);
        sNdsKirbyMetalAttr[player] = metal;
    }
    if ((metal != NULL) && (fp->attr != NULL))
    {
        if (fp->attr != metal)
        {
            sNdsKirbyMetalOwnAttr[player] = fp->attr;
            __builtin_memcpy(metal, fp->attr, sizeof(FTAttributes));
        }
        metal->gravity = sNdsKirbyMetalFall[player][0];
        metal->tvel_base = sNdsKirbyMetalFall[player][1];
        metal->tvel_fast = sNdsKirbyMetalFall[player][2];
        fp->attr = metal;
    }
    fp->nds_kirby_metal = 1u;
}

/* Every frame he holds the metal (after the frame's colour animation update,
 * before he is drawn): the metal grey over his skin while no colour
 * animation runs -- a hit flash, an item's or a status's colour shows
 * through, and the metal comes back when it ends. */
static void ndsKirbyMetalAfterProcMap(FTStruct *fp)
{
    GMColAnim *colanim = &fp->colanim;

    if ((colanim->cs[0].p_script == NULL) && (colanim->cs[1].p_script == NULL))
    {
        colanim->is_use_color1 = TRUE;
        colanim->color1.r = NDS_KIRBY_METAL_R;
        colanim->color1.g = NDS_KIRBY_METAL_G;
        colanim->color1.b = NDS_KIRBY_METAL_B;
        colanim->color1.a = NDS_KIRBY_METAL_A;
        colanim->color1.ir = colanim->color1.ig = colanim->color1.ib =
            colanim->color1.ia = 0;
    }
}
#endif

/* The catch takes the victim content's power, or the one a victim Kirby
 * holds (ftKirbySpecialNCatchProcUpdate: a Kirby passes on his copy_id).
 * Only a Kirby takes one: Polygon Kirby keeps the parent's copy, as Remix's
 * swaps and hats test Kirby's id. A content with no copy gives any catcher
 * Kirby's own id, as its inhale row does: the copy is none (a held power
 * goes, as swallowing a Kirby without one does), not the parent's. */
void ndsP4KirbyCatch(FTStruct *kirby_fp, FTStruct *victim_fp)
{
    u32 power = 0u;

    if ((victim_fp->fkind != nFTKindKirby) && (victim_fp->fkind != nFTKindNKirby) &&
        (ndsP4KirbyNoCopy(victim_fp->nds_p4_content) != FALSE))
    {
        kirby_fp->status_vars.kirby.specialn.copy_id = nFTKindKirby;
    }
    if (kirby_fp->fkind == nFTKindKirby)
    {
        if ((victim_fp->fkind == nFTKindKirby) || (victim_fp->fkind == nFTKindNKirby))
        {
            power = victim_fp->nds_p4_kirby_copy;
        }
        else if (ndsP4KirbyPower(victim_fp->nds_p4_content) != NULL)
        {
            power = victim_fp->nds_p4_content;
        }
    }
    sNdsP4KirbyPending[kirby_fp->player & 3u] = (u8)power;
#if NDS_MMARIO
    ndsKirbyMetalCatch(kirby_fp, victim_fp);
#endif
}

u32 ndsP4KirbyPendingPower(const FTStruct *fp)
{
    return sNdsP4KirbyPending[fp->player & 3u];
}

const NDSP4KirbyPower *ndsP4KirbyPowerOf(u32 content)
{
    return ndsP4KirbyPower(content);
}

/* The black figure a Kirby holding MRGAW's custom power is drawn as
 * (nds_p4_mrgaw.c ndsP4MrgawKirbyOnStatus) goes with the power: a colour
 * animation still holding that full blend to black, with no script, ends. */
void ndsP4KirbyClearSilhouette(FTStruct *fp)
{
#if defined(NDS_P4_KIRBY_SILHOUETTE_CONTENT)
    GMColAnim *colanim = &fp->colanim;

    if ((fp->nds_p4_kirby_copy == NDS_P4_KIRBY_SILHOUETTE_CONTENT) &&
        (colanim->is_use_color1 != FALSE) && (colanim->cs[0].p_script == NULL) &&
        (colanim->color1.r == 0) && (colanim->color1.g == 0) &&
        (colanim->color1.b == 0) && (colanim->color1.a == 0xFF))
    {
        ftParamResetColAnim(colanim);
    }
#else
    (void)fp;
#endif
}

void ndsP4KirbySetMotionScript(FTStruct *fp, const u32 *script)
{
    s32 i;

    for (i = 0; i < ARRAY_COUNT(fp->motion_scripts); i++)
    {
        fp->motion_scripts[i][0].p_script = (u32 *)script;
        fp->motion_scripts[i][0].script_wait = 0.0F;
        fp->motion_scripts[i][0].script_id = 0;
        fp->motion_scripts[i][1].p_script = NULL;
    }
}

void ndsP4KirbyLoseCopy(FTStruct *fp)
{
    ndsP4KirbyClearSilhouette(fp);
#if NDS_MMARIO
    ndsKirbyMetalClear(fp);
#endif
    fp->nds_p4_kirby_copy = 0u;
    fp->nds_p4_kirby_hat = 0u;
}

/* A content player's hat bits for the match's Kirby hat admission
 * (nds_kirby_hat_residency.c): the power's hats, 0 for any other player. */
u32 ndsP4KirbyHatMask(s32 player)
{
    const NDSP4KirbyPower *p = ndsP4KirbyPower(ndsP4MatchContent(player));

    return (p != NULL) ? p->hat_mask : 0u;
}

extern const u8 gNdsP4KirbyHatBases[];
extern const u32 gNdsP4KirbyHatBaseCount;

/* The source hat a Remix hat re-skins, -1 for an id that is none. */
static s32 ndsP4KirbyHatBase(u32 hat)
{
    u32 index = hat - (u32)NDS_P4_KIRBY_FIRST_CUSTOM_HAT;

    return ((hat >= (u32)NDS_P4_KIRBY_FIRST_CUSTOM_HAT) && (index < gNdsP4KirbyHatBaseCount)) ?
        (s32)gNdsP4KirbyHatBases[index] : -1;
}

static sb32 ndsP4KirbyPowerDrawsHat(const NDSP4KirbyPower *p, u32 hat)
{
    u32 i;

    for (i = 0u; (p != NULL) && (i < p->hat_count); i++)
    {
        if (p->hats[i] == hat)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void ndsP4KirbyBindHat(FTStruct *fp)
{
    /* The slot the draw looks the hat up by (ftParamSetModelPartDefaultID's
     * rule: a battle fighter's player, recorded before nds_slot is). */
    u32 slot = (gNdsSceneManagerCurrIsBattle != 0u) ? (u32)fp->player : (u32)fp->nds_slot;

    if (fp->nds_p4_kirby_hat == 0u)
    {
        return;
    }
    /* The match admits one detail (ndsKirbyHatPrepareMatch). */
    if ((ndsRendererNativeEnsureKirbyP4Hat(slot, fp->nds_p4_kirby_hat, 0u) |
         ndsRendererNativeEnsureKirbyP4Hat(slot, fp->nds_p4_kirby_hat, 1u)) == FALSE)
    {
        NDS_DIAG(gNdsKirbyHatMissingCount++);
    }
}

/* Joint 6 of a Kirby holding a power: a Remix hat id stands on the source hat
 * it re-skins (the live part) and is recorded; the power's own source hat id
 * (ResetModelPartAll, the default) is its Remix hat; the same source hat set
 * again (a detail change) keeps the Remix hat; anything else is a source
 * part. */
s32 ndsP4KirbyModelPart(FTStruct *fp, s32 joint_id, s32 modelpart_id)
{
    const NDSP4KirbyPower *p = ndsP4KirbyPower(fp->nds_p4_kirby_copy);
    s32 base = ndsP4KirbyHatBase((u32)modelpart_id);
    u32 hat = 0u;

    if ((fp->fkind != nFTKindKirby) || (joint_id != NDS_P4_KIRBY_HAT_JOINT))
    {
        return modelpart_id;
    }
    if (base >= 0)
    {
        hat = (ndsP4KirbyPowerDrawsHat(p, (u32)modelpart_id) != FALSE) ? (u32)modelpart_id : 0u;
        modelpart_id = base;
    }
    else if ((fp->nds_p4_kirby_hat != 0u) &&
             (modelpart_id == ndsP4KirbyHatBase(fp->nds_p4_kirby_hat)))
    {
        hat = fp->nds_p4_kirby_hat;
    }
    else if ((p != NULL) && (modelpart_id == (s32)p->hat_base) &&
             (ndsP4KirbyPowerDrawsHat(p, p->hat_id) != FALSE))
    {
        hat = p->hat_id;
    }
    fp->nds_p4_kirby_hat = (u8)hat;
    ndsP4KirbyBindHat(fp);
    return modelpart_id;
}

void ndsP4KirbyRefreshHat(FTStruct *fp)
{
    if ((fp->fkind == nFTKindKirby) && (fp->nds_p4_kirby_copy != 0u))
    {
        (void)ndsP4KirbyModelPart(fp, NDS_P4_KIRBY_HAT_JOINT,
            fp->modelpart_status[NDS_P4_KIRBY_HAT_JOINT - nFTPartsJointCommonStart].modelpart_id_curr);
    }
}

sb32 ndsP4KirbyDefaultHat(FTStruct *fp, s32 modelpart_id)
{
    const NDSP4KirbyPower *p = ndsP4KirbyPower(fp->nds_p4_kirby_copy);

    if ((p == NULL) || (fp->fkind != nFTKindKirby))
    {
        return FALSE;
    }
    if ((modelpart_id == (s32)p->hat_base) && (ndsP4KirbyPowerDrawsHat(p, p->hat_id) != FALSE))
    {
        fp->nds_p4_kirby_hat = (u8)p->hat_id;
        ndsP4KirbyBindHat(fp);
        return TRUE;
    }
    fp->nds_p4_kirby_hat = 0u;
    return (p->hat_count != 0u) ? TRUE : FALSE;
}

sb32 ndsP4KirbyPlayerHasPower(s32 player)
{
    return (ndsP4KirbyPower(ndsP4MatchContent(player)) != NULL) ? TRUE : FALSE;
}

u32 ndsP4KirbyMatchHats(u8 *hats, u32 capacity)
{
    u32 count = 0u;
    s32 player;

    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        const NDSP4KirbyPower *p = ndsP4KirbyPower(ndsP4MatchContent(player));
        u32 i, j;

        for (i = 0u; (p != NULL) && (i < p->hat_count); i++)
        {
            for (j = 0u; (j < count) && (hats[j] != p->hats[i]); j++)
            {
            }
            if ((j == count) && (count < capacity))
            {
                hats[count++] = p->hats[i];
            }
        }
    }
    return count;
}

sb32 ndsP4KirbyHatTextureAsset(u32 asset_id)
{
    u32 c;

    for (c = 1u; (asset_id != 0u) && (c < NDS_P4_CONTENT_LIMIT); c++)
    {
        if (gNdsP4KirbyPowers[c].hat_texture_file == asset_id)
        {
            return TRUE;
        }
    }
    return FALSE;
}

s32 ndsP4KirbyCaptureWait(FTStruct *victim_fp, s32 breakout_wait)
{
    FTStruct *holder_fp;
    const NDSP4Overrides *o;

    if ((victim_fp->status_id != nFTCommonStatusCaptureWaitKirby) ||
        (victim_fp->capture_gobj == NULL))
    {
        return breakout_wait;
    }
    holder_fp = ftGetStruct(victim_fp->capture_gobj);
    if (holder_fp->nds_p4_content == 0u)
    {
        return breakout_wait;
    }
    o = ndsP4Overrides(holder_fp);
    return ((o != NULL) && (o->kirby_capture_wait != NULL)) ?
        o->kirby_capture_wait(victim_fp, breakout_wait) : breakout_wait;
}

s32 ndsP4KirbyStarDamage(const FTStruct *victim_fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(victim_fp));

    return ((f != NULL) && (f->kirby_inhale != NULL)) ? f->kirby_inhale->star_damage : -1;
}

f32 ndsP4KirbyInhaleWindX(GObj *fighter_gobj, f32 source_x)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return source_x;
    }
    o = ndsP4Overrides(fp);
    return ((o != NULL) && (o->kirby_inhale_wind_x != 0.0F)) ? o->kirby_inhale_wind_x : source_x;
}

sb32 ndsP4CustomAbsorber(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 kind;

    if (((fp->nds_p4_content | fp->nds_p4_kirby_copy) == 0u) || (fp->special_coll == NULL))
    {
        return FALSE;
    }
    kind = fp->special_coll->kind;
    return (((kind & 0xFFFF) == NDS_P4_SPECIAL_COLL_CUSTOM) && ((kind >> 16) != 0)) ? TRUE : FALSE;
}

void ndsP4OnCustomReflect(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        if (fp->nds_p4_kirby_copy != 0u)
        {
            const NDSP4Fighter *f = ndsP4Fighter(fp->nds_p4_kirby_copy);

            if ((f != NULL) && (f->overrides != NULL) &&
                (f->overrides->kirby_on_custom_reflect != NULL))
            {
                f->overrides->kirby_on_custom_reflect(fighter_gobj);
            }
        }
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->on_custom_reflect != NULL))
    {
        o->on_custom_reflect(fighter_gobj);
    }
}

void ndsP4OnEatTomato(FTStruct *fp)
{
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->on_eat_tomato != NULL))
    {
        o->on_eat_tomato(fp);
    }
}

void ndsP4FallSpecialSetStatus(GObj *fighter_gobj, s32 status_id, f32 frame_begin,
                               f32 anim_speed, u32 flags)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->nds_p4_content != 0u)
    {
        const NDSP4Overrides *o = ndsP4Overrides(fp);

        if ((o != NULL) && (o->fall_special_status != NULL))
        {
            status_id = o->fall_special_status(fp, status_id);
        }
    }
    ftMainSetStatus(fighter_gobj, status_id, frame_begin, anim_speed, flags);
}

GObj *wpYoshiStarMakeStars(GObj *fighter_gobj, Vec3f *pos);

GObj *ndsP4YoshiStarMakeStars(GObj *fighter_gobj, Vec3f *pos)
{
    const NDSP4Overrides *o = ndsP4Overrides(ftGetStruct(fighter_gobj));

    if ((o != NULL) && (o->no_yoshi_lw_stars != 0u))
    {
        return NULL;
    }
    return wpYoshiStarMakeStars(fighter_gobj, pos);
}

void ndsP4ThrowMainSetStatus(GObj *fighter_gobj, s32 status_id, f32 frame_begin,
                             f32 anim_speed, u32 flags)
{
    if (status_id == nFTCommonStatusThrowF)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        const NDSP4Overrides *o = ndsP4Overrides(fp);

        /* jigglypuffkirbyshared.asm kirby_fthrow_1_fix: the content takes
         * Kirby's branch, which sets the fighter airborne before the status
         * and names its own; nothing between the two reads the kinetics. */
        if ((o != NULL) && (o->throw_f_kirby_status != 0))
        {
            mpCommonSetFighterAir(fp);
            status_id = o->throw_f_kirby_status;
        }
    }
    ftMainSetStatus(fighter_gobj, status_id, frame_begin, anim_speed, flags);
}

/* Lab stand-ins for unported special-move starters, counted. */
__attribute__((used)) volatile u32 gNdsP4LabSpecialStandIns;

void ndsP4LabSpecialStandIn(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    gNdsP4LabSpecialStandIns++;
}

sb32 ndsP4CheckSpecialLent(GObj *fighter_gobj, sb32 (*check)(GObj *),
                           NDSP4SpecialStart *const *tables,
                           const u8 *slots, u32 count)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));
    NDSP4SpecialStart saved[3];
    NDSP4SpecialStart starts[NDS_P4_SPECIAL_COUNT] = { NULL };
    sb32 result;
    u32 i;

    if (count > (u32)ARRAY_COUNT(saved))
    {
        return check(fighter_gobj);
    }
    if (f != NULL)
    {
        for (i = 0; i < NDS_P4_SPECIAL_COUNT; i++)
        {
            starts[i] = f->special_starts[i];
        }
    }
    /* S8: Kirby's neutral special with a content's power starts where
     * Remix's kirby_ground_nsp / kirby_air_nsp rows say; a NULL row keeps
     * the source's copy dispatch, on the parent's copy_id. */
    else if ((fp->fkind == nFTKindKirby) && (fp->nds_p4_kirby_copy != 0u))
    {
        const NDSP4KirbyPower *p = ndsP4KirbyPower(fp->nds_p4_kirby_copy);

        if (p != NULL)
        {
            starts[NDS_P4_SPECIAL_GROUND_N] = p->nsp_ground;
            starts[NDS_P4_SPECIAL_AIR_N] = p->nsp_air;
        }
    }
    else
    {
        return check(fighter_gobj);
    }
    for (i = 0; i < count; i++)
    {
        saved[i] = tables[i][fp->fkind];
        if (starts[slots[i]] != NULL)
        {
            tables[i][fp->fkind] = starts[slots[i]];
        }
    }
    result = check(fighter_gobj);
    for (i = 0; i < count; i++)
    {
        tables[i][fp->fkind] = saved[i];
    }
    return result;
}

s32 ndsP4ReplaceStatus(const FTStruct *fp, s32 status_id)
{
    const NDSP4Overrides *o = ndsP4Overrides(fp);
    const u16 *map;

    if ((o == NULL) || (o->status_replace == NULL))
    {
        return status_id;
    }
    for (map = o->status_replace; map[0] != 0xFFFFu; map += 2)
    {
        if ((s32)map[0] == status_id)
        {
            return map[1];
        }
    }
    return status_id;
}

/* The content ftManagerMakeFighter is building: the fighter's own
 * nds_p4_content is written after it returns, and its setup and first status
 * already place the static parts. */
static u32 sNdsP4MakeContent;

void ndsP4SetMakeContent(u32 content)
{
    sNdsP4MakeContent = content;
}

static const NDSP4Overrides *ndsP4StaticOverrides(const FTStruct *fp)
{
    u32 content = (fp->nds_p4_content != 0u) ? fp->nds_p4_content : sNdsP4MakeContent;
    const NDSP4Fighter *f = (content != 0u) ? ndsP4Fighter(content) : NULL;

    return (f != NULL) ? f->overrides : NULL;
}

sb32 ndsP4StaticPart(const FTStruct *fp, s32 joint_id)
{
    const NDSP4Overrides *o;
    u32 bit;

    if ((joint_id < nFTPartsJointCommonStart) ||
        (joint_id >= (nFTPartsJointCommonStart + 64)))
    {
        return FALSE;
    }
    o = ndsP4StaticOverrides(fp);
    if (o == NULL)
    {
        return FALSE;
    }
    bit = (u32)(joint_id - nFTPartsJointCommonStart);

    return (o->static_parts[bit >> 5] & (0x80000000u >> (bit & 31u))) ? TRUE : FALSE;
}

sb32 ndsP4StaticPartSkipsAnim(const FTStruct *fp, s32 joint_id)
{
    u32 word;
    s32 i;

    if (ndsP4StaticPart(fp, joint_id) == FALSE)
    {
        return FALSE;
    }
    /* The anim desc's hidden-part bits, each naming the row of the same
     * index in the attributes' hidden-part table. */
    word = fp->anim_desc.word & ~0x1Fu;

    for (i = 0; word != 0u; i++, word <<= 1)
    {
        if ((word & 0x80000000u) &&
            (fp->attr->hiddenparts[i].root_joint_id == joint_id))
        {
            return FALSE;
        }
    }
    return TRUE;
}

u32 *ndsP4SetupParts(const FTStruct *fp, u32 *setup_parts)
{
    static u32 parts[GMCOMMON_PLAYERS_MAX][2];
    const NDSP4Overrides *o = ndsP4StaticOverrides(fp);
    u32 *words;

    if ((o == NULL) || ((o->static_parts[0] | o->static_parts[1]) == 0u))
    {
        return setup_parts;
    }
    words = parts[fp->player & 3u];
    words[0] = setup_parts[0] | o->static_parts[0];
    words[1] = setup_parts[1] | o->static_parts[1];

    return words;
}

sb32 ndsP4StaticPartDObj(const DObj *dobj)
{
    const FTParts *parts = dobj->user_data.p;

    if ((parts == NULL) || (dobj->parent_gobj == NULL) ||
        (dobj->parent_gobj->id != nGCCommonKindFighter))
    {
        return FALSE;
    }
    return ndsP4StaticPart(ftGetStruct(dobj->parent_gobj), parts->joint_id);
}

sb32 ndsP4GuardSkips(const FTStruct *fp, s32 joint_id)
{
    const NDSP4Overrides *o;
    u32 bit;

    if (ndsP4StaticPart(fp, joint_id) != FALSE)
    {
        return TRUE;
    }
    if ((joint_id < nFTPartsJointCommonStart) ||
        (joint_id >= (nFTPartsJointCommonStart + 64)))
    {
        return FALSE;
    }
    o = ndsP4StaticOverrides(fp);
    if (o == NULL)
    {
        return FALSE;
    }
    bit = (u32)(joint_id - nFTPartsJointCommonStart);

    return (o->guard_skip[bit >> 5] & (0x80000000u >> (bit & 31u))) ? TRUE : FALSE;
}

sb32 ndsP4GuardSkipsDObj(const DObj *dobj)
{
    const FTParts *parts = dobj->user_data.p;

    if ((parts == NULL) || (dobj->parent_gobj == NULL) ||
        (dobj->parent_gobj->id != nGCCommonKindFighter))
    {
        return FALSE;
    }
    return ndsP4GuardSkips(ftGetStruct(dobj->parent_gobj), parts->joint_id);
}

/* NDSP4Overrides.anim_locks_rest, after the status setter's own lock step.
 * The fighter is made with its FTAttributes.animlock joints locked at rest
 * (ftParamSetAnimLocks: transform_update_mode 3, the matrix computed once)
 * and the source frees them only in a motion whose row sets anim_flags bit
 * 0, then locks them again where the last clip left them. The base's N64
 * clips never key them; the content's own clips (Ultimate's motions) do,
 * and with no row setting the bit the game drew those clips with the
 * shoulders, hips and shins at rest (owner p6t: Ike's sword "yaw issues in
 * game that don't show in your renders", Palutena's dair). A row with the
 * bit (p6_author.py "anim_locks") frees them; any other row holds them at
 * rest, as the making did (its clip leaves them out, or keys them for the
 * lock to ignore: Roy's guard); and is_use_animlocks stays FALSE, the plain
 * parts matrices the content's clips are made for (the source's lock rows
 * take the scale-compensating NCS path). */
static void ndsP4AnimLocksRest(FTStruct *fp)
{
    const FTCommonPart *part;
    u32 *animlock;
    u32 flags0;
    u32 flags1;
    u32 current;
    s32 i;

    if (fp->anim_desc.flags.is_use_animlocks)
    {
        ftParamClearAnimLocks(fp);
    }
    else if ((fp->attr != NULL) && (fp->attr->animlock != NULL) &&
             (fp->attr->commonparts_container != NULL))
    {
        part = &fp->attr->commonparts_container->commonparts[0];
        animlock = fp->attr->animlock;
        flags0 = animlock[0];
        flags1 = animlock[1];
        /* ftParamSetAnimLocks' walk, so the same joints. */
        for (i = nFTPartsJointCommonStart; (flags0 != 0u) || (flags1 != 0u); i++)
        {
            current = (i < (ARRAY_COUNT(fp->joints) - 1)) ? flags0 : flags1;
            if ((current & 0x80000000u) && (fp->joints[i] != NULL))
            {
                fp->joints[i]->rotate.vec.f =
                    part->dobjdesc[i - nFTPartsJointCommonStart].rotate;
            }
            if (i < (ARRAY_COUNT(fp->joints) - 1))
            {
                flags0 <<= 1;
            }
            else
            {
                flags1 <<= 1;
            }
        }
        ftParamSetAnimLocks(fp);
    }
    fp->is_use_animlocks = FALSE;
}

void ndsP4AfterSetStatus(GObj *fighter_gobj, s32 status_id)
{
    const NDSP4Fighter *f =
        ndsP4Fighter(ftGetStruct(fighter_gobj)->nds_p4_content);

    if ((f != NULL) && (f->overrides != NULL) && f->overrides->anim_locks_rest)
    {
        ndsP4AnimLocksRest(ftGetStruct(fighter_gobj));
    }
    if ((f != NULL) && (f->on_status != NULL))
    {
        f->on_status(fighter_gobj, status_id);
    }
}

void ndsP4BindMenuScripts(u32 content)
{
    const NDSP4Fighter *f = ndsP4Fighter(content);
    FTMotionDesc *descs;
    uintptr_t old_base;
    uintptr_t new_base;
    s32 i;

    if ((f == NULL) || (f->data->p_file_mainmotion == NULL) ||
        (*f->data->p_file_mainmotion == NULL))
    {
        return;
    }
    new_base = (uintptr_t)*f->data->p_file_mainmotion;
    old_base = (uintptr_t)sNdsP4MenuScriptBase[content];
    if (old_base == new_base)
    {
        return;
    }
    descs = f->data->submotion->motion_desc;
    for (i = 0; i < *f->data->submotion_array_count; i++)
    {
        if ((u32)descs[i].offset != 0x80000000u)
        {
            descs[i].offset = (intptr_t)((uintptr_t)descs[i].offset -
                                         old_base + new_base);
        }
    }
    sNdsP4MenuScriptBase[content] = (void *)new_base;
}

/* A sorted file list (generate_p4_fighter.py and p4_kirby.py emit them
 * ascending) holds `file_id`. */
static sb32 ndsP4AssetListHas(const u16 *assets, u32 count, u32 file_id)
{
    u32 lo = 0u;
    u32 hi = count;

    if ((count == 0u) || (file_id < assets[0]) ||
        (file_id > assets[count - 1u]))
    {
        return FALSE;
    }
    while (lo < hi)
    {
        const u32 mid = (lo + hi) >> 1;

        if (assets[mid] == file_id)
        {
            return TRUE;
        }
        if (assets[mid] < file_id)
        {
            lo = mid + 1u;
        }
        else
        {
            hi = mid;
        }
    }
    return FALSE;
}

/* A content file's NitroFS path, nitro:/reloc/p4/<id as 4 hex digits>; NULL
 * for a file no content has. One buffer, rewritten by each lookup: the
 * reloc loader's caller (ndsRelocAssetFind) keeps it in its own single
 * static entry, which has the same lifetime. Every P4 file lookup asks (the
 * loader's find, the relocation's range checks): a binary search a list,
 * where the linear walk of every list cost four Banjos ~10K cycles a frame
 * of their over-gate frames (2026-10-12 census). */
/* The answers memoized, eight ids round robin (2026-10-19): the lists are
 * constant for the build, and the loader's range checks ask the same few ids
 * over and over -- 15K cycles a frame of four Snakes' costliest frames on
 * Hyrule Temple went to the searches. */
#define NDS_P4_PATH_MEMO 8u
static u32 sNdsP4PathMemoId[NDS_P4_PATH_MEMO] = {
    0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
    0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu
};
static u8 sNdsP4PathMemoFound[NDS_P4_PATH_MEMO];
static u32 sNdsP4PathMemoNext;

const char *ndsP4RelocAssetPath(u32 file_id)
{
    static char path[] = "nitro:/reloc/p4/0000";
    static const char hex[] = "0123456789abcdef";
    u32 found = FALSE;
    u32 c;

    if (file_id > 0xFFFFu)
    {
        return NULL;
    }
    for (c = 0u; c < NDS_P4_PATH_MEMO; c++)
    {
        if (sNdsP4PathMemoId[c] == file_id)
        {
            found = sNdsP4PathMemoFound[c];
            goto answer;
        }
    }
    for (c = 1u; (c <= NDS_P4_CONTENT_LIMIT) && (found == FALSE); c++)
    {
        const NDSP4Fighter *f = (c < NDS_P4_CONTENT_LIMIT) ? ndsP4Fighter(c) : NULL;
        const u16 *assets = (f != NULL) ? f->assets : NULL;
        u32 count = (f != NULL) ? *f->asset_count : 0u;

        /* Past the contents: Kirby's power files (S8). */
        if (c == NDS_P4_CONTENT_LIMIT)
        {
            assets = gNdsP4KirbyRelocAssets;
            count = gNdsP4KirbyRelocAssetCount;
        }
        found = ndsP4AssetListHas(assets, count, file_id);
    }
#if NDS_P5 && (NDS_P5_RELOC_FILE_COUNT > 0)
    if (found == FALSE)
    {
        /* P5: the Remix stages' map, geometry and wallpaper files, staged
         * beside the contents' (scripts/p5/p5_rules.mk). */
        static const u16 kP5Files[] = { NDS_P5_RELOC_FILE_IDS };

        found = ndsP4AssetListHas(kP5Files, NDS_P5_RELOC_FILE_COUNT, file_id);
    }
#endif
#if NDS_P4_ONEP_ART_FILE_COUNT > 0
    if (found == FALSE)
    {
        /* P7 C1: the contents' congratulations plates and intro names
         * (scripts/p4/p4_onep_art.py; battleship_mncongra.c,
         * battleship_sc1pintro.c). */
        static const u16 kOnePArtFiles[] = { NDS_P4_ONEP_ART_FILE_IDS };

        found = ndsP4AssetListHas(kOnePArtFiles, NDS_P4_ONEP_ART_FILE_COUNT, file_id);
    }
#endif
    sNdsP4PathMemoId[sNdsP4PathMemoNext] = file_id;
    sNdsP4PathMemoFound[sNdsP4PathMemoNext] = (u8)found;
    sNdsP4PathMemoNext = (sNdsP4PathMemoNext + 1u) % NDS_P4_PATH_MEMO;
answer:
    if (found == FALSE)
    {
        return NULL;
    }
    {
        u32 at = sizeof(path) - 5u;

        path[at + 0u] = hex[(file_id >> 12) & 0xFu];
        path[at + 1u] = hex[(file_id >> 8) & 0xFu];
        path[at + 2u] = hex[(file_id >> 4) & 0xFu];
        path[at + 3u] = hex[file_id & 0xFu];
    }
    return path;
}

sb32 ndsP4LabSkipsDependency(u32 owner_asset, u32 dep_asset)
{
    u32 c;

    if (ndsP4RelocAssetPath(owner_asset) == NULL)
    {
        return FALSE;
    }
    for (c = 1u; c < NDS_P4_CONTENT_LIMIT; c++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(c);
        u32 i;

        if (f == NULL)
        {
            continue;
        }
        for (i = 0u; i < *f->lab_skip_file_count; i++)
        {
            if (f->lab_skip_files[i] == dep_asset)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* The content's baked texture pointer from owner into dep at slot (any slot
 * when slot is UINT32_MAX), or NULL. Battle only: the entry articles that
 * carry the pixels draw in matches. */
static const NDSP4BakedRef *ndsP4FindBakedRef(u32 owner_asset, u32 dep_asset, u32 slot)
{
    u32 c;

    if (gNdsSceneManagerCurrIsBattle == 0u)
    {
        return NULL;
    }
    for (c = 1u; c < NDS_P4_CONTENT_LIMIT; c++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(c);
        u32 i;

        if (f == NULL)
        {
            continue;
        }
        for (i = 0u; i < *f->baked_ref_count; i++)
        {
            const NDSP4BakedRef *r = &f->baked_refs[i];

            if ((r->owner == owner_asset) && (r->dep == dep_asset) &&
                ((slot == UINT32_MAX) || (r->slot == slot)))
            {
                return r;
            }
        }
    }
    return NULL;
}

sb32 ndsP4NativeOwnsDependency(u32 owner_asset, u32 dep_asset)
{
    return (ndsP4FindBakedRef(owner_asset, dep_asset, UINT32_MAX) != NULL) ? TRUE : FALSE;
}

sb32 ndsP4NativeBakedRef(u32 owner_asset, u32 dep_asset, u32 slot, u32 target)
{
    const NDSP4BakedRef *r = ndsP4FindBakedRef(owner_asset, dep_asset, slot);

    return ((r != NULL) && (r->target == target)) ? TRUE : FALSE;
}

/* A content's animation ids ascend (generate_p4_fighter.py emits them
 * sorted; bit 15 is the event32 flag): one binary search a list. */
static u32 ndsP4FindAnimIn(const u16 *anims, u32 count, u32 asset_id)
{
    u32 lo = 0u;
    u32 hi = count;

    if ((count == 0u) || (asset_id < (anims[0] & 0x7FFFu)) ||
        (asset_id > (anims[count - 1u] & 0x7FFFu)))
    {
        return 0u;
    }
    while (lo < hi)
    {
        const u32 mid = (lo + hi) >> 1;
        const u32 id = anims[mid] & 0x7FFFu;

        if (id == asset_id)
        {
            return 1u | ((anims[mid] >> 15) << 1);
        }
        if (id < asset_id)
        {
            lo = mid + 1u;
        }
        else
        {
            hi = mid;
        }
    }
    return 0u;
}

/* The AObj16 admission gate asks this twice for every animated joint of
 * every clip it binds (ndsRelocPointerIsFighterAObj16), and it was a linear
 * walk of every content's list (~5,000 ids): 158K cycles a frame of four
 * Banjos' over-gate frames (2026-10-12 census). The last answer is memoized:
 * the joints of one clip ask about one id. */
static u32 sNdsP4FindAnimLastId = 0xFFFFFFFFu;
static u32 sNdsP4FindAnimLast;

static u32 ndsP4FindAnim(u32 asset_id)
{
    u32 c;
    u32 found = 0u;

    if ((asset_id < 0x854u) || (asset_id >= 0x8000u))
    {
        return 0u;
    }
    if (asset_id == sNdsP4FindAnimLastId)
    {
        return sNdsP4FindAnimLast;
    }
    for (c = 1u; (c < NDS_P4_CONTENT_LIMIT) && (found == 0u); c++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(c);

        if (f != NULL)
        {
            found = ndsP4FindAnimIn(f->anims, *f->anim_count, asset_id);
        }
    }
    /* Past the contents: Kirby's power animations (S8), ascending too
     * (p4_kirby.py sorts them). */
    if (found == 0u)
    {
        found = ndsP4FindAnimIn(gNdsP4KirbyAnims, gNdsP4KirbyAnimCount,
                                asset_id);
    }
    sNdsP4FindAnimLastId = asset_id;
    sNdsP4FindAnimLast = found;
    return found;
}

sb32 ndsP4IsFighterAnim(u32 asset_id)
{
    return (ndsP4FindAnim(asset_id) != 0u) ? TRUE : FALSE;
}

sb32 ndsP4IsFighterAnimEvent32(u32 asset_id)
{
    return ((ndsP4FindAnim(asset_id) & 2u) != 0u) ? TRUE : FALSE;
}

u32 ndsP4MainAttributesOffset(u32 asset_id)
{
    u32 c;

    for (c = 1u; c < NDS_P4_CONTENT_LIMIT; c++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(c);

        if ((f != NULL) && (f->data->file_main_id == asset_id))
        {
            return (u32)f->data->o_attributes;
        }
    }
    return 0u;
}

/* Upper half-word of an f32, the operand form of Remix's FSM and multiplier
 * commands (`lhu; sll 16`). */
static f32 ndsP4HalfFloat(u32 word)
{
    union { u32 u; f32 f; } v;

    v.u = (word & 0xFFFFu) << 16;
    return v.f;
}

/* Remix 0xD0 SET FRAME SPEED MULTIPLIER (Command.asm fsm_). Flag 0 sets every
 * joint, like gcSetAnimSpeed. Flag 1 leaves the top joint, whose speed is the
 * motion-script clock, and nudges it one unit in the last place off the
 * speed a status sets (1.0 times the content's animation rate) so
 * ftMainSetStatus's `anim_speed != root speed` test still restores the
 * children afterwards. The content's animation rate (P7 B3) multiplies the
 * operand. */
static void ndsP4RemixFrameSpeed(GObj *fighter_gobj, u32 word)
{
    s32 status_id = ftGetStruct(fighter_gobj)->status_id;
    f32 speed = ndsP4ScaleAnimSpeed(fighter_gobj, status_id, ndsP4HalfFloat(word));
    DObj *root = DObjGetStruct(fighter_gobj);
    DObj *dobj;

    if (((word >> 16) & 0xFFu) == 0u)
    {
        gcSetAnimSpeed(fighter_gobj, speed);
        return;
    }
    if (root->anim_speed == ndsP4ScaleAnimSpeed(fighter_gobj, status_id, 1.0F))
    {
        union { u32 u; f32 f; } nudge;

        nudge.f = root->anim_speed;
        nudge.u ^= 1u;
        root->anim_speed = nudge.f;
    }
    for (dobj = root->child; dobj != NULL; dobj = gcGetTreeDObjNext(dobj))
    {
        dobj->anim_speed = speed;
    }
}

/* Remix hitbox overrides by port and hitbox: a launch direction (0 none,
 * NDS_P4_HITBOX_FORWARD, NDS_P4_HITBOX_BACKWARD) and a hit sound (-1 none;
 * bit 15 set plays it after the source's). */
#define NDS_P4_HITBOX_FORWARD 1u
#define NDS_P4_HITBOX_BACKWARD 2u
static u8 sNdsP4HitboxDir[GMCOMMON_PLAYERS_MAX][4];
static u16 sNdsP4HitboxFgm[GMCOMMON_PLAYERS_MAX][4] = {
    { 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu }, { 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu },
    { 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu }, { 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu }
};
/* The knockback values (set knockback, growth, base) of the hit
 * ftMainProcessHitCollisionStatsMain last picked for each fighter, its
 * largest knockback; zeros for a stage's hit (ndsP4PickedHitKnockback). */
static u16 sNdsP4PickedHitKnockback[GMCOMMON_PLAYERS_MAX][3];

/* Defined in the decomp's lb and sys code (no DS header declares them). */
void *lbCommonMakePositionFGM(u16 fgm, f32 pos);
s32 syUtilsRandIntRange(s32 range);

void ndsP4ResetHitboxOverrides(const FTStruct *fp, u32 attack_id)
{
    u32 port = fp->player & 3u;
    u32 i;

    for (i = 0u; i < 4u; i++)
    {
        if ((attack_id >= 4u) || (i == attack_id))
        {
            sNdsP4HitboxDir[port][i] = 0u;
            sNdsP4HitboxFgm[port][i] = 0xFFFFu;
        }
    }
}

void ndsP4MakeHitPositionFGM(FTStruct *attacker_fp, FTAttackColl *attack_coll,
                             u16 fgm_id, f32 pos_x)
{
    u32 hitbox = (u32)(attack_coll - attacker_fp->attack_colls);
    u16 own = 0xFFFFu;

    if ((ndsP4Content(attacker_fp) != 0u) && (hitbox < 4u))
    {
        own = sNdsP4HitboxFgm[attacker_fp->player & 3u][hitbox];
    }
    if ((own == 0xFFFFu) || ((own & 0x8000u) != 0u))
    {
        lbCommonMakePositionFGM(fgm_id, pos_x);
    }
    if (own != 0xFFFFu)
    {
        lbCommonMakePositionFGM(own & 0x7FFFu, pos_x);
    }
}

static void ndsP4KeepPickedHitKnockback(const FTHitLog *hitlog, const FTStruct *fp)
{
    u16 *picked = sNdsP4PickedHitKnockback[fp->player & 3u];
    u32 weight = 0u;
    u32 scale = 0u;
    u32 base = 0u;

    switch (hitlog->attacker_object_class)
    {
    case nFTHitLogObjectFighter:
        weight = (u32)((const FTAttackColl *)hitlog->attack_coll)->knockback_weight;
        scale = (u32)((const FTAttackColl *)hitlog->attack_coll)->knockback_scale;
        base = (u32)((const FTAttackColl *)hitlog->attack_coll)->knockback_base;
        break;

    case nFTHitLogObjectWeapon:
        weight = ((const WPAttackColl *)hitlog->attack_coll)->knockback_weight;
        scale = ((const WPAttackColl *)hitlog->attack_coll)->knockback_scale;
        base = ((const WPAttackColl *)hitlog->attack_coll)->knockback_base;
        break;

    case nFTHitLogObjectItem:
        weight = ((const ITAttackColl *)hitlog->attack_coll)->knockback_weight;
        scale = ((const ITAttackColl *)hitlog->attack_coll)->knockback_scale;
        base = ((const ITAttackColl *)hitlog->attack_coll)->knockback_base;
        break;

    default:
        break;
    }
    picked[0] = (u16)weight;
    picked[1] = (u16)scale;
    picked[2] = (u16)base;
}

void ndsP4PickedHitKnockback(const FTStruct *fp, s32 *weight, s32 *scale, s32 *base)
{
    const u16 *picked = sNdsP4PickedHitKnockback[fp->player & 3u];

    *weight = picked[0];
    *scale = picked[1];
    *base = picked[2];
}

void ndsP4UpdateHitDamageStats(const FTHitLog *hitlog, FTStruct *fp,
                               s32 damage_player, s32 damage_object_class,
                               s32 damage_object_kind, u16 flags,
                               u16 damage_stat_count)
{
    ndsP4KeepPickedHitKnockback(hitlog, fp);
    if (hitlog->attacker_object_class == nFTHitLogObjectFighter)
    {
        const FTStruct *attacker_fp = ftGetStruct(hitlog->attacker_gobj);
        u32 hitbox = (u32)((const FTAttackColl *)hitlog->attack_coll -
                           attacker_fp->attack_colls);

        if ((ndsP4Content(attacker_fp) != 0u) && (hitbox < 4u))
        {
            u32 dir = sNdsP4HitboxDir[attacker_fp->player & 3u][hitbox];

            if (dir == NDS_P4_HITBOX_BACKWARD)
            {
                fp->damage_lr = attacker_fp->lr;
            }
            else if (dir == NDS_P4_HITBOX_FORWARD)
            {
                fp->damage_lr = -attacker_fp->lr;
            }
        }
    }
    /* WaddleDoo.asm beam_knockback_direction_fix (at 0x800E44EC): a Waddle
     * Doo's beam star pushes along its facing, as a moving shot does, not
     * by the sides it and the victim are on (its stars do not move). */
    else if ((hitlog->attacker_object_class == nFTHitLogObjectWeapon) &&
             (wpGetStruct(hitlog->attacker_gobj)->kind == NDS_P4_WP_KIND_WADDLEDOO_BEAM))
    {
        fp->damage_lr = (wpGetStruct(hitlog->attacker_gobj)->lr < 0) ? +1 : -1;
    }
    ftParamUpdate1PGameDamageStats(fp, damage_player, damage_object_class,
                                   damage_object_kind, flags,
                                   damage_stat_count);
}

/* The u16 at a byte address in a file the loader word-swapped. */
static u16 ndsP4ReadSwappedU16(const void *ptr)
{
    uintptr_t at = (uintptr_t)ptr;
    u32 word = *(const u32 *)(at & ~(uintptr_t)3u);

    return (u16)(((at & 2u) != 0u) ? word : (word >> 16));
}

u32 *ndsP4RunRemixMotionEvents(GObj *fighter_gobj, FTMotionScript *ms,
                               sb32 forward)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 *p = ms->p_script;

    while ((p != NULL) && ((*p >> 24) >= 0xD0u))
    {
        u32 word = *p;
        u32 op = word >> 24;
        u32 length = 4u;

        switch (op)
        {
        case 0xD0:
            ndsP4RemixFrameSpeed(fighter_gobj, word);
            break;
        case 0xD1:
            /* SET ARMOUR (armour_), both tables: knockback-based armour,
             * the field Yoshi's double jump sets. */
            fp->knockback_resist_status = ndsP4HalfFloat(word);
            break;
        case 0xD2:
            /* OVERRIDE HITBOX DIRECTION, both tables: byte 2 the hitbox,
             * byte 3 the direction. */
            if (((word >> 8) & 0xFFu) < 4u)
            {
                sNdsP4HitboxDir[fp->player & 3u][(word >> 8) & 0xFFu] =
                    (u8)word;
            }
            break;
        case 0xD3:
            gNdsP4TranslationMultiplier[fp->player & 3u] = ndsP4HalfFloat(word);
            break;
        case 0xD4:
            /* SET Y VELOCITY; the fast-forward table skips it. */
            if (forward == FALSE)
            {
                fp->physics.vel_air.y = ndsP4HalfFloat(word);
            }
            break;
        case 0xD5:
            /* FAST FALL; the fast-forward table skips it. */
            if (forward == FALSE)
            {
                fp->is_fastfall = ((word & 0xFFu) != 0u) ? TRUE : FALSE;
            }
            break;
        case 0xD6:
            /* RANDOM SFX (play_sfx_random_), 8 bytes: byte 1 the chance in
             * 100, byte 2 sound (0) or voice (1), byte 3 the table size; the
             * second word points at the table of u16 sound ids. The
             * fast-forward table skips it. */
            length = 8u;
            if ((forward == FALSE) && (fp->is_muted == FALSE) &&
                ((u32)syUtilsRandIntRange(100) < ((word >> 16) & 0xFFu)))
            {
                const u8 *table = (const u8 *)(uintptr_t)p[1];
                u16 fgm_id = ndsP4ReadSwappedU16(
                    table + 2 * syUtilsRandIntRange((s32)(word & 0xFFu)));

                if (((word >> 8) & 0xFFu) == 1u)
                {
                    ftParamPlayVoice(fp, fgm_id);
                }
                else
                {
                    func_800269C0_275C0(fgm_id);
                }
            }
            break;
        case 0xD7:
            /* SET KINETIC STATE, both tables: aerial (byte 3 != 0) also
             * counts one jump used. */
            fp->jumps_used = ((word & 0xFFu) != 0u) ? 1u : 0u;
            fp->ga = ((word & 0xFFu) != 0u) ? nMPKineticsAir : nMPKineticsGround;
            break;
        case 0xD8:
            /* SET HITBOX FGM, both tables: byte 1's high nibble sets all four
             * hitboxes, else its low nibble names one; halfword 1 the id. */
            if (((word >> 20) & 0xFu) != 0u)
            {
                u32 i;

                for (i = 0u; i < 4u; i++)
                {
                    sNdsP4HitboxFgm[fp->player & 3u][i] = (u16)word;
                }
            }
            else if (((word >> 16) & 0xFu) < 4u)
            {
                sNdsP4HitboxFgm[fp->player & 3u][(word >> 16) & 0xFu] =
                    (u16)word;
            }
            break;
        case 0xD9:
            /* SET ENV COLOR (set_env_color_), 8 bytes: the second word is
             * the port's colour; the fast-forward table skips it. */
            length = 8u;
            if (forward == FALSE)
            {
                gNdsP4EnvColor[fp->player & 3u] = p[1];
            }
            break;
        case 0xDA:
            /* SWITCH DIRECTION; the fast-forward table skips it. */
            if (forward == FALSE)
            {
                fp->lr = -fp->lr;
            }
            break;
        case 0xDC:
            /* L VOICE SFX, 8 bytes: the voice in halfword 1, or the one in
             * the second word's low half while the taunt button is held. The
             * fast-forward table skips it. */
            length = 8u;
            if (forward == FALSE)
            {
                ftParamPlayVoice(fp, ((fp->input.pl.button_hold &
                                       fp->input.button_mask_l) != 0u) ?
                                         (u16)p[1] : (u16)word);
            }
            break;
        default:
            /* E0+ are donor no-ops. */
            break;
        }
        p = (u32 *)((uintptr_t)p + length);
        ms->p_script = p;
    }
    return p;
}

