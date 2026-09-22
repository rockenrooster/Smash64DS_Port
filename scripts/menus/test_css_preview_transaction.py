"""Host-execute the REAL character-select preview transaction (M03/M04).

The repaired path replaces one blocking closure -- ~8.6 frame budgets of
synchronous fighter-file setup wrapped in a BGM suspend/resume, eight times per
CSS visit -- with a resumable transaction that advances one bounded span per
screen update while the shell loop keeps servicing the stream.

Three properties have to hold or the repair is worse than the bug, and all
three are state-machine properties that a host can execute directly:

  1. CANCELLATION IS SHARED-AWARE.  One in-flight closure can satisfy several
     slots that picked the same fighter.  A slot moving its cursor is only a
     cancellation when nobody else is still parked on that acquire, and the
     service pass re-checks before it actually retires anything.
  2. A STALE COMPLETION IS REJECTED.  A transaction opened against one taskman
     resource generation must never publish into the next one.
  3. THE PER-UPDATE BUDGET IS A BOUND ON EACH UNIT, NOT A CHECK AFTER ONE HUGE
     READ.  Every loader step is handed at most NDS_PLAYERS_VS_LOAD_STEP_BYTES,
     every update spends at most NDS_PLAYERS_VS_PREVIEW_SERVICE_BYTES across
     ALL FOUR SLOTS, and the reader never asks the filesystem for more than the
     span it was handed.

These tests EXTRACT the production bodies verbatim (source_test_helpers) and
run them on a host harness:

  * extracted, unmodified, from src/import/battleship_mnplayersvs.c:
    ndsMNPlayersVSPreviewRequestLoadCancel, ndsMNPlayersVSPreviewServiceLoadCancel,
    ndsMNPlayersVSPreviewServiceCompactLoad.
  * extracted, unmodified, from src/port/reloc_preview_pack.c:
    ndsPreviewHash, ndsPreviewRange, ndsPreviewSectionContains,
    ndsPreviewPackLoadRelease, ndsRelocPreviewFighterLoadStep, plus the
    NDSPreviewPackLoad record and its state enum.
  * stubbed (explicit fixture seams): the resident-block cancel/retire helpers,
    the malloc-region swap, ftManagerSetupFilesAllKind, the preparation stages,
    the owner-image capacity test, and -- for the loader -- the reloc register
    and normalize seams.  `fread` is redirected through a recording wrapper so
    the LARGEST SINGLE FILESYSTEM REQUEST is measurable; the file itself is a
    real synthetic FPC2 payload built by the harness, so the reader's chunking,
    running hash and byte-lane pass are the production ones.

Every test has a negative control that patches ONE line of the extracted
production body and must fail.  The mutations are listed on each control.

LIMITATIONS (deliberate): this proves the transaction's ownership, staleness
and budget arithmetic.  It cannot prove wall-clock audio continuity, the cost
of one native owner-image read, or that a step's budget corresponds to any
particular number of DS cycles -- those need the runtime capture named in
docs/VERIFYING.md.
"""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from source_test_helpers import braced, clean, function

ROOT = Path(__file__).resolve().parents[2]
VS = (ROOT / "src/import/battleship_mnplayersvs.c").read_text(encoding="utf-8")
PACK = (ROOT / "src/port/reloc_preview_pack.c").read_text(encoding="utf-8")
HDR = (ROOT / "include/nds/nds_preview_pack.h").read_text(encoding="utf-8")


def pin_kib(source, name):
    """Parse back a `(N u * 1024u)` constant so a drift fails loudly here."""
    hit = re.search(rf"#define\s+{name}\s+\((\d+)u\s*\*\s*1024u\)", clean(source))
    if hit is None:
        raise AssertionError(f"{name} missing or no longer a KiB constant")
    return int(hit.group(1)) * 1024


def pin_plain(source, name):
    hit = re.search(rf"#define\s+{name}\s+(\d+)u\b", clean(source))
    if hit is None:
        raise AssertionError(f"{name} missing or no longer a plain constant")
    return int(hit.group(1))


SERVICE_BYTES = pin_kib(VS, "NDS_PLAYERS_VS_PREVIEW_SERVICE_BYTES")
STEP_BYTES = pin_kib(VS, "NDS_PLAYERS_VS_LOAD_STEP_BYTES")
DWELL_COLD = pin_plain(VS, "NDS_PLAYERS_VS_PREVIEW_DWELL_TICKS")
DWELL_WARM = pin_plain(VS, "NDS_PLAYERS_VS_PREVIEW_DWELL_WARM_TICKS")

# The repair is pointless if a unit can be as large as the whole update, and
# M04 is pointless if the warm case still pays the cold interval.
assert STEP_BYTES < SERVICE_BYTES, (STEP_BYTES, SERVICE_BYTES)
assert 0 < DWELL_WARM <= DWELL_COLD, (DWELL_WARM, DWELL_COLD)
# The 13-tic dwell M04 names as the defect must be gone.
assert DWELL_COLD < 13, DWELL_COLD

# Largest shipped preview pack (Link, 05.fpc): its data span is what the
# reader has to chunk.  Held here as the fixture size, not read from the build.
LINK_DATA_BYTES = 32032
LINK_FIXUPS = 244


def not_suspended():
    """The compact acquire must no longer fence the track around a load."""
    body = braced(VS, r"^ndsMNPlayersVSPreviewServiceCompactLoad\(")
    for banned in ("ndsAudioBgmSuspendForBlockingLoad",
                   "ndsAudioBgmResumeAfterBlockingLoad"):
        if banned in body:
            raise AssertionError(
                f"the compact transaction still calls {banned}; M03 requires "
                "no routine preview suspend/resume")
    return body


COMPACT_BODY = not_suspended()

HEAD = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef s32 sb32;
typedef uintptr_t uptr;
#define TRUE 1
#define FALSE 0
#define ARRAY_COUNT(a) ((u32)(sizeof(a) / sizeof((a)[0])))
#define UINT32_MAX_U 0xffffffffu

static int gFailures;
#define CHECK(cond, ...)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            gFailures++;                                                       \
            printf("FAIL %s:%d ", __func__, __LINE__);                         \
            printf(__VA_ARGS__);                                               \
            printf("\n");                                                      \
        }                                                                      \
    } while (0)
'''

VS_FIXTURE = r'''
/* ---- resident-block fixture (mirrors the production record) ------------- */
#define GMCOMMON_PLAYERS_MAX 4
#define NDS_PLAYERS_VS_RESIDENT_BLOCKS GMCOMMON_PLAYERS_MAX
#define NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES (80u * 1024u)
#define nFTKindNull (-1)
#define nFTKindPlayableStart 0
#define nFTKindPlayableEnd 11

typedef struct SYMallocRegion { int tag; } SYMallocRegion;
typedef struct FTData { void **p_file_main; void **p_file_model; } FTData;

enum {
    NDS_PLAYERS_VS_PREPARE_COMMIT = 0,
    NDS_PLAYERS_VS_PREPARE_ANIM_IDLE,
    NDS_PLAYERS_VS_PREPARE_ANIM_SELECTED,
    NDS_PLAYERS_VS_PREPARE_OWNER_HIGH,
    NDS_PLAYERS_VS_PREPARE_OWNER_LOW,
    NDS_PLAYERS_VS_PREPARE_READY
};
typedef enum NDSPlayersVSResidentAcquireResult {
    nNDSPlayersVSResidentAcquireFail = 0,
    nNDSPlayersVSResidentAcquireReady = 1,
    nNDSPlayersVSResidentAcquireRetry = 2
} NDSPlayersVSResidentAcquireResult;

typedef struct NDSPlayersVSResidentBlock {
    SYMallocRegion arena;
    void *base;
    s32 fkind;
    u32 refs;
    void *load_cursor;
    void *load_root;
    s32 loading_fkind;
    sb32 load_tree_done;
    sb32 cancel_requested;
    u32 load_stage;
    u32 load_generation;
} NDSPlayersVSResidentBlock;

typedef struct NDSPlayersVSPreviewPending {
    s32 fkind;
    u32 stable_tics;
    sb32 seen_request;
    sb32 acquire_pending;
    u32 request_updates;
    sb32 request_warm;
} NDSPlayersVSPreviewPending;

static NDSPlayersVSResidentBlock sNdsPlayersVSResidentBlocks[4];
static NDSPlayersVSPreviewPending sNdsPlayersVSPreviewPending[4];
static u32 sNdsPlayersVSPreviewResidencyActionBudget;
static u32 sNdsPlayersVSPreviewServiceByteBudget;
static sb32 sNdsPlayersVSPreviewEntryDrain;
static u32 gNdsTaskmanHeapGeneration = 7u;
static u32 gNdsRelocAssetPayloadReadCount;
static FTData sFighterData[12];
static void *sFighterMain[12];
static void *sFighterModel[12];
static FTData *dFTManagerDataFiles[12];

static u32 gNdsPlayersVSPreviewAcquireLoadCount;
static u32 gNdsPlayersVSPreviewAcquireLoadFinishCount;
static u32 gNdsPlayersVSPreviewAcquireRetryCount;
static u32 gNdsPlayersVSPreviewAcquireFailCount;
static u32 gNdsPlayersVSPreviewAcquirePayloadReadCount;
static u32 gNdsPlayersVSPreviewAcquirePayloadReadMax;
static u32 gNdsPlayersVSPreviewAcquireTreePayloadByteCount;
static u32 gNdsPlayersVSPreviewAcquireTreePayloadByteMax;
static u32 gNdsPlayersVSPreviewServiceStepCount;
static u32 gNdsPlayersVSPreviewServiceByteMax;
static u32 gNdsPlayersVSPreviewStaleCommitRejectCount;
static u32 gNdsPlayersVSPreviewPrepareStageCount;
static u32 gNdsPlayersVSPreviewResidentCapacityFailCount;

/* ---- fixture seams ------------------------------------------------------ */
static u32 sCancelCalls;
static s32 sCancelledKind;
static u32 sStepCalls;
static u32 sStepBudgetMax;       /* largest span any step was HANDED */
static u32 sStepMovedMax;        /* largest span any step actually moved */
static u32 sUpdateMoved;         /* bytes moved so far this update */
static u32 sUpdateMovedMax;
static u32 sUpdateStages;        /* preparation stages run this update */
static u32 sUpdateStagesMax;
static u32 sRemainingBytes;      /* payload the fake pack still owes */
static u32 sPrepareStagesRun;
static sb32 sPrepareFails;

/* The real loader's step contract, reduced to its budget behaviour. */
static s32 ndsRelocPreviewFighterLoadStepStub(void *handle, u32 byte_budget,
                                              u32 *out_bytes)
{
    u32 moved = byte_budget;

    (void)handle;
    sStepCalls++;
    if (byte_budget > sStepBudgetMax) { sStepBudgetMax = byte_budget; }
    if (byte_budget == 0u) { moved = sRemainingBytes; }   /* entry drain */
    if (moved > sRemainingBytes) { moved = sRemainingBytes; }
    sRemainingBytes -= moved;
    if (moved > sStepMovedMax) { sStepMovedMax = moved; }
    sUpdateMoved += moved;
    if (sUpdateMoved > sUpdateMovedMax) { sUpdateMovedMax = sUpdateMoved; }
    if (out_bytes != NULL) { *out_bytes = moved; }
    return (sRemainingBytes == 0u) ? 2 : 1;
}
#define NDS_PREVIEW_PACK_STEP_FAIL 0
#define NDS_PREVIEW_PACK_STEP_IN_PROGRESS 1
#define NDS_PREVIEW_PACK_STEP_DONE 2
#define ndsRelocPreviewFighterLoadStep ndsRelocPreviewFighterLoadStepStub

static void *ndsRelocPreviewFighterLoadBegin(s32 fkind)
{
    (void)fkind;
    sRemainingBytes = LINK_DATA_BYTES;
    return (void *)(uptr)0xf0c1u;
}
static void ndsRelocPreviewFighterLoadCancel(void *h, s32 fkind)
{
    (void)h; (void)fkind;
}
static SYMallocRegion sScene;
static SYMallocRegion *sCurrentRegion = &sScene;
static SYMallocRegion *ndsTaskmanSwapMallocRegion(SYMallocRegion *next)
{
    SYMallocRegion *previous = sCurrentRegion;
    sCurrentRegion = next;
    return previous;
}
static void syMallocReset(SYMallocRegion *r) { (void)r; }
static void ndsRelocReleaseHeapRange(void *b, u32 n) { (void)b; (void)n; }
static void ndsRendererNativeReleaseOwnerImagesInRange(void *b, u32 n)
{
    (void)b; (void)n;
}
static void ndsMNPlayersClearPreviewFighterFiles(s32 k) { (void)k; }
static void ndsRelocReleasePreviewFighter(s32 k) { (void)k; }
static void ndsMNPlayersVSPreviewMarkPermanentFailure(s32 k) { (void)k; }
static void ndsMNPlayersVSPreviewResetResidentLoadState(
    NDSPlayersVSResidentBlock *block)
{
    block->load_cursor = NULL;
    block->load_root = NULL;
    block->loading_fkind = nFTKindNull;
    block->load_tree_done = FALSE;
    block->cancel_requested = FALSE;
    block->load_stage = NDS_PLAYERS_VS_PREPARE_COMMIT;
    block->load_generation = 0u;
}
static sb32 ndsMNPlayersVSPreviewCancelResidentLoad(
    NDSPlayersVSResidentBlock *block)
{
    if ((block == NULL) || (block->load_cursor == NULL)) { return FALSE; }
    sCancelCalls++;
    sCancelledKind = block->loading_fkind;
    ndsMNPlayersVSPreviewResetResidentLoadState(block);
    return TRUE;
}
static void ndsMNPlayersVSPreviewAbandonCompactLoad(
    NDSPlayersVSResidentBlock *block, s32 fkind)
{
    (void)fkind;
    ndsMNPlayersVSPreviewResetResidentLoadState(block);
}
static void ftManagerSetupFilesAllKind(s32 fkind)
{
    /* The owner boundary: this is the ONLY place the global fighter-file
     * pointers become non-NULL, exactly as production publishes them. */
    sFighterMain[fkind] = (void *)(uptr)0x1000u;
    sFighterModel[fkind] = (void *)(uptr)0x2000u;
}
static sb32 ndsMNPlayersVSPreviewOwnerImagesFit(
    const NDSPlayersVSResidentBlock *block, s32 fkind)
{
    (void)block; (void)fkind;
    return TRUE;
}
static sb32 ndsMNPlayersVSPreviewPrepareResidentStage(
    s32 fkind, SYMallocRegion *anim_region, u32 stage)
{
    (void)fkind; (void)stage;
    /* Production swaps to the block arena and hands the PREVIOUS region in as
     * the animation region; a stage that saw the block arena here would put
     * scene-lifetime cache bytes in a resettable block. */
    CHECK(anim_region == &sScene, "stage %u got the wrong anim region", stage);
    sPrepareStagesRun++;
    return (sPrepareFails != FALSE) ? FALSE : TRUE;
}

static void fixture_reset(void)
{
    u32 i;

    memset(sNdsPlayersVSResidentBlocks, 0, sizeof(sNdsPlayersVSResidentBlocks));
    memset(sNdsPlayersVSPreviewPending, 0, sizeof(sNdsPlayersVSPreviewPending));
    for (i = 0u; i < 4u; i++)
    {
        sNdsPlayersVSResidentBlocks[i].fkind = nFTKindNull;
        sNdsPlayersVSResidentBlocks[i].loading_fkind = nFTKindNull;
        sNdsPlayersVSResidentBlocks[i].base = (void *)(uptr)(0x100000u + i);
        sNdsPlayersVSPreviewPending[i].fkind = nFTKindNull;
    }
    for (i = 0u; i < 12u; i++)
    {
        sFighterMain[i] = NULL;
        sFighterModel[i] = NULL;
        sFighterData[i].p_file_main = &sFighterMain[i];
        sFighterData[i].p_file_model = &sFighterModel[i];
        dFTManagerDataFiles[i] = &sFighterData[i];
    }
    sCancelCalls = 0u;
    sCancelledKind = nFTKindNull;
    sStepCalls = 0u;
    sStepBudgetMax = 0u;
    sStepMovedMax = 0u;
    sUpdateMoved = 0u;
    sUpdateMovedMax = 0u;
    sUpdateStages = 0u;
    sUpdateStagesMax = 0u;
    sPrepareStagesRun = 0u;
    sPrepareFails = FALSE;
    sRemainingBytes = 0u;
    sCurrentRegion = &sScene;
    sNdsPlayersVSPreviewEntryDrain = FALSE;
    gNdsTaskmanHeapGeneration = 7u;
    gNdsPlayersVSPreviewStaleCommitRejectCount = 0u;
    gNdsPlayersVSPreviewPrepareStageCount = 0u;
}

/* One CSS update: refresh the aggregate budgets exactly as SyncRules does. */
static void update_begin(void)
{
    if (sUpdateMoved > sUpdateMovedMax) { sUpdateMovedMax = sUpdateMoved; }
    if (sUpdateStages > sUpdateStagesMax) { sUpdateStagesMax = sUpdateStages; }
    sUpdateMoved = 0u;
    sUpdateStages = 0u;
    sNdsPlayersVSPreviewResidencyActionBudget = 1u;
    sNdsPlayersVSPreviewServiceByteBudget = SERVICE_BYTES_PIN;
}
'''

CANCEL_MAIN = r'''
int main(void)
{
    u32 pass;

    /* Two slots both parked on fighter 5; block 1 is loading it. */
    fixture_reset();
    sNdsPlayersVSResidentBlocks[1].load_cursor = (void *)(uptr)0xabcdu;
    sNdsPlayersVSResidentBlocks[1].loading_fkind = 5;
    sNdsPlayersVSPreviewPending[0].acquire_pending = TRUE;
    sNdsPlayersVSPreviewPending[0].fkind = 5;
    sNdsPlayersVSPreviewPending[2].acquire_pending = TRUE;
    sNdsPlayersVSPreviewPending[2].fkind = 5;

    /* Slot 0 moves away.  Slot 2 still needs that exact closure, so this is
     * not a cancellation -- moving one cursor is not permission to retire
     * another slot's transaction. */
    sNdsPlayersVSPreviewPending[0].acquire_pending = FALSE;
    ndsMNPlayersVSPreviewRequestLoadCancel(5, 0u);
    CHECK(sNdsPlayersVSResidentBlocks[1].cancel_requested == FALSE,
          "a shared in-flight kind was marked for cancellation by one slot");

    /* Even if something DID mark it, the service pass re-checks live demand. */
    sNdsPlayersVSResidentBlocks[1].cancel_requested = TRUE;
    sNdsPlayersVSPreviewResidencyActionBudget = 1u;
    ndsMNPlayersVSPreviewServiceLoadCancel();
    CHECK(sCancelCalls == 0u,
          "service retired a block slot 2 is still waiting on (%u calls)",
          sCancelCalls);
    CHECK(sNdsPlayersVSResidentBlocks[1].load_cursor != NULL,
          "a referenced in-flight block lost its cursor");

    /* Now the last claimant leaves.  Exactly one retirement, one action. */
    sNdsPlayersVSPreviewPending[2].acquire_pending = FALSE;
    ndsMNPlayersVSPreviewRequestLoadCancel(5, 2u);
    CHECK(sNdsPlayersVSResidentBlocks[1].cancel_requested != FALSE,
          "an unwanted in-flight kind was never marked for cancellation");
    sNdsPlayersVSPreviewResidencyActionBudget = 1u;
    ndsMNPlayersVSPreviewServiceLoadCancel();
    CHECK(sCancelCalls == 1u, "expected one retirement, saw %u", sCancelCalls);
    CHECK(sCancelledKind == 5, "retired the wrong kind: %d", sCancelledKind);
    CHECK(sNdsPlayersVSPreviewResidencyActionBudget == 0u,
          "a retirement did not spend the screen's action budget");

    /* A second pass must find nothing left to do and spend nothing. */
    sNdsPlayersVSPreviewResidencyActionBudget = 1u;
    ndsMNPlayersVSPreviewServiceLoadCancel();
    CHECK(sCancelCalls == 1u, "service retired twice (%u)", sCancelCalls);
    CHECK(sNdsPlayersVSPreviewResidencyActionBudget == 1u,
          "an idle service pass spent budget");

    /* A zero budget must defer, never retire. */
    fixture_reset();
    sNdsPlayersVSResidentBlocks[2].load_cursor = (void *)(uptr)0xbeefu;
    sNdsPlayersVSResidentBlocks[2].loading_fkind = 9;
    sNdsPlayersVSResidentBlocks[2].cancel_requested = TRUE;
    sNdsPlayersVSPreviewResidencyActionBudget = 0u;
    for (pass = 0u; pass < 3u; pass++) { ndsMNPlayersVSPreviewServiceLoadCancel(); }
    CHECK(sCancelCalls == 0u, "service ignored an exhausted action budget");

    printf(gFailures ? "CANCEL FAIL\n" : "CANCEL OK\n");
    return gFailures ? 1 : 0;
}
'''

BUDGET_MAIN = r'''
int main(void)
{
    NDSPlayersVSResidentBlock *block = &sNdsPlayersVSResidentBlocks[0];
    NDSPlayersVSResidentAcquireResult result;
    u32 updates = 0u;
    u32 ready_update = 0u;

    fixture_reset();

    /* Cold miss on Link, the largest shipped pack.  Drive whole updates until
     * the transaction reaches READY, exactly as the screen would. */
    for (updates = 1u; updates <= 400u; updates++)
    {
        update_begin();
        result = ndsMNPlayersVSPreviewServiceCompactLoad(block, 5);
        if (block->load_stage != NDS_PLAYERS_VS_PREPARE_COMMIT)
        {
            /* One preparation stage per update, at most. */
            sUpdateStages = gNdsPlayersVSPreviewPrepareStageCount - sUpdateStages;
        }
        CHECK(result != nNDSPlayersVSResidentAcquireFail,
              "the cold transaction failed at update %u", updates);
        if (result == nNDSPlayersVSResidentAcquireReady)
        {
            ready_update = updates;
            break;
        }
        CHECK(block->fkind == nFTKindNull,
              "a slot could see kind %d before READY (update %u)",
              block->fkind, updates);
    }
    CHECK(ready_update != 0u, "the cold transaction never completed");

    /* THE BOUND.  Not "a time check after one huge read": every unit the
     * loader was HANDED, and every unit it actually moved, is at or under the
     * per-unit span, and no single update spent more than the screen's one
     * aggregate budget. */
    CHECK(sStepBudgetMax <= STEP_BYTES_PIN,
          "a loader step was handed %u bytes, over the %u-byte unit bound",
          sStepBudgetMax, (u32)STEP_BYTES_PIN);
    CHECK(sStepMovedMax <= STEP_BYTES_PIN,
          "a loader step moved %u bytes, over the %u-byte unit bound",
          sStepMovedMax, (u32)STEP_BYTES_PIN);
    if (sUpdateMoved > sUpdateMovedMax) { sUpdateMovedMax = sUpdateMoved; }
    CHECK(sUpdateMovedMax <= SERVICE_BYTES_PIN,
          "one update spent %u bytes, over the %u-byte aggregate budget",
          sUpdateMovedMax, (u32)SERVICE_BYTES_PIN);
    CHECK(gNdsPlayersVSPreviewPrepareStageCount ==
              (u32)NDS_PLAYERS_VS_PREPARE_READY,
          "expected %d preparation stages, ran %u",
          (int)NDS_PLAYERS_VS_PREPARE_READY,
          gNdsPlayersVSPreviewPrepareStageCount);
    CHECK(block->fkind == 5, "READY did not name the kind (%d)", block->fkind);
    CHECK(block->refs == 1u, "READY did not take the first reference");
    CHECK(block->load_cursor == NULL, "READY left the transaction open");
    CHECK(sFighterMain[5] != NULL,
          "READY published a slot whose closure was never committed");

    /* FOUR SLOTS SHARE ONE BUDGET.  Four cold transactions on one update must
     * not each get a full budget: the bound is a property of the update. */
    fixture_reset();
    update_begin();
    {
        u32 i;
        for (i = 0u; i < 4u; i++)
        {
            sNdsPlayersVSResidentBlocks[i].load_cursor = (void *)(uptr)0xf0c1u;
            sNdsPlayersVSResidentBlocks[i].loading_fkind = (s32)i;
            sNdsPlayersVSResidentBlocks[i].load_generation =
                gNdsTaskmanHeapGeneration;
        }
        sRemainingBytes = LINK_DATA_BYTES * 4u;
        for (i = 0u; i < 4u; i++)
        {
            (void)ndsMNPlayersVSPreviewServiceCompactLoad(
                &sNdsPlayersVSResidentBlocks[i], (s32)i);
        }
    }
    CHECK(sUpdateMoved <= SERVICE_BYTES_PIN,
          "four slots spent %u bytes on one update, over the %u-byte "
          "aggregate budget", sUpdateMoved, (u32)SERVICE_BYTES_PIN);

    /* The entry load frame is the declared exception and runs before BGM. */
    fixture_reset();
    sNdsPlayersVSPreviewEntryDrain = TRUE;
    sNdsPlayersVSPreviewResidencyActionBudget = 4u;
    sNdsPlayersVSPreviewServiceByteBudget = 0u;
    block = &sNdsPlayersVSResidentBlocks[1];
    (void)ndsMNPlayersVSPreviewServiceCompactLoad(block, 3);
    result = ndsMNPlayersVSPreviewServiceCompactLoad(block, 3);
    CHECK(result == nNDSPlayersVSResidentAcquireReady,
          "the entry load frame did not complete in one update (%d)",
          (int)result);

    printf(gFailures ? "BUDGET FAIL\n" : "BUDGET OK\n");
    return gFailures ? 1 : 0;
}
'''

STALE_MAIN = r'''
int main(void)
{
    NDSPlayersVSResidentBlock *block = &sNdsPlayersVSResidentBlocks[0];
    NDSPlayersVSResidentAcquireResult result;
    u32 i;

    fixture_reset();

    /* Open the transaction, advance it partway, then rewind the scene under
     * it -- Results returning to CSS is exactly this. */
    update_begin();
    result = ndsMNPlayersVSPreviewServiceCompactLoad(block, 5);
    CHECK(result == nNDSPlayersVSResidentAcquireRetry,
          "REQUEST should stage a cursor and retry, got %d", (int)result);
    CHECK(block->load_generation == gNdsTaskmanHeapGeneration,
          "the transaction did not record its resource generation");
    for (i = 0u; i < 2u; i++)
    {
        update_begin();
        (void)ndsMNPlayersVSPreviewServiceCompactLoad(block, 5);
    }
    CHECK(block->load_tree_done == FALSE, "the fixture pack finished too early");

    gNdsTaskmanHeapGeneration++;
    update_begin();
    result = ndsMNPlayersVSPreviewServiceCompactLoad(block, 5);
    CHECK(result == nNDSPlayersVSResidentAcquireFail,
          "a transaction from the previous generation was allowed to continue "
          "(%d)", (int)result);
    CHECK(gNdsPlayersVSPreviewStaleCommitRejectCount == 1u,
          "the stale completion was not counted (%u)",
          gNdsPlayersVSPreviewStaleCommitRejectCount);
    CHECK(block->fkind == nFTKindNull,
          "a stale transaction published kind %d", block->fkind);
    CHECK(block->load_cursor == NULL, "a stale transaction stayed open");
    CHECK(sCancelCalls == 1u,
          "a stale transaction was not cleaned up (%u cancels)", sCancelCalls);

    /* A transaction that reaches the preparation stages is just as stale. */
    fixture_reset();
    block = &sNdsPlayersVSResidentBlocks[2];
    update_begin();
    (void)ndsMNPlayersVSPreviewServiceCompactLoad(block, 4);
    for (i = 0u; i < 64u; i++)
    {
        update_begin();
        if (ndsMNPlayersVSPreviewServiceCompactLoad(block, 4) !=
            nNDSPlayersVSResidentAcquireRetry) { break; }
        if (block->load_stage == NDS_PLAYERS_VS_PREPARE_ANIM_SELECTED) { break; }
    }
    CHECK(block->load_stage == NDS_PLAYERS_VS_PREPARE_ANIM_SELECTED,
          "could not park the fixture mid-preparation (stage %u)",
          block->load_stage);
    gNdsTaskmanHeapGeneration++;
    update_begin();
    result = ndsMNPlayersVSPreviewServiceCompactLoad(block, 4);
    CHECK(result == nNDSPlayersVSResidentAcquireFail,
          "a mid-preparation transaction survived a scene rewind (%d)",
          (int)result);
    CHECK(block->fkind == nFTKindNull,
          "a mid-preparation stale transaction published kind %d", block->fkind);

    printf(gFailures ? "STALE FAIL\n" : "STALE OK\n");
    return gFailures ? 1 : 0;
}
'''


def vs_sources():
    """Verbatim production bodies from the character-select transaction."""
    request = function(VS, "ndsMNPlayersVSPreviewRequestLoadCancel")
    service = function(VS, "ndsMNPlayersVSPreviewServiceLoadCancel")
    for needle in ("changing_slot", "acquire_pending", "cancel_requested"):
        if needle not in request:
            raise AssertionError(f"extracted RequestLoadCancel lost '{needle}'")
    if "acquire_pending" not in service:
        raise AssertionError("extracted ServiceLoadCancel lost its demand re-check")
    return request, service


def compact_source(mutation=None):
    body = "static NDSPlayersVSResidentAcquireResult\n" + COMPACT_BODY
    if mutation is not None:
        anchor, patched = mutation
        if body.count(anchor) != 1:
            raise AssertionError(f"negative-control anchor not unique: {anchor!r}")
        body = body.replace(anchor, patched)
    return body


def build_vs(main_text, mutation=None, with_compact=True, cancel_mutation=None):
    request, service = vs_sources()
    if cancel_mutation is not None:
        anchor, patched = cancel_mutation
        if request.count(anchor) + service.count(anchor) != 1:
            raise AssertionError(f"negative-control anchor not unique: {anchor!r}")
        request = request.replace(anchor, patched)
        service = service.replace(anchor, patched)
    pins = (f"#define LINK_DATA_BYTES {LINK_DATA_BYTES}u\n"
            f"#define SERVICE_BYTES_PIN {SERVICE_BYTES}u\n"
            f"#define STEP_BYTES_PIN {STEP_BYTES}u\n"
            f"#define NDS_PLAYERS_VS_PREVIEW_SERVICE_BYTES {SERVICE_BYTES}u\n"
            f"#define NDS_PLAYERS_VS_LOAD_STEP_BYTES {STEP_BYTES}u\n")
    parts = [HEAD, pins, VS_FIXTURE, request, service]
    if with_compact:
        parts.append(compact_source(mutation))
    parts.append(main_text)
    return "\n".join(parts)


# --------------------------------------------------------------------------
# The reader itself: real chunking, real running hash, real byte-lane pass.
# --------------------------------------------------------------------------

PACK_FIXTURE = r'''
#define NDS_PREVIEW_PACK_MAX_SECTIONS 4u
#define NDS_PREVIEW_PACK_NULL 0xffffffffu
#define NDS_P2_SHELL_ARGMAX_ROSTER 0
#define NDS_P2_COMPACT_BATTLE_FIGHTERS 0

typedef struct NDSPreviewPackSection {
    u32 asset_id; u32 data_offset; u32 data_bytes; u32 source_bytes;
    u32 first_span; u32 span_count; u32 roots_offset; u32 root_count;
} NDSPreviewPackSection;
typedef struct NDSPreviewPackFixup { u32 slot_offset; u32 target_offset; }
    NDSPreviewPackFixup;
typedef struct NDSPreviewPackSpan {
    u32 source_offset; u32 data_offset; u32 data_bytes;
} NDSPreviewPackSpan;
typedef struct NDSPreviewResident {
    u32 generation; u32 model_source_bytes;
    NDSPreviewPackSection *sections; NDSPreviewPackSpan *spans;
} NDSPreviewResident;
typedef struct NDSRelocAssetHeader {
    u32 file_id; u32 data_size; u32 reloc_intern_offset; u32 reloc_extern_offset;
} NDSRelocAssetHeader;
typedef struct NDSRelocLoadedFile {
    void *data; u32 data_size;
    sb32 internal_fixups_applied; sb32 external_fixups_applied;
    u8 reserved[2];
} NDSRelocLoadedFile;

static NDSPreviewResident sNdsPreviewResidents[12];
static NDSRelocLoadedFile sLoadedFiles[8];
static u32 sLoadedFileCount;
static u32 sNdsRelocSceneGeneration = 3u;
static u32 gNdsPreviewPackStepCount;
static u32 gNdsPreviewPackStepByteMax;
static u32 sHaltReason;
static u32 sFsLockDepth;
static u32 sFsLockMaxRequest;   /* largest fread inside one lock span */

static void ndsFsLock(void) { sFsLockDepth++; }
static void ndsFsUnlock(void) { sFsLockDepth--; }

/* Redirect the extracted reader's I/O so the LARGEST SINGLE REQUEST -- which
 * is the span that would hold the filesystem mutex against a stream refill --
 * is measurable.  The file is real; only the call is instrumented. */
static u32 sReadMax;
static u32 sReadCalls;
static size_t harness_fread(void *dst, size_t size, size_t count, FILE *file)
{
    u32 bytes = (u32)(size * count);

    sReadCalls++;
    if (bytes > sReadMax) { sReadMax = bytes; }
    if ((sFsLockDepth != 0u) && (bytes > sFsLockMaxRequest))
    {
        sFsLockMaxRequest = bytes;
    }
    return fread(dst, size, count, file);
}
#define fread(d, s, n, f) harness_fread((d), (s), (n), (f))

static void ndsPreviewPackLoadHalt(u32 reason, u32 kind)
{
    sHaltReason = reason;
    printf("FAIL halt reason %u kind %u\n", reason, kind);
    exit(2);
}
static u32 ndsRelocReadBe32(const void *p)
{
    const u8 *b = p;
    return ((u32)b[0] << 24) | ((u32)b[1] << 16) | ((u32)b[2] << 8) | b[3];
}
static void ndsRelocWriteNative32(void *p, u32 v) { memcpy(p, &v, 4u); }
static void ndsRelocWriteNativePointer(void *p, void *v)
{
    /* A DS pointer slot is four bytes; a host pointer is eight, and writing
     * sizeof(v) here would clobber the NEXT word and make the decode check
     * below report damage the target never suffers. */
    u32 packed = (u32)(uptr)v;
    memcpy(p, &packed, 4u);
}
static NDSRelocLoadedFile *ndsRelocFindLoadedFileByAsset(u32 asset)
{
    (void)asset;
    return NULL;
}
static NDSRelocLoadedFile *ndsRelocRegisterLoadedFile(
    u32 asset, u32 flags, void *data, NDSRelocAssetHeader *header)
{
    NDSRelocLoadedFile *row = &sLoadedFiles[sLoadedFileCount++];
    (void)asset; (void)flags; (void)header;
    row->data = data;
    return row;
}
static sb32 ndsRelocNormalizeFighterAttributesFile(NDSRelocLoadedFile *f)
{
    (void)f;
    return TRUE;
}
static sb32 ndsRelocNormalizeBattleInterfaceSprites(NDSRelocLoadedFile *f)
{
    (void)f;
    return TRUE;
}
'''

PACK_MAIN = r'''
/* Build a synthetic FPC2 body: two sections, a big BE payload and a fixup
 * table, laid out exactly as the reader expects after the header. */
#define SEC0_BYTES 16384u
#define SEC1_BYTES (LINK_DATA_BYTES - SEC0_BYTES)
#define FIXUPS LINK_FIXUPS_PIN
#define SPANS 4u

static u8 sRaw[LINK_DATA_BYTES];
static NDSPreviewPackSection sSections[2];
static NDSPreviewPackFixup sFixups[FIXUPS];
static NDSPreviewPackSpan sSpanRows[SPANS];
static u8 sArena[LINK_DATA_BYTES + 4096u];

int main(void)
{
    NDSPreviewPackLoad *load;
    FILE *file;
    u32 i;
    u32 expect_data_hash;
    u32 expect_fixup_hash;
    u32 steps = 0u;
    s32 step;
    u32 moved;
    u32 total_moved = 0u;

    for (i = 0u; i < LINK_DATA_BYTES; i++) { sRaw[i] = (u8)(i * 7u + 3u); }
    sSections[0].asset_id = 0x101u;
    sSections[0].data_offset = 0u;
    sSections[0].data_bytes = SEC0_BYTES;
    sSections[0].source_bytes = SEC0_BYTES;
    sSections[0].span_count = 2u;
    sSections[1].asset_id = 0x102u;
    sSections[1].data_offset = SEC0_BYTES;
    sSections[1].data_bytes = SEC1_BYTES;
    sSections[1].source_bytes = SEC1_BYTES;
    sSections[1].first_span = 2u;
    sSections[1].span_count = 2u;
    for (i = 0u; i < FIXUPS; i++)
    {
        sFixups[i].slot_offset = (i * 64u) & ~3u;
        sFixups[i].target_offset = SEC0_BYTES + ((i * 32u) % SEC1_BYTES);
    }
    for (i = 0u; i < SPANS; i++)
    {
        u32 half = (i < 2u) ? 0u : 1u;
        sSpanRows[i].source_offset = (i & 1u) ? 8u : 0u;
        sSpanRows[i].data_offset = (i & 1u) ? 8u : 0u;
        sSpanRows[i].data_bytes = (half == 0u) ? 8u : 8u;
    }
    expect_data_hash = ndsPreviewHash(sRaw, LINK_DATA_BYTES, 2166136261u);
    expect_fixup_hash = 2166136261u;
    for (i = 0u; i < FIXUPS; i++)
    {
        expect_fixup_hash = ndsPreviewHash(&sFixups[i], sizeof(sFixups[i]),
                                           expect_fixup_hash);
    }

    file = tmpfile();
    CHECK(file != NULL, "tmpfile unavailable");
    if (file == NULL) { return 1; }
    (void)fwrite(sRaw, 1u, LINK_DATA_BYTES, file);
    (void)fwrite(sFixups, sizeof(sFixups[0]), FIXUPS, file);
    (void)fwrite(sSpanRows, sizeof(sSpanRows[0]), SPANS, file);
    rewind(file);

    load = &sNdsPreviewPackLoads[0];
    memset(load, 0, sizeof(*load));
    load->file = file;
    load->data = sArena;
    load->sections = (NDSPreviewPackSection *)(sArena + LINK_DATA_BYTES);
    memcpy(load->sections, sSections, sizeof(sSections));
    load->spans = (NDSPreviewPackSpan *)(load->sections + 2u);
    load->data_bytes = LINK_DATA_BYTES;
    load->fixup_count = FIXUPS;
    load->span_count = SPANS;
    load->data_hash = expect_data_hash;
    load->fixup_hash = expect_fixup_hash;
    load->span_hash = ndsPreviewHash(sSpanRows, sizeof(sSpanRows), 2166136261u);
    load->model_source_bytes = SEC1_BYTES;
    load->section_count = 2u;
    load->generation = sNdsRelocSceneGeneration;
    load->fkind = 5;
    load->hash = 2166136261u;
    load->state = NDS_FPC_STATE_READ;

    do
    {
        moved = 0u;
        step = ndsRelocPreviewFighterLoadStep(load, STEP_BYTES_PIN, &moved);
        steps++;
        total_moved += moved;
        CHECK(moved <= STEP_BYTES_PIN,
              "step %u moved %u bytes, over the %u-byte unit bound",
              steps, moved, (u32)STEP_BYTES_PIN);
        CHECK(steps < 4096u, "the reader never finished");
    } while ((step == NDS_PREVIEW_PACK_STEP_IN_PROGRESS) && (steps < 4096u));

    CHECK(step == NDS_PREVIEW_PACK_STEP_DONE,
          "the reader did not complete (%d)", (int)step);
    CHECK(sReadMax <= STEP_BYTES_PIN,
          "a single filesystem request was %u bytes, over the %u-byte bound",
          sReadMax, (u32)STEP_BYTES_PIN);
    CHECK(sFsLockMaxRequest <= STEP_BYTES_PIN,
          "the filesystem lock was held across a %u-byte request",
          sFsLockMaxRequest);
    CHECK(sFsLockDepth == 0u, "the reader leaked a filesystem lock");
    CHECK(load->state == NDS_FPC_STATE_STAGED,
          "the completed transaction is not staged (%u)", (u32)load->state);
    CHECK(load->file == NULL, "the completed transaction kept its file open");

    /* The chunked read must reproduce the whole payload byte-lane normalized,
     * which is the one-pass replacement for read-all / hash-all / swap-all. */
    for (i = 0u; i < LINK_DATA_BYTES; i += 4u)
    {
        u32 want = ndsRelocReadBe32(&sRaw[i]);
        u32 got;
        memcpy(&got, &sArena[i], 4u);
        if (want != got)
        {
            /* A fixup may legitimately have overwritten this word. */
            u32 j;
            int patched = 0;
            for (j = 0u; j < FIXUPS; j++)
            {
                if (sFixups[j].slot_offset == i) { patched = 1; break; }
            }
            CHECK(patched, "word at %u decoded wrong (%08x != %08x)",
                  i, got, want);
        }
    }

    printf(gFailures ? "READER FAIL\n" : "READER OK\n");
    return gFailures ? 1 : 0;
}
'''


def pack_sources(mutation=None):
    state_enum = braced(PACK, r"enum \{\s*\n\s*NDS_FPC_STATE_FREE", True)
    record = braced(PACK, r"typedef struct NDSPreviewPackLoad \{", True)
    step = function(PACK, "ndsRelocPreviewFighterLoadStep")
    if mutation is not None:
        anchor, patched = mutation
        if step.count(anchor) != 1:
            raise AssertionError(f"negative-control anchor not unique: {anchor!r}")
        step = step.replace(anchor, patched)
    return "\n".join([
        state_enum,
        record,
        "static NDSPreviewPackLoad sNdsPreviewPackLoads[5];",
        function(PACK, "ndsPreviewHash"),
        function(PACK, "ndsPreviewRange"),
        function(PACK, "ndsPreviewSectionContains"),
        function(PACK, "ndsPreviewPackLoadRelease"),
        step,
    ])


def build_pack(mutation=None):
    pins = (f"#define LINK_DATA_BYTES {LINK_DATA_BYTES}u\n"
            f"#define LINK_FIXUPS_PIN {LINK_FIXUPS}u\n"
            f"#define STEP_BYTES_PIN {STEP_BYTES}u\n"
            "#define NDS_PREVIEW_PACK_STEP_FAIL 0\n"
            "#define NDS_PREVIEW_PACK_STEP_IN_PROGRESS 1\n"
            "#define NDS_PREVIEW_PACK_STEP_DONE 2\n")
    return "\n".join([HEAD, pins, PACK_FIXTURE, pack_sources(mutation), PACK_MAIN])


def bounded(text, limit=2400):
    text = (text or "").strip()
    return text if len(text) <= limit else text[:limit] + "... [truncated]"


class CssPreviewTransactionTest(unittest.TestCase):
    MAX_LOG = 2400

    def run_host(self, source_text, tag, expect_success=True):
        compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc")
                         if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required (clang/gcc/cc)")
        version = subprocess.run([compiler, "--version"], capture_output=True)
        is_clang = b"clang" in version.stdout.lower()
        error_cap = ["-ferror-limit=8"] if is_clang else ["-fmax-errors=8"]
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / f"css_tx_{tag}.c"
            program = Path(directory) / f"css_tx_{tag}.exe"
            source.write_text(source_text, encoding="utf-8")
            built = subprocess.run(
                [compiler, "-std=c11", "-Wall", "-Wextra",
                 "-Wno-unused-function", "-Wno-unused-parameter",
                 *error_cap, str(source), "-o", str(program)],
                capture_output=True)
            self.assertEqual(
                built.returncode, 0,
                "host build failed:\n"
                f"{bounded(built.stderr.decode('utf-8', 'replace'), self.MAX_LOG)}")
            ran = subprocess.run([str(program)], capture_output=True, timeout=60)
            out = ran.stdout.decode("utf-8", "replace")
            if expect_success:
                self.assertEqual(
                    ran.returncode, 0,
                    f"host run failed:\n{bounded(out, self.MAX_LOG)}")
            return ran.returncode, out

    # -- 1. cancellation ---------------------------------------------------
    def test_cancel_respects_a_shared_in_flight_kind(self):
        self.run_host(build_vs(CANCEL_MAIN, with_compact=False), "cancel")

    def test_control_cancel_without_shared_demand_check(self):
        """MUTATION: drop RequestLoadCancel's other-slot early return.

        With `return;` turned into a no-op statement, one slot moving its
        cursor marks a closure two slots are waiting on, and the fixture's
        first assertion must fire.  If this control PASSES, the shared-kind
        guard is no longer load-bearing -- fix the patch, not the source.
        """
        code, out = self.run_host(
            build_vs(CANCEL_MAIN, with_compact=False,
                     cancel_mutation=(
                         "(sNdsPlayersVSPreviewPending[i].fkind == fkind))\n"
                         "        {\n            return;",
                         "(sNdsPlayersVSPreviewPending[i].fkind == fkind))\n"
                         "        {\n            (void)0;")),
            "cancel_ctl", expect_success=False)
        self.assertNotEqual(code, 0, f"negative control passed:\n{bounded(out)}")

    # -- 2. budget ---------------------------------------------------------
    def test_service_budget_bounds_every_unit(self):
        self.run_host(build_vs(BUDGET_MAIN), "budget")

    def test_control_unbounded_unit_breaks_the_budget(self):
        """MUTATION: hand the loader the whole pack instead of one unit.

        `budget = NDS_PLAYERS_VS_LOAD_STEP_BYTES;` becomes a 64 KiB span, i.e.
        a time check after one huge blocking read.  The unit and aggregate
        assertions must both fire.
        """
        code, out = self.run_host(
            build_vs(BUDGET_MAIN,
                     mutation=("        budget = NDS_PLAYERS_VS_LOAD_STEP_BYTES;",
                               "        budget = 64u * 1024u;")),
            "budget_ctl", expect_success=False)
        self.assertNotEqual(code, 0, f"negative control passed:\n{bounded(out)}")

    # -- 3. staleness ------------------------------------------------------
    def test_stale_completion_is_rejected(self):
        self.run_host(build_vs(STALE_MAIN), "stale")

    def test_control_without_generation_guard(self):
        """MUTATION: remove the resource-generation comparison.

        `block->load_generation != gNdsTaskmanHeapGeneration` becomes a
        constant FALSE, so a transaction opened before a scene rewind keeps
        running and can publish into the new generation's arena.
        """
        code, out = self.run_host(
            build_vs(STALE_MAIN,
                     mutation=("    if (block->load_generation != gNdsTaskmanHeapGeneration)",
                               "    if (0)")),
            "stale_ctl", expect_success=False)
        self.assertNotEqual(code, 0, f"negative control passed:\n{bounded(out)}")

    # -- 4. the reader itself ----------------------------------------------
    def test_reader_chunks_every_span(self):
        self.run_host(build_pack(), "reader")

    def test_control_reader_reads_the_whole_payload_at_once(self):
        """MUTATION: restore the single unbounded payload read.

        The READ clamp becomes `chunk = load->data_bytes - load->cursor;`, i.e.
        the shipped one-call behaviour, and the per-request and per-step bounds
        must both fire.
        """
        code, out = self.run_host(
            build_pack(mutation=(
                "            chunk = byte_budget & ~3u;",
                "            chunk = load->data_bytes - load->cursor;")),
            "reader_ctl", expect_success=False)
        self.assertNotEqual(code, 0, f"negative control passed:\n{bounded(out)}")


if __name__ == "__main__":
    unittest.main()
