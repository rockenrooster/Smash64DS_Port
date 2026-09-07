/* --- Screen: Sound Test --------------------------------------------------
 *
 * Three rows in the source's own enum order (port include/mn/mndef.h,
 * MNSoundTestOptions): Music, Sound, Voice. UP and DOWN walk the three
 * with wraparound, L and R walk the row's id with wraparound, A plays
 * the row's id, X stops (the source's Z_TRIG; the DS has no Z and L/R
 * already walk ids), START fades the BGM out over 120 tics, B returns
 * to the DATA menu restoring the BGM volume. Transcribed from
 * decomp mn/mndata/mnsoundtest.c:
 *
 *   cursor   U from Music wraps to Voice, D from Voice wraps to Music
 *            (mnSoundTestUpdateControllerInputs U/D arms). Cue
 *            MenuScroll2, here NDS_UI_KIT_SFX_MOVE.
 *   id       L decrements with wrap, R increments with wrap, per row
 *            (the L/R arms). No cue on either, like the source. The
 *            row counts are the source tables' own lengths:
 *            dMNSoundTestMusicIDs 45, dMNSoundTestSoundIDs 194,
 *            dMNSoundTestVoiceIDs 244 (mnsoundtest.c:40-87, :90-286,
 *            :288-585).
 *   entry    the cursor always restarts on Music with every id at 0
 *            and no fade pending (mnSoundTestInitVars :1697-1714).
 *   A        Music stops all BGM (cancelling a fade first) and plays
 *            the row's id via syAudioPlayBGM; Sound/Voice stop via
 *            func_800266A0_272A0 then play the row's id via
 *            func_800269C0_275C0 (:954-977). The played audio is the
 *            feedback, so no kit cue here.
 *   Z        syAudioStopBGMAll + func_800266A0_272A0 (:978-982).
 *   START    syAudioSetBGMVolumeFade(0, 0, 120) plus func_800266A0,
 *            then the BGM stops 120 tics later (:983-988, :940-951).
 *   B        syAudioStopBGMAll + func_800266A0_272A0, volume back to
 *            0x7000, scene back to nSCKindData (:996-1005). The DATA
 *            screen's own tail replays the ModeSelect track on that
 *            return (ndsMenuShellRunData), so nothing starts here.
 *   volume   every frame without a pending fade reasserts
 *            syAudioSetBGMVolume(0, 0x7000) (:952), mirrored below.
 *
 * WHAT IT COMMITS is nothing: no save field, so a return trip always
 * restarts on Music with every id at 0.
 *
 * PRESENTATION. The source tints the cursor row HI (FF/A8/00) and the
 * rest NOT (7D/45/07) through mnSoundTestUpdateOptionColors (:737-766)
 * and draws each row's 1-based id in digit sprites beside it
 * (:1481-1523). The converted UI kit carries no Sound Test art yet, so
 * the rows are the kit's own font text in those two colours on the
 * shell's shared blue field, in the DATA cascade, with the 1-based id
 * beside each row through the kit number helper.
 *
 * TODO(SOUNDTEST-KIT): bake the Sound Test sprites and replace the
 * font rows -- llMNSoundTestSoundTestTextSprite (title),
 * llMNSoundTestMusicTextSprite, llMNSoundTestSoundTextSprite,
 * llMNSoundTestVoiceTextSprite (rows), llMNSoundTestCapsuleRightSprite,
 * the dMNSoundTestDigitSpriteOffsets digits, llMNCommonArrowLSprite /
 * llMNCommonArrowRSprite (row arrows), llMNSoundTestStartButtonSprite +
 * llIFCommonBattlePauseDecalAButtonSprite /
 * llIFCommonBattlePauseDecalBButtonSprite and
 * llMNSoundTestColonPlayTextSprite / ColonFadeOut / ColonExit (footer)
 * -- following the Option screen's row-surface shape
 * (nds_menu_shell_option.c).
 */

#ifndef NDS_MENU_SHELL_SCREEN_SOUNDTEST
#define NDS_MENU_SHELL_SCREEN_SOUNDTEST 10u
#endif

/* Source enum order (mndef.h MNSoundTestOptions): Music, Sound, Voice.
 * Local indices so this fragment needs no new include. */
#define NDS_MENU_SOUNDTEST_ROWS 3u
#define NDS_MENU_SOUNDTEST_MUSIC 0u
#define NDS_MENU_SOUNDTEST_SOUND 1u
#define NDS_MENU_SOUNDTEST_VOICE 2u

/* The source tables' own lengths (mnsoundtest.c:40-585). Hard bounds, so
 * the wrap below cannot walk off either end whatever the tables hold. */
#define NDS_MENU_SOUNDTEST_MUSIC_IDS 45u
#define NDS_MENU_SOUNDTEST_SOUND_IDS 194u
#define NDS_MENU_SOUNDTEST_VOICE_IDS 244u

/* The source's own option colours (mnSoundTestUpdateOptionColors):
 * HI FF/A8/00, NOT 7D/45/07. */
#define NDS_MENU_SOUNDTEST_RGB_HI 0x00ffa800u
#define NDS_MENU_SOUNDTEST_RGB_NOT 0x007d4507u

/* The DATA cascade this shell's row screens share, so the child reads
 * as the parent's page. */
#define NDS_MENU_SOUNDTEST_X0 96
#define NDS_MENU_SOUNDTEST_Y0 40
#define NDS_MENU_SOUNDTEST_DX (-18)
#define NDS_MENU_SOUNDTEST_DY 31
/* The 1-based id sits right of every row at one fixed right edge, so a
 * wider id grows leftward and the label never moves -- the kit number
 * helper's own contract. */
#define NDS_MENU_SOUNDTEST_NUM_RIGHT_X 200
/* Three digit slots a row; the largest id (244) needs three. Past the
 * VS screen's own number slots, which no row screen owns. */
#define NDS_MENU_SOUNDTEST_NUM_BASE 6u
#define NDS_MENU_SOUNDTEST_NUM_DIGITS 3u

/* The source's own id tables, owned by the imported Sound Test TU
 * (src/import/battleship_mnsoundtest.c), which compiles under the same
 * shell gate as this screen. Reused, not duplicated. */
extern u32 dMNSoundTestMusicIDs[];
extern u32 dMNSoundTestSoundIDs[];
extern u32 dMNSoundTestVoiceIDs[];

static u32 sMenuSoundCursor;
static s32 sMenuSoundIds[NDS_MENU_SOUNDTEST_ROWS];
/* The pending START fade, in tics; -1 is idle, like the source's
 * sMNSoundTestFadeOutWait. */
static s32 sMenuSoundFade;

static const char *kMenuSoundRowText[NDS_MENU_SOUNDTEST_ROWS] = {
    "MUSIC",
    "SOUND",
    "VOICE"
};

/* The row's id-table length, so L/R wraps at the source's own end. */
static u32 ndsMenuShellSoundTestRowCount(u32 row)
{
    if (row == NDS_MENU_SOUNDTEST_SOUND)
    {
        return NDS_MENU_SOUNDTEST_SOUND_IDS;
    }
    if (row == NDS_MENU_SOUNDTEST_VOICE)
    {
        return NDS_MENU_SOUNDTEST_VOICE_IDS;
    }
    return NDS_MENU_SOUNDTEST_MUSIC_IDS;
}

/* One full redraw. Text slots are content-keyed (nds_ui_kit.c), so a
 * call that changed nothing recomposes nothing; refresh runs on
 * populate and on cursor/id moves only. */
static void ndsMenuShellSoundTestRefresh(void)
{
    u32 row;

    ndsUiKitSetText(NDS_MENU_SLOT_HEADER, "SOUND TEST",
                    NDS_MENU_RGB_HEADER);
    ndsUiKitMoveText(NDS_MENU_SLOT_HEADER, NDS_MENU_HEADER_X,
                     NDS_MENU_HEADER_Y);
    for (row = 0u; row < NDS_MENU_SOUNDTEST_ROWS; row++)
    {
        u32 slot = NDS_MENU_SLOT_ROW0 + row;
        u32 rgb = (row == sMenuSoundCursor) ?
            NDS_MENU_SOUNDTEST_RGB_HI : NDS_MENU_SOUNDTEST_RGB_NOT;
        s32 x = NDS_MENU_SOUNDTEST_X0 + ((s32)row * NDS_MENU_SOUNDTEST_DX);
        s32 y = NDS_MENU_SOUNDTEST_Y0 + ((s32)row * NDS_MENU_SOUNDTEST_DY);
        u32 base = NDS_MENU_SOUNDTEST_NUM_BASE +
            (row * NDS_MENU_SOUNDTEST_NUM_DIGITS);
        u32 used;
        u32 i;

        ndsUiKitSetText(slot, kMenuSoundRowText[row], rgb);
        ndsUiKitMoveText(slot, x, y);
        /* The source shows the 1-based id (:1486: number = id + 1). */
        used = ndsUiKitSetNumber(base, NDS_MENU_SOUNDTEST_NUM_DIGITS,
                                 sMenuSoundIds[row] + 1,
                                 NDS_MENU_SOUNDTEST_NUM_RIGHT_X, y);
        for (i = used; i < NDS_MENU_SOUNDTEST_NUM_DIGITS; i++)
        {
            ndsUiKitHideSprite(base + i);
        }
    }
}

/* mnSoundTestInitVars (:1697-1714): cursor Music, every id 0, no fade.
 * No scene_prev dependence -- the source always restarts here. */
static void ndsMenuShellSoundTestLoad(void)
{
    u32 row;

    sMenuSoundCursor = NDS_MENU_SOUNDTEST_MUSIC;
    for (row = 0u; row < NDS_MENU_SOUNDTEST_ROWS; row++)
    {
        sMenuSoundIds[row] = 0;
    }
    sMenuSoundFade = -1;
}

static void ndsMenuShellPopulateSoundTest(void)
{
    ndsMenuShellSoundTestRefresh();
}

/* A plays the cursor row's id (:954-977). */
static void ndsMenuShellSoundTestPlay(void)
{
    if (sMenuSoundCursor == NDS_MENU_SOUNDTEST_MUSIC)
    {
        if (sMenuSoundFade > 0)
        {
            sMenuSoundFade = -1;
        }
        syAudioStopBGMAll();
        syAudioPlayBGM(0, (s32)dMNSoundTestMusicIDs[sMenuSoundIds[NDS_MENU_SOUNDTEST_MUSIC]]);
    }
    else if (sMenuSoundCursor == NDS_MENU_SOUNDTEST_SOUND)
    {
        func_800266A0_272A0();
        (void)func_800269C0_275C0((u16)dMNSoundTestSoundIDs[sMenuSoundIds[NDS_MENU_SOUNDTEST_SOUND]]);
    }
    else
    {
        func_800266A0_272A0();
        (void)func_800269C0_275C0((u16)dMNSoundTestVoiceIDs[sMenuSoundIds[NDS_MENU_SOUNDTEST_VOICE]]);
    }
}

static void ndsMenuShellUpdateSoundTest(u32 held, u32 taps)
{
    /* The START fade, ticked first so it runs while any input below
     * lands (:940-951). Idle frames reassert full volume (:952). */
    if (sMenuSoundFade != -1)
    {
        if (sMenuSoundFade != 0)
        {
            sMenuSoundFade--;
        }
        else
        {
            syAudioStopBGMAll();
            sMenuSoundFade = -1;
        }
    }
    else
    {
        syAudioSetBGMVolume(0, 0x7000u);
    }
    /* B leaves first, like the source's FuncRun: it stops the BGM,
     * restores the volume and returns to DATA (:996-1005). */
    if ((taps & NDS_INPUT_B) != 0u)
    {
        syAudioStopBGMAll();
        func_800266A0_272A0();
        syAudioSetBGMVolume(0, 0x7000u);
        ndsMenuShellGoto((u32)nSCKindData);
        return;
    }
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
    {
        sMenuSoundCursor = (sMenuSoundCursor == NDS_MENU_SOUNDTEST_MUSIC) ?
            (NDS_MENU_SOUNDTEST_ROWS - 1u) : (sMenuSoundCursor - 1u);
        ndsMenuShellSoundTestRefresh();
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        return;
    }
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
    {
        sMenuSoundCursor = (sMenuSoundCursor + 1u) % NDS_MENU_SOUNDTEST_ROWS;
        ndsMenuShellSoundTestRefresh();
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        return;
    }
    /* L/R walk the cursor row's id with wraparound, no cue. The
     * shell's Direction helper owns the repeat, the way the Option
     * screen's Sound row does. */
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT) != FALSE)
    {
        u32 count = ndsMenuShellSoundTestRowCount(sMenuSoundCursor);

        sMenuSoundIds[sMenuSoundCursor]--;
        if (sMenuSoundIds[sMenuSoundCursor] < 0)
        {
            sMenuSoundIds[sMenuSoundCursor] = (s32)(count - 1u);
        }
        ndsMenuShellSoundTestRefresh();
        return;
    }
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_RIGHT) != FALSE)
    {
        u32 count = ndsMenuShellSoundTestRowCount(sMenuSoundCursor);

        sMenuSoundIds[sMenuSoundCursor]++;
        if (sMenuSoundIds[sMenuSoundCursor] >= (s32)count)
        {
            sMenuSoundIds[sMenuSoundCursor] = 0;
        }
        ndsMenuShellSoundTestRefresh();
        return;
    }
    if ((taps & NDS_INPUT_A) != 0u)
    {
        ndsMenuShellSoundTestPlay();
        return;
    }
    /* The source's Z_TRIG stop (:978-982). The DS has no Z and L/R
     * already walk ids, so X is the transcription. */
    if ((taps & NDS_INPUT_X) != 0u)
    {
        syAudioStopBGMAll();
        func_800266A0_272A0();
        return;
    }
    /* START fades out over 120 tics (:983-988). */
    if ((taps & NDS_INPUT_START) != 0u)
    {
        syAudioSetBGMVolumeFade(0, 0u, 120u);
        sMenuSoundFade = 120;
        func_800266A0_272A0();
        return;
    }
}
