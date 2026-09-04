/* --- Screen: VS Item Switch ------------------------------------------------
 *
 * Sixteen rows. Row 0 is the appearance rate; rows 1..15 are the toggleable
 * item kinds in the source's own screen order, which
 * `kNdsItemSwitchToggleKinds` carries. UP and DOWN walk the sixteen with
 * wraparound, LEFT and RIGHT edit the row under the cursor, and B commits and
 * leaves. Transcribed from decomp mn/mnvsmode/mnvsitemswitch.c:
 *
 *   cursor   UP from 0 wraps to 15, DOWN from 15 wraps to 0 (:707/:750),
 *            cue MenuScroll2.
 *   row 0    LEFT/RIGHT walk the rate and WRAP: left from None lands on
 *            VeryHigh, right from VeryHigh lands on None (:764/:822). Cue
 *            MenuScroll1. Six rates, not four -- the port's enum stopped at
 *            Middle until P2-5 completed it.
 *   rows 1+  LEFT and RIGHT both flip the toggle; the source spends
 *            MenuScroll1 only when the state actually changes (:816/:836).
 *   B        commits and returns to VS Options (:696).
 *
 * WHAT IT COMMITS is `ndsMatchConfigItemTogglesFromRows`, which is the game's
 * rule rather than this screen's: every row off means NO items at all, Green
 * Shell carries Red Shell, and while anything is on the four containers are
 * forced on. That function landed before this screen precisely so a preset or
 * a test could ask the same question without drawing anything.
 *
 * THE CURSOR IS THREE OBJ CELLS. `llMNVSItemSwitchCursorSprite` is 183x13 --
 * 146x10 after the kit's 4/5 -- and a DS OBJ cell tops out at 64 texels wide,
 * so the kit bakes it as three abutting DEST-space slices (64 + 64 + 18 = 146
 * exactly) and this screen draws them edge to edge. Every slice texel is the
 * full-width bake's own texel, because the whole sprite is resampled once and
 * only then windowed (`convert_image`, scripts/menus/generate_mn_ui_kit.py):
 * the seams carry no resampling difference and the split is invisible. A cell
 * limit forcing a realisation change, not an approximation.
 */

#define NDS_MENU_ITEMSWITCH_ROWS (NDS_ITEM_SWITCH_TOGGLE_COUNT + 1u)

static u32 sMenuItemsCursor;
/* Index 0 is the rate (nSCBattleItemSwitch*), 1..15 are on/off. One array
 * because the source uses one (sMNVSItemSwitchOptionStatuses) and its cursor
 * indexes it directly. */
static u8 sMenuItemsStatus[NDS_MENU_ITEMSWITCH_ROWS];
/* What is currently ON SCREEN, so a frame that changed nothing blits nothing
 * and a change blits exactly the row that changed. Same discipline as the VS
 * rules screen's button surfaces. */
static NdsUiKitSurfaceId sMenuItemsRowSurface[NDS_MENU_ITEMSWITCH_ROWS];

__attribute__((used)) volatile u32 gNdsMenuShellItemsBlitCount;
/* How many of the cursor's three slices the kit accepted at its last
 * placement. 3 is drawn, 0 is invisible -- a kit that never entered and an
 * image with no OBJ VRAM both make `ndsUiKitSetSprite` return FALSE silently,
 * and a cursor is the one element on this screen with no other witness. */
__attribute__((used)) volatile u32 gNdsMenuShellItemsCursorSlices;
__attribute__((used)) volatile u32 gNdsMenuShellItemsCommitCount;
__attribute__((used)) volatile u32 gNdsMenuShellItemsCommitToggles;
__attribute__((used)) volatile u32 gNdsMenuShellItemsCommitRate;

static const NdsUiKitSurfaceId kNdsMenuItemsPlate[] = {
    NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH
};

/* The six rates in enum order, so the rate surface is an index rather than a
 * switch. nSCBattleItemSwitchNone..VeryHigh, sc/scdef.h:245-254. */
static const NdsUiKitSurfaceId kNdsMenuItemsRateSurface[] = {
    NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_RATE_NONE,
    NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_RATE_VERY_LOW,
    NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_RATE_LOW,
    NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_RATE_MIDDLE,
    NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_RATE_HIGH,
    NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_RATE_VERY_HIGH
};

/* Fifteen rows x two states, indexed [row - 1][on]. Written out rather than
 * computed from the id because the generator appends surfaces in its own
 * order and an arithmetic id is a silent wrong-row bug the moment it does. */
static const NdsUiKitSurfaceId kNdsMenuItemsRowSurface
    [NDS_ITEM_SWITCH_TOGGLE_COUNT][2] = {
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_01_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_01_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_02_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_02_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_03_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_03_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_04_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_04_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_05_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_05_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_06_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_06_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_07_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_07_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_08_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_08_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_09_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_09_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_10_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_10_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_11_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_11_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_12_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_12_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_13_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_13_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_14_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_14_ON },
    { NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_15_OFF, NDS_MN_UI_KIT_SURFACE_ITEM_SWITCH_ROW_15_ON }
};

/* Which surface each row SHOULD be showing, given its status. */
static NdsUiKitSurfaceId ndsMenuShellItemsWantSurface(u32 row)
{
    if (row == 0u)
    {
        u32 rate = sMenuItemsStatus[0];

        if (rate >= (u32)(sizeof(kNdsMenuItemsRateSurface) /
                          sizeof(kNdsMenuItemsRateSurface[0])))
        {
            rate = (u32)nSCBattleItemSwitchNone;
        }
        return kNdsMenuItemsRateSurface[rate];
    }
    return kNdsMenuItemsRowSurface[row - 1u][(sMenuItemsStatus[row] != 0u) ? 1u
                                                                          : 0u];
}

/* At most `budget` rows a frame, so a screen holding still compares sixteen
 * bytes and returns. The VS rules screen made this one-per-frame after a
 * 606,336-tick measurement; sixteen rows only ever differ from the screen on
 * the entry frame, and a cursor edit changes exactly one. */
static void ndsMenuShellItemsSyncRows(u32 budget)
{
    u32 row;

    for (row = 0u; row < NDS_MENU_ITEMSWITCH_ROWS; row++)
    {
        NdsUiKitSurfaceId want = ndsMenuShellItemsWantSurface(row);

        if (want != sMenuItemsRowSurface[row])
        {
            (void)ndsUiKitBlitSurfaces(&want, 1u);
            sMenuItemsRowSurface[row] = want;
            gNdsMenuShellItemsBlitCount++;
            if (budget == 0u)
            {
                return;
            }
            budget--;
        }
    }
}

/* mnVSItemSwitchGetItemSettings (:572): the screen opens on what the
 * descriptor already carries, so a return trip shows what the last visit
 * committed. */
static void ndsMenuShellItemsLoad(void)
{
    u32 row;

    sMenuItemsCursor = 0u;
    sMenuItemsStatus[0] = gNdsMatchConfig.item_appearance_rate;
    ndsMatchConfigItemRowsFromToggles(gNdsMatchConfig.item_toggles,
                                      &sMenuItemsStatus[1]);
    for (row = 0u; row < NDS_MENU_ITEMSWITCH_ROWS; row++)
    {
        /* Nothing is on the freshly cleared BG2, so every row differs and all
         * sixteen blit across the entry frames. */
        sMenuItemsRowSurface[row] = NDS_MENU_VS_SURFACE_NONE;
    }
}

/* mnVSItemSwitchSetItemToggles (:647), through the descriptor rather than
 * straight into the battle state, as every other screen in this shell does. */
static void ndsMenuShellItemsSave(void)
{
    gNdsMatchConfig.item_appearance_rate = sMenuItemsStatus[0];
    gNdsMatchConfig.item_toggles =
        ndsMatchConfigItemTogglesFromRows(&sMenuItemsStatus[1]);
    ndsMatchConfigApply(&gNdsMatchConfig);

    gNdsMenuShellItemsCommitCount++;
    gNdsMenuShellItemsCommitToggles = gNdsMatchConfig.item_toggles;
    gNdsMenuShellItemsCommitRate = (u32)gNdsMatchConfig.item_appearance_rate;
}

/* --- The row cursor ------------------------------------------------------
 *
 * THE BAKE'S OWN ROUNDING, not the truncating NDS_CSS_DS/NDS_SSS_DS the other
 * screens use for free-standing OBJs. This cursor is a row-wide bar that has
 * to register against baked plate art -- the item-name list at source (125,48)
 * and the toggle column at x 244 -- and the generator's `frame_pos` rounds
 * half up where a plain (v*4)/5 truncates. The character select paid exactly
 * one pixel for that difference already (owner screenshot, 2026-08-19;
 * nds_menu_shell_css.c:801). One rule for everything this screen places. */
#define NDS_MENU_ITEMS_DS(v) (((v) * 4 + 2) / 5)

/* Three OBJ slots: the shell's cursor slot and its two number slots. This
 * screen draws no numbers and no arrows -- every value it owns is baked into a
 * row surface -- and `ndsMenuShellHideRows` clears every slot on entry, so
 * borrowing them costs nothing. Same aliasing the main menu does for its lit
 * mode icon. */
#define NDS_MENU_ITEMS_SPRITE_CURSOR0 NDS_MENU_SPRITE_CURSOR
#define NDS_MENU_ITEMS_CURSOR_SLICES 3u

static const u32 kNdsMenuItemsCursorImage[NDS_MENU_ITEMS_CURSOR_SLICES] = {
    NDS_MN_UI_KIT_IMAGE_ITEM_SWITCH_CURSOR_0,
    NDS_MN_UI_KIT_IMAGE_ITEM_SWITCH_CURSOR_1,
    NDS_MN_UI_KIT_IMAGE_ITEM_SWITCH_CURSOR_2
};

/* Each slice's left edge relative to the first, in DS pixels: the kit windowed
 * the resampled 146x10 raster at (0,64), (64,64) and (128,18), so abutting
 * them here is what puts every column back where the bake had it. */
static const s32 kNdsMenuItemsCursorDx[NDS_MENU_ITEMS_CURSOR_SLICES] = {
    0, 64, 128
};

/* mnVSItemSwitchSetCursorPosition (:393): x is 115 on every row; y is 47 on
 * row 0 and `option_id * 10 + 51` on the rest. Row 0 is the source's own
 * special case, not a nudge: rows 1..15 put the bar 3 px above their toggle
 * (`mnVSItemSwitchInitToggles` places those at row * 10 + 54, :485), and at
 * row 0 that arithmetic would give 51 -- BELOW the top of the appearance
 * sprite it is supposed to mark, which sits at y 49 (:465). 47 keeps the lead.
 *
 * NO INTER-CAMERA OFFSET ENTERS HERE, and that was worth checking rather than
 * assuming: the cursor is drawn through its own camera on display link 3
 * (mnVSItemSwitchMakeCursorCamera, :491), but that camera's viewport is
 * (10,10)-(310,230) -- byte for byte the labels camera's (:516) and the
 * decal camera's (:547). One coordinate frame for the whole screen, which is
 * the frame the plate was baked in. */
static void ndsMenuShellItemsPlaceCursor(void)
{
    s32 row_y = (sMenuItemsCursor == 0u) ?
        47 : (s32)((sMenuItemsCursor * 10u) + 51u);
    s32 x = NDS_MENU_ITEMS_DS(115);
    s32 y = NDS_MENU_ITEMS_DS(row_y);
    u32 placed = 0u;
    u32 i;

    for (i = 0u; i < NDS_MENU_ITEMS_CURSOR_SLICES; i++)
    {
        if (ndsUiKitSetSprite(NDS_MENU_ITEMS_SPRITE_CURSOR0 + i,
                              kNdsMenuItemsCursorImage[i],
                              x + kNdsMenuItemsCursorDx[i], y) != FALSE)
        {
            placed++;
        }
    }
    gNdsMenuShellItemsCursorSlices = placed;
}

static void ndsMenuShellPopulateItems(void)
{
    (void)ndsUiKitBlitSurfaces(kNdsMenuItemsPlate, 1u);
    ndsMenuShellItemsSyncRows(NDS_MENU_ITEMSWITCH_ROWS);
    ndsMenuShellItemsPlaceCursor();
}

/* mnvsitemswitch.c:755-836. LEFT and RIGHT differ only on row 0, where they
 * walk the rate in opposite directions and wrap; on every other row both flip
 * the toggle, and the cue is spent only when the state actually changes. */
static void ndsMenuShellItemsAdjust(s32 direction)
{
    if (sMenuItemsCursor == 0u)
    {
        s32 rate = (s32)sMenuItemsStatus[0];

        if (direction < 0)
        {
            rate = (rate > (s32)nSCBattleItemSwitchNone) ?
                (rate - 1) : (s32)nSCBattleItemSwitchVeryHigh;
        }
        else
        {
            rate = (rate < (s32)nSCBattleItemSwitchVeryHigh) ?
                (rate + 1) : (s32)nSCBattleItemSwitchNone;
        }
        sMenuItemsStatus[0] = (u8)rate;
        ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        return;
    }
    sMenuItemsStatus[sMenuItemsCursor] =
        (sMenuItemsStatus[sMenuItemsCursor] != 0u) ? 0u : 1u;
    ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
}

static void ndsMenuShellUpdateItems(u32 held, u32 taps)
{
    u32 moved = FALSE;

    ndsMenuShellItemsSyncRows(1u);

    if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
    {
        sMenuItemsCursor = (sMenuItemsCursor == 0u) ?
            (NDS_MENU_ITEMSWITCH_ROWS - 1u) : (sMenuItemsCursor - 1u);
        moved = TRUE;
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
    {
        sMenuItemsCursor = (sMenuItemsCursor + 1u) % NDS_MENU_ITEMSWITCH_ROWS;
        moved = TRUE;
    }
    if (moved != FALSE)
    {
        /* The source moves the ONE cursor SObj it made (:725/:750) rather than
         * remaking it, and re-placing only on a frame that moved keeps that
         * shape: `ndsUiKitCommit` skips the whole OAM upload while the kit is
         * clean, so a screen holding still costs one branch, exactly as the
         * row sync above costs sixteen byte compares. */
        ndsMenuShellItemsPlaceCursor();
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT) != FALSE)
    {
        ndsMenuShellItemsAdjust(-1);
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_RIGHT) != FALSE)
    {
        ndsMenuShellItemsAdjust(1);
    }

    /* The source leaves on B alone (:696). A and START do nothing here --
     * there is no row to confirm, because every row is edited in place. */
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellItemsSave();
        ndsMenuShellGoto((u32)nSCKindVSOptions);
    }
}
