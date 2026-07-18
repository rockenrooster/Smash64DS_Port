#ifndef SSB64_NDS_OS_BACKEND_H
#define SSB64_NDS_OS_BACKEND_H

#include <PR/ultratypes.h>

/* Resume each runnable/waiting emulated N64 thread once. */
void ndsOsRunThreads(void);
void ndsOsPostVBlank(void);

/* Boot-time architecture check; returns zero on success. */
int ndsOsSelfTest(void);

#if NDS_TASK20_STACK_PROFILE
extern volatile u32 gNdsTask20GameplayStackBase;
extern volatile u32 gNdsTask20GameplayStackSize;
extern volatile u32 gNdsTask20GameplayStackHighWater;
extern volatile u32 gNdsTask20MainStackBottom;
extern volatile u32 gNdsTask20MainStackPoisonStart;
extern volatile u32 gNdsTask20MainStackTop;
extern volatile u32 gNdsTask20MainStackHighWater;
extern volatile u32 gNdsTask20SampleCount;
extern volatile u32 gNdsTask20SampleRequest;
#endif

#endif
