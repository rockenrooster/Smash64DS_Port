#include <stdio.h>
#include <stdint.h>
typedef int32_t s32; typedef uint32_t u32; typedef int16_t s16;
volatile u32 gNdsR2CubicSaturations;
s32 eval_old(s32, s32, s32, s32, s32, s32, u32);
s32 eval_new(s32, s32, s32, s32, s32, s32, u32);
static uint64_t st = 0x9E3779B97F4A7C15ull;
static uint32_t rnd(void) { st ^= st << 13; st ^= st >> 7; st ^= st << 17; return (uint32_t)st; }
int main(void) {
    long long n, bad = 0;
    for (n = 0; n < 300000000LL; n++) {
        u32 k = 5u + (rnd() % 3u); u32 m = rnd() & 7u;
        s32 len = (m < 4) ? (s32)(rnd() % (200u << 12)) - (8 << 12) : (s32)rnd();
        s32 inv = (m & 1) ? (s32)(1073741824u / (1u + (rnd() % 300u))) : (s32)rnd();
        s32 vb = (m < 6) ? ((s32)(s16)rnd()) << (rnd() % 11u) : (s32)rnd();
        s32 vt = (m < 6) ? ((s32)(s16)rnd()) << (rnd() % 11u) : (s32)rnd();
        s32 rb = (m < 6) ? ((s32)(s16)rnd()) << (rnd() % 11u) : (s32)rnd();
        s32 rt = (m < 6) ? ((s32)(s16)rnd()) << (rnd() % 11u) : (s32)rnd();
        u32 s0 = gNdsR2CubicSaturations; s32 a = eval_old(len, inv, vb, vt, rb, rt, k);
        u32 s1 = gNdsR2CubicSaturations; s32 b = eval_new(len, inv, vb, vt, rb, rt, k);
        u32 s2 = gNdsR2CubicSaturations;
        if (a != b || (s1 - s0) != (s2 - s1)) { if (bad < 5) printf("diff %lld\n", n); bad++; }
    }
    printf("tested %lld bad %lld\n", n, bad); return bad != 0;
}
