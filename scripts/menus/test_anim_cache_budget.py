"""Host-execute the REAL anim-cache reservation functions from reloc_backend_assets.c.

Main's measured bug (fixed on this branch): the standalone 258,048-byte
reservation ignored the fighters that still had to load.  On the capture heap
the cache took 258,048 bytes and Fox's 116,752-byte tree then had only 75,788
left -- an OOM halt.  `ndsR2AnimCacheMatchFighterBytes` now counts the unique
unloaded / stale-generation main trees via the same provider the pending load
will call (`lbRelocGetFileSize`, not the N64 census), and
`ndsR2AnimCacheArenaEnsure` reserves only the largest aligned partial raw
block that fits after those bytes plus the 32,768-byte setup floor whenever
the full pack cannot fit.

These tests EXTRACT the production bodies verbatim (source_test_helpers) and
run them on a host harness with real `ndsSyMallocWouldFit`:

  * extracted, unmodified: ndsR2AnimCacheMatchFighterBytes,
    ndsR2AnimCacheArenaEnsure, ndsR2AnimCacheArenaStillOwned,
    ndsR2AnimCacheArenaDropForReset, ndsR2BattlePackCarveWorthIt (all from
    src/port/reloc_backend_assets.c), ndsSyMallocWouldFit (from
    src/import/battleship_sys_malloc.c).
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

from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]
ASSETS = (ROOT / "src/port/reloc_backend_assets.c").read_text(encoding="utf-8")
MALLOC = (ROOT / "src/import/battleship_sys_malloc.c").read_text(encoding="utf-8")

# Pinned production constants.  Each is parsed back out of the source so the
# test fails loudly if the reservation constants drift.
ALIGN = 0x10
STANDALONE = 258048
PACK_RAW = 163840
KEEP_FREE = 32768
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


pin_int("NDS_R2_ANIM_CACHE_STANDALONE_RAW_BYTES", STANDALONE)
pin_int("NDS_R2_ANIM_CACHE_PACK_RAW_BYTES", PACK_RAW)  # KEEP_CACHE arm is first
pin_int("NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE", KEEP_FREE)
pin_int("NDS_BATTLEPACK_LINE_BYTES", LINE)
if not re.search(r"#define\s+NDS_RELOC_ALIGN_BYTES\s+0x10u", ASSETS):
    raise AssertionError("NDS_RELOC_ALIGN_BYTES drifted from 0x10u")

# Explicit provider bytes (fixture, not census).
MARIO_TREE = 99856
FOX_TREE = 116752
OLD_AVAIL = STANDALONE + 75788  # 333836: the measured OOM heap input
OLD_LEFT = OLD_AVAIL - STANDALONE  # 75788: what Fox had after the old reserve
assert OLD_LEFT < FOX_TREE  # the measured OOM
FIX_AVAIL = (OLD_AVAIL - FOX_TREE - KEEP_FREE) & ~(ALIGN - 1)  # 184304 partial
PART190_AVAIL = MARIO_TREE + FOX_TREE + KEEP_FREE + 194560  # ~190 KB case


def extract_real():
    """Pull the verbatim production bodies this test executes."""
    out = {
        "ensure": function(ASSETS, "ndsR2AnimCacheArenaEnsure"),
        "fighter_bytes": function(ASSETS, "ndsR2AnimCacheMatchFighterBytes"),
        "still_owned": function(ASSETS, "ndsR2AnimCacheArenaStillOwned"),
        "drop_for_reset": function(ASSETS, "ndsR2AnimCacheArenaDropForReset"),
        "carve_worth_it": function(ASSETS, "ndsR2BattlePackCarveWorthIt"),
        "would_fit": function(MALLOC, "ndsSyMallocWouldFit"),
    }
    for needle in ("ndsSyMallocWouldFit", "fighter_bytes",
                   "NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE"):
        if needle not in out["ensure"]:
            raise AssertionError(f"extracted Ensure lost '{needle}'")
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
#define NDS_R2_BATTLEPACK_BLOB_BYTES 287904u
#define NDS_BATTLEPACK_RESERVE_BYTES \
    ((NDS_R2_BATTLEPACK_BLOB_BYTES + (2u * NDS_BATTLEPACK_LINE_BYTES) - 1u) & \
     ~(NDS_BATTLEPACK_LINE_BYTES - 1u))
#define NDS_R2_BATTLEPACK 1
#define NDS_R2_ANIM_CACHE_PACK_RAW_BYTES 163840u
#define NDS_R2_ANIM_CACHE_STANDALONE_RAW_BYTES 258048u
#define NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE 32768u
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
static SYMallocRegion gSYTaskmanGeneralHeap;
static u8 sHeapBuf[1 << 20];

static u8 *sNdsR2AnimCacheArena;
static u32 sNdsR2AnimCacheArenaBytes;
static u32 sNdsR2AnimCacheArenaUsed;
static sb32 sNdsR2AnimCacheArenaRawOnly;
static u32 sNdsR2AnimCacheArenaGeneration;
static u32 sNdsR2AnimCacheCount;

static volatile u32 gNdsR2AnimCacheBytes;
static volatile u32 gNdsR2AnimCacheMatchFighterBytes;
static volatile u32 gNdsR2AnimCachePackDroppedForFightersCount;
static volatile u32 gNdsBattlePackCarveDeclineCount;
static volatile u32 gNdsBattlePackCarveMatchKinds;
static volatile u32 gNdsR2AnimCacheArenaReserveFailCount;
static volatile u32 gNdsR2AnimCacheArenaReserveCount;
static volatile u32 gNdsR2AnimCacheArenaReservedBytes;
static volatile u32 gNdsR2AnimCacheArenaUsedBytes;
static volatile u32 gNdsR2AnimCacheArenaInvalidations;
static volatile u32 gNdsR2AnimCacheArenaGenerationMismatches;
static volatile u32 gNdsR2AnimCacheArenaRangeFaults;
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
    sNdsR2AnimCacheArena = NULL;
    sNdsR2AnimCacheArenaBytes = 0u;
    sNdsR2AnimCacheArenaUsed = 0u;
    sNdsR2AnimCacheArenaRawOnly = FALSE;
    sNdsR2AnimCacheArenaGeneration = 0u;
    sNdsR2AnimCacheCount = 0u;
    gNdsR2AnimCacheBytes = 0u;
    gNdsR2AnimCacheMatchFighterBytes = 0u;
    gNdsR2AnimCachePackDroppedForFightersCount = 0u;
    gNdsBattlePackCarveDeclineCount = 0u;
    gNdsBattlePackCarveMatchKinds = 0u;
    gNdsR2AnimCacheArenaReserveFailCount = 0u;
    gNdsR2AnimCacheArenaReserveCount = 0u;
    gNdsR2AnimCacheArenaReservedBytes = 0u;
    gNdsR2AnimCacheArenaUsedBytes = 0u;
    gNdsR2AnimCacheArenaInvalidations = 0u;
    gNdsR2AnimCacheArenaGenerationMismatches = 0u;
    gNdsR2AnimCacheArenaRangeFaults = 0u;
    gNdsRelocFileSizeFallbackCount = 0u;
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
    CHECK(ndsR2AnimCacheMatchFighterBytes() == 0u);

    /* Duplicate kind counts once; Not slots and out-of-range kinds skipped. */
    reset_state();
    roster_mario_fox();
    sBattle.players[2].fkind = nFTKindFox;
    sBattle.players[2].pkind = nFTPlayerKindCom;
    sBattle.players[3].fkind = nFTKindEnumCount;
    sBattle.players[3].pkind = nFTPlayerKindCom;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    total = ndsR2AnimCacheMatchFighterBytes();
    CHECK(total == {MARIO_TREE + FOX_TREE}u);
    CHECK(sProvider[0].calls == 1u && sProvider[1].calls == 1u);

    /* Current-generation resident main tree: skipped BEFORE the provider. */
    reset_state();
    roster_mario_fox();
    sFoxMain = sTreeBuf + 8;
    sLoaded[0].lo = sTreeBuf;
    sLoaded[0].hi = sTreeBuf + sizeof(sTreeBuf);
    sLoaded[0].owner_generation = gNdsTaskmanHeapGeneration;
    sLoaded[0].file.owner_generation = gNdsTaskmanHeapGeneration;
    sLoadedCount = 1u;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    total = ndsR2AnimCacheMatchFighterBytes();
    CHECK(total == {MARIO_TREE}u);
    CHECK(sProvider[1].calls == 0u);

    /* Stale generation: same bytes are resident but dead -- counted. */
    sLoaded[0].owner_generation = gNdsTaskmanHeapGeneration - 1u;
    sLoaded[0].file.owner_generation = gNdsTaskmanHeapGeneration - 1u;
    total = ndsR2AnimCacheMatchFighterBytes();
    CHECK(total == {MARIO_TREE + FOX_TREE}u);
    CHECK(sProvider[1].calls == 1u);

    /* p_file_main present but slot still NULL: tree is pending, counted. */
    reset_state();
    roster_mario_fox();
    sFoxData.p_file_main = &sFoxMain; /* slot stays NULL */
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    CHECK(ndsR2AnimCacheMatchFighterBytes() == {MARIO_TREE + FOX_TREE}u);

    /* Missing FTData: fail closed with UINT32_MAX. */
    reset_state();
    roster_mario_fox();
    dFTManagerDataFiles[nFTKindFox] = NULL;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    CHECK(ndsR2AnimCacheMatchFighterBytes() == 0xFFFFFFFFu);

    /* Provider fires the fallback witness: fail closed. */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 1);
    CHECK(ndsR2AnimCacheMatchFighterBytes() == 0xFFFFFFFFu);
    CHECK(gNdsRelocFileSizeFallbackCount == 1u);

    /* id with no metadata row at all: fallback, fail closed. */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    CHECK(ndsR2AnimCacheMatchFighterBytes() == 0xFFFFFFFFu);

    /* Sum overflow guard: a bogus provider size refuses rather than wraps. */
    reset_state();
    sBattle.players[0].fkind = nFTKindLuigi;
    sBattle.players[0].pkind = nFTPlayerKindCom;
    provider_add(sIdLuigi, (size_t)0x0000FFFFFFFFFFFFull, 0);
    CHECK(ndsR2AnimCacheMatchFighterBytes() == 0xFFFFFFFFu);
}}

static void scenario_pack_kept_and_standalone_full(void)
{{
    /* Mario-only roster: pack declined (no Fox), full standalone reserve,
     * then the pending tree and the floor both still fit. */
    reset_state();
    sBattle.players[0].fkind = nFTKindMario;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    set_heap({MARIO_TREE + KEEP_FREE + STANDALONE}u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsBattlePackCarveDeclineCount == 1u);
    CHECK(gNdsBattlePackCarveMatchKinds == 1u);
    CHECK(sMallocCalls == 1u && sMallocLastSize == {STANDALONE}u &&
          sMallocLastAlign == NDS_RELOC_ALIGN_BYTES);
    CHECK(sNdsR2AnimCacheArenaBytes == {STANDALONE}u);
    CHECK(sNdsR2AnimCacheArenaRawOnly == TRUE);
    CHECK(sNdsR2AnimCacheArenaUsed == 0u);
    CHECK(sNdsR2AnimCacheArenaGeneration == gNdsTaskmanHeapGeneration);
    CHECK(gNdsR2AnimCacheArenaReservedBytes == {STANDALONE}u);
    CHECK(gNdsR2AnimCacheMatchFighterBytes == {MARIO_TREE}u);
    CHECK(sNdsR2AnimCacheArena != NULL);
    CHECK(((uintptr_t)sNdsR2AnimCacheArena & (NDS_RELOC_ALIGN_BYTES - 1u)) == 0u);
    CHECK((uintptr_t)sNdsR2AnimCacheArena >= (uintptr_t)gSYTaskmanGeneralHeap.start);
    /* The future tree and the floor still fit after the real allocation. */
    CHECK(syTaskmanMalloc({MARIO_TREE}u, NDS_RELOC_ALIGN_BYTES) != NULL);
    CHECK((uintptr_t)gSYTaskmanGeneralHeap.end -
          (uintptr_t)gSYTaskmanGeneralHeap.ptr >= {KEEP_FREE}u);

    /* Fox-only roster with room for pack + tree + floor: pack kept, and the
     * pack's region is carved FIRST (used == reserve at reservation time). */
    reset_state();
    sBattle.players[0].fkind = nFTKindFox;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({RESERVE + PACK_RAW + FOX_TREE + KEEP_FREE}u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsBattlePackCarveDeclineCount == 0u);
    CHECK(gNdsR2AnimCachePackDroppedForFightersCount == 0u);
    CHECK(sNdsR2AnimCacheArenaBytes == {RESERVE + PACK_RAW}u);
    CHECK(sNdsR2AnimCacheArenaRawOnly == FALSE);
    CHECK(sNdsR2AnimCacheArenaUsed == {RESERVE}u);
    CHECK(sMallocCalls == 1u && sMallocLastSize == {RESERVE + PACK_RAW}u);
    CHECK(gNdsR2AnimCacheArenaUsedBytes == {RESERVE}u);
}}

static void scenario_cached_lease_and_reset(void)
{{
    reset_state();
    sBattle.players[0].fkind = nFTKindMario;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    set_heap({MARIO_TREE + KEEP_FREE + STANDALONE}u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsR2AnimCacheArenaReserveCount == 1u);

    /* Cached lease: second Ensure reuses the block, no second malloc. */
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    CHECK(sMallocCalls == 1u);
    CHECK(gNdsR2AnimCacheArenaReserveCount == 1u);

    /* Heap rewound under us (generation bump): stale cache is dropped whole
     * -- including its entries -- then re-reserved fresh. */
    sNdsR2AnimCacheCount = 3u;
    gNdsTaskmanHeapGeneration++;
    gSYTaskmanGeneralHeap.ptr = gSYTaskmanGeneralHeap.start;
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsR2AnimCacheArenaInvalidations == 1u);
    CHECK(sResidencyDropCalls == 1u);
    CHECK(sNdsR2AnimCacheCount == 0u);
    CHECK(gNdsR2AnimCacheBytes == 0u);
    CHECK(sMallocCalls == 2u);
    CHECK(gNdsR2AnimCacheArenaReserveCount == 2u);

    /* Same generation but cursor below the block: range fault, same reset. */
    reset_state();
    sBattle.players[0].fkind = nFTKindMario;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    set_heap({MARIO_TREE + KEEP_FREE + STANDALONE}u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    gSYTaskmanGeneralHeap.ptr = gSYTaskmanGeneralHeap.start; /* no gen bump */
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsR2AnimCacheArenaRangeFaults == 1u);
    CHECK(gNdsR2AnimCacheArenaInvalidations == 1u);
    CHECK(sMallocCalls == 2u);
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
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsR2AnimCachePackDroppedForFightersCount == 1u);
    CHECK(sNdsR2AnimCacheArenaBytes == {STANDALONE}u);
    CHECK(sNdsR2AnimCacheArenaRawOnly == TRUE);
    CHECK(sNdsR2AnimCacheArenaUsed == 0u);
    CHECK(gNdsR2AnimCacheMatchFighterBytes == {MARIO_TREE + FOX_TREE}u);

    /* THE MEASURED BUG INPUT (2026-09-03 capture shape): old code reserved
     * {STANDALONE} first and left Fox {OLD_LEFT} of his {FOX_TREE}.  The fix
     * shrinks the cache to the largest aligned partial after Fox + floor. */
    reset_state();
    roster_mario_fox();
    sMarioMain = sTreeBuf + 8; /* Mario resident, current generation */
    sLoaded[0].lo = sTreeBuf;
    sLoaded[0].hi = sTreeBuf + sizeof(sTreeBuf);
    sLoaded[0].owner_generation = gNdsTaskmanHeapGeneration;
    sLoaded[0].file.owner_generation = gNdsTaskmanHeapGeneration;
    sLoadedCount = 1u;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({OLD_AVAIL}u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    CHECK(gNdsR2AnimCacheMatchFighterBytes == {FOX_TREE}u);
    CHECK(gNdsR2AnimCachePackDroppedForFightersCount == 1u);
    CHECK(sNdsR2AnimCacheArenaBytes == {FIX_AVAIL}u);
    CHECK(gNdsR2AnimCacheArenaReservedBytes == {FIX_AVAIL}u);
    CHECK(sNdsR2AnimCacheArenaRawOnly == TRUE);
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
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    CHECK(sNdsR2AnimCacheArenaBytes == {PART190_AVAIL - MARIO_TREE - FOX_TREE - KEEP_FREE}u);
    CHECK(gNdsR2AnimCachePackDroppedForFightersCount == 1u);
}}

static void scenario_alignment_cliffs(void)
{{
    static const u32 slack[] = {{ 15u, 16u, 17u, 4095u, 4096u,
                                  {STANDALONE - 1}u, {STANDALONE}u, {STANDALONE + 15}u }};
    u32 i;

    for (i = 0u; i < sizeof(slack) / sizeof(slack[0]); i++)
    {{
        u32 avail = {MARIO_TREE + KEEP_FREE}u + slack[i];
        u32 expect = (slack[i] < {STANDALONE}u) ? (slack[i] & ~(u32)(NDS_RELOC_ALIGN_BYTES - 1u))
                                                 : {STANDALONE}u;
        reset_state();
        sBattle.players[0].fkind = nFTKindMario;
        sBattle.players[0].pkind = nFTPlayerKindMan;
        provider_add(sIdMario, {MARIO_TREE}u, 0);
        set_heap(avail, 0u);
        if (expect == 0u)
        {{
            CHECK(ndsR2AnimCacheArenaEnsure() == FALSE);
            CHECK(sMallocCalls == 0u);
            CHECK(gNdsR2AnimCacheArenaReserveFailCount == 1u);
        }}
        else
        {{
            CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
            CHECK(sNdsR2AnimCacheArenaBytes == expect);
            CHECK(sMallocCalls == 1u && sMallocLastSize == expect);
            CHECK(((uintptr_t)sNdsR2AnimCacheArena & 15u) == 0u);
            CHECK(((uintptr_t)sNdsR2AnimCacheArena + expect) <=
                  (uintptr_t)gSYTaskmanGeneralHeap.end);
        }}
    }}

    /* Misaligned cursor: availability is measured from the ALIGNED cursor. */
    reset_state();
    sBattle.players[0].fkind = nFTKindMario;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    set_heap({MARIO_TREE + KEEP_FREE + 4096}u, 5u); /* ptr = start + 5 */
    CHECK(((uintptr_t)gSYTaskmanGeneralHeap.ptr & 15u) != 0u);
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    CHECK(sNdsR2AnimCacheArenaBytes == 4096u);
    CHECK(((uintptr_t)sNdsR2AnimCacheArena & 15u) == 0u);
}}

static void scenario_no_fit_no_allocator(void)
{{
    /* Pending trees alone exceed the heap: refuse before any allocation. */
    reset_state();
    roster_mario_fox();
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({FOX_TREE}u - 1u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == 0u);
    CHECK(gNdsR2AnimCacheArenaReserveFailCount == 1u);

    /* Floor guard: trees fit but leave no KEEP_FREE. */
    reset_state();
    sBattle.players[0].fkind = nFTKindFox;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({FOX_TREE + KEEP_FREE}u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == 0u);

    /* Trees + floor fit but round down to a zero cache: still no malloc. */
    reset_state();
    sBattle.players[0].fkind = nFTKindFox;
    sBattle.players[0].pkind = nFTPlayerKindMan;
    provider_add(sIdFox, {FOX_TREE}u, 0);
    set_heap({FOX_TREE + KEEP_FREE + 15}u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == 0u);
    CHECK(gNdsR2AnimCacheArenaReserveFailCount == 1u);

    /* UINT32_MAX from missing metadata must refuse, never allocate. */
    reset_state();
    roster_mario_fox();
    dFTManagerDataFiles[nFTKindFox] = NULL;
    provider_add(sIdMario, {MARIO_TREE}u, 0);
    set_heap(600000u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsure() == FALSE);
    CHECK(sMallocCalls == 0u);
    CHECK(gNdsR2AnimCacheMatchFighterBytes == 0xFFFFFFFFu);
}}

int main(void)
{{
    scenario_match_fighter_bytes();
    scenario_pack_kept_and_standalone_full();
    scenario_cached_lease_and_reset();
    scenario_pack_drop_then_partial();
    scenario_alignment_cliffs();
    scenario_no_fit_no_allocator();
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
    set_heap({OLD_AVAIL}u, 0u);
    /* With the future budget zeroed the reserve SUCCEEDS at full size and
     * starves Fox: this is the regression the negative control must show. */
    CHECK(ndsR2AnimCacheArenaEnsure() == TRUE);
    CHECK(sNdsR2AnimCacheArenaBytes == {STANDALONE}u);
    CHECK(gNdsR2AnimCacheArenaReservedBytes == {STANDALONE}u);
    CHECK(gNdsR2AnimCacheArenaUsedBytes == 0u);
    CHECK(gNdsR2AnimCacheArenaReserveCount == 1u);
    CHECK(gNdsR2AnimCacheArenaReserveFailCount == 0u);
    CHECK(gNdsR2AnimCachePackDroppedForFightersCount == 1u);
    CHECK(gNdsBattlePackCarveDeclineCount == 0u);
    CHECK(gNdsBattlePackCarveMatchKinds == 2u);
    CHECK(gNdsR2AnimCacheMatchFighterBytes == 0u); /* the removed budget */
    CHECK(gNdsR2AnimCacheArenaInvalidations == 0u);
    CHECK(gNdsR2AnimCacheArenaGenerationMismatches == 0u);
    CHECK(gNdsR2AnimCacheArenaRangeFaults == 0u);
    CHECK(sResidencyDropCalls == 0u);
    CHECK(sMallocCalls == 1u);
    CHECK(sMallocLastSize == {STANDALONE}u);
    CHECK(sMallocLastAlign == NDS_RELOC_ALIGN_BYTES);
    CHECK((uintptr_t)gSYTaskmanGeneralHeap.end -
          (uintptr_t)gSYTaskmanGeneralHeap.ptr == {OLD_LEFT}u);
    CHECK({OLD_LEFT}u < {FOX_TREE}u); /* Fox OOM, the measured bug */
    return 0;
}}
"""


def build_source(control=False):
    real = extract_real()
    ensure = real["ensure"]
    if control:
        anchor = "    fighter_bytes = ndsR2AnimCacheMatchFighterBytes();"
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
