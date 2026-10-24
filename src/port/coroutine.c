/* Coroutines for the source's threads (libultra_os.c runs every OSThread as
 * one) and the DTCM hot stack. A coroutine owns its stack: malloc'd, or the
 * caller's block (portCoroutineCreateStatic, never freed here). The register
 * swap is coroutine_arm.s. ndsDtcmHotStackRun (ITCM) runs a subtree on the
 * DTCM hot stack to save its D-cache fills; a call from inside a suspended
 * hot-stack context runs in place instead (gNdsDtcmHotStackBusy). */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <PR/ultratypes.h>
#include <port/coroutine.h>

#define NDS_COROUTINE_MIN_STACK 4096u

typedef struct NDSCoroutineContext {
    u32 r4, r5, r6, r7, r8, r9, r10, r11;
    u32 sp;
    u32 lr;
} NDSCoroutineContext;

struct PortCoroutine {
    NDSCoroutineContext context;
    NDSCoroutineContext caller_context;
    void (*entry)(void *);
    void *arg;
    void *stack;
    size_t stack_size;
    int finished;
    struct PortCoroutine *caller;
    /* Storage is a caller-owned block, not two heap allocations: do not free
     * it. Set only by portCoroutineCreateStatic. */
    int is_static;
#if defined(NDS_LAB_STACK_PAINT) && NDS_LAB_STACK_PAINT
    int lab_owner;
#endif
};

static PortCoroutine *sCurrentCoroutine;

/* The DTCM hot stack. The four-fighter frame spent ~1,470 D-cache line fills
 * a frame on the gameplay coroutine's main-RAM stack (a read-allocate cache:
 * every push is a write-through, every pop of an evicted frame is a ~62-cycle
 * fill). The subtrees that own most of them run here instead: zero-wait,
 * uncached and out of the 4 KB D-cache. The lean fighter list and the stage
 * GX segment commit (same-ROM WORK-H paired -18.4K), then the map-collision
 * step and the particle display proc (-7.9K); each source tick (ndsR2BattleRun,
 * -15.4K); the stage owner prep and the fighter display proc (-11.8K);
 * deepest reach 3,876 B. Only subtrees that read no storage into a stack
 * buffer (ndsAudioStorageReadCard bounces one) and hand no stack address to
 * DMA or the ARM7 may run on it; a coroutine switched to from it runs its own
 * hot-stack calls in place (gNdsDtcmHotStackBusy). The fighter pose was priced
 * and left off: its frames were ~46 fills a frame, a wash against the
 * trampoline. */
#ifndef NDS_DTCM_HOT_STACK_BYTES
#define NDS_DTCM_HOT_STACK_BYTES 6144u
#endif
#define NDS_DTCM_HOT_STACK_STR2(x) #x
#define NDS_DTCM_HOT_STACK_STR(x) NDS_DTCM_HOT_STACK_STR2(x)
u64 gNdsDtcmHotStack[NDS_DTCM_HOT_STACK_BYTES / 8u]
    __attribute__((section(".sbss.gNdsDtcmHotStack"), aligned(8)));
/* Nonzero while a context entered from outside holds frames on the hot stack.
 * A coroutine switch can leave those frames suspended; a hot-stack call from
 * another stack then runs in place on that stack instead of reusing the top
 * (linker/nds_hot_text.ld keeps the word in DTCM). */
u32 gNdsDtcmHotStackBusy;
extern unsigned int ndsDtcmHotStackCall(unsigned int (*fn)(void *), void *arg);

__asm__(
    "    .pushsection .itcm.ndsDtcmHotStackCall,\"ax\",%progbits\n"
    "    .arm\n"
    "    .align 2\n"
    "    .global ndsDtcmHotStackCall\n"
    "    .type ndsDtcmHotStackCall, %function\n"
    "ndsDtcmHotStackCall:\n"
    "    push  {r4, lr}\n"
    "    ldr   r2, =gNdsDtcmHotStack\n"
    "    ldr   r3, =gNdsDtcmHotStack+" NDS_DTCM_HOT_STACK_STR(NDS_DTCM_HOT_STACK_BYTES) "\n"
    "    cmp   sp, r2\n"
    "    bls   1f\n"
    "    cmp   sp, r3\n"
    "    bls   2f\n"
    "1:  ldr   r2, =gNdsDtcmHotStackBusy\n"
    "    ldr   ip, [r2]\n"
    "    cmp   ip, #0\n"
    "    bne   2f\n"
    "    mov   ip, #1\n"
    "    str   ip, [r2]\n"
    "    mov   r4, sp\n"
    "    mov   sp, r3\n"
    "    mov   ip, r0\n"
    "    mov   r0, r1\n"
    "    blx   ip\n"
    "    mov   sp, r4\n"
    "    ldr   r2, =gNdsDtcmHotStackBusy\n"
    "    mov   r1, #0\n"
    "    str   r1, [r2]\n"
    "    pop   {r4, pc}\n"
    "2:  mov   ip, r0\n"
    "    mov   r0, r1\n"
    "    blx   ip\n"
    "    pop   {r4, pc}\n"
    "    .ltorg\n"
    "    .size ndsDtcmHotStackCall, .-ndsDtcmHotStackCall\n"
    "    .popsection\n");

#if NDS_TICK_HUD
/* Lab: the stack's deepest reach. Painted once from outside it, scanned every
 * 256th entry from outside it (a call made on it would scan live frames). */
#define NDS_DTCM_HOT_STACK_PAINT 0x5ca1ab1eu
__attribute__((used)) volatile u32 gNdsDtcmHotStackHighWater;
__attribute__((used)) volatile u32 gNdsDtcmHotStackCalls;
static u32 sNdsDtcmHotStackPainted;

static void ndsDtcmHotStackLabNote(void)
{
    u32 *words = (u32 *)(void *)gNdsDtcmHotStack;
    u32 count = NDS_DTCM_HOT_STACK_BYTES / 4u;
    u32 sp;
    u32 i;

    __asm__ volatile ("mov %0, sp" : "=r"(sp));
    if (((sp > (u32)(uintptr_t)words) &&
         (sp <= (u32)(uintptr_t)(words + count))) ||
        (gNdsDtcmHotStackBusy != 0u))
    {
        return;
    }
    if (sNdsDtcmHotStackPainted == 0u)
    {
        for (i = 0u; i < count; i++)
        {
            words[i] = NDS_DTCM_HOT_STACK_PAINT;
        }
        sNdsDtcmHotStackPainted = 1u;
        return;
    }
    if ((gNdsDtcmHotStackCalls++ & 255u) != 0u)
    {
        return;
    }
    for (i = 0u; (i < count) && (words[i] == NDS_DTCM_HOT_STACK_PAINT); i++)
    {
    }
    if (((count - i) * 4u) > gNdsDtcmHotStackHighWater)
    {
        gNdsDtcmHotStackHighWater = (count - i) * 4u;
    }
}
#endif

/* ITCM: the fill census charged ~1.2M cycles of instruction fetch to the
 * trampoline pair in main RAM. */
unsigned int __attribute__((section(".itcm")))
ndsDtcmHotStackRun(unsigned int (*fn)(void *), void *arg)
{
#if NDS_TICK_HUD
    ndsDtcmHotStackLabNote();
#endif
    return ndsDtcmHotStackCall(fn, arg);
}


#if defined(NDS_LAB_STACK_PAINT) && NDS_LAB_STACK_PAINT
/* Lab (P8 S3, CPPFLAGS=-DNDS_LAB_STACK_PAINT=1): every coroutine stack is
 * painted when it is made, and its deepest reach is read when it is destroyed
 * and, for the live ones, on a match's first frame and every 256th after
 * (portCoroutineLabStackScan,
 * taskman_seam_battle_host.c). Per owner id (the OSThread id) the record keeps
 * the stack's size and the most of it ever used; gNdsCoroutineLabWorstFree is
 * the smallest unused share seen, in per mille. The stability audit's stacks
 * row (>= 25% free) reads them. */
#define NDS_COROUTINE_LAB_PAINT 0x5ca1ab1eu
#define NDS_COROUTINE_LAB_OWNERS 32u
#define NDS_COROUTINE_LAB_LIVE 64u
/* [i][0] owner id + 1 (0 = empty), [1] stack bytes, [2] most bytes used. */
__attribute__((used)) volatile u32 gNdsCoroutineLabStack[NDS_COROUTINE_LAB_OWNERS][3];
__attribute__((used)) volatile u32 gNdsCoroutineLabWorstFree = 1000u;
__attribute__((used)) volatile u32 gNdsCoroutineLabScans;
static PortCoroutine *sNdsCoroutineLabLive[NDS_COROUTINE_LAB_LIVE];

static void ndsCoroutineLabPaint(PortCoroutine *coroutine)
{
    u32 *words = (u32 *)coroutine->stack;
    u32 count = (u32)(coroutine->stack_size / 4u);
    u32 i;

    for (i = 0u; i < count; i++)
    {
        words[i] = NDS_COROUTINE_LAB_PAINT;
    }
    for (i = 0u; i < NDS_COROUTINE_LAB_LIVE; i++)
    {
        if (sNdsCoroutineLabLive[i] == NULL)
        {
            sNdsCoroutineLabLive[i] = coroutine;
            break;
        }
    }
}

/* The used bytes of a painted stack: from its top down to the first word
 * still holding the paint. */
static void ndsCoroutineLabNote(const PortCoroutine *coroutine)
{
    const u32 *words = (const u32 *)coroutine->stack;
    const u32 count = (u32)(coroutine->stack_size / 4u);
    const u32 owner = (u32)coroutine->lab_owner + 1u;
    u32 used;
    u32 i;

    for (i = 0u; (i < count) && (words[i] == NDS_COROUTINE_LAB_PAINT); i++)
    {
    }
    used = (count - i) * 4u;
    for (i = 0u; i < NDS_COROUTINE_LAB_OWNERS; i++)
    {
        if ((gNdsCoroutineLabStack[i][0] == owner) ||
            (gNdsCoroutineLabStack[i][0] == 0u))
        {
            gNdsCoroutineLabStack[i][0] = owner;
            gNdsCoroutineLabStack[i][1] = (u32)coroutine->stack_size;
            if (used > gNdsCoroutineLabStack[i][2])
            {
                gNdsCoroutineLabStack[i][2] = used;
            }
            break;
        }
    }
    if ((count != 0u) &&
        ((((count * 4u) - used) * 1000u) / (count * 4u)) < gNdsCoroutineLabWorstFree)
    {
        gNdsCoroutineLabWorstFree = (((count * 4u) - used) * 1000u) / (count * 4u);
    }
}

static void ndsCoroutineLabForget(PortCoroutine *coroutine)
{
    u32 i;

    ndsCoroutineLabNote(coroutine);
    for (i = 0u; i < NDS_COROUTINE_LAB_LIVE; i++)
    {
        if (sNdsCoroutineLabLive[i] == coroutine)
        {
            sNdsCoroutineLabLive[i] = NULL;
        }
    }
}

void portCoroutineLabStackScan(void)
{
    u32 i;

    gNdsCoroutineLabScans++;
    for (i = 0u; i < NDS_COROUTINE_LAB_LIVE; i++)
    {
        if (sNdsCoroutineLabLive[i] != NULL)
        {
            ndsCoroutineLabNote(sNdsCoroutineLabLive[i]);
        }
    }
}
#endif

extern void ndsCoroutineSwap(NDSCoroutineContext *from,
                             NDSCoroutineContext *to);
extern void ndsCoroutineTrampoline(void);

void portCoroutineTrampolineC(PortCoroutine *coroutine)
{
    coroutine->entry(coroutine->arg);
    coroutine->finished = 1;
    portCoroutineYield();

    while (1) {
    }
}

void portCoroutineInitMain(void)
{
    sCurrentCoroutine = NULL;
}

PortCoroutine *portCoroutineCreate(void (*entry)(void *), void *arg,
                                   size_t stack_size, int owner_id)
{
    PortCoroutine *coroutine;
    uintptr_t stack_top;
    (void)owner_id;

    if (entry == NULL) return NULL;
    if (stack_size < NDS_COROUTINE_MIN_STACK) {
        stack_size = NDS_COROUTINE_MIN_STACK;
    }
    stack_size = (stack_size + 7u) & ~7u;

    coroutine = calloc(1, sizeof(*coroutine));
    if (coroutine == NULL) return NULL;

    coroutine->stack = malloc(stack_size);
    if (coroutine->stack == NULL) {
        free(coroutine);
        return NULL;
    }

    coroutine->entry = entry;
    coroutine->arg = arg;
    coroutine->stack_size = stack_size;

    stack_top = ((uintptr_t)coroutine->stack + stack_size) & ~(uintptr_t)7u;
    coroutine->context.r4 = (u32)(uintptr_t)coroutine;
    coroutine->context.sp = (u32)stack_top;
    coroutine->context.lr = (u32)(uintptr_t)ndsCoroutineTrampoline;
#if defined(NDS_LAB_STACK_PAINT) && NDS_LAB_STACK_PAINT
    coroutine->lab_owner = owner_id;
    ndsCoroutineLabPaint(coroutine);
#endif

    return coroutine;
}

size_t portCoroutineStaticOverhead(void)
{
    return (sizeof(PortCoroutine) + 7u) & ~(size_t)7u;
}

PortCoroutine *portCoroutineCreateStatic(void (*entry)(void *), void *arg,
                                         void *base, size_t size, int owner_id)
{
    PortCoroutine *coroutine;
    uintptr_t top;
    size_t overhead = portCoroutineStaticOverhead();
    size_t stack_size;
    (void)owner_id;

    if (entry == NULL || base == NULL) return NULL;
    top = ((uintptr_t)base + size) & ~(uintptr_t)7u;
    if (top < ((uintptr_t)base + overhead + NDS_COROUTINE_MIN_STACK)) {
        return NULL;
    }
    top -= overhead;
    stack_size = (size_t)(top - (uintptr_t)base);

    coroutine = (PortCoroutine *)top;
    memset(coroutine, 0, sizeof(*coroutine));
    coroutine->entry = entry;
    coroutine->arg = arg;
    coroutine->stack = base;
    coroutine->stack_size = stack_size;
    coroutine->is_static = 1;

    /* The context sits directly above the stack, so `top` is both the initial
     * SP and the context's address. It is 8-aligned, which AAPCS requires. */
    coroutine->context.r4 = (u32)(uintptr_t)coroutine;
    coroutine->context.sp = (u32)top;
    coroutine->context.lr = (u32)(uintptr_t)ndsCoroutineTrampoline;
#if defined(NDS_LAB_STACK_PAINT) && NDS_LAB_STACK_PAINT
    coroutine->lab_owner = owner_id;
    ndsCoroutineLabPaint(coroutine);
#endif

    return coroutine;
}

void portCoroutineDestroy(PortCoroutine *coroutine)
{
    if (coroutine == NULL || coroutine == sCurrentCoroutine) return;
#if defined(NDS_LAB_STACK_PAINT) && NDS_LAB_STACK_PAINT
    ndsCoroutineLabForget(coroutine);
#endif
    if (coroutine->is_static) {
        /* The block belongs to the caller's pool (BattleShip recycles it
         * through gcEjectGObjStack). Clear it so a stale resume is impossible
         * and hand it back; there is nothing to free. */
        memset(coroutine, 0, sizeof(*coroutine));
        return;
    }
    free(coroutine->stack);
    memset(coroutine, 0, sizeof(*coroutine));
    free(coroutine);
}

void portCoroutineResume(PortCoroutine *coroutine)
{
    PortCoroutine *previous;

    if (coroutine == NULL || coroutine->finished) return;

    previous = sCurrentCoroutine;
    coroutine->caller = previous;
    sCurrentCoroutine = coroutine;
    ndsCoroutineSwap(&coroutine->caller_context, &coroutine->context);
    sCurrentCoroutine = previous;
}

void portCoroutineYield(void)
{
    PortCoroutine *coroutine = sCurrentCoroutine;

    if (coroutine == NULL) return;

    sCurrentCoroutine = coroutine->caller;
    ndsCoroutineSwap(&coroutine->context, &coroutine->caller_context);
    sCurrentCoroutine = coroutine;
}

int portCoroutineIsFinished(const PortCoroutine *coroutine)
{
    return coroutine == NULL || coroutine->finished;
}

int portCoroutineInCoroutine(void)
{
    return sCurrentCoroutine != NULL;
}

PortCoroutine *portCoroutineCurrent(void)
{
    return sCurrentCoroutine;
}

