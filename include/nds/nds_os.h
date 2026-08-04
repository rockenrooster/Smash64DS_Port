#ifndef SSB64_NDS_OS_BACKEND_H
#define SSB64_NDS_OS_BACKEND_H

#include <stddef.h>

#include <PR/ultratypes.h>

/* Resume each runnable/waiting emulated N64 thread once. */
void ndsOsRunThreads(void);
void ndsOsPostVBlank(void);

/* Boot-time architecture check; returns zero on success. */
int ndsOsSelfTest(void);

/* Bytes a GObj thread's pooled stack block must hold: the coroutine stack plus
 * the coroutine context carved off its top. BattleShip sizes that block from
 * `GCSetup.gobjthreadstack_size`, so the objman seam clamps it to this. */
size_t ndsOsGObjThreadBlockBytes(void);

/* Thread provisioning census.
 *
 * BattleShip cannot fail to start a GObj thread: objman.c:2376 seeds the thread
 * pool at scene setup and each GObjThread carries its stack inline, so
 * gcRunGObjProcess's `osStartThread` then `osRecvMesg(OS_MESG_BLOCK)` handshake
 * is safe by construction. The port used to malloc the coroutine lazily at
 * first start and RETURN SILENTLY on failure -- and on 2026-08-03 that is
 * exactly what happened to the battle announcer at logic frame 390, on a heap
 * measured with zero headroom: the thread never ran, never posted, and the
 * battle scene coroutine parked in osRecvMesg forever.
 *
 * GObjProvision counts coroutines built in the caller's pooled block at
 * osCreateThread -- the natural path, and the thing the fix does. Everything
 * else must read 0 except HeapCreate, which is the boot service threads:
 *   GObjProvisionFail  a pooled block too small for the coroutine -- a sizing
 *                      defect in the objman seam, not a runtime condition
 *   StartNoEntry       osStartThread on a thread with no entry point
 *   StartCreateFail    the lazy heap path failed, i.e. the original defect */
extern volatile u32 gNdsOsGObjThreadProvisionCount;
extern volatile u32 gNdsOsGObjThreadProvisionFailCount;
extern volatile u32 gNdsOsThreadHeapCreateCount;
extern volatile u32 gNdsOsStartThreadNoEntryCount;
extern volatile u32 gNdsOsStartThreadCreateFailCount;

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
