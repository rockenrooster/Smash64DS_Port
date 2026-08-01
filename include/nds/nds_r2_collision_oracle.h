#ifndef SSB64_NDS_R2_COLLISION_ORACLE_H
#define SSB64_NDS_R2_COLLISION_ORACLE_H

#include <PR/ultratypes.h>

/* R2-07 L7, step one: measure the fixed-point collision kernel against the
 * joints a real match actually produces.
 *
 * include/nds/nds_r2_collision_mtx.h is GREEN on its host falsifier -- 0.016609
 * world units against the 0.0200 bound -- but green on a SYNTHETIC domain. Its
 * own header says so: "It still has to be measured against real hurtbox
 * dimensions and real joint scales." The falsifier sweeps rotations at scales
 * 0.90-1.10 because nobody has measured which scales SSB64 visits, and the
 * conservative 0.25-2.00 sweep is 0.427738 -- twenty times the bound. Whether
 * L7 is safe to wire therefore depends entirely on a number nobody has read off
 * the running game, and wiring first and measuring after is not available: this
 * kernel decides hits, and a wrong answer is a hit that lands or does not,
 * amplified by damage, knockback and hitstun.
 *
 * So this is a READ-ONLY oracle, default off, that changes no decision. It runs
 * after the gameplay tick over the port's own fighter-parts pool, takes every
 * part whose invert latch the decomp set this frame -- i.e. exactly the joints
 * collision inverted, no more -- and re-does that inverse in 20.12, then
 * compares the two on probe points at the distances collision actually queries.
 *
 * WHAT EACH COUNTER SETTLES.
 *   Samples/Singular  coverage, and whether any real joint is degenerate (the
 *                     kernel fails closed there; the decomp spins forever).
 *   MaxDevQ12[bucket] the answer. Deviation in 1/4096 world units at probe
 *                     offsets of 1, 4 and 16 units from the joint origin.
 *                     0.0200 world units is 82 in these units, so any bucket
 *                     at or under 82 is inside the bound L7 is gated on.
 *                     Bucketing by distance is deliberate: the error is
 *                     dominated by the single multiply by R^-1, so it grows
 *                     with |p - t|, and one aggregate number would hide which
 *                     query distances are safe.
 *   ScaleMin/MaxQ12   the domain question, in 1/4096 units. Read from
 *                     vec_scale, which func_ovl2_800EDE5C already computes, so
 *                     it costs nothing and cannot disagree with the game. If
 *                     the real range sits inside 0.90-1.10 the falsifier's
 *                     gated domain is the right one; if it reaches 0.25-2.00
 *                     the kernel needs more fractional bits before it is wired.
 *
 * Not a permanent probe. It exists to answer one question, and it comes out
 * with the commit that wires L7 in or the one that abandons it. */

#define NDS_R2_COLLISION_ORACLE_BUCKETS 3u
/* Probe offsets from the joint origin, in whole world units. */
#define NDS_R2_COLLISION_ORACLE_OFFSET_0 1
#define NDS_R2_COLLISION_ORACLE_OFFSET_1 4
#define NDS_R2_COLLISION_ORACLE_OFFSET_2 16
/* 0.0200 world units expressed in 1/4096ths, the bound R2-03 E64b/E65 set and
 * the one the kernel's host falsifier is gated on. */
#define NDS_R2_COLLISION_ORACLE_BOUND_Q12 82u

void ndsR2CollisionOracleSampleFrame(void);

extern volatile u32 gNdsR2CollisionOracleSamples;
extern volatile u32 gNdsR2CollisionOracleSingular;
extern volatile u32 gNdsR2CollisionOracleMaxDevQ12[
    NDS_R2_COLLISION_ORACLE_BUCKETS];
extern volatile u32 gNdsR2CollisionOracleOverBoundCount;
extern volatile u32 gNdsR2CollisionOracleScaleMinQ12;
extern volatile u32 gNdsR2CollisionOracleScaleMaxQ12;

#endif
