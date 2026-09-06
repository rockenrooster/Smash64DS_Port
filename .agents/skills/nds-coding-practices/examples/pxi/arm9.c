/* ARM9 component: compile beside an existing compatible application main.
 * Exactly one owning ARM9 thread, one outstanding request, no calls from IRQs.
 * connect/echo/stop may block indefinitely if the matching peer is absent or
 * broken. Do not put these in a frame-critical path that requires a timeout.
 */
#include <nds.h>
#include "demo.h"
#include "protocol.h"

void nds_pxi_demo_connect(void)
{
    pxiWaitRemote(PxiChannel_User0);
}

bool nds_pxi_demo_echo(uint32_t value, uint32_t *reply)
{
    if (reply == NULL || value > NDS_PXI_DEMO_VALUE_MAX) return false;
    const u32 response = pxiSendAndReceive(PxiChannel_User0, value);
    if (response != value) return false;
    *reply = response;
    return true;
}

bool nds_pxi_demo_stop(void)
{
    return pxiSendAndReceive(PxiChannel_User0, NDS_PXI_DEMO_STOP)
           == NDS_PXI_DEMO_STOPPED;
}
