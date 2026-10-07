/* P3 wireless UI on the lower screen (src/nds/net/nds_net_ui.c). */
#ifndef NDS_NET_UI_H
#define NDS_NET_UI_H

#include <stdint.h>

void ndsNetUiClear(void);
void ndsNetUiLine(uint32_t row, const char *fmt, ...);
/* Lobby rows on the character select and stage select; redraws on change. */
void ndsNetUiLobbyStatus(void);
void ndsNetUiLobbyReset(void);

#endif
