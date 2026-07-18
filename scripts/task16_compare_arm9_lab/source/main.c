#include <nds.h>
#include <stdint.h>

enum
{
    TASK16_COMPARE_RUNNING = 0x54313652u,
    TASK16_COMPARE_PASS = 0x54313650u,
    TASK16_COMPARE_FAIL = 0x54313646u,
    TASK16_PREDICATE_COUNT = 5
};

#define DECLARE_COMPARE(name) \
    extern uint32_t __aeabi_##name(uint32_t left, uint32_t right); \
    extern uint32_t __nds_task16_libgcc_##name##_golden( \
        uint32_t left, uint32_t right)

DECLARE_COMPARE(fcmplt);
DECLARE_COMPARE(fcmple);
DECLARE_COMPARE(fcmpge);
DECLARE_COMPARE(fcmpgt);
DECLARE_COMPARE(fcmpun);

typedef uint32_t (*Task16Compare)(uint32_t left, uint32_t right);

static const Task16Compare sCandidates[TASK16_PREDICATE_COUNT] = {
    __aeabi_fcmplt, __aeabi_fcmple, __aeabi_fcmpge,
    __aeabi_fcmpgt, __aeabi_fcmpun
};
static const Task16Compare sGoldens[TASK16_PREDICATE_COUNT] = {
    __nds_task16_libgcc_fcmplt_golden,
    __nds_task16_libgcc_fcmple_golden,
    __nds_task16_libgcc_fcmpge_golden,
    __nds_task16_libgcc_fcmpgt_golden,
    __nds_task16_libgcc_fcmpun_golden
};

volatile uint32_t gTask16CompareArm9Result;
volatile uint32_t gTask16CompareArm9ValueCount;
volatile uint32_t gTask16CompareArm9PairCount;
volatile uint32_t gTask16CompareArm9CallCount;
volatile uint32_t gTask16CompareArm9MismatchCount;
volatile uint32_t gTask16CompareArm9FirstPredicate;
volatile uint32_t gTask16CompareArm9FirstLeft;
volatile uint32_t gTask16CompareArm9FirstRight;
volatile uint32_t gTask16CompareArm9FirstCandidate;
volatile uint32_t gTask16CompareArm9FirstGolden;

static const uint32_t sTask16DirectedValues[] = {
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

__attribute__((noinline)) void task16CompareArm9ProofHalt(void)
{
    for (;;)
    {
        swiWaitForVBlank();
    }
}

int main(void)
{
    const uint32_t count = (uint32_t)(sizeof(sTask16DirectedValues) /
                                      sizeof(sTask16DirectedValues[0]));
    uint32_t left_index;
    uint32_t right_index;

    gTask16CompareArm9Result = TASK16_COMPARE_RUNNING;
    gTask16CompareArm9ValueCount = count;
    for (left_index = 0; left_index < count; ++left_index)
    {
        for (right_index = 0; right_index < count; ++right_index)
        {
            const uint32_t left = sTask16DirectedValues[left_index];
            const uint32_t right = sTask16DirectedValues[right_index];
            uint32_t predicate;

            ++gTask16CompareArm9PairCount;
            for (predicate = 0; predicate < TASK16_PREDICATE_COUNT;
                 ++predicate)
            {
                const uint32_t candidate = sCandidates[predicate](left, right);
                const uint32_t golden = sGoldens[predicate](left, right);

                ++gTask16CompareArm9CallCount;
                if ((candidate != golden) || (candidate > 1u) || (golden > 1u))
                {
                    if (gTask16CompareArm9MismatchCount++ == 0u)
                    {
                        gTask16CompareArm9FirstPredicate = predicate;
                        gTask16CompareArm9FirstLeft = left;
                        gTask16CompareArm9FirstRight = right;
                        gTask16CompareArm9FirstCandidate = candidate;
                        gTask16CompareArm9FirstGolden = golden;
                    }
                }
            }
        }
    }

    gTask16CompareArm9Result =
        (gTask16CompareArm9MismatchCount == 0u) ?
            TASK16_COMPARE_PASS : TASK16_COMPARE_FAIL;
    task16CompareArm9ProofHalt();
}
