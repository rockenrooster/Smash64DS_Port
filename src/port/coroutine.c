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
};

static PortCoroutine *sCurrentCoroutine;

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
                                   size_t stack_size)
{
    PortCoroutine *coroutine;
    uintptr_t stack_top;

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

    return coroutine;
}

void portCoroutineDestroy(PortCoroutine *coroutine)
{
    if (coroutine == NULL || coroutine == sCurrentCoroutine) return;
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
