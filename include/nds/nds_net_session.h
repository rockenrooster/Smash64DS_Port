/* P3 net session and lockstep (src/nds/net/nds_net_session.c).
 *
 * Every console runs the whole match from the same descriptor, seed and
 * input stream. A console records its own player's pad once per batch for the
 * batch NDS_NET_DELAY_BATCHES ahead, broadcasts its recent records every
 * batch, and runs a batch only when it holds both ticks' records for every
 * human port (docs/P3_Multiplayer/Smash64DS_Multiplayer_Plan.md sections 5-8). */
#ifndef NDS_NET_SESSION_H
#define NDS_NET_SESSION_H

#include <stdint.h>

#define NDS_NET_PORTS            4u
#define NDS_NET_DELAY_BATCHES    2u   /* input delay: 2 batches = 4 ticks */

/* Session state (gNdsNetSessionState). */
#define NDS_NET_SESSION_OFF      0u
#define NDS_NET_SESSION_RUNNING  2u   /* a lockstep match is in progress */
#define NDS_NET_SESSION_ABORTED  4u

/* The host's port in a lockstep match, NDS_NET_PORTS otherwise. Every console
 * holds the same value, so a rule keyed on it stays in lockstep (only the
 * host can pause: battleship_ftmanager.c ndsFTManagerLowDetailOnly). */
uint32_t ndsNetSessionHostPort(void);

/* Leave lockstep after a match (the character select calls it when no room
 * is open, so a finished lab match does not gate the next offline battle). */
void ndsNetLockstepDisarm(void);

/* VS battle scene start (src/import/battleship_scvsbattle.c), before the
 * source's setup runs: installs the agreed seed once per match. */
void ndsNetBattleSceneStart(void);

/* Battle loop seams (src/nds/nds_battle.c). */
void ndsNetBattleBegin(void);
void ndsNetBattleGate(void);                  /* blocks until the batch can run */
void ndsNetBattleInstallTick(uint32_t index); /* index 0/1 within the batch */
void ndsNetBattleBatchDone(void);
void ndsNetBattleEnd(void);


#endif
