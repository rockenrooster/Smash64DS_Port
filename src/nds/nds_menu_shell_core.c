
#if !NDS_P2_UI_KIT
#error "NDS_P2_MENU_SHELL draws with the P2-1c kit (NDS_P2_UI_KIT=1)"
#endif

#include <nds/arm9/video.h>
#include <nds/timers.h>

#include <string.h>

#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <mn/menu.h>
#include <sc/scene.h>
#include <sys/objman.h>
#include <sys/taskman.h>
#include <sys/video.h>

#include <nds/nds_audio_assets.h>
#include <nds/nds_audio_bgm.h>
#include <nds/nds_audio_fgm.h>
#include <nds/nds_match_config.h>
#include <nds/nds_menu_shell.h>
#include <nds/nds_platform.h>
#include <nds/nds_scene.h>
#include <nds/nds_scene_manager.h>
#include <nds/nds_ui_kit.h>

#include "generated/mn_ui_kit.generated.inc"

_Static_assert(NDS_MENU_SHELL_PLAYERS == GMCOMMON_PLAYERS_MAX,
               "native CSS player bound must match BattleShip");
_Static_assert(NDS_MN_UI_KIT_SURFACE_COUNT <= 65536u,
               "menu surface ids must fit NdsUiKitSurfaceId");

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);
/* sys/utils.c's own time-seeded pick, which mnMapsSaveSceneData uses for the
 * RANDOM cell (mnmaps.c:1379). It reads osGetTime() and does NOT touch
 * sSYUtilsRandomSeed, so calling it from a menu cannot perturb the gameplay
 * RNG. Declared rather than included: include/sys/ carries no utils.h. */
extern s32 syUtilsRandTimeUCharRange(s32 range);
/* decomp sys/utils.c gameplay pick, used by the source title's own demo
 * fighter shuffle (mntitle.c:316/:334). Same local-extern pattern as the
 * import TUs (e.g. battleship_item_link_core.c:203). */
extern s32 syUtilsRandIntRange(s32 range);

/* --- Layout ---------------------------------------------------------------
 *
 * The source lays these screens out in a 320x240 frame inside a (10,10)-(310,
 * 230) viewport; the DS gives 256x192 and the kit draws the source's own 8-row
 * font at 1:1, so scaling the source's coordinates by 0.8 would crowd glyphs
 * that were never scaled with them. What is preserved instead is the SHAPE the
 * player recognises: both menus are a cascade descending to the LEFT, in the
 * source's own entry order, with the source's own colours.
 *
 * Mode select, source: labels at (224,52) (183,89) (142,126) (102,163),
 * step (-41,+37) -- mnmodeselect.c:459.
 * VS mode, source: buttons at (120,31) (97,70) (74,109) (51,148),
 * step (-23,+39) -- mnvsmode.c:319/:601/:687/:733. */
#define NDS_MENU_MODE_X0 150
#define NDS_MENU_MODE_Y0 44
#define NDS_MENU_MODE_DX (-30)
#define NDS_MENU_MODE_DY 30

#define NDS_MENU_VS_X0 96
#define NDS_MENU_VS_Y0 40
#define NDS_MENU_VS_DX (-18)
#define NDS_MENU_VS_DY 31

#define NDS_MENU_HEADER_X 14
#define NDS_MENU_HEADER_Y 12

/* The cursor's source cell is 27x36 inside a 32x64 OBJ cell; it points right,
 * so it sits left of the row it selects and rides one row-height above the
 * text baseline the way the CSS hand does. The offset clears the WHOLE cell,
 * not the drawn hand: OAM ids draw low-first and the cursor owns the lowest
 * sprite id, so a cell that reaches the row hides whatever the row draws
 * there -- which is what hid the digit of "1P GAME" until a capture showed it.
 * The mode-select row also has a digit sprite in front of its text, so its
 * cursor clears eleven more pixels than the VS screen's. */
#define NDS_MENU_CURSOR_DX (-34)
#define NDS_MENU_CURSOR_DX_DIGIT (-46)
#define NDS_MENU_CURSOR_DY (-14)

/* The source's own menu colours, read off the sprite primitive colours it
 * sets: mnmodeselect.c:517 draws the MODE SELECT decal in 0x3C73B4 over a
 * 0x083365 bar, its labels in pure red, and greys an unselected icon to 0x96
 * (mnmodeselect.c:219). */
#define NDS_MENU_RGB_WHITE 0x00ffffffu
#define NDS_MENU_RGB_RED 0x00ff0000u
#define NDS_MENU_RGB_GREY 0x00969696u
#define NDS_MENU_RGB_LOCKED 0x00606060u
#define NDS_MENU_RGB_HEADER 0x003c73b4u
#define NDS_MENU_RGB_VALUE 0x00ffd23cu

/* The DS backdrop, which is main BG palette entry 0: the only pixel source on
 * this engine that costs no VRAM at all. Both main BG banks are full (C and D
 * are one 256x256 Bmp16 each, exactly 128 KiB, docs/p2/P2-1c-vram-map.md), so
 * the source's 300x220 artwork collage has nowhere to live and the flat field
 * behind it -- the deep blue the source's own decal bar uses -- is what this
 * row draws. The collage itself is a board decision with a price, not a choice
 * made here. Restored to black on exit so no later scene inherits it. */
#define NDS_MENU_BACKDROP_BLUE RGB15(1, 6, 12)
#define NDS_MENU_BACKDROP_BLACK RGB15(0, 0, 0)

/* Text slots. Six is the kit's budget and every screen below fits it. */
#define NDS_MENU_SLOT_HEADER 0u
#define NDS_MENU_SLOT_ROW0 1u
#define NDS_MENU_SLOT_ROW1 2u
#define NDS_MENU_SLOT_ROW2 3u
#define NDS_MENU_SLOT_ROW3 4u
#define NDS_MENU_SLOT_EXTRA 5u

/* Sprite slots. */
#define NDS_MENU_SPRITE_CURSOR 0u
#define NDS_MENU_SPRITE_NUM0 1u
#define NDS_MENU_SPRITE_NUM2 3u /* one past the last number slot */
/* P2-1j -- the VS rules screen's own blinking value arrows (mnvsmode.c:2570).
 * They take the slots the invented hand cursor used to reach for, which is
 * what makes owner finding (b) a straight replacement rather than a growth. */
#define NDS_MENU_SPRITE_ARROW_L 4u
#define NDS_MENU_SPRITE_ARROW_R 5u

/* --gc-sections drops a global whose only reader is a probe script; that has
 * reddened Boundary once already on "Missing ELF symbol". */
#define NDS_MENU_PUBLISHED __attribute__((used))

NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellScreen = 0xffffffffu;
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellEnterCount[NDS_MENU_SHELL_SCREEN_COUNT];
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellExitCount[NDS_MENU_SHELL_SCREEN_COUNT];
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellFrames[NDS_MENU_SHELL_SCREEN_COUNT];
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellWorkHist[NDS_MENU_SHELL_SCREEN_COUNT]
                         [NDS_MENU_SHELL_TICK_BUCKETS];
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellWorkMax[NDS_MENU_SHELL_SCREEN_COUNT];
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellVBlankHist[NDS_MENU_SHELL_SCREEN_COUNT]
                           [NDS_MENU_SHELL_VBLANK_BUCKETS];
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellVBlankMax[NDS_MENU_SHELL_SCREEN_COUNT];
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellWorkMaxFrame[NDS_MENU_SHELL_SCREEN_COUNT];
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellWorkMaxCues[NDS_MENU_SHELL_SCREEN_COUNT];
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellEnterTicks[NDS_MENU_SHELL_SCREEN_COUNT];
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellTransitionRing[NDS_MENU_SHELL_RING];
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellTransitionCount;
/* P2-1i. The 1-based presented title frame the fire was first shown on. The
 * source shows it during scene construction on our branch, so this reads 1;
 * anything else means the fire missed frames the source had it burning on. */
NDS_MENU_PUBLISHED volatile u32 gNdsTitleFireRevealFrame;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellStartupCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellInputRing[NDS_MENU_SHELL_RING];
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellInputCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellDeniedCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCommitCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCommitRule;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCommitTime;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCommitStocks;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellWalkSteps;
/* P2-3. A walk step whose button is 0 is a DWELL -- it gives the screen time
 * without pressing anything, so it posts no input-ring entry by construction.
 * The Luigi leg of the character-select script has one (the source's selected-
 * fighter spin needs longer than the 30-tic regrab delay), and it only compiles
 * when the in-progress roster is built, which is why the ring invariant read
 * exactly until now. Count them so the invariant stays EXACT -- entries plus
 * dwells equals steps -- instead of being relaxed to an inequality. */
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellWalkDwellSteps;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellWalkLoops;
/* P2-1g. The budget is a VARIABLE seeded from the compile-time count, not the
 * macro itself, so one linked walk ROM serves a three-lap smoke and a
 * twenty-lap phase-close run. Two lap counts used to mean two builds, and this
 * repository's own measurement law prefers one dual-route binary over two
 * linked ROMs for exactly this reason. Poke it before the first lap closes. */
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellWalkBudget =
#if NDS_P2_MENU_WALK
    (u32)NDS_P2_MENU_WALK;
#else
    0u;
#endif
/* Results START taps the walk synthesised, and the frames it held the button.
 * Non-zero `Press` with a flat gNdsVSResultsRematchCount is "the tap reached
 * the pad and the source's exit test refused it"; both zero is "the walk never
 * pressed". They are the two halves that separate an input failure from a
 * scene-routing failure without guessing. */
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellWalkResultsPressCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellWalkResultsHoldFrames;
/* P2-1j/P2-1N. Every re-blit of a STATE-dependent menu element, counted where
 * it is spent. `csspanel` includes both BG2 gate/READY/flash states and the
 * source-ordered BG3 READY/team overlays: they are all CSS surfaces, and the
 * loop verifier's exact surface arithmetic needs every one named. A steady
 * screen still holds these flat except for source-owned animations (READY's
 * 40/30 blink and portrait flash's per-tic visibility toggle). */
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellVsButtonBlitCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssPanelBlitCount;
/* P2-1k (c). The stage select's per-stage plaque is the THIRD source of a
 * non-entry surface blit, after the VS menu's buttons and the character
 * select's gates. It gets its own counter for the same reason those two do:
 * the loop verifier asserts `blit` equals one backdrop per screen entry PLUS
 * every state blit, and a state blit with no counter behind it makes that
 * assertion fail on arithmetic rather than on a defect. */
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssPlaqueBlitCount;
NDS_MENU_PUBLISHED volatile s32 gNdsMenuShellCssCursorX;
NDS_MENU_PUBLISHED volatile s32 gNdsMenuShellCssCursorY;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssCursorStatus;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssGrabCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssDropCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssDropRefuseCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssRecallCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssKindToggleCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssCostumeCycleCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssLevelChangeCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssStartCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssStartDeniedCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssBackCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssCommitCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssCommitSlot[4];
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssCueCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssCueLastId;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssAnnounceCount;
/* P2-1N (4): mode-label toggle engagements — the walk and probes read it. */
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssModeToggleCount;
/* P2-1N (3): frames a slot's doors spent mid-slide -- engagement proof. */
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCssDoorSlideFrames;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssCursorSlot;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssCursorGkind;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssMoveCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssBlockedCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssConfirmCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssBackCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssCommitCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssCommitGkind;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssCommitSlotGkind;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssRandomCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssRandomFallbackCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssCueCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellSssCueLastId;

/* --- Per-screen frame accounting ----------------------------------------- */

static u32 sMenuScreen;
static u32 sMenuFrameStartTicks;
static u32 sMenuLastVBlank;
static u32 sMenuFrameArmed;
static u32 sMenuHeldPrev;
static u32 sMenuTics;
static u32 sMenuNextScene;
static u32 sMenuLeaving;
static u32 sMenuFrameFgmAtStart;
/* P2-1h. Which half of the title's blink cycle the backdrop currently shows,
 * so the surface is toggled on the EDGE and a still title costs nothing. */
static u32 sMenuTitleBlinkPhase;
#if NDS_P2_1P_GAME
/* P2-7 item 6. Attract idle counter, in the source counter's domain (seed
 * 169, trigger 650/1190; mntitle.c:356/:712-723). Defined here, ahead of
 * ndsMenuShellPopulateTitle's per-entry seed, because this shell compiles
 * core and router as one TU (nds_menu_shell.c). */
static u32 sMenuTitleIdleTics;
#endif

static void ndsMenuShellRecordFrame(void)
{
    /* WORK PER PRESENTED FRAME: from the instant the previous present
     * returned to the instant this frame asks for the next one -- i.e. the
     * ARM9 the screen spent, with the VBlank wait excluded. The first frame of
     * a scene is deliberately NOT in this distribution; it is the scene-load
     * frame and is reported on its own as gNdsMenuShellEnterTicks. */
    u32 now = cpuGetTiming();
    u32 work = now - sMenuFrameStartTicks;
    u32 bucket = work / NDS_MENU_SHELL_TICK_BUCKET;

    if (sMenuFrameArmed == 0u)
    {
        return;
    }
    if (bucket >= NDS_MENU_SHELL_TICK_BUCKETS)
    {
        bucket = NDS_MENU_SHELL_TICK_BUCKETS - 1u;
    }
    gNdsMenuShellWorkHist[sMenuScreen][bucket]++;
    if (work > gNdsMenuShellWorkMax[sMenuScreen])
    {
        gNdsMenuShellWorkMax[sMenuScreen] = work;
        /* P2-1g. THE WORST FRAME NOW CARRIES ITS OWN LABEL, because a maximum
         * with no label is what left P2-1e and P2-1f with an unattributed
         * one-frame outlier on three screens and a suspicion on the board
         * instead of a cause. `Frame` is which presented frame it was, and
         * `Cues` is how many FGM play calls that same frame made -- so the
         * question "is the worst frame the frame that started a sound?" is a
         * comparison in the artifact rather than a theory. A max whose Cues is
         * 0 refutes the audio explanation outright. */
        gNdsMenuShellWorkMaxFrame[sMenuScreen] =
            gNdsMenuShellFrames[sMenuScreen];
        gNdsMenuShellWorkMaxCues[sMenuScreen] =
            gNdsAudioFgmPlayCalls - sMenuFrameFgmAtStart;
    }
    gNdsMenuShellFrames[sMenuScreen]++;
}

static void ndsMenuShellRecordPresent(void)
{
    u32 vblank = ndsPlatformVBlankCount();

    if (sMenuFrameArmed != 0u)
    {
        u32 interval = vblank - sMenuLastVBlank;
        u32 bucket = (interval == 0u) ? 0u : (interval - 1u);

        if (bucket >= NDS_MENU_SHELL_VBLANK_BUCKETS)
        {
            bucket = NDS_MENU_SHELL_VBLANK_BUCKETS - 1u;
        }
        gNdsMenuShellVBlankHist[sMenuScreen][bucket]++;
        if (interval > gNdsMenuShellVBlankMax[sMenuScreen])
        {
            gNdsMenuShellVBlankMax[sMenuScreen] = interval;
        }
    }
    sMenuLastVBlank = vblank;
    sMenuFrameStartTicks = cpuGetTiming();
    /* Snapshot beside the tick start, so the pair describes the SAME frame. */
    sMenuFrameFgmAtStart = gNdsAudioFgmPlayCalls;
    /* Armed one present AFTER entry so the scene-load frame -- the NitroFS
     * pack read and the source scene's own file loads -- is reported alone. */
    sMenuFrameArmed = 1u;
}

/* --- Input ---------------------------------------------------------------
 *
 * The source's repeat, transcribed: a held direction acts when its wait is 0
 * and then reloads the wait to 12 (mndef.h:4/:17, the `is_button` arm); the
 * wait decrements once per frame and is forced to 0 the moment no direction is
 * held (mnmodeselect.c:717, mnvsmode.c:1307). So a tap acts at once and a hold
 * repeats every twelve frames, which is the original's feel. The DS has no
 * analog stick, so only the button arm exists here and the stick divisors the
 * source carries have no expression -- stated rather than silently dropped. */
#define NDS_MENU_REPEAT_WAIT 12

static u32 sMenuChangeWait;

/* Stage-select entries so far. Declared here rather than beside the rest of
 * the stage select's state because the scripted walk below picks its script
 * from it, and the walk is compiled before that section. */
static u32 sSssEnterCount;

#if NDS_P2_MENU_WALK
/* Scripted input, lab only. It drives the SCREENS' OWN handlers, not the scene
 * manager: every step below is a button the player could press, so a walk that
 * reaches the battle is proof the handlers reached it, not proof that a hop
 * counter did. `gNdsMenuShellInputRing` pairs each acted-on tap with the
 * screen that consumed it.
 *
 * A STEP CARRIES A HOLD LENGTH (P2-1e). The row screens only ever needed taps,
 * because one tap moves one row; the character select is a POINTER screen whose
 * cursor moves 4 px a frame while a direction is HELD, so reaching a portrait
 * takes tens of frames of the same input. `hold` is that count in frames, and
 * it is 1 for every step the row screens use, so their behaviour and their
 * banked figures are unchanged. */
#define NDS_MENU_WALK_DWELL 150u
/* The CSS script is twenty-four steps (fifteen before the P2-1M clamp
 * normalization) and its holds spend 193 frames, so it takes the shorter gap
 * between steps; the row screens keep theirs. */
#define NDS_MENU_WALK_DWELL_CSS 40u

typedef struct NdsMenuWalkStep {
    u16 button;
    u16 hold;
} NdsMenuWalkStep;

/* ONE SCRIPT PER SCREEN, reset on that screen's entry. A single flat script
 * would only work on the first pass: the loop re-enters at VS Mode from
 * Results, not at the title, so a flat cursor would sit on a title step that
 * never comes up again and the walk would stall on its second lap.
 *
 * The VS script is a full tour of the screen -- every cursor row, the rule
 * value both ways, the time value both ways, every cursor row -- and
 * ends on VS START. Its value moves are deliberately NET ZERO, so the match
 * every lap enters is still the canonical one-minute Time match and the laps
 * stay comparable. */
static const NdsMenuWalkStep kNdsMenuWalkTitle[] = {
    { (u16)NDS_INPUT_START, 1u }
};
static const NdsMenuWalkStep kNdsMenuWalkMode[] = {
    { (u16)NDS_INPUT_DOWN, 1u }, { (u16)NDS_INPUT_A, 1u }
};
static const NdsMenuWalkStep kNdsMenuWalkVs[] = {
    { (u16)NDS_INPUT_DOWN, 1u },  /* cursor: VS START -> RULE          */
    { (u16)NDS_INPUT_RIGHT, 1u }, /* rule: TIME -> STOCK               */
    { (u16)NDS_INPUT_LEFT, 1u },  /* rule: STOCK -> TIME (net zero)    */
    { (u16)NDS_INPUT_DOWN, 1u },  /* cursor: RULE -> TIME/STOCK value  */
    { (u16)NDS_INPUT_RIGHT, 1u }, /* value: 1 -> 2                     */
    { (u16)NDS_INPUT_LEFT, 1u },  /* value: 2 -> 1 (net zero)          */
    { (u16)NDS_INPUT_DOWN, 1u },  /* cursor: value -> VS OPTIONS       */
    /* The A press that used to sit here proved a REFUSAL, and its own comment
     * said why: "VS OPTIONS is not built". It is built now, so that press
     * leaves this screen -- and the header above states that each script is
     * reset on its screen's entry, so coming back replays this tour from the
     * top, presses A again, and the walk ping-pongs between the VS menu and
     * VS Options forever. That is exactly how the 2026-09-04 lap hung: three
     * scene entries, then no further input, then the capture ceiling.
     *
     * The row is still visited by the DOWN above, so the tour still covers
     * every cursor position. Entering the option screens belongs to a test
     * that asserts the committed descriptor, not to the lap, whose contract
     * is a fixed scene pattern. */
    { (u16)NDS_INPUT_UP, 1u }, { (u16)NDS_INPUT_UP, 1u },
    { (u16)NDS_INPUT_UP, 1u },
    { (u16)NDS_INPUT_A, 1u }      /* VS START -> the character select  */
};

/* THE CHARACTER-SELECT TOUR. Every position below is worked in the SOURCE's
 * 320x240 frame at 4 px a held frame from the cursor's own seat (40,170), and
 * every landing spot is one of mnplayersvs.c's own rectangles:
 *
 *   (40,170) -up 30-> y=50  -right 9-> x=76   cursor hotspot (101,53) is inside
 *                                             Mario's token box (81..107,46..70)
 *   A                                         GRAB
 *   -right 10-> x=116  token centre (140,48)  column 2 = Donkey = LOCKED
 *   A                                         REFUSED, token stays in hand
 *   -left 10-> x=76    token centre (100,48)  column 1 = Mario
 *   A                                         DROP + announce
 *   -down 19-> y=126   -right 43-> x=248      slot 4's HMN/CP/NA box
 *                                             (x+20 in 267..295, y+3 in 127..145)
 *   A, A                                      NA -> CP -> NA, net zero
 *   -down 17-> y=194   -left 44-> x=72        slot 2's CP-LEVEL LEFT arrow
 *                                             (x+20 in 90..112, y+3 in 197..216)
 *   A x8                                      clamp Fox's level to the 1 floor
 *                                             (source clamps 1..9), whatever it
 *                                             was on entry
 *   -right 12-> x=120                         the RIGHT arrow (x+20 in 137..159)
 *   A                                         1 -> 2
 *   START                                     READY TO FIGHT -> the match
 *
 * The two kind-button presses are deliberately ADJACENT and net zero: the first
 * makes slot 4 a CPU with a random fighter and the second empties it again, so
 * the match this walk enters is still the two-fighter one the battle supports.
 * The CPU-level tour is CLAMP-NORMALIZED, on purpose (P2-1M gate catch,
 * 2026-08-19): a bare +1 drifted with the entry level -- the loop arm's laps
 * re-enter the CSS, so the committed level climbed a lap at a time until the
 * source's 9 clamp (LOOPCFG read s1=1/1/9; the one-pass arm read 5 after the
 * round-4 arrow re-geometry). Eight decrements floor the level at 1 from ANY
 * start, one increment commits exactly 2 -- deterministic, lap-stable, and
 *   still distinct from the preset's 3, so the battle state reading 2 remains
 *   the proof that the DESCRIPTOR, not the preset, decides the match.
 *
 *   ROSTER-INDEPENDENT COMMIT (2026-09-04 rung-7 catch): the cursor moves in
 *   PIXELS, so the travel distances above never change -- but A over a portrait
 *   drops the token when that fighter is admitted and is REFUSED when it is
 *   locked, so admitting Link/Pikachu/Yoshi turned the tour's refusals into
 *   real pickups and the walk committed Samus/Fox. The portrait legs above are
 *   kept as the exercise (every grab/drop/refuse/announce counter still fires),
 *   but the commit no longer depends on what they picked up: the CSS START arm
 *   runs a walk-only snapshot (ndsMenuShellCssWalkRestoreGate, walk builds
 *   only, Link-proof and argmax tours excluded) that writes slot 0 = Mario
 *   selected and slot 1 = Fox selected by DIRECT ASSIGNMENT before the ready
 *   test. Direct assignment is roster-independent because Mario and Fox are in
 *   every NDS_CSS_FIGHTER_MASK arm and admission only adds bits; it writes no
 *   level, so the clamp tour above still commits exactly 2 from any entry. */
#if NDS_P2_LINK && (NDS_P2_PROOF_FIGHTER0 == 5)
/* P2-3f31. The proof descriptor enters CSS with Link already selected in slot
 * 0. That is the state the real shell must preserve into battle: moving the
 * token through the historical Mario/Fox tour would overwrite the descriptor
 * and cease to be a Link admission proof. Dwell past the source's 60-tic START
 * arm and the entrance shutters, then commit the live Link selection through
 * the screen's ordinary START handler. */
static const NdsMenuWalkStep kNdsMenuWalkCss[] = {
    { 0u, 90u },
    { (u16)NDS_INPUT_START, 1u }
};
#elif NDS_P2_SHELL_ARGMAX_ROSTER
/* P2-3f9 -- THE ARGMAX TOUR IS THE ONE THAT TOUCHES NOTHING.
 *
 * The descriptor already carries the four heaviest landed kinds
 * (nds_match_config.c), so mnPlayersVSInitPlayer's port -- ndsMenuShellCssInit
 * -- opens this screen with all four slots occupied and selected, every gate
 * open and every live preview built. The tour above would DESTROY that: its
 * first A lands on slot 0's token and its two slot-4 kind presses cycle an
 * occupied CP slot to NA and back, which re-rolls that slot's fighter through
 * mnPlayersVSRandFighterKind and makes the roster nondeterministic.
 *
 * So this arm waits and presses START. The wait is not padding: the source
 * refuses START until sMenuTics > NDS_CSS_START_ARM_TICS (60), and the entry
 * shutters slide 2 px a tic from door_offset 41. One dwell (40) plus a 90-frame
 * hold clears both with margin, on every lap. */
static const NdsMenuWalkStep kNdsMenuWalkCss[] = {
    { 0u, 90u },
    { (u16)NDS_INPUT_START, 1u }
};
#else
static const NdsMenuWalkStep kNdsMenuWalkCss[] = {
    { (u16)NDS_INPUT_UP, 30u },
    { (u16)NDS_INPUT_RIGHT, 9u },
    { (u16)NDS_INPUT_A, 1u },
#if NDS_P2_LUIGI
    /* P2-3 production proof. Luigi is portrait column 0 in the source table.
     * Move the held 1P token from Mario to Luigi, drop it, and give the source
     * selected-fighter process more than the 30-tic regrab delay to complete
     * its +20 degree/tic spin into nFTDemoStatusWin1. Then grab Luigi again,
     * continue to the same locked Donkey negative control, and return the token
     * to Mario before the canonical match starts. Thus the Luigi lab exercises
     * portrait/announce/flash/live-3D selected state without changing the
     * battle that the standing P2 shell probe measures. */
    { (u16)NDS_INPUT_LEFT, 11u },
    { (u16)NDS_INPUT_A, 1u },
    { 0u, 32u },
    { (u16)NDS_INPUT_A, 1u },
#if NDS_P2_SAMUS
    /* P2-3 Samus production proof. BattleShip's portrait table is
     * Luigi/Mario/Donkey/Link/Samus/Captain on the top row
     * (mnplayersvs.c:338-346), with 45 source pixels between cells. After the
     * Luigi regrab above the carried token centre is x=56. The DS D-pad is the
     * source's full-deflection stick at 4 source px/frame (see THE STICK
     * contract above), so 43 RIGHT frames put that centre at x=228 -- the exact
     * centre of Samus's portrait-4 [205,250) cell. Drop, leave more than the
     * source's 30-tic regrab delay so the selected 3D process/announcer run,
     * regrab, then 11 LEFT frames return to Link's portrait-3 cell for the same
     * locked negative control the DK/Falcon roster already uses. The later
     * 20-frame LEFT leg still returns the token to Mario before the canonical
     * Mario/Fox regression match starts. */
    { (u16)NDS_INPUT_RIGHT, 43u },
    { (u16)NDS_INPUT_A, 1u },
    { 0u, 32u },
    { (u16)NDS_INPUT_A, 1u },
    { (u16)NDS_INPUT_LEFT, 11u },
#elif NDS_P2_DONKEY
    /* THE NEGATIVE CONTROL HAS TO MOVE WHEN THE ROSTER GROWS. Donkey is a
     * built fighter in this configuration, so pressing A on column 2 now
     * DROPS the token there and the match that follows is not the one the
     * gate measures. Link, column 3, is the nearest cell that is still
     * locked, so the refusal this leg proves stays a refusal. Column pitch is
     * 40 px at 4 px a held frame, hence ten frames per column. */
    { (u16)NDS_INPUT_RIGHT, 31u },
#else
    { (u16)NDS_INPUT_RIGHT, 21u },
#endif
#else
    { (u16)NDS_INPUT_RIGHT, 10u },
#endif
    { (u16)NDS_INPUT_A, 1u },
#if NDS_P2_DONKEY
    { (u16)NDS_INPUT_LEFT, 20u },
#else
    { (u16)NDS_INPUT_LEFT, 10u },
#endif
    { (u16)NDS_INPUT_A, 1u },
    { (u16)NDS_INPUT_DOWN, 19u },
    { (u16)NDS_INPUT_RIGHT, 43u },
    { (u16)NDS_INPUT_A, 1u },
    { (u16)NDS_INPUT_A, 1u },
    { (u16)NDS_INPUT_DOWN, 17u },
    { (u16)NDS_INPUT_LEFT, 44u },
    { (u16)NDS_INPUT_A, 1u }, { (u16)NDS_INPUT_A, 1u },
    { (u16)NDS_INPUT_A, 1u }, { (u16)NDS_INPUT_A, 1u },
    { (u16)NDS_INPUT_A, 1u }, { (u16)NDS_INPUT_A, 1u },
    { (u16)NDS_INPUT_A, 1u }, { (u16)NDS_INPUT_A, 1u },
    { (u16)NDS_INPUT_RIGHT, 12u },
    { (u16)NDS_INPUT_A, 1u },
    { (u16)NDS_INPUT_START, 1u }
};
#endif /* NDS_P2_SHELL_ARGMAX_ROSTER */

/* THE STAGE-SELECT TOUR (P2-1f), and it is TWO scripts because this screen has
 * two exits and both are the row's deliverable. The cursor opens on the cell
 * `gSCManagerSceneData.maps_vsmode_gkind` names, so where each visit STARTS is
 * decided by what the visit before it chose -- the source's own rule
 * (mnMapsInitVars) -- and that is what makes one pair of scripts cover both
 * confirm paths without a lap counter:
 *
 *   visit 1  opens on slot 6 (Dream Land, the harness seed)
 *            RIGHT  7 and 8 are locked and are SKIPPED -> slot 9, RANDOM
 *            UP     slot 4 is locked: REFUSED, and the counter says so
 *            B      BACK to the character select -- which still commits
 *                   (mnmaps.c:1481), so maps_vsmode_gkind becomes 0xde
 *   visit 2  opens on slot 9 (RANDOM, restored)
 *            RIGHT  wraps to 5, locked, SKIPPED -> slot 6, Dream Land
 *            UP     slot 1 is locked: REFUSED
 *            A      confirm on a GROUND: the direct write path
 *   visit 3  opens on slot 6 again, so RIGHT lands on RANDOM and A confirms
 *            THERE: the random write path, resolving to the same ground by
 *            different code. Visit 4 is visit 2 again, and so on.
 *
 * So a one-pass run proves the back-out and the direct path, and any run of
 * two or more laps proves the random path as well -- with the alternation
 * falling out of the SOURCE's cursor-restore rule rather than out of a lap
 * test written here. */
static const NdsMenuWalkStep kNdsMenuWalkSssBack[] = {
    { (u16)NDS_INPUT_RIGHT, 1u }, { (u16)NDS_INPUT_UP, 1u },
    { (u16)NDS_INPUT_B, 1u }
};
static const NdsMenuWalkStep kNdsMenuWalkSss[] = {
    { (u16)NDS_INPUT_RIGHT, 1u }, { (u16)NDS_INPUT_UP, 1u },
    { (u16)NDS_INPUT_A, 1u }
};

/* P2-1h: the leading NULL/0 entry that used to be the splash's is gone with
 * it. These two tables are indexed by SCREEN, so they move with the screen
 * numbering; the walk's step count per lap is unchanged, because the splash
 * never had a scripted step. */
/* THE TWO OPTION SCREENS THE WALK CAN NOW REACH.
 *
 * Until 2026-09-04 the VS menu's OPTIONS row refused, so the walk could never
 * enter either screen and the two trailing entries of the tables below were
 * NULL with length 0 -- harmless, because nothing indexed them. Opening that
 * row made the walk drive itself into a screen it had no script for, where it
 * simply stopped driving: the lap hung with the guest in f_read, the screen's
 * per-frame surface sync still reading from the cartridge, and the verifier
 * timed out at its 3000 s ceiling having reached three scene entries.
 *
 * A dwell and B is deliberately the whole script. B is the source's own exit
 * from both screens (mnvsoptions.c:1294 returns to the VS menu,
 * mnvsitemswitch.c:696 returns to VS Options), so the lap closes the way it
 * did before while now proving both screens open, commit and leave. Editing
 * their rows is a separate concern from the lap, and belongs in a test that
 * asserts the committed descriptor rather than in the pattern the lap checks. */
static const NdsMenuWalkStep kNdsMenuWalkVsOptions[] = {
    { 0u, 30u }, { (u16)NDS_INPUT_B, 1u }
};
static const NdsMenuWalkStep kNdsMenuWalkItemSwitch[] = {
    { 0u, 30u }, { (u16)NDS_INPUT_B, 1u }
};

static const NdsMenuWalkStep *const
    kNdsMenuWalkScripts[NDS_MENU_SHELL_SCREEN_COUNT] = {
    kNdsMenuWalkTitle, kNdsMenuWalkMode, kNdsMenuWalkVs, kNdsMenuWalkCss,
    kNdsMenuWalkSss, kNdsMenuWalkVsOptions, kNdsMenuWalkItemSwitch
};
static const u8 kNdsMenuWalkLengths[NDS_MENU_SHELL_SCREEN_COUNT] = {
    (u8)(sizeof(kNdsMenuWalkTitle) / sizeof(kNdsMenuWalkTitle[0])),
    (u8)(sizeof(kNdsMenuWalkMode) / sizeof(kNdsMenuWalkMode[0])),
    (u8)(sizeof(kNdsMenuWalkVs) / sizeof(kNdsMenuWalkVs[0])),
    (u8)(sizeof(kNdsMenuWalkCss) / sizeof(kNdsMenuWalkCss[0])),
    (u8)(sizeof(kNdsMenuWalkSss) / sizeof(kNdsMenuWalkSss[0])),
    (u8)(sizeof(kNdsMenuWalkVsOptions) / sizeof(kNdsMenuWalkVsOptions[0])),
    (u8)(sizeof(kNdsMenuWalkItemSwitch) / sizeof(kNdsMenuWalkItemSwitch[0]))
};

static u32 sMenuWalkCursor;
static u32 sMenuWalkTimer;
static u32 sMenuWalkHold;
static u32 sMenuWalkHeld;

/* Returns the button to HOLD this frame, and writes the button to report as a
 * fresh TAP into *out_tap -- which is only the first frame of a step. Without
 * that split a 43-frame hold would post 43 entries into the input ring and the
 * ring would stop being a record of what the player pressed. */
static u32 ndsMenuShellWalkTap(u32 screen, u32 *out_tap)
{
    const NdsMenuWalkStep *script;
    u32 length;
    u32 dwell;

    *out_tap = 0u;
    if (gNdsMenuShellWalkLoops >= gNdsMenuShellWalkBudget)
    {
        /* Budget spent: stop driving and leave the shell to the player. The
         * screen parks rather than looping forever, which is what makes the
         * run end on its own instead of by timeout. */
        return 0u;
    }
    if (screen >= NDS_MENU_SHELL_SCREEN_COUNT)
    {
        return 0u;
    }
    /* A held step keeps injecting the SAME button until its frames run out.
     * The steps counter still counts one step, not one frame -- a hold is one
     * press a player makes, and counting frames would make the two arms of the
     * walk incomparable. */
    if (sMenuWalkHold != 0u)
    {
        sMenuWalkHold--;
        return sMenuWalkHeld;
    }
    length = (u32)kNdsMenuWalkLengths[screen];
    if ((length == 0u) || (sMenuWalkCursor >= length))
    {
        return 0u;
    }
    dwell = ((screen == NDS_MENU_SHELL_SCREEN_CSS) ||
             (screen == NDS_MENU_SHELL_SCREEN_SSS)) ?
        NDS_MENU_WALK_DWELL_CSS : NDS_MENU_WALK_DWELL;
    if (sMenuWalkTimer != 0u)
    {
        sMenuWalkTimer--;
        return 0u;
    }
    sMenuWalkTimer = dwell;
    gNdsMenuShellWalkSteps++;
    sMenuWalkCursor++;
    /* P2-1f MOVED THE LAP COUNT OFF THIS FUNCTION. A lap ends where the pass
     * reaches the match, and that is now the STAGE SELECT's own confirm --
     * counted there (ndsMenuShellUpdateSss) rather than at a script position,
     * because this screen runs two different scripts and only one of them
     * ends in the battle. */
    /* AN OPT-IN STAGE BUILD STEERS AT ITS OWN STAGE.
     *
     * The confirm script below is a fixed RIGHT, UP, A, and which slot that
     * reaches is a property of the grid rather than of what the build shipped.
     * Planet Zebes and Hyrule Castle both PASSED a full lap having played
     * Dream Land, because RIGHT then UP returns to Dream Land from where those
     * two sit -- a green result proving nothing about the stage under test.
     * When a stage flag is on, press RIGHT until the cursor is on that stage
     * and only then confirm. A build with no stage flag targets Dream Land,
     * which is where the cursor already is, so it takes the script below
     * unchanged and the Boundary arm's step counts do not move.
     *
     * The second clause used to read "the target is not Dream Land", which
     * skipped the seek whenever Dream Land was wanted -- correct only while
     * Dream Land was the sole cell, because then the fixed script could not
     * land anywhere else. When the published ROM gained all eight opt-in
     * stages on 2026-09-04 that escape started handing Dream Land runs to the
     * fixed script on a nine-cell grid, and RIGHT-UP-A reaches HYRULE from
     * there: the gate's battle arm played gkind 4 and failed on Dream Land's
     * geometry. So the question is not which stage is wanted, it is whether
     * this build has anywhere else the cursor could be -- if it does, seek,
     * including to Dream Land. A one-stage build still takes the script
     * below, byte for byte. */
    if ((screen == NDS_MENU_SHELL_SCREEN_SSS) && (sSssEnterCount != 1u) &&
        ((gNdsMenuShellSssWalkTargetGkind != NDS_SSS_WALK_TARGET_AUTO) ||
         (ndsMenuShellSssHasNonDefaultGround() != FALSE)))
    {
        u32 want = ndsMenuShellSssWalkTargetSlot();
        u32 have = ndsMenuShellSssWalkCursorSlot();
        u32 press;

        /* ROW FIRST, THEN COLUMN, because the two axes do not behave alike:
         * LEFT and RIGHT cycle within a row and skip locked cells, while UP
         * and DOWN refuse a locked destination outright. Crossing rows first
         * means the one press that can be refused is made while the column is
         * still wherever the cursor started, and stage acceptance runs on a
         * ROM with every stage unlocked precisely so that press always lands.
         * A stage sitting diagonally from the cursor on a one-stage build is
         * simply unreachable, which is how Planet Zebes and Hyrule Castle each
         * passed a lap having played Dream Land. */
        if ((want < NDS_SSS_WALK_ROW) != (have < NDS_SSS_WALK_ROW))
        {
            press = (want < NDS_SSS_WALK_ROW) ?
                (u32)NDS_INPUT_UP : (u32)NDS_INPUT_DOWN;
        }
        else if (want != have)
        {
            press = (u32)NDS_INPUT_RIGHT;
        }
        else
        {
            press = (u32)NDS_INPUT_A;
        }
        sMenuWalkCursor--;   /* seeking does not consume a script step */
        sMenuWalkHeld = press;
        sMenuWalkHold = 0u;
        *out_tap = sMenuWalkHeld;
        return sMenuWalkHeld;
    }
    script = kNdsMenuWalkScripts[screen];
    if ((screen == NDS_MENU_SHELL_SCREEN_SSS) && (sSssEnterCount == 1u))
    {
        /* The FIRST stage-select visit of the run takes the back-out script;
         * every later one takes the confirm script. Keyed on the screen's own
         * entry counter, not on a lap index, because the back-out is a
         * once-per-run proof and a lap index would take it every lap and never
         * reach the battle. */
        script = kNdsMenuWalkSssBack;
    }
    sMenuWalkHeld = (u32)script[sMenuWalkCursor - 1u].button;
    sMenuWalkHold = (u32)script[sMenuWalkCursor - 1u].hold - 1u;
    if (sMenuWalkHeld == 0u)
    {
        gNdsMenuShellWalkDwellSteps++;
    }
    *out_tap = sMenuWalkHeld;
    return sMenuWalkHeld;
}

/* P2-1g -- THE ONE BUTTON THIS SHELL CANNOT PRESS FOR ITSELF.
 *
 * Results is not a shell screen. It is the imported `mnVSResultsFuncRun`, and
 * it leaves only when `mnVSResultsCheckExit` (mnvsresults.c:266) sees
 * START in `gSCManagerDevices[i].button_tap` after `sMNVSResultsTotalTimeTics`
 * has reached `sMNVSResultsAllowExitWait` (410 ticks for a normal result,
 * :2820). Nothing in the shell's own input path can reach that test, so the
 * P2-1b scene walk substituted a scene-manager hop out of Results instead --
 * which closes a lap without ever running `ndsMNVSResultsSetLoadScene`'s body.
 * P2-1f rewrote that body for the shell and could not exercise it; this is
 * what exercises it.
 *
 * IT IS A KEYPAD PRESS, NOT A SHORTCUT. This returns "hold START this frame"
 * and `ndsPlatformReadInput` ORs the DS key in before it latches, so the tap
 * travels the whole real path -- `osContGetReadData` reads the latched keys,
 * `syControllerReadDeviceData`/`syControllerUpdateGlobalData` derive the
 * rising edge, and the source's own exit test samples it. A write straight
 * into `gSYControllerDevices` would prove nothing about that chain, and
 * `battleship_mnvsresults.c:376` records that it also cannot work: the port
 * runs the controller pipeline BEFORE `task_update`, so anything written
 * beside it lands after its only reader.
 *
 * THE PULSE, and why it is a pulse. `button_tap` is an EDGE. A permanently
 * held START produces exactly one edge -- on the frame Results is entered,
 * long before tic 410 -- and then nothing, which is the failure
 * `battleship_mnvsresults.c:357` already paid for once. So the button is held
 * for four frames in every sixteen: the first edge that lands after the wait
 * expires is at most sixteen frames late, and Results runs at least 600
 * updates before any bound in this build stops it. The frame counter is this
 * screen's own dwell, reset whenever Results is not the current scene, so the
 * phase is deterministic per entry rather than free-running across a run. */
u32 ndsMenuShellWalkWantsResultsStart(void)
{
    if ((u32)gSCManagerSceneData.scene_curr != (u32)nSCKindVSResults)
    {
        gNdsMenuShellWalkResultsHoldFrames = 0u;
        return 0u;
    }
    gNdsMenuShellWalkResultsHoldFrames++;
    if ((gNdsMenuShellWalkResultsHoldFrames & 15u) == 8u)
    {
        /* Count the RISING frame only -- one press, however long it is held. */
        gNdsMenuShellWalkResultsPressCount++;
    }
    return (((gNdsMenuShellWalkResultsHoldFrames & 15u) >= 8u) &&
            ((gNdsMenuShellWalkResultsHoldFrames & 15u) < 12u)) ? 1u : 0u;
}
#endif /* NDS_P2_MENU_WALK */

static u32 ndsMenuShellReadTaps(u32 *out_held)
{
    u32 held = ndsPlatformReadInput();
    u32 taps = held & ~sMenuHeldPrev;

    sMenuHeldPrev = held;
#if NDS_P2_MENU_WALK
    {
        u32 tap = 0u;
        u32 injected = ndsMenuShellWalkTap(sMenuScreen, &tap);

        taps |= tap;
        held |= injected;
    }
#endif
    *out_held = held;
    return taps;
}

/* One direction edge, with the source's repeat. `held` carries the direction
 * bits; the wait is shared across directions exactly as the source shares
 * sMNVSModeChangeWait. */
static u32 ndsMenuShellDirection(u32 held, u32 taps, u32 mask)
{
    if ((taps & mask) != 0u)
    {
        sMenuChangeWait = NDS_MENU_REPEAT_WAIT;
        return TRUE;
    }
    if (((held & mask) != 0u) && (sMenuChangeWait == 0u))
    {
        sMenuChangeWait = NDS_MENU_REPEAT_WAIT;
        return TRUE;
    }
    return FALSE;
}

static void ndsMenuShellTickInput(u32 held)
{
    if (sMenuChangeWait != 0u)
    {
        sMenuChangeWait--;
    }
    if ((held & (NDS_INPUT_UP | NDS_INPUT_DOWN | NDS_INPUT_LEFT |
                 NDS_INPUT_RIGHT)) == 0u)
    {
        sMenuChangeWait = 0u;
    }
}

static void ndsMenuShellRecordInput(u32 taps)
{
    u32 slot;

    if (taps == 0u)
    {
        return;
    }
    slot = gNdsMenuShellInputCount % NDS_MENU_SHELL_RING;
    gNdsMenuShellInputRing[slot] = ((sMenuScreen & 0xffu) << 16) |
                                   (taps & 0xffffu);
    gNdsMenuShellInputCount++;
}

/* --- Transition ---------------------------------------------------------- */

static void ndsMenuShellGoto(u32 next_kind)
{
    u32 slot = gNdsMenuShellTransitionCount % NDS_MENU_SHELL_RING;

    gNdsMenuShellTransitionRing[slot] =
        ((sMenuScreen & 0xffu) << 8) | (next_kind & 0xffu);
    gNdsMenuShellTransitionCount++;
    sMenuNextScene = next_kind;
    sMenuLeaving = TRUE;
}

/* --- Text helpers -------------------------------------------------------- */

/* P2-1j deleted the kerning-escape gap builder and its string appender with
 * the VS rules screen's font-composed rows: the only caller spelled
 * "TIME. <value> MIN" as one font string with the value's width in escapes,
 * and that whole row is now the source's own `llMNVSModeTimePeriodTextSprite`
 * and `...MinTextSprite` baked into the button plate they sit on. Nothing else
 * in this shell composes a label around a number sprite. */

static s32 ndsMenuShellCentre(const char *text)
{
    return (s32)((256u - ndsUiKitTextWidth(text)) / 2u);
}

static void ndsMenuShellHideRows(void)
{
    u32 slot;

    for (slot = 0u; slot < NDS_UI_KIT_TEXT_SLOTS; slot++)
    {
        ndsUiKitHideText(slot);
    }
    for (slot = 0u; slot < NDS_UI_KIT_SPRITE_SLOTS; slot++)
    {
        ndsUiKitHideSprite(slot);
    }
}

/* --- Screen: title -------------------------------------------------------
 *
 * P2-1h: THIS IS THE ORIGINAL TITLE PRESENTATION, not a text stand-in. The
 * owner's 2026-08-18 ruling is that a port ships the original branding, so
 * `scripts/menus/generate_mn_ui_kit.py` bakes what `mnTitleMakeLabels` and
 * `mnTitleMakeLogoNoOpening` draw -- the SUPER / SMASH / BROS. wordmark over
 * its drop-shadow cutout, both TM marks, the upper border, the copyright line
 * and the emblem at its own resting alpha -- composited at the DS frame's 4/5
 * into one surface, and blits it into BG2 once on entry. PRESS START is a
 * second surface because it blinks.
 *
 * P2-1i. THE FIRE SHIPS (owner findings 4/5, 2026-08-18). The source's title
 * is one GObj with two SObjs over a FILL camera (mntitle.c:934-996): 32x32
 * RGBA32 fire textures blown up 12x/8.5x and 9.5x/7.0x, each advancing its
 * texture index one per tic from (12, 0) -- a constant phase difference of 12,
 * so exactly thirty distinct pair-states. The bake tiles those thirty states
 * into the 255x252 TITLE_FIRE_ATLAS sheet (build_fire_atlas), this screen
 * blits it into BG3 once on entry, and the BG3 affine reads one 51x42 cell per
 * presented frame: the upscale is the 2D hardware's and a frame costs the
 * matrix+scroll register write alone.
 *
 * THE FIELD IS NOT BLACK, and that was worth measuring rather than assuming.
 * Both fire SObjs are SP_TRANSPARENT and `mnTitleFireProcDisplay` (:864) draws
 * them with RGB = TEXEL0, so the fire camera's COBJ_FLAG_FILLCOLOR colour
 * reaches the screen as a literal gDPFillRectangle (sys/objdisplay.c:2750).
 * Measured over the thirty states, that fill's mean transmittance through the
 * pair is 125.4/255 and only 0.012% of texels are fully covered -- half the
 * title's field IS the fill. The seven `dMNTitleFireColors` are all near-white,
 * so the bake composites onto (0xFF, 0xFF, 0xFF), table entry 0.
 *
 * ONE APPROXIMATION, disclosed: the source re-rolls that fill among the seven
 * every 260 tics with an 80-tic crossfade (mnTitleFireCameraProcUpdate,
 * :1329); a 16bpp DS BG layer has no per-channel modulator, so the bake pins
 * entry 0 rather than cycling.
 *
 * The label pop (`mnTitleMakeLabels`' anim joints, ended and snapped to rest by
 * mnTitleSetEndLayout at tic 220) LANDED in P2-1k (d): the entry blit puts the
 * settled layout up, `ndsUiKitTitleAnimLoad` arms the baked pose table, and the
 * update below drives it to that same snap. Only the FIVE wordmark pieces move
 * -- the copyright and border bands are pinned by `mnTitleUpdateLabelsPosition`
 * on this branch (mntitle.c:772) -- which is why (f2)'s edge anchoring needs no
 * reconciliation with the animation at all.
 *
 * It still plays no BGM: mnTitleInitVars calls syAudioStopBGMAll when the
 * previous scene is not the opening movie (mntitle.c:352), because the music
 * heard over the original title belongs to the opening cinematic, and that
 * cinematic is P2-7. A silent title is the source's behaviour, not a gap. */
#define NDS_MENU_TITLE_BLINK 32u

/* THERE IS NO REVEAL ON THIS BRANCH, and getting that wrong is worth the
 * paragraph. `mnTitleMakeFire` does set GOBJ_FLAG_HIDDEN (mntitle.c:988) --
 * and then calls `mnTitleShowFire` immediately unless the previous scene was
 * the opening movie (:990-993), which sets alpha to 0xFF and clears the flag
 * before the scene's first tic. Our shell enters the title from the frameless
 * boot scene, the same branch whose `mnTitleInitVars` stops the BGM
 * (:344-352) -- so the fire is at FULL ALPHA on presented frame 0.
 *
 * The tic-220 case in `mnTitleTransitionsFuncRun` (:669-677) does call
 * `mnTitleShowFire` again, which is what a reading of that switch alone
 * suggests is the reveal; on this branch it is a no-op re-show. Its real work
 * is `mnTitleSetEndLayout`'s LABEL half -- it ends the link-8 GObj's
 * animation and snaps every logo sprite to its resting position and colour.
 * That half is the still-unimplemented slide-in, not the fire. */

/* The field the title art is composited over at bake time, as a DS BGR5551
 * texel. The title surfaces are baked KEYED (the field is the fire, which
 * runs on BG3 behind the BG2 art), so the field texel is the transparent
 * key: erasing the blinking PRESS START must punch through to the fire, not
 * paint an opaque black box over it. The fire covers every screen pixel with
 * an opaque atlas texel from the first frame, so main BG palette entry 0 is
 * never actually seen on this screen -- it stays black as the safe floor. */
#define NDS_MENU_TITLE_FIELD_TEXEL ((u16)0)

static void ndsMenuShellPopulateTitle(void)
{
#if NDS_P2_1P_GAME
    /* Once per title entry, mirroring mnTitleInitVars' :356 seed. */
    sMenuTitleIdleTics = 169u;
#endif
    /* P2-1N (5). mnTitleMakeLogoNoOpening puts llMNTitleLogoAnimFullSprite at
     * source centre (260,60), primitive red, alpha 0x4C, on DL link 0.
     * mnTitleMakeSprites draws the title words/labels on link 1, so the emblem
     * is BEHIND the words; the title fire is farther back still. The source
     * sprite CONTAINER is 128x124, therefore mnTitleSetPosition puts its
     * top-left at (196,-2), which maps to (157,-2) on the DS. The previous
     * placement used the visible silhouette's inset as the OBJ origin and
     * applied that transparent margin twice, shifting the logo down/right.
     * Bitmap-OBJ alpha 5/16 is the nearest DS coefficient to 0x4C/0xFF, and
     * priority 3 puts it behind BG2 title art while remaining over BG3 fire. */
    (void)ndsUiKitSetSpriteBlend(0u,
                                 (u32)NDS_MN_UI_KIT_IMAGE_TITLE_EMBLEM,
                                 157, -2, 5u, TRUE, 3u);
}

/* The fire's cell for a presented frame: pair-state (frame % 30), with the
 * atlas 5 columns wide. `ndsPlatformSetTitleFireFrame` takes the cell's atlas
 * origin; the affine reference point is 20.8, so whole texels arrive shifted
 * there. */
static void ndsMenuShellTitleFireFrame(u32 presented_frame)
{
    u32 cell = presented_frame % NDS_MN_UI_KIT_FIRE_FRAMES;
    u32 col = cell % NDS_MN_UI_KIT_FIRE_COLS;

    ndsPlatformSetTitleFireFrame((s32)(col * NDS_MN_UI_KIT_FIRE_CELL_W),
                                 (s32)((cell / NDS_MN_UI_KIT_FIRE_COLS) *
                                       NDS_MN_UI_KIT_FIRE_CELL_H));
}

/* P2-1k (d). THE TITLE'S TIC IS A VBLANK COUNT, not an iteration count, and
 * the animation is why. Everything `mnTitle` does is per tic at 60 Hz; this
 * loop's iteration was per PRESENT, which is the same thing only while every
 * present costs one VBlank. The pop animation's peak pose moves 66,183 texels,
 * so some of its presents cost two (the owner's round-4 30 Hz latitude,
 * `docs/P2_EXECUTION_BOARD.md` Decisions) -- and an iteration counter would
 * then run the animation, and the fire beside it, at half the source's speed.
 * Counting VBlanks keeps both at the source's own rate and lets the cadence be
 * whatever the frame costs; the sampling is what drops, not the timeline.
 * Outside the animation window every present is a single VBlank, so this is
 * identical to the count it replaces. */
static u32 sMenuTitleVBlankBase;

static u32 ndsMenuShellTitleTic(void)
{
    return ndsPlatformVBlankCount() - sMenuTitleVBlankBase;
}

#if NDS_P2_1P_GAME
/* P2-7 item 6. Attract idle, transcribed from mntitle.c:260-337 (:302-335 the
 * pick) and :452-487 (mnTitleProceedDemoNext) + :712-723 (the trigger).
 *
 * The counter lives in the source counter's own domain: `mnTitleInitVars`
 * seeds `sMNTitleTransitionTotalTimeTics` at 169 on this branch (:356), and
 * the trigger compares against that numbering. The shared VBlank tic above
 * drives the fire and the pop animation, so it cannot be reseeded; the idle
 * count keeps its own counter, seeded once per title entry in
 * ndsMenuShellPopulateTitle and reset to the same seed on any input -- the
 * transcription of the :544 arm, which re-seeds the transition counter on an
 * input-made layout restart. No input recording exists anywhere here, because
 * the source has none: the demo is CPU-vs-CPU owned by scautodemo.c. */
static s32 ndsMenuShellTitleFighterKindsNum(u16 mask)
{
    s32 i;
    s32 j;

    for (i = 0, j = 0; i < (s32)(sizeof(u16) * 8); i++, mask = (u16)(mask >> 1))
    {
        if (mask & 1u)
        {
            j++;
        }
    }
    return j;
}

/* mnTitleGetShuffledFighterKind, :275-293, verbatim. */
static s32 ndsMenuShellTitleShuffledFighterKind(u16 this_mask, u16 prev_mask,
                                                s32 random)
{
    s32 fkind = -1;

    random++;
    do
    {
        fkind++;
        if ((this_mask & LBBACKUP_MASK_FIGHTER(fkind)) &&
            !(prev_mask & LBBACKUP_MASK_FIGHTER(fkind)))
        {
            random--;
        }
    } while (random != 0);
    return fkind;
}

/* mnTitleSetDemoFighterKinds, :296-337, verbatim: two shuffled no-repeat
 * picks through demo_mask_prev, remembering the first pick of a cycle in
 * demo_first_fkind. */
static void ndsMenuShellTitleSetDemoFighterKinds(void)
{
    u16 unlocked_mask;
    s32 unlocked_count;
    s32 non_recently_demoed_count;

    unlocked_mask = (u16)(gSCManagerBackupData.fighter_mask |
                          LBBACKUP_CHARACTER_MASK_STARTER);
    if ((u16)(~unlocked_mask & gSCManagerSceneData.demo_mask_prev) != 0u)
    {
        gSCManagerSceneData.demo_mask_prev = 0;
    }
    unlocked_count =
        ndsMenuShellTitleFighterKindsNum(unlocked_mask);
    if (unlocked_count <=
        ndsMenuShellTitleFighterKindsNum(gSCManagerSceneData.demo_mask_prev))
    {
        gSCManagerSceneData.demo_mask_prev = 0;
    }
    unlocked_count =
        ndsMenuShellTitleFighterKindsNum(unlocked_mask);
    gSCManagerSceneData.demo_fkind[0] = (u8)ndsMenuShellTitleShuffledFighterKind(
        unlocked_mask, gSCManagerSceneData.demo_mask_prev,
        syUtilsRandIntRange(unlocked_count -
            ndsMenuShellTitleFighterKindsNum(
                gSCManagerSceneData.demo_mask_prev)));
    if (gSCManagerSceneData.demo_mask_prev == 0u)
    {
        gSCManagerSceneData.demo_first_fkind =
            gSCManagerSceneData.demo_fkind[0];
    }
    gSCManagerSceneData.demo_mask_prev |=
        (u16)LBBACKUP_MASK_FIGHTER(gSCManagerSceneData.demo_fkind[0]);
    unlocked_count =
        ndsMenuShellTitleFighterKindsNum(unlocked_mask);
    non_recently_demoed_count = unlocked_count -
        ndsMenuShellTitleFighterKindsNum(gSCManagerSceneData.demo_mask_prev);
    if (non_recently_demoed_count == 0)
    {
        gSCManagerSceneData.demo_fkind[1] =
            gSCManagerSceneData.demo_first_fkind;
    }
    else
    {
        gSCManagerSceneData.demo_fkind[1] = (u8)ndsMenuShellTitleShuffledFighterKind(
            unlocked_mask, gSCManagerSceneData.demo_mask_prev,
            syUtilsRandIntRange(non_recently_demoed_count));
        gSCManagerSceneData.demo_mask_prev |=
            (u16)LBBACKUP_MASK_FIGHTER(gSCManagerSceneData.demo_fkind[1]);
    }
}

/* mnTitleProceedDemoNext, :452-487, transcribed for the native shell. The
 * source writes scene_prev = scene_curr then scene_curr = next (:463) with
 * the next picked from the PREVIOUS scene (:454/465-484); the port-owned
 * transition owns those two writes instead (ndsMenuShellGoto now,
 * ndsSceneManagerRequest at screen exit, which records prev = Title -- the
 * same value the source writes), so this only picks the destination, sets
 * the extend flag (:485), and hands off. scene_prev is read here for the
 * same reason the source reads it: nothing else can change it mid-screen.
 *
 * DELTA, disclosed: the ModeSelect/AutoDemo arm names nSCKindStartup on US
 * (:475). Startup is deliberately outside the scene registry (fail-closed;
 * nds_scene_manager.c:205), so requesting it would refuse and park the
 * title. A title re-entry keeps the attract loop alive on the same screen
 * instead -- the one arm whose destination differs, and only in where the
 * loop resumes. The Explain arm's BGM play (:469) rides ndsAudioBgmPlay, the
 * shell's func_start equivalent the way ModeSelect's own play does. The
 * black camera fade (:456) and the FGM shutter (:461) have no native-shell
 * expression and are stated rather than silently dropped. */
static void ndsMenuShellTitleProceedDemoNext(void)
{
    u8 scene_prev = gSCManagerSceneData.scene_prev;
    u32 next;

    ndsMenuShellTitleSetDemoFighterKinds();
    switch (scene_prev)
    {
    case nSCKindExplain:
        next = (u32)nSCKindCharacters;
        ndsAudioBgmPlay(0, (s32)nSYAudioBGMExplain);
        break;
    case nSCKindModeSelect:
    case nSCKindAutoDemo:
        /* Source: nSCKindStartup (US) / nSCKindOpeningRoom (JP). See above. */
        next = (u32)nSCKindTitle;
        break;
    default:
        next = (u32)nSCKindExplain;
        break;
    }
    gSCManagerSceneData.is_extend_demo_wait = TRUE;
    ndsMenuShellGoto(next);
}
#endif

static void ndsMenuShellUpdateTitle(u32 held, u32 taps)
{
    u32 tic = ndsMenuShellTitleTic();
    u32 phase = ((tic / NDS_MENU_TITLE_BLINK) & 1u);
    u32 animating = (u32)(ndsUiKitTitleAnimActive() != FALSE);

    (void)held;
    /* Toggle on the EDGE only. Redrawing every frame would put a 77x14 blit in
     * the steady-state frame cost of a screen that is not changing, which is
     * exactly what the kit's compose/commit discipline exists to avoid.
     *
     * P2-1k (d): AND NOT AT ALL WHILE THE POP ANIMATION RUNS. PRESS START bakes
     * to (91,134)..(168,148), inside the row band the animation erases and
     * redraws every frame, and sixteen of the fifty-one poses have a dirty
     * rectangle that reaches it. Two attempts at coexistence both failed, and
     * the second failed for a reason worth writing down: redrawing the label
     * from its cache AFTER each animated frame is correct in the buffer and
     * still WRONG ON THE PANEL, because BG2 is scanned out live -- there is no
     * back buffer and none to be had (main C and D are one 256x256 Bmp16 each
     * with zero slack, docs/p2/P2-1c-vram-map.md). A pose costing 850,240 ticks
     * is longer than the beam takes to reach row 134, so whether the frame the
     * panel shows contains the label depends on where the beam was; two
     * captures at the same blink phase, poses 24 and 30, disagreed. That is a
     * flicker, and this project does not ship one.
     *
     * The source does not have the problem because it does not have the label:
     * `mnTitleMakePressStart` leaves the GObj HIDDEN (mntitle.c:1261) and
     * `mnTitleShowGObjLinkID(9)` reveals it at tic 280 (:697) -- sixty tics
     * AFTER the tic-220 snap this animation ends on. Suppressing it for the
     * fifty-one animated tics is therefore the source's own behaviour on the
     * only interval where the two collide; the shell keeps its existing early
     * reveal from the snap onward rather than moving to tic 280, because that
     * arrives paired with the source's input gate (`is_title_anim_viewed`,
     * :533/:644) and that pair is the owner's to rule on. */
    if (animating != FALSE)
    {
        phase = 0xffffffffu;
    }
    if (phase != sMenuTitleBlinkPhase)
    {
        sMenuTitleBlinkPhase = phase;
        if (phase == 0u)
        {
            ndsUiKitDrawCachedSurface();
        }
        else if (phase != 0xffffffffu)
        {
            ndsUiKitEraseCachedSurface(NDS_MENU_TITLE_FIELD_TEXEL);
        }
        else
        {
            /* Entering the animation window: the entry blit drew the label, so
             * take it off the panel once rather than fight the animation for
             * those rows. The sentinel makes the first frame after the snap an
             * edge again, whichever half of the blink cycle it lands in. */
            ndsUiKitEraseCachedSurface(NDS_MENU_TITLE_FIELD_TEXEL);
        }
    }
    /* THE FIRE'S TIMELINE: one cell per presented frame, from entry. The
     * source advances both fire SObjs one texture index per tic
     * (`mnTitleFireProcUpdate`, mntitle.c:925) at a constant phase difference
     * of 12, so the pair has exactly thirty states and state k is the atlas's
     * cell k. Enable happened at entry, so this is the whole per-frame cost:
     * an affine matrix+scroll write, four registers and no texels.
     *
     * THE COUNTER IS sMenuTics, this screen's own tic. `gNdsMenuShellFrames`
     * was the obvious choice and is the wrong one: `ndsMenuShellRecordFrame`
     * returns before incrementing it on a scene's first frame (the load frame
     * is deliberately outside the work distribution), so the first cell was
     * held for three presents before the advance became one-per-frame.
     * sMenuTics increments unconditionally, once per loop iteration, which is
     * exactly the source's own per-tic advance -- and P2-1k (d) replaced it
     * with the VBlank tic above for the reason stated there. */
    ndsMenuShellTitleFireFrame(tic);
    /* P2-1k (d). The pop animation, driven by the same tic and therefore by
     * the same clock as the fire: `ndsUiKitTitleAnimDraw` samples the pose the
     * source would be showing after `tic` tics and settles itself at
     * mnTitleSetEndLayout's own snap, after which this stops being called. */
    if (ndsUiKitTitleAnimActive() != FALSE)
    {
        /* Pose 1 is the FIRST tic, not the zeroth: on this branch
         * `mnTitleInitVars` seeds the counter at 169 and
         * `mnTitleTransitionsFuncRun` increments before it switches, so tic 170
         * -- `mnTitleShowGObjLinkID(8)`, the labels appearing -- is elapsed 0.
         * The snap at tic 220 is therefore elapsed 50, pose 51. */
        ndsUiKitTitleAnimDraw(tic + 1u);
    }
    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        ndsUiKitSfx(NDS_UI_KIT_SFX_START);
        ndsMenuShellGoto((u32)nSCKindModeSelect);
    }
#if NDS_P2_1P_GAME
    /* P2-7 item 6. mntitle.c:712-723: idle 650 tics (1190 once
     * is_extend_demo_wait is set) calls mnTitleProceedDemoNext. Any input
     * re-seeds the counter (:544 arm); the A/START arm above already left,
     * so only the else path advances. Exact-equality cases mirror the
     * source's switch. */
    else if (taps != 0u)
    {
        sMenuTitleIdleTics = 169u;
    }
    else
    {
        sMenuTitleIdleTics++;
        if (((sMenuTitleIdleTics == 650u) &&
             (gSCManagerSceneData.is_extend_demo_wait == FALSE)) ||
            ((sMenuTitleIdleTics == 1190u) &&
             (gSCManagerSceneData.is_extend_demo_wait != FALSE)))
        {
            ndsMenuShellTitleProceedDemoNext();
        }
    }
#endif
}
