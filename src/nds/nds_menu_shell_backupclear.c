/* --- Screen: Backup Clear ------------------------------------------------
 *
 * Six targets in the source's own enum order (decomp mn/mndef.h):
 * Newcomers, 1PHighScore, BonusStageTime, VSRecord, Prize, AllDataClear.
 * UP and DOWN walk the six with wraparound, A or START on a target
 * opens the YES/NO confirm, B returns to Options. Transcribed from
 * decomp mn/mnoption/mnbackupclear.c:
 *
 *   cursor   UP from Newcomers wraps to AllDataClear, DOWN from
 *            AllDataClear wraps to Newcomers (main-menu U/D arms).
 *            Cue MenuScroll2.
 *   entry    the cursor always restarts on Newcomers, the confirm
 *            always restarts at NO, menuKind 0, UpdateWait 10
 *            (mnBackupClearInitVars :513-522).
 *   gate     the first ten tics eat every input; the eleventh opens
 *            the confirm at kind 1 defaulting to NO (:566-602).
 *   confirm  R moves NO -> YES, L moves YES -> NO, cue MenuScroll2.
 *            A on YES applies, A on NO cancels, B cancels from either
 *            side (:672-760). Cancel restores the six rows.
 *   double   AllDataClear needs a SECOND confirm (menuKind 2,
 *            :683-692); every other target applies from the first.
 *            The second confirm defaults to NO the same way.
 *   apply    mnBackupClearApplyOptionID (:525-557): one lbBackupClear*
 *            call per target (AllDataClear also lbBackupApplyOptions),
 *            then always lbBackupCorrectErrors + lbBackupWrite + the
 *            BackupClear FGM cue (269). The 60-tic flash runs, then the
 *            rows come back (:772-790).
 *   B        from the main menu returns to nSCKindOption immediately.
 *
 * WHAT IT COMMITS is the source's own apply path, called directly:
 * the lbBackupClear* bodies from src/import/battleship_lbbackup.c,
 * then lbBackupCorrectErrors, then lbBackupWrite. No copy, no model.
 * Tests run against those same bodies on test data; the live screen
 * never touches a test save.
 *
 * THE CURSOR IS THE SELECTED ROW'S BAKE. The source tints the cursor
 * row HI (FF/A8/00) and the rest NOT (7D/45/07) through
 * mnBackupClearUpdateOptionTabColors (:257-275), so each of the six
 * rows ships a HI twin. The confirm ships four bakes: kind 1/2 prompt
 * (AreYouSure vs IsOkay) by YES/NO selection, with the circle under
 * the selected answer and the blue frame around the dialog.
 */

#ifndef NDS_MENU_SHELL_SCREEN_BACKUPCLEAR
#define NDS_MENU_SHELL_SCREEN_BACKUPCLEAR 8u
#endif
#ifndef NDS_MENU_VS_SURFACE_NONE
#define NDS_MENU_VS_SURFACE_NONE 0xffffu
#endif

#define NDS_MENU_BACKUP_ROWS 6u
#define NDS_MENU_BACKUP_NEWCOMERS 0u
/* Row order is the source enum order (mndef.h): Newcomers, 1PHighScore,
 * BonusStageTime, VSRecord, Prize, AllDataClear. */
#define NDS_MENU_BACKUP_MENU_MAIN 0u
#define NDS_MENU_BACKUP_MENU_CONFIRM1 1u
#define NDS_MENU_BACKUP_MENU_CONFIRM2 2u
#define NDS_MENU_BACKUP_NO 1u
#define NDS_MENU_BACKUP_YES 0u
#define NDS_MENU_BACKUP_ENTRY_WAIT 10u
#define NDS_MENU_BACKUP_APPLY_TICS 60u

static u32 sMenuBackupCursor;
static u32 sMenuBackupMenuKind;
static u32 sMenuBackupYesOrNo;
static u32 sMenuBackupWait;
static u32 sMenuBackupApplyTics;
static u32 sMenuBackupAppliedConfirmKind;
/* What is currently ON SCREEN, so a frame that changed nothing blits
 * nothing and a change blits exactly the row that changed. Same
 * discipline as the VS Options screen. The confirm dialog has its own
 * cache entry: NONE while the six rows are up. */
static NdsUiKitSurfaceId sMenuBackupRowSurface[NDS_MENU_BACKUP_ROWS];
static NdsUiKitSurfaceId sMenuBackupConfirmSurface;

__attribute__((used)) volatile u32 gNdsMenuShellBackupBlitCount;
__attribute__((used)) volatile u32 gNdsMenuShellBackupApplyCount;
__attribute__((used)) volatile u32 gNdsMenuShellBackupApplyTarget;

static const NdsUiKitSurfaceId kNdsMenuBackupPlate[] = {
    NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR
};

static NdsUiKitSurfaceId ndsMenuShellBackupWantRow(u32 row)
{
    u32 hi = (row == sMenuBackupCursor) ? 1u : 0u;

    switch (row)
    {
    case 0u:
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_NEWCOMERS_HI :
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_NEWCOMERS;
    case 1u:
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_1P_HI :
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_1P;
    case 2u:
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_BONUS_HI :
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_BONUS;
    case 3u:
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_VS_HI :
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_VS;
    case 4u:
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_PRIZE_HI :
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_PRIZE;
    default:
        break;
    }
    return (hi != 0u) ?
        NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_ALL_DATA_HI :
        NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_ALL_DATA;
}

static NdsUiKitSurfaceId ndsMenuShellBackupWantConfirm(void)
{
    if (sMenuBackupApplyTics != 0u)
    {
        u32 flash = ((((sMenuBackupApplyTics + 9u) / 10u) & 1u) == 0u);
        if (sMenuBackupAppliedConfirmKind == NDS_MENU_BACKUP_MENU_CONFIRM2)
        {
            return flash ? NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM2_YES_FLASH :
                           NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM2_YES;
        }
        return flash ? NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM1_YES_FLASH :
                       NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM1_YES;
    }
    if (sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_CONFIRM2)
    {
        return (sMenuBackupYesOrNo == NDS_MENU_BACKUP_YES) ?
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM2_YES :
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM2_NO;
    }
    if (sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_CONFIRM1)
    {
        return (sMenuBackupYesOrNo == NDS_MENU_BACKUP_YES) ?
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM1_YES :
            NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM1_NO;
    }
    return NDS_MENU_VS_SURFACE_NONE;
}

static void ndsMenuShellBackupSyncRows(u32 budget)
{
    u32 row;

    for (row = 0u; (row < NDS_MENU_BACKUP_ROWS) && (budget != 0u); row++)
    {
        NdsUiKitSurfaceId want = ndsMenuShellBackupWantRow(row);

        if (want != sMenuBackupRowSurface[row])
        {
            if (ndsUiKitBlitSurfaces(&want, 1u) == FALSE)
            {
                return;
            }
            sMenuBackupRowSurface[row] = want;
            gNdsMenuShellBackupBlitCount++;
            budget--;
        }
    }
}

static void ndsMenuShellBackupSyncConfirm(void)
{
    NdsUiKitSurfaceId want = ndsMenuShellBackupWantConfirm();

    if (want != sMenuBackupConfirmSurface)
    {
        if (want != NDS_MENU_VS_SURFACE_NONE)
        {
            /* Source ejects all six option labels while the dialog is up. */
            if ((sMenuBackupConfirmSurface == NDS_MENU_VS_SURFACE_NONE) &&
                (ndsUiKitBlitSurfaces(kNdsMenuBackupPlate, 1u) == FALSE))
            {
                return;
            }
            if (ndsUiKitBlitSurfaces(&want, 1u) == FALSE)
            {
                return;
            }
            gNdsMenuShellBackupBlitCount++;
        }
        else
        {
            /* Cancel/apply exit: the dialog owned its box, so the rows
             * it covered must be repainted, not just unmarked. */
            u32 row;

            /* The dialog also covered the gaps between the text rows. */
            if (ndsUiKitBlitSurfaces(kNdsMenuBackupPlate, 1u) == FALSE)
            {
                return;
            }
            for (row = 0u; row < NDS_MENU_BACKUP_ROWS; row++)
            {
                sMenuBackupRowSurface[row] = NDS_MENU_VS_SURFACE_NONE;
            }
            ndsMenuShellBackupSyncRows(NDS_MENU_BACKUP_ROWS);
        }
        sMenuBackupConfirmSurface = want;
    }
}

/* mnBackupClearInitVars (:513-522): cursor Newcomers, main menu, NO,
 * ten-tic gate, no apply anim. */
static void ndsMenuShellBackupClearLoad(void)
{
    u32 row;

    sMenuBackupCursor = NDS_MENU_BACKUP_NEWCOMERS;
    sMenuBackupMenuKind = NDS_MENU_BACKUP_MENU_MAIN;
    sMenuBackupYesOrNo = NDS_MENU_BACKUP_NO;
    sMenuBackupWait = NDS_MENU_BACKUP_ENTRY_WAIT;
    sMenuBackupApplyTics = 0u;
    sMenuBackupAppliedConfirmKind = NDS_MENU_BACKUP_MENU_MAIN;
    sMenuBackupConfirmSurface = NDS_MENU_VS_SURFACE_NONE;
    for (row = 0u; row < NDS_MENU_BACKUP_ROWS; row++)
    {
        sMenuBackupRowSurface[row] = NDS_MENU_VS_SURFACE_NONE;
    }
}

static void ndsMenuShellPopulateBackupClear(void)
{
    (void)ndsUiKitBlitSurfaces(kNdsMenuBackupPlate, 1u);
    ndsMenuShellBackupSyncRows(NDS_MENU_BACKUP_ROWS);
}

/* mnBackupClearApplyOptionID (:525-557) exactly: one clear call per
 * target (AllData also ApplyOptions), then CorrectErrors + Write +
 * the BackupClear cue. */
static void ndsMenuShellBackupApply(u32 target)
{
    switch (target)
    {
    case 0u:
        lbBackupClearNewcomers();
        break;
    case 1u:
        lbBackupClear1PHighScore();
        break;
    case 2u:
        lbBackupClearBonusStageTime();
        break;
    case 3u:
        lbBackupClearVSRecord();
        break;
    case 4u:
        lbBackupClearPrize();
        break;
    default:
        lbBackupClearAllData();
        lbBackupApplyOptions();
        break;
    }
    lbBackupCorrectErrors();
    lbBackupWrite();
    (void)ndsAudioFgmPlay((u16)nSYAudioFGMOptionBackupClear);
    gNdsMenuShellBackupApplyCount++;
    gNdsMenuShellBackupApplyTarget = target;
}

static void ndsMenuShellBackupOpenConfirm(void)
{
    ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
    sMenuBackupMenuKind = NDS_MENU_BACKUP_MENU_CONFIRM1;
    sMenuBackupYesOrNo = NDS_MENU_BACKUP_NO;
    sMenuBackupWait = NDS_MENU_BACKUP_ENTRY_WAIT;
    ndsMenuShellBackupSyncConfirm();
}

static void ndsMenuShellBackupCancel(void)
{
    sMenuBackupMenuKind = NDS_MENU_BACKUP_MENU_MAIN;
    sMenuBackupWait = NDS_MENU_BACKUP_ENTRY_WAIT;
    ndsMenuShellBackupSyncConfirm();
}

static void ndsMenuShellUpdateBackupMain(u32 held, u32 taps)
{
    u32 moved = FALSE;

    /* Source gives confirmation/back priority over simultaneous movement. */
    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        ndsMenuShellBackupOpenConfirm();
        return;
    }
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellGoto((u32)nSCKindOption);
        return;
    }
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
    {
        sMenuBackupCursor = (sMenuBackupCursor == 0u) ?
            (NDS_MENU_BACKUP_ROWS - 1u) : (sMenuBackupCursor - 1u);
        moved = TRUE;
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
    {
        sMenuBackupCursor = (sMenuBackupCursor + 1u) % NDS_MENU_BACKUP_ROWS;
        moved = TRUE;
    }
    if (moved != FALSE)
    {
        ndsMenuShellBackupSyncRows(NDS_MENU_BACKUP_ROWS);
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        return;
    }
}

static void ndsMenuShellUpdateBackupConfirm(u32 held, u32 taps)
{
    u32 kind = sMenuBackupMenuKind;

    /* A/START on YES applies; on NO cancels. B cancels outright. */
    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        if (sMenuBackupYesOrNo == NDS_MENU_BACKUP_YES)
        {
            /* AllDataClear needs the second confirm; every other
             * target applies from the first (:683-692). */
            if ((kind == NDS_MENU_BACKUP_MENU_CONFIRM1) &&
                (sMenuBackupCursor == (NDS_MENU_BACKUP_ROWS - 1u)))
            {
                ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
                sMenuBackupMenuKind = NDS_MENU_BACKUP_MENU_CONFIRM2;
                sMenuBackupYesOrNo = NDS_MENU_BACKUP_NO;
                sMenuBackupWait = NDS_MENU_BACKUP_ENTRY_WAIT;
                ndsMenuShellBackupSyncConfirm();
                return;
            }
            ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
            ndsMenuShellBackupApply(sMenuBackupCursor);
            sMenuBackupAppliedConfirmKind = kind;
            sMenuBackupMenuKind = NDS_MENU_BACKUP_MENU_MAIN;
            sMenuBackupApplyTics = NDS_MENU_BACKUP_APPLY_TICS;
            ndsMenuShellBackupSyncConfirm();
            return;
        }
        ndsMenuShellBackupCancel();
        return;
    }
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellBackupCancel();
        return;
    }
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_RIGHT) != FALSE)
    {
        if (sMenuBackupYesOrNo != NDS_MENU_BACKUP_YES)
        {
            sMenuBackupYesOrNo = NDS_MENU_BACKUP_YES;
            ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
            ndsMenuShellBackupSyncConfirm();
        }
        return;
    }
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT) != FALSE)
    {
        if (sMenuBackupYesOrNo != NDS_MENU_BACKUP_NO)
        {
            sMenuBackupYesOrNo = NDS_MENU_BACKUP_NO;
            ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
            ndsMenuShellBackupSyncConfirm();
        }
        return;
    }
}

static void ndsMenuShellUpdateBackupClear(u32 held, u32 taps)
{
    /* Entry gate and post-apply pause eat every input, like the
     * source's UpdateWait and confirm-anim windows. */
    if (sMenuBackupApplyTics != 0u)
    {
        sMenuBackupApplyTics--;
        ndsMenuShellBackupSyncConfirm();
        return;
    }
    if (sMenuBackupWait != 0u)
    {
        sMenuBackupWait--;
        ndsMenuShellBackupSyncConfirm();
        return;
    }
    if (sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_MAIN)
    {
        ndsMenuShellBackupSyncConfirm();
        ndsMenuShellBackupSyncRows(1u);
        ndsMenuShellUpdateBackupMain(held, taps);
    }
    else
    {
        ndsMenuShellBackupSyncConfirm();
        ndsMenuShellUpdateBackupConfirm(held, taps);
    }
}
