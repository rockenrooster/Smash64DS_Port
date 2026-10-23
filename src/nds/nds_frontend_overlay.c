/* The front-end overlay and its loans: the menus' code and data
 * (linker/nds_frontend_overlay.ld) live in a region a battle borrows -- all
 * of it in a VS battle, all but the 1P head in a 1P or bonus battle, all but
 * the training head in training -- and are loaded again when a menu needs
 * them. BeginScene and EndScene bracket each scene;
 * ndsFrontendOverlayLoanContains says whether an address is lent. */

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
/* battleship_sys_objman.c: a battle's lent dead bytes; nds_stage_gx.exec.inc
 * (a build with the stage programs): the loaded program's bytes, given back
 * as one of them. */
extern void *ndsBattleDeadSpanAlloc(size_t size, u32 alignment);
extern s32 ndsStageGxYield(size_t size) __attribute__((weak));
/* End of the 1P battle-time head (linker/nds_frontend_overlay.ld). */
extern u8 __nds_frontend_1p_end[];
/* End of training's own head, the first part of the 1P head (the same
 * script): a training battle keeps only that. */
extern u8 __nds_frontend_training_end[];
/* The 1P head's per-stage parts, in order after its core (the same script):
 * Giant Donkey Kong's motion table, Master Hand's code and table, the
 * Fighting Polygon Team's motion tables, the bonus boards' and Race to the
 * Finish's code. */
extern u8 __nds_frontend_1p_gdonkey[];
extern u8 __nds_frontend_1p_boss[];
extern u8 __nds_frontend_1p_polygon[];
extern u8 __nds_frontend_1p_bonus[];
/* Start of the VS character select's block (the same script): the select lends
 * everything before it. */
extern u8 __nds_frontend_select_start[];
/* The VS Results block sits between the middle and the select's block (the
 * same script): Results lends everything before it and the select's block. */
extern u8 __nds_frontend_results_start[];
extern u8 __nds_frontend_select_end[];

static u8 *sNdsFrontendCursor;
/* Where the current loan ends: the overlay's end, or the Results block. */
static u8 *sNdsFrontendLoanEnd;
/* A second range, taken once the first is full: Results' (the select's
 * block), or the 1P head's idle parts before the part a stage keeps. */
static u8 *sNdsFrontendStart2;
static u8 *sNdsFrontendCursor2;
static u8 *sNdsFrontendLoanEnd2;
static u32 sNdsFrontendInitialized;
static u32 sNdsFrontendLoaded;
/* A 1P, bonus or training battle borrowed the tail: its code and the scene
 * status buffers are overwritten until the overlay is reloaded. */
static u32 sNdsFrontendTailLoaned;
/* ...and an allocation actually landed in it (nothing to reload otherwise). */
static u32 sNdsFrontendTailDirty;
/* The VS character select lent the range before its block to its previews
 * (ndsFrontendOverlayBorrowForSelect). */
static u32 sNdsFrontendSelectDirty;

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
    sNdsFrontendSelectDirty = 0u;
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
    if ((sNdsFrontendLoaded == 0u) || (sNdsFrontendTailDirty != 0u) ||
        (sNdsFrontendSelectDirty != 0u))
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

s32 ndsFrontendOverlayTailResident(void)
{
    return ((sNdsFrontendLoaded != 0u) && (sNdsFrontendTailLoaned == 0u) &&
            (sNdsFrontendTailDirty == 0u)) ? TRUE : FALSE;
}

void *ndsFrontendOverlayBorrowForSelect(u32 *bytes)
{
    u32 lent = (u32)(__nds_frontend_select_start - __nds_frontend_start) & ~31u;

    if (bytes == NULL)
    {
        return NULL;
    }
    *bytes = 0u;
    if ((sNdsFrontendLoaded == 0u) || (sNdsFrontendTailLoaned != 0u) ||
        (sNdsFrontendCursor != NULL) || (lent == 0u) ||
        (gSCManagerSceneData.scene_curr != nSCKindPlayersVS))
    {
        return NULL;
    }
    sNdsFrontendSelectDirty = 1u;
    *bytes = lent;
    return __nds_frontend_start;
}

/* The loan: range 1 from `first` to the overlay's end, and range 2
 * [start2, end2) when there is one (taken once range 1 is full). */
static void ndsFrontendOverlayLendRanges(u8 *first, u8 *start2, u8 *end2)
{
    sNdsFrontendCursor = first;
    sNdsFrontendLoanEnd = __nds_frontend_end;
    gNdsFrontendOverlayLoanBytes = (u32)(__nds_frontend_end - first);
    if ((start2 != NULL) && (end2 > start2))
    {
        sNdsFrontendStart2 = start2;
        sNdsFrontendCursor2 = start2;
        sNdsFrontendLoanEnd2 = end2;
        gNdsFrontendOverlayLoanBytes += (u32)(end2 - start2);
    }
}

/* A 1P Game or bonus battle keeps the 1P head's core -- the manager and the
 * 1P Game, which every 1P battle runs and returns into -- and the parts its
 * stage runs (linker script, in order: Giant Donkey Kong's motion table,
 * Master Hand's code and table, the Fighting Polygon Team's motion tables,
 * the bonus boards' and Race to the Finish's code; the Race runs Polygons
 * too); every other part lends with the tail. */
static void ndsFrontendOverlayLend1P(u32 kind)
{
    if (kind == (u32)nSCKind1PBonusStage)
    {
        /* Break the Targets, Board the Platforms (practice or campaign). */
        ndsFrontendOverlayLendRanges(__nds_frontend_1p_end,
                                     __nds_frontend_1p_gdonkey,
                                     __nds_frontend_1p_bonus);
        return;
    }
    switch (gSCManagerSceneData.spgame_stage)
    {
    case nSC1PGameStageDonkey:
        ndsFrontendOverlayLendRanges(__nds_frontend_1p_boss, NULL, NULL);
        break;
    case nSC1PGameStageBoss:
        ndsFrontendOverlayLendRanges(__nds_frontend_1p_polygon,
                                     __nds_frontend_1p_gdonkey,
                                     __nds_frontend_1p_boss);
        break;
    case nSC1PGameStageZako:
        ndsFrontendOverlayLendRanges(__nds_frontend_1p_bonus,
                                     __nds_frontend_1p_gdonkey,
                                     __nds_frontend_1p_polygon);
        break;
    case nSC1PGameStageBonus3:
        /* The Race's Polygons and its own code, adjacent. */
        ndsFrontendOverlayLendRanges(__nds_frontend_1p_end,
                                     __nds_frontend_1p_gdonkey,
                                     __nds_frontend_1p_polygon);
        break;
    default:
        /* Every other ladder stage and every challenger fight. */
        ndsFrontendOverlayLendRanges(__nds_frontend_1p_gdonkey, NULL, NULL);
        break;
    }
}

void ndsFrontendOverlayBeginScene(u32 kind)
{
    sNdsFrontendCursor = NULL;
    sNdsFrontendStart2 = NULL;
    sNdsFrontendCursor2 = NULL;
    sNdsFrontendLoanEnd2 = NULL;
    gNdsFrontendOverlayLoanBytes = 0u;
    gNdsFrontendOverlayUsedBytes = 0u;
    gNdsFrontendOverlayAllocCount = 0u;
    gNdsFrontendOverlaySpillBytes = 0u;
    sNdsFrontendLoanEnd = __nds_frontend_end;
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
        /* Training runs only its own head (linker script): the rest of the
         * 1P head lends too. A 1P Game or bonus battle keeps the head's core
         * and its own stage's part. */
        if (kind == (u32)nSCKind1PTrainingMode)
        {
            ndsFrontendOverlayLendRanges(__nds_frontend_training_end, NULL, NULL);
        }
        else
        {
            ndsFrontendOverlayLend1P(kind);
        }
        gNdsFrontendOverlayLoanCount++;
        NDS_DIAG(gNdsFrontendOverlayTailLoanCount++);
        sNdsFrontendTailLoaned = 1u;
    }
    else if (((kind == (u32)nSCKindEnding) ||
              (kind == (u32)nSCKind1PContinue)
#if NDS_1P_INTRO_BAKE
              /* LAB: the still bake's live intro makes the cards' fighters
               * (battleship_sc1pintro.c), which the shipping intro never
               * does: Palutena's high image (24,940 B) ran its heap out. */
              || (kind == (u32)nSCKind1PIntro)
#endif
             ) &&
             (sNdsFrontendLoaded != 0u) && (sNdsFrontendTailDirty == 0u))
    {
        /* The 1P ending movie runs its own code (the middle range), the 1P
         * manager's (the head) and then the staff roll's and Congratulations'
         * (the middle again); the continue screen runs its own and the
         * manager's. The Results block and the select's block after it are
         * idle until the overlay reloads. A content's ending fighter ran the
         * heap out there (P7 C1, Wario: 35,760 B asked, 14,376 free,
         * 2026-10-19); they lend 199,712 B on the walk layout. */
        sNdsFrontendCursor = __nds_frontend_results_start;
        sNdsFrontendLoanEnd = __nds_frontend_select_end;
        gNdsFrontendOverlayLoanBytes =
            (u32)(__nds_frontend_select_end - __nds_frontend_results_start);
        /* The ending runs no stage's part of the 1P head (Giant Donkey
         * Kong's and the Polygons' tables, Master Hand, the bonus boards),
         * and no ladder stage follows it: those parts lend too (2026-10-20:
         * Bowser's ending fighter left the room table 38 KB short, and the
         * room went undrawn). The continue screen keeps them: the manager
         * goes from it back into the stage it continues, with no reload. */
        if (kind == (u32)nSCKindEnding)
        {
            sNdsFrontendStart2 = __nds_frontend_1p_gdonkey;
            sNdsFrontendCursor2 = __nds_frontend_1p_gdonkey;
            sNdsFrontendLoanEnd2 = __nds_frontend_1p_end;
            gNdsFrontendOverlayLoanBytes +=
                (u32)(__nds_frontend_1p_end - __nds_frontend_1p_gdonkey);
        }
        gNdsFrontendOverlayLoanCount++;
        sNdsFrontendTailLoaned = 1u;
    }
    else if ((kind == (u32)nSCKindVSResults) && (sNdsFrontendLoaded != 0u))
    {
        /* VS Results runs only its own block, linked last; everything before
         * it is this scene's asset storage (a battle of contents reloads their
         * select packs and images here), as a battle's tail loan, and the next
         * dispatch reloads the overlay once an allocation landed in it. */
        sNdsFrontendCursor = __nds_frontend_start;
        sNdsFrontendLoanEnd = __nds_frontend_results_start;
        sNdsFrontendStart2 = __nds_frontend_select_start;
        sNdsFrontendCursor2 = __nds_frontend_select_start;
        sNdsFrontendLoanEnd2 = __nds_frontend_select_end;
        gNdsFrontendOverlayLoanBytes =
            (u32)(__nds_frontend_results_start - __nds_frontend_start) +
            (u32)(__nds_frontend_select_end - __nds_frontend_select_start);
        gNdsFrontendOverlayLoanCount++;
        sNdsFrontendTailLoaned = 1u;
    }
}

void ndsFrontendOverlayEndScene(void)
{
    sNdsFrontendCursor = NULL;
    sNdsFrontendStart2 = NULL;
    sNdsFrontendCursor2 = NULL;
}

static void *ndsFrontendOverlayTryAllocIn(u8 **cursor, u8 *limit,
                                          size_t bytes, u32 alignment);

void *ndsFrontendOverlayTryAlloc(size_t bytes, u32 alignment)
{
    void *storage;

    if ((sNdsFrontendCursor == NULL) || (bytes == 0u) ||
        (alignment == 0u) || ((alignment & (alignment - 1u)) != 0u))
    {
        return NULL;
    }
    storage = ndsFrontendOverlayTryAllocIn(
        &sNdsFrontendCursor,
        (sNdsFrontendLoanEnd != NULL) ? sNdsFrontendLoanEnd : __nds_frontend_end,
        bytes, alignment);
    if ((storage == NULL) && (sNdsFrontendCursor2 != NULL))
    {
        storage = ndsFrontendOverlayTryAllocIn(&sNdsFrontendCursor2,
                                               sNdsFrontendLoanEnd2, bytes,
                                               alignment);
    }
    if (storage != NULL)
    {
        if (sNdsFrontendTailLoaned != 0u)
        {
            sNdsFrontendTailDirty = 1u;
        }
        gNdsFrontendOverlayUsedBytes =
            (u32)(sNdsFrontendCursor - __nds_frontend_start) +
            ((sNdsFrontendCursor2 != NULL) ?
                 (u32)(sNdsFrontendCursor2 - sNdsFrontendStart2) : 0u);
        gNdsFrontendOverlayAllocCount++;
    }
    return storage;
}

static void *ndsFrontendOverlayTryAllocIn(u8 **cursor, u8 *limit,
                                          size_t bytes, u32 alignment)
{
    uintptr_t start;
    uintptr_t end = (uintptr_t)limit;

    start = (uintptr_t)*cursor;
    if (start > UINTPTR_MAX - (alignment - 1u))
    {
        return NULL;
    }
    start = (start + alignment - 1u) & ~(uintptr_t)(alignment - 1u);
    if ((start > end) || (bytes > end - start))
    {
        return NULL;
    }
    *cursor = (u8 *)(start + bytes);
    return (void *)start;
}

/* A VS battle's syTaskmanMalloc takes the loan first, so a buffer it handed
 * out (an effect's display lists, an MObj chain) may sit in the lent range
 * rather than the taskman arena; the renderer's range checks accept both
 * (a Crash mirror's effect DLs failed native as BAD_ASSET, 2026-10-18). Only
 * the bytes already lent: [start, cursor) and Results' second range. */
s32 ndsFrontendOverlayLoanContains(const void *ptr, size_t bytes)
{
    uintptr_t addr = (uintptr_t)ptr;

    if ((sNdsFrontendCursor != NULL) &&
        (addr >= (uintptr_t)__nds_frontend_start) &&
        (addr < (uintptr_t)sNdsFrontendCursor) &&
        (bytes <= (size_t)((uintptr_t)sNdsFrontendCursor - addr)))
    {
        return TRUE;
    }
    if ((sNdsFrontendCursor2 != NULL) &&
        (addr >= (uintptr_t)sNdsFrontendStart2) &&
        (addr < (uintptr_t)sNdsFrontendCursor2) &&
        (bytes <= (size_t)((uintptr_t)sNdsFrontendCursor2 - addr)))
    {
        return TRUE;
    }
    return FALSE;
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

/* A VS battle lends the whole range, and its fighters' packs, images and files
 * leave the end of it idle: 41,696 B with four P4 contents (play-1011x14),
 * while the same match on Sector Z ran the general heap out making the
 * fighters' parts at entry (owner, 2026-10-11: a frozen host with nothing on
 * screen). Data that lives exactly as long as the battle -- the event32
 * ledger, the contents' match tables, the shield poses, the entry corners --
 * takes that tail first. Only a VS battle: a campaign battle's tail holds
 * its own files. */
u32 ndsSceneIsVSBattle(void)
{
    return (gSCManagerSceneData.scene_curr == nSCKindVSBattle) ? TRUE : FALSE;
}

void *ndsVSBattleOverlayTryAlloc(size_t bytes, u32 alignment)
{
    if (gSCManagerSceneData.scene_curr != nSCKindVSBattle)
    {
        return NULL;
    }
    return ndsFrontendOverlayTryAlloc(bytes, alignment);
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
        /* Short of the heap, a battle's lent bytes may still hold it: its
         * dead spans, then the stage program's, which gives way to an asset
         * the match asks for (Kirby's copy hat: 7,636 B on Congo Jungle). */
        storage = ndsBattleDeadSpanAlloc(bytes, alignment);
        if ((storage == NULL) && (ndsStageGxYield != NULL) &&
            (ndsStageGxYield(bytes + alignment) != FALSE))
        {
            storage = ndsBattleDeadSpanAlloc(bytes, alignment);
        }
        return storage;
    }
    if (sNdsFrontendCursor != NULL)
    {
        gNdsFrontendOverlaySpillBytes += (u32)bytes;
    }
    return syTaskmanMalloc(bytes, alignment);
}
