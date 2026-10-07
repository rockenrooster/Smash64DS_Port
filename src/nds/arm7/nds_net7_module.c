/* P3 ARM7 radio module (include/nds/nds_net_link.h has the design).
 *
 * Linked on its own at NDS_NET_AREA_BASE against the resident ARM7 image's
 * symbols (linker/nds_arm7_net7.ld, --just-symbols), shipped in NitroFS as
 * net/net7.bin and copied into place by the ARM9 when a net session starts.
 * It pulls Calico's Mitsumi driver (mwl), netbuf and wlan objects from
 * libcalico_ds7; everything else it calls (threads, mailboxes, IRQs, PXI,
 * power management, NVRAM) is the resident image's copy.
 *
 * One thread owns the radio. The resident PXI handler forwards every ARM9 word
 * to it through post(); START brings the driver up, STOP shuts it down, KICK
 * drains the TX ring. Received frames are read on the driver's own thread by
 * this module's RX drain (nds_net7_mwl_rx.c, which replaces Calico's) and are
 * copied into the RX ring. */
#include <calico.h>
#include <calico/dev/mwl.h>
#include <calico/dev/netbuf.h>
#include <calico/dev/wlan.h>
#include <string.h>

#include <nds/nds_net_link.h>

/* Calico 1.2.0 internals (source/nds/netbuf.c, source/dev/mwl/common.h). The
 * driver takes data frames only while the station is associated
 * (MwlState.status == Class3); there is no access point here, so the module
 * sets that state itself once the driver is started. */
#include "calico_mwl_common.h"
extern void _netbufPrvInitPools(void *start, const u16 *tx_counts, const u16 *rx_counts);

void ndsNet7Attach(void);
void ndsNet7Post(u32 word);

/* linker/nds_arm7_net7.ld */
extern u8 __net7_image_end[];
extern u8 __net7_bss_end[];

__attribute__((section(".net7.header"), used))
const NdsNet7Header gNdsNet7Header = {
    NDS_NET7_MAGIC,
    NDS_NET_ABI,
    (uintptr_t)__net7_image_end,
    (uintptr_t)__net7_bss_end,
    ndsNet7Attach,
    ndsNet7Post,
};

#define NDS_NET7_MAIL_SLOTS 16u
static Thread sNet7Thread;
static Mailbox sNet7Mailbox;
static u32 sNet7MailSlots[NDS_NET7_MAIL_SLOTS];
alignas(8) static u8 sNet7Stack[1024];
static u8 sNet7Mac[6];
static bool sNet7RadioOn;
static bool sNet7TxBlocked;
static const u8 kNdsNet7Bssid[6] = NDS_NET_BSSID_INIT;

/* Packet buffers: the driver's subpools are 128/256/512/1024/2048 bytes.
 * Frames are at most 24 + NDS_NET_MAX_PAYLOAD = 256 bytes. Only TX uses the
 * pool: the RX drain reads into its own static buffer and never allocates. */
static const u16 kNdsNet7TxCounts[5] = { 0, 6, 0, 0, 0 };
static const u16 kNdsNet7RxCounts[5] = { 0, 0, 0, 0, 0 };
#define NDS_NET7_POOL_BYTES (6u * (sizeof(NetBuf) + 256u))
alignas(4) static u8 sNet7PoolMem[NDS_NET7_POOL_BYTES];

static inline void ndsNet7Barrier(void)
{
    __asm__ volatile("" ::: "memory");
}

static inline void ndsNet7Stat(u32 index)
{
    gNdsNetShared->stats[index]++;
}

/* --- RX: driver thread -> ring -> ARM9 ---------------------------------- */

static bool ndsNet7RxPush(const u8 *src, const u8 *data, u32 len)
{
    NdsNetShared *sh = gNdsNetShared;
    const u32 size = NDS_NET_RX_RING_SIZE;
    const u32 need = (NDS_NET_RX_HDR_BYTES + len + 3u) & ~3u;
    u32 head = sh->rx_head;
    const u32 tail = sh->rx_tail;
    u32 at = head;

    if (head >= tail)
    {
        if ((size - head) > need || ((size - head) == need && tail != 0u))
        {
            at = head;
        }
        else if (tail > need)
        {
            *(volatile u16 *)&sh->rx_ring[head] = NDS_NET_RING_WRAP;
            at = 0u;
        }
        else
        {
            return false;
        }
    }
    else if ((tail - head) <= need)
    {
        return false;
    }
    {
        u8 *rec = &sh->rx_ring[at];
        rec[0] = (u8)len;
        rec[1] = (u8)(len >> 8);
        rec[2] = 0u;
        rec[3] = 0u;
        memcpy(rec + 4, src, 6);
        rec[10] = 0u;
        rec[11] = 0u;
        memcpy(rec + NDS_NET_RX_HDR_BYTES, data, len);
    }
    ndsNet7Barrier();
    head = at + need;
    sh->rx_head = (head == size) ? 0u : head;
    return true;
}

/* One received data frame (802.11 header + payload), from the RX drain on the
 * driver's thread: a game frame from another console goes into the ring. */
void ndsNet7RxFrame(const void *frame, u32 len)
{
    const WlanMacHdr *hdr = (const WlanMacHdr *)frame;

    if (len > sizeof(WlanMacHdr) && len <= sizeof(WlanMacHdr) + NDS_NET_MAX_PAYLOAD &&
        hdr->fc.type == WlanFrameType_Data && hdr->fc.from_ds && !hdr->fc.to_ds &&
        memcmp(hdr->tx_addr, kNdsNet7Bssid, 6) == 0 &&
        memcmp(hdr->xtra_addr, sNet7Mac, 6) != 0)
    {
        if (ndsNet7RxPush(hdr->xtra_addr, (const u8 *)(hdr + 1), len - sizeof(WlanMacHdr)))
            ndsNet7Stat(NDS_NET_STAT_RX_FRAMES);
        else
            ndsNet7Stat(NDS_NET_STAT_RX_RING_FULL);
    }
    else
    {
        ndsNet7Stat(NDS_NET_STAT_RX_FOREIGN);
    }
}

/* Any other frame the MAC passed (another station's management traffic, a
 * beacon, a control frame): skipped unread. */
void ndsNet7RxSkipped(void)
{
    ndsNet7Stat(NDS_NET_STAT_RX_FOREIGN);
}

/* --- TX: ARM9 ring -> driver ------------------------------------------- */

static void ndsNet7TxDone(void *arg, MwlTxEvent evt, MwlDataTxHdr *hdr)
{
    (void)arg;
    (void)hdr;
    switch (evt)
    {
    case MwlTxEvent_Done:
        ndsNet7Stat(NDS_NET_STAT_TX_SENT);
        break;
    case MwlTxEvent_Error:
        ndsNet7Stat(NDS_NET_STAT_TX_ERRORS);
        break;
    case MwlTxEvent_Dropped:
        ndsNet7Stat(NDS_NET_STAT_TX_DROPPED);
        break;
    default:
        return;
    }
    /* A buffer came back: resume a drain that ran out of them. */
    if (sNet7TxBlocked)
    {
        sNet7TxBlocked = false;
        mailboxTrySend(&sNet7Mailbox, NDS_NET_CMD(NDS_NET_CMD_KICK, 0));
    }
}

static void ndsNet7TxDrain(void)
{
    NdsNetShared *sh = gNdsNetShared;
    u32 tail = sh->tx_tail;

    while (sNet7RadioOn && tail != sh->tx_head)
    {
        const u8 *rec = &sh->tx_ring[tail];
        const u32 len = (u32)rec[0] | ((u32)rec[1] << 8);
        NetBuf *nb;
        WlanMacHdr *hdr;

        if (len == NDS_NET_RING_WRAP)
        {
            tail = 0u;
            sh->tx_tail = tail;
            continue;
        }
        if (len == 0u || len > NDS_NET_MAX_PAYLOAD)
        {
            /* Corrupt ring: drop everything queued. */
            ndsNet7Stat(NDS_NET_STAT_TX_DROPPED);
            sh->tx_tail = sh->tx_head;
            return;
        }
        nb = netbufAlloc(sizeof(WlanMacHdr), len, NetBufPool_Tx);
        if (nb == NULL)
        {
            ndsNet7Stat(NDS_NET_STAT_TX_NO_BUFFER);
            sNet7TxBlocked = true;
            return;
        }
        memcpy(netbufGet(nb), rec + NDS_NET_TX_HDR_BYTES, len);
        hdr = netbufPushHeaderType(nb, WlanMacHdr);
        memset(hdr, 0, sizeof(*hdr));
        hdr->fc.type = WlanFrameType_Data;
        hdr->fc.from_ds = 1;
        memset(hdr->rx_addr, 0xFF, 6);
        memcpy(hdr->tx_addr, kNdsNet7Bssid, 6);
        memcpy(hdr->xtra_addr, sNet7Mac, 6);
        mwlDevTx(0, nb, ndsNet7TxDone, NULL);

        tail += (NDS_NET_TX_HDR_BYTES + len + 3u) & ~3u;
        if (tail >= NDS_NET_TX_RING_SIZE)
            tail = 0u;
        ndsNet7Barrier();
        sh->tx_tail = tail;
    }
}

/* --- Radio lifecycle ----------------------------------------------------- */

static void ndsNet7RadioStart(u32 channel)
{
    NdsNetShared *sh = gNdsNetShared;
    MwlMlmeCallbacks *cb;
    ArmIrqState st;

    if (sNet7RadioOn)
        return;
    if (!mwlCalibLoad())
    {
        sh->error = NDS_NET_ERR_CALIB;
        sh->state = NDS_NET_STATE_ERROR;
        return;
    }
    if (channel < 1u || channel > 13u ||
        !(mwlGetCalibData()->enabled_ch_mask & (1u << channel)))
    {
        sh->error = NDS_NET_ERR_CHANNEL;
        sh->state = NDS_NET_STATE_ERROR;
        return;
    }
    memcpy(sNet7Mac, mwlGetCalibData()->mac_addr, 6);
    memcpy((void *)sh->mac, sNet7Mac, 6);

    /* Not pmSetSleepAllowed: the resident image links no copy of it, and the
     * archive's would bring a second pm.o with its own state. The ARM9 owns
     * the lid policy for a session. */
    pmSetPowerLed(PmLedMode_BlinkFast);
    pmPowerOn(POWCNT_WL_MITSUMI);
    REG_EXMEMCNT2 = 0x30;

    mwlDevWakeUp();
    mwlDevReset();
    mwlDevSetMode(MwlMode_Infra);
    mwlDevSetChannel(channel);
    mwlDevSetBssid(kNdsNet7Bssid);
    cb = mwlMlmeGetCallbacks();
    memset(cb, 0, sizeof(*cb));
    mwlDevStart();

    st = armIrqLockByPsr();
    s_mwlState.status = MwlStatus_Class3;
    armIrqUnlockByPsr(st);

    sNet7RadioOn = true;
    sNet7TxBlocked = false;
    sh->channel = channel;
    sh->error = NDS_NET_ERR_NONE;
    sh->state = NDS_NET_STATE_RUNNING;
    ndsNet7TxDrain();
}

static void ndsNet7RadioStop(void)
{
    NdsNetShared *sh = gNdsNetShared;

    if (!sNet7RadioOn)
        return;
    sNet7RadioOn = false;
    mwlDevStop();
    mwlDevShutdown();
    pmPowerOff(POWCNT_WL_MITSUMI);
    pmSetPowerLed(PmLedMode_Steady);
    sh->tx_tail = sh->tx_head;
    sh->state = NDS_NET_STATE_ATTACHED;
}

static int ndsNet7Thread(void *arg)
{
    (void)arg;
    _netbufPrvInitPools(sNet7PoolMem, kNdsNet7TxCounts, kNdsNet7RxCounts);
    gNdsNetShared->state = NDS_NET_STATE_ATTACHED;
    for (;;)
    {
        const u32 word = mailboxRecv(&sNet7Mailbox);
        const u32 arg_bits = word >> 4;

        switch (word & 0xFu)
        {
        case NDS_NET_CMD_START:
            ndsNet7RadioStart(arg_bits);
            break;
        case NDS_NET_CMD_STOP:
            ndsNet7RadioStop();
            break;
        case NDS_NET_CMD_KICK:
            ndsNet7Stat(NDS_NET_STAT_KICKS);
            ndsNet7TxDrain();
            break;
        default:
            break;
        }
    }
    return 0;
}

/* --- Entry points (resident PXI handler, IRQ context) -------------------- */

void ndsNet7Attach(void)
{
    static bool attached;

    if (attached)
        return;
    attached = true;
    mailboxPrepare(&sNet7Mailbox, sNet7MailSlots, NDS_NET7_MAIL_SLOTS);
    threadPrepare(&sNet7Thread, ndsNet7Thread, NULL,
                  &sNet7Stack[sizeof(sNet7Stack)], 0x12);
    threadStart(&sNet7Thread);
}

void ndsNet7Post(u32 word)
{
    mailboxTrySend(&sNet7Mailbox, word);
}
