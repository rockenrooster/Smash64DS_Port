#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    NAN_THRESHOLD = UINT32_C(0xff000000),
    MANTISSA_MASK = UINT32_C(0x007fffff),
    VALUE_CAPACITY = 4096
};

static unsigned selectedLibgccGolden(uint32_t left, uint32_t right)
{
    const uint32_t left_shift = left << 1;
    const uint32_t right_shift = right << 1;

    /* Literal translation of the selected _arm_cmpsf2.o NaN tests. */
    if ((((left_shift >> 24) == UINT32_C(0xff)) &&
         ((left << 9) != 0)) ||
        (((right_shift >> 24) == UINT32_C(0xff)) &&
         ((right << 9) != 0)))
    {
        return 0;
    }

    return (left == right) ||
           ((left_shift | (right_shift >> 1)) == 0);
}

static unsigned hostIeeeGolden(uint32_t left, uint32_t right)
{
    float left_float;
    float right_float;

    memcpy(&left_float, &left, sizeof(left_float));
    memcpy(&right_float, &right, sizeof(right_float));
    return left_float == right_float;
}

static unsigned candidateModel(uint32_t left, uint32_t right)
{
    const uint32_t left_shift = left << 1;
    const uint32_t right_shift = right << 1;

    return (left_shift <= NAN_THRESHOLD) &&
           (right_shift <= NAN_THRESHOLD) &&
           ((left == right) || ((left_shift | right_shift) == 0));
}

static void checkPair(uint32_t left, uint32_t right, uint64_t index)
{
    const unsigned libgcc = selectedLibgccGolden(left, right);
    const unsigned host = hostIeeeGolden(left, right);
    const unsigned candidate = candidateModel(left, right);

    if ((libgcc != host) || (libgcc != candidate))
    {
        fprintf(stderr,
                "mismatch index=%" PRIu64 " left=%08" PRIx32
                " right=%08" PRIx32 " libgcc=%u host=%u candidate=%u\n",
                index, left, right, libgcc, host, candidate);
        exit(EXIT_FAILURE);
    }
}

static uint64_t nextRandom(uint64_t *state)
{
    uint64_t value = *state;

    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;
    *state = value;
    return value * UINT64_C(2685821657736338717);
}

static size_t buildDirectedValues(uint32_t values[VALUE_CAPACITY])
{
    static const uint32_t mantissas[] = {
        0, 1, 2, UINT32_C(0x003fffff), UINT32_C(0x00400000),
        UINT32_C(0x00400001), MANTISSA_MASK - 1, MANTISSA_MASK
    };
    size_t count = 0;
    unsigned sign;
    unsigned exponent;
    size_t mantissa;

    for (sign = 0; sign < 2; ++sign)
    {
        for (exponent = 0; exponent < 256; ++exponent)
        {
            for (mantissa = 0;
                 mantissa < sizeof(mantissas) / sizeof(mantissas[0]);
                 ++mantissa)
            {
                if (count == VALUE_CAPACITY)
                {
                    fputs("directed value capacity exhausted\n", stderr);
                    exit(EXIT_FAILURE);
                }
                values[count++] = ((uint32_t)sign << 31) |
                                  ((uint32_t)exponent << 23) |
                                  mantissas[mantissa];
            }
        }
    }
    return count;
}

int main(int argc, char **argv)
{
    uint32_t values[VALUE_CAPACITY];
    uint64_t random_count = UINT64_C(100000000);
    uint64_t state = UINT64_C(0x9e3779b97f4a7c15);
    uint64_t index = 0;
    size_t value_count;
    size_t left;
    size_t right;

    if (argc > 1)
    {
        random_count = strtoull(argv[1], NULL, 0);
    }
    if (argc > 2)
    {
        state = strtoull(argv[2], NULL, 0);
    }
    if (state == 0)
    {
        fputs("seed must be nonzero\n", stderr);
        return EXIT_FAILURE;
    }

    value_count = buildDirectedValues(values);
    for (left = 0; left < value_count; ++left)
    {
        for (right = 0; right < value_count; ++right)
        {
            checkPair(values[left], values[right], index++);
        }
    }

    for (index = 0; index < random_count; ++index)
    {
        const uint64_t bits = nextRandom(&state);
        checkPair((uint32_t)bits, (uint32_t)(bits >> 32), index);
    }

    printf("PASS directed=%" PRIu64 " random=%" PRIu64
           " final_seed=%016" PRIx64 "\n",
           (uint64_t)value_count * (uint64_t)value_count,
           random_count, state);
    return EXIT_SUCCESS;
}
