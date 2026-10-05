#include <stdint.h>
#include <calico/nds/arm9/ovl.h>
#include <nds/nds_frontend_overlay.h>
#include <nds/nds_reloc_assets.h>
#include <sc/scene.h>
#include <sys/taskman.h>
#include <sys/malloc.h>
#include <nds/nds_startup.h>

extern u8 __nds_frontend_start[];
extern u8 __nds_frontend_end[];
/* End of the 1P battle-time head (linker/nds_frontend_overlay.ld). */
extern u8 __nds_frontend_1p_end[];

static u8 *sNdsFrontendCursor;
static u32 sNdsFrontendInitialized;
static u32 sNdsFrontendLoaded;
/* A 1P, bonus or training battle borrowed the tail: its code and the scene
 * status buffers are overwritten until the overlay is reloaded. */
static u32 sNdsFrontendTailLoaned;
/* ...and an allocation actually landed in it (nothing to reload otherwise). */
static u32 sNdsFrontendTailDirty;

volatile u32 gNdsFrontendOverlayTailLoanCount;
volatile u32 gNdsFrontendOverlayTailReloadCount;

volatile u32 gNdsFrontendOverlayLoadCount;
volatile u32 gNdsFrontendOverlayLoadFailCount;
volatile u32 gNdsFrontendOverlayBytes;
volatile u32 gNdsFrontendOverlayLoanCount;
volatile u32 gNdsFrontendOverlayLoanBytes;
volatile u32 gNdsFrontendOverlayUsedBytes;
volatile u32 gNdsFrontendOverlayAllocCount;
volatile u32 gNdsFrontendOverlaySpillBytes;

static void ndsFrontendOverlayLoadHalt(void)
{
    gNdsFrontendOverlayLoadFailCount++;
    for (;;) {}
}

/* Load (or reload over a borrowed tail) and activate overlay 0. A reload while
 * head code is on the stack rewrites those bytes with themselves. */
static void ndsFrontendOverlayLoadNow(void)
{
    sb32 ok;

    ndsFsLock();
    ok = (sNdsFrontendInitialized != 0u) || ovlInit();
    if (ok != FALSE)
    {
        sNdsFrontendInitialized = 1u;
        ok = ovlLoadAndActivate(0u);
    }
    ndsFsUnlock();
    if (ok == FALSE)
    {
        ndsFrontendOverlayLoadHalt();
    }
    sNdsFrontendLoaded = 1u;
    sNdsFrontendTailLoaned = 0u;
    sNdsFrontendTailDirty = 0u;
    gNdsFrontendOverlayLoadCount++;
}

void ndsFrontendOverlayPrepareDispatch(u32 kind)
{
    u32 bytes = (u32)(__nds_frontend_end - __nds_frontend_start);

    gNdsFrontendOverlayBytes = bytes;
    /* The previous scene has returned to resident code. Drop its borrowed
     * LBFileNode buffers before their overlay bytes can become asset storage.
     * The next scene installs its own workspace through lbRelocInitSetup. */
    ndsRelocReleaseSceneStatusBuffers();
    sNdsFrontendCursor = NULL;
    if (bytes == 0u)
    {
        return;
    }
    if (kind == (u32)nSCKindVSBattle)
    {
        /* The caller is the resident dispatcher, after the previous scene has
         * returned. No front-end return address may remain on this stack.
         * Persistent menu state stays resident; only scene-local relocation
         * workspaces accompany the text/rodata in the loaned range. */
        if (sNdsFrontendLoaded != 0u)
        {
            ovlDeactivate(0u);
            sNdsFrontendLoaded = 0u;
        }
        return;
    }
    if ((sNdsFrontendLoaded == 0u) || (sNdsFrontendTailDirty != 0u))
    {
        if (sNdsFrontendTailDirty != 0u)
        {
            NDS_DIAG(gNdsFrontendOverlayTailReloadCount++);
        }
        ndsFrontendOverlayLoadNow();
    }
    sNdsFrontendTailLoaned = 0u;
}

/* The campaign's nested battles return into head code (sc1PGameStartScene,
 * sc1PBonusStageStartScene); the tail must be back before the manager calls
 * the next intro, tally, continue or challenger scene. */
void ndsFrontendOverlayRestoreTail(void)
{
    sNdsFrontendCursor = NULL;
    if (sNdsFrontendTailDirty != 0u)
    {
        NDS_DIAG(gNdsFrontendOverlayTailReloadCount++);
        ndsFrontendOverlayLoadNow();
    }
    sNdsFrontendTailLoaned = 0u;
}

void ndsFrontendOverlayBeginScene(u32 kind)
{
    sNdsFrontendCursor = NULL;
    gNdsFrontendOverlayLoanBytes = 0u;
    gNdsFrontendOverlayUsedBytes = 0u;
    gNdsFrontendOverlayAllocCount = 0u;
    gNdsFrontendOverlaySpillBytes = 0u;
    if ((kind == (u32)nSCKindVSBattle) && (sNdsFrontendLoaded == 0u))
    {
        /* Reset at every scene instance, including Sudden Death. This seam is
         * also reached by direct battle lab boots which never load a menu. */
        sNdsFrontendCursor = __nds_frontend_start;
        gNdsFrontendOverlayLoanBytes =
            (u32)(__nds_frontend_end - __nds_frontend_start);
        gNdsFrontendOverlayLoanCount++;
    }
    else if (((kind == (u32)nSCKind1PGame) ||
              (kind == (u32)nSCKind1PBonusStage) ||
              (kind == (u32)nSCKind1PTrainingMode)) &&
             (sNdsFrontendLoaded != 0u))
    {
        /* A campaign, bonus or training battle runs head code only (the
         * linker script's 1P battle-time head); the menu and scene code after
         * it borrows out as this battle's asset storage, as VS borrows the
         * whole range. The previous scene's status buffers live in the tail:
         * drop the relocator's reference before the bytes are reused (this
         * scene installs its own in its FuncStart, after this seam). */
        ndsRelocReleaseSceneStatusBuffers();
        sNdsFrontendCursor = __nds_frontend_1p_end;
        gNdsFrontendOverlayLoanBytes =
            (u32)(__nds_frontend_end - __nds_frontend_1p_end);
        gNdsFrontendOverlayLoanCount++;
        NDS_DIAG(gNdsFrontendOverlayTailLoanCount++);
        sNdsFrontendTailLoaned = 1u;
    }
}

void ndsFrontendOverlayEndScene(void)
{
    sNdsFrontendCursor = NULL;
}

void *ndsFrontendOverlayTryAlloc(size_t bytes, u32 alignment)
{
    uintptr_t start;
    uintptr_t end = (uintptr_t)__nds_frontend_end;

    if ((sNdsFrontendCursor == NULL) || (bytes == 0u) ||
        (alignment == 0u) || ((alignment & (alignment - 1u)) != 0u))
    {
        return NULL;
    }
    start = (uintptr_t)sNdsFrontendCursor;
    if (start > UINTPTR_MAX - (alignment - 1u))
    {
        return NULL;
    }
    start = (start + alignment - 1u) & ~(uintptr_t)(alignment - 1u);
    if ((start > end) || (bytes > end - start))
    {
        return NULL;
    }
    sNdsFrontendCursor = (u8 *)(start + bytes);
    if (sNdsFrontendTailLoaned != 0u)
    {
        sNdsFrontendTailDirty = 1u;
    }
    gNdsFrontendOverlayUsedBytes =
        (u32)(sNdsFrontendCursor - __nds_frontend_start);
    gNdsFrontendOverlayAllocCount++;
    return (void *)start;
}

void *ndsSceneAssetAlloc(size_t bytes, u32 alignment)
{
    void *storage = ndsFrontendOverlayTryAlloc(bytes, alignment);

    if (storage == NULL)
    {
        if (sNdsFrontendCursor != NULL)
        {
            gNdsFrontendOverlaySpillBytes += (u32)bytes;
        }
        storage = syTaskmanMalloc(bytes, alignment);
    }
    return storage;
}

void *ndsSceneAssetTryAlloc(size_t bytes, u32 alignment, size_t keep_free)
{
    void *storage;

    if ((bytes == 0u) || (alignment == 0u) ||
        ((alignment & (alignment - 1u)) != 0u) ||
        (bytes > SIZE_MAX - keep_free))
    {
        return NULL;
    }
    if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap, keep_free, 1u) == FALSE)
    {
        return NULL;
    }
    storage = ndsFrontendOverlayTryAlloc(bytes, alignment);
    if (storage != NULL)
    {
        return storage;
    }
    if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                           bytes + keep_free, alignment) == FALSE)
    {
        return NULL;
    }
    if (sNdsFrontendCursor != NULL)
    {
        gNdsFrontendOverlaySpillBytes += (u32)bytes;
    }
    return syTaskmanMalloc(bytes, alignment);
}
