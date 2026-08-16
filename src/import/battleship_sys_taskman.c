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

#if NDS_R2_SECOND_ENTRY_DIAG
#define syTaskmanMalloc ndsBaseSyTaskmanMalloc
#endif
#include <battleship_overlay/src/sys/taskman.c>
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
