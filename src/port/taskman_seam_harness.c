#if NDS_HARNESS_FAST_PRESENT_ON_REQUEST
static volatile u32 sNdsHarnessFastPresentRequested;
volatile u32 gNdsHarnessFastPresentRequestCount;
volatile u32 gNdsHarnessFastPresentConsumeCount;
#endif

void ndsHarnessFastPresentRequest(void)
{
#if NDS_HARNESS_FAST_PRESENT_ON_REQUEST
    sNdsHarnessFastPresentRequested = 1u;
    gNdsHarnessFastPresentRequestCount++;
#endif
}

#if NDS_IMPORT_BATTLESHIP_VS_RESULTS || NDS_P2_1P_GAME
/* P2-7 item 9. The generic source-MENU pump, factored out of the imported VS
 * Results loop below so every imported SOURCE menu runs the same contract:
 * controller read, one task_update, audio update, then one scene_draw present
 * (the graphics-heap reset plus present the source's one-update/one-draw
 * display callbacks need), bounded by the same fast-logic update cap. The
 * sprite overlay is enabled around the run, the boundary kind/result
 * bookkeeping is done here, and a LoadScene exit disables the overlay and
 * returns TRUE so the source scene's own scene_curr write is honoured.
 *
 * Results-only extras stay with the Results caller, not here: the fighter
 * packet release (the battle -> Results handoff), gNdsFtPoseEvalTick (the
 * victory-pose evaluation tick) and ndsMNVSResultsRecordFrame (the results
 * recorder). A source menu has none of those. Pass is_results nonzero for
 * Results, zero for a generic menu. */
static u32 ndsSeamRunSourceMenuScene(struct SYTaskFunction *tfunc, u32 is_results)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
    /* tic 120 starts a finite winner sequence; run long enough for the
     * original Results audio thread to observe AL_STOPPED and start BGM 22. */
    const u32 fast_update_max = NDS_AUDIO_BGM_RESULTS_FAST_UPDATE_MAX;
#else
    const u32 fast_update_max = 132u;
#endif
    u32 updates = 0u;

    ndsPlatformSetOriginalSpriteOverlayEnabled(TRUE);
    while ((tfunc != NULL) && (tfunc->task_update != NULL) &&
           (sSYTaskmanStatus != nSYTaskmanStatusLoadScene) &&
           ((NDS_HARNESS_FAST_LOGIC == 0) ||
            (updates < fast_update_max)))
    {
        /* P2-1g adds the second disjunct. The fast-logic arm used to skip
         * the controller pipeline entirely here, so Results could only be
         * left by its update cap -- and the scripted walk's START (which
         * travels the real keypad latch) had no reader. `NDS_P2_MENU_WALK`
         * is 0 in every published and Boundary configuration, so mode 163
         * evaluates the identical expression it always did. */
        if ((NDS_HARNESS_FAST_LOGIC == 0) || (NDS_P2_MENU_WALK != 0))
        {
            (void)ndsPlatformReadInput();
#if NDS_SEAM_CONTROLLER_PAIR
            syControllerReadDeviceData();
            syControllerUpdateGlobalData();
#endif
        }
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
        if (is_results != 0u)
        {
            /* P2-2p6 normally evaluates fighter body poses on the final source
             * tick before a present. Results has exactly one source update per
             * present, so that update is always the evaluation tick. Without
             * publishing it here the pose player advances its script clock but
             * holds the body matrices forever, freezing every victory / loss /
             * No Contest result animation. */
            gNdsFtPoseEvalTick = 1u;
        }
#else
        (void)is_results;
#endif
        tfunc->task_update(tfunc);
        ndsAudioBackendUpdate();
        dSYTaskmanUpdateCount++;
        updates++;
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
        if (is_results != 0u)
        {
            ndsMNVSResultsRecordFrame();
        }
#endif

        /* Results owns fade progression in display callbacks; preserve the
         * source one-update/one-draw contract in fast verification too. */
        {
            gNdsRendererProfileFrameCount++;
            /* taskman.c:1093-1100 resets these arenas before every
             * source scene draw. */
            ndsTaskmanSampleGraphicsHeap();
            syTaskmanResetGraphicsHeap();
            func_80004AB0();
            ndsSObjPreviewBeginFrame();
            if (tfunc->scene_draw != NULL)
            {
                tfunc->scene_draw();
                dSYTaskmanFrameCount++;
            }
            ndsSObjPreviewEndFrame();
            /* Same position the battle present gives it (see :4810). A
             * scene that drives its own presentation has to draw the HUD
             * itself; until R4d the only thing drawing it here was the
             * redundant main-loop present, so removing that present would
             * otherwise take the HUD with it. */
            ndsPlatformRenderDebugHud();
            ndsPlatformEndFrame();
        }
    }

    ndsFinishTaskmanRun();
    gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
    gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
    if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
    {
        ndsPlatformSetOriginalSpriteOverlayEnabled(FALSE);
        return TRUE;
    }
    return FALSE;
}
#endif

void syTaskmanRunTask(struct SYTaskFunction *tfunc)
{
    ndsPrepareTaskmanRun();
#if NDS_HARNESS_FAST_PRESENT_ON_REQUEST
    sNdsHarnessFastPresentRequested = 0u;
    gNdsHarnessFastPresentRequestCount = 0u;
    gNdsHarnessFastPresentConsumeCount = 0u;
#endif

#if NDS_P2_MENU_SHELL
    /* P2-1d. The VS shell's four real screens, ahead of every bounded branch
     * below because they REPLACE those branches rather than sit beside them:
     * with the shell on, Startup/Title/ModeSelect/VSMode are screens the player
     * drives, not scenes that park. Each returns with its successor already
     * requested through the scene manager, so returning here hands control
     * back to scManagerRunLoop's dispatch exactly as the source's own scene
     * tail does. The VSBattle and VSResults branches are untouched: this shell
     * takes the player TO the match and the match is still the Boundary's. */
    switch (gSCManagerSceneData.scene_curr)
    {
    case nSCKindStartup:
        /* P2-1h: no screen, no presented frame -- this requests the title and
         * returns, so boot reaches the title directly (the N64 flow, with the
         * opening cinematic that precedes it deferred to P2-7). It still runs,
         * rather than being deleted, because it carries the menu audio load
         * and the startup scene's own GObj teardown. */
        ndsMenuShellRunStartup();
        ndsFinishTaskmanRun();
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        return;
    case nSCKindTitle:
        ndsMenuShellRunTitle();
        ndsFinishTaskmanRun();
        gNdsOpeningMovieTitleResult = NDS_OPENING_MOVIE_TITLE_PASS;
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        return;
    case nSCKindModeSelect:
        ndsMenuShellRunModeSelect();
        ndsFinishTaskmanRun();
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        return;
    case nSCKindVSMode:
        ndsMenuShellRunVSMode();
        ndsFinishTaskmanRun();
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        return;
    case nSCKindPlayersVS:
        /* P2-1e. Unlike the four above, this branch replaces a scene that was
         * not a park: the imported PlayersVS branch further down runs the
         * original setup and its transition probe. That branch is unreachable
         * with the shell on, because this switch runs first -- and the imported
         * scene's own StartScene is compiled out at NDS_P2_MENU_SHELL so the
         * heavy original func_start cannot run either. */
        ndsMenuShellRunCharSelect();
        ndsFinishTaskmanRun();
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        return;
    case nSCKindMaps:
        /* P2-1f, and the same story as PlayersVS above: the imported Maps
         * branch further down runs the original setup and its own bounded
         * select-transition probe. That branch is unreachable with the shell
         * on because this switch runs first, and the imported scene's own
         * StartScene is compiled out at NDS_P2_MENU_SHELL so the heavy
         * original func_start (five menu files, two stage-sized model heaps,
         * the 3D preview graph) cannot run either. */
        ndsMenuShellRunStageSelect();
        ndsFinishTaskmanRun();
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        return;
    case nSCKindVSItemSwitch:
        /* P2-5u1. src/port/title_backend.c:431 carries this kind as an
         * NDS_SCENE_STUB, whose whole body is osStopThread, so reaching it
         * before this branch existed would have parked the thread rather
         * than shown anything. */
        ndsMenuShellRunItemSwitch();
        ndsFinishTaskmanRun();
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        return;
    case nSCKindVSOptions:
        /* P2-5u1. The gateway screen the VS menu's OPTIONS row opens, and
         * the one the Item Switch branch above returns to. Carried as an
         * NDS_SCENE_STUB in title_backend.c like its sibling, so reaching it
         * before this branch existed parked the thread rather than showing
         * anything. */
        ndsMenuShellRunVsOptions();
        ndsFinishTaskmanRun();
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        return;
    default:
        break;
    }
#endif

#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
    {
        /* The fighter packets borrow gSYFramebufferSets for the battle; give
         * the Results photo wipe back the clear it reads. The pose tick and
         * the results recorder ride inside the shared pump as its Results arm;
         * this block keeps only what a generic source menu must not run. */
        {
            extern void ndsRendererFighterPacketRelease(void);

            ndsRendererFighterPacketRelease();
        }
        ndsPlatformClearBattleTextHud();
        if (ndsSeamRunSourceMenuScene(tfunc, 1u) != FALSE)
        {
            return;
        }
#if NDS_R2_SCENE_LOOP_WALK
        /* Results leg. Closing the loop back to the menu is what makes the
         * high-water ring an N-loop reading instead of a two-entry one; the
         * START rematch above still sends Results straight to its own
         * destination when a human presses it, and that path is untouched.
         *
         * P2-1f MOVED THE TARGET. The source's Results exit goes to
         * `nSCKindPlayersVS` (mnvsresults.c:3312) and `ndsMNVSResultsSetLoadScene`
         * now transcribes that whenever the shell owns the menus, so the walk's
         * substitute leg has to arrive at the same place or the two arms of the
         * evidence would describe different flows. With the shell off there is
         * no character-select screen to arrive at, so the leg keeps VS Mode --
         * which is the scene the P2-1b walk was defined against. */
        if (ndsSceneWalkAdvance(
#if NDS_P2_MENU_SHELL
                (u32)nSCKindPlayersVS
#else
                (u32)nSCKindVSMode
#endif
            ) != FALSE)
        {
            ndsPlatformSetOriginalSpriteOverlayEnabled(FALSE);
            return;
        }
#endif
        osStopThread(NULL);
        return;
    }
#endif
#if NDS_P2_1P_GAME
    /* P2-7 item 9. The imported SOURCE menu scenes from the registry block of
     * the same gate. A source menu has no fighter packets to release, no
     * results recorder and no pose tick, so all three stay out of this path
     * (the pump runs its generic arm) -- and the source scene's own scene_curr
     * write, e.g. mnoption.c:909 back to nSCKindModeSelect, is honoured by the
     * LoadScene return. */
    switch (gSCManagerSceneData.scene_curr)
    {
    case nSCKindOption:
    case nSCKindScreenAdjust:
    case nSCKindBackupClear:
    case nSCKindSoundTest:
    case nSCKindData:
    case nSCKindVSRecord:
    case nSCKindCharacters:
    case nSCKind1PMode:
    case nSCKind1PContinue:
    case nSCKindMessage:
    /* P2-7 item 6. The attract pair runs through the same generic pump as
     * the source menus above: controller read, one task_update, audio
     * update, one scene_draw present. That honours every exit the two
     * scenes own, because each writes scene_curr itself and calls
     * syTaskmanSetLoadScene, which is what the pump's LoadScene return
     * propagates: scAutoDemoDetectExit back to nSCKindTitle (:234-237),
     * scAutoDemoExit to nSCKindStartup on the focus timeline's end (US,
     * :386-393), scExplainDetectExit back to nSCKindTitle (:591-597), and
     * the Explain phase machine on to nSCKindCharacters past phase 22
     * (:631-634, a registered row above). Neither scene touches
     * gSCManagerTransferBattleState -- scAutoDemoInitDemo (:550-551) and
     * scExplainSetBattleState (scexplain.c:153-155) point
     * gSCManagerBattleState at their own static instead -- and neither
     * passes through scVSBattleStartScene (scvsbattle.c:515), the seam
     * that would otherwise re-point battle state at the transfer block,
     * so no transfer seeding runs on this path by construction. */
    /* nSCKindAutoDemo and nSCKindExplain left this pump on 2026-09-05 for
     * the battle runner above: both put fighters on a ground, which only the
     * runner draws natively. */
#if defined(REGION_US)
    case nSCKindCongra:
#endif
    /* P2-6 (2026-09-05): the campaign's non-battle scenes, registered in the
     * same block of nds_scene_manager.c. Each writes its own scene_curr and
     * calls syTaskmanSetLoadScene at its end (the selects to the manager's
     * nSCKind1PGame, the intro to the fight, the tally to the next stage,
     * the challenger to its fight, the ending to the credits, the credits to
     * the title), which the pump's LoadScene return honours. */
    case nSCKind1PGamePlayers:
    case nSCKindPlayers1PTraining:
    case nSCKind1PBonus1Players:
    case nSCKind1PBonus2Players:
    case nSCKind1PIntro:
    case nSCKind1PChallenger:
    case nSCKind1PStageClear:
    case nSCKindEnding:
    case nSCKindStaffroll:
        if (ndsSeamRunSourceMenuScene(tfunc, 0u) != FALSE)
        {
            return;
        }
        osStopThread(NULL);
        return;
    default:
        break;
    }
#endif

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningRoom)
    {
        gNdsOpeningRoomDL0Size = sizeof(Gfx) * 1500u;
        gNdsOpeningRoomDL1Size = sizeof(Gfx) * 512u;
        gNdsOpeningRoomGraphicsHeapSize = 0x8000u;
        gNdsOpeningRoomRdpBufferSize = 0xC000u;
        gNdsOpeningRoomMallocCount =
            gNdsTaskmanMallocCount - gNdsStartupTaskmanMallocCount;
        gNdsOpeningRoomGObjCount = sGCCommonsActiveNum;
        gNdsOpeningRoomCameraCount = sGCCamerasActiveNum;

        while ((tfunc != NULL) && (tfunc->task_update != NULL) &&
               ((u32)sMVOpeningRoomTotalTimeTics <
                NDS_OPENING_ROOM_HANDOFF_TICK))
        {
            if ((u32)sMVOpeningRoomTotalTimeTics ==
                NDS_OPENING_ROOM_PRE_ASSET_TICK)
            {
                gNdsOpeningRoomPreAssetResult =
                    NDS_OPENING_ROOM_PRE_ASSET_PASS;
            }

            tfunc->task_update(tfunc);
            dSYTaskmanUpdateCount++;

            gNdsOpeningRoomTickCount = (u32)sMVOpeningRoomTotalTimeTics;
            if (sMVOpeningRoomTotalTimeTics > 0)
            {
                gNdsOpeningRoomUpdateResult = NDS_OPENING_ROOM_UPDATE_PASS;
            }
            if ((sSYTaskmanStatus != nSYTaskmanStatusLoadScene) &&
                ((u32)sMVOpeningRoomTotalTimeTics > 0u) &&
                ((u32)sMVOpeningRoomTotalTimeTics <
                 NDS_OPENING_ROOM_TICK560_RUN_TICK) &&
                (((u32)sMVOpeningRoomTotalTimeTics %
                  NDS_OPENING_MOVIE_DRAW_INTERVAL) == 0u))
            {
                ndsOpeningMoviePresentFrame();
            }
            if (((u32)sMVOpeningRoomTotalTimeTics >=
                 NDS_OPENING_ROOM_TICK560_RUN_TICK) &&
                ((((u32)sMVOpeningRoomTotalTimeTics %
                   NDS_OPENING_MOVIE_DRAW_INTERVAL) == 0u) ||
                 ((u32)sMVOpeningRoomTotalTimeTics ==
                  NDS_OPENING_ROOM_HANDOFF_TICK)))
            {
                if (ndsOpeningRoomShouldRunDrawProbe(
                        (u32)sMVOpeningRoomTotalTimeTics) != FALSE)
                {
                    ndsRunBoundedOpeningRoomDraw(tfunc);
                }
                else
                {
                    ndsReuseBoundedOpeningRoomPreview();
                }
            }
            if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
            {
                ndsFinishTaskmanRun();
                return;
            }
        }

        ndsRunBoundedOpeningRoomDraw(tfunc);

        gNdsOpeningRoomTickCount = (u32)sMVOpeningRoomTotalTimeTics;
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningPortraits)
    {
        while ((tfunc != NULL) && (tfunc->task_update != NULL) &&
               ((u32)sMVOpeningPortraitsTotalTimeTics <
                NDS_OPENING_PORTRAITS_HANDOFF_TICK))
        {
            tfunc->task_update(tfunc);
            dSYTaskmanUpdateCount++;
            ndsOpeningPortraitsRecordRunTick();
            if ((sSYTaskmanStatus == nSYTaskmanStatusLoadScene) &&
                (gSCManagerSceneData.scene_curr != nSCKindOpeningPortraits))
            {
                ndsFinishTaskmanRun();
                return;
            }
            if ((((u32)sMVOpeningPortraitsTotalTimeTics %
                  NDS_OPENING_MOVIE_DRAW_INTERVAL) == 0u) ||
                ((u32)sMVOpeningPortraitsTotalTimeTics ==
                 NDS_OPENING_PORTRAITS_HANDOFF_TICK))
            {
                ndsRunBoundedOpeningPortraitsDraw(tfunc);
            }
            if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
            {
                ndsOpeningPortraitsRecordRunTick();
                ndsFinishTaskmanRun();
                return;
            }
        }

        ndsRunBoundedOpeningPortraitsDraw(tfunc);
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (ndsOpeningIsImportedNameScene(gSCManagerSceneData.scene_curr) != FALSE)
    {
        u32 guard = 0;

        while ((tfunc != NULL) && (tfunc->task_update != NULL) &&
               (guard < (NDS_OPENING_MARIO_HANDOFF_TICK + 4u)))
        {
            tfunc->task_update(tfunc);
            guard++;
            dSYTaskmanUpdateCount++;
            if ((guard == 10u) ||
                ((guard % NDS_OPENING_MOVIE_DRAW_INTERVAL) == 0u) ||
                (guard == NDS_OPENING_MARIO_HANDOFF_TICK))
            {
                ndsRunBoundedOpeningPortraitsDraw(tfunc);
            }
            if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
            {
                ndsFinishTaskmanRun();
                return;
            }
        }

        ndsRunBoundedOpeningPortraitsDraw(tfunc);
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindTitle)
    {
        ndsMNTitleRunBoundedUpdates(1u);
        ndsTitleRenderPreview();
        gNdsOpeningMovieTitleResult = NDS_OPENING_MOVIE_TITLE_PASS;
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindVSMode)
    {
        gNdsTaskmanContexts = 2;
        gNdsTaskmanTaskGfxNum = 1;
        gNdsTaskmanGraphicsHeapSize = 0x8000;
        gNdsTaskmanRdpKind = 2;
        gNdsTaskmanRdpBufferSize = 0xC000;
        gNdsTaskmanSceneUpdateSet =
            (sSYTaskmanDefaultFunction.scene_update == gcRunAll) ? 1u : 0u;
        gNdsTaskmanSceneDrawSet =
            (sSYTaskmanDefaultFunction.scene_draw == gcDrawAll) ? 1u : 0u;
        gNdsTaskmanLightsSet =
            (sSYTaskmanDefaultFunction.task_update != NULL) ? 1u : 0u;
        gNdsTaskmanControllerAutoRead = 1;
        gNdsTaskmanDLContextsValid = 2;
        gNdsTaskmanGeneralHeapUsed =
            (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                  (uintptr_t)gSYTaskmanGeneralHeap.start);
        gNdsTaskmanLoopReached = 1;
#if (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_VS_START_TRANSITION) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_VSBATTLE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_PUPUPU_UPDATE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_MODEL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STRUCT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_INIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_TICK) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_GROUND) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DISPLAY_PROBE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_SCAN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_EXECUTE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_MULTI) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_ALL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_INPUT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_JUMP_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LANDING_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PROCESS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_SCHEDULER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_CONTROLLER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PREVIEW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP)
        ndsMNVSModeRunStartTransitionProbe();

        if ((gNdsVSModeStartTransitionResult ==
                NDS_VS_MODE_START_TRANSITION_PASS) &&
            (gSCManagerSceneData.scene_curr == nSCKindPlayersVS) &&
            (sSYTaskmanStatus == nSYTaskmanStatusLoadScene))
        {
            ndsFinishTaskmanRun();
            return;
        }
#endif
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
#if NDS_R2_SCENE_LOOP_WALK
        /* P2-1b scene-loop walk, menu leg. This bounded VS Mode branch parks
         * here in every other configuration; with the walk armed it spends a
         * hop and hands the loop to the match instead. */
        if (ndsSceneWalkAdvance((u32)nSCKindVSBattle) != FALSE)
        {
            ndsFinishTaskmanRun();
            return;
        }
#endif
        osStopThread(NULL);
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindPlayersVS)
    {
        gNdsTaskmanContexts = 2;
        gNdsTaskmanTaskGfxNum = 1;
        gNdsTaskmanGraphicsHeapSize = 0x3A98;
        gNdsTaskmanRdpKind = 2;
        gNdsTaskmanRdpBufferSize = 0x8000;
        gNdsTaskmanSceneUpdateSet =
            (sSYTaskmanDefaultFunction.scene_update == gcRunAll) ? 1u : 0u;
        gNdsTaskmanSceneDrawSet =
            (sSYTaskmanDefaultFunction.scene_draw == gcDrawAll) ? 1u : 0u;
        gNdsTaskmanLightsSet =
            (sSYTaskmanDefaultFunction.task_update != NULL) ? 1u : 0u;
        gNdsTaskmanControllerAutoRead = 1;
        gNdsTaskmanDLContextsValid = 2;
        gNdsTaskmanGeneralHeapUsed =
            (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                  (uintptr_t)gSYTaskmanGeneralHeap.start);
        gNdsTaskmanLoopReached = 1;
#if (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_VSBATTLE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_PUPUPU_UPDATE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_MODEL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STRUCT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_INIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_TICK) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_GROUND) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DISPLAY_PROBE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_SCAN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_EXECUTE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_MULTI) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_ALL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_INPUT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_JUMP_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LANDING_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PROCESS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_SCHEDULER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_CONTROLLER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PREVIEW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP)
        ndsMNPlayersVSRunReadyTransitionProbe();

        if ((gNdsPlayersVSReadyTransitionResult ==
                NDS_PLAYERS_VS_READY_TRANSITION_PASS) &&
            (gSCManagerSceneData.scene_curr == nSCKindMaps) &&
            (sSYTaskmanStatus == nSYTaskmanStatusLoadScene))
        {
            ndsFinishTaskmanRun();
            return;
        }
#endif
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindMaps)
    {
        gNdsTaskmanContexts = 2;
        gNdsTaskmanTaskGfxNum = 1;
        gNdsTaskmanGraphicsHeapSize = 0x8000;
        gNdsTaskmanRdpKind = 2;
        gNdsTaskmanRdpBufferSize = 0x8000;
        gNdsTaskmanSceneUpdateSet =
            (sSYTaskmanDefaultFunction.scene_update == gcRunAll) ? 1u : 0u;
        gNdsTaskmanSceneDrawSet =
            (sSYTaskmanDefaultFunction.scene_draw == gcDrawAll) ? 1u : 0u;
        gNdsTaskmanLightsSet =
            (sSYTaskmanDefaultFunction.task_update != NULL) ? 1u : 0u;
        gNdsTaskmanControllerAutoRead = 1;
        gNdsTaskmanDLContextsValid = 2;
        gNdsTaskmanGeneralHeapUsed =
            (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                  (uintptr_t)gSYTaskmanGeneralHeap.start);
        gNdsTaskmanLoopReached = 1;
#if (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_VSBATTLE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_PUPUPU_UPDATE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_MODEL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STRUCT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_INIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_TICK) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_GROUND) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DISPLAY_PROBE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_SCAN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_EXECUTE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_MULTI) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_ALL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_INPUT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_JUMP_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LANDING_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PROCESS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_SCHEDULER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_CONTROLLER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PREVIEW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP)
        ndsMNMapsRunSelectVSBattleProbe();

        if ((gNdsMapsSelectTransitionResult ==
                NDS_MAPS_SELECT_TRANSITION_PASS) &&
            (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
            (sSYTaskmanStatus == nSYTaskmanStatusLoadScene))
        {
            ndsFinishTaskmanRun();
            return;
        }
#endif
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    /* P2-6 (2026-09-05). The battle runner hosts every battle-like scene, not
     * only the VS match: the ladder fight (sc1pgame.c, dispatched through the
     * manager as nSCKind1PGame), the bonus boards (sc1pbonusstage.c) and
     * Training (sc1ptrainingmode.c) each set up fighters, a ground and the
     * battle HUD the way scvsbattle.c does, and each provides its own scene
     * update, which ndsSeamSceneUpdate ticks in place of the VS one. The VS
     * match's path through this block is unchanged. */
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle)
#if NDS_P2_1P_GAME
        || (gSCManagerSceneData.scene_curr == nSCKind1PGame)
        || (gSCManagerSceneData.scene_curr == nSCKind1PBonusStage)
        || (gSCManagerSceneData.scene_curr == nSCKind1PTrainingMode)
        /* The attract demo is a four-CPU battle (scautodemo.c:546-579) and
         * draws fighters and a stage, so it belongs to this runner, not to
         * the source-menu pump it was first routed through: the pump has no
         * native fighter or stage path. Its scene update is
         * scAutoDemoFuncUpdate through the same dispatch. */
        || (gSCManagerSceneData.scene_curr == nSCKindAutoDemo)
        /* How to Play likewise: two GameKey fighters on nGRKindExplain
         * (scexplain.c:151-169), a battle-shaped scene with its own update. */
        || (gSCManagerSceneData.scene_curr == nSCKindExplain)
#endif
        )
    {
#if NDS_P2_1P_GAME
        /* func_start has registered the current ground/banks and built the
         * fighters/interface. VS prepared before its BGM call; other battle
         * scenes prepare once here, before their first update or presentation.
         * Metal/Zako's later GO-time BGM call must never reset texture names. */
        if ((gNdsSceneManagerCurrIsBattle != 0u) &&
            (gSCManagerSceneData.scene_curr != (u8)nSCKindVSBattle))
        {
            ndsBattlePrepareSceneTextures();
        }
#endif
        gNdsTaskmanContexts = 2;
        gNdsTaskmanTaskGfxNum = 1;
        gNdsTaskmanGraphicsHeapSize = 0xD000;
        gNdsTaskmanRdpKind = 2;
        gNdsTaskmanRdpBufferSize = 0xC000;
        gNdsTaskmanSceneUpdateSet =
            (sSYTaskmanDefaultFunction.scene_update != NULL) ? 1u : 0u;
        gNdsTaskmanSceneDrawSet =
            (sSYTaskmanDefaultFunction.scene_draw != NULL) ? 1u : 0u;
        gNdsTaskmanLightsSet =
            (sSYTaskmanDefaultFunction.task_update != NULL) ? 1u : 0u;
        gNdsTaskmanControllerAutoRead = 1;
        gNdsTaskmanDLContextsValid = 2;
        gNdsTaskmanGeneralHeapUsed =
            (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                  (uintptr_t)gSYTaskmanGeneralHeap.start);
        gNdsTaskmanLoopReached = 1;

#if (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_LIVE_PREVIEW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE)
#if NDS_R2_PATH
        /* R2-01. src/nds/r2 owns battle scene flow. The Runtime 1 loop below
         * stays in the tree as the oracle (plan S6) but is not compiled into
         * this arm, so the two can never both drive the scene. */
        {
            ndsR2BattleRun();
        }
#else
        {
            u32 i;
            u32 update_max;
            u32 is_battle_playable =
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE);
            u32 use_realtime_presentation =
                ((is_battle_playable != 0u) &&
                 (NDS_HARNESS_FAST_LOGIC == 0));

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
            if (is_battle_playable != 0u)
            {
                ndsStageCollisionLoopPrepareRuntime();
#if NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS
                ndsAudioAssetLoadFenced();
#endif
            }
            ndsFighterMarioFoxNaturalMotionPrepare();
#if NDS_IMPORT_BATTLESHIP_IFCOMMON
            if ((is_battle_playable != 0u) &&
                (use_realtime_presentation == 0u))
            {
                gcDrawAll();
                gNdsBattlePlayablePacingDrawCalls++;
            }
#endif
            if (is_battle_playable == 0u)
            {
                update_max = NDS_FIGHTER_NATURAL_MOTION_UPDATE_MAX;
            }
            else if ((use_realtime_presentation == 0u) &&
                     (NDS_DEV_LIVE_INPUT_PREVIEW != 0))
            {
                update_max = NDS_FIGHTER_BATTLE_PLAYABLE_LIVE_UPDATE_MAX;
            }
            else if (use_realtime_presentation == 0u)
            {
                update_max = NDS_FIGHTER_BATTLE_PLAYABLE_UPDATE_MAX;
            }
            else if (NDS_DEV_LIVE_INPUT_PREVIEW != 0)
            {
                update_max = NDS_FIGHTER_BATTLE_PLAYABLE_LIVE_UPDATE_MAX;
            }
            else
            {
                update_max = NDS_FIGHTER_BATTLE_PLAYABLE_REALTIME_SMOKE_UPDATE_MAX;
            }
            if (is_battle_playable != 0u)
            {
                ndsBattlePlayablePacingStart(
                    (NDS_HARNESS_FAST_LOGIC != 0) ? 1u : 0u);
            }
            for (i = 0u; i < update_max;)
            {
                u32 updates_this_iteration = 1u;
                u32 update_in_iteration;
                u32 stop_after_iteration = 0u;
                u32 terminal_update = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                u32 profile_input_ticks = 0u;
                u32 profile_update_ticks = 0u;
                u32 profile_source_update_by_index[2] = {0u, 0u};
                u32 profile_source_update_ticks = 0u;
                u32 profile_audio_update_ticks = 0u;

                sNdsBattlePlayableProfileLoopStartTick = cpuGetTiming();
#endif
#if NDS_TICK_HUD
                sNdsBattlePlayableTickHudLoopStartTick = cpuGetTiming();
                gNdsTickHudFighterTicks = 0u;
                gNdsTickHudStageTicks = 0u;
                gNdsTickHudBackgroundTicks = 0u;
                gNdsTickHudForegroundTicks = 0u;
                gNdsTickHudAudioTicks = 0u;
                gNdsTickHudSourceTicks = 0u;
                gNdsTickHudSrcHitDetectTicks = 0u;
                gNdsTickHudSrcAnimWarmTicks = 0u;
                gNdsTickHudSrcRunAllTicks = 0u;
                gNdsTickHudSrcComputerTicks = 0u;
                gNdsTickHudSrcCatchTicks = 0u;
                gNdsTickHudSrcParamsTicks = 0u;
                gNdsTickHudSrcInterruptTicks = 0u;
                gNdsTickHudSrcPhysicsDefaultTicks = 0u;
                gNdsTickHudSrcPhysicsCaptureTicks = 0u;
                gNdsTickHudFlushTicks = 0u;
                gNdsTickHudVBlankWaitTicks = 0u;
#endif
                if ((use_realtime_presentation != 0u) &&
                    (gNdsBattlePlayablePacingRestartRequested != 0u))
                {
                    /* The lifecycle verifier requests this only after its
                     * synchronized MATCH_START stop. Reset at an iteration
                     * boundary so debugger time is never reported as a game
                     * slowdown while every natural slip remains visible. */
                    ndsBattlePlayablePacingStart(0u);
                    gNdsBattlePlayablePacingRestartRequested = 0u;
                }
                if (use_realtime_presentation != 0u)
                {
                    /* Smash 64 slows uniformly under load and never repays a
                     * missed retrace with a later logic burst. Run exactly two
                     * unchanged 60 Hz source ticks for each presented frame;
                     * a three-VBlank draw is measured as slowdown, and the
                     * next frame starts cleanly with two more ticks. */
                    updates_this_iteration =
                        NDS_BATTLE_PLAYABLE_REALTIME_UPDATES_PER_PRESENT;
                }
                for (update_in_iteration = 0u;
                     update_in_iteration < updates_this_iteration;
                     update_in_iteration++)
                {
#if NDS_R2_POSITION_PROBE
                    gNdsPositionProbeUpdateInPresent = update_in_iteration;
#endif
                    u32 battle_status_before =
                        ((is_battle_playable != 0u) &&
                         (gSCManagerBattleState != NULL)) ?
                            (u32)gSCManagerBattleState->game_status :
                            0xffffffffu;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                    u32 input_start = cpuGetTiming();
#endif
                    /* Slice 1 phase 7's gate; see ndsR2HostBattleUpdateOnce.
                     * Both loops publish it, because which one runs depends on
                     * the harness mode and a gate that is only armed on one
                     * path reads zero for the wrong reason. */
                    gNdsK0BattleInGo =
                        (battle_status_before ==
                         (u32)nSCBattleGameStatusGo) ? 1u : 0u;
                    NDS_FREEZE_DIAGNOSTICS_MARK(
                        NDS_FREEZE_BREADCRUMB_UPDATE_START);
                    if (use_realtime_presentation != 0u)
                    {
                        (void)ndsPlatformReadInput();
                        if (NDS_DEV_LIVE_INPUT_PREVIEW != 0)
                        {
                            syControllerReadDeviceData();
                            syControllerUpdateGlobalData();
                        }
                        ndsBattlePlayableAdvanceRealtimeLogicClock();
                    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                    profile_input_ticks += cpuGetTiming() - input_start;
#endif
                    /* P2-2p6: the fighter pose engine evaluates body joints on
                     * the LAST source tick of a presented frame -- the pose the
                     * draw below will show -- and holds them on the others. */
                    gNdsFtPoseEvalTick =
                        ((update_in_iteration + 1u) >= updates_this_iteration) ?
                            1u : 0u;
#if NDS_P2_LINK_BOMB_TOUR || NDS_P2_LINK_SPECIAL_TOUR
                    /* The first prepare attempt for mode-163 happens before
                     * BattleShip has published either fighter GObj.  Most
                     * legacy proof arms tolerate that because they do not
                     * require guest controller playback immediately; Link's
                     * action proofs do.  Reuse the idempotent prepare on
                     * each proof tick until the real fighters are live. */
                    ndsFighterMarioFoxNaturalMotionPrepare();
#endif
                    ndsRunMarioFoxProofUpdate(
                        &gNdsFighterGCRunAllLoopTaskmanUpdateCount);
#if NDS_R2_POSITION_PROBE
                    if ((gSCManagerBattleState != NULL) &&
                        (gSCManagerBattleState->players[0].fighter_gobj != NULL))
                    {
                        ndsPositionProbeCaptureMarioHurtboxes(
                            gSCManagerBattleState->players[0].fighter_gobj);
                    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                    profile_update_ticks +=
                        gNdsRendererProfileUpdateTicks;
                    if (update_in_iteration < 2u)
                    {
                        profile_source_update_by_index[update_in_iteration] =
                            gNdsRendererProfileSourceUpdateTicks;
                    }
                    profile_source_update_ticks +=
                        gNdsRendererProfileSourceUpdateTicks;
                    profile_audio_update_ticks +=
                        gNdsRendererProfileAudioUpdateTicks;
#endif
                    if ((battle_status_before ==
                         nSCBattleGameStatusWait) &&
                        (gSCManagerBattleState != NULL) &&
                        (gSCManagerBattleState->game_status ==
                         nSCBattleGameStatusGo))
                    {
                        /* Keep all prepare-once countdown atlases resident.
                         * Their compact allocation preserves pre-GO source-
                         * frame headroom; transition deletion only mutates a
                         * live binding after preparation has finished. */
                        ndsRendererHardwareArmBattleStaticTextures();
                    }
                    /* BattleShip syTaskmanRunTask checks LoadScene immediately
                     * after task_update and never draws the terminal update. */
                    if ((is_battle_playable != 0u) &&
                        (sSYTaskmanStatus == nSYTaskmanStatusLoadScene))
                    {
                        terminal_update = 1u;
                        break;
                    }
#if NDS_SCENE_MIP_CACHE_LAB
                    if ((is_battle_playable != 0u) &&
                        (use_realtime_presentation != 0u) &&
                        (i == 0u) &&
                        (ndsSceneMipCacheHoldLogic() != FALSE))
                    {
                        u32 seed_guard = 0u;

                        /* Let BattleShip establish the real fighter/camera
                         * state once, then seed without advancing logic or
                         * match time. Seed draws obey the same locked cap. */
                        while ((ndsSceneMipCacheHoldLogic() != FALSE) &&
                               (seed_guard < 4u))
                        {
                            ndsBattlePlayablePresentRealtimeFrame();
                            seed_guard++;
                        }
                        if (ndsSceneMipCacheHoldLogic() != FALSE)
                        {
                            ndsPlatformSceneMipCacheAbort();
                        }
                        ndsBattlePlayablePacingStart(0u);
                    }
#endif
                    if ((is_battle_playable != 0u) &&
                        (use_realtime_presentation == 0u))
                    {
                        ndsBattlePlayableAdvanceFastLogicClock();
                    }
                    if ((is_battle_playable != 0u) &&
                        (use_realtime_presentation == 0u))
                    {
                        gNdsBattlePlayablePacingLogicFrames++;
                    }
                    i++;

                    if (gNdsFighterNaturalMotionResult ==
                        NDS_FIGHTER_NATURAL_MOTION_PASS)
                    {
                        if ((is_battle_playable != 0u) &&
                            (NDS_DEV_LIVE_INPUT_PREVIEW != 0))
                        {
                            continue;
                        }
                        if ((is_battle_playable != 0u) &&
                            (use_realtime_presentation != 0u) &&
                            (i < update_max))
                        {
                            continue;
                        }
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
                        if ((is_battle_playable != 0u) &&
                            (use_realtime_presentation == 0u) &&
                            (gNdsAudioBgmElapsedFrames <
                             NDS_AUDIO_BGM_RATE_GUARD_FRAMES))
                        {
                            continue;
                        }
#endif
                        stop_after_iteration = 1u;
                        break;
                    }
                }
                if (terminal_update != 0u)
                {
                    break;
                }
#if NDS_RENDERER_HW_TRIANGLES && NDS_HARNESS_FAST_PRESENT_ON_REQUEST
                if ((is_battle_playable != 0u) &&
                    (use_realtime_presentation == 0u) &&
                    (sNdsHarnessFastPresentRequested != 0u))
                {
                    /* Renderer-coupled verification needs the same state
                     * visibility as a BattleShip presentation, but not the
                     * fast harness paying that draw on every source tick. The
                     * request is consumed once and the ordinary hardware owner
                     * performs the draw; gameplay state remains untouched. */
                    sNdsHarnessFastPresentRequested = 0u;
                    gNdsHarnessFastPresentConsumeCount++;
                    ndsFighterMarioFoxStageGCDrawAllLoopSubmitHardwareFrame();
                }
#endif
                if (use_realtime_presentation != 0u)
                {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                    gNdsRendererProfileInputTicks = profile_input_ticks;
                    gNdsRendererProfileUpdateTicks = profile_update_ticks;
                    gNdsRendererProfileSourceUpdate1Ticks =
                        profile_source_update_by_index[0];
                    gNdsRendererProfileSourceUpdate2Ticks =
                        profile_source_update_by_index[1];
                    gNdsRendererProfileSourceUpdateTicks =
                        profile_source_update_ticks;
                    gNdsRendererProfileAudioUpdateTicks =
                        profile_audio_update_ticks;
#endif
                    ndsBattlePlayablePresentRealtimeFrame();
                    /* Count only updates committed to a presented frame. The
                     * source-faithful LoadScene terminal update remains
                     * undrawn and is proven by the lifecycle counters. */
                    gNdsBattlePlayablePacingLogicFrames +=
                        NDS_BATTLE_PLAYABLE_REALTIME_UPDATES_PER_PRESENT;
                    ndsBattlePlayableFinalizePresentedIteration();
                }
                if (stop_after_iteration != 0u)
                {
                    break;
                }
            }
            if (is_battle_playable != 0u)
            {
                ndsBattlePlayableRecordLifecycleTaskmanExit();
                ndsBattlePlayablePacingFinish();
            }
#if NDS_RENDERER_HW_TRIANGLES && \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_COLLISION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE))
            if ((NDS_DEV_SCENE_HARNESS !=
                    NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE) ||
                ((NDS_HARNESS_FAST_LOGIC != 0) &&
                 (sSYTaskmanStatus != nSYTaskmanStatusLoadScene)))
            {
                ndsFighterMarioFoxStageGCDrawAllLoopSubmitHardwareFrame();
#if NDS_SHIP_TELEMETRY || (NDS_RENDERER_PROFILE_LEVEL >= 1)
                /* This submit is the bounded fast path's whole rendered
                 * frame; the realtime path publishes the same diagnostics
                 * from ndsBattlePlayablePresentFrame, which this path never
                 * calls. Without this the Fast* contract globals are never
                 * referenced, --gc-sections drops them, and the proof
                 * harness reads DWARF ghosts (FAST_FINAL poison,
                 * P2-3r3 2026-08-23). */
                ndsRendererProfileFramePublish();
#endif
            }
#endif
#else
            u32 live_update_max =
                (NDS_DEV_LIVE_INPUT_PREVIEW != 0) ?
                NDS_FIGHTER_LIVE_PREVIEW_DEV_UPDATE_MAX :
                NDS_FIGHTER_LIVE_PREVIEW_IDLE_UPDATE_MAX;

            ndsFighterMarioFoxSchedulerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterSchedulerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxSchedulerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxControllerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterControllerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxControllerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxPreviewLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterPreviewLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxPreviewLoopResult ==
                    NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxGCRunAllLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_GCRUNALL_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterGCRunAllLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxGCRunAllLoopResult ==
                    NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_PASS)
                {
                    break;
                }
            }

            if ((NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_GCDRAWALL_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_COLLISION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_TURN_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP))
            {
                ndsRunMarioFoxGCRunAllPrerequisiteLoops();
                ndsFighterMarioFoxStageCollisionLoopPrepare();
                ndsFighterMarioFoxStageFloorFollowLoopPrepare();
                ndsFighterMarioFoxStageFloorEdgeLoopPrepare();
                ndsFighterMarioFoxStageMPProcessFloorLoopPrepare();
                ndsFighterMarioFoxStageMPUpdateFloorLoopPrepare();
                ndsFighterMarioFoxStageMPSweepFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCrossFloorLoopPrepare();
                ndsFighterMarioFoxStageMPAdjustFloorLoopPrepare();
                ndsFighterMarioFoxStageMPEdgeFloorLoopPrepare();
                ndsFighterMarioFoxStageMPWallFloorLoopPrepare();
                ndsFighterMarioFoxStageMPStaleFloorLoopPrepare();
                ndsFighterMarioFoxStageMPLiveStaleFloorLoopPrepare();
                ndsFighterMarioFoxStageMPMotionStaleFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffStatusFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffTickFloorLoopPrepare();
                ndsFighterMarioFoxStageMPFallMapFloorLoopPrepare();
                ndsFighterMarioFoxStageMPFallLandFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCeilFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCeilStatusFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffCatchFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffWaitFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffAttackFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffAttackActionLoopPrepare();
                ndsFighterMarioFoxStageMPCliffCommon2LoopPrepare();
                ndsFighterMarioFoxStageMPCliffEscapeActionLoopPrepare();
                ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopPrepare();
                ndsFighterMarioFoxStageMPCliffClimbFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffClimbActionLoopPrepare();
                ndsFighterMarioFoxStageMPCliffClimbCommon2LoopPrepare();
                ndsFighterMarioFoxStageMPCliffClimbFinishLoopPrepare();
                ndsFighterMarioFoxStageMPCliffWaitDamageLoopPrepare();
                ndsFighterMarioFoxStageMPPassiveLoopPrepare();
                ndsFighterMarioFoxStageMPDamageRecoverLoopPrepare();
                ndsFighterMarioFoxStageMPLiveHitDamageLoopPrepare();
                ndsFighterMarioFoxStageMPDownWaitLoopPrepare();
                ndsFighterMarioFoxStageTurnLoopPrepare();
                ndsFighterMarioFoxStageMPDownRecoverLoopPrepare();
                ndsFighterMarioFoxStageMPCliffLedgeLoopPrepare();
                ndsFighterMarioFoxStageMPCliffLiveLoopPrepare();
                ndsFighterMarioFoxStageMPWallCopyFloorLoopPrepare();
                ndsFighterMarioFoxStageMPPassFloorLoopPrepare();
                ndsFighterMarioFoxStageMPPlatformFloorLoopPrepare();
                ndsFighterMarioFoxStageMPPlatformTickFloorLoopPrepare();
                ndsFighterMarioFoxStageMPPassInputLoopPrepare();
                ndsFighterMarioFoxStageMPPlatformPosFloorLoopPrepare();
                ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopPrepare();
                ndsFighterMarioFoxStageInishieScaleLoopPrepare();
                ndsFighterMarioFoxStageGCDrawAllLoopPrepare();
                ndsFighterMarioFoxGCDrawAllLoopPrepare();
                for (i = 0u; i < NDS_FIGHTER_GCDRAWALL_LOOP_UPDATE_MAX; i++)
                {
                    ndsSeamSceneUpdate();
                    dSYTaskmanUpdateCount++;
                    gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                    gNdsFighterGCDrawAllLoopTaskmanUpdateCount++;
                    gNdsSCVSBattleOriginalUpdateCount++;
                    gNdsSCVSBattleOriginalUpdateResult =
                        NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                    gNdsSCVSBattleOriginalSetupMask |=
                        NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                    if (gNdsFighterMarioFoxGCDrawAllLoopResult ==
                        NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_PASS)
                    {
#if (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
    (NDS_DEV_SCENE_HARNESS == \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP)
                        ndsFighterMarioFoxStageGCDrawAllLoopFinalize();
                        ndsFighterMarioFoxStageCollisionLoopFinalize();
                        ndsFighterMarioFoxStageFloorFollowLoopFinalize();
                        ndsFighterMarioFoxStageFloorEdgeLoopFinalize();
                        ndsFighterMarioFoxStageMPProcessFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPUpdateFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPSweepFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCrossFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPAdjustFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPEdgeFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPWallFloorLoopFinalize();
#endif
#if (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
    (NDS_DEV_SCENE_HARNESS == \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP)
                        if (gNdsStageMPLiveStaleFloorLoopSelectedCallbackCount ==
                            0u)
                        {
                            continue;
                        }
#endif
                        ndsFighterMarioFoxStageGCDrawAllLoopFinalize();
                        ndsFighterMarioFoxStageCollisionLoopFinalize();
                        ndsFighterMarioFoxStageFloorFollowLoopFinalize();
                        ndsFighterMarioFoxStageFloorEdgeLoopFinalize();
                        ndsFighterMarioFoxStageMPProcessFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPUpdateFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPSweepFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCrossFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPAdjustFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPEdgeFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPWallFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPStaleFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPLiveStaleFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPMotionStaleFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffStatusFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffTickFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPFallMapFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPFallLandFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCeilFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCeilStatusFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffCatchFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffWaitFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffClimbFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffClimbActionLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffClimbCommon2LoopFinalize();
                        ndsFighterMarioFoxStageMPCliffClimbFinishLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffWaitDamageLoopFinalize();
                        ndsFighterMarioFoxStageMPPassiveLoopFinalize();
                        ndsFighterMarioFoxStageMPDamageRecoverLoopFinalize();
                        ndsFighterMarioFoxStageMPLiveHitDamageLoopFinalize();
                        ndsFighterMarioFoxStageMPDownWaitLoopFinalize();
                        ndsFighterMarioFoxStageTurnLoopFinalize();
                        ndsFighterMarioFoxStageMPDownRecoverLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffLedgeLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffLiveLoopFinalize();
                        ndsFighterMarioFoxStageMPWallCopyFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPPassFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPPlatformFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPPlatformTickFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPPassInputLoopFinalize();
                        ndsFighterMarioFoxStageMPPlatformPosFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopFinalize();
                        ndsFighterMarioFoxStageInishieScaleLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffAttackFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffAttackActionLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffCommon2LoopFinalize();
                        ndsFighterMarioFoxStageMPCliffEscapeActionLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopFinalize();
                        break;
                    }
                }
                ndsFighterMarioFoxStageGCDrawAllLoopFinalize();
                ndsFighterMarioFoxStageCollisionLoopFinalize();
                ndsFighterMarioFoxStageFloorFollowLoopFinalize();
                ndsFighterMarioFoxStageFloorEdgeLoopFinalize();
                ndsFighterMarioFoxStageMPProcessFloorLoopFinalize();
                ndsFighterMarioFoxStageMPUpdateFloorLoopFinalize();
                ndsFighterMarioFoxStageMPSweepFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCrossFloorLoopFinalize();
                ndsFighterMarioFoxStageMPAdjustFloorLoopFinalize();
                ndsFighterMarioFoxStageMPEdgeFloorLoopFinalize();
                ndsFighterMarioFoxStageMPWallFloorLoopFinalize();
                ndsFighterMarioFoxStageMPStaleFloorLoopFinalize();
                ndsFighterMarioFoxStageMPLiveStaleFloorLoopFinalize();
                ndsFighterMarioFoxStageMPMotionStaleFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffStatusFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffTickFloorLoopFinalize();
                ndsFighterMarioFoxStageMPFallMapFloorLoopFinalize();
                ndsFighterMarioFoxStageMPFallLandFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCeilFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCeilStatusFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffCatchFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffWaitFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffClimbFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffClimbActionLoopFinalize();
                ndsFighterMarioFoxStageMPCliffClimbCommon2LoopFinalize();
                ndsFighterMarioFoxStageMPCliffClimbFinishLoopFinalize();
                ndsFighterMarioFoxStageMPCliffWaitDamageLoopFinalize();
                ndsFighterMarioFoxStageMPPassiveLoopFinalize();
                ndsFighterMarioFoxStageMPDamageRecoverLoopFinalize();
                ndsFighterMarioFoxStageMPLiveHitDamageLoopFinalize();
                ndsFighterMarioFoxStageMPDownWaitLoopFinalize();
                ndsFighterMarioFoxStageTurnLoopFinalize();
                ndsFighterMarioFoxStageMPDownRecoverLoopFinalize();
                ndsFighterMarioFoxStageMPCliffLedgeLoopFinalize();
                ndsFighterMarioFoxStageMPCliffLiveLoopFinalize();
                ndsFighterMarioFoxStageMPWallCopyFloorLoopFinalize();
                ndsFighterMarioFoxStageMPPassFloorLoopFinalize();
                ndsFighterMarioFoxStageMPPlatformFloorLoopFinalize();
                ndsFighterMarioFoxStageMPPlatformTickFloorLoopFinalize();
                ndsFighterMarioFoxStageMPPassInputLoopFinalize();
                ndsFighterMarioFoxStageMPPlatformPosFloorLoopFinalize();
                ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopFinalize();
                ndsFighterMarioFoxStageInishieScaleLoopFinalize();
                ndsFighterMarioFoxStageMPCliffAttackFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffAttackActionLoopFinalize();
                ndsFighterMarioFoxStageMPCliffCommon2LoopFinalize();
                ndsFighterMarioFoxStageMPCliffEscapeActionLoopFinalize();
                ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopFinalize();
            }

            if ((NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_LIVE_PREVIEW) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW))
            {
                ndsFighterMarioFoxLivePreviewPrepare();
                for (i = 0u; i < live_update_max; i++)
                {
                    ndsSeamSceneUpdate();
                    dSYTaskmanUpdateCount++;
                    gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                    gNdsFighterLivePreviewTaskmanUpdateCount++;
                    gNdsSCVSBattleOriginalUpdateCount++;
                    gNdsSCVSBattleOriginalUpdateResult =
                        NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                    gNdsSCVSBattleOriginalSetupMask |=
                        NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                    if ((NDS_DEV_LIVE_INPUT_PREVIEW == 0) &&
                        (gNdsFighterMarioFoxLivePreviewResult ==
                         NDS_FIGHTER_MARIOFOX_LIVE_PREVIEW_PASS))
                    {
                        break;
                    }
                }
            }
#endif
        }
#endif /* NDS_R2_PATH -- R2-01 selects src/nds/r2 instead of the loop above */
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_MODEL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_MODEL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STRUCT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STRUCT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_INIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_INIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WAIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WAIT_TICK) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_TICK) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WAIT_GROUND) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_GROUND) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DISPLAY_PROBE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DISPLAY_PROBE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_SCAN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_SCAN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_EXECUTE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_EXECUTE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_DRAW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_DRAW_MULTI) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_MULTI) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_DRAW_ALL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_ALL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WALK_INPUT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_INPUT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WALK_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_LOOP)
        ndsRunMarioFoxProofUpdate(NULL);
        ndsFighterMarioFoxRunImmediateProofChain();
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP)
        {
            u32 i;

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
            ndsFighterMarioFoxNaturalMotionPrepare();
            for (i = 0u; i < NDS_FIGHTER_NATURAL_MOTION_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterGCRunAllLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterNaturalMotionResult ==
                    NDS_FIGHTER_NATURAL_MOTION_PASS)
                {
                    break;
                }
            }
#else
            ndsRunMarioFoxProcessPrerequisiteLoop();
            ndsFighterMarioFoxSchedulerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterSchedulerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxSchedulerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxControllerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterControllerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxControllerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxPreviewLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterPreviewLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxPreviewLoopResult ==
                    NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxGCRunAllLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_GCRUNALL_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterGCRunAllLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxGCRunAllLoopResult ==
                    NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_PASS)
                {
                    break;
                }
            }
#endif
        }
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_PREVIEW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PREVIEW_LOOP)
        {
            u32 i;

            ndsRunMarioFoxProcessPrerequisiteLoop();
            ndsFighterMarioFoxSchedulerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterSchedulerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxSchedulerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxControllerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterControllerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxControllerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxPreviewLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterPreviewLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxPreviewLoopResult ==
                    NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS)
                {
                    break;
                }
            }
        }
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_CONTROLLER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_CONTROLLER_LOOP)
        {
            u32 i;

            ndsRunMarioFoxProcessPrerequisiteLoop();
            ndsFighterMarioFoxSchedulerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterSchedulerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxSchedulerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxControllerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterControllerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxControllerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS)
                {
                    break;
                }
            }
        }
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_SCHEDULER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_SCHEDULER_LOOP)
        {
            u32 i;

            ndsRunMarioFoxProcessPrerequisiteLoop();
            ndsFighterMarioFoxSchedulerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterSchedulerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxSchedulerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS)
                {
                    break;
                }
            }
        }
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_JUMP_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_JUMP_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_LANDING_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LANDING_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_PROCESS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PROCESS_LOOP)
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && \
    ((NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DASH_RUN) || \
     (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN))
        {
            u32 i;

            ndsFighterMarioFoxNaturalMotionPrepare();
            for (i = 0u; i < NDS_FIGHTER_NATURAL_MOTION_UPDATE_MAX; i++)
            {
                ndsSeamSceneUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterGCRunAllLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterNaturalMotionResult ==
                    NDS_FIGHTER_NATURAL_MOTION_PASS)
                {
                    break;
                }
            }
        }
#else
        ndsRunMarioFoxProcessPrerequisiteLoop();
#endif
#else
        ndsSeamSceneUpdate();
        dSYTaskmanUpdateCount++;
        gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
        gNdsSCVSBattleOriginalUpdateCount++;
        gNdsSCVSBattleOriginalUpdateResult =
            NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
        gNdsSCVSBattleOriginalSetupMask |=
            NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;
#endif

#if (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PUPUPU_UPDATE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_PUPUPU_UPDATE)
        ndsGRPupupuRunSafeUpdateProbe();
        if (gNdsPupupuUpdateResult == NDS_PUPUPU_UPDATE_PASS)
        {
            gNdsPupupuGroundDeferredMask |= 1u << 3;
            gNdsPupupuGroundDeferredMask |= 1u << 4;
        }
#endif

        ndsFinishTaskmanRun();
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;

#if NDS_DEV_LIVE_INPUT_PREVIEW
        if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
        {
            /* The original task loop returns after its cleanup tail. Resume
             * scVSBattleStartScene so it can score and select VS Results. */
            return;
        }
#endif
#if NDS_R2_SCENE_LOOP_WALK
        /* Battle leg -- and it spends NO hop, because the battle scene already
         * owns this transition and must keep owning it. decomp
         * sc/sccommon/scvsbattle.c:559 sets `scene_prev = scene_curr;
         * scene_curr = nSCKindVSResults` after taskman returns, unconditionally
         * and AFTER the Sudden Death check at :540 has had its chance to run a
         * second entry into this same scene. A walk hop here would be
         * overwritten by :560 anyway, and worse, :559 would then copy the hop's
         * VSResults into scene_prev and break the `prev == VSBattle` test
         * battleship_scvsbattle.c:486 makes. Returning instead of parking hands
         * the scene back to its own tail, so the walk gets the source's real
         * battle teardown -- Sudden Death included, which is the re-entry the
         * arena ring most needs to hold flat. */
        return;
#else
        osStopThread(NULL);
        return;
#endif
    }

    /* The real taskman scene setup populated sSYTaskmanDefaultFunction and the
     * DL/heap/RDP state during syTaskmanLoadScene; reflect it here as proof. */
    gNdsTaskmanContexts = 2;
    gNdsTaskmanTaskGfxNum = 1;
    gNdsTaskmanGraphicsHeapSize = 0x2800;
    gNdsTaskmanRdpKind = 2;
    gNdsTaskmanRdpBufferSize = 0xC000;
    gNdsTaskmanSceneUpdateSet = (sSYTaskmanDefaultFunction.scene_update == gcRunAll) ? 1u : 0u;
    gNdsTaskmanSceneDrawSet = (sSYTaskmanDefaultFunction.scene_draw == gcDrawAll) ? 1u : 0u;
    gNdsTaskmanLightsSet = (sSYTaskmanDefaultFunction.task_update != NULL) ? 1u : 0u;
    gNdsTaskmanControllerAutoRead = 1;
    gNdsTaskmanDLContextsValid = 2;
    gNdsTaskmanGeneralHeapUsed =
        (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
              (uintptr_t)gSYTaskmanGeneralHeap.start);
    /* Snapshot the real startup object state now that func_start has run. */
    ndsCaptureRealStartupState();

    /* Execute bounded real task updates (controller read + gcRunAll) and one
     * bounded startup draw. This advances the original object/update coroutine
     * and scene-transition paths without entering an unbounded renderer loop. */
    ndsRunBoundedTaskmanUpdates(tfunc);

    gNdsTaskmanBridgeResult = NDS_TASKMAN_BRIDGE_PASS;
    gNdsTaskmanLoopReached = 1;
    gNdsStartupTaskmanMallocCount = gNdsTaskmanMallocCount;
    ndsFinishTaskmanRun();
}
#include "nds_scene_harness_config.h"
