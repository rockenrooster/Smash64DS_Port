#ifndef SSB64_NDS_P7_H
#define SSB64_NDS_P7_H

#include <PR/ultratypes.h>

/* P7 (docs/P7/P7_Implementation_Plan.md): unlocks beyond the source's, kept
 * in a small progress block of the save image beside the source's two copies
 * of LBBackupData (U1), and the selection ids every unlock and record uses:
 * 0-11 the source's playable kinds, 12 Metal Mario, 12 + content for a
 * content (scripts/p4/contents.json; Ike 31, Palutena 32), up to 39. */
#define NDS_P7_SEL_METAL 12u
#define NDS_P7_SEL_COUNT 40u
#define NDS_P7_SEL_CONTENT(content) (12u + (u32)(content))

/* The stages P7 unlocks (the P5 stages, not gkinds: the source's ground mask
 * stays the source's). */
#define NDS_P7_STAGE_COUNT 8u

/* Boot: after the source block is validated (lbBackupIsSramValid), the
 * progress block's valid copy, or the defaults (nothing new unlocked) when
 * neither copy is valid -- the source block is never touched by that. */
void ndsP7ProgressLoad(void);

/* The masks. A write updates both copies in the image; the caller commits
 * with lbBackupWrite (one flush for the source block and this). */
u32 ndsP7FighterUnlocked(u32 sel);
void ndsP7SetFighterUnlocked(u32 sel);
u32 ndsP7StageUnlocked(u32 stage);
void ndsP7SetStageUnlocked(u32 stage);
u32 ndsP7Cheated(void);
/* The masks as a wireless host sends them (A1): every bit in a dev-open
 * build. */
void ndsP7Masks(u32 fighters[2], u32 *stages);

/* U8: SELECT on VS Options: every source unlock, every P7 fighter and stage,
 * and the cheat flag. FALSE when everything was already open. */
u32 ndsP7UnlockAll(void);

/* Backup Clear's classes (lbbackup.c lbBackupClear*): All Data resets the
 * block, Newcomers locks the new fighters, Prize the new stages. */
void ndsP7ClearAll(void);
void ndsP7ClearFighters(void);
void ndsP7ClearStages(void);

/* U3: the unlock registry (scripts/p7/unlocks.json, generated into
 * src/nds/nds_p7_unlocks.generated.inc; src/nds/nds_p7_unlocks.c). A row's
 * site is where its condition is checked; a run's is where it is. */
#define NDS_P7_SITE_RUN_END 0u      /* the 1P Game cleared */
#define NDS_P7_SITE_STAGE_CLEAR 1u  /* a 1P stage cleared */
#define NDS_P7_SITE_BONUS1_END 2u   /* a Break the Targets practice finished */
#define NDS_P7_SITE_BONUS2_END 3u   /* a Board the Platforms practice finished */
#define NDS_P7_SITE_UNLOCK 4u       /* a row's: another row's unlock (a companion) */
#define NDS_P7_SITE_BONUS_END 5u    /* a row's: either practice */
#define NDS_P7_UNLOCK_FIGHTER 0u
#define NDS_P7_UNLOCK_STAGE 1u
#define NDS_P7_DELIVERY_DIRECT 0u     /* a message, no fight */
#define NDS_P7_DELIVERY_CHALLENGER 1u /* a challenger fight, then the message */
#define NDS_P7_DELIVERY_COMPANION 2u  /* its message after the row that opens it */
#define NDS_P7_ANY 0xFFu
#define NDS_P7_NO_RACE 0xFFFFu
#define NDS_P7_BONUS_TASKS 10u      /* every target, every platform */

typedef struct NDSP7UnlockRow
{
    u8 kind;                /* NDS_P7_UNLOCK_* */
    u8 id;                  /* selection id, or stage unlock id */
    u8 site;                /* NDS_P7_SITE_* */
    u8 stage;               /* stage clear: the 1P stage (nSC1PGameStage*) */
    u8 min_difficulty;      /* 0 Very Easy .. 4 Very Hard */
    u8 max_continues;       /* NDS_P7_ANY: any */
    u8 max_falls;           /* stocks lost over the run */
    u8 max_stage_falls;     /* stocks lost in the stage cleared */
    u8 stock_setting;       /* the stocks the run started with */
    u8 bonus_targets;       /* its fighters' records: targets broken */
    u8 bonus_platforms;     /* platforms boarded */
    u8 all_cleared;         /* every original fighter has cleared the 1P Game */
    u8 all_targets;         /* every original fighter has broken every target */
    u8 delivery;            /* NDS_P7_DELIVERY_* */
    u8 challenger_gkind;    /* a challenger's stage */
    u8 content;             /* a fighter row's content (0: a source kind) */
    u8 stage_gkind;         /* a stage row's gkind */
    u16 max_time_s;         /* the run's time; 0xFFFF any */
    u16 min_race_left_s;    /* Race to the Finish's time left; 0 none */
    u64 fighters;           /* selection ids that may play it; 0 any */
    u64 requires;           /* selection ids unlocked before it */
} NDSP7UnlockRow;

/* One run at a trigger site: the source's 1P and bonus statistics. */
typedef struct NDSP7Run
{
    u8 site;                /* NDS_P7_SITE_RUN_END .. NDS_P7_SITE_BONUS2_END */
    u8 sel;                 /* the fighter played (selection id) */
    u8 difficulty;
    u8 stock_setting;
    u8 continues;
    u8 falls;
    u8 stage;               /* stage clear: the stage */
    u8 stage_falls;
    u16 time_s;
    u16 race_left_s;        /* NDS_P7_NO_RACE without one */
} NDSP7Run;

u32 ndsP7UnlockRowCount(void);
const NDSP7UnlockRow *ndsP7UnlockRow(u32 index);
/* The rows `run` meets -- not unlocked yet, their prerequisites held (this
 * run's earlier rows count), their content and stages compiled -- in table
 * order, then the companions they open: row indices into out (at most max),
 * their count. The save's 1P records are read as the run left them. */
u32 ndsP7UnlockMatch(const NDSP7Run *run, u8 *out, u32 max);
/* The companion rows (a stage that comes with a fighter) whose
 * prerequisites are all unlocked now and which are not yet: row indices into
 * out (at most max), their count. Asked after each delivered unlock. */
u32 ndsP7UnlockCompanions(u8 *out, u32 max);
/* A row still to deliver: not unlocked, compiled, its prerequisites
 * unlocked now (a queued challenger whose prerequisite was lost waits). */
u32 ndsP7UnlockPending(const NDSP7UnlockRow *row);
/* A row's unlock: its fighter or stage bit (the caller commits with
 * lbBackupWrite). */
void ndsP7UnlockDeliver(const NDSP7UnlockRow *row);
/* This build's contents and stages (nds_p7_unlocks.c). */
u32 ndsP7UnlockContentCompiled(u32 content);
u32 ndsP7UnlockStageCompiled(u32 gkind);

/* U4/U5: the P7 challenger on screen or fighting (the 1P manager's DS seam,
 * battleship_sc1pmanager.c). While it is active its content is the match's:
 * the challenger screen's fighter (player 0) and the 1P fight's enemy
 * (ndsP4MatchContent). The fight rides the source's first challenger rows
 * (Luigi's: one stock, no time limit, level {6,7,7,8,9} less the level drop,
 * handicap 6, items off, A4) with the row's stage and fighter in them
 * (battleship_sc1pgame_runtime.c). */
typedef struct NDSP7Challenger
{
    u8 active;
    u8 row;         /* its unlock row */
    u8 fkind;       /* the kind the source makes: a content's parent */
    u8 content;     /* its content; 0 for an original kind */
    u8 gkind;       /* the fight's stage */
} NDSP7Challenger;
extern NDSP7Challenger gNdsP7Challenger;
u32 ndsP7ChallengerContent(s32 player);
/* The bonus practice's end (battleship_sc1pbonusstage.c; `entry_prev`: the
 * scene the bonus stage was started from): its P7 rows; a challenger among
 * them routes the next scene to the 1P manager, which fights it and returns
 * to the practice menu. */
void ndsP7BonusPracticeEnded(u32 entry_prev);
/* U6: one message screen per unlock row, back to back (the source's message
 * scene with the row's lines; battleship_mnmessage.c). The caller sets the
 * scene to nSCKindMessage. */
void ndsP7ShowMessages(const u8 *rows, u32 count);

/* U2: wide VS records (src/nds/nds_p7_records.c). The source's
 * LBBackupVSRecord widened to every selection id, one row each, in a file
 * beside the save; the source's twelve-by-twelve block stays the source's. */
#define NDS_P7_RECORD_BYTES 260u
typedef struct NDSP7VSRecord
{
    u16 ko_count[NDS_P7_SEL_COUNT];
    u32 time_used;
    u32 damage_given;
    u32 damage_taken;
    u16 unk;
    u16 selfdestructs;
    u16 games_played;
    u16 player_count_tally;
    u16 player_count_tallies[NDS_P7_SEL_COUNT];
    u16 played_against[NDS_P7_SEL_COUNT];
} NDSP7VSRecord;
struct SCBattleState;
/* Row `sel` (zero when there is no file or no row). */
void ndsP7RecordsRead(u32 sel, NDSP7VSRecord *out);
/* A finished match (mnVSResultsSaveBackup's arithmetic): every player's row,
 * its opponents by their own selection ids (`sel` per player slot). */
void ndsP7RecordsAddMatch(const struct SCBattleState *state, const u8 sel[4]);
/* Backup Clear's VS Record class and All Data. */
void ndsP7RecordsClear(void);
/* A player's selection id: an original's kind, Metal Mario's, a content's. */
u32 ndsP7SelOf(s32 fkind, u32 content);

/* C3: wide 1P records (src/nds/nds_p7_records.c). The source's
 * LBBackup1PRecord (high score, continues, bonuses, best difficulty, both
 * bonus times and counts, completion) one row per selection id, in a second
 * file beside the save; the source's twelve rows stay the source's. A content
 * or Metal Mario plays the 1P game and the bonus stages under its parent's
 * kind, so for the run the parent's source row holds the player's own row:
 * every save writes the parent's row back in its place and the player's row
 * to this file (lbBackupWrite), and the run's end restores the parent's. */
#define NDS_P7_1P_RECORD_BYTES 32u
struct LBBackup1PRecord;
/* Row `sel` (zero when there is no file or no row). */
void ndsP7OnePRecordsRead(u32 sel, struct LBBackup1PRecord *out);
/* Rows [first, first + count) to `visit`, through one open of the file. */
void ndsP7OnePRecordsVisit(u32 first, u32 count,
                           void (*visit)(u32 sel, const struct LBBackup1PRecord *row,
                                         void *ctx),
                           void *ctx);
/* Backup Clear's 1P High Score (the run's fields) and Bonus Stage Time (the
 * bonus fields) classes, and All Data (every field). */
#define NDS_P7_1P_CLEAR_RUN 1u
#define NDS_P7_1P_CLEAR_BONUS 2u
#define NDS_P7_1P_CLEAR_ALL 3u
void ndsP7OnePRecordsClear(u32 fields);
/* The run of a content or Metal Mario (`sel`) under `parent_kind`'s row;
 * a no-op for an original. */
void ndsP7OnePRunBegin(s32 parent_kind, u32 sel);
void ndsP7OnePRunEnd(void);
/* lbBackupWrite, around the image write: the parent's own row in place,
 * then the player's back and into the file. */
void ndsP7OnePSaveBegin(void);
void ndsP7OnePSaveEnd(void);

/* Dev-open builds (the lab sweep, the four-CPU stress, both-CPU and
 * fast-logic harnesses, and walks) see every fighter and stage, as they
 * always have. By build flag: the shipping ROM's own loop is scene harness
 * 163, so gNdsSceneHarnessMode cannot tell a probe from a player. */
u32 ndsP7DevOpen(void);

#endif
