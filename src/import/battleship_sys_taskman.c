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

#define syTaskmanStartTask ndsBaseSyTaskmanStartTask
#if NDS_R2_SECOND_ENTRY_DIAG
#define syTaskmanMalloc ndsBaseSyTaskmanMalloc
#endif
#include <battleship_overlay/src/sys/taskman.c>
#undef syTaskmanStartTask

void syTaskmanStartTask(SYTaskmanSetup *tsetup);

void syTaskmanStartTask(SYTaskmanSetup *tsetup)
{
    ndsSceneManagerEnter(tsetup->scene_setup.arena_start,
                         (u32)tsetup->scene_setup.arena_size);
    ndsBaseSyTaskmanStartTask(tsetup);
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
