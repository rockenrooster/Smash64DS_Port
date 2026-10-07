/* P3 wireless UI on the lower screen (src/nds/net/nds_net_ui.c). */
#ifndef NDS_NET_UI_H
#define NDS_NET_UI_H

#include <stdint.h>

void ndsNetUiClear(void);
void ndsNetUiLine(uint32_t row, const char *fmt, ...);
/* VS Mode's hint rows: X hosts, Y joins. */
void ndsNetUiVsModeHint(uint32_t host_on);
/* The room list. Returns 1 once a room accepted this console (go to the
 * character select), 0 if the player backed out (radio off). */
int ndsNetUiJoinModal(void);
/* Lobby rows on the character select and stage select; redraws on change. */
void ndsNetUiLobbyStatus(void);
void ndsNetUiLobbyReset(void);

#endif
