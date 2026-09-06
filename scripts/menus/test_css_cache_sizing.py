"""Host-execute the REAL CSS setup-cache sizer from reloc_backend_assets.c.

Measured bug (this branch): the fixed 32 KiB CSS carve cached 8 of the 9
admitted roster's initial clips (32,624 B) and left Pikachu out (anim-fail
bit 0x200); its 20,132 B image pair then loaded only on visit 1, and the
full-allocation ledger showed that plus the 8,704 B optional world cache
behind the 10,632 B visit-1 variance.  Main now derives the exact distinct
initial-clip bytes from the already-loaded admitted FTData submotion 0 via
the same DS stream/raw provider the warm loader uses
(`ndsR2AnimWarmLoadOne` calls `ndsR2AnimCachePayloadBytes`), aligns between
clips, and rejects invalid/overflow fail-closed.

These tests EXTRACT the production bodies verbatim (source_test_helpers)
and run them on a host harness with real `ndsSyMallocWouldFit`:

  * extracted, unmodified: ndsR2AnimCachePayloadBytes,
    ndsR2AnimCacheSetupBytes, ndsR2AnimCacheArenaEnsureSetup,
    ndsR2AnimCacheValidateGeneration, ndsR2AnimCacheArenaStillOwned,
    ndsR2AnimCacheArenaDropForReset (all from
    src/port/reloc_backend_assets.c), ndsSyMallocWouldFit (from
    src/import/battleship_sys_malloc.c).
  * stubbed (explicit fixture seams, exactly like test_anim_cache_budget.py
    stubs lbRelocGetFileSize): ndsRelocAssetIDForToken (identity, one BAD
    token maps INVALID), ndsRelocIsFighterAnimID (one NONFIGHTER id refused),
    the three PayloadBytes providers (ndsRelocAssetLoadFighterStreamClip,
    ndsRelocP2GeneratedAllocSize, ndsRelocAssetAllocSize -- per-id tables so
    stream-vs-raw fallback parity is measurable), ndsBattlePackResidencyDrop,
    and syTaskmanMalloc (a bump carve with the decomp syMallocSet cursor
    semantics).  No sizing/allocation decision under test is re-implemented;
    the only oracle arithmetic lives in this file's Python expectations.

The negative control compiles the same extracted EnsureSetup with ONE patch
(`bytes = 32768u;`, i.e. the restored fixed carve) and proves the nine-clip
need (36,656 B) overflows a 32,768 B arena -- the fixed size fails the full
roster while the derived size fits it exactly.

LIMITATIONS (deliberate): the eight fixture clip sizes are chosen to sum to
the measured 32,624 B, not per-clip captures; the 20,132 B image pair and
the 8,704 B world cache are separate allocations this sizer never covers.
Observed fail-closed semantics, kept: ONE invalid/non-fighter/zero-size/
overflowing clip vetoes the whole setup (returns 0u) rather than skipping
that kind -- that is the production rejection rule, asserted here so any
future skip-instead behaviour fails loudly.  Fixture divergence, stated:
production FTData is 120 B with the submotion pointer at offset 104; the
host fixture types only the two fields the extracted body touches
(p_file_main, submotion->motion_desc[0].anim_file_id).
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
FIXED = 32768
KEEP_FREE = 32768


def pin_int(name, expected, which=0):
    hits = [int(m) for m in re.findall(rf"#define\s+{name}\s+(\d+)u", ASSETS)]
    if not hits or hits[which] != expected:
        raise AssertionError(f"{name} drifted from {expected}: {hits}")
    return expected


pin_int("NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE", KEEP_FREE)
if not re.search(r"#define\s+NDS_RELOC_ALIGN_BYTES\s+0x10u", ASSETS):
    raise AssertionError("NDS_RELOC_ALIGN_BYTES drifted from 0x10u")
if "static size_t ndsR2AnimCachePayloadBytes" not in ASSETS:
    raise AssertionError("ndsR2AnimCachePayloadBytes missing from source")
if "static u32 ndsR2AnimCacheSetupBytes" not in ASSETS:
    raise AssertionError("ndsR2AnimCacheSetupBytes missing from source")
if "static sb32 ndsR2AnimCacheArenaEnsureSetup" not in ASSETS:
    raise AssertionError("ndsR2AnimCacheArenaEnsureSetup missing from source")

# Fixture clip sizes.  The eight sum to the measured 32,624 B (all 16-aligned
# so alignment is a no-op here and the figure is exact); the ninth (Pikachu)
# pushes the roster over the old fixed 32,768 B carve.
S8 = [4000, 4000, 4000, 4000, 4000, 4000, 4128, 4496]
TOTAL8 = sum(S8)
assert TOTAL8 == 32624, TOTAL8
PIKA_SIZE = 4032
TOTAL9 = TOTAL8 + PIKA_SIZE
assert TOTAL9 == 36656, TOTAL9
assert TOTAL8 <= FIXED < TOTAL9  # fixed fit 8, missed the 9th: the bug
A = [0x1001, 0x1002, 0x1003, 0x1004, 0x1005, 0x1006, 0x1007, 0x1008]
PIKA = 0x1009
BAD = 0xDEAD
NONFIGHTER = 0x3001
Q1, Q2, Q3, Q0 = 0x2001, 0x2002, 0x2003, 0x2004
AL1, AL2 = 0x2011, 0x2012
ZERO_ID, BIG_ID = 0x2021, 0x2022
O1, O2 = 0x2023, 0x2024
DUP_EXPECT = 8000  # A1 + A2, duplicate A1 counted once
ALIGN_EXPECT = 65  # 17 + align_up(17)=32 + 33


def extract_real():
    """Pull the verbatim production bodies this test executes."""
    out = {
        "payload": function(ASSETS, "ndsR2AnimCachePayloadBytes"),
        "setup": function(ASSETS, "ndsR2AnimCacheSetupBytes"),
        "ensure_setup": function(ASSETS, "ndsR2AnimCacheArenaEnsureSetup"),
        "validate": function(ASSETS, "ndsR2AnimCacheValidateGeneration"),
        "still_owned": function(ASSETS, "ndsR2AnimCacheArenaStillOwned"),
        "drop_for_reset": function(ASSETS, "ndsR2AnimCacheArenaDropForReset"),
        "would_fit": function(MALLOC, "ndsSyMallocWouldFit"),
    }
    for needle in ("ndsRelocAssetLoadFighterStreamClip",
                   "ndsRelocP2GeneratedAllocSize", "ndsRelocAssetAllocSize"):
        if needle not in out["payload"]:
            raise AssertionError(f"extracted PayloadBytes lost '{needle}'")
    for needle in ("motion_desc[0]", "ndsR2AnimCachePayloadBytes",
                   "UINT32_MAX", "ndsRelocIsFighterAnimID"):
        if needle not in out["setup"]:
            raise AssertionError(f"extracted SetupBytes lost '{needle}'")
    for needle in ("ndsR2AnimCacheSetupBytes", "ndsSyMallocWouldFit",
                   "NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE",
                   "ndsR2AnimCacheArenaStillOwned"):
        if needle not in out["ensure_setup"]:
            raise AssertionError(f"extracted EnsureSetup lost '{needle}'")
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

#define NDS_RELOC_ASSET_INVALID 0xffffffffu
#define NDS_RELOC_ALIGN_BYTES 0x10u
#define NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE 32768u
#define NDS_R2_FTANIM_STREAM 1
#define NDS_IMPORT_BATTLESHIP_FTMANAGER 1

/* Nine-kind admitted roster fixture (production has twelve playable kinds;
 * the measurement under test is a 9-roster with 8 clips at 32,624 B). */
enum { nFTKindPlayableStart = 0, nFTKindMario = 0, nFTKindFox,
       nFTKindDonkey, nFTKindSamus, nFTKindLuigi, nFTKindLink, nFTKindYoshi,
       nFTKindCaptain, nFTKindPikachu, nFTKindPlayableEnd = nFTKindPikachu,
       nFTKindEnumCount };

typedef struct SYMallocRegion { u32 id; void *start; void *end; void *ptr; }
    SYMallocRegion;
/* Fixture FTData: only the two fields the extracted body touches. */
typedef struct { u32 anim_file_id; } FixtureMotionDesc;
typedef struct { FixtureMotionDesc motion_desc[1]; } FixtureMotionArray;
typedef struct FTData { void **p_file_main; FixtureMotionArray *submotion; }
    FTData;

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
static volatile u32 gNdsR2AnimCacheArenaReservedBytes;
static volatile u32 gNdsR2AnimCacheArenaUsedBytes;
static volatile u32 gNdsR2AnimCacheArenaReserveCount;
static volatile u32 gNdsR2AnimCacheArenaReserveFailCount;
static volatile u32 gNdsR2AnimCacheArenaInvalidations;
static volatile u32 gNdsR2AnimCacheArenaGenerationMismatches;
static volatile u32 gNdsR2AnimCacheArenaRangeFaults;

/* ---- stubs: the three PayloadBytes providers + token seams ---- */

typedef struct { u32 id; u32 stream_size; int ready; u32 calls; } StreamEntry;
static StreamEntry sStream[16];
static u32 sStreamCount;
typedef struct { u32 id; size_t bytes; u32 calls; } SizeEntry;
static SizeEntry sGen[16];
static u32 sGenCount;
static SizeEntry sRaw[16];
static u32 sRawCount;
static u32 sBadToken;
static u32 sNonFighter;

static u32 ndsRelocAssetIDForToken(u32 token)
{
    if (token == sBadToken)
    {
        return NDS_RELOC_ASSET_INVALID;
    }
    return token;
}

static s32 ndsRelocIsFighterAnimID(u32 asset_id)
{
    return (asset_id == sNonFighter) ? FALSE : TRUE;
}

static s32 ndsRelocAssetLoadFighterStreamClip(u32 asset_id, void *dst,
                                              u32 *out_size)
{
    u32 i;
    (void)dst;
    for (i = 0u; i < sStreamCount; i++)
    {
        if (sStream[i].id == asset_id)
        {
            sStream[i].calls++;
            if (sStream[i].ready)
            {
                if (out_size != NULL)
                {
                    *out_size = sStream[i].stream_size;
                }
                return TRUE;
            }
            if (out_size != NULL)
            {
                *out_size = 0u;
            }
            return FALSE;
        }
    }
    if (out_size != NULL)
    {
        *out_size = 0u;
    }
    return FALSE;
}

static size_t ndsRelocP2GeneratedAllocSize(u32 asset_id)
{
    u32 i;
    for (i = 0u; i < sGenCount; i++)
    {
        if (sGen[i].id == asset_id)
        {
            sGen[i].calls++;
            return sGen[i].bytes;
        }
    }
    return 0u;
}

static size_t ndsRelocAssetAllocSize(u32 asset_id)
{
    u32 i;
    for (i = 0u; i < sRawCount; i++)
    {
        if (sRaw[i].id == asset_id)
        {
            sRaw[i].calls++;
            return sRaw[i].bytes;
        }
    }
    return 0u;
}

static u32 sResidencyDropCalls;
static void ndsBattlePackResidencyDrop(void) { sResidencyDropCalls++; }

/* Host carve with the decomp syMallocSet cursor semantics (align up, add
 * size exactly).  The fit decision under test is the REAL ndsSyMallocWouldFit. */
static u32 sMallocCalls;
static size_t sMallocLastSize;
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
    return block;
}

/* ---- fixture roster ---- */

static FTData sFT[nFTKindEnumCount];
static FixtureMotionArray sMot[nFTKindEnumCount];
static void *sMainVal[nFTKindEnumCount];
static u8 sDummy[nFTKindEnumCount][16];
static FTData *dFTManagerDataFiles[nFTKindEnumCount];

static void reset_state(void)
{
    u32 i;
    memset(sFT, 0, sizeof(sFT));
    memset(sMot, 0, sizeof(sMot));
    memset(sMainVal, 0, sizeof(sMainVal));
    for (i = 0u; i < (u32)nFTKindEnumCount; i++)
    {
        dFTManagerDataFiles[i] = NULL;
    }
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
    gNdsR2AnimCacheArenaReservedBytes = 0u;
    gNdsR2AnimCacheArenaUsedBytes = 0u;
    gNdsR2AnimCacheArenaReserveCount = 0u;
    gNdsR2AnimCacheArenaReserveFailCount = 0u;
    gNdsR2AnimCacheArenaInvalidations = 0u;
    gNdsR2AnimCacheArenaGenerationMismatches = 0u;
    gNdsR2AnimCacheArenaRangeFaults = 0u;
    memset(sStream, 0, sizeof(sStream));
    sStreamCount = 0u;
    memset(sGen, 0, sizeof(sGen));
    sGenCount = 0u;
    memset(sRaw, 0, sizeof(sRaw));
    sRawCount = 0u;
    sBadToken = 0xDEADu;
    sNonFighter = 0x3001u;
    sResidencyDropCalls = 0u;
    sMallocCalls = 0u;
    sMallocLastSize = 0u;
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

static void kind_load(s32 kind, u32 token)
{
    sMot[kind].motion_desc[0].anim_file_id = token;
    sMainVal[kind] = sDummy[kind];
    sFT[kind].p_file_main = &sMainVal[kind];
    sFT[kind].submotion = &sMot[kind];
    dFTManagerDataFiles[kind] = &sFT[kind];
}

static void stream_add(u32 id, u32 size, int ready)
{
    sStream[sStreamCount].id = id;
    sStream[sStreamCount].stream_size = size;
    sStream[sStreamCount].ready = ready;
    sStreamCount++;
}

static void stream_set_ready(u32 id, int ready)
{
    u32 i;
    for (i = 0u; i < sStreamCount; i++)
    {
        if (sStream[i].id == id)
        {
            sStream[i].ready = ready;
        }
    }
}

static void gen_add(u32 id, size_t bytes)
{
    sGen[sGenCount].id = id;
    sGen[sGenCount].bytes = bytes;
    sGenCount++;
}

static void raw_add(u32 id, size_t bytes)
{
    sRaw[sRawCount].id = id;
    sRaw[sRawCount].bytes = bytes;
    sRawCount++;
}

#define CHECK(cond) do { if (!(cond)) { \
    fprintf(stderr, "CHECK failed line %d: %s\n", __LINE__, #cond); \
    exit(1); } } while (0)
'''


def host_main():
    """Scenario main for the real extracted bodies (all numbers from Python)."""
    nine_ids = ", ".join(f"{v}u" for v in (*A, PIKA))
    nine_sizes = ", ".join(str(v) for v in (*S8, PIKA_SIZE))
    eight_ids = ", ".join(f"{v}u" for v in A)
    eight_sizes = ", ".join(str(v) for v in S8)
    return f"""
static const u32 kNineIDs[9] = {{ {nine_ids} }};
static const u32 kNineSizes[9] = {{ {nine_sizes} }};
static const u32 kEightIDs[8] = {{ {eight_ids} }};
static const u32 kEightSizes[8] = {{ {eight_sizes} }};

static void roster_nine(void)
{{
    u32 k;
    for (k = 0u; k < 9u; k++)
    {{
        kind_load((s32)k, kNineIDs[k]);
        stream_add(kNineIDs[k], kNineSizes[k], 1);
        raw_add(kNineIDs[k], (size_t)kNineSizes[k]);
    }}
}}

static void roster_eight(void)
{{
    u32 k;
    for (k = 0u; k < 8u; k++)
    {{
        kind_load((s32)k, kEightIDs[k]);
        stream_add(kEightIDs[k], kEightSizes[k], 1);
        raw_add(kEightIDs[k], (size_t)kEightSizes[k]);
    }}
}}

static void scenario_nine_exceeds_fixed(void)
{{
    u32 need;

    /* Nine distinct initial clips need {TOTAL9}u: the fixed {FIXED}u carve
     * that shipped before Main covered only eight ({TOTAL8}u). */
    reset_state();
    roster_nine();
    need = ndsR2AnimCacheSetupBytes();
    CHECK(need == {TOTAL9}u);
    CHECK(need > {FIXED}u);
    CHECK({TOTAL8}u <= {FIXED}u);

    /* Eight (Pikachu omitted) reproduce the measured {TOTAL8}u figure. */
    reset_state();
    roster_eight();
    CHECK(ndsR2AnimCacheSetupBytes() == {TOTAL8}u);

    /* The full nine fit exactly when the heap carries clips + floor. */
    reset_state();
    roster_nine();
    set_heap({TOTAL9 + KEEP_FREE}u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == TRUE);
    CHECK(sNdsR2AnimCacheArenaBytes == {TOTAL9}u);
    CHECK(sNdsR2AnimCacheArenaRawOnly == TRUE);
    CHECK(sNdsR2AnimCacheArenaUsed == 0u);
    CHECK(sNdsR2AnimCacheArenaGeneration == gNdsTaskmanHeapGeneration);
    CHECK(gNdsR2AnimCacheArenaReservedBytes == {TOTAL9}u);
    CHECK(gNdsR2AnimCacheArenaUsedBytes == 0u);
    CHECK(sMallocCalls == 1u && sMallocLastSize == {TOTAL9}u);
    CHECK(((uintptr_t)sNdsR2AnimCacheArena & 15u) == 0u);
    CHECK(((uintptr_t)sNdsR2AnimCacheArena + {TOTAL9}u) <=
          (uintptr_t)gSYTaskmanGeneralHeap.end);
}}

static void scenario_duplicates_and_unloaded(void)
{{
    u32 need;

    reset_state();
    kind_load(0, {A[0]}u);
    kind_load(1, {A[0]}u); /* duplicate anim id: sized once */
    kind_load(2, {A[1]}u);
    /* kind 3: FTData NULL (never admitted). */
    kind_load(4, {A[2]}u);
    sFT[4].p_file_main = NULL; /* main never loaded: ignored */
    kind_load(5, {A[3]}u);
    sMainVal[5] = NULL; /* *p_file_main NULL: ignored */
    kind_load(6, {A[4]}u);
    sFT[6].submotion = NULL; /* no submotion: ignored */
    /* kinds 7..8: absent. */
    raw_add({A[0]}u, 4000u);
    raw_add({A[1]}u, 4000u);
    raw_add({A[2]}u, 4000u);
    raw_add({A[3]}u, 4000u);
    raw_add({A[4]}u, 4000u);
    need = ndsR2AnimCacheSetupBytes();
    CHECK(need == {DUP_EXPECT}u);
    CHECK(sRaw[0].calls == 1u); /* duplicate paid one provider sizing */
}}

static void scenario_stream_raw_fallback(void)
{{
    sb32 ready;
    u32 sz;

    reset_state();
    /* Stream tier wins and matches the raw size the fallback would give. */
    stream_add({Q1}u, 4000u, 1);
    raw_add({Q1}u, 4000u);
    CHECK(ndsR2AnimCachePayloadBytes({Q1}u, &ready, &sz) == 4000u);
    CHECK(ready == TRUE && sz == 4000u);
    stream_set_ready({Q1}u, 0);
    CHECK(ndsR2AnimCachePayloadBytes({Q1}u, &ready, &sz) == 4000u);
    CHECK(ready == FALSE); /* raw fallback: same bytes warm loader gets */
    /* Generated tier answers when the stream misses. */
    stream_add({Q2}u, 0u, 0);
    gen_add({Q2}u, 2500u);
    raw_add({Q2}u, 9999u);
    CHECK(ndsR2AnimCachePayloadBytes({Q2}u, &ready, &sz) == 2500u);
    /* Stream outranks both lower tiers. */
    stream_add({Q3}u, 100u, 1);
    gen_add({Q3}u, 200u);
    raw_add({Q3}u, 300u);
    CHECK(ndsR2AnimCachePayloadBytes({Q3}u, &ready, &sz) == 100u);
    /* Nothing anywhere: zero, which SetupBytes rejects. */
    CHECK(ndsR2AnimCachePayloadBytes({Q0}u, &ready, &sz) == 0u);
    reset_state();
    kind_load(0, {Q1}u);
    stream_add({Q1}u, 4000u, 1);
    raw_add({Q1}u, 4000u);
    kind_load(1, {Q1}u); /* same clip on two kinds: distinct bytes once */
    CHECK(ndsR2AnimCacheSetupBytes() == 4000u);
}}

static void scenario_alignment(void)
{{
    reset_state();
    kind_load(0, {AL1}u);
    kind_load(1, {AL2}u);
    raw_add({AL1}u, 17u);
    raw_add({AL2}u, 33u);
    /* 17, then align_up(17) = 32 before the second clip. */
    CHECK(ndsR2AnimCacheSetupBytes() == {ALIGN_EXPECT}u);
    reset_state();
    kind_load(0, {AL1}u);
    raw_add({AL1}u, 17u);
    CHECK(ndsR2AnimCacheSetupBytes() == 17u);
}}

static void scenario_invalid_zero_overflow(void)
{{
    /* Invalid token: fail closed, no allocation. */
    reset_state();
    kind_load(0, {BAD}u);
    raw_add({BAD}u, 4000u);
    CHECK(ndsR2AnimCacheSetupBytes() == 0u);
    set_heap(600000u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == FALSE);
    CHECK(sMallocCalls == 0u);
    CHECK(gNdsR2AnimCacheArenaReserveFailCount == 1u);

    /* Non-fighter anim id: same refusal. */
    reset_state();
    kind_load(0, {NONFIGHTER}u);
    raw_add({NONFIGHTER}u, 4000u);
    CHECK(ndsR2AnimCacheSetupBytes() == 0u);
    set_heap(600000u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == FALSE);
    CHECK(sMallocCalls == 0u);

    /* Zero-size clip: rejected, never a zero-byte arena. */
    reset_state();
    kind_load(0, {ZERO_ID}u);
    CHECK(ndsR2AnimCacheSetupBytes() == 0u);
    set_heap(600000u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == FALSE);
    CHECK(sMallocCalls == 0u);

    /* One absurd provider size: overflow guard refuses. */
    reset_state();
    kind_load(0, {BIG_ID}u);
    raw_add({BIG_ID}u, (size_t)0x00000001FFFFFFFFull);
    CHECK(ndsR2AnimCacheSetupBytes() == 0u);
    set_heap(600000u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == FALSE);
    CHECK(sMallocCalls == 0u);

    /* Accumulation overflow across two clips: second add would wrap. */
    reset_state();
    kind_load(0, {O1}u);
    kind_load(1, {O2}u);
    raw_add({O1}u, (size_t)0xFFFFFFF0u);
    raw_add({O2}u, 32u);
    CHECK(ndsR2AnimCacheSetupBytes() == 0u);
    set_heap(600000u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == FALSE);
    CHECK(sMallocCalls == 0u);

    /* O1 alone is representable but unfundable: budget refuses, no alloc. */
    reset_state();
    kind_load(0, {O1}u);
    raw_add({O1}u, (size_t)0xFFFFFFF0u);
    CHECK(ndsR2AnimCacheSetupBytes() == 0xFFFFFFF0u);
    set_heap(600000u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == FALSE);
    CHECK(sMallocCalls == 0u);
}}

static void scenario_lease_no_fit_stale(void)
{{
    reset_state();
    roster_eight();
    set_heap({TOTAL8 + KEEP_FREE}u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == TRUE);
    CHECK(gNdsR2AnimCacheArenaReserveCount == 1u);

    /* Already-owned lease: second Ensure reuses the block, no malloc. */
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == TRUE);
    CHECK(sMallocCalls == 1u);
    CHECK(gNdsR2AnimCacheArenaReserveCount == 1u);

    /* Sixteen bytes short of clips + floor: refuse before any allocation. */
    reset_state();
    roster_eight();
    set_heap({TOTAL8 + KEEP_FREE - 16}u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == FALSE);
    CHECK(sMallocCalls == 0u);
    CHECK(gNdsR2AnimCacheArenaReserveFailCount == 1u);

    /* Heap rewound under us (generation bump): stale cache is dropped whole
     * then re-reserved fresh at the same derived size. */
    reset_state();
    roster_eight();
    set_heap({TOTAL8 + KEEP_FREE}u, 0u);
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == TRUE);
    gNdsTaskmanHeapGeneration++;
    gSYTaskmanGeneralHeap.ptr = gSYTaskmanGeneralHeap.start;
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == TRUE);
    CHECK(gNdsR2AnimCacheArenaInvalidations == 1u);
    CHECK(sResidencyDropCalls == 1u);
    CHECK(sNdsR2AnimCacheArenaBytes == {TOTAL8}u);
    CHECK(sMallocCalls == 2u);
    CHECK(gNdsR2AnimCacheArenaReserveCount == 2u);
}}

int main(void)
{{
    scenario_nine_exceeds_fixed();
    scenario_duplicates_and_unloaded();
    scenario_stream_raw_fallback();
    scenario_alignment();
    scenario_invalid_zero_overflow();
    scenario_lease_no_fit_stale();
    return 0;
}}
"""


def control_main():
    """Negative control: the restored fixed carve on the nine-clip roster."""
    return f"""
int main(void)
{{
    u32 need;
    u32 used = 0u;
    u32 k;
    int overflow = 0;

    reset_state();
    kind_load(0, {A[0]}u);
    kind_load(1, {A[1]}u);
    kind_load(2, {A[2]}u);
    kind_load(3, {A[3]}u);
    kind_load(4, {A[4]}u);
    kind_load(5, {A[5]}u);
    kind_load(6, {A[6]}u);
    kind_load(7, {A[7]}u);
    kind_load(8, {PIKA}u);
    raw_add({A[0]}u, 4000u);
    raw_add({A[1]}u, 4000u);
    raw_add({A[2]}u, 4000u);
    raw_add({A[3]}u, 4000u);
    raw_add({A[4]}u, 4000u);
    raw_add({A[5]}u, 4000u);
    raw_add({A[6]}u, 4128u);
    raw_add({A[7]}u, 4496u);
    raw_add({PIKA}u, {PIKA_SIZE}u);
    stream_add({A[0]}u, 4000u, 1);
    gen_add({Q2}u, 2500u);
    {{
        sb32 ctl_ready;
        u32 ctl_sz;
        CHECK(ndsR2AnimCachePayloadBytes({A[0]}u, &ctl_ready, &ctl_sz) == 4000u);
        CHECK(ctl_ready == TRUE);
        stream_set_ready({A[0]}u, 0);
        CHECK(ndsR2AnimCachePayloadBytes({A[0]}u, &ctl_ready, &ctl_sz) == 4000u);
        CHECK(ctl_ready == FALSE);
        CHECK(ndsR2AnimCachePayloadBytes({Q2}u, &ctl_ready, &ctl_sz) == 2500u);
    }}
    set_heap({TOTAL9 + KEEP_FREE}u, 0u);
    /* The REAL sizer still reports the roster need ... */
    need = ndsR2AnimCacheSetupBytes();
    CHECK(need == {TOTAL9}u);
    CHECK({FIXED}u < need); /* ... which the fixed carve cannot cover. */
    /* ... while the patched EnsureSetup reserves only the fixed carve. */
    CHECK(ndsR2AnimCacheArenaEnsureSetup() == TRUE);
    CHECK(sNdsR2AnimCacheArenaBytes == {FIXED}u);
    /* Replaying the nine clips into the fixed arena overflows: this is the
     * shipped bug (Pikachu's clip had nowhere to live). */
    for (k = 0u; k < 9u; k++)
    {{
        used = ((used + 15u) & ~15u) + kCtlSizes[k];
        if (used > {FIXED}u)
        {{
            overflow = 1;
        }}
    }}
    CHECK(overflow == 1);
    return 0;
}}
"""


CTL_SIZES = f"""
static const u32 kCtlSizes[9] = {{ {", ".join(str(v) for v in (*S8, PIKA_SIZE))} }};
"""


def build_source(control=False):
    real = extract_real()
    ensure = real["ensure_setup"]
    if control:
        anchor = "    bytes = ndsR2AnimCacheSetupBytes();"
        if ensure.count(anchor) != 1:
            raise AssertionError("negative-control anchor not unique")
        ensure = ensure.replace(
            anchor, "    bytes = 32768u; /* NEGATIVE CONTROL: fixed carve */")
    return "\n".join([
        HARNESS_HEAD,
        CTL_SIZES if control else "",
        real["would_fit"],
        real["still_owned"],
        real["drop_for_reset"],
        real["validate"],
        real["payload"],
        real["setup"],
        ensure,
        control_main() if control else host_main(),
    ])


def bounded(text, limit=2000):
    text = (text or "").strip()
    return text if len(text) <= limit else text[:limit] + "... [truncated]"


class CssCacheSizingTest(unittest.TestCase):
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
            source = Path(directory) / f"css_cache_{tag}.c"
            program = Path(directory) / f"css_cache_{tag}.exe"
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

    def test_real_setup_sizer_functions(self):
        self.compile_and_run(build_source(control=False), "real")

    def test_negative_control_fixed_carve_underfits(self):
        """Restored fixed 32,768 B must fail the nine-clip roster.

        If this control ever FAILS by fitting all nine clips, the control
        patch no longer models the shipped code -- fix the patch, not the
        source.
        """
        self.compile_and_run(build_source(control=True), "control")


if __name__ == "__main__":
    unittest.main()
