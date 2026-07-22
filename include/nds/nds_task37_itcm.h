#ifndef NDS_TASK37_ITCM_H
#define NDS_TASK37_ITCM_H

/* Task 37 placement-only ITCM admission.
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

#ifndef NDS_TASK37_ITCM_LEAVES
#define NDS_TASK37_ITCM_LEAVES 0
#endif

#if NDS_TASK37_ITCM_LEAVES && defined(__arm__)
#define NDS_TASK37_ITCM_CODE __attribute__((section(".itcm")))
#else
#define NDS_TASK37_ITCM_CODE
#endif

#endif /* NDS_TASK37_ITCM_H */
