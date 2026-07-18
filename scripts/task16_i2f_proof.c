#include <fenv.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif

static unsigned leadingBit(uint32_t value)
{
#if defined(__GNUC__)
    return 31u - (unsigned)__builtin_clz(value);
#else
    unsigned bit = 0;
    while (value >>= 1)
    {
        ++bit;
    }
    return bit;
#endif
}

static NOINLINE uint32_t selectedLibgccGolden(uint32_t input)
{
    const uint32_t sign = input & UINT32_C(0x80000000);
    const uint32_t magnitude = sign ? (UINT32_C(0) - input) : input;
    uint32_t significand;
    unsigned bit;

    if (magnitude == 0)
    {
        return 0;
    }
    bit = leadingBit(magnitude);
    if (bit <= 23u)
    {
        significand = magnitude << (23u - bit);
    }
    else
    {
        const unsigned shift = bit - 23u;
        const uint32_t halfway = UINT32_C(1) << (shift - 1u);
        const uint32_t mask = (UINT32_C(1) << shift) - UINT32_C(1);
        const uint32_t remainder = magnitude & mask;

        significand = magnitude >> shift;
        if ((remainder > halfway) ||
            ((remainder == halfway) && ((significand & 1u) != 0)))
        {
            ++significand;
        }
    }
    return sign + ((uint32_t)(bit + 126u) << 23) + significand;
}

static NOINLINE uint32_t candidateModel(uint32_t input)
{
    const uint32_t sign = input & UINT32_C(0x80000000);
    uint32_t magnitude = sign ? (UINT32_C(0) - input) : input;
    uint32_t significand;
    uint32_t discarded;
    unsigned bit;

    if (magnitude == 0)
    {
        return 0;
    }
    bit = leadingBit(magnitude);
    if (bit <= 23u)
    {
        significand = magnitude << (23u - bit);
    }
    else
    {
        const unsigned shift = bit - 23u;

        significand = magnitude >> shift;
        discarded = magnitude << (32u - shift);
        discarded |= significand & 1u;
        if (((discarded & UINT32_C(0x80000000)) != 0) &&
            ((discarded << 1) != 0))
        {
            ++significand;
        }
    }
    return sign + ((uint32_t)(bit + 126u) << 23) + significand;
}

static NOINLINE uint32_t hostIeeeGolden(uint32_t input)
{
    int32_t integer;
    float converted;
    uint32_t result;

    memcpy(&integer, &input, sizeof(integer));
    converted = (float)integer;
    memcpy(&result, &converted, sizeof(result));
    return result;
}

int main(int argc, char **argv)
{
    uint64_t start = 0;
    uint64_t count = UINT64_C(0x100000000);
    uint64_t offset;
    uint64_t checksum = 0;

    if (argc > 1)
    {
        start = strtoull(argv[1], NULL, 0);
    }
    if (argc > 2)
    {
        count = strtoull(argv[2], NULL, 0);
    }
    if ((start > UINT64_C(0xffffffff)) ||
        (count > UINT64_C(0x100000000)) ||
        ((start + count) > UINT64_C(0x100000000)))
    {
        fputs("range exceeds the uint32_t domain\n", stderr);
        return EXIT_FAILURE;
    }
    if (fesetround(FE_TONEAREST) != 0)
    {
        fputs("could not select round-to-nearest-even\n", stderr);
        return EXIT_FAILURE;
    }

    for (offset = 0; offset < count; ++offset)
    {
        const uint32_t input = (uint32_t)(start + offset);
        const uint32_t golden = selectedLibgccGolden(input);
        const uint32_t host = hostIeeeGolden(input);
        const uint32_t candidate = candidateModel(input);

        if ((golden != host) || (golden != candidate))
        {
            fprintf(stderr,
                    "mismatch input=%08" PRIx32 " libgcc=%08" PRIx32
                    " host=%08" PRIx32 " candidate=%08" PRIx32 "\n",
                    input, golden, host, candidate);
            return EXIT_FAILURE;
        }
        checksum = (checksum << 7) ^ (checksum >> 3) ^ candidate ^ input;
    }

    printf("PASS start=%" PRIu64 " count=%" PRIu64
           " checksum=%016" PRIx64 "\n", start, count, checksum);
    return EXIT_SUCCESS;
}
