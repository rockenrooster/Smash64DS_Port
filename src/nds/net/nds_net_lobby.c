/* P3 lobby (include/nds/nds_net_lobby.h). Messages, after the common header
 * (nds_net_internal.h), all fields little-endian and serialized by hand:
 *
 *   LOBBY     host -> all, every 2 frames: revision u16, phase u8, team
 *             battle u8, human mask u8, accepting u8, host name (10 bytes),
 *             four 16-byte slots (pkind, fkind, team, level, handicap,
 *             selected, costume, cursor state, puck x s16, puck y s16, cursor
 *             x s16, cursor y s16), the host's build id (8 bytes) and its
 *             stage-select cursor cell (u8). Doubles as the room beacon.
 *   JOIN_REQ  guest -> host: nonce u32, name (10 bytes), build id (8 bytes)
 *   JOIN_ACK  host -> all: target MAC (6), port u8, result u8 (0 = in,
 *             1 = full or closed, 2 = a different build)
 *   PROPOSE   guest -> host, every 2 frames: port u8, pad, its 16-byte slot
 *   LEAVE     guest -> host: port u8; host -> all with port 0xFF: room closed
 *   START     host -> all: seed u32, delay u8, host port u8, human mask u8,
 *             pad, four transfer-state handicaps, then the descriptor
 *   START_ACK guest -> host: port u8
 *
 * The transport's sender MAC, never a self-asserted port, identifies a guest:
 * a PROPOSE whose MAC is not that port's member is ignored. */
#include <stdio.h>
#include <string.h>
#include <nds/bios.h>
#include <nds/interrupts.h>
#include <nds/timers.h>

#include <PR/os.h>
#include <ssb_types.h>
#include <ft/fighter.h>
#include <sc/scene.h>

#include <nds/nds_match_config.h>
#include <nds/nds_net.h>
#include <nds/nds_net_link.h>
#include <nds/nds_net_lobby.h>
#include <nds/nds_net_session.h>

#include "nds_net_internal.h"

/* Snapshots and proposals carry the cursors, so they go out at 30 Hz. */
#define NDS_NET_LOBBY_SEND_EVERY   2u
#define NDS_NET_LOBBY_TIMEOUT      240u   /* frames without a word: gone */
#define NDS_NET_ROOM_EXPIRE        120u
#define NDS_NET_JOIN_TIMEOUT       180u
#define NDS_NET_START_TIMEOUT      180u
#define NDS_NET_SLOT_BYTES         16u
#define NDS_NET_DESC_BYTES         50u
#define NDS_NET_BUILD_ID_BYTES     8u
#define NDS_NET_BUILD_ID_PATH      "nitro:/net/build.id"
/* After the slots and the build identity: the host's stage-select cursor
 * cell, its save's unlock mask (plan 7.1: the host's unlocks govern the room),
 * the rules its VS menu set (ndsNetLobbyRulesText), and the guests' names
 * (ports 1-3; the host's own is in the header), so every console lists every
 * player. */
#define NDS_NET_LOBBY_STAGE_AT \
    (16u + NDS_NET_PORTS * NDS_NET_SLOT_BYTES + NDS_NET_BUILD_ID_BYTES)
#define NDS_NET_LOBBY_RULES_AT (NDS_NET_LOBBY_STAGE_AT + 2u)
#define NDS_NET_LOBBY_RULES_BYTES 5u
#define NDS_NET_LOBBY_NAMES_AT \
    (NDS_NET_LOBBY_RULES_AT + NDS_NET_LOBBY_RULES_BYTES)
#define NDS_NET_LOBBY_BODY_BYTES \
    (NDS_NET_LOBBY_NAMES_AT + (NDS_NET_PORTS - 1u) * NDS_NET_NAME_LEN)

extern void ndsNetLinkGetUserName(char out[NDS_NET_NAME_LEN + 1u]);

typedef struct NdsNetMember
{
    u8 active;
    u8 has_proposal;
    u8 start_acked;
    u8 present;          /* proposing from the lobby since the last match */
    u8 mac[6];
    char name[NDS_NET_NAME_LEN + 1u];
    u32 last_heard;
    NdsNetLobbySlot proposal;
} NdsNetMember;

static u32 sRole;
static u32 sSession;
static u32 sLocalPort;
static u32 sHumanMask;
static u32 sPhase;
static u32 sLost;
static u32 sFrame;
static u8 sMyMac[6];
static char sMyName[NDS_NET_NAME_LEN + 1u];
/* Plan 7.1: the build identity scripts/nds_net_build_id.py writes at ROM
 * build time (all zero if the file is missing). */
static u8 sBuildId[NDS_NET_BUILD_ID_BYTES] __attribute__((aligned(4)));
static u32 sBuildIdRead;
/* Host: the stage-select cursor cell it publishes; guest: the last heard. */
static u32 sStageCursor;

/* Host. */
static NdsNetMember sMembers[NDS_NET_PORTS];
static NdsNetLobbySlot sHostSlots[NDS_NET_PORTS];
static u32 sHostTeamBattle;
static u32 sRevision;
/* After a match the consoles leave Results at their own pace: no lobby
 * timeouts until this frame. */
static u32 sGraceUntil;

/* Guest. */
static u8 sHostMac[6];
static u32 sHostLastHeard;
static NdsNetLobbySlot sSnapSlots[NDS_NET_PORTS];
static u32 sSnapTeamBattle;
static u32 sSnapFresh;
/* The host's save unlock mask and rules from the latest snapshot. */
static u32 sHostUnlockMask;
static u8 sHostRules[NDS_NET_LOBBY_RULES_BYTES];
static char sSnapNames[NDS_NET_PORTS][NDS_NET_NAME_LEN + 1u];
static char sHostName[NDS_NET_NAME_LEN + 1u];
static NdsNetLobbySlot sMyProposal;
static u32 sHaveProposal;
static u32 sStartReady;
static u32 sStartAckLeft;

/* Scanning / joining. */
static u32 sScanning;
static NdsNetRoom sRooms[NDS_NET_MAX_ROOMS];
static u32 sRoomSeen[NDS_NET_MAX_ROOMS];
static u32 sJoinState;        /* 0 idle, 1 waiting, 2 in, 3 refused */
static u32 sJoinSession;
static u32 sJoinNonce;
static u32 sJoinSentFrame;

volatile u32 gNdsNetLobbyJoins;
volatile u32 gNdsNetLobbyDrops;
volatile u32 gNdsNetLobbyStarts;
volatile u32 gNdsNetLobbyStartFailures;

uint32_t ndsNetRole(void) { return sRole; }
uint32_t ndsNetInSession(void) { return (sRole != NDS_NET_ROLE_NONE) ? 1u : 0u; }
uint32_t ndsNetLobbyLocalPort(void) { return sLocalPort; }
uint32_t ndsNetLobbyHumanMask(void) { return sHumanMask; }
uint32_t ndsNetLobbyPhase(void) { return sPhase; }
uint32_t ndsNetLobbyLost(void) { return sLost; }

const char *ndsNetLobbyMemberName(uint32_t port)
{
    if (port >= NDS_NET_PORTS)
        return "";
    if (sRole == NDS_NET_ROLE_HOST)
        return (sMembers[port].active != 0u) ? sMembers[port].name : "";
    if (port == 0u)
        return sHostName;
    return (port == sLocalPort) ? sMyName : sSnapNames[port];
}

uint32_t ndsNetLobbyReadyMask(void)
{
    const NdsNetLobbySlot *slots =
        (sRole == NDS_NET_ROLE_HOST) ? sHostSlots : sSnapSlots;
    uint32_t mask = 0u;
    u32 p;

    for (p = 0u; p < NDS_NET_PORTS; p++)
    {
        if (slots[p].selected != 0u)
            mask |= 1u << p;
    }
    return mask;
}

static u32 ndsNetPopCount(u32 v)
{
    u32 n = 0u;

    while (v != 0u)
    {
        n += v & 1u;
        v >>= 1;
    }
    return n;
}

static void ndsNetSend(const u8 *buf, u32 len)
{
    (void)ndsNetLinkSendPacket(buf, len);
}

static void ndsNetSlotWrite(u8 *p, const NdsNetLobbySlot *s)
{
    p[0] = s->pkind;
    p[1] = s->fkind;
    p[2] = s->team;
    p[3] = s->level;
    p[4] = s->handicap;
    p[5] = s->selected;
    p[6] = s->costume;
    p[7] = s->cursor;
    ndsNetPut16(p + 8, (u32)(u16)s->puck_x);
    ndsNetPut16(p + 10, (u32)(u16)s->puck_y);
    ndsNetPut16(p + 12, (u32)(u16)s->cursor_x);
    ndsNetPut16(p + 14, (u32)(u16)s->cursor_y);
}

static void ndsNetSlotRead(const u8 *p, NdsNetLobbySlot *s)
{
    s->pkind = p[0];
    s->fkind = p[1];
    s->team = p[2];
    s->level = p[3];
    s->handicap = p[4];
    s->selected = p[5];
    s->costume = p[6];
    s->cursor = p[7];
    s->puck_x = (s16)ndsNetGet16(p + 8);
    s->puck_y = (s16)ndsNetGet16(p + 10);
    s->cursor_x = (s16)ndsNetGet16(p + 12);
    s->cursor_y = (s16)ndsNetGet16(p + 14);
}

static void ndsNetLoadBuildId(void)
{
    FILE *file;

    if (sBuildIdRead != 0u)
        return;
    sBuildIdRead = 1u;
    file = fopen(NDS_NET_BUILD_ID_PATH, "rb");
    if (file == NULL)
        return;
    if (fread(sBuildId, 1, NDS_NET_BUILD_ID_BYTES, file) != NDS_NET_BUILD_ID_BYTES)
        memset(sBuildId, 0, sizeof(sBuildId));
    fclose(file);
}

/* --- The match descriptor ------------------------------------------------- */

static void ndsNetDescriptorWrite(u8 *p, const NdsMatchConfig *cfg)
{
    u32 i;

    for (i = 0u; i < NDS_NET_PORTS; i++)
    {
        const NdsMatchFighterConfig *f = &cfg->fighters[i];

        p[0] = f->fkind;
        p[1] = f->pkind;
        p[2] = f->level;
        p[3] = f->handicap;
        p[4] = f->team;
        p[5] = f->costume;
        p[6] = f->shade;
        p[7] = f->color;
        p += 8;
    }
    p[0] = cfg->gkind;
    p[1] = cfg->game_rules;
    p[2] = cfg->game_type;
    p[3] = cfg->spgame_stage;
    p[4] = cfg->time_limit;
    p[5] = cfg->stocks;
    p[6] = cfg->handicap_mode;
    p[7] = cfg->item_appearance_rate;
    p[8] = cfg->damage_ratio;
    p[9] = (u8)((cfg->is_team_battle ? 1u : 0u) | (cfg->is_team_attack ? 2u : 0u) |
                (cfg->is_stage_select ? 4u : 0u) | (cfg->is_reset_players ? 8u : 0u));
    ndsNetPut32(p + 10, cfg->item_toggles);
    /* 32 + 14 = 46 bytes; NDS_NET_DESC_BYTES leaves room to grow. */
}

static void ndsNetDescriptorRead(const u8 *p, NdsMatchConfig *cfg)
{
    u32 i;

    ndsMatchConfigLoadMarioFoxDreamLand(cfg);
    for (i = 0u; i < NDS_NET_PORTS; i++)
    {
        NdsMatchFighterConfig *f = &cfg->fighters[i];

        f->fkind = p[0];
        f->pkind = p[1];
        f->level = p[2];
        f->handicap = p[3];
        f->team = p[4];
        f->costume = p[5];
        f->shade = p[6];
        f->color = p[7];
        f->stock_count = 0;
        f->is_spgame_enemy = FALSE;
        f->copy_kind = NDS_MATCH_NO_COPY_KIND;
        p += 8;
    }
    cfg->gkind = p[0];
    cfg->game_rules = p[1];
    cfg->game_type = p[2];
    cfg->spgame_stage = p[3];
    cfg->time_limit = p[4];
    cfg->stocks = p[5];
    cfg->handicap_mode = p[6];
    cfg->item_appearance_rate = p[7];
    cfg->damage_ratio = p[8];
    cfg->is_team_battle = (p[9] & 1u) ? TRUE : FALSE;
    cfg->is_team_attack = (p[9] & 2u) ? TRUE : FALSE;
    cfg->is_stage_select = (p[9] & 4u) ? TRUE : FALSE;
    cfg->is_reset_players = (p[9] & 8u) ? TRUE : FALSE;
    cfg->item_toggles = ndsNetGet32(p + 10);
}

/* --- Session lifecycle ---------------------------------------------------- */

static void ndsNetLobbyReset(void)
{
    sRole = NDS_NET_ROLE_NONE;
    sSession = 0u;
    sLocalPort = 0u;
    sHumanMask = 0u;
    sPhase = NDS_NET_PHASE_CSS;
    sLost = 0u;
    sScanning = 0u;
    sJoinState = 0u;
    sStartReady = 0u;
    sStartAckLeft = 0u;
    sSnapFresh = 0u;
    sHaveProposal = 0u;
    sRevision = 0u;
    memset(sMembers, 0, sizeof(sMembers));
    memset(sRooms, 0, sizeof(sRooms));
    memset(sRoomSeen, 0, sizeof(sRoomSeen));
    memset(sHostName, 0, sizeof(sHostName));
    sHostUnlockMask = 0u;
    memset(sHostRules, 0, sizeof(sHostRules));
    memset(sSnapNames, 0, sizeof(sSnapNames));
}

static int ndsNetRadioUp(void)
{
    if (ndsNetLinkStart(NDS_NET_DEFAULT_CHANNEL) != NDS_NET_OK)
        return -1;
    ndsNetLinkGetMac(sMyMac);
    ndsNetLinkGetUserName(sMyName);
    ndsNetLoadBuildId();
    return 0;
}

int ndsNetHostOpen(void)
{
    ndsNetLobbyReset();
    if (ndsNetRadioUp() != 0)
        return -1;
    sRole = NDS_NET_ROLE_HOST;
    sSession = ((u32)cpuGetTiming() * 2654435761u) ^ ((u32)sMyMac[4] << 8) ^ sMyMac[5];
    if (sSession == 0u)
        sSession = 1u;
    sLocalPort = 0u;
    sHumanMask = 1u;
    sMembers[0].active = 1u;
    memcpy(sMembers[0].mac, sMyMac, 6);
    memcpy(sMembers[0].name, sMyName, sizeof(sMyName));
    sMembers[0].last_heard = sFrame;
    return 0;
}

int ndsNetJoinScanOpen(void)
{
    ndsNetLobbyReset();
    if (ndsNetRadioUp() != 0)
        return -1;
    sScanning = 1u;
    return 0;
}

uint32_t ndsNetJoinRooms(NdsNetRoom *rooms, uint32_t max)
{
    u32 n = 0u;
    u32 i;

    for (i = 0u; i < NDS_NET_MAX_ROOMS && n < max; i++)
    {
        if (sRooms[i].session != 0u)
            rooms[n++] = sRooms[i];
    }
    return n;
}

int ndsNetJoinRequest(const NdsNetRoom *room)
{
    u8 buf[NDS_NET_HDR_BYTES + 4u + NDS_NET_NAME_LEN + NDS_NET_BUILD_ID_BYTES];
    u32 n;

    if (room == NULL || room->session == 0u)
        return -1;
    if (room->same_build == 0u)
    {
        sJoinState = 4u;
        return -3;
    }
    sJoinSession = room->session;
    memcpy(sHostMac, room->host_mac, 6);
    sJoinNonce = (u32)cpuGetTiming() ^ sFrame;
    sJoinState = 1u;
    sJoinSentFrame = sFrame;
    n = ndsNetHeaderWrite(buf, NDS_NET_KIND_JOIN_REQ, sJoinSession);
    ndsNetPut32(buf + n, sJoinNonce);
    n += 4u;
    memcpy(buf + n, sMyName, NDS_NET_NAME_LEN);
    n += NDS_NET_NAME_LEN;
    memcpy(buf + n, sBuildId, NDS_NET_BUILD_ID_BYTES);
    n += NDS_NET_BUILD_ID_BYTES;
    ndsNetSend(buf, n);
    return 0;
}

int ndsNetJoinPoll(void)
{
    if (sJoinState == 2u)
        return 1;
    if (sJoinState == 3u)
        return -1;
    if (sJoinState == 4u)
        return -3;
    if (sJoinState == 1u && (sFrame - sJoinSentFrame) > NDS_NET_JOIN_TIMEOUT)
    {
        sJoinState = 0u;
        return -2;
    }
    return 0;
}

void ndsNetSessionClose(void)
{
    u8 buf[NDS_NET_HDR_BYTES + 2u];
    u32 n;
    u32 k;

    if (sRole != NDS_NET_ROLE_NONE && ndsNetLinkRunning() != 0u)
    {
        n = ndsNetHeaderWrite(buf, NDS_NET_KIND_LEAVE, sSession);
        buf[n++] = (sRole == NDS_NET_ROLE_HOST) ? 0xFFu : (u8)sLocalPort;
        buf[n++] = 0u;
        /* Said a few times: there is no acknowledgment for a goodbye. */
        for (k = 0u; k < 4u; k++)
        {
            ndsNetSend(buf, n);
            swiWaitForVBlank();
        }
    }
    ndsNetLobbyReset();
    ndsNetLinkStop();
}

/* --- Receive ------------------------------------------------------------- */

static void ndsNetHostUpdateMask(void)
{
    u32 p;

    sHumanMask = 0u;
    for (p = 0u; p < NDS_NET_PORTS; p++)
    {
        if (sMembers[p].active != 0u)
            sHumanMask |= 1u << p;
    }
}

static void ndsNetHostJoin(const u8 *src, const u8 *body, u32 len)
{
    u8 buf[NDS_NET_HDR_BYTES + 8u];
    u32 port = NDS_NET_PORTS;
    u32 result = 1u;
    u32 p;
    u32 n;

    if (len < 4u + NDS_NET_NAME_LEN)
        return;
    if (len < 4u + NDS_NET_NAME_LEN + NDS_NET_BUILD_ID_BYTES ||
        memcmp(body + 4u + NDS_NET_NAME_LEN, sBuildId, NDS_NET_BUILD_ID_BYTES) != 0)
    {
        /* Plan 7.1: a different build never joins (it would desync). */
        result = 2u;
        goto answer;
    }
    for (p = 1u; p < NDS_NET_PORTS; p++)
    {
        if (sMembers[p].active != 0u && memcmp(sMembers[p].mac, src, 6) == 0)
        {
            port = p;          /* a repeated request: answer it again */
            result = 0u;
            break;
        }
    }
    if (port == NDS_NET_PORTS && sPhase == NDS_NET_PHASE_CSS)
    {
        for (p = 1u; p < NDS_NET_PORTS; p++)
        {
            if (sMembers[p].active == 0u)
            {
                port = p;
                result = 0u;
                memset(&sMembers[p], 0, sizeof(sMembers[p]));
                sMembers[p].active = 1u;
                memcpy(sMembers[p].mac, src, 6);
                memcpy(sMembers[p].name, body + 4, NDS_NET_NAME_LEN);
                sMembers[p].name[NDS_NET_NAME_LEN] = '\0';
                sMembers[p].last_heard = sFrame;
                sMembers[p].present = 1u;
                gNdsNetLobbyJoins++;
                ndsNetHostUpdateMask();
                break;
            }
        }
    }
answer:
    n = ndsNetHeaderWrite(buf, NDS_NET_KIND_JOIN_ACK, sSession);
    memcpy(buf + n, src, 6);
    n += 6u;
    buf[n++] = (u8)((port < NDS_NET_PORTS) ? port : 0xFFu);
    buf[n++] = (u8)result;
    ndsNetSend(buf, n);
}

static void ndsNetRoomSeen(const u8 *src, u32 session, const u8 *body)
{
    u32 slot = NDS_NET_MAX_ROOMS;
    u32 i;

    for (i = 0u; i < NDS_NET_MAX_ROOMS; i++)
    {
        if (sRooms[i].session == session)
        {
            slot = i;
            break;
        }
    }
    if (slot == NDS_NET_MAX_ROOMS)
    {
        for (i = 0u; i < NDS_NET_MAX_ROOMS; i++)
        {
            if (sRooms[i].session == 0u)
            {
                slot = i;
                break;
            }
        }
    }
    if (slot == NDS_NET_MAX_ROOMS)
        return;
    sRooms[slot].session = session;
    memcpy(sRooms[slot].host_mac, src, 6);
    sRooms[slot].phase = body[2];
    sRooms[slot].humans = (u8)ndsNetPopCount(body[4]);
    sRooms[slot].same_build =
        (memcmp(body + 16u + NDS_NET_PORTS * NDS_NET_SLOT_BYTES, sBuildId,
                NDS_NET_BUILD_ID_BYTES) == 0) ? 1u : 0u;
    memcpy(sRooms[slot].name, body + 6, NDS_NET_NAME_LEN);
    sRooms[slot].name[NDS_NET_NAME_LEN] = '\0';
    sRoomSeen[slot] = sFrame;
}

static void ndsNetGuestStart(const u8 *body, u32 len)
{
    NdsMatchConfig cfg;
    u32 seed;
    u32 host_port;
    u32 mask;
    u32 i;

    if (len < 8u + 4u + NDS_NET_DESC_BYTES)
        return;
    if (sStartReady != 0u)
    {
        sStartAckLeft = 8u;   /* the host has not heard us yet */
        return;
    }
    seed = ndsNetGet32(body);
    host_port = body[5];
    mask = body[6];
    if (((mask >> sLocalPort) & 1u) == 0u)
        return;
    ndsNetDescriptorRead(body + 12, &cfg);
    gNdsMatchConfig = cfg;
    ndsMatchConfigApply(&gNdsMatchConfig);
    for (i = 0u; i < NDS_NET_PORTS; i++)
        gSCManagerTransferBattleState.players[i].handicap = body[8 + i];
    ndsNetLockstepConfigure(sSession, sLocalPort, host_port, mask);
    ndsNetLockstepSetSeed(seed);
    sPhase = NDS_NET_PHASE_MATCH;
    sStartReady = 1u;
    sStartAckLeft = 8u;
    /* No proposals until this console is back in the lobby: the host counts
     * a proposing player as present (ready for the next match). */
    sHaveProposal = 0u;
}

static void ndsNetLobbyReceive(void)
{
    u8 buf[NDS_NET_MAX_PAYLOAD + 2u];
    u8 src[6];
    u32 len;

    while ((len = ndsNetLinkRecvPacket(src, buf, sizeof(buf))) != 0u)
    {
        u32 kind;
        u32 session;
        const u8 *body = buf + NDS_NET_HDR_BYTES;
        const u32 blen = (len > NDS_NET_HDR_BYTES) ? len - NDS_NET_HDR_BYTES : 0u;

        if (ndsNetHeaderRead(buf, len, &kind, &session) == 0u)
            continue;

        /* Any snapshot long enough for the build identity lists the room
         * (another build's shows as such); only a full one updates a guest. */
        if (kind == NDS_NET_KIND_LOBBY && blen >= NDS_NET_LOBBY_STAGE_AT)
        {
            if (sScanning != 0u)
                ndsNetRoomSeen(src, session, body);
            if (sRole == NDS_NET_ROLE_GUEST && session == sSession &&
                blen >= NDS_NET_LOBBY_BODY_BYTES)
            {
                u32 p;

                sHostLastHeard = sFrame;
                sPhase = body[2] & 3u;
                sSnapTeamBattle = body[3];
                sHumanMask = body[4];
                memcpy(sHostName, body + 6, NDS_NET_NAME_LEN);
                sHostName[NDS_NET_NAME_LEN] = '\0';
                for (p = 0u; p < NDS_NET_PORTS; p++)
                    ndsNetSlotRead(body + 16 + p * NDS_NET_SLOT_BYTES, &sSnapSlots[p]);
                sStageCursor = body[NDS_NET_LOBBY_STAGE_AT];
                sHostUnlockMask = body[NDS_NET_LOBBY_STAGE_AT + 1u];
                memcpy(sHostRules, body + NDS_NET_LOBBY_RULES_AT,
                       NDS_NET_LOBBY_RULES_BYTES);
                for (p = 1u; p < NDS_NET_PORTS; p++)
                {
                    memcpy(sSnapNames[p], body + NDS_NET_LOBBY_NAMES_AT +
                                              (p - 1u) * NDS_NET_NAME_LEN,
                           NDS_NET_NAME_LEN);
                    sSnapNames[p][NDS_NET_NAME_LEN] = '\0';
                }
                sSnapFresh = 1u;
                if (((sHumanMask >> sLocalPort) & 1u) == 0u)
                    sLost = 1u;   /* the host dropped us */
            }
            continue;
        }
        if (sRole == NDS_NET_ROLE_HOST && session == sSession)
        {
            u32 p;

            switch (kind)
            {
            case NDS_NET_KIND_JOIN_REQ:
                ndsNetHostJoin(src, body, blen);
                break;
            case NDS_NET_KIND_PROPOSE:
                p = (blen >= 2u + NDS_NET_SLOT_BYTES) ? body[0] : NDS_NET_PORTS;
                if (p > 0u && p < NDS_NET_PORTS && sMembers[p].active != 0u &&
                    memcmp(sMembers[p].mac, src, 6) == 0)
                {
                    ndsNetSlotRead(body + 2, &sMembers[p].proposal);
                    sMembers[p].has_proposal = 1u;
                    sMembers[p].present = 1u;
                    sMembers[p].last_heard = sFrame;
                }
                break;
            case NDS_NET_KIND_LEAVE:
                p = (blen >= 1u) ? body[0] : NDS_NET_PORTS;
                if (p > 0u && p < NDS_NET_PORTS && sMembers[p].active != 0u &&
                    memcmp(sMembers[p].mac, src, 6) == 0)
                {
                    sMembers[p].active = 0u;
                    gNdsNetLobbyDrops++;
                    ndsNetHostUpdateMask();
                }
                break;
            case NDS_NET_KIND_START_ACK:
                p = (blen >= 1u) ? body[0] : NDS_NET_PORTS;
                if (p > 0u && p < NDS_NET_PORTS && sMembers[p].active != 0u &&
                    memcmp(sMembers[p].mac, src, 6) == 0)
                {
                    sMembers[p].start_acked = 1u;
                    sMembers[p].last_heard = sFrame;
                }
                break;
            default:
                break;
            }
            continue;
        }
        if (kind == NDS_NET_KIND_JOIN_ACK && sJoinState == 1u &&
            session == sJoinSession && blen >= 8u && memcmp(body, sMyMac, 6) == 0)
        {
            if (body[7] == 2u)
            {
                sJoinState = 4u;
            }
            else if (body[7] == 0u && body[6] < NDS_NET_PORTS)
            {
                sRole = NDS_NET_ROLE_GUEST;
                sSession = session;
                sLocalPort = body[6];
                sHostLastHeard = sFrame;
                sScanning = 0u;
                sPhase = NDS_NET_PHASE_CSS;
                sJoinState = 2u;
            }
            else
            {
                sJoinState = 3u;
            }
            continue;
        }
        if (sRole == NDS_NET_ROLE_GUEST && session == sSession)
        {
            if (kind == NDS_NET_KIND_START)
                ndsNetGuestStart(body, blen);
            else if (kind == NDS_NET_KIND_LEAVE && blen >= 1u && body[0] == 0xFFu)
                sLost = 1u;
        }
    }
}

/* --- Send ---------------------------------------------------------------- */

/* The rules the host's VS menu committed to gNdsMatchConfig: rule, time,
 * stocks, item rate, and whether any item and team attack are on. */
static void ndsNetRulesWrite(u8 *p)
{
    p[0] = gNdsMatchConfig.game_rules;
    p[1] = gNdsMatchConfig.time_limit;
    p[2] = gNdsMatchConfig.stocks;
    p[3] = gNdsMatchConfig.item_appearance_rate;
    p[4] = (u8)(((gNdsMatchConfig.item_toggles != 0u) ? 1u : 0u) |
                ((gNdsMatchConfig.is_team_attack != FALSE) ? 2u : 0u));
}

static void ndsNetHostSendLobby(void)
{
    u8 buf[NDS_NET_HDR_BYTES + NDS_NET_LOBBY_BODY_BYTES];
    u32 n = ndsNetHeaderWrite(buf, NDS_NET_KIND_LOBBY, sSession);
    u32 p;

    ndsNetPut16(buf + n, sRevision);
    buf[n + 2] = (u8)sPhase;
    buf[n + 3] = (u8)sHostTeamBattle;
    buf[n + 4] = (u8)sHumanMask;
    buf[n + 5] = (sPhase == NDS_NET_PHASE_CSS &&
                  ndsNetPopCount(sHumanMask) < NDS_NET_PORTS) ? 1u : 0u;
    memcpy(buf + n + 6, sMyName, NDS_NET_NAME_LEN);
    n += 16u;
    for (p = 0u; p < NDS_NET_PORTS; p++)
    {
        ndsNetSlotWrite(buf + n, &sHostSlots[p]);
        n += NDS_NET_SLOT_BYTES;
    }
    memcpy(buf + n, sBuildId, NDS_NET_BUILD_ID_BYTES);
    n += NDS_NET_BUILD_ID_BYTES;
    buf[n++] = (u8)sStageCursor;
    buf[n++] = gSCManagerBackupData.unlock_mask;
    ndsNetRulesWrite(buf + n);
    n += NDS_NET_LOBBY_RULES_BYTES;
    for (p = 1u; p < NDS_NET_PORTS; p++)
    {
        if (sMembers[p].active != 0u)
            memcpy(buf + n, sMembers[p].name, NDS_NET_NAME_LEN);
        else
            memset(buf + n, 0, NDS_NET_NAME_LEN);
        n += NDS_NET_NAME_LEN;
    }
    ndsNetSend(buf, n);
}

static void ndsNetGuestSendProposal(void)
{
    u8 buf[NDS_NET_HDR_BYTES + 2u + NDS_NET_SLOT_BYTES];
    u32 n = ndsNetHeaderWrite(buf, NDS_NET_KIND_PROPOSE, sSession);

    buf[n++] = (u8)sLocalPort;
    buf[n++] = 0u;
    ndsNetSlotWrite(buf + n, &sMyProposal);
    n += NDS_NET_SLOT_BYTES;
    ndsNetSend(buf, n);
}

static void ndsNetGuestSendStartAck(void)
{
    u8 buf[NDS_NET_HDR_BYTES + 2u];
    u32 n = ndsNetHeaderWrite(buf, NDS_NET_KIND_START_ACK, sSession);

    buf[n++] = (u8)sLocalPort;
    buf[n++] = 0u;
    ndsNetSend(buf, n);
}

void ndsNetLobbyPump(void)
{
    u32 p;

    sFrame++;
    if (ndsNetLinkRunning() == 0u)
        return;
    ndsNetLobbyReceive();

    if (sScanning != 0u)
    {
        for (p = 0u; p < NDS_NET_MAX_ROOMS; p++)
        {
            if (sRooms[p].session != 0u && (sFrame - sRoomSeen[p]) > NDS_NET_ROOM_EXPIRE)
                sRooms[p].session = 0u;
        }
    }
    if (sRole == NDS_NET_ROLE_HOST)
    {
        for (p = 1u; p < NDS_NET_PORTS; p++)
        {
            if (sMembers[p].active != 0u && sPhase != NDS_NET_PHASE_MATCH &&
                (s32)(sFrame - sGraceUntil) > 0 &&
                (sFrame - sMembers[p].last_heard) > NDS_NET_LOBBY_TIMEOUT)
            {
                sMembers[p].active = 0u;
                gNdsNetLobbyDrops++;
                ndsNetHostUpdateMask();
            }
        }
        if ((sFrame % NDS_NET_LOBBY_SEND_EVERY) == 0u)
            ndsNetHostSendLobby();
    }
    else if (sRole == NDS_NET_ROLE_GUEST)
    {
        if (sPhase != NDS_NET_PHASE_MATCH && (s32)(sFrame - sGraceUntil) > 0 &&
            (sFrame - sHostLastHeard) > NDS_NET_LOBBY_TIMEOUT)
            sLost = 1u;
        if (sStartAckLeft != 0u)
        {
            sStartAckLeft--;
            ndsNetGuestSendStartAck();
        }
        else if (sHaveProposal != 0u && (sFrame % NDS_NET_LOBBY_SEND_EVERY) == 1u)
        {
            ndsNetGuestSendProposal();
        }
    }
}

/* --- Host API ------------------------------------------------------------ */

void ndsNetLobbyHostPublish(const NdsNetLobbySlot slots[4], uint32_t team_battle)
{
    if (memcmp(sHostSlots, slots, sizeof(sHostSlots)) != 0 ||
        sHostTeamBattle != team_battle)
    {
        memcpy(sHostSlots, slots, sizeof(sHostSlots));
        sHostTeamBattle = team_battle;
        sRevision++;
    }
}

void ndsNetLobbyHostSetStageCursor(uint32_t slot)
{
    if (sRole == NDS_NET_ROLE_HOST)
        sStageCursor = slot;
}

uint32_t ndsNetLobbyStageCursor(void)
{
    return sStageCursor;
}

uint32_t ndsNetLobbyHostMemberPresent(uint32_t port)
{
    if (sRole != NDS_NET_ROLE_HOST || port >= NDS_NET_PORTS)
        return 0u;
    return (port == 0u) ? 1u : sMembers[port].present;
}

uint32_t ndsNetLobbyHostTakeProposal(uint32_t port, NdsNetLobbySlot *slot)
{
    if (sRole != NDS_NET_ROLE_HOST || port == 0u || port >= NDS_NET_PORTS ||
        sMembers[port].active == 0u || sMembers[port].has_proposal == 0u)
        return 0u;
    *slot = sMembers[port].proposal;
    sMembers[port].has_proposal = 0u;
    return 1u;
}

void ndsNetLobbyHostSetPhase(uint32_t phase)
{
    if (sRole == NDS_NET_ROLE_HOST && sPhase != phase)
    {
        sPhase = phase;
        sRevision++;
        ndsNetHostSendLobby();
    }
}

#if NDS_NET_LAB_SWEEP
extern s32 ftParamGetCostumeCommonID(s32 fkind, s32 color);

volatile u32 gNdsNetLabSweepMatches;
/* Poked nonzero by a harness to replay one reported match instead of the
 * sweep (ndsNetLabSweepDescriptor). */
volatile u32 gNdsNetLabSweepRepro;

#if NDS_NET_LAB_TEAMS
extern s32 ftParamGetCostumeTeamID(s32 fkind, s32 color);

/* Lab teams: the sweep's matches cycle through four Team Battle layouts and
 * a free-for-all. Each human with a CPU partner (team attack off); the same
 * in four-stock Stock with team attack on, where the guest's scripted START
 * takes a stock from its CPU partner once its own fighter is out
 * (ftCommonSleepProcUpdate); three teams; both humans against the CPUs.
 * Costume and shade follow the character select's team rules
 * (mnplayersvs.c:1859-1860): the team costume, and the lowest shade no other
 * same-fighter teammate holds, in port order. */
static void ndsNetLabTeamsDescriptor(NdsMatchConfig *cfg, u32 m)
{
    static const u8 kTeams[4][NDS_NET_PORTS] = {
        { nSCBattleTeamIDRed, nSCBattleTeamIDBlue,
          nSCBattleTeamIDRed, nSCBattleTeamIDBlue },
        { nSCBattleTeamIDRed, nSCBattleTeamIDBlue,
          nSCBattleTeamIDBlue, nSCBattleTeamIDRed },
        { nSCBattleTeamIDRed, nSCBattleTeamIDBlue,
          nSCBattleTeamIDGreen, nSCBattleTeamIDGreen },
        { nSCBattleTeamIDRed, nSCBattleTeamIDRed,
          nSCBattleTeamIDBlue, nSCBattleTeamIDBlue },
    };
    const u32 v = m % 5u;
    u32 i;
    u32 j;

    if (v == 4u)
        return;
    cfg->is_team_battle = TRUE;
    cfg->is_team_attack = (v != 0u) ? TRUE : FALSE;
    if (v == 1u)
    {
        cfg->game_rules = SCBATTLE_GAMERULE_STOCK;
        cfg->stocks = 3u;
    }
    for (i = 0u; i < NDS_NET_PORTS; i++)
    {
        NdsMatchFighterConfig *f = &cfg->fighters[i];
        u32 used = 0u;

        f->team = kTeams[v][i];
        f->costume = (u8)ftParamGetCostumeTeamID((s32)f->fkind, (s32)f->team);
        for (j = 0u; j < i; j++)
        {
            if ((cfg->fighters[j].fkind == f->fkind) &&
                (cfg->fighters[j].team == f->team))
                used |= 1u << cfg->fighters[j].shade;
        }
        f->shade = 0u;
        while (((used >> f->shade) & 1u) != 0u)
            f->shade++;
    }
}
#endif

/* Lab sweep (plan 7.4): the host's descriptor walks the nine VS stages and
 * the roster, adding two level-9 CPUs (Kirby among them, for copies) and
 * every item at the highest rate, in one-minute time matches (ties go to
 * sudden death). The guest receives it like any other descriptor. */
static void ndsNetLabSweepDescriptor(NdsMatchConfig *cfg)
{
    static const u8 kStages[9] = {
        nGRKindPupupu, nGRKindCastle, nGRKindSector, nGRKindJungle,
        nGRKindZebes, nGRKindHyrule, nGRKindYoster, nGRKindYamabuki,
        nGRKindInishie,
    };
    static const u8 kRoster[12] = {
        nFTKindKirby, nFTKindSamus, nFTKindLink, nFTKindYoshi,
        nFTKindCaptain, nFTKindPikachu, nFTKindPurin, nFTKindNess,
        nFTKindKirby, nFTKindFox, nFTKindDonkey, nFTKindLuigi,
    };
    static const u8 kAllRows[NDS_ITEM_SWITCH_TOGGLE_COUNT] = {
        1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
    };
    const u32 m = gNdsNetLabSweepMatches++;
    u32 i;

    if (gNdsNetLabSweepRepro != 0u)
    {
        /* A gdb-poked repro: Kirby (host) against Yoshi (guest) on Hyrule,
         * two humans and no CPUs, two-minute time, every item at Medium. */
        cfg->gkind = nGRKindHyrule;
        for (i = 0u; i < NDS_NET_PORTS; i++)
        {
            NdsMatchFighterConfig *f = &cfg->fighters[i];

            f->pkind = (i < 2u) ? nFTPlayerKindMan : nFTPlayerKindNot;
            f->fkind = (i == 0u) ? nFTKindKirby :
                       (i == 1u) ? nFTKindYoshi : nFTKindNull;
            f->team = (u8)i;
            f->costume = (i < 2u) ?
                (u8)ftParamGetCostumeCommonID((s32)f->fkind, (s32)i) : 0u;
            f->shade = 0u;
        }
        cfg->is_team_battle = FALSE;
        cfg->is_team_attack = FALSE;
        cfg->game_rules = SCBATTLE_GAMERULE_TIME;
        cfg->time_limit = 2u;
        cfg->item_toggles = ndsMatchConfigItemTogglesFromRows(kAllRows);
        cfg->item_appearance_rate = nSCBattleItemSwitchMiddle;
        ndsMatchConfigApply(cfg);
        return;
    }
    cfg->gkind = kStages[m % 9u];
    for (i = 2u; i < NDS_NET_PORTS; i++)
    {
        NdsMatchFighterConfig *f = &cfg->fighters[i];

        f->pkind = nFTPlayerKindCom;
        f->fkind = kRoster[(2u * m + i) % 12u];
        f->level = 9u;
        f->team = (u8)((i == 2u) ? nSCBattleTeamIDGreen : nSCBattleTeamIDBlue);
        f->costume = (u8)ftParamGetCostumeCommonID((s32)f->fkind, (s32)i);
        f->shade = 0u;
    }
    cfg->is_team_battle = FALSE;
    cfg->is_team_attack = FALSE;
    cfg->game_rules = SCBATTLE_GAMERULE_TIME;
    cfg->time_limit = 1u;
    cfg->item_toggles = ndsMatchConfigItemTogglesFromRows(kAllRows);
    cfg->item_appearance_rate = nSCBattleItemSwitchVeryHigh;
#if NDS_NET_LAB_TEAMS
    ndsNetLabTeamsDescriptor(cfg, m);
#endif
    ndsMatchConfigApply(cfg);
}
#endif

int ndsNetLobbyHostStartMatch(void)
{
    u8 buf[NDS_NET_HDR_BYTES + 12u + NDS_NET_DESC_BYTES];
    const u32 seed = ((u32)cpuGetTiming() * 2246822519u) ^ (sFrame * 3266489917u) ^ sSession;
    u32 n;
    u32 p;
    u32 f;
    u32 waiting = 0u;

    if (sRole != NDS_NET_ROLE_HOST)
        return -1;
    for (p = 1u; p < NDS_NET_PORTS; p++)
        sMembers[p].start_acked = 0u;
    sPhase = NDS_NET_PHASE_MATCH;
    memset(buf, 0, sizeof(buf));
    n = ndsNetHeaderWrite(buf, NDS_NET_KIND_START, sSession);
    ndsNetPut32(buf + n, seed);
    buf[n + 4] = (u8)NDS_NET_DELAY_BATCHES;
    buf[n + 5] = 0u; /* host port */
    buf[n + 6] = (u8)sHumanMask;
    buf[n + 7] = 0u;
    for (p = 0u; p < NDS_NET_PORTS; p++)
        buf[n + 8 + p] = gSCManagerTransferBattleState.players[p].handicap;
#if NDS_NET_LAB_SWEEP
    ndsNetLabSweepDescriptor(&gNdsMatchConfig);
#endif
    ndsNetDescriptorWrite(buf + n + 12, &gNdsMatchConfig);
    n += 12u + NDS_NET_DESC_BYTES;

    for (f = 0u; f < NDS_NET_START_TIMEOUT; f++)
    {
        if ((f % 3u) == 0u)
            ndsNetSend(buf, n);
        sFrame++;
        ndsNetLobbyReceive();
        waiting = 0u;
        for (p = 1u; p < NDS_NET_PORTS; p++)
        {
            if (sMembers[p].active != 0u && sMembers[p].start_acked == 0u)
                waiting++;
        }
        if (waiting == 0u)
            break;
        swiWaitForVBlank();
    }
    if (waiting != 0u)
    {
        gNdsNetLobbyStartFailures++;
        sPhase = NDS_NET_PHASE_CSS;
        return -1;
    }
    gNdsNetLobbyStarts++;
    ndsNetLockstepConfigure(sSession, 0u, 0u, sHumanMask);
    ndsNetLockstepSetSeed(seed);
    return 0;
}

/* --- Guest API ----------------------------------------------------------- */

uint32_t ndsNetLobbyGuestSnapshot(NdsNetLobbySlot slots[4], uint32_t *team_battle)
{
    if (sRole != NDS_NET_ROLE_GUEST || sSnapFresh == 0u)
        return 0u;
    memcpy(slots, sSnapSlots, sizeof(sSnapSlots));
    *team_battle = sSnapTeamBattle;
    sSnapFresh = 0u;
    return 1u;
}

void ndsNetLobbyGuestPropose(const NdsNetLobbySlot *mine)
{
    sMyProposal = *mine;
    sHaveProposal = 1u;
}

uint32_t ndsNetLobbyGuestStartReady(void)
{
    return (sRole == NDS_NET_ROLE_GUEST) ? sStartReady : 0u;
}

void ndsNetLobbyMatchOver(void)
{
    ndsNetLockstepDisarm();
    sStartReady = 0u;
    sStartAckLeft = 0u;
    sGraceUntil = sFrame + 60u * 60u;
    if (sRole == NDS_NET_ROLE_HOST)
    {
        u32 p;

        /* Everyone was busy fighting: nobody timed out meanwhile, and nobody
         * counts as ready until they propose from the lobby again. */
        for (p = 1u; p < NDS_NET_PORTS; p++)
        {
            sMembers[p].last_heard = sFrame;
            sMembers[p].present = 0u;
            sMembers[p].has_proposal = 0u;
        }
        sPhase = NDS_NET_PHASE_CSS;
        sRevision++;
    }
    else if (sRole == NDS_NET_ROLE_GUEST)
    {
        sHostLastHeard = sFrame;
        sPhase = NDS_NET_PHASE_CSS;
    }
}

uint32_t ndsNetLobbyUnlockMask(void)
{
    return (sRole == NDS_NET_ROLE_GUEST) ? sHostUnlockMask :
        (u32)gSCManagerBackupData.unlock_mask;
}

void ndsNetLobbyRulesText(char out[NDS_NET_RULES_TEXT_LEN + 1u])
{
    static const char *const kRates[] = {
        "off", "very low", "low", "medium", "high", "very high",
    };
    u8 rules[NDS_NET_LOBBY_RULES_BYTES];
    const char *items;
    char rule[16];

    if (sRole == NDS_NET_ROLE_GUEST)
        memcpy(rules, sHostRules, sizeof(rules));
    else
        ndsNetRulesWrite(rules);
    if ((rules[0] & (SCBATTLE_GAMERULE_TIME | SCBATTLE_GAMERULE_STOCK)) == 0u)
    {
        out[0] = '\0'; /* no snapshot yet */
        return;
    }
    items =(((rules[4] & 1u) == 0u) || (rules[3] == 0u) ||
             (rules[3] >= (u8)nSCBattleItemSwitchEnumCount)) ?
        "off" : kRates[rules[3]];
    if ((rules[0] & SCBATTLE_GAMERULE_STOCK) != 0u)
        sniprintf(rule, sizeof(rule), "Stock %u", (unsigned)rules[2] + 1u);
    else if (rules[1] == SCBATTLE_TIMELIMIT_INFINITE)
        sniprintf(rule, sizeof(rule), "No time limit");
    else
        sniprintf(rule, sizeof(rule), "Time %u:00", (unsigned)rules[1]);
    sniprintf(out, NDS_NET_RULES_TEXT_LEN + 1u, " %s, items %s%s", rule, items,
              ((rules[4] & 2u) != 0u) ? ", TA" : "");
}

uint32_t ndsNetLobbyMemberLeft(const uint8_t mac[6], uint32_t port)
{
    if (sRole == NDS_NET_ROLE_HOST)
    {
        if (port == 0u || port >= NDS_NET_PORTS || sMembers[port].active == 0u ||
            memcmp(sMembers[port].mac, mac, 6) != 0)
            return 0u;
        sMembers[port].active = 0u;
        gNdsNetLobbyDrops++;
        ndsNetHostUpdateMask();
        return 1u;
    }
    if (sRole == NDS_NET_ROLE_GUEST)
    {
        if (port == 0xFFu)
        {
            if (memcmp(sHostMac, mac, 6) != 0)
                return 0u;
            sLost = 1u;
            return 1u;
        }
        /* Another guest: the host drops it and publishes the new room. */
        return (port < NDS_NET_PORTS && port != sLocalPort) ? 1u : 0u;
    }
    return 0u;
}
