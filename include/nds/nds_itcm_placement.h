#ifndef NDS_ITCM_PLACEMENT_H
#define NDS_ITCM_PLACEMENT_H

/* Placement-only ITCM admission (the 2026-07 census, "Task 37").
 *
 * The Task 37 census measured every profiled instruction and split its stall
 * cycles by whether the instruction touched data. Only the non-memory half can
 * be recovered by moving code; a load waits the same number of cycles wherever
 * the load itself lives. By that measure the tiers rank:
 *
 *     tier            cyc/insn   non-mem stall
 *     .itcm             1.68        14.7%
 *     .text.hot.draw    2.63        22.4%
 *     .main             3.29        29.5%
 *     .text.hot         3.30        30.0%
 *
 * ITCM is the only tier that clearly pays, so this is where measured toppers
 * go. Unlike NDS_RENDERER_HOT_CODE, this attribute changes placement and
 * nothing else: no ARM-state switch, no optimization level change. A task that
 * moved code and recompiled it differently at the same time could not
 * attribute its own result, and the ISA switch would roughly double the byte
 * cost of every admission against a section with 1,060 bytes free.
 *
 * See docs/optimization/ClaudeFable5_Task37_ItcmRepack_20260722.md.
 */

/* NDS_ITCM_LIB_LEAVES is a bitmask so the admissions can be bisected; the
 * port-side group is bit 2.
 * The 0-vs-7 A/B failed its state-hash gate and the owner confirmed the enabled
 * lab build misbehaves, so nothing here is trusted until the responsible group
 * is identified. */

#if defined(__arm__)
#define NDS_ITCM_CODE __attribute__((section(".itcm")))
#else
#define NDS_ITCM_CODE
#endif

/* Campaign 01 re-knapsack, 2026-08-17. Same rule as NDS_ITCM_CODE above:
 * placement and nothing else, in both directions, so an admission cannot be
 * confused with a recompile.
 *
 * The 2026-08-16 pack was ranked on a GX=1-era c200 capture. GX compose now
 * ships, and on the v4-c238 shipping census the generic display-list
 * interpreter chain it protected has gone nearly cold measured on the gate's
 * own rank-80 frames: ndsRendererScanList 6,188 B at 0.1 tk/byte,
 * ndsRendererSubmitHardwareTriangle 3,212 B at 0.1,
 * ndsRendererHardwareSubmitVertex 2,256 B at 0.3,
 * ndsRendererMtxMulAffine20p12 616 B at 1.1 (18,549 -> 384 tk/fr whole match
 * when the geometry engine took over the multiply). Those bytes rent ITCM at
 * one to three orders of magnitude below the residents around them.
 *
 * NDS_R2_ITCM_PACK2_EVICTED_CODE keeps hot/O3/ARM exactly as
 * NDS_RENDERER_HOT_CODE had them and drops only the section, so no emitted
 * instruction changes; NDS_R2_ITCM_PACK2_CODE adds a section and no more.
 * artifacts/performance/2026-08-17_itcm-repack2/ carries the ranking. */
#ifndef NDS_R2_ITCM_PACK2
#define NDS_R2_ITCM_PACK2 1
#endif

#if NDS_R2_ITCM_PACK2 && defined(__arm__)
#define NDS_R2_ITCM_PACK2_CODE __attribute__((section(".itcm")))
#else
#define NDS_R2_ITCM_PACK2_CODE
#endif

/* Layout control.
 *
 * Every Task 37 mask -- the libc leaves, a single 236-byte float leaf, the port
 * functions -- diverges from the baseline in exactly the same 692 of 3,892
 * state-hash records, with identical burst boundaries, and it does so with BGM
 * falsified too. That signature is invariant to WHICH code moved, so the
 * question is no longer "which admission is unsafe" but "does any change to
 * .main layout do this".
 *
 * A layout probe (NDS_TASK37_LAYOUT_PROBE, since removed) emitted exactly N
 * bytes of never-executed padding into .main or .itcm, moving nothing into
 * ITCM and calling nothing, to tell a latent layout sensitivity from ITCM
 * residency itself; padding .main passed the gate on 2026-07-22.
 */


#endif /* NDS_ITCM_PLACEMENT_H */
