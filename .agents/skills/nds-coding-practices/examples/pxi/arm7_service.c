/* ARM7 component: preserve the current core's startup and stock services.
 * No libc/TLS use in this worker. Static storage survives until the join.
 * Protocol: one owning ARM9 requester, only simple messages, at most one
 * outstanding request. This prevents overflowing the four-slot mailbox.
 */
#include <nds.h>
#include "demo.h"
#include "protocol.h"

static Mailbox g_requests;
static u32 g_slots[4];
static Thread g_server;
static uint8_t g_stack[1024] __attribute__((aligned(8)));
static bool g_started;

static int server_main(void *argument)
{
    (void)argument;
    pxiSetMailbox(PxiChannel_User0, &g_requests); /* Publishes readiness. */
    for (;;) {
        const u32 request = mailboxRecv(&g_requests);
        pxiReply(PxiChannel_User0, nds_pxi_demo_answer(request));
        if (request == NDS_PXI_DEMO_STOP) {
            pxiSetHandler(PxiChannel_User0, NULL, NULL); /* Unregister route. */
            return 0;
        }
    }
}

bool nds_pxi_demo_server_start(void)
{
    /* Initialization has one main-thread owner; this is deliberately start-once.
     * A restartable service needs its own re-registration/session contract.
     */
    if (g_started) return false;
    g_started = true;
    mailboxPrepare(&g_requests, g_slots, sizeof g_slots / sizeof g_slots[0]);
    threadPrepare(&g_server, server_main, NULL,
                  g_stack + sizeof g_stack, MAIN_THREAD_PRIO + 1);
    threadStart(&g_server);
    return true;
}

int nds_pxi_demo_server_join(void)
{
    if (!g_started) return -1;
    return threadJoin(&g_server);
}
