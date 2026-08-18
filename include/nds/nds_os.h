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

/* Drop every registered thread whose OSThread lives inside [base, base+size),
 * returning how many registry slots that cleared.
 *
 * A GObj thread's OSThread and its coroutine block are drawn from the taskman
 * general arena: objman.c:810-818 hands osCreateThread a stack top inside a
 * GObjThreadStack that syTaskmanMalloc carved out of it. That arena is rewound
 * on every scene entry (decomp sys/taskman.c:1227 -> :258), so the instant a
 * scene starts, any registered thread inside it is storage the next scene is
 * about to reuse -- while sThreads[] still points at it and ndsOsRunThreads
 * still dereferences it.
 *
 * BattleShip has the matching contract by construction: objman.c:918 destroys a
 * GObj process's thread before ejecting its stack, so nothing arena-resident
 * survives a scene. The port keeps a private registry the source does not have
 * and nothing tied it to the arena's lifetime; that is the defect this closes
 * (P2-1b-1). Called from ndsSceneManagerEnter BEFORE the rewind, so the structs
 * inspected here are still intact -- and nothing is WRITTEN through them, since
 * a courtesy `port_registered = FALSE` would be a write into the next scene's
 * memory. */
u32 ndsOsForgetThreadsInArena(const void *base, u32 size);

/* Registry slots cleared by the above, the scene entries that cleared at least
 * one, and the id of the last thread dropped. BattleShip hands GObj threads ids
 * from `dGCProcessThreadID` (10000000 up), so a drop id in that range is the
 * proof the dropped entries were GObj threads and not service threads. All
 * three read 0 through the first scene entry of a run, which is this fix's
 * negative control. */
extern volatile u32 gNdsOsArenaThreadsDropped;
extern volatile u32 gNdsOsArenaThreadDropEntries;
extern volatile u32 gNdsOsArenaThreadDropLastId;

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
