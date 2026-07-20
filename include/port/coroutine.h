#ifndef SSB64_NDS_COROUTINE_H
#define SSB64_NDS_COROUTINE_H

#include <stddef.h>
#if NDS_TASK20_STACK_PROFILE
#include <stdint.h>
#endif

typedef struct PortCoroutine PortCoroutine;

void portCoroutineInitMain(void);
PortCoroutine *portCoroutineCreate(void (*entry)(void *), void *arg,
                                   size_t stack_size, int owner_id);
void portCoroutineDestroy(PortCoroutine *coroutine);
void portCoroutineResume(PortCoroutine *coroutine);
void portCoroutineYield(void);
int portCoroutineIsFinished(const PortCoroutine *coroutine);
int portCoroutineInCoroutine(void);
PortCoroutine *portCoroutineCurrent(void);

#if NDS_TASK20_STACK_PROFILE
/* Task 20 profile-only stack watermarks. */
#define NDS_TASK20_COROUTINE_CENSUS_CAPACITY 64u

typedef struct NDSTask20CoroutineCensusRow {
    int32_t owner_id;
    uint32_t requested_stack_size;
    uint32_t actual_stack_size;
    uint32_t stack_base;
    uint32_t coroutine_address;
    uint32_t state;
    uint32_t high_water;
} NDSTask20CoroutineCensusRow;

extern volatile NDSTask20CoroutineCensusRow
    gNdsTask20CoroutineCensus[NDS_TASK20_COROUTINE_CENSUS_CAPACITY];
extern volatile uint32_t gNdsTask20CoroutineCensusCount;
extern volatile uint32_t gNdsTask20CoroutineCensusOverflowCount;
extern volatile uint32_t gNdsTask20CoroutineCensusLiveCount;
extern volatile uint32_t gNdsTask20CoroutineCensusPeakLiveCount;
extern volatile uint32_t gNdsTask20CoroutineCensusLargeLiveCount;
extern volatile uint32_t gNdsTask20CoroutineCensusPeakLargeLiveCount;

void portCoroutineTask20Sample(PortCoroutine *coroutine);
size_t portCoroutineStackHighWater(const PortCoroutine *coroutine);
void *portCoroutineStackBase(const PortCoroutine *coroutine);
size_t portCoroutineStackSize(const PortCoroutine *coroutine);
size_t portCoroutineMainStackHighWater(void);
uintptr_t portCoroutineMainStackPoisonStart(void);
uintptr_t portCoroutineMainStackBottom(void);
uintptr_t portCoroutineMainStackTop(void);
#endif

#endif
