#include <stdlib.h>
#include <string.h>

#include <PR/os.h>
#include <nds/nds_os.h>
#include <port/coroutine.h>

#define NDS_OS_MAX_THREADS 64
#define NDS_OS_SERVICE_STACK_SIZE (16u * 1024u)
#define NDS_OS_GOBJ_STACK_SIZE (4u * 1024u)
/* BattleShip's own split: main.c's service threads are 1..6, and objman hands
 * GObj threads ids from 10000000 up (`dGCProcessThreadID`). The port's two
 * private threads (os_selftest 90, video_bootstrap 91) sit below the line and
 * pass no stack, so they stay on the service path. */
#define NDS_OS_GOBJ_THREAD_ID_MIN 100

static OSThread *sThreads[NDS_OS_MAX_THREADS];

#if NDS_TASK20_STACK_PROFILE
volatile u32 gNdsTask20GameplayStackBase;
volatile u32 gNdsTask20GameplayStackSize;
volatile u32 gNdsTask20GameplayStackHighWater;
volatile u32 gNdsTask20MainStackBottom;
volatile u32 gNdsTask20MainStackPoisonStart;
volatile u32 gNdsTask20MainStackTop;
volatile u32 gNdsTask20MainStackHighWater;
volatile u32 gNdsTask20SampleCount;
/* Consume one suspension-safe startup census before any timed phase. */
volatile u32 gNdsTask20SampleRequest = 1u;

static void ndsOsTask20SampleStack(const OSThread *thread, s32 finished)
{
    size_t high_water;

    if (thread == NULL || thread->port_coroutine == NULL) return;
    if (finished != FALSE) {
        portCoroutineTask20Sample(thread->port_coroutine);
    }
    if (thread->id != 5) return;
    if ((gNdsTask20SampleRequest == 0u) && (finished == FALSE)) return;
    gNdsTask20SampleRequest = 0u;

    portCoroutineTask20Sample(thread->port_coroutine);

    gNdsTask20GameplayStackBase = (u32)(uintptr_t)
        portCoroutineStackBase(thread->port_coroutine);
    gNdsTask20GameplayStackSize = (u32)
        portCoroutineStackSize(thread->port_coroutine);
    high_water = portCoroutineStackHighWater(thread->port_coroutine);
    if (high_water > gNdsTask20GameplayStackHighWater) {
        gNdsTask20GameplayStackHighWater = (u32)high_water;
    }
    gNdsTask20MainStackBottom = (u32)portCoroutineMainStackBottom();
    gNdsTask20MainStackPoisonStart =
        (u32)portCoroutineMainStackPoisonStart();
    gNdsTask20MainStackTop = (u32)portCoroutineMainStackTop();
    high_water = portCoroutineMainStackHighWater();
    if (high_water > gNdsTask20MainStackHighWater) {
        gNdsTask20MainStackHighWater = (u32)high_water;
    }
    gNdsTask20SampleCount++;
}
#endif

static OSThread *ndsOsCurrentThread(void)
{
    PortCoroutine *current = portCoroutineCurrent();
    s32 i;

    if (current == NULL) return NULL;
    for (i = 0; i < NDS_OS_MAX_THREADS; i++) {
        if (sThreads[i] != NULL && sThreads[i]->port_coroutine == current) {
            return sThreads[i];
        }
    }
    return NULL;
}

static void ndsOsRegisterThread(OSThread *thread)
{
    s32 i;

    for (i = 0; i < NDS_OS_MAX_THREADS; i++) {
        if (sThreads[i] == NULL) {
            sThreads[i] = thread;
            thread->port_registered = TRUE;
            return;
        }
    }
}

static void ndsOsUnregisterThread(OSThread *thread)
{
    s32 i;

    for (i = 0; i < NDS_OS_MAX_THREADS; i++) {
        if (sThreads[i] == thread) {
            sThreads[i] = NULL;
            thread->port_registered = FALSE;
            return;
        }
    }
}

static void ndsOsThreadEntry(void *arg)
{
    OSThread *thread = arg;

    thread->state = OS_STATE_RUNNING;
    thread->port_entry(thread->port_arg);
    thread->state = OS_STATE_STOPPED;
}

size_t ndsOsGObjThreadBlockBytes(void)
{
    return (size_t)NDS_OS_GOBJ_STACK_SIZE + portCoroutineStaticOverhead();
}

void osCreateThread(OSThread *thread, OSId id, void (*entry)(void *),
                    void *arg, void *sp, OSPri priority)
{
    memset(thread, 0, sizeof(*thread));
    thread->id = id;
    thread->priority = priority;
    thread->state = OS_STATE_STOPPED;
    thread->port_entry = entry;
    thread->port_arg = arg;
    thread->context.pc = (u32)(uintptr_t)entry;

    /* Provision the GObj thread's coroutine HERE, out of the pooled block the
     * caller already owns, instead of mallocing one at first start.
     *
     * `sp` is BattleShip's own thread stack top -- objman.c:882 passes
     * `&gobjthread->stack[sGCThreadStackSize / sizeof(u64)]`, pointing one past
     * a GObjStack drawn from the taskman arena and recycled through
     * gcEjectGObjStack. The port used to throw it away and malloc a private
     * 4 KiB stack at the first `osStartThread`, which is the divergence that
     * froze the 2026-08-03 battle: that malloc can fail mid-match, and the
     * failure was silent while gcRunGObjProcess had already committed to a
     * blocking osRecvMesg only the started thread could satisfy.
     *
     * Using the pooled block restores the source's ordering (storage exists
     * before the thread does), costs the C heap nothing, and makes
     * `gobjthread->stack[7] == 0xFEDCBA98` -- written by the caller right after
     * this returns, 56 bytes above the base the stack grows down toward -- a
     * live overflow guard for the port too. The objman seam clamps
     * `gobjthreadstack_size` to ndsOsGObjThreadBlockBytes(), so a NULL here is
     * a build-time sizing defect, never a runtime shortage. */
    if ((id >= NDS_OS_GOBJ_THREAD_ID_MIN) && (sp != NULL) && (entry != NULL)) {
        size_t block = ndsOsGObjThreadBlockBytes();

        thread->port_coroutine = portCoroutineCreateStatic(
            ndsOsThreadEntry, thread, (void *)((u8 *)sp - block), block,
            (int)id);
        if (thread->port_coroutine != NULL) {
            gNdsOsGObjThreadProvisionCount++;
        } else {
            gNdsOsGObjThreadProvisionFailCount++;
        }
    }

    ndsOsRegisterThread(thread);
}

void osStartThread(OSThread *thread)
{
    PortCoroutine *coroutine;
    size_t stack_size;

    if (thread == NULL || thread->port_entry == NULL) {
        gNdsOsStartThreadNoEntryCount++;
        return;
    }

    coroutine = thread->port_coroutine;
    if (coroutine == NULL) {
        /* Service threads only: they are created once at boot, before anything
         * competes for the heap. A GObj thread reaching this is the old defect
         * and StartCreateFail is where it becomes visible instead of silent. */
        stack_size = (thread->id < NDS_OS_GOBJ_THREAD_ID_MIN)
            ? NDS_OS_SERVICE_STACK_SIZE : NDS_OS_GOBJ_STACK_SIZE;
        coroutine = portCoroutineCreate(ndsOsThreadEntry, thread, stack_size,
                                        thread->id);
        if (coroutine == NULL) {
            gNdsOsStartThreadCreateFailCount++;
            return;
        }
        gNdsOsThreadHeapCreateCount++;
        thread->port_coroutine = coroutine;
    }

    if (!portCoroutineIsFinished(coroutine)) {
        thread->state = OS_STATE_RUNNABLE;
        portCoroutineResume(coroutine);
#if NDS_TASK20_STACK_PROFILE
        ndsOsTask20SampleStack(thread, portCoroutineIsFinished(coroutine));
#endif
        if (portCoroutineIsFinished(coroutine)) {
            thread->state = OS_STATE_STOPPED;
        }
    }
}

void osStopThread(OSThread *thread)
{
    OSThread *current = ndsOsCurrentThread();

    if (thread == NULL) thread = current;
    if (thread == NULL) return;

    thread->state = OS_STATE_STOPPED;
    if (thread == current) {
        portCoroutineYield();
        thread->state = OS_STATE_RUNNING;
    }
}

void osDestroyThread(OSThread *thread)
{
    if (thread == NULL) thread = ndsOsCurrentThread();
    if (thread == NULL) return;

    if (thread == ndsOsCurrentThread()) {
        thread->state = OS_STATE_STOPPED;
        portCoroutineYield();
        return;
    }

    portCoroutineDestroy(thread->port_coroutine);
    thread->port_coroutine = NULL;
    thread->state = OS_STATE_STOPPED;
    ndsOsUnregisterThread(thread);
}

void osYieldThread(void)
{
    OSThread *thread = ndsOsCurrentThread();

    if (thread != NULL) thread->state = OS_STATE_RUNNABLE;
    portCoroutineYield();
}

OSId osGetThreadId(OSThread *thread)
{
    if (thread == NULL) thread = ndsOsCurrentThread();
    return (thread != NULL) ? thread->id : 0;
}

void osSetThreadPri(OSThread *thread, OSPri priority)
{
    if (thread == NULL) thread = ndsOsCurrentThread();
    if (thread != NULL) {
        thread->priority = priority;

        /* BattleShip's idle thread lowers itself immediately before its
         * permanent N64 idle loop. Park it here so that loop is never entered
         * on the cooperative DS backend. */
        if (priority == OS_PRIORITY_IDLE &&
            thread == ndsOsCurrentThread()) {
            thread->state = OS_STATE_STOPPED;
            portCoroutineYield();
            thread->state = OS_STATE_RUNNING;
        }
    }
}

OSPri osGetThreadPri(OSThread *thread)
{
    if (thread == NULL) thread = ndsOsCurrentThread();
    return (thread != NULL) ? thread->priority : OS_PRIORITY_IDLE;
}

void osCreateMesgQueue(OSMesgQueue *queue, OSMesg *buffer, s32 count)
{
    queue->mtqueue = NULL;
    queue->fullqueue = NULL;
    queue->validCount = 0;
    queue->first = 0;
    queue->msgCount = count;
    queue->msg = buffer;
}

static s32 ndsOsWaitForQueue(OSMesgQueue *queue, s32 wait_for_space,
                             s32 flag)
{
    OSThread *thread;

    while (wait_for_space ? MQ_IS_FULL(queue) : MQ_IS_EMPTY(queue)) {
        if (flag == OS_MESG_NOBLOCK || !portCoroutineInCoroutine()) {
            return -1;
        }
        thread = ndsOsCurrentThread();
        if (thread != NULL) thread->state = OS_STATE_WAITING;
        portCoroutineYield();
        if (thread != NULL) thread->state = OS_STATE_RUNNING;
    }
    return 0;
}

s32 osSendMesg(OSMesgQueue *queue, OSMesg message, s32 flag)
{
    s32 index;

    if (queue == NULL || queue->msg == NULL || queue->msgCount <= 0) return -1;
    if (ndsOsWaitForQueue(queue, TRUE, flag) != 0) return -1;

    index = (queue->first + queue->validCount) % queue->msgCount;
    queue->msg[index] = message;
    queue->validCount++;
    return 0;
}

s32 osJamMesg(OSMesgQueue *queue, OSMesg message, s32 flag)
{
    if (queue == NULL || queue->msg == NULL || queue->msgCount <= 0) return -1;
    if (ndsOsWaitForQueue(queue, TRUE, flag) != 0) return -1;

    queue->first = (queue->first + queue->msgCount - 1) % queue->msgCount;
    queue->msg[queue->first] = message;
    queue->validCount++;
    return 0;
}

s32 osRecvMesg(OSMesgQueue *queue, OSMesg *message, s32 flag)
{
    if (queue == NULL || queue->msg == NULL || queue->msgCount <= 0) return -1;
    if (ndsOsWaitForQueue(queue, FALSE, flag) != 0) return -1;

    if (message != NULL) *message = queue->msg[queue->first];
    queue->first = (queue->first + 1) % queue->msgCount;
    queue->validCount--;
    return 0;
}

void ndsOsRunThreads(void)
{
    s32 i;

    for (i = 0; i < NDS_OS_MAX_THREADS; i++) {
        OSThread *thread = sThreads[i];
        PortCoroutine *coroutine;

        if (thread == NULL) continue;
        coroutine = thread->port_coroutine;
        if (coroutine == NULL || portCoroutineIsFinished(coroutine)) continue;
        if (thread->state != OS_STATE_WAITING &&
            thread->state != OS_STATE_RUNNABLE) continue;

        thread->state = OS_STATE_RUNNING;
        portCoroutineResume(coroutine);
#if NDS_TASK20_STACK_PROFILE
        ndsOsTask20SampleStack(thread, portCoroutineIsFinished(coroutine));
#endif
        if (portCoroutineIsFinished(coroutine)) {
            thread->state = OS_STATE_STOPPED;
        }
    }
}
