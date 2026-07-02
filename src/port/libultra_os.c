#include <stdlib.h>
#include <string.h>

#include <PR/os.h>
#include <nds/nds_os.h>
#include <port/coroutine.h>

#define NDS_OS_MAX_THREADS 64
#define NDS_OS_SERVICE_STACK_SIZE (16u * 1024u)
#define NDS_OS_GOBJ_STACK_SIZE (16u * 1024u)

static OSThread *sThreads[NDS_OS_MAX_THREADS];

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

void osCreateThread(OSThread *thread, OSId id, void (*entry)(void *),
                    void *arg, void *sp, OSPri priority)
{
    (void)sp;
    memset(thread, 0, sizeof(*thread));
    thread->id = id;
    thread->priority = priority;
    thread->state = OS_STATE_STOPPED;
    thread->port_entry = entry;
    thread->port_arg = arg;
    thread->context.pc = (u32)(uintptr_t)entry;
    ndsOsRegisterThread(thread);
}

void osStartThread(OSThread *thread)
{
    PortCoroutine *coroutine;
    size_t stack_size;

    if (thread == NULL || thread->port_entry == NULL) return;

    coroutine = thread->port_coroutine;
    if (coroutine == NULL) {
        stack_size = (thread->id < 100) ? NDS_OS_SERVICE_STACK_SIZE
                                        : NDS_OS_GOBJ_STACK_SIZE;
        coroutine = portCoroutineCreate(ndsOsThreadEntry, thread, stack_size);
        if (coroutine == NULL) return;
        thread->port_coroutine = coroutine;
    }

    if (!portCoroutineIsFinished(coroutine)) {
        thread->state = OS_STATE_RUNNABLE;
        portCoroutineResume(coroutine);
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
        if (portCoroutineIsFinished(coroutine)) {
            thread->state = OS_STATE_STOPPED;
        }
    }
}
