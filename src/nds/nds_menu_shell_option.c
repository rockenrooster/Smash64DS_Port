/* --- Screen: Options -----------------------------------------------------
 *
 * Three rows in the source's own enum order (decomp mn/mndef.h):
 * Sound, ScreenAdjust, BackupClear. UP and DOWN walk the three with
 * wraparound, A or START on a sub-screen row opens it, B commits and
 * leaves. Transcribed from decomp mn/mnoption/mnoption.c:
 *
 *   cursor   UP from Sound wraps to BackupClear, DOWN from BackupClear
 *            wraps to Sound (mnOptionFuncRun U/D arms). Cue MenuScroll2.
 *   entry    the cursor starts on ScreenAdjust when the previous scene
 *            was nSCKindScreenAdjust, on BackupClear when it was
 *            nSCKindBackupClear, and on Sound otherwise
 *            (mnOptionInitVars :790-815).
 *   sound    the Sound row owns the mono/stereo toggle. L sets stereo,
 *            R sets mono, A flips either way; the cue is MenuScroll1 in
 *            all three cases and syAudioSetQuality follows every change.
 *            Toggling never writes the save (mnOptionFuncRun sound arms).
 *   A/START  on ScreenAdjust or BackupClear writes the save
 *            (mnOptionWriteBackup :818-824: screenflash + mono/stereo,
 *            then lbBackupWrite), cues MenuSelect, and routes to that
 *            sub-scene with IsProceed set. A/START on Sound only toggles.
 *   B        writes the save and returns to nSCKindModeSelect, no cue.
 *   idle     five silent minutes return to Title with one save write;
 *            attract behaviour owned by P2-7, not transcribed here.
 *
 * WHAT IT COMMITS is mnOptionWriteBackup exactly: is_allow_screenflash
 * (preserved, never toggled on this screen) and sound_mono_or_stereo
 * into gSCManagerBackupData, then lbBackupWrite. The sound value opens
 * from dSYAudioSoundQuality and the flash value from the save, so a
 * return trip shows what the last visit left (mnOptionInitVars).
 *
 * THE CURSOR IS THE SELECTED ROW'S BAKE. The source marks it with the
 * tab HIGHLIGHT pair (mnOptionSetOptionSpriteColors :139: ENV 82/00/28
 * PRIM FF/00/28) while the others sit in NOT (ENV 00/00/00 PRIM
 * 82/82/AA), so each row ships a HI twin. The Sound row's bake follows
 * its value too: stereo-white/mono-dim vs mono-white/stereo-dim
 * (mnOptionSetSoundToggleSpriteColors), with the slash always dim.
 *
 * Owner scope: only BackupClear must function among the three Options
 * buttons. Sound keeps its source toggle (cheap, same screen); the
 * ScreenAdjust target screen itself is out of scope, so A on that row
 * routes to the imported source scene after the source's own write.
 */

#ifndef NDS_MENU_SHELL_SCREEN_OPTION
#define NDS_MENU_SHELL_SCREEN_OPTION 7u
#endif

#ifndef NDS_MENU_VS_SURFACE_NONE
#define NDS_MENU_VS_SURFACE_NONE 0xffffu
#endif

#define NDS_MENU_OPTION_ROWS 3u
#define NDS_MENU_OPTION_SOUND 0u
#define NDS_MENU_OPTION_SCREEN_ADJUST 1u
#define NDS_MENU_OPTION_BACKUP_CLEAR 2u

static u32 sMenuOptionCursor;
static u8 sMenuOptionSound;
/* Preserved, never toggled here; the source carries it through the write. */
static u8 sMenuOptionFlash;
/* What is currently ON SCREEN, so a frame that changed nothing blits
 * nothing and a change blits exactly the row that changed. Same
 * discipline as the VS Options screen's row surfaces. */
static NdsUiKitSurfaceId sMenuOptionRowSurface[NDS_MENU_OPTION_ROWS];

__attribute__((used)) volatile u32 gNdsMenuShellOptionBlitCount;
__attribute__((used)) volatile u32 gNdsMenuShellOptionCommitCount;

static const NdsUiKitSurfaceId kNdsMenuOptionPlate[] = {
    NDS_MN_UI_KIT_SURFACE_OPTION
};

/* Which surface each row SHOULD be showing, given its value and cursor. */
static NdsUiKitSurfaceId ndsMenuShellOptionWantSurface(u32 row)
{
    u32 hi = (row == sMenuOptionCursor) ? 1u : 0u;

    switch (row)
    {
    case NDS_MENU_OPTION_SOUND:
        if (sMenuOptionSound != 0u)
        {
            return (hi != 0u) ?
                NDS_MN_UI_KIT_SURFACE_OPTION_SOUND_STEREO_HI :
                NDS_MN_UI_KIT_SURFACE_OPTION_SOUND_STEREO;
        }
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_OPTION_SOUND_MONO_HI :
            NDS_MN_UI_KIT_SURFACE_OPTION_SOUND_MONO;
    case NDS_MENU_OPTION_SCREEN_ADJUST:
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_OPTION_SCREEN_ADJUST_HI :
            NDS_MN_UI_KIT_SURFACE_OPTION_SCREEN_ADJUST;
    default:
        break;
    }
    return (hi != 0u) ?
        NDS_MN_UI_KIT_SURFACE_OPTION_BACKUP_CLEAR_HI :
        NDS_MN_UI_KIT_SURFACE_OPTION_BACKUP_CLEAR;
}

/* At most `budget` rows a frame, so a screen holding still compares three
 * bytes and returns. Same discipline as VS Options. */
static void ndsMenuShellOptionSyncRows(u32 budget)
{
    u32 row;

    for (row = 0u; (row < NDS_MENU_OPTION_ROWS) && (budget != 0u); row++)
    {
        NdsUiKitSurfaceId want = ndsMenuShellOptionWantSurface(row);

        if (want != sMenuOptionRowSurface[row])
        {
            if (ndsUiKitBlitSurfaces(&want, 1u) == FALSE)
            {
                return;
            }
            sMenuOptionRowSurface[row] = want;
            gNdsMenuShellOptionBlitCount++;
            budget--;
        }
    }
}

/* mnOptionInitVars (:790-815): cursor from scene_prev, sound from the
 * mixer quality, flash preserved from the save. */
static void ndsMenuShellOptionLoad(void)
{
    u32 row;

    if ((u8)gSCManagerSceneData.scene_prev == (u8)nSCKindScreenAdjust)
    {
        sMenuOptionCursor = NDS_MENU_OPTION_SCREEN_ADJUST;
    }
    else if ((u8)gSCManagerSceneData.scene_prev == (u8)nSCKindBackupClear)
    {
        sMenuOptionCursor = NDS_MENU_OPTION_BACKUP_CLEAR;
    }
    else
    {
        sMenuOptionCursor = NDS_MENU_OPTION_SOUND;
    }
    sMenuOptionSound = (dSYAudioSoundQuality == 1) ? 1u : 0u;
    sMenuOptionFlash =
        (gSCManagerBackupData.is_allow_screenflash != FALSE) ? 1u : 0u;
    for (row = 0u; row < NDS_MENU_OPTION_ROWS; row++)
    {
        sMenuOptionRowSurface[row] = NDS_MENU_VS_SURFACE_NONE;
    }
}

/* mnOptionWriteBackup (:818-824) exactly. */
static void ndsMenuShellOptionWriteBackup(void)
{
    gSCManagerBackupData.is_allow_screenflash = (s32)sMenuOptionFlash;
    gSCManagerBackupData.sound_mono_or_stereo = (s32)sMenuOptionSound;
    lbBackupWrite();
    gNdsMenuShellOptionCommitCount++;
}

static void ndsMenuShellPopulateOption(void)
{
    (void)ndsUiKitBlitSurfaces(kNdsMenuOptionPlate, 1u);
    ndsMenuShellOptionSyncRows(NDS_MENU_OPTION_ROWS);
}

/* Sound-row value edit. L sets stereo, R sets mono, A flips; cue and
 * mixer follow only on a real change (A always changes). */
static void ndsMenuShellOptionSoundLeft(void)
{
    if (sMenuOptionSound == 0u)
    {
        sMenuOptionSound = 1u;
        ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        syAudioSetQuality((s32)sMenuOptionSound);
    }
}

static void ndsMenuShellOptionSoundRight(void)
{
    if (sMenuOptionSound != 0u)
    {
        sMenuOptionSound = 0u;
        ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        syAudioSetQuality((s32)sMenuOptionSound);
    }
}

static void ndsMenuShellOptionSoundFlip(void)
{
    sMenuOptionSound = (sMenuOptionSound != 0u) ? 0u : 1u;
    ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
    syAudioSetQuality((s32)sMenuOptionSound);
}

static void ndsMenuShellUpdateOption(u32 held, u32 taps)
{
    u32 moved = FALSE;

    ndsMenuShellOptionSyncRows(1u);

    if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
    {
        sMenuOptionCursor = (sMenuOptionCursor == NDS_MENU_OPTION_SOUND) ?
            (NDS_MENU_OPTION_ROWS - 1u) : (sMenuOptionCursor - 1u);
        moved = TRUE;
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
    {
        sMenuOptionCursor = (sMenuOptionCursor + 1u) % NDS_MENU_OPTION_ROWS;
        moved = TRUE;
    }
    if (moved != FALSE)
    {
        ndsMenuShellOptionSyncRows(NDS_MENU_OPTION_ROWS);
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        return;
    }
    /* Sound-row value keys. The source reads L/R/TRIG/C-buttons plus
     * stick LR here; the shell's Direction helper owns repeat, so the
     * DS pad LEFT/RIGHT are the transcription. */
    if ((sMenuOptionCursor == NDS_MENU_OPTION_SOUND) &&
        (ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT) != FALSE))
    {
        ndsMenuShellOptionSoundLeft();
        ndsMenuShellOptionSyncRows(NDS_MENU_OPTION_ROWS);
        return;
    }
    if ((sMenuOptionCursor == NDS_MENU_OPTION_SOUND) &&
        (ndsMenuShellDirection(held, taps, NDS_INPUT_RIGHT) != FALSE))
    {
        ndsMenuShellOptionSoundRight();
        ndsMenuShellOptionSyncRows(NDS_MENU_OPTION_ROWS);
        return;
    }
    /* A or START on a sub-screen row writes and opens it. A on Sound
     * flips the toggle instead. */
    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        if (sMenuOptionCursor == NDS_MENU_OPTION_SOUND)
        {
            ndsMenuShellOptionSoundFlip();
            ndsMenuShellOptionSyncRows(NDS_MENU_OPTION_ROWS);
            return;
        }
        if (sMenuOptionCursor == NDS_MENU_OPTION_SCREEN_ADJUST)
        {
            /* Owner leaves this N64 display adjustment inactive on DS. */
            return;
        }
        ndsMenuShellOptionWriteBackup();
        ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
        ndsMenuShellGoto((u32)nSCKindBackupClear);
        return;
    }
    /* B writes and returns to the main menu. No cue, like the source. */
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellOptionWriteBackup();
        ndsMenuShellGoto((u32)nSCKindModeSelect);
        return;
    }
}
