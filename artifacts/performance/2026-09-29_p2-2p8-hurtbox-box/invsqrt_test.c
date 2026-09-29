/* Exhaustive host proof for the hurtbox kernel's table 1/s bound:
 * for every s2 in [2^22, 2^30] (the RowScales guard), inv(s2) >= 2^39 /
 * sqrt(s2), i.e. inv^2 * s2 >= 2^78, and inv fits int32. Also reports the
 * worst looseness against the exact value and against the previous
 * floor(2^50 / isqrt(s2 << 22)) + 1 form. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "invsqrt_tables.h"

static int32_t inv_table(uint32_t s2)
{
    const uint32_t e = 31u - (uint32_t)__builtin_clz(s2);
    const uint32_t q = e - 5u;
    const uint32_t m = s2 >> q;
    const uint32_t t = (q & 1u) ? sNdsP2HbInvSqrtOdd[m - 32u]
                                : sNdsP2HbInvSqrtEven[m - 32u];
    return (int32_t)(t << (15u - ((q + 1u) >> 1)));
}

static uint64_t isqrt64(uint64_t v)
{
    uint64_t r = (uint64_t)sqrtl((long double)v);
    while (r * r > v) r--;
    while ((r + 1) * (r + 1) <= v) r++;
    return r;
}

int main(void)
{
    const unsigned __int128 target = (unsigned __int128)1 << 78;
    double worst = 0.0, worst_old = 0.0, worst_vs_old = 0.0;
    uint64_t fails = 0, n = 0, overflow = 0;
    uint32_t s2;

    for (s2 = 1u << 22; s2 <= (1u << 30); s2++)
    {
        const int32_t inv = inv_table(s2);
        const unsigned __int128 lhs =
            (unsigned __int128)(uint64_t)inv * (uint64_t)inv * s2;
        double exact = ldexp(1.0, 39) / sqrt((double)s2);
        double ratio = (double)inv / exact;
        n++;
        if (inv <= 0) overflow++;
        if (lhs < target) {
            if (fails < 5) printf("FAIL s2=%u inv=%d\n", s2, inv);
            fails++;
        }
        if (ratio > worst) worst = ratio;
        if ((s2 & 0xfffu) == 0u) {
            uint64_t sq = isqrt64((uint64_t)s2 << 22);
            int64_t old = (int64_t)((1ull << 50) / sq) + 1;
            double r2 = (double)old / exact;
            double r3 = (double)inv / (double)old;
            if (r2 > worst_old) worst_old = r2;
            if (r3 > worst_vs_old) worst_vs_old = r3;
        }
        if (s2 == (1u << 30)) break;
    }
    printf("checked %llu values, fails %llu, overflow %llu\n",
           (unsigned long long)n, (unsigned long long)fails,
           (unsigned long long)overflow);
    printf("worst table/exact %.6f, worst old/exact %.9f, worst table/old %.6f\n",
           worst, worst_old, worst_vs_old);
    return fails != 0 || overflow != 0;
}
