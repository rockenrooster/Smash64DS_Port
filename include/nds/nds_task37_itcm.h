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

/* NDS_TASK37_ITCM_LEAVES is a bitmask so the admissions can be bisected; the
 * port-side group is bit 2, surfaced by the Makefile as NDS_TASK37_ITCM_PORT.
 * The 0-vs-7 A/B failed its state-hash gate and the owner confirmed the enabled
 * lab build misbehaves, so nothing here is trusted until the responsible group
 * is identified. */
#ifndef NDS_TASK37_ITCM_PORT
#define NDS_TASK37_ITCM_PORT 0
#endif

#if NDS_TASK37_ITCM_PORT && defined(__arm__)
#define NDS_TASK37_ITCM_CODE __attribute__((section(".itcm")))
#else
#define NDS_TASK37_ITCM_CODE
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
 * NDS_TASK37_LAYOUT_PROBE emits exactly N bytes of never-executed padding into
 * .main. It moves nothing into ITCM and calls nothing. If the gate diverges
 * against this too, the sensitivity is latent and pre-existing -- Task 37 only
 * disturbs it -- and the gate cannot referee any code change at all. If the
 * gate passes against it, ITCM residency specifically is what breaks, and Task
 * 37 is genuinely at fault.
 */
#ifndef NDS_TASK37_LAYOUT_PROBE
#define NDS_TASK37_LAYOUT_PROBE 0
#endif

/* 0 = pad .main (shifts main-RAM layout), 1 = pad .itcm (grows the ITCM section
 * and its load image without relocating any real function). Padding .main
 * passed the gate on 2026-07-22, so the remaining question is whether growing
 * .itcm at all is what diverges, independently of which code moved. */
#ifndef NDS_TASK37_LAYOUT_PROBE_ITCM
#define NDS_TASK37_LAYOUT_PROBE_ITCM 0
#endif

#if NDS_TASK37_LAYOUT_PROBE && defined(__arm__)
#define NDS_TASK37_PROBE_STR2(x) #x
#define NDS_TASK37_PROBE_STR(x) NDS_TASK37_PROBE_STR2(x)
#if NDS_TASK37_LAYOUT_PROBE_ITCM
__asm__(".section .itcm.nds_task37_layout_probe,\"ax\",%progbits\n"
        ".balign 4\n"
        ".space " NDS_TASK37_PROBE_STR(NDS_TASK37_LAYOUT_PROBE) ", 0\n"
        ".previous\n");
#else
__asm__(".section .text.nds_task37_layout_probe,\"ax\",%progbits\n"
        ".balign 4\n"
        ".space " NDS_TASK37_PROBE_STR(NDS_TASK37_LAYOUT_PROBE) ", 0\n"
        ".previous\n");
#endif
#endif

#endif /* NDS_TASK37_ITCM_H */
