#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum
{
    VALUE_CAPACITY = 4096
};

static uint32_t asr32(uint32_t value, unsigned shift)
{
    if (shift == 0u)
    {
        return value;
    }
    if (shift >= 32u)
    {
        return (value & UINT32_C(0x80000000)) ? UINT32_MAX : 0u;
    }
    return (value >> shift) |
           ((value & UINT32_C(0x80000000)) ?
                (~UINT32_C(0) << (32u - shift)) : 0u);
}

static uint32_t lsl32(uint32_t value, unsigned shift)
{
    return (shift < 32u) ? (value << shift) : 0u;
}

static uint32_t lsr32(uint32_t value, unsigned shift)
{
    return (shift < 32u) ? (value >> shift) : 0u;
}

static uint32_t addSubModel(uint32_t left, uint32_t right,
                            unsigned subtract, unsigned use_clz)
{
    uint32_t r0 = left;
    uint32_t r1 = right ^ (subtract ? UINT32_C(0x80000000) : 0u);
    uint32_t r2 = r0 << 1;
    uint32_t r3 = r1 << 1;
    int32_t exponent;

    if ((r2 == 0u) || (r3 == 0u) || (r2 == r3) ||
        ((r2 >> 24) == 0xffu) || ((r3 >> 24) == 0xffu))
    {
        if (((r2 >> 24) == 0xffu) || ((r3 >> 24) == 0xffu))
        {
            if ((r2 >> 24) != 0xffu)
            {
                r0 = r1;
            }
            else if ((r3 >> 24) != 0xffu)
            {
                r1 = r0;
            }
            if (((r0 << 9) != 0u) || ((r1 << 9) != 0u) ||
                (r0 != r1))
            {
                r0 |= UINT32_C(0x00400000);
            }
            return r0;
        }
        if (r2 != r3)
        {
            return (r2 == 0u) ? r1 : r0;
        }
        if (r0 != r1)
        {
            return 0u;
        }
        if ((r2 & UINT32_C(0xff000000)) == 0u)
        {
            const uint32_t carry = r0 >> 31;
            r0 <<= 1;
            if (carry != 0u)
            {
                r0 |= UINT32_C(0x80000000);
            }
            return r0;
        }
        {
            const uint64_t doubled =
                (uint64_t)r2 + UINT32_C(0x02000000);
            if (doubled <= UINT32_MAX)
            {
                return r0 + UINT32_C(0x00800000);
            }
            return (r0 & UINT32_C(0x80000000)) |
                   UINT32_C(0x7f800000);
        }
    }

    {
        int32_t delta = (int32_t)(r3 >> 24) - (int32_t)(r2 >> 24);

        exponent = (int32_t)(r2 >> 24);
        if (delta > 0)
        {
            const uint32_t swap = r0;
            exponent += delta;
            r0 = r1;
            r1 = swap;
        }
        else if (delta < 0)
        {
            delta = -delta;
        }
        if (delta > 25)
        {
            return r0;
        }
        {
            const uint32_t r0_negative = r0 & UINT32_C(0x80000000);
            const uint32_t r1_negative = r1 & UINT32_C(0x80000000);

            r0 = (r0 | UINT32_C(0x00800000)) & UINT32_C(0x00ffffff);
            if (r0_negative != 0u)
            {
                r0 = 0u - r0;
            }
            r1 = (r1 | UINT32_C(0x00800000)) & UINT32_C(0x00ffffff);
            if (r1_negative != 0u)
            {
                r1 = 0u - r1;
            }
        }

        if (exponent == delta)
        {
            r1 ^= UINT32_C(0x00800000);
            if (exponent == 0)
            {
                r0 ^= UINT32_C(0x00800000);
                ++exponent;
            }
            else
            {
                --delta;
            }
        }
        --exponent;
        r0 += asr32(r1, (unsigned)delta);
        r1 = lsl32(r1, (unsigned)(32 - delta));
    }

    r3 = r0 & UINT32_C(0x80000000);
    if ((r0 & UINT32_C(0x80000000)) != 0u)
    {
        const uint64_t magnitude =
            UINT64_C(0) - (((uint64_t)r0 << 32) | r1);
        r0 = (uint32_t)(magnitude >> 32);
        r1 = (uint32_t)magnitude;
    }
    if (r0 >= UINT32_C(0x00800000))
    {
        if (r0 >= UINT32_C(0x01000000))
        {
            r1 = (r1 >> 1) | (r0 << 31);
            r0 >>= 1;
            if (++exponent >= 254)
            {
                return r3 | UINT32_C(0x7f800000);
            }
        }
    }
    else
    {
        r0 = (r0 << 1) | (r1 >> 31);
        r1 <<= 1;
        --exponent;
        if ((exponent < 0) || (r0 < UINT32_C(0x00800000)))
        {
            if (use_clz != 0u)
            {
                unsigned shift;

                if (r0 == 0u)
                {
                    fputs("candidate CLZ reached zero\n", stderr);
                    exit(EXIT_FAILURE);
                }
#if defined(__GNUC__)
                shift = (unsigned)__builtin_clz(r0) - 8u;
#else
                shift = 0u;
                while ((r0 & UINT32_C(0x00800000)) == 0u)
                {
                    r0 <<= 1;
                    ++shift;
                }
                r0 >>= shift;
#endif
                r0 <<= shift;
                exponent -= (int32_t)shift;
            }
            else
            {
                if ((r0 >> 12) == 0u)
                {
                    r0 <<= 12;
                    exponent -= 12;
                }
                if ((r0 & UINT32_C(0x00ff0000)) == 0u)
                {
                    r0 <<= 8;
                    exponent -= 8;
                }
                if ((r0 & UINT32_C(0x00f00000)) == 0u)
                {
                    r0 <<= 4;
                    exponent -= 4;
                }
                if ((r0 & UINT32_C(0x00c00000)) == 0u)
                {
                    r0 <<= 2;
                    exponent -= 2;
                }
                if (r0 < UINT32_C(0x00800000))
                {
                    r0 <<= 1;
                    --exponent;
                }
            }
            if (exponent >= 0)
            {
                return (r0 + ((uint32_t)exponent << 23)) | r3;
            }
            return r3 | lsr32(r0, (unsigned)-exponent);
        }
    }

    {
        const uint32_t carry = (r1 >= UINT32_C(0x80000000));
        r0 += ((uint32_t)exponent << 23) + carry;
        if (r1 == UINT32_C(0x80000000))
        {
            r0 &= ~UINT32_C(1);
        }
        return r0 | r3;
    }
}

static void checkPair(uint32_t left, uint32_t right, uint64_t index)
{
    unsigned subtract;

    for (subtract = 0u; subtract < 2u; ++subtract)
    {
        const uint32_t stock_model =
            addSubModel(left, right, subtract, 0u);
        const uint32_t candidate = addSubModel(left, right, subtract, 1u);

        if (stock_model != candidate)
        {
            fprintf(stderr,
                    "mismatch index=%" PRIu64 " op=%s left=%08" PRIx32
                    " right=%08" PRIx32 " stock_model=%08" PRIx32
                    " candidate=%08" PRIx32 "\n",
                    index, subtract ? "sub" : "add", left, right,
                    stock_model, candidate);
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
        0u, 1u, 2u, UINT32_C(0x003fffff), UINT32_C(0x00400000),
        UINT32_C(0x00400001), UINT32_C(0x007ffffe),
        UINT32_C(0x007fffff)
    };
    size_t count = 0u;
    unsigned sign;
    unsigned exponent;
    size_t mantissa;

    for (sign = 0u; sign < 2u; ++sign)
    {
        for (exponent = 0u; exponent < 256u; ++exponent)
        {
            for (mantissa = 0u;
                 mantissa < sizeof(mantissas) / sizeof(mantissas[0]);
                 ++mantissa)
            {
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
    uint64_t index;
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
    if (state == 0u)
    {
        fputs("seed must be nonzero\n", stderr);
        return EXIT_FAILURE;
    }
    value_count = buildDirectedValues(values);
    index = 0u;
    for (left = 0u; left < value_count; ++left)
    {
        for (right = 0u; right < value_count; ++right)
        {
            checkPair(values[left], values[right], index++);
        }
    }
    for (index = 0u; index < random_count; ++index)
    {
        const uint64_t bits = nextRandom(&state);
        checkPair((uint32_t)bits, (uint32_t)(bits >> 32), index);
    }
    printf("PASS directed=%" PRIu64 " random=%" PRIu64
           " operations=%" PRIu64 " final_seed=%016" PRIx64 "\n",
           (uint64_t)value_count * (uint64_t)value_count,
           random_count,
           UINT64_C(2) * ((uint64_t)value_count * (uint64_t)value_count +
                          random_count),
           state);
    return EXIT_SUCCESS;
}
