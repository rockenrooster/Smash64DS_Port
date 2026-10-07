/* P3 lobby: rooms, joining, the shared character select and the match start
 * (src/nds/net/nds_net_lobby.c; plan section 10).
 *
 * The host owns membership, ports, rules, CPU slots and the stage; each guest
 * owns only its own port's pick. The host publishes a revisioned snapshot of
 * all four slots a few times a second; a guest proposes its own slot just as
 * often. When the host confirms the stage it sends the full descriptor and the
 * seed, every guest acknowledges, and every console enters the same battle in
 * lockstep. */
#ifndef NDS_NET_LOBBY_H
#define NDS_NET_LOBBY_H

#include <stdint.h>

#define NDS_NET_ROLE_NONE     0u
#define NDS_NET_ROLE_HOST     1u
#define NDS_NET_ROLE_GUEST    2u

/* Lobby phases, carried by every snapshot. */
#define NDS_NET_PHASE_CSS     0u   /* everyone picks */
#define NDS_NET_PHASE_STAGE   1u   /* the host is choosing the stage */
#define NDS_NET_PHASE_MATCH   2u   /* the match is starting or running */

#define NDS_NET_NAME_LEN      10u
#define NDS_NET_MAX_ROOMS     4u

typedef struct NdsNetLobbySlot
{
    uint8_t pkind;
    uint8_t fkind;
    uint8_t team;
    uint8_t level;
    uint8_t handicap;
    uint8_t selected;
    int16_t puck_x;
    int16_t puck_y;
} NdsNetLobbySlot;

typedef struct NdsNetRoom
{
    uint32_t session;
    uint8_t host_mac[6];
    uint8_t humans;
    uint8_t phase;
    char name[NDS_NET_NAME_LEN + 1u];
} NdsNetRoom;

/* Session lifecycle. Every call that can fail returns 0 on success. */
int ndsNetHostOpen(void);
int ndsNetJoinScanOpen(void);
uint32_t ndsNetJoinRooms(NdsNetRoom *rooms, uint32_t max);
int ndsNetJoinRequest(const NdsNetRoom *room);
/* 1 accepted, 0 waiting, negative refused or timed out. */
int ndsNetJoinPoll(void);
void ndsNetSessionClose(void);

uint32_t ndsNetRole(void);
uint32_t ndsNetInSession(void);
uint32_t ndsNetLobbyLocalPort(void);
uint32_t ndsNetLobbyHumanMask(void);
uint32_t ndsNetLobbyPhase(void);
/* Nonzero once the room is gone (host closed or silent, or this guest was
 * dropped); the caller closes the session and leaves the lobby screens. */
uint32_t ndsNetLobbyLost(void);
const char *ndsNetLobbyMemberName(uint32_t port);

/* Every menu frame while in a session: receive, time out, publish. */
void ndsNetLobbyPump(void);

/* Host. */
void ndsNetLobbyHostPublish(const NdsNetLobbySlot slots[4], uint32_t team_battle);
uint32_t ndsNetLobbyHostTakeProposal(uint32_t port, NdsNetLobbySlot *slot);
void ndsNetLobbyHostSetPhase(uint32_t phase);
/* After the stage is committed into gNdsMatchConfig: send the descriptor and
 * seed until every guest acknowledges, then arm the lockstep. */
int ndsNetLobbyHostStartMatch(void);

/* Guest. */
uint32_t ndsNetLobbyGuestSnapshot(NdsNetLobbySlot slots[4], uint32_t *team_battle);
void ndsNetLobbyGuestPropose(const NdsNetLobbySlot *mine);
/* 1 once the host's START arrived: the descriptor is applied, the seed set,
 * the lockstep armed, and the battle may be entered. */
uint32_t ndsNetLobbyGuestStartReady(void);

/* After a match: back to the character select. */
void ndsNetLobbyMatchOver(void);

#endif
