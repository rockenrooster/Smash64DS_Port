/* Compile the original BattleShip task-manager translation unit.
 *
 * Provides real syTaskmanStartTask, syTaskmanLoadScene, syTaskmanInitGeneralHeap,
 * syTaskmanMalloc, the general/graphics heap globals, and the per-context
 * DL/graphics/RDP setup.
 *
 * The original syTaskmanRunTask is the full per-frame task/object loop, which
 * needs the threading scheduler, display-list pipeline, and RSP/RDP backend
 * that are not yet imported. The original source is compiled with an
 * SSB64_TARGET_NDS guard that omits only that loop definition; the strong DS
 * bounded seam in src/port/scene_backend.c receives syTaskmanLoadScene's call,
 * runs startup updates, mirrors the original cleanup tail, and returns to the
 * original scene manager before task_draw. */

/* P2-1b. `syTaskmanStartTask` IS the scene boundary on this target: decomp
 * taskman.c:1227 calls syTaskmanInitGeneralHeap on the arena the scene
 * declared -- the per-scene arena rewind -- then carves the object pools out
 * of it, then does not return until syTaskmanRunTask has finished the scene.
 * Wrapping it therefore brackets exactly one scene lifetime, which is what the
 * scene manager needs to record an entry's arena high-water and prove N loops
 * leak nothing. No intra-TU caller binds around this: taskman.c calls
 * syTaskmanLoadScene directly and never syTaskmanStartTask. */
#include <nds/nds_scene_manager.h>
#include <nds/nds_platform.h>
#include <sc/scene.h> /* gSCManagerSceneData, for the registered-kind arena rule below */

#define syTaskmanStartTask ndsBaseSyTaskmanStartTask
#if NDS_R2_SECOND_ENTRY_DIAG
#define syTaskmanMalloc ndsBaseSyTaskmanMalloc
#endif
#include <battleship_overlay/src/sys/taskman.c>
#undef syTaskmanStartTask

void syTaskmanStartTask(SYTaskmanSetup *tsetup);

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);
/* battleship_scvsbattle.c: the DS display-list / graphics-heap / RDP sizes
 * for a battle scene, applied below to every registered BATTLE kind. */
extern void ndsBattleRebudgetSceneSetup(SYTaskmanSetup *setup);
extern s32 ndsBattleSetupIsRebudgeted(const SYTaskmanSetup *setup);

void syTaskmanStartTask(SYTaskmanSetup *tsetup)
{
    SYTaskmanSetup ds_setup;
    const NdsSceneDesc *desc =
        ndsSceneManagerFind((u32)gSCManagerSceneData.scene_curr);

    /* Native menus hide BG0; source bonus/training entries reach this seam
     * without the VS wrapper that reclaims it. Every battle owns 3D again.
     * The platform defers the enable until this scene submits its first frame. */
    if ((desc != NULL) && ((desc->flags & NDS_SCENE_FLAG_BATTLE) != 0u))
    {
        ndsPlatformSet3DLayerEnabled(TRUE);
    }

    /* A battle-flagged kind also carries the N64 battle reservations -- 7680
     * and 2560 Gfx of display list, a 0xD000 graphics heap and a 0xC000 RDP
     * output buffer (sc1pgame.c:595-621, sc1pbonusstage.c, the Training and
     * demo setups) -- which the DS arena cannot afford beside the fighters;
     * the VS match rebudgets them in its own wrapper, and every other battle
     * kind gets the same four DS sizes here before the task carves them. */
    if ((desc != NULL) && ((desc->flags & NDS_SCENE_FLAG_BATTLE) != 0u) &&
        (ndsBattleSetupIsRebudgeted(tsetup) == FALSE))
    {
        ds_setup = *tsetup;
        ndsBattleRebudgetSceneSetup(&ds_setup);
        tsetup = &ds_setup;
    }

    /* THE DS ARENA IS THE ONLY ARENA (P2-6/P2-7, 2026-09-05). Every source
     * scene declares its task arena from the N64 overlay layout --
     * `&ovlN_BSS_END` as the start and `&gSYFramebufferSets - &ovlN_BSS_END`
     * as the size (sc1pbonusstage.c:274, :1172 and every sibling) -- and on
     * this target those are 4-byte placeholder objects, so the pair is
     * garbage. The eight hand-written scene wrappers (battleship_scvsbattle.c,
     * battleship_mnvsresults.c, the title, VS mode, players, maps and the two
     * opening movies) each rebuild their setup on ndsTaskmanArenaStart/Size;
     * the twenty-odd source scenes imported whole in P2-6 and P2-7 do not,
     * and rather than teach each TU the same three lines the rule lives at
     * the one seam every scene start crosses. A REGISTERED kind (the scene
     * manager's table, which is exactly the set of scenes that share the DS
     * arena and its reset discipline) gets the DS arena here; the unregistered
     * startup scene keeps its own declaration, as nds_scene_manager.c:220
     * records on purpose. A wrapper that already installed the DS arena is a
     * no-op under this test. */
    if ((ndsSceneManagerFind((u32)gSCManagerSceneData.scene_curr) != NULL) &&
        (tsetup->scene_setup.arena_start != ndsTaskmanArenaStart()))
    {
        ds_setup = *tsetup;
        ds_setup.scene_setup.arena_start = ndsTaskmanArenaStart();
        ds_setup.scene_setup.arena_size = ndsTaskmanArenaSize();
        tsetup = &ds_setup;
    }
    ndsSceneManagerEnter(tsetup->scene_setup.arena_start,
                         (u32)tsetup->scene_setup.arena_size);
    ndsBaseSyTaskmanStartTask(tsetup);
    if ((desc != NULL) && ((desc->flags & NDS_SCENE_FLAG_ARENA_RESET) != 0u))
    {
        ndsRelocRecordSceneMemory(&tsetup->scene_setup);
    }
    ndsSceneManagerExit();
}

#if NDS_R2_SECOND_ENTRY_DIAG
#undef syTaskmanMalloc

/* Allocation ledger keyed by caller LR. Default OFF, shares
 * NDS_R2_SECOND_ENTRY_DIAG with the MObj chain validator.
 *
 * WHAT IT IS FOR. Sudden Death's setup takes about 119 KB more taskman arena
 * than the first battle setup does, for the same stage and the same two
 * fighters, and nothing explains why. Guessing at that from call sites is how
 * the last three hypotheses in this area died; this attributes it by
 * measurement. Read `gNdsAllocLedger` with one `print` at each phase boundary
 * and diff the snapshots -- the per-caller delta between "first battle setup"
 * and "Sudden Death setup" IS the answer.
 *
 * WHAT IT CANNOT SEE, stated so nobody reads a false zero. taskman.c calls
 * syTaskmanMalloc from inside its own translation unit -- syTaskmanStartTask
 * for the GObj thread block, and the graphics-heap reserve -- and those direct
 * intra-TU calls bind to the base symbol and bypass this wrapper. That is
 * acceptable here precisely because the question is a DELTA: those calls are
 * the fixed taskman setup and are identical on both entries, so they cancel.
 * It would not be acceptable for an absolute total, and this must never be
 * reported as one. */
#define NDS_ALLOC_LEDGER_ENTRIES 96u

typedef struct NDSAllocLedgerEntry {
    u32 lr;
    u32 count;
    u32 bytes;
} NDSAllocLedgerEntry;

volatile NDSAllocLedgerEntry gNdsAllocLedger[NDS_ALLOC_LEDGER_ENTRIES];
volatile u32 gNdsAllocLedgerUsed;
/* The ledger is an array of structs, and the probe harness reads flat u32
 * globals -- so the table existed but could not actually be read out without
 * hand-driven gdb. Publish the eight largest callers flat, biggest first, so
 * "where did the arena go" is one -ExtraGlobals list. Filled on demand by
 * ndsAllocLedgerPublishTop, which the arena halt calls. */
#define NDS_ALLOC_LEDGER_TOP 8u
volatile u32 gNdsAllocLedgerTopLR[NDS_ALLOC_LEDGER_TOP];
volatile u32 gNdsAllocLedgerTopBytes[NDS_ALLOC_LEDGER_TOP];
volatile u32 gNdsAllocLedgerTopCount[NDS_ALLOC_LEDGER_TOP];

void ndsAllocLedgerPublishTop(void)
{
    u32 rank;

    for (rank = 0u; rank < NDS_ALLOC_LEDGER_TOP; rank++)
    {
        u32 best_bytes = 0u;
        u32 best = NDS_ALLOC_LEDGER_ENTRIES;
        u32 i;

        for (i = 0u; i < gNdsAllocLedgerUsed; i++)
        {
            u32 bytes = gNdsAllocLedger[i].bytes;
            u32 seen = 0u;
            u32 r;

            for (r = 0u; r < rank; r++)
            {
                if (gNdsAllocLedgerTopLR[r] == gNdsAllocLedger[i].lr)
                {
                    seen = 1u;
                }
            }
            if ((seen == 0u) && (bytes > best_bytes))
            {
                best_bytes = bytes;
                best = i;
            }
        }
        if (best == NDS_ALLOC_LEDGER_ENTRIES)
        {
            break;
        }
        gNdsAllocLedgerTopLR[rank] = gNdsAllocLedger[best].lr;
        gNdsAllocLedgerTopBytes[rank] = gNdsAllocLedger[best].bytes;
        gNdsAllocLedgerTopCount[rank] = gNdsAllocLedger[best].count;
    }
}
/* Distinct callers that did not fit. Non-zero means the table is truncated and
 * a delta computed from it is a lower bound. */
volatile u32 gNdsAllocLedgerOverflow;
volatile u32 gNdsAllocLedgerTotalBytes;

void *syTaskmanMalloc(size_t size, u32 align)
{
    u32 lr = (u32)(uintptr_t)__builtin_return_address(0);
    u32 i;

    for (i = 0u; i < gNdsAllocLedgerUsed; i++)
    {
        if (gNdsAllocLedger[i].lr == lr)
        {
            break;
        }
    }
    if (i == gNdsAllocLedgerUsed)
    {
        if (gNdsAllocLedgerUsed < NDS_ALLOC_LEDGER_ENTRIES)
        {
            gNdsAllocLedger[i].lr = lr;
            gNdsAllocLedger[i].count = 0u;
            gNdsAllocLedger[i].bytes = 0u;
            gNdsAllocLedgerUsed++;
        }
        else
        {
            gNdsAllocLedgerOverflow++;
            return ndsBaseSyTaskmanMalloc(size, align);
        }
    }
    gNdsAllocLedger[i].count++;
    gNdsAllocLedger[i].bytes += (u32)size;
    gNdsAllocLedgerTotalBytes += (u32)size;
    return ndsBaseSyTaskmanMalloc(size, align);
}
#endif
