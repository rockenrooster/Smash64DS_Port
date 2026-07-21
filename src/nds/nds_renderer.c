#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <nds/battle_playable_static_textures.h>
#include <nds/nds_gbi_decode.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/nds_startup.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
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
#else
#define NDS_RENDERER_HOT_CODE __attribute__((hot, optimize("O3")))
#define NDS_RENDERER_FAST_RUN_CODE \
    __attribute__((noinline, optimize("O3")))
#define NDS_RENDERER_NATIVE_FIGHTER_CODE \
    __attribute__((noinline, hot, optimize("O3")))
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

#if NDS_TASK36_HW_COMPOSE == 2
static void ndsRendererTask36ReplayRecord(
    NDSRendererTask29GXClass command_class,
    const u32 *words,
    u32 word_count);
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
    ndsRendererTask36ReplayRecord(command_class, words, word_count);
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
    ndsRendererTask36ReplayRecord(command_class, words, word_count);
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

static inline void ndsRendererHardwareWriteColorWord(u32 value)
{
#if !NDS_RENDERER_HW_TRIANGLES
    (void)value;
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    ndsRendererBenchmarkSinkWord(value);
#else
#if NDS_TASK29_GX_CENSUS || NDS_TASK34_STAGE_STREAM_CENSUS || \
    (NDS_TASK36_HW_COMPOSE == 2)
    ndsRendererTask29GXRecord(NDS_TASK29_GX_COLOR, &value, 1u);
#endif
    GFX_COLOR = value;
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
    (NDS_TASK36_HW_COMPOSE == 2)
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
    (NDS_TASK36_HW_COMPOSE == 2)
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
#define NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT 4u
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
#define NDS_RENDERER_HW_TEXTURE_CACHE_COUNT 48u
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
#define NDS_RENDERER_CCMUX_ZERO_D 7u
#define NDS_RENDERER_ACMUX_COMBINED 0u
#define NDS_RENDERER_ACMUX_TEXEL0 1u
#define NDS_RENDERER_ACMUX_TEXEL1 2u
#define NDS_RENDERER_ACMUX_PRIMITIVE 3u
#define NDS_RENDERER_ACMUX_SHADE 4u
#define NDS_RENDERER_ACMUX_ENVIRONMENT 5u
#define NDS_RENDERER_ACMUX_1 6u
#define NDS_RENDERER_ACMUX_0 7u
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

static NDSRendererRuntimeFrameSummary sNdsRendererRuntimeFrameSummary;
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

static inline void ndsRendererProfileRecordMatrixLoad(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileMatrixLoadCount++;
#else
    sNdsRendererRuntimeFrameSummary.matrix_load_count++;
#endif
}

static inline void ndsRendererProfileRecordBatchBegin(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHardwareBatchBeginCount++;
#else
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
#else
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count++;
#endif
}

static inline void ndsRendererProfileRecordBatchEnd(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHardwareBatchEndCount++;
#else
    sNdsRendererRuntimeFrameSummary.hardware_batch_end_count++;
#endif
}

static inline void ndsRendererProfileRecordTexturePrepare(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexturePrepareCount++;
#else
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
#else
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

typedef struct NDSRendererHardwareTextureCacheEntry
{
    int name;
    u32 ready;
    u32 pinned;
    u32 static_record_plus1;
    u32 static_owner_mask;
    u32 params;
    u32 source_texels;
    u32 green_texels;
    u32 nonwhite_texels;
    u32 profile_width;
    u32 profile_height;
    u32 last_used_frame;
    u32 key_generation;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 key_hash;
#endif
    NDSRendererHardwareTextureKey key;
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
#endif
#endif
static u32 sNdsRendererHardwareTextureCacheNext;
static u32 sNdsRendererHardwareTextureKeyGeneration;
static u32 sNdsRendererHardwareFrameSerial;
static const NDSRendererHardwareTextureCacheEntry
    *sNdsRendererHardwareActiveTextureEntry;

static void ndsRendererHardwareRecordBattleStaticTextureHit(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    u32 record_index;

    if ((sNdsRendererBattleStaticTextureArmed == 0u) ||
        (entry == NULL))
    {
        return;
    }
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
_Static_assert(sizeof(sNdsRendererHardwareLightShadeCache) == 2096u,
               "light shade lookup cache must stay within 2096 bytes");
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

typedef struct NDSRendererTraversalVertexStorage
{
    NDSRendererClipVertex20p12 vertices[NDS_RENDERER_MAX_VTX];
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererInputVertex input_vertices[NDS_RENDERER_MAX_VTX];
    u32 vertex_colors[NDS_RENDERER_MAX_VTX];
    u8 vertex_matrix_snapshot[NDS_RENDERER_MAX_VTX];
    u8 vertex_clip_snapshot[NDS_RENDERER_MAX_VTX];
#endif
} NDSRendererTraversalVertexStorage;

typedef struct NDSRendererTraversalState
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    NDSRendererMatrix20p12 matrix;
    Mtx matrix_word_raw;
    NDSRendererMatrix20p12 modelview_stack[NDS_RENDERER_MODELVIEW_STACK_SIZE];
    NDSRendererClipVertex20p12 *vertices;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererInputVertex *input_vertices;
    u32 *vertex_colors;
    NDSRendererMatrixSnapshot *matrix_snapshots;
    u8 *vertex_matrix_snapshot;
    u8 *vertex_clip_snapshot;
    const Gfx *source_command_site;
#endif
    u32 modelview_valid_stack[NDS_RENDERER_MODELVIEW_STACK_SIZE];
    u32 modelview_stack_depth;
    u32 vertex_valid_mask;
#if NDS_RENDERER_HW_TRIANGLES
    u32 input_vertex_valid_mask;
    u32 vertex_color_valid_mask;
    u32 current_transform_vertex_mask;
    u32 matrix_generation;
    u32 matrix_snapshot_count;
    u32 current_matrix_snapshot;
    NDSRendererHardwareLightDirection prepared_light_direction;
    u32 prepared_light_direction_valid;
    u32 texture_prepare_valid;
    u32 texture_prepare_enabled;
    u32 texture_prepare_name;
    u32 texture_prepare_material_color;
    u32 texture_prepare_scale_s;
    u32 texture_prepare_scale_t;
    u32 texture_prepare_origin_s;
    u32 texture_prepare_origin_t;
    s32 texture_prepare_offset;
    u32 texture_prepare_vertex_flags;
    u32 texture_prepare_source_zbuffered;
    u32 texture_prepare_decal_depth;
    u32 texture_prepare_prim_depth;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 texture_prepare_key_hash;
    u32 texture_prepare_params;
    u32 semantic_branch_path;
    u32 semantic_command_index;
    u32 semantic_tri2_half;
#endif
    u32 texture_prepare_alpha_constant;
    u32 texture_prepare_poly_alpha;
    u32 texture_prepare_poly_fmt;
    u16 prepared_vertex_colors[NDS_RENDERER_MAX_VTX];
    s16 prepared_texcoord_s[NDS_RENDERER_MAX_VTX];
    s16 prepared_texcoord_t[NDS_RENDERER_MAX_VTX];
    v16 prepared_projected_x[NDS_RENDERER_MAX_VTX];
    v16 prepared_projected_y[NDS_RENDERER_MAX_VTX];
    v16 prepared_projected_source_z[NDS_RENDERER_MAX_VTX];
    u32 prepared_vertex_color_valid_mask;
    u32 prepared_texcoord_valid_mask;
    u32 prepared_projected_xy_valid_mask;
    u32 prepared_projected_source_z_valid_mask;
#endif
    u32 projection_valid;
    u32 modelview_valid;
    u32 matrix_valid;
    u32 matrix_word_valid;
#if NDS_RENDERER_HW_TRIANGLES
    u32 raw_vertex_fit_mask;
#endif
} NDSRendererTraversalState;

#if NDS_RENDERER_HW_TRIANGLES
#define NDS_NATIVE_STATE_OTHERMODE 2u
#define NDS_NATIVE_STATE_COMBINE 3u
#define NDS_NATIVE_STATE_TEXTURE 4u
#define NDS_NATIVE_STATE_GEOMETRY 5u
#define NDS_NATIVE_STATE_IMAGE 6u
#define NDS_NATIVE_STATE_TILE 7u
#define NDS_NATIVE_STATE_LOAD_TLUT 8u
#define NDS_NATIVE_STATE_LOAD_BLOCK 9u
#define NDS_NATIVE_STATE_TILE_SIZE 10u
#define NDS_NATIVE_STATE_PRIM 11u
#define NDS_NATIVE_STATE_BLEND 12u
#define NDS_NATIVE_STATE_MATERIAL 13u
#define NDS_NATIVE_STATE_LIGHT_COLOR 14u
#define NDS_NATIVE_STATE_NONE 0xffffu
#define NDS_NATIVE_MATERIAL_NONE 0xffu
#define NDS_NATIVE_VERTEX_BLOCK 0u
#define NDS_NATIVE_MODIFY_ST 1u
#define NDS_NATIVE_RUN_RAW_CURRENT 0u
#define NDS_NATIVE_RUN_CROSS_MATRIX 1u
#define NDS_NATIVE_DENSE_VERTEX_COUNT 541u
#define NDS_NATIVE_ROOT_BINDING_COUNT 32u
#define NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK 0x03u
#define NDS_NATIVE_DIRECT_POLICY_TEXTURED_LIT 0u
#define NDS_NATIVE_DIRECT_POLICY_LIT_PRIM 1u
#define NDS_NATIVE_DIRECT_POLICY_LIT_ONLY 2u
#define NDS_NATIVE_DIRECT_POLICY_LIT_PRIM_ALT_ALPHA 3u
#define NDS_NATIVE_DIRECT_POLICY_CULL_NONE 0x80u
#define NDS_NATIVE_DENSE_ID_MASK 0x03ffu
#define NDS_NATIVE_DENSE_SPAN_COUNT_SHIFT 10u
#define NDS_NATIVE_PACKED_CORNER_MATRIX_SHIFT 10u
#define NDS_NATIVE_GX_MATRIX_CURRENT 31u
#define NDS_NATIVE_GX_MATRIX_SLOT_MAX 30u
#define NDS_NATIVE_SOURCE_GEOM_CULL_FRONT 0x00001000u
#define NDS_NATIVE_SOURCE_GEOM_CULL_BACK 0x00002000u
typedef struct NDSNativeStateDelta
{
    u32 w0;
    u32 w1;
    u8 effect;
    u8 reserved[3];
} NDSNativeStateDelta;

typedef struct NDSNativeVertexAction
{
    u8 kind;
    u8 command_index;
    u8 index;
    u8 count;
    u32 source_offset;
    s16 s;
    s16 t;
} NDSNativeVertexAction;

typedef struct NDSNativeDenseVertex
{
    u32 rgba;
    s16 s;
    s16 t;
    u8 matrix_binding;
    u8 cache_slot;
    u16 reserved;
} NDSNativeDenseVertex;

#if NDS_RENDERER_PROFILE_LEVEL < 2
typedef struct NDSNativePreparedDenseVertex
{
    u32 gx_xy;
    u32 shaded_rgba;
    u16 gx_z;
    u16 packed_color;
    s16 s;
    s16 t;
} NDSNativePreparedDenseVertex;
#endif

typedef struct NDSNativeRun
{
    u16 first_triangle;
    u8 triangle_count;
    u8 submit_class;
    u32 required_mask;
} NDSNativeRun;

typedef struct NDSNativeEpoch
{
    u16 before_state_first;
    u16 after_state_first;
    u16 first_action;
    u16 first_run;
    u8 before_state_count;
    u8 after_state_count;
    u8 before_sync_count;
    u8 after_sync_count;
    u8 action_count;
    u8 run_count;
    u8 material_slot;
    u8 first_triangle_command_index;
} NDSNativeEpoch;

typedef struct NDSNativeRoot
{
    u32 root_offset;
    u16 first_epoch;
    u16 tail_state_first;
    u16 source_command_count;
    u8 epoch_count;
    u8 tail_state_count;
    u8 tail_sync_count;
    u8 light_preamble;
} NDSNativeRoot;

typedef struct NDSNativeDirectPolicy
{
    u32 combine_w0;
    u32 combine_w1;
    u8 vertex_flags;
    u8 textured;
    u8 reserved[2];
} NDSNativeDirectPolicy;

_Static_assert(sizeof(NDSNativeStateDelta) == 12u,
               "native state effect ABI must stay compact");
_Static_assert(sizeof(NDSNativeDenseVertex) == 12u,
               "native dense vertex ABI must stay cache-line friendly");
#if NDS_RENDERER_PROFILE_LEVEL < 2
_Static_assert(sizeof(NDSNativePreparedDenseVertex) == 16u,
               "native prepared dense vertex ABI must stay power-of-two");
#endif
_Static_assert(sizeof(NDSNativeRun) == 8u,
               "native run ABI must stay compact");
_Static_assert(sizeof(NDSNativeEpoch) == 16u,
               "native epoch ABI must stay compact");
_Static_assert(sizeof(NDSNativeRoot) == 16u,
               "native root ABI must stay compact");
_Static_assert(sizeof(NDSNativeDirectPolicy) == 12u,
               "native direct policy ABI must stay compact");
#include "nds_native_fighter_owner.generated.inc"
#include "nds_native_stage_owner.generated.inc"

#if NDS_RENDERER_PROFILE_LEVEL < 2
#define NDS_NATIVE_FIGHTER_HIERARCHY_JOINT_MAX 27u
#define NDS_NATIVE_FIGHTER_HIERARCHY_BINDING_MAX 18u
#define NDS_NATIVE_FIGHTER_HIERARCHY_EPOCH_COUNT 49u

typedef struct NDSNativeMatrix3x3
{
    s32 m[3][3];
} NDSNativeMatrix3x3;

typedef struct NDSNativeHierarchyPreparedEpoch
{
    NDSRendererHardwareLightDirection light_direction;
    u32 light_direction_valid;
} NDSNativeHierarchyPreparedEpoch;

typedef struct NDSNativeHierarchyPreparedRun
{
    NDSRendererHardwareTextureCacheEntry *texture_entry;
    u32 texture_name;
    u32 texture_params;
    u32 texture_format;
    u32 texture_width;
    u32 texture_height;
    u32 poly_fmt;
    u32 scale_s;
    u32 scale_t;
    u32 origin_s;
    u32 origin_t;
    s32 texture_offset;
    u32 vertex_flags;
    u32 textured;
} NDSNativeHierarchyPreparedRun;

typedef struct NDSNativeFighterOwnerExecution
{
    NDSRendererTraversalState traversal;
    NDSRendererStats preflight_stats;
    NDSNativeMatrix3x3 hierarchy_world[
        NDS_NATIVE_FIGHTER_HIERARCHY_JOINT_MAX];
    NDSNativeHierarchyPreparedEpoch hierarchy_epochs[
        NDS_NATIVE_FIGHTER_HIERARCHY_EPOCH_COUNT];
    NDSNativeHierarchyPreparedRun hierarchy_runs[
        NDS_NATIVE_FIGHTER_HIERARCHY_EPOCH_COUNT];
    NDSRendererStats *stats;
    NDSRendererVertexCache *vertex_cache;
    u32 slot;
    u32 active;
} NDSNativeFighterOwnerExecution;

#if NDS_RENDERER_M2_DETAILED_LEDGER
typedef struct NDSNativeFighterShadeKey
{
    u32 owner_generation;
    u32 slot;
    u32 epoch_index;
    u32 epoch_policy;
    u32 combine_w0;
    u32 combine_w1;
    u32 policy_flags;
    u32 geometry_mode;
    u32 prim_color;
    u32 light_color_1;
    u32 light_color_2;
    u32 light_masks;
    s32 light_dir_x;
    s32 light_dir_y;
    s32 light_dir_z;
    u32 light_dir_valid;
} NDSNativeFighterShadeKey;

typedef struct NDSNativeFighterShadeCensusEntry
{
    NDSNativeFighterShadeKey key;
    u32 hash;
    u32 valid;
} NDSNativeFighterShadeCensusEntry;

static NDSNativeFighterShadeCensusEntry sNdsNativeFighterShadeCensus[2][
    NDS_NATIVE_FIGHTER_HIERARCHY_EPOCH_COUNT];
static u32 sNdsNativeFighterShadeProducerGeneration[
    sizeof(sNdsNativeFighterDenseVertices) /
        sizeof(sNdsNativeFighterDenseVertices[0])];
static u8 sNdsNativeFighterShadeProducerTag[
    sizeof(sNdsNativeFighterDenseVertices) /
        sizeof(sNdsNativeFighterDenseVertices[0])];

static u32 ndsRendererM2ShadeHash(
    const NDSNativeFighterShadeKey *key)
{
    const u32 *word = (const u32 *)key;
    u32 hash = 2166136261u;
    u32 i;

    for (i = 0u; i < sizeof(*key) / sizeof(*word); i++)
    {
        u32 value = word[i];
        u32 byte;

        for (byte = 0u; byte < sizeof(value); byte++)
        {
            hash ^= (value >> (byte * 8u)) & 0xffu;
            hash *= 16777619u;
        }
    }
    return (hash != 0u) ? hash : 1u;
}

static s32 ndsRendererM2ShadeOutputsResident(
    u32 slot,
    u32 epoch_index,
    u32 owner_generation,
    const NDSNativeEpoch *epoch)
{
    u8 producer_tag = (u8)((slot << 7) | (epoch_index + 1u));
    u32 action_offset;

    for (action_offset = 0u;
         action_offset < epoch->action_count;
         action_offset++)
    {
        u32 action_index = epoch->first_action + action_offset;
        u32 span = sNdsNativeFighterActionDenseSpans[action_index];
        u32 dense_first = span & NDS_NATIVE_DENSE_ID_MASK;
        u32 dense_count = span >> NDS_NATIVE_DENSE_SPAN_COUNT_SHIFT;
        u32 dense_offset;

        for (dense_offset = 0u;
             dense_offset < dense_count;
             dense_offset++)
        {
            u32 dense_id = dense_first + dense_offset;

            if ((sNdsNativeFighterShadeProducerGeneration[dense_id] !=
                 owner_generation) ||
                (sNdsNativeFighterShadeProducerTag[dense_id] !=
                 producer_tag))
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static void __attribute__((noinline)) ndsRendererM2ShadeCensusEpoch(
    u32 slot,
    u32 owner_generation,
    u32 epoch_index,
    const NDSNativeEpoch *epoch,
    const NDSRendererHardwareLightDirection *prepared_direction,
    u32 prepared_direction_valid,
    const NDSRendererStats *stats)
{
    const u32 epoch_policy =
        sNdsNativeFighterEpochDirectPolicy[epoch_index];
    const NDSNativeDirectPolicy *policy =
        &sNdsNativeFighterDirectPolicies[
            epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK];
    NDSNativeFighterShadeCensusEntry *entry =
        &sNdsNativeFighterShadeCensus[slot][epoch_index];
    NDSNativeFighterShadeKey key = {
        owner_generation,
        slot,
        epoch_index,
        epoch_policy,
        policy->combine_w0,
        policy->combine_w1,
        (u32)policy->vertex_flags | ((u32)policy->textured << 8),
        stats->geometry_mode,
        stats->prim_color,
        stats->light_color_1,
        stats->light_color_2,
        stats->light_color_mask | (stats->light_dir_mask << 16),
        (prepared_direction != NULL) ? prepared_direction->x : 0,
        (prepared_direction != NULL) ? prepared_direction->y : 0,
        (prepared_direction != NULL) ? prepared_direction->z : 0,
        prepared_direction_valid
    };
    u32 hash;

    sNdsRendererM2ShadeEpochCount++;
    sNdsRendererM2ShadeOwnerEpochCount[slot]++;
    if (epoch->action_count == 0u)
    {
        return;
    }
    hash = ndsRendererM2ShadeHash(&key);
    if ((entry->valid != 0u) && (entry->hash == hash))
    {
        if (memcmp(&entry->key, &key, sizeof(key)) == 0)
        {
            sNdsRendererM2ShadeKeyHitCount++;
            sNdsRendererM2ShadeOwnerKeyHitCount[slot]++;
            if (ndsRendererM2ShadeOutputsResident(
                    slot, epoch_index, owner_generation, epoch) != FALSE)
            {
                sNdsRendererM2ShadeResidentHitCount++;
                sNdsRendererM2ShadeOwnerResidentHitCount[slot]++;
            }
        }
        else
        {
            sNdsRendererM2ShadeHashCollisionCount++;
        }
    }
    entry->key = key;
    entry->hash = hash;
    entry->valid = TRUE;
}

static void __attribute__((noinline)) ndsRendererM2ShadeRecordProduced(
    u32 slot,
    u32 owner_generation,
    u32 epoch_index,
    const NDSNativeEpoch *epoch,
    u32 epoch_policy,
    const NDSRendererStats *stats)
{
    const NDSNativeDirectPolicy *policy =
        &sNdsNativeFighterDirectPolicies[
            epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK];
    u32 use_material =
        policy->vertex_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL;
    u32 use_lut =
        ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u) &&
        ((stats->light_color_mask &
          (NDS_RENDERER_LIGHT_COLOR_1_MASK |
           NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
         (NDS_RENDERER_LIGHT_COLOR_1_MASK |
          NDS_RENDERER_LIGHT_COLOR_2_MASK));
    u32 action_offset;

    for (action_offset = 0u;
         action_offset < epoch->action_count;
         action_offset++)
    {
        u32 action_index = epoch->first_action + action_offset;
        u32 span = sNdsNativeFighterActionDenseSpans[action_index];
        u32 dense_first = span & NDS_NATIVE_DENSE_ID_MASK;
        u32 dense_count = span >> NDS_NATIVE_DENSE_SPAN_COUNT_SHIFT;
        u32 dense_offset;

        sNdsRendererM2ShadeDenseVisitCount += dense_count;
        if (use_material != 0u)
        {
            sNdsRendererM2ShadeMaterialPackCount += dense_count;
        }
        for (dense_offset = 0u;
             dense_offset < dense_count;
             dense_offset++)
        {
            u32 dense_id = dense_first + dense_offset;

            if (sNdsNativeFighterDenseColorSource[dense_id] != dense_id)
            {
                sNdsRendererM2ShadeAliasCopyCount++;
            }
            else
            {
                sNdsRendererM2ShadeComputeCount++;
                if (use_lut != 0u)
                {
                    sNdsRendererM2ShadeLutComputeCount++;
                }
                else
                {
                    sNdsRendererM2ShadePreparedComputeCount++;
                }
            }
            sNdsNativeFighterShadeProducerGeneration[dense_id] =
                owner_generation;
            sNdsNativeFighterShadeProducerTag[dense_id] =
                (u8)((slot << 7) | (epoch_index + 1u));
        }
    }
}
#endif

typedef struct NDSNativeStagePreparedDense
{
    u16 packed_color;
    s16 s;
    s16 t;
    s16 near_inside;
} NDSNativeStagePreparedDense;

typedef struct NDSRendererProjectedClipVertex
{
    NDSRendererClipVertex20p12 clip;
    s32 s;
    s32 t;
    u16 packed_color;
} NDSRendererProjectedClipVertex;

typedef struct NDSNativeStagePreparedRun
{
    NDSRendererHardwareTextureCacheEntry *texture_entry;
    u32 texture_name;
    u32 texture_params;
    u32 poly_fmt;
    u16 texture_width;
    u16 texture_height;
    u8 texture_format;
    u8 textured;
    u8 alpha_test;
    u8 alpha_ref;
} NDSNativeStagePreparedRun;

typedef struct NDSNativeStageOwnerExecution
{
    NDSRendererTraversalState traversal;
    NDSRendererStats preflight_stats;
    NDSNativeStagePreparedRun runs[NDS_NATIVE_STAGE_RUN_COUNT];
    const NDSRendererMatrix20p12 *binding_composed;
#if NDS_TASK36_HW_COMPOSE
    const NDSRendererMatrix20p12 *projection;
    const NDSRendererMatrix20p12 *camera_modelview;
    const NDSRendererMatrix20p12 *binding_world;
    u64 rigid_binding_mask;
    u64 task36_seen_binding_mask;
    u32 task36_binding;
    u32 task36_coordinate_shift;
    u32 task36_local_pushed;
    u32 task36_segment_active;
#endif
    NDSRendererMatrix20p12 raw_composed;
    NDSRendererMatrix20p12 scaled_raw_modelview;
    NDSRendererStats *stats;
    u32 next_segment;
    u32 active;
} NDSNativeStageOwnerExecution;

typedef struct NDSNativeStageTopologySummary
{
    u32 raw_triangles;
    u32 projected_no_z_triangles;
    u32 projected_range_triangles;
    u32 cross_runs;
    u32 cross_triangles;
    u32 cross_foreign_corners;
} NDSNativeStageTopologySummary;

typedef struct NDSNativeStageValidationCache
{
    NDSNativeStageTopologySummary summary;
    u16 prepared_dense_offsets[NDS_NATIVE_STAGE_RUN_COUNT + 1u];
    u16 prepared_dense_indices[NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT];
    u32 generation;
    u32 stamp;
    u32 valid;
} NDSNativeStageValidationCache;

/* Stage segments straddle the fighter display links, so their accepted
 * preflight must survive the two complete fighter-owner submissions. */
static NDSNativeFighterOwnerExecution sNdsNativeFighterOwnerExecution;
static NDSNativeStageOwnerExecution sNdsNativeStageOwnerExecution;
static NDSNativeStagePreparedDense sNdsNativeStagePreparedDense[
    NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT];
static NDSNativeStageValidationCache sNdsNativeStageValidationCache;
#if NDS_TASK36_HW_COMPOSE == 2
#define NDS_TASK36_REPLAY_WORD_CAPACITY 4608u
#define NDS_TASK36_REPLAY_SEGMENT_MASK \
    ((1u << 0u) | (1u << 5u) | (1u << 7u))

typedef enum NDSRendererTask36ReplayState
{
    NDS_TASK36_REPLAY_UNSEEDED = 0,
    NDS_TASK36_REPLAY_CAPTURING,
    NDS_TASK36_REPLAY_READY,
    NDS_TASK36_REPLAY_DISABLED
} NDSRendererTask36ReplayState;

typedef struct NDSRendererTask36ReplayRun
{
    NDSNativeStagePreparedRun prepared;
    u16 word_offset;
    u16 word_count;
    u8 valid;
    u8 world_mult_count;
    u8 reserved[2];
} NDSRendererTask36ReplayRun;

typedef struct NDSRendererTask36ReplayOwner
{
    u32 words[NDS_TASK36_REPLAY_WORD_CAPACITY] __attribute__((aligned(32)));
    NDSRendererTask36ReplayRun runs[NDS_NATIVE_STAGE_RUN_COUNT];
    NDSRendererStats segment_stats[NDS_NATIVE_STAGE_SEGMENT_COUNT];
    NDSRendererConfig config;
    u64 segment_epoch_mask[NDS_NATIVE_STAGE_SEGMENT_COUNT];
    u32 topology_generation;
    u32 topology_stamp;
    u32 word_count;
    u32 command_word_index;
    u32 command_slot;
    u32 current_run;
    u32 captured_segment_mask;
    u32 capture_active;
    u32 capture_fault;
    u32 frame_capture;
    u32 frame_replay;
    NDSRendererTask36ReplayState state;
} NDSRendererTask36ReplayOwner;

static NDSRendererTask36ReplayOwner sNdsRendererTask36ReplayOwner;

static s32 ndsRendererTask36ReplaySegmentEligible(u32 segment_index)
{
    return ((segment_index < NDS_NATIVE_STAGE_SEGMENT_COUNT) &&
            ((NDS_TASK36_REPLAY_SEGMENT_MASK &
              (1u << segment_index)) != 0u)) ? TRUE : FALSE;
}

static void ndsRendererTask36ReplayReset(void)
{
    memset(&sNdsRendererTask36ReplayOwner, 0,
           sizeof(sNdsRendererTask36ReplayOwner));
    sNdsRendererTask36ReplayOwner.current_run = UINT_MAX;
    sNdsRendererTask36ReplayOwner.command_word_index = UINT_MAX;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36ReplayState = NDS_TASK36_REPLAY_UNSEEDED;
#endif
}

static s32 ndsRendererTask36ReplayTexturesValid(void)
{
    u32 run_index;

    for (run_index = 0u; run_index < NDS_NATIVE_STAGE_RUN_COUNT; run_index++)
    {
        const NDSRendererTask36ReplayRun *run =
            &sNdsRendererTask36ReplayOwner.runs[run_index];
        const NDSRendererHardwareTextureCacheEntry *entry =
            run->prepared.texture_entry;

        if (run->valid == FALSE)
        {
            continue;
        }
        if ((run->prepared.textured != FALSE) &&
            ((entry == NULL) || (entry->ready == FALSE) ||
             ((u32)entry->name != run->prepared.texture_name)))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void ndsRendererTask36ReplayBeginFrame(
    const NDSRendererNativeStageFrame *frame)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;

#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36ReplaySegmentCount = 0u;
    gNdsRendererTask36ReplayRunCount = 0u;
    gNdsRendererTask36ReplayWordCount = 0u;
#endif
    owner->frame_capture = FALSE;
    owner->frame_replay = FALSE;
    if ((owner->topology_generation != frame->topology_generation) ||
        (owner->topology_stamp != frame->topology_stamp))
    {
        ndsRendererTask36ReplayReset();
        owner->topology_generation = frame->topology_generation;
        owner->topology_stamp = frame->topology_stamp;
    }
    if (frame->rigid_binding_mask !=
        NDS_RENDERER_TASK36_RIGID_BINDING_MASK)
    {
        if (owner->state == NDS_TASK36_REPLAY_READY)
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererTask36ReplayFallbackCount++;
#endif
        }
        return;
    }
    if ((gNdsTaskmanArenaChosenSize != 0x150000u) ||
        (gNdsTaskmanArenaAllocFailCount != 0u))
    {
        if ((gNdsTaskmanArenaChosenSize != 0u) &&
            (owner->state != NDS_TASK36_REPLAY_DISABLED))
        {
            owner->state = NDS_TASK36_REPLAY_DISABLED;
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererTask36ReplayArenaRejectCount++;
            gNdsRendererTask36ReplayState = NDS_TASK36_REPLAY_DISABLED;
#endif
        }
        return;
    }
    if (owner->state == NDS_TASK36_REPLAY_UNSEEDED)
    {
        return;
    }
    if (owner->state != NDS_TASK36_REPLAY_READY)
    {
        return;
    }
    if (memcmp(&owner->config, frame->config, sizeof(owner->config)) != 0)
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask36ReplayMaterialRejectCount++;
        gNdsRendererTask36ReplayFallbackCount++;
#endif
        return;
    }
    if (ndsRendererTask36ReplayTexturesValid() == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask36ReplayFallbackCount++;
#endif
        return;
    }
    owner->frame_replay = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36ReplayFrameCount++;
#endif
}

static void ndsRendererTask36ReplayStartCapture(
    const NDSRendererNativeStageFrame *frame)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;

    if ((owner->state != NDS_TASK36_REPLAY_UNSEEDED) ||
        (frame->rigid_binding_mask !=
         NDS_RENDERER_TASK36_RIGID_BINDING_MASK) ||
        (gNdsTaskmanArenaChosenSize != 0x150000u) ||
        (gNdsTaskmanArenaAllocFailCount != 0u))
    {
        return;
    }
    owner->state = NDS_TASK36_REPLAY_CAPTURING;
    owner->frame_capture = TRUE;
    owner->word_count = 0u;
    owner->captured_segment_mask = 0u;
    owner->capture_fault = FALSE;
    owner->config = *frame->config;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36BakeAttemptCount++;
    gNdsRendererTask36ReplayState = NDS_TASK36_REPLAY_CAPTURING;
#endif
}

static void ndsRendererTask36ReplayCapturePreparedSegment(
    u32 segment_index,
    const NDSRendererStats *stats,
    u64 epoch_mask)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;

    if (((owner->frame_capture == FALSE) &&
         (owner->state != NDS_TASK36_REPLAY_UNSEEDED)) ||
        (ndsRendererTask36ReplaySegmentEligible(segment_index) == FALSE))
    {
        return;
    }
    owner->segment_stats[segment_index] = *stats;
    owner->segment_epoch_mask[segment_index] = epoch_mask;
}

static s32 ndsRendererTask36ReplayUsePreparedSegment(
    u32 segment_index,
    NDSRendererStats *stats,
    u64 *epoch_mask)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;

    if ((owner->frame_replay == FALSE) || (stats == NULL) ||
        (epoch_mask == NULL) ||
        (ndsRendererTask36ReplaySegmentEligible(segment_index) == FALSE))
    {
        return FALSE;
    }
    *stats = owner->segment_stats[segment_index];
    *epoch_mask |= owner->segment_epoch_mask[segment_index];
    return TRUE;
}

static void ndsRendererTask36ReplayCaptureBeginRun(u32 run_index)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;
    NDSRendererTask36ReplayRun *run;

    if ((owner->frame_capture == FALSE) ||
        (run_index >= NDS_NATIVE_STAGE_RUN_COUNT) ||
        (owner->capture_active != FALSE))
    {
        owner->capture_fault = TRUE;
        return;
    }
    run = &owner->runs[run_index];
    memset(run, 0, sizeof(*run));
    run->prepared = sNdsNativeStageOwnerExecution.runs[run_index];
    run->word_offset = (u16)owner->word_count;
    owner->current_run = run_index;
    owner->command_word_index = UINT_MAX;
    owner->command_slot = 4u;
    owner->capture_active = TRUE;
}

static void ndsRendererTask36ReplayCaptureEndRun(u32 run_index)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;
    NDSRendererTask36ReplayRun *run;
    u32 word_count;

    if ((owner->capture_active == FALSE) ||
        (owner->current_run != run_index) ||
        (run_index >= NDS_NATIVE_STAGE_RUN_COUNT))
    {
        owner->capture_fault = TRUE;
        return;
    }
    run = &owner->runs[run_index];
    word_count = owner->word_count - run->word_offset;
    if ((word_count == 0u) || (word_count > USHRT_MAX))
    {
        owner->capture_fault = TRUE;
    }
    else
    {
        run->word_count = (u16)word_count;
        run->valid = TRUE;
    }
    owner->capture_active = FALSE;
    owner->current_run = UINT_MAX;
    owner->command_word_index = UINT_MAX;
}

static s32 ndsRendererTask36ReplayOpcode(
    NDSRendererTask29GXClass command_class,
    u32 *opcode,
    u32 *parameter_count)
{
    switch (command_class)
    {
    case NDS_TASK29_GX_MATRIX_MODE:
        *opcode = REG2ID(MATRIX_CONTROL); *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_MATRIX_IDENTITY:
        *opcode = REG2ID(MATRIX_IDENTITY); *parameter_count = 0u; return TRUE;
    case NDS_TASK29_GX_MATRIX_LOAD4X4:
        *opcode = REG2ID(MATRIX_LOAD4x4); *parameter_count = 16u; return TRUE;
    case NDS_TASK29_GX_MATRIX_MULT4X4:
        *opcode = REG2ID(MATRIX_MULT4x4); *parameter_count = 16u; return TRUE;
    case NDS_TASK29_GX_MATRIX_PUSH:
        *opcode = REG2ID(MATRIX_PUSH); *parameter_count = 0u; return TRUE;
    case NDS_TASK29_GX_MATRIX_POP:
        *opcode = REG2ID(MATRIX_POP); *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_MATRIX_STORE:
        *opcode = REG2ID(MATRIX_STORE); *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_MATRIX_RESTORE:
        *opcode = REG2ID(MATRIX_RESTORE); *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_BEGIN:
        *opcode = FIFO_BEGIN; *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_COLOR:
        *opcode = FIFO_COLOR; *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_TEX_COORD:
        *opcode = FIFO_TEX_COORD; *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_VERTEX16:
        *opcode = FIFO_VERTEX16; *parameter_count = 2u; return TRUE;
    default:
        return FALSE;
    }
}

static void ndsRendererTask36ReplayRecord(
    NDSRendererTask29GXClass command_class,
    const u32 *words,
    u32 word_count)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;
    u32 opcode;
    u32 parameter_count;
    u32 i;

    if ((owner->capture_active == FALSE) ||
        (ndsRendererTask36ReplayOpcode(
             command_class, &opcode, &parameter_count) == FALSE))
    {
        return;
    }
    if (command_class == NDS_TASK29_GX_MATRIX_MULT4X4)
    {
        NDSRendererTask36ReplayRun *run =
            &owner->runs[owner->current_run];

        if (run->world_mult_count == UCHAR_MAX)
        {
            owner->capture_fault = TRUE;
            return;
        }
        run->world_mult_count++;
    }
    if ((parameter_count > word_count) ||
        ((parameter_count != 0u) && (words == NULL)))
    {
        owner->capture_fault = TRUE;
        return;
    }
    if (owner->command_slot >= 4u)
    {
        if (owner->word_count >= NDS_TASK36_REPLAY_WORD_CAPACITY)
        {
            owner->capture_fault = TRUE;
            return;
        }
        owner->command_word_index = owner->word_count++;
        owner->words[owner->command_word_index] = 0u;
        owner->command_slot = 0u;
    }
    if (owner->word_count + parameter_count >
        NDS_TASK36_REPLAY_WORD_CAPACITY)
    {
        owner->capture_fault = TRUE;
        return;
    }
    owner->words[owner->command_word_index] |=
        opcode << (owner->command_slot * 8u);
    owner->command_slot++;
    for (i = 0u; i < parameter_count; i++)
    {
        owner->words[owner->word_count++] = words[i];
    }
}

static void ndsRendererTask36ReplayFinishFrame(void)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;
    u32 segment_index;
    u32 valid = TRUE;

    if (owner->frame_capture == FALSE)
    {
        return;
    }
    if ((owner->capture_active != FALSE) || (owner->capture_fault != FALSE) ||
        (owner->captured_segment_mask != NDS_TASK36_REPLAY_SEGMENT_MASK) ||
        (owner->word_count == 0u) ||
        (owner->word_count > NDS_TASK36_REPLAY_WORD_CAPACITY))
    {
        valid = FALSE;
    }
    for (segment_index = 0u;
         (segment_index < NDS_NATIVE_STAGE_SEGMENT_COUNT) && valid;
         segment_index++)
    {
        const NDSNativeStageSegment *segment;
        u32 run_offset;

        if (ndsRendererTask36ReplaySegmentEligible(segment_index) == FALSE)
        {
            continue;
        }
        segment = &sNdsNativeStageSegments[segment_index];
        for (run_offset = 0u; run_offset < segment->run_count; run_offset++)
        {
            u32 run_index = (u32)segment->first_run + run_offset;

            if (owner->runs[run_index].valid == FALSE)
            {
                valid = FALSE;
                break;
            }
        }
    }
    owner->frame_capture = FALSE;
    if (valid == FALSE)
    {
        owner->state = NDS_TASK36_REPLAY_DISABLED;
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask36BakeFailureCount++;
        gNdsRendererTask36ReplayState = NDS_TASK36_REPLAY_DISABLED;
#endif
        return;
    }
    DC_FlushRange(owner->words, owner->word_count * sizeof(owner->words[0]));
    owner->state = NDS_TASK36_REPLAY_READY;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36BakeSuccessCount++;
    gNdsRendererTask36ReplayCaptureWordCount = owner->word_count;
    gNdsRendererTask36ReplayState = NDS_TASK36_REPLAY_READY;
#endif
}
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
static u32 sNdsNativeStageTopologyFaultInjected;
#endif
#endif
#endif

#if NDS_RENDERER_SCREEN_SPACE_CENSUS
volatile NDSRendererScreenSpaceCensusRow
    gNdsRendererScreenSpaceCensus[NDS_RENDERER_PROFILE_OWNER_COUNT]
                                   [NDS_RENDERER_SCREEN_SPACE_CENSUS_PART_COUNT];
volatile u64 gNdsRendererScreenSpaceStageOwnerTicks[
    NDS_RENDERER_SCREEN_SPACE_CENSUS_STAGE_OWNER_COUNT];
volatile u32 gNdsRendererScreenSpaceCensusArmed;
volatile u32 gNdsRendererScreenSpaceCensusResetRequested;
volatile u32 gNdsRendererScreenSpaceCensusFrameCount;
volatile u32 gNdsRendererScreenSpaceCensusOverflowCount;
#endif

#if NDS_RENDER_ECONOMY
volatile u32 gNdsRendererEconomyConfiguredOwnerMask =
    (u32)NDS_RENDER_ECONOMY_OWNER_MASK;
volatile u32 gNdsRendererEconomyActiveOwnerMask;
volatile u32 gNdsRendererEconomyAppliedOwnerMask;
volatile u32 gNdsRendererEconomySkippedRunCount;
volatile u32 gNdsRendererEconomySkippedTriangleCount;
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
typedef struct NDSRendererHardwarePendingPosTest
{
    NDSRendererMatrix20p12 matrix;
    NDSRendererInputVertex input;
    NDSRendererClipVertex20p12 clip;
    u32 generation;
    u32 matrix_word;
} NDSRendererHardwarePendingPosTest;

static NDSRendererHardwarePendingPosTest
    sNdsRendererHardwarePendingPosTests[NDS_RENDERER_HW_POS_TEST_MAX];
static u32 sNdsRendererHardwarePendingPosTestCount;
static u32 sNdsRendererHardwarePendingPosTestLastGeneration;
#endif

#if NDS_RENDERER_HW_TRIANGLES
static void ndsRendererHardwarePrepareLitDirection(
    const NDSRendererStats *stats,
    const NDSRendererMatrix20p12 *modelview,
    NDSRendererHardwareLightDirection *out);
static u32 ndsRendererHardwareLitShadeColorPrepared(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction);
#if NDS_RENDERER_PROFILE_LEVEL < 2
static const u32 *ndsRendererHardwareFindLightShadeLut(
    u32 diffuse, u32 ambient);
static const u32 *ndsRendererHardwareGetLightShadeLut(
    u32 diffuse, u32 ambient);
#endif
static u32 ndsRendererHardwareLitShadeColor(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererMatrix20p12 *modelview);
#endif

static s32 ndsRendererClampS64ToS32(s64 value)
{
    if (value > (s64)INT_MAX)
    {
        return INT_MAX;
    }
    if (value < (s64)INT_MIN)
    {
        return INT_MIN;
    }
    return (s32)value;
}

static s32 ndsRendererRoundShiftS32(s32 value, u32 shift)
{
    s64 wide;
    s64 bias;

    if (shift == 0)
    {
        return value;
    }

    wide = value;
    bias = (s64)(1u << (shift - 1u));
    if (wide < 0)
    {
        return (s32)(-(((-wide) + bias) >> shift));
    }
    return (s32)((wide + bias) >> shift);
}

static s64 ndsRendererRoundShiftS64(s64 value, u32 shift)
{
    s64 bias;

    if (shift == 0)
    {
        return value;
    }

    bias = (s64)(1u << (shift - 1u));
    if (value < 0)
    {
        return -(((-value) + bias) >> shift);
    }
    return (value + bias) >> shift;
}

s32 ndsRendererMtxCellS16p16(const Mtx *mtx, u32 row, u32 col)
{
    const u32 *ai;
    const u32 *af;
    u32 pair;
    u32 hi;
    u32 lo;

    if ((mtx == NULL) || (row >= 4u) || (col >= 4u))
    {
        return 0;
    }

    ai = (const u32 *)&mtx->m[0][0];
    af = (const u32 *)&mtx->m[2][0];
    pair = (row * 2u) + (col / 2u);
    hi = ai[pair];
    lo = af[pair];

    if ((col & 1u) == 0)
    {
        return (s32)((hi & 0xffff0000u) | ((lo >> 16) & 0xffffu));
    }
    return (s32)(((hi << 16) & 0xffff0000u) | (lo & 0xffffu));
}

static s32 ndsRendererMtxCell20p12ToS16p16(s32 value)
{
    return ndsRendererClampS64ToS32(
        (s64)value << (NDS_RENDERER_N64_MTX_FRAC_BITS -
                       NDS_RENDERER_DS_MTX_FRAC_BITS));
}

static void ndsRendererMtxStoreCellS16p16(Mtx *mtx, u32 row, u32 col,
                                          s32 value)
{
    u32 *ai;
    u32 *af;
    u32 pair;
    u32 ui;

    if ((mtx == NULL) || (row >= 4u) || (col >= 4u))
    {
        return;
    }

    ai = (u32 *)&mtx->m[0][0];
    af = (u32 *)&mtx->m[2][0];
    pair = (row * 2u) + (col / 2u);
    ui = (u32)value;
    if ((col & 1u) == 0)
    {
        ai[pair] = (ai[pair] & 0x0000ffffu) | (ui & 0xffff0000u);
        af[pair] = (af[pair] & 0x0000ffffu) |
            ((ui << 16) & 0xffff0000u);
    }
    else
    {
        ai[pair] = (ai[pair] & 0xffff0000u) | ((ui >> 16) & 0xffffu);
        af[pair] = (af[pair] & 0xffff0000u) | (ui & 0xffffu);
    }
}

static void ndsRendererMtxStoreDS20p12ToN64(
    const NDSRendererMatrix20p12 *src, Mtx *dst)
{
    u32 row;
    u32 col;

    if (dst == NULL)
    {
        return;
    }

    memset(dst, 0, sizeof(*dst));
    if (src == NULL)
    {
        return;
    }

    for (row = 0; row < 4u; row++)
    {
        for (col = 0; col < 4u; col++)
        {
            ndsRendererMtxStoreCellS16p16(
                dst, row, col,
                ndsRendererMtxCell20p12ToS16p16(src->m[row][col]));
        }
    }
}

void ndsRendererMtxLoadN64ToDS20p12(const Mtx *src,
                                    NDSRendererMatrix20p12 *dst)
{
    u32 row;
    u32 col;
    const u32 shift =
        NDS_RENDERER_N64_MTX_FRAC_BITS - NDS_RENDERER_DS_MTX_FRAC_BITS;

    if (dst == NULL)
    {
        return;
    }

    memset(dst, 0, sizeof(*dst));
    if (src == NULL)
    {
        return;
    }

    for (row = 0; row < 4u; row++)
    {
        for (col = 0; col < 4u; col++)
        {
            dst->m[row][col] =
                ndsRendererRoundShiftS32(
                    ndsRendererMtxCellS16p16(src, row, col), shift);
        }
    }
}

void ndsRendererTransformVertex20p12(const NDSRendererMatrix20p12 *mtx,
                                     const NDSRendererInputVertex *vtx,
                                     NDSRendererClipVertex20p12 *out)
{
    s64 x;
    s64 y;
    s64 z;

    if ((mtx == NULL) || (vtx == NULL) || (out == NULL))
    {
        return;
    }

    x = vtx->x;
    y = vtx->y;
    z = vtx->z;

    out->x = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][0] * x + (s64)mtx->m[1][0] * y +
        (s64)mtx->m[2][0] * z + mtx->m[3][0]);
    out->y = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][1] * x + (s64)mtx->m[1][1] * y +
        (s64)mtx->m[2][1] * z + mtx->m[3][1]);
    out->z = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][2] * x + (s64)mtx->m[1][2] * y +
        (s64)mtx->m[2][2] * z + mtx->m[3][2]);
    out->w = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][3] * x + (s64)mtx->m[1][3] * y +
        (s64)mtx->m[2][3] * z + mtx->m[3][3]);
}

void ndsRendererMtxMul20p12(const NDSRendererMatrix20p12 *lhs,
                            const NDSRendererMatrix20p12 *rhs,
                            NDSRendererMatrix20p12 *out)
{
    NDSRendererMatrix20p12 temp;
    u32 row;
    u32 col;
    u32 k;

    if ((lhs == NULL) || (rhs == NULL) || (out == NULL))
    {
        return;
    }

    for (row = 0; row < 4u; row++)
    {
        for (col = 0; col < 4u; col++)
        {
            s64 sum = 0;

            for (k = 0; k < 4u; k++)
            {
                sum += (s64)lhs->m[row][k] * rhs->m[k][col];
            }
            temp.m[row][col] = ndsRendererClampS64ToS32(
                ndsRendererRoundShiftS64(sum, NDS_RENDERER_DS_MTX_FRAC_BITS));
        }
    }

    *out = temp;
}

void ndsRendererMtxMulAffine20p12(const NDSRendererMatrix20p12 *lhs,
                                  const NDSRendererMatrix20p12 *rhs,
                                  NDSRendererMatrix20p12 *out)
{
    NDSRendererMatrix20p12 temp;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    NDSRendererMatrix20p12 oracle;
    u32 mismatch = FALSE;
#endif
    u32 row;
    u32 col;

    if ((lhs == NULL) || (rhs == NULL) || (out == NULL))
    {
        return;
    }
    if ((lhs->m[0][3] != 0) || (lhs->m[1][3] != 0) ||
        (lhs->m[2][3] != 0) ||
        (lhs->m[3][3] != (1 << NDS_RENDERER_DS_MTX_FRAC_BITS)) ||
        (rhs->m[0][3] != 0) || (rhs->m[1][3] != 0) ||
        (rhs->m[2][3] != 0) ||
        (rhs->m[3][3] != (1 << NDS_RENDERER_DS_MTX_FRAC_BITS)))
    {
        ndsRendererMtxMul20p12(lhs, rhs, out);
        return;
    }

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            s64 sum = (s64)lhs->m[row][0] * rhs->m[0][col] +
                (s64)lhs->m[row][1] * rhs->m[1][col] +
                (s64)lhs->m[row][2] * rhs->m[2][col];

            temp.m[row][col] = ndsRendererClampS64ToS32(
                ndsRendererRoundShiftS64(
                    sum, NDS_RENDERER_DS_MTX_FRAC_BITS));
        }
        temp.m[row][3] = 0;
    }
    for (col = 0u; col < 3u; col++)
    {
        s64 sum = (s64)lhs->m[3][0] * rhs->m[0][col] +
            (s64)lhs->m[3][1] * rhs->m[1][col] +
            (s64)lhs->m[3][2] * rhs->m[2][col] +
            (s64)lhs->m[3][3] * rhs->m[3][col];

        temp.m[3][col] = ndsRendererClampS64ToS32(
            ndsRendererRoundShiftS64(sum,
                                     NDS_RENDERER_DS_MTX_FRAC_BITS));
    }
    temp.m[3][3] = 1 << NDS_RENDERER_DS_MTX_FRAC_BITS;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererMtxMul20p12(lhs, rhs, &oracle);
    gNdsRendererProfileAffineMatrixSamples++;
    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            s64 delta = (s64)temp.m[row][col] - oracle.m[row][col];
            u32 magnitude;

            if (delta < 0)
            {
                delta = -delta;
            }
            magnitude = (delta > (s64)UINT_MAX) ? UINT_MAX : (u32)delta;
            if (magnitude != 0u)
            {
                mismatch = TRUE;
                if (magnitude > gNdsRendererProfileAffineMatrixMaxDelta)
                {
                    gNdsRendererProfileAffineMatrixMaxDelta = magnitude;
                }
            }
        }
    }
    if (mismatch != FALSE)
    {
        gNdsRendererProfileAffineMatrixMismatches++;
    }
#endif
    *out = temp;
}

static void ndsRendererMtxIdentity20p12(NDSRendererMatrix20p12 *out)
{
    u32 i;

    if (out == NULL)
    {
        return;
    }

    memset(out, 0, sizeof(*out));
    for (i = 0; i < 4u; i++)
    {
        out->m[i][i] = 1 << NDS_RENDERER_DS_MTX_FRAC_BITS;
    }
}

#if NDS_RENDERER_HW_TRIANGLES
static u32 ndsRendererNextMatrixGeneration(void)
{
    sNdsRendererMatrixGenerationSerial++;
    if (sNdsRendererMatrixGenerationSerial == 0u)
    {
        sNdsRendererMatrixGenerationSerial = 1u;
        sNdsRendererHardwareMatrixLoaded = FALSE;
        sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
        sNdsRendererHardwareMatrixGeneration = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareMatrixSignature = 0u;
#endif
    }
    return sNdsRendererMatrixGenerationSerial;
}
#endif

static u32 ndsRendererReadU32(const void *ptr)
{
    const u8 *bytes = ptr;

    return (u32)bytes[0] |
           ((u32)bytes[1] << 8) |
           ((u32)bytes[2] << 16) |
           ((u32)bytes[3] << 24);
}

typedef u32 NDSRendererAliasedU32 __attribute__((__may_alias__));

static void ndsRendererDecodeInputVertex(NDSRendererInputVertex *dst,
                                         const void *src)
{
    u32 xy;
    u32 zf;
    u32 st;
    u32 rgba;

    if ((dst == NULL) || (src == NULL))
    {
        return;
    }

    /* DS is little-endian, so an aligned may-alias word load is exactly the
     * bytewise payload decode below. Retain that fallback for arbitrary DLs. */
    if ((((uintptr_t)src) & 3u) == 0u)
    {
        const NDSRendererAliasedU32 *words =
            (const NDSRendererAliasedU32 *)src;

        xy = words[0];
        zf = words[1];
        st = words[2];
        rgba = words[3];
    }
    else
    {
        xy = ndsRendererReadU32(src);
        zf = ndsRendererReadU32((const u8 *)src + 4);
        st = ndsRendererReadU32((const u8 *)src + 8);
        rgba = ndsRendererReadU32((const u8 *)src + 12);
    }
    dst->x = (s16)(xy >> 16);
    dst->y = (s16)(xy & 0xffffu);
    dst->z = (s16)(zf >> 16);
    dst->s = (s16)(st >> 16);
    dst->t = (s16)(st & 0xffffu);
    dst->r = (u8)(rgba >> 24);
    dst->g = (u8)(rgba >> 16);
    dst->b = (u8)(rgba >> 8);
    dst->a = (u8)rgba;
    if (dst->a == 0u)
    {
        dst->a = 0xffu;
    }
}

static s32 ndsRendererValidateCommand(const Gfx *dl,
                                       const NDSRendererConfig *config)
{
    uintptr_t addr = (uintptr_t)dl;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileValidatedCommandCount++;
#endif

    if ((dl == NULL) || ((addr & 0x3u) != 0))
    {
        return FALSE;
    }
    if (config->validate_range == NULL)
    {
        return TRUE;
    }
    return config->validate_range(dl, sizeof(*dl), config->user);
}

static const void *ndsRendererResolveDataPointer(
    const NDSRendererConfig *config, const void *ptr, size_t bytes)
{
    uintptr_t addr = (uintptr_t)ptr;

    if ((ptr == NULL) || ((addr & 0x3u) != 0))
    {
        return NULL;
    }
    if ((config != NULL) && (config->resolve_data != NULL))
    {
        return config->resolve_data(ptr, bytes, config->user);
    }
    if ((config != NULL) && (config->validate_range != NULL) &&
        (config->validate_range((const Gfx *)ptr, bytes, config->user) ==
         FALSE))
    {
        return NULL;
    }
    return ptr;
}

static void ndsRendererRecordUnsupported(NDSRendererStats *stats, u32 op)
{
    if (stats->unsupported_opcode == 0)
    {
        stats->unsupported_opcode = op;
    }
    stats->unsupported_command_count++;
}

static void ndsRendererRecordOtherMode(NDSRendererStats *stats,
                                       u32 op, u32 w0, u32 w1)
{
    u32 bits;
    u32 pos;
    u32 shift;
    u32 mask;

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    stats->othermode_command_count++;
    stats->ignored_state_command_count++;
    if (stats->first_othermode_opcode == 0)
    {
        stats->first_othermode_opcode = op;
        stats->first_othermode_w0 = w0;
        stats->first_othermode_w1 = w1;
    }

    if (op == NDS_RENDERER_OP_RDPSETOTHERMODE)
    {
        stats->othermode_h = w0 & 0x00ffffffu;
        stats->othermode_l = w1;
        return;
    }
    if ((op != NDS_RENDERER_OP_SETOTHERMODE_H) &&
        (op != NDS_RENDERER_OP_SETOTHERMODE_L))
    {
        return;
    }

    bits = (w0 & 0xffu) + 1u;
    pos = (w0 >> 8) & 0xffu;
    if ((bits > 32u) || (pos >= 32u) || ((bits + pos) > 32u))
    {
        return;
    }
    shift = 32u - pos - bits;
    mask = (bits >= 32u) ? 0xffffffffu : (((1u << bits) - 1u) << shift);
    if (op == NDS_RENDERER_OP_SETOTHERMODE_H)
    {
        stats->othermode_h = (stats->othermode_h & ~mask) | (w1 & mask);
    }
    else
    {
        stats->othermode_l = (stats->othermode_l & ~mask) | (w1 & mask);
    }
}

static void ndsRendererRecordPrimDepth(NDSRendererStats *stats, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->prim_depth = (w1 >> 16) & 0xffffu;
    stats->prim_depth_delta = w1 & 0xffffu;
    stats->prim_depth_command_count++;
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
}

static void ndsRendererRecordCull(NDSRendererStats *stats, u32 w0, u32 w1)
{
    stats->cull_command_count++;
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
    if ((stats->first_cull_w0 == 0) && (stats->first_cull_w1 == 0))
    {
        stats->first_cull_w0 = w0;
        stats->first_cull_w1 = w1;
    }
}

static void ndsRendererSyncTextureTile(NDSRendererStats *stats);

static void ndsRendererRecordTextureState(NDSRendererStats *stats,
                                          u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_TEXTURE;
    stats->texture_command_count++;
    stats->texture_xparam = (w0 >> 16) & 0xFFu;
    stats->texture_level = (w0 >> 11) & 0x7u;
    stats->texture_tile = (w0 >> 8) & 0x7u;
    stats->texture_on = (w0 >> 1) & 0x7Fu;
    stats->texture_scale_s = (w1 >> 16) & 0xFFFFu;
    stats->texture_scale_t = w1 & 0xFFFFu;
    stats->texture_state_flags = NDS_RENDERER_TEXTURE_STATE_SEEN;
    if (stats->texture_on != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_ON;
    }
    if (stats->texture_scale_s != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_SCALE_S;
    }
    if (stats->texture_scale_t != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_SCALE_T;
    }
    ndsRendererSyncTextureTile(stats);
}

static u32 ndsRendererTileFlags(u32 cms, u32 cmt, u32 masks, u32 maskt)
{
    u32 flags = 0u;

    if ((cms & NDS_RENDERER_TX_CLAMP) != 0) { flags |= NDS_RENDERER_TILE_S_CLAMP; }
    if ((cms & NDS_RENDERER_TX_MIRROR) != 0) { flags |= NDS_RENDERER_TILE_S_MIRROR; }
    if (masks != 0u) { flags |= NDS_RENDERER_TILE_S_MASKED; }
    if ((cmt & NDS_RENDERER_TX_CLAMP) != 0) { flags |= NDS_RENDERER_TILE_T_CLAMP; }
    if ((cmt & NDS_RENDERER_TX_MIRROR) != 0) { flags |= NDS_RENDERER_TILE_T_MIRROR; }
    if (maskt != 0u) { flags |= NDS_RENDERER_TILE_T_MASKED; }
    return flags;
}

static u32 ndsRendererActiveTextureTile(const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_SEEN) != 0u))
    {
        return stats->texture_tile & 0x7u;
    }
    return NDS_RENDERER_RENDER_TILE;
}

static void ndsRendererSyncTextureTile(NDSRendererStats *stats)
{
    u32 tile_index;
    const NDSRendererTileState *tile;
    u32 flags = 0u;

    if (stats == NULL)
    {
        return;
    }

    tile_index = ndsRendererActiveTextureTile(stats);
    tile = &stats->texture_tiles[tile_index];

    stats->texture_render_tile = tile_index;
    stats->texture_render_tile_format = tile->format;
    stats->texture_render_tile_size = tile->size;
    stats->texture_render_tile_line = tile->line;
    stats->texture_render_tile_tmem = tile->tmem;
    stats->texture_render_tile_palette = tile->palette;
    stats->texture_render_tile_cms = tile->cms;
    stats->texture_render_tile_cmt = tile->cmt;
    stats->texture_render_tile_masks = tile->masks;
    stats->texture_render_tile_maskt = tile->maskt;
    stats->texture_render_tile_shifts = tile->shifts;
    stats->texture_render_tile_shiftt = tile->shiftt;
    stats->texture_tile_size_tile = tile_index;
    stats->texture_tile_size_uls = tile->uls;
    stats->texture_tile_size_ult = tile->ult;
    stats->texture_tile_size_lrs = tile->lrs;
    stats->texture_tile_size_lrt = tile->lrt;
    stats->texture_tile_width = tile->width;
    stats->texture_tile_height = tile->height;
    if (tile->set_seen != 0u)
    {
        flags |= NDS_RENDERER_TILE_RENDER_SEEN | tile->flags;
    }
    if (stats->texture_tiles[NDS_RENDERER_LOAD_TILE].set_seen != 0u)
    {
        flags |= NDS_RENDERER_TILE_LOAD_SEEN;
    }
    stats->texture_render_tile_flags = flags;
}

static void ndsRendererRecordSetTile(NDSRendererStats *stats,
                                     u32 w0, u32 w1)
{
    u32 tile;
    u32 fmt;
    u32 siz;
    u32 line;
    u32 tmem;
    u32 palette;
    u32 cmt;
    u32 maskt;
    u32 shiftt;
    u32 cms;
    u32 masks;
    u32 shifts;
    NDSRendererTileState *tile_state;

    if (stats == NULL)
    {
        return;
    }

    tile = (w1 >> 24) & 0x7u;
    fmt = (w0 >> 21) & 0x7u;
    siz = (w0 >> 19) & 0x3u;
    line = (w0 >> 9) & 0x1FFu;
    tmem = w0 & 0x1FFu;
    palette = (w1 >> 20) & 0xFu;
    cmt = (w1 >> 18) & 0x3u;
    maskt = (w1 >> 14) & 0xFu;
    shiftt = (w1 >> 10) & 0xFu;
    cms = (w1 >> 8) & 0x3u;
    masks = (w1 >> 4) & 0xFu;
    shifts = w1 & 0xFu;

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTILE;
    stats->texture_set_tile_count++;

    tile_state = &stats->texture_tiles[tile];
    tile_state->set_seen = 1u;
    tile_state->format = fmt;
    tile_state->size = siz;
    tile_state->line = line;
    tile_state->tmem = tmem;
    tile_state->palette = palette;
    tile_state->cms = cms;
    tile_state->cmt = cmt;
    tile_state->masks = masks;
    tile_state->maskt = maskt;
    tile_state->shifts = shifts;
    tile_state->shiftt = shiftt;
    tile_state->flags = ndsRendererTileFlags(cms, cmt, masks, maskt);

    if (tile == NDS_RENDERER_LOAD_TILE)
    {
        stats->texture_load_tile = tile;
    }

    ndsRendererSyncTextureTile(stats);
}

static void ndsRendererCaptureTextureLoad(NDSRendererStats *stats)
{
    NDSRendererTextureLoadState *load;
    u32 tile;

    if (stats == NULL)
    {
        return;
    }

    tile = stats->texture_load_tile & 0x7u;
    stats->texture_load_sequence++;
    load = &stats->texture_loads[
        (stats->texture_load_sequence - 1u) %
        NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT];
    memset(load, 0, sizeof(*load));
    load->sequence = stats->texture_load_sequence;
    load->image = stats->texture_image;
    load->image_format = stats->texture_format;
    load->image_size = stats->texture_size;
    load->image_width = stats->texture_image_width;
    load->load_kind = stats->texture_load_kind;
    load->load_tile = tile;
    load->load_uls = stats->texture_load_block_uls;
    load->load_ult = stats->texture_load_block_ult;
    load->load_lrs = stats->texture_load_block_lrs;
    load->load_dxt = stats->texture_load_block_dxt;
    /* The compact per-TMEM record deliberately stores only bounded texture
     * loads. A large LOADTILE rectangle must not wrap into a plausible small
     * source span and later pass the residency checks. */
    load->load_texels = (stats->texture_load_texels <= 0xffffu) ?
        (u16)stats->texture_load_texels : 0u;
    load->load_tmem = stats->texture_tiles[tile].tmem;
    load->valid = ((load->image != 0u) &&
                   (load->load_texels != 0u) &&
                   (stats->texture_tiles[tile].set_seen != 0u)) ? TRUE : FALSE;
}

static void ndsRendererRecordLoadBlock(NDSRendererStats *stats,
                                       u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_LOADBLOCK;
    stats->texture_load_kind = NDS_RENDERER_TEXTURE_LOADBLOCK;
    stats->texture_load_tile = (w1 >> 24) & 0x7u;
    stats->texture_load_block_uls = (w0 >> 12) & 0x0FFFu;
    stats->texture_load_block_ult = w0 & 0x0FFFu;
    stats->texture_load_block_lrs = (w1 >> 12) & 0x0FFFu;
    stats->texture_load_block_dxt = w1 & 0x0FFFu;
    stats->texture_load_texels = stats->texture_load_block_lrs + 1u;
    ndsRendererCaptureTextureLoad(stats);
}

static void ndsRendererRecordLoadTile(NDSRendererStats *stats,
                                      u32 w0, u32 w1)
{
    u32 uls;
    u32 ult;
    u32 lrs;
    u32 lrt;
    u32 width;
    u32 height;

    if (stats == NULL)
    {
        return;
    }

    uls = (w0 >> 12) & 0x0FFFu;
    ult = w0 & 0x0FFFu;
    lrs = (w1 >> 12) & 0x0FFFu;
    lrt = w1 & 0x0FFFu;

    stats->texture_mask |= NDS_RENDERER_TEXTURE_LOADTILE;
    stats->texture_load_kind = NDS_RENDERER_TEXTURE_LOADTILE;
    stats->texture_load_tile = (w1 >> 24) & 0x7u;
    stats->texture_load_block_uls = uls;
    stats->texture_load_block_ult = ult;
    stats->texture_load_block_lrs = lrs;
    stats->texture_load_block_dxt = lrt;
    stats->texture_load_texels = 0u;
    if ((lrs >= uls) && (lrt >= ult))
    {
        width = ((lrs - uls) >> 2) + 1u;
        height = ((lrt - ult) >> 2) + 1u;
        stats->texture_load_texels = width * height;
    }
    ndsRendererCaptureTextureLoad(stats);
}

static void ndsRendererRecordSetTileSize(NDSRendererStats *stats,
                                         u32 w0, u32 w1)
{
    u32 tile_index;
    u32 uls;
    u32 ult;
    u32 lrs;
    u32 lrt;
    NDSRendererTileState *tile;

    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTILESIZE;

    uls = (w0 >> 12) & 0x0FFFu;
    ult = w0 & 0x0FFFu;
    lrs = (w1 >> 12) & 0x0FFFu;
    lrt = w1 & 0x0FFFu;

    tile_index = (w1 >> 24) & 0x7u;
    tile = &stats->texture_tiles[tile_index];
    tile->size_seen = 1u;
    tile->uls = uls;
    tile->ult = ult;
    tile->lrs = lrs;
    tile->lrt = lrt;
    tile->width = 0u;
    tile->height = 0u;
    if (lrs >= uls)
    {
        tile->width = ((lrs - uls) >> 2) + 1u;
    }
    if (lrt >= ult)
    {
        tile->height = ((lrt - ult) >> 2) + 1u;
    }
    ndsRendererSyncTextureTile(stats);
}

static void ndsRendererRecordSetImage(NDSRendererStats *stats,
                                      u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTIMG;
    stats->texture_format = (w0 >> 21) & 0x7u;
    stats->texture_size = (w0 >> 19) & 0x3u;
    stats->texture_image_width = (w0 & 0x0FFFu) + 1u;
    stats->texture_image = w1;
}

static void ndsRendererRecordLoadTlut(NDSRendererStats *stats, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_command_count++;
    stats->texture_tlut_tile = (w1 >> 24) & 0x7u;
    stats->texture_tlut_count = ((w1 >> 14) & 0x3ffu) + 1u;
    if (stats->texture_image != 0u)
    {
        stats->texture_tlut_image = stats->texture_image;
    }
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererProfileCombineMode(u32 w0, u32 w1)
{
    gNdsRendererProfileCombineModeCount++;
    if (((gNdsRendererProfileCombineMode0W0 == w0) &&
         (gNdsRendererProfileCombineMode0W1 == w1)) ||
        ((gNdsRendererProfileCombineMode1W0 == w0) &&
         (gNdsRendererProfileCombineMode1W1 == w1)) ||
        ((gNdsRendererProfileCombineMode2W0 == w0) &&
         (gNdsRendererProfileCombineMode2W1 == w1)) ||
        ((gNdsRendererProfileCombineMode3W0 == w0) &&
         (gNdsRendererProfileCombineMode3W1 == w1)))
    {
        return;
    }

    switch (gNdsRendererProfileCombineModeDistinctCount)
    {
    case 0:
        gNdsRendererProfileCombineMode0W0 = w0;
        gNdsRendererProfileCombineMode0W1 = w1;
        break;
    case 1:
        gNdsRendererProfileCombineMode1W0 = w0;
        gNdsRendererProfileCombineMode1W1 = w1;
        break;
    case 2:
        gNdsRendererProfileCombineMode2W0 = w0;
        gNdsRendererProfileCombineMode2W1 = w1;
        break;
    case 3:
        gNdsRendererProfileCombineMode3W0 = w0;
        gNdsRendererProfileCombineMode3W1 = w1;
        break;
    default:
        break;
    }
    gNdsRendererProfileCombineModeDistinctCount++;
}
#endif

static void ndsRendererRecordSetCombine(NDSRendererStats *stats,
                                        u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETCOMBINE;
    stats->texture_combine_count++;
    stats->texture_combine_w0 = w0;
    stats->texture_combine_w1 = w1;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileCombineMode(w0, w1);
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static u32 ndsRendererMatrixSnapshotSignature(
    const NDSRendererMatrix20p12 *matrix)
{
    u32 signature = 2166136261u;
    u32 row;
    u32 col;

    if (matrix == NULL)
    {
        return 0u;
    }
    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            signature ^= (u32)matrix->m[row][col];
            signature *= 16777619u;
        }
    }
    return signature;
}

static const NDSRendererMatrixSnapshot *ndsRendererGetMatrixSnapshot(
    const NDSRendererTraversalState *state, u32 snapshot_id)
{
    if ((state == NULL) || (state->matrix_snapshots == NULL) ||
        (snapshot_id == NDS_RENDERER_MATRIX_SNAPSHOT_INVALID) ||
        (snapshot_id > state->matrix_snapshot_count))
    {
        return NULL;
    }
    return &state->matrix_snapshots[snapshot_id - 1u];
}

static u32 ndsRendererAcquireCurrentMatrixSnapshot(
    NDSRendererTraversalState *state)
{
    NDSRendererMatrixSnapshot *snapshot;
    u32 signature;
    u32 i;

    if ((state == NULL) || (state->matrix_valid == 0u) ||
        (state->matrix_snapshots == NULL))
    {
        return NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    }
    if (state->current_matrix_snapshot !=
        NDS_RENDERER_MATRIX_SNAPSHOT_INVALID)
    {
        return state->current_matrix_snapshot;
    }

    signature = ndsRendererMatrixSnapshotSignature(&state->matrix);
    for (i = 0u; i < state->matrix_snapshot_count; i++)
    {
        snapshot = &state->matrix_snapshots[i];
        if ((snapshot->signature == signature) &&
            (memcmp(&snapshot->matrix, &state->matrix,
                    sizeof(snapshot->matrix)) == 0))
        {
            state->current_matrix_snapshot = i + 1u;
            ndsRendererProfileRecordMatrixSnapshotReuse();
            return state->current_matrix_snapshot;
        }
    }
    if (state->matrix_snapshot_count >=
        NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY)
    {
        ndsRendererProfileRecordMatrixSnapshotOverflow();
        return NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    }

    snapshot = &state->matrix_snapshots[state->matrix_snapshot_count++];
    snapshot->matrix = state->matrix;
    snapshot->generation = state->matrix_generation;
    snapshot->signature = signature;
    state->current_matrix_snapshot = state->matrix_snapshot_count;
    ndsRendererProfileRecordMatrixSnapshotCreate();
    return state->current_matrix_snapshot;
}

static s32 ndsRendererTransformCachedVertex(
    NDSRendererStats *stats, NDSRendererTraversalState *state, u32 index,
    const NDSRendererMatrix20p12 *matrix, u32 snapshot_id)
{
    NDSRendererClipVertex20p12 *out;
    u32 mask;

    if ((stats == NULL) || (state == NULL) || (matrix == NULL) ||
        (index >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    mask = 1u << index;
    if ((state->input_vertex_valid_mask & mask) == 0u)
    {
        return FALSE;
    }

    out = &state->vertices[index];
    ndsRendererTransformVertex20p12(matrix, &state->input_vertices[index], out);
    state->vertex_valid_mask |= mask;
    state->vertex_clip_snapshot[index] = (u8)snapshot_id;
    stats->matrix_transform_count++;
    stats->transformed_vertex_count++;
    if (stats->transformed_vertex_count == 1u)
    {
        stats->first_transformed_x = out->x;
        stats->first_transformed_y = out->y;
        stats->first_transformed_z = out->z;
        stats->first_transformed_w = out->w;
    }
    ndsRendererProfileRecordCPUTransform();
    return TRUE;
}

static s32 ndsRendererEnsureTransformedVertex(
    NDSRendererStats *stats, NDSRendererTraversalState *state, u32 index)
{
    const NDSRendererMatrixSnapshot *snapshot;
    u32 snapshot_id;
    u32 mask;

    if ((state == NULL) || (index >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    mask = 1u << index;
    snapshot_id = state->vertex_matrix_snapshot[index];
    if (((state->vertex_valid_mask & mask) != 0u) &&
        (state->vertex_clip_snapshot[index] == snapshot_id))
    {
        ndsRendererProfileRecordTransformCacheHit();
        return TRUE;
    }

    snapshot = ndsRendererGetMatrixSnapshot(state, snapshot_id);
    if (snapshot == NULL)
    {
        return FALSE;
    }
    return ndsRendererTransformCachedVertex(
        stats, state, index, &snapshot->matrix, snapshot_id);
}
#endif

static void ndsRendererComposeMatrix(NDSRendererTraversalState *state)
{
    NDSRendererMatrix20p12 identity;

    if (state == NULL)
    {
        return;
    }

    if ((state->projection_valid != 0u) &&
        (state->modelview_valid != 0u))
    {
        ndsRendererMtxMul20p12(&state->modelview,
                               &state->projection,
                               &state->matrix);
    }
    else if (state->modelview_valid != 0u)
    {
        state->matrix = state->modelview;
    }
    else if (state->projection_valid != 0u)
    {
        state->matrix = state->projection;
    }
    else
    {
        ndsRendererMtxIdentity20p12(&identity);
        state->matrix = identity;
    }
    state->matrix_valid =
        ((state->projection_valid != 0u) ||
         (state->modelview_valid != 0u)) ? TRUE : FALSE;
    state->matrix_word_valid = FALSE;
#if NDS_RENDERER_HW_TRIANGLES
    /* Cached RSP vertices retain the transform active when they were loaded. */
    state->current_transform_vertex_mask = 0u;
    state->current_matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    state->matrix_generation = ndsRendererNextMatrixGeneration();
    NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state);
#endif
}

static void ndsRendererInitMatrixWordRaw(NDSRendererTraversalState *state)
{
    NDSRendererMatrix20p12 identity;

    if (state == NULL)
    {
        return;
    }

    if (state->matrix_valid == 0u)
    {
        ndsRendererMtxIdentity20p12(&identity);
        ndsRendererMtxStoreDS20p12ToN64(&identity, &state->matrix_word_raw);
    }
    else
    {
        ndsRendererMtxStoreDS20p12ToN64(&state->matrix,
                                        &state->matrix_word_raw);
    }
    state->matrix_word_valid = TRUE;
}

static void ndsRendererInitTraversalState(NDSRendererTraversalState *state,
                                          const NDSRendererConfig *config,
                                          NDSRendererStats *stats,
                                          NDSRendererTraversalVertexStorage
                                              *vertex_storage,
                                          NDSRendererMatrixSnapshot *snapshots,
                                          u32 snapshot_count)
{
    if (state == NULL)
    {
        return;
    }

    /* Valid masks and stack depth own every scratch read. Initialize that
     * compact control plane instead of clearing matrix, stack, and derived
     * arrays that will be overwritten before their first valid use. */
    state->modelview_stack_depth = 0u;
    state->vertex_valid_mask = 0u;
    state->vertices = (vertex_storage != NULL) ?
        vertex_storage->vertices : NULL;
#if NDS_RENDERER_HW_TRIANGLES
    state->input_vertices = (vertex_storage != NULL) ?
        vertex_storage->input_vertices : NULL;
    state->vertex_colors = (vertex_storage != NULL) ?
        vertex_storage->vertex_colors : NULL;
    state->vertex_matrix_snapshot = (vertex_storage != NULL) ?
        vertex_storage->vertex_matrix_snapshot : NULL;
    state->vertex_clip_snapshot = (vertex_storage != NULL) ?
        vertex_storage->vertex_clip_snapshot : NULL;
    state->matrix_snapshots = snapshots;
    state->source_command_site = NULL;
    state->matrix_snapshot_count =
        (snapshot_count <= NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY) ?
            snapshot_count : NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY;
    state->input_vertex_valid_mask = 0u;
    state->vertex_color_valid_mask = 0u;
    state->current_transform_vertex_mask = 0u;
    state->matrix_generation = 0u;
    state->current_matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    state->prepared_light_direction_valid = 0u;
    state->texture_prepare_valid = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->texture_prepare_key_hash = 0u;
    state->texture_prepare_params = 0u;
    state->semantic_branch_path =
        sNdsRendererSemanticSourceProvenance.root_branch_path;
    if (state->semantic_branch_path == 0u)
    {
        state->semantic_branch_path = ndsRendererSemanticBranchPath(
            (u32)sNdsRendererProfileOwner,
            sNdsRendererSemanticSourceProvenance.owner_occurrence,
            sNdsRendererSemanticSourceProvenance.list_ordinal,
            FALSE);
    }
    state->semantic_command_index = 0u;
    state->semantic_tri2_half = 0u;
#endif
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_projected_xy_valid_mask = 0u;
    state->prepared_projected_source_z_valid_mask = 0u;
    state->raw_vertex_fit_mask = 0u;
#else
    (void)snapshots;
    (void)snapshot_count;
#endif
    state->projection_valid = 0u;
    state->modelview_valid = 0u;
    state->matrix_valid = 0u;
    state->matrix_word_valid = 0u;
    if (config == NULL)
    {
        return;
    }
    if ((stats != NULL) && (config->initial_geometry_mode != 0u))
    {
        stats->geometry_mode = config->initial_geometry_mode;
    }
    if (config->initial_projection != NULL)
    {
        state->projection = *config->initial_projection;
        state->projection_valid = TRUE;
    }
    if (config->initial_modelview != NULL)
    {
        state->modelview = *config->initial_modelview;
        state->modelview_valid = TRUE;
    }
    ndsRendererComposeMatrix(state);
    if ((stats != NULL) && (state->matrix_valid != 0u))
    {
        stats->hardware_matrix_seed_count++;
    }
}

static void ndsRendererPushModelview(NDSRendererStats *stats,
                                     NDSRendererTraversalState *state)
{
    u32 depth;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    depth = state->modelview_stack_depth;
    if (depth >= NDS_RENDERER_MODELVIEW_STACK_SIZE)
    {
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
        return;
    }

    state->modelview_stack[depth] = state->modelview;
    state->modelview_valid_stack[depth] = state->modelview_valid;
    state->modelview_stack_depth = depth + 1u;
}

static void ndsRendererApplyMatrixCommand(
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    const Mtx *src;
    NDSRendererMatrix20p12 incoming;
    NDSRendererMatrix20p12 *target;
    u32 *target_valid;
    u32 flags;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    flags = (w0 & 0xffu) ^ NDS_RENDERER_MTX_PUSH_XOR;
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    stats->matrix_command_count++;
    stats->matrix_flags = flags;
    if ((flags & NDS_RENDERER_MTX_PROJECTION) != 0u)
    {
        stats->matrix_projection_count++;
    }
    else
    {
        stats->matrix_modelview_count++;
    }
    if ((flags & NDS_RENDERER_MTX_PUSH) != 0u)
    {
        stats->matrix_push_count++;
    }

    src = ndsRendererResolveDataPointer(config,
                                        (const void *)(uintptr_t)w1,
                                        sizeof(Mtx));
    if (src == NULL)
    {
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
        return;
    }
    ndsRendererMtxLoadN64ToDS20p12(src, &incoming);
    if ((flags & NDS_RENDERER_MTX_PROJECTION) != 0u)
    {
        target = &state->projection;
        target_valid = &state->projection_valid;
    }
    else
    {
        target = &state->modelview;
        target_valid = &state->modelview_valid;
        if ((flags & NDS_RENDERER_MTX_PUSH) != 0u)
        {
            ndsRendererPushModelview(stats, state);
        }
    }

    if ((flags & NDS_RENDERER_MTX_LOAD) != 0u)
    {
        *target = incoming;
        *target_valid = TRUE;
        stats->matrix_load_count++;
    }
    else
    {
        if (*target_valid != 0u)
        {
            ndsRendererMtxMul20p12(target, &incoming, target);
        }
        else
        {
            *target = incoming;
            *target_valid = TRUE;
        }
        stats->matrix_mul_count++;
    }
    ndsRendererComposeMatrix(state);
}

static void ndsRendererApplyMvpRecalcCommand(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    if ((((w0 >> NDS_RENDERER_SPECIAL_1_OFFSET_SHIFT) &
          NDS_RENDERER_SPECIAL_1_OFFSET_MASK) != 0u) ||
        ((w0 & NDS_RENDERER_SPECIAL_1_INDEX_MASK) != 1u) ||
        (w1 != 0u))
    {
        ndsRendererRecordUnsupported(stats, NDS_RENDERER_OP_SPECIAL_1);
        return;
    }

    stats->matrix_command_count++;
    stats->matrix_mvp_recalc_count++;
    ndsRendererInitMatrixWordRaw(state);
}

static void ndsRendererRecordFogMoveWord(NDSRendererStats *stats, u32 w1)
{
    s32 mul;
    s32 ofs;
    s32 fog_min;
    s32 fog_max;

    if ((stats == NULL) || (stats->fog_status >= 2u))
    {
        return;
    }

    mul = (s16)(w1 >> 16);
    ofs = (s16)w1;
    if (mul == 0)
    {
        stats->ignored_state_command_count++;
        return;
    }

    fog_min = 500 - ((ofs * 500) / mul);
    fog_max = (128000 / mul) + fog_min;
    if ((stats->fog_status == 0u) ||
        (stats->fog_min != fog_min) ||
        (stats->fog_max != fog_max))
    {
        stats->fog_status++;
        stats->fog_min = fog_min;
        stats->fog_max = fog_max;
    }
}

static void ndsRendererRecordFogColor(NDSRendererStats *stats, u32 w1)
{
    if ((stats != NULL) && (stats->fog_status < 2u))
    {
        stats->fog_color = w1;
    }
}

static u32 ndsRendererPackLightColor(const u8 color[3])
{
    return ((u32)color[0] << 24) |
        ((u32)color[1] << 16) |
        ((u32)color[2] << 8);
}

static void ndsRendererRecordLightColor(NDSRendererStats *stats,
                                        u32 light, u32 color)
{
    if (stats == NULL)
    {
        return;
    }
    if (light == 1u)
    {
        stats->light_color_1 = color;
        stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_1_MASK;
        stats->light_color_command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileLightColorCommands++;
#endif
    }
    else if (light == 2u)
    {
        stats->light_color_2 = color;
        stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_2_MASK;
        stats->light_color_command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileLightColorCommands++;
#endif
    }
}

static void ndsRendererRecordLightColorMoveWord(NDSRendererStats *stats,
                                                u32 offset, u32 color)
{
    switch (offset)
    {
    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_A:
    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_B:
        ndsRendererRecordLightColor(stats, 1u, color);
        break;

    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_A:
    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_B:
        ndsRendererRecordLightColor(stats, 2u, color);
        break;

    default:
        if (stats != NULL)
        {
            stats->ignored_state_command_count++;
        }
        break;
    }
}

static void ndsRendererRecordLightMoveMem(
    const NDSRendererConfig *config, NDSRendererStats *stats, u32 w0, u32 w1)
{
    u32 index = w0 & 0xffu;
    u32 offset =
        ((w0 >> NDS_RENDERER_MOVEMEM_OFFSET_SHIFT) &
         NDS_RENDERER_MOVEMEM_OFFSET_MASK) * 8u;
    u32 length =
        (((w0 >> NDS_RENDERER_MOVEMEM_LENGTH_SHIFT) &
          NDS_RENDERER_MOVEMEM_LENGTH_MASK) + 1u) * 8u;
    u32 light;
    const Light *src;

    if (stats == NULL)
    {
        return;
    }
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    if ((index != NDS_RENDERER_MOVEMEM_LIGHT) ||
        (offset < NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET) ||
        (((offset - NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET) %
          NDS_RENDERER_MOVEMEM_LIGHT_STRIDE) != 0u) ||
        (length < sizeof(Light)))
    {
        stats->ignored_state_command_count++;
        return;
    }

    light = (offset - NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET) /
        NDS_RENDERER_MOVEMEM_LIGHT_STRIDE;
    src = ndsRendererResolveDataPointer(
        config, (const void *)(uintptr_t)w1, sizeof(*src));
    if (src == NULL)
    {
        stats->ignored_state_command_count++;
        return;
    }

    ndsRendererRecordLightColor(stats, light,
                                ndsRendererPackLightColor(src->l.col));
    if (light == 1u)
    {
        stats->light_dir_x = src->l.dir[0];
        stats->light_dir_y = src->l.dir[1];
        stats->light_dir_z = src->l.dir[2];
        stats->light_dir_mask |= NDS_RENDERER_LIGHT_DIR_1_MASK;
        stats->light_direction_command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileLightDirectionCommands++;
#endif
    }
}

static void ndsRendererApplyMatrixMoveWordCommand(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    u32 index;
    u32 offset;
    u32 word_index;
    u32 *words;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    index = (w0 >> NDS_RENDERER_MOVEWORD_INDEX_SHIFT) &
        NDS_RENDERER_MOVEWORD_INDEX_MASK;
    offset = w0 & NDS_RENDERER_MOVEWORD_OFFSET_MASK;
    if ((index == NDS_RENDERER_MOVEWORD_FOG) &&
        (offset == NDS_RENDERER_MOVEWORD_FOG_OFFSET))
    {
        ndsRendererRecordFogMoveWord(stats, w1);
        return;
    }
    if (index == NDS_RENDERER_MOVEWORD_LIGHTCOL)
    {
        ndsRendererRecordLightColorMoveWord(stats, offset, w1);
        return;
    }
    if ((index != NDS_RENDERER_MOVEWORD_MATRIX) ||
        ((offset % NDS_RENDERER_MATRIX_WORD_BYTES) != 0u) ||
        ((offset / NDS_RENDERER_MATRIX_WORD_BYTES) >=
         NDS_RENDERER_MATRIX_WORD_COUNT))
    {
        stats->ignored_state_command_count++;
        return;
    }

    if (state->matrix_word_valid == 0u)
    {
        ndsRendererInitMatrixWordRaw(state);
    }

    word_index = offset / NDS_RENDERER_MATRIX_WORD_BYTES;
    words = (u32 *)&state->matrix_word_raw.m[0][0];
    words[word_index] = w1;
    ndsRendererMtxLoadN64ToDS20p12(&state->matrix_word_raw, &state->matrix);
    state->matrix_valid = TRUE;
#if NDS_RENDERER_HW_TRIANGLES
    state->current_transform_vertex_mask = 0u;
    state->current_matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    state->matrix_generation = ndsRendererNextMatrixGeneration();
    NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state);
#endif
    stats->matrix_command_count++;
    stats->matrix_move_word_count++;
}

static void ndsRendererApplyPopMatrixCommand(NDSRendererStats *stats,
                                             NDSRendererTraversalState *state,
                                             u32 w1)
{
    u32 count;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    count = w1 / 64u;
    if (count == 0u)
    {
        count = 1u;
    }

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    stats->matrix_command_count++;
    stats->matrix_modelview_count++;
    stats->matrix_pop_count += count;

    while (count != 0u)
    {
        u32 depth = state->modelview_stack_depth;

        if (depth == 0u)
        {
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            break;
        }

        depth--;
        state->modelview = state->modelview_stack[depth];
        state->modelview_valid = state->modelview_valid_stack[depth];
        state->modelview_stack_depth = depth;
        count--;
    }
    ndsRendererComposeMatrix(state);
}

static void NDS_RENDERER_HOT_CODE
ndsRendererApplyVertexCommand(
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    u32 v0;
    u32 count;
    const u8 *src;
    u32 i;
#if NDS_RENDERER_HW_TRIANGLES
    const NDSRendererHardwareLightDirection *prepared_light_direction = NULL;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    const u32 *prepared_light_shade_lut = NULL;
#endif
    u32 matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
#endif

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->vertex_command_count++);
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    if (ndsGBIDecodeF3DEX2Vtx(w0, NDS_RENDERER_MAX_VTX, &v0,
                              &count) == FALSE)
    {
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
        return;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    stats->source_vertex_count += count;
#endif
    if ((v0 + count) > stats->vertex_count)
    {
        stats->vertex_count = v0 + count;
    }
#if !NDS_RENDERER_HW_TRIANGLES
    if (state->matrix_valid == 0u)
    {
        return;
    }
#endif

    src = ndsRendererResolveDataPointer(config,
                                        (const void *)(uintptr_t)w1,
                                        (size_t)count * 16u);
    if (src == NULL)
    {
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
        return;
    }
#if NDS_RENDERER_HW_TRIANGLES
    if (state->matrix_valid != 0u)
    {
        matrix_snapshot = ndsRendererAcquireCurrentMatrixSnapshot(state);
    }
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        if (state->prepared_light_direction_valid == 0u)
        {
            /* Matrix and MOVEMEM handlers invalidate this exact source-state
             * value; adjacent VTX commands can share its float normalization. */
            ndsRendererHardwarePrepareLitDirection(
                stats,
                (state->modelview_valid != 0u) ? &state->modelview : NULL,
                &state->prepared_light_direction);
            state->prepared_light_direction_valid = TRUE;
        }
        prepared_light_direction = &state->prepared_light_direction;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((stats->light_color_mask &
             (NDS_RENDERER_LIGHT_COLOR_1_MASK |
              NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
            (NDS_RENDERER_LIGHT_COLOR_1_MASK |
             NDS_RENDERER_LIGHT_COLOR_2_MASK))
        {
            prepared_light_shade_lut = ndsRendererHardwareGetLightShadeLut(
                stats->light_color_1, stats->light_color_2);
        }
#endif
    }
#endif

    for (i = 0u; i < count; i++)
    {
        u32 index = v0 + i;
#if NDS_RENDERER_HW_TRIANGLES
        NDSRendererInputVertex *input = &state->input_vertices[index];
        u32 mask = 1u << index;
#else
        NDSRendererInputVertex input_storage;
        NDSRendererInputVertex *input = &input_storage;
        NDSRendererClipVertex20p12 *out = &state->vertices[index];
#endif

        ndsRendererDecodeInputVertex(input, src + (i * 16u));
#if NDS_RENDERER_HW_TRIANGLES
        ndsRendererProfileRecordSourceVertexLoad();
        state->input_vertex_valid_mask |= mask;
        if (ndsRendererHardwareRawVertexFits(input) != FALSE)
        {
            state->raw_vertex_fit_mask |= mask;
        }
        else
        {
            state->raw_vertex_fit_mask &= ~mask;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (prepared_light_shade_lut != NULL)
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorLut(
                    input, prepared_light_direction,
                    prepared_light_shade_lut);
        }
        else
#endif
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorPrepared(
                    stats, input, prepared_light_direction);
        }
        state->vertex_color_valid_mask |= mask;
        state->vertex_matrix_snapshot[index] = (u8)matrix_snapshot;
        state->vertex_clip_snapshot[index] =
            NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
        state->vertex_valid_mask &= ~mask;
        state->current_transform_vertex_mask &= ~mask;
        if (state->matrix_valid != 0u)
        {
            state->current_transform_vertex_mask |= mask;
        }
#endif
        if (state->matrix_valid == 0u)
        {
            continue;
        }
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        (void)ndsRendererTransformCachedVertex(
            stats, state, index, &state->matrix, matrix_snapshot);
#else
        /* Profile 0/1 keep source vertices raw for GX. Only an exhausted
         * snapshot table needs the eager clip fallback retained here. */
        if (matrix_snapshot == NDS_RENDERER_MATRIX_SNAPSHOT_INVALID)
        {
            (void)ndsRendererTransformCachedVertex(
                stats, state, index, &state->matrix, matrix_snapshot);
        }
#endif
#else
        ndsRendererTransformVertex20p12(&state->matrix, input, out);
        state->vertex_valid_mask |= 1u << index;
        stats->matrix_transform_count++;
        stats->transformed_vertex_count++;
        if (stats->transformed_vertex_count == 1u)
        {
            stats->first_transformed_x = out->x;
            stats->first_transformed_y = out->y;
            stats->first_transformed_z = out->z;
            stats->first_transformed_w = out->w;
        }
#endif
    }
}

static void ndsRendererApplyModifyVertexCommand(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    u32 where;
    u32 packed_index;
    u32 index;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    where = (w0 >> 16) & 0xffu;
    packed_index = w0 & 0xffffu;
    index = packed_index / 2u;

#if NDS_RENDERER_HW_TRIANGLES
    if ((where == NDS_RENDERER_MWO_POINT_ST) &&
        ((packed_index & 1u) == 0u) &&
        (index < NDS_RENDERER_MAX_VTX) &&
        ((state->input_vertex_valid_mask & (1u << index)) != 0u))
    {
        state->input_vertices[index].s = (s16)(w1 >> 16);
        state->input_vertices[index].t = (s16)(w1 & 0xffffu);
        return;
    }
#else
    (void)where;
    (void)packed_index;
    (void)index;
    (void)w1;
#endif

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
}

static s32 ndsRendererTransformedTriangleReady(
    const NDSRendererTraversalState *state, u32 packed,
    u32 *out_i0, u32 *out_i1, u32 *out_i2)
{
    u32 i0;
    u32 i1;
    u32 i2;
    u32 mask;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);
    if (out_i0 != NULL) { *out_i0 = i0; }
    if (out_i1 != NULL) { *out_i1 = i1; }
    if (out_i2 != NULL) { *out_i2 = i2; }

    if ((state == NULL) ||
        (i0 >= NDS_RENDERER_MAX_VTX) ||
        (i1 >= NDS_RENDERER_MAX_VTX) ||
        (i2 >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }

    mask = (1u << i0) | (1u << i1) | (1u << i2);
    return ((state->vertex_valid_mask & mask) == mask) ? TRUE : FALSE;
}

static void ndsRendererRecordTransformedTriangle(
    NDSRendererStats *stats,
    const NDSRendererTraversalState *state,
    u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;

    if (stats == NULL)
    {
        return;
    }
    if (ndsRendererTransformedTriangleReady(state, packed, &i0, &i1, &i2) ==
        FALSE)
    {
        return;
    }

    if (stats->transformed_triangle_count == 0u)
    {
        stats->first_transformed_tri_v0 = i0;
        stats->first_transformed_tri_v1 = i1;
        stats->first_transformed_tri_v2 = i2;
    }
    stats->transformed_triangle_count++;
}

#if NDS_RENDERER_HW_TRIANGLES
static s32 ndsRendererRoundShiftS32Signed(s32 value, u32 shift)
{
    if (shift == 0u)
    {
        return value;
    }
    return (s32)ndsRendererRoundShiftS64(value, shift);
}

static s32 ndsRendererNativeStageVertexShift(s16 value, u32 shift)
{
    u32 magnitude;

    if (shift == 0u)
    {
        return value;
    }
    magnitude = (value < 0) ? (u32)-(s32)value : (u32)value;
    magnitude = (magnitude + (1u << (shift - 1u))) >> shift;
    return (value < 0) ? -(s32)magnitude : (s32)magnitude;
}

static v16 ndsRendererHardwareCoordToV16(s16 value)
{
    const u32 shift = 12u - NDS_RENDERER_HW_WORLD_UNIT_SHIFT;
    s32 scaled = (s32)value << shift;

    if (scaled > 32767)
    {
        return (v16)32767;
    }
    if (scaled < -32768)
    {
        return (v16)-32768;
    }
    return (v16)scaled;
}

static v16 ndsRendererHardwareVertexCoord(s16 value, u32 scale_world)
{
    if (scale_world == 0u)
    {
        return (v16)value;
    }
    return ndsRendererHardwareCoordToV16(value);
}

static v16 ndsRendererHardwareClampS64ToV16(s64 value)
{
    if (value > 32767)
    {
        return (v16)32767;
    }
    if (value < -32768)
    {
        return (v16)-32768;
    }
    return (v16)value;
}

#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static v16 __attribute__((noinline, optimize("Os")))
ndsRendererHardwareProjectToV16(s64 numerator, s32 denominator)
#else
static inline v16 ndsRendererHardwareProjectToV16(
    s64 numerator, s32 denominator)
#endif
{
    s64 low_product;
    s64 high_product;
    v16 result;

    if (denominator == 0)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary |=
            NDS_RENDERER_HW_DIVISION_ZERO_DENOMINATOR;
#endif
        return 0;
    }

    /* The DS 64/32 divider returns a signed 32-bit quotient. Pre-clamp the
     * exact C result into v16 range so the hardware operation cannot overflow
     * its result register. Negative W reverses both product comparisons. */
    low_product = (s64)-32768 * (s64)denominator;
    high_product = (s64)32767 * (s64)denominator;
    if (((denominator > 0) && (numerator < low_product)) ||
        ((denominator < 0) && (numerator > low_product)))
    {
        result = (v16)-32768;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary +=
            NDS_RENDERER_HW_DIVISION_PRECLAMP_LOW_ONE;
#endif
    }
    else if (((denominator > 0) && (numerator > high_product)) ||
             ((denominator < 0) && (numerator < high_product)))
    {
        result = (v16)32767;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary +=
            NDS_RENDERER_HW_DIVISION_PRECLAMP_HIGH_ONE;
#endif
    }
    else
    {
#if defined(__arm__)
        result = (v16)div64(numerator, denominator);
#else
        result = ndsRendererHardwareClampS64ToV16(
            numerator / denominator);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary++;
#endif
    }

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (result != ndsRendererHardwareClampS64ToV16(
                      numerator / denominator))
    {
        sNdsRendererHardwareDivideSummary |=
            NDS_RENDERER_HW_DIVISION_MISMATCH;
    }
#endif
    return result;
}

#if NDS_RENDERER_SCREEN_SPACE_CENSUS
static void ndsRendererScreenSpaceCensusReset(void)
{
    memset((void *)gNdsRendererScreenSpaceCensus, 0,
           sizeof(gNdsRendererScreenSpaceCensus));
    memset((void *)gNdsRendererScreenSpaceStageOwnerTicks, 0,
           sizeof(gNdsRendererScreenSpaceStageOwnerTicks));
    gNdsRendererScreenSpaceCensusFrameCount = 0u;
    gNdsRendererScreenSpaceCensusOverflowCount = 0u;
}

static void ndsRendererScreenSpaceCensusTriangle(
    u32 owner,
    u32 part,
    u32 identity,
    const NDSRendererClipVertex20p12 clip[3])
{
    volatile NDSRendererScreenSpaceCensusRow *row;
    s32 x_q4[3];
    s32 y_q4[3];
    s64 cross;
    u64 area2_q8;
    u32 i;

    if ((gNdsRendererScreenSpaceCensusArmed == 0u) ||
        (owner >= NDS_RENDERER_PROFILE_OWNER_COUNT) ||
        (part >= NDS_RENDERER_SCREEN_SPACE_CENSUS_PART_COUNT))
    {
        if ((gNdsRendererScreenSpaceCensusArmed != 0u) &&
            ((owner >= NDS_RENDERER_PROFILE_OWNER_COUNT) ||
             (part >= NDS_RENDERER_SCREEN_SPACE_CENSUS_PART_COUNT)))
        {
            gNdsRendererScreenSpaceCensusOverflowCount++;
        }
        return;
    }
    row = &gNdsRendererScreenSpaceCensus[owner][part];
    if (row->identity == 0u)
    {
        row->identity = identity;
    }
    row->triangle_count++;
    for (i = 0u; i < 3u; i++)
    {
        v16 projected_x;
        v16 projected_y;

        if (clip[i].w <= 0)
        {
            row->invalid_count++;
            return;
        }
        projected_x = ndsRendererHardwareProjectToV16(
            (s64)clip[i].x * 4096, clip[i].w);
        projected_y = ndsRendererHardwareProjectToV16(
            (s64)clip[i].y * 4096, clip[i].w);
        /* 256x192 viewport, retained as Q4 screen coordinates. */
        x_q4[i] = (s32)projected_x / 2;
        y_q4[i] = ((s32)projected_y * 3) / 8;
    }
    cross =
        (s64)(x_q4[1] - x_q4[0]) * (y_q4[2] - y_q4[0]) -
        (s64)(y_q4[1] - y_q4[0]) * (x_q4[2] - x_q4[0]);
    area2_q8 = (cross < 0) ? (u64)-cross : (u64)cross;
    if (area2_q8 < 512u)
    {
        row->area_lt_1px_count++;
    }
    if (area2_q8 < 2048u)
    {
        row->area_lt_4px_count++;
    }
    row->area2_q8_sum += area2_q8;
}

static void ndsRendererScreenSpaceCensusStageSegment(
    const NDSNativeStageSegment *segment)
{
    u32 run_offset;

    if ((gNdsRendererScreenSpaceCensusArmed == 0u) || (segment == NULL))
    {
        return;
    }
    for (run_offset = 0u; run_offset < segment->run_count; run_offset++)
    {
        const NDSNativeStageRun *run = &sNdsNativeStageRuns[
            (u32)segment->first_run + run_offset];
        u32 triangle_offset;

        for (triangle_offset = 0u;
             triangle_offset < run->triangle_count;
             triangle_offset++)
        {
            NDSRendererClipVertex20p12 clip[3];
            u32 corner_offset;

            for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
            {
                u32 dense_index = sNdsNativeStageCorners[
                    (u32)run->first_corner + triangle_offset * 3u +
                    corner_offset];
                const NDSNativeStageDenseVertex *dense =
                    &sNdsNativeStageVertices[dense_index];
                NDSRendererInputVertex input = {0};

                input.x = dense->x;
                input.y = dense->y;
                input.z = dense->z;
                ndsRendererTransformVertex20p12(
                    &sNdsNativeStageOwnerExecution.binding_composed[
                        dense->matrix_binding],
                    &input, &clip[corner_offset]);
            }
            ndsRendererScreenSpaceCensusTriangle(
                NDS_RENDERER_PROFILE_OWNER_STAGE,
                run->binding_index,
                sNdsNativeStageBindings[run->binding_index].root_offset,
                clip);
        }
    }
}

static void ndsRendererScreenSpaceCensusFighterRun(
    const NDSNativeRun *run,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count,
    u32 root_index)
{
    u32 run_index;
    u32 triangle_offset;

    if ((gNdsRendererScreenSpaceCensusArmed == 0u) || (run == NULL) ||
        (inputs == NULL) || (root_index >= input_count))
    {
        return;
    }
    run_index = (u32)(run - sNdsNativeFighterRuns);
    for (triangle_offset = 0u;
         triangle_offset < run->triangle_count;
         triangle_offset++)
    {
        NDSRendererClipVertex20p12 clip[3];
        u32 corner_offset;

        for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
        {
            u32 packed = sNdsNativeFighterPackedCorners[
                sNdsNativeFighterRunFirstCorner[run_index] +
                triangle_offset * 3u + corner_offset];
            u32 dense_id = packed & NDS_NATIVE_DENSE_ID_MASK;
            const NDSNativeDenseVertex *dense =
                &sNdsNativeFighterDenseVertices[dense_id];
            const NDSNativePreparedDenseVertex *prepared =
                &sNdsNativeFighterPreparedDense[dense_id];
            u32 matrix_binding = dense->matrix_binding;
            NDSRendererInputVertex input = {0};

            if (matrix_binding >= input_count)
            {
                matrix_binding = root_index;
            }
            input.x = (s16)(prepared->gx_xy & 0xffffu) / 16;
            input.y = (s16)(prepared->gx_xy >> 16) / 16;
            input.z = (s16)prepared->gx_z / 16;
            ndsRendererTransformVertex20p12(
                inputs[matrix_binding].composed_matrix,
                &input, &clip[corner_offset]);
        }
        ndsRendererScreenSpaceCensusTriangle(
            (u32)sNdsRendererRuntimeOwner,
            root_index,
            inputs[root_index].root_offset,
            clip);
    }
}
#endif

static inline v16 ndsRendererHardwareSourceDepthToV16(
    s64 numerator, s32 denominator)
{
    v16 depth = ndsRendererHardwareProjectToV16(numerator, denominator);

    /* Reserve 128 strictly ordered v16 depths at each endpoint for no-Z
     * painter primitives.  Canonical source Z is already inside this central
     * range; the clamp only prevents camera extremes from entering a painter
     * band in the DS's otherwise shared depth channel. */
    if (depth < NDS_RENDERER_HW_SOURCE_DEPTH_MIN)
    {
        return (v16)NDS_RENDERER_HW_SOURCE_DEPTH_MIN;
    }
    if (depth > NDS_RENDERER_HW_SOURCE_DEPTH_MAX)
    {
        return (v16)NDS_RENDERER_HW_SOURCE_DEPTH_MAX;
    }
    return depth;
}
#endif

static void ndsRendererProfileVertexRange(
    const NDSRendererInputVertex *vtx, v16 x, v16 y, v16 z)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (vtx == NULL)
    {
        return;
    }

    if (vtx->x < gNdsRendererProfileRawVertexMinX) { gNdsRendererProfileRawVertexMinX = vtx->x; }
    if (vtx->x > gNdsRendererProfileRawVertexMaxX) { gNdsRendererProfileRawVertexMaxX = vtx->x; }
    if (vtx->y < gNdsRendererProfileRawVertexMinY) { gNdsRendererProfileRawVertexMinY = vtx->y; }
    if (vtx->y > gNdsRendererProfileRawVertexMaxY) { gNdsRendererProfileRawVertexMaxY = vtx->y; }
    if (vtx->z < gNdsRendererProfileRawVertexMinZ) { gNdsRendererProfileRawVertexMinZ = vtx->z; }
    if (vtx->z > gNdsRendererProfileRawVertexMaxZ) { gNdsRendererProfileRawVertexMaxZ = vtx->z; }
    if ((s32)x < gNdsRendererProfileHWVertexMinX) { gNdsRendererProfileHWVertexMinX = x; }
    if ((s32)x > gNdsRendererProfileHWVertexMaxX) { gNdsRendererProfileHWVertexMaxX = x; }
    if ((s32)y < gNdsRendererProfileHWVertexMinY) { gNdsRendererProfileHWVertexMinY = y; }
    if ((s32)y > gNdsRendererProfileHWVertexMaxY) { gNdsRendererProfileHWVertexMaxY = y; }
    if ((s32)z < gNdsRendererProfileHWVertexMinZ) { gNdsRendererProfileHWVertexMinZ = z; }
    if ((s32)z > gNdsRendererProfileHWVertexMaxZ) { gNdsRendererProfileHWVertexMaxZ = z; }
#else
    (void)vtx;
#endif
    if ((x == (v16)32767) || (x == (v16)-32768) ||
        (y == (v16)32767) || (y == (v16)-32768) ||
        (z == (v16)32767) || (z == (v16)-32768))
    {
        ndsRendererProfileRecordVertexSaturate();
    }
}

static void ndsRendererProfileHWVertexRange(v16 x, v16 y, v16 z)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if ((s32)x < gNdsRendererProfileHWVertexMinX) { gNdsRendererProfileHWVertexMinX = x; }
    if ((s32)x > gNdsRendererProfileHWVertexMaxX) { gNdsRendererProfileHWVertexMaxX = x; }
    if ((s32)y < gNdsRendererProfileHWVertexMinY) { gNdsRendererProfileHWVertexMinY = y; }
    if ((s32)y > gNdsRendererProfileHWVertexMaxY) { gNdsRendererProfileHWVertexMaxY = y; }
    if ((s32)z < gNdsRendererProfileHWVertexMinZ) { gNdsRendererProfileHWVertexMinZ = z; }
    if ((s32)z > gNdsRendererProfileHWVertexMaxZ) { gNdsRendererProfileHWVertexMaxZ = z; }
#endif
    if ((x == (v16)32767) || (x == (v16)-32768) ||
        (y == (v16)32767) || (y == (v16)-32768) ||
        (z == (v16)32767) || (z == (v16)-32768))
    {
        ndsRendererProfileRecordVertexSaturate();
    }
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererAbsDiffS32(s32 lhs, s32 rhs)
{
    s64 diff = (s64)lhs - (s64)rhs;

    if (diff < 0)
    {
        diff = -diff;
    }
    return (diff > (s64)0xffffffffu) ? 0xffffffffu : (u32)diff;
}

static void ndsRendererHardwareRecordOracleVertex(
    const NDSRendererTraversalState *state, u32 index)
{
    const NDSRendererMatrixSnapshot *snapshot;
    const NDSRendererMatrix20p12 *matrix = NULL;
    NDSRendererClipVertex20p12 expected;
    const NDSRendererClipVertex20p12 *actual;
    u32 dx;
    u32 dy;
    u32 dz;
    u32 dw;
    u32 max_delta;

    if ((state == NULL) ||
        (index >= NDS_RENDERER_MAX_VTX) ||
        ((state->input_vertex_valid_mask & (1u << index)) == 0u) ||
        ((state->vertex_valid_mask & (1u << index)) == 0u) ||
        (state->vertex_clip_snapshot[index] !=
         state->vertex_matrix_snapshot[index]))
    {
        return;
    }

    snapshot = ndsRendererGetMatrixSnapshot(
        state, state->vertex_matrix_snapshot[index]);
    if (snapshot != NULL)
    {
        matrix = &snapshot->matrix;
    }
    else if (((state->current_transform_vertex_mask & (1u << index)) != 0u) &&
             (state->matrix_valid != 0u))
    {
        /* Bounded-table overflow is eagerly transformed at VTX load. */
        matrix = &state->matrix;
    }
    if (matrix == NULL)
    {
        return;
    }

    ndsRendererTransformVertex20p12(matrix,
                                    &state->input_vertices[index],
                                    &expected);
    actual = &state->vertices[index];
    dx = ndsRendererAbsDiffS32(expected.x, actual->x);
    dy = ndsRendererAbsDiffS32(expected.y, actual->y);
    dz = ndsRendererAbsDiffS32(expected.z, actual->z);
    dw = ndsRendererAbsDiffS32(expected.w, actual->w);
    max_delta = dx;
    if (dy > max_delta) { max_delta = dy; }
    if (dz > max_delta) { max_delta = dz; }
    if (dw > max_delta) { max_delta = dw; }

    gNdsRendererProfileOracleSamples++;
    if (max_delta > gNdsRendererProfileOracleMaxDelta)
    {
        gNdsRendererProfileOracleMaxDelta = max_delta;
    }
    if (max_delta > NDS_RENDERER_HW_ORACLE_EPSILON)
    {
        gNdsRendererProfileOracleMismatches++;
    }
}

static void ndsRendererHardwareRecordOracleTriangle(
    const NDSRendererTraversalState *state, u32 i0, u32 i1, u32 i2)
{
    ndsRendererHardwareRecordOracleVertex(state, i0);
    ndsRendererHardwareRecordOracleVertex(state, i1);
    ndsRendererHardwareRecordOracleVertex(state, i2);
}
#endif

static s32 ndsRendererCombineUsesColor(u32 w0, u32 w1, u32 source)
{
    return ((((w0 >> 20) & 0x0fu) == source) ||
            (((w1 >> 28) & 0x0fu) == source) ||
            (((w0 >> 15) & 0x1fu) == source) ||
            (((w1 >> 15) & 0x07u) == source) ||
            (((w0 >> 5) & 0x0fu) == source) ||
            (((w1 >> 24) & 0x0fu) == source) ||
            (((w0 >> 0) & 0x1fu) == source) ||
            (((w1 >> 6) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererCombineOutputUsesColor(u32 w0, u32 w1, u32 source)
{
    return ((((w0 >> 15) & 0x1fu) == source) ||
            (((w1 >> 15) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererCombineSecondOutputUsesColor(u32 w0, u32 w1,
                                                   u32 source)
{
    return ((((w0 >> 0) & 0x1fu) == source) ||
            (((w1 >> 6) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareUseSecondCycle(const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            ((stats->othermode_h & NDS_RENDERER_CYCLETYPE_MASK) ==
             NDS_RENDERER_CYC_2CYCLE)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareUsesTexel01Lerp(
    const NDSRendererStats *stats)
{
    u32 w0;
    u32 w1;

    if ((stats == NULL) ||
        (stats->texture_combine_count == 0u) ||
        (ndsRendererHardwareUseSecondCycle(stats) == FALSE))
    {
        return FALSE;
    }

    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    /* BattleShip's animated Pupupu water uses G_CC_TEMPLERP for color and
     * alpha, followed by COMBINED * SHADE / COMBINED * PRIMITIVE.
     * Decode the semantic mux rather than keying this DS adaptation to a
     * stage address or raw display-list pointer. */
    return ((((w0 >> 20) & 0x0fu) == NDS_RENDERER_CCMUX_TEXEL1) &&
            (((w1 >> 28) & 0x0fu) == NDS_RENDERER_CCMUX_TEXEL0) &&
            (((w0 >> 15) & 0x1fu) ==
             NDS_RENDERER_CCMUX_PRIM_LOD_FRAC) &&
            (((w1 >> 15) & 0x07u) == NDS_RENDERER_CCMUX_TEXEL0) &&
            (((w0 >> 12) & 0x07u) == NDS_RENDERER_ACMUX_TEXEL1) &&
            (((w1 >> 12) & 0x07u) == NDS_RENDERER_ACMUX_TEXEL0) &&
            (((w0 >> 9) & 0x07u) == NDS_RENDERER_ACMUX_1) &&
            (((w1 >> 9) & 0x07u) == NDS_RENDERER_ACMUX_TEXEL0) &&
            (((w0 >> 5) & 0x0fu) == NDS_RENDERER_CCMUX_COMBINED) &&
            (((w1 >> 24) & 0x0fu) == NDS_RENDERER_CCMUX_ZERO_AB) &&
            (((w0 >> 0) & 0x1fu) == NDS_RENDERER_CCMUX_SHADE) &&
            (((w1 >> 6) & 0x07u) == NDS_RENDERER_CCMUX_ZERO_D) &&
            (((w1 >> 21) & 0x07u) == NDS_RENDERER_ACMUX_COMBINED) &&
            (((w1 >> 3) & 0x07u) == NDS_RENDERER_ACMUX_0) &&
            (((w1 >> 18) & 0x07u) == NDS_RENDERER_ACMUX_PRIMITIVE) &&
            (((w1 >> 0) & 0x07u) == NDS_RENDERER_ACMUX_0)) ? TRUE : FALSE;
}

static const NDSRendererTextureLoadState *
ndsRendererHardwareFindTextureLoadForTmem(const NDSRendererStats *stats,
                                           u32 tmem)
{
    const NDSRendererTextureLoadState *best = NULL;
    u32 i;

    if (stats == NULL)
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT; i++)
    {
        const NDSRendererTextureLoadState *load = &stats->texture_loads[i];

        if ((load->valid != 0u) && (load->load_tmem == tmem) &&
            ((best == NULL) || (load->sequence > best->sequence)))
        {
            best = load;
        }
    }
    return best;
}

static s32 ndsRendererHardwareSecondCyclePassesCombined(
    const NDSRendererStats *stats)
{
    u32 w0;
    u32 w1;

    if (stats == NULL)
    {
        return FALSE;
    }
    if (ndsRendererHardwareUseSecondCycle(stats) == FALSE)
    {
        return TRUE;
    }
    if (stats->env_color != 0xffffffffu)
    {
        return FALSE;
    }

    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    /* BattleShip's normal fighter mode uses (COMBINED - 0) * ENV + 0.
     * With the opaque-white environment selected at ftdisplaymain.c:1192-1196,
     * cycle 2 preserves the source material/shade result from cycle 1. */
    return ((((w0 >> 5) & 0x0fu) == NDS_RENDERER_CCMUX_COMBINED) &&
            (((w1 >> 24) & 0x0fu) == NDS_RENDERER_CCMUX_ZERO_AB) &&
            (((w0 >> 0) & 0x1fu) == NDS_RENDERER_CCMUX_ENVIRONMENT) &&
            (((w1 >> 6) & 0x07u) == NDS_RENDERER_CCMUX_ZERO_D)) ? TRUE :
                                                                  FALSE;
}

static s32 ndsRendererHardwareOutputUsesColor(const NDSRendererStats *stats,
                                              u32 source)
{
    u32 w0;
    u32 w1;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return FALSE;
    }
    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    if (ndsRendererHardwareUseSecondCycle(stats) == FALSE)
    {
        return ndsRendererCombineOutputUsesColor(w0, w1, source);
    }
    if (ndsRendererCombineSecondOutputUsesColor(w0, w1, source) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererCombineSecondOutputUsesColor(
            w0, w1, NDS_RENDERER_CCMUX_COMBINED) != FALSE)
    {
        return ndsRendererCombineOutputUsesColor(w0, w1, source);
    }
    return FALSE;
}

static s32 ndsRendererCombineUsesAlpha(u32 w0, u32 w1, u32 source)
{
    return ((((w0 >> 9) & 0x07u) == source) ||
            (((w1 >> 9) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererCombineSecondOutputUsesAlpha(u32 w1, u32 source)
{
    return ((((w1 >> 18) & 0x07u) == source) ||
            (((w1 >> 0) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareOutputUsesAlpha(const NDSRendererStats *stats,
                                              u32 source)
{
    u32 w0;
    u32 w1;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return FALSE;
    }
    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    if (ndsRendererHardwareUseSecondCycle(stats) == FALSE)
    {
        return ndsRendererCombineUsesAlpha(w0, w1, source);
    }
    if (ndsRendererCombineSecondOutputUsesAlpha(w1, source) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererCombineSecondOutputUsesAlpha(
            w1, NDS_RENDERER_ACMUX_COMBINED) != FALSE)
    {
        return ndsRendererCombineUsesAlpha(w0, w1, source);
    }
    return FALSE;
}

static s32 ndsRendererHardwareUseDecal(const NDSRendererStats *stats)
{
    u32 w1;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return FALSE;
    }
    w1 = stats->texture_combine_w1;
    return ((((w1 >> 28) & 0x0fu) == ((w1 >> 15) & 0x07u)) ?
        TRUE : FALSE);
}

static s32 ndsRendererHardwareUsePrimDepth(const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            ((stats->othermode_l & NDS_RENDERER_ZSOURCE_MASK) ==
             NDS_RENDERER_ZSOURCE_PRIM)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwarePrimitiveDecal(const NDSRendererStats *stats)
{
    if (ndsRendererHardwareUseDecal(stats) == FALSE)
    {
        return FALSE;
    }
    return ((((stats->texture_combine_w0 >> 20) & 0x0fu) ==
             NDS_RENDERER_CCMUX_PRIMITIVE) ? TRUE : FALSE);
}

static void ndsRendererHardwareRecordUseTextureReject(
    const NDSRendererStats *stats,
    u32 reason)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    switch (reason)
    {
    case NDS_RENDERER_HW_USETEX_REJECT_NO_STATS:
        gNdsRendererProfileUseTextureRejectNoStatsCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_STATE_OFF:
        gNdsRendererProfileUseTextureRejectStateOffCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_NO_COMBINE:
        gNdsRendererProfileUseTextureRejectNoCombineCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_PRIMITIVE_DECAL:
        gNdsRendererProfileUseTextureRejectPrimitiveDecalCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_NO_TEXEL0:
        gNdsRendererProfileUseTextureRejectNoTexel0Count++;
        break;
    default:
        break;
    }
    if (gNdsRendererProfileUseTextureRejectFirstReason == 0u)
    {
        gNdsRendererProfileUseTextureRejectFirstReason = reason;
        if (stats != NULL)
        {
            gNdsRendererProfileUseTextureRejectFirstFlags =
                stats->texture_state_flags;
            gNdsRendererProfileUseTextureRejectFirstW0 =
                stats->texture_combine_w0;
            gNdsRendererProfileUseTextureRejectFirstW1 =
                stats->texture_combine_w1;
            gNdsRendererProfileUseTextureRejectFirstGeometry =
                stats->geometry_mode;
        }
    }
#else
    (void)stats;
    (void)reason;
#endif
}

static s32 ndsRendererHardwareTextureImplicitStateOn(
    const NDSRendererStats *stats)
{
    const NDSRendererTileState *render_tile;
    u32 required_mask;

    if ((stats == NULL) ||
        ((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_ON) != 0u))
    {
        return FALSE;
    }

    required_mask = NDS_RENDERER_TEXTURE_SETTIMG |
        NDS_RENDERER_TEXTURE_SETTILE |
        NDS_RENDERER_TEXTURE_SETTILESIZE;
    if (((stats->texture_mask & required_mask) != required_mask) ||
        ((stats->texture_mask &
          (NDS_RENDERER_TEXTURE_LOADBLOCK | NDS_RENDERER_TEXTURE_LOADTILE)) ==
         0u) ||
        (stats->texture_image == 0u) ||
        (stats->texture_load_texels == 0u))
    {
        return FALSE;
    }

    render_tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    return ((render_tile->set_seen != 0u) &&
            (render_tile->size_seen != 0u) &&
            (render_tile->line != 0u) &&
            (render_tile->width != 0u) &&
            (render_tile->height != 0u)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareUseTexture(const NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        ndsRendererHardwareRecordUseTextureReject(
            NULL, NDS_RENDERER_HW_USETEX_REJECT_NO_STATS);
        return FALSE;
    }
    if ((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_ON) == 0u)
    {
        if (ndsRendererHardwareTextureImplicitStateOn(stats) == FALSE)
        {
            ndsRendererHardwareRecordUseTextureReject(
                stats, NDS_RENDERER_HW_USETEX_REJECT_STATE_OFF);
            return FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileUseTextureImplicitOnCount++;
#endif
    }
    if (stats->texture_combine_count == 0u)
    {
        ndsRendererHardwareRecordUseTextureReject(
            stats, NDS_RENDERER_HW_USETEX_REJECT_NO_COMBINE);
        return FALSE;
    }
    if (ndsRendererHardwarePrimitiveDecal(stats) != FALSE)
    {
        ndsRendererHardwareRecordUseTextureReject(
            stats, NDS_RENDERER_HW_USETEX_REJECT_PRIMITIVE_DECAL);
        return FALSE;
    }
    if (ndsRendererCombineUsesColor(
            stats->texture_combine_w0, stats->texture_combine_w1,
            NDS_RENDERER_CCMUX_TEXEL0) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererHardwareOutputUsesAlpha(
            stats, NDS_RENDERER_ACMUX_TEXEL0) != FALSE)
    {
        return TRUE;
    }
    ndsRendererHardwareRecordUseTextureReject(
        stats, NDS_RENDERER_HW_USETEX_REJECT_NO_TEXEL0);
    return FALSE;
}

static s32 ndsRendererHardwareUsesLitPrimitiveModulate(
    const NDSRendererStats *stats)
{
    u32 a;
    u32 b;
    u32 c;
    u32 d;

    if ((stats == NULL) || (stats->texture_combine_count == 0u) ||
        (ndsRendererHardwareSecondCyclePassesCombined(stats) == FALSE))
    {
        return FALSE;
    }

    a = (stats->texture_combine_w0 >> 20) & 0x0fu;
    b = (stats->texture_combine_w1 >> 28) & 0x0fu;
    c = (stats->texture_combine_w0 >> 15) & 0x1fu;
    d = (stats->texture_combine_w1 >> 15) & 0x07u;
    if ((b != NDS_RENDERER_CCMUX_ZERO_AB) ||
        (d != NDS_RENDERER_CCMUX_ZERO_D))
    {
        return FALSE;
    }
    return (((a == NDS_RENDERER_CCMUX_PRIMITIVE) &&
             (c == NDS_RENDERER_CCMUX_SHADE)) ||
            ((a == NDS_RENDERER_CCMUX_SHADE) &&
             (c == NDS_RENDERER_CCMUX_PRIMITIVE))) ? TRUE : FALSE;
}

static u32 ndsRendererHardwareColorSource(const NDSRendererStats *stats)
{
    if (ndsRendererHardwareUsesLitPrimitiveModulate(stats) != FALSE)
    {
        return stats->prim_color;
    }
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if (ndsRendererHardwareOutputUsesColor(
                stats, NDS_RENDERER_CCMUX_ENVIRONMENT) != FALSE)
        {
            return stats->env_color;
        }
        if (ndsRendererHardwareOutputUsesColor(
                stats, NDS_RENDERER_CCMUX_PRIMITIVE) != FALSE)
        {
            return stats->prim_color;
        }
    }
    return 0u;
}

static s32 ndsRendererHardwareLitShadeCombine(const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            (stats->texture_combine_count != 0u) &&
            ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
            (ndsRendererCombineUsesColor(stats->texture_combine_w0,
                                         stats->texture_combine_w1,
                                         NDS_RENDERER_CCMUX_SHADE) != FALSE)) ?
        TRUE : FALSE;
}

static s32 ndsRendererHardwareUseMaterialColor(const NDSRendererStats *stats)
{
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if (ndsRendererHardwareLitShadeCombine(stats) != FALSE)
        {
            return ndsRendererHardwareUsesLitPrimitiveModulate(stats);
        }
        return ((ndsRendererHardwareOutputUsesColor(
                     stats, NDS_RENDERER_CCMUX_ENVIRONMENT) != FALSE) ||
                (ndsRendererHardwareOutputUsesColor(
                     stats, NDS_RENDERER_CCMUX_PRIMITIVE) != FALSE)) ?
            TRUE : FALSE;
    }
    return FALSE;
}

static s32 ndsRendererHardwareUseVertexColor(const NDSRendererStats *stats)
{
    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return TRUE;
    }
    if (ndsRendererHardwareLitShadeCombine(stats) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererHardwareUseMaterialColor(stats) != FALSE)
    {
        return FALSE;
    }
    return ndsRendererCombineUsesColor(
        stats->texture_combine_w0, stats->texture_combine_w1,
        NDS_RENDERER_CCMUX_SHADE);
}

static s32 ndsRendererHardwareBlendAlphaUsesMemory(
    const NDSRendererStats *stats)
{
    u32 mode;

    if (stats == NULL)
    {
        return FALSE;
    }
    mode = stats->othermode_l;
    if (((mode >> NDS_RENDERER_BLEND_ALPHA_CYCLE1_SHIFT) &
         NDS_RENDERER_BLEND_ALPHA_BITS_MASK) == NDS_RENDERER_G_BL_A_MEM)
    {
        return TRUE;
    }
    return ((ndsRendererHardwareUseSecondCycle(stats) != FALSE) &&
            (((mode >> NDS_RENDERER_BLEND_ALPHA_CYCLE2_SHIFT) &
              NDS_RENDERER_BLEND_ALPHA_BITS_MASK) ==
             NDS_RENDERER_G_BL_A_MEM)) ? TRUE : FALSE;
}

static u32 ndsRendererHardwareAlpha(const NDSRendererStats *stats,
                                    const NDSRendererInputVertex *vtx)
{
    u32 alpha = 0xffu;

    if (vtx != NULL)
    {
        alpha = vtx->a;
    }
    if ((stats != NULL) &&
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) !=
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD) &&
        ((stats->othermode_l & NDS_RENDERER_FORCE_BL) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_CVG_X_ALPHA) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_ZMODE_MASK) !=
         NDS_RENDERER_ZMODE_XLU))
    {
        return 31u;
    }
    if (ndsRendererHardwareBlendAlphaUsesMemory(stats) != FALSE)
    {
        return 31u;
    }
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if (ndsRendererHardwareOutputUsesAlpha(
                stats, NDS_RENDERER_ACMUX_PRIMITIVE) != FALSE)
        {
            alpha = stats->prim_color & 0xffu;
        }
        else if (ndsRendererHardwareOutputUsesAlpha(
                     stats, NDS_RENDERER_ACMUX_ENVIRONMENT) != FALSE)
        {
            alpha = stats->env_color & 0xffu;
        }
        else if ((ndsRendererHardwareOutputUsesAlpha(
                      stats, NDS_RENDERER_ACMUX_TEXEL0) == FALSE) &&
                 (ndsRendererHardwareOutputUsesAlpha(
                      stats, NDS_RENDERER_ACMUX_SHADE) == FALSE))
        {
            alpha = (ndsRendererHardwareOutputUsesAlpha(
                         stats, NDS_RENDERER_ACMUX_0) != FALSE) ?
                0u : 0xffu;
        }
    }
    return alpha >> 3;
}

static s32 ndsRendererHardwareAlphaUsesVertex(
    const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) !=
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD) &&
        ((stats->othermode_l & NDS_RENDERER_FORCE_BL) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_CVG_X_ALPHA) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_ZMODE_MASK) !=
         NDS_RENDERER_ZMODE_XLU))
    {
        return FALSE;
    }
    if (ndsRendererHardwareBlendAlphaUsesMemory(stats) != FALSE)
    {
        return FALSE;
    }
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if ((ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_PRIMITIVE) != FALSE) ||
            (ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_ENVIRONMENT) != FALSE))
        {
            return FALSE;
        }
        if ((ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_TEXEL0) == FALSE) &&
            (ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_SHADE) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static u32 ndsRendererHardwarePolyFmt(const NDSRendererStats *stats, u32 alpha)
{
    u32 poly_id = (stats != NULL) ?
        (stats->texture_combine_count & NDS_RENDERER_POLY_ID_MASK) : 0u;
    u32 poly_fmt = POLY_CULL_NONE | POLY_ALPHA(alpha) | POLY_ID(poly_id);
    u32 mode = (stats != NULL) ? stats->geometry_mode : 0u;

    if (ndsRendererHardwareUseDecal(stats) != FALSE)
    {
        poly_fmt |= POLY_DECAL;
    }
    if ((mode & NDS_RENDERER_GEOM_FOG) != 0u)
    {
        poly_fmt |= POLY_FOG;
    }
    if ((mode & NDS_RENDERER_GEOM_CULL_FRONT) != 0u)
    {
        poly_fmt &= ~((u32)POLY_CULL_BACK);
    }
    if ((mode & NDS_RENDERER_GEOM_CULL_BACK) != 0u)
    {
        poly_fmt &= ~((u32)POLY_CULL_FRONT);
    }
    return poly_fmt;
}

static void ndsRendererHardwareApplyAlphaTest(const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) ==
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD))
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc((stats->blend_color & 0xffu) >> 4);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }
}

static void ndsRendererHardwareApplyFog(const NDSRendererStats *stats)
{
    s32 range;
    s32 shift;
    s32 density;
    s32 inc;
    s32 i;
    u32 color;

    if ((stats == NULL) ||
        (stats->fog_status == 0u) ||
        (stats->fog_max <= stats->fog_min))
    {
        glDisable(GL_FOG);
        return;
    }

    range = stats->fog_max - stats->fog_min;
    shift = 0;
    for (i = 500; (i >= range) && (i > 0); i >>= 1)
    {
        shift++;
    }

    density = 0;
    inc = (((128 * 1000) << 1) / (range * 32) + 1) >> (shift + 1);
    if (inc < 1)
    {
        inc = 1;
    }
    for (i = 0; i < 32; i++)
    {
        glFogDensity(i, density);
        density += inc;
        if (density > 127)
        {
            density = 127;
        }
    }

    color = stats->fog_color;
    glFogShift(shift);
    glFogOffset((stats->fog_min * 0x7fff / 1000) - (0x400 >> shift));
    glFogColor((color >> 27) & 0x1fu, (color >> 19) & 0x1fu,
               (color >> 11) & 0x1fu, (color >> 3) & 0x1fu);
    glEnable(GL_FOG);
}

static s32 ndsRendererHardwareTextureFilterOffset(
    const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->othermode_h & NDS_RENDERER_TEXTFILT_MASK) !=
         NDS_RENDERER_TF_POINT))
    {
        return NDS_RENDERER_TEXCOORD_FILTER_OFFSET;
    }
    return 0;
}

static s32 ndsRendererHardwareUseTextureMatrix(
    const NDSRendererStats *stats)
{
    return ((stats == NULL) ||
            ((stats->othermode_h & NDS_RENDERER_TEXTPERSP_MASK) ==
             NDS_RENDERER_TP_PERSP)) ? TRUE : FALSE;
}

static s16 ndsRendererHardwareTexCoord(s16 coord, u32 scale, u32 origin,
                                       s32 offset)
{
    s64 scaled_t16 = ((s64)coord * (s64)scale) >> 17;
    s64 origin_t16 = (s64)origin << 2;

    return (s16)(scaled_t16 - origin_t16 + offset);
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererProfileTextureCoord(s16 s, s16 t)
{
    gNdsRendererProfileTexturedVertexCount++;
    if (s < gNdsRendererProfileTextureCoordMinS)
    {
        gNdsRendererProfileTextureCoordMinS = s;
    }
    if (s > gNdsRendererProfileTextureCoordMaxS)
    {
        gNdsRendererProfileTextureCoordMaxS = s;
    }
    if (t < gNdsRendererProfileTextureCoordMinT)
    {
        gNdsRendererProfileTextureCoordMinT = t;
    }
    if (t > gNdsRendererProfileTextureCoordMaxT)
    {
        gNdsRendererProfileTextureCoordMaxT = t;
    }
}
#endif

static u32 ndsRendererHardwareColorByte(u32 color, u32 shift)
{
    return (color >> shift) & 0xffu;
}

static inline u32 ndsRendererHardwareScaleMaterialChannel5(
    u32 shaded, u32 material)
{
    u32 numerator = (shaded * material) + 127u;

    /* For every 16-bit numerator, floor(n / 255) is exactly
     * (n + 1 + (n >> 8)) >> 8.  Fold the following RGB15 divide by eight
     * into the same shift so the native owner does not issue three wide
     * constant divisions for every prepared vertex. */
    return (numerator + 1u + (numerator >> 8)) >> 11;
}

static u8 ndsRendererHardwareClampColor(s32 value)
{
    if (value < 0)
    {
        return 0u;
    }
    if (value > 255)
    {
        return 0xffu;
    }
    return (u8)value;
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static const u32 *
ndsRendererHardwareFindLightShadeLut(u32 diffuse, u32 ambient)
{
    u32 i;

    for (i = 0u; i < NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT; i++)
    {
        const NDSRendererHardwareLightShadeCacheEntry *entry =
            &sNdsRendererHardwareLightShadeCache[i];

        if ((entry->valid != 0u) &&
            (entry->diffuse == diffuse) &&
            (entry->ambient == ambient))
        {
            return entry->rgb;
        }
    }
    return NULL;
}

static const u32 * __attribute__((noinline))
ndsRendererHardwareGetLightShadeLut(u32 diffuse, u32 ambient)
{
    NDSRendererHardwareLightShadeCacheEntry *entry;
    const u32 *resident;
    u32 i;

    /* Cache only the exact RGB function of the two source light colors and
     * diffuse numerator. Vertex normals, direction, and alpha stay live. */
    resident = ndsRendererHardwareFindLightShadeLut(diffuse, ambient);
    if (resident != NULL)
    {
        return resident;
    }

    entry = &sNdsRendererHardwareLightShadeCache[
        sNdsRendererHardwareLightShadeCacheNext];
    sNdsRendererHardwareLightShadeCacheNext =
        (sNdsRendererHardwareLightShadeCacheNext + 1u) &
        (NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT - 1u);
    entry->valid = FALSE;
    entry->diffuse = diffuse;
    entry->ambient = ambient;
    for (i = 0u; i < NDS_RENDERER_HW_LIGHT_SHADE_LUT_COUNT; i++)
    {
        s32 r = (s32)ndsRendererHardwareColorByte(ambient, 24) +
            (s32)((ndsRendererHardwareColorByte(diffuse, 24) * i) / 127u);
        s32 g = (s32)ndsRendererHardwareColorByte(ambient, 16) +
            (s32)((ndsRendererHardwareColorByte(diffuse, 16) * i) / 127u);
        s32 b = (s32)ndsRendererHardwareColorByte(ambient, 8) +
            (s32)((ndsRendererHardwareColorByte(diffuse, 8) * i) / 127u);

        entry->rgb[i] =
            ((u32)ndsRendererHardwareClampColor(r) << 24) |
            ((u32)ndsRendererHardwareClampColor(g) << 16) |
            ((u32)ndsRendererHardwareClampColor(b) << 8);
    }
    entry->valid = TRUE;
    return entry->rgb;
}
#endif

static u32 ndsRendererHardwareLightColor(NDSRendererStats *stats, u32 mask,
                                         u32 color, u32 fallback)
{
    if ((stats == NULL) || ((stats->light_color_mask & mask) == 0u))
    {
        if (stats != NULL)
        {
            stats->light_fallback_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileLightFallbackCount++;
#endif
        }
        return fallback;
    }
    return color;
}

static void ndsRendererHardwarePrepareLitDirection(
    const NDSRendererStats *stats,
    const NDSRendererMatrix20p12 *modelview,
    NDSRendererHardwareLightDirection *out)
{
    s32 light_x;
    s32 light_y;
    s32 light_z;

    if (out == NULL)
    {
        return;
    }

    light_x = (stats != NULL) ? stats->light_dir_x : 0;
    light_y = (stats != NULL) ? stats->light_dir_y : 0;
    light_z = (stats != NULL) ? stats->light_dir_z : 0;
    if ((stats != NULL) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u) &&
        (modelview != NULL))
    {
#if NDS_SCENE_MIP_CACHE_LAB && (NDS_RENDERER_PROFILE_LEVEL < 2)
        s64 transformed_x =
            (s64)light_x * modelview->m[0][0] +
            (s64)light_y * modelview->m[0][1] +
            (s64)light_z * modelview->m[0][2];
        s64 transformed_y =
            (s64)light_x * modelview->m[1][0] +
            (s64)light_y * modelview->m[1][1] +
            (s64)light_z * modelview->m[1][2];
        s64 transformed_z =
            (s64)light_x * modelview->m[2][0] +
            (s64)light_y * modelview->m[2][1] +
            (s64)light_z * modelview->m[2][2];
        s64 length_squared =
            (transformed_x * transformed_x) +
            (transformed_y * transformed_y) +
            (transformed_z * transformed_z);

        if (length_squared > 0)
        {
            u32 length = sqrt64(length_squared);

            if (length != 0u)
            {
                light_x = div64(transformed_x * 127, (s32)length);
                light_y = div64(transformed_y * 127, (s32)length);
                light_z = div64(transformed_z * 127, (s32)length);
            }
        }
#else
        f32 transformed_x = (f32)(
            (s64)light_x * modelview->m[0][0] +
            (s64)light_y * modelview->m[0][1] +
            (s64)light_z * modelview->m[0][2]);
        f32 transformed_y = (f32)(
            (s64)light_x * modelview->m[1][0] +
            (s64)light_y * modelview->m[1][1] +
            (s64)light_z * modelview->m[1][2]);
        f32 transformed_z = (f32)(
            (s64)light_x * modelview->m[2][0] +
            (s64)light_y * modelview->m[2][1] +
            (s64)light_z * modelview->m[2][2]);
        f32 length = sqrtf((transformed_x * transformed_x) +
                           (transformed_y * transformed_y) +
                           (transformed_z * transformed_z));

        if (length > 0.0F)
        {
            light_x = (s32)((transformed_x * 127.0F) / length);
            light_y = (s32)((transformed_y * 127.0F) / length);
            light_z = (s32)((transformed_z * 127.0F) / length);
        }
#endif
    }

    out->x = light_x;
    out->y = light_y;
    out->z = light_z;
}

static u32 ndsRendererHardwareLitDiffuseNumer(
    const NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction)
{
    s32 light_x;
    s32 light_y;
    s32 light_z;
    s32 dot;

    if ((stats == NULL) || (vtx == NULL) ||
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) == 0u))
    {
        return 127u;
    }

    light_x = (direction != NULL) ? direction->x : stats->light_dir_x;
    light_y = (direction != NULL) ? direction->y : stats->light_dir_y;
    light_z = (direction != NULL) ? direction->z : stats->light_dir_z;

    dot = ((s32)(s8)vtx->r * light_x) +
        ((s32)(s8)vtx->g * light_y) +
        ((s32)(s8)vtx->b * light_z);
    if (dot <= 0)
    {
        return 0u;
    }
    if (dot > (127 * 127))
    {
        return 127u;
    }
    return ndsRendererHardwareDivideLitDotBy127((u32)dot);
}

static u32 NDS_RENDERER_HOT_CODE
ndsRendererHardwareLitShadeColorPrepared(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction)
{
    u32 light_1;
    u32 light_2;
    u32 ambient;
    u32 diffuse;
    u32 diffuse_numer;
    s32 r;
    s32 g;
    s32 b;

    if (vtx == NULL)
    {
        return 0xffffffffu;
    }
    if ((stats == NULL) ||
        ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) == 0u))
    {
        return ((u32)vtx->r << 24) | ((u32)vtx->g << 16) |
            ((u32)vtx->b << 8) | (u32)vtx->a;
    }

    light_1 = ndsRendererHardwareLightColor(
        stats, NDS_RENDERER_LIGHT_COLOR_1_MASK, stats->light_color_1,
        NDS_RENDERER_LIGHT_COLOR_1_FALLBACK);
    light_2 = ndsRendererHardwareLightColor(
        stats, NDS_RENDERER_LIGHT_COLOR_2_MASK, stats->light_color_2,
        NDS_RENDERER_LIGHT_COLOR_2_FALLBACK);
    diffuse = light_1;
    ambient = light_2;

    diffuse_numer = ndsRendererHardwareLitDiffuseNumer(stats, vtx, direction);
    r = (s32)ndsRendererHardwareColorByte(ambient, 24) +
        (s32)((ndsRendererHardwareColorByte(diffuse, 24) * diffuse_numer) /
              127u);
    g = (s32)ndsRendererHardwareColorByte(ambient, 16) +
        (s32)((ndsRendererHardwareColorByte(diffuse, 16) * diffuse_numer) /
              127u);
    b = (s32)ndsRendererHardwareColorByte(ambient, 8) +
        (s32)((ndsRendererHardwareColorByte(diffuse, 8) * diffuse_numer) /
              127u);

    return ((u32)ndsRendererHardwareClampColor(r) << 24) |
        ((u32)ndsRendererHardwareClampColor(g) << 16) |
        ((u32)ndsRendererHardwareClampColor(b) << 8) | (u32)vtx->a;
}

static u32 ndsRendererHardwareLitShadeColor(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererMatrix20p12 *modelview)
{
    NDSRendererHardwareLightDirection direction;
    const NDSRendererHardwareLightDirection *prepared_direction = NULL;

    if ((stats != NULL) &&
        ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        ndsRendererHardwarePrepareLitDirection(stats, modelview, &direction);
        prepared_direction = &direction;
    }
    return ndsRendererHardwareLitShadeColorPrepared(
        stats, vtx, prepared_direction);
}

static inline u16 ndsRendererHardwarePackedResolvedColor(
    u32 color,
    u32 material_color,
    s32 use_material_color)
{
    if (use_material_color != FALSE)
    {
        u32 r = ((ndsRendererHardwareColorByte(color, 24) *
                  ndsRendererHardwareColorByte(material_color, 24)) + 127u) /
            255u;
        u32 g = ((ndsRendererHardwareColorByte(color, 16) *
                  ndsRendererHardwareColorByte(material_color, 16)) + 127u) /
            255u;
        u32 b = ((ndsRendererHardwareColorByte(color, 8) *
                  ndsRendererHardwareColorByte(material_color, 8)) + 127u) /
            255u;

        return RGB15((u8)(r >> 3), (u8)(g >> 3), (u8)(b >> 3));
    }
    return RGB15((u8)((color >> 27) & 0x1fu),
                 (u8)((color >> 19) & 0x1fu),
                 (u8)((color >> 11) & 0x1fu));
}

static inline u16 ndsRendererHardwarePackedValidVertexColor(
    u32 material_color,
    s32 use_material_color,
    s32 use_vertex_color,
    u32 vertex_color)
{
    if ((use_material_color != FALSE) && (use_vertex_color == FALSE))
    {
        return RGB15((u8)((material_color >> 27) & 0x1fu),
                     (u8)((material_color >> 19) & 0x1fu),
                     (u8)((material_color >> 11) & 0x1fu));
    }
    if (use_vertex_color == FALSE)
    {
        return RGB15(31u, 31u, 31u);
    }
    return ndsRendererHardwarePackedResolvedColor(
        vertex_color, material_color, use_material_color);
}

static u16 ndsRendererHardwarePackedVertexColor(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    u32 material_color,
    s32 use_material_color,
    s32 use_vertex_color,
    u32 vertex_color,
    s32 vertex_color_valid)
{
    u32 color;

    if (vertex_color_valid != FALSE)
    {
        return ndsRendererHardwarePackedValidVertexColor(
            material_color, use_material_color,
            use_vertex_color, vertex_color);
    }
    if ((use_material_color != FALSE) && (use_vertex_color == FALSE))
    {
        return ndsRendererHardwarePackedValidVertexColor(
            material_color, use_material_color,
            use_vertex_color, vertex_color);
    }
    if (use_vertex_color == FALSE)
    {
        return ndsRendererHardwarePackedValidVertexColor(
            material_color, use_material_color,
            use_vertex_color, vertex_color);
    }
    color = ndsRendererHardwareLitShadeColor(stats, vtx, NULL);
    return ndsRendererHardwarePackedResolvedColor(
        color, material_color, use_material_color);
}

static const void *ndsRendererResolveTextureDataPointer(
    const NDSRendererConfig *config, const void *ptr, size_t bytes)
{
    if (ptr == NULL)
    {
        return NULL;
    }
    if ((config != NULL) && (config->resolve_data != NULL))
    {
        return config->resolve_data(ptr, bytes, config->user);
    }
    if ((config != NULL) && (config->validate_range != NULL) &&
        (config->validate_range((const Gfx *)ptr, bytes, config->user) ==
         FALSE))
    {
        return NULL;
    }
    return ptr;
}

static s32 ndsRendererHardwareTextureKeyEqual(
    const NDSRendererHardwareTextureKey *a,
    const NDSRendererHardwareTextureKey *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return FALSE;
    }
    return (memcmp(a, b, sizeof(*a)) == 0) ? TRUE : FALSE;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererProfileTextureKeyHashFull(
    const NDSRendererHardwareTextureKey *key)
{
    const u32 *words = (const u32 *)key;
    u32 hash = 0u;
    u32 i;

    if (key == NULL)
    {
        return 0u;
    }
    for (i = 0u; i < (sizeof(*key) / sizeof(*words)); i++)
    {
        hash = ndsRendererProfileHashU32(hash, words[i]);
    }
    return hash;
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
static u32 ndsRendererHardwareTextureKeyHash(
    const NDSRendererHardwareTextureKey *key)
{
    u32 hash;

    if (key == NULL)
    {
        return 0u;
    }
    /* This is an index fingerprint, not the equality oracle.  Mix the
     * high-entropy image/material identity and animated tile state, then keep
     * the full 236-byte comparison on every candidate hit.  Hashing all 59
     * words cost more ARM9 time than the open-address lookup saved. */
    hash = key->image ^ (key->tlut_image * 0x9e3779b9u);
    hash ^= key->texel1_image * 0x85ebca6bu;
    hash ^= (key->width << 16) ^ key->height;
    hash ^= (key->format << 28) ^ (key->size << 24) ^ key->flags;
    hash ^= (key->load_uls << 16) ^ key->load_ult ^ key->load_lrs;
    hash ^= (key->tile_uls << 16) ^ key->tile_ult ^ key->tile_lrs;
    hash ^= (key->texel1_load_uls << 16) ^ key->texel1_load_ult ^
        key->texel1_load_lrs;
    hash ^= (key->texel1_tile_uls << 16) ^ key->texel1_tile_ult ^
        key->texel1_tile_lrs;
    hash ^= key->prim_lod_fraction * 0xc2b2ae35u;
    hash ^= key->combine_w0 ^ (key->combine_w1 * 0x27d4eb2fu);
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    return hash;
}

static void ndsRendererHardwareTextureLookupInsert(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    u32 slot_value;
    u32 slot;
    u32 probe;

    if ((entry == NULL) || (entry->ready == 0u))
    {
        return;
    }
    slot_value = (u32)(entry - sNdsRendererHardwareTextureCache) + 1u;
    slot = entry->key_hash & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    for (probe = 0u; probe < NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT; probe++)
    {
        u32 value = sNdsRendererHardwareTextureLookup[slot];

        if (value == slot_value)
        {
            return;
        }
        if (value == NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
        {
            sNdsRendererHardwareTextureLookup[slot] = (u8)slot_value;
            return;
        }
        slot = (slot + 1u) & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    }
}

static void ndsRendererHardwareTextureLookupRemove(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    u32 slot_value;
    u32 slot;
    u32 probe;

    if ((entry == NULL) || (entry->ready == 0u))
    {
        return;
    }
    slot_value = (u32)(entry - sNdsRendererHardwareTextureCache) + 1u;
    slot = entry->key_hash & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    for (probe = 0u; probe < NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT; probe++)
    {
        u32 value = sNdsRendererHardwareTextureLookup[slot];

        if (value == NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
        {
            return;
        }
        if (value == slot_value)
        {
            /* Repair the remainder of this linear-probe cluster instead of
             * leaving tombstones that animated water keys could accumulate
             * over the complete timed match. The table is twice the cache size,
             * so an empty terminator is guaranteed. */
            sNdsRendererHardwareTextureLookup[slot] =
                NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY;
            slot = (slot + 1u) &
                (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
            while (sNdsRendererHardwareTextureLookup[slot] !=
                   NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
            {
                value = sNdsRendererHardwareTextureLookup[slot];
                sNdsRendererHardwareTextureLookup[slot] =
                    NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY;
                ndsRendererHardwareTextureLookupInsert(
                    &sNdsRendererHardwareTextureCache[value - 1u]);
                slot = (slot + 1u) &
                    (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
            }
            return;
        }
        slot = (slot + 1u) & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    }
}
#endif

static s32 ndsRendererHardwareTexel1RefreshCompatible(
    const NDSRendererHardwareTextureKey *a,
    const NDSRendererHardwareTextureKey *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return FALSE;
    }
    /* The exact Pupupu material animates fraction, image IDs, and tile
     * origins. All of those change converted pixels, but not the resident DS
     * RGBA allocation. Width/height and format distinguish its large and
     * small water surfaces; same-frame reuse is excluded by the caller. */
    return ((a->data_layout == b->data_layout) &&
            (a->format == b->format) &&
            (a->size == b->size) &&
            (a->width == b->width) &&
            (a->height == b->height) &&
            (a->combine_w0 == b->combine_w0) &&
            (a->combine_w1 == b->combine_w1)) ? TRUE : FALSE;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static s32 ndsRendererHardwareTextureKeyWouldLegacyAlias(
    const NDSRendererHardwareTextureKey *a,
    const NDSRendererHardwareTextureKey *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return FALSE;
    }
    return ((a->image == b->image) &&
            (a->image_width == b->image_width) &&
            (a->tlut_image == b->tlut_image) &&
            (a->tlut_count == b->tlut_count) &&
            (a->format == b->format) &&
            (a->size == b->size) &&
            (a->width == b->width) &&
            (a->height == b->height) &&
            (a->render_tile == b->render_tile) &&
            (a->render_tmem == b->render_tmem) &&
            (a->render_palette == b->render_palette) &&
            (a->render_tile_cms == b->render_tile_cms) &&
            (a->render_tile_cmt == b->render_tile_cmt) &&
            (a->render_tile_masks == b->render_tile_masks) &&
            (a->render_tile_maskt == b->render_tile_maskt) &&
            (a->render_tile_shifts == b->render_tile_shifts) &&
            (a->render_tile_shiftt == b->render_tile_shiftt) &&
            (a->load_tile == b->load_tile) &&
            (a->load_uls == b->load_uls) &&
            (a->load_ult == b->load_ult) &&
            (a->load_lrs == b->load_lrs) &&
            (a->load_dxt == b->load_dxt) &&
            (a->load_texels == b->load_texels) &&
            (a->tile_uls == b->tile_uls) &&
            (a->tile_ult == b->tile_ult) &&
            (a->line == b->line) &&
            (a->flags == b->flags) &&
            (a->texel1_image == b->texel1_image) &&
            (a->texel1_image_format == b->texel1_image_format) &&
            (a->texel1_image_size == b->texel1_image_size) &&
            (a->texel1_image_width == b->texel1_image_width) &&
            (a->texel1_load_kind == b->texel1_load_kind) &&
            (a->texel1_render_tmem == b->texel1_render_tmem) &&
            (a->texel1_render_line == b->texel1_render_line) &&
            (a->texel1_render_palette == b->texel1_render_palette) &&
            (a->texel1_render_tile_cms == b->texel1_render_tile_cms) &&
            (a->texel1_render_tile_cmt == b->texel1_render_tile_cmt) &&
            (a->texel1_render_tile_masks == b->texel1_render_tile_masks) &&
            (a->texel1_render_tile_maskt == b->texel1_render_tile_maskt) &&
            (a->texel1_render_tile_shifts == b->texel1_render_tile_shifts) &&
            (a->texel1_render_tile_shiftt == b->texel1_render_tile_shiftt) &&
            (a->texel1_load_tile == b->texel1_load_tile) &&
            (a->texel1_load_uls == b->texel1_load_uls) &&
            (a->texel1_load_ult == b->texel1_load_ult) &&
            (a->texel1_load_lrs == b->texel1_load_lrs) &&
            (a->texel1_load_dxt == b->texel1_load_dxt) &&
            (a->texel1_load_texels == b->texel1_load_texels) &&
            (a->texel1_tile_uls == b->texel1_tile_uls) &&
            (a->texel1_tile_ult == b->texel1_tile_ult) &&
            (a->prim_lod_fraction == b->prim_lod_fraction) &&
            (a->combine_w0 == b->combine_w0) &&
            (a->combine_w1 == b->combine_w1)) ? TRUE : FALSE;
}
#endif

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindTexture(const NDSRendererHardwareTextureKey *key,
                               u32 key_hash)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 slot;
    u32 probe;
#else
    u32 i;
#endif

    if (key == NULL)
    {
        return NULL;
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeFrameSummary.texture_lookup_call_count++;
    entry = (NDSRendererHardwareTextureCacheEntry *)
        sNdsRendererHardwareActiveTextureEntry;
    if ((entry != NULL) && (entry->ready != 0u) &&
        (entry->key_hash == key_hash) &&
        (ndsRendererHardwareTextureKeyEqual(&entry->key, key) != FALSE))
    {
        sNdsRendererRuntimeFrameSummary.texture_lookup_active_hit_count++;
        return entry;
    }

    slot = key_hash & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    for (probe = 0u; probe < NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT; probe++)
    {
        u32 value = sNdsRendererHardwareTextureLookup[slot];

        sNdsRendererRuntimeFrameSummary.texture_lookup_probe_count++;
        if (value == NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
        {
            break;
        }
        entry = &sNdsRendererHardwareTextureCache[value - 1u];
        if ((entry->ready != 0u) && (entry->key_hash == key_hash) &&
            (ndsRendererHardwareTextureKeyEqual(&entry->key, key) != FALSE))
        {
            sNdsRendererRuntimeFrameSummary
                .texture_lookup_table_hit_count++;
            return entry;
        }
        slot = (slot + 1u) & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    }
#else
    (void)key_hash;
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        if ((sNdsRendererHardwareTextureCache[i].ready != 0u) &&
            (ndsRendererHardwareTextureKeyEqual(
                 &sNdsRendererHardwareTextureCache[i].key, key) != FALSE))
        {
            return &sNdsRendererHardwareTextureCache[i];
        }
        if ((sNdsRendererHardwareTextureCache[i].ready != 0u) &&
            (ndsRendererHardwareTextureKeyWouldLegacyAlias(
                 &sNdsRendererHardwareTextureCache[i].key, key) != FALSE))
        {
            ndsRendererProfileRecordTextureAliasAvoid();
        }
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeFrameSummary.texture_lookup_miss_count++;
#endif
    return NULL;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindStageSourceFrameTexture(
    const NDSRendererHardwareTextureKey *key)
{
    u32 i;

    if (key == NULL)
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];
        NDSRendererHardwareTextureKey source_frame;

        if ((entry->ready == 0u) || (entry->pinned != 0u))
        {
            continue;
        }
        source_frame = entry->key;
        source_frame.image = key->image;
        if (ndsRendererHardwareTextureKeyEqual(&source_frame, key) != FALSE)
        {
            return entry;
        }
    }
    return NULL;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindTexel1RefreshTexture(
    const NDSRendererHardwareTextureKey *key)
{
    u32 i;

    if ((key == NULL) || (key->texel1_image == 0u))
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];

        if ((entry->ready != 0u) && (entry->pinned == 0u) &&
            (entry->key.texel1_image != 0u) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)) &&
            (ndsRendererHardwareTexel1RefreshCompatible(
                 &entry->key, key) != FALSE))
        {
            return entry;
        }
    }
    return NULL;
}

static s32 ndsRendererHardwareReplaceTextureData(
    NDSRendererHardwareTextureCacheEntry *entry,
    const void *texture,
    u32 staged_bytes,
    u32 texture_bytes,
    const u8 *row_map,
    u32 row_bytes,
    u32 row_count)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_REPLACE_REFRESH);
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    if ((entry == NULL) || (entry->name == 0) ||
        (texture == NULL) || (staged_bytes == 0u) ||
        (texture_bytes == 0u) ||
        ((row_map == NULL) && ((texture_bytes % staged_bytes) != 0u)) ||
        ((row_map != NULL) &&
         ((row_bytes == 0u) || (row_count == 0u) ||
          (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
          ((staged_bytes % row_bytes) != 0u) ||
          (texture_bytes != (row_bytes * row_count)))))
    {
        return FALSE;
    }
    ndsRendererBenchmarkSinkWord((u32)entry->name);
    ndsRendererBenchmarkSinkWord(staged_bytes);
    ndsRendererBenchmarkSinkWord(texture_bytes);
    return TRUE;
#else
    void *vram_address;
    uintptr_t vram_first;
    uintptr_t vram_last;
    u32 vram_state;
    u32 offset;
    u32 i;

    if ((entry == NULL) || (entry->name == 0) ||
        (texture == NULL) || (staged_bytes == 0u) ||
        (texture_bytes == 0u) ||
        ((row_map == NULL) && ((texture_bytes % staged_bytes) != 0u)) ||
        ((row_map != NULL) &&
         ((row_bytes == 0u) || (row_count == 0u) ||
          (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
          ((staged_bytes % row_bytes) != 0u) ||
          (texture_bytes != (row_bytes * row_count)))))
    {
        return FALSE;
    }
    if (row_map != NULL)
    {
        u32 staged_rows = staged_bytes / row_bytes;

        for (i = 0u; i < row_count; i++)
        {
            if (row_map[i] >= staged_rows)
            {
                return FALSE;
            }
        }
    }
    vram_address = glGetTexturePointer(entry->name);
    if (vram_address == NULL)
    {
        return FALSE;
    }

    /* sm64-nds updates allocated textures by temporarily exposing every
     * owning texture bank to the CPU, then restoring the primary mapping. */
    ndsRendererHardwareEndBatch();
    vram_state = VRAM_CR;
    vram_first = (uintptr_t)vram_address;
    vram_last = vram_first + texture_bytes - 1u;
    if ((vram_first < (uintptr_t)VRAM_B) &&
        (vram_last >= (uintptr_t)VRAM_A))
    {
        vramSetBankA(VRAM_A_LCD);
    }
    if ((vram_first < (uintptr_t)VRAM_C) &&
        (vram_last >= (uintptr_t)VRAM_B))
    {
        vramSetBankB(VRAM_B_LCD);
    }
    if ((vram_first < (uintptr_t)VRAM_D) &&
        (vram_last >= (uintptr_t)VRAM_C))
    {
        vramSetBankC(VRAM_C_LCD);
    }
    if ((vram_first < (uintptr_t)VRAM_E) &&
        (vram_last >= (uintptr_t)VRAM_D))
    {
        vramSetBankD(VRAM_D_LCD);
    }

    DC_FlushRange(texture, staged_bytes);
    if (row_map == NULL)
    {
        for (offset = 0u; offset < texture_bytes; offset += staged_bytes)
        {
            dmaCopyWords(0, texture, (u8 *)vram_address + offset,
                         staged_bytes);
        }
    }
    else
    {
        /* The first occurrence of every distinct rendered row is stored once.
         * Adjacent first occurrences remain adjacent in the staging buffer, so
         * coalesce those runs and issue individual copies only for repeats. */
        for (i = 0u; i < row_count; )
        {
            u32 run = 1u;
            u32 source_row = row_map[i];

            while (((i + run) < row_count) &&
                   (row_map[i + run] == (source_row + run)))
            {
                run++;
            }
            dmaCopyWords(0, (const u8 *)texture + (source_row * row_bytes),
                         (u8 *)vram_address + (i * row_bytes),
                         run * row_bytes);
            i += run;
        }
    }
    vramRestorePrimaryBanks(vram_state);
    return TRUE;
#endif
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static s32 ndsRendererHardwareTextureRefreshUses(
    const u16 *pixels)
{
    u32 i;

    for (i = 0u; i < sNdsRendererHardwareTextureRefreshCount; i++)
    {
        if (sNdsRendererHardwareTextureRefreshQueue[i].pixels == pixels)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static s32 ndsRendererHardwareQueueTextureRefresh(
    NDSRendererHardwareTextureCacheEntry *entry,
    const u16 *pixels,
    u32 staged_bytes,
    u32 texture_bytes,
    const u8 *row_map,
    u32 row_bytes,
    u32 row_count)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_REPLACE_REFRESH);
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    (void)entry;
    (void)pixels;
    (void)staged_bytes;
    (void)texture_bytes;
    (void)row_map;
    (void)row_bytes;
    (void)row_count;
    return FALSE;
#else
    NDSRendererHardwareTextureRefresh *refresh;

    if ((entry == NULL) || (entry->name == 0) || (pixels == NULL) ||
        (staged_bytes == 0u) || (texture_bytes == 0u) ||
        ((row_map == NULL) && ((texture_bytes % staged_bytes) != 0u)) ||
        ((row_map != NULL) &&
         ((row_bytes == 0u) || (row_count == 0u) ||
          (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
          ((staged_bytes % row_bytes) != 0u) ||
          (texture_bytes != (row_bytes * row_count)))) ||
        (sNdsRendererHardwareTextureRefreshCount >=
         NDS_RENDERER_HW_TEXTURE_REFRESH_QUEUE_COUNT) ||
        (glGetTexturePointer(entry->name) == NULL))
    {
        return FALSE;
    }
    refresh = &sNdsRendererHardwareTextureRefreshQueue[
        sNdsRendererHardwareTextureRefreshCount++];
    refresh->entry = entry;
    refresh->pixels = pixels;
    refresh->staged_bytes = staged_bytes;
    refresh->texture_bytes = texture_bytes;
    refresh->row_bytes = row_bytes;
    refresh->row_count = row_count;
    if (row_map != NULL)
    {
        memcpy(refresh->row_map, row_map, row_count);
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureVBlankQueuedUploads++;
    gNdsRendererProfileTextureVBlankQueuedBytes += texture_bytes;
#endif
    return TRUE;
#endif
}
#endif

u32 ndsRendererHardwareCommitPendingTextureRefreshes(void)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 committed = 0u;
    u32 i;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 start;
    u32 start_line;

    start_line = REG_VCOUNT;
    start = cpuGetTiming();
    gNdsRendererProfileTextureVBlankStartLine = start_line;
#endif

    for (i = 0u; i < sNdsRendererHardwareTextureRefreshCount; i++)
    {
        NDSRendererHardwareTextureRefresh *refresh =
            &sNdsRendererHardwareTextureRefreshQueue[i];

        if (ndsRendererHardwareReplaceTextureData(
                refresh->entry, refresh->pixels, refresh->staged_bytes,
                refresh->texture_bytes,
                (refresh->row_count != 0u) ? refresh->row_map : NULL,
                refresh->row_bytes, refresh->row_count) != FALSE)
        {
            committed++;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        else
        {
            gNdsRendererProfileTextureVBlankFallbackCount++;
        }
#endif
        refresh->entry = NULL;
        refresh->pixels = NULL;
        refresh->staged_bytes = 0u;
        refresh->texture_bytes = 0u;
        refresh->row_bytes = 0u;
        refresh->row_count = 0u;
    }
    sNdsRendererHardwareTextureRefreshCount = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureVBlankCommittedUploads += committed;
    gNdsRendererProfileTextureVBlankCommitTicks += cpuGetTiming() - start;
    gNdsRendererProfileTextureVBlankEndLine = REG_VCOUNT;
    if ((committed != 0u) &&
        ((start_line < 192u) ||
         (gNdsRendererProfileTextureVBlankEndLine < 192u)))
    {
        gNdsRendererProfileTextureVBlankOutsideCount++;
    }
#endif
    return committed;
#else
    /* Profile 2 keeps its large semantic/oracle ledger resident. Shipping and
     * coarse builds own the compact VBlank staging buffers; the forensic build
     * retains the exact synchronous upload as its independent oracle route. */
    return 0u;
#endif
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareReleaseTexture(
    NDSRendererHardwareTextureCacheEntry *entry)
{
    if (entry == NULL)
    {
        return NULL;
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareTextureLookupRemove(entry);
#endif
    ndsRendererHardwareEndBatch();
    if (sNdsRendererHardwareBoundTextureName == (u32)entry->name)
    {
        sNdsRendererHardwareBoundTextureName = 0u;
    }
    if (sNdsRendererHardwareActiveTextureEntry == entry)
    {
        sNdsRendererHardwareActiveTextureEntry = NULL;
    }
    if (entry->name != 0)
    {
        ndsRendererHardwareFencedGlDeleteTextures(1, &entry->name);
    }
    memset(entry, 0, sizeof(*entry));
    return entry;
}

static s32 ndsRendererHardwareEvictTexture(
    const NDSRendererHardwareTextureCacheEntry *exclude)
{
    u32 i;

    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        u32 index = sNdsRendererHardwareTextureCacheNext %
            NDS_RENDERER_HW_TEXTURE_CACHE_COUNT;
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[index];

        sNdsRendererHardwareTextureCacheNext = index + 1u;
        if ((entry != exclude) && (entry->name != 0) &&
            (entry->pinned == 0u) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)))
        {
            ndsRendererProfileRecordTextureEvict();
            (void)ndsRendererHardwareReleaseTexture(entry);
            return TRUE;
        }
    }
    return FALSE;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareAllocTexture(void)
{
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 i;
    u32 index;

    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_ALLOC);

    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        if (sNdsRendererHardwareTextureCache[i].ready == 0u)
        {
            entry = &sNdsRendererHardwareTextureCache[i];
            return (entry->name != 0) ?
                ndsRendererHardwareReleaseTexture(entry) : entry;
        }
    }
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        index = sNdsRendererHardwareTextureCacheNext %
            NDS_RENDERER_HW_TEXTURE_CACHE_COUNT;
        sNdsRendererHardwareTextureCacheNext = index + 1u;
        entry = &sNdsRendererHardwareTextureCache[index];
        if ((entry->pinned == 0u) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)))
        {
            /* libnds allocates texture VRAM in glTexImage2D. Delete the old
             * allocation, but never recycle a texture referenced this frame. */
            ndsRendererProfileRecordTextureEvict();
            return ndsRendererHardwareReleaseTexture(entry);
        }
    }
    return NULL;
}

void ndsRendererHardwareDiscardTextureCache(void)
{
    if ((sNdsRendererBattleStaticTexturePrepared != 0u) ||
        (sNdsRendererBattleStaticTextureArmed != 0u))
    {
        gNdsRendererBattleStaticTextureViolationCount++;
    }
    sNdsRendererBattleStaticTexturePrepared = FALSE;
#if NDS_RENDERER_HW_TRIANGLES
    u32 i;

    ndsRendererHardwareEndBatch();
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        if ((sNdsRendererHardwareTextureCache[i].name != 0) ||
            (sNdsRendererHardwareTextureCache[i].ready != 0u))
        {
            (void)ndsRendererHardwareReleaseTexture(
                &sNdsRendererHardwareTextureCache[i]);
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    memset(sNdsRendererHardwareTextureLookup, 0,
           sizeof(sNdsRendererHardwareTextureLookup));
    memset(sNdsRendererHardwareTextureRefreshQueue, 0,
           sizeof(sNdsRendererHardwareTextureRefreshQueue));
    sNdsRendererHardwareTextureRefreshCount = 0u;
#endif
    if (sNdsRendererHardwareNoTextureName != 0)
    {
        ndsRendererHardwareFencedGlDeleteTextures(
            1, &sNdsRendererHardwareNoTextureName);
        sNdsRendererHardwareNoTextureName = 0;
    }
    sNdsRendererHardwareTextureCacheNext = 0u;
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    ndsRendererHardwareResetSourceCaches();
#endif
    sNdsRendererBattleStaticTextureArmed = FALSE;
}

s32 ndsRendererHardwareUploadSceneMipCache(const u16 *mip0,
                                            const u16 *mip1,
                                            const u16 *mip2)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
    const u16 *pixels[NDS_RENDERER_SCENE_MIP_COUNT] = {
        mip0, mip1, mip2
    };
    u32 i;

    if ((mip0 == NULL) || (mip1 == NULL) || (mip2 == NULL))
    {
        return FALSE;
    }
    ndsRendererHardwareDiscardTextureCache();
    for (i = 0u; i < NDS_RENDERER_SCENE_MIP_COUNT; i++)
    {
        if (sNdsRendererSceneMipTextureNames[i] != 0)
        {
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsRendererSceneMipTextureNames[i]);
            sNdsRendererSceneMipTextureNames[i] = 0;
        }
        if (ndsRendererHardwareFencedGlGenTextures(
                1, &sNdsRendererSceneMipTextureNames[i]) == 0)
        {
            break;
        }
        ndsRendererHardwareBindTextureState(
            sNdsRendererSceneMipTextureNames[i]);
        if (ndsRendererHardwareFencedGlTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA,
                TEXTURE_SIZE_128, TEXTURE_SIZE_128, 0,
                TEXGEN_TEXCOORD, pixels[i]) == 0)
        {
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsRendererSceneMipTextureNames[i]);
            sNdsRendererSceneMipTextureNames[i] = 0;
            break;
        }
    }
    if (i != NDS_RENDERER_SCENE_MIP_COUNT)
    {
        u32 j;

        for (j = 0u; j < NDS_RENDERER_SCENE_MIP_COUNT; j++)
        {
            if (sNdsRendererSceneMipTextureNames[j] != 0)
            {
                ndsRendererHardwareFencedGlDeleteTextures(
                    1, &sNdsRendererSceneMipTextureNames[j]);
                sNdsRendererSceneMipTextureNames[j] = 0;
            }
        }
        sNdsRendererHardwareBoundTextureName = 0u;
        return FALSE;
    }
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;
#else
    (void)mip0;
    (void)mip1;
    (void)mip2;
    return FALSE;
#endif
}

s32 ndsRendererHardwareDrawSceneMipCache(u32 mip_index,
                                          const s32 *tex_s_q4,
                                          const s32 *tex_t_q4,
                                          u32 columns,
                                          u32 rows)
{
#if NDS_RENDERER_HW_TRIANGLES && NDS_SCENE_MIP_CACHE_LAB
    v16 vertex_x[64];
    v16 vertex_y[64];
    u32 vertex_count;
    u32 row;
    u32 column;
    u32 triangle_count;

    if ((mip_index >= NDS_RENDERER_SCENE_MIP_COUNT) ||
        (sNdsRendererSceneMipTextureNames[mip_index] == 0) ||
        (tex_s_q4 == NULL) || (tex_t_q4 == NULL) ||
        (columns < 2u) || (rows < 2u))
    {
        return FALSE;
    }
    vertex_count = columns * rows;
    if (vertex_count > (sizeof(vertex_x) / sizeof(vertex_x[0])))
    {
        return FALSE;
    }
    for (row = 0u; row < rows; row++)
    {
        for (column = 0u; column < columns; column++)
        {
            u32 index = (row * columns) + column;

            vertex_x[index] = (v16)(-4096 +
                (s32)((8192u * column) / (columns - 1u)));
            vertex_y[index] = (v16)(4096 -
                (s32)((8192u * row) / (rows - 1u)));
        }
    }

    ndsRendererHardwareEndBatch();
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    ndsRendererHardwareBindTextureState(
        sNdsRendererSceneMipTextureNames[mip_index]);
    ndsRendererHardwareSetPolyFmt(
        POLY_CULL_NONE | POLY_ALPHA(31) | POLY_ID(63));
    glColor(RGB15(31, 31, 31));
    glBegin(GL_TRIANGLE);
    for (row = 0u; row + 1u < rows; row++)
    {
        for (column = 0u; column + 1u < columns; column++)
        {
            u32 i00 = (row * columns) + column;
            u32 i10 = i00 + 1u;
            u32 i01 = i00 + columns;
            u32 i11 = i01 + 1u;

#define NDS_SCENE_MIP_EMIT(index) do { \
    glTexCoord2t16((t16)tex_s_q4[(index)], \
                   (t16)tex_t_q4[(index)]); \
    glVertex3v16(vertex_x[(index)], vertex_y[(index)], (v16)4090); \
} while (0)
            NDS_SCENE_MIP_EMIT(i00);
            NDS_SCENE_MIP_EMIT(i01);
            NDS_SCENE_MIP_EMIT(i10);
            NDS_SCENE_MIP_EMIT(i10);
            NDS_SCENE_MIP_EMIT(i01);
            NDS_SCENE_MIP_EMIT(i11);
#undef NDS_SCENE_MIP_EMIT
        }
    }
    triangle_count = (columns - 1u) * (rows - 1u) * 2u;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices +=
        triangle_count * 3u;
    sNdsRendererHardwareSubmitted = TRUE;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    sNdsRendererHardwareMatrixGeneration = 0u;
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;
#else
    (void)mip_index;
    (void)tex_s_q4;
    (void)tex_t_q4;
    (void)columns;
    (void)rows;
    return FALSE;
#endif
}

static s32 ndsRendererHardwareTextureSizeEnum(u32 size, int *out)
{
    int value;

    if (out == NULL)
    {
        return FALSE;
    }
    switch (size)
    {
    case 8u: value = TEXTURE_SIZE_8; break;
    case 16u: value = TEXTURE_SIZE_16; break;
    case 32u: value = TEXTURE_SIZE_32; break;
    case 64u: value = TEXTURE_SIZE_64; break;
    case 128u: value = TEXTURE_SIZE_128; break;
    case 256u: value = TEXTURE_SIZE_256; break;
    default:
        return FALSE;
    }
    *out = value;
    return TRUE;
}

static s32 ndsRendererHardwarePrepareIFCommonAtlas(
    u32 width, u32 height, u32 texture_format,
    const u16 *palette, u32 palette_entries,
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name)
{
#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    int size_x;
    int size_y;
    int name = 0;
    u32 bytes;
    u32 upload_attempts = 0u;
    u8 *pixels = (u8 *)sNdsRendererHardwareTextureScratch;

    if ((fill == NULL) || (palette == NULL) || (texture_name == NULL) ||
        (((texture_format == GL_RGB8_A5) && (palette_entries > 8u)) ||
         ((texture_format == GL_RGB32_A3) && (palette_entries > 32u)) ||
         ((texture_format != GL_RGB8_A5) &&
          (texture_format != GL_RGB32_A3)) || (palette_entries == 0u)) ||
        (height == 0u) || (width > (UINT32_MAX / height)) ||
        (ndsRendererHardwareTextureSizeEnum(width, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(height, &size_y) == FALSE))
    {
        return FALSE;
    }
    bytes = width * height;
    if ((bytes > sizeof(sNdsRendererHardwareTextureScratch)) ||
        (fill(pixels, bytes, user_data) == FALSE))
    {
        return FALSE;
    }
    if (*texture_name != 0u)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(texture_name);
    }
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
    {
        return FALSE;
    }
    ndsRendererHardwareBindTextureState(name);
    while (ndsRendererHardwareFencedGlTexImage2D(
               GL_TEXTURE_2D, 0, (int)texture_format, size_x, size_y, 0,
               TEXGEN_TEXCOORD, pixels) == 0)
    {
        ndsRendererHardwareFencedGlDeleteTextures(1, &name);
        name = 0;
        upload_attempts++;
        if ((upload_attempts >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT) ||
            (ndsRendererHardwareEvictTexture(NULL) == FALSE) ||
            (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0))
        {
            return FALSE;
        }
        ndsRendererHardwareBindTextureState(name);
    }
    glColorTableEXT(GL_TEXTURE_2D, 0, (int)palette_entries, 0, 0, palette);
    *texture_name = (u32)name;
    sNdsRendererHardwareBoundTextureName = (u32)name;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;
#else
    (void)width;
    (void)height;
    (void)texture_format;
    (void)palette;
    (void)palette_entries;
    (void)fill;
    (void)user_data;
    (void)texture_name;
    return FALSE;
#endif
}

s32 ndsRendererHardwarePrepareIFCommonCloudAtlas(
    u32 width, u32 height, const u16 palette[8],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name)
{
    return ndsRendererHardwarePrepareIFCommonAtlas(
        width, height, GL_RGB8_A5, palette, 8u,
        fill, user_data, texture_name);
}

s32 ndsRendererHardwarePrepareIFCommonA3I5Atlas(
    u32 width, u32 height, const u16 palette[32],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name)
{
    return ndsRendererHardwarePrepareIFCommonAtlas(
        width, height, GL_RGB32_A3, palette, 32u,
        fill, user_data, texture_name);
}

void ndsRendererHardwareReleaseIFCommonCloudAtlas(u32 *texture_name)
{
#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    int name;

    if ((texture_name == NULL) || (*texture_name == 0u))
    {
        return;
    }
    name = (int)*texture_name;
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareFencedGlDeleteTextures(1, &name);
    if (sNdsRendererHardwareBoundTextureName == *texture_name)
    {
        sNdsRendererHardwareBoundTextureName = 0u;
    }
    sNdsRendererHardwareActiveTextureEntry = NULL;
    *texture_name = 0u;
#else
    if (texture_name != NULL)
    {
        *texture_name = 0u;
    }
#endif
}

static v16 ndsRendererHardwareIFCommonScreenX(s32 pixel_q16)
{
    s64 scaled = (s64)pixel_q16 * 32;
    s64 rounded = (scaled >= 0) ? (scaled + 0x8000) >> 16 :
                                     -(((-scaled) + 0x8000) >> 16);

    rounded -= 4096;
    if (rounded < -32768)
    {
        rounded = -32768;
    }
    else if (rounded > 32767)
    {
        rounded = 32767;
    }
    return (v16)rounded;
}

static v16 ndsRendererHardwareIFCommonScreenY(s32 pixel_q16)
{
    s64 scaled = (s64)pixel_q16 * 128;
    const s64 denominator = 3 * 65536;
    s64 rounded = (scaled >= 0) ?
        (scaled + (denominator / 2)) / denominator :
        -(((-scaled) + (denominator / 2)) / denominator);

    rounded = 4096 - rounded;
    if (rounded < -32768)
    {
        rounded = -32768;
    }
    else if (rounded > 32767)
    {
        rounded = 32767;
    }
    return (v16)rounded;
}

s32 ndsRendererHardwareDrawIFCommonCloudAtlas(
    u32 texture_name, s32 x_q16, s32 y_q16,
    s32 width_q16, s32 height_q16,
    u32 texture_x, u32 texture_y, u32 texture_width,
    u32 texture_height, u32 poly_id)
{
#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    NDSRendererIFCommonCloudDraw *draw;

    if ((texture_name == 0u) || (width_q16 <= 0) || (height_q16 <= 0) ||
        (texture_width == 0u) || (texture_height == 0u) ||
        (texture_x + texture_width > 256u) ||
        (texture_y + texture_height > 128u) || (poly_id > 63u) ||
        (sNdsRendererIFCommonCloudQueueCount >=
         NDS_RENDERER_IFCOMMON_CLOUD_QUEUE_COUNT))
    {
        return FALSE;
    }
    draw = &sNdsRendererIFCommonCloudQueue[
        sNdsRendererIFCommonCloudQueueCount++];
    draw->texture_name = texture_name;
    draw->x_q16 = x_q16;
    draw->y_q16 = y_q16;
    draw->width_q16 = width_q16;
    draw->height_q16 = height_q16;
    draw->texture_x = texture_x;
    draw->texture_y = texture_y;
    draw->texture_width = texture_width;
    draw->texture_height = texture_height;
    draw->poly_id = poly_id;
    gNdsRendererIFCommonCloudQueuedCount++;
    return TRUE;
#else
    (void)texture_name;
    (void)x_q16;
    (void)y_q16;
    (void)width_q16;
    (void)height_q16;
    (void)texture_x;
    (void)texture_y;
    (void)texture_width;
    (void)texture_height;
    (void)poly_id;
    return FALSE;
#endif
}

static void ndsRendererHardwareEmitIFCommonClouds(void)
{
#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    u32 draw_index;

    if (sNdsRendererIFCommonCloudQueueCount == 0u)
    {
        return;
    }

    /* Emit IFCommon traffic, Contour, and Light quads at the final renderer
     * boundary in the order queued by the source pass. */
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glColor(RGB15(31, 31, 31));
    for (draw_index = 0u;
         draw_index < sNdsRendererIFCommonCloudQueueCount; draw_index++)
    {
        const NDSRendererIFCommonCloudDraw *draw =
            &sNdsRendererIFCommonCloudQueue[draw_index];
        v16 left = ndsRendererHardwareIFCommonScreenX(draw->x_q16);
        v16 right = ndsRendererHardwareIFCommonScreenX(
            draw->x_q16 + draw->width_q16);
        v16 top = ndsRendererHardwareIFCommonScreenY(draw->y_q16);
        v16 bottom = ndsRendererHardwareIFCommonScreenY(
            draw->y_q16 + draw->height_q16);
        t16 tex_left = (t16)(draw->texture_x << 4);
        t16 tex_right = (t16)((draw->texture_x + draw->texture_width) << 4);
        t16 tex_top = (t16)(draw->texture_y << 4);
        t16 tex_bottom = (t16)((draw->texture_y + draw->texture_height) << 4);
        /* Opaque A3 texels update depth on DS. Give each later source SObj a
         * one-step-nearer depth so frame, lamps, contour, and Light compose in
         * painter order instead of the first quad masking every successor. */
        v16 depth = (v16)(-4080 - (s32)draw_index);

        ndsRendererHardwareBindTextureState((int)draw->texture_name);
        ndsRendererHardwareSetPolyFmt(
            POLY_CULL_NONE | POLY_ALPHA(31) | POLY_ID(draw->poly_id));
        glBegin(GL_TRIANGLE);
#define NDS_IFCOMMON_CLOUD_VERTEX(s, t, x, y) do { \
    glTexCoord2t16((s), (t)); \
    glVertex3v16((x), (y), depth); \
} while (0)
        NDS_IFCOMMON_CLOUD_VERTEX(tex_left, tex_top, left, top);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_left, tex_bottom, left, bottom);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_right, tex_top, right, top);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_right, tex_top, right, top);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_left, tex_bottom, left, bottom);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_right, tex_bottom, right, bottom);
#undef NDS_IFCOMMON_CLOUD_VERTEX
        gNdsRendererIFCommonCloudEmittedCount++;
    }
    sNdsRendererHardwareBoundTextureName =
        sNdsRendererIFCommonCloudQueue[
            sNdsRendererIFCommonCloudQueueCount - 1u].texture_name;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    sNdsRendererHardwareMatrixGeneration = 0u;
    sNdsRendererHardwareSubmitted = TRUE;
    sNdsRendererIFCommonCloudQueueCount = 0u;
#endif
}

static u32 ndsRendererHardwareTextureNextPow2(u32 value)
{
    u32 out = 8u;

    while ((out < value) && (out < NDS_RENDERER_HW_TEXTURE_MAX_WIDTH))
    {
        out <<= 1;
    }
    return out;
}

static s32 ndsRendererHardwareTextureMaskedClampNeedsWrap(
    u32 mode, u32 mask, u32 upload_extent, u32 tile_extent)
{
    u32 mask_extent;
    u32 sampler_extent;

    /* RDP mask repeat/mirror can occur before the logical tile clamp edge. */
    if (((mode & NDS_RENDERER_TX_CLAMP) == 0u) || (mask == 0u) ||
        (mask >= 31u) || (upload_extent == 0u) || (tile_extent == 0u))
    {
        return FALSE;
    }
    mask_extent = 1u << mask;
    if (upload_extent != mask_extent)
    {
        return FALSE;
    }
    sampler_extent = upload_extent;
    if ((mode & NDS_RENDERER_TX_MIRROR) != 0u)
    {
        sampler_extent <<= 1;
    }
    return (((mode & NDS_RENDERER_TX_MIRROR) != 0u) ||
            (sampler_extent != tile_extent)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareTextureMaterializesMaskedClamp(
    u32 mode, u32 mask, u32 source_extent, u32 tile_extent)
{
    u32 mask_extent;

    if (((mode & NDS_RENDERER_TX_CLAMP) == 0u) || (mask == 0u) ||
        (mask >= 31u) || (source_extent == 0u) ||
        (tile_extent > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH))
    {
        return FALSE;
    }
    mask_extent = 1u << mask;
    return ((tile_extent > mask_extent) &&
            (source_extent >= mask_extent) &&
            (source_extent <= tile_extent)) ? TRUE : FALSE;
}

static u32 ndsRendererHardwareTextureMaskedAddress(
    u32 coord, u32 mode, u32 mask)
{
    u32 mask_extent = 1u << mask;
    u32 period = coord >> mask;
    u32 local = coord & (mask_extent - 1u);

    if (((mode & NDS_RENDERER_TX_MIRROR) != 0u) &&
        ((period & 1u) != 0u))
    {
        local = mask_extent - 1u - local;
    }
    return local;
}

static u32 ndsRendererHardwareTextureParams(
    const NDSRendererStats *stats,
    const NDSRendererTileState *render_tile,
    u32 upload_width,
    u32 upload_height)
{
    u32 params;
    s32 wrap_s;
    s32 wrap_t;

    if (render_tile == NULL)
    {
        return TEXGEN_OFF;
    }

    params = (ndsRendererHardwareUseTextureMatrix(stats) != FALSE) ?
        TEXGEN_TEXCOORD : TEXGEN_OFF;
    wrap_s = ((render_tile->cms & NDS_RENDERER_TX_CLAMP) == 0u) ||
        ndsRendererHardwareTextureMaskedClampNeedsWrap(
            render_tile->cms, render_tile->masks, upload_width,
            render_tile->width);
    wrap_t = ((render_tile->cmt & NDS_RENDERER_TX_CLAMP) == 0u) ||
        ndsRendererHardwareTextureMaskedClampNeedsWrap(
            render_tile->cmt, render_tile->maskt, upload_height,
            render_tile->height);
    if (wrap_s != FALSE)
    {
        params |= GL_TEXTURE_WRAP_S;
    }
    if ((wrap_s != FALSE) &&
        ((render_tile->cms & NDS_RENDERER_TX_MIRROR) != 0u))
    {
        params |= GL_TEXTURE_FLIP_S;
    }
    if (wrap_t != FALSE)
    {
        params |= GL_TEXTURE_WRAP_T;
    }
    if ((wrap_t != FALSE) &&
        ((render_tile->cmt & NDS_RENDERER_TX_MIRROR) != 0u))
    {
        params |= GL_TEXTURE_FLIP_T;
    }
    return params;
}

static u32 ndsRendererHardwareMergeTextureParams(u32 params)
{
    u32 current = (u32)glGetTexParameter();

    current &= ~NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK;
    current |= params & NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK;
    return current;
}

static void ndsRendererHardwareApplyTextureParams(u32 params)
{
    if (((sNdsRendererGXStateShadow.valid_mask &
          NDS_RENDERER_GX_STATE_TEXTURE_PARAMS) != 0u) &&
        (sNdsRendererGXStateShadow.texture_params == params))
    {
#if NDS_RENDERER_M3_PHASE0_PROFILE
        gNdsRendererM3G2TextureParamSkipCount++;
#endif
        return;
    }
    glTexParameter(GL_TEXTURE_2D, (int)params);
    sNdsRendererGXStateShadow.texture_params = params;
    sNdsRendererGXStateShadow.valid_mask |=
        NDS_RENDERER_GX_STATE_TEXTURE_PARAMS;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererM3G2TextureParamWriteCount++;
#endif
}

#define NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT 128u

typedef struct NDSRendererStageTextureSite
{
    const Gfx *site;
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 state_hash1;
    u32 state_hash2;
    u32 entry_generation;
    u32 format;
    u32 size;
    u32 width;
    u32 height;
    u32 uses_texel1;
    u32 prim_lod_fraction;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 semantic_key_hash;
    u32 semantic_params;
    u32 texel1_tile_state;
    u32 texel1_primary_state;
    u32 texel1_image0;
    u32 texel1_image1;
#endif
} NDSRendererStageTextureSite;

static NDSRendererStageTextureSite
    sNdsRendererStageTextureSites[NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT];
static u32 sNdsRendererStageTextureSiteNext;

_Static_assert(
    sizeof(sNdsRendererStageTextureSites) <= (12u * 1024u),
    "stage texture-site plans must stay below 12 KiB");

static void ndsRendererHardwareBindTextureName(
    NDSRendererStats *stats,
    u32 texture_name);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererProfileTextureCacheEntry(
    const NDSRendererHardwareTextureCacheEntry *entry);
static void ndsRendererProfileTexturePixel(u16 color, u32 *green_texels,
                                           u32 *nonwhite_texels);
static void ndsRendererProfileTextureFormat(
    volatile u32 *mask,
    u32 format,
    u32 size);
static void ndsRendererRecordTextureLaneUse(
    const NDSRendererConfig *config,
    u32 format,
    u32 size);
#endif

static u32 ndsRendererStageTextureSiteSlot(const Gfx *site)
{
    u32 value = (u32)(uintptr_t)site;

    value ^= value >> 11;
    value ^= value >> 19;
    return value & (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
}

static NDSRendererStageTextureSite *ndsRendererStageTextureSiteFind(
    const NDSRendererTraversalState *state,
    const NDSRendererStats *stats)
{
    u32 slot;
    u32 probe;

    if ((state == NULL) || (stats == NULL) ||
        (state->source_command_site == NULL))
    {
        return NULL;
    }
    slot = ndsRendererStageTextureSiteSlot(state->source_command_site);
    for (probe = 0u; probe < NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT; probe++)
    {
        NDSRendererStageTextureSite *plan =
            &sNdsRendererStageTextureSites[slot];

        if (plan->site == NULL)
        {
            return NULL;
        }
        if ((plan->site == state->source_command_site) &&
            (plan->state_hash1 == stats->texture_source_hash1) &&
            (plan->state_hash2 == stats->texture_source_hash2))
        {
            return plan;
        }
        slot = (slot + 1u) &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
    }
    return NULL;
}

static s32 ndsRendererStageTextureSiteTryBind(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    const NDSRendererConfig *config)
{
    NDSRendererStageTextureSite *plan;
    NDSRendererHardwareTextureCacheEntry *entry;

    if (sNdsRendererStageTextureSitesEnabled == 0u)
    {
        return FALSE;
    }
    plan = ndsRendererStageTextureSiteFind(state, stats);
    if (plan == NULL)
    {
        return FALSE;
    }
    entry = plan->entry;
    if ((entry == NULL) || (entry->ready == 0u) ||
        (entry->key_generation != plan->entry_generation) ||
        ((plan->uses_texel1 != 0u) &&
         (plan->prim_lod_fraction != stats->prim_lod_fraction)))
    {
        return FALSE;
    }

    entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    if (entry->pinned != 0u)
    {
        ndsRendererHardwareRecordBattleStaticTextureHit(entry);
    }
    if (sNdsRendererHardwareActiveTextureEntry != entry)
    {
        ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        ndsRendererHardwareApplyTextureParams(entry->params);
        sNdsRendererHardwareActiveTextureEntry = entry;
    }
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = plan->format;
    stats->hardware_texture_width = plan->width;
    stats->hardware_texture_height = plan->height;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererSemanticLastTextureKeyHash = plan->semantic_key_hash;
    sNdsRendererSemanticLastTextureParams = plan->semantic_params;
    ndsRendererRecordTextureLaneUse(config, plan->format, plan->size);
    if (plan->uses_texel1 != 0u)
    {
        ndsRendererRecordTextureLaneUse(config, plan->format, plan->size);
        ndsRendererProfileRecordTexel1Composite();
        gNdsRendererProfileTexel1LastTileState = plan->texel1_tile_state;
        gNdsRendererProfileTexel1LastPrimaryState =
            plan->texel1_primary_state;
        gNdsRendererProfileTexel1LastFraction = plan->prim_lod_fraction;
        gNdsRendererProfileTexel1LastImage0 = plan->texel1_image0;
        gNdsRendererProfileTexel1LastImage1 = plan->texel1_image1;
    }
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureBindFormatMask,
        plan->format, plan->size);
    ndsRendererProfileTextureCacheEntry(entry);
#endif
    return TRUE;
}

static void ndsRendererStageTextureSiteRemember(
    const NDSRendererTraversalState *state,
    const NDSRendererStats *stats,
    NDSRendererHardwareTextureCacheEntry *entry,
    u32 format,
    u32 size,
    u32 width,
    u32 height)
{
    NDSRendererStageTextureSite *plan = NULL;
    NDSRendererStageTextureSite *empty = NULL;
    u32 slot;
    u32 probe;

    if ((sNdsRendererStageTextureSitesEnabled == 0u) ||
        (state == NULL) || (state->source_command_site == NULL) ||
        (stats == NULL) || (entry == NULL) || (entry->ready == 0u))
    {
        return;
    }
    slot = ndsRendererStageTextureSiteSlot(state->source_command_site);
    for (probe = 0u; probe < NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT; probe++)
    {
        NDSRendererStageTextureSite *candidate =
            &sNdsRendererStageTextureSites[slot];

        if (candidate->site == state->source_command_site)
        {
            plan = candidate;
            break;
        }
        if ((empty == NULL) && (candidate->site == NULL))
        {
            empty = candidate;
        }
        slot = (slot + 1u) &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
    }
    if (plan == NULL)
    {
        plan = empty;
    }
    if (plan == NULL)
    {
        plan = &sNdsRendererStageTextureSites[
            sNdsRendererStageTextureSiteNext++ &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u)];
    }
    plan->site = state->source_command_site;
    plan->entry = entry;
    plan->state_hash1 = stats->texture_source_hash1;
    plan->state_hash2 = stats->texture_source_hash2;
    plan->entry_generation = entry->key_generation;
    plan->format = format;
    plan->size = size;
    plan->width = width;
    plan->height = height;
    plan->uses_texel1 = (entry->key.texel1_image != 0u) ? TRUE : FALSE;
    plan->prim_lod_fraction = entry->key.prim_lod_fraction;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    plan->semantic_key_hash = sNdsRendererSemanticLastTextureKeyHash;
    plan->semantic_params = sNdsRendererSemanticLastTextureParams;
    plan->texel1_tile_state = gNdsRendererProfileTexel1LastTileState;
    plan->texel1_primary_state = gNdsRendererProfileTexel1LastPrimaryState;
    plan->texel1_image0 = gNdsRendererProfileTexel1LastImage0;
    plan->texel1_image1 = gNdsRendererProfileTexel1LastImage1;
#endif
}

static void ndsRendererTextureSourceHashCommand(
    NDSRendererStats *stats,
    u32 w0,
    u32 w1)
{
    u32 hash1;
    u32 hash2;

    if ((stats == NULL) ||
        (sNdsRendererRuntimeOwner != NDS_RENDERER_PROFILE_OWNER_STAGE))
    {
        return;
    }

    hash1 = stats->texture_source_hash1;
    hash1 ^= w0 ^ ((w1 << 16) | (w1 >> 16));
    hash1 *= 16777619u;
    stats->texture_source_hash1 = hash1;

    hash2 = stats->texture_source_hash2 + w1 + 0x9e3779b9u;
    hash2 = (hash2 << 7) | (hash2 >> 25);
    stats->texture_source_hash2 = hash2 ^ w0 ^ 0x85ebca6bu;
}

static u32 ndsRendererHardwareAlphaStateKey(const NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return 0u;
    }
    return (stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) |
           ((stats->blend_color & 0xffu) << 8);
}

static u32 ndsRendererHardwareFogStateKey(const NDSRendererStats *stats)
{
    if ((stats == NULL) ||
        (stats->fog_status == 0u) ||
        (stats->fog_max <= stats->fog_min))
    {
        return 0u;
    }
    return ((u32)stats->fog_min & 0x3ffu) |
           (((u32)stats->fog_max & 0x3ffu) << 10) |
           ((stats->fog_color & 0xfffu) << 20);
}

static void ndsRendererHardwareBindTextureName(
    NDSRendererStats *stats,
    u32 texture_name)
{
    if (texture_name == 0u)
    {
        return;
    }
    if (sNdsRendererHardwareBoundTextureName != texture_name)
    {
        ndsRendererHardwareEndBatch();
        ndsRendererHardwareBindTextureState((int)texture_name);
        sNdsRendererHardwareBoundTextureName = texture_name;
        ndsRendererProfileRecordTextureBind();
        if (stats != NULL)
        {
            stats->hardware_texture_bind_count++;
        }
    }
}

static void ndsRendererHardwareReleaseBattleStaticTextureEntries(void)
{
    u32 i;

    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];

        if (entry->pinned != 0u)
        {
            (void)ndsRendererHardwareReleaseTexture(entry);
        }
    }
}

s32 ndsRendererHardwarePrepareBattleStaticTextures(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    FILE *file = NULL;
    long payload_size;
    u32 record_index;

    if (gNdsRendererBattleStaticTextureEnabled == 0u)
    {
        return FALSE;
    }
    if (sNdsRendererBattleStaticTexturePrepared != 0u)
    {
        return TRUE;
    }

    gNdsRendererBattleStaticTexturePrepareCount++;
    gNdsRendererBattleStaticTexturePrepareFailCount = 0u;
    gNdsRendererBattleStaticTexturePreparedCount = 0u;
    gNdsRendererBattleStaticTexturePreparedBytes = 0u;
    gNdsRendererBattleStaticTextureArmCount = 0u;
    gNdsRendererBattleStaticTexturePinnedHitCount = 0u;
    gNdsRendererBattleStaticTextureSeenMask = 0u;
    gNdsRendererBattleStaticTextureOwnerMask = 0u;
    gNdsRendererBattleStaticTextureViolationCount = 0u;
    gNdsRendererBattleStaticTextureTeardownCount = 0u;
    gNdsRendererBattleStaticTextureFirstAddress = 0u;
    gNdsRendererBattleStaticTextureEndAddress = 0u;
    gNdsRendererBattleStaticTextureAllocationSpanBytes = 0u;
    gNdsRendererBattleStaticTextureBankMask = 0u;
    sNdsRendererBattleStaticTextureArmed = FALSE;

    /* A battle owns the cache from this point forward. Starting empty makes
     * the generated residency and its exact VRAM span deterministic. */
    ndsRendererHardwareDiscardTextureCache();
    file = ndsRendererHardwareFencedTextureFopen(
        NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_PAYLOAD_PATH, "rb");
    if (file == NULL)
    {
        goto fail;
    }
    if ((ndsRendererHardwareFencedTextureFseek(file, 0, SEEK_END) != 0) ||
        ((payload_size = ndsRendererHardwareFencedTextureFtell(file)) < 0) ||
        ((u32)payload_size !=
         ndsBattlePlayableStaticTexturePayloadBytes()) ||
        (ndsRendererHardwareFencedTextureFseek(file, 0, SEEK_SET) != 0))
    {
        goto fail;
    }

    for (record_index = 0u;
         record_index < ndsBattlePlayableStaticTextureKeyCount();
         record_index++)
    {
        const NDSBattlePlayableStaticTextureRecord *record =
            ndsBattlePlayableStaticTextureRecordAt(record_index);
        const void *image_base;
        const void *tlut_base;
        u32 image_size;
        u32 tlut_size;
        u32 texel1_offset;
        NDSRendererHardwareTextureKey key;
        NDSRendererHardwareTextureCacheEntry *entry;
        u32 key_hash;
        int size_x;
        int size_y;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        u32 green_texels = 0u;
        u32 nonwhite_texels = 0u;
        u32 x;
        u32 y;
#endif

        if ((record == NULL) || (record->reserved != 0u) ||
            (record->payload_bytes == 0u) ||
            (record->payload_bytes >
             sizeof(sNdsRendererHardwareTextureScratch)) ||
            (record->payload_offset >
             ndsBattlePlayableStaticTexturePayloadBytes()) ||
            (record->payload_bytes >
             ndsBattlePlayableStaticTexturePayloadBytes() -
                 record->payload_offset) ||
            (record->payload_bytes !=
             (u32)record->upload_width * (u32)record->upload_height *
                 sizeof(u16)) ||
            (ndsRendererHardwareTextureSizeEnum(
                 record->upload_width, &size_x) == FALSE) ||
            (ndsRendererHardwareTextureSizeEnum(
                 record->upload_height, &size_y) == FALSE) ||
            (ndsRelocGetLoadedAssetView(
                 record->image_asset_id, &image_base, &image_size) == FALSE) ||
            (ndsRelocGetLoadedAssetView(
                 record->tlut_asset_id, &tlut_base, &tlut_size) == FALSE) ||
            (record->image_offset >= image_size) ||
            (record->tlut_offset >= tlut_size) ||
            ((uintptr_t)image_base >
             (uintptr_t)(0xffffffffu - record->image_offset)) ||
            ((uintptr_t)tlut_base >
             (uintptr_t)(0xffffffffu - record->tlut_offset)) ||
            (record->key_words[
                 NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD] !=
             record->image_offset) ||
            (record->key_words[
                 NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD] !=
             record->tlut_offset) ||
            ((texel1_offset = record->key_words[
                  NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD]) != 0u &&
             ((texel1_offset >= image_size) ||
              ((uintptr_t)image_base >
               (uintptr_t)(0xffffffffu - texel1_offset)))))
        {
            goto fail;
        }

        memcpy(&key, record->key_words, sizeof(key));
        key.image = (u32)(uintptr_t)((const u8 *)image_base +
                                     record->image_offset);
        key.tlut_image = (u32)(uintptr_t)((const u8 *)tlut_base +
                                          record->tlut_offset);
        key.texel1_image = (texel1_offset != 0u) ?
            (u32)(uintptr_t)((const u8 *)image_base + texel1_offset) : 0u;
        if ((key.width != record->logical_width) ||
            (key.height != record->logical_height))
        {
            goto fail;
        }

#if NDS_RENDERER_PROFILE_LEVEL < 2
        key_hash = ndsRendererHardwareTextureKeyHash(&key);
#else
        key_hash = 0u;
#endif
        if (ndsRendererHardwareFindTexture(&key, key_hash) != NULL)
        {
            goto fail;
        }
        if ((ndsRendererHardwareFencedTextureFseek(
                 file, (long)record->payload_offset, SEEK_SET) != 0) ||
            (ndsRendererHardwareFencedTextureFread(
                 sNdsRendererHardwareTextureScratch, 1,
                 record->payload_bytes, file) != record->payload_bytes))
        {
            goto fail;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        for (y = 0u; y < record->logical_height; y++)
        {
            for (x = 0u; x < record->logical_width; x++)
            {
                ndsRendererProfileTexturePixel(
                    sNdsRendererHardwareTextureScratch[
                        (y * record->upload_width) + x],
                    &green_texels, &nonwhite_texels);
            }
        }
#endif

        entry = ndsRendererHardwareAllocTexture();
        if (entry == NULL)
        {
            goto fail;
        }
        if ((entry->name == 0) &&
            (ndsRendererHardwareFencedGlGenTextures(
                 1, &entry->name) == 0))
        {
            goto fail;
        }
        ndsRendererHardwareBindTextureName(NULL, (u32)entry->name);
        if (ndsRendererHardwareFencedGlTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA, size_x, size_y, 0,
                TEXGEN_TEXCOORD,
                sNdsRendererHardwareTextureScratch) == 0)
        {
            (void)ndsRendererHardwareReleaseTexture(entry);
            goto fail;
        }
        {
            uintptr_t first = (uintptr_t)glGetTexturePointer(entry->name);
            uintptr_t end = first + record->payload_bytes;

            if ((first == 0u) || (end <= first) ||
                (end > (uintptr_t)0xffffffffu))
            {
                (void)ndsRendererHardwareReleaseTexture(entry);
                goto fail;
            }
            if ((gNdsRendererBattleStaticTextureFirstAddress == 0u) ||
                (first < gNdsRendererBattleStaticTextureFirstAddress))
            {
                gNdsRendererBattleStaticTextureFirstAddress = (u32)first;
            }
            if (end > gNdsRendererBattleStaticTextureEndAddress)
            {
                gNdsRendererBattleStaticTextureEndAddress = (u32)end;
            }
            if ((first < (uintptr_t)VRAM_B) &&
                (end > (uintptr_t)VRAM_A))
            {
                gNdsRendererBattleStaticTextureBankMask |= 1u << 0;
            }
            if ((first < (uintptr_t)VRAM_C) &&
                (end > (uintptr_t)VRAM_B))
            {
                gNdsRendererBattleStaticTextureBankMask |= 1u << 1;
            }
        }

        entry->key = key;
        sNdsRendererHardwareTextureKeyGeneration++;
        if (sNdsRendererHardwareTextureKeyGeneration == 0u)
        {
            sNdsRendererHardwareTextureKeyGeneration++;
        }
        entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        entry->key_hash = key_hash;
#endif
        entry->params = (u32)glGetTexParameter();
        entry->source_texels = (u32)record->logical_width *
            (u32)record->logical_height;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        entry->green_texels = green_texels;
        entry->nonwhite_texels = nonwhite_texels;
#endif
        entry->profile_width = record->upload_width;
        entry->profile_height = record->upload_height;
        entry->last_used_frame = 0u;
        entry->pinned = TRUE;
        entry->static_record_plus1 = record_index + 1u;
        entry->static_owner_mask = record->owner_mask;
        entry->ready = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ndsRendererHardwareTextureLookupInsert(entry);
#endif
        gNdsRendererBattleStaticTexturePreparedCount++;
        gNdsRendererBattleStaticTexturePreparedBytes +=
            record->payload_bytes;
    }

    if (ndsRendererHardwareFencedTextureFclose(file) != 0)
    {
        file = NULL;
        goto fail;
    }
    file = NULL;
    if ((gNdsRendererBattleStaticTexturePreparedCount !=
         ndsBattlePlayableStaticTextureKeyCount()) ||
        (gNdsRendererBattleStaticTexturePreparedBytes !=
         ndsBattlePlayableStaticTexturePreparedBytes()) ||
        ((gNdsRendererBattleStaticTextureAllocationSpanBytes =
          gNdsRendererBattleStaticTextureEndAddress -
          gNdsRendererBattleStaticTextureFirstAddress) !=
         ndsBattlePlayableStaticTexturePreparedBytes()) ||
        (gNdsRendererBattleStaticTextureFirstAddress != (u32)VRAM_A) ||
        (gNdsRendererBattleStaticTextureEndAddress !=
         ((u32)VRAM_A + ndsBattlePlayableStaticTexturePreparedBytes())) ||
        (gNdsRendererBattleStaticTextureBankMask != 3u))
    {
        goto fail;
    }
    sNdsRendererBattleStaticTexturePrepared = TRUE;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;

fail:
    if (file != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(file);
    }
    ndsRendererHardwareReleaseBattleStaticTextureEntries();
    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
    gNdsRendererBattleStaticTexturePreparedCount = 0u;
    gNdsRendererBattleStaticTexturePreparedBytes = 0u;
    gNdsRendererBattleStaticTextureFirstAddress = 0u;
    gNdsRendererBattleStaticTextureEndAddress = 0u;
    gNdsRendererBattleStaticTextureAllocationSpanBytes = 0u;
    gNdsRendererBattleStaticTextureBankMask = 0u;
    gNdsRendererBattleStaticTexturePrepareFailCount++;
    return FALSE;
#else
    if (gNdsRendererBattleStaticTextureEnabled != 0u)
    {
        gNdsRendererBattleStaticTexturePrepareCount++;
        gNdsRendererBattleStaticTexturePrepareFailCount++;
    }
    return FALSE;
#endif
}

void ndsRendererHardwareArmBattleStaticTextures(void)
{
    if ((gNdsRendererBattleStaticTextureEnabled != 0u) &&
        (sNdsRendererBattleStaticTexturePrepared != 0u) &&
        (sNdsRendererBattleStaticTextureArmed == 0u))
    {
        memset((void *)gNdsRendererBattleTextureFenceCounts, 0,
               sizeof(gNdsRendererBattleTextureFenceCounts));
        gNdsRendererBattleTextureFenceFirstClassPlus1 = 0u;
        gNdsRendererBattleTextureFenceFirstFrame = 0u;
        sNdsRendererBattleStaticTextureArmed = TRUE;
        gNdsRendererBattleStaticTextureArmCount++;
    }
}

void ndsRendererHardwareDiscardBattleStaticTextures(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 was_prepared = sNdsRendererBattleStaticTexturePrepared;

    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
    ndsRendererHardwareDiscardTextureCache();
    if (was_prepared != 0u)
    {
        gNdsRendererBattleStaticTextureTeardownCount++;
    }
#endif
    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
}

void ndsRendererHardwareAbortBattleStaticTextures(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 was_prepared = sNdsRendererBattleStaticTexturePrepared;

    /* This deliberately retains the armed bit through every pinned delete.
     * DiscardTextureCache records the violation and clears the bit last. */
    ndsRendererHardwareDiscardTextureCache();
    if (was_prepared != 0u)
    {
        gNdsRendererBattleStaticTextureTeardownCount++;
    }
#endif
    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
}

static void ndsRendererHardwareBindNoTexture(NDSRendererStats *stats)
{
    if (sNdsRendererHardwareNoTextureName == 0)
    {
        ndsRendererHardwareEndBatch();
        if (ndsRendererHardwareFencedGlGenTextures(
                1, &sNdsRendererHardwareNoTextureName) == 0)
        {
            return;
        }
        ndsRendererHardwareBindTextureState(
            sNdsRendererHardwareNoTextureName);
        sNdsRendererHardwareBoundTextureName =
            (u32)sNdsRendererHardwareNoTextureName;
        ndsRendererHardwareFencedGlTexImage2D(
            GL_TEXTURE_2D, 0, GL_NOTEXTURE, 0, 0, 0,
            TEXGEN_TEXCOORD, NULL);
    }
    else
    {
        ndsRendererHardwareBindTextureName(
            stats, (u32)sNdsRendererHardwareNoTextureName);
    }
    sNdsRendererHardwareActiveTextureEntry = NULL;
}

static u16 ndsRendererHardwareConvertRgba16(u16 n64_color,
                                            s32 preserve_transparent_rgb)
{
    u16 red;
    u16 green;
    u16 blue;
    u16 alpha;

    if (((n64_color & 1u) == 0u) &&
        (preserve_transparent_rgb == FALSE))
    {
        return 0u;
    }

    red = (u16)((n64_color >> 11) & 0x1fu);
    green = (u16)((n64_color >> 6) & 0x1fu);
    blue = (u16)((n64_color >> 1) & 0x1fu);
    alpha = (n64_color & 1u) ? (1u << 15) : 0u;
    return (u16)(alpha | red | (green << 5) | (blue << 10));
}

static u16 ndsRendererHardwareConvertRgba32(u32 rgba)
{
    u8 red = (u8)(rgba >> 24);
    u8 green = (u8)(rgba >> 16);
    u8 blue = (u8)(rgba >> 8);
    u8 alpha = (u8)rgba;

    if (alpha == 0u)
    {
        return 0u;
    }
    return (u16)((1u << 15) |
                 ((u16)(red >> 3)) |
                 ((u16)(green >> 3) << 5) |
                 ((u16)(blue >> 3) << 10));
}

static u16 ndsRendererHardwareConvertI(u8 intensity)
{
    u16 v;

    v = (u16)(intensity >> 3);
    return (u16)((1u << 15) | v | (v << 5) | (v << 10));
}

static u16 ndsRendererHardwareConvertI16(u16 value)
{
    u8 intensity = (u8)(value >> 8);

    if (intensity == 0u)
    {
        intensity = (u8)value;
    }
    return ndsRendererHardwareConvertI(intensity);
}

static u16 ndsRendererHardwareConvertIA(u8 intensity, u8 alpha)
{
    u16 v;

    if (alpha == 0u)
    {
        return 0u;
    }
    v = (u16)(intensity >> 3);
    return (u16)((1u << 15) | v | (v << 5) | (v << 10));
}

static u32 ndsRendererHardwareTextureLinePixels(u32 size, u32 line)
{
    switch (size)
    {
    case NDS_RENDERER_HW_TEXTURE_SIZ_4B:
        return line * 16u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_8B:
        return line * 8u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_16B:
        return line * 4u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_32B:
        return line * 2u;
    default:
        return 0u;
    }
}

static u32 ndsRendererHardwareTextureSourceBytes(u32 format, u32 size,
                                                 u32 texels)
{
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_RGBA16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            return texels * sizeof(u16);
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
            texels * sizeof(u32) : 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_I16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ?
            texels * sizeof(u16) : 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_IA)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ?
            texels * sizeof(u16) : 0u;
    }
    return 0u;
}

static u32 ndsRendererHardwareTextureSourceWidthPixels(u32 render_size,
                                                       u32 image_size,
                                                       u32 image_width)
{
    if ((render_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (image_size == NDS_RENDERER_HW_TEXTURE_SIZ_8B))
    {
        return image_width * 2u;
    }
    return image_width;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererHardwareTextureFormatBit(u32 format, u32 size);
#endif

static NDSRendererTextureDataLayout ndsRendererTextureDataLayout(
    const NDSRendererConfig *config)
{
    if (config == NULL)
    {
        return NDS_RENDERER_TEXTURE_DATA_NATIVE;
    }
    if (config->texture_data_layout ==
        NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED)
    {
        return NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    }
    return NDS_RENDERER_TEXTURE_DATA_NATIVE;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererTexturePackMap(
    NDSRendererTextureDataLayout layout, u32 stride)
{
    u32 map = 0u;
    u32 i;

    for (i = 0u; i < 4u; i++)
    {
        u32 physical = i;

        if (layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED)
        {
            physical = i ^ stride;
        }
        map |= (physical & 0xffu) << (i * 8u);
    }
    return map;
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererRecordTextureLaneCount(
    NDSRendererTextureDataLayout layout, u32 is_halfword,
    u32 format, u32 size, u32 count)
{
    u32 format_bit = ndsRendererHardwareTextureFormatBit(format, size);

    gNdsRendererProfileTextureLaneLayoutMask |= 1u << (u32)layout;
    if (is_halfword != 0u)
    {
        gNdsRendererProfileTextureLaneHalfwordAccessCount += count;
        gNdsRendererProfileTextureLaneHalfwordFormatMask |= format_bit;
        gNdsRendererProfileTextureLaneHalfwordMap =
            ndsRendererTexturePackMap(layout, 1u);
    }
    else
    {
        gNdsRendererProfileTextureLaneByteAccessCount += count;
        gNdsRendererProfileTextureLaneByteFormatMask |= format_bit;
        gNdsRendererProfileTextureLaneByteMap =
            ndsRendererTexturePackMap(layout, 3u);
    }
}
#endif

static u32 ndsRendererTextureLogicalByteIndex(
    NDSRendererTextureDataLayout layout, u32 logical_index)
{
    return (layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ?
        (logical_index ^ 3u) : logical_index;
}

static u32 ndsRendererTextureLogicalHalfwordIndex(
    NDSRendererTextureDataLayout layout, u32 logical_index)
{
    return (layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ?
        (logical_index ^ 1u) : logical_index;
}

static u32 ndsRendererTexturePhysicalByteSpan(
    NDSRendererTextureDataLayout layout, u32 logical_bytes)
{
    if ((layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) &&
        (logical_bytes != 0u))
    {
        return (logical_bytes + 3u) & ~3u;
    }
    return logical_bytes;
}

static u8 ndsRendererReadTextureByte(
    const NDSRendererConfig *config, const u8 *texels, u32 logical_index,
    u32 format, u32 size)
{
    NDSRendererTextureDataLayout layout = ndsRendererTextureDataLayout(config);

    /* The conversion loop aggregates this access after rasterization. Do not
     * update volatile diagnostics for every converted texel: animated
     * TEXEL0/TEXEL1 water reads this path tens of thousands of times per
     * frame, while the lane contract is invariant for the conversion. */
    (void)format;
    (void)size;
    return texels[ndsRendererTextureLogicalByteIndex(layout, logical_index)];
}

static u8 ndsRendererReadTexturePackedNibble(
    const NDSRendererConfig *config, const u8 *texels, u32 logical_texel_index,
    u32 format, u32 size)
{
    u8 packed = ndsRendererReadTextureByte(
        config, texels, logical_texel_index >> 1, format, size);

    return ((logical_texel_index & 1u) == 0u) ?
        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
}

static u32 ndsRendererHardwareCiPaletteEntriesUsed(
    const NDSRendererConfig *config,
    const u8 *texels,
    u32 size,
    u32 source_width,
    u32 source_origin_s,
    u32 source_origin_t,
    u32 source_read_width,
    u32 source_read_height,
    u32 palette_base)
{
    u32 max_index = 0u;
    u32 x;
    u32 y;

    if ((texels == NULL) || (source_width == 0u) ||
        (source_read_width == 0u) || (source_read_height == 0u))
    {
        return palette_base;
    }
    for (y = 0u; y < source_read_height; y++)
    {
        u32 row = (source_origin_t + y) * source_width;

        for (x = 0u; x < source_read_width; x++)
        {
            u32 source_index = row + source_origin_s + x;
            u32 palette_index =
                (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
                    ndsRendererReadTexturePackedNibble(
                        config, texels, source_index,
                        NDS_RENDERER_HW_TEXTURE_FMT_CI, size) :
                    ndsRendererReadTextureByte(
                        config, texels, source_index,
                        NDS_RENDERER_HW_TEXTURE_FMT_CI, size);

            if (palette_index > max_index)
            {
                max_index = palette_index;
            }
        }
    }
    return palette_base + max_index + 1u;
}

static u16 ndsRendererReadTextureHalfword(
    const NDSRendererConfig *config, const u16 *data, u32 logical_index,
    u32 format, u32 size)
{
    NDSRendererTextureDataLayout layout = ndsRendererTextureDataLayout(config);

    (void)format;
    (void)size;
    return data[ndsRendererTextureLogicalHalfwordIndex(layout, logical_index)];
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererRecordTextureLaneUseCount(
    const NDSRendererConfig *config, u32 format, u32 size, u32 count)
{
    NDSRendererTextureDataLayout layout = ndsRendererTextureDataLayout(config);

    if ((size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ||
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B))
    {
        ndsRendererRecordTextureLaneCount(
            layout, FALSE, format, size, count);
    }
    else if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
    {
        ndsRendererRecordTextureLaneCount(
            layout, TRUE, format, size, count);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        ndsRendererRecordTextureLaneCount(
            layout, TRUE, format, NDS_RENDERER_HW_TEXTURE_SIZ_16B, count);
    }
}

static void ndsRendererRecordTextureLaneUse(
    const NDSRendererConfig *config, u32 format, u32 size)
{
    ndsRendererRecordTextureLaneUseCount(config, format, size, 1u);
}
#endif

static u16 ndsRendererHardwarePaletteColor(
    const NDSRendererConfig *config, const u16 *palette, u32 index, u32 count,
    s32 preserve_transparent_rgb)
{
    if ((palette == NULL) || (index >= count))
    {
        return 0u;
    }
    return ndsRendererHardwareConvertRgba16(
        ndsRendererReadTextureHalfword(
            config, palette, index, NDS_RENDERER_HW_TEXTURE_FMT_CI,
            NDS_RENDERER_HW_TEXTURE_SIZ_16B),
        preserve_transparent_rgb);
}

static u16 ndsRendererHardwareTextureColor(
    const NDSRendererConfig *config,
    u32 format,
    u32 size,
    const u8 *texels,
    const u16 *palette,
    u32 palette_count,
    u32 palette_base,
    u32 index,
    s32 preserve_transparent_rgb)
{
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        u32 palette_index;

        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            palette_index = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);
        }
        else
        {
            palette_index = ndsRendererReadTextureByte(
                config, texels, index, format, size);
        }
        palette_index += palette_base;
        return ndsRendererHardwarePaletteColor(config, palette, palette_index,
                                               palette_count,
                                               preserve_transparent_rgb);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_RGBA16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B)
        {
            u32 rgba;

            memcpy(&rgba, &texels[index * sizeof(rgba)], sizeof(rgba));
            return ndsRendererHardwareConvertRgba32(rgba);
        }
        return ndsRendererHardwareConvertRgba16(
            ndsRendererReadTextureHalfword(
                config, (const u16 *)texels, index, format, size),
            preserve_transparent_rgb);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_IA)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            u8 value = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);
            u8 intensity = (u8)(((value >> 1) & 0x07u) * 0x24u);
            u8 alpha = (value & 1u) ? 0xffu : 0u;

            return ndsRendererHardwareConvertIA(intensity, alpha);
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            u8 value = ndsRendererReadTextureByte(
                config, texels, index, format, size);
            u8 intensity = (u8)((value >> 4) * 0x11u);
            u8 alpha = (u8)((value & 0x0fu) * 0x11u);

            return ndsRendererHardwareConvertIA(intensity, alpha);
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            u16 value = ndsRendererReadTextureHalfword(
                config, (const u16 *)texels, index, format, size);

            return ndsRendererHardwareConvertIA((u8)(value >> 8),
                                                (u8)value);
        }
        return 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_I16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            u8 intensity4 = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);

            return ndsRendererHardwareConvertI((u8)(intensity4 * 0x11u));
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return ndsRendererHardwareConvertI(
                ndsRendererReadTextureByte(config, texels, index,
                                           format, size));
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            return ndsRendererHardwareConvertI16(
                ndsRendererReadTextureHalfword(
                    config, (const u16 *)texels, index, format, size));
        }
    }
    return 0u;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static s32 ndsRendererTextureColorNonWhite(u16 color)
{
    u32 r;
    u32 g;
    u32 b;

    if ((color & (1u << 15)) == 0u)
    {
        return FALSE;
    }
    r = color & 0x1fu;
    g = (color >> 5) & 0x1fu;
    b = (color >> 10) & 0x1fu;
    return ((r < 29u) || (g < 29u) || (b < 29u)) ? TRUE : FALSE;
}

static s32 ndsRendererTextureColorDominantGreen(u16 color)
{
    u32 r;
    u32 g;
    u32 b;

    if ((color & (1u << 15)) == 0u)
    {
        return FALSE;
    }
    r = color & 0x1fu;
    g = (color >> 5) & 0x1fu;
    b = (color >> 10) & 0x1fu;
    return ((g >= 10u) && (g > (r + 2u)) && (g > (b + 2u))) ?
        TRUE : FALSE;
}

static void ndsRendererProfileTexturePixel(u16 color, u32 *green_texels,
                                           u32 *nonwhite_texels)
{
    if (ndsRendererTextureColorNonWhite(color) != FALSE)
    {
        if (nonwhite_texels != NULL)
        {
            (*nonwhite_texels)++;
        }
    }
    if (ndsRendererTextureColorDominantGreen(color) != FALSE)
    {
        if (green_texels != NULL)
        {
            (*green_texels)++;
        }
    }
}

static void ndsRendererProfileTextureCacheEntry(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    if ((entry == NULL) || (entry->ready == FALSE))
    {
        return;
    }
    gNdsRendererProfileTextureSourceTexels += entry->source_texels;
    gNdsRendererProfileTextureGreenTexels += entry->green_texels;
    gNdsRendererProfileTextureNonWhiteTexels += entry->nonwhite_texels;
}

static s32 ndsRendererHardwareTextureWrapCoord(s32 coord, u32 size,
                                               u32 wrap, u32 mirror)
{
    s32 period;

    if (size == 0u)
    {
        return 0;
    }
    if (wrap == 0u)
    {
        if (coord < 0)
        {
            return 0;
        }
        if ((u32)coord >= size)
        {
            return (s32)size - 1;
        }
        return coord;
    }

    period = (s32)((mirror != 0u) ? size * 2u : size);
    if (period <= 0)
    {
        return 0;
    }
    coord %= period;
    if (coord < 0)
    {
        coord += period;
    }
    if ((mirror != 0u) && ((u32)coord >= size))
    {
        coord = ((s32)size * 2) - 1 - coord;
    }
    return coord;
}

static void ndsRendererProfileTextureSample(s16 s, s16 t)
{
    const NDSRendererHardwareTextureCacheEntry *entry =
        sNdsRendererHardwareActiveTextureEntry;
    s32 sample_s;
    s32 sample_t;

    if ((entry == NULL) ||
        (entry->profile_width == 0u) ||
        (entry->profile_height == 0u))
    {
        return;
    }

    sample_s = ndsRendererHardwareTextureWrapCoord(
        ((s32)s) >> 4, entry->profile_width,
        (entry->params & GL_TEXTURE_WRAP_S) != 0u,
        (entry->params & GL_TEXTURE_FLIP_S) != 0u);
    sample_t = ndsRendererHardwareTextureWrapCoord(
        ((s32)t) >> 4, entry->profile_height,
        (entry->params & GL_TEXTURE_WRAP_T) != 0u,
        (entry->params & GL_TEXTURE_FLIP_T) != 0u);
    if (((u32)sample_s >= entry->profile_width) ||
        ((u32)sample_t >= entry->profile_height))
    {
        return;
    }

    gNdsRendererProfileTextureSampleCount++;
    if (entry->nonwhite_texels != 0u)
    {
        gNdsRendererProfileTextureSampleNonWhiteCount++;
    }
    if (entry->green_texels != 0u)
    {
        gNdsRendererProfileTextureSampleGreenCount++;
    }
}

static u32 ndsRendererHardwareTextureFormatBit(u32 format, u32 size)
{
    u32 index = (format * 4u) + size;

    return (index < 32u) ? (1u << index) : 0u;
}

static void ndsRendererProfileTextureFormat(volatile u32 *mask,
                                            u32 format, u32 size)
{
    if (mask != NULL)
    {
        *mask |= ndsRendererHardwareTextureFormatBit(format, size);
    }
}
#endif

static void ndsRendererHardwareRejectTexture(NDSRendererStats *stats,
                                             u32 format, u32 size,
                                             u32 reason)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_MANIFEST_FALLBACK);
    if (stats != NULL)
    {
        stats->hardware_texture_reject_count++;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureRejectFormatMask, format, size);
    gNdsRendererProfileTextureRejectReasonMask |= reason;
#else
    (void)format;
    (void)size;
    (void)reason;
#endif
}

static s32 ndsRendererHardwarePrepareTexel1Source(
    const NDSRendererStats *stats,
    const NDSRendererConfig *config,
    u32 primary_format,
    u32 primary_size,
    u32 primary_width,
    u32 primary_height,
    NDSRendererHardwareTexel1Source *out)
{
    const NDSRendererTileState *tile;
    const NDSRendererTextureLoadState *load;
    u32 loaded_bytes;
    u32 width;
    u32 height;
    u32 texels;
    u32 source_read_width;
    u32 source_read_height;
    u32 source_last_index;
    u32 source_bytes;
    u32 source_physical_bytes;
    u32 source_width;
    u32 source_origin_s;
    u32 source_origin_t;
    s32 materialize_s;
    s32 materialize_t;

    if (out == NULL)
    {
        return FALSE;
    }
    memset(out, 0, sizeof(*out));
    if ((stats == NULL) ||
        (ndsRendererHardwareUsesTexel01Lerp(stats) == FALSE) ||
        (ndsRendererActiveTextureTile(stats) != NDS_RENDERER_RENDER_TILE))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_ACTIVE_TILE);
        return FALSE;
    }

    tile = &stats->texture_tiles[NDS_RENDERER_RENDER_TILE_1];
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexel1LastTileState =
        (tile->set_seen & 1u) |
        ((tile->size_seen & 1u) << 1) |
        ((tile->line & 0x1ffu) << 2) |
        ((tile->format & 0x7u) << 11) |
        ((tile->size & 0x3u) << 14) |
        ((tile->shifts & 0xfu) << 16) |
        ((tile->shiftt & 0xfu) << 20);
    gNdsRendererProfileTexel1LastPrimaryState =
        (primary_format & 0x7u) |
        ((primary_size & 0x3u) << 3) |
        ((primary_width & 0xffu) << 8) |
        ((primary_height & 0xffu) << 16);
#endif
    if ((primary_format != NDS_RENDERER_HW_TEXTURE_FMT_CI) ||
        (primary_size != NDS_RENDERER_HW_TEXTURE_SIZ_4B) ||
        (tile->set_seen == 0u) || (tile->size_seen == 0u) ||
        (tile->line == 0u) || (tile->shifts != 0u) ||
        (tile->shiftt != 0u) ||
        (tile->format != primary_format) || (tile->size != primary_size))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_TILE_STATE);
        return FALSE;
    }
    load = ndsRendererHardwareFindTextureLoadForTmem(stats, tile->tmem);
    if ((load == NULL) || (load->image == 0u) ||
        (load->load_texels == 0u))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_LOAD_STATE);
        return FALSE;
    }

    loaded_bytes = (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
        load->load_texels * sizeof(u32) :
        load->load_texels * sizeof(u16);
    width = tile->width;
    height = tile->height;
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        (ndsRendererHardwareTextureSourceBytes(
             primary_format, primary_size, width * height) > loaded_bytes))
    {
        width = ndsRendererHardwareTextureLinePixels(primary_size,
                                                     tile->line);
        texels = load->load_texels * sizeof(u16);
        if (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            texels *= 2u;
        }
        else if ((primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ||
                 (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_32B))
        {
            texels /= 2u;
        }
        height = (width != 0u) ? texels / width : 0u;
    }
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_DIMENSIONS);
        return FALSE;
    }

    out->source_extent_width = width;
    out->source_extent_height = height;
    materialize_s = ndsRendererHardwareTextureMaterializesMaskedClamp(
        tile->cms, tile->masks, width, tile->width);
    materialize_t = ndsRendererHardwareTextureMaterializesMaskedClamp(
        tile->cmt, tile->maskt, height, tile->height);
    if (materialize_s != FALSE)
    {
        width = tile->width;
    }
    if (materialize_t != FALSE)
    {
        height = tile->height;
    }
    if ((width != primary_width) || (height != primary_height))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_PAIR_SIZE);
        return FALSE;
    }

    if (load->load_kind == NDS_RENDERER_TEXTURE_LOADTILE)
    {
        source_origin_s = load->load_uls >> 2;
        source_origin_t = load->load_ult >> 2;
        source_width = ndsRendererHardwareTextureSourceWidthPixels(
            primary_size, load->image_size, load->image_width);
    }
    else
    {
        u32 dxt = load->load_dxt;

        source_origin_s = 0u;
        source_origin_t = 0u;
        source_width = out->source_extent_width;
        if (dxt != 0u)
        {
            u32 qwords =
                (NDS_RENDERER_G_TX_DXT_ONE + dxt - 1u) / dxt;

            source_width = ndsRendererHardwareTextureLinePixels(
                primary_size, qwords);
        }
    }
    source_read_width = (materialize_s != FALSE) ?
        (1u << tile->masks) : width;
    source_read_height = (materialize_t != FALSE) ?
        (1u << tile->maskt) : height;
    if ((source_width == 0u) ||
        (source_origin_s >= source_width) ||
        (source_read_width > (source_width - source_origin_s)) ||
        (source_read_width > out->source_extent_width) ||
        (source_read_height > out->source_extent_height))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_RANGE);
        return FALSE;
    }

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererRecordTextureLaneUse(config, primary_format, primary_size);
#endif
    source_last_index =
        ((source_origin_t + source_read_height - 1u) * source_width) +
        source_origin_s + source_read_width - 1u;
    source_bytes = ndsRendererHardwareTextureSourceBytes(
        primary_format, primary_size, source_last_index + 1u);
    if (source_bytes == 0u)
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_BYTES);
        return FALSE;
    }
    source_physical_bytes = ndsRendererTexturePhysicalByteSpan(
        ndsRendererTextureDataLayout(config), source_bytes);
    out->texels = ndsRendererResolveTextureDataPointer(
        config, (const void *)(uintptr_t)load->image,
        source_physical_bytes);
    if (out->texels == NULL)
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_PTR);
        return FALSE;
    }

    out->load = load;
    out->render_tile = tile;
    out->format = primary_format;
    out->size = primary_size;
    out->width = width;
    out->height = height;
    out->source_width = source_width;
    out->source_texels = source_last_index + 1u;
    out->source_origin_s = source_origin_s;
    out->source_origin_t = source_origin_t;
    out->palette_base = (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
        tile->palette * 16u : 0u;
    out->materialize_s = materialize_s;
    out->materialize_t = materialize_t;
    return TRUE;
}

static s32 ndsRendererHardwareTileOriginDelta(u32 primary, u32 secondary)
{
    s32 delta = (s32)((primary - secondary) & 0x0fffu);

    if ((delta & 0x0800) != 0)
    {
        delta -= 0x1000;
    }
    return delta;
}

static s32 ndsRendererHardwareQuarterToTexel(s32 coord)
{
    if (coord < 0)
    {
        return -(((-coord) + 2) >> 2);
    }
    return (coord + 2) >> 2;
}

static u32 ndsRendererHardwareTextureAddressCoord(
    s32 coord, u32 logical_extent, u32 source_extent, u32 mode, u32 mask)
{
    s32 period;
    s32 local;
    u32 mask_extent;

    if (source_extent == 0u)
    {
        return 0u;
    }
    /* The common interior case is already in the first source/mask period.
     * Return it directly instead of paying signed divide/modulo for every
     * TEXEL1 pixel; edge, wrap and mirror coordinates retain the full path. */
    if ((coord >= 0) && ((u32)coord < source_extent) &&
        ((logical_extent == 0u) || ((u32)coord < logical_extent)) &&
        ((mask == 0u) || (mask >= 31u) ||
         ((u32)coord < (1u << mask))))
    {
        return (u32)coord;
    }
    if ((mode & NDS_RENDERER_TX_CLAMP) != 0u)
    {
        if (coord < 0)
        {
            coord = 0;
        }
        else if ((logical_extent != 0u) &&
                 ((u32)coord >= logical_extent))
        {
            coord = (s32)logical_extent - 1;
        }
    }
    if ((mask != 0u) && (mask < 31u))
    {
        mask_extent = 1u << mask;
        period = coord / (s32)mask_extent;
        local = coord % (s32)mask_extent;
        if (local < 0)
        {
            local += (s32)mask_extent;
            period--;
        }
        if (((mode & NDS_RENDERER_TX_MIRROR) != 0u) &&
            ((period & 1) != 0))
        {
            local = (s32)mask_extent - 1 - local;
        }
        return ((u32)local < source_extent) ? (u32)local :
            (source_extent - 1u);
    }
    local = coord % (s32)source_extent;
    if (local < 0)
    {
        local += (s32)source_extent;
    }
    if ((mode & NDS_RENDERER_TX_CLAMP) != 0u)
    {
        return ((u32)coord < source_extent) ? (u32)coord :
            (source_extent - 1u);
    }
    return (u32)local;
}

static u32 ndsRendererHardwareTexel1SourceIndex(
    const NDSRendererHardwareTexel1Source *source,
    s32 origin_delta_s,
    s32 origin_delta_t,
    u32 x,
    u32 y)
{
    s32 source_x;
    s32 source_y;
    u32 addressed_x;
    u32 addressed_y;
    u32 index;

    source_x = (s32)x + origin_delta_s;
    source_y = (s32)y + origin_delta_t;
    addressed_x = ndsRendererHardwareTextureAddressCoord(
        source_x, source->render_tile->width,
        source->source_extent_width, source->render_tile->cms,
        source->render_tile->masks);
    addressed_y = ndsRendererHardwareTextureAddressCoord(
        source_y, source->render_tile->height,
        source->source_extent_height, source->render_tile->cmt,
        source->render_tile->maskt);
    index = ((source->source_origin_t + addressed_y) *
             source->source_width) + source->source_origin_s + addressed_x;
    return index;
}

static u16 ndsRendererHardwareTexel1Color(
    const NDSRendererHardwareTexel1Source *source,
    const NDSRendererConfig *config,
    const u16 *palette,
    u32 palette_count,
    s32 origin_delta_s,
    s32 origin_delta_t,
    u32 x,
    u32 y)
{
    u32 index;

    if ((source == NULL) || (source->render_tile == NULL) ||
        (source->texels == NULL))
    {
        return 0u;
    }
    index = ndsRendererHardwareTexel1SourceIndex(
        source, origin_delta_s, origin_delta_t, x, y);
    return ndsRendererHardwareTextureColor(
        config, source->format, source->size, source->texels, palette,
        palette_count, source->palette_base, index, TRUE);
}

static u32 ndsRendererHardwareAlphaCoverageThreshold(u32 x, u32 y)
{
    static const u8 bayer4x4[16] = {
        0u, 8u, 2u, 10u,
        12u, 4u, 14u, 6u,
        3u, 11u, 1u, 9u,
        15u, 7u, 13u, 5u
    };

    return ((u32)bayer4x4[((y & 3u) << 2) | (x & 3u)] << 4) + 8u;
}

static u32 ndsRendererHardwareExpand5To8(u32 value)
{
    value &= 0x1fu;
    return (value << 3) | (value >> 2);
}

static u32 ndsRendererHardwareBlendTexel01Value(u16 texel0, u16 texel1,
                                                u32 fraction)
{
    u32 inverse;
    u32 red;
    u32 green;
    u32 blue;
    u32 alpha_coverage;
    u32 texel0_red;
    u32 texel0_green;
    u32 texel0_blue;
    u32 texel1_red;
    u32 texel1_green;
    u32 texel1_blue;

    if (fraction > 0xffu)
    {
        fraction = 0xffu;
    }
    inverse = 0x100u - fraction;
    texel0_red = ndsRendererHardwareExpand5To8(texel0 >> 0);
    texel0_green = ndsRendererHardwareExpand5To8(texel0 >> 5);
    texel0_blue = ndsRendererHardwareExpand5To8(texel0 >> 10);
    texel1_red = ndsRendererHardwareExpand5To8(texel1 >> 0);
    texel1_green = ndsRendererHardwareExpand5To8(texel1 >> 5);
    texel1_blue = ndsRendererHardwareExpand5To8(texel1 >> 10);
    red = (((texel0_red * inverse) + (texel1_red * fraction)) >> 8) >> 3;
    green = (((texel0_green * inverse) +
              (texel1_green * fraction)) >> 8) >> 3;
    blue = (((texel0_blue * inverse) +
             (texel1_blue * fraction)) >> 8) >> 3;
    /* G_RM_AA_TEX_EDGE2 converts the fractional alpha lerp to coverage. DS
     * direct-color textures expose A1 only, so retain the same mean coverage
     * with an ordered 4x4 decision instead of unioning both silhouettes. */
    alpha_coverage = ((((texel0 >> 15) & 1u) * 0x100u * inverse) +
                      (((texel1 >> 15) & 1u) * 0x100u * fraction)) >> 8;
    return red | (green << 5) | (blue << 10) |
        (alpha_coverage << NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT);
}

static u16 ndsRendererHardwareResolveTexel01Value(u32 value, u32 x, u32 y)
{
    u32 alpha_coverage =
        value >> NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT;
    u32 alpha = (alpha_coverage >
                 ndsRendererHardwareAlphaCoverageThreshold(x, y)) ? 1u : 0u;

    return (u16)((value & NDS_RENDERER_HW_TEXEL01_RGB_MASK) |
                 (alpha << 15));
}

static u16 ndsRendererHardwareBlendTexel01(u16 texel0, u16 texel1,
                                           u32 fraction, u32 x, u32 y)
{
    return ndsRendererHardwareResolveTexel01Value(
        ndsRendererHardwareBlendTexel01Value(texel0, texel1, fraction),
        x, y);
}

static void __attribute__((noinline))
ndsRendererHardwareBuildTexel01Ci4Lut(
    const NDSRendererConfig *config,
    const u16 *palette,
    u32 palette_count,
    u32 palette0_base,
    u32 palette1_base,
    u32 fraction)
{
    static const u16 alpha_phase_prefix[17] = {
        0x0000u, 0x0001u, 0x0401u, 0x0405u, 0x0505u, 0x0525u,
        0x8525u, 0x85a5u, 0xa5a5u, 0xa5a7u, 0xada7u, 0xadafu,
        0xafafu, 0xafbfu, 0xefbfu, 0xefffu, 0xffffu
    };
    u16 palette0[16];
    u16 palette1[16];
    u32 index0;
    u32 index1;

    for (index0 = 0u; index0 < 16u; index0++)
    {
        palette0[index0] = ndsRendererHardwarePaletteColor(
            config, palette, palette0_base + index0, palette_count, TRUE);
        palette1[index0] = ndsRendererHardwarePaletteColor(
            config, palette, palette1_base + index0, palette_count, TRUE);
    }
    if ((sNdsRendererHardwareTexel01Ci4LutKeyValid != 0u) &&
        (sNdsRendererHardwareTexel01Ci4LutFraction == fraction) &&
        (memcmp(sNdsRendererHardwareTexel01Ci4LutPalette0,
                palette0, sizeof(palette0)) == 0) &&
        (memcmp(sNdsRendererHardwareTexel01Ci4LutPalette1,
                palette1, sizeof(palette1)) == 0))
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererProfileCi4LutReuseCount++;
#endif
        return;
    }
    memcpy(sNdsRendererHardwareTexel01Ci4LutPalette0,
           palette0, sizeof(palette0));
    memcpy(sNdsRendererHardwareTexel01Ci4LutPalette1,
           palette1, sizeof(palette1));
    sNdsRendererHardwareTexel01Ci4LutFraction = fraction;
    sNdsRendererHardwareTexel01Ci4LutKeyValid = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileCi4LutBuildCount++;
#endif
    for (index0 = 0u; index0 < 16u; index0++)
    {
        for (index1 = 0u; index1 < 16u; index1++)
        {
            u32 value = ndsRendererHardwareBlendTexel01Value(
                palette0[index0], palette1[index1], fraction);
            u32 alpha_coverage =
                value >> NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT;
            u32 alpha_prefix_count = (alpha_coverage + 7u) >> 4;
            u32 lut_index = (index0 << 4) | index1;
            u16 rgb;
            u16 alpha_phase_mask;

            if (alpha_prefix_count > 16u)
            {
                alpha_prefix_count = 16u;
            }
            rgb = (u16)(value & NDS_RENDERER_HW_TEXEL01_RGB_MASK);
            alpha_phase_mask = alpha_phase_prefix[alpha_prefix_count];
            sNdsRendererHardwareTexel01Ci4PairLut[lut_index] =
                (u32)rgb | ((u32)alpha_phase_mask << 16);
        }
    }
}

static inline u16 ndsRendererHardwareResolveTexel01Ci4Lut(
    u32 index0, u32 index1, u32 x, u32 y)
{
    u32 phase = ((y & 3u) << 2) | (x & 3u);
    u32 lut_index = (index0 << 4) | index1;
    u32 pair = sNdsRendererHardwareTexel01Ci4PairLut[lut_index];

    return (u16)((pair & NDS_RENDERER_HW_TEXEL01_RGB_MASK) |
        (((pair >> (16u + phase)) & 1u) << 15));
}

static inline u8 ndsRendererHardwareReadCi4Direct(
    const u8 *texels, u32 logical_texel_index, u32 byte_lane_xor)
{
    u8 packed = texels[(logical_texel_index >> 1) ^ byte_lane_xor];

    return ((logical_texel_index & 1u) == 0u) ?
        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static const u8 *ndsRendererHardwareGetCi4Indices(
    const u8 *source, u32 source_texels, u32 byte_lane_xor,
    const u8 *protected_indices)
{
    NDSRendererHardwareCi4IndexCacheEntry *entry;
    u32 replace_index;
    u32 i;

    if ((source == NULL) || (source_texels == 0u) ||
        (source_texels > NDS_RENDERER_HW_CI4_INDEX_CACHE_TEXELS))
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT; i++)
    {
        entry = &sNdsRendererHardwareCi4IndexCache[i];
        if ((entry->valid != 0u) &&
            (entry->source == source) &&
            (entry->source_texels == source_texels) &&
            (entry->byte_lane_xor == byte_lane_xor))
        {
            ndsRendererProfileRecordCi4IndexCacheReuse();
            return entry->indices;
        }
    }

    replace_index = sNdsRendererHardwareCi4IndexCacheNext;
    if (sNdsRendererHardwareCi4IndexCache[replace_index].indices ==
        protected_indices)
    {
        replace_index = (replace_index + 1u) &
            (NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT - 1u);
    }
    entry = &sNdsRendererHardwareCi4IndexCache[replace_index];
    sNdsRendererHardwareCi4IndexCacheNext =
        (replace_index + 1u) &
        (NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT - 1u);
    entry->valid = FALSE;
    entry->source = source;
    entry->source_texels = source_texels;
    entry->byte_lane_xor = byte_lane_xor;
    for (i = 0u; i < source_texels; i++)
    {
        entry->indices[i] = ndsRendererHardwareReadCi4Direct(
            source, i, byte_lane_xor);
    }
    entry->valid = TRUE;
    ndsRendererProfileRecordCi4IndexCacheBuild();
    return entry->indices;
}

static u32 ndsRendererHardwareBuildCi4RepresentativeMap(
    const u8 *source0, const u8 *source1, u8 *representative, u32 count)
{
    u32 i;
    u32 unique = 0u;

    /* The exact 18-bit class key plus one reserves zero as empty; the upper
     * field stores the first coordinate. At most 128 coordinates enter the
     * 256-slot table, so linear probing always reaches an empty terminator. */
    memset(sNdsRendererHardwareTexel01Ci4ClassTable, 0,
           sizeof(sNdsRendererHardwareTexel01Ci4ClassTable));
    for (i = 0u; i < count; i++)
    {
        u32 key = ((i & 3u) << 16) |
            ((u32)source1[i] << 8) | source0[i];
        u32 stored_key = key + 1u;
        u32 slot = (key * 0x9e3779b1u) >> 24;

        while (sNdsRendererHardwareTexel01Ci4ClassTable[slot] != 0u)
        {
            u32 entry = sNdsRendererHardwareTexel01Ci4ClassTable[slot];

            if ((entry & NDS_RENDERER_HW_CI4_CLASS_KEY_MASK) == stored_key)
            {
                representative[i] =
                    (u8)(entry >> NDS_RENDERER_HW_CI4_CLASS_INDEX_SHIFT);
                break;
            }
            slot = (slot + 1u) &
                (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT - 1u);
        }
        if (sNdsRendererHardwareTexel01Ci4ClassTable[slot] == 0u)
        {
            sNdsRendererHardwareTexel01Ci4ClassTable[slot] =
                (i << NDS_RENDERER_HW_CI4_CLASS_INDEX_SHIFT) | stored_key;
            representative[i] = (u8)i;
            unique++;
        }
    }
    return unique;
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
static s32 ndsRendererHardwareStageUniqueTextureRows(
    const u16 *source,
    u16 *staging,
    u32 staging_bytes,
    u8 *row_map,
    u32 row_bytes,
    u32 row_count,
    u32 *staged_bytes)
{
    u32 staging_rows;
    u32 unique_rows = 0u;
    u32 y;

    if ((source == NULL) || (staging == NULL) || (row_map == NULL) ||
        (staged_bytes == NULL) || (row_bytes == 0u) || (row_count == 0u) ||
        (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        ((row_bytes & (sizeof(u16) - 1u)) != 0u))
    {
        return FALSE;
    }
    staging_rows = staging_bytes / row_bytes;
    if ((staging_rows == 0u) ||
        (staging_rows > NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS))
    {
        return FALSE;
    }

    if (sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid == 0u)
    {
        return FALSE;
    }
    for (y = 0u; y < row_count; y++)
    {
        u32 representative =
            sNdsRendererHardwareTexel01Ci4RepresentativeT[y];

        if ((representative >= row_count) || (representative > y))
        {
            return FALSE;
        }
        if (representative == y)
        {
            if (unique_rows >= staging_rows)
            {
                return FALSE;
            }
            memcpy((u8 *)staging + (unique_rows * row_bytes),
                   (const u8 *)source + (y * row_bytes), row_bytes);
            row_map[y] = (u8)unique_rows;
            unique_rows++;
        }
        else
        {
            row_map[y] = row_map[representative];
        }
    }
    *staged_bytes = unique_rows * row_bytes;
    return TRUE;
}
#endif

static s32 NDS_RENDERER_HOT_CODE
ndsRendererHardwareConvertTexel01Ci4Direct(
    const NDSRendererConfig *config,
    const u8 *texels0,
    u32 source0_texels,
    u32 source0_width,
    u32 source0_origin_s,
    u32 source0_origin_t,
    const NDSRendererTileState *render_tile0,
    s32 materialize0_s,
    s32 materialize0_t,
    const NDSRendererHardwareTexel1Source *source1,
    s32 origin1_delta_s,
    s32 origin1_delta_t,
    u32 width,
    u32 height,
    u32 upload_width,
    u8 *compact_row_map,
    u32 *compact_staged_bytes,
    u32 *green_texels,
    u32 *nonwhite_texels)
{
    u32 byte_lane_xor =
        (ndsRendererTextureDataLayout(config) ==
         NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ? 3u : 0u;
    const NDSRendererTileState *render_tile1 = source1->render_tile;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    const u8 *indices0 = ndsRendererHardwareGetCi4Indices(
        texels0, source0_texels, byte_lane_xor, NULL);
    const u8 *indices1 = ndsRendererHardwareGetCi4Indices(
        source1->texels, source1->source_texels, byte_lane_xor, indices0);
#else
    (void)source0_texels;
#endif
    u32 x;
    u32 y;

#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid = FALSE;
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    (void)green_texels;
    (void)nonwhite_texels;
#else
    (void)compact_row_map;
    (void)compact_staged_bytes;
#endif
    /* Animated tile origins can wrap or mirror TEXEL1. Resolve those exact
     * addressing rules once per S coordinate, then reuse them for every row. */
    for (x = 0u; x < width; x++)
    {
        sNdsRendererHardwareTexel01Ci4Source0S[x] = (u8)(
            (materialize0_s != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    x, render_tile0->cms, render_tile0->masks) : x);
        sNdsRendererHardwareTexel01Ci4Source1S[x] = (u8)
            ndsRendererHardwareTextureAddressCoord(
                (s32)x + origin1_delta_s, render_tile1->width,
                source1->source_extent_width, render_tile1->cms,
                render_tile1->masks);
    }
    for (y = 0u; y < height; y++)
    {
        sNdsRendererHardwareTexel01Ci4Source0T[y] = (u8)(
            (materialize0_t != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    y, render_tile0->cmt, render_tile0->maskt) : y);
        sNdsRendererHardwareTexel01Ci4Source1T[y] = (u8)
            ndsRendererHardwareTextureAddressCoord(
                (s32)y + origin1_delta_t, render_tile1->height,
                source1->source_extent_height, render_tile1->cmt,
                render_tile1->maskt);
    }

#if NDS_RENDERER_PROFILE_LEVEL < 2
    if ((indices0 != NULL) && (indices1 != NULL))
    {
        u32 texels = width * height;

        /* Large clamped/masked animated tiles revisit the same pair of source
         * coordinates and ordered-coverage phase many times. Index the first
         * exact representative of each separable S/T class; forward X expansion
         * reads only earlier representatives. Cold output copies repeated rows
         * in reverse Y order, while warm staging records their exact row map. */
        if (texels >= 4096u)
        {
            u32 unique_s = 0u;
            u32 unique_t = 0u;
            u32 unique_texels;

            unique_s = ndsRendererHardwareBuildCi4RepresentativeMap(
                sNdsRendererHardwareTexel01Ci4Source0S,
                sNdsRendererHardwareTexel01Ci4Source1S,
                sNdsRendererHardwareTexel01Ci4RepresentativeS, width);
            unique_t = ndsRendererHardwareBuildCi4RepresentativeMap(
                sNdsRendererHardwareTexel01Ci4Source0T,
                sNdsRendererHardwareTexel01Ci4Source1T,
                sNdsRendererHardwareTexel01Ci4RepresentativeT, height);

            unique_texels = unique_s * unique_t;
            if ((unique_texels * 2u) <= texels)
            {
                s32 compact_output =
                    (compact_row_map != NULL) &&
                    (compact_staged_bytes != NULL) &&
                    (width == upload_width) &&
                    (height <= NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) &&
                    (unique_t <=
                     NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS) &&
                    ((unique_t * upload_width * sizeof(u16)) <=
                     sizeof(sNdsRendererHardwareTextureRefreshLarge));
                u16 *destination = (compact_output != FALSE) ?
                    sNdsRendererHardwareTextureRefreshLarge :
                    sNdsRendererHardwareTextureScratch;
                u32 unique_row = 0u;

                /* Warm animated-water refreshes already upload a compact set
                 * of unique rows during VBlank.  Produce that exact compact
                 * representation here instead of expanding repeated rows into
                 * the 32 KiB scratch arena and immediately copying the unique
                 * rows back into the 16 KiB refresh staging buffer.  Cold
                 * uploads use the same loop with the full scratch destination. */
                for (y = 0u; y < height; y++)
                {
                    u32 representative_y =
                        sNdsRendererHardwareTexel01Ci4RepresentativeT[y];
                    u32 source0_row;
                    u32 source1_row;
                    u32 dst_index;

                    if (representative_y != y)
                    {
                        if (compact_output != FALSE)
                        {
                            compact_row_map[y] =
                                compact_row_map[representative_y];
                        }
                        continue;
                    }
                    if (compact_output != FALSE)
                    {
                        compact_row_map[y] = (u8)unique_row;
                    }
                    source0_row = ((source0_origin_t +
                        sNdsRendererHardwareTexel01Ci4Source0T[y]) *
                        source0_width) + source0_origin_s;
                    source1_row = ((source1->source_origin_t +
                        sNdsRendererHardwareTexel01Ci4Source1T[y]) *
                        source1->source_width) + source1->source_origin_s;
                    dst_index = ((compact_output != FALSE) ?
                        unique_row : y) * upload_width;

                    for (x = 0u; x < width; x++)
                    {
                        u32 index0;
                        u32 index1;
                        u32 representative_x =
                            sNdsRendererHardwareTexel01Ci4RepresentativeS[x];

                        if (representative_x != x)
                        {
                            destination[dst_index + x] =
                                destination[dst_index + representative_x];
                            continue;
                        }
                        index0 = indices0[source0_row +
                            sNdsRendererHardwareTexel01Ci4Source0S[x]];
                        index1 = indices1[source1_row +
                            sNdsRendererHardwareTexel01Ci4Source1S[x]];
                        destination[dst_index + x] =
                            ndsRendererHardwareResolveTexel01Ci4Lut(
                                index0, index1, x, y);
                    }
                    unique_row++;
                }
                if (compact_output != FALSE)
                {
                    *compact_staged_bytes =
                        unique_row * upload_width * sizeof(u16);
                }
                else
                {
                    for (y = height; y != 0u; )
                    {
                        u32 representative_y;

                        y--;
                        representative_y =
                            sNdsRendererHardwareTexel01Ci4RepresentativeT[y];
                        if (representative_y != y)
                        {
                            memcpy(&sNdsRendererHardwareTextureScratch[
                                       y * upload_width],
                                   &sNdsRendererHardwareTextureScratch[
                                       representative_y * upload_width],
                                   width * sizeof(u16));
                        }
                    }
                }
                ndsRendererProfileRecordCi4RepresentativeReuse(
                    unique_texels, texels - unique_texels);
                sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid = TRUE;
                return compact_output;
            }
        }

        for (y = 0u; y < height; y++)
        {
            u32 source0_y = (materialize0_t != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    y, render_tile0->cmt, render_tile0->maskt) : y;
            u32 source1_y = ndsRendererHardwareTextureAddressCoord(
                (s32)y + origin1_delta_t, render_tile1->height,
                source1->source_extent_height, render_tile1->cmt,
                render_tile1->maskt);
            u32 source0_row =
                ((source0_origin_t + source0_y) * source0_width) +
                source0_origin_s;
            u32 source1_row =
                ((source1->source_origin_t + source1_y) *
                 source1->source_width) + source1->source_origin_s;
            u32 dst_index = y * upload_width;

            for (x = 0u; x < width; x++)
            {
                u32 index0 = indices0[source0_row +
                    sNdsRendererHardwareTexel01Ci4Source0S[x]];
                u32 index1 = indices1[source1_row +
                    sNdsRendererHardwareTexel01Ci4Source1S[x]];

                sNdsRendererHardwareTextureScratch[dst_index + x] =
                    ndsRendererHardwareResolveTexel01Ci4Lut(
                        index0, index1, x, y);
            }
        }
        return FALSE;
    }
#endif
    for (y = 0u; y < height; y++)
    {
        u32 source0_y = (materialize0_t != FALSE) ?
            ndsRendererHardwareTextureMaskedAddress(
                y, render_tile0->cmt, render_tile0->maskt) : y;
        u32 source1_y = ndsRendererHardwareTextureAddressCoord(
            (s32)y + origin1_delta_t, render_tile1->height,
            source1->source_extent_height, render_tile1->cmt,
            render_tile1->maskt);
        u32 source0_row =
            ((source0_origin_t + source0_y) * source0_width) +
            source0_origin_s;
        u32 source1_row =
            ((source1->source_origin_t + source1_y) *
             source1->source_width) + source1->source_origin_s;
        u32 dst_index = y * upload_width;

        for (x = 0u; x < width; x++)
        {
            u32 source0_index = source0_row +
                sNdsRendererHardwareTexel01Ci4Source0S[x];
            u32 source1_index = source1_row +
                sNdsRendererHardwareTexel01Ci4Source1S[x];
            u32 index0 = ndsRendererHardwareReadCi4Direct(
                texels0, source0_index, byte_lane_xor);
            u32 index1 = ndsRendererHardwareReadCi4Direct(
                source1->texels, source1_index, byte_lane_xor);
            u16 color = ndsRendererHardwareResolveTexel01Ci4Lut(
                index0, index1, x, y);

            sNdsRendererHardwareTextureScratch[dst_index + x] = color;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            {
                u16 reference = ndsRendererHardwareBlendTexel01(
                    sNdsRendererHardwareTexel01Ci4LutPalette0[index0],
                    sNdsRendererHardwareTexel01Ci4LutPalette1[index1],
                    sNdsRendererHardwareTexel01Ci4LutFraction, x, y);

                gNdsRendererProfileTexturePairOracleChecks++;
                if (color != reference)
                {
                    gNdsRendererProfileTexturePairOracleMismatches++;
                }
            }
            ndsRendererProfileTexturePixel(
                color, green_texels, nonwhite_texels);
#endif
        }
    }
    return FALSE;
}

static s32 ndsRendererHardwareResolveOrBindTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    NDSRendererHardwareResolvedTexture *resolved,
    s32 allow_stage_source_frame)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 texture_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 convert_start;
    u32 upload_start;
#endif
    NDSRendererHardwareTextureKey key;
    NDSRendererHardwareTextureCacheEntry *entry;
    NDSRendererHardwareTextureCacheEntry *fraction_entry;
    NDSRendererHardwareTexel1Source texel1_source;
    const NDSRendererTextureLoadState *primary_load;
    const u8 *texels_src;
    const u16 *tlut_src;
    u32 width;
    u32 height;
    u32 format;
    u32 size;
    u32 upload_width;
    u32 upload_height;
    u32 upload_bytes;
    u32 staged_bytes;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 staged_row_bytes = 0u;
    u32 staged_row_count = 0u;
    u8 staged_row_map[NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
#endif
    u32 texels;
    u32 bytes;
    u32 loaded_bytes;
    u32 source_extent_width;
    u32 source_extent_height;
    u32 source_width;
    u32 source_read_width;
    u32 source_read_height;
    u32 source_origin_s;
    u32 source_origin_t;
    u32 source_last_index;
    u32 source_texels;
    u32 source_bytes;
    u32 source_physical_bytes;
    u32 palette_base;
    u32 tlut_physical_bytes;
    u32 params;
    u32 key_hash;
    u32 render_tile_index;
    u32 render_tile_flags;
    u32 upload_attempts;
    u32 primary_image;
    u32 primary_image_format;
    u32 primary_image_size;
    u32 primary_image_width;
    u32 primary_load_kind;
    u32 primary_load_tile;
    u32 primary_load_uls;
    u32 primary_load_ult;
    u32 primary_load_lrs;
    u32 primary_load_dxt;
    u32 primary_load_texels;
    s32 materialize_s;
    s32 materialize_t;
    s32 wants_texel1;
    s32 use_texel1 = FALSE;
    s32 use_texel1_ci4_lut = FALSE;
    s32 use_texel1_ci4_direct = FALSE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    s32 queue_texture_refresh = FALSE;
    s32 compact_row_output = FALSE;
#endif
    s32 texel1_origin_delta_s = 0;
    s32 texel1_origin_delta_t = 0;
    const NDSRendererTileState *render_tile;
    u32 green_texels = 0u;
    u32 nonwhite_texels = 0u;
    int size_x;
    int size_y;
    u32 x;
    u32 y;
    u16 *upload_buffer = sNdsRendererHardwareTextureScratch;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererSemanticLastTextureKeyHash = 0u;
    sNdsRendererSemanticLastTextureParams = 0u;
#endif
    if (stats == NULL)
    {
        return FALSE;
    }
    /* Hierarchy preflight must remain a pure resident lookup.  The stage-site
     * shortcut performs the live bind/apply side effects, so only the normal
     * execution path may take it. */
    if ((resolved == NULL) &&
        (ndsRendererStageTextureSiteTryBind(stats, state, config) != FALSE))
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
        return TRUE;
    }
    ndsRendererSyncTextureTile(stats);
    render_tile_index = ndsRendererActiveTextureTile(stats);
    render_tile = &stats->texture_tiles[render_tile_index];
    wants_texel1 = ndsRendererHardwareUsesTexel01Lerp(stats);
    primary_load = NULL;
    primary_image = stats->texture_image;
    primary_image_format = stats->texture_format;
    primary_image_size = stats->texture_size;
    primary_image_width = stats->texture_image_width;
    primary_load_kind = stats->texture_load_kind;
    primary_load_tile = stats->texture_load_tile;
    primary_load_uls = stats->texture_load_block_uls;
    primary_load_ult = stats->texture_load_block_ult;
    primary_load_lrs = stats->texture_load_block_lrs;
    primary_load_dxt = stats->texture_load_block_dxt;
    primary_load_texels = stats->texture_load_texels;
    if (wants_texel1 != FALSE)
    {
        /* A two-texture combiner consumes the images resident at each render
         * tile's TMEM address. SETTIMG is mutable, so the last global image is
         * not necessarily TEXEL0 after both LOADBLOCK commands have run. */
        primary_load = ndsRendererHardwareFindTextureLoadForTmem(
            stats, render_tile->tmem);
        if ((primary_load == NULL) || (primary_load->image == 0u) ||
            (primary_load->load_texels == 0u))
        {
            ndsRendererProfileRecordTexel1Reject();
            ndsRendererProfileRecordTexel1RejectReason(
                NDS_RENDERER_HW_TEXEL1_REJECT_LOAD_STATE);
            ndsRendererHardwareRejectTexture(
                stats, stats->texture_format, stats->texture_size,
                NDS_RENDERER_HW_TEXREJECT_MISSING_STATE);
            return FALSE;
        }
        primary_image = primary_load->image;
        primary_image_format = primary_load->image_format;
        primary_image_size = primary_load->image_size;
        primary_image_width = primary_load->image_width;
        primary_load_kind = primary_load->load_kind;
        primary_load_tile = primary_load->load_tile;
        primary_load_uls = primary_load->load_uls;
        primary_load_ult = primary_load->load_ult;
        primary_load_lrs = primary_load->load_lrs;
        primary_load_dxt = primary_load->load_dxt;
        primary_load_texels = primary_load->load_texels;
    }
    render_tile_flags = 0u;
    if (render_tile->set_seen != 0u)
    {
        render_tile_flags |= NDS_RENDERER_TILE_RENDER_SEEN |
            render_tile->flags;
    }
    if (stats->texture_tiles[NDS_RENDERER_LOAD_TILE].set_seen != 0u)
    {
        render_tile_flags |= NDS_RENDERER_TILE_LOAD_SEEN;
    }
    if ((((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_ON) == 0u) &&
         (ndsRendererHardwareTextureImplicitStateOn(stats) == FALSE)) ||
        (primary_image == 0u) ||
        (render_tile->line == 0u) ||
        (primary_load_texels == 0u))
    {
        ndsRendererHardwareRejectTexture(
            stats, stats->texture_format, stats->texture_size,
            NDS_RENDERER_HW_TEXREJECT_MISSING_STATE);
        return FALSE;
    }

    if (render_tile->set_seen != 0u)
    {
        format = render_tile->format;
        size = render_tile->size;
    }
    else
    {
        format = stats->texture_format;
        size = stats->texture_size;
    }

    if ((format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        ((size != NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
         (size != NDS_RENDERER_HW_TEXTURE_SIZ_8B)))
    {
        if (stats->texture_tlut_count <= 16u)
        {
            size = NDS_RENDERER_HW_TEXTURE_SIZ_4B;
        }
        else if (stats->texture_tlut_count <= 256u)
        {
            size = NDS_RENDERER_HW_TEXTURE_SIZ_8B;
        }
        else
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_CI_SIZE);
            return FALSE;
        }
    }
    if ((format != NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_RGBA16) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_IA) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_I16))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_UNSUPPORTED_FORMAT);
        return FALSE;
    }

    loaded_bytes = (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
        primary_load_texels * sizeof(u32) :
        primary_load_texels * sizeof(u16);
    width = render_tile->width;
    height = render_tile->height;
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        (ndsRendererHardwareTextureSourceBytes(format, size, width * height) >
         loaded_bytes))
    {
        width = ndsRendererHardwareTextureLinePixels(
            size, render_tile->line);
        texels = primary_load_texels * sizeof(u16);
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            texels *= 2u;
        }
        else if ((size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ||
                 (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B))
        {
            texels /= 2u;
        }
        height = (width != 0u) ? texels / width : 0u;
    }
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_DIMENSIONS);
        return FALSE;
    }
    source_extent_width = width;
    source_extent_height = height;
    materialize_s = ndsRendererHardwareTextureMaterializesMaskedClamp(
        render_tile->cms, render_tile->masks, source_extent_width,
        render_tile->width);
    materialize_t = ndsRendererHardwareTextureMaterializesMaskedClamp(
        render_tile->cmt, render_tile->maskt, source_extent_height,
        render_tile->height);
    if (materialize_s != FALSE)
    {
        width = render_tile->width;
    }
    if (materialize_t != FALSE)
    {
        height = render_tile->height;
    }

    upload_width = ndsRendererHardwareTextureNextPow2(width);
    upload_height = ndsRendererHardwareTextureNextPow2(height);
    if ((upload_width < width) || (upload_height < height) ||
        (ndsRendererHardwareTextureSizeEnum(upload_width, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(upload_height, &size_y) == FALSE))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_UPLOAD_SIZE);
        return FALSE;
    }

    if (primary_load_kind == NDS_RENDERER_TEXTURE_LOADTILE)
    {
        source_origin_s = primary_load_uls >> 2;
        source_origin_t = primary_load_ult >> 2;
        source_width = ndsRendererHardwareTextureSourceWidthPixels(
            size, primary_image_size, primary_image_width);
    }
    else
    {
        u32 dxt = primary_load_dxt;

        source_origin_s = 0u;
        source_origin_t = 0u;
        source_width = source_extent_width;
        if (dxt != 0u)
        {
            u32 qwords = (NDS_RENDERER_G_TX_DXT_ONE + dxt - 1u) / dxt;

            /* BattleShip gbi.h:3291,3309-3317 encodes DXT as the rounded
             * 1.11 reciprocal of 64-bit source words per row. LOADBLOCK's
             * SETTIMG width is one, so DXT owns the DRAM row stride. */
            source_width = ndsRendererHardwareTextureLinePixels(size, qwords);
        }
    }
    source_read_width = (materialize_s != FALSE) ?
        (1u << render_tile->masks) : width;
    source_read_height = (materialize_t != FALSE) ?
        (1u << render_tile->maskt) : height;
    if ((source_width == 0u) ||
        (source_origin_s >= source_width) ||
        (source_read_width > (source_width - source_origin_s)))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_RANGE);
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererRecordTextureLaneUse(config, format, size);
#endif
    source_last_index =
        ((source_origin_t + source_read_height - 1u) * source_width) +
        source_origin_s + source_read_width - 1u;
    source_texels = source_last_index + 1u;
    source_bytes = ndsRendererHardwareTextureSourceBytes(format, size,
                                                        source_texels);
    if (source_bytes == 0u)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }
    source_physical_bytes = ndsRendererTexturePhysicalByteSpan(
        ndsRendererTextureDataLayout(config), source_bytes);
    if (source_physical_bytes < source_bytes)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }

    if (wants_texel1 != FALSE)
    {
        if (ndsRendererHardwarePrepareTexel1Source(
                stats, config, format, size, width, height,
                &texel1_source) != FALSE)
        {
            use_texel1 = TRUE;
            texel1_origin_delta_s = ndsRendererHardwareQuarterToTexel(
                ndsRendererHardwareTileOriginDelta(
                    render_tile->uls, texel1_source.render_tile->uls));
            texel1_origin_delta_t = ndsRendererHardwareQuarterToTexel(
                ndsRendererHardwareTileOriginDelta(
                    render_tile->ult, texel1_source.render_tile->ult));
            ndsRendererProfileRecordTexel1Composite();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileTexel1LastFraction =
                stats->prim_lod_fraction;
            gNdsRendererProfileTexel1LastImage0 = primary_image;
            gNdsRendererProfileTexel1LastImage1 =
                texel1_source.load->image;
#endif
        }
        else
        {
            ndsRendererProfileRecordTexel1Reject();
        }
    }

    memset(&key, 0, sizeof(key));
    key.image = primary_image;
    key.image_format = primary_image_format;
    key.image_size = primary_image_size;
    key.image_width = primary_image_width;
    key.tlut_image = stats->texture_tlut_image;
    key.tlut_count = stats->texture_tlut_count;
    key.data_layout = (config != NULL) ?
        (u32)config->texture_data_layout :
        (u32)NDS_RENDERER_TEXTURE_DATA_NATIVE;
    key.format = format;
    key.size = size;
    key.width = width;
    key.height = height;
    key.render_tile = render_tile_index;
    key.render_tmem = render_tile->tmem;
    key.render_palette = render_tile->palette;
    key.render_tile_cms = render_tile->cms;
    key.render_tile_cmt = render_tile->cmt;
    key.render_tile_masks = render_tile->masks;
    key.render_tile_maskt = render_tile->maskt;
    key.render_tile_shifts = render_tile->shifts;
    key.render_tile_shiftt = render_tile->shiftt;
    key.load_tile = primary_load_tile;
    key.load_uls = primary_load_uls;
    key.load_ult = primary_load_ult;
    key.load_lrs = primary_load_lrs;
    key.load_dxt = primary_load_dxt;
    key.load_texels = primary_load_texels;
    key.tile_uls = render_tile->uls;
    key.tile_ult = render_tile->ult;
    key.tile_lrs = render_tile->lrs;
    key.tile_lrt = render_tile->lrt;
    key.line = render_tile->line;
    key.flags = render_tile_flags | (primary_load_kind << 8);
    if (use_texel1 != FALSE)
    {
        const NDSRendererTextureLoadState *load = texel1_source.load;
        const NDSRendererTileState *tile = texel1_source.render_tile;

        key.texel1_image = load->image;
        key.texel1_image_format = load->image_format;
        key.texel1_image_size = load->image_size;
        key.texel1_image_width = load->image_width;
        key.texel1_load_kind = load->load_kind;
        key.texel1_render_tmem = tile->tmem;
        key.texel1_render_line = tile->line;
        key.texel1_render_palette = tile->palette;
        key.texel1_render_tile_cms = tile->cms;
        key.texel1_render_tile_cmt = tile->cmt;
        key.texel1_render_tile_masks = tile->masks;
        key.texel1_render_tile_maskt = tile->maskt;
        key.texel1_render_tile_shifts = tile->shifts;
        key.texel1_render_tile_shiftt = tile->shiftt;
        key.texel1_load_tile = load->load_tile;
        key.texel1_load_uls = load->load_uls;
        key.texel1_load_ult = load->load_ult;
        key.texel1_load_lrs = load->load_lrs;
        key.texel1_load_dxt = load->load_dxt;
        key.texel1_load_texels = load->load_texels;
        key.texel1_tile_uls = tile->uls;
        key.texel1_tile_ult = tile->ult;
        key.texel1_tile_lrs = tile->lrs;
        key.texel1_tile_lrt = tile->lrt;
        key.prim_lod_fraction = stats->prim_lod_fraction;
        key.combine_w0 = stats->texture_combine_w0;
        key.combine_w1 = stats->texture_combine_w1;
    }

#if NDS_RENDERER_PROFILE_LEVEL < 2
    key_hash = ndsRendererHardwareTextureKeyHash(&key);
#else
    key_hash = 0u;
#endif

    fraction_entry = NULL;
    entry = ndsRendererHardwareFindTexture(&key, key_hash);
    params = ndsRendererHardwareTextureParams(stats, render_tile,
                                               upload_width, upload_height);
    if ((entry == NULL) && (allow_stage_source_frame != FALSE) &&
        (sNdsRendererBattleStaticTextureArmed != 0u))
    {
        /* Dynamic Pupupu materials keep their source animation and geometry,
         * but reuse the first resident source image when a later image was not
         * prepared before GO. Every other renderer-key word must still match. */
        entry = ndsRendererHardwareFindStageSourceFrameTexture(&key);
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererSemanticLastTextureKeyHash =
        ndsRendererProfileTextureKeyHashFull(&key);
    sNdsRendererSemanticLastTextureParams = params;
#endif
    if (entry != NULL)
    {
        if (resolved != NULL)
        {
            resolved->entry = entry;
            resolved->name = (u32)entry->name;
            resolved->params = (entry->params &
                ~NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK) |
                (params & NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK);
            resolved->format = format;
            resolved->width = width;
            resolved->height = height;
            return TRUE;
        }
        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
        if (entry->pinned != 0u)
        {
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
            /* The offline key deliberately excludes DS sampler bits. Resolve
             * them from the first live BattleShip state, then let the existing
             * site cache reuse the exact entry. */
            entry->params = ndsRendererHardwareMergeTextureParams(params);
            ndsRendererHardwareApplyTextureParams(entry->params);
            sNdsRendererHardwareActiveTextureEntry = entry;
            ndsRendererHardwareRecordBattleStaticTextureHit(entry);
        }
        else if (sNdsRendererHardwareActiveTextureEntry != entry)
        {
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
            ndsRendererHardwareApplyTextureParams(entry->params);
            sNdsRendererHardwareActiveTextureEntry = entry;
        }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = format;
        stats->hardware_texture_width = width;
        stats->hardware_texture_height = height;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererProfileTextureFormat(
            &gNdsRendererProfileTextureBindFormatMask, format, size);
        ndsRendererProfileTextureCacheEntry(entry);
#endif
        ndsRendererStageTextureSiteRemember(
            state, stats, entry, format, size, width, height);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
        return TRUE;
    }
    if (resolved != NULL)
    {
        return FALSE;
    }
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_MANIFEST_FALLBACK);
    if (use_texel1 != FALSE)
    {
        fraction_entry =
            ndsRendererHardwareFindTexel1RefreshTexture(&key);
    }

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD
    if ((sNdsRendererHardwareFrameSerial != 0u) &&
        (fraction_entry != NULL))
    {
        /* The first completed frame populates the exact resident allocations.
         * Subsequent animated-water key changes retain normal lookup, live
         * params, texture binding, batches, and geometry while deliberately
         * keeping the resident pixels from that warm frame. This benchmark
         * branch occurs before source resolution, conversion, or VRAM upload. */
        entry = fraction_entry;
        ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ndsRendererHardwareTextureLookupRemove(entry);
#endif
        entry->key = key;
        sNdsRendererHardwareTextureKeyGeneration++;
        if (sNdsRendererHardwareTextureKeyGeneration == 0u)
        {
            sNdsRendererHardwareTextureKeyGeneration++;
        }
        entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        entry->key_hash = key_hash;
#endif
        entry->params = ndsRendererHardwareMergeTextureParams(params);
        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
        entry->ready = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ndsRendererHardwareTextureLookupInsert(entry);
#endif
        sNdsRendererHardwareActiveTextureEntry = entry;
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = format;
        stats->hardware_texture_width = width;
        stats->hardware_texture_height = height;
        ndsRendererHardwareApplyTextureParams(entry->params);
        sNdsRendererBenchmarkSuppressedTextureUploads++;
        sNdsRendererBenchmarkSuppressedTextureUploadBytes +=
            upload_width * upload_height * sizeof(u16);
        ndsRendererStageTextureSiteRemember(
            state, stats, entry, format, size, width, height);
        return TRUE;
    }
#endif

    texels = width * height;
    bytes = ndsRendererHardwareTextureSourceBytes(
        format, size, source_read_width * source_read_height);
    if ((bytes == 0u) || (bytes > loaded_bytes))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }
    texels_src = ndsRendererResolveTextureDataPointer(
        config, (const void *)(uintptr_t)primary_image,
        source_physical_bytes);
    if (texels_src == NULL)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_PTR);
        return FALSE;
    }
    tlut_src = NULL;
    palette_base = 0u;
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        u32 palette_entries;

        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            palette_base = render_tile->palette * 16u;
        }
        /* LOADTLUT is allowed to load fewer than the format's full 16/256
         * entries. Mario's source Fireball list deliberately loads 13 CI4
         * entries because its 16x16 image uses only indices 0..12. Validate
         * the exact indices reachable by this tile instead of rejecting that
         * legal BattleShip state or reading beyond the loaded palette. Keep
         * the common full-TLUT path scan-free; only partial source TLUTs pay
         * the bounded index census. */
        palette_entries = palette_base +
            ((size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ? 16u : 256u);
        if ((use_texel1 != FALSE) &&
            (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI))
        {
            u32 texel1_palette_entries = texel1_source.palette_base +
                ((texel1_source.size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
                    16u : 256u);

            if (texel1_palette_entries > palette_entries)
            {
                palette_entries = texel1_palette_entries;
            }
        }
        if (stats->texture_tlut_count < palette_entries)
        {
            ndsRendererHardwareRecordBattleTextureFence(
                NDS_RENDERER_BATTLE_TEXTURE_FENCE_PALETTE_DECODE);
            palette_entries = ndsRendererHardwareCiPaletteEntriesUsed(
                config, texels_src, size, source_width,
                source_origin_s, source_origin_t,
                source_read_width, source_read_height, palette_base);
            if ((use_texel1 != FALSE) &&
                (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI))
            {
                u32 texel1_read_width =
                    (texel1_source.materialize_s != FALSE) ?
                        (1u << texel1_source.render_tile->masks) :
                        texel1_source.width;
                u32 texel1_read_height =
                    (texel1_source.materialize_t != FALSE) ?
                        (1u << texel1_source.render_tile->maskt) :
                        texel1_source.height;
                u32 texel1_palette_entries;

                ndsRendererHardwareRecordBattleTextureFence(
                    NDS_RENDERER_BATTLE_TEXTURE_FENCE_PALETTE_DECODE);
                texel1_palette_entries =
                    ndsRendererHardwareCiPaletteEntriesUsed(
                        config, texel1_source.texels, texel1_source.size,
                        texel1_source.source_width,
                        texel1_source.source_origin_s,
                        texel1_source.source_origin_t,
                        texel1_read_width, texel1_read_height,
                        texel1_source.palette_base);

                if (texel1_palette_entries > palette_entries)
                {
                    palette_entries = texel1_palette_entries;
                }
            }
        }
        if ((stats->texture_tlut_image == 0u) ||
            (stats->texture_tlut_count < palette_entries))
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_TLUT);
            return FALSE;
        }
        tlut_physical_bytes = ndsRendererTexturePhysicalByteSpan(
            ndsRendererTextureDataLayout(config),
            palette_entries * sizeof(u16));
        tlut_src = ndsRendererResolveTextureDataPointer(
            config, (const void *)(uintptr_t)stats->texture_tlut_image,
            tlut_physical_bytes);
        if (tlut_src == NULL)
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_TLUT_PTR);
            return FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererProfileTextureFormat(
            &gNdsRendererProfileTexturePaletteFormatMask, format, size);
#endif
    }

    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_CONVERT);
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_PALETTE_DECODE);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    convert_start = cpuGetTiming();
#endif
    upload_bytes = upload_width * upload_height * sizeof(u16);
    staged_bytes = upload_bytes;
#if (NDS_RENDERER_PROFILE_LEVEL < 2) && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    if (fraction_entry != NULL)
    {
        if ((upload_bytes <=
             sizeof(sNdsRendererHardwareTextureRefreshSmall)) &&
            (ndsRendererHardwareTextureRefreshUses(
                 sNdsRendererHardwareTextureRefreshSmall) == FALSE))
        {
            upload_buffer =
                sNdsRendererHardwareTextureRefreshSmall;
            queue_texture_refresh = TRUE;
        }
        else if ((upload_bytes >
                  sizeof(sNdsRendererHardwareTextureRefreshSmall)) &&
                 (ndsRendererHardwareTextureRefreshUses(
                      sNdsRendererHardwareTextureRefreshLarge) == FALSE))
        {
            upload_buffer =
                sNdsRendererHardwareTextureRefreshLarge;
            queue_texture_refresh = TRUE;
        }
    }
#endif
    if ((use_texel1 != FALSE) &&
        (format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (texel1_source.size == NDS_RENDERER_HW_TEXTURE_SIZ_4B))
    {
        /* The source pond's two CI4 inputs can produce only 16x16 palette
         * pairs for a given primitive LOD fraction. Precompute exact RGB and
         * the 16-bit ordered-coverage mask once per pair so the animated pixel
         * loop avoids per-pixel color blending. */
        ndsRendererHardwareBuildTexel01Ci4Lut(
            config, tlut_src, stats->texture_tlut_count, palette_base,
            texel1_source.palette_base, stats->prim_lod_fraction);
        use_texel1_ci4_lut = TRUE;
        use_texel1_ci4_direct = TRUE;
    }

    /* Only the power-of-two rectangle handed to libnds is observable.  The
     * shared scratch arena is sized for the worst 128x128 texture, but smaller
     * animated uploads must not pay to clear the unused tail every frame. */
    if ((width != upload_width) || (height != upload_height))
    {
        memset(sNdsRendererHardwareTextureScratch, 0, upload_bytes);
    }
    if (use_texel1_ci4_direct != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL < 2
        compact_row_output = ndsRendererHardwareConvertTexel01Ci4Direct(
            config, texels_src, source_texels, source_width, source_origin_s,
            source_origin_t, render_tile, materialize_s, materialize_t,
            &texel1_source, texel1_origin_delta_s, texel1_origin_delta_t,
            width, height, upload_width,
            ((upload_buffer == sNdsRendererHardwareTextureRefreshLarge) &&
             (queue_texture_refresh != FALSE) &&
             (width == upload_width) && (height == upload_height)) ?
                staged_row_map : NULL,
            &staged_bytes, &green_texels, &nonwhite_texels);
#else
        (void)ndsRendererHardwareConvertTexel01Ci4Direct(
            config, texels_src, source_texels, source_width, source_origin_s,
            source_origin_t, render_tile, materialize_s, materialize_t,
            &texel1_source, texel1_origin_delta_s, texel1_origin_delta_t,
            width, height, upload_width, NULL, NULL,
            &green_texels, &nonwhite_texels);
#endif
        ndsRendererProfileRecordTextureCi4Direct(width * height);
    }
    else
    {
        for (y = 0u; y < height; y++)
        {
            u32 source_y = (materialize_t != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    y, render_tile->cmt, render_tile->maskt) : y;

            for (x = 0u; x < width; x++)
            {
                u32 source_x = (materialize_s != FALSE) ?
                    ndsRendererHardwareTextureMaskedAddress(
                        x, render_tile->cms, render_tile->masks) : x;
                u32 src_index =
                    ((source_origin_t + source_y) * source_width) +
                    source_origin_s + source_x;
                u32 dst_index = (y * upload_width) + x;
                u16 color;

                if (use_texel1_ci4_lut != FALSE)
                {
                    u32 index0 = ndsRendererReadTexturePackedNibble(
                        config, texels_src, src_index, format, size);
                    u32 source1_index = ndsRendererHardwareTexel1SourceIndex(
                        &texel1_source, texel1_origin_delta_s,
                        texel1_origin_delta_t, x, y);
                    u32 index1 = ndsRendererReadTexturePackedNibble(
                        config, texel1_source.texels, source1_index,
                        texel1_source.format, texel1_source.size);

                    color = ndsRendererHardwareResolveTexel01Ci4Lut(
                        index0, index1, x, y);
                }
                else
                {
                    color = ndsRendererHardwareTextureColor(
                        config, format, size, texels_src, tlut_src,
                        stats->texture_tlut_count, palette_base, src_index,
                        use_texel1);
                    if (use_texel1 != FALSE)
                    {
                        u16 color1 = ndsRendererHardwareTexel1Color(
                            &texel1_source, config, tlut_src,
                            stats->texture_tlut_count,
                            texel1_origin_delta_s, texel1_origin_delta_t,
                            x, y);

                        color = ndsRendererHardwareBlendTexel01(
                            color, color1, stats->prim_lod_fraction, x, y);
                    }
                }
                sNdsRendererHardwareTextureScratch[dst_index] = color;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                ndsRendererProfileTexturePixel(color, &green_texels,
                                               &nonwhite_texels);
#endif
            }
        }
    }
    /* Preserve the canonical lane-observation totals while paying one volatile
     * update per converted texture instead of one per texel. */
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererRecordTextureLaneUseCount(config, format, size, texels);
    if (use_texel1 != FALSE)
    {
        ndsRendererRecordTextureLaneUseCount(
            config, texel1_source.format, texel1_source.size, texels);
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureConvertFormatMask, format, size);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureConvertTicks += cpuGetTiming() - convert_start;

    upload_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (upload_buffer == sNdsRendererHardwareTextureRefreshLarge)
    {
        staged_row_bytes = upload_width * sizeof(u16);
        staged_row_count = upload_height;
        if ((compact_row_output == FALSE) &&
            (ndsRendererHardwareStageUniqueTextureRows(
                sNdsRendererHardwareTextureScratch, upload_buffer,
                sizeof(sNdsRendererHardwareTextureRefreshLarge),
                staged_row_map, staged_row_bytes, staged_row_count,
                &staged_bytes) == FALSE))
        {
            upload_buffer = sNdsRendererHardwareTextureScratch;
            queue_texture_refresh = FALSE;
            staged_bytes = upload_bytes;
            staged_row_bytes = 0u;
            staged_row_count = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            gNdsRendererProfileTextureVBlankFallbackCount++;
#endif
        }
    }
    else if (upload_buffer != sNdsRendererHardwareTextureScratch)
    {
        memcpy(upload_buffer, sNdsRendererHardwareTextureScratch,
               staged_bytes);
    }
#endif
    entry = fraction_entry;
    if (entry != NULL)
    {
        s32 refresh_ready = FALSE;

#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (queue_texture_refresh != FALSE)
        {
            refresh_ready = ndsRendererHardwareQueueTextureRefresh(
                entry, upload_buffer, staged_bytes, upload_bytes,
                (staged_row_count != 0u) ? staged_row_map : NULL,
                staged_row_bytes, staged_row_count);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            if (refresh_ready == FALSE)
            {
                gNdsRendererProfileTextureVBlankFallbackCount++;
            }
#endif
        }
#endif
        if ((refresh_ready == FALSE) &&
            (ndsRendererHardwareReplaceTextureData(
                 entry, upload_buffer, staged_bytes, upload_bytes, NULL, 0u,
                 0u) != FALSE))
        {
            refresh_ready = TRUE;
        }
        if (refresh_ready == FALSE)
        {
            entry = ndsRendererHardwareReleaseTexture(entry);
            entry = NULL;
        }
        else
        {
            ndsRendererProfileRecordTexel1Refresh();
        }
    }
    if (entry == NULL)
    {
        entry = ndsRendererHardwareAllocTexture();
        if (entry == NULL)
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size, NDS_RENDERER_HW_TEXREJECT_ALLOC);
            return FALSE;
        }
        if (entry->name == 0)
        {
            if (ndsRendererHardwareFencedGlGenTextures(
                    1, &entry->name) == 0)
            {
                ndsRendererHardwareRejectTexture(
                    stats, format, size, NDS_RENDERER_HW_TEXREJECT_GENTEX);
                return FALSE;
            }
        }

        ndsRendererHardwareEndBatch();
        ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        upload_attempts = 0u;
        while (ndsRendererHardwareFencedGlTexImage2D(
                   GL_TEXTURE_2D, 0, GL_RGBA, size_x, size_y, 0,
                   params, sNdsRendererHardwareTextureScratch) == 0)
        {
            (void)ndsRendererHardwareReleaseTexture(entry);
            upload_attempts++;
            if ((upload_attempts >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT) ||
                (ndsRendererHardwareEvictTexture(entry) == FALSE))
            {
                ndsRendererHardwareRejectTexture(
                    stats, format, size,
                    NDS_RENDERER_HW_TEXREJECT_TEXIMAGE);
                return FALSE;
            }
            if (ndsRendererHardwareFencedGlGenTextures(
                    1, &entry->name) == 0)
            {
                ndsRendererHardwareRejectTexture(
                    stats, format, size,
                    NDS_RENDERER_HW_TEXREJECT_GENTEX);
                return FALSE;
            }
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        }
    }
    ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureUploadTicks += cpuGetTiming() - upload_start;
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareTextureLookupRemove(entry);
#endif
    entry->key = key;
    sNdsRendererHardwareTextureKeyGeneration++;
    if (sNdsRendererHardwareTextureKeyGeneration == 0u)
    {
        sNdsRendererHardwareTextureKeyGeneration++;
    }
    entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    entry->key_hash = key_hash;
#endif
    entry->params = ndsRendererHardwareMergeTextureParams(params);
    entry->source_texels = texels;
    entry->green_texels = green_texels;
    entry->nonwhite_texels = nonwhite_texels;
    entry->profile_width = upload_width;
    entry->profile_height = upload_height;
    entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    entry->ready = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareTextureLookupInsert(entry);
#endif
    sNdsRendererHardwareActiveTextureEntry = entry;
    stats->hardware_texture_upload_count++;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = format;
    stats->hardware_texture_width = width;
    stats->hardware_texture_height = height;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureBindFormatMask, format, size);
#endif
    ndsRendererHardwareApplyTextureParams(entry->params);
    ndsRendererProfileRecordTextureUpload(
        upload_width * upload_height * sizeof(u16));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureCacheEntry(entry);
#endif
    ndsRendererStageTextureSiteRemember(
        state, stats, entry, format, size, width, height);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
    return TRUE;
}

static s32 ndsRendererHardwareBindTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state)
{
    return ndsRendererHardwareResolveOrBindTexture(
        stats, config, state, NULL, FALSE);
}

static s32 ndsRendererHardwareResolveResidentTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    NDSRendererHardwareResolvedTexture *resolved)
{
    if (resolved == NULL)
    {
        return FALSE;
    }
    memset(resolved, 0, sizeof(*resolved));
    return ndsRendererHardwareResolveOrBindTexture(
        stats, config, state, resolved, FALSE);
}

static s32 ndsRendererHardwareResolveStageSourceFrameTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    NDSRendererHardwareResolvedTexture *resolved)
{
    if (resolved == NULL)
    {
        return FALSE;
    }
    memset(resolved, 0, sizeof(*resolved));
    return ndsRendererHardwareResolveOrBindTexture(
        stats, config, state, resolved, TRUE);
}

static void ndsRendererHardwareEndBatch(void);

static void ndsRendererCopyMtx20p12ToM4x4(
    const NDSRendererMatrix20p12 *src, m4x4 *dst)
{
    u32 row;
    u32 col;

    if ((src == NULL) || (dst == NULL))
    {
        return;
    }

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            dst->m[(row * 4u) + col] = src->m[row][col];
        }
    }
}

static void ndsRendererBuildRawHardwareMatrix(
    const NDSRendererMatrix20p12 *composed,
    NDSRendererMatrix20p12 *hardware)
{
    u32 col;

    if ((composed == NULL) || (hardware == NULL))
    {
        return;
    }

    /* Source coordinates are submitted as source / 256 in DS 4.12. Keep
     * composed rows 0..2 unchanged and divide the complete homogeneous row 3
     * by the same factor. The GX clip vector is then CPU clip / 256, preserving
     * X/W, Y/W, and Z/W for arbitrary composed/matrix-word state. */
    *hardware = *composed;
    for (col = 0u; col < 4u; col++)
    {
        hardware->m[3][col] = ndsRendererRoundShiftS32Signed(
            hardware->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
}

static s32 ndsRendererBuildShiftedRawHardwareMatrix(
    const NDSRendererMatrix20p12 *composed,
    NDSRendererMatrix20p12 *hardware,
    u32 coordinate_shift)
{
    u32 row;
    u32 col;

    if ((composed == NULL) || (hardware == NULL))
    {
        return FALSE;
    }
    *hardware = *composed;
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            s64 scaled = (s64)hardware->m[row][col] << coordinate_shift;

            if ((scaled > INT_MAX) || (scaled < INT_MIN))
            {
                return FALSE;
            }
            hardware->m[row][col] = (s32)scaled;
        }
    }
    for (col = 0u; col < 4u; col++)
    {
        hardware->m[3][col] = ndsRendererRoundShiftS32Signed(
            hardware->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    return TRUE;
}

static void ndsRendererLoadHardwareMatrixPair(
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    u32 mode, u32 generation, u32 scale_world)
{
    m4x4 projection_hw;
    m4x4 modelview_hw;

    (void)scale_world;

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode == mode) &&
        (sNdsRendererHardwareMatrixGeneration == generation))
    {
        return;
    }

    ndsRendererHardwareEndBatch();
    ndsRendererCopyMtx20p12ToM4x4(projection, &projection_hw);
    ndsRendererCopyMtx20p12ToM4x4(modelview, &modelview_hw);

    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hw);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadMatrix4x4(&modelview_hw);

    ndsRendererProfileRecordMatrixLoad();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileMatrixScaleWorld = scale_world;
    gNdsRendererProfileProjectionM00 = projection->m[0][0];
    gNdsRendererProfileProjectionM11 = projection->m[1][1];
    gNdsRendererProfileProjectionM22 = projection->m[2][2];
    gNdsRendererProfileProjectionM32 = projection->m[3][2];
    gNdsRendererProfileModelviewM00 = modelview->m[0][0];
    gNdsRendererProfileModelviewM11 = modelview->m[1][1];
    gNdsRendererProfileModelviewM22 = modelview->m[2][2];
    gNdsRendererProfileModelviewM30 = modelview->m[3][0];
    gNdsRendererProfileModelviewM31 = modelview->m[3][1];
    gNdsRendererProfileModelviewM32 = modelview->m[3][2];
#endif

    sNdsRendererHardwareMatrixMode = mode;
    sNdsRendererHardwareMatrixGeneration = generation;
    sNdsRendererHardwareMatrixLoaded = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererHardwareMatrixSignature =
        ndsRendererProfileHashMatrixPair(
            projection, modelview, mode, generation);
#endif
}

static void ndsRendererLoadHardwareRawComposedMatrix(
    const NDSRendererMatrix20p12 *composed, u32 generation)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
        (sNdsRendererHardwareMatrixGeneration == generation))
    {
        return;
    }

    ndsRendererMtxIdentity20p12(&projection);
    ndsRendererBuildRawHardwareMatrix(composed, &modelview);
    ndsRendererLoadHardwareMatrixPair(
        &projection, &modelview, NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED,
        generation, TRUE);
}

static void ndsRendererLoadHardwareMatrices(
    const NDSRendererTraversalState *state, u32 scale_world)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;

    if ((state != NULL) && (scale_world != 0u) &&
        (state->matrix_valid != 0u))
    {
        if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
            (sNdsRendererHardwareMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareMatrixGeneration ==
             state->matrix_generation))
        {
            return;
        }
        ndsRendererLoadHardwareRawComposedMatrix(
            &state->matrix, state->matrix_generation);
        return;
    }

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY) &&
        (sNdsRendererHardwareMatrixGeneration == 0u))
    {
        return;
    }

    ndsRendererMtxIdentity20p12(&projection);
    ndsRendererMtxIdentity20p12(&modelview);
    ndsRendererLoadHardwareMatrixPair(
        &projection, &modelview,
        NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY, 0u, FALSE);
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u64 ndsRendererHardwareAbsS64(s64 value)
{
    return (value < 0) ? (u64)(-(value + 1)) + 1u : (u64)value;
}

static u64 ndsRendererHardwareAbsDiffS64(s64 lhs, s64 rhs)
{
    if ((lhs < 0) == (rhs < 0))
    {
        return (lhs >= rhs) ? (u64)(lhs - rhs) : (u64)(rhs - lhs);
    }
    return ndsRendererHardwareAbsS64(lhs) + ndsRendererHardwareAbsS64(rhs);
}

static u32 ndsRendererHardwarePosTestCrossError(
    s32 hardware_axis, s32 hardware_w,
    s32 cpu_axis, s32 cpu_w)
{
    s64 lhs = (s64)hardware_axis * cpu_w;
    s64 rhs = (s64)cpu_axis * hardware_w;
    u64 error = ndsRendererHardwareAbsDiffS64(lhs, rhs);
    u64 scale = ndsRendererHardwareAbsS64(cpu_axis) +
                ndsRendererHardwareAbsS64(cpu_w);
    u64 normalized;

    if (scale == 0u)
    {
        scale = 1u;
    }
    normalized = (error / scale) + ((error % scale) != 0u);
    return (normalized > UINT_MAX) ? UINT_MAX : (u32)normalized;
}

static u32 ndsRendererHardwarePosTestInside(s32 axis, s32 w)
{
    return (ndsRendererHardwareAbsS64(axis) <=
            ndsRendererHardwareAbsS64(w)) ? TRUE : FALSE;
}

static void ndsRendererHardwareQueueRawMatrixPosTestValues(
    const NDSRendererMatrix20p12 *matrix, u32 generation,
    const NDSRendererInputVertex *input,
    const NDSRendererClipVertex20p12 *clip, u32 matrix_word)
{
    NDSRendererHardwarePendingPosTest *probe;

    if ((matrix == NULL) || (input == NULL) || (clip == NULL) ||
        (generation == 0u) ||
        (generation == sNdsRendererHardwarePendingPosTestLastGeneration))
    {
        return;
    }
    sNdsRendererHardwarePendingPosTestLastGeneration =
        generation;
    if (sNdsRendererHardwarePendingPosTestCount >=
        NDS_RENDERER_HW_POS_TEST_MAX)
    {
        gNdsRendererProfileMatrixPosTestDropped++;
        return;
    }

    probe = &sNdsRendererHardwarePendingPosTests[
        sNdsRendererHardwarePendingPosTestCount++];
    probe->matrix = *matrix;
    probe->input = *input;
    probe->clip = *clip;
    probe->generation = generation;
    probe->matrix_word = matrix_word;
}

static void ndsRendererHardwareQueueRawMatrixPosTest(
    const NDSRendererTraversalState *state, u32 index)
{
    if ((state == NULL) || (index >= NDS_RENDERER_MAX_VTX) ||
        (state->matrix_valid == 0u))
    {
        return;
    }
    ndsRendererHardwareQueueRawMatrixPosTestValues(
        &state->matrix, state->matrix_generation,
        &state->input_vertices[index], &state->vertices[index],
        state->matrix_word_valid);
}

static void ndsRendererHardwareQueueSnapshotMatrixPosTest(
    const NDSRendererTraversalState *state, u32 snapshot_id, u32 index)
{
    const NDSRendererMatrixSnapshot *snapshot =
        ndsRendererGetMatrixSnapshot(state, snapshot_id);

    if ((snapshot == NULL) || (index >= NDS_RENDERER_MAX_VTX))
    {
        return;
    }
    ndsRendererHardwareQueueRawMatrixPosTestValues(
        &snapshot->matrix, snapshot->generation,
        &state->input_vertices[index], &state->vertices[index], FALSE);
}

static void ndsRendererHardwareQueueMatrixWordPosTestFixture(void)
{
    NDSRendererHardwarePendingPosTest *base;
    NDSRendererHardwarePendingPosTest *probe;
    NDSRendererTraversalState state;
    NDSRendererStats stats;
    NDSRendererMatrix20p12 target_matrix;
    Mtx target_raw;
    const u32 *target_words;
    u32 *current_words;
    u32 i;

    if (sNdsRendererHardwarePendingPosTestCount == 0u)
    {
        return;
    }
    for (i = 0u; i < sNdsRendererHardwarePendingPosTestCount; i++)
    {
        if (sNdsRendererHardwarePendingPosTests[i].matrix_word != 0u)
        {
            return;
        }
    }

    /*
     * The current Pupupu frame does not naturally issue G_MW_MATRIX. Derive
     * one backend-only fixture from its first eligible matrix so profile 2
     * still proves the exact MVP-recalc + matrix-word reconstruction used by
     * BattleShip. This runs after the submitted triangle batch has closed and
     * cannot alter production geometry.
     */
    base = &sNdsRendererHardwarePendingPosTests[0];
    memset(&state, 0, sizeof(state));
    memset(&stats, 0, sizeof(stats));
    state.matrix = base->matrix;
    state.matrix_valid = TRUE;
    ndsRendererApplyMvpRecalcCommand(&stats, &state, 1u, 0u);

    target_matrix = state.matrix;
    if (target_matrix.m[3][0] <=
        (INT_MAX - NDS_RENDERER_HW_POS_TEST_MATRIX_WORD_DELTA))
    {
        target_matrix.m[3][0] +=
            NDS_RENDERER_HW_POS_TEST_MATRIX_WORD_DELTA;
    }
    else
    {
        target_matrix.m[3][0] -=
            NDS_RENDERER_HW_POS_TEST_MATRIX_WORD_DELTA;
    }
    ndsRendererMtxStoreDS20p12ToN64(&target_matrix, &target_raw);
    target_words = (const u32 *)&target_raw.m[0][0];
    current_words = (u32 *)&state.matrix_word_raw.m[0][0];
    for (i = 0u; i < NDS_RENDERER_MATRIX_WORD_COUNT; i++)
    {
        if (current_words[i] != target_words[i])
        {
            ndsRendererApplyMatrixMoveWordCommand(
                &stats, &state,
                (NDS_RENDERER_MOVEWORD_MATRIX <<
                 NDS_RENDERER_MOVEWORD_INDEX_SHIFT) |
                    (i * NDS_RENDERER_MATRIX_WORD_BYTES),
                target_words[i]);
            current_words = (u32 *)&state.matrix_word_raw.m[0][0];
        }
    }
    if ((stats.matrix_mvp_recalc_count != 1u) ||
        (stats.matrix_move_word_count == 0u))
    {
        gNdsRendererProfileMatrixPosTestDropped++;
        return;
    }

    if (sNdsRendererHardwarePendingPosTestCount <
        NDS_RENDERER_HW_POS_TEST_MAX)
    {
        probe = &sNdsRendererHardwarePendingPosTests[
            sNdsRendererHardwarePendingPosTestCount++];
    }
    else
    {
        probe = &sNdsRendererHardwarePendingPosTests[
            NDS_RENDERER_HW_POS_TEST_MAX - 1u];
    }
    probe->matrix = state.matrix;
    probe->input = base->input;
    ndsRendererTransformVertex20p12(&probe->matrix, &probe->input,
                                    &probe->clip);
    probe->generation = state.matrix_generation;
    probe->matrix_word = TRUE;
}

static void ndsRendererHardwareRunRawMatrixPosTests(void)
{
    u32 i;

    ndsRendererHardwareQueueMatrixWordPosTestFixture();

    for (i = 0u; i < sNdsRendererHardwarePendingPosTestCount; i++)
    {
        const NDSRendererHardwarePendingPosTest *probe =
            &sNdsRendererHardwarePendingPosTests[i];
        v16 x = ndsRendererHardwareVertexCoord(probe->input.x, TRUE);
        v16 y = ndsRendererHardwareVertexCoord(probe->input.y, TRUE);
        v16 z = ndsRendererHardwareVertexCoord(probe->input.z, TRUE);
        s32 hardware_x;
        s32 hardware_y;
        s32 hardware_z;
        s32 hardware_w;
        u32 error_x;
        u32 error_y;
        u32 error_z;
        u32 max_error;
        u32 w_sign_mismatch;
        u32 clip_mismatch;

        ndsRendererLoadHardwareRawComposedMatrix(
            &probe->matrix, probe->generation);
        PosTest(x, y, z);
        hardware_x = PosTestXresult();
        hardware_y = PosTestYresult();
        hardware_z = PosTestZresult();
        hardware_w = PosTestWresult();
        error_x = ndsRendererHardwarePosTestCrossError(
            hardware_x, hardware_w, probe->clip.x, probe->clip.w);
        error_y = ndsRendererHardwarePosTestCrossError(
            hardware_y, hardware_w, probe->clip.y, probe->clip.w);
        error_z = ndsRendererHardwarePosTestCrossError(
            hardware_z, hardware_w, probe->clip.z, probe->clip.w);
        max_error = error_x;
        if (error_y > max_error) { max_error = error_y; }
        if (error_z > max_error) { max_error = error_z; }
        w_sign_mismatch = (((hardware_w < 0) != (probe->clip.w < 0)) ||
                           ((hardware_w == 0) != (probe->clip.w == 0))) ?
                              TRUE : FALSE;
        clip_mismatch =
            ((ndsRendererHardwarePosTestInside(hardware_x, hardware_w) !=
              ndsRendererHardwarePosTestInside(probe->clip.x,
                                               probe->clip.w)) ||
             (ndsRendererHardwarePosTestInside(hardware_y, hardware_w) !=
              ndsRendererHardwarePosTestInside(probe->clip.y,
                                               probe->clip.w)) ||
             (ndsRendererHardwarePosTestInside(hardware_z, hardware_w) !=
              ndsRendererHardwarePosTestInside(probe->clip.z,
                                               probe->clip.w))) ? TRUE : FALSE;

        gNdsRendererProfileMatrixPosTestSamples++;
        if (probe->matrix_word != 0u)
        {
            gNdsRendererProfileMatrixPosTestMatrixWordSamples++;
        }
        if (max_error > gNdsRendererProfileMatrixPosTestMaxError)
        {
            gNdsRendererProfileMatrixPosTestMaxError = max_error;
        }
        if (w_sign_mismatch != FALSE)
        {
            gNdsRendererProfileMatrixPosTestWSignMismatches++;
        }
        if (clip_mismatch != FALSE)
        {
            gNdsRendererProfileMatrixPosTestClipMismatches++;
        }
        if ((max_error > NDS_RENDERER_HW_POS_TEST_TOLERANCE) ||
            (w_sign_mismatch != FALSE) || (clip_mismatch != FALSE))
        {
            gNdsRendererProfileMatrixPosTestMismatches++;
        }
    }
    sNdsRendererHardwarePendingPosTestCount = 0u;
    sNdsRendererHardwarePendingPosTestLastGeneration = 0u;
}
#endif

static s32 ndsRendererHardwareNextProjectedDepth(void)
{
    /* The stored counter is scaled by STEP so each painter primitive must
     * consume one complete submitted-v16 depth value.  Subtracting one here
     * made six consecutive no-Z triangles share a depth after division,
     * allowing an earlier stage triangle to reject a later grass/bush draw. */
    sNdsRendererHardwareProjectedDepth -=
        NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;
    return sNdsRendererHardwareProjectedDepth /
        NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;
}

static void ndsRendererHardwareEnterProjectedForeground(void)
{
    if (sNdsRendererHardwareProjectedBackground == FALSE)
    {
        return;
    }

    /* The DS cannot disable depth testing per polygon. Mirror sm64-nds'
     * source G_ZBUFFER transition: early no-Z background draws count down
     * from the far endpoint, then the first source-Z triangle moves later
     * no-Z painter passes in front of the source depth range. */
    sNdsRendererHardwareProjectedDepth =
        NDS_RENDERER_HW_PROJECTED_DEPTH_FOREGROUND_START;
    sNdsRendererHardwareProjectedBackground = FALSE;
}

static void ndsRendererHardwareClipVertex(
    const NDSRendererClipVertex20p12 *vtx, s32 z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    , NDSRendererSemanticVertex *semantic_vertex
#endif
    )
{
    v16 x;
    v16 y;
    v16 out_z;

    if ((vtx == NULL) || (vtx->w == 0))
    {
        return;
    }

    x = ndsRendererHardwareProjectToV16(
        (s64)vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    y = ndsRendererHardwareProjectToV16(
        (s64)vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    out_z = ndsRendererHardwareSourceDepthToV16(
        (s64)z * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        semantic_vertex->x = x;
        semantic_vertex->y = y;
        semantic_vertex->z = out_z;
        semantic_vertex->valid_flags |=
            NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID;
    }
#endif
    ndsRendererProfileHWVertexRange(x, y, out_z);
    glVertex3v16(x, y, out_z);
}

static void ndsRendererHardwareClipVertexNdcDepth(
    const NDSRendererClipVertex20p12 *vtx, s32 z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    , NDSRendererSemanticVertex *semantic_vertex
#endif
    )
{
    v16 x;
    v16 y;
    v16 out_z;

    if ((vtx == NULL) || (vtx->w == 0))
    {
        return;
    }
    x = ndsRendererHardwareProjectToV16(
        (s64)vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    y = ndsRendererHardwareProjectToV16(
        (s64)vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    out_z = ndsRendererHardwareClampS64ToV16(z);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        semantic_vertex->x = x;
        semantic_vertex->y = y;
        semantic_vertex->z = out_z;
        semantic_vertex->valid_flags |=
            NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID;
    }
#endif
    ndsRendererProfileHWVertexRange(x, y, out_z);
    glVertex3v16(x, y, out_z);
}

static void ndsRendererHardwareEndBatch(void)
{
    if (sNdsRendererHardwareTriangleBatchOpen != 0u)
    {
        /* libnds documents glEnd() as a dummy FIFO write. A later glBegin()
         * starts the next primitive group, so only restore state owned by the
         * logical source-triangle batch here. */
        glDisable(GL_ALPHA_TEST);
        ndsRendererProfileRecordBatchEnd();
        sNdsRendererHardwareTriangleBatchOpen = FALSE;
        sNdsRendererHardwareTriangleBatchTextured = FALSE;
        sNdsRendererHardwareTriangleBatchTextureName = 0u;
        sNdsRendererHardwareTriangleBatchPolyFmt = 0u;
        sNdsRendererHardwareTriangleBatchAlphaKey = 0u;
        sNdsRendererHardwareTriangleBatchFogKey = 0u;
        sNdsRendererHardwareTriangleBatchMatrixMode =
            NDS_RENDERER_HW_MATRIX_MODE_NONE;
        sNdsRendererHardwareTriangleBatchMatrixGeneration = 0u;
    }
}

static void ndsRendererHardwareBeginTriangleBatch(
    const NDSRendererStats *stats,
    u32 textured,
    u32 texture_name,
    u32 poly_fmt,
    u32 matrix_mode,
    u32 matrix_generation)
{
    u32 alpha_key;
    u32 fog_key;

    /* An open GX batch can only contain adjacent TRI1/TRI2 source commands.
     * Every opcode capable of changing alpha, fog, texture, or matrix state
     * closes it in ndsRendererScanList before mutation. Keep the per-triangle
     * reuse check to the values that can differ through vertex selection. */
    if ((sNdsRendererHardwareTriangleBatchOpen != 0u) &&
        (sNdsRendererHardwareTriangleBatchTextured == textured) &&
        (sNdsRendererHardwareTriangleBatchTextureName == texture_name) &&
        (sNdsRendererHardwareTriangleBatchPolyFmt == poly_fmt) &&
        (sNdsRendererHardwareTriangleBatchMatrixMode == matrix_mode) &&
        (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
         matrix_generation))
    {
        ndsRendererProfileRecordBatchReuse();
        return;
    }

    alpha_key = ndsRendererHardwareAlphaStateKey(stats);
    fog_key = ndsRendererHardwareFogStateKey(stats);
    ndsRendererHardwareEndBatch();
    if (textured != 0u)
    {
        glEnable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        ndsRendererHardwareBindNoTexture(NULL);
    }
    ndsRendererHardwareApplyAlphaTest(stats);
    ndsRendererHardwareApplyFog(stats);
    ndsRendererHardwareSetPolyFmt(poly_fmt);
    glBegin(GL_TRIANGLE);
    ndsRendererProfileRecordBatchBegin();

    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = textured;
    sNdsRendererHardwareTriangleBatchTextureName = texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = alpha_key;
    sNdsRendererHardwareTriangleBatchFogKey = fog_key;
    sNdsRendererHardwareTriangleBatchMatrixMode = matrix_mode;
    sNdsRendererHardwareTriangleBatchMatrixGeneration = matrix_generation;
}

static void NDS_RENDERER_HOT_CODE
ndsRendererHardwareSubmitVertex(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 vertex_index,
    s32 projected_z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    , NDSRendererSemanticVertex *semantic_vertex
#endif
    )
{
    const NDSRendererInputVertex *vtx;
    const NDSRendererClipVertex20p12 *clip_vtx;
    u32 material_color;
    u32 scale_s;
    u32 scale_t;
    u32 texture_origin_s;
    u32 texture_origin_t;
    u32 context_flags;
    u32 scale_world;
    u32 vertex_color;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 vertex_color_valid;
    s32 use_texture;
    s32 texture_offset;
    s32 zbuffered;
    s32 decal_depth;
    s32 prim_depth;
    s32 source_clip_depth;
    u16 hardware_color;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        memset(semantic_vertex, 0, sizeof(*semantic_vertex));
    }
#endif
    if ((stats == NULL) || (state == NULL) ||
        (vertex_index >= NDS_RENDERER_MAX_VTX))
    {
        return;
    }
    vtx = &state->input_vertices[vertex_index];
    clip_vtx = &state->vertices[vertex_index];
    material_color = state->texture_prepare_material_color;
    scale_s = state->texture_prepare_scale_s;
    scale_t = state->texture_prepare_scale_t;
    texture_origin_s = state->texture_prepare_origin_s;
    texture_origin_t = state->texture_prepare_origin_t;
    context_flags = state->texture_prepare_vertex_flags;
    scale_world = context_flags & NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD;
    vertex_color = state->vertex_colors[vertex_index];
    use_material_color =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL;
    use_vertex_color =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX;
    vertex_color_valid =
        ((state->vertex_color_valid_mask & (1u << vertex_index)) != 0u) ?
            TRUE : FALSE;
    use_texture = context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
    texture_offset = state->texture_prepare_offset;
    zbuffered = context_flags & NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED;
    decal_depth = context_flags & NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH;
    prim_depth = context_flags & NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH;
    source_clip_depth =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH;

#if NDS_RENDERER_PROFILE_LEVEL < 2
    if ((state != NULL) &&
        (vertex_index < NDS_RENDERER_MAX_VTX) &&
        ((state->prepared_vertex_color_valid_mask &
          (1u << vertex_index)) != 0u))
    {
        hardware_color = state->prepared_vertex_colors[vertex_index];
    }
    else
#endif
    {
        hardware_color = ndsRendererHardwarePackedVertexColor(
            stats, vtx, material_color, use_material_color,
            use_vertex_color, vertex_color, vertex_color_valid);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((state != NULL) && (vertex_index < NDS_RENDERER_MAX_VTX))
        {
            state->prepared_vertex_colors[vertex_index] = hardware_color;
            state->prepared_vertex_color_valid_mask |= 1u << vertex_index;
        }
#endif
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        semantic_vertex->color = hardware_color;
        semantic_vertex->valid_flags |=
            NDS_RENDERER_SEMANTIC_VERTEX_COLOR_VALID;
    }
#endif
    glColor(hardware_color);
    if (use_texture != FALSE)
    {
        s16 s;
        s16 t;

#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((state != NULL) &&
            (vertex_index < NDS_RENDERER_MAX_VTX) &&
            ((state->prepared_texcoord_valid_mask &
              (1u << vertex_index)) != 0u))
        {
            s = state->prepared_texcoord_s[vertex_index];
            t = state->prepared_texcoord_t[vertex_index];
        }
        else
#endif
        {
            s = ndsRendererHardwareTexCoord(vtx->s, scale_s,
                                            texture_origin_s,
                                            texture_offset);
            t = ndsRendererHardwareTexCoord(vtx->t, scale_t,
                                            texture_origin_t,
                                            texture_offset);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            if ((state != NULL) && (vertex_index < NDS_RENDERER_MAX_VTX))
            {
                state->prepared_texcoord_s[vertex_index] = s;
                state->prepared_texcoord_t[vertex_index] = t;
                state->prepared_texcoord_valid_mask |= 1u << vertex_index;
            }
#endif
        }

#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererProfileTextureCoord(s, t);
        ndsRendererProfileTextureSample(s, t);
        if (semantic_vertex != NULL)
        {
            semantic_vertex->s = s;
            semantic_vertex->t = t;
            semantic_vertex->valid_flags |=
                NDS_RENDERER_SEMANTIC_VERTEX_ST_VALID;
        }
#endif
        glTexCoord2t16(s, t);
    }
    if ((zbuffered != FALSE) &&
        (decal_depth == FALSE) &&
        (prim_depth == FALSE))
    {
        v16 x = ndsRendererHardwareVertexCoord(vtx->x, scale_world);
        v16 y = ndsRendererHardwareVertexCoord(vtx->y, scale_world);
        v16 z = ndsRendererHardwareVertexCoord(vtx->z, scale_world);

#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (semantic_vertex != NULL)
        {
            semantic_vertex->x = x;
            semantic_vertex->y = y;
            semantic_vertex->z = z;
            semantic_vertex->valid_flags |=
                NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID;
        }
#endif
        ndsRendererProfileVertexRange(vtx, x, y, z);
        glVertex3v16(x, y, z);
    }
    else if (prim_depth != FALSE)
    {
        ndsRendererHardwareClipVertex(clip_vtx, projected_z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                      , semantic_vertex
#endif
                                      );
    }
    else if (decal_depth != FALSE)
    {
        if (clip_vtx != NULL)
        {
            ndsRendererHardwareClipVertex(
                clip_vtx, clip_vtx->z - NDS_RENDERER_HW_DECAL_DEPTH_BIAS
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                , semantic_vertex
#endif
                );
        }
    }
    else
    {
        /* X, Y, Z, and W must come from the same composed matrix snapshot.
         * Matrix-word updates can make projection/modelview fields stale. */
        if ((source_clip_depth != FALSE) && (clip_vtx != NULL) &&
            (clip_vtx->w != 0))
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            s32 depth = ndsRendererHardwareSourceDepthToV16(
                (s64)clip_vtx->z * NDS_RENDERER_HW_PROJECTED_VERTEX,
                clip_vtx->w);

            if (stats->hardware_projected_depth_sample_count == 0u)
            {
                stats->hardware_projected_depth_min = depth;
                stats->hardware_projected_depth_max = depth;
                stats->hardware_projected_w_min = clip_vtx->w;
                stats->hardware_projected_w_max = clip_vtx->w;
            }
            else
            {
                if (depth < stats->hardware_projected_depth_min)
                {
                    stats->hardware_projected_depth_min = depth;
                }
                if (depth > stats->hardware_projected_depth_max)
                {
                    stats->hardware_projected_depth_max = depth;
                }
                if (clip_vtx->w < stats->hardware_projected_w_min)
                {
                    stats->hardware_projected_w_min = clip_vtx->w;
                }
                if (clip_vtx->w > stats->hardware_projected_w_max)
                {
                    stats->hardware_projected_w_max = clip_vtx->w;
                }
            }
            stats->hardware_projected_depth_sample_count++;
#endif
            projected_z = clip_vtx->z;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((state != NULL) &&
            (vertex_index < NDS_RENDERER_MAX_VTX) &&
            (clip_vtx != NULL) && (clip_vtx->w != 0))
        {
            u32 vertex_mask = 1u << vertex_index;
            v16 x;
            v16 y;
            v16 z;

            if ((state->prepared_projected_xy_valid_mask & vertex_mask) !=
                0u)
            {
                x = state->prepared_projected_x[vertex_index];
                y = state->prepared_projected_y[vertex_index];
            }
            else
            {
                x = ndsRendererHardwareProjectToV16(
                    (s64)clip_vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX,
                    clip_vtx->w);
                y = ndsRendererHardwareProjectToV16(
                    (s64)clip_vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX,
                    clip_vtx->w);
                state->prepared_projected_x[vertex_index] = x;
                state->prepared_projected_y[vertex_index] = y;
                state->prepared_projected_xy_valid_mask |= vertex_mask;
            }
            if (source_clip_depth != FALSE)
            {
                if ((state->prepared_projected_source_z_valid_mask &
                     vertex_mask) != 0u)
                {
                    z = state->prepared_projected_source_z[vertex_index];
                }
                else
                {
                    z = ndsRendererHardwareSourceDepthToV16(
                        (s64)projected_z *
                            NDS_RENDERER_HW_PROJECTED_VERTEX,
                        clip_vtx->w);
                    state->prepared_projected_source_z[vertex_index] = z;
                    state->prepared_projected_source_z_valid_mask |=
                        vertex_mask;
                }
            }
            else
            {
                z = ndsRendererHardwareClampS64ToV16(projected_z);
            }
            glVertex3v16(x, y, z);
        }
        else if (source_clip_depth != FALSE)
        {
            ndsRendererHardwareClipVertex(clip_vtx, projected_z);
        }
        else
        {
            ndsRendererHardwareClipVertexNdcDepth(clip_vtx, projected_z);
        }
#else
        if (source_clip_depth != FALSE)
        {
            ndsRendererHardwareClipVertex(
                clip_vtx, projected_z, semantic_vertex);
        }
        else
        {
            ndsRendererHardwareClipVertexNdcDepth(
                clip_vtx, projected_z, semantic_vertex);
        }
#endif
    }
}

static s32 ndsRendererInputTriangleReady(
    const NDSRendererTraversalState *state, u32 packed,
    u32 *out_i0, u32 *out_i1, u32 *out_i2)
{
    u32 i0;
    u32 i1;
    u32 i2;
    u32 mask;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);
    if (out_i0 != NULL) { *out_i0 = i0; }
    if (out_i1 != NULL) { *out_i1 = i1; }
    if (out_i2 != NULL) { *out_i2 = i2; }

    if ((state == NULL) ||
        (i0 >= NDS_RENDERER_MAX_VTX) ||
        (i1 >= NDS_RENDERER_MAX_VTX) ||
        (i2 >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }

    mask = (1u << i0) | (1u << i1) | (1u << i2);
    return ((state->input_vertex_valid_mask & mask) == mask) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareClipZWInsideNearPlane(s32 z, s32 w)
{
    return ((w > 0) && (((s64)z + (s64)w) >= 0)) ? TRUE : FALSE;
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static s32 __attribute__((optimize("Os"))) ndsRendererHardwareClipLerpQ16(
    s32 from, s32 to, s32 ratio_q16)
{
    s64 delta = (s64)to - (s64)from;

    return (s32)((s64)from + ((delta * ratio_q16) >> 16));
}

static u16 __attribute__((optimize("Os")))
ndsRendererHardwareClipLerpColorQ16(
    u16 from, u16 to, s32 ratio_q16)
{
    u32 r = (u32)ndsRendererHardwareClipLerpQ16(
        from & 31u, to & 31u, ratio_q16);
    u32 g = (u32)ndsRendererHardwareClipLerpQ16(
        (from >> 5) & 31u, (to >> 5) & 31u, ratio_q16);
    u32 b = (u32)ndsRendererHardwareClipLerpQ16(
        (from >> 10) & 31u, (to >> 10) & 31u, ratio_q16);

    return (u16)(r | (g << 5) | (b << 10));
}

static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererHardwareClipNearIntersection(
    const NDSRendererProjectedClipVertex *from,
    const NDSRendererProjectedClipVertex *to,
    NDSRendererProjectedClipVertex *out)
{
    s32 from_distance = (s32)(
        ((s64)from->clip.z + (s64)from->clip.w) / 4);
    s32 to_distance = (s32)(
        ((s64)to->clip.z + (s64)to->clip.w) / 4);
    s32 denominator = from_distance - to_distance;
    s32 ratio_q16;

    if (denominator == 0)
    {
        *out = *from;
        return;
    }
#if defined(__arm__)
    ratio_q16 = (s32)div64((s64)from_distance * 65536, denominator);
#else
    ratio_q16 = (s32)(((s64)from_distance * 65536) / denominator);
#endif
    out->clip.x = ndsRendererHardwareClipLerpQ16(
        from->clip.x, to->clip.x, ratio_q16);
    out->clip.y = ndsRendererHardwareClipLerpQ16(
        from->clip.y, to->clip.y, ratio_q16);
    out->clip.w = ndsRendererHardwareClipLerpQ16(
        from->clip.w, to->clip.w, ratio_q16);
    out->clip.z = -out->clip.w;
    out->s = ndsRendererHardwareClipLerpQ16(
        from->s, to->s, ratio_q16);
    out->t = ndsRendererHardwareClipLerpQ16(
        from->t, to->t, ratio_q16);
    out->packed_color = ndsRendererHardwareClipLerpColorQ16(
        from->packed_color, to->packed_color, ratio_q16);
}

static u32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererHardwareClipTriangleNearPlane(
    const NDSRendererProjectedClipVertex input[3],
    NDSRendererProjectedClipVertex output[4])
{
    const NDSRendererProjectedClipVertex *previous = &input[2];
    s32 previous_inside = ndsRendererHardwareClipZWInsideNearPlane(
        previous->clip.z, previous->clip.w);
    u32 input_index;
    u32 output_count = 0u;

    for (input_index = 0u; input_index < 3u; input_index++)
    {
        const NDSRendererProjectedClipVertex *current = &input[input_index];
        s32 current_inside = ndsRendererHardwareClipZWInsideNearPlane(
            current->clip.z, current->clip.w);

        if (current_inside != FALSE)
        {
            if (previous_inside == FALSE)
            {
                ndsRendererHardwareClipNearIntersection(
                    previous, current, &output[output_count++]);
            }
            output[output_count++] = *current;
        }
        else if (previous_inside != FALSE)
        {
            ndsRendererHardwareClipNearIntersection(
                previous, current, &output[output_count++]);
        }
        previous = current;
        previous_inside = current_inside;
    }
    return output_count;
}
#endif

static s32 ndsRendererHardwareTriangleInsideNearPlane(
    const NDSRendererClipVertex20p12 *v0,
    const NDSRendererClipVertex20p12 *v1,
    const NDSRendererClipVertex20p12 *v2)
{
    if ((v0 == NULL) || (v1 == NULL) || (v2 == NULL))
    {
        return FALSE;
    }
    return ((ndsRendererHardwareClipZWInsideNearPlane(v0->z, v0->w) != FALSE) &&
            (ndsRendererHardwareClipZWInsideNearPlane(v1->z, v1->w) != FALSE) &&
            (ndsRendererHardwareClipZWInsideNearPlane(v2->z, v2->w) != FALSE)) ?
        TRUE : FALSE;
}

static s32 ndsRendererHardwareRawMatrixCompatible(
    const NDSRendererTraversalState *state)
{
    return ((state != NULL) && (state->matrix_valid != 0u) &&
            (state->matrix_generation != 0u)) ? TRUE : FALSE;
}

static NDSRendererHWSubmitClass ndsRendererHardwareClassifySubmit(
    const NDSRendererTraversalState *state,
    u32 i0, u32 i1, u32 i2,
    s32 source_zbuffered, s32 decal_depth, s32 prim_depth,
    u32 *out_snapshot_id)
{
    u32 mask;
    u32 snapshot_id;

    if (out_snapshot_id != NULL)
    {
        *out_snapshot_id = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    }

    if (source_zbuffered == FALSE)
    {
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z;
    }
    if (decal_depth != FALSE)
    {
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL;
    }
    if (prim_depth != FALSE)
    {
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH;
    }
    mask = (1u << i0) | (1u << i1) | (1u << i2);
    if ((state->raw_vertex_fit_mask & mask) != mask)
    {
        ndsRendererProfileRecordRawCurrentRangeReject();
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX;
    }

    if ((state->current_transform_vertex_mask & mask) == mask)
    {
        if (ndsRendererHardwareRawMatrixCompatible(state) == FALSE)
        {
            return NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX;
        }
        ndsRendererProfileRecordRawCurrentCandidate();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererHardwareQueueRawMatrixPosTest(state, i0);
#endif
        return NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX;
    }

    snapshot_id = state->vertex_matrix_snapshot[i0];
    if ((snapshot_id != NDS_RENDERER_MATRIX_SNAPSHOT_INVALID) &&
        (state->vertex_matrix_snapshot[i1] == snapshot_id) &&
        (state->vertex_matrix_snapshot[i2] == snapshot_id) &&
        (ndsRendererGetMatrixSnapshot(state, snapshot_id) != NULL))
    {
        if (out_snapshot_id != NULL)
        {
            *out_snapshot_id = snapshot_id;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererHardwareQueueSnapshotMatrixPosTest(
            state, snapshot_id, i0);
#endif
        return NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX;
    }

    ndsRendererProfileRecordRawCrossMatrix();
    return NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX;
}

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP
static void __attribute__((noinline)) NDS_RENDERER_HOT_CODE
ndsRendererSubmitHardwareTriangle(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 packed)
{
    (void)stats;
    (void)config;
    (void)state;
    (void)packed;
    sNdsRendererBenchmarkTriangleCount++;
}
#else
static void NDS_RENDERER_HOT_CODE
ndsRendererSubmitHardwareTriangle(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;
    const NDSRendererInputVertex *v0;
    const NDSRendererTileState *render_tile;
    s32 use_texture;
    s32 implicit_texture_on;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 scale_world;
    u32 material_color;
    u32 poly_alpha;
    u32 poly_fmt;
    u32 texture_name;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 texture_offset;
    s32 zbuffered;
    s32 source_zbuffered;
    s32 decal_depth;
    s32 prim_depth;
    s32 transformed_ready;
    s32 projected_submit;
    s32 raw_submit;
    s32 source_clip_depth;
    s32 no_oracle;
    u32 raw_snapshot_id = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    const NDSRendererMatrixSnapshot *raw_snapshot = NULL;
    NDSRendererHWSubmitClass submit_class;
    s32 projected_z[3] = { 0, 0, 0 };
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 vertex_submit_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    NDSRendererSemanticEvent semantic_event;
#endif

    if (stats == NULL)
    {
        return;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    memset(&semantic_event, 0, sizeof(semantic_event));
    semantic_event.owner = (u32)sNdsRendererProfileOwner;
    semantic_event.owner_occurrence =
        sNdsRendererSemanticSourceProvenance.owner_occurrence;
    semantic_event.list_ordinal =
        sNdsRendererSemanticSourceProvenance.list_ordinal;
    semantic_event.branch_path = (state != NULL) ?
        state->semantic_branch_path :
        sNdsRendererSemanticSourceProvenance.root_branch_path;
    semantic_event.command_index = (state != NULL) ?
        state->semantic_command_index : 0u;
    semantic_event.tri2_half = (state != NULL) ?
        state->semantic_tri2_half : 0u;
    semantic_event.outcome = NDS_RENDERER_SEMANTIC_INPUT_REJECT;
    semantic_event.packed = packed;
    semantic_event.submit_class = NDS_RENDERER_HW_SUBMIT_REJECT;
    semantic_event.source_state_hash =
        ndsRendererSemanticSourceStateHash(stats);
    semantic_event.raw_snapshot_id =
        NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    semantic_event.no_z_depth_before =
        sNdsRendererHardwareProjectedDepth;
    semantic_event.no_z_depth_after =
        sNdsRendererHardwareProjectedDepth;
    semantic_event.no_z_background_before =
        sNdsRendererHardwareProjectedBackground;
    semantic_event.no_z_background_after =
        sNdsRendererHardwareProjectedBackground;
    semantic_event.fog_color = stats->fog_color;
    semantic_event.fog_min = stats->fog_min;
    semantic_event.fog_max = stats->fog_max;
    semantic_event.fog_status = stats->fog_status;
#endif
    if (ndsRendererInputTriangleReady(state, packed, &i0, &i1, &i2) == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        semantic_event.vertex_index[0] = i0;
        semantic_event.vertex_index[1] = i1;
        semantic_event.vertex_index[2] = i2;
        ndsRendererSemanticCommitEvent(&semantic_event);
#endif
        stats->hardware_oracle_reject_count++;
        ndsRendererProfileRecordSubmitClass(
            NDS_RENDERER_HW_SUBMIT_REJECT);
        return;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.vertex_index[0] = i0;
    semantic_event.vertex_index[1] = i1;
    semantic_event.vertex_index[2] = i2;
    semantic_event.vertex_matrix_snapshot[0] =
        state->vertex_matrix_snapshot[i0];
    semantic_event.vertex_matrix_snapshot[1] =
        state->vertex_matrix_snapshot[i1];
    semantic_event.vertex_matrix_snapshot[2] =
        state->vertex_matrix_snapshot[i2];
    semantic_event.vertex_clip_snapshot[0] =
        state->vertex_clip_snapshot[i0];
    semantic_event.vertex_clip_snapshot[1] =
        state->vertex_clip_snapshot[i1];
    semantic_event.vertex_clip_snapshot[2] =
        state->vertex_clip_snapshot[i2];
#endif
    no_oracle = (sNdsRendererHardwareNoOracle != 0u) ? TRUE : FALSE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (state->texture_prepare_valid != 0u)
    {
        source_zbuffered =
            (state->texture_prepare_source_zbuffered != 0u) ? TRUE : FALSE;
        zbuffered = source_zbuffered;
        decal_depth =
            (state->texture_prepare_decal_depth != 0u) ? TRUE : FALSE;
        prim_depth =
            (state->texture_prepare_prim_depth != 0u) ? TRUE : FALSE;
    }
    else
#endif
    {
        zbuffered =
            ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) != 0u) ?
                TRUE : FALSE;
        source_zbuffered = zbuffered;
        decal_depth = ((zbuffered != FALSE) &&
                       ((stats->othermode_l & NDS_RENDERER_ZMODE_DEC) ==
                        NDS_RENDERER_ZMODE_DEC)) ? TRUE : FALSE;
        prim_depth = ((zbuffered != FALSE) &&
                      (ndsRendererHardwareUsePrimDepth(stats) != FALSE)) ?
            TRUE : FALSE;
        state->texture_prepare_source_zbuffered =
            (source_zbuffered != FALSE) ? TRUE : FALSE;
        state->texture_prepare_decal_depth =
            (decal_depth != FALSE) ? TRUE : FALSE;
        state->texture_prepare_prim_depth =
            (prim_depth != FALSE) ? TRUE : FALSE;
    }
    v0 = &state->input_vertices[i0];
    submit_class = ndsRendererHardwareClassifySubmit(
        state, i0, i1, i2, source_zbuffered, decal_depth, prim_depth,
        &raw_snapshot_id);
    raw_submit =
        ((submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) ||
         (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX)) ?
             TRUE : FALSE;
    projected_submit =
        ((source_zbuffered != FALSE) && (raw_submit == FALSE)) ? TRUE : FALSE;
    source_clip_depth = projected_submit;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.submit_class = (u32)submit_class;
    semantic_event.raw_snapshot_id = raw_snapshot_id;
    semantic_event.source_zbuffered =
        (source_zbuffered != FALSE) ? TRUE : FALSE;
    semantic_event.zbuffered = (zbuffered != FALSE) ? TRUE : FALSE;
    semantic_event.raw_submit = (raw_submit != FALSE) ? TRUE : FALSE;
    semantic_event.projected_submit =
        (projected_submit != FALSE) ? TRUE : FALSE;
    semantic_event.decal_depth = (decal_depth != FALSE) ? TRUE : FALSE;
    semantic_event.prim_depth = (prim_depth != FALSE) ? TRUE : FALSE;
    semantic_event.source_clip_depth =
        (source_clip_depth != FALSE) ? TRUE : FALSE;
#endif
    transformed_ready = TRUE;
    if (raw_submit == FALSE)
    {
        transformed_ready =
            ((ndsRendererEnsureTransformedVertex(stats, state, i0) != FALSE) &&
             (ndsRendererEnsureTransformedVertex(stats, state, i1) != FALSE) &&
             (ndsRendererEnsureTransformedVertex(stats, state, i2) != FALSE)) ?
                 TRUE : FALSE;
    }
    if (transformed_ready == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        semantic_event.outcome = NDS_RENDERER_SEMANTIC_TRANSFORM_REJECT;
        semantic_event.submit_class = NDS_RENDERER_HW_SUBMIT_REJECT;
        ndsRendererSemanticCommitEvent(&semantic_event);
#endif
        stats->hardware_oracle_reject_count++;
        ndsRendererProfileRecordSubmitClass(
            NDS_RENDERER_HW_SUBMIT_REJECT);
        return;
    }
    if ((raw_submit == FALSE) &&
        (ndsRendererHardwareTriangleInsideNearPlane(
             &state->vertices[i0], &state->vertices[i1],
             &state->vertices[i2]) == FALSE))
    {
        /* The source RSP clips before perspective divide. Conservatively drop
         * CPU-projected triangles that cross its near plane; emitting any of
         * their post-divide vertices can create a screen-spanning primitive. */
        if (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
        {
            (void)ndsRendererHardwareNextProjectedDepth();
        }
        if (source_zbuffered != FALSE)
        {
            ndsRendererHardwareEnterProjectedForeground();
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        semantic_event.outcome = NDS_RENDERER_SEMANTIC_TRANSFORM_REJECT;
        semantic_event.submit_class = NDS_RENDERER_HW_SUBMIT_REJECT;
        ndsRendererSemanticCommitEvent(&semantic_event);
#endif
        ndsRendererProfileRecordNearPlaneTriangleReject();
        ndsRendererProfileRecordSubmitClass(NDS_RENDERER_HW_SUBMIT_REJECT);
        return;
    }
    if (no_oracle == FALSE)
    {
        stats->hardware_oracle_triangle_count++;
    }
    if (raw_snapshot_id != NDS_RENDERER_MATRIX_SNAPSHOT_INVALID)
    {
        raw_snapshot = ndsRendererGetMatrixSnapshot(state, raw_snapshot_id);
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (sNdsRendererHardwareNoOracle == 0u)
    {
        ndsRendererHardwareRecordOracleTriangle(state, i0, i1, i2);
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (state->texture_prepare_valid == 0u)
    {
        material_color = ndsRendererHardwareColorSource(stats);
        use_material_color = ndsRendererHardwareUseMaterialColor(stats);
        use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
        state->texture_prepare_material_color = material_color;
        state->texture_prepare_vertex_flags =
            ((use_material_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
            ((use_vertex_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);
    }
#else
    material_color = ndsRendererHardwareColorSource(stats);
    use_material_color = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
    state->texture_prepare_material_color = material_color;
    state->texture_prepare_vertex_flags =
        ((use_material_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
        ((use_vertex_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);
    if (stats->texture_combine_count != 0u)
    {
        if (ndsRendererHardwareLitShadeCombine(stats) != FALSE)
        {
            gNdsRendererProfileLitShadeCombineCount++;
        }
        if (use_material_color != FALSE)
        {
            gNdsRendererProfileMaterialCombineCount++;
        }
    }
#endif
    if (state->texture_prepare_valid == 0u)
    {
        poly_alpha = ndsRendererHardwareAlpha(stats, v0);
        state->texture_prepare_alpha_constant =
            (ndsRendererHardwareAlphaUsesVertex(stats) == FALSE) ?
                TRUE : FALSE;
        if (state->texture_prepare_alpha_constant != 0u)
        {
            state->texture_prepare_poly_alpha = poly_alpha;
            state->texture_prepare_poly_fmt =
                ndsRendererHardwarePolyFmt(stats, poly_alpha);
        }
    }
    else if (state->texture_prepare_alpha_constant != 0u)
    {
        poly_alpha = state->texture_prepare_poly_alpha;
    }
    else
    {
        poly_alpha = ndsRendererHardwareAlpha(stats, v0);
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.poly_alpha = poly_alpha;
    semantic_event.alpha_key = ndsRendererHardwareAlphaStateKey(stats);
    semantic_event.fog_key = ndsRendererHardwareFogStateKey(stats);
#endif
    if (poly_alpha == 0u)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        semantic_event.outcome = NDS_RENDERER_SEMANTIC_ALPHA_ZERO;
        semantic_event.poly_fmt =
            ndsRendererHardwarePolyFmt(stats, poly_alpha);
        ndsRendererSemanticCommitEvent(&semantic_event);
#endif
        return;
    }
    ndsRendererProfileRecordSubmitClass(submit_class);
    if (projected_submit != FALSE)
    {
        ndsRendererProfileRecordProjectedSubmit();
    }
    if ((raw_submit == FALSE) &&
        (submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL) &&
        (submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH))
    {
        zbuffered = FALSE;
        decal_depth = FALSE;
        prim_depth = FALSE;
    }
    scale_world = (raw_submit != FALSE) ? TRUE : FALSE;
    if (state->texture_prepare_valid == 0u)
    {
        render_tile =
            &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
        implicit_texture_on =
            ndsRendererHardwareTextureImplicitStateOn(stats);
        use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ?
            ndsRendererHardwareBindTexture(stats, config, state) : FALSE;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->texture_prepare_key_hash = (use_texture != FALSE) ?
            sNdsRendererSemanticLastTextureKeyHash : 0u;
        state->texture_prepare_params = (use_texture != FALSE) ?
            sNdsRendererSemanticLastTextureParams : 0u;
#endif
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled =
            (use_texture != FALSE) ? TRUE : FALSE;
        state->texture_prepare_name = (use_texture != FALSE) ?
            sNdsRendererHardwareBoundTextureName : 0u;
        ndsRendererProfileRecordTexturePrepare();

        texture_scale_s = stats->texture_scale_s;
        texture_scale_t = stats->texture_scale_t;
        if ((use_texture != FALSE) && (implicit_texture_on != FALSE))
        {
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
            {
                texture_scale_s = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
            {
                texture_scale_t = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
        }
        texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
        state->texture_prepare_scale_s = texture_scale_s;
        state->texture_prepare_scale_t = texture_scale_t;
        state->texture_prepare_origin_s = render_tile->uls;
        state->texture_prepare_origin_t = render_tile->ult;
        state->texture_prepare_offset = texture_offset;
        if (use_texture != FALSE)
        {
            state->texture_prepare_vertex_flags |=
                NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        }
    }
    else
    {
        use_texture =
            (state->texture_prepare_enabled != 0u) ? TRUE : FALSE;
        ndsRendererProfileRecordTexturePrepareReuse();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (use_texture != FALSE)
        {
            state->texture_prepare_vertex_flags |=
                NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        }
#endif
    }
#if NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY
    if (use_texture != FALSE)
    {
        state->texture_prepare_vertex_flags &=
            ~(NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL |
              NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX);
    }
#endif
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        ndsRendererLoadHardwareMatrices(state, scale_world);
    }
    else if (raw_snapshot != NULL)
    {
        ndsRendererLoadHardwareRawComposedMatrix(
            &raw_snapshot->matrix, raw_snapshot->generation);
    }
    else
    {
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
    }
    if (prim_depth != FALSE)
    {
        projected_z[0] = projected_z[1] = projected_z[2] =
            (s32)(stats->prim_depth & 0xffffu);
    }
    else if (source_zbuffered != FALSE)
    {
        /* Source-Z projected submissions use the composed clip Z below and
         * must not consume the synthetic no-Z painter counter. */
        projected_z[0] = projected_z[1] = projected_z[2] = 0;
    }
    else
    {
        projected_z[0] = projected_z[1] = projected_z[2] =
            ndsRendererHardwareNextProjectedDepth();
    }
    poly_fmt = (state->texture_prepare_alpha_constant != 0u) ?
        state->texture_prepare_poly_fmt :
        ndsRendererHardwarePolyFmt(stats, poly_alpha);
    texture_name = state->texture_prepare_name;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.poly_fmt = poly_fmt;
    semantic_event.texture_name = texture_name;
    semantic_event.texture_key_hash = state->texture_prepare_key_hash;
    semantic_event.texture_params = state->texture_prepare_params;
    semantic_event.matrix_loaded = sNdsRendererHardwareMatrixLoaded;
    semantic_event.matrix_mode = sNdsRendererHardwareMatrixMode;
    semantic_event.matrix_generation =
        sNdsRendererHardwareMatrixGeneration;
    semantic_event.matrix_signature =
        sNdsRendererHardwareMatrixSignature;
    semantic_event.projected_z[0] = projected_z[0];
    semantic_event.projected_z[1] = projected_z[1];
    semantic_event.projected_z[2] = projected_z[2];
#endif
    ndsRendererHardwareBeginTriangleBatch(
        stats, (use_texture != FALSE) ? TRUE : FALSE,
        texture_name, poly_fmt, sNdsRendererHardwareMatrixMode,
        sNdsRendererHardwareMatrixGeneration);
    state->texture_prepare_vertex_flags =
        (state->texture_prepare_vertex_flags &
         NDS_RENDERER_VERTEX_CONTEXT_PREPARED_MASK) |
        ((scale_world != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD : 0u) |
        ((zbuffered != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED : 0u) |
        ((decal_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH : 0u) |
        ((prim_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH : 0u) |
        ((source_clip_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH : 0u);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.context_flags = state->texture_prepare_vertex_flags;
    semantic_event.zbuffered = (zbuffered != FALSE) ? TRUE : FALSE;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    vertex_submit_start = cpuGetTiming();
#endif
    ndsRendererHardwareSubmitVertex(stats, state, i0, projected_z[0]
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                    , &semantic_event.vertex[0]
#endif
                                    );
    ndsRendererHardwareSubmitVertex(stats, state, i1, projected_z[1]
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                    , &semantic_event.vertex[1]
#endif
                                    );
    ndsRendererHardwareSubmitVertex(stats, state, i2, projected_z[2]
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                    , &semantic_event.vertex[2]
#endif
                                    );
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileVertexSubmitTicks +=
        cpuGetTiming() - vertex_submit_start;
#endif
    if (source_zbuffered != FALSE)
    {
        ndsRendererHardwareEnterProjectedForeground();
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.outcome = NDS_RENDERER_SEMANTIC_EMITTED;
    semantic_event.no_z_depth_after = sNdsRendererHardwareProjectedDepth;
    semantic_event.no_z_background_after =
        sNdsRendererHardwareProjectedBackground;
    ndsRendererSemanticCommitEvent(&semantic_event);
#endif

    sNdsRendererHardwareSubmitted = TRUE;
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount++;
#endif
    stats->hardware_triangle_count++;
    stats->hardware_vertex_count += 3u;
    ndsRendererProfileRecordHardwareTriangle();
    if (zbuffered != FALSE)
    {
        stats->hardware_zbuffer_triangle_count++;
        if (decal_depth != FALSE)
        {
            stats->hardware_decal_depth_triangle_count++;
        }
        if (prim_depth != FALSE)
        {
            stats->hardware_prim_depth_triangle_count++;
        }
    }
    else
    {
        stats->hardware_projected_depth_triangle_count++;
    }
}
#endif
#endif

#if !NDS_RENDERER_HW_TRIANGLES
/* Shared scene code owns teardown calls even in the software build.  Keep the
 * hardware-only texture API link-complete without allocating GX state. */
volatile u32 gNdsRendererBattleStaticTextureEnabled = 0u;

s32 ndsRendererHardwarePrepareBattleStaticTextures(void)
{
    return FALSE;
}

void ndsRendererHardwareArmBattleStaticTextures(void)
{
}

void ndsRendererHardwareDiscardBattleStaticTextures(void)
{
}

void ndsRendererHardwareAbortBattleStaticTextures(void)
{
}
#endif

static inline void ndsRendererExecuteTriangleCommand(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 op,
    u32 w0,
    u32 w1)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    u32 triangle_submit_start;
#endif
#if !NDS_RENDERER_HW_TRIANGLES
    (void)config;
#endif

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->triangle_command_count++);
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->render_command_count++);
    if (op == NDS_RENDERER_OP_TRI1)
    {
        u32 packed = ndsGBIDecodeF3DEX2Tri1(w0);

        stats->triangle_count++;
#if NDS_RENDERER_HW_TRIANGLES
        if (sNdsRendererHardwareNoOracle == 0u)
#endif
        ndsRendererRecordTransformedTriangle(stats, state, packed);
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        triangle_submit_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_tri2_half = 0u;
#endif
        ndsRendererSubmitHardwareTriangle(stats, config, state, packed);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererProfileTriangleSubmitTicks +=
            cpuGetTiming() - triangle_submit_start;
#endif
#endif
        return;
    }

    stats->triangle_count += 2u;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererHardwareNoOracle == 0u)
    {
#endif
        ndsRendererRecordTransformedTriangle(
            stats, state, ndsGBIDecodeF3DEX2Tri2First(w0));
        ndsRendererRecordTransformedTriangle(
            stats, state, ndsGBIDecodeF3DEX2Tri2Second(w1));
#if NDS_RENDERER_HW_TRIANGLES
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    triangle_submit_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->semantic_tri2_half = 0u;
#endif
    ndsRendererSubmitHardwareTriangle(
        stats, config, state, ndsGBIDecodeF3DEX2Tri2First(w0));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->semantic_tri2_half = 1u;
#endif
    ndsRendererSubmitHardwareTriangle(
        stats, config, state, ndsGBIDecodeF3DEX2Tri2Second(w1));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileTriangleSubmitTicks +=
        cpuGetTiming() - triangle_submit_start;
#endif
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static inline s32 ndsRendererFastRawStateEligible(
    const NDSRendererTraversalState *state)
{
    const u32 required_flags =
        NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD |
        NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED;
    const u32 forbidden_flags =
        NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH |
        NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH |
        NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH;

    return ((state != NULL) &&
            (state->texture_prepare_valid != 0u) &&
            (state->texture_prepare_source_zbuffered != 0u) &&
            (state->texture_prepare_decal_depth == 0u) &&
            (state->texture_prepare_prim_depth == 0u) &&
            (state->texture_prepare_alpha_constant != 0u) &&
            (state->texture_prepare_poly_alpha != 0u) &&
            ((state->texture_prepare_vertex_flags & required_flags) ==
             required_flags) &&
            ((state->texture_prepare_vertex_flags & forbidden_flags) == 0u) &&
            (state->matrix_valid != 0u) &&
            (state->matrix_generation != 0u) &&
            (sNdsRendererHardwareMatrixLoaded != 0u) &&
            (sNdsRendererHardwareMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareMatrixGeneration ==
             state->matrix_generation) &&
            (sNdsRendererHardwareTriangleBatchOpen != 0u) &&
            (sNdsRendererHardwareTriangleBatchTextured ==
             state->texture_prepare_enabled) &&
            (sNdsRendererHardwareTriangleBatchTextureName ==
             state->texture_prepare_name) &&
            (sNdsRendererHardwareTriangleBatchPolyFmt ==
             state->texture_prepare_poly_fmt) &&
            (sNdsRendererHardwareTriangleBatchMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
             state->matrix_generation)) ? TRUE : FALSE;
}

static inline s32 ndsRendererFastDecodeTriangle(
    u32 packed, u32 *indices, u32 *required_mask)
{
    u32 i0;
    u32 i1;
    u32 i2;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);
    if ((i0 >= NDS_RENDERER_MAX_VTX) ||
        (i1 >= NDS_RENDERER_MAX_VTX) ||
        (i2 >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    indices[0] = i0;
    indices[1] = i1;
    indices[2] = i2;
    *required_mask |= (1u << i0) | (1u << i1) | (1u << i2);
    return TRUE;
}

static u32 ndsRendererDirectRawPlanHash(const Gfx *source)
{
    uintptr_t value = (uintptr_t)source;

    return (u32)(((value >> 3) ^ (value >> 11)) &
                 (NDS_RENDERER_DIRECT_RAW_PLAN_COUNT - 1u));
}

static NDSRendererDirectRawPlan *ndsRendererDirectRawFindPlan(
    const Gfx *source, u32 max_commands)
{
    NDSRendererDirectRawPlan *empty = NULL;
    u32 slot = ndsRendererDirectRawPlanHash(source);
    u32 probe;
    u32 command_count = 0u;
    u32 triangle_count = 0u;
    u32 entry_offset;
    u32 i;

    if ((source == NULL) || (max_commands == 0u))
    {
        return NULL;
    }
    for (probe = 0u; probe < NDS_RENDERER_DIRECT_RAW_PLAN_COUNT; probe++)
    {
        NDSRendererDirectRawPlan *plan =
            &sNdsRendererDirectRawPlans[
                (slot + probe) & (NDS_RENDERER_DIRECT_RAW_PLAN_COUNT - 1u)];

        if (plan->source == source)
        {
            const Gfx *last;

            if ((plan->command_count == 0u) ||
                (plan->command_count > max_commands))
            {
                return NULL;
            }
            last = source + plan->command_count - 1u;
            if ((source->words.w0 != plan->first_w0) ||
                (source->words.w1 != plan->first_w1) ||
                (last->words.w0 != plan->last_w0) ||
                (last->words.w1 != plan->last_w1))
            {
                return NULL;
            }
            return plan;
        }
        if (plan->source == NULL)
        {
            empty = plan;
            break;
        }
    }
    if (empty == NULL)
    {
        return NULL;
    }

    while (command_count < max_commands)
    {
        u32 op = source[command_count].words.w0 >> 24;

        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            break;
        }
        triangle_count += (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        command_count++;
    }
    if ((command_count == 0u) ||
        (command_count > 0xffffu) ||
        (triangle_count > 0xffffu) ||
        ((sNdsRendererDirectRawEntryCount + command_count) >
         NDS_RENDERER_DIRECT_RAW_ENTRY_COUNT))
    {
        return NULL;
    }

    entry_offset = sNdsRendererDirectRawEntryCount;
    for (i = 0u; i < command_count; i++)
    {
        const Gfx *command = source + i;
        NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[entry_offset + i];
        u32 w0 = command->words.w0;
        u32 w1 = command->words.w1;
        u32 op = w0 >> 24;
        u32 packed[2];
        u32 indices[6];
        u32 required_mask = 0u;
        u32 command_triangles =
            (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        u32 index;

        packed[0] = (op == NDS_RENDERER_OP_TRI1) ?
            ndsGBIDecodeF3DEX2Tri1(w0) :
            ndsGBIDecodeF3DEX2Tri2First(w0);
        packed[1] = (command_triangles == 2u) ?
            ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
        if ((ndsRendererFastDecodeTriangle(
                 packed[0], &indices[0], &required_mask) == FALSE) ||
            ((command_triangles == 2u) &&
             (ndsRendererFastDecodeTriangle(
                  packed[1], &indices[3], &required_mask) == FALSE)))
        {
            return NULL;
        }
        entry->required_mask = required_mask;
        entry->triangle_count = (u8)command_triangles;
        entry->reserved = 0u;
        for (index = 0u; index < (command_triangles * 3u); index++)
        {
            entry->indices[index] = (u8)indices[index];
        }
        for (; index < 6u; index++)
        {
            entry->indices[index] = 0u;
        }
    }

    sNdsRendererDirectRawEntryCount += command_count;
    empty->entry_offset = (u16)entry_offset;
    empty->command_count = (u16)command_count;
    empty->triangle_count = (u16)triangle_count;
    empty->reserved = 0u;
    empty->first_w0 = source->words.w0;
    empty->first_w1 = source->words.w1;
    empty->last_w0 = source[command_count - 1u].words.w0;
    empty->last_w1 = source[command_count - 1u].words.w1;
    empty->source = source;
    return empty;
}

static void NDS_RENDERER_FAST_RUN_CODE ndsRendererFastPrepareRawSlots(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 required_mask,
    u32 textured)
{
    u32 missing_color = required_mask &
        ~state->prepared_vertex_color_valid_mask;
    u32 context_flags = state->texture_prepare_vertex_flags;

    while (missing_color != 0u)
    {
        u32 vertex_index = (u32)__builtin_ctz(missing_color);
        u32 vertex_mask = 1u << vertex_index;
        const NDSRendererInputVertex *vtx =
            &state->input_vertices[vertex_index];

        state->prepared_vertex_colors[vertex_index] =
            ndsRendererHardwarePackedVertexColor(
                stats, vtx, state->texture_prepare_material_color,
                context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL,
                context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX,
                state->vertex_colors[vertex_index],
                ((state->vertex_color_valid_mask & vertex_mask) != 0u) ?
                    TRUE : FALSE);
        state->prepared_vertex_color_valid_mask |= vertex_mask;
        missing_color &= ~vertex_mask;
    }

    if (textured != 0u)
    {
        u32 missing_texcoord = required_mask &
            ~state->prepared_texcoord_valid_mask;

        while (missing_texcoord != 0u)
        {
            u32 vertex_index = (u32)__builtin_ctz(missing_texcoord);
            u32 vertex_mask = 1u << vertex_index;
            const NDSRendererInputVertex *vtx =
                &state->input_vertices[vertex_index];

            state->prepared_texcoord_s[vertex_index] =
                ndsRendererHardwareTexCoord(
                    vtx->s, state->texture_prepare_scale_s,
                    state->texture_prepare_origin_s,
                    state->texture_prepare_offset);
            state->prepared_texcoord_t[vertex_index] =
                ndsRendererHardwareTexCoord(
                    vtx->t, state->texture_prepare_scale_t,
                    state->texture_prepare_origin_t,
                    state->texture_prepare_offset);
            state->prepared_texcoord_valid_mask |= vertex_mask;
            missing_texcoord &= ~vertex_mask;
        }
    }
}

static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererFastRawFallbackCommand(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 op,
    u32 w0,
    u32 w1)
{
    ndsRendererExecuteTriangleCommand(stats, config, state, op, w0, w1);
}

static inline void ndsRendererFastEmitRawUntexturedVertex(
    const NDSRendererTraversalState *state, u32 vertex_index)
{
    const NDSRendererInputVertex *vtx =
        &state->input_vertices[vertex_index];

    glColor(state->prepared_vertex_colors[vertex_index]);
    glVertex3v16(
        (v16)((s32)vtx->x * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->y * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->z * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
}

static inline void ndsRendererFastEmitRawTexturedVertex(
    const NDSRendererTraversalState *state, u32 vertex_index)
{
    const NDSRendererInputVertex *vtx =
        &state->input_vertices[vertex_index];

    glColor(state->prepared_vertex_colors[vertex_index]);
    glTexCoord2t16(state->prepared_texcoord_s[vertex_index],
                  state->prepared_texcoord_t[vertex_index]);
    glVertex3v16(
        (v16)((s32)vtx->x * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->y * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->z * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
}

static inline void ndsRendererFastEmitRawCommand(
    const NDSRendererTraversalState *state,
    const u32 *indices,
    u32 triangle_count,
    u32 textured)
{
    u32 triangle_index;

    if (textured != 0u)
    {
        for (triangle_index = 0u;
             triangle_index < triangle_count;
             triangle_index++)
        {
            const u32 *triangle = &indices[triangle_index * 3u];

            ndsRendererFastEmitRawTexturedVertex(state, triangle[0]);
            ndsRendererFastEmitRawTexturedVertex(state, triangle[1]);
            ndsRendererFastEmitRawTexturedVertex(state, triangle[2]);
        }
    }
    else
    {
        for (triangle_index = 0u;
             triangle_index < triangle_count;
             triangle_index++)
        {
            const u32 *triangle = &indices[triangle_index * 3u];

            ndsRendererFastEmitRawUntexturedVertex(state, triangle[0]);
            ndsRendererFastEmitRawUntexturedVertex(state, triangle[1]);
            ndsRendererFastEmitRawUntexturedVertex(state, triangle[2]);
        }
    }
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererFastCommitRawSemanticTriangle(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 packed,
    const u32 *indices)
{
    NDSRendererSemanticEvent event;
    u32 vertex_number;

    memset(&event, 0, sizeof(event));
    event.owner = (u32)sNdsRendererProfileOwner;
    event.owner_occurrence =
        sNdsRendererSemanticSourceProvenance.owner_occurrence;
    event.list_ordinal =
        sNdsRendererSemanticSourceProvenance.list_ordinal;
    event.branch_path = state->semantic_branch_path;
    event.command_index = state->semantic_command_index;
    event.tri2_half = state->semantic_tri2_half;
    event.outcome = NDS_RENDERER_SEMANTIC_EMITTED;
    event.packed = packed;
    event.submit_class =
        NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX;
    event.source_state_hash = ndsRendererSemanticSourceStateHash(stats);
    event.raw_snapshot_id = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    event.context_flags = state->texture_prepare_vertex_flags;
    event.source_zbuffered = TRUE;
    event.zbuffered = TRUE;
    event.raw_submit = TRUE;
    event.poly_alpha = state->texture_prepare_poly_alpha;
    event.poly_fmt = state->texture_prepare_poly_fmt;
    event.alpha_key = ndsRendererHardwareAlphaStateKey(stats);
    event.fog_key = ndsRendererHardwareFogStateKey(stats);
    event.fog_color = stats->fog_color;
    event.fog_min = stats->fog_min;
    event.fog_max = stats->fog_max;
    event.fog_status = stats->fog_status;
    event.texture_name = state->texture_prepare_name;
    event.texture_key_hash = state->texture_prepare_key_hash;
    event.texture_params = state->texture_prepare_params;
    event.matrix_loaded = sNdsRendererHardwareMatrixLoaded;
    event.matrix_mode = sNdsRendererHardwareMatrixMode;
    event.matrix_generation = sNdsRendererHardwareMatrixGeneration;
    event.matrix_signature = sNdsRendererHardwareMatrixSignature;
    event.no_z_depth_before = sNdsRendererHardwareProjectedDepth;
    event.no_z_depth_after = sNdsRendererHardwareProjectedDepth;
    event.no_z_background_before =
        sNdsRendererHardwareProjectedBackground;
    event.no_z_background_after =
        sNdsRendererHardwareProjectedBackground;

    ndsRendererHardwareRecordOracleTriangle(
        state, indices[0], indices[1], indices[2]);
    stats->hardware_oracle_triangle_count++;
    for (vertex_number = 0u; vertex_number < 3u; vertex_number++)
    {
        u32 vertex_index = indices[vertex_number];
        const NDSRendererInputVertex *vtx =
            &state->input_vertices[vertex_index];
        NDSRendererSemanticVertex *vertex = &event.vertex[vertex_number];

        event.vertex_index[vertex_number] = vertex_index;
        event.vertex_matrix_snapshot[vertex_number] =
            state->vertex_matrix_snapshot[vertex_index];
        event.vertex_clip_snapshot[vertex_number] =
            state->vertex_clip_snapshot[vertex_index];
        vertex->x = (v16)((s32)vtx->x *
            (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)));
        vertex->y = (v16)((s32)vtx->y *
            (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)));
        vertex->z = (v16)((s32)vtx->z *
            (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)));
        vertex->color = state->prepared_vertex_colors[vertex_index];
        vertex->valid_flags =
            NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID |
            NDS_RENDERER_SEMANTIC_VERTEX_COLOR_VALID;
        if (state->texture_prepare_enabled != 0u)
        {
            vertex->s = state->prepared_texcoord_s[vertex_index];
            vertex->t = state->prepared_texcoord_t[vertex_index];
            vertex->valid_flags |= NDS_RENDERER_SEMANTIC_VERTEX_ST_VALID;
            ndsRendererProfileTextureCoord(vertex->s, vertex->t);
            ndsRendererProfileTextureSample(vertex->s, vertex->t);
        }
        ndsRendererProfileVertexRange(
            vtx, vertex->x, vertex->y, vertex->z);
    }
    ndsRendererSemanticCommitEvent(&event);
}
#endif

static inline void ndsRendererFastAccountRawTriangles(
    NDSRendererStats *stats,
    u32 triangle_count,
    u32 reuse_count)
{
    sNdsRendererHardwareSubmitClassCounts[
        NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX] += triangle_count;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    {
        NDSRendererProfileOwnerHotLedger *owner =
            ndsRendererProfileCurrentOwner();

        if (owner != NULL)
        {
            owner->submit_class_count[
                NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX] +=
                    triangle_count;
        }
    }
    gNdsRendererProfileRawCurrentCandidateCount += triangle_count;
    gNdsRendererProfileHardwareBatchReuseCount += reuse_count;
    gNdsRendererProfileTexturePrepareReuseCount += reuse_count;
    gNdsRendererProfileHardwareTriangles += triangle_count;
    gNdsRendererProfileHardwareVertices += triangle_count * 3u;
    if ((gNdsRendererProfileHardwareTriangles > 2048u) ||
        (gNdsRendererProfileHardwareVertices > 6144u))
    {
        gNdsRendererProfileHardwareOverLimit = 1u;
    }
#else
    sNdsRendererRuntimeFrameSummary.raw_current_candidate_count +=
        triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices +=
        triangle_count * 3u;
#endif
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    stats->hardware_zbuffer_triangle_count += triangle_count;
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount += triangle_count;
#endif
    sNdsRendererHardwareSubmitted = TRUE;
}

static inline void ndsRendererFastEmitDirectRawEntry(
    const NDSRendererTraversalState *state,
    const NDSRendererDirectRawEntry *entry,
    u32 textured)
{
    u32 vertex_count = (u32)entry->triangle_count * 3u;
    u32 vertex_index;

    if (textured != 0u)
    {
        for (vertex_index = 0u; vertex_index < vertex_count; vertex_index++)
        {
            ndsRendererFastEmitRawTexturedVertex(
                state, entry->indices[vertex_index]);
        }
    }
    else
    {
        for (vertex_index = 0u; vertex_index < vertex_count; vertex_index++)
        {
            ndsRendererFastEmitRawUntexturedVertex(
                state, entry->indices[vertex_index]);
        }
    }
}

static s32 NDS_RENDERER_FAST_RUN_CODE ndsRendererExecuteDirectRawRemainder(
    const Gfx **dl_io,
    u32 *list_index_io,
    u32 immutable_command_count,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    NDSRendererCommandCallback callback)
{
    const Gfx *source;
    NDSRendererDirectRawPlan *plan;
    u32 first_index;
    u32 remaining_commands;
    u32 available_mask;
    u32 required_mask = 0u;
    u32 entry_number;
    u32 textured;

    if ((callback != NULL) ||
        (ndsRendererFastRawStateEligible(state) == FALSE))
    {
        return FALSE;
    }
    first_index = *list_index_io + 1u;
    if ((first_index >= immutable_command_count) ||
        (first_index >= config->max_list_commands))
    {
        return FALSE;
    }
    remaining_commands = immutable_command_count - first_index;
    if (remaining_commands > (config->max_list_commands - first_index))
    {
        remaining_commands = config->max_list_commands - first_index;
    }
    source = *dl_io + 1;
    plan = ndsRendererDirectRawFindPlan(source, remaining_commands);
    if ((plan == NULL) ||
        ((stats->command_count + plan->command_count) > config->max_commands))
    {
        return FALSE;
    }

    available_mask = state->input_vertex_valid_mask &
        state->raw_vertex_fit_mask & state->current_transform_vertex_mask;
    for (entry_number = 0u;
         entry_number < plan->command_count;
         entry_number++)
    {
        const NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[
                plan->entry_offset + entry_number];

        if ((available_mask & entry->required_mask) != entry->required_mask)
        {
            return FALSE;
        }
        required_mask |= entry->required_mask;
    }

    textured = state->texture_prepare_enabled;
    ndsRendererFastPrepareRawSlots(stats, state, required_mask, textured);
    for (entry_number = 0u;
         entry_number < plan->command_count;
         entry_number++)
    {
        const Gfx *command = source + entry_number;
        const NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[
                plan->entry_offset + entry_number];

        stats->command_count++;
        NDS_RENDERER_RECORD_PROOF_ONLY(
            stats->triangle_command_count++;
            stats->render_command_count++;
        );
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_command_index = first_index + entry_number;
        state->semantic_tri2_half = 0u;
        sNdsRendererProfileTrustedCommandCount++;
        sNdsRendererProfileTriangleRunReuseCount++;
#endif
        ndsRendererFastEmitDirectRawEntry(state, entry, textured);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        {
            u32 w0 = command->words.w0;
            u32 w1 = command->words.w1;
            u32 packed[2];
            u32 triangle_index;

            packed[0] = (entry->triangle_count == 1u) ?
                ndsGBIDecodeF3DEX2Tri1(w0) :
                ndsGBIDecodeF3DEX2Tri2First(w0);
            packed[1] = (entry->triangle_count == 2u) ?
                ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
            for (triangle_index = 0u;
                 triangle_index < entry->triangle_count;
                 triangle_index++)
            {
                u32 indices[3];
                u32 base = triangle_index * 3u;

                indices[0] = entry->indices[base];
                indices[1] = entry->indices[base + 1u];
                indices[2] = entry->indices[base + 2u];
                if (sNdsRendererHardwareNoOracle == 0u)
                {
                    ndsRendererRecordTransformedTriangle(
                        stats, state, packed[triangle_index]);
                }
                state->semantic_tri2_half = triangle_index;
                ndsRendererFastCommitRawSemanticTriangle(
                    stats, state, packed[triangle_index], indices);
            }
        }
#else
        (void)command;
#endif
    }

    stats->triangle_count += plan->triangle_count;
    ndsRendererFastAccountRawTriangles(
        stats, plan->triangle_count, plan->triangle_count);
    sNdsRendererFastRunCount++;
    sNdsRendererFastTriangleCount += plan->triangle_count;
    if ((u32)sNdsRendererRuntimeOwner <
        (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererFastOwnerTriangleCount[
            (u32)sNdsRendererRuntimeOwner] += plan->triangle_count;
    }
    *dl_io = source + plan->command_count - 1u;
    *list_index_io = first_index + plan->command_count - 1u;
    return TRUE;
}

static void NDS_RENDERER_FAST_RUN_CODE ndsRendererExecuteFastRawCurrentRun(
    const Gfx **dl_io,
    u32 *list_index_io,
    u32 immutable_command_count,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 depth,
    NDSRendererCommandCallback callback,
    void *callback_user)
{
    const Gfx *dl = *dl_io;
    u32 list_index = *list_index_io;
    u32 fast_triangles = 0u;
    u32 fallback_state = 0u;
    u32 fallback_vertex = 0u;
    u32 fallback_command = 0u;
    u32 fast_command_count = 0u;

    if (ndsRendererExecuteDirectRawRemainder(
            dl_io, list_index_io, immutable_command_count,
            config, stats, state, callback) != FALSE)
    {
        return;
    }

    while (((list_index + 1u) < immutable_command_count) &&
           ((list_index + 1u) < config->max_list_commands))
    {
        const Gfx *next_dl = dl + 1;
        u32 w0 = next_dl->words.w0;
        u32 w1 = next_dl->words.w1;
        u32 op = w0 >> 24;
        u32 packed[2];
        u32 indices[6];
        u32 required_mask = 0u;
        u32 triangle_count;
        s32 decode_ok;
        s32 state_ok;
        s32 vertex_ok;
        NDSRendererCommand command;

        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            break;
        }
        if (stats->command_count >= config->max_commands)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
            break;
        }

        list_index++;
        dl = next_dl;
        stats->command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_command_index = list_index;
        state->semantic_tri2_half = 0u;
        sNdsRendererProfileTrustedCommandCount++;
        sNdsRendererProfileTriangleRunReuseCount++;
#endif
        if (callback != NULL)
        {
            memset(&command, 0, sizeof(command));
            command.dl = dl;
            command.w0 = w0;
            command.w1 = w1;
            command.op = op;
            command.depth = depth;
            command.list_index = list_index;
            command.transformed_vertices = state->vertices;
            command.transformed_vertex_valid_mask = state->vertex_valid_mask;
            command.matrix_valid = state->matrix_valid;
            if (callback(&command, callback_user) == FALSE)
            {
                ndsRendererRecordUnsupported(stats, op);
                stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
                break;
            }
        }
        triangle_count = (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        packed[0] = (op == NDS_RENDERER_OP_TRI1) ?
            ndsGBIDecodeF3DEX2Tri1(w0) :
            ndsGBIDecodeF3DEX2Tri2First(w0);
        packed[1] = (triangle_count == 2u) ?
            ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
        decode_ok = ndsRendererFastDecodeTriangle(
            packed[0], &indices[0], &required_mask);
        if ((decode_ok != FALSE) && (triangle_count == 2u))
        {
            decode_ok = ndsRendererFastDecodeTriangle(
                packed[1], &indices[3], &required_mask);
        }
        state_ok = ndsRendererFastRawStateEligible(state);
        vertex_ok = ((decode_ok != FALSE) &&
                     ((state->input_vertex_valid_mask & required_mask) ==
                      required_mask) &&
                     ((state->raw_vertex_fit_mask & required_mask) ==
                      required_mask) &&
                     ((state->current_transform_vertex_mask & required_mask) ==
                      required_mask)) ? TRUE : FALSE;

        if ((state_ok != FALSE) && (vertex_ok != FALSE))
        {
            u32 textured = state->texture_prepare_enabled;
            u32 triangle_index;

            NDS_RENDERER_RECORD_PROOF_ONLY(
                stats->triangle_command_count++;
                stats->render_command_count++;
            );
            stats->triangle_count += triangle_count;
            ndsRendererFastPrepareRawSlots(
                stats, state, required_mask, textured);
            for (triangle_index = 0u;
                 triangle_index < triangle_count;
                 triangle_index++)
            {
                const u32 *triangle = &indices[triangle_index * 3u];

                if (sNdsRendererHardwareNoOracle == 0u)
                {
                    ndsRendererRecordTransformedTriangle(
                        stats, state, packed[triangle_index]);
                }
                ndsRendererFastEmitRawCommand(
                    state, triangle, 1u, textured);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                state->semantic_tri2_half = triangle_index;
                ndsRendererFastCommitRawSemanticTriangle(
                    stats, state, packed[triangle_index], triangle);
#endif
            }
            fast_triangles += triangle_count;
            fast_command_count++;
        }
        else
        {
            if (decode_ok == FALSE)
            {
                fallback_command++;
            }
            else if (state_ok == FALSE)
            {
                fallback_state++;
            }
            else
            {
                fallback_vertex++;
            }
            ndsRendererFastRawFallbackCommand(
                stats, config, state, op, w0, w1);
        }
    }

    if (fast_triangles != 0u)
    {
        ndsRendererFastAccountRawTriangles(
            stats, fast_triangles, fast_triangles);
        sNdsRendererFastRunCount++;
        sNdsRendererFastTriangleCount += fast_triangles;
        if ((u32)sNdsRendererRuntimeOwner <
            (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
        {
            sNdsRendererFastOwnerTriangleCount[
                (u32)sNdsRendererRuntimeOwner] += fast_triangles;
        }
    }
    (void)fast_command_count;
    sNdsRendererFastFallbackCount[0] += fallback_state;
    sNdsRendererFastFallbackCount[1] += fallback_vertex;
    sNdsRendererFastFallbackCount[2] += fallback_command;
    *dl_io = dl;
    *list_index_io = list_index;
}

static inline void ndsRendererNativeSourceBoundary(
    NDSRendererTraversalState *state)
{
    ndsRendererHardwareEndBatch();
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_projected_xy_valid_mask = 0u;
    state->prepared_projected_source_z_valid_mask = 0u;
}

static s32 ndsRendererNativePrepareDirectRun(
    const NDSNativeRun *run,
    u32 first_vertex,
    s32 projected_run,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const NDSRendererInputVertex *v0;
    const NDSRendererTileState *render_tile;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 material_color;
    u32 poly_alpha;
    u32 poly_fmt;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 implicit_texture_on;
    s32 use_texture;
    s32 texture_offset;
    u32 available_mask;

    if ((run == NULL) || (config == NULL) || (stats == NULL) ||
        (state == NULL) || (first_vertex >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u) ||
        ((stats->othermode_l & NDS_RENDERER_ZMODE_DEC) ==
         NDS_RENDERER_ZMODE_DEC) ||
        (ndsRendererHardwareUsePrimDepth(stats) != FALSE) ||
        ((projected_run == FALSE) &&
         (ndsRendererHardwareRawMatrixCompatible(state) == FALSE)))
    {
        return FALSE;
    }
    available_mask = state->input_vertex_valid_mask &
        state->raw_vertex_fit_mask;
    if (projected_run == FALSE)
    {
        available_mask &= state->current_transform_vertex_mask;
    }
    if ((available_mask & run->required_mask) != run->required_mask)
    {
        return FALSE;
    }
    v0 = &state->input_vertices[first_vertex];
    if (projected_run != FALSE)
    {
        u32 missing = run->required_mask;

        while (missing != 0u)
        {
            u32 index = (u32)__builtin_ctz(missing);

            if (ndsRendererEnsureTransformedVertex(
                    stats, state, index) == FALSE)
            {
                return FALSE;
            }
            missing &= ~(1u << index);
        }
    }
    state->texture_prepare_source_zbuffered = TRUE;
    state->texture_prepare_decal_depth = FALSE;
    state->texture_prepare_prim_depth = FALSE;

#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (state->texture_prepare_valid == 0u)
#endif
    {
        material_color = ndsRendererHardwareColorSource(stats);
        use_material_color = ndsRendererHardwareUseMaterialColor(stats);
        use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
        state->texture_prepare_material_color = material_color;
        state->texture_prepare_vertex_flags =
            ((use_material_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
            ((use_vertex_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (stats->texture_combine_count != 0u)
        {
            if (ndsRendererHardwareLitShadeCombine(stats) != FALSE)
            {
                gNdsRendererProfileLitShadeCombineCount++;
            }
            if (use_material_color != FALSE)
            {
                gNdsRendererProfileMaterialCombineCount++;
            }
        }
#endif
    }
    if (state->texture_prepare_valid == 0u)
    {
        poly_alpha = ndsRendererHardwareAlpha(stats, v0);
        state->texture_prepare_alpha_constant =
            (ndsRendererHardwareAlphaUsesVertex(stats) == FALSE) ?
                TRUE : FALSE;
        if ((state->texture_prepare_alpha_constant == 0u) ||
            (poly_alpha == 0u))
        {
            return FALSE;
        }
        state->texture_prepare_poly_alpha = poly_alpha;
        state->texture_prepare_poly_fmt =
            ndsRendererHardwarePolyFmt(stats, poly_alpha);

        render_tile =
            &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
        implicit_texture_on =
            ndsRendererHardwareTextureImplicitStateOn(stats);
        use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ?
            ndsRendererHardwareBindTexture(stats, config, state) : FALSE;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->texture_prepare_key_hash = (use_texture != FALSE) ?
            sNdsRendererSemanticLastTextureKeyHash : 0u;
        state->texture_prepare_params = (use_texture != FALSE) ?
            sNdsRendererSemanticLastTextureParams : 0u;
#endif
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled =
            (use_texture != FALSE) ? TRUE : FALSE;
        state->texture_prepare_name = (use_texture != FALSE) ?
            sNdsRendererHardwareBoundTextureName : 0u;
        ndsRendererProfileRecordTexturePrepare();

        texture_scale_s = stats->texture_scale_s;
        texture_scale_t = stats->texture_scale_t;
        if ((use_texture != FALSE) && (implicit_texture_on != FALSE))
        {
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
            {
                texture_scale_s = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
            {
                texture_scale_t = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
        }
        texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
        state->texture_prepare_scale_s = texture_scale_s;
        state->texture_prepare_scale_t = texture_scale_t;
        state->texture_prepare_origin_s = render_tile->uls;
        state->texture_prepare_origin_t = render_tile->ult;
        state->texture_prepare_offset = texture_offset;
        if (use_texture != FALSE)
        {
            state->texture_prepare_vertex_flags |=
                NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        }
    }
    else
    {
        use_texture =
            (state->texture_prepare_enabled != 0u) ? TRUE : FALSE;
        ndsRendererProfileRecordTexturePrepareReuse();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (use_texture != FALSE)
        {
            state->texture_prepare_vertex_flags |=
                NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        }
#endif
    }
#if NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY
    if (use_texture != FALSE)
    {
        state->texture_prepare_vertex_flags &=
            ~(NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL |
              NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX);
    }
#endif

    state->texture_prepare_vertex_flags =
        (state->texture_prepare_vertex_flags &
         NDS_RENDERER_VERTEX_CONTEXT_PREPARED_MASK) |
        ((projected_run != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH :
             (NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD |
              NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED));
    ndsRendererFastPrepareRawSlots(
        stats, state, run->required_mask,
        state->texture_prepare_enabled);
    if (projected_run != FALSE)
    {
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
        poly_fmt = state->texture_prepare_poly_fmt;
        ndsRendererHardwareBeginTriangleBatch(
            stats, (use_texture != FALSE) ? TRUE : FALSE,
            state->texture_prepare_name, poly_fmt,
            sNdsRendererHardwareMatrixMode,
            sNdsRendererHardwareMatrixGeneration);
        return TRUE;
    }
    ndsRendererLoadHardwareMatrices(state, TRUE);
    poly_fmt = state->texture_prepare_poly_fmt;
    ndsRendererHardwareBeginTriangleBatch(
        stats, (use_texture != FALSE) ? TRUE : FALSE,
        state->texture_prepare_name, poly_fmt,
        sNdsRendererHardwareMatrixMode,
        sNdsRendererHardwareMatrixGeneration);
    return ndsRendererFastRawStateEligible(state);
}

static void __attribute__((noinline, optimize("Os")))
ndsRendererNativeApplyRootLightPreamble(
    const NDSNativeRoot *root, NDSRendererStats *stats)
{
    if (root->light_preamble == 0u)
    {
        return;
    }
    /* Each source gSPLightColor expands to its A/B G_MW_LIGHTCOL pair. */
    stats->light_color_1 = NDS_NATIVE_ROOT_LIGHT1;
    stats->light_color_2 = (root->light_preamble == 1u) ?
        NDS_NATIVE_ROOT_LIGHT2_1 : NDS_NATIVE_ROOT_LIGHT2_2;
    stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_1_MASK |
        NDS_RENDERER_LIGHT_COLOR_2_MASK;
    stats->light_color_command_count += 4u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileLightColorCommands += 4u;
#endif
}

static void ndsRendererNativeApplyStateDelta(
    const NDSNativeStateDelta *delta,
    const u8 *asset_base,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    if ((delta == NULL) || (stats == NULL) || (state == NULL))
    {
        return;
    }
    switch (delta->effect)
    {
    case NDS_NATIVE_STATE_OTHERMODE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordOtherMode(
            stats, delta->w0 >> 24, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_COMBINE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordSetCombine(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_TEXTURE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordTextureState(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_GEOMETRY:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->geometry_mode =
            (stats->geometry_mode & delta->w0) | delta->w1;
        stats->geometry_clear_mask = delta->w0;
        stats->geometry_set_mask = delta->w1;
        stats->geometry_command_count++;
        break;
    case NDS_NATIVE_STATE_IMAGE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordSetImage(
            stats, delta->w0,
            (u32)(uintptr_t)(asset_base + delta->w1));
        break;
    case NDS_NATIVE_STATE_TILE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordSetTile(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_LOAD_TLUT:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordLoadTlut(stats, delta->w1);
        break;
    case NDS_NATIVE_STATE_LOAD_BLOCK:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordLoadBlock(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_TILE_SIZE:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererRecordSetTileSize(stats, delta->w0, delta->w1);
        break;
    case NDS_NATIVE_STATE_PRIM:
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->prim_color = delta->w1;
        stats->prim_min_level = (delta->w0 >> 8) & 0xffu;
        stats->prim_lod_fraction = delta->w0 & 0xffu;
        stats->color_command_count++;
        break;
    case NDS_NATIVE_STATE_LIGHT_COLOR:
        ndsRendererApplyMatrixMoveWordCommand(
            stats, state, delta->w0, delta->w1);
        break;
    default:
        break;
    }
}

static void ndsRendererNativeApplyStateSpan(
    u16 first,
    u32 count,
    u32 sync_count,
    const u8 *asset_base,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 i;

    if ((count == 0u) && (sync_count == 0u))
    {
        return;
    }
    ndsRendererNativeSourceBoundary(state);
    stats->sync_command_count += sync_count;
    if ((count == 0u) || (first == NDS_NATIVE_STATE_NONE))
    {
        return;
    }
    for (i = 0u; i < count; i++)
    {
        u32 delta_index = sNdsNativeFighterStateSequence[first + i];

        ndsRendererNativeApplyStateDelta(
            &sNdsNativeFighterStateDeltas[delta_index],
            asset_base, stats, state);
    }
}

static void ndsRendererNativeApplyMaterial(
    const NDSRendererNativeMaterial *material,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 effects;

    if ((material == NULL) || (stats == NULL) || (state == NULL))
    {
        return;
    }
    ndsRendererNativeSourceBoundary(state);
    effects = material->effects;
    /* Root DE call + generated segment-E table DE jump. The root command is
     * already included in the generated source count; only the table word
     * and typed branch body are additional commands. */
    stats->command_count += (u32)material->command_count + 1u;
    stats->branch_command_count += 2u;
    stats->branch_call_count++;
    stats->branch_jump_count++;
    stats->segment_resolve_count++;
    stats->end_command_count++;
    stats->sync_command_count += material->sync_count;

    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->palette_image_w0, material->palette_image);
        ndsRendererRecordSetImage(
            stats, material->palette_image_w0, material->palette_image);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->palette_tile_w0, material->palette_tile_w1);
        ndsRendererRecordSetTile(
            stats, material->palette_tile_w0, material->palette_tile_w1);
        ndsRendererTextureSourceHashCommand(
            stats, NDS_RENDERER_OP_LOADTLUT << 24,
            material->palette_tlut_w1);
        ndsRendererRecordLoadTlut(stats, material->palette_tlut_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LIGHT1) != 0u)
    {
        ndsRendererRecordLightColor(stats, 1u, material->light1);
        ndsRendererRecordLightColor(stats, 1u, material->light1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LIGHT2) != 0u)
    {
        ndsRendererRecordLightColor(stats, 2u, material->light2);
        ndsRendererRecordLightColor(stats, 2u, material->light2);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_PRIM) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->prim_color = material->prim_w1;
        stats->prim_min_level = (material->prim_w0 >> 8) & 0xffu;
        stats->prim_lod_fraction = material->prim_w0 & 0xffu;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_ENV) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        stats->env_color = material->env_color;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_BLEND) != 0u)
    {
        stats->blend_color = material->blend_color;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->block_image_w0, material->block_image);
        ndsRendererRecordSetImage(
            stats, material->block_image_w0, material->block_image);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LOAD_BLOCK) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->load_block_w0, material->load_block_w1);
        ndsRendererRecordLoadBlock(
            stats, material->load_block_w0, material->load_block_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->current_image_w0, material->current_image);
        ndsRendererRecordSetImage(
            stats, material->current_image_w0, material->current_image);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->render_tile_size_w0,
            material->render_tile_size_w1);
        ndsRendererRecordSetTileSize(
            stats, material->render_tile_size_w0,
            material->render_tile_size_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_SCROLL_TILE_SIZE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->scroll_tile_size_w0,
            material->scroll_tile_size_w1);
        ndsRendererRecordSetTileSize(
            stats, material->scroll_tile_size_w0,
            material->scroll_tile_size_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_TEXTURE) != 0u)
    {
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        ndsRendererTextureSourceHashCommand(
            stats, material->texture_w0, material->texture_w1);
        ndsRendererRecordTextureState(
            stats, material->texture_w0, material->texture_w1);
    }
}

/* The hierarchy candidate replays the complete retained owner into private
 * scratch before its first GX command.  The shared state/material helpers only
 * touch GX through their source-boundary batch close, so mask the batch-open
 * bit around those exact helpers and restore it immediately.  Existing mode-8
 * execution keeps calling the original helpers byte-for-byte. */
static void ndsRendererNativeApplyStateSpanPreflight(
    u16 first,
    u32 count,
    u32 sync_count,
    const u8 *asset_base,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 batch_open = sNdsRendererHardwareTriangleBatchOpen;

    sNdsRendererHardwareTriangleBatchOpen = FALSE;
    ndsRendererNativeApplyStateSpan(
        first, count, sync_count, asset_base, stats, state);
    sNdsRendererHardwareTriangleBatchOpen = batch_open;
}

static void ndsRendererNativeApplyMaterialPreflight(
    const NDSRendererNativeMaterial *material,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 batch_open = sNdsRendererHardwareTriangleBatchOpen;

    sNdsRendererHardwareTriangleBatchOpen = FALSE;
    ndsRendererNativeApplyMaterial(material, stats, state);
    sNdsRendererHardwareTriangleBatchOpen = batch_open;
}

static s32 ndsRendererNativeVisitSourceCommand(
    const u8 *root_base,
    u32 command_index,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const Gfx *dl;
    NDSRendererCommand command;

    if (callback == NULL)
    {
        return TRUE;
    }
    dl = (const Gfx *)(root_base + (command_index * sizeof(*dl)));
    memset(&command, 0, sizeof(command));
    command.dl = dl;
    command.w0 = dl->words.w0;
    command.w1 = dl->words.w1;
    command.op = command.w0 >> 24;
    command.list_index = command_index;
    command.transformed_vertices = state->vertices;
    command.transformed_vertex_valid_mask = state->vertex_valid_mask;
    command.matrix_valid = state->matrix_valid;
    if (callback(&command, callback_user) == FALSE)
    {
        ndsRendererRecordUnsupported(stats, command.op);
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return FALSE;
    }
    return TRUE;
}

static void ndsRendererNativeLoadVertexBlock(
    const u8 *src,
    u32 v0,
    u32 count,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const NDSRendererHardwareLightDirection *prepared_light_direction = NULL;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    const u32 *prepared_light_shade_lut = NULL;
#endif
    u32 matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    u32 i;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    stats->source_vertex_count += count;
#endif
    if ((v0 + count) > stats->vertex_count)
    {
        stats->vertex_count = v0 + count;
    }
    if (state->matrix_valid != 0u)
    {
        matrix_snapshot = ndsRendererAcquireCurrentMatrixSnapshot(state);
    }
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        if (state->prepared_light_direction_valid == 0u)
        {
            ndsRendererHardwarePrepareLitDirection(
                stats,
                (state->modelview_valid != 0u) ? &state->modelview : NULL,
                &state->prepared_light_direction);
            state->prepared_light_direction_valid = TRUE;
        }
        prepared_light_direction = &state->prepared_light_direction;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((stats->light_color_mask &
             (NDS_RENDERER_LIGHT_COLOR_1_MASK |
              NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
            (NDS_RENDERER_LIGHT_COLOR_1_MASK |
             NDS_RENDERER_LIGHT_COLOR_2_MASK))
        {
            prepared_light_shade_lut = ndsRendererHardwareGetLightShadeLut(
                stats->light_color_1, stats->light_color_2);
        }
#endif
    }
    for (i = 0u; i < count; i++)
    {
        u32 index = v0 + i;
        u32 mask = 1u << index;
        NDSRendererInputVertex *input = &state->input_vertices[index];

        ndsRendererDecodeInputVertex(input, src + (i * 16u));
        state->input_vertex_valid_mask |= mask;
        if (ndsRendererHardwareRawVertexFits(input) != FALSE)
        {
            state->raw_vertex_fit_mask |= mask;
        }
        else
        {
            state->raw_vertex_fit_mask &= ~mask;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (prepared_light_shade_lut != NULL)
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorLut(
                    input, prepared_light_direction,
                    prepared_light_shade_lut);
        }
        else
#endif
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorPrepared(
                    stats, input, prepared_light_direction);
        }
        state->vertex_color_valid_mask |= mask;
        state->vertex_matrix_snapshot[index] = (u8)matrix_snapshot;
        state->vertex_clip_snapshot[index] =
            NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
        state->vertex_valid_mask &= ~mask;
        state->current_transform_vertex_mask &= ~mask;
        if (state->matrix_valid != 0u)
        {
            state->current_transform_vertex_mask |= mask;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (state->matrix_valid != 0u)
        {
            (void)ndsRendererTransformCachedVertex(
                stats, state, index, &state->matrix, matrix_snapshot);
        }
#else
        if ((state->matrix_valid != 0u) &&
            (matrix_snapshot == NDS_RENDERER_MATRIX_SNAPSHOT_INVALID))
        {
            (void)ndsRendererTransformCachedVertex(
                stats, state, index, &state->matrix, matrix_snapshot);
        }
#endif
    }
    sNdsRendererHardwareSourceVertexLoadCount += count;
}

static void ndsRendererNativeApplyVertexActions(
    const NDSNativeEpoch *epoch,
    const u8 *asset_base,
    const u8 *root_base,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 i;

    if (epoch->action_count == 0u)
    {
        return;
    }
    ndsRendererNativeSourceBoundary(state);
    for (i = 0u; i < epoch->action_count; i++)
    {
        const NDSNativeVertexAction *action =
            &sNdsNativeFighterVertexActions[epoch->first_action + i];

        if (ndsRendererNativeVisitSourceCommand(
                root_base, action->command_index, callback, callback_user,
                stats, state) == FALSE)
        {
            return;
        }
        if (action->kind == NDS_NATIVE_VERTEX_BLOCK)
        {
            ndsRendererNativeLoadVertexBlock(
                asset_base + action->source_offset,
                action->index, action->count, stats, state);
        }
        else if ((action->kind == NDS_NATIVE_MODIFY_ST) &&
                 (action->index < NDS_RENDERER_MAX_VTX) &&
                 ((state->input_vertex_valid_mask &
                   (1u << action->index)) != 0u))
        {
            state->input_vertices[action->index].s = action->s;
            state->input_vertices[action->index].t = action->t;
        }
    }
}

static void ndsRendererNativeSubmitGenericTriangle(
    u32 packed,
    u32 command_index,
    u32 tri2_half,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->semantic_command_index = command_index;
    state->semantic_tri2_half = tri2_half;
#else
    (void)command_index;
    (void)tri2_half;
#endif
    if (sNdsRendererHardwareNoOracle == 0u)
    {
        ndsRendererRecordTransformedTriangle(
            stats, state, packed);
    }
    ndsRendererSubmitHardwareTriangle(
        stats, config, state, packed);
}

static inline u32 ndsRendererNativeDecodeTriangle(
    u16 encoded,
    u32 indices[3])
{
    u32 compact = (u32)encoded & 0x7fffu;

    indices[0] = (compact >> 10) & 31u;
    indices[1] = (compact >> 5) & 31u;
    indices[2] = compact & 31u;
    return (indices[0] << 17) |
           (indices[1] << 9) |
           (indices[2] << 1);
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static u32 ndsRendererNativeNormalizeGeometryMode(u32 mode)
{
    u32 source_cull = mode &
        (NDS_NATIVE_SOURCE_GEOM_CULL_FRONT |
         NDS_NATIVE_SOURCE_GEOM_CULL_BACK);

    if ((source_cull & NDS_NATIVE_SOURCE_GEOM_CULL_FRONT) != 0u)
    {
        mode |= NDS_RENDERER_GEOM_CULL_FRONT;
    }
    if ((source_cull & NDS_NATIVE_SOURCE_GEOM_CULL_BACK) != 0u)
    {
        mode |= NDS_RENDERER_GEOM_CULL_BACK;
    }
    return mode &
        ~(NDS_NATIVE_SOURCE_GEOM_CULL_FRONT |
          NDS_NATIVE_SOURCE_GEOM_CULL_BACK);
}

static s32 ndsRendererNativeDirectReject(NDSRendererStats *stats)
{
    if (stats != NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
    }
    return FALSE;
}

static inline void ndsRendererNativeBeginDirectBatch(
    const NDSRendererStats *stats,
    u32 textured,
    u32 texture_name,
    u32 poly_fmt,
    u32 matrix_generation)
{
    if ((sNdsRendererHardwareTriangleBatchOpen != 0u) &&
        (sNdsRendererHardwareTriangleBatchTextured == textured) &&
        (sNdsRendererHardwareTriangleBatchTextureName == texture_name) &&
        (sNdsRendererHardwareTriangleBatchPolyFmt == poly_fmt) &&
        (sNdsRendererHardwareTriangleBatchMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
        (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
         matrix_generation))
    {
        ndsRendererProfileRecordBatchReuse();
        return;
    }

    ndsRendererHardwareEndBatch();
    glEnable(GL_TEXTURE_2D);
    if (textured == 0u)
    {
        ndsRendererHardwareBindNoTexture(NULL);
    }
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    ndsRendererHardwareSetPolyFmt(poly_fmt);
    glBegin(GL_TRIANGLE);
    ndsRendererProfileRecordBatchBegin();

    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = textured;
    sNdsRendererHardwareTriangleBatchTextureName = texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = 0u;
    sNdsRendererHardwareTriangleBatchFogKey = 0u;
    sNdsRendererHardwareTriangleBatchMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED;
    sNdsRendererHardwareTriangleBatchMatrixGeneration = matrix_generation;
    (void)stats;
}

static void ndsRendererNativeApplyProductionPreamble(
    const NDSRendererNativeFighterPreamble *preamble,
    NDSRendererStats *stats)
{
    if ((preamble == NULL) || (stats == NULL) ||
        ((preamble->flags & NDS_RENDERER_NATIVE_PREAMBLE_VALID) == 0u))
    {
        return;
    }

    stats->geometry_mode = ndsRendererNativeNormalizeGeometryMode(
        preamble->geometry_mode);
    stats->othermode_h =
        (stats->othermode_h & ~NDS_RENDERER_CYCLETYPE_MASK) |
        (preamble->cycle_type & NDS_RENDERER_CYCLETYPE_MASK);
    stats->othermode_l = preamble->render_mode;
    stats->prim_color = preamble->prim_color;
    stats->env_color = preamble->env_color;

    if ((preamble->flags &
         NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID) != 0u)
    {
        stats->light_dir_x = preamble->light_dir_x;
        stats->light_dir_y = preamble->light_dir_y;
        stats->light_dir_z = preamble->light_dir_z;
        stats->light_dir_mask = NDS_RENDERER_LIGHT_DIR_1_MASK;
    }
}

static void ndsRendererNativeBindProductionRoot(
    NDSRendererTraversalState *state,
    const NDSRendererNativeFighterRoot *input,
    NDSRendererStats *stats)
{
    ndsRendererNativeApplyProductionPreamble(&input->preamble, stats);
    state->modelview_stack_depth = 0u;
    state->vertex_valid_mask = 0u;
    state->input_vertex_valid_mask = 0u;
    state->vertex_color_valid_mask = 0u;
    state->current_transform_vertex_mask = 0u;
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_projected_xy_valid_mask = 0u;
    state->prepared_projected_source_z_valid_mask = 0u;
    state->prepared_light_direction_valid = 0u;
    state->texture_prepare_valid = 0u;
    state->projection_valid = 0u;
    state->modelview = *input->modelview_matrix;
    state->modelview_valid = TRUE;
    state->matrix = *input->composed_matrix;
    state->matrix_valid = TRUE;
    state->matrix_word_valid = FALSE;
    state->matrix_generation = ndsRendererNextMatrixGeneration();
    stats->hardware_matrix_seed_count++;
}

static s32 ndsRendererNativePreflightProductionOwner(
    u32 slot,
    const void *asset_base,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count,
    NDSRendererCommandCallback callback,
    NDSRendererStats *stats)
{
    const NDSNativeRoot *roots;
    u32 root_count;
    u32 root_index;

    if ((slot > 1u) || (asset_base == NULL) || (inputs == NULL) ||
        (stats == NULL) || (callback != NULL) ||
        (sNdsNativeFighterOwnerExecution.active != 0u))
    {
        return FALSE;
    }
    if (slot == 0u)
    {
        roots = sNdsNativeMarioRoots;
        root_count = sizeof(sNdsNativeMarioRoots) /
            sizeof(sNdsNativeMarioRoots[0]);
    }
    else
    {
        roots = sNdsNativeFoxRoots;
        root_count = sizeof(sNdsNativeFoxRoots) /
            sizeof(sNdsNativeFoxRoots[0]);
    }
    if (input_count != root_count)
    {
        return FALSE;
    }
    for (root_index = 0u; root_index < root_count; root_index++)
    {
        const NDSRendererNativeFighterRoot *input = &inputs[root_index];
        const NDSNativeRoot *root = &roots[root_index];
        u32 epoch_index;

        if ((input->root_offset != root->root_offset) ||
            (input->composed_matrix == NULL) ||
            (input->modelview_matrix == NULL) ||
            (input->config == NULL) ||
            ((input->preamble.flags &
              NDS_RENDERER_NATIVE_PREAMBLE_VALID) == 0u))
        {
            return FALSE;
        }
#if NDS_RENDERER_M2_DETAILED_LEDGER
        if ((input->owner_generation == 0u) ||
            (input->owner_generation != inputs[0].owner_generation))
        {
            return FALSE;
        }
#endif
        for (epoch_index = 0u;
             epoch_index < root->epoch_count;
             epoch_index++)
        {
            const NDSNativeEpoch *epoch =
                &sNdsNativeFighterEpochs[root->first_epoch + epoch_index];

            if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
                ((input->materials == NULL) ||
                 (epoch->material_slot >= input->material_count)))
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static s32 NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativeShadeProductionActions(
    const NDSNativeEpoch *epoch,
    u32 epoch_policy,
    u32 packet_mode,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const NDSNativeDirectPolicy *policy =
        &sNdsNativeFighterDirectPolicies[
            epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK];
    const NDSRendererHardwareLightDirection *prepared_direction = NULL;
    const u32 *shade_lut = NULL;
    u32 use_material =
        policy->vertex_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL;
    u32 material_color = (use_material != 0u) ? stats->prim_color : 0u;
    u32 action_offset;

    if (epoch->action_count == 0u)
    {
        return TRUE;
    }
    ndsRendererNativeSourceBoundary(state);
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        if (state->prepared_light_direction_valid == 0u)
        {
            ndsRendererHardwarePrepareLitDirection(
                stats, &state->modelview,
                &state->prepared_light_direction);
            state->prepared_light_direction_valid = TRUE;
        }
        prepared_direction = &state->prepared_light_direction;
        if ((stats->light_color_mask &
             (NDS_RENDERER_LIGHT_COLOR_1_MASK |
              NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
            (NDS_RENDERER_LIGHT_COLOR_1_MASK |
             NDS_RENDERER_LIGHT_COLOR_2_MASK))
        {
            if (packet_mode != 0u)
            {
                shade_lut = ndsRendererHardwareFindLightShadeLut(
                    stats->light_color_1, stats->light_color_2);
                if (shade_lut == NULL)
                {
                    return FALSE;
                }
            }
            else
            {
                shade_lut = ndsRendererHardwareGetLightShadeLut(
                    stats->light_color_1, stats->light_color_2);
            }
        }
    }

    for (action_offset = 0u;
         action_offset < epoch->action_count;
         action_offset++)
    {
        u32 action_index = epoch->first_action + action_offset;
        const NDSNativeVertexAction *action =
            &sNdsNativeFighterVertexActions[action_index];
        u32 span = sNdsNativeFighterActionDenseSpans[action_index];
        u32 dense_first = span & NDS_NATIVE_DENSE_ID_MASK;
        u32 dense_count = span >> NDS_NATIVE_DENSE_SPAN_COUNT_SHIFT;
        u32 dense_offset;

        if (action->kind == NDS_NATIVE_VERTEX_BLOCK)
        {
            u32 vertex_end = (u32)action->index + (u32)action->count;

            if (vertex_end > stats->vertex_count)
            {
                stats->vertex_count = vertex_end;
            }
            if (packet_mode == 0u)
            {
                sNdsRendererHardwareSourceVertexLoadCount += action->count;
            }
        }
        for (dense_offset = 0u;
             dense_offset < dense_count;
             dense_offset++)
        {
            u32 dense_id = dense_first + dense_offset;
            u32 color_source =
                sNdsNativeFighterDenseColorSource[dense_id];

            if (color_source != dense_id)
            {
                sNdsNativeFighterPreparedDense[dense_id].shaded_rgba =
                    sNdsNativeFighterPreparedDense[color_source].shaded_rgba;
            }
            else
            {
                const NDSNativeDenseVertex *dense =
                    &sNdsNativeFighterDenseVertices[dense_id];
                NDSRendererInputVertex input;

                input.r = (u8)(dense->rgba >> 24);
                input.g = (u8)(dense->rgba >> 16);
                input.b = (u8)(dense->rgba >> 8);
                input.a = (u8)dense->rgba;
                if (shade_lut != NULL)
                {
                    sNdsNativeFighterPreparedDense[dense_id].shaded_rgba =
                        ndsRendererHardwareLitShadeColorLut(
                            &input, prepared_direction, shade_lut);
                }
                else
                {
                    sNdsNativeFighterPreparedDense[dense_id].shaded_rgba =
                        ndsRendererHardwareLitShadeColorPrepared(
                            stats, &input, prepared_direction);
                }
            }
            {
                NDSNativePreparedDenseVertex *prepared =
                    &sNdsNativeFighterPreparedDense[dense_id];
                u32 color = prepared->shaded_rgba;

                if (use_material != 0u)
                {
                    u32 r = ndsRendererHardwareScaleMaterialChannel5(
                        (color >> 24) & 0xffu,
                        (material_color >> 24) & 0xffu);
                    u32 g = ndsRendererHardwareScaleMaterialChannel5(
                        (color >> 16) & 0xffu,
                        (material_color >> 16) & 0xffu);
                    u32 b = ndsRendererHardwareScaleMaterialChannel5(
                        (color >> 8) & 0xffu,
                        (material_color >> 8) & 0xffu);

                    prepared->packed_color = RGB15(r, g, b);
                }
                else
                {
                    prepared->packed_color =
                        RGB15((color >> 27) & 0x1fu,
                              (color >> 19) & 0x1fu,
                              (color >> 11) & 0x1fu);
                }
            }
        }
    }
    return TRUE;
}

static s32 NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativePrepareProductionRun(
    u32 run_index,
    u32 epoch_policy,
    u32 packet_mode,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    NDSNativeHierarchyPreparedRun *hierarchy_run)
{
    const NDSNativeDirectPolicy *policy;
    const NDSRendererTileState *render_tile;
    u32 family = epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK;
    u32 expected_geometry_cull =
        ((epoch_policy & NDS_NATIVE_DIRECT_POLICY_CULL_NONE) != 0u) ?
            0u : NDS_RENDERER_GEOM_CULL_BACK;
    u32 expected_poly_cull =
        ((epoch_policy & NDS_NATIVE_DIRECT_POLICY_CULL_NONE) != 0u) ?
            POLY_CULL_NONE : POLY_CULL_BACK;
    u32 geometry_cull;
    u32 material_color;
    u32 use_texture;
    u32 texture_scale_s = 0u;
    u32 texture_scale_t = 0u;
    u32 texture_origin_s = 0u;
    u32 texture_origin_t = 0u;
    u32 unique_first;
    u32 unique_count;
    u32 unique_offset;
    s32 texture_offset = 0;
    NDSRendererHardwareResolvedTexture resolved_texture;

    memset(&resolved_texture, 0, sizeof(resolved_texture));

    if ((config == NULL) || (stats == NULL) || (state == NULL) ||
        (family >= (sizeof(sNdsNativeFighterDirectPolicies) /
                    sizeof(sNdsNativeFighterDirectPolicies[0]))))
    {
        return ndsRendererNativeDirectReject(stats);
    }
    policy = &sNdsNativeFighterDirectPolicies[family];
    geometry_cull = stats->geometry_mode &
        (NDS_RENDERER_GEOM_CULL_FRONT | NDS_RENDERER_GEOM_CULL_BACK);
    if (((stats->geometry_mode &
          (NDS_RENDERER_GEOM_ZBUFFER | NDS_RENDERER_GEOM_LIGHTING)) !=
         (NDS_RENDERER_GEOM_ZBUFFER | NDS_RENDERER_GEOM_LIGHTING)) ||
        ((stats->geometry_mode &
          (NDS_RENDERER_GEOM_FOG |
           NDS_RENDERER_GEOM_TEXTURE_GEN |
           NDS_RENDERER_GEOM_TEXTURE_GEN_LINEAR)) != 0u) ||
        (geometry_cull != expected_geometry_cull) ||
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) != 0u) ||
        ((stats->othermode_l & NDS_RENDERER_ZMODE_MASK) ==
         NDS_RENDERER_ZMODE_DEC) ||
        ((stats->othermode_l & NDS_RENDERER_ZSOURCE_MASK) != 0u) ||
        (stats->texture_combine_w0 != policy->combine_w0) ||
        (stats->texture_combine_w1 != policy->combine_w1) ||
        ((family != NDS_NATIVE_DIRECT_POLICY_LIT_ONLY) &&
         (stats->env_color != 0xffffffffu)) ||
        (state->matrix_valid == 0u) ||
        (state->matrix_generation == 0u))
    {
        return ndsRendererNativeDirectReject(stats);
    }

    material_color =
        ((policy->vertex_flags &
          NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL) != 0u) ?
            stats->prim_color : 0u;
    state->texture_prepare_source_zbuffered = TRUE;
    state->texture_prepare_decal_depth = FALSE;
    state->texture_prepare_prim_depth = FALSE;
    state->texture_prepare_material_color = material_color;
    state->texture_prepare_vertex_flags = policy->vertex_flags;
    if (state->texture_prepare_valid == 0u)
    {
        u32 texture_name = 0u;

        use_texture = FALSE;
        if (policy->textured != 0u)
        {
            u32 render_tile_index =
                ((stats->texture_state_flags &
                  NDS_RENDERER_TEXTURE_STATE_SEEN) != 0u) ?
                    (stats->texture_tile & 0x7u) :
                    NDS_RENDERER_RENDER_TILE;
            u32 implicit_texture_on = FALSE;
            u32 required_texture_mask =
                NDS_RENDERER_TEXTURE_SETTIMG |
                NDS_RENDERER_TEXTURE_SETTILE |
                NDS_RENDERER_TEXTURE_SETTILESIZE;

            render_tile = &stats->texture_tiles[render_tile_index];
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_ON) == 0u)
            {
                implicit_texture_on =
                    (((stats->texture_mask & required_texture_mask) ==
                      required_texture_mask) &&
                     ((stats->texture_mask &
                       (NDS_RENDERER_TEXTURE_LOADBLOCK |
                        NDS_RENDERER_TEXTURE_LOADTILE)) != 0u) &&
                     (stats->texture_image != 0u) &&
                     (stats->texture_load_texels != 0u) &&
                     (render_tile->set_seen != 0u) &&
                     (render_tile->size_seen != 0u) &&
                     (render_tile->line != 0u) &&
                     (render_tile->width != 0u) &&
                     (render_tile->height != 0u)) ? TRUE : FALSE;
            }
            use_texture = (hierarchy_run != NULL) ?
                ndsRendererHardwareResolveResidentTexture(
                    stats, config, state, &resolved_texture) :
                ndsRendererHardwareBindTexture(stats, config, state);
            if (use_texture == FALSE)
            {
                return ndsRendererNativeDirectReject(stats);
            }
            texture_name = (hierarchy_run != NULL) ?
                resolved_texture.name : sNdsRendererHardwareBoundTextureName;
            texture_scale_s = stats->texture_scale_s;
            texture_scale_t = stats->texture_scale_t;
            if (implicit_texture_on != FALSE)
            {
                if ((stats->texture_state_flags &
                     NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
                {
                    texture_scale_s =
                        NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
                }
                if ((stats->texture_state_flags &
                     NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
                {
                    texture_scale_t =
                        NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
                }
            }
            texture_origin_s = render_tile->uls;
            texture_origin_t = render_tile->ult;
            texture_offset =
                ((stats->othermode_h & NDS_RENDERER_TEXTFILT_MASK) !=
                 NDS_RENDERER_TF_POINT) ?
                    NDS_RENDERER_TEXCOORD_FILTER_OFFSET : 0;
        }
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled = use_texture;
        state->texture_prepare_name = texture_name;
        state->texture_prepare_alpha_constant = TRUE;
        state->texture_prepare_poly_alpha = 31u;
        state->texture_prepare_poly_fmt =
            expected_poly_cull | POLY_ALPHA(31u) |
            POLY_ID(stats->texture_combine_count &
                    NDS_RENDERER_POLY_ID_MASK);
        state->texture_prepare_scale_s = texture_scale_s;
        state->texture_prepare_scale_t = texture_scale_t;
        state->texture_prepare_origin_s = texture_origin_s;
        state->texture_prepare_origin_t = texture_origin_t;
        state->texture_prepare_offset = texture_offset;
        if (packet_mode == 0u)
        {
            ndsRendererProfileRecordTexturePrepare();
        }
    }
    else
    {
        use_texture = state->texture_prepare_enabled;
        if ((use_texture != FALSE) != (policy->textured != 0u))
        {
            return ndsRendererNativeDirectReject(stats);
        }
        if (packet_mode == 0u)
        {
            ndsRendererProfileRecordTexturePrepareReuse();
        }
    }
    if ((hierarchy_run != NULL) && (policy->textured != 0u) &&
        (resolved_texture.entry == NULL))
    {
        if (ndsRendererHardwareResolveResidentTexture(
                stats, config, state, &resolved_texture) == FALSE)
        {
            return ndsRendererNativeDirectReject(stats);
        }
        state->texture_prepare_name = resolved_texture.name;
    }
#if NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY
    if (use_texture != FALSE)
    {
        state->texture_prepare_vertex_flags &=
            ~(NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL |
              NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX);
    }
#endif

    unique_first = sNdsNativeFighterRunFirstUnique[run_index];
    unique_count = sNdsNativeFighterRunUniqueCount[run_index];
    /* Hierarchy preflight records immutable UV policy only; commit evaluates
     * the live dense UVs once.  Mode-8 keeps its original immediate path. */
    if ((policy->textured != 0u) && (hierarchy_run == NULL))
    {
        for (unique_offset = 0u;
             unique_offset < unique_count;
             unique_offset++)
        {
            u32 dense_id = sNdsNativeFighterRunUniqueDense[
                unique_first + unique_offset];
            const NDSNativeDenseVertex *dense =
                &sNdsNativeFighterDenseVertices[dense_id];
            NDSNativePreparedDenseVertex *prepared =
                &sNdsNativeFighterPreparedDense[dense_id];
            s32 scaled_s =
                ((s32)dense->s *
                 (s32)state->texture_prepare_scale_s) >> 17;
            s32 scaled_t =
                ((s32)dense->t *
                 (s32)state->texture_prepare_scale_t) >> 17;

            prepared->s = (s16)(
                scaled_s -
                ((s32)state->texture_prepare_origin_s << 2) +
                state->texture_prepare_offset);
            prepared->t = (s16)(
                scaled_t -
                ((s32)state->texture_prepare_origin_t << 2) +
                state->texture_prepare_offset);
        }
    }

    if (hierarchy_run != NULL)
    {
        hierarchy_run->texture_entry = resolved_texture.entry;
        hierarchy_run->texture_name = state->texture_prepare_name;
        hierarchy_run->texture_params = resolved_texture.params;
        hierarchy_run->texture_format = resolved_texture.format;
        hierarchy_run->texture_width = resolved_texture.width;
        hierarchy_run->texture_height = resolved_texture.height;
        hierarchy_run->poly_fmt = state->texture_prepare_poly_fmt;
        hierarchy_run->scale_s = state->texture_prepare_scale_s;
        hierarchy_run->scale_t = state->texture_prepare_scale_t;
        hierarchy_run->origin_s = state->texture_prepare_origin_s;
        hierarchy_run->origin_t = state->texture_prepare_origin_t;
        hierarchy_run->texture_offset = state->texture_prepare_offset;
        hierarchy_run->vertex_flags = state->texture_prepare_vertex_flags;
        hierarchy_run->textured = state->texture_prepare_enabled;
    }
    else if (packet_mode == 0u)
    {
        ndsRendererNativeBeginDirectBatch(
            stats, policy->textured, state->texture_prepare_name,
            state->texture_prepare_poly_fmt, state->matrix_generation);
    }
    return TRUE;
}


static void NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativeEmitProductionRawTexturedRun(
    u32 run_index,
    u32 corner_count)
{
    const u16 *corner =
        &sNdsNativeFighterPackedCorners[
            sNdsNativeFighterRunFirstCorner[run_index]];
    u32 remaining = corner_count;

    while (remaining-- != 0u)
    {
        u32 dense_id = *corner++;
        const NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterPreparedDense[dense_id];

        ndsRendererHardwareWriteColorWord(prepared->packed_color);
        ndsRendererHardwareWriteTexCoordWord(
            (u32)(u16)prepared->s |
            ((u32)(u16)prepared->t << 16));
        ndsRendererHardwareWriteVertex16Words(
            prepared->gx_xy, prepared->gx_z);
    }
}

static void NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativeEmitProductionRawUntexturedRun(
    u32 run_index,
    u32 corner_count)
{
    const u16 *corner =
        &sNdsNativeFighterPackedCorners[
            sNdsNativeFighterRunFirstCorner[run_index]];
    u32 remaining = corner_count;

    while (remaining-- != 0u)
    {
        u32 dense_id = *corner++;
        const NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterPreparedDense[dense_id];

        ndsRendererHardwareWriteColorWord(prepared->packed_color);
        ndsRendererHardwareWriteVertex16Words(
            prepared->gx_xy, prepared->gx_z);
    }
}

static void NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativeEmitProductionCrossRun(
    u32 run_index,
    u32 corner_count,
    u32 textured,
    u32 current_palette_slot,
    const u8 *binding_palette_slots)
{
    const u16 *corner =
        &sNdsNativeFighterPackedCorners[
            sNdsNativeFighterRunFirstCorner[run_index]];
    u32 active_palette_slot = current_palette_slot;
    u32 remaining = corner_count;

    while (remaining-- != 0u)
    {
        u32 packed = *corner++;
        u32 dense_id = packed & NDS_NATIVE_DENSE_ID_MASK;
        const NDSNativeDenseVertex *dense =
            &sNdsNativeFighterDenseVertices[dense_id];
        const NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterPreparedDense[dense_id];
        u32 palette_slot;

        if (binding_palette_slots != NULL)
        {
            palette_slot = binding_palette_slots[dense->matrix_binding];
        }
        else
        {
            palette_slot =
                packed >> NDS_NATIVE_PACKED_CORNER_MATRIX_SHIFT;
        }
        if (palette_slot == NDS_NATIVE_GX_MATRIX_CURRENT)
        {
            palette_slot = current_palette_slot;
        }
        if (palette_slot != active_palette_slot)
        {
            glRestoreMatrix((int)palette_slot);
            active_palette_slot = palette_slot;
        }
        ndsRendererHardwareWriteColorWord(prepared->packed_color);
        if (textured != 0u)
        {
            ndsRendererHardwareWriteTexCoordWord(
                (u32)(u16)prepared->s |
                ((u32)(u16)prepared->t << 16));
        }
        ndsRendererHardwareWriteVertex16Words(
            prepared->gx_xy, prepared->gx_z);
    }
    if (active_palette_slot != current_palette_slot)
    {
        glRestoreMatrix((int)current_palette_slot);
    }
}

static inline void ndsRendererNativeAccountGXCrossTriangles(
    NDSRendererStats *stats,
    u32 triangle_count,
    u32 reuse_count)
{
    sNdsRendererHardwareSubmitClassCounts[
        NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX] += triangle_count;
    sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices +=
        triangle_count * 3u;
    if ((sNdsRendererRuntimeFrameSummary.hardware_triangles > 2048u) ||
        (sNdsRendererRuntimeFrameSummary.hardware_vertices > 6144u))
    {
        sNdsRendererRuntimeFrameSummary.hardware_over_limit = 1u;
    }
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    stats->hardware_zbuffer_triangle_count += triangle_count;
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount += triangle_count;
#endif
    sNdsRendererHardwareSubmitted = TRUE;
}

static s32 ndsRendererNativeSubmitProductionRun(
    const NDSNativeRun *run,
    u32 epoch_policy,
    u32 current_palette_slot,
    const u8 *binding_palette_slots,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 *raw_triangle_count,
    u32 *raw_reuse_count,
    u32 *cross_triangle_count,
    u32 *cross_reuse_count)
{
    u32 run_index;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    volatile NDSRendererOwnerProfile *m2_owner =
        ndsRendererProfileM2Owner();
    u32 m2_phase_start = 0u;
#endif

    if ((run == NULL) || (run->triangle_count == 0u))
    {
        return ndsRendererNativeDirectReject(stats);
    }
    run_index = (u32)(run - sNdsNativeFighterRuns);
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP
    (void)epoch_policy;
    (void)current_palette_slot;
    (void)binding_palette_slots;
    (void)config;
    (void)state;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_phase_start = cpuGetTiming();
#endif
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_phase_start = cpuGetTiming();
#endif
    if (ndsRendererNativePrepareProductionRun(
            run_index, epoch_policy,
            FALSE,
            config, stats, state, NULL) == FALSE)
    {
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        if (m2_owner != NULL)
        {
            m2_owner->m2_run_prepare_ticks +=
                cpuGetTiming() - m2_phase_start;
            m2_owner->m2_run_prepare_count++;
        }
#endif
        return FALSE;
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    if (m2_owner != NULL)
    {
        m2_owner->m2_run_prepare_ticks +=
            cpuGetTiming() - m2_phase_start;
        m2_owner->m2_run_prepare_count++;
    }
#endif
    if ((run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX) &&
        (current_palette_slot > NDS_NATIVE_GX_MATRIX_SLOT_MAX))
    {
        return ndsRendererNativeDirectReject(stats);
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_phase_start = cpuGetTiming();
#endif
    if (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
    {
        ndsRendererNativeEmitProductionCrossRun(
            run_index, (u32)run->triangle_count * 3u,
            state->texture_prepare_enabled,
            current_palette_slot, binding_palette_slots);
    }
    else
    {
        if (state->texture_prepare_enabled != 0u)
        {
            ndsRendererNativeEmitProductionRawTexturedRun(
                run_index, (u32)run->triangle_count * 3u);
        }
        else
        {
            ndsRendererNativeEmitProductionRawUntexturedRun(
                run_index, (u32)run->triangle_count * 3u);
        }
    }
#else
    (void)epoch_policy;
    (void)current_palette_slot;
    (void)binding_palette_slots;
    (void)config;
    (void)state;
    return ndsRendererNativeDirectReject(stats);
#endif
    stats->triangle_count += run->triangle_count;
    if (run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT)
    {
        *raw_triangle_count += run->triangle_count;
        *raw_reuse_count += run->triangle_count - 1u;
    }
    else if (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
    {
        *cross_triangle_count += run->triangle_count;
        *cross_reuse_count += run->triangle_count - 1u;
    }
    else
    {
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        if (m2_owner != NULL)
        {
            m2_owner->m2_corner_emit_account_ticks +=
                cpuGetTiming() - m2_phase_start;
            m2_owner->m2_corner_emit_run_count++;
        }
#endif
        return ndsRendererNativeDirectReject(stats);
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    if (m2_owner != NULL)
    {
        m2_owner->m2_corner_emit_account_ticks +=
            cpuGetTiming() - m2_phase_start;
        m2_owner->m2_corner_emit_run_count++;
    }
#endif
    return TRUE;
}

static void NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativeEmitDenseRawRun(
    u32 run_index,
    u32 corner_count,
    const NDSRendererTraversalState *state,
    u32 textured)
{
    const u16 *corner =
        &sNdsNativeFighterDenseCorners[
            sNdsNativeFighterRunFirstCorner[run_index]];
    u32 remaining = corner_count;

    if (textured != 0u)
    {
        while (remaining-- != 0u)
        {
            u32 dense_id = *corner++;
            const NDSNativeDenseVertex *vertex =
                &sNdsNativeFighterDenseVertices[dense_id];
            const NDSNativePreparedDenseVertex *prepared =
                &sNdsNativeFighterPreparedDense[dense_id];
            u32 slot = vertex->cache_slot;

            ndsRendererHardwareWriteColorWord(
                state->prepared_vertex_colors[slot]);
            ndsRendererHardwareWriteTexCoordWord(
                (u32)(u16)state->prepared_texcoord_s[slot] |
                ((u32)(u16)state->prepared_texcoord_t[slot] << 16));
            ndsRendererHardwareWriteVertex16Words(
                prepared->gx_xy, prepared->gx_z);
        }
    }
    else
    {
        while (remaining-- != 0u)
        {
            u32 dense_id = *corner++;
            const NDSNativeDenseVertex *vertex =
                &sNdsNativeFighterDenseVertices[dense_id];
            const NDSNativePreparedDenseVertex *prepared =
                &sNdsNativeFighterPreparedDense[dense_id];
            u32 slot = vertex->cache_slot;

            ndsRendererHardwareWriteColorWord(
                state->prepared_vertex_colors[slot]);
            ndsRendererHardwareWriteVertex16Words(
                prepared->gx_xy, prepared->gx_z);
        }
    }
}

static inline void ndsRendererNativeAccountProjectedCrossTriangle(
    NDSRendererStats *stats,
    u32 triangle_count)
{
    u32 reuse_count = (triangle_count != 0u) ?
        triangle_count - 1u : 0u;

    sNdsRendererHardwareSubmitClassCounts[
        NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX] += triangle_count;
    sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count += triangle_count;
    sNdsRendererRuntimeFrameSummary.projected_submit_fallback_count +=
        triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count +=
        reuse_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices +=
        triangle_count * 3u;
    if ((sNdsRendererRuntimeFrameSummary.hardware_triangles > 2048u) ||
        (sNdsRendererRuntimeFrameSummary.hardware_vertices > 6144u))
    {
        sNdsRendererRuntimeFrameSummary.hardware_over_limit = 1u;
    }
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    stats->hardware_projected_depth_triangle_count += triangle_count;
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount += triangle_count;
#endif
    sNdsRendererHardwareSubmitted = TRUE;
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
static s32 ndsRendererNativeSubmitRunDirect(
    const NDSNativeRun *run,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP
    if ((run == NULL) || (config == NULL) || (stats == NULL) ||
        (state == NULL) || (run->triangle_count == 0u))
    {
        return FALSE;
    }
    /* This benchmark measures everything surrounding triangle transport.
     * Consume the generated run as one unit instead of paying the generic
     * per-triangle fallback loop that production never executes. */
    stats->triangle_count += run->triangle_count;
    if (run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT)
    {
        ndsRendererFastAccountRawTriangles(
            stats, run->triangle_count,
            (run->triangle_count != 0u) ? run->triangle_count - 1u : 0u);
    }
    else
    {
        ndsRendererNativeAccountProjectedCrossTriangle(
            stats, run->triangle_count);
    }
    return TRUE;
#elif NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
    u32 emitted_triangles = 0u;
    u32 run_index;
    u32 i;

    if ((run == NULL) || (config == NULL) || (stats == NULL) ||
        (state == NULL) || (run->triangle_count == 0u))
    {
        return FALSE;
    }
    run_index = (u32)(run - sNdsNativeFighterRuns);
    if (run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT)
    {
        const u16 *triangles =
            &sNdsNativeFighterTriangles[run->first_triangle];
        u32 first_indices[3];

        (void)ndsRendererNativeDecodeTriangle(
            triangles[0], first_indices);
        if (ndsRendererNativePrepareDirectRun(
                run, first_indices[0], FALSE,
                config, stats, state) == FALSE)
        {
            return FALSE;
        }
        stats->triangle_count += run->triangle_count;
        ndsRendererNativeEmitDenseRawRun(
            run_index,
            (u32)run->triangle_count * 3u,
            state, state->texture_prepare_enabled);
        ndsRendererFastAccountRawTriangles(
            stats, run->triangle_count, run->triangle_count - 1u);
        return TRUE;
    }
    if (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
    {
        const u16 *triangles =
            &sNdsNativeFighterTriangles[run->first_triangle];
        u32 first_indices[3];

        (void)ndsRendererNativeDecodeTriangle(triangles[0], first_indices);
        if (ndsRendererNativePrepareDirectRun(
                run, first_indices[0], TRUE,
                config, stats, state) == FALSE)
        {
            return FALSE;
        }
        stats->triangle_count += run->triangle_count;
        for (i = 0u; i < run->triangle_count; i++)
        {
            u32 indices[3];

            (void)ndsRendererNativeDecodeTriangle(
                triangles[i], indices);
            if (ndsRendererHardwareTriangleInsideNearPlane(
                    &state->vertices[indices[0]],
                    &state->vertices[indices[1]],
                    &state->vertices[indices[2]]) == FALSE)
            {
                ndsRendererProfileRecordNearPlaneTriangleReject();
                ndsRendererProfileRecordSubmitClass(
                    NDS_RENDERER_HW_SUBMIT_REJECT);
                continue;
            }
            ndsRendererHardwareSubmitVertex(
                stats, state, indices[0], 0);
            ndsRendererHardwareSubmitVertex(
                stats, state, indices[1], 0);
            ndsRendererHardwareSubmitVertex(
                stats, state, indices[2], 0);
            emitted_triangles++;
        }
        ndsRendererHardwareEnterProjectedForeground();
        if (emitted_triangles != 0u)
        {
            ndsRendererNativeAccountProjectedCrossTriangle(
                stats, emitted_triangles);
        }
        return TRUE;
    }
#else
    (void)run;
    (void)config;
    (void)stats;
    (void)state;
#endif
    return FALSE;
}
#endif

static void ndsRendererNativeSubmitRun(
    const NDSNativeRun *run,
    const u8 *root_base,
    NDSRendererCommandCallback callback,
    void *callback_user,
    u32 *last_callback_command,
    u32 *source_command_index,
    u32 *tri2_half,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const u16 *triangle;
    s32 native_raw_ready = FALSE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    s32 native_snapshot_ready = FALSE;
    u32 native_raw_submitted = 0u;
    u32 native_cross_submitted = 0u;
#endif
    u32 i;

    stats->triangle_count += run->triangle_count;
    triangle = &sNdsNativeFighterTriangles[run->first_triangle];
    if ((run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT) &&
        (run->triangle_count != 0u) &&
        (NDS_RENDERER_BENCHMARK_MODE !=
         NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP))
    {
        u32 first_indices[3];

        (void)ndsRendererNativeDecodeTriangle(
            triangle[0], first_indices);
        native_raw_ready = ndsRendererNativePrepareDirectRun(
            run, first_indices[0], FALSE,
            config, stats, state);
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    else if ((run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX) &&
             (run->triangle_count != 0u) &&
             (NDS_RENDERER_BENCHMARK_MODE !=
              NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP))
    {
        u32 first_indices[3];

        (void)ndsRendererNativeDecodeTriangle(
            triangle[0], first_indices);
        native_snapshot_ready = ndsRendererNativePrepareDirectRun(
            run, first_indices[0], TRUE,
            config, stats, state);
    }
#endif
    for (i = 0u; i < run->triangle_count; i++)
    {
        u16 encoded = triangle[i];
        u32 indices[3];
        u32 packed = ndsRendererNativeDecodeTriangle(encoded, indices);
        u32 command_index = *source_command_index;
        u32 command_half = *tri2_half;

        if (*last_callback_command != command_index)
        {
            if (ndsRendererNativeVisitSourceCommand(
                    root_base, command_index,
                    callback, callback_user, stats, state) == FALSE)
            {
                return;
            }
            *last_callback_command = command_index;
        }
        if (((run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT) &&
             (native_raw_ready == FALSE)) ||
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ((run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX) &&
             (native_snapshot_ready == FALSE)) ||
#else
            (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX) ||
#endif
            (NDS_RENDERER_BENCHMARK_MODE ==
             NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP))
        {
            if ((run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT) &&
                (i != 0u) &&
                (ndsRendererFastRawStateEligible(state) == FALSE))
            {
                sNdsRendererFastFallbackCount[0]++;
            }
            ndsRendererNativeSubmitGenericTriangle(
                packed, command_index, command_half,
                config, stats, state);
            if ((run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT) &&
                (i == 0u) &&
                (ndsRendererFastRawStateEligible(state) != FALSE))
            {
                ndsRendererFastPrepareRawSlots(
                    stats, state, run->required_mask,
                    state->texture_prepare_enabled);
                native_raw_ready = TRUE;
            }
        }
        else
        {
#if NDS_RENDERER_PROFILE_LEVEL < 2
            if (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
            {
                if (ndsRendererHardwareTriangleInsideNearPlane(
                        &state->vertices[indices[0]],
                        &state->vertices[indices[1]],
                        &state->vertices[indices[2]]) == FALSE)
                {
                    ndsRendererProfileRecordNearPlaneTriangleReject();
                    ndsRendererProfileRecordSubmitClass(
                        NDS_RENDERER_HW_SUBMIT_REJECT);
                    continue;
                }
                ndsRendererHardwareSubmitVertex(
                    stats, state, indices[0], 0);
                ndsRendererHardwareSubmitVertex(
                    stats, state, indices[1], 0);
                ndsRendererHardwareSubmitVertex(
                    stats, state, indices[2], 0);
                ndsRendererHardwareEnterProjectedForeground();
                native_cross_submitted++;
            }
            else
#endif
            {
            if (sNdsRendererHardwareNoOracle == 0u)
            {
                ndsRendererRecordTransformedTriangle(
                    stats, state, packed);
            }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            state->semantic_command_index = command_index;
            state->semantic_tri2_half = command_half;
#endif
            ndsRendererFastEmitRawCommand(
                state, indices, 1u, state->texture_prepare_enabled);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            ndsRendererFastCommitRawSemanticTriangle(
                stats, state, packed, indices);
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
            native_raw_submitted++;
#else
            ndsRendererFastAccountRawTriangles(
                stats, 1u, (i == 0u) ? 0u : 1u);
#endif
            }
        }
        if ((encoded & 0x8000u) != 0u)
        {
            *tri2_half = 1u;
        }
        else
        {
            (*source_command_index)++;
            *tri2_half = 0u;
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (native_raw_submitted != 0u)
    {
        ndsRendererFastAccountRawTriangles(
            stats, native_raw_submitted,
            native_raw_submitted - 1u);
    }
    if (native_cross_submitted != 0u)
    {
        ndsRendererNativeAccountProjectedCrossTriangle(
            stats, native_cross_submitted);
    }
#endif
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static void ndsRendererNativeBindOwnerRootState(
    NDSRendererTraversalState *state,
    const NDSRendererConfig *config,
    NDSRendererStats *stats)
{
    state->modelview_stack_depth = 0u;
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_projected_xy_valid_mask = 0u;
    state->prepared_projected_source_z_valid_mask = 0u;
    state->prepared_light_direction_valid = 0u;
    state->texture_prepare_valid = 0u;
    state->projection_valid = 0u;
    state->modelview_valid = 0u;
    state->matrix_valid = 0u;
    state->matrix_word_valid = 0u;
    if ((stats != NULL) && (config->initial_geometry_mode != 0u))
    {
        stats->geometry_mode = config->initial_geometry_mode;
    }
    if (config->initial_projection != NULL)
    {
        state->projection = *config->initial_projection;
        state->projection_valid = TRUE;
    }
    if (config->initial_modelview != NULL)
    {
        state->modelview = *config->initial_modelview;
        state->modelview_valid = TRUE;
    }
    ndsRendererComposeMatrix(state);
    if ((stats != NULL) && (state->matrix_valid != 0u))
    {
        stats->hardware_matrix_seed_count++;
    }
}
#endif

static s32 ndsRendererExecuteNativeFighterRootHardware(
    u32 slot,
    u32 root_ordinal,
    const void *asset_base_ptr,
    u32 root_offset,
    const NDSRendererNativeMaterial *materials,
    u32 material_count,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
    const NDSNativeRoot *roots;
    const NDSNativeRoot *root;
    const u8 *asset_base = asset_base_ptr;
    const u8 *root_base;
    u32 root_count;
    u32 epoch_index;
    u32 native_triangle_count = 0u;
    u32 native_run_count = 0u;
    u32 last_callback_command = 0xffffffffu;
    NDSRendererTraversalState local_state;
    NDSRendererTraversalState *state_ptr = &local_state;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    s32 shared_owner_state = FALSE;
#endif
#define state (*state_ptr)

    if ((asset_base == NULL) || (config == NULL) || (stats == NULL) ||
        (vertex_cache == NULL) || (slot > 1u))
    {
        return FALSE;
    }
    if (slot == 0u)
    {
        roots = sNdsNativeMarioRoots;
        root_count = sizeof(sNdsNativeMarioRoots) /
            sizeof(sNdsNativeMarioRoots[0]);
    }
    else
    {
        roots = sNdsNativeFoxRoots;
        root_count = sizeof(sNdsNativeFoxRoots) /
            sizeof(sNdsNativeFoxRoots[0]);
    }
    if ((root_ordinal >= root_count) ||
        (roots[root_ordinal].root_offset != root_offset))
    {
        return FALSE;
    }
    root = &roots[root_ordinal];
    for (epoch_index = 0u; epoch_index < root->epoch_count; epoch_index++)
    {
        const NDSNativeEpoch *epoch =
            &sNdsNativeFighterEpochs[root->first_epoch + epoch_index];

        if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
            ((materials == NULL) ||
             (epoch->material_slot >= material_count)))
        {
            return FALSE;
        }
    }

#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (sNdsNativeFighterOwnerExecution.active != 0u)
    {
        if ((sNdsNativeFighterOwnerExecution.slot != slot) ||
            (sNdsNativeFighterOwnerExecution.stats != stats) ||
            (sNdsNativeFighterOwnerExecution.vertex_cache != vertex_cache))
        {
            ndsRendererAbortNativeFighterOwner();
            return FALSE;
        }
        state_ptr = &sNdsNativeFighterOwnerExecution.traversal;
        shared_owner_state = TRUE;
        ndsRendererNativeBindOwnerRootState(&state, config, stats);
    }
    else
#endif
    {
        ndsRendererInitTraversalState(
            &state, config, stats, NULL,
            vertex_cache->matrix_snapshots,
            vertex_cache->matrix_snapshot_count);
        state.vertices = vertex_cache->transformed_vertices;
        state.vertex_valid_mask = vertex_cache->transformed_valid_mask;
        state.input_vertices = vertex_cache->input_vertices;
        state.input_vertex_valid_mask = vertex_cache->input_valid_mask;
        state.raw_vertex_fit_mask = vertex_cache->raw_vertex_fit_mask;
        state.vertex_colors = vertex_cache->vertex_colors;
        state.vertex_color_valid_mask =
            vertex_cache->vertex_color_valid_mask;
        state.vertex_matrix_snapshot = vertex_cache->vertex_matrix_snapshot;
        state.vertex_clip_snapshot = vertex_cache->vertex_clip_snapshot;
    }
    root_base = asset_base + root_offset;
    if (stats->first_opcode == 0u)
    {
        stats->first_opcode = NDS_RENDERER_OP_RDPPIPESYNC;
    }
    stats->command_count += root->source_command_count;
    ndsRendererNativeApplyRootLightPreamble(root, stats);
    for (epoch_index = 0u; epoch_index < root->epoch_count; epoch_index++)
    {
        const NDSNativeEpoch *epoch =
            &sNdsNativeFighterEpochs[root->first_epoch + epoch_index];
        u32 run_index;
        u32 source_command_index =
            epoch->first_triangle_command_index;
        u32 tri2_half = 0u;

        ndsRendererNativeApplyStateSpan(
            epoch->before_state_first, epoch->before_state_count,
            epoch->before_sync_count,
            asset_base, stats, &state);
        if (epoch->material_slot != NDS_NATIVE_MATERIAL_NONE)
        {
            ndsRendererNativeApplyMaterial(
                &materials[epoch->material_slot], stats, &state);
        }
        ndsRendererNativeApplyStateSpan(
            epoch->after_state_first, epoch->after_state_count,
            epoch->after_sync_count,
            asset_base, stats, &state);
        ndsRendererNativeApplyVertexActions(
            epoch, asset_base, root_base,
            callback, callback_user, stats, &state);
        if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            break;
        }
        for (run_index = 0u; run_index < epoch->run_count; run_index++)
        {
            const NDSNativeRun *run =
                &sNdsNativeFighterRuns[epoch->first_run + run_index];

#if NDS_RENDERER_PROFILE_LEVEL < 2
            if ((callback != NULL) ||
                (sNdsRendererHardwareNoOracle == 0u) ||
                (ndsRendererNativeSubmitRunDirect(
                     run, config, stats, &state) == FALSE))
#endif
            {
                ndsRendererNativeSubmitRun(
                    run, root_base, callback, callback_user,
                    &last_callback_command,
                    &source_command_index, &tri2_half,
                    config, stats, &state);
            }
            native_triangle_count += run->triangle_count;
            native_run_count++;
            if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
            {
                break;
            }
        }
        if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            break;
        }
    }
    if (stats->blocker == NDS_RENDERER_BLOCKER_NONE)
    {
        ndsRendererNativeApplyStateSpan(
            root->tail_state_first, root->tail_state_count,
            root->tail_sync_count,
            asset_base, stats, &state);
        stats->end_command_count++;
    }
    ndsRendererHardwareEndBatch();
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (shared_owner_state == FALSE)
#endif
    {
        vertex_cache->transformed_valid_mask = state.vertex_valid_mask;
        vertex_cache->input_valid_mask = state.input_vertex_valid_mask;
        vertex_cache->raw_vertex_fit_mask = state.raw_vertex_fit_mask;
        vertex_cache->vertex_color_valid_mask =
            state.vertex_color_valid_mask;
        vertex_cache->matrix_snapshot_count = state.matrix_snapshot_count;
    }
    if (native_triangle_count != 0u)
    {
        sNdsRendererFastRunCount += native_run_count;
        sNdsRendererFastTriangleCount += native_triangle_count;
        if ((u32)sNdsRendererRuntimeOwner <
            (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
        {
            sNdsRendererFastOwnerTriangleCount[
                (u32)sNdsRendererRuntimeOwner] += native_triangle_count;
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if ((shared_owner_state != FALSE) &&
        (stats->blocker != NDS_RENDERER_BLOCKER_NONE))
    {
        ndsRendererAbortNativeFighterOwner();
        return FALSE;
    }
#endif
    return TRUE;
#undef state
}
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
typedef struct NDSNativeHierarchyTables
{
    const NDSNativeRoot *roots;
    const u16 *schedule;
    const u8 *binding_joints;
    const u8 *cross_slots;
    u32 root_count;
    u32 joint_count;
} NDSNativeHierarchyTables;

static s32 ndsRendererNativeGetHierarchyTables(
    u32 slot, NDSNativeHierarchyTables *tables)
{
    if ((slot > 1u) || (tables == NULL))
    {
        return FALSE;
    }
    if (slot == 0u)
    {
        tables->roots = sNdsNativeMarioRoots;
        tables->schedule = sNdsNativeMarioJointSchedule;
        tables->binding_joints = sNdsNativeMarioBindingJoints;
        tables->cross_slots = sNdsNativeMarioCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeMarioRoots) /
            sizeof(sNdsNativeMarioRoots[0]);
        tables->joint_count = sizeof(sNdsNativeMarioJointSchedule) /
            sizeof(sNdsNativeMarioJointSchedule[0]);
    }
    else
    {
        tables->roots = sNdsNativeFoxRoots;
        tables->schedule = sNdsNativeFoxJointSchedule;
        tables->binding_joints = sNdsNativeFoxBindingJoints;
        tables->cross_slots = sNdsNativeFoxCrossPaletteSlots;
        tables->root_count = sizeof(sNdsNativeFoxRoots) /
            sizeof(sNdsNativeFoxRoots[0]);
        tables->joint_count = sizeof(sNdsNativeFoxJointSchedule) /
            sizeof(sNdsNativeFoxJointSchedule[0]);
    }
    return TRUE;
}

static void ndsRendererNativeMatrix3From20p12(
    const NDSRendererMatrix20p12 *source,
    NDSNativeMatrix3x3 *matrix)
{
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            matrix->m[row][col] = source->m[row][col];
        }
    }
}

static void ndsRendererNativeMatrix3Mul20p12(
    const NDSNativeMatrix3x3 *lhs,
    const NDSNativeMatrix3x3 *rhs,
    NDSNativeMatrix3x3 *out)
{
    NDSNativeMatrix3x3 result;
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            s64 sum = (s64)lhs->m[row][0] * rhs->m[0][col] +
                (s64)lhs->m[row][1] * rhs->m[1][col] +
                (s64)lhs->m[row][2] * rhs->m[2][col];

            result.m[row][col] = ndsRendererClampS64ToS32(
                ndsRendererRoundShiftS64(
                    sum, NDS_RENDERER_DS_MTX_FRAC_BITS));
        }
    }
    *out = result;
}

static void ndsRendererNativeMatrix3To20p12(
    const NDSNativeMatrix3x3 *source,
    NDSRendererMatrix20p12 *matrix)
{
    u32 row;
    u32 col;

    ndsRendererMtxIdentity20p12(matrix);
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            matrix->m[row][col] = source->m[row][col];
        }
    }
}

static s32 ndsRendererNativeHierarchyMatrixIsAffine(
    const NDSRendererMatrix20p12 *matrix)
{
    return ((matrix != NULL) &&
            (matrix->m[0][3] == 0) &&
            (matrix->m[1][3] == 0) &&
            (matrix->m[2][3] == 0) &&
            (matrix->m[3][3] ==
             (1 << NDS_RENDERER_DS_MTX_FRAC_BITS))) ? TRUE : FALSE;
}

static s32 ndsRendererNativePreflightFighterHierarchy(
    u32 slot,
    const u8 *asset_base,
    const NDSRendererNativeFighterHierarchy *hierarchy,
    NDSRendererCommandCallback callback,
    NDSRendererStats *stats,
    NDSNativeHierarchyTables *tables)
{
    NDSNativeFighterOwnerExecution *execution =
        &sNdsNativeFighterOwnerExecution;
    NDSRendererTraversalState *state = &execution->traversal;
    NDSRendererStats *scratch = &execution->preflight_stats;
    NDSNativeMatrix3x3 camera;
    NDSNativeMatrix3x3 identity;
    u32 binding_seen = 0u;
    u32 joint_index;
    u32 root_index;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    volatile NDSRendererOwnerProfile *m2_owner =
        ndsRendererProfileM2Owner();
#endif

    if ((asset_base == NULL) || (hierarchy == NULL) || (callback != NULL) ||
        (stats == NULL) || (tables == NULL) ||
        (hierarchy->projection == NULL) ||
        (hierarchy->camera_modelview == NULL) ||
        (hierarchy->joint_locals == NULL) ||
        (hierarchy->joint_parents == NULL) ||
        (hierarchy->joint_bindings == NULL) ||
        (hierarchy->roots == NULL) || (hierarchy->config == NULL) ||
        (sNdsNativeFighterOwnerExecution.active != 0u) ||
        (stats->blocker != NDS_RENDERER_BLOCKER_NONE) ||
        (ndsRendererNativeGetHierarchyTables(slot, tables) == FALSE) ||
        (hierarchy->joint_count != tables->joint_count) ||
        (hierarchy->root_count != tables->root_count) ||
        (tables->joint_count > NDS_NATIVE_FIGHTER_HIERARCHY_JOINT_MAX) ||
        (tables->root_count > NDS_NATIVE_FIGHTER_HIERARCHY_BINDING_MAX) ||
        (ndsRendererNativeHierarchyMatrixIsAffine(
             hierarchy->camera_modelview) == FALSE))
    {
        return FALSE;
    }
    ndsRendererNativeMatrix3From20p12(
        hierarchy->camera_modelview, &camera);
    memset(&identity, 0, sizeof(identity));
    identity.m[0][0] = 1 << NDS_RENDERER_DS_MTX_FRAC_BITS;
    identity.m[1][1] = 1 << NDS_RENDERER_DS_MTX_FRAC_BITS;
    identity.m[2][2] = 1 << NDS_RENDERER_DS_MTX_FRAC_BITS;
    for (joint_index = 0u; joint_index < tables->joint_count; joint_index++)
    {
        u32 packed = tables->schedule[joint_index];
        u32 parent = packed & 31u;
        u32 binding = (packed >> 5) & 31u;
        NDSNativeMatrix3x3 local;
        const NDSNativeMatrix3x3 *parent_world;

        if ((hierarchy->joint_parents[joint_index] != parent) ||
            (hierarchy->joint_bindings[joint_index] != binding) ||
            (ndsRendererNativeHierarchyMatrixIsAffine(
                 &hierarchy->joint_locals[joint_index]) == FALSE) ||
            ((parent != 31u) && (parent >= joint_index)) ||
            ((binding != 31u) && (binding >= tables->root_count)))
        {
            return FALSE;
        }
        ndsRendererNativeMatrix3From20p12(
            &hierarchy->joint_locals[joint_index], &local);
        /* Match BattleShip's fixed-point association exactly: build each
         * source world from identity, then apply the camera once at a bound
         * root.  Seeding camera into the recurrence is algebraically equal in
         * real arithmetic but rounds differently in 20.12. */
        parent_world = (parent == 31u) ? &identity :
            &execution->hierarchy_world[parent];
        ndsRendererNativeMatrix3Mul20p12(
            &local, parent_world,
            &execution->hierarchy_world[joint_index]);
        if (binding != 31u)
        {
            if (((binding_seen >> binding) & 1u) != 0u)
            {
                return FALSE;
            }
            binding_seen |= 1u << binding;
        }
    }
    if (binding_seen != ((1u << tables->root_count) - 1u))
    {
        return FALSE;
    }
    for (root_index = 0u; root_index < tables->root_count; root_index++)
    {
        u32 binding_joint = tables->binding_joints[root_index];

        if ((binding_joint >= tables->joint_count) ||
            (hierarchy->joint_bindings[binding_joint] != root_index))
        {
            return FALSE;
        }
        /* All source-world descendants are complete, so the binding joints
         * can now hold their exact once-composed lighting matrices in place. */
        ndsRendererNativeMatrix3Mul20p12(
            &execution->hierarchy_world[binding_joint], &camera,
            &execution->hierarchy_world[binding_joint]);
    }

    *scratch = *stats;
    ndsRendererInitTraversalState(
        state, hierarchy->config, scratch, NULL, NULL, 0u);
    for (root_index = 0u; root_index < tables->root_count; root_index++)
    {
        const NDSNativeRoot *root = &tables->roots[root_index];
        const NDSRendererNativeFighterRoot *input =
            &hierarchy->roots[root_index];
        NDSRendererMatrix20p12 light_modelview;
        u32 epoch_offset;

        if ((input->root_offset != root->root_offset) ||
            (input->composed_matrix != NULL) ||
            (input->modelview_matrix != NULL) ||
            (input->config != hierarchy->config) ||
            ((input->preamble.flags &
              NDS_RENDERER_NATIVE_PREAMBLE_VALID) == 0u))
        {
            return FALSE;
        }
        ndsRendererNativeMatrix3To20p12(
            &execution->hierarchy_world[
                tables->binding_joints[root_index]],
            &light_modelview);
        ndsRendererNativeApplyProductionPreamble(&input->preamble, scratch);
        ndsRendererNativeApplyRootLightPreamble(root, scratch);
        state->vertex_valid_mask = 0u;
        state->input_vertex_valid_mask = 0u;
        state->vertex_color_valid_mask = 0u;
        state->current_transform_vertex_mask = 0u;
        state->prepared_vertex_color_valid_mask = 0u;
        state->prepared_texcoord_valid_mask = 0u;
        state->prepared_light_direction_valid = 0u;
        state->texture_prepare_valid = 0u;
        state->modelview = light_modelview;
        state->modelview_valid = TRUE;
        state->matrix = light_modelview;
        state->matrix_valid = TRUE;
        state->matrix_generation = root_index + 1u;

        for (epoch_offset = 0u;
             epoch_offset < root->epoch_count;
             epoch_offset++)
        {
            u32 epoch_index = root->first_epoch + epoch_offset;
            const NDSNativeEpoch *epoch =
                &sNdsNativeFighterEpochs[epoch_index];
            NDSNativeHierarchyPreparedEpoch *prepared_epoch =
                &execution->hierarchy_epochs[epoch_index];
            u32 run_offset;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            u32 m2_lighting_start;
#endif

            if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
                ((input->materials == NULL) ||
                 (epoch->material_slot >= input->material_count)))
            {
                return FALSE;
            }
            ndsRendererNativeApplyStateSpanPreflight(
                epoch->before_state_first, epoch->before_state_count,
                epoch->before_sync_count, asset_base, scratch, state);
            if (epoch->material_slot != NDS_NATIVE_MATERIAL_NONE)
            {
                ndsRendererNativeApplyMaterialPreflight(
                    &input->materials[epoch->material_slot], scratch, state);
            }
            ndsRendererNativeApplyStateSpanPreflight(
                epoch->after_state_first, epoch->after_state_count,
                epoch->after_sync_count, asset_base, scratch, state);
            if (scratch->blocker != NDS_RENDERER_BLOCKER_NONE)
            {
                return FALSE;
            }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            m2_lighting_start = cpuGetTiming();
#endif
            prepared_epoch->light_direction_valid = FALSE;
            if ((epoch->action_count != 0u) &&
                ((scratch->geometry_mode &
                  NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
                ((scratch->light_dir_mask &
                  NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
            {
                if (state->prepared_light_direction_valid == 0u)
                {
                    ndsRendererHardwarePrepareLitDirection(
                        scratch, &state->modelview,
                        &state->prepared_light_direction);
                    state->prepared_light_direction_valid = TRUE;
                }
                prepared_epoch->light_direction =
                    state->prepared_light_direction;
                prepared_epoch->light_direction_valid = TRUE;
                if ((scratch->light_color_mask &
                     (NDS_RENDERER_LIGHT_COLOR_1_MASK |
                      NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
                    (NDS_RENDERER_LIGHT_COLOR_1_MASK |
                     NDS_RENDERER_LIGHT_COLOR_2_MASK))
                {
                    /* Populate the bounded CPU shade cache before GX.  Commit
                     * may look the pair up again after cache rotation, but the
                     * lookup/build has no failure branch. */
                    (void)ndsRendererHardwareGetLightShadeLut(
                        scratch->light_color_1,
                        scratch->light_color_2);
                }
            }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            if (m2_owner != NULL)
            {
                m2_owner->m2_lighting_shading_ticks +=
                    cpuGetTiming() - m2_lighting_start;
            }
#endif
            memset(&execution->hierarchy_runs[epoch_index], 0,
                   sizeof(execution->hierarchy_runs[epoch_index]));
            for (run_offset = 0u;
                 run_offset < epoch->run_count;
                 run_offset++)
            {
                u32 run_index = epoch->first_run + run_offset;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                u32 m2_run_start = cpuGetTiming();
                s32 run_ready;
#endif

#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                run_ready = ndsRendererNativePrepareProductionRun(
                    run_index,
                    sNdsNativeFighterEpochDirectPolicy[epoch_index],
                    TRUE, hierarchy->config, scratch, state,
                    &execution->hierarchy_runs[epoch_index]);
                if (m2_owner != NULL)
                {
                    m2_owner->m2_run_prepare_ticks +=
                        cpuGetTiming() - m2_run_start;
                }
                if (run_ready == FALSE)
#else
                if (ndsRendererNativePrepareProductionRun(
                        run_index,
                        sNdsNativeFighterEpochDirectPolicy[epoch_index],
                        TRUE, hierarchy->config, scratch, state,
                        &execution->hierarchy_runs[epoch_index]) == FALSE)
#endif
                {
                    return FALSE;
                }
            }
        }
        ndsRendererNativeApplyStateSpanPreflight(
            root->tail_state_first, root->tail_state_count,
            root->tail_sync_count, asset_base, scratch, state);
        if (scratch->blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void ndsRendererNativeBuildHierarchyHardwareAffine(
    const NDSRendererMatrix20p12 *source,
    m4x4 *hardware)
{
    NDSRendererMatrix20p12 scaled = *source;
    u32 col;

    for (col = 0u; col < 3u; col++)
    {
        scaled.m[3][col] = ndsRendererRoundShiftS32Signed(
            scaled.m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    ndsRendererCopyMtx20p12ToM4x4(&scaled, hardware);
}

static void ndsRendererNativePrepareHierarchyTexcoords(
    u32 run_index,
    const NDSNativeHierarchyPreparedRun *prepared_run)
{
    u32 unique_first;
    u32 unique_count;
    u32 unique_offset;

    if ((prepared_run == NULL) || (prepared_run->textured == 0u))
    {
        return;
    }
    unique_first = sNdsNativeFighterRunFirstUnique[run_index];
    unique_count = sNdsNativeFighterRunUniqueCount[run_index];
    for (unique_offset = 0u; unique_offset < unique_count; unique_offset++)
    {
        u32 dense_id = sNdsNativeFighterRunUniqueDense[
            unique_first + unique_offset];
        const NDSNativeDenseVertex *dense =
            &sNdsNativeFighterDenseVertices[dense_id];
        NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterPreparedDense[dense_id];
        s32 scaled_s = ((s32)dense->s *
            (s32)prepared_run->scale_s) >> 17;
        s32 scaled_t = ((s32)dense->t *
            (s32)prepared_run->scale_t) >> 17;

        prepared->s = (s16)(scaled_s -
            ((s32)prepared_run->origin_s << 2) +
            prepared_run->texture_offset);
        prepared->t = (s16)(scaled_t -
            ((s32)prepared_run->origin_t << 2) +
            prepared_run->texture_offset);
    }
}

static inline void ndsRendererNativeBeginHierarchyBatch(
    NDSRendererStats *stats,
    const NDSNativeHierarchyPreparedRun *prepared_run,
    u32 matrix_generation)
{
    u32 texture_name = (prepared_run->textured != 0u) ?
        prepared_run->texture_name : 0u;

    if ((sNdsRendererHardwareTriangleBatchOpen != 0u) &&
        (sNdsRendererHardwareTriangleBatchTextured ==
         prepared_run->textured) &&
        (sNdsRendererHardwareTriangleBatchTextureName == texture_name) &&
        (sNdsRendererHardwareTriangleBatchPolyFmt ==
         prepared_run->poly_fmt) &&
        (sNdsRendererHardwareTriangleBatchMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY) &&
        (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
         matrix_generation))
    {
        ndsRendererProfileRecordBatchReuse();
        return;
    }

    ndsRendererHardwareEndBatch();
    glEnable(GL_TEXTURE_2D);
    if (prepared_run->textured != 0u)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            prepared_run->texture_entry;

        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
        entry->params = prepared_run->texture_params;
        ndsRendererHardwareBindTextureName(stats, prepared_run->texture_name);
        ndsRendererHardwareApplyTextureParams(prepared_run->texture_params);
        sNdsRendererHardwareActiveTextureEntry = entry;
        if (entry->pinned != 0u)
        {
            ndsRendererHardwareRecordBattleStaticTextureHit(entry);
        }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = prepared_run->texture_format;
        stats->hardware_texture_width = prepared_run->texture_width;
        stats->hardware_texture_height = prepared_run->texture_height;
    }
    else
    {
        ndsRendererHardwareBindNoTexture(NULL);
    }
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    ndsRendererHardwareSetPolyFmt(prepared_run->poly_fmt);
    glBegin(GL_TRIANGLE);
    ndsRendererProfileRecordBatchBegin();

    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = prepared_run->textured;
    sNdsRendererHardwareTriangleBatchTextureName = texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = prepared_run->poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = 0u;
    sNdsRendererHardwareTriangleBatchFogKey = 0u;
    sNdsRendererHardwareTriangleBatchMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY;
    sNdsRendererHardwareTriangleBatchMatrixGeneration = matrix_generation;
}

static void ndsRendererNativeCommitHierarchyRoot(
    const u8 *asset_base,
    const NDSNativeRoot *root,
    const NDSRendererNativeFighterRoot *input,
    u32 binding_joint,
    u32 current_palette_slot,
    u32 matrix_generation,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 *run_count,
    u32 *triangle_count)
{
    NDSNativeFighterOwnerExecution *execution =
        &sNdsNativeFighterOwnerExecution;
    NDSRendererMatrix20p12 light_modelview;
    u32 epoch_offset;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    volatile NDSRendererOwnerProfile *m2_owner =
        ndsRendererProfileM2Owner();
#endif

    ndsRendererNativeMatrix3To20p12(
        &execution->hierarchy_world[binding_joint], &light_modelview);
    ndsRendererNativeApplyProductionPreamble(&input->preamble, stats);
    state->vertex_valid_mask = 0u;
    state->input_vertex_valid_mask = 0u;
    state->vertex_color_valid_mask = 0u;
    state->current_transform_vertex_mask = 0u;
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_light_direction_valid = 0u;
    state->texture_prepare_valid = 0u;
    state->modelview = light_modelview;
    state->modelview_valid = TRUE;
    state->matrix = light_modelview;
    state->matrix_valid = TRUE;
    state->matrix_generation = matrix_generation;
    if (stats->first_opcode == 0u)
    {
        stats->first_opcode = NDS_RENDERER_OP_RDPPIPESYNC;
    }
    stats->command_count += root->source_command_count;
    ndsRendererNativeApplyRootLightPreamble(root, stats);
    for (epoch_offset = 0u;
         epoch_offset < root->epoch_count;
         epoch_offset++)
    {
        u32 epoch_index = root->first_epoch + epoch_offset;
        const NDSNativeEpoch *epoch =
            &sNdsNativeFighterEpochs[epoch_index];
        const NDSNativeHierarchyPreparedEpoch *prepared_epoch =
            &execution->hierarchy_epochs[epoch_index];
        u32 run_offset;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        u32 m2_lighting_start;
#endif

        ndsRendererNativeApplyStateSpan(
            epoch->before_state_first, epoch->before_state_count,
            epoch->before_sync_count, asset_base, stats, state);
        if (epoch->material_slot != NDS_NATIVE_MATERIAL_NONE)
        {
            ndsRendererNativeApplyMaterial(
                &input->materials[epoch->material_slot], stats, state);
        }
        ndsRendererNativeApplyStateSpan(
            epoch->after_state_first, epoch->after_state_count,
            epoch->after_sync_count, asset_base, stats, state);
        state->prepared_light_direction =
            prepared_epoch->light_direction;
        state->prepared_light_direction_valid =
            prepared_epoch->light_direction_valid;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        m2_lighting_start = cpuGetTiming();
#endif
        (void)ndsRendererNativeShadeProductionActions(
            epoch, sNdsNativeFighterEpochDirectPolicy[epoch_index],
            FALSE, stats, state);
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        if (m2_owner != NULL)
        {
            m2_owner->m2_lighting_shading_ticks +=
                cpuGetTiming() - m2_lighting_start;
        }
#endif

        for (run_offset = 0u;
             run_offset < epoch->run_count;
             run_offset++)
        {
            u32 run_index = epoch->first_run + run_offset;
            const NDSNativeRun *run =
                &sNdsNativeFighterRuns[run_index];
            const NDSNativeHierarchyPreparedRun *prepared_run =
                &execution->hierarchy_runs[epoch_index];
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            u32 m2_phase_start = cpuGetTiming();
#endif

            ndsRendererNativePrepareHierarchyTexcoords(
                run_index, prepared_run);
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            if (m2_owner != NULL)
            {
                m2_owner->m2_run_prepare_ticks +=
                    cpuGetTiming() - m2_phase_start;
            }
            m2_phase_start = cpuGetTiming();
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
            ndsRendererNativeBeginHierarchyBatch(
                stats, prepared_run, matrix_generation);
            if (run->submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
            {
                ndsRendererNativeEmitProductionCrossRun(
                    run_index, (u32)run->triangle_count * 3u,
                    prepared_run->textured, current_palette_slot, NULL);
            }
            else
            {
                if (prepared_run->textured != 0u)
                {
                    ndsRendererNativeEmitProductionRawTexturedRun(
                        run_index, (u32)run->triangle_count * 3u);
                }
                else
                {
                    ndsRendererNativeEmitProductionRawUntexturedRun(
                        run_index, (u32)run->triangle_count * 3u);
                }
            }
#endif
            stats->triangle_count += run->triangle_count;
            if (run->submit_class == NDS_NATIVE_RUN_RAW_CURRENT)
            {
                ndsRendererFastAccountRawTriangles(
                    stats, run->triangle_count,
                    run->triangle_count - 1u);
            }
            else
            {
                ndsRendererNativeAccountGXCrossTriangles(
                    stats, run->triangle_count,
                    run->triangle_count - 1u);
            }
            (*run_count)++;
            *triangle_count += run->triangle_count;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            if (m2_owner != NULL)
            {
                m2_owner->m2_corner_emit_account_ticks +=
                    cpuGetTiming() - m2_phase_start;
            }
#endif
        }
    }
    ndsRendererNativeApplyStateSpan(
        root->tail_state_first, root->tail_state_count,
        root->tail_sync_count, asset_base, stats, state);
    stats->end_command_count++;
}
#endif

s32 ndsRendererExecuteNativeFighterOwnerHierarchy(
    u32 slot,
    const void *asset_base_ptr,
    const NDSRendererNativeFighterHierarchy *hierarchy,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    u32 *out_hardware_started)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    const u8 *asset_base = asset_base_ptr;
    NDSNativeHierarchyTables tables;
    NDSRendererTraversalState *state =
        &sNdsNativeFighterOwnerExecution.traversal;
    u32 native_run_count = 0u;
    u32 native_triangle_count = 0u;
    u32 matrix_generation = 1u;
    u32 joint_index;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    volatile NDSRendererOwnerProfile *m2_owner =
        ndsRendererProfileM2Owner();
    u32 m2_total_start = cpuGetTiming();
    u32 m2_lighting_before = (m2_owner != NULL) ?
        m2_owner->m2_lighting_shading_ticks : 0u;
    u32 m2_root_gx_before = (m2_owner != NULL) ?
        m2_owner->m2_root_gx_ticks : 0u;
    u32 m2_run_prepare_before = (m2_owner != NULL) ?
        m2_owner->m2_run_prepare_ticks : 0u;
    u32 m2_emit_before = (m2_owner != NULL) ?
        m2_owner->m2_corner_emit_account_ticks : 0u;
#endif

    (void)callback_user;
    if (out_hardware_started == NULL)
    {
        return FALSE;
    }
    *out_hardware_started = FALSE;
    if (ndsRendererNativePreflightFighterHierarchy(
            slot, asset_base, hierarchy, callback, stats, &tables) == FALSE)
    {
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        ndsRendererProfileM2FinishProduction(
            m2_owner, m2_total_start, m2_lighting_before,
            m2_root_gx_before, m2_run_prepare_before,
            m2_emit_before, FALSE);
#endif
        return FALSE;
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    if (m2_owner != NULL)
    {
        m2_owner->m2_run_prepare_count += (slot == 0u) ? 30u : 37u;
        m2_owner->m2_lighting_epoch_count += (slot == 0u) ? 18u : 31u;
    }
#endif

    ndsRendererInitTraversalState(
        state, NULL, stats, NULL, NULL, 0u);
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
    {
        m4x4 camera_hardware;

        ndsRendererNativeBuildHierarchyHardwareAffine(
            hierarchy->camera_modelview, &camera_hardware);
        *out_hardware_started = TRUE;
        ndsRendererHardwareEndBatch();
        ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
        {
            m4x4 projection_hardware;
            ndsRendererCopyMtx20p12ToM4x4(
                hierarchy->projection, &projection_hardware);
            glLoadMatrix4x4(&projection_hardware);
        }
        ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
        glLoadMatrix4x4(&camera_hardware);
        ndsRendererProfileRecordMatrixLoad();
        sNdsRendererHardwareMatrixMode =
            NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY;
        sNdsRendererHardwareMatrixGeneration = matrix_generation;
        sNdsRendererHardwareMatrixLoaded = TRUE;
        stats->hardware_matrix_seed_count++;
    }
#endif

    for (joint_index = 0u; joint_index < tables.joint_count; joint_index++)
    {
        u32 packed = tables.schedule[joint_index];
        u32 binding = (packed >> 5) & 31u;
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
        m4x4 local_hardware;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        u32 m2_gx_start = cpuGetTiming();
#endif

        ndsRendererHardwareEndBatch();
        ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
        if ((packed & 0x8000u) != 0u)
        {
            glPushMatrix();
            stats->matrix_push_count++;
        }
        ndsRendererNativeBuildHierarchyHardwareAffine(
            &hierarchy->joint_locals[joint_index], &local_hardware);
        glMultMatrix4x4(&local_hardware);
        matrix_generation = ndsRendererNextMatrixGeneration();
        sNdsRendererHardwareMatrixMode =
            NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY;
        sNdsRendererHardwareMatrixGeneration = matrix_generation;
        sNdsRendererHardwareMatrixLoaded = TRUE;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        if (m2_owner != NULL)
        {
            m2_owner->m2_root_gx_ticks += cpuGetTiming() - m2_gx_start;
        }
#endif
#else
        matrix_generation++;
#endif
        if (binding != 31u)
        {
            u32 palette_slot = tables.cross_slots[binding];

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            u32 m2_gx_start = cpuGetTiming();
#endif
            if (palette_slot <= NDS_NATIVE_GX_MATRIX_SLOT_MAX)
            {
                glStoreMatrix((int)palette_slot);
            }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            if (m2_owner != NULL)
            {
                m2_owner->m2_root_gx_ticks +=
                    cpuGetTiming() - m2_gx_start;
                m2_owner->m2_root_gx_count++;
            }
#endif
#endif
            ndsRendererNativeCommitHierarchyRoot(
                asset_base, &tables.roots[binding],
                &hierarchy->roots[binding],
                tables.binding_joints[binding], palette_slot,
                matrix_generation, stats, state,
                &native_run_count, &native_triangle_count);
        }
        {
            u32 next_parent = (joint_index + 1u < tables.joint_count) ?
                (tables.schedule[joint_index + 1u] & 31u) : 31u;
            u32 cursor = joint_index;

            while (cursor != next_parent)
            {
                u32 cursor_packed = tables.schedule[cursor];
                u32 parent = cursor_packed & 31u;

                if ((cursor_packed & 0x8000u) != 0u)
                {
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                    u32 m2_gx_start = cpuGetTiming();
#endif
                    ndsRendererHardwareEndBatch();
                    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
                    glPopMatrix(1);
                    stats->matrix_pop_count++;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                    if (m2_owner != NULL)
                    {
                        m2_owner->m2_root_gx_ticks +=
                            cpuGetTiming() - m2_gx_start;
                    }
#endif
#endif
                }
                cursor = parent;
            }
        }
    }
    ndsRendererHardwareEndBatch();
    sNdsRendererFastRunCount += native_run_count;
    sNdsRendererFastTriangleCount += native_triangle_count;
    if ((u32)sNdsRendererRuntimeOwner <
        (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererFastOwnerTriangleCount[
            (u32)sNdsRendererRuntimeOwner] += native_triangle_count;
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    if (m2_owner != NULL)
    {
        m2_owner->m2_corner_emit_run_count += native_run_count;
    }
    ndsRendererProfileM2FinishProduction(
        m2_owner, m2_total_start, m2_lighting_before,
        m2_root_gx_before, m2_run_prepare_before,
        m2_emit_before, TRUE);
#endif
    return TRUE;
#else
    (void)slot;
    (void)asset_base_ptr;
    (void)hierarchy;
    (void)callback;
    (void)callback_user;
    (void)stats;
    if (out_hardware_started != NULL)
    {
        *out_hardware_started = FALSE;
    }
    return FALSE;
#endif
}
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static s32 ndsRendererNativeStageValidateStateSpanTopology(
    const NDSNativeStageStateSpan *span)
{
    u32 i;

    if ((span == NULL) ||
        ((u32)span->first_state + span->state_count >
         NDS_NATIVE_STAGE_STATE_SEQUENCE_COUNT))
    {
        return FALSE;
    }
    for (i = 0u; i < span->state_count; i++)
    {
        u32 delta_index = sNdsNativeStageStateSequence[
            (u32)span->first_state + i];
        const NDSNativeStageStateDelta *delta;

        if (delta_index >= NDS_NATIVE_STAGE_STATE_DELTA_COUNT)
        {
            return FALSE;
        }
        delta = &sNdsNativeStageStateDeltas[delta_index];
        if (delta->effect == NDS_NATIVE_STATE_MATERIAL)
        {
            if ((delta->material_event >=
                 NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT) ||
                (delta->material_command >=
                 sNdsNativeStageMaterialEvents[
                     delta->material_event].source_command_count))
            {
                return FALSE;
            }
            continue;
        }
        if (delta->effect == NDS_NATIVE_STATE_BLEND)
        {
            continue;
        }
        if ((delta->effect < NDS_NATIVE_STATE_OTHERMODE) ||
            (delta->effect > NDS_NATIVE_STATE_PRIM))
        {
            return FALSE;
        }
        if ((delta->effect == NDS_NATIVE_STATE_IMAGE) &&
            ((delta->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
             (delta->w1 >= sNdsNativeStageAssets[
                 delta->asset_index].payload_size)))
        {
            return FALSE;
        }
    }
    return TRUE;
}

#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
static u32 ndsRendererNativeStageGeneratedHashU32(u32 hash, u32 value)
{
    u32 shift;

    for (shift = 0u; shift < 32u; shift += 8u)
    {
        hash ^= (value >> shift) & 0xffu;
        hash *= 16777619u;
    }
    return hash;
}

/* This runs only behind the existing generation/stamp full validation.  It
 * binds the compact Task-26 rows to the exact production packet and to the
 * Task-14 dense first-visit plan consumed by PrepareRun. */
static s32 ndsRendererNativeStageValidateGeneratedSegment0(u32 inject_fault)
{
    const NDSNativeStageGeneratedCertificate *certificate =
        &sNdsNativeStageSegment0ColdCertificate;
    const NDSNativeStageSegment *segment = &sNdsNativeStageSegments[0];
    u32 hot_hash = 2166136261u;
    u32 dense_hash = 2166136261u;
    u32 state_count = 0u;
    u32 sync_count = 0u;
    u32 state_cursor = certificate->first_state;
    u32 triangle_count = 0u;
    u64 epoch_mask = 0u;
    u32 i;

    if ((certificate->source_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_SOURCE_CHECKSUM) ||
        (certificate->table_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_TABLE_CHECKSUM) ||
        (certificate->hot_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_HOT_CHECKSUM) ||
        (certificate->prepared_dense_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_CHECKSUM) ||
        (certificate->segment_index != 0u) ||
        (certificate->first_dobj != segment->first_dobj) ||
        (certificate->dobj_count != segment->dobj_count) ||
        (certificate->owner != segment->owner) ||
        (certificate->link != segment->link) ||
        (certificate->submit_class !=
         NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ||
        (certificate->first_binding != segment->first_binding) ||
        (certificate->binding_count != segment->binding_count) ||
        (certificate->first_run != segment->first_run) ||
        (certificate->run_count != segment->run_count) ||
        (certificate->first_texture_epoch != 0u) ||
        (certificate->triangle_count != 54u) ||
        (certificate->texture_epoch_count != 22u) ||
        (certificate->live_operand_mask !=
         (((u32)1u << NDS_NATIVE_STAGE_LIVE_OPERAND_ASSET_BASES) |
          ((u32)1u << NDS_NATIVE_STAGE_LIVE_OPERAND_BINDING_COMPOSED) |
          ((u32)1u << NDS_NATIVE_STAGE_LIVE_OPERAND_CONFIG))) ||
        (certificate->asset_base_mask != 3u) ||
        (certificate->material_count != 0u) ||
        (certificate->final_tail_span != 73u) ||
        (certificate->prepared_dense_count !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_COUNT) ||
        (certificate->prepared_dense_offset_count !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_OFFSET_COUNT) ||
        (sNdsNativeStageValidationCache.prepared_dense_offsets[0] != 0u) ||
        (sNdsNativeStageValidationCache.prepared_dense_offsets[
             NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT] !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_COUNT))
    {
        return FALSE;
    }

    hot_hash = ndsRendererNativeStageGeneratedHashU32(
        hot_hash, 0x4d335348u);
    hot_hash = ndsRendererNativeStageGeneratedHashU32(
        hot_hash, NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT);
    for (i = 0u; i < NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT; i++)
    {
        const NDSNativeStageGeneratedRun *generated =
            &sNdsNativeStageSegment0HotRuns[i];
        u32 run_index = generated->run_index;
        const NDSNativeStageRun *run;
        const NDSNativeStageStateSpan *span;

        if ((inject_fault != 0u) && (i == 0u))
        {
            run_index ^= 1u;
        }
        hot_hash = ndsRendererNativeStageGeneratedHashU32(
            hot_hash, run_index);
        hot_hash = ndsRendererNativeStageGeneratedHashU32(
            hot_hash, generated->binding_composed_index);
        if (run_index != (u32)certificate->first_run + i)
        {
            return FALSE;
        }
        run = &sNdsNativeStageRuns[run_index];
        span = &sNdsNativeStageStateSpans[run_index];
        if ((run->binding_index != generated->binding_composed_index) ||
            (run->submit_class != certificate->submit_class) ||
            (run->flags != 0u) ||
            (run->texture_epoch < certificate->first_texture_epoch) ||
            (run->texture_epoch >=
             (u32)certificate->first_texture_epoch +
                 certificate->texture_epoch_count))
        {
            return FALSE;
        }
        if ((span->state_count != 0u) &&
            (span->first_state != state_cursor))
        {
            return FALSE;
        }
        state_cursor += span->state_count;
        state_count += span->state_count;
        sync_count += span->sync_count;
        triangle_count += run->triangle_count;
        epoch_mask |= (u64)1u << run->texture_epoch;
    }
    {
        const NDSNativeStageStateSpan *tail =
            &sNdsNativeStageStateSpans[certificate->final_tail_span];

        if ((tail->state_count != 0u) &&
            (tail->first_state != state_cursor))
        {
            return FALSE;
        }
        state_cursor += tail->state_count;
        state_count += tail->state_count;
        sync_count += tail->sync_count;
    }
    if ((hot_hash != certificate->hot_checksum) ||
        (state_count != certificate->state_count) ||
        (sync_count != certificate->sync_count) ||
        (state_cursor !=
         (u32)certificate->first_state + certificate->state_count) ||
        (triangle_count != certificate->triangle_count) ||
        (epoch_mask != (((u64)1u << certificate->texture_epoch_count) - 1u)))
    {
        return FALSE;
    }

    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, 0x4d33535du);
    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, certificate->prepared_dense_offset_count);
    for (i = 0u; i < certificate->prepared_dense_offset_count; i++)
    {
        dense_hash = ndsRendererNativeStageGeneratedHashU32(
            dense_hash,
            sNdsNativeStageValidationCache.prepared_dense_offsets[i]);
    }
    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, 0x4d33535eu);
    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, certificate->prepared_dense_count);
    for (i = 0u; i < certificate->prepared_dense_count; i++)
    {
        dense_hash = ndsRendererNativeStageGeneratedHashU32(
            dense_hash,
            sNdsNativeStageValidationCache.prepared_dense_indices[i]);
    }
    if (dense_hash != certificate->prepared_dense_checksum)
    {
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL == 1
    if (inject_fault == 0u)
    {
        gNdsRendererM3GeneratedSegment0CertificateValidationCount++;
    }
#endif
    return TRUE;
}
#endif

static s32 ndsRendererNativeStageValidateTopologyFull(
    const NDSRendererNativeStageFrame *frame,
    NDSNativeStageTopologySummary *summary)
{
    u32 prepared_dense_mask[
        (NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT + 31u) / 32u];
    u32 prepared_dense_count = 0u;
    u32 i;

    if ((frame == NULL) || (summary == NULL) ||
        (frame->topology_generation == 0u) ||
        (frame->topology_stamp == 0u))
    {
        return FALSE;
    }
    memset(summary, 0, sizeof(*summary));
    memset(prepared_dense_mask, 0, sizeof(prepared_dense_mask));
    for (i = 0u; i < NDS_NATIVE_STAGE_DOBJ_COUNT; i++)
    {
        const NDSNativeStageDObj *expected = &sNdsNativeStageDObjs[i];
        const NDSRendererNativeStageDObj *live = &frame->dobjs[i];

        if ((live->identity == NULL) ||
            (live->parent_index != expected->parent_index) ||
            (live->binding_index != expected->binding_index) ||
            (live->transform_flags != expected->transform_flags) ||
            (live->owner != expected->owner) ||
            (live->depth != expected->depth))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_ASSET_COUNT; i++)
    {
        if ((frame->asset_bases[i] == NULL) ||
            (sNdsNativeStageAssets[i].payload_size == 0u))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_SEGMENT_COUNT; i++)
    {
        const NDSNativeStageSegment *segment = &sNdsNativeStageSegments[i];
        u32 binding_offset;

        if (((u32)segment->first_dobj + segment->dobj_count >
             NDS_NATIVE_STAGE_DOBJ_COUNT) ||
            ((u32)segment->first_binding + segment->binding_count >
             NDS_NATIVE_STAGE_BINDING_COUNT) ||
            ((u32)segment->first_run + segment->run_count >
             NDS_NATIVE_STAGE_RUN_COUNT) ||
            (segment->reserved != 0u))
        {
            return FALSE;
        }
        for (binding_offset = 0u;
             binding_offset < segment->binding_count;
             binding_offset++)
        {
            u32 binding_index =
                (u32)segment->first_binding + binding_offset;
            const NDSNativeStageBinding *binding =
                &sNdsNativeStageBindings[binding_index];

            if ((binding->first_run < segment->first_run) ||
                ((u32)binding->first_run + binding->run_count >
                 (u32)segment->first_run + segment->run_count))
            {
                return FALSE;
            }
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_BINDING_COUNT; i++)
    {
        const NDSNativeStageBinding *binding = &sNdsNativeStageBindings[i];

        if ((binding->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
            (binding->root_offset >= sNdsNativeStageAssets[
                 binding->asset_index].payload_size) ||
            ((u32)binding->first_vertex + binding->source_vertex_count >
             NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT) ||
            ((u32)binding->first_run + binding->run_count >
             NDS_NATIVE_STAGE_RUN_COUNT) ||
            ((u32)binding->first_epoch + binding->texture_epoch_count >
             NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT) ||
            ((binding->material_event != 0xffu) &&
             (binding->material_event >=
              NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT)) ||
            (frame->binding_display_lists[i] !=
             (const void *)((const u8 *)frame->asset_bases[
                 binding->asset_index] + binding->root_offset)))
        {
            return FALSE;
        }
        {
            u32 run_offset;

            for (run_offset = 0u; run_offset < binding->run_count;
                 run_offset++)
            {
                if (sNdsNativeStageRuns[
                        (u32)binding->first_run + run_offset].binding_index != i)
                {
                    return FALSE;
                }
            }
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT; i++)
    {
        const NDSNativeStageTextureEpoch *epoch =
            &sNdsNativeStageTextureEpochs[i];

        if ((epoch->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
            (epoch->source_command_offset >= sNdsNativeStageAssets[
                 epoch->asset_index].payload_size) ||
            (epoch->policy_index >= NDS_NATIVE_STAGE_STATE_POLICY_COUNT) ||
            ((epoch->material_event != 0xffu) &&
             (epoch->material_event >=
              NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT)))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT; i++)
    {
        const NDSNativeStageMaterialEvent *event =
            &sNdsNativeStageMaterialEvents[i];

        if ((event->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
            (event->mobj_offset >= sNdsNativeStageAssets[
                 event->asset_index].payload_size) ||
            (event->binding_index >= NDS_NATIVE_STAGE_BINDING_COUNT) ||
            (event->segment_index >= NDS_NATIVE_STAGE_SEGMENT_COUNT) ||
            (event->material_slot >=
             NDS_RENDERER_NATIVE_STAGE_MATERIAL_COUNT) ||
            (event->source_command_count == 0u))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_STATE_SPAN_COUNT; i++)
    {
        if (ndsRendererNativeStageValidateStateSpanTopology(
                &sNdsNativeStageStateSpans[i]) == FALSE)
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT; i++)
    {
        if (sNdsNativeStageVertices[i].matrix_binding >=
            NDS_NATIVE_STAGE_BINDING_COUNT)
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_RUN_COUNT; i++)
    {
        const NDSNativeStageRun *run = &sNdsNativeStageRuns[i];
        const NDSNativeStageTextureEpoch *epoch;
        u32 corner_count = (u32)run->triangle_count * 3u;
        u32 corner_offset;
        u32 run_alpha = UINT_MAX;

        if ((run->triangle_count == 0u) ||
            (run->binding_index >= NDS_NATIVE_STAGE_BINDING_COUNT) ||
            (run->texture_epoch >= NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT) ||
            (run->state_policy >= NDS_NATIVE_STAGE_STATE_POLICY_COUNT) ||
            ((run->flags &
              ~NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX) != 0u) ||
            ((u32)run->first_corner + corner_count >
             NDS_NATIVE_STAGE_CORNER_COUNT))
        {
            return FALSE;
        }
        epoch = &sNdsNativeStageTextureEpochs[run->texture_epoch];
        if (epoch->policy_index != run->state_policy)
        {
            return FALSE;
        }
        if (run->submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
        {
            if (run->binding_index != 29u)
            {
                return FALSE;
            }
            summary->raw_triangles += run->triangle_count;
        }
        else if (run->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
        {
            summary->projected_no_z_triangles += run->triangle_count;
        }
        else if (run->submit_class ==
                 NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
        {
            summary->projected_range_triangles += run->triangle_count;
        }
        else
        {
            return FALSE;
        }
        sNdsNativeStageValidationCache.prepared_dense_offsets[i] =
            (u16)prepared_dense_count;
        for (corner_offset = 0u; corner_offset < corner_count; corner_offset++)
        {
            u32 dense_index = sNdsNativeStageCorners[
                (u32)run->first_corner + corner_offset];
            const NDSNativeStageDenseVertex *dense;

            if (dense_index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT)
            {
                return FALSE;
            }
            dense = &sNdsNativeStageVertices[dense_index];
            if (run_alpha == UINT_MAX)
            {
                run_alpha = dense->rgba & 0xffu;
            }
            else if (run_alpha != (dense->rgba & 0xffu))
            {
                return FALSE;
            }
            if ((prepared_dense_mask[dense_index / 32u] &
                 ((u32)1u << (dense_index & 31u))) == 0u)
            {
                if (prepared_dense_count >=
                    NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT)
                {
                    return FALSE;
                }
                prepared_dense_mask[dense_index / 32u] |=
                    (u32)1u << (dense_index & 31u);
                sNdsNativeStageValidationCache.prepared_dense_indices[
                    prepared_dense_count++] = (u16)dense_index;
            }
            if (run->submit_class ==
                NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
            {
                s32 x = ndsRendererNativeStageVertexShift(dense->x, 1u);
                s32 y = ndsRendererNativeStageVertexShift(dense->y, 1u);
                s32 z = ndsRendererNativeStageVertexShift(dense->z, 1u);

                if ((run->binding_index != 29u) ||
                    (dense->matrix_binding != run->binding_index) ||
                    (x < -2048) || (x > 2047) ||
                    (y < -2048) || (y > 2047) ||
                    (z < -2048) || (z > 2047))
                {
                    return FALSE;
                }
            }
        }
        if (run_alpha == UINT_MAX)
        {
            return FALSE;
        }
        if ((run->flags &
             NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX) != 0u)
        {
            if ((run->submit_class !=
                 NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ||
                (run->triangle_count != 2u))
            {
                return FALSE;
            }
            summary->cross_runs++;
            summary->cross_triangles += run->triangle_count;
            for (corner_offset = 0u;
                 corner_offset < corner_count;
                 corner_offset++)
            {
                u32 dense_index = sNdsNativeStageCorners[
                    (u32)run->first_corner + corner_offset];

                if (sNdsNativeStageVertices[dense_index].matrix_binding !=
                    run->binding_index)
                {
                    summary->cross_foreign_corners++;
                }
            }
        }
    }
    sNdsNativeStageValidationCache.prepared_dense_offsets[
        NDS_NATIVE_STAGE_RUN_COUNT] = (u16)prepared_dense_count;
#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
    if ((NDS_NATIVE_STAGE_STATE_SPAN_COUNT !=
         NDS_NATIVE_STAGE_RUN_COUNT + NDS_NATIVE_STAGE_BINDING_COUNT) ||
        (prepared_dense_count != NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT) ||
        (summary->raw_triangles != 66u) ||
        (summary->projected_no_z_triangles != 126u) ||
        (summary->projected_range_triangles != 10u) ||
        (summary->cross_runs != 5u) ||
        (summary->cross_triangles != 10u) ||
        (summary->cross_foreign_corners != 15u))
    {
        return FALSE;
    }
    return ndsRendererNativeStageValidateGeneratedSegment0(FALSE);
#else
    return ((NDS_NATIVE_STAGE_STATE_SPAN_COUNT ==
             NDS_NATIVE_STAGE_RUN_COUNT + NDS_NATIVE_STAGE_BINDING_COUNT) &&
            (prepared_dense_count == NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT) &&
            (summary->raw_triangles == 66u) &&
            (summary->projected_no_z_triangles == 126u) &&
            (summary->projected_range_triangles == 10u) &&
            (summary->cross_runs == 5u) &&
            (summary->cross_triangles == 10u) &&
            (summary->cross_foreign_corners == 15u)) ? TRUE : FALSE;
#endif
}

static s32 ndsRendererNativeStageValidateTopology(
    const NDSRendererNativeStageFrame *frame,
    NDSNativeStageTopologySummary *summary)
{
    s32 injected_fault = FALSE;

#if NDS_RENDERER_M3_PHASE0_PROFILE
    if ((sNdsNativeStageValidationCache.valid != FALSE) &&
        (sNdsNativeStageTopologyFaultInjected == FALSE))
    {
        sNdsNativeStageValidationCache.stamp ^= 1u;
        sNdsNativeStageTopologyFaultInjected = TRUE;
        gNdsRendererM3TopologyFaultInjectionCount++;
        injected_fault = TRUE;
    }
#endif
    if ((sNdsNativeStageValidationCache.valid != FALSE) &&
        (sNdsNativeStageValidationCache.generation ==
         frame->topology_generation) &&
        (sNdsNativeStageValidationCache.stamp == frame->topology_stamp))
    {
        *summary = sNdsNativeStageValidationCache.summary;
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3TopologyCacheHitCount++;
#endif
        return TRUE;
    }
    if (sNdsNativeStageValidationCache.valid != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3TopologyStampMismatchCount++;
#endif
    }
    sNdsNativeStageValidationCache.valid = FALSE;
    if (ndsRendererNativeStageValidateTopologyFull(frame, summary) == FALSE)
    {
        return FALSE;
    }
    sNdsNativeStageValidationCache.summary = *summary;
    sNdsNativeStageValidationCache.generation = frame->topology_generation;
    sNdsNativeStageValidationCache.stamp = frame->topology_stamp;
    sNdsNativeStageValidationCache.valid = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3TopologyFullValidationCount++;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    if (injected_fault != FALSE)
    {
        gNdsRendererM3TopologyFaultRevalidationCount++;
    }
#else
    (void)injected_fault;
#endif
    return TRUE;
}

static s32 ndsRendererNativeStageApplyStateSpan(
    const NDSNativeStageStateSpan *span,
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 i;

    if ((span == NULL) || (frame == NULL) || (stats == NULL) ||
        (state == NULL))
    {
        return FALSE;
    }
    stats->sync_command_count += span->sync_count;
    for (i = 0u; i < span->state_count; i++)
    {
        u32 delta_index = sNdsNativeStageStateSequence[
            (u32)span->first_state + i];
        const NDSNativeStageStateDelta *delta =
            &sNdsNativeStageStateDeltas[delta_index];
        if (delta->effect == NDS_NATIVE_STATE_MATERIAL)
        {
            if (delta->material_command == 0u)
            {
                ndsRendererNativeApplyMaterialPreflight(
                    &frame->materials[delta->material_event], stats, state);
            }
            continue;
        }
        if (delta->effect == NDS_NATIVE_STATE_BLEND)
        {
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            stats->blend_color = delta->w1;
            stats->color_command_count++;
            continue;
        }
        {
            NDSNativeStateDelta native_delta;
            const u8 *asset_base = frame->asset_bases[0];

            native_delta.w0 = delta->w0;
            native_delta.w1 = delta->w1;
            native_delta.effect = delta->effect;
            native_delta.reserved[0] = 0u;
            native_delta.reserved[1] = 0u;
            native_delta.reserved[2] = 0u;
            if (delta->effect == NDS_NATIVE_STATE_IMAGE)
            {
                asset_base = frame->asset_bases[delta->asset_index];
            }
            ndsRendererNativeApplyStateDelta(
                &native_delta, asset_base, stats, state);
        }
    }
    return TRUE;
}

static s32 ndsRendererNativeStagePolicyMatches(
    const NDSNativeStageStatePolicy *policy,
    const NDSRendererStats *stats)
{
    return ((policy != NULL) && (stats != NULL) &&
            (stats->texture_combine_w0 == policy->combine_w0) &&
            (stats->texture_combine_w1 == policy->combine_w1) &&
            (stats->othermode_h == policy->othermode_h) &&
            (stats->othermode_l == policy->othermode_l) &&
            (stats->geometry_mode == policy->geometry_mode)) ? TRUE : FALSE;
}

static void ndsRendererNativeStageInputVertex(
    const NDSNativeStageDenseVertex *dense,
    NDSRendererInputVertex *input)
{
    input->x = dense->x;
    input->y = dense->y;
    input->z = dense->z;
    input->s = dense->s;
    input->t = dense->t;
    input->r = (u8)(dense->rgba >> 24);
    input->g = (u8)(dense->rgba >> 16);
    input->b = (u8)(dense->rgba >> 8);
    input->a = (u8)dense->rgba;
}

static s32 ndsRendererNativeStagePrepareRun(
    u32 run_index,
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u64 *epoch_mask)
{
    const NDSNativeStageRun *run = &sNdsNativeStageRuns[run_index];
    const NDSNativeStageTextureEpoch *epoch;
    const NDSNativeStageStatePolicy *policy;
    NDSNativeStagePreparedRun *prepared =
        &sNdsNativeStageOwnerExecution.runs[run_index];
    const NDSRendererTileState *render_tile;
    NDSRendererHardwareResolvedTexture resolved;
    u32 implicit_texture_on;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 material_color;
    s32 alpha_uses_vertex;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 texture_offset;
    u32 first_visit_offset;
    u32 first_visit_end;
    u32 dense_offset;
    u32 alpha = UINT_MAX;
    u32 use_texture;
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
    gNdsRendererTask36PrepareRunRejectReason = 0u;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 vertex_prepare_start;
    u32 residual_vertex_ticks_start;
    u32 residual_near_ticks_start;
    u32 residual_near_count_start;
#endif

    epoch = &sNdsNativeStageTextureEpochs[run->texture_epoch];
    policy = &sNdsNativeStageStatePolicies[run->state_policy];
    if (ndsRendererNativeStagePolicyMatches(policy, stats) == FALSE)
    {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        gNdsRendererTask36PrepareRunRejectReason = 1u;
#endif
        return FALSE;
    }

    memset(prepared, 0, sizeof(*prepared));
    memset(&resolved, 0, sizeof(resolved));
    use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ?
        TRUE : FALSE;
    implicit_texture_on = ndsRendererHardwareTextureImplicitStateOn(stats);
    texture_scale_s = stats->texture_scale_s;
    texture_scale_t = stats->texture_scale_t;
    render_tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    if ((use_texture != FALSE) &&
        (ndsRendererHardwareResolveStageSourceFrameTexture(
             stats, frame->config, state, &resolved) == FALSE))
    {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        gNdsRendererTask36PrepareRunRejectReason = 2u;
#endif
        return FALSE;
    }
    if ((use_texture != FALSE) && (implicit_texture_on != FALSE))
    {
        if ((stats->texture_state_flags &
             NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
        {
            texture_scale_s = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
        }
        if ((stats->texture_state_flags &
             NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
        {
            texture_scale_t = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
        }
    }
    prepared->texture_entry = resolved.entry;
    prepared->texture_name = resolved.name;
    prepared->texture_params = resolved.params;
    prepared->texture_format = (u8)resolved.format;
    prepared->texture_width = (u16)resolved.width;
    prepared->texture_height = (u16)resolved.height;
    prepared->textured = (u8)use_texture;
    prepared->alpha_test =
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) ==
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD) ? TRUE : FALSE;
    prepared->alpha_ref = (u8)((stats->blend_color & 0xffu) >> 4);

    material_color = ndsRendererHardwareColorSource(stats);
    alpha_uses_vertex = ndsRendererHardwareAlphaUsesVertex(stats);
    use_material_color = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
    if (alpha_uses_vertex == FALSE)
    {
        alpha = ndsRendererHardwareAlpha(stats, NULL);
    }
    if (alpha_uses_vertex != FALSE)
    {
        u32 dense_index = sNdsNativeStageCorners[run->first_corner];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];

        alpha = (dense->rgba & 0xffu) >> 3;
    }
    first_visit_offset =
        sNdsNativeStageValidationCache.prepared_dense_offsets[run_index];
    first_visit_end =
        sNdsNativeStageValidationCache.prepared_dense_offsets[run_index + 1u];
    if ((first_visit_offset > first_visit_end) ||
        (first_visit_end > NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT))
    {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        gNdsRendererTask36PrepareRunRejectReason = 3u;
#endif
        return FALSE;
    }
#if NDS_RENDERER_M3_PHASE0_PROFILE
    residual_vertex_ticks_start = gNdsRendererM3Phase0VertexPrepareTicks;
    residual_near_ticks_start = gNdsRendererM3Phase0NearTransformTicks;
    residual_near_count_start = gNdsRendererM3Phase0NearTransformCount;
    vertex_prepare_start = ndsRendererM3Phase0Tick();
#endif
    for (dense_offset = first_visit_offset;
         dense_offset < first_visit_end;
         dense_offset++)
    {
        u32 dense_index =
            sNdsNativeStageValidationCache.prepared_dense_indices[
                dense_offset];
        const NDSNativeStageDenseVertex *dense;
        NDSNativeStagePreparedDense *prepared_dense;

        if (dense_index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT)
        {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
            gNdsRendererTask36PrepareRunRejectReason = 4u;
#endif
            return FALSE;
        }
        dense = &sNdsNativeStageVertices[dense_index];
        prepared_dense = &sNdsNativeStagePreparedDense[dense_index];
#if NDS_RENDERER_M3_PHASE0_PROFILE
        gNdsRendererM3Phase0PreparedDenseCount++;
#endif
        prepared_dense->packed_color =
            ndsRendererHardwarePackedValidVertexColor(
                material_color, use_material_color,
                use_vertex_color, dense->rgba);
        if (use_texture != FALSE)
        {
            prepared_dense->s = ndsRendererHardwareTexCoord(
                dense->s, texture_scale_s, render_tile->uls,
                texture_offset);
            prepared_dense->t = ndsRendererHardwareTexCoord(
                dense->t, texture_scale_t, render_tile->ult,
                texture_offset);
        }
        if (run->submit_class ==
                  NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
        {
#if NDS_TASK36_HW_COMPOSE
            if ((frame->rigid_binding_mask &
                 ((u64)1u << dense->matrix_binding)) != 0u)
            {
                /* GX performs the rigid near-plane clip after hardware
                 * composition; dynamic bindings keep the exact CPU clip. */
                prepared_dense->near_inside = TRUE;
            }
            else
#endif
            {
            NDSRendererInputVertex input;
            NDSRendererClipVertex20p12 clip;

            ndsRendererNativeStageInputVertex(dense, &input);
#if NDS_RENDERER_M3_PHASE0_PROFILE
            u32 near_transform_start = ndsRendererM3Phase0Tick();
#endif

            ndsRendererTransformVertex20p12(
                &frame->binding_composed[dense->matrix_binding],
                &input, &clip);
#if NDS_RENDERER_M3_PHASE0_PROFILE
            ndsRendererM3Phase0FinishSpan(
                &gNdsRendererM3Phase0NearTransformTicks,
                near_transform_start);
            gNdsRendererM3Phase0NearTransformCount++;
#endif
            if (clip.w == 0)
            {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
                gNdsRendererTask36PrepareRunRejectReason = 5u;
#endif
                return FALSE;
            }
            if (ndsRendererHardwareClipZWInsideNearPlane(
                    clip.z, clip.w) != FALSE)
            {
                prepared_dense->near_inside = TRUE;
            }
            else
            {
                prepared_dense->near_inside = FALSE;
            }
            }
        }
    }
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0FinishSpan(
        &gNdsRendererM3Phase0VertexPrepareTicks, vertex_prepare_start);
    if (run_index >= NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT)
    {
        gNdsRendererM3ResidualVertexTicks +=
            gNdsRendererM3Phase0VertexPrepareTicks -
            residual_vertex_ticks_start;
        gNdsRendererM3ResidualNearTicks +=
            gNdsRendererM3Phase0NearTransformTicks -
            residual_near_ticks_start;
        gNdsRendererM3ResidualDenseCount +=
            first_visit_end - first_visit_offset;
        gNdsRendererM3ResidualNearCount +=
            gNdsRendererM3Phase0NearTransformCount -
            residual_near_count_start;
    }
#endif
    if (alpha == UINT_MAX)
    {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        gNdsRendererTask36PrepareRunRejectReason = 6u;
#endif
        return FALSE;
    }
    prepared->poly_fmt = ndsRendererHardwarePolyFmt(stats, alpha);
    *epoch_mask |= (u64)1u << run->texture_epoch;
    return TRUE;
}

#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
static s32 ndsRendererNativeStagePrepareGeneratedSegment0(
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u64 *epoch_mask)
{
    const NDSNativeStageGeneratedCertificate *certificate =
        &sNdsNativeStageSegment0ColdCertificate;

#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3GeneratedSegment0AttemptCount++;
#endif
    if ((frame == NULL) || (stats == NULL) || (state == NULL) ||
        (epoch_mask == NULL) ||
        (sNdsNativeStageValidationCache.valid == FALSE))
    {
        goto fail;
    }
#define NDS_TASK26_SYNC(count) \
    do { \
        stats->sync_command_count += (u32)(count); \
    } while (0)
#define NDS_TASK26_OTHERMODE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordOtherMode( \
            stats, ((u32)(w0)) >> 24, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_COMBINE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetCombine(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_TEXTURE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordTextureState(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_GEOMETRY(clear_mask, set_mask) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        stats->geometry_mode = \
            (stats->geometry_mode & (u32)(clear_mask)) | (u32)(set_mask); \
        stats->geometry_clear_mask = (u32)(clear_mask); \
        stats->geometry_set_mask = (u32)(set_mask); \
        stats->geometry_command_count++; \
    } while (0)
#define NDS_TASK26_IMAGE(asset_index, w0, offset) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetImage( \
            stats, (u32)(w0), \
            (u32)(uintptr_t)(frame->asset_bases[(u32)(asset_index)] + \
                             (u32)(offset))); \
    } while (0)
#define NDS_TASK26_TILE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetTile(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_LOAD_TLUT(w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordLoadTlut(stats, (u32)(w1)); \
    } while (0)
#define NDS_TASK26_LOAD_BLOCK(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordLoadBlock(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_TILE_SIZE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetTileSize(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_BLEND(w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        stats->blend_color = (u32)(w1); \
        stats->color_command_count++; \
    } while (0)
#if NDS_RENDERER_M3_PHASE0_PROFILE
#define NDS_TASK26_RUN(run_index) \
    do { \
        u32 task26_prepare_start = ndsRendererM3Phase0Tick(); \
        s32 task26_prepare_result = ndsRendererNativeStagePrepareRun( \
            (u32)(run_index), frame, stats, state, epoch_mask); \
        ndsRendererM3Phase0FinishSpan( \
            &gNdsRendererM3Phase0PrepareRunTicks, task26_prepare_start); \
        if (task26_prepare_result == FALSE) \
        { \
            goto fail; \
        } \
    } while (0)
#else
#define NDS_TASK26_RUN(run_index) \
    do { \
        if (ndsRendererNativeStagePrepareRun( \
                (u32)(run_index), frame, stats, state, epoch_mask) == FALSE) \
        { \
            goto fail; \
        } \
    } while (0)
#endif
    NDS_NATIVE_STAGE_SEGMENT0_GENERATED_PROGRAM;
#undef NDS_TASK26_RUN
#undef NDS_TASK26_BLEND
#undef NDS_TASK26_TILE_SIZE
#undef NDS_TASK26_LOAD_BLOCK
#undef NDS_TASK26_LOAD_TLUT
#undef NDS_TASK26_TILE
#undef NDS_TASK26_IMAGE
#undef NDS_TASK26_GEOMETRY
#undef NDS_TASK26_TEXTURE
#undef NDS_TASK26_COMBINE
#undef NDS_TASK26_OTHERMODE
#undef NDS_TASK26_SYNC
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3GeneratedSegment0SuccessCount++;
    gNdsRendererM3GeneratedSegment0RunCount += certificate->run_count;
    gNdsRendererM3GeneratedSegment0TriangleCount +=
        certificate->triangle_count;
    gNdsRendererM3GeneratedSegment0EpochCount +=
        certificate->texture_epoch_count;
    gNdsRendererM3GeneratedSegment0MaterialCount +=
        certificate->material_count;
#endif
    return TRUE;

fail:
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3GeneratedSegment0PreGxFallbackCount++;
#endif
    return FALSE;
}

#if NDS_RENDERER_M3_PHASE0_PROFILE
static void ndsRendererNativeStageHashGeneratedSegment0Outputs(
    const NDSRendererStats *stats,
    const NDSRendererTraversalState *state,
    u64 epoch_mask,
    u32 *out_hash_a,
    u32 *out_hash_b,
    u32 *out_field_count)
{
    u32 hash_a = 2166136261u;
    u32 hash_b = 0x9e3779b9u;
    u32 field_count = 0u;
    u32 i;

#define NDS_TASK26_HASH_FIELD(value) \
    do { \
        u32 task26_value = (u32)(value); \
        hash_a = (hash_a ^ task26_value) * 16777619u; \
        hash_b ^= task26_value + 0x9e3779b9u + \
            (hash_b << 6) + (hash_b >> 2); \
        hash_b = ((hash_b << 7) | (hash_b >> 25)) * 0x85ebca6bu; \
        field_count++; \
    } while (0)

    for (i = 0u; i < NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT; i++)
    {
        u32 run_index = sNdsNativeStageSegment0HotRuns[i].run_index;
        const NDSNativeStagePreparedRun *prepared =
            &sNdsNativeStageOwnerExecution.runs[run_index];

        NDS_TASK26_HASH_FIELD((uintptr_t)prepared->texture_entry);
        NDS_TASK26_HASH_FIELD(prepared->texture_name);
        NDS_TASK26_HASH_FIELD(prepared->texture_params);
        NDS_TASK26_HASH_FIELD(prepared->poly_fmt);
        NDS_TASK26_HASH_FIELD(prepared->texture_width);
        NDS_TASK26_HASH_FIELD(prepared->texture_height);
        NDS_TASK26_HASH_FIELD(prepared->texture_format);
        NDS_TASK26_HASH_FIELD(prepared->textured);
        NDS_TASK26_HASH_FIELD(prepared->alpha_test);
        NDS_TASK26_HASH_FIELD(prepared->alpha_ref);
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_COUNT; i++)
    {
        u32 dense_index =
            sNdsNativeStageValidationCache.prepared_dense_indices[i];
        const NDSNativeStagePreparedDense *prepared =
            &sNdsNativeStagePreparedDense[dense_index];

        NDS_TASK26_HASH_FIELD(prepared->packed_color);
        NDS_TASK26_HASH_FIELD((u16)prepared->s);
        NDS_TASK26_HASH_FIELD((u16)prepared->t);
        NDS_TASK26_HASH_FIELD((u16)prepared->near_inside);
    }
    NDS_TASK26_HASH_FIELD(stats->sync_command_count);
    NDS_TASK26_HASH_FIELD(stats->state_command_count);
    NDS_TASK26_HASH_FIELD(stats->othermode_command_count);
    NDS_TASK26_HASH_FIELD(stats->ignored_state_command_count);
    NDS_TASK26_HASH_FIELD(stats->first_othermode_opcode);
    NDS_TASK26_HASH_FIELD(stats->first_othermode_w0);
    NDS_TASK26_HASH_FIELD(stats->first_othermode_w1);
    NDS_TASK26_HASH_FIELD(stats->othermode_h);
    NDS_TASK26_HASH_FIELD(stats->othermode_l);
    NDS_TASK26_HASH_FIELD(stats->geometry_mode);
    NDS_TASK26_HASH_FIELD(stats->geometry_clear_mask);
    NDS_TASK26_HASH_FIELD(stats->geometry_set_mask);
    NDS_TASK26_HASH_FIELD(stats->geometry_command_count);
    NDS_TASK26_HASH_FIELD(stats->texture_mask);
    NDS_TASK26_HASH_FIELD(stats->texture_command_count);
    NDS_TASK26_HASH_FIELD(stats->texture_state_flags);
    NDS_TASK26_HASH_FIELD(stats->texture_image);
    NDS_TASK26_HASH_FIELD(stats->texture_combine_w0);
    NDS_TASK26_HASH_FIELD(stats->texture_combine_w1);
    NDS_TASK26_HASH_FIELD(stats->texture_combine_count);
    NDS_TASK26_HASH_FIELD(stats->color_command_count);
    NDS_TASK26_HASH_FIELD(stats->blend_color);
    NDS_TASK26_HASH_FIELD(stats->texture_source_hash1);
    NDS_TASK26_HASH_FIELD(stats->texture_source_hash2);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_valid);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_enabled);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_name);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_material_color);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_scale_s);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_scale_t);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_origin_s);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_origin_t);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_offset);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_vertex_flags);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_source_zbuffered);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_decal_depth);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_prim_depth);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_alpha_constant);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_poly_alpha);
    NDS_TASK26_HASH_FIELD(state->texture_prepare_poly_fmt);
    NDS_TASK26_HASH_FIELD((u32)epoch_mask);
#undef NDS_TASK26_HASH_FIELD

    *out_hash_a = hash_a;
    *out_hash_b = hash_b;
    *out_field_count = field_count;
}
#endif
#endif

static void ndsRendererNativeStageAccountRun(
    NDSRendererStats *stats, u32 submit_class, u32 triangle_count)
{
    u32 reuse_count = (triangle_count != 0u) ? triangle_count - 1u : 0u;

    sNdsRendererHardwareSubmitClassCounts[submit_class] += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count += reuse_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count += reuse_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices += triangle_count * 3u;
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        sNdsRendererRuntimeFrameSummary.raw_current_candidate_count +=
            triangle_count;
        stats->hardware_zbuffer_triangle_count += triangle_count;
    }
    else
    {
        sNdsRendererRuntimeFrameSummary.projected_submit_fallback_count +=
            triangle_count;
        stats->hardware_projected_depth_triangle_count += triangle_count;
    }
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    sNdsRendererHardwareSubmitted = TRUE;
}

static inline void ndsRendererNativeStageWriteColor(u32 value)
{
    ndsRendererHardwareWriteColorWord(value);
}

static inline void ndsRendererNativeStageWriteTexCoord(u32 value)
{
    ndsRendererHardwareWriteTexCoordWord(value);
}

static inline void ndsRendererNativeStageWriteVertex16(u32 xy, u32 z)
{
    ndsRendererHardwareWriteVertex16Words(xy, z);
}

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
static u32 ndsRendererBenchmarkSegment0NormalizeAssetAddress(
    u32 address, u32 *valid)
{
    u32 asset_index;

    if (address == 0u)
    {
        return 0u;
    }
    for (asset_index = 0u;
         asset_index < NDS_RENDERER_NATIVE_STAGE_ASSET_COUNT;
         asset_index++)
    {
        u32 base = (u32)(uintptr_t)
            sNdsRendererBenchmarkSegment0AssetBases[asset_index];
        u32 offset = address - base;

        if ((base != 0u) && (address >= base) &&
            (offset < sNdsNativeStageAssets[asset_index].payload_size) &&
            (offset < 0x01000000u))
        {
            return ((asset_index + 1u) << 24) | offset;
        }
    }
    *valid = FALSE;
    return 0xffffffffu;
}

static void ndsRendererBenchmarkSegment0HashTextureKey(
    const NDSRendererHardwareTextureCacheEntry *entry,
    u32 *valid,
    u32 *image,
    u32 *tlut,
    u32 *texel1,
    u32 *out_hash_a,
    u32 *out_hash_b)
{
    NDSRendererHardwareTextureKey normalized = entry->key;
    const u32 *words;
    u32 hash_a = 2166136261u;
    u32 hash_b = 0x9e3779b9u;
    u32 i;

    normalized.image = ndsRendererBenchmarkSegment0NormalizeAssetAddress(
        normalized.image, valid);
    normalized.tlut_image =
        ndsRendererBenchmarkSegment0NormalizeAssetAddress(
            normalized.tlut_image, valid);
    normalized.texel1_image =
        ndsRendererBenchmarkSegment0NormalizeAssetAddress(
            normalized.texel1_image, valid);
    *image = normalized.image;
    *tlut = normalized.tlut_image;
    *texel1 = normalized.texel1_image;
    words = (const u32 *)&normalized;
    for (i = 0u; i < (sizeof(normalized) / sizeof(u32)); i++)
    {
        hash_a = ndsRendererBenchmarkSinkHashWordA(hash_a, words[i]);
        hash_b = ndsRendererBenchmarkSinkHashWordB(hash_b, words[i]);
    }
    *out_hash_a = hash_a;
    *out_hash_b = hash_b;
}

static void ndsRendererBenchmarkSegment0ArmRun(
    u32 run_offset,
    const NDSNativeStageRun *run,
    const NDSNativeStagePreparedRun *prepared)
{
    const NDSRendererHardwareTextureCacheEntry *entry;
    const NDSNativeStageTextureEpoch *epoch;
    u32 image = 0u;
    u32 tlut = 0u;
    u32 texel1 = 0u;
    u32 key_hash_a = 0u;
    u32 key_hash_b = 0u;
    u32 valid = TRUE;

    if ((run_offset >= NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT) ||
        (run == NULL) || (prepared == NULL) ||
        (run->texture_epoch >= NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT))
    {
        sNdsRendererBenchmarkSegment0SinkArmFaults++;
        sNdsRendererBenchmarkSegment0TextureValid = FALSE;
        return;
    }
    epoch = &sNdsNativeStageTextureEpochs[run->texture_epoch];
    entry = prepared->texture_entry;
    if (prepared->textured != 0u)
    {
        if ((entry == NULL) || (entry->ready == 0u) ||
            (prepared->texture_name == 0u) ||
            (prepared->texture_name != (u32)entry->name))
        {
            valid = FALSE;
        }
        if (entry != NULL)
        {
            ndsRendererBenchmarkSegment0HashTextureKey(
                entry, &valid, &image, &tlut, &texel1,
                &key_hash_a, &key_hash_b);
        }
    }
    else if ((entry != NULL) || (prepared->texture_name != 0u))
    {
        valid = FALSE;
    }
    sNdsRendererBenchmarkSegment0TextureEpochPlus1 =
        (u32)run->texture_epoch + 1u;
    sNdsRendererBenchmarkSegment0TextureEpochSourceOffset =
        epoch->source_command_offset;
    sNdsRendererBenchmarkSegment0TextureEpochMetadata =
        (u32)epoch->asset_index |
        ((u32)epoch->policy_index << 8) |
        ((u32)epoch->material_event << 16) |
        ((u32)epoch->flags << 24);
    sNdsRendererBenchmarkSegment0TextureImage = image;
    sNdsRendererBenchmarkSegment0TextureTlut = tlut;
    sNdsRendererBenchmarkSegment0TextureTexel1 = texel1;
    sNdsRendererBenchmarkSegment0TextureKeyHashA = key_hash_a;
    sNdsRendererBenchmarkSegment0TextureKeyHashB = key_hash_b;
    sNdsRendererBenchmarkSegment0TextureDescriptor =
        ((u32)prepared->texture_format & 0xffu) |
        (((u32)prepared->texture_width & 0xfffu) << 8) |
        (((u32)prepared->texture_height & 0xfffu) << 20);
    sNdsRendererBenchmarkSegment0TextureFlags =
        (u32)prepared->textured |
        ((u32)prepared->alpha_test << 8) |
        ((u32)prepared->alpha_ref << 16);
    sNdsRendererBenchmarkSegment0TextureParams = prepared->texture_params;
    sNdsRendererBenchmarkSegment0TexturePolyFmt = prepared->poly_fmt;
    sNdsRendererBenchmarkSegment0TextureBinding = run->binding_index;
    sNdsRendererBenchmarkSegment0TextureValid = valid;
    gNdsRendererBenchmarkSegment0RunRawTextureName[run_offset] =
        prepared->texture_name;
    gNdsRendererBenchmarkSegment0RunTextureEpochPlus1[run_offset] =
        sNdsRendererBenchmarkSegment0TextureEpochPlus1;
    gNdsRendererBenchmarkSegment0RunTextureImage[run_offset] = image;
    gNdsRendererBenchmarkSegment0RunTextureTlut[run_offset] = tlut;
    gNdsRendererBenchmarkSegment0RunTextureKeyHashA[run_offset] = key_hash_a;
    gNdsRendererBenchmarkSegment0RunTextureKeyHashB[run_offset] = key_hash_b;
    gNdsRendererBenchmarkSegment0RunTextureDescriptor[run_offset] =
        sNdsRendererBenchmarkSegment0TextureDescriptor;
    gNdsRendererBenchmarkSegment0RunTextureParams[run_offset] =
        prepared->texture_params;
    if (valid == FALSE)
    {
        sNdsRendererBenchmarkSegment0SinkArmFaults++;
    }
}

static void ndsRendererBenchmarkSegment0CheckpointRun(u32 run_offset)
{
    if (run_offset >= NDS_RENDERER_BENCHMARK_SEGMENT0_RUN_COUNT)
    {
        sNdsRendererBenchmarkSegment0SinkArmFaults++;
        return;
    }
    gNdsRendererBenchmarkSegment0RunWords[run_offset] =
        sNdsRendererBenchmarkSegment0SinkWords;
    gNdsRendererBenchmarkSegment0RunHashA[run_offset] =
        sNdsRendererBenchmarkSegment0SinkHashA;
    gNdsRendererBenchmarkSegment0RunHashB[run_offset] =
        sNdsRendererBenchmarkSegment0SinkHashB;
}
#endif

#if NDS_TASK36_HW_COMPOSE
static s32 ndsRendererNativeStageTask36BindingIsRigid(u32 binding_index)
{
    return ((binding_index < NDS_NATIVE_STAGE_BINDING_COUNT) &&
            ((sNdsNativeStageOwnerExecution.rigid_binding_mask &
              ((u64)1u << binding_index)) != 0u)) ? TRUE : FALSE;
}

static s32 ndsRendererNativeStageTask36BuildWorld(
    u32 binding_index, u32 coordinate_shift, m4x4 *hardware)
{
    NDSRendererMatrix20p12 scaled;
    u32 row;
    u32 col;

    if ((binding_index >= NDS_NATIVE_STAGE_BINDING_COUNT) ||
        (sNdsNativeStageOwnerExecution.binding_world == NULL) ||
        (hardware == NULL))
    {
        return FALSE;
    }
    scaled = sNdsNativeStageOwnerExecution.binding_world[binding_index];
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            s64 value = (s64)scaled.m[row][col] << coordinate_shift;

            if ((value > INT_MAX) || (value < INT_MIN))
            {
                return FALSE;
            }
            scaled.m[row][col] = (s32)value;
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        scaled.m[3][col] = ndsRendererRoundShiftS32Signed(
            scaled.m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    ndsRendererCopyMtx20p12ToM4x4(&scaled, hardware);
    return TRUE;
}

static u32 ndsRendererNativeStageTask36TriangleShift(
    const NDSNativeStageRun *run, u32 triangle_offset)
{
    u32 first_corner = (u32)run->first_corner + triangle_offset * 3u;
    u32 coordinate_shift = 0u;
    u32 corner_offset;

    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];
        u32 vertex_shift = sNdsNativeStageVertices[dense_index].packed_cache_shift >>
            NDS_NATIVE_STAGE_COORDINATE_SHIFT;

        if (vertex_shift > coordinate_shift)
        {
            coordinate_shift = vertex_shift;
        }
    }
    return coordinate_shift;
}

static s32 ndsRendererNativeStageTask36BeginSegment(void)
{
    m4x4 projection_hardware;
    m4x4 camera_hardware;

    if ((sNdsNativeStageOwnerExecution.projection == NULL) ||
        (sNdsNativeStageOwnerExecution.camera_modelview == NULL) ||
        (sNdsNativeStageOwnerExecution.binding_world == NULL))
    {
        return FALSE;
    }
    ndsRendererHardwareEndBatch();
    ndsRendererCopyMtx20p12ToM4x4(
        sNdsNativeStageOwnerExecution.projection, &projection_hardware);
    ndsRendererNativeBuildHierarchyHardwareAffine(
        sNdsNativeStageOwnerExecution.camera_modelview, &camera_hardware);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hardware);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadMatrix4x4(&camera_hardware);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE;
    sNdsRendererHardwareMatrixGeneration = ndsRendererNextMatrixGeneration();
    sNdsRendererHardwareMatrixLoaded = TRUE;
    sNdsNativeStageOwnerExecution.task36_binding = UINT_MAX;
    sNdsNativeStageOwnerExecution.task36_coordinate_shift = UINT_MAX;
    sNdsNativeStageOwnerExecution.task36_local_pushed = FALSE;
    sNdsNativeStageOwnerExecution.task36_segment_active = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36CameraLoadCount++;
#endif
    return TRUE;
}

static s32 ndsRendererNativeStageTask36EnsureWorld(
    u32 binding_index, u32 coordinate_shift)
{
    m4x4 world_hardware;

    if ((sNdsNativeStageOwnerExecution.task36_segment_active == FALSE) ||
        (ndsRendererNativeStageTask36BindingIsRigid(binding_index) == FALSE) ||
        (ndsRendererNativeStageTask36BuildWorld(
             binding_index, coordinate_shift, &world_hardware) == FALSE))
    {
        return FALSE;
    }
    if ((sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE) &&
        (sNdsNativeStageOwnerExecution.task36_binding == binding_index) &&
        (sNdsNativeStageOwnerExecution.task36_coordinate_shift ==
         coordinate_shift))
    {
        return TRUE;
    }
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    if (sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE)
    {
        glPopMatrix(1);
    }
    glPushMatrix();
    glMultMatrix4x4(&world_hardware);
    sNdsNativeStageOwnerExecution.task36_binding = binding_index;
    sNdsNativeStageOwnerExecution.task36_coordinate_shift = coordinate_shift;
    sNdsNativeStageOwnerExecution.task36_local_pushed = TRUE;
    sNdsRendererHardwareMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE;
    sNdsRendererHardwareMatrixGeneration = ndsRendererNextMatrixGeneration();
    sNdsRendererHardwareMatrixLoaded = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    {
        u64 binding_bit = (u64)1u << binding_index;

        if ((sNdsNativeStageOwnerExecution.task36_seen_binding_mask &
             binding_bit) == 0u)
        {
            sNdsNativeStageOwnerExecution.task36_seen_binding_mask |=
                binding_bit;
            gNdsRendererTask36HardwareComposedDObjCount++;
        }
    }
    gNdsRendererTask36WorldMultCount++;
#endif
    return TRUE;
}

static void ndsRendererNativeStageTask36LoadNoZProjection(s16 projected_z)
{
    NDSRendererMatrix20p12 projection =
        *sNdsNativeStageOwnerExecution.projection;
    m4x4 projection_hardware;
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        projection.m[row][2] = (s32)ndsRendererRoundShiftS64(
            (s64)projection.m[row][3] * projected_z, 12u);
    }
    ndsRendererCopyMtx20p12ToM4x4(&projection, &projection_hardware);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hardware);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    ndsRendererProfileRecordMatrixLoad();
#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererM3Phase0NoZMatrixCount++;
#endif
}

static void ndsRendererNativeStageTask36EndSegment(void)
{
    if (sNdsNativeStageOwnerExecution.task36_segment_active == FALSE)
    {
        return;
    }
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    if (sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE)
    {
        glPopMatrix(1);
    }
    sNdsNativeStageOwnerExecution.task36_local_pushed = FALSE;
    sNdsNativeStageOwnerExecution.task36_segment_active = FALSE;
    sNdsRendererHardwareMatrixLoaded = FALSE;
}
#endif

static void ndsRendererNativeStageBeginRun(
    const NDSNativeStageRun *native_run,
    const NDSNativeStagePreparedRun *run,
    u32 submit_class,
    u32 segment_owner,
    NDSRendererStats *stats,
    u32 replay)
{
    u32 poly_fmt = run->poly_fmt;

    /* Dream Land's four static layer owners are closed front-facing stage
     * surfaces.  Keep actor owners (Whispy and flowers) two-sided. */
    if ((submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) &&
        (segment_owner < NDS_RENDERER_NATIVE_STAGE_STATIC_OWNER_COUNT) &&
        ((poly_fmt & POLY_CULL_NONE) == POLY_CULL_NONE))
    {
        poly_fmt &= ~((u32)POLY_CULL_NONE);
        poly_fmt |= POLY_CULL_BACK;
    }
    ndsRendererHardwareEndBatch();
    if (replay == FALSE)
    {
#if NDS_TASK36_HW_COMPOSE
    if (ndsRendererNativeStageTask36BindingIsRigid(
            native_run->binding_index) != FALSE)
    {
        u32 coordinate_shift =
            (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
                1u :
            (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ?
                ndsRendererNativeStageTask36TriangleShift(native_run, 0u) : 0u;

        if ((sNdsNativeStageOwnerExecution.task36_segment_active == FALSE) &&
            (ndsRendererNativeStageTask36BeginSegment() == FALSE))
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
        }
        if (ndsRendererNativeStageTask36EnsureWorld(
                native_run->binding_index, coordinate_shift) == FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
        }
    }
    else
#endif
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        ndsRendererLoadHardwareRawComposedMatrix(
            &sNdsNativeStageOwnerExecution.raw_composed, 1u);
    }
    else if (submit_class ==
             NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
    {
        NDSRendererMatrix20p12 projection;

        ndsRendererMtxIdentity20p12(&projection);
        ndsRendererLoadHardwareMatrixPair(
            &projection,
            &sNdsNativeStageOwnerExecution.scaled_raw_modelview,
            NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED, 2u, TRUE);
    }
    else
    {
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
    }
    }
    glEnable(GL_TEXTURE_2D);
    if (run->textured != 0u)
    {
        run->texture_entry->last_used_frame =
            sNdsRendererHardwareFrameSerial + 1u;
        run->texture_entry->params = run->texture_params;
        ndsRendererHardwareBindTextureName(stats, run->texture_name);
        ndsRendererHardwareApplyTextureParams(run->texture_params);
        sNdsRendererHardwareActiveTextureEntry = run->texture_entry;
        if (run->texture_entry->pinned != 0u)
        {
            ndsRendererHardwareRecordBattleStaticTextureHit(
                run->texture_entry);
        }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = run->texture_format;
        stats->hardware_texture_width = run->texture_width;
        stats->hardware_texture_height = run->texture_height;
    }
    else
    {
        ndsRendererHardwareBindNoTexture(NULL);
    }
    if (run->alpha_test != 0u)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(run->alpha_ref);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }
    glDisable(GL_FOG);
    ndsRendererHardwareSetPolyFmt(poly_fmt);
    if (replay == FALSE)
    {
        glBegin(GL_TRIANGLE);
        ndsRendererProfileRecordBatchBegin();
        sNdsRendererHardwareTriangleBatchOpen = TRUE;
        sNdsRendererHardwareTriangleBatchTextured = run->textured;
        sNdsRendererHardwareTriangleBatchTextureName = run->texture_name;
        sNdsRendererHardwareTriangleBatchPolyFmt = poly_fmt;
        sNdsRendererHardwareTriangleBatchAlphaKey = run->alpha_test;
        sNdsRendererHardwareTriangleBatchFogKey = 0u;
        sNdsRendererHardwareTriangleBatchMatrixMode =
#if NDS_TASK36_HW_COMPOSE
            (ndsRendererNativeStageTask36BindingIsRigid(
                 native_run->binding_index) != FALSE) ?
                NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE :
#endif
            ((submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) ||
             (submit_class ==
              NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)) ?
                NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED :
                NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY;
        sNdsRendererHardwareTriangleBatchMatrixGeneration =
#if NDS_TASK36_HW_COMPOSE
            (ndsRendererNativeStageTask36BindingIsRigid(
                 native_run->binding_index) != FALSE) ?
                sNdsRendererHardwareMatrixGeneration :
#endif
            (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) ?
                1u :
            (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
                2u : 0u;
    }
}

#if NDS_TASK36_HW_COMPOSE == 2
static s32 NDS_RENDERER_FAST_RUN_CODE ndsRendererTask36ReplayRun(
    u32 run_index,
    const NDSNativeStageRun *native_run,
    u32 segment_owner,
    NDSRendererStats *stats)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;
    const NDSRendererTask36ReplayRun *run;
    const u32 *words;
    u32 i;

    if ((run_index >= NDS_NATIVE_STAGE_RUN_COUNT) ||
        (native_run == NULL) || (stats == NULL))
    {
        return FALSE;
    }
    run = &owner->runs[run_index];
    if ((run->valid == FALSE) || (run->word_count == 0u) ||
        ((u32)run->word_offset + (u32)run->word_count > owner->word_count))
    {
        return FALSE;
    }
    ndsRendererNativeStageBeginRun(
        native_run, &run->prepared, native_run->submit_class,
        segment_owner, stats, TRUE);
    words = &owner->words[run->word_offset];
    for (i = 0u; i < run->word_count; i++)
    {
        GFX_FIFO = words[i];
    }
    sNdsNativeStageOwnerExecution.task36_local_pushed = TRUE;
    sNdsNativeStageOwnerExecution.task36_binding = native_run->binding_index;
    sNdsRendererHardwareMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE;
    sNdsRendererHardwareMatrixGeneration = ndsRendererNextMatrixGeneration();
    sNdsRendererHardwareMatrixLoaded = TRUE;
    ndsRendererProfileRecordBatchBegin();
    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = run->prepared.textured;
    sNdsRendererHardwareTriangleBatchTextureName = run->prepared.texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = run->prepared.poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = run->prepared.alpha_test;
    sNdsRendererHardwareTriangleBatchFogKey = 0u;
    sNdsRendererHardwareTriangleBatchMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE;
    sNdsRendererHardwareTriangleBatchMatrixGeneration =
        sNdsRendererHardwareMatrixGeneration;
    ndsRendererHardwareEndBatch();
#if NDS_RENDERER_PROFILE_LEVEL == 1
    {
        u64 binding_bit = (u64)1u << native_run->binding_index;

        if ((sNdsNativeStageOwnerExecution.task36_seen_binding_mask &
             binding_bit) == 0u)
        {
            sNdsNativeStageOwnerExecution.task36_seen_binding_mask |=
                binding_bit;
            gNdsRendererTask36HardwareComposedDObjCount++;
        }
    }
    gNdsRendererTask36WorldMultCount += run->world_mult_count;
    gNdsRendererTask36ReplayRunCount++;
    gNdsRendererTask36ReplayWordCount += run->word_count;
#endif
    return TRUE;
}
#endif

static inline void ndsRendererNativeStageEmitVertex(
    const NDSNativeStageDenseVertex *dense,
    const NDSNativeStagePreparedDense *prepared,
    const NDSNativeStagePreparedRun *run,
    u32 submit_class)
{
    ndsRendererNativeStageWriteColor(prepared->packed_color);
    if (run->textured != 0u)
    {
        ndsRendererNativeStageWriteTexCoord(
            (u32)(u16)prepared->s | ((u32)(u16)prepared->t << 16));
    }
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        ndsRendererNativeStageWriteVertex16(
            (u32)(u16)((s32)dense->x *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) |
            ((u32)(u16)((s32)dense->y *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) << 16),
            (u16)((s32)dense->z *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
    }
    else
    {
        ndsRendererNativeStageWriteVertex16(
            (u32)(u16)(ndsRendererNativeStageVertexShift(dense->x, 1u) *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) |
            ((u32)(u16)(ndsRendererNativeStageVertexShift(dense->y, 1u) *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) << 16),
            (u16)(ndsRendererNativeStageVertexShift(dense->z, 1u) *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
    }
}

static void ndsRendererNativeStageSetNoZColumn(
    NDSRendererMatrix20p12 *matrix,
    s16 projected_z)
{
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        matrix->m[row][2] = (s32)ndsRendererRoundShiftS64(
            (s64)matrix->m[row][3] * projected_z, 12u);
    }
}

static void ndsRendererNativeStageLoadNoZMatrix(
    u32 binding_index,
    u32 coordinate_shift,
    s16 projected_z)
{
    NDSRendererMatrix20p12 matrix;
    m4x4 hardware;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 matrix_start = ndsRendererM3Phase0Tick();
#endif

    if (coordinate_shift == 0u)
    {
        ndsRendererBuildRawHardwareMatrix(
            &sNdsNativeStageOwnerExecution.binding_composed[binding_index],
            &matrix);
    }
    else
    {
        (void)ndsRendererBuildShiftedRawHardwareMatrix(
            &sNdsNativeStageOwnerExecution.binding_composed[binding_index],
            &matrix, coordinate_shift);
    }
    ndsRendererNativeStageSetNoZColumn(&matrix, projected_z);
    ndsRendererCopyMtx20p12ToM4x4(&matrix, &hardware);
    glLoadMatrix4x4(&hardware);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixLoaded = FALSE;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0FinishSpan(
        &gNdsRendererM3Phase0NoZMatrixTicks, matrix_start);
    gNdsRendererM3Phase0NoZMatrixCount++;
#endif
}

static void ndsRendererNativeStageEmitNoZVertex(
    const NDSNativeStageDenseVertex *dense,
    const NDSNativeStagePreparedDense *prepared,
    const NDSNativeStagePreparedRun *run,
    u32 coordinate_shift)
{
    s32 x = ndsRendererNativeStageVertexShift(dense->x, coordinate_shift);
    s32 y = ndsRendererNativeStageVertexShift(dense->y, coordinate_shift);
    s32 z = ndsRendererNativeStageVertexShift(dense->z, coordinate_shift);

    ndsRendererNativeStageWriteColor(prepared->packed_color);
    if (run->textured != 0u)
    {
        ndsRendererNativeStageWriteTexCoord(
            (u32)(u16)prepared->s | ((u32)(u16)prepared->t << 16));
    }
    ndsRendererNativeStageWriteVertex16(
        (u32)(u16)(x * 16) | ((u32)(u16)(y * 16) << 16),
        (u16)(z * 16));
}

static void ndsRendererNativeStageEmitClippedVertex(
    const NDSRendererProjectedClipVertex *vertex,
    const NDSNativeStagePreparedRun *run,
    s16 projected_z)
{
    NDSRendererMatrix20p12 matrix;
    m4x4 hardware;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 matrix_start = ndsRendererM3Phase0Tick();
#endif

    memset(&matrix, 0, sizeof(matrix));
    matrix.m[3][0] = ndsRendererRoundShiftS32Signed(vertex->clip.x, 8u);
    matrix.m[3][1] = ndsRendererRoundShiftS32Signed(vertex->clip.y, 8u);
    matrix.m[3][3] = ndsRendererRoundShiftS32Signed(vertex->clip.w, 8u);
    ndsRendererNativeStageSetNoZColumn(&matrix, projected_z);
    ndsRendererCopyMtx20p12ToM4x4(&matrix, &hardware);
    glLoadMatrix4x4(&hardware);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixLoaded = FALSE;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0FinishSpan(
        &gNdsRendererM3Phase0NoZMatrixTicks, matrix_start);
    gNdsRendererM3Phase0NoZMatrixCount++;
#endif

    ndsRendererNativeStageWriteColor(vertex->packed_color);
    if (run->textured != 0u)
    {
        ndsRendererNativeStageWriteTexCoord(
            (u32)(u16)vertex->s | ((u32)(u16)vertex->t << 16));
    }
    ndsRendererNativeStageWriteVertex16(0u, 0u);
}

static u32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererNativeStageEmitNearClippedTriangle(
    const NDSNativeStageRun *run,
    const NDSNativeStagePreparedRun *prepared_run,
    u32 triangle_offset,
    s16 projected_z)
{
    NDSRendererProjectedClipVertex input[3];
    NDSRendererProjectedClipVertex clipped[4];
    u32 corner_offset;
    u32 clipped_count;
    u32 fan_index;

    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[
            (u32)run->first_corner + triangle_offset * 3u + corner_offset];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];
        const NDSNativeStagePreparedDense *prepared =
            &sNdsNativeStagePreparedDense[dense_index];
        NDSRendererInputVertex source;

        ndsRendererNativeStageInputVertex(dense, &source);
        ndsRendererTransformVertex20p12(
            &sNdsNativeStageOwnerExecution.binding_composed[
                dense->matrix_binding],
            &source, &input[corner_offset].clip);
        input[corner_offset].s = prepared->s;
        input[corner_offset].t = prepared->t;
        input[corner_offset].packed_color = prepared->packed_color;
    }
    clipped_count = ndsRendererHardwareClipTriangleNearPlane(input, clipped);
    if (clipped_count < 3u)
    {
        ndsRendererProfileRecordNearPlaneTriangleReject();
        ndsRendererProfileRecordSubmitClass(NDS_RENDERER_HW_SUBMIT_REJECT);
        return 0u;
    }
    for (fan_index = 1u; fan_index + 1u < clipped_count; fan_index++)
    {
        ndsRendererNativeStageEmitClippedVertex(
            &clipped[0], prepared_run, projected_z);
        ndsRendererNativeStageEmitClippedVertex(
            &clipped[fan_index], prepared_run, projected_z);
        ndsRendererNativeStageEmitClippedVertex(
            &clipped[fan_index + 1u], prepared_run, projected_z);
    }
    return clipped_count - 2u;
}

static u32 __attribute__((noinline))
ndsRendererNativeStageEmitNoZTriangle(
    const NDSNativeStageRun *run,
    const NDSNativeStagePreparedRun *prepared_run,
    u32 triangle_offset,
    s16 projected_z)
{
    u32 first_corner = (u32)run->first_corner + triangle_offset * 3u;
    u32 inside_count = 0u;
    u32 corner_offset;
    u32 first_dense_index;
    u32 binding_index;
    u32 coordinate_shift = 0u;
    s32 one_binding = TRUE;

#if NDS_TASK36_HW_COMPOSE
    if (ndsRendererNativeStageTask36BindingIsRigid(run->binding_index) != FALSE)
    {
        coordinate_shift = ndsRendererNativeStageTask36TriangleShift(
            run, triangle_offset);
        for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
        {
            u32 dense_index =
                sNdsNativeStageCorners[first_corner + corner_offset];

            if (sNdsNativeStageVertices[dense_index].matrix_binding !=
                run->binding_index)
            {
#if NDS_RENDERER_PROFILE_LEVEL == 1
                gNdsRendererM3PostArmFailureCount++;
#endif
                return 0u;
            }
        }
        if (ndsRendererNativeStageTask36EnsureWorld(
                run->binding_index, coordinate_shift) == FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
            return 0u;
        }
        ndsRendererNativeStageTask36LoadNoZProjection(projected_z);
        for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
        {
            u32 dense_index =
                sNdsNativeStageCorners[first_corner + corner_offset];

            ndsRendererNativeStageEmitNoZVertex(
                &sNdsNativeStageVertices[dense_index],
                &sNdsNativeStagePreparedDense[dense_index],
                prepared_run, coordinate_shift);
        }
        return 1u;
    }
#endif

    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];

        inside_count +=
            (sNdsNativeStagePreparedDense[dense_index].near_inside != FALSE) ?
                1u : 0u;
    }
    if (inside_count == 0u)
    {
        ndsRendererProfileRecordNearPlaneTriangleReject();
        ndsRendererProfileRecordSubmitClass(NDS_RENDERER_HW_SUBMIT_REJECT);
        return 0u;
    }
    if (inside_count != 3u)
    {
        return ndsRendererNativeStageEmitNearClippedTriangle(
            run, prepared_run, triangle_offset, projected_z);
    }

    first_dense_index = sNdsNativeStageCorners[first_corner];
    binding_index = sNdsNativeStageVertices[first_dense_index].matrix_binding;
    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];
        u32 vertex_shift = dense->packed_cache_shift >>
            NDS_NATIVE_STAGE_COORDINATE_SHIFT;

        if (dense->matrix_binding != binding_index)
        {
            one_binding = FALSE;
        }
        if (vertex_shift > coordinate_shift)
        {
            coordinate_shift = vertex_shift;
        }
    }
    if (one_binding != FALSE)
    {
        ndsRendererNativeStageLoadNoZMatrix(
            binding_index, coordinate_shift, projected_z);
    }
    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];
        const NDSNativeStagePreparedDense *prepared =
            &sNdsNativeStagePreparedDense[dense_index];
        u32 vertex_shift = (one_binding != FALSE) ?
            coordinate_shift :
            (dense->packed_cache_shift >>
             NDS_NATIVE_STAGE_COORDINATE_SHIFT);

        if (one_binding == FALSE)
        {
            ndsRendererNativeStageLoadNoZMatrix(
                dense->matrix_binding, vertex_shift, projected_z);
        }
        ndsRendererNativeStageEmitNoZVertex(
            dense, prepared, prepared_run, vertex_shift);
    }
    return 1u;
}

s32 ndsRendererPrepareNativeStageOwner(
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats)
{
    NDSRendererTraversalState *state =
        &sNdsNativeStageOwnerExecution.traversal;
    NDSNativeStageTopologySummary topology;
    u64 epoch_mask = 0u;
    u32 segment_index;
    s32 accepted = FALSE;
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
    u32 task36_reject_reason = 1u;

    gNdsRendererTask36RendererRejectReason = 0u;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 phase0_preflight_start;
#endif

#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3PreflightAttemptCount++;
    gNdsRendererM3SegmentCount = 0u;
    gNdsRendererM3SegmentMask = 0u;
    gNdsRendererM3DObjCount = 0u;
    gNdsRendererM3BindingCount = 0u;
    gNdsRendererM3RunCount = 0u;
    gNdsRendererM3TriangleCount = 0u;
    gNdsRendererM3ResidentEpochCount = 0u;
    gNdsRendererM3MaterialShadowCount = 0u;
    gNdsRendererM3MaterialCommitCount = 0u;
    gNdsRendererM3CrossRunCount = 0u;
    gNdsRendererM3CrossTriangleCount = 0u;
    gNdsRendererM3CrossForeignCornerCount = 0u;
#if NDS_TASK36_HW_COMPOSE
    gNdsRendererTask36HardwareComposedDObjCount = 0u;
    gNdsRendererTask36CameraLoadCount = 0u;
    gNdsRendererTask36WorldMultCount = 0u;
#endif
#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
    gNdsRendererM3GeneratedSegment0AttemptCount = 0u;
    gNdsRendererM3GeneratedSegment0SuccessCount = 0u;
    gNdsRendererM3GeneratedSegment0PreGxFallbackCount = 0u;
    gNdsRendererM3GeneratedSegment0RunCount = 0u;
    gNdsRendererM3GeneratedSegment0TriangleCount = 0u;
    gNdsRendererM3GeneratedSegment0EpochCount = 0u;
    gNdsRendererM3GeneratedSegment0MaterialCount = 0u;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererM3GeneratedSegment0ShadowDenseCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowStateEntryCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowSyncCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowFieldComparisonCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowMismatchCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowFaultInjectedCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowFaultRejectedCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowLiveFaultInjectedCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowLiveFaultRejectedCount = 0u;
    gNdsRendererM3GeneratedSegment0ShadowLiveFaultRevalidatedCount = 0u;
#endif
#endif
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0Reset();
    phase0_preflight_start = ndsRendererM3Phase0Tick();
#endif
    sNdsNativeStageOwnerExecution.active = FALSE;
    sNdsNativeStageOwnerExecution.binding_composed = NULL;
#if NDS_TASK36_HW_COMPOSE
    sNdsNativeStageOwnerExecution.projection = NULL;
    sNdsNativeStageOwnerExecution.camera_modelview = NULL;
    sNdsNativeStageOwnerExecution.binding_world = NULL;
    sNdsNativeStageOwnerExecution.rigid_binding_mask = 0u;
    sNdsNativeStageOwnerExecution.task36_seen_binding_mask = 0u;
    sNdsNativeStageOwnerExecution.task36_local_pushed = FALSE;
    sNdsNativeStageOwnerExecution.task36_segment_active = FALSE;
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    memset(sNdsRendererBenchmarkSegment0AssetBases, 0,
           sizeof(sNdsRendererBenchmarkSegment0AssetBases));
#endif
    if ((gNdsRendererFastRunMode !=
         NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE) ||
        (frame == NULL) || (stats == NULL) ||
        (frame->dobjs == NULL) ||
        (frame->binding_display_lists == NULL) ||
        (frame->projection == NULL) ||
#if NDS_TASK36_HW_COMPOSE
        (frame->camera_modelview == NULL) ||
        (frame->binding_world == NULL) ||
#endif
        (frame->binding_composed == NULL) ||
        (frame->materials == NULL) || (frame->config == NULL) ||
        (NDS_NATIVE_STAGE_PRODUCTION_PACKET_ABI != 0x4d335031u) ||
        (ndsRendererNativeStageValidateTopology(frame, &topology) == FALSE))
    {
        goto done;
    }
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
    task36_reject_reason = 2u;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3MeasureResidualKey(frame);
#endif
#if NDS_TASK36_HW_COMPOSE == 2
    ndsRendererTask36ReplayBeginFrame(frame);
#endif

    for (segment_index = 0u;
         segment_index < NDS_NATIVE_STAGE_SEGMENT_COUNT;
         segment_index++)
    {
        const NDSNativeStageSegment *segment =
            &sNdsNativeStageSegments[segment_index];
        u32 binding_offset;

        ndsRendererInitStats(&sNdsNativeStageOwnerExecution.preflight_stats);
        sNdsNativeStageOwnerExecution.preflight_stats.geometry_mode =
            segment->initial_geometry;
        ndsRendererInitTraversalState(
            state, frame->config,
            &sNdsNativeStageOwnerExecution.preflight_stats,
            NULL, NULL, 0u);
#if NDS_TASK36_HW_COMPOSE == 2
        if (ndsRendererTask36ReplayUsePreparedSegment(
                segment_index,
                &sNdsNativeStageOwnerExecution.preflight_stats,
                &epoch_mask) != FALSE)
        {
            continue;
        }
#endif
#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE && \
    !NDS_RENDERER_M3_PHASE0_PROFILE
        if (segment_index == 0u)
        {
            if (ndsRendererNativeStagePrepareGeneratedSegment0(
                    frame,
                    &sNdsNativeStageOwnerExecution.preflight_stats,
                    state, &epoch_mask) == FALSE)
            {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
                task36_reject_reason = 100u;
#endif
                goto done;
            }
#if NDS_TASK36_HW_COMPOSE == 2
            ndsRendererTask36ReplayCapturePreparedSegment(
                segment_index,
                &sNdsNativeStageOwnerExecution.preflight_stats,
                epoch_mask);
#endif
            continue;
        }
#endif
        for (binding_offset = 0u;
             binding_offset < segment->binding_count;
             binding_offset++)
        {
            u32 binding_index = (u32)segment->first_binding + binding_offset;
            const NDSNativeStageBinding *binding =
                &sNdsNativeStageBindings[binding_index];
            u32 run_offset;

            for (run_offset = 0u;
                 run_offset < binding->run_count;
                 run_offset++)
            {
                u32 run_index = (u32)binding->first_run + run_offset;

                if (ndsRendererNativeStageApplyStateSpan(
                         &sNdsNativeStageStateSpans[run_index], frame,
                         &sNdsNativeStageOwnerExecution.preflight_stats,
                         state) == FALSE)
                {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
                    task36_reject_reason = 200u + run_index;
#endif
                    goto done;
                }
#if NDS_RENDERER_M3_PHASE0_PROFILE
                {
                    u32 residual_prepare_ticks_start =
                        gNdsRendererM3Phase0PrepareRunTicks;
                    u32 prepare_run_start = ndsRendererM3Phase0Tick();
                    s32 prepare_run_result = ndsRendererNativeStagePrepareRun(
                        run_index, frame,
                        &sNdsNativeStageOwnerExecution.preflight_stats,
                        state, &epoch_mask);

                    ndsRendererM3Phase0FinishSpan(
                        &gNdsRendererM3Phase0PrepareRunTicks,
                        prepare_run_start);
                    if (run_index >=
                        NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT)
                    {
                        gNdsRendererM3ResidualPrepareTicks +=
                            gNdsRendererM3Phase0PrepareRunTicks -
                            residual_prepare_ticks_start;
                        gNdsRendererM3ResidualRunCount++;
                    }
                    if (prepare_run_result == FALSE)
                    {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
                        task36_reject_reason = 300u + run_index;
#endif
                        goto done;
                    }
                }
#else
                if (ndsRendererNativeStagePrepareRun(
                        run_index, frame,
                        &sNdsNativeStageOwnerExecution.preflight_stats,
                        state, &epoch_mask) == FALSE)
                {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
                    task36_reject_reason = 300u + run_index;
#endif
                    goto done;
                }
#endif
            }
            if (ndsRendererNativeStageApplyStateSpan(
                    &sNdsNativeStageStateSpans[
                        NDS_NATIVE_STAGE_RUN_COUNT + binding_index],
                    frame, &sNdsNativeStageOwnerExecution.preflight_stats,
                    state) == FALSE)
            {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
                task36_reject_reason = 400u + binding_index;
#endif
                goto done;
            }
        }
#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE && \
    NDS_RENDERER_M3_PHASE0_PROFILE
        if (segment_index == 0u)
        {
            u32 current_hash_a;
            u32 current_hash_b;
            u32 current_field_count;
            u32 generated_hash_a;
            u32 generated_hash_b;
            u32 generated_field_count;
            u64 generated_epoch_mask = 0u;
            NDSRendererNativeStageFrame live_fault_frame;
            NDSNativeStageTopologySummary live_fault_topology;

            ndsRendererNativeStageHashGeneratedSegment0Outputs(
                &sNdsNativeStageOwnerExecution.preflight_stats, state,
                epoch_mask, &current_hash_a, &current_hash_b,
                &current_field_count);
            gNdsRendererM3GeneratedSegment0ShadowFaultInjectedCount++;
            if (ndsRendererNativeStageValidateGeneratedSegment0(TRUE) ==
                FALSE)
            {
                gNdsRendererM3GeneratedSegment0ShadowFaultRejectedCount++;
            }
            else
            {
                gNdsRendererM3GeneratedSegment0ShadowMismatchCount++;
                goto done;
            }
            live_fault_frame = *frame;
            live_fault_frame.asset_bases[1] =
                (const void *)((const u8 *)frame->asset_bases[1] + 8u);
            gNdsRendererM3GeneratedSegment0ShadowLiveFaultInjectedCount++;
            if (ndsRendererNativeStageValidateTopologyFull(
                    &live_fault_frame, &live_fault_topology) == FALSE)
            {
                gNdsRendererM3GeneratedSegment0ShadowLiveFaultRejectedCount++;
            }
            else
            {
                gNdsRendererM3GeneratedSegment0ShadowMismatchCount++;
                goto done;
            }
            if ((ndsRendererNativeStageValidateTopologyFull(
                     frame, &live_fault_topology) == FALSE) ||
                (memcmp(&live_fault_topology, &topology,
                        sizeof(live_fault_topology)) != 0))
            {
                gNdsRendererM3GeneratedSegment0ShadowMismatchCount++;
                goto done;
            }
            gNdsRendererM3GeneratedSegment0ShadowLiveFaultRevalidatedCount++;

            ndsRendererInitStats(
                &sNdsNativeStageOwnerExecution.preflight_stats);
            sNdsNativeStageOwnerExecution.preflight_stats.geometry_mode =
                segment->initial_geometry;
            ndsRendererInitTraversalState(
                state, frame->config,
                &sNdsNativeStageOwnerExecution.preflight_stats,
                NULL, NULL, 0u);
            if (ndsRendererNativeStagePrepareGeneratedSegment0(
                    frame,
                    &sNdsNativeStageOwnerExecution.preflight_stats,
                    state, &generated_epoch_mask) == FALSE)
            {
                gNdsRendererM3GeneratedSegment0ShadowMismatchCount++;
                goto done;
            }
            ndsRendererNativeStageHashGeneratedSegment0Outputs(
                &sNdsNativeStageOwnerExecution.preflight_stats, state,
                generated_epoch_mask, &generated_hash_a,
                &generated_hash_b, &generated_field_count);
            gNdsRendererM3GeneratedSegment0ShadowDenseCount =
                NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_COUNT;
            gNdsRendererM3GeneratedSegment0ShadowStateEntryCount =
                sNdsNativeStageSegment0ColdCertificate.state_count;
            gNdsRendererM3GeneratedSegment0ShadowSyncCount =
                sNdsNativeStageSegment0ColdCertificate.sync_count;
            gNdsRendererM3GeneratedSegment0ShadowFieldComparisonCount =
                generated_field_count;
            if ((current_hash_a != generated_hash_a) ||
                (current_hash_b != generated_hash_b) ||
                (current_field_count != generated_field_count) ||
                (generated_epoch_mask != epoch_mask))
            {
                gNdsRendererM3GeneratedSegment0ShadowMismatchCount++;
                goto done;
            }
            epoch_mask = generated_epoch_mask;
        }
#endif
#if NDS_TASK36_HW_COMPOSE == 2
        ndsRendererTask36ReplayCapturePreparedSegment(
            segment_index,
            &sNdsNativeStageOwnerExecution.preflight_stats,
            epoch_mask);
#endif
    }
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
    task36_reject_reason = 3u;
#endif
    if ((epoch_mask != (((u64)1u <<
                         NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT) - 1u)) ||
        (topology.raw_triangles != 66u) ||
        (topology.projected_no_z_triangles != 126u) ||
        (topology.projected_range_triangles != 10u) ||
        (topology.cross_runs != 5u) ||
        (topology.cross_triangles != 10u) ||
        (topology.cross_foreign_corners != 15u))
    {
        goto done;
    }
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
    task36_reject_reason = 4u;
#endif

    sNdsNativeStageOwnerExecution.binding_composed = frame->binding_composed;
#if NDS_TASK36_HW_COMPOSE
    sNdsNativeStageOwnerExecution.projection = frame->projection;
    sNdsNativeStageOwnerExecution.camera_modelview = frame->camera_modelview;
    sNdsNativeStageOwnerExecution.binding_world = frame->binding_world;
    sNdsNativeStageOwnerExecution.rigid_binding_mask =
        frame->rigid_binding_mask;
    if ((frame->rigid_binding_mask & ((u64)1u << 29u)) == 0u)
    {
        sNdsNativeStageOwnerExecution.raw_composed =
            frame->binding_composed[29u];
        if (ndsRendererBuildShiftedRawHardwareMatrix(
                &sNdsNativeStageOwnerExecution.raw_composed,
                &sNdsNativeStageOwnerExecution.scaled_raw_modelview,
                1u) == FALSE)
        {
            goto done;
        }
    }
#else
    sNdsNativeStageOwnerExecution.raw_composed =
        frame->binding_composed[29u];
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    memcpy(sNdsRendererBenchmarkSegment0AssetBases, frame->asset_bases,
           sizeof(sNdsRendererBenchmarkSegment0AssetBases));
#endif
#if !NDS_TASK36_HW_COMPOSE
    if (ndsRendererBuildShiftedRawHardwareMatrix(
            &sNdsNativeStageOwnerExecution.raw_composed,
            &sNdsNativeStageOwnerExecution.scaled_raw_modelview,
            1u) == FALSE)
    {
        goto done;
    }
#endif
    ndsRendererInitStats(stats);
    stats->command_count = NDS_NATIVE_STAGE_SOURCE_COMMAND_COUNT;
    stats->vertex_count = NDS_NATIVE_STAGE_SOURCE_VERTEX_COUNT;
    stats->vertex_command_count = NDS_NATIVE_STAGE_VERTEX_COMMAND_COUNT;
    stats->triangle_command_count = NDS_NATIVE_STAGE_TRIANGLE_COMMAND_COUNT;
    stats->sync_command_count =
        sNdsNativeStageOwnerExecution.preflight_stats.sync_command_count;
    sNdsNativeStageOwnerExecution.stats = stats;
    sNdsNativeStageOwnerExecution.next_segment = 0u;
    sNdsNativeStageOwnerExecution.active = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3PreflightSuccessCount++;
    gNdsRendererM3ResidentEpochCount =
        NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT;
    gNdsRendererM3CrossRunCount = topology.cross_runs;
    gNdsRendererM3CrossTriangleCount = topology.cross_triangles;
    gNdsRendererM3CrossForeignCornerCount =
        topology.cross_foreign_corners;
#endif
    accepted = TRUE;
#if NDS_TASK36_HW_COMPOSE == 2
    ndsRendererTask36ReplayStartCapture(frame);
#endif

done:
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0FinishSpan(
        &gNdsRendererM3Phase0PreflightTicks, phase0_preflight_start);
#endif
    if (accepted == FALSE)
    {
#if NDS_TASK36_HW_COMPOSE == 2
        if (sNdsRendererTask36ReplayOwner.frame_capture != FALSE)
        {
            sNdsRendererTask36ReplayOwner.capture_fault = TRUE;
            ndsRendererTask36ReplayFinishFrame();
        }
#endif
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        gNdsRendererTask36RendererRejectReason = task36_reject_reason;
#endif
        sNdsNativeStageOwnerExecution.stats = NULL;
        sNdsNativeStageOwnerExecution.binding_composed = NULL;
#if NDS_TASK36_HW_COMPOSE
        sNdsNativeStageOwnerExecution.projection = NULL;
        sNdsNativeStageOwnerExecution.camera_modelview = NULL;
        sNdsNativeStageOwnerExecution.binding_world = NULL;
        sNdsNativeStageOwnerExecution.rigid_binding_mask = 0u;
#endif
        sNdsNativeStageOwnerExecution.next_segment = 0u;
        sNdsNativeStageOwnerExecution.active = FALSE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3PreflightFallbackCount++;
#endif
    }
    return accepted;
}

void ndsRendererResetNativeStageValidationCache(void)
{
    memset(&sNdsNativeStageValidationCache, 0,
           sizeof(sNdsNativeStageValidationCache));
#if NDS_TASK36_HW_COMPOSE == 2
    ndsRendererTask36ReplayReset();
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    sNdsRendererM3ResidualKeyValid = FALSE;
#endif
}

#if NDS_RENDER_ECONOMY
static u32 ndsRendererEconomySkipNativeStageSegment(
    const NDSNativeStageSegment *segment,
    NDSRendererStats *stats)
{
    u32 segment_triangles = 0u;
    u32 run_offset;

    ndsRendererHardwareEndBatch();
    for (run_offset = 0u; run_offset < segment->run_count; run_offset++)
    {
        const NDSNativeStageRun *run = &sNdsNativeStageRuns[
            (u32)segment->first_run + run_offset];
        u32 triangle_offset;

        for (triangle_offset = 0u;
             triangle_offset < run->triangle_count;
             triangle_offset++)
        {
            if (run->submit_class ==
                NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                (void)ndsRendererHardwareNextProjectedDepth();
            }
            else
            {
                ndsRendererHardwareEnterProjectedForeground();
            }
        }
        stats->triangle_count += run->triangle_count;
        segment_triangles += run->triangle_count;
    }
    gNdsRendererEconomyAppliedOwnerMask |= (u32)1u << segment->owner;
    gNdsRendererEconomySkippedRunCount += segment->run_count;
    gNdsRendererEconomySkippedTriangleCount += segment_triangles;
    return segment_triangles;
}
#endif

s32 ndsRendererCommitNativeStageSegment(u32 segment_index)
{
    NDSRendererStats *stats = sNdsNativeStageOwnerExecution.stats;
    const NDSNativeStageSegment *segment;
    u32 run_offset;
    u32 segment_triangles = 0u;
#if NDS_TASK36_HW_COMPOSE == 2
    u32 task36_capture_segment = FALSE;
    u32 task36_replay_segment = FALSE;
#endif
#if NDS_RENDERER_SCREEN_SPACE_CENSUS
    u32 census_owner_start = 0u;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 commit_start;
#endif

    if (sNdsNativeStageOwnerExecution.active == FALSE)
    {
        return FALSE;
    }
    if ((segment_index >= NDS_NATIVE_STAGE_SEGMENT_COUNT) ||
        (segment_index != sNdsNativeStageOwnerExecution.next_segment) ||
        (stats == NULL))
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3PostArmFailureCount++;
#endif
        return TRUE;
    }
    segment = &sNdsNativeStageSegments[segment_index];
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    if (segment_index == 0u)
    {
        ndsRendererBenchmarkSegment0SinkBegin();
    }
#endif
    sNdsRendererRuntimeOwner = NDS_RENDERER_PROFILE_OWNER_STAGE;
#if NDS_RENDERER_SCREEN_SPACE_CENSUS
    ndsRendererScreenSpaceCensusStageSegment(segment);
    if (gNdsRendererScreenSpaceCensusArmed != 0u)
    {
        census_owner_start = cpuGetTiming();
    }
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    commit_start = ndsRendererM3Phase0Tick();
#endif
#if NDS_TASK34_STAGE_STREAM_CENSUS
    ndsRendererTask34StageStreamBeginSegment(segment_index);
#endif
#if NDS_TASK36_HW_COMPOSE == 2
    task36_capture_segment =
        (sNdsRendererTask36ReplayOwner.frame_capture != FALSE) &&
        (ndsRendererTask36ReplaySegmentEligible(segment_index) != FALSE);
    task36_replay_segment =
        (sNdsRendererTask36ReplayOwner.frame_replay != FALSE) &&
        (ndsRendererTask36ReplaySegmentEligible(segment_index) != FALSE);
    if ((task36_capture_segment || task36_replay_segment) &&
        (ndsRendererNativeStageTask36BeginSegment() == FALSE))
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3PostArmFailureCount++;
#endif
        return TRUE;
    }
#endif
#if NDS_RENDER_ECONOMY
    if ((gNdsRendererEconomyActiveOwnerMask &
         ((u32)1u << segment->owner)) != 0u)
    {
        segment_triangles = ndsRendererEconomySkipNativeStageSegment(
            segment, stats);
    }
    else
#endif
    for (run_offset = 0u; run_offset < segment->run_count; run_offset++)
    {
        u32 run_index = (u32)segment->first_run + run_offset;
        const NDSNativeStageRun *run = &sNdsNativeStageRuns[run_index];
        const NDSNativeStagePreparedRun *prepared_run =
            &sNdsNativeStageOwnerExecution.runs[run_index];
        u32 emitted_triangles = 0u;
        u32 triangle_offset;
#if NDS_TASK34_STAGE_STREAM_CENSUS
        u32 task34_dobj_index;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
        u32 phase_start = ndsRendererM3Phase0Tick();
#endif

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
        if (segment_index == 0u)
        {
            ndsRendererBenchmarkSegment0ArmRun(
                run_offset, run, prepared_run);
        }
#endif
#if NDS_TASK34_STAGE_STREAM_CENSUS
        for (task34_dobj_index = 0u;
             task34_dobj_index < NDS_NATIVE_STAGE_DOBJ_COUNT;
             task34_dobj_index++)
        {
            if (sNdsNativeStageDObjs[task34_dobj_index].binding_index ==
                run->binding_index)
            {
                break;
            }
        }
        ndsRendererTask34StageStreamSetDObj(
            (task34_dobj_index < NDS_NATIVE_STAGE_DOBJ_COUNT) ?
                task34_dobj_index : NDS_TASK34_STAGE_STREAM_DOBJ_NONE);
#endif
#if NDS_TASK36_HW_COMPOSE == 2
        if (task36_replay_segment != FALSE)
        {
            if (ndsRendererTask36ReplayRun(
                    run_index, run, segment->owner, stats) == FALSE)
            {
#if NDS_RENDERER_PROFILE_LEVEL == 1
                gNdsRendererM3PostArmFailureCount++;
#endif
                return TRUE;
            }
#if NDS_RENDERER_M3_PHASE0_PROFILE
            ndsRendererM3Phase0FinishSpan(
                &gNdsRendererM3Phase0RunTransitionTicks, phase_start);
            phase_start = ndsRendererM3Phase0Tick();
#endif
            for (triangle_offset = 0u;
                 triangle_offset < run->triangle_count;
                 triangle_offset++)
            {
                if (run->submit_class ==
                    NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
                {
                    (void)ndsRendererHardwareNextProjectedDepth();
                }
                else
                {
                    ndsRendererHardwareEnterProjectedForeground();
                }
            }
            emitted_triangles = run->triangle_count;
#if NDS_RENDERER_M3_PHASE0_PROFILE
            if (run->submit_class ==
                NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
            {
                ndsRendererM3Phase0FinishSpan(
                    &gNdsRendererM3Phase0RawEmitTicks, phase_start);
            }
            else if (run->submit_class ==
                     NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
            {
                ndsRendererM3Phase0FinishSpan(
                    &gNdsRendererM3Phase0RangeEmitTicks, phase_start);
            }
            else
            {
                ndsRendererM3Phase0FinishSpan(
                    &gNdsRendererM3Phase0NoZEmitTicks, phase_start);
            }
            phase_start = ndsRendererM3Phase0Tick();
#endif
            goto task36_account_run;
        }
        if (task36_capture_segment != FALSE)
        {
            ndsRendererTask36ReplayCaptureBeginRun(run_index);
        }
#endif
        ndsRendererNativeStageBeginRun(
            run, prepared_run, run->submit_class, segment->owner, stats,
            FALSE);
#if NDS_RENDERER_M3_PHASE0_PROFILE
        ndsRendererM3Phase0FinishSpan(
            &gNdsRendererM3Phase0RunTransitionTicks, phase_start);
        phase_start = ndsRendererM3Phase0Tick();
#endif
        for (triangle_offset = 0u;
             triangle_offset < run->triangle_count;
             triangle_offset++)
        {
            s16 no_z = (run->submit_class ==
                NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ?
                ndsRendererHardwareNextProjectedDepth() : 0;
            u32 corner_offset;

            if (run->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                emitted_triangles += ndsRendererNativeStageEmitNoZTriangle(
                    run, prepared_run, triangle_offset, no_z);
                continue;
            }

            for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
            {
                u32 dense_index = sNdsNativeStageCorners[
                    (u32)run->first_corner + triangle_offset * 3u +
                    corner_offset];
                const NDSNativeStageDenseVertex *dense =
                    &sNdsNativeStageVertices[dense_index];
                const NDSNativeStagePreparedDense *prepared =
                    &sNdsNativeStagePreparedDense[dense_index];
                ndsRendererNativeStageEmitVertex(
                    dense, prepared, prepared_run, run->submit_class);
            }
            emitted_triangles++;
            if (run->submit_class !=
                NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                ndsRendererHardwareEnterProjectedForeground();
            }
        }
#if NDS_RENDERER_M3_PHASE0_PROFILE
        if (run->submit_class ==
            NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
        {
            ndsRendererM3Phase0FinishSpan(
                &gNdsRendererM3Phase0RawEmitTicks, phase_start);
        }
        else if (run->submit_class ==
                 NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
        {
            ndsRendererM3Phase0FinishSpan(
                &gNdsRendererM3Phase0RangeEmitTicks, phase_start);
        }
        else
        {
            ndsRendererM3Phase0FinishSpan(
                &gNdsRendererM3Phase0NoZEmitTicks, phase_start);
        }
        phase_start = ndsRendererM3Phase0Tick();
#endif
        ndsRendererHardwareEndBatch();
#if NDS_TASK36_HW_COMPOSE == 2
        if (task36_capture_segment != FALSE)
        {
            ndsRendererTask36ReplayCaptureEndRun(run_index);
        }
task36_account_run:
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
        if (segment_index == 0u)
        {
            ndsRendererBenchmarkSegment0CheckpointRun(run_offset);
        }
#endif
        ndsRendererNativeStageAccountRun(
            stats,
            (run->submit_class ==
             NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
                NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX :
                run->submit_class,
            emitted_triangles);
        stats->triangle_count += run->triangle_count;
        segment_triangles += run->triangle_count;
#if NDS_RENDERER_M3_PHASE0_PROFILE
        ndsRendererM3Phase0FinishSpan(
            &gNdsRendererM3Phase0AccountingTicks, phase_start);
#endif
    }
#if NDS_TASK36_HW_COMPOSE
    ndsRendererNativeStageTask36EndSegment();
#endif
#if NDS_TASK36_HW_COMPOSE == 2
    if (task36_capture_segment != FALSE)
    {
        sNdsRendererTask36ReplayOwner.captured_segment_mask |=
            1u << segment_index;
    }
    if (task36_replay_segment != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask36ReplaySegmentCount++;
#endif
    }
#endif
#if NDS_TASK34_STAGE_STREAM_CENSUS
    ndsRendererTask34StageStreamEndSegment();
#endif
    sNdsRendererFastRunCount += segment->run_count;
    sNdsRendererFastTriangleCount += segment_triangles;
    sNdsRendererFastOwnerTriangleCount[
        NDS_RENDERER_PROFILE_OWNER_STAGE] += segment_triangles;
    sNdsNativeStageOwnerExecution.next_segment++;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3SegmentCount++;
    gNdsRendererM3SegmentMask |= (u32)1u << segment_index;
    gNdsRendererM3RunCount += segment->run_count;
    gNdsRendererM3TriangleCount += segment_triangles;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererM3Phase0FinishSpan(
        &gNdsRendererM3Phase0CommitTicks, commit_start);
#endif
#if NDS_RENDERER_SCREEN_SPACE_CENSUS
    if (census_owner_start != 0u)
    {
        gNdsRendererScreenSpaceStageOwnerTicks[segment->owner] +=
            cpuGetTiming() - census_owner_start;
    }
#endif
    sNdsRendererRuntimeOwner = NDS_RENDERER_PROFILE_OWNER_NONE;
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    if (segment_index == 0u)
    {
        ndsRendererBenchmarkSegment0SinkEnd();
        sNdsRendererBenchmarkSegment0TextureValid = FALSE;
    }
    ndsRendererBenchmarkSinkEndOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
    return TRUE;
}

void ndsRendererFinishNativeStageOwner(void)
{
#if NDS_TASK36_HW_COMPOSE == 2
    ndsRendererTask36ReplayFinishFrame();
#endif
    if (sNdsNativeStageOwnerExecution.active != FALSE)
    {
#if NDS_TASK36_HW_COMPOSE
        ndsRendererNativeStageTask36EndSegment();
#endif
        ndsRendererHardwareEndBatch();
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
        ndsRendererBenchmarkSinkEndOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
        if ((sNdsNativeStageOwnerExecution.next_segment !=
             NDS_NATIVE_STAGE_SEGMENT_COUNT) ||
            (sNdsNativeStageOwnerExecution.stats == NULL) ||
            (sNdsNativeStageOwnerExecution.stats->triangle_count !=
             NDS_NATIVE_STAGE_TRIANGLE_COUNT))
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererM3PostArmFailureCount++;
#endif
        }
    }
    sNdsNativeStageOwnerExecution.stats = NULL;
    sNdsNativeStageOwnerExecution.binding_composed = NULL;
#if NDS_TASK36_HW_COMPOSE
    sNdsNativeStageOwnerExecution.projection = NULL;
    sNdsNativeStageOwnerExecution.camera_modelview = NULL;
    sNdsNativeStageOwnerExecution.binding_world = NULL;
    sNdsNativeStageOwnerExecution.rigid_binding_mask = 0u;
#endif
    sNdsNativeStageOwnerExecution.next_segment = 0u;
    sNdsNativeStageOwnerExecution.active = FALSE;
    sNdsRendererRuntimeOwner = NDS_RENDERER_PROFILE_OWNER_NONE;
}
#else
s32 ndsRendererPrepareNativeStageOwner(
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats)
{
    (void)frame;
    (void)stats;
    return FALSE;
}

s32 ndsRendererCommitNativeStageSegment(u32 segment_index)
{
    (void)segment_index;
    return FALSE;
}

void ndsRendererFinishNativeStageOwner(void)
{
}

void ndsRendererResetNativeStageValidationCache(void)
{
}
#endif

s32 NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererExecuteNativeFighterOwnerProduction(
    u32 slot,
    const void *asset_base_ptr,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    u32 *out_hardware_started)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    const NDSNativeRoot *roots;
    const u8 *palette_slots;
    const u8 *binding_palette_slots = NULL;
    const u8 *asset_base = asset_base_ptr;
    NDSRendererTraversalState *state =
        &sNdsNativeFighterOwnerExecution.traversal;
    u32 root_count;
    u32 root_index;
    u32 native_run_count = 0u;
    u32 native_triangle_count = 0u;
    u32 raw_triangle_count = 0u;
    u32 raw_reuse_count = 0u;
    u32 cross_triangle_count = 0u;
    u32 cross_reuse_count = 0u;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    volatile NDSRendererOwnerProfile *m2_owner;
    u32 m2_total_start;
    u32 m2_lighting_before = 0u;
    u32 m2_root_gx_before = 0u;
    u32 m2_run_prepare_before = 0u;
    u32 m2_emit_account_before = 0u;
#endif

    (void)callback_user;
    if (out_hardware_started == NULL)
    {
        return FALSE;
    }
    *out_hardware_started = FALSE;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_owner = ndsRendererProfileM2Owner();
    if (m2_owner != NULL)
    {
        m2_lighting_before = m2_owner->m2_lighting_shading_ticks;
        m2_root_gx_before = m2_owner->m2_root_gx_ticks;
        m2_run_prepare_before = m2_owner->m2_run_prepare_ticks;
        m2_emit_account_before = m2_owner->m2_corner_emit_account_ticks;
    }
    m2_total_start = cpuGetTiming();
#endif
    if ((stats == NULL) ||
        (stats->blocker != NDS_RENDERER_BLOCKER_NONE) ||
        (ndsRendererNativePreflightProductionOwner(
             slot, asset_base, inputs, input_count,
             callback, stats) == FALSE))
    {
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        ndsRendererProfileM2FinishProduction(
            m2_owner, m2_total_start,
            m2_lighting_before, m2_root_gx_before,
            m2_run_prepare_before, m2_emit_account_before, FALSE);
#endif
        return FALSE;
    }
    if (slot == 0u)
    {
        roots = sNdsNativeMarioRoots;
        root_count = sizeof(sNdsNativeMarioRoots) /
            sizeof(sNdsNativeMarioRoots[0]);
        palette_slots = sNdsNativeMarioCrossPaletteSlots;
    }
    else
    {
        roots = sNdsNativeFoxRoots;
        root_count = sizeof(sNdsNativeFoxRoots) /
            sizeof(sNdsNativeFoxRoots[0]);
        palette_slots = sNdsNativeFoxCrossPaletteSlots;
    }

    ndsRendererInitTraversalState(
        state, NULL, stats, NULL, NULL, 0u);
    for (root_index = 0u; root_index < root_count; root_index++)
    {
        const NDSNativeRoot *root = &roots[root_index];
        const NDSRendererNativeFighterRoot *input = &inputs[root_index];
        u32 palette_slot = palette_slots[root_index];
        u32 epoch_offset;

        ndsRendererNativeBindProductionRoot(state, input, stats);
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        {
            u32 m2_root_gx_start = cpuGetTiming();
#endif
        *out_hardware_started = TRUE;
        ndsRendererLoadHardwareRawComposedMatrix(
            &state->matrix, state->matrix_generation);
        if (palette_slot <= NDS_NATIVE_GX_MATRIX_SLOT_MAX)
        {
            glStoreMatrix((int)palette_slot);
        }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            if (m2_owner != NULL)
            {
                m2_owner->m2_root_gx_ticks +=
                    cpuGetTiming() - m2_root_gx_start;
                m2_owner->m2_root_gx_count++;
            }
        }
#endif
#endif
        if (stats->first_opcode == 0u)
        {
            stats->first_opcode = NDS_RENDERER_OP_RDPPIPESYNC;
        }
        stats->command_count += root->source_command_count;
        ndsRendererNativeApplyRootLightPreamble(root, stats);

        for (epoch_offset = 0u;
             epoch_offset < root->epoch_count;
             epoch_offset++)
        {
            u32 epoch_index = root->first_epoch + epoch_offset;
            const NDSNativeEpoch *epoch =
                &sNdsNativeFighterEpochs[epoch_index];
            u32 run_offset;

            ndsRendererNativeApplyStateSpan(
                epoch->before_state_first, epoch->before_state_count,
                epoch->before_sync_count,
                asset_base, stats, state);
            if (epoch->material_slot != NDS_NATIVE_MATERIAL_NONE)
            {
                ndsRendererNativeApplyMaterial(
                    &input->materials[epoch->material_slot], stats, state);
            }
            ndsRendererNativeApplyStateSpan(
                epoch->after_state_first, epoch->after_state_count,
                epoch->after_sync_count,
                asset_base, stats, state);
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            {
                u32 m2_lighting_start = cpuGetTiming();
#endif
            (void)ndsRendererNativeShadeProductionActions(
                epoch,
                sNdsNativeFighterEpochDirectPolicy[epoch_index],
                FALSE,
                stats, state);
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                if (m2_owner != NULL)
                {
                    m2_owner->m2_lighting_shading_ticks +=
                        cpuGetTiming() - m2_lighting_start;
                    m2_owner->m2_lighting_epoch_count++;
                }
            }
#endif
#if NDS_RENDERER_M2_DETAILED_LEDGER
            ndsRendererM2ShadeCensusEpoch(
                slot, inputs[0].owner_generation, epoch_index, epoch,
                (state->prepared_light_direction_valid != 0u) ?
                    &state->prepared_light_direction : NULL,
                state->prepared_light_direction_valid, stats);
            ndsRendererM2ShadeRecordProduced(
                slot, inputs[0].owner_generation, epoch_index, epoch,
                sNdsNativeFighterEpochDirectPolicy[epoch_index], stats);
#endif

            for (run_offset = 0u;
                 run_offset < epoch->run_count;
                 run_offset++)
            {
                const NDSNativeRun *run =
                    &sNdsNativeFighterRuns[epoch->first_run + run_offset];

#if NDS_RENDERER_SCREEN_SPACE_CENSUS
                ndsRendererScreenSpaceCensusFighterRun(
                    run, inputs, input_count, root_index);
#endif
                if (ndsRendererNativeSubmitProductionRun(
                        run,
                        sNdsNativeFighterEpochDirectPolicy[epoch_index],
                        palette_slot, binding_palette_slots,
                        input->config,
                        stats, state,
                        &raw_triangle_count, &raw_reuse_count,
                        &cross_triangle_count, &cross_reuse_count) == FALSE)
                {
                    if (raw_triangle_count != 0u)
                    {
                        ndsRendererFastAccountRawTriangles(
                            stats, raw_triangle_count, raw_reuse_count);
                    }
                    if (cross_triangle_count != 0u)
                    {
                        ndsRendererNativeAccountGXCrossTriangles(
                            stats, cross_triangle_count, cross_reuse_count);
                    }
                    ndsRendererHardwareEndBatch();
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                    ndsRendererProfileM2FinishProduction(
                        m2_owner, m2_total_start,
                        m2_lighting_before, m2_root_gx_before,
                        m2_run_prepare_before, m2_emit_account_before,
                        FALSE);
#endif
                    return FALSE;
                }
                native_run_count++;
                native_triangle_count += run->triangle_count;
            }
        }
        ndsRendererNativeApplyStateSpan(
            root->tail_state_first, root->tail_state_count,
            root->tail_sync_count,
            asset_base, stats, state);
        stats->end_command_count++;
    }
    if (raw_triangle_count != 0u)
    {
        ndsRendererFastAccountRawTriangles(
            stats, raw_triangle_count, raw_reuse_count);
    }
    if (cross_triangle_count != 0u)
    {
        ndsRendererNativeAccountGXCrossTriangles(
            stats, cross_triangle_count, cross_reuse_count);
    }
    ndsRendererHardwareEndBatch();
    sNdsRendererFastRunCount += native_run_count;
    sNdsRendererFastTriangleCount += native_triangle_count;
    if ((u32)sNdsRendererRuntimeOwner <
        (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererFastOwnerTriangleCount[
            (u32)sNdsRendererRuntimeOwner] += native_triangle_count;
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    ndsRendererProfileM2FinishProduction(
        m2_owner, m2_total_start,
        m2_lighting_before, m2_root_gx_before,
        m2_run_prepare_before, m2_emit_account_before, TRUE);
#endif
    return TRUE;
#else
    (void)slot;
    (void)asset_base_ptr;
    (void)inputs;
    (void)input_count;
    (void)callback;
    (void)callback_user;
    (void)stats;
    if (out_hardware_started != NULL)
    {
        *out_hardware_started = FALSE;
    }
    return FALSE;
#endif
}

#if !NDS_RENDERER_HW_TRIANGLES
static void ndsRendererTextureSourceHashCommand(
    NDSRendererStats *stats, u32 w0, u32 w1)
{
    (void)stats;
    (void)w0;
    (void)w1;
}
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static void ndsRendererClearNativeFighterOwner(void)
{
    ndsRendererHardwareEndBatch();
    sNdsNativeFighterOwnerExecution.stats = NULL;
    sNdsNativeFighterOwnerExecution.vertex_cache = NULL;
    sNdsNativeFighterOwnerExecution.slot = 0u;
    sNdsNativeFighterOwnerExecution.active = FALSE;
}
#endif

s32 ndsRendererBeginNativeFighterOwner(
    u32 slot,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    NDSRendererTraversalState *state;

    if ((slot > 1u) || (stats == NULL) || (vertex_cache == NULL) ||
        (sNdsNativeFighterOwnerExecution.active != 0u))
    {
        return FALSE;
    }
    state = &sNdsNativeFighterOwnerExecution.traversal;
    ndsRendererInitTraversalState(
        state, NULL, stats, NULL,
        vertex_cache->matrix_snapshots,
        vertex_cache->matrix_snapshot_count);
    state->vertices = vertex_cache->transformed_vertices;
    state->vertex_valid_mask = vertex_cache->transformed_valid_mask;
    state->input_vertices = vertex_cache->input_vertices;
    state->input_vertex_valid_mask = vertex_cache->input_valid_mask;
    state->raw_vertex_fit_mask = vertex_cache->raw_vertex_fit_mask;
    state->vertex_colors = vertex_cache->vertex_colors;
    state->vertex_color_valid_mask =
        vertex_cache->vertex_color_valid_mask;
    state->vertex_matrix_snapshot = vertex_cache->vertex_matrix_snapshot;
    state->vertex_clip_snapshot = vertex_cache->vertex_clip_snapshot;
    sNdsNativeFighterOwnerExecution.stats = stats;
    sNdsNativeFighterOwnerExecution.vertex_cache = vertex_cache;
    sNdsNativeFighterOwnerExecution.slot = slot;
    sNdsNativeFighterOwnerExecution.active = TRUE;
    return TRUE;
#else
    (void)slot;
    (void)stats;
    (void)vertex_cache;
    return FALSE;
#endif
}

s32 ndsRendererEndNativeFighterOwner(
    u32 slot,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    NDSRendererTraversalState *state;
    NDSRendererVertexCache *owner_vertex_cache;
    s32 identity_matches;

    if (sNdsNativeFighterOwnerExecution.active == 0u)
    {
        ndsRendererClearNativeFighterOwner();
        return FALSE;
    }
    identity_matches =
        ((sNdsNativeFighterOwnerExecution.slot == slot) &&
         (sNdsNativeFighterOwnerExecution.stats == stats) &&
         (sNdsNativeFighterOwnerExecution.vertex_cache == vertex_cache)) ?
            TRUE : FALSE;
    owner_vertex_cache =
        sNdsNativeFighterOwnerExecution.vertex_cache;
    if ((identity_matches == FALSE) || (owner_vertex_cache == NULL))
    {
        /* An identity mismatch is a caller integrity failure, but it must not
         * strand the singleton owner and poison the next fighter draw. */
        ndsRendererClearNativeFighterOwner();
        return FALSE;
    }
    state = &sNdsNativeFighterOwnerExecution.traversal;
    owner_vertex_cache->transformed_valid_mask = state->vertex_valid_mask;
    owner_vertex_cache->input_valid_mask = state->input_vertex_valid_mask;
    owner_vertex_cache->raw_vertex_fit_mask = state->raw_vertex_fit_mask;
    owner_vertex_cache->vertex_color_valid_mask =
        state->vertex_color_valid_mask;
    owner_vertex_cache->matrix_snapshot_count =
        state->matrix_snapshot_count;
    ndsRendererClearNativeFighterOwner();
    return TRUE;
#else
    (void)slot;
    (void)stats;
    (void)vertex_cache;
    return FALSE;
#endif
}

void ndsRendererAbortNativeFighterOwner(void)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    ndsRendererClearNativeFighterOwner();
#endif
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static s32 ndsRendererNativeArraySpanFits(
    u32 first, u32 count, u32 total)
{
    return ((first <= total) && (count <= (total - first))) ?
        TRUE : FALSE;
}

static s32 ndsRendererNativeAssetSpanFits(
    u32 offset, u32 count, u32 element_size, u32 asset_data_size)
{
    if ((element_size == 0u) || (offset > asset_data_size))
    {
        return FALSE;
    }
    return (count <= ((asset_data_size - offset) / element_size)) ?
        TRUE : FALSE;
}

static s32 ndsRendererValidateNativeStateSpan(
    u16 first, u32 count, u32 asset_data_size)
{
    u32 sequence_count = sizeof(sNdsNativeFighterStateSequence) /
        sizeof(sNdsNativeFighterStateSequence[0]);
    u32 delta_count = sizeof(sNdsNativeFighterStateDeltas) /
        sizeof(sNdsNativeFighterStateDeltas[0]);
    u32 i;

    if (count == 0u)
    {
        return TRUE;
    }
    if ((first == NDS_NATIVE_STATE_NONE) ||
        (ndsRendererNativeArraySpanFits(
             first, count, sequence_count) == FALSE))
    {
        return FALSE;
    }
    for (i = 0u; i < count; i++)
    {
        u32 delta_index = sNdsNativeFighterStateSequence[first + i];
        const NDSNativeStateDelta *delta;

        if (delta_index >= delta_count)
        {
            return FALSE;
        }
        delta = &sNdsNativeFighterStateDeltas[delta_index];
        switch (delta->effect)
        {
        case NDS_NATIVE_STATE_OTHERMODE:
        case NDS_NATIVE_STATE_COMBINE:
        case NDS_NATIVE_STATE_TEXTURE:
        case NDS_NATIVE_STATE_GEOMETRY:
        case NDS_NATIVE_STATE_TILE:
        case NDS_NATIVE_STATE_LOAD_TLUT:
        case NDS_NATIVE_STATE_LOAD_BLOCK:
        case NDS_NATIVE_STATE_TILE_SIZE:
        case NDS_NATIVE_STATE_PRIM:
            break;
        case NDS_NATIVE_STATE_IMAGE:
            if (delta->w1 >= asset_data_size)
            {
                return FALSE;
            }
            break;
        case NDS_NATIVE_STATE_LIGHT_COLOR:
        {
            u32 index =
                (delta->w0 >> NDS_RENDERER_MOVEWORD_INDEX_SHIFT) &
                NDS_RENDERER_MOVEWORD_INDEX_MASK;
            u32 offset = delta->w0 & NDS_RENDERER_MOVEWORD_OFFSET_MASK;

            if (((delta->w0 >> 24) != NDS_RENDERER_OP_MOVEWORD) ||
                (index != NDS_RENDERER_MOVEWORD_LIGHTCOL) ||
                ((offset != NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_A) &&
                 (offset != NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_B) &&
                 (offset != NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_A) &&
                 (offset != NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_B)))
            {
                return FALSE;
            }
            break;
        }
        default:
            return FALSE;
        }
    }
    return TRUE;
}

static s32 ndsRendererValidateNativeVertexAction(
    u32 action_index,
    const NDSNativeRoot *root,
    u32 asset_data_size)
{
    u32 action_count = sizeof(sNdsNativeFighterVertexActions) /
        sizeof(sNdsNativeFighterVertexActions[0]);
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 dense_first_count =
        sizeof(sNdsNativeFighterActionDenseFirst) /
        sizeof(sNdsNativeFighterActionDenseFirst[0]);
#endif
    const NDSNativeVertexAction *action;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 dense_first;
    u32 i;
#endif

    if ((root == NULL) || (action_index >= action_count)
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ||
        (action_index >= dense_first_count))
#else
        )
#endif
    {
        return FALSE;
    }
    action = &sNdsNativeFighterVertexActions[action_index];
#if NDS_RENDERER_PROFILE_LEVEL < 2
    dense_first = sNdsNativeFighterActionDenseFirst[action_index];
#endif
    if (action->command_index >= root->source_command_count)
    {
        return FALSE;
    }
    if (action->kind == NDS_NATIVE_VERTEX_BLOCK)
    {
        if ((action->count == 0u) ||
            ((u32)action->index > NDS_RENDERER_MAX_VTX) ||
            ((u32)action->count >
             (NDS_RENDERER_MAX_VTX - (u32)action->index)) ||
            (ndsRendererNativeAssetSpanFits(
                 action->source_offset, action->count, 16u,
                 asset_data_size) == FALSE)
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ||
            (ndsRendererNativeArraySpanFits(
                 dense_first, action->count,
                 NDS_NATIVE_DENSE_VERTEX_COUNT) == FALSE))
#else
            )
#endif
        {
            return FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        for (i = 0u; i < action->count; i++)
        {
            const NDSNativeDenseVertex *dense =
                &sNdsNativeFighterDenseVertices[dense_first + i];

            if ((dense->cache_slot != ((u32)action->index + i)) ||
                (dense->matrix_binding >=
                 NDS_NATIVE_ROOT_BINDING_COUNT))
            {
                return FALSE;
            }
        }
#endif
        return TRUE;
    }
    if (action->kind == NDS_NATIVE_MODIFY_ST)
    {
        if ((action->index >= NDS_RENDERER_MAX_VTX)
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ||
            (dense_first >= NDS_NATIVE_DENSE_VERTEX_COUNT) ||
            (sNdsNativeFighterDenseVertices[dense_first].cache_slot !=
             action->index) ||
            (sNdsNativeFighterDenseVertices[dense_first].matrix_binding >=
             NDS_NATIVE_ROOT_BINDING_COUNT))
#else
            )
#endif
        {
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

static s32 ndsRendererValidateNativeRun(
    u32 run_index,
    const NDSNativeRoot *root,
    u32 *source_command_index,
    u32 *tri2_half)
{
    u32 run_count = sizeof(sNdsNativeFighterRuns) /
        sizeof(sNdsNativeFighterRuns[0]);
    u32 triangle_count = sizeof(sNdsNativeFighterTriangles) /
        sizeof(sNdsNativeFighterTriangles[0]);
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 run_corner_count = sizeof(sNdsNativeFighterRunFirstCorner) /
        sizeof(sNdsNativeFighterRunFirstCorner[0]);
    u32 dense_corner_count = sizeof(sNdsNativeFighterDenseCorners) /
        sizeof(sNdsNativeFighterDenseCorners[0]);
#endif
    const NDSNativeRun *run;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 first_corner;
    u32 corner_count;
#endif
    u32 i;

    if ((root == NULL) || (source_command_index == NULL) ||
        (tri2_half == NULL) || (run_index >= run_count)
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ||
        (run_index >= run_corner_count))
#else
        )
#endif
    {
        return FALSE;
    }
    run = &sNdsNativeFighterRuns[run_index];
#if NDS_RENDERER_PROFILE_LEVEL < 2
    corner_count = (u32)run->triangle_count * 3u;
    first_corner = sNdsNativeFighterRunFirstCorner[run_index];
#endif
    if ((run->triangle_count == 0u) ||
        (run->submit_class > NDS_NATIVE_RUN_CROSS_MATRIX) ||
        (ndsRendererNativeArraySpanFits(
             run->first_triangle, run->triangle_count,
             triangle_count) == FALSE)
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ||
        (ndsRendererNativeArraySpanFits(
             first_corner, corner_count, dense_corner_count) == FALSE))
#else
        )
#endif
    {
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    for (i = 0u; i < corner_count; i++)
    {
        u32 dense_id = sNdsNativeFighterDenseCorners[first_corner + i];

        if ((dense_id >= NDS_NATIVE_DENSE_VERTEX_COUNT) ||
            (sNdsNativeFighterDenseVertices[dense_id].cache_slot >=
             NDS_RENDERER_MAX_VTX) ||
            (sNdsNativeFighterDenseVertices[dense_id].matrix_binding >=
             NDS_NATIVE_ROOT_BINDING_COUNT))
        {
            return FALSE;
        }
    }
#endif
    for (i = 0u; i < run->triangle_count; i++)
    {
        u32 encoded =
            sNdsNativeFighterTriangles[run->first_triangle + i];
        u32 compact = encoded & 0x7fffu;
        u32 required =
            (1u << ((compact >> 10) & 31u)) |
            (1u << ((compact >> 5) & 31u)) |
            (1u << (compact & 31u));

        if ((*source_command_index >= root->source_command_count) ||
            ((run->required_mask & required) != required))
        {
            return FALSE;
        }
        if ((encoded & 0x8000u) != 0u)
        {
            if (*tri2_half != 0u)
            {
                return FALSE;
            }
            *tri2_half = 1u;
        }
        else
        {
            (*source_command_index)++;
            *tri2_half = 0u;
        }
    }
    return TRUE;
}
#endif

#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER && NDS_RENDERER_HW_TRIANGLES
void ndsRendererProfileCensusNativeFighterSchedule(
    u32 slot,
    const u8 *joint_parents,
    const u8 *joint_bindings,
    u32 joint_count,
    u32 binding_count,
    u32 *schedule_match_count,
    u32 *binding_match_count)
{
    const u16 *schedule;
    const u8 *binding_joints;
    u32 expected_joint_count;
    u32 expected_binding_count;
    u32 joint_index;
    u32 binding_index;
    u32 schedule_matches = 0u;
    u32 binding_matches = 0u;

    if ((schedule_match_count == NULL) || (binding_match_count == NULL))
    {
        return;
    }
    *schedule_match_count = 0u;
    *binding_match_count = 0u;
    if ((slot > 1u) || (joint_parents == NULL) ||
        (joint_bindings == NULL))
    {
        return;
    }
    if (slot == 0u)
    {
        schedule = sNdsNativeMarioJointSchedule;
        binding_joints = sNdsNativeMarioBindingJoints;
        expected_joint_count = sizeof(sNdsNativeMarioJointSchedule) /
            sizeof(sNdsNativeMarioJointSchedule[0]);
        expected_binding_count = sizeof(sNdsNativeMarioBindingJoints) /
            sizeof(sNdsNativeMarioBindingJoints[0]);
    }
    else
    {
        schedule = sNdsNativeFoxJointSchedule;
        binding_joints = sNdsNativeFoxBindingJoints;
        expected_joint_count = sizeof(sNdsNativeFoxJointSchedule) /
            sizeof(sNdsNativeFoxJointSchedule[0]);
        expected_binding_count = sizeof(sNdsNativeFoxBindingJoints) /
            sizeof(sNdsNativeFoxBindingJoints[0]);
    }
    for (joint_index = 0u;
         (joint_index < joint_count) &&
         (joint_index < expected_joint_count);
         joint_index++)
    {
        u32 expected = schedule[joint_index];

        if (((expected & 31u) == joint_parents[joint_index]) &&
            (((expected >> 5) & 31u) == joint_bindings[joint_index]))
        {
            schedule_matches++;
        }
    }
    for (binding_index = 0u;
         (binding_index < binding_count) &&
         (binding_index < expected_binding_count);
         binding_index++)
    {
        u32 joint = binding_joints[binding_index];

        if ((joint < joint_count) &&
            (joint_bindings[joint] == binding_index))
        {
            binding_matches++;
        }
    }
    *schedule_match_count = schedule_matches;
    *binding_match_count = binding_matches;
}
#endif

s32 ndsRendererValidateNativeFighterOwner(
    u32 slot,
    u32 asset_data_size,
    u32 root_count,
    const u32 *root_offsets,
    const u32 *material_counts)
{
#if NDS_RENDERER_HW_TRIANGLES
    const NDSNativeRoot *roots;
    u32 expected_count;
    u32 expected_asset_data_size;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 epoch_count = sizeof(sNdsNativeFighterEpochs) /
        sizeof(sNdsNativeFighterEpochs[0]);
    u32 action_count = sizeof(sNdsNativeFighterVertexActions) /
        sizeof(sNdsNativeFighterVertexActions[0]);
    u32 run_count = sizeof(sNdsNativeFighterRuns) /
        sizeof(sNdsNativeFighterRuns[0]);
#endif
    u32 root_index;

    if ((slot > 1u) || (root_offsets == NULL) ||
        (material_counts == NULL))
    {
        return FALSE;
    }
    if (slot == 0u)
    {
        roots = sNdsNativeMarioRoots;
        expected_asset_data_size = 0x7510u;
        expected_count = sizeof(sNdsNativeMarioRoots) /
            sizeof(sNdsNativeMarioRoots[0]);
    }
    else
    {
        roots = sNdsNativeFoxRoots;
        expected_asset_data_size = 0x7e50u;
        expected_count = sizeof(sNdsNativeFoxRoots) /
            sizeof(sNdsNativeFoxRoots[0]);
    }
    if ((asset_data_size != expected_asset_data_size) ||
        (root_count != expected_count))
    {
        return FALSE;
    }
    for (root_index = 0u; root_index < root_count; root_index++)
    {
        const NDSNativeRoot *root = &roots[root_index];
        u32 epoch_index;

        if (root->root_offset != root_offsets[root_index])
        {
            return FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (
            (ndsRendererNativeAssetSpanFits(
                 root->root_offset, root->source_command_count,
                 sizeof(Gfx), asset_data_size) == FALSE) ||
            (ndsRendererNativeArraySpanFits(
                 root->first_epoch, root->epoch_count,
                 epoch_count) == FALSE) ||
            (ndsRendererValidateNativeStateSpan(
                 root->tail_state_first, root->tail_state_count,
                 asset_data_size) == FALSE))
        {
            return FALSE;
        }
#endif
        for (epoch_index = 0u;
             epoch_index < root->epoch_count;
             epoch_index++)
        {
            const NDSNativeEpoch *epoch =
                &sNdsNativeFighterEpochs[
                    root->first_epoch + epoch_index];
#if NDS_RENDERER_PROFILE_LEVEL < 2
            u32 action_index;
            u32 run_index;
            u32 source_command_index =
                epoch->first_triangle_command_index;
            u32 tri2_half = 0u;
#endif

            if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
                (epoch->material_slot >= material_counts[root_index]))
            {
                return FALSE;
            }
#if NDS_RENDERER_PROFILE_LEVEL < 2
            if ((ndsRendererValidateNativeStateSpan(
                     epoch->before_state_first,
                     epoch->before_state_count,
                     asset_data_size) == FALSE) ||
                (ndsRendererValidateNativeStateSpan(
                     epoch->after_state_first,
                     epoch->after_state_count,
                     asset_data_size) == FALSE) ||
                (ndsRendererNativeArraySpanFits(
                     epoch->first_action, epoch->action_count,
                     action_count) == FALSE) ||
                (ndsRendererNativeArraySpanFits(
                     epoch->first_run, epoch->run_count,
                     run_count) == FALSE))
            {
                return FALSE;
            }
            for (action_index = 0u;
                 action_index < epoch->action_count;
                 action_index++)
            {
                if (ndsRendererValidateNativeVertexAction(
                        (u32)epoch->first_action + action_index,
                        root, asset_data_size) == FALSE)
                {
                    return FALSE;
                }
            }
            for (run_index = 0u;
                 run_index < epoch->run_count;
                 run_index++)
            {
                if (ndsRendererValidateNativeRun(
                        (u32)epoch->first_run + run_index,
                        root, &source_command_index,
                        &tri2_half) == FALSE)
                {
                    return FALSE;
                }
            }
            if (tri2_half != 0u)
            {
                return FALSE;
            }
#endif
        }
    }
    return TRUE;
#else
    (void)slot;
    (void)asset_data_size;
    (void)root_count;
    (void)root_offsets;
    (void)material_counts;
    return FALSE;
#endif
}

s32 ndsRendererExecuteNativeFighterRoot(
    u32 slot,
    u32 root_ordinal,
    const void *asset_base,
    u32 root_offset,
    const NDSRendererNativeMaterial *materials,
    u32 material_count,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
#if NDS_RENDERER_HW_TRIANGLES
    return ndsRendererExecuteNativeFighterRootHardware(
        slot, root_ordinal, asset_base, root_offset,
        materials, material_count, config,
        callback, callback_user, stats, vertex_cache);
#else
    (void)slot;
    (void)root_ordinal;
    (void)asset_base;
    (void)root_offset;
    (void)materials;
    (void)material_count;
    (void)config;
    (void)callback;
    (void)callback_user;
    (void)stats;
    (void)vertex_cache;
    return FALSE;
#endif
}

static void NDS_RENDERER_HOT_CODE
ndsRendererScanList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats,
                                NDSRendererTraversalState *state,
                                u32 depth,
                                NDSRendererCommandCallback callback,
                                void *callback_user)
{
    u32 i;
    u32 immutable_command_count = 0u;

    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (depth > config->max_depth)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_TOO_DEEP;
        return;
    }
    if (ndsRendererValidateCommand(dl, config) == FALSE)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }
    if (depth > stats->max_depth_seen)
    {
        stats->max_depth_seen = depth;
    }
    if (config->immutable_command_span != NULL)
    {
        size_t immutable_bytes =
            config->immutable_command_span(dl, config->user);
        size_t immutable_count = immutable_bytes / sizeof(*dl);

        if (immutable_count > config->max_list_commands)
        {
            immutable_count = config->max_list_commands;
        }
        immutable_command_count = (u32)immutable_count;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (immutable_command_count != 0u)
        {
            sNdsRendererProfileImmutableListCount++;
        }
#endif
    }

    for (i = 0; i < config->max_list_commands; i++, dl++)
    {
        u32 w0;
        u32 w1;
        u32 op;
        NDSRendererCommand command;

        if ((i >= immutable_command_count) &&
            (ndsRendererValidateCommand(dl, config) == FALSE))
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
            return;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (i < immutable_command_count)
        {
            sNdsRendererProfileTrustedCommandCount++;
        }
#endif
        if (stats->command_count >= config->max_commands)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
            return;
        }

        w0 = dl->words.w0;
        w1 = dl->words.w1;
        op = w0 >> 24;
#if NDS_RENDERER_HW_TRIANGLES
        state->source_command_site = dl;
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        state->semantic_command_index = i;
        state->semantic_tri2_half = 0u;
#endif
#if NDS_RENDERER_HW_TRIANGLES
        /* Preserve the source-command boundary: only adjacent TRI1/TRI2
         * opcodes may share a GX triangle group. In particular, close before
         * VTX/MODIFYVTX mutate the cached vertices and before any matrix,
         * texture, state, branch, sync, or ENDDL command. */
        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            ndsRendererHardwareEndBatch();
            /* VTX and matrix commands end the GX primitive group but cannot
             * change the prepared texture/material/depth epoch. The exact
             * state opcodes below invalidate that epoch at their mutation. */
            state->prepared_vertex_color_valid_mask = 0u;
            state->prepared_texcoord_valid_mask = 0u;
            state->prepared_projected_xy_valid_mask = 0u;
            state->prepared_projected_source_z_valid_mask = 0u;
        }
#endif
        if ((callback != NULL) || (op == NDS_RENDERER_OP_DL))
        {
            memset(&command, 0, sizeof(command));
            command.dl = dl;
            command.w0 = w0;
            command.w1 = w1;
            command.op = op;
            command.depth = depth;
            command.list_index = i;
            command.transformed_vertices = state->vertices;
            command.transformed_vertex_valid_mask = state->vertex_valid_mask;
            command.matrix_valid = state->matrix_valid;

            if (op == NDS_RENDERER_OP_DL)
            {
                command.raw_branch_dl = (const Gfx *)(uintptr_t)w1;
                command.resolved_branch_dl = command.raw_branch_dl;
                if (config->resolve_branch != NULL)
                {
                    command.resolved_branch_dl = config->resolve_branch(
                        command.raw_branch_dl,
                        &command.branch_resolve_kind,
                        config->user);
                }
                command.branch_is_jump =
                    ((w0 & (1u << 16)) != 0) ? TRUE : FALSE;
            }
        }

        if (stats->first_opcode == 0)
        {
            stats->first_opcode = op;
        }
        stats->command_count++;

        if ((callback != NULL) &&
            (callback(&command, callback_user) == FALSE))
        {
            ndsRendererRecordUnsupported(stats, op);
            stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
            return;
        }

        switch (op)
        {
        case NDS_RENDERER_OP_NOOP:
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            break;

        case NDS_RENDERER_OP_MODIFYVTX:
            ndsRendererApplyModifyVertexCommand(stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_VTX:
            ndsRendererApplyVertexCommand(config, stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_TRI1:
        case NDS_RENDERER_OP_TRI2:
        {
            ndsRendererExecuteTriangleCommand(
                stats, config, state, op, w0, w1);
#if NDS_RENDERER_HW_TRIANGLES
            /* Immutable adjacent TRI commands have no intervening source
             * state transition. Profile 0/1 has no command callback, so
             * replay the remainder of the run without rebuilding a generic
             * command record or re-entering the full opcode switch. */
#if NDS_RENDERER_PROFILE_LEVEL < 2
            if ((callback == NULL) && (sNdsRendererFastOwnerEnabled != 0u))
#else
            if (sNdsRendererFastOwnerEnabled != 0u)
#endif
            {
                ndsRendererExecuteFastRawCurrentRun(
                    &dl, &i, immutable_command_count,
                    config, stats, state, depth, callback, callback_user);
            }
#if NDS_RENDERER_PROFILE_LEVEL < 2
            else while ((callback == NULL) &&
                   ((i + 1u) < immutable_command_count) &&
                   ((i + 1u) < config->max_list_commands))
            {
                const Gfx *next_dl = dl + 1;
                u32 next_w0 = next_dl->words.w0;
                u32 next_op = next_w0 >> 24;

                if ((next_op != NDS_RENDERER_OP_TRI1) &&
                    (next_op != NDS_RENDERER_OP_TRI2))
                {
                    break;
                }
                if (stats->command_count >= config->max_commands)
                {
                    stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
                    return;
                }
                i++;
                dl = next_dl;
                stats->command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                sNdsRendererProfileTrustedCommandCount++;
                sNdsRendererProfileTriangleRunReuseCount++;
#endif
                ndsRendererExecuteTriangleCommand(
                    stats, config, state, next_op, next_w0,
                    next_dl->words.w1);
            }
#endif
#endif
            break;
        }

        case NDS_RENDERER_OP_ENDDL:
            stats->end_command_count++;
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            return;

        case NDS_RENDERER_OP_DL:
        {
            const Gfx *raw_branch = command.raw_branch_dl;
            const Gfx *branch = command.resolved_branch_dl;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
            u32 parent_branch_path = state->semantic_branch_path;
            u32 child_branch_path;
#endif

            stats->branch_command_count++;
            if (stats->first_branch_dl == NULL)
            {
                stats->first_branch_dl = raw_branch;
            }
            if (stats->first_resolved_branch_dl == NULL)
            {
                stats->first_resolved_branch_dl = branch;
            }
            if (command.branch_resolve_kind == NDS_RENDERER_RESOLVE_SEGMENT)
            {
                stats->segment_resolve_count++;
            }
            if (ndsRendererValidateCommand(branch, config) == FALSE)
            {
                stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
                return;
            }
            if ((w0 & (1u << 16)) != 0)
            {
                stats->branch_jump_count++;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
                child_branch_path = ndsRendererSemanticBranchPath(
                    parent_branch_path, i, depth + 1u, TRUE);
                state->semantic_branch_path = child_branch_path;
#endif
                ndsRendererScanList(branch, config, stats, state, depth + 1u,
                                    callback, callback_user);
                return;
            }

            stats->branch_call_count++;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
            child_branch_path = ndsRendererSemanticBranchPath(
                parent_branch_path, i, depth + 1u, FALSE);
            state->semantic_branch_path = child_branch_path;
#endif
            ndsRendererScanList(branch, config, stats, state, depth + 1u,
                                callback, callback_user);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
            state->semantic_branch_path = parent_branch_path;
#endif
            if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
            {
                return;
            }
            break;
        }

        case NDS_RENDERER_OP_TEXTURE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordTextureState(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_MTX:
            ndsRendererApplyMatrixCommand(config, stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_POPMTX:
            ndsRendererApplyPopMatrixCommand(stats, state, w1);
            break;

        case NDS_RENDERER_OP_MOVEWORD:
            ndsRendererApplyMatrixMoveWordCommand(stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_MOVEMEM:
            ndsRendererRecordLightMoveMem(config, stats, w0, w1);
            NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state);
            break;

        case NDS_RENDERER_OP_SPECIAL_1:
            ndsRendererApplyMvpRecalcCommand(stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_SETSCISSOR:
        case NDS_RENDERER_OP_SETCIMG:
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            stats->ignored_state_command_count++;
            break;

        case NDS_RENDERER_OP_SETPRIMDEPTH:
            ndsRendererRecordPrimDepth(stats, w1);
            break;

        case NDS_RENDERER_OP_GEOMETRYMODE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            stats->geometry_mode = (stats->geometry_mode & w0) | w1;
            stats->geometry_clear_mask = w0;
            stats->geometry_set_mask = w1;
            stats->geometry_command_count++;
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETCOMBINE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetCombine(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTIMG:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetImage(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTILE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetTile(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADTILE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadTile(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADBLOCK:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadBlock(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADTLUT:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadTlut(stats, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTILESIZE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetTileSize(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETFOGCOLOR:
            ndsRendererRecordFogColor(stats, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            stats->color_command_count++;
            break;

        case NDS_RENDERER_OP_SETBLENDCOLOR:
        case NDS_RENDERER_OP_SETENVCOLOR:
        case NDS_RENDERER_OP_SETPRIMCOLOR:
            if (op != NDS_RENDERER_OP_SETBLENDCOLOR)
            {
                NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            }
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            stats->color_command_count++;
            if (op == NDS_RENDERER_OP_SETPRIMCOLOR)
            {
                stats->prim_color = w1;
                stats->prim_min_level = (w0 >> 8) & 0xffu;
                stats->prim_lod_fraction = w0 & 0xffu;
            }
            else if (op == NDS_RENDERER_OP_SETENVCOLOR)
            {
                stats->env_color = w1;
            }
            else
            {
                stats->blend_color = w1;
            }
            break;

        case NDS_RENDERER_OP_RDPPIPESYNC:
        case NDS_RENDERER_OP_RDPLOADSYNC:
        case NDS_RENDERER_OP_RDPTILESYNC:
        case NDS_RENDERER_OP_RDPFULLSYNC:
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            stats->sync_command_count++;
            break;

        case NDS_RENDERER_OP_SETOTHERMODE_H:
        case NDS_RENDERER_OP_SETOTHERMODE_L:
        case NDS_RENDERER_OP_RDPSETOTHERMODE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordOtherMode(stats, op, w0, w1);
            break;

        case NDS_RENDERER_OP_CULLDL:
            ndsRendererRecordCull(stats, w0, w1);
            break;

        default:
            ndsRendererRecordUnsupported(stats, op);
            break;
        }
    }
}

void ndsRendererInitStats(NDSRendererStats *stats)
{
    if (stats != NULL)
    {
        memset(stats, 0, sizeof(*stats));
        stats->geometry_mode = NDS_RENDERER_GEOM_RESET_MODE;
        stats->othermode_h = NDS_RENDERER_TP_PERSP | NDS_RENDERER_TF_BILERP;
        stats->texture_source_hash1 = 2166136261u;
        stats->texture_source_hash2 = 0x9e3779b9u;
    }
}

void ndsRendererInitVertexCache(NDSRendererVertexCache *vertex_cache)
{
    if (vertex_cache == NULL)
    {
        return;
    }

    vertex_cache->input_valid_mask = 0u;
    vertex_cache->raw_vertex_fit_mask = 0u;
    vertex_cache->transformed_valid_mask = 0u;
    vertex_cache->vertex_color_valid_mask = 0u;
    vertex_cache->matrix_snapshot_count = 0u;
    memset(vertex_cache->vertex_matrix_snapshot, 0,
           sizeof(vertex_cache->vertex_matrix_snapshot));
    memset(vertex_cache->vertex_clip_snapshot, 0,
           sizeof(vertex_cache->vertex_clip_snapshot));
}

void ndsRendererScanDisplayList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats)
{
    NDSRendererTraversalState state;
    NDSRendererTraversalVertexStorage vertex_storage;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererMatrixSnapshot
        matrix_snapshot_storage[NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY];
    NDSRendererMatrixSnapshot *matrix_snapshots = matrix_snapshot_storage;
#else
    NDSRendererMatrixSnapshot *matrix_snapshots = NULL;
#endif

    if (stats == NULL)
    {
        return;
    }

    if (config == NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }

    ndsRendererInitTraversalState(&state, config, stats, &vertex_storage,
                                  matrix_snapshots, 0u);
    ndsRendererScanList(dl, config, stats, &state, 0, NULL, NULL);
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererHardwareEndBatch();
#endif
    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (stats->unsupported_command_count != 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return;
    }
    if (stats->vertex_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_VERTICES;
        return;
    }
    if (stats->triangle_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_TRIANGLES;
        return;
    }
    if (stats->end_command_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_END;
        return;
    }
}

void ndsRendererHardwareResetSourceCaches(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    memset(sNdsRendererDirectRawPlans, 0,
           sizeof(sNdsRendererDirectRawPlans));
    sNdsRendererDirectRawEntryCount = 0u;
    memset(sNdsRendererStageTextureSites, 0,
           sizeof(sNdsRendererStageTextureSites));
    sNdsRendererStageTextureSiteNext = 0u;
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    memset(sNdsRendererHardwareCi4IndexCache, 0,
           sizeof(sNdsRendererHardwareCi4IndexCache));
    sNdsRendererHardwareCi4IndexCacheNext = 0u;
#endif
}

void ndsRendererHardwareSetNoOracle(u32 enabled)
{
#if NDS_RENDERER_HW_TRIANGLES
    sNdsRendererHardwareNoOracle = (enabled != 0u) ? TRUE : FALSE;
#else
    (void)enabled;
#endif
}

u32 ndsRendererHardwareNoOracleEnabled(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    return sNdsRendererHardwareNoOracle;
#else
    return FALSE;
#endif
}

void ndsRendererProfileSetOwner(NDSRendererProfileOwner owner)
{
#if NDS_TASK29_GX_CENSUS
    ndsRendererTask29GXSetOwner(owner);
#endif
#if NDS_RENDERER_HW_TRIANGLES
    u32 mode = gNdsRendererFastRunMode;

    sNdsRendererRuntimeOwner =
        ((u32)owner < (u32)NDS_RENDERER_PROFILE_OWNER_COUNT) ? owner :
        NDS_RENDERER_PROFILE_OWNER_NONE;
    sNdsRendererStageTextureSitesEnabled =
        (((mode == NDS_RENDERER_FAST_RUN_STAGE_TEXTURE_SITES) ||
          (mode == NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS) ||
          (mode ==
           NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION)) &&
         (sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_STAGE)) ?
            TRUE : FALSE;
    sNdsRendererFastOwnerEnabled =
        ((mode == NDS_RENDERER_FAST_RUN_MARIO_ONLY) &&
         (sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_MARIO)) ||
        ((mode == NDS_RENDERER_FAST_RUN_FIGHTERS) &&
         ((sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_MARIO) ||
          (sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_FOX))) ||
        (((mode == NDS_RENDERER_FAST_RUN_ALL_RAW_CURRENT) ||
          (mode == NDS_RENDERER_FAST_RUN_STAGE_TEXTURE_SITES) ||
          (mode == NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS) ||
          (mode ==
           NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION)) &&
         ((u32)sNdsRendererRuntimeOwner <
          (u32)NDS_RENDERER_PROFILE_OWNER_COUNT));
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileOwner =
        ((u32)owner < (u32)NDS_RENDERER_PROFILE_OWNER_COUNT) ? owner :
        NDS_RENDERER_PROFILE_OWNER_NONE;
    memset(&sNdsRendererSemanticSourceProvenance, 0,
           sizeof(sNdsRendererSemanticSourceProvenance));
#else
#if !NDS_RENDERER_HW_TRIANGLES
    (void)owner;
#endif
#endif
}

void ndsRendererProfileSetSourceProvenance(u32 owner_occurrence,
                                           u32 list_ordinal,
                                           u32 root_branch_path)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    NDSRendererProfileOwnerHotLedger *owner;
    u32 owner_index = (u32)sNdsRendererProfileOwner;
    u32 owner_mask;

    sNdsRendererSemanticSourceProvenance.owner_occurrence =
        owner_occurrence;
    sNdsRendererSemanticSourceProvenance.list_ordinal = list_ordinal;
    sNdsRendererSemanticSourceProvenance.root_branch_path = root_branch_path;
    owner = ndsRendererProfileCurrentOwner();
    if (owner == NULL)
    {
        return;
    }
    owner_mask = 1u << owner_index;
    if (((sNdsRendererSemanticOwnerOccurrenceValidMask & owner_mask) == 0u) ||
        (sNdsRendererSemanticOwnerLastOccurrence[owner_index] !=
         owner_occurrence))
    {
        sNdsRendererSemanticOwnerOccurrenceValidMask |= owner_mask;
        sNdsRendererSemanticOwnerLastOccurrence[owner_index] =
            owner_occurrence;
        owner->semantic_occurrence_count++;
    }
#else
    (void)owner_occurrence;
    (void)list_ordinal;
    (void)root_branch_path;
#endif
}

void ndsRendererProfileRecordFrameBoundaryGXState(u32 status, u32 control)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsRendererProfileGXStatusPostVBlank = status;
    sNdsRendererProfileGXControlPostVBlank = control;
#else
    (void)status;
    (void)control;
#endif
}

void ndsRendererProfileRecordMaterialOperations(u32 count)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    NDSRendererProfileOwnerHotLedger *owner =
        ndsRendererProfileCurrentOwner();

    if (owner != NULL)
    {
        owner->material_operation_count += count;
    }
#else
    (void)count;
#endif
}

u32 ndsRendererProfileGlobalStateHash(void)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    u32 hash = 0u;

    hash = ndsRendererProfileHashU32(
        hash, (u32)sNdsRendererHardwareProjectedDepth);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareProjectedBackground);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererMatrixGenerationSerial);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareMatrixLoaded);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareMatrixMode);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareMatrixGeneration);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareMatrixSignature);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareBoundTextureName);
    hash = ndsRendererProfileHashU32(
        hash, (u32)sNdsRendererHardwareNoTextureName);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchOpen);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchTextured);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchTextureName);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchPolyFmt);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchAlphaKey);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchFogKey);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchMatrixMode);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchMatrixGeneration);
    if (sNdsRendererHardwareActiveTextureEntry != NULL)
    {
        hash = ndsRendererProfileHashU32(hash, 1u);
        hash = ndsRendererProfileHashU32(
            hash, (u32)sNdsRendererHardwareActiveTextureEntry->name);
        hash = ndsRendererProfileHashU32(
            hash, sNdsRendererHardwareActiveTextureEntry->ready);
        hash = ndsRendererProfileHashU32(
            hash, sNdsRendererHardwareActiveTextureEntry->params);
        hash = ndsRendererProfileHashU32(
            hash, ndsRendererProfileTextureKeyHashFull(
                &sNdsRendererHardwareActiveTextureEntry->key));
    }
    else
    {
        hash = ndsRendererProfileHashU32(hash, 0u);
    }
    return hash;
#else
    return 0u;
#endif
}

#if NDS_RENDER_ECONOMY
void ndsRendererProfileFrameBegin(u32 render_economy_allowed)
#else
void ndsRendererProfileFrameBegin(void)
#endif
{
    gNdsRendererIFCommonCloudQueuedCount = 0u;
    gNdsRendererIFCommonCloudEmittedCount = 0u;
#if NDS_RENDERER_SCREEN_SPACE_CENSUS
    if (gNdsRendererScreenSpaceCensusResetRequested != 0u)
    {
        ndsRendererScreenSpaceCensusReset();
        gNdsRendererScreenSpaceCensusResetRequested = 0u;
    }
    if (gNdsRendererScreenSpaceCensusArmed != 0u)
    {
        gNdsRendererScreenSpaceCensusFrameCount++;
    }
#endif
#if NDS_RENDER_ECONOMY
    gNdsRendererEconomyActiveOwnerMask = 0u;
    gNdsRendererEconomyAppliedOwnerMask = 0u;
    gNdsRendererEconomySkippedRunCount = 0u;
    gNdsRendererEconomySkippedTriangleCount = 0u;
    if (render_economy_allowed != 0u)
    {
        gNdsRendererEconomyActiveOwnerMask =
            gNdsRendererEconomyConfiguredOwnerMask;
    }
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    if (gNdsRendererBenchmarkSinkCalibrationWords == 0u)
    {
        u32 calibration_start = cpuGetTiming();
        u32 i;

        for (i = 0u; i < NDS_RENDERER_BENCHMARK_SINK_WORDS; i++)
        {
            ndsRendererBenchmarkSinkWord(i);
        }
        gNdsRendererBenchmarkSinkCalibrationTicks =
            cpuGetTiming() - calibration_start;
        gNdsRendererBenchmarkSinkCalibrationWords =
            NDS_RENDERER_BENCHMARK_SINK_WORDS;
    }
    sNdsRendererBenchmarkSinkCursor = 0u;
    sNdsRendererBenchmarkSinkWordCount = 0u;
    sNdsRendererBenchmarkSinkLastOwnerCursor = 0u;
    sNdsRendererBenchmarkSinkHashA = 2166136261u;
    sNdsRendererBenchmarkSinkHashB = 0x9e3779b9u;
    sNdsRendererBenchmarkSegment0SinkWords = 0u;
    sNdsRendererBenchmarkSegment0SinkHashA = 2166136261u;
    sNdsRendererBenchmarkSegment0SinkHashB = 0x9e3779b9u;
    sNdsRendererBenchmarkSegment0SinkActive = FALSE;
    sNdsRendererBenchmarkSegment0SinkArmFaults = 0u;
    memset(sNdsRendererBenchmarkSinkOwnerWords, 0,
           sizeof(sNdsRendererBenchmarkSinkOwnerWords));
#endif
    gNdsRendererProfileLevel = NDS_RENDERER_PROFILE_LEVEL;
    gNdsRendererProfileNearPlaneTriangleRejectCount = 0u;
    gNdsRendererProfileRawCurrentCandidateCount = 0u;
    gNdsRendererProfileRawCurrentRangeRejectCount = 0u;
    gNdsRendererProfileRawCrossMatrixCount = 0u;
    gNdsRendererProfileMatrixPosTestSamples = 0u;
    gNdsRendererProfileMatrixPosTestMismatches = 0u;
    gNdsRendererProfileMatrixPosTestMaxError = 0u;
    gNdsRendererProfileMatrixPosTestWSignMismatches = 0u;
    gNdsRendererProfileMatrixPosTestClipMismatches = 0u;
    gNdsRendererProfileMatrixPosTestMatrixWordSamples = 0u;
    gNdsRendererProfileMatrixPosTestDropped = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    memset((void *)gNdsRendererProfileOwners, 0,
           sizeof(gNdsRendererProfileOwners));
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsRendererM2ShadeEpochCount = 0u;
    sNdsRendererM2ShadeKeyHitCount = 0u;
    sNdsRendererM2ShadeResidentHitCount = 0u;
    sNdsRendererM2ShadeHashCollisionCount = 0u;
    sNdsRendererM2ShadeDenseVisitCount = 0u;
    sNdsRendererM2ShadeComputeCount = 0u;
    sNdsRendererM2ShadeLutComputeCount = 0u;
    sNdsRendererM2ShadePreparedComputeCount = 0u;
    sNdsRendererM2ShadeAliasCopyCount = 0u;
    sNdsRendererM2ShadeMaterialPackCount = 0u;
    memset(sNdsRendererM2ShadeOwnerEpochCount, 0,
           sizeof(sNdsRendererM2ShadeOwnerEpochCount));
    memset(sNdsRendererM2ShadeOwnerKeyHitCount, 0,
           sizeof(sNdsRendererM2ShadeOwnerKeyHitCount));
    memset(sNdsRendererM2ShadeOwnerResidentHitCount, 0,
           sizeof(sNdsRendererM2ShadeOwnerResidentHitCount));
#endif
#if NDS_RENDERER_HW_TRIANGLES
    gNdsRendererProfileTexturePairOracleChecks = 0u;
    gNdsRendererProfileTexturePairOracleMismatches = 0u;
    gNdsRendererProfileTextureVBlankQueuedUploads = 0u;
    gNdsRendererProfileTextureVBlankQueuedBytes = 0u;
    gNdsRendererProfileTextureVBlankCommittedUploads = 0u;
    gNdsRendererProfileTextureVBlankCommitTicks = 0u;
    gNdsRendererProfileTextureVBlankOutsideCount = 0u;
    gNdsRendererProfileTextureVBlankFallbackCount = 0u;
    gNdsRendererProfileTextureVBlankStartLine = 0u;
    gNdsRendererProfileTextureVBlankEndLine = 0u;
#endif
    sNdsRendererProfileGXStatusPostVBlank = 0u;
    sNdsRendererProfileGXControlPostVBlank = 0u;
    gNdsRendererProfileGXStatusPostVBlank = 0u;
    gNdsRendererProfileGXControlPostVBlank = 0u;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    memset(sNdsRendererProfileOwnerHot, 0,
           sizeof(sNdsRendererProfileOwnerHot));
    sNdsRendererProfileOwner = NDS_RENDERER_PROFILE_OWNER_NONE;
    sNdsRendererProfileImmutableListCount = 0u;
    sNdsRendererProfileTrustedCommandCount = 0u;
    sNdsRendererProfileValidatedCommandCount = 0u;
    sNdsRendererProfileTriangleRunReuseCount = 0u;
    sNdsRendererProfileTriangleSubmitTicks = 0u;
    sNdsRendererProfileVertexSubmitTicks = 0u;
    sNdsRendererProfileCi4LutBuildCount = 0u;
    sNdsRendererProfileCi4LutReuseCount = 0u;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    memset(&sNdsRendererSemanticSourceProvenance, 0,
           sizeof(sNdsRendererSemanticSourceProvenance));
    memset(sNdsRendererSemanticOwnerLastOccurrence, 0,
           sizeof(sNdsRendererSemanticOwnerLastOccurrence));
    sNdsRendererSemanticOwnerOccurrenceValidMask = 0u;
    sNdsRendererSemanticOutputHash = 0u;
    sNdsRendererSemanticOutputHash2 = 0u;
    sNdsRendererSemanticEventCount = 0u;
    sNdsRendererSemanticOverflowCount = 0u;
    sNdsRendererSemanticLastTextureKeyHash = 0u;
    sNdsRendererSemanticLastTextureParams = 0u;
    gNdsRendererSemanticOutputHash = 0u;
    gNdsRendererSemanticOutputHash2 = 0u;
    gNdsRendererSemanticEventCount = 0u;
    gNdsRendererSemanticOverflowCount = 0u;
    memset((void *)gNdsRendererStageDepthTrace, 0,
           sizeof(gNdsRendererStageDepthTrace));
    gNdsRendererStageDepthTraceCount = 0u;
    gNdsRendererStageDepthTraceOverflowCount = 0u;
    gNdsRendererStageDepthTraceHash = 0u;
    memset((void *)gNdsRendererStageDepthTraceClassCount, 0,
           sizeof(gNdsRendererStageDepthTraceClassCount));
    gNdsRendererStageDepthTraceNoZCollisionCount = 0u;
    gNdsRendererStageDepthTraceBackgroundCount = 0u;
    gNdsRendererStageDepthTraceBackgroundMin = 0;
    gNdsRendererStageDepthTraceBackgroundMax = 0;
    gNdsRendererStageDepthTraceForegroundCount = 0u;
    gNdsRendererStageDepthTraceForegroundMin = 0;
    gNdsRendererStageDepthTraceForegroundMax = 0;
    sNdsRendererStageDepthTraceLastBackground = 0;
    sNdsRendererStageDepthTraceLastForeground = 0;
    sNdsRendererStageDepthTraceLastValidMask = 0u;
#endif
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileResetSubmitSummary();
#endif
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount = 0u;
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD
    sNdsRendererBenchmarkSuppressedTextureUploads = 0u;
    sNdsRendererBenchmarkSuppressedTextureUploadBytes = 0u;
#endif
#if NDS_RENDERER_HW_TRIANGLES
    sNdsRendererFastRunCount = 0u;
    sNdsRendererFastTriangleCount = 0u;
    memset(sNdsRendererFastOwnerTriangleCount, 0,
           sizeof(sNdsRendererFastOwnerTriangleCount));
    memset(sNdsRendererFastFallbackCount, 0,
           sizeof(sNdsRendererFastFallbackCount));
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    memset(&sNdsRendererRuntimeFrameSummary, 0,
           sizeof(sNdsRendererRuntimeFrameSummary));
    if (gNdsRendererProfileFrameCount == 1u)
    {
        sNdsRendererRuntimeTexel1FractionRefreshCount = 0u;
        sNdsRendererRuntimeTextureCacheEvictCount = 0u;
        sNdsRendererRuntimeTextureCi4DirectPixels = 0u;
        sNdsRendererRuntimeCi4IndexCacheBuildCount = 0u;
        sNdsRendererRuntimeCi4IndexCacheReuseCount = 0u;
        sNdsRendererRuntimeCi4RepresentativePixelCount = 0u;
        sNdsRendererRuntimeCi4ReusePixelCount = 0u;
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    sNdsRendererHardwarePendingPosTestCount = 0u;
    sNdsRendererHardwarePendingPosTestLastGeneration = 0u;
#endif
}

void ndsRendererProfileFramePublish(void)
{
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    gNdsRendererM2ShadeEpochCount = sNdsRendererM2ShadeEpochCount;
    gNdsRendererM2ShadeKeyHitCount = sNdsRendererM2ShadeKeyHitCount;
    gNdsRendererM2ShadeResidentHitCount =
        sNdsRendererM2ShadeResidentHitCount;
    gNdsRendererM2ShadeHashCollisionCount =
        sNdsRendererM2ShadeHashCollisionCount;
    gNdsRendererM2ShadeDenseVisitCount =
        sNdsRendererM2ShadeDenseVisitCount;
    gNdsRendererM2ShadeComputeCount = sNdsRendererM2ShadeComputeCount;
    gNdsRendererM2ShadeLutComputeCount =
        sNdsRendererM2ShadeLutComputeCount;
    gNdsRendererM2ShadePreparedComputeCount =
        sNdsRendererM2ShadePreparedComputeCount;
    gNdsRendererM2ShadeAliasCopyCount =
        sNdsRendererM2ShadeAliasCopyCount;
    gNdsRendererM2ShadeMaterialPackCount =
        sNdsRendererM2ShadeMaterialPackCount;
    memcpy((void *)gNdsRendererM2ShadeOwnerEpochCount,
           sNdsRendererM2ShadeOwnerEpochCount,
           sizeof(gNdsRendererM2ShadeOwnerEpochCount));
    memcpy((void *)gNdsRendererM2ShadeOwnerKeyHitCount,
           sNdsRendererM2ShadeOwnerKeyHitCount,
           sizeof(gNdsRendererM2ShadeOwnerKeyHitCount));
    memcpy((void *)gNdsRendererM2ShadeOwnerResidentHitCount,
           sNdsRendererM2ShadeOwnerResidentHitCount,
           sizeof(gNdsRendererM2ShadeOwnerResidentHitCount));
#endif
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    gNdsRendererBenchmarkTriangleCount =
        sNdsRendererBenchmarkTriangleCount;
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    gNdsRendererBenchmarkSinkCursor =
        sNdsRendererBenchmarkSinkCursor;
    gNdsRendererBenchmarkSinkWordCount =
        sNdsRendererBenchmarkSinkWordCount;
    gNdsRendererBenchmarkSinkHashA = sNdsRendererBenchmarkSinkHashA;
    gNdsRendererBenchmarkSinkHashB = sNdsRendererBenchmarkSinkHashB;
    gNdsRendererBenchmarkSegment0SinkWords =
        sNdsRendererBenchmarkSegment0SinkWords;
    gNdsRendererBenchmarkSegment0SinkHashA =
        sNdsRendererBenchmarkSegment0SinkHashA;
    gNdsRendererBenchmarkSegment0SinkHashB =
        sNdsRendererBenchmarkSegment0SinkHashB;
    gNdsRendererBenchmarkSegment0SinkArmFaults =
        sNdsRendererBenchmarkSegment0SinkArmFaults;
    memcpy((void *)gNdsRendererBenchmarkSinkOwnerWords,
           sNdsRendererBenchmarkSinkOwnerWords,
           sizeof(gNdsRendererBenchmarkSinkOwnerWords));
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD
    gNdsRendererBenchmarkSuppressedTextureUploads =
        sNdsRendererBenchmarkSuppressedTextureUploads;
    gNdsRendererBenchmarkSuppressedTextureUploadBytes =
        sNdsRendererBenchmarkSuppressedTextureUploadBytes;
#endif
#if NDS_RENDERER_HW_TRIANGLES
    gNdsRendererFastRunCount = sNdsRendererFastRunCount;
    gNdsRendererFastTriangleCount = sNdsRendererFastTriangleCount;
    memcpy((void *)gNdsRendererFastOwnerTriangleCount,
           sNdsRendererFastOwnerTriangleCount,
           sizeof(gNdsRendererFastOwnerTriangleCount));
    memcpy((void *)gNdsRendererFastFallbackCount,
           sNdsRendererFastFallbackCount,
           sizeof(gNdsRendererFastFallbackCount));
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileGXStatusPostVBlank =
        sNdsRendererProfileGXStatusPostVBlank;
    gNdsRendererProfileGXControlPostVBlank =
        sNdsRendererProfileGXControlPostVBlank;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 owner_index;

    gNdsRendererProfileImmutableListCount =
        sNdsRendererProfileImmutableListCount;
    gNdsRendererProfileTrustedCommandCount =
        sNdsRendererProfileTrustedCommandCount;
    gNdsRendererProfileValidatedCommandCount =
        sNdsRendererProfileValidatedCommandCount;
    gNdsRendererProfileTriangleRunReuseCount =
        sNdsRendererProfileTriangleRunReuseCount;
    gNdsRendererProfileTriangleSubmitTicks =
        sNdsRendererProfileTriangleSubmitTicks;
    gNdsRendererProfileVertexSubmitTicks =
        sNdsRendererProfileVertexSubmitTicks;
    gNdsRendererProfileCi4LutBuildCount =
        sNdsRendererProfileCi4LutBuildCount;
    gNdsRendererProfileCi4LutReuseCount =
        sNdsRendererProfileCi4LutReuseCount;
    for (owner_index = 0u;
         owner_index < NDS_RENDERER_PROFILE_OWNER_COUNT;
         owner_index++)
    {
        u32 submit_class;
        volatile NDSRendererOwnerProfile *owner =
            &gNdsRendererProfileOwners[owner_index];
        const NDSRendererProfileOwnerHotLedger *hot =
            &sNdsRendererProfileOwnerHot[owner_index];

        owner->material_operation_count = hot->material_operation_count;
        owner->texture_change_count = hot->texture_change_count;
        owner->run_count = hot->run_count;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        owner->semantic_output_hash = hot->semantic_output_hash;
        owner->semantic_output_hash2 = hot->semantic_output_hash2;
        owner->semantic_event_count = hot->semantic_event_count;
        owner->semantic_overflow_count = hot->semantic_overflow_count;
        owner->semantic_occurrence_count = hot->semantic_occurrence_count;
        owner->semantic_first_owner_occurrence =
            hot->semantic_first_owner_occurrence;
        owner->semantic_first_list_ordinal =
            hot->semantic_first_list_ordinal;
        owner->semantic_first_branch_path =
            hot->semantic_first_branch_path;
        owner->semantic_first_command_index =
            hot->semantic_first_command_index;
        owner->semantic_first_tri2_half = hot->semantic_first_tri2_half;
        owner->semantic_first_outcome = hot->semantic_first_outcome;
#endif
        for (submit_class = 0u; submit_class < 8u; submit_class++)
        {
            owner->submit_class_count[submit_class] =
                hot->submit_class_count[submit_class];
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererSemanticOutputHash = sNdsRendererSemanticOutputHash;
    gNdsRendererSemanticOutputHash2 = sNdsRendererSemanticOutputHash2;
    gNdsRendererSemanticEventCount = sNdsRendererSemanticEventCount;
    gNdsRendererSemanticOverflowCount =
        sNdsRendererSemanticOverflowCount;
#endif
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    /* One compact publication replaces hot-loop volatile diagnostic writes. */
    gNdsRendererProfileTextureBinds =
        sNdsRendererRuntimeFrameSummary.texture_binds;
    gNdsRendererProfileTextureUploads =
        sNdsRendererRuntimeFrameSummary.texture_uploads;
    gNdsRendererProfileTextureUploadBytes =
        sNdsRendererRuntimeFrameSummary.texture_upload_bytes;
    gNdsRendererProfileTextureCi4DirectPixels =
        sNdsRendererRuntimeTextureCi4DirectPixels;
    gNdsRendererProfileCi4IndexCacheBuildCount =
        sNdsRendererRuntimeCi4IndexCacheBuildCount;
    gNdsRendererProfileCi4IndexCacheReuseCount =
        sNdsRendererRuntimeCi4IndexCacheReuseCount;
    gNdsRendererProfileCi4RepresentativePixelCount =
        sNdsRendererRuntimeCi4RepresentativePixelCount;
    gNdsRendererProfileCi4ReusePixelCount =
        sNdsRendererRuntimeCi4ReusePixelCount;
    gNdsRendererProfileTextureCacheAliasAvoidCount =
        sNdsRendererRuntimeFrameSummary.texture_cache_alias_avoid_count;
    gNdsRendererProfileTextureLookupCallCount =
        sNdsRendererRuntimeFrameSummary.texture_lookup_call_count;
    gNdsRendererProfileTextureLookupProbeCount =
        sNdsRendererRuntimeFrameSummary.texture_lookup_probe_count;
    gNdsRendererProfileTextureLookupActiveHitCount =
        sNdsRendererRuntimeFrameSummary.texture_lookup_active_hit_count;
    gNdsRendererProfileTextureLookupTableHitCount =
        sNdsRendererRuntimeFrameSummary.texture_lookup_table_hit_count;
    gNdsRendererProfileTextureLookupMissCount =
        sNdsRendererRuntimeFrameSummary.texture_lookup_miss_count;
    gNdsRendererProfileTexel1CompositeCount =
        sNdsRendererRuntimeFrameSummary.texel1_composite_count;
    gNdsRendererProfileTexel1LoadMatchCount =
        sNdsRendererRuntimeFrameSummary.texel1_load_match_count;
    gNdsRendererProfileTexel1RejectCount =
        sNdsRendererRuntimeFrameSummary.texel1_reject_count;
    gNdsRendererProfileTexel1FractionRefreshCount =
        sNdsRendererRuntimeTexel1FractionRefreshCount;
    gNdsRendererProfileTextureCacheEvictCount =
        sNdsRendererRuntimeTextureCacheEvictCount;
    gNdsRendererProfileProjectedSubmitFallbackCount =
        sNdsRendererRuntimeFrameSummary.projected_submit_fallback_count;
    gNdsRendererProfileMatrixLoadCount =
        sNdsRendererRuntimeFrameSummary.matrix_load_count;
    gNdsRendererProfileHardwareVertices =
        sNdsRendererRuntimeFrameSummary.hardware_vertices;
    gNdsRendererProfileHardwareTriangles =
        sNdsRendererRuntimeFrameSummary.hardware_triangles;
    gNdsRendererProfileHardwareBatchBeginCount =
        sNdsRendererRuntimeFrameSummary.hardware_batch_begin_count;
    gNdsRendererProfileHardwareBatchReuseCount =
        sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count;
    gNdsRendererProfileHardwareBatchEndCount =
        sNdsRendererRuntimeFrameSummary.hardware_batch_end_count;
    gNdsRendererProfileTexturePrepareCount =
        sNdsRendererRuntimeFrameSummary.texture_prepare_count;
    gNdsRendererProfileTexturePrepareReuseCount =
        sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count;
    gNdsRendererProfileHardwareOverLimit =
        sNdsRendererRuntimeFrameSummary.hardware_over_limit;
    gNdsRendererProfileHWVertexSaturateCount =
        sNdsRendererRuntimeFrameSummary.hardware_vertex_saturate_count;
    gNdsRendererProfileNearPlaneTriangleRejectCount =
        sNdsRendererRuntimeFrameSummary.near_plane_triangle_reject_count;
    gNdsRendererProfileRawCurrentCandidateCount =
        sNdsRendererRuntimeFrameSummary.raw_current_candidate_count;
    gNdsRendererProfileRawCurrentRangeRejectCount =
        sNdsRendererRuntimeFrameSummary.raw_current_range_reject_count;
    gNdsRendererProfileRawCrossMatrixCount =
        sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count;
    gNdsRendererProfileOracleSamples = 0u;
    gNdsRendererProfileOracleMismatches = 0u;
    gNdsRendererProfileOracleMaxDelta = 0u;
#endif
}

u32 ndsRendererHardwareConsumeSubmittedFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 submitted;

    ndsRendererHardwareEmitIFCommonClouds();
    submitted = sNdsRendererHardwareSubmitted;
    ndsRendererHardwareEndBatch();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererHardwareRunRawMatrixPosTests();
#endif
    if ((submitted != FALSE) ||
        (sNdsRendererHardwareSubmitClassCounts[
             NDS_RENDERER_HW_SUBMIT_REJECT] != 0u))
    {
        /* The accelerated mode-163 proof does not use the realtime presentation
         * wrapper. Publish at the renderer-owned hardware-frame boundary so
         * every configuration reports the same completed-frame contract. */
        ndsRendererProfilePublishSubmitSummary();
    }
    ndsRendererProfileResetSubmitSummary();
    sNdsRendererHardwareSubmitted = FALSE;
    sNdsRendererHardwareProjectedDepth =
        NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START;
    sNdsRendererHardwareProjectedBackground = TRUE;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    sNdsRendererHardwareMatrixGeneration = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererHardwareMatrixSignature = 0u;
#endif
    sNdsRendererHardwareBoundTextureName = 0;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
    sNdsRendererHardwareFrameSerial++;
    return submitted;
#else
    return FALSE;
#endif
}

void ndsRendererExecuteDisplayListWithVertexCache(
    const Gfx *dl,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
    NDSRendererTraversalState state;
    NDSRendererTraversalVertexStorage vertex_storage;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererMatrixSnapshot
        local_matrix_snapshots[NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY];
    NDSRendererMatrixSnapshot *matrix_snapshots = local_matrix_snapshots;
#else
    NDSRendererMatrixSnapshot *matrix_snapshots = NULL;
#endif
    u32 matrix_snapshot_count = 0u;

    if (stats == NULL)
    {
        return;
    }

    if (config == NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }

#if NDS_RENDERER_HW_TRIANGLES
    if (vertex_cache != NULL)
    {
        matrix_snapshots = vertex_cache->matrix_snapshots;
        matrix_snapshot_count = vertex_cache->matrix_snapshot_count;
    }
#endif
    ndsRendererInitTraversalState(&state, config, stats, &vertex_storage,
                                  matrix_snapshots, matrix_snapshot_count);
    if (vertex_cache != NULL)
    {
        /* BattleShip submits the selected lists through one persistent RSP
         * stream. Back traversal directly with that stream instead of
         * clearing and copying every 32-slot plane around each list. */
        state.vertices = vertex_cache->transformed_vertices;
        state.vertex_valid_mask = vertex_cache->transformed_valid_mask;
#if NDS_RENDERER_HW_TRIANGLES
        state.input_vertices = vertex_cache->input_vertices;
        state.input_vertex_valid_mask = vertex_cache->input_valid_mask;
        state.raw_vertex_fit_mask = vertex_cache->raw_vertex_fit_mask;
        state.vertex_colors = vertex_cache->vertex_colors;
        state.vertex_color_valid_mask =
            vertex_cache->vertex_color_valid_mask;
        state.vertex_matrix_snapshot = vertex_cache->vertex_matrix_snapshot;
        state.vertex_clip_snapshot = vertex_cache->vertex_clip_snapshot;
#endif
    }
    ndsRendererScanList(dl, config, stats, &state, 0, callback,
                        callback_user);
    if (vertex_cache != NULL)
    {
        vertex_cache->transformed_valid_mask = state.vertex_valid_mask;
#if NDS_RENDERER_HW_TRIANGLES
        vertex_cache->input_valid_mask = state.input_vertex_valid_mask;
        vertex_cache->raw_vertex_fit_mask = state.raw_vertex_fit_mask;
        vertex_cache->vertex_color_valid_mask =
            state.vertex_color_valid_mask;
        vertex_cache->matrix_snapshot_count = state.matrix_snapshot_count;
#endif
    }
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererHardwareEndBatch();
#endif
    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (stats->unsupported_command_count != 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return;
    }
    if (stats->end_command_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_END;
        return;
    }
}

void ndsRendererExecuteDisplayList(const Gfx *dl,
                                   const NDSRendererConfig *config,
                                   NDSRendererCommandCallback callback,
                                   void *callback_user,
                                   NDSRendererStats *stats)
{
    ndsRendererExecuteDisplayListWithVertexCache(dl, config, callback,
                                                 callback_user, stats, NULL);
}
