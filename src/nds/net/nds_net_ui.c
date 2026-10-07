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

void ndsNetUiVsModeHint(u32 host_on)
{
    ndsNetUiLine(14, " WIRELESS PLAY (up to 4 DS)");
    ndsNetUiLine(16, "  X  Host a room ......... %s", host_on ? "ON" : "OFF");
    ndsNetUiLine(17, "  Y  Join a room");
    ndsNetUiLine(19, host_on ? "  VS START opens your room" : "");
}

static u32 ndsNetUiKeys(void)
{
    (void)ndsPlatformReadInput();
    return ndsPlatformHeldKeys();
}

static void ndsNetUiWaitRelease(void)
{
    do
    {
        swiWaitForVBlank();
        ndsNetLobbyPump();
    } while ((ndsNetUiKeys() & (KEY_A | KEY_B | KEY_START)) != 0u);
}

int ndsNetUiJoinModal(void)
{
    NdsNetRoom rooms[NDS_NET_MAX_ROOMS];
    u32 cursor = 0u;
    u32 joining = 0u;
    u32 prev = ndsNetUiKeys();
    const char *status = "Searching for rooms...";
    u32 frame = 0u;

    ndsNetUiClear();
    if (ndsNetJoinScanOpen() != 0)
    {
        ndsNetUiLine(2, " JOIN A ROOM");
        ndsNetUiLine(5, " Wireless could not start.");
        ndsNetUiLine(22, " B  Back");
        do
        {
            swiWaitForVBlank();
        } while ((ndsNetUiKeys() & KEY_B) == 0u);
        ndsNetUiWaitRelease();
        ndsNetUiClear();
        return 0;
    }
    for (;;)
    {
        u32 down;
        u32 n;
        u32 i;

        swiWaitForVBlank();
        {
            const u32 held = ndsNetUiKeys();

            down = held & ~prev;
            prev = held;
        }
        ndsNetLobbyPump();
        n = ndsNetJoinRooms(rooms, NDS_NET_MAX_ROOMS);
        if (cursor >= n)
            cursor = (n != 0u) ? n - 1u : 0u;

        if (joining != 0u)
        {
            const int r = ndsNetJoinPoll();

            if (r > 0)
            {
                ndsNetUiWaitRelease();
                ndsNetUiClear();
                return 1;
            }
            if (r < 0)
            {
                joining = 0u;
                status = (r == -1) ? "The room is full." :
                         (r == -3) ? "That room runs another build." :
                                     "No answer from the room.";
            }
        }
        else
        {
            if ((down & KEY_UP) != 0u && cursor > 0u)
                cursor--;
            if ((down & KEY_DOWN) != 0u && cursor + 1u < n)
                cursor++;
            if ((down & KEY_A) != 0u && n != 0u && rooms[cursor].phase == NDS_NET_PHASE_CSS)
            {
                const int r = ndsNetJoinRequest(&rooms[cursor]);

                if (r == 0)
                {
                    joining = 1u;
                    status = "Joining...";
                }
                else if (r == -3)
                {
                    /* Plan 7.1: every player runs an identical build. */
                    status = "That room runs another build.";
                }
            }
            if ((down & KEY_B) != 0u)
            {
                ndsNetSessionClose();
                ndsNetUiWaitRelease();
                ndsNetUiClear();
                return 0;
            }
        }

        if ((frame++ & 7u) == 0u)
        {
            ndsNetUiLine(2, " JOIN A ROOM");
            for (i = 0u; i < NDS_NET_MAX_ROOMS; i++)
            {
                if (i < n)
                    ndsNetUiLine(5 + i * 2, " %c %-10s  %u/4 %s", (i == cursor) ? '>' : ' ',
                                 rooms[i].name, (unsigned)rooms[i].humans,
                                 (rooms[i].same_build == 0u) ? "(other build)" :
                                 (rooms[i].phase == NDS_NET_PHASE_CSS) ? "" : "(playing)");
                else
                    ndsNetUiLine(5 + i * 2, "");
            }
            ndsNetUiLine(14, (n == 0u && joining == 0u) ? " Searching for rooms..." : status);
            ndsNetUiLine(22, " A  Join      B  Back");
        }
    }
}

void ndsNetUiLobbyStatus(void)
{
    const u32 role = ndsNetRole();
    const u32 mask = ndsNetLobbyHumanMask();
    const u32 phase = ndsNetLobbyPhase();
    const u32 key = (role << 16) | (mask << 8) | (phase << 4) | ndsNetLobbyLocalPort();
    u32 p;

    if (role == NDS_NET_ROLE_NONE || key == sNdsNetUiLobbyKey)
        return;
    sNdsNetUiLobbyKey = key;
    ndsNetUiLine(14, (role == NDS_NET_ROLE_HOST) ? " WIRELESS ROOM (you host)" :
                                                    " WIRELESS ROOM");
    for (p = 0u; p < 4u; p++)
    {
        if (((mask >> p) & 1u) != 0u)
            ndsNetUiLine(16 + p, "  P%u %-10s%s", (unsigned)(p + 1u), ndsNetLobbyMemberName(p),
                         (p == ndsNetLobbyLocalPort()) ? "  (you)" : "");
        else
            ndsNetUiLine(16 + p, "  P%u -", (unsigned)(p + 1u));
    }
    if (role == NDS_NET_ROLE_HOST)
        ndsNetUiLine(21, (phase == NDS_NET_PHASE_STAGE) ? " Choose the stage." :
                                                          " START when everyone is ready.");
    else
        ndsNetUiLine(21, (phase == NDS_NET_PHASE_STAGE) ? " The host is choosing a stage." :
                                                          " Pick your fighter.");
    ndsNetUiLine(22, " Hold B to leave the room");
}

void ndsNetUiLobbyReset(void)
{
    ndsNetUiClear();
}
