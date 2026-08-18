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
#include <nds/nds_match_config.h>
#include <nds/nds_menu_shell.h>
#include <nds/nds_platform.h>
#include <nds/nds_scene.h>
#include <nds/nds_scene_manager.h>
#include <nds/nds_ui_kit.h>

#include "generated/mn_ui_kit.generated.inc"

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);

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
    gNdsMenuShellEnterTicks[NDS_MENU_SHELL_SCREEN_COUNT];
NDS_MENU_PUBLISHED volatile u32
    gNdsMenuShellTransitionRing[NDS_MENU_SHELL_RING];
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellTransitionCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellInputRing[NDS_MENU_SHELL_RING];
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellInputCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellDeniedCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCommitCount;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCommitRule;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCommitTime;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellCommitStocks;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellWalkSteps;
NDS_MENU_PUBLISHED volatile u32 gNdsMenuShellWalkLoops;

/* --- Per-screen frame accounting ----------------------------------------- */

static u32 sMenuScreen;
static u32 sMenuFrameStartTicks;
static u32 sMenuLastVBlank;
static u32 sMenuFrameArmed;
static u32 sMenuHeldPrev;
static u32 sMenuTics;
static u32 sMenuNextScene;
static u32 sMenuLeaving;

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

#if NDS_P2_MENU_WALK
/* Scripted input, lab only. It drives the SCREENS' OWN handlers, not the scene
 * manager: every step below is a button the player could press, so a walk that
 * reaches the battle is proof the handlers reached it, not proof that a hop
 * counter did. `gNdsMenuShellInputRing` pairs each acted-on tap with the
 * screen that consumed it. */
#define NDS_MENU_WALK_DWELL 150u

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
static const u16 kNdsMenuWalkTitle[] = { (u16)NDS_INPUT_START };
static const u16 kNdsMenuWalkMode[] = {
    (u16)NDS_INPUT_DOWN, (u16)NDS_INPUT_A
};
static const u16 kNdsMenuWalkVs[] = {
    (u16)NDS_INPUT_DOWN,  /* cursor: VS START -> RULE            */
    (u16)NDS_INPUT_RIGHT, /* rule: TIME -> STOCK                 */
    (u16)NDS_INPUT_LEFT,  /* rule: STOCK -> TIME (net zero)      */
    (u16)NDS_INPUT_DOWN,  /* cursor: RULE -> TIME/STOCK value    */
    (u16)NDS_INPUT_RIGHT, /* value: 1 -> 2                       */
    (u16)NDS_INPUT_LEFT,  /* value: 2 -> 1 (net zero)            */
    (u16)NDS_INPUT_DOWN,  /* cursor: value -> VS OPTIONS         */
    (u16)NDS_INPUT_A,     /* refusal: VS OPTIONS is not built    */
    (u16)NDS_INPUT_UP, (u16)NDS_INPUT_UP, (u16)NDS_INPUT_UP,
    (u16)NDS_INPUT_A      /* VS START -> the match               */
};

static const u16 *kNdsMenuWalkScripts[NDS_MENU_SHELL_SCREEN_COUNT] = {
    NULL, kNdsMenuWalkTitle, kNdsMenuWalkMode, kNdsMenuWalkVs
};
static const u8 kNdsMenuWalkLengths[NDS_MENU_SHELL_SCREEN_COUNT] = {
    0u,
    (u8)(sizeof(kNdsMenuWalkTitle) / sizeof(kNdsMenuWalkTitle[0])),
    (u8)(sizeof(kNdsMenuWalkMode) / sizeof(kNdsMenuWalkMode[0])),
    (u8)(sizeof(kNdsMenuWalkVs) / sizeof(kNdsMenuWalkVs[0]))
};

static u32 sMenuWalkCursor;
static u32 sMenuWalkTimer;

static u32 ndsMenuShellWalkTap(u32 screen)
{
    u32 length;

    if (gNdsMenuShellWalkLoops >= (u32)NDS_P2_MENU_WALK)
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
    length = (u32)kNdsMenuWalkLengths[screen];
    if ((length == 0u) || (sMenuWalkCursor >= length))
    {
        return 0u;
    }
    if (sMenuWalkTimer != 0u)
    {
        sMenuWalkTimer--;
        return 0u;
    }
    sMenuWalkTimer = NDS_MENU_WALK_DWELL;
    gNdsMenuShellWalkSteps++;
    sMenuWalkCursor++;
    if ((screen == NDS_MENU_SHELL_SCREEN_VSMODE) &&
        (sMenuWalkCursor == length))
    {
        /* A lap is a completed menu pass into the match, counted where the
         * pass actually ends rather than where a ring happens to wrap. */
        gNdsMenuShellWalkLoops++;
    }
    return (u32)kNdsMenuWalkScripts[screen][sMenuWalkCursor - 1u];
}
#endif /* NDS_P2_MENU_WALK */

static u32 ndsMenuShellReadTaps(u32 *out_held)
{
    u32 held = ndsPlatformReadInput();
    u32 taps = held & ~sMenuHeldPrev;

    sMenuHeldPrev = held;
#if NDS_P2_MENU_WALK
    {
        u32 injected = ndsMenuShellWalkTap(sMenuScreen);

        taps |= injected;
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

/* The kit's text path has no digits: '0'-'9' are the source's own kerning
 * ESCAPES and advance without drawing (mnmaps.c:308). That is exactly what a
 * label needs when a NUMBER SPRITE is going to be drawn into the gap, so a
 * label like "TIME." followed by a value followed by "MIN" is one string with
 * the value's width spelled out in escapes. Nothing here invents a mechanism:
 * it uses the one the original font already carries. */
static void ndsMenuShellAppendGap(char *dst, u32 capacity, u32 *len, u32 pixels)
{
    while ((pixels > 0u) && (*len + 1u < capacity))
    {
        u32 step = (pixels > 9u) ? 9u : pixels;

        dst[*len] = (char)('0' + step);
        (*len)++;
        pixels -= step;
    }
    dst[*len] = '\0';
}

static void ndsMenuShellAppend(char *dst, u32 capacity, u32 *len,
                               const char *text)
{
    while ((*text != '\0') && (*len + 1u < capacity))
    {
        dst[*len] = *text;
        (*len)++;
        text++;
    }
    dst[*len] = '\0';
}

static s32 ndsMenuShellCentre(const char *text)
{
    return (s32)((256u - ndsUiKitTextWidth(text)) / 2u);
}

static void ndsMenuShellHideRows(void)
{
    u32 slot;

    for (slot = 0u; slot < 6u; slot++)
    {
        ndsUiKitHideText(slot);
    }
    for (slot = 0u; slot < 8u; slot++)
    {
        ndsUiKitHideSprite(slot);
    }
}

/* --- Screen: splash ------------------------------------------------------
 *
 * The source's boot scene is the N64 logo (mnstartup.c:230). That is
 * first-party branding this project must not ship, so the slot carries a
 * Smash64DS identity card instead: the project wordmark, briefly, skippable,
 * which is the same role the original screen plays. */
#define NDS_MENU_SPLASH_FRAMES 110u

static void ndsMenuShellPopulateSplash(void)
{
    s32 head_w = (s32)ndsUiKitTextWidth("SMASH");
    s32 tail_w = (s32)ndsUiKitTextWidth("DS");
    s32 num_w = (s32)(2 * NDS_UI_KIT_DIGIT_PITCH);
    s32 x = (256 - (head_w + num_w + tail_w)) / 2;

    ndsUiKitSetText(NDS_MENU_SLOT_ROW0, "SMASH", NDS_MENU_RGB_WHITE);
    ndsUiKitMoveText(NDS_MENU_SLOT_ROW0, x, 90);
    /* "64" as two digit sprites: the wordmark carries digits and the menu font
     * does not have any. */
    (void)ndsUiKitSetNumber(NDS_MENU_SPRITE_NUM0, 2u, 64,
                            x + head_w + num_w, 86);
    ndsUiKitSetText(NDS_MENU_SLOT_ROW1, "DS", NDS_MENU_RGB_WHITE);
    ndsUiKitMoveText(NDS_MENU_SLOT_ROW1, x + head_w + num_w, 90);
}

static void ndsMenuShellUpdateSplash(u32 held, u32 taps)
{
    (void)held;
    if ((sMenuTics >= NDS_MENU_SPLASH_FRAMES) ||
        ((taps & (NDS_INPUT_A | NDS_INPUT_B | NDS_INPUT_START)) != 0u))
    {
        ndsMenuShellGoto((u32)nSCKindTitle);
    }
}

/* --- Screen: title -------------------------------------------------------
 *
 * The source's title screen shows the SUPER SMASH BROS. logo and a blinking
 * PRESS START, and takes A or START to the mode select (mntitle.c:490). It
 * plays no BGM of its own -- mnTitleInitVars calls syAudioStopBGMAll when the
 * previous scene is not the opening movie (mntitle.c:352), because the music
 * heard over the original title is the opening cinematic's, and that cinematic
 * is deferred to P2-7. A silent title is therefore the source's behaviour, not
 * a gap. Its confirm cue (FGM 157) is a gap and is named as one: the FGM pack
 * carries 158/163/164/165 and not 157, so the request lands in the miss ring
 * and row P2-1d-1 renders it. */
#define NDS_MENU_TITLE_BLINK 32u

static void ndsMenuShellPopulateTitle(void)
{
    ndsUiKitSetText(NDS_MENU_SLOT_ROW0, "SUPER SMASH BROS.",
                    NDS_MENU_RGB_WHITE);
    ndsUiKitMoveText(NDS_MENU_SLOT_ROW0,
                     ndsMenuShellCentre("SUPER SMASH BROS."), 66);
    ndsUiKitSetText(NDS_MENU_SLOT_ROW1, "PRESS START", NDS_MENU_RGB_VALUE);
    ndsUiKitMoveText(NDS_MENU_SLOT_ROW1, ndsMenuShellCentre("PRESS START"),
                     138);
}

static void ndsMenuShellUpdateTitle(u32 held, u32 taps)
{
    (void)held;
    if (((sMenuTics / NDS_MENU_TITLE_BLINK) & 1u) == 0u)
    {
        ndsUiKitSetText(NDS_MENU_SLOT_ROW1, "PRESS START", NDS_MENU_RGB_VALUE);
        ndsUiKitMoveText(NDS_MENU_SLOT_ROW1,
                         ndsMenuShellCentre("PRESS START"), 138);
    }
    else
    {
        ndsUiKitHideText(NDS_MENU_SLOT_ROW1);
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
 * are drawn locked and refuse with MenuDenied -- the id the source itself
 * spends on a refused selection.
 *
 * The five-minute idle return to the title (mnmodeselect.c:702) is attract
 * behaviour and belongs to P2-7; it is deliberately absent rather than
 * stubbed. */
#define NDS_MENU_MODE_ENTRIES 4u
#define NDS_MENU_MODE_VS 1u

static const char *const kNdsMenuModeLabels[NDS_MENU_MODE_ENTRIES] = {
    "P GAME", "VS MODE", "OPTION", "DATA"
};

static u32 sMenuModeCursor;

static void ndsMenuShellPopulateMode(void)
{
    u32 i;

    ndsUiKitSetText(NDS_MENU_SLOT_HEADER, "MODE SELECT", NDS_MENU_RGB_HEADER);
    ndsUiKitMoveText(NDS_MENU_SLOT_HEADER, NDS_MENU_HEADER_X,
                     NDS_MENU_HEADER_Y);

    for (i = 0u; i < NDS_MENU_MODE_ENTRIES; i++)
    {
        s32 x = NDS_MENU_MODE_X0 + ((s32)i * NDS_MENU_MODE_DX);
        s32 y = NDS_MENU_MODE_Y0 + ((s32)i * NDS_MENU_MODE_DY);
        u32 rgb;

        if (i != NDS_MENU_MODE_VS)
        {
            rgb = NDS_MENU_RGB_LOCKED;
        }
        else
        {
            rgb = (i == sMenuModeCursor) ? NDS_MENU_RGB_WHITE :
                                           NDS_MENU_RGB_RED;
        }
        ndsUiKitSetText(NDS_MENU_SLOT_ROW0 + i, kNdsMenuModeLabels[i], rgb);
        ndsUiKitMoveText(NDS_MENU_SLOT_ROW0 + i, x, y);
    }

    /* "1P GAME" carries a digit the menu font does not have; the source drew
     * the whole label as one sprite. The 1 is the digit sprite and the rest is
     * text, which is why row 0's string starts at "P". */
    (void)ndsUiKitSetNumber(NDS_MENU_SPRITE_NUM0, 1u, 1,
                            NDS_MENU_MODE_X0, NDS_MENU_MODE_Y0 - 4);

    ndsUiKitSetSprite(NDS_MENU_SPRITE_CURSOR,
                      NDS_MN_UI_KIT_IMAGE_CURSOR_HAND_POINT,
                      NDS_MENU_MODE_X0 +
                          ((s32)sMenuModeCursor * NDS_MENU_MODE_DX) +
                          NDS_MENU_CURSOR_DX_DIGIT,
                      NDS_MENU_MODE_Y0 +
                          ((s32)sMenuModeCursor * NDS_MENU_MODE_DY) +
                          NDS_MENU_CURSOR_DY);
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
 * TWO DELIBERATE NARROWINGS, both plan non-goals rather than omissions:
 * the rule range stops at STOCK instead of continuing to TIME TEAM and STOCK
 * TEAM, because teams need the four-fighter engine (P2-2) -- the source
 * already clamps this range, so the narrowing is a different bound and not a
 * different mechanism; and VS OPTIONS keeps its row and its place in the
 * cursor cycle but refuses, because handicap/damage/item-switch are P2-5/P2-7.
 *
 * The rules this screen commits are the descriptor fields it owns -- rule,
 * time limit, stock count -- written into gNdsMatchConfig and installed with
 * ndsMatchConfigApply. The fighters stay the preset's until P2-1e's character
 * select fills them. */
#define NDS_MENU_VS_ENTRIES 4u
#define NDS_MENU_VS_START 0u
#define NDS_MENU_VS_RULE 1u
#define NDS_MENU_VS_VALUE 2u
#define NDS_MENU_VS_OPTIONS 3u

#define NDS_MENU_RULE_TIME 0u
#define NDS_MENU_RULE_STOCK 1u
#define NDS_MENU_RULE_MAX NDS_MENU_RULE_STOCK

#define NDS_MENU_TIME_MAX 100 /* SCBATTLE_TIMELIMIT_INFINITE */
#define NDS_MENU_STOCK_MAX 98

static u32 sMenuVsCursor;
static u32 sMenuVsRule;
static s32 sMenuVsTime;
static s32 sMenuVsStock;

static u32 ndsMenuShellVsIsTime(void)
{
    return (sMenuVsRule == NDS_MENU_RULE_TIME) ? TRUE : FALSE;
}

static s32 ndsMenuShellVsValue(void)
{
    return (ndsMenuShellVsIsTime() != FALSE) ? sMenuVsTime : (sMenuVsStock + 1);
}

static void ndsMenuShellPopulateVs(void)
{
    char buffer[40];
    u32 len = 0u;
    s32 value = ndsMenuShellVsValue();
    s32 label_x = NDS_MENU_VS_X0 + (2 * NDS_MENU_VS_DX);
    s32 label_y = NDS_MENU_VS_Y0 + (2 * NDS_MENU_VS_DY);
    s32 digits_right;
    u32 i;
    u32 used;

    ndsUiKitSetText(NDS_MENU_SLOT_HEADER, "VS MODE", NDS_MENU_RGB_HEADER);
    ndsUiKitMoveText(NDS_MENU_SLOT_HEADER, NDS_MENU_HEADER_X,
                     NDS_MENU_HEADER_Y);

    ndsUiKitSetText(NDS_MENU_SLOT_ROW0, "VS START",
                    (sMenuVsCursor == NDS_MENU_VS_START) ? NDS_MENU_RGB_WHITE :
                                                           NDS_MENU_RGB_RED);
    ndsUiKitMoveText(NDS_MENU_SLOT_ROW0, NDS_MENU_VS_X0, NDS_MENU_VS_Y0);

    ndsUiKitSetText(NDS_MENU_SLOT_ROW1,
                    (ndsMenuShellVsIsTime() != FALSE) ? "RULE. TIME" :
                                                        "RULE. STOCK",
                    (sMenuVsCursor == NDS_MENU_VS_RULE) ? NDS_MENU_RGB_WHITE :
                                                          NDS_MENU_RGB_RED);
    ndsUiKitMoveText(NDS_MENU_SLOT_ROW1, NDS_MENU_VS_X0 + NDS_MENU_VS_DX,
                     NDS_MENU_VS_Y0 + NDS_MENU_VS_DY);

    /* "TIME." + <value gap> + "MIN", one slot, the gap spelled in the font's
     * own kerning escapes so the digit sprites drop into it. */
    ndsMenuShellAppend(buffer, sizeof(buffer), &len,
                       (ndsMenuShellVsIsTime() != FALSE) ? "TIME." : "STOCK.");
    ndsMenuShellAppendGap(buffer, sizeof(buffer), &len,
                          4u + ((value == NDS_MENU_TIME_MAX) ?
                                26u : ndsUiKitNumberWidth(value)) + 4u);
    if (ndsMenuShellVsIsTime() != FALSE)
    {
        ndsMenuShellAppend(buffer, sizeof(buffer), &len, "MIN");
    }
    ndsUiKitSetText(NDS_MENU_SLOT_ROW2, buffer,
                    (sMenuVsCursor == NDS_MENU_VS_VALUE) ? NDS_MENU_RGB_WHITE :
                                                           NDS_MENU_RGB_RED);
    ndsUiKitMoveText(NDS_MENU_SLOT_ROW2, label_x, label_y);

    ndsUiKitSetText(NDS_MENU_SLOT_ROW3, "VS OPTIONS",
                    (sMenuVsCursor == NDS_MENU_VS_OPTIONS) ?
                        NDS_MENU_RGB_GREY : NDS_MENU_RGB_LOCKED);
    ndsUiKitMoveText(NDS_MENU_SLOT_ROW3, NDS_MENU_VS_X0 +
                         (3 * NDS_MENU_VS_DX),
                     NDS_MENU_VS_Y0 + (3 * NDS_MENU_VS_DY));

    for (i = NDS_MENU_SPRITE_NUM0; i < NDS_MENU_SPRITE_NUM2 + 1u; i++)
    {
        ndsUiKitHideSprite(i);
    }
    digits_right = label_x +
        (s32)ndsUiKitTextWidth((ndsMenuShellVsIsTime() != FALSE) ? "TIME." :
                                                                   "STOCK.");
    digits_right += 4;
    if (value == NDS_MENU_TIME_MAX)
    {
        /* The source draws the infinity glyph instead of a number when the
         * time limit is SCBATTLE_TIMELIMIT_INFINITE (mnvsmode.c:1206). */
        ndsUiKitSetSprite(NDS_MENU_SPRITE_NUM0, NDS_MN_UI_KIT_IMAGE_INFINITY,
                          digits_right, label_y - 2);
    }
    else
    {
        used = ndsUiKitSetNumber(NDS_MENU_SPRITE_NUM0, 2u, value,
                                 digits_right + (s32)ndsUiKitNumberWidth(value),
                                 label_y - 4);
        (void)used;
    }

    ndsUiKitSetSprite(NDS_MENU_SPRITE_CURSOR,
                      NDS_MN_UI_KIT_IMAGE_CURSOR_HAND_POINT,
                      NDS_MENU_VS_X0 + ((s32)sMenuVsCursor * NDS_MENU_VS_DX) +
                          NDS_MENU_CURSOR_DX,
                      NDS_MENU_VS_Y0 + ((s32)sMenuVsCursor * NDS_MENU_VS_DY) +
                          NDS_MENU_CURSOR_DY);
}

/* mnVSModeFuncStartVars: the screen opens on the rules the battle state
 * already carries, so a return trip shows what the last visit committed. */
static void ndsMenuShellVsLoadRules(void)
{
    sMenuVsCursor = NDS_MENU_VS_START;
    sMenuVsRule = (gSCManagerTransferBattleState.game_rules ==
                   SCBATTLE_GAMERULE_TIME) ? NDS_MENU_RULE_TIME :
                                             NDS_MENU_RULE_STOCK;
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
    gNdsMatchConfig.is_team_battle = FALSE;
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
        ndsMenuShellPopulateVs();
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
    ndsMenuShellPopulateVs();
}

static void ndsMenuShellUpdateVs(u32 held, u32 taps)
{
    u32 moved = FALSE;

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
        ndsMenuShellPopulateVs();
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
            /* The walk spends its hop here when it is armed, so the P2-1b
             * loop budget still counts two hops a loop; with the walk off it
             * returns FALSE and the screen makes the transition itself. Both
             * arms request the same scene. */
            if (ndsSceneWalkAdvance((u32)nSCKindVSBattle) == FALSE)
            {
                ndsMenuShellGoto((u32)nSCKindVSBattle);
            }
            else
            {
                u32 slot = gNdsMenuShellTransitionCount %
                           NDS_MENU_SHELL_RING;

                gNdsMenuShellTransitionRing[slot] =
                    ((sMenuScreen & 0xffu) << 8) |
                    ((u32)nSCKindVSBattle & 0xffu);
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

/* --- The screen loop ----------------------------------------------------- */

static void ndsMenuShellPopulate(u32 screen)
{
    switch (screen)
    {
    case NDS_MENU_SHELL_SCREEN_SPLASH:
        ndsMenuShellPopulateSplash();
        break;
    case NDS_MENU_SHELL_SCREEN_TITLE:
        ndsMenuShellPopulateTitle();
        break;
    case NDS_MENU_SHELL_SCREEN_MODE:
        ndsMenuShellPopulateMode();
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
    case NDS_MENU_SHELL_SCREEN_SPLASH:
        ndsMenuShellUpdateSplash(held, taps);
        break;
    case NDS_MENU_SHELL_SCREEN_TITLE:
        ndsMenuShellUpdateTitle(held, taps);
        break;
    case NDS_MENU_SHELL_SCREEN_MODE:
        ndsMenuShellUpdateMode(held, taps);
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
    sMenuWalkTimer = NDS_MENU_WALK_DWELL;
    sMenuWalkCursor = 0u;
#endif

    gNdsMenuShellScreen = screen;
    gNdsMenuShellEnterCount[screen]++;

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
    ndsMenuShellPopulate(screen);
    BG_PALETTE[0] = (screen == NDS_MENU_SHELL_SCREEN_SPLASH) ?
        NDS_MENU_BACKDROP_BLACK : NDS_MENU_BACKDROP_BLUE;

    gNdsMenuShellEnterTicks[screen] = cpuGetTiming() - enter_start;

    while (sMenuLeaving == FALSE)
    {
        u32 held;
        u32 taps = ndsMenuShellReadTaps(&held);

        ndsMenuShellRecordInput(taps);
        ndsMenuShellUpdate(screen, held, taps);
        ndsMenuShellTickInput(held);
        sMenuTics++;

        ndsPlatformRenderDebugHud();
        ndsMenuShellRecordFrame();
        ndsPlatformEndFrame();
        ndsMenuShellRecordPresent();
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

void ndsMenuShellRunSplash(void)
{
    /* P2-1d-1 ROOT CAUSE, not scoped to FGM 157: ndsAudioAssetLoadFenced (which
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
     * -- a defect the miss-ring-only P2-1c probe could not see. Splash is the
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
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_SPLASH);
}

void ndsMenuShellRunTitle(void)
{
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

void ndsMenuShellRunVSMode(void)
{
    ndsMenuShellVsLoadRules();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_VSMODE);
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

#endif /* NDS_P2_MENU_SHELL */
