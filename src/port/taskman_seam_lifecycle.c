static void ndsDrainTaskmanQueue(OSMesgQueue *queue)
{
    while (osRecvMesg(queue, NULL, OS_MESG_NOBLOCK) != -1)
    {
    }
}

static void ndsPrepareTaskmanRun(void)
{
    D_800454BC = 0;
    ndsDrainTaskmanQueue(&sSYTaskmanContextMesgQueue);
    ndsDrainTaskmanQueue(&sSYTaskmanResetMesgQueue);
    ndsDrainTaskmanQueue(&sSYTaskmanGameTicMesgQueue);
    sSYTaskmanStatus = nSYTaskmanStatusDefault;
    sSYTaskmanFramebufferID = -1;
    gSYTaskmanTaskID = 1;
    gSYSchedulerIsCustomFramebuffer = FALSE;
    D_80046638[0] = 0;
    D_80046638[1] = 0;
}

static void ndsFinishTaskmanRun(void)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
    if (gSCManagerSceneData.scene_curr == nSCKindVSBattle)
    {
        syAudioStopBGMAll();
    }
#endif
    func_80005BFC();
    ndsDrainTaskmanQueue(&sSYTaskmanContextMesgQueue);
    ndsDrainTaskmanQueue(&sSYTaskmanResetMesgQueue);
    ndsDrainTaskmanQueue(&sSYTaskmanGameTicMesgQueue);
    syRdpSetFuncLights(NULL);
    D_800454BC = 2;

    gNdsTaskmanCleanupQueuesEmpty =
        (sSYTaskmanContextMesgQueue.validCount == 0) &&
        (sSYTaskmanResetMesgQueue.validCount == 0) &&
        (sSYTaskmanGameTicMesgQueue.validCount == 0);
    gNdsTaskmanCleanupMode = D_800454BC;
    gNdsTaskmanCleanupResult = NDS_TASKMAN_CLEANUP_PASS;
    gNdsTaskmanReturnCount++;
}

static void ndsOpeningMoviePresentFrame(void)
{
    (void)ndsPlatformReadInput();
    ndsPlatformBeginFrame();
    ndsPlatformRenderDebugHud();
    ndsPlatformEndFrame();
    gNdsFrameCounter++;
    gNdsOpeningMoviePresentFrameCount++;
}

static void ndsOpeningRoomRenderSelectedDLPreview(void);

static ub8 ndsOpeningRoomShouldRunDrawProbe(u32 tick)
{
    if (gNdsOpeningRoomDrawProbeCount == 0)
    {
        return TRUE;
    }
    if (tick >= NDS_OPENING_ROOM_HANDOFF_TICK)
    {
        return TRUE;
    }
    if (gNdsOpeningRoomDLPreviewResult != NDS_OPENING_ROOM_DL_PREVIEW_PASS)
    {
        return TRUE;
    }
    return FALSE;
}

static void ndsRunBoundedOpeningRoomDraw(struct SYTaskFunction *tfunc)
{
    gNdsOpeningRoomDrawTickCount = (u32)sMVOpeningRoomTotalTimeTics;
    gNdsOpeningRoomDrawFrameCount = dSYTaskmanFrameCount;
    gNdsOpeningRoomDrawProbeCount++;

    if ((tfunc == NULL) || (tfunc->scene_draw == NULL))
    {
        gNdsOpeningRoomDrawBlocker =
            NDS_OPENING_ROOM_DRAW_BLOCKER_NO_SCENE_DRAW;
        return;
    }

    gNdsOpeningRoomDrawResult = NDS_OPENING_ROOM_DRAW_PASS;
    tfunc->scene_draw();
    ndsOpeningRoomRenderSelectedDLPreview();
    dSYTaskmanFrameCount++;
    ndsOpeningMoviePresentFrame();
}

static void ndsReuseBoundedOpeningRoomPreview(void)
{
    gNdsOpeningRoomDrawReuseCount++;
    ndsOpeningMoviePresentFrame();
}

void ndsOpeningMovieRecordRoomHandoff(u32 tick, u32 next_scene)
{
    gNdsOpeningMovieRoomHandoffTick = tick;
    gNdsOpeningMovieRoomHandoffScene = next_scene;
    gNdsOpeningMovieRoomHandoffResult =
        NDS_OPENING_MOVIE_ROOM_HANDOFF_PASS;
}

void ndsOpeningPortraitsRecordStart(void)
{
    gNdsOpeningPortraitsDispatchCount++;
    gNdsOpeningPortraitsStartResult = NDS_OPENING_PORTRAITS_START_PASS;
}

void ndsOpeningPortraitsRecordFuncStart(void)
{
    gNdsOpeningPortraitsFuncStartResult =
        NDS_OPENING_PORTRAITS_FUNC_START_PASS;
}

void ndsOpeningPortraitsRecordRunTick(void)
{
    gNdsOpeningPortraitsTickCount = (u32)sMVOpeningPortraitsTotalTimeTics;
    if (sMVOpeningPortraitsTotalTimeTics > 0)
    {
        gNdsOpeningPortraitsUpdateResult =
            NDS_OPENING_PORTRAITS_UPDATE_PASS;
    }
    if ((sSYTaskmanStatus == nSYTaskmanStatusLoadScene) &&
        (gSCManagerSceneData.scene_curr != nSCKindOpeningPortraits))
    {
        gNdsOpeningPortraitsNextSceneKind = gSCManagerSceneData.scene_curr;
        gNdsOpeningPortraitsNextSceneResult =
            NDS_OPENING_PORTRAITS_NEXT_SCENE_PASS;
    }
}

void ndsOpeningMarioRecordStart(void)
{
    gNdsOpeningMarioDispatchCount++;
    gNdsOpeningMarioStartResult = NDS_OPENING_MARIO_START_PASS;
}

void ndsOpeningMarioRecordFuncStart(void)
{
    gNdsOpeningMarioFuncStartResult =
        NDS_OPENING_MARIO_FUNC_START_PASS;
}

void ndsOpeningMarioRecordFighterDeferred(void)
{
    gNdsOpeningMarioFighterDeferredTick =
        (u32)sMVOpeningMarioTotalTimeTics;
    gNdsOpeningMarioFighterDeferredResult =
        NDS_OPENING_MARIO_FIGHTER_DEFER_PASS;
}

void ndsOpeningMarioRecordRunTick(void)
{
    gNdsOpeningMarioTickCount = (u32)sMVOpeningMarioTotalTimeTics;
    if (sMVOpeningMarioTotalTimeTics > 0)
    {
        gNdsOpeningMarioUpdateResult = NDS_OPENING_MARIO_UPDATE_PASS;
    }
    if ((sSYTaskmanStatus == nSYTaskmanStatusLoadScene) &&
        (gSCManagerSceneData.scene_curr != nSCKindOpeningMario))
    {
        gNdsOpeningMarioNextSceneKind = gSCManagerSceneData.scene_curr;
        gNdsOpeningMarioNextSceneResult =
            NDS_OPENING_MARIO_NEXT_SCENE_PASS;
    }
}

static u32 ndsOpeningNameSceneMask(u32 scene_kind)
{
    if ((scene_kind < nSCKindOpeningMario) ||
        (scene_kind > nSCKindOpeningKirby))
    {
        return 0;
    }
    return 1u << (scene_kind - nSCKindOpeningMario);
}

static sb32 ndsOpeningIsImportedNameScene(u32 scene_kind)
{
    return (ndsOpeningNameSceneMask(scene_kind) != 0) ? TRUE : FALSE;
}

void ndsOpeningNameRecordStart(u32 scene_kind)
{
    u32 mask = ndsOpeningNameSceneMask(scene_kind);

    gNdsOpeningNameSceneDispatchCount++;
    gNdsOpeningNameSceneLastKind = scene_kind;
    gNdsOpeningNameSceneDispatchMask |= mask;
}

void ndsOpeningNameRecordFuncStart(u32 scene_kind)
{
    gNdsOpeningNameSceneLastKind = scene_kind;
    gNdsOpeningNameSceneFuncStartMask |= ndsOpeningNameSceneMask(scene_kind);
}

void ndsOpeningNameRecordFighterDeferred(u32 scene_kind, u32 tick)
{
    gNdsOpeningNameSceneLastKind = scene_kind;
    gNdsOpeningNameSceneLastTick = tick;
    gNdsOpeningNameSceneFighterDeferMask |=
        ndsOpeningNameSceneMask(scene_kind);
}

void ndsOpeningNameRecordRunTick(u32 scene_kind, u32 tick)
{
    gNdsOpeningNameSceneLastKind = scene_kind;
    gNdsOpeningNameSceneLastTick = tick;
    if (tick > 0)
    {
        gNdsOpeningNameSceneUpdateMask |= ndsOpeningNameSceneMask(scene_kind);
    }
    if ((sSYTaskmanStatus == nSYTaskmanStatusLoadScene) &&
        (gSCManagerSceneData.scene_curr != scene_kind))
    {
        gNdsOpeningNameSceneLastNextKind = gSCManagerSceneData.scene_curr;
        gNdsOpeningNameSceneNextMask |= ndsOpeningNameSceneMask(scene_kind);
    }
}

static void ndsTitleRenderPreview(void);

static void ndsRunBoundedOpeningPortraitsDraw(struct SYTaskFunction *tfunc)
{
    if ((tfunc == NULL) || (tfunc->scene_draw == NULL))
    {
        return;
    }

    tfunc->scene_draw();
    dSYTaskmanFrameCount++;
    ndsOpeningMoviePresentFrame();
}

/* DS seam: bound the original task loop and one startup draw.
 *
 * syTaskmanLoadScene calls syTaskmanRunTask(&sSYTaskmanDefaultFunction) at its
 * end (sys/taskman.c). The real RunTask would enter the per-frame
 * gcRunAll/gcDrawAll loop, which needs the threading scheduler, display-list
 * pipeline, and RSP/RDP backend that are not yet imported. This DS
 * implementation instead records that the original flow reached the loop entry,
 * snapshots the real startup state, runs a bounded number of original task
 * updates, invokes one startup scene_draw while the original N64 logo SObj is
 * still alive, mirrors the original cleanup tail, and returns to the scene
 * manager. This preserves the original control flow while keeping the draw path
 * bounded to the first imported sprite asset. */
