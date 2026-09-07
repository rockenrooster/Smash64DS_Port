/* --- Screen: Data ----------------------------------------------------------
 *
 * Three rows in the source's own enum order (port include/mn/mndef.h,
 * MNDataOptions): Characters, VSRecord, SoundTest. UP and DOWN walk the
 * available rows with wraparound, A or START on a row opens its scene, B
 * returns to the main menu. Transcribed from
 * decomp mn/mndata/mndata.c:
 *
 *   cursor   UP from Characters wraps to the last available option, DOWN
 *            from the last wraps to Characters (mnDataFuncRun U/D arms).
 *            Cue MenuScroll2.
 *   entry    the cursor starts on VSRecord when the previous scene was
 *            nSCKindVSRecord, on SoundTest when it was nSCKindSoundTest,
 *            and on Characters otherwise (mnDataInitVars :586-601).
 *   gate     SoundTest exists only while LBBACKUP_UNLOCK_MASK_SOUNDTEST
 *            is set in gSCManagerBackupData.unlock_mask; without it the
 *            last available option is VSRecord and no SoundTest row is
 *            built at all (:602-613, :802-805). Hidden, not greyed.
 *   A/START  on Characters / VSRecord / SoundTest cues MenuSelect, stops
 *            all BGM and routes to nSCKindCharacters / nSCKindVSRecord /
 *            nSCKindSoundTest with IsProceed set (:669-702).
 *   B        returns to nSCKindModeSelect, no cue, no save write
 *            (:704-713).
 *   BGM      FuncStart replays the ModeSelect track when returning from
 *            one of its own children (:808-816); see ndsMenuShellRunData.
 *   idle     five silent minutes return to Title (:633-643); attract
 *            behaviour owned by P2-7, deliberately absent like the Option
 *            screen's, not stubbed.
 *
 * WHAT IT COMMITS is nothing: this screen writes no save field, so a
 * return trip restores the cursor from scene_prev alone.
 *
 * PRESENTATION. The source marks the cursor row with the tab HIGHLIGHT
 * pair (ENV 82/00/28 PRIM FF/00/28) and the rest NOT (ENV 00/00/00 PRIM
 * 82/82/AA) (mnDataSetOptionSpriteColors :131-177). The converted UI kit
 * carries no DATA art yet, so the rows are the kit's own font text in
 * those two primitive colours on the shell's shared blue field, in the
 * VS-menu cascade the other row screens use.
 *
 * TODO(DATA-KIT): bake the DATA sprites and replace the font rows --
 * llMNDataDataTextSprite (header), llMNDataCharactersTextSprite,
 * llMNDataVSRecordTextSprite, llMNDataSoundTestTextSprite (rows),
 * llMNDataDataIconDarkSprite (entry icon),
 * llMNCommonSmashBrosCollageSprite + llMNCommonDecalPaperSprite
 * (backdrop), llMNCommonSmashLogoSprite (emblem) and the
 * llMNCommonOptionTabLeft/Middle/RightSprite tabs -- following the
 * Option screen's row-surface shape (nds_menu_shell_option.c).
 */

#ifndef NDS_MENU_SHELL_SCREEN_DATA
#define NDS_MENU_SHELL_SCREEN_DATA 9u
#endif

/* Source enum order (mndef.h MNDataOptions): Characters, VSRecord,
 * SoundTest. Local indices so this fragment needs no new include. */
#define NDS_MENU_DATA_ROWS 3u
#define NDS_MENU_DATA_CHARACTERS 0u
#define NDS_MENU_DATA_VS_RECORD 1u
#define NDS_MENU_DATA_SOUND_TEST 2u

/* The source's own tab primitive colours (mnDataSetOptionSpriteColors):
 * HI PRIM FF/00/28, NOT PRIM 82/82/AA. */
#define NDS_MENU_DATA_RGB_HI 0x00ff0028u
#define NDS_MENU_DATA_RGB_NOT 0x008282aau

/* The VS-menu cascade this shell's row screens share (core NDS_MENU_VS_*):
 * descending to the left in the source's entry order. */
#define NDS_MENU_DATA_X0 96
#define NDS_MENU_DATA_Y0 40
#define NDS_MENU_DATA_DX (-18)
#define NDS_MENU_DATA_DY 31

static u32 sMenuDataCursor;
static u32 sMenuDataHaveSoundTest;

static const char *kMenuDataRowText[NDS_MENU_DATA_ROWS] = {
    "CHARACTERS",
    "VS RECORD",
    "SOUND TEST"
};

/* The last row the cursor may rest on: the SoundTest row when unlocked,
 * the VSRecord row otherwise (mnDataInitVars :602-613). */
static u32 ndsMenuShellDataLast(void)
{
    return (sMenuDataHaveSoundTest != FALSE) ?
        NDS_MENU_DATA_SOUND_TEST : NDS_MENU_DATA_VS_RECORD;
}

/* One full redraw. Text slots are content-keyed (nds_ui_kit.c), so a call
 * that changed nothing recomposes nothing; refresh runs on populate and
 * on cursor moves only. */
static void ndsMenuShellDataRefresh(void)
{
    u32 row;

    ndsUiKitSetText(NDS_MENU_SLOT_HEADER, "DATA", NDS_MENU_RGB_HEADER);
    ndsUiKitMoveText(NDS_MENU_SLOT_HEADER, NDS_MENU_HEADER_X,
                     NDS_MENU_HEADER_Y);
    for (row = 0u; row < NDS_MENU_DATA_ROWS; row++)
    {
        u32 slot = NDS_MENU_SLOT_ROW0 + row;

        if ((row == NDS_MENU_DATA_SOUND_TEST) &&
            (sMenuDataHaveSoundTest == FALSE))
        {
            ndsUiKitHideText(slot);
        }
        else
        {
            u32 rgb = (row == sMenuDataCursor) ?
                NDS_MENU_DATA_RGB_HI : NDS_MENU_DATA_RGB_NOT;

            ndsUiKitSetText(slot, kMenuDataRowText[row], rgb);
            ndsUiKitMoveText(slot,
                             NDS_MENU_DATA_X0 + ((s32)row * NDS_MENU_DATA_DX),
                             NDS_MENU_DATA_Y0 + ((s32)row * NDS_MENU_DATA_DY));
        }
    }
}

/* mnDataInitVars (:586-618): cursor from scene_prev, range from the
 * SoundTest unlock bit. The clamp keeps the cursor on a drawn row if the
 * save lost the bit between visits; the source cannot produce that state
 * because the row is unreachable while locked. */
static void ndsMenuShellDataLoad(void)
{
    if ((u8)gSCManagerSceneData.scene_prev == (u8)nSCKindVSRecord)
    {
        sMenuDataCursor = NDS_MENU_DATA_VS_RECORD;
    }
    else if ((u8)gSCManagerSceneData.scene_prev == (u8)nSCKindSoundTest)
    {
        sMenuDataCursor = NDS_MENU_DATA_SOUND_TEST;
    }
    else
    {
        sMenuDataCursor = NDS_MENU_DATA_CHARACTERS;
    }
    sMenuDataHaveSoundTest =
        ((gSCManagerBackupData.unlock_mask &
          LBBACKUP_UNLOCK_MASK_SOUNDTEST) != 0u) ? TRUE : FALSE;
    if ((sMenuDataHaveSoundTest == FALSE) &&
        (sMenuDataCursor == NDS_MENU_DATA_SOUND_TEST))
    {
        sMenuDataCursor = NDS_MENU_DATA_CHARACTERS;
    }
}

static void ndsMenuShellPopulateData(void)
{
    ndsMenuShellDataRefresh();
}

static void ndsMenuShellUpdateData(u32 held, u32 taps)
{
    u32 last = ndsMenuShellDataLast();

    if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
    {
        sMenuDataCursor = (sMenuDataCursor == NDS_MENU_DATA_CHARACTERS) ?
            last : (sMenuDataCursor - 1u);
        ndsMenuShellDataRefresh();
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        return;
    }
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
    {
        sMenuDataCursor = (sMenuDataCursor == last) ?
            NDS_MENU_DATA_CHARACTERS : (sMenuDataCursor + 1u);
        ndsMenuShellDataRefresh();
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        return;
    }
    /* A or START opens the row's scene through the same request path the
     * Option screen uses for its children. */
    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        u32 want_kind = (u32)nSCKindCharacters;

        if (sMenuDataCursor == NDS_MENU_DATA_VS_RECORD)
        {
            want_kind = (u32)nSCKindVSRecord;
        }
        else if (sMenuDataCursor == NDS_MENU_DATA_SOUND_TEST)
        {
            want_kind = (u32)nSCKindSoundTest;
        }
        ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
        ndsMenuShellGoto(want_kind);
        return;
    }
    /* B returns to the main menu. No cue, like the source. */
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellGoto((u32)nSCKindModeSelect);
        return;
    }
}
