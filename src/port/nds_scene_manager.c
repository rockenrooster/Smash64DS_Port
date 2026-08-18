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
    if (next_kind == (u32)nSCKindVSMode)
    {
        /* Arriving back at the menu is what closes a loop. */
        gNdsSceneWalkLoopsCompleted++;
    }
    return TRUE;
#else
    (void)next_kind;
    return FALSE;
#endif
}
