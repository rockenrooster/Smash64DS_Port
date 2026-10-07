/* P3: the resident half of the ARM7 radio (include/nds/nds_net_link.h). It
 * only forwards PXI words to the radio module, which the ARM9 loads into the
 * net area when a session starts; until then the area holds the loader's
 * return stub, whose first word is not the module magic, and every word is
 * ignored. Kept in main RAM: ARM7 WRAM has almost nothing free. */
#include <calico.h>
#include <nds/nds_net_link.h>

#define NDS_NET_ARM7_TEXT __attribute__((section(".main.nds_net_arm7")))

NDS_NET_ARM7_TEXT
static void ndsNetArm7PxiHandler(void *user, u32 data)
{
    const NdsNet7Header *module = (const NdsNet7Header *)NDS_NET_AREA_BASE;

    (void)user;
    if (module->magic != NDS_NET7_MAGIC || module->abi != NDS_NET_ABI)
        return;
    if ((data & 0xFu) == NDS_NET_CMD_ATTACH)
        module->attach();
    else
        module->post(data);
}

NDS_NET_ARM7_TEXT
void ndsNetArm7Init(void)
{
    pxiSetHandler((PxiChannel)NDS_NET_PXI_CHANNEL, ndsNetArm7PxiHandler, NULL);
}
