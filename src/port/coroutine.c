#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <PR/ultratypes.h>
#include <port/coroutine.h>

#define NDS_COROUTINE_MIN_STACK 4096u
#if NDS_TASK20_STACK_PROFILE
#define NDS_TASK20_STACK_POISON 0xa5u
#endif

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

#if NDS_TASK20_STACK_PROFILE
extern u8 __dtcm_bss_end[];
extern u8 __sp_usr[];
static uintptr_t sMainStackPoisonStart;

volatile NDSTask20CoroutineCensusRow
    gNdsTask20CoroutineCensus[NDS_TASK20_COROUTINE_CENSUS_CAPACITY];
volatile uint32_t gNdsTask20CoroutineCensusCount;
volatile uint32_t gNdsTask20CoroutineCensusOverflowCount;
volatile uint32_t gNdsTask20CoroutineCensusLiveCount;
volatile uint32_t gNdsTask20CoroutineCensusPeakLiveCount;
volatile uint32_t gNdsTask20CoroutineCensusLargeLiveCount;
volatile uint32_t gNdsTask20CoroutineCensusPeakLargeLiveCount;
#endif

extern void ndsCoroutineSwap(NDSCoroutineContext *from,
                             NDSCoroutineContext *to);
extern void ndsCoroutineTrampoline(void);
#if NDS_TASK20_STACK_PROFILE
extern uintptr_t ndsCoroutineReadSp(void);
extern void ndsCoroutinePoisonWords(void *start, void *end, u32 value);
static size_t portCoroutinePoisonHighWater(const void *stack,
                                           size_t stack_size)
{
    const volatile u8 *bytes = stack;
    size_t i;

    for (i = 0; i < stack_size; i++) {
        if (bytes[i] != NDS_TASK20_STACK_POISON) break;
    }
    return stack_size - i;
}

static volatile NDSTask20CoroutineCensusRow *
portCoroutineTask20FindRow(const PortCoroutine *coroutine)
{
    uint32_t i;
    uint32_t address = (uint32_t)(uintptr_t)coroutine;

    for (i = 0; i < gNdsTask20CoroutineCensusCount; i++) {
        if (gNdsTask20CoroutineCensus[i].coroutine_address == address) {
            return &gNdsTask20CoroutineCensus[i];
        }
    }
    return NULL;
}

static void portCoroutineTask20Register(PortCoroutine *coroutine,
                                        int owner_id,
                                        size_t requested_stack_size)
{
    volatile NDSTask20CoroutineCensusRow *row;
    uint32_t index = gNdsTask20CoroutineCensusCount;

    gNdsTask20CoroutineCensusLiveCount++;
    if (gNdsTask20CoroutineCensusLiveCount >
        gNdsTask20CoroutineCensusPeakLiveCount) {
        gNdsTask20CoroutineCensusPeakLiveCount =
            gNdsTask20CoroutineCensusLiveCount;
    }
    if (coroutine->stack_size >= (16u * 1024u)) {
        gNdsTask20CoroutineCensusLargeLiveCount++;
        if (gNdsTask20CoroutineCensusLargeLiveCount >
            gNdsTask20CoroutineCensusPeakLargeLiveCount) {
            gNdsTask20CoroutineCensusPeakLargeLiveCount =
                gNdsTask20CoroutineCensusLargeLiveCount;
        }
    }

    if (index >= NDS_TASK20_COROUTINE_CENSUS_CAPACITY) {
        gNdsTask20CoroutineCensusOverflowCount++;
        return;
    }
    gNdsTask20CoroutineCensusCount = index + 1u;
    row = &gNdsTask20CoroutineCensus[index];
    row->owner_id = owner_id;
    row->requested_stack_size = (uint32_t)requested_stack_size;
    row->actual_stack_size = (uint32_t)coroutine->stack_size;
    row->stack_base = (uint32_t)(uintptr_t)coroutine->stack;
    row->coroutine_address = (uint32_t)(uintptr_t)coroutine;
    row->state = 1u;
}

void portCoroutineTask20Sample(PortCoroutine *coroutine)
{
    volatile NDSTask20CoroutineCensusRow *row;
    size_t high_water;

    if (coroutine == NULL || coroutine->stack == NULL) return;
    row = portCoroutineTask20FindRow(coroutine);
    if (row == NULL) return;
    high_water = portCoroutinePoisonHighWater(coroutine->stack,
                                              coroutine->stack_size);
    if (high_water > row->high_water) {
        row->high_water = (uint32_t)high_water;
    }
}

static void portCoroutineTask20Retire(PortCoroutine *coroutine)
{
    volatile NDSTask20CoroutineCensusRow *row;

    if (coroutine == NULL) return;
    portCoroutineTask20Sample(coroutine);
    row = portCoroutineTask20FindRow(coroutine);
    if (row != NULL) row->state = 2u;
    if (gNdsTask20CoroutineCensusLiveCount != 0u) {
        gNdsTask20CoroutineCensusLiveCount--;
    }
    if ((coroutine->stack_size >= (16u * 1024u)) &&
        (gNdsTask20CoroutineCensusLargeLiveCount != 0u)) {
        gNdsTask20CoroutineCensusLargeLiveCount--;
    }
}
#endif

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
#if NDS_TASK20_STACK_PROFILE
    sMainStackPoisonStart = ndsCoroutineReadSp() & ~(uintptr_t)3u;
    if (sMainStackPoisonStart > (uintptr_t)__dtcm_bss_end) {
        ndsCoroutinePoisonWords(__dtcm_bss_end,
                                (void *)sMainStackPoisonStart,
                                0xa5a5a5a5u);
    }
#endif
}

PortCoroutine *portCoroutineCreate(void (*entry)(void *), void *arg,
                                   size_t stack_size, int owner_id)
{
    PortCoroutine *coroutine;
    uintptr_t stack_top;
#if NDS_TASK20_STACK_PROFILE
    size_t requested_stack_size = stack_size;
#else
    (void)owner_id;
#endif

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
#if NDS_TASK20_STACK_PROFILE
    memset(coroutine->stack, NDS_TASK20_STACK_POISON, stack_size);
#endif

    stack_top = ((uintptr_t)coroutine->stack + stack_size) & ~(uintptr_t)7u;
    coroutine->context.r4 = (u32)(uintptr_t)coroutine;
    coroutine->context.sp = (u32)stack_top;
    coroutine->context.lr = (u32)(uintptr_t)ndsCoroutineTrampoline;
#if NDS_TASK20_STACK_PROFILE
    portCoroutineTask20Register(coroutine, owner_id,
                                requested_stack_size);
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
#if !NDS_TASK20_STACK_PROFILE
    (void)owner_id;
#endif

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
#if NDS_TASK20_STACK_PROFILE
    memset(base, NDS_TASK20_STACK_POISON, stack_size);
#endif

    /* The context sits directly above the stack, so `top` is both the initial
     * SP and the context's address. It is 8-aligned, which AAPCS requires. */
    coroutine->context.r4 = (u32)(uintptr_t)coroutine;
    coroutine->context.sp = (u32)top;
    coroutine->context.lr = (u32)(uintptr_t)ndsCoroutineTrampoline;
#if NDS_TASK20_STACK_PROFILE
    portCoroutineTask20Register(coroutine, owner_id, stack_size);
#endif

    return coroutine;
}

void portCoroutineDestroy(PortCoroutine *coroutine)
{
    if (coroutine == NULL || coroutine == sCurrentCoroutine) return;
#if NDS_TASK20_STACK_PROFILE
    portCoroutineTask20Retire(coroutine);
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

#if NDS_TASK20_STACK_PROFILE
size_t portCoroutineStackHighWater(const PortCoroutine *coroutine)
{
    if (coroutine == NULL || coroutine->stack == NULL) return 0;
    return portCoroutinePoisonHighWater(coroutine->stack,
                                        coroutine->stack_size);
}

void *portCoroutineStackBase(const PortCoroutine *coroutine)
{
    return (coroutine != NULL) ? coroutine->stack : NULL;
}

size_t portCoroutineStackSize(const PortCoroutine *coroutine)
{
    return (coroutine != NULL) ? coroutine->stack_size : 0;
}

size_t portCoroutineMainStackHighWater(void)
{
    size_t poisoned_size;

    if (sMainStackPoisonStart <= (uintptr_t)__dtcm_bss_end) return 0;
    poisoned_size = sMainStackPoisonStart - (uintptr_t)__dtcm_bss_end;
    return ((uintptr_t)__sp_usr - sMainStackPoisonStart) +
        portCoroutinePoisonHighWater(__dtcm_bss_end, poisoned_size);
}

uintptr_t portCoroutineMainStackPoisonStart(void)
{
    return sMainStackPoisonStart;
}

uintptr_t portCoroutineMainStackBottom(void)
{
    return (uintptr_t)__dtcm_bss_end;
}

uintptr_t portCoroutineMainStackTop(void)
{
    return (uintptr_t)__sp_usr;
}
#endif
