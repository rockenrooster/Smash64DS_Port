/* P2-2p8 (2026-09-27): memcpy and memset in ARM state, in ITCM.
 *
 * The per-PC profile of the four-CPU stress put newlib's generic C memcpy at
 * 33.8K ticks a frame and memset at 15.7K. Both were the -Os Thumb builds that
 * Task 37 moved into ITCM unchanged (libc_a-memcpy-stub.o, libc_a-memset.o):
 * one LDR/STR pair per word, 16 bytes a loop, so every main-RAM line crosses
 * the bus one word at a time. 252 calls a frame are mostly 64-byte matrix
 * copies (the stage GX patcher's 173).
 *
 * These are the same contracts -- memcpy(dst, src, n) and memset(dst, c, n)
 * both return dst -- with a 32-byte LDM/STM body when both pointers are word
 * aligned, a word loop for the remainder and bytes for the rest. A pair that
 * is not word aligned copies bytewise, as newlib's did. Linked in place of the
 * libc members (NDS_FAST_MEM, Makefile), so every caller -- including GCC's
 * own struct copies and libc internals -- takes them.
 *
 * Tick-HUD builds run ndsFastMemSelfTest at boot: every size 0..100 at every
 * source/destination word offset against a byte loop, with guard bytes on
 * both sides; gNdsFastMemSelfTestFailures must read 0. */
#include <nds.h>
#include <string.h>

__asm__(
    "    .pushsection .itcm,\"ax\",%progbits\n"
    "    .arm\n"
    "    .align 2\n"
    "    .global memcpy\n"
    "    .type memcpy, %function\n"
    "memcpy:\n"
    "    mov   ip, r0\n"
    "    orr   r3, r0, r1\n"
    "    tst   r3, #3\n"
    "    bne   .LndsMcBytes\n"
    "    cmp   r2, #32\n"
    "    blo   .LndsMcWords\n"
    "    push  {r4-r10}\n"
    "    sub   r2, r2, #32\n"
    ".LndsMcLoop32:\n"
    "    ldmia r1!, {r3-r10}\n"
    "    stmia ip!, {r3-r10}\n"
    "    subs  r2, r2, #32\n"
    "    bhs   .LndsMcLoop32\n"
    "    add   r2, r2, #32\n"
    "    pop   {r4-r10}\n"
    ".LndsMcWords:\n"
    "    subs  r2, r2, #4\n"
    "    blo   .LndsMcWordsDone\n"
    ".LndsMcLoop4:\n"
    "    ldr   r3, [r1], #4\n"
    "    str   r3, [ip], #4\n"
    "    subs  r2, r2, #4\n"
    "    bhs   .LndsMcLoop4\n"
    ".LndsMcWordsDone:\n"
    "    adds  r2, r2, #4\n"
    "    bxeq  lr\n"
    ".LndsMcBytes:\n"
    "    subs  r2, r2, #1\n"
    "    bxlo  lr\n"
    ".LndsMcLoop1:\n"
    "    ldrb  r3, [r1], #1\n"
    "    strb  r3, [ip], #1\n"
    "    subs  r2, r2, #1\n"
    "    bhs   .LndsMcLoop1\n"
    "    bx    lr\n"
    "    .size memcpy, . - memcpy\n"
    "\n"
    "    .global memset\n"
    "    .type memset, %function\n"
    "memset:\n"
    "    mov   ip, r0\n"
    "    and   r1, r1, #255\n"
    "    orr   r1, r1, r1, lsl #8\n"
    "    orr   r1, r1, r1, lsl #16\n"
    ".LndsMsHead:\n"
    "    tst   ip, #3\n"
    "    beq   .LndsMsAligned\n"
    "    subs  r2, r2, #1\n"
    "    bxlo  lr\n"
    "    strb  r1, [ip], #1\n"
    "    b     .LndsMsHead\n"
    ".LndsMsAligned:\n"
    "    cmp   r2, #32\n"
    "    blo   .LndsMsWords\n"
    "    push  {r4-r9}\n"
    "    mov   r3, r1\n"
    "    mov   r4, r1\n"
    "    mov   r5, r1\n"
    "    mov   r6, r1\n"
    "    mov   r7, r1\n"
    "    mov   r8, r1\n"
    "    mov   r9, r1\n"
    "    sub   r2, r2, #32\n"
    ".LndsMsLoop32:\n"
    "    stmia ip!, {r1, r3-r9}\n"
    "    subs  r2, r2, #32\n"
    "    bhs   .LndsMsLoop32\n"
    "    add   r2, r2, #32\n"
    "    pop   {r4-r9}\n"
    ".LndsMsWords:\n"
    "    subs  r2, r2, #4\n"
    "    blo   .LndsMsWordsDone\n"
    ".LndsMsLoop4:\n"
    "    str   r1, [ip], #4\n"
    "    subs  r2, r2, #4\n"
    "    bhs   .LndsMsLoop4\n"
    ".LndsMsWordsDone:\n"
    "    adds  r2, r2, #4\n"
    "    bxeq  lr\n"
    ".LndsMsTail:\n"
    "    strb  r1, [ip], #1\n"
    "    subs  r2, r2, #1\n"
    "    bne   .LndsMsTail\n"
    "    bx    lr\n"
    "    .size memset, . - memset\n"
    "    .popsection\n"
#if defined(__thumb__)
    "    .thumb\n"
#endif
    );

#if defined(NDS_TICK_HUD) && NDS_TICK_HUD
__attribute__((used)) volatile u32 gNdsFastMemSelfTestRuns;
__attribute__((used)) volatile u32 gNdsFastMemSelfTestFailures;

/* Called through volatile pointers so GCC cannot expand the calls itself. */
static void *(*volatile sNdsFastMemCopy)(void *, const void *, size_t) = memcpy;
static void *(*volatile sNdsFastMemSet)(void *, int, size_t) = memset;

void ndsFastMemSelfTest(void)
{
    static u8 src[160] __attribute__((aligned(4)));
    static u8 dst[160] __attribute__((aligned(4)));
    u32 size;
    u32 so;
    u32 dof;
    u32 i;

    for (size = 0u; size <= 100u; size++)
    {
        for (so = 0u; so < 4u; so++)
        {
            for (dof = 0u; dof < 4u; dof++)
            {
                u8 *d = &dst[16u + dof];
                void *ret;

                for (i = 0u; i < sizeof(src); i++)
                {
                    src[i] = (u8)((i * 7u) + size + (so << 4));
                    dst[i] = 0xa5u;
                }
                ret = sNdsFastMemCopy(d, &src[16u + so], size);
                gNdsFastMemSelfTestRuns++;
                if (ret != d)
                {
                    gNdsFastMemSelfTestFailures++;
                }
                for (i = 0u; i < sizeof(dst); i++)
                {
                    u32 want = ((i >= 16u + dof) && (i < 16u + dof + size)) ?
                        src[i - dof + so] : 0xa5u;

                    if (dst[i] != want)
                    {
                        gNdsFastMemSelfTestFailures++;
                        break;
                    }
                }
                for (i = 0u; i < sizeof(dst); i++)
                {
                    dst[i] = 0x5au;
                }
                ret = sNdsFastMemSet(d, (int)(0x100u + size + so), size);
                gNdsFastMemSelfTestRuns++;
                if (ret != d)
                {
                    gNdsFastMemSelfTestFailures++;
                }
                for (i = 0u; i < sizeof(dst); i++)
                {
                    u32 want = ((i >= 16u + dof) && (i < 16u + dof + size)) ?
                        ((size + so) & 0xffu) : 0x5au;

                    if (dst[i] != want)
                    {
                        gNdsFastMemSelfTestFailures++;
                        break;
                    }
                }
            }
        }
    }
}
#endif
