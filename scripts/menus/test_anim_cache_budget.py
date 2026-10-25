"""Host-execute the REAL anim-cache reservation functions from reloc_backend_assets.c.

Main's measured bug (fixed on this branch): the standalone 258,048-byte
reservation ignored the fighters that still had to load.  On the capture heap
the cache took 258,048 bytes and Fox's 116,752-byte tree then had only 75,788
left -- an OOM halt.  `ndsAnimCacheMatchFighterBytes` now counts the unique
unloaded / stale-generation main trees via the same provider the pending load
will call (`lbRelocGetFileSize`, not the N64 census), and
`ndsAnimCacheArenaEnsure` reserves only the largest aligned partial raw
block that fits after those bytes plus the live ledger floor (128 KiB)
whenever the full pack cannot fit.

Measured follow-up (2026-09-06 trace): Results reserved 258048 before the
mandatory 153600 sprite surface and broke the floor. The live fix returns a
generation-owned arena first (the CSS explicit setup cache) and otherwise
requires the setup stamp in ALL scenes, not only battles. This file proves
that gate: unprepared nonbattle cannot carve, an owned arena is reusable
without a fresh stamp, battle still waits for its stamp, and a stale
generation cannot reuse. Proposal: none; all numbers above are measured or
live-pinned.

Since then the pack-declined path carves the ELASTIC block instead
(ndsAnimCacheArenaCarveElastic): everything above the pending trees and a
12 KB floor, at the heap's top, which any allocation that reaches it takes
back first (battleship_sys_malloc.c) -- no syTaskmanMalloc, so the Fox OOM
shape cannot recur there. The reserve arithmetic above governs the pack-kept
path and its drop to a standalone carve, and a decline latches for its heap
generation. The Mario-only scenarios below are the elastic carve's.

These tests EXTRACT the production bodies verbatim (source_test_helpers) and
run them on a host harness with real `ndsSyMallocWouldFit`:

  * extracted, unmodified: ndsAnimCacheMatchFighterBytes,
    ndsAnimCacheArenaEnsure, ndsAnimCacheArenaStillOwned,
    ndsAnimCacheArenaDropForReset, ndsBattlePackCarveWorthIt,
    ndsAnimCacheArenaReserveDecline, ndsAnimCacheArenaCarveElastic (all
    from src/port/reloc_backend_assets.c and its slices),
    ndsSyMallocWouldFit (from src/import/battleship_sys_malloc.c).
  * stubbed: lbRelocGetFileSize (the source file provider -- explicit bytes
    per file id, exactly the seam the task allows), the loaded-file registry
    lookup, ndsBattlePackResidencyDrop, and syTaskmanMalloc (a bump carve
    with the decomp syMallocSet cursor semantics over the same SYMallocRegion
    layout).  No algorithm under test is re-implemented; the only oracle
    arithmetic lives in this file's Python expectations.

The negative control compiles the same extracted body with ONE patch --
`fighter_bytes = 0u;` right after the helper call, i.e. the old "reservation
ignores remaining fighters" behaviour -- and proves the old-Fox-OOM heap input
then reserves the full cache and leaves Fox his measured 75,788 < 116,752.

LIMITATIONS (deliberate, so nobody reads these tests as "the whole game
fits"): fighter_bytes models ONLY each distinct kind's main figatree via the
provider.  Mandatory future general-heap costs that are NOT modelled here
include native owner images, figatree-subnode working storage, the CSS
DemoNull clips, and every non-fighter battle allocation.  A green run says
the reservation arithmetic respects trees + floor; it does not say the match
loads.

Fixture divergence, stated: production FTData.file_main_id is u32 while
lbRelocGetFileSize takes const void*; the host fixture types file_main_id as
const void* keyed on sentinel pointers so the extracted call compiles clean.
"""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from source_test_helpers import function, source as read_split

ROOT = Path(__file__).resolve().parents[2]
ASSETS = read_split(ROOT / "src/port/reloc_backend_assets.c")
MALLOC = (ROOT / "src/import/battleship_sys_malloc.c").read_text(encoding="utf-8")

# Pinned production constants.  Each is parsed back out of the source so the
# test fails loudly if the reservation constants drift.
ALIGN = 0x10
STANDALONE = 258048
PACK_RAW = 163840
# Live KEEP_FREE aliases the ledger runtime reserve (128 KiB), not the old
# 32 KiB setup floor. The Results trace (2026-09-06) broke the 32 KiB floor by
# reserving 258048 before the mandatory 153600 sprite surface; the host floor
# here tracks the live reserve so partial math proves the current guarantee.
KEEP_FREE = 128 * 1024
BLOB = 287904  # generated: `wc -c battlepack_fox.bin` via nds_build_config.h
LINE = 32
RESERVE = (BLOB + 2 * LINE - 1) & ~(LINE - 1)
assert RESERVE == 287936
assert re.search(r"287,936-byte request", ASSETS)  # measured figure in source


def pin_int(name, expected, which=0):
    hits = [int(m) for m in re.findall(rf"#define\s+{name}\s+(\d+)u", ASSETS)]
    if not hits or hits[which] != expected:
        raise AssertionError(f"{name} drifted from {expected}: {hits}")
    return expected


pin_int("NDS_ANIM_CACHE_STANDALONE_RAW_BYTES", STANDALONE)
pin_int("NDS_ANIM_CACHE_PACK_RAW_BYTES", PACK_RAW)  # KEEP_CACHE arm is first
# KEEP_FREE is an alias, not a literal: pin the ledger reserve and the alias.
if not re.search(
        r"#define\s+NDS_RELOC_MEMORY_LEDGER_RESERVE_BYTES\s+\(128u\s*\*\s*1024u\)",
        ASSETS):
    raise AssertionError("ledger reserve drifted from (128u * 1024u)")
if not re.search(
        r"#define\s+NDS_ANIM_CACHE_ARENA_KEEP_FREE\s+NDS_RELOC_MEMORY_LEDGER_RESERVE_BYTES",
        ASSETS):
    raise AssertionError("ARENA_KEEP_FREE drifted from ledger-reserve alias")
pin_int("NDS_BATTLEPACK_LINE_BYTES", LINE)
ELASTIC_FLOOR = pin_int("NDS_ANIM_CACHE_ELASTIC_FLOOR", 12288)
ELASTIC_MIN = pin_int("NDS_ANIM_CACHE_ELASTIC_MIN", 4096)
ELASTIC_MAX = 448 * 1024
if not re.search(r"#define\s+NDS_ANIM_CACHE_ELASTIC_MAX_BYTES\s+\(448u\s*\*\s*1024u\)",
                 ASSETS):
    raise AssertionError("NDS_ANIM_CACHE_ELASTIC_MAX_BYTES drifted from (448u * 1024u)")
if not re.search(r"#define\s+NDS_RELOC_ALIGN_BYTES\s+0x10u", ASSETS):
    raise AssertionError("NDS_RELOC_ALIGN_BYTES drifted from 0x10u")

# Explicit provider bytes (fixture, not census).
MARIO_TREE = 99856
FOX_TREE = 116752
OLD_AVAIL = STANDALONE + 75788  # 333836: the measured OOM heap input
OLD_LEFT = OLD_AVAIL - STANDALONE  # 75788: what Fox had after the old reserve
assert OLD_LEFT < FOX_TREE  # the measured OOM
# Control heap sized for the live floor: full standalone + floor + the same
# 75788 left for Fox, so the zeroed-budget control still reserves full and
# still starves him.
CONTROL_AVAIL = STANDALONE + KEEP_FREE + OLD_LEFT
FIX_AVAIL = (OLD_AVAIL - FOX_TREE - KEEP_FREE) & ~(ALIGN - 1)
PART190_AVAIL = MARIO_TREE + FOX_TREE + KEEP_FREE + 194560  # ~190 KB case


def elastic_bytes(slack):
    """The elastic block for `slack` bytes above the pending trees: 0 is a
    decline (ndsAnimCacheArenaCarveElastic)."""
    if slack <= ELASTIC_FLOOR + ELASTIC_MIN + 32:
        return 0
    return min(ELASTIC_MAX, slack - ELASTIC_FLOOR - 32) & ~31


ELASTIC_FULL = elastic_bytes(KEEP_FREE + STANDALONE)
ELASTIC_SLACKS = (ELASTIC_FLOOR + ELASTIC_MIN + 32, ELASTIC_FLOOR + ELASTIC_MIN + 33,
                  ELASTIC_FLOOR + ELASTIC_MIN + 63, ELASTIC_FLOOR + ELASTIC_MIN + 64,
                  ELASTIC_MAX + ELASTIC_FLOOR + 32, ELASTIC_MAX + ELASTIC_FLOOR + 32 + 4096)


def extract_real():
    """Pull the verbatim production bodies this test executes."""
    out = {
        "ensure": function(ASSETS, "ndsAnimCacheArenaEnsure"),
        "fighter_bytes": function(ASSETS, "ndsAnimCacheMatchFighterBytes"),
        "still_owned": function(ASSETS, "ndsAnimCacheArenaStillOwned"),
        "drop_for_reset": function(ASSETS, "ndsAnimCacheArenaDropForReset"),
        "carve_worth_it": function(ASSETS, "ndsBattlePackCarveWorthIt"),
        "decline": function(ASSETS, "ndsAnimCacheArenaReserveDecline"),
        "carve_elastic": function(ASSETS, "ndsAnimCacheArenaCarveElastic"),
        "would_fit": function(MALLOC, "ndsSyMallocWouldFit"),
    }
    for needle in ("ndsSyMallocWouldFit", "fighter_bytes",
                   "NDS_ANIM_CACHE_ARENA_KEEP_FREE",
                   "ndsAnimCacheArenaStillOwned",
                   "sNdsAnimCacheSetupGeneration",
                   "ndsAnimCacheArenaCarveElastic"):
        if needle not in out["ensure"]:
            raise AssertionError(f"extracted Ensure lost '{needle}'")
    # Owned-arena-first: the StillOwned early return must precede the setup
    # gate, and the gate must apply in all scenes (no battle-only carve).
    if out["ensure"].index("ndsAnimCacheArenaStillOwned") > \
            out["ensure"].index("sNdsAnimCacheSetupGeneration"):
        raise AssertionError("Ensure lost owned-first order")
    if "gNdsSceneManagerCurrIsBattle" in out["ensure"]:
        raise AssertionError("Ensure regained battle-only gate")
    return out


HARNESS_HEAD = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
typedef s32 sb32;
enum { FALSE = 0, TRUE = 1 };

#define GMCOMMON_PLAYERS_MAX 4
enum { nFTKindMario, nFTKindFox, nFTKindLuigi, nFTKindNess,
       nFTKindPlayableEnd = nFTKindNess, nFTKindEnumCount };
enum { nFTPlayerKindNot, nFTPlayerKindMan, nFTPlayerKindCom };

#define NDS_RELOC_ALIGN_BYTES 0x10u
#define NDS_BATTLEPACK_LINE_BYTES 32u
#define NDS_BATTLEPACK_BLOB_BYTES 287904u
#define NDS_BATTLEPACK_RESERVE_BYTES \
    ((NDS_BATTLEPACK_BLOB_BYTES + (2u * NDS_BATTLEPACK_LINE_BYTES) - 1u) & \
     ~(NDS_BATTLEPACK_LINE_BYTES - 1u))
#define NDS_BATTLEPACK 1
#define NDS_ANIM_CACHE_PACK_RAW_BYTES 163840u
#define NDS_ANIM_CACHE_STANDALONE_RAW_BYTES 258048u
#define NDS_ANIM_CACHE_ARENA_KEEP_FREE 131072u
#define NDS_BATTLEPACK_FIGHTER_KIND nFTKindFox
#define NDS_BATTLEPACK_CARVE_MAX_KINDS 2u

typedef struct SYMallocRegion { u32 id; void *start; void *end; void *ptr; }
    SYMallocRegion;

/* Fixture battle state: only fkind/pkind are read by the extracted bodies. */
typedef struct { s32 fkind; s32 pkind; } TestPlayer;
typedef struct { TestPlayer players[GMCOMMON_PLAYERS_MAX]; } TestBattleState;

/* Fixture FTData: file_main_id typed as the provider's parameter so the
 * verbatim extracted call compiles clean (production passes the u32 id). */
typedef struct FTData { const void *file_main_id; void **p_file_main; } FTData;

typedef struct NDSRelocLoadedFile { u32 owner_generation; } NDSRelocLoadedFile;

static u32 gNdsTaskmanHeapGeneration;
static u32 sNdsRelocSceneGeneration;
static SYMallocRegion gSYTaskmanGeneralHeap;
static u8 sHeapBuf[1 << 20];

static u8 *sNdsAnimCacheArena;
static u32 sNdsAnimCacheArenaBytes;
static u32 sNdsAnimCacheArenaUsed;
static sb32 sNdsAnimCacheArenaRawOnly;
static u32 sNdsAnimCacheArenaGeneration;
static u32 sNdsAnimCacheSetupGeneration;
static u32 gNdsSceneManagerCurrIsBattle;
static u32 sNdsAnimCacheCount;
/* The elastic arena (above the cursor, taken back by allocations), the clip
 * index, the prefetch queue and the pins, as the reset drops them. */
static sb32 sNdsAnimCacheArenaElastic;
static uint16_t sNdsAnimCacheIndex[512];
static u32 sPrefetchDrainCalls, sPinsRescueCalls;
static void ndsAnimPrefetchDrain(void) { sPrefetchDrainCalls++; }
static void ndsAnimPinsRescueRange(const u8 *start, const u8 *end)
{
    (void)start; (void)end; sPinsRescueCalls++;
}
/* The decline latch, the extent origin and the elastic carve's registration
 * with the taskman allocator (reloc_backend_motion_cache.c). */
#define NDS_ANIM_CACHE_ELASTIC_FLOOR 12288u
#define NDS_ANIM_CACHE_ELASTIC_MIN 4096u
#define NDS_ANIM_CACHE_ELASTIC_MAX_BYTES (448u * 1024u)
#define NDS_DIAG(x) (x)
static sb32 sNdsAnimCacheReserveFailLatched;
static u32 sNdsAnimCacheReserveFailGeneration;
static uintptr_t sNdsAnimCacheExtentOrigin;
static volatile u32 gNdsAnimCacheArenaReserveFailSkips;
static volatile u32 gNdsAnimCacheElasticCarves;
static void *sElasticBase;
static u32 sElasticRegisters;
static u8 *ndsAnimCacheElasticYield(u8 *need_top) { return need_top; }
static void ndsTaskmanElasticRegister(void *base, u8 *(*yield)(u8 *need_top))
{
    (void)yield; sElasticBase = base; sElasticRegisters++;
}
/* No content plays here (nds_p4.c). */
static u32 ndsP4MatchContent(s32 player) { (void)player; return 0u; }

static volatile u32 gNdsAnimCacheBytes;
static volatile u32 gNdsAnimCacheMatchFighterBytes;
static volatile u32 gNdsAnimCachePackDroppedForFightersCount;
static volatile u32 gNdsBattlePackCarveDeclineCount;
static volatile u32 gNdsBattlePackCarveMatchKinds;
static volatile u32 gNdsAnimCacheArenaReserveFailCount;
static volatile u32 gNdsAnimCacheArenaReserveCount;
static volatile u32 gNdsAnimCacheArenaReservedBytes;
static volatile u32 gNdsAnimCacheArenaUsedBytes;
static volatile u32 gNdsAnimCacheArenaInvalidations;
static volatile u32 gNdsAnimCacheArenaGenerationMismatches;
static volatile u32 gNdsAnimCacheArenaRangeFaults;
static volatile u32 gNdsRelocFileSizeFallbackCount;

/* ---- stubs: only the source-file provider and registry state ---- */

typedef struct {
    const void *id;
    size_t bytes;      /* explicit fixture bytes */
    int fire_fallback; /* provider gives up: bumps the fallback counter */
    size_t calls;
} ProviderEntry;
static ProviderEntry sProvider[8];
static u32 sProviderCount;

static size_t lbRelocGetFileSize(const void *file_id)
{
    u32 i;
    for (i = 0u; i < sProviderCount; i++)
    {
        if (sProvider[i].id == file_id)
        {
            sProvider[i].calls++;
            if (sProvider[i].fire_fallback)
            {
                gNdsRelocFileSizeFallbackCount++;
                return 24u;
            }
            return sProvider[i].bytes;
        }
    }
    gNdsRelocFileSizeFallbackCount++; /* id with no metadata at all */
    return 24u;
}

typedef struct { const void *lo; const void *hi; u32 owner_generation;
                 NDSRelocLoadedFile file; } LoadedEntry;
static LoadedEntry sLoaded[8];
static u32 sLoadedCount;

static NDSRelocLoadedFile *ndsRelocFindLoadedFileContaining(const void *ptr,
                                                            size_t size)
{
    u32 i;
    for (i = 0u; i < sLoadedCount; i++)
    {
        if ((ptr >= sLoaded[i].lo) &&
            (((const u8 *)ptr + size) <= (const u8 *)sLoaded[i].hi))
        {
            return &sLoaded[i].file;
        }
    }
    return NULL;
}

static u32 sResidencyDropCalls;
static void ndsBattlePackResidencyDrop(void) { sResidencyDropCalls++; }

/* Host carve with the decomp syMallocSet cursor semantics (align up, add
 * size exactly).  The fit decision under test is the REAL ndsSyMallocWouldFit. */
static u32 sMallocCalls;
static size_t sMallocLastSize;
static u32 sMallocLastAlign;
static void *syTaskmanMalloc(size_t size, u32 align)
{
    uintptr_t base = (uintptr_t)gSYTaskmanGeneralHeap.ptr;
    uintptr_t aligned = (align != 0u) ? (base + align - 1u) & ~((uintptr_t)align - 1u)
                                      : base;
    void *block;
    if ((aligned + size) > (uintptr_t)gSYTaskmanGeneralHeap.end)
    {
        return NULL; /* unreachable when the pre-check is honoured */
    }
    block = (void *)aligned;
    gSYTaskmanGeneralHeap.ptr = (u8 *)aligned + size;
    sMallocCalls++;
    sMallocLastSize = size;
    sMallocLastAlign = align;
    return block;
}

/* ---- fixture state ---- */

static TestBattleState sBattle;
static TestBattleState *gSCManagerBattleState;
static FTData sMarioData, sFoxData, sLuigiData;
static FTData *dFTManagerDataFiles[nFTKindEnumCount];
static const u8 sIdMario[4], sIdFox[4], sIdLuigi[4];
static void *sMarioMain, *sFoxMain;
static u8 sTreeBuf[64];

static void reset_state(void)
{
    u32 i;
    sNdsAnimCacheSetupGeneration = 0u;
    gNdsSceneManagerCurrIsBattle = 0u;
    memset(&sBattle, 0, sizeof(sBattle));
    gSCManagerBattleState = &sBattle;
    for (i = 0u; i < (u32)nFTKindEnumCount; i++)
    {
        dFTManagerDataFiles[i] = NULL;
    }
    sMarioData.file_main_id = sIdMario;
    sMarioData.p_file_main = &sMarioMain;
    sFoxData.file_main_id = sIdFox;
    sFoxData.p_file_main = &sFoxMain;
    sLuigiData.file_main_id = sIdLuigi;
    sLuigiData.p_file_main = NULL;
    dFTManagerDataFiles[nFTKindMario] = &sMarioData;
    dFTManagerDataFiles[nFTKindFox] = &sFoxData;
    dFTManagerDataFiles[nFTKindLuigi] = &sLuigiData;
    sMarioMain = sFoxMain = NULL;
    memset(&gSYTaskmanGeneralHeap, 0, sizeof(gSYTaskmanGeneralHeap));
    gSYTaskmanGeneralHeap.start = sHeapBuf + 64;
    gSYTaskmanGeneralHeap.ptr = sHeapBuf + 64;
    gNdsTaskmanHeapGeneration = 7u;
    /* Live residency keys on the scene generation, not the heap generation.
     * Keep the fixture synced so current-generation resident means resident. */
    sNdsRelocSceneGeneration = gNdsTaskmanHeapGeneration;
    sNdsAnimCacheArena = NULL;
    sNdsAnimCacheArenaBytes = 0u;
    sNdsAnimCacheArenaUsed = 0u;
    sNdsAnimCacheArenaRawOnly = FALSE;
    sNdsAnimCacheArenaGeneration = 0u;
    sNdsAnimCacheCount = 0u;
    gNdsAnimCacheBytes = 0u;
    gNdsAnimCacheMatchFighterBytes = 0u;
    gNdsAnimCachePackDroppedForFightersCount = 0u;
    gNdsBattlePackCarveDeclineCount = 0u;
    gNdsBattlePackCarveMatchKinds = 0u;
    gNdsAnimCacheArenaReserveFailCount = 0u;
    gNdsAnimCacheArenaReserveCount = 0u;
    gNdsAnimCacheArenaReservedBytes = 0u;
    gNdsAnimCacheArenaUsedBytes = 0u;
    gNdsAnimCacheArenaInvalidations = 0u;
    gNdsAnimCacheArenaGenerationMismatches = 0u;
    gNdsAnimCacheArenaRangeFaults = 0u;
    gNdsRelocFileSizeFallbackCount = 0u;
    sNdsAnimCacheArenaElastic = FALSE;
    sNdsAnimCacheReserveFailLatched = FALSE;
    sNdsAnimCacheReserveFailGeneration = 0u;
    sNdsAnimCacheExtentOrigin = 0u;
    gNdsAnimCacheArenaReserveFailSkips = 0u;
    gNdsAnimCacheElasticCarves = 0u;
    sElasticBase = NULL;
    sElasticRegisters = 0u;
    memset(sProvider, 0, sizeof(sProvider));
    sProviderCount = 0u;
    memset(sLoaded, 0, sizeof(sLoaded));
    sLoadedCount = 0u;
    sResidencyDropCalls = 0u;
    sMallocCalls = 0u;
    sMallocLastSize = 0u;
    sMallocLastAlign = 0u;
}

/* Point the heap cursor/end so that aligned-cursor-to-end == avail. */
static void set_heap(u32 avail, uintptr_t ptr_offset)
{
    uintptr_t start = (uintptr_t)sHeapBuf + 64;
    uintptr_t ptr = start + ptr_offset;
    uintptr_t aligned = (ptr + NDS_RELOC_ALIGN_BYTES - 1u) &
                        ~(uintptr_t)(NDS_RELOC_ALIGN_BYTES - 1u);
    gSYTaskmanGeneralHeap.ptr = (void *)ptr;
    gSYTaskmanGeneralHeap.end = (void *)(aligned + avail);
}

static void provider_add(const void *id, size_t bytes, int fire_fallback)
{
    sProvider[sProviderCount].id = id;
    sProvider[sProviderCount].bytes = bytes;
    sProvider[sProviderCount].fire_fallback = fire_fallback;
    sProviderCount++;
}

static void roster_mario_fox(void)
{
    sBattle.players[0].fkind = nFTKindMario;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    sBattle.players[1].fkind = nFTKindFox;
    sBattle.players[1].pkind = nFTPlayerKindCom;
}

#define CHECK(cond) do { if (!(cond)) { \
    fprintf(stderr, "CHECK failed line %d: %s\n", __LINE__, #cond); \
    exit(1); } } while (0)
'''


def host_main():
    """Scenario main for the real extracted bodies (all numbers from Python)."""
    return f"""
static void scenario_match_fighter_bytes(void)
{{
    u32 total;

    reset_state();
    gSCManagerBattleState = NULL;
    CHECK(ndsAnimCacheMatchFighterBytes() == 0u);

    /* Duplicate kind counts once; Not slots and out-of-range kinds skipped. */
    reset_state();
    roster_mario_fox();
    sBattle.players[2].fkind = nFTKindFox;
    sBattle.players[2].pkind = nFTPlayerKindCom;
    sBattle.players[3].fkind = nFTKindEnumCount;
    sBattle.players[3].pkind = nFTPlayerKindCom;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    total = ndsAnimCacheMatchFighterBytes();
    CHECK(total == {MARIO_TREE + FOX_TREE}u);
    CHECK(sProvider[0].calls == 1u && sProvider[1].calls == 1u);

    /* Current-generation resident main tree: skipped BEFORE the provider. */
    reset_state();
    roster_mario_fox();
    sFoxMain = sTreeBuf + 8;
    sLoaded[0].lo = sTreeBuf;
    sLoaded[0].hi = sTreeBuf + sizeof(sTreeBuf);
    sLoaded[0].owner_generation = sNdsRelocSceneGeneration;
    sLoaded[0].file.owner_generation = sNdsRelocSceneGeneration;
    sLoadedCount = 1u;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    total = ndsAnimCacheMatchFighterBytes();
    CHECK(total == {MARIO_TREE}u);
    CHECK(sProvider[1].calls == 0u);

    /* Stale generation: same bytes are resident but dead -- counted. */
    sLoaded[0].owner_generation = sNdsRelocSceneGeneration - 1u;
    sLoaded[0].file.owner_generation = sNdsRelocSceneGeneration - 1u;
    total = ndsAnimCacheMatchFighterBytes();
    CHECK(total == {MARIO_TREE + FOX_TREE}u);
    CHECK(sProvider[1].calls == 1u);

    /* p_file_main present but slot still NULL: tree is pending, counted. */
    reset_state();
    roster_mario_fox();
    sFoxData.p_file_main = &sFoxMain; /* slot stays NULL */
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    CHECK(ndsAnimCacheMatchFighterBytes() == {MARIO_TREE + FOX_TREE}u);

    /* Missing FTData: fail closed with UINT32_MAX. */
    reset_state();
    roster_mario_fox();
    dFTManagerDataFiles[nFTKindFox] = NULL;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    CHECK(ndsAnimCacheMatchFighterBytes() == 0xFFFFFFFFu);

    /* Provider fires the fallback witness: fail closed. */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 1);
    CHECK(ndsAnimCacheMatchFighterBytes() == 0xFFFFFFFFu);
    CHECK(gNdsRelocFileSizeFallbackCount == 1u);

    /* id with no metadata row at all: fallback, fail closed. */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    CHECK(ndsAnimCacheMatchFighterBytes() == 0xFFFFFFFFu);

    /* Sum overflow guard: a bogus provider size refuses rather than wraps. */
    reset_state();
    sBattle.players[0].fkind = nFTKindLuigi;
    sBattle.players[0].pkind = nFTPlayerKindCom;
    provider_add(sIdLuigi, (size_t)0x0000FFFFFFFFFFFFull, 0);
    CHECK(ndsAnimCacheMatchFighterBytes() == 0xFFFFFFFFu);
}}

static void scenario_pack_kept_and_standalone_full(void)
{{
    /* Mario-only roster: the pack is declined (no Fox), and the cache is the
     * ELASTIC block -- everything above the pending tree and its floor, at
     * the heap's top, no allocation made. */
    reset_state();
    sBattle.players[0].fkind = nFTKindMario;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    set_heap({MARIO_TREE + KEEP_FREE + STANDALONE}u, 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsBattlePackCarveDeclineCount == 1u);
    CHECK(gNdsBattlePackCarveMatchKinds == 1u);
    CHECK(sMallocCalls == 0u);
    CHECK(sNdsAnimCacheArenaElastic == TRUE);
    CHECK(sNdsAnimCacheArenaBytes == {ELASTIC_FULL}u);
    CHECK(sNdsAnimCacheArenaRawOnly == TRUE);
    CHECK(sNdsAnimCacheArenaUsed == 0u);
    CHECK(sNdsAnimCacheArenaGeneration == gNdsTaskmanHeapGeneration);
    CHECK(gNdsAnimCacheArenaReservedBytes == {ELASTIC_FULL}u);
    CHECK(gNdsAnimCacheMatchFighterBytes == {MARIO_TREE}u);
    CHECK(sElasticRegisters == 1u && sElasticBase == (void *)sNdsAnimCacheArena);
    CHECK(((uintptr_t)sNdsAnimCacheArena & 31u) == 0u);
    CHECK((uintptr_t)sNdsAnimCacheArena + sNdsAnimCacheArenaBytes <=
          (uintptr_t)gSYTaskmanGeneralHeap.end);
    /* The pending tree fits below the block, and its floor is kept. */
    CHECK(syTaskmanMalloc({MARIO_TREE}u, NDS_RELOC_ALIGN_BYTES) != NULL);
    CHECK((uintptr_t)sNdsAnimCacheArena -
          (uintptr_t)gSYTaskmanGeneralHeap.ptr >= {ELASTIC_FLOOR}u);

    /* Fox-only roster with room for pack + tree + floor: pack kept, and the
     * pack's region is carved FIRST (used == reserve at reservation time). */
    reset_state();
    sBattle.players[0].fkind = nFTKindFox;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({RESERVE + PACK_RAW + FOX_TREE + KEEP_FREE}u, 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsBattlePackCarveDeclineCount == 0u);
    CHECK(gNdsAnimCachePackDroppedForFightersCount == 0u);
    CHECK(sNdsAnimCacheArenaBytes == {RESERVE + PACK_RAW}u);
    CHECK(sNdsAnimCacheArenaRawOnly == FALSE);
    CHECK(sNdsAnimCacheArenaUsed == {RESERVE}u);
    CHECK(sMallocCalls == 1u && sMallocLastSize == {RESERVE + PACK_RAW}u);
    CHECK(gNdsAnimCacheArenaUsedBytes == {RESERVE}u);
}}

static void scenario_cached_lease_and_reset(void)
{{
    reset_state();
    sBattle.players[0].fkind = nFTKindMario;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    set_heap({MARIO_TREE + KEEP_FREE + STANDALONE}u, 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsAnimCacheArenaReserveCount == 1u);

    /* Cached lease: the block still owns its range above the cursor. */
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(sMallocCalls == 0u && sElasticRegisters == 1u);
    CHECK(gNdsAnimCacheArenaReserveCount == 1u);

    /* Heap rewound under us (generation bump): without a fresh setup stamp
     * the stale block cannot be reused, then the new setup carves fresh --
     * including dropping its entries whole. */
    sNdsAnimCacheCount = 3u;
    gNdsTaskmanHeapGeneration++;
    gSYTaskmanGeneralHeap.ptr = gSYTaskmanGeneralHeap.start;
    CHECK(ndsAnimCacheArenaEnsure() == FALSE);
    CHECK(gNdsAnimCacheArenaGenerationMismatches == 1u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsAnimCacheArenaInvalidations == 1u);
    CHECK(sResidencyDropCalls == 1u);
    CHECK(sNdsAnimCacheCount == 0u);
    CHECK(gNdsAnimCacheBytes == 0u);
    CHECK(sMallocCalls == 0u && sElasticRegisters == 2u);
    CHECK(gNdsAnimCacheArenaReserveCount == 2u);

    /* Same generation, but an allocation reached into the block without
     * taking it back (the cursor above its base): range fault, same reset. */
    reset_state();
    sBattle.players[0].fkind = nFTKindMario;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    set_heap({MARIO_TREE + KEEP_FREE + STANDALONE}u, 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    gSYTaskmanGeneralHeap.ptr = sNdsAnimCacheArena + 16; /* no gen bump */
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsAnimCacheArenaRangeFaults == 1u);
    CHECK(gNdsAnimCacheArenaInvalidations == 1u);
    CHECK(gNdsAnimCacheArenaReserveCount == 2u);
    CHECK((uintptr_t)sNdsAnimCacheArena >= (uintptr_t)gSYTaskmanGeneralHeap.ptr);
    CHECK(sMallocCalls == 0u);
}}

static void scenario_pack_drop_then_partial(void)
{{
    /* Pack is worth it but cannot fit beside the trees: pack half dropped,
     * standalone cache kept whole when it fits after trees + floor. */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({MARIO_TREE + FOX_TREE + KEEP_FREE + STANDALONE}u, 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsAnimCachePackDroppedForFightersCount == 1u);
    CHECK(sNdsAnimCacheArenaBytes == {STANDALONE}u);
    CHECK(sNdsAnimCacheArenaRawOnly == TRUE);
    CHECK(sNdsAnimCacheArenaUsed == 0u);
    CHECK(gNdsAnimCacheMatchFighterBytes == {MARIO_TREE + FOX_TREE}u);

    /* THE MEASURED BUG INPUT (2026-09-03 capture shape): old code reserved
     * {STANDALONE} first and left Fox {OLD_LEFT} of his {FOX_TREE}.  The fix
     * shrinks the cache to the largest aligned partial after Fox + floor. */
    reset_state();
    roster_mario_fox();
    sMarioMain = sTreeBuf + 8; /* Mario resident, current generation */
    sLoaded[0].lo = sTreeBuf;
    sLoaded[0].hi = sTreeBuf + sizeof(sTreeBuf);
    sLoaded[0].owner_generation = sNdsRelocSceneGeneration;
    sLoaded[0].file.owner_generation = sNdsRelocSceneGeneration;
    sLoadedCount = 1u;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({OLD_AVAIL}u, 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsAnimCacheMatchFighterBytes == {FOX_TREE}u);
    CHECK(gNdsAnimCachePackDroppedForFightersCount == 1u);
    CHECK(sNdsAnimCacheArenaBytes == {FIX_AVAIL}u);
    CHECK(gNdsAnimCacheArenaReservedBytes == {FIX_AVAIL}u);
    CHECK(sNdsAnimCacheArenaRawOnly == TRUE);
    /* Fox's tree now fits beside the shrunken cache AND the floor survives. */
    CHECK(syTaskmanMalloc({FOX_TREE}u, NDS_RELOC_ALIGN_BYTES) != NULL);
    CHECK((uintptr_t)gSYTaskmanGeneralHeap.end -
          (uintptr_t)gSYTaskmanGeneralHeap.ptr >= {KEEP_FREE}u);

    /* ~190 KB partial: two pending trees, cache cut to what is left. */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({PART190_AVAIL}u, 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(sNdsAnimCacheArenaBytes == {PART190_AVAIL - MARIO_TREE - FOX_TREE - KEEP_FREE}u);
    CHECK(gNdsAnimCachePackDroppedForFightersCount == 1u);
}}

static void scenario_alignment_cliffs(void)
{{
    /* The elastic block's own cliffs: a decline at or under its floor plus
     * minimum, 32-byte steps above, the cap at the top. */
    static const u32 slack[] = {{ {", ".join(f"{s}u" for s in ELASTIC_SLACKS)} }};
    static const u32 expect[] = {{ {", ".join(f"{elastic_bytes(s)}u" for s in ELASTIC_SLACKS)} }};
    u32 i;

    for (i = 0u; i < sizeof(slack) / sizeof(slack[0]); i++)
    {{
        reset_state();
        sBattle.players[0].fkind = nFTKindMario;
        sBattle.players[0].pkind = nFTPlayerKindMan;
        provider_add(sIdMario, {MARIO_TREE}u, 0);
        set_heap({MARIO_TREE}u + slack[i], 0u);
        /* Post-setup stamp: refusals below must be budget refusals, not the
         * setup gate, and successes must be post-setup carves. */
        sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
        if (expect[i] == 0u)
        {{
            CHECK(ndsAnimCacheArenaEnsure() == FALSE);
            CHECK(sMallocCalls == 0u && sElasticRegisters == 0u);
            CHECK(gNdsAnimCacheArenaReserveFailCount == 1u);
            /* The decline latches for the generation: no second attempt. */
            CHECK(ndsAnimCacheArenaEnsure() == FALSE);
            CHECK(gNdsAnimCacheArenaReserveFailCount == 1u);
            CHECK(gNdsAnimCacheArenaReserveFailSkips == 1u);
        }}
        else
        {{
            CHECK(ndsAnimCacheArenaEnsure() == TRUE);
            CHECK(sNdsAnimCacheArenaBytes == expect[i]);
            CHECK(sMallocCalls == 0u);
            CHECK(((uintptr_t)sNdsAnimCacheArena & 31u) == 0u);
            CHECK(((uintptr_t)sNdsAnimCacheArena + expect[i]) <=
                  (uintptr_t)gSYTaskmanGeneralHeap.end);
        }}
    }}

    /* Misaligned cursor: availability is measured from the ALIGNED cursor. */
    reset_state();
    sBattle.players[0].fkind = nFTKindMario;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    set_heap({MARIO_TREE + ELASTIC_FLOOR + 32 + ELASTIC_MIN + 64}u, 5u); /* ptr = start + 5 */
    CHECK(((uintptr_t)gSYTaskmanGeneralHeap.ptr & 15u) != 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(sNdsAnimCacheArenaBytes == {elastic_bytes(ELASTIC_FLOOR + 32 + ELASTIC_MIN + 64)}u);
    CHECK(((uintptr_t)sNdsAnimCacheArena & 31u) == 0u);
}}

static void scenario_no_fit_no_allocator(void)
{{
    /* Pending trees alone exceed the heap: refuse before any allocation. */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({FOX_TREE}u - 1u, 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == 0u);
    CHECK(gNdsAnimCacheArenaReserveFailCount == 1u);

    /* Floor guard: trees fit but leave no KEEP_FREE. */
    reset_state();
    sBattle.players[0].fkind = nFTKindFox;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({FOX_TREE + KEEP_FREE}u, 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == 0u);

    /* Trees + floor fit but round down to a zero cache: still no malloc. */
    reset_state();
    sBattle.players[0].fkind = nFTKindFox;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({FOX_TREE + KEEP_FREE + 15}u, 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == 0u);
    CHECK(gNdsAnimCacheArenaReserveFailCount == 1u);

    /* UINT32_MAX from missing metadata must refuse, never allocate. */
    reset_state();
    roster_mario_fox();
    dFTManagerDataFiles[nFTKindFox] = NULL;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    set_heap(600000u, 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == 0u);
    CHECK(gNdsAnimCacheMatchFighterBytes == 0xFFFFFFFFu);
}}

static void scenario_setup_gate_all_scenes(void)
{{
    u8 *held;
    u32 held_bytes;
    u32 calls_before;

    /* Results shape (2026-09-06 trace): ample heap, but the 258048 carve ran
     * before the mandatory 153600 sprite surface and broke the floor. An
     * unprepared nonbattle scene must not carve at all. */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap(600000u, 0u);
    gNdsSceneManagerCurrIsBattle = 0u;
    CHECK(sNdsAnimCacheSetupGeneration != gNdsTaskmanHeapGeneration);
    CHECK(ndsAnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == 0u);
    CHECK(gNdsAnimCacheArenaReserveFailCount == 0u);
    CHECK(sNdsAnimCacheArena == NULL);

    /* Explicit CSS setup cache is reusable: a generation-owned arena returns
     * first, even while the setup stamp says unprepared. */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap(600000u, 0u);
    gNdsSceneManagerCurrIsBattle = 0u;
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(sMallocCalls == 1u);
    held = sNdsAnimCacheArena;
    held_bytes = sNdsAnimCacheArenaBytes;
    calls_before = sMallocCalls;
    CHECK(held != NULL && held_bytes != 0u);
    sNdsAnimCacheSetupGeneration = 0u; /* next phase unprepared */
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(sMallocCalls == calls_before);
    CHECK(sNdsAnimCacheArena == held);
    CHECK(sNdsAnimCacheArenaBytes == held_bytes);
    CHECK(gNdsAnimCacheArenaReserveCount == 1u);

    /* Stale generation cannot reuse: after the heap rewinds, the old block
     * is dead until the new generation stamps setup. */
    gNdsTaskmanHeapGeneration++;
    gSYTaskmanGeneralHeap.ptr = gSYTaskmanGeneralHeap.start;
    CHECK(ndsAnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == calls_before);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(sMallocCalls == calls_before + 1u);
}}

int main(void)
{{
    scenario_match_fighter_bytes();
    scenario_pack_kept_and_standalone_full();
    scenario_cached_lease_and_reset();
    scenario_pack_drop_then_partial();
    scenario_alignment_cliffs();
    scenario_no_fit_no_allocator();
    scenario_setup_gate_all_scenes();
    /* Battle constructor reads use the uncached path. Opening the current
     * scene's warmer allows allocation; the next scene must establish its
     * own phase. */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap(600000u, 0u);
    gNdsSceneManagerCurrIsBattle = 1u;
    CHECK(ndsAnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == 0u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(sMallocCalls == 1u);
    gNdsTaskmanHeapGeneration++;
    CHECK(ndsAnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == 1u);
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    gSYTaskmanGeneralHeap.ptr = gSYTaskmanGeneralHeap.start;
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(sMallocCalls == 2u);
    return 0;
}}
"""


def control_main():
    """Negative control: the OLD behaviour on the measured OOM input."""
    return f"""
int main(void)
{{
    (void)sTreeBuf; /* the control exercises no residency fixture */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({CONTROL_AVAIL}u, 0u);
    /* Post-setup stamp so the control reaches the budget it removes. */
    sNdsAnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    /* With the future budget zeroed the reserve SUCCEEDS at full size and
     * starves Fox: this is the regression the negative control must show. */
    CHECK(ndsAnimCacheArenaEnsure() == TRUE);
    CHECK(sNdsAnimCacheArenaBytes == {STANDALONE}u);
    CHECK(gNdsAnimCacheArenaReservedBytes == {STANDALONE}u);
    CHECK(gNdsAnimCacheArenaUsedBytes == 0u);
    CHECK(gNdsAnimCacheArenaReserveCount == 1u);
    CHECK(gNdsAnimCacheArenaReserveFailCount == 0u);
    CHECK(gNdsAnimCachePackDroppedForFightersCount == 1u);
    CHECK(gNdsBattlePackCarveDeclineCount == 0u);
    CHECK(gNdsBattlePackCarveMatchKinds == 2u);
    CHECK(gNdsAnimCacheMatchFighterBytes == 0u); /* the removed budget */
    CHECK(gNdsAnimCacheArenaInvalidations == 0u);
    CHECK(gNdsAnimCacheArenaGenerationMismatches == 0u);
    CHECK(gNdsAnimCacheArenaRangeFaults == 0u);
    CHECK(sResidencyDropCalls == 0u);
    CHECK(sMallocCalls == 1u);
    CHECK(sMallocLastSize == {STANDALONE}u);
    CHECK(sMallocLastAlign == NDS_RELOC_ALIGN_BYTES);
    CHECK((uintptr_t)gSYTaskmanGeneralHeap.end -
          (uintptr_t)gSYTaskmanGeneralHeap.ptr == {KEEP_FREE + OLD_LEFT}u);
    CHECK((uintptr_t)gSYTaskmanGeneralHeap.end -
          (uintptr_t)gSYTaskmanGeneralHeap.ptr - {KEEP_FREE}u == {OLD_LEFT}u);
    CHECK({OLD_LEFT}u < {FOX_TREE}u); /* Fox OOM, the measured bug */
    return 0;
}}
"""


def build_source(control=False):
    real = extract_real()
    ensure = real["ensure"]
    if control:
        anchor = "    fighter_bytes = ndsAnimCacheMatchFighterBytes();"
        if ensure.count(anchor) != 1:
            raise AssertionError("negative-control anchor not unique")
        ensure = ensure.replace(
            anchor, anchor + "\n    fighter_bytes = 0u; /* NEGATIVE CONTROL */")
    return "\n".join([
        HARNESS_HEAD,
        real["would_fit"],
        real["still_owned"],
        real["drop_for_reset"],
        real["carve_worth_it"],
        real["decline"],
        real["carve_elastic"],
        real["fighter_bytes"],
        ensure,
        control_main() if control else host_main(),
    ])


def bounded(text, limit=2000):
    text = (text or "").strip()
    return text if len(text) <= limit else text[:limit] + "... [truncated]"


class AnimCacheBudgetTest(unittest.TestCase):
    MAX_LOG = 2000

    def compile_and_run(self, source_text, tag):
        compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc")
                         if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required (clang/gcc/cc)")
        version = subprocess.run([compiler, "--version"], capture_output=True)
        is_clang = b"clang" in version.stdout.lower()
        # Both vendors bound compile diagnostics; the spellings differ.
        error_cap = ["-ferror-limit=8"] if is_clang else ["-fmax-errors=8"]
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / f"anim_cache_{tag}.c"
            program = Path(directory) / f"anim_cache_{tag}.exe"
            source.write_text(source_text, encoding="utf-8")
            built = subprocess.run(
                [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                 *error_cap, str(source), "-o", str(program)],
                capture_output=True)
            self.assertEqual(
                built.returncode, 0,
                f"host build failed:\n{bounded(built.stderr.decode('utf-8', 'replace'), self.MAX_LOG)}")
            ran = subprocess.run([str(program)], capture_output=True, timeout=30)
            out = ran.stdout.decode("utf-8", "replace")
            err = ran.stderr.decode("utf-8", "replace")
            self.assertEqual(
                ran.returncode, 0,
                f"host run failed (stderr):\n{bounded(err, self.MAX_LOG)}\n"
                f"(stdout):\n{bounded(out, self.MAX_LOG)}")

    def test_real_reservation_functions(self):
        self.compile_and_run(build_source(control=False), "real")

    def test_negative_control_without_future_budget(self):
        """Old behaviour (future trees ignored) must reproduce the Fox OOM.

        If this control ever FAILS by allocating a partial cache, the control
        patch no longer models the old code -- fix the patch, not the source.
        """
        self.compile_and_run(build_source(control=True), "control")


if __name__ == "__main__":
    unittest.main()
