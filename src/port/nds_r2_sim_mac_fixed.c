/* The warm-MAC exchange-rate instrument's ARM-state bodies.
 *
 * Why its own translation unit: src/import/battleship_gmcollision.c builds
 * -mthumb, ARMv5TE Thumb has no SMULL, and every (int64)a * b in these kernels
 * would become a call to __aeabi_lmul at 4.49 cyc per multiply against a
 * hardware umull's 2.00. The Makefile builds THIS object -marm for exactly the
 * reason nds_r2_collision_fixed.o is built -marm.
 *
 * The arithmetic is not restated here. Every kernel below is the already-proven
 * one from include/nds/nds_r2_collision_fixed.h, graded by
 * scripts/check-r2-collision-fixed.ps1 against a transcription of the decomp
 * float originals (forward chain depth 6: 0.0014219 world units against a
 * 0.0200 bound). Restating them would price a different implementation than the
 * one a conversion would ship.
 *
 * NO DIVIDE AND NO ROOT ARE REACHABLE FROM HERE. ndsR2CfxLoadF32,
 * ndsR2CfxCompose and ndsR2CfxTransformPoint are shifts, adds and SMULL only --
 * ndsR2CfxRowScales is the only kernel in that header that touches
 * NDS_R2_CFX_DIV64 / NDS_R2_CFX_ISQRT64 and nothing here calls it. The refusal
 * on __udivmoddi4 (already 11.70 calls/fr, 2,909 tk/fr) is satisfied by
 * construction, not by a guard.
 */

#include <nds/nds_r2_sim_mac_fixed.h>
#include <nds/nds_r2_collision_fixed.h>

volatile uint32_t gNdsR2SimMacShadowArm
    __attribute__((used, section(".data"))) = 0u;

volatile uint32_t gNdsR2SimMacXfrmCalls __attribute__((used));
volatile uint32_t gNdsR2SimMacXfrmShadow __attribute__((used));
volatile uint32_t gNdsR2SimMacXfrmDecline __attribute__((used));
volatile uint32_t gNdsR2SimMacCmpsShadow __attribute__((used));
volatile uint32_t gNdsR2SimMacCmpsDecline __attribute__((used));
volatile uint32_t gNdsR2SimMacCmpsCalls __attribute__((used));
volatile uint32_t gNdsR2SimMacDriveCalls __attribute__((used));

volatile uint32_t gNdsR2SimMacXfrmMaxDevQ12 __attribute__((used));
volatile uint32_t gNdsR2SimMacXfrmDev0 __attribute__((used));
volatile uint32_t gNdsR2SimMacXfrmDev1 __attribute__((used));
volatile uint32_t gNdsR2SimMacXfrmDev2 __attribute__((used));
volatile uint32_t gNdsR2SimMacXfrmDev3 __attribute__((used));
volatile uint32_t gNdsR2SimMacXfrmDev4 __attribute__((used));
volatile uint32_t gNdsR2SimMacXfrmDev5 __attribute__((used));

/* External linkage and NOT volatile: the result has to be observably stored so
 * -O2 cannot delete the arithmetic being priced, and a plain store to an
 * externally visible object costs one instruction where a volatile one costs a
 * load and a store. `used` because --gc-sections collects a global whose only
 * consumer is the optimiser's inability to prove it dead. */
uint32_t gNdsR2SimMacXfrmSink __attribute__((used));
uint32_t gNdsR2SimMacCmpsSink __attribute__((used));

/* The compose shadow's second operand. The real call at gm/gmcollision.c:390
 * composes a DObj's mtx_translate with a DIFFERENT DObj's unk_dobjtrans_0x10,
 * and only one of the two is reachable from the transform seam, so the previous
 * captured matrix stands in for the other. Identity until the first refresh so
 * the first evaluation is in-domain rather than reading uninitialised floats. */
static float sNdsR2SimMacPrev[4][4] = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
};

/* Reinterpret, NOT convert. `(uint32_t)some_float` is __aeabi_f2uiz -- a
 * soft-float call -- and three of them per evaluation would have been charged
 * to the fixed form, which is precisely the cost this instrument exists to
 * measure. This is a register move. */
static inline uint32_t ndsR2SimMacBits(float value)
{
    uint32_t bits;

    __builtin_memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static void ndsR2SimMacGrade(int32_t dev)
{
    uint32_t magnitude = (uint32_t)((dev < 0) ? -dev : dev);

    if (magnitude > gNdsR2SimMacXfrmMaxDevQ12)
    {
        gNdsR2SimMacXfrmMaxDevQ12 = magnitude;
    }
    if (magnitude == 0u)
    {
        gNdsR2SimMacXfrmDev0++;
    }
    else if (magnitude <= 4u)
    {
        gNdsR2SimMacXfrmDev1++;
    }
    else if (magnitude <= 16u)
    {
        gNdsR2SimMacXfrmDev2++;
    }
    else if (magnitude <= 81u)
    {
        /* 81 Q12 quanta = 0.01978 world units, the last value strictly inside
         * the cluster's standing 0.0200 bound. */
        gNdsR2SimMacXfrmDev3++;
    }
    else if (magnitude <= 4095u)
    {
        gNdsR2SimMacXfrmDev4++;
    }
    else
    {
        gNdsR2SimMacXfrmDev5++;
    }
}

/* The repeat loop's barrier. `memory` forces `mtx` to be re-read from memory on
 * every iteration, which is what stops -O2 from hoisting the whole pure kernel
 * out of the loop and reporting a slope of zero. It emits no instruction, so it
 * costs nothing it is measuring. */
static inline uint32_t ndsR2SimMacRepeat(uint32_t arm)
{
    uint32_t repeat = arm >> NDS_R2_SIM_MAC_ARM_REPEAT_SHIFT;

    return (repeat == 0u) ? 1u : repeat;
}

static void ndsR2SimMacTransformOnce(float mtx[4][4], const float in[3],
                                     const float ref[3], uint32_t arm)
{
    NDSR2CfxMtx fixed;
    int32_t point[3];
    int32_t out[3];
    unsigned int axis;

    if (ndsR2CfxLoadF32(&fixed, mtx) == 0)
    {
        gNdsR2SimMacXfrmDecline++;
        return;
    }
    for (axis = 0u; axis < 3u; axis++)
    {
        point[axis] = ndsR2CollisionF32ToFixed(in[axis], NDS_R2_CFX_POS_BITS);
        if (point[axis] == NDS_R2_COLLISION_F32_OVERFLOW)
        {
            gNdsR2SimMacXfrmDecline++;
            return;
        }
    }

    ndsR2CfxTransformPoint(out, &fixed, point);

    /* The f32 boundary a real conversion would pay on the way out. */
    gNdsR2SimMacXfrmSink =
        ndsR2SimMacBits(
            ndsR2CollisionFixedToF32((int64_t)out[0], NDS_R2_CFX_POS_BITS)) ^
        ndsR2SimMacBits(
            ndsR2CollisionFixedToF32((int64_t)out[1], NDS_R2_CFX_POS_BITS)) ^
        ndsR2SimMacBits(
            ndsR2CollisionFixedToF32((int64_t)out[2], NDS_R2_CFX_POS_BITS));
    gNdsR2SimMacXfrmShadow++;

    if ((arm & NDS_R2_SIM_MAC_ARM_GRADE) != 0u)
    {
        for (axis = 0u; axis < 3u; axis++)
        {
            int32_t reference =
                ndsR2CollisionF32ToFixed(ref[axis], NDS_R2_CFX_POS_BITS);

            if (reference == NDS_R2_COLLISION_F32_OVERFLOW)
            {
                continue;
            }
            ndsR2SimMacGrade(reference - out[axis]);
        }
    }
}

void ndsR2SimMacShadowTransform(float mtx[4][4], const float in[3],
                                const float ref[3], uint32_t arm)
{
    uint32_t remaining = ndsR2SimMacRepeat(arm);

    while (remaining-- != 0u)
    {
        __asm__ __volatile__("" ::: "memory");
        ndsR2SimMacTransformOnce(mtx, in, ref, arm);
    }
}

static void ndsR2SimMacComposeOnce(float mtx[4][4])
{
    NDSR2CfxMtx lhs;
    NDSR2CfxMtx rhs;
    NDSR2CfxMtx dst;
    float store[4][4];
    unsigned int row;
    unsigned int col;

    if ((ndsR2CfxLoadF32(&lhs, sNdsR2SimMacPrev) == 0) ||
        (ndsR2CfxLoadF32(&rhs, mtx) == 0) ||
        (ndsR2CfxCompose(&dst, &lhs, &rhs) == 0))
    {
        gNdsR2SimMacCmpsDecline++;
    }
    else
    {
        ndsR2CfxStoreF32(store, &dst);
        gNdsR2SimMacCmpsSink ^=
            ndsR2SimMacBits(store[0][0]) ^ ndsR2SimMacBits(store[3][2]);
        gNdsR2SimMacCmpsShadow++;
    }

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            sNdsR2SimMacPrev[row][col] = mtx[row][col];
        }
    }
}

void ndsR2SimMacShadowCompose(float mtx[4][4], uint32_t arm)
{
    uint32_t remaining = ndsR2SimMacRepeat(arm);

    while (remaining-- != 0u)
    {
        __asm__ __volatile__("" ::: "memory");
        ndsR2SimMacComposeOnce(mtx);
    }
}
