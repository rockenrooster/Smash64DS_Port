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
 * radio keeps being serviced. After the batch's second tick the net digest
 * (the replay digest plus the battle state's results fields, ndsNetDigest) is
 * folded and carried by the next INPUT packet; a different digest from a peer
 * for the same batch is a desync (gNdsNetDesyncBatch). When the battle ends,
 * every console compares its terminal tick's digest with every peer's before
 * Results publish a winner (ndsNetBattleEnd).
 *
 * LEAVING. A guest leaves a running match by holding START (START is the
 * host's alone otherwise, plan section 10); its LEAVE, like the host closing
 * the room, ends the match for everyone as NO CONTEST. */
#include <string.h>
#include <nds/bios.h>
#include <nds/interrupts.h>
#include <nds/input.h>
#include <nds/timers.h>

#include <PR/os.h>
#include <ssb_types.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <it/item.h>
#include <sc/scene.h>
#include <sys/malloc.h>
#include <wp/weapon.h>

#include <nds/nds_controller.h>
#include <nds/nds_match_config.h>
#include <nds/nds_net.h>
#include <nds/nds_net_link.h>
#include <nds/nds_net_lobby.h>
#include <nds/nds_net_session.h>
#include <nds/nds_platform.h>
#include <nds/nds_net_ui.h>

#include "nds_net_internal.h"

#define NDS_NET_RING_TICKS        256u      /* per port, power of two */
#define NDS_NET_INPUT_MAX         24u       /* records per INPUT packet */
#define NDS_NET_DIGEST_RING       64u       /* batches, power of two */
#define NDS_NET_STALL_ABORT_VBLANKS (60u * 10u)
/* The first batch also absorbs the consoles' different battle load times. */
#define NDS_NET_STALL_ABORT_FIRST   (60u * 30u)
#define NDS_NET_STALL_NOTICE        20u
/* A guest leaves a running match by holding START this long. */
#define NDS_NET_LEAVE_HOLD_VBLANKS  (60u * 2u)
/* Battle end: how long to wait for the peers' terminal digests, and how long
 * to keep sending ours once every peer's has been compared. */
#define NDS_NET_TERMINAL_WAIT       (60u * 3u)
#define NDS_NET_TERMINAL_LINGER     12u

extern void syTaskmanSetLoadScene(void);
extern SYMallocRegion gSYTaskmanGeneralHeap;

extern void syUtilsSetRandomSeed(s32 seed);
extern u32 ndsReplayDigestTick(void);
extern void ndsControllerMapKeys(u32 keys, u16 *button, s8 *stick_x, s8 *stick_y);
extern sb32 ftCommonSleepCheckIgnorePauseMenu(GObj *fighter_gobj);

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
/* Per port: the last batch whose digest was compared with ours, + 1. */
static u32 sNetCompared[NDS_NET_PORTS];
static u32 sNetBatch;
static u32 sNetProducedBatch;
/* VBlank count + 1 since a guest started holding START, 0 when released. */
static u32 sNetLeaveHeldSince;
#if NDS_NET_LAB_SOAK
extern u32 ndsNetLabAutopilotSoakLeave(void);
#endif
#if NDS_NET_LAB_LEAVE
static u32 sNetLabLeaveDone;
static u32 sNetLabBattles;     /* battle scenes this console has entered */
#endif

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
    sNetCompared[port] = batch + 1u;
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

static void ndsNetLockstepAbort(void);

/* A LEAVE during a match -- a guest holding START, or the host closing the
 * room -- ends it for everyone: the leaver's records stop, and the source
 * cannot play on without them. It ends as NO CONTEST like any abort, and the
 * lobby drops the player. */
static void ndsNetHandleLeave(const u8 *src, u32 port)
{
    const u32 who = (port == 0xFFu) ? sNetHostPort : port;
    char name[NDS_NET_NAME_LEN + 1u];

    if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING || who >= NDS_NET_PORTS ||
        who == gNdsNetLocalPort || ((gNdsNetHumanMask >> who) & 1u) == 0u)
        return;
    strncpy(name, ndsNetLobbyMemberName(who), NDS_NET_NAME_LEN);
    name[NDS_NET_NAME_LEN] = '\0';
    if (ndsNetLobbyMemberLeft(src, port) == 0u)
        return;
    if (name[0] != '\0')
        ndsNetUiLine(21, " %s left the match.", name);
    else
        ndsNetUiLine(21, " Player %lu left the match.", (unsigned long)(who + 1u));
    ndsNetLockstepAbort();
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
        else if (kind == NDS_NET_KIND_LEAVE && len > NDS_NET_HDR_BYTES)
            ndsNetHandleLeave(src, buf[NDS_NET_HDR_BYTES]);
    }
}

#if (NDS_NET_LAB_MATCH || NDS_NET_LAB_LOBBY) && NDS_NET_LAB_INPUT
/* Lab: a scripted pad per port, so an unattended two-console match exchanges
 * real, different input. A new action every six batches. */
static u32 ndsNetLocalKeys(u32 batch)
{
    /* Every source control the pad maps: moves, jumps, attacks, specials in
     * all four directions, shield (R), grab and shield (L = Z), and the
     * taunt (SELECT = L), so grabs, throws and Kirby's inhale/copy run too. */
    static const u32 kActions[] = {
        0u, KEY_LEFT, KEY_RIGHT, KEY_A, KEY_B, KEY_X, KEY_LEFT | KEY_A,
        KEY_RIGHT | KEY_B, KEY_R, KEY_DOWN | KEY_A, KEY_UP | KEY_B, KEY_RIGHT,
        KEY_LEFT | KEY_X, KEY_DOWN, KEY_UP | KEY_A, KEY_RIGHT | KEY_A,
        KEY_L, KEY_L | KEY_RIGHT, KEY_R | KEY_A, KEY_DOWN | KEY_B,
        KEY_SELECT, KEY_LEFT | KEY_B, KEY_B, KEY_RIGHT | KEY_L,
        KEY_L | KEY_LEFT, KEY_X | KEY_A, KEY_DOWN | KEY_R, KEY_L | KEY_DOWN,
        KEY_B, KEY_RIGHT | KEY_X, KEY_L | KEY_UP, KEY_A,
    };
    u32 h = ((batch / 6u) * 2654435761u) ^ (gNdsNetLocalPort * 0x9E3779B9u);
    u32 keys;

    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    keys = kActions[h & 31u];
#if NDS_NET_LAB_TEAMS
    /* Guests press START every two seconds: it reaches the game only as a
     * team Stock steal (ndsNetStartIsStockSteal), never as a pause. */
    if ((gNdsNetLocalPort != 0u) && ((batch % 60u) < 2u))
        keys |= KEY_START;
#endif
    return keys;
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

#if NDS_NET_LAB_GATE_RADIO
/* Lab (plan section 12, P3-0C): the four-fighter gate with the radio running.
 * Set to 1 at boot (the sampler's -BootSetGlobals), the battle opens a room
 * and runs the lockstep with this console as its only human, so every batch
 * pumps the radio, sends its INPUT packet and folds the net digest as a net
 * match does; left 0, the same ROM measures without the radio. */
volatile u32 gNdsNetLabGateRadio;
#endif

void ndsNetBattleBegin(void)
{
    u32 p;
    u32 t;

#if NDS_NET_LAB_GATE_RADIO
    if ((gNdsNetLabGateRadio != 0u) &&
        (gNdsNetSessionState != NDS_NET_SESSION_RUNNING) &&
        (ndsNetHostOpen() == 0))
        ndsNetLockstepConfigure(1u, 0u, 0u, 1u);
#endif
    if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
        return;
    memset(sNetRecords, 0, sizeof(sNetRecords));
    memset(sNetDigestValid, 0, sizeof(sNetDigestValid));
    memset(sNetPeerDigestValid, 0, sizeof(sNetPeerDigestValid));
    memset(sNetCompared, 0, sizeof(sNetCompared));
    sNetLeaveHeldSince = 0u;
#if NDS_NET_LAB_LEAVE
    sNetLabBattles++;
#endif
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

/* Ends the match without inventing a result. As the source's pause quit
 * (ifcommon.c, A+B+R+Z) does, it ends as a reset, which Results shows as NO
 * CONTEST; the scene exit is requested directly so it works in any phase (the
 * countdown ignores the interface's Set status). */
static void ndsNetLockstepAbort(void)
{
    gNdsNetAborted++;
    gNdsNetSessionState = NDS_NET_SESSION_ABORTED;
    ndsControllerPlaybackSetEnabled(FALSE);
    gSCManagerSceneData.is_reset = TRUE;
    syTaskmanSetLoadScene();
}

/* The keys a guest's Leave hold reads: the live pad. */
static u32 ndsNetLeaveKeys(void)
{
    u32 keys = ndsPlatformHeldKeys();

#if NDS_NET_LAB_LEAVE
    /* Lab: the guest holds START from batch 600 of its Nth battle. */
    if (sNetLabLeaveDone == 0u && sNetLabBattles >= (u32)NDS_NET_LAB_LEAVE &&
        sNetBatch >= 600u)
        keys |= KEY_START;
#endif
#if NDS_NET_LAB_SOAK
    /* Lab soak: the guest of every fourth room leaves its first match. */
    if (ndsNetLabAutopilotSoakLeave() != 0u && sNetBatch >= 600u)
        keys |= KEY_START;
#endif
    return keys;
}

/* Plan section 10: START pauses for the host alone, so a guest's held START
 * is its Leave command; its first press says so on the lower screen. */
static u32 ndsNetGuestHoldsLeave(void)
{
    const u32 now = ndsPlatformVBlankCount() + 1u;

    if (gNdsNetLocalPort == sNetHostPort)
        return 0u;
    if ((ndsNetLeaveKeys() & KEY_START) == 0u)
    {
        if (sNetLeaveHeldSince != 0u)
        {
            sNetLeaveHeldSince = 0u;
            ndsNetUiLine(22, "");
        }
        return 0u;
    }
    if (sNetLeaveHeldSince == 0u)
    {
        sNetLeaveHeldSince = now;
        ndsNetUiLine(22, " Hold START to leave the match.");
    }
    return ((now - sNetLeaveHeldSince) >= NDS_NET_LEAVE_HOLD_VBLANKS) ? 1u : 0u;
}

static void ndsNetLeaveMatch(void)
{
    sNetLeaveHeldSince = 0u;
#if NDS_NET_LAB_LEAVE
    sNetLabLeaveDone = 1u;
#endif
    ndsNetUiLine(22, "");
    ndsNetUiLine(21, " You left the match.");
    ndsNetLockstepAbort();
    /* LEAVE to the room, then the radio stops: this console is out of it. */
    ndsNetSessionClose();
}

void ndsNetBattleGate(void)
{
    const u32 me = gNdsNetLocalPort;
    u32 waited = 0u;

    if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
        return;
    if (gNdsNetDesyncBatch != 0xFFFFFFFFu)
    {
        /* Plan 7.3: a divergence ends the match, naming the first tick whose
         * digests differ; state is never copied to repair it. */
        if (gNdsNetDesyncBatch == 0u)
            ndsNetUiLine(21, " The consoles' setups differ.");
        else
            ndsNetUiLine(21, " Out of sync at tick %lu.",
                         (unsigned long)(2u * gNdsNetDesyncBatch + 1u));
        ndsNetLockstepAbort();
        return;
    }
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
        if (ndsNetGuestHoldsLeave() != 0u)
        {
            ndsNetLeaveMatch();
            return;
        }
    }
    ndsNetPump();
    if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
        return; /* a player left */
    ndsNetSendInput();
    while (ndsNetBatchReady(sNetBatch) == 0u)
    {
        swiWaitForVBlank();
        waited++;
        ndsNetPump();
        if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
            return;
        if ((waited & 3u) == 0u)
        {
            ndsNetSendInput();
            /* A guest may leave while waiting, too. */
            (void)ndsPlatformReadInput();
            if (ndsNetGuestHoldsLeave() != 0u)
            {
                ndsNetLeaveMatch();
                return;
            }
        }
        if (waited == NDS_NET_STALL_NOTICE)
            ndsNetUiLine(21, " Waiting for the other players...");
        if (waited >= ((sNetBatch == 0u) ? NDS_NET_STALL_ABORT_FIRST :
                                           NDS_NET_STALL_ABORT_VBLANKS))
        {
            /* No progress: end the match rather than invent input. */
            ndsNetUiLine(21, " Connection lost.");
            ndsNetLockstepAbort();
            return;
        }
    }
    if (waited >= NDS_NET_STALL_NOTICE)
        ndsNetUiLine(21, "");
    if (waited != 0u)
    {
        gNdsNetStallBatches++;
        gNdsNetStallVBlanks += waited;
        if (waited > gNdsNetStallMaxVBlanks)
            gNdsNetStallMaxVBlanks = waited;
    }
}

/* A KO'd fighter in a team Stock match takes a stock from a teammate with
 * START (ftCommonSleepProcUpdate, ftcommonsleep.c:77), and the source's
 * pause trigger skips exactly that fighter (ifcommon.c:2920), so that START
 * is a steal, never a pause, and passes on any port. The trigger runs ahead of
 * the tick's fighters (ifCommonBattleGoUpdateInterface, before its gcRunAll),
 * so the state read here, before the tick, is the state it reads. */
static u32 ndsNetStartIsStockSteal(u32 p)
{
    GObj *fighter_gobj;

    if (gSCManagerBattleState->game_status != nSCBattleGameStatusGo)
        return FALSE;
    fighter_gobj = gSCManagerBattleState->players[p].fighter_gobj;
    if (fighter_gobj == NULL)
        return FALSE;
    return ((ftGetStruct(fighter_gobj)->status_id == nFTCommonStatusSleep) &&
            (ftCommonSleepCheckIgnorePauseMenu(fighter_gobj) != FALSE)) ?
        TRUE : FALSE;
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
         * dropped identically on every console, except where the source's
         * pause trigger passes that port over (ndsNetStartIsStockSteal). */
        if ((p != sNetHostPort) && ((button & START_BUTTON) != 0u) &&
            (ndsNetStartIsStockSteal(p) == FALSE))
            button &= (u16)~START_BUTTON;
        ndsControllerPlaybackSetPad(p, button, (s8)(value >> 16), (s8)(value >> 24));
    }
}

/* Plan 7.1's setup check: the general heap's size and how much of it setup
 * used decide the source's GObj latch (ifCommonSetMaxNumGObj, 25 KiB free),
 * so consoles whose arenas differ must not play on. Folded into the first
 * batch's digest, a difference ends the match at once like any divergence.
 * The arena's address is not compared: it still varies with the boot
 * environment (plan 7.2 item 4). */
static u32 ndsNetSetupDigest(void)
{
    const u32 size = (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
                           (uintptr_t)gSYTaskmanGeneralHeap.start);
    const u32 used = (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                           (uintptr_t)gSYTaskmanGeneralHeap.start);

    return (size * 2654435761u) ^ (used * 2246822519u) ^ 0x5E7u;
}

static u32 ndsNetMix(u32 hash, u32 value)
{
    hash ^= value;
    hash *= 16777619u;
    return hash ^ (hash >> 15);
}

static u32 ndsNetMixF32(u32 hash, f32 value)
{
    union
    {
        f32 f;
        u32 u;
    } bits;

    bits.f = value;
    return ndsNetMix(hash, bits.u);
}

static u32 ndsNetMixVec3(u32 hash, const Vec3f *v)
{
    hash = ndsNetMixF32(hash, v->x);
    hash = ndsNetMixF32(hash, v->y);
    return ndsNetMixF32(hash, v->z);
}

/* Plan 7.3's wider coverage folds values only, never a pointer: the arena's
 * address may differ between consoles (plan 7.2 item 4). The stage: every
 * moving collision object's transform and speed (what mpcollision reads), and
 * the hazard state each venue's ground update keys on -- Whispy's wind, the
 * clouds, the bumper, the barrel, the acid, the tornado, the Pokemon door,
 * the Arwing, the scales, the POW block. Cosmetic timers stay out. */
static u32 ndsNetDigestStage(u32 hash)
{
    const GRStruct *gr = &gGRCommonStruct;
    s32 i;

    if (gMPCollisionYakumonoDObjs != NULL)
    {
        for (i = 0; (i < gMPCollisionYakumonosNum) &&
                    (i < NDS_MP_YAKUMONO_DOBJ_SLOTS); i++)
        {
            const DObj *dobj = gMPCollisionYakumonoDObjs->dobjs[i];

            if (dobj != NULL)
                hash = ndsNetMixVec3(hash, &dobj->translate.vec.f);
            if (gMPCollisionSpeeds != NULL)
                hash = ndsNetMixVec3(hash, &gMPCollisionSpeeds[i]);
        }
    }
    switch (gSCManagerBattleState->gkind)
    {
    case nGRKindPupupu:
        hash = ndsNetMix(hash, (u32)gr->pupupu.whispy_wind_wait |
                               ((u32)gr->pupupu.whispy_wind_duration << 16));
        hash = ndsNetMix(hash, (u32)gr->pupupu.whispy_status |
                               ((u32)(u8)gr->pupupu.lr_players << 8));
        break;
    case nGRKindYoster:
        for (i = 0; i < 3; i++)
        {
            const GRYosterCloud *cloud = &gr->yoster.clouds[i];

            hash = ndsNetMixF32(hash, cloud->altitude);
            hash = ndsNetMixF32(hash, cloud->pressure);
            hash = ndsNetMix(hash, (u32)cloud->status |
                                   ((u32)cloud->is_cloud_line_active << 8) |
                                   ((u32)cloud->evaporate_wait << 16) |
                                   ((u32)(u8)cloud->pressure_timer << 24));
        }
        break;
    case nGRKindCastle:
        hash = ndsNetMixVec3(hash, &gr->castle.bumper_pos);
        break;
    case nGRKindJungle:
        hash = ndsNetMix(hash, (u32)gr->jungle.tarucann_status |
                               ((u32)gr->jungle.tarucann_wait << 16));
        hash = ndsNetMixF32(hash, gr->jungle.tarucann_rotate_step);
        break;
    case nGRKindZebes:
        hash = ndsNetMixF32(hash, gr->zebes.acid_level_curr);
        hash = ndsNetMixF32(hash, gr->zebes.acid_level_step);
        hash = ndsNetMix(hash, (u32)gr->zebes.acid_level_wait |
                               ((u32)gr->zebes.acid_status << 16) |
                               ((u32)gr->zebes.acid_attr_id << 24));
        break;
    case nGRKindHyrule:
        hash = ndsNetMixF32(hash, gr->hyrule.twister_vel);
        hash = ndsNetMix(hash, (u32)gr->hyrule.twister_wait |
                               ((u32)gr->hyrule.twister_speed_wait << 16));
        hash = ndsNetMix(hash, (u32)gr->hyrule.twister_turn_wait |
                               ((u32)gr->hyrule.twister_line_id << 16));
        hash = ndsNetMix(hash, (u32)gr->hyrule.twister_status);
        break;
    case nGRKindYamabuki:
        hash = ndsNetMix(hash, (u32)gr->yamabuki.gate_status |
                               ((u32)gr->yamabuki.gate_noentry << 8) |
                               ((u32)gr->yamabuki.monster_id_prev << 16));
        hash = ndsNetMix(hash, (u32)gr->yamabuki.monster_wait |
                               ((u32)gr->yamabuki.gate_wait << 16));
        break;
    case nGRKindSector:
        hash = ndsNetMixF32(hash, gr->sector.arwing_target_x);
        hash = ndsNetMix(hash, (u32)gr->sector.arwing_appear_timer |
                               ((u32)gr->sector.arwing_state_timer << 16));
        hash = ndsNetMix(hash, (u32)gr->sector.arwing_status |
                               ((u32)(u8)gr->sector.arwing_flight_pattern << 8) |
                               ((u32)gr->sector.arwing_laser_ammo << 16) |
                               ((u32)gr->sector.arwing_laser_count << 24));
        hash = ndsNetMix(hash, (u32)gr->sector.arwing_laser_timer);
        break;
    case nGRKindInishie:
        hash = ndsNetMixF32(hash, gr->inishie.splat_alt);
        hash = ndsNetMix(hash, (u32)gr->inishie.splat_wait |
                               ((u32)gr->inishie.splat_status << 16) |
                               ((u32)gr->inishie.pblock_status << 24));
        hash = ndsNetMix(hash, (u32)gr->inishie.pblock_appear_wait);
        break;
    default:
        break;
    }
    return hash;
}

/* The item fields the source's rules read beyond the replay digest's kind and
 * position: owner, team, facing, ground state, damage taken, lifetime, ammo,
 * hold/throw/pickup state and velocity. */
static u32 ndsNetDigestItems(u32 hash)
{
    GObj *gobj;

    for (gobj = gGCCommonLinks[nGCCommonLinkIDItem]; gobj != NULL;
         gobj = gobj->link_next)
    {
        const ITStruct *ip = (const ITStruct *)gobj->user_data.p;

        if (ip == NULL)
            continue;
        hash = ndsNetMix(hash, (u32)ip->player | ((u32)ip->team << 8) |
                               ((u32)(ip->ga != 0) << 16) |
                               ((ip->owner_gobj != NULL) ? (1u << 17) : 0u));
        hash = ndsNetMix(hash, (u32)ip->percent_damage);
        hash = ndsNetMix(hash, (u32)ip->lifetime);
        hash = ndsNetMix(hash, (u32)ip->lr);
        hash = ndsNetMix(hash, ip->hitlag_tics);
        hash = ndsNetMix(hash, (u32)ip->multi | ((u32)ip->attach_line_id << 16));
        hash = ndsNetMix(hash, (u32)ip->is_hold | ((u32)ip->is_thrown << 1) |
                               ((u32)ip->is_allow_pickup << 2) |
                               ((u32)ip->is_attach_surface << 3) |
                               ((u32)ip->times_landed << 4) |
                               ((u32)ip->times_thrown << 6) |
                               ((u32)ip->pickup_wait << 9));
        hash = ndsNetMixVec3(hash, &ip->physics.vel_air);
        hash = ndsNetMixF32(hash, ip->physics.vel_ground);
    }
    return hash;
}

/* Weapons likewise: owner, team, facing, ground state, damage and velocity --
 * only fields wpManagerMakeWeapon sets for every weapon. The weapon structs
 * come from a pool that is never cleared, and `lifetime` is set only by the
 * kinds that use it, so for the others it holds the slot's previous owner's
 * count, which differs between consoles with different histories. */
static u32 ndsNetDigestWeapons(u32 hash)
{
    GObj *gobj;

    for (gobj = gGCCommonLinks[nGCCommonLinkIDWeapon]; gobj != NULL;
         gobj = gobj->link_next)
    {
        const WPStruct *wp = (const WPStruct *)gobj->user_data.p;

        if (wp == NULL)
            continue;
        hash = ndsNetMix(hash, (u32)wp->player | ((u32)wp->team << 8) |
                               ((u32)(wp->ga != 0) << 16));
        hash = ndsNetMix(hash, (u32)wp->lr);
        hash = ndsNetMix(hash, (u32)wp->hit_normal_damage);
        hash = ndsNetMixVec3(hash, &wp->physics.vel_air);
        hash = ndsNetMixF32(hash, wp->physics.vel_ground);
    }
    return hash;
}

/* Plan 7.3's net digest: the replay digest plus the battle state that the
 * match's end and Results read -- game status, the timer, and each player's
 * stocks, place, KOs, falls, self-destructs, damage tallies, combo counts and
 * stale-move queue (stale moves scale damage). Net matches only: the replay
 * digest itself, and every gate that compares it, is unchanged. */
#if NDS_NET_LAB_SOAK
/* Lab: each batch's net digest after each of its parts in turn -- the replay
 * digest, then the battle state, the stage, the items, the weapons -- so a
 * mismatch names the first part that differs (gdb reads both consoles). */
volatile u32 gNdsNetLabParts[NDS_NET_DIGEST_RING][5];
#define NDS_NET_LAB_PART(k, h) \
    (gNdsNetLabParts[sNetBatch & (NDS_NET_DIGEST_RING - 1u)][k] = (h))
#else
#define NDS_NET_LAB_PART(k, h) ((void)0)
#endif

static u32 ndsNetDigest(void)
{
    const SCBattleState *bs = gSCManagerBattleState;
    u32 hash = ndsReplayDigestTick();
    u32 p;
    u32 i;

    NDS_NET_LAB_PART(0u, hash);

    hash = ndsNetMix(hash, bs->game_status);
    hash = ndsNetMix(hash, bs->time_remain);
    hash = ndsNetMix(hash, bs->time_passed);
    for (p = 0u; p < NDS_NET_PORTS; p++)
    {
        const SCPlayerData *pl = &bs->players[p];

        if (pl->pkind == nFTPlayerKindNot)
            continue;
        hash = ndsNetMix(hash, (u32)(u8)pl->stock_count | ((u32)pl->place << 8));
        hash = ndsNetMix(hash, (u32)pl->falls);
        hash = ndsNetMix(hash, (u32)pl->score);
        hash = ndsNetMix(hash, (u32)pl->total_selfdestructs);
        hash = ndsNetMix(hash, (u32)pl->total_damage_given);
        hash = ndsNetMix(hash, (u32)pl->total_damage_all);
        hash = ndsNetMix(hash, (u32)pl->stock_damage_all);
        hash = ndsNetMix(hash, (u32)pl->combo_damage_foe);
        hash = ndsNetMix(hash, (u32)pl->combo_count_foe);
        for (i = 0u; i < NDS_NET_PORTS; i++)
        {
            hash = ndsNetMix(hash, (u32)pl->total_kos_players[i]);
            hash = ndsNetMix(hash, (u32)pl->total_damage_players[i]);
        }
        hash = ndsNetMix(hash, pl->stale_id);
        for (i = 0u; i < 5u; i++)
        {
            hash = ndsNetMix(hash, (u32)pl->stale_info[i].attack_id |
                                   ((u32)pl->stale_info[i].motion_count << 16));
        }
    }
    NDS_NET_LAB_PART(1u, hash);
    hash = ndsNetDigestStage(hash);
    NDS_NET_LAB_PART(2u, hash);
    hash = ndsNetDigestItems(hash);
    NDS_NET_LAB_PART(3u, hash);
    hash = ndsNetDigestWeapons(hash);
    NDS_NET_LAB_PART(4u, hash);
    return hash;
}

void ndsNetBattleBatchDone(void)
{
    u32 slot;

    if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
        return;
    slot = sNetBatch & (NDS_NET_DIGEST_RING - 1u);
    sNetDigests[slot] = ndsNetDigest();
    if (sNetBatch == 0u)
        sNetDigests[slot] ^= ndsNetSetupDigest();
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
    const u32 slot = sNetBatch & (NDS_NET_DIGEST_RING - 1u);
    u32 waited;
    u32 linger = 0u;
    u32 p;

    if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
        return;
    /* Plan 7.3: Results come from the simulation every console ran, so the
     * terminal tick's digest is compared with every peer's before they
     * publish a winner. The scene ended inside batch sNetBatch on every
     * console alike (the loop leaves before that batch's BatchDone). Once
     * every peer's has been compared, this console keeps sending its own a
     * little longer for a peer still waiting. A difference, or a peer that
     * never sends one, ends the match as NO CONTEST like any divergence. */
    sNetDigests[slot] = ndsNetDigest();
    sNetDigestValid[slot] = sNetBatch + 1u;
    sNetLastDigestBatch = sNetBatch;
    for (waited = 0u; (waited < NDS_NET_TERMINAL_WAIT) &&
                      (linger < NDS_NET_TERMINAL_LINGER); waited++)
    {
        u32 pending = 0u;

        ndsNetPump();
        if (gNdsNetSessionState != NDS_NET_SESSION_RUNNING)
            return; /* a player left */
        for (p = 0u; p < NDS_NET_PORTS; p++)
        {
            if ((p == gNdsNetLocalPort) || (((gNdsNetHumanMask >> p) & 1u) == 0u))
                continue;
            ndsNetCompareDigest(p, sNetBatch);
            if (sNetCompared[p] != sNetBatch + 1u)
                pending++;
        }
        if ((waited & 1u) == 0u)
            ndsNetSendInput();
        if (pending == 0u)
            linger++;
        swiWaitForVBlank();
    }
    if ((gNdsNetDesyncBatch != 0xFFFFFFFFu) || (linger == 0u))
    {
        if (gNdsNetDesyncBatch != 0xFFFFFFFFu)
            ndsNetUiLine(21, " Out of sync at tick %lu.",
                         (unsigned long)(2u * gNdsNetDesyncBatch + 1u));
        else
            ndsNetUiLine(21, " Connection lost.");
        ndsNetLockstepAbort();
        return;
    }
    /* The lockstep stays armed: sudden death is another battle scene of the
     * same match. The live keypad comes back for Results; the lobby disarms
     * after the match. */
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
