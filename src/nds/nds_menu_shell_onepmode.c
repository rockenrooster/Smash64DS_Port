/* P2-6 -- DS-native presentation for BattleShip's 1P Mode menu.
 *
 * Behaviour remains in mn1pmode.c: it owns the four-option cursor, repeat and
 * wrap law, B-back, A/START acceptance, player assignment and scene routing.
 * This file is only the native presentation sink for that source state. */

#include <nds/arm9/video.h>

#include <nds/nds_menu_shell.h>
#include <nds/nds_platform.h>
#include <nds/nds_ui_kit.h>

#include "generated/mn_ui_kit.generated.inc"

#define NDS_ONEP_MODE_OPTIONS 4u
#define NDS_ONEP_MODE_NONE 0xffffffffu

static u32 sNdsOnePlayerModeActive;
static u32 sNdsOnePlayerModeLastOption = NDS_ONEP_MODE_NONE;
static u32 sNdsOnePlayerModeLastSelected = NDS_ONEP_MODE_NONE;

__attribute__((used)) volatile u32 gNdsOnePlayerModeNativeEnterCount;
__attribute__((used)) volatile u32 gNdsOnePlayerModeNativePresentCount;
__attribute__((used)) volatile u32 gNdsOnePlayerModeNativeExitCount;
__attribute__((used)) volatile u32 gNdsOnePlayerModeNativeBaseBlitCount;
__attribute__((used)) volatile u32 gNdsOnePlayerModeNativeStateBlitCount;
__attribute__((used)) volatile u32 gNdsOnePlayerModeNativeSurfaceFailCount;
__attribute__((used)) volatile u32 gNdsOnePlayerModeNativeLastOption;
__attribute__((used)) volatile u32 gNdsOnePlayerModeNativeLastSelected;
__attribute__((used)) volatile u32 gNdsOnePlayerModeNativeVisibleMask;

static const NdsUiKitSurfaceId sNdsOnePlayerModeNot[NDS_ONEP_MODE_OPTIONS] = {
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_GAME_NOT,
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_TRAINING_NOT,
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_BONUS1_NOT,
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_BONUS2_NOT
};

static const NdsUiKitSurfaceId sNdsOnePlayerModeHi[NDS_ONEP_MODE_OPTIONS] = {
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_GAME_HI,
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_TRAINING_HI,
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_BONUS1_HI,
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_BONUS2_HI
};

static const NdsUiKitSurfaceId sNdsOnePlayerModeSelected[
    NDS_ONEP_MODE_OPTIONS] = {
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_GAME_SELECTED,
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_TRAINING_SELECTED,
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_BONUS1_SELECTED,
    NDS_MN_UI_KIT_SURFACE_ONEP_MODE_BONUS2_SELECTED
};

static s32 ndsMenuShellOnePlayerModeBlit(NdsUiKitSurfaceId surface)
{
    if (ndsUiKitBlitSurfaces(&surface, 1u) == FALSE)
    {
        gNdsOnePlayerModeNativeSurfaceFailCount++;
        return FALSE;
    }
    gNdsOnePlayerModeNativeStateBlitCount++;
    return TRUE;
}

static s32 ndsMenuShellOnePlayerModeEnter(void)
{
    NdsUiKitSurfaceId base = NDS_MN_UI_KIT_SURFACE_ONEP_MODE_SCREEN;

    /* A return from 1P CSS can leave BG0 armed with that scene's last fighter
     * preview.  This menu is entirely 2D, so explicitly reclaim the layer
     * rather than relying on the previous scene's teardown ordering. */
    ndsPlatformSet3DLayerEnabled(FALSE);
    ndsPlatformSetOriginalSpriteOverlayEnabled(TRUE);
    ndsPlatformClearOriginalSpriteOverlayLayer(FALSE);
    ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
    ndsPlatformClearBattleTextHud();

    if (ndsUiKitEnter(NDS_UI_KIT_ENGINE_MAIN) == FALSE)
    {
        gNdsOnePlayerModeNativeSurfaceFailCount++;
        return FALSE;
    }
    BG_PALETTE[0] = RGB15(1, 6, 12);
    ndsPlatformCommitOriginalSpriteOverlayTransform();
    if (ndsUiKitBlitSurfaces(&base, 1u) == FALSE)
    {
        gNdsOnePlayerModeNativeSurfaceFailCount++;
        ndsUiKitExit();
        return FALSE;
    }

    gNdsOnePlayerModeNativeBaseBlitCount++;
    sNdsOnePlayerModeLastOption = NDS_ONEP_MODE_NONE;
    sNdsOnePlayerModeLastSelected = NDS_ONEP_MODE_NONE;
    sNdsOnePlayerModeActive = TRUE;
    gNdsOnePlayerModeNativeEnterCount++;
    return TRUE;
}

void ndsMenuShellOnePlayerModePresent(u32 option, u32 is_selected)
{
    u32 selected = (is_selected != FALSE) ? 1u : 0u;

    if (option >= NDS_ONEP_MODE_OPTIONS)
    {
        option = 0u;
    }
    if ((sNdsOnePlayerModeActive == FALSE) &&
        (ndsMenuShellOnePlayerModeEnter() == FALSE))
    {
        return;
    }

    gNdsOnePlayerModeNativePresentCount++;

    if (option != sNdsOnePlayerModeLastOption)
    {
        /* Source mn1PModeFuncRun paints the old row NOT before it paints the
         * new row HI.  Mirror the same ordering with bounded BG2 patches. */
        if (sNdsOnePlayerModeLastOption < NDS_ONEP_MODE_OPTIONS)
        {
            (void)ndsMenuShellOnePlayerModeBlit(
                sNdsOnePlayerModeNot[sNdsOnePlayerModeLastOption]);
        }
        (void)ndsMenuShellOnePlayerModeBlit(
            selected ? sNdsOnePlayerModeSelected[option] :
                       sNdsOnePlayerModeHi[option]);
    }
    else if (selected != sNdsOnePlayerModeLastSelected)
    {
        /* A/START changes the source tab to Selected for the transition frame.
         * If a future source path cancels that state, HI restores it exactly. */
        (void)ndsMenuShellOnePlayerModeBlit(
            selected ? sNdsOnePlayerModeSelected[option] :
                       sNdsOnePlayerModeHi[option]);
    }

    sNdsOnePlayerModeLastOption = option;
    sNdsOnePlayerModeLastSelected = selected;
    gNdsOnePlayerModeNativeLastOption = option;
    gNdsOnePlayerModeNativeLastSelected = selected;
    gNdsOnePlayerModeNativeVisibleMask =
        1u | (1u << (option + 1u)) | (selected << 5);
}

void ndsMenuShellOnePlayerModeExit(void)
{
    if (sNdsOnePlayerModeActive != FALSE)
    {
        ndsUiKitExit();
        ndsPlatformClearOriginalSpriteOverlayLayer(FALSE);
        ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
        ndsPlatformSet3DLayerEnabled(FALSE);
        sNdsOnePlayerModeActive = FALSE;
        gNdsOnePlayerModeNativeExitCount++;
    }
    sNdsOnePlayerModeLastOption = NDS_ONEP_MODE_NONE;
    sNdsOnePlayerModeLastSelected = NDS_ONEP_MODE_NONE;
}
