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

/* Remix's reflect AI (Reflect.asm, AI.asm), per content: TABLE is its
 * Character.fighter_reflect row (CPUs hold their projectiles against a
 * reflector, and its own CPU flags item hazards to reflect); FOX is Fox's
 * branch in the reflect hooks (extend_projectile_reflect_initial_,
 * maintain_reflect_input_, apply_reflect_input_) and in the long-range
 * special's reflector-target roll (improve_remix_charged_NSP_2). Remix
 * compares the content's own id everywhere else, so the parent's id
 * compares at those sites see a foreign kind (generate_p4_fighter.py). */
#define NDS_P4_COMPUTER_REFLECT_TABLE 0x01u
#define NDS_P4_COMPUTER_REFLECT_FOX 0x02u

/* A texture pointer in one of the content's own files whose image the
 * native entry packet already carries (scripts/3d_vfx/
 * generate_nds_entry_effects.py --p4): the loader resolves it to NULL and
 * never loads `dep` for it, as for Fox's Arwing (reloc_backend_assets.c
 * ndsRelocResolveNativeEntryExternalFixup). */
typedef struct NDSP4BakedRef
{
    u16 owner;
    u16 dep;
    u32 slot;
    u32 target;
} NDSP4BakedRef;

/* A content's native guard-pose package (scripts/p4/p4_shield_pose.py,
 * src/nds/nds_shield_pose.c): its own NSP1 blob at
 * nitro:/fighters/shield_pose/<NDS_P4_SEL_BASE + content>.bin, or its
 * parent's package when it guards with the parent's vanilla file at the
 * same nine targets (alias_fkind), or neither (blob_bytes 0, alias -1: the
 * raw file stays). main_fixup_slots are the content Main's nine guard
 * pointers: dobj_lookup, then shield_anim_joints[0..7]. */
#define NDS_P4_SHIELD_POSE_MAX_BASE_COUNT 40u
#define NDS_P4_SHIELD_POSE_MAX_SCRATCH_WORDS 512u
typedef struct NDSP4ShieldPose
{
    u16 blob_bytes;
    u16 main_asset;
    u16 shield_asset;
    u16 dobj_offset;
    u16 table_offsets[8];
    u16 main_fixup_slots[9];
    s8 alias_fkind;
    u8 base_count;
    u16 scratch_words;
} NDSP4ShieldPose;

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
    /* ftParamUpdateDamage's head: the damage the hit deals (Marth's
     * counter takes it to 0 and marks the hit). */
    s32 (*update_damage)(FTStruct *fp, s32 damage);
    /* ftMainSetHitInteractStats's head, fp the attacker (Wario.asm
     * body_slam_recoil_: Wario's Body Slam, Sheik's and Banjo's recoils). */
    void (*on_hit_interact)(FTStruct *fp, s32 attack_type);
    /* ftPhysicsApplyGravityClampTVel's head: TRUE when the content holds
     * the fighter this frame, no gravity (Peach's float,
     * PeachFloat.handle_physics_). */
    sb32 (*gravity)(FTStruct *fp);
    /* The Jump, JumpAerial, Fall and Pass interrupts, between the aerial
     * check and the jump check: TRUE when the content changed the status
     * (Peach's float start, PeachFloat.check_float_). */
    sb32 (*air_jump_check)(GObj *fighter_gobj);
    /* ftCommonFallSetStatus' status change: TRUE when the content set its
     * own status instead (Peach floats on, PeachFloat.fall_override_). */
    sb32 (*fall_status)(GObj *fighter_gobj);
    /* ftCommonAttackAirCheckInterruptCommon: TRUE keeps a held item from
     * being thrown (Peach while floating, PeachFloat.prevent_item_throw_). */
    sb32 (*air_item_throw_block)(FTStruct *fp);
    /* ftParamStopVoiceRunProcDamage's head (Peach's float ends,
     * PeachFloat.end_float_on_hit_). */
    void (*on_damage)(FTStruct *fp);
    /* mpCommonSetFighterLandingParams after its kind case: the routine
     * Remix's grounded_script row names in place of a case; cliff is TRUE
     * for a ledge catch (Peach.grounded_script_ and ledge_patch_). */
    void (*on_landing)(FTStruct *fp, sb32 cliff);
    /* ftCommonFallSpecialSetStatus' status: the content's in place of
     * FallSpecial (Peach's parasol fall, PeachUSP.fall_special_patch_). */
    s32 (*fall_special_status)(FTStruct *fp, s32 status_id);
    /* ftMainProcPhysicsMap's head (Remix's Size.asm copies the attributes'
     * collision box every frame; Crash's dig changes his). */
    void (*before_physics_map)(GObj *fighter_gobj);
    /* ftMainSearchFighterCatch: FALSE keeps that hurtbox of the victim from
     * being grabbed (Crash's dig, Hitbox.asm force_grab_immunity_). */
    sb32 (*grabbable)(FTStruct *victim_fp, s32 damage_coll_id);
    /* The aerial start's down-air hit routine, in place of the source's
     * (Link's rehit case; Crash.asm dair_bounce_). */
    void (*attack_air_lw_hit)(GObj *fighter_gobj);
    /* ftCommonLightGetProcDamage's Maxim Tomato (Crash.asm crash_eat_sfx). */
    void (*on_eat_tomato)(FTStruct *fp);
    /* ftCommonDamageInitDamageVars' status change: the status the hit sets
     * and, for an electric hit, the one it leads to after (Lanky's balloon
     * damage, LankyUSP.damage_patch_). */
    s32 (*damage_status)(FTStruct *fp, s32 status_id, s32 *status_id_after);
    /* ftCommonSpecialAirCheckInterruptCommon's head: TRUE keeps every
     * aerial special from starting (Sonic's spent spring,
     * SonicUSP.action_check_patch_). */
    sb32 (*air_special_block)(FTStruct *fp);
    /* Non-NULL: the aerial jumps take Kirby's multi-jump branches with these
     * JumpAerialF2..F5 heights in place of his (jigglypuffkirbyshared.asm
     * jump_fix_1-5 on Dedede's id, Dedede.jump_multiplier_table). */
    const f32 *multi_jump_vel;
    /* ftManagerInitFighter's kind switch: Remix's initial_script routine
     * (spawn and rebirth; Dedede.initial_script_). */
    void (*on_init)(FTStruct *fp);
    /* ftkirbyspecialn.c's status changes: the content's status for each of
     * Kirby's inhale statuses (Remix's DededeNSP hooks). */
    s32 (*kirby_inhale_status)(s32 status_id);
    /* ftCommonCaptureWaitKirbySetStatus' breakout wait when this content
     * holds the victim (Dedede.custom_initial_absorbed_timer_). */
    s32 (*kirby_capture_wait)(FTStruct *victim_fp, s32 breakout_wait);
    /* efManagerKirbyInhaleWindProcUpdate's distance ahead of the fighter,
     * 0 for Kirby's (Dedede.dedede_gfx_). */
    f32 kirby_inhale_wind_x;
    /* ftMainProcParams' reflect switch, a special_coll of Remix's custom
     * kind (Reflect.asm extend_reflect_types, custom_reflect_table). */
    void (*on_custom_reflect)(GObj *fighter_gobj);
} NDSP4Overrides;

/* Weapon kinds past the source's last (nWPKindMonsterEnd, wp/weapon.h), one
 * per content weapon: the DS renderer keys its native weapon owners on the
 * kind, and Remix's projectile ids reuse the source's. */
#define NDS_P4_WP_KIND_WOLF_BLASTER (nWPKindMonsterEnd + 1)
#define NDS_P4_WP_KIND_SHEIK_NEEDLE (nWPKindMonsterEnd + 2)
#define NDS_P4_WP_KIND_BANJO_EGG (nWPKindMonsterEnd + 3)
#define NDS_P4_WP_KIND_LANKY_GRAPE (nWPKindMonsterEnd + 4)
#define NDS_P4_WP_KIND_SONIC_SPRING (nWPKindMonsterEnd + 5)

/* Remix's custom reflect kind (Reflect.asm reflect_type.CUSTOM): the low
 * half of FTSpecialColl.kind (the N64's second halfword), the routine index
 * in the high half; index 0 is the Franklin Badge's plain reflect. */
#define NDS_P4_SPECIAL_COLL_CUSTOM 3
#define NDS_P4_SPECIAL_COLL_CUSTOM_KIND(index_) \
    (((s32)(index_) << 16) | NDS_P4_SPECIAL_COLL_CUSTOM)

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

/* A content's match tables (generate_p4_fighter.py, nds_p4_<name>.tables.c):
 * its motion descriptors, CPU attack list and the Remix input routines the
 * list names, objcopied into nitro:/p4/<name>.tab and read into the scene
 * heap for the contents a scene makes fighters of (ndsP4LoadTables), so the
 * fourteen contents' tables stop costing every scene's arena. Offsets are
 * from the header; nothing inside is a pointer. */
#define NDS_P4_TABLES_MAGIC 0x31543450u /* "P4T1" */
#define NDS_P4_TABLES_VERSION 1u
typedef struct NDSP4TablesHeader
{
    u32 magic;
    u32 version;
    u32 bytes;
    u32 motion_off;       /* FTMotionDesc[motion_count]: FTData.mainmotion */
    u32 motion_count;
    u32 attack_off;       /* FTComputerAttack[attack_count]; 0: the parent's */
    u32 attack_count;
    u32 script_off;       /* NDSP4ComputerScript[script_count] */
    u32 script_count;
    u32 script_bytes_off;
} NDSP4TablesHeader;

/* Remix's per-kind jump tables whose rows name a case of a source fkind
 * switch (S3): mpCommonSetFighterLandingParams' (grounded_script) as the
 * vanilla kind whose case runs, NDS_P4_FOREIGN_FKIND for none; and
 * pipe_turn, Mario's turn in the Dokan statuses (marioshared.asm). */
typedef struct NDSP4KindCases
{
    s8 grounded;
    u8 pipe_turn;
    u8 pad[2];
} NDSP4KindCases;

/* Remix's kirby_inhale_struct row for this fighter (Character.asm): the power
 * and hat an inhaling Kirby takes (Remix character ids, S8), and the star he
 * spits it out as, its scale and damage. */
typedef struct NDSP4KirbyInhale
{
    s16 copy_id;
    s16 hat_id;
    f32 star_scale;
    s32 star_damage;
} NDSP4KirbyInhale;

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
    /* Lab builds: special slots (bit 0 = special1) whose stand-in, the
     * parent's file, loads whole; 0 when shipping. */
    const u8 *open_special_mask;
    const NDSP4KindCases *kind_cases;
    const NDSP4KirbyInhale *kirby_inhale;
    /* CPU rows from the export: NDS_P4_COMPUTER_LONG_RANGE_*. The attack
     * list and the Remix input routines it names are in the content's
     * match tables (NDSP4TablesHeader). */
    const u8 *computer_long_range;
    /* NDS_P4_COMPUTER_REFLECT_* bits. */
    const u8 *computer_reflect;
    /* Entry-article texture pointers its native packets carry. */
    const NDSP4BakedRef *baked_refs;
    const u32 *baked_ref_count;
    /* Its guard-pose package (NDSP4ShieldPose). */
    const NDSP4ShieldPose *shield_pose;
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
/* ftMainSetHitInteractStats's head (its source calls and the port's
 * wrapper): the attacking content's on_hit_interact. */
void ndsP4OnHitInteractSlow(FTStruct *fp, s32 attack_type);
static inline void ndsP4OnHitInteract(FTStruct *fp, s32 attack_type)
{
    if (__builtin_expect(fp->nds_p4_content != 0u, 0))
    {
        ndsP4OnHitInteractSlow(fp, attack_type);
    }
}
/* The content's gravity hook (ftPhysicsApplyGravityClampTVel): TRUE skips
 * the source's gravity this frame. */
sb32 ndsP4GravitySlow(FTStruct *fp);
static inline sb32 ndsP4Gravity(FTStruct *fp)
{
    if (__builtin_expect(fp->nds_p4_content != 0u, 0))
    {
        return ndsP4GravitySlow(fp);
    }
    return FALSE;
}
/* The content's air_jump_check (battleship_ftcommon_jump.c, _fall.c,
 * _pass.c and the JumpAerial import): TRUE when it changed the status. */
sb32 ndsP4AirJumpCheck(GObj *fighter_gobj);
/* ftCommonFallSetStatus' status change through the content's fall_status. */
void ndsP4FallSetStatus(GObj *fighter_gobj, s32 status_id, f32 frame_begin,
                        f32 anim_speed, u32 flags);
/* ftCommonAttackAirCheckInterruptCommon's item-throw test: FALSE while the
 * content blocks it, else the source's. */
sb32 ndsP4AirLightThrowCheck(FTStruct *fp);
/* ftParamStopVoiceRunProcDamage's head: the content's on_damage. */
void ndsP4OnDamage(FTStruct *fp);
/* mpCommonSetFighterLandingParams: the content's on_landing. */
void ndsP4OnLanding(FTStruct *fp, sb32 cliff);
/* ftCommonFallSpecialSetStatus' status through the content's
 * fall_special_status. */
void ndsP4FallSpecialSetStatus(GObj *fighter_gobj, s32 status_id, f32 frame_begin,
                               f32 anim_speed, u32 flags);
/* ftMainProcPhysicsMap's head (battleship_ftmain.c): the content's
 * before_physics_map. */
void ndsP4BeforePhysicsMapSlow(GObj *fighter_gobj);
static inline void ndsP4BeforePhysicsMap(GObj *fighter_gobj)
{
    if (__builtin_expect(ftGetStruct(fighter_gobj)->nds_p4_content != 0u, 0))
    {
        ndsP4BeforePhysicsMapSlow(fighter_gobj);
    }
}
/* ftMainSearchFighterCatch's hurtbox test (battleship_ftmain.c): FALSE when
 * the victim's content keeps that hurtbox from a grab, else the source's
 * collision test. */
sb32 ndsP4CatchDamageCollide(FTStruct *victim_fp, FTAttackColl *attack_coll,
                             FTDamageColl *damage_coll);
/* ftCommonAttackAirCheckInterruptCommon's aerial start, before its events
 * (battleship_ftcommon_attackair.c): a content's down-air hit routine. */
void ndsP4AttackAirStart(GObj *fighter_gobj);
/* ftCommonLightGetProcDamage's Maxim Tomato: the content's on_eat_tomato. */
void ndsP4OnEatTomato(FTStruct *fp);
/* ftCommonDamageInitDamageVars' status change through the content's
 * damage_status (battleship_ftcommon_damage.c). */
s32 ndsP4DamageStatus(GObj *fighter_gobj, s32 status_id, s32 *status_id_after);
/* ftCommonSpecialAirCheckInterruptCommon's head: the content's
 * air_special_block (battleship_special_common.c). */
sb32 ndsP4AirSpecialBlocked(GObj *fighter_gobj);
/* The content's multi_jump_vel (battleship_ftcommon_normal_moveset.c), NULL
 * for every other fighter. */
const f32 *ndsP4MultiJumpVelocities(const FTStruct *fp);
/* ftManagerInitFighter's kind switch, for a content: its on_init
 * (battleship_ftmanager.c at spawn, battleship_ftcommon_rebirth.c). */
void ndsP4OnInitFighter(GObj *fighter_gobj);
/* ftkirbyspecialn.c's status changes (battleship_kirby.c): the content's
 * kirby_inhale_status. */
s32 ndsP4KirbyInhaleStatus(GObj *fighter_gobj, s32 status_id);
/* Kirby's copy table, the rows of his main motion file's
 * (relocData/228_KirbyMainMotion.c), for the inhale code a content runs
 * without Kirby's file in the match (battleship_kirby.c,
 * battleship_ftcommon_capturekirby.c, battleship_efmanager.c). */
void *ndsP4KirbyCopyFile(void);
/* ftCommonCaptureWaitKirbySetStatus' breakout wait: the holder content's
 * kirby_capture_wait (battleship_ftcommon_capturekirby.c). */
s32 ndsP4KirbyCaptureWait(FTStruct *victim_fp, s32 breakout_wait);
/* Remix's kirby_inhale_struct star damage of a content victim, -1 for an
 * original fighter (the copy table's row stands). */
s32 ndsP4KirbyStarDamage(const FTStruct *victim_fp);
/* efManagerKirbyInhaleWindProcUpdate's distance ahead (battleship_efmanager.c). */
f32 ndsP4KirbyInhaleWindX(GObj *fighter_gobj, f32 source_x);
/* TRUE when the fighter's special_coll is Remix's custom kind past the
 * Franklin Badge's index: the absorbers (Dedede's inhale), whose catch
 * destroys a weapon or item in place of reflecting it
 * (battleship_wpmanager_core.c, battleship_item_link_core.c). */
sb32 ndsP4CustomAbsorber(GObj *fighter_gobj);
/* ftMainProcParams' reflect switch for a custom special_coll: the content's
 * on_custom_reflect (battleship_ftmain.c). */
void ndsP4OnCustomReflect(GObj *fighter_gobj);
/* gcEjectGObj's head: Sonic's homing target let go (battleship_sys_objman.c,
 * src/port/nds_p4_sonic.c). */
void ndsP4SonicOnEjectGObj(GObj *gobj);
/* The content's match tables, read into this scene's heap at its first use
 * (FTData.mainmotion points into them from then on); halts on a missing or
 * malformed file, a build defect. */
const NDSP4TablesHeader *ndsP4LoadTables(u32 content);
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
/* Owner 2026-10-08 (S15 memory): a content in a battle of three or four
 * fighters draws its low-detail model everywhere, the source's KO and pause
 * close-ups included, and its high-detail owner image is never admitted.
 * TRUE when the player is such a content. */
sb32 ndsP4ContentLowDetailOnly(s32 player);
/* S3 kind-table cases (NDSP4KindCases). The kind whose case of
 * mpCommonSetFighterLandingParams' switch runs for the fighter (its own for
 * the original cast), and, for a content, the kind the Dokan statuses'
 * Mario compares see: its own when Remix's pipe_turn names it, else
 * NDS_P4_FOREIGN_FKIND. */
s32 ndsP4GroundedKind(const FTStruct *fp);
s32 ndsP4PipeTurnKind(const FTStruct *fp);
#define NDS_P4_PIPE_TURN_KIND(fp_, kind_) \
    (((fp_)->nds_p4_content != 0u) ? ndsP4PipeTurnKind(fp_) : (s32)(kind_))
/* TRUE in a VS battle of three or four fighters: every content there draws
 * only its low detail, so it loads its low-detail battle pack
 * (fighters/battle/<kind>.fpc, p4_preview_pack.py --battle-out). */
sb32 ndsP4LowDetailBattle(void);
/* TRUE on the VS select: a content's preview draws its low detail (owner
 * 2026-10-08, lower content fidelity; a preview panel is 42 pixels wide), from
 * the battle pack and the low owner image. */
sb32 ndsP4LowDetailSelect(void);
/* NDS_P4_COMPUTER_REFLECT_* bits; 0 for the original cast. */
u32 ndsP4ComputerReflect(const FTStruct *fp);
/* The fighter as the CPU reflect checks see it: Fox for a content with
 * Fox's branch, NDS_P4_FOREIGN_FKIND for any other content, else its kind. */
s32 ndsP4ComputerReflectKind(const FTStruct *fp);
/* ftComputerCheckDetectTarget's reflector test on the target (Remix's
 * fighter_reflect row: Fox and Ness for the original cast). */
sb32 ndsP4ComputerTargetReflects(const FTStruct *fp);
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
/* A content file's dependency named only by texture pointers its native
 * entry packets carry (NDSP4BakedRef): never loaded in a match. */
sb32 ndsP4NativeOwnsDependency(u32 owner_asset, u32 dep_asset);
/* One such pointer: TRUE resolves it to NULL. */
sb32 ndsP4NativeBakedRef(u32 owner_asset, u32 dep_asset, u32 slot, u32 target);
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
