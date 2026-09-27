#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "conv_impl.h"

static uint32_t fbits(float f) { uint32_t b; memcpy(&b, &f, 4); return b; }

static uint64_t rng = 0x9e3779b97f4a7c15ull;
static uint64_t next64(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return rng; }

int main(void)
{
    uint64_t bad = 0;
    uint32_t x = 0;
    do {
        if (ndsConvU32ToF32Bits(x) != fbits((float)x)) { if (bad < 5) printf("u32 %u\n", x); bad++; }
        x++;
    } while (x != 0);
    printf("u32 exhaustive: %llu mismatches\n", (unsigned long long)bad);
    bad = 0;
    for (uint64_t i = 0; i < 400000000ull; i++) {
        uint64_t r = next64();
        int shift = (int)(next64() % 64);
        int64_t v = (int64_t)(r >> shift);
        if (i & 1) v = -v;
        if (ndsConvS64ToF32Bits(v) != fbits((float)v)) { if (bad < 5) printf("s64 %lld\n", (long long)v); bad++; }
    }
    /* edges: every power of two and neighbours, halfway cases */
    for (int e = 0; e < 64; e++) {
        for (int d = -3; d <= 3; d++) {
            int64_t v = (int64_t)((e == 63) ? 0x8000000000000000ull : (1ull << e)) + d;
            int64_t vs[2] = { v, -v };
            for (int k = 0; k < 2; k++)
                if (ndsConvS64ToF32Bits(vs[k]) != fbits((float)vs[k])) { if (bad < 10) printf("edge %lld\n", (long long)vs[k]); bad++; }
        }
        for (int m = 1; m < 256; m++) {
            if (e + 8 >= 63) break;
            int64_t v = ((int64_t)m << e) + ((int64_t)1 << (e > 0 ? e - 1 : 0));
            if (ndsConvS64ToF32Bits(v) != fbits((float)v)) { if (bad < 10) printf("half %lld\n", (long long)v); bad++; }
        }
    }
    printf("s64 random+edges: %llu mismatches\n", (unsigned long long)bad);
    return 0;
}
