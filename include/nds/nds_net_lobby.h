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
    /* In the host's snapshot, the costume the host resolved for the slot; in
     * a guest's proposal, a count of its costume presses (the host turns each
     * new one into a costume change by the source's rules). */
    uint8_t costume;
    /* The owner's cursor: its state + 1 (pointer, grab, hover), 0 for none.
     * Cosmetic, like the token position: never part of the match. */
    uint8_t cursor;
    int16_t puck_x;
    int16_t puck_y;
    int16_t cursor_x;
    int16_t cursor_y;
} NdsNetLobbySlot;

typedef struct NdsNetRoom
{
    uint32_t session;
    uint8_t host_mac[6];
    uint8_t humans;
    uint8_t phase;
    uint8_t same_build;   /* the room runs this console's build identity */
    char name[NDS_NET_NAME_LEN + 1u];
} NdsNetRoom;

/* Session lifecycle. Every call that can fail returns 0 on success. */
int ndsNetHostOpen(void);
int ndsNetJoinScanOpen(void);
uint32_t ndsNetJoinRooms(NdsNetRoom *rooms, uint32_t max);
int ndsNetJoinRequest(const NdsNetRoom *room);
/* 1 accepted, 0 waiting, -1 refused (full or closed), -2 no answer,
 * -3 refused because the room runs a different build. */
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
/* The ports whose player has picked (their token is down), from the slots the
 * host last published or this guest last heard. */
uint32_t ndsNetLobbyReadyMask(void);

/* Every menu frame while in a session: receive, time out, publish. */
void ndsNetLobbyPump(void);

/* Host. */
void ndsNetLobbyHostPublish(const NdsNetLobbySlot slots[4], uint32_t team_battle);
uint32_t ndsNetLobbyHostTakeProposal(uint32_t port, NdsNetLobbySlot *slot);
/* Whether that player has proposed from the lobby since the last match (a
 * player still on Results is not ready). */
uint32_t ndsNetLobbyHostMemberPresent(uint32_t port);
void ndsNetLobbyHostSetPhase(uint32_t phase);
/* The stage select's cursor cell, published so guests watch the host choose. */
void ndsNetLobbyHostSetStageCursor(uint32_t slot);
/* After the stage is committed into gNdsMatchConfig: send the descriptor and
 * seed until every guest acknowledges, then arm the lockstep. */
int ndsNetLobbyHostStartMatch(void);

/* Guest. */
uint32_t ndsNetLobbyGuestSnapshot(NdsNetLobbySlot slots[4], uint32_t *team_battle);
void ndsNetLobbyGuestPropose(const NdsNetLobbySlot *mine);
/* 1 once the host's START arrived: the descriptor is applied, the seed set,
 * the lockstep armed, and the battle may be entered. */
uint32_t ndsNetLobbyGuestStartReady(void);
/* The host's stage-select cursor cell from the latest snapshot. */
uint32_t ndsNetLobbyStageCursor(void);

/* After a match: back to the character select. */
void ndsNetLobbyMatchOver(void);

/* One lower-screen line of the rules the host's VS menu set (the character
 * select has no rule panel, and a guest never saw the host's VS menu): the
 * host's own, or a guest's copy from the latest snapshot. The match takes its
 * rules from the descriptor, never from this. */
#define NDS_NET_RULES_TEXT_LEN 32u
void ndsNetLobbyRulesText(char out[NDS_NET_RULES_TEXT_LEN + 1u]);

/* The save unlock mask that governs the room (plan 7.1): the host's, which a
 * guest has from the snapshot. It gates the stage select on every console and
 * is never written to a guest's save. */
uint32_t ndsNetLobbyUnlockMask(void);

/* A LEAVE the session layer read during a match (port, or 0xFF for the host
 * closing the room). The host drops that member if the radio address is its
 * own; a guest marks the room lost when the host closed it. Returns 1 when the
 * LEAVE is genuine. */
uint32_t ndsNetLobbyMemberLeft(const uint8_t mac[6], uint32_t port);

#endif
