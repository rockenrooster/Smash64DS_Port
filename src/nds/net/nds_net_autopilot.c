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
extern volatile u32 gNdsNetBatch;
extern volatile u32 gNdsNetLabMuteUntil;
extern u32 ndsPlatformVBlankCount(void);

#if NDS_NET_LAB_SWEEP
#define NDS_NET_AP_MATCHES 10u
#else
#define NDS_NET_AP_MATCHES 3u
#endif
/* When the fighter picks start on the first visit to the character select:
 * after the Team Battle walk when there is one. */
#if NDS_NET_LAB_TEAMS
#define NDS_NET_AP_PICK_AT 200u
#else
#define NDS_NET_AP_PICK_AT 0u
#endif

volatile u32 gNdsNetAutopilotRole;     /* 1 host, 2 guest */
volatile u32 gNdsNetAutopilotMatches;
volatile u32 gNdsNetAutopilotStep;

static u32 sApRole;
static u32 sApScreen = 0xFFFFFFFFu;
static u32 sApScene = 0xFFFFFFFFu;
static u32 sApTics;
static u32 sApDropped;

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
    }
    sApTics++;

    if (scene == (u32)nSCKindVSBattle)
    {
#if NDS_NET_LAB_DROP
        /* The guest's radio drops out for 15 s in the middle of the first
         * match: both consoles must end it as NO CONTEST and meet again in
         * the lobby for the second. */
        if (sApRole == 2u && sApDropped == 0u && gNdsNetAutopilotMatches == 1u &&
            gNdsNetBatch >= 300u)
        {
            sApDropped = 1u;
            gNdsNetLabMuteUntil = ndsPlatformVBlankCount() + 900u;
        }
#endif
        return 0u; /* the lockstep scripts its own pad (NDS_NET_LAB_INPUT) */
    }
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
        /* VS START's wireless value: one step right is HOST, one step left
         * (it wraps) JOIN; A confirms, and JOIN then finds the room itself.
         * The guest waits a little longer so the host's room is up. */
        if (sApRole == 1u)
        {
            if (ndsNetApWindow(40u, 1u))
                keys = KEY_RIGHT;
            if (ndsNetApWindow(80u, 4u))
                keys = KEY_A;
        }
        else
        {
            if (ndsNetApWindow(40u, 1u))
                keys = KEY_LEFT;
            if (ndsNetApWindow(120u, 4u))
                keys = KEY_A;
        }
        break;
    case NDS_MENU_SHELL_SCREEN_CSS:
        gNdsNetAutopilotStep = 4u;
        /* Pick once: after a match the lobby reopens on the same picks. */
        if (gNdsNetAutopilotMatches == 0u)
        {
#if NDS_NET_LAB_TEAMS
            /* Team Battle through the real screen before anyone picks. The
             * cursor moves 4 px a frame and hits with its (+20, +3) hotspot:
             * from its seat (40, 170) the host walks 36 frames up onto the
             * mode label (27..137, 14..35) and presses A (mnplayersvs.c:3356);
             * the guest, from (108, 170), one step left and 10 up onto its
             * own team button (103..127, 131..141, :1985) and presses A; both
             * walk back to their seats. */
            if (sApRole == 1u)
            {
                if (ndsNetApWindow(10u, 36u))
                    keys |= KEY_UP;
                if (ndsNetApWindow(50u, 4u))
                    keys |= KEY_A;
                if (ndsNetApWindow(60u, 36u))
                    keys |= KEY_DOWN;
            }
            else
            {
                if (ndsNetApWindow(120u, 1u))
                    keys |= KEY_LEFT;
                if (ndsNetApWindow(121u, 10u))
                    keys |= KEY_UP;
                if (ndsNetApWindow(140u, 4u))
                    keys |= KEY_A;
                if (ndsNetApWindow(150u, 10u))
                    keys |= KEY_DOWN;
                if (ndsNetApWindow(165u, 1u))
                    keys |= KEY_RIGHT;
            }
#endif
            if (ndsNetApWindow(NDS_NET_AP_PICK_AT + 30u, 12u))
                keys |= (sApRole == 1u) ? KEY_RIGHT : 0u;
            if (ndsNetApWindow(NDS_NET_AP_PICK_AT + 45u, 25u))
                keys |= KEY_UP;
            if (ndsNetApWindow(NDS_NET_AP_PICK_AT + 90u, 4u))
                keys |= KEY_A;
#if NDS_NET_LAB_SWEEP
            /* The guest walks back down to its own preview (the cursor's
             * seat is inside it, mnplayersvs.c:4604) and asks for the next
             * costume once: the host resolves it and both show it. */
            if (sApRole == 2u)
            {
                if (ndsNetApWindow(NDS_NET_AP_PICK_AT + 100u, 25u))
                    keys |= KEY_DOWN;
                if (ndsNetApWindow(NDS_NET_AP_PICK_AT + 128u, 4u))
                    keys |= KEY_A;
            }
#endif
        }

        /* The host asks to start every half second once everyone has picked;
         * the screen refuses until then. */
        if (sApRole == 1u && sApTics > NDS_NET_AP_PICK_AT + 300u &&
            (sApTics % 30u) < 4u && gNdsNetAutopilotMatches < NDS_NET_AP_MATCHES)
            keys |= KEY_START;
        break;
    case NDS_MENU_SHELL_SCREEN_SSS:
        gNdsNetAutopilotStep = 5u;
        /* NDS_NET_LAB_SWEEP: the stage comes from the host's descriptor
         * (ndsNetLabSweepDescriptor), whatever is confirmed here. */
        if (sApRole == 1u)
        {
            if (ndsNetApWindow(60u, 4u))
                keys = KEY_A;
        }
        break;
    default:
        break;
    }
    return keys;
}

#endif
