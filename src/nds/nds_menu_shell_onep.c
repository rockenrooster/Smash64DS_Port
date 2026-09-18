/* P2-6 -- DS-native presentation for BattleShip's 1P character select.
 *
 * Behaviour remains in mnplayers1pgame.c.  That source scene owns every
 * cursor hit test, fighter selection, setting, costume change and transition;
 * this file only maps its evaluated state onto the same AOT menu assets used
 * by the native VS CSS.  No N64 display list or sprite-compositor path is
 * needed for the 2D screen. */

#include <nds/arm9/video.h>

#include <nds/nds_menu_shell.h>
#include <nds/nds_platform.h>
#include <nds/nds_ui_kit.h>

#include "generated/mn_ui_kit.generated.inc"

#define NDS_ONEP_DS(v) (((v) * 4) / 5)

/* Low OAM id wins ties, matching the VS CSS: player tag over hand, then puck,
 * settings arrows/value and READY prompt. */
#define NDS_ONEP_SPR_CURSOR_TAG 0u
#define NDS_ONEP_SPR_CURSOR 1u
#define NDS_ONEP_SPR_PUCK 2u
#define NDS_ONEP_SPR_LEVEL_L 3u
#define NDS_ONEP_SPR_LEVEL_R 4u
#define NDS_ONEP_SPR_STOCK_L 5u
#define NDS_ONEP_SPR_STOCK_R 6u
#define NDS_ONEP_SPR_STOCK_VALUE 7u
#define NDS_ONEP_SPR_READY_PRESS 8u
#define NDS_ONEP_SPR_READY_START 9u

#define NDS_ONEP_STATE_NONE 0xffffffffu
#define NDS_ONEP_TIME_INFINITE 100u

static u32 sNdsOnePlayerCssActive;
static u32 sNdsOnePlayerCssLastFkind = NDS_ONEP_STATE_NONE;
static u32 sNdsOnePlayerCssLastTime = NDS_ONEP_STATE_NONE;
static u32 sNdsOnePlayerCssLastDifficulty = NDS_ONEP_STATE_NONE;
static u32 sNdsOnePlayerCssLastReady = NDS_ONEP_STATE_NONE;
static u32 sNdsOnePlayerCssFighterMask;

volatile u32 gNdsOnePlayerCssNativeEnterCount;
volatile u32 gNdsOnePlayerCssNativePresentCount;
volatile u32 gNdsOnePlayerCssNativeExitCount;
volatile u32 gNdsOnePlayerCssNativeBaseBlitCount;
volatile u32 gNdsOnePlayerCssNativeStateBlitCount;
volatile u32 gNdsOnePlayerCssNativeSurfaceFailCount;
volatile u32 gNdsOnePlayerCssNativeLockedBlitCount;
volatile u32 gNdsOnePlayerCssNativeLastFkind;
volatile u32 gNdsOnePlayerCssNativeLastDifficulty;
volatile u32 gNdsOnePlayerCssNativeLastStock;
volatile u32 gNdsOnePlayerCssNativeLastTime;
volatile u32 gNdsOnePlayerCssNativeVisibleMask;

static const NdsUiKitSurfaceId sNdsOnePlayerGateSurface[
    NDS_MENU_SHELL_FIGHTER_KINDS] = {
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_MARIO,
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_FOX,
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_DONKEY,
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_SAMUS,
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_LUIGI,
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_LINK,
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_YOSHI,
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_CAPTAIN,
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_KIRBY,
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_PIKACHU,
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_PURIN,
    NDS_MN_UI_KIT_SURFACE_ONEP_GATE_NESS
};

static const NdsUiKitSurfaceId sNdsOnePlayerLevelSurface[5] = {
    NDS_MN_UI_KIT_SURFACE_ONEP_LEVEL_VERY_EASY,
    NDS_MN_UI_KIT_SURFACE_ONEP_LEVEL_EASY,
    NDS_MN_UI_KIT_SURFACE_ONEP_LEVEL_NORMAL,
    NDS_MN_UI_KIT_SURFACE_ONEP_LEVEL_HARD,
    NDS_MN_UI_KIT_SURFACE_ONEP_LEVEL_VERY_HARD
};

static s32 ndsMenuShellOnePlayerCssBlit(NdsUiKitSurfaceId surface)
{
    if (ndsUiKitBlitSurfaces(&surface, 1u) == FALSE)
    {
        gNdsOnePlayerCssNativeSurfaceFailCount++;
        return FALSE;
    }
    gNdsOnePlayerCssNativeStateBlitCount++;
    return TRUE;
}

static void ndsMenuShellOnePlayerCssApplyLocks(u32 fighter_mask)
{
    static const struct {
        u8 fkind;
        NdsUiKitSurfaceId surface;
    } locked[] = {
        { 4u, NDS_MN_UI_KIT_SURFACE_CSS_LOCKED_LUIGI },
        { 7u, NDS_MN_UI_KIT_SURFACE_CSS_LOCKED_CAPTAIN },
        { 11u, NDS_MN_UI_KIT_SURFACE_CSS_LOCKED_NESS },
        { 10u, NDS_MN_UI_KIT_SURFACE_CSS_LOCKED_PURIN }
    };
    u32 i;

    for (i = 0u; i < (u32)(sizeof(locked) / sizeof(locked[0])); i++)
    {
        if ((fighter_mask & (1u << locked[i].fkind)) == 0u)
        {
            if (ndsMenuShellOnePlayerCssBlit(locked[i].surface) != FALSE)
            {
                gNdsOnePlayerCssNativeLockedBlitCount++;
            }
        }
    }
}

static s32 ndsMenuShellOnePlayerCssEnter(u32 fighter_mask)
{
    NdsUiKitSurfaceId base = NDS_MN_UI_KIT_SURFACE_ONEP_CSS_SCREEN;

    ndsPlatformSetOriginalSpriteOverlayEnabled(TRUE);
    ndsPlatformClearOriginalSpriteOverlayLayer(FALSE);
    ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
    ndsPlatformClearBattleTextHud();
    if (ndsUiKitEnter(NDS_UI_KIT_ENGINE_MAIN) == FALSE)
    {
        return FALSE;
    }
    BG_PALETTE[0] = RGB15(1, 6, 12);
    ndsPlatformCommitOriginalSpriteOverlayTransform();
    if (ndsUiKitBlitSurfaces(&base, 1u) == FALSE)
    {
        gNdsOnePlayerCssNativeSurfaceFailCount++;
        ndsUiKitExit();
        return FALSE;
    }
    gNdsOnePlayerCssNativeBaseBlitCount++;
    sNdsOnePlayerCssFighterMask = fighter_mask;
    ndsMenuShellOnePlayerCssApplyLocks(fighter_mask);
    sNdsOnePlayerCssLastFkind = NDS_ONEP_STATE_NONE;
    sNdsOnePlayerCssLastTime = NDS_ONEP_STATE_NONE;
    sNdsOnePlayerCssLastDifficulty = NDS_ONEP_STATE_NONE;
    sNdsOnePlayerCssLastReady = NDS_ONEP_STATE_NONE;
    sNdsOnePlayerCssActive = TRUE;
    gNdsOnePlayerCssNativeEnterCount++;
    return TRUE;
}

static void ndsMenuShellOnePlayerCssSyncGate(s32 fkind)
{
    NdsUiKitSurfaceId surface;

    if ((fkind >= 0) && ((u32)fkind < NDS_MENU_SHELL_FIGHTER_KINDS))
    {
        surface = sNdsOnePlayerGateSurface[(u32)fkind];
    }
    else
    {
        surface = NDS_MN_UI_KIT_SURFACE_ONEP_GATE_EMPTY;
    }
    if (ndsMenuShellOnePlayerCssBlit(surface) != FALSE)
    {
        gNdsOnePlayerCssNativeVisibleMask |= (1u << 3);
    }
    sNdsOnePlayerCssLastFkind = (u32)fkind;
}

static void ndsMenuShellOnePlayerCssSyncLevel(u32 difficulty)
{
    if (difficulty > 4u)
    {
        difficulty = 4u;
    }
    if (ndsMenuShellOnePlayerCssBlit(
            sNdsOnePlayerLevelSurface[difficulty]) != FALSE)
    {
        gNdsOnePlayerCssNativeVisibleMask |= (1u << 4);
    }
    sNdsOnePlayerCssLastDifficulty = difficulty;
}

static void ndsMenuShellOnePlayerCssSyncTime(u32 time_setting)
{
    NdsUiKitSurfaceId surface = (time_setting == NDS_ONEP_TIME_INFINITE) ?
        NDS_MN_UI_KIT_SURFACE_ONEP_TIME_INF :
        NDS_MN_UI_KIT_SURFACE_ONEP_TIME_5;

    if (ndsMenuShellOnePlayerCssBlit(surface) != FALSE)
    {
        gNdsOnePlayerCssNativeVisibleMask |= (1u << 6);
    }
    sNdsOnePlayerCssLastTime = time_setting;
}

static void ndsMenuShellOnePlayerCssSyncReady(u32 visible)
{
    NdsUiKitSurfaceId state = (visible != FALSE) ?
        NDS_MN_UI_KIT_SURFACE_ONEP_READY_ON :
        NDS_MN_UI_KIT_SURFACE_ONEP_READY_OFF;

    (void)ndsMenuShellOnePlayerCssBlit(state);
    ndsUiKitClearForegroundRect(NDS_ONEP_DS(0), NDS_ONEP_DS(71),
                                (u32)NDS_ONEP_DS(320), 18u);
    if (visible != FALSE)
    {
        NdsUiKitSurfaceId foreground =
            NDS_MN_UI_KIT_SURFACE_CSS_READY_FOREGROUND;

        if (ndsUiKitBlitForegroundSurfaces(&foreground, 1u) == FALSE)
        {
            gNdsOnePlayerCssNativeSurfaceFailCount++;
        }
        ndsUiKitSetSprite(NDS_ONEP_SPR_READY_PRESS,
                          NDS_MN_UI_KIT_IMAGE_CSS_READY_PRESS,
                          NDS_ONEP_DS(133), NDS_ONEP_DS(219));
        ndsUiKitSetSprite(NDS_ONEP_SPR_READY_START,
                          NDS_MN_UI_KIT_IMAGE_CSS_READY_START,
                          NDS_ONEP_DS(162), NDS_ONEP_DS(219));
        gNdsOnePlayerCssNativeVisibleMask |= (1u << 7);
    }
    else
    {
        ndsUiKitHideSprite(NDS_ONEP_SPR_READY_PRESS);
        ndsUiKitHideSprite(NDS_ONEP_SPR_READY_START);
        gNdsOnePlayerCssNativeVisibleMask &= ~(1u << 7);
    }
    /* The READY band's opaque restore intersects the lower portrait row.
     * Reapply source save locks on BG2; the foreground banner remains above
     * them, matching the proven VS CSS layering rule. */
    ndsMenuShellOnePlayerCssApplyLocks(sNdsOnePlayerCssFighterMask);
    sNdsOnePlayerCssLastReady = (visible != FALSE) ? 1u : 0u;
}

static void ndsMenuShellOnePlayerCssSyncCursor(s32 cursor_x, s32 cursor_y,
                                                u32 cursor_status)
{
    static const s8 tag_offset[3][2] = {
        { 7, 15 }, { 9, 10 }, { 9, 15 }
    };
    u32 image;

    if (cursor_status > 2u)
    {
        cursor_status = 0u;
    }
    image = NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_POINT + cursor_status;
    ndsUiKitSetSprite(NDS_ONEP_SPR_CURSOR_TAG,
                      NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_1P,
                      NDS_ONEP_DS(cursor_x + tag_offset[cursor_status][0]),
                      NDS_ONEP_DS(cursor_y + tag_offset[cursor_status][1]));
    ndsUiKitSetSprite(NDS_ONEP_SPR_CURSOR, image,
                      NDS_ONEP_DS(cursor_x), NDS_ONEP_DS(cursor_y));
    gNdsOnePlayerCssNativeVisibleMask |= (1u << 1);
}

static void ndsMenuShellOnePlayerCssSyncPuck(s32 puck_x, s32 puck_y,
                                              u32 visible)
{
    if (visible != FALSE)
    {
        ndsUiKitSetSprite(NDS_ONEP_SPR_PUCK,
                          NDS_MN_UI_KIT_IMAGE_PUCK_1P,
                          NDS_ONEP_DS(puck_x), NDS_ONEP_DS(puck_y));
        gNdsOnePlayerCssNativeVisibleMask |= (1u << 2);
    }
    else
    {
        ndsUiKitHideSprite(NDS_ONEP_SPR_PUCK);
        gNdsOnePlayerCssNativeVisibleMask &= ~(1u << 2);
    }
}

static void ndsMenuShellOnePlayerCssSyncSettings(u32 difficulty, u32 stock,
                                                  u32 total_tics)
{
    u32 blink = (((total_tics / 10u) & 1u) == 0u) ? TRUE : FALSE;

    /* The source stores lower stock count 0..4 and draws count+1 icons.  Until
     * those fighter-specific 8x10 stock cells join the main-OBJ bake, publish
     * the same value with the already-native source digit art.  This keeps the
     * setting visible and live without reintroducing the retired SObj renderer;
     * the behaviour/state remains source-owned. */
    if (stock > 4u) stock = 4u;
    ndsUiKitSetSprite(NDS_ONEP_SPR_STOCK_VALUE,
                      NDS_MN_UI_KIT_IMAGE_DIGIT_0 + stock + 1u,
                      NDS_ONEP_DS(207), NDS_ONEP_DS(178));
    gNdsOnePlayerCssNativeVisibleMask |= (1u << 5);

    if ((blink != FALSE) && (difficulty > 0u))
    {
        ndsUiKitSetSprite(NDS_ONEP_SPR_LEVEL_L,
                          NDS_MN_UI_KIT_IMAGE_CSS_ARROW_L,
                          NDS_ONEP_DS(194), NDS_ONEP_DS(158));
    }
    else ndsUiKitHideSprite(NDS_ONEP_SPR_LEVEL_L);
    if ((blink != FALSE) && (difficulty < 4u))
    {
        ndsUiKitSetSprite(NDS_ONEP_SPR_LEVEL_R,
                          NDS_MN_UI_KIT_IMAGE_CSS_ARROW_R,
                          NDS_ONEP_DS(269), NDS_ONEP_DS(158));
    }
    else ndsUiKitHideSprite(NDS_ONEP_SPR_LEVEL_R);

    if ((blink != FALSE) && (stock > 0u))
    {
        ndsUiKitSetSprite(NDS_ONEP_SPR_STOCK_L,
                          NDS_MN_UI_KIT_IMAGE_CSS_ARROW_L,
                          NDS_ONEP_DS(194), NDS_ONEP_DS(178));
    }
    else ndsUiKitHideSprite(NDS_ONEP_SPR_STOCK_L);
    if ((blink != FALSE) && (stock < 4u))
    {
        ndsUiKitSetSprite(NDS_ONEP_SPR_STOCK_R,
                          NDS_MN_UI_KIT_IMAGE_CSS_ARROW_R,
                          NDS_ONEP_DS(269), NDS_ONEP_DS(178));
    }
    else ndsUiKitHideSprite(NDS_ONEP_SPR_STOCK_R);
}

void ndsMenuShellOnePlayerCssPresent(s32 cursor_x, s32 cursor_y,
                                     u32 cursor_status,
                                     s32 puck_x, s32 puck_y,
                                     u32 puck_visible,
                                     s32 fkind, u32 time_setting,
                                     u32 difficulty, u32 stock,
                                     u32 fighter_mask, u32 ready_visible,
                                     u32 total_tics)
{
    if ((sNdsOnePlayerCssActive == FALSE) &&
        (ndsMenuShellOnePlayerCssEnter(fighter_mask) == FALSE))
    {
        return;
    }

    gNdsOnePlayerCssNativePresentCount++;
    gNdsOnePlayerCssNativeVisibleMask |= 1u; /* base */

    if ((u32)fkind != sNdsOnePlayerCssLastFkind)
    {
        ndsMenuShellOnePlayerCssSyncGate(fkind);
    }
    if (difficulty != sNdsOnePlayerCssLastDifficulty)
    {
        ndsMenuShellOnePlayerCssSyncLevel(difficulty);
    }
    if (time_setting != sNdsOnePlayerCssLastTime)
    {
        ndsMenuShellOnePlayerCssSyncTime(time_setting);
    }
    if (((ready_visible != FALSE) ? 1u : 0u) != sNdsOnePlayerCssLastReady)
    {
        ndsMenuShellOnePlayerCssSyncReady(ready_visible);
    }

    ndsMenuShellOnePlayerCssSyncCursor(cursor_x, cursor_y, cursor_status);
    ndsMenuShellOnePlayerCssSyncPuck(puck_x, puck_y, puck_visible);
    ndsMenuShellOnePlayerCssSyncSettings(difficulty, stock, total_tics);

    gNdsOnePlayerCssNativeLastFkind = (u32)fkind;
    gNdsOnePlayerCssNativeLastDifficulty = difficulty;
    gNdsOnePlayerCssNativeLastStock = stock;
    gNdsOnePlayerCssNativeLastTime = time_setting;
}

u32 ndsMenuShellOnePlayerCssOwnsSource2D(void)
{
    return (sNdsOnePlayerCssActive != FALSE) ? TRUE : FALSE;
}

void ndsMenuShellOnePlayerCssExit(void)
{
    if (sNdsOnePlayerCssActive != FALSE)
    {
        ndsUiKitExit();
        ndsPlatformClearOriginalSpriteOverlayLayer(FALSE);
        ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
        sNdsOnePlayerCssActive = FALSE;
        gNdsOnePlayerCssNativeExitCount++;
    }
    sNdsOnePlayerCssLastFkind = NDS_ONEP_STATE_NONE;
    sNdsOnePlayerCssLastTime = NDS_ONEP_STATE_NONE;
    sNdsOnePlayerCssLastDifficulty = NDS_ONEP_STATE_NONE;
    sNdsOnePlayerCssLastReady = NDS_ONEP_STATE_NONE;
}
