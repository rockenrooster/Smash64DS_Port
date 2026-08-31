#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <nds/battle_playable_static_textures.h>
#if NDS_R2_IMPACT_WAVE_NATIVE
#include <nds/nds_effects.h>
#endif
#if NDS_R2_FOX_GUN_OVERLAY
#include <nds/nds_fox_gun.h>
#endif
#include <nds/nds_gbi_decode.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_r2_hwmath_unit.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/nds_startup.h>
#include <nds/nds_task37_itcm.h>
#include <nds/nds_task49_gx_differ.h>
#if NDS_R2_PARTICLE_RUNTIME
#include <nds/generated/nds_particle_banks.generated.h>
#endif

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#if NDS_TASK49_GX_DIFFER
/* Forward declaration: the lean ndsRendererTask29GXRecord funnel below needs
 * the current runtime owner, which is defined later in this TU. The whole
 * block (comment included) is compiled only when the differ is armed, so the
 * default-off build stays byte-identical to master. */
static NDSRendererProfileOwner sNdsRendererRuntimeOwner;
#endif

#ifndef NDS_SCENE_MIP_CACHE_LAB
#define NDS_SCENE_MIP_CACHE_LAB 0
#endif

#ifndef NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT
#define NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT 0
#endif

#ifndef NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY
#define NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY 0
#endif

/* The measured mode-163 renderer wins in ARM state on retail hardware.  Its
 * six zero-wait ITCM paths retain explicit ARM state and O3; host fixtures
 * retain only the portable optimization annotations. */
#if defined(__arm__)
#define NDS_RENDERER_HOT_CODE \
    __attribute__((hot, optimize("O3"), target("arm"), section(".itcm")))
#define NDS_RENDERER_FAST_RUN_CODE \
    __attribute__((noinline, optimize("O3"), target("arm")))
#define NDS_RENDERER_NATIVE_FIGHTER_CODE \
    __attribute__((noinline, hot, optimize("O3"), target("arm"), \
                   section(".itcm.native_fighter")))
#define NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE \
    __attribute__((noinline, hot, optimize("O3"), target("arm")))
#else
#define NDS_RENDERER_HOT_CODE __attribute__((hot, optimize("O3")))
#define NDS_RENDERER_FAST_RUN_CODE \
    __attribute__((noinline, optimize("O3")))
#define NDS_RENDERER_NATIVE_FIGHTER_CODE \
    __attribute__((noinline, hot, optimize("O3")))
#define NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE \
    __attribute__((noinline, hot, optimize("O3")))
#endif

/* Task 82 repack. Placement and nothing else, in both directions.
 *
 * include/nds/nds_task37_itcm.h states the rule this obeys: a task that moved
 * code and recompiled it differently at the same time could not attribute its
 * own result. Admission adds a section attribute and no more; eviction drops
 * the section while keeping hot/O3/ARM exactly as NDS_RENDERER_HOT_CODE had
 * them. Neither direction changes an emitted instruction, only where it lives.
 *
 * The pack this replaces was measured on 2026-07-22 and drifted: 27 of the 71
 * .itcm residents no longer execute in the battle window (Task 82 E0). ITCM is
 * a hard 32 KiB and was 32,596 bytes full, so admitting anything required
 * evicting something first.
 *
 * Exactly one eviction, on the owner's decision: the animated-CI4 texel path.
 * It measures zero cycles because PROJECT_GOAL.md freezes Dream Land water at
 * source frame 0, so this is a stage-specific specialisation with a named
 * reason -- not a general claim that the path is dead. A stage with live water
 * needs this re-packed, and the checkers were narrowed from six paths to five
 * rather than deleted so that stays visible. */
#if NDS_TASK82_ITCM_REPACK && defined(__arm__)
#define NDS_TASK82_ITCM_CODE __attribute__((section(".itcm")))
#define NDS_TASK82_EVICTED_HOT_CODE     __attribute__((hot, optimize("O3"), target("arm")))
#else
#define NDS_TASK82_ITCM_CODE
#define NDS_TASK82_EVICTED_HOT_CODE NDS_RENDERER_HOT_CODE
#endif

/* Campaign 01 re-knapsack, 2026-08-17. See include/nds/nds_task37_itcm.h for
 * the ranking and the measured rents. The generic display-list interpreter and
 * its submit chain are the fallback renderer that the native stage and fighter
 * owner paths replaced; GX compose retired the affine multiply on top of that.
 * They stay exact and stay callable -- they move to main RAM, they do not go
 * away -- and the bytes go to residents the shipping census actually measures.
 *
 * Placement only, in both directions: NDS_R2_ITCM_PACK2_EVICTED_CODE is
 * NDS_RENDERER_HOT_CODE minus the section. */
#if NDS_R2_ITCM_PACK2 && defined(__arm__)
#define NDS_R2_ITCM_PACK2_EVICTED_CODE \
    __attribute__((hot, optimize("O3"), target("arm")))
#define NDS_R2_ITCM_PACK2_EVICTED_PLAIN_CODE
#else
#define NDS_R2_ITCM_PACK2_EVICTED_CODE NDS_RENDERER_HOT_CODE
#define NDS_R2_ITCM_PACK2_EVICTED_PLAIN_CODE NDS_TASK82_ITCM_CODE
#endif

/* R2-03 E26 SIZING ARM ONLY -- THE SHIPPED ROM'S ITCM PACK IS UNTOUCHED.
 *
 * docs/P1_EXECUTION_BOARD.md names the delta-redundancy census as the thing that
 * sizes the state-span bake and records it as blocked: "does not build: region
 * 'itcm' overflowed by 64 bytes". On this tree it overflows by **616**, because
 * the census block is inline in ndsRendererNativeApplyStateDelta and
 * ndsRendererNativeApplyStateSpan, both of which are ITCM residents, so arming
 * the diagnostic grows ITCM itself. The board's own remedy is "evict a resident
 * for the diagnostic arm before designing the bake".
 *
 * ndsRendererScanList is that resident: 7,728 bytes, the largest in ITCM, and it
 * is the GENERIC display-list interpreter that the native fighter owner path
 * exists to replace -- ndsRendererExecuteNativeFighterOwnerProduction does not
 * call it. Evicting it frees twelve times what the census needs.
 *
 * WHAT THIS ARM MAY AND MAY NOT BE READ FOR. The census's redundancy figures
 * (gNdsR2SpanDeltaRepeats, gNdsR2DeltaEffectCounts, gNdsR2SpanIdenticalOperands)
 * are COUNTS, and a count does not depend on where the code lives, so they are
 * exactly as valid here as in any build. The Task 91 tick brackets in the same
 * arm are NOT: moving 7,728 bytes out of ITCM changes instruction fetch for
 * everything that shares it. Size the bake from the counts; take ticks from an
 * unevicted arm.
 *
 * Placement only, in the spirit of Task 82: NDS_TASK82_EVICTED_HOT_CODE drops
 * the section and keeps hot/O3/ARM, so no emitted instruction changes. */
/* 2026-08-17: the re-knapsack makes this eviction unconditional. The shipping
 * census measures ndsRendererScanList at 599 tk/fr on the gate's own rank-80
 * frames over 6,188 bytes -- 0.1 tk/byte, the second-cheapest rent in a table
 * whose top residents run 100,000 cyc/byte -- so the census arm and the
 * shipping arm now want the same placement for the same measured reason. */
#if NDS_TASK91_DRAW_PHASE_CENSUS
#define NDS_R2_CENSUS_EVICTED_CODE NDS_TASK82_EVICTED_HOT_CODE
#else
#define NDS_R2_CENSUS_EVICTED_CODE NDS_R2_ITCM_PACK2_EVICTED_CODE
#endif

/* R2-03 E46. ndsRendererNativeApplyStateDelta is already ITCM-resident, but the
 * helpers its switch calls are not: in the census ELF the switch sits at
 * 0x01ff9934 while ndsRendererRecordSetTile is at 0x0200d4e8 and
 * ndsRendererNativeApplyStateSpan -- the loop itself -- at 0x02003a14. So every
 * one of the before-span's 134.5 applications a frame leaves zero-wait ITCM for
 * icache-served main RAM and comes back.
 *
 * E45 left ~186 ticks per application unexplained after ruling out the tile
 * republish (~23 ticks, E44), the invalidation macro (one store) and the span
 * entry (~33 ticks). Instruction fetch is what is left, and the repo emulator
 * models icache, so it is measurable. The whole path is ~1,088 bytes against
 * 2,912 free in .itcm.
 *
 * Placement and nothing else, in the spirit of Task 82 -- no behaviour changes,
 * so a win is attributable to fetch and a null refutes the mechanism. */
#if NDS_R2_DELTA_PATH_ITCM
#define NDS_R2_DELTA_PATH_CODE NDS_TASK82_ITCM_CODE
#else
#define NDS_R2_DELTA_PATH_CODE
#endif

/* Profiles 0/1 publish the shipping contract through the compact frame
 * summary.  Profile 1 is the low-frequency O2 coarse build, so it must not
 * maintain the generic per-command proof ledger either.  Profile 2 retains
 * the full forensic counters and semantic observer. */
#if NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_RECORD_PROOF_ONLY(statement) ((void)0)
#else
#define NDS_RENDERER_RECORD_PROOF_ONLY(statement) \
    do { statement; } while (0)
#endif

#if NDS_RENDERER_HW_TRIANGLES
#include <math.h>
#include <nds.h>
#include <nds/arm9/postest.h>
#endif

#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX)
#define NDS_RENDERER_BENCHMARK_SINK_WORDS 1024u
#define NDS_RENDERER_BENCHMARK_SINK_MASK \
    (NDS_RENDERER_BENCHMARK_SINK_WORDS - 1u)
#define NDS_RENDERER_BENCHMARK_SEGMENT0_TRACE_WORDS 3072u
#define NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT 26u

u32 gNdsRendererBenchmarkSink[
    NDS_RENDERER_BENCHMARK_SINK_WORDS] __attribute__((aligned(32)));
volatile u32 gNdsRendererBenchmarkSinkCursor;
volatile u32 gNdsRendererBenchmarkSinkWordCount;
volatile u32 gNdsRendererBenchmarkSinkOwnerWords[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
volatile u32 gNdsRendererBenchmarkSinkCalibrationWords;
volatile u32 gNdsRendererBenchmarkSinkCalibrationTicks;
volatile u32 gNdsRendererBenchmarkSinkHashA;
volatile u32 gNdsRendererBenchmarkSinkHashB;
volatile u32 gNdsRendererBenchmarkSegment0SinkWords;
volatile u32 gNdsRendererBenchmarkSegment0SinkHashA;
volatile u32 gNdsRendererBenchmarkSegment0SinkHashB;
volatile u32 gNdsRendererBenchmarkSegment0SinkArmFaults;
u32 gNdsRendererBenchmarkSegment0Trace[
    NDS_RENDERER_BENCHMARK_SEGMENT0_TRACE_WORDS] __attribute__((aligned(32)));
volatile u32 gNdsRendererBenchmarkSegment0RunWords[
    NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT];
volatile u32 gNdsRendererBenchmarkSegment0RunHashA[
    NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT];
volatile u32 gNdsRendererBenchmarkSegment0RunHashB[
    NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT];
volatile u32 gNdsRendererBenchmarkSegment0RunRawTextureName[
    NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT];
volatile u32 gNdsRendererBenchmarkSegment0RunTextureEpochPlus1[
    NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT];
volatile u32 gNdsRendererBenchmarkSegment0RunTextureImage[
    NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT];
volatile u32 gNdsRendererBenchmarkSegment0RunTextureTlut[
    NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT];
volatile u32 gNdsRendererBenchmarkSegment0RunTextureKeyHashA[
    NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT];
volatile u32 gNdsRendererBenchmarkSegment0RunTextureKeyHashB[
    NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT];
volatile u32 gNdsRendererBenchmarkSegment0RunTextureDescriptor[
    NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT];
volatile u32 gNdsRendererBenchmarkSegment0RunTextureParams[
    NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT];
static u32 sNdsRendererBenchmarkSinkCursor;
static u32 sNdsRendererBenchmarkSinkWordCount;
static u32 sNdsRendererBenchmarkSinkLastOwnerCursor;
static u32 sNdsRendererBenchmarkSinkOwnerWords[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
static u32 sNdsRendererBenchmarkSinkHashA;
static u32 sNdsRendererBenchmarkSinkHashB;
static u32 sNdsRendererBenchmarkSegment0SinkWords;
static u32 sNdsRendererBenchmarkSegment0SinkHashA;
static u32 sNdsRendererBenchmarkSegment0SinkHashB;
static u32 sNdsRendererBenchmarkSegment0SinkActive;
static u32 sNdsRendererBenchmarkSegment0SinkArmFaults;
static u32 sNdsRendererBenchmarkSegment0TextureEpochPlus1;
static u32 sNdsRendererBenchmarkSegment0TextureEpochSourceOffset;
static u32 sNdsRendererBenchmarkSegment0TextureEpochMetadata;
static u32 sNdsRendererBenchmarkSegment0TextureImage;
static u32 sNdsRendererBenchmarkSegment0TextureTlut;
static u32 sNdsRendererBenchmarkSegment0TextureTexel1;
static u32 sNdsRendererBenchmarkSegment0TextureKeyHashA;
static u32 sNdsRendererBenchmarkSegment0TextureKeyHashB;
static u32 sNdsRendererBenchmarkSegment0TextureDescriptor;
static u32 sNdsRendererBenchmarkSegment0TextureFlags;
static u32 sNdsRendererBenchmarkSegment0TextureParams;
static u32 sNdsRendererBenchmarkSegment0TexturePolyFmt;
static u32 sNdsRendererBenchmarkSegment0TextureBinding;
static u32 sNdsRendererBenchmarkSegment0TextureValid;
static const void *sNdsRendererBenchmarkSegment0AssetBases[
    NDS_RENDERER_NATIVE_STAGE_ASSET_COUNT];
static u32 sNdsRendererBenchmarkFakeTextureName = 1u;
static u32 sNdsRendererBenchmarkTextureParameter;

static inline u32 ndsRendererBenchmarkSinkHashWordA(u32 hash, u32 value)
{
    return (hash ^ value) * 16777619u;
}

static inline u32 ndsRendererBenchmarkSinkHashWordB(u32 hash, u32 value)
{
    hash ^= value + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    hash = (hash << 7) | (hash >> 25);
    return hash * 0x85ebca6bu;
}

static inline void ndsRendererBenchmarkSinkWord(u32 value)
{
    gNdsRendererBenchmarkSink[
        sNdsRendererBenchmarkSinkCursor &
        NDS_RENDERER_BENCHMARK_SINK_MASK] = value;
    sNdsRendererBenchmarkSinkCursor++;
    sNdsRendererBenchmarkSinkWordCount++;
    sNdsRendererBenchmarkSinkHashA = ndsRendererBenchmarkSinkHashWordA(
        sNdsRendererBenchmarkSinkHashA, value);
    sNdsRendererBenchmarkSinkHashB = ndsRendererBenchmarkSinkHashWordB(
        sNdsRendererBenchmarkSinkHashB, value);
    if (sNdsRendererBenchmarkSegment0SinkActive != 0u)
    {
        if (sNdsRendererBenchmarkSegment0SinkWords <
            NDS_RENDERER_BENCHMARK_SEGMENT0_TRACE_WORDS)
        {
            gNdsRendererBenchmarkSegment0Trace[
                sNdsRendererBenchmarkSegment0SinkWords] = value;
        }
        else
        {
            sNdsRendererBenchmarkSegment0SinkArmFaults++;
        }
        sNdsRendererBenchmarkSegment0SinkWords++;
        sNdsRendererBenchmarkSegment0SinkHashA =
            ndsRendererBenchmarkSinkHashWordA(
                sNdsRendererBenchmarkSegment0SinkHashA, value);
        sNdsRendererBenchmarkSegment0SinkHashB =
            ndsRendererBenchmarkSinkHashWordB(
                sNdsRendererBenchmarkSegment0SinkHashB, value);
    }
}

static void ndsRendererBenchmarkSegment0SinkBegin(void)
{
    if (sNdsRendererBenchmarkSegment0SinkActive != 0u)
    {
        sNdsRendererBenchmarkSegment0SinkArmFaults++;
    }
    sNdsRendererBenchmarkSegment0SinkWords = 0u;
    sNdsRendererBenchmarkSegment0SinkHashA = 2166136261u;
    sNdsRendererBenchmarkSegment0SinkHashB = 0x9e3779b9u;
    sNdsRendererBenchmarkSegment0SinkActive = TRUE;
    sNdsRendererBenchmarkSegment0TextureEpochPlus1 = 0u;
    sNdsRendererBenchmarkSegment0TextureEpochSourceOffset = 0u;
    sNdsRendererBenchmarkSegment0TextureEpochMetadata = 0u;
    sNdsRendererBenchmarkSegment0TextureImage = 0u;
    sNdsRendererBenchmarkSegment0TextureTlut = 0u;
    sNdsRendererBenchmarkSegment0TextureTexel1 = 0u;
    sNdsRendererBenchmarkSegment0TextureKeyHashA = 0u;
    sNdsRendererBenchmarkSegment0TextureKeyHashB = 0u;
    sNdsRendererBenchmarkSegment0TextureDescriptor = 0u;
    sNdsRendererBenchmarkSegment0TextureFlags = 0u;
    sNdsRendererBenchmarkSegment0TextureParams = 0u;
    sNdsRendererBenchmarkSegment0TexturePolyFmt = 0u;
    sNdsRendererBenchmarkSegment0TextureBinding = 0u;
    sNdsRendererBenchmarkSegment0TextureValid = FALSE;
    memset((void *)gNdsRendererBenchmarkSegment0RunWords, 0,
           sizeof(gNdsRendererBenchmarkSegment0RunWords));
    memset((void *)gNdsRendererBenchmarkSegment0RunHashA, 0,
           sizeof(gNdsRendererBenchmarkSegment0RunHashA));
    memset((void *)gNdsRendererBenchmarkSegment0RunHashB, 0,
           sizeof(gNdsRendererBenchmarkSegment0RunHashB));
    memset((void *)gNdsRendererBenchmarkSegment0RunRawTextureName, 0,
           sizeof(gNdsRendererBenchmarkSegment0RunRawTextureName));
    memset((void *)gNdsRendererBenchmarkSegment0RunTextureEpochPlus1, 0,
           sizeof(gNdsRendererBenchmarkSegment0RunTextureEpochPlus1));
    memset((void *)gNdsRendererBenchmarkSegment0RunTextureImage, 0,
           sizeof(gNdsRendererBenchmarkSegment0RunTextureImage));
    memset((void *)gNdsRendererBenchmarkSegment0RunTextureTlut, 0,
           sizeof(gNdsRendererBenchmarkSegment0RunTextureTlut));
    memset((void *)gNdsRendererBenchmarkSegment0RunTextureKeyHashA, 0,
           sizeof(gNdsRendererBenchmarkSegment0RunTextureKeyHashA));
    memset((void *)gNdsRendererBenchmarkSegment0RunTextureKeyHashB, 0,
           sizeof(gNdsRendererBenchmarkSegment0RunTextureKeyHashB));
    memset((void *)gNdsRendererBenchmarkSegment0RunTextureDescriptor, 0,
           sizeof(gNdsRendererBenchmarkSegment0RunTextureDescriptor));
    memset((void *)gNdsRendererBenchmarkSegment0RunTextureParams, 0,
           sizeof(gNdsRendererBenchmarkSegment0RunTextureParams));
}

static void ndsRendererBenchmarkSegment0SinkEnd(void)
{
    if (sNdsRendererBenchmarkSegment0SinkActive == 0u)
    {
        sNdsRendererBenchmarkSegment0SinkArmFaults++;
    }
    sNdsRendererBenchmarkSegment0SinkActive = FALSE;
}

static inline void ndsRendererBenchmarkGlEnable(int bits)
{
    ndsRendererBenchmarkSinkWord(0xe1000000u | (u32)bits);
}

static inline void ndsRendererBenchmarkGlDisable(int bits)
{
    ndsRendererBenchmarkSinkWord(0xd1000000u | (u32)bits);
}

static inline void ndsRendererBenchmarkGlAlphaFunc(int threshold)
{
    ndsRendererBenchmarkSinkWord((u32)threshold);
}

static inline void ndsRendererBenchmarkGlFogDensity(int index, int density)
{
    ndsRendererBenchmarkSinkWord(
        ((u32)index & 0xffu) | (((u32)density & 0xffu) << 8));
}

static inline void ndsRendererBenchmarkGlFogShift(int shift)
{
    ndsRendererBenchmarkSinkWord((u32)shift);
}

static inline void ndsRendererBenchmarkGlFogOffset(int offset)
{
    ndsRendererBenchmarkSinkWord((u32)offset);
}

static inline void ndsRendererBenchmarkGlFogColor(
    u8 red, u8 green, u8 blue, u8 alpha)
{
    ndsRendererBenchmarkSinkWord(
        (u32)red | ((u32)green << 5) | ((u32)blue << 10) |
        ((u32)alpha << 15));
}

static inline void ndsRendererBenchmarkGlTexParameter(int target, int param)
{
    (void)target;
    sNdsRendererBenchmarkTextureParameter = (u32)param;
    ndsRendererBenchmarkSinkWord((u32)param);
}

static inline u32 ndsRendererBenchmarkGlGetTexParameter(void)
{
    return sNdsRendererBenchmarkTextureParameter;
}

static inline void ndsRendererBenchmarkGlBindTexture(int target, int name)
{
    (void)target;
    if (sNdsRendererBenchmarkSegment0SinkActive != 0u)
    {
        /* A libnds texture name is an opaque CPU handle, not a GX word.
         * Segment 0 instead records the certified epoch and the actual
         * pointer-normalized prepared cache key/descriptor. */
        if (sNdsRendererBenchmarkSegment0TextureValid == FALSE)
        {
            sNdsRendererBenchmarkSegment0SinkArmFaults++;
        }
        ndsRendererBenchmarkSinkWord(
            0xb1000000u |
            (sNdsRendererBenchmarkSegment0TextureEpochPlus1 & 0xffffu));
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TextureEpochSourceOffset);
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TextureEpochMetadata);
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TextureImage);
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TextureTlut);
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TextureTexel1);
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TextureDescriptor);
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TextureFlags);
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TextureParams);
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TexturePolyFmt);
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TextureBinding);
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TextureKeyHashA);
        ndsRendererBenchmarkSinkWord(
            sNdsRendererBenchmarkSegment0TextureKeyHashB);
        return;
    }
    ndsRendererBenchmarkSinkWord((u32)name);
}

static inline int ndsRendererBenchmarkGlGenTextures(int count, int *names)
{
    int i;

    if ((count <= 0) || (names == NULL))
    {
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        names[i] = (int)sNdsRendererBenchmarkFakeTextureName++;
        ndsRendererBenchmarkSinkWord((u32)names[i]);
    }
    return 1;
}

static inline int ndsRendererBenchmarkGlDeleteTextures(int count, int *names)
{
    int i;

    if ((count <= 0) || (names == NULL))
    {
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        ndsRendererBenchmarkSinkWord((u32)names[i]);
    }
    return 1;
}

static inline int ndsRendererBenchmarkGlTexImage2D(
    int target, int empty1, GL_TEXTURE_TYPE_ENUM type,
    int size_x, int size_y, int empty2, int params, const void *texture)
{
    (void)target;
    (void)empty1;
    (void)empty2;
    (void)texture;
    sNdsRendererBenchmarkTextureParameter = (u32)params;
    ndsRendererBenchmarkSinkWord((u32)type);
    ndsRendererBenchmarkSinkWord(
        ((u32)size_x & 0xffffu) | ((u32)size_y << 16));
    ndsRendererBenchmarkSinkWord((u32)params);
    return 1;
}

static inline void ndsRendererBenchmarkGlMatrixMode(int mode)
{
    ndsRendererBenchmarkSinkWord((u32)mode);
}

static inline void ndsRendererBenchmarkGlLoadMatrix4x4(const m4x4 *matrix)
{
    const u32 *words = (const u32 *)matrix;
    u32 i;

    for (i = 0u; i < 16u; i++)
    {
        ndsRendererBenchmarkSinkWord(words[i]);
    }
}

static inline void ndsRendererBenchmarkGlVertex3v16(v16 x, v16 y, v16 z)
{
    ndsRendererBenchmarkSinkWord(
        (u32)(u16)x | ((u32)(u16)y << 16));
    ndsRendererBenchmarkSinkWord((u32)(u16)z);
}

static inline void ndsRendererBenchmarkGlPolyFmt(u32 params)
{
    ndsRendererBenchmarkSinkWord(params);
}

static inline void ndsRendererBenchmarkGlBegin(GL_GLBEGIN_ENUM mode)
{
    ndsRendererBenchmarkSinkWord((u32)mode);
}

static inline void ndsRendererBenchmarkGlColor(u16 color)
{
    ndsRendererBenchmarkSinkWord((u32)color);
}

static inline void ndsRendererBenchmarkGlTexCoord2t16(t16 s, t16 t)
{
    ndsRendererBenchmarkSinkWord(
        (u32)(u16)s | ((u32)(u16)t << 16));
}

#define glEnable ndsRendererBenchmarkGlEnable
#define glDisable ndsRendererBenchmarkGlDisable
#define glAlphaFunc ndsRendererBenchmarkGlAlphaFunc
#define glFogDensity ndsRendererBenchmarkGlFogDensity
#define glFogShift ndsRendererBenchmarkGlFogShift
#define glFogOffset ndsRendererBenchmarkGlFogOffset
#define glFogColor ndsRendererBenchmarkGlFogColor
#define glTexParameter ndsRendererBenchmarkGlTexParameter
#define glGetTexParameter ndsRendererBenchmarkGlGetTexParameter
#define glBindTexture ndsRendererBenchmarkGlBindTexture
#define glGenTextures ndsRendererBenchmarkGlGenTextures
#define glDeleteTextures ndsRendererBenchmarkGlDeleteTextures
#define glTexImage2D ndsRendererBenchmarkGlTexImage2D
#define glMatrixMode ndsRendererBenchmarkGlMatrixMode
#define glLoadMatrix4x4 ndsRendererBenchmarkGlLoadMatrix4x4
#define glVertex3v16 ndsRendererBenchmarkGlVertex3v16
#define glPolyFmt ndsRendererBenchmarkGlPolyFmt
#define glBegin ndsRendererBenchmarkGlBegin
#define glColor ndsRendererBenchmarkGlColor
#define glTexCoord2t16 ndsRendererBenchmarkGlTexCoord2t16
#endif

#if (NDS_TASK29_GX_CENSUS || NDS_TASK34_STAGE_STREAM_CENSUS || \
     (NDS_TASK36_HW_COMPOSE == 2)) && \
    NDS_RENDERER_HW_TRIANGLES
#define NDS_TASK29_GX_MAX_WORDS 16u
#define NDS_TASK29_GX_CENSUS_CODE \
    __attribute__((noinline, noclone, cold, optimize("Os"), \
                   section(".text.task29_gx_census")))

#if NDS_TASK34_STAGE_STREAM_CENSUS
volatile u32 gNdsTask34StageStreamFrame;
volatile u32 gNdsTask34StageStreamCaptureEnabled;
volatile u32 gNdsTask34StageStreamEntryCount;
volatile u32 gNdsTask34StageStreamWordCount;
volatile u32 gNdsTask34StageStreamOverflowCount;
volatile u32 gNdsTask34StageStreamFaultCount;
volatile NDSRendererTask34StageStreamEntry
    gNdsTask34StageStreamEntries[NDS_TASK34_STAGE_STREAM_ENTRY_CAPACITY];
volatile u32
    gNdsTask34StageStreamWords[NDS_TASK34_STAGE_STREAM_WORD_CAPACITY];

static u32 sNdsTask34StageStreamDObj = NDS_TASK34_STAGE_STREAM_DOBJ_NONE;
static u32 sNdsTask34StageStreamSegment;
static u32 sNdsTask34StageStreamActive;

static void NDS_TASK29_GX_CENSUS_CODE
ndsRendererTask34StageStreamRecord(
    NDSRendererTask29GXClass command_class,
    const u32 *words,
    u32 word_count)
{
    u32 entry_index;
    u32 first_word;
    u32 word_index;
    volatile NDSRendererTask34StageStreamEntry *entry;

    if ((gNdsTask34StageStreamCaptureEnabled == FALSE) ||
        (sNdsTask34StageStreamActive == FALSE))
    {
        return;
    }
    entry_index = gNdsTask34StageStreamEntryCount;
    first_word = gNdsTask34StageStreamWordCount;
    if ((sNdsTask34StageStreamDObj ==
         NDS_TASK34_STAGE_STREAM_DOBJ_NONE) ||
        ((u32)command_class >= NDS_TASK29_GX_CLASS_COUNT) ||
        (word_count > NDS_TASK29_GX_MAX_WORDS) ||
        ((word_count != 0u) && (words == NULL)))
    {
        gNdsTask34StageStreamFaultCount++;
        return;
    }
    if ((entry_index >= NDS_TASK34_STAGE_STREAM_ENTRY_CAPACITY) ||
        (first_word + word_count > NDS_TASK34_STAGE_STREAM_WORD_CAPACITY))
    {
        gNdsTask34StageStreamOverflowCount++;
        return;
    }
    entry = &gNdsTask34StageStreamEntries[entry_index];
    entry->word_offset = (u16)first_word;
    entry->dobj_index = (u16)sNdsTask34StageStreamDObj;
    entry->command_class = (u8)command_class;
    entry->word_count = (u8)word_count;
    entry->segment_index = (u8)sNdsTask34StageStreamSegment;
    entry->reserved = 0u;
    for (word_index = 0u; word_index < word_count; word_index++)
    {
        gNdsTask34StageStreamWords[first_word + word_index] = words[word_index];
    }
    gNdsTask34StageStreamEntryCount++;
    gNdsTask34StageStreamWordCount += word_count;
}

void NDS_TASK29_GX_CENSUS_CODE
ndsRendererTask34StageStreamBeginSegment(u32 segment_index)
{
    if (gNdsTask34StageStreamCaptureEnabled == FALSE)
    {
        return;
    }
    if (gNdsTask34StageStreamFrame != gNdsRendererProfileFrameCount)
    {
        gNdsTask34StageStreamFrame = gNdsRendererProfileFrameCount;
        gNdsTask34StageStreamEntryCount = 0u;
        gNdsTask34StageStreamWordCount = 0u;
        gNdsTask34StageStreamOverflowCount = 0u;
        gNdsTask34StageStreamFaultCount = 0u;
        sNdsTask34StageStreamDObj = NDS_TASK34_STAGE_STREAM_DOBJ_NONE;
        sNdsTask34StageStreamActive = FALSE;
    }
    if (sNdsTask34StageStreamActive != FALSE)
    {
        gNdsTask34StageStreamFaultCount++;
    }
    sNdsTask34StageStreamSegment = segment_index;
    sNdsTask34StageStreamDObj = NDS_TASK34_STAGE_STREAM_DOBJ_NONE;
    sNdsTask34StageStreamActive = TRUE;
}

void NDS_TASK29_GX_CENSUS_CODE
ndsRendererTask34StageStreamSetDObj(u32 dobj_index)
{
    if (gNdsTask34StageStreamCaptureEnabled == FALSE)
    {
        return;
    }
    if (sNdsTask34StageStreamActive == FALSE)
    {
        gNdsTask34StageStreamFaultCount++;
        return;
    }
    sNdsTask34StageStreamDObj = dobj_index;
}

void NDS_TASK29_GX_CENSUS_CODE
ndsRendererTask34StageStreamEndSegment(void)
{
    if (gNdsTask34StageStreamCaptureEnabled == FALSE)
    {
        return;
    }
    if (sNdsTask34StageStreamActive == FALSE)
    {
        gNdsTask34StageStreamFaultCount++;
        return;
    }
    sNdsTask34StageStreamDObj = NDS_TASK34_STAGE_STREAM_DOBJ_NONE;
    sNdsTask34StageStreamActive = FALSE;
}
#endif
#if NDS_TICK_HUD
/* G3 STEP 1 -- THE EFFECT GX STREAM CAPTURE, and the question it exists to
 * settle before a packet builder is written: what SHAPE is the GX stream an
 * effect display list emits, and can a captured copy of it be replayed?
 *
 * This matters because the precompiled-packet mechanism the board's G3 row
 * describes ALREADY EXISTS in this file as Task 36 replay
 * (NDSRendererTask36ReplayOwner, below): a captured word stream, a fixed static
 * arena, per-run word offsets, and a segment admission mask. What Task 36 also
 * carries is the reason it admits only three stage segments, recorded at its
 * NDS_TASK36_REPLAY_SEGMENT_MASK -- a RIGID binding records PUSH + MULT4x4 of a
 * constant world under a camera the segment bracket loads live each frame, so
 * it replays; a DYNAMIC binding records LOAD4x4 per triangle of
 * projection x view x model, so replaying it pins that geometry to the camera
 * the capture frame happened to have. R2-02 E3 and E4 widened that mask twice
 * and produced a smear of specks across Whispy's trunk and then a lost flower
 * bed.
 *
 * Effect instances MOVE -- each one spawns somewhere else and fades on its own
 * clock -- so E3's failure is the DEFAULT outcome for a verbatim effect packet.
 * The design survives only if the stream carries one patchable matrix per list
 * rather than one per triangle, and that count is what this instrument returns.
 *
 * Geometry, colour and matrix words are hashed SEPARATELY, and the split is the
 * experiment rather than a convenience. Effect colour fades over an effect's
 * life and effect position changes per instance, so the colour and matrix
 * hashes MUST vary across instances of one template: they are the positive
 * control proving the comparator can see a difference at all. Only against that
 * control does an invariant GEOMETRY hash mean model-space words rather than a
 * comparator that always answers "same". */
#define NDS_EFFECT_PACKET_CAPTURE_WORDS 1024u

volatile u32 gNdsEffectPacketWords[NDS_EFFECT_PACKET_CAPTURE_WORDS];
volatile u32 gNdsEffectPacketClassCommands[NDS_TASK29_GX_CLASS_COUNT];
volatile u32 gNdsEffectPacketClassWords[NDS_TASK29_GX_CLASS_COUNT];
volatile u32 gNdsEffectPacketGeomHash;
volatile u32 gNdsEffectPacketColorHash;
volatile u32 gNdsEffectPacketMatrixHash;
volatile u32 gNdsEffectPacketGeomWords;
volatile u32 gNdsEffectPacketColorWords;
volatile u32 gNdsEffectPacketMatrixWords;
volatile u32 gNdsEffectPacketCaptureCount;
volatile u32 gNdsEffectPacketDroppedWords;
volatile u32 gNdsEffectPacketFaultCount;
volatile u32 gNdsEffectPacketLastWordCount;
/* Cumulative twins of the four per-list figures above. The per-list ones are
 * reset by every CaptureBegin, so a single end-of-run read returns the LAST
 * list's shape and nothing about the other 580; these are never reset, so
 * dividing by gNdsEffectPacketCaptureCount gives the per-instance average over
 * the whole window. gNdsEffectPacketTotalVertexCommands exists to be divided
 * that way and checked against the board's banked per-instance vertex count
 * (40.95 gate / 48.2 Boundary): a capture that is seeing the effect layer and
 * nothing else must reproduce it. */
volatile u32 gNdsEffectPacketTotalGeomWords;
volatile u32 gNdsEffectPacketTotalColorWords;
volatile u32 gNdsEffectPacketTotalMatrixWords;
volatile u32 gNdsEffectPacketTotalMatrixCommands;
volatile u32 gNdsEffectPacketTotalVertexCommands;
/* G3 STEP 2 -- WHICH PREDICATE ACTUALLY DECIDES. The cycle-88 result proved
 * effect vertices are CPU-projected, but not WHY the raw path was refused, and
 * the two possibilities call for very different repairs: a single early flag
 * test is narrow to fix, while a range or matrix condition genuinely true of
 * effect geometry is a different and larger problem.
 *
 * ndsRendererHardwareClassifySubmit returns a distinct value at each of its
 * predicates, so binning AT THE RETURN makes the histogram name the deciding
 * predicate rather than merely the resulting class. The one place that is not
 * one-to-one -- PROJECTED_RANGE_OR_MATRIX is returned from two sites -- is
 * split into bins 3 and 4 by hand for exactly that reason.
 *
 *   0 source_zbuffered == FALSE      4 raw matrix incompatible
 *   1 decal depth                    5 RAW current matrix   (the raw path)
 *   2 prim depth                     6 RAW snapshot matrix  (the raw path)
 *   3 raw vertex range reject        7 cross-matrix triangle
 *
 * Binned only while the effect capture is armed, so the population is the
 * effect layer and nothing else -- the same arming the cycle-88 capture used,
 * whose count matched gNdsEffectDLSubmitCount exactly. The total is published
 * beside the bins so it can be checked against the renderer's own independent
 * effect triangle count; if they disagree the histogram is measuring something
 * other than what it claims.
 *
 * Named scalars rather than an array on purpose: sample-tick-hud-buckets.ps1
 * validates -ExtraGlobals against the ELF symbol table by bare name, so an
 * array element is not readable by the standard instrument and a bin nobody
 * can read is not a measurement. */
volatile u32 gNdsEffectSubmitNoZ;
volatile u32 gNdsEffectSubmitDecal;
volatile u32 gNdsEffectSubmitPrimDepth;
volatile u32 gNdsEffectSubmitRangeReject;
volatile u32 gNdsEffectSubmitMatrixReject;
volatile u32 gNdsEffectSubmitRawCurrent;
volatile u32 gNdsEffectSubmitRawSnapshot;
volatile u32 gNdsEffectSubmitCrossMatrix;
volatile u32 gNdsEffectSubmitTotal;

static u32 sNdsEffectPacketArmed;
static u32 sNdsEffectPacketCursor;

/* FNV-1a. The offset basis is not decorative: a zero seed makes the empty
 * stream and a stream of one zero word hash identically, and an effect list
 * that emits no colour at all is a real case here. */
#define NDS_EFFECT_PACKET_HASH_SEED 2166136261u

static u32 ndsEffectPacketClassBucket(u32 command_class)
{
    switch (command_class)
    {
    case (u32)NDS_TASK29_GX_COLOR:
        return 1u;
    case (u32)NDS_TASK29_GX_MATRIX_MODE:
    case (u32)NDS_TASK29_GX_MATRIX_IDENTITY:
    case (u32)NDS_TASK29_GX_MATRIX_LOAD4X4:
    case (u32)NDS_TASK29_GX_MATRIX_MULT4X4:
    case (u32)NDS_TASK29_GX_MATRIX_MULT4x3:
    case (u32)NDS_TASK29_GX_MATRIX_PUSH:
    case (u32)NDS_TASK29_GX_MATRIX_POP:
    case (u32)NDS_TASK29_GX_MATRIX_STORE:
    case (u32)NDS_TASK29_GX_MATRIX_RESTORE:
        return 2u;
    default:
        break;
    }
    return 0u;
}

static void ndsEffectPacketRecord(u32 command_class, const u32 *words,
                                  u32 word_count)
{
    u32 bucket;
    u32 hash;
    u32 index;

    if (command_class >= (u32)NDS_TASK29_GX_CLASS_COUNT)
    {
        gNdsEffectPacketFaultCount++;
        return;
    }
    gNdsEffectPacketClassCommands[command_class]++;
    gNdsEffectPacketClassWords[command_class] += word_count;
    if (command_class == (u32)NDS_TASK29_GX_VERTEX16)
    {
        gNdsEffectPacketTotalVertexCommands++;
    }
    bucket = ndsEffectPacketClassBucket(command_class);
    hash = (bucket == 1u) ? gNdsEffectPacketColorHash :
        ((bucket == 2u) ? gNdsEffectPacketMatrixHash :
                          gNdsEffectPacketGeomHash);
    /* The class tag is folded in ahead of its words so a stream that moves the
     * same word between two classes cannot hash equal to the original. */
    hash = (hash ^ command_class) * 16777619u;
    for (index = 0u; index < word_count; index++)
    {
        u32 word = (words != NULL) ? words[index] : 0u;
        u32 cursor = sNdsEffectPacketCursor;

        /* The buffer holds one list for a gdb dump; the HASHES cover the whole
         * stream whether or not it fits, so a truncated capture can never read
         * as agreement. */
        if (cursor < NDS_EFFECT_PACKET_CAPTURE_WORDS)
        {
            gNdsEffectPacketWords[cursor] = word;
            sNdsEffectPacketCursor = cursor + 1u;
        }
        else
        {
            gNdsEffectPacketDroppedWords++;
        }
        hash = (hash ^ word) * 16777619u;
    }
    if (bucket == 1u)
    {
        gNdsEffectPacketColorHash = hash;
        gNdsEffectPacketColorWords += word_count;
        gNdsEffectPacketTotalColorWords += word_count;
    }
    else if (bucket == 2u)
    {
        gNdsEffectPacketMatrixHash = hash;
        gNdsEffectPacketMatrixWords += word_count;
        gNdsEffectPacketTotalMatrixWords += word_count;
        gNdsEffectPacketTotalMatrixCommands++;
    }
    else
    {
        gNdsEffectPacketGeomHash = hash;
        gNdsEffectPacketGeomWords += word_count;
        gNdsEffectPacketTotalGeomWords += word_count;
    }
}

/* G3 STEP 3 -- OPTION B'S PRICE. The banked per-list partition charges 65.57%
 * to "generic DL interpretation", but that lumps together two costs with
 * opposite fates under a precompiled invariant packet: walking the source list
 * and dispatching its opcodes (REMOVABLE -- the packet is the walk's answer)
 * and transforming and projecting the vertices (NOT removable -- cycle 88
 * proved the vertex words are the per-instance payload, so option B still
 * computes them every frame). Only the first is what B buys.
 *
 * Four spans, all armed by the SAME flag the cycle-88 capture used, so the
 * population is the effect lists and nothing else:
 *
 *   Exec  existing, the whole interpreter call
 *   Vtx   the G_VTX handler -- per-vertex modelview/projection into the cache
 *   Tri   the per-triangle submit -- classify, the clip-vertex divides, the
 *         descending painter depth, the GX emit
 *   TexX  texture resolve taken INSIDE Exec (the existing gNdsEffectPhaseTex
 *         counter is armed by the whole-tree flag instead, so it can include
 *         work outside this window and cannot close an identity with it)
 *
 * Traversal is DERIVED as Exec - TexX - Vtx - Tri and is never counted
 * directly, for the same reason SBAS is derived from SRC: a residual that goes
 * negative disproves the nesting, so the subtraction is itself the proof the
 * three measured spans really sit inside Exec.
 *
 * THE TIMER READS BIAS THIS TOWARD B BEING SMALL, which is the safe direction.
 * Each bracket charges roughly one timer-read latency to the span it wraps, and
 * Vtx and Tri fire thousands of times per frame while Exec is entered once per
 * list -- so Vtx+Tri are inflated and the derived traversal residual is
 * deflated. The measured traversal share is therefore a LOWER bound on what a
 * packet removes, never an optimistic one. */
static u32 ndsEffectPhaseMark(void)
{
    return (sNdsEffectPacketArmed != 0u) ? cpuGetTiming() : 0u;
}

static void ndsEffectPhaseAddVtx(u32 mark)
{
    if (sNdsEffectPacketArmed != 0u)
    {
        gNdsEffectPhaseVtxTicks += cpuGetTiming() - mark;
        gNdsEffectPhaseVtxCount++;
    }
}

static void ndsEffectPhaseAddTri(u32 mark)
{
    if (sNdsEffectPacketArmed != 0u)
    {
        gNdsEffectPhaseTriTicks += cpuGetTiming() - mark;
        gNdsEffectPhaseTriCount++;
    }
}

#define NDS_EFFECT_PHASE_VTX(call) \
    do { u32 nds_eff_m_ = ndsEffectPhaseMark(); call; \
         ndsEffectPhaseAddVtx(nds_eff_m_); } while (0)
#define NDS_EFFECT_PHASE_TRI(call) \
    do { u32 nds_eff_m_ = ndsEffectPhaseMark(); call; \
         ndsEffectPhaseAddTri(nds_eff_m_); } while (0)

static void ndsEffectPacketSubmitBin(u32 bin)
{
    if (sNdsEffectPacketArmed == 0u)
    {
        return;
    }
    switch (bin)
    {
    case 0u: gNdsEffectSubmitNoZ++; break;
    case 1u: gNdsEffectSubmitDecal++; break;
    case 2u: gNdsEffectSubmitPrimDepth++; break;
    case 3u: gNdsEffectSubmitRangeReject++; break;
    case 4u: gNdsEffectSubmitMatrixReject++; break;
    case 5u: gNdsEffectSubmitRawCurrent++; break;
    case 6u: gNdsEffectSubmitRawSnapshot++; break;
    default: gNdsEffectSubmitCrossMatrix++; break;
    }
    gNdsEffectSubmitTotal++;
}

#define NDS_EFFECT_SUBMIT_BIN(bin) ndsEffectPacketSubmitBin(bin)

void ndsEffectPacketCaptureBegin(void)
{
    sNdsEffectPacketCursor = 0u;
    gNdsEffectPacketGeomWords = 0u;
    gNdsEffectPacketColorWords = 0u;
    gNdsEffectPacketMatrixWords = 0u;
    gNdsEffectPacketGeomHash = NDS_EFFECT_PACKET_HASH_SEED;
    gNdsEffectPacketColorHash = NDS_EFFECT_PACKET_HASH_SEED;
    gNdsEffectPacketMatrixHash = NDS_EFFECT_PACKET_HASH_SEED;
    sNdsEffectPacketArmed = 1u;
}

void ndsEffectPacketCaptureEnd(void)
{
    sNdsEffectPacketArmed = 0u;
    gNdsEffectPacketLastWordCount = sNdsEffectPacketCursor;
    gNdsEffectPacketCaptureCount++;
}
#endif

#if NDS_TASK36_HW_COMPOSE == 2
/* Task 44 item 2: the single source of truth for "the replay capture window is
 * open". It lives out here, next to the wrapped GX record sites, so the hot
 * path can test it inline instead of calling the recorder to learn that it has
 * nothing to do. The replay owner owns every write; see
 * ndsRendererTask36ReplayCaptureBeginRun/EndRun and ...ReplayReset. */
static u32 sNdsRendererTask36CaptureActive;

static void ndsRendererTask36ReplayRecord(
    NDSRendererTask29GXClass command_class,
    const u32 *words,
    u32 word_count);

#if NDS_TASK44_STAGE_STEADY
/* Replay steady state pays one predictable test-and-skip per wrapped GX
 * command. The capture window itself is bit-identical: when the scalar is set
 * the recorder runs exactly as before. */
#define NDS_TASK36_REPLAY_RECORD(command_class, words, word_count) \
    do { \
        if (sNdsRendererTask36CaptureActive != 0u) \
        { \
            ndsRendererTask36ReplayRecord((command_class), (words), \
                                          (word_count)); \
        } \
    } while (0)
#else
#define NDS_TASK36_REPLAY_RECORD(command_class, words, word_count) \
    ndsRendererTask36ReplayRecord((command_class), (words), (word_count))
#endif
#endif

#if NDS_TASK29_GX_CENSUS
volatile u32 gNdsTask29GXFrame;
volatile u32 gNdsTask29GXCommandCount[NDS_TASK29_GX_CLASS_COUNT];
volatile u32 gNdsTask29GXWordCount[NDS_TASK29_GX_CLASS_COUNT];
volatile u32 gNdsTask29GXRepeatCount[NDS_TASK29_GX_CLASS_COUNT];
volatile u32 gNdsTask29GXOwnerCommandCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
volatile u32 gNdsTask29GXOwnerWordCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
volatile u32 gNdsTask29GXOwnerRepeatCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
volatile u32 gNdsTask29GXTotalCommandCount;
volatile u32 gNdsTask29GXTotalWordCount;
volatile u32 gNdsTask29GXTotalRepeatCount;
volatile u32 gNdsTask29GXStreamHashA;
volatile u32 gNdsTask29GXStreamHashB;
volatile u32 gNdsTask29GXOwnerHashA[NDS_TASK29_GX_OWNER_COUNT];
volatile u32 gNdsTask29GXOwnerHashB[NDS_TASK29_GX_OWNER_COUNT];
volatile u32 gNdsTask29GXBoundaryHashA;
volatile u32 gNdsTask29GXBoundaryHashB;
volatile u32 gNdsTask29GXBoundaryCount;
volatile u32 gNdsTask29GXFaultCount;
volatile u32 gNdsTask29GXNeverSuppressMask =
    (1u << NDS_TASK29_GX_TEXTURE_BIND) |
    (1u << NDS_TASK29_GX_MATRIX_IDENTITY) |
    (1u << NDS_TASK29_GX_MATRIX_LOAD4X4) |
    (1u << NDS_TASK29_GX_MATRIX_MULT4X4) |
    (1u << NDS_TASK29_GX_MATRIX_PUSH) |
    (1u << NDS_TASK29_GX_MATRIX_POP) |
    (1u << NDS_TASK29_GX_MATRIX_STORE) |
    (1u << NDS_TASK29_GX_MATRIX_RESTORE) |
    (1u << NDS_TASK29_GX_BEGIN) |
    (1u << NDS_TASK29_GX_END) |
    (1u << NDS_TASK29_GX_VERTEX16) |
    (1u << NDS_TASK29_GX_FLUSH);

static u32 sNdsTask29GXCommandCount[NDS_TASK29_GX_CLASS_COUNT];
static u32 sNdsTask29GXWordCount[NDS_TASK29_GX_CLASS_COUNT];
static u32 sNdsTask29GXRepeatCount[NDS_TASK29_GX_CLASS_COUNT];
static u32 sNdsTask29GXOwnerCommandCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
static u32 sNdsTask29GXOwnerWordCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
static u32 sNdsTask29GXOwnerRepeatCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
static u32 sNdsTask29GXLastWords
    [NDS_TASK29_GX_CLASS_COUNT][NDS_TASK29_GX_MAX_WORDS];
static u8 sNdsTask29GXLastWordCount[NDS_TASK29_GX_CLASS_COUNT];
static u32 sNdsTask29GXLastValidMask;
static u32 sNdsTask29GXOwnerHashA[NDS_TASK29_GX_OWNER_COUNT];
static u32 sNdsTask29GXOwnerHashB[NDS_TASK29_GX_OWNER_COUNT];
static u32 sNdsTask29GXTotalCommandCount;
static u32 sNdsTask29GXTotalWordCount;
static u32 sNdsTask29GXTotalRepeatCount;
static u32 sNdsTask29GXStreamHashA;
static u32 sNdsTask29GXStreamHashB;
static u32 sNdsTask29GXBoundaryHashA;
static u32 sNdsTask29GXBoundaryHashB;
static u32 sNdsTask29GXBoundaryCount;
static u32 sNdsTask29GXFaultCount;
static u32 sNdsTask29GXOwner = NDS_RENDERER_PROFILE_OWNER_NONE;
static u32 sNdsTask29GXActive;

static inline u32 ndsRendererTask29GXHashA(u32 hash, u32 value)
{
    return (hash ^ value) * 16777619u;
}

static inline u32 ndsRendererTask29GXHashB(u32 hash, u32 value)
{
    hash ^= value + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    return ((hash << 7) | (hash >> 25)) * 0x85ebca6bu;
}

static void NDS_TASK29_GX_CENSUS_CODE
ndsRendererTask29GXResetWorking(void)
{
    u32 owner;

    memset(sNdsTask29GXCommandCount, 0,
           sizeof(sNdsTask29GXCommandCount));
    memset(sNdsTask29GXWordCount, 0, sizeof(sNdsTask29GXWordCount));
    memset(sNdsTask29GXRepeatCount, 0,
           sizeof(sNdsTask29GXRepeatCount));
    memset(sNdsTask29GXOwnerCommandCount, 0,
           sizeof(sNdsTask29GXOwnerCommandCount));
    memset(sNdsTask29GXOwnerWordCount, 0,
           sizeof(sNdsTask29GXOwnerWordCount));
    memset(sNdsTask29GXOwnerRepeatCount, 0,
           sizeof(sNdsTask29GXOwnerRepeatCount));
    memset(sNdsTask29GXLastWordCount, 0,
           sizeof(sNdsTask29GXLastWordCount));
    sNdsTask29GXLastValidMask = 0u;
    sNdsTask29GXTotalCommandCount = 0u;
    sNdsTask29GXTotalWordCount = 0u;
    sNdsTask29GXTotalRepeatCount = 0u;
    sNdsTask29GXStreamHashA = 2166136261u;
    sNdsTask29GXStreamHashB = 0x9e3779b9u;
    sNdsTask29GXBoundaryHashA = 2166136261u;
    sNdsTask29GXBoundaryHashB = 0x9e3779b9u;
    sNdsTask29GXBoundaryCount = 0u;
    sNdsTask29GXFaultCount = 0u;
    sNdsTask29GXOwner = NDS_RENDERER_PROFILE_OWNER_NONE;
    for (owner = 0u; owner < NDS_TASK29_GX_OWNER_COUNT; owner++)
    {
        sNdsTask29GXOwnerHashA[owner] = 2166136261u;
        sNdsTask29GXOwnerHashB[owner] = 0x9e3779b9u;
    }
    sNdsTask29GXActive = TRUE;
}

static inline void ndsRendererTask29GXEnsureActive(void)
{
    if (sNdsTask29GXActive == FALSE)
    {
        ndsRendererTask29GXResetWorking();
    }
}

static void NDS_TASK29_GX_CENSUS_CODE
ndsRendererTask29GXRecord(
    NDSRendererTask29GXClass command_class,
    const u32 *words,
    u32 word_count)
{
    u32 class_index = (u32)command_class;
    u32 owner;
    u32 word_index;
    u32 repeat = TRUE;

#if NDS_TASK34_STAGE_STREAM_CENSUS
    ndsRendererTask34StageStreamRecord(command_class, words, word_count);
#endif
#if NDS_TASK36_HW_COMPOSE == 2
    NDS_TASK36_REPLAY_RECORD(command_class, words, word_count);
#endif
#if NDS_TICK_HUD
    if (sNdsEffectPacketArmed != 0u)
    {
        ndsEffectPacketRecord((u32)command_class, words, word_count);
    }
#endif
    /* Task 49 GX-differ hook (mirror of the lean-funnel hook below). */
#if NDS_TASK49_GX_DIFFER
    ndsRendererTask49GxDifferRecord(
        (u32)sNdsRendererRuntimeOwner, (u32)command_class, words, word_count);
#endif
    ndsRendererTask29GXEnsureActive();
    if ((class_index >= NDS_TASK29_GX_CLASS_COUNT) ||
        (word_count > NDS_TASK29_GX_MAX_WORDS) ||
        ((word_count != 0u) && (words == NULL)))
    {
        sNdsTask29GXFaultCount++;
        return;
    }
    owner = (sNdsTask29GXOwner < NDS_TASK29_GX_OWNER_COUNT) ?
        sNdsTask29GXOwner : (u32)NDS_RENDERER_PROFILE_OWNER_NONE;
    if (((sNdsTask29GXLastValidMask & (1u << class_index)) == 0u) ||
        (sNdsTask29GXLastWordCount[class_index] != word_count))
    {
        repeat = FALSE;
    }
    for (word_index = 0u; (word_index < word_count) && repeat; word_index++)
    {
        if (sNdsTask29GXLastWords[class_index][word_index] !=
            words[word_index])
        {
            repeat = FALSE;
        }
    }

    sNdsTask29GXCommandCount[class_index]++;
    sNdsTask29GXWordCount[class_index] += word_count;
    sNdsTask29GXOwnerCommandCount[owner][class_index]++;
    sNdsTask29GXOwnerWordCount[owner][class_index] += word_count;
    sNdsTask29GXTotalCommandCount++;
    sNdsTask29GXTotalWordCount += word_count;
    if (repeat != FALSE)
    {
        sNdsTask29GXRepeatCount[class_index]++;
        sNdsTask29GXOwnerRepeatCount[owner][class_index]++;
        sNdsTask29GXTotalRepeatCount++;
    }

    sNdsTask29GXStreamHashA = ndsRendererTask29GXHashA(
        sNdsTask29GXStreamHashA, 0xc0000000u | class_index);
    sNdsTask29GXStreamHashB = ndsRendererTask29GXHashB(
        sNdsTask29GXStreamHashB, 0xc0000000u | class_index);
    sNdsTask29GXStreamHashA = ndsRendererTask29GXHashA(
        sNdsTask29GXStreamHashA, owner);
    sNdsTask29GXStreamHashB = ndsRendererTask29GXHashB(
        sNdsTask29GXStreamHashB, owner);
    sNdsTask29GXStreamHashA = ndsRendererTask29GXHashA(
        sNdsTask29GXStreamHashA, word_count);
    sNdsTask29GXStreamHashB = ndsRendererTask29GXHashB(
        sNdsTask29GXStreamHashB, word_count);
    sNdsTask29GXOwnerHashA[owner] = ndsRendererTask29GXHashA(
        sNdsTask29GXOwnerHashA[owner], class_index);
    sNdsTask29GXOwnerHashB[owner] = ndsRendererTask29GXHashB(
        sNdsTask29GXOwnerHashB[owner], class_index);
    for (word_index = 0u; word_index < word_count; word_index++)
    {
        u32 word = words[word_index];

        sNdsTask29GXStreamHashA = ndsRendererTask29GXHashA(
            sNdsTask29GXStreamHashA, word);
        sNdsTask29GXStreamHashB = ndsRendererTask29GXHashB(
            sNdsTask29GXStreamHashB, word);
        sNdsTask29GXOwnerHashA[owner] = ndsRendererTask29GXHashA(
            sNdsTask29GXOwnerHashA[owner], word);
        sNdsTask29GXOwnerHashB[owner] = ndsRendererTask29GXHashB(
            sNdsTask29GXOwnerHashB[owner], word);
        sNdsTask29GXLastWords[class_index][word_index] = word;
    }
    sNdsTask29GXLastWordCount[class_index] = (u8)word_count;
    sNdsTask29GXLastValidMask |= 1u << class_index;
}

void NDS_TASK29_GX_CENSUS_CODE
ndsRendererTask29GXSetOwner(NDSRendererProfileOwner owner)
{
    u32 owner_index = ((u32)owner < NDS_TASK29_GX_OWNER_COUNT) ?
        (u32)owner : (u32)NDS_RENDERER_PROFILE_OWNER_NONE;
    u32 boundary_word;

    ndsRendererTask29GXEnsureActive();
    boundary_word = 0xb0000000u | owner_index;
    sNdsTask29GXBoundaryHashA = ndsRendererTask29GXHashA(
        sNdsTask29GXBoundaryHashA, boundary_word);
    sNdsTask29GXBoundaryHashB = ndsRendererTask29GXHashB(
        sNdsTask29GXBoundaryHashB, boundary_word);
    sNdsTask29GXStreamHashA = ndsRendererTask29GXHashA(
        sNdsTask29GXStreamHashA, boundary_word);
    sNdsTask29GXStreamHashB = ndsRendererTask29GXHashB(
        sNdsTask29GXStreamHashB, boundary_word);
    sNdsTask29GXBoundaryCount++;
    sNdsTask29GXOwner = owner_index;
    sNdsTask29GXLastValidMask = 0u;
}

void NDS_TASK29_GX_CENSUS_CODE ndsRendererTask29GXRecordFlush(u32 mode)
{
    ndsRendererTask29GXRecord(NDS_TASK29_GX_FLUSH, &mode, 1u);
    sNdsTask29GXLastValidMask = 0u;
}

void NDS_TASK29_GX_CENSUS_CODE ndsRendererTask29GXPublishFrame(void)
{
    u32 command_class;
    u32 owner;

    ndsRendererTask29GXEnsureActive();
    gNdsTask29GXFrame = gNdsRendererProfileFrameCount;
    for (command_class = 0u;
         command_class < NDS_TASK29_GX_CLASS_COUNT;
         command_class++)
    {
        gNdsTask29GXCommandCount[command_class] =
            sNdsTask29GXCommandCount[command_class];
        gNdsTask29GXWordCount[command_class] =
            sNdsTask29GXWordCount[command_class];
        gNdsTask29GXRepeatCount[command_class] =
            sNdsTask29GXRepeatCount[command_class];
        for (owner = 0u; owner < NDS_TASK29_GX_OWNER_COUNT; owner++)
        {
            gNdsTask29GXOwnerCommandCount[owner][command_class] =
                sNdsTask29GXOwnerCommandCount[owner][command_class];
            gNdsTask29GXOwnerWordCount[owner][command_class] =
                sNdsTask29GXOwnerWordCount[owner][command_class];
            gNdsTask29GXOwnerRepeatCount[owner][command_class] =
                sNdsTask29GXOwnerRepeatCount[owner][command_class];
        }
    }
    for (owner = 0u; owner < NDS_TASK29_GX_OWNER_COUNT; owner++)
    {
        gNdsTask29GXOwnerHashA[owner] = sNdsTask29GXOwnerHashA[owner];
        gNdsTask29GXOwnerHashB[owner] = sNdsTask29GXOwnerHashB[owner];
    }
    gNdsTask29GXTotalCommandCount = sNdsTask29GXTotalCommandCount;
    gNdsTask29GXTotalWordCount = sNdsTask29GXTotalWordCount;
    gNdsTask29GXTotalRepeatCount = sNdsTask29GXTotalRepeatCount;
    gNdsTask29GXStreamHashA = sNdsTask29GXStreamHashA;
    gNdsTask29GXStreamHashB = sNdsTask29GXStreamHashB;
    gNdsTask29GXBoundaryHashA = sNdsTask29GXBoundaryHashA;
    gNdsTask29GXBoundaryHashB = sNdsTask29GXBoundaryHashB;
    gNdsTask29GXBoundaryCount = sNdsTask29GXBoundaryCount;
    gNdsTask29GXFaultCount = sNdsTask29GXFaultCount;
    gNdsTask29GXNeverSuppressMask =
        (1u << NDS_TASK29_GX_TEXTURE_BIND) |
        (1u << NDS_TASK29_GX_MATRIX_IDENTITY) |
        (1u << NDS_TASK29_GX_MATRIX_LOAD4X4) |
        (1u << NDS_TASK29_GX_MATRIX_MULT4X4) |
        (1u << NDS_TASK29_GX_MATRIX_PUSH) |
        (1u << NDS_TASK29_GX_MATRIX_POP) |
        (1u << NDS_TASK29_GX_MATRIX_STORE) |
        (1u << NDS_TASK29_GX_MATRIX_RESTORE) |
        (1u << NDS_TASK29_GX_BEGIN) |
        (1u << NDS_TASK29_GX_END) |
        (1u << NDS_TASK29_GX_VERTEX16) |
        (1u << NDS_TASK29_GX_FLUSH);
    sNdsTask29GXActive = FALSE;
}
#else
static inline void ndsRendererTask29GXRecord(
    NDSRendererTask29GXClass command_class,
    const u32 *words,
    u32 word_count)
{
#if NDS_TASK34_STAGE_STREAM_CENSUS
    ndsRendererTask34StageStreamRecord(command_class, words, word_count);
#endif
#if NDS_TASK36_HW_COMPOSE == 2
    NDS_TASK36_REPLAY_RECORD(command_class, words, word_count);
#endif
#if NDS_TICK_HUD
    if (sNdsEffectPacketArmed != 0u)
    {
        ndsEffectPacketRecord((u32)command_class, words, word_count);
    }
#endif
    /* Task 49 GX-differ hook: the single writer touch on this TU for the
     * differ. Records the per-owner stream word-for-word (body in the
     * nds_task49_gx_differ TU; compiles to nothing at the default). */
#if NDS_TASK49_GX_DIFFER
    ndsRendererTask49GxDifferRecord(
        (u32)sNdsRendererRuntimeOwner, (u32)command_class, words, word_count);
#endif
}
#endif

static inline void ndsRendererTask29GlEnable(int bits)
{
    u32 value;

    glEnable(bits);
    value = (u32)GFX_CONTROL;
    ndsRendererTask29GXRecord(NDS_TASK29_GX_CONTROL, &value, 1u);
}

static inline void ndsRendererTask29GlDisable(int bits)
{
    u32 value;

    glDisable(bits);
    value = (u32)GFX_CONTROL;
    ndsRendererTask29GXRecord(NDS_TASK29_GX_CONTROL, &value, 1u);
}

static inline void ndsRendererTask29GlAlphaFunc(int threshold)
{
    u32 value = (u32)threshold;

    glAlphaFunc(threshold);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_ALPHA_TEST, &value, 1u);
}

static inline void ndsRendererTask29GlFogDensity(int index, int density)
{
    u32 words[2] = {(u32)index, (u32)density};

    glFogDensity(index, density);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_FOG_TABLE, words, 2u);
}

static inline void ndsRendererTask29GlFogShift(int shift)
{
    u32 value;

    glFogShift(shift);
    value = (u32)GFX_CONTROL;
    ndsRendererTask29GXRecord(NDS_TASK29_GX_CONTROL, &value, 1u);
}

static inline void ndsRendererTask29GlFogOffset(int offset)
{
    u32 value = (u32)offset;

    glFogOffset(offset);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_FOG_OFFSET, &value, 1u);
}

static inline void ndsRendererTask29GlFogColor(
    u8 red, u8 green, u8 blue, u8 alpha)
{
    u32 value = RGB15(red, green, blue) | ((u32)alpha << 16);

    glFogColor(red, green, blue, alpha);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_FOG_COLOR, &value, 1u);
}

static inline void ndsRendererTask29GlTexParameter(int target, int param)
{
    u32 value;

    glTexParameter(target, param);
    value = glGetTexParameter();
    ndsRendererTask29GXRecord(NDS_TASK29_GX_TEXTURE_PARAM, &value, 1u);
}

static inline void ndsRendererTask29GlBindTexture(int target, int name)
{
    u32 words[2];

    glBindTexture(target, name);
    words[0] = (u32)name;
    words[1] = glGetTexParameter();
    ndsRendererTask29GXRecord(NDS_TASK29_GX_TEXTURE_BIND, words, 2u);
}

static inline void ndsRendererTask29GlMatrixMode(int mode)
{
    u32 value = (u32)mode;

    glMatrixMode(mode);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_MODE, &value, 1u);
}

static inline void ndsRendererTask29GlLoadIdentity(void)
{
    u32 value = 0u;

    glLoadIdentity();
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_IDENTITY, &value, 1u);
}

static inline void ndsRendererTask29GlLoadMatrix4x4(const m4x4 *matrix)
{
    glLoadMatrix4x4(matrix);
    ndsRendererTask29GXRecord(
        NDS_TASK29_GX_MATRIX_LOAD4X4, (const u32 *)matrix, 16u);
}

static inline void ndsRendererTask29GlMultMatrix4x4(const m4x4 *matrix)
{
    glMultMatrix4x4(matrix);
    ndsRendererTask29GXRecord(
        NDS_TASK29_GX_MATRIX_MULT4X4, (const u32 *)matrix, 16u);
}

/* Task 51: wrapped emit so the differ (Task 49) observes the 12-word 4x3
 * model-matrix command. Mirrors the MULT4X4 wrapper; records the new class. */
static inline void ndsRendererTask29GlMultMatrix4x3(const m4x3 *matrix)
{
    glMultMatrix4x3(matrix);
    ndsRendererTask29GXRecord(
        NDS_TASK29_GX_MATRIX_MULT4x3, (const u32 *)matrix, 12u);
}

static inline void ndsRendererTask29GlPushMatrix(void)
{
    u32 value = 0u;

    glPushMatrix();
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_PUSH, &value, 1u);
}

static inline void ndsRendererTask29GlPopMatrix(int count)
{
    u32 value = (u32)count;

    glPopMatrix(count);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_POP, &value, 1u);
}

static inline void ndsRendererTask29GlStoreMatrix(int index)
{
    u32 value = (u32)index;

    glStoreMatrix(index);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_STORE, &value, 1u);
}

static inline void ndsRendererTask29GlRestoreMatrix(int index)
{
    u32 value = (u32)index;

    glRestoreMatrix(index);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_RESTORE, &value, 1u);
}

static inline void ndsRendererTask29GlPolyFmt(u32 params)
{
    glPolyFmt(params);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_POLY_FORMAT, &params, 1u);
}

static inline void ndsRendererTask29GlBegin(GL_GLBEGIN_ENUM mode)
{
    u32 value = (u32)mode;

    glBegin(mode);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_BEGIN, &value, 1u);
}

static inline void ndsRendererTask29GlEnd(void)
{
    u32 value = 0u;

    glEnd();
    ndsRendererTask29GXRecord(NDS_TASK29_GX_END, &value, 1u);
}

static inline void ndsRendererTask29GlColor(rgb color)
{
    u32 value = (u32)color;

    glColor(color);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_COLOR, &value, 1u);
}

static inline void ndsRendererTask29GlTexCoord2t16(t16 s, t16 t)
{
    u32 value = (u32)(u16)s | ((u32)(u16)t << 16);

    glTexCoord2t16(s, t);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_TEX_COORD, &value, 1u);
}

static inline void ndsRendererTask29GlVertex3v16(v16 x, v16 y, v16 z)
{
    u32 words[2] = {
        (u32)(u16)x | ((u32)(u16)y << 16),
        (u32)(u16)z
    };

    glVertex3v16(x, y, z);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_VERTEX16, words, 2u);
}

#define glEnable ndsRendererTask29GlEnable
#define glDisable ndsRendererTask29GlDisable
#define glAlphaFunc ndsRendererTask29GlAlphaFunc
#define glFogDensity ndsRendererTask29GlFogDensity
#define glFogShift ndsRendererTask29GlFogShift
#define glFogOffset ndsRendererTask29GlFogOffset
#define glFogColor ndsRendererTask29GlFogColor
#define glTexParameter ndsRendererTask29GlTexParameter
#define glBindTexture ndsRendererTask29GlBindTexture
#define glMatrixMode ndsRendererTask29GlMatrixMode
#define glLoadIdentity ndsRendererTask29GlLoadIdentity
#define glLoadMatrix4x4 ndsRendererTask29GlLoadMatrix4x4
#define glMultMatrix4x4 ndsRendererTask29GlMultMatrix4x4
#define glMultMatrix4x3 ndsRendererTask29GlMultMatrix4x3
#define glPushMatrix ndsRendererTask29GlPushMatrix
#define glPopMatrix ndsRendererTask29GlPopMatrix
#define glStoreMatrix ndsRendererTask29GlStoreMatrix
#define glRestoreMatrix ndsRendererTask29GlRestoreMatrix
#define glPolyFmt ndsRendererTask29GlPolyFmt
#define glBegin ndsRendererTask29GlBegin
#define glEnd ndsRendererTask29GlEnd
#define glColor ndsRendererTask29GlColor
#define glTexCoord2t16 ndsRendererTask29GlTexCoord2t16
#define glVertex3v16 ndsRendererTask29GlVertex3v16
#endif

/* The bin macro is defined by the G3 capture block above, which is itself
 * nested inside the GX-record configuration guard. This classifier is not, so
 * the fallback has to be unconditional rather than an #else on that block --
 * otherwise a configuration that compiles the classifier without the capture
 * fails at the first bin call rather than simply not counting. These three
 * must sit OUTSIDE every NDS_RENDERER_HW_TRIANGLES-guarded block AND before
 * the first use: the primaries live under NDS_TICK_HUD at line 886, the first
 * use is inside the hardware block at 17434, and d34917b9 put the fallback
 * inside that same block, which left a default build with neither flag -- the
 * plain `make` target -- with no definition at all. The #ifndefs are belt and
 * braces now, since nothing else defines them on this path. */
#ifndef NDS_EFFECT_SUBMIT_BIN
#define NDS_EFFECT_SUBMIT_BIN(bin) ((void)0)
#endif
#ifndef NDS_EFFECT_PHASE_VTX
#define NDS_EFFECT_PHASE_VTX(call) do { call; } while (0)
#endif
#ifndef NDS_EFFECT_PHASE_TRI
#define NDS_EFFECT_PHASE_TRI(call) do { call; } while (0)
#endif

/* These three carry the GX record hook. A fighter emit path must NOT use them --
 * see ndsRendererHardwareWriteFighter*Word below, and the measurement that made
 * the split: the hook's two capture flags can never be set on the fighter path
 * and testing them once per corner cost 24% of the untextured emit. */
static inline void ndsRendererHardwareWriteColorWord(u32 value)
{
#if !NDS_RENDERER_HW_TRIANGLES
    (void)value;
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    ndsRendererBenchmarkSinkWord(value);
#else
#if NDS_TASK29_GX_CENSUS || NDS_TASK34_STAGE_STREAM_CENSUS || \
    (NDS_TASK36_HW_COMPOSE == 2) || NDS_TASK49_GX_DIFFER
    ndsRendererTask29GXRecord(NDS_TASK29_GX_COLOR, &value, 1u);
#endif
    GFX_COLOR = value;
#endif
}

static inline void ndsRendererHardwareWriteNormalWord(u32 value)
{
#if !NDS_RENDERER_HW_TRIANGLES
    (void)value;
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    ndsRendererBenchmarkSinkWord(value);
#else
    GFX_NORMAL = value;
#endif
}

static inline void ndsRendererHardwareWriteDiffuseAmbient(u32 value)
{
#if !NDS_RENDERER_HW_TRIANGLES
    (void)value;
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    ndsRendererBenchmarkSinkWord(value);
#else
    GFX_DIFFUSE_AMBIENT = value;
#endif
}

static inline void ndsRendererHardwareWriteTexCoordWord(u32 value)
{
#if !NDS_RENDERER_HW_TRIANGLES
    (void)value;
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    ndsRendererBenchmarkSinkWord(value);
#else
#if NDS_TASK29_GX_CENSUS || NDS_TASK34_STAGE_STREAM_CENSUS || \
    (NDS_TASK36_HW_COMPOSE == 2) || NDS_TASK49_GX_DIFFER
    ndsRendererTask29GXRecord(NDS_TASK29_GX_TEX_COORD, &value, 1u);
#endif
    GFX_TEX_COORD = value;
#endif
}

static inline void ndsRendererHardwareWriteVertex16Words(u32 xy, u32 z)
{
#if !NDS_RENDERER_HW_TRIANGLES
    (void)xy;
    (void)z;
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    ndsRendererBenchmarkSinkWord(xy);
    ndsRendererBenchmarkSinkWord(z);
#else
#if NDS_TASK29_GX_CENSUS || NDS_TASK34_STAGE_STREAM_CENSUS || \
    (NDS_TASK36_HW_COMPOSE == 2) || NDS_TASK49_GX_DIFFER
    u32 words[2] = {xy, z};

    ndsRendererTask29GXRecord(NDS_TASK29_GX_VERTEX16, words, 2u);
#endif
    GFX_VERTEX16 = xy;
    GFX_VERTEX16 = z;
#endif
}

/* ---------------------------------------------------------------------------
 * Fighter-owned GX writers: the same stores, without the two capture hooks that
 * can never fire on this path.
 *
 * The Task 36 replay capture window is opened by
 * ndsRendererTask36ReplayCaptureBeginRun and closed by ...EndRun, and both calls
 * sit inside ndsRendererCommitNativeStageSegment bracketing ONE STAGE run --
 * BeginRun even faults on `run_index >= NDS_NATIVE_STAGE_RUN_COUNT`. The fighter
 * production owner is a separate draw call and can never be nested inside that
 * window, so `sNdsRendererTask36CaptureActive` is FALSE at every fighter corner.
 * The effect packet capture is armed the same way, around an effect display list
 * (`phase_effect`) in reloc_backend_renderer_dl.c, never around a fighter.
 *
 * Testing both per corner was not free. In the c106 profile the untextured emit
 * runs 537,780 corners for 27,484,418 cycles (51.1 a corner) and the capture
 * scaffolding -- the main-RAM flag load, the compare, the branch, and the two
 * register spills the maybe-call forces -- is 6,591,047 of them for Task 36
 * alone, 24.0%, ~7,600 ticks/frame. The textured emit pays it twice a corner.
 *
 * Every other GX diagnostic is kept: the Task 29 census, the Task 34 stage
 * stream and the Task 49 differ all want the fighter's stream, and their builds
 * are not performance builds. Only the two capture recorders are dropped, and
 * both were provably no-ops here. */
#define NDS_RENDERER_GX_RECORD_FIGHTER \
    (NDS_TASK29_GX_CENSUS || NDS_TASK34_STAGE_STREAM_CENSUS || \
     NDS_TASK49_GX_DIFFER)

static inline void ndsRendererHardwareWriteFighterColorWord(u32 value)
{
#if !NDS_RENDERER_HW_TRIANGLES
    (void)value;
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    ndsRendererBenchmarkSinkWord(value);
#else
#if NDS_RENDERER_GX_RECORD_FIGHTER
    ndsRendererTask29GXRecord(NDS_TASK29_GX_COLOR, &value, 1u);
#endif
    GFX_COLOR = value;
#endif
}

static inline void ndsRendererHardwareWriteFighterTexCoordWord(u32 value)
{
#if !NDS_RENDERER_HW_TRIANGLES
    (void)value;
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    ndsRendererBenchmarkSinkWord(value);
#else
#if NDS_RENDERER_GX_RECORD_FIGHTER
    ndsRendererTask29GXRecord(NDS_TASK29_GX_TEX_COORD, &value, 1u);
#endif
    GFX_TEX_COORD = value;
#endif
}

static inline void ndsRendererHardwareWriteFighterVertex16Words(u32 xy, u32 z)
{
#if !NDS_RENDERER_HW_TRIANGLES
    (void)xy;
    (void)z;
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    ndsRendererBenchmarkSinkWord(xy);
    ndsRendererBenchmarkSinkWord(z);
#else
#if NDS_RENDERER_GX_RECORD_FIGHTER
    u32 words[2] = {xy, z};

    ndsRendererTask29GXRecord(NDS_TASK29_GX_VERTEX16, words, 2u);
#endif
    GFX_VERTEX16 = xy;
    GFX_VERTEX16 = z;
#endif
}

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
void ndsRendererBenchmarkSinkEndOwner(NDSRendererProfileOwner owner)
{
    u32 cursor = sNdsRendererBenchmarkSinkCursor;

    if ((u32)owner < NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererBenchmarkSinkOwnerWords[(u32)owner] +=
            cursor - sNdsRendererBenchmarkSinkLastOwnerCursor;
    }
    sNdsRendererBenchmarkSinkLastOwnerCursor = cursor;
}
#endif

#define NDS_RENDERER_OP_NOOP 0x00u
#define NDS_RENDERER_OP_VTX 0x01u
#define NDS_RENDERER_OP_MODIFYVTX 0x02u
#define NDS_RENDERER_OP_CULLDL 0x03u
#define NDS_RENDERER_OP_TRI1 0x05u
#define NDS_RENDERER_OP_TRI2 0x06u
#define NDS_RENDERER_OP_TEXTURE 0xd7u
#define NDS_RENDERER_OP_POPMTX 0xd8u
#define NDS_RENDERER_OP_MTX 0xdau
#define NDS_RENDERER_OP_GEOMETRYMODE 0xd9u
#define NDS_RENDERER_OP_MOVEWORD 0xdbu
#define NDS_RENDERER_OP_MOVEMEM 0xdcu
#define NDS_RENDERER_OP_SPECIAL_1 0xd5u
#define NDS_RENDERER_OP_DL 0xdeu
#define NDS_RENDERER_OP_ENDDL 0xdfu
#define NDS_RENDERER_OP_SETOTHERMODE_H 0xe3u
#define NDS_RENDERER_OP_SETOTHERMODE_L 0xe2u
#define NDS_RENDERER_OP_SETSCISSOR 0xedu
#define NDS_RENDERER_OP_SETPRIMDEPTH 0xeeu
#define NDS_RENDERER_OP_SETCOMBINE 0xfcu
#define NDS_RENDERER_OP_SETCIMG 0xffu
#define NDS_RENDERER_OP_SETFOGCOLOR 0xf8u
#define NDS_RENDERER_OP_SETBLENDCOLOR 0xf9u
#define NDS_RENDERER_OP_SETENVCOLOR 0xfbu
#define NDS_RENDERER_OP_SETPRIMCOLOR 0xfau
#define NDS_RENDERER_OP_SETTIMG 0xfdu
#define NDS_RENDERER_OP_SETTILE 0xf5u
#define NDS_RENDERER_OP_LOADTILE 0xf4u
#define NDS_RENDERER_OP_LOADBLOCK 0xf3u
#define NDS_RENDERER_OP_LOADTLUT 0xf0u
#define NDS_RENDERER_OP_SETTILESIZE 0xf2u
#define NDS_RENDERER_OP_RDPSETOTHERMODE 0xefu
#define NDS_RENDERER_OP_RDPPIPESYNC 0xe7u
#define NDS_RENDERER_OP_RDPLOADSYNC 0xe6u
#define NDS_RENDERER_OP_RDPTILESYNC 0xe8u
#define NDS_RENDERER_OP_RDPFULLSYNC 0xe9u

#define NDS_RENDERER_TX_CLAMP 0x2u
#define NDS_RENDERER_TX_MIRROR 0x1u
#define NDS_RENDERER_RENDER_TILE 0u
#define NDS_RENDERER_RENDER_TILE_1 1u
#define NDS_RENDERER_LOAD_TILE 7u

#define NDS_RENDERER_MAX_VTX NDS_RENDERER_VERTEX_CACHE_SIZE
#define NDS_RENDERER_MODELVIEW_STACK_SIZE 32u
#define NDS_RENDERER_N64_MTX_FRAC_BITS 16u
#define NDS_RENDERER_DS_MTX_FRAC_BITS 12u
#define NDS_RENDERER_MTX_PUSH_XOR 0x01u
#define NDS_RENDERER_MTX_PUSH 0x01u
#define NDS_RENDERER_MTX_LOAD 0x02u
#define NDS_RENDERER_MTX_PROJECTION 0x04u
#define NDS_RENDERER_MOVEWORD_MATRIX 0x00u
#define NDS_RENDERER_MOVEWORD_FOG 0x08u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL 0x0au
#define NDS_RENDERER_MOVEWORD_FOG_OFFSET 0x00u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_A 0x00u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_B 0x04u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_A 0x18u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_B 0x1cu
#define NDS_RENDERER_MOVEWORD_INDEX_SHIFT 16u
#define NDS_RENDERER_MOVEWORD_OFFSET_MASK 0xffffu
#define NDS_RENDERER_MOVEWORD_INDEX_MASK 0xffu
#define NDS_RENDERER_SPECIAL_1_OFFSET_SHIFT 8u
#define NDS_RENDERER_SPECIAL_1_OFFSET_MASK 0xffffu
#define NDS_RENDERER_SPECIAL_1_INDEX_MASK 0xffu
#define NDS_RENDERER_MWO_POINT_ST 0x14u
#define NDS_RENDERER_MOVEMEM_LIGHT 10u
#define NDS_RENDERER_MOVEMEM_OFFSET_SHIFT 8u
#define NDS_RENDERER_MOVEMEM_OFFSET_MASK 0xffu
#define NDS_RENDERER_MOVEMEM_LENGTH_SHIFT 19u
#define NDS_RENDERER_MOVEMEM_LENGTH_MASK 0x1fu
#define NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET 24u
#define NDS_RENDERER_MOVEMEM_LIGHT_STRIDE 24u
#define NDS_RENDERER_MATRIX_WORD_BYTES 4u
#define NDS_RENDERER_MATRIX_WORD_COUNT 16u
#define NDS_RENDERER_HW_TEXTURE_MAX_WIDTH 128u
#define NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT 128u
#define NDS_RENDERER_HW_TEXTURE_MAX_TEXELS \
    (NDS_RENDERER_HW_TEXTURE_MAX_WIDTH * NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT)
#define NDS_RENDERER_HW_TEXEL01_CI4_LUT_COUNT (16u * 16u)
#define NDS_RENDERER_HW_TEXEL01_CI4_PHASE_COUNT 16u
#define NDS_RENDERER_HW_TEXEL01_CI4_PHASE_LUT_COUNT \
    (NDS_RENDERER_HW_TEXEL01_CI4_PHASE_COUNT * \
     NDS_RENDERER_HW_TEXEL01_CI4_LUT_COUNT)
/* Sized from the measured working set, not chosen: Task 90 E0 traced 128
 * consecutive light-shade requests on the Boundary battle and found exactly 6
 * distinct (diffuse, ambient) pairs against 49 requests per frame. At 4 entries
 * the round-robin cache evicted live pairs and rebuilt a 128-entry table 6
 * times every frame; at 8 the trace replay drops to 6 misses total, which is
 * the compulsory floor -- one build per distinct pair for the whole match.
 * Must stay a power of two: the eviction cursor masks with COUNT - 1.
 * Re-run scripts/census-light-shade-lut.ps1 if the fighter or stage set
 * changes; a working set above 8 would put this back into steady-state thrash. */
#define NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT 8u
#define NDS_RENDERER_HW_LIGHT_SHADE_LUT_COUNT 128u
#define NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT 2u
#define NDS_RENDERER_HW_CI4_INDEX_CACHE_TEXELS 1024u
#define NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT 256u
#define NDS_RENDERER_HW_CI4_CLASS_KEY_MASK 0x7ffffu
#define NDS_RENDERER_HW_CI4_CLASS_INDEX_SHIFT 19u
#define NDS_RENDERER_HW_TEXEL01_RGB_MASK 0x7fffu
#define NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT 15u
#define NDS_RENDERER_HW_TEXTURE_FMT_RGBA16 0u
#define NDS_RENDERER_HW_TEXTURE_FMT_CI 2u
#define NDS_RENDERER_HW_TEXTURE_FMT_IA 3u
#define NDS_RENDERER_HW_TEXTURE_FMT_I16 4u
#define NDS_RENDERER_HW_TEXTURE_SIZ_4B 0u
#define NDS_RENDERER_HW_TEXTURE_SIZ_8B 1u
#define NDS_RENDERER_HW_TEXTURE_SIZ_16B 2u
#define NDS_RENDERER_HW_TEXTURE_SIZ_32B 3u
#define NDS_RENDERER_G_TX_DXT_ONE (1u << 11)
#define NDS_RENDERER_HW_TEXREJECT_MISSING_STATE (1u << 0)
#define NDS_RENDERER_HW_TEXREJECT_BAD_CI_SIZE (1u << 1)
#define NDS_RENDERER_HW_TEXREJECT_UNSUPPORTED_FORMAT (1u << 2)
#define NDS_RENDERER_HW_TEXREJECT_BAD_DIMENSIONS (1u << 3)
#define NDS_RENDERER_HW_TEXREJECT_BAD_UPLOAD_SIZE (1u << 4)
#define NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_RANGE (1u << 5)
#define NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES (1u << 6)
#define NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_PTR (1u << 7)
#define NDS_RENDERER_HW_TEXREJECT_BAD_TLUT (1u << 8)
#define NDS_RENDERER_HW_TEXREJECT_BAD_TLUT_PTR (1u << 9)
#define NDS_RENDERER_HW_TEXREJECT_ALLOC (1u << 10)
#define NDS_RENDERER_HW_TEXREJECT_GENTEX (1u << 11)
#define NDS_RENDERER_HW_TEXREJECT_TEXIMAGE (1u << 12)
#define NDS_RENDERER_HW_TEXEL1_REJECT_ACTIVE_TILE (1u << 0)
#define NDS_RENDERER_HW_TEXEL1_REJECT_TILE_STATE (1u << 1)
#define NDS_RENDERER_HW_TEXEL1_REJECT_LOAD_STATE (1u << 2)
#define NDS_RENDERER_HW_TEXEL1_REJECT_DIMENSIONS (1u << 3)
#define NDS_RENDERER_HW_TEXEL1_REJECT_PAIR_SIZE (1u << 4)
#define NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_RANGE (1u << 5)
#define NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_BYTES (1u << 6)
#define NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_PTR (1u << 7)
#define NDS_RENDERER_HW_USETEX_REJECT_NO_STATS (1u << 0)
#define NDS_RENDERER_HW_USETEX_REJECT_STATE_OFF (1u << 1)
#define NDS_RENDERER_HW_USETEX_REJECT_NO_COMBINE (1u << 2)
#define NDS_RENDERER_HW_USETEX_REJECT_PRIMITIVE_DECAL (1u << 3)
#define NDS_RENDERER_HW_USETEX_REJECT_NO_TEXEL0 (1u << 4)
#define NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE 0xffffu
#define NDS_RENDERER_HW_WORLD_UNIT_SHIFT 8u
#define NDS_RENDERER_HW_RAW_COORD_MIN (-2048)
#define NDS_RENDERER_HW_RAW_COORD_MAX 2047
#define NDS_RENDERER_HW_MATRIX_MODE_NONE 0u
#define NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY 1u
#define NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED 2u
#define NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY 3u
#define NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE 4u
#define NDS_RENDERER_HW_POS_TEST_MAX 40u
#define NDS_RENDERER_HW_POS_TEST_TOLERANCE 16u
#define NDS_RENDERER_HW_POS_TEST_MATRIX_WORD_DELTA 4352
#define NDS_RENDERER_MATRIX_SNAPSHOT_INVALID 0u
#define NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START (0x1000 * 6)
#define NDS_RENDERER_HW_PROJECTED_DEPTH_FOREGROUND_START \
    ((128 - 0x1000) * 6)
#define NDS_RENDERER_HW_PROJECTED_DEPTH_STEP 6
#define NDS_RENDERER_HW_SOURCE_DEPTH_MIN (128 - 0x1000)
#define NDS_RENDERER_HW_SOURCE_DEPTH_MAX (0x1000 - 129)
#define NDS_RENDERER_NATIVE_STAGE_STATIC_OWNER_COUNT 4u
#define NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z_CLASS 3u
#define NDS_RENDERER_HW_PROJECTED_VERTEX (1 << 12)
#define NDS_RENDERER_HW_DECAL_DEPTH_BIAS (3 << 4)
#define NDS_RENDERER_HW_ORACLE_EPSILON 0u
/* 48 is confirmed correct by measurement, not just by the checker that pins it:
 * Task 93 E0 traced 256 consecutive bind requests on the Boundary battle and
 * found 22 distinct keys against 25 binds per frame. A cache of 16 or fewer
 * would miss 87.9% of the trace; 32 already reaches the compulsory floor, so 48
 * carries real headroom and evicts nothing in steady state -- which is why Task
 * 81 measured zero evictions and zero uploads in its window.
 * Re-derive with scripts/census-texture-key-rebuild.ps1.
 *
 * DO NOT SIMPLY RAISE THIS. It was tried on 2026-08-04 and the ROM did not boot.
 * The Task 93 trace above covers the STEADY-STATE battle; it never covered the
 * scene after a KO, where BUGS row 6 measures the cache genuinely exhausted --
 * five consecutive probe tags reading free=0 live=48 pinned=28 evictable=0
 * thisframe=20, so nothing may be evicted and every further distinct texture is
 * refused with reason 0x400 (ALLOC), 2,592 times in one match. That diagnosis
 * stands. What does not work is paying for it in slots: 48 -> 96 costs +14,016
 * bytes (entries were 292 each) and main RAM has no headroom to give.
 * soak-freeze-watch.ps1:1104 records gNdsTaskmanGeneralHeapFreeMin at 24,404 --
 * already under the 25,600 at which ifCommonSetMaxNumGObj permanently caps the
 * GObj pool -- and binary growth costs that arena one for one. The measured
 * result was a guest that never completed a single battle frame.
 * Any fix for row 6 must be RAM-neutral or RAM-negative: shrink the entry, or
 * release some of the 28 pins. Do not re-run the growth experiment.
 *
 * 69 IS THAT RAM-NEGATIVE FIX, AND THE BYTES CAME FROM ROM DUPLICATION.
 *
 * Each of the 24 pinned static entries carried a resident 236-byte key that is
 * byte-identical to the key_words[59] its generated ROM record already holds,
 * except at the three POINTER words (image / tlut / texel1) where ROM stores an
 * asset offset and the runtime needs a loaded address. So the key left the
 * entry: dynamic slots own a pool key, static slots own three resident words
 * and read the other 56 straight out of ROM. Repacking the entry's own fields
 * (u8 flags, u16 dimensions) took it from 56 bytes to 44 on top of that.
 *
 *   before  48 x 292                                   = 14,016 bytes
 *   after   69 x 44 + 45 x 236 + 24 x 12                = 13,944 bytes
 *
 * The cache is a fixed partition, which is what makes the pool index free:
 * slots [0, STATIC_COUNT) are the static corpus, one slot per record index, and
 * slots [STATIC_COUNT, CACHE_COUNT) are dynamic and own key pool entry
 * slot - STATIC_COUNT. No free list, no back-pointer, no extra word per entry.
 *
 * WHY 69 AND NOT MORE. Every non-static entry needs its own 236-byte key, so
 * the pool is CACHE_COUNT - STATIC_COUNT and the whole thing costs
 * 280 x CACHE_COUNT - 5,376 bytes; 70 crosses the 14,016 the old cache spent.
 * The demand it has to serve is 37 distinct keys per frame measured AT THE
 * REQUEST SITE, so rejected requests still count (artifacts/performance/
 * texture-demand-postko.json, 113 requests -> 37 distinct, flat over 40
 * post-KO frames). 28 pinned + 37 distinct = 65 slots is therefore sufficient
 * whatever fraction of the 37 turns out to be pinned, and 69 clears it by 4.
 * Growing past 69 needs a smaller key, not more slots -- the 24-word texel1
 * block is 96 of the 236 bytes and is zero on every entry that does not
 * multitexture. */
#define NDS_RENDERER_HW_TEXTURE_CACHE_COUNT 69u
#define NDS_RENDERER_HW_TEXTURE_STATIC_COUNT 24u
#define NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT \
    (NDS_RENDERER_HW_TEXTURE_CACHE_COUNT - NDS_RENDERER_HW_TEXTURE_STATIC_COUNT)
#define NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT 128u
#define NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY 0u
#define NDS_RENDERER_CCMUX_COMBINED 0u
#define NDS_RENDERER_CCMUX_TEXEL0 1u
#define NDS_RENDERER_CCMUX_TEXEL1 2u
#define NDS_RENDERER_CCMUX_PRIMITIVE 3u
#define NDS_RENDERER_CCMUX_SHADE 4u
#define NDS_RENDERER_CCMUX_ENVIRONMENT 5u
#define NDS_RENDERER_CCMUX_PRIM_LOD_FRAC 14u
#define NDS_RENDERER_CCMUX_ZERO_AB 15u
/* G_CCMUX_0 is 31, and every colour field masks it to its own width: 4 bits for
 * a and b (15), 5 bits for c (31), 3 bits for d (7). One name per width, so a
 * gate cannot silently compare a c field against the a/b encoding. */
#define NDS_RENDERER_CCMUX_ZERO_C 31u
#define NDS_RENDERER_CCMUX_ZERO_D 7u
#define NDS_RENDERER_ACMUX_COMBINED 0u
#define NDS_RENDERER_ACMUX_TEXEL0 1u
#define NDS_RENDERER_ACMUX_TEXEL1 2u
#define NDS_RENDERER_ACMUX_PRIMITIVE 3u
#define NDS_RENDERER_ACMUX_SHADE 4u
#define NDS_RENDERER_ACMUX_ENVIRONMENT 5u
#define NDS_RENDERER_ACMUX_1 6u
#define NDS_RENDERER_ACMUX_0 7u
#define NDS_RENDERER_PRIM_ENV_BLEND_NONE 0u
#define NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA 1u
#define NDS_RENDERER_PRIM_ENV_BLEND_SOURCE_ALPHA 2u
/* The rebirth halo's beam (85.vpk0.bin 0x28a0, G_SETCOMBINE FCFFFFFF FFFDF2F9)
 * asks for rgb = PRIMITIVE and alpha = TEXEL0 in both cycles -- a constant
 * colour, not the BLENDPE endpoint lerp above. Its texture is I4, so without
 * this mode ndsRendererHardwareConvertI replicates the intensity into RGB and
 * forces the alpha bit, and the beam draws as an opaque grey ramp on an opaque
 * black surround. Modes 1 and 2 share one bake; this one does not, which is
 * why it needs its own cache-key bit below. */
#define NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA 3u
#define NDS_RENDERER_HW_TEXTURE_KEY_PRIM_ENV_BLEND (1u << 31)
/* Bit 30 is free: key.flags carries render_tile_flags in bits 0-7 and
 * (load_kind << 8), and load_kind's largest value is NDS_RENDERER_TEXTURE_
 * LOADTILE (1u << 6), so nothing above bit 14 is ever set there. */
#define NDS_RENDERER_HW_TEXTURE_KEY_PRIM_RGB_TEXEL0_ALPHA (1u << 30)
#define NDS_RENDERER_MDSFT_CYCLETYPE 20u
#define NDS_RENDERER_CYCLETYPE_MASK (3u << NDS_RENDERER_MDSFT_CYCLETYPE)
#define NDS_RENDERER_CYC_2CYCLE (1u << NDS_RENDERER_MDSFT_CYCLETYPE)
#define NDS_RENDERER_MDSFT_TEXTPERSP 19u
#define NDS_RENDERER_TP_PERSP (1u << NDS_RENDERER_MDSFT_TEXTPERSP)
#define NDS_RENDERER_TEXTPERSP_MASK (1u << NDS_RENDERER_MDSFT_TEXTPERSP)
#define NDS_RENDERER_MDSFT_TEXTFILT 12u
#define NDS_RENDERER_TF_POINT (0u << NDS_RENDERER_MDSFT_TEXTFILT)
#define NDS_RENDERER_TF_BILERP (2u << NDS_RENDERER_MDSFT_TEXTFILT)
#define NDS_RENDERER_TEXTFILT_MASK (3u << NDS_RENDERER_MDSFT_TEXTFILT)
#define NDS_RENDERER_TEXCOORD_FILTER_OFFSET (1 << 4)
#define NDS_RENDERER_ALPHA_COMPARE_MASK 0x3u
#define NDS_RENDERER_ALPHA_COMPARE_THRESHOLD 0x1u
#define NDS_RENDERER_ZSOURCE_PRIM 0x00000004u
#define NDS_RENDERER_ZSOURCE_MASK 0x00000004u
#define NDS_RENDERER_ZMODE_MASK 0x00000c00u
#define NDS_RENDERER_ZMODE_XLU 0x00000800u
#define NDS_RENDERER_ZMODE_DEC 0x00000c00u
#define NDS_RENDERER_CVG_X_ALPHA 0x00001000u
#define NDS_RENDERER_FORCE_BL 0x00004000u
#define NDS_RENDERER_G_BL_A_MEM 1u
#define NDS_RENDERER_BLEND_ALPHA_BITS_MASK 0x3u
#define NDS_RENDERER_BLEND_ALPHA_CYCLE1_SHIFT 18u
#define NDS_RENDERER_BLEND_ALPHA_CYCLE2_SHIFT 16u
#define NDS_RENDERER_POLY_ID_MASK 0x3fu
#define NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK \
    ((3u << 30) | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | \
     GL_TEXTURE_FLIP_S | GL_TEXTURE_FLIP_T)
#define NDS_RENDERER_LIGHT_COLOR_1_MASK (1u << 0)
#define NDS_RENDERER_LIGHT_COLOR_2_MASK (1u << 1)
#define NDS_RENDERER_LIGHT_DIR_1_MASK (1u << 0)
/* BattleShip's fighter display seed emits these light colors before its
 * collision overlay DL: ftdisplaymain.c:205-206. They are a source-shaped
 * fallback for lit lists whose scene callback supplied only light direction. */
#define NDS_RENDERER_LIGHT_COLOR_1_FALLBACK 0x40404000u
#define NDS_RENDERER_LIGHT_COLOR_2_FALLBACK 0xc0c0c000u
#define NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL (1u << 0)
#define NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX (1u << 1)
#define NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE (1u << 2)
#define NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD (1u << 3)
#define NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED (1u << 4)
#define NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH (1u << 5)
#define NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH (1u << 6)
#define NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH (1u << 7)
#define NDS_RENDERER_VERTEX_CONTEXT_PREPARED_MASK \
    (NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL | \
     NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX | \
     NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE)
#if NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state) \
    ((state)->texture_prepare_valid = FALSE)
#define NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state) \
    ((state)->prepared_light_direction_valid = FALSE)
#else
#define NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state) ((void)(state))
#define NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state) ((void)(state))
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 1
static u32 sNdsRendererProfileGXStatusPostVBlank;
static u32 sNdsRendererProfileGXControlPostVBlank;
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 sNdsRendererProfileImmutableListCount;
static u32 sNdsRendererProfileTrustedCommandCount;
static u32 sNdsRendererProfileValidatedCommandCount;
static u32 sNdsRendererProfileTriangleRunReuseCount;
static u32 sNdsRendererProfileTriangleSubmitTicks;
static u32 sNdsRendererProfileVertexSubmitTicks;
static u32 sNdsRendererProfileCi4LutBuildCount;
static u32 sNdsRendererProfileCi4LutReuseCount;
static NDSRendererProfileOwner sNdsRendererProfileOwner =
    NDS_RENDERER_PROFILE_OWNER_NONE;

typedef enum NDSRendererSemanticOutcome
{
    NDS_RENDERER_SEMANTIC_INPUT_REJECT = 0,
    NDS_RENDERER_SEMANTIC_TRANSFORM_REJECT,
    NDS_RENDERER_SEMANTIC_ALPHA_ZERO,
    NDS_RENDERER_SEMANTIC_EMITTED
} NDSRendererSemanticOutcome;

#define NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID (1u << 0)
#define NDS_RENDERER_SEMANTIC_VERTEX_ST_VALID (1u << 1)
#define NDS_RENDERER_SEMANTIC_VERTEX_COLOR_VALID (1u << 2)

typedef struct NDSRendererSemanticVertex
{
    s16 x;
    s16 y;
    s16 z;
    s16 s;
    s16 t;
    u16 color;
    u32 valid_flags;
} NDSRendererSemanticVertex;

typedef struct NDSRendererSemanticEvent
{
    u32 owner;
    u32 owner_occurrence;
    u32 list_ordinal;
    u32 branch_path;
    u32 command_index;
    u32 tri2_half;
    u32 outcome;
    u32 packed;
    u32 vertex_index[3];
    u32 submit_class;
    u32 source_state_hash;
    u32 raw_snapshot_id;
    u32 vertex_matrix_snapshot[3];
    u32 vertex_clip_snapshot[3];
    u32 context_flags;
    u32 source_zbuffered;
    u32 zbuffered;
    u32 raw_submit;
    u32 projected_submit;
    u32 decal_depth;
    u32 prim_depth;
    u32 source_clip_depth;
    u32 poly_alpha;
    u32 poly_fmt;
    u32 alpha_key;
    u32 fog_key;
    u32 fog_color;
    s32 fog_min;
    s32 fog_max;
    u32 fog_status;
    u32 texture_name;
    u32 texture_key_hash;
    u32 texture_params;
    u32 texture_format;
    u32 texture_width;
    u32 texture_height;
    u32 matrix_loaded;
    u32 matrix_mode;
    u32 matrix_generation;
    u32 matrix_signature;
    s32 no_z_depth_before;
    s32 no_z_depth_after;
    u32 no_z_background_before;
    u32 no_z_background_after;
    s32 projected_z[3];
    NDSRendererSemanticVertex vertex[3];
} NDSRendererSemanticEvent;

typedef struct NDSRendererSemanticSourceProvenance
{
    u32 owner_occurrence;
    u32 list_ordinal;
    u32 root_branch_path;
} NDSRendererSemanticSourceProvenance;

static NDSRendererSemanticSourceProvenance
    sNdsRendererSemanticSourceProvenance;
static u32 sNdsRendererSemanticOwnerLastOccurrence[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
static u32 sNdsRendererSemanticOwnerOccurrenceValidMask;
static u32 sNdsRendererSemanticOutputHash;
static u32 sNdsRendererSemanticOutputHash2;
static u32 sNdsRendererSemanticEventCount;
static u32 sNdsRendererSemanticOverflowCount;
static u32 sNdsRendererSemanticLastTextureKeyHash;
static u32 sNdsRendererSemanticLastTextureParams;
static s32 sNdsRendererStageDepthTraceLastBackground;
static s32 sNdsRendererStageDepthTraceLastForeground;
static u32 sNdsRendererStageDepthTraceLastValidMask;

typedef struct NDSRendererProfileOwnerHotLedger
{
    u32 submit_class_count[8];
    u32 material_operation_count;
    u32 texture_change_count;
    u32 run_count;
    u32 semantic_output_hash;
    u32 semantic_output_hash2;
    u32 semantic_event_count;
    u32 semantic_overflow_count;
    u32 semantic_occurrence_count;
    u32 semantic_first_owner_occurrence;
    u32 semantic_first_list_ordinal;
    u32 semantic_first_branch_path;
    u32 semantic_first_command_index;
    u32 semantic_first_tri2_half;
    u32 semantic_first_outcome;
} NDSRendererProfileOwnerHotLedger;

static NDSRendererProfileOwnerHotLedger
    sNdsRendererProfileOwnerHot[NDS_RENDERER_PROFILE_OWNER_COUNT];

static inline NDSRendererProfileOwnerHotLedger *
ndsRendererProfileCurrentOwner(void)
{
    return ((u32)sNdsRendererProfileOwner <
            (u32)NDS_RENDERER_PROFILE_OWNER_COUNT) ?
        &sNdsRendererProfileOwnerHot[(u32)sNdsRendererProfileOwner] : NULL;
}

static void ndsRendererSemanticHash1Word(u32 *hash, u32 value)
{
    u32 i;

    for (i = 0u; i < sizeof(value); i++)
    {
        *hash ^= (value >> (i * 8u)) & 0xffu;
        *hash *= 16777619u;
    }
}

static void ndsRendererSemanticHash2Word(u32 *hash, u32 value)
{
    u32 mixed = *hash;

    mixed ^= value + 0x9e3779b9u + (mixed << 6) + (mixed >> 2);
    mixed = ((mixed << 13) | (mixed >> 19)) * 0x85ebca6bu;
    *hash = mixed;
}

static u32 ndsRendererSemanticBranchPath(u32 parent_path,
                                         u32 command_index,
                                         u32 depth,
                                         u32 branch_is_jump)
{
    u32 hash = 2166136261u;

    ndsRendererSemanticHash1Word(&hash, 0x4252414eu);
    ndsRendererSemanticHash1Word(&hash, parent_path);
    ndsRendererSemanticHash1Word(&hash, command_index);
    ndsRendererSemanticHash1Word(&hash, depth);
    ndsRendererSemanticHash1Word(&hash, branch_is_jump);
    return (hash != 0u) ? hash : 1u;
}

static u32 ndsRendererSemanticSourceStateHash(
    const NDSRendererStats *stats)
{
    u32 hash = 2166136261u;
    u32 i;

#define NDS_RENDERER_SEMANTIC_STATE_WORD(value) \
    ndsRendererSemanticHash1Word(&hash, (u32)(value))
    NDS_RENDERER_SEMANTIC_STATE_WORD(0x53524331u);
    if (stats == NULL)
    {
        NDS_RENDERER_SEMANTIC_STATE_WORD(0xffffffffu);
        return hash;
    }

    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->othermode_h);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->othermode_l);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->geometry_mode);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_mask);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_kind);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_scale_s);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_scale_t);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_level);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_on);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_xparam);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_state_flags);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_image);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_format);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_size);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_image_width);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_set_tile_count);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tlut_image);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tlut_count);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tlut_tile);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_format);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_size);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_line);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_tmem);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_palette);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_cms);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_cmt);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_masks);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_maskt);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_shifts);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_shiftt);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_flags);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_tile);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_block_uls);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_block_ult);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_block_lrs);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_block_dxt);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_texels);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_size_tile);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_size_uls);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_size_ult);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_size_lrs);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_size_lrt);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_width);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_height);
    for (i = 0u; i < NDS_RENDERER_TILE_COUNT; i++)
    {
        const NDSRendererTileState *tile = &stats->texture_tiles[i];

        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->set_seen);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->size_seen);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->format);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->size);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->line);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->tmem);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->palette);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->cms);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->cmt);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->masks);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->maskt);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->shifts);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->shiftt);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->uls);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->ult);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->lrs);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->lrt);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->width);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->height);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->flags);
    }
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_sequence);
    for (i = 0u; i < NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT; i++)
    {
        const NDSRendererTextureLoadState *load =
            &stats->texture_loads[i];

        NDS_RENDERER_SEMANTIC_STATE_WORD(load->image);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->sequence);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->image_width);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_uls);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_ult);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_lrs);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_dxt);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_texels);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_tmem);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->valid);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->image_format);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->image_size);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_kind);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_tile);
    }
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_combine_w0);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_combine_w1);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_combine_count);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->prim_color);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->prim_min_level);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->prim_lod_fraction);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->env_color);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->blend_color);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_color_1);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_color_2);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_color_mask);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_dir_x);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_dir_y);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_dir_z);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_dir_mask);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->prim_depth);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->prim_depth_delta);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->fog_color);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->fog_min);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->fog_max);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->fog_status);
#undef NDS_RENDERER_SEMANTIC_STATE_WORD
    return hash;
}

static void ndsRendererSemanticHashEvent(u32 *hash1,
                                         u32 *hash2,
                                         const NDSRendererSemanticEvent *event)
{
    u32 i;

#define NDS_RENDERER_SEMANTIC_EVENT_WORD(value) do { \
        u32 semantic_word = (u32)(value); \
        ndsRendererSemanticHash1Word(hash1, semantic_word); \
        ndsRendererSemanticHash2Word(hash2, semantic_word); \
    } while (0)
    NDS_RENDERER_SEMANTIC_EVENT_WORD(0x53454d31u);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->owner);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->owner_occurrence);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->list_ordinal);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->branch_path);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->command_index);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->tri2_half);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->outcome);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->packed);
    for (i = 0u; i < 3u; i++)
    {
        NDS_RENDERER_SEMANTIC_EVENT_WORD(event->vertex_index[i]);
    }
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->submit_class);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->source_state_hash);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->raw_snapshot_id);
    for (i = 0u; i < 3u; i++)
    {
        NDS_RENDERER_SEMANTIC_EVENT_WORD(
            event->vertex_matrix_snapshot[i]);
        NDS_RENDERER_SEMANTIC_EVENT_WORD(
            event->vertex_clip_snapshot[i]);
    }
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->context_flags);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->source_zbuffered);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->zbuffered);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->raw_submit);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->projected_submit);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->decal_depth);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->prim_depth);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->source_clip_depth);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->poly_alpha);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->poly_fmt);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->alpha_key);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->fog_key);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->fog_color);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->fog_min);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->fog_max);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->fog_status);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->texture_name);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->texture_key_hash);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->texture_params);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->matrix_loaded);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->matrix_mode);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->matrix_generation);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->matrix_signature);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->no_z_depth_before);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->no_z_depth_after);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->no_z_background_before);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->no_z_background_after);
    for (i = 0u; i < 3u; i++)
    {
        const NDSRendererSemanticVertex *vertex = &event->vertex[i];

        NDS_RENDERER_SEMANTIC_EVENT_WORD(event->projected_z[i]);
        NDS_RENDERER_SEMANTIC_EVENT_WORD((u16)vertex->x);
        NDS_RENDERER_SEMANTIC_EVENT_WORD((u16)vertex->y);
        NDS_RENDERER_SEMANTIC_EVENT_WORD((u16)vertex->z);
        NDS_RENDERER_SEMANTIC_EVENT_WORD((u16)vertex->s);
        NDS_RENDERER_SEMANTIC_EVENT_WORD((u16)vertex->t);
        NDS_RENDERER_SEMANTIC_EVENT_WORD(vertex->color);
        NDS_RENDERER_SEMANTIC_EVENT_WORD(vertex->valid_flags);
    }
    NDS_RENDERER_SEMANTIC_EVENT_WORD(0x454e4431u);
#undef NDS_RENDERER_SEMANTIC_EVENT_WORD
}

static void ndsRendererStageDepthTraceEvent(
    const NDSRendererSemanticEvent *event)
{
    volatile NDSRendererStageDepthTrace *trace;
    u32 trace_index;
    u32 phase = 0u;
    u32 hash;
    u32 i;

    if ((event == NULL) ||
        (event->owner != NDS_RENDERER_PROFILE_OWNER_STAGE) ||
        (event->outcome != NDS_RENDERER_SEMANTIC_EMITTED))
    {
        return;
    }
    trace_index = gNdsRendererStageDepthTraceCount;
    if (trace_index >= NDS_RENDERER_STAGE_DEPTH_TRACE_CAPACITY)
    {
        gNdsRendererStageDepthTraceOverflowCount++;
        return;
    }
    if (event->submit_class ==
        NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z_CLASS)
    {
        s32 depth = event->projected_z[0];
        u32 valid_bit;
        volatile u32 *count;
        volatile s32 *minimum;
        volatile s32 *maximum;
        s32 *last;

        phase = (event->no_z_background_before != FALSE) ? 1u : 2u;
        valid_bit = 1u << phase;
        if (phase == 1u)
        {
            count = &gNdsRendererStageDepthTraceBackgroundCount;
            minimum = &gNdsRendererStageDepthTraceBackgroundMin;
            maximum = &gNdsRendererStageDepthTraceBackgroundMax;
            last = &sNdsRendererStageDepthTraceLastBackground;
        }
        else
        {
            count = &gNdsRendererStageDepthTraceForegroundCount;
            minimum = &gNdsRendererStageDepthTraceForegroundMin;
            maximum = &gNdsRendererStageDepthTraceForegroundMax;
            last = &sNdsRendererStageDepthTraceLastForeground;
        }
        if ((sNdsRendererStageDepthTraceLastValidMask & valid_bit) == 0u)
        {
            *minimum = depth;
            *maximum = depth;
            sNdsRendererStageDepthTraceLastValidMask |= valid_bit;
        }
        else
        {
            if (depth >= *last)
            {
                gNdsRendererStageDepthTraceNoZCollisionCount++;
            }
            if (depth < *minimum) { *minimum = depth; }
            if (depth > *maximum) { *maximum = depth; }
        }
        *last = depth;
        (*count)++;
    }

    trace = &gNdsRendererStageDepthTrace[trace_index];
    trace->owner_occurrence = event->owner_occurrence;
    trace->list_ordinal = event->list_ordinal;
    trace->branch_path = event->branch_path;
    trace->command_index = event->command_index;
    for (i = 0u; i < 3u; i++)
    {
        trace->projected_z[i] = event->projected_z[i];
        trace->submitted_z[i] = event->vertex[i].z;
    }
    trace->submit_class = (u8)event->submit_class;
    trace->source_zbuffered = (u8)event->source_zbuffered;
    trace->no_z_phase = (u8)phase;
    trace->tri2_half = (u8)event->tri2_half;
    if (event->submit_class < 8u)
    {
        gNdsRendererStageDepthTraceClassCount[event->submit_class]++;
    }

    hash = (trace_index == 0u) ?
        2166136261u : gNdsRendererStageDepthTraceHash;
    ndsRendererSemanticHash1Word(&hash, 0x53445031u);
    ndsRendererSemanticHash1Word(&hash, trace_index);
    ndsRendererSemanticHash1Word(&hash, event->submit_class);
    ndsRendererSemanticHash1Word(&hash, event->source_zbuffered);
    ndsRendererSemanticHash1Word(&hash, phase);
    for (i = 0u; i < 3u; i++)
    {
        ndsRendererSemanticHash1Word(&hash, (u32)event->projected_z[i]);
    }
    gNdsRendererStageDepthTraceHash = hash;
    gNdsRendererStageDepthTraceCount++;
}

static void ndsRendererSemanticCommitEvent(
    const NDSRendererSemanticEvent *event)
{
    NDSRendererProfileOwnerHotLedger *owner =
        ndsRendererProfileCurrentOwner();
    u32 owner_hash1;
    u32 owner_hash2;
    u32 frame_hash1;
    u32 frame_hash2;
    u32 frame_index;

    if ((event == NULL) || (owner == NULL))
    {
        return;
    }
    ndsRendererStageDepthTraceEvent(event);
    if (owner->semantic_event_count == 0u)
    {
        owner->semantic_first_owner_occurrence = event->owner_occurrence;
        owner->semantic_first_list_ordinal = event->list_ordinal;
        owner->semantic_first_branch_path = event->branch_path;
        owner->semantic_first_command_index = event->command_index;
        owner->semantic_first_tri2_half = event->tri2_half;
        owner->semantic_first_outcome = event->outcome;
    }

    owner_hash1 = (owner->semantic_event_count == 0u) ?
        2166136261u : owner->semantic_output_hash;
    owner_hash2 = (owner->semantic_event_count == 0u) ?
        0x9e3779b9u : owner->semantic_output_hash2;
    ndsRendererSemanticHashEvent(&owner_hash1, &owner_hash2, event);
    owner->semantic_output_hash = owner_hash1;
    owner->semantic_output_hash2 = owner_hash2;
    owner->semantic_event_count++;

    frame_index = sNdsRendererSemanticEventCount;
    frame_hash1 = (frame_index == 0u) ?
        2166136261u : sNdsRendererSemanticOutputHash;
    frame_hash2 = (frame_index == 0u) ?
        0x9e3779b9u : sNdsRendererSemanticOutputHash2;
    ndsRendererSemanticHashEvent(&frame_hash1, &frame_hash2, event);
    sNdsRendererSemanticOutputHash = frame_hash1;
    sNdsRendererSemanticOutputHash2 = frame_hash2;
    if (frame_index < NDS_RENDERER_SEMANTIC_TRACE_CAPACITY)
    {
        gNdsRendererSemanticPrefixHash[frame_index] = frame_hash1;
        gNdsRendererSemanticPrefixHash2[frame_index] = frame_hash2;
    }
    else
    {
        sNdsRendererSemanticOverflowCount++;
        owner->semantic_overflow_count++;
    }
    sNdsRendererSemanticEventCount++;
}
#endif

/* Frame begin is shared by software and GX builds.  Keep these diagnostics
 * defined in both; only the hardware queue increments them. */
volatile u32 gNdsRendererIFCommonCloudQueuedCount;
volatile u32 gNdsRendererIFCommonCloudEmittedCount;

#if NDS_RENDERER_HW_TRIANGLES
typedef enum NDSRendererHWSubmitClass
{
    NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX = 0,
    NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX,
    NDS_RENDERER_HW_SUBMIT_REJECT,
    NDS_RENDERER_HW_SUBMIT_CLASS_COUNT
} NDSRendererHWSubmitClass;

#if NDS_RENDERER_PROFILE_LEVEL == 0
#if NDS_RENDERER_FAST_RUN_DEFAULT
volatile u32 gNdsRendererFastRunMode =
    NDS_RENDERER_FAST_RUN_DEFAULT;
#else
volatile u32 gNdsRendererFastRunMode =
    NDS_RENDERER_FAST_RUN_STAGE_TEXTURE_SITES;
#endif
#elif NDS_RENDERER_FAST_RUN_DEFAULT
volatile u32 gNdsRendererFastRunMode =
    NDS_RENDERER_FAST_RUN_DEFAULT;
#else
volatile u32 gNdsRendererFastRunMode = NDS_RENDERER_FAST_RUN_GENERIC;
#endif
volatile u32 gNdsRendererFastRunCount;
volatile u32 gNdsRendererFastTriangleCount;
volatile u32 gNdsRendererFastOwnerTriangleCount[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
volatile u32 gNdsRendererFastFallbackCount[3];
/* R2-07 E2. The two reject-reason encodings already name every way the stage
 * owner can refuse -- for the owner: 1/2 early bails, 100 the generated segment
 * 0, 200+run an ApplyStateSpan refusal, 300+run a PrepareRun refusal,
 * 400+binding, 3/4 the post-loop checks; and inside PrepareRun a further 1..6.
 * Both were gated on NDS_RENDERER_PROFILE_LEVEL == 1 and the tick-HUD lane is
 * level 0, so every writer compiled out and --gc-sections dropped the words
 * entirely (absent from the ELF, which is why the first read printed nothing).
 *
 * Let the route probe enable just this encoding rather than raising the profile
 * level, which would compile in a great deal of unrelated instrumentation and
 * change what is being measured. Defined HERE, above both writers:
 * ndsRendererNativeStagePrepareRun precedes the owner by ~2,000 lines, and an
 * undefined macro in #if evaluates to 0 silently -- those seven sites would
 * have compiled out with no diagnostic. */
#define NDS_TASK36_REJECT_TRACE \
    (NDS_TASK36_HW_COMPOSE && \
     ((NDS_RENDERER_PROFILE_LEVEL == 1) || NDS_R2_STAGE_ROUTE_PROBE))

/* Counting form of the reject latch, live only with the route probe. The latch
 * is reset per prepare and therefore cannot answer "how many, and which"; this
 * can. Compiles to nothing at profile level 1 alone, so the level-1 counter
 * block keeps its existing cost. */
#if NDS_R2_STAGE_ROUTE_PROBE
extern volatile u32 gNdsR2StageRejectCounts[7];
#define NDS_R2_STAGE_REJECT_COUNT(reason) (gNdsR2StageRejectCounts[reason]++)
#else
#define NDS_R2_STAGE_REJECT_COUNT(reason) ((void)0)
#endif

#if NDS_R2_STAGE_ROUTE_PROBE && (NDS_RENDERER_PROFILE_LEVEL != 1) && \
    NDS_TASK36_HW_COMPOSE
/* The words themselves. Their normal declarations sit inside the level-1
 * counter block below; declaring just these two keeps the probe from dragging
 * in the whole block. */
volatile u32 gNdsRendererTask36RendererRejectReason;
volatile u32 gNdsRendererTask36PrepareRunRejectReason;
#endif
#if NDS_R2_STAGE_ROUTE_PROBE
/* R2-07 E3's allocator map and refused-request stash are DELETED, not disabled.
 * They answered their question -- the refusal was neither space, fragmentation,
 * slots nor binding, and a full `glResetTextures()` at scene entry fixes it --
 * and that answer is written up in `docs/BUGS.md` with the numbers. What ships
 * instead is the fix plus two permanent counters
 * (`gNdsRendererSceneTextureVramResetCount` / `...Enable`). Keeping the
 * instrument would have cost a `glGlob` read inside
 * `ndsRendererHardwareResolveOrBindTexture` that the native-stage field
 * certificate correctly refuses to classify.
 *
 * Cache census at the first texture rejection. See ndsRendererHardwareRejectTexture. */
volatile u32 gNdsR2TexRejectCensusValid;
volatile u32 gNdsR2TexRejectCensusFree;
volatile u32 gNdsR2TexRejectCensusLive;
volatile u32 gNdsR2TexRejectCensusPinned;
volatile u32 gNdsR2TexRejectCensusThisFrame;
volatile u32 gNdsR2TexRejectCensusEvictable;
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
volatile u32 gNdsRendererM3PreflightAttemptCount;
volatile u32 gNdsRendererM3PreflightSuccessCount;
volatile u32 gNdsRendererM3PreflightFallbackCount;
volatile u32 gNdsRendererM3SegmentCount;
volatile u32 gNdsRendererM3SegmentMask;
volatile u32 gNdsRendererM3PostArmFailureCount;
volatile u32 gNdsRendererM3DObjCount;
volatile u32 gNdsRendererM3BindingCount;
volatile u32 gNdsRendererM3RunCount;
volatile u32 gNdsRendererM3TriangleCount;
volatile u32 gNdsRendererM3ResidentEpochCount;
volatile u32 gNdsRendererM3MaterialShadowCount;
volatile u32 gNdsRendererM3MaterialCommitCount;
volatile u32 gNdsRendererM3CrossRunCount;
volatile u32 gNdsRendererM3CrossTriangleCount;
volatile u32 gNdsRendererM3CrossForeignCornerCount;
volatile u32 gNdsRendererM3TopologyFullValidationCount;
volatile u32 gNdsRendererM3TopologyCacheHitCount;
volatile u32 gNdsRendererM3TopologyStampMismatchCount;
#if NDS_TASK36_HW_COMPOSE
volatile u32 gNdsRendererTask36HardwareComposedDObjCount;
volatile u32 gNdsRendererTask36CameraLoadCount;
volatile u32 gNdsRendererTask36WorldMultCount;
volatile u32 gNdsRendererTask36AdapterRejectReason;
volatile u32 gNdsRendererTask36RendererRejectReason;
volatile u32 gNdsRendererTask36PrepareRunRejectReason;
volatile u32 gNdsRendererTask36RigidConstancyMismatchCount;
volatile u32 gNdsRendererTask36ObservedDynamicMaskLo;
volatile u32 gNdsRendererTask36ObservedDynamicMaskHi;
#if NDS_TASK44_STAGE_STEADY
volatile u32 gNdsRendererTask44SteadyAdmitCount;
volatile u32 gNdsRendererTask44RevalidateCount;
volatile u32 gNdsRendererTask44AdmissionGeneration;
#endif
#if NDS_TASK36_HW_COMPOSE == 2
volatile u32 gNdsRendererTask36ReplayState;
volatile u32 gNdsRendererTask36BakeAttemptCount;
volatile u32 gNdsRendererTask36BakeSuccessCount;
volatile u32 gNdsRendererTask36BakeFailureCount;
volatile u32 gNdsRendererTask36ReplayFrameCount;
volatile u32 gNdsRendererTask36ReplaySegmentCount;
volatile u32 gNdsRendererTask36ReplayRunCount;
volatile u32 gNdsRendererTask36ReplayWordCount;
volatile u32 gNdsRendererTask36ReplayFallbackCount;
volatile u32 gNdsRendererTask36ReplayArenaRejectCount;
volatile u32 gNdsRendererTask36ReplayMaterialRejectCount;
/* BUGS.md #9. Nonzero exactly while the live projection differs from the one
 * the stream was baked against -- so it should read 0 for a whole match and
 * start counting the moment the paused player-zoom camera moves the FOV. */
volatile u32 gNdsRendererTask36ReplayProjectionRejectCount;
volatile u32 gNdsRendererTask36ReplayCaptureWordCount;
#endif
#endif
#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
volatile u32 gNdsRendererM3GeneratedSegment0AttemptCount;
volatile u32 gNdsRendererM3GeneratedSegment0SuccessCount;
volatile u32 gNdsRendererM3GeneratedSegment0PreGxFallbackCount;
volatile u32 gNdsRendererM3GeneratedSegment0RunCount;
volatile u32 gNdsRendererM3GeneratedSegment0TriangleCount;
volatile u32 gNdsRendererM3GeneratedSegment0EpochCount;
volatile u32 gNdsRendererM3GeneratedSegment0MaterialCount;
volatile u32 gNdsRendererM3GeneratedSegment0CertificateValidationCount;
#if NDS_RENDERER_M3_PHASE0_PROFILE
volatile u32 gNdsRendererM3GeneratedSegment0ShadowDenseCount;
volatile u32 gNdsRendererM3GeneratedSegment0ShadowStateEntryCount;
volatile u32 gNdsRendererM3GeneratedSegment0ShadowSyncCount;
volatile u32 gNdsRendererM3GeneratedSegment0ShadowFieldComparisonCount;
volatile u32 gNdsRendererM3GeneratedSegment0ShadowMismatchCount;
volatile u32 gNdsRendererM3GeneratedSegment0ShadowFaultInjectedCount;
volatile u32 gNdsRendererM3GeneratedSegment0ShadowFaultRejectedCount;
volatile u32 gNdsRendererM3GeneratedSegment0ShadowLiveFaultInjectedCount;
volatile u32 gNdsRendererM3GeneratedSegment0ShadowLiveFaultRejectedCount;
volatile u32 gNdsRendererM3GeneratedSegment0ShadowLiveFaultRevalidatedCount;
#endif
#endif
#if NDS_RENDERER_M2_DETAILED_LEDGER
volatile u32 gNdsRendererM2ShadeEpochCount;
volatile u32 gNdsRendererM2ShadeKeyHitCount;
volatile u32 gNdsRendererM2ShadeResidentHitCount;
volatile u32 gNdsRendererM2ShadeHashCollisionCount;
volatile u32 gNdsRendererM2ShadeDenseVisitCount;
volatile u32 gNdsRendererM2ShadeComputeCount;
volatile u32 gNdsRendererM2ShadeLutComputeCount;
volatile u32 gNdsRendererM2ShadePreparedComputeCount;
volatile u32 gNdsRendererM2ShadeAliasCopyCount;
volatile u32 gNdsRendererM2ShadeMaterialPackCount;
volatile u32 gNdsRendererM2ShadeOwnerEpochCount[2];
volatile u32 gNdsRendererM2ShadeOwnerKeyHitCount[2];
volatile u32 gNdsRendererM2ShadeOwnerResidentHitCount[2];
static u32 sNdsRendererM2ShadeEpochCount;
static u32 sNdsRendererM2ShadeKeyHitCount;
static u32 sNdsRendererM2ShadeResidentHitCount;
static u32 sNdsRendererM2ShadeHashCollisionCount;
static u32 sNdsRendererM2ShadeDenseVisitCount;
static u32 sNdsRendererM2ShadeComputeCount;
static u32 sNdsRendererM2ShadeLutComputeCount;
static u32 sNdsRendererM2ShadePreparedComputeCount;
static u32 sNdsRendererM2ShadeAliasCopyCount;
static u32 sNdsRendererM2ShadeMaterialPackCount;
static u32 sNdsRendererM2ShadeOwnerEpochCount[2];
static u32 sNdsRendererM2ShadeOwnerKeyHitCount[2];
static u32 sNdsRendererM2ShadeOwnerResidentHitCount[2];
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
volatile u32 gNdsRendererM3TopologyFaultInjectionCount;
volatile u32 gNdsRendererM3TopologyFaultRevalidationCount;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
volatile u32 gNdsRendererM3Phase0PreflightTicks;
volatile u32 gNdsRendererM3Phase0PrepareRunTicks;
volatile u32 gNdsRendererM3Phase0VertexPrepareTicks;
volatile u32 gNdsRendererM3Phase0NearTransformTicks;
volatile u32 gNdsRendererM3Phase0RunTransitionTicks;
volatile u32 gNdsRendererM3Phase0RawEmitTicks;
volatile u32 gNdsRendererM3Phase0RangeEmitTicks;
volatile u32 gNdsRendererM3Phase0NoZEmitTicks;
volatile u32 gNdsRendererM3Phase0NoZMatrixTicks;
volatile u32 gNdsRendererM3Phase0AccountingTicks;
volatile u32 gNdsRendererM3Phase0CommitTicks;
volatile u32 gNdsRendererM3Phase0TimerReadCount;
volatile u32 gNdsRendererM3Phase0TimerSpanCount;
volatile u32 gNdsRendererM3Phase0CalibrationTicks;
volatile u32 gNdsRendererM3Phase0CalibrationIntervals;
volatile u32 gNdsRendererM3Phase0PreparedDenseCount;
volatile u32 gNdsRendererM3Phase0NearTransformCount;
volatile u32 gNdsRendererM3Phase0NoZMatrixCount;
volatile u32 gNdsRendererM3ResidualPrepareTicks;
volatile u32 gNdsRendererM3ResidualVertexTicks;
volatile u32 gNdsRendererM3ResidualNearTicks;
volatile u32 gNdsRendererM3ResidualKeyTicks;
volatile u32 gNdsRendererM3ResidualKeyHitCount;
volatile u32 gNdsRendererM3ResidualKeyMissCount;
volatile u32 gNdsRendererM3ResidualKeyByteCount;
volatile u32 gNdsRendererM3ResidualRunCount;
volatile u32 gNdsRendererM3ResidualDenseCount;
volatile u32 gNdsRendererM3ResidualNearCount;
volatile u32 gNdsRendererM3G2TextureParamWriteCount;
volatile u32 gNdsRendererM3G2TextureParamSkipCount;
volatile u32 gNdsRendererM3G2MatrixModeWriteCount;
volatile u32 gNdsRendererM3G2MatrixModeSkipCount;
volatile u32 gNdsRendererM3G2PolyFmtWriteCount;
volatile u32 gNdsRendererM3G2PolyFmtSkipCount;

static inline u32 ndsRendererM3Phase0Tick(void)
{
    gNdsRendererM3Phase0TimerReadCount++;
    return cpuGetTiming();
}

typedef struct NDSRendererM3ResidualKey
{
    const void *asset_bases[NDS_RENDERER_NATIVE_STAGE_ASSET_COUNT];
    NDSRendererNativeMaterial materials[NDS_RENDERER_NATIVE_STAGE_MATERIAL_COUNT];
    NDSRendererConfig config;
    u32 topology_generation;
    u32 topology_stamp;
    u32 static_prepared_count;
    u32 static_prepared_bytes;
    u32 static_arm_count;
} NDSRendererM3ResidualKey;

static NDSRendererM3ResidualKey sNdsRendererM3ResidualKey;
static u32 sNdsRendererM3ResidualKeyValid;

static inline void ndsRendererM3Phase0FinishSpan(
    volatile u32 *bucket, u32 start)
{
    *bucket += ndsRendererM3Phase0Tick() - start;
    gNdsRendererM3Phase0TimerSpanCount++;
}

static void ndsRendererM3Phase0Reset(void)
{
    u32 calibration_tick;
    u32 calibration_index;

    gNdsRendererM3Phase0PreflightTicks = 0u;
    gNdsRendererM3Phase0PrepareRunTicks = 0u;
    gNdsRendererM3Phase0VertexPrepareTicks = 0u;
    gNdsRendererM3Phase0NearTransformTicks = 0u;
    gNdsRendererM3Phase0RunTransitionTicks = 0u;
    gNdsRendererM3Phase0RawEmitTicks = 0u;
    gNdsRendererM3Phase0RangeEmitTicks = 0u;
    gNdsRendererM3Phase0NoZEmitTicks = 0u;
    gNdsRendererM3Phase0NoZMatrixTicks = 0u;
    gNdsRendererM3Phase0AccountingTicks = 0u;
    gNdsRendererM3Phase0CommitTicks = 0u;
    gNdsRendererM3Phase0TimerReadCount = 0u;
    gNdsRendererM3Phase0TimerSpanCount = 0u;
    gNdsRendererM3Phase0CalibrationTicks = 0u;
    gNdsRendererM3Phase0CalibrationIntervals = 16u;
    gNdsRendererM3Phase0PreparedDenseCount = 0u;
    gNdsRendererM3Phase0NearTransformCount = 0u;
    gNdsRendererM3Phase0NoZMatrixCount = 0u;
    gNdsRendererM3ResidualPrepareTicks = 0u;
    gNdsRendererM3ResidualVertexTicks = 0u;
    gNdsRendererM3ResidualNearTicks = 0u;
    gNdsRendererM3ResidualKeyTicks = 0u;
    gNdsRendererM3ResidualKeyHitCount = 0u;
    gNdsRendererM3ResidualKeyMissCount = 0u;
    gNdsRendererM3ResidualKeyByteCount = sizeof(sNdsRendererM3ResidualKey);
    gNdsRendererM3ResidualRunCount = 0u;
    gNdsRendererM3ResidualDenseCount = 0u;
    gNdsRendererM3ResidualNearCount = 0u;
    gNdsRendererM3G2TextureParamWriteCount = 0u;
    gNdsRendererM3G2TextureParamSkipCount = 0u;
    gNdsRendererM3G2MatrixModeWriteCount = 0u;
    gNdsRendererM3G2MatrixModeSkipCount = 0u;
    gNdsRendererM3G2PolyFmtWriteCount = 0u;
    gNdsRendererM3G2PolyFmtSkipCount = 0u;

    calibration_tick = ndsRendererM3Phase0Tick();
    for (calibration_index = 0u;
         calibration_index < gNdsRendererM3Phase0CalibrationIntervals;
         calibration_index++)
    {
        u32 next_tick = ndsRendererM3Phase0Tick();

        gNdsRendererM3Phase0CalibrationTicks += next_tick - calibration_tick;
        calibration_tick = next_tick;
    }
}

static void ndsRendererM3MeasureResidualKey(
    const NDSRendererNativeStageFrame *frame)
{
    u32 start = cpuGetTiming();
    s32 hit = FALSE;

    if ((sNdsRendererM3ResidualKeyValid != FALSE) &&
        (sNdsRendererM3ResidualKey.topology_generation ==
         frame->topology_generation) &&
        (sNdsRendererM3ResidualKey.topology_stamp == frame->topology_stamp) &&
        (sNdsRendererM3ResidualKey.static_prepared_count ==
         gNdsRendererBattleStaticTexturePreparedCount) &&
        (sNdsRendererM3ResidualKey.static_prepared_bytes ==
         gNdsRendererBattleStaticTexturePreparedBytes) &&
        (sNdsRendererM3ResidualKey.static_arm_count ==
         gNdsRendererBattleStaticTextureArmCount) &&
        (memcmp(sNdsRendererM3ResidualKey.asset_bases, frame->asset_bases,
                sizeof(sNdsRendererM3ResidualKey.asset_bases)) == 0) &&
        (memcmp(sNdsRendererM3ResidualKey.materials, frame->materials,
                sizeof(sNdsRendererM3ResidualKey.materials)) == 0) &&
        (memcmp(&sNdsRendererM3ResidualKey.config, frame->config,
                sizeof(sNdsRendererM3ResidualKey.config)) == 0))
    {
        hit = TRUE;
    }
    else
    {
        memcpy(sNdsRendererM3ResidualKey.asset_bases, frame->asset_bases,
               sizeof(sNdsRendererM3ResidualKey.asset_bases));
        memcpy(sNdsRendererM3ResidualKey.materials, frame->materials,
               sizeof(sNdsRendererM3ResidualKey.materials));
        sNdsRendererM3ResidualKey.config = *frame->config;
        sNdsRendererM3ResidualKey.topology_generation =
            frame->topology_generation;
        sNdsRendererM3ResidualKey.topology_stamp = frame->topology_stamp;
        sNdsRendererM3ResidualKey.static_prepared_count =
            gNdsRendererBattleStaticTexturePreparedCount;
        sNdsRendererM3ResidualKey.static_prepared_bytes =
            gNdsRendererBattleStaticTexturePreparedBytes;
        sNdsRendererM3ResidualKey.static_arm_count =
            gNdsRendererBattleStaticTextureArmCount;
        sNdsRendererM3ResidualKeyValid = TRUE;
    }
    gNdsRendererM3ResidualKeyTicks = cpuGetTiming() - start;
    gNdsRendererM3ResidualKeyHitCount = (hit != FALSE) ? 1u : 0u;
    gNdsRendererM3ResidualKeyMissCount = (hit != FALSE) ? 0u : 1u;
}
#endif
#endif
/* Task 53: the arena-staleness counter is declared at file scope here (outside
 * the profile-1 block above) so it exists at profile-0 too. Its use site in
 * ndsRendererTask36ReplayBeginFrame is gated only on NDS_TASK53_REPLAY_ARENA_FIX
 * (no profile gate), so the definition must match. The staleness detector is a
 * regression catch -- proof the relaxed guard is admitting frames the legacy
 * strict guard would have blocked -- not a profiling instrument. */
#if NDS_TASK36_HW_COMPOSE == 2 && NDS_TASK53_REPLAY_ARENA_FIX
volatile u32 gNdsRendererTask36ReplayArenaStaleCount;
#endif
static NDSRendererProfileOwner sNdsRendererRuntimeOwner =
    NDS_RENDERER_PROFILE_OWNER_NONE;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
static volatile NDSRendererOwnerProfile *ndsRendererProfileM2Owner(void)
{
    if ((sNdsRendererRuntimeOwner != NDS_RENDERER_PROFILE_OWNER_MARIO) &&
        (sNdsRendererRuntimeOwner != NDS_RENDERER_PROFILE_OWNER_FOX))
    {
        return NULL;
    }
    return &gNdsRendererProfileOwners[(u32)sNdsRendererRuntimeOwner];
}

static void ndsRendererProfileM2FinishProduction(
    volatile NDSRendererOwnerProfile *owner,
    u32 total_start,
    u32 lighting_before,
    u32 root_gx_before,
    u32 run_prepare_before,
    u32 emit_account_before,
    u32 success)
{
    u32 total_ticks;
    u32 measured_ticks;

    if (owner == NULL)
    {
        return;
    }
    total_ticks = cpuGetTiming() - total_start;
    measured_ticks =
        (owner->m2_lighting_shading_ticks - lighting_before) +
        (owner->m2_root_gx_ticks - root_gx_before) +
        (owner->m2_run_prepare_ticks - run_prepare_before) +
        (owner->m2_corner_emit_account_ticks - emit_account_before);
    owner->m2_production_total_ticks += total_ticks;
    if (total_ticks >= measured_ticks)
    {
        owner->m2_production_preflight_state_ticks +=
            total_ticks - measured_ticks;
    }
    else
    {
        owner->m2_production_phase_overlap_count++;
    }
    if (success != FALSE)
    {
        owner->m2_production_success_count++;
    }
    else
    {
        owner->m2_production_failure_count++;
    }
}
#endif
static u32 sNdsRendererFastOwnerEnabled;
static u32 sNdsRendererStageTextureSitesEnabled;
static u32 sNdsRendererFastRunCount;
static u32 sNdsRendererFastTriangleCount;
static u32 sNdsRendererFastOwnerTriangleCount[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
static u32 sNdsRendererFastFallbackCount[3];

/* Direct immutable TRI-run records are topology only.  Live vertex, matrix,
 * material, texture, and light state remains in the traversal object and is
 * rebound by the existing exact path.  The cache is reset with reloc/source
 * caches at scene boundaries, so it never survives pointer ownership changes. */
#define NDS_RENDERER_DIRECT_RAW_PLAN_COUNT 128u
#define NDS_RENDERER_DIRECT_RAW_ENTRY_COUNT 384u

typedef struct NDSRendererDirectRawEntry
{
    u32 required_mask;
    u8 indices[6];
    u8 triangle_count;
    u8 reserved;
} NDSRendererDirectRawEntry;

typedef struct NDSRendererDirectRawPlan
{
    const Gfx *source;
    u16 entry_offset;
    u16 command_count;
    u16 triangle_count;
    u16 reserved;
    u32 first_w0;
    u32 first_w1;
    u32 last_w0;
    u32 last_w1;
} NDSRendererDirectRawPlan;

static NDSRendererDirectRawPlan
    sNdsRendererDirectRawPlans[NDS_RENDERER_DIRECT_RAW_PLAN_COUNT];
static NDSRendererDirectRawEntry
    sNdsRendererDirectRawEntries[NDS_RENDERER_DIRECT_RAW_ENTRY_COUNT];
static u32 sNdsRendererDirectRawEntryCount;

_Static_assert(
    (sizeof(sNdsRendererDirectRawPlans) +
     sizeof(sNdsRendererDirectRawEntries)) <= (8u * 1024u),
    "direct raw topology cache must remain within 8 KiB");

static u32 sNdsRendererHardwareSubmitted;
/* GO peaks at ten traffic/flare SObjs plus three announce glyphs. Three spare
 * entries keep the exact source painter order fail-closed. */
#define NDS_RENDERER_IFCOMMON_CLOUD_QUEUE_COUNT 16u
typedef struct NDSRendererIFCommonCloudDraw
{
    u32 texture_name;
    s32 x_q16;
    s32 y_q16;
    s32 width_q16;
    s32 height_q16;
    u32 texture_x;
    u32 texture_y;
    u32 texture_width;
    u32 texture_height;
    u32 poly_id;
} NDSRendererIFCommonCloudDraw;
static NDSRendererIFCommonCloudDraw sNdsRendererIFCommonCloudQueue[
    NDS_RENDERER_IFCOMMON_CLOUD_QUEUE_COUNT];
static u32 sNdsRendererIFCommonCloudQueueCount;
static u32 sNdsRendererHardwareNoOracle;
static u32 sNdsRendererHardwareTriangleBatchOpen;
static u32 sNdsRendererHardwareTriangleBatchTextured;
static u32 sNdsRendererHardwareTriangleBatchTextureName;
static u32 sNdsRendererHardwareTriangleBatchPolyFmt;
static u32 sNdsRendererHardwareTriangleBatchAlphaKey;
static u32 sNdsRendererHardwareTriangleBatchFogKey;
static u32 sNdsRendererHardwareTriangleBatchMatrixMode;
static u32 sNdsRendererHardwareTriangleBatchMatrixGeneration;
static u32 sNdsRendererHardwareBoundTextureName;
static int sNdsRendererHardwareNoTextureName;
/* The rebirth-halo beam's dedicated A5I3 slot; the reasoning is at
 * ndsRendererHardwarePreparePrimRgbTexel0AlphaTexture. Identity rather than a
 * cache key: one source image, one upload extent, one primitive colour, all
 * three re-prepared when any of them moves. */
static u32 sNdsRendererHardwarePrimRgbTexel0AlphaName;
static u32 sNdsRendererHardwarePrimRgbTexel0AlphaImage;
static u32 sNdsRendererHardwarePrimRgbTexel0AlphaExtent;
static u32 sNdsRendererHardwarePrimRgbTexel0AlphaPrim;
volatile u32 gNdsRendererPrimRgbTexel0AlphaPrepareCount;
volatile u32 gNdsRendererPrimRgbTexel0AlphaBindCount;
#if NDS_R2_IMPACT_WAVE_NATIVE
#define NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT 5u
#define NDS_RENDERER_IMPACT_WAVE_TEX_WIDTH 16u
#define NDS_RENDERER_IMPACT_WAVE_TEX_HEIGHT 32u
#define NDS_RENDERER_IMPACT_WAVE_TEX_BYTES 256u
/* Source file 83, DL_0x7C28: CI4 texels @ 0x7980 and RGBA5551 TLUT @
 * 0x7958. The live sampler is 16x32: SETTILE uses CI4 line=1, S mask=4 and
 * T mask=5. That live state is authoritative over the reloc-data annotation
 * that describes the same 256 bytes as 32x16. Effect assets use the port's O2R
 * word-swapped layout, so logical CI4 bytes are physical byte[index ^ 3] and
 * logical TLUT halfwords are physical halfword[index ^ 1]. After undoing that
 * storage layout, N64's first/high nibble is packed into DS PAL16's first/low
 * nibble. Logical palette index 0 is already the sole transparent source entry,
 * so GL_TEXTURE_COLOR0_TRANSPARENT is lossless and no runtime N64 texture/TLUT
 * conversion remains. */
static const u8 sNdsRendererImpactWaveTexels[NDS_RENDERER_IMPACT_WAVE_TEX_BYTES] = {
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xfcu, 0xfdu, 0xffu, 0xffu, 0xffu, 0xffu, 0xcfu, 0x99u,
    0xebu, 0xfcu, 0xffu, 0xffu, 0xffu, 0xffu, 0xbfu, 0x98u,
    0xdau, 0xfbu, 0xffu, 0xffu, 0xffu, 0xffu, 0xafu, 0x97u,
    0xc9u, 0xeau, 0xffu, 0xffu, 0xffu, 0xffu, 0x9fu, 0x86u,
    0xb9u, 0xd9u, 0xffu, 0xffu, 0xfeu, 0xffu, 0x9eu, 0x75u,
    0xa9u, 0xc9u, 0xffu, 0xffu, 0xedu, 0xffu, 0x9du, 0x64u,
    0x98u, 0xb9u, 0xffu, 0xefu, 0xdcu, 0xffu, 0x8cu, 0x53u,
    0x97u, 0xa9u, 0xfeu, 0xdfu, 0xcbu, 0xeeu, 0x7bu, 0x42u,
    0x96u, 0x97u, 0xfdu, 0xcfu, 0xbau, 0xddu, 0x6au, 0x30u,
    0x85u, 0x97u, 0xfcu, 0xbeu, 0xa9u, 0xccu, 0x59u, 0x20u,
    0x74u, 0x96u, 0xebu, 0xadu, 0x99u, 0xbbu, 0x49u, 0x20u,
    0x63u, 0x05u, 0xdau, 0x9cu, 0x99u, 0xaau, 0x39u, 0x10u,
    0x52u, 0x04u, 0xc9u, 0x9bu, 0x98u, 0x99u, 0x08u, 0x10u,
    0x42u, 0x03u, 0xb9u, 0x9au, 0x87u, 0x99u, 0x07u, 0x10u,
    0x31u, 0x02u, 0xa9u, 0x89u, 0x76u, 0x99u, 0x06u, 0x10u,
    0x21u, 0x02u, 0x99u, 0x79u, 0x65u, 0x98u, 0x05u, 0x10u,
    0x21u, 0x01u, 0x98u, 0x69u, 0x04u, 0x87u, 0x04u, 0x10u,
    0x11u, 0x01u, 0x97u, 0x58u, 0x03u, 0x76u, 0x00u, 0x00u,
    0x11u, 0x01u, 0x80u, 0x47u, 0x02u, 0x65u, 0x00u, 0x00u,
    0x11u, 0x01u, 0x70u, 0x36u, 0x02u, 0x54u, 0x00u, 0x00u,
    0x11u, 0x00u, 0x60u, 0x25u, 0x00u, 0x43u, 0x00u, 0x00u,
    0x11u, 0x00u, 0x50u, 0x24u, 0x00u, 0x30u, 0x00u, 0x00u,
    0x11u, 0x00u, 0x40u, 0x13u, 0x00u, 0x20u, 0x00u, 0x00u,
    0x10u, 0x00u, 0x30u, 0x12u, 0x00u, 0x20u, 0x00u, 0x00u,
    0x10u, 0x00u, 0x20u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x10u, 0x00u, 0x20u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x10u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x10u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
};

/* DL_0x7C28 combines (PRIM-ENV)*TEXEL0+ENV; ImpactWave ENV is black.
 * These five RGB555 palettes are the exact 5-bit result of that combine for
 * the source variants (red, green, blue, yellow, white), generated from the
 * real 0x7958 TLUT after undoing its O2R halfword swap. Logical source index 0
 * is transparent, so GL_TEXTURE_COLOR0_TRANSPARENT supplies the source A1
 * coverage and polygon alpha supplies the live PRIMITIVE alpha. */
static const u16 sNdsRendererImpactWavePalettes[NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT][16] = {
    {
        0x0007u, 0x0008u, 0x000bu, 0x000eu, 0x0011u, 0x0013u, 0x0016u, 0x0019u,
        0x001bu, 0x001eu, 0x001eu, 0x001eu, 0x001eu, 0x001eu, 0x001eu, 0x001eu,
    },
    {
        0x02c0u, 0x0300u, 0x0320u, 0x0340u, 0x0340u, 0x0360u, 0x0380u, 0x03a0u,
        0x03a0u, 0x03c0u, 0x03c0u, 0x03c0u, 0x03c0u, 0x03c0u, 0x03c0u, 0x03c0u,
    },
    {
        0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u,
        0x0000u, 0x0000u, 0x1000u, 0x2800u, 0x4000u, 0x5400u, 0x6c00u, 0x7800u,
    },
    {
        0x02c7u, 0x0308u, 0x032bu, 0x034eu, 0x0351u, 0x0373u, 0x0396u, 0x03b9u,
        0x03bbu, 0x03deu, 0x03deu, 0x03deu, 0x03deu, 0x03deu, 0x03deu, 0x03deu,
    },
    {
        0x02c7u, 0x0308u, 0x032bu, 0x034eu, 0x0351u, 0x0373u, 0x0396u, 0x03b9u,
        0x03bbu, 0x03deu, 0x13deu, 0x2bdeu, 0x43deu, 0x57deu, 0x6fdeu, 0x7bdeu,
    },
};
static u32 sNdsRendererImpactWaveTextureName[NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT];
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
extern volatile u32 gNdsRebirthHaloNativeTexturePrepareCount;
extern volatile u32 gNdsRebirthHaloNativeTextureBindCount;

typedef struct NDSRebirthHaloGroup
{
    u16 first_vertex;
    u8 triangle_count;
    u8 reserved;
} NDSRebirthHaloGroup;

typedef struct NDSRebirthHaloBounds
{
    s16 min_x;
    s16 min_y;
    s16 min_z;
    s16 max_x;
    s16 max_y;
    s16 max_z;
} NDSRebirthHaloBounds;

#include "nds_rebirth_halo.generated.inc"

static u32 sNdsRendererRebirthHaloTextureName[NDS_REBIRTH_HALO_TEXTURE_COUNT];
#if NDS_R2_REBIRTH_HALO_FULL_OFFLOAD
volatile u32 gNdsRebirthHaloFullOffloadRootCount;
volatile u32 gNdsRebirthHaloFullOffloadBoundCornerCount;
volatile u32 gNdsRebirthHaloFullOffloadBoundRejectCount;
#endif
#endif
static s32 sNdsRendererHardwareProjectedDepth =
    NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START;
static u32 sNdsRendererHardwareProjectedBackground = TRUE;
static u32 sNdsRendererHardwareMatrixLoaded;
static u32 sNdsRendererHardwareMatrixMode;
static u32 sNdsRendererHardwareMatrixGeneration;
static u32 sNdsRendererMatrixGenerationSerial;

#define NDS_RENDERER_GX_STATE_TEXTURE_PARAMS (1u << 0)
#define NDS_RENDERER_GX_STATE_MATRIX_MODE    (1u << 1)
#define NDS_RENDERER_GX_STATE_POLY_FMT       (1u << 2)
#define NDS_RENDERER_GX_STATE_ALL            \
    (NDS_RENDERER_GX_STATE_TEXTURE_PARAMS |  \
     NDS_RENDERER_GX_STATE_MATRIX_MODE |     \
     NDS_RENDERER_GX_STATE_POLY_FMT)

typedef struct NDSRendererGXStateShadow
{
    u32 texture_params;
    u32 matrix_mode;
    u32 poly_fmt;
    u32 valid_mask;
} NDSRendererGXStateShadow;

static NDSRendererGXStateShadow sNdsRendererGXStateShadow;

static inline void ndsRendererHardwareInvalidateGXState(u32 mask)
{
    sNdsRendererGXStateShadow.valid_mask &= ~mask;
}

static inline void ndsRendererHardwareBindTextureState(int name)
{
    glBindTexture(GL_TEXTURE_2D, name);
    ndsRendererHardwareInvalidateGXState(
        NDS_RENDERER_GX_STATE_TEXTURE_PARAMS);
}

/* P2-2 fighter packet. The four-CPU stress arm measured the four fighter draws
 * at 607,040 ticks of a 1,600,832-tick median frame (NDS_R2_DRAW_SUPPRESS_MASK
 * =15 A/B, 2026-08-23), almost all of it CPU work re-deriving and re-pushing a
 * command stream that is identical from one frame to the next. This captures
 * the stream once per fighter as packed GXFIFO words -- the format the stage
 * replay already DMAs -- and replays it by DMA, patching only what moves: the
 * projection, each root's seed and joint-chain matrices, and the light vector.
 * Everything else the words depend on is in the key; a key miss re-records
 * through the ordinary path, with the hooks below teeing into the packet the
 * words the hardware receives. The words live in gSYFramebufferSets, which
 * nothing touches during a battle (include/sys/video.h), and the Results entry
 * restores that buffer's clear through ndsRendererFighterPacketRelease. */
#define NDS_FIGHTER_PACKET_LIVE \
    (NDS_R2_FIGHTER_PACKET && NDS_RENDERER_HW_TRIANGLES && \
     (NDS_RENDERER_PROFILE_LEVEL < 2) && \
     NDS_R2_FIGHTER_GX_COMPOSE && NDS_R2_FIGHTER_HW_MTX && \
     NDS_R2_FIGHTER_HW_LIGHT)
#if NDS_FIGHTER_PACKET_LIVE
#define NDS_FIGHTER_PACKET_SLOTS 4u
/* 141,440 B of the 147,840-byte framebuffer: stops short of the z-buffer start
 * pointer that sys/video.h documents as aliased into the buffer's tail. */
#define NDS_FIGHTER_PACKET_ARENA_WORDS 35360u
#define NDS_FIGHTER_PACKET_ROOT_MAX 32u
#define NDS_FIGHTER_PACKET_LOCAL_MAX 8u
#define NDS_FIGHTER_PACKET_INDEX_NONE 0xffffu
#define NDS_FIGHTER_PACKET_KEY_WORDS 6u

typedef struct NDSFighterPacketRoot
{
    u16 seed_index;
    /* One index per MULT4x3: a packed header word can fall between two of
     * them, so their parameter blocks are not contiguous. */
    u16 local_index[NDS_FIGHTER_PACKET_LOCAL_MAX];
    u8 local_count;
    u8 parent_slot;
    u8 store_slot;
    u8 seed_is_identity;
} NDSFighterPacketRoot;

/* One DIF_AMB word the shade wrote. The churn census (2026-08-23, 480
 * frames of the four-CPU arm) put 222 of 350 re-records on the hurt flash --
 * the colour modulate and the root prim move every flash frame -- so the
 * word is re-derived on replay from the recorded light colours and the live
 * prim/modulate instead of invalidating the packet: exactly the shade's
 * ndsRendererR2MaterialColor15 inputs, with `prim_from_root` saying whether
 * the material colour was the root preamble's prim (follows the flash) or a
 * material/state-delta override (stays as recorded, as the source does). */
typedef struct NDSFighterPacketShadeSite
{
    u16 index;
    u8 root;
    u8 use_material;
    u8 prim_from_root;
    u8 reserved[3];
    u32 light_color_1;
    u32 light_color_2;
    u32 material_color;
} NDSFighterPacketShadeSite;

/* A texture the packet binds, validated on replay against the hardware
 * cache entry it was recorded from, so an eviction or re-upload of THIS
 * fighter's textures re-records this packet alone. */
typedef struct NDSFighterPacketTexture
{
    u16 slot_plus1;
    u16 name;
    u32 key_generation;
} NDSFighterPacketTexture;

#define NDS_FIGHTER_PACKET_SITE_MAX 64u
/* Four-distinct-kind Low-detail stress reaches more than 16 unique resident
 * cache identities in one fighter packet. Falling off this array is correct
 * but coarse: it reverts that packet to the global texture-generation fence,
 * so unrelated cache churn forces an otherwise-identical GX stream to be
 * re-recorded. 24 costs 64 B/packet (256 B total) and keeps residency proof
 * per texture; the packet words and BattleShip draw semantics are unchanged. */
#define NDS_FIGHTER_PACKET_TEXTURE_MAX 24u

typedef struct NDSFighterPacket
{
    u32 valid;
    u32 key[NDS_FIGHTER_PACKET_KEY_WORDS];
    u32 *words;
    u32 word_count;
    u32 word_capacity;
    u32 root_count;
    u16 projection_index;
    u16 light_index;
    u8 light_root;
    u8 light_valid;
    /* Set when a textured bind left no cache entry to validate; the packet
     * then keeps the global texture generation fence in its key. */
    u8 needs_fence;
    u8 texture_count;
    u32 triangle_count;
    u32 run_count;
    u32 raw_triangles;
    u32 raw_reuse;
    u32 cross_triangles;
    u32 cross_reuse;
    /* The frame-summary batch and texture-prepare counts the record frame
     * accrued between the adapter's pre-check and the finish, net of what
     * the raw/cross credits above already add back. A replay presents the
     * same batches to the hardware, so a hit credits these too and the
     * per-frame accounting every verifier reads stays exact. */
    u32 batch_begin;
    u32 batch_reuse;
    u32 batch_end;
    u32 prepare_begin;
    u32 prepare_reuse;
    u32 matrix_loads;
    u32 texture_binds;
    u32 vertex_loads;
    u32 site_count;
    /* The prim/modulate the shade sites currently encode. */
    u32 tint_modulate;
    u32 tint_prim_hash;
    NDSFighterPacketRoot roots[NDS_FIGHTER_PACKET_ROOT_MAX];
    NDSFighterPacketShadeSite sites[NDS_FIGHTER_PACKET_SITE_MAX];
    NDSFighterPacketTexture textures[NDS_FIGHTER_PACKET_TEXTURE_MAX];
} NDSFighterPacket;

typedef struct NDSFighterPacketRecorder
{
    NDSFighterPacket *packet;
    const NDSRendererNativeFighterRoot *inputs;
    u32 *words;
    u32 count;
    u32 capacity;
    u32 cmd_slot;
    u32 cmd_word;
    u32 header_params;
    u32 header_valid;
    u32 fault;
    u32 current_root;
    /* A material or state delta rewrote prim_color under the current root. */
    u32 prim_overridden;
} NDSFighterPacketRecorder;

static NDSFighterPacket sNdsFighterPackets[NDS_FIGHTER_PACKET_SLOTS];
static NDSFighterPacketRecorder sNdsFighterPacketRecorder;
static u32 sNdsFighterPacketRecording;
/* Frame-summary batch/prepare counters at the adapter's pre-check, so a
 * record frame can store what the slot's draw accrued (see NDSFighterPacket). */
static u32 sNdsFighterPacketRecordBase[8];
volatile u32 gNdsFighterPacketHits;
volatile u32 gNdsFighterPacketRecords;
volatile u32 gNdsFighterPacketFaults;
volatile u32 gNdsFighterPacketDeclines;
volatile u32 gNdsFighterPacketWordsMax;
/* Per key word (then root count, then texture residency): how often a valid
 * packet was invalidated by that cause, alone or with others. */
volatile u32 gNdsFighterPacketMissWord[NDS_FIGHTER_PACKET_KEY_WORDS + 2u];

/* One predictable test at each GX state write the production path makes;
 * zero cost when no packet is being recorded. The ITCM region is full (the
 * first link of this feature overflowed it by 2,112 bytes), so every record
 * helper a hook reaches is noinline, cold and size-optimised: the hook leaves
 * a test and a call in ITCM, nothing else. */
#define NDS_FIGHTER_PACKET_HOOK(stmt) \
    do { if (sNdsFighterPacketRecording != 0u) { stmt; } } while (0)
#define NDS_FIGHTER_PACKET_COLD_CODE \
    __attribute__((noinline, cold, optimize("Os")))

/* P2-2p3. The replay no longer waits for its own DMA: the geometry engine
 * drains the packet while the CPU prepares the next fighter, and whoever
 * writes the FIFO next waits first. Every GX writer outside the packet path
 * enters through one of the seams that call this (the production execute,
 * the stage owner prepare, the particle/effect/gun/halo/entry-effect submits,
 * the generic display-list executor and the end-of-frame flush); a writer
 * that bypasses them would interleave its words with the DMA's. */
static u32 sNdsFighterPacketDmaPending;
static inline void ndsFighterPacketDmaWait(void)
{
    if (sNdsFighterPacketDmaPending != 0u)
    {
        while ((DMA_CR(0) & DMA_BUSY) != 0u) { }
        sNdsFighterPacketDmaPending = 0u;
    }
}
#define NDS_FIGHTER_PACKET_DMA_WAIT() ndsFighterPacketDmaWait()
/* The hook sites still cost ITCM (496 bytes at the second link), and the
 * region was full. Four residents the 2026-08-22 four-CPU census ranks at the
 * bottom of its rent table (ndsRendererSetParticleCamera 320 B at 1,314
 * cycles/byte, ndsRendererMtxLoadN64ToDS20p12 256 B, ndsRendererRecordSetTileSize
 * 180 B, ndsRendererRecordLoadBlock 100 B -- under 3,000 ticks/frame between
 * them) go to main RAM in a packet build only, so the control arm keeps its
 * exact placement. */
#define NDS_FIGHTER_PACKET_EVICT(attr)

/* Packed geometry command: a header word carries up to four opcodes and each
 * opcode's parameters follow in order. A header whose commands take no
 * parameters at all gets one dummy word, the display-list convention. Returns
 * the index of the command's first parameter word. */
static u32 NDS_FIGHTER_PACKET_COLD_CODE
ndsFighterPacketCmd(u32 opcode, u32 param_count)
{
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;
    u32 first;

    if (rec->fault != 0u)
    {
        return 0u;
    }
    if (rec->cmd_slot >= 4u)
    {
        if ((rec->header_valid != 0u) && (rec->header_params == 0u))
        {
            if (rec->count >= rec->capacity)
            {
                rec->fault = 1u;
                return 0u;
            }
            rec->words[rec->count++] = 0u;
        }
        if (rec->count >= rec->capacity)
        {
            rec->fault = 1u;
            return 0u;
        }
        rec->cmd_word = rec->count++;
        rec->words[rec->cmd_word] = 0u;
        rec->cmd_slot = 0u;
        rec->header_params = 0u;
        rec->header_valid = 1u;
    }
    if (rec->count + param_count > rec->capacity)
    {
        rec->fault = 1u;
        return 0u;
    }
    rec->words[rec->cmd_word] |= opcode << (rec->cmd_slot * 8u);
    rec->cmd_slot++;
    rec->header_params += param_count;
    first = rec->count;
    rec->count += param_count;
    return first;
}

static void NDS_FIGHTER_PACKET_COLD_CODE ndsFighterPacketCmd0(u32 opcode)
{
    (void)ndsFighterPacketCmd(opcode, 0u);
}

static void NDS_FIGHTER_PACKET_COLD_CODE
ndsFighterPacketCmd1(u32 opcode, u32 word)
{
    u32 i = ndsFighterPacketCmd(opcode, 1u);

    if (sNdsFighterPacketRecorder.fault == 0u)
    {
        sNdsFighterPacketRecorder.words[i] = word;
    }
}

static void NDS_FIGHTER_PACKET_COLD_CODE
ndsFighterPacketCmd2(u32 opcode, u32 a, u32 b)
{
    u32 i = ndsFighterPacketCmd(opcode, 2u);

    if (sNdsFighterPacketRecorder.fault == 0u)
    {
        sNdsFighterPacketRecorder.words[i] = a;
        sNdsFighterPacketRecorder.words[i + 1u] = b;
    }
}

static void ndsFighterPacketStoreMatrix4x4(
    u32 *dst, const NDSRendererMatrix20p12 *m)
{
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        *dst++ = (u32)m->m[row][0];
        *dst++ = (u32)m->m[row][1];
        *dst++ = (u32)m->m[row][2];
        *dst++ = (u32)m->m[row][3];
    }
}

static void ndsFighterPacketStoreMatrix4x3(
    u32 *dst, const NDSRendererMatrix20p12 *m)
{
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        *dst++ = (u32)m->m[row][0];
        *dst++ = (u32)m->m[row][1];
        *dst++ = (u32)m->m[row][2];
    }
}

/* The texture the hardware holds right now: libnds keeps the full
 * TEXIMAGE_PARAM word and the palette base it wrote, so the packet records
 * exactly the bind the production path just performed. */
static void NDS_FIGHTER_PACKET_COLD_CODE ndsFighterPacketRecordBoundTexture(void)
{
    int palette_format = -1;

    ndsFighterPacketCmd1(REG2ID(GFX_TEX_FORMAT), glGetTexParameter());
    glGetColorTableParameterEXT(
        GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT, &palette_format);
    if (palette_format >= 0)
    {
        ndsFighterPacketCmd1(REG2ID(GFX_PAL_FORMAT), (u32)palette_format);
    }
}

/* Defined with the replay: it reads the hardware texture cache, which is
 * declared after this block. */
static void ndsFighterPacketNoteTextureEntry(void);

/* One hook at the end of a run's texture prepare stands in for the three
 * batch writes the production path makes through shared, ITCM-resident
 * helpers (texture bind, POLYGON_ATTR, BEGIN). Those helpers write only when
 * their shadow state changes; the packet writes all three at every prepare,
 * which is the same hardware state because a prepare sits at a primitive
 * boundary and the first prepare of a record follows a full tracker reset. An
 * untextured run binds libnds's no-texture object, whose TEXIMAGE_PARAM is 0. */
static void NDS_FIGHTER_PACKET_COLD_CODE
ndsFighterPacketRecordPrepare(u32 use_texture, u32 poly_fmt)
{
    if (use_texture != 0u)
    {
        ndsFighterPacketRecordBoundTexture();
        ndsFighterPacketNoteTextureEntry();
    }
    else
    {
        ndsFighterPacketCmd1(REG2ID(GFX_TEX_FORMAT), 0u);
    }
    ndsFighterPacketCmd1(REG2ID(GFX_POLY_FORMAT), poly_fmt);
    ndsFighterPacketCmd1(FIFO_BEGIN, (u32)GL_TRIANGLE);
}

/* The shade's DIF_AMB write and the inputs that re-derive it on replay. */
static void NDS_FIGHTER_PACKET_COLD_CODE
ndsFighterPacketRecordDiffuseAmbient(
    u32 word, u32 light_color_1, u32 light_color_2,
    u32 material_color, u32 use_material)
{
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;
    u32 i = ndsFighterPacketCmd(REG2ID(GFX_DIFFUSE_AMBIENT), 1u);
    NDSFighterPacketShadeSite *site;

    if ((rec->fault != 0u) || (rec->packet == NULL))
    {
        return;
    }
    rec->words[i] = word;
    if (rec->packet->site_count >= NDS_FIGHTER_PACKET_SITE_MAX)
    {
        rec->fault = 1u;
        return;
    }
    site = &rec->packet->sites[rec->packet->site_count++];
    site->index = (u16)i;
    site->root = (u8)rec->current_root;
    site->use_material = (use_material != 0u) ? 1u : 0u;
    site->prim_from_root =
        ((use_material != 0u) && (rec->prim_overridden == 0u)) ? 1u : 0u;
    site->light_color_1 = light_color_1;
    site->light_color_2 = light_color_2;
    site->material_color = material_color;
}

static void NDS_FIGHTER_PACKET_COLD_CODE ndsFighterPacketBeginRoot(
    u32 root_index, const NDSRendererNativeFighterRoot *input)
{
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;
    NDSFighterPacketRoot *root;
    u32 j;

    rec->current_root = root_index;
    rec->prim_overridden = 0u;
    if ((rec->packet == NULL) ||
        (root_index >= NDS_FIGHTER_PACKET_ROOT_MAX) ||
        ((u32)input->gx_local_count > NDS_FIGHTER_PACKET_LOCAL_MAX))
    {
        rec->fault = 1u;
        return;
    }
    root = &rec->packet->roots[root_index];
    root->seed_index = NDS_FIGHTER_PACKET_INDEX_NONE;
    for (j = 0u; j < NDS_FIGHTER_PACKET_LOCAL_MAX; j++)
    {
        root->local_index[j] = NDS_FIGHTER_PACKET_INDEX_NONE;
    }
    root->local_count = input->gx_local_count;
    root->parent_slot = input->gx_parent_slot;
    root->store_slot = input->gx_store_slot;
    root->seed_is_identity = input->gx_seed_is_identity;
}

/* ndsRendererR2WriteLightVector's five commands; the vector word is the
 * per-frame patch. */
static void NDS_FIGHTER_PACKET_COLD_CODE
ndsFighterPacketRecordLightVector(u32 word)
{
    NDSFighterPacketRecorder *rec = &sNdsFighterPacketRecorder;
    u32 i;

    ndsFighterPacketCmd1(REG2ID(MATRIX_CONTROL), (u32)GL_MODELVIEW);
    ndsFighterPacketCmd0(REG2ID(MATRIX_PUSH));
    ndsFighterPacketCmd0(REG2ID(MATRIX_IDENTITY));
    i = ndsFighterPacketCmd(REG2ID(GFX_LIGHT_VECTOR), 1u);
    if ((rec->fault == 0u) && (rec->packet != NULL))
    {
        rec->words[i] = word;
        if (rec->packet->light_index == NDS_FIGHTER_PACKET_INDEX_NONE)
        {
            rec->packet->light_index = (u16)i;
            rec->packet->light_root = (u8)rec->current_root;
            rec->packet->light_valid = 1u;
        }
    }
    ndsFighterPacketCmd1(REG2ID(MATRIX_POP), 1u);
}
#else
#define NDS_FIGHTER_PACKET_HOOK(stmt) ((void)0)
#define NDS_FIGHTER_PACKET_EVICT(attr) attr
#define NDS_FIGHTER_PACKET_DMA_WAIT() ((void)0)
#endif

static inline void ndsRendererHardwareSetMatrixMode(int mode)
{
#if NDS_RENDERER_M3_PHASE0_PROFILE
    if (((sNdsRendererGXStateShadow.valid_mask &
          NDS_RENDERER_GX_STATE_MATRIX_MODE) != 0u) &&
        (sNdsRendererGXStateShadow.matrix_mode == (u32)mode))
    {
        gNdsRendererM3G2MatrixModeSkipCount++;
        return;
    }
#endif
    glMatrixMode(mode);
#if NDS_RENDERER_M3_PHASE0_PROFILE
    sNdsRendererGXStateShadow.matrix_mode = (u32)mode;
    sNdsRendererGXStateShadow.valid_mask |=
        NDS_RENDERER_GX_STATE_MATRIX_MODE;
    gNdsRendererM3G2MatrixModeWriteCount++;
#endif
}

static inline void ndsRendererHardwareSetPolyFmt(u32 poly_fmt)
{
    if (((sNdsRendererGXStateShadow.valid_mask &
          NDS_RENDERER_GX_STATE_POLY_FMT) != 0u) &&
        (sNdsRendererGXStateShadow.poly_fmt == poly_fmt))
    {
#if NDS_RENDERER_M3_PHASE0_PROFILE
        gNdsRendererM3G2PolyFmtSkipCount++;
#endif
        return;
    }
    glPolyFmt(poly_fmt);
    sNdsRendererGXStateShadow.poly_fmt = poly_fmt;
    sNdsRendererGXStateShadow.valid_mask |= NDS_RENDERER_GX_STATE_POLY_FMT;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererM3G2PolyFmtWriteCount++;
#endif
}
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 sNdsRendererHardwareMatrixSignature;

static inline u32 ndsRendererProfileHashU32(u32 hash, u32 value)
{
    u32 i;

    if (hash == 0u)
    {
        hash = 2166136261u;
    }
    for (i = 0u; i < sizeof(value); i++)
    {
        hash ^= (value >> (i * 8u)) & 0xffu;
        hash *= 16777619u;
    }
    if (hash == 0u)
    {
        hash = 1u;
    }
    return hash;
}

static u32 ndsRendererProfileHashMatrixPair(
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    u32 mode, u32 generation)
{
    u32 hash = 0u;
    u32 row;
    u32 col;

    hash = ndsRendererProfileHashU32(hash, mode);
    hash = ndsRendererProfileHashU32(hash, generation);
    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            hash = ndsRendererProfileHashU32(
                hash, (u32)projection->m[row][col]);
            hash = ndsRendererProfileHashU32(
                hash, (u32)modelview->m[row][col]);
        }
    }
    return hash;
}
#endif
static u32 sNdsRendererHardwareSubmitClassCounts[
    NDS_RENDERER_HW_SUBMIT_CLASS_COUNT];
/* The logical demand is derived exactly from the submitted class totals.
 * Profiles 1/2 pack actual call/clamp counts plus error flags into this one
 * existing summary word, avoiding forensic BSS growth in the tight P1 build. */
#define NDS_RENDERER_HW_DIVISION_CALL_MASK          0x00000fffu
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_LOW_SHIFT 12u
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_LOW_ONE   (1u << 12)
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_LOW_MASK  0x000ff000u
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_HIGH_SHIFT 20u
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_HIGH_ONE  (1u << 20)
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_HIGH_MASK 0x0ff00000u
#define NDS_RENDERER_HW_DIVISION_ZERO_DENOMINATOR   (1u << 28)
#define NDS_RENDERER_HW_DIVISION_MISMATCH           (1u << 29)
static u32 sNdsRendererHardwareDivideSummary;
static u32 sNdsRendererHardwareSourceVertexLoadCount;
static u32 sNdsRendererHardwareCPUTransformCount;
static u32 sNdsRendererHardwareTransformCacheHitCount;
static u32 sNdsRendererHardwareMatrixSnapshotCreateCount;
static u32 sNdsRendererHardwareMatrixSnapshotReuseCount;
static u32 sNdsRendererHardwareMatrixSnapshotOverflowCount;

/* Profile levels 0/1 accumulate the small runtime health summary in ordinary
 * memory and publish it once at frame completion. The performance build never
 * writes the exported volatile profile globals from command, triangle, or
 * vertex loops. Level 2 retains the exact forensic counters below. */
#if NDS_RENDERER_PROFILE_LEVEL < 2
typedef struct NDSRendererRuntimeFrameSummary
{
    u32 texture_binds;
    u32 texture_uploads;
    u32 texture_upload_bytes;
    u32 texture_cache_alias_avoid_count;
    u32 texture_lookup_call_count;
    u32 texture_lookup_probe_count;
    u32 texture_lookup_active_hit_count;
    u32 texture_lookup_table_hit_count;
    u32 texture_lookup_miss_count;
    u32 texel1_composite_count;
    u32 texel1_load_match_count;
    u32 texel1_reject_count;
    u32 projected_submit_fallback_count;
    u32 matrix_load_count;
    u32 hardware_vertices;
    u32 hardware_triangles;
    u32 hardware_batch_begin_count;
    u32 hardware_batch_reuse_count;
    u32 hardware_batch_end_count;
    u32 texture_prepare_count;
    u32 texture_prepare_reuse_count;
    u32 hardware_over_limit;
    u32 hardware_vertex_saturate_count;
    u32 near_plane_triangle_reject_count;
    u32 raw_current_candidate_count;
    u32 raw_current_range_reject_count;
    u32 raw_cross_matrix_count;
} NDSRendererRuntimeFrameSummary;

/* 108 bytes in DTCM, which is the cheapest place on this machine to do a
 * read-modify-write. Cycle 110 priced these counters by compiling them out:
 * FTR fell 7,378 and STG 2,776, an order of magnitude past the ~1,300 the
 * per-line profile showed, because the profile only sees the two symbols the
 * lines live in and every hardware batch on every path pays them. They are not
 * removable -- verify-battle-mariofox-gcrunall-loop-harness.ps1 asserts exact
 * batch and texture-prepare accounting off them, and the Boundary profile runs
 * it -- so the fix is to stop paying main-memory latency and a cache line for
 * evidence.
 *
 * `.dtcm.fighter`, not `.dtcm.bss`: check-task20-dtcm-layout.ps1 pins
 * `.dtcm.bss` at Calico's own 152 bytes (__irq_table + __sched_state) and
 * throws on anything else landing there. The fighter group is the modelled
 * home for audited renderer DTCM data and the checker rounds it to 32, which
 * keeps __irq_table's alignment. Adding an owner here means adding its size to
 * that script's $fighterOwnerSizes -- the layout is pinned on purpose.
 *
 * Audited against that gate's DMA/IPC/ARM7 requirement, same as the two vertex
 * tables above it: written and read only by ARM9 renderer code, never a DMA
 * source or destination, never visible to the ARM7 or IPC. DMA cannot read
 * DTCM at all. Zero-initialised at every frame begin by memset, so nothing
 * depends on the crt's copy of its (zero) image. */
static NDSRendererRuntimeFrameSummary sNdsRendererRuntimeFrameSummary
    __attribute__((section(".dtcm.fighter")));
static u32 sNdsRendererRuntimeTexel1FractionRefreshCount;
static u32 sNdsRendererRuntimeTextureCacheEvictCount;
static u32 sNdsRendererRuntimeTextureCi4DirectPixels;
static u32 sNdsRendererRuntimeCi4IndexCacheBuildCount;
static u32 sNdsRendererRuntimeCi4IndexCacheReuseCount;
static u32 sNdsRendererRuntimeCi4RepresentativePixelCount;
static u32 sNdsRendererRuntimeCi4ReusePixelCount;
#endif

volatile u32 gNdsRendererBattleStaticTextureEnabled =
    NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT;
volatile u32 gNdsRendererBattleStaticTexturePrepareCount;
volatile u32 gNdsRendererBattleStaticTexturePrepareFailCount;
volatile u32 gNdsRendererBattleStaticTexturePreparedCount;
volatile u32 gNdsRendererBattleStaticTexturePreparedBytes;
volatile u32 gNdsRendererBattleStaticTextureArmCount;
volatile u32 gNdsRendererBattleStaticTexturePinnedHitCount;
volatile u32 gNdsRendererBattleStaticTextureSeenMask;
volatile u32 gNdsRendererBattleStaticTextureOwnerMask;
volatile u32 gNdsRendererBattleStaticTextureViolationCount;
volatile u32 gNdsRendererBattleStaticTextureTeardownCount;
volatile u32 gNdsRendererBattleStaticTextureFirstAddress;
volatile u32 gNdsRendererBattleStaticTextureEndAddress;
volatile u32 gNdsRendererBattleStaticTextureAllocationSpanBytes;
volatile u32 gNdsRendererBattleStaticTextureBankMask;

/* BUGS ROW 6. THE MOST DESTRUCTIVE EVENT IN THIS RENDERER WAS INVISIBLE IN THE
 * CONFIGURATION THAT SHIPS.
 *
 * ndsRendererAdapterPrepareNativeStageOwner rejects through a label that calls
 * ndsRendererHardwareAbortBattleStaticTextures, which discards the entire
 * hardware texture cache and clears sNdsRendererBattleStaticTexturePrepared --
 * and ndsRendererHardwareArmBattleStaticTextures refuses to re-arm without
 * that flag. So one transient "this frame cannot use the native fast path"
 * removes every texture in the scene for the remainder of the match, with no
 * path back. That is scene-wide and permanent, which is verbatim what the
 * owner reported.
 *
 * The only existing records of it were gNdsRendererM3PostArmFailureCount and
 * gNdsRendererTask36AdapterRejectReason, both inside
 * `#if NDS_RENDERER_PROFILE_LEVEL == 1` and both nm-confirmed absent from the
 * shipping ELF -- so a shipping build could destroy its own texture cache and
 * leave no counter behind. These are unconditional.
 *
 * The FIRST-reject latch is the load-bearing field. A count only says it
 * happened; the reason names which of the six branches in that function to go
 * and read, which is the difference between a next cycle that measures and a
 * next cycle that hunts. Reason codes are that function's own: 2 null CObj,
 * 3 asset lookup/size/generation, 4 topology collect or stamp, 5 matrices,
 * 6 ndsRendererPrepareNativeStageOwner, 7 materials, 1 unreached default. */
volatile u32 gNdsRendererStageOwnerRejectCount;
volatile u32 gNdsRendererStageOwnerLastRejectReason;
volatile u32 gNdsRendererStageOwnerFirstRejectReason;
volatile u32 gNdsRendererStageOwnerAbortCount;
/* Post-arm rejects, which used to be the destructive case. This is the counter
 * that keeps the 2026-08-04 fix honest: it must climb exactly where the abort
 * used to, while gNdsRendererStageOwnerAbortCount stays 0 and
 * gNdsRendererStaticTexturePreparedNow stays 1. If this reads 0 across a match
 * with a death, the lever is not being exercised and the proof is vacuous. */
volatile u32 gNdsRendererStageOwnerPostArmRejectCount;
/* Mirrors sNdsRendererBattleStaticTexturePrepared, which is static and so
 * cannot be read from a probe. Without it the permanent-latch claim is an
 * inference from reading the source; with it the latch is observable. */
volatile u32 gNdsRendererStaticTexturePreparedNow;

static u32 sNdsRendererBattleStaticTexturePrepared;
static u32 sNdsRendererBattleStaticTextureArmed;
volatile u32 gNdsRendererBattleTextureFenceCounts[
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_COUNT];
volatile u32 gNdsRendererBattleTextureFenceFirstClassPlus1;
volatile u32 gNdsRendererBattleTextureFenceFirstFrame;

_Static_assert(NDS_RENDERER_BATTLE_TEXTURE_FENCE_COUNT == 10,
               "post-GO texture fence schema changed");
_Static_assert(sizeof(gNdsRendererBattleTextureFenceCounts) ==
                   (10u * sizeof(u32)),
               "post-GO texture fence must remain 40 bytes");
_Static_assert(sizeof(gNdsRendererBattleTextureFenceCounts) +
                   sizeof(gNdsRendererBattleTextureFenceFirstClassPlus1) +
                   sizeof(gNdsRendererBattleTextureFenceFirstFrame) == 48u,
               "post-GO texture fence diagnostics must remain 48 bytes");

static inline void ndsRendererHardwareRecordBattleTextureFence(
    NDSRendererBattleTextureFenceClass fence_class)
{
    if ((sNdsRendererBattleStaticTextureArmed == 0u) ||
        ((u32)fence_class >= NDS_RENDERER_BATTLE_TEXTURE_FENCE_COUNT))
    {
        return;
    }
    if (gNdsRendererBattleTextureFenceFirstClassPlus1 == 0u)
    {
        gNdsRendererBattleTextureFenceFirstClassPlus1 =
            (u32)fence_class + 1u;
        gNdsRendererBattleTextureFenceFirstFrame =
            gNdsRendererProfileFrameCount;
    }
    gNdsRendererBattleTextureFenceCounts[(u32)fence_class]++;
}

static inline int ndsRendererHardwareFencedGlGenTextures(int count,
                                                         int *names)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_CREATE);
    return glGenTextures(count, names);
}

static inline int ndsRendererHardwareFencedGlTexImage2D(
    int target, int empty1, GL_TEXTURE_TYPE_ENUM type,
    int size_x, int size_y, int empty2, int params, const void *texture)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_UPLOAD);
    ndsRendererHardwareInvalidateGXState(
        NDS_RENDERER_GX_STATE_TEXTURE_PARAMS);
    return glTexImage2D(target, empty1, type, size_x, size_y, empty2,
                        params, texture);
}

static inline int ndsRendererHardwareFencedGlDeleteTextures(int count,
                                                            int *names)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_DELETE);
    ndsRendererHardwareInvalidateGXState(
        NDS_RENDERER_GX_STATE_TEXTURE_PARAMS);
    return glDeleteTextures(count, names);
}

static FILE *ndsRendererHardwareFencedTextureFopen(const char *path,
                                                   const char *mode)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO);
    return fopen(path, mode);
}

static int ndsRendererHardwareFencedTextureFseek(FILE *file, long offset,
                                                 int origin)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO);
    return fseek(file, offset, origin);
}

static long ndsRendererHardwareFencedTextureFtell(FILE *file)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO);
    return ftell(file);
}

static size_t ndsRendererHardwareFencedTextureFread(
    void *destination, size_t size, size_t count, FILE *file)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO);
    return fread(destination, size, count, file);
}

static int ndsRendererHardwareFencedTextureFclose(FILE *file)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO);
    return fclose(file);
}

#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
volatile u32 gNdsRendererBenchmarkTriangleCount;
static u32 sNdsRendererBenchmarkTriangleCount;
#endif

/* Profile 2 compares every phase-masked palette-pair result against the
 * existing exact blend formula before the pixels are submitted to GX. Keep
 * symbols available in profile 1 so one synchronized benchmark marker can be
 * used for both coarse timing and forensic runs. */
volatile u32 gNdsRendererProfileTexturePairOracleChecks;
volatile u32 gNdsRendererProfileTexturePairOracleMismatches;
volatile u32 gNdsRendererProfileTextureVBlankQueuedUploads;
volatile u32 gNdsRendererProfileTextureVBlankQueuedBytes;
volatile u32 gNdsRendererProfileTextureVBlankCommittedUploads;
volatile u32 gNdsRendererProfileTextureVBlankCommitTicks;
volatile u32 gNdsRendererProfileTextureVBlankOutsideCount;
volatile u32 gNdsRendererProfileTextureVBlankFallbackCount;
volatile u32 gNdsRendererProfileTextureVBlankStartLine;
volatile u32 gNdsRendererProfileTextureVBlankEndLine;
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD
volatile u32 gNdsRendererBenchmarkSuppressedTextureUploads;
volatile u32 gNdsRendererBenchmarkSuppressedTextureUploadBytes;
static u32 sNdsRendererBenchmarkSuppressedTextureUploads;
static u32 sNdsRendererBenchmarkSuppressedTextureUploadBytes;
#endif

static inline void ndsRendererProfileRecordTextureBind(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureBinds++;
#else
    sNdsRendererRuntimeFrameSummary.texture_binds++;
#endif
}

static inline void ndsRendererProfileRecordTextureUpload(u32 bytes)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureUploads++;
    gNdsRendererProfileTextureUploadBytes += bytes;
#else
    sNdsRendererRuntimeFrameSummary.texture_uploads++;
    sNdsRendererRuntimeFrameSummary.texture_upload_bytes += bytes;
#if NDS_TICK_HUD
    /* R2-07: cumulative mirrors. The struct above is memset every frame, so a
     * ring stop can only ever read one frame of it. */
    gNdsMiscTexUploadCount++;
    gNdsMiscTexUploadBytes += bytes;
#endif
#endif
}

static inline void ndsRendererProfileRecordTextureCi4Direct(u32 pixels)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureCi4DirectPixels += pixels;
#else
    sNdsRendererRuntimeTextureCi4DirectPixels += pixels;
#endif
}

static inline void ndsRendererProfileRecordCi4IndexCacheBuild(void)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeCi4IndexCacheBuildCount++;
#endif
}

static inline void ndsRendererProfileRecordCi4IndexCacheReuse(void)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeCi4IndexCacheReuseCount++;
#endif
}

static inline void ndsRendererProfileRecordCi4RepresentativeReuse(
    u32 representative_pixels, u32 reused_pixels)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeCi4RepresentativePixelCount += representative_pixels;
    sNdsRendererRuntimeCi4ReusePixelCount += reused_pixels;
#else
    (void)representative_pixels;
    (void)reused_pixels;
#endif
}

static inline void ndsRendererProfileRecordTextureAliasAvoid(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureCacheAliasAvoidCount++;
#else
    sNdsRendererRuntimeFrameSummary.texture_cache_alias_avoid_count++;
#endif
}

static inline void ndsRendererProfileRecordTextureEvict(void)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_EVICT);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureCacheEvictCount++;
#else
    sNdsRendererRuntimeTextureCacheEvictCount++;
#endif
}

static inline void ndsRendererProfileRecordTexel1Composite(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexel1CompositeCount++;
    gNdsRendererProfileTexel1LoadMatchCount++;
#else
    sNdsRendererRuntimeFrameSummary.texel1_composite_count++;
    sNdsRendererRuntimeFrameSummary.texel1_load_match_count++;
#endif
}

static inline void ndsRendererProfileRecordTexel1Reject(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexel1RejectCount++;
#else
    sNdsRendererRuntimeFrameSummary.texel1_reject_count++;
#endif
}

static inline void ndsRendererProfileRecordTexel1RejectReason(u32 reason)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexel1RejectReasonMask |= reason;
#else
    (void)reason;
#endif
}

static inline void ndsRendererProfileRecordTexel1Refresh(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexel1FractionRefreshCount++;
#else
    sNdsRendererRuntimeTexel1FractionRefreshCount++;
#endif
}

/* NDS_RENDERER_FRAME_SUMMARY_COUNTERS gates the PROFILE_LEVEL 0 arm of the
 * six per-call recorders below. They are pure diagnostics -- nothing in the
 * Latest or Boundary registry reads them -- and at level 0 each was still a
 * read-modify-write on a global struct on every matrix load, every hardware
 * batch, and every texture prepare. See the flag's comment in
 * include/nds/nds_renderer.h for who has to build with it at 1. */
static inline void ndsRendererProfileRecordMatrixLoad(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileMatrixLoadCount++;
#elif NDS_RENDERER_FRAME_SUMMARY_COUNTERS
    sNdsRendererRuntimeFrameSummary.matrix_load_count++;
#endif
}

static inline void ndsRendererProfileRecordBatchBegin(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHardwareBatchBeginCount++;
#elif NDS_RENDERER_FRAME_SUMMARY_COUNTERS
    sNdsRendererRuntimeFrameSummary.hardware_batch_begin_count++;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    {
        NDSRendererProfileOwnerHotLedger *owner =
            ndsRendererProfileCurrentOwner();

        if (owner != NULL)
        {
            owner->run_count++;
        }
    }
#endif
}

static inline void ndsRendererProfileRecordBatchReuse(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHardwareBatchReuseCount++;
#elif NDS_RENDERER_FRAME_SUMMARY_COUNTERS
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count++;
#endif
}

static inline void ndsRendererProfileRecordBatchEnd(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHardwareBatchEndCount++;
#elif NDS_RENDERER_FRAME_SUMMARY_COUNTERS
    sNdsRendererRuntimeFrameSummary.hardware_batch_end_count++;
#endif
}

static inline void ndsRendererProfileRecordTexturePrepare(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexturePrepareCount++;
#elif NDS_RENDERER_FRAME_SUMMARY_COUNTERS
    sNdsRendererRuntimeFrameSummary.texture_prepare_count++;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    {
        NDSRendererProfileOwnerHotLedger *owner =
            ndsRendererProfileCurrentOwner();

        if (owner != NULL)
        {
            owner->texture_change_count++;
        }
    }
#endif
}

static inline void ndsRendererProfileRecordTexturePrepareReuse(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexturePrepareReuseCount++;
#elif NDS_RENDERER_FRAME_SUMMARY_COUNTERS
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count++;
#endif
}

static inline void ndsRendererProfileRecordVertexSaturate(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHWVertexSaturateCount++;
#else
    sNdsRendererRuntimeFrameSummary.hardware_vertex_saturate_count++;
#endif
}

static inline void ndsRendererProfileRecordNearPlaneTriangleReject(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileNearPlaneTriangleRejectCount++;
#else
    sNdsRendererRuntimeFrameSummary.near_plane_triangle_reject_count++;
#endif
}

static inline void ndsRendererProfileRecordProjectedSubmit(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileProjectedSubmitFallbackCount++;
#else
    sNdsRendererRuntimeFrameSummary.projected_submit_fallback_count++;
#endif
}

static inline void ndsRendererProfileRecordRawCurrentCandidate(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileRawCurrentCandidateCount++;
#else
    sNdsRendererRuntimeFrameSummary.raw_current_candidate_count++;
#endif
}

static inline void ndsRendererProfileRecordRawCurrentRangeReject(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileRawCurrentRangeRejectCount++;
#else
    sNdsRendererRuntimeFrameSummary.raw_current_range_reject_count++;
#endif
}

static inline void ndsRendererProfileRecordRawCrossMatrix(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileRawCrossMatrixCount++;
#else
    sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count++;
#endif
}

static inline void ndsRendererProfileRecordSubmitClass(
    NDSRendererHWSubmitClass submit_class)
{
    if ((u32)submit_class < NDS_RENDERER_HW_SUBMIT_CLASS_COUNT)
    {
        sNdsRendererHardwareSubmitClassCounts[submit_class]++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        {
            NDSRendererProfileOwnerHotLedger *owner =
                ndsRendererProfileCurrentOwner();

            if (owner != NULL)
            {
                owner->submit_class_count[(u32)submit_class]++;
            }
        }
#endif
    }
}

static inline void ndsRendererProfileRecordSourceVertexLoad(void)
{
    sNdsRendererHardwareSourceVertexLoadCount++;
}

static inline void ndsRendererProfileRecordCPUTransform(void)
{
    sNdsRendererHardwareCPUTransformCount++;
}

static inline void ndsRendererProfileRecordTransformCacheHit(void)
{
    sNdsRendererHardwareTransformCacheHitCount++;
}

static inline void ndsRendererProfileRecordMatrixSnapshotCreate(void)
{
    sNdsRendererHardwareMatrixSnapshotCreateCount++;
}

static inline void ndsRendererProfileRecordMatrixSnapshotReuse(void)
{
    sNdsRendererHardwareMatrixSnapshotReuseCount++;
}

static inline void ndsRendererProfileRecordMatrixSnapshotOverflow(void)
{
    sNdsRendererHardwareMatrixSnapshotOverflowCount++;
}

static void ndsRendererProfileResetSubmitSummary(void)
{
    memset(sNdsRendererHardwareSubmitClassCounts, 0,
           sizeof(sNdsRendererHardwareSubmitClassCounts));
    sNdsRendererHardwareDivideSummary = 0u;
    sNdsRendererHardwareSourceVertexLoadCount = 0u;
    sNdsRendererHardwareCPUTransformCount = 0u;
    sNdsRendererHardwareTransformCacheHitCount = 0u;
    sNdsRendererHardwareMatrixSnapshotCreateCount = 0u;
    sNdsRendererHardwareMatrixSnapshotReuseCount = 0u;
    sNdsRendererHardwareMatrixSnapshotOverflowCount = 0u;
}

static void ndsRendererProfilePublishSubmitSummary(void)
{
    gNdsRendererProfileLevel = NDS_RENDERER_PROFILE_LEVEL;
    gNdsRendererProfileSubmitRawCurrentCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX];
    gNdsRendererProfileSubmitRawSnapshotCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX];
    gNdsRendererProfileSubmitProjectedCrossCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX];
    gNdsRendererProfileSubmitProjectedNoZCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z];
    gNdsRendererProfileSubmitProjectedDecalCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL];
    gNdsRendererProfileSubmitProjectedPrimDepthCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH];
    gNdsRendererProfileSubmitProjectedRangeOrMatrixCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX];
    gNdsRendererProfileSubmitRejectCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_REJECT];
    gNdsRendererProfileHardwareDivideSummary =
        sNdsRendererHardwareDivideSummary;
    gNdsRendererProfileSourceVertexLoadCount =
        sNdsRendererHardwareSourceVertexLoadCount;
    gNdsRendererProfileCPUTransformCount =
        sNdsRendererHardwareCPUTransformCount;
    gNdsRendererProfileTransformCacheHitCount =
        sNdsRendererHardwareTransformCacheHitCount;
    gNdsRendererProfileMatrixSnapshotCreateCount =
        sNdsRendererHardwareMatrixSnapshotCreateCount;
    gNdsRendererProfileMatrixSnapshotReuseCount =
        sNdsRendererHardwareMatrixSnapshotReuseCount;
    gNdsRendererProfileMatrixSnapshotOverflowCount =
        sNdsRendererHardwareMatrixSnapshotOverflowCount;
}

static inline void ndsRendererProfileRecordHardwareTriangle(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHardwareTriangles++;
    gNdsRendererProfileHardwareVertices += 3u;
    if ((gNdsRendererProfileHardwareTriangles > 2048u) ||
        (gNdsRendererProfileHardwareVertices > 6144u))
    {
        gNdsRendererProfileHardwareOverLimit = 1u;
    }
#else
    sNdsRendererRuntimeFrameSummary.hardware_triangles++;
    sNdsRendererRuntimeFrameSummary.hardware_vertices += 3u;
    if ((sNdsRendererRuntimeFrameSummary.hardware_triangles > 2048u) ||
        (sNdsRendererRuntimeFrameSummary.hardware_vertices > 6144u))
    {
        sNdsRendererRuntimeFrameSummary.hardware_over_limit = 1u;
    }
#endif
}

typedef struct NDSRendererHardwareTextureKey
{
    u32 image;
    u32 image_format;
    u32 image_size;
    u32 image_width;
    u32 tlut_image;
    u32 tlut_count;
    u32 data_layout;
    u32 format;
    u32 size;
    u32 width;
    u32 height;
    u32 render_tile;
    u32 render_tmem;
    u32 render_palette;
    u32 render_tile_cms;
    u32 render_tile_cmt;
    u32 render_tile_masks;
    u32 render_tile_maskt;
    u32 render_tile_shifts;
    u32 render_tile_shiftt;
    u32 load_tile;
    u32 load_uls;
    u32 load_ult;
    u32 load_lrs;
    u32 load_dxt;
    u32 load_texels;
    u32 tile_uls;
    u32 tile_ult;
    u32 tile_lrs;
    u32 tile_lrt;
    u32 line;
    u32 flags;
    u32 texel1_image;
    u32 texel1_image_format;
    u32 texel1_image_size;
    u32 texel1_image_width;
    u32 texel1_load_kind;
    u32 texel1_render_tmem;
    u32 texel1_render_line;
    u32 texel1_render_palette;
    u32 texel1_render_tile_cms;
    u32 texel1_render_tile_cmt;
    u32 texel1_render_tile_masks;
    u32 texel1_render_tile_maskt;
    u32 texel1_render_tile_shifts;
    u32 texel1_render_tile_shiftt;
    u32 texel1_load_tile;
    u32 texel1_load_uls;
    u32 texel1_load_ult;
    u32 texel1_load_lrs;
    u32 texel1_load_dxt;
    u32 texel1_load_texels;
    u32 texel1_tile_uls;
    u32 texel1_tile_ult;
    u32 texel1_tile_lrs;
    u32 texel1_tile_lrt;
    u32 prim_lod_fraction;
    u32 combine_w0;
    u32 combine_w1;
} NDSRendererHardwareTextureKey;

/* NO RESIDENT KEY. A slot's key lives either in the dynamic pool or in its
 * generated ROM record plus three resident pointer words -- see the
 * NDS_RENDERER_HW_TEXTURE_CACHE_COUNT note for why, and
 * ndsRendererHardwareEntryKeyEqual for the one place that has to know which.
 * Read a word through ndsRendererHardwareEntryKeyWord and a whole key through
 * ndsRendererHardwareEntryCopyKey; there is no `entry->key` to reach for.
 *
 * The remaining fields are packed rather than one-u32-each because at 69 slots
 * every four bytes spent here is 276 bytes of a budget with 72 to spare. Widths
 * are the source's own: owner_mask and the upload dimensions are u16 in
 * NDSBattlePlayableStaticTextureRecord, static_record_plus1 is bounded at 32 by
 * ndsRendererHardwareRecordBattleStaticTextureHit, and ready/pinned are
 * booleans. */
typedef struct NDSRendererHardwareTextureCacheEntry
{
    int name;
    u32 params;
    u32 source_texels;
    u32 green_texels;
    u32 nonwhite_texels;
    u32 last_used_frame;
    u32 key_generation;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 key_hash;
#endif
    u16 profile_width;
    u16 profile_height;
    u16 static_owner_mask;
    u8 ready;
    u8 pinned;
    u8 static_record_plus1;
    u8 reserved;
} NDSRendererHardwareTextureCacheEntry;

typedef struct NDSRendererHardwareResolvedTexture
{
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 name;
    u32 params;
    u32 format;
    u32 width;
    u32 height;
} NDSRendererHardwareResolvedTexture;

typedef struct NDSRendererHardwareTexel1Source
{
    const NDSRendererTextureLoadState *load;
    const NDSRendererTileState *render_tile;
    const u8 *texels;
    u32 format;
    u32 size;
    u32 width;
    u32 height;
    u32 source_width;
    u32 source_extent_width;
    u32 source_extent_height;
    u32 source_texels;
    u32 source_origin_s;
    u32 source_origin_t;
    u32 palette_base;
    s32 materialize_s;
    s32 materialize_t;
} NDSRendererHardwareTexel1Source;

typedef struct NDSRendererHardwareLightDirection
{
    s32 x;
    s32 y;
    s32 z;
} NDSRendererHardwareLightDirection;

#if NDS_RENDERER_PROFILE_LEVEL < 2
typedef struct NDSRendererHardwareCi4IndexCacheEntry
{
    const u8 *source;
    u32 source_texels;
    u32 byte_lane_xor;
    u32 valid;
    u8 indices[NDS_RENDERER_HW_CI4_INDEX_CACHE_TEXELS];
} NDSRendererHardwareCi4IndexCacheEntry;
#endif

typedef struct NDSRendererHardwareLightShadeCacheEntry
{
    u32 valid;
    u32 diffuse;
    u32 ambient;
    u32 rgb[NDS_RENDERER_HW_LIGHT_SHADE_LUT_COUNT];
} NDSRendererHardwareLightShadeCacheEntry;

static NDSRendererHardwareTextureCacheEntry
    sNdsRendererHardwareTextureCache[NDS_RENDERER_HW_TEXTURE_CACHE_COUNT];
/* Dynamic slot i owns pool entry i - STATIC_COUNT. The partition IS the pool
 * index, which is why no entry carries one. */
static NDSRendererHardwareTextureKey
    sNdsRendererHardwareTextureKeyPool[NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT];
/* The three words a generated record cannot supply, because ROM stores asset
 * OFFSETS and a live key holds loaded ADDRESSES. Indexed by static slot, which
 * is the record index. Order is image, tlut, texel1. */
static u32 sNdsRendererHardwareStaticKeyPointers[
    NDS_RENDERER_HW_TEXTURE_STATIC_COUNT][3];
#if NDS_RENDERER_PROFILE_LEVEL < 2
static u8 sNdsRendererHardwareTextureLookup[
    NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT];
#if defined(__arm__)
_Static_assert(sizeof(NDSRendererHardwareTextureKey) == 236u,
               "texture key layout must remain exact-match stable");
_Static_assert(
    (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT &
     (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u)) == 0u,
    "texture lookup count must remain a power of two");
_Static_assert(NDS_RENDERER_HW_TEXTURE_CACHE_COUNT <
                   NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT,
               "texture lookup must retain an empty cluster terminator");
/* The lookup stores slot + 1 in a u8, so 254 slots is its hard ceiling. */
_Static_assert(NDS_RENDERER_HW_TEXTURE_CACHE_COUNT <= 254u,
               "texture lookup stores slot+1 in a u8");
/* The whole point of the exercise: this must not exceed the 14,016 bytes the
 * 48-entry resident-key cache spent, because +14KB of bss took
 * gNdsTaskmanGeneralHeapFreeMin under the GObj cap and the ROM stopped booting.
 * 13,944 today. Raise CACHE_COUNT and this is what refuses. */
_Static_assert(sizeof(sNdsRendererHardwareTextureCache) +
                       sizeof(sNdsRendererHardwareTextureKeyPool) +
                       sizeof(sNdsRendererHardwareStaticKeyPointers) <=
                   14016u,
               "texture cache storage must stay at or under the 48x292 budget");
#endif
#endif
#if defined(__arm__)
/* A static slot's key is its record's key_words with three words replaced, so
 * these four have to agree or the reconstruction silently compares the wrong
 * bytes. */
_Static_assert(sizeof(NDSRendererHardwareTextureKey) ==
                   NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT *
                       sizeof(u32),
               "texture key and generated record must hold the same words");
_Static_assert(__builtin_offsetof(NDSRendererHardwareTextureKey, image) ==
                   NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD * sizeof(u32),
               "image word index disagrees with the generated record");
_Static_assert(__builtin_offsetof(NDSRendererHardwareTextureKey, tlut_image) ==
                   NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD * sizeof(u32),
               "tlut word index disagrees with the generated record");
_Static_assert(__builtin_offsetof(NDSRendererHardwareTextureKey,
                                  texel1_image) ==
                   NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD * sizeof(u32),
               "texel1 word index disagrees with the generated record");
#endif

#define NDS_RENDERER_HW_TEXTURE_KEY_WORD(field) \
    ((u32)(__builtin_offsetof(NDSRendererHardwareTextureKey, field) / \
           sizeof(u32)))

static u32 ndsRendererHardwareEntrySlot(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    return (u32)(entry - sNdsRendererHardwareTextureCache);
}

/* The writable pool key of a dynamic slot; NULL for a static one. */
static NDSRendererHardwareTextureKey *ndsRendererHardwareEntryDynamicKey(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    u32 slot;

    if (entry == NULL)
    {
        return NULL;
    }
    slot = ndsRendererHardwareEntrySlot(entry);
    if ((slot < NDS_RENDERER_HW_TEXTURE_STATIC_COUNT) ||
        (slot >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT))
    {
        return NULL;
    }
    return &sNdsRendererHardwareTextureKeyPool[
        slot - NDS_RENDERER_HW_TEXTURE_STATIC_COUNT];
}

/* The generated record backing a static slot; NULL for a dynamic or empty one. */
static const NDSBattlePlayableStaticTextureRecord *
ndsRendererHardwareEntryStaticRecord(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    if ((entry == NULL) || (entry->static_record_plus1 == 0u) ||
        (ndsRendererHardwareEntrySlot(entry) >=
         NDS_RENDERER_HW_TEXTURE_STATIC_COUNT))
    {
        return NULL;
    }
    return ndsBattlePlayableStaticTextureRecordAt(
        (u32)entry->static_record_plus1 - 1u);
}

static u32 ndsRendererHardwareEntryKeyWord(
    const NDSRendererHardwareTextureCacheEntry *entry, u32 word)
{
    const NDSRendererHardwareTextureKey *dynamic;
    const NDSBattlePlayableStaticTextureRecord *record;

    if (word >= NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT)
    {
        return 0u;
    }
    dynamic = ndsRendererHardwareEntryDynamicKey(entry);
    if (dynamic != NULL)
    {
        return ((const u32 *)dynamic)[word];
    }
    record = ndsRendererHardwareEntryStaticRecord(entry);
    if (record == NULL)
    {
        return 0u;
    }
    switch (word)
    {
    case NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD:
        return sNdsRendererHardwareStaticKeyPointers[
            ndsRendererHardwareEntrySlot(entry)][0];
    case NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD:
        return sNdsRendererHardwareStaticKeyPointers[
            ndsRendererHardwareEntrySlot(entry)][1];
    case NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD:
        return sNdsRendererHardwareStaticKeyPointers[
            ndsRendererHardwareEntrySlot(entry)][2];
    default:
        break;
    }
    return record->key_words[word];
}

/* Materialise a whole key. 236 bytes of copy -- callers that want one word use
 * ndsRendererHardwareEntryKeyWord, and the lookup compares in place. */
static void ndsRendererHardwareEntryCopyKey(
    const NDSRendererHardwareTextureCacheEntry *entry,
    NDSRendererHardwareTextureKey *out)
{
    const NDSRendererHardwareTextureKey *dynamic;
    const NDSBattlePlayableStaticTextureRecord *record;
    const u32 *pointers;

    if (out == NULL)
    {
        return;
    }
    dynamic = ndsRendererHardwareEntryDynamicKey(entry);
    if (dynamic != NULL)
    {
        *out = *dynamic;
        return;
    }
    record = ndsRendererHardwareEntryStaticRecord(entry);
    if (record == NULL)
    {
        memset(out, 0, sizeof(*out));
        return;
    }
    memcpy(out, record->key_words, sizeof(*out));
    pointers =
        sNdsRendererHardwareStaticKeyPointers[
            ndsRendererHardwareEntrySlot(entry)];
    out->image = pointers[0];
    out->tlut_image = pointers[1];
    out->texel1_image = pointers[2];
}

/* THE ONE PLACE THAT KNOWS A SLOT MAY NOT OWN ITS KEY.
 *
 * Dynamic slots compare against the pool exactly as the resident key used to.
 * Static slots compare the three runtime pointer words against RAM and the
 * other 56 against ROM -- which is exact, not an approximation, because the
 * prepare built the key by memcpy-ing key_words and then overwriting precisely
 * those three. Word 0, word 4 and word 32 leave three contiguous spans. */
static s32 ndsRendererHardwareEntryKeyEqual(
    const NDSRendererHardwareTextureCacheEntry *entry,
    const NDSRendererHardwareTextureKey *key)
{
    const NDSRendererHardwareTextureKey *dynamic;
    const NDSBattlePlayableStaticTextureRecord *record;
    const u32 *pointers;
    const u32 *words;

    if ((entry == NULL) || (key == NULL))
    {
        return FALSE;
    }
    dynamic = ndsRendererHardwareEntryDynamicKey(entry);
    if (dynamic != NULL)
    {
        return (memcmp(dynamic, key, sizeof(*key)) == 0) ? TRUE : FALSE;
    }
    record = ndsRendererHardwareEntryStaticRecord(entry);
    if (record == NULL)
    {
        return FALSE;
    }
    pointers =
        sNdsRendererHardwareStaticKeyPointers[
            ndsRendererHardwareEntrySlot(entry)];
    words = (const u32 *)key;
    if ((words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD] !=
         pointers[0]) ||
        (words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD] !=
         pointers[1]) ||
        (words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD] !=
         pointers[2]))
    {
        return FALSE;
    }
    return ((memcmp(&record->key_words[1], &words[1],
                    3u * sizeof(u32)) == 0) &&
            (memcmp(&record->key_words[5], &words[5],
                    27u * sizeof(u32)) == 0) &&
            (memcmp(&record->key_words[33], &words[33],
                    26u * sizeof(u32)) == 0)) ? TRUE : FALSE;
}

static void ndsRendererHardwareEntrySetKey(
    NDSRendererHardwareTextureCacheEntry *entry,
    const NDSRendererHardwareTextureKey *key)
{
    NDSRendererHardwareTextureKey *dynamic =
        ndsRendererHardwareEntryDynamicKey(entry);

    if ((dynamic == NULL) || (key == NULL))
    {
        return;
    }
    *dynamic = *key;
}

/* A static slot keeps only the three pointer words; the rest is its record. */
static void ndsRendererHardwareEntrySetStaticKey(
    NDSRendererHardwareTextureCacheEntry *entry,
    const NDSRendererHardwareTextureKey *key)
{
    u32 slot = ndsRendererHardwareEntrySlot(entry);
    u32 *pointers;

    if ((key == NULL) || (slot >= NDS_RENDERER_HW_TEXTURE_STATIC_COUNT))
    {
        return;
    }
    pointers = sNdsRendererHardwareStaticKeyPointers[slot];
    pointers[0] = key->image;
    pointers[1] = key->tlut_image;
    pointers[2] = key->texel1_image;
}

static void ndsRendererHardwareEntryClearKey(
    NDSRendererHardwareTextureCacheEntry *entry)
{
    NDSRendererHardwareTextureKey *dynamic =
        ndsRendererHardwareEntryDynamicKey(entry);
    u32 slot;

    if (dynamic != NULL)
    {
        memset(dynamic, 0, sizeof(*dynamic));
        return;
    }
    slot = ndsRendererHardwareEntrySlot(entry);
    if (slot < NDS_RENDERER_HW_TEXTURE_STATIC_COUNT)
    {
        memset(sNdsRendererHardwareStaticKeyPointers[slot], 0,
               sizeof(sNdsRendererHardwareStaticKeyPointers[slot]));
    }
}

static u32 sNdsRendererHardwareTextureCacheNext;
static u32 sNdsRendererHardwareTextureKeyGeneration;
static u32 sNdsRendererHardwareFrameSerial;
static const NDSRendererHardwareTextureCacheEntry
    *sNdsRendererHardwareActiveTextureEntry;

#if NDS_R2_PARTICLE_RUNTIME
/* Declared here rather than beside the atlas prepare because the static-texture
 * ownership guard below runs long before it and has to recognise the atlas. */
/* Every sheet is a PINNED cache entry for the life of the battle, so the sheet
 * count is spent out of the same 48 slots the static corpus takes 24 of. Four
 * leaves 20 evictable, which the stage and fighters share; this is the bound
 * that would bite first if coverage were bought by adding sheets indefinitely,
 * and it is a slot count rather than a byte count. */
_Static_assert(NDS_PARTICLE_QUAD_ATLAS_SHEETS <= 8u,
               "particle atlas sheets would crowd the texture cache");
static int sNdsRendererParticleAtlasName[NDS_PARTICLE_QUAD_ATLAS_SHEETS];
#if NDS_R2_WHISPY_NATIVE_TEXTURES
_Static_assert(NDS_PARTICLE_QUAD_ATLAS_SHEETS +
                   NDS_WHISPY_NATIVE_TEXTURE_COUNT +
                   NDS_R2_FOX_BLASTER_GLOW_AOT <= 8u,
               "particle native textures would crowd the texture cache");
static int sNdsRendererWhispyNativeName[NDS_WHISPY_NATIVE_TEXTURE_COUNT];
#if NDS_R2_FOX_BLASTER_GLOW_AOT
static int sNdsRendererFoxBlasterGlowName;
#endif
#if NDS_R2_WHISPY_NATIVE_AOT
/* glBindTexture looks each GL name up through libnds's DynamicArray before it
 * writes TEXIMAGE_PARAM and PLTT_BASE.  These three textures are immutable and
 * pinned for the whole scene, so capture the exact two hardware words and the
 * matching libnds active-name state once at upload.  The route-3 lab arm can
 * then perform the identical register transition without a per-particle name
 * lookup; route 4 uses the same words in a packed GXFIFO stream. */
typedef struct NDSRendererWhispyNativeBinding
{
    u32 texture_name;
    u32 texture_format;
    u32 palette_format;
    s32 palette_name;
    u32 valid;
} NDSRendererWhispyNativeBinding;

static NDSRendererWhispyNativeBinding
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT + NDS_R2_FOX_BLASTER_GLOW_AOT];
#endif
#endif
static u32 sNdsRendererParticleAtlasPrepared;
/* THE WHOLE PALETTE BLOCK, NOT ONE TABLE: NDS_PARTICLE_QUAD_ATLAS_SHEETS
 * tables of NDS_PARTICLE_QUAD_PALETTE_ENTRIES laid end to end, which is how the
 * generator writes it. Each sheet gets its own colours; see the note at the
 * glColorTableEXT call in ndsRendererHardwarePrepareParticleAtlas.
 *
 * ndsRendererHardwarePrepareWhispyNativeTextures borrows this buffer as palette
 * scratch, which stays safe because it runs after all four sheets are uploaded
 * and its own guard still bounds it by NDS_PARTICLE_QUAD_PALETTE_ENTRIES. */
static u16 sNdsRendererParticleAtlasPalette[
    NDS_PARTICLE_QUAD_PALETTE_BYTES / sizeof(u16)];
/* The upload loop indexes sheet * NDS_PARTICLE_QUAD_PALETTE_ENTRIES and the
 * read above takes sizeof(), so a generator that ever emitted a different
 * number of tables than sheets would walk off the end silently rather than
 * fail. Impossible to see on screen; free to catch here. */
_Static_assert(NDS_PARTICLE_QUAD_PALETTE_BYTES ==
                   NDS_PARTICLE_QUAD_ATLAS_SHEETS *
                       NDS_PARTICLE_QUAD_PALETTE_STRIDE_BYTES,
               "particle atlas palette block must hold one table per sheet");
_Static_assert(NDS_PARTICLE_QUAD_PALETTE_STRIDE_BYTES ==
                   NDS_PARTICLE_QUAD_PALETTE_ENTRIES * sizeof(u16),
               "particle atlas palette stride must be its entry count");

static s32 ndsRendererParticleAtlasOwnsName(int name)
{
    u32 sheet;

    for (sheet = 0u; sheet < NDS_PARTICLE_QUAD_ATLAS_SHEETS; sheet++)
    {
        if (sNdsRendererParticleAtlasName[sheet] == name)
        {
            return TRUE;
        }
    }
#if NDS_R2_WHISPY_NATIVE_TEXTURES
    for (sheet = 0u; sheet < NDS_WHISPY_NATIVE_TEXTURE_COUNT; sheet++)
    {
        if (sNdsRendererWhispyNativeName[sheet] == name)
        {
            return TRUE;
        }
    }
#if NDS_R2_FOX_BLASTER_GLOW_AOT
    if (sNdsRendererFoxBlasterGlowName == name)
    {
        return TRUE;
    }
#endif
#endif
    return FALSE;
}
#endif

static void ndsRendererHardwareRecordBattleStaticTextureHit(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    u32 record_index;

    if ((sNdsRendererBattleStaticTextureArmed == 0u) ||
        (entry == NULL))
    {
        return;
    }
#if NDS_R2_PARTICLE_RUNTIME
    /* The particle atlas is a legitimate second owner of battle texture VRAM,
     * not a leak. This guard predates it and asks "is every texture bound
     * during an armed battle a pinned STATIC RECORD" -- true while the static
     * set was the only pinned resident, and false the moment the particle
     * draw binds its atlas, which reported ViolationCount 1 for a texture that
     * is exactly where it should be. Exempted by identity rather than by
     * loosening the test, so a genuinely unowned binding still trips it.
     *
     * The ViolationCount 1 / stage-rebuilds-197 pair this was first aimed at
     * was NOT an ownership question -- it was texture VRAM, and this exemption
     * could never have fixed it. Attributed 2026-08-01 by three tick-HUD ROMs
     * differing only in the particle flags: control and RUNTIME=1 both report
     * 0 and 2, DRAW=1 reports 1 and 197 and aborts at the GO countdown. The
     * prepare ORDER in ndsSCVSBattleBeginSceneTextures is where that is fixed.
     * This stays because it is independently correct: the atlas is a pinned
     * resident with no static record, which is exactly what the test asks
     * about. */
    if ((entry->pinned != 0u) && (entry->static_record_plus1 == 0u) &&
        (sNdsRendererParticleAtlasPrepared != 0u) &&
        (ndsRendererParticleAtlasOwnsName(entry->name) != FALSE))
    {
        return;
    }
#endif
    if ((entry->pinned == 0u) || (entry->static_record_plus1 == 0u) ||
        (entry->static_record_plus1 > 32u))
    {
        gNdsRendererBattleStaticTextureViolationCount++;
        return;
    }
    record_index = entry->static_record_plus1 - 1u;
    gNdsRendererBattleStaticTexturePinnedHitCount++;
    gNdsRendererBattleStaticTextureSeenMask |= 1u << record_index;
    gNdsRendererBattleStaticTextureOwnerMask |= entry->static_owner_mask;
}
#if NDS_SCENE_MIP_CACHE_LAB
#define NDS_RENDERER_SCENE_MIP_COUNT 3u
#define NDS_RENDERER_SCENE_MIP_SIZE 128u
static int sNdsRendererSceneMipTextureNames[
    NDS_RENDERER_SCENE_MIP_COUNT];
#endif
static u16 sNdsRendererHardwareTextureScratch[
    NDS_RENDERER_HW_TEXTURE_MAX_TEXELS];
/* Sixteen entries is the whole of a GL_RGB16 palette; the static corpus never
 * needs more (generate_battle_playable_static_textures.py falls back to direct
 * colour above that). */
/* The whole palette block, read once per prepare. Per-record reads cost 22
 * extra NitroFS round trips inside the longest pause in the game. */
static u8 sNdsRendererStaticTexturePaletteBlock[
    NDS_BATTLE_STATIC_TEXTURE_PALETTE_BLOCK_MAX_BYTES];
static u16 sNdsRendererStaticTexturePalette[16];

/* Bytes the record's payload span occupies, which is its ENCODING's business
 * and not upload_width x upload_height x 2 any more. 0 means "not a format this
 * uploader knows", which every caller treats as a rejection. */
static u32 ndsRendererStaticTextureSpanBytes(
    const NDSBattlePlayableStaticTextureRecord *record)
{
    u32 texels;

    if (record == NULL)
    {
        return 0u;
    }
    texels = (u32)record->upload_width * (u32)record->upload_height;
    if (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL16)
    {
        return (texels + 1u) >> 1;
    }
    if (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_RGBA)
    {
        return texels * sizeof(u16);
    }
    return 0u;
}
#if NDS_RENDERER_PROFILE_LEVEL < 2
#define NDS_RENDERER_HW_TEXTURE_REFRESH_QUEUE_COUNT 2u
#define NDS_RENDERER_HW_TEXTURE_REFRESH_SMALL_TEXELS 2048u
#define NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS 64u
typedef struct NDSRendererHardwareTextureRefresh
{
    NDSRendererHardwareTextureCacheEntry *entry;
    const u16 *pixels;
    u32 staged_bytes;
    u32 texture_bytes;
    u32 row_bytes;
    u32 row_count;
    u8 row_map[NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
} NDSRendererHardwareTextureRefresh;
static NDSRendererHardwareTextureRefresh
    sNdsRendererHardwareTextureRefreshQueue[
        NDS_RENDERER_HW_TEXTURE_REFRESH_QUEUE_COUNT];
static u32 sNdsRendererHardwareTextureRefreshCount;
static u16 sNdsRendererHardwareTextureRefreshSmall[
    NDS_RENDERER_HW_TEXTURE_REFRESH_SMALL_TEXELS];
static u16 sNdsRendererHardwareTextureRefreshLarge[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH *
    NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS];
#endif
static u8 sNdsRendererHardwareTexel01Ci4Source0S[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH];
static u8 sNdsRendererHardwareTexel01Ci4Source1S[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH];
static u8 sNdsRendererHardwareTexel01Ci4Source0T[
    NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
static u8 sNdsRendererHardwareTexel01Ci4Source1T[
    NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
#if NDS_RENDERER_PROFILE_LEVEL < 2
static u8 sNdsRendererHardwareTexel01Ci4RepresentativeS[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH];
static u8 sNdsRendererHardwareTexel01Ci4RepresentativeT[
    NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
static u32 sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid;
static u32 sNdsRendererHardwareTexel01Ci4ClassTable[
    NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT];
#if defined(__arm__)
_Static_assert(
    sizeof(sNdsRendererHardwareTexel01Ci4Source0T) +
    sizeof(sNdsRendererHardwareTexel01Ci4Source1T) +
    sizeof(sNdsRendererHardwareTexel01Ci4RepresentativeS) +
    sizeof(sNdsRendererHardwareTexel01Ci4RepresentativeT) == 512u,
    "CI4 representative maps must stay within 512 bytes");
_Static_assert(sizeof(sNdsRendererHardwareTexel01Ci4ClassTable) == 1024u,
               "CI4 representative class table must stay within 1 KiB");
_Static_assert(
    (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT &
     (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT - 1u)) == 0u,
    "CI4 representative class table must remain a power of two");
_Static_assert(
    (NDS_RENDERER_HW_TEXTURE_MAX_WIDTH <=
     (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT / 2u)) &&
    (NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT <=
     (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT / 2u)),
    "CI4 representative class table must remain at most half full");
#endif
#endif
static u32 sNdsRendererHardwareTexel01Ci4PairLut[
    NDS_RENDERER_HW_TEXEL01_CI4_LUT_COUNT];
#if defined(__arm__)
_Static_assert(sizeof(sNdsRendererHardwareTexel01Ci4PairLut) == 1024u,
               "phase-masked CI4 pair lookup must stay within 1 KiB");
#endif
static u16 sNdsRendererHardwareTexel01Ci4LutPalette0[16];
static u16 sNdsRendererHardwareTexel01Ci4LutPalette1[16];
static u32 sNdsRendererHardwareTexel01Ci4LutFraction;
static u32 sNdsRendererHardwareTexel01Ci4LutKeyValid;
#if NDS_RENDERER_PROFILE_LEVEL < 2
static NDSRendererHardwareCi4IndexCacheEntry
    sNdsRendererHardwareCi4IndexCache[
        NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT];
static u32 sNdsRendererHardwareCi4IndexCacheNext;
#if defined(__arm__)
_Static_assert(sizeof(sNdsRendererHardwareCi4IndexCache) == 2080u,
               "CI4 index cache must stay within 2080 bytes");
#endif
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
static NDSRendererHardwareLightShadeCacheEntry
    sNdsRendererHardwareLightShadeCache[
        NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT];
static u32 sNdsRendererHardwareLightShadeCacheNext;
#if defined(__arm__)
_Static_assert(sizeof(sNdsRendererHardwareLightShadeCache) == 4192u,
               "light shade lookup cache must stay within 4192 bytes");
#endif
#endif

static void ndsRendererHardwareEndBatch(void);

static inline s32 ndsRendererHardwareRawVertexFits(
    const NDSRendererInputVertex *vtx)
{
    if (vtx == NULL)
    {
        return FALSE;
    }
    return ((vtx->x >= NDS_RENDERER_HW_RAW_COORD_MIN) &&
            (vtx->x <= NDS_RENDERER_HW_RAW_COORD_MAX) &&
            (vtx->y >= NDS_RENDERER_HW_RAW_COORD_MIN) &&
            (vtx->y <= NDS_RENDERER_HW_RAW_COORD_MAX) &&
            (vtx->z >= NDS_RENDERER_HW_RAW_COORD_MIN) &&
            (vtx->z <= NDS_RENDERER_HW_RAW_COORD_MAX)) ? TRUE : FALSE;
}

static inline u32 ndsRendererHardwareDivideLitDotBy127(u32 dot)
{
    u32 biased = dot + 1u;

    /* The caller clamps dot to 127^2.  Over that complete domain this is
     * exactly floor(dot / 127), which is the reduced source expression
     * floor((dot * 127) / (127 * 127)). */
    return (biased + (biased >> 7)) >> 7;
}

static inline u32 ndsRendererHardwareLitShadeColorLut(
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction,
    const u32 *rgb_lut)
{
    s32 dot;
    u32 diffuse_numer;

    dot = ((s32)(s8)vtx->r * direction->x) +
        ((s32)(s8)vtx->g * direction->y) +
        ((s32)(s8)vtx->b * direction->z);
    if (dot <= 0)
    {
        diffuse_numer = 0u;
    }
    else if (dot > (127 * 127))
    {
        diffuse_numer = 127u;
    }
    else
    {
        diffuse_numer = ndsRendererHardwareDivideLitDotBy127((u32)dot);
    }
    return rgb_lut[diffuse_numer] | (u32)vtx->a;
}
#endif
