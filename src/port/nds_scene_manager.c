/* P2-1b -- the port-owned scene seam. Contract and reasoning: include/nds/nds_scene_manager.h. */

#include <nds/nds_os.h>
#include <nds/nds_scene_manager.h>
#include <sc/scene.h>
#include <sys/malloc.h>
#include <sys/taskman.h>
#include "nds_build_config.h"

/* THE REGISTRY. Only scenes this build actually has; a kind that is not here
 * cannot be requested. The order is the VS loop's order, which is also the
 * order P2-1d/e/f will fill the menus in.
 *
 * `NDS_SCENE_FLAG_ARENA_RESET` is not an aspiration: every one of these
 * reaches `syTaskmanStartTask`, which calls `syTaskmanInitGeneralHeap` on the
 * arena the scene declares (decomp sys/taskman.c:1227/:258). The flag is what
 * the exit hook keys the flat-high-water invariant on. */
static const NdsSceneDesc sNdsSceneTable[] = {
    /* The placeholder menu. Today it is the imported BattleShip VS Mode scene
     * (src/import/battleship_mnvsmode.c), which is why the loop needs no new
     * screen and no UI kit -- P2-1c owns that, P2-1d replaces this scene's
     * body with the real VS menu, and the loop below does not change when it
     * does. */
    { (u8)nSCKindVSMode, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_NONE },
    /* Character select and stage select exist as imported scenes already
     * (battleship_mnplayersvs.c, battleship_mnmaps.c) and are the loop's real
     * middle once P2-1e/f land. They are in the table now because a request to
     * either must be accepted, not refused, the moment those rows start. */
    { (u8)nSCKindPlayersVS, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_NONE },
    { (u8)nSCKindMaps, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_NONE },
    /* The match. Sudden Death is NOT a separate row: it is a second entry into
     * this same kind (scVSBattleStartSuddenDeath is the func_start swapped in
     * by battleship_scvsbattle.c:118), so it is exactly the re-entry the
     * high-water ring has to hold flat. */
    { (u8)nSCKindVSBattle, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_BATTLE,
      NDS_SCENE_TRANSITION_NONE },
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    { (u8)nSCKindVSResults, NDS_SCENE_FLAG_ARENA_RESET,
      NDS_SCENE_TRANSITION_SOURCE },
#endif
    { (u8)nSCKindTitle, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_NONE },
#if NDS_P2_MENU_SHELL
    /* P2-1d. The main menu. It is the one scene in this table with no imported
     * translation unit behind it -- nSCKindModeSelect was an NDS_SCENE_STUB --
     * so its StartScene is the port's own (src/nds/nds_menu_shell.c). It
     * declares the same arena as every other row, which is what keeps the
     * high-water ring comparable across kinds.
     *
     * Gated because it can only be REQUESTED when the shell exists: with the
     * flag off nothing names this kind, and admitting it would advertise a
     * destination whose scene is still the stub that parks forever. */
    { (u8)nSCKindModeSelect, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_NONE },
    /* P2-5u1. The two option screens behind the VS menu's OPTIONS row. Both
     * were missing here, and this table fails CLOSED, so every request for
     * either was refused and the shell stayed on the VS menu -- the owner's
     * Item Switch screen was unreachable for this reason as much as for the
     * refusing row that used to sit in front of it. A scripted lap showed it
     * plainly: gNdsSceneManagerRejectCount 21 with a scene ring of nothing
     * but VSMode.
     *
     * Same flags as the menu rows above: each resets the arena and declares
     * itself a menu, which is what keeps the high-water ring comparable
     * across kinds. Gated with the shell for the same reason ModeSelect is --
     * with the flag off their scenes are still NDS_SCENE_STUBs that park. */
    { (u8)nSCKindVSOptions, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_NONE },
    { (u8)nSCKindVSItemSwitch,
      NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_NONE },
#endif
#if NDS_P2_1P_GAME
    /* P2-7 item 9. The imported SOURCE menu scenes. Same flags as the menu rows
     * above (arena reset + menu, so the high-water ring stays comparable) and
     * NDS_SCENE_TRANSITION_SOURCE like the VS Results row: each builds its own
     * transition. One gate for all: each real StartScene lives in its
     * src/import TU behind this same flag, so with the flag off the kind is
     * still an NDS_SCENE_STUB in title_backend.c that parks, and admitting it
     * here would advertise a destination that never returns. nSCKindScreenAdjust
     * is deliberately NOT listed: its scene is still that stub. */
    /* mnoption.c (battleship_mnoption.c); the ModeSelect OPTION entry. */
    { (u8)nSCKindOption, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    /* mnbackupclear.c (battleship_mnbackupclear.c); the Option screen's target. */
    { (u8)nSCKindBackupClear, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    /* mnsoundtest.c (battleship_mnsoundtest.c); a DATA-menu target. */
    { (u8)nSCKindSoundTest, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    /* mndata.c (battleship_mndata.c); the ModeSelect DATA entry. */
    { (u8)nSCKindData, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    /* mnvsrecord.c (battleship_mnvsrecord.c); a DATA-menu target. */
    { (u8)nSCKindVSRecord, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    /* mncharacters.c (battleship_mncharacters.c); a DATA-menu target. */
    { (u8)nSCKindCharacters, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    /* mn1pmode.c (battleship_mn1pmode.c); the ModeSelect 1P GAME entry. */
    { (u8)nSCKind1PMode, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    /* mn1pcontinue.c (battleship_mn1pcontinue.c); the 1P Continue screen. */
    { (u8)nSCKind1PContinue, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    /* mnmessage.c (battleship_mnmessage.c); the unlock-message scene. */
    { (u8)nSCKindMessage, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    /* P2-7 item 6. The attract pair, reached from the native title's idle
     * timer (nds_menu_shell_core.c). Same arena-reset discipline as every
     * other row so the high-water ring stays comparable, and SOURCE like the
     * rows above: each builds its own transition (scautodemo.c makes its
     * fade in scAutoDemoMakeFade; scexplain.c likewise). AutoDemo carries
     * BATTLE: it is a four-CPU battle, mechanically (scautodemo.c:546-579);
     * Explain carries MENU: it is the scripted How to Play screen
     * (scexplain.c:151-169, GameKey fighters). nSCKindScreenAdjust stays
     * unlisted: still a stub that parks. */
    /* scautodemo.c (battleship_scautodemo.c); the title-idle demo battle. */
    { (u8)nSCKindAutoDemo, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_BATTLE,
      NDS_SCENE_TRANSITION_SOURCE },
    /* scexplain.c (battleship_scexplain.c); the How to Play screen. BATTLE,
     * not MENU (2026-09-05): it stands two GameKey fighters on nGRKindExplain
     * with the battle reservations, runs through the battle runner and takes
     * the DS battle re-budget at syTaskmanStartTask like the demo. */
    { (u8)nSCKindExplain, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_BATTLE,
      NDS_SCENE_TRANSITION_SOURCE },
#if defined(REGION_US)
    /* mncongra.c (battleship_mncongra.c); Congratulations, a US-only scene. */
    { (u8)nSCKindCongra, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
#endif
    /* P2-6 (2026-09-05). The campaign itself. The 1P menus route here from
     * mn1pmode.c: the character selects (mnplayers1pgame.c,
     * mnplayers1ptraining.c, mnplayers1pbonus.c for both bonus rounds), then
     * the manager loop (sc1pmanager.c, dispatched as nSCKind1PGame) drives
     * the ladder through the intro (sc1pintro.c), the fight, the tally
     * (sc1pstageclear.c), the challenger (sc1pchallenger.c), the two bonus
     * boards (sc1pbonusstage.c), the ending movie (mvending.c) and the
     * credits (scstaffroll.c). The MENU rows run through the source-scene
     * pump; the three BATTLE rows -- the ladder fight, the bonus boards and
     * Training -- run through the battle runner, which ticks each scene's
     * own update (taskman_seam_battle_host.c ndsSeamSceneUpdate). */
    { (u8)nSCKind1PGamePlayers, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    { (u8)nSCKindPlayers1PTraining, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    { (u8)nSCKind1PBonus1Players, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    { (u8)nSCKind1PBonus2Players, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    { (u8)nSCKind1PIntro, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    { (u8)nSCKind1PChallenger, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    { (u8)nSCKind1PStageClear, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    { (u8)nSCKindEnding, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    { (u8)nSCKindStaffroll, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_MENU,
      NDS_SCENE_TRANSITION_SOURCE },
    { (u8)nSCKind1PGame, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_BATTLE,
      NDS_SCENE_TRANSITION_NONE },
    { (u8)nSCKind1PBonusStage, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_BATTLE,
      NDS_SCENE_TRANSITION_NONE },
    { (u8)nSCKind1PTrainingMode, NDS_SCENE_FLAG_ARENA_RESET | NDS_SCENE_FLAG_BATTLE,
      NDS_SCENE_TRANSITION_NONE },
#endif
};

#define NDS_SCENE_TABLE_COUNT \
    (u32)(sizeof(sNdsSceneTable) / sizeof(sNdsSceneTable[0]))

/* --gc-sections drops a global whose only readers are gdb and a verifier
 * script; that has reddened Boundary once already on "Missing ELF symbol". */
#define NDS_SCENE_PUBLISHED __attribute__((used))

NDS_SCENE_PUBLISHED volatile u32 gNdsSceneManagerEnterCount;
NDS_SCENE_PUBLISHED volatile u32 gNdsSceneManagerExitCount;
NDS_SCENE_PUBLISHED volatile u32 gNdsSceneManagerRequestCount;
NDS_SCENE_PUBLISHED volatile u32 gNdsSceneManagerRejectCount;
NDS_SCENE_PUBLISHED volatile u32 gNdsSceneManagerCurrKind = 0xffffffffu;
volatile u32 gNdsSceneManagerCurrIsBattle = 0u;
NDS_SCENE_PUBLISHED volatile u32 gNdsSceneManagerPrevKind = 0xffffffffu;
NDS_SCENE_PUBLISHED volatile u32 gNdsSceneManagerArenaBase;
NDS_SCENE_PUBLISHED volatile u32 gNdsSceneManagerArenaSize;
NDS_SCENE_PUBLISHED volatile u32 gNdsSceneManagerArenaMismatchCount;
NDS_SCENE_PUBLISHED volatile u8 gNdsSceneManagerRingKind[NDS_SCENE_MANAGER_RING];
NDS_SCENE_PUBLISHED volatile u32
    gNdsSceneManagerRingArenaHigh[NDS_SCENE_MANAGER_RING];
NDS_SCENE_PUBLISHED volatile u32
    gNdsSceneManagerRingArenaFree[NDS_SCENE_MANAGER_RING];
NDS_SCENE_PUBLISHED volatile u32
    gNdsSceneManagerRingTransition[NDS_SCENE_MANAGER_RING];
NDS_SCENE_PUBLISHED volatile u32 gNdsSceneManagerUnregisteredEnterCount;
NDS_SCENE_PUBLISHED volatile u32 gNdsSceneManagerCurrTransition;

NDS_SCENE_PUBLISHED volatile u32 gNdsSceneWalkHopsRemaining;
NDS_SCENE_PUBLISHED volatile u32 gNdsSceneWalkLoopsCompleted;

/* Index of the entry currently running, so Exit writes the slot Enter claimed
 * even if a nested start ever appears. */
static u32 sNdsSceneManagerRingIndex;
static u32 sNdsSceneManagerDepth;
#if NDS_R2_SCENE_LOOP_WALK
static u32 sNdsSceneWalkArmed;
#endif

const NdsSceneDesc *ndsSceneManagerFind(u32 kind)
{
    u32 i;

    for (i = 0u; i < NDS_SCENE_TABLE_COUNT; i++)
    {
        if ((u32)sNdsSceneTable[i].kind == kind)
        {
            return &sNdsSceneTable[i];
        }
    }
    return NULL;
}

void ndsSceneManagerRequest(u32 next_kind, u32 prev_kind)
{
    const NdsSceneDesc *desc = ndsSceneManagerFind(next_kind);
    u32 slot;

    if (desc == NULL)
    {
        /* Fail closed. Leaving the current scene running is recoverable and
         * loud (the counter is non-zero); writing an unknown kind into
         * scene_curr hands scManagerRunLoop a switch case it does not have,
         * which falls through its while(TRUE) and hangs with no evidence. */
        gNdsSceneManagerRejectCount++;
        return;
    }

    slot = gNdsSceneManagerRequestCount % NDS_SCENE_MANAGER_RING;
    gNdsSceneManagerRingTransition[slot] =
        ((prev_kind & 0xffu) << 8) | (next_kind & 0xffu);
    gNdsSceneManagerRequestCount++;

    gSCManagerSceneData.scene_prev = (u8)prev_kind;
    gSCManagerSceneData.scene_curr = (u8)next_kind;
    syTaskmanSetLoadScene();
}

void ndsSceneManagerEnter(const void *arena_start, u32 arena_size)
{
    u32 kind = (u32)gSCManagerSceneData.scene_curr;
    const NdsSceneDesc *desc = ndsSceneManagerFind(kind);

    sNdsSceneManagerDepth++;

    if (desc == NULL)
    {
        /* `nSCKindStartup` lands here on every boot and is expected: BattleShip's
         * startup scene declares the N64 overlay-derived arena
         * (mncommon/mnstartup.c:272, `ovl1_VRAM - ovl58_BSS_END`), not
         * `ndsTaskmanArenaStart()`, so it is deliberately outside the registry
         * and outside the arena invariant below. Any OTHER unregistered kind
         * means the table is incomplete and a flat ring covers less than it
         * claims -- which is why this is one counter and not a silent skip. */
        gNdsSceneManagerUnregisteredEnterCount++;
        gNdsSceneManagerCurrTransition = NDS_SCENE_TRANSITION_NONE;
    }
    else
    {
        gNdsSceneManagerCurrTransition = (u32)desc->transition;

        if ((desc->flags & NDS_SCENE_FLAG_ARENA_RESET) != 0u)
        {
            /* THE REWIND IS ONE CALL AWAY -- ndsBaseSyTaskmanStartTask ->
             * syTaskmanInitGeneralHeap (decomp sys/taskman.c:1227 -> :258) --
             * so every GObj thread the OUTGOING scene left registered is about
             * to become a pointer into the incoming scene's memory. The port's
             * thread registry is the only structure that outlives the rewind
             * still holding those pointers, so it is dropped HERE, at the
             * rewind that invalidates them, and not by a validity test in each
             * reader.
             *
             * P2-1b-1 measured: the VS Mode scene left two GObj threads
             * registered (sThreads[5]/[6] = 0x022c6868/0x022c7f20, both inside
             * this arena), the next VSBattle entry rewound over them, and
             * ndsOsRunThreads then read 0xFFFFFFFF out of the reused memory as
             * their coroutine and took a data abort in portCoroutineIsFinished
             * -- reported as a SIGILL in a function --gc-sections had already
             * deleted. Reads 0 on the first entry of a run. */
            (void)ndsOsForgetThreadsInArena(arena_start, arena_size);

            if (gNdsSceneManagerArenaBase == 0u)
            {
                gNdsSceneManagerArenaBase = (u32)(uintptr_t)arena_start;
                gNdsSceneManagerArenaSize = arena_size;
            }
            else if ((gNdsSceneManagerArenaBase !=
                      (u32)(uintptr_t)arena_start) ||
                     (gNdsSceneManagerArenaSize != arena_size))
            {
                /* Two loop scenes rewinding DIFFERENT arenas is not a reset
                 * discipline, and their high-waters are not comparable.
                 * Counted rather than asserted: a legitimate future scene may
                 * want its own arena, and this is what would tell the reader
                 * that a flat ring stopped meaning what it says. */
                gNdsSceneManagerArenaMismatchCount++;
            }
        }
    }

    sNdsSceneManagerRingIndex =
        gNdsSceneManagerEnterCount % NDS_SCENE_MANAGER_RING;
    gNdsSceneManagerRingKind[sNdsSceneManagerRingIndex] = (u8)kind;
    gNdsSceneManagerRingArenaHigh[sNdsSceneManagerRingIndex] = 0u;
    gNdsSceneManagerRingArenaFree[sNdsSceneManagerRingIndex] = 0u;

    gNdsSceneManagerPrevKind = (u32)gSCManagerSceneData.scene_prev;
    gNdsSceneManagerCurrKind = kind;
    gNdsSceneManagerCurrIsBattle =
        ((desc != NULL) && ((desc->flags & NDS_SCENE_FLAG_BATTLE) != 0u)) ?
        1u : 0u;
    gNdsSceneManagerEnterCount++;

#if NDS_R2_SCENE_LOOP_WALK
    if ((sNdsSceneWalkArmed == 0u) && (desc != NULL))
    {
        /* Armed on the first REGISTERED entry, not on the first entry: the
         * boot's startup scene is not part of the loop and arming on it would
         * spend the count before the walk begins.
         *
         * TWO hops per loop, not three. The loop has three legs, but the
         * battle -> results leg is the source's own (scvsbattle.c:560) and
         * must stay that way, so only results -> menu and menu -> battle are
         * requested here. */
        sNdsSceneWalkArmed = 1u;
        gNdsSceneWalkHopsRemaining = (u32)NDS_R2_SCENE_LOOP_WALK * 2u;
    }
#endif
}

void ndsSceneManagerExit(void)
{
    u32 high = (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                     (uintptr_t)gSYTaskmanGeneralHeap.start);
    u32 freed = (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
                      (uintptr_t)gSYTaskmanGeneralHeap.ptr);

    gNdsSceneManagerRingArenaHigh[sNdsSceneManagerRingIndex] = high;
    gNdsSceneManagerRingArenaFree[sNdsSceneManagerRingIndex] = freed;
    gNdsSceneManagerExitCount++;
    if (sNdsSceneManagerDepth != 0u)
    {
        sNdsSceneManagerDepth--;
    }
}

sb32 ndsSceneWalkAdvance(u32 next_kind)
{
#if NDS_R2_SCENE_LOOP_WALK
    u32 before;

    if (gNdsSceneWalkHopsRemaining == 0u)
    {
        return FALSE;
    }
    before = gNdsSceneManagerRequestCount;
    ndsSceneManagerRequest(next_kind, (u32)gSCManagerSceneData.scene_curr);
    if (gNdsSceneManagerRequestCount == before)
    {
        /* Refused. Do not spend the hop -- the caller parks, and the reject
         * counter says why, instead of the walk silently running short. */
        return FALSE;
    }
    gNdsSceneWalkHopsRemaining--;
    /* Leaving RESULTS for a menu is what closes a loop, and P2-1f keys it on
     * that rather than on the destination kind. It used to test
     * `next_kind == nSCKindVSMode`, which was the same predicate while the
     * Results leg was the only hop that could name VS Mode; with the shell on,
     * Results returns to the CHARACTER SELECT (the source's own destination,
     * mnvsresults.c:3312) and so does the VS menu's own START hop, so a
     * destination test would either miss every loop or count the first one
     * twice. `scene_prev` is the scene just left -- ndsSceneManagerRequest
     * above has already written it -- so this reads "a lap ended" directly. */
    if ((u32)gSCManagerSceneData.scene_prev == (u32)nSCKindVSResults)
    {
        gNdsSceneWalkLoopsCompleted++;
    }
    return TRUE;
#else
    (void)next_kind;
    return FALSE;
#endif
}
