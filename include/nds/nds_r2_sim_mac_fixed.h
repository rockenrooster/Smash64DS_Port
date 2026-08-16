#ifndef NDS_R2_SIM_MAC_FIXED_H
#define NDS_R2_SIM_MAC_FIXED_H

/* THE WARM-MAC EXCHANGE-RATE INSTRUMENT. Lab only; never in a published ROM.
 *
 * WHAT QUESTION THIS ANSWERS. The sim-side soft-float classification
 * (artifacts/performance/2026-08-16_simside-softfloat/SIMSIDE.md) sized a
 * 71,491 tk/fr subset of fourteen bodies that are >=80% multiply-accumulate and
 * entered >=8 times a frame, and refused to assert an exchange rate for it
 * because the three rates measured in this binary span 3x (1.70x camera, 2.68x
 * narrow phase, 5.14x same-operation matrix pair). Its two largest members are
 * PURE 100% MAC and reproduce their source operation counts to the unit:
 *
 *   func_ovl2_800ED490           18,759 tk/fr   19.0 entr/fr   63 ops/entry
 *   gmCollisionGetWorldPosition  13,091 tk/fr   45.2 entr/fr   18 ops/entry
 *
 * A conversion's net is (float cost deleted) - (fixed cost added). The first
 * term is already measured, above. THIS HEADER EXISTS TO MEASURE THE SECOND ONE
 * WITHOUT CHANGING A SINGLE GAMEPLAY VALUE.
 *
 * HOW, and why the route is a SHADOW rather than a replacement. A route A/B
 * cannot price a gameplay change: one on this exact collision code ended with
 * damage 130/51 against 33/65 on the same ELF, one poked bit apart, because the
 * two arms played different fights and the ticks priced the fight. So the
 * candidate arm here runs the decomp float body EXACTLY AS BEFORE, keeps its
 * result, and then additionally evaluates the fixed-point form and throws the
 * answer away. Both arms therefore play the bit-identical match by
 * construction -- every whole-match invariant MUST be equal, and if one is not,
 * the instrument is broken rather than the candidate being interesting.
 *
 *   arm bit 0 (1) -- evaluate the fixed point x matrix transform
 *   arm bit 1 (2) -- evaluate the fixed 3x4 affine compose
 *   arm bit 2 (4) -- grade the transform against the float body's own result
 *
 * price(transform) = WORK-H(arm 1) - WORK-H(arm 0), over the counted shadow
 * evaluations. price(compose) = WORK-H(arm 3) - WORK-H(arm 1). Same binary,
 * poked with -SetGlobals, so there is no placement term at all -- the >=14,080
 * rank-80 and ~5,700 P50 cross-build floors do not apply to a within-pair
 * delta on one ELF.
 *
 * THE EDGE CONVERSION IS THE WHOLE ANSWER AND IT IS COUNTABLE BEFORE THE RUN.
 * Both bodies are f32-in and f32-out leaves, so a per-call conversion pays one
 * ndsR2CollisionF32ToFixed / FixedToF32 per float that crosses the boundary:
 *
 *   body                        floats in  floats out  edge conv  float ops
 *   gmCollisionGetWorldPosition     15          3          18        18
 *   func_ovl2_800ED490              24         12          36        63
 *
 * i.e. 1.00 conversion per deleted float operation for the transform and 0.57
 * for the compose. That ratio is the R2-07 L7 footprint lesson restated as
 * arithmetic (include/nds/nds_r2_collision_mtx.h) and it is why the SUBSET
 * SIZED BY FUNCTION IS NOT A CONVERTIBLE UNIT: what converts is a CHAIN whose
 * interior stays fixed, which is what nds_r2_collision_fixed.h is built for.
 * This instrument measures the leaf price so the chain's arithmetic can be done
 * with a measured constant instead of a projected one.
 *
 * COVERAGE, stated because it is a limit and not a detail. gmcollision.c calls
 * both bodies from inside its own translation unit -- fifteen sites -- and a
 * #define-before-#include rename moves the definition AND those call sites
 * together, so the wrapper below sees the CROSS-TU calls only. The counters
 * report exactly how many that is; the per-evaluation price does not depend on
 * the fraction, only its noise floor does. Makefile:2302 forbids a decomp
 * overlay patch for a new adaptation, so 100% capture is not available without
 * editing the source of truth.
 *
 * TWO DISCLOSED BIASES, both upward on the fixed cost, i.e. both conservative:
 *   1. The compose shadow's second operand is the previous captured matrix,
 *      held in a file-static, and refreshing it copies twelve floats (~12 tk)
 *      that a real compose does not pay. The real second operand is a
 *      different DObj's matrix, so its d-cache behaviour is colder than this
 *      static's, which biases the other way; neither is quantified.
 *   2. The input copy the wrapper needs (the float body writes *vec in place)
 *      may be sunk into the shadow branch by the compiler, charging ~3 tk of
 *      it to the candidate arm.
 *
 * NOTHING HERE IS A CONVERSION AND NOTHING HERE MAY SHIP. NDS_R2_SIM_MAC_SHADOW
 * defaults to 0 and the translation unit is not linked at 0.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NDS_R2_SIM_MAC_ARM_TRANSFORM 1u
#define NDS_R2_SIM_MAC_ARM_COMPOSE   2u
#define NDS_R2_SIM_MAC_ARM_GRADE     4u
/* Repeat count, arm >> 8. Zero means one evaluation.
 *
 * IT EXISTS BECAUSE THE REAL CALL SITES ARE UNREACHABLE, AND THAT IS ITSELF A
 * MEASURED RESULT. On the first run of this instrument the whole match produced
 * gNdsR2SimMacCmpsCalls = 0 and gNdsR2SimMacXfrmCalls = 168 over 2,039 presented
 * frames -- 0.082 entries a frame against gmCollisionGetWorldPosition's 45.2 and
 * func_ovl2_800ED490's 19.0. Both bodies are called almost entirely from inside
 * gmcollision.c, so no wrapper in port code can see them.
 *
 * The repeat turns cost(R) = a + b.R per driving call into a two-parameter fit
 * across three arms, and both parameters are wanted:
 *   b (slope)     -- the WARM marginal cost, the kernel's own arithmetic with
 *                    its lines already resident. A LOWER bound on real cost.
 *   a (intercept) -- everything paid once per driving call, which at 0.95
 *                    calls/frame is dominated by compulsory instruction fetch
 *                    of 1,768 B of kernel that nothing else touches. An UPPER
 *                    bound contribution.
 * The real per-entry cost at 45.2 entries a frame is bracketed by b and a + b,
 * and quoting either one alone would be a claim this instrument cannot make. */
#define NDS_R2_SIM_MAC_ARM_REPEAT_SHIFT 8u

/* `.data` and not `.bss`: a zero-initialised route word moves to .bss and
 * shifts every later .data object, which is how a previous same-binary pair
 * acquired a ~10,000 tk/fr placement floor. */
extern volatile uint32_t gNdsR2SimMacShadowArm;

/* Engagement. A route that never fires is indistinguishable from a route that
 * fired and saved nothing, and this campaign has shipped that mistake. */
extern volatile uint32_t gNdsR2SimMacXfrmCalls;   /* wrapper entries */
extern volatile uint32_t gNdsR2SimMacXfrmShadow;  /* fixed evaluations */
extern volatile uint32_t gNdsR2SimMacXfrmDecline; /* out of the kernel's domain */
extern volatile uint32_t gNdsR2SimMacCmpsShadow;
extern volatile uint32_t gNdsR2SimMacCmpsDecline;
extern volatile uint32_t gNdsR2SimMacCmpsCalls;   /* cross-TU compose entries */
extern volatile uint32_t gNdsR2SimMacDriveCalls;  /* damage-gateway drives */

/* Live-domain equivalence of the transform, graded against the decomp float
 * body's OWN result on the SAME inputs, every captured call, whole match. Units
 * are Q12 world quanta (1 = 1/4096 world unit); the cluster's standing bound is
 * 0.0200 world units = 81.92 in these units.
 *
 * Six separate scalars rather than one [6] array on purpose: reading a [N]
 * counter as a scalar through the harness silently returns its own address,
 * which cost this campaign a cycle. */
extern volatile uint32_t gNdsR2SimMacXfrmMaxDevQ12;
extern volatile uint32_t gNdsR2SimMacXfrmDev0;  /* dev == 0            */
extern volatile uint32_t gNdsR2SimMacXfrmDev1;  /* 1   <= dev <= 4     */
extern volatile uint32_t gNdsR2SimMacXfrmDev2;  /* 5   <= dev <= 16    */
extern volatile uint32_t gNdsR2SimMacXfrmDev3;  /* 17  <= dev <= 81    */
extern volatile uint32_t gNdsR2SimMacXfrmDev4;  /* 82  <= dev <= 4095  */
extern volatile uint32_t gNdsR2SimMacXfrmDev5;  /* dev >= 4096         */

/* Both take the float matrix the decomp body was handed. `in` is the ORIGINAL
 * point (the float body overwrites it in place) and `ref` is what the float
 * body produced from it. `arm` is the caller's single volatile read. */
void ndsR2SimMacShadowTransform(float mtx[4][4], const float in[3],
                                const float ref[3], uint32_t arm);
void ndsR2SimMacShadowCompose(float mtx[4][4], uint32_t arm);

/* Cleaned out of the D-cache at the frame-complete seam. melonDS's ReadMem has
 * no DCache lookup, and a max-deviation counter is written only when a new
 * maximum occurs -- exactly the access pattern that is still dirty at the final
 * stop and reads STALE. */
#define NDS_R2_SIM_MAC_GROUP(X) \
    X(gNdsR2SimMacXfrmCalls) \
    X(gNdsR2SimMacXfrmShadow) \
    X(gNdsR2SimMacXfrmDecline) \
    X(gNdsR2SimMacCmpsShadow) \
    X(gNdsR2SimMacCmpsDecline) \
    X(gNdsR2SimMacCmpsCalls) \
    X(gNdsR2SimMacDriveCalls) \
    X(gNdsR2SimMacXfrmMaxDevQ12) \
    X(gNdsR2SimMacXfrmDev0) \
    X(gNdsR2SimMacXfrmDev1) \
    X(gNdsR2SimMacXfrmDev2) \
    X(gNdsR2SimMacXfrmDev3) \
    X(gNdsR2SimMacXfrmDev4) \
    X(gNdsR2SimMacXfrmDev5)

#ifdef __cplusplus
}
#endif

#endif /* NDS_R2_SIM_MAC_FIXED_H */
