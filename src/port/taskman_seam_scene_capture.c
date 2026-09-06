/* Diagnostic snapshot of the real object-manager state after
 * mnStartupFuncStart ran. Every value here is read from the original object
 * manager's pools/links, proving startup built real GObj/CObj/SObj state
 * through the original gcMake and gcAdd and lbCommon paths (not a hand-written
 * bridge). */
static void ndsCaptureRealStartupState(void)
{
    GObj *wallpaper_gobj;
    GObj *actor_gobj;
    GObj *wallpaper_camera_gobj = NULL;
    GObj *default_camera_gobj = NULL;
    SObj *logo_sobj = NULL;
    CObj *default_cobj = NULL;
    GObjProcess *proc = NULL;
    s32 link;

    gNdsStartupSkipAllowWait = (u32)sMNStartupSkipAllowWait;
    gNdsStartupProceedOpening = (u32)sMNStartupIsProceedOpening;

    /* Real object counts from the original object manager free-list counters. */
    gNdsStartupGObjCreateCount = sGCCommonsActiveNum;
    gNdsStartupCameraCreateCount = sGCCamerasActiveNum;
    gNdsStartupSpriteCreateCount = sGCSpritesActiveNum;
    gNdsStartupRelocInitCount = sNdsRelocInitCount;
    gNdsStartupFadeCreateCount = gNdsLBFadeCreateCount;

    /* Find the real startup GObjs through the original object manager. The
     * actor is id 0 (gcMakeGObjSPAfter(0, ...)); the wallpaper and wallpaper
     * camera use the kind enums the original scene set. */
    actor_gobj = gcFindGObjByID(0);
    wallpaper_gobj = gcFindGObjByID(nGCCommonKindWallpaper);
    wallpaper_camera_gobj = gcFindGObjByID(nGCCommonKindWallpaperCamera);

    /* The default camera is created with id 0xffffffff via gcMakeDefaultCameraGObj;
     * locate it by scanning the camera link for the non-wallpaper CObj GObj. */
    for (link = 0; link < (s32)GC_COMMON_MAX_LINKS; link++)
    {
        GObj *gobj = gGCCommonLinks[link];
        while (gobj != NULL)
        {
            if (gobj->obj_kind == nGCCommonAppendCamera &&
                gobj->id != nGCCommonKindWallpaperCamera &&
                default_camera_gobj == NULL)
            {
                default_camera_gobj = gobj;
            }
            gobj = gobj->link_next;
        }
    }

    if (actor_gobj != NULL)
    {
        gNdsStartupActorFuncSet = (actor_gobj->func_run == mnStartupActorFuncRun) ? 1u : 0u;
    }
    if (wallpaper_gobj != NULL)
    {
        logo_sobj = SObjGetStruct(wallpaper_gobj);
        proc = wallpaper_gobj->gobjproc_head;
        gNdsStartupWallpaperDisplaySet =
            (wallpaper_gobj->proc_display == lbCommonDrawSObjAttr) &&
            (wallpaper_gobj->dl_link_id == 0) ? 1u : 0u;
    }
    if (logo_sobj != NULL)
    {
        gNdsStartupWallpaperParentValid =
            (logo_sobj->parent_gobj == wallpaper_gobj) ? 1u : 0u;
        gNdsStartupLogoPosX = (u32)(s32)logo_sobj->pos.x;
        gNdsStartupLogoPosY = (u32)(s32)logo_sobj->pos.y;
        gNdsStartupLogoFastcopyCleared =
            ((logo_sobj->sprite.attr & SP_FASTCOPY) == 0) ? 1u : 0u;
    }
    if (proc != NULL)
    {
        gNdsStartupWallpaperProcessKind = (u32)proc->kind;
        gNdsStartupWallpaperProcessPriority = (u32)proc->priority;
    }
    if (wallpaper_camera_gobj != NULL)
    {
        gNdsStartupWallpaperCameraMaskLow =
            (u32)(wallpaper_camera_gobj->camera_mask & 0xFFFFFFFFu);
    }
    if (default_camera_gobj != NULL)
    {
        default_cobj = CObjGetStruct(default_camera_gobj);
        gNdsStartupDefaultCameraColor = default_cobj->color;
    }
}

static void ndsCapturePostUpdateStartupState(void)
{
    GObj *wallpaper_gobj = gcFindGObjByID(nGCCommonKindWallpaper);

    gNdsTaskmanPostUpdateOpening = (u32)sMNStartupIsProceedOpening;
    gNdsTaskmanPostUpdateSceneKind = gSCManagerSceneData.scene_curr;
    gNdsTaskmanPostUpdateScenePrev = gSCManagerSceneData.scene_prev;
    gNdsTaskmanPostUpdateStatus = (u32)sSYTaskmanStatus;
    gNdsTaskmanPostUpdateGObjCount = sGCCommonsActiveNum;
    gNdsTaskmanPostUpdateFadeCount = gNdsLBFadeCreateCount;

    if (wallpaper_gobj != NULL)
    {
        SObj *logo_sobj = SObjGetStruct(wallpaper_gobj);

        if (logo_sobj != NULL)
        {
            gNdsTaskmanPostUpdateLogoPosX = (u32)(s32)logo_sobj->pos.x;
            gNdsTaskmanPostUpdateLogoPosY = (u32)(s32)logo_sobj->pos.y;
        }
    }
}

static void ndsRunBoundedTaskmanUpdates(struct SYTaskFunction *tfunc)
{
    u32 i;

    if ((tfunc == NULL) || (tfunc->task_update == NULL))
    {
        return;
    }
    for (i = 0; i < NDS_STARTUP_BOUNDED_UPDATES; i++)
    {
        tfunc->task_update(tfunc);
        dSYTaskmanUpdateCount++;
        gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
        gNdsTaskmanPostUpdateSkip = (u32)sMNStartupSkipAllowWait;
        ndsCapturePostUpdateStartupState();

        if (((i + 1u) == NDS_STARTUP_LOGO_DRAW_UPDATE) &&
            (tfunc->scene_draw != NULL))
        {
            gNdsStartupLogoDrawUpdateCount = dSYTaskmanUpdateCount;
            tfunc->scene_draw();
            dSYTaskmanFrameCount++;
        }
        if (((i + 1u) >= NDS_STARTUP_LOGO_DRAW_UPDATE) &&
            (((i + 1u) == NDS_STARTUP_LOGO_DRAW_UPDATE) ||
             (((i + 1u) % NDS_STARTUP_PRESENT_INTERVAL) == 0u)))
        {
            ndsOpeningMoviePresentFrame();
        }
    }
}

void ndsOpeningRoomCapturePencilsCountsBefore(void)
{
    if ((sNdsOpeningRoomPencilsCountsCaptured != 0) ||
        (gNdsOpeningRoomFirstEventDataResult !=
         NDS_OPENING_ROOM_FIRST_EVENT_DATA_PASS))
    {
        return;
    }

    sNdsOpeningRoomPencilsGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomPencilsDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomPencilsXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomPencilsAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomPencilsCountsCaptured = 1;
}

void ndsOpeningRoomCapturePencilsCreation(void)
{
    u32 mask = 0;
    DObj *dobj;

    if ((gNdsOpeningRoomFirstEventDataResult !=
         NDS_OPENING_ROOM_FIRST_EVENT_DATA_PASS) ||
        (sNdsOpeningRoomPencilsCountsCaptured == 0) ||
        (sMVOpeningRoomPencilsGObj == NULL))
    {
        return;
    }

    gNdsOpeningRoomPencilsGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomPencilsGObjsBefore;
    gNdsOpeningRoomPencilsDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomPencilsDObjsBefore;
    gNdsOpeningRoomPencilsXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomPencilsXObjsBefore;
    gNdsOpeningRoomPencilsAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomPencilsAObjsBefore;

    if ((sMVOpeningRoomPencilsGObj != NULL) &&
        (gNdsOpeningRoomPencilsGObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_GOBJ_READY;
    }
    if (gNdsOpeningRoomPencilsDObjDelta == NDS_OPENING_ROOM_PENCILS_RENDER_DOBJS)
    {
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomPencilsXObjDelta ==
        (NDS_OPENING_ROOM_PENCILS_RENDER_DOBJS * 2u))
    {
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_XOBJ_READY;
    }
    if ((sMVOpeningRoomPencilsGObj != NULL) &&
        (sMVOpeningRoomPencilsGObj->gobjproc_head != NULL) &&
        (sMVOpeningRoomPencilsGObj->gobjproc_head->exec.func ==
         mvOpeningRoomCommonProcUpdate))
    {
        gNdsOpeningRoomPencilsProcessSet = 1;
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_PROCESS_READY;
    }
    if ((sMVOpeningRoomPencilsGObj != NULL) &&
        (sMVOpeningRoomPencilsGObj->proc_display == gcDrawDObjTreeForGObj) &&
        (sMVOpeningRoomPencilsGObj->dl_link_id == 6))
    {
        gNdsOpeningRoomPencilsDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_DISPLAY_READY;
    }

    gNdsOpeningRoomPencilsDObjTreeCount = 0;
    gNdsOpeningRoomPencilsAnimRootCount = 0;
    dobj = DObjGetStruct(sMVOpeningRoomPencilsGObj);
    while (dobj != NULL)
    {
        gNdsOpeningRoomPencilsDObjTreeCount++;
        if (dobj->is_anim_root != FALSE)
        {
            gNdsOpeningRoomPencilsAnimRootCount++;
        }
        dobj = gcGetTreeDObjNext(dobj);
    }
    if ((gNdsOpeningRoomPencilsDObjTreeCount ==
         NDS_OPENING_ROOM_PENCILS_RENDER_DOBJS) &&
        (gNdsOpeningRoomPencilsAnimRootCount == 1u))
    {
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_ANIM_ROOT_READY;
    }

    gNdsOpeningRoomPencilsCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_PENCILS_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_PENCILS_CREATE_READY_MASK)
    {
        gNdsOpeningRoomPencilsCreateResult =
            NDS_OPENING_ROOM_PENCILS_CREATE_PASS;
    }
}

static sb32 ndsOpeningRoomContainsCommonGObj(GObj *target)
{
    s32 link;

    if (target == NULL)
    {
        return FALSE;
    }
    for (link = 0; link < (s32)GC_COMMON_MAX_LINKS; link++)
    {
        GObj *gobj = gGCCommonLinks[link];

        while (gobj != NULL)
        {
            if (gobj == target)
            {
                return TRUE;
            }
            gobj = gobj->link_next;
        }
    }
    return FALSE;
}

static sb32 ndsOpeningRoomContainsDLGObj(GObj *target)
{
    s32 link;

    if (target == NULL)
    {
        return FALSE;
    }
    for (link = 0; link < (s32)GC_COMMON_MAX_DLLINKS; link++)
    {
        GObj *gobj = gGCCommonDLLinks[link];

        while (gobj != NULL)
        {
            if (gobj == target)
            {
                return TRUE;
            }
            gobj = gobj->dl_link_next;
        }
    }
    return FALSE;
}

void ndsOpeningRoomRecordOverlayEject(void *gobj)
{
    GObj *overlay_gobj = (GObj*)gobj;
    u32 mask = 0;

    if (overlay_gobj == NULL)
    {
        gNdsOpeningRoomOverlayEjectUnlinkedMask = 0;
        return;
    }

    if (!ndsOpeningRoomContainsCommonGObj(overlay_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_COMMON_UNLINKED;
    }
    if (!ndsOpeningRoomContainsDLGObj(overlay_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_DL_UNLINKED;
    }

    gNdsOpeningRoomOverlayEjectUnlinkedMask = mask;
    if ((mask & NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK) ==
        NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK)
    {
        gNdsOpeningRoomOverlayEjectResult =
            NDS_OPENING_ROOM_OVERLAY_EJECT_PASS;
    }
}

void ndsOpeningRoomRecordSunlightEject(void *gobj)
{
    GObj *sunlight_gobj = (GObj*)gobj;
    u32 mask = 0;

    if (sunlight_gobj == NULL)
    {
        gNdsOpeningRoomSunlightEjectUnlinkedMask = 0;
        return;
    }

    if (!ndsOpeningRoomContainsCommonGObj(sunlight_gobj))
    {
        mask |= NDS_OPENING_ROOM_SUNLIGHT_EJECT_COMMON_UNLINKED;
    }
    if (!ndsOpeningRoomContainsDLGObj(sunlight_gobj))
    {
        mask |= NDS_OPENING_ROOM_SUNLIGHT_EJECT_DL_UNLINKED;
    }

    gNdsOpeningRoomSunlightEjectUnlinkedMask = mask;
    if ((mask & NDS_OPENING_ROOM_SUNLIGHT_EJECT_UNLINKED_MASK) ==
        NDS_OPENING_ROOM_SUNLIGHT_EJECT_UNLINKED_MASK)
    {
        gNdsOpeningRoomSunlightEjectResult =
            NDS_OPENING_ROOM_SUNLIGHT_EJECT_PASS;
    }
}

void ndsOpeningRoomCaptureCloseUpOverlayCountsBefore(void)
{
    if (sNdsOpeningRoomCloseUpOverlayCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomCloseUpOverlayGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomCloseUpOverlayCountsCaptured = 1;
}

void ndsOpeningRoomCaptureCloseUpOverlayCreation(void *gobj)
{
    GObj *overlay_gobj = (GObj*)gobj;
    u32 mask = 0;

    if ((overlay_gobj == NULL) ||
        (sNdsOpeningRoomCloseUpOverlayCountsCaptured == 0))
    {
        return;
    }

    gNdsOpeningRoomCloseUpOverlayCreateTick =
        (u32)sMVOpeningRoomTotalTimeTics;
    gNdsOpeningRoomCloseUpOverlayCreateGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomCloseUpOverlayGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomCloseUpOverlayGObjsBefore;

    if (gNdsOpeningRoomCloseUpOverlayGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_GOBJ_READY;
    }
    if ((overlay_gobj->proc_display == mvOpeningRoomCloseUpOverlayProcDisplay) &&
        (overlay_gobj->dl_link_id == 26))
    {
        gNdsOpeningRoomCloseUpOverlayDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_DISPLAY_READY;
    }
    if (gNdsOpeningRoomCloseUpOverlayAlphaInit == 0u)
    {
        mask |= NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_ALPHA_READY;
    }

    gNdsOpeningRoomCloseUpOverlayCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_READY_MASK)
    {
        gNdsOpeningRoomCloseUpOverlayCreateResult =
            NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureSunlightCountsBefore(void)
{
    if (sNdsOpeningRoomSunlightCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomSunlightGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomSunlightDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomSunlightXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomSunlightCountsCaptured = 1;
}

void ndsOpeningRoomCaptureOutsideCountsBefore(void)
{
    if (sNdsOpeningRoomOutsideCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomOutsideGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomOutsideDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomOutsideXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomOutsideCountsCaptured = 1;
}

void ndsOpeningRoomCaptureHazeCountsBefore(void)
{
    if (sNdsOpeningRoomHazeCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomHazeGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomHazeDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomHazeXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomHazeCountsCaptured = 1;
}

void ndsOpeningRoomCaptureOutsideCreation(void *gobj)
{
    GObj *outside_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;

    if ((outside_gobj == NULL) ||
        (sNdsOpeningRoomOutsideCountsCaptured == 0))
    {
        return;
    }

    dobj = DObjGetStruct(outside_gobj);
    gNdsOpeningRoomOutsideCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomOutsideGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomOutsideGObjsBefore;
    gNdsOpeningRoomOutsideDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomOutsideDObjsBefore;
    gNdsOpeningRoomOutsideXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomOutsideXObjsBefore;

    if (gNdsOpeningRoomOutsideGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_OUTSIDE_CREATE_GOBJ_READY;
    }
    if ((dobj != NULL) && (gNdsOpeningRoomOutsideDObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_OUTSIDE_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomOutsideXObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_OUTSIDE_CREATE_XOBJ_READY;
    }
    if ((outside_gobj->proc_display == gcDrawDObjDLLinksForGObj) &&
        (outside_gobj->dl_link_id == 6))
    {
        gNdsOpeningRoomOutsideDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_OUTSIDE_CREATE_DISPLAY_READY;
    }

    gNdsOpeningRoomOutsideCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_OUTSIDE_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_OUTSIDE_CREATE_READY_MASK)
    {
        gNdsOpeningRoomOutsideCreateResult =
            NDS_OPENING_ROOM_OUTSIDE_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureHazeCreation(void *gobj)
{
    GObj *haze_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;

    if ((haze_gobj == NULL) ||
        (sNdsOpeningRoomHazeCountsCaptured == 0))
    {
        return;
    }

    dobj = DObjGetStruct(haze_gobj);
    gNdsOpeningRoomHazeCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomHazeGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomHazeGObjsBefore;
    gNdsOpeningRoomHazeDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomHazeDObjsBefore;
    gNdsOpeningRoomHazeXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomHazeXObjsBefore;

    if (gNdsOpeningRoomHazeGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_HAZE_CREATE_GOBJ_READY;
    }
    if ((dobj != NULL) && (gNdsOpeningRoomHazeDObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_HAZE_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomHazeXObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_HAZE_CREATE_XOBJ_READY;
    }
    if ((haze_gobj->proc_display == gcDrawDObjDLLinksForGObj) &&
        (haze_gobj->dl_link_id == 6))
    {
        gNdsOpeningRoomHazeDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_HAZE_CREATE_DISPLAY_READY;
    }

    gNdsOpeningRoomHazeCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_HAZE_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_HAZE_CREATE_READY_MASK)
    {
        gNdsOpeningRoomHazeCreateResult =
            NDS_OPENING_ROOM_HAZE_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureSunlightCreation(void *gobj)
{
    GObj *sunlight_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;

    if ((sunlight_gobj == NULL) ||
        (sNdsOpeningRoomSunlightCountsCaptured == 0))
    {
        return;
    }

    dobj = DObjGetStruct(sunlight_gobj);
    gNdsOpeningRoomSunlightCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomSunlightGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomSunlightGObjsBefore;
    gNdsOpeningRoomSunlightDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomSunlightDObjsBefore;
    gNdsOpeningRoomSunlightXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomSunlightXObjsBefore;

    if (gNdsOpeningRoomSunlightGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_SUNLIGHT_CREATE_GOBJ_READY;
    }
    if ((dobj != NULL) && (gNdsOpeningRoomSunlightDObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_SUNLIGHT_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomSunlightXObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_SUNLIGHT_CREATE_XOBJ_READY;
    }
    if ((sunlight_gobj->proc_display == gcDrawDObjDLLinksForGObj) &&
        (sunlight_gobj->dl_link_id == 6))
    {
        gNdsOpeningRoomSunlightDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_SUNLIGHT_CREATE_DISPLAY_READY;
    }

    gNdsOpeningRoomSunlightCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_SUNLIGHT_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_SUNLIGHT_CREATE_READY_MASK)
    {
        gNdsOpeningRoomSunlightCreateResult =
            NDS_OPENING_ROOM_SUNLIGHT_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureDeskCountsBefore(void)
{
    if (sNdsOpeningRoomDeskCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomDeskGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomDeskDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomDeskXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomDeskCountsCaptured = 1;
}

void ndsOpeningRoomCaptureDeskCreation(void *gobj)
{
    GObj *desk_gobj = (GObj*)gobj;
    u32 mask = 0;

    if ((desk_gobj == NULL) ||
        (sNdsOpeningRoomDeskCountsCaptured == 0))
    {
        return;
    }

    gNdsOpeningRoomDeskCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomDeskGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomDeskGObjsBefore;
    gNdsOpeningRoomDeskDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomDeskDObjsBefore;
    gNdsOpeningRoomDeskXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomDeskXObjsBefore;

    if (gNdsOpeningRoomDeskGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_DESK_CREATE_GOBJ_READY;
    }
    if (gNdsOpeningRoomDeskDObjDelta != 0u)
    {
        mask |= NDS_OPENING_ROOM_DESK_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomDeskXObjDelta != 0u)
    {
        mask |= NDS_OPENING_ROOM_DESK_CREATE_XOBJ_READY;
    }
    if ((desk_gobj->proc_display == gcDrawDObjTreeForGObj) &&
        (desk_gobj->dl_link_id == 6))
    {
        gNdsOpeningRoomDeskDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_DESK_CREATE_DISPLAY_READY;
    }

    gNdsOpeningRoomDeskCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_DESK_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_DESK_CREATE_READY_MASK)
    {
        gNdsOpeningRoomDeskCreateResult =
            NDS_OPENING_ROOM_DESK_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureSpotlightCountsBefore(void)
{
    if (sNdsOpeningRoomSpotlightCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomSpotlightGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomSpotlightDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomSpotlightXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomSpotlightMObjsBefore = sGCMaterialsActive;
    sNdsOpeningRoomSpotlightAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomSpotlightCountsCaptured = 1;
}

void ndsOpeningRoomCaptureSpotlightCreation(void *gobj)
{
    GObj *spotlight_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;

    if ((spotlight_gobj == NULL) ||
        (sNdsOpeningRoomSpotlightCountsCaptured == 0))
    {
        return;
    }

    dobj = DObjGetStruct(spotlight_gobj);
    gNdsOpeningRoomSpotlightCreateTick = (u32)sMVOpeningRoomTotalTimeTics;
    gNdsOpeningRoomSpotlightCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomSpotlightGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomSpotlightGObjsBefore;
    gNdsOpeningRoomSpotlightDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomSpotlightDObjsBefore;
    gNdsOpeningRoomSpotlightXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomSpotlightXObjsBefore;
    gNdsOpeningRoomSpotlightMObjDelta =
        sGCMaterialsActive - sNdsOpeningRoomSpotlightMObjsBefore;
    gNdsOpeningRoomSpotlightAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomSpotlightAObjsBefore;

    if (gNdsOpeningRoomSpotlightGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_GOBJ_READY;
    }
    if ((dobj != NULL) && (gNdsOpeningRoomSpotlightDObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomSpotlightXObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_XOBJ_READY;
    }
    if (gNdsOpeningRoomSpotlightMObjDelta >= 1u)
    {
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_MOBJ_READY;
    }
    if ((spotlight_gobj->proc_display == gcDrawDObjDLHead1) &&
        (spotlight_gobj->dl_link_id == 27))
    {
        gNdsOpeningRoomSpotlightDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_DISPLAY_READY;
    }
    if ((spotlight_gobj->gobjproc_head != NULL) &&
        (spotlight_gobj->gobjproc_head->exec.func == gcPlayAnimAll))
    {
        gNdsOpeningRoomSpotlightProcessSet = 1;
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_PROCESS_READY;
    }
    if ((dobj != NULL) && (dobj->mobj != NULL))
    {
        gNdsOpeningRoomSpotlightMObjSet = 1;
        if (dobj->mobj->matanim_joint.event32 != NULL)
        {
            gNdsOpeningRoomSpotlightMatAnimSet = 1;
            mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_MATANIM_READY;
        }
    }
    if ((dobj != NULL) &&
        (dobj->scale.vec.f.x > 0.0F) &&
        (dobj->scale.vec.f.y == 1.0F) &&
        (dobj->scale.vec.f.z > 0.0F) &&
        ((dobj->translate.vec.f.x != 0.0F) ||
         (dobj->translate.vec.f.y != 0.0F) ||
         (dobj->translate.vec.f.z != 0.0F)))
    {
        gNdsOpeningRoomSpotlightPositionSet = 1;
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_POSITION_READY;
    }

    gNdsOpeningRoomSpotlightCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_SPOTLIGHT_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_SPOTLIGHT_CREATE_READY_MASK)
    {
        gNdsOpeningRoomSpotlightCreateResult =
            NDS_OPENING_ROOM_SPOTLIGHT_CREATE_PASS;
    }
}

static sb32 ndsOpeningRoomCObjViewportIsSet(CObj *cobj)
{
    return ((cobj != NULL) &&
            (cobj->viewport.vp.vscale[0] == 600) &&
            (cobj->viewport.vp.vscale[1] == 440) &&
            (cobj->viewport.vp.vtrans[0] == 640) &&
            (cobj->viewport.vp.vtrans[1] == 480) &&
            (cobj->viewport.vp.vscale[2] == 511) &&
            (cobj->viewport.vp.vtrans[2] == 511));
}

void ndsOpeningRoomCaptureScene1CameraCountsBefore(void)
{
    if (sNdsOpeningRoomScene1CameraCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomScene1CameraGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomScene1CameraCObjsBefore = sGCCamerasActiveNum;
    sNdsOpeningRoomScene1CameraXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomScene1CameraAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomScene1CameraCountsCaptured = 1;
}

void ndsOpeningRoomCaptureScene1CameraCreation(void)
{
    GObj *main_gobj = sMVOpeningRoomMainCameraGObj;
    GObj *fighter_gobj = sMVOpeningRoomFighterCameraGObj;
    CObj *main_cobj;
    CObj *fighter_cobj;
    u32 mask = 0;

    if ((main_gobj == NULL) || (fighter_gobj == NULL) ||
        (sNdsOpeningRoomScene1CameraCountsCaptured == 0))
    {
        return;
    }

    main_cobj = CObjGetStruct(main_gobj);
    fighter_cobj = CObjGetStruct(fighter_gobj);
    gNdsOpeningRoomScene1CameraCreateGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomScene1CameraGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomScene1CameraGObjsBefore;
    gNdsOpeningRoomScene1CameraCObjDelta =
        sGCCamerasActiveNum - sNdsOpeningRoomScene1CameraCObjsBefore;
    gNdsOpeningRoomScene1CameraXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomScene1CameraXObjsBefore;
    gNdsOpeningRoomScene1CameraAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomScene1CameraAObjsBefore;

    if (gNdsOpeningRoomScene1CameraGObjDelta == 2u)
    {
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_MAIN_GOBJ_READY;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_FIGHTER_GOBJ_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        (gNdsOpeningRoomScene1CameraCObjDelta == 2u))
    {
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_COBJ_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        (gNdsOpeningRoomScene1CameraXObjDelta == 4u) &&
        (main_cobj->xobjs_num == 2) &&
        (fighter_cobj->xobjs_num == 2))
    {
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_XOBJ_READY;
    }
    if ((main_gobj->proc_display == func_80017EC0) &&
        (main_gobj->dl_link_priority == 80) &&
        (main_gobj->camera_mask == (1ULL << 6)) &&
        (fighter_gobj->proc_display == func_80017EC0) &&
        (fighter_gobj->dl_link_priority == 40) &&
        (fighter_gobj->camera_mask == ((1ULL << 27) | (1ULL << 9))))
    {
        gNdsOpeningRoomScene1CameraDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_DISPLAY_READY;
    }
    if ((main_gobj->gobjproc_head != NULL) &&
        (main_gobj->gobjproc_head->exec.func == gcPlayCamAnim) &&
        (fighter_gobj->gobjproc_head != NULL) &&
        (fighter_gobj->gobjproc_head->exec.func == gcPlayCamAnim))
    {
        gNdsOpeningRoomScene1CameraProcessSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_PROCESS_READY;
    }
    if ((main_cobj != NULL) &&
        (main_cobj->camanim_joint.event32 != NULL) &&
        (fighter_cobj != NULL) &&
        (fighter_cobj->camanim_joint.event32 != NULL))
    {
        gNdsOpeningRoomScene1CameraAnimSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_CAMANIM_READY;
    }
    if (ndsOpeningRoomCObjViewportIsSet(main_cobj) &&
        ndsOpeningRoomCObjViewportIsSet(fighter_cobj) &&
        (main_cobj->projection.persp.near == 80.0F) &&
        (main_cobj->projection.persp.far == 15000.0F) &&
        (fighter_cobj->projection.persp.near == 80.0F) &&
        (fighter_cobj->projection.persp.far == 15000.0F))
    {
        gNdsOpeningRoomScene1CameraViewportSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_VIEWPORT_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        ((main_cobj->flags & COBJ_FLAG_DLBUFFERS) != 0) &&
        ((fighter_cobj->flags & COBJ_FLAG_DLBUFFERS) != 0))
    {
        gNdsOpeningRoomScene1CameraDLBufferSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_DLBUFFER_READY;
    }

    gNdsOpeningRoomScene1CameraCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_READY_MASK)
    {
        gNdsOpeningRoomScene1CameraCreateResult =
            NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureScene2CameraEjectBefore(void)
{
    if (sNdsOpeningRoomScene2CameraEjectCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomScene2EjectMainCameraGObj = sMVOpeningRoomMainCameraGObj;
    sNdsOpeningRoomScene2EjectFighterCameraGObj = sMVOpeningRoomFighterCameraGObj;
    gNdsOpeningRoomScene2CameraEjectBeforeGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomScene2CameraEjectBeforeCameraCount = sGCCamerasActiveNum;
    sNdsOpeningRoomScene2CameraEjectCaptured = 1;
}

void ndsOpeningRoomRecordScene2CameraEject(void)
{
    GObj *main_gobj = sNdsOpeningRoomScene2EjectMainCameraGObj;
    GObj *fighter_gobj = sNdsOpeningRoomScene2EjectFighterCameraGObj;
    u32 mask = 0;

    if ((main_gobj == NULL) || (fighter_gobj == NULL) ||
        (sNdsOpeningRoomScene2CameraEjectCaptured == 0))
    {
        return;
    }

    gNdsOpeningRoomScene2CameraEjectAfterGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomScene2CameraEjectAfterCameraCount = sGCCamerasActiveNum;

    if (!ndsOpeningRoomContainsCommonGObj(main_gobj) &&
        (main_gobj->obj_kind == nGCCommonAppendNone) &&
        (main_gobj->obj == NULL))
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_MAIN_UNLINKED;
    }
    if (!ndsOpeningRoomContainsCommonGObj(fighter_gobj) &&
        (fighter_gobj->obj_kind == nGCCommonAppendNone) &&
        (fighter_gobj->obj == NULL))
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_FIGHTER_UNLINKED;
    }
    if (gNdsOpeningRoomScene2CameraEjectBeforeCameraCount ==
        (gNdsOpeningRoomScene2CameraEjectAfterCameraCount + 2u))
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_COBJ_FREED;
    }

    gNdsOpeningRoomScene2CameraEjectMask = mask;
    if ((mask & NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_READY_MASK) ==
        NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_READY_MASK)
    {
        gNdsOpeningRoomScene2CameraEjectResult =
            NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_PASS;
    }
}

void ndsOpeningRoomCaptureScene2CameraCountsBefore(void)
{
    if (sNdsOpeningRoomScene2CameraCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomScene2CameraGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomScene2CameraCObjsBefore = sGCCamerasActiveNum;
    sNdsOpeningRoomScene2CameraXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomScene2CameraAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomScene2CameraCountsCaptured = 1;
}

void ndsOpeningRoomCaptureScene2CameraCreation(void)
{
    GObj *main_gobj = sMVOpeningRoomMainCameraGObj;
    GObj *fighter_gobj = sMVOpeningRoomFighterCameraGObj;
    CObj *main_cobj;
    CObj *fighter_cobj;
    u32 mask = 0;

    if ((main_gobj == NULL) || (fighter_gobj == NULL) ||
        (sNdsOpeningRoomScene2CameraCountsCaptured == 0))
    {
        return;
    }

    main_cobj = CObjGetStruct(main_gobj);
    fighter_cobj = CObjGetStruct(fighter_gobj);
    gNdsOpeningRoomScene2CameraCreateGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomScene2CameraGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomScene2CameraGObjsBefore;
    gNdsOpeningRoomScene2CameraCObjDelta =
        sGCCamerasActiveNum - sNdsOpeningRoomScene2CameraCObjsBefore;
    gNdsOpeningRoomScene2CameraXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomScene2CameraXObjsBefore;
    gNdsOpeningRoomScene2CameraAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomScene2CameraAObjsBefore;

    if (gNdsOpeningRoomScene2CameraGObjDelta == 2u)
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_MAIN_GOBJ_READY;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_FIGHTER_GOBJ_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        (gNdsOpeningRoomScene2CameraCObjDelta == 2u))
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_COBJ_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        (gNdsOpeningRoomScene2CameraXObjDelta == 4u) &&
        (main_cobj->xobjs_num == 2) &&
        (fighter_cobj->xobjs_num == 2))
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_XOBJ_READY;
    }
    if ((main_gobj->proc_display == func_80017EC0) &&
        (main_gobj->dl_link_priority == 80) &&
        (main_gobj->camera_mask == (1ULL << 6)) &&
        (fighter_gobj->proc_display == func_80017EC0) &&
        (fighter_gobj->dl_link_priority == 40) &&
        (fighter_gobj->camera_mask == ((1ULL << 27) | (1ULL << 9))))
    {
        gNdsOpeningRoomScene2CameraDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_DISPLAY_READY;
    }
    if ((main_gobj->gobjproc_head != NULL) &&
        (main_gobj->gobjproc_head->exec.func == gcPlayCamAnim) &&
        (fighter_gobj->gobjproc_head != NULL) &&
        (fighter_gobj->gobjproc_head->exec.func == gcPlayCamAnim))
    {
        gNdsOpeningRoomScene2CameraProcessSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_PROCESS_READY;
    }
    if ((main_cobj != NULL) &&
        (main_cobj->camanim_joint.event32 != NULL) &&
        (fighter_cobj != NULL) &&
        (fighter_cobj->camanim_joint.event32 != NULL))
    {
        gNdsOpeningRoomScene2CameraAnimSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_CAMANIM_READY;
    }
    if (ndsOpeningRoomCObjViewportIsSet(main_cobj) &&
        ndsOpeningRoomCObjViewportIsSet(fighter_cobj))
    {
        gNdsOpeningRoomScene2CameraViewportSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_VIEWPORT_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        ((main_cobj->flags & COBJ_FLAG_DLBUFFERS) != 0) &&
        ((fighter_cobj->flags & COBJ_FLAG_DLBUFFERS) != 0))
    {
        gNdsOpeningRoomScene2CameraDLBufferSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_DLBUFFER_READY;
    }

    gNdsOpeningRoomScene2CameraCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_READY_MASK)
    {
        gNdsOpeningRoomScene2CameraCreateResult =
            NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureCloseUpOverlayCameraCountsBefore(void)
{
    if (sNdsOpeningRoomCloseUpOverlayCameraCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomCloseUpOverlayCameraGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomCloseUpOverlayCameraCObjsBefore = sGCCamerasActiveNum;
    sNdsOpeningRoomCloseUpOverlayCameraXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomCloseUpOverlayCameraCountsCaptured = 1;
}

void ndsOpeningRoomCaptureCloseUpOverlayCameraCreation(void *gobj)
{
    GObj *camera_gobj = (GObj*)gobj;
    CObj *cobj;
    u32 mask = 0;

    if ((camera_gobj == NULL) ||
        (sNdsOpeningRoomCloseUpOverlayCameraCountsCaptured == 0))
    {
        return;
    }

    cobj = CObjGetStruct(camera_gobj);
    gNdsOpeningRoomCloseUpOverlayCameraCreateGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomCloseUpOverlayCameraGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomCloseUpOverlayCameraGObjsBefore;
    gNdsOpeningRoomCloseUpOverlayCameraCObjDelta =
        sGCCamerasActiveNum - sNdsOpeningRoomCloseUpOverlayCameraCObjsBefore;
    gNdsOpeningRoomCloseUpOverlayCameraXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomCloseUpOverlayCameraXObjsBefore;

    if (gNdsOpeningRoomCloseUpOverlayCameraGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_GOBJ_READY;
    }
    if ((cobj != NULL) &&
        (gNdsOpeningRoomCloseUpOverlayCameraCObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_COBJ_READY;
    }
    if (gNdsOpeningRoomCloseUpOverlayCameraXObjDelta == 0u)
    {
        mask |= NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_XOBJ_READY;
    }
    if ((camera_gobj->proc_display == lbCommonDrawSprite) &&
        (camera_gobj->dl_link_priority == 60) &&
        (camera_gobj->camera_mask == (1ULL << 26)))
    {
        gNdsOpeningRoomCloseUpOverlayCameraDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_DISPLAY_READY;
    }
    if (ndsOpeningRoomCObjViewportIsSet(cobj))
    {
        gNdsOpeningRoomCloseUpOverlayCameraViewportSet = 1;
        mask |= NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_VIEWPORT_READY;
    }

    gNdsOpeningRoomCloseUpOverlayCameraCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_READY_MASK)
    {
        gNdsOpeningRoomCloseUpOverlayCameraCreateResult =
            NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureWallpaperCameraCountsBefore(void)
{
    if (sNdsOpeningRoomWallpaperCameraCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomWallpaperCameraGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomWallpaperCameraCObjsBefore = sGCCamerasActiveNum;
    sNdsOpeningRoomWallpaperCameraXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomWallpaperCameraCountsCaptured = 1;
}

void ndsOpeningRoomCaptureWallpaperCameraCreation(void *gobj)
{
    GObj *camera_gobj = (GObj*)gobj;
    CObj *cobj;
    u32 mask = 0;

    if ((camera_gobj == NULL) ||
        (sNdsOpeningRoomWallpaperCameraCountsCaptured == 0))
    {
        return;
    }

    cobj = CObjGetStruct(camera_gobj);
    gNdsOpeningRoomWallpaperCameraCreateGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomWallpaperCameraGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomWallpaperCameraGObjsBefore;
    gNdsOpeningRoomWallpaperCameraCObjDelta =
        sGCCamerasActiveNum - sNdsOpeningRoomWallpaperCameraCObjsBefore;
    gNdsOpeningRoomWallpaperCameraXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomWallpaperCameraXObjsBefore;

    if (gNdsOpeningRoomWallpaperCameraGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_GOBJ_READY;
    }
    if ((cobj != NULL) && (gNdsOpeningRoomWallpaperCameraCObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_COBJ_READY;
    }
    if (gNdsOpeningRoomWallpaperCameraXObjDelta == 0u)
    {
        mask |= NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_XOBJ_READY;
    }
    if ((camera_gobj->proc_display == lbCommonDrawSprite) &&
        (camera_gobj->dl_link_priority == 90) &&
        (camera_gobj->camera_mask == (1ULL << 28)))
    {
        gNdsOpeningRoomWallpaperCameraDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_DISPLAY_READY;
    }
    if ((cobj != NULL) &&
        (cobj->viewport.vp.vscale[0] == 600) &&
        (cobj->viewport.vp.vscale[1] == 440) &&
        (cobj->viewport.vp.vtrans[0] == 640) &&
        (cobj->viewport.vp.vtrans[1] == 480) &&
        (cobj->viewport.vp.vscale[2] == 511) &&
        (cobj->viewport.vp.vtrans[2] == 511))
    {
        gNdsOpeningRoomWallpaperCameraViewportSet = 1;
        mask |= NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_VIEWPORT_READY;
    }

    gNdsOpeningRoomWallpaperCameraCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_READY_MASK)
    {
        gNdsOpeningRoomWallpaperCameraCreateResult =
            NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureLogoCameraCountsBefore(void)
{
    if (sNdsOpeningRoomLogoCameraCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomLogoCameraGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomLogoCameraCObjsBefore = sGCCamerasActiveNum;
    sNdsOpeningRoomLogoCameraXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomLogoCameraAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomLogoCameraCountsCaptured = 1;
}

void ndsOpeningRoomCaptureLogoCameraCreation(void *gobj)
{
    GObj *camera_gobj = (GObj*)gobj;
    CObj *cobj;
    u32 mask = 0;

    if ((camera_gobj == NULL) ||
        (sNdsOpeningRoomLogoCameraCountsCaptured == 0))
    {
        return;
    }

    cobj = CObjGetStruct(camera_gobj);
    gNdsOpeningRoomLogoCameraCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomLogoCameraGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomLogoCameraGObjsBefore;
    gNdsOpeningRoomLogoCameraCObjDelta =
        sGCCamerasActiveNum - sNdsOpeningRoomLogoCameraCObjsBefore;
    gNdsOpeningRoomLogoCameraXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomLogoCameraXObjsBefore;
    gNdsOpeningRoomLogoCameraAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomLogoCameraAObjsBefore;

    if (gNdsOpeningRoomLogoCameraGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_GOBJ_READY;
    }
    if ((cobj != NULL) && (gNdsOpeningRoomLogoCameraCObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_COBJ_READY;
    }
    if (gNdsOpeningRoomLogoCameraXObjDelta == 2u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_XOBJ_READY;
    }
    if ((camera_gobj->proc_display == func_80017EC0) &&
        (camera_gobj->dl_link_priority == 50) &&
        (camera_gobj->camera_mask == (1ULL << 29)))
    {
        gNdsOpeningRoomLogoCameraDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_DISPLAY_READY;
    }
    if ((camera_gobj->gobjproc_head != NULL) &&
        (camera_gobj->gobjproc_head->exec.func == gcPlayCamAnim))
    {
        gNdsOpeningRoomLogoCameraProcessSet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_PROCESS_READY;
    }
    if ((cobj != NULL) && (cobj->camanim_joint.event32 != NULL))
    {
        gNdsOpeningRoomLogoCameraAnimSet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_CAMANIM_READY;
    }
    if ((cobj != NULL) &&
        (cobj->viewport.vp.vscale[0] == 600) &&
        (cobj->viewport.vp.vscale[1] == 440) &&
        (cobj->viewport.vp.vtrans[0] == 640) &&
        (cobj->viewport.vp.vtrans[1] == 480) &&
        (cobj->viewport.vp.vscale[2] == 511) &&
        (cobj->viewport.vp.vtrans[2] == 511))
    {
        gNdsOpeningRoomLogoCameraViewportSet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_VIEWPORT_READY;
    }

    gNdsOpeningRoomLogoCameraCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_READY_MASK)
    {
        gNdsOpeningRoomLogoCameraCreateResult =
            NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureLogoCountsBefore(void)
{
    if (sNdsOpeningRoomLogoCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomLogoGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomLogoDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomLogoXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomLogoMObjsBefore = sGCMaterialsActive;
    sNdsOpeningRoomLogoAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomLogoCountsCaptured = 1;
}

void ndsOpeningRoomCaptureLogoCreation(void *gobj)
{
    GObj *logo_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;
    u32 dobj_count = 0;
    u32 mobj_count = 0;
    u32 matanim_count = 0;

    if ((logo_gobj == NULL) || (sNdsOpeningRoomLogoCountsCaptured == 0))
    {
        return;
    }

    gNdsOpeningRoomLogoCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomLogoGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomLogoGObjsBefore;
    gNdsOpeningRoomLogoDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomLogoDObjsBefore;
    gNdsOpeningRoomLogoXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomLogoXObjsBefore;
    gNdsOpeningRoomLogoMObjDelta =
        sGCMaterialsActive - sNdsOpeningRoomLogoMObjsBefore;
    gNdsOpeningRoomLogoAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomLogoAObjsBefore;

    if (gNdsOpeningRoomLogoGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_GOBJ_READY;
    }
    if (gNdsOpeningRoomLogoDObjDelta == 2u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomLogoXObjDelta == 4u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_XOBJ_READY;
    }
    if (gNdsOpeningRoomLogoMObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_MOBJ_READY;
    }
    if ((logo_gobj->proc_display == gcDrawDObjTreeDLLinksForGObj) &&
        (logo_gobj->dl_link_id == 29))
    {
        gNdsOpeningRoomLogoDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_DISPLAY_READY;
    }

    dobj = DObjGetStruct(logo_gobj);
    while (dobj != NULL)
    {
        dobj_count++;
        if (dobj->mobj != NULL)
        {
            mobj_count++;
            if (dobj->mobj->matanim_joint.event32 != NULL)
            {
                matanim_count++;
            }
        }
        dobj = gcGetTreeDObjNext(dobj);
    }
    if ((dobj_count == 2u) && (mobj_count == 1u))
    {
        gNdsOpeningRoomLogoMObjSet = 1;
    }
    if (matanim_count == 1u)
    {
        gNdsOpeningRoomLogoMatAnimSet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_MATANIM_READY;
    }

    gNdsOpeningRoomLogoCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_LOGO_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_LOGO_CREATE_READY_MASK)
    {
        gNdsOpeningRoomLogoCreateResult = NDS_OPENING_ROOM_LOGO_CREATE_PASS;
    }
}

void ndsOpeningRoomRecordLogoEject(void *gobj)
{
    GObj *logo_gobj = (GObj*)gobj;
    u32 mask = 0;

    if (logo_gobj == NULL)
    {
        gNdsOpeningRoomLogoEjectUnlinkedMask = 0;
        return;
    }

    if (!ndsOpeningRoomContainsCommonGObj(logo_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_COMMON_UNLINKED;
    }
    if (!ndsOpeningRoomContainsDLGObj(logo_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_DL_UNLINKED;
    }

    gNdsOpeningRoomLogoEjectUnlinkedMask = mask;
    if ((mask & NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK) ==
        NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK)
    {
        gNdsOpeningRoomLogoEjectResult = NDS_OPENING_ROOM_LOGO_EJECT_PASS;
    }
}

void ndsOpeningRoomCaptureBossShadowCountsBefore(void)
{
    if (sNdsOpeningRoomBossShadowCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomBossShadowGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomBossShadowDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomBossShadowXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomBossShadowAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomBossShadowCountsCaptured = 1;
}

void ndsOpeningRoomCaptureBossShadowCreation(void *gobj)
{
    GObj *boss_shadow_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;

    if ((boss_shadow_gobj == NULL) ||
        (sNdsOpeningRoomBossShadowCountsCaptured == 0))
    {
        return;
    }

    dobj = DObjGetStruct(boss_shadow_gobj);
    gNdsOpeningRoomBossShadowCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomBossShadowGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomBossShadowGObjsBefore;
    gNdsOpeningRoomBossShadowDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomBossShadowDObjsBefore;
    gNdsOpeningRoomBossShadowXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomBossShadowXObjsBefore;
    gNdsOpeningRoomBossShadowAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomBossShadowAObjsBefore;

    if (gNdsOpeningRoomBossShadowGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_GOBJ_READY;
    }
    if ((dobj != NULL) && (gNdsOpeningRoomBossShadowDObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomBossShadowXObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_XOBJ_READY;
    }
    if ((boss_shadow_gobj->gobjproc_head != NULL) &&
        (boss_shadow_gobj->gobjproc_head->exec.func == gcPlayAnimAll))
    {
        gNdsOpeningRoomBossShadowProcessSet = 1;
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_PROCESS_READY;
    }
    if ((boss_shadow_gobj->proc_display == gcDrawDObjDLHead1) &&
        (boss_shadow_gobj->dl_link_id == 9))
    {
        gNdsOpeningRoomBossShadowDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_DISPLAY_READY;
    }
    if ((dobj != NULL) && (dobj->anim_joint.event32 != NULL))
    {
        gNdsOpeningRoomBossShadowAnimSet = 1;
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_ANIM_READY;
    }

    gNdsOpeningRoomBossShadowCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_READY_MASK)
    {
        gNdsOpeningRoomBossShadowCreateResult =
            NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_PASS;
    }
}

void ndsOpeningRoomRecordBossShadowEject(void *gobj)
{
    GObj *boss_shadow_gobj = (GObj*)gobj;
    u32 mask = 0;

    if (boss_shadow_gobj == NULL)
    {
        gNdsOpeningRoomBossShadowEjectUnlinkedMask = 0;
        return;
    }

    if (!ndsOpeningRoomContainsCommonGObj(boss_shadow_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_COMMON_UNLINKED;
    }
    if (!ndsOpeningRoomContainsDLGObj(boss_shadow_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_DL_UNLINKED;
    }

    gNdsOpeningRoomBossShadowEjectUnlinkedMask = mask;
    if ((mask & NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK) ==
        NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK)
    {
        gNdsOpeningRoomBossShadowEjectResult =
            NDS_OPENING_ROOM_BOSS_SHADOW_EJECT_PASS;
    }
}
