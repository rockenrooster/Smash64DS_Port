#include <nds.h>
#include <stdint.h>

enum
{
    TASK16_ADDSUB_RUNNING = 0x41313652u,
    TASK16_ADDSUB_PASS = 0x41313650u,
    TASK16_ADDSUB_FAIL = 0x41313646u
};

#ifndef TASK16_ADDSUB_RANDOM_PAIRS
#define TASK16_ADDSUB_RANDOM_PAIRS 100000000u
#endif

extern uint32_t __aeabi_fadd(uint32_t left, uint32_t right);
extern uint32_t __aeabi_fsub(uint32_t left, uint32_t right);
extern uint32_t __nds_task16_libgcc_fadd_golden(uint32_t left,
                                                uint32_t right);
extern uint32_t __nds_task16_libgcc_fsub_golden(uint32_t left,
                                                uint32_t right);

volatile uint32_t gTask16AddSubArm9Result;
volatile uint32_t gTask16AddSubArm9ValueCount;
volatile uint32_t gTask16AddSubArm9PairCount;
volatile uint32_t gTask16AddSubArm9DirectedOperationCount;
volatile uint32_t gTask16AddSubArm9RandomTarget;
volatile uint32_t gTask16AddSubArm9RandomCompleted;
volatile uint32_t gTask16AddSubArm9MismatchCount;
volatile uint32_t gTask16AddSubArm9FirstOperation;
volatile uint32_t gTask16AddSubArm9FirstLeft;
volatile uint32_t gTask16AddSubArm9FirstRight;
volatile uint32_t gTask16AddSubArm9FirstCandidate;
volatile uint32_t gTask16AddSubArm9FirstGolden;

static const uint32_t sTask16AddSubDirectedValues[] = {
    0x00000000u, 0x80000000u,
    0x00000001u, 0x00000002u, 0x003fffffu, 0x00400000u,
    0x007ffffeu, 0x007fffffu,
    0x80000001u, 0x80000002u, 0x803fffffu, 0x80400000u,
    0x807ffffeu, 0x807fffffu,
    0x00800000u, 0x00800001u, 0x80800000u, 0x80800001u,
    0x3effffffu, 0x3f000000u, 0x3f000001u,
    0xbeffffffu, 0xbf000000u, 0xbf000001u,
    0x3f7fffffu, 0x3f800000u, 0x3f800001u,
    0xbf7fffffu, 0xbf800000u, 0xbf800001u,
    0x3fffffffu, 0x40000000u, 0x40000001u,
    0xbfffffffu, 0xc0000000u, 0xc0000001u,
    0x7f000000u, 0x7f7ffffeu, 0x7f7fffffu,
    0xff000000u, 0xff7ffffeu, 0xff7fffffu,
    0x7f800000u, 0xff800000u,
    0x7f800001u, 0x7fbfffffu, 0x7fc00000u, 0x7fc00001u,
    0x7fffffffu,
    0xff800001u, 0xffbfffffu, 0xffc00000u, 0xffc00001u,
    0xffffffffu
};

__attribute__((noinline)) void task16AddSubArm9ProofHalt(void)
{
    for (;;)
    {
        swiWaitForVBlank();
    }
}

static void task16AddSubCheck(uint32_t operation, uint32_t left,
                              uint32_t right, uint32_t candidate,
                              uint32_t golden)
{
    if ((candidate != golden) && (gTask16AddSubArm9MismatchCount++ == 0u))
    {
        gTask16AddSubArm9FirstOperation = operation;
        gTask16AddSubArm9FirstLeft = left;
        gTask16AddSubArm9FirstRight = right;
        gTask16AddSubArm9FirstCandidate = candidate;
        gTask16AddSubArm9FirstGolden = golden;
    }
}

typedef struct Task16AddSubRandom
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t w;
} Task16AddSubRandom;

static uint32_t task16AddSubNextRandom(Task16AddSubRandom *random)
{
    const uint32_t mixed = random->x ^ (random->x << 11);

    random->x = random->y;
    random->y = random->z;
    random->z = random->w;
    random->w = random->w ^ (random->w >> 19) ^
                mixed ^ (mixed >> 8);
    return random->w;
}

int main(void)
{
    const uint32_t count = (uint32_t)(sizeof(sTask16AddSubDirectedValues) /
                                      sizeof(sTask16AddSubDirectedValues[0]));
    uint32_t left_index;
    uint32_t right_index;
    uint32_t random_index;
    Task16AddSubRandom random = {
        0x9e3779b9u, 0x243f6a88u, 0xb7e15162u, 0x8aed2a6bu
    };

    gTask16AddSubArm9Result = TASK16_ADDSUB_RUNNING;
    gTask16AddSubArm9ValueCount = count;
    for (left_index = 0; left_index < count; ++left_index)
    {
        for (right_index = 0; right_index < count; ++right_index)
        {
            const uint32_t left = sTask16AddSubDirectedValues[left_index];
            const uint32_t right = sTask16AddSubDirectedValues[right_index];

            task16AddSubCheck(0u, left, right,
                              __aeabi_fadd(left, right),
                              __nds_task16_libgcc_fadd_golden(left, right));
            task16AddSubCheck(1u, left, right,
                              __aeabi_fsub(left, right),
                              __nds_task16_libgcc_fsub_golden(left, right));
        }
    }
    gTask16AddSubArm9DirectedOperationCount = 2u * count * count;
    gTask16AddSubArm9PairCount = gTask16AddSubArm9DirectedOperationCount;
    gTask16AddSubArm9RandomTarget = TASK16_ADDSUB_RANDOM_PAIRS;
    for (random_index = 0u;
         random_index < TASK16_ADDSUB_RANDOM_PAIRS;
         ++random_index)
    {
        const uint32_t random_left = task16AddSubNextRandom(&random);
        const uint32_t random_right = task16AddSubNextRandom(&random);
        task16AddSubCheck(0u, random_left, random_right,
                          __aeabi_fadd(random_left, random_right),
                          __nds_task16_libgcc_fadd_golden(random_left,
                                                         random_right));
        task16AddSubCheck(1u, random_left, random_right,
                          __aeabi_fsub(random_left, random_right),
                          __nds_task16_libgcc_fsub_golden(random_left,
                                                         random_right));
    }
    gTask16AddSubArm9RandomCompleted = TASK16_ADDSUB_RANDOM_PAIRS;
    gTask16AddSubArm9PairCount += 2u * TASK16_ADDSUB_RANDOM_PAIRS;
    gTask16AddSubArm9Result =
        (gTask16AddSubArm9MismatchCount == 0u) ?
            TASK16_ADDSUB_PASS : TASK16_ADDSUB_FAIL;
    task16AddSubArm9ProofHalt();
}
