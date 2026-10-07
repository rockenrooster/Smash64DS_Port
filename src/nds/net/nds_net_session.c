/* P3 net session and deterministic input lockstep (include/nds/nds_net_session.h).
 *
 * WIRE FORMAT (protocol version 1, nds_net_internal.h). Every payload starts
 * with an 8-byte header: 'S', '6', version, kind, then the session id
 * (little-endian u32). Fields are serialized byte by byte; no struct is ever
 * sent as-is. This file owns INPUT (and the lab's HELLO/START); the lobby's
 * messages are nds_net_lobby.c's.
 *
 *   INPUT      every batch: port, first tick, record count, the sender's
 *              receive horizon for every port, its newest batch digest, and
 *              up to NDS_NET_INPUT_MAX records from first tick on
 *
 * A record is one tick of one port's pad: buttons (u16, source button bits),
 * stick x, stick y. Records are produced once and never rewritten; a
 * different value for an accepted (port, tick) is counted as a conflict and
 * ignored.
 *
 * LOCKSTEP. Batch b is ticks 2b and 2b+1 on every console. At the gate of
 * batch b the console samples its pad once and records it for both ticks of
 * batch b + NDS_NET_DELAY_BATCHES (the first batches are neutral on every
 * console), broadcasts, then waits until it holds both ticks of b for every
 * human port. Waiting runs no gameplay: the last frame stays on screen and the
 * radio keeps being serviced. After the batch's second tick the replay digest
 * is folded and carried by the next INPUT packet; a different digest from a
 * peer for the same batch is a desync (gNdsNetDesyncBatch). */
#include <string.h>
#include <nds/bios.h>
#include <nds/interrupts.h>
#include <nds/input.h>
#include <nds/timers.h>

#include <PR/os.h>
#include <ssb_types.h>
#include <ft/fighter.h>

#include <nds/nds_controller.h>
#include <nds/nds_match_config.h>
#include <nds/nds_net.h>
#include <nds/nds_net_link.h>
#include <nds/nds_net_session.h>
#include <nds/nds_platform.h>

#include "nds_net_internal.h"

#define NDS_NET_RING_TICKS        256u      /* per port, power of two */
#define NDS_NET_INPUT_MAX         24u       /* records per INPUT packet */
#define NDS_NET_DIGEST_RING       64u       /* batches, power of two */
#define NDS_NET_STALL_ABORT_VBLANKS (60u * 10u)

extern void syUtilsSetRandomSeed(s32 seed);
extern u32 ndsReplayDigestTick(void);
extern void ndsControllerMapKeys(u32 keys, u16 *button, s8 *stick_x, s8 *stick_y);

volatile u32 gNdsNetSessionState;
volatile u32 gNdsNetLocalPort;
volatile u32 gNdsNetHumanMask;
volatile u32 gNdsNetBatch;
volatile u32 gNdsNetStallBatches;
volatile u32 gNdsNetStallVBlanks;
volatile u32 gNdsNetStallMaxVBlanks;
volatile u32 gNdsNetPacketsSent;
volatile u32 gNdsNetPacketsRecv;
volatile u32 gNdsNetPacketsBad;
volatile u32 gNdsNetRecordConflicts;
volatile u32 gNdsNetDigestCompares;
volatile u32 gNdsNetDesyncs;
volatile u32 gNdsNetDesyncBatch = 0xFFFFFFFFu;
volatile u32 gNdsNetAborted;

static u32 sNetSession;
static u32 sNetHostPort;
/* Records: one u32 per (port, tick): buttons | stick_x << 16 | stick_y << 24. */
static u32 sNetRecords[NDS_NET_PORTS][NDS_NET_RING_TICKS];
/* Next tick whose record this console does not hold, per port. */
static u32 sNetHave[NDS_NET_PORTS];
/* Per port: the receive horizon its owner last reported for our records. */
static u32 sNetPeerHaveMine[NDS_NET_PORTS];
static u32 sNetDigests[NDS_NET_DIGEST_RING];
static u32 sNetDigestValid[NDS_NET_DIGEST_RING];          /* batch + 1 */
static u32 sNetPeerDigests[NDS_NET_PORTS][NDS_NET_DIGEST_RING];
static u32 sNetPeerDigestValid[NDS_NET_PORTS][NDS_NET_DIGEST_RING];
static u32 sNetLastDigestBatch = 0xFFFFFFFFu;
static u32 sNetBatch;
static u32 sNetProducedBatch;

static u32 ndsNetHeader(u8 *buf, u32 kind)
{
    return ndsNetHeaderWrite(buf, kind, sNetSession);
}

uint32_t ndsNetSessionInMatch(void)
{
    return (gNdsNetSessionState == NDS_NET_SESSION_RUNNING) ? 1u : 0u;
}

/* --- Records ------------------------------------------------------------- */

static void ndsNetStoreRecord(u32 port, u32 tick, u32 value)
{
    u32 *slot;

    if (port >= NDS_NET_PORTS)
        return;
    if (tick < sNetHave[port])
    {
        /* Already held: identical is a redundant copy, different a fault. */
        if (tick + NDS_NET_RING_TICKS > sNetHave[port] &&
            sNetRecords[port][tick & (NDS_NET_RING_TICKS - 1u)] != value)
            gNdsNetRecordConflicts++;
        return;
    }
    if (tick != sNetHave[port])
        return; /* a hole before it: the sender resends from our horizon */
    slot = &sNetRecords[port][tick & (NDS_NET_RING_TICKS - 1u)];
    *slot = value;
    sNetHave[port] = tick + 1u;
}

/* --- INPUT packets ------------------------------------------------------- */

static void ndsNetSendInput(void)
{
    u8 buf[NDS_NET_MAX_PAYLOAD];
    const u32 me = gNdsNetLocalPort;
    u32 first = sNetHave[me];
    u32 count;
    u32 n;
    u32 p;

    /* Resend from the oldest horizon any human peer has reported. */
    for (p = 0u; p < NDS_NET_PORTS; p++)
    {
        if (p != me && ((gNdsNetHumanMask >> p) & 1u) != 0u &&
            sNetPeerHaveMine[p] < first)
            first = sNetPeerHaveMine[p];
    }
    if (sNetHave[me] - first > NDS_NET_RING_TICKS)
        first = sNetHave[me] - NDS_NET_RING_TICKS;
    count = sNetHave[me] - first;
    if (count > NDS_NET_INPUT_MAX)
        count = NDS_NET_INPUT_MAX;

    n = ndsNetHeader(buf, NDS_NET_KIND_INPUT);
    buf[n++] = (u8)me;
    buf[n++] = (u8)count;
    buf[n++] = 0u;
    buf[n++] = 0u;
    ndsNetPut32(buf + n, first);
    n += 4u;
    for (p = 0u; p < NDS_NET_PORTS; p++)
    {
        ndsNetPut32(buf + n, sNetHave[p]);
        n += 4u;
    }
    ndsNetPut32(buf + n, sNetLastDigestBatch);
    n += 4u;
    ndsNetPut32(buf + n, (sNetLastDigestBatch != 0xFFFFFFFFu) ?
                sNetDigests[sNetLastDigestBatch & (NDS_NET_DIGEST_RING - 1u)] : 0u);
    n += 4u;
    for (p = 0u; p < count; p++)
    {
        ndsNetPut32(buf + n, sNetRecords[me][(first + p) & (NDS_NET_RING_TICKS - 1u)]);
        n += 4u;
    }
    if (ndsNetLinkSendPacket(buf, n) == NDS_NET_OK)
        gNdsNetPacketsSent++;
}

static void ndsNetCompareDigest(u32 port, u32 batch)
{
    const u32 slot = batch & (NDS_NET_DIGEST_RING - 1u);

    if (sNetDigestValid[slot] != batch + 1u ||
        sNetPeerDigestValid[port][slot] != batch + 1u)
        return;
    gNdsNetDigestCompares++;
    if (sNetDigests[slot] != sNetPeerDigests[port][slot])
    {
        gNdsNetDesyncs++;
        if (gNdsNetDesyncBatch == 0xFFFFFFFFu)
            gNdsNetDesyncBatch = batch;
    }
    /* Compared once: retire the peer's copy. */
    sNetPeerDigestValid[port][slot] = 0u;
}

static void ndsNetHandleInput(const u8 *buf, u32 len)
{
    u32 port;
    u32 count;
    u32 first;
    u32 digest_batch;
    u32 digest;
    u32 i;
    const u8 *rec;

    if (len < NDS_NET_HDR_BYTES + 4u + 4u + 16u + 8u)
    {
        gNdsNetPacketsBad++;
        return;
    }
    port = buf[8];
    count = buf[9];
    first = ndsNetGet32(buf + 12);
    if (port >= NDS_NET_PORTS || port == gNdsNetLocalPort ||
        ((gNdsNetHumanMask >> port) & 1u) == 0u ||
        len < NDS_NET_HDR_BYTES + 4u + 4u + 16u + 8u + count * 4u)
    {
        gNdsNetPacketsBad++;
        return;
    }
    sNetPeerHaveMine[port] = ndsNetGet32(buf + 16 + gNdsNetLocalPort * 4u);
    digest_batch = ndsNetGet32(buf + 32);
    digest = ndsNetGet32(buf + 36);
    rec = buf + 40;
    for (i = 0u; i < count; i++)
        ndsNetStoreRecord(port, first + i, ndsNetGet32(rec + i * 4u));
    if (digest_batch != 0xFFFFFFFFu)
    {
        const u32 slot = digest_batch & (NDS_NET_DIGEST_RING - 1u);

        if (sNetPeerDigestValid[port][slot] != digest_batch + 1u)
        {
            sNetPeerDigests[port][slot] = digest;
            sNetPeerDigestValid[port][slot] = digest_batch + 1u;
            ndsNetCompareDigest(port, digest_batch);
        }
    }
}

static void ndsNetPump(void)
{
    u8 buf[NDS_NET_MAX_PAYLOAD + 2u];
    u8 src[6];
    u32 len;

    while ((len = ndsNetLinkRecvPacket(src, buf, sizeof(buf))) != 0u)
    {
        u32 kind;
        u32 session;

        if (ndsNetHeaderRead(buf, len, &kind, &session) == 0u ||
            session != sNetSession)
        {
            gNdsNetPacketsBad++;
            continue;
        }
        gNdsNetPacketsRecv++;
        if (kind == NDS_NET_KIND_INPUT)
            ndsNetHandleInput(buf, len);
    }
}

#if (NDS_NET_LAB_MATCH || NDS_NET_LAB_LOBBY) && NDS_NET_LAB_INPUT
/* Lab: a scripted pad per port, so an unattended two-console match exchanges
 * real, different input. A new action every six batches. */
static u32 ndsNetLocalKeys(u32 batch)
{
    static const u32 kActions[] = {
        0u, KEY_LEFT, KEY_RIGHT, KEY_A, KEY_B, KEY_X, KEY_LEFT | KEY_A,
        KEY_RIGHT | KEY_B, KEY_R, KEY_DOWN | KEY_A, KEY_UP | KEY_B, KEY_RIGHT,
        KEY_LEFT | KEY_X, KEY_DOWN, KEY_UP | KEY_A, KEY_RIGHT | KEY_A,
    };
    u32 h = ((batch / 6u) * 2654435761u) ^ (gNdsNetLocalPort * 0x9E3779B9u);

    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    return kActions[h & 15u];
}
#else
static u32 ndsNetLocalKeys(u32 batch)
{
    (void)batch;
    return ndsPlatformHeldKeys();
}
#endif

static u32 sNetSeed;
static u32 sNetSeedPending;

void ndsNetLockstepSetSeed(uint32_t seed)
{
    sNetSeed = seed;
    sNetSeedPending = 1u;
}

void ndsNetLockstepDisarm(void)
{
    if (gNdsNetSessionState != NDS_NET_SESSION_OFF)
    {
        gNdsNetSessionState = NDS_NET_SESSION_OFF;
        sNetSeedPending = 0u;
        ndsControllerPlaybackSetEnabled(FALSE);
    }
}

void ndsNetBattleSceneStart(void)
{
    if (gNdsNetSessionState == NDS_NET_SESSION_RUNNING && sNetSeedPending != 0u)
    {
        syUtilsSetRandomSeed((s32)sNetSeed);
        sNetSeedPending = 0u;
    }
}

void ndsNetLockstepConfigure(uint32_t session, uint32_t local_port,
                             uint32_t host_port, uint32_t human_mask)
{
    sNetSession = session;
    sNetHostPort = host_port;
    gNdsNetLocalPort = local_port;
    gNdsNetHumanMask = human_mask & ((1u << NDS_NET_PORTS) - 1u);
    gNdsNetDesyncBatch = 0xFFFFFFFFu;
    gNdsNetSessionState = NDS_NET_SESSION_RUNNING;
}

/* --- Battle loop seams ---------------------------------------------------- */

void ndsNetBattleBegin(void)
{
    u32 p;
    u32 t;

    if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
        return;
    memset(sNetRecords, 0, sizeof(sNetRecords));
    memset(sNetDigestValid, 0, sizeof(sNetDigestValid));
    memset(sNetPeerDigestValid, 0, sizeof(sNetPeerDigestValid));
    sNetLastDigestBatch = 0xFFFFFFFFu;
    sNetBatch = 0u;
    sNetProducedBatch = 0u;
    gNdsNetBatch = 0u;
    /* The first NDS_NET_DELAY_BATCHES batches are neutral on every console. */
    for (p = 0u; p < NDS_NET_PORTS; p++)
    {
        for (t = 0u; t < 2u * NDS_NET_DELAY_BATCHES; t++)
            sNetRecords[p][t] = 0u;
        sNetHave[p] = (((gNdsNetHumanMask >> p) & 1u) != 0u) ?
            2u * NDS_NET_DELAY_BATCHES : 0u;
        sNetPeerHaveMine[p] = 2u * NDS_NET_DELAY_BATCHES;
    }
    ndsControllerPlaybackReset();
    ndsControllerPlaybackSetEnabled(TRUE);
    ndsControllerPlaybackSetConnectedMask(gNdsNetHumanMask);
}

static u32 ndsNetBatchReady(u32 batch)
{
    const u32 need = 2u * batch + 2u;
    u32 p;

    for (p = 0u; p < NDS_NET_PORTS; p++)
    {
        if (((gNdsNetHumanMask >> p) & 1u) != 0u && sNetHave[p] < need)
            return 0u;
    }
    return 1u;
}

void ndsNetBattleGate(void)
{
    const u32 me = gNdsNetLocalPort;
    u32 waited = 0u;

    if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
        return;
    /* Produce this console's record for batch + delay, once. */
    if (sNetProducedBatch <= sNetBatch)
    {
        u16 button;
        s8 sx;
        s8 sy;
        u32 value;
        const u32 tick = 2u * (sNetBatch + NDS_NET_DELAY_BATCHES);

        (void)ndsPlatformReadInput();
        ndsControllerMapKeys(ndsNetLocalKeys(sNetBatch + NDS_NET_DELAY_BATCHES),
                             &button, &sx, &sy);
        value = (u32)button | ((u32)(u8)sx << 16) | ((u32)(u8)sy << 24);
        ndsNetStoreRecord(me, tick, value);
        ndsNetStoreRecord(me, tick + 1u, value);
        sNetProducedBatch = sNetBatch + 1u;
    }
    ndsNetPump();
    ndsNetSendInput();
    while (ndsNetBatchReady(sNetBatch) == 0u)
    {
        swiWaitForVBlank();
        waited++;
        ndsNetPump();
        if ((waited & 3u) == 0u)
            ndsNetSendInput();
        if (waited >= NDS_NET_STALL_ABORT_VBLANKS)
        {
            /* No progress for ten seconds: end the session rather than
             * invent input. The battle then runs on neutral pads. */
            gNdsNetAborted++;
            gNdsNetSessionState = NDS_NET_SESSION_ABORTED;
            ndsControllerPlaybackSetEnabled(FALSE);
            return;
        }
    }
    if (waited != 0u)
    {
        gNdsNetStallBatches++;
        gNdsNetStallVBlanks += waited;
        if (waited > gNdsNetStallMaxVBlanks)
            gNdsNetStallMaxVBlanks = waited;
    }
}

void ndsNetBattleInstallTick(uint32_t index)
{
    const u32 tick = 2u * sNetBatch + index;
    u32 p;

    if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
        return;
    for (p = 0u; p < NDS_NET_PORTS; p++)
    {
        u32 value;
        u16 button;

        if (((gNdsNetHumanMask >> p) & 1u) == 0u)
            continue;
        value = sNetRecords[p][tick & (NDS_NET_RING_TICKS - 1u)];
        button = (u16)value;
        /* Host-only pause (plan section 10): START from any other port is
         * dropped identically on every console. */
        if (p != sNetHostPort)
            button &= (u16)~START_BUTTON;
        ndsControllerPlaybackSetPad(p, button, (s8)(value >> 16), (s8)(value >> 24));
    }
}

void ndsNetBattleBatchDone(void)
{
    u32 slot;

    if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
        return;
    slot = sNetBatch & (NDS_NET_DIGEST_RING - 1u);
    sNetDigests[slot] = ndsReplayDigestTick();
    sNetDigestValid[slot] = sNetBatch + 1u;
    sNetLastDigestBatch = sNetBatch;
    {
        u32 p;

        for (p = 0u; p < NDS_NET_PORTS; p++)
        {
            if (p != gNdsNetLocalPort && ((gNdsNetHumanMask >> p) & 1u) != 0u)
                ndsNetCompareDigest(p, sNetBatch);
        }
    }
    sNetBatch++;
    gNdsNetBatch = sNetBatch;
}

void ndsNetBattleEnd(void)
{
    if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
        return;
    /* Tell the peers our final records and digest. The lockstep stays armed:
     * sudden death is another battle scene of the same match. The live keypad
     * comes back for Results; the lobby disarms after the match. */
    ndsNetSendInput();
    ndsControllerPlaybackSetEnabled(FALSE);
}

/* --- Lab: two consoles, fixed match ------------------------------------- */

#if NDS_NET_LAB_MATCH

volatile u32 gNdsNetLabRole;          /* 1 host, 2 guest */
volatile u32 gNdsNetLabHandshakeVBlanks;
volatile u32 gNdsNetLabSeed;

static void ndsNetLabDescriptor(NdsMatchConfig *cfg)
{
    ndsMatchConfigLoadMarioFoxDreamLand(cfg);
    cfg->fighters[0].fkind = nFTKindMario;
    cfg->fighters[0].pkind = nFTPlayerKindMan;
    cfg->fighters[1].fkind = nFTKindFox;
    cfg->fighters[1].pkind = nFTPlayerKindMan;
    cfg->fighters[2].fkind = nFTKindNull;
    cfg->fighters[2].pkind = nFTPlayerKindNot;
    cfg->fighters[3].fkind = nFTKindNull;
    cfg->fighters[3].pkind = nFTPlayerKindNot;
    cfg->game_rules = SCBATTLE_GAMERULE_TIME;
    cfg->time_limit = 2;
    cfg->item_toggles = 0u;
}

int ndsNetLabMatchHandshake(void)
{
    u8 mac[6];
    u8 peer[6];
    u8 buf[NDS_NET_MAX_PAYLOAD + 2u];
    u8 src[6];
    u32 have_peer = 0u;
    u32 is_host = 0u;
    u32 seed = 0u;
    u32 started = 0u;
    u32 acked = 0u;
    u32 vblanks;
    NdsMatchConfig cfg;

    if (ndsNetLinkStart(NDS_NET_DEFAULT_CHANNEL) != NDS_NET_OK)
        return -1;
    ndsNetLinkGetMac(mac);
    sNetSession = 0u;
    for (vblanks = 0u; vblanks < 60u * 60u; vblanks++)
    {
        u32 len;
        u32 n;

        /* Discovery: everyone says hello until the roles are settled. */
        if (!started)
        {
            n = ndsNetHeader(buf, NDS_NET_KIND_HELLO);
            memcpy(buf + n, mac, 6);
            n += 6u;
            buf[n++] = 0u;
            buf[n++] = 0u;
            ndsNetLinkSendPacket(buf, n);
        }
        if (have_peer && is_host)
        {
            /* START until the guest acknowledges, then a few more. */
            sNetSession = seed | 1u;
            n = ndsNetHeader(buf, NDS_NET_KIND_START);
            ndsNetPut32(buf + n, seed);
            n += 4u;
            buf[n++] = (u8)NDS_NET_DELAY_BATCHES;
            buf[n++] = 1u; /* the guest's port */
            buf[n++] = 0u;
            buf[n++] = 0u;
            ndsNetLinkSendPacket(buf, n);
            started = 1u;
            if (acked && acked++ > 30u)
                break;
        }
        while ((len = ndsNetLinkRecvPacket(src, buf, sizeof(buf))) != 0u)
        {
            u32 kind;
            u32 session;

            if (ndsNetHeaderRead(buf, len, &kind, &session) == 0u)
                continue;
            if (kind == NDS_NET_KIND_HELLO && !have_peer && len >= 14u)
            {
                memcpy(peer, buf + 8, 6);
                have_peer = 1u;
                is_host = (memcmp(mac, peer, 6) < 0) ? 1u : 0u;
                if (is_host)
                {
                    seed = (u32)cpuGetTiming() ^ ((u32)mac[5] << 24) ^ 0x5EEDu;
                }
            }
            else if (kind == NDS_NET_KIND_START && !is_host && len >= 16u)
            {
                seed = ndsNetGet32(buf + 8);
                sNetSession = session;
                started = 1u;
            }
            else if (kind == NDS_NET_KIND_START_ACK && is_host && session == sNetSession)
            {
                if (!acked)
                    acked = 1u;
            }
        }
        if (started && !is_host)
        {
            n = ndsNetHeader(buf, NDS_NET_KIND_START_ACK);
            buf[n++] = 1u;
            buf[n++] = 0u;
            ndsNetLinkSendPacket(buf, n);
            if (++acked > 45u)
                break;
        }
        swiWaitForVBlank();
    }
    gNdsNetLabHandshakeVBlanks = vblanks;
    if (!started)
        return -2;
    gNdsNetLabRole = is_host ? 1u : 2u;
    gNdsNetLabSeed = seed;
    ndsNetLabDescriptor(&cfg);
    gNdsMatchConfig = cfg;
    ndsMatchConfigApply(&gNdsMatchConfig);
    ndsNetLockstepConfigure(sNetSession, is_host ? 0u : 1u, 0u, 0x3u);
    ndsNetLockstepSetSeed(seed);
    return 0;
}

#endif
