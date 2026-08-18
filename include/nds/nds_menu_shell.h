#ifndef NDS_MENU_SHELL_H
#define NDS_MENU_SHELL_H

#include <PR/ultratypes.h>

/* P2-1d -- the VS shell's real screens: splash, title, main menu, VS menu and
 * its rules. `docs/p2/P2-1-vs-shell.md` work item 4.
 *
 * WHAT THIS IS. Four native DS screens drawn out of the P2-1c UI kit, each
 * running as a REAL SCENE: `scManagerRunLoop` dispatches to a `<X>StartScene`,
 * `syTaskmanStartTask` rewinds the per-scene arena (and the scene manager
 * brackets that entry), and the screen owns its own frame loop until it
 * requests the next scene through `ndsSceneManagerRequest`. That replaces the
 * bounded park branches P2-1b inherited, whose whole behaviour was
 * `osStopThread(NULL)`.
 *
 * BEHAVIOUR IS THE SOURCE'S, PRESENTATION IS NOT. Cursor order, wrap
 * direction, the value ranges, which button does what, which cue plays where,
 * and which scene each entry leads to are all transcribed from
 * `mn/mncommon/mnmodeselect.c` and `mn/mnvsmode/mnvsmode.c`. The DRAWING is a
 * recognizable approximation per `PROJECT_GOAL.md`: the source composes N64
 * sprite objects (icon sheets, tab-button nine-slices, a 300x220 artwork
 * collage) through the RDP, and this composes the source's own menu font and
 * digit sprites onto the DS OBJ layer instead. The cascade layout, the entry
 * order and the source's own colours are kept; the artwork is not, and the
 * open background question is a board decision, not a choice made here.
 *
 * WHY THE SCREENS DRIVE THEIR OWN FRAME LOOP rather than running as GObjs
 * under `gcRunAll`/`gcDrawAll`: those need the RSP/RDP display-list backend
 * this target does not have, which is exactly why the branches being replaced
 * were bounded in the first place. A native loop reads input, updates its own
 * state, and calls `ndsPlatformEndFrame` -- the same present the battle uses.
 *
 * MODE 163 IS UNTOUCHED. Every entry point here is compiled out at
 * `NDS_P2_MENU_SHELL == 0`, which is every published and Boundary
 * configuration, and the boot scene only becomes the splash when the flag is
 * on. The battle scene, its harness seeding and its transitions are not
 * modified by this row at all. */

/* Which screen a published per-screen figure belongs to. */
#define NDS_MENU_SHELL_SCREEN_SPLASH 0u
#define NDS_MENU_SHELL_SCREEN_TITLE 1u
#define NDS_MENU_SHELL_SCREEN_MODE 2u
#define NDS_MENU_SHELL_SCREEN_VSMODE 3u
#define NDS_MENU_SHELL_SCREEN_COUNT 4u

/* Per-screen work histogram: sixteen buckets of 35,012 ARM9 ticks, one
 * sixteenth of the 560,190-tick 60 Hz VBlank budget, so a bucket index is
 * 6.25% of budget and a percentile read off it carries a stated resolution.
 * Bucket b is [b*35012, (b+1)*35012). */
#define NDS_MENU_SHELL_TICK_BUCKET 35012u
#define NDS_MENU_SHELL_TICK_BUCKETS 16u
/* VBlank interval between presents: index i counts presents whose interval was
 * i+1, so index 0 is a clean 60 Hz present and anything else is a slip. */
#define NDS_MENU_SHELL_VBLANK_BUCKETS 4u
/* Transition/input evidence ring. Eight menu transitions is two full loops. */
#define NDS_MENU_SHELL_RING 16u

/* Screen entry points, called from the bounded scene branches in
 * src/port/taskman_seam.c. Each returns with the next scene already requested
 * through the scene manager. */
void ndsMenuShellRunSplash(void);
void ndsMenuShellRunTitle(void);
void ndsMenuShellRunModeSelect(void);
void ndsMenuShellRunVSMode(void);

/* --- Published state. Read by scripts/menus/probe-p2-1d-menus.ps1; none of it
 * is read by gameplay. --- */

/* The screen currently running, or 0xffffffff between scenes. */
extern volatile u32 gNdsMenuShellScreen;
/* Entries and exits per screen. Entry k and entry k+1 of the same screen must
 * agree on every other per-screen figure, or the screen is not re-entrant. */
extern volatile u32 gNdsMenuShellEnterCount[NDS_MENU_SHELL_SCREEN_COUNT];
extern volatile u32 gNdsMenuShellExitCount[NDS_MENU_SHELL_SCREEN_COUNT];
/* Presented frames measured per screen, and the ARM9 work each one cost. The
 * window for any percentile is gNdsMenuShellFrames of that screen. */
extern volatile u32 gNdsMenuShellFrames[NDS_MENU_SHELL_SCREEN_COUNT];
extern volatile u32
    gNdsMenuShellWorkHist[NDS_MENU_SHELL_SCREEN_COUNT]
                         [NDS_MENU_SHELL_TICK_BUCKETS];
extern volatile u32 gNdsMenuShellWorkMax[NDS_MENU_SHELL_SCREEN_COUNT];
extern volatile u32
    gNdsMenuShellVBlankHist[NDS_MENU_SHELL_SCREEN_COUNT]
                           [NDS_MENU_SHELL_VBLANK_BUCKETS];
extern volatile u32 gNdsMenuShellVBlankMax[NDS_MENU_SHELL_SCREEN_COUNT];
/* The scene-entry frame, reported on its own: it carries the NitroFS pack read
 * and the source scene's own file loads, which are load-time work and must not
 * be the max of a steady-state distribution. */
extern volatile u32 gNdsMenuShellEnterTicks[NDS_MENU_SHELL_SCREEN_COUNT];

/* ((from_screen << 8) | to_scene_kind) per menu transition this shell made. */
extern volatile u32 gNdsMenuShellTransitionRing[NDS_MENU_SHELL_RING];
extern volatile u32 gNdsMenuShellTransitionCount;
/* Input taps the screens acted on, ((screen << 16) | tap_mask), so a walk's
 * evidence pairs an INPUT with the transition it produced rather than
 * asserting that one caused the other. */
extern volatile u32 gNdsMenuShellInputRing[NDS_MENU_SHELL_RING];
extern volatile u32 gNdsMenuShellInputCount;
/* Refusals: a greyed main-menu entry or the not-yet-built VS OPTIONS button.
 * Non-zero is the proof the greyed rows are inert rather than absent. */
extern volatile u32 gNdsMenuShellDeniedCount;
/* The rules the VS screen last committed into the match descriptor. */
extern volatile u32 gNdsMenuShellCommitCount;
extern volatile u32 gNdsMenuShellCommitRule;
extern volatile u32 gNdsMenuShellCommitTime;
extern volatile u32 gNdsMenuShellCommitStocks;
/* Scripted-walk state (NDS_P2_MENU_WALK). Steps injected and loops closed. */
extern volatile u32 gNdsMenuShellWalkSteps;
extern volatile u32 gNdsMenuShellWalkLoops;

#endif /* NDS_MENU_SHELL_H */
