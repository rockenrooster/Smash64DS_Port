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

/* A content's hand-ported CPU routines: Remix code AI.asm runs from the
 * per-character tables (Character.asm). A NULL field keeps the parent's
 * vanilla path. The data rows (attack list, long range, input routines)
 * are generated: NDSP4Fighter.computer_*. */
typedef struct NDSP4Computer
{
    /* ai_attack_prevent: per detected attack, in place of the parent's
     * fkind switch (the jump table at 0x801334E4); NDS_P4_COMPUTER_*. */
    s32 (*prevent)(FTStruct *fp, s32 input_kind);
    /* recovery_logic: after the recover objective's walk. */
    void (*recover)(FTStruct *fp);
    /* cpu_post_process: after the objective, before the inputs run. */
    void (*post_process)(FTStruct *fp);
    /* AI.asm usp_check_: the input interpreter's Fox up-special test
     * (ftcomputer.c:3575) takes this content, as it takes Fox (Falco). */
    u8 fox_usp_check;
} NDSP4Computer;

/* ai_long_range: the parent's own case, or one of the two cases of the
 * source's fkind switch in func_ovl3_80138AA8 (0x80138ECC no long-range
 * special; 0x80138D24 the projectile users' walk-and-shoot). */
enum
{
    NDS_P4_COMPUTER_LONG_RANGE_PARENT,
    NDS_P4_COMPUTER_LONG_RANGE_NONE,
    NDS_P4_COMPUTER_LONG_RANGE_PROJECTILE
};

/* P4: the parent's routines a content replaces, where Remix hooks the
 * parent's code on the content's character id. The effect makers (S6) build
 * the content's own effect from its own special files (the generator's
 * OWN_SPECIAL_FILES, g<Ident>Special<n>) in place of the parent's desc, file
 * and offsets. NULL keeps the parent's routine. Defined in
 * src/port/nds_p4_<name>.c as gNdsP4<Title>Overrides, weak, so a content
 * without one gets NULL. */
typedef struct NDSP4Overrides
{
    /* efManagerFoxReflectorMakeEffect (Fox's reflector statuses). */
    GObj *(*fox_reflector)(GObj *fighter_gobj);
    /* efManagerFoxEntryArwingMakeEffect (ftCommonAppearSetStatus). */
    GObj *(*fox_entry_arwing)(FTStruct *fp, Vec3f *pos, s32 lr);
    /* NDS_P4_ENTRY_PORT: the ftCommonAppearSetStatus case the content's
     * entry_script names, on its own files (Bowser's Clown Copter in the
     * Falcon Flyer's case). */
    void (*entry_case)(FTStruct *fp);
    /* ftCommonThrowSetStatus's forward throw takes Kirby's path, airborne,
     * with this status (Bowser's 0xE5; 0: the parent's ground throw). */
    s32 throw_f_kirby_status;
    /* ftMainProcPhysicsMap, after the map proc, every frame (Bowser's flame
     * recharge). */
    void (*after_proc_map)(GObj *fighter_gobj);
    /* ftManagerDestroyFighterWeapons, on every fall (Bowser's refill). */
    void (*on_dead)(GObj *fighter_gobj);
    /* ftYoshiSpecialLwLandingProcUpdate makes no stars (Bowser). */
    u8 no_yoshi_lw_stars;
} NDSP4Overrides;

/* A Remix CPU input routine (AI.asm add_cpu_input_routine) as assembled:
 * its id and its bytes in the content's ComputerScriptBytes. */
typedef struct NDSP4ComputerScript
{
    u16 input;
    u16 offset;
    u16 length;
    u16 reserved;
} NDSP4ComputerScript;

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
    nNDSP4ComputerInputPointStickToTarget = 0x49,
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
    NDS_P4_ENTRY_PARENT, /* the parent's case, on parent-layout files */
    NDS_P4_ENTRY_PORT    /* another kind's case, ported: NDSP4Overrides.entry_case */
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

/* Jab rows: Remix's jab_3, jab_3_timer, jab_3_action, rapid_jab and the
 * rapid jab's four action rows, which replace the source's kind tests in
 * ftcommonattack1.c and ftcommonattack100.c. A content runs those files'
 * P4 copy (battleship_ftcommon_jab_p4.c), which reads these in place of
 * the kind tests Remix tabled and fails the others, as its own id does. A
 * zero row is DISABLED: the switch's end, where the N64 reads a stale
 * register for the status. */
typedef struct NDSP4Jab
{
    f32 jab3_followup;       /* jab_3_timer: frames left for the third jab */
    u16 jab3_status;         /* jab_3_action */
    u8 jab3;                 /* jab_3: ENABLED */
    u8 rapid;                /* rapid_jab: ENABLED */
    u16 rapid_count_status;  /* rapid_jab_unknown: the status A presses count in */
    u8 rapid_inputs;         /* ... and how many start the rapid jab */
    u8 reserved;
    u16 rapid_start;         /* rapid_jab_begin_action */
    u16 rapid_loop;          /* rapid_jab_loop_action */
    u16 rapid_end;           /* rapid_jab_ending_action */
} NDSP4Jab;

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
    /* The content's files in NitroFS (nitro:/reloc/p4/<id>), ascending. */
    const u16 *assets;
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
    const NDSP4Jab *jab;
    /* yoshi_egg: the egg Yoshi lays around it, an ftCommonYoshiEggDesc. */
    const f32 *yoshi_egg;
    /* Lab builds: donor special files a stand-in replaces (never loaded). */
    const u16 *lab_skip_files;
    const u32 *lab_skip_file_count;
    /* CPU rows from the export: the attack list (count 0: the parent's),
     * NDS_P4_COMPUTER_LONG_RANGE_*, the Remix input routines it names. */
    const FTComputerAttack *computer_attacks;
    const u32 *computer_attack_count;
    const u8 *computer_long_range;
    const NDSP4ComputerScript *computer_scripts;
    const u32 *computer_script_count;
    const u8 *computer_script_bytes;
    /* Optional: the parent's routines it replaces (Remix's id hooks). */
    const NDSP4Overrides *overrides;
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

/* A vanilla `x->fkind == nFTKind<Parent>` compare that Remix leaves alone
 * sees a Remix fighter's own character id on the N64, so the fighter takes
 * the other branch (Bowser guards with the common shield, and his grab and
 * throws leave the victim visible). A content carries its parent's fkind
 * here, so around a decomp include whose every use of the parent's constant
 * is such a compare on one fighter variable, the wrapper redefines it:
 *
 *     #define nFTKindYoshi NDS_P4_PARENT_KIND(fp, nFTKindYoshi)
 *
 * and a content compares unequal (the constant inside the expansion is not
 * expanded again). scripts/p4/parent_checks.py lists the compares and the
 * contents Remix's hooks on them name. */
#if NDS_P4
/* An fkind past every vanilla kind (FTKind ends at nFTKindGDonkey), as a
 * Remix fighter's own id is to the original code. */
#define NDS_P4_FOREIGN_FKIND 0x7F
#define NDS_P4_PARENT_KIND(fp_, kind_) \
    (((fp_)->nds_p4_content != 0u) ? NDS_P4_FOREIGN_FKIND : (s32)(kind_))
#else
#define NDS_P4_PARENT_KIND(fp_, kind_) (kind_)
#endif

#if NDS_P4

const NDSP4Fighter *ndsP4Fighter(u32 content);
static inline u32 ndsP4Content(const FTStruct *fp)
{
    return (fp != NULL) ? fp->nds_p4_content : 0u;
}
/* Each compiled content's id by name (NDS_P4_ID_WOLF), for a content's own
 * routines where Remix tests its character id (Character.id.WOLF). */
enum
{
#define NDS_P4_ID_ROW(id_, T_, N_, n_, parent_, model_, main_) NDS_P4_ID_##N_ = (id_),
    NDS_P4_CONTENT_ROWS(NDS_P4_ID_ROW)
#undef NDS_P4_ID_ROW
    NDS_P4_ID_NONE = 0
};
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
/* The content's effect makers (S6), or NULL. */
const NDSP4Overrides *ndsP4Overrides(const FTStruct *fp);
/* ftMainProcPhysicsMap after the map proc (battleship_ftmain.c): the
 * content's after_proc_map, at the cost of one load for everyone else. */
void ndsP4AfterProcMapSlow(GObj *fighter_gobj);
static inline void ndsP4AfterProcMap(GObj *fighter_gobj)
{
    if (__builtin_expect(ftGetStruct(fighter_gobj)->nds_p4_content != 0u, 0))
    {
        ndsP4AfterProcMapSlow(fighter_gobj);
    }
}
/* ftManagerDestroyFighterWeapons in the dead statuses: the content's on_dead
 * after the source's. */
void ndsP4OnDead(GObj *fighter_gobj);
/* The content's jab rows (all DISABLED for none). */
const NDSP4Jab *ndsP4JabRows(const FTStruct *fp);
/* The content's yoshi_egg row (7 words), or NULL for the original cast. */
const f32 *ndsP4YoshiEggRow(const FTStruct *fp);
/* The jab and rapid-jab files' P4 copy (battleship_ftcommon_jab_p4.c): the
 * source's entry points for a content; the source's copies send it here. */
void ndsP4JabAttack11ProcUpdate(GObj *fighter_gobj);
void ndsP4JabAttack12ProcUpdate(GObj *fighter_gobj);
void ndsP4JabAttack13ProcUpdate(GObj *fighter_gobj);
void ndsP4JabAttack11ProcInterrupt(GObj *fighter_gobj);
void ndsP4JabAttack12ProcInterrupt(GObj *fighter_gobj);
void ndsP4JabAttack13ProcInterrupt(GObj *fighter_gobj);
void ndsP4JabAttack11ProcStatus(GObj *fighter_gobj);
void ndsP4JabAttack11SetStatus(GObj *fighter_gobj);
void ndsP4JabAttack12SetStatus(GObj *fighter_gobj);
void ndsP4JabAttack13SetStatus(GObj *fighter_gobj);
sb32 ndsP4JabAttack1CheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsP4JabAttack11CheckGoto(GObj *fighter_gobj);
sb32 ndsP4JabAttack12CheckGoto(GObj *fighter_gobj);
sb32 ndsP4JabAttack13CheckGoto(GObj *fighter_gobj);
void ndsP4JabAttack100StartProcUpdate(GObj *fighter_gobj);
void ndsP4JabAttack100StartSetStatus(GObj *fighter_gobj);
void ndsP4JabAttack100LoopKirbyUpdateEffect(FTStruct *fp);
void ndsP4JabAttack100LoopProcUpdate(GObj *fighter_gobj);
void ndsP4JabAttack100LoopProcInterrupt(GObj *fighter_gobj);
void ndsP4JabAttack100LoopSetStatus(GObj *fighter_gobj);
void ndsP4JabAttack100EndSetStatus(GObj *fighter_gobj);
sb32 ndsP4JabAttack100StartCheckInterruptCommon(GObj *fighter_gobj);
/* ftYoshiSpecialLwLandingProcUpdate's stars, unless the content has none. */
GObj *ndsP4YoshiStarMakeStars(GObj *fighter_gobj, Vec3f *pos);
/* ftCommonThrowSetStatus's one ftMainSetStatus (battleship_ftcommon_catch.c):
 * a content's forward throw on Kirby's airborne path. */
void ndsP4ThrowMainSetStatus(GObj *fighter_gobj, s32 status_id, f32 frame_begin,
                             f32 anim_speed, u32 flags);
/* ftCommonSpecial*CheckInterruptCommon: run `check` with the content's
 * special-move starters lent to the source tables' rows for its kind
 * (tables[i][fkind] takes starter slots[i]), then restore them. */
sb32 ndsP4CheckSpecialLent(GObj *fighter_gobj, sb32 (*check)(GObj *),
                           NDSP4SpecialStart *const *tables,
                           const u8 *slots, u32 count);
/* Lab builds: the starter of an unported donor special (starts nothing).
 * The count also takes per-fighter stand-ins inside ported routines. */
void ndsP4LabSpecialStandIn(GObj *fighter_gobj);
extern volatile u32 gNdsP4LabSpecialStandIns;
/* ftMainSetStatus prologue: Remix change_action_ resets. */
void ndsP4OnSetStatus(GObj *fighter_gobj);
/* CPU (src/import/battleship_ftcomputer*.c). The content's hand-ported
 * routines, or NULL for the original cast. */
const NDSP4Computer *ndsP4Computer(const FTStruct *fp);
/* The content's generated attack list, or NULL (the parent's). */
const FTComputerAttack *ndsP4ComputerAttacks(const FTStruct *fp);
/* NDS_P4_COMPUTER_LONG_RANGE_*; PARENT for the original cast. */
u32 ndsP4ComputerLongRange(const FTStruct *fp);
/* ftComputerFollowObjectiveRecover, after its walk: the content's CPU
 * recovery_logic (Remix AI.asm custom_recovery_logic). */
void ndsP4ComputerRecover(FTStruct *fp);
/* ftComputerProcessAll, after the objective (AI.asm cpu_post_process). */
void ndsP4ComputerPostProcess(FTStruct *fp);
/* ftComputerSetCommand* that also takes Remix's input routine ids. */
void ndsP4ComputerSetCommandWaitShort(FTStruct *fp, s32 index);
void ndsP4ComputerSetCommandImmediate(FTStruct *fp, s32 index);
/* ftComputerUpdateInputs for a P4 content: the source's interpreter with
 * Remix's stick-X values and custom commands, run on every script the
 * content's CPU issues; FALSE (the original cast) leaves the tick to the
 * source's. */
sb32 ndsP4ComputerRunInputs(FTStruct *this_fp);
/* After a content's motion file loads: bind menu-motion script pointers. */
void ndsP4BindMenuScripts(u32 content);
/* NitroFS path for a P4 file id, or NULL. */
const char *ndsP4RelocAssetPath(u32 file_id);
/* Lab builds: a P4 file's dependency on a donor special file that a lab
 * stand-in replaces. The loader neither sizes nor loads it and leaves the
 * slots naming it NULL (the main's header words, its only readers). */
sb32 ndsP4LabSkipsDependency(u32 owner_asset, u32 dep_asset);
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

/* Remix hitbox overrides (Command.asm 0xD2 OVERRIDE HITBOX DIRECTION, 0xD8
 * SET HITBOX FGM), per port and hitbox. Remix clears a hitbox's pair when the
 * source makes that hitbox (create_hitbox_) and all four when it clears them
 * all (end_hitbox_). Only a hitbox the source made reads its pair, so the
 * make-time clear alone gives the same reads; attack_id 4 or more clears
 * all. Contents only. */
void ndsP4ResetHitboxOverrides(const FTStruct *fp, u32 attack_id);
/* ftMainParseMotionEvent's make-hitbox reads (battleship_ftmain.c): the
 * hitbox id is bits 23-25 of the event's first word. */
static inline void *ndsP4MakeAttackCursor(const FTStruct *fp,
                                          FTMotionScript *ms)
{
    if (__builtin_expect(ndsP4Content(fp) != 0u, 0))
    {
        ndsP4ResetHitboxOverrides(fp, (*(const u32 *)ms->p_script >> 23) & 7u);
    }
    return ms->p_script;
}
/* ftMainPlayHitSFX's sound (apply_fgm_): the attacker's override for that
 * hitbox replaces the source's, or with bit 15 plays after it. */
void ndsP4MakeHitPositionFGM(FTStruct *attacker_fp, FTAttackColl *attack_coll,
                             u16 fgm_id, f32 pos_x);
/* ftMainProcessHitCollisionStatsMain, once the victim's damage_lr is set
 * (apply_direction_, fighter hits only): forward launches away from the
 * attacker's facing, backward along it; then the source's stats call. */
void ndsP4UpdateHitDamageStats(const FTHitLog *hitlog, FTStruct *fp,
                               s32 damage_player, s32 damage_object_class,
                               s32 damage_object_kind, u16 flags,
                               u16 damage_stat_count);

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
