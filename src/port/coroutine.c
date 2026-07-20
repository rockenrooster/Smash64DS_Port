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
};

static PortCoroutine *sCurrentCoroutine;

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

void portCoroutineDestroy(PortCoroutine *coroutine)
{
    if (coroutine == NULL || coroutine == sCurrentCoroutine) return;
#if NDS_TASK20_STACK_PROFILE
    portCoroutineTask20Retire(coroutine);
#endif
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
