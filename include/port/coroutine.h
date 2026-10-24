/* Port coroutines (src/port/coroutine.c, coroutine_arm.s): create, resume,
 * yield and destroy, and the DTCM hot stack. */

#ifndef SSB64_NDS_COROUTINE_H
#define SSB64_NDS_COROUTINE_H

#include <stddef.h>

typedef struct PortCoroutine PortCoroutine;

void portCoroutineInitMain(void);
PortCoroutine *portCoroutineCreate(void (*entry)(void *), void *arg,
                                   size_t stack_size, int owner_id);

/* Bytes of a caller-supplied block that portCoroutineCreateStatic keeps for its
 * own context, carved off the top. Add this to the usable stack size to get the
 * block size to reserve. */
size_t portCoroutineStaticOverhead(void);

/* Build a coroutine entirely inside `[base, base + size)` -- no heap at all.
 * The context lives at the top of the block and the remainder is the stack, so
 * a caller that already owns pooled, recycled storage (BattleShip's GObjStack
 * pool) never has to allocate to start a thread. Returns NULL only when the
 * block is structurally unusable (too small, or no entry), which is a build-time
 * sizing error rather than a runtime condition.
 *
 * portCoroutineDestroy recognises these and returns the storage to the caller
 * untouched instead of calling free(). */
PortCoroutine *portCoroutineCreateStatic(void (*entry)(void *), void *arg,
                                         void *base, size_t size, int owner_id);
void portCoroutineDestroy(PortCoroutine *coroutine);
void portCoroutineResume(PortCoroutine *coroutine);
void portCoroutineYield(void);
int portCoroutineIsFinished(const PortCoroutine *coroutine);
int portCoroutineInCoroutine(void);
PortCoroutine *portCoroutineCurrent(void);
#if defined(NDS_LAB_STACK_PAINT) && NDS_LAB_STACK_PAINT
/* Lab (P8 S3): note every live coroutine's deepest stack reach. */
void portCoroutineLabStackScan(void);
#endif

/* Run fn(arg) on the DTCM hot stack and return its result. A call made while
 * already on that stack runs in place. fn must not yield a coroutine or hand
 * a stack address to DMA, the ARM7 or storage: none of them can see DTCM. */
unsigned int ndsDtcmHotStackRun(unsigned int (*fn)(void *), void *arg);


#endif
