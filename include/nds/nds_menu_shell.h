#ifndef NDS_MENU_SHELL_H
#define NDS_MENU_SHELL_H

#include <PR/ultratypes.h>

/* Keep this leaf header free of the BattleShip scene/fighter include graph.
 * `nds_platform.c` includes libnds first, whose ARM9 linked-list header reaches
 * the decomp sys/malloc.h; pulling our mirrored scene.h in here then reaches the
 * port sys/malloc.h and defines SYMallocRegion twice. The CSS's source bound is
 * four and nds_menu_shell.c statically checks this against GMCOMMON_PLAYERS_MAX. */
#define NDS_MENU_SHELL_PLAYERS 4u
/* Same leaf-header rule for the source's twelve playable fighter kinds. The
 * implementation statically checks this against nFTKindPlayableEnd once the
 * fighter headers are available. */
#define NDS_MENU_SHELL_FIGHTER_KINDS 12u

/* P2-1d -- the VS shell's real screens: title, main menu, VS menu and its
 * rules, plus P2-1e/1f's character and stage selects.
 * `docs/p2/P2-1-vs-shell.md` work item 4.
 *
 * WHAT THIS IS. Native DS screens drawn out of the P2-1c UI kit, each
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
 * configuration, and the boot scene only reaches the title when the flag is
 * on. The battle scene, its harness seeding and its transitions are not
 * modified by this row at all. */

/* Which screen a published per-screen figure belongs to.
 *
 * P2-1h RENUMBERED THESE. Screen 0 used to be an invented "Smash64DS" splash
 * card; the owner's 2026-08-18 ruling deleted it -- this is a port, so the
 * original branding ships and nothing stands in for it -- and boot now reaches
 * the title with no screen in between, which is the N64 flow once the opening
 * cinematic (P2-7) is accounted for. Every index below moved down one. */
#define NDS_MENU_SHELL_SCREEN_TITLE 0u
#define NDS_MENU_SHELL_SCREEN_MODE 1u
#define NDS_MENU_SHELL_SCREEN_VSMODE 2u
/* P2-1e, the VS character select (mn/mnplayers/mnplayersvs.c). */
#define NDS_MENU_SHELL_SCREEN_CSS 3u
/* P2-1f, the VS stage select (mn/mnmaps/mnmaps.c). */
#define NDS_MENU_SHELL_SCREEN_SSS 4u
/* P2-5u1, the two screens behind the VS menu's OPTIONS row
 * (mn/mnvsmode/mnvsoptions.c and mnvsitemswitch.c). VSOPTIONS is the gateway
 * -- handicap, team attack, stage select, damage ratio -- and ITEMSWITCH is
 * the sixteen-row screen it opens onto. Appended rather than inserted: every
 * per-screen array below is indexed by these values and the shell-loop
 * verifier reads them positionally, so an insert would silently relabel every
 * existing screen's counters. */
#define NDS_MENU_SHELL_SCREEN_VSOPTIONS 5u
#define NDS_MENU_SHELL_SCREEN_ITEMSWITCH 6u
#define NDS_MENU_SHELL_SCREEN_COUNT 7u

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
/* The boot scene, and NOT a screen: it presents no frame and draws nothing.
 * It exists because the source's own startup scene still runs its func_start
 * (and makes GObjs), so its teardown still has work, and because it is the
 * shell's earliest entry -- which is where the menu audio pack has to be
 * loaded, or every menu cue before the first battle resolves against an empty
 * pack. It requests the title and returns. */
void ndsMenuShellRunStartup(void);
void ndsMenuShellRunTitle(void);
void ndsMenuShellRunModeSelect(void);
void ndsMenuShellRunVSMode(void);
void ndsMenuShellRunCharSelect(void);
void ndsMenuShellRunStageSelect(void);
/* P2-5u1, the Item Switch screen behind the VS menu's OPTIONS row. */
void ndsMenuShellRunItemSwitch(void);

/* The shell's 2D CSS owns source fighter previews through a deliberately
 * bounded PlayersVS subset; these are implemented by the imported source TU.
 * P2-2 restores the source's full four-slot preview capacity while keeping the
 * available character set bounded to the Mario/Fox assets this build ships. */
void ndsMNPlayersVSPreviewInit(void);
void ndsMNPlayersVSPreviewSyncRules(sb32 is_team_battle, const u8 *teams,
                                    u32 team_count);
void ndsMNPlayersVSPreviewSync(u32 slot, s32 pkind, s32 fkind,
                               sb32 is_selected);
s32 ndsMNPlayersVSPreviewCycleCostume(u32 slot);
u32 ndsMNPlayersVSPreviewGetAppearance(u32 slot);
void ndsMNPlayersVSPreviewFrame(void);
void ndsMNPlayersVSPreviewExit(void);

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
/* P2-1g: the worst frame's LABEL. `Frame` is which presented frame of that
 * screen it was; `Cues` is how many FGM play calls that same frame made. A
 * maximum with no label is what left the P2-1e/1f one-frame outlier
 * unattributed and on the board as a suspicion. */
extern volatile u32 gNdsMenuShellWorkMaxFrame[NDS_MENU_SHELL_SCREEN_COUNT];
extern volatile u32 gNdsMenuShellWorkMaxCues[NDS_MENU_SHELL_SCREEN_COUNT];
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
/* P2-1i. The 1-based presented title frame the fire was first shown on.
 * `mnTitleMakeFire` shows it during scene construction on our branch
 * (mntitle.c:990-993 -- the HIDDEN flag two lines above is the opening-movie
 * branch only), so a correct run reads 1 and 0 means it never fired. The
 * platform's own enable/frame/disable counters live beside the fire helpers
 * in nds_platform.c. */
extern volatile u32 gNdsTitleFireRevealFrame;
/* P2-1h. How many times the frameless boot scene ran and handed straight to
 * the title. Exactly 1 per run, and the audio-pack load rides on it: a run
 * that shows 0 here has no menu SFX, whatever the miss ring says. */
extern volatile u32 gNdsMenuShellStartupCount;
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
extern volatile u32 gNdsMenuShellWalkDwellSteps;
extern volatile u32 gNdsMenuShellWalkLoops;
/* P2-1g. Laps the walk will drive, seeded from NDS_P2_MENU_WALK and writable,
 * so one linked ROM covers a three-lap smoke and a twenty-lap phase-close run.
 * Poke it before the first lap closes; the walk parks when Loops reaches it. */
extern volatile u32 gNdsMenuShellWalkBudget;
/* P2-1g. The Results START the walk synthesises: presses made (rising edges)
 * and frames spent on the Results screen this entry. Paired with
 * gNdsVSResultsRematchCount these split "the walk never pressed" from "the
 * source's own exit test refused the press". */
extern volatile u32 gNdsMenuShellWalkResultsPressCount;
extern volatile u32 gNdsMenuShellWalkResultsHoldFrames;
/* Returns 1 while the walk wants START held on the Results screen. Defined
 * only under NDS_P2_MENU_WALK and called only from ndsPlatformReadInput under
 * the same guard -- the DS key constant stays on the platform side, the scene
 * test stays in the shell. Declared unconditionally: a declaration nothing
 * calls costs nothing, and a #if here would depend on every includer having
 * pulled nds_build_config.h first. */
u32 ndsMenuShellWalkWantsResultsStart(void);

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
extern volatile u32 gNdsMenuShellCssCostumeCycleCount;
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
extern volatile u32 gNdsMenuShellCssModeToggleCount;
extern volatile u32 gNdsMenuShellCssDoorSlideFrames;
/* Live source-fighter preview proof. Updated after the source process step and
 * before its display callbacks each CSS frame; retained across CSS teardown so
 * the scene-entry probe can inspect the last real preview state safely. */
extern volatile u32 gNdsPlayersVSPreviewFrameCount;
extern volatile u32 gNdsPlayersVSPreviewDrawCount;
extern volatile f32 gNdsPlayersVSPreviewRotationY[NDS_MENU_SHELL_PLAYERS];
extern volatile s32 gNdsPlayersVSPreviewStatus[NDS_MENU_SHELL_PLAYERS];
extern volatile s32 gNdsPlayersVSPreviewMotion[NDS_MENU_SHELL_PLAYERS];
extern volatile u32
    gNdsPlayersVSPreviewFreeRotateFrames[NDS_MENU_SHELL_PLAYERS];
extern volatile f32
    gNdsPlayersVSPreviewLastFreeRotationY[NDS_MENU_SHELL_PLAYERS];
extern volatile s32
    gNdsPlayersVSPreviewLastFreeStatus[NDS_MENU_SHELL_PLAYERS];
extern volatile s32
    gNdsPlayersVSPreviewLastFreeMotion[NDS_MENU_SHELL_PLAYERS];
extern volatile u32 gNdsPlayersVSPreviewSelectedMask;
extern volatile u32 gNdsPlayersVSPreviewVisibleMask;
extern volatile u32 gNdsPlayersVSPreviewExitCount;
extern volatile u32 gNdsPlayersVSPreviewCostumeChangeCount;
extern volatile u32 gNdsPlayersVSPreviewSelectedKindMask;
extern volatile u32
    gNdsPlayersVSPreviewSelectedKindFrames[NDS_MENU_SHELL_FIGHTER_KINDS];
extern volatile s32
    gNdsPlayersVSPreviewSelectedKindStatus[NDS_MENU_SHELL_FIGHTER_KINDS];
extern volatile s32
    gNdsPlayersVSPreviewSelectedKindMotion[NDS_MENU_SHELL_FIGHTER_KINDS];

/* --- P2-1j/P2-1N, state-dependent menu surfaces. -------------------------
 *
 * The VS menu buttons and CSS gates are BG2 surfaces rather than oversized OBJ
 * cells. P2-1N also added source-ordered CSS overlays/animations (team labels,
 * READY and portrait flashes), so the CSS counter names every non-backdrop CSS
 * surface blit across BG2 and BG3. These counters are read against the global
 * UI-kit surface count by the loop verifier; no unowned blit is allowed. */
extern volatile u32 gNdsMenuShellVsButtonBlitCount;
extern volatile u32 gNdsMenuShellCssPanelBlitCount;
extern volatile u32 gNdsMenuShellSssPlaqueBlitCount;

/* --- P2-1f, the stage select. Same rule: none of it is read by gameplay. --- */

/* The cursor's slot, 0..9, in mnMapsGetGroundKind's own numbering (mnmaps.c
 * :453), and the ground kind that slot names -- 0xde for RANDOM, exactly as
 * the source spells it. */
extern volatile u32 gNdsMenuShellSssCursorSlot;
extern volatile u32 gNdsMenuShellSssCursorGkind;
/* The stage an opt-in build is for, so the scripted walk confirms on THAT
 * stage rather than on whichever slot a fixed cursor path happens to reach.
 * Returns Dream Land when no stage flag is set. */
/* Stage-select grid width, so the walk can tell the two rows apart. */
#define NDS_SSS_WALK_ROW 5u
/* "no explicit target": the walk picks the first stage the build has. */
#define NDS_SSS_WALK_TARGET_AUTO 0xffu
/* 0xff means "whichever stage this build has"; the harness pokes a gkind
 * to steer the walk at one stage in particular on an all-stages ROM. */
extern volatile u32 gNdsMenuShellSssWalkTargetGkind;
u32 ndsMenuShellSssWalkTargetGkind(void);
u32 ndsMenuShellSssWalkTargetSlot(void);
/* TRUE when this build has a ground other than Dream Land, so the walk's
 * stage-select cursor can be somewhere the fixed script would not fix. */
u32 ndsMenuShellSssHasNonDefaultGround(void);
u32 ndsMenuShellSssWalkCursorSlot(void);
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
