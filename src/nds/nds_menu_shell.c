/* P2-1d -- the VS shell's real screens. Contract and reasoning:
 * include/nds/nds_menu_shell.h. Per-scene VRAM ownership:
 * docs/p2/P2-1c-vram-map.md. */

#include "nds_build_config.h"

#if NDS_P2_MENU_SHELL

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
/* CSS panel surface ids are retained in u8 state. P2-2 adds the 108 exact
 * Team-Battle panel variants below; keep the representation honest if future
 * menu art ever pushes the generated manifest past one byte. */
_Static_assert(NDS_MN_UI_KIT_SURFACE_COUNT <= 256u,
               "menu surface ids stored in u8 must fit the generated pack");

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);
/* sys/utils.c's own time-seeded pick, which mnMapsSaveSceneData uses for the
 * RANDOM cell (mnmaps.c:1379). It reads osGetTime() and does NOT touch
 * sSYUtilsRandomSeed, so calling it from a menu cannot perturb the gameplay
 * RNG. Declared rather than included: include/sys/ carries no utils.h. */
extern s32 syUtilsRandTimeUCharRange(s32 range);

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
 * value both ways, the time value both ways, the refusal on VS OPTIONS -- and
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
    { (u16)NDS_INPUT_A, 1u },     /* refusal: VS OPTIONS is not built  */
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
 * still distinct from the preset's 3, so the battle state reading 2 remains
 * the proof that the DESCRIPTOR, not the preset, decides the match. */
static const NdsMenuWalkStep kNdsMenuWalkCss[] = {
    { (u16)NDS_INPUT_UP, 30u },
    { (u16)NDS_INPUT_RIGHT, 9u },
    { (u16)NDS_INPUT_A, 1u },
    { (u16)NDS_INPUT_RIGHT, 10u },
    { (u16)NDS_INPUT_A, 1u },
    { (u16)NDS_INPUT_LEFT, 10u },
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
static const NdsMenuWalkStep *const
    kNdsMenuWalkScripts[NDS_MENU_SHELL_SCREEN_COUNT] = {
    kNdsMenuWalkTitle, kNdsMenuWalkMode, kNdsMenuWalkVs, kNdsMenuWalkCss,
    kNdsMenuWalkSss
};
static const u8 kNdsMenuWalkLengths[NDS_MENU_SHELL_SCREEN_COUNT] = {
    (u8)(sizeof(kNdsMenuWalkTitle) / sizeof(kNdsMenuWalkTitle[0])),
    (u8)(sizeof(kNdsMenuWalkMode) / sizeof(kNdsMenuWalkMode[0])),
    (u8)(sizeof(kNdsMenuWalkVs) / sizeof(kNdsMenuWalkVs[0])),
    (u8)(sizeof(kNdsMenuWalkCss) / sizeof(kNdsMenuWalkCss[0])),
    (u8)(sizeof(kNdsMenuWalkSss) / sizeof(kNdsMenuWalkSss[0]))
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
}

/* --- Screen: mode select -------------------------------------------------
 *
 * mnmodeselect.c: four entries in the order 1P GAME / VS MODE / OPTION / DATA;
 * UP or RIGHT decrements the cursor and wraps Start -> End, DOWN or LEFT
 * increments and wraps End -> Start (mnmodeselect.c:806/:826); the move cue is
 * MenuScroll2 and the confirm cue is MenuSelect; B returns to the title, and
 * the source spends no cue on it. Only VS MODE is built, so the other three
 * refuse with MenuDenied -- the id the source itself spends on a refused
 * selection. P2-1i stopped GREYING them: the source draws all four entries
 * identically and distinguishes only the selected one, so a locked colour
 * would be invented state on top of the source's own art. The refusal cue is
 * what says "not built", and it says it at the moment it is true.
 *
 * The five-minute idle return to the title (mnmodeselect.c:702) is attract
 * behaviour and belongs to P2-7; it is deliberately absent rather than
 * stubbed. */
/* The entry count is the BAKE'S, not a second opinion: the four sites the
 * generator recorded and the four bright icons it packed are the same four
 * entries this screen moves between, so a fifth entry has to arrive in both
 * places or in neither. */
#define NDS_MENU_MODE_ENTRIES NDS_MN_UI_KIT_MODE_ENTRY_COUNT
#define NDS_MENU_MODE_VS 1u
/* Slot 0 held the invented hand; it now holds the lit entry icon. */
#define NDS_MENU_SPRITE_MODE_LIT NDS_MENU_SPRITE_CURSOR

static u32 sMenuModeCursor;

static void ndsMenuShellPopulateMode(void)
{
    /* P2-1i, owner finding (2). THE WHOLE PLATE IS THE SOURCE'S OWN ART now:
     * the MODE SELECT heading, both decal bars, the SMASH emblem, the four
     * entry icons in their unselected form and the four red English labels are
     * one baked MODE_SELECT surface (mnModeSelectMakeDecals/MakeLabels and
     * mnModeSelectMake<Entry>, mnmodeselect.c:151-579), blitted into BG2 once
     * at entry. The font-composed "MODE SELECT"/"P GAME"/"VS MODE"/"OPTION"/
     * "DATA" rows and the digit-sprite "1" that used to stand in for them are
     * gone.
     *
     * THE ONLY THING THE CURSOR CHANGES is which entry is lit, and the source
     * does that by swapping that one entry's sprite: `...IconSprite` at white
     * with a black env colour when selected (:161-175), `...IconDarkSprite` at
     * grey 0x96 otherwise (:213-223). The dark four are already in the
     * surface, so this is ONE OBJ drawn over the selected one -- at the exact
     * pixels the bake recorded for its dark twin, which is why the light-up is
     * a recolour in place and not a sprite that lands near it.
     *
     * AND THERE IS NO CURSOR ON THIS SCREEN. `mnmodeselect.c` contains no
     * cursor of any kind -- not a hand, not a frame; the light-up IS the
     * selection feedback. P2-1d's hand here was invented, and it is removed
     * rather than kept beside the source's own mechanism. */
    ndsUiKitSetSprite(NDS_MENU_SPRITE_MODE_LIT,
                      NDS_MN_UI_KIT_IMAGE_MODE_ICON_1P + sMenuModeCursor,
                      kNdsUiKitModeEntrySite[sMenuModeCursor][0],
                      kNdsUiKitModeEntrySite[sMenuModeCursor][1]);
}

static void ndsMenuShellUpdateMode(u32 held, u32 taps)
{
    u32 moved = FALSE;

    if (ndsMenuShellDirection(held, taps,
                              NDS_INPUT_UP | NDS_INPUT_RIGHT) != FALSE)
    {
        sMenuModeCursor = (sMenuModeCursor == 0u) ?
            (NDS_MENU_MODE_ENTRIES - 1u) : (sMenuModeCursor - 1u);
        moved = TRUE;
    }
    else if (ndsMenuShellDirection(held, taps,
                                   NDS_INPUT_DOWN | NDS_INPUT_LEFT) != FALSE)
    {
        sMenuModeCursor = (sMenuModeCursor + 1u) % NDS_MENU_MODE_ENTRIES;
        moved = TRUE;
    }
    if (moved != FALSE)
    {
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        ndsMenuShellPopulateMode();
    }

    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        if (sMenuModeCursor == NDS_MENU_MODE_VS)
        {
            ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
            ndsMenuShellGoto((u32)nSCKindVSMode);
            return;
        }
        /* 1P GAME (P2-6), OPTION and DATA (P2-7) are present and inert. */
        ndsUiKitSfx(NDS_UI_KIT_SFX_BACK);
        gNdsMenuShellDeniedCount++;
    }
    else if ((taps & NDS_INPUT_B) != 0u)
    {
        /* P2-1k (g). mnmodeselect.c:774 -- the main menu's B arm stops all BGM
         * before it requests the title. The title's own mnTitleInitVars stop
         * would cover it, but both are the source's and transcribing only one
         * would leave the other's absence looking deliberate. */
        ndsAudioBgmStopAll();
        ndsMenuShellGoto((u32)nSCKindTitle);
    }
}

/* --- Screen: VS mode + rules --------------------------------------------
 *
 * In SSB64 the VS menu IS the rules screen: mnvsmode.c draws four buttons --
 * VS START, RULE, the TIME/STOCK value, and VS OPTIONS -- and UP/DOWN moves
 * between them while LEFT/RIGHT edits the one under the cursor. Transcribed
 * from the source:
 *
 *   cursor  UP wraps Start -> Options, DOWN wraps Options -> Start
 *           (mnvsmode.c:1371/:1412), cue MenuScroll2.
 *   RULE    LEFT/RIGHT walk the rule, CLAMPED at both ends rather than
 *           wrapping (mnvsmode.c:1445/:1476), cue MenuScroll1.
 *   VALUE   time 1..99 then INFINITE; left from 1 lands on INFINITE, right
 *           from 100 lands on 1 (mnvsmode.c:1524/:1568). Stock is stored 0..98
 *           and shown +1. Cue MenuScroll1.
 *   A/START on VS START saves the settings and leaves (mnvsmode.c:1318),
 *           cue MenuSelect.
 *   B       saves the settings and returns to the mode select
 *           (mnvsmode.c:1347), and the source spends no cue.
 *
 * The source's full four-value rule range is live here: TIME, STOCK, TIME TEAM,
 * STOCK TEAM.  P2-2 made Team Battle a real engine mode, so keeping the former
 * P2-1 two-value clamp would now be a behavior difference. VS OPTIONS is the
 * one deliberate narrowing left: it keeps its row and its place in the cursor
 * cycle but refuses, because handicap/damage/item-switch are P2-5/P2-7.
 *
 * The rules this screen commits are the descriptor fields it owns -- rule,
 * time limit, stock count -- written into gNdsMatchConfig and installed with
 * ndsMatchConfigApply. The FIGHTER half of the same descriptor belongs to the
 * character select below (P2-1e), which this screen's VS START now leads to. */
#define NDS_MENU_VS_ENTRIES 4u
#define NDS_MENU_VS_START 0u
#define NDS_MENU_VS_RULE 1u
#define NDS_MENU_VS_VALUE 2u
#define NDS_MENU_VS_OPTIONS 3u

#define NDS_MENU_RULE_TIME 0u
#define NDS_MENU_RULE_STOCK 1u
#define NDS_MENU_RULE_TIME_TEAM 2u
#define NDS_MENU_RULE_STOCK_TEAM 3u
#define NDS_MENU_RULE_MAX NDS_MENU_RULE_STOCK_TEAM

#define NDS_MENU_TIME_MAX 100 /* SCBATTLE_TIMELIMIT_INFINITE */
#define NDS_MENU_STOCK_MAX 98

/* P2-1j, owner finding (b). THE SOURCE HAS NO CURSOR ON THIS SCREEN, and the
 * hand P2-1d put here was an invention that survived P2-1i only because
 * removing it would have left the screen with no selection feedback at all
 * (that row's own note). This is the feedback the source uses:
 * `mnVSModeUpdateButton` (mnvsmode.c:231) recolours the BUTTON the cursor
 * index names, through the IA combiner's two-colour ramp, and the four buttons
 * are the screen's only moving part.
 *
 * WHAT A BUTTON IS: `llMNCommonOptionTab{Left,Middle,Right}Sprite` with the
 * middle tiled over `arg3 * 8` px (:281), arg3 = 17 everywhere here, plus the
 * button's own black text -- 168x29 in the source's frame, 134x23 on the DS.
 * That is larger than a 64x64 bitmap-OBJ cell in both axes' worth of area, and
 * two states of four buttons would be 34,816 B of main OBJ against 16,640 B
 * free, so each button-state is a BG2 surface baked WITH the collage that sits
 * under it (generate_mn_ui_kit.py `under=`). Opaque, so a state change is a
 * whole-row DMA that overwrites the previous state exactly.
 *
 * THE COST IS PAID ON A CURSOR MOVE AND NOWHERE ELSE. Only the two buttons
 * whose state actually changed are re-blitted, so a move reads 12,328 B of
 * NitroFS against the 560,190-tick 60 Hz budget, and a screen holding still
 * reads none. `gNdsMenuShellVsButtonBlitCount` is what keeps that honest.
 *
 * THE ARROWS ARE OBJ, and that is a blink fact rather than a size one: both
 * pairs toggle on a 30-tic cycle while the cursor is on their row
 * (`mnVSModeAnimateRuleArrows` :466, `...TimeStockArrows` :539), and hiding an
 * OBJ is free where re-blitting a surface is a NitroFS read. */
#define NDS_MENU_VS_SURFACE_NONE 0xffu
/* mnVSModeAnimateRuleArrows' own blink period (30 tics on, 30 off). */
#define NDS_MENU_VS_ARROW_BLINK 30u
/* This screen's layout is now kept in the SOURCE's own 320x240 units, exactly
 * as the character select's and the stage select's are, because every number
 * on it is quoted from mnvsmode.c. The DS screen is exactly 0.8 of that frame
 * on both axes (256/320 = 192/240), so drawing is one multiply. */
#define NDS_MENU_VS_DS(v) (((v) * 4) / 5)

static u32 sMenuVsCursor;
static u32 sMenuVsRule;
static s32 sMenuVsTime;
static s32 sMenuVsStock;
static u8 sMenuVsButtonSurface[NDS_MENU_VS_ENTRIES];
static u32 sMenuVsArrowsShown;

static u32 ndsMenuShellVsIsTime(void)
{
    return ((sMenuVsRule == NDS_MENU_RULE_TIME) ||
            (sMenuVsRule == NDS_MENU_RULE_TIME_TEAM)) ? TRUE : FALSE;
}

static u32 ndsMenuShellVsIsTeam(void)
{
    return (sMenuVsRule >= NDS_MENU_RULE_TIME_TEAM) ? TRUE : FALSE;
}

static s32 ndsMenuShellVsValue(void)
{
    return (ndsMenuShellVsIsTime() != FALSE) ? sMenuVsTime : (sMenuVsStock + 1);
}

/* Which baked state each button should be showing right now. The pairs are the
 * source's own branches: the rule button carries its VALUE word and the
 * time/stock button its PERIOD word, both of which change with the rule
 * (mnvsmode.c:337/:679), so those two buttons have a TIME and a STOCK bake. */
static u8 ndsMenuShellVsWantSurface(u32 button)
{
    u32 lit = (sMenuVsCursor == button) ? TRUE : FALSE;

    switch (button)
    {
    case NDS_MENU_VS_START:
        return (u8)((lit != FALSE) ? NDS_MN_UI_KIT_SURFACE_VS_BTN_START_HI :
                                     NDS_MN_UI_KIT_SURFACE_VS_BTN_START_NOT);
    case NDS_MENU_VS_RULE:
        if (sMenuVsRule == NDS_MENU_RULE_TIME)
        {
            return (u8)((lit != FALSE) ?
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_TIME_HI :
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_TIME_NOT);
        }
        if (sMenuVsRule == NDS_MENU_RULE_STOCK)
        {
            return (u8)((lit != FALSE) ?
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_STOCK_HI :
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_STOCK_NOT);
        }
        if (sMenuVsRule == NDS_MENU_RULE_TIME_TEAM)
        {
            return (u8)((lit != FALSE) ?
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_TIME_TEAM_HI :
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_TIME_TEAM_NOT);
        }
        return (u8)((lit != FALSE) ?
                    NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_STOCK_TEAM_HI :
                    NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_STOCK_TEAM_NOT);
    case NDS_MENU_VS_VALUE:
        if (ndsMenuShellVsIsTime() != FALSE)
        {
            return (u8)((lit != FALSE) ?
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_TIME_HI :
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_TIME_NOT);
        }
        return (u8)((lit != FALSE) ? NDS_MN_UI_KIT_SURFACE_VS_BTN_STOCK_HI :
                                     NDS_MN_UI_KIT_SURFACE_VS_BTN_STOCK_NOT);
    default:
        break;
    }
    return (u8)((lit != FALSE) ? NDS_MN_UI_KIT_SURFACE_VS_BTN_OPTIONS_HI :
                                 NDS_MN_UI_KIT_SURFACE_VS_BTN_OPTIONS_NOT);
}

/* Re-blit the buttons whose state changed, AT MOST `budget` of them, in one
 * call so a batch costs one NitroFS open.
 *
 * THE BUDGET IS THE 60 Hz FRAME, and it is measured rather than assumed. The
 * first cut of this blitted every changed button on the frame the change
 * happened, which for a cursor move is TWO -- the one going dark and the one
 * lighting up, 12,328 B of NitroFS -- and the shipping-configuration probe
 * priced that frame at **606,336 ticks against the 560,190-tick single-VBlank
 * budget**, the one 2-VBlank present in 1,811 VS-menu frames
 * (artifacts/verification/2026-08-18_p2-1j-shell-before.txt, MSVB2/MSMAX w2).
 * A screen ENTRY can afford all four because it is already a load frame; a
 * cursor move gets one a frame, so the two-button swap completes in two
 * frames, 33 ms, and no single present carries the burst.
 *
 * The order that falls out is also the right one to watch: the loop below
 * takes buttons in cursor order, so the button that just LIT UP is written
 * first whenever the cursor moved down, and the stale highlight is cleared on
 * the following frame. */
static void ndsMenuShellVsSyncButtons(u32 budget)
{
    u8 list[NDS_MENU_VS_ENTRIES];
    u8 wanted[NDS_MENU_VS_ENTRIES];
    u32 index[NDS_MENU_VS_ENTRIES];
    u32 count = 0u;
    u32 i;

    for (i = 0u; i < NDS_MENU_VS_ENTRIES; i++)
    {
        wanted[i] = ndsMenuShellVsWantSurface(i);
        if ((wanted[i] != sMenuVsButtonSurface[i]) && (count < budget))
        {
            list[count] = wanted[i];
            index[count] = i;
            count++;
        }
    }
    if (count == 0u)
    {
        return;
    }
    if (ndsUiKitBlitSurfaces(list, count) == FALSE)
    {
        /* A refused blit leaves the tracker alone so the next frame retries,
         * rather than recording a state the screen is not showing. */
        return;
    }
    for (i = 0u; i < count; i++)
    {
        sMenuVsButtonSurface[index[i]] = list[i];
    }
    gNdsMenuShellVsButtonBlitCount += count;
}

/* mnVSModeMakeTimeStockValue, mnvsmode.c:634. `x` is the RIGHT edge -- the
 * first digit lands at `x - 11` and the rest walk left at that pitch, which is
 * exactly ndsUiKitSetNumber's contract -- and the value is drawn in white at
 * y = 116, or the infinity glyph at (162, 118) for the infinite time limit. */
static void ndsMenuShellVsDrawValue(void)
{
    s32 value = ndsMenuShellVsValue();
    s32 right;
    u32 i;

    for (i = NDS_MENU_SPRITE_NUM0; i < NDS_MENU_SPRITE_NUM2 + 1u; i++)
    {
        ndsUiKitHideSprite(i);
    }
    if (value == NDS_MENU_TIME_MAX)
    {
        ndsUiKitSetSprite(NDS_MENU_SPRITE_NUM0, NDS_MN_UI_KIT_IMAGE_INFINITY,
                          NDS_MENU_VS_DS(162), NDS_MENU_VS_DS(118));
        return;
    }
    if (ndsMenuShellVsIsTime() != FALSE)
    {
        right = (value < 10) ? 185 : 190;
    }
    else
    {
        right = (value < 10) ? 210 : 215;
    }
    (void)ndsUiKitSetNumber(NDS_MENU_SPRITE_NUM0, 2u, value,
                            NDS_MENU_VS_DS(right), NDS_MENU_VS_DS(116));
}

/* The two arrow pairs, at their own source positions. Only the pair belonging
 * to the row the cursor is on is ever shown (`else ... GOBJ_FLAG_HIDDEN` in
 * both animate threads), the rule pair drops its LEFT arrow at the first rule
 * and its RIGHT at the last (:487/:501 -- Time and Stock Team respectively),
 * and the pair blinks on the source's own 30-tic cycle. */
static void ndsMenuShellVsDrawArrows(void)
{
    s32 x_left;
    s32 y;
    u32 show_left;
    u32 show_right;

    if (sMenuVsCursor == NDS_MENU_VS_RULE)
    {
        x_left = 165;
        y = 70;
        show_left = (sMenuVsRule == NDS_MENU_RULE_TIME) ? FALSE : TRUE;
        show_right = (sMenuVsRule == NDS_MENU_RULE_STOCK_TEAM) ? FALSE : TRUE;
    }
    else if (sMenuVsCursor == NDS_MENU_VS_VALUE)
    {
        x_left = (ndsMenuShellVsIsTime() != FALSE) ? 155 : 165;
        y = 109;
        show_left = TRUE;
        show_right = TRUE;
    }
    else
    {
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_L);
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_R);
        return;
    }
    if (sMenuVsArrowsShown == FALSE)
    {
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_L);
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_R);
        return;
    }
    if (show_left != FALSE)
    {
        ndsUiKitSetSprite(NDS_MENU_SPRITE_ARROW_L,
                          NDS_MN_UI_KIT_IMAGE_VS_ARROW_L,
                          NDS_MENU_VS_DS(x_left), NDS_MENU_VS_DS(y));
    }
    else
    {
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_L);
    }
    if (show_right != FALSE)
    {
        ndsUiKitSetSprite(NDS_MENU_SPRITE_ARROW_R,
                          NDS_MN_UI_KIT_IMAGE_VS_ARROW_R,
                          NDS_MENU_VS_DS((sMenuVsCursor ==
                                          NDS_MENU_VS_RULE) ? 250 : 230),
                          NDS_MENU_VS_DS(y));
    }
    else
    {
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_R);
    }
}

/* A STATE CHANGE, not an entry: the OBJ half only. The buttons follow on the
 * next frames through the per-frame sync above, which is what keeps a cursor
 * move inside one 60 Hz present. */
static void ndsMenuShellRefreshVs(void)
{
    ndsMenuShellVsDrawValue();
    ndsMenuShellVsDrawArrows();
}

/* Screen ENTRY. This one runs on a load frame -- ndsMenuShellRun calls it once
 * after the backdrop, before the loop -- so it may write all four buttons. */
static void ndsMenuShellPopulateVs(void)
{
    ndsMenuShellVsSyncButtons(NDS_MENU_VS_ENTRIES);
    ndsMenuShellRefreshVs();
}

/* mnVSModeFuncStartVars: the screen opens on the rules the battle state
 * already carries, so a return trip shows what the last visit committed. */
static void ndsMenuShellVsLoadRules(void)
{
    u32 i;

    for (i = 0u; i < NDS_MENU_VS_ENTRIES; i++)
    {
        /* Nothing is on the screen yet, so every button differs from what the
         * freshly cleared BG2 shows and all four blit on the entry frame. */
        sMenuVsButtonSurface[i] = (u8)NDS_MENU_VS_SURFACE_NONE;
    }
    sMenuVsArrowsShown = TRUE;
    sMenuVsCursor = NDS_MENU_VS_START;
    if (gSCManagerTransferBattleState.is_team_battle == FALSE)
    {
        sMenuVsRule = (gSCManagerTransferBattleState.game_rules ==
                       SCBATTLE_GAMERULE_TIME) ? NDS_MENU_RULE_TIME :
                                                 NDS_MENU_RULE_STOCK;
    }
    else
    {
        sMenuVsRule = (gSCManagerTransferBattleState.game_rules ==
                       SCBATTLE_GAMERULE_TIME) ? NDS_MENU_RULE_TIME_TEAM :
                                                 NDS_MENU_RULE_STOCK_TEAM;
    }
    sMenuVsTime = (s32)gSCManagerTransferBattleState.time_limit;
    sMenuVsStock = (s32)gSCManagerTransferBattleState.stocks;
}

/* mnVSModeSaveSettings, through the P2-1a descriptor rather than straight into
 * the battle state: the descriptor is the battle's only input, so the menu
 * writes the fields it owns and ndsMatchConfigApply installs the whole match. */
static void ndsMenuShellVsSaveRules(void)
{
    gNdsMatchConfig.game_rules = (ndsMenuShellVsIsTime() != FALSE) ?
        (u8)SCBATTLE_GAMERULE_TIME : (u8)SCBATTLE_GAMERULE_STOCK;
    gNdsMatchConfig.time_limit = (u8)sMenuVsTime;
    gNdsMatchConfig.stocks = (u8)sMenuVsStock;
    /* mnVSModeSaveSettings, mnvsmode.c:1144-1162: the rule itself carries the
     * FFA/Team bit. CSS exposes the same state through its source mode toggle;
     * whichever screen changes it last becomes the descriptor's one truth. */
    gNdsMatchConfig.is_team_battle =
        (ndsMenuShellVsIsTeam() != FALSE) ? TRUE : FALSE;
    ndsMatchConfigApply(&gNdsMatchConfig);

    gNdsMenuShellCommitCount++;
    gNdsMenuShellCommitRule = (u32)gNdsMatchConfig.game_rules;
    gNdsMenuShellCommitTime = (u32)gNdsMatchConfig.time_limit;
    gNdsMenuShellCommitStocks = (u32)gNdsMatchConfig.stocks;
}

static void ndsMenuShellVsAdjust(s32 direction)
{
    if (sMenuVsCursor == NDS_MENU_VS_RULE)
    {
        /* Clamped, not wrapped -- the source's own shape. */
        if ((direction < 0) && (sMenuVsRule > NDS_MENU_RULE_TIME))
        {
            sMenuVsRule--;
        }
        else if ((direction > 0) && (sMenuVsRule < NDS_MENU_RULE_MAX))
        {
            sMenuVsRule++;
        }
        else
        {
            return;
        }
        ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        ndsMenuShellRefreshVs();
        return;
    }
    if (sMenuVsCursor != NDS_MENU_VS_VALUE)
    {
        return;
    }
    if (ndsMenuShellVsIsTime() != FALSE)
    {
        if (direction < 0)
        {
            sMenuVsTime = (sMenuVsTime == 1) ? NDS_MENU_TIME_MAX :
                                               (sMenuVsTime - 1);
        }
        else
        {
            sMenuVsTime = (sMenuVsTime == NDS_MENU_TIME_MAX) ? 1 :
                                                               (sMenuVsTime + 1);
        }
    }
    else
    {
        if (direction < 0)
        {
            sMenuVsStock = (sMenuVsStock == 0) ? NDS_MENU_STOCK_MAX :
                                                 (sMenuVsStock - 1);
        }
        else
        {
            sMenuVsStock = (sMenuVsStock == NDS_MENU_STOCK_MAX) ? 0 :
                                                                  (sMenuVsStock + 1);
        }
    }
    ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
    ndsMenuShellRefreshVs();
}

static void ndsMenuShellUpdateVs(u32 held, u32 taps)
{
    u32 moved = FALSE;
    /* The arrows' own blink, mnvsmode.c:474/:545: a tic counter that toggles
     * the GObj's HIDDEN flag every 30 tics. Recomputed from sMenuTics rather
     * than kept as a countdown, so a screen re-entry restarts the phase where
     * the source's own timer restarts it -- at the top. */
    u32 shown = (((sMenuTics / NDS_MENU_VS_ARROW_BLINK) & 1u) == 0u) ? TRUE :
                                                                       FALSE;

    /* ONE button surface a frame, and never more (see ndsMenuShellVsSyncButtons
     * for the 606,336-tick measurement that put this here). A frame on which
     * nothing changed compares four bytes and returns. */
    ndsMenuShellVsSyncButtons(1u);

    if (shown != sMenuVsArrowsShown)
    {
        sMenuVsArrowsShown = shown;
        ndsMenuShellVsDrawArrows();
    }

    if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
    {
        sMenuVsCursor = (sMenuVsCursor == NDS_MENU_VS_START) ?
            (NDS_MENU_VS_ENTRIES - 1u) : (sMenuVsCursor - 1u);
        moved = TRUE;
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
    {
        sMenuVsCursor = (sMenuVsCursor + 1u) % NDS_MENU_VS_ENTRIES;
        moved = TRUE;
    }
    if (moved != FALSE)
    {
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        ndsMenuShellRefreshVs();
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT) != FALSE)
    {
        ndsMenuShellVsAdjust(-1);
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_RIGHT) != FALSE)
    {
        ndsMenuShellVsAdjust(1);
    }

    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        if (sMenuVsCursor == NDS_MENU_VS_START)
        {
            ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
            ndsMenuShellVsSaveRules();
            /* VS START goes to the CHARACTER SELECT, which is where it goes in
             * the source too (mnvsmode.c's VS START leads to nSCKindPlayersVS,
             * and the imported scene's own transition probe asserts exactly
             * that pair). P2-1d sent it straight to the battle because there
             * was no character select yet; P2-1e is that screen, and it is now
             * the only route from this menu into a match.
             *
             * The walk spends its hop here when it is armed, so the P2-1b loop
             * budget still counts TWO hops a loop -- Results -> VS Mode and
             * VS Mode -> here. The CSS -> battle leg is the screen's own
             * transition and deliberately spends no hop, which is what keeps
             * that budget the same shape as when P2-1b defined it. With the
             * walk off this returns FALSE and the screen makes the transition
             * itself; both arms request the same scene. */
            if (ndsSceneWalkAdvance((u32)nSCKindPlayersVS) == FALSE)
            {
                ndsMenuShellGoto((u32)nSCKindPlayersVS);
            }
            else
            {
                u32 slot = gNdsMenuShellTransitionCount %
                           NDS_MENU_SHELL_RING;

                gNdsMenuShellTransitionRing[slot] =
                    ((sMenuScreen & 0xffu) << 8) |
                    ((u32)nSCKindPlayersVS & 0xffu);
                gNdsMenuShellTransitionCount++;
                sMenuNextScene = 0xffffffffu;
                sMenuLeaving = TRUE;
            }
            return;
        }
        /* VS OPTIONS: present, in the cursor cycle, and inert. */
        ndsUiKitSfx(NDS_UI_KIT_SFX_BACK);
        gNdsMenuShellDeniedCount++;
    }
    else if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellVsSaveRules();
        ndsMenuShellGoto((u32)nSCKindModeSelect);
    }
}

/* --- Screen: VS character select (P2-1e) ---------------------------------
 *
 * mn/mnplayers/mnplayersvs.c. A pointer screen, not a row screen: a hand
 * cursor roams the frame, picks a player's TOKEN up off its portrait, carries
 * it, and drops it on the fighter that player will use. Everything below is
 * transcribed from that file; what is NOT transcribed is named where it is
 * dropped, and there is exactly one mechanism substitution, disclosed here:
 *
 * THE STICK. mnPlayersVSAdjustCursor moves the cursor by
 * `stick_range / 20` per frame with a deadzone of 8 (mnplayersvs.c:3090/:3113).
 * The DS has no analog stick, and the port already fixed the exchange rate:
 * `ndsControllerMapPad` gives a held D-pad direction `stick_range = +-80`
 * (src/port/controller_backend.c:60). 80/20 is 4, so a held direction moves the
 * cursor at exactly the speed a fully deflected N64 stick moves it, and the
 * only expression the DS loses is partial deflection. That is the port's own
 * established mapping composed with the source's own divisor, not a number
 * chosen here.
 *
 * THE FRAME. Every hit test, clamp and layout constant below is in the
 * SOURCE's 320x240 frame, unaltered -- the portrait pitch of 45, the row bands
 * (35,79) and (78,122), the 69 px panel pitch, the button rectangles at
 * y 127..145 and y 197..216, the BACK box at x 244..292. The DS screen is
 * exactly 0.8 of that frame on BOTH axes (256/320 = 192/240), so drawing is
 * one multiply: NDS_CSS_DS(v) = v * 4 / 5. Keeping the logic in source units
 * means no constant here had to be re-derived, and a probe reads back the same
 * numbers mnplayersvs.c is written in.
 *
 * WHAT IS LOCKED. The source gates a portrait on `gSCManagerBackupData
 * .fighter_mask` (mnplayersvs.c:296), which the harness sets to ALL because
 * this build's save data is a fully unlocked cart. The gate this build needs is
 * a different one with the same mechanism: which fighters EXIST. Ten of the
 * twelve are P2-3, so the mask is Mario|Fox and the other ten portraits draw
 * the source's own question-mark plate -- the thing it draws for a locked
 * fighter (mnplayersvs.c:374). Same bitmask over fkind, different bound.
 *
 * DELIBERATE NARROWINGS, each a plan non-goal rather than an omission:
 *   - TEAMS ARE NO LONGER A NARROWING. P2-2 adds the source RED/BLUE/GREEN
 *     selector on all four panels, team gate palettes, team costume/shade
 *     updates and READY same-team rejection. Single-console still has only one
 *     human cursor; additional human cursors belong to P3 wireless.
 *   - FREE-FOR-ALL COSTUME BUTTONS. C-buttons pick alternate costumes
 *     (mnplayersvs.c:3372); the DS pad has no C-buttons and P2-3 owns alternate
 *     costume input. Team costumes are not optional presentation: P2-2 applies
 *     the source's team costume IDs automatically when Team Battle is active.
 *   - THE TIME/STOCK ARROWS at the top of the source's CSS duplicate the VS
 *     menu P2-1d already transcribed, so the rules stay that screen's.
 *   - THE 5-MINUTE IDLE RETURN (mnplayersvs.c:4470) is attract behaviour and
 *     belongs to P2-7, exactly as it does on the mode select.
 *   - THE RECALL TOSS. mnPlayersVSPuckAdjustRecall arcs the token back over 11
 *     frames and re-grabs it at tic 11 (mnplayersvs.c:3927). The END STATE is
 *     transcribed -- the token is back in the cursor's hand -- and the arc is
 *     not; it is presentation, and this screen has no SObj velocity model.
 *   - STAGE SELECT is no longer a narrowing: P2-1f landed the screen, so the
 *     ready START takes the source's own `is_stage_select` branch to
 *     `nSCKindMaps` (mnplayersvs.c:4497). The branch's OTHER arm -- randomise
 *     the ground here and go straight to the battle -- is still not the
 *     source's, because eight of the nine grounds are P2-4; it is unreachable
 *     in every configuration this build ships. */

/* Source frame -> DS pixels. 256/320 == 192/240 == 4/5, exactly. */
#define NDS_CSS_DS(v) (((v) * 4) / 5)

#define NDS_CSS_SLOTS 4
#define NDS_CSS_PORTRAITS 12

/* mnPlayersVSAdjustCursor's clamps and its full-deflection step. */
#define NDS_CSS_CURSOR_X_MIN 0
#define NDS_CSS_CURSOR_X_MAX 280
#define NDS_CSS_CURSOR_Y_MIN 10
#define NDS_CSS_CURSOR_Y_MAX 205
#define NDS_CSS_CURSOR_STEP 4

/* nMNPlayersCursorStatus*, in the order mnPlayersVSUpdateCursor's own
 * cursor_offsets[] indexes them (mnplayersvs.c:1723). */
#define NDS_CSS_STATUS_POINTER 0u
#define NDS_CSS_STATUS_GRAB 1u
#define NDS_CSS_STATUS_HOVER 2u

/* mnPlayersVSMakeCursor's player-0 seat, mnplayersvs.c:3684. */
#define NDS_CSS_CURSOR_HOME_X 40
#define NDS_CSS_CURSOR_HOME_Y 170
/* mnPlayersVSMakePuck's parking spot for a slot with no fighter yet. */
#define NDS_CSS_PUCK_HOME_X 51
#define NDS_CSS_PUCK_HOME_Y 161
/* The offset a carried token keeps from the cursor, mnplayersvs.c:3529. */
#define NDS_CSS_PUCK_CARRY_DX 11
#define NDS_CSS_PUCK_CARRY_DY (-14)
/* mnPlayersVSSelectFighter's re-grab lockout, mnplayersvs.c:2837. */
#define NDS_CSS_REGRAB_TICS 30
/* mnPlayersVSFuncRun's ready-START delay and its one-second input guard. */
#define NDS_CSS_START_WAIT 30
#define NDS_CSS_START_ARM_TICS 60
/* mnPlayersVSDetectBack: B held for forty tics leaves, mnplayersvs.c:3247. */
#define NDS_CSS_BACK_HOLD_TICS 40
/* mnPlayersVSReadyProcUpdate: a 40-tic cycle, lit for the first 30. */
#define NDS_CSS_READY_BLINK 40
#define NDS_CSS_READY_LIT 30

/* THE CUES, by the source's own REGION_US FGM ids. Derived by parsing
 * gm/gmsound.h's gmFGMVoiceID enum with REGION_US honoured and cross-checked
 * against every id this tree already pins -- Escape 11, GuardOn 13, FoxLanding
 * 74, MarioLanding 77, UnkGrind4 85, AltitudeWarn 153, DeadExplodeL 154,
 * MenuSelect 158, MenuScroll1 163, MenuScroll2 164, MenuDenied 165,
 * TitlePressStart 157, GamePause 278 -- all thirteen landing where the tree
 * already has them before any new id was trusted.
 *
 * FIVE OF THESE ARE IN THE FGM PACK AND THREE ARE NOT, which the miss ring
 * proves rather than this comment asserting it: 164/165 came in with P2-1c-1,
 * 157 with P2-1d-1, and 486/499 (the Fox and Mario announcer names) and 618
 * (the crowd cheer) were already packed for the Results sequence. 121, 127 and
 * 167 -- the two dash sounds the CSS reuses as its grab/announce whooshes and
 * the player-slot whoosh -- are NOT packed, and neither is 512
 * (FREE-FOR-ALL). Row P2-1e-1 renders them; this seam asks with the real ids so
 * the gap is measured. */
#define NDS_CSS_FGM_ANNOUNCE_WHOOSH 121u /* nSYAudioFGMMarioDash        */
#define NDS_CSS_FGM_GRAB 127u            /* nSYAudioFGMSamusDash        */
#define NDS_CSS_FGM_PRESS_START 157u     /* nSYAudioFGMTitlePressStart  */
#define NDS_CSS_FGM_SCROLL2 164u         /* nSYAudioFGMMenuScroll2      */
#define NDS_CSS_FGM_DENIED 165u          /* nSYAudioFGMMenuDenied       */
#define NDS_CSS_FGM_SLOT_WHOOSH 167u     /* nSYAudioFGMPlayerSlotWhoosh */
#define NDS_CSS_VOICE_FREE_FOR_ALL 512u  /* nSYAudioVoiceAnnounceFreeForAll */
#define NDS_CSS_VOICE_CHEER 618u         /* nSYAudioVoicePublicCheer    */
/* nSYAudioBGMBattleSelect, mnplayersvs.c:4899. Not one of the five tracks the
 * BGM pack carries (0/12/16/22/44), so ndsAudioBgmPlay counts it in
 * gNdsAudioBgmUnsupportedTrackCount -- the same measured gap, on the BGM side.
 * P2-1e-1 renders it. */
#define NDS_CSS_BGM_BATTLE_SELECT 10

/* mnPlayersVSAnnounceFighter's own table, indexed by fkind (mnplayersvs.c:2547
 * -- twelve entries, transcribed whole rather than trimmed to the two fighters
 * that exist, because it is the SOURCE's array and trimming it would make P2-3
 * edit this file again for no benefit). */
static const u16 kNdsCssAnnounceVoice[NDS_CSS_PORTRAITS] = {
    499u, 486u, 483u, 513u, 498u, 497u,
    535u, 485u, 496u, 507u, 508u, 501u
};

/* mnPlayersVSGetFighterKind: which fighter each of the twelve portrait cells
 * holds (mnplayersvs.c:2120). Cells 0-5 are the top row, 6-11 the bottom. */
static const u8 kNdsCssPortraitFighter[NDS_CSS_PORTRAITS] = {
    (u8)nFTKindLuigi, (u8)nFTKindMario, (u8)nFTKindDonkey,
    (u8)nFTKindLink, (u8)nFTKindSamus, (u8)nFTKindCaptain,
    (u8)nFTKindNess, (u8)nFTKindYoshi, (u8)nFTKindKirby,
    (u8)nFTKindFox, (u8)nFTKindPikachu, (u8)nFTKindPurin
};

/* mnPlayersVSGetPortrait, the inverse (mnplayersvs.c:2168). */
static const u8 kNdsCssFighterPortrait[NDS_CSS_PORTRAITS] = {
    1u, 9u, 2u, 4u, 0u, 3u, 7u, 5u, 8u, 10u, 11u, 6u
};

/* Which fighters this build HAS. Same shape as the source's fighter_mask. */
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox))

static u8 sCssPkind[NDS_CSS_SLOTS];
static u8 sCssFkind[NDS_CSS_SLOTS];
static u8 sCssLevel[NDS_CSS_SLOTS];
static u8 sCssTeam[NDS_CSS_SLOTS];
static u8 sCssSelected[NDS_CSS_SLOTS]; /* is_fighter_selected */
static s16 sCssPuckX[NDS_CSS_SLOTS];
static s16 sCssPuckY[NDS_CSS_SLOTS];
static s32 sCssCursorX;
static s32 sCssCursorY;
static u32 sCssStatus;
static s32 sCssHeld;      /* slot whose token the cursor carries, -1 = none */
static u32 sCssRegrabTic;
static u32 sCssBackTics;
/* The FFA/Team-Battle mode, seeded from the transfer state on CSS entry the way
 * the source seeds sMNPlayersVSIsTeamBattle (mnplayersvs.c:4679), flipped by
 * the mode-label toggle, and written back through the descriptor on commit.
 * P2-2 also makes sCssTeam[] live through the source RED/BLUE/GREEN controls. */
static u8 sCssIsTeamBattle;
/* P2-1N (3): the shutter, in the source's own units. door_offset slides
 * 2/tic between 0 (open — halves parked at the panel edges, hidden by the
 * frame) and 41 (shut — halves meeting at +22/+47, which is exactly the
 * baked NA gate). mnPlayersVSShutterProcUpdate/mnPlayersVSUpdateShutter. */
static u8 sCssDoorOffset[NDS_CSS_SLOTS];
static u32 sCssStartWait;
static u32 sCssReadyBlink;
static u32 sCssReadyShown;
/* mnPlayersVSMakePortraitFlash owns one independently-timed flash GObj per
 * player.  The GObj's position is fixed when it is created, so keep the
 * Mario/Fox content id separately from sCssFkind[]: a later kind/holder change
 * must not drag an already-live flash to a different portrait. */
static u8 sCssFlashRemain[NDS_CSS_SLOTS];
static u8 sCssFlashVisible[NDS_CSS_SLOTS];
static u8 sCssFlashKind[NDS_CSS_SLOTS];
static u8 sCssFlashShown[2];

/* One cursor: the DS has one keypad, so exactly one player has a controller.
 * mnPlayersVSUpdateControllerOrders would report orders[0] = 0 and -1 for the
 * rest, and every branch below that reads a controller order is written from
 * that fact rather than from a general N-cursor model. */
#define NDS_CSS_CURSOR_SLOT 0

/* --- Audio seam ---------------------------------------------------------- */

static void ndsMenuShellCssCue(u32 id)
{
    gNdsMenuShellCssCueCount++;
    gNdsMenuShellCssCueLastId = id;
    /* Unguarded, like the kit's own seam: NDS_IMPORT_BATTLESHIP_AUDIO_FGM is an
     * unconditional Makefile override, so this symbol always exists. */
    (void)ndsAudioFgmPlay((u16)id);
}

/* P2-1N (4): the mode announcer voice, shared by the source's entry tail and
 * mode-toggle handler — FREE FOR ALL (512) or TEAM BATTLE (526). */
static void ndsMenuShellCssAnnounceMode(void)
{
    ndsMenuShellCssCue((sCssIsTeamBattle != FALSE) ? 526u : 512u);
}

/* mnPlayersVSAnnounceFighter, mnplayersvs.c:2545: the whoosh then the name. */
static void ndsMenuShellCssAnnounce(u32 slot)
{
    u32 fkind = (u32)sCssFkind[slot];

    if (fkind >= NDS_CSS_PORTRAITS)
    {
        return;
    }
    ndsMenuShellCssCue(NDS_CSS_FGM_ANNOUNCE_WHOOSH);
    ndsMenuShellCssCue((u32)kNdsCssAnnounceVoice[fkind]);
    gNdsMenuShellCssAnnounceCount++;
}

/* --- Source geometry ----------------------------------------------------- */

static u32 ndsMenuShellCssFighterLocked(u32 fkind)
{
    if (fkind >= NDS_CSS_PORTRAITS)
    {
        return TRUE;
    }
    return ((NDS_CSS_FIGHTER_MASK & (1u << fkind)) != 0u) ? FALSE : TRUE;
}

/* mnPlayersVSMakePortraitShadow's own placement, mnplayersvs.c:2412. */
static s32 ndsMenuShellCssPortraitX(u32 portrait)
{
    return (s32)(((portrait >= 6u) ? (portrait - 6u) : portrait) * 45u) + 25;
}

static s32 ndsMenuShellCssPortraitY(u32 portrait)
{
    return (s32)(((portrait >= 6u) ? 1u : 0u) * 43u) + 36;
}

/* mnPlayersVSCenterPuckInPortrait, mnplayersvs.c:3454. */
static void ndsMenuShellCssCenterPuck(u32 slot, u32 fkind)
{
    u32 portrait;

    if (fkind >= NDS_CSS_PORTRAITS)
    {
        sCssPuckX[slot] = (s16)NDS_CSS_PUCK_HOME_X;
        sCssPuckY[slot] = (s16)NDS_CSS_PUCK_HOME_Y;
        return;
    }
    portrait = (u32)kNdsCssFighterPortrait[fkind];
    if (portrait >= 6u)
    {
        sCssPuckX[slot] = (s16)((portrait * 45u) - (6u * 45u) + 36u);
        sCssPuckY[slot] = (s16)89;
    }
    else
    {
        sCssPuckX[slot] = (s16)((portrait * 45u) + 36u);
        sCssPuckY[slot] = (s16)46;
    }
}

/* mnPlayersVSGetPuckFighterKind, mnplayersvs.c:3001 -- which fighter the
 * token's CENTRE is over, or nFTKindNull. */
static u32 ndsMenuShellCssPuckFighterKind(u32 slot)
{
    s32 x = (s32)sCssPuckX[slot] + 13;
    s32 y = (s32)sCssPuckY[slot] + 12;
    u32 fkind;

    if ((x <= 24) || (x >= 295))
    {
        return (u32)nFTKindNull;
    }
    if ((y > 35) && (y < 79))
    {
        fkind = (u32)kNdsCssPortraitFighter[(x - 25) / 45];
    }
    else if ((y > 78) && (y < 122))
    {
        fkind = (u32)kNdsCssPortraitFighter[((x - 25) / 45) + 6];
    }
    else
    {
        return (u32)nFTKindNull;
    }
    return (ndsMenuShellCssFighterLocked(fkind) != FALSE) ?
        (u32)nFTKindNull : fkind;
}

/* mnPlayersVSCheckPuckInRange, mnplayersvs.c:2157 -- the cursor's puck hotspot
 * is (+25, +3) and a token is 26x24. */
static u32 ndsMenuShellCssPuckInRange(u32 slot)
{
    s32 x = sCssCursorX + 25;
    s32 y = sCssCursorY + 3;

    if ((x < (s32)sCssPuckX[slot]) || (x > ((s32)sCssPuckX[slot] + 26)))
    {
        return FALSE;
    }
    if ((y < (s32)sCssPuckY[slot]) || (y > ((s32)sCssPuckY[slot] + 24)))
    {
        return FALSE;
    }
    return TRUE;
}

/* The BUTTON hotspot is (+20, +3), a different offset from the token one --
 * mnPlayersVSCheckPlayerKindSelectInRange:2138 against :2163. */
static u32 ndsMenuShellCssBoxHit(s32 x0, s32 x1, s32 y0, s32 y1)
{
    s32 x = sCssCursorX + 20;
    s32 y = sCssCursorY + 3;

    return ((x >= x0) && (x <= x1) && (y >= y0) && (y <= y1)) ? TRUE : FALSE;
}

/* --- Drawing ------------------------------------------------------------- */

/* Text slots. P2-1k gave the four per-slot fighter names back: they are source
 * art baked into the gate surface now, so this screen holds four of the eight
 * and the other four are free for the next element that needs one. */
#define NDS_CSS_SLOT_MODE 0u
#define NDS_CSS_SLOT_BACK 1u
#define NDS_CSS_SLOT_READY 2u

/* Sprite slots, in DEPTH order: lower OAM ids draw in front. The source's
 * REGION_US PRESS/START GObj is display link 28 (player-kind camera priority
 * 50), not the link-38 READY banner GObj. It therefore stays behind cursor 20
 * and puck 25 just like the other link-28 panel art. The banner itself is
 * reinforced on foreground BG3 below so its separate camera-10 depth is kept. */
#define NDS_CSS_SPRITE_CURSOR_TAG 0u
#define NDS_CSS_SPRITE_CURSOR 1u
#define NDS_CSS_SPRITE_PUCK0 2u
#define NDS_CSS_SPRITE_KIND0 6u
#define NDS_CSS_SPRITE_LEVEL0 10u
#define NDS_CSS_SPRITE_DIGIT0 14u
/* P2-1j, owner finding (e): the player panel's own art. The tag is the
 * mnPlayersVSMakePlayerKind sprite (1P / CP, :2003), the colon and the two
 * blinking arrows are mnPlayersVSMakeHandicapLevel's and
 * mnPlayersVSArrowThreadUpdate's (:2696/:2626). All four are behind the
 * tokens and the cursor in id order, which is the depth the source draws them
 * at (display 35 under the puck's 37). */
#define NDS_CSS_SPRITE_READY_PRESS 18u
#define NDS_CSS_SPRITE_READY_START 19u
#define NDS_CSS_SPRITE_TAG0 20u
#define NDS_CSS_SPRITE_COLON0 24u
#define NDS_CSS_SPRITE_ARROWL0 28u
#define NDS_CSS_SPRITE_ARROWR0 32u

/* mnPlayersVSArrowThreadUpdate's own blink (10 tics on, 10 off) and the two
 * values at which an arrow is EJECTED rather than blinked (:2643/:2660). */
#define NDS_CSS_ARROW_BLINK 10u
#define NDS_CSS_LEVEL_MIN 1u
#define NDS_CSS_LEVEL_MAX 9u

/* mnPlayersVSMakeLabels' own colours, mnplayersvs.c:1391. */
#define NDS_CSS_RGB_MODE 0x00e3ac04u
/* mnPlayersVSMakeReady's ready-text primitive, mnplayersvs.c:4128. */

static u32 ndsMenuShellCssKindImage(u32 pkind)
{
    /* LABEL_HMN/CP/NA are baked as three consecutive entries in nFTPlayerKind
     * order, which the generator asserts (check_kind_label_block), so this is
     * the source's own offsets[pkind] indexing (mnplayersvs.c:934). */
    if (pkind > (u32)nFTPlayerKindNot)
    {
        pkind = (u32)nFTPlayerKindNot;
    }
    return NDS_MN_UI_KIT_IMAGE_LABEL_HMN + pkind;
}

/* P2-1j, owner finding (e), EXTENDED BY P2-1k (a)/(b). THE PANEL IS ONE CARD
 * AND EIGHT PALETTES in the source -- `mnPlayersVSSetGateLUT`
 * (mnplayersvs.c:900) swaps the CI4 gate card's TLUT between `GateMan<N>PLUT`
 * and `GateCom<N>PLUT` -- and an EMPTY slot additionally has its two shutter
 * doors closed over it (door_offset 41, :640). A direct-colour DS bitmap has no
 * palette to swap, so the bake resolves each reachable result per slot and the
 * runtime blits the one the slot's state names.
 *
 * P2-1k adds the two sprites `mnPlayersVSMakeGate` builds that this shell had
 * never drawn -- the owner's "the preview boxes aren't rendering the fighter".
 * `mnPlayersVSMakeNameAndEmblem` (:578) puts the SERIES EMBLEM at (p*69+24,
 * 143), tinted 0x1E for a human slot and 0x44 otherwise, and the FIGHTER NAME
 * at (p*69+22, 201). They are baked INTO the card rather than drawn as OBJ
 * cells because one fighter's pair needs 10,240 B of main OBJ against the
 * 3,456 B P2-1j left free, and because folding them in keeps every state of a
 * slot exactly one overwriting blit.
 *
 * NINE STATES, and the third fighter state is the source's, not an invention:
 * the NAME and the CP LEVEL row occupy the SAME row (y 201), and
 * `mnPlayersVSUpdateHandicapLevel` opens by hiding the name (:2788) while
 * `mnPlayersVSHandicapLevelProcUpdate` destroys the level row the moment the
 * slot stops being a settled fighter (:2689). So a settled CPU shows CP LEVEL
 * (drawn as OBJ over this surface) and a CPU whose token is in the cursor's
 * hand shows its name again. */
#define NDS_CSS_GATE_NA 0u
#define NDS_CSS_GATE_MAN 1u
#define NDS_CSS_GATE_COM 2u
#define NDS_CSS_GATE_MAN_F0 3u   /* + fighter index (Mario 0, Fox 1) */
#define NDS_CSS_GATE_COM_F0 5u
#define NDS_CSS_GATE_HOLD_F0 7u
#define NDS_CSS_GATE_STATES 9u

static const u8 kNdsCssGateSurface[NDS_CSS_SLOTS][NDS_CSS_GATE_STATES] = {
    { (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_NA,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_MAN,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_COM,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_MAN_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_MAN_FOX,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_COM_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_COM_FOX,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_HOLD_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_HOLD_FOX },
    { (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_1_NA,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_1_MAN,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_1_COM,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_1_MAN_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_1_MAN_FOX,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_1_COM_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_1_COM_FOX,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_1_HOLD_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_1_HOLD_FOX },
    { (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_2_NA,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_2_MAN,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_2_COM,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_2_MAN_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_2_MAN_FOX,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_2_COM_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_2_COM_FOX,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_2_HOLD_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_2_HOLD_FOX },
    { (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_NA,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_MAN,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_COM,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_MAN_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_MAN_FOX,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_COM_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_COM_FOX,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_HOLD_MARIO,
      (u8)NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_HOLD_FOX }
};

/* P2-2a: the generated Team-Battle panel variants are intentionally contiguous
 * in [team][player][state] order. BattleShip maps team ids Red/Blue/Green to
 * gate color ids 0/1/3. The RED/BLUE/GREEN selector itself is a SEPARATE source
 * display object on DL 34, in front of the fighter camera on DL 33, so its
 * twelve exact baked rasters live in a separate foreground block below. */
#define NDS_CSS_TEAM_COUNT 3u
#define NDS_CSS_TEAM_GATE_STRIDE (NDS_CSS_SLOTS * NDS_CSS_GATE_STATES)
_Static_assert(nSCBattleTeamIDRed == 0 && nSCBattleTeamIDBlue == 1 &&
                   nSCBattleTeamIDGreen == 2,
               "CSS team surface arithmetic follows BattleShip team ids");
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_BLUE_0_NA ==
                   NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_RED_0_NA +
                       NDS_CSS_TEAM_GATE_STRIDE,
               "team gate surfaces must stay contiguous by team");
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_GREEN_0_NA ==
                   NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_RED_0_NA +
                       (2u * NDS_CSS_TEAM_GATE_STRIDE),
               "team gate surfaces must stay contiguous by team");
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_GREEN_3_HOLD_FOX ==
                   NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_RED_0_NA +
                       (NDS_CSS_TEAM_COUNT * NDS_CSS_TEAM_GATE_STRIDE) - 1u,
               "team gate surface block must contain 3x4x9 variants");

#define NDS_CSS_TEAM_SELECT_STRIDE NDS_CSS_SLOTS
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_TEAM_SELECT_BLUE_0 ==
                   NDS_MN_UI_KIT_SURFACE_CSS_TEAM_SELECT_RED_0 +
                       NDS_CSS_TEAM_SELECT_STRIDE,
               "team selector surfaces must stay contiguous by team");
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_TEAM_SELECT_GREEN_3 ==
                   NDS_MN_UI_KIT_SURFACE_CSS_TEAM_SELECT_RED_0 +
                       (NDS_CSS_TEAM_COUNT * NDS_CSS_TEAM_SELECT_STRIDE) - 1u,
               "team selector surface block must contain 3x4 variants");

static u8 sCssPanelSurface[NDS_CSS_SLOTS];
static u32 sCssArrowsShown;

static u32 ndsMenuShellCssGateState(u32 slot);

static u8 ndsMenuShellCssGateSurfaceForState(u32 slot, u32 state)
{
    u32 team;

    if (sCssIsTeamBattle == FALSE)
    {
        return kNdsCssGateSurface[slot][state];
    }
    team = (u32)sCssTeam[slot];
    if (team >= NDS_CSS_TEAM_COUNT)
    {
        team = (u32)nSCBattleTeamIDRed;
    }
    return (u8)(NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_RED_0_NA +
                (team * NDS_CSS_TEAM_GATE_STRIDE) +
                (slot * NDS_CSS_GATE_STATES) + state);
}

static u8 ndsMenuShellCssGateSurface(u32 slot)
{
    return ndsMenuShellCssGateSurfaceForState(
        slot, ndsMenuShellCssGateState(slot));
}

static s32 ndsMenuShellCssRoundSource(s32 value)
{
    return (value * 4 + 2) / 5;
}

static u8 ndsMenuShellCssTeamSelectSurface(u32 slot)
{
    u32 team = (u32)sCssTeam[slot];

    if (team >= NDS_CSS_TEAM_COUNT)
    {
        team = (u32)nSCBattleTeamIDRed;
    }
    return (u8)(NDS_MN_UI_KIT_SURFACE_CSS_TEAM_SELECT_RED_0 +
                (team * NDS_CSS_TEAM_SELECT_STRIDE) + slot);
}

static void ndsMenuShellCssClearTeamSelect(u32 slot)
{
    s32 source_x = (s32)(slot * 69u) + 30;

    /* The source sprite sits at x+34,y=131 and its hit rectangle ends at
     * x+58,y=141. Clear a deliberately slightly wider transparent BG3 pocket
     * so antialiased/keyed edge pixels from the previous team cannot survive a
     * color change. No other CSS foreground owner occupies this rectangle. */
    ndsUiKitClearForegroundRect(ndsMenuShellCssRoundSource(source_x),
                                ndsMenuShellCssRoundSource(128),
                                (u32)ndsMenuShellCssRoundSource(34),
                                (u32)ndsMenuShellCssRoundSource(16));
}

static void ndsMenuShellCssDrawTeamSelect(u32 slot)
{
    u8 surface;

    ndsMenuShellCssClearTeamSelect(slot);
    if (sCssIsTeamBattle == FALSE)
    {
        return;
    }
    surface = ndsMenuShellCssTeamSelectSurface(slot);
    if (ndsUiKitBlitForegroundSurfaces(&surface, 1u) != FALSE)
    {
        gNdsMenuShellCssPanelBlitCount++;
    }
}

static void ndsMenuShellCssSyncTeamSelectAll(void)
{
    u32 slot;

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        ndsMenuShellCssDrawTeamSelect(slot);
    }
}

/* Which of the nine the slot is in, transcribed from the two source predicates
 * above: the GObj is hidden entirely for an empty slot
 * (`mnPlayersVSUpdateNameAndEmblem`, :2401) and draws nothing when the fighter
 * is Null (`mnPlayersVSMakeNameAndEmblem`, :609); a settled CPU has its name
 * hidden by the level row and an unsettled one has it back. */
static u32 ndsMenuShellCssGateState(u32 slot)
{
    u32 fkind = (u32)sCssFkind[slot];
    u32 fighter;

    if (sCssPkind[slot] == (u8)nFTPlayerKindNot)
    {
        return NDS_CSS_GATE_NA;
    }
    if (fkind == (u32)nFTKindMario)
    {
        fighter = 0u;
    }
    else if (fkind == (u32)nFTKindFox)
    {
        fighter = 1u;
    }
    else
    {
        return (sCssPkind[slot] == (u8)nFTPlayerKindCom) ?
            NDS_CSS_GATE_COM : NDS_CSS_GATE_MAN;
    }
    if (sCssPkind[slot] != (u8)nFTPlayerKindCom)
    {
        return NDS_CSS_GATE_MAN_F0 + fighter;
    }
    return ((sCssSelected[slot] != 0u) ? NDS_CSS_GATE_COM_F0 :
                                         NDS_CSS_GATE_HOLD_F0) + fighter;
}

/* P2-1N (3): mnPlayersVSShutterProcUpdate + mnPlayersVSUpdateShutter,
 * transcribed. door_offset walks 2/tic toward 41 (shut) for an empty slot and
 * 0 (open) otherwise; the close cue fires on arrival at 41
 * (nSYAudioFGMPlayerSlotClose = 166 — asks; the miss ring reports it until a
 * pack row lands). Mid-slide, the slot's current gate surface re-blits as the
 * underlay and both card halves draw from the cached CSS_DOORS strip at their
 * slid positions, clipped to the gate box exactly as the source's frame art
 * masks the pocketed part. Terminal frames land the baked gate, so steady
 * state costs nothing. */
static void ndsMenuShellCssStepDoors(void)
{
    u32 i;

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        s32 start = (s32)(i * 69u);
        u32 target = (sCssPkind[i] == (u8)nFTPlayerKindNot) ? 41u : 0u;
        u32 offset = (u32)sCssDoorOffset[i];
        u8 blit;

        if (offset == target)
        {
            continue;
        }
        if (offset < target)
        {
            offset += 2u;
            if (offset >= 41u)
            {
                offset = 41u;
                ndsMenuShellCssCue(166u);
            }
        }
        else
        {
            offset = (offset >= 2u) ? (offset - 2u) : 0u;
        }
        sCssDoorOffset[i] = (u8)offset;
        if ((offset == 41u) || (offset == 0u))
        {
            blit = ndsMenuShellCssGateSurface(i);
            if (ndsUiKitBlitSurfaces(&blit, 1u) != FALSE)
            {
                sCssPanelSurface[i] = blit;
                gNdsMenuShellCssPanelBlitCount++;
            }
            continue;
        }
        blit = sCssPanelSurface[i];
        if (ndsUiKitBlitSurfaces(&blit, 1u) != FALSE)
        {
            gNdsMenuShellCssPanelBlitCount++;
        }
        {
            /* THE BAKE'S OWN ROUNDING, not NDS_CSS_DS: frame_pos rounds half
             * up while the plain macro truncates, and the two disagree by a
             * pixel exactly where it shows — the right half drew 32 of its
             * 33 columns and the pair sat one row above the gate (owner
             * screenshot, 2026-08-19). One rule for every door number. */
            #define NDS_CSS_DSR(v) (((v) * 4 + 2) / 5)
            s32 clip0 = NDS_CSS_DSR(start + 22);
            s32 clip1 = NDS_CSS_DSR(start + 88);
            s32 dy = NDS_CSS_DSR(126);
            /* Both source door sprites are 41x92. Closed, the left occupies
             * source x=22..62 and the right x=47..87, overlapping by 16 px;
             * their alpha masks form the source's zigzag seam. The cache keeps
             * both complete masks side-by-side (41+41) and these destination
             * positions recreate that overlap. Cutting the cache at x=25 was
             * the old straight-seam defect: it discarded those 16 left-door
             * pixels before the two source masks could interlock. */
            u32 split = (u32)NDS_CSS_DSR(41);
            u32 strip_w = (u32)NDS_CSS_DSR(41 + 41);

            (void)ndsUiKitDrawCachedSub(0u, split,
                                        NDS_CSS_DSR(start - 19 + (s32)offset),
                                        dy, clip0, clip1);
            (void)ndsUiKitDrawCachedSub(split, strip_w - split,
                                        NDS_CSS_DSR(start + 88 - (s32)offset),
                                        dy, clip0, clip1);
            gNdsMenuShellCssDoorSlideFrames++;
            #undef NDS_CSS_DSR
        }
    }
}

/* Only the panels whose kind changed, at most `budget` of them, in one call.
 * The VS menu's rule, for the VS menu's measured reason: a panel is 7,738 B of
 * NitroFS and the character select's own worst frame already sits at 71% of
 * the 60 Hz budget before one is read, so the screen ENTRY (a load frame)
 * writes all four and everything after it writes one a frame. A kind change
 * only ever moves one slot anyway; the budget is what keeps that true when the
 * next row adds a reason for two. */
static void ndsMenuShellCssSyncPanels(u32 budget)
{
    u8 list[NDS_CSS_SLOTS];
    u8 wanted[NDS_CSS_SLOTS];
    u32 index[NDS_CSS_SLOTS];
    u32 count = 0u;
    u32 i;

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        wanted[i] = ndsMenuShellCssGateSurface(i);
        /* P2-1N (3): while a slot is closing (pkind Not, doors mid-slide),
         * the card must stay visible UNDER the sliding halves — the shut NA
         * bake lands only when the doors arrive (the animator blits it and
         * cues the close). The open underlay for an emptying slot is its
         * COM-lut card with no fighter, which is exactly the COM state. */
        if ((sCssPkind[i] == (u8)nFTPlayerKindNot) &&
            (sCssDoorOffset[i] < 41u))
        {
            wanted[i] = ndsMenuShellCssGateSurfaceForState(
                i, NDS_CSS_GATE_COM);
        }
        if ((wanted[i] != sCssPanelSurface[i]) && (count < budget))
        {
            list[count] = wanted[i];
            index[count] = i;
            count++;
        }
    }
    if (count == 0u)
    {
        return;
    }
    if (ndsUiKitBlitSurfaces(list, count) == FALSE)
    {
        return;
    }
    for (i = 0u; i < count; i++)
    {
        sCssPanelSurface[index[i]] = list[i];
    }
    gNdsMenuShellCssPanelBlitCount += count;
}

/* The CPU-level arrows, mnPlayersVSArrowThreadUpdate (:2626). Drawn only for a
 * slot that shows the CP LEVEL row at all, dropped at the ends of the 1..9
 * range exactly as the source ejects the SObj there, and blinking on its own
 * 10-tic cycle. Split from the populate below because the blink is per frame
 * and the row's existence is not. */
static void ndsMenuShellCssDrawArrows(void)
{
    u32 i;

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        s32 panel = (s32)(i * 69u);
        u32 level = (u32)sCssLevel[i];
        u32 live = ((sCssPkind[i] == (u8)nFTPlayerKindCom) &&
                    (sCssSelected[i] != 0u) && (sCssArrowsShown != FALSE)) ?
            TRUE : FALSE;

        if ((live != FALSE) && (level > NDS_CSS_LEVEL_MIN))
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_ARROWL0 + i,
                              NDS_MN_UI_KIT_IMAGE_CSS_ARROW_L,
                              NDS_CSS_DS(panel + 25), NDS_CSS_DS(201));
        }
        else
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_ARROWL0 + i);
        }
        if ((live != FALSE) && (level < NDS_CSS_LEVEL_MAX))
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_ARROWR0 + i,
                              NDS_MN_UI_KIT_IMAGE_CSS_ARROW_R,
                              NDS_CSS_DS(panel + 79), NDS_CSS_DS(201));
        }
        else
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_ARROWR0 + i);
        }
    }
}

static void ndsMenuShellCssPopulate(void)
{
    u32 i;

    /* P2-1N (2)+(4): the mode label and BACK ship the source's own sprites
     * (`mnPlayersVSMakeLabels` at (27,24) with the toggle's FFA tint;
     * `llMNPlayersCommonBackButtonSprite` at (244,23)), blitted as baked
     * states over the base. The mode label is a two-state surface so the
     * FFA/Team toggle is a swap; BACK is static and blits once here. */
    {
        u8 entry_states[2];

        entry_states[0] = (sCssIsTeamBattle != FALSE) ?
            (u8)NDS_MN_UI_KIT_SURFACE_CSS_MODE_TEAM :
            (u8)NDS_MN_UI_KIT_SURFACE_CSS_MODE_FFA;
        entry_states[1] = (u8)NDS_MN_UI_KIT_SURFACE_CSS_BACK;
        if (ndsUiKitBlitSurfaces(entry_states, 2u) != FALSE)
        {
            gNdsMenuShellCssPanelBlitCount += 2u;
        }
    }

    /* P2-1L (5): EVERY portrait cell is backdrop art now. The old twelve OBJ
     * slots were dead after that migration and are no longer reserved; P2-1
     * closeout reuses the resulting OAM id space for source cursor/READY art. */

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        s32 panel = (s32)(i * 69u);

        /* The player-kind button, mnplayersvs.c:963. */
        ndsUiKitSetSprite(NDS_CSS_SPRITE_KIND0 + i,
                          ndsMenuShellCssKindImage((u32)sCssPkind[i]),
                          NDS_CSS_DS(panel + 64), NDS_CSS_DS(131));

        /* P2-1j (e): the panel's own player tag, mnPlayersVSMakePlayerKind
         * (:969) -- a CPU slot draws the shared CP art at `p*69+26` and a
         * human one its own `<N>P` art at `pos_x[p] + p*69 + 22`, both black
         * on the card at y 131. Team Battle does NOT remove this object; its
         * later DL-34 team selector simply draws over it. Keep the tag live and
         * let foreground BG3 reproduce that source ordering. */
        if (sCssPkind[i] == (u8)nFTPlayerKindCom)
        {
            (void)ndsUiKitSetSpriteBlend(
                NDS_CSS_SPRITE_TAG0 + i, NDS_MN_UI_KIT_IMAGE_PANEL_CP,
                NDS_CSS_DS(panel + 26), NDS_CSS_DS(131), 15u, 0u,
                (sCssIsTeamBattle != FALSE) ? 1u : 0u);
        }
        else if (sCssPkind[i] == (u8)nFTPlayerKindMan)
        {
            (void)ndsUiKitSetSpriteBlend(
                NDS_CSS_SPRITE_TAG0 + i, NDS_MN_UI_KIT_IMAGE_PANEL_1P,
                NDS_CSS_DS(panel + 30), NDS_CSS_DS(131), 15u, 0u,
                (sCssIsTeamBattle != FALSE) ? 1u : 0u);
        }
        else
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_TAG0 + i);
        }

        /* P2-1k (a)/(b): THE FIGHTER'S NAME AND SERIES EMBLEM ARE SOURCE ART
         * NOW, and they are not drawn here -- they are baked into the gate
         * surface `ndsMenuShellCssSyncPanels` blits, at the source's own sites
         * (emblem (p*69+24,143), name (p*69+22,201) -- mnplayersvs.c:614/:632),
         * because one fighter's pair is 10,240 B of OBJ against 3,456 B free.
         * The font-composed name P2-1e drew at (p*69+22,146) is deleted with
         * this row; that position was the EMBLEM's row, not the name's, so the
         * substitution was in the wrong place as well as the wrong medium. */

        /* CP LEVEL and its value, mnplayersvs.c:2762/:2790. The source shows
         * this row when the slot is a selected CPU, or when handicap is on for
         * the cursor's own human slot -- handicap is Off in every configuration
         * this build reaches (the descriptor's handicap_mode), so only the CPU
         * arm can run and only it is drawn. */
        if ((sCssPkind[i] == (u8)nFTPlayerKindCom) && (sCssSelected[i] != 0u))
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_LEVEL0 + i,
                              NDS_MN_UI_KIT_IMAGE_CP_LEVEL,
                              NDS_CSS_DS(panel + 34), NDS_CSS_DS(201));
            /* P2-1j (e): the colon between label and value is its own sprite
             * in the source too (llMNCommonColonSprite at `p*69+61`, y 202,
             * white -- mnplayersvs.c:2740), and it was simply absent. */
            ndsUiKitSetSprite(NDS_CSS_SPRITE_COLON0 + i,
                              NDS_MN_UI_KIT_IMAGE_COLON,
                              NDS_CSS_DS(panel + 61), NDS_CSS_DS(202));
            (void)ndsUiKitSetNumber(NDS_CSS_SPRITE_DIGIT0 + i, 1u,
                                    (s32)sCssLevel[i],
                                    NDS_CSS_DS(panel + 67) +
                                        NDS_UI_KIT_DIGIT_PITCH,
                                    NDS_CSS_DS(200));
        }
        else
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_LEVEL0 + i);
            ndsUiKitHideSprite(NDS_CSS_SPRITE_DIGIT0 + i);
            ndsUiKitHideSprite(NDS_CSS_SPRITE_COLON0 + i);
        }

        /* The token. mnPlayersVSUpdatePuckDisplay hides it for an empty slot
         * (mnplayersvs.c:2310) and draws the CP token for a CPU one. */
        if (sCssPkind[i] == (u8)nFTPlayerKindNot)
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_PUCK0 + i);
        }
        else
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_PUCK0 + i,
                              (sCssPkind[i] == (u8)nFTPlayerKindCom) ?
                                  NDS_MN_UI_KIT_IMAGE_PUCK_CP :
                                  NDS_MN_UI_KIT_IMAGE_PUCK_1P,
                              NDS_CSS_DS((s32)sCssPuckX[i]),
                              NDS_CSS_DS((s32)sCssPuckY[i]));
        }
    }
    ndsMenuShellCssDrawArrows();
}

/* Per frame, and deliberately separate from the populate above: the cursor and
 * a carried token move every frame while the screen's CONTENT changes only on
 * an action, so composing text every frame would show up as a climbing
 * gNdsUiKitTextComposeCount for a screen that is not changing. */
static void ndsMenuShellCssMove(void)
{
    u32 image = NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_POINT;
    s32 tag_dx = 7;
    s32 tag_dy = 15;
    u32 i;
    /* mnPlayersVSArrowThreadUpdate's 10-tic toggle, recomputed from the
     * screen's tic count for the same reason the VS menu's 30-tic one is. */
    u32 shown = (((sMenuTics / NDS_CSS_ARROW_BLINK) & 1u) == 0u) ? TRUE : FALSE;

    if (shown != sCssArrowsShown)
    {
        sCssArrowsShown = shown;
        ndsMenuShellCssDrawArrows();
    }

    if (sCssStatus == NDS_CSS_STATUS_GRAB)
    {
        image = NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_GRAB;
        tag_dx = 9;
        tag_dy = 10;
    }
    else if (sCssStatus == NDS_CSS_STATUS_HOVER)
    {
        image = NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_HOVER;
        tag_dx = 9;
        tag_dy = 15;
    }
    /* mnPlayersVSUpdateCursor appends the player's gradient tag after the hand
     * at offsets {7,15}/{9,10}/{9,15}.  Only the 1P cursor is reachable on a
     * single DS, and its source PRIM/ENV IA gradient is baked into this image. */
    ndsUiKitSetSprite(NDS_CSS_SPRITE_CURSOR_TAG,
                      NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_1P,
                      NDS_CSS_DS(sCssCursorX + tag_dx),
                      NDS_CSS_DS(sCssCursorY + tag_dy));
    ndsUiKitSetSprite(NDS_CSS_SPRITE_CURSOR, image, NDS_CSS_DS(sCssCursorX),
                      NDS_CSS_DS(sCssCursorY));
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        if (sCssPkind[i] != (u8)nFTPlayerKindNot)
        {
            ndsUiKitMoveSprite(NDS_CSS_SPRITE_PUCK0 + i,
                               NDS_CSS_DS((s32)sCssPuckX[i]),
                               NDS_CSS_DS((s32)sCssPuckY[i]));
        }
    }
}

/* --- Portrait flash ------------------------------------------------------- */

#define NDS_CSS_FLASH_TICS 16u
#define NDS_CSS_FLASH_KIND_NONE 0xffu
#define NDS_CSS_FLASH_KIND_MARIO 0u
#define NDS_CSS_FLASH_KIND_FOX 1u
#define NDS_CSS_FLASH_KIND_COUNT 2u

static u32 ndsMenuShellCssFlashKindFromFighter(u32 fkind)
{
    if (fkind == (u32)nFTKindMario)
    {
        return NDS_CSS_FLASH_KIND_MARIO;
    }
    if (fkind == (u32)nFTKindFox)
    {
        return NDS_CSS_FLASH_KIND_FOX;
    }
    return NDS_CSS_FLASH_KIND_NONE;
}

static u8 ndsMenuShellCssFlashSurface(u32 kind, u32 visible)
{
    u32 ready = (sCssReadyShown == 1u) ? 1u : 0u;

    if (kind == NDS_CSS_FLASH_KIND_MARIO)
    {
        if (ready != 0u)
        {
            return (visible != FALSE) ?
                (u8)NDS_MN_UI_KIT_SURFACE_CSS_FLASH_MARIO_ON_READY1 :
                (u8)NDS_MN_UI_KIT_SURFACE_CSS_FLASH_MARIO_OFF_READY1;
        }
        return (visible != FALSE) ?
            (u8)NDS_MN_UI_KIT_SURFACE_CSS_FLASH_MARIO_ON_READY0 :
            (u8)NDS_MN_UI_KIT_SURFACE_CSS_FLASH_MARIO_OFF_READY0;
    }
    if (ready != 0u)
    {
        return (visible != FALSE) ?
            (u8)NDS_MN_UI_KIT_SURFACE_CSS_FLASH_FOX_ON_READY1 :
            (u8)NDS_MN_UI_KIT_SURFACE_CSS_FLASH_FOX_OFF_READY1;
    }
    return (visible != FALSE) ?
        (u8)NDS_MN_UI_KIT_SURFACE_CSS_FLASH_FOX_ON_READY0 :
        (u8)NDS_MN_UI_KIT_SURFACE_CSS_FLASH_FOX_OFF_READY0;
}

static u32 ndsMenuShellCssFlashAggregate(u32 kind)
{
    u32 slot;

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        if ((sCssFlashRemain[slot] != 0u) &&
            (sCssFlashVisible[slot] != 0u) &&
            ((u32)sCssFlashKind[slot] == kind))
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsMenuShellCssDrawFlashKind(u32 kind, u32 visible)
{
    u8 surface;

    if (kind >= NDS_CSS_FLASH_KIND_COUNT)
    {
        return;
    }
    surface = ndsMenuShellCssFlashSurface(kind, visible);
    if (ndsUiKitBlitSurfaces(&surface, 1u) != FALSE)
    {
        gNdsMenuShellCssPanelBlitCount++;
    }
}

static void ndsMenuShellCssSyncFlashKind(u32 kind)
{
    u32 visible;

    if (kind >= NDS_CSS_FLASH_KIND_COUNT)
    {
        return;
    }
    visible = ndsMenuShellCssFlashAggregate(kind);
    if (visible == (u32)sCssFlashShown[kind])
    {
        return;
    }
    ndsMenuShellCssDrawFlashKind(kind, visible);
    sCssFlashShown[kind] = (u8)((visible != FALSE) ? 1u : 0u);
}

static void ndsMenuShellCssFlashStart(u32 slot)
{
    u32 old_kind;
    u32 kind;

    if (slot >= (u32)NDS_CSS_SLOTS)
    {
        return;
    }
    kind = ndsMenuShellCssFlashKindFromFighter((u32)sCssFkind[slot]);
    if (kind >= NDS_CSS_FLASH_KIND_COUNT)
    {
        return;
    }
    old_kind = (u32)sCssFlashKind[slot];
    sCssFlashRemain[slot] = (u8)NDS_CSS_FLASH_TICS;
    sCssFlashVisible[slot] = 1u;
    sCssFlashKind[slot] = (u8)kind;
    if ((old_kind < NDS_CSS_FLASH_KIND_COUNT) && (old_kind != kind))
    {
        ndsMenuShellCssSyncFlashKind(old_kind);
    }
    ndsMenuShellCssSyncFlashKind(kind);
}

static void ndsMenuShellCssFlashCancel(u32 slot)
{
    u32 kind;

    if (slot >= (u32)NDS_CSS_SLOTS)
    {
        return;
    }
    kind = (u32)sCssFlashKind[slot];
    sCssFlashRemain[slot] = 0u;
    sCssFlashVisible[slot] = 0u;
    sCssFlashKind[slot] = (u8)NDS_CSS_FLASH_KIND_NONE;
    if (kind < NDS_CSS_FLASH_KIND_COUNT)
    {
        ndsMenuShellCssSyncFlashKind(kind);
    }
}

static void ndsMenuShellCssStepFlashes(void)
{
    u32 dirty = 0u;
    u32 slot;
    u32 kind;

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        if (sCssFlashRemain[slot] == 0u)
        {
            continue;
        }
        kind = (u32)sCssFlashKind[slot];
        if (kind < NDS_CSS_FLASH_KIND_COUNT)
        {
            dirty |= (1u << kind);
        }
        sCssFlashRemain[slot]--;
        if (sCssFlashRemain[slot] == 0u)
        {
            sCssFlashVisible[slot] = 0u;
            sCssFlashKind[slot] = (u8)NDS_CSS_FLASH_KIND_NONE;
        }
        else
        {
            /* mnPlayersVSPortraitFlashThreadUpdate toggles hidden every tic. */
            sCssFlashVisible[slot] = (sCssFlashVisible[slot] != 0u) ? 0u : 1u;
        }
    }
    for (kind = 0u; kind < NDS_CSS_FLASH_KIND_COUNT; kind++)
    {
        if ((dirty & (1u << kind)) != 0u)
        {
            ndsMenuShellCssSyncFlashKind(kind);
        }
    }
}

static void ndsMenuShellCssRedrawVisibleFlashes(void)
{
    u32 kind;

    for (kind = 0u; kind < NDS_CSS_FLASH_KIND_COUNT; kind++)
    {
        if (ndsMenuShellCssFlashAggregate(kind) != FALSE)
        {
            ndsMenuShellCssDrawFlashKind(kind, TRUE);
            sCssFlashShown[kind] = 1u;
        }
    }
}

/* --- Ready ---------------------------------------------------------------- */

/* mnPlayersVSGetReadyPlayerCount + mnPlayersVSCheckSingleTeam +
 * mnPlayersVSCheckNoPuckOnPortraitAll + mnPlayersVSCheckReady,
 * mnplayersvs.c:4268/:4314/:4337/:4352. */
static u32 ndsMenuShellCssCheckReady(void)
{
    u32 ready = 0u;
    s32 ready_team = -1;
    u32 has_other_team = FALSE;
    u32 i;

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        if ((sCssPkind[i] != (u8)nFTPlayerKindNot) && (sCssSelected[i] != 0u))
        {
            ready++;
            if (ready_team < 0)
            {
                ready_team = (s32)sCssTeam[i];
            }
            else if (ready_team != (s32)sCssTeam[i])
            {
                has_other_team = TRUE;
            }
        }
    }
    if (ready < 2u)
    {
        return FALSE;
    }
    if ((sCssIsTeamBattle != FALSE) && (has_other_team == FALSE))
    {
        return FALSE;
    }
    /* A token in the hand is not a choice yet. */
    return (sCssStatus == NDS_CSS_STATUS_GRAB) ? FALSE : TRUE;
}

static void ndsMenuShellCssShowReady(u32 lit)
{
    /* mnPlayersVSMakeReady owns TWO GObjs on the same 40/30 blink but on
     * DIFFERENT display links. The banner + READY TO FIGHT is link 38 through
     * the priority-10 Ready camera; REGION_US PRESS + START is link 28 through
     * the priority-50 player-kind camera. Keep the existing BG2 state swap for
     * the flash-overlap restoration, then mirror only the link-38 band onto
     * transparent foreground BG3 so it also has the source's depth over OBJ. */
    u8 state = (lit != FALSE) ?
        (u8)NDS_MN_UI_KIT_SURFACE_CSS_READY_ON :
        (u8)NDS_MN_UI_KIT_SURFACE_CSS_READY_OFF;

    if (ndsUiKitBlitSurfaces(&state, 1u) != FALSE)
    {
        gNdsMenuShellCssPanelBlitCount++;
    }
    ndsUiKitClearForegroundRect(NDS_CSS_DS(0), NDS_CSS_DS(71),
                                (u32)NDS_CSS_DS(320),
                                18u); /* baked 22-source-row box -> 18 DS rows */
    if (lit != FALSE)
    {
        u8 foreground = (u8)NDS_MN_UI_KIT_SURFACE_CSS_READY_FOREGROUND;

        if (ndsUiKitBlitForegroundSurfaces(&foreground, 1u) != FALSE)
        {
            gNdsMenuShellCssPanelBlitCount++;
        }
        ndsUiKitSetSprite(NDS_CSS_SPRITE_READY_PRESS,
                          NDS_MN_UI_KIT_IMAGE_CSS_READY_PRESS,
                          NDS_CSS_DS(133), NDS_CSS_DS(219));
        ndsUiKitSetSprite(NDS_CSS_SPRITE_READY_START,
                          NDS_MN_UI_KIT_IMAGE_CSS_READY_START,
                          NDS_CSS_DS(162), NDS_CSS_DS(219));
    }
    else
    {
        ndsUiKitHideSprite(NDS_CSS_SPRITE_READY_PRESS);
        ndsUiKitHideSprite(NDS_CSS_SPRITE_READY_START);
    }
    sCssReadyShown = (lit != FALSE) ? 1u : 0u;
    /* READY is source camera priority 10, in front of the priority-73 portrait
     * flash.  Its state surface overwrites the overlap, so re-apply only live
     * flash rectangles using the matching READY-aware baked variant. */
    ndsMenuShellCssRedrawVisibleFlashes();
}

/* --- Actions -------------------------------------------------------------- */

/* mnPlayersVSSetCursorGrab, mnplayersvs.c:2932. */
static void ndsMenuShellCssGrab(u32 slot)
{
    /* mnPlayersVSSetCursorGrab destroys this slot's existing portrait flash. */
    ndsMenuShellCssFlashCancel(slot);
    sCssHeld = (s32)slot;
    sCssStatus = NDS_CSS_STATUS_GRAB;
    sCssSelected[slot] = 0u;
    sCssPuckX[slot] = (s16)(sCssCursorX + NDS_CSS_PUCK_CARRY_DX);
    sCssPuckY[slot] = (s16)(sCssCursorY + NDS_CSS_PUCK_CARRY_DY);
    ndsMenuShellCssCue(NDS_CSS_FGM_GRAB);
    gNdsMenuShellCssGrabCount++;
    ndsMenuShellCssPopulate();
}

/* mnPlayersVSSelectFighterPuck, mnplayersvs.c:167. */
static void ndsMenuShellCssDrop(u32 slot)
{
    sCssSelected[slot] = 1u;
    sCssHeld = -1;
    sCssStatus = NDS_CSS_STATUS_HOVER;
    sCssRegrabTic = sMenuTics + (u32)NDS_CSS_REGRAB_TICS;
    ndsMenuShellCssAnnounce(slot);
    ndsMenuShellCssFlashStart(slot);
    gNdsMenuShellCssDropCount++;
    ndsMenuShellCssPopulate();
}

/* mnPlayersVSSelectFighter, mnplayersvs.c:2825. */
static u32 ndsMenuShellCssSelectFighter(void)
{
    if (sCssStatus != NDS_CSS_STATUS_GRAB)
    {
        return FALSE;
    }
    if (sCssHeld < 0)
    {
        return FALSE;
    }
    if (sCssFkind[sCssHeld] != (u8)nFTKindNull)
    {
        ndsMenuShellCssDrop((u32)sCssHeld);
        return TRUE;
    }
    /* The token is over a locked cell or off the grid: the source refuses with
     * the cue it spends on every refusal, and the token stays in hand. */
    ndsMenuShellCssCue(NDS_CSS_FGM_DENIED);
    gNdsMenuShellCssDropRefuseCount++;
    return FALSE;
}

/* mnPlayersVSCheckCursorPuckGrab, mnplayersvs.c:2957. Its loop runs 3 -> 0 so
 * a higher slot's token wins an overlap, and a slot that is NOT the cursor's
 * own is grabbable only when it is a CPU. */
static u32 ndsMenuShellCssCheckGrab(void)
{
    s32 i;

    if (sMenuTics < sCssRegrabTic)
    {
        return FALSE;
    }
    if (sCssStatus != NDS_CSS_STATUS_HOVER)
    {
        return FALSE;
    }
    for (i = NDS_CSS_SLOTS - 1; i >= 0; i--)
    {
        u32 slot = (u32)i;

        if (ndsMenuShellCssPuckInRange(slot) == FALSE)
        {
            continue;
        }
        if (slot == (u32)NDS_CSS_CURSOR_SLOT)
        {
            if (sCssPkind[slot] != (u8)nFTPlayerKindNot)
            {
                ndsMenuShellCssGrab(slot);
                return TRUE;
            }
        }
        else if (sCssPkind[slot] == (u8)nFTPlayerKindCom)
        {
            ndsMenuShellCssGrab(slot);
            return TRUE;
        }
    }
    return FALSE;
}

/* mnPlayersVSUpdatePlayerKind, mnplayersvs.c:2181, reduced to the one-cursor
 * case this hardware has. Its three arms all begin the same way: whatever the
 * cursor was carrying is DROPPED (selected) before the slot changes kind. */
static void ndsMenuShellCssApplyKind(u32 slot)
{
    if ((sCssHeld >= 0) && ((u32)sCssHeld == slot))
    {
        /* mnPlayersVSUpdatePlayerKind flashes a token it is about to release
         * before changing that slot's ownership/kind state. */
        ndsMenuShellCssFlashStart(slot);
        sCssHeld = -1;
        sCssStatus = NDS_CSS_STATUS_HOVER;
    }
    switch (sCssPkind[slot])
    {
    case (u8)nFTPlayerKindMan:
        /* The slot goes back to holding its own token, which for the cursor's
         * own slot means the cursor picks it up again. */
        sCssSelected[slot] = 0u;
        sCssFkind[slot] = (u8)nFTKindNull;
        if (slot == (u32)NDS_CSS_CURSOR_SLOT)
        {
            sCssHeld = (s32)slot;
            sCssStatus = NDS_CSS_STATUS_GRAB;
            sCssPuckX[slot] = (s16)(sCssCursorX + NDS_CSS_PUCK_CARRY_DX);
            sCssPuckY[slot] = (s16)(sCssCursorY + NDS_CSS_PUCK_CARRY_DY);
        }
        ndsMenuShellCssCue(NDS_CSS_FGM_SLOT_WHOOSH);
        break;

    case (u8)nFTPlayerKindCom:
        sCssSelected[slot] = 1u;
        if (sCssFkind[slot] == (u8)nFTKindNull)
        {
            u32 fkind;

            /* mnPlayersVSRandFighterKind, mnplayersvs.c:3471, samples the full
             * playable range with syUtilsRandTimeUCharRange and rerolls crossed
             * / locked fighters. FighterCrossed is a source stub returning
             * FALSE; this build's EXISTENCE mask is the locked predicate. Do
             * not substitute frame parity: it is distribution-equivalent for
             * two choices but not the source RNG state/sequence. */
            do
            {
                fkind = (u32)syUtilsRandTimeUCharRange(
                    (s32)nFTKindPlayableEnd + 1);
            }
            while (ndsMenuShellCssFighterLocked(fkind) != FALSE);
            sCssFkind[slot] = (u8)fkind;
        }
        ndsMenuShellCssCenterPuck(slot, (u32)sCssFkind[slot]);
        ndsMenuShellCssAnnounce(slot);
        /* mnPlayersVSCheckPlayerKindSelect makes a fresh flash after the COM
         * transition, replacing any one UpdatePlayerKind just created. */
        ndsMenuShellCssFlashStart(slot);
        break;

    default:
        sCssSelected[slot] = 0u;
        sCssFkind[slot] = (u8)nFTKindNull;
        ndsMenuShellCssCue(NDS_CSS_FGM_SLOT_WHOOSH);
        break;
    }
    /* mnPlayersVSCheckPlayerKindSelect spends this on EVERY kind change, after
     * the per-kind cue above (mnplayersvs.c:2513). */
    ndsMenuShellCssCue(NDS_CSS_FGM_PRESS_START);
}

/* mnPlayersVSCheckPlayerKindSelectAllPlayer, mnplayersvs.c:2521. */
static u32 ndsMenuShellCssCheckKindButton(void)
{
    u32 slot;

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        s32 panel = (s32)(slot * 69u);
        u32 next;

        if (ndsMenuShellCssBoxHit(panel + 60, panel + 88, 127, 145) == FALSE)
        {
            continue;
        }
        next = (u32)sCssPkind[slot] + 1u;
        if (next > (u32)nFTPlayerKindNot)
        {
            /* The wrap depends on whether the slot HAS a controller
             * (mnplayersvs.c:2477): with one, HUMAN is in the cycle; without
             * one, it is not, so an unmanned slot alternates CP and NA. */
            next = (slot == (u32)NDS_CSS_CURSOR_SLOT) ?
                (u32)nFTPlayerKindMan : (u32)nFTPlayerKindCom;
        }
        sCssPkind[slot] = (u8)next;
        ndsMenuShellCssApplyKind(slot);
        gNdsMenuShellCssKindToggleCount++;
        ndsMenuShellCssPopulate();
        return TRUE;
    }
    return FALSE;
}

/* mnPlayersVSCheckTeamSelectInRangeAll, mnplayersvs.c:1985. The source draws a
 * selector for every panel in Team Battle but only lets an occupied panel
 * consume A. Each press cycles Red -> Blue -> Green -> Red, recolors the gate,
 * switches the fighter to its team costume/shade, and spends TitlePressStart.
 * The appearance mutation itself remains in the imported PlayersVS helper so
 * mnPlayersVSGetShade/ftParamGetCostumeTeamID stay the behavioral authority. */
static u32 ndsMenuShellCssCheckTeamSelect(void)
{
    u32 slot;

    if (sCssIsTeamBattle == FALSE)
    {
        return FALSE;
    }
    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        s32 panel = (s32)(slot * 69u);

        if ((sCssPkind[slot] == (u8)nFTPlayerKindNot) ||
            (ndsMenuShellCssBoxHit(panel + 34, panel + 58, 131, 141) == FALSE))
        {
            continue;
        }
        sCssTeam[slot] =
            (sCssTeam[slot] == (u8)nSCBattleTeamIDGreen) ?
                (u8)nSCBattleTeamIDRed : (u8)(sCssTeam[slot] + 1u);
        /* The source swaps THIS panel's LUT + team selector immediately. Do not
         * route it through SyncPanels(1): if an older panel update were pending,
         * that budget could spend itself on an earlier slot and visibly delay
         * the clicked team. This direct one-surface replacement is the exact
         * DS equivalent of UpdateTeamSelect + SetGateLUT. */
        {
            u8 panel_surface = ndsMenuShellCssGateSurface(slot);

            if (ndsUiKitBlitSurfaces(&panel_surface, 1u) != FALSE)
            {
                sCssPanelSurface[slot] = panel_surface;
                gNdsMenuShellCssPanelBlitCount++;
            }
        }
        ndsMenuShellCssDrawTeamSelect(slot);
        ndsMenuShellCssCue(NDS_CSS_FGM_PRESS_START);
        return TRUE;
    }
    return FALSE;
}

/* mnPlayersVSCheckHandicapArrowInRangeAll, mnplayersvs.c:2027 -- clamped 1..9
 * both ways, and the cue only fires when the value actually moves. */
static u32 ndsMenuShellCssCheckLevelArrows(void)
{
    u32 slot;

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        s32 panel = (s32)(slot * 69u);

        if ((sCssPkind[slot] != (u8)nFTPlayerKindCom) ||
            (sCssSelected[slot] == 0u))
        {
            continue;
        }
        if (ndsMenuShellCssBoxHit(panel + 68, panel + 90, 197, 216) != FALSE)
        {
            if (sCssLevel[slot] < 9u)
            {
                sCssLevel[slot]++;
                ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
                gNdsMenuShellCssLevelChangeCount++;
                ndsMenuShellCssPopulate();
            }
            return TRUE;
        }
        if (ndsMenuShellCssBoxHit(panel + 21, panel + 43, 197, 216) != FALSE)
        {
            if (sCssLevel[slot] > 1u)
            {
                sCssLevel[slot]--;
                ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
                gNdsMenuShellCssLevelChangeCount++;
                ndsMenuShellCssPopulate();
            }
            return TRUE;
        }
    }
    return FALSE;
}

/* mnPlayersVSRecallPuck, mnplayersvs.c:3198 -- B with your own fighter placed
 * pulls the token back. The END STATE is what is transcribed: the token is in
 * the hand again (mnPlayersVSPuckAdjustRecall re-grabs it at recall tic 11). */
static void ndsMenuShellCssRecall(void)
{
    u32 slot = (u32)NDS_CSS_CURSOR_SLOT;

    if ((sCssSelected[slot] == 0u) || (sCssHeld >= 0) ||
        (sCssPkind[slot] != (u8)nFTPlayerKindMan))
    {
        return;
    }
    sCssSelected[slot] = 0u;
    sCssHeld = (s32)slot;
    sCssStatus = NDS_CSS_STATUS_GRAB;
    sCssPuckX[slot] = (s16)(sCssCursorX + NDS_CSS_PUCK_CARRY_DX);
    sCssPuckY[slot] = (s16)(sCssCursorY + NDS_CSS_PUCK_CARRY_DY);
    /* This shell jumps directly to mnPlayersVSPuckAdjustRecall's tic-11 end
     * state, whose SetCursorGrab call destroys the portrait flash. */
    ndsMenuShellCssFlashCancel(slot);
    gNdsMenuShellCssRecallCount++;
    ndsMenuShellCssPopulate();
}

/* mnPlayersVSSetSceneData, mnplayersvs.c:4379 -- through the P2-1a descriptor,
 * which is the battle's only input, rather than straight into the battle
 * state. The rules half stays whatever the VS menu committed. */
static void ndsMenuShellCssCommit(void)
{
    u32 i;

    for (i = 0u; i < (u32)NDS_MATCH_FIGHTERS_MAX; i++)
    {
        NdsMatchFighterConfig *slot = &gNdsMatchConfig.fighters[i];

        slot->fkind = sCssFkind[i];
        slot->pkind = sCssPkind[i];
        slot->level = sCssLevel[i];
        /* mnPlayersVSSetSceneData writes the slot's selected team. The old
         * array-index assignment even produced team id 3 for slot 4, outside
         * the source's Red/Blue/Green enum. */
        slot->team = sCssTeam[i];
        /* The source CSS commits the appearance it is actually showing. Ask
         * the imported PlayersVS state instead of re-deriving team/free-color
         * allocation here; this preserves mnPlayersVSGetShade's slot-order
         * behavior for mirrored Mario/Fox fighters. */
        {
            u32 appearance = ndsMNPlayersVSPreviewGetAppearance(i);

            slot->costume = (u8)(appearance & 0xffu);
            slot->shade = (u8)((appearance >> 8) & 0xffu);
        }
        gNdsMenuShellCssCommitSlot[i] =
            (((u32)sCssFkind[i] & 0xffu) << 16) |
            (((u32)sCssPkind[i] & 0xffu) << 8) | ((u32)sCssLevel[i] & 0xffu);
    }
    /* P2-2a: mode and all four source slot-team selectors are live. The
     * descriptor therefore carries the same CSS state mnPlayersVSSetSceneData
     * commits for FFA and Team Battle. */
    gNdsMatchConfig.is_team_battle = (sCssIsTeamBattle != FALSE) ? 1u : 0u;
    ndsMatchConfigApply(&gNdsMatchConfig);
    gNdsMenuShellCssCommitCount++;
}

/* mnPlayersVSSetIdlePlayerNotAll, mnplayersvs.c:4294. */
static void ndsMenuShellCssIdleSlotsNot(void)
{
    u32 i;

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        if (sCssSelected[i] == 0u)
        {
            sCssPkind[i] = (u8)nFTPlayerKindNot;
            sCssFkind[i] = (u8)nFTKindNull;
        }
    }
}

/* mnPlayersVSBackToVSMode, mnplayersvs.c:3227: the CSS commits on the way out
 * too, so a return trip shows what the last visit chose. */
static void ndsMenuShellCssBack(void)
{
    ndsMenuShellCssCommit();
    gNdsMenuShellCssBackCount++;
    /* P2-1k (g). mnPlayersVSBackToVSMode, mnplayersvs.c:3234: the character
     * select STOPS all BGM on its way out, and the VS menu it returns to
     * starts the mode-select track again from its own func_start. Without the
     * stop, BGM 10 would keep playing under a VS menu that then starts 44 over
     * the top of it. */
    ndsAudioBgmStopAll();
    ndsMenuShellGoto((u32)nSCKindVSMode);
}

/* mnPlayersVSUpdateCursorNoRecall, mnplayersvs.c:3128. The band is 38..124 --
 * NOT the 36..122 the grid itself uses, which is the source's own two-pixel
 * asymmetry and is kept. */
static void ndsMenuShellCssUpdateStatus(void)
{
    if ((sCssCursorY > 124) || (sCssCursorY < 38))
    {
        sCssStatus = NDS_CSS_STATUS_POINTER;
    }

    else if (sCssHeld < 0)
    {
        sCssStatus = NDS_CSS_STATUS_HOVER;
    }
    else
    {
        sCssStatus = NDS_CSS_STATUS_GRAB;
    }
    /* The source has one more branch here -- a POINTER cursor over a placed
     * token becomes a HOVER one -- gated on the CURSOR PLAYER's own
     * `is_selected`, which mnPlayersVSUpdatePuckDisplay clears every frame for
     * a human slot (mnplayersvs.c:2320). It can therefore never run for a human
     * cursor, which is the only kind this hardware has, so it is absent rather
     * than transcribed-and-dead. */
}

static void ndsMenuShellUpdateCss(u32 held, u32 taps)
{
    /* ONE panel surface a frame, and BEFORE this frame's input, so a kind
     * change lands on the frame AFTER the press rather than on the press's own
     * frame -- which is also the frame the source spends an FGM cue on. See
     * ndsMenuShellCssSyncPanels for the measurement behind the budget. */
    ndsMenuShellCssSyncPanels(1u);
    ndsMenuShellCssStepDoors();
    ndsMenuShellCssStepFlashes();

    /* mnPlayersVSFuncRun's start delay: once START is accepted the screen stops
     * taking input and counts down before the scene changes. */
    if (sCssStartWait != 0u)
    {
        sCssStartWait--;
        if (sCssStartWait == 0u)
        {
            ndsMenuShellCssCommit();
            /* mnPlayersVSFuncRun:4493 -- P2-1f. The source picks the STAGE
             * SELECT when `gSCManagerTransferBattleState.is_stage_select` is
             * set and randomises the ground itself when it is not; the
             * descriptor's `is_stage_select` IS that field (P2-1a applies it
             * verbatim), so this is the source's own branch on the source's
             * own bit. P2-1e went straight to the battle because
             * `nSCKindMaps` had no screen; it has one now.
             *
             * THE ELSE ARM IS STILL NOT THE SOURCE'S. The source randomises
             * over every unlocked ground there, and eight of the nine are
             * P2-4; until then a no-stage-select match keeps whatever ground
             * the descriptor already carries, which is the same narrowing
             * P2-1e recorded and it is unreachable in every configuration
             * this build ships (the preset sets the bit). */
            ndsMenuShellGoto(
                (gNdsMatchConfig.is_stage_select != FALSE) ?
                    (u32)nSCKindMaps : (u32)nSCKindVSBattle);
        }
        return;
    }

    /* 1. The cursor. */
    if ((held & NDS_INPUT_RIGHT) != 0u)
    {
        sCssCursorX += NDS_CSS_CURSOR_STEP;
    }
    if ((held & NDS_INPUT_LEFT) != 0u)
    {
        sCssCursorX -= NDS_CSS_CURSOR_STEP;
    }
    if ((held & NDS_INPUT_DOWN) != 0u)
    {
        sCssCursorY += NDS_CSS_CURSOR_STEP;
    }
    if ((held & NDS_INPUT_UP) != 0u)
    {
        sCssCursorY -= NDS_CSS_CURSOR_STEP;
    }
    if (sCssCursorX < NDS_CSS_CURSOR_X_MIN)
    {
        sCssCursorX = NDS_CSS_CURSOR_X_MIN;
    }
    if (sCssCursorX > NDS_CSS_CURSOR_X_MAX)
    {
        sCssCursorX = NDS_CSS_CURSOR_X_MAX;
    }
    if (sCssCursorY < NDS_CSS_CURSOR_Y_MIN)
    {
        sCssCursorY = NDS_CSS_CURSOR_Y_MIN;
    }
    if (sCssCursorY > NDS_CSS_CURSOR_Y_MAX)
    {
        sCssCursorY = NDS_CSS_CURSOR_Y_MAX;
    }

    /* 2. A carried token rides the cursor, and while it rides it the fighter
     * under it is tracked live -- mnPlayersVSPuckProcUpdate:3542 is what makes
     * the portrait under the token light the name up before you let go. */
    if (sCssHeld >= 0)
    {
        u32 slot = (u32)sCssHeld;
        u32 fkind;

        sCssPuckX[slot] = (s16)(sCssCursorX + NDS_CSS_PUCK_CARRY_DX);
        sCssPuckY[slot] = (s16)(sCssCursorY + NDS_CSS_PUCK_CARRY_DY);
        fkind = ndsMenuShellCssPuckFighterKind(slot);
        if ((u8)fkind != sCssFkind[slot])
        {
            sCssFkind[slot] = (u8)fkind;
            /* mnPlayersVSPuckProcUpdate's Not -> Man promotion: dropping a
             * token on a portrait re-mans an empty slot that has a controller
             * (mnplayersvs.c:3550). */
            if ((sCssPkind[slot] == (u8)nFTPlayerKindNot) &&
                (slot == (u32)NDS_CSS_CURSOR_SLOT) &&
                (fkind != (u32)nFTKindNull))
            {
                sCssPkind[slot] = (u8)nFTPlayerKindMan;
            }
            ndsMenuShellCssPopulate();
        }
    }

    /* 3. A, in mnPlayersVSCursorProcUpdate's own order (mnplayersvs.c:3310):
     * the player-kind buttons first, then a DROP, then a GRAB, then the frame
     * furniture. Each returns TRUE only if it consumed the press. */
    if ((taps & NDS_INPUT_A) != 0u)
    {
        if (ndsMenuShellCssCheckKindButton() == FALSE)
        {
            if (ndsMenuShellCssSelectFighter() == FALSE)
            {
                if (ndsMenuShellCssCheckGrab() == FALSE)
                {
                    /* mnPlayersVSCursorProcUpdate checks game mode, BACK, then
                     * the team selectors in exactly this order (:3356-3367). */
                    if (ndsMenuShellCssBoxHit(27, 137, 14, 35) != FALSE)
                    {
                        u8 label;

                        sCssIsTeamBattle = (sCssIsTeamBattle != FALSE) ?
                            0u : 1u;
                        label = (sCssIsTeamBattle != FALSE) ?
                            (u8)NDS_MN_UI_KIT_SURFACE_CSS_MODE_TEAM :
                            (u8)NDS_MN_UI_KIT_SURFACE_CSS_MODE_FFA;
                        if (ndsUiKitBlitSurfaces(&label, 1u) != FALSE)
                        {
                            gNdsMenuShellCssPanelBlitCount++;
                        }
                        /* mnPlayersVSUpdateGameMode:1918 stops every current FGM
                         * before the scroll and new mode announcement. */
                        ndsAudioFgmStopAll();
                        ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
                        ndsMenuShellCssAnnounceMode();
                        /* UpdateGateAll and Make/DestroyTeamSelectAll are same-
                         * frame source operations. Gate palettes are BG2; the
                         * source DL-34 selectors are foreground BG3. */
                        ndsMenuShellCssSyncPanels((u32)NDS_CSS_SLOTS);
                        ndsMenuShellCssSyncTeamSelectAll();
                        gNdsMenuShellCssModeToggleCount++;
                        return;
                    }
                    if (ndsMenuShellCssBoxHit(244, 292, 13, 34) != FALSE)
                    {
                        ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
                        ndsMenuShellCssBack();
                        return;
                    }
                    if (ndsMenuShellCssCheckTeamSelect() == FALSE)
                    {
                        (void)ndsMenuShellCssCheckLevelArrows();
                    }
                }
            }
        }
    }

    /* 4. B recalls your own placed token (mnplayersvs.c:3411). */
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellCssRecall();
    }

    /* 5. mnPlayersVSDetectBack: B HELD for forty tics leaves the screen. */
    if ((held & NDS_INPUT_B) != 0u)
    {
        sCssBackTics++;
        if (sCssBackTics >= (u32)NDS_CSS_BACK_HOLD_TICS)
        {
            ndsMenuShellCssBack();
            return;
        }
    }
    else
    {
        sCssBackTics = 0u;
    }

    /* 6. The cursor's own state, last, exactly as the source updates it after
     * every action rather than before them. */
    ndsMenuShellCssUpdateStatus();

    /* 7. READY TO FIGHT, and the START it invites. */
    {
        u32 ready = ndsMenuShellCssCheckReady();
        u32 lit;

        if (ready != FALSE)
        {
            sCssReadyBlink++;
            if (sCssReadyBlink >= (u32)NDS_CSS_READY_BLINK)
            {
                sCssReadyBlink = 0u;
            }
            lit = (sCssReadyBlink < (u32)NDS_CSS_READY_LIT) ? TRUE : FALSE;
        }
        else
        {
            sCssReadyBlink = 0u;
            lit = FALSE;
        }
        if (lit != sCssReadyShown)
        {
            ndsMenuShellCssShowReady(lit);
        }

        if (((taps & NDS_INPUT_START) != 0u) &&
            (sMenuTics > (u32)NDS_CSS_START_ARM_TICS))
        {
            if (ready != FALSE)
            {
                ndsMenuShellCssCue(NDS_CSS_VOICE_CHEER);
                ndsMenuShellCssIdleSlotsNot();
                sCssStartWait = (u32)NDS_CSS_START_WAIT;
                gNdsMenuShellCssStartCount++;
                ndsMenuShellCssPopulate();
            }
            else
            {
                ndsMenuShellCssCue(NDS_CSS_FGM_DENIED);
                gNdsMenuShellCssStartDeniedCount++;
            }
        }
    }

    ndsMenuShellCssMove();
    gNdsMenuShellCssCursorX = sCssCursorX;
    gNdsMenuShellCssCursorY = sCssCursorY;
    gNdsMenuShellCssCursorStatus = sCssStatus;
}

/* mnPlayersVSInitVars + mnPlayersVSInitPlayer + mnPlayersVSInitSlot,
 * mnplayersvs.c:4670/:4579/:4698, reading the P2-1a descriptor instead of
 * gSCManagerTransferBattleState: the descriptor is what the battle consumes, so
 * it is also what the screen that fills it should open from. */
static void ndsMenuShellCssInit(void)
{
    u32 i;

    sCssCursorX = NDS_CSS_CURSOR_HOME_X;
    sCssCursorY = NDS_CSS_CURSOR_HOME_Y;
    sCssStatus = NDS_CSS_STATUS_POINTER;
    sCssHeld = -1;
    sCssRegrabTic = 0u;
    sCssBackTics = 0u;
    sCssStartWait = 0u;
    sCssReadyBlink = 0u;
    sCssReadyShown = 0xffffffffu; /* forces the first ShowReady to publish */
    sCssArrowsShown = TRUE;
    sCssFlashShown[NDS_CSS_FLASH_KIND_MARIO] = 0u;
    sCssFlashShown[NDS_CSS_FLASH_KIND_FOX] = 0u;
    /* P2-1N (4): seeded from the transfer state exactly as the source seeds
     * sMNPlayersVSIsTeamBattle on scene entry (mnplayersvs.c:4679). */
    sCssIsTeamBattle = (gSCManagerTransferBattleState.is_team_battle != 0) ?
        1u : 0u;

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        /* Nothing of this screen is on BG2 yet, so every panel differs and all
         * four blit on the entry frame -- the same tracker reset the VS menu's
         * buttons take in ndsMenuShellVsLoadRules. */
        sCssPanelSurface[i] = (u8)NDS_MENU_VS_SURFACE_NONE;
        sCssFlashRemain[i] = 0u;
        sCssFlashVisible[i] = 0u;
        sCssFlashKind[i] = (u8)NDS_CSS_FLASH_KIND_NONE;
    }

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        const NdsMatchFighterConfig *cfg = &gNdsMatchConfig.fighters[i];

        sCssPkind[i] = cfg->pkind;
        sCssFkind[i] = cfg->fkind;
        sCssLevel[i] = (cfg->level < 1u) ? 1u : ((cfg->level > 9u) ? 9u :
                                                 cfg->level);
        sCssTeam[i] = (cfg->team <= (u8)nSCBattleTeamIDGreen) ? cfg->team :
            (u8)((i < 2u) ? nSCBattleTeamIDRed : nSCBattleTeamIDBlue);
        /* mnPlayersVSUpdateGate, mnplayersvs.c:4193: a slot with no controller
         * cannot be HUMAN. Only slot 0 has one here, so any other slot the
         * descriptor calls human arrives as empty -- which is the same
         * correction the source makes, one frame later, in its gate pass. */
        if ((sCssPkind[i] == (u8)nFTPlayerKindMan) &&
            (i != (u32)NDS_CSS_CURSOR_SLOT))
        {
            sCssPkind[i] = (u8)nFTPlayerKindNot;
            sCssFkind[i] = (u8)nFTKindNull;
        }
        if (sCssPkind[i] == (u8)nFTPlayerKindNot)
        {
            sCssFkind[i] = (u8)nFTKindNull;
        }
        if (ndsMenuShellCssFighterLocked((u32)sCssFkind[i]) != FALSE)
        {
            sCssFkind[i] = (u8)nFTKindNull;
        }
        sCssSelected[i] = (sCssFkind[i] != (u8)nFTKindNull) ? 1u : 0u;
        ndsMenuShellCssCenterPuck(i, (u32)sCssFkind[i]);
        gNdsMenuShellCssCommitSlot[i] = 0u;
    }
    /* A human slot with no fighter yet holds its own token, so the screen opens
     * with the token already in the cursor's hand (mnplayersvs.c:4604). */
    if ((sCssPkind[NDS_CSS_CURSOR_SLOT] == (u8)nFTPlayerKindMan) &&
        (sCssFkind[NDS_CSS_CURSOR_SLOT] == (u8)nFTKindNull))
    {
        sCssHeld = NDS_CSS_CURSOR_SLOT;
        sCssPuckX[NDS_CSS_CURSOR_SLOT] =
            (s16)(sCssCursorX + NDS_CSS_PUCK_CARRY_DX);
        sCssPuckY[NDS_CSS_CURSOR_SLOT] =
            (s16)(sCssCursorY + NDS_CSS_PUCK_CARRY_DY);
    }
    /* P2-1N (3): mnPlayersVSMakeGate seeds EVERY slot at door_offset=41
     * (closed), then mnPlayersVSShutterProcUpdate opens each occupied slot by
     * 2/tic. Do not shortcut occupied slots to zero on entry: that deletes the
     * source's entrance shutter animation. The CSS_DOORS strip is cached here
     * for those slides. */
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        sCssDoorOffset[i] = 41u;
    }
    (void)ndsUiKitCacheSurface((u32)NDS_MN_UI_KIT_SURFACE_CSS_DOORS);
}

static void ndsMenuShellCssSyncPreviews(void)
{
    u32 slot;

    /* The source's mode/team state must be visible to mnPlayersVSUpdateFighter
     * BEFORE any slot can ask mnPlayersVSGetFreeCostume/GetShade. This call also
     * performs UpdateGameMode/TeamSelect's in-place material changes for
     * fighters whose kind did not change. */
    ndsMNPlayersVSPreviewSyncRules(
        (sCssIsTeamBattle != FALSE) ? TRUE : FALSE,
        sCssTeam, (u32)NDS_CSS_SLOTS);
    /* Source mnPlayersVSInitSlotAll explicitly initializes 0,1,2,3 and every
     * player-kind / puck update can rebuild that slot's fighter. Mirror all
     * four here now that the native owner path is instance-safe for mirrors. */
    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        ndsMNPlayersVSPreviewSync(slot, (s32)sCssPkind[slot],
                                  (s32)sCssFkind[slot],
                                  (sCssSelected[slot] != 0u) ? TRUE : FALSE);
    }
}

/* Screen ENTRY, on a load frame: the only place all four panels are written at
 * once. Every later change goes through ndsMenuShellCssMove's one-a-frame
 * sync. */
static void ndsMenuShellPopulateCssScreen(void)
{
    ndsMenuShellCssSyncPanels((u32)NDS_CSS_SLOTS);
    ndsMenuShellCssPopulate();
    ndsMenuShellCssSyncTeamSelectAll();
    ndsMenuShellCssShowReady(FALSE);
    ndsMenuShellCssMove();
}

/* --- Screen: VS stage select (P2-1f) --------------------------------------
 *
 * mn/mnmaps/mnmaps.c. A GRID screen, not a pointer one: a red frame sits on
 * one of ten cells -- nine grounds in a 5x2 block plus RANDOM -- and moves a
 * whole cell at a time. Everything below is transcribed from that file, in the
 * SOURCE's own 320x240 frame exactly as the character select is, so a probe
 * reads back the numbers mnmaps.c is written in.
 *
 * THE GRID, mnMapsMakeIcons:530 and mnMapsSetCursorPosition:865. Icon i sits
 * at (i*50 + 30, 30) for i < 5 and (i*50 - 220, 68) for i >= 5, so the columns
 * are x = 30/80/130/180/230 and the rows y = 30/68; the cursor sits at exactly
 * (icon - 7, icon - 7). mnMapsGetGroundKind:453 is what a slot MEANS, and slot
 * 9 is `0xDE` -- the source's own spelling of RANDOM, kept rather than renamed.
 *
 * WHAT IS LOCKED, and it is the same substitution the character select makes.
 * The source's `mnMapsCheckLocked` (mnmaps.c:166) locks exactly one ground --
 * Mushroom Kingdom, behind `LBBACKUP_UNLOCK_MASK_INISHIE` -- because a retail
 * cart's other eight are always available. The gate this build needs is which
 * grounds EXIST: eight of the nine are P2-4, so the mask is Dream Land alone.
 * Same bitmask over gkind, different bound, and the RANDOM cell is never
 * locked because the source never locks it.
 *
 * THE ONE GENERALISATION, disclosed rather than buried. The source's UP/DOWN
 * arms test the destination with `mnMapsCheckLocked` and refuse the move; its
 * LEFT/RIGHT arms do NOT, because the only lockable ground is slot 4 and the
 * left/right wraps SPECIAL-CASE slot 4 by hand (`case 0: locked(4) ? 3 : 4`,
 * `case 3: locked(4) ? 0 : 4`). With eight locked cells that hand-written case
 * stops covering the lock, so left/right here SKIP locked cells in the
 * direction of travel around the row's own cycle. That reproduces the source's
 * table exactly on every case it enumerates -- left from 0 reaches 4, finds it
 * locked and lands on 3; right from 3 reaches 4, finds it locked and wraps to
 * 0; right from 4 is 0; left from 5 is 9; right from 9 is 5; everything else
 * is +-1 -- and it extends to a lock set the source never had. The scan is
 * bounded by the row length so an all-locked row cannot spin.
 *
 * DELIBERATE NARROWINGS, each a plan non-goal rather than an omission:
 *   - THE 3D PREVIEW. mnMapsMakePreview loads the ground's map file into one
 *     of two model heaps, builds up to four layer GObjs from its own DObj
 *     descriptors, and orbits a camera over them (mnmaps.c:1096/:1027/:1330).
 *     That is the RSP/RDP scene graph this target does not have, and it is the
 *     single most expensive thing on the source's screen. The preview PANEL is
 *     kept and holds the selected ground's own icon instead.
 *   - THE PLAQUE AND THE EMBLEM. mnMapsMakePlaque draws an 84x85 CI8 wooden
 *     circle with a 256-entry TLUT and five bitmap bands, and the emblem on it
 *     is a per-series FTEmblem sprite (mnmaps.c:245/:790). Main OBJ VRAM has
 *     8,448 B left after this row's three icons; the circle alone is 8,192 of
 *     it for one decoration. The words it frames are kept.
 *   - THE STAGE NAME AND HEADER as SPRITES. The source draws
 *     `llMNMaps<Ground>TextSprite` and `llMNMapsStageSelectTextSprite`; this
 *     composes the same words out of the source's own menu font, which is the
 *     P2-1c kit's whole reason for existing and exactly what P2-1e did with
 *     the fighter names. REGION_US draws no subtitle at all
 *     (mnMapsMakeSubtitle is `return;` under REGION_US, mnmaps.c:659).
 *   - THE 5-MINUTE IDLE RETURN (mnmaps.c:1451) is attract behaviour and
 *     belongs to P2-7, exactly as it does on the mode select and the CSS.
 *   - TRAINING MODE. `sMNMapsIsTrainingMode` is set when scene_prev is
 *     nSCKindPlayers1PTraining (mnmaps.c:1415); that scene is P2-7, so the
 *     flag is FALSE by construction here and its two branches -- the training
 *     wallpaper set and the 1PTrainingMode destination -- are absent rather
 *     than transcribed-and-dead.
 *
 * ONE LOCKED CELL IS DRAWN WHERE THE SOURCE DRAWS NOTHING, and it is the row's
 * only presentation ADDITION. mnMapsMakeIcons simply skips a locked ground, so
 * a build with eight locked grounds would show two icons floating in an empty
 * field and the 5x2 grid the cursor moves around would be invisible. The cell
 * gets the source's own locked plate -- the question-mark sprite
 * mnPlayersVSMakePortrait draws for a locked FIGHTER, already in the pack from
 * P2-1e -- so the grid reads as a grid. Zero new bytes, and it is the
 * precedent the character select already set. */

/* Source frame -> DS pixels, same exact 4/5 the character select uses. */
#define NDS_SSS_DS(v) NDS_CSS_DS(v)

#define NDS_SSS_SLOTS 10u
#define NDS_SSS_ROW 5u
/* mnMapsGetGroundKind:453 -- slot 9's ground kind. */
#define NDS_SSS_GKIND_RANDOM 0xdeu

/* mnMapsMakeIcons' grid, in source units. */
#define NDS_SSS_ICON_X0 30
#define NDS_SSS_ICON_PITCH 50
#define NDS_SSS_ICON_Y0 30
#define NDS_SSS_ICON_Y1 68
/* mnMapsSetCursorPosition (mnmaps.c:845/:851): the frame sits 7 px up and
 * left of the icon -- `slot * 50 + 23` against the icon's `slot * 50 + 30`.
 * P2-1L (6) applies it directly: with the grid and the cursor both at the
 * frame's own 4/5, the source's own offset is the only one that keeps a 50x40
 * frame around a 38x29 icon. */
#define NDS_SSS_CURSOR_DX (-7)
#define NDS_SSS_CURSOR_DY (-7)
/* P2-1L (9) DELETED `NDS_SSS_ICON_INSET_X/Y` with the placeholder that needed
 * them: they centred P2-1f's 5/8 icon inside the 4/5 grid footprint, and the
 * preview panel was the last draw on this screen still at 5/8. Nothing on the
 * stage select is at any ratio but the frame's own 4/5 now. */

/* mnMapsFuncRun's own input gate and repeat (mnmaps.c:1440/:1523). */
#define NDS_SSS_INPUT_ARM_TICS 10
#define NDS_SSS_SCROLL_WAIT 12

/* THE CUES, by the source's own REGION_US FGM ids, derived by the same
 * gm/gmsound.h parse P2-1c-1/P2-1d-1/P2-1e-1 used and cross-checked against
 * every id this tree already pins.
 *   164 MenuScroll2  -- every cursor move (mnmaps.c:1508 and the three arms
 *                       below it). IN THE PACK since P2-1c-1.
 *   159 StageSelect  -- the A/START confirm (mnmaps.c:1470). NOT IN THE PACK:
 *                       the pack carries 158/163/164/165 plus P2-1d-1's 157
 *                       and P2-1e-1's 121/127/167/512, and 159 is none of
 *                       them. The seam asks with the real id so the FGM miss
 *                       ring measures the gap; row P2-1f-1 renders it.
 * B PLAYS NOTHING. mnMapsFuncRun's B arm transitions silently (mnmaps.c:1483)
 * -- no `func_800269C0_275C0` call at all -- which is worth saying because
 * every other screen in this shell cues its back-out. */
#define NDS_SSS_FGM_SCROLL2 164u    /* nSYAudioFGMMenuScroll2 */
#define NDS_SSS_FGM_CONFIRM 159u    /* nSYAudioFGMStageSelect */

/* mnMapsFuncStart starts NO music (mnmaps.c:1595 makes cameras and sprites and
 * nothing else), so the character select's BGM 10 plays straight through this
 * screen -- which is why mnPlayersVSFuncStart's own track is gated on
 * `scene_prev != nSCKindMaps` (mnplayersvs.c:4899), the condition P2-1e already
 * transcribed against a scene that could not yet be scene_prev. It can now. */

/* mnMapsGetGroundKind, mnmaps.c:453. */
static const u8 kNdsSssSlotGkind[NDS_SSS_SLOTS] = {
    (u8)nGRKindCastle, (u8)nGRKindJungle, (u8)nGRKindHyrule,
    (u8)nGRKindZebes, (u8)nGRKindInishie, (u8)nGRKindYoster,
    (u8)nGRKindPupupu, (u8)nGRKindSector, (u8)nGRKindYamabuki,
    (u8)NDS_SSS_GKIND_RANDOM
};

/* Which grounds this build HAS. Same shape as the source's ground_mask
 * (LBBACKUP_MASK_STAGE, sc/scene.h:107), and it is the whole lock table. */
#define NDS_SSS_GROUND_MASK LBBACKUP_MASK_STAGE(nGRKindPupupu)

/* P2-1k (c). THE STAGE SELECT'S TEXT IS SOURCE ART NOW and this screen draws
 * no kit text at all: the STAGE SELECT header is `llMNMapsStageSelectTextSprite`
 * baked into SSS_SCREEN, and the per-stage name is
 * `llMNMaps<Ground>TextSprite` baked into one of the ten SSS_PLAQUE surfaces.
 *
 * The plaque is its own surface for the reason the VS menu's buttons are:
 * `mnMapsMakeNameAndEmblem` (mnmaps.c:818) ejects and re-makes the pair on
 * every cursor move, and all ten share one declared box composited over the
 * furniture beneath them, so a move is a single blit that overwrites the last
 * state exactly. Indexed by SLOT, which is what the cursor holds. */
static const u8 kNdsSssPlaqueSurface[NDS_SSS_SLOTS] = {
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_0,
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_1,
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_2,
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_3,
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_4,
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_5,
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_6,
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_7,
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_8,
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_9
};

/* P2-1L (9). THE PREVIEW PANEL'S ART, on the same terms as the plaque above.
 * `mnMapsMakePreviewWallpaper` (mnmaps.c:909) draws the SELECTED ground's own
 * 300x220 background at scale 0.37 -- or RANDOM's own 110x82 plate unscaled --
 * over the fill and the seven tiles SSS_SCREEN already carries, and
 * `mnMapsMakePreview` (:1100) rebuilds it on every cursor move exactly as
 * `mnMapsMakeNameAndEmblem` rebuilds the plaque.  So it is a per-state surface
 * sharing one declared box, blitted on a change, for the plaque's own reasons.
 *
 * INDEXED BY SLOT and TOTAL: a locked slot has no preview because the cursor
 * cannot land on it (`mnMapsCheckLocked` refuses the move, mnmaps.c:166), so
 * its entry is the NONE sentinel and the sync leaves the panel alone rather
 * than blitting art for a stage this build does not have. */
static const u8 kNdsSssPreviewSurface[NDS_SSS_SLOTS] = {
    (u8)NDS_MENU_VS_SURFACE_NONE, (u8)NDS_MENU_VS_SURFACE_NONE,
    (u8)NDS_MENU_VS_SURFACE_NONE, (u8)NDS_MENU_VS_SURFACE_NONE,
    (u8)NDS_MENU_VS_SURFACE_NONE, (u8)NDS_MENU_VS_SURFACE_NONE,
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PREVIEW_DREAM_LAND,
    (u8)NDS_MENU_VS_SURFACE_NONE, (u8)NDS_MENU_VS_SURFACE_NONE,
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_PREVIEW_RANDOM
};

static u8 sSssPlaqueSurface;
static u8 sSssPreviewSurface;

/* THE CURSOR IS THE ONLY OBJ LEFT ON THIS SCREEN. P2-1L (6) moved the ten grid
 * cells into the backdrop surface and (9) moved the preview panel into two
 * surfaces of its own, so the depth-ordered slot list this once needed is one
 * entry -- and it keeps id 0 because the cursor frame draws over the cell it
 * selects (see NDS_UI_KIT_SPRITE_SLOTS). */
#define NDS_SSS_SPRITE_CURSOR 0u

/* P2-1f's two colour constants are DELETED with this row, and the reason they
 * existed is the thing P2-1k fixed: the name plate's text is BLACK in the
 * source (mnmaps.c:600) and P2-1f had no plate to put it on, so it borrowed the
 * header's light tone. The plate is baked now (llMNMapsPlate{Left,Middle,Right}
 * over mnMapsLabelsProcDisplay's own drop-shadow fill), so the name is the
 * source's own sprite in the source's own black. */

static u32 sSssCursorSlot;
static u32 sSssScrollWait;

static void ndsMenuShellSssCue(u32 id)
{
    gNdsMenuShellSssCueCount++;
    gNdsMenuShellSssCueLastId = id;
    (void)ndsAudioFgmPlay((u16)id);
}

/* mnMapsCheckLocked, mnmaps.c:166 -- against this build's ground mask. RANDOM
 * is never locked, which is the source's own `else return FALSE`. */
static u32 ndsMenuShellSssGroundLocked(u32 gkind)
{
    if (gkind == NDS_SSS_GKIND_RANDOM)
    {
        return FALSE;
    }
    if (gkind > (u32)nGRKindInishie)
    {
        return TRUE;
    }
    return ((NDS_SSS_GROUND_MASK & (1u << gkind)) != 0u) ? FALSE : TRUE;
}

static u32 ndsMenuShellSssSlotLocked(u32 slot)
{
    return (slot >= NDS_SSS_SLOTS) ?
        TRUE : ndsMenuShellSssGroundLocked((u32)kNdsSssSlotGkind[slot]);
}

/* mnMapsGetSlot, mnmaps.c:471 -- the inverse, used only to restore the cursor
 * from gSCManagerSceneData.maps_vsmode_gkind on re-entry. */
static u32 ndsMenuShellSssSlotOfGkind(u32 gkind)
{
    u32 i;

    for (i = 0u; i < NDS_SSS_SLOTS; i++)
    {
        if ((u32)kNdsSssSlotGkind[i] == gkind)
        {
            return i;
        }
    }
    return 6u; /* nGRKindPupupu's slot: the only ground this build has. */
}

static s32 ndsMenuShellSssIconX(u32 slot)
{
    return (slot < NDS_SSS_ROW) ?
        (s32)((slot * (u32)NDS_SSS_ICON_PITCH)) + NDS_SSS_ICON_X0 :
        (s32)(((slot - NDS_SSS_ROW) * (u32)NDS_SSS_ICON_PITCH)) +
            NDS_SSS_ICON_X0;
}

static s32 ndsMenuShellSssIconY(u32 slot)
{
    return (slot < NDS_SSS_ROW) ? NDS_SSS_ICON_Y0 : NDS_SSS_ICON_Y1;
}

/* THE TWO SURFACES THE CURSOR OWNS, at most `budget` of them a frame.
 *
 * Shaped exactly like `ndsMenuShellVsSyncButtons`, and for the reason P2-1j
 * MEASURED there: re-blitting every changed surface on the frame the change
 * happens put 12,328 B of NitroFS inside one 60 Hz frame and priced it at
 * 606,336 ticks against a 560,190-tick budget. A move between Dream Land and
 * RANDOM changes BOTH surfaces (8,162 B of plaque and 11,748 B of preview), so
 * the same burst is available here; spending one a frame retires it in two
 * frames against a 12-tic scroll wait, and the CURSOR still moves on the press
 * because OAM costs nothing.
 *
 * A refused blit leaves the tracker alone so the next frame retries, rather
 * than recording a state the screen is not showing. */
static void ndsMenuShellSssSyncSurfaces(u32 budget)
{
    u8 list[2];
    u8 *tracker[2];
    u32 count = 0u;
    u32 i;
    u8 wanted;

    wanted = kNdsSssPlaqueSurface[sSssCursorSlot];
    if ((wanted != sSssPlaqueSurface) && (count < budget))
    {
        list[count] = wanted;
        tracker[count] = &sSssPlaqueSurface;
        count++;
    }
    wanted = kNdsSssPreviewSurface[sSssCursorSlot];
    if ((wanted != (u8)NDS_MENU_VS_SURFACE_NONE) &&
        (wanted != sSssPreviewSurface) && (count < budget))
    {
        list[count] = wanted;
        tracker[count] = &sSssPreviewSurface;
        count++;
    }
    if (count == 0u)
    {
        return;
    }
    if (ndsUiKitBlitSurfaces(list, count) == FALSE)
    {
        return;
    }
    for (i = 0u; i < count; i++)
    {
        *tracker[i] = list[i];
    }
    gNdsMenuShellSssPlaqueBlitCount += count;
}

/* mnMapsMakeNameAndEmblem, mnmaps.c:1015: the name follows the cursor, and
 * RANDOM has no name (the source draws only the question-mark emblem for it).
 * The preview panel follows it too -- mnMapsMakePreview:1096. */
static void ndsMenuShellSssShowSelection(u32 budget)
{
    u32 gkind = (u32)kNdsSssSlotGkind[sSssCursorSlot];

    ndsMenuShellSssSyncSurfaces(budget);
    ndsUiKitSetSprite(NDS_SSS_SPRITE_CURSOR, NDS_MN_UI_KIT_IMAGE_MAP_CURSOR,
                      NDS_SSS_DS(ndsMenuShellSssIconX(sSssCursorSlot) +
                                 NDS_SSS_CURSOR_DX),
                      NDS_SSS_DS(ndsMenuShellSssIconY(sSssCursorSlot) +
                                 NDS_SSS_CURSOR_DY));
    gNdsMenuShellSssCursorSlot = sSssCursorSlot;
    gNdsMenuShellSssCursorGkind = gkind;
}

static void ndsMenuShellSssPopulate(void)
{
    /* P2-1L (6): THE GRID IS BACKDROP ART NOW. `mnMapsMakeIcons` draws a 48x36
     * icon on a 50x38 pitch (mnmaps.c:540/:546), which is a near-continuous
     * mosaic; P2-1f's OBJ cells were 30x23 at 5/8 inside that 4/5 footprint,
     * so eight columns and six rows of stone showed inside every cell. The
     * lock set is a compile-time constant, so the whole grid is fixed for the
     * life of the screen and belongs in the surface -- where 4/5 costs no OBJ
     * cell. P2-1L (9) took the preview panel the same way, so the CURSOR is
     * the only OBJ this screen still owns and `ndsMenuShellHideRows` (called by
     * the screen entry, before this) is the only thing that has to clear the
     * previous screen's slots.
     *
     * The ENTRY frame spends both surfaces at once, as the VS menu's entry
     * does: it is already the frame that blits 98,304 B of SSS_SCREEN, and a
     * screen whose first present had no stage name or preview on it would show
     * the panel empty for two frames. */
    ndsMenuShellSssShowSelection(2u);
}

/* One cell of travel, with the source's own wrap. `step` is +1 or -1 and the
 * scan is bounded by the row length -- see the generalisation note above. */
static u32 ndsMenuShellSssStepRow(u32 slot, s32 step)
{
    u32 base = (slot < NDS_SSS_ROW) ? 0u : NDS_SSS_ROW;
    u32 index = slot - base;
    u32 tries;

    for (tries = 0u; tries < NDS_SSS_ROW; tries++)
    {
        index = (step > 0) ? ((index + 1u) % NDS_SSS_ROW) :
                             ((index + NDS_SSS_ROW - 1u) % NDS_SSS_ROW);
        if (ndsMenuShellSssSlotLocked(base + index) == FALSE)
        {
            return base + index;
        }
    }
    return slot;
}

static void ndsMenuShellSssMoveTo(u32 slot)
{
    if (slot == sSssCursorSlot)
    {
        gNdsMenuShellSssBlockedCount++;
        return;
    }
    sSssCursorSlot = slot;
    gNdsMenuShellSssMoveCount++;
    ndsMenuShellSssCue(NDS_SSS_FGM_SCROLL2);
    /* Budget ZERO: the cursor moves on the press, the surfaces follow from the
     * next frame's sync. This frame already carries the FGM cue's own sample
     * read, and P2-1j's measurement is that a cue frame is exactly the wrong
     * one to hang a NitroFS blit on. */
    ndsMenuShellSssShowSelection(0u);
}

/* mnMapsSaveSceneData, mnmaps.c:1367 -- through the P2-1a descriptor, which is
 * the battle's only input, rather than straight into the scene data. The
 * descriptor's STAGE FIELD is the only thing this screen writes: the fighters
 * belong to the character select and the rules to the VS menu, and
 * ndsMatchConfigApply re-installs the whole struct so the other two halves
 * survive untouched. */
static void ndsMenuShellSssCommit(void)
{
    u32 slot_gkind = (u32)kNdsSssSlotGkind[sSssCursorSlot];
    u32 gkind = slot_gkind;

    if (slot_gkind == NDS_SSS_GKIND_RANDOM)
    {
        /* The source's own pick: a ground that is neither locked nor the one
         * already loaded (mnmaps.c:1377). THE SECOND CLAUSE IS UNSATISFIABLE
         * IN THIS BUILD -- there is exactly one unlocked ground and it is
         * always the one already loaded -- so the do-while is bounded and
         * falls back to dropping the no-repeat clause, which is the weaker of
         * the two and the one with no meaning when there is only one place to
         * go. With two or more grounds landed (P2-4) the first arm resolves
         * and the fallback never runs; gNdsMenuShellSssRandomFallbackCount is
         * what says which one did, rather than a comment claiming it. */
        u32 tries;

        gkind = NDS_SSS_GKIND_RANDOM; /* "unresolved" -- never a ground id */
        for (tries = 0u; tries < 32u; tries++)
        {
            u32 pick = (u32)syUtilsRandTimeUCharRange((s32)nGRKindInishie + 1);

            if ((ndsMenuShellSssGroundLocked(pick) == FALSE) &&
                (pick != (u32)gSCManagerSceneData.gkind))
            {
                gkind = pick;
                gNdsMenuShellSssRandomCount++;
                break;
            }
        }
        if (gkind == NDS_SSS_GKIND_RANDOM)
        {
            u32 scan;

            gkind = (u32)nGRKindPupupu;
            for (scan = 0u; scan <= (u32)nGRKindInishie; scan++)
            {
                if (ndsMenuShellSssGroundLocked(scan) == FALSE)
                {
                    gkind = scan;
                    break;
                }
            }
            gNdsMenuShellSssRandomFallbackCount++;
        }
    }
    gNdsMatchConfig.gkind = (u8)gkind;
    ndsMatchConfigApply(&gNdsMatchConfig);
    /* mnMapsSaveSceneData's second write: the CURSOR's own ground, 0xde and
     * all, so a return trip opens on the cell the player last chose
     * (mnMapsInitVars:1418 reads it back). ndsMatchConfigApply owns
     * gSCManagerSceneData.gkind and does not touch this field. */
    gSCManagerSceneData.maps_vsmode_gkind = (u8)slot_gkind;
    gNdsMenuShellSssCommitGkind = gkind;
    gNdsMenuShellSssCommitSlotGkind = slot_gkind;
    gNdsMenuShellSssCommitCount++;
}

static void ndsMenuShellUpdateSss(u32 held, u32 taps)
{
    /* ONE surface a frame, and never more (see ndsMenuShellSssSyncSurfaces).
     * Ahead of the input gate on purpose: a move made on the frame before the
     * gate closes still has its surfaces retired. A frame on which nothing
     * changed compares two bytes and returns. */
    ndsMenuShellSssSyncSurfaces(1u);

    /* mnMapsFuncRun:1436 gates EVERYTHING on ten tics having passed, which is
     * what stops the A that left the character select from being read again
     * here. sMenuTics is this screen's own frame counter. */
    if (sMenuTics < (u32)NDS_SSS_INPUT_ARM_TICS)
    {
        return;
    }
    if (sSssScrollWait != 0u)
    {
        sSssScrollWait--;
    }
    /* mnmaps.c:1450: the wait is forced to zero the moment no direction is
     * held, so a tap always acts at once. */
    if ((held & (NDS_INPUT_UP | NDS_INPUT_DOWN | NDS_INPUT_LEFT |
                 NDS_INPUT_RIGHT)) == 0u)
    {
        sSssScrollWait = 0u;
    }

    /* A or START. mnmaps.c:1464 takes both, and it commits BEFORE it cues. */
    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        ndsMenuShellSssCommit();
        ndsMenuShellSssCue(NDS_SSS_FGM_CONFIRM);
        gNdsMenuShellSssConfirmCount++;
#if NDS_P2_MENU_WALK
        /* A lap is a completed menu pass into the match, counted where the
         * pass actually ends. P2-1d counted it on the VS screen and P2-1e
         * moved it to the character select for the same reason: this screen
         * is now the last stop before the battle. */
        gNdsMenuShellWalkLoops++;
#endif
        ndsMenuShellGoto((u32)nSCKindVSBattle);
        return;
    }
    /* B. mnmaps.c:1481 -- it commits on the way out too, and it plays NOTHING. */
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellSssCommit();
        gNdsMenuShellSssBackCount++;
        ndsMenuShellGoto((u32)nSCKindPlayersVS);
        return;
    }

    if (sSssScrollWait != 0u)
    {
        return;
    }
    /* The four direction arms in the source's own order (mnmaps.c:1494 down):
     * UP and DOWN change ROW and refuse a locked destination outright; LEFT
     * and RIGHT cycle within the row. Each arm reloads the wait to twelve and
     * returns, so exactly one arm runs per frame -- including the arms that
     * refuse, which is why a blocked press still costs the repeat delay. */
    if ((held & NDS_INPUT_UP) != 0u)
    {
        if ((sSssCursorSlot >= NDS_SSS_ROW) &&
            (ndsMenuShellSssSlotLocked(sSssCursorSlot - NDS_SSS_ROW) == FALSE))
        {
            ndsMenuShellSssMoveTo(sSssCursorSlot - NDS_SSS_ROW);
        }
        else
        {
            gNdsMenuShellSssBlockedCount++;
        }
        sSssScrollWait = (u32)NDS_SSS_SCROLL_WAIT;
        return;
    }
    if ((held & NDS_INPUT_DOWN) != 0u)
    {
        if ((sSssCursorSlot < NDS_SSS_ROW) &&
            (ndsMenuShellSssSlotLocked(sSssCursorSlot + NDS_SSS_ROW) == FALSE))
        {
            ndsMenuShellSssMoveTo(sSssCursorSlot + NDS_SSS_ROW);
        }
        else
        {
            gNdsMenuShellSssBlockedCount++;
        }
        sSssScrollWait = (u32)NDS_SSS_SCROLL_WAIT;
        return;
    }
    if ((held & NDS_INPUT_LEFT) != 0u)
    {
        ndsMenuShellSssMoveTo(ndsMenuShellSssStepRow(sSssCursorSlot, -1));
        sSssScrollWait = (u32)NDS_SSS_SCROLL_WAIT;
        return;
    }
    if ((held & NDS_INPUT_RIGHT) != 0u)
    {
        ndsMenuShellSssMoveTo(ndsMenuShellSssStepRow(sSssCursorSlot, 1));
        sSssScrollWait = (u32)NDS_SSS_SCROLL_WAIT;
    }
    (void)taps;
}

/* mnMapsInitVars, mnmaps.c:1404. The cursor opens on the cell the last visit
 * chose -- `maps_vsmode_gkind`, which is 0xde after a RANDOM pick -- and the
 * harness seeds that field to nGRKindPupupu for a cold boot
 * (scene_harness.c:39). The source keys this on `scene_prev` being
 * nSCKindPlayersVS or nSCKindPlayers1PTraining; only the first exists here. */
static void ndsMenuShellSssInit(void)
{
    sSssCursorSlot =
        ndsMenuShellSssSlotOfGkind((u32)gSCManagerSceneData.maps_vsmode_gkind);
    if (ndsMenuShellSssSlotLocked(sSssCursorSlot) != FALSE)
    {
        sSssCursorSlot = ndsMenuShellSssSlotOfGkind((u32)nGRKindPupupu);
    }
    sSssScrollWait = 0u;
    /* The screen entry re-blits SSS_SCREEN over BG2, which erases whatever
     * plaque the last visit left there -- so the cached "which plaque is on
     * screen" must be invalidated with it, or a re-entry on the same slot
     * would skip the blit and show a stage select with no name on it. Same
     * reason NDS_MENU_VS_SURFACE_NONE exists for the gates and the buttons. */
    sSssPlaqueSurface = (u8)NDS_MENU_VS_SURFACE_NONE;
    sSssPreviewSurface = (u8)NDS_MENU_VS_SURFACE_NONE;
    sSssEnterCount++;
}

/* --- The screen loop ----------------------------------------------------- */

/* P2-1h. A screen's backdrop art, drawn ONCE per entry into BG2.
 *
 * Separate from `ndsMenuShellPopulate` on purpose: populate re-runs on every
 * content change -- a cursor move, a value change -- and re-reading 84 KiB of
 * NitroFS on a keypress would put a scene load inside a 60 Hz frame. This runs
 * from `ndsMenuShellRun` only, after the overlay layers are cleared and the
 * backdrop colour is set, and then never again for the life of the screen.
 *
 * The two menus that take the collage are the two the SOURCE gives it to:
 * `mnModeSelectMakeDecals` (mnmodeselect.c:525) and `mnVSModeMakeBackground`
 * (mnvsmode.c:972), both at (10, 10). The character and stage selects have
 * their own source backgrounds and are not this row's. */
static const u8 kNdsMenuTitleSurfaces[] = {
    (u8)NDS_MN_UI_KIT_SURFACE_TITLE_SCREEN
};
/* P2-1i, owner finding (2). The main menu's own plate: one surface carrying
 * everything mnModeSelectMake* composes that the cursor does not change --
 * the collage, both decal bars, the MODE SELECT text, the SMASH emblem, the
 * four dark entry icons and the four red labels.
 *
 * P2-1j gave the VS menu the same treatment, so the BARE collage surface no
 * longer has a consumer and is no longer baked: both screens that show it now
 * show it inside their own composed plate. */
static const u8 kNdsMenuModeSelectSurfaces[] = {
    (u8)NDS_MN_UI_KIT_SURFACE_MODE_SELECT
};
/* P2-1i, owner finding (1). The character and stage selects sat on a flat
 * blue field; the source sits both of them on the SAME stone tile --
 * `llMNSelectCommonStoneBackgroundSprite` wrapped at its own 64x32 period
 * (masks 6, maskt 5), from mnplayersvs.c:1370 and mnmaps.c:356.
 *
 * P2-1k RETIRED the shared bare-stone surface: the stage select now has its
 * own composed plate exactly as the character select has, so the stone is the
 * first placement of each rather than a surface either has to blit.
 *
 * P2-1j. The VS rules screen's own plate -- collage, both decal papers, the
 * console-icon watermark, the menu-name fill rectangle and its three plaque
 * sprites (mnvsmode.c:965 and :909) -- with the four buttons blitted over it
 * by ndsMenuShellVsSyncButtons in whichever state the cursor puts them.
 *
 * P2-1j findings (c)/(d). The character select's stone now carries the twelve
 * portrait BOXES and the ten locked stacks, because neither ever changes while
 * the screen is up: `llMNPlayersPortraitsPortraitFireBgSprite` behind every
 * cell (mnplayersvs.c:2437/:2503) and, for a locked one, the fighter's noise-
 * dithered shadow and the question-mark plate over it (:2404). The stage
 * select keeps the plain stone -- it draws no portrait grid. */
static const u8 kNdsMenuVsSurfaces[] = {
    (u8)NDS_MN_UI_KIT_SURFACE_VS_MODE
};

static const u8 kNdsMenuCssSurfaces[] = {
    (u8)NDS_MN_UI_KIT_SURFACE_CSS_SCREEN
};

/* P2-1k (c). The stage select's own plate: the full-bleed stone, the preview
 * panel's fill and its seven tiles, the wooden plaque, both of
 * mnMapsLabelsProcDisplay's fills, the STAGE SELECT decal and the three-part
 * name plate -- everything mnMapsFuncStart composes that the cursor does not
 * change. The per-stage name and emblem ride the SSS_PLAQUE surfaces instead,
 * blitted by ndsMenuShellSssShowSelection. */
static const u8 kNdsMenuSssSurfaces[] = {
    (u8)NDS_MN_UI_KIT_SURFACE_SSS_SCREEN
};

static void ndsMenuShellEnterBackdrop(u32 screen)
{
    switch (screen)
    {
    case NDS_MENU_SHELL_SCREEN_TITLE:
        (void)ndsUiKitBlitSurfaces(kNdsMenuTitleSurfaces,
                                   (u32)(sizeof(kNdsMenuTitleSurfaces) /
                                         sizeof(kNdsMenuTitleSurfaces[0])));
        /* Cached rather than re-read: PRESS START toggles every 32 frames and
         * one NitroFS open costs more than a whole 60 Hz frame's budget. */
        (void)ndsUiKitCacheSurface(NDS_MN_UI_KIT_SURFACE_TITLE_PRESS_START);
        sMenuTitleBlinkPhase = 0u;
        ndsUiKitDrawCachedSurface();
        /* P2-1i: the fire atlas goes into BG3 at entry and BURNS FROM THE
         * FIRST PRESENT -- `mnTitleMakeFire` shows it during construction on
         * this branch (mntitle.c:990), before the scene's first tic, so a
         * title whose first frames are a black field is not the source's.
         * Enabling here rather than in the update is what guarantees frame 0
         * already has it: the loop's first Update runs after this. */
        (void)ndsUiKitBlitFireAtlas();
        ndsPlatformSetTitleFireEnabled(TRUE, (s32)NDS_MN_UI_KIT_FIRE_PA,
                                       (s32)NDS_MN_UI_KIT_FIRE_PD);
        /* Cell 0, matching what the loop's first update will select from
         * sMenuTics == 0 -- so the reference point never shows a cell the
         * timeline has not reached. */
        ndsMenuShellTitleFireFrame(0u);
        gNdsTitleFireRevealFrame =
            gNdsMenuShellFrames[NDS_MENU_SHELL_SCREEN_TITLE] + 1u;
        /* P2-1k (d). THE POP ANIMATION IS ARMED HERE, after the static title is
         * on the screen, because pose 1 is defined as the removal of it: the
         * source hides the label GObj until tic 170 and shows it already at
         * scale 0, so the first thing the animation owes the panel is the
         * settled wordmark's absence.
         *
         * ARENA, NOT .bss. The six rasters are 98,920 bytes and the binary's
         * size comes straight out of the taskman arena, one for one -- a
         * static buffer this size would cost every scene in the game, battle
         * included, for a screen that shows it for 0.85 s. `syTaskmanMalloc`
         * takes it out of the TITLE scene's own arena, which the scene
         * teardown rewinds, so the cost lands on the one screen that has it to
         * spare (title high-water 416,828 of 1,548,288 bytes).
         *
         * A refusal is not a failure path worth branching on: the animation
         * simply does not arm, the entry blit's settled layout stays, and the
         * screen still reads input and still reaches the mode select. The
         * counter is what tells the two apart. */
        {
            u32 anim_bytes = ndsUiKitTitleAnimBytes();
            void *anim_block = syTaskmanMalloc((size_t)anim_bytes, 4u);

            sMenuTitleVBlankBase = ndsPlatformVBlankCount();
            if (ndsUiKitTitleAnimLoad(anim_block, anim_bytes) != FALSE)
            {
                ndsUiKitTitleAnimDraw(1u);
            }
        }
        break;
    case NDS_MENU_SHELL_SCREEN_MODE:
        (void)ndsUiKitBlitSurfaces(kNdsMenuModeSelectSurfaces,
                                   (u32)(sizeof(kNdsMenuModeSelectSurfaces) /
                                         sizeof(kNdsMenuModeSelectSurfaces[0])));
        break;
    case NDS_MENU_SHELL_SCREEN_VSMODE:
        (void)ndsUiKitBlitSurfaces(kNdsMenuVsSurfaces,
                                   (u32)(sizeof(kNdsMenuVsSurfaces) /
                                         sizeof(kNdsMenuVsSurfaces[0])));
        break;
    case NDS_MENU_SHELL_SCREEN_CSS:
        (void)ndsUiKitBlitSurfaces(kNdsMenuCssSurfaces,
                                   (u32)(sizeof(kNdsMenuCssSurfaces) /
                                         sizeof(kNdsMenuCssSurfaces[0])));
        break;
    case NDS_MENU_SHELL_SCREEN_SSS:
        (void)ndsUiKitBlitSurfaces(kNdsMenuSssSurfaces,
                                   (u32)(sizeof(kNdsMenuSssSurfaces) /
                                         sizeof(kNdsMenuSssSurfaces[0])));
        break;
    default:
        break;
    }
}

static void ndsMenuShellPopulate(u32 screen)
{
    switch (screen)
    {
    case NDS_MENU_SHELL_SCREEN_TITLE:
        ndsMenuShellPopulateTitle();
        break;
    case NDS_MENU_SHELL_SCREEN_MODE:
        ndsMenuShellPopulateMode();
        break;
    case NDS_MENU_SHELL_SCREEN_CSS:
        ndsMenuShellPopulateCssScreen();
        break;
    case NDS_MENU_SHELL_SCREEN_SSS:
        ndsMenuShellSssPopulate();
        break;
    default:
        ndsMenuShellPopulateVs();
        break;
    }
}

static void ndsMenuShellUpdate(u32 screen, u32 held, u32 taps)
{
    switch (screen)
    {
    case NDS_MENU_SHELL_SCREEN_TITLE:
        ndsMenuShellUpdateTitle(held, taps);
        break;
    case NDS_MENU_SHELL_SCREEN_MODE:
        ndsMenuShellUpdateMode(held, taps);
        break;
    case NDS_MENU_SHELL_SCREEN_CSS:
        ndsMenuShellUpdateCss(held, taps);
        ndsMenuShellCssSyncPreviews();
        ndsMNPlayersVSPreviewFrame();
        break;
    case NDS_MENU_SHELL_SCREEN_SSS:
        ndsMenuShellUpdateSss(held, taps);
        break;
    default:
        ndsMenuShellUpdateVs(held, taps);
        break;
    }
}

static void ndsMenuShellRun(u32 screen)
{
    u32 enter_start = cpuGetTiming();

    sMenuScreen = screen;
    sMenuTics = 0u;
    sMenuChangeWait = 0u;
    sMenuLeaving = FALSE;
    sMenuNextScene = 0xffffffffu;
    sMenuFrameArmed = 0u;
    sMenuLastVBlank = ndsPlatformVBlankCount();
    sMenuFrameStartTicks = enter_start;
    /* The keypad's current state is the baseline, so a button still held from
     * the previous screen cannot read as a fresh tap on this one. */
    sMenuHeldPrev = ndsPlatformReadInput();
#if NDS_P2_MENU_WALK
    /* Every screen gets a full dwell BEFORE its first scripted input, not just
     * between inputs. Without it the title's START lands on the title's first
     * frame, which leaves no cadence window and nothing to capture. The step
     * cursor restarts here too, which is what lets the loop re-enter VS Mode
     * from Results and replay the whole tour. */
    sMenuWalkTimer = ((screen == NDS_MENU_SHELL_SCREEN_CSS) ||
                      (screen == NDS_MENU_SHELL_SCREEN_SSS)) ?
        NDS_MENU_WALK_DWELL_CSS : NDS_MENU_WALK_DWELL;
    sMenuWalkCursor = 0u;
    sMenuWalkHold = 0u;
    sMenuWalkHeld = 0u;
#endif

    gNdsMenuShellScreen = screen;
    gNdsMenuShellEnterCount[screen]++;

    /* Main BG0 is the retained 3D surface in MODE_5_3D. Only the character
     * select owns 3D inside this native menu shell; every other screen hides
     * it so CSS's last fighter frame cannot remain composited over Stage
     * Select or a VS/menu screen after the source GObjs are gone. Battle
     * explicitly reclaims BG0 at its own scene entry. */
    ndsPlatformSet3DLayerEnabled(
        (screen == NDS_MENU_SHELL_SCREEN_CSS) ? TRUE : FALSE);

    /* The battle's sprite compositor owns BG2/BG3, and a menu must not inherit
     * whatever the last battle frame left in them -- so both layers are
     * CLEARED. They are deliberately NOT hidden.
     *
     * MEASURED, and it cost a cycle to find: with the overlay disabled the
     * main OBJ layer does not reach the screen at all, even though DISPCNT
     * bit 12 reads 1, OAM holds valid 32x8 bitmap entries at priority 0, and
     * the composed texels are in bank E at the offset attr2 names
     * (0x06400000+24832, read back over GDB). The top screen measured
     * 0/49152 pixels differing from the clear colour on three separate
     * captures. Disabling the overlay is the ONE display-state difference
     * against P2-1c's demo, which renders the same kit through the same
     * ndsPlatformEndFrame: it takes the 3D clear to alpha 31 (opaque) and
     * hides BG2/BG3, the second alpha-blend target REG_BLDCNT names. Keeping
     * the overlay enabled restores the proven state -- transparent 3D clear,
     * both overlay layers present and empty -- and it is what makes the DS
     * backdrop (main BG palette entry 0) the visible field behind a menu. */
    ndsPlatformSetOriginalSpriteOverlayEnabled(TRUE);
    ndsPlatformClearOriginalSpriteOverlayLayer(FALSE);
    ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
    ndsPlatformClearBattleTextHud();

    /* A refusal here is counted in gNdsUiKitEnterRejectCount and is NOT a
     * reason to change the flow: every kit call below returns FALSE while the
     * kit is inactive, so the screen still reads input and still reaches its
     * successor -- it just draws nothing. A refusal that also re-routed the
     * player would turn one visible defect into two invisible ones. */
    (void)ndsUiKitEnter(NDS_UI_KIT_ENGINE_MAIN);
    ndsMenuShellHideRows();
    /* THE BACKDROP IS SET BEFORE THE ART, not after. A backdrop surface is
     * composited over its screen's own field at bake time, so the two have to
     * agree: the title's art is composited over black and every other screen's
     * collage over the source's decal blue. Setting it afterwards left one
     * entry frame showing the previous screen's field behind the new art. */
    BG_PALETTE[0] = (screen == NDS_MENU_SHELL_SCREEN_TITLE) ?
        NDS_MENU_BACKDROP_BLACK : NDS_MENU_BACKDROP_BLUE;
    /* The battle's fast wallpaper leaves BG2 under a 4/5 affine transform, and
     * the reset the clear above queues is only applied at the next present --
     * which is one frame AFTER this backdrop is drawn. Committing it here is
     * what stops a menu entered straight out of a battle showing one frame of
     * scaled artwork. */
    ndsPlatformCommitOriginalSpriteOverlayTransform();
    ndsMenuShellEnterBackdrop(screen);
    ndsMenuShellPopulate(screen);

    gNdsMenuShellEnterTicks[screen] = cpuGetTiming() - enter_start;

    while (sMenuLeaving == FALSE)
    {
        u32 held;
        u32 taps = ndsMenuShellReadTaps(&held);

        ndsMenuShellRecordInput(taps);
        ndsMenuShellUpdate(screen, held, taps);
        ndsMenuShellTickInput(held);
        sMenuTics++;

        /* P2-1k (g2). `ndsAudioBgmUpdate` has exactly two callers in the whole
         * tree before this one -- the battle's own per-frame update
         * (`ndsRunMarioFoxProofUpdate`, taskman_seam.c:4488) and the VS
         * Results dev-harness loop (taskman_seam.c:7364) -- so no menu shell
         * screen ever served a BGM refill: `ndsAudioBgmPlay` synchronously
         * preloads two packets (~1.5 s at 16,384 samples/22,050 Hz) and the
         * hardware-timer-driven worker thread (`ndsAudioBgmHandleSeam`,
         * nds_audio_bgm.c) can swap between those two once, but nothing ever
         * called `ndsAudioBgmServiceRefills` to load a third -- the seam miss
         * that follows sets `gNdsAudioBgmSeamMissCount`/`OverrunCount` and
         * `sNdsAudioBgmErrorPending` from the worker thread, but only
         * `ndsAudioBgmUpdate` (main thread) ever reads that flag and turns it
         * into `ndsAudioBgmFailPlayback`, so `gNdsAudioBgmPlaying` never even
         * dropped to 0 -- track 44/10 looked "stuck playing" while the
         * hardware channel had already gone silent underneath it. Menus are
         * the ONLY screens this file drives that need it: title never plays
         * BGM (P2-1k (g)) and every other native screen (mode select, VS
         * mode, character select, stage select) funnels through this one
         * loop. Matches the battle's own placement -- logic update before the
         * frame's render/present calls. */
        ndsAudioBgmUpdate();

        ndsPlatformRenderDebugHud();
        ndsMenuShellRecordFrame();
        ndsPlatformEndFrame();
        ndsMenuShellRecordPresent();
    }

    /* EVERY title exit path lands here (START/A is the only one today), so
     * this is where the fire hands BG3 back: disable restores the identity
     * affine and priority 0, and the NEXT screen's entry clears the bitmap
     * itself (the same clear above), which is what leaves the next owner the
     * transparent foreground overlay it expects. */
    if (screen == NDS_MENU_SHELL_SCREEN_TITLE)
    {
        ndsPlatformSetTitleFireEnabled(FALSE, 0, 0);
        /* P2-1k (d). Drops the pose table's view of the arena block; the block
         * itself belongs to the scene the teardown below rewinds. */
        ndsUiKitTitleAnimEnd();
    }
    if (screen == NDS_MENU_SHELL_SCREEN_CSS)
    {
        /* Clear the renderer's scene-local fighter registrations while the
         * PlayersVS arena is still valid; gcEjectAll below owns the camera and
         * every remaining GObj. */
        ndsMNPlayersVSPreviewExit();
        ndsPlatformSet3DLayerEnabled(FALSE);
    }

    ndsUiKitExit();
    BG_PALETTE[0] = NDS_MENU_BACKDROP_BLACK;
    gNdsMenuShellExitCount[screen]++;
    gNdsMenuShellScreen = 0xffffffffu;

    /* THE SOURCE'S OWN TEARDOWN. syTaskmanCommonTaskUpdate calls gcEjectAll
     * when the scene's loop breaks (decomp sys/taskman.c:1099), which is what
     * runs osDestroyThread before gcEjectGObjStack (objman.c:918). The bounded
     * branches this row replaces never reached it, which is how P2-1b-1's
     * stale-thread data abort happened; a native screen makes no GObjs of its
     * own, but the scene's own func_start did, so the teardown still has work
     * and is still the contract. */
    gcEjectAll();

    if (sMenuNextScene != 0xffffffffu)
    {
        ndsSceneManagerRequest(sMenuNextScene,
                               (u32)gSCManagerSceneData.scene_curr);
    }
}

void ndsMenuShellRunStartup(void)
{
    /* P2-1h MOVED THIS SEAM, and it is the one thing that had to move with the
     * splash rather than after it. The splash was the shell's earliest entry;
     * with the splash deleted, THIS is, and the load has to stay ahead of the
     * first cue or every menu SFX before the first battle resolves against an
     * empty pack again. Startup presents no frame -- boot reaches the title
     * directly, which is the N64 flow once the opening cinematic (P2-7) is
     * accounted for -- but the scene still EXISTS, because the source's own
     * startup func_start has already run and made GObjs by the time
     * syTaskmanRunTask is reached.
     *
     * P2-1d-1 ROOT CAUSE, not scoped to FGM 157: ndsAudioAssetLoadFenced (which
     * loads the FGM pack, nds_audio_fgm.c's ndsAudioFgmLoadFenced included) had
     * exactly three call sites before this one -- battleship_scvsbattle.c:469
     * and the two ndsR2HostBattlePrepare/is_battle_playable sites in
     * taskman_seam.c -- all battle-entry only. Every menu-shell FGM request
     * made before battle starts therefore found gNdsAudioFgmLoaded == 0 and
     * sNdsAudioFgmEntries[] all-zero: not a missing pack entry, a pack that had
     * not been asked to load yet. Measured with a throwaway GDB read at
     * ndsMenuShellRunModeSelect: `loaded=0 result=00000000 openfail=0
     * readfail=0 fmtfail=0` (every failure counter zero -- the loader was never
     * called, not called-and-failed) alongside `unsupported=1`, matching FGM
     * 157's miss ring entry exactly. This is why the P2-1c-1 kit's ids
     * (158/163/164/165) are NOT in the miss ring either: they are declared in
     * ndsAudioFgmIDIsIncluded, so the same unloaded-pack failure silently
     * incremented gNdsAudioFgmIncludedLookupFailCount instead of the miss ring
     * -- a defect the miss-ring-only P2-1c probe could not see. Startup is the
     * menu shell's own earliest entry (nSCKindStartup, scene kind 27, the
     * first scene of every run) and this call is idempotent
     * (sNdsAudioAssetLoaded guards it), so loading here once, before Title can
     * ever request the press-start cue, costs nothing on every later call and
     * makes every downstream FGM request resolve against a real pack instead
     * of an empty one. The pack lives in static .bss (sNdsAudioFgmMetadata /
     * sNdsAudioFgmEntries), not the taskman arena, so it survives every scene's
     * arena rewind after this one load. */
#if NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS
    ndsAudioAssetLoadFenced();
#endif
    gNdsMenuShellStartupCount++;

    /* THE SOURCE'S OWN TEARDOWN, for the same reason every screen runs it
     * (decomp sys/taskman.c:1099). This branch presents no frame, but the
     * startup scene's func_start ran before syTaskmanRunTask and its GObjs are
     * live in the arena the next scene entry rewinds -- which is precisely the
     * shape of the P2-1b-1 stale-thread data abort. Skipping it because "this
     * screen draws nothing" would reopen that bug. */
    gcEjectAll();
    /* Deliberately NOT written into the transition ring: the ring pairs a
     * SCREEN with the scene it hands off to, and no screen ran here. An empty
     * first ring slot is the evidence that boot reaches the title directly. */
    ndsSceneManagerRequest((u32)nSCKindTitle,
                           (u32)gSCManagerSceneData.scene_curr);
}

/* P2-1k (g), OWNER FINDING: "the background music is always playing".
 *
 * `mnTitleInitVars` (mntitle.c:340) is the source's answer and it is
 * unconditional on our branch: `scene_prev` can only be `nSCKindOpeningNewcomers`
 * when the opening cinematic ran, which is P2-7, so the `else` arm is the one
 * this build always takes and its first statement is `syAudioStopBGMAll()`
 * (:352). THE TITLE IS SILENT. This shell had no BGM call on the title at all,
 * so a track started downstream simply kept playing over it -- boot reached the
 * title with nothing playing and looked correct, but the main menu's own B
 * (mode select -> title) left BGM 44 running under the title screen, which is
 * exactly what the owner heard.
 *
 * Placed in the RunTitle wrapper rather than inside ndsMenuShellRun for the
 * same reason the mode select's play is: this is the title's func_start
 * equivalent, it runs once per entry, and it runs after
 * ndsSceneManagerRequest has written scene_prev. */
void ndsMenuShellRunTitle(void)
{
    ndsAudioBgmStopAll();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_TITLE);
}

/* mnmodeselect.c:882 (mnModeSelectFuncStart), transcribed exactly: the main
 * menu's own track plays on arrival at ModeSelect UNLESS the previous scene
 * is one of ModeSelect's own children -- 1P Game, VS Mode, Option, or Data --
 * in which case a track is presumably already playing and is left alone
 * rather than restarted. `scene_prev` is read here because this function
 * runs once, as ModeSelect's func_start-equivalent (wired from
 * syTaskmanRunTask, P2-1d), which is after ndsSceneManagerRequest has written
 * it and before anything else can change it -- the same ordering the source
 * itself relies on. Only VSMode is reachable as scene_prev in this build
 * today (1P Game/Option/Data are P2-1e/1f/1g); all four are transcribed
 * unconditionally so the condition matches the source exactly and needs no
 * revisiting when those scenes land. */
static void ndsMenuShellModeSelectPlayBgm(void)
{
    if (((u8)gSCManagerSceneData.scene_prev != (u8)nSCKind1PMode) &&
        ((u8)gSCManagerSceneData.scene_prev != (u8)nSCKindVSMode) &&
        ((u8)gSCManagerSceneData.scene_prev != (u8)nSCKindOption) &&
        ((u8)gSCManagerSceneData.scene_prev != (u8)nSCKindData))
    {
        ndsAudioBgmPlay(0, (s32)nSYAudioBGMModeSelect);
    }
}

void ndsMenuShellRunModeSelect(void)
{
    ndsMenuShellModeSelectPlayBgm();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_MODE);
}

/* P2-1k (g). mnVSModeFuncStart's tail, mnvsmode.c:1645: the VS menu starts the
 * mode-select track when -- and only when -- it was entered FROM the character
 * select. Arriving from the mode select it starts nothing, because that track
 * is already playing and restarting it would re-cue the intro; arriving back
 * from the CSS it must start, because `mnPlayersVSBackToVSMode` stopped all BGM
 * on the way out (mnplayersvs.c:3234, transcribed in ndsMenuShellCssBack). The
 * pair is what makes the music continuous across mode -> VS and restart across
 * VS -> CSS -> VS, and this shell had neither half. */
void ndsMenuShellRunVSMode(void)
{
    if ((u8)gSCManagerSceneData.scene_prev == (u8)nSCKindPlayersVS)
    {
        ndsAudioBgmPlay(0, (s32)nSYAudioBGMModeSelect);
    }
    ndsMenuShellVsLoadRules();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_VSMODE);
}

/* mnPlayersVSFuncStart's tail, mnplayersvs.c:4790-4798: the CSS starts its own
 * track unless it was entered FROM the stage select, then announces the mode
 * already carried by the transfer state.  Do not hard-code FREE FOR ALL here:
 * returning from Maps while Team Battle is selected must say TEAM BATTLE just
 * as the source does. */
static void ndsMenuShellCssPlayBgm(void)
{
    if ((u8)gSCManagerSceneData.scene_prev != (u8)nSCKindMaps)
    {
        ndsAudioBgmPlay(0, NDS_CSS_BGM_BATTLE_SELECT);
    }
    ndsMenuShellCssAnnounceMode();
}

void ndsMenuShellRunCharSelect(void)
{
    ndsMNPlayersVSPreviewInit();
    ndsMenuShellCssInit();
    ndsMenuShellCssSyncPreviews();
    ndsMenuShellCssPlayBgm();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_CSS);
}

/* mnMapsFuncStart, mnmaps.c:1591: no BGM call anywhere in it, so the character
 * select's track plays straight through. Nothing to start here, and saying so
 * is the point -- every other RunX above starts or deliberately skips one. */
void ndsMenuShellRunStageSelect(void)
{
    ndsMenuShellSssInit();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_SSS);
}

/* --- The mode-select scene ----------------------------------------------
 *
 * nSCKindModeSelect had no scene in this build at all: src/port/title_backend.c
 * carried it as an NDS_SCENE_STUB, whose whole body is osStopThread. This is
 * the real one, and it is deliberately the same shape as the imported menu
 * scenes (src/import/battleship_mnvsmode.c:308): the taskman setup is VS
 * Mode's own -- the object pools, DL buffer, graphics heap and RDP buffer a
 * menu scene declares in this build, already proven by P2-1b's walk -- with
 * the arena repointed at the shared taskman arena so every registered scene
 * rewinds the SAME memory and their high-waters stay comparable.
 *
 * `func_start` is NULL because the screen is native: the source's func_start
 * exists to load MNCommon/MNMain and build an SObj graph for gcDrawAll, and
 * this scene draws through the UI kit instead. */
extern SYTaskmanSetup dMNVSModeTaskmanSetup;
extern SYVideoSetup dMNVSModeVideoSetup;

void mnModeSelectStartScene(void)
{
    SYTaskmanSetup setup = dMNVSModeTaskmanSetup;

    dMNVSModeVideoSetup.zbuffer = SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNVSModeVideoSetup);

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = NULL;
    syTaskmanStartTask(&setup);
}

/* --- The character-select scene ------------------------------------------
 *
 * This one REPLACES an imported scene rather than filling an empty stub:
 * src/import/battleship_mnplayersvs.c compiles the original scene and its
 * `mnPlayersVSStartScene` runs the original `mnPlayersVSFuncStart`, which loads
 * seven menu files, calls ftManagerSetupFilesAllKind for all TWELVE fighters
 * and allocates four figatree heaps out of the scene arena (mnplayersvs.c:4750)
 * -- everything the RSP/RDP scene graph this build does not have would need.
 * That definition is compiled out at NDS_P2_MENU_SHELL and this is the one that
 * runs: the source's own taskman declaration for this scene, with the arena
 * repointed at the shared taskman arena so every registered scene rewinds the
 * SAME memory and the high-waters stay comparable, and `func_start = NULL`
 * because the screen draws through the UI kit. Same shape as the mode select
 * above, and the same shape P2-1d recorded as the template. */
extern SYTaskmanSetup dMNPlayersVSTaskmanSetup;
extern SYVideoSetup dMNPlayersVSVideoSetup;

void mnPlayersVSStartScene(void)
{
    SYTaskmanSetup setup = dMNPlayersVSTaskmanSetup;

    dMNPlayersVSVideoSetup.zbuffer =
        SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNPlayersVSVideoSetup);

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = NULL;
    syTaskmanStartTask(&setup);
}

/* --- The stage-select scene ----------------------------------------------
 *
 * The second scene this shell REPLACES rather than fills. The imported
 * `mnMapsStartScene` (src/import/battleship_mnmaps.c) runs the original
 * `mnMapsFuncStart`, which loads five menu files, allocates TWO model heaps
 * sized to the largest of the nine stage map files, and builds the wallpaper,
 * plaque, icon, name, cursor and 3D-preview object graph plus eight cameras
 * (mnmaps.c:1591) -- all of it for the RSP/RDP pipeline this build does not
 * have. That definition is compiled out at NDS_P2_MENU_SHELL and this is the
 * one that runs: the source's own taskman declaration for this scene, with the
 * arena repointed at the shared taskman arena so every registered scene
 * rewinds the SAME memory and the high-waters stay comparable, and
 * `func_start = NULL` because the screen draws through the UI kit. Same shape
 * as the two above. */
extern SYTaskmanSetup dMNMapsTaskmanSetup;
extern SYVideoSetup dMNMapsVideoSetup;

void mnMapsStartScene(void)
{
    SYTaskmanSetup setup = dMNMapsTaskmanSetup;

    dMNMapsVideoSetup.zbuffer = SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNMapsVideoSetup);

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = NULL;
    syTaskmanStartTask(&setup);
}

#endif /* NDS_P2_MENU_SHELL */
