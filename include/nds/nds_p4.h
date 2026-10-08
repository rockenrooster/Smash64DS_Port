#ifndef NDS_P4_H
#define NDS_P4_H

/* P4: Smash Remix fighters on the DS.
 *
 * A P4 fighter is its donor's setup parent plus resolved donor content. The
 * fighter keeps fp->fkind == the parent's kind (Falco: Fox), so every
 * BattleShip kind-indexed table and kind comparison resolves exactly as
 * Remix's define_character resolves it: by copying the parent's row. What the
 * donor overrides is selected by fp->nds_p4_content (1-based; 0 = original
 * cast): its FTData (files, attributes, motion scripts), the status callbacks
 * its action table replaces, its kind-table rows and its DS-native assets.
 *
 * Content data is generated at build time from the user's own ROM by
 * scripts/p4/generate_p4_fighter.py; see docs/P4/P4_STATUS.md. */

#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <ft/ftcomputer.h>
#include <nds_build_config.h>

#ifndef NDS_P4
#define NDS_P4 0
#endif
#ifndef NDS_P4_FALCO
#define NDS_P4_FALCO 0
#endif

enum
{
    NDS_P4_CONTENT_NONE = 0,
    NDS_P4_CONTENT_FALCO = 1,
    NDS_P4_CONTENT_LIMIT
};

typedef struct NDSP4StatusOverride
{
    u32 status_id;
    void (*procs[4])(GObj *); /* update, interrupt, physics, map */
} NDSP4StatusOverride;

typedef struct NDSP4RelocAsset
{
    u32 file_id;
    const char *path;
} NDSP4RelocAsset;

/* A Sprite in a content file whose header the reloc loader normalizes
 * (reloc_backend_assets.c, the battle-interface sprite manifest). */
typedef struct NDSP4SpriteDesc
{
    u16 file_id;
    u16 bitmap_count;
    u32 offset;
    u16 width;
    u16 height;
    u8 bmfmt;
    u8 bmsiz;
} NDSP4SpriteDesc;

/* Results and menu presentation, from the donor's kind-table rows. */
typedef struct NDSP4Present
{
    const char *results_name; /* mnVSResultsMakeString text (digits = spacing) */
    f32 results_name_x;
    f32 results_name_scale;
    f32 results_wins_x;       /* "WINS!" left x */
    u16 announce_fgm;         /* winner_fgm: the announcer's name call */
    u16 victory_bgm;          /* winner_bgm */
    u16 crowd_chant_fgm;
    f32 menu_zoom;
    u8 default_costumes[8];
} NDSP4Present;

/* A content's CPU rows: Remix's per-character tables (Character.asm)
 * that AI.asm runs. A NULL field keeps the parent's vanilla path. */
typedef struct NDSP4Computer
{
    /* ai_behaviour: the attack list (grounded rows, END, aerial rows, END)
     * ftComputerCheckDetectTarget reads instead of dFTComputerAttackList. */
    const FTComputerAttack *attacks;
    /* ai_attack_prevent: per detected attack, in place of the parent's
     * fkind switch (the jump table at 0x801334E4); NDS_P4_COMPUTER_*. */
    s32 (*prevent)(FTStruct *fp, s32 input_kind);
    /* ai_long_range NONE: no special from long range (0x80138ECC). */
    sb32 long_range_none;
    /* recovery_logic: after the recover objective's walk. */
    void (*recover)(FTStruct *fp);
    /* cpu_post_process: after the objective, before the inputs run. */
    void (*post_process)(FTStruct *fp);
} NDSP4Computer;

/* ai_attack_prevent results: keep the attack, run the ledge-ground test
 * the parent switch sets for recovery specials (is_attempt_cliffcatch),
 * or skip the attack (0x80133A14). */
enum
{
    NDS_P4_COMPUTER_ALLOW,
    NDS_P4_COMPUTER_CHECK_GROUND,
    NDS_P4_COMPUTER_SKIP
};

/* Remix's added CPU input routines (AI.asm add_cpu_input_routine) keep
 * Remix's ids, past the vanilla table's 0x31 (bass log "CPU input routine
 * added"). */
#define NDS_P4_COMPUTER_INPUT_BASE 0x31
enum
{
    nNDSP4ComputerInputMultiShine = 0x3A,
    nNDSP4ComputerInputNSPTowards = 0x42,
    nNDSP4ComputerInputFair = 0x43,
    nNDSP4ComputerInputBair = 0x44,
    nNDSP4ComputerInputDashAttack = 0x47,
    nNDSP4ComputerInputNull = 0x4C
};

typedef struct NDSP4Fighter
{
    const char *name;
    s32 parent_kind;
    FTData *data;
    const NDSP4StatusOverride *overrides;
    const u32 *override_count;
    const NDSP4RelocAsset *assets;
    const u32 *asset_count;
    const FTFileSize *file_size;
    const u16 *anims; /* own animation files, bit 15 = AObjEvent32 */
    const u32 *anim_count;
    /* Optional: after the status callbacks are applied (donor init hooks). */
    void (*on_status)(GObj *fighter_gobj, s32 status_id);
    /* Optional: the content's CPU rows. */
    const NDSP4Computer *computer;
    /* HUD: stock icon (8x8 OBJ4 cell, 32 B) and its costume LUTs. */
    const u8 *stock_gfx;
    const u16 (*stock_palettes)[16];
    const u32 *stock_palette_count;
    const NDSP4SpriteDesc *sprites;
    const u32 *sprite_count;
    const NDSP4Present *present;
} NDSP4Fighter;

/* Character-select selection ids: an original's fkind, or NDS_P4_SEL_BASE +
 * content for a Remix selection (above every FTKind and nFTKindNull). */
#define NDS_P4_SEL_BASE 0x40u
static inline u32 ndsP4SelContent(u32 sel)
{
    return ((sel > NDS_P4_SEL_BASE) &&
            (sel < (NDS_P4_SEL_BASE + NDS_P4_CONTENT_LIMIT))) ?
        (sel - NDS_P4_SEL_BASE) : 0u;
}

/* Content selected for each battle player, written by the character select
 * (or a lab descriptor) next to gSCManagerBattleState->players[].fkind, which
 * holds the parent kind. */
extern u8 gNdsP4PlayerContent[GMCOMMON_PLAYERS_MAX];

#if NDS_P4

const NDSP4Fighter *ndsP4Fighter(u32 content);
static inline u32 ndsP4Content(const FTStruct *fp)
{
    return (fp != NULL) ? fp->nds_p4_content : 0u;
}
/* The content of a battle player's live fighter (0 = original cast or none). */
u32 ndsP4PlayerContent(u32 player);
/* The content a player's fighter is made with in the current scene: the
 * match's selection in the VS battle and its Results, the slot's hovered
 * selection on the VS character select, none anywhere else (a finished
 * match's selection must not reach the menus or the 1P modes). Only a
 * content whose parent is `fkind` counts. */
u32 ndsP4MakeContent(s32 player, s32 fkind);
/* The match's selection: in the VS battle and its Results only. */
u32 ndsP4MatchContent(s32 player);
/* VS character select: the content a slot's preview shows (0 = none). */
void ndsP4SetPreviewContent(u32 slot, u32 content);
/* The VS character select's P4 preview files: the content's preview pack
 * (kind NDS_P4_SEL_BASE + content, scripts/p4/p4_preview_pack.py) into its
 * FTData, then its motion file, which holds the menu motions' scripts
 * (battleship_ftmanager.c). */
sb32 ndsFTManagerSetupPreviewFilesP4(u32 content);
/* ftManagerSetupFilesAllKind: TRUE when some live selection still needs the
 * parent's own files (not only P4 children that reuse its code). */
sb32 ndsP4ParentFilesNeeded(s32 fkind);
/* ftManagerAllocFighter: publish each registered content's file sizes. */
void ndsP4SetupFileSizes(u32 data_flags);
/* ftMainSetStatus epilogue: apply the content's replaced status callbacks. */
void ndsP4ApplyStatusOverrides(GObj *fighter_gobj, s32 status_id);
/* ftMainSetStatus prologue: Remix change_action_ resets. */
void ndsP4OnSetStatus(GObj *fighter_gobj);
/* CPU (src/import/battleship_ftcomputer*.c). The content's rows, or NULL
 * for the original cast. */
const NDSP4Computer *ndsP4Computer(const FTStruct *fp);
/* ftComputerFollowObjectiveRecover, after its walk: the content's CPU
 * recovery_logic (Remix AI.asm custom_recovery_logic). */
void ndsP4ComputerRecover(FTStruct *fp);
/* ftComputerProcessAll, after the objective (AI.asm cpu_post_process). */
void ndsP4ComputerPostProcess(FTStruct *fp);
/* ftComputerSetCommand* that also takes Remix's input routine ids. */
void ndsP4ComputerSetCommandWaitShort(FTStruct *fp, s32 index);
void ndsP4ComputerSetCommandImmediate(FTStruct *fp, s32 index);
/* Before ftComputerUpdateInputs: the stick X a Remix routine's run this
 * tick ends on, for Remix's directional values (AI.asm
 * extend_stick_x_commands), else NDS_P4_COMPUTER_STICK_KEEP. */
#define NDS_P4_COMPUTER_STICK_KEEP 0x100
s32 ndsP4ComputerStickX(const FTStruct *fp);
/* After a content's motion file loads: bind menu-motion script pointers. */
void ndsP4BindMenuScripts(u32 content);
/* NitroFS path for a P4 file id, or NULL. */
const char *ndsP4RelocAssetPath(u32 file_id);
/* Reloc normalizer seams: a content's own animation files, and the
 * FTAttributes offset when asset_id is a content's main file (else 0). */
sb32 ndsP4IsFighterAnim(u32 asset_id);
sb32 ndsP4IsFighterAnimEvent32(u32 asset_id);
u32 ndsP4MainAttributesOffset(u32 asset_id);

/* Remix custom motion commands (first byte 0xD0..0xDC, src/Command.asm).
 * Called at every motion-event boundary read; executes and steps past custom
 * commands, returning the first vanilla command. `forward` selects Remix's
 * command_table_2 (the fast-forward loops). */
u32 *ndsP4RunRemixMotionEvents(GObj *fighter_gobj, FTMotionScript *ms,
                               sb32 forward);
static inline void *ndsP4MotionEventCursor(GObj *fighter_gobj,
                                           FTMotionScript *ms, sb32 forward)
{
    u32 *p = ms->p_script;

    if (__builtin_expect((p == NULL) || ((*p >> 24) < 0xD0u), 1))
    {
        return p;
    }
    return ndsP4RunRemixMotionEvents(fighter_gobj, ms, forward);
}

/* Remix TOPJOINT TRANSLATION MULTIPLIER (0xD3), per player port. */
extern f32 gNdsP4TranslationMultiplier[GMCOMMON_PLAYERS_MAX];
static inline f32 ndsP4TranslationMultiplier(const FTStruct *fp)
{
    return gNdsP4TranslationMultiplier[fp->player & 3u];
}

#else

static inline u32 ndsP4Content(const FTStruct *fp)
{
    (void)fp;
    return 0u;
}
static inline u32 ndsP4PlayerContent(u32 player)
{
    (void)player;
    return 0u;
}

#endif /* NDS_P4 */

#endif /* NDS_P4_H */
