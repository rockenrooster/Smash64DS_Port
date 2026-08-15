#ifndef NDS_R2_COLLISION_RING_H
#define NDS_R2_COLLISION_RING_H

/* R2-07 slice 52 -- the wiring of the fixed-point collision cluster.
 *
 * WHAT IS WIRED, AND WHAT DELIBERATELY IS NOT. The three PRODUCERS of the
 * fighter narrow phase run in fixed point; the CONSUMER does not.
 *
 *   func_ovl2_800EDBA4   chain walk        -> fixed interior, f32 boundary
 *   func_ovl2_800EDE00   inverse matrix    -> fixed cofactor, f32 out
 *   func_ovl2_800EDE5C   axis scales       -> fixed isqrt, f32 out
 *   gmCollisionTestRectangle / TestSphere  -> UNCHANGED decomp float
 *
 * That split is the whole correctness argument and it is why this can be
 * landed in one cycle. Every collision DECISION is still taken by decomp code,
 * by decomp comparisons, on f32 values living in the fields the source declared
 * -- FTParts::mtx_translate, ::unk_dobjtrans_0x10, ::unk_dobjtrans_0x9C and
 * ::vec_scale all keep their type, their layout and every one of their readers.
 * The only difference from the shipped ROM is the last bits of those f32
 * values, and that difference is bounded by the falsifier
 * (scripts/check-r2-collision-fixed.ps1) at 0.0024 world units against the
 * 0.0200 bound the campaign carries, over the LIVE joint-scale domain
 * 0.9937-2.0479.
 *
 * The alternative -- reinterpreting the 64 bytes of unk_dobjtrans_0x9C as a
 * fixed NDSR2CfxFrame -- buys gmCollisionTestRectangle and part of
 * gmCollisionGetWorldPosition as well, and the board has already proven the
 * slot reinterpretable (`sNdsFighterPartsPool`, `ndsFighterPartsSyncDObj` and
 * `ndsFighterPartsSetIdentity` are absent from both linked ELFs). It is not
 * done here because it moves the collision decision itself into port code and
 * obliges gmCollisionTestSphere -- which writes a shield hit's knockback ANGLE
 * and normal, continuous values no flip count can express -- to be converted
 * with it. Sized, named, and left for a cycle that can measure it on its own.
 *
 * HOW IT ENGAGES. The hook is the two gmCollisionCheckFighter* entry points,
 * which the linked ELF says have ZERO in-TU callers -- the property that makes
 * the `#define`-before-`#include` rename in src/import/battleship_gmcollision.c
 * capture every call rather than half of them. The wrapper prepares the joint
 * and then delegates to the decomp original, which finds its own
 * unk_dobjtrans_0x5/0x6/0x7 latches already set and early-returns out of the
 * float producers. Same joints, same order, no speculative work.
 *
 * WHAT DECLINING MEANS. Every stage is fail-closed: outside its proven domain,
 * on an is_use_animlocks joint (gmCollisionSetMatrixNcs is NOT converted), or
 * on a chain deeper than the source's own 18-deep stack, the kernel writes
 * nothing, leaves the latch clear, and the decomp float path takes the joint.
 * A decline is therefore a slower correct frame, never a wrong one -- and the
 * counters below are what turn "it declined" from a guess into a reading.
 */

#include <PR/ultratypes.h>

typedef struct DObj DObj;

/* Prepare one fighter joint's world matrix, inverse and axis scales in fixed
 * point, setting the source's own latches. Safe to call on any DObj: it is a
 * no-op when the latches are already set and declines to the float path
 * whenever it cannot represent the joint. */
void ndsR2CfxPrepareFighterJoint(DObj *main_dobj);

/* The falsifier arm's switch, initialised from NDS_R2_COLLISION_FIXED_DISPATCH.
 * Volatile so both arms link byte-identical text; see the definition. */
extern volatile u32 gNdsCfxRingEnable;

#if NDS_R2_COLLISION_FIXED_NARROW
/* R2-07 slice 53 -- the CONSUMER, and it exists to measure one number.
 *
 * Slice 52 stopped at the producers on purpose: leaving every collision DECISION
 * in decomp float code is what let it be graded by a bound instead of by a flip
 * count. It measured an exchange rate of 1.00, and the only remaining reason to
 * believe the lane pays is that the f32 boundary -- not the arithmetic -- is
 * what it spends. Deciding that needs a CONSUMER converted, because a consumer
 * early-exits and the straight-line producers do not.
 *
 * This answers gmCollisionCheckFighterAttackDamageCollide's tail
 * (gm/gmcollision.c:1379) directly: the source runs func_ovl2_800EDE00 and
 * func_ovl2_800EDE5C -- which ndsR2CfxPrepareFighterJoint has already done in
 * fixed point -- and then returns gmCollisionTestRectangle against
 * FTParts::unk_dobjtrans_0x9C. Here the same test runs as ndsR2CfxTestRectangle
 * against a frame built from FTParts::mtx_translate.
 *
 * FAIL-CLOSED, and the guard is the source's own latches rather than a new one:
 * unless prepare left unk_dobjtrans_0x5/0x6/0x7 all set, this declines and the
 * decomp body runs -- which also guarantees the two side effects the caller
 * would otherwise lose (unk_dobjtrans_0x9C and vec_scale are filled by prepare,
 * not by this).
 *
 * Returns 1 (hit), 0 (miss) or NDS_R2_CFX_NARROW_DECLINE. */
#define NDS_R2_CFX_NARROW_DECLINE (-1)

struct FTAttackColl;
struct FTDamageColl;

int ndsR2CfxTestFighterDamage(struct FTAttackColl *attack_coll,
                              struct FTDamageColl *damage_coll);

/* Slice 53's own falsifier arm, initialised from
 * NDS_R2_COLLISION_FIXED_NARROW_DISPATCH. Same mechanism as gNdsCfxRingEnable:
 * volatile, forced into .data, so the two arms differ in one byte. */
extern volatile u32 gNdsCfxNarrowEnable;

#if NDS_TICK_HUD
extern volatile u32 gNdsCfxNarrowCalls;    /* entries with dispatch on */
extern volatile u32 gNdsCfxNarrowAnswered; /* answered in fixed point */
extern volatile u32 gNdsCfxNarrowHits;     /* of those, TRUE */
extern volatile u32 gNdsCfxNarrowDeclined; /* latch/domain/clip-guard declines */
#endif
#endif /* NDS_R2_COLLISION_FIXED_NARROW */

#if NDS_TICK_HUD
/* Engagement, per the standing law that a fix ships with a counter showing it
 * firing AND a control showing where it is inert. `Declined` is not a warning
 * light: the float path is the oracle and a joint taking it is correct. What
 * would be a stop is `Declined` dominating `Engaged`, because then the measured
 * tick delta is placement, not arithmetic. */
extern volatile u32 gNdsCfxRingPrepareCalls;    /* wrapper entries */
extern volatile u32 gNdsCfxRingChainFixed;      /* chain walks completed fixed */
extern volatile u32 gNdsCfxRingChainDeclined;   /* animlocks / domain / depth */
extern volatile u32 gNdsCfxRingLocalsBuilt;     /* ndsR2CfxBuildLocal calls */
extern volatile u32 gNdsCfxRingComposes;        /* ndsR2CfxCompose calls */
extern volatile u32 gNdsCfxRingInvertFixed;     /* unk_dobjtrans_0x9C fills */
extern volatile u32 gNdsCfxRingInvertDeclined;
extern volatile u32 gNdsCfxRingScaleFixed;      /* vec_scale fills */
extern volatile u32 gNdsCfxRingScaleDeclined;
#endif

#endif /* NDS_R2_COLLISION_RING_H */
