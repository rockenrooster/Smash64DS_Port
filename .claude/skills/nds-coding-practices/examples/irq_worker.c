/* Standalone ARM9 / Calico IRQ-to-worker demonstration.
 * This owns a VBlank callback; do not replace an existing handler in a port.
 * Usually a threadWaitForVBlank loop alone is simpler. Use this pattern when
 * actual ISR capture must wake a worker. No lossless-event guarantee: a full
 * one-slot mailbox deliberately coalesces WORK hints.
 */
#include <nds.h>
#include <stdint.h>

static Mailbox g_work;
static u32 g_slots[1];
static Thread g_worker;
static uint8_t g_stack[4096] __attribute__((aligned(8)));
enum { WORK_STOP = 0, WORK_HINT = 1 };

static void on_vblank(void)
{
    /* Documented IRQ-safe exception. Failure means a hint is already queued;
     * no event payload is being silently lost. Never retry inside this ISR.
     */
    (void)mailboxTrySend(&g_work, WORK_HINT);
}

static int worker_main(void *argument)
{
    (void)argument;
    unsigned hints = 0;
    for (;;) {
        const u32 message = mailboxRecv(&g_work); /* Blocks when empty. */
        if (message == WORK_STOP) return 0;
        /* Do the deferred, bounded job here, outside IRQ mode. Logging once
         * per 60 consumed hints demonstrates worker context, not elapsed FPS.
         * Remove diagnostics from performance builds.
         */
        if (++hints == 60u) {
            nocashMessage("IRQ work processed in a Calico thread");
            hints = 0;
        }
    }
}

int main(void)
{
    /* Reserve at least 2048 bytes for this demonstration's call stack after
     * TLS. This is a bounded example, not a universal worker stack size.
     */
    if (threadGetLocalStorageSize() > sizeof g_stack - 2048u) return 1;
    mailboxPrepare(&g_work, g_slots, sizeof g_slots / sizeof g_slots[0]);
    threadPrepare(&g_worker, worker_main, NULL,
                  g_stack + sizeof g_stack, MAIN_THREAD_PRIO + 1);
    threadAttachLocalStorage(&g_worker, NULL); /* Consumes worker stack. */
    threadStart(&g_worker);

    irqSet(IRQ_VBLANK, on_vblank);
    lcdSetIrqMask(DISPSTAT_IE_VBLANK, DISPSTAT_IE_VBLANK);
    irqEnable(IRQ_VBLANK);

    while (pmMainLoop()) {
        scanKeys();
        if ((keysDown() & KEY_START) != 0) break;
        threadWaitForVBlank(); /* Lets the lower-priority worker run. */
    }

    /* Stop the producer first. Runtime VBlank/tick interrupts remain active.
     * A blocking sleep (not threadYield) lets the lower-priority consumer
     * drain a pending hint before we enqueue the stop token.
     */
    irqClear(IRQ_VBLANK);
    while (!mailboxTrySend(&g_work, WORK_STOP)) threadSleep(1000);
    return threadJoin(&g_worker);
}
