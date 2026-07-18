#include <nds.h>
#include <stdio.h>

#include <nds/nds_task10_hardware_calibration.h>

#define NDS_TASK10_ALU_ITERATIONS 1000000u
#define NDS_TASK10_ALU_ADDS_PER_ITERATION 32u
#define NDS_TASK10_MAIN_RAM_WORDS (256u * 1024u / sizeof(u32))
#define NDS_TASK10_CACHE_WORDS (4u * 1024u / sizeof(u32))
#define NDS_TASK10_TOTAL_LOADS (8u * 1024u * 1024u)
#define NDS_TASK10_GX_TRIANGLES 10000u
#define NDS_TASK10_GX_SWAP_TRIANGLES 2048u

enum
{
    NDS_TASK10_RESULT_ALU_ITCM,
    NDS_TASK10_RESULT_MEM_THUMB,
    NDS_TASK10_RESULT_MEM_ARM,
    NDS_TASK10_RESULT_CACHE4K,
    NDS_TASK10_RESULT_GX_BURST,
    NDS_TASK10_RESULT_COUNT
};

static u32 sNdsTask10MainRamBuffer[NDS_TASK10_MAIN_RAM_WORDS]
    __attribute__((aligned(32)));

volatile u32 gNdsTask10HardwareCalibrationResults[NDS_TASK10_RESULT_COUNT];
volatile u32 gNdsTask10HardwareCalibrationSink;
volatile u32 gNdsTask10HardwareCalibrationComplete;

static u32 __attribute__((noinline, used, hot, optimize("O3"),
                          target("arm"), section(".itcm")))
ndsTask10DependentAddItcm(u32 value)
{
    u32 iterations = NDS_TASK10_ALU_ITERATIONS;

    __asm__ volatile(
        "1:\n"
        ".rept 32\n"
        "add %[value], %[value], #1\n"
        ".endr\n"
        "subs %[iterations], %[iterations], #1\n"
        "bne 1b\n"
        : [value] "+r" (value), [iterations] "+r" (iterations)
        :
        : "cc");
    return value;
}

#define NDS_TASK10_DEFINE_SUM(name, attributes) \
static u32 attributes name(const volatile u32 *words, u32 word_count, \
                           u32 passes) \
{ \
    u32 pass; \
    u32 sum = 0u; \
    for (pass = 0u; pass < passes; pass++) \
    { \
        u32 index; \
        for (index = 0u; index < word_count; index++) \
        { \
            sum += words[index]; \
        } \
    } \
    return sum; \
}

NDS_TASK10_DEFINE_SUM(ndsTask10MemoryThumb,
                      __attribute__((noinline, used, optimize("O2"))))
NDS_TASK10_DEFINE_SUM(ndsTask10MemoryArm,
                      __attribute__((noinline, used, optimize("O2"),
                                     target("arm"))))

static u32 ndsTask10GxBurst(void)
{
    const u32 xy0 = (u32)(u16)-2048 | ((u32)(u16)-2048 << 16);
    const u32 xy1 = (u32)(u16)2048 | ((u32)(u16)-2048 << 16);
    const u32 xy2 = (u32)(u16)0 | ((u32)(u16)2048 << 16);
    u32 triangle;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(1));
    GFX_COLOR = RGB15(31, 31, 31);
    GFX_BEGIN = GL_TRIANGLE;

    for (triangle = 0u; triangle < NDS_TASK10_GX_TRIANGLES; triangle++)
    {
        GFX_VERTEX16 = xy0;
        GFX_VERTEX16 = 0u;
        GFX_VERTEX16 = xy1;
        GFX_VERTEX16 = 0u;
        GFX_VERTEX16 = xy2;
        GFX_VERTEX16 = 0u;

        if ((((triangle + 1u) % NDS_TASK10_GX_SWAP_TRIANGLES) == 0u) ||
            ((triangle + 1u) == NDS_TASK10_GX_TRIANGLES))
        {
            glFlush(GL_TRANS_MANUALSORT);
            swiWaitForVBlank();
            while (GFX_BUSY)
            {
            }
        }
    }
    return GFX_STATUS ^ GFX_POLYGON_RAM_USAGE ^ GFX_VERTEX_RAM_USAGE;
}

static u32 ndsTask10Measure(u32 (*bench)(void), u32 *result)
{
    u32 start = cpuGetTiming();

    *result = bench();
    return cpuGetTiming() - start;
}

static u32 ndsTask10RunAlu(void)
{
    return ndsTask10DependentAddItcm(0x12345678u);
}

static u32 ndsTask10RunMemoryThumb(void)
{
    return ndsTask10MemoryThumb(
        sNdsTask10MainRamBuffer, NDS_TASK10_MAIN_RAM_WORDS,
        NDS_TASK10_TOTAL_LOADS / NDS_TASK10_MAIN_RAM_WORDS);
}

static u32 ndsTask10RunMemoryArm(void)
{
    return ndsTask10MemoryArm(
        sNdsTask10MainRamBuffer, NDS_TASK10_MAIN_RAM_WORDS,
        NDS_TASK10_TOTAL_LOADS / NDS_TASK10_MAIN_RAM_WORDS);
}

static u32 ndsTask10RunCache4K(void)
{
    return ndsTask10MemoryThumb(
        sNdsTask10MainRamBuffer, NDS_TASK10_CACHE_WORDS,
        NDS_TASK10_TOTAL_LOADS / NDS_TASK10_CACHE_WORDS);
}

void __attribute__((noinline, used))
ndsTask10HardwareCalibrationCompleteMarker(void)
{
    __asm__ volatile("" ::: "memory");
}

void ndsTask10HardwareCalibrationRun(void)
{
    static const char *const labels[NDS_TASK10_RESULT_COUNT] = {
        "ALU-ITCM", "MEM-THMB", "MEM-ARM", "CACHE4K", "GX-BRST"
    };
    u32 index;
    u32 result;

    consoleClear();
    iprintf("TASK10 HW CAL\n");
    iprintf("GIT %s\n\n", NDS_TASK10_GIT_SHORT);
    iprintf("BENCH       TICKS\n");

    for (index = 0u; index < NDS_TASK10_MAIN_RAM_WORDS; index++)
    {
        sNdsTask10MainRamBuffer[index] =
            (index * 2654435761u) ^ 0x9e3779b9u;
    }
    (void)ndsTask10MemoryThumb(
        sNdsTask10MainRamBuffer, NDS_TASK10_CACHE_WORDS, 1u);

    gNdsTask10HardwareCalibrationResults[NDS_TASK10_RESULT_ALU_ITCM] =
        ndsTask10Measure(ndsTask10RunAlu, &result);
    gNdsTask10HardwareCalibrationSink ^= result;
    gNdsTask10HardwareCalibrationResults[NDS_TASK10_RESULT_MEM_THUMB] =
        ndsTask10Measure(ndsTask10RunMemoryThumb, &result);
    gNdsTask10HardwareCalibrationSink ^= result;
    gNdsTask10HardwareCalibrationResults[NDS_TASK10_RESULT_MEM_ARM] =
        ndsTask10Measure(ndsTask10RunMemoryArm, &result);
    gNdsTask10HardwareCalibrationSink ^= result;
    gNdsTask10HardwareCalibrationResults[NDS_TASK10_RESULT_CACHE4K] =
        ndsTask10Measure(ndsTask10RunCache4K, &result);
    gNdsTask10HardwareCalibrationSink ^= result;
    gNdsTask10HardwareCalibrationResults[NDS_TASK10_RESULT_GX_BURST] =
        ndsTask10Measure(ndsTask10GxBurst, &result);
    gNdsTask10HardwareCalibrationSink ^= result;

    for (index = 0u; index < NDS_TASK10_RESULT_COUNT; index++)
    {
        iprintf("%-8s %10lu\n", labels[index],
                (unsigned long)gNdsTask10HardwareCalibrationResults[index]);
    }
    iprintf("CARD            TBD\n\n");
    iprintf("ALU 1Mx32 dependent ADD\n");
    iprintf("MEM 8M loads / 256 KiB\n");
    iprintf("C4K 8M loads / 4 KiB\n");
    iprintf("GX  10000 tri / swap 2048\n");
    iprintf("\nCOMPLETE - photograph this\n");

    gNdsTask10HardwareCalibrationComplete = 0x50415353u;
    while (1)
    {
        ndsTask10HardwareCalibrationCompleteMarker();
        swiWaitForVBlank();
    }
}
