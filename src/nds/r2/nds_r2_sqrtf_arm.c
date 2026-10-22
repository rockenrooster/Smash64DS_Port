/* LAB ONLY -- the ARM-state arm of the sqrtf same-binary route.
 *
 * NDS_R2_HWMATH_ROUTE defaults to 0 and at 0 this translation unit is not in
 * CFILES at all, so a published ROM is byte-identical with or without it.
 *
 * WHY IT EXISTS. The change being measured is a codegen flag on
 * src/nds/r2/nds_r2_sqrtf.c: -marm instead of -mthumb, so that
 * include/nds/nds_r2_sqrtf.h's 48-bit `root * root` compiles to one UMULL
 * rather than `bl __aeabi_lmul` (ARMv5TE Thumb has no UMULL). A cross-build A/B
 * cannot decide a 4,300-4,750 tk/fr win against a >=14,080 rank-80 cross-build
 * floor, so both bodies ship in ONE binary and a `.data` word selects between
 * them: nds_r2_sqrtf.o stays -mthumb (the control, i.e. exactly what ships
 * today) and this object is built -marm by the Makefile.
 *
 * THE ARITHMETIC IS NOT DUPLICATED. Both arms compile the identical
 * include/nds/nds_r2_sqrtf.h; only the surrounding ten lines are repeated, and
 * they must stay a mirror of nds_r2_sqrtf.c's ndsR2SqrtfBody. The equivalence
 * is already graded: build-c213-hwmath4's gNdsR2HwMathBenchSqrtfMismatch is 0
 * over 65,536 inputs, comparing an ARM build of this header against the shipped
 * Thumb sqrtf.
 */

#include "nds_scene_harness_config.h"

#include <nds.h>

#include <nds/nds_r2_hwmath_unit.h>

