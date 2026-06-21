#ifndef SSB64_NDS_COROUTINE_H
#define SSB64_NDS_COROUTINE_H

#include <stddef.h>

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

#endif

