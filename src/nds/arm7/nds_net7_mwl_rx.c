// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
/* P3 ARM7 radio module: Calico 1.2.0's mwl RX tasks (source/dev/mwl/mwl_rx.c),
 * replaced.
 *
 * Calico's _mwlRxEndTask drains the MAC's receive buffer by allocating a
 * netbuf for every data and management frame and queueing it for a later task
 * on the same thread -- and when the pool runs dry it sleeps and retries
 * forever. Only those later tasks free buffers, so a drain that meets more
 * frames than the pool holds never returns: the radio thread deadlocks and the
 * link goes silent until the session gives up ("Connection lost", seen
 * 2026-10-07 on an original DS and a DS Lite, a minute and a half into a
 * match). On real hardware the channel also carries other stations'
 * management frames (probe requests, which this filter passes), so such
 * bursts are ordinary; melonDS carries none, which is why the two-console
 * harness never met it.
 *
 * The module needs none of the driver's management handling -- there is no
 * access point, so no scan, join, authentication, association or beacon
 * sync -- so this drain reads only data frames, into one static buffer, and
 * hands each to the module's ring at once (ndsNet7RxFrame); every other frame
 * is skipped in place, a stray deauthentication included. It never allocates
 * and never waits. Defining all four of mwl_rx.o's public functions keeps the
 * linker from pulling Calico's copy out of libcalico_ds7. */
#include "calico_mwl_common.h"
#include <calico/arm/common.h>

#include <nds/nds_net_link.h>

void ndsNet7RxFrame(const void *frame, u32 len);
void ndsNet7RxSkipped(void);

/* A whole frame of ours is 24 + NDS_NET_MAX_PAYLOAD bytes; anything longer is
 * not ours and is skipped unread. */
#define NDS_NET7_RX_FRAME_MAX (sizeof(WlanMacHdr) + NDS_NET_MAX_PAYLOAD)

static u16 sNet7RxFrame[(NDS_NET7_RX_FRAME_MAX + 1u) / 2u];

MK_NOINLINE static void ndsNet7MwlRxRead(void *dst, unsigned len)
{
    u16 *dst16 = (u16 *)dst;

    while (len > 1u)
    {
        *dst16++ = MWL_REG(W_RXBUF_RD_DATA);
        len -= 2u;
    }
}

void _mwlRxEndTask(void)
{
    for (;;)
    {
        /* Read cursor against the write cursor the RX_END IRQ latched. */
        const unsigned rdcsr = MWL_REG(W_RXBUF_READCSR);
        const unsigned wrcsr = s_mwlState.rx_wrcsr;
        MwlDataRxHdr rxhdr;
        unsigned type;
        unsigned read_sz;
        unsigned end;

        armCompilerBarrier();
        if (rdcsr == wrcsr)
            break;

        MWL_REG(W_RXBUF_RD_ADDR) = rdcsr * 2u;
        ndsNet7MwlRxRead(&rxhdr, sizeof(rxhdr));
        type = rxhdr.status & 0xFu;
        read_sz = (rxhdr.mpdu_len + 1u) & ~1u;
        /* The next frame starts after this one, word aligned, wrapping inside
         * the RX region exactly as Calico computes it. */
        end = (rdcsr * 2u + sizeof(MwlDataRxHdr) + rxhdr.mpdu_len + 3u) & ~3u;
        if (end >= MWL_MAC_RAM_SZ)
            end -= MWL_MAC_RAM_SZ - s_mwlState.rx_pos;

        if ((type == MwlRxType_IeeeData) && ((rxhdr.status & (1u << 9)) == 0u) &&
            (read_sz >= sizeof(WlanMacHdr)) && (read_sz <= sizeof(sNet7RxFrame)) &&
            (s_mwlState.status == MwlStatus_Class3))
        {
            ndsNet7MwlRxRead(sNet7RxFrame, read_sz);
            MWL_REG(W_RXBUF_READCSR) = end / 2u;
            ndsNet7RxFrame(sNet7RxFrame, read_sz);
        }
        else
        {
            MWL_REG(W_RXBUF_READCSR) = end / 2u;
            ndsNet7RxSkipped();
        }
    }
}

/* Nothing is ever queued for these. */
void _mwlRxMgmtCtrlTask(void)
{
}

void _mwlRxDataTask(void)
{
}

void _mwlRxQueueClear(void)
{
}
