/* P3 wireless UI on the lower screen: the text console the platform keeps on
 * the sub engine (nds_platform.c) carries the Host/Join hints, the room list
 * and the lobby status, so no screen's baked art changes. */
#include <nds.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <nds/nds_net.h>
#include <nds/nds_net_lobby.h>
#include <nds/nds_net_ui.h>
#include <nds/nds_platform.h>

#define NDS_NET_UI_ROWS 24
#define NDS_NET_UI_COLS 32

static u32 sNdsNetUiShown;
static u32 sNdsNetUiLobbyKey = 0xFFFFFFFFu;
static char sNdsNetUiLobbyRules[NDS_NET_RULES_TEXT_LEN + 1u];

void ndsNetUiClear(void)
{
    sNdsNetUiLobbyKey = 0xFFFFFFFFu;
    if (sNdsNetUiShown != 0u)
    {
        consoleClear();
        sNdsNetUiShown = 0u;
    }
}

/* One row, padded to the full width so a shorter line erases a longer one. */
void ndsNetUiLine(u32 row, const char *fmt, ...)
{
    char text[NDS_NET_UI_COLS + 1];
    va_list ap;
    size_t n;

    if (row >= NDS_NET_UI_ROWS)
        return;
    va_start(ap, fmt);
    vsniprintf(text, sizeof(text), fmt, ap);
    va_end(ap);
    n = strlen(text);
    while (n < NDS_NET_UI_COLS)
        text[n++] = ' ';
    text[NDS_NET_UI_COLS] = '\0';
    iprintf("\x1b[%lu;0H%s", (unsigned long)row, text);
    sNdsNetUiShown = 1u;
}

void ndsNetUiLobbyStatus(void)
{
    const u32 role = ndsNetRole();
    const u32 mask = ndsNetLobbyHumanMask();
    const u32 phase = ndsNetLobbyPhase();
    const u32 ready = ndsNetLobbyReadyMask() & mask;
    const u32 key = (ready << 20) | (role << 16) | (mask << 8) | (phase << 4) |
                    ndsNetLobbyLocalPort();
    char rules[NDS_NET_RULES_TEXT_LEN + 1u];
    u32 p;

    if (role == NDS_NET_ROLE_NONE)
        return;
    ndsNetLobbyRulesText(rules);
    if (key == sNdsNetUiLobbyKey && strcmp(rules, sNdsNetUiLobbyRules) == 0)
        return;
    sNdsNetUiLobbyKey = key;
    memcpy(sNdsNetUiLobbyRules, rules, sizeof(rules));
    ndsNetUiLine(14, (role == NDS_NET_ROLE_HOST) ? " WIRELESS ROOM (you host)" :
                                                    " WIRELESS ROOM");
    for (p = 0u; p < 4u; p++)
    {
        if (((mask >> p) & 1u) != 0u)
            ndsNetUiLine(16 + p, "  P%u %-10s %-5s%s", (unsigned)(p + 1u),
                         ndsNetLobbyMemberName(p),
                         (((ready >> p) & 1u) != 0u) ? "ready" : "",
                         (p == ndsNetLobbyLocalPort()) ? " (you)" : "");
        else
            ndsNetUiLine(16 + p, "  P%u -", (unsigned)(p + 1u));
    }
    if (role == NDS_NET_ROLE_HOST)
        ndsNetUiLine(21, (phase == NDS_NET_PHASE_STAGE) ? " Choose the stage." :
                                                          " START when everyone is ready.");
    else
        ndsNetUiLine(21, (phase == NDS_NET_PHASE_STAGE) ? " The host is choosing a stage." :
                                                          " Pick your fighter.");
    ndsNetUiLine(20, "%s", rules);
    ndsNetUiLine(22, " Hold B to leave the room");
}

void ndsNetUiLobbyReset(void)
{
    ndsNetUiClear();
}
