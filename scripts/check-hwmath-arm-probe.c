/* ARM codegen probe for the hardware-math kernels.
 *
 * check-hwmath.c compiles the ALGORITHM with the unit modelled by a C
 * divide, so compiling THAT file for ARM would find __aeabi_ldivmod every time
 * and prove nothing. This file is the shape the ROM actually builds: the real
 * MMIO primitive from include/nds/nds_hwmath_unit.h under the same
 * algorithm, so the assembly the checker greps is the assembly that ships.
 *
 * What must not appear: any soft-float helper (the point is to stop calling
 * them) and any libgcc 64-bit divide (the point is to stop calling those too --
 * EXCHANGE.md section 0.4 names __udivmoddi4 four times per narrow-phase entry
 * as the measured cause of the collision ring's 2.68x).
 */

#include <stddef.h>
#include <stdint.h>

#include "../include/nds/nds_hwmath_unit.h"

/* No pointer cast, deliberately: see the type note in nds_hwmath.h. If the
 * primitive's remainder type ever drifts from the algorithm's, this is a hard
 * -Werror incompatible-pointer error rather than silent aliasing UB. */
static int64_t ndsHwMathDivideUnit(int64_t numerator, int32_t denominator,
                                     int32_t *remainder_out)
{
    return ndsHwMathDivide6432(numerator, denominator, remainder_out);
}

#include "../include/nds/nds_hwmath.h"

/* Out-of-line instantiations so the probe has bodies to grep. */
int ndsHwMathProbeDivBits(unsigned int a, unsigned int b, unsigned int *out);
int32_t ndsHwMathProbeCfxDiv64(int64_t numerator, int64_t denominator);
uint32_t ndsHwMathProbeCfxIsqrt64(uint64_t value);

int ndsHwMathProbeDivBits(unsigned int a, unsigned int b, unsigned int *out)
{
    return ndsHwMathDivBits(a, b, out);
}

int32_t ndsHwMathProbeCfxDiv64(int64_t numerator, int64_t denominator)
{
    return ndsHwMathCfxDiv64(numerator, denominator);
}

uint32_t ndsHwMathProbeCfxIsqrt64(uint64_t value)
{
    return ndsHwMathCfxIsqrt64(value);
}
