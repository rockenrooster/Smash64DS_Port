#ifndef SSB64_NDS_COROUTINE_H
#define SSB64_NDS_COROUTINE_H

#include <stddef.h>
#if NDS_TASK20_STACK_PROFILE
#include <stdint.h>
#endif

typedef struct PortCoroutine PortCoroutine;

void portCoroutineInitMain(void);
PortCoroutine *portCoroutineCreate(void (*entry)(void *), void *arg,
                                   size_t stack_size);
void portCoroutineDestroy(PortCoroutine *coroutine);
void portCoroutineResume(PortCoroutine *coroutine);
void portCoroutineYield(void);
int portCoroutineIsFinished(const PortCoroutine *coroutine);
int portCoroutineInCoroutine(void);
PortCoroutine *portCoroutineCurrent(void);

#if NDS_TASK20_STACK_PROFILE
/* Task 20 profile-only stack watermarks. */
size_t portCoroutineStackHighWater(const PortCoroutine *coroutine);
void *portCoroutineStackBase(const PortCoroutine *coroutine);
size_t portCoroutineStackSize(const PortCoroutine *coroutine);
size_t portCoroutineMainStackHighWater(void);
uintptr_t portCoroutineMainStackPoisonStart(void);
uintptr_t portCoroutineMainStackBottom(void);
uintptr_t portCoroutineMainStackTop(void);
#endif

#endif
