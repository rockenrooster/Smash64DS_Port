/* P2-6 step 1. 1P Game campaign driver (stage loop).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pmanager.c whole (587
 * lines: ally pick, Kirby copy, stage loop with intro/bonus/game/continue/
 * clear dispatch, challenger, ending), following battleship_sc1pbonusstage.c
 * (scene TU imported as ndsBase* and re-exported under its source name).
 *
 * Gated on NDS_P2_1P_GAME like battleship_sc1pgame_runtime.c. The scene entry
 * is imported as ndsBaseSC1PManagerUpdateScene and re-exported under its
 * source name, so the dispatch in battleship_scmanager.c
 * (case nSCKind1PGame) keeps calling sc1PManagerUpdateScene while the DS
 * seam stays reviewable. Helpers (GetFighterKindsNum, GetShuffledFighterKind,
 * GetShuffledKirbyCopy, TrySetChallengers, CheckUnlockSoundTest,
 * TrySaveBackup) and data (overlays, Kirby part IDs, challenger kinds,
 * unlock kinds, totals, level drop/guard) keep their source names: the port
 * defines none of them yet, so each has exactly one definition here.
 *
 * Shims vs unresolved (by reading; no compile per owner directive):
 * - syUtilsRandIntRange: shimmed below as a local extern (same line every
 *   other import TU carries; the port links it, no header publishes it).
 * - sc1PGameStartScene: shimmed below as a local extern; PROVIDED by the
 *   step-1 pair battleship_sc1pgame_runtime.c as a DS first-stage boot
 *   (Link/Hyrule through ndsMatchConfigApply). Full N64 taskman boot is NOT
 *   imported.
 * - sc1PIntroStartScene, sc1PBonusStageStartScene, sc1PStageClearStartScene,
 *   sc1PChallengerStartScene, mnPlayers1PGameContinueStartScene,
 *   mnMessageStartScene, mnCongraStartScene, mvEndingStartScene,
 *   scStaffrollStartScene: NOT provided here; they resolve to the
 *   title_backend.c stubs (BonusStage resolves to the step-5 real import when
 *   the flag is on). A stub that stays a stub stays: intro/continue/clear/
 *   challenger/ending/staffroll/congra/message halt at ndsSceneBoundary, so
 *   the manager loop links in step 1 but does not yet run end to end. The
 *   DS-verified path in step 1 is the sc1PGameStartScene bridge alone.
 * - syDmaLoadOverlay, lbBackupWrite, ftParamGetCostumeCommonID: real via
 *   port headers (sys/dma.h, mn/menu.h, ft/fighter.h).
 */

#if NDS_P2_1P_GAME

#include <string.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <mn/menu.h>
#include <mv/movie.h>
#include <sc/scene.h>
#include <sys/dma.h>
#include <nds/nds_p4.h>
#include <nds/nds_p7.h>

/* No port header publishes this; same local extern as every other import TU. */
s32 syUtilsRandIntRange(s32 range);

/* decomp ft/ftdef.h:5 verbatim. Port include/ft/fighter.h lacks it; the
 * included manager TU uses it at sc1pmanager.c:278. */
#ifndef FTCOMMON_HANDICAP_DEFAULT
#define FTCOMMON_HANDICAP_DEFAULT 9
#endif

/* Step-1 pair: DS first-stage boot in battleship_sc1pgame_runtime.c. */
void sc1PGameStartScene(void);

/* P7 U4 (docs/P7/P7_Implementation_Plan.md): the unlock rows' trigger sites
 * are scene calls the manager makes -- a fight, a stage clear, the run's
 * congratulations, a message. Inside the source loop each goes through a DS
 * function that records what the rows read or queues what they unlock, then
 * calls the source scene; the loop itself is the source's. */
void ndsP7ManagerGameScene(void);
void ndsP7ManagerStageClearScene(void);
void ndsP7ManagerCongraScene(void);
void ndsP7ManagerMessageScene(void);
#define sc1PGameStartScene ndsP7ManagerGameScene
#define sc1PStageClearStartScene ndsP7ManagerStageClearScene
#define mnCongraStartScene ndsP7ManagerCongraScene
#define mnMessageStartScene ndsP7ManagerMessageScene

#define sc1PManagerUpdateScene ndsBaseSC1PManagerUpdateScene
void ndsBaseSC1PManagerUpdateScene(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pmanager.c"

#undef sc1PManagerUpdateScene
#undef sc1PGameStartScene
#undef sc1PStageClearStartScene
#undef mnCongraStartScene
#undef mnMessageStartScene

/* ---- P7 U4/U5: challengers and unlocks beyond the source's ---- */

#define NDS_P7_QUEUE_MAX 24u
/* The rows a run (or a bonus practice) met, in table order: its challengers
 * fight first, back to back, then its direct rows are delivered (A5). */
static u8 sNdsP7Queue[NDS_P7_QUEUE_MAX];
static u8 sNdsP7QueueCount;
/* The unlocks whose messages the run owes (A6: a stage clear's show when
 * the run ends, cleared or not). */
static u8 sNdsP7Owed[NDS_P7_QUEUE_MAX];
static u8 sNdsP7OwedCount;
/* A bonus practice routed here to fight its rows (ndsP7BonusPracticeEnded),
 * and the scene the practice was going to. */
static u8 sNdsP7BonusEntry;
static u8 sNdsP7BonusNextScene;
/* What the rows read that the source keeps for no reader: the last fight's
 * falls, and Race to the Finish's seconds left. */
static u8 sNdsP7StageFalls;
static u16 sNdsP7RaceLeft = NDS_P7_NO_RACE;

static u8 ndsP7Clamp8(s32 v)
{
    return (v < 0) ? 0u : (v > 254) ? 254u : (u8)v;
}

static void ndsP7RunFill(NDSP7Run *run, u32 site, s32 sel)
{
    u32 time_s = gSC1PManagerTotalTimeTics / 60u;

    run->site = (u8)site;
    run->sel = (u8)sel;
    run->difficulty = gSCManagerBackupData.spgame_difficulty;
    /* The menu's stock setting, 1-5 (the save keeps it less one). */
    run->stock_setting = (u8)(gSCManagerBackupData.spgame_stock_count + 1);
    run->continues = ndsP7Clamp8(gSCManagerSceneData.continues_used);
    run->falls = ndsP7Clamp8(gSC1PManagerTotalFalls);
    run->stage = gSCManagerSceneData.spgame_stage;
    run->stage_falls = sNdsP7StageFalls;
    run->time_s = (u16)((time_s > 0xFFFEu) ? 0xFFFEu : time_s);
    run->race_left_s = sNdsP7RaceLeft;
}

static void ndsP7Owe(u32 index)
{
    if (sNdsP7OwedCount < NDS_P7_QUEUE_MAX)
    {
        sNdsP7Owed[sNdsP7OwedCount++] = (u8)index;
    }
}

/* A row's unlock, then the companions it completes (a stage that comes with
 * its fighter), each owing its message; one write commits them. */
static void ndsP7Deliver(u32 index)
{
    u8 companions[8];
    u32 n;
    u32 i;

    ndsP7UnlockDeliver(ndsP7UnlockRow(index));
    ndsP7Owe(index);
    n = ndsP7UnlockCompanions(companions, ARRAY_COUNT(companions));
    for (i = 0u; i < n; i++)
    {
        ndsP7UnlockDeliver(ndsP7UnlockRow(companions[i]));
        ndsP7Owe(companions[i]);
    }
    lbBackupWrite();
}

/* The owed messages, one screen each (U6), as the source shows its own
 * (sc1pmanager.c:537-543); the scene the caller was in stands after. */
static void ndsP7ShowOwed(void)
{
    u8 prev = gSCManagerSceneData.scene_prev;
    u8 curr = gSCManagerSceneData.scene_curr;
    u32 count = sNdsP7OwedCount;

    sNdsP7OwedCount = 0u;
    if (count == 0u)
    {
        return;
    }
    syDmaLoadOverlay(&dSC1PManagerSubsysOverlay);
    syDmaLoadOverlay(&dSC1PManagerMessageOverlay);
    gSCManagerSceneData.scene_prev = nSCKind1PGame;
    gSCManagerSceneData.scene_curr = nSCKindMessage;
    ndsP7ShowMessages(sNdsP7Owed, count);
    gSCManagerSceneData.scene_prev = prev;
    gSCManagerSceneData.scene_curr = curr;
}

/* The rows `run` meets, less the companions (they come with their opener's
 * delivery): queued, or delivered at once when `now`. */
static void ndsP7Collect(const NDSP7Run *run, sb32 now)
{
    u8 rows[NDS_P7_QUEUE_MAX];
    u32 n = ndsP7UnlockMatch(run, rows, NDS_P7_QUEUE_MAX);
    u32 i;

    for (i = 0u; i < n; i++)
    {
        const NDSP7UnlockRow *row = ndsP7UnlockRow(rows[i]);

        if ((row == NULL) || (row->site == NDS_P7_SITE_UNLOCK))
        {
            continue;
        }
        if ((now != FALSE) && (row->delivery != NDS_P7_DELIVERY_CHALLENGER))
        {
            ndsP7Deliver(rows[i]);
        }
        else if (sNdsP7QueueCount < NDS_P7_QUEUE_MAX)
        {
            sNdsP7Queue[sNdsP7QueueCount++] = rows[i];
        }
    }
}

/* One challenger: the screen, then the fight on the source's first
 * challenger rows with the row's stage and fighter in them (sc1pmanager.c
 * :506-548 for a source challenger). 1 won, 0 lost, -1 RESET. */
static s32 ndsP7FightChallenger(u32 index)
{
    const NDSP7UnlockRow *row = ndsP7UnlockRow(index);
    s32 fkind = (row->id == NDS_P7_SEL_METAL) ? nFTKindMMario : (s32)row->id;
#if NDS_P4
    const NDSP4Fighter *f = (row->content != 0u) ? ndsP4Fighter(row->content) : NULL;
    u8 saved_content[GMCOMMON_PLAYERS_MAX];
#endif
    u8 saved_stage = gSCManagerSceneData.spgame_stage;
    s32 player = gSCManagerSceneData.player;
    s32 won;

#if NDS_P4
    if (f != NULL)
    {
        fkind = f->parent_kind;
    }
    /* The VS select's contents come back after; meanwhile the challenger is
     * the one content in the match (the renderer's article roots ask). */
    memcpy(saved_content, gNdsP4PlayerContent, sizeof(saved_content));
    memset(gNdsP4PlayerContent, 0, sizeof(saved_content));
    gNdsP4PlayerContent[0] = row->content;
#endif
    gNdsP7Challenger.row = (u8)index;
    gNdsP7Challenger.fkind = (u8)fkind;
    gNdsP7Challenger.content = row->content;
    gNdsP7Challenger.gkind = row->challenger_gkind;
    gNdsP7Challenger.active = 1u;

    gSCManagerSceneData.challenger_fkind = gNdsP7Challenger.fkind;

    syDmaLoadOverlay(&dSC1PManagerSubsysOverlay);
    syDmaLoadOverlay(&dSC1PManager1PChallengerOverlay);

    gSCManagerSceneData.scene_prev = nSCKind1PGame;
    gSCManagerSceneData.scene_curr = nSCKind1PChallenger;

    sc1PChallengerStartScene();

    gSCManager1PGameBattleState.players[player].stock_count = 0;
    gSCManagerSceneData.spgame_stage = nSC1PGameStageChallengerStart;

    syDmaLoadOverlay(&dSC1PManagerObjectsOverlay);
    syDmaLoadOverlay(&dSC1PManager1PGameOverlay);

    sc1PGameStartScene();

    gNdsP7Challenger.active = 0u;
#if NDS_P4
    memcpy(gNdsP4PlayerContent, saved_content, sizeof(saved_content));
#endif
    gSCManagerSceneData.spgame_stage = saved_stage;

    if (gSCManagerSceneData.is_reset != FALSE)
    {
        return -1;
    }
    won = ((gSCManager1PGameBattleState.players[player].stock_count != -1) &&
           (gSCManager1PGameBattleState.time_remain != 0)) ? 1 : 0;
    /* A4: the source's level drop. */
    if (won != 0)
    {
        gSCManagerSceneData.challenger_level_drop = dSCManagerDefaultSceneData.challenger_level_drop;
    }
    else if (gSCManagerSceneData.challenger_level_drop < 9)
    {
        gSCManagerSceneData.challenger_level_drop += 2;
    }
    return won;
}

/* The queue (A5): every challenger whose prerequisites hold now, back to
 * back, each won one delivered; then the direct rows; then the messages.
 * FALSE on a RESET: the rest waits for the next time its condition is met. */
static sb32 ndsP7RunQueue(void)
{
    u32 i;

    for (i = 0u; i < sNdsP7QueueCount; i++)
    {
        const NDSP7UnlockRow *row = ndsP7UnlockRow(sNdsP7Queue[i]);
        s32 result;

        if ((row->delivery != NDS_P7_DELIVERY_CHALLENGER) ||
            (ndsP7UnlockPending(row) == FALSE))
        {
            continue;
        }
        result = ndsP7FightChallenger(sNdsP7Queue[i]);
        if (result < 0)
        {
            sNdsP7QueueCount = 0u;
            return FALSE;
        }
        if (result > 0)
        {
            ndsP7Deliver(sNdsP7Queue[i]);
        }
    }
    for (i = 0u; i < sNdsP7QueueCount; i++)
    {
        const NDSP7UnlockRow *row = ndsP7UnlockRow(sNdsP7Queue[i]);

        if ((row->delivery != NDS_P7_DELIVERY_CHALLENGER) &&
            (ndsP7UnlockPending(row) != FALSE))
        {
            ndsP7Deliver(sNdsP7Queue[i]);
        }
    }
    sNdsP7QueueCount = 0u;
    ndsP7ShowOwed();
    return TRUE;
}

/* A fight: its falls (a stage clear's row reads them) and, for Race to the
 * Finish, the seconds left (0 when time ran out; sc1pgame.c:2867). */
void ndsP7ManagerGameScene(void)
{
    s32 falls = gSC1PManagerTotalFalls;
    u8 stage = gSCManagerSceneData.spgame_stage;

    sc1PGameStartScene();
    sNdsP7StageFalls = ndsP7Clamp8(gSC1PManagerTotalFalls - falls);
    if (stage == nSC1PGameStageBonus3)
    {
        sNdsP7RaceLeft = (u16)gSCManagerSceneData.spgame_time_remain;
    }
}

/* A6: a stage clear's unlocks (Metal Mario, Duel Zone) are earned here;
 * their messages show when the run ends. */
void ndsP7ManagerStageClearScene(void)
{
    NDSP7Run run;

    ndsP7RunFill(&run, NDS_P7_SITE_STAGE_CLEAR, gSCManagerSceneData.fkind);
    ndsP7Collect(&run, TRUE);
    sc1PStageClearStartScene();
}

/* The run is cleared: its rows queue (sc1pmanager.c:475 has saved its
 * records, which the rows read). */
void ndsP7ManagerCongraScene(void)
{
    NDSP7Run run;

    mnCongraStartScene();
    ndsP7RunFill(&run, NDS_P7_SITE_RUN_END, gSCManagerSceneData.fkind);
    ndsP7Collect(&run, FALSE);
}

/* A5: Mushroom Kingdom's message comes last -- the queue before it. */
void ndsP7ManagerMessageScene(void)
{
    if ((gSCManagerSceneData.unlock_messages[0] == nLBBackupUnlockInishie) &&
        ((sNdsP7QueueCount != 0u) || (sNdsP7OwedCount != 0u)))
    {
        u8 saved[ARRAY_COUNT(gSCManagerSceneData.unlock_messages)];

        memcpy(saved, gSCManagerSceneData.unlock_messages, sizeof(saved));
        memset(gSCManagerSceneData.unlock_messages, nLBBackupUnlockEnumCount, sizeof(saved));
        if (ndsP7RunQueue() == FALSE)
        {
            return;
        }
        memcpy(gSCManagerSceneData.unlock_messages, saved, sizeof(saved));
    }
    mnMessageStartScene();
}

/* The player's rows, as the manager's head writes them (sc1pmanager.c
 * :267-311), for a challenger fought outside a run. */
static void ndsP7SetupPlayer(void)
{
    s32 player = gSCManagerSceneData.player;
    s32 i;

    gSCManager1PGameBattleState.is_team_battle = TRUE;
    gSCManager1PGameBattleState.game_rules = (SCBATTLE_GAMERULE_1PGAME | SCBATTLE_GAMERULE_TIME);
    gSCManager1PGameBattleState.damage_ratio = 100;
    gSCManager1PGameBattleState.is_show_score = FALSE;
    gSCManager1PGameBattleState.is_not_teamshadows = TRUE;

    gSCManager1PGameBattleState.players[player].handicap = FTCOMMON_HANDICAP_DEFAULT;
    gSCManager1PGameBattleState.players[player].pkind = nFTPlayerKindMan;
    gSCManager1PGameBattleState.players[player].team = 0;
    gSCManager1PGameBattleState.players[player].shade = 0;
    gSCManager1PGameBattleState.players[player].color = player;
    gSCManager1PGameBattleState.players[player].tag = player;
    gSCManager1PGameBattleState.players[player].fkind = gSCManagerSceneData.fkind;
    gSCManager1PGameBattleState.players[player].costume = gSCManagerSceneData.costume;
    gSCManager1PGameBattleState.players[player].stock_count = gSCManagerBackupData.spgame_stock_count;
    gSCManager1PGameBattleState.players[player].is_spgame_enemy = FALSE;

    for (i = 0; i < ARRAY_COUNT(gSCManagerSceneData.ally_players); i++)
    {
        player = (player == (GMCOMMON_PLAYERS_MAX - 1)) ? 0 : (player + 1);
        gSCManagerSceneData.ally_players[i] = player;
    }
}

/* A bonus practice finished (`entry_prev`: the scene it was started from).
 * Its direct rows are delivered; its challengers fight next, in the 1P
 * manager, which then goes where the practice was going. After Luigi's
 * challenge when the source routes to it (sc1pbonusstage.c:1227). */
void ndsP7BonusPracticeEnded(u32 entry_prev)
{
    NDSP7Run run;
    u32 site;

    if (entry_prev == nSCKind1PBonus1Players)
    {
        site = NDS_P7_SITE_BONUS1_END;
    }
    else if (entry_prev == nSCKind1PBonus2Players)
    {
        site = NDS_P7_SITE_BONUS2_END;
    }
    else
    {
        return;
    }
    /* Paused: the player quit (sc1pbonusstage.c:1187). */
    if ((gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->game_status == nSCBattleGameStatusPause))
    {
        return;
    }
    ndsP7RunFill(&run, site, gSCManagerSceneData.bonus_fkind);
    sNdsP7QueueCount = 0u;
    ndsP7Collect(&run, TRUE);
    if (sNdsP7QueueCount == 0u)
    {
        ndsP7ShowOwed();
        return;
    }
    if (gSCManagerSceneData.scene_curr == nSCKind1PGame)
    {
        return;
    }
    sNdsP7BonusEntry = 1u;
    sNdsP7BonusNextScene = gSCManagerSceneData.scene_curr;
    gSCManagerSceneData.fkind = gSCManagerSceneData.bonus_fkind;
    gSCManagerSceneData.costume = gSCManagerSceneData.bonus_costume;
    gSCManagerSceneData.scene_curr = nSCKind1PGame;
}

/* After the source's loop: the queue it left (no Mushroom Kingdom message
 * took it) and the messages a stage clear owes; the scene the loop chose
 * stands. */
static void ndsP7AfterManager(void)
{
    u8 prev = gSCManagerSceneData.scene_prev;
    u8 curr = gSCManagerSceneData.scene_curr;

    if ((sNdsP7QueueCount == 0u) && (sNdsP7OwedCount == 0u))
    {
        return;
    }
    if (gSCManagerSceneData.is_reset != FALSE)
    {
        sNdsP7QueueCount = 0u;
        ndsP7ShowOwed();
        return;
    }
    if (ndsP7RunQueue() == FALSE)
    {
        gSCManagerSceneData.scene_prev = nSCKind1PGame;
        gSCManagerSceneData.scene_curr = nSCKind1PMode;
        return;
    }
    gSCManagerSceneData.scene_prev = prev;
    gSCManagerSceneData.scene_curr = curr;
}

#if NDS_P2_MENU_WALK
/* LAB (walk builds only, P7 U10): an unlock row poked by the probe; the
 * manager's next entry fights that challenger at once and goes back to the
 * title, unlocking nothing. */
volatile u32 gNdsP7LabChallengerRow = 0xFFu;
volatile u32 gNdsP7LabChallengerResult = 0xFFu;
#endif

void sc1PManagerUpdateScene(void)
{
#if NDS_P2_MENU_WALK
    if (gNdsP7LabChallengerRow < ndsP7UnlockRowCount())
    {
        u32 row = gNdsP7LabChallengerRow;

        gNdsP7LabChallengerRow = 0xFFu;
        ndsP7SetupPlayer();
        gNdsP7LabChallengerResult = (u32)ndsP7FightChallenger(row);
        if (gNdsP7LabChallengerResult == 1u)
        {
            /* Its message (U6), as a win shows it; nothing is unlocked. */
            ndsP7Owe(row);
            ndsP7ShowOwed();
        }
        gSCManagerSceneData.scene_prev = nSCKind1PGame;
        gSCManagerSceneData.scene_curr = nSCKindStartup;
        return;
    }
#endif
#if NDS_P2_MENU_WALK && (NDS_1P_WALK_START_STAGE >= 0)
    /* LAB (Makefile NDS_1P_WALK_START_STAGE): the walk enters the ladder at
     * one stage, for instruments that cannot poke it -- the ARM9 profiler
     * runs without a debugger. */
    static u8 sNdsWalkStartStagePoked;

    if (sNdsWalkStartStagePoked == 0u)
    {
        gSCManagerSceneData.spgame_stage = (u8)NDS_1P_WALK_START_STAGE;
        sNdsWalkStartStagePoked = 1u;
    }
#endif
#if defined(NDS_1P_PLAYTEST_START_STAGE) && (NDS_1P_PLAYTEST_START_STAGE >= 0)
    /* PLAYTEST ROMs (Makefile NDS_1P_PLAYTEST_START_STAGE; owner 2026-10-06:
     * "a way to easily playtest masterhand without going through the whole
     * 1p campaign"): a game entering the ladder at its first stage enters at
     * this one, every time 1P is started. */
    if (gSCManagerSceneData.spgame_stage == 0u)
    {
        gSCManagerSceneData.spgame_stage = (u8)NDS_1P_PLAYTEST_START_STAGE;
    }
#endif
    if (sNdsP7BonusEntry != 0u)
    {
        /* A bonus practice's challengers (ndsP7BonusPracticeEnded). */
        u8 next = sNdsP7BonusNextScene;

        sNdsP7BonusEntry = 0u;
        ndsP7SetupPlayer();
        if (ndsP7RunQueue() == FALSE)
        {
            gSCManagerSceneData.scene_prev = nSCKind1PGame;
            gSCManagerSceneData.scene_curr = nSCKind1PMode;
            return;
        }
        gSCManagerSceneData.scene_prev = nSCKind1PBonusStage;
        gSCManagerSceneData.scene_curr = next;
        return;
    }
    sNdsP7StageFalls = 0u;
    sNdsP7RaceLeft = NDS_P7_NO_RACE;
    ndsBaseSC1PManagerUpdateScene();
    ndsP7AfterManager();
}

#endif /* NDS_P2_1P_GAME */
