/* --- Screen: Character Data ---------------------------------------------
 *
 * Native transcription of mn/mndata/mncharacters.c's DATA-entry path.
 * The source has twelve pages in presentation order
 *
 *   Mario, Luigi, Donkey, Link, Samus, Yoshi, Kirby, Fox,
 *   Pikachu, Purin, Captain, Ness
 *
 * and LEFT/RIGHT wraps while skipping only the four unlockable fighters that
 * are absent from gSCManagerBackupData.fighter_mask. B stores the current
 * fighter in backup.characters_fkind and returns to DATA. A/START do nothing.
 *
 * Static presentation comes from source sprites baked by
 * generate_mn_ui_kit.py. The live fighter uses the already-native PlayersVS
 * preview owner: it gives this screen source fighter objects, native GX output
 * and the established 30 Hz retained-3D cadence without reviving an N64 scene
 * renderer. Build-disabled preview owners fail closed to the complete 2D
 * profile page rather than changing the source's page/unlock rules.
 */

#ifndef NDS_MENU_SHELL_SCREEN_CHARACTERS
#define NDS_MENU_SHELL_SCREEN_CHARACTERS 12u
#endif

#define NDS_MENU_CHARACTERS_PAGES 12u

static u32 sMenuCharactersPage;
static u32 sMenuCharactersFighterMask;

static const s32 kMenuCharactersKinds[NDS_MENU_CHARACTERS_PAGES] = {
    nFTKindMario,
    nFTKindLuigi,
    nFTKindDonkey,
    nFTKindLink,
    nFTKindSamus,
    nFTKindYoshi,
    nFTKindKirby,
    nFTKindFox,
    nFTKindPikachu,
    nFTKindPurin,
    nFTKindCaptain,
    nFTKindNess
};

/* Enumerate every generated page explicitly. Besides avoiding an assumption
 * about generated-id contiguity, this makes the source->kit coverage checker
 * see the complete runtime-reachable page inventory. Indexed by fighter kind. */
static const NdsUiKitSurfaceId kNdsMenuCharactersSurfaces[] = {
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_MARIO,
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_FOX,
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_DONKEY,
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_SAMUS,
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_LUIGI,
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_LINK,
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_YOSHI,
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_CAPTAIN,
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_KIRBY,
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_PIKACHU,
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_PURIN,
    NDS_MN_UI_KIT_SURFACE_CHARACTERS_NESS
};

static s32 ndsMenuShellCharactersKind(void)
{
    return kMenuCharactersKinds[sMenuCharactersPage];
}

/* mnCharactersCheckHaveFighterKind (:2328-2345) exactly: only the four
 * unlockables consult fighter_mask; the eight starters are always pages. */
static u32 ndsMenuShellCharactersHaveKind(s32 fkind)
{
    if ((fkind == nFTKindLuigi) || (fkind == nFTKindCaptain) ||
        (fkind == nFTKindPurin) || (fkind == nFTKindNess))
    {
        return ((sMenuCharactersFighterMask & LBBACKUP_MASK_FIGHTER(fkind)) !=
                0u) ? TRUE : FALSE;
    }
    return TRUE;
}

/* mnCharactersGetPage (:1366-1376), expressed against the source page table
 * so an invalid/stale save falls back to Mario rather than indexing outside
 * the native surface block. */
static u32 ndsMenuShellCharactersPageForKind(s32 fkind)
{
    u32 page;

    for (page = 0u; page < NDS_MENU_CHARACTERS_PAGES; page++)
    {
        if (kMenuCharactersKinds[page] == fkind)
        {
            return page;
        }
    }
    return 0u;
}

static void ndsMenuShellCharactersSyncPreview(void)
{
    static const u8 teams[NDS_MENU_SHELL_PLAYERS] = { 0u, 0u, 0u, 0u };
    u32 slot;

    ndsMNPlayersVSPreviewSyncRules(FALSE, teams, NDS_MENU_SHELL_PLAYERS);
    ndsMNPlayersVSPreviewSync(0u, nFTPlayerKindMan,
                              ndsMenuShellCharactersKind(), TRUE);
    for (slot = 1u; slot < NDS_MENU_SHELL_PLAYERS; slot++)
    {
        ndsMNPlayersVSPreviewSync(slot, nFTPlayerKindNot, nFTKindNull, FALSE);
    }
}

static void ndsMenuShellCharactersLoad(void)
{
    sMenuCharactersFighterMask = (u32)gSCManagerBackupData.fighter_mask;
    sMenuCharactersPage =
        ndsMenuShellCharactersPageForKind(gSCManagerBackupData.characters_fkind);
    if (ndsMenuShellCharactersHaveKind(ndsMenuShellCharactersKind()) == FALSE)
    {
        sMenuCharactersPage = 0u;
    }
}

static void ndsMenuShellPopulateCharacters(void)
{
    NdsUiKitSurfaceId surface =
        kNdsMenuCharactersSurfaces[(u32)ndsMenuShellCharactersKind()];

    (void)ndsUiKitBlitSurfaces(&surface, 1u);
}

static void ndsMenuShellCharactersMove(s32 delta)
{
    do
    {
        if (delta < 0)
        {
            sMenuCharactersPage = (sMenuCharactersPage == 0u) ?
                (NDS_MENU_CHARACTERS_PAGES - 1u) :
                (sMenuCharactersPage - 1u);
        }
        else
        {
            sMenuCharactersPage =
                (sMenuCharactersPage + 1u) % NDS_MENU_CHARACTERS_PAGES;
        }
    }
    while (ndsMenuShellCharactersHaveKind(ndsMenuShellCharactersKind()) ==
           FALSE);

    ndsMenuShellPopulateCharacters();
}

static void ndsMenuShellUpdateCharacters(u32 held, u32 taps)
{
    if (ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT | NDS_INPUT_L) != FALSE)
    {
        ndsMenuShellCharactersMove(-1);
    }
    else if (ndsMenuShellDirection(held, taps,
                                   NDS_INPUT_RIGHT | NDS_INPUT_R) != FALSE)
    {
        ndsMenuShellCharactersMove(1);
    }
    else if ((taps & NDS_INPUT_B) != 0u)
    {
        /* mnCharactersBackupFighterKind (:2383-2388) then the B arm at
         * :2469-2477. */
        gSCManagerBackupData.characters_fkind =
            (u8)ndsMenuShellCharactersKind();
        lbBackupWrite();
        ndsMenuShellGoto((u32)nSCKindData);
        return;
    }

    /* Re-issue the desired kind while the preview owner's bounded residency
     * state converges. Once resident these calls are cheap state comparisons. */
    ndsMenuShellCharactersSyncPreview();
    ndsMNPlayersVSPreviewFrame();
}
