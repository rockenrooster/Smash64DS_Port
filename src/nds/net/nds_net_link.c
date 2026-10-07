/* P3: the ARM9 side of the local wireless link (include/nds/nds_net_link.h).
 *
 * Loads the ARM7 radio module from NitroFS into the net area once per boot,
 * starts and stops the radio, and moves application packets through the two
 * rings in uncached high shared RAM. Nothing here runs unless a net session
 * calls it; offline play never touches the net area. */
#include <nds.h>
#include <stdio.h>
#include <string.h>

#include <nds/nds_net.h>
#include <nds/nds_net_link.h>

#define NDS_NET_LINK_MODULE_PATH "nitro:/net/net7.bin"

static u32 sNdsNetLinkAttached;

volatile u32 gNdsNetLinkLoadResult;

static inline void ndsNetLinkBarrier(void)
{
    __asm__ volatile("" ::: "memory");
}

static void ndsNetLinkSend(u32 cmd, u32 arg)
{
    pxiSend((PxiChannel)NDS_NET_PXI_CHANNEL, NDS_NET_CMD(cmd, arg));
}

/* Busy-waits on a shared word for at most `frames` VBlanks. */
static int ndsNetLinkWaitState(u32 want, u32 frames)
{
    while (frames-- != 0u)
    {
        const u32 state = gNdsNetShared->state;

        if (state == want)
            return NDS_NET_OK;
        if (state == NDS_NET_STATE_ERROR)
            return NDS_NET_ERR_RADIO;
        swiWaitForVBlank();
    }
    return NDS_NET_ERR_TIMEOUT;
}

int ndsNetLinkAttach(void)
{
    u8 *const base = (u8 *)NDS_NET_AREA_BASE;
    const NdsNet7Header *module = (const NdsNet7Header *)base;
    NdsNetShared *sh = gNdsNetShared;
    FILE *file;
    size_t bytes;
    int rc;

    if (sNdsNetLinkAttached != 0u)
        return NDS_NET_OK;
    file = fopen(NDS_NET_LINK_MODULE_PATH, "rb");
    if (file == NULL)
    {
        gNdsNetLinkLoadResult = 1u;
        return NDS_NET_ERR_MODULE;
    }
    bytes = fread(base, 1, NDS_NET_MODULE_MAX, file);
    fclose(file);
    if (bytes < sizeof(NdsNet7Header) || module->magic != NDS_NET7_MAGIC ||
        module->abi != NDS_NET_ABI ||
        module->image_end != NDS_NET_AREA_BASE + (u32)bytes ||
        module->bss_end < module->image_end ||
        module->bss_end > NDS_NET_AREA_BASE + NDS_NET_MODULE_MAX)
    {
        /* Leave nothing the ARM7 shim would accept. */
        memset(base, 0, sizeof(NdsNet7Header));
        gNdsNetLinkLoadResult = 2u;
        return NDS_NET_ERR_MODULE;
    }
    memset(base + bytes, 0, module->bss_end - module->image_end);
    memset(sh, 0, sizeof(*sh));
    sh->magic = NDS_NET_SHARED_MAGIC;
    sh->abi = NDS_NET_ABI;
    ndsNetLinkBarrier();
    ndsNetLinkSend(NDS_NET_CMD_ATTACH, 0u);
    rc = ndsNetLinkWaitState(NDS_NET_STATE_ATTACHED, 60u);
    gNdsNetLinkLoadResult = (rc == NDS_NET_OK) ? 0x600Du : (0x100u | (u32)-rc);
    if (rc == NDS_NET_OK)
        sNdsNetLinkAttached = 1u;
    return rc;
}

int ndsNetLinkStart(u32 channel)
{
    int rc = ndsNetLinkAttach();

    if (rc != NDS_NET_OK)
        return rc;
    if (gNdsNetShared->state == NDS_NET_STATE_RUNNING)
        return NDS_NET_OK;
    gNdsNetShared->error = 0u;
    ndsNetLinkSend(NDS_NET_CMD_START, channel);
    return ndsNetLinkWaitState(NDS_NET_STATE_RUNNING, 120u);
}

void ndsNetLinkStop(void)
{
    if (sNdsNetLinkAttached == 0u)
        return;
    if (gNdsNetShared->state == NDS_NET_STATE_RUNNING)
    {
        ndsNetLinkSend(NDS_NET_CMD_STOP, 0u);
        (void)ndsNetLinkWaitState(NDS_NET_STATE_ATTACHED, 60u);
    }
}

u32 ndsNetLinkRunning(void)
{
    return (sNdsNetLinkAttached != 0u &&
            gNdsNetShared->state == NDS_NET_STATE_RUNNING) ? 1u : 0u;
}

void ndsNetLinkGetMac(u8 mac[6])
{
    u32 i;

    for (i = 0u; i < 6u; i++)
        mac[i] = gNdsNetShared->mac[i];
}

int ndsNetLinkSendPacket(const void *data, u32 len)
{
    NdsNetShared *sh = gNdsNetShared;
    const u32 size = NDS_NET_TX_RING_SIZE;
    const u32 need = (NDS_NET_TX_HDR_BYTES + len + 3u) & ~3u;
    u32 head;
    u32 tail;
    u32 at;

    if (ndsNetLinkRunning() == 0u)
        return NDS_NET_ERR_DOWN;
    if (len == 0u || len > NDS_NET_MAX_PAYLOAD)
        return NDS_NET_ERR_SIZE;
    head = sh->tx_head;
    tail = sh->tx_tail;
    if (head >= tail)
    {
        if ((size - head) > need || ((size - head) == need && tail != 0u))
        {
            at = head;
        }
        else if (tail > need)
        {
            *(volatile u16 *)&sh->tx_ring[head] = NDS_NET_RING_WRAP;
            at = 0u;
        }
        else
        {
            return NDS_NET_ERR_FULL;
        }
    }
    else if ((tail - head) > need)
    {
        at = head;
    }
    else
    {
        return NDS_NET_ERR_FULL;
    }
    sh->tx_ring[at + 0u] = (u8)len;
    sh->tx_ring[at + 1u] = (u8)(len >> 8);
    sh->tx_ring[at + 2u] = 0u;
    sh->tx_ring[at + 3u] = 0u;
    memcpy(&sh->tx_ring[at + NDS_NET_TX_HDR_BYTES], data, len);
    ndsNetLinkBarrier();
    head = at + need;
    sh->tx_head = (head == size) ? 0u : head;
    ndsNetLinkSend(NDS_NET_CMD_KICK, 0u);
    return NDS_NET_OK;
}

u32 ndsNetLinkRecvPacket(u8 src_mac[6], void *buffer, u32 capacity)
{
    NdsNetShared *sh = gNdsNetShared;
    u32 tail;

    if (sNdsNetLinkAttached == 0u)
        return 0u;
    tail = sh->rx_tail;
    for (;;)
    {
        const u8 *rec;
        u32 len;

        if (tail == sh->rx_head)
            return 0u;
        rec = &sh->rx_ring[tail];
        len = (u32)rec[0] | ((u32)rec[1] << 8);
        if (len == NDS_NET_RING_WRAP)
        {
            tail = 0u;
            sh->rx_tail = tail;
            continue;
        }
        if (src_mac != NULL)
            memcpy(src_mac, rec + 4, 6);
        if (buffer != NULL)
            memcpy(buffer, rec + NDS_NET_RX_HDR_BYTES, (len < capacity) ? len : capacity);
        tail += (NDS_NET_RX_HDR_BYTES + len + 3u) & ~3u;
        if (tail >= NDS_NET_RX_RING_SIZE)
            tail = 0u;
        ndsNetLinkBarrier();
        sh->rx_tail = tail;
        return (len <= capacity) ? len : 0u;
    }
}

u32 ndsNetLinkStat(u32 index)
{
    return (index < NDS_NET_STAT_COUNT) ? gNdsNetShared->stats[index] : 0u;
}

/* The firmware user name, ASCII-folded, for room lists and lobby panels. */
void ndsNetLinkGetUserName(char out[11])
{
    u32 len = g_envUserSettings->user.name_len;
    u32 i;

    if (len > 10u)
        len = 10u;
    for (i = 0u; i < len; i++)
    {
        const u16 c = g_envUserSettings->user.name_ucs2[i];

        out[i] = (c >= 0x20u && c < 0x7Fu) ? (char)c : '?';
    }
    if (len == 0u)
    {
        memcpy(out, "PLAYER", 6);
        len = 6u;
    }
    for (i = len; i <= 10u; i++)
        out[i] = '\0';
}
