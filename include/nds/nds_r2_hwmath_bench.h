#ifndef NDS_R2_HWMATH_BENCH_H
#define NDS_R2_HWMATH_BENCH_H

/* Lab-only price and equivalence instrument for the ARM9 divide and square-root
 * units. See src/port/nds_r2_hwmath_bench.c for what each counter means and for
 * why this is a microbenchmark rather than a whole-match A/B. */

#ifdef __cplusplus
extern "C" {
#endif

void ndsR2HwMathBenchRun(void);

#ifdef __cplusplus
}
#endif

#endif /* NDS_R2_HWMATH_BENCH_H */
