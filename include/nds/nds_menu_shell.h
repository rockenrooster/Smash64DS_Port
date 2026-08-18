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
/* P2-1e, the VS character select (mn/mnplayers/mnplayersvs.c). */
#define NDS_MENU_SHELL_SCREEN_CSS 4u
/* P2-1f, the VS stage select (mn/mnmaps/mnmaps.c). */
#define NDS_MENU_SHELL_SCREEN_SSS 5u
#define NDS_MENU_SHELL_SCREEN_COUNT 6u

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
void ndsMenuShellRunCharSelect(void);
void ndsMenuShellRunStageSelect(void);

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

/* --- P2-1e, the character select. Everything below is the CSS's own seam
 * state; none of it is read by gameplay. --- */

/* Cursor position in the SOURCE's own 320x240 frame -- the frame every hit
 * test below is written in -- so a probe reads the same numbers
 * mnplayersvs.c's constants are expressed in. */
extern volatile s32 gNdsMenuShellCssCursorX;
extern volatile s32 gNdsMenuShellCssCursorY;
/* nMNPlayersCursorStatus{Pointer,Grab,Hover} = 0/1/2. */
extern volatile u32 gNdsMenuShellCssCursorStatus;
/* Token picked up, token dropped onto a portrait, token refused (dropped where
 * no unlocked fighter is), and token recalled with B. */
extern volatile u32 gNdsMenuShellCssGrabCount;
extern volatile u32 gNdsMenuShellCssDropCount;
extern volatile u32 gNdsMenuShellCssDropRefuseCount;
extern volatile u32 gNdsMenuShellCssRecallCount;
/* HMN/CP/NA button presses, and CPU-level arrow presses that changed a value. */
extern volatile u32 gNdsMenuShellCssKindToggleCount;
extern volatile u32 gNdsMenuShellCssLevelChangeCount;
/* START while READY TO FIGHT is up, START while it is not (the source's own
 * MenuDenied refusal), and the frame count the accepted START waits out. */
extern volatile u32 gNdsMenuShellCssStartCount;
extern volatile u32 gNdsMenuShellCssStartDeniedCount;
/* Back to the VS menu -- by the BACK button or by holding B. */
extern volatile u32 gNdsMenuShellCssBackCount;
/* The descriptor the CSS committed, and what it put in it. One entry a slot,
 * ((fkind << 16) | (pkind << 8) | cpu level), so the probe can read the match
 * the player built without walking the battle state. */
extern volatile u32 gNdsMenuShellCssCommitCount;
extern volatile u32 gNdsMenuShellCssCommitSlot[4];
/* Cue requests this screen made, by the SOURCE's own FGM id, plus the last id.
 * Paired with the FGM miss ring this separates "the seam never asked" from
 * "the pack has no sample" -- the split P2-1c/P2-1c-1 established. */
extern volatile u32 gNdsMenuShellCssCueCount;
extern volatile u32 gNdsMenuShellCssCueLastId;
extern volatile u32 gNdsMenuShellCssAnnounceCount;

/* --- P2-1f, the stage select. Same rule: none of it is read by gameplay. --- */

/* The cursor's slot, 0..9, in mnMapsGetGroundKind's own numbering (mnmaps.c
 * :453), and the ground kind that slot names -- 0xde for RANDOM, exactly as
 * the source spells it. */
extern volatile u32 gNdsMenuShellSssCursorSlot;
extern volatile u32 gNdsMenuShellSssCursorGkind;
/* Cursor moves that CHANGED the slot, and direction presses the lock table
 * refused. Non-zero `blocked` is the proof the locked cells are inert rather
 * than absent -- the same role gNdsMenuShellDeniedCount plays on the main
 * menu. */
extern volatile u32 gNdsMenuShellSssMoveCount;
extern volatile u32 gNdsMenuShellSssBlockedCount;
/* A/START accepted, and B taken back to the character select. */
extern volatile u32 gNdsMenuShellSssConfirmCount;
extern volatile u32 gNdsMenuShellSssBackCount;
/* THE WRITE PATH, which is what makes the stage claim a measurement and not an
 * assertion. `Commit` counts mnMapsSaveSceneData-equivalents; `Gkind` is the
 * ground it resolved to; `SlotGkind` is what the CURSOR named, which differs
 * from `Gkind` on the random path and equals it on the direct one -- one pair,
 * two code paths, and a control that can fail. */
extern volatile u32 gNdsMenuShellSssCommitCount;
extern volatile u32 gNdsMenuShellSssCommitGkind;
extern volatile u32 gNdsMenuShellSssCommitSlotGkind;
/* Which arm of mnMapsSaveSceneData's random pick resolved: the source's own
 * do-while (`Random`), or the bounded fallback it needs while this build has
 * ONE unlocked ground and its no-repeat clause is unsatisfiable (`Fallback`).
 * Published because "the loop terminated" is otherwise an assertion. */
extern volatile u32 gNdsMenuShellSssRandomCount;
extern volatile u32 gNdsMenuShellSssRandomFallbackCount;
/* Cue requests this screen made, by the source's own FGM id. */
extern volatile u32 gNdsMenuShellSssCueCount;
extern volatile u32 gNdsMenuShellSssCueLastId;

#endif /* NDS_MENU_SHELL_H */
