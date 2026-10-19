/* --- Screen: VS Record -------------------------------------------------------
 *
 * P7 U9 (2026-10-19): the source's three views (decomp mn/mndata/
 * mnvsrecord.c) over every selectable fighter -- the twelve under the
 * source's own lock, Metal Mario and every compiled content under P7's
 * (nds_p7_vsrecord.c loads the wide records, U2, and answers the source's
 * formulas over them). The upper screen is the source's: its black field,
 * the DATA header plate, the VS RECORD label and the view's subtitle (the
 * VSREC_BASE_* surfaces, blitted when the view changes), a one-pixel grid,
 * fighter icons and every number in the source's own digits, all drawn into
 * BG2 out of one resident atlas (VSREC_ATLAS), so a cursor move costs no
 * NitroFS work. The lower screen names what the cursor is on and the
 * controls, in the kit's font (the SUB engine keeps the text slab).
 *
 *   Battle Score  mnVSRecordMakeBattleScoreTable* (:1238, :1280): rows sorted
 *                 by KOs (mnVSRecordSortData :1127), each row's KOs of every
 *                 column and its total. The source shows all twelve; 33 do
 *                 not fit, so an 8x8 window follows a cell cursor (the pad
 *                 moves it, L/R page the columns).
 *   Ranking       mnVSRecordMakeRankingTable* (:1389, :1555): the seven
 *                 columns rotated so the first is the sort key (L/R, as the
 *                 source's FirstColumn), rows sorted by it, the cursor row
 *                 highlighted (:1311). Eight rows show; the cursor scrolls.
 *   Individual    mnVSRecordMakeIndivPortraitAll / ...TableValues (:1719,
 *                 :1648): the ranking cursor's fighter, its portrait, rank,
 *                 use percent and damage totals (:1068), and win percent,
 *                 KOs, KO'd and average against every opponent, sorted by
 *                 win percent. L/R walk the fighter as the source does; U/D
 *                 page the opponents, eight at a time.
 *
 * A/START step the view forward and B back; B on Battle Score returns to
 * DATA (mnVSRecordFuncRun :1974-2009). The kit carries neither the source's
 * BurnS nor its FoxFoot, so a view step is the kit's confirm cue and a move
 * its cursor cue. Data plays on entry (mnVSRecordFuncStart :2201; see
 * ndsMenuShellRunVsRecord).
 */

#include <nds/nds_p7.h>
#include <nds/nds_startup.h>

#ifndef NDS_MENU_SHELL_SCREEN_VSRECORD
#define NDS_MENU_SHELL_SCREEN_VSRECORD 11u
#endif

#define NDS_MENU_VSRECORD_VIEW_SCORE 0u
#define NDS_MENU_VSRECORD_VIEW_RANK 1u
#define NDS_MENU_VSRECORD_VIEW_INDIV 2u

/* BG2 texels, BGR5551 with bit 15 opaque: the field, the grid
 * (mnVSRecordTableGridProcDisplay, 0x62/0x62/0x6A) and the ranking highlight
 * (mnVSRecordRankingHighlightProcDisplay, 0x27/0x00/0xFF). */
#define NDS_MENU_VSRECORD_TEXEL(r, g, b) \
    ((u16)(0x8000u | (((u32)(b) >> 3) << 10) | (((u32)(g) >> 3) << 5) | \
           ((u32)(r) >> 3)))
#define NDS_MENU_VSRECORD_FIELD NDS_MENU_VSRECORD_TEXEL(0x00, 0x00, 0x00)
#define NDS_MENU_VSRECORD_GRID NDS_MENU_VSRECORD_TEXEL(0x62, 0x62, 0x6A)
#define NDS_MENU_VSRECORD_HIGHLIGHT NDS_MENU_VSRECORD_TEXEL(0x27, 0x00, 0xFF)

/* The atlas's two digit rows: table values and portrait stats. */
#define NDS_MENU_VSRECORD_CREAM 0u
#define NDS_MENU_VSRECORD_GREY 1u

/* Rows and columns a table window shows. */
#define NDS_MENU_VSRECORD_WINDOW 8u
#define NDS_MENU_VSRECORD_ROW_PITCH 17
#define NDS_MENU_VSRECORD_ICON ((s32)NDS_MN_UI_KIT_VSREC_ICON)

/* Battle Score, DS pixels: the icon row under the header, the row icons in
 * the left column, eight 24-pixel cells and the total column. */
#define NDS_MENU_VSRECORD_SCORE_TOP 33
#define NDS_MENU_VSRECORD_SCORE_HEAD 50
#define NDS_MENU_VSRECORD_SCORE_ROW_Y 51
#define NDS_MENU_VSRECORD_SCORE_CELL_X 20
#define NDS_MENU_VSRECORD_SCORE_CELL_W 24
#define NDS_MENU_VSRECORD_SCORE_TOTAL_R 252
#define NDS_MENU_VSRECORD_SCORE_BOTTOM 186

/* Ranking: the label row, eight rows under it and the seven columns. */
#define NDS_MENU_VSRECORD_RANK_TOP 33
#define NDS_MENU_VSRECORD_RANK_HEAD 47
#define NDS_MENU_VSRECORD_RANK_ROW_Y 48
#define NDS_MENU_VSRECORD_RANK_COL_X 20
#define NDS_MENU_VSRECORD_RANK_BOTTOM 183
#define NDS_MENU_VSRECORD_RANK_LABEL_Y 38

/* Individual: the stat lines beside the portrait (their words are in the
 * base) and the table of opponents under it. */
#define NDS_MENU_VSRECORD_INDIV_VALUE_R 252
#define NDS_MENU_VSRECORD_INDIV_LINE_Y 40
#define NDS_MENU_VSRECORD_INDIV_LINE_STEP 12
#define NDS_MENU_VSRECORD_INDIV_TOP 95
#define NDS_MENU_VSRECORD_INDIV_ICON_Y 96
#define NDS_MENU_VSRECORD_INDIV_HEAD 112
#define NDS_MENU_VSRECORD_INDIV_ROW_Y 113
#define NDS_MENU_VSRECORD_INDIV_ROW_STEP 15
#define NDS_MENU_VSRECORD_INDIV_ROWS 4
#define NDS_MENU_VSRECORD_INDIV_COL_X 36
#define NDS_MENU_VSRECORD_INDIV_COL_W 27
#define NDS_MENU_VSRECORD_INDIV_BOTTOM 172

/* The lower screen's lines (kit text slots, SUB engine). */
#define NDS_MENU_VSRECORD_TEXT_X 14
#define NDS_MENU_VSRECORD_RGB_TITLE 0x00f2c70du
#define NDS_MENU_VSRECORD_RGB_NAME 0x00e5d199u
#define NDS_MENU_VSRECORD_RGB_HINT 0x008a8892u

/* mnVSRecordGetTKO's cap, which the source also gives every count it
 * draws in four places. */
#define NDS_MENU_VSRECORD_COUNT_MAX 9999u

/* nMNVSRecordRankingKind order: win %, KOs, TKO, SD %, time, use %, avg.
 * DS widths (the source's 33/33/33/33/46/35/34 widened to the panel), the
 * label's x pad (mnVSRecordMakeRankingTableHeaders :1570, at the frame's
 * 4/5), the label and the column's lower-screen name. */
#define NDS_MENU_VSRECORD_COLUMNS 7u
static const u8 kMenuVsRecordRankWidth[NDS_MENU_VSRECORD_COLUMNS] = {
    31u, 31u, 31u, 31u, 43u, 34u, 33u
};
static const u8 kMenuVsRecordRankPad[NDS_MENU_VSRECORD_COLUMNS] = {
    2u, 2u, 2u, 3u, 3u, 2u, 1u
};
static const u8 kMenuVsRecordRankLabel[NDS_MENU_VSRECORD_COLUMNS] = {
    NDS_MN_UI_KIT_VSREC_RECT_LABEL_WINPERCENT,
    NDS_MN_UI_KIT_VSREC_RECT_LABEL_KOS,
    NDS_MN_UI_KIT_VSREC_RECT_LABEL_TKO,
    NDS_MN_UI_KIT_VSREC_RECT_LABEL_SDPERCENT,
    NDS_MN_UI_KIT_VSREC_RECT_LABEL_TIME,
    NDS_MN_UI_KIT_VSREC_RECT_LABEL_USEPERCENT,
    NDS_MN_UI_KIT_VSREC_RECT_LABEL_AVG
};
static const char *const kMenuVsRecordRankName[NDS_MENU_VSRECORD_COLUMNS] = {
    "SORTED BY WIN %", "SORTED BY KOS", "SORTED BY TKO", "SORTED BY SD %",
    "SORTED BY TIME", "SORTED BY USE %", "SORTED BY AVERAGE"
};

/* The lower screen's names by selection id (nds_p7.h), in the kit font's
 * characters (A-Z, apostrophe, percent, period, space). */
static const char *const kMenuVsRecordNames[NDS_MN_UI_KIT_VSREC_SELS] = {
    "MARIO", "FOX", "DONKEY KONG", "SAMUS", "LUIGI", "LINK", "YOSHI",
    "CAPTAIN FALCON", "KIRBY", "PIKACHU", "JIGGLYPUFF", "NESS",
    "METAL MARIO", "FALCO", "GANONDORF", "WOLF", "BOWSER", "MARTH", "ROY",
    "WARIO", "PEACH", "CRASH", "LANKY KONG", "SHEIK", "BANJO", "SONIC",
    "KING DEDEDE", "MAD PIANO", "META KNIGHT", "MR. GAME AND WATCH",
    "SNAKE", "IKE", "PALUTENA"
};

static NDSP7VsRecordData *sMenuVsRecordData;
static u8 *sMenuVsRecordAtlas;
static u32 sMenuVsRecordHeapGeneration;
static u32 sMenuVsRecordReady;
static u32 sMenuVsRecordView;
/* Shown indices (into the data's sel[]) in the select grid's order, and the
 * three views' sorted orders over them (mnVSRecordSortData). */
static u8 sMenuVsRecordGrid[NDS_P7_SEL_COUNT];
static u8 sMenuVsRecordScoreOrder[NDS_P7_SEL_COUNT];
static u8 sMenuVsRecordRankOrder[NDS_P7_SEL_COUNT];
static u8 sMenuVsRecordIndivOrder[NDS_P7_SEL_COUNT];
static u32 sMenuVsRecordScoreRow;
static u32 sMenuVsRecordScoreCol;
static u32 sMenuVsRecordScoreTop;
static u32 sMenuVsRecordScoreLeft;
/* sMNVSRecordCurrentIndex: the ranking cursor, and the Individual fighter. */
static u32 sMenuVsRecordCurrent;
static u32 sMenuVsRecordRankTop;
static u32 sMenuVsRecordFirstColumn;
static u32 sMenuVsRecordIndivLeft;
static u32 sMenuVsRecordPortrait;

/* --- Data ----------------------------------------------------------------- */

static u32 ndsMenuShellVsRecordCount(void)
{
    return (sMenuVsRecordData != NULL) ? sMenuVsRecordData->count : 0u;
}

static u32 ndsMenuShellVsRecordSel(u32 shown)
{
    return sMenuVsRecordData->sel[shown];
}

static s32 ndsMenuShellVsRecordAlloc(void)
{
    const u32 atlas_bytes =
        kNdsUiKitSurfaceMetrics[NDS_MN_UI_KIT_SURFACE_VSREC_ATLAS].bytes;

    if ((sMenuVsRecordData != NULL) && (sMenuVsRecordAtlas != NULL) &&
        (sMenuVsRecordHeapGeneration == gNdsTaskmanHeapGeneration))
    {
        return TRUE;
    }
    sMenuVsRecordData = NULL;
    sMenuVsRecordAtlas = NULL;
    /* Out of the scene's own arena, which the teardown rewinds: the screen
     * needs it only while it is up. */
    if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                            sizeof(NDSP7VsRecordData) + atlas_bytes + 8u,
                            4u) == FALSE)
    {
        return FALSE;
    }
    sMenuVsRecordData = syTaskmanMalloc(sizeof(NDSP7VsRecordData), 4u);
    sMenuVsRecordAtlas = syTaskmanMalloc(atlas_bytes, 4u);
    sMenuVsRecordHeapGeneration = gNdsTaskmanHeapGeneration;
    return ((sMenuVsRecordData != NULL) && (sMenuVsRecordAtlas != NULL)) ?
        TRUE : FALSE;
}

/* mnVSRecordGetFighterKindByIndex's role: the order before any sort, here
 * the select grid's (the bake's kNdsUiKitVsrecOrder), then any shown
 * fighter the grid does not carry. */
static void ndsMenuShellVsRecordGridOrder(void)
{
    u32 count = ndsMenuShellVsRecordCount();
    u32 placed = 0u;
    u8 taken[NDS_P7_SEL_COUNT];
    u32 i;
    u32 j;

    memset(taken, 0, sizeof(taken));
    for (i = 0u; i < NDS_MN_UI_KIT_VSREC_ORDER_COUNT; i++)
    {
        for (j = 0u; j < count; j++)
        {
            if ((taken[j] == 0u) &&
                (ndsMenuShellVsRecordSel(j) == kNdsUiKitVsrecOrder[i]))
            {
                taken[j] = 1u;
                sMenuVsRecordGrid[placed++] = (u8)j;
                break;
            }
        }
    }
    for (j = 0u; j < count; j++)
    {
        if (taken[j] == 0u)
        {
            sMenuVsRecordGrid[placed++] = (u8)j;
        }
    }
}

/* A ranking column's value for shown fighter `i` (mnVSRecordSortData
 * :1145-1180). */
static f32 ndsMenuShellVsRecordColumnStat(u32 column, u32 i)
{
    const NDSP7VsRecordData *d = sMenuVsRecordData;

    switch (column)
    {
    case 0u: return ndsP7VsRecordWinPercent(d, i);
    case 1u: return (f32)ndsP7VsRecordKOs(d, i);
    case 2u: return (f32)ndsP7VsRecordTKO(d, i);
    case 3u: return ndsP7VsRecordSDPercent(d, i);
    case 4u: return (f32)ndsP7VsRecordTimeUsed(d, i);
    case 5u: return ndsP7VsRecordUsePercent(d, i);
    default: return ndsP7VsRecordAvg(d, i);
    }
}

/* mnVSRecordSortData's exchange sort, descending, over the grid order: a
 * later fighter with a larger stat swaps forward. Every shown fighter is a
 * "have" kind, so the source's lock arm never fires. */
static void ndsMenuShellVsRecordSort(u8 *order, u32 view)
{
    u32 count = ndsMenuShellVsRecordCount();
    f32 stats[NDS_P7_SEL_COUNT];
    u32 i;
    u32 j;

    for (i = 0u; i < count; i++)
    {
        switch (view)
        {
        case NDS_MENU_VSRECORD_VIEW_SCORE:
            stats[i] = (f32)ndsP7VsRecordKOs(sMenuVsRecordData, i);
            break;
        case NDS_MENU_VSRECORD_VIEW_RANK:
            stats[i] = ndsMenuShellVsRecordColumnStat(sMenuVsRecordFirstColumn,
                                                      i);
            break;
        default:
            stats[i] = ndsP7VsRecordWinPercentAgainst(
                sMenuVsRecordData, sMenuVsRecordRankOrder[sMenuVsRecordCurrent],
                i);
            break;
        }
        order[i] = sMenuVsRecordGrid[i];
    }
    for (i = 0u; i < count; i++)
    {
        for (j = i + 1u; j < count; j++)
        {
            if (stats[order[i]] < stats[order[j]])
            {
                u8 prev = order[i];

                order[i] = order[j];
                order[j] = prev;
            }
        }
    }
}

/* mnVSRecordGetRanking (:1016): the place by win percent, ties sharing it. */
static u32 ndsMenuShellVsRecordRanking(u32 shown)
{
    u32 count = ndsMenuShellVsRecordCount();
    f32 stats[NDS_P7_SEL_COUNT];
    u8 order[NDS_P7_SEL_COUNT];
    u32 current = 1u;
    u32 i;
    u32 j;

    for (i = 0u; i < count; i++)
    {
        stats[i] = ndsP7VsRecordWinPercent(sMenuVsRecordData, i);
        order[i] = sMenuVsRecordGrid[i];
    }
    for (i = 0u; i < count; i++)
    {
        for (j = i + 1u; j < count; j++)
        {
            if (stats[order[i]] < stats[order[j]])
            {
                u8 prev = order[i];

                order[i] = order[j];
                order[j] = prev;
            }
        }
    }
    for (i = 0u; i < count; i++)
    {
        if (order[i] == shown)
        {
            return current;
        }
        if ((i + 1u < count) && (stats[order[i]] != stats[order[i + 1u]]))
        {
            current = i + 2u;
        }
    }
    return current;
}

/* --- Drawing ----------------------------------------------------------- */

static void ndsMenuShellVsRecordPart(u32 rect, s32 x, s32 y)
{
    const u8 *r = kNdsUiKitVsrecRects[rect];

    (void)ndsUiKitDrawCopyRect(NDS_MN_UI_KIT_SURFACE_VSREC_ATLAS,
                               sMenuVsRecordAtlas, r[0], r[1], r[2], r[3],
                               x, y);
}

static void ndsMenuShellVsRecordIcon(u32 shown, s32 x, s32 y)
{
    u32 sel = ndsMenuShellVsRecordSel(shown);

    if (sel < NDS_MN_UI_KIT_VSREC_SELS)
    {
        ndsMenuShellVsRecordPart(NDS_MN_UI_KIT_VSREC_RECT_ICON_0 + sel, x, y);
    }
}

static u32 ndsMenuShellVsRecordPowerOf10(u32 exp)
{
    u32 raised = 1u;

    while (exp-- != 0u)
    {
        raised *= 10u;
    }
    return raised;
}

/* mnVSRecordGetDigitCount (:237). */
static u32 ndsMenuShellVsRecordDigitCount(u32 number, u32 max)
{
    u32 count = max;

    while (count > 0u)
    {
        if ((number / ndsMenuShellVsRecordPowerOf10(count - 1u)) != 0u)
        {
            return count;
        }
        count--;
    }
    return 0u;
}

/* mnVSRecordMakeDigits (:255) over the atlas: `number` right-aligned so its
 * last digit ends just left of `x`; with tenths, its last place after a
 * point (1000 drops them: 100.0 draws as 100); narrow digits step 4 pixels,
 * wide ones 5. */
static void ndsMenuShellVsRecordDigits(s32 number, s32 x, s32 y, u32 tone,
                                       u32 show_tenths, u32 wide, u32 max,
                                       u32 fixed)
{
    const u32 digit0 = (tone == NDS_MENU_VSRECORD_GREY) ?
        NDS_MN_UI_KIT_VSREC_RECT_GREY_0 : NDS_MN_UI_KIT_VSREC_RECT_CREAM_0;
    const u32 point = (tone == NDS_MENU_VSRECORD_GREY) ?
        NDS_MN_UI_KIT_VSREC_RECT_GREY_POINT :
        NDS_MN_UI_KIT_VSREC_RECT_CREAM_POINT;
    const s32 step = (wide != FALSE) ? 5 : 4;
    u32 value = (number < 0) ? 0u : (u32)number;
    s32 calc_x = x;
    u32 places;
    u32 i;

    if ((show_tenths != FALSE) && (value == 1000u))
    {
        show_tenths = FALSE;
        value /= 10u;
    }
    if (show_tenths != FALSE)
    {
        calc_x -= step;
        ndsMenuShellVsRecordPart(digit0 + (value % 10u), calc_x, y);
        value /= 10u;
        calc_x -= (wide != FALSE) ? 3 : 2;
        ndsMenuShellVsRecordPart(point, calc_x, y + 4);
    }
    calc_x -= step;
    ndsMenuShellVsRecordPart(digit0 + (value % 10u), calc_x, y);
    places = (fixed != FALSE) ? max :
        ndsMenuShellVsRecordDigitCount(value, max);
    for (i = 1u; i < places; i++)
    {
        calc_x -= step;
        ndsMenuShellVsRecordPart(
            digit0 + ((value / ndsMenuShellVsRecordPowerOf10(i)) % 10u),
            calc_x, y);
    }
}

static void ndsMenuShellVsRecordHLine(s32 x0, s32 x1, s32 y)
{
    ndsUiKitFillRect(x0, y, (u32)(x1 - x0 + 1), 1u, NDS_MENU_VSRECORD_GRID);
}

static void ndsMenuShellVsRecordVLine(s32 x, s32 y0, s32 y1)
{
    ndsUiKitFillRect(x, y0, 1u, (u32)(y1 - y0 + 1), NDS_MENU_VSRECORD_GRID);
}

static void ndsMenuShellVsRecordText(u32 slot, const char *text, u32 rgb,
                                     s32 y)
{
    ndsUiKitSetText(slot, text, rgb);
    ndsUiKitMoveText(slot, NDS_MENU_VSRECORD_TEXT_X, y);
}

/* The lower screen: the view, what the cursor is on, the controls. */
static void ndsMenuShellVsRecordLowerScreen(const char *title,
                                            const char *line_a,
                                            const char *line_b,
                                            const char *hint_a,
                                            const char *hint_b)
{
    ndsMenuShellVsRecordText(0u, title, NDS_MENU_VSRECORD_RGB_TITLE, 16);
    ndsMenuShellVsRecordText(1u, line_a, NDS_MENU_VSRECORD_RGB_NAME, 44);
    ndsMenuShellVsRecordText(2u, line_b, NDS_MENU_VSRECORD_RGB_NAME, 58);
    ndsMenuShellVsRecordText(3u, hint_a, NDS_MENU_VSRECORD_RGB_HINT, 150);
    ndsMenuShellVsRecordText(4u, hint_b, NDS_MENU_VSRECORD_RGB_HINT, 164);
}

static const char *ndsMenuShellVsRecordName(u32 shown)
{
    u32 sel = ndsMenuShellVsRecordSel(shown);

    return (sel < NDS_MN_UI_KIT_VSREC_SELS) ? kMenuVsRecordNames[sel] : "";
}

/* Keeps `cursor` inside a window of NDS_MENU_VSRECORD_WINDOW starting at
 * *top over `count` entries. */
static void ndsMenuShellVsRecordFollow(u32 cursor, u32 count, u32 *top)
{
    if (cursor < *top)
    {
        *top = cursor;
    }
    else if (cursor >= (*top + NDS_MENU_VSRECORD_WINDOW))
    {
        *top = cursor + 1u - NDS_MENU_VSRECORD_WINDOW;
    }
    if ((count > NDS_MENU_VSRECORD_WINDOW) &&
        (*top > (count - NDS_MENU_VSRECORD_WINDOW)))
    {
        *top = count - NDS_MENU_VSRECORD_WINDOW;
    }
}

static void ndsMenuShellVsRecordDrawScore(void)
{
    static char against[40];
    u32 count = ndsMenuShellVsRecordCount();
    u32 rows = (count < NDS_MENU_VSRECORD_WINDOW) ? count :
        NDS_MENU_VSRECORD_WINDOW;
    u32 r;
    u32 c;

    ndsMenuShellVsRecordFollow(sMenuVsRecordScoreRow, count,
                               &sMenuVsRecordScoreTop);
    ndsMenuShellVsRecordFollow(sMenuVsRecordScoreCol, count,
                               &sMenuVsRecordScoreLeft);
    /* The table, not the base's header or its TOTAL label. */
    ndsUiKitFillRect(0, NDS_MENU_VSRECORD_SCORE_TOP, 213u,
                     (u32)(NDS_MENU_VSRECORD_SCORE_HEAD -
                           NDS_MENU_VSRECORD_SCORE_TOP),
                     NDS_MENU_VSRECORD_FIELD);
    ndsUiKitFillRect(0, NDS_MENU_VSRECORD_SCORE_HEAD, 256u,
                     (u32)(192 - NDS_MENU_VSRECORD_SCORE_HEAD),
                     NDS_MENU_VSRECORD_FIELD);
    if ((sMenuVsRecordScoreRow >= sMenuVsRecordScoreTop) &&
        (sMenuVsRecordScoreCol >= sMenuVsRecordScoreLeft))
    {
        ndsUiKitFillRect(
            NDS_MENU_VSRECORD_SCORE_CELL_X +
                ((s32)(sMenuVsRecordScoreCol - sMenuVsRecordScoreLeft) *
                 NDS_MENU_VSRECORD_SCORE_CELL_W),
            NDS_MENU_VSRECORD_SCORE_ROW_Y +
                ((s32)(sMenuVsRecordScoreRow - sMenuVsRecordScoreTop) *
                 NDS_MENU_VSRECORD_ROW_PITCH),
            (u32)(NDS_MENU_VSRECORD_SCORE_CELL_W - 1),
            (u32)(NDS_MENU_VSRECORD_ROW_PITCH - 1),
            NDS_MENU_VSRECORD_HIGHLIGHT);
    }
    /* The grid (mnVSRecordDrawBattleScoreGrid :719): a line over the icon
     * row and under every row, one left of the icons, after every cell and
     * at the total column's edge. */
    ndsMenuShellVsRecordHLine(1, 255, NDS_MENU_VSRECORD_SCORE_TOP);
    for (r = 0u; r <= rows; r++)
    {
        ndsMenuShellVsRecordHLine(1, 255, NDS_MENU_VSRECORD_SCORE_HEAD +
                                  ((s32)r * NDS_MENU_VSRECORD_ROW_PITCH));
    }
    ndsMenuShellVsRecordVLine(1, NDS_MENU_VSRECORD_SCORE_TOP,
                              NDS_MENU_VSRECORD_SCORE_HEAD +
                              ((s32)rows * NDS_MENU_VSRECORD_ROW_PITCH));
    for (c = 0u; c <= NDS_MENU_VSRECORD_WINDOW; c++)
    {
        ndsMenuShellVsRecordVLine(
            NDS_MENU_VSRECORD_SCORE_CELL_X - 1 +
                ((s32)c * NDS_MENU_VSRECORD_SCORE_CELL_W),
            NDS_MENU_VSRECORD_SCORE_TOP,
            NDS_MENU_VSRECORD_SCORE_HEAD +
                ((s32)rows * NDS_MENU_VSRECORD_ROW_PITCH));
    }
    ndsMenuShellVsRecordVLine(255, NDS_MENU_VSRECORD_SCORE_TOP,
                              NDS_MENU_VSRECORD_SCORE_HEAD +
                              ((s32)rows * NDS_MENU_VSRECORD_ROW_PITCH));
    for (c = 0u; (c < NDS_MENU_VSRECORD_WINDOW) &&
                 ((sMenuVsRecordScoreLeft + c) < count); c++)
    {
        ndsMenuShellVsRecordIcon(
            sMenuVsRecordScoreOrder[sMenuVsRecordScoreLeft + c],
            NDS_MENU_VSRECORD_SCORE_CELL_X + 3 +
                ((s32)c * NDS_MENU_VSRECORD_SCORE_CELL_W),
            NDS_MENU_VSRECORD_SCORE_TOP + 1);
    }
    for (r = 0u; r < rows; r++)
    {
        u32 i = sMenuVsRecordScoreOrder[sMenuVsRecordScoreTop + r];
        s32 y = NDS_MENU_VSRECORD_SCORE_ROW_Y +
                ((s32)r * NDS_MENU_VSRECORD_ROW_PITCH);
        u32 kos = ndsP7VsRecordKOs(sMenuVsRecordData, i);

        ndsMenuShellVsRecordIcon(i, 2, y);
        for (c = 0u; (c < NDS_MENU_VSRECORD_WINDOW) &&
                     ((sMenuVsRecordScoreLeft + c) < count); c++)
        {
            u32 j = sMenuVsRecordScoreOrder[sMenuVsRecordScoreLeft + c];

            ndsMenuShellVsRecordDigits(
                (s32)ndsP7VsRecordKOsAgainst(sMenuVsRecordData, i, j),
                NDS_MENU_VSRECORD_SCORE_CELL_X + 21 +
                    ((s32)c * NDS_MENU_VSRECORD_SCORE_CELL_W),
                y + 5, NDS_MENU_VSRECORD_CREAM, FALSE, FALSE, 4u, FALSE);
        }
        ndsMenuShellVsRecordDigits((s32)kos, NDS_MENU_VSRECORD_SCORE_TOTAL_R,
                                   y + 5, NDS_MENU_VSRECORD_CREAM, FALSE,
                                   FALSE, 6u, FALSE);
    }
    if (count != 0u)
    {
        u32 row = sMenuVsRecordScoreOrder[sMenuVsRecordScoreRow];
        u32 col = sMenuVsRecordScoreOrder[sMenuVsRecordScoreCol];

        strcpy(against, "VS ");
        strncat(against, ndsMenuShellVsRecordName(col),
                sizeof(against) - 4u);
        ndsMenuShellVsRecordLowerScreen("BATTLE SCORE",
                                        ndsMenuShellVsRecordName(row), against,
                                        "PAD MOVE   L R PAGE",
                                        "A RANKING   B BACK");
    }
    else
    {
        ndsMenuShellVsRecordLowerScreen("BATTLE SCORE", "", "",
                                        "", "B BACK");
    }
}

/* One ranking cell (mnVSRecordMakeRankingTableValues :1389), right edge
 * `x`. */
static void ndsMenuShellVsRecordRankValue(u32 column, u32 i, s32 x, s32 y)
{
    const NDSP7VsRecordData *d = sMenuVsRecordData;

    switch (column)
    {
    case 0u:
        ndsMenuShellVsRecordDigits(
            (s32)(ndsP7VsRecordWinPercent(d, i) * 10.0F), x, y,
            NDS_MENU_VSRECORD_CREAM, TRUE, FALSE, 3u, FALSE);
        break;
    case 1u:
        ndsMenuShellVsRecordDigits((s32)ndsP7VsRecordKOs(d, i), x, y,
                                   NDS_MENU_VSRECORD_CREAM, FALSE, FALSE, 6u,
                                   FALSE);
        break;
    case 2u:
        ndsMenuShellVsRecordDigits((s32)ndsP7VsRecordTKO(d, i), x, y,
                                   NDS_MENU_VSRECORD_CREAM, FALSE, FALSE, 6u,
                                   FALSE);
        break;
    case 3u:
        ndsMenuShellVsRecordDigits(
            (s32)(ndsP7VsRecordSDPercent(d, i) * 10.0F), x, y,
            NDS_MENU_VSRECORD_CREAM, TRUE, FALSE, 3u, FALSE);
        break;
    case 4u:
    {
        /* Minutes, colon, two-place seconds (:1484-1515). */
        u32 time = ndsP7VsRecordTimeUsed(d, i);

        ndsMenuShellVsRecordDigits((s32)((time % 3600u) / 60u), x, y,
                                   NDS_MENU_VSRECORD_CREAM, FALSE, FALSE, 2u,
                                   TRUE);
        ndsMenuShellVsRecordPart(NDS_MN_UI_KIT_VSREC_RECT_CREAM_COLON, x - 11,
                                 y);
        ndsMenuShellVsRecordDigits((s32)(time / 3600u), x - 13, y,
                                   NDS_MENU_VSRECORD_CREAM, FALSE, FALSE, 3u,
                                   FALSE);
        break;
    }
    case 5u:
        ndsMenuShellVsRecordDigits(
            (s32)(ndsP7VsRecordUsePercent(d, i) * 10.0F), x, y,
            NDS_MENU_VSRECORD_CREAM, TRUE, FALSE, 3u, FALSE);
        break;
    default:
        ndsMenuShellVsRecordDigits((s32)(ndsP7VsRecordAvg(d, i) * 10.0F), x,
                                   y, NDS_MENU_VSRECORD_CREAM, TRUE, FALSE,
                                   1u, FALSE);
        break;
    }
}

static void ndsMenuShellVsRecordDrawRank(void)
{
    u32 count = ndsMenuShellVsRecordCount();
    u32 rows = (count < NDS_MENU_VSRECORD_WINDOW) ? count :
        NDS_MENU_VSRECORD_WINDOW;
    s32 bottom = NDS_MENU_VSRECORD_RANK_HEAD +
                 ((s32)rows * NDS_MENU_VSRECORD_ROW_PITCH);
    s32 x;
    u32 r;
    u32 k;

    ndsMenuShellVsRecordFollow(sMenuVsRecordCurrent, count,
                               &sMenuVsRecordRankTop);
    /* The table, not the base's header or the side arrows over the icons. */
    ndsUiKitFillRect(20, NDS_MENU_VSRECORD_RANK_TOP, 236u,
                     (u32)(NDS_MENU_VSRECORD_RANK_HEAD -
                           NDS_MENU_VSRECORD_RANK_TOP),
                     NDS_MENU_VSRECORD_FIELD);
    ndsUiKitFillRect(0, NDS_MENU_VSRECORD_RANK_HEAD, 256u,
                     (u32)(192 - NDS_MENU_VSRECORD_RANK_HEAD),
                     NDS_MENU_VSRECORD_FIELD);
    if ((count != 0u) && (sMenuVsRecordCurrent >= sMenuVsRecordRankTop))
    {
        /* Between the table's outer lines (x 1 and 253). */
        ndsUiKitFillRect(2, NDS_MENU_VSRECORD_RANK_ROW_Y +
                         ((s32)(sMenuVsRecordCurrent - sMenuVsRecordRankTop) *
                          NDS_MENU_VSRECORD_ROW_PITCH),
                         251u, (u32)(NDS_MENU_VSRECORD_ROW_PITCH - 1),
                         NDS_MENU_VSRECORD_HIGHLIGHT);
    }
    /* mnVSRecordDrawRankingGrid (:740). */
    ndsMenuShellVsRecordHLine(1, 253, NDS_MENU_VSRECORD_RANK_TOP);
    for (r = 0u; r <= rows; r++)
    {
        ndsMenuShellVsRecordHLine(1, 253, NDS_MENU_VSRECORD_RANK_HEAD +
                                  ((s32)r * NDS_MENU_VSRECORD_ROW_PITCH));
    }
    /* The side arrows sit in the label row's icon cell, so its left edge
     * starts under them. */
    ndsMenuShellVsRecordVLine(1, NDS_MENU_VSRECORD_RANK_HEAD, bottom);
    x = NDS_MENU_VSRECORD_RANK_COL_X;
    ndsMenuShellVsRecordVLine(x - 1, NDS_MENU_VSRECORD_RANK_TOP, bottom);
    for (k = 0u; k < NDS_MENU_VSRECORD_COLUMNS; k++)
    {
        u32 column = (sMenuVsRecordFirstColumn + k) % NDS_MENU_VSRECORD_COLUMNS;
        const u8 *label = kNdsUiKitVsrecRects[kMenuVsRecordRankLabel[column]];

        ndsMenuShellVsRecordPart(kMenuVsRecordRankLabel[column],
                                 x + kMenuVsRecordRankPad[column],
                                 NDS_MENU_VSRECORD_RANK_LABEL_Y +
                                 (7 - (s32)label[3]));
        for (r = 0u; r < rows; r++)
        {
            ndsMenuShellVsRecordRankValue(
                column, sMenuVsRecordRankOrder[sMenuVsRecordRankTop + r],
                x + (s32)kMenuVsRecordRankWidth[column] - 4,
                NDS_MENU_VSRECORD_RANK_ROW_Y + 5 +
                    ((s32)r * NDS_MENU_VSRECORD_ROW_PITCH));
        }
        x += (s32)kMenuVsRecordRankWidth[column];
        ndsMenuShellVsRecordVLine(x - 1, NDS_MENU_VSRECORD_RANK_TOP, bottom);
    }
    for (r = 0u; r < rows; r++)
    {
        ndsMenuShellVsRecordIcon(
            sMenuVsRecordRankOrder[sMenuVsRecordRankTop + r], 2,
            NDS_MENU_VSRECORD_RANK_ROW_Y +
                ((s32)r * NDS_MENU_VSRECORD_ROW_PITCH));
    }
    if (count != 0u)
    {
        ndsMenuShellVsRecordLowerScreen(
            "RANKING",
            ndsMenuShellVsRecordName(
                sMenuVsRecordRankOrder[sMenuVsRecordCurrent]),
            kMenuVsRecordRankName[sMenuVsRecordFirstColumn],
            "UP DOWN MOVE   L R SORT", "A INDIVIDUAL   B BACK");
    }
    else
    {
        ndsMenuShellVsRecordLowerScreen("RANKING", "", "", "", "B BACK");
    }
}

static void ndsMenuShellVsRecordDrawIndiv(void)
{
    const NDSP7VsRecordData *d = sMenuVsRecordData;
    u32 count = ndsMenuShellVsRecordCount();
    u32 cur;
    u32 sel;
    u32 cols;
    u32 c;
    u32 r;
    s32 y;

    if (count == 0u)
    {
        ndsMenuShellVsRecordLowerScreen("INDIVIDUAL", "", "", "", "B BACK");
        return;
    }
    cur = sMenuVsRecordRankOrder[sMenuVsRecordCurrent];
    sel = ndsMenuShellVsRecordSel(cur);
    if ((sel < NDS_MN_UI_KIT_VSREC_SELS) && (sel != sMenuVsRecordPortrait))
    {
        NdsUiKitSurfaceId portrait =
            (NdsUiKitSurfaceId)(NDS_MN_UI_KIT_SURFACE_VSREC_PORTRAIT_0 + sel);

        (void)ndsUiKitBlitSurfaces(&portrait, 1u);
        sMenuVsRecordPortrait = sel;
    }
    /* The portrait stats (mnVSRecordMakePortraitStats :1068): rank of the
     * shown, use percent and the damage totals, grey and wide. */
    ndsUiKitFillRect(200, NDS_MENU_VSRECORD_INDIV_LINE_Y, 56u,
                     (u32)(4 * NDS_MENU_VSRECORD_INDIV_LINE_STEP),
                     NDS_MENU_VSRECORD_FIELD);
    y = NDS_MENU_VSRECORD_INDIV_LINE_Y + 1;
    ndsMenuShellVsRecordDigits((s32)count, NDS_MENU_VSRECORD_INDIV_VALUE_R, y,
                               NDS_MENU_VSRECORD_GREY, FALSE, TRUE, 2u, FALSE);
    ndsMenuShellVsRecordPart(NDS_MN_UI_KIT_VSREC_RECT_GREY_SLASH,
                             NDS_MENU_VSRECORD_INDIV_VALUE_R - 14, y + 1);
    ndsMenuShellVsRecordDigits((s32)ndsMenuShellVsRecordRanking(cur),
                               NDS_MENU_VSRECORD_INDIV_VALUE_R - 15, y,
                               NDS_MENU_VSRECORD_GREY, FALSE, TRUE, 2u, FALSE);
    y += NDS_MENU_VSRECORD_INDIV_LINE_STEP;
    ndsMenuShellVsRecordDigits((s32)(ndsP7VsRecordUsePercent(d, cur) * 10.0F),
                               NDS_MENU_VSRECORD_INDIV_VALUE_R, y,
                               NDS_MENU_VSRECORD_GREY, TRUE, TRUE, 3u, FALSE);
    y += NDS_MENU_VSRECORD_INDIV_LINE_STEP;
    ndsMenuShellVsRecordDigits((s32)d->rows[sel].damage_given,
                               NDS_MENU_VSRECORD_INDIV_VALUE_R, y,
                               NDS_MENU_VSRECORD_GREY, FALSE, TRUE, 6u, FALSE);
    y += NDS_MENU_VSRECORD_INDIV_LINE_STEP;
    ndsMenuShellVsRecordDigits((s32)d->rows[sel].damage_taken,
                               NDS_MENU_VSRECORD_INDIV_VALUE_R, y,
                               NDS_MENU_VSRECORD_GREY, FALSE, TRUE, 6u, FALSE);

    /* The table of opponents (mnVSRecordMakeIndivTableValues :1648), eight
     * a page, over mnVSRecordDrawIndivGrid's lines (:776). */
    ndsUiKitFillRect(NDS_MENU_VSRECORD_INDIV_COL_X - 1,
                     NDS_MENU_VSRECORD_INDIV_TOP,
                     (u32)(256 - NDS_MENU_VSRECORD_INDIV_COL_X + 1),
                     (u32)(192 - NDS_MENU_VSRECORD_INDIV_TOP),
                     NDS_MENU_VSRECORD_FIELD);
    ndsUiKitFillRect(0, NDS_MENU_VSRECORD_INDIV_TOP, 2u,
                     (u32)(192 - NDS_MENU_VSRECORD_INDIV_TOP),
                     NDS_MENU_VSRECORD_FIELD);
    cols = count - sMenuVsRecordIndivLeft;
    if (cols > NDS_MENU_VSRECORD_WINDOW)
    {
        cols = NDS_MENU_VSRECORD_WINDOW;
    }
    ndsMenuShellVsRecordHLine(1, 251, NDS_MENU_VSRECORD_INDIV_TOP);
    for (r = 0u; r <= NDS_MENU_VSRECORD_INDIV_ROWS; r++)
    {
        ndsMenuShellVsRecordHLine(1, 251, NDS_MENU_VSRECORD_INDIV_HEAD +
                                  ((s32)r * NDS_MENU_VSRECORD_INDIV_ROW_STEP));
    }
    ndsMenuShellVsRecordVLine(1, NDS_MENU_VSRECORD_INDIV_TOP,
                              NDS_MENU_VSRECORD_INDIV_BOTTOM);
    for (c = 0u; c <= NDS_MENU_VSRECORD_WINDOW; c++)
    {
        ndsMenuShellVsRecordVLine(NDS_MENU_VSRECORD_INDIV_COL_X - 1 +
                                  ((s32)c * NDS_MENU_VSRECORD_INDIV_COL_W),
                                  NDS_MENU_VSRECORD_INDIV_TOP,
                                  NDS_MENU_VSRECORD_INDIV_BOTTOM);
    }
    for (c = 0u; c < cols; c++)
    {
        u32 j = sMenuVsRecordIndivOrder[sMenuVsRecordIndivLeft + c];
        s32 x = NDS_MENU_VSRECORD_INDIV_COL_X +
                ((s32)c * NDS_MENU_VSRECORD_INDIV_COL_W);
        s32 right = x + NDS_MENU_VSRECORD_INDIV_COL_W - 3;
        u32 kos = ndsP7VsRecordKOsAgainst(d, cur, j);
        u32 kod = ndsP7VsRecordKOsAgainst(d, j, cur);

        ndsMenuShellVsRecordIcon(j, x + 5, NDS_MENU_VSRECORD_INDIV_ICON_Y);
        y = NDS_MENU_VSRECORD_INDIV_ROW_Y + 4;
        ndsMenuShellVsRecordDigits(
            (s32)(ndsP7VsRecordWinPercentAgainst(d, cur, j) * 10.0F), right, y,
            NDS_MENU_VSRECORD_CREAM, TRUE, FALSE, 3u, FALSE);
        y += NDS_MENU_VSRECORD_INDIV_ROW_STEP;
        ndsMenuShellVsRecordDigits(
            (s32)((kos > NDS_MENU_VSRECORD_COUNT_MAX) ?
                  NDS_MENU_VSRECORD_COUNT_MAX : kos), right, y,
            NDS_MENU_VSRECORD_CREAM, FALSE, FALSE, 4u, FALSE);
        y += NDS_MENU_VSRECORD_INDIV_ROW_STEP;
        ndsMenuShellVsRecordDigits(
            (s32)((kod > NDS_MENU_VSRECORD_COUNT_MAX) ?
                  NDS_MENU_VSRECORD_COUNT_MAX : kod), right, y,
            NDS_MENU_VSRECORD_CREAM, FALSE, FALSE, 4u, FALSE);
        y += NDS_MENU_VSRECORD_INDIV_ROW_STEP;
        ndsMenuShellVsRecordDigits(
            (s32)(ndsP7VsRecordAvgAgainst(d, cur, j) * 10.0F), right, y,
            NDS_MENU_VSRECORD_CREAM, TRUE, FALSE, 3u, FALSE);
    }
    ndsMenuShellVsRecordLowerScreen("INDIVIDUAL",
                                    ndsMenuShellVsRecordName(cur), "",
                                    "L R FIGHTER", "UP DOWN PAGE   B BACK");
}

static void ndsMenuShellVsRecordRefresh(void)
{
    if (sMenuVsRecordReady == FALSE)
    {
        return;
    }
    switch (sMenuVsRecordView)
    {
    case NDS_MENU_VSRECORD_VIEW_SCORE:
        ndsMenuShellVsRecordDrawScore();
        break;
    case NDS_MENU_VSRECORD_VIEW_RANK:
        ndsMenuShellVsRecordDrawRank();
        break;
    default:
        ndsMenuShellVsRecordDrawIndiv();
        break;
    }
}

/* mnVSRecordRedrawStats (:1935) on a view change: the view's base, its
 * sort, then its table. */
static void ndsMenuShellVsRecordEnterView(u32 view)
{
    NdsUiKitSurfaceId base;

    sMenuVsRecordView = view;
    if (sMenuVsRecordReady == FALSE)
    {
        return;
    }
    switch (view)
    {
    case NDS_MENU_VSRECORD_VIEW_SCORE:
        base = NDS_MN_UI_KIT_SURFACE_VSREC_BASE_SCORE;
        ndsMenuShellVsRecordSort(sMenuVsRecordScoreOrder, view);
        break;
    case NDS_MENU_VSRECORD_VIEW_RANK:
        base = NDS_MN_UI_KIT_SURFACE_VSREC_BASE_RANK;
        ndsMenuShellVsRecordSort(sMenuVsRecordRankOrder, view);
        break;
    default:
        base = NDS_MN_UI_KIT_SURFACE_VSREC_BASE_INDIV;
        ndsMenuShellVsRecordSort(sMenuVsRecordIndivOrder, view);
        sMenuVsRecordIndivLeft = 0u;
        break;
    }
    (void)ndsUiKitBlitSurfaces(&base, 1u);
    sMenuVsRecordPortrait = 0xffffffffu;
    ndsMenuShellVsRecordRefresh();
}

/* mnVSRecordInitVars (:1924): Battle Score, cursor and first column at
 * their starts. */
static void ndsMenuShellVsRecordLoad(void)
{
    sMenuVsRecordView = NDS_MENU_VSRECORD_VIEW_SCORE;
    sMenuVsRecordCurrent = 0u;
    sMenuVsRecordFirstColumn = 0u;
    sMenuVsRecordScoreRow = 0u;
    sMenuVsRecordScoreCol = 0u;
    sMenuVsRecordScoreTop = 0u;
    sMenuVsRecordScoreLeft = 0u;
    sMenuVsRecordRankTop = 0u;
    sMenuVsRecordIndivLeft = 0u;
    sMenuVsRecordReady = FALSE;
}

static void ndsMenuShellPopulateVsRecord(void)
{
    u32 i;

    for (i = 0u; i < NDS_UI_KIT_TEXT_SLOTS; i++)
    {
        ndsUiKitHideText(i);
    }
    /* Without its arena or its atlas the screen draws nothing, and B still
     * returns to DATA. */
    sMenuVsRecordReady = FALSE;
    if (ndsMenuShellVsRecordAlloc() == FALSE)
    {
        return;
    }
    ndsP7VsRecordLoad(sMenuVsRecordData);
    if (ndsUiKitLoadSurfaceCopy(
            NDS_MN_UI_KIT_SURFACE_VSREC_ATLAS, sMenuVsRecordAtlas,
            kNdsUiKitSurfaceMetrics[NDS_MN_UI_KIT_SURFACE_VSREC_ATLAS].bytes) ==
        FALSE)
    {
        return;
    }
    ndsMenuShellVsRecordGridOrder();
    /* The Ranking order is the Individual view's fighter list, so it exists
     * from the start (the source sorts it at its view's first draw). */
    ndsMenuShellVsRecordSort(sMenuVsRecordRankOrder,
                             NDS_MENU_VSRECORD_VIEW_RANK);
    sMenuVsRecordReady = TRUE;
    ndsMenuShellVsRecordEnterView(sMenuVsRecordView);
}

/* One step of `cursor` over `count` with wraparound. */
static u32 ndsMenuShellVsRecordStep(u32 cursor, u32 count, s32 delta)
{
    if (count == 0u)
    {
        return 0u;
    }
    if (delta < 0)
    {
        return (cursor == 0u) ? (count - 1u) : (cursor - 1u);
    }
    return ((cursor + 1u) >= count) ? 0u : (cursor + 1u);
}

static void ndsMenuShellUpdateVsRecord(u32 held, u32 taps)
{
    u32 count = ndsMenuShellVsRecordCount();

    if ((taps & NDS_INPUT_B) != 0u)
    {
        if (sMenuVsRecordView == NDS_MENU_VSRECORD_VIEW_SCORE)
        {
            ndsMenuShellGoto((u32)nSCKindData);
            return;
        }
        ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
        ndsMenuShellVsRecordEnterView(sMenuVsRecordView - 1u);
        return;
    }
    if (((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u) &&
        (sMenuVsRecordView < NDS_MENU_VSRECORD_VIEW_INDIV))
    {
        ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
        ndsMenuShellVsRecordEnterView(sMenuVsRecordView + 1u);
        return;
    }
    if ((sMenuVsRecordReady == FALSE) || (count == 0u))
    {
        return;
    }
    switch (sMenuVsRecordView)
    {
    case NDS_MENU_VSRECORD_VIEW_SCORE:
        if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
        {
            sMenuVsRecordScoreRow =
                ndsMenuShellVsRecordStep(sMenuVsRecordScoreRow, count, -1);
        }
        else if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
        {
            sMenuVsRecordScoreRow =
                ndsMenuShellVsRecordStep(sMenuVsRecordScoreRow, count, 1);
        }
        else if (ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT) != FALSE)
        {
            sMenuVsRecordScoreCol =
                ndsMenuShellVsRecordStep(sMenuVsRecordScoreCol, count, -1);
        }
        else if (ndsMenuShellDirection(held, taps, NDS_INPUT_RIGHT) != FALSE)
        {
            sMenuVsRecordScoreCol =
                ndsMenuShellVsRecordStep(sMenuVsRecordScoreCol, count, 1);
        }
        else if ((taps & (NDS_INPUT_L | NDS_INPUT_R)) != 0u)
        {
            /* A page of columns, the cursor keeping its place in it. */
            u32 page = NDS_MENU_VSRECORD_WINDOW;

            if ((taps & NDS_INPUT_R) != 0u)
            {
                sMenuVsRecordScoreCol =
                    ((sMenuVsRecordScoreCol + page) < count) ?
                    (sMenuVsRecordScoreCol + page) : (count - 1u);
                sMenuVsRecordScoreLeft += page;
            }
            else
            {
                sMenuVsRecordScoreCol = (sMenuVsRecordScoreCol >= page) ?
                    (sMenuVsRecordScoreCol - page) : 0u;
                sMenuVsRecordScoreLeft = (sMenuVsRecordScoreLeft >= page) ?
                    (sMenuVsRecordScoreLeft - page) : 0u;
            }
            ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
            ndsMenuShellVsRecordRefresh();
            return;
        }
        else
        {
            return;
        }
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        ndsMenuShellVsRecordRefresh();
        return;
    case NDS_MENU_VSRECORD_VIEW_RANK:
        if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
        {
            sMenuVsRecordCurrent =
                ndsMenuShellVsRecordStep(sMenuVsRecordCurrent, count, -1);
            ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
            ndsMenuShellVsRecordRefresh();
        }
        else if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
        {
            sMenuVsRecordCurrent =
                ndsMenuShellVsRecordStep(sMenuVsRecordCurrent, count, 1);
            ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
            ndsMenuShellVsRecordRefresh();
        }
        else if ((ndsMenuShellDirection(held, taps, NDS_INPUT_RIGHT) != FALSE) ||
                 ((taps & NDS_INPUT_R) != 0u))
        {
            /* The source's R steps FirstColumn back (:2096). */
            sMenuVsRecordFirstColumn = ndsMenuShellVsRecordStep(
                sMenuVsRecordFirstColumn, NDS_MENU_VSRECORD_COLUMNS, -1);
            ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
            ndsMenuShellVsRecordSort(sMenuVsRecordRankOrder,
                                     NDS_MENU_VSRECORD_VIEW_RANK);
            ndsMenuShellVsRecordRefresh();
        }
        else if ((ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT) != FALSE) ||
                 ((taps & NDS_INPUT_L) != 0u))
        {
            sMenuVsRecordFirstColumn = ndsMenuShellVsRecordStep(
                sMenuVsRecordFirstColumn, NDS_MENU_VSRECORD_COLUMNS, 1);
            ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
            ndsMenuShellVsRecordSort(sMenuVsRecordRankOrder,
                                     NDS_MENU_VSRECORD_VIEW_RANK);
            ndsMenuShellVsRecordRefresh();
        }
        return;
    default:
        if ((ndsMenuShellDirection(held, taps, NDS_INPUT_RIGHT) != FALSE) ||
            ((taps & NDS_INPUT_R) != 0u))
        {
            sMenuVsRecordCurrent =
                ndsMenuShellVsRecordStep(sMenuVsRecordCurrent, count, 1);
        }
        else if ((ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT) != FALSE) ||
                 ((taps & NDS_INPUT_L) != 0u))
        {
            sMenuVsRecordCurrent =
                ndsMenuShellVsRecordStep(sMenuVsRecordCurrent, count, -1);
        }
        else if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
        {
            if ((sMenuVsRecordIndivLeft + NDS_MENU_VSRECORD_WINDOW) < count)
            {
                sMenuVsRecordIndivLeft += NDS_MENU_VSRECORD_WINDOW;
                ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
                ndsMenuShellVsRecordRefresh();
            }
            return;
        }
        else if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
        {
            if (sMenuVsRecordIndivLeft != 0u)
            {
                sMenuVsRecordIndivLeft -= NDS_MENU_VSRECORD_WINDOW;
                ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
                ndsMenuShellVsRecordRefresh();
            }
            return;
        }
        else
        {
            return;
        }
        /* A new fighter re-sorts its opponents from the first page. */
        ndsMenuShellVsRecordSort(sMenuVsRecordIndivOrder,
                                 NDS_MENU_VSRECORD_VIEW_INDIV);
        sMenuVsRecordIndivLeft = 0u;
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        ndsMenuShellVsRecordRefresh();
        return;
    }
}
