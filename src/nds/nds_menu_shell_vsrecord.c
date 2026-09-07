/* --- Screen: VS Record (first view) -----------------------------------------
 *
 * The per-fighter battle-score summary behind the DATA menu's VS RECORD row
 * (decomp mn/mndata/mnvsrecord.c, entry kind nMNVSRecordKindBattleScore).
 * One row per fighter in the source's own fighter order
 * (nFTKindPlayableStart..nFTKindPlayableEnd, the order the source's own
 * GetKOs/GetTKO loops walk), each with the fighter name and the KO / TKO /
 * SD counts plus win percent as kit numbers. UP and DOWN walk the cursor
 * with wraparound, L and R page the rows, B returns to DATA. Transcribed
 * from mnvsrecord.c:
 *
 *   math     mnVSRecordGetKOs (:141-154: row sum of ko_count over
 *            have-kinds), mnVSRecordGetTKO (:157-174: column sum plus
 *            selfdestructs, capped at 9999), mnVSRecordGetTotalTKO
 *            (:177-190), mnVSRecordGetWinPercent (:193-199:
 *            KOs/TotalTKO*100, 0 when the total is 0). SD here is the raw
 *            selfdestructs count feeding the source's SD percent
 *            (:1373-1386); the percent itself is a Ranking-column view and
 *            stays there. Win percent draws rounded to whole points; the
 *            source's tenths digit is a Ranking-view digit sprite this kit
 *            has no cell for.
 *   mask     mnVSRecordCheckHaveFighterKind (:502-521: Ness/Purin/Captain/
 *            Luigi gated on the snapshot mask, everything else admitted)
 *            over the mnVSRecordInitVars snapshot (:1924-1932: cursor 0,
 *            mask from gSCManagerBackupData.fighter_mask).
 *   B        on the BattleScore kind returns to nSCKindData with no cue
 *            (:1974-1982; the BurnS cue at :1985/:2003 belongs to the
 *            kind-step arms, and the kit carries neither BurnS nor FoxFoot,
 *            so kind steps and their cues stay out until a later slice
 *            wires the Ranking/Indiv views).
 *   A/START  steps StatsKind forward in the source (:1996-2009); only the
 *            first view exists here, so it is a refusal (MenuDenied, the
 *            kit's own refusal cue) that stays on the screen.
 *   U/D      walks the cursor with wraparound (the Ranking arms
 *            :2014-2060, minus the locked-kind skip: all twelve rows draw,
 *            so there is nothing to skip). Cue MenuScroll2, the shell's
 *            cursor cue. L/R pages the rows; cue MenuScroll1, the shell's
 *            value cue.
 *   BGM      FuncStart plays the Data track unconditionally (:2201); see
 *            ndsMenuShellRunVsRecord.
 *   idle     none in mnVSRecordFuncRun (:1949-2165 carries only the change
 *            wait plus the inputs above); attract behaviour owned by P2-7,
 *            deliberately absent like the DATA screen's, not stubbed.
 *
 * PRESENTATION. The source draws this view as a 12x12 KO matrix plus a
 * total column of digit sprites over RDP-fill grids (:1238-1280, :719);
 * the kit has 45 sprite slots and one matrix cell per place would need
 * more than three times that, so the first view is the per-fighter
 * summary instead: the kit's own font text for names (cursor row in the
 * DATA tab HIGHLIGHT pair, the rest in NOT, like the DATA rows) and
 * ndsUiKitSetNumber digits right-aligned over four columns. Twelve rows
 * do not fit the eight text slots, hence paging: three rows a page, four
 * pages, which is also the only page size whose worst case (4+4+4+3
 * places a row) exactly fits the 45 number slots with no cursor sprite
 * to spend. A steady page composes nothing (every kit call is
 * content-keyed); a cursor move recomposes only what changed.
 *
 * TODO(VSRECORD-KIT): bake the VS Record sprites and replace the font
 * rows -- llMNVSRecordMainLabelSprite (header),
 * llMNVSRecordMainBattleScoreSprite (subtitle),
 * llMNVSRecordMainDigit0Sprite..Digit9 + SymbolPointSprite (digits),
 * llMNVSRecordMainLabelTotalSprite (total label),
 * llMNVSRecordMainMarioIconColorSprite.. (row icons),
 * llMNDataCommonDataHeaderSprite + ArrowLSprite/RSprite (chrome),
 * llMNCommonFontsLetterASprite.. (font) -- following the Option
 * screen's row-surface shape (nds_menu_shell_option.c). Ranking and
 * Indiv views ride the same bake when they land.
 */

#ifndef NDS_MENU_SHELL_SCREEN_VSRECORD
#define NDS_MENU_SHELL_SCREEN_VSRECORD 11u
#endif

/* Twelve playable fighters, three rows a page: the only page size whose
 * worst-case number cost (KO 4 + TKO 4 + SD 4 + WIN 3 places a row) fits
 * the kit's 45 sprite slots exactly. */
#define NDS_MENU_VSRECORD_FIGHTERS \
    ((u32)nFTKindPlayableEnd - (u32)nFTKindPlayableStart + 1u)
#define NDS_MENU_VSRECORD_ROWS_PER_PAGE 3u
#define NDS_MENU_VSRECORD_PAGES \
    (NDS_MENU_VSRECORD_FIGHTERS / NDS_MENU_VSRECORD_ROWS_PER_PAGE)

/* The DATA tab primitive colours (mnDataSetOptionSpriteColors): the cursor
 * row reads as the selected row in the same language as the DATA menu. */
#define NDS_MENU_VSRECORD_RGB_HI 0x00ff0028u
#define NDS_MENU_VSRECORD_RGB_NOT 0x008282aau

/* Table geometry, DS pixels. Names left, four right-aligned number
 * columns; legend row above the first data row. */
#define NDS_MENU_VSRECORD_X_NAME 12
#define NDS_MENU_VSRECORD_R_KO 128
#define NDS_MENU_VSRECORD_R_TKO 168
#define NDS_MENU_VSRECORD_R_SD 204
#define NDS_MENU_VSRECORD_R_WIN 248
#define NDS_MENU_VSRECORD_Y_LEGEND 30
#define NDS_MENU_VSRECORD_Y0 48
#define NDS_MENU_VSRECORD_DY 26

/* Display caps. TKO and the ko_count/selfdestructs fields already cap at
 * 9999 in (and under) the source; the KO row sum has no source cap, so
 * the display clamps it rather than growing a fifth place. */
#define NDS_MENU_VSRECORD_COUNT_MAX 9999

static u32 sMenuVsRecordCursor;
static u32 sMenuVsRecordMask;
static u32 sMenuVsRecordNumSlot;

/* Fighter names in nFTKindPlayableStart..End order: the kit font carries
 * A-Z only, so CAPTAIN and PURIN ship unadorned rather than as C.FALCON
 * and JIGGLYPUFF punctuation the kit cannot draw. */
static const char *kMenuVsRecordFighterNames[(nFTKindPlayableEnd - nFTKindPlayableStart) + 1] = {
    "MARIO",
    "FOX",
    "DONKEY",
    "SAMUS",
    "LUIGI",
    "LINK",
    "YOSHI",
    "CAPTAIN",
    "KIRBY",
    "PIKACHU",
    "PURIN",
    "NESS"
};

/* mnVSRecordCheckHaveFighterKind (mnvsrecord.c:502-521) over this screen's
 * own InitVars snapshot, not the source TU's: the import compiles out
 * whenever its gate is off and this screen must not link against it. */
static u32 ndsMenuShellVsRecordHaveKind(s32 fkind)
{
    switch (fkind)
    {
    case nFTKindNess:
        return ((sMenuVsRecordMask &
                 LBBACKUP_MASK_FIGHTER(nFTKindNess)) != 0u) ? TRUE : FALSE;
    case nFTKindPurin:
        return ((sMenuVsRecordMask &
                 LBBACKUP_MASK_FIGHTER(nFTKindPurin)) != 0u) ? TRUE : FALSE;
    case nFTKindCaptain:
        return ((sMenuVsRecordMask &
                 LBBACKUP_MASK_FIGHTER(nFTKindCaptain)) != 0u) ? TRUE : FALSE;
    case nFTKindLuigi:
        return ((sMenuVsRecordMask &
                 LBBACKUP_MASK_FIGHTER(nFTKindLuigi)) != 0u) ? TRUE : FALSE;
    default:
        break;
    }
    return TRUE;
}

/* mnVSRecordGetKOs (mnvsrecord.c:141-154) verbatim. */
static s32 ndsMenuShellVsRecordGetKOs(s32 fkind)
{
    s32 i;
    s32 total_kos = 0;

    for (i = nFTKindPlayableStart; i <= nFTKindPlayableEnd; i++)
    {
        if (ndsMenuShellVsRecordHaveKind(i) != FALSE)
        {
            total_kos += gSCManagerBackupData.vs_records[fkind].ko_count[i];
        }
    }
    return total_kos;
}

/* mnVSRecordGetTKO (mnvsrecord.c:157-174) verbatim, cap included. */
static s32 ndsMenuShellVsRecordGetTKO(s32 fkind)
{
    s32 i;
    s32 total_tkos = 0;

    for (i = nFTKindPlayableStart; i <= nFTKindPlayableEnd; i++)
    {
        if (ndsMenuShellVsRecordHaveKind(i) != FALSE)
        {
            total_tkos += gSCManagerBackupData.vs_records[i].ko_count[fkind];
        }
    }
    if ((gSCManagerBackupData.vs_records[fkind].selfdestructs +
         total_tkos) > NDS_MENU_VSRECORD_COUNT_MAX)
    {
        return NDS_MENU_VSRECORD_COUNT_MAX;
    }
    return gSCManagerBackupData.vs_records[fkind].selfdestructs + total_tkos;
}

/* mnVSRecordGetTotalTKO (mnvsrecord.c:177-190) verbatim. */
static s32 ndsMenuShellVsRecordGetTotalTKO(void)
{
    s32 i;
    s32 total_tkos = 0;

    for (i = nFTKindPlayableStart; i <= nFTKindPlayableEnd; i++)
    {
        if (ndsMenuShellVsRecordHaveKind(i) != FALSE)
        {
            total_tkos += ndsMenuShellVsRecordGetTKO(i);
        }
    }
    return total_tkos;
}

/* mnVSRecordGetWinPercent (mnvsrecord.c:193-199) verbatim. */
static f32 ndsMenuShellVsRecordGetWinPercent(s32 fkind)
{
    f32 kos = (f32)ndsMenuShellVsRecordGetKOs(fkind);
    f32 tko = (f32)ndsMenuShellVsRecordGetTotalTKO();

    return ((tko != 0.0F) ? kos / tko : 0.0F) * 100.0F;
}

/* One right-aligned number out of the running sprite-slot cursor. Places
 * beyond the slots left are dropped by the kit call itself; the page size
 * above is what keeps that arm unreachable. */
static void ndsMenuShellVsRecordNumber(s32 value, s32 right_x, s32 y,
                                       u32 max_places)
{
    u32 avail = (sMenuVsRecordNumSlot >= NDS_UI_KIT_SPRITE_SLOTS) ?
        0u : (NDS_UI_KIT_SPRITE_SLOTS - sMenuVsRecordNumSlot);
    u32 used;

    if (avail > max_places)
    {
        avail = max_places;
    }
    if (avail == 0u)
    {
        return;
    }
    used = ndsUiKitSetNumber(sMenuVsRecordNumSlot, avail, value, right_x, y);
    sMenuVsRecordNumSlot += used;
}

/* One full redraw. Text slots are content-keyed (nds_ui_kit.c), so a call
 * that changed nothing recomposes nothing; refresh runs on populate and
 * on cursor/page moves only. */
static void ndsMenuShellVsRecordRefresh(void)
{
    u32 page = sMenuVsRecordCursor / NDS_MENU_VSRECORD_ROWS_PER_PAGE;
    u32 row;
    u32 slot;

    ndsUiKitSetText(NDS_MENU_SLOT_HEADER, "VS RECORD", NDS_MENU_RGB_HEADER);
    ndsUiKitMoveText(NDS_MENU_SLOT_HEADER, NDS_MENU_HEADER_X,
                     NDS_MENU_HEADER_Y);
    ndsUiKitSetText(NDS_MENU_SLOT_ROW3, "KO", NDS_MENU_RGB_HEADER);
    ndsUiKitMoveText(NDS_MENU_SLOT_ROW3,
                     (s32)NDS_MENU_VSRECORD_R_KO -
                     (s32)ndsUiKitTextWidth("KO"),
                     NDS_MENU_VSRECORD_Y_LEGEND);
    ndsUiKitSetText(NDS_MENU_SLOT_EXTRA, "TKO", NDS_MENU_RGB_HEADER);
    ndsUiKitMoveText(NDS_MENU_SLOT_EXTRA,
                     (s32)NDS_MENU_VSRECORD_R_TKO -
                     (s32)ndsUiKitTextWidth("TKO"),
                     NDS_MENU_VSRECORD_Y_LEGEND);
    ndsUiKitSetText(6u, "SD", NDS_MENU_RGB_HEADER);
    ndsUiKitMoveText(6u,
                     (s32)NDS_MENU_VSRECORD_R_SD -
                     (s32)ndsUiKitTextWidth("SD"),
                     NDS_MENU_VSRECORD_Y_LEGEND);
    ndsUiKitSetText(7u, "WIN%", NDS_MENU_RGB_HEADER);
    ndsUiKitMoveText(7u,
                     (s32)NDS_MENU_VSRECORD_R_WIN -
                     (s32)ndsUiKitTextWidth("WIN%"),
                     NDS_MENU_VSRECORD_Y_LEGEND);

    sMenuVsRecordNumSlot = 0u;
    for (row = 0u; row < NDS_MENU_VSRECORD_ROWS_PER_PAGE; row++)
    {
        u32 index = (page * NDS_MENU_VSRECORD_ROWS_PER_PAGE) + row;
        u32 text_slot = NDS_MENU_SLOT_ROW0 + row;
        s32 fkind = (s32)nFTKindPlayableStart + (s32)index;
        s32 kos;
        s32 tko;
        s32 win;
        s32 y = (s32)NDS_MENU_VSRECORD_Y0 +
                ((s32)row * (s32)NDS_MENU_VSRECORD_DY);
        u32 rgb = (index == sMenuVsRecordCursor) ?
            NDS_MENU_VSRECORD_RGB_HI : NDS_MENU_VSRECORD_RGB_NOT;

        ndsUiKitSetText(text_slot, kMenuVsRecordFighterNames[index], rgb);
        ndsUiKitMoveText(text_slot, NDS_MENU_VSRECORD_X_NAME, y);

        kos = ndsMenuShellVsRecordGetKOs(fkind);
        if (kos > NDS_MENU_VSRECORD_COUNT_MAX)
        {
            kos = NDS_MENU_VSRECORD_COUNT_MAX;
        }
        ndsMenuShellVsRecordNumber(kos, (s32)NDS_MENU_VSRECORD_R_KO, y, 4u);
        tko = ndsMenuShellVsRecordGetTKO(fkind);
        ndsMenuShellVsRecordNumber(tko, (s32)NDS_MENU_VSRECORD_R_TKO, y, 4u);
        ndsMenuShellVsRecordNumber(
            (s32)gSCManagerBackupData.vs_records[fkind].selfdestructs,
            (s32)NDS_MENU_VSRECORD_R_SD, y, 4u);
        win = (s32)(ndsMenuShellVsRecordGetWinPercent(fkind) + 0.5F);
        if (win > 100)
        {
            win = 100;
        }
        if (win < 0)
        {
            win = 0;
        }
        ndsMenuShellVsRecordNumber(win, (s32)NDS_MENU_VSRECORD_R_WIN, y, 3u);
    }
    for (slot = sMenuVsRecordNumSlot; slot < NDS_UI_KIT_SPRITE_SLOTS; slot++)
    {
        ndsUiKitHideSprite(slot);
    }
}

/* mnVSRecordInitVars (mnvsrecord.c:1924-1932), first-view subset: the
 * entry kind is BattleScore by construction here (no Ranking/Indiv yet),
 * so only the cursor and the fighter-mask snapshot carry over. */
static void ndsMenuShellVsRecordLoad(void)
{
    sMenuVsRecordCursor = 0u;
    sMenuVsRecordMask = (u32)gSCManagerBackupData.fighter_mask;
}

static void ndsMenuShellPopulateVsRecord(void)
{
    ndsMenuShellVsRecordRefresh();
}

static void ndsMenuShellUpdateVsRecord(u32 held, u32 taps)
{
    u32 page = sMenuVsRecordCursor / NDS_MENU_VSRECORD_ROWS_PER_PAGE;
    u32 offset = sMenuVsRecordCursor % NDS_MENU_VSRECORD_ROWS_PER_PAGE;

    if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
    {
        sMenuVsRecordCursor = (sMenuVsRecordCursor == 0u) ?
            (NDS_MENU_VSRECORD_FIGHTERS - 1u) : (sMenuVsRecordCursor - 1u);
        ndsMenuShellVsRecordRefresh();
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        return;
    }
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
    {
        sMenuVsRecordCursor = (sMenuVsRecordCursor + 1u) %
            NDS_MENU_VSRECORD_FIGHTERS;
        ndsMenuShellVsRecordRefresh();
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        return;
    }
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT) != FALSE)
    {
        page = (page == 0u) ?
            (NDS_MENU_VSRECORD_PAGES - 1u) : (page - 1u);
        sMenuVsRecordCursor =
            (page * NDS_MENU_VSRECORD_ROWS_PER_PAGE) + offset;
        ndsMenuShellVsRecordRefresh();
        ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        return;
    }
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_RIGHT) != FALSE)
    {
        page = (page + 1u) % NDS_MENU_VSRECORD_PAGES;
        sMenuVsRecordCursor =
            (page * NDS_MENU_VSRECORD_ROWS_PER_PAGE) + offset;
        ndsMenuShellVsRecordRefresh();
        ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        return;
    }
    /* A or START steps the stats kind forward in the source; only the
     * first view exists, so this is a refusal that stays put. */
    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        ndsUiKitSfx(NDS_UI_KIT_SFX_BACK);
        return;
    }
    /* B on the BattleScore kind returns to DATA. No cue: the kit carries
     * neither of the source's kind-step cues (BurnS/FoxFoot). */
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellGoto((u32)nSCKindData);
        return;
    }
}
