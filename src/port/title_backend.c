static void ndsSceneBoundary(void)
{
    gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
    gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
    if (gSCManagerSceneData.scene_curr == nSCKindTitle)
    {
        gNdsOpeningMovieTitleResult = NDS_OPENING_MOVIE_TITLE_PASS;
    }
    gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
    osStopThread(NULL);
}

/* Retired title thumbnail.
 *
 * BattleShip source contract (decomp/BattleShip-main/decomp/src/mn/mncommon/
 * mntitle.c): title sprites position through mnTitleSetPosition and animate
 * through mnTitleProcUpdate display procs. The DS software thumbnail
 * (reloc file load, SObj compose, admission test, software raster with
 * platform preview staging) was removed; its original remains in git at
 * a5f2223179d. The canonical shell already uses
 * native title UI; this stub never claims PASS or success for empty
 * rendering. It publishes the first native failure and leaves
 * gNdsTitlePreviewResult unset (existing failure result: 0). */
static void ndsTitleRenderPreview(void)
{
    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_SPRITE,
        (u32)gSCManagerSceneData.scene_curr, 0xffffffffu, 0u, 0u, 0u,
        NDS_NATIVE_FAILURE_NO_PROGRAM);
}

#define NDS_SCENE_STUB(name) void name(void) { ndsSceneBoundary(); }

static u32 ndsOpeningMovieBridgeMaskForKind(u32 scene_kind)
{
    if ((scene_kind < nSCKindOpeningRun) ||
        (scene_kind > nSCKindOpeningNewcomers))
    {
        return 0;
    }
    return 1u << (scene_kind - nSCKindOpeningRun);
}

/* Retired opening-action thumbnail.
 *
 * The software SObj raster with preview staging and cache commit lives
 * removed; the original remains in git at a5f2223179d.
 * This stub records the native failure with the requesting scene kind as
 * identity context and returns FALSE, never setting the preview PASS. */
static s32 ndsOpeningActionPreviewRender(u32 scene_kind)
{
    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_SPRITE,
        (u32)gSCManagerSceneData.scene_curr, scene_kind, 0u, 0u, 0u,
        NDS_NATIVE_FAILURE_NO_PROGRAM);
    return FALSE;
}

static void ndsOpeningActionPreviewHoldFrames(u32 frames)
{
    u32 i;

    for (i = 0; i < frames; i++)
    {
        ndsOpeningMoviePresentFrame();
        gNdsOpeningMovieActionPreviewFrameCount++;
    }
}

static void ndsOpeningMovieBridgeTo(u32 next_scene)
{
    u32 scene_kind = gSCManagerSceneData.scene_curr;

    gNdsOpeningMovieBridgeResult = NDS_OPENING_MOVIE_BRIDGE_PASS;
    gNdsOpeningMovieBridgeMask |=
        ndsOpeningMovieBridgeMaskForKind(scene_kind);
    gNdsOpeningMovieBridgeCount++;
    gNdsOpeningMovieBridgeLastKind = scene_kind;
    gNdsOpeningMovieBridgeLastNextKind = next_scene;

    (void)ndsOpeningActionPreviewRender(scene_kind);
    ndsOpeningActionPreviewHoldFrames(NDS_OPENING_ACTION_PREVIEW_FRAME_HOLD);

    gSCManagerSceneData.scene_prev = scene_kind;
    gSCManagerSceneData.scene_curr = next_scene;
}

NDS_SCENE_STUB(dbBattleStartScene)
NDS_SCENE_STUB(dbCubeStartScene)
NDS_SCENE_STUB(dbFallsStartScene)
NDS_SCENE_STUB(dbMapsStartScene)
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(mn1PModeStartScene)
#endif
#if !NDS_P2_MENU_SHELL && !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnBackupClearStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnCharactersStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnCongraStartScene)
#endif
#if !NDS_P2_MENU_SHELL && !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnDataStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnMessageStartScene)
#endif
#if !NDS_P2_MENU_SHELL
/* P2-1d defines the real main-menu scene in src/nds/nds_menu_shell.c. */
NDS_SCENE_STUB(mnModeSelectStartScene)
#endif
NDS_SCENE_STUB(mnNoControllerStartScene)
#if !NDS_P2_MENU_SHELL && !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnOptionStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnPlayers1PBonusStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnPlayers1PTrainingStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnPlayers1PGameContinueStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnPlayers1PGameStartScene)
#endif
#if !NDS_P2_MENU_SHELL && !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnScreenAdjustStartScene)
#endif
#if !NDS_P2_MENU_SHELL && !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnSoundTestStartScene)
#endif
NDS_SCENE_STUB(mnUnusedFightersStartScene)
#if !NDS_P2_MENU_SHELL
NDS_SCENE_STUB(mnVSItemSwitchStartScene)
NDS_SCENE_STUB(mnVSOptionsStartScene)
#endif
#if !NDS_P2_MENU_SHELL && !NDS_P2_1P_GAME
NDS_SCENE_STUB(mnVSRecordStartScene)
#endif
#if !NDS_IMPORT_BATTLESHIP_VS_RESULTS
NDS_SCENE_STUB(mnVSResultsStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(mvEndingStartScene)
#endif
NDS_SCENE_STUB(mvUnknownMarioStartScene)
/* P2-6 step 5 owns this scene when NDS_P2_1P_GAME=1
 * (battleship_sc1pbonusstage.c defines the real sc1PBonusStageStartScene). */
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(sc1PBonusStageStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(sc1PChallengerStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(sc1PIntroStartScene)
#endif
/* P2-6 step 1 owns this scene when NDS_P2_1P_GAME=1
 * (battleship_sc1pmanager.c defines the real sc1PManagerUpdateScene). */
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(sc1PManagerUpdateScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(sc1PStageClearStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(sc1PTrainingModeStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(scAutoDemoStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(scExplainStartScene)
#endif
#if !NDS_P2_1P_GAME
NDS_SCENE_STUB(scStaffrollStartScene)
#endif

void mvOpeningRunStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningCliff);
}

void mvOpeningCliffStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningYamabuki);
}

void mvOpeningYamabukiStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningJungle);
}

void mvOpeningJungleStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningYoster);
}

void mvOpeningYosterStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningSector);
}

void mvOpeningSectorStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningStandoff);
}

void mvOpeningStandoffStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningClash);
}

void mvOpeningClashStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningNewcomers);
}

void mvOpeningNewcomersStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindTitle);
}
