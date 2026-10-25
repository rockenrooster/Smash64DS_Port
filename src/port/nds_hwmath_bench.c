/* What one operation on the ARM9 divide and square-root units actually costs,
 * measured on the machine rather than read off GBATEK, and whether it returns
 * the same answer as the software form it would replace.
 *
 * WHY A MICROBENCHMARK AND NOT A WHOLE-MATCH A/B. The question here is a
 * PRICE PER OPERATION for four primitives, not a gate delta for a shipped
 * change. A whole-match arm would cost two builds and two hour-long runs to
 * answer it, would answer it only for the one lane that happens to call the
 * primitive, and -- for the collision ring, which is the lane that named these
 * hooks -- would be spent on a route already measured closed at 2.68x with a
 * lane ceiling of 15,217 tk/fr at rank-80 against a +94,481 requirement. The
 * per-operation price is the transferable number: it re-prices EXCHANGE.md's
 * measured __udivmoddi4 row directly, and it is also the engine cost of
 * SIMSIDE.md's column N, which is a different lane entirely.
 *
 * WHAT THIS PRICE IS AND IS NOT. Every arm runs its kernel back to back in a
 * tight loop, so the instruction cache is fully warm and the branch history is
 * ideal. That is the RIGHT basis for "which of these two implementations of the
 * same operation is cheaper", because both arms get the same favour, and it is
 * the WRONG basis for "what will this cost in situ" -- a cold kernel entered a
 * few times a frame pays compulsory fetch on top, which this campaign has
 * measured at 23-51 cycles per line and has twice seen invert a win. Read every
 * number below as a lower bound on the in-situ cost and an upper bound on the
 * saving.
 *
 * THE BASELINE ARM IS NOT OPTIONAL. gNdsHwMathBenchBaseTicks times the loop,
 * the operand generator and the sink store with no kernel at all. Every other
 * arm's honest cost is its own ticks minus that one. Reporting an arm's raw
 * ticks would charge each kernel for an LCG it does not have.
 *
 * cpuGetTiming() ticks are the project's own unit -- the same clock the tick
 * HUD's buckets and the +94,481 requirement are quoted in -- so a figure here
 * is directly comparable with SIMSIDE.md section 3's tk/call column
 * (__aeabi_fdiv 59.19, sqrtf 144.62, __aeabi_fmul 13.23).
 *
 * LAB ONLY. NDS_HWMATH_BENCH defaults to 0 and at 0 this translation unit is
 * not linked at all, so a published ROM is byte-identical with or without it.
 */

#include "nds_scene_harness_config.h"

