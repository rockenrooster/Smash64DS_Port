/* P3 lab autopilot (NDS_NET_LAB_LOBBY): drives the real menus on two
 * emulated consoles through a whole wireless session -- VS Mode, host or
 * join, pick a fighter in the shared character select, the host's stage pick,
 * the match, Results, and a rematch. The console whose firmware user name is
 * "melonDS2" (the harness's second instance) joins; any other hosts. Keys are
 * ORed into the keypad by ndsPlatformReadInput, so every screen reads them as
 * a player's presses. */
#include <string.h>
#include <nds/input.h>

#include <PR/os.h>
#include <ssb_types.h>
#include <sc/scene.h>

#include <nds/nds_menu_shell.h>
#include <nds/nds_net_lobby.h>

#if NDS_NET_LAB_LOBBY

extern void ndsNetLinkGetUserName(char out[11]);

volatile u32 gNdsNetAutopilotRole;     /* 1 host, 2 guest */
volatile u32 gNdsNetAutopilotMatches;
volatile u32 gNdsNetAutopilotStep;

static u32 sApRole;
static u32 sApScreen = 0xFFFFFFFFu;
static u32 sApScene = 0xFFFFFFFFu;
static u32 sApTics;
static u32 sApJoinTaps;

static u32 ndsNetApWindow(u32 start, u32 len)
{
    return (sApTics >= start && sApTics < start + len) ? 1u : 0u;
}

u32 ndsNetLabAutopilotKeys(void)
{
    const u32 scene = (u32)gSCManagerSceneData.scene_curr;
    const u32 screen = gNdsMenuShellScreen;
    u32 keys = 0u;

    if (sApRole == 0u)
    {
        char name[11];

        ndsNetLinkGetUserName(name);
        /* The harness names instance n "melonDSn" (n >= 2): those join. */
        sApRole = (strncmp(name, "melonDS", 7) == 0 && name[7] >= '2' &&
                   name[7] <= '9') ? 2u : 1u;
        gNdsNetAutopilotRole = sApRole;
    }
    if (scene != sApScene || (scene != (u32)nSCKindVSBattle &&
                              scene != (u32)nSCKindVSResults && screen != sApScreen))
    {
        if (scene == (u32)nSCKindVSBattle && sApScene != (u32)nSCKindVSBattle)
            gNdsNetAutopilotMatches++;
        sApScene = scene;
        sApScreen = screen;
        sApTics = 0u;
        sApJoinTaps = 0u;
    }
    sApTics++;

    if (scene == (u32)nSCKindVSBattle)
        return 0u; /* the lockstep scripts its own pad (NDS_NET_LAB_INPUT) */
    if (scene == (u32)nSCKindVSResults)
        return (sApTics > 240u && (sApTics % 60u) < 4u) ? KEY_START : 0u;

    switch (screen)
    {
    case NDS_MENU_SHELL_SCREEN_TITLE:
        if (ndsNetApWindow(90u, 4u))
            keys = KEY_START;
        gNdsNetAutopilotStep = 1u;
        break;
    case NDS_MENU_SHELL_SCREEN_MODE:
        if (ndsNetApWindow(40u, 4u))
            keys = KEY_DOWN;
        if (ndsNetApWindow(80u, 4u))
            keys = KEY_A;
        gNdsNetAutopilotStep = 2u;
        break;
    case NDS_MENU_SHELL_SCREEN_VSMODE:
        gNdsNetAutopilotStep = 3u;
        if (sApRole == 1u)
        {
            if (ndsNetApWindow(40u, 4u))
                keys = KEY_X;
            if (ndsNetApWindow(80u, 4u))
                keys = KEY_A;
        }
        else
        {
            /* Y opens the room list (a modal inside this screen); A joins once
             * a room shows up. */
            if (ndsNetApWindow(60u, 4u))
                keys = KEY_Y;
            if (sApTics > 120u && (sApTics % 40u) < 4u && sApJoinTaps < 400u)
            {
                NdsNetRoom rooms[NDS_NET_MAX_ROOMS];

                if (ndsNetJoinRooms(rooms, NDS_NET_MAX_ROOMS) != 0u)
                {
                    keys = KEY_A;
                    sApJoinTaps++;
                }
            }
        }
        break;
    case NDS_MENU_SHELL_SCREEN_CSS:
        gNdsNetAutopilotStep = 4u;
        /* Pick once: after a match the lobby reopens on the same picks. */
        if (gNdsNetAutopilotMatches == 0u && ndsNetApWindow(30u, 12u))
            keys |= (sApRole == 1u) ? KEY_RIGHT : 0u;
        if (gNdsNetAutopilotMatches == 0u && ndsNetApWindow(45u, 25u))
            keys |= KEY_UP;
        if (gNdsNetAutopilotMatches == 0u && ndsNetApWindow(90u, 4u))
            keys |= KEY_A;
        /* The host asks to start every half second once it has picked; the
         * screen refuses until every player has. Two matches, then idle. */
        if (sApRole == 1u && sApTics > 150u && (sApTics % 30u) < 4u &&
            gNdsNetAutopilotMatches < 3u)
            keys |= KEY_START;
        break;
    case NDS_MENU_SHELL_SCREEN_SSS:
        gNdsNetAutopilotStep = 5u;
        if (sApRole == 1u && ndsNetApWindow(60u, 4u))
            keys = KEY_A;
        break;
    default:
        break;
    }
    return keys;
}

#endif
