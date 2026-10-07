/* P3 lab: a boot-time radio exchange for the two-instance harness
 * (scratchpad mp/, melonDS-mp with MELONDS_MP_INSTANCES=2). Each console
 * broadcasts one numbered packet a VBlank and counts the peers' packets; gdb
 * reads the counters below. Compiled only with NDS_NET_LAB_PING. */
#include <nds.h>
#include <string.h>

#include <nds/nds_net.h>
#include <nds/nds_net_link.h>

#if NDS_NET_LAB_PING

volatile u32 gNdsNetLabPingStart = 0xFFFFFFFFu;
volatile u32 gNdsNetLabPingSent;
volatile u32 gNdsNetLabPingSendFail;
volatile u32 gNdsNetLabPingRecv;
volatile u32 gNdsNetLabPingBad;
volatile u32 gNdsNetLabPingLastSeq;
volatile u32 gNdsNetLabPingGaps;
volatile u32 gNdsNetLabPingFrames;
volatile u32 gNdsNetLabPingFirstRecvFrame = 0xFFFFFFFFu;
volatile u32 gNdsNetLabPingDone;
volatile u8 gNdsNetLabPingMyMac[8];
volatile u8 gNdsNetLabPingPeerMac[8];

void ndsNetLabPingRun(u32 frames)
{
    u8 packet[32];
    u8 rx[64];
    u8 src[6];
    u32 seq = 0u;
    u32 have_last = 0u;
    u32 f;
    int rc = ndsNetLinkStart(NDS_NET_DEFAULT_CHANNEL);

    gNdsNetLabPingStart = (u32)rc;
    if (rc != NDS_NET_OK)
    {
        gNdsNetLabPingDone = 1u;
        return;
    }
    {
        u8 mac[6];
        u32 i;

        ndsNetLinkGetMac(mac);
        for (i = 0u; i < 6u; i++)
            gNdsNetLabPingMyMac[i] = mac[i];
    }
    for (f = 0u; f < frames; f++)
    {
        u32 len;

        memset(packet, 0, sizeof(packet));
        memcpy(packet, "PING", 4);
        packet[4] = (u8)seq;
        packet[5] = (u8)(seq >> 8);
        packet[6] = (u8)(seq >> 16);
        packet[7] = (u8)(seq >> 24);
        if (ndsNetLinkSendPacket(packet, sizeof(packet)) == NDS_NET_OK)
            gNdsNetLabPingSent++;
        else
            gNdsNetLabPingSendFail++;
        seq++;

        while ((len = ndsNetLinkRecvPacket(src, rx, sizeof(rx))) != 0u)
        {
            u32 got;
            u32 i;

            if (len < 8u || memcmp(rx, "PING", 4) != 0)
            {
                gNdsNetLabPingBad++;
                continue;
            }
            got = (u32)rx[4] | ((u32)rx[5] << 8) | ((u32)rx[6] << 16) | ((u32)rx[7] << 24);
            if (have_last != 0u && got != gNdsNetLabPingLastSeq + 1u)
                gNdsNetLabPingGaps++;
            have_last = 1u;
            gNdsNetLabPingLastSeq = got;
            if (gNdsNetLabPingRecv == 0u)
                gNdsNetLabPingFirstRecvFrame = f;
            gNdsNetLabPingRecv++;
            for (i = 0u; i < 6u; i++)
                gNdsNetLabPingPeerMac[i] = src[i];
        }
        gNdsNetLabPingFrames = f + 1u;
        swiWaitForVBlank();
    }
    gNdsNetLabPingDone = 1u;
    ndsNetLinkStop();
}

#endif
