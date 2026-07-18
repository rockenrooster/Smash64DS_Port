#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    NAN_THRESHOLD = UINT32_C(0xff000000),
    MANTISSA_MASK = UINT32_C(0x007fffff),
    VALUE_CAPACITY = 4096,
    PREDICATE_COUNT = 5
};

static const char *const sPredicateNames[PREDICATE_COUNT] = {
    "fcmplt", "fcmple", "fcmpge", "fcmpgt", "fcmpun"
};

static int selectedLibgccCompare(uint32_t left, uint32_t right)
{
    const uint32_t left_abs = left << 1;
    const uint32_t right_abs = right << 1;

    if ((left_abs > NAN_THRESHOLD) || (right_abs > NAN_THRESHOLD))
    {
        return 2;
    }
    if ((left == right) || ((left_abs | right_abs) == 0))
    {
        return 0;
    }
    if ((left ^ right) >> 31)
    {
        return (left >> 31) ? -1 : 1;
    }
    if (left >> 31)
    {
        return (left > right) ? -1 : 1;
    }
    return (left < right) ? -1 : 1;
}

static unsigned selectedLibgccGolden(
    unsigned predicate, uint32_t left, uint32_t right)
{
    const int comparison = selectedLibgccCompare(left, right);

    switch (predicate)
    {
    case 0: return comparison == -1;
    case 1: return comparison <= 0;
    case 2: return (comparison != 2) && (comparison >= 0);
    case 3: return comparison == 1;
    default: return comparison == 2;
    }
}

static unsigned hostIeeeGolden(
    unsigned predicate, uint32_t left, uint32_t right)
{
    float left_float;
    float right_float;

    memcpy(&left_float, &left, sizeof(left_float));
    memcpy(&right_float, &right, sizeof(right_float));
    switch (predicate)
    {
    case 0: return left_float < right_float;
    case 1: return left_float <= right_float;
    case 2: return left_float >= right_float;
    case 3: return left_float > right_float;
    default:
        return (left_float != left_float) || (right_float != right_float);
    }
}

static unsigned candidateModel(
    unsigned predicate, uint32_t left, uint32_t right)
{
    const uint32_t left_abs = left << 1;
    const uint32_t right_abs = right << 1;
    unsigned less;
    unsigned equal;

    if ((left_abs > NAN_THRESHOLD) || (right_abs > NAN_THRESHOLD))
    {
        return predicate == 4;
    }
    if (predicate == 4)
    {
        return 0;
    }

    equal = (left == right) || ((left_abs | right_abs) == 0);
    if (equal)
    {
        return (predicate == 1) || (predicate == 2);
    }
    if ((left ^ right) >> 31)
    {
        less = left >> 31;
    }
    else if (left >> 31)
    {
        less = left > right;
    }
    else
    {
        less = left < right;
    }
    switch (predicate)
    {
    case 0: return less;
    case 1: return less;
    case 2: return !less;
    default: return !less;
    }
}

static void checkPair(uint32_t left, uint32_t right, uint64_t index)
{
    unsigned predicate;

    for (predicate = 0; predicate < PREDICATE_COUNT; ++predicate)
    {
        const unsigned libgcc = selectedLibgccGolden(predicate, left, right);
        const unsigned host = hostIeeeGolden(predicate, left, right);
        const unsigned candidate = candidateModel(predicate, left, right);

        if ((libgcc != host) || (libgcc != candidate))
        {
            fprintf(stderr,
                    "mismatch predicate=%s index=%" PRIu64
                    " left=%08" PRIx32 " right=%08" PRIx32
                    " libgcc=%u host=%u candidate=%u\n",
                    sPredicateNames[predicate], index, left, right,
                    libgcc, host, candidate);
            exit(EXIT_FAILURE);
        }
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
    uint64_t state = UINT64_C(0x6a09e667f3bcc909);
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

    printf("PASS predicates=%u directed=%" PRIu64 " random=%" PRIu64
           " final_seed=%016" PRIx64 "\n",
           PREDICATE_COUNT,
           (uint64_t)value_count * (uint64_t)value_count,
           random_count, state);
    return EXIT_SUCCESS;
}
