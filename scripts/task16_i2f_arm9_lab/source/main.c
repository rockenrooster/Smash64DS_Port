#include <nds.h>
#include <stdint.h>

enum
{
    TASK16_I2F_RUNNING = 0x49324652u,
    TASK16_I2F_PASS = 0x49324650u,
    TASK16_I2F_FAIL = 0x49324646u,
    TASK16_I2F_RANDOM_COUNT = 262144u,
    TASK16_I2F_TIMING_COUNT = 65536u
};

extern uint32_t __aeabi_i2f(uint32_t input);
extern uint32_t __nds_task16_libgcc_i2f_golden(uint32_t input);

volatile uint32_t gTask16I2fResult;
volatile uint32_t gTask16I2fVectorCount;
volatile uint32_t gTask16I2fMismatchCount;
volatile uint32_t gTask16I2fFirstInput;
volatile uint32_t gTask16I2fFirstCandidate;
volatile uint32_t gTask16I2fFirstGolden;
volatile uint32_t gTask16I2fCandidateTicks;
volatile uint32_t gTask16I2fGoldenTicks;
volatile uint32_t gTask16I2fTimingCount;
volatile uint32_t gTask16I2fTimingSink;

static const uint32_t sTask16I2fDirected[] = {
    0x00000000u, 0x00000001u, 0x00000002u, 0x00000003u,
    0x007fffffu, 0x00800000u, 0x00800001u,
    0x00ffffffu, 0x01000000u, 0x01000001u,
    0x01fffffeu, 0x01ffffffu, 0x02000000u, 0x02000001u,
    0x3ffffffdu, 0x3ffffffeu, 0x3fffffffu, 0x40000000u,
    0x40000001u, 0x40000002u, 0x40000003u,
    0x7ffffffdu, 0x7ffffffeu, 0x7fffffffu,
    0x80000000u, 0x80000001u, 0x80000002u, 0x80000003u,
    0xbffffffdu, 0xbffffffeu, 0xbfffffffu, 0xc0000000u,
    0xc0000001u, 0xc0000002u, 0xc0000003u,
    0xffffff00u, 0xffffff7fu, 0xffffff80u, 0xfffffffeu, 0xffffffffu
};

static const uint32_t sTask16I2fTimingValues[] = {
    0u, 1u, 2u, 3u, 7u, 15u, 31u, 63u,
    127u, 255u, 511u, 1023u, 4095u, 16383u, 32767u, 65535u,
    0x000fffffu, 0x001fffffu, 0x003fffffu, 0x007fffffu,
    0x00800000u, 0x00ffffffu, 0x01000000u, 0x01ffffffu,
    0x02000000u, 0x03ffffffu, 0x07ffffffu, 0x0fffffffu,
    0x1fffffffu, 0x3fffffffu, 0x7fffffffu, 0x80000000u,
    0xffffffffu, 0xfffffffeu, 0xfffffffdu, 0xfffffff9u,
    0xfffffff1u, 0xffffffe1u, 0xffffffc1u, 0xffffff81u,
    0xffffff01u, 0xfffffe01u, 0xfffffc01u, 0xfffff001u,
    0xffffc001u, 0xffff8001u, 0xffff0001u, 0xfff00001u,
    0xffe00001u, 0xffc00001u, 0xff800001u, 0xff000001u,
    0xfe000001u, 0xfc000001u, 0xf8000001u, 0xf0000001u,
    0xe0000001u, 0xc0000001u, 0x80000003u, 0x80000002u,
    0x80000001u, 0x40000003u, 0x40000002u, 0x40000001u
};

static void task16I2fCheck(uint32_t input)
{
    const uint32_t candidate = __aeabi_i2f(input);
    const uint32_t golden = __nds_task16_libgcc_i2f_golden(input);

    ++gTask16I2fVectorCount;
    if (candidate != golden)
    {
        if (gTask16I2fMismatchCount++ == 0u)
        {
            gTask16I2fFirstInput = input;
            gTask16I2fFirstCandidate = candidate;
            gTask16I2fFirstGolden = golden;
        }
    }
}

__attribute__((noinline)) void task16I2fArm9ProofHalt(void)
{
    for (;;)
    {
        swiWaitForVBlank();
    }
}

int main(void)
{
    uint32_t index;
    uint32_t state = 0x6d2b79f5u;
    uint32_t sink = 0;

    gTask16I2fResult = TASK16_I2F_RUNNING;
    for (index = 0;
         index < (uint32_t)(sizeof(sTask16I2fDirected) /
                            sizeof(sTask16I2fDirected[0]));
         ++index)
    {
        task16I2fCheck(sTask16I2fDirected[index]);
    }
    for (index = 0; index <= 0xffffu; ++index)
    {
        task16I2fCheck(index);
        task16I2fCheck(UINT32_C(0x80000000) + index);
    }
    for (index = 0; index < TASK16_I2F_RANDOM_COUNT; ++index)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        task16I2fCheck(state);
    }

    cpuStartTiming(0);
    for (index = 0; index < TASK16_I2F_TIMING_COUNT; ++index)
    {
        sink ^= __aeabi_i2f(
            sTask16I2fTimingValues[index & 63u]);
    }
    gTask16I2fCandidateTicks = cpuGetTiming();
    cpuStartTiming(0);
    for (index = 0; index < TASK16_I2F_TIMING_COUNT; ++index)
    {
        sink ^= __nds_task16_libgcc_i2f_golden(
            sTask16I2fTimingValues[index & 63u]);
    }
    gTask16I2fGoldenTicks = cpuGetTiming();
    gTask16I2fTimingCount = TASK16_I2F_TIMING_COUNT;
    gTask16I2fTimingSink = sink;
    gTask16I2fResult = (gTask16I2fMismatchCount == 0u) ?
        TASK16_I2F_PASS : TASK16_I2F_FAIL;
    task16I2fArm9ProofHalt();
}
