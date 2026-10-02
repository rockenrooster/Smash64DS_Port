/* P2-6 -- DS-native presentation for BattleShip's 1P character select.
 *
 * Behaviour remains in mnplayers1pgame.c.  That source scene owns every
 * cursor hit test, fighter selection, setting, costume change and transition;
 * this file only maps its evaluated state onto the same AOT menu assets used
 * by the native VS CSS.  No N64 display list or sprite-compositor path is
 * needed for the 2D screen.
 *
 * The Bonus 1/2 Practice selects (mnplayers1pbonus.c, end of file) present
 * through this same owner: one Active latch for
 * ndsMenuShellOnePlayerCssOwnsSource2D and ...Exit, the hand/token/READY
 * helpers and the save-lock cells.  The scenes never run at once. */

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

/* Which select's base the owner entered. */
#define NDS_ONEP_SCREEN_GAME 1u
#define NDS_ONEP_SCREEN_BONUS 2u
/* The bonus gate's "no fighter" key (UpdateNameAndEmblem hides the pair). */
#define NDS_BONUS_CSS_GATE_EMPTY 0xffu

static u32 sNdsOnePlayerCssActive;
static u32 sNdsOnePlayerCssScreen;
static u32 sNdsOnePlayerCssLastFkind = NDS_ONEP_STATE_NONE;
static u32 sNdsOnePlayerCssLastTime = NDS_ONEP_STATE_NONE;
static u32 sNdsOnePlayerCssLastDifficulty = NDS_ONEP_STATE_NONE;
static u32 sNdsOnePlayerCssLastReady = NDS_ONEP_STATE_NONE;
static u32 sNdsOnePlayerCssFighterMask;
static u32 sNdsBonusCssLastKind = NDS_ONEP_STATE_NONE;
static u32 sNdsBonusCssLastGate = NDS_ONEP_STATE_NONE;
static u32 sNdsBonusCssLastRecord = NDS_ONEP_STATE_NONE;
static u32 sNdsBonusCssLastTotal = NDS_ONEP_STATE_NONE;

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

static s32 ndsMenuShellOnePlayerCssEnterBase(NdsUiKitSurfaceId base,
                                             u32 screen, u32 fighter_mask)
{
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
    /* The bonus base already shows an empty gate and both empty record rows
     * -- those three surfaces are the base over their own boxes -- so a bonus
     * entry reads only its title and whatever record differs. */
    sNdsBonusCssLastKind = NDS_ONEP_STATE_NONE;
    sNdsBonusCssLastGate = NDS_BONUS_CSS_GATE_EMPTY;
    sNdsBonusCssLastRecord = 0u;
    sNdsBonusCssLastTotal = 0u;
    sNdsOnePlayerCssScreen = screen;
    sNdsOnePlayerCssActive = TRUE;
    gNdsOnePlayerCssNativeEnterCount++;
    return TRUE;
}

/* One owner, two selects. A base the other select left open is closed first
 * (each scene's StartScene wrapper exits it, so the shipped flow never needs
 * this), then the requested one is entered once. */
static s32 ndsMenuShellOnePlayerCssClaim(u32 screen, NdsUiKitSurfaceId base,
                                         u32 fighter_mask)
{
    if ((sNdsOnePlayerCssActive != FALSE) &&
        (sNdsOnePlayerCssScreen != screen))
    {
        ndsMenuShellOnePlayerCssExit();
    }
    if (sNdsOnePlayerCssActive != FALSE)
    {
        return TRUE;
    }
    return ndsMenuShellOnePlayerCssEnterBase(base, screen, fighter_mask);
}

static s32 ndsMenuShellOnePlayerCssEnter(u32 fighter_mask)
{
    return ndsMenuShellOnePlayerCssClaim(NDS_ONEP_SCREEN_GAME,
                                         NDS_MN_UI_KIT_SURFACE_ONEP_CSS_SCREEN,
                                         fighter_mask);
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
    if (ndsMenuShellOnePlayerCssEnter(fighter_mask) == FALSE)
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
    sNdsOnePlayerCssScreen = 0u;
    sNdsOnePlayerCssLastFkind = NDS_ONEP_STATE_NONE;
    sNdsOnePlayerCssLastTime = NDS_ONEP_STATE_NONE;
    sNdsOnePlayerCssLastDifficulty = NDS_ONEP_STATE_NONE;
    sNdsOnePlayerCssLastReady = NDS_ONEP_STATE_NONE;
    sNdsBonusCssLastKind = NDS_ONEP_STATE_NONE;
    sNdsBonusCssLastGate = NDS_ONEP_STATE_NONE;
    sNdsBonusCssLastRecord = NDS_ONEP_STATE_NONE;
    sNdsBonusCssLastTotal = NDS_ONEP_STATE_NONE;
}

/* --- P2-6. The Bonus 1/2 Practice character selects. ----------------------
 *
 * mnplayers1pbonus.c is the 1P select's twin and stays its only behaviour
 * owner (both scenes are that one TU; sMNPlayers1PBonusBonusKind picks the
 * bonus). battleship_mnplayers1pbonus.c's draw seam publishes the evaluated
 * state; this maps it onto the BONUS_* bake (generate_mn_ui_kit.py) and the
 * helpers above. The base keeps the 1P grid texel for texel, so ONEP_READY_*
 * and CSS_LOCKED_* restore it exactly (check_bonus_css_shared_states). What
 * differs is the source's own layout: no time/level/stock column, the card
 * 33 px right (MakeGate :743-796), a title toggled in place
 * (UpdateGameMode :2081-2102) and the puck fighter's record under the
 * fighter (MakeHiScore :1171-1178, MakeTotalTime :1187-1244). One frame's
 * BG2 changes ride ONE NitroFS open. */

/* The bake's own frame_pos: 4/5 of a source coordinate rounded half up, so a
 * digit lands on the texel its label was composited against. */
#define NDS_BONUS_CSS_DS(v) ((((v) * 4) + 2) / 5)
/* Title + gate + record (label, six digits) + total (label, seven digits). */
#define NDS_BONUS_CSS_BATCH 20u

_Static_assert(NDS_MN_UI_KIT_SURFACE_BONUS_GATE_NESS ==
                   NDS_MN_UI_KIT_SURFACE_BONUS_GATE_MARIO +
                       NDS_MENU_SHELL_FIGHTER_KINDS - 1u,
               "bonus gate surfaces must stay contiguous in fkind order");
_Static_assert(NDS_MN_UI_KIT_SURFACE_BONUS_DIGIT_9 ==
                   NDS_MN_UI_KIT_SURFACE_BONUS_DIGIT_0 + 9u,
               "bonus digit glyphs must stay contiguous in digit order");

/* One frame's BG2 changes, flushed by ndsUiKitBlitSurfacesAt in one open. */
static struct {
    NdsUiKitSurfaceId surface[NDS_BONUS_CSS_BATCH];
    s16 site[NDS_BONUS_CSS_BATCH * 2u];
    u32 count;
} sNdsBonusCssBatch;

volatile u32 gNdsBonusCssNativePresentCount;
volatile u32 gNdsBonusCssNativeBatchCount;
volatile u32 gNdsBonusCssNativeLastKind;
volatile u32 gNdsBonusCssNativeLastGate;
volatile u32 gNdsBonusCssNativeLastRecord;
volatile u32 gNdsBonusCssNativeLastTotal;

static void ndsMenuShellBonusCssQueue(u32 surface, s32 x, s32 y)
{
    u32 n = sNdsBonusCssBatch.count;

    if (n >= NDS_BONUS_CSS_BATCH)
    {
        gNdsOnePlayerCssNativeSurfaceFailCount++;
        return;
    }
    sNdsBonusCssBatch.surface[n] = (NdsUiKitSurfaceId)surface;
    sNdsBonusCssBatch.site[2u * n] = (s16)x;
    sNdsBonusCssBatch.site[(2u * n) + 1u] = (s16)y;
    sNdsBonusCssBatch.count = n + 1u;
}

/* mnPlayers1PBonusMakeNumber (:181-217) as every bonus caller runs it,
 * is_fixed_digit_count TRUE: exactly `places` digits, the ones at right - 8
 * and each higher place 8 px further left (:203-215). */
static void ndsMenuShellBonusCssNumber(u32 value, s32 right, s32 y, u32 places)
{
    s32 x = right;

    while (places != 0u)
    {
        x -= 8;
        ndsMenuShellBonusCssQueue(NDS_MN_UI_KIT_SURFACE_BONUS_DIGIT_0 +
                                      (value % 10u),
                                  NDS_BONUS_CSS_DS(x), NDS_BONUS_CSS_DS(y));
        value /= 10u;
        places--;
    }
}

/* Only what the source draws enters a key: two places a field and the kind
 * for the count's word. 0 is the empty row the base already shows. */
static u32 ndsMenuShellBonusCssRecordKey(const NdsMenuShellBonusCssState *state)
{
    switch (state->record_kind)
    {
    case NDS_MENU_SHELL_BONUS_RECORD_TIME:
        return (1u << 28) | ((state->record_value[0] % 100u) << 16) |
               ((state->record_value[1] % 100u) << 8) |
               (state->record_value[2] % 100u);
    case NDS_MENU_SHELL_BONUS_RECORD_COUNT:
        return (2u << 28) | ((state->bonus_kind & 1u) << 24) |
               (state->record_value[0] % 100u);
    default:
        return 0u;
    }
}

/* Three places for the minutes, two for the rest; 0 is the empty row. */
static u32 ndsMenuShellBonusCssTotalKey(const NdsMenuShellBonusCssState *state)
{
    if (state->total_visible == FALSE)
    {
        return 0u;
    }
    return (1u << 31) | ((state->total_value[0] % 1000u) << 14) |
           ((state->total_value[1] % 100u) << 7) |
           (state->total_value[2] % 100u);
}

static void ndsMenuShellBonusCssRecord(const NdsMenuShellBonusCssState *state)
{
    switch (state->record_kind)
    {
    case NDS_MENU_SHELL_BONUS_RECORD_TIME:
        /* mnPlayers1PBonusMakeBestTime (:1055-1092): BEST TIME and the two
         * marks are the state; minutes, seconds and hundredths end at 237,
         * 259 and 283 on row 195. */
        ndsMenuShellBonusCssQueue(NDS_MN_UI_KIT_SURFACE_BONUS_RECORD_TIME,
                                  NDS_UI_KIT_SITE_BAKED, 0);
        ndsMenuShellBonusCssNumber(state->record_value[0], 237, 195, 2u);
        ndsMenuShellBonusCssNumber(state->record_value[1], 259, 195, 2u);
        ndsMenuShellBonusCssNumber(state->record_value[2], 283, 195, 2u);
        break;
    case NDS_MENU_SHELL_BONUS_RECORD_COUNT:
        /* mnPlayers1PBonusMakeBestTaskCount (:1126-1145, REGION_US): the
         * kind's own word, and two places ending at 225 on row 194. */
        ndsMenuShellBonusCssQueue(
            (state->bonus_kind == 0u) ?
                NDS_MN_UI_KIT_SURFACE_BONUS_RECORD_TARGETS :
                NDS_MN_UI_KIT_SURFACE_BONUS_RECORD_PLATFORMS,
            NDS_UI_KIT_SITE_BAKED, 0);
        ndsMenuShellBonusCssNumber(state->record_value[0], 225, 194, 2u);
        break;
    default:
        /* No puck fighter: the source ejected the record (:1043-1047). */
        ndsMenuShellBonusCssQueue(NDS_MN_UI_KIT_SURFACE_BONUS_RECORD_NONE,
                                  NDS_UI_KIT_SITE_BAKED, 0);
        break;
    }
}

static void ndsMenuShellBonusCssTotal(const NdsMenuShellBonusCssState *state)
{
    if (state->total_visible == FALSE)
    {
        ndsMenuShellBonusCssQueue(NDS_MN_UI_KIT_SURFACE_BONUS_TOTAL_NONE,
                                  NDS_UI_KIT_SITE_BAKED, 0);
        return;
    }
    /* mnPlayers1PBonusMakeTotalTime (:1201-1243): TOTAL BEST TIME and the two
     * marks are the state; hundredths and seconds keep two places ending at
     * 283 and 259, the minutes three ending at 237, all on row 206. */
    ndsMenuShellBonusCssQueue(NDS_MN_UI_KIT_SURFACE_BONUS_TOTAL_TIME,
                              NDS_UI_KIT_SITE_BAKED, 0);
    ndsMenuShellBonusCssNumber(state->total_value[2], 283, 206, 2u);
    ndsMenuShellBonusCssNumber(state->total_value[1], 259, 206, 2u);
    ndsMenuShellBonusCssNumber(state->total_value[0], 237, 206, 3u);
}

static s32 ndsMenuShellBonusCssEnter(u32 fighter_mask)
{
    return ndsMenuShellOnePlayerCssClaim(NDS_ONEP_SCREEN_BONUS,
                                         NDS_MN_UI_KIT_SURFACE_BONUS_CSS_SCREEN,
                                         fighter_mask);
}

void ndsMenuShellBonusCssPresent(const NdsMenuShellBonusCssState *state)
{
    u32 gate;
    u32 record;
    u32 total;

    if ((state == NULL) ||
        (ndsMenuShellBonusCssEnter(state->fighter_mask) == FALSE))
    {
        return;
    }
    gNdsBonusCssNativePresentCount++;
    gNdsOnePlayerCssNativeVisibleMask |= 1u; /* base */
    sNdsBonusCssBatch.count = 0u;

    /* mnPlayers1PBonusMakeLabels (:885-897), re-made on a title press
     * (UpdateGameMode :2089-2090). */
    if (state->bonus_kind != sNdsBonusCssLastKind)
    {
        ndsMenuShellBonusCssQueue(
            (state->bonus_kind == 0u) ?
                NDS_MN_UI_KIT_SURFACE_BONUS_TITLE_TARGETS :
                NDS_MN_UI_KIT_SURFACE_BONUS_TITLE_PLATFORMS,
            NDS_UI_KIT_SITE_BAKED, 0);
        sNdsBonusCssLastKind = state->bonus_kind;
        gNdsOnePlayerCssNativeVisibleMask |= (1u << 8);
    }
    /* mnPlayers1PBonusUpdateNameAndEmblem (:1573-1584). */
    gate = ((state->gate_fkind >= 0) &&
            ((u32)state->gate_fkind < NDS_MENU_SHELL_FIGHTER_KINDS)) ?
        (u32)state->gate_fkind : NDS_BONUS_CSS_GATE_EMPTY;
    if (gate != sNdsBonusCssLastGate)
    {
        ndsMenuShellBonusCssQueue(
            (gate == NDS_BONUS_CSS_GATE_EMPTY) ?
                NDS_MN_UI_KIT_SURFACE_BONUS_GATE_EMPTY :
                (NDS_MN_UI_KIT_SURFACE_BONUS_GATE_MARIO + gate),
            NDS_UI_KIT_SITE_BAKED, 0);
        sNdsBonusCssLastGate = gate;
        gNdsOnePlayerCssNativeVisibleMask |= (1u << 3);
    }
    /* The record the puck process rebuilt this frame (:2248). */
    record = ndsMenuShellBonusCssRecordKey(state);
    if (record != sNdsBonusCssLastRecord)
    {
        ndsMenuShellBonusCssRecord(state);
        sNdsBonusCssLastRecord = record;
        gNdsOnePlayerCssNativeVisibleMask |= (1u << 9);
    }
    total = ndsMenuShellBonusCssTotalKey(state);
    if (total != sNdsBonusCssLastTotal)
    {
        ndsMenuShellBonusCssTotal(state);
        sNdsBonusCssLastTotal = total;
        gNdsOnePlayerCssNativeVisibleMask |= (1u << 10);
    }
    if (sNdsBonusCssBatch.count != 0u)
    {
        if (ndsUiKitBlitSurfacesAt(sNdsBonusCssBatch.surface,
                                   sNdsBonusCssBatch.site,
                                   sNdsBonusCssBatch.count) == FALSE)
        {
            gNdsOnePlayerCssNativeSurfaceFailCount++;
        }
        else
        {
            gNdsOnePlayerCssNativeStateBlitCount += sNdsBonusCssBatch.count;
        }
        gNdsBonusCssNativeBatchCount++;
    }

    /* mnPlayers1PBonusMakeReady (:2574-2657) and its blink (:2554-2571) are
     * the 1P select's own, so its band, banner and PRESS START serve both. */
    if (((state->ready_visible != FALSE) ? 1u : 0u) !=
        sNdsOnePlayerCssLastReady)
    {
        ndsMenuShellOnePlayerCssSyncReady(state->ready_visible);
    }
    ndsMenuShellOnePlayerCssSyncCursor(state->cursor_x, state->cursor_y,
                                       state->cursor_status);
    ndsMenuShellOnePlayerCssSyncPuck(state->puck_x, state->puck_y,
                                     state->puck_visible);

    gNdsBonusCssNativeLastKind = state->bonus_kind;
    gNdsBonusCssNativeLastGate = gate;
    gNdsBonusCssNativeLastRecord = record;
    gNdsBonusCssNativeLastTotal = total;
}
