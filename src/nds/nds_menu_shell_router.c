/* --- The screen loop ----------------------------------------------------- */

/* P2-1h. A screen's backdrop art, drawn ONCE per entry into BG2.
 *
 * Separate from `ndsMenuShellPopulate` on purpose: populate re-runs on every
 * content change -- a cursor move, a value change -- and re-reading 84 KiB of
 * NitroFS on a keypress would put a scene load inside a 60 Hz frame. This runs
 * from `ndsMenuShellRun` only, after the overlay layers are cleared and the
 * backdrop colour is set, and then never again for the life of the screen.
 *
 * The two menus that take the collage are the two the SOURCE gives it to:
 * `mnModeSelectMakeDecals` (mnmodeselect.c:525) and `mnVSModeMakeBackground`
 * (mnvsmode.c:972), both at (10, 10). The character and stage selects have
 * their own source backgrounds and are not this row's. */
static const NdsUiKitSurfaceId kNdsMenuTitleSurfaces[] = {
    NDS_MN_UI_KIT_SURFACE_TITLE_SCREEN
};
/* P2-1i, owner finding (2). The main menu's own plate: one surface carrying
 * everything mnModeSelectMake* composes that the cursor does not change --
 * the collage, both decal bars, the MODE SELECT text, the SMASH emblem, the
 * four dark entry icons and the four red labels.
 *
 * P2-1j gave the VS menu the same treatment, so the BARE collage surface no
 * longer has a consumer and is no longer baked: both screens that show it now
 * show it inside their own composed plate. */
static const NdsUiKitSurfaceId kNdsMenuModeSelectSurfaces[] = {
    NDS_MN_UI_KIT_SURFACE_MODE_SELECT
};
/* P2-1i, owner finding (1). The character and stage selects sat on a flat
 * blue field; the source sits both of them on the SAME stone tile --
 * `llMNSelectCommonStoneBackgroundSprite` wrapped at its own 64x32 period
 * (masks 6, maskt 5), from mnplayersvs.c:1370 and mnmaps.c:356.
 *
 * P2-1k RETIRED the shared bare-stone surface: the stage select now has its
 * own composed plate exactly as the character select has, so the stone is the
 * first placement of each rather than a surface either has to blit.
 *
 * P2-1j. The VS rules screen's own plate -- collage, both decal papers, the
 * console-icon watermark, the menu-name fill rectangle and its three plaque
 * sprites (mnvsmode.c:965 and :909) -- with the four buttons blitted over it
 * by ndsMenuShellVsSyncButtons in whichever state the cursor puts them.
 *
 * P2-1j findings (c)/(d). The character select's stone now carries the twelve
 * portrait BOXES and every admitted portrait, because neither ever changes
 * while the screen is up: `llMNPlayersPortraitsPortraitFireBgSprite` behind
 * every cell (mnplayersvs.c:2437/:2503) with the fighter's portrait on it.
 * A SAVE-locked newcomer's noise-dithered shadow and question-mark plate
 * (:2404) are not in the plate: which newcomers the save has earned changes
 * per cartridge (mnplayersvs.c:296-314), so each ships once as its own
 * CSS_LOCKED_* surface and ndsMenuShellCssSyncLockedCells blits a locked
 * cell's own surface at entry. The stage select keeps the plain stone -- it
 * draws no portrait grid. */
static const NdsUiKitSurfaceId kNdsMenuVsSurfaces[] = {
    NDS_MN_UI_KIT_SURFACE_VS_MODE
};

static const NdsUiKitSurfaceId kNdsMenuCssSurfaces[] = {
    NDS_MN_UI_KIT_SURFACE_CSS_SCREEN
};

/* P2-1k (c). The stage select's own plate: the full-bleed stone, the preview
 * panel's fill and its seven tiles, the wooden plaque, both of
 * mnMapsLabelsProcDisplay's fills, the STAGE SELECT decal and the three-part
 * name plate -- everything mnMapsFuncStart composes that the cursor does not
 * change. The per-stage name and emblem ride the SSS_PLAQUE surfaces instead,
 * blitted by ndsMenuShellSssShowSelection. */
static const NdsUiKitSurfaceId kNdsMenuSssSurfaces[] = {
    NDS_MN_UI_KIT_SURFACE_SSS_SCREEN
};

static void ndsMenuShellEnterBackdrop(u32 screen)
{
    switch (screen)
    {
    case NDS_MENU_SHELL_SCREEN_TITLE:
        (void)ndsUiKitBlitSurfaces(kNdsMenuTitleSurfaces,
                                   (u32)(sizeof(kNdsMenuTitleSurfaces) /
                                         sizeof(kNdsMenuTitleSurfaces[0])));
        /* Cached rather than re-read: PRESS START toggles every 32 frames and
         * one NitroFS open costs more than a whole 60 Hz frame's budget. */
        (void)ndsUiKitCacheSurface(NDS_MN_UI_KIT_SURFACE_TITLE_PRESS_START);
        sMenuTitleBlinkPhase = 0u;
        ndsUiKitDrawCachedSurface();
        /* P2-1i: the fire atlas goes into BG3 at entry and BURNS FROM THE
         * FIRST PRESENT -- `mnTitleMakeFire` shows it during construction on
         * this branch (mntitle.c:990), before the scene's first tic, so a
         * title whose first frames are a black field is not the source's.
         * Enabling here rather than in the update is what guarantees frame 0
         * already has it: the loop's first Update runs after this. */
        (void)ndsUiKitBlitFireAtlas();
        ndsPlatformSetTitleFireEnabled(TRUE, (s32)NDS_MN_UI_KIT_FIRE_PA,
                                       (s32)NDS_MN_UI_KIT_FIRE_PD);
        /* Cell 0, matching what the loop's first update will select from
         * sMenuTics == 0 -- so the reference point never shows a cell the
         * timeline has not reached. */
        ndsMenuShellTitleFireFrame(0u);
        gNdsTitleFireRevealFrame =
            gNdsMenuShellFrames[NDS_MENU_SHELL_SCREEN_TITLE] + 1u;
        /* P2-1k (d). THE POP ANIMATION IS ARMED HERE, after the static title is
         * on the screen, because pose 1 is defined as the removal of it: the
         * source hides the label GObj until tic 170 and shows it already at
         * scale 0, so the first thing the animation owes the panel is the
         * settled wordmark's absence.
         *
         * ARENA, NOT .bss. The six rasters are 98,920 bytes and the binary's
         * size comes straight out of the taskman arena, one for one -- a
         * static buffer this size would cost every scene in the game, battle
         * included, for a screen that shows it for 0.85 s. `syTaskmanMalloc`
         * takes it out of the TITLE scene's own arena, which the scene
         * teardown rewinds, so the cost lands on the one screen that has it to
         * spare (title high-water 416,828 of 1,548,288 bytes).
         *
         * A refusal is not a failure path worth branching on: the animation
         * simply does not arm, the entry blit's settled layout stays, and the
         * screen still reads input and still reaches the mode select. The
         * counter is what tells the two apart. */
        {
            u32 anim_bytes = ndsUiKitTitleAnimBytes();
            void *anim_block = syTaskmanMalloc((size_t)anim_bytes, 4u);

            sMenuTitleVBlankBase = ndsPlatformVBlankCount();
            if (ndsUiKitTitleAnimLoad(anim_block, anim_bytes) != FALSE)
            {
                ndsUiKitTitleAnimDraw(1u);
            }
        }
        break;
    case NDS_MENU_SHELL_SCREEN_MODE:
        (void)ndsUiKitBlitSurfaces(kNdsMenuModeSelectSurfaces,
                                   (u32)(sizeof(kNdsMenuModeSelectSurfaces) /
                                         sizeof(kNdsMenuModeSelectSurfaces[0])));
        break;
    case NDS_MENU_SHELL_SCREEN_VSMODE:
        (void)ndsUiKitBlitSurfaces(kNdsMenuVsSurfaces,
                                   (u32)(sizeof(kNdsMenuVsSurfaces) /
                                         sizeof(kNdsMenuVsSurfaces[0])));
        break;
    case NDS_MENU_SHELL_SCREEN_CSS:
        (void)ndsUiKitBlitSurfaces(kNdsMenuCssSurfaces,
                                   (u32)(sizeof(kNdsMenuCssSurfaces) /
                                         sizeof(kNdsMenuCssSurfaces[0])));
        break;
    case NDS_MENU_SHELL_SCREEN_SSS:
        (void)ndsUiKitBlitSurfaces(kNdsMenuSssSurfaces,
                                   (u32)(sizeof(kNdsMenuSssSurfaces) /
                                         sizeof(kNdsMenuSssSurfaces[0])));
        break;
    default:
        break;
    }
}

static void ndsMenuShellPopulate(u32 screen)
{
    switch (screen)
    {
    case NDS_MENU_SHELL_SCREEN_TITLE:
        ndsMenuShellPopulateTitle();
        break;
    case NDS_MENU_SHELL_SCREEN_MODE:
        ndsMenuShellPopulateMode();
        break;
    case NDS_MENU_SHELL_SCREEN_CSS:
        ndsMenuShellPopulateCssScreen();
        break;
    case NDS_MENU_SHELL_SCREEN_SSS:
        ndsMenuShellSssPopulate();
        break;
    case NDS_MENU_SHELL_SCREEN_ITEMSWITCH:
        ndsMenuShellPopulateItems();
        break;
    case NDS_MENU_SHELL_SCREEN_VSOPTIONS:
        ndsMenuShellPopulateVsOptions();
        break;
    case NDS_MENU_SHELL_SCREEN_OPTION:
        ndsMenuShellPopulateOption();
        break;
    case NDS_MENU_SHELL_SCREEN_BACKUPCLEAR:
        ndsMenuShellPopulateBackupClear();
        break;
    default:
        ndsMenuShellPopulateVs();
        break;
    }
}

static void ndsMenuShellUpdate(u32 screen, u32 held, u32 taps)
{
    switch (screen)
    {
    case NDS_MENU_SHELL_SCREEN_TITLE:
        ndsMenuShellUpdateTitle(held, taps);
        break;
    case NDS_MENU_SHELL_SCREEN_MODE:
        ndsMenuShellUpdateMode(held, taps);
        break;
    case NDS_MENU_SHELL_SCREEN_CSS:
        ndsMenuShellUpdateCss(held, taps);
        ndsMenuShellCssSyncPreviews();
        ndsMNPlayersVSPreviewFrame();
        break;
    case NDS_MENU_SHELL_SCREEN_SSS:
        ndsMenuShellUpdateSss(held, taps);
        break;
    case NDS_MENU_SHELL_SCREEN_ITEMSWITCH:
        ndsMenuShellUpdateItems(held, taps);
        break;
    case NDS_MENU_SHELL_SCREEN_VSOPTIONS:
        ndsMenuShellUpdateVsOptions(held, taps);
        break;
    case NDS_MENU_SHELL_SCREEN_OPTION:
        ndsMenuShellUpdateOption(held, taps);
        break;
    case NDS_MENU_SHELL_SCREEN_BACKUPCLEAR:
        ndsMenuShellUpdateBackupClear(held, taps);
        break;
    default:
        ndsMenuShellUpdateVs(held, taps);
        break;
    }
}

static void ndsMenuShellRun(u32 screen)
{
    u32 enter_start = cpuGetTiming();

    sMenuScreen = screen;
    sMenuTics = 0u;
    sMenuChangeWait = 0u;
    sMenuLeaving = FALSE;
    sMenuNextScene = 0xffffffffu;
    sMenuFrameArmed = 0u;
    sMenuLastVBlank = ndsPlatformVBlankCount();
    sMenuFrameStartTicks = enter_start;
    /* The keypad's current state is the baseline, so a button still held from
     * the previous screen cannot read as a fresh tap on this one. */
    sMenuHeldPrev = ndsPlatformReadInput();
#if NDS_P2_MENU_WALK
    /* Every screen gets a full dwell BEFORE its first scripted input, not just
     * between inputs. Without it the title's START lands on the title's first
     * frame, which leaves no cadence window and nothing to capture. The step
     * cursor restarts here too, which is what lets the loop re-enter VS Mode
     * from Results and replay the whole tour. */
    sMenuWalkTimer = ((screen == NDS_MENU_SHELL_SCREEN_CSS) ||
                      (screen == NDS_MENU_SHELL_SCREEN_SSS)) ?
        NDS_MENU_WALK_DWELL_CSS : NDS_MENU_WALK_DWELL;
    sMenuWalkCursor = 0u;
    sMenuWalkHold = 0u;
    sMenuWalkHeld = 0u;
#endif

    gNdsMenuShellScreen = screen;
    gNdsMenuShellEnterCount[screen]++;

    /* Main BG0 is the retained 3D surface in MODE_5_3D. Only the character
     * select owns 3D inside this native menu shell; every other screen hides
     * it so CSS's last fighter frame cannot remain composited over Stage
     * Select or a VS/menu screen after the source GObjs are gone. Battle
     * explicitly reclaims BG0 at its own scene entry. */
    ndsPlatformSet3DLayerEnabled(
        (screen == NDS_MENU_SHELL_SCREEN_CSS) ? TRUE : FALSE);

    /* The battle's sprite compositor owns BG2/BG3, and a menu must not inherit
     * whatever the last battle frame left in them -- so both layers are
     * CLEARED. They are deliberately NOT hidden.
     *
     * MEASURED, and it cost a cycle to find: with the overlay disabled the
     * main OBJ layer does not reach the screen at all, even though DISPCNT
     * bit 12 reads 1, OAM holds valid 32x8 bitmap entries at priority 0, and
     * the composed texels are in bank E at the offset attr2 names
     * (0x06400000+24832, read back over GDB). The top screen measured
     * 0/49152 pixels differing from the clear colour on three separate
     * captures. Disabling the overlay is the ONE display-state difference
     * against P2-1c's demo, which renders the same kit through the same
     * ndsPlatformEndFrame: it takes the 3D clear to alpha 31 (opaque) and
     * hides BG2/BG3, the second alpha-blend target REG_BLDCNT names. Keeping
     * the overlay enabled restores the proven state -- transparent 3D clear,
     * both overlay layers present and empty -- and it is what makes the DS
     * backdrop (main BG palette entry 0) the visible field behind a menu. */
    ndsPlatformSetOriginalSpriteOverlayEnabled(TRUE);
    ndsPlatformClearOriginalSpriteOverlayLayer(FALSE);
    ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
    ndsPlatformClearBattleTextHud();

    /* A refusal here is counted in gNdsUiKitEnterRejectCount and is NOT a
     * reason to change the flow: every kit call below returns FALSE while the
     * kit is inactive, so the screen still reads input and still reaches its
     * successor -- it just draws nothing. A refusal that also re-routed the
     * player would turn one visible defect into two invisible ones. */
    (void)ndsUiKitEnter(NDS_UI_KIT_ENGINE_MAIN);
    ndsMenuShellHideRows();
    /* THE BACKDROP IS SET BEFORE THE ART, not after. A backdrop surface is
     * composited over its screen's own field at bake time, so the two have to
     * agree: the title's art is composited over black and every other screen's
     * collage over the source's decal blue. Setting it afterwards left one
     * entry frame showing the previous screen's field behind the new art. */
    BG_PALETTE[0] = (screen == NDS_MENU_SHELL_SCREEN_TITLE) ?
        NDS_MENU_BACKDROP_BLACK : NDS_MENU_BACKDROP_BLUE;
    /* The battle's fast wallpaper leaves BG2 under a 4/5 affine transform, and
     * the reset the clear above queues is only applied at the next present --
     * which is one frame AFTER this backdrop is drawn. Committing it here is
     * what stops a menu entered straight out of a battle showing one frame of
     * scaled artwork. */
    ndsPlatformCommitOriginalSpriteOverlayTransform();
    ndsMenuShellEnterBackdrop(screen);
    ndsMenuShellPopulate(screen);

    gNdsMenuShellEnterTicks[screen] = cpuGetTiming() - enter_start;

    while (sMenuLeaving == FALSE)
    {
        u32 held;
        u32 taps = ndsMenuShellReadTaps(&held);

        ndsMenuShellRecordInput(taps);
        ndsMenuShellUpdate(screen, held, taps);
        ndsMenuShellTickInput(held);
        sMenuTics++;

        /* P2-1k (g2). `ndsAudioBgmUpdate` has exactly two callers in the whole
         * tree before this one -- the battle's own per-frame update
         * (`ndsRunMarioFoxProofUpdate`, taskman_seam.c:4488) and the VS
         * Results dev-harness loop (taskman_seam.c:7364) -- so no menu shell
         * screen ever served a BGM refill: `ndsAudioBgmPlay` synchronously
         * preloads two packets (~1.5 s at 16,384 samples/22,050 Hz) and the
         * hardware-timer-driven worker thread (`ndsAudioBgmHandleSeam`,
         * nds_audio_bgm.c) can swap between those two once, but nothing ever
         * called `ndsAudioBgmServiceRefills` to load a third -- the seam miss
         * that follows sets `gNdsAudioBgmSeamMissCount`/`OverrunCount` and
         * `sNdsAudioBgmErrorPending` from the worker thread, but only
         * `ndsAudioBgmUpdate` (main thread) ever reads that flag and turns it
         * into `ndsAudioBgmFailPlayback`, so `gNdsAudioBgmPlaying` never even
         * dropped to 0 -- track 44/10 looked "stuck playing" while the
         * hardware channel had already gone silent underneath it. Menus are
         * the ONLY screens this file drives that need it: title never plays
         * BGM (P2-1k (g)) and every other native screen (mode select, VS
         * mode, character select, stage select) funnels through this one
         * loop. Matches the battle's own placement -- logic update before the
         * frame's render/present calls. */
        ndsAudioBgmUpdate();

        ndsPlatformRenderDebugHud();
        ndsMenuShellRecordFrame();
        ndsPlatformEndFrame();
        ndsMenuShellRecordPresent();
    }

    /* EVERY title exit path lands here (START/A is the only one today), so
     * this is where the fire hands BG3 back: disable restores the identity
     * affine and priority 0, and the NEXT screen's entry clears the bitmap
     * itself (the same clear above), which is what leaves the next owner the
     * transparent foreground overlay it expects. */
    if (screen == NDS_MENU_SHELL_SCREEN_TITLE)
    {
        ndsPlatformSetTitleFireEnabled(FALSE, 0, 0);
        /* P2-1k (d). Drops the pose table's view of the arena block; the block
         * itself belongs to the scene the teardown below rewinds. */
        ndsUiKitTitleAnimEnd();
    }
    if (screen == NDS_MENU_SHELL_SCREEN_CSS)
    {
        /* Clear the renderer's scene-local fighter registrations while the
         * PlayersVS arena is still valid; gcEjectAll below owns the camera and
         * every remaining GObj. */
        ndsMNPlayersVSPreviewExit();
        ndsPlatformSet3DLayerEnabled(FALSE);
    }

    ndsUiKitExit();
    BG_PALETTE[0] = NDS_MENU_BACKDROP_BLACK;
    gNdsMenuShellExitCount[screen]++;
    gNdsMenuShellScreen = 0xffffffffu;

    /* THE SOURCE'S OWN TEARDOWN. syTaskmanCommonTaskUpdate calls gcEjectAll
     * when the scene's loop breaks (decomp sys/taskman.c:1099), which is what
     * runs osDestroyThread before gcEjectGObjStack (objman.c:918). The bounded
     * branches this row replaces never reached it, which is how P2-1b-1's
     * stale-thread data abort happened; a native screen makes no GObjs of its
     * own, but the scene's own func_start did, so the teardown still has work
     * and is still the contract. */
    gcEjectAll();

    if (sMenuNextScene != 0xffffffffu)
    {
        ndsSceneManagerRequest(sMenuNextScene,
                               (u32)gSCManagerSceneData.scene_curr);
    }
}

void ndsMenuShellRunStartup(void)
{
    /* P2-1h MOVED THIS SEAM, and it is the one thing that had to move with the
     * splash rather than after it. The splash was the shell's earliest entry;
     * with the splash deleted, THIS is, and the load has to stay ahead of the
     * first cue or every menu SFX before the first battle resolves against an
     * empty pack again. Startup presents no frame -- boot reaches the title
     * directly, which is the N64 flow once the opening cinematic (P2-7) is
     * accounted for -- but the scene still EXISTS, because the source's own
     * startup func_start has already run and made GObjs by the time
     * syTaskmanRunTask is reached.
     *
     * P2-1d-1 ROOT CAUSE, not scoped to FGM 157: ndsAudioAssetLoadFenced (which
     * loads the FGM pack, nds_audio_fgm.c's ndsAudioFgmLoadFenced included) had
     * exactly three call sites before this one -- battleship_scvsbattle.c:469
     * and the two ndsR2HostBattlePrepare/is_battle_playable sites in
     * taskman_seam.c -- all battle-entry only. Every menu-shell FGM request
     * made before battle starts therefore found gNdsAudioFgmLoaded == 0 and
     * sNdsAudioFgmEntries[] all-zero: not a missing pack entry, a pack that had
     * not been asked to load yet. Measured with a throwaway GDB read at
     * ndsMenuShellRunModeSelect: `loaded=0 result=00000000 openfail=0
     * readfail=0 fmtfail=0` (every failure counter zero -- the loader was never
     * called, not called-and-failed) alongside `unsupported=1`, matching FGM
     * 157's miss ring entry exactly. This is why the P2-1c-1 kit's ids
     * (158/163/164/165) are NOT in the miss ring either: they are declared in
     * ndsAudioFgmIDIsIncluded, so the same unloaded-pack failure silently
     * incremented gNdsAudioFgmIncludedLookupFailCount instead of the miss ring
     * -- a defect the miss-ring-only P2-1c probe could not see. Startup is the
     * menu shell's own earliest entry (nSCKindStartup, scene kind 27, the
     * first scene of every run) and this call is idempotent
     * (sNdsAudioAssetLoaded guards it), so loading here once, before Title can
     * ever request the press-start cue, costs nothing on every later call and
     * makes every downstream FGM request resolve against a real pack instead
     * of an empty one. The pack lives in static .bss (sNdsAudioFgmMetadata /
     * sNdsAudioFgmEntries), not the taskman arena, so it survives every scene's
     * arena rewind after this one load. */
#if NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS
    ndsAudioAssetLoadFenced();
#endif
    gNdsMenuShellStartupCount++;

    /* THE SOURCE'S OWN TEARDOWN, for the same reason every screen runs it
     * (decomp sys/taskman.c:1099). This branch presents no frame, but the
     * startup scene's func_start ran before syTaskmanRunTask and its GObjs are
     * live in the arena the next scene entry rewinds -- which is precisely the
     * shape of the P2-1b-1 stale-thread data abort. Skipping it because "this
     * screen draws nothing" would reopen that bug. */
    gcEjectAll();
    /* Deliberately NOT written into the transition ring: the ring pairs a
     * SCREEN with the scene it hands off to, and no screen ran here. An empty
     * first ring slot is the evidence that boot reaches the title directly. */
    ndsSceneManagerRequest((u32)nSCKindTitle,
                           (u32)gSCManagerSceneData.scene_curr);
}

/* P2-1k (g), OWNER FINDING: "the background music is always playing".
 *
 * `mnTitleInitVars` (mntitle.c:340) is the source's answer and it is
 * unconditional on our branch: `scene_prev` can only be `nSCKindOpeningNewcomers`
 * when the opening cinematic ran, which is P2-7, so the `else` arm is the one
 * this build always takes and its first statement is `syAudioStopBGMAll()`
 * (:352). THE TITLE IS SILENT. This shell had no BGM call on the title at all,
 * so a track started downstream simply kept playing over it -- boot reached the
 * title with nothing playing and looked correct, but the main menu's own B
 * (mode select -> title) left BGM 44 running under the title screen, which is
 * exactly what the owner heard.
 *
 * Placed in the RunTitle wrapper rather than inside ndsMenuShellRun for the
 * same reason the mode select's play is: this is the title's func_start
 * equivalent, it runs once per entry, and it runs after
 * ndsSceneManagerRequest has written scene_prev. */
void ndsMenuShellRunTitle(void)
{
    ndsAudioBgmStopAll();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_TITLE);
}

/* mnmodeselect.c:882 (mnModeSelectFuncStart), transcribed exactly: the main
 * menu's own track plays on arrival at ModeSelect UNLESS the previous scene
 * is one of ModeSelect's own children -- 1P Game, VS Mode, Option, or Data --
 * in which case a track is presumably already playing and is left alone
 * rather than restarted. `scene_prev` is read here because this function
 * runs once, as ModeSelect's func_start-equivalent (wired from
 * syTaskmanRunTask, P2-1d), which is after ndsSceneManagerRequest has written
 * it and before anything else can change it -- the same ordering the source
 * itself relies on. Only VSMode is reachable as scene_prev in this build
 * today (1P Game/Option/Data are P2-1e/1f/1g); all four are transcribed
 * unconditionally so the condition matches the source exactly and needs no
 * revisiting when those scenes land. */
static void ndsMenuShellModeSelectPlayBgm(void)
{
    if (((u8)gSCManagerSceneData.scene_prev != (u8)nSCKind1PMode) &&
        ((u8)gSCManagerSceneData.scene_prev != (u8)nSCKindVSMode) &&
        ((u8)gSCManagerSceneData.scene_prev != (u8)nSCKindOption) &&
        ((u8)gSCManagerSceneData.scene_prev != (u8)nSCKindData))
    {
        ndsAudioBgmPlay(0, (s32)nSYAudioBGMModeSelect);
    }
}

void ndsMenuShellRunModeSelect(void)
{
    ndsMenuShellModeSelectPlayBgm();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_MODE);
}

/* P2-1k (g). mnVSModeFuncStart's tail, mnvsmode.c:1645: the VS menu starts the
 * mode-select track when -- and only when -- it was entered FROM the character
 * select. Arriving from the mode select it starts nothing, because that track
 * is already playing and restarting it would re-cue the intro; arriving back
 * from the CSS it must start, because `mnPlayersVSBackToVSMode` stopped all BGM
 * on the way out (mnplayersvs.c:3234, transcribed in ndsMenuShellCssBack). The
 * pair is what makes the music continuous across mode -> VS and restart across
 * VS -> CSS -> VS, and this shell had neither half. */
void ndsMenuShellRunVSMode(void)
{
    if ((u8)gSCManagerSceneData.scene_prev == (u8)nSCKindPlayersVS)
    {
        ndsAudioBgmPlay(0, (s32)nSYAudioBGMModeSelect);
    }
    ndsMenuShellVsLoadRules();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_VSMODE);
}

/* mnPlayersVSFuncStart's tail, mnplayersvs.c:4790-4798: the CSS starts its own
 * track unless it was entered FROM the stage select, then announces the mode
 * already carried by the transfer state.  Do not hard-code FREE FOR ALL here:
 * returning from Maps while Team Battle is selected must say TEAM BATTLE just
 * as the source does. */
static void ndsMenuShellCssPlayBgm(void)
{
    if ((u8)gSCManagerSceneData.scene_prev != (u8)nSCKindMaps)
    {
        ndsAudioBgmPlay(0, NDS_CSS_BGM_BATTLE_SELECT);
    }
    ndsMenuShellCssAnnounceMode();
}

void ndsMenuShellRunCharSelect(void)
{
    ndsMNPlayersVSPreviewInit();
    ndsMenuShellCssInit();
    ndsMenuShellCssSyncPreviews();
    ndsMenuShellCssPlayBgm();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_CSS);
}

/* mnMapsFuncStart, mnmaps.c:1591: no BGM call anywhere in it, so the character
 * select's track plays straight through. Nothing to start here, and saying so
 * is the point -- every other RunX above starts or deliberately skips one. */
void ndsMenuShellRunStageSelect(void)
{
    ndsMenuShellSssInit();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_SSS);
}

/* mnVSItemSwitchFuncStart, mnvsitemswitch.c:851: no BGM call anywhere in it,
 * so whatever the VS menu started keeps playing across the option screens --
 * the same reasoning, and the same silence, as the stage select above. */
void ndsMenuShellRunItemSwitch(void)
{
    ndsMenuShellItemsLoad();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_ITEMSWITCH);
}

/* mnVSOptionsFuncStart, mnvsoptions.c:1160. Same silence as the Item Switch
 * screen above: the source starts no BGM here, so the VS menu's music keeps
 * playing across both option screens. */
void ndsMenuShellRunVsOptions(void)
{
    ndsMenuShellVsOptionsLoad();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_VSOPTIONS);
}

void ndsMenuShellRunOption(void)
{
    if (gSCManagerSceneData.scene_prev == nSCKindScreenAdjust)
    {
        syAudioPlayBGM(0, nSYAudioBGMModeSelect);
    }
    ndsMenuShellOptionLoad();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_OPTION);
}

void ndsMenuShellRunBackupClear(void)
{
    ndsMenuShellBackupClearLoad();
    ndsMenuShellRun(NDS_MENU_SHELL_SCREEN_BACKUPCLEAR);
}

/* --- The mode-select scene ----------------------------------------------
 *
 * nSCKindModeSelect had no scene in this build at all: src/port/title_backend.c
 * carried it as an NDS_SCENE_STUB, whose whole body is osStopThread. This is
 * the real one, and it is deliberately the same shape as the imported menu
 * scenes (src/import/battleship_mnvsmode.c:308): the taskman setup is VS
 * Mode's own -- the object pools, DL buffer, graphics heap and RDP buffer a
 * menu scene declares in this build, already proven by P2-1b's walk -- with
 * the arena repointed at the shared taskman arena so every registered scene
 * rewinds the SAME memory and their high-waters stay comparable.
 *
 * `func_start` is NULL because the screen is native: the source's func_start
 * exists to load MNCommon/MNMain and build an SObj graph for gcDrawAll, and
 * this scene draws through the UI kit instead. */
extern SYTaskmanSetup dMNVSModeTaskmanSetup;
extern SYVideoSetup dMNVSModeVideoSetup;

static void ndsMenuShellStartNative2DScene(void)
{
    SYTaskmanSetup setup = dMNVSModeTaskmanSetup;

    dMNVSModeVideoSetup.zbuffer = SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNVSModeVideoSetup);

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = NULL;
    syTaskmanStartTask(&setup);
}

void mnModeSelectStartScene(void)
{
    ndsMenuShellStartNative2DScene();
}

/* These screens use the same native kit and arena lifecycle as VS Mode.
 * The task runner dispatches their existing native loops by scene kind. */
void mnVSOptionsStartScene(void)
{
    ndsMenuShellStartNative2DScene();
}

void mnVSItemSwitchStartScene(void)
{
    ndsMenuShellStartNative2DScene();
}

void mnOptionStartScene(void)
{
    ndsMenuShellStartNative2DScene();
}

void mnBackupClearStartScene(void)
{
    ndsMenuShellStartNative2DScene();
}

/* --- The character-select scene ------------------------------------------
 *
 * This one REPLACES an imported scene rather than filling an empty stub:
 * src/import/battleship_mnplayersvs.c compiles the original scene and its
 * `mnPlayersVSStartScene` runs the original `mnPlayersVSFuncStart`, which loads
 * seven menu files, calls ftManagerSetupFilesAllKind for all TWELVE fighters
 * and allocates four figatree heaps out of the scene arena (mnplayersvs.c:4750)
 * -- everything the RSP/RDP scene graph this build does not have would need.
 * That definition is compiled out at NDS_P2_MENU_SHELL and this is the one that
 * runs: the source's own taskman declaration for this scene, with the arena
 * repointed at the shared taskman arena so every registered scene rewinds the
 * SAME memory and the high-waters stay comparable, and `func_start = NULL`
 * because the screen draws through the UI kit. Same shape as the mode select
 * above, and the same shape P2-1d recorded as the template. */
extern SYTaskmanSetup dMNPlayersVSTaskmanSetup;
extern SYVideoSetup dMNPlayersVSVideoSetup;

void mnPlayersVSStartScene(void)
{
    SYTaskmanSetup setup = dMNPlayersVSTaskmanSetup;

    dMNPlayersVSVideoSetup.zbuffer =
        SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNPlayersVSVideoSetup);

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = NULL;
    syTaskmanStartTask(&setup);
}

/* --- The stage-select scene ----------------------------------------------
 *
 * The second scene this shell REPLACES rather than fills. The imported
 * `mnMapsStartScene` (src/import/battleship_mnmaps.c) runs the original
 * `mnMapsFuncStart`, which loads five menu files, allocates TWO model heaps
 * sized to the largest of the nine stage map files, and builds the wallpaper,
 * plaque, icon, name, cursor and 3D-preview object graph plus eight cameras
 * (mnmaps.c:1591) -- all of it for the RSP/RDP pipeline this build does not
 * have. That definition is compiled out at NDS_P2_MENU_SHELL and this is the
 * one that runs: the source's own taskman declaration for this scene, with the
 * arena repointed at the shared taskman arena so every registered scene
 * rewinds the SAME memory and the high-waters stay comparable, and
 * `func_start = NULL` because the screen draws through the UI kit. Same shape
 * as the two above. */
extern SYTaskmanSetup dMNMapsTaskmanSetup;
extern SYVideoSetup dMNMapsVideoSetup;

void mnMapsStartScene(void)
{
    SYTaskmanSetup setup = dMNMapsTaskmanSetup;

    dMNMapsVideoSetup.zbuffer = SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNMapsVideoSetup);

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = NULL;
    syTaskmanStartTask(&setup);
}

