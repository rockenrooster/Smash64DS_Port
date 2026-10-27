/* P3 lab autopilot (NDS_NET_LAB_LOBBY): drives the real menus on two
 * emulated consoles through a whole wireless session -- VS Mode, host or
 * join, pick a fighter in the shared character select, the host's stage pick,
 * the match, Results, and a rematch. The console whose firmware user name is
 * "melonDS2" (the harness's second instance) joins; any other hosts (with
 * NDS_NET_LAB_SOAK the two swap every room). Keys are ORed into the keypad by
 * ndsPlatformReadInput, so every screen reads them as a player's presses. */
#include <string.h>
#include <nds/bios.h>
#include <nds/input.h>
#include <nds/interrupts.h>

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
#define NDS_NET_AP_PICK_AT 0u

volatile u32 gNdsNetAutopilotRole;     /* 1 host, 2 guest */
volatile u32 gNdsNetAutopilotMatches;
volatile u32 gNdsNetAutopilotStep;
/* The matches the host starts in a room; a probe may raise it at boot. */
volatile u32 gNdsNetLabMatchCap __attribute__((used)) = NDS_NET_AP_MATCHES;
/* Nonzero (a probe pokes it at boot): after the first pick each console asks
 * for its own next costume once, as the sweep's guest does, so the rematches
 * show which costumes carry over. */
volatile u32 gNdsNetLabCostumePress __attribute__((used));
/* Nonzero (poked at boot): every stage select, the host first walks its cursor
 * over every cell -- up to the top row, then each row left to right, one tap
 * every NDS_NET_AP_SSS_STEP tics so each 3D preview has time to load -- and
 * confirms after, so a probe sees every stage's preview on both consoles. */
volatile u32 gNdsNetLabSssTour __attribute__((used));
/* Nonzero (poked at boot): on its first select visit the host presses A at
 * tics 160 and 220 -- a probe puts the cursor on the mode label first, so the
 * select goes to Team Battle and back to free-for-all. */
volatile u32 gNdsNetLabModeToggles __attribute__((used));
#define NDS_NET_AP_SSS_STEP 80u
static const u16 kNdsNetApSssTour[] = {
    KEY_UP, KEY_UP,
    KEY_RIGHT, KEY_RIGHT, KEY_RIGHT, KEY_RIGHT, KEY_RIGHT, KEY_DOWN,
    KEY_RIGHT, KEY_RIGHT, KEY_RIGHT, KEY_RIGHT, KEY_RIGHT, KEY_DOWN,
    KEY_RIGHT, KEY_RIGHT, KEY_RIGHT, KEY_RIGHT, KEY_RIGHT,
};

static u32 sApRole;
static u32 sApScreen = 0xFFFFFFFFu;
static u32 sApScene = 0xFFFFFFFFu;
static u32 sApTics;
static u32 sApDropped;
#if NDS_NET_LAB_DROP > 1
/* The batch the last dropout started at, in this match (all ones: none). */
static u32 sApDropBatch = 0xFFFFFFFFu;
volatile u32 gNdsNetLabDrops;
/* The dropout's length in VBlanks: NDS_NET_LAB_DROP, a probe may set it
 * before the first match. */
volatile u32 gNdsNetLabDropVBlanks __attribute__((used)) = NDS_NET_LAB_DROP;
/* 0: the radio goes deaf (a dropout). 1: the game thread stops for the same
 * VBlanks with the radio untouched, so nothing drains its receive ring (a
 * freeze, the hardware reports of 2026-10-07/12). */
volatile u32 gNdsNetLabDropMode __attribute__((used));
#endif
/* Matches since this console last entered VS Mode (picks happen once a room). */
static u32 sApSessionMatches;


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
        {
            gNdsNetAutopilotMatches++;
            sApSessionMatches++;
#if NDS_NET_LAB_DROP > 1
            sApDropBatch = 0xFFFFFFFFu;
#endif
        }
        else if (scene != (u32)nSCKindVSBattle && scene != (u32)nSCKindVSResults &&
                 screen == NDS_MENU_SHELL_SCREEN_VSMODE && screen != sApScreen)
        {
            sApSessionMatches = 0u;
        }
        sApScene = scene;
        sApScreen = screen;
        sApTics = 0u;
    }
    sApTics++;

    if (scene == (u32)nSCKindVSBattle)
    {
#if NDS_NET_LAB_DROP == 1
        /* The guest's radio drops out for 40 s in the middle of the first
         * match, past the 30 s stall bound: both consoles must end it as NO
         * CONTEST and meet again in the lobby for the second. */
        if (sApRole == 2u && sApDropped == 0u && gNdsNetAutopilotMatches == 1u &&
            gNdsNetBatch >= 300u)
        {
            sApDropped = 1u;
            gNdsNetLabMuteUntil = ndsPlatformVBlankCount() + (60u * 40u);
        }
#elif NDS_NET_LAB_DROP > 1
        /* NDS_NET_LAB_DROP=N: dropouts the lockstep must ride out (owner,
         * 2026-10-18: recovering from a freeze or lag is unreliable). Every
         * match, every 1,200 batches from batch 300 the guest's radio goes
         * deaf for N VBlanks (gNdsNetLabDropVBlanks), and 600 batches after
         * each the host's: under the 10 s abort, each match must resume with
         * 0 desyncs. */
        {
            const u32 first = (sApRole == 2u) ? 300u : 900u;

            if (gNdsNetBatch >= first && ((gNdsNetBatch - first) % 1200u) == 0u &&
                gNdsNetBatch != sApDropBatch)
            {
                sApDropBatch = gNdsNetBatch;
                gNdsNetLabDrops++;
                if (gNdsNetLabDropMode == 1u)
                {
                    const u32 until = ndsPlatformVBlankCount() + gNdsNetLabDropVBlanks;

                    while ((s32)(until - ndsPlatformVBlankCount()) > 0)
                        swiWaitForVBlank();
                }
                else
                {
                    gNdsNetLabMuteUntil = ndsPlatformVBlankCount() + gNdsNetLabDropVBlanks;
                }
            }
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
        if (sApSessionMatches == 0u)
        {
            if (ndsNetApWindow(NDS_NET_AP_PICK_AT + 30u, 12u))
                keys |= (sApRole == 1u) ? KEY_RIGHT : 0u;
            if (ndsNetApWindow(NDS_NET_AP_PICK_AT + 45u, 25u))
                keys |= KEY_UP;
            if (ndsNetApWindow(NDS_NET_AP_PICK_AT + 90u, 4u))
                keys |= KEY_A;
            /* The sweep's guest (or, with gNdsNetLabCostumePress, each
             * console) walks back down to its own preview (the cursor's seat
             * is inside it, mnplayersvs.c:4604; the host first undoes its
             * step right) and asks for the next costume once: the host
             * resolves it and both show it. */
            if ((NDS_NET_LAB_SWEEP && (sApRole == 2u)) ||
                (gNdsNetLabCostumePress != 0u))
            {
                if ((sApRole == 1u) &&
                    ndsNetApWindow(NDS_NET_AP_PICK_AT + 100u, 12u))
                    keys |= KEY_LEFT;
                if (ndsNetApWindow(NDS_NET_AP_PICK_AT + 100u, 25u))
                    keys |= KEY_DOWN;
                if (ndsNetApWindow(NDS_NET_AP_PICK_AT + 128u, 4u))
                    keys |= KEY_A;
            }
            if ((sApRole == 1u) && (gNdsNetLabModeToggles != 0u) &&
                (ndsNetApWindow(NDS_NET_AP_PICK_AT + 160u, 4u) ||
                 ndsNetApWindow(NDS_NET_AP_PICK_AT + 220u, 4u)))
                keys |= KEY_A;
        }

        /* The host asks to start every half second once everyone has picked;
         * the screen refuses until then. */
        if (sApRole == 1u && sApTics > NDS_NET_AP_PICK_AT + 300u &&
            (sApTics % 30u) < 4u && gNdsNetAutopilotMatches < gNdsNetLabMatchCap)
            keys |= KEY_START;
        break;
    case NDS_MENU_SHELL_SCREEN_SSS:
        gNdsNetAutopilotStep = 5u;
        /* NDS_NET_LAB_SWEEP: the stage comes from the host's descriptor
         * (ndsNetLabSweepDescriptor), whatever is confirmed here. */
        if ((sApRole == 1u) && (gNdsNetLabSssTour != 0u))
        {
            const u32 steps = sizeof(kNdsNetApSssTour) / sizeof(kNdsNetApSssTour[0]);
            const u32 step = sApTics / NDS_NET_AP_SSS_STEP;

            if ((step < steps) && ((sApTics % NDS_NET_AP_SSS_STEP) == 30u))
                keys = kNdsNetApSssTour[step];
            if (ndsNetApWindow((steps * NDS_NET_AP_SSS_STEP) + 30u, 4u))
                keys = KEY_A;
            break;
        }
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
