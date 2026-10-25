#ifndef NDS_HWMATH_BENCH_H
#define NDS_HWMATH_BENCH_H

/* Lab-only price and equivalence instrument for the ARM9 divide and square-root
 * units. See src/port/nds_hwmath_bench.c for what each counter means and for
 * why this is a microbenchmark rather than a whole-match A/B. */

#ifdef __cplusplus
extern "C" {
#endif

void ndsHwMathBenchRun(void);

#ifdef __cplusplus
}
#endif

#endif /* NDS_HWMATH_BENCH_H */
