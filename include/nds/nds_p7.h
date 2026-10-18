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
/* This build's contents and stages (nds_p7_unlocks.c). */
u32 ndsP7UnlockContentCompiled(u32 content);
u32 ndsP7UnlockStageCompiled(u32 gkind);

/* Dev-open builds (the lab sweep, the four-CPU stress, both-CPU and
 * fast-logic harnesses, and walks) see every fighter and stage, as they
 * always have. By build flag: the shipping ROM's own loop is scene harness
 * 163, so gNdsSceneHarnessMode cannot tell a probe from a player. */
u32 ndsP7DevOpen(void);

#endif
