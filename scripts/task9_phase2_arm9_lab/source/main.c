#include <nds.h>
#include <stdint.h>

enum
{
    TASK9_FCMPEQ_RUNNING = 0x46395152u,
    TASK9_FCMPEQ_PASS = 0x46395150u,
    TASK9_FCMPEQ_FAIL = 0x46395146u
};

extern uint32_t __aeabi_fcmpeq(uint32_t left, uint32_t right);
extern uint32_t __nds_task9_libgcc_fcmpeq_golden(
    uint32_t left, uint32_t right);

volatile uint32_t gTask9FcmpeqArm9Result;
volatile uint32_t gTask9FcmpeqArm9ValueCount;
volatile uint32_t gTask9FcmpeqArm9PairCount;
volatile uint32_t gTask9FcmpeqArm9MismatchCount;
volatile uint32_t gTask9FcmpeqArm9FirstLeft;
volatile uint32_t gTask9FcmpeqArm9FirstRight;
volatile uint32_t gTask9FcmpeqArm9FirstCandidate;
volatile uint32_t gTask9FcmpeqArm9FirstGolden;

static const uint32_t sTask9FcmpeqDirectedValues[] = {
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

__attribute__((noinline)) void task9Phase2Arm9ProofHalt(void)
{
    for (;;)
    {
        swiWaitForVBlank();
    }
}

int main(void)
{
    const uint32_t count = (uint32_t)(sizeof(sTask9FcmpeqDirectedValues) /
                                      sizeof(sTask9FcmpeqDirectedValues[0]));
    uint32_t left_index;
    uint32_t right_index;

    gTask9FcmpeqArm9Result = TASK9_FCMPEQ_RUNNING;
    gTask9FcmpeqArm9ValueCount = count;

    for (left_index = 0; left_index < count; ++left_index)
    {
        for (right_index = 0; right_index < count; ++right_index)
        {
            const uint32_t left = sTask9FcmpeqDirectedValues[left_index];
            const uint32_t right = sTask9FcmpeqDirectedValues[right_index];
            const uint32_t candidate = __aeabi_fcmpeq(left, right);
            const uint32_t golden =
                __nds_task9_libgcc_fcmpeq_golden(left, right);

            ++gTask9FcmpeqArm9PairCount;
            if ((candidate != golden) || (candidate > 1u) || (golden > 1u))
            {
                if (gTask9FcmpeqArm9MismatchCount++ == 0u)
                {
                    gTask9FcmpeqArm9FirstLeft = left;
                    gTask9FcmpeqArm9FirstRight = right;
                    gTask9FcmpeqArm9FirstCandidate = candidate;
                    gTask9FcmpeqArm9FirstGolden = golden;
                }
            }
        }
    }

    gTask9FcmpeqArm9Result =
        (gTask9FcmpeqArm9MismatchCount == 0u) ?
            TASK9_FCMPEQ_PASS : TASK9_FCMPEQ_FAIL;
    task9Phase2Arm9ProofHalt();
}
