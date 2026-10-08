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

#include <nds/nds_p4_contents.h>

enum
{
    NDS_P4_CONTENT_NONE = 0,
#define NDS_P4_CONTENT_ENUM(id_, T_, N_, n_, parent_, model_, main_) \
    NDS_P4_CONTENT_##N_ = (id_),
    NDS_P4_CONTENT_ROWS(NDS_P4_CONTENT_ENUM)
#undef NDS_P4_CONTENT_ENUM
    NDS_P4_CONTENT_LIMIT = NDS_P4_CONTENT_MAX_ID + 1
};

/* A special-status callback slot the content keeps from its parent. The
 * generator never names a parent routine; the slot takes the parent's
 * routine for the same status the first time the table is used
 * (ndsP4SpecialStatusDescs). */
#define NDS_P4_PROC_INHERIT ((void (*)(GObj *))1)

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

/* Remix SwordTrail.asm: a trail the SET AFTERIMAGE motion command selects
 * with is_itemswing >= 2 (0 and 1 are the vanilla Link-sword and item-swing
 * trails). It follows model part `model_part` (joint model_part +
 * nFTPartsJointCommonStart) along that joint's matrix row `axis` (0 X, 1 Y,
 * 2 Z) from `start` to `end`, base colour `colour_1` to tip colour
 * `colour_2` (RGBA32, alpha unused). A content lists the rows whose
 * character is its own. */
typedef struct NDSP4SwordTrail
{
    u8 id;
    u8 model_part;
    u8 axis;
    u8 reserved;
    u32 colour_1;
    u32 colour_2;
    f32 start;
    f32 end;
} NDSP4SwordTrail;

/* Remix's entry_action and entry_script rows (Character.asm): the appear
 * statuses for entering facing right and left, and the entry effect. Remix
 * repoints a fighter's entry_script at another kind's case of the source's
 * switch (ftCommonAppearSetStatus) or past it (no effect), and patches the
 * shared makers to read its own files. Only two forms need no port: no
 * effect, and the parent's own case when the content's special files are the
 * parent's or same-layout copies of them. */
enum
{
    NDS_P4_ENTRY_NONE,   /* entry_script skips the switch */
    NDS_P4_ENTRY_PARENT  /* the parent's case, on parent-layout files */
};
typedef struct NDSP4Entry
{
    s32 appear_status[2]; /* entry_action: facing right, facing left */
    u8 effect;            /* NDS_P4_ENTRY_* */
    u8 lab_fallback;      /* lab builds: an unported effect, run as NONE */
    u8 reserved[2];
} NDSP4Entry;

/* Special-move starters: Remix's ground_nsp ... air_dsp tables, which
 * replace the source's dFTCommonSpecial*StatusList rows by kind. A content's
 * row is NULL where it equals the parent's; lab builds start nothing for a
 * donor routine with no port (ndsP4LabSpecialStandIn). */
enum
{
    NDS_P4_SPECIAL_GROUND_N,
    NDS_P4_SPECIAL_AIR_N,
    NDS_P4_SPECIAL_GROUND_HI,
    NDS_P4_SPECIAL_AIR_HI,
    NDS_P4_SPECIAL_GROUND_LW,
    NDS_P4_SPECIAL_AIR_LW,
    NDS_P4_SPECIAL_COUNT
};
typedef void (*NDSP4SpecialStart)(GObj *fighter_gobj);

typedef struct NDSP4Fighter
{
    const char *name;
    s32 parent_kind;
    FTData *data;
    /* Special statuses from nFTCommonStatusSpecialStart: Remix's action
     * array, statuses its add_new_action appended included. ftMainSetStatus
     * reads it in place of the parent's table. */
    FTStatusDesc *special_statuses;
    const u32 *special_status_count;
    /* Lab builds: donor routines that run a stand-in (0 when shipping). */
    const u32 *lab_fallback_count;
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
    const NDSP4SwordTrail *sword_trails;
    const u32 *sword_trail_count;
    const NDSP4Entry *entry;
    const NDSP4SpecialStart *special_starts; /* NDS_P4_SPECIAL_COUNT rows */
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
/* ftMainSetStatus: the special status table the source reads for this
 * fighter. The content's, its inherited slots filled from `parent` (the
 * parent's own table) at first use; NULL for the original cast. */
FTStatusDesc *ndsP4SpecialStatusDescs(const FTStruct *fp,
                                      const FTStatusDesc *parent);
/* ftMainSetStatus epilogue: the content's on_status hook. */
void ndsP4AfterSetStatus(GObj *fighter_gobj, s32 status_id);
/* ftCommonAppearSetStatus: the content's entry row, or NULL. */
const NDSP4Entry *ndsP4Entry(const FTStruct *fp);
/* ftCommonSpecial*CheckInterruptCommon: run `check` with the content's
 * special-move starters lent to the source tables' rows for its kind
 * (tables[i][fkind] takes starter slots[i]), then restore them. */
sb32 ndsP4CheckSpecialLent(GObj *fighter_gobj, sb32 (*check)(GObj *),
                           NDSP4SpecialStart *const *tables,
                           const u8 *slots, u32 count);
/* Lab builds: the starter of an unported donor special (starts nothing). */
void ndsP4LabSpecialStandIn(GObj *fighter_gobj);
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

/* The fighter's SwordTrail.asm row for a SET AFTERIMAGE id >= 2, or NULL
 * (not its trail: the source then records and draws nothing). */
const NDSP4SwordTrail *ndsP4SwordTrail(const FTStruct *fp, u32 id);
/* ftMainProcParams' afterimage step for a Remix trail (SwordTrail.asm
 * initial_setup_ and axis_setup_): the source's Link-sword step on the row's
 * joint and axis. The caller applies the source's gate (no hitlag at the
 * proc's start, drawstatus not -1). */
void ndsP4UpdateSwordTrail(FTStruct *fp);

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
