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
 * PRESENTATION. The baked DATA plate (collage, decal papers, Data icon,
 * smash logo, DATA label) plus one baked surface per row in the source's
 * tab HIGHLIGHT / NOT pairs, in whichever of the source's two row layouts
 * the Sound Test unlock bit selects (generate_mn_ui_kit.py DATA_*). The
 * MAIN engine carries no text slab, which is why the former font rows drew
 * nothing and the screen was a bare blue field.
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

#ifndef NDS_MENU_VS_SURFACE_NONE
#define NDS_MENU_VS_SURFACE_NONE 0xffffu
#endif

static u32 sMenuDataCursor;
static u32 sMenuDataHaveSoundTest;
/* What is currently ON SCREEN, so a still screen blits nothing and a cursor
 * move blits exactly the two rows that changed (Option-screen discipline). */
static NdsUiKitSurfaceId sMenuDataRowSurface[NDS_MENU_DATA_ROWS];

__attribute__((used)) volatile u32 gNdsMenuShellDataBlitCount;

static const NdsUiKitSurfaceId kNdsMenuDataPlate[] = {
    NDS_MN_UI_KIT_SURFACE_DATA
};

/* The last row the cursor may rest on: the SoundTest row when unlocked,
 * the VSRecord row otherwise (mnDataInitVars :602-613). */
static u32 ndsMenuShellDataLast(void)
{
    return (sMenuDataHaveSoundTest != FALSE) ?
        NDS_MENU_DATA_SOUND_TEST : NDS_MENU_DATA_VS_RECORD;
}

/* The source has two row layouts, picked by the Sound Test unlock bit
 * (mnDataMakeCharacters :223, mnDataMakeVSRecord :263); each is baked in the
 * tab HIGHLIGHT and NOT pairs (mnDataSetOptionSpriteColors :131-177). */
static NdsUiKitSurfaceId ndsMenuShellDataWantSurface(u32 row)
{
    u32 hi = (row == sMenuDataCursor) ? 1u : 0u;

    if (sMenuDataHaveSoundTest == FALSE)
    {
        if (row == NDS_MENU_DATA_CHARACTERS)
        {
            return (hi != 0u) ?
                NDS_MN_UI_KIT_SURFACE_DATA_CHARACTERS_NOSOUND_HI :
                NDS_MN_UI_KIT_SURFACE_DATA_CHARACTERS_NOSOUND;
        }
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_DATA_VS_RECORD_NOSOUND_HI :
            NDS_MN_UI_KIT_SURFACE_DATA_VS_RECORD_NOSOUND;
    }
    switch (row)
    {
    case NDS_MENU_DATA_CHARACTERS:
        return (hi != 0u) ? NDS_MN_UI_KIT_SURFACE_DATA_CHARACTERS_HI :
                            NDS_MN_UI_KIT_SURFACE_DATA_CHARACTERS;
    case NDS_MENU_DATA_VS_RECORD:
        return (hi != 0u) ? NDS_MN_UI_KIT_SURFACE_DATA_VS_RECORD_HI :
                            NDS_MN_UI_KIT_SURFACE_DATA_VS_RECORD;
    default:
        break;
    }
    return (hi != 0u) ? NDS_MN_UI_KIT_SURFACE_DATA_SOUND_TEST_HI :
                        NDS_MN_UI_KIT_SURFACE_DATA_SOUND_TEST;
}

static void ndsMenuShellDataRefresh(void)
{
    u32 row;

    for (row = 0u; row <= ndsMenuShellDataLast(); row++)
    {
        NdsUiKitSurfaceId want = ndsMenuShellDataWantSurface(row);

        if (want != sMenuDataRowSurface[row])
        {
            if (ndsUiKitBlitSurfaces(&want, 1u) == FALSE)
            {
                return;
            }
            sMenuDataRowSurface[row] = want;
            gNdsMenuShellDataBlitCount++;
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
    {
        u32 row;

        for (row = 0u; row < NDS_MENU_DATA_ROWS; row++)
        {
            sMenuDataRowSurface[row] = NDS_MENU_VS_SURFACE_NONE;
        }
    }
}

static void ndsMenuShellPopulateData(void)
{
    (void)ndsUiKitBlitSurfaces(kNdsMenuDataPlate, 1u);
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
        if (ndsSceneManagerFind(want_kind) != NULL)
        {
            ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
            ndsMenuShellGoto(want_kind);
            return;
        }
        ndsUiKitSfx(NDS_UI_KIT_SFX_BACK);
        gNdsMenuShellDeniedCount++;
    }
    /* B returns to the main menu. No cue, like the source. */
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellGoto((u32)nSCKindModeSelect);
        return;
    }
}
